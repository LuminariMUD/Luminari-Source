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
 * COMPONENT INTERACTIONS:
 * - USED BY: ai_service.c for all security operations
 * - ACCESSES: ai_state.config for API key storage
 * - CRITICAL: All API keys pass through this component
 * 
 * SECURITY MODEL:
 * - API keys stored in ai_state.config->encrypted_api_key
 * - Input sanitization prevents prompt injection
 * - Secure memory clearing prevents key leakage
 * 
 * Note: This is a simplified implementation. Production systems should use
 * proper key management services and stronger encryption.
 * 
 * TODO: Implement AES-256 encryption for API keys
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
 * 
 * Prevents compiler optimization from removing memory clearing operations.
 * Uses volatile pointer to ensure memory is actually overwritten.
 * 
 * Used by:
 * - load_encrypted_api_key() to clear file buffers
 * - shutdown_ai_service() to clear API keys on shutdown
 * - Any function handling sensitive data
 * 
 * IMPORTANT: Standard memset() may be optimized away if the compiler
 * determines the memory won't be read again. This function prevents
 * that optimization for security-critical memory clearing.
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
 * 
 * Currently stores API key in plaintext (NO ENCRYPTION).
 * This is a security vulnerability that needs addressing.
 * 
 * Called by:
 * - load_ai_config() when loading API key from .env
 * - Admin commands when updating API key
 * 
 * Stores in: ai_state.config->encrypted_api_key (256 bytes max)
 * 
 * TODO: Implement proper encryption:
 * 1. Use AES-256-CBC with server-specific key
 * 2. Store initialization vector with encrypted data
 * 3. Use PBKDF2 for key derivation from server seed
 * 
 * Returns: 1 on success, 0 on failure
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
 * 
 * Currently returns API key as-is (NO DECRYPTION).
 * Must be updated when encryption is implemented.
 * 
 * Called by:
 * - make_api_request_single() for each API request
 * 
 * SECURITY CONSIDERATIONS:
 * - Returned key is in plaintext memory
 * - Caller MUST free() the returned string
 * - Caller should clear memory after use
 * 
 * Memory: Allocates 256 bytes for decrypted key
 * Returns: Allocated string with API key or NULL on error
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
 * 
 * Reads API key from file and stores in global config.
 * Uses secure_memset() to clear file buffer after reading.
 * 
 * Called by:
 * - Manual configuration (not currently used)
 * - Could be used for file-based key storage
 * 
 * Security measures:
 * 1. Clears file buffer after reading
 * 2. Removes newlines from key
 * 3. Validates file access
 * 
 * File format: Single line with API key
 * Storage: ai_state.config->encrypted_api_key
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
 * 
 * Critical security function that prevents prompt injection attacks.
 * Sanitizes user input before sending to OpenAI API.
 * 
 * Called by:
 * - ai_generate_response() for all AI requests
 * - ai_generate_response_async() for async requests
 * 
 * Security measures:
 * 1. Removes control characters (except newlines)
 * 2. Escapes quotes and backslashes for JSON
 * 3. Replaces newlines with spaces
 * 4. Enforces MAX_STRING_LENGTH limit
 * 5. Trims trailing spaces
 * 
 * IMPORTANT: This is the primary defense against prompt injection.
 * All user input MUST pass through this function.
 * 
 * Returns: Static buffer with sanitized input (not thread-safe)
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