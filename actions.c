/*
 * Luminari Action System
 *
 */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "screen.h"
#include "modify.h"
#include "structs.h"
#include "lists.h"
#include "handler.h"
#include "spells.h"
#include "mud_event.h"
#include "actions.h"
#include "act.h"

/*  Attack action definitions - Define the relationships between 
 *  AA_XXXXX and do_XXXXX. */ 

void  (*attack_actions[NUM_ATTACK_ACTIONS])(struct char_data *ch,
                                            char *argument,
                                            int cmd,
                                            int subcmd) = {
  do_trip,         /* AA_TRIP */
  do_charge,       /* AA_CHARGE */
  do_smite,        /* AA_SMITE */
  do_stunningfist, /* AA_STUNNINGFIST */
  do_headbutt,     /* AA_HEDABUTT */
  do_kick,         /* AA_KICK */
  do_shieldpunch   /* AA_SHIELDPUNCH */
};


/*
 * Action Cooldown events are:
 *   eSTANDARDACTION
 *   eMOVEACTION
 *
 * If a player has one of these events, that signifies that they do NOT have
 * that action available.
 *
 */
EVENTFUNC(event_action_cooldown)
{
  struct mud_event_data * pMudEvent;
  struct char_data * ch = NULL;
  char buf[32];

  pMudEvent = (struct mud_event_data * ) event_obj;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type) {
    case EVENT_CHAR:
      ch = (struct char_data * ) pMudEvent->pStruct;
      break;
    default:
      break;
  }

  /* Lets show duration for immortals */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  	sprintf(buf, " (%.2f sec)", (float)(atoi(pMudEvent->sVariables))/10);
  else
  	*buf = '\0';

  switch (pMudEvent->iId) {
    case eSTANDARDACTION:
      if(!char_has_mud_event(ch, eSTANDARDACTION))
        send_to_char(ch, "You may perform another standard action.%s\r\n", buf);
      break;
	case eMOVEACTION:
	  if(!char_has_mud_event(ch, eMOVEACTION))
        send_to_char(ch, "You may perform another move action.%s\r\n", buf);
      break;
    default:
      break;
  }

  return 0;
};

bool is_action_available(struct char_data * ch, action_type act_type, bool msg_to_char)
{
	bool result = TRUE;

	if (act_type == atSTANDARD) {
		/* Is ch on standard action cooldown? */
		if(char_has_mud_event(ch, eSTANDARDACTION)) {
			if(msg_to_char)
				send_to_char(ch, "You don't have a standard action.");
			result = FALSE;
		}
		else
			result = TRUE;
	} else if (act_type == atMOVE) {
		/* Is ch on physical cooldown? */
		if(char_has_mud_event(ch, eMOVEACTION)) {
			if(msg_to_char)
				send_to_char(ch, "You don't have a move action.");
			result = FALSE;
		} else
			result = TRUE;
	}

	return result;
};

/* Start cooldowns for a specific command. Read cooldown length from command table. */
void start_cmd_cooldown(struct char_data *ch, int cmd)
{
	int i;

	for (i = 0; i < NUM_ACTIONS; i++) {
		if (complete_cmd_info[cmd].action_cooldowns[i] > 0) {
			start_action_cooldown(ch, i, complete_cmd_info[cmd].action_cooldowns[i]);
		}
	}
}

#undef OLDCODE
#ifdef OLDCODE
/* Start cooldowns for a specific skill/spell. Read cooldown length from spell table. */
void start_skill_cooldown(struct char_data *ch, int skill, int weapon)
{
	int i;

	for (i = 0; i < NUM_COOLDOWNS; i++) {
		if (spell_info[skill].applied_cooldowns[i] > 0) {
			start_action_cooldown(ch, i, MODIFY_COOLDOWN(spell_info[skill].applied_cooldowns[i], get_character_speed(ch, i, weapon)));
		}
	}
}
#endif

void start_action_cooldown(struct char_data * ch, action_type act_type, int duration)
{
	char svar[50];

	/* Format the sVariables - Always duration first. */
	sprintf(svar, "%d", duration);

	if (act_type == atMOVE)
	  attach_mud_event(new_mud_event(eMOVEACTION, ch, svar), duration);
	else if (act_type == atSTANDARD)
	  attach_mud_event(new_mud_event(eSTANDARDACTION, ch, svar), duration);
};
