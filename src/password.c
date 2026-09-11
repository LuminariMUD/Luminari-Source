/**************************************************************************
 *  File: password.c                                   Part of LuminariMUD *
 *  Usage: Adaptive password hashing and verification.                     *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "password.h"

#include <crypt.h>
#include <string.h>

/* The settings prefix ("$y$<params>$") every current-policy hash carries. */
static const char *password_current_params(void)
{
  static char params[CRYPT_GENSALT_OUTPUT_SIZE];
  char setting[CRYPT_GENSALT_OUTPUT_SIZE];
  const char *third;

  if (params[0])
    return params;

  if (crypt_gensalt_rn(PASSWORD_HASH_PREFIX, PASSWORD_HASH_COST, NULL, 0, setting,
                       sizeof(setting)) == NULL)
    return NULL;

  /* "$y$j9T$salt" -> keep "$y$j9T$" */
  third = strchr(setting + strlen(PASSWORD_HASH_PREFIX), '$');
  if (third == NULL)
    return NULL;
  snprintf(params, sizeof(params), "%.*s", (int)(third - setting + 1), setting);
  return params;
}

/* True when plaintext is non-empty and within the MAX_PWD_LENGTH policy. */
static bool password_plaintext_acceptable(const char *plaintext)
{
  return plaintext != NULL && plaintext[0] != '\0' && strlen(plaintext) <= MAX_PWD_LENGTH;
}

/* Wipe a buffer that held password material with a non-optimizable primitive. */
void password_secure_zero(void *buffer, size_t size)
{
  if (buffer != NULL && size > 0)
    explicit_bzero(buffer, size);
}

/* Hash plaintext under the current policy into out; false on any failure. */
bool password_hash(const char *plaintext, char *out, size_t out_size)
{
  struct crypt_data data;
  char setting[CRYPT_GENSALT_OUTPUT_SIZE];
  const char *hash;
  bool ok;

  if (out == NULL || out_size == 0 || !password_plaintext_acceptable(plaintext))
    return false;

  if (crypt_gensalt_rn(PASSWORD_HASH_PREFIX, PASSWORD_HASH_COST, NULL, 0, setting,
                       sizeof(setting)) == NULL)
  {
    log("SYSERR: password_hash: unable to generate a %s setting", PASSWORD_HASH_PREFIX);
    return false;
  }

  memset(&data, 0, sizeof(data));
  hash = crypt_rn(plaintext, setting, &data, sizeof(data));
  ok = hash != NULL && hash[0] != '*' && strlen(hash) < out_size;
  if (ok)
    strlcpy(out, hash, out_size);
  else
    log("SYSERR: password_hash: hashing failed or output exceeds %zu bytes", out_size);
  password_secure_zero(&data, sizeof(data));
  return ok;
}

/* Constant-time check of plaintext against a stored hash of any supported scheme. */
bool password_verify(const char *plaintext, const char *stored)
{
  struct crypt_data data;
  const char *computed;
  size_t length;
  size_t i;
  unsigned char diff;

  if (!password_plaintext_acceptable(plaintext) || stored == NULL || stored[0] == '\0')
    return false;

  memset(&data, 0, sizeof(data));
  computed = crypt_rn(plaintext, stored, &data, sizeof(data));
  if (computed == NULL || computed[0] == '*')
  {
    password_secure_zero(&data, sizeof(data));
    return false;
  }

  /* Hash length is public per scheme; the comparison of the bytes is not. */
  length = strlen(stored);
  if (strlen(computed) != length)
  {
    password_secure_zero(&data, sizeof(data));
    return false;
  }
  diff = 0;
  for (i = 0; i < length; i++)
    diff |= (unsigned char)(stored[i] ^ computed[i]);
  password_secure_zero(&data, sizeof(data));
  return diff == 0;
}

/* True when stored is missing, malformed, legacy, or below the current parameters. */
bool password_needs_rehash(const char *stored)
{
  const char *params;

  if (stored == NULL || stored[0] == '\0')
    return true;
  if (crypt_checksalt(stored) != CRYPT_SALT_OK)
    return true;
  params = password_current_params();
  if (params == NULL)
    return false; /* cannot produce a better hash right now */
  return strncmp(stored, params, strlen(params)) != 0;
}
