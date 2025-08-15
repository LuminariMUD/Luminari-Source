# LuminariMUD AI Service Documentation

## Overview

The AI Service integrates both OpenAI's GPT models and local Ollama LLM into LuminariMUD, enabling dynamic NPC dialogue through natural language processing. NPCs with the AI_ENABLED flag can respond intelligently to player messages using context-aware responses. The system features automatic fallback from OpenAI to Ollama, ensuring AI-powered NPCs are always available.

**Current Status**: IMPLEMENTED (January 2025) - Core NPC dialogue with dual AI backend support

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
ollama pull llama3.2:1b      # 1.3GB, fastest
ollama pull tinyllama         # 1.1GB, optimized for speed

# Better quality (2-3 seconds)
ollama pull llama3.2:3b      # 3GB, better responses
ollama pull mistral:7b       # 7GB, excellent roleplay

# Check installed models
ollama list
```

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

#### ai_service.c
- Main API interface and request handling
- Dual backend support (OpenAI + Ollama)
- CURL-based HTTP client with connection pooling
- Threading support for non-blocking operations
- Retry logic with exponential backoff
- Automatic fallback logic

**New Ollama Functions**:
- `make_ollama_request()` - Sends requests to local Ollama
- `build_ollama_json_request()` - Formats Ollama API requests
- `parse_ollama_json_response()` - Extracts Ollama responses
- `generate_fallback_response()` - Now tries Ollama before generic responses

#### ai_security.c
- Input sanitization for prompt injection prevention
- API key storage (currently plaintext - encryption planned)
- Secure memory operations
- Works for both OpenAI and Ollama prompts

#### ai_cache.c
- In-memory LRU cache implementation
- Stores both OpenAI and Ollama responses
- Configurable TTL (default 1 hour)
- Automatic cleanup of expired entries

#### ai_events.c
- Integration with MUD event system
- Delayed response delivery
- Character validation to prevent crashes
- Handles responses from both AI backends

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
- Timeout: 3 seconds (local network)

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
- Cache operations
- Fallback decisions
- Thread creation and completion
- Character validation
- Rate limiting decisions

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

// Ollama-specific functions (internal)
static char *make_ollama_request(const char *prompt);
static char *build_ollama_json_request(const char *prompt);
static char *parse_ollama_json_response(const char *json_str);
```

### Cache Functions
```c
// Store response in cache (works for both backends)
void ai_cache_response(const char *key, const char *response);

// Retrieve from cache
char *ai_cache_get(const char *key);

// Clear entire cache
void ai_cache_clear(void);

// Remove expired entries
void ai_cache_cleanup(void);
```

### Configuration Structure
```c
struct ai_config {
    char encrypted_api_key[256];  // OpenAI API key storage
    char model[64];              // OpenAI model name
    int max_tokens;              // Response length limit
    float temperature;           // Creativity (0.0-1.0)
    int timeout_ms;              // Request timeout
    bool content_filter_enabled; // Enable content filtering
    bool enabled;               // Global enable flag (OpenAI)
};

// Ollama configuration (hardcoded defaults)
#define OLLAMA_API_ENDPOINT "http://localhost:11434/api/generate"
#define OLLAMA_MODEL "llama3.2:1b"
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
6. **Memory**: Conversation history per player/NPC pair
7. **Moderation**: ai_moderate_content() implementation
8. **Dynamic Models**: Per-NPC model selection
9. **Ollama Config**: Runtime model switching

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

---

**Version**: 3.0  
**Last Updated**: January 2025  
**Maintainer**: LuminariMUD Development Team  
**Key Update**: Added Ollama LLM integration for always-on AI capability