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

struct region_data;

#define EVENT_WORLD  0
#define EVENT_DESC   1
#define EVENT_CHAR   2
#define EVENT_ROOM   3
#define EVENT_REGION 4
#define EVENT_OBJECT 5

#define NEW_EVENT(event_id, struct, var, time) (attach_mud_event(new_mud_event(event_id, struct,  var), time))

typedef enum {
  eNULL, /*0*/
  ePROTOCOLS, /* The Protocol Detection Event */
  eWHIRLWIND, /* The Whirlwind Attack */
  eCASTING, //  casting time
  eLAYONHANDS, //  lay on hands
  /*5*/eTREATINJURY, //  treat injury
  eTAUNT, //  taunt
  eTAUNTED, //  taunted
  eMUMMYDUST, //  mummy dust
  eDRAGONKNIGHT, //  dragon knight
  /*10*/eGREATERRUIN, //  greater ruin
  eHELLBALL, //  hellball
  eEPICMAGEARMOR, //  epic mage armor
  eEPICWARDING, //  epic warding
  ePREPARING, //  memorization
  /*15*/eSTUNNED, //  stunning fist stun
  eSTUNNINGFIST, //  stunner's cooldown for stunning fist
  eCRAFTING, //  crafting event
  eCRYSTALFIST, //  crystal fist cooldown
  eCRYSTALBODY, //  crystal body cooldown
  /*20*/eRAGE, //  rage skill cooldown
  eACIDARROW, //  acid arrow damage event
  eD_ROLL, //  rogue's defensive roll cooldown
  ePURIFY, //  paladin's disease removal skill cooldown
  eC_ANIMAL, //  animal companion cooldown
  /*25*/eC_FAMILIAR, //  familiar cooldown
  eC_MOUNT, //  paladin's called mount cooldown
  eIMPLODE, //  implode damage event
  eSMITE_EVIL, //  smite eeeevil cooldown
  ePERFORM, //  Bard performance
  /*30*/ePURGEMOB, //  mob purge
  eICE_STORM, //  storm of vengeance - ice storm
  eCHAIN_LIGHTNING, //  storm of vengeance - chain lightning
  eDARKNESS, //  darkness room event
  eMAGIC_FOOD, // magic food/drink cooldown
  /*35*/eFISTED, // being fisted
  eWAIT, // replace WAIT_STATE with wait event
  eTURN_UNDEAD, // turn undead
  eSPELLBATTLE, // spellbattle
  eFALLING, // char falling
  /*40*/eCHECK_OCCUPIED, // Event to check that a room is occupied (for wilderness)
  eTRACKS, // Tracks in the room, decay on event processing.
  eWILD_SHAPE, // Wild shape event
  eSHIELD_RECOVERY, // Recovery from shield punch
  eCOMBAT_ROUND, // Combat round
  /*45*/eSTANDARDACTION, // Standard action cooldown
  eMOVEACTION, // Move action cooldown
  eWHOLENESSOFBODY, /* Wholeness of Body, Monk Healing Feat */
  eEMPTYBODY, /* Empty Body, Monk Concealment Feat */
  eQUIVERINGPALM, /* cooldown for quivering palm */
  /*50*/eSWIFTACTION, /* Swift action cooldown */
  eTRAPTRIGGERED, /* Trap Triggered */
  eSUPRISE_ACCURACY, /* rage power suprise accuracy */
  ePOWERFUL_BLOW, /* rage power powerful blow */
  eRENEWEDVIGOR, /* Renewed Vigor, Berserker Healing Feat */
  /*55*/eCOME_AND_GET_ME, /* rage power 'come and get me' */
  eANIMATEDEAD, /* cool down for animate dead feat */
  eVANISH, /* vanish concealment */
  eVANISHED, /* vanish daily cooldown */
  eINTIMIDATED, /* intimidated victim! */
  /*60*/eINTIMIDATE_COOLDOWN, /* cooldown to reuse intimidate */
  eLIGHTNING_ARC, /* cooldown to reuse lightning arc */
  eACID_DART, /* cooldown to reuse acid dart */
  eFIRE_BOLT, /* cooldown to reuse fire bolt */
  eICICLE, /* cooldown to reuse icicle */
  /*65*/eSTRUGGLE, /* struggle cooldown (escape from grapple) */
  eCURSE_TOUCH, /* cooldown to reuse curse touch */
  eSMITE_GOOD, //  smite goooodies cooldown
  eSMITE_DESTRUCTION, //  destructive smite cooldown
  eDESTRUCTIVE_AURA, //  destructive aura cooldown
  /*70*/eEVIL_TOUCH, /*more domain powers*/
  eGOOD_TOUCH, /*more domain powers*/
  eHEALING_TOUCH, /*more domain powers*/
  eEYE_OF_KNOWLEDGE, /*more domain powers*/
  eBLESSED_TOUCH, /*more domain powers*/
  /*75*/eLAWFUL_WEAPON, /*more domain powers*/
  eCOPYCAT, /*more domain powers*/
  eMASS_INVIS, /*more domain powers*/
  eAURA_OF_PROTECTION, /*more domain powers*/
  eBATTLE_RAGE, /*more domain powers*/
  /*80*/eCRYSTALFIST_AFF, //  crystal fist affect
  eCRYSTALBODY_AFF, //  crystal body affect
  eBARDIC_PERFORMANCE, // bard performance/song 
  eENCOUNTER_REG_RESET, // Reset event for encounter regions.          
  eSEEKER_ARROW, /*pew pew seeker arrows!*/
  /*85*/eIMBUE_ARROW, /*pew pew imbued arrows!*/
  eDEATHARROW, /*pew pew arrow of death!*/
  eARROW_SWARM, /*pew (+20 more pews) swarm of arrows!*/
  eRENEWEDDEFENSE, /* Renewed Defense, Stalwart Defender Healing Feat */
  eLAST_WORD, //  stalwart defender's 'last word' cooldown
  /*90*/eSMASH_DEFENSE, //  stalwart defender's 'last word' cooldown          
  eDEFENSIVE_STANCE, //  defensive stance skill cooldown
  eCRIPPLING_CRITICAL, /* duelist cirppling critical */
  eQUEST_COMPLETE, /* char completed a quest */
  eSLA_LEVITATE, /* innate levitate */
  /*95*/eSLA_DARKNESS, /* innate darkness */
  eSLA_FAERIE_FIRE, /* innate faerie fire */
  eDRACBREATH, // Sorcerer draconic heritage breath weapon
  eDRACCLAWS, // Sorcerer draconic heritage claws attacks
  ePREPARATION, /* new spell preparation system */
  eCRAFT, /* NewCraft */
  eCOPYOVER, /* copyover event */
  eCOLLECT_DELAY, /* autocollect event */
  eARCANEADEPT, // Sorcerer metamagic adept feat uses
  eARMOR_SPECAB_BLINDING, /* cooldown event for blinding armor special ability */
} event_id;

/* probably a smart place to mention to not forget to update:
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
struct mud_event_data *room_has_mud_event(struct room_data *rm, event_id iId);      // Ornir
struct mud_event_data *obj_has_mud_event(struct obj_data *obj, event_id iId);       // Ornir
struct mud_event_data *region_has_mud_event(struct region_data *reg, event_id iId); // Ornir
void clear_char_event_list(struct char_data *ch);
void clear_room_event_list(struct room_data *rm);
void clear_region_event_list(struct region_data *reg);
void change_event_duration(struct char_data *ch, event_id iId, long time);
void change_event_svariables(struct char_data * ch, event_id iId, char *sVariables);
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
EVENTFUNC(event_preparing);
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
EVENTFUNC(event_preparation);
EVENTFUNC(event_craft); /* NewCraft */
EVENTFUNC(event_copyover);
#endif /* _MUD_EVENT_H_ */
