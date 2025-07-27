# OpenAI Technical Implementation Guide for LuminariMUD

**Status: IMPLEMENTED** - January 27, 2025

## Overview
This document provides the technical implementation details for integrating OpenAI capabilities into the LuminariMUD codebase. The implementation has been completed with C90/C89 compliance and is fully functional.

## Core Implementation Files

### 1. AI Service Module (ai_service.c/h)

```c
/* ai_service.h */
#ifndef AI_SERVICE_H
#define AI_SERVICE_H

#include "structs.h"
#include "utils.h"
#include <curl/curl.h>

/* Configuration */
#define OPENAI_API_ENDPOINT "https://api.openai.com/v1/chat/completions"
#define AI_CACHE_EXPIRE_TIME 3600  /* 1 hour default */
#define AI_MAX_RETRIES 3
#define AI_TIMEOUT_MS 5000
#define AI_MAX_TOKENS 500

/* AI Service State */
struct ai_service_state {
  bool initialized;
  CURL *curl_handle;
  struct ai_config *config;
  struct ai_cache *cache;
  struct rate_limiter *limiter;
};

/* AI Configuration */
struct ai_config {
  char encrypted_api_key[256];
  char model[32];              /* gpt-4.1-mini, gpt-4o-mini, gpt-4.1 */
  int max_tokens;
  float temperature;
  int timeout_ms;
  bool content_filter_enabled;
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

/* Function Prototypes */
void init_ai_service(void);
void shutdown_ai_service(void);
char *ai_generate_response(const char *prompt, int request_type);
char *ai_npc_dialogue(struct char_data *npc, struct char_data *ch, const char *input);
char *ai_generate_room_desc(int room_vnum, int sector_type);
bool ai_moderate_content(const char *text);
void ai_cache_response(const char *key, const char *response);
char *ai_cache_get(const char *key);
bool ai_check_rate_limit(void);

/* Internal Functions */
static char *make_api_request(const char *prompt);
static char *build_json_request(const char *prompt);
static char *parse_json_response(const char *json_str);
static void log_ai_error(const char *function, const char *error);

#endif /* AI_SERVICE_H */
```

```c
/* ai_service.c */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "ai_service.h"
#include <curl/curl.h>
#include <json-c/json.h>

/* Global AI Service State */
static struct ai_service_state ai_state = {0};

/* CURL Write Callback */
struct curl_response {
  char *data;
  size_t size;
};

static size_t curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct curl_response *mem = (struct curl_response *)userp;
  
  char *ptr = realloc(mem->data, mem->size + realsize + 1);
  if (!ptr) return 0;
  
  mem->data = ptr;
  memcpy(&(mem->data[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->data[mem->size] = 0;
  
  return realsize;
}

/* Initialize AI Service */
void init_ai_service(void) {
  /* Initialize CURL */
  curl_global_init(CURL_GLOBAL_ALL);
  
  /* Allocate structures */
  CREATE(ai_state.config, struct ai_config, 1);
  CREATE(ai_state.limiter, struct rate_limiter, 1);
  
  /* Load configuration from database */
  load_ai_config();
  
  /* Initialize rate limiter */
  ai_state.limiter->requests_per_minute = 60;
  ai_state.limiter->requests_per_hour = 1000;
  ai_state.limiter->minute_reset = time(NULL) + 60;
  ai_state.limiter->hour_reset = time(NULL) + 3600;
  
  ai_state.initialized = TRUE;
  log("AI Service initialized successfully.");
}

/* Make API Request */
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
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, ai_state.config->timeout_ms);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    /* Execute request */
    res = curl_easy_perform(curl);
    
    if (res == CURLE_OK) {
      long http_code = 0;
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
      /* Exponential backoff */
      usleep((1 << retry_count) * 100000);
    }
  }
  
  /* Cleanup */
  secure_memset(api_key, 0, strlen(api_key));
  free(api_key);
  free(json_request);
  if (response.data) free(response.data);
  
  return result;
}

/* NPC Dialogue Handler */
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

/* Rate Limiting Check */
bool ai_check_rate_limit(void) {
  time_t now = time(NULL);
  
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
```

### 2. Integration Points

#### act.comm.c Modifications
```c
/* Modified do_tell for AI NPCs */
ACMD(do_tell) {
  struct char_data *vict;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  
  half_chop(argument, buf, buf2);
  
  if (!*buf || !*buf2) {
    send_to_char(ch, "Who do you wish to tell what??\r\n");
    return;
  }
  
  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, NOPERSON);
    return;
  }
  
  /* AI Enhancement for NPCs */
  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_AI_ENABLED)) {
    char *ai_response = ai_npc_dialogue(vict, ch, buf2);
    if (ai_response) {
      /* Send player's message to NPC */
      snprintf(buf, sizeof(buf), "$n tells you, '%s'", buf2);
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      
      /* Send to player that they told the NPC */
      send_to_char(ch, "You tell %s, '%s'\r\n", GET_NAME(vict), buf2);
      
      /* Queue AI response with slight delay for realism */
      CREATE(event_obj, struct ai_response_event, 1);
      event_obj->ch = ch;
      event_obj->npc = vict;
      event_obj->response = ai_response;
      event_create(ai_response_event, event_obj, 1 RL_SEC);
      return;
    }
  }
  
  /* Standard tell handling continues... */
}
```

#### Database Schema
```sql
-- AI Configuration
CREATE TABLE IF NOT EXISTS ai_config (
  id INT PRIMARY KEY AUTO_INCREMENT,
  config_key VARCHAR(50) UNIQUE NOT NULL,
  config_value TEXT,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- AI Request Log
CREATE TABLE IF NOT EXISTS ai_requests (
  id INT PRIMARY KEY AUTO_INCREMENT,
  request_type ENUM('npc_dialogue', 'room_desc', 'quest_gen', 'moderation'),
  prompt TEXT,
  response TEXT,
  tokens_used INT,
  response_time_ms INT,
  player_id INT,
  npc_vnum INT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_created (created_at),
  INDEX idx_player (player_id)
);

-- AI Cache
CREATE TABLE IF NOT EXISTS ai_cache (
  cache_key VARCHAR(255) PRIMARY KEY,
  response TEXT,
  expires_at TIMESTAMP,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_expires (expires_at)
);

-- NPC AI Personalities  
CREATE TABLE IF NOT EXISTS ai_npc_personalities (
  mob_vnum INT PRIMARY KEY,
  personality JSON,
  enabled BOOLEAN DEFAULT TRUE,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

### 3. Security Implementation

```c
/* ai_security.c */
#include "ai_service.h"
#include <openssl/evp.h>
#include <openssl/rand.h>

#define AES_KEY_SIZE 32
#define AES_IV_SIZE 16

/* Encrypt API Key */
int encrypt_api_key(const char *plaintext, char *encrypted_out) {
  unsigned char key[AES_KEY_SIZE];
  unsigned char iv[AES_IV_SIZE];
  EVP_CIPHER_CTX *ctx;
  int len, ciphertext_len;
  
  /* Generate key from server seed */
  derive_key_from_seed(key);
  RAND_bytes(iv, AES_IV_SIZE);
  
  /* Create and initialize context */
  if (!(ctx = EVP_CIPHER_CTX_new())) return 0;
  
  /* Initialize encryption */
  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return 0;
  }
  
  /* Encrypt the data */
  if (EVP_EncryptUpdate(ctx, encrypted_out + AES_IV_SIZE, &len, 
                        plaintext, strlen(plaintext)) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return 0;
  }
  ciphertext_len = len;
  
  /* Finalize encryption */
  if (EVP_EncryptFinal_ex(ctx, encrypted_out + AES_IV_SIZE + len, &len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return 0;
  }
  ciphertext_len += len;
  
  /* Prepend IV to ciphertext */
  memcpy(encrypted_out, iv, AES_IV_SIZE);
  
  EVP_CIPHER_CTX_free(ctx);
  return AES_IV_SIZE + ciphertext_len;
}

/* Decrypt API Key */
char *decrypt_api_key(const char *encrypted) {
  /* Implementation similar to encrypt but in reverse */
  /* Returns allocated string that must be freed */
}

/* Input Sanitization */
char *sanitize_ai_input(const char *input) {
  static char cleaned[MAX_STRING_LENGTH];
  char *dest = cleaned;
  const char *src = input;
  int len = 0;
  
  while (*src && len < MAX_STRING_LENGTH - 1) {
    /* Remove control characters */
    if (*src >= 32 && *src < 127) {
      /* Escape special characters */
      if (*src == '"' || *src == '\\') {
        if (len < MAX_STRING_LENGTH - 2) {
          *dest++ = '\\';
          len++;
        }
      }
      *dest++ = *src;
      len++;
    }
    src++;
  }
  
  *dest = '\0';
  return cleaned;
}
```

### 4. Performance Optimization

```c
/* ai_cache.c */
#include "ai_service.h"

static struct ai_cache_entry *cache_head = NULL;
static int cache_size = 0;
#define MAX_CACHE_SIZE 1000

/* Add to Cache */
void ai_cache_response(const char *key, const char *response) {
  struct ai_cache_entry *entry, *old;
  time_t now = time(NULL);
  
  /* Check if exists and update */
  for (entry = cache_head; entry; entry = entry->next) {
    if (!strcmp(entry->key, key)) {
      free(entry->response);
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
  entry->next = cache_head;
  cache_head = entry;
  cache_size++;
  
  /* Enforce cache size limit */
  if (cache_size > MAX_CACHE_SIZE) {
    ai_cache_cleanup();
  }
}

/* Get from Cache */
char *ai_cache_get(const char *key) {
  struct ai_cache_entry *entry;
  time_t now = time(NULL);
  
  for (entry = cache_head; entry; entry = entry->next) {
    if (!strcmp(entry->key, key)) {
      if (entry->expires_at > now) {
        return entry->response;
      }
      break;
    }
  }
  
  return NULL;
}

/* Cleanup Expired Entries */
void ai_cache_cleanup(void) {
  struct ai_cache_entry *entry, *next, *prev = NULL;
  time_t now = time(NULL);
  
  for (entry = cache_head; entry; entry = next) {
    next = entry->next;
    
    if (entry->expires_at <= now) {
      /* Remove expired entry */
      if (prev) {
        prev->next = next;
      } else {
        cache_head = next;
      }
      
      free(entry->key);
      free(entry->response);
      free(entry);
      cache_size--;
    } else {
      prev = entry;
    }
  }
}
```

### 5. Event System Integration

```c
/* ai_events.c */
#include "ai_service.h"
#include "dg_event.h"

/* AI Response Event Data */
struct ai_response_event {
  struct char_data *ch;
  struct char_data *npc;
  char *response;
};

/* AI Response Event Handler */
EVENTFUNC(ai_response_event) {
  struct ai_response_event *data = (struct ai_response_event *)event_obj;
  
  if (!data->ch || !data->npc || !data->response) {
    if (data->response) free(data->response);
    free(data);
    return 0;
  }
  
  /* Check both characters still exist and are in same room */
  if (IN_ROOM(data->ch) == IN_ROOM(data->npc)) {
    /* Send the AI response */
    snprintf(buf, sizeof(buf), "$n tells you, '%s'", data->response);
    act(buf, FALSE, data->npc, 0, data->ch, TO_VICT);
    
    /* Log the interaction */
    log_ai_interaction(data->ch, data->npc, data->response);
  }
  
  /* Cleanup */
  free(data->response);
  free(data);
  return 0;
}
```

### 6. Makefile Integration

```makefile
# Add to CFLAGS
CFLAGS = -g -O2 -Wall $(MYFLAGS) $(CPPFLAGS) -I/usr/include/json-c -lcurl -lssl -lcrypto -ljson-c

# Add to OBJFILES
OBJFILES = ... ai_service.o ai_cache.o ai_security.o ai_events.o ...

# Add compilation rules
ai_service.o: ai_service.c ai_service.h structs.h utils.h
	$(CC) -c $(CFLAGS) ai_service.c

ai_cache.o: ai_cache.c ai_service.h
	$(CC) -c $(CFLAGS) ai_cache.c

ai_security.o: ai_security.c ai_service.h
	$(CC) -c $(CFLAGS) ai_security.c

ai_events.o: ai_events.c ai_service.h dg_event.h
	$(CC) -c $(CFLAGS) ai_events.c
```

### 7. Configuration File Integration

```c
/* In config.c - add AI configuration options */
void load_config(void) {
  /* Existing config loading... */
  
  /* AI Service Configuration */
  if (!strcasecmp(tag, "ai_enabled")) {
    ai_config.enabled = strcasecmp(line, "YES") == 0;
  } else if (!strcasecmp(tag, "ai_model")) {
    strncpy(ai_config.model, line, sizeof(ai_config.model) - 1);
  } else if (!strcasecmp(tag, "ai_max_tokens")) {
    ai_config.max_tokens = atoi(line);
  } else if (!strcasecmp(tag, "ai_temperature")) {
    ai_config.temperature = atof(line);
  } else if (!strcasecmp(tag, "ai_api_key_file")) {
    load_encrypted_api_key(line);
  }
}
```

### 8. Command Implementation

```c
/* In act.wizard.c - AI management commands */
ACMD(do_ai) {
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  
  two_arguments(argument, arg1, arg2);
  
  if (!*arg1) {
    send_to_char(ch, "AI Service Status:\r\n");
    send_to_char(ch, "  Enabled: %s\r\n", ai_state.initialized ? "YES" : "NO");
    send_to_char(ch, "  Model: %s\r\n", ai_state.config->model);
    send_to_char(ch, "  Cache Size: %d entries\r\n", get_cache_size());
    send_to_char(ch, "  Requests (minute/hour): %d/%d\r\n",
                 ai_state.limiter->current_minute_count,
                 ai_state.limiter->current_hour_count);
    return;
  }
  
  if (!strcasecmp(arg1, "enable")) {
    init_ai_service();
    send_to_char(ch, "AI Service enabled.\r\n");
  } else if (!strcasecmp(arg1, "disable")) {
    shutdown_ai_service();
    send_to_char(ch, "AI Service disabled.\r\n");
  } else if (!strcasecmp(arg1, "cache")) {
    if (!strcasecmp(arg2, "clear")) {
      ai_cache_clear();
      send_to_char(ch, "AI cache cleared.\r\n");
    } else {
      send_to_char(ch, "Usage: ai cache clear\r\n");
    }
  } else if (!strcasecmp(arg1, "test")) {
    char *response = ai_generate_response("Hello, how are you?", AI_REQUEST_TEST);
    if (response) {
      send_to_char(ch, "AI Response: %s\r\n", response);
      free(response);
    } else {
      send_to_char(ch, "AI test failed.\r\n");
    }
  }
}
```

### 9. Mob Flag Implementation

```c
/* In structs.h - add to mob flags */
#define MOB_AI_ENABLED    (1 << 25)  /* Mob uses AI for responses */

/* In constants.c - add to action_bits[] */
"AI_ENABLED",

/* In medit.c - add to mob editing */
case MEDIT_AI_ENABLED:
  if (YESNO(OLC_MOB(d), MOB_AI_ENABLED)) {
    send_to_char(d->character, "AI responses enabled.\r\n");
  } else {
    send_to_char(d->character, "AI responses disabled.\r\n");
  }
  break;
```

### 10. JSON Building/Parsing

```c
/* JSON Request Builder */
static char *build_json_request(const char *prompt) {
  json_object *root, *messages, *message;
  char *result;
  
  root = json_object_new_object();
  json_object_object_add(root, "model", 
                        json_object_new_string(ai_state.config->model));
  
  messages = json_object_new_array();
  message = json_object_new_object();
  json_object_object_add(message, "role", json_object_new_string("user"));
  json_object_object_add(message, "content", json_object_new_string(prompt));
  json_object_array_add(messages, message);
  
  json_object_object_add(root, "messages", messages);
  json_object_object_add(root, "max_tokens", 
                        json_object_new_int(ai_state.config->max_tokens));
  json_object_object_add(root, "temperature", 
                        json_object_new_double(ai_state.config->temperature));
  
  result = strdup(json_object_to_json_string(root));
  json_object_put(root);
  
  return result;
}

/* JSON Response Parser */
static char *parse_json_response(const char *json_str) {
  json_object *root, *choices, *choice, *message, *content;
  const char *text;
  char *result = NULL;
  
  root = json_tokener_parse(json_str);
  if (!root) return NULL;
  
  if (json_object_object_get_ex(root, "choices", &choices) &&
      json_object_array_length(choices) > 0) {
    choice = json_object_array_get_idx(choices, 0);
    if (json_object_object_get_ex(choice, "message", &message) &&
        json_object_object_get_ex(message, "content", &content)) {
      text = json_object_get_string(content);
      if (text) {
        result = strdup(text);
      }
    }
  }
  
  json_object_put(root);
  return result;
}
```

## Build Dependencies

```bash
# Install required libraries
sudo apt-get install libcurl4-openssl-dev libjson-c-dev libssl-dev

# Or for CentOS/RHEL
sudo yum install libcurl-devel json-c-devel openssl-devel
```

## Testing Implementation

```c
/* test_ai_service.c */
#include "CuTest.h"
#include "ai_service.h"

void test_ai_cache(CuTest *tc) {
  char *response;
  
  /* Test cache miss */
  response = ai_cache_get("test_key");
  CuAssertPtrEquals(tc, NULL, response);
  
  /* Test cache set */
  ai_cache_response("test_key", "test_response");
  
  /* Test cache hit */
  response = ai_cache_get("test_key");
  CuAssertStrEquals(tc, "test_response", response);
}

void test_rate_limiting(CuTest *tc) {
  int i;
  bool result;
  
  /* Reset rate limiter */
  ai_state.limiter->current_minute_count = 0;
  
  /* Test within limits */
  for (i = 0; i < 60; i++) {
    result = ai_check_rate_limit();
    CuAssertTrue(tc, result);
  }
  
  /* Test exceeding limit */
  result = ai_check_rate_limit();
  CuAssertTrue(tc, !result);
}
```

## Performance Monitoring

```c
/* Add to perfmon tracking */
PERFMON_TIMER_START(ai_request);
char *response = make_api_request(prompt);
PERFMON_TIMER_STOP(ai_request);

if (PERFMON_ELAPSED_MS(ai_request) > 1000) {
  log("PERFORMANCE: AI request took %ld ms", PERFMON_ELAPSED_MS(ai_request));
}
```

## Error Handling Patterns

```c
/* Comprehensive error handling */
char *ai_safe_request(const char *prompt) {
  char *response = NULL;
  
  /* Validate input */
  if (!prompt || !*prompt) {
    log("SYSERR: ai_safe_request called with invalid prompt");
    return NULL;
  }
  
  /* Check service availability */
  if (!ai_state.initialized) {
    log("AI Service not available - using fallback");
    return generate_fallback_response(prompt);
  }
  
  /* Try cache first */
  response = ai_cache_get(prompt);
  if (response) return strdup(response);
  
  /* Make request with error handling */
  response = make_api_request(prompt);
  if (!response) {
    log("AI request failed - using fallback");
    return generate_fallback_response(prompt);
  }
  
  return response;
}
```

## Deployment Checklist

- [x] Install required libraries (curl, json-c, openssl)
- [x] Add AI configuration to .env file
- [x] Create database tables
- [x] Set up encrypted API key file
- [x] Configure rate limits based on API plan
- [x] Enable AI flags on test NPCs
- [ ] Monitor initial performance metrics
- [ ] Set up error alerting
- [x] Document fallback behaviors
- [ ] Train builders on AI mob flags

## Implementation Notes (January 27, 2025)

### Actual Implementation Details

1. **File Structure**:
   - `ai_service.c/h` - Core service with CURL integration
   - `ai_cache.c` - In-memory caching system
   - `ai_security.c` - XOR encryption for API keys
   - `ai_events.c` - Event system integration
   - `dotenv.c/h` - .env file parser for configuration

2. **Configuration Location**:
   - Configuration stored in `lib/.env` (following MUD conventions)
   - Example provided in `.env.example`
   - API key and all settings loaded from environment variables

3. **C90 Compliance**:
   - All variables declared at beginning of blocks
   - No C99 features (no for-loop declarations, etc.)
   - C-style comments throughout
   - Proper bool typedef handling

4. **Integration Points**:
   - `act.comm.c` - Modified do_tell for AI responses
   - `act.wizard.c` - Added do_ai admin command
   - `interpreter.c` - Registered ai command
   - `structs.h` - Added MOB_AI_ENABLED flag (bit 98)
   - `constants.c` - Added "AI-Enabled" to action_bits

5. **Key Differences from Original Plan**:
   - Simplified JSON parsing (not using json-c fully)
   - XOR encryption instead of OpenSSL AES
   - .env configuration instead of database storage
   - dotenv.c parser added for configuration

6. **Current Model Support**:
   - Default: gpt-4.1-mini (fast, cost-effective)
   - Alternative: gpt-4o-mini ($0.15/1M input tokens)
   - Premium: gpt-4.1 (best quality, 1M context)

### Testing the Implementation

```bash
# 1. Set up configuration
cp .env.example lib/.env
echo "OPENAI_API_KEY=sk-proj-your-key-here" >> lib/.env

# 2. Compile
make clean && make

# 3. Run database migration
mysql -h host -u user -p database < ai_service_migration.sql

# 4. In-game testing
ai enable
ai reload
ai test

# 5. Test with NPC
# First, use medit to set AI_ENABLED flag on a mob
# Then: tell <mob_name> Hello!
```

### Known Limitations

1. **Security**: XOR encryption is basic - upgrade to AES for production
2. **JSON Parsing**: Simplified implementation - full json-c integration recommended
3. **Error Handling**: Basic error messages - enhance for production
4. **Async**: Responses are queued but not truly async

### Performance Considerations

- Cache hit rate critical for cost control
- Default 1-hour cache TTL
- Rate limiting prevents runaway costs
- Event-based delays create natural conversation flow

### Future Enhancements

1. Implement full json-c parsing
2. Add OpenSSL AES encryption
3. Create async request queue
4. Add database-backed cache option
5. Implement batch processing
6. Add content moderation
7. Create quest generation system