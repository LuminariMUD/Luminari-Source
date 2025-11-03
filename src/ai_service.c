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
 * COMPONENT INTERACTIONS:
 * - DEPENDS ON: ai_security.c for input sanitization and API key handling
 * - DEPENDS ON: ai_cache.c for response caching to reduce API calls
 * - DEPENDS ON: ai_events.c for async response delivery
 * - USED BY: act.comm.c (do_tell command) when NPCs have AI_ENABLED flag
 * 
 * THREADING MODEL:
 * - Main thread: Handles immediate cache hits and fallback responses
 * - Worker threads: Created for API calls to prevent blocking
 * - Event system: Delivers responses after minimal delay
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
#include <pthread.h>

/* Global AI Service State
 * This global state is shared across all AI components:
 * - ai_security.c accesses config for API key storage
 * - ai_cache.c directly manipulates cache_head and cache_size
 * - ai_events.c validates character pointers against this state
 * - All components check initialized flag before operations
 */
struct ai_service_state ai_state = {0};

/* Thread request structure
 * Used for communication between main thread and worker threads.
 * Created in ai_npc_dialogue_async() and processed in ai_thread_worker().
 * Character pointers must be validated before AND after thread execution
 * as characters may be freed during async operation.
 */
struct ai_thread_request {
  char *prompt;        /* Sanitized prompt to send to API */
  char *cache_key;     /* Key for storing response in cache */
  struct char_data *ch;   /* Player character (must validate) */
  struct char_data *npc;  /* NPC character (must validate) */
  int request_type;    /* AI_REQUEST_* constant for logging */
  pthread_t thread_id; /* Thread ID for debugging */
  char backend[32];    /* Track which backend was used */
};

/* CURL Response Buffer */
struct curl_response {
  char *data;
  size_t size;
};

/* Function prototypes for static functions */
static size_t ai_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp);
static char *make_api_request(const char *prompt);
static char *make_api_request_single(const char *prompt);
static char *build_json_request(const char *prompt);
static char *parse_json_response(const char *json_str);
static char *make_ollama_request(const char *prompt);
static char *build_ollama_json_request(const char *prompt);
static char *parse_ollama_json_response(const char *json_str);
static void warmup_ollama_model(void);
static void *ai_thread_worker(void *arg);
static int json_escape_string(char *dest, size_t dest_size, const char *src);
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
 * 
 * Called from comm.c during MUD startup. Sets up all AI subsystems:
 * 1. Initializes CURL library for HTTP requests
 * 2. Allocates configuration structure with defaults
 * 3. Sets up rate limiter to prevent API abuse
 * 4. Initializes cache system (empty at start)
 * 5. Creates persistent CURL handle for connection pooling
 * 6. Loads configuration from .env file
 * 
 * IMPORTANT: This must be called before any other AI functions.
 * Other components (ai_cache.c, ai_security.c, ai_events.c) rely on
 * the initialized state and allocated structures.
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
  AI_DEBUG("CURL library initialized successfully");
  
  /* Allocate configuration structure */
  AI_DEBUG("Allocating AI configuration structure (size=%zu)", sizeof(struct ai_config));
  CREATE(ai_state.config, struct ai_config, 1);
  if (!ai_state.config) {
    log("SYSERR: Failed to allocate AI configuration structure");
    AI_DEBUG("ERROR: Failed to allocate %zu bytes for ai_config", sizeof(struct ai_config));
    curl_global_cleanup();
    return;
  }
  AI_DEBUG("AI configuration allocated at %p", (void*)ai_state.config);
  
  /* Set default configuration */
  AI_DEBUG("Setting default configuration values");
  strcpy(ai_state.config->model, "gpt-4o-mini");   /* Latest fast model */
  AI_DEBUG("  Model: %s", ai_state.config->model);
  ai_state.config->max_tokens = AI_MAX_TOKENS;
  AI_DEBUG("  Max tokens: %d", ai_state.config->max_tokens);
  ai_state.config->temperature = 0.3;  /* Lower temperature for faster responses */
  AI_DEBUG("  Temperature: %.2f", ai_state.config->temperature);
  ai_state.config->timeout_ms = AI_TIMEOUT_MS;
  AI_DEBUG("  Timeout: %d ms", ai_state.config->timeout_ms);
  ai_state.config->content_filter_enabled = TRUE;
  AI_DEBUG("  Content filter: %s", ai_state.config->content_filter_enabled ? "ENABLED" : "DISABLED");
  ai_state.config->enabled = FALSE;  /* Disabled by default */
  AI_DEBUG("  Service enabled: %s", ai_state.config->enabled ? "YES" : "NO");
  
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
  AI_DEBUG("Rate limiter allocated at %p", (void*)ai_state.limiter);
  
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
           ai_state.limiter->current_minute_count, ai_state.limiter->current_hour_count);
  
  /* Initialize cache */
  AI_DEBUG("Initializing AI response cache");
  ai_state.cache_head = NULL;
  ai_state.cache_size = 0;
  AI_DEBUG("  Cache initialized: head=%p, size=%d, max_size=%d", 
           (void*)ai_state.cache_head, ai_state.cache_size, AI_MAX_CACHE_SIZE);
  
  /* Initialize persistent CURL handle for connection pooling */
  AI_DEBUG("Initializing persistent CURL handle");
  ai_state.curl_handle = curl_easy_init();
  if (ai_state.curl_handle) {
    AI_DEBUG("  Persistent CURL handle created successfully");
  } else {
    AI_DEBUG("  WARNING: Failed to create persistent CURL handle");
  }
  
  /* Load configuration from database/files */
  AI_DEBUG("Loading AI configuration from .env file");
  load_ai_config();
  
  /* Warmup Ollama model to avoid first-request delay */
  warmup_ollama_model();
  
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
  for (entry = ai_state.cache_head; entry; entry = next) {
    next = entry->next;
    AI_DEBUG("  Freeing cache entry: key='%s', response_len=%zu", 
             entry->key ? entry->key : "(null)", 
             entry->response ? strlen(entry->response) : 0);
    if (entry->key) free(entry->key);
    if (entry->response) free(entry->response);
    free(entry);
  }
  AI_DEBUG("Cache entries freed");
  
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
  
  /* Cleanup persistent CURL handle */
  if (ai_state.curl_handle) {
    AI_DEBUG("Cleaning up persistent CURL handle");
    curl_easy_cleanup(ai_state.curl_handle);
    ai_state.curl_handle = NULL;
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
 * 
 * Central check used by all AI components and act.comm.c to determine
 * if AI features should be active. Returns true only if:
 * 1. Service is initialized (init_ai_service was called)
 * 2. Configuration is allocated
 * 3. Service is explicitly enabled (via 'ai enable' command)
 * 
 * This prevents any AI operations when service is disabled or
 * improperly configured, providing a single point of control.
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
 * 
 * Loads configuration from .env file via dotenv.c. This function:
 * 1. Retrieves API key and calls ai_security.c encrypt_api_key()
 * 2. Sets model, temperature, timeout, and other parameters
 * 3. Configures rate limiting thresholds
 * 
 * Called by:
 * - init_ai_service() during startup
 * - 'ai reload' admin command to refresh settings
 * 
 * Interacts with:
 * - dotenv.c: get_env_value() for reading .env file
 * - ai_security.c: encrypt_api_key() for secure storage
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
           get_env_value("AI_CONTENT_FILTER_ENABLED") ? "set" : "default");
  
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
 * 
 * Low-level function that makes a single API request attempt.
 * 
 * Flow:
 * 1. Check rate limits via ai_check_rate_limit()
 * 2. Decrypt API key via ai_security.c decrypt_api_key()
 * 3. Build JSON request with sanitized prompt
 * 4. Execute CURL request (uses persistent handle if available)
 * 5. Parse JSON response and return content
 * 
 * Rate limiting: Updates limiter counters on each request
 * Connection pooling: Reuses ai_state.curl_handle when possible
 * 
 * Called by: make_api_request() which adds retry logic
 * Returns: Allocated response string or NULL on failure
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
  AI_DEBUG("Rate limit check passed");
  
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
           strlen(json_request) > 200 ? "..." : "");
  
  AI_DEBUG("Getting CURL handle");
  /* Try to use persistent handle for connection pooling */
  if (ai_state.curl_handle) {
    AI_DEBUG("Using persistent CURL handle for connection pooling");
    curl = ai_state.curl_handle;
    /* Reset the handle to clear previous request data */
    curl_easy_reset(curl);
  } else {
    AI_DEBUG("Creating new CURL handle (no persistent handle available)");
    curl = curl_easy_init();
    if (!curl) {
      log("SYSERR: Failed to initialize CURL handle in make_api_request_single");
      AI_DEBUG("ERROR: curl_easy_init() returned NULL");
      free(api_key);
      return NULL;
    }
  }
  AI_DEBUG("CURL handle ready at %p", (void*)curl);
  
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
  AI_DEBUG("  Content-Type header added successfully");
  
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
  AI_DEBUG("  SSL verification enabled (peer=1, host=2)");
  
  /* Performance optimizations */
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
  /* HTTP/2 support - only if available in this CURL version */
#ifdef CURL_HTTP_VERSION_2_0
  curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
  AI_DEBUG("  HTTP/2 and TCP keep-alive enabled");
#else
  AI_DEBUG("  TCP keep-alive enabled (HTTP/2 not available in this CURL version)");
#endif
  
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
  /* Only cleanup the handle if it's not the persistent one */
  if (curl != ai_state.curl_handle) {
    curl_easy_cleanup(curl);
    AI_DEBUG("  CURL handle cleaned up (non-persistent)");
  } else {
    AI_DEBUG("  Keeping persistent CURL handle for reuse");
  }
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
 * 
 * This is the high-level request function that handles retries.
 * It calls make_api_request_single() up to AI_MAX_RETRIES times
 * with exponential backoff between attempts.
 * 
 * Used by:
 * - ai_generate_response() for synchronous requests
 * - ai_thread_worker() for async requests in worker threads
 * 
 * Returns: Allocated string with response (caller must free) or NULL
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
  
  AI_DEBUG("All %d OpenAI attempts failed, trying Ollama fallback", AI_MAX_RETRIES);
  
  /* Try Ollama as a fallback when OpenAI fails */
  result = make_ollama_request(prompt);
  if (result) {
    log("AI Service: OpenAI failed, using Ollama fallback");
    return result;
  }
  
  AI_DEBUG("Both OpenAI and Ollama failed");
  return NULL;
}

/**
 * Generate an AI response for a given prompt
 * 
 * High-level function that generates AI responses with fallback.
 * 
 * Flow:
 * 1. Validate input and check if service enabled
 * 2. Sanitize prompt via ai_security.c sanitize_ai_input()
 * 3. Make API request via make_api_request()
 * 4. Return fallback response if request fails
 * 
 * This is the main entry point for synchronous AI requests.
 * For async requests, use ai_generate_response_async() instead.
 * 
 * Called by:
 * - ai_npc_dialogue() for NPC conversations
 * - Future: quest generation, room descriptions, etc.
 * 
 * Returns: Allocated response string (never NULL)
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
 * Generate AI dialogue for an NPC (BLOCKING VERSION)
 * 
 * Synchronous version that blocks until response is received.
 * Flow:
 * 1. Build cache key from NPC vnum and input
 * 2. Check cache via ai_cache_get() for existing response
 * 3. If miss, build prompt and call ai_generate_response()
 * 4. Store successful response in cache via ai_cache_response()
 * 
 * NOTE: This function blocks the game loop! Only use for testing.
 * Production code should use ai_npc_dialogue_async() instead.
 * 
 * Called by: act.comm.c when AI_ENABLED NPCs receive tells
 * Returns: Allocated response string (caller must free) or NULL
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
      return NULL;
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
 * 
 * NON-BLOCKING version that creates a worker thread for API calls.
 * This is the primary function used in production.
 * 
 * Flow:
 * 1. Check cache first (immediate response if hit)
 * 2. If cache miss, create ai_thread_request structure
 * 3. Spawn detached pthread with ai_thread_worker()
 * 4. Worker thread makes blocking API call
 * 5. Response delivered via ai_events.c queue_ai_response()
 * 
 * Thread safety: Character pointers are validated in the event
 * handler since characters may be freed during async operation.
 * 
 * Called by: act.comm.c for AI-enabled NPC interactions
 */
void ai_npc_dialogue_async(struct char_data *npc, struct char_data *ch, const char *input) {
  char prompt[MAX_STRING_LENGTH];
  char cache_key[256];
  char *cached_response;
  
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
    queue_ai_response(ch, npc, cached_response, "Cache", TRUE);
    return;
  }
  
  /* Build prompt */
  snprintf(prompt, sizeof(prompt),
    "You are %s in a fantasy RPG world. "
    "Respond to the player's message in character. "
    "Keep response under 100 words. "
    "Player says: \"%s\"",
    GET_NAME(npc) ? GET_NAME(npc) : "someone", input);
  
  /* Create thread request */
  struct ai_thread_request *req;
  CREATE(req, struct ai_thread_request, 1);
  if (!req) {
    log("SYSERR: Failed to allocate thread request");
    return;
  }
  
  req->prompt = strdup(prompt);
  if (!req->prompt) {
    log("SYSERR: Failed to allocate prompt copy");
    free(req);
    return;
  }
  
  req->cache_key = strdup(cache_key);
  if (!req->cache_key) {
    log("SYSERR: Failed to allocate cache key copy");
    free(req->prompt);
    free(req);
    return;
  }
  
  req->ch = ch;
  req->npc = npc;
  req->request_type = AI_REQUEST_NPC_DIALOGUE;
  
  /* Create detached thread to handle the request */
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  
  if (pthread_create(&req->thread_id, &attr, ai_thread_worker, req) != 0) {
    log("SYSERR: Failed to create AI thread");
    /* Fall back to event system */
    free(req->prompt);
    free(req->cache_key);
    free(req);
    queue_ai_request_retry(prompt, AI_REQUEST_NPC_DIALOGUE, 0, ch, npc);
  } else {
    AI_DEBUG("Created thread for AI request");
  }
  
  pthread_attr_destroy(&attr);
}

/**
 * Check rate limiting
 * 
 * Enforces per-minute and per-hour request limits to prevent
 * API abuse and control costs. Uses sliding window approach:
 * - Minute window: 60 requests (default)
 * - Hour window: 1000 requests (default)
 * 
 * Called by:
 * - make_api_request_single() before each API request
 * - Admin commands to display current usage
 * 
 * Interacts with: ai_state.limiter for shared counter state
 * Thread-safe: No (assumes single-threaded rate check)
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
  int escape_result;
  
  AI_DEBUG("build_json_request() called");
  AI_DEBUG("Input prompt length: %zu", prompt ? strlen(prompt) : 0);
  
  /* Validate input */
  if (!prompt) {
    log("SYSERR: build_json_request called with NULL prompt");
    AI_DEBUG("ERROR: NULL prompt provided");
    return NULL;
  }
  
  /* Use proper JSON escaping function */
  AI_DEBUG("Escaping JSON special characters");
  escape_result = json_escape_string(escaped_prompt, sizeof(escaped_prompt), prompt);
  if (escape_result < 0) {
    log("SYSERR: Failed to escape prompt for JSON - string too long");
    AI_DEBUG("ERROR: json_escape_string failed - prompt too long");
    return NULL;
  }
  AI_DEBUG("Escaping complete: final length: %d", escape_result);
  
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
  
  /* Return a dynamically allocated copy instead of static buffer */
  char *result = strdup(json_buffer);
  if (!result) {
    log("SYSERR: Failed to allocate memory for JSON request");
    AI_DEBUG("ERROR: strdup failed for JSON request");
  }
  
  return result;
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
    AI_DEBUG("JSON snippet: %.200s", json_str);
    return NULL;
  }
  AI_DEBUG("Found 'content' field at position %ld", (long)(content_start - json_str));
  AI_DEBUG("Context around content field: %.100s", content_start);
  
  /* Skip past "content": and any whitespace to find the opening quote of the value */
  content_start += strlen("\"content\":");
  AI_DEBUG("After skipping 'content:' label, position %ld", (long)(content_start - json_str));
  AI_DEBUG("Character at position: '%c' (ASCII %d)", *content_start, (int)*content_start);
  
  int whitespace_count = 0;
  while (*content_start && (*content_start == ' ' || *content_start == '\t' || 
         *content_start == '\n' || *content_start == '\r')) {
    whitespace_count++;
    content_start++;
  }
  AI_DEBUG("Skipped %d whitespace characters", whitespace_count);
  
  if (*content_start != '"') {
    AI_DEBUG("ERROR: Opening quote of content value not found");
    AI_DEBUG("Expected '\"' but found '%c' (ASCII %d)", *content_start, (int)*content_start);
    AI_DEBUG("Context: %.50s", content_start);
    return NULL;
  }
  content_start++; /* Skip opening quote of value */
  AI_DEBUG("Content value starts at position %ld", (long)(content_start - json_str));
  AI_DEBUG("First 50 chars of content: %.50s", content_start);
  
  /* Find closing quote, handling escaped quotes */
  AI_DEBUG("Searching for closing quote with escape handling");
  content_end = content_start;
  int char_count = 0;
  int quotes_found = 0;
  while (*content_end) {
    if (*content_end == '"') {
      quotes_found++;
      /* Count consecutive backslashes before this quote */
      int backslash_count = 0;
      char *p = content_end - 1;
      while (p >= content_start && *p == '\\') {
        backslash_count++;
        p--;
      }
      AI_DEBUG("  Found quote #%d at position %d with %d preceding backslashes",
               quotes_found, char_count, backslash_count);
      /* If even number of backslashes (or zero), quote is not escaped */
      if (backslash_count % 2 == 0) {
        AI_DEBUG("  This is the closing quote (unescaped)");
        break;
      } else {
        AI_DEBUG("  This quote is escaped, continuing search");
      }
    }
    content_end++;
    char_count++;
    if (char_count > 10000) {
      AI_DEBUG("ERROR: Content too long or malformed (>10000 chars)");
      AI_DEBUG("Current position context: %.50s", content_end);
      return NULL;
    }
  }
  
  if (!*content_end) {
    AI_DEBUG("ERROR: Closing quote not found after %d characters", char_count);
    AI_DEBUG("Last 50 chars searched: %.50s", content_end - MIN(50, char_count));
    return NULL;
  }
  
  AI_DEBUG("Found closing quote at position %d", char_count);
  
  /* Extract content and handle escape sequences */
  len = content_end - content_start;
  AI_DEBUG("Extracting content of length %zu", len);
  AI_DEBUG("Raw content before unescaping: %.100s%s", content_start, len > 100 ? "..." : "");
  
  CREATE(result, char, len + 1);
  if (!result) {
    log("SYSERR: Failed to allocate memory for JSON response parsing");
    AI_DEBUG("ERROR: Failed to allocate %zu bytes", len + 1);
    return NULL;
  }
  AI_DEBUG("Allocated %zu bytes for result at %p", len + 1, result);
  
  /* Copy while unescaping */
  AI_DEBUG("Starting content unescaping process");
  char *src = content_start;
  char *dst = result;
  int escape_sequences = 0;
  int chars_processed = 0;
  while (src < content_end) {
    chars_processed++;
    if (*src == '\\' && src + 1 < content_end) {
      src++; /* Skip backslash */
      escape_sequences++;
      AI_DEBUG("  Escape sequence #%d at position %d: \\%c", 
               escape_sequences, chars_processed, *src);
      if (*src == '"') *dst++ = '"';
      else if (*src == '\\') *dst++ = '\\';
      else if (*src == 'n') *dst++ = '\n';
      else if (*src == 'r') *dst++ = '\r';
      else if (*src == 't') *dst++ = '\t';
      else {
        /* Unknown escape, copy as-is */
        AI_DEBUG("  Unknown escape sequence: \\%c (ASCII %d)", *src, (int)*src);
        *dst++ = '\\';
        *dst++ = *src;
      }
      src++;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
  
  AI_DEBUG("Unescaping complete: processed %d chars, found %d escape sequences", 
           chars_processed, escape_sequences);
  AI_DEBUG("Final content length: %zu", strlen(result));
  AI_DEBUG("Content preview: %.100s%s", result, strlen(result) > 100 ? "..." : "");
  AI_DEBUG("Full content: %s", result);
  
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
void log_ai_interaction(struct char_data *ch, struct char_data *npc, const char *response, const char *backend, bool from_cache) {
  /* This would log to database in production */
  if (ch && npc && response) {
    log("AI [%s%s]: %s -> %s: %s", 
        backend ? backend : "Unknown",
        from_cache ? "/CACHED" : "",
        GET_NAME(ch) ? GET_NAME(ch) : "Unknown",
        GET_NAME(npc) ? GET_NAME(npc) : "Unknown", 
        response);
  }
}

/**
 * Warmup Ollama model by making a test request
 * 
 * This preloads the model into memory to avoid the startup delay
 * on the first real player request. Called during server startup.
 */
static void warmup_ollama_model(void) {
  char *test_response;
  CURL *curl;
  CURLcode res;
  long http_code = 0;
  
  log("AI Service: Warming up Ollama model (%s) at %s...", OLLAMA_MODEL, OLLAMA_API_ENDPOINT);
  AI_DEBUG("Starting Ollama warmup with test prompt");
  
  /* First check if Ollama service is even reachable */
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/tags");
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);  /* HEAD request */
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000L);  /* Quick 1 second timeout */
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
      if (res == CURLE_COULDNT_CONNECT) {
        log("AI Service: ERROR - Cannot connect to Ollama at localhost:11434 (is Ollama running?)");
        log("AI Service: Run 'systemctl status ollama' or 'ollama serve' to start Ollama");
      } else if (res == CURLE_OPERATION_TIMEDOUT) {
        log("AI Service: ERROR - Ollama connection timed out (service may be overloaded)");
      } else {
        log("AI Service: ERROR - Ollama connectivity check failed: %s", curl_easy_strerror(res));
      }
      curl_easy_cleanup(curl);
      return;
    }
    
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    
    if (http_code != 200 && http_code != 0) {
      log("AI Service: WARNING - Ollama service returned unexpected status: %ld", http_code);
    }
  }
  
  /* Make a simple test request to load the model into memory */
  test_response = make_ollama_request("Hello");
  
  if (test_response) {
    log("AI Service: Ollama model %s warmed up successfully (response length: %zu)", 
        OLLAMA_MODEL, strlen(test_response));
    AI_DEBUG("Ollama warmup successful, response: %.100s%s", 
             test_response, strlen(test_response) > 100 ? "..." : "");
    free(test_response);
  } else {
    log("AI Service: ERROR - Ollama model %s warmup failed", OLLAMA_MODEL);
    log("AI Service: Possible issues:");
    log("  - Model not installed: run 'ollama pull %s'", OLLAMA_MODEL);
    log("  - Service not running: run 'systemctl start ollama' or 'ollama serve'");
    log("  - Model loading timeout: try a smaller model or increase timeout");
    AI_DEBUG("Ollama warmup failed - model may not be loaded");
  }
}

/**
 * Escape a string for JSON encoding
 * 
 * Escapes special characters that have meaning in JSON strings.
 * Handles quotes, backslashes, control characters, etc.
 * 
 * @param dest Destination buffer for escaped string
 * @param dest_size Size of destination buffer
 * @param src Source string to escape
 * @return Length of escaped string, or -1 on error
 */
static int json_escape_string(char *dest, size_t dest_size, const char *src) {
  const char *s;
  char *d;
  size_t space_left;
  
  if (!dest || !src || dest_size < 1) {
    return -1;
  }
  
  d = dest;
  space_left = dest_size - 1; /* Leave room for null terminator */
  
  for (s = src; *s && space_left > 0; s++) {
    switch (*s) {
      case '"':
        if (space_left < 2) goto truncated;
        *d++ = '\\';
        *d++ = '"';
        space_left -= 2;
        break;
      case '\\':
        if (space_left < 2) goto truncated;
        *d++ = '\\';
        *d++ = '\\';
        space_left -= 2;
        break;
      case '\b':
        if (space_left < 2) goto truncated;
        *d++ = '\\';
        *d++ = 'b';
        space_left -= 2;
        break;
      case '\f':
        if (space_left < 2) goto truncated;
        *d++ = '\\';
        *d++ = 'f';
        space_left -= 2;
        break;
      case '\n':
        if (space_left < 2) goto truncated;
        *d++ = '\\';
        *d++ = 'n';
        space_left -= 2;
        break;
      case '\r':
        if (space_left < 2) goto truncated;
        *d++ = '\\';
        *d++ = 'r';
        space_left -= 2;
        break;
      case '\t':
        if (space_left < 2) goto truncated;
        *d++ = '\\';
        *d++ = 't';
        space_left -= 2;
        break;
      default:
        if ((unsigned char)*s < 0x20) {
          /* Other control characters - skip them */
          continue;
        } else if ((unsigned char)*s >= 0x80) {
          /* Non-ASCII characters - skip for safety in ANSI C */
          continue;
        } else {
          /* Normal printable ASCII character */
          *d++ = *s;
          space_left--;
        }
        break;
    }
  }
  
  *d = '\0';
  return (int)(d - dest);
  
truncated:
  *d = '\0';
  return -1; /* String was truncated */
}

/**
 * Build JSON request for Ollama API
 * 
 * Creates a JSON request formatted for Ollama's generate endpoint.
 * Ollama uses a simpler format than OpenAI.
 * 
 * Returns: Allocated JSON string or NULL on failure
 */
static char *build_ollama_json_request(const char *prompt) {
  char json_buffer[MAX_STRING_LENGTH * 2];
  char escaped_prompt[MAX_STRING_LENGTH];
  char system_prefix[] = "You are an NPC in a fantasy RPG game. ";
  char system_suffix[] = " Respond in character briefly:";
  int written;
  int escape_result;
  
  AI_DEBUG("Building Ollama JSON request for prompt: '%.50s%s'",
           prompt, strlen(prompt) > 50 ? "..." : "");
  
  /* Escape the prompt for JSON */
  escape_result = json_escape_string(escaped_prompt, sizeof(escaped_prompt), prompt);
  if (escape_result < 0) {
    log("SYSERR: Failed to escape prompt for Ollama JSON - string too long");
    return NULL;
  }
  
  /* Build the JSON request manually with proper escaping already done */
  written = snprintf(json_buffer, sizeof(json_buffer),
    "{"
      "\"model\":\"%s\","
      "\"prompt\":\"%s%s%s\","
      "\"stream\":false,"
      "\"options\":{"
        "\"num_predict\":100,"
        "\"temperature\":0.7,"
        "\"top_k\":40,"
        "\"top_p\":0.9"
      "}"
    "}",
    OLLAMA_MODEL,
    system_prefix,
    escaped_prompt,
    system_suffix
  );
  
  if (written < 0 || written >= (int)sizeof(json_buffer)) {
    log("SYSERR: Ollama JSON request truncated or failed");
    return NULL;
  }
  
  char *result = strdup(json_buffer);
  if (!result) {
    log("SYSERR: Failed to allocate memory for Ollama JSON request");
  }
  
  AI_DEBUG("Ollama JSON request built: %.200s%s", json_buffer,
           strlen(json_buffer) > 200 ? "..." : "");
  
  return result;
}

/**
 * Parse Ollama JSON response
 * 
 * Extracts the 'response' field from Ollama's JSON response.
 * Ollama format: {"response":"text content","done":true,...}
 * 
 * Returns: Allocated response string or NULL on failure
 */
static char *parse_ollama_json_response(const char *json_str) {
  const char *response_start, *response_end;
  char *result;
  size_t response_len;
  int i, j;
  
  AI_DEBUG("Parsing Ollama JSON response (length=%zu)", strlen(json_str));
  
  /* Find "response":" in the JSON */
  response_start = strstr(json_str, "\"response\":\"");
  if (!response_start) {
    log("SYSERR: No 'response' field in Ollama JSON");
    return NULL;
  }
  
  response_start += 12; /* Skip past "response":" */
  
  /* Find the closing quote */
  response_end = response_start;
  while (*response_end) {
    if (*response_end == '"' && *(response_end - 1) != '\\') {
      break;
    }
    response_end++;
  }
  
  if (!*response_end) {
    log("SYSERR: Unterminated string in Ollama JSON response");
    return NULL;
  }
  
  response_len = response_end - response_start;
  result = malloc(response_len + 1);
  if (!result) {
    log("SYSERR: Failed to allocate memory for Ollama response");
    return NULL;
  }
  
  /* Copy and unescape the response */
  for (i = 0, j = 0; i < (int)response_len; i++) {
    if (response_start[i] == '\\' && i + 1 < (int)response_len) {
      switch (response_start[i + 1]) {
        case 'n': result[j++] = '\n'; i++; break;
        case 'r': result[j++] = '\r'; i++; break;
        case 't': result[j++] = '\t'; i++; break;
        case '"': result[j++] = '"'; i++; break;
        case '\\': result[j++] = '\\'; i++; break;
        default: result[j++] = response_start[i]; break;
      }
    } else {
      result[j++] = response_start[i];
    }
  }
  result[j] = '\0';
  
  AI_DEBUG("Ollama response parsed: '%.100s%s'", result,
           strlen(result) > 100 ? "..." : "");
  
  return result;
}

/**
 * Make an API request to Ollama (local LLM)
 * 
 * Makes a request to the local Ollama service as a fallback
 * when OpenAI is disabled or unavailable.
 * 
 * Flow:
 * 1. Build JSON request for Ollama format
 * 2. Execute CURL request to localhost:11434
 * 3. Parse JSON response
 * 
 * Returns: Allocated response string or NULL on failure
 */
static char *make_ollama_request(const char *prompt) {
  CURL *curl;
  CURLcode res;
  struct curl_response response = {0};
  struct curl_slist *headers = NULL;
  char *json_request;
  char *result = NULL;
  long http_code;
  
  AI_DEBUG("make_ollama_request() called with prompt: '%.50s%s'",
           prompt, strlen(prompt) > 50 ? "..." : "");
  
  /* Build JSON request */
  json_request = build_ollama_json_request(prompt);
  if (!json_request) {
    AI_DEBUG("ERROR: Failed to build Ollama JSON request");
    return NULL;
  }
  
  /* Initialize CURL */
  curl = curl_easy_init();
  if (!curl) {
    log("SYSERR: Failed to initialize CURL for Ollama request");
    free(json_request);
    return NULL;
  }
  
  /* Set headers (no auth needed for local Ollama) */
  headers = curl_slist_append(headers, "Content-Type: application/json");
  
  /* Configure CURL for Ollama */
  curl_easy_setopt(curl, CURLOPT_URL, OLLAMA_API_ENDPOINT);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_request);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ai_curl_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000L); /* 10 second timeout for local (model may need to load) */
  
  AI_DEBUG("Executing Ollama request to %s", OLLAMA_API_ENDPOINT);
  res = curl_easy_perform(curl);
  
  if (res == CURLE_OK) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    AI_DEBUG("Ollama HTTP response code: %ld", http_code);
    
    if (http_code == 200 && response.data) {
      result = parse_ollama_json_response(response.data);
      if (result) {
        AI_DEBUG("Ollama response received successfully");
      } else {
        log("AI Service: ERROR - Failed to parse Ollama JSON response");
        if (response.data) {
          AI_DEBUG("Raw response: %.200s%s", response.data, 
                   strlen(response.data) > 200 ? "..." : "");
        }
      }
    } else if (http_code == 404) {
      log("AI Service: ERROR - Ollama model %s not found (run: ollama pull %s)", 
          OLLAMA_MODEL, OLLAMA_MODEL);
    } else if (http_code == 500) {
      log("AI Service: ERROR - Ollama internal server error (check ollama logs)");
    } else if (http_code == 0) {
      log("AI Service: ERROR - No response from Ollama (service may not be running)");
    } else {
      log("AI Service: ERROR - Ollama HTTP error %ld", http_code);
      if (response.data) {
        AI_DEBUG("Error response: %.200s%s", response.data,
                 strlen(response.data) > 200 ? "..." : "");
      }
    }
  } else {
    if (res == CURLE_COULDNT_CONNECT) {
      log("AI Service: ERROR - Cannot connect to Ollama (is it running?)");
    } else if (res == CURLE_OPERATION_TIMEDOUT) {
      log("AI Service: ERROR - Ollama request timed out (model may be loading)");
    } else {
      log("AI Service: ERROR - Ollama CURL error: %s", curl_easy_strerror(res));
    }
    AI_DEBUG("Ollama CURL error code: %d", res);
  }
  
  /* Cleanup */
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  free(json_request);
  if (response.data) {
    free(response.data);
  }
  
  return result;
}

/**
 * Generate fallback response when AI is unavailable
 * 
 * Modified to try Ollama first before returning generic responses.
 * This provides AI-powered responses even when OpenAI is disabled.
 */
char *generate_fallback_response(const char *prompt) {
  char *ollama_response;
  static char *fallback_responses[] = {
    "I don't understand what you're saying.",
    "Could you repeat that?",
    "I'm not sure how to respond to that.",
    "Hmmmmm...",
    "...",
    NULL
  };
  int num_responses = 0;
  int choice;
  char **ptr;
  
  /* First, try to get response from Ollama (local LLM) */
  AI_DEBUG("Attempting Ollama fallback for prompt: '%.50s%s'",
           prompt, strlen(prompt) > 50 ? "..." : "");
  
  ollama_response = make_ollama_request(prompt);
  if (ollama_response) {
    log("AI Service: Using Ollama fallback response");
    return ollama_response;
  }
  
  /* If Ollama fails, use generic fallback responses */
  AI_DEBUG("Ollama unavailable, using generic fallback");
  
  /* Count responses */
  for (ptr = fallback_responses; *ptr; ptr++) {
    num_responses++;
  }
  
  /* Select random response */
  choice = rand_number(0, num_responses - 1);
  
  char *result = strdup(fallback_responses[choice]);
  if (!result) {
    log("SYSERR: Failed to allocate memory for fallback response");
    return NULL;
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
 * 
 * Used by ai_events.c ai_request_retry_event() for retry logic.
 * Unlike ai_generate_response(), this:
 * 1. Makes only ONE attempt (no internal retries)
 * 2. Returns NULL on failure (no fallback)
 * 3. Expects caller to handle retries via event system
 * 
 * Flow:
 * 1. Sanitize input via ai_security.c
 * 2. Check cache first via ai_cache_get()
 * 3. Make single API request attempt
 * 4. Cache successful responses
 * 
 * Returns: Response string or NULL (caller handles retries)
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
      return NULL;
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

/**
 * Thread worker function for async API calls
 * 
 * Runs in a detached pthread to make blocking API calls without
 * freezing the game. This function:
 * 1. Makes the blocking API request via make_api_request()
 * 2. Caches successful responses via ai_cache_response()
 * 3. Queues response for delivery via queue_ai_response()
 * 4. Cleans up all allocated memory
 * 
 * IMPORTANT: This runs in a separate thread! Character pointers
 * may become invalid during execution. The event system in
 * ai_events.c handles validation before delivery.
 * 
 * Thread lifecycle: Created detached, self-destructs on completion
 */
static void *ai_thread_worker(void *arg) {
  struct ai_thread_request *req = (struct ai_thread_request *)arg;
  char *response = NULL;
  char *ollama_response = NULL;
  
  AI_DEBUG("Thread worker started for request type %d", req->request_type);
  
  /* Make the blocking API call in this thread */
  /* Track which backend ultimately provides the response */
  if (is_ai_enabled()) {
    /* Try OpenAI first (with retries) */
    int retry_count = 0;
    while (retry_count < AI_MAX_RETRIES) {
      response = make_api_request_single(req->prompt);
      if (response) {
        strcpy(req->backend, "OpenAI");
        break;
      }
      retry_count++;
      if (retry_count < AI_MAX_RETRIES) {
        int sleep_time = 1 << retry_count;
        sleep(sleep_time);
      }
    }
    
    /* If OpenAI failed, try Ollama */
    if (!response) {
      ollama_response = make_ollama_request(req->prompt);
      if (ollama_response) {
        response = ollama_response;
        strcpy(req->backend, "Ollama");
        log("AI Service: OpenAI failed, using Ollama fallback");
      }
    }
  } else {
    /* AI disabled, try Ollama directly */
    ollama_response = make_ollama_request(req->prompt);
    if (ollama_response) {
      response = ollama_response;
      strcpy(req->backend, "Ollama");
      log("AI Service: Using Ollama (OpenAI disabled)");
    }
  }
  
  /* If all AI failed, get generic fallback (don't call generate_fallback_response as it tries Ollama again) */
  if (!response) {
    static char *fallback_responses[] = {
      "I don't understand what you're saying.",
      "Could you repeat that?",
      "I'm not sure how to respond to that.",
      "...",
      "I see.",
      NULL
    };
    int num_responses = 0;
    char **ptr;
    for (ptr = fallback_responses; *ptr; ptr++) {
      num_responses++;
    }
    int choice = rand_number(0, num_responses - 1);
    response = strdup(fallback_responses[choice]);
    strcpy(req->backend, "Fallback");
    AI_DEBUG("Using generic fallback response");
  }
  
  if (response) {
    /* Cache the response */
    if (req->cache_key) {
      ai_cache_response(req->cache_key, response);
    }
    
    /* Queue the response to be delivered in main thread */
    queue_ai_response(req->ch, req->npc, response, req->backend, FALSE);
    free(response);
    
    AI_DEBUG("Thread worker completed successfully with backend: %s", req->backend);
  } else {
    AI_DEBUG("Thread worker failed to get any response");
  }
  
  /* Cleanup */
  if (req->prompt) free(req->prompt);
  if (req->cache_key) free(req->cache_key);
  free(req);
  
  return NULL;
}