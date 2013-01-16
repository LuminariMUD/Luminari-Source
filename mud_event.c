/**************************************************************************
*  File: mud_event.c                                       Part of tbaMUD *
*  Usage: Handling of the mud event system                                *
*                                                                         *
*  By Vatiken. Copyright 2012 by Joseph Arnusch                           *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_event.h"
#include "constants.h"
#include "comm.h"  /* For access to the game pulse */
#include "mud_event.h"

/* Global List */
struct list_data * world_events = NULL;

/* The mud_event_index[] is merely a tool for organizing events, and giving
 * them a "const char *" name to help in potential debugging */
struct mud_event_list mud_event_index[] = {
  { "Null"        	     , NULL           , 	-1          }, /* eNULL */
  { "Protocol"    	     , get_protocols  , 	EVENT_DESC  }, /* ePROTOCOLS */
  { "Whirlwind"   	     , event_whirlwind, 	EVENT_CHAR  }, /* eWHIRLWIND */
  { "Casting"            , event_casting, 	EVENT_CHAR  },  /* eCASTING */
  { "Lay on hands"       , event_countdown,	EVENT_CHAR  }, // eLAYONHANDS
  { "Treat injury"	     , event_countdown,	EVENT_CHAR  }, // eTREATINJURY
  { "Taunt"	          , event_countdown,	EVENT_CHAR  }, // eTAUNT
  { "Taunted"	          , event_countdown,	EVENT_CHAR  }, // eTAUNTED
  { "Mummy dust"	     , event_countdown,	EVENT_CHAR  }, // eMUMMYDUST
  { "Dragon knight"	     , event_countdown,	EVENT_CHAR  }, //  eDRAGONKNIGHT
  { "Greater ruin"	     , event_countdown,	EVENT_CHAR  }, // eGREATERRUIN
  { "Hellball"	          , event_countdown,	EVENT_CHAR  }, // eHELLBALL
  { "Epic mage armor"	, event_countdown,	EVENT_CHAR  }, // eEPICMAGEARMOR
  { "Epic warding"	     , event_countdown,	EVENT_CHAR  }, // eEPICWARDING
  { "Memorizing"  	     , event_memorizing, EVENT_CHAR  }, //eMEMORIZING 
  { "Stunned"  	     , event_countdown, 	EVENT_CHAR  }, //eSTUNNED
  { "Stunning fist"  	, event_countdown, 	EVENT_CHAR  }, //eSTUNNINGFIST 
  { "Crafting"  		, event_crafting, 	EVENT_CHAR  },  //eCRAFTING
  { "Crystal fist"       , event_countdown, 	EVENT_CHAR  },  //eCRYSTALFIST
  { "Crystal body"       , event_countdown, 	EVENT_CHAR  },  //eCRYRSTALBODY
  { "Rage"               , event_countdown, 	EVENT_CHAR  },  //eRAGE
  { "Acid arrow"         , event_acid_arrow, EVENT_CHAR  },  //eACIDARROW
};


/* init_events() is the ideal function for starting global events. This
 * might be the case if you were to move the contents of heartbeat() into
 * the event system */
void init_events(void)
{
  /* Allocate Event List */
  world_events = create_list();
}


 /* The bottom switch() is for any post-event actions, like telling the character they can
 * now access their skill again.
 */
EVENTFUNC(event_countdown)
{
  struct mud_event_data * pMudEvent = NULL;
  struct char_data * ch = NULL;
	
  pMudEvent = (struct mud_event_data * ) event_obj;
	
  switch (mud_event_index[pMudEvent->iId].iEvent_Type) {
    case EVENT_CHAR:
      ch = (struct char_data * ) pMudEvent->pStruct;
    break;
    default:
    break;
  }	
	
  switch (pMudEvent->iId) {
    case eMUMMYDUST:
      send_to_char(ch, "You are now able to cast Mummy Dust again.\r\n");
      break;
    case eDRAGONKNIGHT:
      send_to_char(ch, "You are now able to cast Dragon Knight again.\r\n");
      break;
    case eGREATERRUIN:
      send_to_char(ch, "You are now able to cast Greater Ruin again.\r\n");
      break;
    case eHELLBALL:
      send_to_char(ch, "You are now able to cast Hellball again.\r\n");
      break;
    case eEPICMAGEARMOR:
      send_to_char(ch, "You are now able to cast Epic Mage Armor again.\r\n");
      break;
    case eEPICWARDING:
      send_to_char(ch, "You are now able to cast Epic Warding again.\r\n");
      break;
    case eTAUNT:
      send_to_char(ch, "You are now able to taunt again.\r\n");
      break;
    case eRAGE:
      send_to_char(ch, "You are now able to Rage again.\r\n");
      break;
    case eTAUNTED:
      send_to_char(ch, "You feel the effects of the taunt wear off.\r\n");
      break;
    case eLAYONHANDS:
      send_to_char(ch, "You are now able to lay on hands again.\r\n");
      break;
    case eTREATINJURY:
      send_to_char(ch, "You are now able to treat injuries again.\r\n");
      break;
    case eSTUNNED:
      send_to_char(ch, "You are now free from the stunning affect.\r\n");
      break;
    case eSTUNNINGFIST:
      send_to_char(ch, "You are now able to strike with your stunning"
                       " fist again.\r\n");
      break;
    case eCRYSTALFIST:
      send_to_char(ch, "You are now able to use crystal"
                       " fist again.\r\n");
      break;
    case eCRYSTALBODY:
      send_to_char(ch, "You are now able to use crystal"
                       " body again.\r\n");
      break;
    default:
    break;
  }

  return 0;
}


/* As of 3.63, there are only global, descriptor, and character events. This
 * is due to the potential scope of the necessary debugging if events were
 * included with rooms, objects, spells or any other structure type. Adding
 * events to these other systems should be just as easy as adding the current
 * library was, and should be available in a future release. - Vat */
void attach_mud_event(struct mud_event_data *pMudEvent, long time)
{
  struct event * pEvent = NULL;
  struct descriptor_data * d = NULL;
  struct char_data * ch = NULL;
   
  pEvent = event_create(mud_event_index[pMudEvent->iId].func, pMudEvent, time);
  pEvent->isMudEvent = TRUE;	
  pMudEvent->pEvent = pEvent;        

  switch (mud_event_index[pMudEvent->iId].iEvent_Type) {
    case EVENT_WORLD:
      add_to_list(pEvent, world_events);
    break;
    case EVENT_DESC:
      d = (struct descriptor_data *) pMudEvent->pStruct;
      add_to_list(pEvent, d->events);
    break;
    case EVENT_CHAR:
      ch = (struct char_data *) pMudEvent->pStruct;
      add_to_list(pEvent, ch->events);
    break;
  }
}


struct mud_event_data *new_mud_event(event_id iId, void *pStruct, char *sVariables)
{
  struct mud_event_data *pMudEvent = NULL;
  char *varString = NULL;
		
  CREATE(pMudEvent, struct mud_event_data, 1);
  varString = (sVariables != NULL) ? strdup(sVariables) : NULL;	
		
  pMudEvent->iId         = iId;
  pMudEvent->pStruct     = pStruct;
  pMudEvent->sVariables  = varString;
  pMudEvent->pEvent      = NULL;
	
  return (pMudEvent);	
}


void free_mud_event(struct mud_event_data *pMudEvent)
{
  struct descriptor_data * d = NULL;
  struct char_data * ch = NULL;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type) {
    case EVENT_WORLD:
      remove_from_list(pMudEvent->pEvent, world_events);
    break;
    case EVENT_DESC:
      d = (struct descriptor_data *) pMudEvent->pStruct;
      remove_from_list(pMudEvent->pEvent, d->events);
    break;
    case EVENT_CHAR:
      ch = (struct char_data *) pMudEvent->pStruct;
      remove_from_list(pMudEvent->pEvent, ch->events);
    break;
  }

  if (pMudEvent->sVariables != NULL)
    free(pMudEvent->sVariables);
	  
  pMudEvent->pEvent->event_obj = NULL;
  free(pMudEvent);
}


struct mud_event_data * char_has_mud_event(struct char_data * ch, event_id iId)
{
  struct event * pEvent = NULL;
  struct mud_event_data * pMudEvent = NULL;
  bool found = FALSE;

  if (ch->events->iSize == 0)
    return NULL;

  simple_list(NULL);
	
  while ((pEvent = (struct event *) simple_list(ch->events)) != NULL) {
		if (!pEvent->isMudEvent)
		  continue;
		pMudEvent = (struct mud_event_data * ) pEvent->event_obj;
	  if (pMudEvent->iId == iId) {
	    found = TRUE;	
	    break;
    }
  }
  
  simple_list(NULL);
  
  if (found)
    return (pMudEvent);
  
  return NULL;
} 


void clear_char_event_list(struct char_data * ch)
{
  struct event * pEvent = NULL;
    
  if (ch->events == NULL)
    return;
    
  if (ch->events->iSize == 0)
    return;
    
  simple_list(NULL);

  while ((pEvent = (struct event *) simple_list(ch->events)) != NULL) {
    event_cancel(pEvent);
  }
  
  simple_list(NULL);  
}

