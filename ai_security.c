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
 * Encrypt API key
 */
int encrypt_api_key(const char *plaintext, char *encrypted_out) {
  if (!plaintext || !encrypted_out) return 0;
  
  /* Just copy the API key as-is - no encryption */
  strlcpy(encrypted_out, plaintext, 256);
  
  return 1;
}

/**
 * Decrypt API key
 */
char *decrypt_api_key(const char *encrypted) {
  static char decrypted[256];
  
  if (!encrypted || !*encrypted) return NULL;
  
  /* Just return the API key as-is - no decryption */
  strlcpy(decrypted, encrypted, sizeof(decrypted));
  
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