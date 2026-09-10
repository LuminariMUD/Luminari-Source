/**
 * @file ai_security.c
 * @author Zusuk
 * @brief Security functions for AI service
 *
 * Handles:
 * - OpenAI API key lifecycle (store, copy, clear)
 * - Input sanitization
 * - Secure memory operations
 *
 * COMPONENT INTERACTIONS:
 * - USED BY: ai_service.c for all security operations
 * - CRITICAL: All API keys pass through this component
 *
 * SECURITY BOUNDARY (honest statement, see docs/systems/AI_SERVICE_README.md):
 * - The API key is injected at runtime from the process environment or
 *   lib/.env. The application never writes it anywhere: not to disk, not to
 *   the database, not to logs, not to player or staff output.
 * - While loaded, the key lives in exactly one process-private buffer that is
 *   guarded by a mutex so a worker thread can copy it while the main thread
 *   reloads configuration. Callers receive a copy in a buffer they own and
 *   must clear with secure_memset() as soon as the request is built.
 * - There is NO at-rest encryption. Protecting the key at rest is the job of
 *   file permissions on lib/.env and the deployment secret store. Nothing in
 *   this file claims otherwise.
 *
 * Part of the LuminariMUD distribution.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "ai_service.h"
#include <pthread.h>

/* The single process-private copy of the OpenAI API key. */
static char ai_api_key_store[AI_API_KEY_MAX_LEN];
static pthread_mutex_t ai_api_key_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Secure memory clearing
 *
 * Standard memset() may be optimized away if the compiler determines the
 * memory won't be read again. Writing through a volatile pointer forces the
 * stores to happen.
 */
void secure_memset(void *ptr, int value, size_t num)
{
  volatile unsigned char *p = ptr;

  if (!ptr)
    return;
  while (num--)
  {
    *p++ = (unsigned char)value;
  }
}

/**
 * Store the OpenAI API key for later use by request builders.
 *
 * The key is copied verbatim into the process-private store. A NULL or empty
 * key clears the store. A key that does not fit is rejected outright rather
 * than silently truncated, because a truncated key would fail every request
 * with an error message that is very hard to trace.
 *
 * Returns: TRUE when the store now holds the requested value, FALSE when the
 * key was too long (the previous value is cleared in that case).
 */
bool ai_api_key_set(const char *key)
{
  size_t len;
  bool stored;

  len = key ? strlen(key) : 0;
  stored = TRUE;

  pthread_mutex_lock(&ai_api_key_mutex);
  secure_memset(ai_api_key_store, 0, sizeof(ai_api_key_store));
  if (len >= sizeof(ai_api_key_store))
  {
    log("SYSERR: AI Service: OPENAI_API_KEY is longer than %zu characters and was rejected",
        sizeof(ai_api_key_store) - 1);
    stored = FALSE;
  }
  else if (len > 0)
  {
    memcpy(ai_api_key_store, key, len);
    ai_api_key_store[len] = '\0';
  }
  pthread_mutex_unlock(&ai_api_key_mutex);

  return stored;
}

/**
 * Whether a non-empty API key is currently stored.
 */
bool ai_api_key_is_set(void)
{
  bool present;

  pthread_mutex_lock(&ai_api_key_mutex);
  present = ai_api_key_store[0] != '\0';
  pthread_mutex_unlock(&ai_api_key_mutex);

  return present;
}

/**
 * Copy the stored API key into a caller-owned buffer.
 *
 * Safe to call from worker threads. The caller must clear the buffer with
 * secure_memset() once the request headers have been built.
 *
 * Returns: TRUE when out now holds a non-empty key, FALSE when no key is
 * configured or the buffer is too small (out is emptied either way).
 */
bool ai_api_key_copy(char *out, size_t out_size)
{
  bool copied;

  if (!out || out_size == 0)
    return FALSE;

  copied = FALSE;
  pthread_mutex_lock(&ai_api_key_mutex);
  if (ai_api_key_store[0] != '\0' && strlen(ai_api_key_store) < out_size)
  {
    strlcpy(out, ai_api_key_store, out_size);
    copied = TRUE;
  }
  else
  {
    out[0] = '\0';
  }
  pthread_mutex_unlock(&ai_api_key_mutex);

  return copied;
}

/**
 * Clear the stored API key. Called on shutdown and when a reload finds no key.
 */
void ai_api_key_clear(void)
{
  pthread_mutex_lock(&ai_api_key_mutex);
  secure_memset(ai_api_key_store, 0, sizeof(ai_api_key_store));
  pthread_mutex_unlock(&ai_api_key_mutex);
}

/**
 * Sanitize user input for AI prompts
 *
 * Critical security function that prevents prompt injection attacks.
 * Sanitizes user input before it is embedded in a JSON request body.
 *
 * Security measures:
 * 1. Removes control characters (except newlines)
 * 2. Escapes quotes and backslashes for JSON
 * 3. Replaces newlines with spaces
 * 4. Bounds output to out_size
 * 5. Trims trailing spaces
 *
 * The result is written to a caller-owned buffer so the function is safe to
 * call from any thread. out is always NUL-terminated when out_size > 0.
 */
void sanitize_ai_input(const char *input, char *out, size_t out_size)
{
  const char *src = input;
  char *dest = out;
  size_t len = 0;

  if (!out || out_size == 0)
    return;

  out[0] = '\0';
  if (!input)
    return;

  while (*src && len < out_size - 1)
  {
    /* Skip control characters */
    if ((unsigned char)*src < 32 && *src != '\n' && *src != '\r')
    {
      src++;
      continue;
    }

    /* Escape special characters for JSON */
    if (*src == '"' || *src == '\\')
    {
      if (len >= out_size - 2)
      {
        break; /* No room for escape sequence */
      }
      *dest++ = '\\';
      len++;
    }

    /* Replace newlines with spaces */
    if (*src == '\n' || *src == '\r')
    {
      *dest++ = ' ';
    }
    else
    {
      *dest++ = *src;
    }

    len++;
    src++;
  }

  *dest = '\0';

  /* Trim trailing spaces */
  while (dest > out && *(dest - 1) == ' ')
  {
    *(--dest) = '\0';
  }
}
