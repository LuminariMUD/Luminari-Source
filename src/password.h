/**************************************************************************
 *  File: password.h                                   Part of LuminariMUD *
 *  Usage: Adaptive password hashing and verification.                     *
 *                                                                         *
 *  Stored passwords are self-describing PHC strings produced by libxcrypt *
 *  yescrypt with a random per-password salt.  Legacy crypt() records are  *
 *  still verifiable so existing accounts migrate transparently on login.  *
 **************************************************************************/

#ifndef PASSWORD_H
#define PASSWORD_H

#include <stdbool.h>
#include <stddef.h>

/* Policy.  The cost is the libxcrypt yescrypt count (1..11); 5 is the
 * distribution default used for system accounts and costs roughly 16 MiB
 * and tens of milliseconds per hash.  Raising it makes every legacy or
 * lower-cost record rehash on the next successful login. */
#define PASSWORD_HASH_PREFIX "$y$"
#define PASSWORD_HASH_COST 5UL

/* Hash plaintext into out using the current policy.  Returns false and leaves
 * out untouched when hashing fails; never stores plaintext. */
bool password_hash(const char *plaintext, char *out, size_t out_size);

/* Constant-time verification of plaintext against a stored hash of any
 * supported scheme, including legacy crypt() output. */
bool password_verify(const char *plaintext, const char *stored);

/* True when stored does not use the current scheme and parameters. */
bool password_needs_rehash(const char *stored);

/* Wipe a buffer that held password material. */
void password_secure_zero(void *buffer, size_t size);

#endif /* PASSWORD_H */
