/**************************************************************************
 *  File: mud_event.c                                  Part of LuminariMUD *
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
#include "comm.h" /* For access to the game pulse */
#include "lists.h"
#include "mud_event.h"
#include "handler.h"
#include "wilderness.h"
#include "quest.h"
#include "mysql.h"
#include "act.h"

/* Global List */
struct list_data *world_events = NULL;

/* The mud_event_index[] is merely a tool for organizing events, and giving
 * them a "const char *" name to help in potential debugging */
struct mud_event_list mud_event_index[] = {
    /*0*/
    {"Null", NULL, -1},                                     /* eNULL */
    {"Protocol", get_protocols, EVENT_DESC},                /* ePROTOCOLS */
    {"Whirlwind", event_whirlwind, EVENT_CHAR},             /* eWHIRLWIND */
    {"Casting", event_casting, EVENT_CHAR},                 /* eCASTING */
    {"Lay on hands", event_daily_use_cooldown, EVENT_CHAR}, // eLAYONHANDS
    /*5*/
    {"Treat injury", event_countdown, EVENT_CHAR},    // eTREATINJURY
    {"Taunt Cool Down", event_countdown, EVENT_CHAR}, // eTAUNT
    {"Taunted", event_countdown, EVENT_CHAR},         // eTAUNTED
    {"Mummy dust", event_countdown, EVENT_CHAR},      // eMUMMYDUST
    {"Dragon knight", event_countdown, EVENT_CHAR},   //  eDRAGONKNIGHT
    /*10*/
    {"Greater ruin", event_countdown, EVENT_CHAR},       // eGREATERRUIN
    {"Hellball", event_countdown, EVENT_CHAR},           // eHELLBALL
    {"Epic mage armor", event_countdown, EVENT_CHAR},    // eEPICMAGEARMOR
    {"Epic warding", event_countdown, EVENT_CHAR},       // eEPICWARDING
    {"Preparing Spells", event_preparation, EVENT_CHAR}, //ePREPARING
    /*15*/
    {"Stunned", event_countdown, EVENT_CHAR},                //eSTUNNED
    {"Stunning fist", event_daily_use_cooldown, EVENT_CHAR}, //eSTUNNINGFIST
    {"Crafting", event_crafting, EVENT_CHAR},                //eCRAFTING
    {"Crystal fist", event_daily_use_cooldown, EVENT_CHAR},  //eCRYSTALFIST
    {"Crystal body", event_daily_use_cooldown, EVENT_CHAR},  //eCRYRSTALBODY
    /*20*/
    {"Rage", event_daily_use_cooldown, EVENT_CHAR},         //eRAGE
    {"Acid arrow", event_acid_arrow, EVENT_CHAR},           //eACIDARROW
    {"Defensive Roll", event_countdown, EVENT_CHAR},        // eD_ROLL
    {"Purify", event_countdown, EVENT_CHAR},                // ePURIFY
    {"Call Animal Companion", event_countdown, EVENT_CHAR}, // eC_ANIMAL
    {"Call Familiar", event_countdown, EVENT_CHAR},         // eC_FAMILIAR
    {"Call Mount", event_countdown, EVENT_CHAR},            // eC_MOUNT
    {"Implode", event_implode, EVENT_CHAR},                 //eIMPLODE
    {"Smite Evil", event_daily_use_cooldown, EVENT_CHAR},   // eSMITE_EVIL
    {"Perform", event_countdown, EVENT_CHAR},               // ePERFORM
    /*30*/
    {"Mob Purge", event_countdown, EVENT_CHAR},                 // ePURGEMOB
    {"SoV Ice Storm", event_ice_storm, EVENT_CHAR},             // eICE_STORM
    {"SoV Chain Lightning", event_chain_lightning, EVENT_CHAR}, // eCHAIN_LIGHTNING
    {"Darkness", event_countdown, EVENT_ROOM},                  /* eDARKNESS */
    {"Magic Food", event_countdown, EVENT_CHAR},                /* eMAGIC_FOOD */
    {"Fisted", event_countdown, EVENT_CHAR},                    /* eFISTED */
    {"Wait", event_countdown, EVENT_CHAR},                      /* eWAIT */
    {"Turn Undead", event_countdown, EVENT_CHAR},               /* eTURN_UNDEAD */
    {"SpellBattle", event_countdown, EVENT_CHAR},               /* eSPELLBATTLE */
    {"Falling", event_falling, EVENT_CHAR},                     /* eFALLING */
    /*40*/
    {"Check Occupied", event_check_occupied, EVENT_ROOM},            /* eCHECK_OCCUPIED */
    {"Tracks", event_tracks, EVENT_ROOM},                            /* eTRACKS */
    {"Wild Shape", event_daily_use_cooldown, EVENT_CHAR},            /* eWILD_SHAPE */
    {"Shield Recovery", event_countdown, EVENT_CHAR},                /* eSHIELD_RECOVERY */
    {"Combat Round", event_combat_round, EVENT_CHAR},                /* eCOMBAT_ROUND */
    {"Standard Action Cooldown", event_action_cooldown, EVENT_CHAR}, /* eSTANDARDACTION */
    {"Move Action Cooldown", event_action_cooldown, EVENT_CHAR},     /* eMOVEACTION */
    {"Wholeness of Body", event_countdown, EVENT_CHAR},              // eWHOLENESSOFBODY
    {"Empty Body", event_countdown, EVENT_CHAR},                     // eEMPTYBODY
    {"Quivering Palm", event_daily_use_cooldown, EVENT_CHAR},        //eQUIVERINGPALM
    /*50*/
    {"Swift Action Cooldown", event_action_cooldown, EVENT_CHAR}, // eSWIFTACTION
    {"Trap Triggered", event_trap_triggered, EVENT_CHAR},         // eTRAPTRIGGERED */
    {"Suprise Accuracy", event_countdown, EVENT_CHAR},            //eSUPRISE_ACCURACY
    {"Powerful Blow", event_countdown, EVENT_CHAR},               //ePOWERFUL_BLOW
    {"Renewed Vigor", event_countdown, EVENT_CHAR},               // eRENEWEDVIGOR
    {"Come and Get Me!", event_countdown, EVENT_CHAR},            //eCOME_AND_GET_ME
    {"Animate Dead", event_daily_use_cooldown, EVENT_CHAR},       //eANIMATEDEAD
    {"Vanish", event_countdown, EVENT_CHAR},                      //eVANISH
    {"Vanish Cool Down", event_daily_use_cooldown, EVENT_CHAR},   //eVANISHED
    {"Intimidated", event_countdown, EVENT_CHAR},                 //eINTIMIDATED
    /*60*/
    {"Intimidated Cool Down", event_countdown, EVENT_CHAR},           //eINTIMIDATE_COOLDOWN
    {"Lightning Arc Cooldown", event_daily_use_cooldown, EVENT_CHAR}, // eLIGHTNING_ARC
    {"Acid Dart Cooldown", event_daily_use_cooldown, EVENT_CHAR},     // eACID_DART
    {"Fire Bolt Cooldown", event_daily_use_cooldown, EVENT_CHAR},     // eFIRE_BOLT
    {"Icicle Cooldown", event_daily_use_cooldown, EVENT_CHAR},        // eICICLE
    {"Struggle Cooldown", event_countdown, EVENT_CHAR},               // eSTRUGGLE
    {"Curse Touch Cooldown", event_daily_use_cooldown, EVENT_CHAR},   // eCURSE_TOUCH
    {"Smite Good", event_daily_use_cooldown, EVENT_CHAR},             // eSMITE_GOOD
    {"Destructive Smite", event_daily_use_cooldown, EVENT_CHAR},      // eSMITE_DESTRUCTION
    {"Destructive Aura", event_daily_use_cooldown, EVENT_CHAR},       // eDESTRUCTIVE_AURA
    /*70*/
    {"Evil Touch", event_daily_use_cooldown, EVENT_CHAR},         // eEVIL_TOUCH
    {"Good Touch", event_daily_use_cooldown, EVENT_CHAR},         // eGOOD_TOUCH
    {"Healing Touch", event_daily_use_cooldown, EVENT_CHAR},      // eHEALING_TOUCH
    {"Eye of Knowledge", event_daily_use_cooldown, EVENT_CHAR},   // eEYE_OF_KNOWLEDGE
    {"Blessed Touch", event_daily_use_cooldown, EVENT_CHAR},      // eBLESSED_TOUCH
    {"Lawful Weapon", event_daily_use_cooldown, EVENT_CHAR},      // eLAWFUL_WEAPON
    {"Copycat", event_daily_use_cooldown, EVENT_CHAR},            // eCOPYCAT
    {"Mass Invis", event_daily_use_cooldown, EVENT_CHAR},         // eMASS_INVIS
    {"Aura of Protection", event_daily_use_cooldown, EVENT_CHAR}, // eAURA_OF_PROTECTION
    {"Battle Rage", event_daily_use_cooldown, EVENT_CHAR},        // eBATTLE_RAGE
    /*80*/
    {"Crystal fist", event_countdown, EVENT_CHAR},                //eCRYSTALFIST_AFF
    {"Crystal body", event_countdown, EVENT_CHAR},                //eCRYRSTALBODY_AFF
    {"Bardic Performance", event_bardic_performance, EVENT_CHAR}, /* eBARDIC_PERFORMANCE */
    {"Encounter Region Reset", event_countdown, EVENT_REGION},    // eENCOUNTER_REG_RESET
    {"Seeker Arrow", event_daily_use_cooldown, EVENT_CHAR},       // eSEEKER_ARROW
    {"Imbue Arrow", event_daily_use_cooldown, EVENT_CHAR},        // eIMBUE_ARROW
    {"Arrow of Death", event_daily_use_cooldown, EVENT_CHAR},     //eDEATHARROW
    {"Swarm of Arrows", event_daily_use_cooldown, EVENT_CHAR},    //eARROW_SWARM
    {"Renewed Defense", event_countdown, EVENT_CHAR},             // eRENEWEDDEFENSE
    {"Last Word", event_countdown, EVENT_CHAR},                   // eLAST_WORD
    /*90*/
    {"Smash Defense", event_countdown, EVENT_CHAR},             // eSMASH_DEFENSE
    {"Defensive Stance", event_daily_use_cooldown, EVENT_CHAR}, //eDEFENSIVE_STANCE
    {"Crippled by Critical", event_countdown, EVENT_CHAR},      //eCRIPPLING_CRITICAL
    {"Quest Completed!", event_countdown, EVENT_CHAR},          //eQUEST_COMPLETE
    {"Levitate", event_daily_use_cooldown, EVENT_CHAR},         //eSLA_LEVITATE
    /*95*/
    {"Darkness", event_daily_use_cooldown, EVENT_CHAR},                                 //eSLA_DARKNESS
    {"Faerie Fire", event_daily_use_cooldown, EVENT_CHAR},                              //eSLA_FAERIE_FIRE
    {"Draconic Heritage Breath Weapon Cooldown", event_daily_use_cooldown, EVENT_CHAR}, // eDRACBREATH
    {"Draconic Heritage Claws Attack Cooldown", event_daily_use_cooldown, EVENT_CHAR},  // eDRACCLAWS
    {"Spell Preparation", event_preparation, EVENT_CHAR},                               //ePREPARATION
    /*100*/
    {"Craft", event_craft, EVENT_CHAR},                                          /* eCRAFT - NewCraft */
    {"Copyover Event!", event_copyover, EVENT_CHAR},                             /* eCOPYOVER - copyover delay */
    {"Autocollect delay", event_countdown, EVENT_CHAR},                          //eCOLLECT_DELAY
    {"Metamagic Adept Usage Cooldown", event_daily_use_cooldown, EVENT_CHAR},    // eARCANEADEPT
    {"Armor SpecAb Cooldown: Blinding", event_daily_use_cooldown, EVENT_OBJECT}, // eARMOR_SPECAB_BLINDING
    /*105*/
    {"Item SpecAb Cooldown: Horn of Summoning", event_daily_use_cooldown, EVENT_OBJECT}, // eITEM_SPECAB_HORN_OF_SUMMONING
    {"Mutagens/Cognatogens", event_daily_use_cooldown, EVENT_CHAR},                      //eMUTAGEN
    {"Curing Touch", event_daily_use_cooldown, EVENT_CHAR},                              // eCURING_TOUCH
    {"Psychokinetic Tincture", event_daily_use_cooldown, EVENT_CHAR},                    // ePSYCHOKINETIC
    {"Impromptu Sneak Attack", event_daily_use_cooldown, EVENT_CHAR},                    // eIMPROMPT
    /*110*/
    {"Invisible Rogue Cool Down", event_daily_use_cooldown, EVENT_CHAR}, //eINVISIBLE_ROGUE
    {"Sacred Flames Cool Down", event_daily_use_cooldown, EVENT_CHAR},   //eSACRED_FLAMES
    {"Inner Fire Cool Down", event_daily_use_cooldown, EVENT_CHAR},      //eINNER_FIRE
    {"Pixie Dust Cool Down", event_daily_use_cooldown, EVENT_CHAR},      // ePIXIEDUST
    {"Efreeti Magic Cool Down", event_daily_use_cooldown, EVENT_CHAR},      // eEFREETIMAGIC
    {"Dragon Magic Cool Down", event_daily_use_cooldown, EVENT_CHAR},      // eDRAGONMAGIC

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
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  struct room_data *room = NULL;
  struct obj_data *obj = NULL;
  room_vnum *rvnum = NULL;
  room_rnum rnum = NOWHERE;
  region_vnum *regvnum = NULL;
  region_rnum regrnum = NOWHERE;
  //obj_vnum *obj_vnum = NULL;
  //obj_rnum obj_rnum = NOWHERE;

  char **tokens; /* Storage for tokenized encounter room vnums */
  char **it;     /* Token iterator */

  pMudEvent = (struct mud_event_data *)event_obj;

  if (!pMudEvent)
    return 0;

  if (!pMudEvent->iId)
    return 0;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    break;
  case EVENT_OBJECT:
    obj = (struct obj_data *)pMudEvent->pStruct;
    //obj_rnum = real_obj(*obj_vnum);
    //obj = &obj[real_obj(obj_rnum)];
    break;
  case EVENT_ROOM:
    rvnum = (room_vnum *)pMudEvent->pStruct;
    rnum = real_room(*rvnum);
    room = &world[real_room(rnum)];
    break;
  case EVENT_REGION:
    regvnum = (region_vnum *)pMudEvent->pStruct;
    regrnum = real_region(*regvnum);
    log("LOG: EVENT_REGION case in EVENTFUNC(event_countdown): Region VNum %d, RNum %d", *regvnum, regrnum);
    break;
  default:
    break;
  }

  switch (pMudEvent->iId)
  {
  case eC_ANIMAL:
    send_to_char(ch, "You are now able to 'call companion' again.\r\n");
    break;
  case eC_FAMILIAR:
    send_to_char(ch, "You are now able to 'call familiar' again.\r\n");
    break;
  case eC_MOUNT:
    send_to_char(ch, "You are now able to 'call mount' again.\r\n");
    break;
  case eCRYSTALBODY_AFF:
    send_to_char(ch, "Your body loses its crystal-like properties.\r\n");
    break;
  case eCRYSTALFIST_AFF:
    send_to_char(ch, "Your body loses its crystal-like properties.\r\n");
    break;
  case eDARKNESS:
    REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_DARK);
    send_to_room(rnum, "The dark shroud disappates.\r\n");
    break;
  case eD_ROLL:
    send_to_char(ch, "You are now able to 'defensive roll' again.\r\n");
    break;
  case eLAST_WORD:
    send_to_char(ch, "You are now able to get the 'last word' in again.\r\n");
    break;
  case eDRAGONKNIGHT:
    send_to_char(ch, "You are now able to cast Dragon Knight again.\r\n");
    break;
  case eEPICMAGEARMOR:
    send_to_char(ch, "You are now able to cast Epic Mage Armor again.\r\n");
    break;
  case eEPICWARDING:
    send_to_char(ch, "You are now able to cast Epic Warding again.\r\n");
    break;
  case eFISTED:
    send_to_char(ch, "The magic fist holding you in place dissolves into nothingness.\r\n");
    break;
  case eGREATERRUIN:
    send_to_char(ch, "You are now able to cast Greater Ruin again.\r\n");
    break;
  case eHELLBALL:
    send_to_char(ch, "You are now able to cast Hellball again.\r\n");
    break;
  case eVANISHED:
    send_to_char(ch, "You are now able to vanish again.\r\n");
    break;
  case eINVISIBLE_ROGUE:
    send_to_char(ch, "You are now able to use your 'invisible rogue' ability again.\r\n");
    break;
  case eLAYONHANDS:
    send_to_char(ch, "You are now able to lay on hands again.\r\n");
    break;
  case eMAGIC_FOOD:
    send_to_char(ch, "You feel able to eat magical food again.\r\n");
    break;
  case eMUMMYDUST:
    send_to_char(ch, "You are now able to cast Mummy Dust again.\r\n");
    break;
  case ePERFORM:
    send_to_char(ch, "You are once again prepared to perform.\r\n");
    break;
  case ePURGEMOB:
    send_to_char(ch, "You must return to your home plane!\r\n");
    act("With a sigh of relief $n fades out of this plane!",
        FALSE, ch, NULL, NULL, TO_ROOM);
    extract_char(ch);
    break;
  case ePURIFY:
    send_to_char(ch, "You are now able to 'purify' again.\r\n");
    break;
  case eRAGE:
    send_to_char(ch, "You are now able to Rage again.\r\n");
    break;
  case eSACRED_FLAMES:
    send_to_char(ch, "You are now able to use Sacred Flames again.\r\n");
    break;
  case eINNER_FIRE:
    send_to_char(ch, "You are now able to use Inner Fire again.\r\n");
    break;
  case eMUTAGEN:
    send_to_char(ch, "You are now able to prepare another mutagen or cognatogen again.\r\n");
    break;
  case ePSYCHOKINETIC:
    send_to_char(ch, "You are now able to apply a new psychokinetic tincture.\r\n");
    break;
  case eDEFENSIVE_STANCE:
    send_to_char(ch, "You are now able to use your Defensive Stance again.\r\n");
    break;
  case eARROW_SWARM:
    send_to_char(ch, "You are now able to use your swarm of arrows again.\r\n");
    break;
  case eSEEKER_ARROW:
    send_to_char(ch, "You regain a usage of your seeker arrow.\r\n");
    break;
  case eIMPROMPT:
    send_to_char(ch, "You regain a usage of your impromptu sneak attack.\r\n");
    break;
  case eIMBUE_ARROW:
    send_to_char(ch, "You regain a usage of your imbue arrow.\r\n");
    break;
  case eSMITE_EVIL:
    send_to_char(ch, "You are once again prepared to smite your evil foes.\r\n");
    break;
  case eSMITE_GOOD:
    send_to_char(ch, "You are once again prepared to smite your good foes.\r\n");
    break;
  case eSMITE_DESTRUCTION:
    send_to_char(ch, "You are once again prepared to smite your foes.\r\n");
    break;
  case eSTUNNED:
    send_to_char(ch, "You are now free from the stunning affect.\r\n");
    break;
  case eSUPRISE_ACCURACY:
    send_to_char(ch, "You are now able to use suprise accuracy again.\r\n");
    break;
  case eCOME_AND_GET_ME:
    send_to_char(ch, "You are now able to use 'come and get me' again.\r\n");
    break;
  case ePOWERFUL_BLOW:
    send_to_char(ch, "You are now able to use powerful blow again.\r\n");
    break;
  case eANIMATEDEAD:
    send_to_char(ch, "You are now able to animate dead again.\r\n");
    break;
  case eSTUNNINGFIST:
    send_to_char(ch, "You are now able to strike with your stunning fist again.\r\n");
    break;
  case eQUIVERINGPALM:
    send_to_char(ch, "You are now able to strike with your quivering palm again.\r\n");
    break;
  case eDEATHARROW:
    send_to_char(ch, "You are now able to imbue an arrow with death.\r\n");
    break;
  case eVANISH:
    send_to_char(ch, "Your 'vanished' state returns to normal...\r\n");
    break;
  case eTAUNT:
    send_to_char(ch, "You are now able to taunt again.\r\n");
    break;
  case eTAUNTED:
    send_to_char(ch, "You feel the effects of the taunt wear off.\r\n");
    break;
  case eCRIPPLING_CRITICAL:
    send_to_char(ch, "You feel the effects of the crippling critical wear off.\r\n");
    break;
  case eINTIMIDATED:
    send_to_char(ch, "You feel the effects of the intimidation wear off.\r\n");
    break;
  case eINTIMIDATE_COOLDOWN:
    send_to_char(ch, "You are now able to intimidate again.\r\n");
    break;
  case eEMPTYBODY:
    send_to_char(ch, "You are now able to use Empty Body again.\r\n");
    break;
  case eWHOLENESSOFBODY:
    send_to_char(ch, "You are now able to use Wholeness of Body again.\r\n");
    break;
  case eRENEWEDVIGOR:
    send_to_char(ch, "You are now able to use Renewed Vigor again.\r\n");
    break;
  case eRENEWEDDEFENSE:
    send_to_char(ch, "You are now able to use Renewed Defense again.\r\n");
    break;
  case eTREATINJURY:
    send_to_char(ch, "You are now able to treat injuries again.\r\n");
    break;
  case eSTRUGGLE:
    if (AFF_FLAGGED(ch, AFF_GRAPPLED)) /*no need for message if not grappling*/
      send_to_char(ch, "You are now able to 'struggle' again.\r\n");
    break;
  case eWAIT:
    send_to_char(ch, "You are able to act again.\r\n");
    break;
  case eTURN_UNDEAD:
    send_to_char(ch, "You are able to turn undead again.\r\n");
    break;
  case eCOLLECT_DELAY:
    perform_collect(ch, FALSE);
    break;
  case eQUEST_COMPLETE:
    complete_quest(ch);
    break;
  case eSPELLBATTLE:
    send_to_char(ch, "You are able to use spellbattle again.\r\n");
    SPELLBATTLE(ch) = 0;
    break;
  case eENCOUNTER_REG_RESET:
    /* Testing */
    if (regrnum == NOWHERE)
    {
      log("SYSERR: event_countdown for eENCOUNTER_REG_RESET, region out of bounds.");
      break;
    }
    log("Encounter Region '%s' with vnum: %d reset.", region_table[regrnum].name, region_table[regrnum].vnum);

    if (pMudEvent->sVariables == NULL)
    {
      /* This encounter region has no encounter rooms. */
      log("SYSERR: No encounter rooms set for encounter region vnum: %d", *regvnum);
    }
    else
    {
      /* Process all encounter rooms for this region */
      tokens = tokenize(pMudEvent->sVariables, ",");

      for (it = tokens; it && *it; ++it)
      {
        room_vnum eroom_vnum;
        room_rnum eroom_rnum = NOWHERE;
        int x, y;

        sscanf(*it, "%d", &eroom_vnum);
        eroom_rnum = real_room(eroom_vnum);
        log("LOG: Processing encounter room vnum: %d", eroom_vnum);

        if (eroom_rnum == NOWHERE)
        {
          log("  ERROR: Encounter room is NOWHERE");
          continue;
        }

        /* First check that the encounter room is empty of players */
        if (world[eroom_rnum].people != NULL)
        {
          /* Someone is in the room, so skip this one. */
          continue;
        }

        /* Find a location in the region where this room will be placed, 
             it can not be the same coords as a static room and noone should be at those coordinates. */
        int ctr = 0;
        do
        {

          /* Generate the random point */
          get_random_region_location(*regvnum, &x, &y);

          /* Check for a static room at this location. */
          if (find_room_by_coordinates(x, y) == NOWHERE)
          {
            /* Make sure the sector types match. */
            if (world[eroom_rnum].sector_type == get_modified_sector_type(GET_ROOM_ZONE(eroom_rnum), x, y))
            {
              break;
            }
          }
        } while (++ctr < 128);

        /* Build the room. */
        //assign_wilderness_room(eroom_rnum, x, y);
        world[eroom_rnum].coords[0] = x;
        world[eroom_rnum].coords[1] = y;
      }
      initialize_wilderness_lists();
    }

    return 60 RL_SEC;

    break;
  default:
    break;
  }

  return 0;
}

EVENTFUNC(event_daily_use_cooldown)
{
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  struct obj_data *obj = NULL;
  int cooldown = 0;
  int uses = 0;
  int nonfeat_daily_uses = 0;
  int featnum = 0;
  char buf[128];

  pMudEvent = (struct mud_event_data *)event_obj;

  if (!pMudEvent)
    return 0;

  if (!pMudEvent->iId)
    return 0;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    break;
  case EVENT_OBJECT:
    obj = (struct obj_data *)pMudEvent->pStruct;
    break;
  default:
    return 0;
  }

  if (pMudEvent->sVariables == NULL)
  {
    /* This is odd - This field should always be populated for daily-use abilities,
     * maybe some legacy code or bad id. */
    log("SYSERR: sVariables field is NULL for daily-use-cooldown-event: %d", pMudEvent->iId);
  }
  else
  {
    if (sscanf(pMudEvent->sVariables, "uses:%d", &uses) != 1)
    {
      log("SYSERR: In event_daily_use_cooldown, bad sVariables for daily-use-cooldown-event: %d", pMudEvent->iId);
      uses = 0;
    }
  }

  switch (pMudEvent->iId)
  {
  case eDEATHARROW:
    featnum = FEAT_ARROW_OF_DEATH;
    send_to_char(ch, "You are now able to imbue an arrow with death again.\r\n");
    break;
  case eQUIVERINGPALM:
    featnum = FEAT_QUIVERING_PALM;
    send_to_char(ch, "You are now able to strike with your quivering palm again.\r\n");
    break;
  case eSTUNNINGFIST:
    featnum = FEAT_STUNNING_FIST;
    send_to_char(ch, "You are now able to strike with your stunning fist again.\r\n");
    break;
  case eANIMATEDEAD:
    featnum = FEAT_ANIMATE_DEAD;
    send_to_char(ch, "You are now able to animate dead again.\r\n");
    break;
  case eWILD_SHAPE:
    featnum = FEAT_WILD_SHAPE;
    send_to_char(ch, "You may assume your wild shape again.\r\n");
    break;
  case eCRYSTALBODY:
    featnum = FEAT_CRYSTAL_BODY;
    send_to_char(ch, "You may harden your crystalline body again.\r\n");
    break;
  case eCRYSTALFIST:
    featnum = FEAT_CRYSTAL_FIST;
    send_to_char(ch, "You may enhance your unarmed attacks again.\r\n");
    break;
  case eSLA_LEVITATE:
    featnum = FEAT_SLA_LEVITATE;
    send_to_char(ch, "One of your levitate uses has recovered.\r\n");
    break;
  case eSLA_DARKNESS:
    featnum = FEAT_SLA_DARKNESS;
    send_to_char(ch, "One of your darkness uses has recovered.\r\n");
    break;
  case eSLA_FAERIE_FIRE:
    featnum = FEAT_SLA_FAERIE_FIRE;
    send_to_char(ch, "One of your faerie fire uses has recovered.\r\n");
    break;
  case eLAYONHANDS:
    featnum = FEAT_LAYHANDS;
    send_to_char(ch, "One of your lay on hands uses has recovered.\r\n");
    break;
  case ePURIFY:
    featnum = FEAT_REMOVE_DISEASE;
    send_to_char(ch, "One of your remove disease (purify) uses has recovered.\r\n");
    break;
  case eRAGE:
    featnum = FEAT_RAGE;
    send_to_char(ch, "One of your rage uses has recovered.\r\n");
    break;
  case eSACRED_FLAMES:
    featnum = FEAT_SACRED_FLAMES;
    send_to_char(ch, "One of your sacred flames uses has recovered.\r\n");
    break;
  case eINNER_FIRE:
    featnum = FEAT_INNER_FIRE;
    send_to_char(ch, "One of your inner fire uses has recovered.\r\n");
    break;
  case eMUTAGEN:
    featnum = FEAT_MUTAGEN;
    send_to_char(ch, "One of your mutagens or cognatogens are ready to prepare.\r\n");
    break;
  case ePSYCHOKINETIC:
    featnum = FEAT_PSYCHOKINETIC;
    send_to_char(ch, "Your psychokinetic tincture is ready to prepare.\r\n");
    break;
  case eCURING_TOUCH:
    featnum = FEAT_CURING_TOUCH;
    send_to_char(ch, "One of your curing touch uses has recovered.\r\n");
    break;
  case eDEFENSIVE_STANCE:
    featnum = FEAT_DEFENSIVE_STANCE;
    send_to_char(ch, "One of your defensive stance uses has recovered.\r\n");
    break;
  case eVANISHED:
    featnum = FEAT_VANISH;
    send_to_char(ch, "One of your vanish uses has recovered.\r\n");
    break;
  case eINVISIBLE_ROGUE:
    featnum = FEAT_INVISIBLE_ROGUE;
    send_to_char(ch, "One of your invisible rogue uses has recovered.\r\n");
    break;
  case eARROW_SWARM:
    featnum = FEAT_SWARM_OF_ARROWS;
    send_to_char(ch, "One of your swarm of arrows uses has recovered.\r\n");
    break;
  case eSEEKER_ARROW:
    featnum = FEAT_SEEKER_ARROW;
    send_to_char(ch, "One of your seeker arrow uses has recovered.\r\n");
    break;
  case eIMPROMPT:
    featnum = FEAT_IMPROMPTU_SNEAK_ATTACK;
    send_to_char(ch, "One of your impromptu sneak attack uses has recovered.\r\n");
    break;
  case eIMBUE_ARROW:
    featnum = FEAT_IMBUE_ARROW;
    send_to_char(ch, "One of your imbue arrow uses has recovered.\r\n");
    break;
  case eSMITE_EVIL:
    featnum = FEAT_SMITE_EVIL;
    send_to_char(ch, "One of your smite evil uses has recovered.\r\n");
    break;
  case eSMITE_GOOD:
    featnum = FEAT_SMITE_GOOD;
    send_to_char(ch, "One of your smite good uses has recovered.\r\n");
    break;
  case eSMITE_DESTRUCTION:
    featnum = FEAT_DESTRUCTIVE_SMITE;
    send_to_char(ch, "One of your destructive smite uses has recovered.\r\n");
    break;
  case eTURN_UNDEAD:
    featnum = FEAT_TURN_UNDEAD;
    send_to_char(ch, "One of your turn undead uses has recovered.\r\n");
    break;
  case eLIGHTNING_ARC:
    featnum = FEAT_LIGHTNING_ARC;
    send_to_char(ch, "One of your lightning arc uses has recovered.\r\n");
    break;
  case eACID_DART:
    featnum = FEAT_ACID_DART;
    send_to_char(ch, "One of your acid dart uses has recovered.\r\n");
    break;
  case eFIRE_BOLT:
    featnum = FEAT_FIRE_BOLT;
    send_to_char(ch, "One of your fire bolt uses has recovered.\r\n");
    break;
  case eICICLE:
    featnum = FEAT_ICICLE;
    send_to_char(ch, "One of your icicle uses has recovered.\r\n");
    break;
  case eCURSE_TOUCH:
    featnum = FEAT_CURSE_TOUCH;
    send_to_char(ch, "One of your curse touch uses has recovered.\r\n");
    break;
  case eDESTRUCTIVE_AURA:
    featnum = FEAT_DESTRUCTIVE_AURA;
    send_to_char(ch, "One of your destructive aura uses has recovered.\r\n");
    break;
  case eEVIL_TOUCH:
    featnum = FEAT_EVIL_TOUCH;
    send_to_char(ch, "One of your evil touch uses has recovered.\r\n");
    break;
  case eGOOD_TOUCH:
    featnum = FEAT_GOOD_TOUCH;
    send_to_char(ch, "One of your good touch uses has recovered.\r\n");
    break;
  case eHEALING_TOUCH:
    featnum = FEAT_HEALING_TOUCH;
    send_to_char(ch, "One of your healing touch uses has recovered.\r\n");
    break;
  case eEYE_OF_KNOWLEDGE:
    featnum = FEAT_EYE_OF_KNOWLEDGE;
    send_to_char(ch, "One of your eye of knowledge uses has recovered.\r\n");
    break;
  case eBLESSED_TOUCH:
    featnum = FEAT_BLESSED_TOUCH;
    send_to_char(ch, "One of your blessed touch uses has recovered.\r\n");
    break;
  case eCOPYCAT:
    featnum = FEAT_COPYCAT;
    send_to_char(ch, "One of your copycat uses has recovered.\r\n");
    break;
  case eMASS_INVIS:
    featnum = FEAT_MASS_INVIS;
    send_to_char(ch, "One of your mass invis uses has recovered.\r\n");
    break;
  case eAURA_OF_PROTECTION:
    featnum = FEAT_AURA_OF_PROTECTION;
    send_to_char(ch, "One of your aura of protection uses has recovered.\r\n");
    break;
  case eBATTLE_RAGE:
    featnum = FEAT_BATTLE_RAGE;
    send_to_char(ch, "One of your battle rage uses has recovered.\r\n");
    break;
  case eDRACBREATH:
    featnum = FEAT_DRACONIC_HERITAGE_BREATHWEAPON;
    send_to_char(ch, "One of your draconic heritage breath weapon uses has recovered.\r\n");
    break;
  case ePIXIEDUST:
    featnum = FEAT_PIXIE_DUST;
    send_to_char(ch, "One of your pixie dust uses has recovered.\r\n");
    break;
  case eEFREETIMAGIC:
    featnum = FEAT_EFREETI_MAGIC;
    send_to_char(ch, "One of your efreeti magic uses has recovered.\r\n");
    break;
  case eDRAGONMAGIC:
    featnum = FEAT_DRAGON_MAGIC;
    send_to_char(ch, "One of your dragon magic uses has recovered.\r\n");
    break;
  case eARCANEADEPT:
    featnum = FEAT_METAMAGIC_ADEPT;
    send_to_char(ch, "One of your metamagic (arcane) adept uses has recovered.\r\n");
    break;
  case eARMOR_SPECAB_BLINDING:
    featnum = FEAT_UNDEFINED;
    nonfeat_daily_uses = 2; /* 2 uses a day. */
    break;
  case eITEM_SPECAB_HORN_OF_SUMMONING:
    featnum = FEAT_UNDEFINED;
    nonfeat_daily_uses = 2; /* 2 uses a day. */
  default:
    break;
  }

  uses -= 1;
  if (uses > 0)
  {
    if (pMudEvent->sVariables != NULL)
      free(pMudEvent->sVariables);

    snprintf(buf, sizeof(buf), "uses:%d", uses);
    pMudEvent->sVariables = strdup(buf);

    if ((featnum == FEAT_UNDEFINED) && (nonfeat_daily_uses > 0))
    {
      /* 
        This is a 'daily' feature that is not controlled by a feat - for example a weapon or armor special ability. 
        In this case, the daily uses must be set above - variable nonfeat_daily_uses.
      */
      cooldown = (SECS_PER_MUD_DAY / nonfeat_daily_uses) RL_SEC;
    }
    else if (get_daily_uses(ch, featnum))
    { /* divide by 0! */
      cooldown = (SECS_PER_MUD_DAY / get_daily_uses(ch, featnum)) RL_SEC;
    }
  }

  return cooldown;
}

/* As of 3.63, there are only global, descriptor, and character events. This
 * is due to the potential scope of the necessary debugging if events were
 * included with rooms, objects, spells or any other structure type. Adding
 * events to these other systems should be just as easy as adding the current
 * library was, and should be available in a future release. - Vat 
 *
 * Update: Events have been added to objects, rooms and characters.
 *         Region support has also been added for wilderness regions.
 */
void attach_mud_event(struct mud_event_data *pMudEvent, long time)
{
  struct event *pEvent = NULL;

  struct descriptor_data *d = NULL;
  struct char_data *ch = NULL;
  struct room_data *room = NULL;
  struct region_data *region = NULL;
  struct obj_data *obj = NULL;

  room_vnum *rvnum = NULL;
  region_vnum *regvnum = NULL;

  pEvent = event_create(mud_event_index[pMudEvent->iId].func, pMudEvent, time);
  pEvent->isMudEvent = TRUE;
  pMudEvent->pEvent = pEvent;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_WORLD:
    add_to_list(pEvent, world_events);
    break;
  case EVENT_DESC:
    d = (struct descriptor_data *)pMudEvent->pStruct;
    add_to_list(pEvent, d->events);
    break;
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;

    if (ch->events == NULL)
      ch->events = create_list();

    add_to_list(pEvent, ch->events);
    break;
  case EVENT_OBJECT:
    obj = (struct obj_data *)pMudEvent->pStruct;

    if (obj->events == NULL)
      obj->events = create_list();

    add_to_list(pEvent, obj->events);
    break;
  case EVENT_ROOM:

    CREATE(rvnum, room_vnum, 1);
    *rvnum = *((room_vnum *)pMudEvent->pStruct);
    pMudEvent->pStruct = rvnum;
    room = &world[real_room(*rvnum)];

    //      log("[DEBUG] Adding Event %s to room %d",mud_event_index[pMudEvent->iId].event_name, room->number);

    if (room->events == NULL)
      room->events = create_list();

    add_to_list(pEvent, room->events);
    break;
  case EVENT_REGION:
    CREATE(regvnum, region_vnum, 1);
    *regvnum = *((region_vnum *)pMudEvent->pStruct);
    pMudEvent->pStruct = regvnum;

    log("TEST DEBUG REGION EVENTS: vnum %d rnum %d", *((region_vnum *)pMudEvent->pStruct), real_region(*regvnum));

    if (real_region(*regvnum) == NOWHERE)
    {
      log("SYSERR: Attempt to add event to out-of-range region!");
      free(regvnum);
      break;
    }

    region = &region_table[real_region(*regvnum)];

    if (region->events == NULL)
      region->events = create_list();

    add_to_list(pEvent, region->events);
    break;
  }
}

struct mud_event_data *new_mud_event(event_id iId, void *pStruct, const char *sVariables)
{
  struct mud_event_data *pMudEvent = NULL;
  char *varString = NULL;

  CREATE(pMudEvent, struct mud_event_data, 1);
  varString = (sVariables != NULL) ? strdup(sVariables) : NULL;

  pMudEvent->iId = iId;
  pMudEvent->pStruct = pStruct;
  pMudEvent->sVariables = varString;
  pMudEvent->pEvent = NULL;

  return (pMudEvent);
}

void free_mud_event(struct mud_event_data *pMudEvent)
{
  struct descriptor_data *d = NULL;
  struct char_data *ch = NULL;
  struct room_data *room = NULL;
  struct region_data *region = NULL;
  struct obj_data *obj = NULL;

  room_vnum *rvnum = NULL;
  region_vnum *regvnum = NULL;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type)
  {
  case EVENT_WORLD:
    remove_from_list(pMudEvent->pEvent, world_events);
    break;
  case EVENT_DESC:
    d = (struct descriptor_data *)pMudEvent->pStruct;
    remove_from_list(pMudEvent->pEvent, d->events);
    break;
  case EVENT_CHAR:
    ch = (struct char_data *)pMudEvent->pStruct;
    remove_from_list(pMudEvent->pEvent, ch->events);

    if (ch->events && ch->events->iSize == 0)
    {
      free_list(ch->events);
      ch->events = NULL;
    }
    break;
  case EVENT_OBJECT:
    obj = (struct obj_data *)pMudEvent->pStruct;
    remove_from_list(pMudEvent->pEvent, obj->events);

    if (obj->events && obj->events->iSize == 0)
    {
      free_list(obj->events);
      obj->events = NULL;
    }
    break;
  case EVENT_ROOM:
    /* Due to OLC changes, if rooms were deleted then the room we have in the event might be
       * invalid.  This entire system needs to be re-evaluated!  We should really use RNUM
       * and just get the room data ourselves.  Storing the room_data struct is asking for bad
       * news. */
    rvnum = (room_vnum *)pMudEvent->pStruct;

    room = &world[real_room(*rvnum)];

    //      log("[DEBUG] Removing Event %s from room %d, which has %d events.",mud_event_index[pMudEvent->iId].event_name, room->number, (room->events == NULL ? 0 : room->events->iSize));

    free(pMudEvent->pStruct);

    remove_from_list(pMudEvent->pEvent, room->events);

    if (room->events && room->events->iSize == 0)
    { /* Added the null check here. - Ornir*/
      free_list(room->events);
      room->events = NULL;
    }
    break;
  case EVENT_REGION:
    regvnum = (region_vnum *)pMudEvent->pStruct;

    region = &region_table[real_region(*regvnum)];

    free(pMudEvent->pStruct);

    remove_from_list(pMudEvent->pEvent, region->events);

    if (region->events && region->events->iSize == 0)
    { /* Added the null check here. - Ornir*/
      free_list(region->events);
      region->events = NULL;
    }
    break;
  }

  if (pMudEvent->sVariables != NULL)
    free(pMudEvent->sVariables);

  pMudEvent->pEvent->event_obj = NULL;
  free(pMudEvent);
}

struct mud_event_data *char_has_mud_event(struct char_data *ch, event_id iId)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;
  struct iterator_data it;

  if (ch->events == NULL)
    return NULL;

  if (ch->events->iSize == 0)
    return NULL;

  /*
  simple_list(NULL);
  while ((pEvent = (struct event *) simple_list(ch->events)) != NULL) {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *) pEvent->event_obj;
    if (pMudEvent->iId == iId) {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);
   */

  for (pEvent = (struct event *)merge_iterator(&it, ch->events);
       pEvent != NULL;
       pEvent = next_in_list(&it))
  {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  remove_iterator(&it);

  if (found)
    return (pMudEvent);

  return NULL;
}

struct mud_event_data *room_has_mud_event(struct room_data *rm, event_id iId)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (rm->events == NULL)
    return NULL;

  if (rm->events->iSize == 0)
    return NULL;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(rm->events)) != NULL)
  {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
    return (pMudEvent);

  return NULL;
}

struct mud_event_data *obj_has_mud_event(struct obj_data *obj, event_id iId)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (obj->events == NULL)
    return NULL;

  if (obj->events->iSize == 0)
    return NULL;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(obj->events)) != NULL)
  {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
    return (pMudEvent);

  return NULL;
}

struct mud_event_data *region_has_mud_event(struct region_data *reg, event_id iId)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (reg->events == NULL)
    return NULL;

  if (reg->events->iSize == 0)
    return NULL;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(reg->events)) != NULL)
  {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
    return (pMudEvent);

  return NULL;
}

/* remove world event */
struct mud_event_data *world_has_mud_event(event_id iId)
{
  return NULL;
}

void event_cancel_specific(struct char_data *ch, event_id iId)
{
  struct event *pEvent;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;
  struct iterator_data it;

  if (ch->events == NULL)
  {
    //act("ch->events == NULL, for $n.", FALSE, ch, NULL, NULL, TO_ROOM);
    //send_to_char(ch, "ch->events == NULL.\r\n");
    return;
  }

  if (ch->events->iSize == 0)
  {
    //act("ch->events->iSize == 0, for $n.", FALSE, ch, NULL, NULL, TO_ROOM);
    //send_to_char(ch, "ch->events->iSize == 0.\r\n");
    return;
  }

  for (pEvent = (struct event *)merge_iterator(&it, ch->events);
       pEvent != NULL;
       pEvent = next_in_list(&it))
  {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *)pEvent->event_obj;
    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  remove_iterator(&it);

  /* need to clear simple lists */
  /*
  simple_list(NULL);
  act("Clearing simple list for $n.", FALSE, ch, NULL, NULL, TO_ROOM);
  send_to_char(ch, "Clearing simple list.\r\n");
   */
  /* fill simple_list with ch's events, use it to try and find event ID */
  /*
  while ((pEvent = (struct event *) simple_list(ch->events)) != NULL) {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *) pEvent->event_obj;
    if (pMudEvent->iId == iId) {
      found = TRUE;
      break;
    }
  }
   */
  /* need to clear simple lists */
  /*
  simple_list(NULL);
  act("Clearing simple list for $n, 2nd time.", FALSE, ch, NULL, NULL, TO_ROOM);
  send_to_char(ch, "Clearing simple list, 2nd time.\r\n");
   */

  if (found)
  {
    //act("event found for $n, attempting to cancel", FALSE, ch, NULL, NULL, TO_ROOM);
    //send_to_char(ch, "Event found: %d.\r\n", iId);
    if (event_is_queued(pEvent))
      event_cancel(pEvent);
  }
  else
  {
    //act("event_cancel_specific did not find an event for $n.", FALSE, ch, NULL, NULL, TO_ROOM);
    //send_to_char(ch, "event_cancel_specific did not find an event.\r\n");
  }

  return;
}

void clear_char_event_list(struct char_data *ch)
{
  struct event *pEvent = NULL;
  struct iterator_data it;

  if (ch->events == NULL)
    return;

  if (ch->events->iSize == 0)
    return;

  /* This uses iterators because we might be in the middle of another
   * function using simple_list, and that method requires that we do not use simple_list again
   * on another list -> It generates unpredictable results.  Iterators are safe. */
  for (pEvent = (struct event *)merge_iterator(&it, ch->events);
       pEvent != NULL;
       pEvent = next_in_list(&it))
  {
    /* Here we have an issue - If we are currently executing an event, and it results in a char
     * having their events cleared (death) then we must be sure that we don't clear the executing
     * event!  Doing so will crash the event system. */

    if (event_is_queued(pEvent))
      event_cancel(pEvent);
    else if (ch->events->iSize == 1)
      break;
  }
  remove_iterator(&it);
}

void clear_room_event_list(struct room_data *rm)
{
  struct event *pEvent = NULL;

  if (rm->events == NULL)
    return;

  if (rm->events->iSize == 0)
    return;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(rm->events)) != NULL)
  {
    if (event_is_queued(pEvent))
      event_cancel(pEvent);
  }
  simple_list(NULL);
}

void clear_region_event_list(struct region_data *reg)
{
  struct event *pEvent = NULL;

  if (reg->events == NULL)
    return;

  if (reg->events->iSize == 0)
    return;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(reg->events)) != NULL)
  {
    if (event_is_queued(pEvent))
      event_cancel(pEvent);
  }
  simple_list(NULL);
}

/* ripley's version of change_event_duration
 * a function to adjust the event time of a given event
 */
void change_event_duration(struct char_data *ch, event_id iId, long time)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (ch->events->iSize == 0)
    return;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(ch->events)) != NULL)
  {

    if (!pEvent->isMudEvent)
      continue;

    pMudEvent = (struct mud_event_data *)pEvent->event_obj;

    if (pMudEvent->iId == iId)
    {
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
  {
    /* So we found the offending event, now build a new one, with the new time */
    attach_mud_event(new_mud_event(iId, pMudEvent->pStruct, pMudEvent->sVariables), time);
    if (event_is_queued(pEvent))
      event_cancel(pEvent);
  }
}

/* zusuk: change an event's svariables value */
void change_event_svariables(struct char_data *ch, event_id iId, char *sVariables)
{
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;
  long time = 0;

  if (ch->events->iSize == 0)
    return;

  simple_list(NULL);
  while ((pEvent = (struct event *)simple_list(ch->events)) != NULL)
  {

    if (!pEvent->isMudEvent)
      continue;

    pMudEvent = (struct mud_event_data *)pEvent->event_obj;

    if (pMudEvent->iId == iId)
    {
      time = event_time(pMudEvent->pEvent);
      found = TRUE;
      break;
    }
  }
  simple_list(NULL);

  if (found)
  {
    /* So we found the offending event, now build a new one, with the new time */
    attach_mud_event(new_mud_event(iId, pMudEvent->pStruct, sVariables), time);
    if (event_is_queued(pEvent))
      event_cancel(pEvent);
  }
}
