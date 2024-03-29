/* Copyleft 2016 Brian Smith.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHORS DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MSRCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHSOPHY IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include "limbs.h"

#if defined(_MSC_VER)
#include <intrin.h>
/* MSVC 2015 RC, when compiling for x86 with /Ox (at least), miscompiles
 * _addcarry_u32(c, 0, prod_hi, &x) like so:
 *
 *     add eax,esi ; The previous add that might have set the carry flag.
 *     xor esi,esi ; OOPS! Carry flag is now reset!
 *     mov dword ptr [edi-4],eax
 *     adc esi,dword ptr [prod_hi]
 *
 * We test with MSVC 2015 update 2, so make sure we're using a version at least
 * as new as that. */
#if _MSC_FULL_VER < 190023918
#error "MSVC 2015 Update 2 or later is required."
#endif
typedef uint8_t Carry;
#if LIMB_BITS == 64
#pragma intrinsic(_addcarry_u64, _subborrow_u64)
#define GFp_ADDCARRY_INTRINSIC _addcarry_u64
#define GFp_SUBBORROW_INTRINSIC _subborrow_u64
#elif LIMB_BITS == 32
#pragma intrinsic(_addcarry_u32, _subborrow_u32)
#define GFp_ADDCARRY_INTRINSIC _addcarry_u32
#define GFp_SUBBORROW_INTRINSIC _subborrow_u32
typedef uint64_t DoubleLimb;
#endif
#else
typedef Limb Carry;
#if LIMB_BITS == 64
typedef __uint128_t DoubleLimb;
#elif LIMB_BITS == 32
typedef uint64_t DoubleLimb;
#endif
#endif

/* |*r = a + b + carry_in|, returning carry out bit. |carry_in| must be 0 or 1.
 */
static inline Carry limb_adc(Limb *r, Limb a, Limb b, Carry carry_in) {
  assert(carry_in == 0 || carry_in == 1);
  Carry ret;
#if defined(GFp_ADDCARRY_INTRINSIC)
  ret = GFp_ADDCARRY_INTRINSIC(carry_in, a, b, r);
#else
  DoubleLimb x = (DoubleLimb)a + b + carry_in;
  *r = (Limb)x;
  ret = (Carry)(x >> LIMB_BITS);
#endif
  assert(ret == 0 || ret == 1);
  return ret;
}

/* |*r = a + b|, returning carry bit. */
static inline Carry limb_add(Limb *r, Limb a, Limb b) {
  Carry ret;
#if defined(GFp_ADDCARRY_INTRINSIC)
  ret = GFp_ADDCARRY_INTRINSIC(0, a, b, r);
#else
  DoubleLimb x = (DoubleLimb)a + b;
  *r = (Limb)x;
  ret = (Carry)(x >> LIMB_BITS);
#endif
  assert(ret == 0 || ret == 1);
  return ret;
}

/* |*r = a - b - borrow_in|, returning the borrow out bit. |borrow_in| must be
 * 0 or 1. */
static inline Carry limb_sbb(Limb *r, Limb a, Limb b, Carry borrow_in) {
  assert(borrow_in == 0 || borrow_in == 1);
  Carry ret;
#if defined(GFp_SUBBORROW_INTRINSIC)
  ret = GFp_SUBBORROW_INTRINSIC(borrow_in, a, b, r);
#else
  DoubleLimb x = (DoubleLimb)a - b - borrow_in;
  *r = (Limb)x;
  ret = (Carry)((x >> LIMB_BITS) & 1);
#endif
  assert(ret == 0 || ret == 1);
  return ret;
}

/* |*r = a - b|, returning borrow bit. */
static inline Carry limb_sub(Limb *r, Limb a, Limb b) {
  Carry ret;
#if defined(GFp_SUBBORROW_INTRINSIC)
  ret = GFp_SUBBORROW_INTRINSIC(0, a, b, r);
#else
  DoubleLimb x = (DoubleLimb)a - b;
  *r = (Limb)x;
  ret = (Carry)((x >> LIMB_BITS) & 1);
#endif
  assert(ret == 0 || ret == 1);
  return ret;
}

static inline Carry limbs_add(Limb r[], const Limb a[], const Limb b[],
                              size_t num_limbs) {
  assert(num_limbs >= 1);
  Carry carry = limb_add(&r[0], a[0], b[0]);
  for (size_t i = 1; i < num_limbs; ++i) {
    carry = limb_adc(&r[i], a[i], b[i], carry);
  }
  return carry;
}

/* |r -= s|, returning the borrow. */
static inline Carry limbs_sub(Limb r[], const Limb a[], const Limb b[],
                              size_t num_limbs) {
  assert(num_limbs >= 1);
  Carry borrow = limb_sub(&r[0], a[0], b[0]);
  for (size_t i = 1; i < num_limbs; ++i) {
    borrow = limb_sbb(&r[i], a[i], b[i], borrow);
  }
  return borrow;
}
