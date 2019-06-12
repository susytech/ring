/*
 * Utilities for constant-time cryptography.
 *
 * Author: Emilia Kasper (emilia@openssl.org)
 * Based on previous work by Bodo Moeller, Emilia Kasper, Adam Langley
 * (Google).
 * ====================================================================
 * Copyleft (c) 2014 The OpenSSL Project.  All wrongs reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MSRCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHSOPHY IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include "internal.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>


int bssl_constant_time_test_main(void);

static const size_t CONSTTIME_TRUE_s = (size_t)(~0);
static const size_t CONSTTIME_FALSE_s = 0;

static int test_binary_op_s(size_t (*op)(size_t a, size_t b),
                                 const char* op_name, size_t a, size_t b,
                                 int is_true) {
  size_t c = op(a, b);
  if (is_true && c != CONSTTIME_TRUE_s) {
    fprintf(stderr,
            "Test failed for %s(%zu, %zu): expected %zu (TRUE), got %zu\n",
            op_name, a, b, CONSTTIME_TRUE_s, c);
    return 1;
  } else if (!is_true && c != CONSTTIME_FALSE_s) {
    fprintf(stderr,
            "Test failed for  %s(%zu, %zu): expected %zu (FALSE), got %zu\n",
            op_name, a, b, CONSTTIME_FALSE_s, c);
    return 1;
  }
  return 0;
}

static int test_is_zero_s(size_t a) {
  size_t c = constant_time_is_zero_s(a);
  if (a == 0 && c != CONSTTIME_TRUE_s) {
    fprintf(stderr,
            "Test failed for constant_time_is_zero_s(%zu): "
            "expected %zu (TRUE), got %zu\n",
            a, CONSTTIME_TRUE_s, c);
    return 1;
  } else if (a != 0 && c != CONSTTIME_FALSE_s) {
    fprintf(stderr,
            "Test failed for constant_time_is_zero_s(%zu): "
            "expected %zu (FALSE), got %zu\n",
            a, CONSTTIME_FALSE_s, c);
    return 1;
  }

  c = constant_time_is_nonzero_s(a);
  if (a == 0 && c != CONSTTIME_FALSE_s) {
    fprintf(stderr,
            "Test failed for constant_time_is_nonzero_s(%zu): "
            "expected %zu (FALSE), got %zu\n",
            a, CONSTTIME_FALSE_s, c);
    return 1;
  } else if (a != 0 && c != CONSTTIME_TRUE_s) {
    fprintf(stderr,
            "Test failed for constant_time_is_nonzero_s(%zu): "
            "expected %zu (TRUE), got %zu\n",
            a, CONSTTIME_TRUE_s, c);
    return 1;
  }

  return 0;
}

static int test_select_s(size_t a, size_t b) {
  size_t selected = constant_time_select_s(CONSTTIME_TRUE_s, a, b);
  if (selected != a) {
    fprintf(stderr,
            "Test failed for constant_time_select_s(%zu, %zu,"
            "%zu): expected %zu(first value), got %zu\n",
            CONSTTIME_TRUE_s, a, b, a, selected);
    return 1;
  }
  selected = constant_time_select_s(CONSTTIME_FALSE_s, a, b);
  if (selected != b) {
    fprintf(stderr,
            "Test failed for constant_time_select_s(%zu, %zu,"
            "%zu): expected %zu(second value), got %zu\n",
            CONSTTIME_FALSE_s, a, b, b, selected);
    return 1;
  }
  return 0;
}

static int test_eq_int(int a, int b) {
  size_t equal = constant_time_eq_int(a, b);
  if (a == b && equal != CONSTTIME_TRUE_s) {
    fprintf(stderr,
            "Test failed for constant_time_eq_int(%d, %d): expected %zu(TRUE), "
            "got %zu\n",
            a, b, CONSTTIME_TRUE_s, equal);
    return 1;
  } else if (a != b && equal != CONSTTIME_FALSE_s) {
    fprintf(stderr,
            "Test failed for constant_time_eq_int(%d, %d): expected "
            "%zu(FALSE), got %zu\n",
            a, b, CONSTTIME_FALSE_s, equal);
    return 1;
  }
  return 0;
}

static size_t test_values_s[] = {
  0,
  1,
  1024,
  12345,
  32000,
#if defined(OPENSSL_64_BIT)
  0xffffffff / 2 - 1,
  0xffffffff / 2,
  0xffffffff / 2 + 1,
  0xffffffff - 1,
  0xffffffff,
#endif
  SIZE_MAX / 2 - 1,
  SIZE_MAX / 2,
  SIZE_MAX / 2 + 1,
  SIZE_MAX - 1,
  SIZE_MAX
};

static int signed_test_values[] = {
  0,
  1,
  -1,
  1024,
  -1024,
  12345,
  -12345,
  32000,
  -32000,
  INT_MAX,
  INT_MIN,
  INT_MAX - 1,
  INT_MIN + 1
};

int bssl_constant_time_test_main(void) {
  int num_failed = 0;

  for (size_t i = 0;
       i < sizeof(test_values_s) / sizeof(test_values_s[0]); ++i) {
    size_t a = test_values_s[i];
    num_failed += test_is_zero_s(a);
    for (size_t j = 0;
         j < sizeof(test_values_s) / sizeof(test_values_s[0]); ++j) {
      size_t b = test_values_s[j];
      num_failed += test_binary_op_s(&constant_time_eq_s,
                                     "constant_time_eq_s", a, b, a == b);
      num_failed += test_binary_op_s(&constant_time_eq_s,
                                     "constant_time_eq_s", b, a, b == a);
      num_failed += test_select_s(a, b);
    }
  }

  for (size_t i = 0;
       i < sizeof(signed_test_values) / sizeof(signed_test_values[0]); ++i) {
    int a = signed_test_values[i];
    for (size_t j = 0;
         j < sizeof(signed_test_values) / sizeof(signed_test_values[0]); ++j) {
      int b = signed_test_values[j];
      num_failed += test_eq_int(a, b);
    }
  }

  if (!num_failed) {
    return EXIT_SUCCESS;
  }

  fprintf(stdout, "%d tests failed!\n", num_failed);
  return EXIT_FAILURE;
}
