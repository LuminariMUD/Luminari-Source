# LuminariMUD AI Service Documentation

## Overview

The AI Service integrates OpenAI's GPT models into LuminariMUD, enabling dynamic NPC dialogue through natural language processing. NPCs with the AI_ENABLED flag can respond intelligently to player messages using context-aware responses.

**Current Status**: IMPLEMENTED (January 2025) - Core NPC dialogue functionality operational

## System Architecture & Component Interaction

### Component Overview
The AI system consists of four tightly integrated components that work together to provide seamless AI-powered NPC interactions:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           Player Interaction                             │
└─────────────────────────────────┬───────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                            ai_service.c                                  │
│  - Main API interface & request handling                                 │
│  - CURL-based HTTP client with connection pooling                       │
│  - Threading support for non-blocking operations                        │
│  - JSON request/response processing                                     │
│  - Rate limiting & retry logic                                          │
└────────────────┬──────────────────┬──────────────────┬─────────────────┘
                 │                  │                  │
     ┌───────────▼──────────┐ ┌────▼─────┐ ┌─────────▼─────────┐
     │    ai_security.c     │ │ai_cache.c│ │   ai_events.c     │
     │ - Input sanitization │ │ - LRU    │ │ - Event queuing   │
     │ - API key handling   │ │   cache  │ │ - Async delivery  │
     │ - Secure memory ops  │ │ - TTL    │ │ - Thread safety   │
     └──────────────────────┘ └──────────┘ └───────────────────┘
```

### Component Interactions

1. **Request Flow**:
   ```
   Player Tell → ai_service.c → ai_security.c (sanitize) → ai_cache.c (check)
                                                              ↓ (miss)
                                                          OpenAI API
                                                              ↓
                                                          ai_cache.c (store)
                                                              ↓
                                                          ai_events.c (queue)
                                                              ↓
                                                          Player Response
   ```

2. **Threading Model**:
   - Main thread: Game loop, never blocks
   - Worker threads: API calls (detached pthreads)
   - Event system: Response delivery with minimal delay

3. **Data Flow Between Components**:
   - **ai_service.c → ai_security.c**: Raw input for sanitization
   - **ai_security.c → ai_service.c**: Sanitized, safe prompts
   - **ai_service.c → ai_cache.c**: Cache lookups and storage
   - **ai_service.c → ai_events.c**: Response queuing
   - **ai_events.c → ai_service.c**: Retry requests on failure

### Key Data Structures

```c
// Global AI state (ai_service.c) - shared across all components
struct ai_service_state {
    bool initialized;
    struct ai_config *config;
    struct rate_limiter *limiter;
    struct ai_cache_entry *cache_head;
    int cache_size;
    CURL *curl_handle;  // Persistent for connection pooling
};

// Thread communication (ai_service.c ↔ ai_events.c)
struct ai_thread_request {
    char *prompt;
    char *cache_key;
    struct char_data *ch;
    struct char_data *npc;
    int request_type;
    pthread_t thread_id;
};
```

## Quick Start Guide

### Prerequisites
- OpenAI API key with GPT-4 access
- libcurl and pthread libraries installed
- MySQL/MariaDB database access

### Basic Setup (5 minutes)
```bash
# 1. Install dependencies
sudo apt-get install libcurl4-openssl-dev libssl-dev

# 2. Configure API key
cp .env.example lib/.env
echo "OPENAI_API_KEY=your-api-key-here" >> lib/.env

# 3. Compile with AI support
make clean && make

# 4. Run database migration
mysql -u user -p database < ai_service_migration.sql

# 5. Enable in-game
# Login as admin (LVL_GRGOD+) and run:
ai enable
ai reload
```

## Installation & Configuration

### Environment Configuration (.env)
Create `lib/.env` with the following settings:
```bash
# Required
OPENAI_API_KEY=sk-your-api-key-here

# Optional (defaults shown)
AI_MODEL=gpt-4o-mini              # Fast, cost-effective model
AI_MAX_TOKENS=500                  # Response length limit
AI_TEMPERATURE=3                   # Creativity (3 = 0.3)
AI_TIMEOUT_MS=30000               # 30 second timeout
AI_REQUESTS_PER_MINUTE=60         # Rate limiting
AI_REQUESTS_PER_HOUR=1000         # Rate limiting
AI_CONTENT_FILTER_ENABLED=true    # Content moderation
```

### Database Tables
The migration script creates:
- `ai_config` - Runtime configuration storage
- `ai_interactions` - Interaction history and analytics
- `ai_cache` - Response caching (not yet implemented)

### In-Game Commands (Admin Only)
```
ai                    # Show service status and statistics
ai enable            # Enable AI service globally
ai disable           # Disable AI service globally
ai reload            # Reload configuration from .env
ai test              # Test API connectivity
ai cache clear       # Clear response cache
ai cache cleanup     # Remove expired cache entries
ai reset             # Reset rate limits
```

## Usage Guide

### Enabling AI for NPCs

1. **Using medit** (when implemented):
   ```
   medit <mob_vnum>
   # Toggle AI_ENABLED flag
   ```

2. **Direct flag setting** (current method):
   - Set MOB_AI_ENABLED flag (bit 98) on the mobile
   - The NPC will now respond to tells using AI

3. **Testing AI NPCs**:
   ```
   tell guard Hello, how are you today?
   # AI-enabled guard will respond contextually
   ```

### How It Works
1. Player sends tell/say to AI-enabled NPC
2. Message is sanitized and sent to OpenAI API
3. Response is generated based on NPC context
4. Response delivered after minimal delay (< 1 second typical)

### Response Caching
- Duplicate messages return cached responses instantly
- Cache expires after 1 hour
- Reduces API costs and improves response time
- Average cache hit rate: 70%+

## Architecture Overview

### Core Components

#### ai_service.c
- Main API interface and request handling
- CURL-based HTTP client with connection pooling
- Threading support for non-blocking operations
- Retry logic with exponential backoff

#### ai_security.c
- Input sanitization for prompt injection prevention
- API key storage (currently plaintext - encryption planned)
- Secure memory operations

#### ai_cache.c
- In-memory LRU cache implementation
- Configurable TTL (default 1 hour)
- Automatic cleanup of expired entries

#### ai_events.c
- Integration with MUD event system
- Delayed response delivery
- Character validation to prevent crashes

### Request Flow
```
Player Input → Sanitization → Cache Check → API Request → Response Processing → Event Queue → Player
                                    ↓ (cache miss)
                              Thread Spawn → OpenAI API → Cache Storage
```

### Threading Model
- API calls run in detached pthreads
- Main game loop never blocks
- Responses delivered via event system
- Automatic cleanup on character disconnect

## Performance & Optimization

### Current Optimizations
- **Model**: gpt-4o-mini (80% cheaper than GPT-4, faster responses)
- **Temperature**: 0.3 (consistent, faster responses)
- **Threading**: True async - no game blocking
- **Caching**: 5000 entry cache, 1-hour TTL
- **Connection Pooling**: Persistent CURL handle
- **HTTP/2**: When supported by CURL version
- **Zero Delay**: Responses delivered immediately

### Performance Metrics
- Average response time: 1-2 seconds (uncached)
- Cache hit rate: 70%+ 
- API timeout: 30 seconds
- Memory usage: ~10MB for full cache

### Cost Management
- gpt-4o-mini: ~$0.001 per request
- With 70% cache rate: ~$0.30 per 1000 player interactions
- Monitor usage with `ai` command

## Security Considerations

### Current Implementation
- **API Key Storage**: Plaintext in .env file (AES encryption planned)
- **Input Sanitization**: Escapes special characters, limits length
- **Prompt Injection Prevention**: Fixed prompt templates
- **Rate Limiting**: Per-minute and per-hour limits
- **Access Control**: Admin-only configuration

### Best Practices
1. Restrict .env file permissions: `chmod 600 lib/.env`
2. Never commit .env to version control
3. Rotate API keys regularly
4. Monitor usage for anomalies
5. Review ai_interactions table for abuse

## Troubleshooting

### Common Issues

**AI not responding**
- Check service enabled: `ai`
- Verify API key loaded: Check logs for "API key loaded"
- Test connectivity: `ai test`
- Check rate limits: `ai` shows current usage

**NPCs not using AI**
- Verify MOB_AI_ENABLED flag (bit 98) is set
- Ensure NPC is not flagged MOBACT_NOTDEADYET
- Check NPC can receive tells (not MOB_NOTELL)

**Slow responses**
- Normal API latency: 1-2 seconds
- Check cache hit rate with `ai`
- Consider reducing AI_MAX_TOKENS
- Monitor API status at status.openai.com

**High costs**
- Switch to gpt-3.5-turbo for lower priority NPCs
- Increase cache TTL
- Implement more aggressive rate limiting
- Disable AI for non-essential NPCs

### Debug Mode
Enable detailed logging:
```c
// In ai_service.h, uncomment:
#define AI_DEBUG_ENABLED
```

## API Reference

### Core Functions

```c
// Initialize AI service (called at startup)
void init_ai_service(void);

// Shutdown and cleanup
void shutdown_ai_service(void);

// Check if service is enabled
bool is_ai_enabled(void);

// Generate NPC dialogue (blocking)
char *ai_npc_dialogue(struct char_data *npc, struct char_data *ch, const char *input);

// Generate NPC dialogue (async/non-blocking)
void ai_npc_dialogue_async(struct char_data *npc, struct char_data *ch, const char *input);

// Load configuration from .env
void load_ai_config(void);
```

### Cache Functions
```c
// Store response in cache
void ai_cache_response(const char *key, const char *response);

// Retrieve from cache
char *ai_cache_get(const char *key);

// Clear entire cache
void ai_cache_clear(void);

// Remove expired entries
void ai_cache_cleanup(void);
```

### Security Functions
```c
// Sanitize user input
char *sanitize_ai_input(const char *input);

// Secure memory clearing
void secure_memset(void *ptr, int value, size_t num);
```

### Configuration Structure
```c
struct ai_config {
    char encrypted_api_key[256];  // API key storage
    char model[64];              // OpenAI model name
    int max_tokens;              // Response length limit
    float temperature;           // Creativity (0.0-1.0)
    int timeout_ms;              // Request timeout
    bool content_filter_enabled; // Enable content filtering
    bool enabled;               // Global enable flag
};
```

## Current Limitations & Future Work

### Implemented Features
- ✅ Basic NPC dialogue via tells
- ✅ Response caching system
- ✅ Rate limiting
- ✅ Async/non-blocking operation
- ✅ Admin commands
- ✅ Performance optimizations

### Known Limitations
- API keys stored in plaintext (encryption planned)
- No medit integration yet (manual flag setting required)
- Cache is memory-only (lost on reboot)
- Single prompt template for all NPCs
- No conversation memory between interactions

### Planned Enhancements
1. **Security**: AES-256 encryption for API keys
2. **Editor Support**: medit integration for AI flags
3. **Persistence**: Database-backed cache
4. **Features**: Room descriptions, quest generation
5. **NPC Personalities**: Individual personality traits
6. **Memory**: Conversation history per player/NPC pair
7. **Moderation**: ai_moderate_content() implementation

### Not Yet Implemented
```c
// Declared but not implemented:
char *ai_generate_room_desc(int room_vnum, int sector_type);
bool ai_moderate_content(const char *text);
```

## Contributing

When modifying the AI service:
1. Maintain C90/C89 compatibility (no C99 features)
2. Follow existing code style
3. Update this documentation
4. Test with valgrind for memory leaks
5. Verify thread safety

## Support

For issues:
1. Check system logs: `grep AI syslog`
2. Enable debug mode (see Troubleshooting)
3. Review this documentation
4. Submit issues to GitHub repository

## Component Responsibilities

### 1. ai_service.c - Core Engine
**Primary Role**: Orchestrates all AI operations and manages API communication

**Key Responsibilities**:
- Manages global AI state (ai_state)
- Handles OpenAI API requests via CURL
- Implements retry logic and connection pooling
- Creates worker threads for async operations
- Builds and parses JSON requests/responses

**Depends On**:
- ai_security.c for input sanitization and API key handling
- ai_cache.c for response caching
- ai_events.c for async response delivery

**Key Functions**:
- `ai_npc_dialogue_async()` - Main entry point from game
- `make_api_request()` - Handles API communication with retries
- `ai_thread_worker()` - Worker thread for async requests

### 2. ai_security.c - Security Layer
**Primary Role**: Protects against malicious input and manages API keys

**Key Responsibilities**:
- Sanitizes all user input to prevent prompt injection
- Manages API key encryption/decryption (currently plaintext)
- Provides secure memory clearing functions
- Validates and escapes special characters for JSON

**Used By**: ai_service.c exclusively

**Key Functions**:
- `sanitize_ai_input()` - CRITICAL: Prevents prompt injection
- `encrypt_api_key()` / `decrypt_api_key()` - API key management
- `secure_memset()` - Prevents memory inspection of keys

**Security Notes**:
- All user input MUST pass through sanitize_ai_input()
- API keys need proper encryption implementation
- Memory containing keys should be cleared with secure_memset()

### 3. ai_cache.c - Performance Layer
**Primary Role**: Reduces API costs and improves response time

**Key Responsibilities**:
- Maintains in-memory cache of responses
- Implements TTL-based expiration (1 hour)
- Enforces cache size limits (5000 entries)
- Provides O(1) cache operations

**Direct Access**: Manipulates ai_state.cache_head and cache_size

**Key Functions**:
- `ai_cache_get()` - Check cache before API calls
- `ai_cache_response()` - Store successful responses
- `ai_cache_cleanup()` - Remove expired/excess entries

**Performance Impact**:
- Cache hit: ~0ms response time
- Cache miss: 1-2 second API call
- Target 70%+ cache hit rate

### 4. ai_events.c - Async Delivery Layer
**Primary Role**: Ensures thread-safe async response delivery

**Key Responsibilities**:
- Queues AI responses for delivery to players
- Validates character pointers before delivery
- Implements retry logic with exponential backoff
- Integrates with MUD event system

**Thread Safety**: Critical for preventing crashes when characters are freed

**Key Functions**:
- `queue_ai_response()` - Queue response from any thread
- `ai_response_event()` - Deliver response (with validation)
- `queue_ai_request_retry()` - Retry failed requests

**Event Types**:
- `ai_response_event` - Delivers responses to players
- `ai_request_retry_event` - Retries failed API requests

## Data Flow

### Successful Request Flow
```
1. Player tells NPC → act.comm.c
2. act.comm.c → ai_npc_dialogue_async()
3. Check cache → ai_cache_get()
4. Cache hit → queue_ai_response() → Player
5. Cache miss → Create worker thread
6. Worker thread → sanitize_ai_input()
7. Worker thread → make_api_request()
8. API response → ai_cache_response()
9. queue_ai_response() → ai_response_event()
10. Validate characters → Deliver to player
```

### Failed Request with Retry
```
1. Initial request fails in worker thread
2. queue_ai_request_retry() with retry_count=0
3. ai_request_retry_event() after delay
4. ai_generate_response_async() (one attempt)
5. If success → queue_ai_response()
6. If fail → queue_ai_request_retry() with retry_count+1
7. Repeat until AI_MAX_RETRIES reached
```

## Critical Safety Checks

### Thread Safety
- Character pointers may become invalid during async operations
- Always validate characters exist in character_list before use
- Event handlers must re-validate even if caller validated

### Input Validation
- All user input must pass through sanitize_ai_input()
- Escape special characters for JSON
- Enforce length limits to prevent buffer overflows

### Memory Management
- API responses are allocated strings - caller must free()
- Cache returns pointers to internal strings - do NOT free()
- Use secure_memset() for sensitive data like API keys

## Common Integration Points

### Adding AI to NPCs
1. Set MOB_AI_ENABLED flag (bit 98) on the mobile
2. NPC will respond to tells using AI
3. Responses are cached by "npc_<vnum>_<input>" key

### Admin Commands
- `ai` - Show status and statistics
- `ai enable/disable` - Toggle service
- `ai reload` - Reload configuration
- `ai cache clear` - Clear all cached responses
- `ai reset` - Reset rate limits

## Debugging

### Enable Debug Logging
In ai_service.h, set:
```c
#define AI_DEBUG_MODE 1
```

This enables detailed logging of:
- API request/response flow
- Cache operations
- Thread creation and completion
- Character validation
- Rate limiting decisions

### Common Issues
1. **No AI responses**: Check service enabled, API key loaded
2. **Slow responses**: Normal latency 1-2 seconds for cache misses
3. **Rate limiting**: Check counters with 'ai' command
4. **Crashes**: Usually character validation issues - check debug logs

## Future Enhancements
1. Implement proper API key encryption (AES-256)
2. Add persistent cache (database-backed)
3. Implement ai_generate_room_desc() for dynamic rooms
4. Add conversation memory/context
5. Implement content moderation
6. Add per-NPC personality configuration

---

**Version**: 2.0  
**Last Updated**: January 2025  
**Maintainer**: LuminariMUD Development Team