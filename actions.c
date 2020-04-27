/*
 * Luminari Action System
 * Author:  Ornir
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
#include "domains_schools.h"

/*  Attack action definitions - Define the relationships between
 *  AA_XXXXX and do_XXXXX. */
void (*attack_actions[NUM_ATTACK_ACTIONS])(struct char_data *ch,
                                           const char *argument,
                                           int cmd,
                                           int subcmd) = {
    do_trip,             /* AA_TRIP */
    do_charge,           /* AA_CHARGE */
    do_smiteevil,        /* AA_SMITE_EVIL */
    do_stunningfist,     /* AA_STUNNINGFIST */
    do_headbutt,         /* AA_HEDABUTT */
    do_kick,             /* AA_KICK */
    do_shieldpunch,      /* AA_SHIELDPUNCH */
    do_quiveringpalm,    /* AA_QUIVERINGPALM */
    do_supriseaccuracy,  /* AA_SUPRISE_ACCURACY */
    do_powerfulblow,     /* AA_POWERFUL_BLOW */
    do_comeandgetme,     /* AA_COME_AND_GET_ME */
    do_disarm,           /* AA_DISARM */
    do_smitegood,        /* AA_SMITE_GOOD */
    do_destructivesmite, /* AA_SMITE_DESTRUCTION */
    do_seekerarrow,      /* AA_SEEKER_ARROW */
    do_deatharrow,       /* AA_DEATH_ARROW */
    do_arrowswarm,       /* AA_ARROW_SWARM */
    do_faeriefire,       /* AA_FAERIE_FIRE */
    do_feint,            /* AA_FEINT */
    do_impromptu,        /* AA_IMPROMPT */
};

void update_msdp_actions(struct char_data *ch)
{
  char msdp_buffer[MAX_STRING_LENGTH];
  struct affected_type *af, *next;
  bool first = TRUE;

  /* MSDP */

  msdp_buffer[0] = '\0';
  if (ch && ch->desc)
  {
    //const char MsdpArrayStart[] = {(char) MSDP_ARRAY_OPEN, '\0'};
    //const char MsdpArrayStop[] = {(char) MSDP_ARRAY_CLOSE, '\0'};

    char buf[4000]; // Buffer for building the actions table for MSDP

    next = af->next;
    snprintf(buf, sizeof(buf), "%c%s%c%d"
                 "%c%s%c%d"
                 "%c%s%c%d",
            (char)MSDP_VAR, "STANDARD_ACTION", (char)MSDP_VAL, is_action_available(ch, atSTANDARD, FALSE),
            (char)MSDP_VAR, "MOVE_ACTION", (char)MSDP_VAL, is_action_available(ch, atMOVE, FALSE),
            (char)MSDP_VAR, "SWIFT_ACTION", (char)MSDP_VAL, is_action_available(ch, atSWIFT, FALSE));

    strlcat(msdp_buffer, buf, sizeof(msdp_buffer));
    first = FALSE;

    MSDPSetTable(ch->desc, eMSDP_ACTIONS, msdp_buffer);
    MSDPFlush(ch->desc, eMSDP_ACTIONS);
  }
}

/* Action Cooldown events are:
 *   eSTANDARDACTION
 *   eMOVEACTION
 *   eSWIFTACTION
 *
 * If a player has one of these events, that signifies that they do NOT have
 * that action available. */
EVENTFUNC(event_action_cooldown)
{
  struct mud_event_data *pMudEvent;
  struct char_data *ch = NULL;
  char buf[32];

  pMudEvent = (struct mud_event_data *)event_obj;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    break;
  default:
    break;
  }

  /* Lets show duration for immortals */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    snprintf(buf, sizeof(buf), " (%.2f sec)", (float)(atoi(pMudEvent->sVariables)) / 10);
  else
    *buf = '\0';

  switch (pMudEvent->iId)
  {
  case eSTANDARDACTION:
    if (!char_has_mud_event(ch, eSTANDARDACTION))
    {
      send_to_char(ch, "You may perform another standard action.%s\r\n", buf);
      // Update MSDP
      update_msdp_actions(ch);
    }
    break;
  case eMOVEACTION:
    if (!char_has_mud_event(ch, eMOVEACTION))
    {
      send_to_char(ch, "You may perform another move action.%s\r\n", buf);
      update_msdp_actions(ch);
    }
    break;
  case eSWIFTACTION:
    if (!char_has_mud_event(ch, eSWIFTACTION))
    {
      send_to_char(ch, "You may perform another swift action.%s\r\n", buf);
      update_msdp_actions(ch);
    }
    break;
  default:
    break;
  }

  return 0;
};

bool is_action_available(struct char_data *ch, action_type act_type, bool msg_to_char)
{
  bool result = TRUE;

  if (act_type == atSTANDARD)
  {
    /* Is ch on standard action cooldown? */
    if (char_has_mud_event(ch, eSTANDARDACTION))
    {
      if (msg_to_char)
        send_to_char(ch, "You don't have a standard action.\r\n");
      result = FALSE;
    }
    else
      result = TRUE;
  }
  else if (act_type == atMOVE)
  {
    /* Is ch on physical cooldown? */
    if (char_has_mud_event(ch, eMOVEACTION))
    {
      if (msg_to_char)
        send_to_char(ch, "You don't have a move action.\r\n");
      result = FALSE;
    }
    else
      result = TRUE;
  }
  else if (act_type == atSWIFT)
  {
    /* Is ch on swift action cooldown? */
    if (char_has_mud_event(ch, eSWIFTACTION))
    {
      if (msg_to_char)
        send_to_char(ch, "You don't have a swift action.\r\n");
      result = FALSE;
    }
    else
      result = TRUE;
  }
  return result;
};

/* Start cooldowns for a specific command. Read cooldown length from command table. */
void start_cmd_cooldown(struct char_data *ch, int cmd)
{
  int i;

  for (i = 0; i < NUM_ACTIONS; i++)
  {
    if (complete_cmd_info[cmd].action_cooldowns[i] > 0)
    {
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

  for (i = 0; i < NUM_COOLDOWNS; i++)
  {
    if (spell_info[skill].applied_cooldowns[i] > 0)
    {
      start_action_cooldown(ch, i, MODIFY_COOLDOWN(spell_info[skill].applied_cooldowns[i], get_character_speed(ch, i, weapon)));
    }
  }
}
#endif

void start_action_cooldown(struct char_data *ch, action_type act_type, int duration)
{
  char svar[50];

  /* Format the sVariables - Always duration first. */
  snprintf(svar, sizeof(svar), "%d", duration);

  if (act_type == atMOVE)
  {
    attach_mud_event(new_mud_event(eMOVEACTION, ch, svar), duration);
    if (AFF_FLAGGED(ch, AFF_STAGGERED))
      attach_mud_event(new_mud_event(eSTANDARDACTION, ch, svar), duration);
  }
  else if (act_type == atSTANDARD)
  {
    attach_mud_event(new_mud_event(eSTANDARDACTION, ch, svar), duration);
    if (AFF_FLAGGED(ch, AFF_STAGGERED))
      attach_mud_event(new_mud_event(eMOVEACTION, ch, svar), duration);
  }
  else if (act_type == atSWIFT)
  {
    attach_mud_event(new_mud_event(eSWIFTACTION, ch, svar), duration);
  }
};
