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

## IMPORTANT NOTES FROM README
- Current encryption is NOT real - just copies plaintext (critical security issue)
- API service is DISABLED by default - use 'ai enable' command
- Several features listed as "implemented" are actually incomplete

## Critical Bugs
- [x] Fix buffer overflow risk in JSON escaping (ai_service.c:427-432) - check buffer bounds properly
- [x] Fix buffer overflow risk in input sanitization (ai_security.c:109-133) - validate buffer limits
- [x] Add length validation for cache_key in ai_service.c:345 (max 256 bytes)
- [x] Fix missing include for sleep() - ai_service.c uses sleep() but doesn't include unistd.h
- [x] Fix thread safety issue - decrypt_api_key() returns static buffer that can be overwritten

## Memory Issues
- [x] Remove unnecessary strdup in build_json_request() (ai_service.c:450) - json_buffer is already static
- [x] Fix JSON parsing logic in parse_json_response() (ai_service.c:476) - doesn't handle escaped quotes
- [x] Add character validation in ai_events.c:53 to prevent accessing freed characters
- [x] Implement mechanism to cancel events if characters are freed

## Logic Bugs
- [x] Optimize cache cleanup algorithm from O(n²) to O(n) (ai_cache.c:151-165)
- [x] Replace blocking sleep() with non-blocking delay mechanism (ai_service.c:295)
- [x] Fix CURLOPT_TIMEOUT_MS cast to long potential overflow (ai_service.c:257)
- [ ] Replace sleep() with usleep() for millisecond precision in retry backoff

## Missing Error Handling
- [x] Check curl_global_init() return value (ai_service.c:82)
- [x] Add NULL checks before accessing pointers in critical paths
- [x] Handle memory allocation failures consistently throughout
- [x] Add validation for curl_easy_init() failures

## Unimplemented Features
- [ ] Implement ai_generate_room_desc() function (currently returns NULL)
- [ ] Implement ai_moderate_content() function (currently always returns TRUE)
- [ ] Add database logging for AI requests (ai_requests table exists but unused)
- [ ] Implement database-backed caching (ai_cache table exists but unused)
- [ ] Implement batch processing for multiple requests
- [ ] Add request priority system

## Performance Improvements
- [ ] Add connection pooling for CURL handles
- [ ] Implement async request queue instead of synchronous delays
- [ ] Cache frequently accessed values in loops
- [ ] Consider using json-c library for proper JSON handling

## Code Quality
- [x] Add comprehensive debug logging with AI_DEBUG_MODE
- [ ] Add comprehensive unit tests for all AI functions
- [ ] Improve event delay calculation (ai_events.c:87-89)
- [ ] Add proper documentation for missing function implementations
- [ ] Add integration tests with mock API responses
- [ ] Improve error messages and logging for better debugging
- [ ] Fix simplified JSON parsing that may fail with complex responses

## Configuration & Integration
- [ ] Add medit support for AI_ENABLED flag on NPCs
- [ ] Integrate AI configuration with config.c file-based system
- [ ] Add performance metrics tracking
- [ ] Implement usage monitoring and reporting