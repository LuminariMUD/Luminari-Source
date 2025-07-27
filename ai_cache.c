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
  
  AI_DEBUG("ai_cache_response() called with key='%s', response_len=%zu",
           key ? key : "(null)", response ? strlen(response) : 0);
  
  if (!key || !response) {
    AI_DEBUG("ERROR: NULL key or response, aborting cache operation");
    return;
  }
  
  /* Check if key already exists and update it */
  AI_DEBUG("Searching for existing cache entry");
  int position = 0;
  for (entry = ai_state.cache_head; entry; entry = entry->next, position++) {
    if (!strcmp(entry->key, key)) {
      AI_DEBUG("Found existing entry at position %d", position);
      /* Update existing entry */
      if (entry->response) {
        AI_DEBUG("  Freeing old response (length=%zu)", strlen(entry->response));
        free(entry->response);
      }
      entry->response = strdup(response);
      if (!entry->response) {
        log("SYSERR: Failed to allocate memory for cache response update");
        AI_DEBUG("ERROR: strdup failed for response update");
        /* Keep old response if allocation fails */
        return;
      }
      entry->expires_at = now + AI_CACHE_EXPIRE_TIME;
      AI_DEBUG("  Entry updated, expires at %ld", (long)entry->expires_at);
      return;
    }
  }
  AI_DEBUG("No existing entry found, creating new one");
  
  /* Create new entry */
  AI_DEBUG("Creating new cache entry");
  CREATE(entry, struct ai_cache_entry, 1);
  if (!entry) {
    log("SYSERR: Failed to allocate memory for cache entry");
    AI_DEBUG("ERROR: Failed to allocate cache entry structure");
    return;
  }
  
  entry->key = strdup(key);
  if (!entry->key) {
    log("SYSERR: Failed to allocate memory for cache key");
    AI_DEBUG("ERROR: strdup failed for key");
    free(entry);
    return;
  }
  AI_DEBUG("  Key stored: '%s'", entry->key);
  
  entry->response = strdup(response);
  if (!entry->response) {
    log("SYSERR: Failed to allocate memory for cache response");
    AI_DEBUG("ERROR: strdup failed for response");
    free(entry->key);
    free(entry);
    return;
  }
  AI_DEBUG("  Response stored (length=%zu)", strlen(entry->response));
  
  entry->expires_at = now + AI_CACHE_EXPIRE_TIME;
  AI_DEBUG("  Entry expires at %ld (in %d seconds)", 
           (long)entry->expires_at, AI_CACHE_EXPIRE_TIME);
  
  entry->next = ai_state.cache_head;
  ai_state.cache_head = entry;
  ai_state.cache_size++;
  
  AI_DEBUG("Cache entry added, new cache size: %d/%d", 
           ai_state.cache_size, AI_MAX_CACHE_SIZE);
  
  /* Enforce cache size limit */
  if (ai_state.cache_size > AI_MAX_CACHE_SIZE) {
    AI_DEBUG("Cache size exceeded limit, running cleanup");
    ai_cache_cleanup();
  }
}

/**
 * Retrieve a response from the cache
 */
char *ai_cache_get(const char *key) {
  struct ai_cache_entry *entry;
  time_t now = time(NULL);
  
  AI_DEBUG("ai_cache_get() called with key='%s'", key ? key : "(null)");
  
  if (!key) {
    AI_DEBUG("ERROR: NULL key provided");
    return NULL;
  }
  
  AI_DEBUG("Searching cache (size=%d)", ai_state.cache_size);
  int position = 0;
  for (entry = ai_state.cache_head; entry; entry = entry->next, position++) {
    if (!strcmp(entry->key, key)) {
      AI_DEBUG("Found matching entry at position %d", position);
      /* Check if expired */
      if (entry->expires_at > now) {
        AI_DEBUG("  Entry valid (expires in %ld seconds)", 
                 (long)(entry->expires_at - now));
        AI_DEBUG("  Returning cached response (length=%zu)", 
                 strlen(entry->response));
        return entry->response;
      }
      /* Entry expired, will be cleaned up later */
      AI_DEBUG("  Entry expired %ld seconds ago", 
               (long)(now - entry->expires_at));
      break;
    }
  }
  
  AI_DEBUG("Cache miss - no valid entry found");
  return NULL;
}

/**
 * Clear all cache entries
 */
void ai_cache_clear(void) {
  struct ai_cache_entry *entry, *next;
  
  AI_DEBUG("ai_cache_clear() called - clearing all %d entries", ai_state.cache_size);
  
  for (entry = ai_state.cache_head; entry; entry = next) {
    next = entry->next;
    AI_DEBUG("  Clearing entry: key='%s'", 
             entry->key ? entry->key : "(null)");
    if (entry->key) free(entry->key);
    if (entry->response) free(entry->response);
    free(entry);
  }
  
  ai_state.cache_head = NULL;
  ai_state.cache_size = 0;
  
  AI_DEBUG("Cache cleared");
  log("AI cache cleared.");
}

/**
 * Remove expired entries and enforce size limits
 */
void ai_cache_cleanup(void) {
  struct ai_cache_entry *entry, *next, *prev = NULL;
  struct ai_cache_entry **sorted_entries = NULL;
  int removed_count = 0;
  int valid_count = 0;
  int i;
  time_t now = time(NULL);
  
  /* First pass: remove expired entries and count valid ones */
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
      valid_count++;
      prev = entry;
    }
  }
  
  /* Second pass: remove oldest entries if still over limit */
  if (ai_state.cache_size > AI_MAX_CACHE_SIZE) {
    int to_remove = ai_state.cache_size - AI_MAX_CACHE_SIZE;
    
    /* Create array of pointers to sort */
    CREATE(sorted_entries, struct ai_cache_entry *, ai_state.cache_size);
    if (!sorted_entries) {
      log("SYSERR: Failed to allocate memory for cache cleanup sorting");
      return;
    }
    
    /* Fill array with cache entries */
    i = 0;
    for (entry = ai_state.cache_head; entry && i < ai_state.cache_size; entry = entry->next) {
      sorted_entries[i++] = entry;
    }
    
    /* Sort by expiration time (oldest first) using simple selection sort */
    for (i = 0; i < ai_state.cache_size - 1 && i < to_remove; i++) {
      int min_idx = i;
      int j;
      for (j = i + 1; j < ai_state.cache_size; j++) {
        if (sorted_entries[j]->expires_at < sorted_entries[min_idx]->expires_at) {
          min_idx = j;
        }
      }
      if (min_idx != i) {
        struct ai_cache_entry *temp = sorted_entries[i];
        sorted_entries[i] = sorted_entries[min_idx];
        sorted_entries[min_idx] = temp;
      }
    }
    
    /* Remove the oldest entries */
    for (i = 0; i < to_remove && i < ai_state.cache_size; i++) {
      entry = sorted_entries[i];
      
      /* Find and remove from linked list */
      prev = NULL;
      struct ai_cache_entry *curr;
      for (curr = ai_state.cache_head; curr; curr = curr->next) {
        if (curr == entry) {
          if (prev) {
            prev->next = curr->next;
          } else {
            ai_state.cache_head = curr->next;
          }
          break;
        }
        prev = curr;
      }
      
      if (entry->key) free(entry->key);
      if (entry->response) free(entry->response);
      free(entry);
      ai_state.cache_size--;
      removed_count++;
    }
    
    free(sorted_entries);
  }
  
  if (removed_count > 0) {
    log("AI cache cleanup: removed %d entries", removed_count);
  }
}