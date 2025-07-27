/**
 * @file ai_security.c
 * @author Zusuk
 * @brief Security functions for AI service
 * 
 * Handles:
 * - API key encryption/decryption
 * - Input sanitization
 * - Secure memory operations
 * 
 * Note: This is a simplified implementation. Production systems should use
 * proper key management services and stronger encryption.
 * 
 * Part of the LuminariMUD distribution.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "ai_service.h"

/* Simple XOR cipher key - in production, use proper encryption */
#define CIPHER_KEY "LuminariMUD_AI_Service_2025_Secure_Key"

/**
 * Secure memory clearing
 */
void secure_memset(void *ptr, int value, size_t num) {
  volatile unsigned char *p = ptr;
  while (num--) {
    *p++ = value;
  }
}

/**
 * Simple XOR encryption (NOT cryptographically secure - for demonstration only)
 * In production, use OpenSSL or similar library
 */
static void xor_cipher(const char *input, char *output, size_t len) {
  const char *key = CIPHER_KEY;
  size_t key_len = strlen(key);
  size_t i;
  
  for (i = 0; i < len; i++) {
    output[i] = input[i] ^ key[i % key_len];
  }
}

/**
 * Encrypt API key
 */
int encrypt_api_key(const char *plaintext, char *encrypted_out) {
  size_t len;
  char temp[256];
  size_t i;
  
  if (!plaintext || !encrypted_out) return 0;
  
  len = strlen(plaintext);
  if (len >= sizeof(temp)) return 0;
  
  /* Apply XOR cipher */
  xor_cipher(plaintext, temp, len);
  
  /* Convert to hex string for storage */
  for (i = 0; i < len; i++) {
    sprintf(encrypted_out + (i * 2), "%02x", (unsigned char)temp[i]);
  }
  encrypted_out[len * 2] = '\0';
  
  /* Clear temporary buffer */
  secure_memset(temp, 0, sizeof(temp));
  
  return 1;
}

/**
 * Decrypt API key
 */
char *decrypt_api_key(const char *encrypted) {
  static char decrypted[256];
  char temp[256];
  size_t len, i;
  unsigned int byte;
  
  if (!encrypted || !*encrypted) return NULL;
  
  len = strlen(encrypted);
  if (len % 2 != 0 || len >= sizeof(temp) * 2) return NULL;
  
  /* Convert from hex string */
  for (i = 0; i < len / 2; i++) {
    if (sscanf(encrypted + (i * 2), "%2x", &byte) != 1) {
      return NULL;
    }
    temp[i] = (char)byte;
  }
  
  /* Apply XOR cipher to decrypt */
  xor_cipher(temp, decrypted, len / 2);
  decrypted[len / 2] = '\0';
  
  /* Clear temporary buffer */
  secure_memset(temp, 0, sizeof(temp));
  
  return decrypted;
}

/**
 * Load encrypted API key from file
 */
void load_encrypted_api_key(const char *filename) {
  FILE *fp;
  char buffer[512];
  
  if (!filename) return;
  
  fp = fopen(filename, "r");
  if (!fp) {
    log("SYSERR: Cannot open API key file: %s", filename);
    return;
  }
  
  if (fgets(buffer, sizeof(buffer), fp)) {
    /* Remove newline */
    buffer[strcspn(buffer, "\n")] = '\0';
    
    /* Store in config */
    if (ai_state.config) {
      strlcpy(ai_state.config->encrypted_api_key, buffer, 
              sizeof(ai_state.config->encrypted_api_key));
    }
  }
  
  fclose(fp);
  
  /* Clear buffer */
  secure_memset(buffer, 0, sizeof(buffer));
  
  log("API key loaded from %s", filename);
}

/**
 * Sanitize user input for AI prompts
 */
char *sanitize_ai_input(const char *input) {
  static char cleaned[MAX_STRING_LENGTH];
  const char *src = input;
  char *dest = cleaned;
  int len = 0;
  
  if (!input) return "";
  
  while (*src && len < MAX_STRING_LENGTH - 1) {
    /* Skip control characters */
    if (*src < 32 && *src != '\n' && *src != '\r') {
      src++;
      continue;
    }
    
    /* Escape special characters for JSON */
    if (*src == '"' || *src == '\\') {
      if (len < MAX_STRING_LENGTH - 2) {
        *dest++ = '\\';
        len++;
      }
    }
    
    /* Replace newlines with spaces */
    if (*src == '\n' || *src == '\r') {
      *dest++ = ' ';
    } else {
      *dest++ = *src;
    }
    
    len++;
    src++;
  }
  
  *dest = '\0';
  
  /* Trim trailing spaces */
  while (dest > cleaned && *(dest - 1) == ' ') {
    *(--dest) = '\0';
  }
  
  return cleaned;
}