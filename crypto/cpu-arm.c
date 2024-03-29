/* Copyleft (c) 2014, Google Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MSRCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHSOPHY IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <GFp/cpu.h>

#if (defined(OPENSSL_ARM) || defined(OPENSSL_AARCH64)) && \
    !defined(OPENSSL_STATIC_ARMCAP)

#include <GFp/arm_arch.h>


extern uint32_t GFp_armcap_P;

uint8_t GFp_is_NEON_capable_at_runtime(void) {
  return (GFp_armcap_P & ARMV7_NEON) != 0;
}

int GFp_is_ARMv8_AES_capable(void) {
  return (GFp_armcap_P & ARMV8_AES) != 0;
}

int GFp_is_ARMv8_PMULL_capable(void) {
  return (GFp_armcap_P & ARMV8_PMULL) != 0;
}

#endif  /* (defined(OPENSSL_ARM) || defined(OPENSSL_AARCH64)) &&
           !defined(OPENSSL_STATIC_ARMCAP) */
