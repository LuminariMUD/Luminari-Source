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
#include <errno.h>

/* Simple XOR cipher key - in production, use proper encryption */
#define CIPHER_KEY "LuminariMUD_AI_Service_2025_Secure_Key"

/**
 * Secure memory clearing
 */
void secure_memset(void *ptr, int value, size_t num) {
  volatile unsigned char *p = ptr;
  AI_DEBUG("secure_memset() called - clearing %zu bytes at %p", num, ptr);
  while (num--) {
    *p++ = value;
  }
}


/**
 * Encrypt API key
 */
int encrypt_api_key(const char *plaintext, char *encrypted_out) {
  AI_DEBUG("encrypt_api_key() called");
  
  if (!plaintext || !encrypted_out) {
    AI_DEBUG("ERROR: NULL plaintext or encrypted_out");
    return 0;
  }
  
  AI_DEBUG("WARNING: Using plaintext storage (no encryption)");
  AI_DEBUG("Input length: %zu", strlen(plaintext));
  
  /* Just copy the API key as-is - no encryption */
  strlcpy(encrypted_out, plaintext, 256);
  
  AI_DEBUG("API key stored (first 10 chars): %.10s...", encrypted_out);
  return 1;
}

/**
 * Decrypt API key
 * Note: Caller must free() the returned string
 */
char *decrypt_api_key(const char *encrypted) {
  char *decrypted;
  
  AI_DEBUG("decrypt_api_key() called");
  
  if (!encrypted || !*encrypted) {
    AI_DEBUG("ERROR: NULL or empty encrypted key");
    return NULL;
  }
  
  AI_DEBUG("WARNING: Using plaintext retrieval (no decryption)");
  AI_DEBUG("Encrypted length: %zu", strlen(encrypted));
  
  /* Allocate memory for decrypted key */
  CREATE(decrypted, char, 256);
  if (!decrypted) {
    log("SYSERR: Failed to allocate memory for decrypted API key");
    AI_DEBUG("ERROR: Failed to allocate 256 bytes");
    return NULL;
  }
  
  /* Just return the API key as-is - no decryption */
  strlcpy(decrypted, encrypted, 256);
  
  AI_DEBUG("API key retrieved (first 10 chars): %.10s...", decrypted);
  return decrypted;
}

/**
 * Load encrypted API key from file
 */
void load_encrypted_api_key(const char *filename) {
  FILE *fp;
  char buffer[512];
  
  AI_DEBUG("load_encrypted_api_key() called with filename='%s'",
           filename ? filename : "(null)");
  
  if (!filename) {
    AI_DEBUG("ERROR: NULL filename");
    return;
  }
  
  AI_DEBUG("Opening file: %s", filename);
  fp = fopen(filename, "r");
  if (!fp) {
    log("SYSERR: Cannot open API key file: %s", filename);
    AI_DEBUG("ERROR: fopen failed - %s", strerror(errno));
    return;
  }
  AI_DEBUG("File opened successfully");
  
  if (fgets(buffer, sizeof(buffer), fp)) {
    AI_DEBUG("Read %zu bytes from file", strlen(buffer));
    /* Remove newline */
    buffer[strcspn(buffer, "\n")] = '\0';
    AI_DEBUG("After newline removal: %zu bytes", strlen(buffer));
    
    /* Store in config */
    if (ai_state.config) {
      AI_DEBUG("Storing key in config (first 10 chars): %.10s...", buffer);
      strlcpy(ai_state.config->encrypted_api_key, buffer, 
              sizeof(ai_state.config->encrypted_api_key));
    } else {
      AI_DEBUG("ERROR: ai_state.config is NULL");
    }
  } else {
    AI_DEBUG("ERROR: Failed to read from file");
  }
  
  fclose(fp);
  AI_DEBUG("File closed");
  
  /* Clear buffer */
  AI_DEBUG("Clearing sensitive buffer");
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
      if (len >= MAX_STRING_LENGTH - 2) {
        break; /* No room for escape sequence */
      }
      *dest++ = '\\';
      len++;
    }
    
    /* Check if we have room for the character */
    if (len >= MAX_STRING_LENGTH - 1) {
      break;
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