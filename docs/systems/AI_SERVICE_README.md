# LuminariMUD AI Service Documentation

## Overview

The AI Service integrates both OpenAI's GPT models and local Ollama LLM into LuminariMUD, enabling dynamic NPC dialogue through natural language processing. NPCs with the AI_ENABLED flag can respond intelligently to player messages using context-aware responses. The system features automatic fallback from OpenAI to Ollama, ensuring AI-powered NPCs are always available.

**Current Status**: IMPLEMENTED (January 2025) - Core NPC dialogue with dual AI backend support  
**Latest Updates**: 
- Ollama model warmup during server startup
- Enhanced error reporting with specific failure reasons
- Proper JSON escaping for special characters
- Backend tracking in syslogs (OpenAI/Ollama/Cache/Fallback)
- AI service auto-initialization at boot

## Key Features

- **Dual AI Backend**: OpenAI GPT-4 primary, Ollama LLM fallback
- **Always-On AI**: Automatic fallback ensures AI responses even when OpenAI is disabled/unavailable
- **Cost Optimization**: Local Ollama provides free AI responses when OpenAI is disabled
- **Response Caching**: Reduces API calls and improves response time
- **Non-Blocking**: Async operations prevent game lag
- **Thread-Safe**: Robust handling of character disconnections

## System Architecture & Component Interaction

### Component Overview
The AI system consists of four tightly integrated components with dual AI backend support:

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
│  - Ollama fallback integration                                          │
└────────────────┬──────────────────┬──────────────────┬─────────────────┘
                 │                  │                  │
     ┌───────────▼──────────┐ ┌────▼─────┐ ┌─────────▼─────────┐
     │    ai_security.c     │ │ai_cache.c│ │   ai_events.c     │
     │ - Input sanitization │ │ - LRU    │ │ - Event queuing   │
     │ - API key handling   │ │   cache  │ │ - Async delivery  │
     │ - Secure memory ops  │ │ - TTL    │ │ - Thread safety   │
     └──────────────────────┘ └──────────┘ └───────────────────┘
```

### AI Backend Flow

```
1. AI Service Disabled (ai disable):
   Player → NPC Tell → Ollama (local) → Response
                        ↓ (if fails)
                        Generic fallback

2. AI Service Enabled (ai enable):
   Player → NPC Tell → OpenAI API → Response
                        ↓ (if fails after 3 retries)
                        Ollama (local) → Response
                        ↓ (if fails)
                        Generic fallback

3. Cache Hit (both modes):
   Player → NPC Tell → Cache → Instant Response
```

### Component Interactions

1. **Request Flow with Fallback**:
   ```
   Player Tell → ai_service.c → ai_security.c (sanitize) → ai_cache.c (check)
                                                              ↓ (miss)
                                                          OpenAI API (if enabled)
                                                              ↓ (fail)
                                                          Ollama API (fallback)
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
    char *prompt;        /* Sanitized prompt to send to API */
    char *cache_key;     /* Key for storing response in cache */
    struct char_data *ch;   /* Player character (must validate) */
    struct char_data *npc;  /* NPC character (must validate) */
    int request_type;    /* AI_REQUEST_* constant for logging */
    pthread_t thread_id; /* Thread ID for debugging */
};
```

## Quick Start Guide

### Prerequisites
- OpenAI API key with GPT-4 access (optional)
- Ollama installed locally (for fallback/always-on AI)
- libcurl and pthread libraries installed
- MySQL/MariaDB database access

### Basic Setup (10 minutes)

#### Step 1: Install Dependencies
```bash
sudo apt-get install libcurl4-openssl-dev libssl-dev
```

#### Step 2: Install Ollama (for fallback AI)
```bash
# Install Ollama service
sudo su -
curl -fsSL https://ollama.com/install.sh | sh
exit

# Add your user to ollama group
sudo usermod -a -G ollama $(whoami)

# Pull the lightweight model
ollama pull llama3.2:1b

# Verify Ollama is running
systemctl status ollama
ollama list
```

#### Step 3: Configure OpenAI (optional)
```bash
# Only if you have an OpenAI API key
cp lib/.env.example lib/.env
echo "OPENAI_API_KEY=sk-your-api-key-here" >> lib/.env
```

#### Step 4: Compile and Enable
```bash
# Compile with AI support
make clean && make

# Run database migration
mysql -u user -p database < ai_service_migration.sql

# Enable in-game (as admin)
ai enable  # Enables OpenAI with Ollama fallback
# OR
ai disable # Uses only Ollama (free, local AI)
ai reload
```

## Installation & Configuration

### Environment Configuration (.env)
Create `lib/.env` with the following settings:
```bash
# OpenAI Configuration (optional - system works without it)
OPENAI_API_KEY=sk-your-api-key-here

# Optional Settings (defaults shown)
AI_MODEL=gpt-4o-mini              # OpenAI model (when available)
AI_MAX_TOKENS=500                  # Response length limit
AI_TEMPERATURE=3                   # Creativity (3 = 0.3)
AI_TIMEOUT_MS=30000               # 30 second timeout for OpenAI
AI_REQUESTS_PER_MINUTE=60         # Rate limiting
AI_REQUESTS_PER_HOUR=1000         # Rate limiting
AI_CONTENT_FILTER_ENABLED=true    # Content moderation

# Ollama Configuration (automatic defaults)
# OLLAMA_HOST=localhost:11434     # Ollama endpoint (default)
# OLLAMA_MODEL=llama3.2:1b        # Fast local model (default)
```

### Ollama Models
Different models can be used for different performance/quality tradeoffs:

```bash
# Fast responses (1-2 seconds)
ollama pull llama3.2:1b      # 1.3GB, fastest, 128K context window
ollama pull tinyllama         # 1.1GB, optimized for speed

# Better quality (2-3 seconds)
ollama pull llama3.2:3b      # 3GB, better responses, 128K context
ollama pull mistral:7b       # 7GB, excellent roleplay

# Check installed models
ollama list
```

**Model Context Windows**:
- **llama3.2:1b**: 128,000 tokens (~100,000 words) - Excellent for conversation history
- **llama3.2:3b**: 128,000 tokens - Same capacity, better quality
- **mistral:7b**: 32,000 tokens (~24,000 words) - Still plenty for NPCs

Current implementation uses only single prompts. Future enhancements will leverage the large context window for conversation history and world lore.

### Database Tables
The migration script creates:
- `ai_config` - Runtime configuration storage
- `ai_interactions` - Interaction history and analytics
- `ai_cache` - Response caching (memory-based currently)

### In-Game Commands (Admin Only)
```
ai                    # Show service status and statistics
ai enable            # Enable OpenAI (with Ollama fallback)
ai disable           # Disable OpenAI (Ollama-only mode)
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
   # Guard responds with AI (OpenAI or Ollama depending on config)
   ```

### How It Works

#### When AI is Enabled (OpenAI mode)
1. Player sends tell/say to AI-enabled NPC
2. Check cache for existing response
3. If not cached, sanitize and send to OpenAI API
4. If OpenAI fails, automatically try Ollama
5. Cache successful response
6. Deliver response to player (typically < 2 seconds)

#### When AI is Disabled (Ollama-only mode)
1. Player sends tell/say to AI-enabled NPC
2. Check cache for existing response
3. If not cached, send to local Ollama service
4. Cache successful response
5. Deliver response to player (typically < 1 second)

### Response Caching
- Works for both OpenAI and Ollama responses
- Duplicate messages return cached responses instantly
- Cache expires after 1 hour
- Reduces API costs and improves response time
- Average cache hit rate: 70%+

## Architecture Details

### Core Components

#### ai_service.c (Main Service Implementation)
**Location**: `src/ai_service.c`  
**Purpose**: Main API interface and request handling

**Key Features**:
- Dual backend support (OpenAI + Ollama)
- CURL-based HTTP client with connection pooling
- Threading support for non-blocking operations
- Retry logic with exponential backoff
- Automatic fallback logic

**Key Functions**:
- `init_ai_service()` - Initialize AI service at boot (called from db.c)
- `shutdown_ai_service()` - Clean shutdown and resource deallocation
- `is_ai_enabled()` - Central check for AI service status
- `load_ai_config()` - Load configuration from .env file
- `ai_npc_dialogue()` - Blocking NPC dialogue (testing only)
- `ai_npc_dialogue_async()` - Non-blocking NPC dialogue (production)
- `ai_generate_response()` - High-level response generation with fallback
- `ai_generate_response_async()` - Async version for event system
- `ai_check_rate_limit()` - Enforce API rate limits
- `ai_reset_rate_limits()` - Admin command to reset limits

**Ollama Integration Functions**:
- `make_ollama_request()` - Sends requests to local Ollama with enhanced error reporting
- `build_ollama_json_request()` - Formats Ollama API requests with proper JSON escaping
- `parse_ollama_json_response()` - Extracts Ollama responses from JSON
- `json_escape_string()` - Properly escapes strings for JSON encoding (C89-compatible)
- `warmup_ollama_model()` - Preloads model at startup to avoid first-request delays
- `generate_fallback_response()` - Now tries Ollama before generic responses

**OpenAI Functions**:
- `make_api_request()` - High-level request with retries
- `make_api_request_single()` - Single API request attempt
- `build_json_request()` - Build OpenAI JSON request
- `parse_json_response()` - Parse OpenAI JSON response

**Thread Management**:
- `ai_thread_worker()` - Worker thread function for async requests
- Thread request structure for communication between threads
- Detached thread creation for non-blocking operations

#### ai_security.c (Security Layer)
**Location**: `src/ai_security.c`  
**Purpose**: Security functions for API key handling and input sanitization

**Key Functions**:
- `secure_memset()` - Secure memory clearing (prevents compiler optimization)
- `encrypt_api_key()` - API key encryption (currently plaintext - TODO: AES-256)
- `decrypt_api_key()` - API key decryption (returns allocated string)
- `load_encrypted_api_key()` - Load API key from file
- `sanitize_ai_input()` - Critical prompt injection prevention

**Security Features**:
- Input sanitization for prompt injection prevention
- API key storage (currently plaintext - encryption planned)
- Secure memory operations to prevent key leakage
- Control character filtering
- JSON special character escaping
- Works for both OpenAI and Ollama prompts

**TODO**: Implement proper AES-256 encryption for API keys

#### ai_cache.c (Response Caching)
**Location**: `src/ai_cache.c`  
**Purpose**: LRU cache implementation for response reuse

**Key Functions**:
- `ai_cache_response()` - Add/update response in cache
- `ai_cache_get()` - Retrieve cached response by key
- `ai_cache_clear()` - Clear all cache entries
- `ai_cache_cleanup()` - Remove expired entries and enforce size limits

**Cache Strategy**:
- In-memory linked list (lost on reboot)
- Time-based expiration (1 hour TTL)
- 5000 entries maximum
- Stores both OpenAI and Ollama responses
- Key format: "npc_<vnum>_<input>"
- O(n) lookup (could be optimized with hash table)
- Two-phase cleanup: expired entries first, then oldest

#### ai_events.c (Event System Integration)
**Location**: `src/ai_events.c`  
**Purpose**: Async response delivery and retry logic

**Key Functions**:
- `queue_ai_response()` - Queue AI response for delivery
- `queue_ai_request_retry()` - Queue retry for failed requests
- `ai_response_event()` - Event handler for response delivery
- `ai_request_retry_event()` - Event handler for request retries

**Event System Features**:
- Integration with MUD event system (mud_event.c)
- Delayed response delivery for natural flow
- Character validation to prevent crashes
- Thread-safe response queuing
- Exponential backoff for retries
- Handles responses from both AI backends

**Thread Safety**:
- Worker threads queue events via `queue_ai_response()`
- Main thread processes events via event handlers
- Extensive character validation before delivery
- Events self-destruct if characters freed

### Request Flow with Ollama Fallback
```
Player Input 
    ↓
Sanitization 
    ↓
Cache Check ─── Hit ──→ Return Cached Response
    ↓ Miss
AI Enabled? ─── No ──→ Try Ollama ──→ Response
    ↓ Yes                    ↓ Fail
Try OpenAI                Generic Fallback
    ↓ Fail (3 retries)
Try Ollama
    ↓ Fail
Generic Fallback
```

### Threading Model
- API calls run in detached pthreads
- Main game loop never blocks
- Both OpenAI and Ollama use same threading system
- Responses delivered via event system
- Automatic cleanup on character disconnect

## Performance & Optimization

### Dual Backend Performance

**OpenAI (when enabled)**:
- Response time: 1-2 seconds (uncached)
- Cost: ~$0.001 per request
- Model: gpt-4o-mini (fast, cost-effective)
- Timeout: 30 seconds

**Ollama (fallback/primary)**:
- Response time: 0.5-1 second (uncached)
- Cost: Free (runs locally)
- Model: llama3.2:1b (configurable)
- Timeout: 10 seconds for requests (allows for model loading)
- Token limit: 100 tokens per response (configurable)
- Context window: 128K tokens available (future use)

**Cache (both backends)**:
- Response time: ~0ms (instant)
- Hit rate: 70%+
- Capacity: 5000 entries
- TTL: 1 hour

### Current Optimizations
- **Dual Backend**: Automatic fallback ensures 99%+ AI availability
- **Model Selection**: gpt-4o-mini for OpenAI, llama3.2:1b for Ollama
- **Temperature**: 0.3-0.7 (consistent responses)
- **Threading**: True async - no game blocking
- **Caching**: Unified cache for both backends
- **Connection Pooling**: Persistent CURL handles
- **HTTP/2**: When supported by CURL version
- **Zero Delay**: Responses delivered immediately

### Cost Management
- **OpenAI costs**: ~$0.30 per 1000 interactions (with cache)
- **Ollama costs**: $0 (runs on your hardware)
- **Hybrid mode**: Significant cost reduction when Ollama handles failures
- **Ollama-only mode**: Completely free AI NPCs

### Resource Usage
- **Memory**: ~10MB for full cache + ~1.2GB for Ollama model
- **CPU**: Ollama uses 50-70% CPU for 1-2 seconds during generation
- **Network**: OpenAI requires internet, Ollama is local-only

## Security Considerations

### Current Implementation
- **API Key Storage**: Plaintext in .env file (AES encryption planned)
- **Input Sanitization**: Escapes special characters, limits length
- **Prompt Injection Prevention**: Fixed prompt templates
- **Rate Limiting**: Per-minute and per-hour limits (OpenAI only)
- **Access Control**: Admin-only configuration
- **Local Ollama**: No authentication (localhost only)

### Best Practices
1. Restrict .env file permissions: `chmod 600 lib/.env`
2. Never commit .env to version control
3. Rotate OpenAI API keys regularly
4. Keep Ollama bound to localhost only
5. Monitor usage for anomalies
6. Review ai_interactions table for abuse

## Troubleshooting

### Common Issues

**No AI responses at all**
- Check Ollama is running: `systemctl status ollama`
- Test Ollama: `curl http://localhost:11434/api/generate -d '{"model":"llama3.2:1b","prompt":"Hi","stream":false}'`
- Check service status: `ai` command in-game
- Verify NPC has MOB_AI_ENABLED flag
- Check startup logs for "AI Service: Warming up Ollama model" message
- Look for specific error messages:
  - "Cannot connect to Ollama" - Service not running
  - "Ollama model llama3.2:1b not found" - Model not installed
  - "Ollama request timed out" - Model loading, increase timeout

**OpenAI not working (but Ollama works)**
- Verify API key in lib/.env
- Check rate limits with `ai` command
- Monitor OpenAI status: status.openai.com
- Fallback to Ollama is automatic

**Ollama not working**
- Ensure Ollama service is running: `sudo systemctl start ollama`
- Check model is installed: `ollama list`
- Pull model if missing: `ollama pull llama3.2:1b`
- Check port 11434 is not blocked

**Slow responses**
- OpenAI: Normal latency 1-2 seconds
- Ollama: Should be < 1 second locally
- Check cache hit rate with `ai` command
- Consider using smaller Ollama model
- First Ollama request after idle may be slower (model loading)

**High costs (OpenAI)**
- Switch to Ollama-only mode: `ai disable`
- Or increase cache TTL
- Or use gpt-3.5-turbo for lower priority NPCs

### Debug Mode
Enable detailed logging:
```c
// In ai_service.h, set:
#define AI_DEBUG_MODE 1
```

This enables logging of:
- API request/response flow (both backends)
- Cache operations with hit/miss details
- Fallback decisions with reasons
- Thread creation and completion
- Character validation
- Rate limiting decisions
- JSON request/response content (first 200 chars)
- CURL operations and HTTP status codes
- Ollama model warmup process
- Backend selection ("AI [OpenAI]:", "AI [Ollama]:", "AI [Cache]:", "AI [Fallback]:")

## API Reference

### Core Functions (ai_service.c)

```c
// Initialization and lifecycle
void init_ai_service(void);              // Called at boot from db.c
void shutdown_ai_service(void);          // Clean shutdown
bool is_ai_enabled(void);                // Check service status
void load_ai_config(void);               // Load/reload configuration

// NPC dialogue generation
char *ai_npc_dialogue(struct char_data *npc, struct char_data *ch, const char *input);       // Blocking
void ai_npc_dialogue_async(struct char_data *npc, struct char_data *ch, const char *input);  // Non-blocking

// Response generation
char *ai_generate_response(const char *prompt, int request_type);                            // With fallback
char *ai_generate_response_async(const char *prompt, int request_type, int retry_count);     // For events
char *generate_fallback_response(const char *prompt);                                        // Ollama + generic

// Rate limiting
bool ai_check_rate_limit(void);          // Check if request allowed
void ai_reset_rate_limits(void);         // Reset counters (admin)

// Utility functions
void log_ai_error(const char *function, const char *error);
void log_ai_interaction(struct char_data *ch, struct char_data *npc, 
                       const char *response, const char *backend, bool from_cache);
int get_cache_size(void);

// Future implementations (stubs)
char *ai_generate_room_desc(int room_vnum, int sector_type);
bool ai_moderate_content(const char *text);
```

### Security Functions (ai_security.c)
```c
// Memory security
void secure_memset(void *ptr, int value, size_t num);   // Secure clearing

// API key management
int encrypt_api_key(const char *plaintext, char *encrypted_out);  // Store key
char *decrypt_api_key(const char *encrypted);                     // Retrieve key (must free)
void load_encrypted_api_key(const char *filename);                // Load from file

// Input sanitization
char *sanitize_ai_input(const char *input);                      // Prevent injection
```

### Cache Functions (ai_cache.c)
```c
// Cache operations
void ai_cache_response(const char *key, const char *response);   // Add/update
char *ai_cache_get(const char *key);                             // Retrieve (don't free)
void ai_cache_clear(void);                                       // Clear all
void ai_cache_cleanup(void);                                     // Remove expired
```

### Event Functions (ai_events.c)
```c
// Event queuing
void queue_ai_response(struct char_data *ch, struct char_data *npc, 
                       const char *response, const char *backend, bool from_cache);
void queue_ai_request_retry(const char *prompt, int request_type, int retry_count,
                            struct char_data *ch, struct char_data *npc);

// Event handlers (internal)
EVENTFUNC(ai_response_event);            // Deliver responses
EVENTFUNC(ai_request_retry_event);       // Retry failed requests
```

### Data Structures

```c
// Global AI state (ai_service.c)
struct ai_service_state {
    bool initialized;                    // Service initialization flag
    struct ai_config *config;            // Configuration structure
    struct rate_limiter *limiter;        // API rate limiting
    struct ai_cache_entry *cache_head;   // Cache linked list head
    int cache_size;                      // Current cache size
    CURL *curl_handle;                   // Persistent CURL handle
};

// Configuration (ai_service.h)
struct ai_config {
    char encrypted_api_key[256];         // OpenAI API key storage
    char model[64];                      // OpenAI model name
    int max_tokens;                      // Response length limit
    float temperature;                   // Creativity (0.0-1.0)
    int timeout_ms;                      // Request timeout
    bool content_filter_enabled;         // Enable content filtering
    bool enabled;                        // Global enable flag (OpenAI)
};

// Rate limiter (ai_service.h)
struct rate_limiter {
    int requests_per_minute;             // Minute limit
    int requests_per_hour;               // Hour limit
    time_t minute_reset;                 // Next minute reset
    time_t hour_reset;                   // Next hour reset
    int current_minute_count;            // Current minute count
    int current_hour_count;              // Current hour count
};

// Cache entry (ai_service.h)
struct ai_cache_entry {
    char *key;                           // Cache key
    char *response;                      // Cached response
    time_t expires_at;                   // Expiration timestamp
    struct ai_cache_entry *next;         // Next entry in list
};

// Thread request (ai_service.c)
struct ai_thread_request {
    char *prompt;                        // Sanitized prompt
    char *cache_key;                     // Cache storage key
    struct char_data *ch;                // Player character
    struct char_data *npc;               // NPC character
    int request_type;                    // AI_REQUEST_* constant
    pthread_t thread_id;                 // Thread identifier
    char backend[32];                    // Backend used (OpenAI/Ollama/etc)
};

// Response event (ai_events.c)
struct ai_response_event {
    struct char_data *ch;                // Player character
    struct char_data *npc;               // NPC character
    char *response;                      // AI response text
    char *backend;                       // Backend identifier
    bool from_cache;                     // Cache hit flag
};

// Retry event (ai_events.c)
struct ai_request_retry_event {
    char *prompt;                        // Original prompt
    int request_type;                    // Request type
    int retry_count;                     // Retry attempt number
    struct char_data *ch;                // Optional player
    struct char_data *npc;               // Optional NPC
};
```

### Configuration Constants

```c
// API endpoints
#define OPENAI_API_ENDPOINT "https://api.openai.com/v1/chat/completions"
#define OLLAMA_API_ENDPOINT "http://localhost:11434/api/generate"

// Model defaults
#define OLLAMA_MODEL "llama3.2:1b"      // Default Ollama model

// Cache settings
#define AI_MAX_CACHE_SIZE 5000            // Maximum cache entries
#define AI_CACHE_EXPIRE_TIME 3600         // Cache TTL (1 hour)

// Rate limiting defaults
#define AI_MAX_RETRIES 3                  // API retry attempts
#define AI_MAX_TOKENS 500                 // Default max response tokens
#define AI_TIMEOUT_MS 30000               // Default timeout (30 seconds)

// Request types
#define AI_REQUEST_NPC_DIALOGUE 1         // NPC conversation
#define AI_REQUEST_ROOM_DESC 2            // Room description (future)
#define AI_REQUEST_QUEST_GEN 3            // Quest generation (future)

// Ollama request options (in build_ollama_json_request)
"options": {
  "num_predict": 100,    // Max tokens to generate
  "temperature": 0.7,    // Creativity level
  "top_k": 40,          // Sampling parameter
  "top_p": 0.9          // Nucleus sampling
}
```

## Testing Tools

### Test Ollama Integration
A standalone test program is available:
```bash
# Compile test program
gcc -o test_ollama test_ollama_ai.c -lcurl

# Test with custom prompt
./test_ollama "A player asks about your quest"

# Test default greeting
./test_ollama
```

### In-Game Testing
```bash
# Test with AI enabled (OpenAI + Ollama fallback)
ai enable
ai test
tell guard hello

# Test with AI disabled (Ollama only)
ai disable
tell guard hello

# Check statistics
ai
```

## Current Limitations & Future Work

### Implemented Features
- ✅ Basic NPC dialogue via tells
- ✅ Response caching system
- ✅ Rate limiting (OpenAI)
- ✅ Async/non-blocking operation
- ✅ Admin commands
- ✅ Performance optimizations
- ✅ Ollama fallback integration
- ✅ Always-on AI capability

### Known Limitations
- API keys stored in plaintext (encryption planned)
- No medit integration yet (manual flag setting required)
- Cache is memory-only (lost on reboot)
- Single prompt template for all NPCs
- No conversation memory between interactions
- Ollama model must be pre-downloaded

### Planned Enhancements
1. **Security**: AES-256 encryption for API keys
2. **Editor Support**: medit integration for AI flags
3. **Persistence**: Database-backed cache
4. **Features**: Room descriptions, quest generation
5. **NPC Personalities**: Individual personality traits
6. **Memory**: Conversation history per player/NPC pair (leverage 128K context)
7. **Moderation**: ai_moderate_content() implementation
8. **Dynamic Models**: Per-NPC model selection
9. **Ollama Config**: Runtime model switching
10. **Context Enhancement**: Include world lore, zone info, NPC backgrounds
11. **Conversation Memory**: Track last N exchanges with each player
12. **Dynamic Prompts**: Context-aware prompts based on game state

### Not Yet Implemented
```c
// Declared but not implemented:
char *ai_generate_room_desc(int room_vnum, int sector_type);
bool ai_moderate_content(const char *text);
```

## Ollama Administration

### Managing Ollama Service
```bash
# Service control
sudo systemctl start ollama
sudo systemctl stop ollama
sudo systemctl restart ollama
sudo systemctl status ollama

# View logs
journalctl -u ollama -f          # Follow logs
journalctl -u ollama -n 100      # Last 100 lines

# Model management
ollama list                      # List installed models
ollama pull MODEL_NAME           # Download a model
ollama rm MODEL_NAME             # Remove a model
ollama show MODEL_NAME           # Show model details
```

### Performance Tuning
```bash
# For faster responses (lower quality)
ollama pull tinyllama

# For better quality (slower)
ollama pull llama3.2:3b
ollama pull mistral:7b

# Set default model in ai_service.h
#define OLLAMA_MODEL "tinyllama"  # Change as needed
```

## Contributing

When modifying the AI service:
1. Maintain C90/C89 compatibility (no C99 features)
2. Test both OpenAI and Ollama backends
3. Ensure fallback logic works correctly
4. Update this documentation
5. Test with valgrind for memory leaks
6. Verify thread safety

## Support

For issues:
1. Check system logs: `grep AI syslog`
2. Check Ollama logs: `journalctl -u ollama -n 50`
3. Enable debug mode (see Troubleshooting)
4. Test with standalone test program
5. Review this documentation
6. Submit issues to GitHub repository

## Implementation Notes

### File Locations
```
src/
├── ai_service.c     # Main service implementation
├── ai_service.h     # Headers and structures
├── ai_security.c    # Security functions
├── ai_events.c      # Event system integration
└── ai_cache.c       # Response caching

lib/
├── .env             # API keys and configuration
└── .env.example     # Configuration template

docs/systems/
└── AI_SERVICE_README.md  # This documentation
```

### JSON Handling
The system uses a custom `json_escape_string()` function for proper JSON encoding since the codebase maintains C89/C90 compatibility and cannot use modern JSON libraries. This function handles:
- Escape sequences: `\"`, `\\`, `\n`, `\r`, `\t`, `\b`, `\f`
- Control character filtering (< 0x20)
- Non-ASCII character filtering (>= 0x80)
- Buffer overflow protection
- ASCII-only output for compatibility

### Startup Sequence
1. `boot_db()` in db.c calls `init_ai_service()`
2. AI service initializes CURL library globally
3. Allocates configuration structure with defaults
4. Allocates rate limiter structure
5. Initializes empty cache
6. Creates persistent CURL handle for connection pooling
7. Loads configuration from .env file via `load_ai_config()`
8. Attempts to warm up Ollama model (non-blocking on failure)
9. Sets initialized flag, service ready for requests

### Shutdown Sequence
1. `shutdown_ai_service()` called during MUD shutdown
2. Clears all cache entries, freeing memory
3. Securely clears API key from memory
4. Frees configuration structure
5. Frees rate limiter structure
6. Cleans up persistent CURL handle
7. Calls `curl_global_cleanup()`
8. Clears initialized flag

### Threading Model
```
Main Thread:
├── Game loop processing
├── Cache lookups (immediate)
├── Event processing
└── Character validation

Worker Threads (detached):
├── API calls (blocking)
├── Response caching
└── Event queuing
```

### Memory Management
- All allocated strings use `strdup()` or `CREATE()` macro
- Worker threads free their own request structures
- Event handlers free event data after processing
- Cache entries freed on expiration or cleanup
- API keys cleared with `secure_memset()` on shutdown

### Error Handling Strategy
1. **API Failures**: Retry with exponential backoff, then Ollama fallback
2. **Ollama Failures**: Return generic fallback responses
3. **Memory Failures**: Log error, return NULL or abort operation
4. **Character Freed**: Events self-destruct, no response delivered
5. **Rate Limit**: Block request, return NULL

### Syslog Backend Identification
All AI interactions are logged with backend identification:
- `AI [OpenAI]:` - Response from OpenAI API
- `AI [Ollama]:` - Response from local Ollama
- `AI [Cache]:` - Response from cache (any backend)
- `AI [Fallback]:` - Generic fallback response
- `AI [Retry]:` - Response after retry attempts

### Debug Mode
When `AI_DEBUG_MODE` is enabled in ai_service.h:
- Detailed function entry/exit logging
- Parameter validation traces
- CURL operation details
- JSON request/response content (truncated)
- Cache hit/miss statistics
- Thread creation/completion
- Character validation results
- Memory allocation tracking

---

**Version**: 3.1  
**Last Updated**: January 2025  
**Maintainer**: LuminariMUD Development Team  
**Key Updates**: 
- Added Ollama LLM integration for always-on AI capability
- Implemented proper JSON escaping for special characters
- Added model warmup and auto-initialization at boot
- Enhanced error reporting and backend tracking
- Documented 128K context window capabilities