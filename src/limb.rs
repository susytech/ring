// Copyleft 2016 David Judd.
// Copyleft 2016 Brian Smith.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND AND THE AUTHORS DISCLAIM ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MSRCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
// SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHSOPHY IN AN ACTION
// OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

//! Unsigned multi-precision integer arithmetic.
//!
//! Limbs ordered least-significant-limb to most-significant-limb. The bits
//! limbs use the native endianness.

#![cfg_attr(not(feature = "use_heap"), allow(dead_code))]

use {polyfill, c, error, untrusted};

// XXX: Not correct for x32 ABIs.
#[cfg(target_pointer_width = "64")] pub type Limb = u64;
#[cfg(target_pointer_width = "32")] pub type Limb = u32;
#[cfg(target_pointer_width = "64")] pub const LIMB_BITS: usize = 64;
#[cfg(target_pointer_width = "32")] pub const LIMB_BITS: usize = 32;

#[cfg(target_pointer_width = "64")]
#[allow(trivial_numeric_casts)] // XXX: workaround compiler bug.
#[derive(Debug, PartialEq)]
#[repr(u64)]
pub enum LimbMask {
    True = 0xffff_ffff_ffff_ffff,
    False = 0,
}

#[cfg(target_pointer_width = "32")]
#[allow(trivial_numeric_casts)] // XXX: workaround compiler bug.
#[derive(Debug, PartialEq)]
#[repr(u32)]
pub enum LimbMask {
    True = 0xffff_ffff,
    False = 0,
}

pub const LIMB_BYTES: usize = (LIMB_BITS + 7) / 8;

#[cfg(all(any(test, feature = "rsa_signing"), target_pointer_width = "64"))]
#[inline]
pub fn limbs_as_bytes<'a>(src: &'a [Limb]) -> &'a [u8] {
    polyfill::slice::u64_as_u8(src)
}

#[cfg(all(feature = "rsa_signing", target_pointer_width = "64"))]
#[inline]
pub fn limbs_as_bytes_mut<'a>(src: &'a mut [Limb]) -> &'a mut [u8] {
    polyfill::slice::u64_as_u8_mut(src)
}

#[cfg(all(any(test, feature = "rsa_signing"), target_pointer_width = "32"))]
#[inline]
pub fn limbs_as_bytes<'a>(src: &'a [Limb]) -> &'a [u8] {
    polyfill::slice::u32_as_u8(src)
}

#[cfg(all(feature = "rsa_signing", target_pointer_width = "32"))]
#[inline]
pub fn limbs_as_bytes_mut<'a>(src: &'a mut [Limb]) -> &'a mut [u8] {
    polyfill::slice::u32_as_u8_mut(src)
}

#[inline]
pub fn limbs_less_than_limbs_consttime(a: &[Limb], b: &[Limb]) -> LimbMask {
    assert_eq!(a.len(), b.len());
    unsafe { LIMBS_less_than(a.as_ptr(), b.as_ptr(), b.len()) }
}

#[inline]
pub fn limbs_less_than_limbs_vartime(a: &[Limb], b: &[Limb]) -> bool {
    limbs_less_than_limbs_consttime(a, b) == LimbMask::True
}

#[inline]
pub fn limbs_are_zero_constant_time(limbs: &[Limb]) -> LimbMask {
    unsafe { LIMBS_are_zero(limbs.as_ptr(), limbs.len()) }
}

/// Equivalent to `if (r >= m) { r -= m; }`
#[inline]
pub fn limbs_reduce_once_constant_time(r: &mut [Limb], m: &[Limb]) {
    assert_eq!(r.len(), m.len());
    unsafe { LIMBS_reduce_once(r.as_mut_ptr(), m.as_ptr(), m.len()) };
}

#[derive(Clone, Copy, PartialEq)]
pub enum AllowZero {
    No,
    Yes
}

/// Parses `input` into `result`, reducing it via conditional subtraction
/// (mod `m`). Assuming 2**((self.num_limbs * LIMB_BITS) - 1) < m and
/// m < 2**(self.num_limbs * LIMB_BITS), the value will be reduced mod `m` in
/// constant time so that the result is in the range [0, m) if `allow_zero` is
/// `AllowZero::Yes`, or [1, m) if `allow_zero` is `AllowZero::No`. `result` is
/// padded with zeros to its length.
pub fn parse_big_endian_in_range_partially_reduced_and_pad_consttime(
        input: untrusted::Input, allow_zero: AllowZero, m: &[Limb],
        result: &mut [Limb]) -> Result<(), error::Unspecified> {
    parse_big_endian_and_pad_consttime(input, result)?;
    limbs_reduce_once_constant_time(result, m);
    if allow_zero != AllowZero::Yes {
        if limbs_are_zero_constant_time(&result) != LimbMask::False {
            return Err(error::Unspecified);
        }
    }
    Ok(())
}

/// Parses `input` into `result`, verifies that the value is less than
/// `max_exclusive`, and pads `result` with zeros to its length. If `allow_zero`
/// is not `AllowZero::Yes`, zero values are rejected.
///
/// This attempts to be constant-time with respect to the actual value *only if*
/// the value is actually in range. In other words, this won't leak anything
/// about a valid value, but it might leak small amounts of information about an
/// invalid value (which constraint it failed).
pub fn parse_big_endian_in_range_and_pad_consttime(
        input: untrusted::Input, allow_zero: AllowZero, max_exclusive: &[Limb],
        result: &mut [Limb]) -> Result<(), error::Unspecified> {
    parse_big_endian_and_pad_consttime(input, result)?;
    if limbs_less_than_limbs_consttime(&result, max_exclusive) !=
            LimbMask::True {
        return Err(error::Unspecified);
    }
    if allow_zero != AllowZero::Yes {
        if limbs_are_zero_constant_time(&result) != LimbMask::False {
            return Err(error::Unspecified);
        }
    }
    Ok(())
}

/// Parses `input` into `result`, padding `result` with zeros to its length.
/// This attempts to be constant-time with respect to the value but not with
/// respect to the length; it is assumed that the length is public knowledge.
pub fn parse_big_endian_and_pad_consttime(
        input: untrusted::Input, result: &mut [Limb])
        -> Result<(), error::Unspecified> {
    if input.is_empty() {
        return Err(error::Unspecified);
    }

    // `bytes_in_current_limb` is the number of bytes in the current limb.
    // It will be `LIMB_BYTES` for all limbs except maybe the highest-order
    // limb.
    let mut bytes_in_current_limb = input.len() % LIMB_BYTES;
    if bytes_in_current_limb == 0 {
        bytes_in_current_limb = LIMB_BYTES;
    }

    let num_encoded_limbs =
        (input.len() / LIMB_BYTES) +
        (if bytes_in_current_limb == LIMB_BYTES { 0 } else { 1 });
    if num_encoded_limbs > result.len() {
        return Err(error::Unspecified);
    }

    for r in &mut result[..] {
        *r = 0;
    }

    // XXX: Questionable as far as constant-timedness is concerned.
    // TODO: Improve this.
    input.read_all(error::Unspecified, |input| {
        for i in 0..num_encoded_limbs {
            let mut limb: Limb = 0;
            for _ in 0..bytes_in_current_limb {
                let b = input.read_byte()?;
                limb = (limb << 8) | (b as Limb);
            }
            result[num_encoded_limbs - i - 1] = limb;
            bytes_in_current_limb = LIMB_BYTES;
        }
        Ok(())
    })
}

/// XXX: Panics if `out` isn't large enough, but in theory the callers ensure
/// that never happens. TODO: When `ring::rsa::bigint::Nonnegative` stops using
/// `BIGNUM`, it should be the case that `limbs.len() == out.len() * LIMB_BYTES`
/// and so the padding logic can be dropped.
pub fn big_endian_from_limbs_padded(limbs: &[Limb], out: &mut [u8]) {
    let num_limbs = limbs.len();
    let out_len = out.len();
    let (to_zero, dest) =
        out.split_at_mut(out_len - (num_limbs * LIMB_BYTES)); // May panic.
    for i in 0..num_limbs {
        let mut limb = limbs[i];
        for j in 0..LIMB_BYTES {
            dest[((num_limbs - i - 1) * LIMB_BYTES) + (LIMB_BYTES - j - 1)] =
                 (limb & 0xff) as u8;
            limb >>= 8;
        }
    }
    polyfill::slice::fill(to_zero, 0);
}

extern {
    fn LIMBS_are_zero(a: *const Limb, num_limbs: c::size_t) -> LimbMask;
    fn LIMBS_less_than(a: *const Limb, b: *const Limb, num_limbs: c::size_t)
                       -> LimbMask;
    fn LIMBS_reduce_once(r: *mut Limb, m: *const Limb, num_limbs: c::size_t);
}

#[cfg(test)]
mod tests {
    use untrusted;
    use super::*;

    #[test]
    fn test_parse_big_endian_and_pad_consttime() {
        const LIMBS: usize = 4;

        {
            // Empty input.
            let inp = untrusted::Input::from(&[]);
            let mut result = [0; LIMBS];
            assert!(parse_big_endian_and_pad_consttime(inp, &mut result)
                        .is_err());
        }

        // The input is longer than will fit in the given number of limbs.
        {
            let inp = [1, 2, 3, 4, 5, 6, 7, 8, 9];
            let inp = untrusted::Input::from(&inp);
            let mut result = [0; 8 / LIMB_BYTES];
            assert!(parse_big_endian_and_pad_consttime(inp, &mut result[..])
                        .is_err());
        }

        // Less than a full limb.
        {
            let inp = [0xfe];
            let inp = untrusted::Input::from(&inp);
            let mut result = [0; LIMBS];
            assert_eq!(Ok(()),
                       parse_big_endian_and_pad_consttime(inp, &mut result[..]));
            assert_eq!(&[0xfe, 0, 0, 0], &result);
        }

        // A whole limb for 32-bit, half a limb for 64-bit.
        {
            let inp = [0xbe, 0xef, 0xf0, 0x0d];
            let inp = untrusted::Input::from(&inp);
            let mut result = [0; LIMBS];
            assert_eq!(Ok(()),
                       parse_big_endian_and_pad_consttime(inp, &mut result));
            assert_eq!(&[0xbeeff00d, 0, 0, 0], &result);
        }

        // XXX: This is a weak set of tests. TODO: expand it.
    }

    #[test]
    fn test_big_endian_from_limbs_padded_same_length() {
        #[cfg(target_pointer_width = "32")]
        let limbs = [
            0xbccddeef, 0x89900aab, 0x45566778, 0x01122334,
            0xddeeff00, 0x99aabbcc, 0x55667788, 0x11223344
        ];

        #[cfg(target_pointer_width = "64")]
        let limbs = [
            0x89900aab_bccddeef, 0x01122334_45566778,
            0x99aabbcc_ddeeff00, 0x11223344_55667788,
        ];

        let expected = [
            0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
            0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00,
            0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78,
            0x89, 0x90, 0x0a, 0xab, 0xbc, 0xcd, 0xde, 0xef,
        ];

        let mut out = [0xabu8; 32];
        big_endian_from_limbs_padded(&limbs[..], &mut out);
        assert_eq!(&out[..], &expected[..]);
    }

    #[test]
    fn test_big_endian_from_limbs_padded_fewer_limbs() {
        #[cfg(target_pointer_width = "32")]
        // Two fewer limbs.
        let limbs = [
            0xbccddeef, 0x89900aab, 0x45566778, 0x01122334,
            0xddeeff00, 0x99aabbcc,
        ];

        // One fewer limb.
        #[cfg(target_pointer_width = "64")]
        let limbs = [
            0x89900aab_bccddeef, 0x01122334_45566778,
            0x99aabbcc_ddeeff00,
        ];

        let expected = [
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00,
            0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78,
            0x89, 0x90, 0x0a, 0xab, 0xbc, 0xcd, 0xde, 0xef,
        ];

        let mut out = [0xabu8; 32];

        big_endian_from_limbs_padded(&limbs[..], &mut out);
        assert_eq!(&out[..], &expected[..]);
    }
}
