/**************************************************************************
 *  File: fight.c                                      Part of LuminariMUD *
 *  Usage: Combat system.                                                  *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#define __FIGHT_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "shop.h"
#include "quest.h"
#include "mud_event.h"
#include "spec_procs.h"
#include "clan.h"
#include "treasure.h"
#include "mudlim.h"
#include "spec_abilities.h"
#include "feats.h"
#include "actions.h"
#include "actionqueues.h"
#include "craft.h"
#include "assign_wpn_armor.h"
#include "grapple.h"
#include "alchemy.h"
#include "missions.h"
#include "hunts.h"
#include "domains_schools.h"
#include "staff_events.h" /* for staff events!  prisoner no xp penalty! */

/* toggle for debug mode
   true = annoying messages used for debugging
   false = normal gameplay */
#define DEBUGMODE FALSE

/* return results from hit() */
#define HIT_MISS 0
#define HIT_RESULT_ACTION -1
#define HIT_NEED_RELOAD -2
/* vnum of special mobile:  the prisoner */
#define THE_PRISONER 113750
#define DRACOLICH_PRISONER 113751

/* local global */
struct obj_data *last_missile = NULL;

/* head of l-list of fighting chars */
struct char_data *combat_list = NULL;

// external functions
void save_char_pets(struct char_data *ch);
bool is_using_keen_weapon(struct char_data *ch);
int hands_used(struct char_data *ch);

/* Weapon attack texts
 * don't forget to add to constants.c attack_hit_types */
struct attack_hit_type attack_hit_text[] = {
    {"hit", "hits"}, /* 0 */
    {"sting", "stings"},
    {"whip", "whips"},
    {"slash", "slashes"},
    {"bite", "bites"},
    {"bludgeon", "bludgeons"}, /* 5 */
    {"crush", "crushes"},
    {"pound", "pounds"},
    {"claw", "claws"},
    {"maul", "mauls"},
    {"thrash", "thrashes"}, /* 10 */
    {"pierce", "pierces"},
    {"blast", "blasts"},
    {"punch", "punches"},
    {"stab", "stabs"},
    {"slice", "slices"}, /* 15 */
    {"thrust", "thrusts"},
    {"hack", "hacks"},
    {"rake", "rakes"},
    {"peck", "pecks"},
    {"smash", "smashes"}, /* 20 */
    {"trample", "tramples"},
    {"charge", "charges"},
    {"gore", "gores"}};

/* currently unused */
#define NUM_ATTACK_DAMAGE_TYPE_TEXT 10
struct attack_hit_type attack_damage_type_text[NUM_ATTACK_DAMAGE_TYPE_TEXT] = {
    /* DAMAGE_TYPE_BLUDGEONING */
    {"bludgeon", "bludgeons"},
    {"pound", "pounds"},
    {"crush", "crushes"},

    /* DAMAGE_TYPE_SLASHING */
    {"slash", "slashes"},
    {"slice", "slices"},

    /* DAMAGE_TYPE_PIERCING */
    {"pierce", "pierces"},
    {"stab", "stabs"},

    /* unarmed, non-lethal */
    {"punch", "punches"},
    {"knee", "knees"},
    {"elbow", "elbows"},

};

/* local (file scope only) variables */
static struct char_data *next_combat_list = NULL;

/* local file scope utility functions */
struct obj_data *get_wielded(struct char_data *ch, int attack_type);
static void perform_group_gain(struct char_data *ch, int base,
                               struct char_data *victim);
static void dam_message(int dam, struct char_data *ch, struct char_data *victim,
                        int w_type, int offhand);
static void make_corpse(struct char_data *ch);
static void change_alignment(struct char_data *ch, struct char_data *victim);
static void group_gain(struct char_data *ch, struct char_data *victim);
static void solo_gain(struct char_data *ch, struct char_data *victim);
/** @todo refactor this function name */
static char *replace_string(const char *str, const char *weapon_singular,
                            const char *weapon_plural);

#define IS_WEAPON(type) (((type) >= TOP_ATTACK_TYPES) && ((type) < BOT_WEAPON_TYPES))

/************ utility functions *********************/

/* simple utility function to check if ch is tanking */
bool is_tanking(struct char_data *ch)
{
  struct char_data *vict;
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
  {
    if (FIGHTING(vict) == ch)
      return TRUE;
  }

  return FALSE;
}

/* code to check if vict is going to be auto-rescued by someone while
 being attacked by ch */
void guard_check(struct char_data *ch, struct char_data *vict)
{
  struct char_data *tch;
  struct char_data *next_tch;

  if (!ch || !vict)
    return;

  if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE))
    return;

  for (tch = world[ch->in_room].people; tch; tch = next_tch)
  {
    next_tch = tch->next_in_room;
    if (!tch)
      continue;
    if (tch == ch || tch == vict)
      continue;
    if (IS_NPC(tch))
      continue;
    if (GET_POS(tch) < POS_FIGHTING)
      continue;
    if (AFF_FLAGGED(tch, AFF_BLIND))
      continue;
    /*  Require full round action availability to guard.  */
    if (!is_action_available(tch, atSTANDARD, FALSE) ||
        !is_action_available(tch, atMOVE, FALSE))
      continue;

    /* vict = guarded individual, tch = guard */
    if (GUARDING(tch) == vict)
    {
      /* This MUST be changed.  Skills are obsolete.
         Set to a flat 70% chance for now. */
      if (rand_number(1, 100) > 70)
      {
        GUI_CMBT_OPEN(vict);
        act("$N fails to guard you.", FALSE, vict, 0, tch, TO_CHAR);
        GUI_CMBT_CLOSE(vict);

        GUI_CMBT_OPEN(tch);
        act("You fail to guard $n.", FALSE, vict, 0, tch, TO_VICT);
        GUI_CMBT_CLOSE(tch);
      }
      else
      {
        GUI_CMBT_OPEN(vict);
        act("$N protects you from attack!", FALSE, vict, 0, tch, TO_CHAR);
        GUI_CMBT_CLOSE(vict);

        GUI_CMBT_NOTVICT_OPEN(vict, tch);
        act("$N guards $n succesfully.", FALSE, vict, 0, tch, TO_NOTVICT);
        GUI_CMBT_NOTVICT_CLOSE(vict, tch);

        GUI_CMBT_OPEN(tch);
        act("You guard $n succesfully.", FALSE, vict, 0, tch, TO_VICT);
        GUI_CMBT_CLOSE(tch);

        perform_rescue(tch, vict);
        return;
      }
    }
  }
}

/* rewritten subfunction
   the engine for fleeing */
void perform_flee(struct char_data *ch)
{
  int i, found = 0, fleeOptions[DIR_COUNT];
  struct char_data *was_fighting, *k, *temp;

  /* disqualifications? */
  if (AFF_FLAGGED(ch, AFF_STUN) || AFF_FLAGGED(ch, AFF_DAZED) ||
      AFF_FLAGGED(ch, AFF_PARALYZED) || char_has_mud_event(ch, eSTUNNED))
  {
    GUI_CMBT_OPEN(ch);
    send_to_char(ch, "You try to flee, but you are unable to move!\r\n");
    GUI_CMBT_CLOSE(ch);

    GUI_CMBT_NOTVICT_OPEN(ch, NULL);
    act("$n attemps to flee, but is unable to move!", TRUE, ch, 0, 0, TO_ROOM);
    GUI_CMBT_NOTVICT_CLOSE(ch, NULL);
    return;
  }

  /* got to be in a position to flee */
  if (GET_POS(ch) <= POS_SITTING)
  {
    GUI_CMBT_OPEN(ch);
    send_to_char(ch, "You need to be standing to flee!\r\n");
    GUI_CMBT_CLOSE(ch);
    return;
  }

  if (!is_action_available(ch, atMOVE, TRUE))
  {
    GUI_CMBT_OPEN(ch);
    send_to_char(ch, "You need a move action to flee!\r\n");
    GUI_CMBT_CLOSE(ch);
    return;
  }

  // first find which directions are fleeable
  for (i = 0; i < DIR_COUNT; i++)
  {
    if (CAN_GO(ch, i))
    {
      fleeOptions[found] = i;
      found++;
    }
  }

  // no actual fleeable directions
  if (!found)
  {
    GUI_CMBT_OPEN(ch);
    send_to_char(ch, "You have no route of escape!\r\n");
    GUI_CMBT_CLOSE(ch);
    return;
  }

  /* cost */
  USE_MOVE_ACTION(ch);

  // not fighting?  no problems
  if (!FIGHTING(ch))
  {
    send_to_char(ch, "You quickly flee the area...\r\n");
    act("$n quickly flees the area!", TRUE, ch, 0, 0, TO_ROOM);

    // pick a random direction
    do_simple_move(ch, fleeOptions[rand_number(0, found - 1)], 3);
  }

  else
  {

    GUI_CMBT_OPEN(ch);
    send_to_char(ch, "You attempt to flee:  ");
    GUI_CMBT_CLOSE(ch);

    GUI_CMBT_NOTVICT_OPEN(ch, NULL);
    act("$n attemps to flee...", TRUE, ch, 0, 0, TO_ROOM);
    GUI_CMBT_NOTVICT_CLOSE(ch, NULL);

    // ok beat all odds, fleeing
    was_fighting = FIGHTING(ch);

    // pick a random direction
    if (do_simple_move(ch, fleeOptions[rand_number(0, found - 1)], 3))
    {
      GUI_CMBT_OPEN(ch);
      send_to_char(ch, "You quickly flee from combat...\r\n");
      GUI_CMBT_CLOSE(ch);

      GUI_CMBT_NOTVICT_OPEN(ch, NULL);
      act("$n quickly flees the battle!", TRUE, ch, 0, 0, TO_ROOM);
      GUI_CMBT_NOTVICT_CLOSE(ch, NULL);

      /* lets stop combat here, further consideration might be to continue
         ranged combat -zusuk */
      /* fleer */
      if (FIGHTING(ch))
        stop_fighting(ch);
      /* fighting fleer */
      for (k = combat_list; k; k = temp)
      {
        temp = k->next_fighting;
        if (FIGHTING(k) == ch)
          stop_fighting(k);
      }
    }
    else
    { // failure
      GUI_CMBT_OPEN(ch);
      send_to_char(ch, "You failed to flee the battle...\r\n");
      GUI_CMBT_CLOSE(ch);

      GUI_CMBT_NOTVICT_OPEN(ch, NULL);
      act("$n failed to flee the battle!", TRUE, ch, 0, 0, TO_ROOM);
      GUI_CMBT_NOTVICT_CLOSE(ch, NULL);
    }
  }
}

/* a function for removing sneak/hide/invisibility on a ch
   the forced variable is just used for greater invis */
void appear(struct char_data *ch, bool forced)
{

  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  if (AFF_FLAGGED(ch, AFF_SNEAK))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
    send_to_char(ch, "You stop sneaking...\r\n");
    act("$n stops moving silently...", FALSE, ch, 0, 0, TO_ROOM);
  }

  if (AFF_FLAGGED(ch, AFF_HIDE))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    send_to_char(ch, "You step out of the shadows...\r\n");
    act("$n steps out of the shadows...", FALSE, ch, 0, 0, TO_ROOM);
  }

  // this is a hack, so order in this function is important
  if (affected_by_spell(ch, SPELL_GREATER_INVIS))
  {
    if (forced)
    {
      affect_from_char(ch, SPELL_GREATER_INVIS);
      if (AFF_FLAGGED(ch, AFF_INVISIBLE))
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
      send_to_char(ch, "You snap into visibility...\r\n");
      act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
    }
    else
      return;
  }

  if (affected_by_spell(ch, PSIONIC_SHADOW_BODY))
  {
    if (forced)
    {
      affect_from_char(ch, SPELL_INVISIBLE);
      if (AFF_FLAGGED(ch, AFF_INVISIBLE))
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
      send_to_char(ch, "You snap into visibility...\r\n");
      act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
    }
    else
      return;
  }

  /* this has to come after greater_invis */
  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
    send_to_char(ch, "You snap into visibility...\r\n");
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  }
}

/*  has_dex_bonus_to_ac(attacker, ch)
 *  Helper function to determine if a char can apply his dexterity bonus to his AC. */
bool has_dex_bonus_to_ac(struct char_data *attacker, struct char_data *ch)
{
  if (!ch)
    return TRUE; // if (!attacker) return TRUE;

  /* conditions for losing dex to ch */

  /* ch is sleeping */
  if (!AWAKE(ch))
  {

    if (DEBUGMODE)
    {
      if (FIGHTING(ch))
      {
        send_to_char(ch, "has_dex_bonus_to_ac() - %s not awake  ", GET_NAME(ch));
        if (attacker)
          send_to_char(attacker, "has_dex_bonus_to_ac() - %s not awake  ", GET_NAME(ch));
      }
    }
    return FALSE;
  }

  if (AFF_FLAGGED(ch, AFF_PINNED))
  {
    if (DEBUGMODE)
    {
      if (FIGHTING(ch))
      {
        send_to_char(ch, "has_dex_bonus_to_ac() - %s pinned  ", GET_NAME(ch));
        if (attacker)
          send_to_char(attacker, "has_dex_bonus_to_ac() - %s pinned  ", GET_NAME(ch));
      }
    }
    return FALSE;
  }

  /* ch unable to see attacker WITHOUT blind-fighting feat */
  if (attacker)
  {
    if (!CAN_SEE(ch, attacker) && !HAS_FEAT(ch, FEAT_BLIND_FIGHT))
    {
      if (DEBUGMODE)
      {
        if (FIGHTING(ch))
        {
          send_to_char(ch, "has_dex_bonus_to_ac() - %s unable to see attacker  ", GET_NAME(ch));
          send_to_char(attacker, "has_dex_bonus_to_ac() - %s unable to see attacker  ", GET_NAME(ch));
        }
      }
      return FALSE;
    }
  }

  /* ch is flat-footed WITHOUT uncanny dodge feat */
  if ((AFF_FLAGGED(ch, AFF_FLAT_FOOTED) && !HAS_FEAT(ch, FEAT_UNCANNY_DODGE)))
  {
    if (DEBUGMODE)
    {
      if (FIGHTING(ch))
      {
        send_to_char(ch, "has_dex_bonus_to_ac() - %s flat-footed  ", GET_NAME(ch));
        if (attacker)
          send_to_char(attacker, "has_dex_bonus_to_ac() - %s flat-footed  ", GET_NAME(ch));
      }
    }
    return FALSE;
  }

  /* ch is stunned */
  if (AFF_FLAGGED(ch, AFF_STUN) || char_has_mud_event(ch, eSTUNNED))
  {
    if (DEBUGMODE)
    {
      if (FIGHTING(ch))
      {
        send_to_char(ch, "has_dex_bonus_to_ac() - %s stunned  ", GET_NAME(ch));
        if (attacker)
          send_to_char(attacker, "has_dex_bonus_to_ac() - %s stunned  ", GET_NAME(ch));
      }
    }
    return FALSE;
  }

  /* ch is paralyzed */
  if (AFF_FLAGGED(ch, AFF_PARALYZED))
  {
    if (DEBUGMODE)
    {
      if (FIGHTING(ch))
      {
        send_to_char(ch, "has_dex_bonus_to_ac() - %s paralyzed  ", GET_NAME(ch));
        if (attacker)
          send_to_char(attacker, "has_dex_bonus_to_ac() - %s paralyzed  ", GET_NAME(ch));
      }
    }
    return FALSE;
  }

  /* ch is feinted */
  if (AFF_FLAGGED(ch, AFF_FEINTED) || affected_by_spell(ch, SKILL_FEINT))
  {
    if (DEBUGMODE)
    {
      if (FIGHTING(ch))
      {
        send_to_char(ch, "has_dex_bonus_to_ac() - %s feinted  ", GET_NAME(ch));
        if (attacker)
          send_to_char(attacker, "has_dex_bonus_to_ac() - %s feinted  ", GET_NAME(ch));
      }
    }
    return FALSE;
  }

  if (DEBUGMODE)
  {
    if (FIGHTING(ch))
    {
      send_to_char(ch, "has_dex_bonus_to_ac() - %s -retained- dex bonus  ", GET_NAME(ch));
      if (attacker)
        send_to_char(attacker, "has_dex_bonus_to_ac() - %s -retained- dex bonus  ", GET_NAME(ch));
    }
  }
  return TRUE; /* ok, made it through, we DO have our dex bonus still */
}

/* our definition of flanking right now simply means the victim (ch's) target
   is not the attacker */
bool is_flanked(struct char_data *attacker, struct char_data *ch)
{

  /* some instant disqualifiers */
  if (!attacker)
    return FALSE;
  if (!ch)
    return FALSE;

  if (affected_by_spell(ch, PSIONIC_UBIQUITUS_VISION))
    return FALSE;

  if (affected_by_spell(ch, SPELL_SHIELD_OF_FORTIFICATION) && dice(1, 4) == 1)
    return FALSE;

  /* most common scenario */
  if (FIGHTING(ch) && (FIGHTING(ch) != attacker) && !HAS_FEAT(ch, FEAT_IMPROVED_UNCANNY_DODGE))
    return TRUE;

  /* ok so ch is fighting AND it is not the attacker tanking, by default
   * this is flanked, but we have to check for uncanny dodge */
  if (FIGHTING(ch) && (FIGHTING(ch) != attacker) && HAS_FEAT(ch, FEAT_IMPROVED_UNCANNY_DODGE))
  {

    int attacker_level = CLASS_LEVEL(attacker, CLASS_BERSERKER) +
                         CLASS_LEVEL(attacker, CLASS_ROGUE);
    int ch_level = CLASS_LEVEL(ch, CLASS_BERSERKER) +
                   CLASS_LEVEL(ch, CLASS_ROGUE);

    /* 4 or more levels of berserker or rogue will trump imp. uncanny dodge*/
    if (attacker_level >= (ch_level + 4))
      return TRUE;

    return FALSE; /* uncanny dodge WILL help */
  }

  return FALSE; /* default */
}

int roll_initiative(struct char_data *ch)
{
  int initiative = 0;

  initiative = d20(ch) + GET_DEX_BONUS(ch) + 4 * HAS_FEAT(ch, FEAT_IMPROVED_INITIATIVE);
  initiative += 2 * HAS_FEAT(ch, FEAT_IMPROVED_REACTION);
  initiative += GET_WIS_BONUS(ch) * HAS_FEAT(ch, FEAT_CUNNING_INITIATIVE);
  // initiative += HAS_FEAT(ch, FEAT_HEROIC_INITIATIVE);

  return initiative;
}

/* this function will go through all the tests for modifying
 * a given ch's AC under the circumstances of being attacked
 * by 'attacker' (protection from alignment (evil/good), has dexterity bonus,
 * favored enemy) */
int compute_armor_class(struct char_data *attacker, struct char_data *ch,
                        int is_touch, int mode)
{
  /* hack to translate old D&D to 3.5 Edition
   * Modified 09/09/2014 : Ornir
   * Changed this to use the AC as-is.  AC has been modified on gear. */
  int armorclass = 0, eq_armoring = 0, temp = GET_AC(ch),
      ac_penalty = 0; /* we keep track of all AC penalties */
  int i = 0, bonuses[NUM_BONUS_TYPES];
  struct affected_type *affections = NULL, *next = NULL;

  /* Initialize bonuse-types to 0 */
  for (i = 0; i < NUM_BONUS_TYPES; i++)
    bonuses[i] = 0;

  /* base AC */
  armorclass = 10;

  /* REMINDER:  We are still working with a 100 AC stock system with this
   beginning chunk of code */

  /* philosophical question, what is GET_AC()?
     So unless I am missing something, GET_AC() will include ALL your worn
     gear and all the bonuses you have via spells (affections structures).

     So we have to extract these aff values separately and make sure they are applying
     respective bonuses to the right bonus-types
   *note:  base armor class of stock code system is a system of 100 vs 10 of pathfinder
   */
  /* increment through all the affections on the character, check for matches */
  for (affections = ch->affected; affections; affections = next)
  {
    next = affections->next;

    if (affections->location == APPLY_AC_NEW)
    {
      bonuses[affections->bonus_type] += affections->modifier;
      /* temp is just our GET_AC(), so remember 10 factor */
      temp -= (affections->modifier * 10);
    }
    /* divide by 10 since old AC system */
    if (affections->location == APPLY_AC)
    {
      bonuses[affections->bonus_type] += (affections->modifier / 10);
      temp -= affections->modifier;
    }
  }

  /* now that affections have been extracted from AC, all that is left is
     armoring (helm, body, leggings, sleeves) and shield */

  /* under certain circumstances, like using your shield to attack, you will
     lose your AC bonus from your shield while recovering from that attack */
  if (GET_EQ(ch, WEAR_SHIELD))
  {
    temp -= apply_ac(ch, WEAR_SHIELD); /*factor 10*/
    /* recovering from shield use in attack, don't add shield bonus! */
    if (char_has_mud_event(ch, eSHIELD_RECOVERY))
    {
    }
    else
    { /* okay to add shield bonus to shield, remember factor 10 */
      bonuses[BONUS_TYPE_SHIELD] += (apply_ac(ch, WEAR_SHIELD) / 10);
    }
    if (teamwork_using_shield(ch, FEAT_SHIELD_WALL))
      bonuses[BONUS_TYPE_SHIELD] += teamwork_using_shield(ch, FEAT_SHIELD_WALL);
  }

  /* that should be it, just base armoring should be left, assign away! */
  eq_armoring = ((temp - 100) / 10);

  /* END REMINDER: From now on, we should be finished working with the stock
     100 AC system of stock code */

  /* here is our TODO list:
     1)  handling armor-affecting spells?
     2)  calculate equipped armor separately?
   */

  /**********/
  /* bonus types */

  /* bonus type racial */
  if (GET_RACE(ch) == RACE_ARCANA_GOLEM)
  {
    bonuses[BONUS_TYPE_RACIAL] -= 2;
    ac_penalty -= 2;
  }
  /**/

  /* bonus type natural-armor */
  /* arcana golem */
  if (char_has_mud_event(ch, eSPELLBATTLE) && SPELLBATTLE(ch) > 0)
  {
    bonuses[BONUS_TYPE_NATURALARMOR] += SPELLBATTLE(ch);
  }
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_ARMOR_SKIN))
  {
    bonuses[BONUS_TYPE_NATURALARMOR] += HAS_FEAT(ch, FEAT_ARMOR_SKIN);
  }
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_DRACONIC_HERITAGE_DRAGON_RESISTANCES))
  {
    bonuses[BONUS_TYPE_NATURALARMOR] += (CLASS_LEVEL(ch, CLASS_SORCERER) >= 15 ? 4 : (CLASS_LEVEL(ch, CLASS_SORCERER) >= 9 ? 2 : 1));
  }
  // vampires
  if (HAS_FEAT(ch, FEAT_VAMPIRE_NATURAL_ARMOR) && CAN_USE_VAMPIRE_ABILITY(ch))
  {
    bonuses[BONUS_TYPE_NATURALARMOR] += 6;
  }
  /**/

  /* bonus type armor */
  /* This is our equipped gear */
  if (!IS_WILDSHAPED(ch) || IS_MORPHED(ch))
    bonuses[BONUS_TYPE_ARMOR] += eq_armoring;

  /* ...Trelux carapace is not effective vs touch attacks! */
  if (GET_RACE(ch) == RACE_TRELUX)
  {
    bonuses[BONUS_TYPE_ARMOR] += GET_LEVEL(ch) / 3;
  }
  /**/

  /* bonus type shield (usually equipment) */
  if (!is_touch && !IS_NPC(ch) && GET_EQ(ch, WEAR_SHIELD) &&
      GET_SKILL(ch, SKILL_SHIELD_SPECIALIST))
  {
    bonuses[BONUS_TYPE_SHIELD] += 2;
  }
  if (!is_touch && !IS_NPC(ch) && GET_EQ(ch, WEAR_SHIELD) &&
      HAS_FEAT(ch, FEAT_ARMOR_MASTERY_2))
  {
    bonuses[BONUS_TYPE_SHIELD] += 2;
  }
  /**/

  /* bonus type deflection */
  /* two weapon defense */
  if (!IS_NPC(ch) && is_using_double_weapon(ch) && HAS_FEAT(ch, FEAT_TWO_WEAPON_DEFENSE))
  {
    bonuses[BONUS_TYPE_SHIELD]++;
  }
  else if (!IS_NPC(ch) && GET_EQ(ch, WEAR_WIELD_OFFHAND) && HAS_FEAT(ch, FEAT_TWO_WEAPON_DEFENSE))
  {
    bonuses[BONUS_TYPE_SHIELD]++;
  }

  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_WEAPON_MASTERY) &&
      (GET_EQ(ch, WEAR_WIELD_1) || GET_EQ(ch, WEAR_WIELD_OFFHAND || GET_EQ(ch, WEAR_WIELD_2H))))
  {
    bonuses[BONUS_TYPE_DEFLECTION] += 2;
  }

  if (AFF_FLAGGED(ch, AFF_TOTAL_DEFENSE) && HAS_FEAT(ch, FEAT_ELABORATE_PARRY))
  {
    bonuses[BONUS_TYPE_DEFLECTION] += CLASS_LEVEL(ch, CLASS_DUELIST) / 2;
  }

  if (attacker)
  {
    if (AFF_FLAGGED(ch, AFF_PROTECT_GOOD) && IS_GOOD(attacker))
    {
      bonuses[BONUS_TYPE_DEFLECTION] += 2;
    }
    if (AFF_FLAGGED(ch, AFF_PROTECT_EVIL) && IS_EVIL(attacker))
    {
      bonuses[BONUS_TYPE_DEFLECTION] += 2;
    }
  }
  /**/

  /* bonus type enhancement (equipment) */

  // This code is replaced by the next portion of code that goes over each armor piece.
  /* bonuses[BONUS_TYPE_ENHANCEMENT] += compute_gear_enhancement_bonus(ch); */

  int ac_bonus = 0;
  struct obj_data *ac_piece = NULL;

  if ((ac_piece = GET_EQ(ch, WEAR_BODY)) != NULL)
  {
    switch (GET_OBJ_MATERIAL(ac_piece))
    {
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_DRAGONHIDE:
    case MATERIAL_DRAGONSCALE:
    case MATERIAL_DRAGONBONE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DARKWOOD:
      ac_bonus++;
      break;
    }
    ac_bonus += MAX(GET_OBJ_VAL(ac_piece, 4), get_char_affect_modifier(ch, SPELL_MAGIC_VESTMENT, APPLY_SPECIAL));
  }
  if ((ac_piece = GET_EQ(ch, WEAR_HEAD)) != NULL)
  {
    switch (GET_OBJ_MATERIAL(ac_piece))
    {
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_DRAGONHIDE:
    case MATERIAL_DRAGONSCALE:
    case MATERIAL_DRAGONBONE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DARKWOOD:
      ac_bonus++;
      break;
    }
    ac_bonus += MAX(GET_OBJ_VAL(ac_piece, 4), get_char_affect_modifier(ch, SPELL_MAGIC_VESTMENT, APPLY_SPECIAL));
  }
  if ((ac_piece = GET_EQ(ch, WEAR_ARMS)) != NULL)
  {
    switch (GET_OBJ_MATERIAL(ac_piece))
    {
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_DRAGONHIDE:
    case MATERIAL_DRAGONSCALE:
    case MATERIAL_DRAGONBONE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DARKWOOD:
      ac_bonus++;
      break;
    }
    ac_bonus += MAX(GET_OBJ_VAL(ac_piece, 4), get_char_affect_modifier(ch, SPELL_MAGIC_VESTMENT, APPLY_SPECIAL));
  }
  if ((ac_piece = GET_EQ(ch, WEAR_LEGS)) != NULL)
  {
    switch (GET_OBJ_MATERIAL(ac_piece))
    {
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_DRAGONHIDE:
    case MATERIAL_DRAGONSCALE:
    case MATERIAL_DRAGONBONE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DARKWOOD:
      ac_bonus++;
      break;
    }
    ac_bonus += MAX(GET_OBJ_VAL(ac_piece, 4), get_char_affect_modifier(ch, SPELL_MAGIC_VESTMENT, APPLY_SPECIAL));
  }

  // important! We're dividing the total bonuses from body, head, arms and legs by 4.  Then we add shield at the end.
  ac_bonus /= 4;

  if ((ac_piece = GET_EQ(ch, WEAR_SHIELD)) != NULL)
  {
    switch (GET_OBJ_MATERIAL(ac_piece))
    {
    case MATERIAL_ADAMANTINE:
    case MATERIAL_MITHRIL:
    case MATERIAL_DRAGONHIDE:
    case MATERIAL_DRAGONSCALE:
    case MATERIAL_DRAGONBONE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DARKWOOD:
      ac_bonus++;
      break;
    }
    ac_bonus += MAX(GET_OBJ_VAL(ac_piece, 4), get_char_affect_modifier(ch, SPELL_MAGIC_VESTMENT, APPLY_SPECIAL));
  }

  bonuses[BONUS_TYPE_ENHANCEMENT] = MAX(0, ac_bonus);

  // End of new armor piece code
  // replaces bonuses[BONUS_TYPE_ENHANCEMENT] += compute_gear_enhancement_bonus(ch); above

  bonuses[BONUS_TYPE_ENHANCEMENT] += get_defending_weapon_bonus(ch, false);
  /**/

  /* bonus type dodge */
  /* Determine if the ch loses their dex bonus to armor class. */
  if (has_dex_bonus_to_ac(attacker, ch))
  {

    /* this will include a dex-cap bonus on equipment as well */
    bonuses[BONUS_TYPE_DODGE] += GET_DEX_BONUS(ch);

    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_LUCK_OF_HEROES))
    {
      bonuses[BONUS_TYPE_DODGE] += 1;
    }

    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_DODGE))
    {
      bonuses[BONUS_TYPE_DODGE] += 1;
    }

    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_LVL_AC_BONUS))
    {
      bonuses[BONUS_TYPE_DODGE] += 1 + ((CLASS_LEVEL(ch, CLASS_MONK) + CLASS_LEVEL(ch, CLASS_SACRED_FIST)) / 4);
    }

    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_AC_BONUS))
    {
      bonuses[BONUS_TYPE_DODGE] += HAS_FEAT(ch, FEAT_AC_BONUS);
    }

    /* acrobatics offers no benefit if you are wearing heavier than light-armor */
    if (!IS_NPC(ch) && GET_ABILITY(ch, ABILITY_ACROBATICS) &&
        compute_gear_armor_type(ch) <= ARMOR_TYPE_LIGHT)
    { // caps at 5
      bonuses[BONUS_TYPE_DODGE] += MIN(5, (int)(compute_ability(ch, ABILITY_ACROBATICS) / 7));
    }

    /* this feat requires light armor and no shield */
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_CANNY_DEFENSE) && HAS_FREE_HAND(ch) &&
        compute_gear_armor_type(ch) <= ARMOR_TYPE_LIGHT)
    {
      bonuses[BONUS_TYPE_DODGE] += MAX(0, GET_INT_BONUS(ch));
    }

    if (AFF_FLAGGED(ch, AFF_EXPERTISE) && !IS_CASTING(ch))
    {
      bonuses[BONUS_TYPE_DODGE] += COMBAT_MODE_VALUE(ch);
    }
  }
  /**/

  /* bonus type circumstance */

  if (has_teamwork_feat(ch, FEAT_BACK_TO_BACK) && is_flanked(attacker, ch))
    bonuses[BONUS_TYPE_CIRCUMSTANCE] = MAX(bonuses[BONUS_TYPE_CIRCUMSTANCE], 2);

  switch (GET_POS(ch))
  { // position penalty
  case POS_RECLINING:
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 3;
    ac_penalty -= 3;
    break;
  case POS_SITTING:
  case POS_RESTING:
  case POS_STUNNED:
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 2;
    ac_penalty -= 2;
    break;
  case POS_SLEEPING:
  case POS_INCAP:
  case POS_MORTALLYW:
  case POS_DEAD:
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 20;
    ac_penalty -= 20;
    break;
  case POS_FIGHTING:
  case POS_STANDING:
  default:
    break;
  }

  if (char_has_mud_event(ch, eSTUNNED))
  { /* POS_STUNNED below in case statement */
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 2;
    ac_penalty -= 2;
  }

  if (AFF_FLAGGED(ch, AFF_FATIGUED))
  {
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 2;
    ac_penalty -= 2;
  }

  /* current implementation of taunt */
  if (char_has_mud_event(ch, eTAUNTED))
  {
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 6;
    ac_penalty -= 6;
  }
  /**/

  /* bonus type size (should not stack) */
  bonuses[BONUS_TYPE_SIZE] += size_modifiers_inverse[GET_SIZE(ch)];
  if (attacker)
  { /* racial bonus vs. larger opponents */
    if ((GET_RACE(ch) == RACE_DWARF ||
         GET_RACE(ch) == RACE_CRYSTAL_DWARF ||
         GET_RACE(ch) == RACE_GNOME ||
         GET_RACE(ch) == RACE_DUERGAR ||
         GET_RACE(ch) == RACE_HALFLING) &&
        GET_SIZE(attacker) > GET_SIZE(ch))
    {
      bonuses[BONUS_TYPE_SIZE] += 4;
    }
  }
  if (bonuses[BONUS_TYPE_SIZE] < 0)
    ac_penalty -= bonuses[BONUS_TYPE_SIZE];
  /**/

  /* bonus type undefined */
  /* stalwart warrior - warrior feat */
  if (HAS_FEAT(ch, FEAT_STALWART_WARRIOR))
  {
    bonuses[BONUS_TYPE_ARMOR] += (CLASS_LEVEL(ch, CLASS_WARRIOR) / 4);
  }
  /* favored enemy */
  if (attacker && attacker != ch && !IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_RANGER))
  {
    // checking if we have humanoid favored enemies for PC victims
    if (!IS_NPC(attacker) && IS_FAV_ENEMY_OF(ch, RACE_TYPE_HUMANOID))
    {
      bonuses[BONUS_TYPE_MORALE] += CLASS_LEVEL(ch, CLASS_RANGER) / 5 + 2;
    }
    else if (IS_NPC(attacker) && IS_FAV_ENEMY_OF(ch, GET_RACE(attacker)))
    {
      bonuses[BONUS_TYPE_MORALE] += CLASS_LEVEL(ch, CLASS_RANGER) / 5 + 2;
    }
  }
  /* These bonuses to AC apply even against touch attacks or when the monk is
   * flat-footed. She loses these bonuses when she is immobilized or helpless,
   * when she wears any armor, when she carries a shield, or when she carries
   * a medium or heavy load. */
  if (MONK_TYPE(ch) && monk_gear_ok(ch))
  {
    bonuses[BONUS_TYPE_DODGE] += GET_WIS_BONUS(ch);

    if (MONK_TYPE(ch) >= 4)
    {
      bonuses[BONUS_TYPE_DODGE]++;
    }
    if (MONK_TYPE(ch) >= 8)
    {
      bonuses[BONUS_TYPE_DODGE]++;
    }
    if (MONK_TYPE(ch) >= 12)
    {
      bonuses[BONUS_TYPE_DODGE]++;
    }
    if (MONK_TYPE(ch) >= 16)
    {
      bonuses[BONUS_TYPE_DODGE]++;
    }
    if (MONK_TYPE(ch) >= 20)
    {
      bonuses[BONUS_TYPE_DODGE]++;
    }
    if (MONK_TYPE(ch) >= 24)
    {
      bonuses[BONUS_TYPE_DODGE]++;
    }
    if (MONK_TYPE(ch) >= 28)
    {
      bonuses[BONUS_TYPE_DODGE]++;
    }
  }
  /**/

  if (attacker && has_teamwork_feat(ch, FEAT_DUCK_AND_COVER) && teamwork_using_shield(ch, FEAT_DUCK_AND_COVER) &&
      is_using_ranged_weapon(attacker, TRUE))
    bonuses[BONUS_TYPE_INSIGHT] += 2;

  if (has_teamwork_feat(ch, FEAT_PHALANX_FIGHTER) && attacker)
  {
    if (IS_EVIL(ch) && !IS_EVIL(attacker))
      bonuses[BONUS_TYPE_PROFANE] = MAX(bonuses[BONUS_TYPE_PROFANE], has_teamwork_feat(ch, FEAT_PHALANX_FIGHTER));
    else if (!IS_EVIL(ch) && IS_EVIL(attacker))
      bonuses[BONUS_TYPE_SACRED] = MAX(bonuses[BONUS_TYPE_SACRED], has_teamwork_feat(ch, FEAT_PHALANX_FIGHTER));
  }

  if (is_judgement_possible(ch, attacker, INQ_JUDGEMENT_PROTECTION))
    bonuses[judgement_bonus_type(ch)] = MAX(bonuses[judgement_bonus_type(ch)], get_judgement_bonus(ch, INQ_JUDGEMENT_PROTECTION));

  // Sacred Bonus
  if (attacker && IS_UNDEAD(attacker) && affected_by_spell(ch, SPELL_VEIL_OF_POSITIVE_ENERGY))
    bonuses[BONUS_TYPE_SACRED] = MAX(bonuses[BONUS_TYPE_SACRED], 2);

  /* Add up all the bonuses */
  if (is_touch)
  { /* don't include armor, natural armor, shield */
    for (i = 0; i < NUM_BONUS_TYPES; i++)
    {
      if (i == BONUS_TYPE_NATURALARMOR)
        continue;
      else if (i == BONUS_TYPE_ARMOR)
        continue;
      else if (i == BONUS_TYPE_SHIELD)
        continue;
      else
        armorclass += bonuses[i];
    }
  }

  switch (mode)
  {
  case MODE_ARMOR_CLASS_PENALTIES:
    return ac_penalty;
    break;
  case MODE_ARMOR_CLASS_DISPLAY:

    send_to_char(ch, "Base Armor Class: 10\r\n");

    for (i = 0; i < NUM_BONUS_TYPES; i++)
    {
      if (bonuses[i])
        send_to_char(ch, "%-16s: %d\r\n", bonus_types[i], bonuses[i]);
      armorclass += bonuses[i];
    }

    send_to_char(ch, "%-16s: %d\r\n", "TOTAL", armorclass);
    break;
  case MODE_ARMOR_CLASS_COMBAT_MANEUVER_DEFENSE:
  case MODE_ARMOR_CLASS_NORMAL:
  default:
    if (!is_touch)
      for (i = 0; i < NUM_BONUS_TYPES; i++)
        armorclass += bonuses[i];
    break;
  }

  /* value for normal mode */
  return (MIN(CONFIG_PLAYER_AC_CAP, armorclass));
}

// the whole update_pos system probably needs to be rethought -zusuk

void update_pos_dam(struct char_data *victim)
{

  if (HAS_FEAT(victim, FEAT_DEATHLESS_FRENZY) && affected_by_spell(victim, SKILL_RAGE))
  {
    if (GET_HIT(victim) <= -121)
      change_position(victim, POS_DEAD);
    else
      return;
  }

  if (GET_HIT(victim) <= -11)
  {
    if (HAS_REAL_FEAT(victim, FEAT_DIEHARD) && dice(1, 3) == 1)
    {
      act("\tYYour die hard toughness let's you push through.\tn", FALSE, victim, 0, 0, TO_CHAR);
      act("$n's die hard toughness let's $m push through.", FALSE, victim, 0, 0, TO_ROOM);
      GET_HIT(victim) = 1;
    }
    else
    {
      change_position(victim, POS_DEAD);
    }
  }
  else if (GET_HIT(victim) <= -6)
    change_position(victim, POS_MORTALLYW);
  else if (GET_HIT(victim) <= -3)
    change_position(victim, POS_INCAP);
  else if (GET_HIT(victim) == 0)
    change_position(victim, POS_STUNNED);

  else
  { // hp > 0
    if (GET_POS(victim) < POS_RESTING)
    {
      if (!AWAKE(victim))
      {
        GUI_CMBT_OPEN(victim);
        send_to_char(victim, "\tRYour sleep is disturbed!!\tn  ");
        GUI_CMBT_CLOSE(victim);
      }
      change_position(victim, POS_SITTING);
      GUI_CMBT_OPEN(victim);
      send_to_char(victim,
                   "You instinctively shift from dangerous positioning to sitting...\r\n");
      GUI_CMBT_CLOSE(victim);
    }
  }
}

void update_pos(struct char_data *victim)
{

  if (HAS_FEAT(victim, FEAT_DEATHLESS_FRENZY) && affected_by_spell(victim, SKILL_RAGE))
  {
    if (GET_HIT(victim) <= -121)
      change_position(victim, POS_DEAD);
    else
      return;
  }

  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;

  if (GET_HIT(victim) <= -11)
    change_position(victim, POS_DEAD);
  else if (GET_HIT(victim) <= -6)
    change_position(victim, POS_MORTALLYW);
  else if (GET_HIT(victim) <= -3)
    change_position(victim, POS_INCAP);
  else if (GET_HIT(victim) == 0)
    change_position(victim, POS_STUNNED);

  // hp > 0 , pos <= stunned
  else
  {
    change_position(victim, POS_RESTING);
    send_to_char(victim,
                 "You find yourself in a resting position...\r\n");
  }
}

/* if appropriate, this function will set the 'killer' flag
   a 'killer' is someone who pkilled against the current ruleset
 */
void check_killer(struct char_data *ch, struct char_data *vict)
{
  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
    return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
    return;
  if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_PEACEFUL))
  {
    send_to_char(ch, "You will not be flagged as a killer for "
                     "attempting to attack in a peaceful room...\r\n");
    return;
  }
  if (GET_LEVEL(ch) > LVL_IMMORT)
  {
    send_to_char(ch, "Normally you would've been flagged a "
                     "PKILLER for this action...\r\n");
    return;
  }
  if (GET_LEVEL(vict) > LVL_IMMORT && !IS_NPC(vict))
  {
    send_to_char(ch, "You will not be flagged as a killer for "
                     "attacking an Immortal...\r\n");
    return;
  }

  /* // We don't use the killer flag anymore -- gicker june 18, 2021
  SET_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
  send_to_char(ch, "If you want to be a PLAYER KILLER, so be it...\r\n");
  mudlog(BRF, LVL_IMMORT, TRUE, "PC Killer bit set on %s for "
                                "initiating attack on %s at %s.",
         GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)].name);
  */
}

/* a function that sets ch fighting victim */

/* TRUE - succeeding in engaging in combat
   FALSE - failed to engage in combat */
bool set_fighting(struct char_data *ch, struct char_data *vict)
{
  struct char_data *current = NULL, *previous = NULL;
  int delay;

  if (ch == vict)
    return FALSE;

  if (FIGHTING(ch))
  {
    core_dump();
    return FALSE;
    ;
  }

  if (char_has_mud_event(ch, eCOMBAT_ROUND))
  {
    return FALSE;
    ;
  }

  GET_INITIATIVE(ch) = roll_initiative(ch);

  if (combat_list == NULL)
  {
    ch->next_fighting = combat_list;
    combat_list = ch;
  }
  else
  {
    for (current = combat_list; current != NULL; current = current->next_fighting)
    {
      if ((GET_INITIATIVE(ch) > GET_INITIATIVE(current)) ||
          ((GET_INITIATIVE(ch) == GET_INITIATIVE(current)) &&
           (GET_DEX_BONUS(ch) < GET_DEX_BONUS(current))))
      {
        previous = current;
        continue;
      }
      break;
    }
    if (previous == NULL)
    {
      /* First. */
      ch->next_fighting = combat_list;
      combat_list = ch;
    }
    else
    {
      ch->next_fighting = current;
      previous->next_fighting = ch;
    }
  }

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  /*  The char is flat footed until they take an action,
   *  but only if they are not currently fighting.  */
  if (!FIGHTING(ch))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_FLAT_FOOTED);
  FIGHTING(ch) = vict;

  if (!CONFIG_PK_ALLOWED)
    check_killer(ch, vict);

  if (GET_INITIATIVE(ch) >= GET_INITIATIVE(vict))
    delay = 2 RL_SEC;
  else
    delay = 4 RL_SEC;

  /* make sure firing if appropriate */
  if (can_fire_ammo(ch, TRUE))
    FIRING(ch) = TRUE;

  /* start the combat loop, making sure we begin with phase "1" */
  attach_mud_event(new_mud_event(eCOMBAT_ROUND, ch, strdup("1")), delay);

  return TRUE;
}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
  struct char_data *temp = NULL;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  FIRING(ch) = 0;
  if (GET_POS(ch) == POS_FIGHTING) /* in case they are position fighting */
    change_position(ch, POS_STANDING);
  update_pos(ch);

  /* don't forget to remove the fight event! */
  if (char_has_mud_event(ch, eCOMBAT_ROUND))
  {
    event_cancel_specific(ch, eCOMBAT_ROUND);
  }

  /* Reset the combat data */
  GET_TOTAL_AOO(ch) = 0;
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLAT_FOOTED);
  if (affected_by_spell(ch, SKILL_SMITE_EVIL))
  {
    affect_from_char(ch, SKILL_SMITE_EVIL);
    send_to_char(ch, "Your smite evil affect expires.\r\n");
  }
  if (affected_by_spell(ch, SKILL_SMITE_GOOD))
  {
    send_to_char(ch, "Your smite good affect expires.\r\n");
    affect_from_char(ch, SKILL_SMITE_GOOD);
  }
  ch->player_specials->has_banishment_been_attempted = false;
}

/* PC:  function for creating corpses, ch just died -zusuk */
static void make_pc_corpse(struct char_data *ch)
{
  char buf2[MAX_NAME_LENGTH + 64] = {'\0'};
  struct obj_data *corpse = NULL;
  int x = 0, y = 0;

  /* create the corpse object, blank prototype */
  corpse = create_obj();

  /* start setting up all the variables for a corpse */
  corpse->item_number = NOTHING;

  IN_ROOM(corpse) = NOWHERE;

  snprintf(buf2, sizeof(buf2), "pcorpse %s pcorpse_%s",
           GET_NAME(ch), GET_NAME(ch));
  corpse->name = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "%sThe corpse of %s%s is lying here.",
           CCNRM(ch, C_NRM), GET_NAME(ch), CCNRM(ch, C_NRM));
  corpse->description = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "%sthe corpse of %s%s", CCNRM(ch, C_NRM),
           GET_NAME(ch), CCNRM(ch, C_NRM));
  corpse->short_description = strdup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;

  for (x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++)
  {
    if (x < EF_ARRAY_MAX)
      GET_OBJ_EXTRA_AR(corpse, x) = 0;
    if (y < TW_ARRAY_MAX)
      corpse->obj_flags.wear_flags[y] = 0;
  }

  SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);

  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);

  GET_OBJ_VAL(corpse, 0) = 0; /* You can't store stuff in a corpse */

  GET_OBJ_VAL(corpse, 3) = 1; /* corpse identifier */

  GET_OBJ_VAL(corpse, 4) = GET_IDNUM(ch); /* save the ID on the object value */

  GET_OBJ_VAL(corpse, 5) = GET_LOST_XP(ch); /* save the xp loss into the object */

  /* todo for players? : save race, etc */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch);

  GET_OBJ_RENT(corpse) = 100000;

  GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;
  /* ok done setting up the corpse */

  /* place corpse in room */
  obj_to_room(corpse, IN_ROOM(ch));
}

/* NPC:  function for creating corpses, ch just died */
static void make_corpse(struct char_data *ch)
{
  char buf2[MAX_NAME_LENGTH + 64] = {'\0'};
  struct obj_data *corpse = NULL, *o = NULL;
  struct obj_data *money = NULL;
  int i = 0, x = 0, y = 0;

  /* handle mobile death that should not leave a corpse */
  if (IS_NPC(ch))
  { /* necessary check because of morphed druids */
    if (IS_UNDEAD(ch) ||
        IS_ELEMENTAL(ch) ||
        IS_INCORPOREAL(ch))
    {
      GUI_CMBT_OPEN(ch);
      send_to_char(ch, "You feel your body crumble to dust!\r\n");
      GUI_CMBT_CLOSE(ch);

      GUI_CMBT_NOTVICT_OPEN(ch, NULL);
      act("With a final moan $n crumbles to dust!",
          FALSE, ch, NULL, NULL, TO_ROOM);
      GUI_CMBT_NOTVICT_CLOSE(ch, NULL);

      /* transfer gold */
      if (GET_GOLD(ch) > 0)
      {
        /* duplication loophole */
        if (IS_NPC(ch) || ch->desc)
        {
          money = create_money(GET_GOLD(ch));
          obj_to_room(money, IN_ROOM(ch));
          obj_to_obj(money, corpse);
        }
        GET_GOLD(ch) = 0;
      }
      extract_char(ch);
      return;
    }
  } /* if we continue on, we need to actually make a corpse.... */

  /* create the corpse object, blank prototype */
  corpse = create_obj();

  /* start setting up all the variables for a corpse */
  corpse->item_number = NOTHING;
  IN_ROOM(corpse) = NOWHERE;
  corpse->name = strdup("corpse");

  snprintf(buf2, sizeof(buf2), "%sThe corpse of %s%s is lying here.",
           CCNRM(ch, C_NRM), GET_NAME(ch), CCNRM(ch, C_NRM));
  corpse->description = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "%sthe corpse of %s%s", CCNRM(ch, C_NRM),
           GET_NAME(ch), CCNRM(ch, C_NRM));
  corpse->short_description = strdup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  for (x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++)
  {
    if (x < EF_ARRAY_MAX)
      GET_OBJ_EXTRA_AR(corpse, x) = 0;
    if (y < TW_ARRAY_MAX)
      corpse->obj_flags.wear_flags[y] = 0;
  }
  SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);
  GET_OBJ_VAL(corpse, 0) = 0; /* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = 1; /* corpse identifier */
  /* todo for players: save id onto corpse, and save race, etc */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_RENT(corpse) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
  else
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;
  // was the corpse killed while being blood drained?
  // If so we want to set drainKilled, so the corpse can
  // be raised as vampire spawn.
  if (ch->char_specials.drainKilled)
    corpse->drainKilled = true;
  // The character's short desc for things like vampire spawn
  corpse->char_sdesc = strdup(ch->player.short_descr);

  /* ok done setting up the corpse */

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
    {
      remove_otrigger(GET_EQ(ch, i), ch);
      obj_to_obj(unequip_char(ch, i), corpse);
    }

  /* transfer gold */
  if (GET_GOLD(ch) > 0)
  {
    /* following 'if' clause added to fix gold duplication loophole. The above
     * line apparently refers to the old "partially log in, kill the game
     * character, then finish login sequence" duping bug. The duplication has
     * been fixed (knock on wood) but the test below shall live on, for a
     * while. -gg 3/3/2002 */
    if (IS_NPC(ch) || ch->desc)
    {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  /* empty out inventory and carrying-number and carrying-weight */
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  /* place filled corpse in room */
  obj_to_room(corpse, IN_ROOM(ch));
}

/* When ch kills victim */
static void change_alignment(struct char_data *ch, struct char_data *victim)
{
  if (GET_ALIGNMENT(victim) < GET_ALIGNMENT(ch) && !rand_number(0, 19))
  {
    if (GET_ALIGNMENT(ch) < 1000)
      GET_ALIGNMENT(ch)
    ++;
  }
  else if (GET_ALIGNMENT(victim) > GET_ALIGNMENT(ch) && !rand_number(0, 19))
  {
    if (GET_ALIGNMENT(ch) > -1000)
      GET_ALIGNMENT(ch)
    --;
  }

  /* new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast. */
  //  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}

/* a function for 'audio' effect of killing, notifies neighboring
 room of a nearby death */
void death_cry(struct char_data *ch)
{
  int door = -1;
  struct descriptor_data *pt = NULL;

  if (IS_NPC(ch))
  {
    switch (GET_MOB_VNUM(ch))
    {

    /* special custom death cry messages can be placed here for NPCs! */
    case THE_PRISONER:
      /* no message here, special one in zone_procs.c code */
      break;
    case DRACOLICH_PRISONER:

      for (pt = descriptor_list; pt; pt = pt->next)
      {
        if (pt->character)
        {
          send_to_char(pt->character, "\tLWith a final horrifying wail, the skeletal remains of the Prisoner\n\r"
                                      "fall to the ground with a resounding thud.\tn"
                                      "\n\r\n\r\n\r\twThe mighty \tLPrisoner \twfinally ceases to move.\tn\r\n");
        }
      }

      break;

    default:

      /* this is the default NPC message */
      GUI_CMBT_NOTVICT_OPEN(ch, NULL);
      act("Your blood freezes as you hear $n's death cry.",
          FALSE, ch, 0, 0, TO_ROOM);
      GUI_CMBT_NOTVICT_CLOSE(ch, NULL);

      for (door = 0; door < DIR_COUNT; door++)
      {
        if (CAN_GO(ch, door))
        {
          send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room,
                       "Your blood freezes as you hear someone's death cry.\r\n");
        }
      }
      break;
    }
  }
  else /* this is the default PC message */
  {
    GUI_CMBT_NOTVICT_OPEN(ch, NULL);
    act("Your blood freezes as you hear $n's death cry.",
        FALSE, ch, 0, 0, TO_ROOM);
    GUI_CMBT_NOTVICT_CLOSE(ch, NULL);

    for (door = 0; door < DIR_COUNT; door++)
    {
      if (CAN_GO(ch, door))
      {
        send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room,
                     "Your blood freezes as you hear someone's death cry.\r\n");
      }
    }
  }
}

/* this message is a replacement in our new (temporary?) death system */
void death_message(struct char_data *ch)
{
  GUI_CMBT_OPEN(ch);
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\tD'||''|.   '||''''|      |     |''||''| '||'  '||' \tn\r\n");
  send_to_char(ch, "\tD ||   ||   ||  .       |||       ||     ||    ||  \tn\r\n");
  send_to_char(ch, "\tD ||    ||  ||''|      |  ||      ||     ||''''||  \tn\r\n");
  send_to_char(ch, "\tD ||    ||  ||        .''''|.     ||     ||    ||  \tn\r\n");
  send_to_char(ch, "\tD.||...|'  .||.....| .|.  .||.   .||.   .||.  .||. \tn\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\tYTIPS - \tRbe creative and tenacious!\tn\r\n");
  send_to_char(ch, "1) Try stocking up on disposables such as scrolls, potions and wands\r\n");
  send_to_char(ch, "2) Try bringing some friends with you, if you can't find someone on chat - check our discord\r\n");
  send_to_char(ch, "3) Try a 'respec' and selecting different feats\r\n");
  send_to_char(ch, "4) Upgrade your gear, check out the bazaar, player shops and quest shop in sanctus for some nice upgrades\r\n");
  send_to_char(ch, "5) Try gaining some levels\r\n");
  send_to_char(ch, "6) Try hiring a mercenary\r\n");
  send_to_char(ch, "7) Try different tactics for the battle, such as hit-and-run\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "\r\n");
  send_to_char(ch, "You awaken... you realize someone has resurrected you...\r\n");
  send_to_char(ch, "\r\n");
  GUI_CMBT_CLOSE(ch);
}

/* Added quest completion for all group members if they are in the room.
 * Oct 6, 2014 - Ornir. */
void kill_quest_completion_check(struct char_data *killer, struct char_data *ch)
{
  struct group_data *group = NULL;
  struct char_data *k = NULL;
  struct iterator_data it;

  /* dummy checks */
  if (!killer)
    return;
  if (!ch)
    return;

  /* check for killer first */
  autoquest_trigger_check(killer, ch, NULL, 0, AQ_MOB_KILL);

  /* check for all group members next */
  group = GROUP(killer);

  if (group != NULL)
  {
    /* Initialize the iterator */

    for (k = (struct char_data *)merge_iterator(&it, group->members); k != NULL; k = (struct char_data *)next_in_list(&it))
    {

      if (k == killer) /* should not need this */
        continue;
      if (IS_PET(k))
        continue;
      if (IN_ROOM(k) == IN_ROOM(killer))
        autoquest_trigger_check(k, ch, NULL, 0, AQ_MOB_KILL);
    }
    /* Be kind, rewind. */
    remove_iterator(&it);
  }
}

/* PC death: we are not extracting OR creating corpses.  Open to
     restoring corpse creation upon creation of a good corpse
     saving solution so we do not have to worry about copyover
     or crashes deleting all of the PC's gear */
void raw_kill(struct char_data *ch, struct char_data *killer)
{
  struct char_data *k, *temp;
  struct affected_type af;

  /* stop relevant fighting */
  if (FIGHTING(ch))
    stop_fighting(ch);

  for (k = combat_list; k; k = temp)
  {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }

  /* clear all affections */
  while (ch->affected)
    affect_remove(ch, ch->affected);

  /* this was commented out for some reason, undid that to make sure
   events clear on death */
  clear_char_event_list(ch);

  /* Wipe character from the memory of hunters and other intelligent NPCs... */
  for (temp = character_list; temp; temp = temp->next)
  {
    /* PCs can't use MEMORY, and don't use HUNTING() */
    if (!IS_NPC(temp))
      continue;
    /* If "temp" is hunting our extracted char, stop the hunt. */
    if (HUNTING(temp) == ch)
      HUNTING(temp) = NULL;
    /* If "temp" has allocated memory data and our ch is a PC, forget the
     * extracted character (if he/she is remembered) */
    if (!IS_NPC(ch) && MEMORY(temp))
      forget(temp, ch); /* forget() is safe to use without a check. */
  }

  /* handle pets */
  if (ch->followers || ch->master) // handle followers
    die_follower(ch);
  save_char_pets(ch);

  /* group handling */
  if (GROUP(ch))
  {
    send_to_group(ch, GROUP(ch), "%s has died.\r\n", GET_NAME(ch));

    /* we used to kick players out of groups here, but we changed that.. now only NPCs */
    if (IS_NPC(ch))
      leave_group(ch);
  }

  /* ordinary commands work in scripts -welcor */
  GET_POS(ch) = POS_STANDING;
  if (killer)
  {
    if (death_mtrigger(ch, killer))
      death_cry(ch);
  }
  else
    death_cry(ch);
  GET_POS(ch) = POS_DEAD;
  /* end making ordinary commands work in scripts */

  /* make sure group gets credit for kill if ch involved in autoquest auto-quest */
  kill_quest_completion_check(killer, ch);

  /* Clear the action queue */
  clear_action_queue(GET_QUEUE(ch));

  /* random treasure drop, other related drops */
  if (killer && ch && IS_NPC(ch))
  {
    determine_treasure(find_treasure_recipient(killer), ch);
    if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_HUNTS_TARGET))
    {
      drop_hunt_mob_rewards(killer, ch);
      remove_hunts_mob(ch->mob_specials.hunt_type);
    }
  }

  /* spec-abil saves on exit, so make sure this does not save */
  DOOM(ch) = 0;
  INCENDIARY(ch) = 0;
  CLOUDKILL(ch) = 0;
  GET_MARK(killer) = NULL;
  GET_MARK_ROUNDS(killer) = 0;

  /* final handling, primary difference between npc/pc death */
  if (IS_NPC(ch))
  {

    /* added a switch here for special handling of special mobiles -zusuk */
    switch (GET_MOB_VNUM(ch))
    {

    case THE_PRISONER:
      /* this is the head/dracolich transitions */
      prisoner_on_death(ch);
      /* extraction!  *SLURRRRRRRRRRRRRP* */
      extract_char(ch);
      break;

    case DRACOLICH_PRISONER:

      /* we are ending the event here! */
      if (IS_STAFF_EVENT && STAFF_EVENT_NUM == THE_PRISONER_EVENT)
        end_staff_event(STAFF_EVENT_NUM);
      /* this is our indication to the staff event code that the prisoner has already been killed and shouldn't be started */
      prisoner_heads = -2;
      /*****/

      make_corpse(ch);
      /* extraction!  *SLURRRRRRRRRRRRRP* */
      extract_char(ch);

      break;

    default:
      make_corpse(ch);
      /* extraction!  *SLURRRRRRRRRRRRRP* */
      extract_char(ch);
      break;
    }
  }
  else if (IN_ARENA(ch) || IN_ARENA(killer))
  {
    /* no corpse - arena */

    /* move the character out of the room */
    char_from_room(ch);

    /* death message */
    death_message(ch);

    /* get you back to life */
    GET_HIT(ch) = GET_MAX_HIT(ch);
    update_pos(ch);

    /* move char to starting room of arena */
    char_to_room(ch, real_room(CONFIG_ARENA_DEATH));
    act("$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);
    entry_memory_mtrigger(ch);
    greet_mtrigger(ch, -1);
    greet_memory_mtrigger(ch);

    /* make sure not casting! */
    resetCastingData(ch);

    /* save! */
    save_char(ch, 0);
    Crash_delete_crashfile(ch);
  }
  else
  { /* real DEATH! */
    /* create the corpse */
    if (!IN_ARENA(ch) && !IN_ARENA(killer))
      make_pc_corpse(ch);

    /* move the character out of the room */
    char_from_room(ch);

    /* death message */
    death_message(ch);

    /* get you back to life */
    GET_HIT(ch) = 1;
    update_pos(ch);

    // Apply recently deceased affect
    new_affect(&af);
    af.spell = AFFECT_RECENTLY_DIED;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = 30; // 3 minutes
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);

    /* move char to starting room */
    char_to_room(ch, r_mortal_start_room);
    act("$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);
    entry_memory_mtrigger(ch);
    greet_mtrigger(ch, -1);
    greet_memory_mtrigger(ch);

    /* make sure not casting! */
    resetCastingData(ch);

    /* save! */
    save_char(ch, 0);
    Crash_delete_crashfile(ch);

    /* "punishment" for death */
    start_action_cooldown(ch, atSTANDARD, 12 RL_SEC);
  }

  /* autoquest system check point -Zusuk */
  if (killer)
  {
    autoquest_trigger_check(killer, NULL, NULL, 0, AQ_MOB_SAVE);
    autoquest_trigger_check(killer, NULL, NULL, 0, AQ_ROOM_CLEAR);
  }
}

/* called after striking the mortal blow to ch */
#define XP_LOSS_FACTOR 5 /*20%*/

void die(struct char_data *ch, struct char_data *killer)
{

  if (!killer)
    return;

  struct char_data *temp;
  struct descriptor_data *pt;
  int xp_to_lvl = level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch));
  int penalty = xp_to_lvl / XP_LOSS_FACTOR;

  penalty = penalty * CONFIG_DEATH_EXP_LOSS / 100;

  if (GET_LEVEL(ch) <= 6)
  {
    /* no xp loss for newbs - Bakarus */
  }
  else if (IS_STAFF_EVENT && STAFF_EVENT_NUM == THE_PRISONER_EVENT)
  {
    /* no exp loss during the prisoner event */
  }
  else
  {
    // if not a newbie then bang that xp! - Bakarus
    if (!IN_ARENA(ch) && !IN_ARENA(killer))
    {
      /* we are storing lost xp for ressurect */
      GET_LOST_XP(ch) = gain_exp(ch, -penalty, GAIN_EXP_MODE_DEATH);
    }
  }

  if (!IS_NPC(ch))
  {
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_SALVATION);
    if (AFF_FLAGGED(ch, AFF_SPELLBATTLE))
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SPELLBATTLE);
    SPELLBATTLE(ch) = 0;
    wildshape_return(ch);
  }

  /* clear grapple */
  if (GRAPPLE_TARGET(ch))
  {
    GRAPPLE_TARGET(ch) = NULL;
  }
  for (temp = character_list; temp; temp = temp->next)
  {
    if (GRAPPLE_TARGET(temp) == ch)
    {
      clear_grapple(temp, ch);
    }
  }

  /* clear guard */
  if (GUARDING(ch))
  {
    GUARDING(ch) = NULL;
  }
  for (temp = character_list; temp; temp = temp->next)
  {
    if (GUARDING(temp) == ch)
    {
      GUARDING(temp) = NULL;
    }
  }

  /* Info-Kill mobs against player, print info about the death of this player by the mob to the world
   * TODO: add info channel for these guys */
  if (IS_NPC(killer) && MOB_FLAGGED(killer, MOB_INFO_KILL_PLR))
  {
    for (pt = descriptor_list; pt; pt = pt->next)
    {
      if (IS_PLAYING(pt) && pt->character)
      {
        if (GROUP(ch) && GROUP(ch)->members->iSize)
        {
          send_to_char(pt->character, "\tR[\tW%s\tR]\tn %s of %s's group has been defeated by %s!\r\n",
                       MOB_FLAGGED(killer, MOB_HUNTS_TARGET) ? "Hunt" : "Info",
                       GET_NAME(ch), GET_NAME(ch->group->leader), GET_NAME(killer));
        }
        else if (IS_NPC(ch) && ch->master)
        {
          send_to_char(pt->character, "\tR[\tW%s\tR]\tn %s's follower has been defeated by %s!\r\n",
                       MOB_FLAGGED(killer, MOB_HUNTS_TARGET) ? "Hunt" : "Info",
                       GET_NAME(ch->master), GET_NAME(killer));
        }
        else
        {
          send_to_char(pt->character, "\tR[\tW%s\tR]\tn %s has been defeated by %s!\r\n",
                       MOB_FLAGGED(killer, MOB_HUNTS_TARGET) ? "Hunt" : "Info",
                       GET_NAME(ch), GET_NAME(killer));
        }
      }
    }
  }

  /* Info-Kill mobs, print info about the death of this mob to the world
   * TODO: add info channel for these guys */
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_INFO_KILL))
  {
    for (pt = descriptor_list; pt; pt = pt->next)
    {
      if (IS_PLAYING(pt) && pt->character)
      {
        if (GROUP(killer) && GROUP(killer)->members->iSize)
        {
          send_to_char(pt->character, "\tR[\tW%s\tR]\tn %s of %s's group has defeated %s!\r\n",
                       MOB_FLAGGED(ch, MOB_HUNTS_TARGET) ? "Hunt" : "Info",
                       GET_NAME(killer), GET_NAME(killer->group->leader), GET_NAME(ch));
        }
        else if (IS_NPC(killer) && killer->master)
        {
          send_to_char(pt->character, "\tR[\tW%s\tR]\tn %s's follower has defeated %s!\r\n",
                       MOB_FLAGGED(ch, MOB_HUNTS_TARGET) ? "Hunt" : "Info",
                       GET_NAME(killer->master), GET_NAME(ch));
        }
        else
        {
          send_to_char(pt->character, "\tR[\tW%s\tR]\tn %s has defeated %s!\r\n",
                       MOB_FLAGGED(ch, MOB_HUNTS_TARGET) ? "Hunt" : "Info",
                       GET_NAME(killer), GET_NAME(ch));
        }
      }
    }
  }

  raw_kill(ch, killer);
}

/* called for splitting xp in a group (engine) */
static void perform_group_gain(struct char_data *ch, int base,
                               struct char_data *victim)
{
  int share, hap_share;

  share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, base));

  if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
  {
    /* This only reports the correct amount - the calc is done in gain_exp */
    hap_share = share + (int)((float)share * ((float)HAPPY_EXP / (float)(100)));
    share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, hap_share));
  }

  if (share > 1)
    send_to_char(ch, "You receive your share of experience -- %d points.\r\n", gain_exp(ch, share, GAIN_EXP_MODE_GROUP));
  else
  {
    send_to_char(ch, "You receive your share of experience -- one measly little point!\r\n");
    gain_exp(ch, share, GAIN_EXP_MODE_GROUP);
  }

  change_alignment(ch, victim);
}

/* called for splitting xp in a group (prelim) */
#define BONUS_PER_MEMBER 2

static void group_gain(struct char_data *ch, struct char_data *victim)
{
  int tot_members = 0, base = 0, tot_gain = 0;
  struct char_data *k;
  int party_level = 0;

  /* count total members in group and total party level */
  while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
  {
    if (IS_PET(k))
    {
      continue;
    }

    if (IN_ROOM(ch) == IN_ROOM(k))
    {
      tot_members++;
      party_level += GET_LEVEL(k);
    }
  }

  /* what is our average party level? */
  if (tot_members)
    party_level /= tot_members;

  /* total XP received, round up to the nearest tot_members */
  tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;

  /* Calculate level-difference bonus */
  if (GET_LEVEL(victim) > party_level)
  {
    if (IS_NPC(ch))
      tot_gain += MAX(0, (tot_gain * MIN(4, (GET_LEVEL(victim) - party_level))) / 8);
    else
      tot_gain += MAX(0, (tot_gain * MIN(8, (GET_LEVEL(victim) - party_level))) / 8);
  }

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(CONFIG_MAX_EXP_LOSS * 2 / 3, tot_gain);

  /* base: split the xp between the individuals in the party */
  if (tot_members >= 1)
    base = MAX(1, tot_gain / tot_members);
  else
    base = 0;

  /* if mob isn't within X levels, don't give xp -zusuk */
  if ((GET_LEVEL(victim) + CONFIG_EXP_LEVEL_DIFFERENCE) < party_level)
    base = 1;

  /* XP bonus for groupping */
  if (tot_members > 1)
    base = 1 + base *
                   ((100 + (tot_members * BONUS_PER_MEMBER)) / 100);

  while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
  {
    if (IS_PET(k))
      continue;
    if (IN_ROOM(k) == IN_ROOM(ch))
      perform_group_gain(k, base, victim);
  }
}
#undef BONUS_PER_MEMBER

/* called for splitting xp if NOT in a group (engine) */
static void solo_gain(struct char_data *ch, struct char_data *victim)
{
  int exp = 0, happy_exp = 0;

  /* the base exp is the totally victim's exp divided by 3, limited by config */
  exp = MIN(CONFIG_MAX_EXP_GAIN, GET_EXP(victim) / 3);

  /* Calculate level-difference bonus */
  if (GET_LEVEL(victim) > GET_LEVEL(ch))
  {
    if (IS_NPC(ch))
      exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
    else
      exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
  }

  /* minimum of 1 xp point */
  exp = MAX(exp, 1);

  /* if mob isn't within X levels, don't give xp -zusuk */
  if ((GET_LEVEL(victim) + CONFIG_EXP_LEVEL_DIFFERENCE) < GET_LEVEL(ch))
    exp = 1;

  /* avoid xp abuse in PvP */
  if (!IS_NPC(victim))
    exp = MIN(CONFIG_MAX_EXP_LOSS * 2 / 3, exp);

  /* happyhour bonus XP */
  if (IS_HAPPYHOUR && IS_HAPPYEXP)
  {
    happy_exp = exp + (int)((float)exp * ((float)HAPPY_EXP / (float)(100)));
    exp = MAX(happy_exp, 1);
  }

  if (exp > 1)
    send_to_char(ch, "You receive %d experience points.\r\n", gain_exp(ch, exp, GAIN_EXP_MODE_SOLO));
  else
  {
    send_to_char(ch, "You receive one lousy experience point.\r\n");
    gain_exp(ch, exp, GAIN_EXP_MODE_SOLO);
  }

  change_alignment(ch, victim);
}

/* this function replaces the #w or #W with an appropriate weapon
   constant dependent on plural or not */
static char *replace_string(const char *str, const char *weapon_singular,
                            const char *weapon_plural)
{
  static char buf[MEDIUM_STRING];
  char *cp = buf;

  for (; *str; str++)
  {
    if (*str == '#')
    {
      switch (*(++str))
      {
      case 'W':
        for (; *weapon_plural; *(cp++) = *(weapon_plural++))
          ;
        break;
      case 'w':
        for (; *weapon_singular; *(cp++) = *(weapon_singular++))
          ;
        break;
      default:
        *(cp++) = '#';
        break;
      }
    }
    else
      *(cp++) = *str;

    *cp = 0;
  } /* For */

  return (buf);
}

/* message for doing damage with a weapon */
static void dam_message(int dam, struct char_data *ch, struct char_data *victim,
                        int w_type, int offhand)
{
  int msgnum = -1, hp = 0, pct = 0;

  hp = GET_HIT(victim);
  if (GET_HIT(victim) < 1)
    hp = 1;

  pct = 100 * dam / hp;

  if (dam && pct <= 0)
    pct = 1;

  if (affected_by_spell(ch, SKILL_DRHRT_CLAWS))
    w_type = TYPE_CLAW;

  static struct dam_weapon_type
  {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {
      /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */
      {
          "\tn$n tries to #w \tn$N, but misses.\tn", /* 0: 0     */
          "You try to #w \tn$N, but miss.\tn",
          "\tn$n tries to #w you, but misses.\tn"},
      {"\tn$n \tYbarely grazes \tn$N \tYas $e #W $M.\tn", /* 1: dam <= 2% */
       "\tMYou barely graze \tn$N \tMas you #w $M.\tn",
       "\tn$n \tRbarely grazes you as $e #W you.\tn"},
      {"\tn$n \tYnicks \tn$N \tYas $e #W $M.", /* 2: dam <= 4% */
       "\tMYou nick \tn$N \tMas you #w $M.",
       "\tn$n \tRnicks you as $e #W you.\tn"},
      {"\tn$n \tYbarely #W \tn$N\tY.\tn", /* 3: dam <= 6%  */
       "\tMYou barely #w \tn$N\tM.\tn",
       "\tn$n \tRbarely #W you.\tn"},
      {"\tn$n \tY#W \tn$N\tY.\tn", /* 4: dam <= 8%  */
       "\tMYou #w \tn$N\tM.\tn",
       "\tn$n \tR#W you.\tn"},
      {"\tn$n \tY#W \tn$N \tYhard.\tn", /* 5: dam <= 11% */
       "\tMYou #w \tn$N \tMhard.\tn",
       "\tn$n \tR#W you hard.\tn"},
      {"\tn$n \tY#W \tn$N \tYvery hard.\tn", /* 6: dam <= 14%  */
       "\tMYou #w \tn$N \tMvery hard.\tn",
       "\tn$n \tR#W you very hard.\tn"},
      {"\tn$n \tY#W \tn$N \tYextremely hard.\tn", /* 7: dam <= 18%  */
       "\tMYou #w \tn$N \tMextremely hard.\tn",
       "\tn$n \tR#W you extremely hard.\tn"},
      {"\tn$n \tYinjures \tn$N \tYwith $s #w.\tn", /* 8: dam <= 22%  */
       "\tMYou injure \tn$N \tMwith your #w.\tn",
       "\tn$n \tRinjures you with $s #w.\tn"},
      {"\tn$n \tYwounds \tn$N \tYwith $s #w.\tn", /* 9: dam <= 27% */
       "\tMYou wound \tn$N \tMwith your #w.\tn",
       "\tn$n \tRwounds you with $s #w.\tn"},
      {"\tn$n \tYinjures \tn$N \tYharshly with $s #w.\tn", /* 10: dam <= 32%  */
       "\tMYou injure \tn$N \tMharshly with your #w.\tn",
       "\tn$n \tRinjures you harshly with $s #w.\tn"},
      {"\tn$n \tYseverely wounds \tn$N \tYwith $s #w.\tn", /* 11: dam <= 40% */
       "\tMYou severely wound \tn$N \tMwith your #w.\tn",
       "\tn$n \tRseverely wounds you with $s #w.\tn"},
      {"\tn$n \tYinflicts grave damage on \tn$N\tY with $s #w.\tn", /* 12: dam <= 50% */
       "\tMYou inflict grave damage on \tn$N \tMwith your #w.\tn",
       "\tn$n \tRinflicts grave damage on you with $s #w.\tn"},
      {"\tn$n \tYnearly kills \tn$N\tY with $s deadly #w!!\tn", /* (13): > 51   */
       "\tMYou nearly kill \tn$N \tMwith your deadly #w!!\tn",
       "\tn$n \tRnearly kills you with $s deadly #w!!\tn"}};

  static struct dam_ranged_weapon_type
  {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_ranged[] = {
      {"*WHOOSH* $n fires $p at $N but misses!", /* 0: 0     */
       "\ty*WHOOSH*\ty you fire \tn$p\ty at \tn$N \tybut \tYmiss!\tn",
       "*WHOOSH* $n fires $p at you but misses!"},
      {"*THWISH* $n fires $p at $N grazing $M.", /* 1: dam <= 2% */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]grazing $M.\tn",
       "]*THWISH* $n fires $p at you grazing you."},
      {"*THWISH* $n fires $p at $N nicking $M.", /* 2: dam <= 4% */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]nicking $M.\tn",
       "*THWISH* $n fires $p at you nicking you."},
      {"*THWISH* $n fires $p at $N *THUNK* barely damaging $M.", /* 3: dam <= 6%  */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THUNK* barely damaging $M.\tn",
       "*THWISH* $n fires $p at you *THUNK* barely damaging you."},
      {"*THWISH* $n fires $p at $N *THUNK* damaging $M.", /* 4: dam <= 8%  */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THUNK* damaging $M.",
       "*THWISH* $n fires $p at you *THUNK* damaging you."},
      {"*THWISH* $n fires $p at $N *THUNK* damaging $M moderately!", /* 5: dam <= 11% */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THUNK* damaging $M moderately!\tn",
       "*THWISH* $n fires $p at you *THUNK* damaging you moderately!"},
      {"*THWISH* $n fires $p at $N *THUNK* damaging $M badly!", /* 6: dam <= 14%  */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THUNK* damaging $M badly!\tn",
       "*THWISH* $n fires $p at you *THUNK* damaging you badly!"},
      {"*THWISH* $n fires $p at $N *THUNK* injuring $M harshly!", /* 7: dam <= 18%  */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THUNK* injuring $M harshly!\tn",
       "*THWISH* $n fires $p at you *THUNK* injuring you harshly!"},
      {"*THWISH* $n fires $p at $N *THWAK* severely injuring $M!", /* 8: dam <= 22%  */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* severely injuring $M!\tn",
       "*THWISH* $n fires $p at you *THWAK* severely injuring you!"},
      {"*THWISH* $n fires $p at $N *THWAK* causing serious wounds to $M!", /* 9: dam <= 27% */
       "\t[f500]*THWISH*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* causing serious wounds to $M!\tn",
       "*THWISH* $n fires $p at you *THWAK* causing serious wounds to you!"},
      {"*THFFFT* $n fires $p at $N *THWAK* damaging $M gravely!", /* 10: dam <= 32%  */
       "\t[f500]*THFFFT*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* damaging $M gravely!\tn",
       "*THFFFT* $n fires $p at you *THWAK* damaging you gravely!"},
      {"*THFFFT* $n fires $p at $N *THWAK* severely wounding $M!", /* 11: dam <= 40% */
       "\t[f500]*THFFFT*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* severely wounding $M!\tn",
       "*THFFFT* $n fires $p at you *THWAK* severely wounding you!"},
      {"*THFFFT* $n fires $p at $N *THWAK* lethally wounding $M!", /* 12: dam <= 50% */
       "\t[f500]*THFFFT*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* lethally wounding $M!\tn",
       "*THFFFT* $n fires $p at you *THWAK* lethally wounding you!"},
      {"*THFFFT* $n fires $p at $N *THWAK* nearly killing $M!", /* (13): > 51   */
       "\t[f500]*THFFFT*\tn \t[f030]you fire \tn$p\tn \t[f030]at \tn$N\tn \t[f030]*THWAK* nearly killing $M!\tn",
       "*THFFFT* $n fires $p at you *THWAK* nearly killing you!"}};

  w_type -= TYPE_HIT; /* Change to base of table with text */

  if (pct == 0)
    msgnum = 0;
  else if (pct <= 2)
    msgnum = 1;
  else if (pct <= 4)
    msgnum = 2;
  else if (pct <= 6)
    msgnum = 3;
  else if (pct <= 8)
    msgnum = 4;
  else if (pct <= 11)
    msgnum = 5;
  else if (pct <= 14)
    msgnum = 6;
  else if (pct <= 18)
    msgnum = 7;
  else if (pct <= 22)
    msgnum = 8;
  else if (pct <= 27)
    msgnum = 9;
  else if (pct <= 32)
    msgnum = 10;
  else if (pct <= 40)
    msgnum = 11;
  else if (pct <= 50)
    msgnum = 12;
  else
    msgnum = 13;

  /* ranged */
  if (offhand == 2 && last_missile && GET_POS(victim) > POS_DEAD)
  {

    /* damage message to room */
    act(dam_ranged[msgnum].to_room, FALSE, ch, last_missile, victim, TO_NOTVICT);

    /* damage message to damager */
    act(dam_ranged[msgnum].to_char, FALSE, ch, last_missile, victim, TO_CHAR);
    send_to_char(ch, CCNRM(ch, C_CMP));

    /* damage message to damagee */
    send_to_char(victim, CCRED(victim, C_CMP));
    act(dam_ranged[msgnum].to_victim, FALSE, ch, last_missile, victim, TO_VICT | TO_SLEEP);
    send_to_char(victim, CCNRM(victim, C_CMP));
  } /* non ranged */
  else if (GET_POS(victim) > POS_DEAD)
  {
    char *buf = NULL;

    /* damage message to observers (to room) */
    buf = replace_string(dam_weapons[msgnum].to_room,
                         attack_hit_text[w_type].singular, attack_hit_text[w_type].plural),
    dam;
    GUI_CMBT_NOTVICT_OPEN(ch, victim);
    act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);
    GUI_CMBT_NOTVICT_CLOSE(ch, victim);

    /* damage message to damager (to_ch) */
    buf = replace_string(dam_weapons[msgnum].to_char,
                         attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
    GUI_CMBT_OPEN(ch);
    act(buf, FALSE, ch, NULL, victim, TO_CHAR);
    send_to_char(ch, CCNRM(ch, C_CMP));
    GUI_CMBT_CLOSE(ch);

    /* damage message to damagee (to_vict) */
    buf = replace_string(dam_weapons[msgnum].to_victim,
                         attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
    GUI_CMBT_OPEN(victim);
    act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
    send_to_char(victim, CCNRM(victim, C_CMP));
    GUI_CMBT_CLOSE(victim);
  }
  else
  {
  }
}

/*  message for doing damage with a spell or skill. Also used for weapon
 *  damage on miss and death blows. */
/* took out attacking-staff-messages -zusuk*/
/* this is so trelux's natural attack reflects an actual object */
#define TRELUX_CLAWS 800

int skill_message(int dam, struct char_data *ch, struct char_data *vict,
                  int attacktype, int dualing)
{
  int i, j, nr, return_value = SKILL_MESSAGE_MISS_FAIL;
  struct message_type *msg;
  struct obj_data *opponent_weapon = GET_EQ(vict, WEAR_WIELD_1);
  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD_1);
  struct obj_data *shield = NULL;
  int (*name)(struct char_data * ch, void *me, int cmd, const char *argument);
  bool is_ranged = FALSE;

  if (DEBUGMODE)
  {
    send_to_char(ch, "Debug - We are in skill_message(), dam %d, ch %s, vict %s, attacktype %d, dualing %d\r\n", dam, GET_NAME(ch), GET_NAME(vict), attacktype, dualing);
    send_to_char(vict, "Debug - We are in skill_message(), dam %d, ch %s, vict %s, attacktype %d, dualing %d\r\n", dam, GET_NAME(ch), GET_NAME(vict), attacktype, dualing);
  }

  /* attacker weapon */
  if (GET_EQ(ch, WEAR_WIELD_2H))
    weap = GET_EQ(ch, WEAR_WIELD_2H);
  else if (dualing == 1)
    weap = GET_EQ(ch, WEAR_WIELD_OFFHAND);

  /* special handling for Trelux */
  if (GET_RACE(ch) == RACE_TRELUX)
  {
    weap = read_object(TRELUX_CLAWS, VIRTUAL);
    attacktype = TYPE_CLAW;
  }

  if (affected_by_spell(ch, SKILL_DRHRT_CLAWS))
    attacktype = TYPE_CLAW;

  /* ranged weapon - general check and we want the missile to serve as our weapon */
  if (can_fire_ammo(ch, TRUE) && (dualing == 2))
  {
    is_ranged = TRUE;
    weap = GET_EQ(ch, WEAR_AMMO_POUCH)->contains; /* top missile */
  }

  /* defender weapon for parry message */
  if (!opponent_weapon)
  {
    opponent_weapon = GET_EQ(vict, WEAR_WIELD_2H);
  }

  if (!opponent_weapon)
  { /* maybe no weapon in main hand, but offhand has one */
    opponent_weapon = GET_EQ(vict, WEAR_WIELD_OFFHAND);
  }

  if (GET_EQ(vict, WEAR_WIELD_1) && GET_EQ(vict, WEAR_WIELD_OFFHAND))
  {
    if (rand_number(0, 1))
      opponent_weapon = GET_EQ(vict, WEAR_WIELD_1);
    else
      opponent_weapon = GET_EQ(vict, WEAR_WIELD_OFFHAND);
  }

  /* These attacks use a shield as a weapon. */
  if ((attacktype == SKILL_SHIELD_PUNCH) ||
      (attacktype == SKILL_SHIELD_CHARGE) ||
      (attacktype == SKILL_SHIELD_SLAM))
    weap = GET_EQ(ch, WEAR_SHIELD);

  for (i = 0; i < MAX_MESSAGES; i++)
  {
    /* first search through our messages trying to match the attacktype */
    if (fight_messages[i].a_type == attacktype)
    {
      /* might have several messages for that attacktype, pick a random one */
      nr = dice(1, fight_messages[i].number_of_attacks);
      /* increment the messages until we get to that selected message */
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
        msg = msg->next;
      /* we now have a message! */

      /* old location of staff-messages */

      /* we did some damage or deathblow */
      if (dam != 0)
      {

        if (GET_POS(vict) == POS_DEAD)
        { // death messages
          /* Don't send redundant color codes for TYPE_SUFFERING & other types
           * of damage without attacker_msg. */
          if (is_ranged)
          { /* ranged attack death blow */
            /* death message to room */
            act("* THWISH * $n fires $p at $N * THUNK * $E \tRcollapses\tn to the ground!",
                FALSE, ch, weap, vict, TO_NOTVICT);
            /* death message to damager */
            act("* THWISH * you fire $p at $N * THUNK * $E \tRcollapses\tn to the ground!",
                FALSE, ch, weap, vict, TO_CHAR);
            /* death message to damagee */
            act("* THWISH * $n fires $p at you * THUNK * you \tRcollapse\tn to the ground!",
                FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);

            return SKILL_MESSAGE_DEATH_BLOW; /* no reason to stay here */
          }
          else
          { /* NOT ranged death blow */
            if (msg->die_msg.attacker_msg)
            {
              send_to_char(ch, CCYEL(ch, C_CMP));
              act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
              send_to_char(ch, CCNRM(ch, C_CMP));
            }

            send_to_char(vict, CCRED(vict, C_CMP));
            act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
            send_to_char(vict, CCNRM(vict, C_CMP));

            act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
            return SKILL_MESSAGE_DEATH_BLOW;
          }
        }
        else
        { // not dead
          if (msg->hit_msg.attacker_msg && ch != vict)
          {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

          send_to_char(vict, CCRED(vict, C_CMP));
          act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
          send_to_char(vict, CCNRM(vict, C_CMP));

          act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);

          return SKILL_MESSAGE_GENERIC_HIT;
        }
      } /* dam == 0, we did not do any damage! */
      else if (ch != vict)
      {
        if (DEBUGMODE)
        {
          send_to_char(ch, "Debug - We are in skill_message() - ZERO DAMAGE, dam %d, ch %s, vict %s, attacktype %d, dualing %d\r\n", dam, GET_NAME(ch), GET_NAME(vict), attacktype, dualing);
          send_to_char(vict, "Debug - We are in skill_message() - ZERO DAMAGE, dam %d, ch %s, vict %s, attacktype %d, dualing %d\r\n", dam, GET_NAME(ch), GET_NAME(vict), attacktype, dualing);
        }

        /* do we have armor that can stop a blow? */
        struct obj_data *armor = GET_EQ(vict, WEAR_BODY);
        int armor_val = -1;
        if (armor)
          armor_val = GET_OBJ_VAL(armor, 1); /* armor type */

        /* insert more colorful defensive messages here */

        /* shield block */
        if ((shield = GET_EQ(vict, WEAR_SHIELD)) && !rand_number(0, 3))
        {
          return_value = SKILL_MESSAGE_MISS_SHIELDBLOCK;

          send_to_char(ch, CCYEL(ch, C_CMP));
          act("$N blocks your attack with $p!", FALSE, ch, shield, vict, TO_CHAR);
          send_to_char(ch, CCNRM(ch, C_CMP));
          send_to_char(vict, CCRED(vict, C_CMP));
          act("You block $n's attack with $p!", FALSE, ch, shield, vict, TO_VICT | TO_SLEEP);
          send_to_char(vict, CCNRM(vict, C_CMP));
          act("$N blocks $n's attack with $p!", FALSE, ch, shield, vict, TO_NOTVICT);

          /* fire any shieldblock specs we might have */
          name = obj_index[GET_OBJ_RNUM(shield)].func;
          if (name)
            (name)(vict, shield, 0, "shieldblock");

          /* parry */
        }
        else if (opponent_weapon && !rand_number(0, 2))
        {
          return_value = SKILL_MESSAGE_MISS_PARRY;

          send_to_char(ch, CCYEL(ch, C_CMP));
          act("$N parries your attack with $p!", FALSE, ch, opponent_weapon, vict, TO_CHAR);
          send_to_char(ch, CCNRM(ch, C_CMP));
          send_to_char(vict, CCRED(vict, C_CMP));
          act("You parry $n's attack with $p!", FALSE, ch, opponent_weapon, vict, TO_VICT | TO_SLEEP);
          send_to_char(vict, CCNRM(vict, C_CMP));
          act("$N parries $n's attack with $p!", FALSE, ch, opponent_weapon, vict, TO_NOTVICT);

          /* fire any parry specs we might have */
          name = obj_index[GET_OBJ_RNUM(opponent_weapon)].func;
          if (name)
            (name)(vict, opponent_weapon, 0, "parry");

          /* glance off armor */
        }
        else if (armor && armor_list[armor_val].armorType > ARMOR_TYPE_NONE &&
                 !rand_number(0, 2))
        {
          return_value = SKILL_MESSAGE_MISS_GLANCE;
          send_to_char(ch, CCYEL(ch, C_CMP));
          act("Your attack glances off $p, protecting $N!", FALSE, ch, armor, vict, TO_CHAR);
          send_to_char(ch, CCNRM(ch, C_CMP));
          send_to_char(vict, CCRED(vict, C_CMP));
          act("$n's attack glances off $p!", FALSE, ch, armor, vict, TO_VICT | TO_SLEEP);
          send_to_char(vict, CCNRM(vict, C_CMP));
          act("$n's attack glances off $p, protecting $N!", FALSE, ch, armor, vict, TO_NOTVICT);

          /* fire any glance specs we might have */
          name = obj_index[GET_OBJ_RNUM(armor)].func;
          if (name)
            (name)(vict, armor, 0, "glance");
        }
        else
        { /* we fell through to generic miss message from file */
          return_value = SKILL_MESSAGE_MISS_GENERIC;
          /* default to miss messages in-file */
          if (msg->miss_msg.attacker_msg)
          {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }
          send_to_char(vict, CCRED(vict, C_CMP));
          act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
          send_to_char(vict, CCNRM(vict, C_CMP));
          act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);

          /* fire any dodge specs we might have, right now its only on weapons */
          if (opponent_weapon)
          {
            name = obj_index[GET_OBJ_RNUM(opponent_weapon)].func;
            if (name)
            {
              (name)(vict, opponent_weapon, 0, "dodge");
            }
          }
        }
      }
      return (return_value);
    } /* attacktype check */
  }   /* for loop for damage messages */

  return (return_value); /* did not find a message to use? */
}
#undef TRELUX_CLAWS

// this is just like damage reduction, except applies to certain type

int compute_energy_absorb(struct char_data *ch, int dam_type)
{
  int dam_reduction = 0;

  /* universal bonuses */
  if (HAS_FEAT(ch, FEAT_ENERGY_RESISTANCE))
    dam_reduction += HAS_FEAT(ch, FEAT_ENERGY_RESISTANCE);

  switch (dam_type)
  {
  case DAM_FIRE:
    if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
      dam_reduction += 3;
    if (affected_by_spell(ch, SPELL_PROTECTION_FROM_ENERGY))
      dam_reduction += get_char_affect_modifier(ch, SPELL_PROTECTION_FROM_ENERGY, APPLY_SPECIAL);
    break;
  case DAM_COLD:
    if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
      dam_reduction += 3;
    if (affected_by_spell(ch, SPELL_PROTECTION_FROM_ENERGY))
      dam_reduction += get_char_affect_modifier(ch, SPELL_PROTECTION_FROM_ENERGY, APPLY_SPECIAL);
    if (HAS_FEAT(ch, FEAT_VAMPIRE_ENERGY_RESISTANCE) && CAN_USE_VAMPIRE_ABILITY(ch))
      dam_reduction += 10;
    break;
  case DAM_AIR:
    if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
      dam_reduction += 3;
    if (affected_by_spell(ch, SPELL_PROTECTION_FROM_ENERGY))
      dam_reduction += get_char_affect_modifier(ch, SPELL_PROTECTION_FROM_ENERGY, APPLY_SPECIAL);
    break;
  case DAM_EARTH:
    if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
      dam_reduction += 3;
    if (affected_by_spell(ch, SPELL_PROTECTION_FROM_ENERGY))
      dam_reduction += get_char_affect_modifier(ch, SPELL_PROTECTION_FROM_ENERGY, APPLY_SPECIAL);
    break;
  case DAM_ACID:
    if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
      dam_reduction += 3;
    if (affected_by_spell(ch, SPELL_PROTECTION_FROM_ENERGY))
      dam_reduction += get_char_affect_modifier(ch, SPELL_PROTECTION_FROM_ENERGY, APPLY_SPECIAL);
    break;
  case DAM_HOLY:
    break;
  case DAM_ELECTRIC:
    if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
      dam_reduction += 3;
    if (affected_by_spell(ch, SPELL_PROTECTION_FROM_ENERGY))
      dam_reduction += get_char_affect_modifier(ch, SPELL_PROTECTION_FROM_ENERGY, APPLY_SPECIAL);
    if (HAS_FEAT(ch, FEAT_VAMPIRE_ENERGY_RESISTANCE) && CAN_USE_VAMPIRE_ABILITY(ch))
      dam_reduction += 10;
    break;
  case DAM_UNHOLY:
    if (AFF_FLAGGED(ch, AFF_DEATH_WARD))
      dam_reduction += 10;
    break;
  case DAM_SLICE:
    break;
  case DAM_PUNCTURE:
    break;
  case DAM_FORCE:
    break;
  case DAM_SOUND:
    if (affected_by_spell(ch, SPELL_PROTECTION_FROM_ENERGY))
      dam_reduction += get_char_affect_modifier(ch, SPELL_PROTECTION_FROM_ENERGY, APPLY_SPECIAL);
    break;
  case DAM_POISON:
  case DAM_CELESTIAL_POISON:
    break;
  case DAM_DISEASE:
    break;
  case DAM_NEGATIVE:
    if (AFF_FLAGGED(ch, AFF_DEATH_WARD))
      dam_reduction += 20;
    break;
  case DAM_ILLUSION:
    break;
  case DAM_MENTAL:
    break;
  case DAM_LIGHT:
    break;
  case DAM_ENERGY:
    if (affected_by_spell(ch, SPELL_RESIST_ENERGY))
      dam_reduction += 3;
    if (affected_by_spell(ch, SPELL_PROTECTION_FROM_ENERGY))
      dam_reduction += get_char_affect_modifier(ch, SPELL_PROTECTION_FROM_ENERGY, APPLY_SPECIAL);
    break;
  default:
    break;
  }

  return (MIN(MAX_ENERGY_ABSORB, dam_reduction));
}

// can return negative values, which indicates vulnerability (this is percent)
// dam_ defines are in spells.h
int compute_damtype_reduction(struct char_data *ch, int dam_type)
{
  int damtype_reduction = 0;

  /* base resistance */
  damtype_reduction += GET_RESISTANCES(ch, dam_type);

  /* universal bonsues */

  if (HAS_FEAT(ch, FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS) && draconic_heritage_energy_types[GET_BLOODLINE_SUBTYPE(ch)] == dam_type)
  {
    damtype_reduction += 100; // full immunity
  }

  if (IS_LICH(ch))
  {
    if (dam_type == DAM_COLD)
      damtype_reduction += 100; // full immunity
    if (dam_type == DAM_ELECTRIC)
      damtype_reduction += 100; // full immunity
  }

  if (HAS_FEAT(ch, FEAT_RAGE_RESISTANCE) && affected_by_spell(ch, SKILL_RAGE))
  {
    damtype_reduction += 15;
  }

  if (HAS_FEAT(ch, FEAT_RESISTANCE))
  {
    damtype_reduction += CLASS_LEVEL(ch, CLASS_CLERIC) / 6;
  }

  if (HAS_FEAT(ch, FEAT_DRACONIC_HERITAGE_DRAGON_RESISTANCES) && draconic_heritage_energy_types[GET_BLOODLINE_SUBTYPE(ch)] == dam_type)
  {
    damtype_reduction += CLASS_LEVEL(ch, CLASS_SORCERER) >= 9 ? 10 : 5;
  }

  switch (dam_type)
  {

  case DAM_FIRE:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_HALF_TROLL)
      damtype_reduction += -50;
    if (!IS_NPC(ch) && (GET_RACE(ch) == RACE_WHITE_DRAGON || GET_DISGUISE_RACE(ch) == RACE_WHITE_DRAGON))
      damtype_reduction += -50;
    if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
      damtype_reduction += 10;
    if (affected_by_spell(ch, SPELL_COLD_SHIELD))
      damtype_reduction += 50;
    if (is_judgement_possible(ch, FIGHTING(ch), INQ_JUDGEMENT_RESISTANCE))
      damtype_reduction += get_judgement_bonus(ch, INQ_JUDGEMENT_RESISTANCE);

    if (HAS_FEAT(ch, FEAT_DOMAIN_FIRE_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 20)
      damtype_reduction += 50;
    else if (HAS_FEAT(ch, FEAT_DOMAIN_FIRE_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 12)
      damtype_reduction += 20;
    else if (HAS_FEAT(ch, FEAT_DOMAIN_FIRE_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 6)
      damtype_reduction += 10;

    /* npc vulnerabilities/strengths */
    if (GET_NPC_RACE(ch) == RACE_TYPE_ELEMENTAL &&
        HAS_SUBRACE(ch, SUBRACE_FIRE))
      damtype_reduction += 100;
    if (IS_NPC(ch))
    {
      if (HAS_SUBRACE(ch, SUBRACE_FIRE))
        damtype_reduction += 50;
      if (HAS_SUBRACE(ch, SUBRACE_SWARM))
        damtype_reduction -= 50;
    }
    if (GET_RACE(ch) == RACE_RED_DRAGON || GET_DISGUISE_RACE(ch) == RACE_RED_DRAGON)
      damtype_reduction += 100;
    if (IS_EFREETI(ch))
      damtype_reduction += 100;
    break;

  case DAM_COLD:

    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += -20;
    if (!IS_NPC(ch) && (GET_RACE(ch) == RACE_RED_DRAGON || GET_DISGUISE_RACE(ch) == RACE_RED_DRAGON))
      damtype_reduction += -50;
    if (IS_EFREETI(ch))
      damtype_reduction -= 50;
    if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
      damtype_reduction += 10;
    if (AFF_FLAGGED(ch, AFF_FSHIELD))
      damtype_reduction += 50;
    if (HAS_FEAT(ch, FEAT_DEATHS_GIFT))
    {
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 9)
        damtype_reduction += 40;
      else
        damtype_reduction += 20;
    }
    if (HAS_FEAT(ch, FEAT_ONE_OF_US))
      damtype_reduction += 100;

    if (HAS_FEAT(ch, FEAT_DOMAIN_COLD_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 20)
      damtype_reduction += 50;
    else if (HAS_FEAT(ch, FEAT_DOMAIN_COLD_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 12)
      damtype_reduction += 20;
    else if (HAS_FEAT(ch, FEAT_DOMAIN_COLD_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 6)
      damtype_reduction += 10;

    /* npc vulnerabilities/strengths */
    if (GET_NPC_RACE(ch) == RACE_TYPE_ELEMENTAL &&
        HAS_SUBRACE(ch, SUBRACE_FIRE))
      damtype_reduction -= 100;
    if (IS_NPC(ch))
    {
      if (HAS_SUBRACE(ch, SUBRACE_FIRE))
        damtype_reduction -= 50;
      if (HAS_SUBRACE(ch, SUBRACE_REPTILIAN))
        damtype_reduction -= 25;
    }
    if (GET_RACE(ch) == RACE_WHITE_DRAGON || GET_DISGUISE_RACE(ch) == RACE_WHITE_DRAGON)
      damtype_reduction += 100;
    if (is_judgement_possible(ch, FIGHTING(ch), INQ_JUDGEMENT_RESISTANCE))
      damtype_reduction += get_judgement_bonus(ch, INQ_JUDGEMENT_RESISTANCE);
    break;

  case DAM_AIR:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
      damtype_reduction += 10;

    /* npc vulnerabilities/strengths */
    if (GET_NPC_RACE(ch) == RACE_TYPE_ELEMENTAL &&
        HAS_SUBRACE(ch, SUBRACE_AIR))
      damtype_reduction += 100;
    if (IS_NPC(ch))
    {
      if (HAS_SUBRACE(ch, SUBRACE_AIR))
        damtype_reduction += 50;
    }

    break;

  case DAM_EARTH:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
      damtype_reduction += 10;
    if (affected_by_spell(ch, SPELL_ACID_SHEATH))
      damtype_reduction += 50;

    /* npc vulnerabilities/strengths */
    if (GET_NPC_RACE(ch) == RACE_TYPE_ELEMENTAL &&
        HAS_SUBRACE(ch, SUBRACE_EARTH))
      damtype_reduction += 100;
    if (IS_NPC(ch))
    {
      if (HAS_SUBRACE(ch, SUBRACE_EARTH))
        damtype_reduction += 50;
    }

    break;

  case DAM_ACID:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_HALF_TROLL)
      damtype_reduction += -25;
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_CRYSTAL_DWARF)
      damtype_reduction += 10;
    if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
      damtype_reduction += 10;

    if (HAS_FEAT(ch, FEAT_DOMAIN_ACID_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 20)
      damtype_reduction += 50;
    else if (HAS_FEAT(ch, FEAT_DOMAIN_ACID_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 12)
      damtype_reduction += 20;
    else if (HAS_FEAT(ch, FEAT_DOMAIN_ACID_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 6)
      damtype_reduction += 10;

    /* npc vulnerabilities/strengths */
    if (GET_NPC_RACE(ch) == RACE_TYPE_ELEMENTAL &&
        HAS_SUBRACE(ch, SUBRACE_EARTH))
      damtype_reduction += 50;
    if (IS_NPC(ch))
    {
      if (HAS_SUBRACE(ch, SUBRACE_EARTH))
        damtype_reduction += 25;
    }
    if (GET_RACE(ch) == RACE_BLACK_DRAGON || GET_DISGUISE_RACE(ch) == RACE_BLACK_DRAGON)
      damtype_reduction += 100;
    if (is_judgement_possible(ch, FIGHTING(ch), INQ_JUDGEMENT_RESISTANCE))
      damtype_reduction += get_judgement_bonus(ch, INQ_JUDGEMENT_RESISTANCE);
    break;

  case DAM_HOLY:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
      if (GET_NPC_RACE(ch) == RACE_TYPE_UNDEAD)
        damtype_reduction -= 50;
      if (HAS_SUBRACE(ch, SUBRACE_ANGEL))
        damtype_reduction += 50;
      if (HAS_SUBRACE(ch, SUBRACE_ARCHON))
        damtype_reduction += 50;
      if (HAS_SUBRACE(ch, SUBRACE_EVIL))
        damtype_reduction -= 50;
      if (HAS_SUBRACE(ch, SUBRACE_GOOD))
        damtype_reduction += 50;
    }

    break;

  case DAM_ELECTRIC:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
      damtype_reduction += 10;

    if (HAS_FEAT(ch, FEAT_DOMAIN_ELECTRIC_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 20)
      damtype_reduction += 50;
    else if (HAS_FEAT(ch, FEAT_DOMAIN_ELECTRIC_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 12)
      damtype_reduction += 20;
    else if (HAS_FEAT(ch, FEAT_DOMAIN_ELECTRIC_RESIST) && CLASS_LEVEL(ch, CLASS_CLERIC) >= 6)
      damtype_reduction += 10;

    /* npc vulnerabilities/strengths */
    if (GET_NPC_RACE(ch) == RACE_TYPE_ELEMENTAL &&
        HAS_SUBRACE(ch, SUBRACE_WATER))
      damtype_reduction -= 100;
    if (IS_NPC(ch))
    {
      if (HAS_SUBRACE(ch, SUBRACE_WATER))
        damtype_reduction -= 50;
    }
    if (GET_RACE(ch) == RACE_BLUE_DRAGON || GET_DISGUISE_RACE(ch) == RACE_BLUE_DRAGON)
      damtype_reduction += 100;
    if (is_judgement_possible(ch, FIGHTING(ch), INQ_JUDGEMENT_RESISTANCE))
      damtype_reduction += get_judgement_bonus(ch, INQ_JUDGEMENT_RESISTANCE);
    break;

  case DAM_UNHOLY:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
      if (GET_NPC_RACE(ch) == RACE_TYPE_UNDEAD)
        damtype_reduction += 75;
      if (HAS_SUBRACE(ch, SUBRACE_ANGEL))
        damtype_reduction -= 50;
      if (HAS_SUBRACE(ch, SUBRACE_ARCHON))
        damtype_reduction -= 50;
      if (HAS_SUBRACE(ch, SUBRACE_EVIL))
        damtype_reduction += 50;
      if (HAS_SUBRACE(ch, SUBRACE_GOOD))
        damtype_reduction -= 50;
    }

    break;

  case DAM_SLICE:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
    }

    break;

  case DAM_PUNCTURE:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_CRYSTAL_DWARF)
      damtype_reduction += 10;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
    }

    break;

  case DAM_FORCE:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
    }

    break;

  case DAM_SOUND:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
      if (GET_NPC_RACE(ch) == RACE_TYPE_OOZE)
        damtype_reduction += 50;
    }

    if (is_judgement_possible(ch, FIGHTING(ch), INQ_JUDGEMENT_RESISTANCE))
      damtype_reduction += get_judgement_bonus(ch, INQ_JUDGEMENT_RESISTANCE);

    break;

  case DAM_POISON:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_CRYSTAL_DWARF)
      damtype_reduction += 10;
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_HALF_TROLL)
      damtype_reduction += 25;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
      if (GET_NPC_RACE(ch) == RACE_TYPE_CONSTRUCT)
        damtype_reduction += 50;
      if (GET_NPC_RACE(ch) == RACE_TYPE_PLANT)
        damtype_reduction += 50;
      if (GET_NPC_RACE(ch) == RACE_TYPE_OUTSIDER && IS_EVIL(ch))
        damtype_reduction += 75;
      if (GET_NPC_RACE(ch) == RACE_TYPE_UNDEAD)
        damtype_reduction += 75;
    }
    if (GET_RACE(ch) == RACE_GREEN_DRAGON || GET_DISGUISE_RACE(ch) == RACE_GREEN_DRAGON)
      damtype_reduction += 100;
    if (HAS_FEAT(ch, FEAT_POISON_IMMUNITY) || HAS_FEAT(ch, FEAT_SOUL_OF_THE_FEY))
      damtype_reduction += 100;
    break;

  case DAM_CELESTIAL_POISON:
    // celestial poisons are not resisted by undead or evil outsiders

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
      if (GET_NPC_RACE(ch) == RACE_TYPE_CONSTRUCT)
        damtype_reduction += 50;
      if (GET_NPC_RACE(ch) == RACE_TYPE_PLANT)
        damtype_reduction += 50;
    }

    break;

  case DAM_DISEASE:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_CRYSTAL_DWARF)
      damtype_reduction += 10;
    if (HAS_FEAT(ch, FEAT_STRONG_AGAINST_DISEASE))
      damtype_reduction += 50;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
      if (GET_NPC_RACE(ch) == RACE_TYPE_CONSTRUCT)
        damtype_reduction += 50;
      if (GET_NPC_RACE(ch) == RACE_TYPE_UNDEAD)
        damtype_reduction += 50;
    }

    break;

  case DAM_NEGATIVE:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_TIMELESS_BODY))
      damtype_reduction += 25;
    //      if (AFF_FLAGGED(ch, AFF_SHADOW_SHIELD))
    //        damtype_reduction += 100;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
      if (GET_NPC_RACE(ch) == RACE_TYPE_CONSTRUCT)
        damtype_reduction += 50;
      if (GET_NPC_RACE(ch) == RACE_TYPE_UNDEAD)
        damtype_reduction += 75;
      if (HAS_SUBRACE(ch, SUBRACE_ANGEL))
        damtype_reduction -= 25;
      if (HAS_SUBRACE(ch, SUBRACE_ARCHON))
        damtype_reduction -= 25;
    }

    break;

  case DAM_ILLUSION:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
      if (GET_NPC_RACE(ch) == RACE_TYPE_CONSTRUCT)
        damtype_reduction += 50;
      if (GET_NPC_RACE(ch) == RACE_TYPE_OOZE)
        damtype_reduction += 50;
      if (GET_NPC_RACE(ch) == RACE_TYPE_PLANT)
        damtype_reduction += 50;
      if (GET_NPC_RACE(ch) == RACE_TYPE_UNDEAD)
        damtype_reduction += 50;
    }

    break;

  case DAM_MENTAL:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
      if (GET_NPC_RACE(ch) == RACE_TYPE_CONSTRUCT)
        damtype_reduction += 50;
      if (GET_NPC_RACE(ch) == RACE_TYPE_UNDEAD)
        damtype_reduction += 50;
    }

    break;

  case DAM_LIGHT:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
      if (GET_NPC_RACE(ch) == RACE_TYPE_UNDEAD)
        damtype_reduction -= 25;
    }

    break;

  case DAM_ENERGY:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
      damtype_reduction += 10;

    /* npc vulnerabilities/strengths */
    if (IS_NPC(ch))
    {
    }

    break;

  case DAM_WATER:
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_TRELUX)
      damtype_reduction += 20;
    if (affected_by_spell(ch, SPELL_ENDURE_ELEMENTS))
      damtype_reduction += 10;

    /* npc vulnerabilities/strengths */
    if (GET_NPC_RACE(ch) == RACE_TYPE_ELEMENTAL &&
        HAS_SUBRACE(ch, SUBRACE_WATER))
      damtype_reduction += 100;
    if (IS_NPC(ch))
    {
      if (HAS_SUBRACE(ch, SUBRACE_WATER))
        damtype_reduction += 50;
    }

    break;

  default:
    break;
  }

  /* caps */
  if (damtype_reduction < -999)
    damtype_reduction = -999; /* 10x vulnerability? */
  if (damtype_reduction > 100)
    damtype_reduction = 100; /* complete resistance */

  return damtype_reduction;
}

/* this is straight damage reduction, applies to ALL attacks
 (not melee exclusive damage reduction) */
int compute_damage_reduction(struct char_data *ch, int dam_type)
{
  int damage_reduction = 0;

  if (char_has_mud_event(ch, eCRYSTALBODY_AFF))
    damage_reduction += 3;

  //  if (CLASS_LEVEL(ch, CLASS_BERSERKER))
  //    damage_reduction += CLASS_LEVEL(ch, CLASS_BERSERKER) / 4;

  //  if (AFF_FLAGGED(ch, AFF_SHADOW_SHIELD))
  //    damage_reduction += 12;

  if (HAS_FEAT(ch, FEAT_PERFECT_SELF)) /* temporary mechanic until we upgrade this system */
    damage_reduction += 3;

  if (HAS_FEAT(ch, FEAT_SOUL_OF_THE_FEY)) /* temporary mechanic until we upgrade this system */
    damage_reduction += 3;

  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_RP_HEAVY_SHRUG) && affected_by_spell(ch, SKILL_RAGE))
    damage_reduction += 3;

  if (HAS_FEAT(ch, FEAT_IMMOBILE_DEFENSE) && affected_by_spell(ch, SKILL_DEFENSIVE_STANCE))
    damage_reduction += 1;

  if (HAS_FEAT(ch, FEAT_ARMOR_MASTERY) && (GET_EQ(ch, WEAR_BODY) || GET_EQ(ch, WEAR_SHIELD)))
    damage_reduction += 5;

  /* armor specialization, doesn't stack */
  if (HAS_FEAT(ch, FEAT_ARMOR_SPECIALIZATION_HEAVY) &&
      compute_gear_armor_type(ch) == ARMOR_TYPE_HEAVY)
    damage_reduction += 2;
  else if (HAS_FEAT(ch, FEAT_ARMOR_SPECIALIZATION_MEDIUM) &&
           compute_gear_armor_type(ch) == ARMOR_TYPE_MEDIUM)
    damage_reduction += 2;
  else if (HAS_FEAT(ch, FEAT_ARMOR_SPECIALIZATION_LIGHT) &&
           compute_gear_armor_type(ch) == ARMOR_TYPE_LIGHT)
    damage_reduction += 2;

  /* this is now in the new system, study sets it up */
  // if (HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION))
  // damage_reduction += HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION) * 3;

  if (HAS_FEAT(ch, FEAT_SHRUG_DAMAGE))
    damage_reduction += HAS_FEAT(ch, FEAT_SHRUG_DAMAGE);

  if (affected_by_spell(ch, SPELL_EPIC_MAGE_ARMOR))
    damage_reduction += 6;

  if (IS_IRON_GOLEM(ch))
    damage_reduction += 10;
  if (IS_PIXIE(ch))
    damage_reduction += 10;
  if (IS_DRAGON(ch))
    damage_reduction += 5;
  if (IS_LICH(ch))
    damage_reduction += 4;

  if (HAS_FEAT(ch, FEAT_ONE_OF_US))
    damage_reduction += 5;
  else if (HAS_FEAT(ch, FEAT_DEATHS_GIFT))
  {
    if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 9)
      damage_reduction += 2;
    else
      damage_reduction += 1;
  }

  if (HAS_FEAT(ch, FEAT_AURA_OF_DEPRAVITY))
    damage_reduction += 1;
  if (HAS_FEAT(ch, FEAT_AURA_OF_RIGHTEOUSNESS))
    damage_reduction += 1;

  if (HAS_FEAT(ch, FEAT_HOLY_WARRIOR))
    damage_reduction += 2;
  if (HAS_FEAT(ch, FEAT_HOLY_CHAMPION))
    damage_reduction += 2;
  if (HAS_FEAT(ch, FEAT_UNHOLY_WARRIOR))
    damage_reduction += 2;
  if (HAS_FEAT(ch, FEAT_UNHOLY_CHAMPION))
    damage_reduction += 2;

  if (IS_SHADOW_CONDITIONS(ch) && HAS_FEAT(ch, FEAT_SHADOW_MASTER))
    damage_reduction += 5;

  if (GET_EQ(ch, WEAR_BODY) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_BODY)) == ITEM_ARMOR &&
      ((GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_BODY)) == MATERIAL_DRAGONHIDE) ||
       (GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_BODY)) == MATERIAL_DRAGONSCALE) ||
       (GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_BODY)) == MATERIAL_DRAGONBONE)))
    damage_reduction += 1;
  if (GET_EQ(ch, WEAR_HEAD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HEAD)) == ITEM_ARMOR &&
      ((GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_HEAD)) == MATERIAL_DRAGONHIDE) ||
       (GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_HEAD)) == MATERIAL_DRAGONSCALE) ||
       (GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_HEAD)) == MATERIAL_DRAGONBONE)))
    damage_reduction += 1;
  if (GET_EQ(ch, WEAR_ARMS) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_ARMS)) == ITEM_ARMOR &&
      ((GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_ARMS)) == MATERIAL_DRAGONHIDE) ||
       (GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_ARMS)) == MATERIAL_DRAGONSCALE) ||
       (GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_ARMS)) == MATERIAL_DRAGONBONE)))
    damage_reduction += 1;
  if (GET_EQ(ch, WEAR_LEGS) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_LEGS)) == ITEM_ARMOR &&
      ((GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_LEGS)) == MATERIAL_DRAGONHIDE) ||
       (GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_LEGS)) == MATERIAL_DRAGONSCALE) ||
       (GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_LEGS)) == MATERIAL_DRAGONBONE)))
    damage_reduction += 1;
  if (GET_EQ(ch, WEAR_SHIELD) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_SHIELD)) == ITEM_ARMOR &&
      ((GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_SHIELD)) == MATERIAL_DRAGONHIDE) ||
       (GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_SHIELD)) == MATERIAL_DRAGONSCALE) ||
       (GET_OBJ_MATERIAL(GET_EQ(ch, WEAR_SHIELD)) == MATERIAL_DRAGONBONE)))
    damage_reduction += 1;

  if (is_judgement_possible(ch, FIGHTING(ch), INQ_JUDGEMENT_RESILIENCY))
    damage_reduction += get_judgement_bonus(ch, INQ_JUDGEMENT_RESILIENCY);

  // damage reduction cap is 20
  return (MIN(MAX_DAM_REDUC, damage_reduction));
}

/* this is straight avoidance percentage, applies to ALL attacks
 (not exclusive to just melee attacks) */
int compute_concealment(struct char_data *ch)
{
  int concealment = 0;
  int concealment_cap = 0; /* vanish can push you over */

  /* these shouldn't stack, should be sorted from top to bottom by highest concealment % */
  if (AFF_FLAGGED(ch, AFF_DISPLACE))
    concealment += 50;
  else if (AFF_FLAGGED(ch, AFF_BLINKING))
    concealment += 20;
  else if (affected_by_spell(ch, PSIONIC_CONCEALING_AMORPHA)) // this is here to prevent overpowered combinations of buffs
    concealment += 20;
  else if (AFF_FLAGGED(ch, AFF_BLUR))
    concealment += 20;

  if (ROOM_AFFECTED(IN_ROOM(ch), RAFF_OBSCURING_MIST))
    concealment += 20;
  if (HAS_FEAT(ch, FEAT_OUTSIDER))
    concealment += 15;
  if (HAS_FEAT(ch, FEAT_SELF_CONCEALMENT))
    concealment += HAS_FEAT(ch, FEAT_SELF_CONCEALMENT) * 10;

  // concealment cap is 50% normally
  concealment_cap = MIN(MAX_CONCEAL, concealment);

  /* vanish can push us over */
  if (char_has_mud_event(ch, eVANISH))
  {
    concealment_cap += 25;
    if (HAS_FEAT(ch, FEAT_IMPROVED_VANISH))
      concealment_cap += 75;
  }

  return (concealment_cap);
}

/* this function lets damage_handling know that the given attacktype
   is VALID for being handled, otherwise ignore this attack */
bool ok_damage_handling(int attacktype)
{
  switch (attacktype)
  {
  case TYPE_SUFFERING:
    return FALSE;
  case SKILL_BASH:
    return FALSE;
  case SKILL_TRIP:
    return FALSE;
  case SPELL_POISON:
    return FALSE;
  case SPELL_SPIKE_GROWTH:
    return FALSE;
  case SKILL_CHARGE:
    return FALSE;
  case SKILL_BODYSLAM:
    return FALSE;
  case SKILL_SPRINGLEAP:
    return FALSE;
  case SKILL_SHIELD_PUNCH:
    return FALSE;
  case SKILL_SHIELD_CHARGE:
    return FALSE;
  case SKILL_SHIELD_SLAM:
    return FALSE;
  case SKILL_DIRT_KICK:
    return FALSE;
  case SKILL_SAP:
    return FALSE;
  }
  return TRUE;
}

/* returns modified damage, process elements/resistance/avoidance
   -1 means we're gonna go ahead and exit damage()
   anything that goes through here will affect ALL damage, whether
   skill or spell, etc */
int damage_handling(struct char_data *ch, struct char_data *victim,
                    int dam, int attacktype, int dam_type)
{
  bool is_spell = FALSE;
  struct obj_data *weapon = NULL;
  weapon = is_using_ranged_weapon(ch, true);
  int damage_reduction = 0;
  float damtype_reduction = 0;

  /* lets figure out if this attacktype is magical or not */
  if (is_spell_or_spell_like(attacktype))
    is_spell = TRUE;

  /* checking for actual damage, acceptable attacktypes and other dummy checks */
  if (dam > 0 && ok_damage_handling(attacktype) && victim != ch)
  {

    if (dam_type == DAM_POISON && !can_poison(victim))
      return 0;
    /* handle concealment */
    int concealment = compute_concealment(victim);
    // seeking weapons (ranged weapons only) bypass concealment always
    if (weapon && OBJ_FLAGGED(weapon, ITEM_SEEKING))
      concealment = 0;
    if (affected_by_spell(ch, PSIONIC_INEVITABLE_STRIKE))
      concealment = 0;
    if (dice(1, 100) <= compute_concealment(victim) && !is_spell)
    {
      if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
        send_to_char(victim, "\tW<conceal:%d>\tn", concealment);
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
        send_to_char(ch, "\tR<oconceal:%d>\tn", concealment);
      return 0;
    }

    /* trelux racial dodge */
    if (GET_RACE(victim) == RACE_TRELUX && !rand_number(0, 4) && !is_spell)
    {
      send_to_char(victim, "\tWYou leap away avoiding the attack!\tn\r\n");
      send_to_char(ch, "\tRYou fail to cause %s any harm as he leaps away!\tn\r\n",
                   GET_NAME(victim));
      act("$n fails to harm $N as $S leaps away!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      return -1;
    }

    /* stalwart defender */
    if (HAS_FEAT(victim, FEAT_LAST_WORD) && !rand_number(0, 9) && !is_spell)
    {
      send_to_char(victim, "\tWYour defensive stance holds firm against the onlsaught!\tn\r\n");
      send_to_char(ch, "\tRYou fail to cause %s any harm as the defensive stance holds firms!\tn\r\n",
                   GET_NAME(victim));
      act("$n fails to harm $N as the defensive stance holds firm!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      return -1;
    }

    /* mirror image gives (1 / (# of image + 1)) chance of hitting */
    /* Don't allow mirror image to absorb spells - Danavan 2018-04-09 */
    if (!is_spell && (affected_by_spell(victim, SPELL_MIRROR_IMAGE) || affected_by_spell(victim, SPELL_GREATER_MIRROR_IMAGE)) && dam > 0)
    {
      if (GET_IMAGES(victim) > 0)
      {
        if (rand_number(0, GET_IMAGES(victim)))
        {
          send_to_char(victim, "\tWOne of your images is destroyed!\tn\r\n");
          send_to_char(ch, "\tRYou have struck an illusionary "
                           "image of %s!\tn\r\n",
                       GET_NAME(victim));
          act("$n struck an illusionary image of $N!", FALSE, ch, 0, victim,
              TO_NOTVICT);
          GET_IMAGES(victim)
          --;
          if (GET_IMAGES(victim) <= 0)
          {
            send_to_char(victim, "\t2All of your illusionary "
                                 "images are gone!\tn\r\n");
            affect_from_char(victim, SPELL_MIRROR_IMAGE);
            affect_from_char(victim, SPELL_GREATER_MIRROR_IMAGE);
          }
          return -1;
        }
      }
      else
      {
        // dummy check
        send_to_char(victim, "\t2All of your illusionary "
                             "images are gone!\tn\r\n");
        affect_from_char(victim, SPELL_MIRROR_IMAGE);
        affect_from_char(victim, SPELL_GREATER_MIRROR_IMAGE);
      }
    }

    // we'll apply the cedit configuration for altered spell damage
    if (is_spell && !IS_NPC(ch) && GET_CASTING_CLASS(ch) != CLASS_UNDEFINED)
    {
      switch (GET_CASTING_CLASS(ch))
      {
      case CLASS_WIZARD:
      case CLASS_SORCERER:
      case CLASS_BARD:
        dam = dam * (100 + CONFIG_ARCANE_DAMAGE) / 100;
        break;

      case CLASS_CLERIC:
      case CLASS_DRUID:
      case CLASS_PALADIN:
      case CLASS_RANGER:
        dam = dam * (100 + CONFIG_DIVINE_DAMAGE) / 100;
        break;

      case CLASS_PSIONICIST:
        dam = dam * (100 + CONFIG_PSIONIC_DAMAGE) / 100;
        break;

      case CLASS_ALCHEMIST:
        // place holder in case we want to adjust alchemy damage down the line -- Gicker
        break;
      }
      // To ensure they don't get the bonus on subsequent spells/powers unless casting_class has been set again
      GET_CASTING_CLASS(ch) = CLASS_UNDEFINED;
    }

    else if (!is_spell && victim && IS_EVIL(victim) && group_member_affected_by_spell(ch, SPELL_LITANY_OF_RIGHTEOUSNESS) && has_aura_of_good(ch))
    {
      dam *= 2;
      if (has_aura_of_evil(victim) && !affected_by_spell(victim, SPELL_EFFECT_DAZZLED))
      {
        struct affected_type af;
        new_affect(&af);
        af.spell = SPELL_EFFECT_DAZZLED;
        af.location = APPLY_SPECIAL;
        af.modifier = 0;
        af.duration = dice(1, 4);
        SET_BIT_AR(af.bitvector, AFF_DAZZLED);
        affect_to_char(ch, &af);
        act("You are dazzled by $n's attack.", false, ch, 0, victim, TO_VICT);
        act("$N is dazzled by Your attack.", false, ch, 0, victim, TO_CHAR);
        act("$N is dazzled by $n's attack.", false, ch, 0, victim, TO_NOTVICT);
      }
    }

    // some damage types cannot be reduced or resisted, such as a vampire's blood drain ability
    if (can_dam_be_resisted(dam_type))
    {
      /* energy absorption system */
      absorb_energy_conversion(victim, dam_type, dam);
      damage_reduction = compute_energy_absorb(victim, dam_type);
      dam -= compute_energy_absorb(victim, dam_type);
    }
    if (dam <= 0 && (ch != victim))
    {
      send_to_char(victim, "\tWYou absorb all the damage! (%d)\tn\r\n", damage_reduction);
      send_to_char(ch, "\tRYou fail to cause %s any harm! (energy absorb: %d)\tn\r\n",
                   GET_NAME(victim), damage_reduction);
      act("$n fails to do any harm to $N!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      return -1;
    }
    else if (damage_reduction)
    {
      if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
        send_to_char(victim, "\tW<EA:%d>\tn", damage_reduction);
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
        send_to_char(ch, "\tR<oEA:%d>\tn", damage_reduction);
    }

    /* damage type PERCENTAGE reduction */
    // some damage types cannot be reduced or resisted, such as a vampire's blood drain ability
    if (can_dam_be_resisted(dam_type))
    {
      damtype_reduction = (float)compute_damtype_reduction(victim, dam_type);
      damtype_reduction = (((float)(damtype_reduction / 100.0)) * (float)dam);
      dam -= (int)damtype_reduction;
    }

    if (dam <= 0 && (ch != victim))
    {
      send_to_char(victim, "\tWYou absorb all the damage! (%d)\tn\r\n", (int)damtype_reduction);
      send_to_char(ch, "\tRYou fail to cause %s any harm! (dam-type reduction: %d)\tn\r\n",
                   GET_NAME(victim), (int)damtype_reduction);
      act("$n fails to do any harm to $N!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      return -1;
    }
    else if (damtype_reduction < 0)
    { // no reduction, vulnerability
      if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
        send_to_char(victim, "\tR<TR:%d>\tn", (int)damtype_reduction);
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
        send_to_char(ch, "\tW<oTR:%d>\tn", (int)damtype_reduction);
    }
    else if (damtype_reduction > 0)
    {
      if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
        send_to_char(victim, "\tW<TR:%d>\tn", (int)damtype_reduction);
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
        send_to_char(ch, "\tR<oTR:%d>\tn", (int)damtype_reduction);
    }

    /* damage reduction system (old version) */
    if (!is_spell)
    {
      // We want to handle incorporeal affect on damage before applying damage reduction
      // damage by a spell will always do full damage to incorporeal creatures
      // we halve damage against an incorporeal person in most circumstances
      if (IS_INCORPOREAL(victim))
      {
        // damage is normal if you're using a ghost touch weapon, or you're incorporeal yourself
        if (is_using_ghost_touch_weapon(ch) || IS_INCORPOREAL(ch))
          ;
        else
          dam /= 2;
      }
      // The same is also true.  If you are incorporeal and your foe isn't, you deal 1/2 damage
      // If you're using a ghost touch weapon you do full damage
      else if (IS_INCORPOREAL(ch))
      {
        // damage is normal if you're using a ghost touch weapon, or if your foe is also incorporeal
        if (is_using_ghost_touch_weapon(ch) || IS_INCORPOREAL(victim))
          ;
        else
          dam /= 2;
      }

      damage_reduction = compute_damage_reduction(victim, dam_type);
      dam -= MIN(dam, damage_reduction);
      if (!dam && (ch != victim))
      {
        send_to_char(victim, "\tWYou absorb all the damage! (%d)\tn\r\n", damage_reduction);
        send_to_char(ch, "\tRYou fail to cause %s any harm! (damaged reduction: %d)\tn\r\n",
                     GET_NAME(victim), damage_reduction);
        act("$n fails to do any harm to $N!", FALSE, ch, 0, victim,
            TO_NOTVICT);
        return -1;
      }
      else if (damage_reduction)
      {
        if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
          send_to_char(victim, "\tW<DR:%d>\tn", damage_reduction);
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
          send_to_char(ch, "\tR<oDR:%d>\tn", damage_reduction);
      }
    }

    /* inertial barrier - damage absorption using psp */
    if (AFF_FLAGGED(victim, AFF_INERTIAL_BARRIER) && dam && !rand_number(0, 1) && can_dam_be_resisted(dam_type))
    {
      send_to_char(ch, "\twYour attack is absorbed by some manner of invisible barrier.\tn\r\n");
      GET_PSP(victim) -= (1 + (dam / 5));
      if (GET_PSP(victim) <= 0)
      {
        // affect_from_char(victim, SPELL_INERTIAL_BARRIER);
        REMOVE_BIT_AR(AFF_FLAGS(victim), AFF_INERTIAL_BARRIER);
        send_to_char(victim, "Your mind can not maintain the barrier anymore.\r\n");
      }
      dam = 0;
    }

  } /* end big dummy check if() */

  return dam;
}

/* victim died at the hands of ch
 * this is called before die() to handle xp gain, corpse, memory and
 * a handful of other things */
int dam_killed_vict(struct char_data *ch, struct char_data *victim)
{
  char local_buf[MEDIUM_STRING] = {'\0'};
  long local_gold = 0, happy_gold = 0;
  struct char_data *tmp_char = NULL, *tch = NULL;
  struct obj_data *corpse_obj;
  room_rnum rnum = NOWHERE;

  if (!ok_damage_shopkeeper(ch, victim) || MOB_FLAGGED(victim, MOB_NOKILL) || !is_mission_mob(ch, victim))
  {
    send_to_char(ch, "This mob is immune to your attack.\r\n");
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    if (FIGHTING(victim) && FIGHTING(victim) == ch)
      stop_fighting(victim);
    return (0);
  }

  // checking to see if they've been having their blood drained by a vampire
  // so we know if the corpse can be used to create vampire spawn
  if (IS_NPC(victim) && IS_HUMANOID(victim) && IN_ROOM(victim) != NOWHERE && IN_ROOM(ch) != NOWHERE)
  {
    for (tch = world[IN_ROOM(victim)].people; tch; tch = tch->next_in_room)
    {
      if (FIGHTING(tch) == victim && IS_VAMPIRE(tch) && tch != victim && affected_by_spell(victim, ABILITY_BLOOD_DRAIN))
        victim->char_specials.drainKilled = TRUE;
    }
  }

  GET_POS(victim) = POS_DEAD;

  if (ch != victim && (IS_NPC(victim) || victim->desc))
  { // xp gain
    /* pets give xp to their master */
    if (IS_PET(ch) && ch->master && IN_ROOM(ch) == IN_ROOM(ch->master))
    {
      if (GROUP(ch->master))
        group_gain(ch->master, victim);
      else
        solo_gain(ch->master, victim);
    }
    else if (IS_NPC(ch) && ch->confuser_idnum > 0 && is_pc_idnum_in_room(ch, ch->confuser_idnum))
    {
      if (GROUP(ch))
        group_gain(ch, victim);
      else
        solo_gain(ch, victim);
    }
    else
    {
      if (GROUP(ch))
        group_gain(ch, victim);
      else
        solo_gain(ch, victim);
    }
  }

  resetCastingData(victim); // stop casting
  CLOUDKILL(victim) = 0;    // stop any cloudkill bursts
  DOOM(victim) = 0;         // stop any creeping doom
  INCENDIARY(victim) = 0;   // stop any incendiary bursts

  if (!IS_NPC(victim))
  { // forget victim, log
    mudlog(BRF, LVL_IMMORT, TRUE, "%s killed by %s (%d) at %s (%d)", GET_NAME(victim),
           GET_NAME(ch), GET_MOB_VNUM(ch), world[IN_ROOM(victim)].name, GET_ROOM_VNUM(IN_ROOM(victim)));
    if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_MEMORY))
      forget(ch, victim);
    if (IS_NPC(ch) && HUNTING(ch) == victim)
      HUNTING(ch) = NULL;
  }

  if (IN_ARENA(ch) || IN_ARENA(victim))
  {
    struct descriptor_data *pt = NULL;

    for (pt = descriptor_list; pt; pt = pt->next)
    {
      if (IS_PLAYING(pt) && pt->character)
      {
        if (GROUP(ch) && GROUP(ch)->members->iSize)
        {
          send_to_char(pt->character, "\tR[\tW%s\tR]\tn %s of %s's group has defeated %s in the Arena!\r\n",
                       MOB_FLAGGED(ch, MOB_HUNTS_TARGET) ? "Hunt" : "Info",
                       GET_NAME(ch), GET_NAME(ch->group->leader), GET_NAME(victim));
        }
        else if (IS_NPC(ch) && ch->master)
        {
          send_to_char(pt->character, "\tR[\tW%s\tR]\tn %s's follower has defeated %s in the Arena!\r\n",
                       MOB_FLAGGED(ch, MOB_HUNTS_TARGET) ? "Hunt" : "Info",
                       GET_NAME(ch->master), GET_NAME(victim));
        }
        else
        {
          send_to_char(pt->character, "\tR[\tW%s\tR]\tn %s has defeated %s in the Arena!\r\n",
                       MOB_FLAGGED(ch, MOB_HUNTS_TARGET) ? "Hunt" : "Info",
                       GET_NAME(ch), GET_NAME(victim));
        }
      }
    }
  }

  if (IS_NPC(victim))
  { // determine gold before corpse created
    if ((IS_HAPPYHOUR) && (IS_HAPPYGOLD))
    {
      happy_gold = (long)(GET_GOLD(victim) * (((float)(HAPPY_GOLD)) / (float)100));
      happy_gold = MAX(0, happy_gold);
      increase_gold(victim, happy_gold);
    }
    local_gold = GET_GOLD(victim);
    snprintf(local_buf, sizeof(local_buf), "%ld", (long)local_gold);
  }

  /* grab room number of victim before we extract him for corpse making */
  rnum = IN_ROOM(victim);

  /* corpse should be made here */
  die(victim, ch);

  /* todo: maybe make die() return a value to let us know if there really is a corpse */

  /* we make everyone in the room with auto-collect search for ammo here before
     any of the autolooting, etc */
  for (tch = world[rnum].people; tch; tch = tch->next_in_room)
  {
    if (!tch)
      continue;
    if (IS_NPC(tch))
    {
      if (tch->master && PRF_FLAGGED(tch->master, PRF_AUTOCOLLECT) && !IS_NPC(tch->master))
      {
        perform_collect(tch->master, FALSE);
        // attach_mud_event(new_mud_event(eCOLLECT_DELAY, ch, NULL), 1);
      }
      else
      {
        continue;
      }
    }
    if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_AUTOCOLLECT))
    {
      perform_collect(tch, FALSE);
      // attach_mud_event(new_mud_event(eCOLLECT_DELAY, ch, NULL), 1);
    }
  }

  // handle dead mob and PRF_

  /* auto split / auto gold */
  if (!IS_NPC(ch) && GROUP(ch) && (local_gold > 0) && PRF_FLAGGED(ch, PRF_AUTOSPLIT))
  {
    generic_find("corpse", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
    if (corpse_obj)
    {
      do_get(ch, "all.coin corpse", 0, 0);
      do_split(ch, local_buf, 0, 0);
    }
  }
  else if (IS_NPC(ch) && GROUP(ch) && (local_gold > 0) && ch->master && !IS_NPC(ch->master) && PRF_FLAGGED(ch->master, PRF_AUTOSPLIT))
  {
    generic_find("corpse", FIND_OBJ_ROOM, ch->master, &tmp_char, &corpse_obj);
    if (corpse_obj)
    {
      do_get(ch->master, "all.coin corpse", 0, 0);
      do_split(ch->master, local_buf, 0, 0);
    }
  }
  else if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOGOLD))
  {
    do_get(ch, "all.coin corpse", 0, 0);
    do_get(ch, "all.coin", 0, 0); // added for incorporeal - no corpse -zusuk
  }
  else if (IS_NPC(ch) && ch->master && (ch != victim) && (ch->master != victim) && !IS_NPC(ch->master) && PRF_FLAGGED(ch->master, PRF_AUTOGOLD))
  {
    do_get(ch->master, "all.coin corpse", 0, 0);
    do_get(ch->master, "all.coin", 0, 0); // added for incorporeal - no corpse -zusuk
  }

  /* autoloot */
  if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOLOOT))
  {
    do_get(ch, "all corpse", 0, 0);
    // do_get(ch, "all.coin", 0, 0);  //added for incorporeal - no corpse
  }
  else if (IS_NPC(ch) && (ch != victim) && ch->master && (IN_ROOM(ch) == IN_ROOM(ch->master)) && !IS_NPC(ch->master) && PRF_FLAGGED(ch->master, PRF_AUTOLOOT))
  {
    do_get(ch->master, "all corpse", 0, 0);
  }

  /* autosac */
  if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSAC))
  {
    do_sac(ch, "corpse", 0, 0);
  }
  else if (IS_NPC(ch) && (ch != victim) && ch->master && (IN_ROOM(ch) == IN_ROOM(ch->master)) && !IS_NPC(ch->master) && PRF_FLAGGED(ch->master, PRF_AUTOSAC))
  {
    do_sac(ch->master, "corpse", 0, 0);
  }

  /* all done! */
  return (-1);
}

// death < 0, no dam = 0, damage done > 0
/* ALLLLLL damage goes through this function */
/* probably need to bring in another variable letting us know our source, like:
   -melee attack
   -spell
   -item
   -etc */
/* if it's a spell, the spellnum will be carried through the w_type variable */
int damage(struct char_data *ch, struct char_data *victim, int dam,
           int w_type, int dam_type, int offhand)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  char buf1[MAX_INPUT_LENGTH] = {'\0'};
  bool is_ranged = FALSE;
  struct affected_type af;

  /* this is just a dummy check */
  if (!ch)
    return 0;
  if (!victim)
    return 0;

  if (offhand == 2)
    is_ranged = TRUE;

  if (GET_POS(victim) <= POS_DEAD)
  { // delayed extraction
    if (PLR_FLAGGED(victim, PLR_NOTDEADYET) ||
        MOB_FLAGGED(victim, MOB_NOTDEADYET))
      return (-1);
    log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
        GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    die(victim, ch);
    return (-1);
  }

  if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
      ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return (0);
  }

  if (!ok_damage_shopkeeper(ch, victim) || MOB_FLAGGED(victim, MOB_NOKILL) || !is_mission_mob(ch, victim))
  {
    send_to_char(ch, "This mob is protected.\r\n");
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    if (FIGHTING(victim) && FIGHTING(victim) == ch)
      stop_fighting(victim);
    return (0);
  }

  if (!IS_NPC(victim) && ((GET_LEVEL(victim) >= LVL_IMMORT) &&
                          PRF_FLAGGED(victim, PRF_NOHASSLE)))
    dam = 0; // immort protection

  if (victim != ch)
  {
    /* Only auto engage if both parties are unengaged. */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL) && (FIGHTING(victim) == NULL) && !is_wall_spell(w_type)) // ch -> vict
      set_fighting(ch, victim);

    // vict -> ch
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL) && !is_wall_spell(w_type))
    {
      set_fighting(victim, ch);
    }
    victim->last_attacker = ch;
  }
  else
  { // mainly for type_suffering, dying without awarding xp
    if (victim->last_attacker)
    {
      if (IN_ROOM(victim->last_attacker) == IN_ROOM(victim))
        ch = victim->last_attacker;
    }
  }

  /* pets leave if attacked */
  if (victim->master == ch)
    stop_follower(victim);

  /* if target is in your group, you forfeit your position in the group -zusuk */
  if (GROUP(ch) && GROUP(victim) && GROUP(ch) == GROUP(victim) && ch != victim)
  {
    leave_group(ch);
  }

  if (!CONFIG_PK_ALLOWED)
  { // PK check
    check_killer(ch, victim);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }

  /* add to memory if applicable */
  if (MOB_FLAGGED(victim, MOB_MEMORY) && CAN_SEE(victim, ch))
  {
    if (!IS_NPC(ch))
    {
      remember(victim, ch);
    }
    else if (IS_PET(ch) && ch->master && IN_ROOM(ch->master) == IN_ROOM(ch) && !IS_NPC(ch->master))
      remember(victim, ch->master); // help curb pet-fodder methods
  }

  /* set to hunting if applicable */
  if (MOB_FLAGGED(victim, MOB_HUNTER) && CAN_SEE(victim, ch) &&
      !HUNTING(victim))
  {
    if (!IS_NPC(ch))
    {
      HUNTING(victim) = ch;
    }
    else if (IS_PET(ch) && ch->master && IN_ROOM(ch->master) == IN_ROOM(ch) && !IS_NPC(ch->master))
      HUNTING(victim) = ch->master; // help curb pet-fodder methods
  }

  /* modify damage: concealment, trelux leap, mirror image, energey absorb
       damage-type reduction, old-skool damage reduction, inertial barrier */
  dam = damage_handling(ch, victim, dam, w_type, dam_type); // modify damage
  if (dam == -1)                                            // make sure message handling has been done!
    return 0;

  /* last word!  gonna die to this blow, SMACK the fool hard! */
  if (!IS_NPC(victim) && ((GET_HIT(victim) - dam) <= 0) &&
      HAS_FEAT(victim, FEAT_LAST_WORD) && affected_by_spell(victim, SKILL_DEFENSIVE_STANCE) &&
      !char_has_mud_event(victim, eLAST_WORD) && ch != victim)
  {
    act("\tWSensing the death blow targeting you, you unleash an attack of your own against"
        " \tn$N\tW!\tn",
        FALSE, victim, NULL, ch, TO_CHAR);
    act("$n \tRsensing death, unleashes a last attack!\tn",
        FALSE, victim, NULL, ch, TO_VICT | TO_SLEEP);
    act("$n sensing a \tWdeath blow\tn from $N, unleashes a final attack!\tn", FALSE, victim, NULL, ch, TO_NOTVICT);
    attach_mud_event(new_mud_event(eLAST_WORD, victim, NULL),
                     (2 * SECS_PER_MUD_DAY));
    if (ch && victim && IN_ROOM(ch) == IN_ROOM(victim) && GET_POS(ch) > POS_DEAD)
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    if (ch && victim && IN_ROOM(ch) == IN_ROOM(victim) && GET_POS(ch) > POS_DEAD)
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }

  /* defensive roll, avoids a lethal blow once every X minutes
   * X = about 7 minutes with current settings
   */
  if (!IS_NPC(victim) && ((GET_HIT(victim) - dam) <= 0) &&
      HAS_FEAT(victim, FEAT_DEFENSIVE_ROLL) &&
      !char_has_mud_event(victim, eD_ROLL) && ch != victim &&
      can_dam_be_resisted(dam_type))
  {
    act("\tWYou time a defensive roll perfectly and avoid the attack from"
        " \tn$N\tW!\tn",
        FALSE, victim, NULL, ch, TO_CHAR);
    act("$n \tRtimes a defensive roll perfectly and avoids your attack!\tn",
        FALSE, victim, NULL, ch, TO_VICT | TO_SLEEP);
    act("$n times a \tWdefensive roll\tn perfectly and avoids an attack "
        "from $N!\tn",
        FALSE, victim, NULL, ch, TO_NOTVICT);
    attach_mud_event(new_mud_event(eD_ROLL, victim, NULL),
                     (2 * SECS_PER_MUD_DAY));
    return 0;
  }

  /* lich rejuvenation, avoids and heals from a lethal blow once every X minutes
   * X = about 7 minutes with current settings - one game day
   */
  if (!IS_NPC(victim) && ((GET_HIT(victim) - dam) <= 0) &&
      HAS_FEAT(victim, FEAT_LICH_REJUV) &&
      !char_has_mud_event(victim, eLICH_REJUV) && ch != victim)
  {
    act("\tWYour phylactery explodes with power protecting you from the deadly attack from"
        " \tn$N\tW!\tn",
        FALSE, victim, NULL, ch, TO_CHAR);
    act("$n \tRcrumbles then quickly revives from the power of a phylactery as a result of your deadly attack!\tn",
        FALSE, victim, NULL, ch, TO_VICT | TO_SLEEP);
    act("$n crumbles in death but suddenly \tWflares with power from a phylactery\tn reviving in defiance of the deadly attack "
        "from $N!\tn",
        FALSE, victim, NULL, ch, TO_NOTVICT);

    attach_mud_event(new_mud_event(eLICH_REJUV, victim, NULL),
                     (2 * SECS_PER_MUD_DAY));

    GET_HIT(victim) = GET_MAX_HIT(victim);
    GET_MOVE(victim) = GET_MAX_MOVE(victim);

    return 0;
  }

  dam = MAX(MIN(dam, 1499), 0); // damage cap
  GET_HIT(victim) -= dam;

  // check for life shield spell
  if (victim && ch != victim && IS_UNDEAD(ch) && affected_by_spell(victim, SPELL_LIFE_SHIELD))
  {
    int threshold = get_char_affect_modifier(victim, SPELL_LIFE_SHIELD, APPLY_SPECIAL);
    int lifedam = 0;
    struct affected_type *af = NULL;
    bool remove_spell = false;

    if (threshold > dam / 2)
    {
      threshold -= dam / 2;
    }
    else
    {
      threshold = dam / 2;
    }
    lifedam = dam / 2;
    damage(victim, ch, lifedam, SPELL_LIFE_SHIELD, DAM_HOLY, FALSE);
    for (af = victim->affected; af; af = af->next)
    {
      if (af->spell == SPELL_LIFE_SHIELD && af->location == APPLY_SPECIAL)
      {
        af->modifier -= threshold;
        if (af->modifier <= 0)
        {
          remove_spell = true;
          break;
        }
      }
    }
    if (remove_spell)
    {
      affect_from_char(victim, SPELL_LIFE_SHIELD);
    }
  }

  /* xp gain for damage, limiting it more -zusuk */
  if (ch != victim && GET_EXP(victim) && (GET_LEVEL(ch) - GET_LEVEL(victim)) <= 3)
    gain_exp(ch, GET_LEVEL(victim) * dam, GAIN_EXP_MODE_DAMAGE);

  if (!dam)
    update_pos(victim);
  else
    update_pos_dam(victim);

  if (dam)
  { // display damage done
    snprintf(buf1, sizeof(buf1), "[%d]", dam);
    snprintf(buf, sizeof(buf), "%5s", buf1);
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
      send_to_char(ch, "\tW%s\tn ", buf);
    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
      send_to_char(victim, "\tR%s\tn ", buf);
  }

  if (DEBUGMODE)
  {
    send_to_char(victim, "In damage() function, Position: %d, HP: %d, DAM: %d, Attacker %s, You: %s\r\n",
                 GET_POS(victim), GET_HIT(victim), dam, GET_NAME(ch), GET_NAME(victim));
    int weapon_type = w_type - TOP_ATTACK_TYPES;
    if (weapon_type < 0 || weapon_type >= NUM_ATTACK_TYPES)
    {
      send_to_char(ch, "Weapon-type: %d!!", weapon_type);
    }
    else
    {
      send_to_char(ch, "Weapon-type: %s", attack_hit_types[weapon_type]);
    }
    send_to_char(ch, ", Dam-type: %s, Offhand: %d (2==ranged)\r\n",
                 damtypes[dam_type], offhand);
  }

  /** DAMAGE MESSAGES
      Two systems:
        1) skill_message:  these are called from file, if the damage being
           done is via a SKILL_ or SPELL_ missed attacks or death blows we
           should be using this function
        2) dam_message:  these are our backup messages, this function should
           be primarily used for weapon attacks (non-miss and non-deathblow)
   **/
  /* if our weapon type is -1 that means the damage is not from a weapon OR
     skill - an example is damage caused by being bucked off a mount */
  if (w_type != -1)
  {

    /* IS_WEAPON is simply defined as a 'normal' weapon-type value */
    if (!IS_WEAPON(w_type))
    {
      /* OK we now know this is not a weapon type, it should be either a
       SKILL_ or SPELL_ */
      if (!skill_message(dam, ch, victim, w_type, offhand))
      {
        /* somehow there is no SKILL_ or SPELL_ message for this damage
           so we have a fallback message here */
        act("$n winces in visible pain.", TRUE, victim, 0, 0, TO_ROOM);
        send_to_char(victim, "You wince in pain!\r\n");
        // if (FIGHTING(victim))
        // send_to_char(FIGHTING(victim), "%s winces in pain! %d\r\n", GET_NAME(victim), w_type);
      }

      /* we now should be handling damage done via weapons */
    }
    else
    {

      /* if the damage fails (miss) or the attack finishes the victim... */
      if (GET_POS(victim) == POS_DEAD || dam == 0)
      {

        if (!dam && is_ranged)
        { // miss with ranged = dam_message()
          dam_message(dam, ch, victim, w_type, offhand);
        }
        else if (!skill_message(dam, ch, victim, w_type, offhand))
        {
          /* no skill_message? try dam_message */
          dam_message(dam, ch, victim, w_type, offhand);
        }

        /* landed a normal weapon attack hit */
      }
      else
      {
        dam_message(dam, ch, victim, w_type, offhand);
      }
    }
  }
  else
  {
    /* w_type is -1, ideally shouldn't arrive here, but got a fallback msg */
    act("$n winces in visible pain...", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You wince in pain!\r\n");
    // if (FIGHTING(victim))
    // send_to_char(FIGHTING(victim), "%s winces in pain! %d\r\n", GET_NAME(victim), w_type);
  }

  switch (GET_POS(victim))
  { // act() used in case someone is dead
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.",
        TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are mortally wounded, and will die "
                         "soon, if not aided.\r\n");
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.",
        TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are incapacitated and will slowly "
                         "die, if not aided.\r\n");
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.",
        TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You're stunned, but will probably regain "
                         "consciousness again.\r\n");
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are dead!  Sorry...\r\n");
    break;
  default:
    if (dam > (GET_MAX_HIT(victim) / 4))
      send_to_char(victim, "\tYThat really did \tRHURT\tY!\tn\r\n");
    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4))
    {
      send_to_char(victim, "\tnYou wish that your wounds would stop "
                           "\tRB\trL\tRE\trE\tRD\trI\tRN\trG\tn so much!\r\n");

      if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY)) // wimpy mobs
        /* mob dex check: mob_dex_bonus + 10 vs 1d20 */
        if ((GET_DEX_BONUS(victim) + 10) > dice(1, 20))
          if (GET_HIT(victim) > 0)
            if (!IS_CASTING(victim) && GET_POS(victim) >= POS_FIGHTING)
              if (IN_ROOM(ch) == IN_ROOM(victim) && !IS_CASTING(victim))
                perform_flee(victim);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) && // pc wimpy
        GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0 &&
        IN_ROOM(ch) == IN_ROOM(victim) && !IS_CASTING(victim) &&
        GET_POS(victim) >= POS_FIGHTING)
    {
      send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
      perform_flee(victim);
    }
    break;
  } // end SWITCH

  // linkless, very abusable, so trying with this off
  /*
  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) {
    perform_flee(victim);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = IN_ROOM(victim);
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }
   */

  // too hurt to continue
  if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
    stop_fighting(victim);

  // lose hide/invis
  if (AFF_FLAGGED(ch, AFF_INVISIBLE) || AFF_FLAGGED(ch, AFF_HIDE))
    appear(ch, FALSE);

  if (GET_POS(victim) == POS_DEAD) // victim died
  {
    /* psionic assimilate affect */
    if (w_type == PSIONIC_ASSIMILATE)
    {

      /* if we have this affection, remove it first so it doesn't restack */
      if (affected_by_spell(ch, PSIONIC_ASSIMILATE))
        GET_HIT(ch) -= get_char_affect_modifier(ch, PSIONIC_ASSIMILATE, APPLY_HIT);
      GET_HIT(ch) = MAX(1, GET_HIT(ch));
      affect_from_char(ch, PSIONIC_ASSIMILATE);

      /* ok now build the affection */
      new_affect(&af);
      af.spell = PSIONIC_ASSIMILATE;
      af.location = APPLY_HIT;
      af.modifier = 4 * GET_LEVEL(victim);
      af.duration = 600;
      af.bonus_type = BONUS_TYPE_CIRCUMSTANCE; /* stacks */
      GET_HIT(ch) += af.modifier + GET_LEVEL(victim);
      affect_to_char(ch, &af); /* apply affection! */

      /* message for flavor */
      act("You fully assimilate the form of $N gaining some of $S power.", false, ch, 0, victim, TO_CHAR);
      act("$n fully assimilates your form, gaining some of your power.", false, ch, 0, victim, TO_VICT);
      act("$n fully assimilates the form of $N gaining some of $S power.", false, ch, 0, victim, TO_NOTVICT);
    }

    return (dam_killed_vict(ch, victim));
  }

  return (dam);
}

/* you are going to arrive here from an attack, or viewing mode
 * We have two functions: compute_hit_damage() and compute_damage_bonus() that
 * both basically will compute how much damage a given hit will do or display
 * how much damage potential you have (attacks command).  What is the difference?
 *   Compute_hit_damage() basically calculates bonus damage that will not be
 * displayed, compute_damage_bonus() calculates bonus damage that will be
 * displayed.  compute_hit_damage() always calls compute_damage_bonus() */

/* #define MODE_NORMAL_HIT       0
   #define MODE_DISPLAY_PRIMARY  2
   #define MODE_DISPLAY_OFFHAND  3
   #define MODE_DISPLAY_RANGED   4
 * Valid attack_type(s) are:
 *   ATTACK_TYPE_PRIMARY       : Primary hand attack.
 *   ATTACK_TYPE_OFFHAND       : Offhand attack.
 *   ATTACK_TYPE_RANGED        : Ranged attack.
 *   ATTACK_TYPE_UNARMED       : Unarmed attack.
 *   ATTACK_TYPE_TWOHAND       : Two-handed weapon attack.
 *   ATTACK_TYPE_BOMB_TOSS     : Alchemist - tossing bombs
 *   ATTACK_TYPE_PRIMARY_SNEAK : impromptu sneak attack, primary hand
 *   ATTACK_TYPE_OFFHAND_SNEAK : impromptu sneak attack, offhand */
/* using w_type -1 as a display mode */
int compute_damage_bonus(struct char_data *ch, struct char_data *vict,
                         struct obj_data *wielded, int w_type, int mod, int mode, int attack_type)
{
  int dambonus = mod;
  bool display_mode = FALSE;
  int str_bonus = GET_STR_BONUS(ch);
  char strength[20];

  if (w_type == -1)
    display_mode = TRUE;

  /* redundancy necessary due to sometimes arriving here without going through
   * compute_hit_damage()*/
  if (attack_type == ATTACK_TYPE_UNARMED || IS_WILDSHAPED(ch) || IS_MORPHED(ch))
    wielded = NULL;
  else
    wielded = get_wielded(ch, attack_type);

  if (wielded && is_using_light_weapon(ch, wielded) && OBJ_FLAGGED(wielded, ITEM_AGILE))
  {
    str_bonus = MAX(get_agile_weapon_dex_bonus(ch), GET_STR_BONUS(ch));
    sprintf(strength, "Dexterity (Agile Weapon)");
  }
  else
  {
    sprintf(strength, "Strength");
  }

  /* damroll (should be mostly just gear, spell affections) */
  dambonus += GET_DAMROLL(ch);
  if (display_mode)
    send_to_char(ch, "Damroll: \tR%d\tn\r\n", GET_DAMROLL(ch));

  if (affected_by_aura_of_sin(ch))
  {
    dambonus++;
    if (display_mode)
      send_to_char(ch, "Aura of Sin: \tR1\tn\r\n");
  }

  if (affected_by_aura_of_faith(ch))
  {
    dambonus++;
    if (display_mode)
      send_to_char(ch, "Aura of Faith: \tR1\tn\r\n");
  }

  if (AFF_FLAGGED(ch, AFF_SICKENED))
  {
    dambonus -= 2;
    if (display_mode)
      send_to_char(ch, "Sickened Status: \tR-2\tn\r\n");
  }

  if (is_judgement_possible(ch, vict, INQ_JUDGEMENT_DESTRUCTION))
  {
    dambonus += get_judgement_bonus(ch, INQ_JUDGEMENT_DESTRUCTION);
    if (display_mode)
      send_to_char(ch, "Judgement of Destruction: \tR%d\tn\r\n", get_judgement_bonus(ch, INQ_JUDGEMENT_DESTRUCTION));
  }

  /* strength bonus */
  switch (attack_type)
  {

  case ATTACK_TYPE_PRIMARY:
  case ATTACK_TYPE_PRIMARY_SNEAK:
    if (affected_by_spell(ch, SKILL_DRHRT_CLAWS))
    {
      dambonus += str_bonus;
      if (display_mode)
        send_to_char(ch, "%s from Claws: \tR%d\tn\r\n", strength, str_bonus);
    }
    else if (!IS_WILDSHAPED(ch) && !IS_MORPHED(ch) && GET_EQ(ch, WEAR_WIELD_2H) && !is_using_double_weapon(ch) && !OBJ_FLAGGED(wielded, ITEM_AGILE))
    {
      dambonus += str_bonus * 3 / 2; /* 2handed weapon */
      if (display_mode)
        send_to_char(ch, "%s from 2Hand Weapon: \tR%d\tn\r\n", strength, str_bonus * 3 / 2);
    }
    else if (hands_available(ch) > 0)
    {
      dambonus += str_bonus + 2; /* one handed weapon held in two hands because of empty off hand */
      if (display_mode)
        send_to_char(ch, "%s from 1Hand Weapon, free offhand: \tR%d\tn\r\n", strength, str_bonus + 2);
    }
    /*
    else if (GET_EQ(ch, WEAR_WIELD_2H))
    {
      if (display_mode)
        send_to_char(ch, "Two-hand strength bonus: \tR%d\tn\r\n", str_bonus * 3 / 2);
      dambonus += str_bonus * 3 / 2; // 2handed weapon
    }
    */
    else
    {
      dambonus += str_bonus;
      if (display_mode)
        send_to_char(ch, "%s bonus: \tR%d\tn\r\n", strength, str_bonus);
    }
    break;

  case ATTACK_TYPE_OFFHAND:
  case ATTACK_TYPE_OFFHAND_SNEAK:
    dambonus += str_bonus / 2;
    if (display_mode)
      send_to_char(ch, "Offhand %s bonus: \tR%d\tn\r\n", strength, str_bonus / 2);
    break;

  case ATTACK_TYPE_RANGED:

    /* strength penalties DO apply to ranged weapons */
    if (str_bonus <= 0)
    {
      dambonus += str_bonus;
      if (display_mode)
        send_to_char(ch, "ranged strength penalty: \tR%d\tn\r\n", GET_STR_BONUS(ch));
    }
    else
    {
      /* some ranged weapons get various strength bonus */
      if (wielded)
      {
        if (OBJ_FLAGGED(wielded, ITEM_ADAPTIVE))
          dambonus += str_bonus;
        else
        {
          switch (GET_OBJ_VAL(wielded, 0))
          {
          case WEAPON_TYPE_COMPOSITE_SHORTBOW:
          case WEAPON_TYPE_COMPOSITE_LONGBOW:
            if (display_mode)
              send_to_char(ch, "Comp bow (1) bonus: \tR%d\tn\r\n", MIN(1, str_bonus));
            dambonus += MIN(1, str_bonus);
            break;
          case WEAPON_TYPE_COMPOSITE_LONGBOW_2:
          case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
            if (display_mode)
              send_to_char(ch, "Comp bow (2) bonus: \tR%d\tn\r\n", MIN(2, str_bonus));
            dambonus += MIN(2, str_bonus);
            break;
          case WEAPON_TYPE_COMPOSITE_LONGBOW_3:
          case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
            if (display_mode)
              send_to_char(ch, "Comp bow (3) bonus: \tR%d\tn\r\n", MIN(3, str_bonus));
            dambonus += MIN(3, str_bonus);
            break;
          case WEAPON_TYPE_COMPOSITE_LONGBOW_4:
          case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
            if (display_mode)
              send_to_char(ch, "Comp bow (4) bonus: \tR%d\tn\r\n", MIN(4, str_bonus));
            dambonus += MIN(4, str_bonus);
            break;
          case WEAPON_TYPE_COMPOSITE_LONGBOW_5:
          case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
            if (display_mode)
              send_to_char(ch, "Comp bow (5) bonus: \tR%d\tn\r\n", MIN(5, str_bonus));
            dambonus += MIN(5, str_bonus);
            break;
          case WEAPON_TYPE_SLING:
            if (display_mode)
              send_to_char(ch, "Sling strength bonus: \tR%d\tn\r\n", str_bonus);
            dambonus += str_bonus;
            break;
          default:
            break; /* nope, no bonus */
          }
        }
      }
    }

    if (vict && IN_ROOM(ch) == IN_ROOM(vict))
    {
      if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_POINT_BLANK_SHOT))
      {
        if (display_mode)
          send_to_char(ch, "Point Blank Shot bonus: \tR1\tn\r\n");
        dambonus++;
      }
    }
    break;

  case ATTACK_TYPE_UNARMED:
    if (display_mode)
      send_to_char(ch, "Unarmed %s bonus: \tR%d\tn\r\n", strength, str_bonus);
    dambonus += str_bonus;
    break;

  case ATTACK_TYPE_TWOHAND:
    if (wielded && OBJ_FLAGGED(wielded, ITEM_AGILE))
      break;
    if (display_mode)
      send_to_char(ch, "Two-hand strength bonus: \tR%d\tn\r\n", str_bonus * 3 / 2);
    dambonus += str_bonus * 3 / 2; /* 2handed weapon */
    break;

  default:
    break;
  }

  // Sorcerer Draconic Bloodline Claw Attacks
  if (ch && vict && affected_by_spell(ch, SKILL_DRHRT_CLAWS) && CLASS_LEVEL(ch, CLASS_SORCERER) >= 11)
  {
    if (display_mode)
      send_to_char(ch, "Draconic claw elemental damage bonus: \tR%d\tn\r\n",
                   add_draconic_claws_elemental_damage(ch, vict));
    dambonus += add_draconic_claws_elemental_damage(ch, vict);
  }

  /* penalties */

  /* Circumstance penalty */
  switch (GET_POS(ch))
  {
  case POS_SITTING:
  case POS_RESTING:
  case POS_SLEEPING:
  case POS_STUNNED:
  case POS_INCAP:
  case POS_MORTALLYW:
  case POS_DEAD:
    dambonus -= 2;
    if (display_mode)
      send_to_char(ch, "Position penalty: \tR-2\tn\r\n");
    break;
  case POS_FIGHTING:
  case POS_STANDING:
  default:
    break;
  }

  /* fatigued */
  if (AFF_FLAGGED(ch, AFF_FATIGUED))
  {
    if (display_mode)
      send_to_char(ch, "Fatigue penalty: \tR-2\tn\r\n");
    dambonus -= 2;
  }

  /* current implementation of intimidate */
  if (char_has_mud_event(ch, eINTIMIDATED))
  {
    if (display_mode)
      send_to_char(ch, "Intimidate penalty: \tR-6\tn\r\n");
    dambonus -= 6;
  }

  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
  {
    if (display_mode)
      send_to_char(ch, "Grapple/Entangle penalty: \tR-2\tn\r\n");
    dambonus -= 2;
  }

  /* end penalties */

  /* size */
  dambonus += size_modifiers[GET_SIZE(ch)];
  if (display_mode)
    send_to_char(ch, "Size modifier: \tR%d\tn\r\n", size_modifiers[GET_SIZE(ch)]);

  if (IN_ROOM(ch) != NOWHERE && ROOM_AFFECTED(IN_ROOM(ch), RAFF_SACRED_SPACE) && IS_EVIL(ch))
  {
    dambonus -= 1;
    if (display_mode)
      send_to_char(ch, "Sacred Space Effect: \tR-1\tn\r\n");
  }

  /* weapon specialist */
  if (HAS_FEAT(ch, FEAT_WEAPON_SPECIALIZATION))
  {
    /* Check the weapon type, make sure it matches. */
    if (((wielded != NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_SPECIALIZATION), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily)) ||
        ((wielded == NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_SPECIALIZATION), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily)))
    {
      if (display_mode)
        send_to_char(ch, "Weapon specializiation: \tR2\tn\r\n");
      dambonus += 2;
    }
  }

  if (!display_mode)
  {
    // Assassin stuff
    dambonus += GET_MARK_DAM_BONUS(ch);
    GET_MARK_DAM_BONUS(ch) = 0;
  }

  if (vict)
  {
    if (HAS_FEAT(ch, FEAT_ALIGNED_ATTACK_GOOD) && IS_GOOD(vict))
      dambonus += 2;
    else if (HAS_FEAT(ch, FEAT_ALIGNED_ATTACK_EVIL) && IS_EVIL(vict))
      dambonus += 2;
    else if (HAS_FEAT(ch, FEAT_ALIGNED_ATTACK_CHAOS) && IS_CHAOTIC(vict))
      dambonus += 2;
    else if (HAS_FEAT(ch, FEAT_ALIGNED_ATTACK_LAW) && IS_LAWFUL(vict))
      dambonus += 2;
  }

  if (HAS_FEAT(ch, FEAT_GREATER_WEAPON_SPECIALIZATION))
  {
    /* Check the weapon type, make sure it matches. */
    if (((wielded != NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_GREATER_WEAPON_SPECIALIZATION), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily)) ||
        ((wielded == NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_GREATER_WEAPON_SPECIALIZATION), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily)))
    {
      if (display_mode)
        send_to_char(ch, "Greater weapon specializiation: \tR4\tn\r\n");
      dambonus += 4;
    }
  }

  if (HAS_FEAT(ch, FEAT_EPIC_WEAPON_SPECIALIZATION))
  {
    /* Check the weapon type, make sure it matches. */
    if (((wielded != NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_EPIC_WEAPON_SPECIALIZATION), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily)) ||
        ((wielded == NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_EPIC_WEAPON_SPECIALIZATION), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily)))
    {
      if (display_mode)
        send_to_char(ch, "Greater weapon specializiation: \tR3\tn\r\n");
      dambonus += 3;
    }
  }

  /* weapon enhancement bonus */
  if (wielded)
  {
    // greater magic weapon
    if (affected_by_spell(ch, SPELL_GREATER_MAGIC_WEAPON))
    {
      if (display_mode)
        send_to_char(ch, "Weapon enhancement bonus: \tR%d\tn\r\n", MAX(GET_ENHANCEMENT_BONUS(wielded), get_char_affect_modifier(ch, SPELL_GREATER_MAGIC_WEAPON, APPLY_SPECIAL)));
      dambonus += MAX(GET_ENHANCEMENT_BONUS(wielded), get_char_affect_modifier(ch, SPELL_GREATER_MAGIC_WEAPON, APPLY_SPECIAL));
    }
    else
    {
      if (display_mode)
        send_to_char(ch, "Weapon enhancement bonus: \tR%d\tn\r\n", GET_ENHANCEMENT_BONUS(wielded));
      dambonus += GET_ENHANCEMENT_BONUS(wielded);
    }
  }
  else if (affected_by_spell(ch, SPELL_GREATER_MAGIC_WEAPON))
  {
    if (display_mode)
      send_to_char(ch, "Unarmed enhancement bonus: \tR%d\tn\r\n", get_char_affect_modifier(ch, SPELL_GREATER_MAGIC_WEAPON, APPLY_SPECIAL));
    dambonus += get_char_affect_modifier(ch, SPELL_GREATER_MAGIC_WEAPON, APPLY_SPECIAL);
  }

  /* monk glove enhancement bonus */
  if (MONK_TYPE(ch) && is_bare_handed(ch) && monk_gear_ok(ch) &&
      GET_EQ(ch, WEAR_HANDS) && GET_OBJ_VAL(GET_EQ(ch, WEAR_HANDS), 0))
  {
    if (display_mode)
      send_to_char(ch, "Monk glove enhancement bonus: \tR%d\tn\r\n", GET_OBJ_VAL(GET_EQ(ch, WEAR_HANDS), 0));
    dambonus += GET_OBJ_VAL(GET_EQ(ch, WEAR_HANDS), 0);
  }

  /* lich touch */
  if (is_bare_handed(ch) && IS_LICH(ch))
  {
    if (display_mode)
      send_to_char(ch, "Lich free hands bonus: \tR%d\tn\r\n", GET_LEVEL(ch) / 2);
    dambonus += GET_LEVEL(ch) / 2;
  }

  /* ranged includes arrow enhancement bonus + special ranged bonus to favored enemies with the epic favored enemy feat */
  if (can_fire_ammo(ch, TRUE))
  {
    if (display_mode)
      send_to_char(ch, "Ammo enhancement bonus: \tR%d\tn\r\n",
                   GET_ENHANCEMENT_BONUS(GET_EQ(ch, WEAR_AMMO_POUCH)->contains));
    dambonus += GET_ENHANCEMENT_BONUS(GET_EQ(ch, WEAR_AMMO_POUCH)->contains);

    if (HAS_FEAT(ch, FEAT_ENHANCE_ARROW_MAGIC) && display_mode)
      send_to_char(ch, "Enhance ammo magic bonus: \tR%d\tn\r\n", HAS_FEAT(ch, FEAT_ENHANCE_ARROW_MAGIC));
    dambonus += HAS_FEAT(ch, FEAT_ENHANCE_ARROW_MAGIC);

    /* favored enemy */
    if (vict && vict != ch && !IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_RANGER))
    {
      // checking if we have humanoid favored enemies for PC victims
      if (!IS_NPC(vict) && IS_FAV_ENEMY_OF(ch, RACE_TYPE_HUMANOID))
      {

        if (HAS_FEAT(ch, FEAT_EPIC_FAVORED_ENEMY))
        {
          if (display_mode)
            send_to_char(ch, "Epic favored enemy ranged dex bonus: \tR%d\tn\r\n", GET_DEX_BONUS(ch));
          dambonus += GET_DEX_BONUS(ch);
        }
      }
      else if (IS_NPC(vict) && IS_FAV_ENEMY_OF(ch, GET_RACE(vict)))
      {

        if (HAS_FEAT(ch, FEAT_EPIC_FAVORED_ENEMY))
        {
          if (display_mode)
            send_to_char(ch, "Epic favored enemy ranged dex bonus: \tR%d\tn\r\n", GET_DEX_BONUS(ch));
          dambonus += GET_DEX_BONUS(ch);
        }
      }
    }
  }

  /* wildshape bonus */
  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
  {
    if (display_mode)
      send_to_char(ch, "Natural attack bonus: \tR%d\tn\r\n",
                   HAS_FEAT(ch, FEAT_NATURAL_ATTACK));
    dambonus += HAS_FEAT(ch, FEAT_NATURAL_ATTACK);
  }

  /*
  if (wielded && GET_OBJ_MATERIAL(wielded) == MATERIAL_ADAMANTINE)
      dambonus++;
   */

  if (wielded && GET_OBJ_MATERIAL(wielded) == MATERIAL_DRAGONBONE)
    dambonus += 2;

  /* power attack */
  if (AFF_FLAGGED(ch, AFF_POWER_ATTACK) && attack_type != ATTACK_TYPE_RANGED && attack_type != ATTACK_TYPE_BOMB_TOSS)
  {
    if (GET_EQ(ch, WEAR_WIELD_2H) && !is_using_double_weapon(ch))
    {
      dambonus += COMBAT_MODE_VALUE(ch) * 2; /* 2h weapons gets 2x bonus */
      if (display_mode)
        send_to_char(ch, "2h power attack bonus: \tR%d\tn\r\n",
                     COMBAT_MODE_VALUE(ch) * 2);
    }
    else
    {
      dambonus += COMBAT_MODE_VALUE(ch);
      if (display_mode)
        send_to_char(ch, "Power attack bonus: \tR%d\tn\r\n",
                     COMBAT_MODE_VALUE(ch));
    }
  }

  /* crystal fist */
  if (char_has_mud_event(ch, eCRYSTALFIST_AFF))
  {
    dambonus += 3;
    if (display_mode)
      send_to_char(ch, "Crystal fist bonus: \tR3\tn\r\n");
  }

  /* smite evil (remove after one attack) */
  if (affected_by_spell(ch, SKILL_SMITE_EVIL) && vict && IS_EVIL(vict))
  {
    if (display_mode)
      send_to_char(ch, "Smite Evil bonus: \tR%d\tn\r\n", CLASS_LEVEL(ch, CLASS_PALADIN));
    dambonus += CLASS_LEVEL(ch, CLASS_PALADIN) * smite_evil_target_type(vict);
  }
  /* smite good (remove after one attack) */
  if (affected_by_spell(ch, SKILL_SMITE_GOOD) && vict && IS_GOOD(vict))
  {
    if (display_mode)
      send_to_char(ch, "Smite Good bonus: \tR%d\tn\r\n", CLASS_LEVEL(ch, CLASS_BLACKGUARD));

    dambonus += CLASS_LEVEL(ch, CLASS_BLACKGUARD) * smite_good_target_type(vict);
  }
  /* destructive smite (remove after one attack) */
  if (affected_by_spell(ch, SKILL_SMITE_DESTRUCTION) && vict)
  {
    if (display_mode)
      send_to_char(ch, "Destructive Smite bonus: \tR%d\tn\r\n", (DIVINE_LEVEL(ch) / 2) + 1);
    dambonus += (DIVINE_LEVEL(ch) / 2) + 1;
    if (mode == MODE_NORMAL_HIT && !display_mode)
      affect_from_char(ch, SKILL_SMITE_DESTRUCTION);
  }

  /* favored enemy */
  if (vict && vict != ch && !IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_RANGER))
  {
    // checking if we have humanoid favored enemies for PC victims
    if (!IS_NPC(vict) && IS_FAV_ENEMY_OF(ch, RACE_TYPE_HUMANOID))
    {
      if (display_mode)
        send_to_char(ch, "Favored enemy bonus: \tR%d\tn\r\n", CLASS_LEVEL(ch, CLASS_RANGER) / 5 + 2);
      dambonus += CLASS_LEVEL(ch, CLASS_RANGER) / 3 + 2;

      if (HAS_FEAT(ch, FEAT_EPIC_FAVORED_ENEMY))
      {
        if (display_mode)
          send_to_char(ch, "Epic favored enemy bonus: \tR4\tn\r\n");
        dambonus += 6;
      }
    }
    else if (IS_NPC(vict) && IS_FAV_ENEMY_OF(ch, GET_RACE(vict)))
    {
      if (display_mode)
        send_to_char(ch, "Favored enemy bonus: \tR%d\tn\r\n", CLASS_LEVEL(ch, CLASS_RANGER) / 5 + 2);
      dambonus += CLASS_LEVEL(ch, CLASS_RANGER) / 3 + 2;

      if (HAS_FEAT(ch, FEAT_EPIC_FAVORED_ENEMY))
      {
        if (display_mode)
          send_to_char(ch, "Epic favored enemy bonus: \tR4\tn\r\n");
        dambonus += 6;
      }
    }
  }

  /* paladin's divine bond */
  /* maximum of 6 damage 1 + level / 3 (past level 5) */
  if (HAS_FEAT(ch, FEAT_DIVINE_BOND))
  {
    if (display_mode)
      send_to_char(ch, "Divine bond bonus: \tR%d\tn\r\n",
                   MIN(6, 1 + MAX(0, (CLASS_LEVEL(ch, CLASS_PALADIN) - 5) / 3)));
    dambonus += MIN(6, 1 + MAX(0, (CLASS_LEVEL(ch, CLASS_PALADIN) - 5) / 3));
  }

  /* morale bonus */
  if (affected_by_spell(ch, SKILL_POWERFUL_BLOW))
  {
    if (display_mode)
      send_to_char(ch, "Powerful blow bonus: \tR%d\tn\r\n",
                   CLASS_LEVEL(ch, CLASS_BERSERKER));
    dambonus += CLASS_LEVEL(ch, CLASS_BERSERKER);
  } /* THIS IS JUST FOR SHOW, it gets taken out before the damage is calculated
     * the actual damage bonus is inserted in the damage code */

  /* if the victim is using 'come and get me' then they will be vulnerable */
  if (vict && affected_by_spell(vict, SKILL_COME_AND_GET_ME))
  {
    if (display_mode)
      send_to_char(ch, "Opponent has 'come and get me' bonus: \tR4\tn\r\n");
    dambonus += 4;
  }

  /* temporary filler for ki-strike until we get it working right */
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_KI_STRIKE))
  {
    if (display_mode)
      send_to_char(ch, "Ki Strike bonus: \tR%d\tn\r\n",
                   HAS_FEAT(ch, FEAT_KI_STRIKE));
    dambonus += HAS_FEAT(ch, FEAT_KI_STRIKE);
  }

  /* precise strike mechanic for duelist */
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_PRECISE_STRIKE) && HAS_FREE_HAND(ch) &&
      compute_gear_armor_type(ch) <= ARMOR_TYPE_LIGHT)
  {
    if (display_mode)
      send_to_char(ch, "Precise Strike bonus: \tR%d\tn\r\n",
                   MAX(1, CLASS_LEVEL(ch, CLASS_DUELIST)));
  }
  if (vict && !IS_IMMUNE_CRITS(vict) && !IS_NPC(ch) &&
      HAS_FEAT(ch, FEAT_PRECISE_STRIKE) && HAS_FREE_HAND(ch) &&
      compute_gear_armor_type(ch) <= ARMOR_TYPE_LIGHT)
  {
    dambonus += MAX(1, CLASS_LEVEL(ch, CLASS_DUELIST));
  }

  /* light blindness - dayblind, underdark/underworld penalties */
  if (!IS_NPC(ch) && IS_DAYLIT(IN_ROOM(ch)) && HAS_FEAT(ch, FEAT_LIGHT_BLINDNESS))
  {
    dambonus -= 1;
    if (display_mode)
      send_to_char(ch, "Dayblind penalty: \tR-1\tn\r\n");
  }

  /* trelux pincers bonus */
  if (GET_RACE(ch) == RACE_TRELUX)
  {
    dambonus += GET_LEVEL(ch) / 5;
  }

  /****************************************/
  /**** display, keep mods above this *****/
  /****************************************/
  if (display_mode)
  {
    send_to_char(ch, "\tYTotal Damage Bonus:  \tR**%d**\tn\r\n\r\n", dambonus);
  }
  else if (mode != MODE_NORMAL_HIT)
  {
    send_to_char(ch, "Dam Bonus:  %d\r\n\r\n", dambonus);
  }

  return (MIN(MAX_DAM_BONUS, dambonus));
}

/* when unarmed, this is how we handle weapon dice */
void compute_barehand_dam_dice(struct char_data *ch, int *diceOne, int *diceTwo)
{
  if (!ch)
    return;

  int monkLevel = MONK_TYPE(ch);

  if (IS_NPC(ch))
  {
    *diceOne = ch->mob_specials.damnodice;
    *diceTwo = ch->mob_specials.damsizedice;
  }
  else
  {
    if (monkLevel && monk_gear_ok(ch))
    { // monk?
      if (monkLevel < 3)
      {
        *diceOne = 1;
        *diceTwo = 6;
      }
      else if (monkLevel < 6)
      {
        *diceOne = 1;
        *diceTwo = 8;
      }
      else if (monkLevel < 9)
      {
        *diceOne = 1;
        *diceTwo = 10;
      }
      else if (monkLevel < 12)
      {
        *diceOne = 2;
        *diceTwo = 6;
      }
      else if (monkLevel < 15)
      {
        *diceOne = 4;
        *diceTwo = 4;
      }
      else if (monkLevel < 18)
      {
        *diceOne = 4;
        *diceTwo = 5;
      }
      else if (monkLevel < 21)
      {
        *diceOne = 4;
        *diceTwo = 6;
      }
      else if (monkLevel < 24)
      {
        *diceOne = 5;
        *diceTwo = 6;
      }
      else if (monkLevel < 27)
      {
        *diceOne = 6;
        *diceTwo = 6;
      }
      else
      {
        *diceOne = 7;
        *diceTwo = 7;
      }
      if (GET_RACE(ch) == RACE_TRELUX)
      {
        /* at level 20 they get an extra die/roll */
        *diceOne = *diceOne + 1 + (GET_LEVEL(ch) / 20);
        *diceTwo = *diceTwo + 1 + (GET_LEVEL(ch) / 20);
      }
      if (IS_LICH(ch))
      {
        *diceOne = *diceOne + 1;
      }
    }

    // non-monk bare-hand damage

    else
    {

      if (GET_RACE(ch) == RACE_TRELUX)
      {
        if (affected_by_spell(ch, PSIONIC_OAK_BODY) || affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
        {
          *diceOne = 3 + (GET_LEVEL(ch) / 20);
          *diceTwo = 6 + (GET_LEVEL(ch) / 20);
        }
        else
        {
          *diceOne = 2 + (GET_LEVEL(ch) / 20);
          *diceTwo = 6 + (GET_LEVEL(ch) / 20);
        }
      }

      else if (IS_LICH(ch))
      {
        if (affected_by_spell(ch, PSIONIC_OAK_BODY) || affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
        {
          *diceOne = 3;
          *diceTwo = 4;
        }
        else
        {
          *diceOne = 2;
          *diceTwo = 4;
        }
      }

      else
      {
        if (affected_by_spell(ch, PSIONIC_OAK_BODY) || affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
        {
          *diceOne = 1;
          *diceTwo = 6;
        }
        else
        {
          *diceOne = 1;
          *diceTwo = 2;
        }
      }
    }
  }
}

/* TODO! */

/*
int crit_range_extension(struct char_data *ch, struct obj_data *weap) {
  int ext = weap ? GET_OBJ_VAL(weap, VAL_WEAPON_CRITRANGE) + 1 : 1; // include 20
  int tp = weap ? GET_OBJ_VAL(weap, VAL_WEAPON_SKILL) : WEAPON_TYPE_UNARMED;
  int mult = 1;
  int imp_crit = FALSE;

  if (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_CRITICAL, tp) ||
      has_weapon_feat(ch, FEAT_IMPROVED_CRITICAL, tp))
    imp_crit = TRUE;

  if (AFF_FLAGGED(ch, AFF_KEEN_WEAPON)) {
    if (weap) {
      if (IS_SET(weapon_list[GET_OBJ_VAL(weap, 0)].damageTypes, DAMAGE_TYPE_SLASHING))
        imp_crit = TRUE;
      else if (IS_SET(weapon_list[GET_OBJ_VAL(weap, 0)].damageTypes, DAMAGE_TYPE_PIERCING))
        imp_crit = TRUE;
    } else if (IS_NPC(ch)) {
      switch (GET_ATTACK(ch) + TYPE_HIT) {
        case TYPE_SLASH:
        case TYPE_BITE:
        case TYPE_SHOOT:
        case TYPE_CLEAVE:
        case TYPE_CLAW:
        case TYPE_LASH:
        case TYPE_THRASH:
        case TYPE_PIERCE:
        case TYPE_GORE:
          imp_crit = TRUE;
          break;
      }
    } else {
      if (HAS_FEAT(ch, FEAT_CLAWS_AND_BITE))
        imp_crit = TRUE;
    }
  }

  if (AFF_FLAGGED(ch, AFF_IMPACT_WEAPON)) {
    if (weap) {
      if (IS_SET(weapon_list[GET_OBJ_VAL(weap, 0)].damageTypes, DAMAGE_TYPE_BLUDGEONING))
        imp_crit = TRUE;
    } else if (IS_NPC(ch)) {
      switch (GET_ATTACK(ch) + TYPE_HIT) {
        case TYPE_HIT:
        case TYPE_STUN:
        case TYPE_BLUDGEON:
        case TYPE_BLAST:
        case TYPE_PUNCH:
        case TYPE_BATTER:
          imp_crit = TRUE;
          break;
      }
    } else {
      if (!HAS_FEAT(ch, FEAT_CLAWS_AND_BITE))
        imp_crit = TRUE;
    }
  }

  if (imp_crit)
    mult++;
  if (HAS_WEAPON_MASTERY(ch, weap) && HAS_FEAT(ch, FEAT_KI_CRITICAL))
    mult++;

  return (ext * mult) - 1; // difference from 20
}
 */

int determine_threat_range(struct char_data *ch, struct obj_data *wielded)
{
  int threat_range = 19;

  if (wielded)
    threat_range = 20 - weapon_list[GET_OBJ_VAL(wielded, 0)].critRange;
  else
    threat_range = 20;

  /* mods */
  if (HAS_FEAT(ch, FEAT_IMPROVED_CRITICAL) || is_using_keen_weapon(ch))
  { /* Check the weapon type, make sure it matches. */
    if ((((wielded != NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_IMPROVED_CRITICAL), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily)) ||
         ((wielded == NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_IMPROVED_CRITICAL), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily))) ||
        is_using_keen_weapon(ch))
    {
      if ((wielded == NULL) || weapon_list[GET_OBJ_VAL(wielded, 0)].critRange == 0)
      {
        threat_range--;
      }
      else
      {
        /* Subtract the threat range again, with +1 to correspond to 20.
         * i.e. a 20-only weapon (range 0) will be reduced by (0+1) so that the result is 19-20.
         *      a 19-20 weapon (range 1) will be reduced by (1+1) so that the result is 17-20.
         *      a 18-20 weapon (range 2) will be reduced by (2+1) so that the result is 15-20.
         */
        threat_range -= (weapon_list[GET_OBJ_VAL(wielded, 0)].critRange + 1);
      }
    }
  }

  if (HAS_FEAT(ch, FEAT_CRITICAL_SPECIALIST))
  { /* Check the weapon type, make sure it matches. */
    if (((wielded != NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily)) ||
        ((wielded == NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily)))
      threat_range -= HAS_FEAT(ch, FEAT_CRITICAL_SPECIALIST);
  }

  if (HAS_FEAT(ch, FEAT_KEEN_STRIKE) && is_bare_handed(ch))
  {
    threat_range -= 2;
  }

  if (HAS_FEAT(ch, FEAT_TONGUE_OF_THE_SUN_AND_MOON) && is_bare_handed(ch))
  {
    threat_range--;
  }

  if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) && HAS_FEAT(ch, FEAT_CRITICAL_FOCUS))
    threat_range--;

  /* end mods */

  if (threat_range <= 2) /* just in case */
    threat_range = 3;
  return threat_range;
}

#define CRIT_MULTI_MIN 2
#define CRIT_MULTI_MAX 7

int determine_critical_multiplier(struct char_data *ch, struct obj_data *wielded)
{
  int crit_multi = 2;

  if (wielded)
  {
    switch (weapon_list[GET_OBJ_VAL(wielded, 0)].critMult)
    {
    case CRIT_X2:
      crit_multi = 2;
      break;
    case CRIT_X3:
      crit_multi = 3;
      break;
    case CRIT_X4:
      crit_multi = 4;
      break;
    case CRIT_X5:
      crit_multi = 5;
      break;
    case CRIT_X6:
      crit_multi = 6;
      break;
    }
  }

  if (HAS_FEAT(ch, FEAT_INCREASED_MULTIPLIER))
  { /* Check the weapon type, make sure it matches. */
    if (((wielded != NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily)) ||
        ((wielded == NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily)))
      crit_multi += HAS_FEAT(ch, FEAT_INCREASED_MULTIPLIER);
  }

  /* high level mobs are getting a crit bonus here */
  if (IS_NPC(ch) && GET_LEVEL(ch) > 30)
  {
    crit_multi += (GET_LEVEL(ch) - 30);
  }

  /* establish some caps */
  if (crit_multi < CRIT_MULTI_MIN)
    crit_multi = CRIT_MULTI_MIN;
  if (crit_multi > CRIT_MULTI_MAX)
    crit_multi = CRIT_MULTI_MAX;

  return crit_multi;
}
#undef CRIT_MULTI_MIN
#undef CRIT_MULTI_MAX

/* computes damage dice based on bare-hands, weapon, class (monk), or
 npc's (which use special bare hand damage dice) */

/* #define MODE_NORMAL_HIT       0
   #define MODE_DISPLAY_PRIMARY  2
   #define MODE_DISPLAY_OFFHAND  3
   #define MODE_DISPLAY_RANGED   4 */
int compute_dam_dice(struct char_data *ch, struct char_data *victim,
                     struct obj_data *wielded, int mode)
{
  int diceOne = 0, diceTwo = 0;
  bool is_ranged = FALSE;

  /* going to check if we are in a state ready to use ranged weapon
     before anything else */
  if (can_fire_ammo(ch, TRUE))
  {
    is_ranged = TRUE;
    /* this -has- to be a weapon, can_fire_ammo() already verified this */
    wielded = is_using_ranged_weapon(ch, TRUE);
  } /* should be ready to check for ranged */

  // just information mode
  if (mode == MODE_DISPLAY_PRIMARY)
  {
    if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
    {
      if (IS_PIXIE(ch))
        send_to_char(ch, "A Tiny Short Sword\r\n");
      if (IS_EFREETI(ch))
        send_to_char(ch, "A Flaming Ranseur\r\n");
      else if (IS_IRON_GOLEM(ch))
        send_to_char(ch, "Huge Iron Fists\r\n");
      else
        send_to_char(ch, "Claws, Teeth and Smash!\r\n");
    }
    else if (affected_by_spell(ch, SKILL_DRHRT_CLAWS))
    {
      send_to_char(ch, "Draconic Claws!\r\n");
    }
    else if (!GET_EQ(ch, WEAR_WIELD_1) && !GET_EQ(ch, WEAR_WIELD_2H))
    {
      send_to_char(ch, "Bare-hands\r\n");
    }
    else
    {
      if (GET_EQ(ch, WEAR_WIELD_2H))
        wielded = GET_EQ(ch, WEAR_WIELD_2H);
      else
        wielded = GET_EQ(ch, WEAR_WIELD_1);
      show_obj_to_char(wielded, ch, SHOW_OBJ_SHORT, 0);
    }
  }
  else if (mode == MODE_DISPLAY_OFFHAND)
  {
    if (is_using_double_weapon(ch))
    {
      show_obj_to_char(GET_EQ(ch, WEAR_WIELD_2H), ch, SHOW_OBJ_SHORT, 0);
    }
    else if (!GET_EQ(ch, WEAR_WIELD_OFFHAND))
    {
      send_to_char(ch, "Bare-hands\r\n");
    }
    else
    {
      wielded = GET_EQ(ch, WEAR_WIELD_OFFHAND);
      show_obj_to_char(GET_EQ(ch, WEAR_WIELD_OFFHAND), ch, SHOW_OBJ_SHORT, 0);
    }
  }
  else if (mode == MODE_DISPLAY_RANGED && is_ranged)
  { // ranged info
    show_obj_to_char(wielded, ch, SHOW_OBJ_SHORT, 0);
  }

  /* real calculations */
  if (IS_WILDSHAPED(ch))
  {
    diceOne = MAX(1, HAS_FEAT(ch, FEAT_NATURAL_ATTACK));
    switch (GET_SIZE(ch))
    {
    case SIZE_FINE:
      diceTwo = 1;
      break;
    case SIZE_DIMINUTIVE:
      diceTwo = 1;
      break;
    case SIZE_TINY:
      diceTwo = 2;
      break;
    case SIZE_SMALL:
      diceTwo = 3;
      break;
    case SIZE_MEDIUM:
      diceTwo = 4;
      break;
    case SIZE_LARGE:
      diceTwo = 4;
      break;
    case SIZE_HUGE:
      diceTwo = 5;
      break;
    case SIZE_GARGANTUAN:
      diceTwo = 5;
      break;
    case SIZE_COLOSSAL:
      diceTwo = 6;
      break;
    default:
      diceTwo = 1;
      break;
    }
    if (IS_MANTICORE(ch))
    {
      diceOne = 3 + (CLASS_LEVEL(ch, CLASS_SHIFTER) / 2);
      diceTwo = 4;
    }
    if (IS_PIXIE(ch))
    {
      diceOne = 2;
      diceTwo = 2 + (CLASS_LEVEL(ch, CLASS_SHIFTER) / 2);
    }
  }
  else if (IS_MORPHED(ch))
  {
    diceOne = 2;
    diceTwo = 6;
  }
  else if (affected_by_spell(ch, SKILL_DRHRT_CLAWS))
  {
    diceOne = 1;
    diceTwo = (CLASS_LEVEL(ch, CLASS_SORCERER) >= 7) ? 6 : 4;
  }
  else if (!is_ranged && wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
  { // weapon
    diceOne = GET_OBJ_VAL(wielded, 1);
    diceTwo = GET_OBJ_VAL(wielded, 2);
  }
  else if (is_ranged)
  { // ranged weapon
    diceOne = GET_OBJ_VAL(wielded, 1);
    diceTwo = GET_OBJ_VAL(wielded, 2);
  }
  else
  { // barehand
    compute_barehand_dam_dice(ch, &diceOne, &diceTwo);
  }

  /* display modes */
  if (mode == MODE_DISPLAY_PRIMARY ||
      mode == MODE_DISPLAY_OFFHAND ||
      mode == MODE_DISPLAY_RANGED)
  {
    send_to_char(ch, "Threat Range: %d, ", determine_threat_range(ch, wielded));
    send_to_char(ch, "Critical Multiplier: %d, ", determine_critical_multiplier(ch, wielded));
    send_to_char(ch, "Damage Dice: %dD%d, ", diceOne, diceTwo);
  }

  /* mods */
  if ((HAS_FEAT(ch, FEAT_UNSTOPPABLE_STRIKE) * 5) >= rand_number(1, 100))
  { /* Check the weapon type, make sure it matches. */
    if (((wielded != NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily)) ||
        ((wielded == NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily)))
      return (diceOne * diceTwo); /* max damage! */
  }

  if (victim && affected_by_spell(victim, PSIONIC_BRUTALIZE_WOUNDS))
  {
    if (get_char_affect_modifier(victim, PSIONIC_BRUTALIZE_WOUNDS, APPLY_SPECIAL) == BRUTALIZE_WOUNDS_SAVE_SUCCESS)
      return dice(diceOne, diceTwo) + diceOne;
    else
      return (diceOne * diceTwo) + diceOne; /* max damage! And more! */
  }

  return dice(diceOne, diceTwo);
}

/* simple test for testing (confirming) critical hit */
int is_critical_hit(struct char_data *ch, struct obj_data *wielded, int diceroll,
                    int calc_bab, int victim_ac)
{
  int threat_range, confirm_roll = d20(ch) + calc_bab;

  if (FIGHTING(ch) && CLASS_LEVEL(ch, CLASS_INQUISITOR) >= 10 && is_judgement_possible(ch, FIGHTING(ch), INQ_JUDGEMENT_JUSTICE))
    confirm_roll += get_judgement_bonus(ch, INQ_JUDGEMENT_JUSTICE);
  if (FIGHTING(ch) && CLASS_LEVEL(FIGHTING(ch), CLASS_INQUISITOR) >= 10 && is_judgement_possible(FIGHTING(ch), ch, INQ_JUDGEMENT_PROTECTION))
    confirm_roll -= get_judgement_bonus(FIGHTING(ch), INQ_JUDGEMENT_PROTECTION);

  if (FIGHTING(ch) && KNOWS_DISCOVERY(FIGHTING(ch), ALC_DISC_PRESERVE_ORGANS) && dice(1, 4) == 1 && !(FIGHTING(ch)->preserve_organs_procced))
  {
    FIGHTING(ch)->preserve_organs_procced = TRUE;
    return FALSE;
  }

  if (FIGHTING(ch) && (affected_by_spell(FIGHTING(ch), PSIONIC_BODY_OF_IRON) || affected_by_spell(FIGHTING(ch), PSIONIC_SHADOW_BODY)))
    return FALSE;

  threat_range = determine_threat_range(ch, wielded);

  if (diceroll >= threat_range)
  { /* critical potential? */

    if (HAS_FEAT(ch, FEAT_POWER_CRITICAL))
    { /* Check the weapon type, make sure it matches. */
      if (((wielded != NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_POWER_CRITICAL), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily)) ||
          ((wielded == NULL) && HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_POWER_CRITICAL), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily)))
        confirm_roll += 4;
    }

    if (HAS_FEAT(ch, FEAT_WEAPON_TRAINING))
    {
      confirm_roll += HAS_FEAT(ch, FEAT_WEAPON_TRAINING) * 2;
    }

    /* high level mobs get a bonus! */
    if (IS_NPC(ch) && GET_LEVEL(ch) > 30)
    {
      confirm_roll += (GET_LEVEL(ch) - 30) * 2;
    }

    if (confirm_roll >= victim_ac) /* confirm critical */
      return 1;                    /* yep, critical! */
  }

  return 0; /* nope, no critical */
}

/* you are going to arrive here from an attack, or viewing mode
 * We have two functions: compute_hit_damage() and compute_damage_bonus() that
 * both basically will compute how much damage a given hit will do or display
 * how much damage potential you have (attacks command).  What is the difference?
 *   Compute_hit_damage() basically calculates bonus damage that will not be
 * displayed, compute_damage_bonus() calculates bonus damage that will be
 * displayed.  compute_hit_damage() always calls compute_damage_bonus() */

/* #define MODE_NORMAL_HIT       0
   #define MODE_DISPLAY_PRIMARY  2
   #define MODE_DISPLAY_OFFHAND  3
   #define MODE_DISPLAY_RANGED   4
 * Valid attack_type(s) are:
 *   ATTACK_TYPE_PRIMARY       : Primary hand attack.
 *   ATTACK_TYPE_OFFHAND       : Offhand attack.
 *   ATTACK_TYPE_RANGED        : Ranged attack.
 *   ATTACK_TYPE_UNARMED       : Unarmed attack.
 *   ATTACK_TYPE_TWOHAND       : Two-handed weapon attack.
 *   ATTACK_TYPE_BOMB_TOSS     : Alchemist - tossing bombs
 *   ATTACK_TYPE_PRIMARY_SNEAK : impromptu sneak attack, primary hand
 *   ATTACK_TYPE_OFFHAND_SNEAK : impromptu sneak attack, offhand  */
int compute_hit_damage(struct char_data *ch, struct char_data *victim,
                       int w_type, int diceroll, int mode, bool is_critical, int attack_type)
{
  int dam = 0, damage_holder = 0;
  struct obj_data *wielded = NULL;

  /* redundancy necessary due to sometimes arriving here without going through
   * hit()*/
  if (attack_type == ATTACK_TYPE_UNARMED || IS_WILDSHAPED(ch) || IS_MORPHED(ch))
    wielded = NULL;
  else
    wielded = get_wielded(ch, attack_type);

  if (GET_EQ(ch, WEAR_WIELD_2H) && mode != MODE_DISPLAY_RANGED &&
      attack_type != ATTACK_TYPE_RANGED)
    attack_type = ATTACK_TYPE_TWOHAND;

  /* calculate how much damage to do with a given hit() */
  if (mode == MODE_NORMAL_HIT)
  {
    /* determine weapon dice damage (or lack of weaopn) */
    dam = compute_dam_dice(ch, victim, wielded, mode);
    /* add any modifers to melee damage: strength, circumstance penalty, fatigue, size, etc etc */
    dam += compute_damage_bonus(ch, victim, wielded, w_type, NO_MOD, mode, attack_type);

    /* calculate bonus to damage based on target position */
    switch (GET_POS(victim))
    {
    case POS_SITTING:
      dam += 2;
      break;
    case POS_RESTING:
      dam += 4;
      break;
    case POS_SLEEPING:
      dam *= 2;
      break;
    case POS_STUNNED:
      dam *= 1.25;
      break;
    case POS_INCAP:
      dam *= 1.5;
      break;
    case POS_MORTALLYW:
    case POS_DEAD:
      dam *= 1.75;
      break;
    case POS_STANDING:
    case POS_FIGHTING:
    default:
      break;
    }

    damage_holder = dam; /* store so we don't multiply a multiply */

    /* handle critical hit damage here */
    if (is_critical && !IS_IMMUNE_CRITS(victim))
    {

      /* critical message */
      send_to_char(ch, "\tW[CRIT!]\tn");
      send_to_char(victim, "\tR[CRIT!]\tn");

      if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) && HAS_FEAT(ch, FEAT_CRITICAL_FOCUS))
        dam += 2;

      /* high level mobs are getting a crit bonus here */
      if (IS_NPC(ch) && GET_LEVEL(ch) > 30)
      {
        dam += (GET_LEVEL(ch) - 30) * 5;
      }

      /* critical bonus */
      dam *= determine_critical_multiplier(ch, wielded);

      if (has_teamwork_feat(ch, FEAT_PRECISE_FLANKING) && is_flanked(ch, victim))
        dam += dice(1, 6);

      if (affected_by_spell(ch, SPELL_WEAPON_OF_AWE) && !affected_by_spell(victim, SPELL_AFFECT_WEAPON_OF_AWE))
      {
        if (!is_immune_mind_affecting(ch, victim, FALSE) &&
            !is_immune_fear(ch, victim, FALSE))
        {
          struct affected_type af;
          new_affect(&af);
          af.spell = SPELL_AFFECT_WEAPON_OF_AWE;
          af.duration = 1;
          SET_BIT_AR(af.bitvector, AFF_SHAKEN);
          affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
          act("Your attack causes $N to become shaken!", FALSE, ch, 0, victim, TO_CHAR);
          act("$n's attack causes YOU to become shaken!", FALSE, ch, 0, victim, TO_VICT);
          act("$n's attack causes $N to become shaken!", FALSE, ch, 0, victim, TO_NOTVICT);
        }
      }

      /* duelist crippling critical: light armor and free hand: 1d4 str dam, 1d4 dex dam,
       * 4 penalty to saves, 4 penalty to AC, 2d4 bleed damage and 2d4 drain moves */
      if (HAS_FEAT(ch, FEAT_CRIPPLING_CRITICAL) && HAS_FREE_HAND(ch) &&
          compute_gear_armor_type(ch) <= ARMOR_TYPE_LIGHT)
      {
        struct mud_event_data *pMudEvent = NULL;
        struct affected_type af;

        /* has event?  then we increment the events svariable */
        if ((pMudEvent = char_has_mud_event(victim, eCRIPPLING_CRITICAL)))
        {
          int crippling_critical_var = 0;
          char buf[10] = {'\0'};

          crippling_critical_var = atoi((char *)pMudEvent->sVariables);
          crippling_critical_var++;
          snprintf(buf, sizeof(buf), "%d", crippling_critical_var);
          if (pMudEvent->sVariables) /* need to free memory if we changing it */
            free(pMudEvent->sVariables);
          pMudEvent->sVariables = strdup(buf);
        }
        else
        { /* no event, so make one */
          pMudEvent = new_mud_event(eCRIPPLING_CRITICAL, victim, strdup("1"));
          /* create and attach new event, apply the first effect */
          attach_mud_event(pMudEvent, 60 * PASSES_PER_SEC);
        }

        /* dummy check */
        if (!pMudEvent)
          ;
        else
        { /* decide on the effect to drop */
          act("\tRYou strike $N with a crippling critical!\tn", FALSE, ch, NULL, victim, TO_CHAR);
          act("\tr$n strikes you with a crippling critical!\tn", FALSE, ch, NULL, victim, TO_VICT);
          act("\tr$n strikes $N with a crippling critical!\tn", FALSE, ch, NULL, victim, TO_NOTVICT);
          switch (atoi((char *)pMudEvent->sVariables))
          {
          case 1: /* 1d4 strength damage */
            new_affect(&af);
            af.spell = SKILL_CRIPPLING_CRITICAL;
            af.location = APPLY_STR;
            af.modifier = -dice(1, 4);
            af.duration = MAX(1, (int)(event_time(pMudEvent->pEvent) / 60));
            SET_BIT_AR(af.bitvector, AFF_CRIPPLING_CRITICAL);
            affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
            break;
          case 2: /* 1d4 dexterity damage */
            new_affect(&af);
            af.spell = SKILL_CRIPPLING_CRITICAL;
            af.location = APPLY_DEX;
            af.modifier = -dice(1, 4);
            af.duration = MAX(1, (int)(event_time(pMudEvent->pEvent) / 60));
            SET_BIT_AR(af.bitvector, AFF_CRIPPLING_CRITICAL);
            affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
            break;
          case 3: /* -4 penalty to fortitude saves */
            new_affect(&af);
            af.spell = SKILL_CRIPPLING_CRITICAL;
            af.location = APPLY_SAVING_FORT;
            af.modifier = -4;
            af.duration = MAX(1, (int)(event_time(pMudEvent->pEvent) / 60));
            SET_BIT_AR(af.bitvector, AFF_CRIPPLING_CRITICAL);
            affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
            break;
          case 4: /* -4 penalty to reflex saves */
            new_affect(&af);
            af.spell = SKILL_CRIPPLING_CRITICAL;
            af.location = APPLY_SAVING_REFL;
            af.modifier = -4;
            af.duration = MAX(1, (int)(event_time(pMudEvent->pEvent) / 60));
            SET_BIT_AR(af.bitvector, AFF_CRIPPLING_CRITICAL);
            affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
            break;
          case 5: /* -4 penalty to will saves */
            new_affect(&af);
            af.spell = SKILL_CRIPPLING_CRITICAL;
            af.location = APPLY_SAVING_WILL;
            af.modifier = -4;
            af.duration = MAX(1, (int)(event_time(pMudEvent->pEvent) / 60));
            SET_BIT_AR(af.bitvector, AFF_CRIPPLING_CRITICAL);
            affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
            break;
          case 6: /* -4 penalty to AC */
            new_affect(&af);
            af.spell = SKILL_CRIPPLING_CRITICAL;
            af.location = APPLY_AC_NEW;
            af.modifier = -4;
            af.duration = MAX(1, (int)(event_time(pMudEvent->pEvent) / 60));
            SET_BIT_AR(af.bitvector, AFF_CRIPPLING_CRITICAL);
            affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
            break;
          default: /* 2d4 bleed damage and 20d4 moves drain */
            GET_MOVE(victim) -= dice(20, 4);
            dam += dice(2, 4);
            break;
          }
        }
      } /* end crippling critical */

      /* raging critical feat */
      if (HAS_FEAT(ch, FEAT_RAGING_CRITICAL) && affected_by_spell(ch, SKILL_RAGE))
      {
        /*fail*/ if ((GET_SIZE(ch) - GET_SIZE(victim)) >= 2)
        {
          ;
        }
        /*fail*/ else if ((GET_SIZE(victim) - GET_SIZE(ch)) >= 2)
        {
          ;
        }
        /*fail*/ else if (GET_POS(victim) <= POS_SITTING)
        {
          ;
        }
        /*fail*/ else if (IS_INCORPOREAL(victim))
        {
          ;
        }
        /*fail*/ else if (MOB_FLAGGED(victim, MOB_NOBASH))
        {
          ;
        }
        /*fail*/ else if (MOB_FLAGGED(victim, MOB_NOKILL) || !is_mission_mob(ch, victim))
        {
          ;
        }
        /*fail*/ else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
                          ch->next_in_room != victim && victim->next_in_room != ch)
        {
          ;
        }
        else
        { /*success!*/
          send_to_char(ch, "\tW[Raging CRIT!]\tn");
          perform_knockdown(ch, victim, SKILL_BASH);
        }
      }
    } /* END critical hit */

    /* mounted charging character using charging weapons, whether this goes up
     * top or bottom of dam calculation can have a dramatic effect on this number */
    if (AFF_FLAGGED(ch, AFF_CHARGING) && RIDING(ch))
    {
      if (HAS_FEAT(ch, FEAT_SPIRITED_CHARGE))
      { /* mounted, charging with spirited charge feat */
        if (wielded && HAS_WEAPON_FLAG(wielded, WEAPON_FLAG_CHARGE))
        { /* with lance too */
          if (DEBUGMODE)
          {
            send_to_char(ch, "DEBUG: Weapon Charge Flag Working on Lance!\r\n");
          }
          dam += damage_holder * 2; /* x3 */
        }
        else
        {
          dam += damage_holder; /* x2 */
        }
      }
      else if (wielded && HAS_WEAPON_FLAG(wielded, WEAPON_FLAG_CHARGE))
      { /* mounted charging, no feat, but with lance */
        if (DEBUGMODE)
        {
          send_to_char(ch, "DEBUG: Weapon Charge Flag Working on Lance!\r\n");
        }
        dam += damage_holder; /* x2 */
      }

      /* handle acrobatic charge */
    }
    else if (AFF_FLAGGED(ch, AFF_CHARGING) && !RIDING(ch) &&
             HAS_FEAT(ch, FEAT_ACROBATIC_CHARGE))
    {
      dam += 4;
    }

    /* Add additional damage dice from weapon special abilities. - Ornir */
    if ((wielded || using_monk_gloves(ch)) && FIGHTING(ch))
    {
      if (!wielded)
        wielded = GET_EQ(ch, WEAR_HANDS);

      /* process weapon abilities - critical */
      if (is_critical && !(IS_NPC(victim) && GET_RACE(victim) == RACE_TYPE_UNDEAD))
        process_weapon_abilities(wielded, ch, victim, ACTMTD_ON_CRIT, NULL);
      /*chaotic weapon*/
      if ((obj_has_special_ability(wielded, WEAPON_SPECAB_ANARCHIC) ||
           HAS_FEAT(ch, FEAT_CHAOTIC_WEAPON)) &&
          ((IS_LG(victim)) ||
           (IS_LN(victim)) ||
           (IS_LE(victim)) ||
           (IS_NPC(victim) && HAS_SUBRACE(victim, SUBRACE_LAWFUL))))
      {
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
          send_to_char(ch, "\tW[CHAOS]\tn ");
        if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
          send_to_char(victim, "\tR[CHAOS]\tn ");
        dam += dice(2, 6);
      }
      /*lawful weapon*/
      if ((obj_has_special_ability(wielded, WEAPON_SPECAB_AXIOMATIC) ||
           HAS_FEAT(ch, FEAT_LAWFUL_WEAPON)) &&
          ((IS_CG(victim)) ||
           (IS_CN(victim)) ||
           (IS_CE(victim)) ||
           (IS_NPC(victim) && HAS_SUBRACE(victim, SUBRACE_CHAOTIC))))
      {
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
          send_to_char(ch, "\tW[LAW]\tn ");
        if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
          send_to_char(victim, "\tR[LAW]\tn ");
        dam += dice(2, 6);
      }
      /*evil weapon*/
      if ((obj_has_special_ability(wielded, WEAPON_SPECAB_UNHOLY) ||
           HAS_FEAT(ch, FEAT_EVIL_SCYTHE)) &&
          ((IS_CG(victim)) ||
           (IS_NG(victim)) ||
           (IS_LG(victim)) ||
           (IS_NPC(victim) && HAS_SUBRACE(victim, SUBRACE_GOOD))))
      {
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
          send_to_char(ch, "\tW[EVIL]\tn ");
        if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
          send_to_char(victim, "\tR[EVIL]\tn ");
        dam += dice(2, 6);
      }
      /*good weapon*/
      if ((obj_has_special_ability(wielded, WEAPON_SPECAB_HOLY) ||
           HAS_FEAT(ch, FEAT_GOOD_LANCE)) &&
          ((IS_CE(victim)) ||
           (IS_NE(victim)) ||
           (IS_LE(victim)) ||
           (IS_NPC(victim) && HAS_SUBRACE(victim, SUBRACE_EVIL))))
      {
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
          send_to_char(ch, "\tW[GOOD]\tn ");
        if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
          send_to_char(victim, "\tR[GOOD]\tn ");
        dam += dice(2, 6);
      }
      /*bane weapon*/
      if (affected_by_spell(ch, ABILITY_AFFECT_BANE_WEAPON))
      {
        if ((IS_NPC(victim) && (GET_RACE(victim) == GET_BANE_TARGET_TYPE(ch)) && GET_RACE(victim) != RACE_TYPE_UNDEFINED) ||
            (!IS_NPC(victim) && (GET_BANE_TARGET_TYPE(ch) == RACE_TYPE_HUMANOID)))
        {
          if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
            send_to_char(ch, "\tW[BANE]\tn ");
          if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
            send_to_char(victim, "\tR[BANE]\tn ");
          dam += dice(HAS_FEAT(ch, FEAT_PERFECT_JUDGEMENT) ? 6 : (HAS_FEAT(ch, FEAT_GREATER_BANE) ? 4 : 2), 6);
        }
      }
      /*bane weapon*/
      if ((obj_has_special_ability(wielded, WEAPON_SPECAB_BANE)))
      {
        /* Check the values in the special ability record for the NPCRACE and SUBRACE. */
        int *value = get_obj_special_ability(wielded, WEAPON_SPECAB_BANE)->value;
        if ((GET_RACE(victim) == value[0]) && (HAS_SUBRACE(victim, value[1])))
        {
          if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
            send_to_char(ch, "\tW[BANE]\tn ");
          if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
            send_to_char(victim, "\tR[BANE]\tn ");
          dam += dice(2, 6);
        }
      }
      /*bane weapon*/
      if (victim != ch && HAS_FEAT(ch, FEAT_BANE_OF_ENEMIES) && HAS_FEAT(ch, FEAT_FAVORED_ENEMY))
      {
        if (!IS_NPC(victim) && IS_FAV_ENEMY_OF(ch, RACE_TYPE_HUMANOID))
        {
          if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
            send_to_char(ch, "\tW[BANE]\tn ");
          if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
            send_to_char(victim, "\tR[BANE]\tn ");
          dam += dice(2, 6);
        }
        else if (IS_NPC(victim) && IS_FAV_ENEMY_OF(ch, GET_RACE(victim)))
        {
          if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
            send_to_char(ch, "\tW[BANE]\tn ");
          if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
            send_to_char(victim, "\tR[BANE]\tn ");
          dam += dice(2, 6);
        }
      }

    } /* end wielded */

    /* calculate weapon damage for _display_ purposes */
  }
  else if (mode == MODE_DISPLAY_PRIMARY ||
           mode == MODE_DISPLAY_OFFHAND ||
           mode == MODE_DISPLAY_RANGED)
  {
    /* calculate dice */
    dam = compute_dam_dice(ch, ch, wielded, mode);
    /* modifiers to melee damage */
    dam += compute_damage_bonus(ch, ch, wielded, TYPE_UNDEFINED_WTYPE, NO_MOD, mode, attack_type);
  }

  return MAX(1, dam); // min damage of 1
}

/* this function takes ch (attacker) against victim (defender) who has
   inflicted dam damage and will reduce damage by X depending on the type
   of 'ward' the defender has (such as stoneskin)
   this will return the modified damage
 * -note- melee only */
#define STONESKIN_ABSORB 16
#define IRONSKIN_ABSORB 36
#define EPIC_WARDING_ABSORB 76

int handle_warding(struct char_data *ch, struct char_data *victim, int dam)
{
  int warding = 0;

  if (affected_by_spell(victim, SPELL_STONESKIN))
  {
    return dam;
    /* comment or delete this line above to re-enable old stone skin */

    if (GET_STONESKIN(victim) <= 0)
    {
      send_to_char(victim, "\tDYour skin has reverted from stone!\tn\r\n");
      affect_from_char(victim, SPELL_STONESKIN);
      GET_STONESKIN(victim) = 0;
      return dam;
    }
    warding = MIN(MIN(STONESKIN_ABSORB, GET_STONESKIN(victim)), dam);

    GET_STONESKIN(victim) -= warding;
    dam -= warding;
    if (GET_STONESKIN(victim) <= 0)
    {
      send_to_char(victim, "\tDYour skin has reverted from stone!\tn\r\n");
      affect_from_char(victim, SPELL_STONESKIN);
      GET_STONESKIN(victim) = 0;
    }
    if (dam <= 0)
    {
      send_to_char(victim, "\tWYour skin of stone absorbs the attack!\tn\r\n");
      send_to_char(ch,
                   "\tRYou have failed to penetrate the stony skin of %s!\tn\r\n",
                   GET_NAME(victim));
      act("$n fails to penetrate the stony skin of $N!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      return -1;
    }
    else
    {
      send_to_char(victim, "\tW<stone:%d>\tn", warding);
      send_to_char(ch, "\tR<oStone:%d>\tn", warding);
    }
  }
  else if (affected_by_spell(victim, SPELL_IRONSKIN))
  {
    if (GET_STONESKIN(victim) <= 0)
    {
      send_to_char(victim, "\tDYour ironskin has faded!\tn\r\n");
      affect_from_char(victim, SPELL_IRONSKIN);
      GET_STONESKIN(victim) = 0;
      return dam;
    }
    warding = MIN(IRONSKIN_ABSORB, GET_STONESKIN(victim));

    GET_STONESKIN(victim) -= warding;
    dam -= warding;
    if (GET_STONESKIN(victim) <= 0)
    {
      send_to_char(victim, "\tDYour ironskin has fallen!\tn\r\n");
      affect_from_char(victim, SPELL_IRONSKIN);
      GET_STONESKIN(victim) = 0;
    }
    if (dam <= 0)
    {
      send_to_char(victim, "\tWYour ironskin absorbs the attack!\tn\r\n");
      send_to_char(ch,
                   "\tRYou have failed to penetrate the ironskin of %s!\tn\r\n",
                   GET_NAME(victim));
      act("$n fails to penetrate the ironskin of $N!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      return -1;
    }
    else
    {
      send_to_char(victim, "\tW<ironskin:%d>\tn", warding);
      send_to_char(ch, "\tR<oIronskin:%d>\tn", warding);
    }
  }
  else if (affected_by_spell(victim, SPELL_EPIC_WARDING))
  {
    if (GET_STONESKIN(victim) <= 0)
    {
      send_to_char(victim, "\tDYour ward has fallen!\tn\r\n");
      affect_from_char(victim, SPELL_EPIC_WARDING);
      GET_STONESKIN(victim) = 0;
      return dam;
    }
    warding = MIN(EPIC_WARDING_ABSORB, GET_STONESKIN(victim));

    GET_STONESKIN(victim) -= warding;
    dam -= warding;
    if (GET_STONESKIN(victim) <= 0)
    {
      send_to_char(victim, "\tDYour ward has fallen!\tn\r\n");
      affect_from_char(victim, SPELL_EPIC_WARDING);
      GET_STONESKIN(victim) = 0;
    }
    if (dam <= 0)
    {
      send_to_char(victim, "\tWYour ward absorbs the attack!\tn\r\n");
      send_to_char(ch,
                   "\tRYou have failed to penetrate the ward of %s!\tn\r\n",
                   GET_NAME(victim));
      act("$n fails to penetrate the ward of $N!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      return -1;
    }
    else
    {
      send_to_char(victim, "\tW<ward:%d>\tn", warding);
      send_to_char(ch, "\tR<oWard:%d>\tn", warding);
    }
  }
  else
  { // has no warding
    return dam;
  }

  return dam;
}
#undef STONESKIN_ABSORB
#undef EPIC_WARDING_ABSORB
#undef IRONSKIN_ABSORB

bool weapon_bypasses_dr(struct obj_data *weapon, struct damage_reduction_type *dr, struct char_data *ch)
{
  bool passed = FALSE;
  int i = 0;

  /* TODO Change this to handle unarmed attacks! */
  if (weapon == NULL)
    return FALSE;

  for (i = 0; i < MAX_DR_BYPASS; i++)
  {
    if (dr->bypass_cat[i] != DR_BYPASS_CAT_UNUSED)
    {
      switch (dr->bypass_cat[i])
      {
      case DR_BYPASS_CAT_NONE:
        break;
      case DR_BYPASS_CAT_MAGIC:
        if (IS_SET_AR(GET_OBJ_EXTRA(weapon), ITEM_MAGIC))
          passed = TRUE;
        if (affected_by_spell(ch, SKILL_DRHRT_CLAWS) && CLASS_LEVEL(ch, CLASS_SORCERER) >= 7)
          passed = TRUE;
        break;
      case DR_BYPASS_CAT_MATERIAL:
        if (GET_OBJ_MATERIAL(weapon) == dr->bypass_val[i])
          passed = TRUE;
        break;
      case DR_BYPASS_CAT_DAMTYPE:
          if ((dr->bypass_val[i] == DR_DAMTYPE_BLUDGEONING) &&
                  (HAS_DAMAGE_TYPE(weapon, DAMAGE_TYPE_BLUDGEONING))
                  passed = TRUE;
          else if ((dr->bypass_val[i] == DR_DAMTYPE_SLASHING) &&
                  (HAS_DAMAGE_TYPE(weapon, DAMAGE_TYPE_SLASHING))
                  passed = TRUE;
          else if ((dr->bypass_val[i] == DR_DAMTYPE_PIERCING) &&
                  (HAS_DAMAGE_TYPE(weapon, DAMAGE_TYPE_PIERCING))
                  passed = TRUE;

            break;
      }
    }
  }
  return passed;
}

/* this fuction will apply damage reduction to incoming melee damage, it also has a display mode
  ch -> person damaging with possible bypass
  victim -> person being damaged with possible DR
  wielded -> which weapon is producing this damage
  dam -> how much damage is coming in

  return value for non display is the modified damage reduction */
int apply_damage_reduction(struct char_data *ch, struct char_data *victim, struct obj_data *wielded, int dam, bool display)
{
  struct damage_reduction_type *dr, *cur;
  // struct damage_reduction_type *temp;
  int reduction = 0;

  /* No DR, just return dam.*/
  if (GET_DR(victim) == NULL)
    return dam;

  dr = NULL;
  for (cur = GET_DR(victim); cur != NULL; cur = cur->next)
  {
    if (dr == NULL || (dr->amount < cur->amount && (weapon_bypasses_dr(wielded, cur, ch) == FALSE)))
      dr = cur;
  }

  /* Now dr is set to the 'best' DR for the incoming damage. */
  if (weapon_bypasses_dr(wielded, dr, ch) == TRUE)
  {
    reduction = 0;
  }
  else
    reduction = MIN(dr->amount, dam);

  if ((reduction > 0) &&
      (dr->max_damage > 0))
  {
    /* Damage the DR...*/
    dr->max_damage -= reduction;
    if (dr->max_damage <= 0)
    {
      /* The DR was destroyed!*/
      if (affected_by_spell(victim, dr->spell))
      {
        affect_from_char(victim, dr->spell);

        if (get_wearoff(dr->spell))
          send_to_char(victim, "%s\r\n", get_wearoff(dr->spell));
      }
    }
  }

  if (reduction)
  {
    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
      send_to_char(victim, "\tW<nDR:%d>\tn", reduction);
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
      send_to_char(ch, "\tR<onDR:%d>\tn", reduction);
  }

  return MAX(-1, dam - reduction);
}

/* all weapon poison system is right now is just firing spells off our weapon
   if the weapon has that given spell-num applied to it as a poison
   i have ambitious plans in the future to completely re-work poison in our
   system, and at that time i will re-work this -zusuk */
#define MAX_PSN_LVL LVL_IMPL /* maximum level a poison can be */
#define MAX_PSN_HIT 15       /* maximum times a poison can hit before wearing off */
void weapon_poison(struct char_data *ch, struct char_data *victim,
                   struct obj_data *wielded, struct obj_data *missile)
{

  /* start with the usual dummy checks */
  if (!ch)
    return;
  if (!victim)
    return;

  bool is_trelux = FALSE;

  if (GET_RACE(ch) == RACE_TRELUX)
    is_trelux = TRUE;
  else if (!wielded)
    *wielded = *missile;

  if (!wielded && !is_trelux)
    return;

  /* weapon or claws not poisoned */
  if (is_trelux && (TRLX_PSN_VAL(ch) <= 0 || TRLX_PSN_VAL(ch) >= NUM_SPELLS))
    return;
  else if (!is_trelux && (wielded->weapon_poison.poison <= 0 || wielded->weapon_poison.poison >= MAX_SPELLS)) /* this weapon is not poisoned */
  {
    return;
  }

  /* decrement strength and hits on weapon */
  if (is_trelux)
  {
    if (TRLX_PSN_LVL(ch) > MAX_PSN_LVL)
      TRLX_PSN_LVL(ch) = MAX_PSN_LVL;
    TRLX_PSN_LVL(ch) -= 2;
    if (TRLX_PSN_LVL(ch) <= 0)
      TRLX_PSN_LVL(ch) = 1;

    TRLX_PSN_HIT(ch)
    --;
    if (TRLX_PSN_HIT(ch) < 0)
      TRLX_PSN_HIT(ch) = 0;
    if (TRLX_PSN_HIT(ch) > MAX_PSN_HIT)
      TRLX_PSN_HIT(ch) = MAX_PSN_HIT;

    /* message/effect for poison wearing off of claws */
    if (TRLX_PSN_HIT(ch) <= 0)
    {
      send_to_char(ch, "The final bits of applied \tgpoison\tn wear off your claws!\r\n");
      TRLX_PSN_HIT(ch) = 0;
      TRLX_PSN_LVL(ch) = 0;
      TRLX_PSN_VAL(ch) = 0;
      return;
    }
  }
  else
  {
    if (wielded->weapon_poison.poison_level > MAX_PSN_LVL)
      wielded->weapon_poison.poison_level = MAX_PSN_LVL;
    wielded->weapon_poison.poison_level -= 2;
    if (wielded->weapon_poison.poison_level <= 0)
      wielded->weapon_poison.poison_level = 1;

    wielded->weapon_poison.poison_hits--;
    if (wielded->weapon_poison.poison_hits < 0)
      wielded->weapon_poison.poison = 0;
    if (wielded->weapon_poison.poison_hits > MAX_PSN_HIT)
      wielded->weapon_poison.poison_hits = MAX_PSN_HIT;

    /* message for poison wearing off of your weapon */
    if (wielded->weapon_poison.poison_hits <= 0)
    {
      send_to_char(ch, "The final bits of applied \tgpoison\tn wear off %s!\r\n",
                   wielded->short_description);
      wielded->weapon_poison.poison_hits = 0;
      wielded->weapon_poison.poison = 0;
      wielded->weapon_poison.poison_level = 0;
      return;
    }
  }

  /* for now we will not let you apply a spell that is already affecting vict */
  if (!is_trelux && affected_by_spell(victim, wielded->weapon_poison.poison))
    return;
  if (is_trelux && affected_by_spell(victim, TRLX_PSN_VAL(ch)))
    return;

  /* 20% chance to fire currently on melee weapons, %100 missiles/claws */
  /* disabled */
  // if (missile || is_trelux)
  //   ;
  // else if (rand_number(0, 5))
  //   return;

  if (is_trelux)
  {
    act("The claw's \tGpoison\tn attaches to $n.",
        FALSE, victim, NULL, 0, TO_ROOM);
    call_magic(ch, victim, NULL, TRLX_PSN_VAL(ch), 0, TRLX_PSN_LVL(ch),
               CAST_WEAPON_POISON);
  }
  else
  {
    act("The \tGpoison\tn from $p attaches to $n.", FALSE, victim, wielded, 0, TO_ROOM);
    call_magic(ch, victim, wielded, wielded->weapon_poison.poison, 0, wielded->weapon_poison.poison_level, CAST_WEAPON_POISON);
  }
}

/* this function will call the spell-casting ability of the
   given weapon (wpn) attacker (ch) has when attacking vict
   these are always 'violent' spells */
void weapon_spells(struct char_data *ch, struct char_data *vict,
                   struct obj_data *wpn)
{

  /* if this is a no-magic room, we aren't going to continue */
  if (ch->in_room && ch->in_room != NOWHERE && ch->in_room < top_of_world &&
      (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC) ||
       ROOM_AFFECTED(ch->in_room, RAFF_ANTI_MAGIC)))
    return;
  if (vict && vict->in_room && vict->in_room != NOWHERE && vict->in_room < top_of_world &&
      (ROOM_FLAGGED(IN_ROOM(vict), ROOM_NOMAGIC) ||
       ROOM_AFFECTED(vict->in_room, RAFF_ANTI_MAGIC)))
    return;

  int i = 0, random;

  if (wpn && HAS_SPELLS(wpn))
  {

    for (i = 0; i < MAX_WEAPON_SPELLS; i++)
    { /* increment this weapons spells */
      if (GET_WEAPON_SPELL(wpn, i) && GET_WEAPON_SPELL_AGG(wpn, i))
      {
        if (ch->in_room != vict->in_room)
        {
          if (FIGHTING(ch) && FIGHTING(ch) == vict)
            stop_fighting(ch);
          return;
        }
        random = rand_number(1, 100);
        if (GET_WEAPON_SPELL_PCT(wpn, i) >= random)
        {
          act("$p leaps to action with an attack of its own.", TRUE, ch, wpn, 0, TO_CHAR);
          act("$p leaps to action with an attack of its own.", TRUE, ch, wpn, 0, TO_ROOM);
          if (call_magic(ch, vict, wpn, GET_WEAPON_SPELL(wpn, i), 0, GET_WEAPON_SPELL_LVL(wpn, i), CAST_WEAPON_SPELL) < 0)
            return;
        }
      }
    }
  }

  if (wpn)
  {
    for (i = 0; i < MAX_WEAPON_CHANNEL_SPELLS; i++)
    { /* increment this weapons spells */
      if (GET_WEAPON_CHANNEL_SPELL(wpn, i) && GET_WEAPON_CHANNEL_SPELL_AGG(wpn, i))
      {
        if (ch->in_room != vict->in_room)
        {
          if (FIGHTING(ch) && FIGHTING(ch) == vict)
            stop_fighting(ch);
          return;
        }
        random = rand_number(1, 100);
        if (GET_WEAPON_CHANNEL_SPELL_PCT(wpn, i) >= random)
        {
          GET_WEAPON_CHANNEL_SPELL_USES(wpn, i)
          --;
          act("Your channelled spell erupts from $p.", TRUE, ch, wpn, 0, TO_CHAR);
          act("A channelled spell erupts from $p.", TRUE, ch, wpn, 0, TO_ROOM);
          if (call_magic(ch, vict, wpn, GET_WEAPON_CHANNEL_SPELL(wpn, i), 0, GET_WEAPON_CHANNEL_SPELL_LVL(wpn, i), CAST_WEAPON_SPELL) < 0)
            return;
        }
        if (GET_WEAPON_CHANNEL_SPELL_USES(wpn, i) <= 0)
        {
          send_to_char(ch, "\tMYour channelled spell '\tW%s\tn' on %s has expired.\r\n\tn", spell_info[GET_WEAPON_CHANNEL_SPELL(wpn, i)].name, wpn->short_description);
          GET_WEAPON_CHANNEL_SPELL(wpn, i) = 0;
          wpn->channel_spells[i].level = 0;
          GET_WEAPON_CHANNEL_SPELL_PCT(wpn, i) = 0;
          GET_WEAPON_CHANNEL_SPELL_AGG(wpn, i) = 0;
          GET_WEAPON_CHANNEL_SPELL_USES(wpn, i) = 0;
          continue;
        }
      }
    }
  }
}

/* if (ch) has a weapon with weapon spells on it that is
   considered non-offensive, the weapon will target (ch)
   with this spell - does not require to be in combat */
void idle_weapon_spells(struct char_data *ch)
{

  /* dummy check */
  if (!ch)
    return;

  /* if this is a no-magic room, we aren't going to continue */
  if (ch->in_room && ch->in_room != NOWHERE && ch->in_room < top_of_world &&
      (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOMAGIC) ||
       ROOM_AFFECTED(ch->in_room, RAFF_ANTI_MAGIC)))
    return;

  int random = 0, i = 0, j = 0;
  struct obj_data *gear = NULL;
  const char *buf = "$p begins to vibrate and release sparks of energy!";

  /* give some random messages */
  switch (dice(1, 4))
  {
  case 1:
    buf = "$p hums with power!";
    break;
  case 2:
    buf = "$p flashes with energy!";
    break;
  case 3:
    buf = "$p glows and lets off a deep sound!";
    break;
  default: /* default "vibrate and sparks" */
    break;
  }

  /* scan through gear */
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (GET_EQ(ch, i))
    {
      gear = GET_EQ(ch, i);

      /* we have an item, and does this item have spells on it? */
      if (gear && HAS_SPELLS(gear))
      {

        for (j = 0; j < MAX_WEAPON_SPELLS; j++)
        {
          if (GET_WEAPON_SPELL(gear, j) && !GET_WEAPON_SPELL_AGG(gear, j))
          {
            random = rand_number(1, 100);
            if (!affected_by_spell(ch, GET_WEAPON_SPELL(gear, j)) &&
                GET_WEAPON_SPELL_PCT(gear, j) >= random)
            {
              act(buf, TRUE, ch, gear, 0, TO_CHAR);
              act(buf, TRUE, ch, gear, 0, TO_ROOM);
              call_magic(ch, ch, NULL, GET_WEAPON_SPELL(gear, j), 0, GET_WEAPON_SPELL_LVL(gear, j), CAST_WEAPON_SPELL);
              return;
              /* we exit here because we don't want two or more items proccing off the same tick */
            }
          }
        }
      }
    }
  }
}

/* weapon spell function for random weapon procs,
 *  modified from original source - Iyachtu */
int weapon_special(struct obj_data *wpn, struct char_data *ch, char *hit_msg)
{
  if (!wpn)
    return 0;

  extern struct index_data *obj_index;
  int (*name)(struct char_data * ch, void *me, int cmd, const char *argument);

  name = obj_index[GET_OBJ_RNUM(wpn)].func;

  if (!name)

    return 0;

  return (name)(ch, wpn, 0, hit_msg);
}

/* Return the wielded weapon based on the attack type.
 * Valid attack_type(s) are:
 *   ATTACK_TYPE_PRIMARY : Primary hand attack.
 *   ATTACK_TYPE_OFFHAND : Offhand attack.
 *   ATTACK_TYPE_RANGED  : Ranged attack
 *   ATTACK_TYPE_TWOHAND : Two-handed weapon attack.
 *   ATTACK_TYPE_BOMB_TOSS     : Alchemist - tossing bombs
 *   ATTACK_TYPE_PRIMARY_SNEAK : impromptu sneak attack, primary hand
 *   ATTACK_TYPE_OFFHAND_SNEAK : impromptu sneak attack, offhand  */
struct obj_data *get_wielded(struct char_data *ch, /* Wielder */
                             int attack_type)      /* Type of attack. */
{
  struct obj_data *wielded = NULL;
  /* Check the primary hand location. */
  wielded = GET_EQ(ch, WEAR_WIELD_1);

  switch (attack_type)
  {
  case ATTACK_TYPE_RANGED:
  case ATTACK_TYPE_PRIMARY:
  case ATTACK_TYPE_PRIMARY_SNEAK:
    if (!wielded)
    { // 2-hand weapon, primary hand
      wielded = GET_EQ(ch, WEAR_WIELD_2H);
    }
    break;
  case ATTACK_TYPE_OFFHAND:
  case ATTACK_TYPE_OFFHAND_SNEAK:
    if (is_using_double_weapon(ch))
    {
      wielded = GET_EQ(ch, WEAR_WIELD_2H);
    }
    else
    {
      wielded = GET_EQ(ch, WEAR_WIELD_OFFHAND);
    }
    break;
  case ATTACK_TYPE_UNARMED:
  case ATTACK_TYPE_BOMB_TOSS:
    wielded = NULL;
    break;
  case ATTACK_TYPE_TWOHAND:
    wielded = GET_EQ(ch, WEAR_WIELD_2H);
    break;
  default:
    break;
  }

  return wielded;
}

/* Calculate ch's attack bonus when attacking victim, for the type of attack given.
 * Valid attack_type(s) are:
 *   ATTACK_TYPE_PRIMARY : Primary hand attack.
 *   ATTACK_TYPE_OFFHAND : Offhand attack.
 *   ATTACK_TYPE_RANGED  : Ranged attack.
 *   ATTACK_TYPE_UNARMED : Unarmed attack.
 *   ATTACK_TYPE_TWOHAND : Two-handed weapon attack.
 *   ATTACK_TYPE_BOMB_TOSS     : Alchemist - tossing bombs
 *   ATTACK_TYPE_PRIMARY_SNEAK : impromptu sneak attack, primary hand
 *   ATTACK_TYPE_OFFHAND_SNEAK : impromptu sneak attack, offhand  */
int compute_attack_bonus(struct char_data *ch,     /* Attacker */
                         struct char_data *victim, /* Defender */
                         int attack_type)          /* Type of attack  */
{
  int i = 0;
  int bonuses[NUM_BONUS_TYPES];
  int calc_bab = BAB(ch); /* Start with base attack bonus */
  struct obj_data *wielded = NULL;
  struct char_data *k = NULL;

  /* redundancy necessary due to sometimes arriving here without going through
   * hit()*/
  if (attack_type == ATTACK_TYPE_UNARMED)
    wielded = NULL;
  else
    wielded = get_wielded(ch, attack_type);

  /* Initialize bonuses to 0 */
  for (i = 0; i < NUM_BONUS_TYPES; i++)
    bonuses[i] = 0;

  /* start with our base bonus of strength (or dex with feat/ranged)
               should this have a type?  for now it doesn't... */
  switch (attack_type)
  {
  case ATTACK_TYPE_PSIONICS: /* psionic attacks */
    calc_bab += GET_INT_BONUS(ch);
    calc_bab += IS_PSI_TYPE(ch) / 5;
    /* FALLTHROUGH, no break please */
  case ATTACK_TYPE_OFFHAND:
  case ATTACK_TYPE_PRIMARY:
  case ATTACK_TYPE_PRIMARY_SNEAK:
  case ATTACK_TYPE_OFFHAND_SNEAK:
    if (wielded && HAS_FEAT(ch, FEAT_WEAPON_FINESSE) &&
        is_using_light_weapon(ch, wielded) &&
        GET_DEX_BONUS(ch) > GET_STR_BONUS(ch))
    {
      calc_bab += GET_DEX_BONUS(ch); /* superior bonus is used */
    }
    else if (!wielded && HAS_FEAT(ch, FEAT_UNARMED_STRIKE) &&
             HAS_FEAT(ch, FEAT_WEAPON_FINESSE) && GET_DEX_BONUS(ch) > GET_STR_BONUS(ch))
    {
      calc_bab += GET_DEX_BONUS(ch); /* superior bonus is used */
    }
    else
    {
      calc_bab += GET_STR_BONUS(ch);
    }
    break;
  case ATTACK_TYPE_RANGED:
    calc_bab += GET_DEX_BONUS(ch);
    break;
  case ATTACK_TYPE_BOMB_TOSS:
    calc_bab += GET_DEX_BONUS(ch);
    if (KNOWS_DISCOVERY(ch, ALC_DISC_PRECISE_BOMBS))
    {
      calc_bab += 2;
    }
    break;
  case ATTACK_TYPE_UNARMED:
    if (HAS_FEAT(ch, FEAT_UNARMED_STRIKE) &&
        HAS_FEAT(ch, FEAT_WEAPON_FINESSE) && GET_DEX_BONUS(ch) > GET_STR_BONUS(ch))
    {
      calc_bab += GET_DEX_BONUS(ch); /* superior bonus is used */
    }
    else
    {
      calc_bab += GET_STR_BONUS(ch);
    }

    break;
  case ATTACK_TYPE_TWOHAND:
    calc_bab += GET_STR_BONUS(ch);
    break;
  default:
    break;
  }

  /* NOTICE:  This may be something we phase out, but for basic
   function of the game to date, it is still necessary
   10/14/2014 Zusuk */
  calc_bab += GET_HITROLL(ch);
  /******/

  if (!IS_NPC(ch))
  {
    calc_bab += GET_TEMP_ATTACK_ROLL_BONUS(ch);
    GET_TEMP_ATTACK_ROLL_BONUS(ch) = 0;
  }

  /* Circumstance bonus (stacks)*/
  switch (GET_POS(ch))
  {
  case POS_SITTING:
  case POS_RESTING:
  case POS_SLEEPING:
  case POS_STUNNED:
  case POS_INCAP:
  case POS_MORTALLYW:
  case POS_DEAD:
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 2;
    break;
  case POS_FIGHTING:
  case POS_STANDING:
  default:
    break;
  }

  if (is_judgement_possible(ch, victim, INQ_JUDGEMENT_JUSTICE))
    bonuses[judgement_bonus_type(ch)] = MAX(bonuses[judgement_bonus_type(ch)], get_judgement_bonus(ch, INQ_JUDGEMENT_JUSTICE));

  if (has_teamwork_feat(ch, FEAT_COORDINATED_SHOT) && attack_type == ATTACK_TYPE_RANGED)
  {
    bonuses[BONUS_TYPE_CIRCUMSTANCE] += 1;
    while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
      if (is_flanked(k, victim))
      {
        bonuses[BONUS_TYPE_CIRCUMSTANCE] += 1;
        break;
      }
  }

  // flanking gives a +2 bonus to hit
  if (is_flanked(ch, victim))
  {
    bonuses[BONUS_TYPE_CIRCUMSTANCE] += 2;
    if (has_teamwork_feat(ch, FEAT_OUTFLANK))
      bonuses[BONUS_TYPE_CIRCUMSTANCE] += 2;
  }

  if (IN_ROOM(ch) != NOWHERE && ROOM_AFFECTED(IN_ROOM(ch), RAFF_SACRED_SPACE) && IS_EVIL(ch))
    bonuses[BONUS_TYPE_SACRED] -= 1;

  if (AFF_FLAGGED(ch, AFF_FATIGUED))
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 2;
  if (AFF_FLAGGED(ch, AFF_DAZZLED))
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 1;
  if (IS_FRIGHTENED(ch))
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 2;
  if (ROOM_AFFECTED(IN_ROOM(ch), RAFF_DIFFICULT_TERRAIN))
    bonuses[BONUS_TYPE_CIRCUMSTANCE] -= 2;

  /* Competence bonus */

  /* Enhancement bonus */
  if (wielded)
  {
    bonuses[BONUS_TYPE_ENHANCEMENT] = MAX(bonuses[BONUS_TYPE_ENHANCEMENT], GET_ENHANCEMENT_BONUS(wielded));
    // greater magic weapon
    if (affected_by_spell(ch, SPELL_GREATER_MAGIC_WEAPON))
    {
      bonuses[BONUS_TYPE_ENHANCEMENT] = MAX(bonuses[BONUS_TYPE_ENHANCEMENT], get_char_affect_modifier(ch, SPELL_GREATER_MAGIC_WEAPON, APPLY_SPECIAL));
    }
    bonuses[BONUS_TYPE_ENHANCEMENT] -= get_defending_weapon_bonus(ch, true);
  }
  else if (affected_by_spell(ch, SPELL_GREATER_MAGIC_WEAPON)) // greater magic weapon works on unarmed strikes
    bonuses[BONUS_TYPE_ENHANCEMENT] = MAX(bonuses[BONUS_TYPE_ENHANCEMENT], get_char_affect_modifier(ch, SPELL_GREATER_MAGIC_WEAPON, APPLY_SPECIAL));

  /* ranged includes arrow, what a hack */ /* why is that a hack again? */
  if (can_fire_ammo(ch, TRUE))
  {
    bonuses[BONUS_TYPE_ENHANCEMENT] += GET_ENHANCEMENT_BONUS(GET_EQ(ch, WEAR_AMMO_POUCH)->contains);
  }
  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
  {
    bonuses[BONUS_TYPE_ENHANCEMENT] = MAX(bonuses[BONUS_TYPE_ENHANCEMENT], HAS_FEAT(ch, FEAT_NATURAL_ATTACK) / 2);
  }
  /* monk glove */
  if (MONK_TYPE(ch) && is_bare_handed(ch) && monk_gear_ok(ch) &&
      GET_EQ(ch, WEAR_HANDS) && GET_OBJ_VAL(GET_EQ(ch, WEAR_HANDS), 0))
  {
    bonuses[BONUS_TYPE_ENHANCEMENT] = GET_OBJ_VAL(GET_EQ(ch, WEAR_HANDS), 0);
  }
  /**/

  /* Insight bonus  */

  /* Luck bonus */

  /* Morale bonus */
  if (affected_by_spell(ch, SKILL_SURPRISE_ACCURACY))
  {
    bonuses[BONUS_TYPE_MORALE] += CLASS_LEVEL(ch, CLASS_BERSERKER) / 4 + 1;
  }

  if (affected_by_spell(ch, SPELL_TACTICAL_ACUMEN) && FIGHTING(ch) &&
      (is_flanked(ch, victim) || !CAN_SEE(FIGHTING(ch), ch)))
    bonuses[BONUS_TYPE_MORALE] += get_char_affect_modifier(ch, SPELL_TACTICAL_ACUMEN, APPLY_SPECIAL);

  if (affected_by_aura_of_sin(ch) || affected_by_aura_of_faith(ch))
  {
    bonuses[BONUS_TYPE_MORALE] += 1;
  }

  if (AFF_FLAGGED(ch, AFF_SICKENED))
    bonuses[BONUS_TYPE_UNDEFINED] -= 2;

  /* masterwork bonus */
  // only if the weapon doesn't already have a magical enhancement
  if (wielded && OBJ_FLAGGED(wielded, ITEM_MASTERWORK) && (bonuses[BONUS_TYPE_ENHANCEMENT] <= 0))
    bonuses[BONUS_TYPE_ENHANCEMENT] += 1;

  /* Profane bonus */

  /* Racial bonus */
  if (GET_RACE(ch) == RACE_TRELUX)
  {
    bonuses[BONUS_TYPE_RACIAL] += 4;
  }
  /* light blindness - dayblind, underdark/underworld penalties */
  if (!IS_NPC(ch) && IS_DAYLIT(IN_ROOM(ch)) && HAS_FEAT(ch, FEAT_LIGHT_BLINDNESS))
    bonuses[BONUS_TYPE_RACIAL] -= 1;

  /* Size bonus */
  bonuses[BONUS_TYPE_SIZE] = MAX(bonuses[BONUS_TYPE_SIZE], size_modifiers[GET_SIZE(ch)]);

  /* Unnamed / Undefined (stacks) */

  /*unnamed penalties*/
  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
    bonuses[BONUS_TYPE_UNDEFINED] -= 2;
  /* Modify this to store a player-chosen number for power attack and expertise */
  if (AFF_FLAGGED(ch, AFF_POWER_ATTACK) || AFF_FLAGGED(ch, AFF_EXPERTISE))
    bonuses[BONUS_TYPE_UNDEFINED] -= COMBAT_MODE_VALUE(ch);
  /* spellbattle */
  if (char_has_mud_event(ch, eSPELLBATTLE) && SPELLBATTLE(ch) > 0)
    bonuses[BONUS_TYPE_UNDEFINED] -= SPELLBATTLE(ch);
  /*****/

  /*unnamed bonuses*/

  if (HAS_FEAT(ch, FEAT_WEAPON_EXPERT) && wielded)
    bonuses[BONUS_TYPE_UNDEFINED]++;

  if (HAS_FEAT(ch, FEAT_WEAPON_FOCUS))
  {
    if (wielded)
    {
      /* weapon focus - wielded */
      if (HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily))
      {
        bonuses[BONUS_TYPE_UNDEFINED] += 1;
        /* superior weapon focus - wielded */
        if (HAS_FEAT(ch, FEAT_WEAPON_OF_CHOICE) &&
            HAS_FEAT(ch, FEAT_SUPERIOR_WEAPON_FOCUS))
          bonuses[BONUS_TYPE_UNDEFINED] += 1;
      }
      /* weapon focus - unarmed */
    }
    else if (HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily))
    {
      bonuses[BONUS_TYPE_UNDEFINED] += 1;
      /* superior weapon focus - unarmed */
      if (HAS_FEAT(ch, FEAT_WEAPON_OF_CHOICE) &&
          HAS_FEAT(ch, FEAT_SUPERIOR_WEAPON_FOCUS))
        bonuses[BONUS_TYPE_UNDEFINED] += 1;
    }
  }

  /* greater weapon focus */
  if (HAS_FEAT(ch, FEAT_GREATER_WEAPON_FOCUS))
  {
    if (wielded)
    {
      if (HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_WEAPON_FOCUS), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily))
        bonuses[BONUS_TYPE_UNDEFINED] += 1;
    }
    else if (HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_GREATER_WEAPON_FOCUS), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily))
      bonuses[BONUS_TYPE_UNDEFINED] += 1;
  }

  /* epic weapon specialization */
  if (HAS_FEAT(ch, FEAT_EPIC_WEAPON_SPECIALIZATION))
  {
    if (wielded)
    {
      if (HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_EPIC_WEAPON_SPECIALIZATION), weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily))
        bonuses[BONUS_TYPE_UNDEFINED] += 3;
    }
    else if (HAS_COMBAT_FEAT(ch, feat_to_cfeat(FEAT_EPIC_WEAPON_SPECIALIZATION), weapon_list[WEAPON_TYPE_UNARMED].weaponFamily))
      bonuses[BONUS_TYPE_UNDEFINED] += 3;
  }

  if (victim)
  {
    /* blind fighting */
    if (!CAN_SEE(victim, ch) && !HAS_FEAT(victim, FEAT_BLIND_FIGHT))
      bonuses[BONUS_TYPE_UNDEFINED] += 4;

    /* point blank shot will give +1 bonus to hitroll if you are using a ranged
     * attack in the same room as victim */
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_POINT_BLANK_SHOT) && IN_ROOM(ch) == IN_ROOM(victim))
      bonuses[BONUS_TYPE_UNDEFINED]++;
  }

  if (ch && victim)
  {
    // add charisma bonus for a valid smite situation
    if (affected_by_spell(ch, SKILL_SMITE_EVIL) && smite_evil_target_type(victim))
      bonuses[BONUS_TYPE_UNDEFINED] += GET_CHA_BONUS(victim);
    if (affected_by_spell(ch, SKILL_SMITE_GOOD) && smite_good_target_type(victim))
      bonuses[BONUS_TYPE_UNDEFINED] += GET_CHA_BONUS(victim);

    // smite gives a +4 bonus to ac against their current target.  We'll just translate into a -4 to hit for the opponent
    if (affected_by_spell(victim, SKILL_SMITE_EVIL) && smite_evil_target_type(ch))
      bonuses[BONUS_TYPE_UNDEFINED] -= 4;
    if (affected_by_spell(victim, SKILL_SMITE_GOOD) && smite_good_target_type(ch))
      bonuses[BONUS_TYPE_UNDEFINED] -= 4;
  }

  // Assassin stuff
  bonuses[BONUS_TYPE_UNDEFINED] += GET_MARK_HIT_BONUS(ch);
  GET_MARK_HIT_BONUS(ch) = 0;

  /* EPIC PROWESS feat stacks, +1 for each time the feat is taken. */
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_EPIC_PROWESS))
    bonuses[BONUS_TYPE_UNDEFINED] += HAS_FEAT(ch, FEAT_EPIC_PROWESS);

  if (victim)
  {
    if (HAS_FEAT(ch, FEAT_ALIGNED_ATTACK_GOOD) && IS_GOOD(victim))
      bonuses[BONUS_TYPE_UNDEFINED] += 1;
    else if (HAS_FEAT(ch, FEAT_ALIGNED_ATTACK_EVIL) && IS_EVIL(victim))
      bonuses[BONUS_TYPE_UNDEFINED] += 1;
    else if (HAS_FEAT(ch, FEAT_ALIGNED_ATTACK_CHAOS) && IS_CHAOTIC(victim))
      bonuses[BONUS_TYPE_UNDEFINED] += 1;
    else if (HAS_FEAT(ch, FEAT_ALIGNED_ATTACK_LAW) && IS_LAWFUL(victim))
      bonuses[BONUS_TYPE_UNDEFINED] += 1;
  }

  /* paladin's divine bond, maximum of 6 hitroll: 1 + level / 3 (past level 5) */
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_DIVINE_BOND))
  {
    bonuses[BONUS_TYPE_UNDEFINED] += MIN(6, 1 + MAX(0, (CLASS_LEVEL(ch, CLASS_PALADIN) - 5) / 3));
  }

  /* temporary filler for ki-strike until we get it working right */
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_KI_STRIKE))
    bonuses[BONUS_TYPE_UNDEFINED] += HAS_FEAT(ch, FEAT_KI_STRIKE);

  /* favored enemy - Needs work */
  if (victim && victim != ch && !IS_NPC(ch) && HAS_FEAT(ch, FEAT_FAVORED_ENEMY))
  {
    // checking if we have humanoid favored enemies for PC victims
    if (!IS_NPC(victim) && IS_FAV_ENEMY_OF(ch, RACE_TYPE_HUMANOID))
    {
      bonuses[BONUS_TYPE_MORALE] += CLASS_LEVEL(ch, CLASS_RANGER) / 3 + 2;
      if (HAS_FEAT(ch, FEAT_EPIC_FAVORED_ENEMY))
      {
        bonuses[BONUS_TYPE_MORALE] += 6;
      }
    }
    else if (IS_NPC(victim) && IS_FAV_ENEMY_OF(ch, GET_RACE(victim)))
    {
      bonuses[BONUS_TYPE_MORALE] += CLASS_LEVEL(ch, CLASS_RANGER) / 3 + 2;
      if (HAS_FEAT(ch, FEAT_EPIC_FAVORED_ENEMY))
      {
        bonuses[BONUS_TYPE_MORALE] += 6;
      }
    }
  }

  /* if the victim is using 'come and get me' then they will be vulnerable */
  if (victim && affected_by_spell(victim, SKILL_COME_AND_GET_ME))
  {
    bonuses[BONUS_TYPE_UNDEFINED] += 4;
  }

  /* end bonuses */

  if (char_has_mud_event(ch, eHOLYJAVELIN))
    calc_bab -= 2;

  /*  Check armor/weapon proficiency
   *  If not proficient with weapon, -4 penalty applies. */
  if (wielded)
  {
    if (!is_proficient_with_weapon(ch, GET_WEAPON_TYPE(wielded)))
    {
      if (DEBUGMODE)
      {
        send_to_char(ch, "NOT PROFICIENT\r\n");
      }
      calc_bab -= 4;
    }
  }

  /* Add armor prof here: If not proficient with worn armor, armor check
   * penalty applies to attack roll. */
  if (!is_proficient_with_armor(ch))
  {
    if (DEBUGMODE)
    {
      send_to_char(ch, "NOT PROFICIENT\r\n");
    }
    calc_bab -= 2;
  }

  calc_bab -= get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);

  /* Add up all the bonuses */
  for (i = 0; i < NUM_BONUS_TYPES; i++)
    calc_bab += bonuses[i];

  return (MIN(MAX_BAB, calc_bab));
}

/* compute a combat maneuver bonus (attack) value */
int compute_cmb(struct char_data *ch,     /* Attacker */
                int combat_maneuver_type) /* Type of combat maneuver */
{
  int cm_bonus = 0; /* combat maneuver bonus */

  /* CMB = Base attack bonus + Strength modifier + special size modifier */
  cm_bonus += BAB(ch);
  /* Creatures that are size Tiny or smaller use their Dexterity modifier in place of their Strength modifier to determine their CMB. */
  if (GET_SIZE(ch) > SIZE_TINY)
    cm_bonus += GET_STR_BONUS(ch);
  else
    cm_bonus += GET_DEX_BONUS(ch);
  cm_bonus += size_modifiers[GET_SIZE(ch)];
  /* misc here*/

  if (has_teamwork_feat(ch, FEAT_COORDINATED_MANEUVERS))
    cm_bonus += 2;

  cm_bonus -= get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);

  switch (combat_maneuver_type)
  {
  case COMBAT_MANEUVER_TYPE_KNOCKDOWN:
    break;
  case COMBAT_MANEUVER_TYPE_KICK:
    break;
  case COMBAT_MANEUVER_TYPE_DISARM:
    if (HAS_FEAT(ch, FEAT_IMPROVED_DISARM))
      cm_bonus += 2;
    break;
  case COMBAT_MANEUVER_TYPE_GRAPPLE:
    if (HAS_FEAT(ch, FEAT_IMPROVED_GRAPPLE))
      cm_bonus += 2;
    if (has_teamwork_feat(ch, FEAT_COORDINATED_MANEUVERS))
      cm_bonus += 2; // doubled for grapple
    break;
  case COMBAT_MANEUVER_TYPE_PIN:
    if (HAS_FEAT(ch, FEAT_IMPROVED_GRAPPLE))
      cm_bonus += 2;
    break;
  case COMBAT_MANEUVER_TYPE_INIT_GRAPPLE:
    if (HAS_FEAT(ch, FEAT_IMPROVED_GRAPPLE))
      cm_bonus += 2;
    break;
  case COMBAT_MANEUVER_TYPE_REVERSAL:
    /* for grapple reversals, the person attempting the reversal can use their
                 escape artist instead of their cmb */
    if (compute_ability(ch, ABILITY_ESCAPE_ARTIST) > cm_bonus)
      cm_bonus = compute_ability(ch, ABILITY_ESCAPE_ARTIST);

    if (HAS_FEAT(ch, FEAT_IMPROVED_GRAPPLE))
      cm_bonus += 2;
    break;
  case COMBAT_MANEUVER_TYPE_UNDEFINED:
  default:
    break;
  }

  /*cmb penalty if you aren't attempting grapple related checks while being grappled */
  if (combat_maneuver_type != COMBAT_MANEUVER_TYPE_REVERSAL &&
      combat_maneuver_type != COMBAT_MANEUVER_TYPE_INIT_GRAPPLE &&
      combat_maneuver_type != COMBAT_MANEUVER_TYPE_GRAPPLE &&
      combat_maneuver_type != COMBAT_MANEUVER_TYPE_PIN &&
      (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED)))
    cm_bonus -= 2;

  // send_to_char(ch, "<CMB:%d> ", cm_bonus);

  return cm_bonus;
}

/* compute a combat maneuver defense value */
int compute_cmd(struct char_data *vict,   /* Defender */
                int combat_maneuver_type) /* Type of combat maneuver */
{
  int cm_defense = 9; /* combat maneuver defense, should be 10 but if the difference is 0, then you failed your defense */

  switch (combat_maneuver_type)
  {
  case COMBAT_MANEUVER_TYPE_KNOCKDOWN:
    break;
  case COMBAT_MANEUVER_TYPE_KICK:
    break;
  case COMBAT_MANEUVER_TYPE_DISARM:
    if (HAS_FEAT(vict, FEAT_IMPROVED_DISARM))
      cm_defense += 2;
    break;
  case COMBAT_MANEUVER_TYPE_GRAPPLE:
    if (HAS_FEAT(vict, FEAT_IMPROVED_GRAPPLE))
      cm_defense += 2;
    break;
  case COMBAT_MANEUVER_TYPE_PIN:
    if (HAS_FEAT(vict, FEAT_IMPROVED_GRAPPLE))
      cm_defense += 2;
    break;
  case COMBAT_MANEUVER_TYPE_INIT_GRAPPLE:
    if (HAS_FEAT(vict, FEAT_IMPROVED_GRAPPLE))
      cm_defense += 2;
    break;
  case COMBAT_MANEUVER_TYPE_REVERSAL:
    if (HAS_FEAT(vict, FEAT_IMPROVED_GRAPPLE))
      cm_defense += 2;
    break;
  case COMBAT_MANEUVER_TYPE_UNDEFINED:
  default:
    break;
  }

  if (has_teamwork_feat(vict, FEAT_COORDINATED_DEFENSE))
    cm_defense += 2;

  cm_defense -= get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);

  /* CMD = 10 + Base attack bonus + Strength modifier + Dexterity modifier + special size modifier + miscellaneous modifiers */
  cm_defense += BAB(vict);
  cm_defense += GET_STR_BONUS(vict);
  /* A flat-footed creature does not add its Dexterity bonus to its CMD.*/
  if (!AFF_FLAGGED(vict, AFF_FLAT_FOOTED))
    cm_defense += GET_DEX_BONUS(vict);
  cm_defense += size_modifiers[GET_SIZE(vict)];

  /*cmd penalty if you aren't defending from grapple related checks while being grappled*/
  if (combat_maneuver_type != COMBAT_MANEUVER_TYPE_REVERSAL &&
      combat_maneuver_type != COMBAT_MANEUVER_TYPE_INIT_GRAPPLE &&
      combat_maneuver_type != COMBAT_MANEUVER_TYPE_GRAPPLE &&
      (AFF_FLAGGED(vict, AFF_GRAPPLED) || AFF_FLAGGED(vict, AFF_ENTANGLED)))
    cm_defense -= 2;

  /* misc here */

  /* for grapple reversals, the person attempting the reversal can use their
               escape artist instead of their cmb */
  if (combat_maneuver_type == COMBAT_MANEUVER_TYPE_REVERSAL &&
      compute_ability(vict, ABILITY_ESCAPE_ARTIST) > cm_defense)
    cm_defense = compute_ability(vict, ABILITY_ESCAPE_ARTIST);

  /* immobile defense! */
  if (HAS_FEAT(vict, FEAT_IMMOBILE_DEFENSE) &&
      affected_by_spell(vict, SKILL_DEFENSIVE_STANCE))
    cm_defense += CLASS_LEVEL(vict, CLASS_STALWART_DEFENDER) / 2;

  /* should include: A creature can also add any circumstance,
   * deflection, dodge, insight, luck, morale, profane, and sacred bonuses to
   * AC to its CMD. Any penalties to a creature's AC also apply to its CMD. */

  // send_to_char(vict, "<CMD:%d>", cm_defense);

  return cm_defense;
}

/* basic check for combat maneuver success, + incoming bonus (or negative value for penalty
 * this returns the level of success or failure, which applies in cases such as bull rush
 * 1 or higher = success, 0 or lower = failure */
int combat_maneuver_check(struct char_data *ch, struct char_data *vict,
                          int combat_maneuver_type, int attacker_bonus)
{
  int attack_roll = d20(ch);
  int cm_bonus = attacker_bonus; /* combat maneuver bonus */
  int cm_defense = 0;            /* combat maneuver defense */
  int result = 0;

  if (!ch)
  {
    log("ERR: combat_maneuver_check has no ch! (act.offensive.c)");
    return 0;
  }
  if (!vict)
  {
    log("ERR: combat_maneuver_check has no vict! (act.offensive.c)");
    return 0;
  }

  /* CMB = Base attack bonus + Strength modifier + special size modifier, etc */
  cm_bonus = compute_cmb(ch, combat_maneuver_type) + attack_roll;
  if (HAS_FEAT(ch, FEAT_WEAPON_MASTERY_2) &&
      (GET_EQ(ch, WEAR_WIELD_1) || GET_EQ(ch, WEAR_WIELD_OFFHAND || GET_EQ(ch, WEAR_WIELD_2H))))
    cm_bonus += 6;

  /* CMD = 10 + Base attack bonus + Strength modifier + Dexterity modifier + special size modifier + miscellaneous modifiers */
  cm_defense = compute_cmd(vict, combat_maneuver_type);
  if (HAS_FEAT(vict, FEAT_WEAPON_MASTERY_2) &&
      (GET_EQ(vict, WEAR_WIELD_1) || GET_EQ(vict, WEAR_WIELD_OFFHAND || GET_EQ(vict, WEAR_WIELD_2H))))
    cm_defense += 6;

  /* other modifications based on what type of maneuver (note: the combat maneuver
             type also gets carried to compute_cmb()/compute_cmd() */
  switch (combat_maneuver_type)
  {
  case COMBAT_MANEUVER_TYPE_REVERSAL:
    cm_defense += 5; /* current grappler gets bonus to maintain grapple */
    break;
  default:
    break;
  }

  /* result! */
  result = cm_bonus - cm_defense;

  /* handle results! */
  /* easy outs:  natural 20 roll is success, natural 1 is failure */
  if (attack_roll == 20)
  {
    if (result > 1)
      return result; /* big success? */
    else
      return 1;
  }
  else if (attack_roll == 1)
  {
    if (result < 0)
      return result; /* big failure? */
    else
      return 0;
  }

  else /* roll 2-19 */
    return result;
}

/*
 * Perform an attack, returns the difference of the attacker's roll and the defender's
 * AC.  This value can be negative, and will be on a miss.  Does not deal damage, only
 * checks to see if the attack was successful!
 *
 * Valid attack_type(s) are:
 *   ATTACK_TYPE_PRIMARY : Primary hand attack.
 *   ATTACK_TYPE_OFFHAND : Offhand attack.
 *   ATTACK_TYPE_RANGED  : Ranged attack.
 *   ATTACK_TYPE_UNARMED : Unarmed attack.
 *   ATTACK_TYPE_BOMB_TOSS     : Alchemist - tossing bombs
 *   ATTACK_TYPE_PRIMARY_SNEAK : impromptu sneak attack, primary hand
 *   ATTACK_TYPE_OFFHAND_SNEAK : impromptu sneak attack, offhand   */
int attack_roll(struct char_data *ch,     /* Attacker */
                struct char_data *victim, /* Defender */
                int attack_type,          /* Type of attack */
                int is_touch,             /* TRUE/FALSE this is a touch attack? */
                int attack_number)        /* Attack number, determines penalty. */
{

  //  struct obj_data *wielded = get_wielded(ch, attack_type);

  int attack_bonus = compute_attack_bonus(ch, victim, attack_type);
  int victim_ac = compute_armor_class(ch, victim, is_touch, MODE_ARMOR_CLASS_NORMAL);

  int diceroll = d20(ch);
  int result = ((attack_bonus + diceroll) - victim_ac);

  //  if (attack_type == ATTACK_TYPE_RANGED) {
  /* 1d20 + base attack bonus + Dexterity modifier + size modifier + range penalty */
  /* Range penalty - only if victim is in a different room. */
  //  } else if (attack_type == ATTACK_TYPE_PRIMARY) {
  /* 1d20 + base attack bonus + Strength modifier + size modifier */
  //  } else if (attack_type == ATTACK_TYPE_OFFHAND) {
  /* 1d20 + base attack bonus + Strength modifier + size modifier */
  //  }

  if (DEBUGMODE)
  {
    send_to_char(ch, "DEBUG: attack bonus: %d, diceroll: %d, victim_ac: %d, result: %d\r\n", attack_bonus, diceroll, victim_ac, result);
  }

  return result;
}

/* Perform an attack of opportunity (ch attacks victim).
 * Very simple, single hit with applied penalty (or bonus!) from ch to victim.
 *
 * If return value is != 0, then attack was a success.  If < 0, victim died.  If > 0 then it is the
 * amount of damage dealt.
 */
int attack_of_opportunity(struct char_data *ch, struct char_data *victim, int penalty)
{

  if (AFF_FLAGGED(ch, AFF_FLAT_FOOTED) && !HAS_FEAT(ch, FEAT_COMBAT_REFLEXES))
    return 0;

  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
    return 0;

  if (GET_TOTAL_AOO(ch) < (!HAS_FEAT(ch, FEAT_COMBAT_REFLEXES) ? 1 : GET_DEX_BONUS(ch)))
  {
    GET_TOTAL_AOO(ch)
    ++;
    return hit(ch, victim, TYPE_ATTACK_OF_OPPORTUNITY, DAM_RESERVED_DBC, penalty, FALSE);
  }
  else
  {

    return 0; /* No attack, out of AOOs for this round. */
  }
}

/* Perform an attack of opportunity from every character engaged with ch. */
void attacks_of_opportunity(struct char_data *victim, int penalty)
{
  struct char_data *ch;

  /* Check each char in the room, if it is engaged with victim, give it an AOO */
  for (ch = world[victim->in_room].people; ch; ch = ch->next_in_room)
  {
    /* Check engaged. */
    if (FIGHTING(ch) == victim)
    {

      attack_of_opportunity(ch, victim, penalty);
    }
  }
}

/* Perform an attack of opportunity from every character engaged with ch. */
void teamwork_attacks_of_opportunity(struct char_data *victim, int penalty, int featnum)
{
  struct char_data *ch;

  /* Check each char in the room, if it is engaged with victim, give it an AOO */
  for (ch = world[victim->in_room].people; ch; ch = ch->next_in_room)
  {
    /* Check engaged. */
    if (FIGHTING(ch) == victim && has_teamwork_feat(ch, featnum))
    {
      attack_of_opportunity(ch, victim, penalty);
    }
  }
}

int wildshape_weapon_type(struct char_data *ch)
{
  int w_type_array[NUM_ATTACK_TYPES];
  int weapon_type = TYPE_HIT;
  int count = 0;
  int race = 0;

  /* clear some quick outs */
  if (!ch)
    return TYPE_HIT;
  if (!IS_WILDSHAPED(ch) && !IS_MORPHED(ch))
    return TYPE_HIT;

  if (!IS_MORPHED(ch))
  { /* disguise or wildshape */
    int i;
    race = GET_DISGUISE_RACE(ch);

    /* loop through the race attack type list to add valid types to our array */
    for (i = 0; i < NUM_ATTACK_TYPES; i++)
    {
      if (race_list[race].attack_types[i])
      {
        w_type_array[count++] = i + TYPE_HIT;
      }
    }

    if (DEBUGMODE)
    {
      send_to_char(ch, "Count: %d | ", count);
      for (i = 0; i < count; i++)
      {
        send_to_char(ch, "%d ", w_type_array[i]);
      }
      send_to_char(ch, "\r\n");
    }

    /* list built, pick random */
    if (count <= 0) /* dummy check */
      weapon_type = TYPE_HIT;
    else
      weapon_type = w_type_array[rand_number(0, count - 1)];
  } /* handle old shapechange system */
  else
  {
    count = 0;
    race = IS_MORPHED(ch);
    switch (race)
    {
    case RACE_TYPE_HUMANOID:
    case RACE_TYPE_UNDEAD:
    case RACE_TYPE_GIANT:
    case RACE_TYPE_CONSTRUCT:
    case RACE_TYPE_ELEMENTAL:
    case RACE_TYPE_FEY:
    case RACE_TYPE_MONSTROUS_HUMANOID:
    case RACE_TYPE_OUTSIDER:
    case RACE_TYPE_PLANT:
      w_type_array[count] = TYPE_HIT;
      w_type_array[++count] = TYPE_PUNCH;
      w_type_array[++count] = TYPE_SMASH;
      break;

    case RACE_TYPE_ANIMAL:
    case RACE_TYPE_DRAGON:
    case RACE_TYPE_ABERRATION:
    case RACE_TYPE_MAGICAL_BEAST:
    case RACE_TYPE_OOZE:
    case RACE_TYPE_VERMIN:
    default:
      w_type_array[count] = TYPE_BITE;
      w_type_array[++count] = TYPE_CLAW;
      w_type_array[++count] = TYPE_MAUL;

      break;
    }
    /* pick random */
    weapon_type = w_type_array[rand_number(0, count)];
  }
  if (IS_PIXIE(ch))
  {
    count = 0;
    w_type_array[count] = TYPE_PIERCE;
    w_type_array[++count] = TYPE_STAB;
    w_type_array[++count] = TYPE_STING;
  }

  return weapon_type;
}

/* a function that will return the weapon-type being used based on attack_type
 * and wielded data */
int determine_weapon_type(struct char_data *ch, struct char_data *victim,
                          struct obj_data *wielded, int attack_type)
{
  int w_type = TYPE_HIT, count = 0;
  int w_type_array[NUM_ATTACK_TYPES];

  if (attack_type == ATTACK_TYPE_RANGED)
  { // ranged-attack
    if (!wielded)
      w_type = TYPE_HIT;
    else
    {

      /* check for alternative messages, damageTypes on ranged weapon */
      if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_BLUDGEONING))
      {
        w_type_array[count] = TYPE_BLUDGEON;
        w_type_array[++count] = TYPE_CRUSH;
        w_type_array[++count] = TYPE_POUND;
      }
      if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_PIERCING))
      {
        if (!count)
          w_type_array[count] = TYPE_PIERCE;
        else
          w_type_array[++count] = TYPE_PIERCE;
        w_type_array[++count] = TYPE_STAB;
        w_type_array[++count] = TYPE_THRUST;
      }
      if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_SLASHING))
      {
        if (!count)
          w_type_array[count] = TYPE_SLASH;
        else
          w_type_array[++count] = TYPE_SLASH;
        w_type_array[++count] = TYPE_SLICE;
        w_type_array[++count] = TYPE_HACK;
      }

      if (count)
        w_type = w_type_array[rand_number(0, count)];
      else
        w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
    }
  }
  else if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
  { // !ranged
    count = 0;

    /* check for alternative messages, damageTypes on weapon */
    if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_BLUDGEONING))
    {
      w_type_array[count] = TYPE_BLUDGEON;
      w_type_array[++count] = TYPE_CRUSH;
      w_type_array[++count] = TYPE_POUND;
    }
    if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_PIERCING))
    {
      if (!count)
        w_type_array[count] = TYPE_PIERCE;
      else
        w_type_array[++count] = TYPE_PIERCE;
      w_type_array[++count] = TYPE_STAB;
      w_type_array[++count] = TYPE_THRUST;
    }
    if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_SLASHING))
    {
      if (!count)
        w_type_array[count] = TYPE_SLASH;
      else
        w_type_array[++count] = TYPE_SLASH;
      w_type_array[++count] = TYPE_SLICE;
      w_type_array[++count] = TYPE_HACK;
    }

    if (count)
      w_type = w_type_array[rand_number(0, count)];
    else
      w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  }
  else if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
  {

    w_type = wildshape_weapon_type(ch);
  }
  else
  { /* mobile messages or unarmed */
    if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type + TYPE_HIT; // We are a mob, and we have an attack type, so use that.

    else
      w_type = TYPE_HIT; // Generic default, barehand
  }

  return w_type;
}

/*#define DAMAGE_TYPE_BLUDGEONING        (1 << 0)
  #define DAMAGE_TYPE_SLASHING           (1 << 1)
  #define DAMAGE_TYPE_PIERCING           (1 << 2)
  #define DAMAGE_TYPE_NONLETHAL          (1 << 3) */
/*
int determine_weapon_type(struct obj_data *wielded) {
  int weapon_type_array[NUM_WEAPON_DAMAGE_TYPES];
  int i = 0, count = 0;
  int damage_type = -1;

  if (!wielded && MONK_TYPE(ch))
    return DAMAGE_TYPE_BLUDGEONING;
  else if (!wielded)
    return DAMAGE_TYPE_NONLETHAL;

  // init are array with -1 values
  for (i = 0; i < NUM_WEAPON_DAMAGE_TYPES; i++) {
    weapon_type_array[i] = -1;
  }

  // assign damage types
  if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_BLUDGEONING)) {
    weapon_type_array[0] = DAMAGE_TYPE_BLUDGEONING;
  }
  if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_SLASHING)) {
    weapon_type_array[1] = DAMAGE_TYPE_SLASHING;
  }
  if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_PIERCING)) {
    weapon_type_array[2] = DAMAGE_TYPE_PIERCING;
  }
  if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_NONLETHAL)) {
    weapon_type_array[3] = DAMAGE_TYPE_NONLETHAL;
  }

  while (damage_type == -1 && count < 99) {
    damage_type = weapon_type_array[rand_number(0, NUM_WEAPON_DAMAGE_TYPES-1)];
    count++;
  }

  if (damage_type == -1)
    return DAMAGE_TYPE_NONLETHAL;

  return damage_type;
}
 */

/* arrow imbued with spell will now activate */
void imbued_arrow(struct char_data *ch, struct char_data *vict, struct obj_data *missile)
{
  int original_loc = NOWHERE;

  /* start with the usual dummy checks */
  if (!ch || !vict || !missile)
    return;

  /* imbued? */
  if (GET_OBJ_TYPE(missile) != ITEM_MISSILE || !GET_OBJ_VAL(missile, 1))
    return;

  /* messages */
  act("You watch as $p you launched at $N ignites with magical energy!", FALSE, ch, missile, vict, TO_CHAR);
  act("You watch as $p launched by $n ignites with magical energy!", FALSE, ch, missile, vict, TO_VICT);
  act("..$p ignites with magical energy as $n launches it at $N!", FALSE, ch, missile, vict, TO_NOTVICT);

  if (IN_ROOM(ch) != IN_ROOM(vict))
  {
    /* a location has been found. */
    original_loc = IN_ROOM(ch);
    char_from_room(ch);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(vict)), ZONE_WILDERNESS))
    {
      X_LOC(ch) = world[IN_ROOM(vict)].coords[0];
      Y_LOC(ch) = world[IN_ROOM(vict)].coords[1];
    }

    char_to_room(ch, IN_ROOM(vict));

    /* time to call the magic! */
    call_magic(ch, vict, missile, GET_OBJ_VAL(missile, 1), 0, MAGIC_LEVEL(ch),
               CAST_SPELL);

    /* check if the char is still there */
    if (IN_ROOM(ch) == IN_ROOM(vict))
    {
      char_from_room(ch);

      if (ZONE_FLAGGED(GET_ROOM_ZONE(original_loc), ZONE_WILDERNESS))
      {
        X_LOC(ch) = world[original_loc].coords[0];
        Y_LOC(ch) = world[original_loc].coords[1];
      }

      char_to_room(ch, original_loc);
    }
  }
  else
  {
    /* time to call the magic! */
    call_magic(ch, vict, missile, GET_OBJ_VAL(missile, 1), 0, MAGIC_LEVEL(ch),
               CAST_SPELL);
  }

  /* clear the imbued spell on the arrow */
  GET_OBJ_VAL(missile, 1) = 0;

  return;
}

/* called from hit() */
void handle_missed_attack(struct char_data *ch, struct char_data *victim,
                          int type, int w_type, int dam_type, int attack_type,
                          struct obj_data *missile)
{

  if (affected_by_spell(ch, SPELL_RIGHTEOUS_VIGOR))
  {
    struct affected_type *aff = NULL;
    for (aff = ch->affected; aff; aff = aff->next)
    {
      if (aff->spell == SPELL_RIGHTEOUS_VIGOR)
      {
        aff->modifier = 1;
      }
    }
  }

  /* stunning fist, quivering palm, etc need to be expended even if you miss */
  if (affected_by_spell(ch, SKILL_STUNNING_FIST))
  {
    send_to_char(ch, "You fail to land your stunning fist attack!  ");
    affect_from_char(ch, SKILL_STUNNING_FIST);
  }

  if (affected_by_spell(ch, SKILL_DEATH_ARROW))
  {
    send_to_char(ch, "You fail to land your death arrow attack!  ");
    affect_from_char(ch, SKILL_DEATH_ARROW);
  }

  if (affected_by_spell(ch, SKILL_QUIVERING_PALM))
  {
    send_to_char(ch, "You fail to land your quivering palm attack!  ");
    affect_from_char(ch, SKILL_QUIVERING_PALM);
  }

  if (affected_by_spell(ch, SKILL_SURPRISE_ACCURACY))
  {
    send_to_char(ch, "You fail to land your surprise accuracy attack!  ");
    affect_from_char(ch, SKILL_SURPRISE_ACCURACY);
  }

  if (affected_by_spell(ch, SKILL_POWERFUL_BLOW))
  {
    send_to_char(ch, "You fail to land your powerful blow!  ");
    affect_from_char(ch, SKILL_POWERFUL_BLOW);
  }

  /* Display the flavorful backstab miss messages. This should be changed so we can
   * get rid of the SKILL_ defined (and convert abilities to skills :))
   * it should be noted that it displays miss messages based on weapon-types as well */
  damage(ch, victim, 0, type == SKILL_BACKSTAB ? SKILL_BACKSTAB : w_type,
         dam_type, attack_type);

  /* Ranged miss */
  if (attack_type == ATTACK_TYPE_RANGED)
  {
    /* set off imbued arrow! */
    imbued_arrow(ch, victim, missile);
    /* breakage chance! */
    if (GET_OBJ_VAL(missile, 2) >= dice(1, 100))
    { /*broke!*/
      act("\tnThe $o\tn you fire at $N\tn misses badly and ends up breaking!",
          FALSE, ch, missile, victim, TO_CHAR);
      act("\tnThe $o\tn that $n\tn fired at you misses and ends up breaking!",
          FALSE, ch, missile, victim, TO_VICT | TO_SLEEP);
      act("\tnThe $o\tn that $n\tn fires at $N\tn misses badly and ends up breaking!",
          FALSE, ch, missile, victim, TO_NOTVICT);
      extract_obj(missile);
    }
    else
    { /* Drop the ammo to the ground.*/

      obj_to_room(missile, IN_ROOM(victim));
    }
  }

  return;
}

/* called from hit() */
int handle_successful_attack(struct char_data *ch, struct char_data *victim,
                             struct obj_data *wielded, int dam, int w_type, int type, int diceroll,
                             int is_critical, int attack_type, int dam_type,
                             struct obj_data *missile)
{
  struct affected_type af; /* for crippling strike */
  /* This is a bit of cruft from homeland code - It is used to activate a weapon 'special'
            under certain circumstances.  This could be refactored into something else, but it may
            actually be best to refactor the entire homeland 'specials' system and include it into
            weapon special abilities. */
  char hit_msg[32] = "";
  int sneakdam = 0; /* Additional sneak attack damage. */
  bool victim_is_dead = FALSE;

  if (affected_by_spell(ch, SPELL_RIGHTEOUS_VIGOR))
  {
    struct affected_type *aff = NULL;
    for (aff = ch->affected; aff; aff = aff->next)
    {
      if (aff->spell == SPELL_RIGHTEOUS_VIGOR)
      {
        aff->modifier = MIN(4, aff->modifier + 1);
        GET_HIT(ch) += 2;
        break;
      }
    }
  }

  /* Wrap the message in tags for GUI mode. JTM 1/5/18 */
  // gui_combat_wrap_open(ch);

  if (is_critical)
  {
    strlcpy(hit_msg, "critical", sizeof(hit_msg));
    if (HAS_FEAT(ch, FEAT_EXPLOIT_WEAKNESS))
    {
      victim->player.exploit_weaknesses = 2;
      act("You have exploited $N's weaknesses", TRUE, ch, 0, victim, TO_CHAR);
      act("$n has exploited YOUR weaknesses", TRUE, ch, 0, victim, TO_VICT);
      act("$n has exploited $N's weaknesses", TRUE, ch, 0, victim, TO_NOTVICT);
    }
    if (HAS_REAL_FEAT(ch, FEAT_SPELL_CRITICAL) && !HAS_ELDRITCH_SPELL_CRIT(ch))
    {
      send_to_char(ch, "[\tWSPELL-CRITICAL\tn] ");
      HAS_ELDRITCH_SPELL_CRIT(ch) = true;
    }
    if (!IS_NPC(ch) && HAS_REAL_FEAT(ch, FEAT_SHADOW_MASTER) && victim &&
        !AFF_FLAGGED(victim, AFF_BLIND) && IS_SHADOW_CONDITIONS(ch) && IS_SHADOW_CONDITIONS(victim))
    {
      /* without a bonus to the challenge here, this was completely ineffective -zusuk */
      if (!mag_savingthrow(ch, victim, SAVING_FORT, 10, CAST_INNATE, CLASS_LEVEL(ch, CLASS_SHADOW_DANCER) + ARCANE_LEVEL(ch), ILLUSION))
      {
        send_to_char(ch, "[\tWSHADOW-BLIND SUCCESS!\tn] ");
        new_affect(&af);
        af.spell = SPELL_BLINDNESS;
        af.duration = dice(1, 6);
        af.location = APPLY_NONE;
        af.modifier = 0;
        SET_BIT_AR(af.bitvector, AFF_BLIND);
        affect_to_char(victim, &af);
      }
      else
      {
        send_to_char(ch, "[\tWSHADOW-BLIND FAILED!\tn] ");
      }
    }
  }

  /* Print descriptive tags - This needs some form of control, via a toggle
   * and also should be formatted in some standard way with standard colors.
   * This section also implement the effects of stunning fist, smite and true strike,
   * reccomended: to be moved outta here and put into their own attack
   * routines, then called as an attack action. */
  if (affected_by_spell(ch, SPELL_TRUE_STRIKE))
  {
    send_to_char(ch, "[\tWTRUE-STRIKE\tn] ");
    affect_from_char(ch, SPELL_TRUE_STRIKE);
  }
  if (affected_by_spell(ch, PSIONIC_INEVITABLE_STRIKE))
  {
    send_to_char(ch, "[\tWINEVITABLE-STRIKE\tn] ");
    affect_from_char(ch, PSIONIC_INEVITABLE_STRIKE);
  }
  /* rage powers */
  if (affected_by_spell(ch, SKILL_SURPRISE_ACCURACY))
  {
    send_to_char(ch, "[\tWSURPRISE_ACCURACY\tn] ");
    affect_from_char(ch, SKILL_SURPRISE_ACCURACY);
  }
  int powerful_blow_bonus = 0;
  if (affected_by_spell(ch, SKILL_POWERFUL_BLOW))
  {
    send_to_char(ch, "[\tWPOWERFUL_BLOW\tn] ");
    affect_from_char(ch, SKILL_POWERFUL_BLOW);
    powerful_blow_bonus += CLASS_LEVEL(ch, CLASS_BERSERKER);
    /* what is this?  because we are removing the affect, it won't
             be calculated properly in damage_bonus, so we just tag it on afterwards */
  }
  if (affected_by_spell(ch, SKILL_SMITE_EVIL))
  {
    if (IS_EVIL(victim))
    {
      send_to_char(ch, "[SMITE-EVIL] ");
      send_to_char(victim, "[\tRSMITE-EVIL\tn] ");
      act("$n performs a \tYsmiting\tn attack on $N!",
          FALSE, ch, wielded, victim, TO_NOTVICT);
    }
    if (affected_by_spell(ch, SPELL_FIRE_OF_ENTANGLEMENT) && dice(1, 5) == 1)
    {
      if (!mag_savingthrow(ch, victim, SAVING_REFL, 0, CASTING_TYPE_DIVINE, DIVINE_LEVEL(ch), EVOCATION) &&
          !affected_by_spell(victim, AFFECT_ENTANGLING_FLAMES) && !mag_resistance(ch, victim, 0))
      {
        send_to_char(ch, "[ENTANGLING-FLAMES] ");
        send_to_char(victim, "[\tRENTANGLING-FLAMES\tn] ");
        act("$n entangles $N in chains of flickering flame!", FALSE, ch, wielded, victim, TO_NOTVICT);

        new_affect(&af);
        af.spell = AFFECT_ENTANGLING_FLAMES;
        af.duration = DIVINE_LEVEL(ch);
        af.location = APPLY_NONE;
        af.modifier = 0;
        SET_BIT_AR(af.bitvector, AFF_ENTANGLED);
        affect_to_char(victim, &af);
      }
    }
  }
  if (affected_by_spell(ch, SKILL_SMITE_GOOD))
  {
    if (IS_GOOD(victim))
    {
      send_to_char(ch, "[SMITE-GOOD] ");
      send_to_char(victim, "[\tRSMITE-GOOD\tn] ");
      act("$n performs a \tYsmiting\tn attack on $N!",
          FALSE, ch, wielded, victim, TO_NOTVICT);
    }
  }
  if (affected_by_spell(ch, SKILL_SMITE_DESTRUCTION))
  {
    if (victim)
    {
      send_to_char(ch, "[DESTRUCTIVE-SMITE] ");
      send_to_char(victim, "[\tRDESTRUCTIVE-SMITE\tn] ");
      act("$n performs a \tYsmiting\tn attack on $N!",
          FALSE, ch, wielded, victim, TO_NOTVICT);
    }
  }
  if (affected_by_spell(ch, SKILL_STUNNING_FIST))
  {
    if (!wielded || (OBJ_FLAGGED(wielded, ITEM_KI_FOCUS)) || (weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily == WEAPON_FAMILY_MONK))
    {
      /* check for save */
      if (!savingthrow(victim, SAVING_FORT, 0,
                       ((HAS_FEAT(ch, FEAT_KEEN_STRIKE) * 4) + 10 + (MONK_TYPE(ch) / 2) + GET_WIS_BONUS(ch))))
      {
        send_to_char(ch, "[STUNNING-FIST] ");
        send_to_char(victim, "[\tRSTUNNING-FIST\tn] ");
        act("$n performs a \tYstunning fist\tn attack on $N!",
            FALSE, ch, wielded, victim, TO_NOTVICT);
        if (!char_has_mud_event(victim, eSTUNNED))
        {
          attach_mud_event(new_mud_event(eSTUNNED, victim, NULL), 6 * PASSES_PER_SEC);
        }
      }
      else
      {
        send_to_char(ch, "[\tRstunning fist saved\tn] ");
        send_to_char(victim, "[\tWstunning fist saved\tn] ");
      }
      /* regardless, remove affect */
      affect_from_char(ch, SKILL_STUNNING_FIST);
    }
  }
  if (affected_by_spell(ch, SKILL_QUIVERING_PALM))
  {
    if (!wielded || (OBJ_FLAGGED(wielded, ITEM_KI_FOCUS)) || (weapon_list[GET_WEAPON_TYPE(wielded)].weaponFamily == WEAPON_FAMILY_MONK))
    {
      int keen_strike_bonus = HAS_FEAT(ch, FEAT_KEEN_STRIKE) * 4;
      int quivering_palm_dc = 10 + (MONK_TYPE(ch) / 2) + GET_WIS_BONUS(ch) + keen_strike_bonus;

      send_to_char(ch, "[QUIVERING-PALM] ");
      send_to_char(victim, "[\tRQUIVERING-PALM\tn] ");
      act("$n performs a \tYquivering palm\tn attack on $N!",
          FALSE, ch, wielded, victim, TO_NOTVICT);

      /* apply quivering palm affect, muahahahah */
      if (GET_LEVEL(ch) >= GET_LEVEL(victim) &&
          !savingthrow(victim, SAVING_FORT, 0, quivering_palm_dc))
      {
        /*GRAND SLAM!*/
        act("$N \tRblows up into little pieces\tn as soon as you make contact with your palm!",
            FALSE, ch, wielded, victim, TO_CHAR);
        act("You feel your body \tRblow up in to little pieces\tn as $n touches you!",
            FALSE, ch, wielded, victim, TO_VICT | TO_SLEEP);
        act("You watch as $N's body gets \tRblown into little pieces\tn from a single touch from $n!",
            FALSE, ch, wielded, victim, TO_NOTVICT);
        dam_killed_vict(ch, victim);
        /* ok, now remove quivering palm */
        affect_from_char(ch, SKILL_QUIVERING_PALM);
        return dam;
      }
      else
      { /* failed, but quivering palm will still do damage */
        dam += (GET_WIS_BONUS(ch) + keen_strike_bonus) * (MONK_TYPE(ch) / 2) + 20;
      }
      /* ok, now remove quivering palm */
      affect_from_char(ch, SKILL_QUIVERING_PALM);
    }
  }
  if (affected_by_spell(ch, ABILITY_AFFECT_TRUE_JUDGEMENT))
  {
    int true_judgement_dc = 10 + (CLASS_LEVEL(ch, CLASS_INQUISITOR) / 2) + GET_WIS_BONUS(ch);
    if (victim == GET_JUDGEMENT_TARGET(ch))
    {
      send_to_char(ch, "[TRUE-JUDGEMENT] ");
      send_to_char(victim, "[\tRTRUE-JUDGEMENT\tn] ");
      act("$n performs a \tYtrue judgement\tn attack on $N!", FALSE, ch, wielded, victim, TO_NOTVICT);

      if (!is_immune_death_magic(ch, victim, false) && !savingthrow(victim, SAVING_FORT, 0, true_judgement_dc))
      {

        act("$N \tRblows up into little pieces\tn as soon as you make contact!", FALSE, ch, wielded, victim, TO_CHAR);
        act("You feel your body \tRblow up in to little pieces\tn as $n strikes you!", FALSE, ch, wielded, victim, TO_VICT | TO_SLEEP);
        act("You watch as $N's body gets \tRblown into little pieces\tn from $n's attack!", FALSE, ch, wielded, victim, TO_NOTVICT);
        dam_killed_vict(ch, victim);
        /* ok, now remove quivering palm */
        affect_from_char(ch, ABILITY_AFFECT_TRUE_JUDGEMENT);
        return dam;
      }
      else
      {
        dam += 1 + GET_WIS_BONUS(ch);
      }
      /* ok, now remove quivering palm */
      affect_from_char(ch, ABILITY_AFFECT_TRUE_JUDGEMENT);
    }
  }
  if (affected_by_spell(ch, SKILL_DEATH_ARROW))
  {
    int deatharrow_dc = 10 + CLASS_LEVEL(ch, CLASS_ARCANE_ARCHER) +
                        MAX(GET_CHA_BONUS(ch), GET_INT_BONUS(ch));
    if (can_fire_ammo(ch, TRUE))
    {
      send_to_char(ch, "[ARROW OF DEATH] ");
      send_to_char(victim, "[\tRARROW OF DEATH\tn] ");
      act("$n performs an \tDarrow of death\tn attack on $N!",
          FALSE, ch, wielded, victim, TO_NOTVICT);
      /* apply death arrow affect, muahahahah */
      if (GET_LEVEL(ch) >= GET_LEVEL(victim) &&
          !savingthrow(victim, SAVING_FORT, 0, deatharrow_dc))
      {
        /*GRAND SLAM!*/
        act("$N \tRstops suddenly, then keels over\tn as soon as $p makes contact!",
            FALSE, ch, GET_EQ(ch, WEAR_AMMO_POUCH)->contains, victim, TO_CHAR);
        act("You feel your body \tRshut down as you keel over\tn as $p shot from $n makes contact!",
            FALSE, ch, GET_EQ(ch, WEAR_AMMO_POUCH)->contains, victim, TO_VICT | TO_SLEEP);
        act("You watch as $N \tRstops suddenly then keels over\tn as $p shot from $n makes contact!",
            FALSE, ch, GET_EQ(ch, WEAR_AMMO_POUCH)->contains, victim, TO_NOTVICT);
        dam_killed_vict(ch, victim);
        /* ok, now remove death arrow */
        affect_from_char(ch, SKILL_DEATH_ARROW);
        return dam;
      }
      else
      { /* death arrow will still do damage */
        dam += 1 + MAX(GET_CHA_BONUS(ch), GET_INT_BONUS(ch));
      }
      /* ok, now remove death arrow */
      affect_from_char(ch, SKILL_DEATH_ARROW);
    }
  }

  /* impromptu sneak attack */
  if (attack_type == ATTACK_TYPE_PRIMARY_SNEAK ||
      attack_type == ATTACK_TYPE_OFFHAND_SNEAK)
  {

    /* Display why we are sneak attacking */
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
    {
      send_to_char(ch, "\tW[IMPROMPTU]\tn");
    }
    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
    {
      send_to_char(victim, "\tR[IMPROMPTU]\tn");
    }

    sneakdam = dice(HAS_FEAT(ch, FEAT_SNEAK_ATTACK), 6);

    if (sneakdam)
    {
      send_to_char(ch, "[\tDSNEAK\tn] ");
      send_to_char(victim, "[\tRSNEAK\tn] ");
    }

    /* Calculate regular sneak attack damage. */
  }
  else if (HAS_FEAT(ch, FEAT_SNEAK_ATTACK) &&
           (!KNOWS_DISCOVERY(victim, ALC_DISC_PRESERVE_ORGANS) || dice(1, 4) > 1 || (FIGHTING(ch)->preserve_organs_procced)) &&
           (compute_concealment(victim) == 0) &&
           ((AFF_FLAGGED(victim, AFF_FLAT_FOOTED)) /* Flat-footed */
            || !(has_dex_bonus_to_ac(ch, victim))  /* No dex bonus to ac */
            || is_flanked(ch, victim)              /* Flanked */
            ))
  {

    /* Display why we are sneak attacking */
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
    {
      send_to_char(ch, "\tW[");
      if (AFF_FLAGGED(victim, AFF_FLAT_FOOTED))
        send_to_char(ch, "FF");
      if (!has_dex_bonus_to_ac(ch, victim))
        send_to_char(ch, "Dx");
      if (is_flanked(ch, victim))
        send_to_char(ch, "Fk");
      send_to_char(ch, "]\tn");
    }
    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
    {
      send_to_char(victim, "\tR[");
      if (AFF_FLAGGED(victim, AFF_FLAT_FOOTED))
        send_to_char(victim, "FF");
      if (!has_dex_bonus_to_ac(ch, victim))
        send_to_char(victim, "Dx");
      if (is_flanked(ch, victim))
        send_to_char(victim, "Fk");
      send_to_char(victim, "]\tn");
    }

    sneakdam = dice(HAS_FEAT(ch, FEAT_SNEAK_ATTACK), 6);

    if (sneakdam)
    {
      send_to_char(ch, "[\tDSNEAK\tn] ");
      send_to_char(victim, "[\tRSNEAK\tn] ");
    }
  }

  /* ok we checked has_dex_bonus_to_ac(), if the victim was feinted, then
   remove the feint affect on them now*/
  if (affected_by_spell(victim, SKILL_FEINT))
  {
    affect_from_char(victim, SKILL_FEINT);
  }

  /* Calculate damage for this hit */
  dam += compute_hit_damage(ch, victim, w_type, diceroll, 0,
                            is_critical, attack_type);
  if (type == TYPE_ATTACK_OF_OPPORTUNITY && has_teamwork_feat(ch, FEAT_PAIRED_OPPORTUNISTS))
    dam += 2;
  dam += powerful_blow_bonus; /* ornir is going to yell at me for this :p  -zusuk */

  /* This comes after computing the other damage since sneak attack damage
   * is not affected by crit multipliers. */
  dam += sneakdam;

  /* We hit with a ranged weapon, victim gets a new arrow, stuck neatly in his butt. */
  if (attack_type == ATTACK_TYPE_RANGED)
  {
    /* set off imbued arrow! */
    imbued_arrow(ch, victim, missile);
    /* the victim gets to inherit the bullet, right in the bum! */
    obj_to_char(missile, victim); /* note bullet will be in inventory, not in the bum eq-slot */
  }

  /* Melee warding modifies damage. */
  /* once Damage Reduction is ready to launch, this should be removed */
  if ((dam = handle_warding(ch, victim, dam)) == -1)
    return (HIT_MISS);

  /* Apply Damage Reduction */
  if ((dam = apply_damage_reduction(ch, victim, wielded, dam, FALSE)) == -1)
    return (HIT_MISS); /* This should be changed to something more reasonable */

  /* ok we are about to do damage() so here we are adding a special counter-attack
               for berserkers that is suppose to fire BEFORE damage is done to vict */
  if (ch != victim &&
      affected_by_spell(victim, SKILL_COME_AND_GET_ME) &&
      affected_by_spell(victim, SKILL_RAGE))
  {
    GET_TOTAL_AOO(victim)
    --; /* free aoo and will be incremented in the function */
    attack_of_opportunity(victim, ch, 0);

    /* dummy check */
    update_pos(ch);
    if (GET_POS(ch) <= POS_INCAP)
      return (HIT_MISS);
  }
  /***** end counter attacks ******/

  /* if the 'type' of hit() requires special handling, do it here */
  switch (type)
  {
    /* More SKILL_ garbage - This needs a better mechanic.  */
  case SKILL_BACKSTAB:
    dam += 4; /* base backstab bonus to damage */
    if (damage(ch, victim, (dam * backstab_mult(ch)), SKILL_BACKSTAB, dam_type, attack_type) < 0)
      victim_is_dead = TRUE;

    break;
  default:
    /* Here we manage the racial specials, Trelux have claws and can not use weapons. */
    if (GET_RACE(ch) == RACE_TRELUX)
    {
      if (damage(ch, victim, dam, TYPE_CLAW, dam_type, attack_type) < 0)
      {
        victim_is_dead = TRUE;
      }
    }
    else
    {
      /* charging combat maneuver */
      if (AFF_FLAGGED(ch, AFF_CHARGING))
      {
        send_to_char(ch, "You \tYcharge\tn: ");
        send_to_char(victim, "%s \tYcharges\tn toward you: ", GET_NAME(ch));
        act("$n \tYcharges\tn toward $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
      }

      /* So do damage! (We aren't trelux, so do it normally) */
      if (damage(ch, victim, dam, w_type, dam_type, attack_type) < 0)
        victim_is_dead = TRUE;

      if (AFF_FLAGGED(ch, AFF_CHARGING))
      { /* only a single strike */
        affect_from_char(ch, SKILL_CHARGE);
      }
    }
    break;
  }

  /* 20% chance to poison as a trelux. This could be made part of the general poison code, once that is
   * implemented, also, shouldn't they be able to control if they poison or not?  Why not make them envenom
   * their claws before an attack? NOTE: poison bite feat is here as well, generalized damage message */
  if (!victim_is_dead && dam && !rand_number(0, 5))
  {
    if ((GET_RACE(ch) == RACE_TRELUX && is_bare_handed(ch)) || HAS_FEAT(ch, FEAT_POISON_BITE))
    {
      /* We are just using the poison spell for this...Maybe there would be a better way, some unique poison?
       * Note the CAST_INNATE, this removes armor spell failure from the call. */
      act("You inject \tgVenom\tn into $N's wound!",
          FALSE, ch, wielded, victim, TO_CHAR);
      act("\tgVenom\tn from $n is injected into your wounds!",
          FALSE, ch, wielded, victim, TO_VICT | TO_SLEEP);
      act("\tgVenom\tn from $n is injected into $N's wounds!",
          FALSE, ch, wielded, victim, TO_NOTVICT);
      call_magic(ch, victim, 0, SPELL_POISON, 0, GET_LEVEL(ch), CAST_INNATE);
    }
  }

  /* crippling strike */
  if (sneakdam)
  {
    if (dam && !victim_is_dead && HAS_FEAT(ch, FEAT_CRIPPLING_STRIKE) &&
        !affected_by_spell(victim, SKILL_CRIP_STRIKE))
    {

      new_affect(&af);
      af.spell = SKILL_CRIP_STRIKE;
      af.duration = 10;
      af.location = APPLY_STR;
      af.modifier = -(dice(2, 4));
      affect_to_char(victim, &af);

      act("Your well placed attack \tTcripples\tn $N!",
          FALSE, ch, wielded, victim, TO_CHAR);
      act("A well placed attack from $n \tTcripples\tn you!",
          FALSE, ch, wielded, victim, TO_VICT | TO_SLEEP);
      act("A well placed attack from $n \tTcripples\tn $N!",
          FALSE, ch, wielded, victim, TO_NOTVICT);
    }
  }

  if (affected_by_spell(victim, SPELL_STUNNING_BARRIER))
  {
    if (!mag_savingthrow(victim, ch, SAVING_WILL, 0, CASTING_TYPE_ANY, CASTER_LEVEL(victim), CONJURATION))
    {
      new_affect(&af);
      af.spell = SPELL_AFFECT_STUNNING_BARRIER;
      af.duration = 1;
      af.location = APPLY_AC_NEW;
      af.modifier = -4;
      affect_to_char(ch, &af);

      act("$n's stunning barrier stuns you!", FALSE, ch, wielded, victim, TO_CHAR);
      act("Your stunning barrier stuns $N!", FALSE, ch, wielded, victim, TO_VICT);
      act("$n's stunning barrier stuns $N!", FALSE, ch, wielded, victim, TO_NOTVICT);

      affect_from_char(victim, SPELL_STUNNING_BARRIER);
    }
  }

  /* weapon spells - deprecated, although many weapons still have these.  Weapon Special Abilities supercede
   * this implementation. */
  if (ch && victim && wielded && !victim_is_dead)
    weapon_spells(ch, victim, wielded);

  /* Weapon special abilities that trigger on hit. */
  if (ch && victim && (wielded || using_monk_gloves(ch)) && !victim_is_dead)
    process_weapon_abilities(wielded, ch, victim, ACTMTD_ON_HIT, NULL);
  if (IS_EFREETI(ch))
    damage(ch, victim, dice(2, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);

  /* our primitive weapon-poison system, needs some love */
  if (ch && victim && (wielded || missile || IS_TRELUX(ch)) && !victim_is_dead)
    weapon_poison(ch, victim, wielded, missile);

  /* special weapon (or gloves for monk) procedures.  Need to implement something similar for the new system. */
  if (ch && victim && wielded && !victim_is_dead)
    weapon_special(wielded, ch, hit_msg);
  else if (ch && victim && GET_EQ(ch, WEAR_HANDS) && !victim_is_dead && is_bare_handed(ch))
    weapon_special(GET_EQ(ch, WEAR_HANDS), ch, hit_msg);

  /* vampiric curse will do some minor healing to attacker */
  if (!IS_UNDEAD(victim) && IS_AFFECTED(victim, AFF_VAMPIRIC_CURSE))
  {
    send_to_char(ch, "\tWYou feel slightly better as you land an attack!\r\n");
    GET_HIT(ch) += MIN(GET_MAX_HIT(ch) - GET_HIT(ch), dice(1, 10));
  }

  /* vampiric touch will do some healing to attacker */
  if (dam > 0 && !IS_UNDEAD(victim) && IS_AFFECTED(ch, AFF_VAMPIRIC_TOUCH))
  {
    send_to_char(ch, "\tWYou feel \tRvampiric\tn \tWenergy heal you as you "
                     "land an attack!\r\n");
    GET_HIT(ch) += MIN(GET_MAX_HIT(ch) - GET_HIT(ch), dam);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_VAMPIRIC_TOUCH);
  }

  // damage inflicting shields, like fire shield
  damage_shield_check(ch, victim, attack_type, dam);

  return dam;
}

/* damage inflicting shields, like fire shield */
int damage_shield_check(struct char_data *ch, struct char_data *victim,
                        int attack_type, int dam)
{
  int return_val = 0;
  int energy = 0;
  int save_type = 0;
  int power_resist_bonus = 0;
  int dam_bonus = 0;

  if (attack_type != ATTACK_TYPE_RANGED)
  {
    if (dam && victim && GET_HIT(victim) >= -1 &&
        IS_AFFECTED(victim, AFF_CSHIELD))
    { // cold shield
      return_val = damage(victim, ch, dice(1, 6), SPELL_CSHIELD_DAM, DAM_COLD, attack_type);
    }
    else if (dam && victim && GET_HIT(victim) >= -1 &&
             IS_AFFECTED(victim, AFF_FSHIELD))
    { // fire shield
      return_val = damage(victim, ch, dice(1, 6), SPELL_FSHIELD_DAM, DAM_FIRE, attack_type);
    }
    else if (dam && victim && GET_HIT(victim) >= -1 &&
             IS_AFFECTED(victim, AFF_ESHIELD))
    { // electric shield
      return_val = damage(victim, ch, dice(1, 6), SPELL_ESHIELD_DAM, DAM_ELECTRIC, attack_type);
    }
    else if (dam && victim && GET_HIT(victim) >= -1 &&
             IS_AFFECTED(victim, AFF_ASHIELD))
    { // acid shield
      return_val = damage(victim, ch, dice(2, 6), SPELL_ASHIELD_DAM, DAM_ACID, attack_type);
    }
    if (dam && victim && GET_HIT(victim) >= -1 && affected_by_spell(victim, PSIONIC_EMPATHIC_FEEDBACK))
    {
      if (!power_resistance(ch, victim, 0))
        if (!is_immune_mind_affecting(ch, victim, 0))
          if (!mag_savingthrow(ch, victim, SAVING_WILL, 0, CAST_SPELL, GET_PSIONIC_LEVEL(ch), 0))
            return_val = damage(victim, ch, dice(4, get_char_affect_modifier(victim, PSIONIC_EMPATHIC_FEEDBACK, APPLY_SPECIAL)), PSIONIC_EMPATHIC_FEEDBACK, DAM_MENTAL, attack_type);
    }
    if (dam && affected_by_spell(victim, PSIONIC_ENERGY_RETORT) && !victim->char_specials.energy_retort_used)
    {
      victim->char_specials.energy_retort_used = true;

      energy = get_char_affect_modifier(victim, PSIONIC_ENERGY_RETORT, APPLY_SPECIAL);
      if (energy == DAM_ELECTRIC || energy == DAM_SOUND)
      {
        GET_DC_BONUS(victim) += 2;
        power_resist_bonus -= 2;
      }

      // let's do this now, because if resisted we don't need to worry about below code.
      if (!power_resistance(victim, ch, power_resist_bonus))
      {
        if (energy == DAM_COLD)
          save_type = SAVING_FORT;
        else
          save_type = SAVING_REFL;

        if (energy == DAM_FIRE || energy == DAM_COLD || energy == DAM_ACID)
          dam_bonus = 4;

        if (mag_savingthrow(victim, ch, save_type, 0, CAST_SPELL, GET_PSIONIC_LEVEL(victim), 0))
        {
          return_val = damage(victim, ch, (dice(4, 6) + dam_bonus) / 2, PSIONIC_ENERGY_RETORT, energy, attack_type);
        }
        else
        {
          return_val = damage(victim, ch, dice(4, 6) + dam_bonus, PSIONIC_ENERGY_RETORT, energy, attack_type);
        }
      }
    }
  }

  return return_val;
}

/* primary function for a single melee attack
   ch -> attacker, victim -> defender
   type -> SKILL_  /  SPELL_  / TYPE_ / etc. (attack of opportunity)
   dam_type -> DAM_FIRE / etc (not used here, passed to dam() function)
   penalty ->  (or bonus)  applied to hitroll, BAB multi-attack for example
   attack_type ->
    #define ATTACK_TYPE_PRIMARY   0
    #define ATTACK_TYPE_OFFHAND   1
    #define ATTACK_TYPE_RANGED    2
    #define ATTACK_TYPE_UNARMED   3
    #define ATTACK_TYPE_TWOHAND   4 //doesn't really serve any purpose?
    #define ATTACK_TYPE_BOMB_TOSS 5
    #define ATTACK_TYPE_PRIMARY_SNEAK   6  //impromptu sneak attack
    #define ATTACK_TYPE_OFFHAND_SNEAK   7  //impromptu sneak attack
   Attack queue will determine what kind of hit this is. */
#define DAM_MES_LENGTH 20
int hit(struct char_data *ch, struct char_data *victim, int type, int dam_type,
        int penalty, int attack_type)
{
  int w_type = 0,         /* Weapon type? */
      victim_ac = 0,      /* Target's AC, from compute_ac(). */
      calc_bab = penalty, /* ch's base attack bonus for the attack. */
      diceroll = 0,       /* ch's attack roll. */
      can_hit = 0,        /* ch successfully hit? */
      dam = 0;            /* Damage for the attack, with mods. */

  bool is_critical = FALSE;

  char buf[DAM_MES_LENGTH] = {'\0'}; /* Damage message buffers. */
  char buf1[DAM_MES_LENGTH] = {'\0'};

  bool same_room = FALSE; /* Is the victim in the same room as ch? */

  struct obj_data *ammo_pouch = GET_EQ(ch, WEAR_AMMO_POUCH); /* For ranged combat. */
  struct obj_data *missile = NULL;                           /* For ranged combat. */

  if (!ch || !victim)
    return (HIT_MISS); /* ch and victim exist? */

  // each hit we want to reset the preserve organs proc.  This is to prevent double dipping
  // from sneak attacks and crits
  victim->preserve_organs_procced = FALSE;

  struct obj_data *wielded = get_wielded(ch, attack_type); /* Wielded weapon for this hand (uses offhand) */
                                                           /*if (GET_EQ(ch, WEAR_WIELD_2H) && attack_type != ATTACK_TYPE_RANGED)
            attack_type = ATTACK_TYPE_TWOHAND;*/
  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
    wielded = NULL;

  /* First - check the attack queue.  If we have a queued attack, dispatch!
              The attack queue should be more tightly integrated into the combat system.  Basically,
              if there is no attack queued up, we perform the default: A standard melee attack.
              The parameter 'penalty' allows an external procedure to call hit with a to hit modifier
              of some kind - This need not be a penalty, but can rather be a bonus (due to a charge or some
              other buff or situation.)  It is not hit()'s job to determine these bonuses.' */
  if (pending_attacks(ch))
  {
    /* Dequeue the pending attack action.*/
    struct attack_action_data *attack = dequeue_attack(GET_ATTACK_QUEUE(ch));
    /* Execute the proper function pointer for that attack action. Notice the painfully bogus
                  parameters.  Needs improvement. */
    ((*attack_actions[attack->attack_type])(ch, attack->argument, -1, -1));
    /* Currently no way to get a result from these kinds of actions, so return something bogus.
                  Needs improvement. */
    return (HIT_RESULT_ACTION);
  }

  /* if we come into the hit() function anticipating a ranged attack, we are
   examining obvious cases where the attack will fail */
  if (attack_type == ATTACK_TYPE_RANGED)
  {
    if (ammo_pouch)
      /* If we need a global variable to make some information available outside
       *  this function, then we might have a bit of an issue with the design.
       * Set the current missile to the first missile in the ammo pouch. */
      last_missile = missile = ammo_pouch->contains;
    if (!missile)
    { /* no ammo = miss for ranged attacks*/
      send_to_char(ch, "You have no ammo!\r\n");
      return (HIT_MISS);
    }

    /* reloading type of weapons, such as crossbows */
    if (is_reloading_weapon(ch, wielded, TRUE))
    {
      if (!weapon_is_loaded(ch, wielded, FALSE))
      {
        FIRING(ch) = FALSE;
        return (HIT_NEED_RELOAD);
      }
    }
  }

  /* Activate any scripts on this mob OR PLAYER. */
  fight_mtrigger(ch); // fight trig
  /* If the mob can't fight, just return an automatic miss. */
  if (!MOB_CAN_FIGHT(ch))
  { /* this mob can't hit */
    send_to_char(ch, "But you can't perform a normal melee attack routine!\r\n");
    return (HIT_MISS);
  }
  /* ^^ So this means that non fighting mobs with a fight trigger get their fight
    trigger executed if they are ever 'ch' in a run of perform_violence.  Ok.*/

  /* If this is a melee attack, check if the target and the aggressor are in
    the same room. */
  if (attack_type != ATTACK_TYPE_RANGED && IN_ROOM(ch) != IN_ROOM(victim))
  {
    /* Not in the same room, so stop fighting. */
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
    {
      stop_fighting(ch);
    }
    /* Automatic miss. */
    return (HIT_MISS);
  }
  else if (IN_ROOM(ch) == IN_ROOM(victim))
    same_room = TRUE; /* ch and victim are in the same room, great. */

  /* Make sure that ch and victim have an updated position. */
  update_pos(ch);
  update_pos(victim);

  /* If ch is dead (or worse) or victim is dead (or worse), return an automatic miss. */
  if (GET_POS(ch) <= POS_DEAD || GET_POS(victim) <= POS_DEAD)
    return (HIT_MISS);

  // added these two checks in case totaldefense is successful on opening attack -zusuk
  if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
      ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    stop_fighting(ch);
    return (HIT_MISS);
  }

  /* ranged attack and victim in peace room */
  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_PEACEFUL))
  {
    send_to_char(ch, "That room just has such a peaceful, easy feeling...\r\n");
    stop_fighting(ch);
    return (HIT_MISS);
  }

  /* single file rooms restriction */
  if (!FIGHTING(ch))
  {
    if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE) &&
        (ch->next_in_room != victim && victim->next_in_room != ch))
      return (HIT_MISS);
  }

  /* Here is where we start fighting, if we were not fighting before. */
  if (victim != ch)
  {
    if (same_room && GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL)) // ch -> vict
      set_fighting(ch, victim);                                           /* Start fighting in one direction. */
                                                                          // vict -> ch
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL))
    {
      if (same_room)
        set_fighting(victim, ch); /* Start fighting in the other direction. */
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
        remember(victim, ch); /* If I am a mob with memory, remember the bastard. */
    }
  }

  /* At this point, ch should be fighting vict and vice versa, unless ch and vict
    are in different rooms and this is a ranged attack. */

  // determine weapon type, potentially a deprecated function
  w_type = determine_weapon_type(ch, victim, wielded, attack_type);
  /* some ranged attack handling */
  if (attack_type == ATTACK_TYPE_RANGED)
  {
    // tag missile so that only this char collects it.
    MISSILE_ID(missile) = GET_IDNUM(ch);
    /* Remove the missile from the ammo_pouch. */
    obj_from_obj(missile);
    /* if this was a weapon that was loaded, unload */
    if (wielded && GET_OBJ_VAL(wielded, 5) > 0)
      GET_OBJ_VAL(wielded, 5)
    --;

    /* we are checking here for spec procs associated with arrows */
#define WARBOW_VNUM 132115
    if (is_wearing(ch, WARBOW_VNUM) && !rand_number(0, 10))
    {
      act("$p\tw \tWsparks with power\tw as it is fired toward $N!\tn",
          FALSE, ch, wielded, victim, TO_CHAR);
      act("$p\tw used by $n \tWsparks with power\tw as it is fired toward you!\tn",
          FALSE, ch, wielded, victim, TO_VICT);
      act("$p\tw used by $n \tWsparks with power\tw as it is fired toward $N!\tn",
          FALSE, ch, wielded, victim, TO_ROOM);

      dam += dice(14, 5);
    }
#undef WARBOW_VNUM
  }

  /* Get the important numbers : ch's Attack bonus and victim's AC
   * attack rolls: 1 = stumble, 20 = hit, also check for threat-range for criticals */
  victim_ac = compute_armor_class(ch, victim, FALSE, MODE_ARMOR_CLASS_NORMAL);

  switch (attack_type)
  {

  case ATTACK_TYPE_OFFHAND: /*fallthrough*/ /* secondary or 'off' hand */
  case ATTACK_TYPE_OFFHAND_SNEAK:           /* impromptu */
    if (!wielded)
    {
      calc_bab += compute_attack_bonus(ch, victim, ATTACK_TYPE_UNARMED);
    }
    else
    {
      calc_bab += compute_attack_bonus(ch, victim, attack_type);
    }
    break;

  case ATTACK_TYPE_RANGED: /* ranged weapon */
    calc_bab += compute_attack_bonus(ch, victim, attack_type);
    break;

  case ATTACK_TYPE_TWOHAND: /* two handed weapon */
    calc_bab += compute_attack_bonus(ch, victim, attack_type);
    break;

  case ATTACK_TYPE_PRIMARY: /* primary hand and default */ /*fallthrough*/
  case ATTACK_TYPE_PRIMARY_SNEAK: /* impromptu */          /*fallthrough*/
  default:
    if (!wielded)
      calc_bab += compute_attack_bonus(ch, victim, ATTACK_TYPE_UNARMED);
    else
    {
      calc_bab += compute_attack_bonus(ch, victim, attack_type);
    }
    break;
  }

  if (type == TYPE_ATTACK_OF_OPPORTUNITY)
  {
    if (HAS_FEAT(victim, FEAT_MOBILITY) && has_dex_bonus_to_ac(ch, victim))
      victim_ac += 4;
    if (HAS_FEAT(victim, FEAT_ENHANCED_MOBILITY) && has_dex_bonus_to_ac(ch, victim))
      victim_ac += 4;
    if (has_teamwork_feat(ch, FEAT_PAIRED_OPPORTUNISTS))
      victim_ac -= 4;
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
      send_to_char(ch, "\tW[\tRAOO\tW]\tn");
    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
      send_to_char(victim, "\tW[\tRAOO\tW]\tn");
  }

  if (HAS_FEAT(ch, FEAT_WEAPON_TRAINING))
    diceroll = d20(ch) + HAS_FEAT(ch, FEAT_WEAPON_TRAINING);
  else
    diceroll = d20(ch);
  if (is_critical_hit(ch, wielded, diceroll, calc_bab, victim_ac) && !IS_IMMUNE_CRITS(victim))
  {
    can_hit = TRUE;
    is_critical = TRUE;
    /* old critical message was here -zusuk */
  }
  else if (diceroll == 20)
  { /*auto hit, not critical though*/
    can_hit = TRUE;
  }
  else if (!AWAKE(victim))
  {
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
      send_to_char(ch, "\tW[down!]\tn");
    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
      send_to_char(victim, "\tR[down!]\tn");
    can_hit = TRUE;
  }
  else if (diceroll == 1)
  {
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
      send_to_char(ch, "[stum!]");
    if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
      send_to_char(victim, "[stum!]");
    can_hit = FALSE;
  }
  else
  {
    can_hit = (calc_bab + diceroll >= victim_ac);
  }
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
  {
    snprintf(buf1, sizeof(buf1), "\tW[R:%2d]\tn", diceroll);
    snprintf(buf, sizeof(buf), "%7s", buf1);
    send_to_char(ch, "%s", buf);
  }
  if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
  {
    snprintf(buf1, sizeof(buf1), "\tR[R:%2d]\tn", diceroll);
    snprintf(buf, sizeof(buf), "%7s", buf1);
    send_to_char(victim, "%s", buf);
  }

  if (DEBUGMODE)
  {

    send_to_char(ch, "\tc{T:%d+", calc_bab);
    send_to_char(ch, "D:%d>=", diceroll);
    send_to_char(ch, "AC:%d}\tn", victim_ac);
  }

  /* Total Defense calculation -
   * This only applies if the victim is in totaldefense mode and is based on
   * the totaldefense 'skill'.  You can not totaldefense if you are casting and you have
   * a number of totaldefense attempts equal to the attacks granted by your BAB. */
  int total_defense_DC = calc_bab + diceroll;
  int total_defense_attempt = 0;
  if (!IS_NPC(victim) &&
      compute_ability(victim, ABILITY_TOTAL_DEFENSE) &&
      TOTAL_DEFENSE(victim) &&
      AFF_FLAGGED(victim, AFF_TOTAL_DEFENSE) &&
      !IS_CASTING(victim) &&
      GET_POS(victim) >= POS_SITTING &&
      attack_type != ATTACK_TYPE_RANGED &&
      !is_critical)
  {

    /* -2 penalty to totaldefense attempts if you are sitting, basically.  You will never ever
     * get here if you are in a lower position than sitting, so the 'less than' is
     * redundant. */
    if (GET_POS(victim) <= POS_SITTING)
      total_defense_DC += 2;

    if (!(total_defense_attempt = skill_check(victim, ABILITY_TOTAL_DEFENSE, total_defense_DC)))
    {
      send_to_char(victim, "You failed to \tcdefend\tn yourself from the attack from %s!  ",
                   GET_NAME(ch));
    }
    else if ((total_defense_attempt + (2 * HAS_FEAT(ch, FEAT_RIPOSTE))) >= 10)
    {
      /* We can riposte, as the 'skill check' was 10 or more higher than the DC. */
      send_to_char(victim, "You deftly \tcriposte the attack\tn from %s!\r\n",
                   GET_NAME(ch));
      send_to_char(ch, "%s \tCdefends\tn from your attack and counterattacks!\r\n",
                   GET_NAME(victim));
      act("$N \tDdefends\tn from an attack from $n!", FALSE, ch, 0, victim,
          TO_NOTVICT);

      /* fire any parry specs we might have */
      struct obj_data *opp_wpn = get_wielded(victim, ATTACK_TYPE_PRIMARY);
      if (opp_wpn && !rand_number(0, 3))
      {
        int (*name)(struct char_data * victim, void *me, int cmd, const char *argument);
        name = obj_index[GET_OBJ_RNUM(opp_wpn)].func;
        if (name)
          (name)(victim, opp_wpn, 0, "parry");
      }

      /* Encapsulate this?  We need better control of 'hit()s' */
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, attack_type);
      TOTAL_DEFENSE(victim)
      --;
      return (HIT_MISS);
    }
    else
    {
      send_to_char(victim, "You \tcdefend\tn yourself from the attack from %s!\r\n",
                   GET_NAME(ch));
      send_to_char(ch, "%s \tCdefends\tn from your attack!\r\n", GET_NAME(victim));
      act("$N \tDdefends\tn from an attack from $n!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      TOTAL_DEFENSE(victim)
      --;

      /* fire any parry specs we might have */
      struct obj_data *opp_wpn = get_wielded(victim, ATTACK_TYPE_PRIMARY);
      if (opp_wpn && !rand_number(0, 3))
      {
        int (*name)(struct char_data * victim, void *me, int cmd, const char *argument);
        name = obj_index[GET_OBJ_RNUM(opp_wpn)].func;
        if (name)
          (name)(victim, opp_wpn, 0, "parry");
      }

      return (HIT_MISS);
    }
  } /* End of totaldefense */

  if (attack_type != ATTACK_TYPE_RANGED && affected_by_spell(ch, SPELL_GASEOUS_FORM) && AFF_FLAGGED(victim, AFF_WIND_WALL))
  {
    act("You are unable to get close enough to $N to complete your attack.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n is unable to get close enough to You to complete $s attack.", FALSE, ch, 0, victim, TO_VICT);
    act("$n is unable to get close enough to $N to complete $s attack.", FALSE, ch, 0, victim, TO_NOTVICT);

    return (HIT_MISS);
  }

  /* Once per round when your mount is hit in combat, you may attempt a Ride
   * check (as an immediate action) to negate the hit. The hit is negated if
   * your Ride check result is greater than the opponent's attack roll.*/
  if (RIDING(victim) && HAS_FEAT(victim, FEAT_MOUNTED_COMBAT) &&
      MOUNTED_BLOCKS_LEFT(victim) > 0)
  {
    int mounted_block_dc = calc_bab + diceroll;
    int mounted_block_bonus = compute_ability(victim, ABILITY_RIDE) + d20(victim);
    if (mounted_block_dc <= mounted_block_bonus)
    {
      send_to_char(victim, "You \tcmaneuver %s to block\tn the attack from %s!\r\n",
                   GET_NAME(RIDING(victim)), GET_NAME(ch));
      send_to_char(ch, "%s \tCmaneuvers %s to block\tn your attack!\r\n", GET_NAME(victim), GET_NAME(RIDING(victim)));
      act("$N \tDmaneuvers $S mount to block\tn an attack from $n!", FALSE, ch, 0, victim,
          TO_NOTVICT);
      MOUNTED_BLOCKS_LEFT(victim)
      --;
      return (HIT_MISS);
    }
  } /* end mounted combat check */

  /* You must have at least one hand free to use this feat.
   * Once per round when you would normally be hit with an attack from a ranged
   * weapon, you may deflect it so that you take no damage from it. You must be
   * aware of the attack and not flat-footed. Attempting to deflect a ranged
   * attack doesn't count as an action. Unusually massive ranged weapons (such
   * as boulders or ballista bolts) and ranged attacks generated by natural
   * attacks or spell effects can't be deflected. */
  if (attack_type == ATTACK_TYPE_RANGED && HAS_FEAT(victim, FEAT_DEFLECT_ARROWS) &&
      DEFLECT_ARROWS_LEFT(victim) > 0 && has_dex_bonus_to_ac(ch, victim) &&
      HAS_FREE_HAND(victim))
  {
    if (HAS_FEAT(victim, FEAT_SNATCH_ARROWS))
    {
      act("\tnWith inhuman dexterity you quickly snatch out of the air $o\tn that was fired at you!",
          FALSE, victim, missile, ch, TO_CHAR);
      act("\tnWith inhuman dexterity $n snatches out of the air $o\tn that you fired!",
          FALSE, victim, missile, ch, TO_VICT | TO_SLEEP);
      act("\tnWith inhuman dexterity $n snatches out of the air $o that $N fired!",
          FALSE, victim, missile, ch, TO_NOTVICT);
      obj_to_char(missile, victim);
    }
    else
    {
      act("\tnYou deftly deflect $o out of the air, fired by $N!",
          FALSE, victim, missile, ch, TO_CHAR);
      act("\tn$n deftly deflects $o out of the air, that you fired!",
          FALSE, victim, missile, ch, TO_VICT | TO_SLEEP);
      act("\tn$n deftly deflects $o out of the air, that was fired by $N!",
          FALSE, victim, missile, ch, TO_NOTVICT);
      obj_to_room(missile, IN_ROOM(victim));
    }
    DEFLECT_ARROWS_LEFT(victim)
    --;
    return (HIT_MISS);
  }

  if (attack_type == ATTACK_TYPE_RANGED && AFF_FLAGGED(victim, AFF_WIND_WALL) && dice(1, 10) <= 3)
  {
    act("Your wall of wind throws aside $N's shot.", FALSE, victim, 0, ch, TO_CHAR);
    act("$n's wall of wind throws aside Your shot.", FALSE, victim, 0, ch, TO_VICT);
    act("$n's wall of wind throws aside $N's shot.", FALSE, victim, 0, ch, TO_NOTVICT);
    return (HIT_MISS);
  }

  if (can_hit <= 0)
  {
    /* So if we have actually hit, then dam > 0. This is how we process a miss. */
    handle_missed_attack(ch, victim, type, w_type, dam_type, attack_type, missile);
  }
  else
  {
    /* OK, attack should be a success at this stage */
    dam = handle_successful_attack(ch, victim, wielded, dam, w_type, type, diceroll,
                                   is_critical, attack_type, dam_type, missile);
  }

  if (is_critical)
  {
    if (is_flanked(ch, victim))
      teamwork_attacks_of_opportunity(victim, 0, FEAT_OUTFLANK);
    if (teamwork_using_shield(ch, FEAT_SEIZE_THE_MOMENT))
      teamwork_attacks_of_opportunity(victim, 0, FEAT_SEIZE_THE_MOMENT);
  }

  hitprcnt_mtrigger(victim); // hitprcnt trigger

  return dam;
}

/* ch dual wielding or is trelux */
int is_dual_wielding(struct char_data *ch)
{

  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
    return FALSE;

  if (GET_EQ(ch, WEAR_WIELD_OFFHAND) || GET_RACE(ch) == RACE_TRELUX)
    return TRUE;

  if (is_using_double_weapon(ch))
    return TRUE;

  return FALSE;
}

#define MODE_2_WPN 1       /* two weapon fighting equivalent (reduce two weapon fighting penalty) */
#define MODE_IMP_2_WPN 2   /* improved two weapon fighting - extra attack at -5 */
#define MODE_GREAT_2_WPN 3 /* greater two weapon fighting - extra attack at -10 */
#define MODE_EPIC_2_WPN 4  /* perfect two weapon fighting - extra attack */
int is_skilled_dualer(struct char_data *ch, int mode)
{
  switch (mode)
  {
  case MODE_2_WPN:
    if (IS_NPC(ch))
      return TRUE;
    else if (!IS_NPC(ch) && (HAS_FEAT(ch, FEAT_TWO_WEAPON_FIGHTING) ||
                             (compute_gear_armor_type(ch) <= ARMOR_TYPE_LIGHT &&
                              HAS_FEAT(ch, FEAT_DUAL_WEAPON_FIGHTING))))
    {
      return TRUE;
    }
    else
      return FALSE;
  case MODE_IMP_2_WPN:
    if (IS_NPC(ch) && (GET_CLASS(ch) == CLASS_RANGER || GET_CLASS(ch) == CLASS_ROGUE))
      return TRUE;
    else if (!IS_NPC(ch) && (HAS_FEAT(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING) ||
                             (compute_gear_armor_type(ch) <= ARMOR_TYPE_LIGHT &&
                              HAS_FEAT(ch, FEAT_IMPROVED_DUAL_WEAPON_FIGHTING))))
    {
      return TRUE;
    }
    else
      return FALSE;
  case MODE_GREAT_2_WPN:
    if (IS_NPC(ch) && GET_LEVEL(ch) >= 17 && (GET_CLASS(ch) == CLASS_RANGER || GET_CLASS(ch) == CLASS_ROGUE))
      return TRUE;
    else if (!IS_NPC(ch) && (HAS_FEAT(ch, FEAT_GREATER_TWO_WEAPON_FIGHTING) ||
                             (compute_gear_armor_type(ch) <= ARMOR_TYPE_LIGHT &&
                              HAS_FEAT(ch, FEAT_GREATER_DUAL_WEAPON_FIGHTING))))
    {
      return TRUE;
    }
    else
      return FALSE;
  case MODE_EPIC_2_WPN:
    if (IS_NPC(ch) && GET_LEVEL(ch) >= 24 && (GET_CLASS(ch) == CLASS_RANGER || GET_CLASS(ch) == CLASS_ROGUE))
      return TRUE;
    else if (!IS_NPC(ch) && (HAS_FEAT(ch, FEAT_PERFECT_TWO_WEAPON_FIGHTING) ||
                             (compute_gear_armor_type(ch) <= ARMOR_TYPE_LIGHT &&
                              HAS_FEAT(ch, FEAT_PERFECT_DUAL_WEAPON_FIGHTING))))
    {
      return TRUE;
    }
    else
      return FALSE;
  }

  log("ERR: is_skilled_dualer() reached end!");

  return 0;
}

/* common dummy check in perform_attacks()
   expanded this for dummy checking outside of perform_attacks via 'strict' mode -zusuk */
int valid_fight_cond(struct char_data *ch, bool strict)
{
  if (!ch)
    return FALSE;

  if (FIGHTING(ch) && !strict)
  {
    update_pos(FIGHTING(ch));
    if (GET_POS(FIGHTING(ch)) != POS_DEAD &&
        IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))

      return TRUE;
  }
  else if (strict && FIGHTING(ch))
  {
    update_pos(FIGHTING(ch));
    update_pos(ch);
    if (GET_POS(FIGHTING(ch)) != POS_DEAD &&
        IN_ROOM(ch) != NOWHERE &&
        GET_POS(ch) > POS_SLEEPING &&
        IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch) &&
        GET_HIT(FIGHTING(ch)) >= 1 &&
        GET_HIT(ch) >= 1)

      return TRUE;
  }

  return FALSE;
}

/* returns # of attacks and has mode functionality */
#define ATTACK_CAP 3                       /* MAX # of main-hand BONUS attacks */
#define MONK_CAP (ATTACK_CAP + 2)          /* monks main-hand bonus attack cap */
#define NPC_ATTACK_CAP (MONK_CAPK_CAP + 2) /* high level NPC bonus attack cap */
#define TWO_WPN_PNLTY -5                   /* improved two weapon fighting */
#define GREAT_TWO_PNLY -10                 /* greater two weapon fighting */
#define EPIC_TWO_PNLTY 0                   /* perfect two weapon fighting */
/* mode functionality */
#define NORMAL_ATTACK_ROUTINE 0     /*mode = 0  normal attack routine*/
#define RETURN_NUM_ATTACKS 1        /*mode = 1  return # of attacks, nothing else*/
#define DISPLAY_ROUTINE_POTENTIAL 2 /*mode = 2  display attack routine potential*/
/* Phase determines what part of the combat round we are currently
 * in.  Each combat round is broken up into 3 2-second phases.
 * Attacks are split among the phases (for a full-round-action)
 * in the following pattern:
 * For 1 attack:
 *   Phase 1 - Attack once.
 *   Phase 2 - Do Nothing.
 *   Phase 3 - Do Nothing.
 * For 2 attacks:
 *   Phase 1 - Attack once.
 *   Phase 2 - Attack once.
 *   Phase 3 - Do Nothing.
 * For 4 attacks:
 *   Phase 1 - Attack twice.
 *   Phase 2 - Attack once.
 *   Phase 3 - Attack once.  ...ETC... */
#define PHASE_0 0
#define PHASE_1 1
#define PHASE_2 2
#define PHASE_3 3
int perform_attacks(struct char_data *ch, int mode, int phase)
{
  int i = 0, penalty = 0, numAttacks = 0, bonus_mainhand_attacks = 0;
  int attacks_at_max_bab = 0;
  int ranged_attacks = 1; /* ranged combat gets 1 bonus attacks currently */
  bool dual = FALSE;
  bool perform_attack = FALSE;
  /* so if ranged is not performed and we fall through to melee, we need to make
   * sure our attacks with max. BAB are maintained */
  int drop_an_attack_at_max_bab = 0;
  struct obj_data *wielded = NULL;
  int wpn_reload_status = 0;

  /* Check position..  we don't check < POS_STUNNED anymore */
  if (GET_POS(ch) == POS_DEAD)
    return (0);

  /*  If we have no standard action (and are using regular attack mode.)
   *  Do not attack at all. If we have no move action (and are in regular
   *  attack mode) skip all phases but the first. */
  if ((mode == NORMAL_ATTACK_ROUTINE) && !is_action_available(ch, atSTANDARD, FALSE))
    return (0);
  else if ((mode == NORMAL_ATTACK_ROUTINE) && (phase != PHASE_1) && !is_action_available(ch, atMOVE, FALSE))
    return (0);

  guard_check(ch, FIGHTING(ch)); /* this is the guard skill check */

  /** BEGIN PROCESS OF COUNTING ATTACKS AND PENALTIES FOR SUCCESSIVE ATTACKS  **/

  /* level based bonus attacks, which is BAB / 5 up to the ATTACK_CAP
   * [note might need to add armor restrictions here?] */
  bonus_mainhand_attacks = MIN((BAB(ch) - 1) / 5, ATTACK_CAP);

  /* monk flurry of blows */
  if (MONK_TYPE(ch) && monk_gear_ok(ch) &&
      AFF_FLAGGED(ch, AFF_FLURRY_OF_BLOWS))
  {
    bonus_mainhand_attacks++;
    attacks_at_max_bab++;
    if (MONK_TYPE(ch) < 5)
      penalty = -2; /* flurry penalty */
    else if (MONK_TYPE(ch) < 9)
      penalty = -1; /* 9th level+, no more penalty to flurry! */
    if (HAS_FEAT(ch, FEAT_GREATER_FLURRY))
    { /* FEAT_GREATER_FLURRY, 11th level */
      bonus_mainhand_attacks++;
      attacks_at_max_bab++;
      if (MONK_TYPE(ch) >= 15)
      {
        bonus_mainhand_attacks++;
        attacks_at_max_bab++;
      }
    }
  }

  /* high level NPCs get bonus attacks at max-bab! */
  if (IS_NPC(ch) && GET_LEVEL(ch) > 30)
  {
    bonus_mainhand_attacks += GET_LEVEL(ch) - 30;
    attacks_at_max_bab += GET_LEVEL(ch) - 30;
  }

  /* Haste or equivalent gives one extra attack, ranged or melee, at max BAB. */
  if (AFF_FLAGGED(ch, AFF_HASTE) ||
      (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_BLINDING_SPEED)) ||
      (has_speed_weapon(ch)))
  {
    ranged_attacks++;
    attacks_at_max_bab++;
  }

  /* how do we know if we are in "ranged" or "melee" combat?  the current solution
   is simply to see if you are qualified to perform a ranged attack, if so, execute
   and then exit.  Otherwise you will fall through and perform a melee attack
   (unless you have a ranged weapon equipped, in which case exit) */

  /* -- Process ranged attacks, determine base number of attacks irregardless of
   * whether ch is in combat or not ------ */
  if (can_fire_ammo(ch, TRUE))
  {

    /* Early Exits from ranged combat? */

    /* if we don't have point blank shot, unceremoniously dump out of function */
    if (is_tanking(ch) && !IS_NPC(ch) && !HAS_FEAT(ch, FEAT_POINT_BLANK_SHOT))
    {
      send_to_char(ch, "You are too close to use your ranged weapon.\r\n");
      stop_fighting(ch);
      FIRING(ch) = FALSE;
      return 0;
    }

    /* END Early exits from ranged combat! */

    ranged_attacks += bonus_mainhand_attacks;

    /* Rapidshot mode gives an extra attack, but with a penalty to all attacks. */
    if (AFF_FLAGGED(ch, AFF_RAPID_SHOT))
    {
      penalty -= 2;
      ranged_attacks++;
      attacks_at_max_bab++; /* we have to drop this if this isn't a ranged attack! */
      drop_an_attack_at_max_bab++;

      if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_MANYSHOT))
      {
        ranged_attacks++;
        attacks_at_max_bab++; /* we have to drop this if this isn't a ranged attack! */
        drop_an_attack_at_max_bab++;
      }

      if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_EPIC_MANYSHOT))
      {
        ranged_attacks++;
        attacks_at_max_bab++; /* we have to drop this if this isn't a ranged attack! */
        drop_an_attack_at_max_bab++;
      }
    }
  }
  /* -- finished processing base number of attacks and BAB for ranged -- */

  /* Ranged attacker, lets process some penalties, etc. then start *processing*
   * Note:  This is not a display-info mode, so unique modifications to actual
   * combat are included here as opposed to above calculations */
  if (can_fire_ammo(ch, TRUE) && FIRING(ch) && mode == NORMAL_ATTACK_ROUTINE)
  {
    if (is_tanking(ch))
    {
      if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_IMPROVED_PRECISE_SHOT))
        penalty += 4;
      else if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_PRECISE_SHOT))
        ;  /* no penalty/bonus */
      else /* not skilled with close combat archery */
        penalty -= 4;
    }

    /* mounted archery requires a feat or you receive 4 penalty to attack rolls */
    if (RIDING(ch) && !IS_NPC(ch))
    {
      if (!HAS_FEAT(ch, FEAT_MOUNTED_ARCHERY))
        penalty -= 4;
    }

    /** BEGIN RANGED COMBAT EXECUTION ROUTINE **/

    for (i = 0; i <= ranged_attacks; i++)
    { /* check phase for corresponding attack */
      /* phase 1: 1 4 7 10 13
       * phase 2: 2 5 8 11 14
       * phase 3: 3 6 9 12 15 */
      perform_attack = FALSE;
      switch (i)
      {
      case 1:
      case 4:
      case 7:
      case 10:
      case 13:
        if (phase == PHASE_0 || phase == PHASE_1)
        {
          perform_attack = TRUE;
        }
        break;
      case 2:
      case 5:
      case 8:
      case 11:
      case 14:
        if (phase == PHASE_0 || phase == PHASE_2)
        {
          perform_attack = TRUE;
        }
        break;
      case 3:
      case 6:
      case 9:
      case 12:
      case 15:
        if (phase == PHASE_0 || phase == PHASE_3)
        {
          perform_attack = TRUE;
        }
        break;
      }
      if (perform_attack)
      { /* correct phase for this attack? */
        if (can_fire_ammo(ch, FALSE) && FIGHTING(ch))
        {

          /* FIRE! PEW-PEW!! */
          wpn_reload_status = hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, penalty,
                                  ATTACK_TYPE_RANGED);

          if (attacks_at_max_bab > 0)
            attacks_at_max_bab--;
          else
            penalty -= 5; /* cummulative penalty */

          /* here is our auto-reload system for xbows, etc */
          if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTORELOAD))
          {
            auto_reload_weapon(ch, TRUE);
          }

          /* bail if our hit() caused a need-reload return value */
          if (wpn_reload_status == HIT_NEED_RELOAD)
            return 0;
        }
        else
        {
          /* we can't fire an arrow, we are NOT in silent-mode so the
           reason we are exiting ranged combat should be announced via
           can_fire_ammo() */
          stop_fighting(ch);
          FIRING(ch) = FALSE;
          return 0;
        }
      }
    }

    /** COMPLETED RANGED COMBAT EXECUTION ROUTINE **/

    /* cleanup and/or related processes ranged-related */

    /* in case your very last ranged attack above leaves you not able to
       fire, we will send one more message here using can_fire_ammo
       with silent-mode off */
    if (!can_fire_ammo(ch, FALSE))
    {
      stop_fighting(ch);
      FIRING(ch) = FALSE;
    }

    /* that is it, all done with ranged combat! */
    return 0;

    /* Display Modes, not actually firing, how many attacks? */
  }
  else if (mode == RETURN_NUM_ATTACKS && is_using_ranged_weapon(ch, TRUE))
  {
    return ranged_attacks;

    /* Display Modes, not actually firing, show our routines full POWAH! */
  }
  else if (mode == DISPLAY_ROUTINE_POTENTIAL && is_using_ranged_weapon(ch, TRUE))
  {
    while (ranged_attacks > 0)
    {
      /* display hitroll bonus */
      send_to_char(ch, "Ranged Attack Bonus:  %d; ",
                   compute_attack_bonus(ch, ch, ATTACK_TYPE_RANGED) + penalty);
      /* display damage bonus */
      compute_hit_damage(ch, ch, TYPE_UNDEFINED_WTYPE, NO_DICEROLL, MODE_DISPLAY_RANGED, FALSE, ATTACK_TYPE_RANGED);

      if (attacks_at_max_bab > 0)
        attacks_at_max_bab--;
      else
        penalty -= 5;

      ranged_attacks--;
    }

    if (!can_fire_ammo(ch, TRUE))
    {
      send_to_char(ch, "You are not prepared to fire your weapon:  ");
      can_fire_ammo(ch, FALSE); /* sends message why */
    }

    return 0;
  }
  /***/
  if (drop_an_attack_at_max_bab) /*cleanup for ranged*/
    attacks_at_max_bab -= drop_an_attack_at_max_bab;
  /***/
  /*  End ranged attacks ---------------------------------------------------- */

  /************************/
  /* Process Melee Attacks -------------------------------------------------- */
  // melee: now lets determine base attack(s) and resulting possible penalty
  dual = is_dual_wielding(ch); // trelux or has off-hander equipped

  /* we are going to exit melee combat if we are somehow wielding a ranged
             weapon here */
  wielded = is_using_ranged_weapon(ch, TRUE);
  if (wielded)
  {
    send_to_char(ch, "You can not use a ranged weapon in melee combat: ");
    can_fire_ammo(ch, FALSE); /* we are using the function to report why! */

    /* check for message on reloading weapons such as xbow */
    if (is_reloading_weapon(ch, wielded, TRUE))
    {
      if (!weapon_is_loaded(ch, wielded, FALSE))
      {
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTORELOAD))
        {
          auto_reload_weapon(ch, TRUE);
        }
      }
    }

    return 0;
  }

  if (dual)
  {                  /*default of one offhand attack for everyone*/
    numAttacks += 2; /* mainhand + offhand */
    if (GET_EQ(ch, WEAR_WIELD_OFFHAND))
    { /* determine if offhand is smaller than ch */
      if (!is_using_light_weapon(ch, GET_EQ(ch, WEAR_WIELD_OFFHAND)))
        penalty -= 4; /* offhand weapon is not light! */
    }
    if (is_skilled_dualer(ch, MODE_2_WPN)) /* two weapon fighting feat? */
      penalty -= 2;                        /* yep! */
    else                                   /* nope! */
      penalty -= 4;
    if (mode == NORMAL_ATTACK_ROUTINE)
    { // normal attack routine
      if (valid_fight_cond(ch, FALSE))
        if (phase == PHASE_0 || phase == PHASE_1)
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC,
              penalty, ATTACK_TYPE_PRIMARY); /* whack with mainhand */
      if (valid_fight_cond(ch, FALSE))
        if (phase == PHASE_0 || phase == PHASE_2)
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC,
              penalty * 2, ATTACK_TYPE_OFFHAND); /* whack with offhand */
                                                 // display attack routine
    }
    else if (mode == DISPLAY_ROUTINE_POTENTIAL)
    {
      /* display hitroll bonus */
      send_to_char(ch, "Mainhand, Attack Bonus:  %d; ",
                   compute_attack_bonus(ch, ch, ATTACK_TYPE_PRIMARY) + penalty);
      /* display damage bonus */
      compute_hit_damage(ch, ch, TYPE_UNDEFINED_WTYPE, NO_DICEROLL, MODE_DISPLAY_PRIMARY, FALSE, ATTACK_TYPE_PRIMARY);
      /* display hitroll bonus */
      send_to_char(ch, "Offhand, Attack Bonus:  %d; ",
                   compute_attack_bonus(ch, ch, ATTACK_TYPE_OFFHAND) + penalty * 2);
      /* display damage bonus */
      compute_hit_damage(ch, ch, TYPE_UNDEFINED_WTYPE, NO_DICEROLL, MODE_DISPLAY_OFFHAND, FALSE, ATTACK_TYPE_OFFHAND);
    }
  }
  else
  {

    // not dual wielding
    numAttacks++; // default of one attack for everyone

    if (mode == NORMAL_ATTACK_ROUTINE)
    { // normal attack routine
      if (valid_fight_cond(ch, FALSE))
        if (phase == PHASE_0 || phase == PHASE_1)
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, penalty, ATTACK_TYPE_PRIMARY);
    }
    else if (mode == DISPLAY_ROUTINE_POTENTIAL)
    {
      /* display hitroll bonus */
      send_to_char(ch, "Mainhand, Attack Bonus:  %d; ",
                   compute_attack_bonus(ch, ch, ATTACK_TYPE_PRIMARY) + penalty);
      /* display damage bonus */
      compute_hit_damage(ch, ch, TYPE_UNDEFINED_WTYPE, NO_DICEROLL, MODE_DISPLAY_PRIMARY, FALSE, ATTACK_TYPE_PRIMARY);
    }
  }

  /* haste or equivalent? */
  if (AFF_FLAGGED(ch, AFF_HASTE) ||
      (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_BLINDING_SPEED)))
  {
    numAttacks++;
    if (mode == NORMAL_ATTACK_ROUTINE)
    { // normal attack routine
      attacks_at_max_bab--;
      if (valid_fight_cond(ch, FALSE))
        if (phase == PHASE_0 || ((phase == PHASE_2) && numAttacks == 2) || ((phase == PHASE_3) && numAttacks == 3))
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, penalty, ATTACK_TYPE_PRIMARY);
    }
    else if (mode == DISPLAY_ROUTINE_POTENTIAL)
    {
      /* display hitroll bonus */
      send_to_char(ch, "Mainhand (Haste), Attack Bonus:  %d; ",
                   compute_attack_bonus(ch, ch, ATTACK_TYPE_PRIMARY) + penalty);
      /* display damage bonus */
      compute_hit_damage(ch, ch, TYPE_UNDEFINED_WTYPE, NO_DICEROLL, MODE_DISPLAY_PRIMARY, FALSE, ATTACK_TYPE_PRIMARY);
    }
  }

  // execute the calculated attacks from above
  int j = 0;
  for (i = 0; i < bonus_mainhand_attacks; i++)
  {
    numAttacks++;
    j = numAttacks + i;
    perform_attack = FALSE;
    switch (j)
    {
    case 1:
    case 4:
    case 7:
    case 10:
    case 13:
      if (phase == PHASE_0 || phase == PHASE_1)
      {
        perform_attack = TRUE;
      }
      break;
    case 2:
    case 5:
    case 8:
    case 11:
    case 14:
      if (phase == PHASE_0 || phase == PHASE_2)
      {
        perform_attack = TRUE;
      }
      break;
    case 3:
    case 6:
    case 9:
    case 12:
    case 15:
      if (phase == PHASE_0 || phase == PHASE_3)
      {
        perform_attack = TRUE;
      }
      break;
    }

    if (perform_attack)
    {
      if (attacks_at_max_bab > 0)
        attacks_at_max_bab--; /* like monks flurry */
      else
        penalty -= 5; /* everyone gets -5 penalty per bonus attack by mainhand */

      if (FIGHTING(ch) && mode == NORMAL_ATTACK_ROUTINE)
      { // normal attack routine
        update_pos(FIGHTING(ch));
        if (valid_fight_cond(ch, FALSE))
        {
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, penalty, ATTACK_TYPE_PRIMARY);
        }
      }
    }

    /* Display attack routine. */
    if (mode == DISPLAY_ROUTINE_POTENTIAL)
    {

      /* we have to account for perform-attack not being called since this is just a display */
      int spec_hand = compute_attack_bonus(ch, ch, ATTACK_TYPE_PRIMARY) + penalty;
      if (!perform_attack)
        spec_hand -= 5;

      /* display hitroll bonus */
      send_to_char(ch, "Mainhand Bonus %d, Attack Bonus:  %d; ",
                   i + 1, spec_hand);
      /* display damage bonus */
      compute_hit_damage(ch, ch, TYPE_UNDEFINED_WTYPE, NO_DICEROLL, MODE_DISPLAY_PRIMARY, FALSE, ATTACK_TYPE_PRIMARY);
    }
  } /* end for loop */

  /*additional off-hand attacks*/
  if (dual)
  {

    if (!IS_NPC(ch) && is_skilled_dualer(ch, MODE_IMP_2_WPN))
    { /* improved 2-weapon fighting */
      numAttacks++;
      if (mode == NORMAL_ATTACK_ROUTINE)
      { // normal attack routine
        if (valid_fight_cond(ch, FALSE))
          if (phase == PHASE_0 || ((phase == PHASE_1) && ((numAttacks == 1) || (numAttacks == 4) || (numAttacks == 7) || (numAttacks == 10) || (numAttacks == 13))) ||
              ((phase == PHASE_2) && ((numAttacks == 2) ||
                                      (numAttacks == 5) ||
                                      (numAttacks == 8) ||
                                      (numAttacks == 11) ||
                                      (numAttacks == 14))) ||
              ((phase == PHASE_1) && ((numAttacks == 3) ||
                                      (numAttacks == 6) ||
                                      (numAttacks == 9) ||
                                      (numAttacks == 12) ||
                                      (numAttacks == 15))))
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, TWO_WPN_PNLTY, ATTACK_TYPE_OFFHAND);
      }
      else if (mode == DISPLAY_ROUTINE_POTENTIAL)
      {
        /* display hitroll bonus */
        send_to_char(ch, "Offhand (Improved 2 Weapon Fighting), Attack Bonus:  %d; ",
                     compute_attack_bonus(ch, ch, ATTACK_TYPE_OFFHAND) + TWO_WPN_PNLTY);
        /* display damage bonus */
        compute_hit_damage(ch, ch, TYPE_UNDEFINED_WTYPE, NO_DICEROLL, MODE_DISPLAY_OFFHAND, FALSE, ATTACK_TYPE_OFFHAND);
      }
    }

    if (!IS_NPC(ch) && is_skilled_dualer(ch, MODE_GREAT_2_WPN))
    { /* greater two weapon fighting */
      numAttacks++;
      if (mode == NORMAL_ATTACK_ROUTINE)
      { // normal attack routine
        if (valid_fight_cond(ch, FALSE))
          if (phase == PHASE_0 || ((phase == PHASE_1) && ((numAttacks == 1) || (numAttacks == 4) || (numAttacks == 7) || (numAttacks == 10) || (numAttacks == 13))) ||
              ((phase == PHASE_2) && ((numAttacks == 2) ||
                                      (numAttacks == 5) ||
                                      (numAttacks == 8) ||
                                      (numAttacks == 11) ||
                                      (numAttacks == 14))) ||
              ((phase == PHASE_1) && ((numAttacks == 3) ||
                                      (numAttacks == 6) ||
                                      (numAttacks == 9) ||
                                      (numAttacks == 12) ||
                                      (numAttacks == 15))))

            hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, GREAT_TWO_PNLY, ATTACK_TYPE_OFFHAND);
      }
      else if (mode == DISPLAY_ROUTINE_POTENTIAL)
      {
        /* display hitroll bonus */
        send_to_char(ch, "Offhand (Great 2 Weapon Fighting), Attack Bonus:  %d; ",
                     compute_attack_bonus(ch, ch, ATTACK_TYPE_OFFHAND) + GREAT_TWO_PNLY);
        /* display damage bonus */
        compute_hit_damage(ch, ch, TYPE_UNDEFINED_WTYPE, NO_DICEROLL, MODE_DISPLAY_OFFHAND, FALSE, ATTACK_TYPE_OFFHAND);
      }
    }

    if (!IS_NPC(ch) && is_skilled_dualer(ch, MODE_EPIC_2_WPN))
    { /* perfect two weapon fighting */
      numAttacks++;
      if (mode == NORMAL_ATTACK_ROUTINE)
      { // normal attack routine
        if (valid_fight_cond(ch, FALSE))
          if (phase == PHASE_0 || ((phase == PHASE_1) && ((numAttacks == 1) || (numAttacks == 4) || (numAttacks == 7) || (numAttacks == 10) || (numAttacks == 13))) ||
              ((phase == PHASE_2) && ((numAttacks == 2) ||
                                      (numAttacks == 5) ||
                                      (numAttacks == 8) ||
                                      (numAttacks == 11) ||
                                      (numAttacks == 14))) ||
              ((phase == PHASE_1) && ((numAttacks == 3) ||
                                      (numAttacks == 6) ||
                                      (numAttacks == 9) ||
                                      (numAttacks == 12) ||
                                      (numAttacks == 15))))
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, EPIC_TWO_PNLTY, ATTACK_TYPE_OFFHAND);
      }
      else if (mode == DISPLAY_ROUTINE_POTENTIAL)
      {

        /* display hitroll bonus */
        send_to_char(ch, "Offhand (Epic 2 Weapon Fighting), Attack Bonus:  %d; ",
                     compute_attack_bonus(ch, ch, ATTACK_TYPE_OFFHAND) + EPIC_TWO_PNLTY);
        /* display damage bonus */
        compute_hit_damage(ch, ch, TYPE_UNDEFINED_WTYPE, NO_DICEROLL, MODE_DISPLAY_OFFHAND, FALSE, ATTACK_TYPE_OFFHAND);
      }
    }
  }
  return numAttacks;
}
#undef ATTACK_CAP
#undef MONK_CAP
#undef TWO_WPN_PNLTY
#undef EPIC_TWO_PNLY
#undef NORMAL_ATTACK_ROUTINE
#undef RETURN_NUM_ATTACKS
#undef DISPLAY_ROUTINE_POTENTIAL
#undef PHASE_1
#undef PHASE_2
#undef PHASE_3
#undef NPC_ATTACK_CAP

/* display condition of FIGHTING() target to ch */
/* this is deprecated with the prompt changes */
void autoDiagnose(struct char_data *ch)
{
  if (!ch) /* silly dummy check */
    return;

  struct char_data *char_fighting = NULL, *tank = NULL;
  int percent;

  char_fighting = FIGHTING(ch);

  if (char_fighting && (ch->in_room == char_fighting->in_room) &&
      GET_HIT(char_fighting) > 0)
  {

    if ((tank = char_fighting->char_specials.fighting) &&
        (ch->in_room == tank->in_room))
    {

      if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_COMPACT))
        send_to_char(ch, "\r\n");

      send_to_char(ch, "%sT-%s%s",
                   CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                   (CAN_SEE(ch, tank)) ? GET_NAME(tank) : "someone");
      send_to_char(ch, "%s: ",
                   CCCYN(ch, C_NRM));

      if (GET_MAX_HIT(tank) > 0)
        percent = (100 * GET_HIT(tank)) / GET_MAX_HIT(tank);
      else
        percent = -1;

      if (percent >= 100)
      {
        send_to_char(ch, CBWHT(ch, C_NRM));
        send_to_char(ch, "excellent");
      }
      else if (percent >= 95)
      {
        send_to_char(ch, CCNRM(ch, C_NRM));
        send_to_char(ch, "few scratches");
      }
      else if (percent >= 75)
      {
        send_to_char(ch, CBGRN(ch, C_NRM));
        send_to_char(ch, "small wounds");
      }
      else if (percent >= 55)
      {
        send_to_char(ch, CBBLK(ch, C_NRM));
        send_to_char(ch, "few wounds");
      }
      else if (percent >= 35)
      {
        send_to_char(ch, CBMAG(ch, C_NRM));
        send_to_char(ch, "nasty wounds");
      }
      else if (percent >= 15)
      {
        send_to_char(ch, CBBLU(ch, C_NRM));
        send_to_char(ch, "pretty hurt");
      }
      else if (percent >= 1)
      {
        send_to_char(ch, CBRED(ch, C_NRM));
        send_to_char(ch, "awful");
      }
      else
      {
        send_to_char(ch, CBFRED(ch, C_NRM));
        send_to_char(ch, "bleeding, close to death");
        send_to_char(ch, CCNRM(ch, C_NRM));
      }
    } // end tank

    send_to_char(ch, "%s     E-%s%s",
                 CBCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                 (CAN_SEE(ch, char_fighting)) ? GET_NAME(char_fighting) : "someone");

    send_to_char(ch, "%s: ",
                 CBCYN(ch, C_NRM));

    if (GET_MAX_HIT(char_fighting) > 0)
      percent = (100 * GET_HIT(char_fighting)) / GET_MAX_HIT(char_fighting);
    else
      percent = -1;
    if (percent >= 100)
    {
      send_to_char(ch, CBWHT(ch, C_NRM));
      send_to_char(ch, "excellent");
    }
    else if (percent >= 95)
    {
      send_to_char(ch, CCNRM(ch, C_NRM));
      send_to_char(ch, "few scratches");
    }
    else if (percent >= 75)
    {
      send_to_char(ch, CBGRN(ch, C_NRM));
      send_to_char(ch, "small wounds");
    }
    else if (percent >= 55)
    {
      send_to_char(ch, CBBLK(ch, C_NRM));
      send_to_char(ch, "few wounds");
    }
    else if (percent >= 35)
    {
      send_to_char(ch, CBMAG(ch, C_NRM));
      send_to_char(ch, "nasty wounds");
    }
    else if (percent >= 15)
    {
      send_to_char(ch, CBBLU(ch, C_NRM));
      send_to_char(ch, "pretty hurt");
    }
    else if (percent >= 1)
    {
      send_to_char(ch, CBRED(ch, C_NRM));
      send_to_char(ch, "awful");
    }
    else
    {
      send_to_char(ch, CBFRED(ch, C_NRM));
      send_to_char(ch, "bleeding, close to death");
      send_to_char(ch, CCNRM(ch, C_NRM));
    }
    send_to_char(ch, "\tn\r\n");

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_COMPACT))
      send_to_char(ch, "\r\n");
  }
}

/* Fight control event.  Replaces the violence loop. */
EVENTFUNC(event_combat_round)
{
  struct char_data *ch = NULL;
  struct mud_event_data *pMudEvent = NULL;

  /*  This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /*  For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  if ((!IS_NPC(ch) && (ch->desc != NULL && !IS_PLAYING(ch->desc))) || (FIGHTING(ch) == NULL))
  {
    if (DEBUGMODE)
    {
      send_to_char(ch, "DEBUG: RETURNING 0 FROM COMBAT EVENT.\r\n");
    }
    stop_fighting(ch);
    return 0;
  }

  if (FIGHTING(ch) == NULL)
  {
    return 0;
  }

  if (GET_POS(FIGHTING(ch)) <= POS_DEAD || GET_POS(ch) <= POS_DEAD)
  {
    stop_fighting(ch);
    return 0;
  }

  if (IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
  {
    stop_fighting(ch);
    return 0;
  }

  /* action queue system */
  execute_next_action(ch);
  /* execute phase */
  perform_violence(ch, (pMudEvent->sVariables != NULL && is_number(pMudEvent->sVariables) ? atoi(pMudEvent->sVariables) : 0));

  /* set the next phase */
  if (pMudEvent->sVariables != NULL)
    sprintf(pMudEvent->sVariables, "%d", (atoi(pMudEvent->sVariables) < 3 ? atoi(pMudEvent->sVariables) + 1 : 1));

  return 2 RL_SEC; /* 6 second rounds, hack! */
}

void handle_cleave(struct char_data *ch)
{
  struct list_data *target_list = NULL;
  struct char_data *tch = NULL;

  /* find target */
  if (!ch || IN_ROOM(ch) == NOWHERE || !FIGHTING(ch))
    return;

  /* dynamic memory allocation required */
  target_list = create_list();
  /* loop through chars in room to find possible targets to build list */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch)
      continue;
    if (!CAN_SEE(ch, tch))
      continue;
    if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_NOHASSLE))
      continue;
    /* me fighting? */
    if (FIGHTING(ch) == tch)
      continue;
    /* fighting me? */
    if (FIGHTING(tch) == ch)
      add_to_list(tch, target_list);
  }
  /* did we snag anything? */
  if (target_list->iSize == 0)
  {
    free_list(target_list);
    return;
  }
  /* ok should be golden, go ahead snag a random and free list */
  tch = random_from_list(target_list);
  if (target_list) /*cleanup*/
    free_list(target_list);
  if (!tch)
    return;

  send_to_char(ch, "You cleave to %s!\r\n", (CAN_SEE(ch, tch)) ? GET_NAME(tch) : "someone");
  act("$n cleaves to $N!", TRUE, ch, 0, tch, TO_ROOM);

  hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC,
      -4, ATTACK_TYPE_PRIMARY); /* whack with mainhand */

  if (HAS_FEAT(ch, FEAT_GREAT_CLEAVE) && !is_using_ranged_weapon(ch, TRUE))
  {

    hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC,
        0, ATTACK_TYPE_PRIMARY); /* whack with mainhand */
  }
}

void handle_smash_defense(struct char_data *ch)
{
  struct char_data *vict = FIGHTING(ch);

  /* general dummy checks */
  if (IN_ROOM(ch) == NOWHERE || !vict ||
      IN_ROOM(vict) != IN_ROOM(ch))
    return;

  /* some automatic disqualifiers, we will silently return from these */
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
    return;
  if (MOB_FLAGGED(vict, MOB_NOKILL) || !is_mission_mob(ch, vict))
    return;
  if ((GET_SIZE(ch) - GET_SIZE(vict)) >= 2)
    return;
  if ((GET_SIZE(vict) - GET_SIZE(ch)) >= 2)
    return;
  if (GET_POS(vict) <= POS_SITTING)
    return;
  if (IS_INCORPOREAL(vict) && !is_using_ghost_touch_weapon(ch))
    return;
  if (MOB_FLAGGED(vict, MOB_NOBASH))
    return;

  /* OK should be ok now! */
  send_to_char(ch, "\tW[Smash Defense]\tn");
  perform_knockdown(ch, vict, SKILL_BASH);

  /* tag with event to make sure this only happens once per round! */
  attach_mud_event(new_mud_event(eSMASH_DEFENSE, ch, NULL), 6 * PASSES_PER_SEC);

  return;
}

/* control the fights going on.
 * Called from combat round event. */
void perform_violence(struct char_data *ch, int phase)
{
  struct char_data *tch = NULL, *charmee;
  struct list_data *room_list = NULL;

  /* Reset combat data */
  GET_TOTAL_AOO(ch) = 0;
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLAT_FOOTED);

  ch->char_specials.energy_retort_used = false;
  remove_fear_affects(ch, TRUE);

  if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
  {
    stop_fighting(ch);
    return;
  }

  if (phase == 1 || phase == 0)
  { /* make sure this doesn't happen more than once a round */
#define RETURN_NUM_ATTACKS 1
    TOTAL_DEFENSE(ch) = perform_attacks(ch, RETURN_NUM_ATTACKS, phase);
#undef RETURN_NUM_ATTACKS

    /* Once per round when your mount is hit in combat, you may attempt a Ride
     * check (as an immediate action) to negate the hit. The hit is negated if
     * your Ride check result is greater than the opponent's attack roll. */
    if (RIDING(ch) && HAS_FEAT(ch, FEAT_MOUNTED_COMBAT))
      MOUNTED_BLOCKS_LEFT(ch) = 1;
    if (RIDING(ch) && HAS_FEAT(ch, FEAT_LEGENDARY_RIDER))
      MOUNTED_BLOCKS_LEFT(ch) += 1;

    /* You must have at least one hand free to use this feat.
     * Once per round when you would normally be hit with an attack from a ranged
     * weapon, you may deflect it so that you take no damage from it. You must be
     * aware of the attack and not flat-footed. Attempting to deflect a ranged
     * attack doesn't count as an action. Unusually massive ranged weapons (such
     * as boulders or ballista bolts) and ranged attacks generated by natural
     * attacks or spell effects can't be deflected. */
    if (HAS_FREE_HAND(ch) && HAS_FEAT(ch, FEAT_DEFLECT_ARROWS) &&
        has_dex_bonus_to_ac(NULL, ch))
      DEFLECT_ARROWS_LEFT(ch) = 1;

    /* once per round, while in defensive stance, we must remove this event
                   to ensure that the stalwart defender only gets ONE free knockdown
                   attempt (note: just made this time out) */
    /*
    if (char_has_mud_event(ch, eSMASH_DEFENSE)) {
      event_cancel_specific(ch, eSMASH_DEFENSE);
    }
    */

    /* autostand mechanic */
    if (ch && FIGHTING(ch) && !IS_NPC(ch) && can_stand(ch) && PRF_FLAGGED(ch, PRF_AUTO_STAND))
    {
      /* check if we can springleap out of this */
      if (HAS_FEAT(ch, FEAT_SPRING_ATTACK) && CLASS_LEVEL(ch, CLASS_MONK) >= 5)
      {
        do_springleap(ch, 0, 0, 0);
      }

      /* attempt to stand! checking if we can stand again in case springleap worked */
      if (can_stand(ch))
        do_stand(ch, 0, 0, 0);
    }
  }

  // if they're affected by hedging weapon, we'll throw one at our current fighting target
  throw_hedging_weapon(ch);

  if (AFF_FLAGGED(ch, AFF_PARALYZED))
  {
    if (AFF_FLAGGED(ch, AFF_FREE_MOVEMENT))
    {
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PARALYZED);
      send_to_char(ch, "Your free movement breaks the paralysis!\r\n");
      act("$n's free movement breaks the paralysis!",
          TRUE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      send_to_char(ch, "You are paralyzed and unable to react!\r\n");
      act("$n seems to be paralyzed and unable to react!",
          TRUE, ch, 0, 0, TO_ROOM);
      return;
    }
  }

  if (AFF_FLAGGED(ch, AFF_NAUSEATED))
  {
    send_to_char(ch, "You are too nauseated to fight!\r\n");
    act("$n seems to be too nauseated to fight!",
        TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_DAZED))
  {
    send_to_char(ch, "You are too dazed to fight!\r\n");
    act("$n seems too dazed to fight!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  if (char_has_mud_event(ch, eSTUNNED))
  {
    if (AFF_FLAGGED(ch, AFF_FREE_MOVEMENT))
    {
      change_event_duration(ch, eSTUNNED, 0);
      send_to_char(ch, "Your free movement breaks the stun!\r\n");
      act("$n's free movement breaks the stun!",
          TRUE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      send_to_char(ch, "You are stunned and unable to react!\r\n");
      act("$n seems to be stunned and unable to react!",
          TRUE, ch, 0, 0, TO_ROOM);
      return;
    }
  }

  /* we'll break stun here if under free-movement affect */
  if (AFF_FLAGGED(ch, AFF_STUN))
  {
    if (AFF_FLAGGED(ch, AFF_FREE_MOVEMENT))
    {
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_STUN);
      send_to_char(ch, "Your free movement breaks the stun!\r\n");
      act("$n's free movement breaks the stun!",
          TRUE, ch, 0, 0, TO_ROOM);
    }
  }

  /* make sure this goes after attack-stopping affects like paralyze */
  if (!MOB_CAN_FIGHT(ch))
  {
    /* this should be called in hit() but need a copy here for !fight flag */
    fight_mtrigger(ch); // fight trig
    return;
  }

  if (IS_NPC(ch) && (phase == 1))
  {
    if (GET_MOB_WAIT(ch) > 0 || HAS_WAIT(ch))
    {
      GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
    }
    else
    {
      GET_MOB_WAIT(ch) = 0;
      if ((GET_POS(ch) < POS_FIGHTING) && (GET_POS(ch) > POS_STUNNED) &&
          !AFF_FLAGGED(ch, AFF_PINNED) && GET_HIT(ch) > 0)
      {
        change_position(ch, POS_FIGHTING); /* this should be changed with event system since pos_fight is deprecated */
        attacks_of_opportunity(ch, 0);
        send_to_char(ch, "You scramble to your feet!\r\n");
        if (is_flying(ch) || AFF_FLAGGED(ch, AFF_LEVITATE))
          act("$n scrambles to $s feet then launches back into the air!", TRUE, ch, 0, 0, TO_ROOM);
        else
          act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
      }
    }
  }

  /* Positions
 POS_DEAD       0
 POS_MORTALLYW	1
 POS_INCAP      2
 POS_STUNNED	3
 POS_SLEEPING	4
 POS_RESTING	5
 POS_SITTING	6
 POS_FIGHTING	7
 POS_STANDING	8	*/
  if (GET_POS(ch) < POS_SITTING && GET_POS(ch) != POS_RECLINING)
  {
    send_to_char(ch, "You are in no position to fight!!\r\n");
    return;
  }

  if (affected_by_spell(ch, PSIONIC_DEATH_URGE))
  {
    stop_fighting(ch);
    hit(ch, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    send_to_char(ch, "\tDCrazed self-harm\tc overcomes you and you lash out against yourself!\tn  ");
    act("$n \tcis overcome with \tDcrazed self-harm and lashes out against $mself\tc!\tn", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  // confusion code
  // 20% chance to act normal
  if (AFF_FLAGGED(ch, AFF_CONFUSED) && rand_number(0, 4))
  {

    // 30% to do nothing
    if (rand_number(1, 100) <= 30)
    {
      send_to_char(ch, "\tDConfusion\tc overcomes you and you stand dumbfounded!\tn  ");
      act("$n \tcis overcome with \tDconfusion and stands dumbfounded\tc!\tn",
          TRUE, ch, 0, 0, TO_ROOM);
      stop_fighting(ch);
      USE_FULL_ROUND_ACTION(ch);
      return;
    } // 20% to flee
    else if (rand_number(1, 100) <= 20)
    {
      send_to_char(ch, "\tDFear\tc overcomes you!\tn  ");
      act("$n \tcis overcome with \tDfear\tc!\tn",
          TRUE, ch, 0, 0, TO_ROOM);
      perform_flee(ch);
      perform_flee(ch);
      return;
    } // 30% to attack random
    else
    {
      /* allocate memory for list */
      room_list = create_list();

      /* dummy check */
      if (!IN_ROOM(ch))
        return;

      /* build list */
      for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
        if (tch)
          add_to_list(tch, room_list);

      /* If our list is empty or has "0" entries, we free it from memory
       * and flee for the hills */
      if (room_list->iSize == 0)
      {
        free_list(room_list);
        return;
      }

      /* ok we should have something in the list, pick randomly and switch
         to our new target */
      tch = random_from_list(room_list);
      if (tch)
      {
        stop_fighting(ch);
        hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        send_to_char(ch, "\tDConfusion\tc overcomes you and you lash out!\tn  ");
        act("$n \tcis overcome with \tDconfusion and lashes out\tc!\tn",
            TRUE, ch, 0, 0, TO_ROOM);
      }

      /* we're done, free the list */
      free_list(room_list);
      return;
    }
  }

  /* group members will autoassist if
   1)  npc or
   2)  pref flagged autoassist
   */
  if (GROUP(ch))
  {
    while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
    {
      if (tch == ch)
        continue;
      if (!IS_NPC(tch) && !PRF_FLAGGED(tch, PRF_AUTOASSIST))
        continue;
      if (IN_ROOM(ch) != IN_ROOM(tch))
        continue;
      if (FIGHTING(tch))
        continue;
      if (GET_POS(tch) != POS_STANDING)
        continue;
      if (!CAN_SEE(tch, ch))
        continue;

      perform_assist(tch, ch);
    }
  }

  // your charmee, even if not grouped, should assist
  for (charmee = world[IN_ROOM(ch)].people; charmee;
       charmee = charmee->next_in_room)
    if (AFF_FLAGGED(charmee, AFF_CHARM) && charmee->master == ch &&
        !FIGHTING(charmee) &&
        GET_POS(charmee) == POS_STANDING && CAN_SEE(charmee, ch))
      perform_assist(charmee, ch);

  /* here is our entry point for melee attack rotation */
  /* conditions for not performing melee attacks:
               -casting
               -total defense
               -grappling without light weapons*/
  if (IS_CASTING(ch))
    ;
  else if (AFF_FLAGGED(ch, AFF_TOTAL_DEFENSE))
    send_to_char(ch, "You continue the battle in defensive positioning!\r\n");

  else if (AFF_FLAGGED(ch, AFF_GRAPPLED) &&
           (!is_using_light_weapon(ch, GET_EQ(ch, WEAR_WIELD_1)) ||
            !is_using_light_weapon(ch, GET_EQ(ch, WEAR_WIELD_OFFHAND)) ||
            GET_EQ(ch, WEAR_WIELD_2H)))
    send_to_char(ch, "You need to fight unarmed or with light weapons (both hands) while grappling or being grappled!\r\n");

  else
  {

    /* handle smash defense */
    if (HAS_FEAT(ch, FEAT_SMASH_DEFENSE) && PRF_FLAGGED(ch, PRF_SMASH_DEFENSE) &&
        affected_by_spell(ch, SKILL_DEFENSIVE_STANCE) &&
        !char_has_mud_event(ch, eSMASH_DEFENSE))
      handle_smash_defense(ch);

#define NORMAL_ATTACK_ROUTINE 0
    perform_attacks(ch, NORMAL_ATTACK_ROUTINE, phase);
#undef NORMAL_ATTACK_ROUTINE

    /* handle cleave */
    if (phase == 1 && HAS_FEAT(ch, FEAT_CLEAVE) && !is_using_ranged_weapon(ch, TRUE))
      handle_cleave(ch);
  }
  /**/

  if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) &&
      !MOB_FLAGGED(ch, MOB_NOTDEADYET) && GET_HIT(ch) > 0)
  {
    char actbuf[MAX_INPUT_LENGTH] = "";
    (GET_MOB_SPEC(ch))(ch, ch, 0, actbuf);
  }

  // the mighty awesome fear code
  if (AFF_FLAGGED(ch, AFF_FEAR) && !rand_number(0, 2))
  {
    send_to_char(ch, "\tDFear\tc overcomes you!\tn  ");
    act("$n \tcis overcome with \tDfear\tc!\tn",
        TRUE, ch, 0, 0, TO_ROOM);
    perform_flee(ch);
  }
}

#undef HIT_MISS
#undef HIT_RESULT_ACTION
#undef HIT_NEED_RELOAD

#undef MODE_NORMAL_HIT      // Normal damage calculating in hit()
#undef MODE_DISPLAY_PRIMARY // Display damage info primary
#undef MODE_DISPLAY_OFFHAND // Display damage info offhand
#undef MODE_DISPLAY_RANGED  // Display damage info ranged

#undef MODE_2_WPN       /* two weapon fighting equivalent (reduce two weapon fighting penalty) */
#undef MODE_IMP_2_WPN   /* improved two weapon fighting - extra attack at -5 */
#undef MODE_GREAT_2_WPN /* greater two weapon fighting - extra attack at -10 */
#undef MODE_EPIC_2_WPN  /* perfect two weapon fighting - extra attack */

#undef THE_PRISONER       /* vnum for special mobile 'the prisoner' */
#undef DRACOLICH_PRISONER /* vnum for special mobile 'the dracolich of the prisoner' */

#undef DEBUGMODE
