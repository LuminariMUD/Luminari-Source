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

/* Configuration defaults - these can be overridden in lib/.env
 * See lib/.env_example for full documentation of each setting
 */

/* API Endpoints (can override via OPENAI_API_ENDPOINT, OLLAMA_API_ENDPOINT in .env) */
#define DEFAULT_OPENAI_API_ENDPOINT "https://api.openai.com/v1/chat/completions"
#define DEFAULT_OLLAMA_API_ENDPOINT "http://localhost:11434/api/generate"

/* General settings (override via AI_* variables in .env) */
#define DEFAULT_AI_CACHE_EXPIRE_TIME 3600 /* 1 hour, override: AI_CACHE_EXPIRE_SECONDS */
#define DEFAULT_AI_MAX_RETRIES 3          /* override: AI_MAX_RETRIES */
#define DEFAULT_AI_TIMEOUT_MS 30000       /* 30 seconds, override: AI_TIMEOUT_MS */
#define DEFAULT_AI_MAX_TOKENS 500         /* override: AI_MAX_TOKENS */
#define DEFAULT_AI_MAX_CACHE_SIZE 5000    /* override: AI_MAX_CACHE_SIZE */
#define DEFAULT_AI_DEBUG_MODE 0           /* override: AI_DEBUG_MODE */

/* Ollama defaults (override via OLLAMA_* variables in .env) */
#define DEFAULT_OLLAMA_MODEL "llama3.2:1b" /* override: OLLAMA_MODEL */
#define DEFAULT_OLLAMA_TIMEOUT_MS 10000    /* 10 seconds, override: OLLAMA_TIMEOUT_MS */
#define DEFAULT_OLLAMA_MAX_TOKENS 100      /* override: OLLAMA_MAX_TOKENS */
#define DEFAULT_OLLAMA_TEMPERATURE 7       /* 0.7, override: OLLAMA_TEMPERATURE */
#define DEFAULT_OLLAMA_TOP_K 40            /* override: OLLAMA_TOP_K */
#define DEFAULT_OLLAMA_TOP_P 90            /* 0.9, override: OLLAMA_TOP_P */

/* Legacy compatibility defines - use these in code, they reference defaults */
#define OPENAI_API_ENDPOINT DEFAULT_OPENAI_API_ENDPOINT
#define AI_CACHE_EXPIRE_TIME DEFAULT_AI_CACHE_EXPIRE_TIME
#define AI_MAX_RETRIES DEFAULT_AI_MAX_RETRIES
#define AI_TIMEOUT_MS DEFAULT_AI_TIMEOUT_MS
#define AI_MAX_TOKENS DEFAULT_AI_MAX_TOKENS
#define AI_MAX_CACHE_SIZE DEFAULT_AI_MAX_CACHE_SIZE

/* Debug mode - runtime configurable via AI_DEBUG_MODE in .env
 * Note: Compile-time debug uses DEFAULT_AI_DEBUG_MODE
 * For runtime debug, check ai_state.config->debug_mode */
#define AI_DEBUG_MODE DEFAULT_AI_DEBUG_MODE

/* Debug logging macro */
#if AI_DEBUG_MODE
#define AI_DEBUG(fmt, ...)                                                                         \
  do                                                                                               \
  {                                                                                                \
    log("AI_DEBUG [%s:%d in %s()]: " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__);            \
  } while (0)
#else
#define AI_DEBUG(fmt, ...) /* Debug mode disabled */
#endif

/* Request types for logging and rate limiting */
enum ai_request_type
{
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
struct ai_service_state
{
  bool initialized;                  /* Service ready flag */
  CURL *curl_handle;                 /* Persistent connection pooling */
  struct ai_config *config;          /* Runtime configuration */
  struct ai_cache_entry *cache_head; /* Response cache list */
  int cache_size;                    /* Current cache entries */
  struct rate_limiter *limiter;      /* API rate limiting */
};

/* AI Configuration - loaded from lib/.env at startup */
struct ai_config
{
  /* OpenAI settings */
  char encrypted_api_key[256];
  char openai_endpoint[256]; /* OPENAI_API_ENDPOINT */
  char model[64];            /* AI_MODEL: gpt-4o-mini, etc */
  int max_tokens;            /* AI_MAX_TOKENS */
  float temperature;         /* AI_TEMPERATURE / 10 */
  int timeout_ms;            /* AI_TIMEOUT_MS */

  /* Ollama settings */
  char ollama_endpoint[256]; /* OLLAMA_API_ENDPOINT */
  char ollama_model[64];     /* OLLAMA_MODEL */
  int ollama_timeout_ms;     /* OLLAMA_TIMEOUT_MS */
  int ollama_max_tokens;     /* OLLAMA_MAX_TOKENS (num_predict) */
  float ollama_temperature;  /* OLLAMA_TEMPERATURE / 10 */
  int ollama_top_k;          /* OLLAMA_TOP_K */
  float ollama_top_p;        /* OLLAMA_TOP_P / 100 */

  /* General settings */
  int max_retries;             /* AI_MAX_RETRIES */
  int cache_expire_seconds;    /* AI_CACHE_EXPIRE_SECONDS */
  int max_cache_size;          /* AI_MAX_CACHE_SIZE */
  bool debug_mode;             /* AI_DEBUG_MODE */
  bool content_filter_enabled; /* AI_CONTENT_FILTER_ENABLED */
  bool enabled;                /* Runtime toggle via 'ai enable/disable' */
};

/* Cache Entry */
struct ai_cache_entry
{
  char *key;
  char *response;
  time_t expires_at;
  struct ai_cache_entry *next;
};

/* Rate Limiting */
struct rate_limiter
{
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
void init_ai_service(void);     /* Initialize at startup (comm.c) */
void shutdown_ai_service(void); /* Cleanup at shutdown */
void load_ai_config(void);      /* Reload from .env file */
bool is_ai_enabled(void);       /* Global enable check (all components) */

/* API Request Functions
 * MAIN FUNCTIONALITY - Called by game systems
 */
char *ai_generate_response(const char *prompt, int request_type); /* Generic AI request */
char *ai_npc_dialogue(struct char_data *npc, struct char_data *ch,
                      const char *input); /* BLOCKING (testing only) */
void ai_npc_dialogue_async(struct char_data *npc, struct char_data *ch,
                           const char *input); /* PRIMARY: Non-blocking NPC dialogue */
char *ai_generate_room_desc(int room_vnum, int sector_type); /* TODO: Not implemented */
bool ai_moderate_content(const char *text);                  /* TODO: Not implemented */

/* Cache Management
 * PERFORMANCE OPTIMIZATION - Reduces API calls
 * Implemented in ai_cache.c, called by ai_service.c
 */
void ai_cache_response(const char *key, const char *response); /* Store response */
char *ai_cache_get(const char *key); /* Retrieve (returns ptr, don't free) */
void ai_cache_clear(void);           /* Admin command: clear all */
void ai_cache_cleanup(void);         /* Remove expired/excess entries */
int get_cache_size(void);            /* Current cache size */

/* Rate Limiting */
bool ai_check_rate_limit(void);
void ai_reset_rate_limits(void);

/* Security Functions (defined in ai_security.c)
 * CRITICAL SECURITY - All API keys and user input pass through here
 */
char *decrypt_api_key(const char *encrypted); /* Returns allocated string (caller frees) */
int encrypt_api_key(const char *plaintext, char *encrypted_out); /* Store API key */
void load_encrypted_api_key(const char *filename);               /* Load from file (unused) */
char *sanitize_ai_input(const char *input);           /* CRITICAL: Prevent prompt injection */
void secure_memset(void *ptr, int value, size_t num); /* Clear sensitive memory */

/* Utility Functions */
void log_ai_error(const char *function, const char *error);
void log_ai_interaction(struct char_data *ch, struct char_data *npc, const char *response,
                        const char *backend, bool from_cache);
char *generate_fallback_response(const char *prompt);

/* Event Functions (defined in ai_events.c)
 * ASYNC DELIVERY - Thread-safe response handling
 */
void queue_ai_response(struct char_data *ch, struct char_data *npc, const char *response,
                       const char *backend, bool from_cache); /* Queue response for delivery */
void queue_ai_request_retry(const char *prompt, int request_type,
                            int retry_count, /* Retry with backoff */
                            struct char_data *ch, struct char_data *npc);

/* Async API Functions
 * INTERNAL USE - Called by retry system
 */
char *ai_generate_response_async(const char *prompt, int request_type,
                                 int retry_count); /* Single attempt, no retry */

#endif /* AI_SERVICE_H */
