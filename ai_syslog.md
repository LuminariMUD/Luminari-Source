
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:93 in init_ai_service()]: Starting AI service initialization
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:95 in init_ai_service()]: AI state at start: initialized=0, config=(nil), limiter=(nil), cache_size=0
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:98 in init_ai_service()]: Initializing CURL library with CURL_GLOBAL_ALL flags
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:104 in init_ai_service()]: CURL library initialized successfully
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:107 in init_ai_service()]: Allocating AI configuration structure (size=304)
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:115 in init_ai_service()]: AI configuration allocated at 0x7529ee0
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:118 in init_ai_service()]: Setting default configuration values
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:120 in init_ai_service()]:   Model: gpt-4.1-mini
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:122 in init_ai_service()]:   Max tokens: 500
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:124 in init_ai_service()]:   Temperature: 0.70
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:126 in init_ai_service()]:   Timeout: 5000 ms
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:128 in init_ai_service()]:   Content filter: ENABLED
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:130 in init_ai_service()]:   Service enabled: NO
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:133 in init_ai_service()]: Allocating rate limiter structure (size=32)
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:143 in init_ai_service()]: Rate limiter allocated at 0x752a020
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:146 in init_ai_service()]: Initializing rate limiter with default values
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:148 in init_ai_service()]:   Requests per minute: 60
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:150 in init_ai_service()]:   Requests per hour: 1000
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:152 in init_ai_service()]:   Minute reset time: 1753647130
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:154 in init_ai_service()]:   Hour reset time: 1753650670
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:158 in init_ai_service()]:   Current counts: minute=0, hour=0
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:161 in init_ai_service()]: Initializing AI response cache
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:165 in init_ai_service()]:   Cache initialized: head=(nil), size=0, max_size=1000
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:168 in init_ai_service()]: Loading AI configuration from .env file
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:250 in load_ai_config()]: Loading AI configuration from environment
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:259 in load_ai_config()]: Looking for OPENAI_API_KEY in environment
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:262 in load_ai_config()]:   Found API key (length=164)
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:263 in load_ai_config()]:   First 10 chars: sk-proj-wy...
Jul 27 20:11:10 :: AI_DEBUG [ai_security.c:44 in encrypt_api_key()]: encrypt_api_key() called
Jul 27 20:11:10 :: AI_DEBUG [ai_security.c:51 in encrypt_api_key()]: WARNING: Using plaintext storage (no encryption)
Jul 27 20:11:10 :: AI_DEBUG [ai_security.c:52 in encrypt_api_key()]: Input length: 164
Jul 27 20:11:10 :: AI_DEBUG [ai_security.c:57 in encrypt_api_key()]: API key stored (first 10 chars): sk-proj-wy...
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:266 in load_ai_config()]:   API key encrypted and stored
Jul 27 20:11:10 :: AI Service: API key loaded from .env file
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:274 in load_ai_config()]: Looking for AI_MODEL in environment
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:277 in load_ai_config()]:   Found model: gpt-4.1-mini
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:284 in load_ai_config()]: Loading numeric configurations
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:287 in load_ai_config()]:   Max tokens: 500 (env: set)
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:291 in load_ai_config()]:   Temperature: 0.00 (env: set)
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:295 in load_ai_config()]:   Raw timeout value: 5000 ms
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:306 in load_ai_config()]:   Final timeout: 5000 ms
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:311 in load_ai_config()]:   Content filter: ENABLED (env: set)
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:315 in load_ai_config()]: Loading rate limit configuration
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:318 in load_ai_config()]:   Requests per minute: 60 (env: set)
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:321 in load_ai_config()]:   Requests per hour: 1000 (env: set)
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:326 in load_ai_config()]: AI configuration loading complete
Jul 27 20:11:10 :: AI configuration loaded from .env
Jul 27 20:11:10 :: AI_DEBUG [ai_service.c:172 in init_ai_service()]: AI Service initialization complete - initialized=1
Jul 27 20:11:10 :: AI Service initialized successfully.
Jul 27 20:11:10 :: Zusuk enabled AI service.
Jul 27 20:11:15 :: AI_DEBUG [ai_cache.c:115 in ai_cache_get()]: ai_cache_get() called with key='npc_1218_Hi.'
Jul 27 20:11:15 :: AI_DEBUG [ai_cache.c:122 in ai_cache_get()]: Searching cache (size=0)
Jul 27 20:11:15 :: AI_DEBUG [ai_cache.c:142 in ai_cache_get()]: Cache miss - no valid entry found
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:532 in ai_generate_response()]: ai_generate_response() called with request_type=1
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:534 in ai_generate_response()]: Original prompt: 'You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep...'
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:239 in is_ai_enabled()]: Checking if AI enabled: initialized=1, config=0x7529ee0, enabled=1, result=1
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:549 in ai_generate_response()]: Sanitizing input prompt
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:552 in ai_generate_response()]: Sanitized prompt: 'You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep...'
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:555 in ai_generate_response()]: Making API request
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:502 in make_api_request()]: make_api_request() called with max retries: 3
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:505 in make_api_request()]: Attempt 1/3
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:345 in make_api_request_single()]: make_api_request_single() called with prompt: 'You are a janni bladesman in a fantasy RPG world. ...'
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:354 in make_api_request_single()]: Checking rate limits
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:671 in ai_check_rate_limit()]: Checking rate limits at time 1753647075
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:680 in ai_check_rate_limit()]: Current counts: minute=0/60, hour=0/1000
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:710 in ai_check_rate_limit()]: Rate limit check passed, new counts: minute=1, hour=1
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:360 in make_api_request_single()]: Rate limit check passed
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:363 in make_api_request_single()]: Decrypting API key
Jul 27 20:11:15 :: AI_DEBUG [ai_security.c:68 in decrypt_api_key()]: decrypt_api_key() called
Jul 27 20:11:15 :: AI_DEBUG [ai_security.c:75 in decrypt_api_key()]: WARNING: Using plaintext retrieval (no decryption)
Jul 27 20:11:15 :: AI_DEBUG [ai_security.c:76 in decrypt_api_key()]: Encrypted length: 164
Jul 27 20:11:15 :: AI_DEBUG [ai_security.c:89 in decrypt_api_key()]: API key retrieved (first 10 chars): sk-proj-wy...
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:370 in make_api_request_single()]: API key decrypted successfully (length=164)
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:373 in make_api_request_single()]: Building JSON request
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:737 in build_json_request()]: build_json_request() called
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:738 in build_json_request()]: Input prompt length: 147
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:748 in build_json_request()]: Escaping JSON special characters
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:771 in build_json_request()]: Escaping complete: 4 characters escaped, final length: 151
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:774 in build_json_request()]: Building JSON request structure
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:775 in build_json_request()]:   Model: gpt-4.1-mini
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:776 in build_json_request()]:   Max tokens: 500
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:777 in build_json_request()]:   Temperature: 0.00
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:793 in build_json_request()]: JSON request built, total length: 260
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:795 in build_json_request()]: JSON structure: {"model": "gpt-4.1-mini","messages": [{"role": "user", "content": "You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep response under 100 words. Player...
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:380 in make_api_request_single()]: JSON request built (length=260)
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:382 in make_api_request_single()]: JSON content: {"model": "gpt-4.1-mini","messages": [{"role": "user", "content": "You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep response under 100 words. Player...
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:384 in make_api_request_single()]: Initializing CURL handle
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:394 in make_api_request_single()]: CURL handle initialized at 0x2836d430
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:397 in make_api_request_single()]: Setting up HTTP headers
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:399 in make_api_request_single()]:   Auth header length: 186
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:408 in make_api_request_single()]:   Auth header added successfully
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:418 in make_api_request_single()]:   Content-Type header added successfully
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:421 in make_api_request_single()]: Configuring CURL options
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:423 in make_api_request_single()]:   URL: https://api.openai.com/v1/chat/completions
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:425 in make_api_request_single()]:   Headers set
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:427 in make_api_request_single()]:   POST data length: 260 bytes
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:429 in make_api_request_single()]:   Write callback set to ai_curl_write_callback
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:431 in make_api_request_single()]:   Write data pointer set to response buffer
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:438 in make_api_request_single()]:   Timeout set to 5000 ms
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:442 in make_api_request_single()]:   SSL verification enabled (peer=1, host=2)
Jul 27 20:11:15 :: AI_DEBUG [ai_service.c:445 in make_api_request_single()]: Executing CURL request to API endpoint
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:52 in ai_curl_write_callback()]: CURL callback received 996 bytes (size=1, nmemb=996)
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:53 in ai_curl_write_callback()]: Current buffer size: 0 bytes
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:67 in ai_curl_write_callback()]: Buffer successfully expanded to 996 bytes
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:69 in ai_curl_write_callback()]: Response chunk: {
  "id": "chatcmpl-By20B0X2zLi1UWGOgsuWSEAkNMMGv",
  "object": "chat.completion",
  "created": 1753...
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:447 in make_api_request_single()]: CURL request completed with result: 0 (No error)
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:452 in make_api_request_single()]: HTTP response code: 200
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:453 in make_api_request_single()]: Response size: 996 bytes
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:456 in make_api_request_single()]: Success! Parsing JSON response
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:458 in make_api_request_single()]: Raw response: {
  "id": "chatcmpl-By20B0X2zLi1UWGOgsuWSEAkNMMGv",
  "object": "chat.completion",
  "created": 1753647075,
  "model": "gpt-4.1-mini-2025-04-14",
  "choices": [
    {
      "index": 0,
      "message": {
        "role": "assistant",
        "content": "Greetings, traveler! The desert winds whisper your arrival. What brings you to these sun-scorched sands? Are you seeking wisdom, fortune, or perhaps a blade’s edge to carve your destiny?",
        "refusal": null,
        "annotations": []
     ...
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:809 in parse_json_response()]: parse_json_response() called
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:810 in parse_json_response()]: JSON input length: 996
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:819 in parse_json_response()]: Searching for content field in JSON
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:828 in parse_json_response()]: Found 'content' field at position 241
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:829 in parse_json_response()]: Context around content field: "content": "Greetings, traveler! The desert winds whisper your arrival. What brings you to these sun
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:833 in parse_json_response()]: After skipping 'content:' label, position 251
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:834 in parse_json_response()]: Character at position: ' ' (ASCII 32)
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:842 in parse_json_response()]: Skipped 1 whitespace characters
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:851 in parse_json_response()]: Content value starts at position 253
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:852 in parse_json_response()]: First 50 chars of content: Greetings, traveler! The desert winds whisper your
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:855 in parse_json_response()]: Searching for closing quote with escape handling
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:870 in parse_json_response()]:   Found quote #1 at position 188 with 0 preceding backslashes
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:873 in parse_json_response()]:   This is the closing quote (unescaped)
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:894 in parse_json_response()]: Found closing quote at position 188
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:898 in parse_json_response()]: Extracting content of length 188
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:899 in parse_json_response()]: Raw content before unescaping: Greetings, traveler! The desert winds whisper your arrival. What brings you to these sun-scorched sa...
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:907 in parse_json_response()]: Allocated 189 bytes for result at 0xa35e8b0
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:910 in parse_json_response()]: Starting content unescaping process
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:941 in parse_json_response()]: Unescaping complete: processed 188 chars, found 0 escape sequences
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:942 in parse_json_response()]: Final content length: 188
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:943 in parse_json_response()]: Content preview: Greetings, traveler! The desert winds whisper your arrival. What brings you to these sun-scorched sa...
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:944 in parse_json_response()]: Full content: Greetings, traveler! The desert winds whisper your arrival. What brings you to these sun-scorched sands? Are you seeking wisdom, fortune, or perhaps a blade’s edge to carve your destiny?
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:461 in make_api_request_single()]: JSON parsing successful, result length: 188
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:476 in make_api_request_single()]: Cleaning up CURL resources
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:478 in make_api_request_single()]:   CURL handle cleaned up
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:480 in make_api_request_single()]:   Headers freed
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:483 in make_api_request_single()]:   Freeing API key
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:487 in make_api_request_single()]:   Freeing response data (996 bytes)
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:491 in make_api_request_single()]: make_api_request_single() returning success
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:508 in make_api_request()]: Request succeeded on attempt 1
Jul 27 20:11:16 :: AI_DEBUG [ai_service.c:564 in ai_generate_response()]: Response received (length=188)
Jul 27 20:11:16 :: AI_DEBUG [ai_cache.c:30 in ai_cache_response()]: ai_cache_response() called with key='npc_1218_Hi.', response_len=188
Jul 27 20:11:16 :: AI_DEBUG [ai_cache.c:38 in ai_cache_response()]: Searching for existing cache entry
Jul 27 20:11:16 :: AI_DEBUG [ai_cache.c:60 in ai_cache_response()]: No existing entry found, creating new one
Jul 27 20:11:16 :: AI_DEBUG [ai_cache.c:63 in ai_cache_response()]: Creating new cache entry
Jul 27 20:11:16 :: AI_DEBUG [ai_cache.c:78 in ai_cache_response()]:   Key stored: 'npc_1218_Hi.'
Jul 27 20:11:16 :: AI_DEBUG [ai_cache.c:88 in ai_cache_response()]:   Response stored (length=188)
Jul 27 20:11:16 :: AI_DEBUG [ai_cache.c:92 in ai_cache_response()]:   Entry expires at 1753650676 (in 3600 seconds)
Jul 27 20:11:16 :: AI_DEBUG [ai_cache.c:99 in ai_cache_response()]: Cache entry added, new cache size: 1/1000
Jul 27 20:11:16 :: AI_DEBUG [ai_events.c:130 in queue_ai_response()]: queue_ai_response() called: ch=0x2834e620, npc=0x28368e40, response_len=188
Jul 27 20:11:16 :: AI_DEBUG [ai_events.c:138 in queue_ai_response()]: Validating character pointers
Jul 27 20:11:16 :: AI_DEBUG [ai_events.c:146 in queue_ai_response()]: Validation result: ch_valid=YES, npc_valid=YES
Jul 27 20:11:16 :: AI_DEBUG [ai_events.c:155 in queue_ai_response()]: Creating AI response event data
Jul 27 20:11:16 :: AI_DEBUG [ai_events.c:162 in queue_ai_response()]: Event data allocated at 0x6b209c0
Jul 27 20:11:16 :: AI_DEBUG [ai_events.c:173 in queue_ai_response()]: Response duplicated (length=188)
Jul 27 20:11:16 :: AI_DEBUG [ai_events.c:182 in queue_ai_response()]: Calculated delay: 5 seconds (response_len=188)
Jul 27 20:11:16 :: AI_DEBUG [ai_events.c:186 in queue_ai_response()]: Creating event with 5 second delay (50 pulses)
Jul 27 20:11:16 :: AI_DEBUG [ai_events.c:188 in queue_ai_response()]: AI response event queued successfully
Jul 27 20:11:16 :: PERFMON: High load detected (771.5%), enabling full monitoring
Jul 27 20:11:16 :: Pulse usage new high water mark [771.53%, 771527 usec]. Trace info: 
Pulse profiling info


Jul 27 20:11:16 :: PERFMON: Load normalized (3.9%), returning to sampling mode
Jul 27 20:11:20 :: AI_DEBUG [ai_events.c:50 in ai_response_event()]: ai_response_event() triggered
Jul 27 20:11:20 :: AI_DEBUG [ai_events.c:60 in ai_response_event()]: Validating event data: ch=0x2834e620, npc=0x28368e40, response=0x66ea130
Jul 27 20:11:20 :: AI_DEBUG [ai_events.c:73 in ai_response_event()]: Checking if characters still exist in game
Jul 27 20:11:20 :: AI_DEBUG [ai_events.c:84 in ai_response_event()]: Character validation: ch_found=YES, npc_found=YES
Jul 27 20:11:20 :: AI_DEBUG [ai_events.c:96 in ai_response_event()]: Checking room locations: ch_room=845, npc_room=845
Jul 27 20:11:20 :: AI_DEBUG [ai_events.c:101 in ai_response_event()]: Characters in same room, delivering response
Jul 27 20:11:20 :: AI: Zusuk -> a janni bladesman: Greetings, traveler! The desert winds whisper your arrival. What brings you to these sun-scorched sands? Are you seeking wisdom, fortune, or perhaps a blade’s edge to carve your destiny?
Jul 27 20:11:20 :: AI_DEBUG [ai_events.c:108 in ai_response_event()]: Response delivered successfully
Jul 27 20:11:20 :: AI_DEBUG [ai_events.c:114 in ai_response_event()]: Cleaning up event data
Jul 27 20:11:21 :: AI_DEBUG [ai_cache.c:115 in ai_cache_get()]: ai_cache_get() called with key='npc_1218_Where are we?'
Jul 27 20:11:21 :: AI_DEBUG [ai_cache.c:122 in ai_cache_get()]: Searching cache (size=1)
Jul 27 20:11:21 :: AI_DEBUG [ai_cache.c:142 in ai_cache_get()]: Cache miss - no valid entry found
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:532 in ai_generate_response()]: ai_generate_response() called with request_type=1
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:534 in ai_generate_response()]: Original prompt: 'You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep...'
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:239 in is_ai_enabled()]: Checking if AI enabled: initialized=1, config=0x7529ee0, enabled=1, result=1
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:549 in ai_generate_response()]: Sanitizing input prompt
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:552 in ai_generate_response()]: Sanitized prompt: 'You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep...'
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:555 in ai_generate_response()]: Making API request
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:502 in make_api_request()]: make_api_request() called with max retries: 3
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:505 in make_api_request()]: Attempt 1/3
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:345 in make_api_request_single()]: make_api_request_single() called with prompt: 'You are a janni bladesman in a fantasy RPG world. ...'
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:354 in make_api_request_single()]: Checking rate limits
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:671 in ai_check_rate_limit()]: Checking rate limits at time 1753647081
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:680 in ai_check_rate_limit()]: Current counts: minute=1/60, hour=1/1000
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:710 in ai_check_rate_limit()]: Rate limit check passed, new counts: minute=2, hour=2
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:360 in make_api_request_single()]: Rate limit check passed
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:363 in make_api_request_single()]: Decrypting API key
Jul 27 20:11:21 :: AI_DEBUG [ai_security.c:68 in decrypt_api_key()]: decrypt_api_key() called
Jul 27 20:11:21 :: AI_DEBUG [ai_security.c:75 in decrypt_api_key()]: WARNING: Using plaintext retrieval (no decryption)
Jul 27 20:11:21 :: AI_DEBUG [ai_security.c:76 in decrypt_api_key()]: Encrypted length: 164
Jul 27 20:11:21 :: AI_DEBUG [ai_security.c:89 in decrypt_api_key()]: API key retrieved (first 10 chars): sk-proj-wy...
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:370 in make_api_request_single()]: API key decrypted successfully (length=164)
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:373 in make_api_request_single()]: Building JSON request
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:737 in build_json_request()]: build_json_request() called
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:738 in build_json_request()]: Input prompt length: 157
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:748 in build_json_request()]: Escaping JSON special characters
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:771 in build_json_request()]: Escaping complete: 4 characters escaped, final length: 161
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:774 in build_json_request()]: Building JSON request structure
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:775 in build_json_request()]:   Model: gpt-4.1-mini
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:776 in build_json_request()]:   Max tokens: 500
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:777 in build_json_request()]:   Temperature: 0.00
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:793 in build_json_request()]: JSON request built, total length: 270
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:795 in build_json_request()]: JSON structure: {"model": "gpt-4.1-mini","messages": [{"role": "user", "content": "You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep response under 100 words. Player...
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:380 in make_api_request_single()]: JSON request built (length=270)
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:382 in make_api_request_single()]: JSON content: {"model": "gpt-4.1-mini","messages": [{"role": "user", "content": "You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep response under 100 words. Player...
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:384 in make_api_request_single()]: Initializing CURL handle
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:394 in make_api_request_single()]: CURL handle initialized at 0x2836d430
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:397 in make_api_request_single()]: Setting up HTTP headers
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:399 in make_api_request_single()]:   Auth header length: 186
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:408 in make_api_request_single()]:   Auth header added successfully
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:418 in make_api_request_single()]:   Content-Type header added successfully
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:421 in make_api_request_single()]: Configuring CURL options
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:423 in make_api_request_single()]:   URL: https://api.openai.com/v1/chat/completions
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:425 in make_api_request_single()]:   Headers set
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:427 in make_api_request_single()]:   POST data length: 270 bytes
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:429 in make_api_request_single()]:   Write callback set to ai_curl_write_callback
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:431 in make_api_request_single()]:   Write data pointer set to response buffer
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:438 in make_api_request_single()]:   Timeout set to 5000 ms
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:442 in make_api_request_single()]:   SSL verification enabled (peer=1, host=2)
Jul 27 20:11:21 :: AI_DEBUG [ai_service.c:445 in make_api_request_single()]: Executing CURL request to API endpoint
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:52 in ai_curl_write_callback()]: CURL callback received 1111 bytes (size=1, nmemb=1111)
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:53 in ai_curl_write_callback()]: Current buffer size: 0 bytes
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:67 in ai_curl_write_callback()]: Buffer successfully expanded to 1111 bytes
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:69 in ai_curl_write_callback()]: Response chunk: {
  "id": "chatcmpl-By20HzYoGS86Xl877ur5agYyr5RcV",
  "object": "chat.completion",
  "created": 1753...
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:447 in make_api_request_single()]: CURL request completed with result: 0 (No error)
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:452 in make_api_request_single()]: HTTP response code: 200
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:453 in make_api_request_single()]: Response size: 1111 bytes
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:456 in make_api_request_single()]: Success! Parsing JSON response
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:458 in make_api_request_single()]: Raw response: {
  "id": "chatcmpl-By20HzYoGS86Xl877ur5agYyr5RcV",
  "object": "chat.completion",
  "created": 1753647081,
  "model": "gpt-4.1-mini-2025-04-14",
  "choices": [
    {
      "index": 0,
      "message": {
        "role": "assistant",
        "content": "We stand at the edge of the Whispering Dunes, traveler. The sands here shift like the tides, hiding secrets older than the stars. Beyond lies the lost city of Zharim, said to be guarded by ancient spirits. Tread carefully—these lands test both c...
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:809 in parse_json_response()]: parse_json_response() called
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:810 in parse_json_response()]: JSON input length: 1111
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:819 in parse_json_response()]: Searching for content field in JSON
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:828 in parse_json_response()]: Found 'content' field at position 241
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:829 in parse_json_response()]: Context around content field: "content": "We stand at the edge of the Whispering Dunes, traveler. The sands here shift like the ti
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:833 in parse_json_response()]: After skipping 'content:' label, position 251
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:834 in parse_json_response()]: Character at position: ' ' (ASCII 32)
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:842 in parse_json_response()]: Skipped 1 whitespace characters
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:851 in parse_json_response()]: Content value starts at position 253
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:852 in parse_json_response()]: First 50 chars of content: We stand at the edge of the Whispering Dunes, trav
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:855 in parse_json_response()]: Searching for closing quote with escape handling
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:870 in parse_json_response()]:   Found quote #1 at position 302 with 0 preceding backslashes
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:873 in parse_json_response()]:   This is the closing quote (unescaped)
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:894 in parse_json_response()]: Found closing quote at position 302
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:898 in parse_json_response()]: Extracting content of length 302
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:899 in parse_json_response()]: Raw content before unescaping: We stand at the edge of the Whispering Dunes, traveler. The sands here shift like the tides, hiding ...
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:907 in parse_json_response()]: Allocated 303 bytes for result at 0x22d27d10
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:910 in parse_json_response()]: Starting content unescaping process
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:941 in parse_json_response()]: Unescaping complete: processed 302 chars, found 0 escape sequences
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:942 in parse_json_response()]: Final content length: 302
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:943 in parse_json_response()]: Content preview: We stand at the edge of the Whispering Dunes, traveler. The sands here shift like the tides, hiding ...
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:944 in parse_json_response()]: Full content: We stand at the edge of the Whispering Dunes, traveler. The sands here shift like the tides, hiding secrets older than the stars. Beyond lies the lost city of Zharim, said to be guarded by ancient spirits. Tread carefully—these lands test both courage and wit. What brings you to this forsaken place?
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:461 in make_api_request_single()]: JSON parsing successful, result length: 302
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:476 in make_api_request_single()]: Cleaning up CURL resources
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:478 in make_api_request_single()]:   CURL handle cleaned up
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:480 in make_api_request_single()]:   Headers freed
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:483 in make_api_request_single()]:   Freeing API key
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:487 in make_api_request_single()]:   Freeing response data (1111 bytes)
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:491 in make_api_request_single()]: make_api_request_single() returning success
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:508 in make_api_request()]: Request succeeded on attempt 1
Jul 27 20:11:22 :: AI_DEBUG [ai_service.c:564 in ai_generate_response()]: Response received (length=302)
Jul 27 20:11:22 :: AI_DEBUG [ai_cache.c:30 in ai_cache_response()]: ai_cache_response() called with key='npc_1218_Where are we?', response_len=302
Jul 27 20:11:22 :: AI_DEBUG [ai_cache.c:38 in ai_cache_response()]: Searching for existing cache entry
Jul 27 20:11:22 :: AI_DEBUG [ai_cache.c:60 in ai_cache_response()]: No existing entry found, creating new one
Jul 27 20:11:22 :: AI_DEBUG [ai_cache.c:63 in ai_cache_response()]: Creating new cache entry
Jul 27 20:11:22 :: AI_DEBUG [ai_cache.c:78 in ai_cache_response()]:   Key stored: 'npc_1218_Where are we?'
Jul 27 20:11:22 :: AI_DEBUG [ai_cache.c:88 in ai_cache_response()]:   Response stored (length=302)
Jul 27 20:11:22 :: AI_DEBUG [ai_cache.c:92 in ai_cache_response()]:   Entry expires at 1753650682 (in 3600 seconds)
Jul 27 20:11:22 :: AI_DEBUG [ai_cache.c:99 in ai_cache_response()]: Cache entry added, new cache size: 2/1000
Jul 27 20:11:22 :: AI_DEBUG [ai_events.c:130 in queue_ai_response()]: queue_ai_response() called: ch=0x2834e620, npc=0x28368e40, response_len=302
Jul 27 20:11:22 :: AI_DEBUG [ai_events.c:138 in queue_ai_response()]: Validating character pointers
Jul 27 20:11:22 :: AI_DEBUG [ai_events.c:146 in queue_ai_response()]: Validation result: ch_valid=YES, npc_valid=YES
Jul 27 20:11:22 :: AI_DEBUG [ai_events.c:155 in queue_ai_response()]: Creating AI response event data
Jul 27 20:11:22 :: AI_DEBUG [ai_events.c:162 in queue_ai_response()]: Event data allocated at 0x609d830
Jul 27 20:11:22 :: AI_DEBUG [ai_events.c:173 in queue_ai_response()]: Response duplicated (length=302)
Jul 27 20:11:22 :: AI_DEBUG [ai_events.c:182 in queue_ai_response()]: Calculated delay: 5 seconds (response_len=302)
Jul 27 20:11:22 :: AI_DEBUG [ai_events.c:186 in queue_ai_response()]: Creating event with 5 second delay (50 pulses)
Jul 27 20:11:22 :: AI_DEBUG [ai_events.c:188 in queue_ai_response()]: AI response event queued successfully
Jul 27 20:11:22 :: PERFMON: High load detected (839.0%), enabling full monitoring
Jul 27 20:11:22 :: Pulse usage new high water mark [838.98%, 838983 usec]. Trace info: 
Pulse profiling info


Jul 27 20:11:22 :: PERFMON: Load normalized (13.2%), returning to sampling mode
Jul 27 20:11:26 :: AI_DEBUG [ai_events.c:50 in ai_response_event()]: ai_response_event() triggered
Jul 27 20:11:26 :: AI_DEBUG [ai_events.c:60 in ai_response_event()]: Validating event data: ch=0x2834e620, npc=0x28368e40, response=0xbd2fa00
Jul 27 20:11:26 :: AI_DEBUG [ai_events.c:73 in ai_response_event()]: Checking if characters still exist in game
Jul 27 20:11:26 :: AI_DEBUG [ai_events.c:84 in ai_response_event()]: Character validation: ch_found=YES, npc_found=YES
Jul 27 20:11:26 :: AI_DEBUG [ai_events.c:96 in ai_response_event()]: Checking room locations: ch_room=845, npc_room=845
Jul 27 20:11:26 :: AI_DEBUG [ai_events.c:101 in ai_response_event()]: Characters in same room, delivering response
Jul 27 20:11:26 :: AI: Zusuk -> a janni bladesman: We stand at the edge of the Whispering Dunes, traveler. The sands here shift like the tides, hiding secrets older than the stars. Beyond lies the lost city of Zharim, said to be guarded by ancient spirits. Tread carefully—these lands test both courage and wit. What brings you to this forsaken place?
Jul 27 20:11:26 :: AI_DEBUG [ai_events.c:108 in ai_response_event()]: Response delivered successfully
Jul 27 20:11:26 :: AI_DEBUG [ai_events.c:114 in ai_response_event()]: Cleaning up event data
Jul 27 20:11:27 :: AI_DEBUG [ai_cache.c:115 in ai_cache_get()]: ai_cache_get() called with key='npc_1218_Do you have a quest?'
Jul 27 20:11:27 :: AI_DEBUG [ai_cache.c:122 in ai_cache_get()]: Searching cache (size=2)
Jul 27 20:11:27 :: AI_DEBUG [ai_cache.c:142 in ai_cache_get()]: Cache miss - no valid entry found
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:532 in ai_generate_response()]: ai_generate_response() called with request_type=1
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:534 in ai_generate_response()]: Original prompt: 'You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep...'
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:239 in is_ai_enabled()]: Checking if AI enabled: initialized=1, config=0x7529ee0, enabled=1, result=1
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:549 in ai_generate_response()]: Sanitizing input prompt
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:552 in ai_generate_response()]: Sanitized prompt: 'You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep...'
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:555 in ai_generate_response()]: Making API request
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:502 in make_api_request()]: make_api_request() called with max retries: 3
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:505 in make_api_request()]: Attempt 1/3
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:345 in make_api_request_single()]: make_api_request_single() called with prompt: 'You are a janni bladesman in a fantasy RPG world. ...'
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:354 in make_api_request_single()]: Checking rate limits
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:671 in ai_check_rate_limit()]: Checking rate limits at time 1753647087
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:680 in ai_check_rate_limit()]: Current counts: minute=2/60, hour=2/1000
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:710 in ai_check_rate_limit()]: Rate limit check passed, new counts: minute=3, hour=3
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:360 in make_api_request_single()]: Rate limit check passed
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:363 in make_api_request_single()]: Decrypting API key
Jul 27 20:11:27 :: AI_DEBUG [ai_security.c:68 in decrypt_api_key()]: decrypt_api_key() called
Jul 27 20:11:27 :: AI_DEBUG [ai_security.c:75 in decrypt_api_key()]: WARNING: Using plaintext retrieval (no decryption)
Jul 27 20:11:27 :: AI_DEBUG [ai_security.c:76 in decrypt_api_key()]: Encrypted length: 164
Jul 27 20:11:27 :: AI_DEBUG [ai_security.c:89 in decrypt_api_key()]: API key retrieved (first 10 chars): sk-proj-wy...
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:370 in make_api_request_single()]: API key decrypted successfully (length=164)
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:373 in make_api_request_single()]: Building JSON request
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:737 in build_json_request()]: build_json_request() called
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:738 in build_json_request()]: Input prompt length: 164
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:748 in build_json_request()]: Escaping JSON special characters
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:771 in build_json_request()]: Escaping complete: 4 characters escaped, final length: 168
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:774 in build_json_request()]: Building JSON request structure
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:775 in build_json_request()]:   Model: gpt-4.1-mini
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:776 in build_json_request()]:   Max tokens: 500
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:777 in build_json_request()]:   Temperature: 0.00
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:793 in build_json_request()]: JSON request built, total length: 277
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:795 in build_json_request()]: JSON structure: {"model": "gpt-4.1-mini","messages": [{"role": "user", "content": "You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep response under 100 words. Player...
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:380 in make_api_request_single()]: JSON request built (length=277)
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:382 in make_api_request_single()]: JSON content: {"model": "gpt-4.1-mini","messages": [{"role": "user", "content": "You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep response under 100 words. Player...
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:384 in make_api_request_single()]: Initializing CURL handle
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:394 in make_api_request_single()]: CURL handle initialized at 0x2836d430
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:397 in make_api_request_single()]: Setting up HTTP headers
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:399 in make_api_request_single()]:   Auth header length: 186
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:408 in make_api_request_single()]:   Auth header added successfully
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:418 in make_api_request_single()]:   Content-Type header added successfully
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:421 in make_api_request_single()]: Configuring CURL options
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:423 in make_api_request_single()]:   URL: https://api.openai.com/v1/chat/completions
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:425 in make_api_request_single()]:   Headers set
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:427 in make_api_request_single()]:   POST data length: 277 bytes
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:429 in make_api_request_single()]:   Write callback set to ai_curl_write_callback
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:431 in make_api_request_single()]:   Write data pointer set to response buffer
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:438 in make_api_request_single()]:   Timeout set to 5000 ms
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:442 in make_api_request_single()]:   SSL verification enabled (peer=1, host=2)
Jul 27 20:11:27 :: AI_DEBUG [ai_service.c:445 in make_api_request_single()]: Executing CURL request to API endpoint
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:52 in ai_curl_write_callback()]: CURL callback received 75 bytes (size=1, nmemb=75)
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:53 in ai_curl_write_callback()]: Current buffer size: 0 bytes
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:67 in ai_curl_write_callback()]: Buffer successfully expanded to 75 bytes
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:69 in ai_curl_write_callback()]: Response chunk: {
  "id": "chatcmpl-By20OnMqaEYP112sJoFulZass46yp",
  "object": "chat.compl
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:52 in ai_curl_write_callback()]: CURL callback received 1068 bytes (size=1, nmemb=1068)
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:53 in ai_curl_write_callback()]: Current buffer size: 75 bytes
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:67 in ai_curl_write_callback()]: Buffer successfully expanded to 1143 bytes
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:69 in ai_curl_write_callback()]: Response chunk: etion",
  "created": 1753647088,
  "model": "gpt-4.1-mini-2025-04-14",
  "choices": [
    {
      "i...
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:447 in make_api_request_single()]: CURL request completed with result: 0 (No error)
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:452 in make_api_request_single()]: HTTP response code: 200
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:453 in make_api_request_single()]: Response size: 1143 bytes
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:456 in make_api_request_single()]: Success! Parsing JSON response
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:458 in make_api_request_single()]: Raw response: {
  "id": "chatcmpl-By20OnMqaEYP112sJoFulZass46yp",
  "object": "chat.completion",
  "created": 1753647088,
  "model": "gpt-4.1-mini-2025-04-14",
  "choices": [
    {
      "index": 0,
      "message": {
        "role": "assistant",
        "content": "Aye, traveler! The desert winds whisper of a lost relic buried beneath the shifting sands—a blade said to harness the storm’s fury. Retrieve it, and you’ll earn the favor of the Janni and the blessings of the desert spirits. But beware, for ...
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:809 in parse_json_response()]: parse_json_response() called
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:810 in parse_json_response()]: JSON input length: 1143
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:819 in parse_json_response()]: Searching for content field in JSON
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:828 in parse_json_response()]: Found 'content' field at position 241
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:829 in parse_json_response()]: Context around content field: "content": "Aye, traveler! The desert winds whisper of a lost relic buried beneath the shifting sand
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:833 in parse_json_response()]: After skipping 'content:' label, position 251
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:834 in parse_json_response()]: Character at position: ' ' (ASCII 32)
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:842 in parse_json_response()]: Skipped 1 whitespace characters
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:851 in parse_json_response()]: Content value starts at position 253
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:852 in parse_json_response()]: First 50 chars of content: Aye, traveler! The desert winds whisper of a lost 
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:855 in parse_json_response()]: Searching for closing quote with escape handling
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:870 in parse_json_response()]:   Found quote #1 at position 334 with 0 preceding backslashes
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:873 in parse_json_response()]:   This is the closing quote (unescaped)
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:894 in parse_json_response()]: Found closing quote at position 334
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:898 in parse_json_response()]: Extracting content of length 334
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:899 in parse_json_response()]: Raw content before unescaping: Aye, traveler! The desert winds whisper of a lost relic buried beneath the shifting sands—a blade ...
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:907 in parse_json_response()]: Allocated 335 bytes for result at 0x1821bb40
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:910 in parse_json_response()]: Starting content unescaping process
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:941 in parse_json_response()]: Unescaping complete: processed 334 chars, found 0 escape sequences
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:942 in parse_json_response()]: Final content length: 334
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:943 in parse_json_response()]: Content preview: Aye, traveler! The desert winds whisper of a lost relic buried beneath the shifting sands—a blade ...
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:944 in parse_json_response()]: Full content: Aye, traveler! The desert winds whisper of a lost relic buried beneath the shifting sands—a blade said to harness the storm’s fury. Retrieve it, and you’ll earn the favor of the Janni and the blessings of the desert spirits. But beware, for sand wraiths guard its resting place. Are you ready to face the scorching trials ahead?
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:461 in make_api_request_single()]: JSON parsing successful, result length: 334
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:476 in make_api_request_single()]: Cleaning up CURL resources
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:478 in make_api_request_single()]:   CURL handle cleaned up
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:480 in make_api_request_single()]:   Headers freed
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:483 in make_api_request_single()]:   Freeing API key
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:487 in make_api_request_single()]:   Freeing response data (1143 bytes)
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:491 in make_api_request_single()]: make_api_request_single() returning success
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:508 in make_api_request()]: Request succeeded on attempt 1
Jul 27 20:11:29 :: AI_DEBUG [ai_service.c:564 in ai_generate_response()]: Response received (length=334)
Jul 27 20:11:29 :: AI_DEBUG [ai_cache.c:30 in ai_cache_response()]: ai_cache_response() called with key='npc_1218_Do you have a quest?', response_len=334
Jul 27 20:11:29 :: AI_DEBUG [ai_cache.c:38 in ai_cache_response()]: Searching for existing cache entry
Jul 27 20:11:29 :: AI_DEBUG [ai_cache.c:60 in ai_cache_response()]: No existing entry found, creating new one
Jul 27 20:11:29 :: AI_DEBUG [ai_cache.c:63 in ai_cache_response()]: Creating new cache entry
Jul 27 20:11:29 :: AI_DEBUG [ai_cache.c:78 in ai_cache_response()]:   Key stored: 'npc_1218_Do you have a quest?'
Jul 27 20:11:29 :: AI_DEBUG [ai_cache.c:88 in ai_cache_response()]:   Response stored (length=334)
Jul 27 20:11:29 :: AI_DEBUG [ai_cache.c:92 in ai_cache_response()]:   Entry expires at 1753650689 (in 3600 seconds)
Jul 27 20:11:29 :: AI_DEBUG [ai_cache.c:99 in ai_cache_response()]: Cache entry added, new cache size: 3/1000
Jul 27 20:11:29 :: AI_DEBUG [ai_events.c:130 in queue_ai_response()]: queue_ai_response() called: ch=0x2834e620, npc=0x28368e40, response_len=334
Jul 27 20:11:29 :: AI_DEBUG [ai_events.c:138 in queue_ai_response()]: Validating character pointers
Jul 27 20:11:29 :: AI_DEBUG [ai_events.c:146 in queue_ai_response()]: Validation result: ch_valid=YES, npc_valid=YES
Jul 27 20:11:29 :: AI_DEBUG [ai_events.c:155 in queue_ai_response()]: Creating AI response event data
Jul 27 20:11:29 :: AI_DEBUG [ai_events.c:162 in queue_ai_response()]: Event data allocated at 0x72235e0
Jul 27 20:11:29 :: AI_DEBUG [ai_events.c:173 in queue_ai_response()]: Response duplicated (length=334)
Jul 27 20:11:29 :: AI_DEBUG [ai_events.c:182 in queue_ai_response()]: Calculated delay: 5 seconds (response_len=334)
Jul 27 20:11:29 :: AI_DEBUG [ai_events.c:186 in queue_ai_response()]: Creating event with 5 second delay (50 pulses)
Jul 27 20:11:29 :: AI_DEBUG [ai_events.c:188 in queue_ai_response()]: AI response event queued successfully
Jul 27 20:11:29 :: PERFMON: High load detected (1893.4%), enabling full monitoring
Jul 27 20:11:29 :: Pulse usage new high water mark [1893.42%, 1893419 usec]. Trace info: 
Pulse profiling info


Jul 27 20:11:29 :: PERFMON: Load normalized (11.9%), returning to sampling mode
Jul 27 20:11:32 :: AI_DEBUG [ai_events.c:50 in ai_response_event()]: ai_response_event() triggered
Jul 27 20:11:32 :: AI_DEBUG [ai_events.c:60 in ai_response_event()]: Validating event data: ch=0x2834e620, npc=0x28368e40, response=0xdc5ca10
Jul 27 20:11:32 :: AI_DEBUG [ai_events.c:73 in ai_response_event()]: Checking if characters still exist in game
Jul 27 20:11:32 :: AI_DEBUG [ai_events.c:84 in ai_response_event()]: Character validation: ch_found=YES, npc_found=YES
Jul 27 20:11:32 :: AI_DEBUG [ai_events.c:96 in ai_response_event()]: Checking room locations: ch_room=845, npc_room=845
Jul 27 20:11:32 :: AI_DEBUG [ai_events.c:101 in ai_response_event()]: Characters in same room, delivering response
Jul 27 20:11:32 :: AI: Zusuk -> a janni bladesman: Aye, traveler! The desert winds whisper of a lost relic buried beneath the shifting sands—a blade said to harness the storm’s fury. Retrieve it, and you’ll earn the favor of the Janni and the blessings of the desert spirits. But beware, for sand wraiths guard its resting place. Are you ready to face the scorching trials ahead?
Jul 27 20:11:32 :: AI_DEBUG [ai_events.c:108 in ai_response_event()]: Response delivered successfully
Jul 27 20:11:32 :: AI_DEBUG [ai_events.c:114 in ai_response_event()]: Cleaning up event data
Jul 27 20:11:58 :: AI_DEBUG [ai_cache.c:115 in ai_cache_get()]: ai_cache_get() called with key='npc_1218_I am too weak to continue.'
Jul 27 20:11:58 :: AI_DEBUG [ai_cache.c:122 in ai_cache_get()]: Searching cache (size=3)
Jul 27 20:11:58 :: AI_DEBUG [ai_cache.c:142 in ai_cache_get()]: Cache miss - no valid entry found
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:532 in ai_generate_response()]: ai_generate_response() called with request_type=1
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:534 in ai_generate_response()]: Original prompt: 'You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep...'
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:239 in is_ai_enabled()]: Checking if AI enabled: initialized=1, config=0x7529ee0, enabled=1, result=1
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:549 in ai_generate_response()]: Sanitizing input prompt
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:552 in ai_generate_response()]: Sanitized prompt: 'You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep...'
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:555 in ai_generate_response()]: Making API request
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:502 in make_api_request()]: make_api_request() called with max retries: 3
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:505 in make_api_request()]: Attempt 1/3
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:345 in make_api_request_single()]: make_api_request_single() called with prompt: 'You are a janni bladesman in a fantasy RPG world. ...'
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:354 in make_api_request_single()]: Checking rate limits
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:671 in ai_check_rate_limit()]: Checking rate limits at time 1753647118
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:680 in ai_check_rate_limit()]: Current counts: minute=3/60, hour=3/1000
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:710 in ai_check_rate_limit()]: Rate limit check passed, new counts: minute=4, hour=4
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:360 in make_api_request_single()]: Rate limit check passed
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:363 in make_api_request_single()]: Decrypting API key
Jul 27 20:11:58 :: AI_DEBUG [ai_security.c:68 in decrypt_api_key()]: decrypt_api_key() called
Jul 27 20:11:58 :: AI_DEBUG [ai_security.c:75 in decrypt_api_key()]: WARNING: Using plaintext retrieval (no decryption)
Jul 27 20:11:58 :: AI_DEBUG [ai_security.c:76 in decrypt_api_key()]: Encrypted length: 164
Jul 27 20:11:58 :: AI_DEBUG [ai_security.c:89 in decrypt_api_key()]: API key retrieved (first 10 chars): sk-proj-wy...
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:370 in make_api_request_single()]: API key decrypted successfully (length=164)
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:373 in make_api_request_single()]: Building JSON request
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:737 in build_json_request()]: build_json_request() called
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:738 in build_json_request()]: Input prompt length: 170
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:748 in build_json_request()]: Escaping JSON special characters
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:771 in build_json_request()]: Escaping complete: 4 characters escaped, final length: 174
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:774 in build_json_request()]: Building JSON request structure
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:775 in build_json_request()]:   Model: gpt-4.1-mini
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:776 in build_json_request()]:   Max tokens: 500
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:777 in build_json_request()]:   Temperature: 0.00
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:793 in build_json_request()]: JSON request built, total length: 283
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:795 in build_json_request()]: JSON structure: {"model": "gpt-4.1-mini","messages": [{"role": "user", "content": "You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep response under 100 words. Player...
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:380 in make_api_request_single()]: JSON request built (length=283)
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:382 in make_api_request_single()]: JSON content: {"model": "gpt-4.1-mini","messages": [{"role": "user", "content": "You are a janni bladesman in a fantasy RPG world. Respond to the player's message in character. Keep response under 100 words. Player...
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:384 in make_api_request_single()]: Initializing CURL handle
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:394 in make_api_request_single()]: CURL handle initialized at 0x2836d430
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:397 in make_api_request_single()]: Setting up HTTP headers
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:399 in make_api_request_single()]:   Auth header length: 186
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:408 in make_api_request_single()]:   Auth header added successfully
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:418 in make_api_request_single()]:   Content-Type header added successfully
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:421 in make_api_request_single()]: Configuring CURL options
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:423 in make_api_request_single()]:   URL: https://api.openai.com/v1/chat/completions
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:425 in make_api_request_single()]:   Headers set
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:427 in make_api_request_single()]:   POST data length: 283 bytes
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:429 in make_api_request_single()]:   Write callback set to ai_curl_write_callback
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:431 in make_api_request_single()]:   Write data pointer set to response buffer
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:438 in make_api_request_single()]:   Timeout set to 5000 ms
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:442 in make_api_request_single()]:   SSL verification enabled (peer=1, host=2)
Jul 27 20:11:58 :: AI_DEBUG [ai_service.c:445 in make_api_request_single()]: Executing CURL request to API endpoint
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:52 in ai_curl_write_callback()]: CURL callback received 1149 bytes (size=1, nmemb=1149)
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:53 in ai_curl_write_callback()]: Current buffer size: 0 bytes
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:67 in ai_curl_write_callback()]: Buffer successfully expanded to 1149 bytes
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:69 in ai_curl_write_callback()]: Response chunk: {
  "id": "chatcmpl-By20tkpycrIi9eQRLt5ReU4nLHX2t",
  "object": "chat.completion",
  "created": 1753...
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:447 in make_api_request_single()]: CURL request completed with result: 0 (No error)
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:452 in make_api_request_single()]: HTTP response code: 200
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:453 in make_api_request_single()]: Response size: 1149 bytes
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:456 in make_api_request_single()]: Success! Parsing JSON response
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:458 in make_api_request_single()]: Raw response: {
  "id": "chatcmpl-By20tkpycrIi9eQRLt5ReU4nLHX2t",
  "object": "chat.completion",
  "created": 1753647119,
  "model": "gpt-4.1-mini-2025-04-14",
  "choices": [
    {
      "index": 0,
      "message": {
        "role": "assistant",
        "content": "Weakness is but a shadow before the dawn, traveler. Even the mightiest blade was once forged in fire and doubt. Rest now, gather your strength, and remember—every step forward, no matter how small, carves the path to victory. The desert winds wh...
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:809 in parse_json_response()]: parse_json_response() called
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:810 in parse_json_response()]: JSON input length: 1149
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:819 in parse_json_response()]: Searching for content field in JSON
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:828 in parse_json_response()]: Found 'content' field at position 241
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:829 in parse_json_response()]: Context around content field: "content": "Weakness is but a shadow before the dawn, traveler. Even the mightiest blade was once fo
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:833 in parse_json_response()]: After skipping 'content:' label, position 251
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:834 in parse_json_response()]: Character at position: ' ' (ASCII 32)
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:842 in parse_json_response()]: Skipped 1 whitespace characters
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:851 in parse_json_response()]: Content value starts at position 253
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:852 in parse_json_response()]: First 50 chars of content: Weakness is but a shadow before the dawn, traveler
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:855 in parse_json_response()]: Searching for closing quote with escape handling
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:870 in parse_json_response()]:   Found quote #1 at position 340 with 0 preceding backslashes
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:873 in parse_json_response()]:   This is the closing quote (unescaped)
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:894 in parse_json_response()]: Found closing quote at position 340
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:898 in parse_json_response()]: Extracting content of length 340
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:899 in parse_json_response()]: Raw content before unescaping: Weakness is but a shadow before the dawn, traveler. Even the mightiest blade was once forged in fire...
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:907 in parse_json_response()]: Allocated 341 bytes for result at 0x14e71a00
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:910 in parse_json_response()]: Starting content unescaping process
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:941 in parse_json_response()]: Unescaping complete: processed 340 chars, found 0 escape sequences
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:942 in parse_json_response()]: Final content length: 340
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:943 in parse_json_response()]: Content preview: Weakness is but a shadow before the dawn, traveler. Even the mightiest blade was once forged in fire...
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:944 in parse_json_response()]: Full content: Weakness is but a shadow before the dawn, traveler. Even the mightiest blade was once forged in fire and doubt. Rest now, gather your strength, and remember—every step forward, no matter how small, carves the path to victory. The desert winds whisper of your destiny; do not let them fade unheard. Rise, for the sands favor the steadfast.
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:461 in make_api_request_single()]: JSON parsing successful, result length: 340
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:476 in make_api_request_single()]: Cleaning up CURL resources
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:478 in make_api_request_single()]:   CURL handle cleaned up
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:480 in make_api_request_single()]:   Headers freed
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:483 in make_api_request_single()]:   Freeing API key
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:487 in make_api_request_single()]:   Freeing response data (1149 bytes)
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:491 in make_api_request_single()]: make_api_request_single() returning success
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:508 in make_api_request()]: Request succeeded on attempt 1
Jul 27 20:12:00 :: AI_DEBUG [ai_service.c:564 in ai_generate_response()]: Response received (length=340)
Jul 27 20:12:00 :: AI_DEBUG [ai_cache.c:30 in ai_cache_response()]: ai_cache_response() called with key='npc_1218_I am too weak to continue.', response_len=340
Jul 27 20:12:00 :: AI_DEBUG [ai_cache.c:38 in ai_cache_response()]: Searching for existing cache entry
Jul 27 20:12:00 :: AI_DEBUG [ai_cache.c:60 in ai_cache_response()]: No existing entry found, creating new one
Jul 27 20:12:00 :: AI_DEBUG [ai_cache.c:63 in ai_cache_response()]: Creating new cache entry
Jul 27 20:12:00 :: AI_DEBUG [ai_cache.c:78 in ai_cache_response()]:   Key stored: 'npc_1218_I am too weak to continue.'
Jul 27 20:12:00 :: AI_DEBUG [ai_cache.c:88 in ai_cache_response()]:   Response stored (length=340)
Jul 27 20:12:00 :: AI_DEBUG [ai_cache.c:92 in ai_cache_response()]:   Entry expires at 1753650720 (in 3600 seconds)
Jul 27 20:12:00 :: AI_DEBUG [ai_cache.c:99 in ai_cache_response()]: Cache entry added, new cache size: 4/1000
Jul 27 20:12:00 :: AI_DEBUG [ai_events.c:130 in queue_ai_response()]: queue_ai_response() called: ch=0x2834e620, npc=0x28368e40, response_len=340
Jul 27 20:12:00 :: AI_DEBUG [ai_events.c:138 in queue_ai_response()]: Validating character pointers
Jul 27 20:12:00 :: AI_DEBUG [ai_events.c:146 in queue_ai_response()]: Validation result: ch_valid=YES, npc_valid=YES
Jul 27 20:12:00 :: AI_DEBUG [ai_events.c:155 in queue_ai_response()]: Creating AI response event data
Jul 27 20:12:00 :: AI_DEBUG [ai_events.c:162 in queue_ai_response()]: Event data allocated at 0xb99a0d0
Jul 27 20:12:00 :: AI_DEBUG [ai_events.c:173 in queue_ai_response()]: Response duplicated (length=340)
Jul 27 20:12:00 :: AI_DEBUG [ai_events.c:182 in queue_ai_response()]: Calculated delay: 5 seconds (response_len=340)
Jul 27 20:12:00 :: AI_DEBUG [ai_events.c:186 in queue_ai_response()]: Creating event with 5 second delay (50 pulses)
Jul 27 20:12:00 :: AI_DEBUG [ai_events.c:188 in queue_ai_response()]: AI response event queued successfully
Jul 27 20:12:00 :: PERFMON: High load detected (1064.3%), enabling full monitoring
Jul 27 20:12:00 :: PERFMON: Load normalized (6.1%), returning to sampling mode
Jul 27 20:12:02 :: PERFMON: High load detected (484.2%), enabling full monitoring
Jul 27 20:12:02 :: PERFMON: Load normalized (0.3%), returning to sampling mode
Jul 27 20:12:03 :: AI_DEBUG [ai_events.c:50 in ai_response_event()]: ai_response_event() triggered
Jul 27 20:12:03 :: AI_DEBUG [ai_events.c:60 in ai_response_event()]: Validating event data: ch=0x2834e620, npc=0x28368e40, response=0x746b7d0
Jul 27 20:12:03 :: AI_DEBUG [ai_events.c:73 in ai_response_event()]: Checking if characters still exist in game
Jul 27 20:12:03 :: AI_DEBUG [ai_events.c:84 in ai_response_event()]: Character validation: ch_found=YES, npc_found=YES
Jul 27 20:12:03 :: AI_DEBUG [ai_events.c:96 in ai_response_event()]: Checking room locations: ch_room=845, npc_room=845
Jul 27 20:12:03 :: AI_DEBUG [ai_events.c:101 in ai_response_event()]: Characters in same room, delivering response
Jul 27 20:12:03 :: AI: Zusuk -> a janni bladesman: Weakness is but a shadow before the dawn, traveler. Even the mightiest blade was once forged in fire and doubt. Rest now, gather your strength, and remember—every step forward, no matter how small, carves the path to victory. The desert winds whisper of your destiny; do not let them fade unheard. Rise, for the sands favor the steadfast.
Jul 27 20:12:03 :: AI_DEBUG [ai_events.c:108 in ai_response_event()]: Response delivered successfully
Jul 27 20:12:03 :: AI_DEBUG [ai_events.c:114 in ai_response_event()]: Cleaning up event data
