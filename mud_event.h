/**
 * @file mud_event.h
 * Mud_Event Header file.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * This source code, which was not part of the CircleMUD legacy code,
 * is attributed to:
 * Copyright 2012 by Joseph Arnusch.
 */

#ifndef _MUD_EVENT_H_
#define _MUD_EVENT_H_

#include "dg_event.h"

#define EVENT_WORLD 0
#define EVENT_DESC  1
#define EVENT_CHAR  2
#define EVENT_ROOM  3

#define NEW_EVENT(event_id, struct, var, time) (attach_mud_event(new_mud_event(event_id, struct,  var), time))

typedef enum {
  eNULL,
  ePROTOCOLS, /* The Protocol Detection Event */
  eWHIRLWIND, /* The Whirlwind Attack */
  eCASTING, //  casting time
  eLAYONHANDS, //  lay on hands
  eTREATINJURY, //  treat injury
  eTAUNT, //  taunt
  eTAUNTED, //  taunted
  eMUMMYDUST, //  mummy dust
  eDRAGONKNIGHT, //  dragon knight
  eGREATERRUIN, //  greater ruin
  eHELLBALL, //  hellball
  eEPICMAGEARMOR, //  epic mage armor
  eEPICWARDING, //  epic warding
  eMEMORIZING, //  memorization
  eSTUNNED, //  stunning fist stun
  eSTUNNINGFIST, //  stunner's cooldown for stunning fist
  eCRAFTING, //  crafting event
  eCRYSTALFIST, //  crystal fist cooldown
  eCRYSTALBODY, //  crystal body cooldown
  eRAGE, //  rage skill cooldown
  eACIDARROW, //  acid arrow damage event
  eD_ROLL, //  rogue's defensive roll cooldown
  ePURIFY, //  paladin's disease removal skill cooldown
  eC_ANIMAL, //  animal companion cooldown
  eC_FAMILIAR, //  familiar cooldown
  eC_MOUNT, //  paladin's called mount cooldown
  eIMPLODE, //  implode damage event
  eSMITE_EVIL, //  smite eeeevil cooldown
  ePERFORM, //  Bard performance
  ePURGEMOB, //  mob purge
  eICE_STORM, //  storm of vengeance - ice storm
  eCHAIN_LIGHTNING, //  storm of vengeance - chain lightning
  eDARKNESS, //  darkness room event
  eMAGIC_FOOD, // magic food/drink cooldown
  eFISTED, // being fisted
  eWAIT, // replace WAIT_STATE with wait event
  eTURN_UNDEAD, // turn undead
  eSPELLBATTLE, // spellbattle
  eFALLING, // char falling
  eCHECK_OCCUPIED, // Event to check that a room is occupied (for wilderness)
  eTRACKS, // Tracks in the room, decay on event processing.
  eWILD_SHAPE, // Wild shape event
  eSHIELD_RECOVERY, // Recovery from shield punch
  eCOMBAT_ROUND, // Combat round
  eSTANDARDACTION, // Standard action cooldown
  eMOVEACTION, // Move action cooldown
  eWHOLENESSOFBODY, /* Wholeness of Body, Monk Healing Feat */
  eEMPTYBODY, /* Empty Body, Monk Concealment Feat */
  eQUIVERINGPALM, /* cooldown for quivering palm */
  eSWIFTACTION, /* Swift action cooldown */
  eTRAPTRIGGERED, /* Trap Triggered */
  eSUPRISE_ACCURACY, /* rage power suprise accuracy */
  ePOWERFUL_BLOW, /* rage power powerful blow */
  eRENEWEDVIGOR, /* Renewed Vigor, Berserker Healing Feat */
  eCOME_AND_GET_ME, /* rage power 'come and get me' */
  eANIMATEDEAD, /* cool down for animate dead feat */
  eVANISH, /* vanish concealment */
  eVANISHED, /* vanish daily cooldown */
  eINTIMIDATED, /* intimidated victim! */
  eINTIMIDATE_COOLDOWN, /* cooldown to reuse intimidate */
  eLIGHTNING_ARC, /* cooldown to reuse lightning arc */
  eACID_DART, /* cooldown to reuse acid dart */
  eFIRE_BOLT, /* cooldown to reuse fire bolt */
  eICICLE, /* cooldown to reuse icicle */
  eSTRUGGLE, /* struggle cooldown (escape from grapple) */
  eCURSE_TOUCH, /* cooldown to reuse curse touch */
  eSMITE_GOOD, //  smite goooodies cooldown
  eSMITE_DESTRUCTION, //  destructive smite cooldown
  eDESTRUCTIVE_AURA, //  destructive aura cooldown
  eEVIL_TOUCH, /*more domain powers*/
  eGOOD_TOUCH, /*more domain powers*/
  eHEALING_TOUCH, /*more domain powers*/
  eEYE_OF_KNOWLEDGE, /*more domain powers*/
  eBLESSED_TOUCH, /*more domain powers*/
  eCOPYCAT, /*more domain powers*/
  eMASS_INVIS, /*more domain powers*/
  eAURA_OF_PROTECTION, /*more domain powers*/
  eBATTLE_RAGE, /*more domain powers*/
  eCRYSTALFIST_AFF, //  crystal fist affect
  eCRYSTALBODY_AFF, //  crystal body affect
  eBARDIC_PERFORMANCE, // bard performance/song
} event_id;

/* probaly a smart place to mention to not forget to update:
   act.informative.c (if you want do_affects to show status)
   players.c (if you want it to save)
 */

struct mud_event_list {
  const char *event_name;
  EVENTFUNC(*func);
  int iEvent_Type;
};

struct mud_event_data {
  struct event *pEvent; /***< Pointer reference to the event */
  event_id iId; /***< General ID reference */
  void *pStruct; /***< Pointer to NULL, Descriptor, Character .... */
  char *sVariables; /***< String variable */
};

/* Externals */
extern struct list_data *world_events;
extern struct mud_event_list mud_event_index[];

/* Local Functions */
void init_events(void);
struct mud_event_data *new_mud_event(event_id iId, void *pStruct, char *sVariables);
void attach_mud_event(struct mud_event_data *pMudEvent, long time);
void free_mud_event(struct mud_event_data *pMudEvent);
struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id iId);
struct mud_event_data *room_has_mud_event(struct room_data *rm, event_id iId); // Ornir
void clear_char_event_list(struct char_data *ch);
void clear_room_event_list(struct room_data *rm);
void change_event_duration(struct char_data *ch, event_id iId, long time);
void event_cancel_specific(struct char_data *ch, event_id iId);

#define HAS_WAIT(ch)            char_has_mud_event(ch, eWAIT)
/* note: ornir has (temporarily?) disabled this event */
#define SET_WAIT(ch, wait)      attach_mud_event(new_mud_event(eWAIT, ch, NULL), wait)

/* Events */
EVENTFUNC(event_countdown);
EVENTFUNC(event_daily_use_cooldown);
EVENTFUNC(get_protocols);
EVENTFUNC(event_whirlwind);
EVENTFUNC(event_casting);
EVENTFUNC(event_memorizing);
EVENTFUNC(event_crafting);
EVENTFUNC(event_acid_arrow);
EVENTFUNC(event_implode);
EVENTFUNC(event_ice_storm);
EVENTFUNC(event_chain_lightning);
EVENTFUNC(event_falling);;
EVENTFUNC(event_check_occupied);
EVENTFUNC(event_tracks);
EVENTFUNC(event_combat_round);
EVENTFUNC(event_action_cooldown);
EVENTFUNC(event_trap_triggered);
EVENTFUNC(event_bardic_performance);
#endif /* _MUD_EVENT_H_ */
