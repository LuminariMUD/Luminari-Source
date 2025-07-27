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
#include <unistd.h>

/* Global AI Service State */
struct ai_service_state ai_state = {0};

/* CURL Response Buffer */
struct curl_response {
  char *data;
  size_t size;
};

/* Function prototypes for static functions */
static size_t ai_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp);
static char *make_api_request(const char *prompt);
static char *build_json_request(const char *prompt);
static char *parse_json_response(const char *json_str);
/* static void derive_key_from_seed(unsigned char *key); */

/**
 * CURL write callback for receiving API responses
 */
static size_t ai_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct curl_response *mem = (struct curl_response *)userp;
  char *ptr;
  
  AI_DEBUG("CURL callback received %zu bytes (size=%zu, nmemb=%zu)", realsize, size, nmemb);
  AI_DEBUG("Current buffer size: %zu bytes", mem->size);
  
  ptr = realloc(mem->data, mem->size + realsize + 1);
  if (!ptr) {
    AI_DEBUG("ERROR: Failed to realloc buffer from %zu to %zu bytes", 
             mem->size, mem->size + realsize + 1);
    return 0;
  }
  
  mem->data = ptr;
  memcpy(&(mem->data[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->data[mem->size] = '\0';
  
  AI_DEBUG("Buffer successfully expanded to %zu bytes", mem->size);
  AI_DEBUG("Response chunk: %.100s%s", (char*)contents, 
           realsize > 100 ? "..." : "");
  
  return realsize;
}


/**
 * Derive encryption key from server seed
 * NOTE: This is a placeholder - implement proper key derivation
 */
/*
static void derive_key_from_seed(unsigned char *key) {
  int i;
  // Simple placeholder - in production, use proper KDF
  for (i = 0; i < 32; i++) {
    key[i] = (unsigned char)(rand() % 256);
  }
}
*/

/**
 * Initialize the AI service
 */
void init_ai_service(void) {
  AI_DEBUG("Starting AI service initialization");
  AI_DEBUG("AI state at start: initialized=%d, config=%p, limiter=%p, cache_size=%d",
           ai_state.initialized, (void*)ai_state.config, (void*)ai_state.limiter, ai_state.cache_size);
  
  /* Initialize CURL globally */
  AI_DEBUG("Initializing CURL library with CURL_GLOBAL_ALL flags");
  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    log("SYSERR: Failed to initialize CURL library for AI service");
    AI_DEBUG("ERROR: curl_global_init failed");
    return;
  }
  AI_DEBUG("CURL library initialized successfully")
  
  /* Allocate configuration structure */
  AI_DEBUG("Allocating AI configuration structure (size=%zu)", sizeof(struct ai_config));
  CREATE(ai_state.config, struct ai_config, 1);
  if (!ai_state.config) {
    log("SYSERR: Failed to allocate AI configuration structure");
    AI_DEBUG("ERROR: Failed to allocate %zu bytes for ai_config", sizeof(struct ai_config));
    curl_global_cleanup();
    return;
  }
  AI_DEBUG("AI configuration allocated at %p", (void*)ai_state.config)
  
  /* Set default configuration */
  AI_DEBUG("Setting default configuration values");
  strcpy(ai_state.config->model, "gpt-4.1-mini");
  AI_DEBUG("  Model: %s", ai_state.config->model);
  ai_state.config->max_tokens = AI_MAX_TOKENS;
  AI_DEBUG("  Max tokens: %d", ai_state.config->max_tokens);
  ai_state.config->temperature = 0.7;
  AI_DEBUG("  Temperature: %.2f", ai_state.config->temperature);
  ai_state.config->timeout_ms = AI_TIMEOUT_MS;
  AI_DEBUG("  Timeout: %d ms", ai_state.config->timeout_ms);
  ai_state.config->content_filter_enabled = TRUE;
  AI_DEBUG("  Content filter: %s", ai_state.config->content_filter_enabled ? "ENABLED" : "DISABLED");
  ai_state.config->enabled = FALSE;  /* Disabled by default */
  AI_DEBUG("  Service enabled: %s", ai_state.config->enabled ? "YES" : "NO")
  
  /* Allocate rate limiter */
  AI_DEBUG("Allocating rate limiter structure (size=%zu)", sizeof(struct rate_limiter));
  CREATE(ai_state.limiter, struct rate_limiter, 1);
  if (!ai_state.limiter) {
    log("SYSERR: Failed to allocate AI rate limiter");
    AI_DEBUG("ERROR: Failed to allocate %zu bytes for rate_limiter", sizeof(struct rate_limiter));
    free(ai_state.config);
    ai_state.config = NULL;
    curl_global_cleanup();
    return;
  }
  AI_DEBUG("Rate limiter allocated at %p", (void*)ai_state.limiter)
  
  /* Initialize rate limiter */
  AI_DEBUG("Initializing rate limiter with default values");
  ai_state.limiter->requests_per_minute = 60;
  AI_DEBUG("  Requests per minute: %d", ai_state.limiter->requests_per_minute);
  ai_state.limiter->requests_per_hour = 1000;
  AI_DEBUG("  Requests per hour: %d", ai_state.limiter->requests_per_hour);
  ai_state.limiter->minute_reset = time(NULL) + 60;
  AI_DEBUG("  Minute reset time: %ld", (long)ai_state.limiter->minute_reset);
  ai_state.limiter->hour_reset = time(NULL) + 3600;
  AI_DEBUG("  Hour reset time: %ld", (long)ai_state.limiter->hour_reset);
  ai_state.limiter->current_minute_count = 0;
  ai_state.limiter->current_hour_count = 0;
  AI_DEBUG("  Current counts: minute=%d, hour=%d", 
           ai_state.limiter->current_minute_count, ai_state.limiter->current_hour_count)
  
  /* Initialize cache */
  AI_DEBUG("Initializing AI response cache");
  ai_state.cache_head = NULL;
  ai_state.cache_size = 0;
  AI_DEBUG("  Cache initialized: head=%p, size=%d, max_size=%d", 
           (void*)ai_state.cache_head, ai_state.cache_size, AI_MAX_CACHE_SIZE);
  
  /* Load configuration from database/files */
  AI_DEBUG("Loading AI configuration from .env file");
  load_ai_config();
  
  ai_state.initialized = TRUE;
  AI_DEBUG("AI Service initialization complete - initialized=%d", ai_state.initialized);
  log("AI Service initialized successfully.");
}

/**
 * Shutdown the AI service and free resources
 */
void shutdown_ai_service(void) {
  struct ai_cache_entry *entry, *next;
  
  AI_DEBUG("Starting AI service shutdown");
  
  if (!ai_state.initialized) {
    AI_DEBUG("AI service not initialized, skipping shutdown");
    return;
  }
  
  /* Clean up cache */
  AI_DEBUG("Cleaning up cache entries (size=%d)", ai_state.cache_size);
  int freed_count = 0;
  for (entry = ai_state.cache_head; entry; entry = next) {
    next = entry->next;
    AI_DEBUG("  Freeing cache entry %d: key='%s', response_len=%zu", 
             freed_count++, entry->key ? entry->key : "(null)", 
             entry->response ? strlen(entry->response) : 0);
    if (entry->key) free(entry->key);
    if (entry->response) free(entry->response);
    free(entry);
  }
  AI_DEBUG("Freed %d cache entries", freed_count)
  
  /* Free configuration */
  if (ai_state.config) {
    AI_DEBUG("Freeing AI configuration at %p", (void*)ai_state.config);
    AI_DEBUG("  Clearing API key from memory");
    secure_memset(ai_state.config->encrypted_api_key, 0, 
                  sizeof(ai_state.config->encrypted_api_key));
    free(ai_state.config);
    ai_state.config = NULL;
  }
  
  /* Free rate limiter */
  if (ai_state.limiter) {
    AI_DEBUG("Freeing rate limiter at %p", (void*)ai_state.limiter);
    AI_DEBUG("  Final stats: minute_count=%d/%d, hour_count=%d/%d",
             ai_state.limiter->current_minute_count, ai_state.limiter->requests_per_minute,
             ai_state.limiter->current_hour_count, ai_state.limiter->requests_per_hour);
    free(ai_state.limiter);
    ai_state.limiter = NULL;
  }
  
  /* Cleanup CURL */
  AI_DEBUG("Cleaning up CURL library");
  curl_global_cleanup();
  
  ai_state.initialized = FALSE;
  AI_DEBUG("AI Service shutdown complete - initialized=%d", ai_state.initialized);
  log("AI Service shutdown complete.");
}

/**
 * Check if AI service is enabled
 */
bool is_ai_enabled(void) {
  bool result = ai_state.initialized && ai_state.config && ai_state.config->enabled;
  AI_DEBUG("Checking if AI enabled: initialized=%d, config=%p, enabled=%d, result=%d",
           ai_state.initialized, (void*)ai_state.config, 
           ai_state.config ? ai_state.config->enabled : -1, result);
  return result;
}

/**
 * Load AI configuration from database/files
 */
void load_ai_config(void) {
  char *api_key;
  char *model;
  
  AI_DEBUG("Loading AI configuration from environment");
  
  if (!ai_state.config) {
    log("SYSERR: AI config not allocated");
    AI_DEBUG("ERROR: ai_state.config is NULL");
    return;
  }
  
  /* Load API key from .env */
  AI_DEBUG("Looking for OPENAI_API_KEY in environment");
  api_key = get_env_value("OPENAI_API_KEY");
  if (api_key && *api_key) {
    AI_DEBUG("  Found API key (length=%zu)", strlen(api_key));
    AI_DEBUG("  First 10 chars: %.10s...", api_key);
    /* Encrypt and store the API key */
    encrypt_api_key(api_key, ai_state.config->encrypted_api_key);
    AI_DEBUG("  API key encrypted and stored");
    log("AI Service: API key loaded from .env file");
  } else {
    AI_DEBUG("  No API key found in environment");
    log("AI Service: No API key found in .env file (OPENAI_API_KEY)");
  }
  
  /* Load model configuration */
  AI_DEBUG("Looking for AI_MODEL in environment");
  model = get_env_value("AI_MODEL");
  if (model && *model) {
    AI_DEBUG("  Found model: %s", model);
    strlcpy(ai_state.config->model, model, sizeof(ai_state.config->model));
  } else {
    AI_DEBUG("  Using default model: %s", ai_state.config->model);
  }
  
  /* Load numeric configurations */
  AI_DEBUG("Loading numeric configurations");
  ai_state.config->max_tokens = get_env_int("AI_MAX_TOKENS", AI_MAX_TOKENS);
  AI_DEBUG("  Max tokens: %d (env: %s)", ai_state.config->max_tokens, 
           get_env_value("AI_MAX_TOKENS") ? "set" : "default");
           
  ai_state.config->temperature = (float)get_env_int("AI_TEMPERATURE", 7) / 10.0;
  AI_DEBUG("  Temperature: %.2f (env: %s)", ai_state.config->temperature,
           get_env_value("AI_TEMPERATURE") ? "set" : "default");
  
  /* Load timeout with validation */
  ai_state.config->timeout_ms = get_env_int("AI_TIMEOUT_MS", AI_TIMEOUT_MS);
  AI_DEBUG("  Raw timeout value: %d ms", ai_state.config->timeout_ms);
  if (ai_state.config->timeout_ms < 1) {
    log("AI Service: Invalid timeout_ms %d, using minimum 1ms", ai_state.config->timeout_ms);
    AI_DEBUG("  Timeout too low, adjusting to 1ms");
    ai_state.config->timeout_ms = 1;
  } else if (ai_state.config->timeout_ms > 300000) {
    log("AI Service: Timeout_ms %d exceeds maximum, clamping to 300000ms (5 minutes)", 
        ai_state.config->timeout_ms);
    AI_DEBUG("  Timeout too high, clamping to 300000ms");
    ai_state.config->timeout_ms = 300000;
  }
  AI_DEBUG("  Final timeout: %d ms", ai_state.config->timeout_ms);
  
  ai_state.config->content_filter_enabled = get_env_bool("AI_CONTENT_FILTER_ENABLED", TRUE);
  AI_DEBUG("  Content filter: %s (env: %s)", 
           ai_state.config->content_filter_enabled ? "ENABLED" : "DISABLED",
           get_env_value("AI_CONTENT_FILTER_ENABLED") ? "set" : "default")
  
  /* Load rate limits */
  if (ai_state.limiter) {
    AI_DEBUG("Loading rate limit configuration");
    ai_state.limiter->requests_per_minute = get_env_int("AI_REQUESTS_PER_MINUTE", 60);
    AI_DEBUG("  Requests per minute: %d (env: %s)", ai_state.limiter->requests_per_minute,
             get_env_value("AI_REQUESTS_PER_MINUTE") ? "set" : "default");
    ai_state.limiter->requests_per_hour = get_env_int("AI_REQUESTS_PER_HOUR", 1000);
    AI_DEBUG("  Requests per hour: %d (env: %s)", ai_state.limiter->requests_per_hour,
             get_env_value("AI_REQUESTS_PER_HOUR") ? "set" : "default");
  } else {
    AI_DEBUG("WARNING: Rate limiter not allocated, skipping rate limit config");
  }
  
  AI_DEBUG("AI configuration loading complete");
  log("AI configuration loaded from .env");
}

/**
 * Make an API request to OpenAI (internal - no retries)
 */
static char *make_api_request_single(const char *prompt) {
  CURL *curl;
  CURLcode res;
  struct curl_response response = {0};
  struct curl_slist *headers = NULL;
  char auth_header[512];
  char *api_key;
  char *json_request;
  char *result = NULL;
  long http_code;
  
  AI_DEBUG("make_api_request_single() called with prompt: '%.50s%s'",
           prompt, strlen(prompt) > 50 ? "..." : "");
  
  if (!ai_state.initialized) {
    log("SYSERR: AI Service not initialized");
    AI_DEBUG("ERROR: Service not initialized, aborting request");
    return NULL;
  }
  
  /* Check rate limits */
  AI_DEBUG("Checking rate limits");
  if (!ai_check_rate_limit()) {
    log("AI Service: Rate limit exceeded");
    AI_DEBUG("Rate limit exceeded - request blocked");
    return NULL;
  }
  AI_DEBUG("Rate limit check passed")
  
  /* Decrypt API key */
  AI_DEBUG("Decrypting API key");
  api_key = decrypt_api_key(ai_state.config->encrypted_api_key);
  if (!api_key) {
    log("SYSERR: Failed to decrypt API key");
    AI_DEBUG("ERROR: API key decryption failed");
    return NULL;
  }
  AI_DEBUG("API key decrypted successfully (length=%zu)", strlen(api_key));
  
  /* Build JSON request */
  AI_DEBUG("Building JSON request");
  json_request = build_json_request(prompt);
  if (!json_request) {
    AI_DEBUG("ERROR: Failed to build JSON request");
    free(api_key);
    return NULL;
  }
  AI_DEBUG("JSON request built (length=%zu)", strlen(json_request));
  AI_DEBUG("JSON content: %.200s%s", json_request, 
           strlen(json_request) > 200 ? "..." : "")
  
  AI_DEBUG("Initializing CURL handle");
  curl = curl_easy_init();
  if (!curl) {
    log("SYSERR: Failed to initialize CURL handle in make_api_request_single");
    AI_DEBUG("ERROR: curl_easy_init() returned NULL");
    free(api_key);
    /* Also free json_request which was allocated above */
    /* Note: json_request points to static buffer, so no need to free */
    return NULL;
  }
  AI_DEBUG("CURL handle initialized at %p", (void*)curl);
  
  /* Set headers */
  AI_DEBUG("Setting up HTTP headers");
  snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
  AI_DEBUG("  Auth header length: %zu", strlen(auth_header));
  headers = curl_slist_append(headers, auth_header);
  if (!headers) {
    log("SYSERR: Failed to allocate CURL header list (auth header)");
    AI_DEBUG("ERROR: curl_slist_append failed for auth header");
    curl_easy_cleanup(curl);
    free(api_key);
    return NULL;
  }
  AI_DEBUG("  Auth header added successfully");
  
  headers = curl_slist_append(headers, "Content-Type: application/json");
  if (!headers) {
    log("SYSERR: Failed to allocate CURL header list (content-type)");
    AI_DEBUG("ERROR: curl_slist_append failed for content-type header");
    curl_easy_cleanup(curl);
    free(api_key);
    return NULL;
  }
  AI_DEBUG("  Content-Type header added successfully")
  
  /* Configure CURL */
  AI_DEBUG("Configuring CURL options");
  curl_easy_setopt(curl, CURLOPT_URL, OPENAI_API_ENDPOINT);
  AI_DEBUG("  URL: %s", OPENAI_API_ENDPOINT);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  AI_DEBUG("  Headers set");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_request);
  AI_DEBUG("  POST data length: %zu bytes", strlen(json_request));
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ai_curl_write_callback);
  AI_DEBUG("  Write callback set to ai_curl_write_callback");
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  AI_DEBUG("  Write data pointer set to response buffer");
  
  /* Safely set timeout - clamp to reasonable range (1ms to 300000ms/5 minutes) */
  long timeout_ms = ai_state.config->timeout_ms;
  if (timeout_ms < 1) timeout_ms = 1;
  if (timeout_ms > 300000) timeout_ms = 300000;
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
  AI_DEBUG("  Timeout set to %ld ms", timeout_ms);
  
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
  AI_DEBUG("  SSL verification enabled (peer=1, host=2)")
  
  /* Execute request */
  AI_DEBUG("Executing CURL request to API endpoint");
  res = curl_easy_perform(curl);
  AI_DEBUG("CURL request completed with result: %d (%s)", res, curl_easy_strerror(res));
  
  if (res == CURLE_OK) {
    http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    AI_DEBUG("HTTP response code: %ld", http_code);
    AI_DEBUG("Response size: %zu bytes", response.size);
    
    if (http_code == 200) {
      AI_DEBUG("Success! Parsing JSON response");
      AI_DEBUG("Raw response: %.500s%s", response.data,
               response.size > 500 ? "..." : "");
      result = parse_json_response(response.data);
      if (result) {
        AI_DEBUG("JSON parsing successful, result length: %zu", strlen(result));
      } else {
        AI_DEBUG("ERROR: JSON parsing failed");
      }
    } else {
      log("AI Service: HTTP error %ld", http_code);
      AI_DEBUG("HTTP error response: %.200s%s", response.data ? response.data : "(null)",
               response.data && strlen(response.data) > 200 ? "..." : "");
    }
  } else {
    log("AI Service: CURL error: %s", curl_easy_strerror(res));
    AI_DEBUG("CURL error details: code=%d, message=%s", res, curl_easy_strerror(res));
  }
  
  /* Cleanup */
  AI_DEBUG("Cleaning up CURL resources");
  curl_easy_cleanup(curl);
  AI_DEBUG("  CURL handle cleaned up");
  curl_slist_free_all(headers);
  AI_DEBUG("  Headers freed");
  
  if (api_key) {
    AI_DEBUG("  Freeing API key");
    free(api_key);
  }
  if (response.data) {
    AI_DEBUG("  Freeing response data (%zu bytes)", response.size);
    free(response.data);
  }
  
  AI_DEBUG("make_api_request_single() returning %s", result ? "success" : "NULL");
  return result;
}

/**
 * Make an API request to OpenAI with retries (blocking version)
 */
static char *make_api_request(const char *prompt) {
  char *result = NULL;
  int retry_count = 0;
  
  AI_DEBUG("make_api_request() called with max retries: %d", AI_MAX_RETRIES);
  
  while (retry_count < AI_MAX_RETRIES) {
    AI_DEBUG("Attempt %d/%d", retry_count + 1, AI_MAX_RETRIES);
    result = make_api_request_single(prompt);
    if (result) {
      AI_DEBUG("Request succeeded on attempt %d", retry_count + 1);
      return result;
    }
    
    retry_count++;
    if (retry_count < AI_MAX_RETRIES) {
      /* Simple exponential backoff using sleep */
      int sleep_time = 1 << retry_count;
      AI_DEBUG("Request failed, sleeping for %d seconds before retry", sleep_time);
      sleep(sleep_time);
    }
  }
  
  AI_DEBUG("All %d attempts failed, returning NULL", AI_MAX_RETRIES);
  return NULL;
}

/**
 * Generate an AI response for a given prompt
 */
char *ai_generate_response(const char *prompt, int request_type) {
  char sanitized_prompt[MAX_STRING_LENGTH];
  char *response;
  
  AI_DEBUG("ai_generate_response() called with request_type=%d", request_type);
  AI_DEBUG("Original prompt: '%.100s%s'", prompt ? prompt : "(null)",
           prompt && strlen(prompt) > 100 ? "..." : "");
  
  /* Validate input */
  if (!prompt) {
    log("SYSERR: ai_generate_response called with NULL prompt");
    AI_DEBUG("ERROR: NULL prompt provided");
    return NULL;
  }
  
  if (!is_ai_enabled()) {
    AI_DEBUG("AI service disabled, generating fallback response");
    return generate_fallback_response(prompt);
  }
  
  /* Sanitize input */
  AI_DEBUG("Sanitizing input prompt");
  strlcpy(sanitized_prompt, sanitize_ai_input(prompt), sizeof(sanitized_prompt));
  AI_DEBUG("Sanitized prompt: '%.100s%s'", sanitized_prompt,
           strlen(sanitized_prompt) > 100 ? "..." : "");
  
  /* Make API request */
  AI_DEBUG("Making API request");
  response = make_api_request(sanitized_prompt);
  
  if (!response) {
    log("AI request failed for type %d", request_type);
    AI_DEBUG("API request failed, generating fallback response");
    return generate_fallback_response(prompt);
  }
  
  AI_DEBUG("Response received (length=%zu)", strlen(response));
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
  
  /* Build cache key - limit input length to prevent overflow */
  if (strlen(input) > 200) {
    snprintf(cache_key, sizeof(cache_key), "npc_%d_%.200s", 
             GET_MOB_VNUM(npc), input);
  } else {
    snprintf(cache_key, sizeof(cache_key), "npc_%d_%s", 
             GET_MOB_VNUM(npc), input);
  }
  
  /* Check cache */
  cached_response = ai_cache_get(cache_key);
  if (cached_response) {
    char *result = strdup(cached_response);
    if (!result) {
      log("SYSERR: Failed to allocate memory for cached AI response");
    }
    return result;
  }
  
  /* Build prompt */
  snprintf(prompt, sizeof(prompt),
    "You are %s in a fantasy RPG world. "
    "Respond to the player's message in character. "
    "Keep response under 100 words. "
    "Player says: \"%s\"",
    GET_NAME(npc) ? GET_NAME(npc) : "someone", input);
  
  /* Get AI response */
  response = ai_generate_response(prompt, AI_REQUEST_NPC_DIALOGUE);
  
  /* Cache if successful */
  if (response) {
    ai_cache_response(cache_key, response);
  }
  
  return response;
}

/**
 * Async NPC dialogue - returns immediately, response delivered via event
 */
void ai_npc_dialogue_async(struct char_data *npc, struct char_data *ch, const char *input) {
  char prompt[MAX_STRING_LENGTH];
  char cache_key[256];
  char *cached_response;
  char *response;
  
  if (!npc || !ch || !input || !*input) return;
  
  /* Build cache key - limit input length to prevent overflow */
  if (strlen(input) > 200) {
    snprintf(cache_key, sizeof(cache_key), "npc_%d_%.200s", 
             GET_MOB_VNUM(npc), input);
  } else {
    snprintf(cache_key, sizeof(cache_key), "npc_%d_%s", 
             GET_MOB_VNUM(npc), input);
  }
  
  /* Check cache */
  cached_response = ai_cache_get(cache_key);
  if (cached_response) {
    queue_ai_response(ch, npc, cached_response);
    return;
  }
  
  /* Build prompt */
  snprintf(prompt, sizeof(prompt),
    "You are %s in a fantasy RPG world. "
    "Respond to the player's message in character. "
    "Keep response under 100 words. "
    "Player says: \"%s\"",
    GET_NAME(npc) ? GET_NAME(npc) : "someone", input);
  
  /* Try to get immediate response */
  response = ai_generate_response_async(prompt, AI_REQUEST_NPC_DIALOGUE, 0);
  
  if (response) {
    /* Got immediate response, queue it */
    queue_ai_response(ch, npc, response);
    free(response);
  } else {
    /* Need to retry, queue retry event */
    queue_ai_request_retry(prompt, AI_REQUEST_NPC_DIALOGUE, 0, ch, npc);
  }
}

/**
 * Check rate limiting
 */
bool ai_check_rate_limit(void) {
  time_t now = time(NULL);
  
  AI_DEBUG("Checking rate limits at time %ld", (long)now);
  
  if (!ai_state.limiter) {
    AI_DEBUG("ERROR: Rate limiter not initialized");
    return FALSE;
  }
  
  AI_DEBUG("Current counts: minute=%d/%d, hour=%d/%d",
           ai_state.limiter->current_minute_count, ai_state.limiter->requests_per_minute,
           ai_state.limiter->current_hour_count, ai_state.limiter->requests_per_hour);
  
  /* Reset counters if needed */
  if (now >= ai_state.limiter->minute_reset) {
    AI_DEBUG("Minute window expired, resetting minute counter");
    ai_state.limiter->current_minute_count = 0;
    ai_state.limiter->minute_reset = now + 60;
    AI_DEBUG("  New minute reset time: %ld", (long)ai_state.limiter->minute_reset);
  }
  
  if (now >= ai_state.limiter->hour_reset) {
    AI_DEBUG("Hour window expired, resetting hour counter");
    ai_state.limiter->current_hour_count = 0;
    ai_state.limiter->hour_reset = now + 3600;
    AI_DEBUG("  New hour reset time: %ld", (long)ai_state.limiter->hour_reset);
  }
  
  /* Check limits */
  if (ai_state.limiter->current_minute_count >= ai_state.limiter->requests_per_minute ||
      ai_state.limiter->current_hour_count >= ai_state.limiter->requests_per_hour) {
    AI_DEBUG("Rate limit exceeded: minute=%s, hour=%s",
             ai_state.limiter->current_minute_count >= ai_state.limiter->requests_per_minute ? "YES" : "NO",
             ai_state.limiter->current_hour_count >= ai_state.limiter->requests_per_hour ? "YES" : "NO");
    return FALSE;
  }
  
  /* Increment counters */
  ai_state.limiter->current_minute_count++;
  ai_state.limiter->current_hour_count++;
  AI_DEBUG("Rate limit check passed, new counts: minute=%d, hour=%d",
           ai_state.limiter->current_minute_count, ai_state.limiter->current_hour_count);
  
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
  
  AI_DEBUG("build_json_request() called");
  AI_DEBUG("Input prompt length: %zu", prompt ? strlen(prompt) : 0);
  
  /* Validate input */
  if (!prompt) {
    log("SYSERR: build_json_request called with NULL prompt");
    AI_DEBUG("ERROR: NULL prompt provided");
    return NULL;
  }
  
  /* Simple JSON escaping */
  AI_DEBUG("Escaping JSON special characters");
  src = prompt;
  dest = escaped_prompt;
  int escape_count = 0;
  while (*src && dest - escaped_prompt < MAX_STRING_LENGTH - 3) {
    if (*src == '"' || *src == '\\') {
      if (dest - escaped_prompt >= MAX_STRING_LENGTH - 2) {
        AI_DEBUG("WARNING: Truncating at position %ld - no room for escape sequence",
                 (long)(dest - escaped_prompt));
        break; /* No room for escape sequence */
      }
      *dest++ = '\\';
      escape_count++;
    }
    if (dest - escaped_prompt >= MAX_STRING_LENGTH - 1) {
      AI_DEBUG("WARNING: Truncating at position %ld - buffer full",
               (long)(dest - escaped_prompt));
      break; /* No room for character */
    }
    *dest++ = *src++;
  }
  *dest = '\0';
  AI_DEBUG("Escaping complete: %d characters escaped, final length: %zu",
           escape_count, strlen(escaped_prompt))
  
  /* Build JSON request */
  AI_DEBUG("Building JSON request structure");
  AI_DEBUG("  Model: %s", ai_state.config->model);
  AI_DEBUG("  Max tokens: %d", ai_state.config->max_tokens);
  AI_DEBUG("  Temperature: %.2f", ai_state.config->temperature);
  
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
  
  AI_DEBUG("JSON request built, total length: %zu", strlen(json_buffer));
  AI_DEBUG("JSON structure: %.200s%s", json_buffer,
           strlen(json_buffer) > 200 ? "..." : "");
  
  return json_buffer;
}

/**
 * Parse JSON response from OpenAI API
 * Note: This is a simplified version. Full implementation would use json-c library
 */
static char *parse_json_response(const char *json_str) {
  char *content_start, *content_end;
  char *result;
  size_t len;
  
  AI_DEBUG("parse_json_response() called");
  AI_DEBUG("JSON input length: %zu", json_str ? strlen(json_str) : 0);
  
  /* Validate input */
  if (!json_str) {
    log("SYSERR: parse_json_response called with NULL json_str");
    AI_DEBUG("ERROR: NULL json_str provided");
    return NULL;
  }
  
  AI_DEBUG("Searching for content field in JSON");
  
  /* Find "content": " */
  content_start = strstr(json_str, "\"content\":");
  if (!content_start) {
    AI_DEBUG("ERROR: 'content' field not found in JSON");
    return NULL;
  }
  AI_DEBUG("Found 'content' field at position %ld", (long)(content_start - json_str));
  
  content_start = strchr(content_start, '"');
  if (!content_start) {
    AI_DEBUG("ERROR: Opening quote not found after 'content:'");
    return NULL;
  }
  content_start++; /* Skip opening quote */
  
  content_start = strchr(content_start, '"');
  if (!content_start) {
    AI_DEBUG("ERROR: Value quote not found");
    return NULL;
  }
  content_start++; /* Skip opening quote of value */
  AI_DEBUG("Content value starts at position %ld", (long)(content_start - json_str));
  
  /* Find closing quote, handling escaped quotes */
  AI_DEBUG("Searching for closing quote with escape handling");
  content_end = content_start;
  int char_count = 0;
  while (*content_end) {
    if (*content_end == '"') {
      /* Count consecutive backslashes before this quote */
      int backslash_count = 0;
      char *p = content_end - 1;
      while (p >= content_start && *p == '\\') {
        backslash_count++;
        p--;
      }
      AI_DEBUG("  Found quote at position %d with %d preceding backslashes",
               char_count, backslash_count);
      /* If even number of backslashes (or zero), quote is not escaped */
      if (backslash_count % 2 == 0) {
        AI_DEBUG("  This is the closing quote");
        break;
      }
    }
    content_end++;
    char_count++;
  }
  
  if (!*content_end) {
    AI_DEBUG("ERROR: Closing quote not found");
    return NULL;
  }
  
  /* Extract content and handle escape sequences */
  len = content_end - content_start;
  AI_DEBUG("Extracting content of length %zu", len);
  
  CREATE(result, char, len + 1);
  if (!result) {
    log("SYSERR: Failed to allocate memory for JSON response parsing");
    AI_DEBUG("ERROR: Failed to allocate %zu bytes", len + 1);
    return NULL;
  }
  
  /* Copy while unescaping */
  AI_DEBUG("Unescaping content");
  char *src = content_start;
  char *dst = result;
  int escape_sequences = 0;
  while (src < content_end) {
    if (*src == '\\' && src + 1 < content_end) {
      src++; /* Skip backslash */
      escape_sequences++;
      if (*src == '"') *dst++ = '"';
      else if (*src == '\\') *dst++ = '\\';
      else if (*src == 'n') *dst++ = '\n';
      else if (*src == 'r') *dst++ = '\r';
      else if (*src == 't') *dst++ = '\t';
      else {
        /* Unknown escape, copy as-is */
        AI_DEBUG("  Unknown escape sequence: \\%c", *src);
        *dst++ = '\\';
        *dst++ = *src;
      }
      src++;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
  
  AI_DEBUG("Content extracted successfully: %d escape sequences processed", escape_sequences);
  AI_DEBUG("Final content length: %zu", strlen(result));
  AI_DEBUG("Content preview: %.100s%s", result, strlen(result) > 100 ? "..." : "");
  
  return result;
}

/**
 * Log AI errors
 */
void log_ai_error(const char *function, const char *error) {
  if (!function) function = "Unknown";
  if (!error) error = "Unknown error";
  log("SYSERR: AI Service [%s]: %s", function, error);
}

/**
 * Log AI interactions for debugging/monitoring
 */
void log_ai_interaction(struct char_data *ch, struct char_data *npc, const char *response) {
  /* This would log to database in production */
  if (ch && npc && response) {
    log("AI: %s -> %s: %s", 
        GET_NAME(ch) ? GET_NAME(ch) : "Unknown",
        GET_NAME(npc) ? GET_NAME(npc) : "Unknown", 
        response);
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
  choice = rand_number(0, num_responses - 1);
  
  char *result = strdup(fallback_responses[choice]);
  if (!result) {
    log("SYSERR: Failed to allocate memory for fallback response");
  }
  return result;
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

/**
 * Generate an AI response asynchronously (non-blocking)
 * This version attempts once and returns NULL on failure
 * The caller should use the event system to retry if needed
 */
char *ai_generate_response_async(const char *prompt, int request_type, int retry_count) {
  char sanitized_prompt[MAX_STRING_LENGTH];
  char *response;
  
  /* Validate input */
  if (!prompt) {
    log("SYSERR: ai_generate_response_async called with NULL prompt");
    return NULL;
  }
  
  if (!is_ai_enabled()) {
    return generate_fallback_response(prompt);
  }
  
  /* Sanitize input */
  strlcpy(sanitized_prompt, sanitize_ai_input(prompt), sizeof(sanitized_prompt));
  
  /* Check cache first */
  response = ai_cache_get(sanitized_prompt);
  if (response) {
    log("AI Service: Cache hit for request type %d", request_type);
    char *result = strdup(response);
    if (!result) {
      log("SYSERR: Failed to allocate memory for cached AI response");
    }
    return result;
  }
  
  /* Make single request attempt */
  response = make_api_request_single(sanitized_prompt);
  
  /* Cache successful response */
  if (response) {
    ai_cache_response(sanitized_prompt, response);
    log("AI Service: Request type %d completed successfully", request_type);
  } else if (retry_count < AI_MAX_RETRIES) {
    /* Log that we need to retry */
    log("AI Service: Request type %d failed, retry %d/%d pending", 
        request_type, retry_count + 1, AI_MAX_RETRIES);
  } else {
    /* Final failure */
    log("AI Service: Request type %d failed after %d retries", request_type, AI_MAX_RETRIES);
  }
  
  return response;
}