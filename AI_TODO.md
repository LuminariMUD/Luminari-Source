# AI System TODO Checklist

## CRITICAL CONTEXT
 - Knowledge:
 AI_SERVICE_README.md
 OPEN_AI_TECH.md

 - Code:
 ai_cache.c
 ai_cache.h
 ai_events.c
 ai_events.h
 ai_security.c
 ai_security.h
 ai_service.c
 ai_service.h

## PERFORMANCE OPTIMIZATIONS (Added 2025-01-27)

### Immediate Quick Wins
1. **Remove/Reduce Artificial Delay** (ai_events.c:177)
   - Change `delay = strlen(response) / 20;` to `delay = 0;` or `delay = 1;`
   - Current delay adds 5 seconds on top of API response time

2. **Disable Debug Mode** (ai_service.h:47)
   - Set `AI_DEBUG_MODE` to 0
   - Extensive logging is causing performance overhead

3. **Switch to Faster Model**
   - Change from "gpt-4.1-mini" to "gpt-4o-mini" in .env
   - 80% cheaper and faster response times

4. **Lower Temperature Setting**
   - Set temperature to 0.3-0.5 instead of 0.7
   - Reduces response variability and speeds up generation

### CURL Optimizations
1. **Enable HTTP/2 and Keep-Alive** (ai_service.c after line 442)
   ```c
   curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
   curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
   curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
   curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
   ```

2. **Implement Connection Pooling**
   - Add persistent CURL handle to ai_service_state
   - Reuse handle instead of creating new one for each request
   - Reduces connection overhead

### Cache Improvements
1. **Replace Linear Search with Hash Table**
   - Current O(n) search causes delays with many entries
   - Implement proper hash table for O(1) lookups

2. **Increase Cache Size**
   - Change from 1000 to 5000 entries
   - Pre-warm cache with common phrases

3. **Add Database-Backed Cache**
   - Persist cache across server restarts
   - Use ai_content_cache table already in schema

### JSON Parsing Optimization
1. **Replace Character-by-Character Parser**
   - Current implementation in parse_json_response() is inefficient
   - Either use proper json-c library or optimize current algorithm
   - Avoid multiple string scans

### Event System Improvements
1. **Reduce Event Delay Calculation**
   - Change divisor from 20 to 100 in delay calculation
   - Cap maximum delay at 3 seconds instead of 5

2. **Implement Priority Queue**
   - High-priority responses (combat, urgent) bypass delays
   - Low-priority responses (casual chat) can wait

### Architecture Improvements
1. **Async Request Processing**
   - Move API calls to separate thread
   - Prevent blocking main game loop

2. **Batch API Requests**
   - Group multiple NPC responses together
   - Single API call for multiple interactions

3. **Response Streaming**
   - Use OpenAI streaming API for faster first-token delivery
   - Show partial responses as they arrive

