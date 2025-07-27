/**
 * @file ai_cache.c
 * @author Zusuk
 * @brief AI response caching system
 * 
 * Implements a simple LRU cache for AI responses to reduce API calls
 * and improve response times for frequently asked questions.
 * 
 * Part of the LuminariMUD distribution.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "ai_service.h"

/* External reference to global AI state */
extern struct ai_service_state ai_state;

/**
 * Add or update a response in the cache
 */
void ai_cache_response(const char *key, const char *response) {
  struct ai_cache_entry *entry;
  time_t now = time(NULL);
  
  if (!key || !response) return;
  
  /* Check if key already exists and update it */
  for (entry = ai_state.cache_head; entry; entry = entry->next) {
    if (!strcmp(entry->key, key)) {
      /* Update existing entry */
      if (entry->response) free(entry->response);
      entry->response = strdup(response);
      entry->expires_at = now + AI_CACHE_EXPIRE_TIME;
      return;
    }
  }
  
  /* Create new entry */
  CREATE(entry, struct ai_cache_entry, 1);
  entry->key = strdup(key);
  entry->response = strdup(response);
  entry->expires_at = now + AI_CACHE_EXPIRE_TIME;
  entry->next = ai_state.cache_head;
  ai_state.cache_head = entry;
  ai_state.cache_size++;
  
  /* Enforce cache size limit */
  if (ai_state.cache_size > AI_MAX_CACHE_SIZE) {
    ai_cache_cleanup();
  }
}

/**
 * Retrieve a response from the cache
 */
char *ai_cache_get(const char *key) {
  struct ai_cache_entry *entry;
  time_t now = time(NULL);
  
  if (!key) return NULL;
  
  for (entry = ai_state.cache_head; entry; entry = entry->next) {
    if (!strcmp(entry->key, key)) {
      /* Check if expired */
      if (entry->expires_at > now) {
        return entry->response;
      }
      /* Entry expired, will be cleaned up later */
      break;
    }
  }
  
  return NULL;
}

/**
 * Clear all cache entries
 */
void ai_cache_clear(void) {
  struct ai_cache_entry *entry, *next;
  
  for (entry = ai_state.cache_head; entry; entry = next) {
    next = entry->next;
    if (entry->key) free(entry->key);
    if (entry->response) free(entry->response);
    free(entry);
  }
  
  ai_state.cache_head = NULL;
  ai_state.cache_size = 0;
  
  log("AI cache cleared.");
}

/**
 * Remove expired entries and enforce size limits
 */
void ai_cache_cleanup(void) {
  struct ai_cache_entry *entry, *next, *prev = NULL;
  struct ai_cache_entry *oldest = NULL, *oldest_prev = NULL;
  time_t now = time(NULL);
  time_t oldest_time = now + AI_CACHE_EXPIRE_TIME;
  int removed_count = 0;
  
  /* First pass: remove expired entries */
  for (entry = ai_state.cache_head; entry; entry = next) {
    next = entry->next;
    
    if (entry->expires_at <= now) {
      /* Remove expired entry */
      if (prev) {
        prev->next = next;
      } else {
        ai_state.cache_head = next;
      }
      
      if (entry->key) free(entry->key);
      if (entry->response) free(entry->response);
      free(entry);
      ai_state.cache_size--;
      removed_count++;
    } else {
      /* Track oldest non-expired entry */
      if (entry->expires_at < oldest_time) {
        oldest_time = entry->expires_at;
        oldest = entry;
        oldest_prev = prev;
      }
      prev = entry;
    }
  }
  
  /* Second pass: remove oldest entries if still over limit */
  while (ai_state.cache_size > AI_MAX_CACHE_SIZE && oldest) {
    if (oldest_prev) {
      oldest_prev->next = oldest->next;
    } else {
      ai_state.cache_head = oldest->next;
    }
    
    if (oldest->key) free(oldest->key);
    if (oldest->response) free(oldest->response);
    free(oldest);
    ai_state.cache_size--;
    removed_count++;
    
    /* Find next oldest */
    oldest = NULL;
    oldest_prev = NULL;
    oldest_time = now + AI_CACHE_EXPIRE_TIME;
    prev = NULL;
    
    for (entry = ai_state.cache_head; entry; entry = entry->next) {
      if (entry->expires_at < oldest_time) {
        oldest_time = entry->expires_at;
        oldest = entry;
        oldest_prev = prev;
      }
      prev = entry;
    }
  }
  
  if (removed_count > 0) {
    log("AI cache cleanup: removed %d entries", removed_count);
  }
}