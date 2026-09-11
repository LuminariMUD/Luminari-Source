/* Adaptive password hashing regressions: current-scheme storage, legacy
 * crypt() verification, rehash detection, and boundary handling. */
#include "CuTest.h"

#include "conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/password.h"

#include <crypt.h>
#include <string.h>

/* Traditional DES crypt("foob", "ar"), the historical known answer. */
static const char *legacy_des_hash = "arlEKn0OzVJn.";

void Test_password_hash_uses_current_scheme_with_random_salt(CuTest *tc)
{
  char first[MAX_PWD_HASH_LENGTH + 1];
  char second[MAX_PWD_HASH_LENGTH + 1];

  CuAssertTrue(tc, password_hash("correct horse battery", first, sizeof(first)));
  CuAssertTrue(tc, password_hash("correct horse battery", second, sizeof(second)));
  CuAssertTrue(tc, strncmp(first, PASSWORD_HASH_PREFIX, strlen(PASSWORD_HASH_PREFIX)) == 0);
  CuAssertTrue(tc, strcmp(first, second) != 0);
  CuAssertTrue(tc, strlen(first) < MAX_PWD_HASH_LENGTH);
  CuAssertTrue(tc, strstr(first, "correct horse battery") == NULL);
  CuAssertTrue(tc, password_verify("correct horse battery", first));
  CuAssertTrue(tc, password_verify("correct horse battery", second));
  CuAssertTrue(tc, !password_verify("correct horse batterx", first));
  CuAssertTrue(tc, !password_needs_rehash(first));
}

void Test_password_hash_rejects_empty_and_small_buffers(CuTest *tc)
{
  char out[MAX_PWD_HASH_LENGTH + 1];
  char tiny[8];

  strlcpy(out, "untouched", sizeof(out));
  CuAssertTrue(tc, !password_hash("", out, sizeof(out)));
  CuAssertTrue(tc, !password_hash(NULL, out, sizeof(out)));
  CuAssertStrEquals(tc, "untouched", out);
  strlcpy(tiny, "keep", sizeof(tiny));
  CuAssertTrue(tc, !password_hash("secret", tiny, sizeof(tiny)));
  CuAssertStrEquals(tc, "keep", tiny);
}

void Test_password_verify_accepts_legacy_crypt_records(CuTest *tc)
{
  CuAssertTrue(tc, password_verify("foob", legacy_des_hash));
  CuAssertTrue(tc, !password_verify("fooc", legacy_des_hash));
  CuAssertTrue(tc, password_needs_rehash(legacy_des_hash));
}

void Test_password_verify_rejects_empty_or_malformed_input(CuTest *tc)
{
  char hash[MAX_PWD_HASH_LENGTH + 1];

  CuAssertTrue(tc, password_hash("secret", hash, sizeof(hash)));
  CuAssertTrue(tc, !password_verify("", hash));
  CuAssertTrue(tc, !password_verify(NULL, hash));
  CuAssertTrue(tc, !password_verify("secret", ""));
  CuAssertTrue(tc, !password_verify("secret", NULL));
  CuAssertTrue(tc, !password_verify("secret", "$y$"));
  CuAssertTrue(tc, !password_verify("secret", "not-a-hash"));
  CuAssertTrue(tc, password_needs_rehash(""));
  CuAssertTrue(tc, password_needs_rehash("not-a-hash"));
}

void Test_password_hash_handles_long_and_special_plaintext(CuTest *tc)
{
  char plaintext[MAX_PWD_LENGTH + 1];
  char hash[MAX_PWD_HASH_LENGTH + 1];
  int i;

  for (i = 0; i < MAX_PWD_LENGTH; i++)
    plaintext[i] = (char)('!' + (i % 90));
  plaintext[MAX_PWD_LENGTH] = '\0';

  CuAssertTrue(tc, password_hash(plaintext, hash, sizeof(hash)));
  CuAssertTrue(tc, password_verify(plaintext, hash));
  plaintext[MAX_PWD_LENGTH - 1] ^= 1;
  CuAssertTrue(tc, !password_verify(plaintext, hash));

  CuAssertTrue(tc, password_hash("it's \\ \"quoted\" ; -- \xc3\xa9", hash, sizeof(hash)));
  CuAssertTrue(tc, password_verify("it's \\ \"quoted\" ; -- \xc3\xa9", hash));
}

void Test_password_rejects_plaintext_over_policy_length(CuTest *tc)
{
  char plaintext[MAX_PWD_LENGTH + 2];
  char hash[MAX_PWD_HASH_LENGTH + 1];

  memset(plaintext, 'a', MAX_PWD_LENGTH + 1);
  plaintext[MAX_PWD_LENGTH + 1] = '\0';

  strlcpy(hash, "untouched", sizeof(hash));
  CuAssertTrue(tc, !password_hash(plaintext, hash, sizeof(hash)));
  CuAssertStrEquals(tc, "untouched", hash);

  /* A stored hash never matches over-length input, even one that shares a prefix. */
  plaintext[MAX_PWD_LENGTH] = '\0';
  CuAssertTrue(tc, password_hash(plaintext, hash, sizeof(hash)));
  plaintext[MAX_PWD_LENGTH] = 'a';
  plaintext[MAX_PWD_LENGTH + 1] = '\0';
  CuAssertTrue(tc, !password_verify(plaintext, hash));
}

void Test_password_needs_rehash_on_parameter_change(CuTest *tc)
{
  /* A current-scheme record produced with a lower cost must be upgraded. */
  struct crypt_data data;
  char setting[CRYPT_GENSALT_OUTPUT_SIZE];
  const char *cheaper;

  memset(&data, 0, sizeof(data));
  CuAssertPtrNotNull(tc, crypt_gensalt_rn(PASSWORD_HASH_PREFIX, PASSWORD_HASH_COST - 2, NULL, 0,
                                          setting, sizeof(setting)));
  cheaper = crypt_rn("secret", setting, &data, sizeof(data));
  CuAssertPtrNotNull(tc, cheaper);
  CuAssertTrue(tc, password_verify("secret", cheaper));
  CuAssertTrue(tc, password_needs_rehash(cheaper));
}
