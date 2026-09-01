/**
 * @file mud_event.h                      LuminariMUD
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

#include "event_runtime.h"
#include "mud_event_callback.h"

#if defined(LUMINARI_ENABLE_EVENT_ROLLBACK) || defined(LUMINARI_EVENT_ROLLBACK_TESTS)
#include "dgscript/dg_event_rollback.h"
#endif

struct char_data;
struct region_data;

#define EVENT_WORLD 0
#define EVENT_DESC 1
#define EVENT_CHAR 2
#define EVENT_ROOM 3
#define EVENT_REGION 4
#define EVENT_OBJECT 5

#define MUD_EVENT_DURABLE_FORMAT_VERSION 1U
#define MUD_EVENT_MAX_PERSISTED_USES 100000

enum mud_event_storage_class
{
  MUD_EVENT_TRANSIENT = 0,
  MUD_EVENT_RECONSTRUCTABLE,
  MUD_EVENT_COPYOVER_PRESERVED,
  MUD_EVENT_PERSISTED
};

enum mud_event_offline_policy
{
  MUD_EVENT_OFFLINE_DISCARD = 0,
  MUD_EVENT_OFFLINE_RECONSTRUCT,
  MUD_EVENT_OFFLINE_PAUSE,
  MUD_EVENT_OFFLINE_ELAPSE
};

enum mud_event_payload_policy
{
  MUD_EVENT_PAYLOAD_NONE = 0,
  MUD_EVENT_PAYLOAD_USES
};

enum mud_event_restore_status
{
  MUD_EVENT_RESTORE_OK = 0,
  MUD_EVENT_RESTORE_EXPIRED,
  MUD_EVENT_RESTORE_INVALID_ARGUMENT,
  MUD_EVENT_RESTORE_INVALID_FORMAT,
  MUD_EVENT_RESTORE_UNKNOWN_TYPE,
  MUD_EVENT_RESTORE_CLASS_MISMATCH,
  MUD_EVENT_RESTORE_SCHEMA_MISMATCH,
  MUD_EVENT_RESTORE_OWNER_MISMATCH,
  MUD_EVENT_RESTORE_PAYLOAD_MALFORMED,
  MUD_EVENT_RESTORE_DUPLICATE,
  MUD_EVENT_RESTORE_ADMISSION_FAILED
};

#define NEW_EVENT(event_id, struct, var, time)                                                     \
  (attach_mud_event(new_mud_event(event_id, struct, var), time))

typedef enum
{
  eNULL,                          /*0*/
  ePROTOCOLS,                     /* The Protocol Detection Event */
  eWHIRLWIND,                     /* The Whirlwind Attack */
  eCASTING,                       //  casting time
  eLAYONHANDS,                    //  lay on hands
  /*5*/ eTREATINJURY,             //  treat injury
  eTAUNT,                         //  taunt
  eTAUNTED,                       //  taunted
  eMUMMYDUST,                     //  mummy dust
  eDRAGONKNIGHT,                  //  dragon knight
  /*10*/ eGREATERRUIN,            //  greater ruin
  eHELLBALL,                      //  hellball
  eEPICMAGEARMOR,                 //  epic mage armor
  eEPICWARDING,                   //  epic warding
  ePREPARING,                     //  memorization
  /*15*/ eSTUNNED,                //  stunning fist stun
  eSTUNNINGFIST,                  //  stunner's cooldown for stunning fist
  eCRAFTING,                      //  crafting event
  eCRYSTALFIST,                   //  crystal fist cooldown
  eCRYSTALBODY,                   //  crystal body cooldown
  /*20*/ eRAGE,                   //  rage skill cooldown
  eACIDARROW,                     //  acid arrow damage event
  eD_ROLL,                        //  rogue's defensive roll cooldown
  ePURIFY,                        //  paladin's disease removal skill cooldown
  eC_ANIMAL,                      //  animal companion cooldown
  /*25*/ eC_FAMILIAR,             //  familiar cooldown
  eC_MOUNT,                       //  paladin's called mount cooldown
  eIMPLODE,                       //  implode damage event
  eSMITE_EVIL,                    //  smite eeeevil cooldown
  ePERFORM,                       //  Bard performance
  /*30*/ ePURGEMOB,               //  mob purge
  eICE_STORM,                     //  storm of vengeance - ice storm
  eCHAIN_LIGHTNING,               //  storm of vengeance - chain lightning
  eDARKNESS,                      //  darkness room event
  eMAGIC_FOOD,                    // magic food/drink cooldown
  /*35*/ eFISTED,                 // being fisted
  eWAIT,                          // replace WAIT_STATE with wait event
  eTURN_UNDEAD,                   // turn undead
  eSPELLBATTLE,                   // spellbattle
  eFALLING,                       // char falling
  /*40*/ eCHECK_OCCUPIED,         // Event to check that a room is occupied (for wilderness)
  eTRACKS,                        // Tracks in the room, decay on event processing.
  eWILD_SHAPE,                    // Wild shape event
  eSHIELD_RECOVERY,               // Recovery from shield punch
  eCOMBAT_ROUND,                  // Combat round
  /*45*/ eSTANDARDACTION,         // Standard action cooldown
  eMOVEACTION,                    // Move action cooldown
  eWHOLENESSOFBODY,               /* Wholeness of Body, Monk Healing Feat */
  eEMPTYBODY,                     /* Empty Body, Monk Concealment Feat */
  eQUIVERINGPALM,                 /* cooldown for quivering palm */
  /*50*/ eSWIFTACTION,            /* Swift action cooldown */
  eTRAPTRIGGERED,                 /* Trap Triggered */
  eSURPRISE_ACCURACY,             /* rage power surprise accuracy */
  ePOWERFUL_BLOW,                 /* rage power powerful blow */
  eRENEWEDVIGOR,                  /* Renewed Vigor, Berserker Healing Feat */
  /*55*/ eCOME_AND_GET_ME,        /* rage power 'come and get me' */
  eANIMATEDEAD,                   /* cool down for animate dead feat */
  eVANISH,                        /* vanish concealment */
  eVANISHED,                      /* vanish daily cooldown */
  eINTIMIDATED,                   /* intimidated victim! */
  /*60*/ eINTIMIDATE_COOLDOWN,    /* cooldown to reuse intimidate */
  eLIGHTNING_ARC,                 /* cooldown to reuse lightning arc */
  eACID_DART,                     /* cooldown to reuse acid dart */
  eFIRE_BOLT,                     /* cooldown to reuse fire bolt */
  eICICLE,                        /* cooldown to reuse icicle */
  /*65*/ eSTRUGGLE,               /* struggle cooldown (escape from grapple) */
  eCURSE_TOUCH,                   /* cooldown to reuse curse touch */
  eSMITE_GOOD,                    //  smite goooodies cooldown
  eSMITE_DESTRUCTION,             //  destructive smite cooldown
  eDESTRUCTIVE_AURA,              //  destructive aura cooldown
  /*70*/ eEVIL_TOUCH,             /*more domain powers*/
  eGOOD_TOUCH,                    /*more domain powers*/
  eHEALING_TOUCH,                 /*more domain powers*/
  eEYE_OF_KNOWLEDGE,              /*more domain powers*/
  eBLESSED_TOUCH,                 /*more domain powers*/
  /*75*/ eLAWFUL_WEAPON,          /*more domain powers*/
  eCOPYCAT,                       /*more domain powers*/
  eMASS_INVIS,                    /*more domain powers*/
  eAURA_OF_PROTECTION,            /*more domain powers*/
  eBATTLE_RAGE,                   /*more domain powers*/
  /*80*/ eCRYSTALFIST_AFF,        //  crystal fist affect
  eCRYSTALBODY_AFF,               //  crystal body affect
  eBARDIC_PERFORMANCE,            /* Retired bard performance event ID. */
  eENCOUNTER_REG_RESET,           // Reset event for encounter regions.
  eSEEKER_ARROW,                  /*pew pew seeker arrows!*/
  /*85*/ eIMBUE_ARROW,            /*pew pew imbued arrows!*/
  eDEATHARROW,                    /*pew pew arrow of death!*/
  eARROW_SWARM,                   /*pew (+20 more pews) swarm of arrows!*/
  eRENEWEDDEFENSE,                /* Renewed Defense, Stalwart Defender Healing Feat */
  eLAST_WORD,                     //  stalwart defender's 'last word' cooldown
  /*90*/ eSMASH_DEFENSE,          //  stalwart defender's 'last word' cooldown
  eDEFENSIVE_STANCE,              //  defensive stance skill cooldown
  eCRIPPLING_CRITICAL,            /* duelist cirppling critical */
  eQUEST_COMPLETE,                /* char completed a quest */
  eSLA_LEVITATE,                  /* innate levitate */
  /*95*/ eSLA_DARKNESS,           /* innate darkness */
  eSLA_FAERIE_FIRE,               /* innate faerie fire */
  eDRACBREATH,                    // Sorcerer draconic heritage breath weapon
  eDRACCLAWS,                     // Sorcerer draconic heritage claws attacks
  ePREPARATION,                   /* new spell preparation system */
  /*100*/ eCRAFT,                 /* NewCraft */
  eCOPYOVER,                      /* copyover event */
  eCOLLECT_DELAY,                 /* autocollect event */
  eARCANEADEPT,                   // Sorcerer metamagic adept feat uses
  eARMOR_SPECAB_BLINDING,         /* cooldown event for blinding armor special ability */
                                  /*105*/
  eITEM_SPECAB_HORN_OF_SUMMONING, /* cooldown event for the horn of summoning special ability */
  eMUTAGEN,
  eCURING_TOUCH,                    // alchemical discovery curing touch
  ePSYCHOKINETIC,                   // alchemical discovery psychokinetic tincture
  eIMPROMPT,                        /*slice & dice!  impromptu sneak attacks*/
  /*110*/ eINVISIBLE_ROGUE,         /* invisible rogue, SLA */
  eSACRED_FLAMES,                   /*flaming sacred unarmed attacks!*/
  eINNER_FIRE,                      /*inner fire, lots of juicy bonuses!*/
  ePIXIEDUST,                       // pixie dust used with shifter form: pixie
  eEFREETIMAGIC,                    // efreeti magic used with shifter form: efreeti
  /*115*/ eDRAGONMAGIC,             // dragon magic used with shifter form: dragon
  eSLA_STRENGTH,                    /* innate strength*/
  eSLA_ENLARGE,                     /* innate enlarge */
  eSLA_INVIS,                       /* innate invisibility */
  eCONCUSSIVEONSLAUGHT,             // concussive onsalught psionic power
  /*120*/ eCHANNELSPELL,            // countdown for channel spell ability
  ePOWERLEECH,                      // power leech ability
  ePSIONICFOCUS,                    // psionic focus ability
  eDOUBLEMANIFEST,                  // double manifest ability
  eSUMMONSHADOW,                    // shadowdancer summon shadow
  /*125*/ eSHADOWILLUSION,          // shadowdancer shadow illusion
  eSHADOWCALL,                      // shadowdancer  shadow call
  eSHADOWJUMP,                      // shadowdancer  shadow jump
  eSHADOWPOWER,                     // shadowdancer  shadow power
  eTOUCHOFCORRUPTION,               // touch of corruption - blackguard
  /*130*/ eCHANNELENERGY,           // channel positive/negative energy
  eLICH_TOUCH,                      // lich touch event
  eLICH_REJUV,                      // lich rejuv event
  eLICH_FEAR,                       // lich fear event
  eJUDGEMENT,                       // inquisitor judgement ability
  /*135*/ eBANE,                    // inquisitor bane ability
  eTRUEJUDGEMENT,                   // inquisitor true judgement ability
  eSPIRITUALWEAPON,                 // spiritual weapon spell
  eDANCINGWEAPON,                   // dancing weapon spell
  eHOLYJAVELIN,                     // holy javelin spell
  /*140*/ eITEM_SPECAB_ITEM_SUMMON, // summon item
  eCHILDRENOFTHENIGHT,              // children of the night vampire ability
  eVAMPIREENERGYDRAIN,              // energy drain vampire ability
  eMASTERMIND,                      // mastermind ability
  eINSECTBEING,                     /* insect being ability */
  /*145*/
  eBLUR_ATTACK_DELAY,                /* stop blur attack proccing too much :) */
  eTINKER,                           // use the rock gnome tinker ability
  eMOONBEAM,                         // moon beam spell lingering effect
  eDRAGBREATH,                       // Dragonborn breath weapon
  eMANYSHOT,                         // Manyshot command cooldown (Ranger Hunter Perk)
  eARROW_STORM,                      // Arrow Storm (Ranger Hunter Capstone) cooldown
  eCATSCLAWS,                        // tabaxi Cats Claws ability
  eSTONESENDURANCE,                  // goliath stones endurance ability
  eAQUEOUSORB,                       // aqueous orb spell
  eVAMPIREBLOODDRAIN,                // vampire blood drain ability
  eEVOBREATH,                        // eidolon evolution breath weapon
  eC_EIDOLON,                        //  call eidolon cooldown
  eTOUCHOFUNDEATH,                   // necromancer touch of undeath ability
  eSTRENGTHOFHONOR,                  // strength of honor knight of the crown ability
  eCROWNOFKNIGHTHOOD,                // crown of knighthood knight of the crown ability
  eSOULOFKNIGHTHOOD,                 // soul of knighthood knight of the sword ability
  eINSPIRECOURAGE,                   // inspire courage ability
  eWISDOMOFTHEMEASURE,               // wisdom of the measure ability
  eFINALSTAND,                       // final stand ability
  eKNIGHTHOODSFLOWER,                // knighthood's flower ability
  eRALLYINGCRY,                      // rallying cry ability
  eCOSMICUNDERSTANDING,              // foretell and prescience abilites
  eDRAGOONPOINTS,                    // dragoon points
  eC_DRAGONMOUNT,                    // call dragon mount
  eREGENERATION,                     // resource regeneration event
  eDEVICE_CREATION,                  // artificer device creation
  eDEVICE_PROGRESS,                  // artificer device creation progress updates
  eBREWING,                          /* Potion brewing event */
  eBEACON_OF_HOPE,                   /* Beacon of Hope daily cooldown */
  eFIST_OF_FOUR_THUNDERS,            /* Fist of Four Thunders lightning strike */
  eSAVAGE_CHARGE_USED,               /* Savage Charge used this rage */
  eDIVINE_SACRIFICE,                 /* Divine Sacrifice damage transfer cooldown */
  eRADIANT_AURA,                     /* Radiant Aura periodic undead damage */
  ePALADIN_CHANNEL_ENERGY,           /* Paladin Channel Energy perk daily uses */
  eMASS_CURE_WOUNDS,                 /* Mass Cure Wounds daily uses */
  eFERAL_CHARGE_USED,                /* Beast Master Feral Charge used this combat */
  eDEVICE_REPAIR,                    // artificer device repair
  eCURTAIN_CALL_COOLDOWN,            /* Curtain Call 5-minute cooldown */
  ePERFECT_TEMPO_HIT_THIS_ROUND,     /* Bard Perfect Tempo - tracked hit this round */
  eUNIVERSAL_MUTAGEN_COOLDOWN,       /* Alchemist Universal Mutagen 30-minute lockout */
  eDEFLECTIVE_SCREEN_HIT_THIS_ROUND, /* Psionicist Deflective Screen - first hit DR per round */
  eACCELERATED_MANIFESTATION_USED,   /* Psionicist Accelerated Manifestation - used this combat */
  eGRAVITY_WELL_USED,                /* Psionicist Gravity Well - used this combat */
  eSINGULAR_IMPACT_USED,             /* Psionicist Singular Impact - used today */
  ePERFECT_DEFLECTION_USED,          /* Psionicist Perfect Deflection - used today */
  eECTOPLASMIC_ARTISAN_USED, /* Psionicist Ectoplasmic Artisan - PSP reduction used this encounter */
  eRAPID_MANIFESTER_USED,   /* Psionicist Rapid Manifester - action reduction used this encounter */
  eASTRAL_JUGGERNAUT_USED,  /* Psionicist Astral Juggernaut - used today */
  ePERFECT_FABRICATOR_USED, /* Psionicist Perfect Fabricator - used today */
  eINTIMIDATE_SWIFT,        /* Blackguard Command the Weak swift intimidate gate */
  eFEAR_ESCALATION,         /* Blackguard Sovereign of Terror fear escalation per round */
  eMIDNIGHT_EDICT,          /* Blackguard Midnight Edict daily cooldown */
  ePROFANE_DOMINION_DAMAGE, /* Blackguard Profane Dominion periodic damage */
  ePROFANE_WEAPON_BOND,     /* Blackguard Profane Weapon Bond encounter cooldown */
  eRELENTLESS_ASSAULT,      /* Blackguard Relentless Assault per-round gate */
  eUNHOLY_BLITZ,            /* Blackguard Unholy Blitz haste burst */
  eAVATAR_OF_PROFANITY,     /* Blackguard Avatar of Profanity daily cooldown */
  eCATACLYSMIC_SMITE,       /* Blackguard Cataclysmic Smite daily cooldown */
  eGRAVEBORN_VIGOR,         /* Blackguard Graveborn Vigor threshold cooldown */
  eSINISTER_RECOVERY,       /* Blackguard Sinister Recovery self-heal cooldown */
  eSHADE_STEP,              /* Blackguard Shade Step usage cooldown */
  eUNDYING_VIGOR,           /* Blackguard Undying Vigor death-save daily cooldown */
  eEMPOWERED_JUDGMENT_DUAL, /* Inquisitor Empowered Judgment dual judgment encounter gate */
  eSWIFT_SPELLCASTER_USED,  /* Inquisitor Swift Spellcaster casting time reduction used */
  eJUDGMENT_RECOVERY_USED,  /* Inquisitor Judgment Recovery kill-triggered use used */
  eSPELL_METAMASTERY_USED,  /* Inquisitor Spell Metamastery 5-minute cooldown */
  eDIVINE_SPELLSTRIKE_USED, /* Inquisitor Divine Spellstrike daily use */
  eINEXORABLE_JUDGMENT_USED,    /* Inexorable Judgment daily use */
  eSUPREME_SPELLCASTING_USED,   /* Supreme Spellcasting daily free cast */
  eINSTANT_DEATH_USED,          /* Inquisitor Instant Death daily use */
  eTRUE_SEEING_DETECT_INVIS,    /* Inquisitor perk: Detect Invisibility daily use */
  eTRUE_SEEING_TRUE_SEEING,     /* Inquisitor perk: True Seeing daily use */
  eAURA_READING_SENSE_LIFE,     /* Inquisitor perk: Sense Life daily use */
  eAURA_READING_DETECT_ALIGN,   /* Inquisitor perk: Detect Alignment daily use */
  eLEGENDARY_RESILIENCE_USED,   /* Inquisitor Legendary Resilience auto-save cooldown */
  ePERFECT_ADAPTATION_COOLDOWN, /* Inquisitor Perfect Adaptation 5-minute cooldown */
  eAASIMAR_HEALING_HANDS,       /* Aasimar Healing Hands daily uses */
  eAASIMAR_LIGHT_BEARER,        /* Aasimar Light Bearer daily uses */
  eROL_YGGDRASIL_RELEASE,       /* Converted Yggdrasil branch entangle release */
  eROL_SEELIE_FAERIE_FIRE,      /* Converted Seelie faerie-fire cooldown */
  eROL_BARBAZU_BLOODLOSS,       /* Converted Barbazu glaive recurring blood loss */
  eROL_DROW_DECAY,              /* Converted drow-equipment surface decay */
  eROL_DEATHS_HEAD_SEED,        /* Converted Death's Head implanted-seed growth */
  eROL_SPIDERHAUNT_MAGGOTS,     /* Converted Spiderhaunt delayed maggot sensation */
  eDRAGON_ATTACK_COOLDOWN,      /* Shared short cooldown for innate dragon attacks */
  eROL_CALM,                    /* Calm feat daily use cooldown */
  eROL_CALL_LYCANTHROPE_CHARM,  /* Converted call-lycanthrope charm check */
  eROL_TAZRIKS_FRENZIED_HOUND,  /* Converted Tazrik's hound recurring strike */
  eMUD_EVENT_COUNT
} event_id;

/* probably a smart place to mention to not forget to update:
   act.informative.c (if you want do_affects to show status)
   players.c (if you want it to save)
 */

struct mud_event_list
{
  const char *event_name;
  mud_event_callback_func func;
  int iEvent_Type;

  /* Extended fields for centralized handling */
  const char *completion_msg; /* Message when countdown completes */
  const char *recovery_msg;   /* Message for daily use recovery */
  int feat_num;               /* Associated feat (FEAT_UNDEFINED if none) */
  int daily_uses;             /* Non-feat daily uses (0 if feat-based) */
};

struct mud_event_data
{
  struct event_runtime_handle runtime_handle; /***< Native timed-event identity. */
#if defined(LUMINARI_ENABLE_EVENT_ROLLBACK) || defined(LUMINARI_EVENT_ROLLBACK_TESTS)
  event_handle_t rollback_handle; /***< Temporary physical-legacy fallback. */
#endif
  event_id iId;         /***< General ID reference */
  void *pStruct;        /***< Pointer to NULL, Descriptor, Character .... */
  char *sVariables;     /***< String variable */
  struct game_event_owner owner; /***< Stable scheduler owner handle. */
  bool owner_detached;  /***< Owner list was detached before deferred cleanup. */
};

struct mud_event_persistence_policy
{
  enum mud_event_storage_class storage_class;
  enum mud_event_offline_policy offline_policy;
  enum mud_event_payload_policy payload_policy;
  unsigned int schema_version;
};

struct mud_event_durable_record
{
  event_id event_type;
  unsigned int schema_version;
  int64_t owner_id;
  int64_t remaining_ticks;
  int64_t saved_at_epoch;
  int payload_value;
};

/* Externals */
extern struct list_data *world_events;
extern struct mud_event_list mud_event_index[];
extern const size_t mud_event_index_count;

/* Local Functions */
void init_events(void);
bool mud_event_runtime_init(void);
const struct mud_event_persistence_policy *mud_event_persistence_policy(event_id iId);
const char *mud_event_storage_class_name(enum mud_event_storage_class storage_class);
const char *mud_event_restore_status_name(enum mud_event_restore_status status);
bool mud_event_legacy_persistence_writer_enabled(void);
bool mud_event_make_durable_record(struct char_data *ch, struct mud_event_data *pMudEvent,
                                   int64_t saved_at_epoch,
                                   struct mud_event_durable_record *record);
enum mud_event_restore_status
mud_event_restore_character_record(struct char_data *ch,
                                   const struct mud_event_durable_record *record,
                                   int64_t now_epoch);
struct mud_event_data *new_mud_event(event_id iId, void *pStruct, const char *sVariables);
void attach_mud_event(struct mud_event_data *pMudEvent, long time);
bool mud_event_is_live(const struct mud_event_data *pMudEvent);
long mud_event_remaining(const struct mud_event_data *pMudEvent);
void mud_event_cancel(struct mud_event_data *pMudEvent);
void mud_event_detach_owner(struct mud_event_data *pMudEvent);
struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id iId);
struct mud_event_data *room_has_mud_event(struct room_data *rm, event_id iId);      // Ornir
struct mud_event_data *obj_has_mud_event(struct obj_data *obj, event_id iId);       // Ornir
struct mud_event_data *region_has_mud_event(struct region_data *reg, event_id iId); // Ornir
void clear_char_event_list(struct char_data *ch);
void clear_descriptor_event_list(struct descriptor_data *d);
void clear_obj_event_list(struct obj_data *obj);
void clear_room_event_list(struct room_data *rm);
void clear_region_event_list(struct region_data *reg);
void change_event_duration(struct char_data *ch, event_id iId, long time);
void change_event_svariables(struct char_data *ch, event_id iId, char *sVariables);
void event_cancel_specific(struct char_data *ch, event_id iId);

#define HAS_WAIT(ch) char_has_mud_event(ch, eWAIT)
/* note: ornir has (temporarily?) disabled this event */
#define SET_WAIT(ch, wait) attach_mud_event(new_mud_event(eWAIT, ch, NULL), wait)

/* Events */
MUD_EVENT_CALLBACK(event_countdown);
MUD_EVENT_CALLBACK(event_daily_use_cooldown);
MUD_EVENT_CALLBACK(get_protocols);
MUD_EVENT_CALLBACK(event_whirlwind);
MUD_EVENT_CALLBACK(event_casting);
MUD_EVENT_CALLBACK(event_preparing);
MUD_EVENT_CALLBACK(event_crafting);
MUD_EVENT_CALLBACK(event_acid_arrow);
MUD_EVENT_CALLBACK(event_concussive_onslaught);
MUD_EVENT_CALLBACK(event_power_leech);
MUD_EVENT_CALLBACK(event_implode);
MUD_EVENT_CALLBACK(event_ice_storm);
MUD_EVENT_CALLBACK(event_chain_lightning);
MUD_EVENT_CALLBACK(event_falling);
MUD_EVENT_CALLBACK(event_check_occupied);
MUD_EVENT_CALLBACK(event_tracks);
MUD_EVENT_CALLBACK(event_combat_round);
MUD_EVENT_CALLBACK(event_action_cooldown);
MUD_EVENT_CALLBACK(event_trap_triggered);
MUD_EVENT_CALLBACK(event_preparation);
MUD_EVENT_CALLBACK(event_craft); /* NewCraft */
MUD_EVENT_CALLBACK(event_copyover);
MUD_EVENT_CALLBACK(event_spiritual_weapon);
MUD_EVENT_CALLBACK(event_dancing_weapon);
MUD_EVENT_CALLBACK(event_holy_javelin);
MUD_EVENT_CALLBACK(event_moonbeam);
MUD_EVENT_CALLBACK(event_aqueous_orb);
MUD_EVENT_CALLBACK(event_device_progress);
MUD_EVENT_CALLBACK(event_device_creation);
MUD_EVENT_CALLBACK(event_device_repair);
MUD_EVENT_CALLBACK(event_rol_call_lycanthrope_charm);
MUD_EVENT_CALLBACK(event_rol_tazriks_frenzied_hound);

#ifdef LUMINARI_CUTEST
void mud_event_test_reset_cleanup_count(void);
int mud_event_test_cleanup_count(void);
#endif
#endif /* _MUD_EVENT_H_ */
