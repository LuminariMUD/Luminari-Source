/**
 * @file ai_service.h
 * @author Zusuk
 * @brief OpenAI API integration service for NPC dialogue and content generation
 * 
 * This module provides AI-powered features including:
 * - Dynamic NPC dialogue responses
 * - Procedural room description generation  
 * - Content moderation
 * - Quest/mission generation assistance
 * 
 * PUBLIC API INTERFACE:
 * This header defines the public interface between AI components
 * and the rest of the MUD. Components interact as follows:
 * 
 * INITIALIZATION:
 * - init_ai_service() - Called once from comm.c at startup
 * - shutdown_ai_service() - Called on MUD shutdown
 * 
 * CORE FUNCTIONALITY:
 * - ai_npc_dialogue_async() - Primary interface from act.comm.c
 * - is_ai_enabled() - Global enable check
 * 
 * ADMIN INTERFACE:
 * - load_ai_config() - Reload configuration
 * - ai_cache_clear() - Clear response cache
 * - ai_reset_rate_limits() - Reset API limits
 * 
 * Part of the LuminariMUD distribution.
 */

#ifndef AI_SERVICE_H
#define AI_SERVICE_H

#include "structs.h"
#include "utils.h"

/* Use existing bool from structs.h if available, otherwise define */
#ifndef bool
typedef char bool;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE  
#define FALSE 0
#endif

/* Forward declarations */
struct curl_slist;
typedef void CURL;

/* Configuration constants */
#define OPENAI_API_ENDPOINT "https://api.openai.com/v1/chat/completions"
#define AI_CACHE_EXPIRE_TIME 3600  /* 1 hour default */
#define AI_MAX_RETRIES 3
#define AI_TIMEOUT_MS 5000
#define AI_MAX_TOKENS 500
#define AI_MAX_CACHE_SIZE 5000  /* Increased for better performance */

/* Debug mode - set to 1 to enable verbose debug logging - set to 0 to disable */
#define AI_DEBUG_MODE 0

/* Debug logging macro */
#if AI_DEBUG_MODE
#define AI_DEBUG(fmt, ...) do { \
  log("AI_DEBUG [%s:%d in %s()]: " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
} while(0)
#else
#define AI_DEBUG(fmt, ...) /* Debug mode disabled */
#endif

/* Request types for logging and rate limiting */
enum ai_request_type {
  AI_REQUEST_TEST = 0,
  AI_REQUEST_NPC_DIALOGUE = 1,
  AI_REQUEST_ROOM_DESC = 2,
  AI_REQUEST_QUEST_GEN = 3,
  AI_REQUEST_MODERATION = 4
};

/* AI Service State
 * GLOBAL STATE shared across all AI components:
 * - ai_service.c: Owns and manages this state
 * - ai_cache.c: Directly accesses cache_head and cache_size
 * - ai_security.c: Accesses config for API key operations
 * - ai_events.c: Reads state for validation
 */
struct ai_service_state {
  bool initialized;            /* Service ready flag */
  CURL *curl_handle;          /* Persistent connection pooling */
  struct ai_config *config;    /* Runtime configuration */
  struct ai_cache_entry *cache_head;  /* Response cache list */
  int cache_size;             /* Current cache entries */
  struct rate_limiter *limiter;  /* API rate limiting */
};

/* AI Configuration */
struct ai_config {
  char encrypted_api_key[256];
  char model[32];              /* gpt-4, gpt-3.5-turbo */
  int max_tokens;
  float temperature;
  int timeout_ms;
  bool content_filter_enabled;
  bool enabled;
};

/* Cache Entry */
struct ai_cache_entry {
  char *key;
  char *response;
  time_t expires_at;
  struct ai_cache_entry *next;
};

/* Rate Limiting */
struct rate_limiter {
  int requests_per_minute;
  int current_minute_count;
  time_t minute_reset;
  int requests_per_hour;
  int current_hour_count;
  time_t hour_reset;
};

/* Global AI state - extern declaration */
extern struct ai_service_state ai_state;

/* Core Service Functions
 * PRIMARY INTERFACE - Called by main MUD systems
 */
void init_ai_service(void);      /* Initialize at startup (comm.c) */
void shutdown_ai_service(void);  /* Cleanup at shutdown */
void load_ai_config(void);       /* Reload from .env file */
bool is_ai_enabled(void);        /* Global enable check (all components) */

/* API Request Functions
 * MAIN FUNCTIONALITY - Called by game systems
 */
char *ai_generate_response(const char *prompt, int request_type);  /* Generic AI request */
char *ai_npc_dialogue(struct char_data *npc, struct char_data *ch, const char *input);  /* BLOCKING (testing only) */
void ai_npc_dialogue_async(struct char_data *npc, struct char_data *ch, const char *input);  /* PRIMARY: Non-blocking NPC dialogue */
char *ai_generate_room_desc(int room_vnum, int sector_type);  /* TODO: Not implemented */
bool ai_moderate_content(const char *text);  /* TODO: Not implemented */

/* Cache Management
 * PERFORMANCE OPTIMIZATION - Reduces API calls
 * Implemented in ai_cache.c, called by ai_service.c
 */
void ai_cache_response(const char *key, const char *response);  /* Store response */
char *ai_cache_get(const char *key);  /* Retrieve (returns ptr, don't free) */
void ai_cache_clear(void);  /* Admin command: clear all */
void ai_cache_cleanup(void);  /* Remove expired/excess entries */
int get_cache_size(void);  /* Current cache size */

/* Rate Limiting */
bool ai_check_rate_limit(void);
void ai_reset_rate_limits(void);

/* Security Functions (defined in ai_security.c)
 * CRITICAL SECURITY - All API keys and user input pass through here
 */
char *decrypt_api_key(const char *encrypted);  /* Returns allocated string (caller frees) */
int encrypt_api_key(const char *plaintext, char *encrypted_out);  /* Store API key */
void load_encrypted_api_key(const char *filename);  /* Load from file (unused) */
char *sanitize_ai_input(const char *input);  /* CRITICAL: Prevent prompt injection */
void secure_memset(void *ptr, int value, size_t num);  /* Clear sensitive memory */

/* Utility Functions */
void log_ai_error(const char *function, const char *error);
void log_ai_interaction(struct char_data *ch, struct char_data *npc, const char *response);
char *generate_fallback_response(const char *prompt);

/* Event Functions (defined in ai_events.c)
 * ASYNC DELIVERY - Thread-safe response handling
 */
void queue_ai_response(struct char_data *ch, struct char_data *npc, const char *response);  /* Queue response for delivery */
void queue_ai_request_retry(const char *prompt, int request_type, int retry_count,  /* Retry with backoff */
                           struct char_data *ch, struct char_data *npc);

/* Async API Functions
 * INTERNAL USE - Called by retry system
 */
char *ai_generate_response_async(const char *prompt, int request_type, int retry_count);  /* Single attempt, no retry */

#endif /* AI_SERVICE_H */