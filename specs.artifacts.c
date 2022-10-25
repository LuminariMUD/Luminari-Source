/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\     Luminari Artifact System
/  File:       specs.artifacts.c
/  Created By: Outcast3 & Zusuk
\  Header:     specs.artifacts.h
/    Artifacts and Items of Power
\    Supporting functions
/  Created on September 1, 2022
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#undef NOT_CONVERTED
#ifdef NOT_CONVERTED

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "prototypes.h"
#include "specs.include.h"
#include "specs.prototypes.h"
#include "skillrec.h"
#include "spells.h"
#include "race_class.h"
#include "utils.h"

extern P_desc descriptor_list;
extern P_event event_type_list[];
extern P_event current_event;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *coin_names[];
extern char *command[];
extern const char *race_types[];
extern const char *class_types[];
extern const char *dirs[];
extern const char rev_dir[];
extern const int exp_table[TOTALCLASS][52];
extern const long boot_time;
extern const struct stat_data stat_factor[];
extern int planes_room_num[];
extern int racial_base[LAST_PC_RACE];
extern int top_of_world;
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

/* The staff of ancient oaks. */

static char TrorxekD[] = "TRORXEK_DEFENDER";
static char TrorxekC[] = "TRORXEK_CREPPINGDOOM";
static char TrorxekR[] = "TRORXEK_RECALL";
static char TrorxekM[] = "TRORXEK_MOONWELL";

#define TRORXEK_DEFENDER BIT_1     /* Defender Called this month? */
#define TRORXEK_CREEPINGDOOM BIT_2 /* Creeping Doom cast today? */
#define TRORXEK_RECALL BIT_3       /* Teleport called this hour? */
#define TRORXEK_MOONWELL BIT_4     /* Moonwell cast today? */

#define TRORXEK_DEFENDER_CMD "come oaken defender"
#define TRORXEK_CREEPINGDOOM_CMD "carpet of death"
#define TRORXEK_RECALL_CMD "forest path home"
#define TRORXEK_MOONWELL_CMD "moonlit path to"

#define OAKEN_DEFENDER_VNUM 1111

int OakenDefender(P_obj obj, P_char ch, int cmd, char *arg)
{
   /* Trorxek             Wielded initially by Liran                        */
   /*                                                                       */
   /* Classes Allowed: Druid                                                */
   /* Base Damage:     5d4                                                  */
   /* Bonuses:         +75 hits, +25 constitution.                          */
   /* Called Effects:  'summon oaken defender' Summons Treant.       1/week */
   /* Called Effects:  'carpet of death' Casts instant Doom.         1/hour */
   /* Called Effects:  'forest path home' Instant teleport to home. 1/hour */
   /* Called Effects:  'moonlit path to <target> Instant Moonwell.   1/hour */
   /* Continual Effects: None.                                              */
   /* Special Id:      Yes                                                  */
   P_char owner = NULL, treant = NULL, target = NULL;
   char idString[MAX_STRING_LENGTH], word[MAX_STRING_LENGTH];
   char word1[MAX_STRING_LENGTH], word2[MAX_STRING_LENGTH];
   char word3[MAX_STRING_LENGTH], word4[MAX_STRING_LENGTH];
   int calltype = 0, home = 0;
   struct affected_type af;

   /* backup the values */
   BACKUP_VARIABLES();

   PARSE_ARG(cmd, calltype, cmd);

   /* Initialize object */
   if (calltype == PROC_INITIALIZE)
   {
      ClearObjEvents(obj);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              TRORXEK_DEFENDER, PULSE_WEEK, TrorxekD);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              TRORXEK_CREEPINGDOOM, PULSE_DAY, TrorxekC);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              TRORXEK_RECALL, PULSE_HOUR, TrorxekR);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              TRORXEK_MOONWELL, PULSE_DAY, TrorxekM);

      BACKUP_OBJECT(obj);
      return IDX_COMMAND | IDX_WEAPON_CRIT | IDX_PERIODIC | IDX_IDENTIFY;
   }

   /* trap for special id */
   if (calltype == PROC_SPECIAL_ID)
   {
      sprintf(idString,
              "Called Effects:  (invoked by 'say'ing them)\n"
              "                 'summon oaken defender'  Summons Treant    1/month\n"
              "                 'carpet of death'   Casts Creeping Doom    1/hour\n"
              "                 'forest path home'  Recall to home tree   1/hour\n"
              "                 'moonlit path to'  Casts Moonwell          1/hour\n"
              "Combat Critical: Blinding Strike");
      specialProcedureIdentifyObject(obj, ch, calltype, arg,
                                     ID_CLASS_DRUID, 0, 0, idString);
      return TRUE;
   }

   if (calltype == PROC_EVENT)
   {
      /* Recharge spell effect from event */
      if (rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                TRORXEK_DEFENDER, TrorxekD) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                TRORXEK_CREEPINGDOOM, TrorxekC) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                TRORXEK_RECALL, TrorxekR) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                TRORXEK_MOONWELL, TrorxekM))
         return FALSE;

      owner = getObjectOwner(obj);

      if (owner)
         /* Now we check to be sure the guy wielding this falls under the */
         /* restrictions we have set */
         if (checkObjectRestrictions(ITEM_WEAPON, obj, owner, OBJ_RESTRICT_CLASS,
                                     OBJ_RESTRICT_DRUID))
         {
            performRestrictionPenalty(ITEM_WEAPON, obj, owner, OBJ_BURN);
            return TRUE;
         }
      /* Here on out, we are a valid wielder */
      RESTORE_WEAPON(obj);
      return TRUE;
   }

   /* If the player ain't here, why are we? */
   if (!ch || !obj)
      return FALSE;

   /* Check for God Reset of Item */
   if (godMaintenanceCommands(obj, ch, cmd, arg))
      return TRUE;

   /* Are we wielding it? */
   if (!OBJ_WORN(obj))
      return FALSE;

   /* Now process called commands. */
   if (word && ((cmd == CMD_SAY) || (cmd == CMD_SAY2)))
   {
      sscanf(arg, "%s %s %s %s \n", word1, word2, word3, word4);
      sprintf(word, "%s %s %s", word1, word2, word3);
      if (!strcmp(word, TRORXEK_DEFENDER_CMD))
      {
         if (!IS_SET(obj->value[0], TRORXEK_DEFENDER))
         {
            send_to_char("The staff refuses, you feel a sense that the staff doesn't want to oblidge you anymore, almost as if it dissapproves of you..", ch);
            return TRUE;

            if (get_char("OakenDefenderTreant") != NULL)
            {
               send_to_char("You may not summon more than one oaken servant!\n", ch);
               return (FALSE);
            }
            act("&+yYou call $p &n&+yfor the Oaken Defender..\n"
                "&+YMagical energy courses through the staff into the ground in a bright bolt.",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&+gThe ground trembles and shakes like an earthquake tremor!\n"
                "&+yA massive Treant grows out of the ground in seconds, towering over you!",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&+yIn your mind your hear a deep voice, 'How may I serve, master..'",
                FALSE, ch, obj, 0, TO_CHAR);
            act("$n &+ymutters something to &s $p.\n"
                "&+YMagical energy courses through $s's staff, into the ground in a bright bold.",
                FALSE, ch, obj, 0, TO_ROOM);
            act("&+gThe ground trembles and shakes like an earthquake tremor!\n"
                "&+yA massive Treant grows out of the ground in seconds, towering over you!",
                FALSE, ch, obj, 0, TO_ROOM);
            act("&+yThe Treant stands next to &n$n&n&+y, as silent as an ancient oak..",
                FALSE, ch, obj, 0, TO_ROOM);

            treant = read_mobile(OAKEN_DEFENDER_VNUM, VIRTUAL);
            if (!treant)
            {
               logit(LOG_DEBUG, "StaffAncientOaks(): mob %d not loadable", OAKEN_DEFENDER_VNUM);
               send_to_char("Bug in conjure elemental.  Tell a god!\n", ch);
               return FALSE;
            }

            if (ZONE_FLAGGED(GET_ROOM_ZONE(ch->in_room), ZONE_WILDERNESS))
            {
               X_LOC(treant) = world[ch->in_room].coords[0];
               Y_LOC(treant) = world[ch->in_room].coords[1];
            }

            char_to_room(treant, ch->in_room, 0);

            bzero(&af, sizeof(af));
            af.type = SPELL_CHARM_PERSON;
            af.duration = 9999;
            SET_CBIT(af.sets_affs, AFF_CHARM);
            add_follower(treant, ch, TRUE);
            affect_to_char(treant, &af);

            SET_BIT(obj->value[0], TRORXEK_DEFENDER);
            AddEvent(EVENT_OBJ_TIMER, PULSE_WEEK, TRUE, obj, TrorxekD);
            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', TRORXEK_DEFENDER_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, TRORXEK_CREEPINGDOOM_CMD))
      {
         if (!IS_SET(obj->value[0], TRORXEK_CREEPINGDOOM))
         {
            act("&+yYou call $p &n&+yfor the forest's carpet of death..\n"
                "&+LThe staff glows with a blackish aura as the spell energy flows into you..",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&n$n&+y mutters something to $s $p.\n"
                "&+LThe staff glows with a blackish aura as energy flows into $s.",
                FALSE, ch, obj, 0, TO_ROOM);

            spell_creeping(60, ch, 0, 0);

            SET_BIT(obj->value[0], TRORXEK_CREEPINGDOOM);
            AddEvent(EVENT_OBJ_TIMER, PULSE_DAY, TRUE, obj, TrorxekC);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', TRORXEK_CREEPINGDOOM_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, TRORXEK_RECALL_CMD))
      {
         if (!IS_SET(obj->value[0], TRORXEK_RECALL))
         {
            act("&+yYou call $p &n&+yfor the forest path home..\n"
                "&+gThe ground around you suddenly turns into tall grass, which grows up around you!",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&+gThen the grass dimmishes, you find yourself back in your home.",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&n$n &+ymutters something to $s $p.\n"
                "&+gTall grass suddenly appears beneath $n, and grows up around $n within seconds!",
                FALSE, ch, obj, 0, TO_ROOM);
            act("&+gThe grass consumes $n, which then vanishes along with $n into thin air..",
                FALSE, ch, obj, 0, TO_ROOM);

            home = ch->player.birthplace;
            treant = get_char("OakenDefenderTreant");
            if (treant)
            {
               if (world[ch->in_room].number == world[treant->in_room].number)
               {
                  char_from_room(treant);

                  if (ZONE_FLAGGED(GET_ROOM_ZONE(real_room(home)), ZONE_WILDERNESS))
                  {
                     X_LOC(ch) = world[real_room(home)].coords[0];
                     Y_LOC(ch) = world[real_room(home)].coords[1];
                  }

                  char_to_room(ch, real_room(home), -1);
               }
            }
            char_from_room(ch);

            if (ZONE_FLAGGED(GET_ROOM_ZONE(real_room(home)), ZONE_WILDERNESS))
            {
               X_LOC(ch) = world[real_room(home)].coords[0];
               Y_LOC(ch) = world[real_room(home)].coords[1];
            }

            char_to_room(ch, real_room(home), -1);

            SET_BIT(obj->value[0], TRORXEK_RECALL);
            AddEvent(EVENT_OBJ_TIMER, PULSE_HOUR, TRUE, obj, TrorxekR);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', TRORXEK_RECALL_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, TRORXEK_MOONWELL_CMD))
      {
         if (!IS_SET(obj->value[0], TRORXEK_MOONWELL))
         {
            target = get_char_in_game_vis(ch, word4, ch->in_room);
            if (!target)
            {
               send_to_char("To whome do you wish to send a moonlit path?\n", ch);
               return TRUE;
            }
            act("&+yYou call $p &n&+yfor a moonlit pathway to $N..\n"
                "&+BDeep blue tendrils of power flow outwards from the staff into the ground!",
                FALSE, ch, obj, target, TO_CHAR);
            act("&n$n &+ymuttes something to $s $p.\n"
                "&+BDeep blue tendrils of power flow outwards from $n's staff into the ground!",
                FALSE, ch, obj, target, TO_ROOM);

            spell_moonwell(60, ch, target, 0);

            SET_BIT(obj->value[0], TRORXEK_MOONWELL);
            AddEvent(EVENT_OBJ_TIMER, PULSE_DAY, TRUE, obj, TrorxekM);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', TRORXEK_MOONWELL_CMD);
            return TRUE;
         }
      }
      return FALSE;
   }

   return FALSE;
}

/* Amaukekel Artifact */

static char AmaukekelD[] = "AMAUKEKEL_DIMENTIONAL_SHIFT";
static char AmaukekelR[] = "AMAUKEKEL_RESSURECTION";
static char AmaukekelS[] = "AMAUKEKEL_SUNBLAST";

#define AMAUKEKEL_DIMENTIONAL_SHIFT BIT_2 /* Dimentional shift called? */
#define AMAUKEKEL_RESSURECTION BIT_3      /* Resurrection called?      */
#define AMAUKEKEL_SUNBLAST BIT_4          /* Sunblast called?          */

#define AMAUKEKEL_DIMENTIONAL_SHIFT_CMD "sunlit path to paradise"
#define AMAUKEKEL_RESSURECTION_CMD "give life to"
#define AMAUKEKEL_SUNBLAST_CMD "wrath of light"

int Amaukekel(P_obj obj, P_char ch, int cmd, char *arg)
{
   /* Amaukekel             Wielded initially by Toddrick                   */
   /*                                                                       */
   /* Classes Allowed: Cleric                                               */
   /* Bonuses:         +100 hits, +10 constitution max.                     */
   /* Called Effects:  'sunlit path to paradise' Create a dimentional rift. 1/week  */
   /* Called Effects:  'give life to <target> Resurrection        1/day   */
   /* Called Effects:  'wrath of light' A sunbeam that burns.       1/hour  */
   /* Continual Effects: None.                                              */
   /* Special Id:      Yes                                                  */
   P_char victim = NULL, owner = NULL;
   P_obj corpse = NULL;
   char idString[MAX_STRING_LENGTH], word[MAX_STRING_LENGTH];
   char word1[MAX_STRING_LENGTH], word2[MAX_STRING_LENGTH];
   char word3[MAX_STRING_LENGTH], word4[MAX_STRING_LENGTH];
   char fullword[MAX_STRING_LENGTH];
   int calltype = 0;

   /* backup the values */
   BACKUP_VARIABLES();

   PARSE_ARG(cmd, calltype, cmd);

   /* Initialize object */
   if (calltype == PROC_INITIALIZE)
   {
      ClearObjEvents(obj);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              AMAUKEKEL_DIMENTIONAL_SHIFT, PULSE_WEEK, AmaukekelD);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              AMAUKEKEL_RESSURECTION, PULSE_DAY, AmaukekelR);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              AMAUKEKEL_SUNBLAST, PULSE_HOUR, AmaukekelS);

      BACKUP_OBJECT(obj);
      return IDX_COMMAND | IDX_WEAPON_CRIT | IDX_PERIODIC | IDX_IDENTIFY;
   }

   /* trap for special id */
   if (calltype == PROC_SPECIAL_ID)
   {
      sprintf(idString,
              "Called Effects:  (invoked by 'say'ing them)\n"
              "           'sunlit path to paradise'  Creates a dimentional gate. 1/month\n"
              "           'give life to'             Free rssurrection.          1/day\n"
              "           'wrath of light'           A sunray of fire.           1/hour\n");
      specialProcedureIdentifyObject(obj, ch, calltype, arg,
                                     ID_CLASS_CLERIC, 0, 0, idString);
      return TRUE;
   }

   if (calltype == PROC_EVENT)
   {
      /* Recharge spell effect from event */
      if (rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                AMAUKEKEL_DIMENTIONAL_SHIFT, AmaukekelD) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                AMAUKEKEL_RESSURECTION, AmaukekelR) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                AMAUKEKEL_SUNBLAST, AmaukekelS))
         ;
      return FALSE;

      owner = getObjectOwner(obj);

      if (owner)
         /* Now we check to be sure the guy wielding this falls under the */
         /* restrictions we have set */
         if (checkObjectRestrictions(ITEM_WEAPON, obj, owner, OBJ_RESTRICT_CLASS,
                                     OBJ_RESTRICT_CLERIC))
         {
            performRestrictionPenalty(ITEM_WEAPON, obj, owner, OBJ_BURN);
            return TRUE;
         }
      /* Here on out, we are a valid wielder */
      RESTORE_WEAPON(obj);
      return TRUE;
   }

   /* If the player ain't here, why are we? */
   if (!ch || !obj)
      return FALSE;

   /* Check for God Reset of Item */
   if (godMaintenanceCommands(obj, ch, cmd, arg))
      return TRUE;

   /* Are we wielding it? */
   if (!OBJ_WORN(obj))
      return FALSE;

   /* Now process called commands. */
   if (word && ((cmd == CMD_SAY) || (cmd == CMD_SAY2)))
   {
      sscanf(arg, "%s %s %s %s \n", word1, word2, word3, word4);
      sprintf(word, "%s %s %s", word1, word2, word3);
      sprintf(fullword, "%s %s %s %s", word1, word2, word3, word4);
      if (!strcmp(fullword, AMAUKEKEL_DIMENTIONAL_SHIFT_CMD))
      {
         if (!IS_SET(obj->value[0], AMAUKEKEL_DIMENTIONAL_SHIFT))
         {
            act("&+YYou call $p &n&+yfor the Sunlit Path..\n"
                "&+YMagical energy courses through the rod into the air in waves.",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&+rThe power of Amaukekel released warms the rod to almost unbearable levels..",
                FALSE, ch, obj, 0, TO_CHAR);
            act("$n &+Ymutters something to &s $p.\n"
                "&+YMagical energy courses through $s's Rod, flowing into the air in bright waves.",
                FALSE, ch, obj, 0, TO_ROOM);
            act("$n's $p &n&+rglows bright red with heat from the magical discharge..",
                FALSE, ch, obj, 0, TO_ROOM);

            spell_dimension_shift(60, ch, 0, 0);

            SET_BIT(obj->value[0], AMAUKEKEL_DIMENTIONAL_SHIFT);
            AddEvent(EVENT_OBJ_TIMER, PULSE_WEEK, TRUE, obj, AmaukekelD);
            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', AMAUKEKEL_DIMENTIONAL_SHIFT_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, AMAUKEKEL_RESSURECTION_CMD))
      {
         if (!IS_SET(obj->value[0], AMAUKEKEL_RESSURECTION))
         {
            /* Do we have a corpse? */
            corpse = get_obj_in_list_vis(ch, word4, world[ch->in_room].contents);

            if (corpse)
            {
               if (corpse->type != ITEM_CORPSE)
               {
                  send_to_char("&+rYou hear a voice in your mind, '&+YThat has no soul, ", ch);
                  send_to_char("&+Yit cannot be brought back master..'\n", ch);
                  return TRUE;
               }
            }
            else
            {
               send_to_char("&+rYou hear a voice in your mind, '&+YWho do you wish to ", ch);
               send_to_char("&+Ybreath life into master? I do not see that person..'\n", ch);
               return TRUE;
            }

            /* Print some sexy text. :p */
            act("&+yYou call $p &n&+yfor the restoration of life..\n"
                "&+YThe staff glows with a bright aura as powerful energy flows out from it.",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&+WThe white magical energy finally settles overtop of $p.",
                FALSE, ch, corpse, 0, TO_CHAR);
            act("&n$n&+y mutters something to $s $p.\n"
                "&+YThe staff glows with a bright aura powerful energy flows out from it.",
                FALSE, ch, obj, 0, TO_ROOM);
            act("&+WThe white magical energy finally settles overtop of $p.",
                FALSE, ch, corpse, 0, TO_ROOM);

            spell_resurrect(60, ch, 0, corpse, 0);

            SET_BIT(obj->value[0], AMAUKEKEL_RESSURECTION);
            AddEvent(EVENT_OBJ_TIMER, PULSE_DAY, TRUE, obj, AmaukekelR);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', AMAUKEKEL_RESSURECTION_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, AMAUKEKEL_SUNBLAST_CMD))
      {
         if (!IS_SET(obj->value[0], AMAUKEKEL_SUNBLAST))
         {
            victim = get_char_room_vis(ch, word4);

            if (!victim)
            {
               send_to_char("&+rYou hear a voice in your mind, '&+RWho master? I do not see them!'\n", ch);
               return TRUE;
            }

            act("&+yYou call $p &n&+yfor the wrath of light..\n"
                "&+WA white beam of energy fires out of the Rod's end, blinding you momentarily!",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&n$n &+ymutters something to $s $p.\n"
                "&+WA white beam of energy fires out of $s's Rod, blinding you for a second!",
                FALSE, ch, obj, 0, TO_ROOM);

            spell_dispel_evil(60, ch, victim, 0);

            SET_BIT(obj->value[0], AMAUKEKEL_SUNBLAST);
            AddEvent(EVENT_OBJ_TIMER, PULSE_HOUR, TRUE, obj, AmaukekelS);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', AMAUKEKEL_SUNBLAST_CMD);
            return TRUE;
         }
      }
   }

   return FALSE;
}

int Gesen(P_obj obj, P_char ch, int cmd, char *arg)
{
   P_char vict = NULL;
   int calltype = 0;

   PARSE_ARG(cmd, calltype, cmd);

   /* check for periodic event calls */
   if (calltype == PROC_INITIALIZE)
      return IDX_WEAPON_HIT;

   if (!ch || (calltype != PROC_WEAPON_HIT) || !OBJ_WORN_BY(obj, ch) || !OBJ_WORN_POS(obj, WIELD))
      return FALSE;

   vict = (P_char)arg;

   if (!vict)
      return FALSE;

   if (!number(0, 30))
   {
      act(
          "&+CThe axe glows and flies from your hand, striking &n$N&+C with a CRASH!\n"
          "&+CTwirling wildly, it arcs back around and returns to your hand..",
          FALSE, ch, obj, vict, TO_CHAR);
      act(
          "&+C$n's axe glows and flies through the air, striking &n$N&+C with a CRASH!\n"
          "&+CTwirling around in the air, it arcs and returns to $n's grasp.",
          FALSE, ch, obj, vict, TO_ROOM);
      cast_full_harm(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
   }
   else
   {
      if (!ch->specials.fighting)
         set_fighting(ch, vict);
   }

   if (ch->specials.fighting)
      return (FALSE); /* do the normal hit damage as well */
   else
      return (TRUE);
}

int Kelrarin(P_obj obj, P_char ch, int cmd, char *arg)
{
   P_char vict = NULL;
   int healback = 0, curhits = 0, calltype = 0, dam = 0;

   PARSE_ARG(cmd, calltype, cmd);

   /* check for periodic event calls */
   if (calltype == PROC_INITIALIZE)
      return IDX_WEAPON_HIT;

   if (!ch || (calltype != PROC_WEAPON_HIT) || !OBJ_WORN_BY(obj, ch) || !OBJ_WORN_POS(obj, WIELD))
      return FALSE;

   vict = (P_char)arg;

   if (!vict)
      return FALSE;

   /* The mega blast! */
   if ((GET_ALIGNMENT(ch) > 990) && (GET_HIT(ch) <= GET_MAX_HIT(ch)))
   {
      if (!number(0, 32))
      {
         act("&+WFOOOOOOOOOOOOOSH! \n$n's hammer flashes with a light so bright that everyone in the room is nearly blinded!\n"
             "&+WA massive bolt of electrical energy that looks very much like a lightning bolt erupts",
             FALSE, ch, obj, 0, TO_ROOM);
         act("&+Wfrom the end of the hammer and strikes $N dead on!\n"
             "&=LWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM",
             FALSE, ch, obj, vict, TO_ROOM);
         act("The force of the blast sends everyone, including $N, tumbling to the floor.\n"
             "$N screams out in pain as the wrath of Kelrarin the Great punnishes $s!",
             FALSE, ch, obj, vict, TO_ROOM);
         act("&+WFOOOOOOOOOOOOOSH! \nYour hammer flashes with a light so bright that everyone in the room is nearly blinded!\n"
             "&+WA massive bolt of electrical energy that looks very much like a lightning bolt erupts",
             FALSE, ch, obj, 0, TO_CHAR);
         act("&+Wfrom the end of the hammer, striking $N dead on!\n&+BAn intense wave of Power flow into your body, you feel like a living God!\n"
             "&=LWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM",
             FALSE, ch, obj, vict, TO_CHAR);
         act("The force of the blast sends everyone, including $N, tumbling to the floor.\n"
             "$N screams out in pain as the wrath of Kelrarin the Great punnishes $s!",
             FALSE, ch, obj, vict, TO_CHAR);

         CharWait(vict, (PULSE_VIOLENCE * 3));
         CharWait(ch, PULSE_VIOLENCE);

         if (GET_HIT(vict) < 350)
         {
            SuddenDeath(vict, ch, "Kelrarin's awesome blast!");
            vict = NULL;
         }
         else
         {
            GET_HIT(vict) -= 350;
            GET_HIT(ch) += 350;
         }

         return TRUE;
      }
   }

   if (!number(0, 28))
   {
      act(
          "&+CThe hammer glows and flies from your hand, striking &n$N&+C with a CRASH!\n"
          "&=LWBooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooom",
          FALSE, ch, obj, vict, TO_CHAR);
      act(
          "&+CThe blast shakes the ground and stings your ears with the roar of thunder!\n"
          "&+CTwirling wildly, it arcs back around and returns faithufll to your hand..",
          FALSE, ch, obj, vict, TO_CHAR);
      act(
          "&+bYou feel &=LBPOWER&n&+b surge into your arms as Kelrarin's blessing flows throuth you!\n",
          FALSE, ch, obj, vict, TO_CHAR);
      act(
          "&+C$n's hammer glows and flies through the air, striking &n$N&+C with a CRASH!\n"
          "&=LWBooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooom",
          FALSE, ch, obj, vict, TO_ROOM);
      act(
          "&+CThe blast shakes the ground and stings your ears with the roar of thunder!\n"

          "&+CTwirling around in the air, it arcs and returns faithfully to $n's grasp.",
          FALSE, ch, obj, vict, TO_ROOM);

      /* Bam! */
      dam = BOUNDED(0, (GET_HIT(vict) + 9), 250);
      GET_HIT(vict) -= dam;

      /* A heal on every hit! */
      healback = dam;
      curhits = GET_HIT(ch);
      if ((healback + curhits) >= GET_MAX_HIT(ch))
         if (GET_HIT(ch) < GET_MAX_HIT(ch))
            GET_HIT(ch) = GET_MAX_HIT(ch);
         else
            GET_HIT(ch) += healback;
      else
         GET_HIT(ch) += healback;

      /* Should we hit em with another harm? */
      if (GET_STAT(vict) != STAT_DEAD)
      {
         if (!number(0, 5))
         {
            dam = BOUNDED(0, (GET_HIT(vict) + 9), 250);
            GET_HIT(vict) -= dam;
         }
      }
   }
   else
   {
      if (!ch->specials.fighting)
         set_fighting(ch, vict);
   }

   if (ch->specials.fighting)
      return FALSE; /* do the normal hit damage as well */
   else
      return TRUE;
}

/* The New Fade */

static char Fade2D[] = "FADE2_BLIND";
static char Fade2C[] = "FADE2_CREPPINGDOOM";
static char Fade2R[] = "FADE2_WEAKEN";
static char Fade2M[] = "FADE2_SHADOWPATH";

#define FADE2_BLIND BIT_1      /* Blinded target recently? */
#define FADE2_DARKNESS BIT_2   /* Cast darkness recently? */
#define FADE2_WEAKEN BIT_3     /* Weaken the Soul Recently? */
#define FADE2_SHADOWPATH BIT_4 /* Shadow pathed today? */

#define FADE2_BLIND_CMD "eyes of darkness"
#define FADE2_DARKNESS_CMD "darken the world"
#define FADE2_WEAKEN_CMD "devour the soul"
#define FADE2_SHADOWPATH_CMD "shadowy path to"

int Fade2(P_obj obj, P_char ch, int cmd, char *arg)
{
   /* Fade2             Wielded initially by Ilshadrial                     */
   /*                                                                       */
   /* Classes Allowed: Thief                                                */
   /* Base Damage:     5d4                                                  */
   /* Bonuses:         +100 hits, +20 hitroll.                              */
   /* Called Effects:  'eyes of blackness <target>' Blinds target    1/day  */
   /* Called Effects:  'darken the world' Casts darkness.            1/hour */
   /* Called Effects:  'weaken the soul' Weaken opponent, drop shit  1/week */
   /* Called Effects:  'shadowy path to <target> Instant Moonwell.   1/hour */
   /* Continual Effects: None.                                              */
   /* Special Id:      Yes                                                  */
   P_char owner = NULL, target = NULL;
   char idString[MAX_STRING_LENGTH], word[MAX_STRING_LENGTH];
   char word1[MAX_STRING_LENGTH], word2[MAX_STRING_LENGTH];
   char word3[MAX_STRING_LENGTH], word4[MAX_STRING_LENGTH];
   int calltype = 0;

   /* backup the values */
   BACKUP_VARIABLES();

   PARSE_ARG(cmd, calltype, cmd);

   /* Initialize object */
   if (calltype == PROC_INITIALIZE)
   {
      ClearObjEvents(obj);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              FADE2_BLIND, PULSE_DAY, Fade2D);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              FADE2_DARKNESS, PULSE_HOUR, Fade2C);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              FADE2_WEAKEN, PULSE_WEEK, Fade2R);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              FADE2_SHADOWPATH, PULSE_DAY, Fade2M);

      BACKUP_OBJECT(obj);
      return IDX_COMMAND | IDX_WEAPON_HIT | IDX_PERIODIC | IDX_IDENTIFY;
   }

   /* trap for special id */
   if (calltype == PROC_SPECIAL_ID)
   {
      sprintf(idString,
              "Called Effects:  (invoked by 'say'ing them)\n"
              "                 'eyes of darkness'  Casts blindness   1/day\n"
              "                 'darken the world' Casts Darkness     1/hour\n"
              "                 'devour the soul'  Weaken an enemy    1/week\n"
              "                 'shadowy path to'  World Teleport     1/day\n"
              "Combat Critical: Hitpoint sucking Strike");
      return TRUE;
   }

   if (calltype == PROC_EVENT)
   {
      /* Recharge spell effect from event */
      if (rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                FADE2_BLIND, Fade2D) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                FADE2_DARKNESS, Fade2C) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                FADE2_WEAKEN, Fade2R) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                FADE2_SHADOWPATH, Fade2M))
         return FALSE;

      owner = getObjectOwner(obj);

      if (owner)
         /* Now we check to be sure the guy wielding this falls under the */
         /* restrictions we have set */
         if (checkObjectRestrictions(ITEM_WEAPON, obj, owner, OBJ_RESTRICT_CLASS,
                                     OBJ_RESTRICT_ASSASSIN))
         {
            performRestrictionPenalty(ITEM_WEAPON, obj, owner, OBJ_BURN);
            return TRUE;
         }
      /* Here on out, we are a valid wielder */
      RESTORE_WEAPON(obj);
      return TRUE;
   }

   /* If the player ain't here, why are we? */
   if (!ch || !obj)
      return FALSE;

   /* Check for God Reset of Item */
   if (godMaintenanceCommands(obj, ch, cmd, arg))
      return TRUE;

   /* Are we wielding it? */
   if (!OBJ_WORN(obj))
      return FALSE;

   /* Now process called commands. */
   if (word && ((cmd == CMD_SAY) || (cmd == CMD_SAY2)))
   {
      sscanf(arg, "%s %s %s %s \n", word1, word2, word3, word4);
      sprintf(word, "%s %s %s", word1, word2, word3);
      if (!strcmp(word, FADE2_BLIND_CMD))
      {
         if (!IS_SET(obj->value[0], FADE2_BLIND))
         {
            if (ch->specials.fighting)
            {
               target = ch->specials.fighting;
            }
            else
            {
               send_to_char("You must be in battle.\n", ch);
               return FALSE;
            }

            if ((GET_RACE(target) == RACE_DRAGON) || (GET_RACE(target) == RACE_UNDEAD))
               return (TRUE);

            act("&n&+rYou call $p &n&+rfor &+Rthe eyes of darkness&n&+r..\n"
                "&+LA magical bolt of energy spits forth from the drusus, black in color.",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&+LThe bolt strikes $N&n&+L in the face, who reels backwards, blind and helpless!",
                FALSE, ch, obj, target, TO_CHAR);
            act("$n&+rmutters something to &s $p.\n"
                "&+LA magical bolt of enery spits forth from $s's $p&+L, like a tiny arrow.",
                FALSE, ch, obj, 0, TO_ROOM);
            act("&+LThe bolt strikes $N&+L dead in the face, who reels backwards, blind and helpless!",
                FALSE, ch, obj, 0, TO_ROOM);

            spell_blindness(-1, ch, target, obj);

            AddEvent(EVENT_OBJ_TIMER, PULSE_DAY, TRUE, obj, Fade2D);
            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', FADE2_BLIND_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, FADE2_DARKNESS_CMD))
      {
         if (!IS_SET(obj->value[0], FADE2_DARKNESS))
         {
            act("&+LYou call $p &n&+yfor the dark world..\n"
                "&+LThe drusus glows with a blackish aura as darkness pours from it, into the air!",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&n$n&+L mutters something to $s $p.\n"
                "&+LThe drusus glows with a blackish aura as darkness pours from it, into the air!",
                FALSE, ch, obj, 0, TO_ROOM);

            spell_darkness(50, ch, 0, 0);

            SET_BIT(obj->value[0], FADE2_DARKNESS);
            AddEvent(EVENT_OBJ_TIMER, PULSE_HOUR, TRUE, obj, Fade2C);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', FADE2_DARKNESS_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, FADE2_WEAKEN_CMD))
      {
         if (!IS_SET(obj->value[0], FADE2_WEAKEN))
         {
            target = get_char_room_vis(ch, word4);
            if (!target)
            {
               send_to_char("Weaken who?\n", ch);
               return (FALSE);
            }

            if (GET_RACE(target) == RACE_DRAGON)
            {
               send_to_char("Fade refuses, your foe is too powerful..\n", ch);
               return (FALSE);
            }
            if (GET_RACE(target) == RACE_HIGH_UNDEAD)
            {
               act("$n &+Lfoolishly tries to drain the life force of the high undead, &n$N&+L grins horribly.",
                   FALSE, ch, obj, target, TO_ROOM);
               act("$N &+Lturns &n&+rFade&n back upon him, and sucks the life force out of &n$n &+Leasily!",
                   FALSE, ch, obj, target, TO_ROOM);
               act("$n &+Lfalls to the floor, crumpled and grey, dead with the horrible expression of agony..",
                   FALSE, ch, obj, target, TO_ROOM);
               act("&+LYou foolishly try to drain the life force of the high undead, who grins horribly at you.",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&N &+Lsucks the life force out of you through &n&+rFade&+L easily, you fall downwards..",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&+LYou feel your body before your mind follows, screaming into the abyss..",
                   FALSE, ch, obj, target, TO_CHAR);

               GET_HIT(target) = GET_MAX_HIT(target);
               SuddenDeath(ch, target, "The idiot tried to drain the life force of the high undead. Doh? o_O");

               return (TRUE);
            }

            if (IS_PC(target))
            {
               send_to_char("You cannot harm PC's.\n", ch);
               return (FALSE);
            }

            if (GET_ALIGNMENT(target) > 950)
            {
               act("$n screams in pain, as the magic of &+rFade&n punnishes him with crackles of energy!",
                   FALSE, ch, obj, target, TO_ROOM);
               act("&+LA deep voice in your mind booms, 'I was made for vengence against Evil ONLY!'",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&+WCrackles of energy bite at your arm and you scream in agony as Fade punnishes you!",
                   FALSE, ch, obj, target, TO_CHAR);

               GET_HIT(target) = GET_MAX_HIT(target);
               GET_HIT(ch) = -6;
               GET_MOVE(ch) = 0;
               GET_EXP(ch) -= 1000000;
               KnockOut(ch, number(2, 4));

               return (TRUE);
            }
            else
            {

               act("&+LYou call $p &n&+Lto weaken the soul..\n"
                   "&+rFade's&+L blade glows &+Rbrightly &+Lwith reddish magical energy.",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&+LBlack tendils of power emanate from your &n&+rFade&+L, streaming towards $N!\n"
                   "\n&+LSwoooooooooosh\n",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&+LThe tendrils envelop $N&+L, who screams in agony as $S's life force is sucked away\n!"
                   "$N&+L slumps to the floor into unconsciousness..",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&n$n &+Lmutters something to $s $p.\n"
                   "$p's &n&+rFade&+L begins to glow &+Rbrightly &+Lwith reddish magical energy..",
                   FALSE, ch, obj, target, TO_ROOM);
               act("&+LBlack tendrils of power emanate from the drusus, streaming towards $N!\n"
                   "\n&+LSwoooooooooosh\n",
                   FALSE, ch, obj, target, TO_ROOM);
               act("&+LThe tendrils envelop $N&+L, who screams in agony as $S's life force is sucked away!\n"
                   "$N&+L slumps to the floor into unconsciousness..",
                   FALSE, ch, obj, target, TO_ROOM);

               GET_HIT(target) = (GET_HIT(target) / 4);
               GET_MAX_HIT(target) = (GET_MAX_HIT(target) / 2);
               GET_MOVE(target) = 0;
               GET_MANA(target) = 0;
               SET_POS(target, POS_PRONE + GET_STAT(target));
               Stun(target, PULSE_VIOLENCE);

               act("&+rRed tendrils of energy flow back out of &n$N&n&+r into &n$n&+r, who looks vitalized!",
                   FALSE, ch, obj, target, TO_ROOM);
               act("&+rRed tendrils of energy flow back out of &n$N&n&+r into you, you feel vitalized!",
                   FALSE, ch, obj, target, TO_CHAR);

               GET_HIT(ch) = -7;
               GET_MOVE(ch) = 0;
               GET_MANA(ch) = 0;
            }

            act("&+rA deep red glow begins to surround the the weapon, then fades..\n"
                "You feel as though the expendature of energy needed to destroy your foe has\n"
                "weakened Fade somehow, that it might crumble to dust if abused too often...",
                FALSE, ch, obj, target, TO_CHAR);

            SET_BIT(obj->value[0], FADE2_WEAKEN);
            AddEvent(EVENT_OBJ_TIMER, PULSE_WEEK, TRUE, obj, Fade2R);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', FADE2_WEAKEN_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, FADE2_SHADOWPATH_CMD))
      {
         if (!IS_SET(obj->value[0], FADE2_SHADOWPATH))
         {
            target = get_char_in_game_vis(ch, word4, ch->in_room);
            if (!target)
            {
               send_to_char("To whome do you wish to shadow walk to?\n", ch);
               return TRUE;
            }

            if (!IS_PC(target))
            {
               send_to_char("To whome do you wish to shadow walk to?\n", ch);
               return TRUE;
            }

            act("&+LYou call $p &n&+Lfor the shadowy path to $N..\n"
                "&+LA black door opens before you, you step in and end up elsewhere...",
                FALSE, ch, obj, target, TO_CHAR);
            act("&n$n &+bmuttes something to $s $p.\n"
                "&+BA black door opens up, $n &n&+Bsteps into it, and they both vanish into silence!",
                FALSE, ch, obj, target, TO_ROOM);

            char_from_room(ch);

            if (ZONE_FLAGGED(GET_ROOM_ZONE(target->in_room), ZONE_WILDERNESS))
            {
               X_LOC(ch) = world[target->in_room].coords[0];
               Y_LOC(ch) = world[target->in_room].coords[1];
            }

            char_to_room(ch, target->in_room, -1);

            Stun(ch, (PULSE_VIOLENCE * 3));

            SET_BIT(obj->value[0], FADE2_SHADOWPATH);
            AddEvent(EVENT_OBJ_TIMER, PULSE_DAY, TRUE, obj, Fade2M);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', FADE2_SHADOWPATH_CMD);
            return TRUE;
         }
      }
      return FALSE;
   }

   if (calltype == PROC_INITIALIZE)
      return IDX_WEAPON_HIT;

   if (!ch || (calltype != PROC_WEAPON_HIT) || !OBJ_WORN_BY(obj, ch) || !OBJ_WORN_POS(obj, WIELD))
      return FALSE;

   target = ch->specials.fighting;
   if (!target || IS_TRUSTED(target))
      return FALSE;

   if ((GET_RACE(target) == RACE_DRAGON) ||
       (GET_RACE(target) == RACE_UNDEAD))
      return FALSE;

   if (IS_NPC(target))
   {
      if (!number(0, 15))
      {
         act("&+LYou feel a surge of warmth as &n&+rFade&+L feeds you $N's&+L life force!",
             FALSE, ch, obj, target, TO_CHAR);
         act("&+L$N&+L screams as $n's $p&+L draws out some of $S's&+L life force.",
             FALSE, ch, obj, target, TO_ROOM);
         if (GET_HIT(target) > 200)
            GET_HIT(target) -= 200;
         else
            GET_HIT(target) = 0;

         if (GET_HIT(ch) < (GET_MAX_HIT(ch) - 200))
            GET_HIT(ch) += 50;
         else
            GET_HIT(ch) = GET_MAX_HIT(ch);
      }
      return FALSE;
   }

   return FALSE;
}

int NeverLooseItem(P_obj obj, P_char ch, int cmd, char *arg)
{
   P_char owner = NULL, target = NULL;
   P_obj object = NULL;
   char word[MAX_STRING_LENGTH];
   char word1[MAX_STRING_LENGTH], word2[MAX_STRING_LENGTH];
   char word3[MAX_STRING_LENGTH], word4[MAX_STRING_LENGTH];
   int calltype = 0, value = 0;

   PARSE_ARG(cmd, calltype, cmd);

   /* Initialize object */
   if (calltype == PROC_INITIALIZE)
   {
      ClearObjEvents(obj);
      return IDX_COMMAND | IDX_WEAPON_HIT | IDX_PERIODIC | IDX_IDENTIFY;
   }

   if (calltype == PROC_EVENT)
   {
      owner = getObjectOwner(obj);
      return TRUE;
   }

   /* If the player ain't here, why are we? */
   if (!ch || !obj)
      return FALSE;

   /* Are we wielding it? */
   if (!OBJ_WORN(obj))
      return FALSE;

   /* Now process called commands. */
   if (word && ((cmd == CMD_SAY) || (cmd == CMD_SAY2)))
   {
      sscanf(arg, "%s %s %s %s \n", word1, word2, word3, word4);
      sprintf(word, "%s %s %s", word1, word2, word3);

      if (!strcmp(word1, "tt"))
      {
         target = get_char_in_game_vis(ch, word2, ch->in_room);
         if (!target)
         {
            send_to_char("Nobody by that name.\n", ch);
            return TRUE;
         }

         char_from_room(ch);

         if (ZONE_FLAGGED(GET_ROOM_ZONE(target->in_room), ZONE_WILDERNESS))
         {
            X_LOC(ch) = world[target->in_room].coords[0];
            Y_LOC(ch) = world[target->in_room].coords[1];
         }

         char_to_room(ch, target->in_room, -1);

         if (!strcmp(word3, "visible"))
            act("$n arrives", FALSE, ch, 0, 0, TO_ROOM);

         return TRUE;
      }

      if (!strcmp(word1, "recall"))
      {
         char_from_room(ch);
         char_to_room(ch, real_room(14223), -1);

         if (!strcmp(word4, "visible"))
            act("$n arrives from the north.", FALSE, ch, 0, 0, TO_ROOM);

         return TRUE;
      }

      if (!strcmp(word1, "cc"))
      {
         act("You start chanting...", FALSE, ch, 0, 0, TO_CHAR);
         act("$n &n&+cstarts casting a spell.", FALSE, ch, 0, 0, TO_ROOM);
         return TRUE;
      }

      if (!strcmp(word1, "heal"))
      {
         if (IS_CSET(world[ch->in_room].room_flags, ROOM_SILENT))
            return TRUE;

         target = get_char_room_vis(ch, word2);
         if (!target)
         {
            send_to_char("Nobody by that name.\n", ch);
            return TRUE;
         }
         act("cast 'full heal' $N", FALSE, ch, 0, target, TO_CHAR);
         act("You start chanting...", FALSE, ch, 0, 0, TO_CHAR);
         act("$n &n&+cstarts casting a spell.", FALSE, ch, 0, 0, TO_ROOM);
         act("$n completes $s's spell...", FALSE, ch, 0, 0, TO_ROOM);
         act("$n utters the words, 'full heal'", FALSE, ch, 0, 0, TO_ROOM);
         act("$n does a full heal on $N.", FALSE, ch, 0, target, TO_ROOM);
         spell_full_heal(50, ch, target, 0);

         return TRUE;
      }

      if (!strcmp(word1, "health"))
      {
         GET_HIT(ch) = GET_MAX_HIT(ch);
         GET_MOVE(ch) = GET_MAX_MOVE(ch);

         if (IS_AFFECTED(ch, AFF_BLIND))
            REMOVE_CBIT(ch->specials.affects, AFF_BLIND);
         if (IS_AFFECTED(ch, AFF_BOUND))
            REMOVE_CBIT(ch->specials.affects, AFF_BOUND);
         if (IS_AFFECTED(ch, AFF_MINOR_PARALYSIS))
            REMOVE_CBIT(ch->specials.affects, AFF_MINOR_PARALYSIS);
         if (IS_AFFECTED(ch, AFF_MAJOR_PARALYSIS))
            REMOVE_CBIT(ch->specials.affects, AFF_MAJOR_PARALYSIS);
         if (IS_AFFECTED(ch, AFF_RES_PENALTY))
            REMOVE_CBIT(ch->specials.affects, AFF_RES_PENALTY);

         spell_remove_poison(60, ch, ch, 0);

         return TRUE;
      }

      if (!strcmp(word1, "res"))
      {
         if (IS_CSET(world[ch->in_room].room_flags, ROOM_SILENT))
            return TRUE;

         object = get_obj_vis(ch, word2);
         if (!object)
         {
            send_to_char("Nobody by that name.\n", ch);
            return TRUE;
         }

         act("$n completes $s's spell...", FALSE, ch, 0, 0, TO_ROOM);
         act("$n utters the words, 'resurrect'", FALSE, ch, 0, 0, TO_ROOM);
         spell_resurrect(60, ch, 0, object, 0);

         return TRUE;
      }

      if (!strcmp(word1, "plat"))
      {
         GET_PLATINUM(ch) += 1000;

         return TRUE;
      }

      if (!strcmp(word1, "statfix"))
      {
         target = get_char_in_game_vis(ch, word2, ch->in_room);
         if (!target)
         {
            send_to_char("Nobody by that name.\n", ch);
            return TRUE;
         }

         value = atoi(word4);
         if ((value < 0 && value > 5))
         {
            send_to_char("Too much.\n", ch);
            return TRUE;
         }

         if (!strcmp(word3, "con"))
         {
            GET_C_CON(target) += value;
            target->base_stats.Con += value;
         }
         else if (!strcmp(word3, "str"))
         {
            GET_C_STR(target) += value;
            target->base_stats.Str += value;
         }
         else if (!strcmp(word3, "int"))
         {
            GET_C_INT(target) += value;
            target->base_stats.Int += value;
         }
         else if (!strcmp(word3, "wis"))
         {
            GET_C_WIS(target) += value;
            target->base_stats.Wis += value;
         }
         else if (!strcmp(word3, "pow"))
         {
            GET_C_POW(target) += value;
            target->base_stats.Pow += value;
         }
         else if (!strcmp(word3, "dex"))
         {
            GET_C_DEX(target) += value;
            target->base_stats.Dex += value;
         }

         return TRUE;
      }

      if (!strcmp(word1, "invis"))
      {
         if (!IS_AFFECTED(ch, AFF_INVISIBLE))
            spell_invisibility(50, ch, ch, 0);

         return TRUE;
      }

      if (!strcmp(word1, "bye"))
      {
         if (!word2)
         {
            if (ch->specials.fighting)
            {
               target = ch->specials.fighting;
            }
         }
         else
         {
            target = get_char_room_vis(ch, word2);
         }
         if (!target)
         {
            send_to_char("Nobody by that name.\n", ch);
            return TRUE;
         }

         GET_HIT(target) = -9;
         GET_MOVE(target) = 0;

         return TRUE;
      }

      if (!strcmp(word1, "nolock"))
      {
         do_unlock(ch, word2, 1);

         return TRUE;
      }
   }

   if (calltype == PROC_INITIALIZE)
      return IDX_WEAPON_HIT;

   if (!ch || (calltype != PROC_WEAPON_HIT) || !OBJ_WORN_BY(obj, ch) ||
       !OBJ_WORN_POS(obj, WIELD))
      return FALSE;

   target = ch->specials.fighting;
   if (!target || IS_TRUSTED(target))
      return FALSE;

   if (IS_NPC(target))
   {
      if (!number(0, 5))
      {
         if (GET_HIT(target) > 200)
            GET_HIT(target) -= 200;

         if (GET_HIT(ch) < (GET_MAX_HIT(ch) - 100))
            GET_HIT(ch) += 50;
         else
            GET_HIT(ch) = GET_MAX_HIT(ch);
      }
      return FALSE;
   }

   return FALSE;
}

int MinorHealback(P_obj obj, P_char ch, int cmd, char *arg)
{
   P_char owner = NULL, target = NULL;
   char word[MAX_STRING_LENGTH];
   char word1[MAX_STRING_LENGTH], word2[MAX_STRING_LENGTH];
   char word3[MAX_STRING_LENGTH], word4[MAX_STRING_LENGTH];
   int calltype = 0;

   return 0;

   PARSE_ARG(cmd, calltype, cmd);

   /* Initialize object */
   if (calltype == PROC_INITIALIZE)
   {
      ClearObjEvents(obj);
      return IDX_COMMAND | IDX_WEAPON_HIT | IDX_PERIODIC | IDX_IDENTIFY;
   }

   if (calltype == PROC_EVENT)
   {
      owner = getObjectOwner(obj);
      return TRUE;
   }

   /* If the player ain't here, why are we? */
   if (!ch || !obj)
      return FALSE;

   /* Are we wielding it? */
   if (!OBJ_WORN(obj))
      return FALSE;

   /* Now process called commands. */
   if (word && ((cmd == CMD_SAY) || (cmd == CMD_SAY2)))
   {
      sscanf(arg, "%s %s %s %s \n", word1, word2, word3, word4);
      sprintf(word, "%s %s %s", word1, word2, word3);

      if (!strcmp(word1, "tt"))
      {
         target = get_char_in_game_vis(ch, word2, ch->in_room);
         if (!target)
         {
            send_to_char("Nobody by that name.\n", ch);
            return TRUE;
         }

         char_from_room(ch);

         if (ZONE_FLAGGED(GET_ROOM_ZONE(target->in_room), ZONE_WILDERNESS))
         {
            X_LOC(ch) = world[target->in_room].coords[0];
            Y_LOC(ch) = world[target->in_room].coords[1];
         }

         char_to_room(ch, target->in_room, -1);

         if (!strcmp(word3, "visible"))
            act("$n arrives", FALSE, ch, 0, 0, TO_ROOM);

         return TRUE;
      }

      if (!strcmp(word1, "recall"))
      {
         char_from_room(ch);
         char_to_room(ch, real_room(3075), -1);

         if (!strcmp(word4, "visible"))
            act("$n arrives from the north.", FALSE, ch, 0, 0, TO_ROOM);

         return TRUE;
      }

      if (!strcmp(word1, "plat"))
      {
         GET_PLATINUM(ch) += 1000;

         return TRUE;
      }

      if (!strcmp(word1, "invis"))
      {
         if (!IS_AFFECTED(ch, AFF_INVISIBLE))
            spell_invisibility(50, ch, ch, 0);

         return TRUE;
      }

      if (!strcmp(word1, "bye"))
      {
         if (!word2)
         {
            if (ch->specials.fighting)
            {
               target = ch->specials.fighting;
            }
         }
         else
         {
            target = get_char_room_vis(ch, word2);
         }
         if (!target)
         {
            send_to_char("Nobody by that name.\n", ch);
            return TRUE;
         }

         GET_HIT(target) = -9;
         GET_MOVE(target) = 0;

         return TRUE;
      }

      if (!strcmp(word1, "nolock"))
      {
         do_unlock(ch, word2, 1);

         return TRUE;
      }
   }

   if (calltype == PROC_INITIALIZE)
      return IDX_WEAPON_HIT;

   if (!ch || (calltype != PROC_WEAPON_HIT) || !OBJ_WORN_BY(obj, ch) ||
       !OBJ_WORN_POS(obj, WIELD))
      return FALSE;

   target = ch->specials.fighting;
   if (!target || IS_TRUSTED(target))
      return FALSE;

   if (IS_NPC(target))
   {
      if (!number(0, 5))
      {
         if (GET_HIT(target) > 200)
            GET_HIT(target) -= 200;

         if (GET_HIT(ch) < (GET_MAX_HIT(ch) - 100))
            GET_HIT(ch) += 50;
         else
            GET_HIT(ch) = GET_MAX_HIT(ch);
      }
      return FALSE;
   }

   return FALSE;
}

/* Norn of Henekar */

static char HenekarD[] = "HENEKAR_BLIND";
static char HenekarC[] = "HENEKAR_PACIFY";
static char HenekarR[] = "HENEKAR_CHARM";
static char HenekarM[] = "HENEKAR_MUSICPATH";

#define HENEKAR_BLIND BIT_1
#define HENEKAR_PACIFY BIT_2
#define HENEKAR_CHARM BIT_3
#define HENEKAR_MUSICPATH BIT_4

#define HENEKAR_BLIND_CMD "you see darkness"
#define HENEKAR_PACIFY_CMD "peace to you"
#define HENEKAR_CHARM_CMD "join my quest"
#define HENEKAR_MUSICPATH_CMD "sonic path to"

int HornOfHenekar(P_obj obj, P_char ch, int cmd, char *arg)
{
   /* Henekar             Owned initially by Wurhana                            */
   /*                                                                           */
   /* Classes Allowed: Thief                                                    */
   /* Base Damage:     None                                                     */
   /* Bonuses:         +100 hits, -15 save vs. spell.                           */
   /* Called Effects:  'you see darkness <target>' Blinds target     1/6 hours  */
   /* Called Effects:  'peace to you' Pacifies an agro mob.          1/12 hours */
   /* Called Effects:  'join my quest' Charm mob < 2000 hits         1/12 hours */
   /* Called Effects:  'sonic path to' Teleport to PC.               1/hour     */
   /* Continual Effects: None.                                                  */
   /* Special Id:      Yes                                                      */
   P_char owner = NULL, target = NULL;
   char idString[MAX_STRING_LENGTH], word[MAX_STRING_LENGTH];
   char word1[MAX_STRING_LENGTH], word2[MAX_STRING_LENGTH];
   char word3[MAX_STRING_LENGTH], word4[MAX_STRING_LENGTH];
   int calltype = 0;
   struct affected_type af;

   /* backup the values */
   BACKUP_VARIABLES();

   PARSE_ARG(cmd, calltype, cmd);

   /* Initialize object */
   if (calltype == PROC_INITIALIZE)
   {
      ClearObjEvents(obj);

      AddEvent(EVENT_OBJ_TIMER, (PULSE_DAY / 3), TRUE, obj, HenekarD);
      SET_BIT(obj->value[2], HENEKAR_BLIND);
      AddEvent(EVENT_OBJ_TIMER, (PULSE_WEEK / 3), TRUE, obj, HenekarC);
      SET_BIT(obj->value[2], HENEKAR_PACIFY);
      AddEvent(EVENT_OBJ_TIMER, PULSE_WEEK, TRUE, obj, HenekarR);
      SET_BIT(obj->value[2], HENEKAR_CHARM);
      AddEvent(EVENT_OBJ_TIMER, PULSE_HOUR, TRUE, obj, HenekarM);
      SET_BIT(obj->value[2], HENEKAR_MUSICPATH);

      BACKUP_OBJECT(obj);
      return IDX_COMMAND | IDX_WEAPON_HIT | IDX_PERIODIC | IDX_IDENTIFY;
   }

   /* trap for special id */
   if (calltype == PROC_SPECIAL_ID)
   {
      sprintf(idString,
              "Called Effects:  (invoked by 'say'ing them)\n"
              "                 'you see darkness'  Casts blindness   1/day\n"
              "                 'peace to you' Pacifies a Mob         1/12 hours\n"
              "                 'join my quest' Charms a mob          1/12 hours\n"
              "                 'sonic path to' world teleport        1/hour\n"
              "Combat Hit: Hitpoint sucking Strike");
      specialProcedureIdentifyObject(obj, ch, calltype, arg,
                                     ID_CLASS_BARD, 0, 0, idString);
      return TRUE;
   }

   if ((calltype == PROC_EVENT) && (arg))
   {
      /* Recharge spell effect from event */
      if (!strcmp(arg, HenekarD))
      {
         REMOVE_BIT(obj->value[2], HENEKAR_BLIND);
         return FALSE;
      }
      if (!strcmp(arg, HenekarC))
      {
         REMOVE_BIT(obj->value[2], HENEKAR_PACIFY);
         return FALSE;
      }
      if (!strcmp(arg, HenekarR))
      {
         REMOVE_BIT(obj->value[2], HENEKAR_CHARM);
         return FALSE;
      }
      if (!strcmp(arg, HenekarM))
      {
         REMOVE_BIT(obj->value[2], HENEKAR_MUSICPATH);
         return FALSE;
      }

      owner = getObjectOwner(obj);

      /* Here on out, we are a valid wielder */
      RESTORE_WEAPON(obj);
      return TRUE;
   }

   /* If the player ain't here, why are we? */
   if (!ch || !obj)
      return FALSE;

   /* Check for God Reset of Item */
   if (godMaintenanceCommands(obj, ch, cmd, arg))
      return TRUE;

   /* Are we wielding it? */
   if (!OBJ_WORN(obj))
      return FALSE;

   /* Now process called commands. */
   if (word && ((cmd == CMD_SAY) || (cmd == CMD_SAY2)))
   {
      sscanf(arg, "%s %s %s %s \n", word1, word2, word3, word4);
      sprintf(word, "%s %s %s", word1, word2, word3);

      if (!strcmp(word, HENEKAR_BLIND_CMD))
      {
         if (!IS_SET(obj->value[2], HENEKAR_BLIND))
         {
            if (ch->specials.fighting)
            {
               target = ch->specials.fighting;
            }
            else
            {
               send_to_char("You must be in battle.\n", ch);
               return FALSE;
            }

            act("&n&+bYou call $p &n&+bfor &+Bthe power to blind&+b..\n"
                "&+BA magical bolt of energy spits forth from the horn, &n&+bblue &+Bin color.",
                FALSE, ch, obj, 0, TO_CHAR);
            act("&+BThe bolt strikes $N&n&+B in the face, who reels backwards, blind and helpless!",
                FALSE, ch, obj, target, TO_CHAR);
            act("$n&+bmutters something to &s $p.\n"
                "&+BA magical bolt of enery spits forth from $s's $p&+B, like a tiny arrow.",
                FALSE, ch, obj, 0, TO_ROOM);
            act("&+BThe bolt strikes $N&+B dead in the face, who reels backwards, blind and helpless!",
                FALSE, ch, obj, 0, TO_ROOM);

            spell_blindness(-1, ch, target, obj);

            SET_BIT(obj->value[2], HENEKAR_BLIND);
            AddEvent(EVENT_OBJ_TIMER, (PULSE_DAY / 3), TRUE, obj, HenekarD);
            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', HENEKAR_BLIND_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, HENEKAR_PACIFY_CMD))
      {
         if (!IS_SET(obj->value[2], HENEKAR_PACIFY))
         {
            target = get_char_room_vis(ch, word4);
            if (!target)
            {
               send_to_char("To whome do you wish to pacify?\n", ch);
               return TRUE;
            }
            if ((!IS_NPC(target)) || (GET_RACE(target) == RACE_DRAGON) ||
                (GET_RACE(target) == RACE_UNDEAD))
            {
               send_to_char("You cannot pacify a that person.\n", ch);
               return TRUE;
            }

            act("&+bYou call $p &n&+bto bring peace to $N&n&+b..\n"
                "&+BWith a powerful blast, the horn jets forth energy at $N&+B, which rapidly surrounds $H!",
                FALSE, ch, obj, target, TO_CHAR);
            act("$N&+C screams in agony for a moment, unable to escape the permiating energy!\n"
                "&+BThen, slowly, $S begins to calm down, stopping dead and relaxing.",
                FALSE, ch, obj, target, TO_CHAR);
            act("$N&n&+b becomes completely passive, staring out into space, unmoving..",
                FALSE, ch, obj, target, TO_CHAR);
            act("&n$n&+b mutters something to $s $p.\n"
                "&+BWith a powerful blast, the horn jets forth energy at $N&+B, which rapidly surrounds $H!",
                FALSE, ch, obj, target, TO_ROOM);
            act("$N&+C screams in agony for a moment, unable to escape the permiating energy!\n"
                "&+BThen, slowly, $S begins to calm down, stopping dead and relaxing.",
                FALSE, ch, obj, target, TO_ROOM);
            act("&n&+b becomes completely passive, staring out into space, unmoving..",
                FALSE, ch, obj, target, TO_ROOM);

            if (IS_CSET(target->only.npc->npcact, ACT_AGGRESSIVE))
               REMOVE_CBIT(target->only.npc->npcact, ACT_AGGRESSIVE);
            if (IS_CSET(target->only.npc->npcact, ACT_AGGRESSIVE_GOOD))
               REMOVE_CBIT(target->only.npc->npcact, ACT_AGGRESSIVE_GOOD);
            if (IS_CSET(target->only.npc->npcact, ACT_AGGRESSIVE_NEUTRAL))
               REMOVE_CBIT(target->only.npc->npcact, ACT_AGGRESSIVE_NEUTRAL);
            if (IS_CSET(target->only.npc->npcact, ACT_AGGRESSIVE_EVIL))
               REMOVE_CBIT(target->only.npc->npcact, ACT_AGGRESSIVE_EVIL);
            if (IS_CSET(target->only.npc->npcact, ACT_AGG_RACEEVIL))
               REMOVE_CBIT(target->only.npc->npcact, ACT_AGG_RACEEVIL);
            if (IS_CSET(target->only.npc->npcact, ACT_AGG_RACEGOOD))
               REMOVE_CBIT(target->only.npc->npcact, ACT_AGG_RACEGOOD);
            if (IS_CSET(target->only.npc->npcact, ACT_AGG_OUTCAST))
               REMOVE_CBIT(target->only.npc->npcact, ACT_AGG_OUTCAST);

            if (target->specials.fighting)
               stop_fighting(target);
            if (ch->specials.fighting)
               if (ch->specials.fighting == target)
                  stop_fighting(ch);

            memClear(target);

            SET_BIT(obj->value[2], HENEKAR_PACIFY);
            AddEvent(EVENT_OBJ_TIMER, (PULSE_WEEK / 3), TRUE, obj, HenekarC);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', HENEKAR_PACIFY_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, HENEKAR_CHARM_CMD))
      {
         if (!IS_SET(obj->value[2], HENEKAR_CHARM))
         {
            target = get_char_room_vis(ch, word4);
            if (!target)
            {
               send_to_char("Charm who?\n", ch);
               return (FALSE);
            }

            if ((GET_RACE(target) == RACE_DRAGON) ||
                (GET_RACE(target) == RACE_UNDEAD) ||
                (GET_MAX_HIT(target) > 2500))
            {
               send_to_char("The horn refuses, your foe is too powerful..\n", ch);
               return (FALSE);
            }

            if (IS_PC(target))
            {
               send_to_char("You cannot charm PC's.\n", ch);
               return (FALSE);
            }

            act("&+bYou call $p &n&+bto join your quest..\n"
                "&+BThe Horn's opening glows &+Cbrightly &+Bwith bluish magical energy.",
                FALSE, ch, obj, target, TO_CHAR);
            act("&+BBlue streams of power emanate from your Horn, streaming towards $N!\n"
                "\n&+CFoooooooooosh\n",
                FALSE, ch, obj, target, TO_CHAR);
            act("&+BThe streams envelop $N&+B, who tries desperately to escape the magic!\n!"
                "$N&+b finally stops, turns to face you, and bows obediently.",
                FALSE, ch, obj, target, TO_CHAR);
            act("&n$n &+bmutters something to $s $p.\n"
                "&+BThe Horn' opening begins to glow &+Cbrightly &+Bwith bluish magical energy..",
                FALSE, ch, obj, target, TO_ROOM);
            act("&+BBlue streams of power emanate from the horn, streaming towards $N!\n"
                "\n&+CFoooooooooosh\n",
                FALSE, ch, obj, target, TO_ROOM);
            act("&+BThe streams envelop $N&+L, who tries desperately to escape the magic!\n"
                "$N&+b finally stops, turns to face &n$n&n&+b, and bows obediently.",
                FALSE, ch, obj, target, TO_ROOM);

            bzero(&af, sizeof(af));
            af.type = SPELL_CHARM_PERSON;
            af.duration = 9999;
            SET_CBIT(af.sets_affs, AFF_CHARM);
            add_follower(target, ch, TRUE);
            affect_to_char(target, &af);

            SET_BIT(obj->value[2], HENEKAR_CHARM);
            AddEvent(EVENT_OBJ_TIMER, PULSE_WEEK, TRUE, obj, HenekarR);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', HENEKAR_CHARM_CMD);
            return TRUE;
         }
      }

      else if (!strcmp(word, HENEKAR_MUSICPATH_CMD))
      {
         if (!IS_SET(obj->value[2], HENEKAR_MUSICPATH))
         {
            target = get_char_in_game_vis(ch, word4, ch->in_room);
            if (!target)
            {
               send_to_char("To whome do you wish to walk the sonic waves to?\n", ch);
               return TRUE;
            }
            if (!IS_PC(target))
            {
               send_to_char("To whome do you wish to walk the sonic waves to?\n", ch);
               return TRUE;
            }

            if (GET_LEVEL(target) > 50)
            { // targetting gods gets you sent to outhouse
               act("&+bYou call $p &n&+bfor a sonic path to $N..\n"
                   "&+BThe air before you ripples like disturbed water, and draws you in!",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&+ROh No!!! Somthing has gone horribly wrong..",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&n$n &+bmuttes something to $s $p.\n"
                   "&+BA rippling wave in the air forms. \n$n steps into it, and they both vanish into silence!",
                   FALSE, ch, obj, target, TO_ROOM);

               char_from_room(ch);
               char_to_room(ch, real_room(16706), -1);
            }
            else
            {
               act("&+bYou call $p &n&+bfor a sonic path to $N..\n"
                   "&+BThe air before you ripples like disturbed water, and draws you in!",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&+BWhen you emerge from the rippling wave on the other side, you are elsehwere..",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&n$n &+bmuttes something to $s $p.\n"
                   "&+BA rippling wave in the air forms. \n$n steps into it, and they both vanish into silence!",
                   FALSE, ch, obj, target, TO_ROOM);

               char_from_room(ch);

               if (ZONE_FLAGGED(GET_ROOM_ZONE(target->in_room), ZONE_WILDERNESS))
               {
                  X_LOC(ch) = world[target->in_room].coords[0];
                  Y_LOC(ch) = world[target->in_room].coords[1];
               }

               char_to_room(ch, target->in_room, -1);
            }

            Stun(ch, (PULSE_VIOLENCE * 3));

            SET_BIT(obj->value[2], HENEKAR_MUSICPATH);
            AddEvent(EVENT_OBJ_TIMER, PULSE_HOUR, TRUE, obj, HenekarM);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', HENEKAR_MUSICPATH_CMD);
            return TRUE;
         }
      }
      return FALSE;
   }

   if (calltype == PROC_INITIALIZE)
      return IDX_WEAPON_HIT;

   return FALSE;
}

int New_Avernus(P_obj obj, P_char ch, int cmd, char *arg)
{
   char idString[MAX_STRING_LENGTH];
   P_char vict = NULL;
   int dam = 0, calltype = 0;

   PARSE_ARG(cmd, calltype, cmd);

   /* check for periodic event calls */
   if (calltype == PROC_INITIALIZE)
      return IDX_WEAPON_HIT | IDX_IDENTIFY;

   if (calltype == PROC_SPECIAL_ID)
   { /* Special ID to show Proc */
      sprintf(idString, "Combat Bonuses:  +20 To Hit, +8 Damage\nSpecial Effects: Bladesong\n");
      specialProcedureIdentifyObject(obj, ch, calltype, arg, 0, 0, 0, idString);
      return TRUE;
   }

   if ((GET_HIT(ch) < 100) && (number(0, 100) < 30))
   {
      act("&n$p&+L glows with blackish energy, drawing from it's reserves of power..\n"
          "&+rReddish energy&n&+L courses into your body, feeding you power and the will to live.",
          FALSE, ch, obj, 0, TO_CHAR);
      act("&n$p&+L glows with blackish energy, just as $n nears death, humming with power..\n"
          "&+rReddish energy&n&+L courses into $n, whose wounds heal and eyes glow with rage!",
          FALSE, ch, obj, 0, TO_ROOM);
      GET_HIT(ch) = GET_MAX_HIT(ch);
      return TRUE;
   }

   if (calltype != PROC_WEAPON_HIT || !ch || !OBJ_WORN_BY(obj, ch) || !OBJ_WORN_POS(obj, WIELD))
      return (FALSE);

   vict = (P_char)arg;

   if (!vict)
      return FALSE;

   /* A little style. */
   if (!number(0, 10))
   {
      act("&+LYou charge and dash, evade and parry with the skill of a master warrior.",
          FALSE, ch, obj, vict, TO_CHAR);
      act("&+L$n charges and dashes, parrying and strinking with the skill of a master warrior.",
          FALSE, ch, obj, vict, TO_ROOM);
      if (GET_HIT(ch) < (GET_MAX_HIT(ch) - 10))
         GET_HIT(ch) += 2;
   }

   /* Stand's if bashed */
   if (GET_POS(ch) != POS_STANDING)
   {
      act("&+LYou spring from the ground like a leopard, coming up into a fighting stance!\n"
          "&+LNothing can hold you to the ground, you must fight with the strength of giants!",
          FALSE, ch, obj, vict, TO_CHAR);
      act("&+L$n springs from the ground like a leopard, coming up into a fighting stance!\n"
          "&+LHe goes wild as if nothing can hold him to the ground, rage fills his eyes",
          FALSE, ch, obj, vict, TO_ROOM);
      SET_POS(ch, POS_STANDING + GET_STAT(ch));
   }

   /* Main proc */
   dam = BOUNDED(0, (GET_HIT(vict) + 9), 250);

   if (vict)
   {
      if ((!number(0, 30)) && (GET_HIT(ch) < (GET_MAX_HIT(ch) + 1000)))
      {
         act("&+LAvernus, the life stealer &N&=LCglows&n&+W brightly&+C in your hands as it dives\n"
             "&+Cdeep into $N&+C.  &+r$p &+rdraws the precious life force from &n$N\n"
             "&+CYou feel energy flow into your body as &+LAvernus&+C feeds your soul...",
             FALSE, ch, obj, vict, TO_CHAR);
         act("$n's &+Csword &=LCglows&n&+C with a &+Wbright flash&+C as it cuts mercilessly\n"
             "&+Cinto $N&+C!  &+LA dark stream of &n&+rreddish energy &+Lflows up the sword's blade,\n"
             "&+Cfeeding &n&+r$n&+L the life energy\n&+Lof &n&+r$N&+r, &+Lwho screams in agony!",
             FALSE, ch, obj, vict, TO_NOTVICT);
         act("$n's sword &+Wglows with a bright light as it bites into you.",
             FALSE, ch, obj, vict, TO_VICT);
         act("&+LYou feel your life flowing away and &+W$n&+L looks revitalized.",
             FALSE, ch, obj, vict, TO_VICT);
         GET_HIT(ch) += dam;
         GET_HIT(vict) -= (dam * 3);
         return TRUE;
      }
   }
   if (ch->specials.fighting)
      return FALSE; /* do the normal hit damage as well */
   else
      return TRUE;
}

int Kelrom(P_obj obj, P_char ch, int cmd, char *arg)
{
   P_char vict = NULL, tch = NULL;
   int healback = 0, curhits = 0, calltype = 0, dam = 0, limit = 0;
   P_char leader = ch;
   P_gmember member = NULL;

   PARSE_ARG(cmd, calltype, cmd);

   /* check for periodic event calls */
   if (calltype == PROC_INITIALIZE)
      return IDX_WEAPON_HIT;

   if (!ch || (calltype != PROC_WEAPON_HIT) || !OBJ_WORN_BY(obj, ch) || !OBJ_WORN_POS(obj, WIELD))
      return FALSE;

   vict = (P_char)arg;

   if (!vict)
      return FALSE;

   if (GET_RACE(vict) == RACE_ANIMAL)
   {
      act("&+CA bright flash of energy erupts from the mighty axe!\n"
          "&+cYou feel shame fill you, you feel dread as you attack this poor animal!",
          FALSE, ch, obj, 0, TO_CHAR);
      act("&+cThe extreme displeasure of Pahluruk flows through you, bites at your soul,\n"
          "&+cand stops your heart from beating instantly, punnishment for your sin",
          FALSE, ch, obj, 0, TO_CHAR);
      act("&+LYou feel .. darkness. You feel .. shame. You fall into oblivion..",
          FALSE, ch, obj, 0, TO_CHAR);

      act("&+CA bright flash of energy erupts from &n$n's&+C mighty axe!\n"
          "$n's&+c face fills with shame as he realizes he struck and an animal!",
          FALSE, ch, obj, 0, TO_CHAR);
      act("&+cThe extreme displeasure of $s goddess flows into $m, draining $m of life!\n"
          "&+L$e falls to the ground, lifeless.. His expression one of sadnes..",
          FALSE, ch, obj, 0, TO_CHAR);

      SuddenDeath(ch, ch, "Kelrom's punnishment for attacking an animal.. The fool!");
      return TRUE;
   }

   /* The mega blast! */
   if (GET_ALIGNMENT(ch) > 350)
   {
      if ((ch->following && GET_GROUP(ch)) && (!number(0, 80)))
      {
         leader = ch->following;
         /*
               act("&+CThe weapon glows briefly, then fades out as if drained of energy..",
                   FALSE, ch, obj, 0, TO_CHAR);
         */
         act("&+CA bright flash of energy erupts from the mighty axe!\n"
             "&+cAs you look in awe, pure energy flows up from the earth into the weapon",
             FALSE, ch, obj, 0, TO_CHAR);
         act("&+yThe energy flows through you, and everyone with you, filling you with\n"
             "&+yhealth and strength.. Pahluruk's blessing encompases you with her essence..",
             FALSE, ch, obj, 0, TO_CHAR);

         act("&+CA bright flash of energy erupts from &n&n's&+C mighty axe!\n"
             "&+cAs you look in awe, pure energy flows up from the earth into the weapon",
             FALSE, ch, obj, 0, TO_ROOM);
         act("&+yThe energy flows through &n$n&+y, and everyone with $m, filling them with\n"
             "&+yhealth and strength.. Pahluruk's blessing encompases all with her essence..",
             FALSE, ch, obj, 0, TO_ROOM);

         LOOP_THRU_GROUP(member, GET_GROUP(ch))
         if (member->this->in_room == ch->in_room)
         {
            if (member->this != ch)
               act("Your blessing heals $N!", FALSE, ch, 0, member->this, TO_CHAR);
            act("$n's blessing heals $N!", FALSE, ch, 0, member->this, TO_NOTVICT);
            spell_full_heal(50, ch, tch, 0);
            limit--;
         }

         return (TRUE);
      }

      if (!number(0, 50))
      {
         /*
              act("&+WThe weapon glows Brightly for an instant, but the magic fades..\n"
                   "&+LYou are left with a feeling of emptiness, loss, and betrayal..",
                   FALSE, ch, obj, 0, TO_CHAR);
         */
         act("&=LWFLASH! &n&+CBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM\n"
             "$n's&+C axe crackles with energy as a massive lightning bolt erupts forth from it!",
             FALSE, ch, obj, 0, TO_ROOM);
         act("&+CYour hair stands on end as the lightning bolt strikes &n$N&n&+C dead on!\n"
             "&+CThe force of the blast is blinding, the roar shakes the ground mightily..",
             FALSE, ch, obj, 0, TO_ROOM);
         act("&+c$N&n&+c is hurled through the air, a look of unbearable agony on its face..\n"
             "&+cYou stand in awe of $n, the might power of Pahluruk flowing through $s!",
             FALSE, ch, obj, 0, TO_ROOM);

         act("&=LWFLASH! &n&+CBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM\n"
             "&+CYour axe crackles with energy as a massive lightning bolt erupts forth from it!",
             FALSE, ch, obj, 0, TO_ROOM);
         act("&+CYour hair stands on end as the lightning bolt strikes &n$N&n&+C dead on!\n"
             "&+CThe force of the blast is blinding, the roar shakes the ground mightily..",
             FALSE, ch, obj, 0, TO_ROOM);
         act("&+c$N&n&+c is hurled through the air, a look of unbearable agony on its face..\n"
             "&+cYou stand in awe of Pahluruk power, and feel her presence in your heart..",
             FALSE, ch, obj, 0, TO_ROOM);

         CharWait(vict, (PULSE_VIOLENCE * 3));
         CharWait(ch, PULSE_VIOLENCE);

         if (GET_HIT(vict) < 500)
         {
            SuddenDeath(vict, ch, "Kelrom's awesome blast!");
            vict = NULL;
         }
         else
         {
            GET_HIT(vict) -= 500;
            if ((GET_HIT(ch) + 500) < (GET_MAX_HIT(ch) * 2))
               GET_HIT(ch) += 500;
            else
               GET_HIT(ch) = (GET_MAX_HIT(ch) * 2);
         }

         return TRUE;
      }
   }

   if (!number(0, 25))
   {
      act("$n's&+C mighty axe glows with magical fury as $e strike deep into &n$N!\n"
          "$n&+C roars with fury as electrical energy flows throughout $s body!",
          FALSE, ch, obj, vict, TO_CHAR);
      act("&+WThe power of Pahluruk ripples into &n$N&+W, who screams in agony!\n"
          "&+CWith a mighty CRASH and a loud clap of thunder, $E is thrown backwards.",
          FALSE, ch, obj, vict, TO_CHAR);

      act("&+CThe mighty axe glows with magical fury as you strike deep into &n$N!\n"
          "&+CYou roar with fury as electrical energy flows throughout your body!",
          FALSE, ch, obj, vict, TO_CHAR);
      act("&+WThe power of Pahluruk ripples into &n$N&+W, who screams in agony!\n"
          "&+CWith a mighty CRASH and a loud clap of thunder, $E is thrown backwards.",
          FALSE, ch, obj, vict, TO_CHAR);
      act("&+cWhen the energy fades, you feel charged and alive with the power of\n"
          "&+cyour goddess.. An omen of might, a blessing of fortitude..",
          FALSE, ch, obj, vict, TO_CHAR);

      /* Bam! */
      dam = BOUNDED(0, (GET_HIT(vict) + 9), 250);
      GET_HIT(vict) -= dam;

      /* A heal on every hit! */
      healback = dam;
      curhits = GET_HIT(ch);
      if ((healback + curhits) >= GET_MAX_HIT(ch))
      {
         if (GET_HIT(ch) < GET_MAX_HIT(ch))
            GET_HIT(ch) = GET_MAX_HIT(ch);
         else if ((GET_HIT(ch) + healback) < (GET_MAX_HIT(ch) * 2))
            GET_HIT(ch) += healback;
      }
      else if ((GET_HIT(ch) + healback) < (GET_MAX_HIT(ch) * 2))
         GET_HIT(ch) += healback;

      /* Should we hit em with another harm? */
      if (GET_STAT(vict) != STAT_DEAD)
      {
         if (!number(0, 5))
         {
            dam = BOUNDED(0, (GET_HIT(vict) + 9), 250);
            GET_HIT(vict) -= dam;
         }
      }
   }
   else
   {
      if (!ch->specials.fighting)
         set_fighting(ch, vict);
   }

   if (ch->specials.fighting)
      return FALSE; /* do the normal hit damage as well */
   else
      return TRUE;
}

/* The New Doombringer */

static char DoombringerS[] = "DOOMY_SPHERE";
static char DoombringerL[] = "DOOMY_LIGHTNING";
static char DoombringerM[] = "DOOMY_MEGADOOM";
static char DoombringerZ[] = "DOOMY_SPAZ";

#define DOOMY_SPHERE BIT_1    /* Sphere of Annhilation!  */
#define DOOMY_LIGHTNING BIT_2 /* Black Lightning of Doom */
#define DOOMY_MEGADOOM BIT_3  /* The MegaDoom Attack     */
#define DOOMY_SPAZ BIT_4      /* Normal Doom attack      */

#define DOOMY_SPHERE_CMD "bring annhilation forth!"
#define DOOMY_LIGHTNING_CMD "feel my power"
#define DOOMY_MEGADOOM_CMD "enrage me doombringer!"

int Doombringer(P_obj obj, P_char ch, int cmd, char *arg)
{
   /* Doombringer             Wielded initially by Trogar                       */
   /*                                                                           */
   /* Classes Allowed: Warrior                                                  */
   /* Base Damage:     5d4                                                      */
   /* Bonuses:         +8 hitroll, +8 damage.                                   */
   /* Called Effects:  'bring annhilation forth!' Sphere of ahhilateion  1/week */
   /* Called Effects:  'feel my power <target>' Black Lightning         1/day   */
   /* Called Effects:  'enrage me doombringer!" The Mega Doom!           1/day  */
   /* Continual Effects: None.                                                  */
   /* Special Id:      Yes                                                      */
   P_char owner = NULL, target = NULL;
   char idString[MAX_STRING_LENGTH], word[MAX_STRING_LENGTH];
   char word1[MAX_STRING_LENGTH], word2[MAX_STRING_LENGTH];
   char word3[MAX_STRING_LENGTH], word4[MAX_STRING_LENGTH];
   int calltype = 0, Count = 0;

   /* backup the values */
   BACKUP_VARIABLES();

   PARSE_ARG(cmd, calltype, cmd);

   /* Initialize object */
   if (calltype == PROC_INITIALIZE)
   {
      ClearObjEvents(obj);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              DOOMY_SPHERE, PULSE_WEEK, DoombringerS);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              DOOMY_LIGHTNING, PULSE_DAY, DoombringerL);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              DOOMY_MEGADOOM, PULSE_DAY, DoombringerM);
      initializeObjectFeature(ITEM_WEAPON, obj,
                              DOOMY_SPAZ, PULSE_HOUR, DoombringerZ);

      BACKUP_OBJECT(obj);
      return IDX_COMMAND | IDX_WEAPON_HIT | IDX_PERIODIC | IDX_IDENTIFY;
   }

   /* trap for special id */
   if (calltype == PROC_SPECIAL_ID)
   {
      sprintf(idString,
              "Called Effects:  (invoked by 'say'ing them)\n"
              "                 'bring annhilation forth!' Summons a Shere of Annhilation!  1/week\n"
              "                 'feel my power <target>' Summons Black Lightning.           1/day\n"
              "                 'enrage me doombringer!' envoke the Mega Doom rage!         1/day\n"
              "Combat Critical: Hitpoint Sucking/Alignment Reducing Strike");
      return TRUE;
   }

   if (calltype == PROC_EVENT)
   {
      /* Recharge spell effect from event */
      if (rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                DOOMY_SPHERE, DoombringerS) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                DOOMY_LIGHTNING, DoombringerL) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                DOOMY_MEGADOOM, DoombringerM) ||
          rechargeObjectFeature(ITEM_WEAPON, obj, ch, PROC_EVENT, arg,
                                DOOMY_SPAZ, DoombringerZ))
         return FALSE;

      owner = getObjectOwner(obj);

      if (owner)
         /* Now we check to be sure the guy wielding this falls under the */
         /* restrictions we have set */
         if (checkObjectRestrictions(ITEM_WEAPON, obj, owner, OBJ_RESTRICT_CLASS,
                                     OBJ_RESTRICT_WARRIOR))
         {
            performRestrictionPenalty(ITEM_WEAPON, obj, owner, OBJ_BURN);
            return TRUE;
         }
      /* Here on out, we are a valid wielder */
      RESTORE_WEAPON(obj);
      return TRUE;
   }

   /* If the player ain't here, why are we? */
   if (!ch || !obj)
      return FALSE;

   /* Check for God Reset of Item */
   if (godMaintenanceCommands(obj, ch, cmd, arg))
      return TRUE;

   /* Are we wielding it? */
   if (!OBJ_WORN(obj))
      return FALSE;

   /* Now process called commands. */
   if (word && ((cmd == CMD_SAY) || (cmd == CMD_SAY2)))
   {
      sscanf(arg, "%s %s %s %s \n", word1, word2, word3, word4);
      sprintf(word, "%s %s %s", word1, word2, word3);

      /* The Sphere of Annhilation */
      if (!strcmp(word, DOOMY_SPHERE_CMD))
      {
         target = ch->specials.fighting;

         if (!target)
         {
            send_to_char("Annhilate who?\n", ch);
            return FALSE;
         }

         if (!IS_SET(obj->value[0], DOOMY_SPHERE))
         {
            act("&+LYou mutter arcane words and call upon Doombringer for the power of annhilation!\n"
                "&+LDoombringer glows with a blackish aura as darkness pours forth from it..",
                FALSE, ch, obj, target, TO_CHAR);
            act("&+LIt forms into a perfect sphere, one foot in diameter, glowing black with energy!\n"
                "&+LThe sphere rushes at &n$N&+L, who cannot escape in time..",
                FALSE, ch, obj, target, TO_CHAR);
            act("&+L$n mutters arcane words and calls upon Doombringer for the power of annhilation!\n"
                "&+L$s Doombringer glows with a blackish aura as darkness pours forth from it..",
                FALSE, ch, obj, target, TO_ROOM);
            act("&+LIt forms into a perfect sphere, one foot in diameter, glowing black with energy!\n"
                "&+LThe sphere rushes at &n$N&+L, who cannot escape in time..",
                FALSE, ch, obj, target, TO_ROOM);
            act("&+L   \n"
                "&+L               OOOOOO      ",
                FALSE, target, 0, 0, TO_ROOM);
            act("&+L             OOOOOOOOOO    \n"
                "&+L            OOOOOOOOOOOO   ",
                FALSE, target, 0, 0, TO_ROOM);
            act("&+L            OOOOOOOOOOOO   \n"
                "&+L             OOOOOOOOOO    ",
                FALSE, target, 0, 0, TO_ROOM);
            act("&+L               OOOOOO      \n"
                "&+L   ",
                FALSE, target, 0, 0, TO_ROOM);
            act("&+LFoooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooosh!\n"
                "&+LYou watch with awe and wonder as the sphere instantly sucks &n$N&+L in.",
                FALSE, ch, obj, target, TO_NOTVICT);
            act("&+LThere is no scream, no sound, no visual as to what happens in that instant.\n"
                "&+LThe sphere and &n$N&+L both simple vanish from sight, and are gone...\n",
                FALSE, ch, obj, target, TO_NOTVICT);

            if (GET_ALIGNMENT(target) > 350)
            {
               act("&+rYou feel utterly drained of all energy, as the expenditure of strength needed\n"
                   "&+rto summon annhilation is beyond reckoning. You succumb to exhaustion and feel\n"
                   "&+rsomehow darker for the entire experience..",
                   FALSE, ch, obj, 0, TO_CHAR);
               if (GET_ALIGNMENT(ch) < -975)
                  GET_ALIGNMENT(ch) = -1000;
               else
                  GET_ALIGNMENT(ch) = (GET_ALIGNMENT(ch) - 25);
            }
            else
            {
               act("&+rYou feel utterly drained of all energy, as the expenditure of strength needed\n"
                   "&+rto summon annhilation is beyond reckoning. You succumb to exhaustion..",
                   FALSE, ch, obj, 0, TO_CHAR);
            }

            char_from_room(target);
            char_to_room(target, real_room(87), -1);
            extract_char(target);
            target = NULL;

            act("&+rThe expenditure of so much power drains $n's face of color, $s eyes roll back\n"
                "&+rinto $s head, and $e collapses weakly to the floor from exhaustion..",
                FALSE, ch, obj, 0, TO_ROOM);
            SET_POS(ch, POS_PRONE + GET_STAT(ch));
            Stun(ch, (PULSE_VIOLENCE * 3));

            SET_BIT(obj->value[0], DOOMY_SPHERE);
            AddEvent(EVENT_OBJ_TIMER, PULSE_WEEK, TRUE, obj, DoombringerS);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', DOOMY_SPHERE_CMD);
            return TRUE;
         }
      }

      /* Doombringers blast of Black Ligntning */
      else if (!strcmp(word, DOOMY_LIGHTNING_CMD))
      {
         if (!IS_SET(obj->value[0], DOOMY_LIGHTNING))
         {
            target = get_char_room_vis(ch, word4);
            if (!target)
            {
               send_to_char("Who do you wish to hurl lightning at?\n", ch);
               return (FALSE);
            }
            if (IS_PC(target))
            {
               send_to_char("You cannot harm PC's.\n", ch);
               return (FALSE);
            }

            if (GET_LEVEL(target) < 60)
            {
               act("&+LYou utter arcane words to Doombringer, which responds with a &=LWbright flash!\n"
                   "&+LImmediately following the flash, twin bolts of super-heated Black Lightning stream\n"
                   "&+Loutwards from the mighty sword, accompanied by the massive blast of thunder!",
                   FALSE, ch, obj, target, TO_CHAR);
               act("&+LDoooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooom\n"
                   "$N &+Lis blown backwards by the blasts, crumpling to the floor in\n"
                   "&+Lin what can only be described as Pure Agony. For the victim of Doombringer can\n"
                   "&+Lknow only pain, and only death..",
                   FALSE, ch, obj, target, TO_CHAR);
               act("$n &+Lutters arcane words to Doombringer, which responds with a &=LWbright flash!\n"
                   "&+LImmediately following the flash, twin bolts of super-heated black lightning stream\n"
                   "&+Loutwards from the mighty sword, accompanied by the massive blast of thunder!",
                   FALSE, ch, obj, target, TO_ROOM);
               act("&+LDoooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooom\n"
                   "$N &+Lis blown backwards by the blasts, crumpling to the floor in\n"
                   "&+Lin what can only be described as Pure Agony. For the victim of Doombringer can\n"
                   "&+Lknow only pain, and only death..",
                   FALSE, ch, obj, target, TO_ROOM);

               if ((GET_HIT(target) - 500) < 0)
                  SuddenDeath(target, ch, "&+LDoombringer's Black Lightning! &+WO_O");
               else
                  GET_HIT(target) = (GET_HIT(target) - 500);

               act("&+rYou feel drained from the expendature of energy..",
                   FALSE, ch, obj, 0, TO_CHAR);
               GET_MOVE(ch) = 0;
               GET_MANA(ch) = 0;
            }

            SET_BIT(obj->value[0], DOOMY_LIGHTNING);
            AddEvent(EVENT_OBJ_TIMER, PULSE_DAY, TRUE, obj, DoombringerL);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', DOOMY_LIGHTNING_CMD);
            return TRUE;
         }
      }

      /* The Mega Dooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooom O_O */
      else if (!strcmp(word, DOOMY_MEGADOOM_CMD))
      {
         if (!IS_SET(obj->value[0], DOOMY_MEGADOOM))
         {
            target = ch->specials.fighting;
            if (!target)
            {
               send_to_char("&+LYou cannot invoke that power unless in combat..\n", ch);
               return TRUE;
            }

            act("&+LYou utter a series of strange arcane words to mighty Doombringer..\n"
                "&+LThe sword eagerly responds as black tendrils of power flow down the blase into you!\n"
                "&+LYour eyes glow black, your movements blur and accelerate to unbelievable speeds!\n"
                "&+LYou find yourself flinging about, striking over and over with incredible rage\n"
                "&+LThe tempo increases, you feel POWER beyond comprehention flow into your attack!",
                FALSE, ch, obj, 0, TO_CHAR);
            act("$n &+Lutters a series of strange arcane words to his mighty sword, Doombringer..\n"
                "&+LThe sword eagerly responds as black tendrils of power flow down the blase into $m!\n"
                "&+L$s eyes glow black, and his movements blur and accelerate to unbelievable speeds!\n"
                "&+L$n flings about so fast you can't even watch $m, striking over and over with rage!\n"
                "&+L$s tempo increases, more energy streams into $m and $e explodes with fury!",
                FALSE, ch, obj, 0, TO_ROOM);

            /* Omg the MEGA DOOM O_O */
            for (Count = 1; Count <= 20; Count++)
            {
               if (ch->specials.fighting)
                  hit(ch, ch->specials.fighting, TYPE_UNDEFINED);
               else
                  break;
            }

            act("&+rYou feel drained from the expendature of energy..",
                FALSE, ch, obj, 0, TO_CHAR);
            Stun(ch, (PULSE_VIOLENCE * 3));
            GET_MOVE(ch) = 0;
            GET_MANA(ch) = 0;

            SET_BIT(obj->value[0], DOOMY_MEGADOOM);
            AddEvent(EVENT_OBJ_TIMER, PULSE_DAY, TRUE, obj, DoombringerM);

            return TRUE;
         }
         else
         {
            usedFeatureMessage(obj, ch, 'L', DOOMY_MEGADOOM_CMD);
            return TRUE;
         }
      }
      return FALSE;
   }

   if (!ch || (calltype != PROC_WEAPON_HIT) || !OBJ_WORN_BY(obj, ch) || !OBJ_WORN_POS(obj, WIELD))
      return FALSE;

   target = ch->specials.fighting;
   if (!target || IS_TRUSTED(target))
      return FALSE;

   if (IS_NPC(target))
   {
      if (!IS_SET(obj->value[0], DOOMY_SPAZ))
      {
         if (number(0, 30) == 8)
         {
            act("&+LYou utter a series of strange arcane words to mighty Doombringer..\n"
                "&+LThe sword eagerly responds as black tendrils of power flow down the blase into you!\n"
                "&+LYour eyes glow black, your movements blur and accelerate to unbelievable speeds!\n"
                "&+LYou find yourself flinging about, striking over and over with incredible rage!",
                FALSE, ch, obj, 0, TO_CHAR);
            act("$n &+Lutters a series of strange arcane words to his mighty sword, Doombringer..\n"
                "&+LThe sword eagerly responds as black tendrils of power flow down the blase into $m!\n"
                "&+L$s eyes glow black, and his movements blur and accelerate to unbelievable speeds!\n"
                "&+L$n flings about so fast you can't even watch $m, striking over and over with rage!",
                FALSE, ch, obj, 0, TO_ROOM);

            for (Count = 1; Count <= 5; Count++)
            {
               if (ch->specials.fighting)
                  hit(ch, ch->specials.fighting, TYPE_UNDEFINED);
               else
                  break;
            }

            /* The Hitch ;) */
            if (GET_ALIGNMENT(target) > 350)
            {
               if (GET_ALIGNMENT(ch) < -999)
                  GET_ALIGNMENT(ch) = -1000;
               else
                  GET_ALIGNMENT(ch) = (GET_ALIGNMENT(ch) - 1);
            }

            SET_BIT(obj->value[0], DOOMY_SPAZ);
            AddEvent(EVENT_OBJ_TIMER, (PULSE_HOUR / 3), TRUE, obj, DoombringerZ);
         }
      }
      return FALSE;
   }

   return FALSE;
}

#endif

/* EoF */
