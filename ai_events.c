/**
 * @file ai_events.c
 * @author Zusuk
 * @brief AI event system integration
 * 
 * Handles event-driven AI responses with delays for more natural
 * conversation flow.
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

/* AI Response Event Data */
struct ai_response_event {
  struct char_data *ch;
  struct char_data *npc;
  char *response;
};

/* Event function prototype */
EVENTFUNC(ai_response_event);

/**
 * AI Response Event Handler
 */
EVENTFUNC(ai_response_event) {
  struct ai_response_event *data = (struct ai_response_event *)event_obj;
  char buf[MAX_STRING_LENGTH];
  
  /* Validate data */
  if (!data) {
    return 0;
  }
  
  /* Check if characters still exist */
  if (!data->ch || !data->npc || !data->response) {
    if (data->response) free(data->response);
    free(data);
    return 0;
  }
  
  /* Verify both characters are still valid and in same room */
  if (IN_ROOM(data->ch) == IN_ROOM(data->npc) && 
      IN_ROOM(data->ch) != NOWHERE) {
    
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

/**
 * Queue an AI response with a delay
 */
void queue_ai_response(struct char_data *ch, struct char_data *npc, const char *response) {
  struct ai_response_event *event_data;
  int delay;
  
  if (!ch || !npc || !response) return;
  
  /* Create event data */
  CREATE(event_data, struct ai_response_event, 1);
  event_data->ch = ch;
  event_data->npc = npc;
  event_data->response = strdup(response);
  
  /* Calculate delay based on response length for realism */
  /* Roughly 1 second per 20 characters, minimum 1 second, maximum 5 seconds */
  delay = strlen(response) / 20;
  if (delay < 1) delay = 1;
  if (delay > 5) delay = 5;
  
  /* Create the event */
  event_create(ai_response_event, event_data, delay * PASSES_PER_SEC);
}