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

/* AI Service State */
struct ai_service_state {
  bool initialized;
  CURL *curl_handle;
  struct ai_config *config;
  struct ai_cache_entry *cache_head;
  int cache_size;
  struct rate_limiter *limiter;
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

/* Core Service Functions */
void init_ai_service(void);
void shutdown_ai_service(void);
void load_ai_config(void);
bool is_ai_enabled(void);

/* API Request Functions */
char *ai_generate_response(const char *prompt, int request_type);
char *ai_npc_dialogue(struct char_data *npc, struct char_data *ch, const char *input);
void ai_npc_dialogue_async(struct char_data *npc, struct char_data *ch, const char *input);
char *ai_generate_room_desc(int room_vnum, int sector_type);
bool ai_moderate_content(const char *text);

/* Cache Management */
void ai_cache_response(const char *key, const char *response);
char *ai_cache_get(const char *key);
void ai_cache_clear(void);
void ai_cache_cleanup(void);
int get_cache_size(void);

/* Rate Limiting */
bool ai_check_rate_limit(void);
void ai_reset_rate_limits(void);

/* Security Functions (defined in ai_security.c) */
char *decrypt_api_key(const char *encrypted);
int encrypt_api_key(const char *plaintext, char *encrypted_out);
void load_encrypted_api_key(const char *filename);
char *sanitize_ai_input(const char *input);
void secure_memset(void *ptr, int value, size_t num);

/* Utility Functions */
void log_ai_error(const char *function, const char *error);
void log_ai_interaction(struct char_data *ch, struct char_data *npc, const char *response);
char *generate_fallback_response(const char *prompt);

/* Event Functions (defined in ai_events.c) */
void queue_ai_response(struct char_data *ch, struct char_data *npc, const char *response);
void queue_ai_request_retry(const char *prompt, int request_type, int retry_count, 
                           struct char_data *ch, struct char_data *npc);

/* Async API Functions */
char *ai_generate_response_async(const char *prompt, int request_type, int retry_count);

#endif /* AI_SERVICE_H */