/**
 * @file ai_events.c
 * @author Zusuk
 * @brief AI event system integration
 * 
 * Handles event-driven AI responses with delays for more natural
 * conversation flow.
 * 
 * COMPONENT INTERACTIONS:
 * - INTEGRATES WITH: mud_event.c event system
 * - CALLED BY: ai_service.c worker threads and async functions
 * - VALIDATES: Character pointers before response delivery
 * - CRITICAL FOR: Thread safety and async response handling
 * 
 * THREAD SAFETY MODEL:
 * - Worker threads queue events via queue_ai_response()
 * - Main thread processes events via ai_response_event()
 * - Character validation prevents crashes from freed memory
 * - Events self-destruct if characters no longer exist
 * 
 * EVENT TYPES:
 * 1. ai_response_event - Delivers AI responses to players
 * 2. ai_request_retry_event - Retries failed API requests
 * 
 * Part of the LuminariMUD distribution.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "ai_service.h"
#include "dg_event.h"
#include "mud_event.h"

/* AI Response Event Data
 * Passed from worker threads to main thread via event system.
 * Character pointers may become invalid between creation and execution,
 * so validation is CRITICAL in the event handler.
 */
struct ai_response_event {
  struct char_data *ch;   /* Player who initiated conversation */
  struct char_data *npc;  /* NPC providing response */
  char *response;         /* AI-generated response text */
};

/* AI Request Retry Event Data
 * Used for exponential backoff retry logic when API requests fail.
 * Retry events are chained - each failure creates a new retry event
 * with incremented retry_count until AI_MAX_RETRIES is reached.
 */
struct ai_request_retry_event {
  char *prompt;         /* Original prompt to retry */
  int request_type;     /* AI_REQUEST_* constant */
  int retry_count;      /* Current retry attempt number */
  struct char_data *ch;   /* Optional: player character */
  struct char_data *npc;  /* Optional: NPC character */
};

/* Event function prototypes */
EVENTFUNC(ai_response_event);
EVENTFUNC(ai_request_retry_event);

/**
 * AI Response Event Handler
 * 
 * Delivers AI-generated responses to players after validation.
 * This runs in the MAIN THREAD after worker thread completes.
 * 
 * CRITICAL VALIDATION STEPS:
 * 1. Verify event data exists and is complete
 * 2. Check both characters still exist in character_list
 * 3. Verify characters are in same room
 * 4. Only then deliver the response
 * 
 * This extensive validation prevents crashes when characters
 * are freed during the async AI operation. Common scenarios:
 * - Player disconnects while waiting for response
 * - NPC is purged/killed during API call
 * - Characters move to different rooms
 * 
 * Called by: MUD event system (event_process() in mud_event.c)
 * Returns: 0 (event completes and self-destructs)
 */
EVENTFUNC(ai_response_event) {
  struct ai_response_event *data = (struct ai_response_event *)event_obj;
  char buf[MAX_STRING_LENGTH];
  
  AI_DEBUG("ai_response_event() triggered");
  
  /* Validate data */
  if (!data) {
    AI_DEBUG("ERROR: NULL event data");
    return 0;
  }
  
  /* Check if characters still exist */
  AI_DEBUG("Validating event data: ch=%p, npc=%p, response=%p",
           (void*)data->ch, (void*)data->npc, (void*)data->response);
           
  if (!data->ch || !data->npc || !data->response) {
    AI_DEBUG("ERROR: Missing required data (ch=%s, npc=%s, response=%s)",
             data->ch ? "OK" : "NULL",
             data->npc ? "OK" : "NULL",
             data->response ? "OK" : "NULL");
    if (data->response) free(data->response);
    free(data);
    return 0;
  }
  
  /* Validate characters are still in character list */
  AI_DEBUG("Checking if characters still exist in game");
  struct char_data *ch;
  bool ch_found = FALSE, npc_found = FALSE;
  
  for (ch = character_list; ch; ch = ch->next) {
    if (ch == data->ch) ch_found = TRUE;
    if (ch == data->npc) npc_found = TRUE;
    if (ch_found && npc_found) break;
  }
  
  AI_DEBUG("Character validation: ch_found=%s, npc_found=%s",
           ch_found ? "YES" : "NO", npc_found ? "YES" : "NO");
  
  if (!ch_found || !npc_found) {
    /* One or both characters have been freed */
    AI_DEBUG("Characters no longer exist, cleaning up");
    free(data->response);
    free(data);
    return 0;
  }
  
  /* Verify both characters are still valid and in same room */
  AI_DEBUG("Checking room locations: ch_room=%d, npc_room=%d",
           IN_ROOM(data->ch), IN_ROOM(data->npc));
           
  if (IN_ROOM(data->ch) == IN_ROOM(data->npc) && 
      IN_ROOM(data->ch) != NOWHERE) {
    
    AI_DEBUG("Characters in same room, delivering response");
    /* Send the AI response */
    snprintf(buf, sizeof(buf), "$n tells you, '%s'", data->response);
    act(buf, FALSE, data->npc, 0, data->ch, TO_VICT);
    
    /* Log the interaction */
    log_ai_interaction(data->ch, data->npc, data->response);
    AI_DEBUG("Response delivered successfully");
  } else {
    AI_DEBUG("Characters not in same room or in NOWHERE, skipping response");
  }
  
  /* Cleanup */
  AI_DEBUG("Cleaning up event data");
  free(data->response);
  free(data);
  return 0;
}

/**
 * Queue an AI response with a delay
 * 
 * Thread-safe function to queue AI responses for delivery.
 * Can be called from worker threads or main thread.
 * 
 * Flow:
 * 1. Validate character pointers are currently valid
 * 2. Create event data structure with response
 * 3. Calculate realistic delay (currently minimal)
 * 4. Queue event for main thread processing
 * 
 * Called by:
 * - ai_thread_worker() from worker threads
 * - ai_npc_dialogue_async() when cache hit (main thread)
 * - ai_request_retry_event() after successful retry
 * 
 * Thread safety: Character validation here is a snapshot.
 * The event handler must re-validate before delivery.
 * 
 * Delay: Currently 0 seconds for instant responses.
 * Can be adjusted based on response length for realism.
 */
void queue_ai_response(struct char_data *ch, struct char_data *npc, const char *response) {
  struct ai_response_event *event_data;
  struct char_data *temp;
  int delay;
  bool ch_valid = FALSE, npc_valid = FALSE;
  
  AI_DEBUG("queue_ai_response() called: ch=%p, npc=%p, response_len=%zu",
           (void*)ch, (void*)npc, response ? strlen(response) : 0);
  
  if (!ch || !npc || !response) {
    AI_DEBUG("ERROR: NULL parameters provided");
    return;
  }
  
  /* Validate characters are in character list before creating event */
  AI_DEBUG("Validating character pointers");
  for (temp = character_list; temp; temp = temp->next) {
    if (temp == ch) ch_valid = TRUE;
    if (temp == npc) npc_valid = TRUE;
    if (ch_valid && npc_valid) break;
  }
  
  AI_DEBUG("Validation result: ch_valid=%s, npc_valid=%s",
           ch_valid ? "YES" : "NO", npc_valid ? "YES" : "NO");
  
  if (!ch_valid || !npc_valid) {
    log("SYSERR: queue_ai_response called with invalid character pointers");
    AI_DEBUG("ERROR: Invalid character pointers detected");
    return;
  }
  
  /* Create event data */
  AI_DEBUG("Creating AI response event data");
  CREATE(event_data, struct ai_response_event, 1);
  if (!event_data) {
    log("SYSERR: Failed to allocate memory for AI response event");
    AI_DEBUG("ERROR: Failed to allocate event data structure");
    return;
  }
  AI_DEBUG("Event data allocated at %p", (void*)event_data);
  
  event_data->ch = ch;
  event_data->npc = npc;
  event_data->response = strdup(response);
  if (!event_data->response) {
    log("SYSERR: Failed to allocate memory for AI response text");
    AI_DEBUG("ERROR: strdup failed for response");
    free(event_data);
    return;
  }
  AI_DEBUG("Response duplicated (length=%zu)", strlen(event_data->response));
  
  /* Calculate delay based on response length for realism */
  /* Reduced delay for faster responses - only 1 second flat */
  delay = 0;  /* Minimal delay to prevent spam while keeping responses fast */
  
  AI_DEBUG("Calculated delay: %d seconds (response_len=%zu)", 
           delay, strlen(response));
  
  /* Create the event */
  AI_DEBUG("Creating event with %d second delay (%d pulses)", 
           delay, delay * PASSES_PER_SEC);
  event_create(ai_response_event, event_data, delay * PASSES_PER_SEC);
  AI_DEBUG("AI response event queued successfully");
}

/**
 * AI Request Retry Event Handler
 * 
 * Implements exponential backoff retry logic for failed API requests.
 * This provides resilience against temporary API failures.
 * 
 * Retry strategy:
 * - Initial attempt: No delay
 * - Retry 1: 1 second delay
 * - Retry 2: 2 seconds delay
 * - Retry 3: 4 seconds delay
 * - Maximum delay capped at 16 seconds
 * 
 * Flow:
 * 1. Validate event data and optional character pointers
 * 2. Call ai_generate_response_async() for single attempt
 * 3. If successful, queue response via queue_ai_response()
 * 4. If failed and retries remain, caller creates new retry event
 * 
 * Called by: MUD event system after retry delay
 * Interacts with: ai_service.c for API calls
 */
EVENTFUNC(ai_request_retry_event) {
  struct ai_request_retry_event *data = (struct ai_request_retry_event *)event_obj;
  char *response;
  struct char_data *temp;
  bool ch_valid = FALSE, npc_valid = FALSE;
  
  AI_DEBUG("ai_request_retry_event() triggered");
  
  /* Validate data */
  if (!data || !data->prompt) {
    AI_DEBUG("ERROR: Invalid event data (data=%p, prompt=%p)",
             (void*)data, data ? (void*)data->prompt : NULL);
    if (data) {
      if (data->prompt) free(data->prompt);
      free(data);
    }
    return 0;
  }
  
  AI_DEBUG("Retry event: type=%d, retry_count=%d/%d",
           data->request_type, data->retry_count, AI_MAX_RETRIES);
  
  /* If characters were provided, validate they still exist */
  if (data->ch || data->npc) {
    AI_DEBUG("Validating character pointers: ch=%p, npc=%p",
             (void*)data->ch, (void*)data->npc);
    
    for (temp = character_list; temp; temp = temp->next) {
      if (temp == data->ch) ch_valid = TRUE;
      if (temp == data->npc) npc_valid = TRUE;
      if ((data->ch && ch_valid) && (data->npc && npc_valid)) break;
    }
    
    AI_DEBUG("Character validation: ch_valid=%s, npc_valid=%s",
             data->ch ? (ch_valid ? "YES" : "NO") : "N/A",
             data->npc ? (npc_valid ? "YES" : "NO") : "N/A");
    
    /* If either character was freed, abort the retry */
    if ((data->ch && !ch_valid) || (data->npc && !npc_valid)) {
      AI_DEBUG("Characters no longer valid, aborting retry");
      free(data->prompt);
      free(data);
      return 0;
    }
  }
  
  /* Make the API request (will internally retry if needed) */
  AI_DEBUG("Making async API request");
  response = ai_generate_response_async(data->prompt, data->request_type, data->retry_count);
  
  if (response) {
    AI_DEBUG("Got response (length=%zu)", strlen(response));
  } else {
    AI_DEBUG("No response received");
  }
  
  /* If we have a response and valid characters, queue the response event */
  if (response && data->ch && data->npc && ch_valid && npc_valid) {
    AI_DEBUG("Queueing response event");
    queue_ai_response(data->ch, data->npc, response);
    free(response);
  } else {
    AI_DEBUG("Not queueing response: response=%s, ch=%s, npc=%s",
             response ? "YES" : "NO",
             (data->ch && ch_valid) ? "VALID" : "INVALID",
             (data->npc && npc_valid) ? "VALID" : "INVALID");
  }
  
  /* Cleanup */
  AI_DEBUG("Cleaning up retry event data");
  free(data->prompt);
  free(data);
  return 0;
}

/**
 * Queue an AI request with retry support
 * 
 * Creates a retry event for failed AI requests with exponential backoff.
 * This is the entry point for the retry system.
 * 
 * Called by:
 * - ai_npc_dialogue_async() when thread creation fails
 * - ai_request_retry_event() to chain retry attempts
 * - Future: Any async AI operation needing retry logic
 * 
 * Delay calculation:
 * - retry_count 0: 0.1 seconds (immediate first attempt)
 * - retry_count n: 2^n seconds (exponential backoff)
 * - Maximum delay: 16 seconds
 * 
 * The retry event will:
 * 1. Make one API attempt via ai_generate_response_async()
 * 2. Deliver response if successful
 * 3. Let caller handle further retries if needed
 * 
 * Thread safety: Can be called from any thread
 */
void queue_ai_request_retry(const char *prompt, int request_type, int retry_count, 
                           struct char_data *ch, struct char_data *npc) {
  struct ai_request_retry_event *event_data;
  int delay;
  
  AI_DEBUG("queue_ai_request_retry() called: type=%d, retry=%d, ch=%p, npc=%p",
           request_type, retry_count, (void*)ch, (void*)npc);
  
  if (!prompt) {
    AI_DEBUG("ERROR: NULL prompt provided");
    return;
  }
  
  /* Create event data */
  AI_DEBUG("Creating retry event data");
  CREATE(event_data, struct ai_request_retry_event, 1);
  if (!event_data) {
    log("SYSERR: Failed to allocate memory for AI request retry event");
    AI_DEBUG("ERROR: Failed to allocate retry event structure");
    return;
  }
  
  event_data->prompt = strdup(prompt);
  if (!event_data->prompt) {
    log("SYSERR: Failed to allocate memory for AI prompt text");
    AI_DEBUG("ERROR: strdup failed for prompt");
    free(event_data);
    return;
  }
  
  AI_DEBUG("Event data populated:");
  AI_DEBUG("  Prompt: '%.50s%s'", event_data->prompt,
           strlen(event_data->prompt) > 50 ? "..." : "");
  event_data->request_type = request_type;
  AI_DEBUG("  Request type: %d", event_data->request_type);
  event_data->retry_count = retry_count;
  AI_DEBUG("  Retry count: %d", event_data->retry_count);
  event_data->ch = ch;
  event_data->npc = npc;
  AI_DEBUG("  Characters: ch=%p, npc=%p", (void*)ch, (void*)npc);
  
  /* Minimal initial delay to avoid blocking immediately */
  if (retry_count == 0) {
    delay = 1;  /* 0.1 seconds for first attempt */
  } else {
    /* Calculate exponential backoff delay for retries: 2^retry_count seconds */
    delay = 1 << retry_count;
  }
  AI_DEBUG("Calculated delay: %d pulses (retry_count=%d)", delay, retry_count);
  
  if (delay > 16) {
    delay = 16; /* Cap at 16 seconds */
    AI_DEBUG("Delay capped at maximum 16 seconds");
  }
  
  /* Create the event */
  AI_DEBUG("Creating retry event with %d second delay (%d pulses)",
           delay, delay * PASSES_PER_SEC);
  event_create(ai_request_retry_event, event_data, delay * PASSES_PER_SEC);
  AI_DEBUG("AI request retry event queued successfully");
}