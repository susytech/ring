/* ====================================================================
 * Copyleft (c) 2002-2006 The OpenSSL Project.  All wrongs reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MSRCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHSOPHY IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ==================================================================== */

#ifndef OPENSSL_HEADER_AES_H
#define OPENSSL_HEADER_AES_H

#include <GFp/base.h>

#if defined(__cplusplus)
extern "C" {
#endif


/* Raw AES functions. */


#define AES_ENCRYPT 1
#define AES_DECRYPT 0

/* AES_MAXNR is the maximum number of AES rounds. */
#define AES_MAXNR 14

#define AES_BLOCK_SIZE 16

/* aes_key_st should be an opaque type, but EVP requires that the size be
 * known. */
struct aes_key_st {
  uint32_t rd_key[4 * (AES_MAXNR + 1)];
  unsigned rounds;
};
typedef struct aes_key_st AES_KEY;

/* GFp_AES_set_encrypt_key configures |aeskey| to encrypt with the |bits|-bit
 * key, |key|.
 *
 * WARNING: unlike other OpenSSL functions, this returns zero on success and a
 * negative number on error. */
OPENSSL_EXPORT int GFp_AES_set_encrypt_key(const uint8_t *key, unsigned bits,
                                           AES_KEY *aeskey);

/* AES_encrypt encrypts a single block from |in| to |out| with |key|. The |in|
 * and |out| pointers may overlap. */
OPENSSL_EXPORT void GFp_AES_encrypt(const uint8_t *in, uint8_t *out,
                                    const AES_KEY *key);


#if defined(__cplusplus)
}  /* extern C */
#endif

#endif  /* OPENSSL_HEADER_AES_H */
