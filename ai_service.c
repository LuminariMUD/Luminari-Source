/**
 * @file ai_service.c
 * @author Zusuk
 * @brief OpenAI API integration service implementation
 * 
 * Core implementation of AI service functionality including:
 * - API request handling with CURL
 * - JSON parsing and building
 * - Error handling and retries
 * - Rate limiting
 * 
 * Part of the LuminariMUD distribution.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "ai_service.h"
#include "dotenv.h"
#include <curl/curl.h>

/* Global AI Service State */
struct ai_service_state ai_state = {0};

/* CURL Response Buffer */
struct curl_response {
  char *data;
  size_t size;
};

/* Function prototypes for static functions */
static size_t curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp);
static char *make_api_request(const char *prompt);
static char *build_json_request(const char *prompt);
static char *parse_json_response(const char *json_str);
static void derive_key_from_seed(unsigned char *key);

/**
 * CURL write callback for receiving API responses
 */
static size_t curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct curl_response *mem = (struct curl_response *)userp;
  char *ptr;
  
  ptr = realloc(mem->data, mem->size + realsize + 1);
  if (!ptr) return 0;
  
  mem->data = ptr;
  memcpy(&(mem->data[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->data[mem->size] = '\0';
  
  return realsize;
}

/**
 * Initialize the AI service
 */
void init_ai_service(void) {
  /* Initialize CURL globally */
  curl_global_init(CURL_GLOBAL_ALL);
  
  /* Allocate configuration structure */
  CREATE(ai_state.config, struct ai_config, 1);
  
  /* Set default configuration */
  strcpy(ai_state.config->model, "gpt-4.1-mini");
  ai_state.config->max_tokens = AI_MAX_TOKENS;
  ai_state.config->temperature = 0.7;
  ai_state.config->timeout_ms = AI_TIMEOUT_MS;
  ai_state.config->content_filter_enabled = TRUE;
  ai_state.config->enabled = FALSE;  /* Disabled by default */
  
  /* Allocate rate limiter */
  CREATE(ai_state.limiter, struct rate_limiter, 1);
  
  /* Initialize rate limiter */
  ai_state.limiter->requests_per_minute = 60;
  ai_state.limiter->requests_per_hour = 1000;
  ai_state.limiter->minute_reset = time(NULL) + 60;
  ai_state.limiter->hour_reset = time(NULL) + 3600;
  ai_state.limiter->current_minute_count = 0;
  ai_state.limiter->current_hour_count = 0;
  
  /* Initialize cache */
  ai_state.cache_head = NULL;
  ai_state.cache_size = 0;
  
  /* Load configuration from database/files */
  load_ai_config();
  
  ai_state.initialized = TRUE;
  log("AI Service initialized successfully.");
}

/**
 * Shutdown the AI service and free resources
 */
void shutdown_ai_service(void) {
  struct ai_cache_entry *entry, *next;
  
  if (!ai_state.initialized) return;
  
  /* Clean up cache */
  for (entry = ai_state.cache_head; entry; entry = next) {
    next = entry->next;
    if (entry->key) free(entry->key);
    if (entry->response) free(entry->response);
    free(entry);
  }
  
  /* Free configuration */
  if (ai_state.config) {
    secure_memset(ai_state.config->encrypted_api_key, 0, 
                  sizeof(ai_state.config->encrypted_api_key));
    free(ai_state.config);
  }
  
  /* Free rate limiter */
  if (ai_state.limiter) {
    free(ai_state.limiter);
  }
  
  /* Cleanup CURL */
  curl_global_cleanup();
  
  ai_state.initialized = FALSE;
  log("AI Service shutdown complete.");
}

/**
 * Check if AI service is enabled
 */
bool is_ai_enabled(void) {
  return ai_state.initialized && ai_state.config && ai_state.config->enabled;
}

/**
 * Load AI configuration from database/files
 */
void load_ai_config(void) {
  char *api_key;
  char *model;
  
  if (!ai_state.config) {
    log("SYSERR: AI config not allocated");
    return;
  }
  
  /* Load API key from .env */
  api_key = get_env_value("OPENAI_API_KEY");
  if (api_key && *api_key) {
    /* Encrypt and store the API key */
    encrypt_api_key(api_key, ai_state.config->encrypted_api_key);
    log("AI Service: API key loaded from .env file");
  } else {
    log("AI Service: No API key found in .env file (OPENAI_API_KEY)");
  }
  
  /* Load model configuration */
  model = get_env_value("AI_MODEL");
  if (model && *model) {
    strlcpy(ai_state.config->model, model, sizeof(ai_state.config->model));
  }
  
  /* Load numeric configurations */
  ai_state.config->max_tokens = get_env_int("AI_MAX_TOKENS", AI_MAX_TOKENS);
  ai_state.config->temperature = (float)get_env_int("AI_TEMPERATURE", 7) / 10.0;
  ai_state.config->timeout_ms = get_env_int("AI_TIMEOUT_MS", AI_TIMEOUT_MS);
  ai_state.config->content_filter_enabled = get_env_bool("AI_CONTENT_FILTER_ENABLED", TRUE);
  
  /* Load rate limits */
  if (ai_state.limiter) {
    ai_state.limiter->requests_per_minute = get_env_int("AI_REQUESTS_PER_MINUTE", 60);
    ai_state.limiter->requests_per_hour = get_env_int("AI_REQUESTS_PER_HOUR", 1000);
  }
  
  log("AI configuration loaded from .env");
}

/**
 * Make an API request to OpenAI
 */
static char *make_api_request(const char *prompt) {
  CURL *curl;
  CURLcode res;
  struct curl_response response = {0};
  struct curl_slist *headers = NULL;
  char auth_header[512];
  char *api_key;
  char *json_request;
  char *result = NULL;
  int retry_count = 0;
  long http_code;
  
  if (!ai_state.initialized) {
    log("SYSERR: AI Service not initialized");
    return NULL;
  }
  
  /* Check rate limits */
  if (!ai_check_rate_limit()) {
    log("AI Service: Rate limit exceeded");
    return NULL;
  }
  
  /* Decrypt API key */
  api_key = decrypt_api_key(ai_state.config->encrypted_api_key);
  if (!api_key) {
    log("SYSERR: Failed to decrypt API key");
    return NULL;
  }
  
  /* Build JSON request */
  json_request = build_json_request(prompt);
  if (!json_request) {
    free(api_key);
    return NULL;
  }
  
  /* Retry loop */
  while (retry_count < AI_MAX_RETRIES) {
    curl = curl_easy_init();
    if (!curl) break;
    
    /* Set headers */
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    /* Configure CURL */
    curl_easy_setopt(curl, CURLOPT_URL, OPENAI_API_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_request);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)ai_state.config->timeout_ms);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    /* Execute request */
    res = curl_easy_perform(curl);
    
    if (res == CURLE_OK) {
      http_code = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
      
      if (http_code == 200) {
        result = parse_json_response(response.data);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        break;
      } else {
        log("AI Service: HTTP error %ld", http_code);
      }
    } else {
      log("AI Service: CURL error: %s", curl_easy_strerror(res));
    }
    
    /* Cleanup for retry */
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    headers = NULL;
    
    if (response.data) {
      free(response.data);
      response.data = NULL;
      response.size = 0;
    }
    
    retry_count++;
    if (retry_count < AI_MAX_RETRIES) {
      /* Simple exponential backoff using sleep */
      sleep(1 << retry_count);
    }
  }
  
  /* Cleanup */
  secure_memset(api_key, 0, strlen(api_key));
  free(api_key);
  free(json_request);
  if (response.data) free(response.data);
  
  return result;
}

/**
 * Generate an AI response for a given prompt
 */
char *ai_generate_response(const char *prompt, int request_type) {
  char sanitized_prompt[MAX_STRING_LENGTH];
  char *response;
  
  if (!is_ai_enabled()) {
    return generate_fallback_response(prompt);
  }
  
  /* Sanitize input */
  strlcpy(sanitized_prompt, sanitize_ai_input(prompt), sizeof(sanitized_prompt));
  
  /* Make API request */
  response = make_api_request(sanitized_prompt);
  
  if (!response) {
    log("AI request failed for type %d", request_type);
    return generate_fallback_response(prompt);
  }
  
  return response;
}

/**
 * Generate AI dialogue for an NPC
 */
char *ai_npc_dialogue(struct char_data *npc, struct char_data *ch, const char *input) {
  char prompt[MAX_STRING_LENGTH];
  char cache_key[256];
  char *cached_response;
  char *response;
  
  if (!npc || !ch || !input || !*input) return NULL;
  
  /* Build cache key */
  snprintf(cache_key, sizeof(cache_key), "npc_%d_%s", 
           GET_MOB_VNUM(npc), input);
  
  /* Check cache */
  cached_response = ai_cache_get(cache_key);
  if (cached_response) return strdup(cached_response);
  
  /* Build prompt */
  snprintf(prompt, sizeof(prompt),
    "You are %s in a fantasy RPG world. "
    "Respond to the player's message in character. "
    "Keep response under 100 words. "
    "Player says: \"%s\"",
    GET_NAME(npc), input);
  
  /* Get AI response */
  response = ai_generate_response(prompt, AI_REQUEST_NPC_DIALOGUE);
  
  /* Cache if successful */
  if (response) {
    ai_cache_response(cache_key, response);
  }
  
  return response;
}

/**
 * Check rate limiting
 */
bool ai_check_rate_limit(void) {
  time_t now = time(NULL);
  
  if (!ai_state.limiter) return FALSE;
  
  /* Reset counters if needed */
  if (now >= ai_state.limiter->minute_reset) {
    ai_state.limiter->current_minute_count = 0;
    ai_state.limiter->minute_reset = now + 60;
  }
  
  if (now >= ai_state.limiter->hour_reset) {
    ai_state.limiter->current_hour_count = 0;
    ai_state.limiter->hour_reset = now + 3600;
  }
  
  /* Check limits */
  if (ai_state.limiter->current_minute_count >= ai_state.limiter->requests_per_minute ||
      ai_state.limiter->current_hour_count >= ai_state.limiter->requests_per_hour) {
    return FALSE;
  }
  
  /* Increment counters */
  ai_state.limiter->current_minute_count++;
  ai_state.limiter->current_hour_count++;
  
  return TRUE;
}

/**
 * Reset rate limits (admin command)
 */
void ai_reset_rate_limits(void) {
  if (!ai_state.limiter) return;
  
  ai_state.limiter->current_minute_count = 0;
  ai_state.limiter->current_hour_count = 0;
  ai_state.limiter->minute_reset = time(NULL) + 60;
  ai_state.limiter->hour_reset = time(NULL) + 3600;
}

/**
 * Build JSON request for OpenAI API
 * Note: This is a simplified version. Full implementation would use json-c library
 */
static char *build_json_request(const char *prompt) {
  static char json_buffer[MAX_STRING_LENGTH * 2];
  char escaped_prompt[MAX_STRING_LENGTH];
  const char *src;
  char *dest;
  
  /* Simple JSON escaping */
  src = prompt;
  dest = escaped_prompt;
  while (*src && dest - escaped_prompt < MAX_STRING_LENGTH - 2) {
    if (*src == '"' || *src == '\\') {
      *dest++ = '\\';
    }
    *dest++ = *src++;
  }
  *dest = '\0';
  
  /* Build JSON request */
  snprintf(json_buffer, sizeof(json_buffer),
    "{"
    "\"model\": \"%s\","
    "\"messages\": ["
    "{\"role\": \"user\", \"content\": \"%s\"}"
    "],"
    "\"max_tokens\": %d,"
    "\"temperature\": %.2f"
    "}",
    ai_state.config->model,
    escaped_prompt,
    ai_state.config->max_tokens,
    ai_state.config->temperature);
  
  return strdup(json_buffer);
}

/**
 * Parse JSON response from OpenAI API
 * Note: This is a simplified version. Full implementation would use json-c library
 */
static char *parse_json_response(const char *json_str) {
  char *content_start, *content_end;
  char *result;
  size_t len;
  
  /* Find "content": " */
  content_start = strstr(json_str, "\"content\":");
  if (!content_start) return NULL;
  
  content_start = strchr(content_start, '"');
  if (!content_start) return NULL;
  content_start++; /* Skip opening quote */
  
  content_start = strchr(content_start, '"');
  if (!content_start) return NULL;
  content_start++; /* Skip opening quote of value */
  
  /* Find closing quote */
  content_end = content_start;
  while (*content_end && !(*content_end == '"' && *(content_end - 1) != '\\')) {
    content_end++;
  }
  
  if (!*content_end) return NULL;
  
  /* Extract content */
  len = content_end - content_start;
  CREATE(result, char, len + 1);
  strncpy(result, content_start, len);
  result[len] = '\0';
  
  return result;
}

/**
 * Log AI errors
 */
void log_ai_error(const char *function, const char *error) {
  log("SYSERR: AI Service [%s]: %s", function, error);
}

/**
 * Log AI interactions for debugging/monitoring
 */
void log_ai_interaction(struct char_data *ch, struct char_data *npc, const char *response) {
  /* This would log to database in production */
  if (ch && npc && response) {
    log("AI: %s -> %s: %s", GET_NAME(ch), GET_NAME(npc), response);
  }
}

/**
 * Generate fallback response when AI is unavailable
 */
char *generate_fallback_response(const char *prompt) {
  static char *fallback_responses[] = {
    "I don't understand what you're saying.",
    "Could you repeat that?",
    "I'm not sure how to respond to that.",
    "That's interesting...",
    "I see.",
    NULL
  };
  int num_responses = 0;
  int choice;
  char **ptr;
  
  /* Count responses */
  for (ptr = fallback_responses; *ptr; ptr++) {
    num_responses++;
  }
  
  /* Select random response */
  choice = number(0, num_responses - 1);
  
  return strdup(fallback_responses[choice]);
}

/**
 * Generate room description (stub for now)
 */
char *ai_generate_room_desc(int room_vnum, int sector_type) {
  /* TODO: Implement room description generation */
  return NULL;
}

/**
 * Content moderation (stub for now)
 */
bool ai_moderate_content(const char *text) {
  /* TODO: Implement content moderation */
  return TRUE;
}

/**
 * Get cache size
 */
int get_cache_size(void) {
  return ai_state.cache_size;
}