/**************************************************************************
 *  File: spec_procs.c                                 Part of LuminariMUD *
 *  Usage: Implementation of special procedures for mobiles/objects/rooms. *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "modify.h"
#include "house.h"
#include "clan.h"
#include "mudlim.h"
#include "graph.h"
#include "dg_scripts.h" /* for send_to_zone() */
#include "mud_event.h"
#include "actions.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "feats.h"
#include "spell_prep.h"
#include "item.h" /* do_stat_object */
#include "alchemy.h"
#include "treasure.h"   /* for set_armor_object */
#include "mobact.h"     /* npc_find_target() */
#include "spell_prep.h" /* for star circlet proc */

/* toggle for debug mode
   true = annoying messages used for debugging
   false = normal gameplay */
#define DEBUGMODE FALSE

/* external functions */
extern struct house_control_rec house_control[];
extern int num_of_houses;

/* locally defined functions of local (file) scope */
static int compare_spells(const void *x, const void *y);
static void npc_steal(struct char_data *ch, struct char_data *victim);
static void zone_yell(struct char_data *ch, const char *buf);

/* Special procedures for mobiles. */
int spell_sort_info[TOP_SKILL_DEFINE];
int sorted_spells[TOP_SKILL_DEFINE];
int sorted_skills[TOP_SKILL_DEFINE];

#define LEARNED_LEVEL 0 /* % known which is considered "learned" */
#define MAX_PER_PRAC 1  /* max percent gain in skill per practice */
#define MIN_PER_PRAC 2  /* min percent gain in skill per practice */
#define PRAC_TYPE 3     /* should it say 'spell' or 'skill'?	 */

static int compare_spells(const void *x, const void *y)
{
  int a = *(const int *)x,
      b = *(const int *)y;

  if (a <= 1 || b <= 1)
    return FALSE;

  if (a > MAX_SKILLS || b > MAX_SKILLS)
    return FALSE;

  return strcmp(spell_info[a].name, spell_info[b].name);
}

/* this will create a full list, added two more lists
   to seperate the skills/spells */
void sort_spells(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a < TOP_SKILL_DEFINE; a++)
  {
    spell_sort_info[a] = a;
    sorted_spells[a] = -1;
    sorted_skills[a] = -1;
  }

  /* full list */

  qsort(&spell_sort_info[1], TOP_SKILL_DEFINE, sizeof(int),
        compare_spells);

  /* spell list */

  /* initialize array, avoiding reserved. */
  for (a = 1; a < TOP_SKILL_DEFINE; a++)
    sorted_spells[a] = a;

  qsort(&sorted_spells[1], TOP_SKILL_DEFINE, sizeof(int),
        compare_spells);

  /* skill list */

  /* initialize array, avoiding reserved. */
  for (a = 1; a < TOP_SKILL_DEFINE; a++)
    sorted_skills[a] = a + MAX_SPELLS;

  /*
  qsort(&sorted_skills[1], TOP_SKILL_DEFINE,
        sizeof(int), compare_spells);
        */
}

// returns true if you have all the requisites for the skill
// false if you don't

int meet_skill_reqs(struct char_data *ch, int skillnum)
{
  // doesn't apply to staff
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return TRUE;
  // spells should return true
  if (skillnum < NUM_SPELLS && skillnum > 0)
    return TRUE;

  /* i'm -trying- to keep this organized */
  switch (skillnum)
  {

    /* proficiencies */
  case SKILL_PROF_BASIC:
    if (GET_SKILL(ch, SKILL_PROF_MINIMAL))
      return TRUE;
    else
      return FALSE;
  case SKILL_PROF_ADVANCED:
    if (GET_SKILL(ch, SKILL_PROF_BASIC))
      return TRUE;
    else
      return FALSE;
  case SKILL_PROF_MASTER:
    if (GET_SKILL(ch, SKILL_PROF_ADVANCED))
      return TRUE;
    else
      return FALSE;
  case SKILL_PROF_EXOTIC:
    if (GET_SKILL(ch, SKILL_PROF_MASTER))
      return TRUE;
    else
      return FALSE;
  case SKILL_PROF_MEDIUM_A:
    if (GET_SKILL(ch, SKILL_PROF_LIGHT_A))
      return TRUE;
    else
      return FALSE;
  case SKILL_PROF_HEAVY_A:
    if (GET_SKILL(ch, SKILL_PROF_MEDIUM_A))
      return TRUE;
    else
      return FALSE;
  case SKILL_PROF_T_SHIELDS:
    if (GET_SKILL(ch, SKILL_PROF_SHIELDS))
      return TRUE;
    else
      return FALSE;

    /* epic spells */
  case SKILL_MUMMY_DUST:
    if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 23 && GET_LEVEL(ch) >= 20)
      return TRUE;
    else
      return FALSE;
  case SKILL_DRAGON_KNIGHT:
    if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 25 && GET_LEVEL(ch) >= 20 &&
        (CLASS_LEVEL(ch, CLASS_WIZARD) > 17 ||
         CLASS_LEVEL(ch, CLASS_SORCERER) > 19))
      return TRUE;
    else
      return FALSE;
  case SKILL_GREATER_RUIN:
    if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 27 && GET_LEVEL(ch) >= 20)
      return TRUE;
    else
      return FALSE;
  case SKILL_HELLBALL:
    if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 29 && GET_LEVEL(ch) >= 20 &&
        (CLASS_LEVEL(ch, CLASS_WIZARD) > 16 ||
         CLASS_LEVEL(ch, CLASS_SORCERER) > 18))
      return TRUE;
    else
      return FALSE;
    /* magical based epic spells (not accessable by divine) */
  case SKILL_EPIC_MAGE_ARMOR:
    if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 31 && GET_LEVEL(ch) >= 20 && (CLASS_LEVEL(ch, CLASS_WIZARD) > 13 || CLASS_LEVEL(ch, CLASS_SORCERER) > 13))
      return TRUE;
    else
      return FALSE;
  case SKILL_EPIC_WARDING:
    if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 33 && GET_LEVEL(ch) >= 20 && (CLASS_LEVEL(ch, CLASS_WIZARD) > 15 || CLASS_LEVEL(ch, CLASS_SORCERER) > 15))
      return TRUE;
    else
      return FALSE;

    /* 'epic' skills */
  case SKILL_BLINDING_SPEED:
    if (GET_REAL_DEX(ch) >= 21 && GET_LEVEL(ch) >= 20)
      return TRUE;
    else
      return FALSE;
  case SKILL_SPELL_RESIST_4:
    if (GET_LEVEL(ch) >= 20 && GET_SKILL(ch, SKILL_SPELL_RESIST_3))
      return TRUE;
    else
      return FALSE;
  case SKILL_SPELL_RESIST_5:
    if (GET_LEVEL(ch) >= 25 && GET_SKILL(ch, SKILL_SPELL_RESIST_4))
      return TRUE;
    else
      return FALSE;
  case SKILL_IMPROVED_BASH:
    if (GET_SKILL(ch, SKILL_BASH) && GET_LEVEL(ch) >= 20)
      return TRUE;
    else
      return FALSE;
  case SKILL_IMPROVED_WHIRL:
    if (GET_SKILL(ch, SKILL_WHIRLWIND) && GET_LEVEL(ch) >= 20)
      return TRUE;
    else
      return FALSE;
  case SKILL_ARMOR_SKIN:
    if (GET_LEVEL(ch) >= 20)
      return TRUE;
    else
      return FALSE;
  case SKILL_SELF_CONCEAL_3:
    if (GET_REAL_DEX(ch) >= 21 && GET_SKILL(ch, SKILL_SELF_CONCEAL_2))
      return TRUE;
    else
      return FALSE;
  case SKILL_OVERWHELMING_CRIT:
    if (GET_LEVEL(ch) >= 20)
      return TRUE;
    else
      return FALSE;
  case SKILL_DAMAGE_REDUC_3:
    if (GET_REAL_CON(ch) >= 19 && GET_SKILL(ch, SKILL_DAMAGE_REDUC_2))
      return TRUE;
    else
      return FALSE;
  case SKILL_EPIC_REFLEXES:
  case SKILL_EPIC_FORTITUDE:
  case SKILL_EPIC_WILL:
    if (GET_LEVEL(ch) >= 20)
      return TRUE;
    else
      return FALSE;
  case SKILL_IMPROVED_TRIP:
    if (GET_SKILL(ch, SKILL_TRIP) && GET_LEVEL(ch) >= 20)
      return TRUE;
    else
      return FALSE;
  case SKILL_HEADBUTT:
    if (GET_LEVEL(ch) >= 20 &&
        (GET_REAL_CON(ch) + GET_REAL_STR(ch) >= 32))
      return TRUE;
    else
      return FALSE;

    /* melee combat related */
  case SKILL_AMBIDEXTERITY:
    if (GET_REAL_DEX(ch) >= 13)
      return TRUE;
    else
      return FALSE;
  case SKILL_BASH:
    if (GET_REAL_STR(ch) >= 13)
      return TRUE;
    else
      return FALSE;
  case SKILL_TRIP:
    if (GET_REAL_DEX(ch) >= 13)
      return TRUE;
    else
      return FALSE;
  case SKILL_WHIRLWIND:
  case SKILL_DAMAGE_REDUC_1:
    if (GET_REAL_CON(ch) >= 15)
      return TRUE;
    else
      return FALSE;
  case SKILL_DAMAGE_REDUC_2:
    if (GET_REAL_CON(ch) >= 17 && GET_SKILL(ch, SKILL_DAMAGE_REDUC_1))
      return TRUE;
    else
      return FALSE;
  case SKILL_SELF_CONCEAL_1:
    if (GET_REAL_DEX(ch) >= 15)
      return TRUE;
    else
      return FALSE;
  case SKILL_SELF_CONCEAL_2:
    if (GET_REAL_DEX(ch) >= 17 && GET_SKILL(ch, SKILL_SELF_CONCEAL_1))
      return TRUE;
    else
      return FALSE;
  case SKILL_EPIC_CRIT:
    if (GET_LEVEL(ch) >= 10 && GET_SKILL(ch, SKILL_IMPROVED_CRITICAL))
      return TRUE;
    else
      return FALSE;

    /* more caster related */
  case SKILL_SPELL_RESIST_1:
    if (GET_LEVEL(ch) >= 5)
      return TRUE;
    else
      return FALSE;
  case SKILL_SPELL_RESIST_2:
    if (GET_LEVEL(ch) >= 10 && GET_SKILL(ch, SKILL_SPELL_RESIST_1))
      return TRUE;
    else
      return FALSE;
  case SKILL_SPELL_RESIST_3:
    if (GET_LEVEL(ch) >= 15 && GET_SKILL(ch, SKILL_SPELL_RESIST_2))
      return TRUE;
    else
      return FALSE;
  case SKILL_QUICK_CHANT:
    if (CASTER_LEVEL(ch))
      return TRUE;
    else
      return FALSE;

    /* special restrictions, i.e. not restricted to one class, etc */
  case SKILL_USE_MAGIC: /* shared - with casters and rogue */
    if ((CLASS_LEVEL(ch, CLASS_ROGUE) >= 9) ||
        (IS_CASTER(ch) && GET_LEVEL(ch) >= 2))
      return TRUE;
    else
      return FALSE;
  case SKILL_CALL_FAMILIAR: // sorc, wiz only
    if (CLASS_LEVEL(ch, CLASS_SORCERER) || CLASS_LEVEL(ch, CLASS_WIZARD))
      return TRUE;
    else
      return FALSE;
  case SKILL_RECHARGE: // casters only
    if (CASTER_LEVEL(ch) >= 14)
      return TRUE;
    else
      return FALSE;
  case SKILL_TRACK: // rogue / ranger / x-stats only
    if (CLASS_LEVEL(ch, CLASS_ROGUE) || CLASS_LEVEL(ch, CLASS_RANGER) ||
        (GET_WIS(ch) + GET_INT(ch) >= 28))
      return TRUE;
    else
      return FALSE;
  case SKILL_CHARGE:
    if (GET_ABILITY(ch, ABILITY_RIDE) >= 10)
      return TRUE;
    else
      return FALSE;
  case SKILL_HITALL:
    if ((GET_REAL_STR(ch) + GET_REAL_CON(ch)) >= 29)
      return TRUE;
    else
      return FALSE;
  case SKILL_SHIELD_PUNCH:
    if (GET_SKILL(ch, SKILL_SHIELD_SPECIALIST))
      return TRUE;
    else
      return FALSE;
  case SKILL_BODYSLAM:
    if (GET_RACE(ch) == RACE_HALF_TROLL)
      return TRUE;
    else
      return FALSE;

    /* ranger */
  case SKILL_NATURE_STEP: // shared with druid
    if (CLASS_LEVEL(ch, CLASS_RANGER) >= 3 ||
        CLASS_LEVEL(ch, CLASS_DRUID) >= 6)
      return TRUE;
    else
      return FALSE;

    /* druid */
    // animal companion - level 1 (shared with ranger)
    // nature step - level 6 (shared with ranger)

    /* warrior */
  case SKILL_SHIELD_SPECIALIST: // not a free skill
    if (WARRIOR_LEVELS(ch) >= 6)
      return TRUE;
    else
      return FALSE;

    /* monk */
  case SKILL_STUNNING_FIST:
    if (MONK_TYPE(ch) >= 2)
      return TRUE;
    else
      return FALSE;
  case SKILL_SPRINGLEAP:
    if (MONK_TYPE(ch) >= 6)
      return TRUE;
    else
      return FALSE;
  case SKILL_QUIVERING_PALM:
    if (MONK_TYPE(ch) >= 15)
      return TRUE;
    else
      return FALSE;

    /* bard */
  case SKILL_PERFORM:
    if (CLASS_LEVEL(ch, CLASS_BARD) >= 2)
      return TRUE;
    else
      return FALSE;

    /* paladin */
    /* rogue */
  case SKILL_BACKSTAB:
    if (CLASS_LEVEL(ch, CLASS_ROGUE))
      return TRUE;
    else
      return FALSE;
  case SKILL_DIRTY_FIGHTING:
    if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 4)
      return TRUE;
    else
      return FALSE;
  case SKILL_SAP: // not a free skill
    if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 10)
      return TRUE;
    else
      return FALSE;
  case SKILL_SLIPPERY_MIND:
    if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 15)
      return TRUE;
    else
      return FALSE;
  case SKILL_DEFENSE_ROLL:
    if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 18)
      return TRUE;
    else
      return FALSE;
  case SKILL_DIRT_KICK:
    if (GET_LEVEL(ch) >= 20 && GET_REAL_DEX(ch) >= 17)
    {
      if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 15)
        return TRUE;
    }
    else
      return FALSE;

    /* berserker */
  case SKILL_RAGE:
    if (CLASS_LEVEL(ch, CLASS_BERSERKER) >= 2)
      return TRUE;
    else
      return FALSE;

    /*** no reqs ***/
  case SKILL_RESCUE:
  case SKILL_LUCK_OF_HEROES:
  case SKILL_KICK:
  case SKILL_IMPROVED_CRITICAL:
  case SKILL_PROWESS:
  case SKILL_PROF_MINIMAL:
  case SKILL_PROF_SHIELDS:
  case SKILL_PROF_LIGHT_A:
  case SKILL_MINING:
  case SKILL_HUNTING:
  case SKILL_FORESTING:
  case SKILL_KNITTING:
  case SKILL_CHEMISTRY:
  case SKILL_ARMOR_SMITHING:
  case SKILL_WEAPON_SMITHING:
  case SKILL_JEWELRY_MAKING:
  case SKILL_LEATHER_WORKING:
  case SKILL_FAST_CRAFTER:
    return TRUE;

    /**
     *  not implemented yet or
     * unattainable
     *  **/
  case SKILL_MURMUR:
  case SKILL_PROPAGANDA:
  case SKILL_LOBBY:
  case SKILL_BONE_ARMOR:
  case SKILL_ELVEN_CRAFTING:
  case SKILL_MASTERWORK_CRAFTING:
  case SKILL_DRACONIC_CRAFTING:
  case SKILL_DWARVEN_CRAFTING:
  case SKILL_SPELLBATTLE: // arcana golem innate
  default:
    return FALSE;
  }
  return FALSE;
}

/* completely re-written for Luminari, probably needs to be rewritten again :P
   this is the engine for the 'spells' and 'spelllist' commands
   class - you can send -1 for a 'default' class
   mode = 0:  known spells
   mode = anything else: full spelllist for given class
   circle = What spell circle to list, -1 for all.
 */
void list_spells(struct char_data *ch, int mode, int class, int circle)
{
  int i = 0, slot = 0, sinfo = 0;
  int bottom = 0, top = 0;
  size_t len = 0, nlen = 0;
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  const char *overflow = "\r\n**OVERFLOW**\r\n";
  if (!ch)
    return;
  int domain_1 = GET_1ST_DOMAIN(ch);
  int domain_2 = GET_2ND_DOMAIN(ch);
  bool is_psionic = (class == CLASS_PSIONICIST);

  // default class case
  if (class == -1)
  {
    class = GET_CLASS(ch);
    if (!CLASS_LEVEL(ch, class))
      send_to_char(ch, "You don't have any levels in your current class.\r\n");
  }

  if (mode == 0)
  {

    len = snprintf(buf2, sizeof(buf2), "\tCKnown %s List\tn\r\n%s", is_psionic ? "Power" : "Spell",
                   (is_psionic && CLASS_LEVEL(ch, CLASS_PSIONICIST) == 1) ? "\tYNOTE:\tnThere is a known bug where new psionicists will show all powers instead of\r\n"
                                                                            "only the ones they know. To correct this, please quit, then press '0' to return to\r\n"
                                                                            "the account menu, and login again.\r\n"
                                                                          : "");

    for (slot = get_class_highest_circle(ch, class); slot > 0; slot--)
    {
      if ((circle != -1) && circle != slot)
        continue;
      nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                      "\r\n\tC%s Circle Level %d\tn\r\n", is_psionic ? "Power" : "Spell", slot);
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;

      bottom = 1;
      top = TOP_SKILL_DEFINE;

      for (; bottom < top; bottom++)
      {
        i = spell_sort_info[bottom];
        if (do_not_list_spell(i))
          continue;
        sinfo = spell_info[i].min_level[class];

        /* SPELL PREPARATION HOOK (spellCircle) */
        if (class == CLASS_SORCERER && is_a_known_spell(ch, CLASS_SORCERER, i) &&
            compute_spells_circle(CLASS_SORCERER, i, 0, DOMAIN_UNDEFINED) == slot)
        {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                          "%-30s %2ds base spellcasting time\r\n", spell_info[i].name, spell_info[i].time);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
          /* SPELL PREPARATION HOOK (spellCircle) */
        }
        else if (class == CLASS_BARD && is_a_known_spell(ch, CLASS_BARD, i) &&
                 compute_spells_circle(CLASS_BARD, i, 0, DOMAIN_UNDEFINED) == slot)
        {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                          "%-30s %2ds base spellcasting time\r\n", spell_info[i].name, spell_info[i].time);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
          /* SPELL PREPARATION HOOK (spellCircle) */
        }
        else if (class == CLASS_INQUISITOR && is_a_known_spell(ch, CLASS_INQUISITOR, i) &&
                 compute_spells_circle(CLASS_INQUISITOR, i, 0, GET_1ST_DOMAIN(ch)) == slot)
        {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                          "%-30s %2ds base spellcasting time\r\n", spell_info[i].name, spell_info[i].time);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
          /* SPELL PREPARATION HOOK (spellCircle) */
        }
        else if (class == CLASS_PSIONICIST && is_a_known_spell(ch, CLASS_PSIONICIST, i) &&
                 compute_spells_circle(CLASS_PSIONICIST, i, 0, DOMAIN_UNDEFINED) == slot)
        {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                          "%-30s %2ds base manifesting time\r\n", spell_info[i].name, spell_info[i].time);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
          /* SPELL PREPARATION HOOK (spellCircle) */
        }
        else if (class == CLASS_WIZARD && spellbook_ok(ch, i, class, FALSE) &&
                 (BONUS_CASTER_LEVEL(ch, class) + CLASS_LEVEL(ch, class)) >= sinfo &&
                 compute_spells_circle(class, i, 0, DOMAIN_UNDEFINED) == slot &&
                 GET_SKILL(ch, i))
        {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                          "%-30s %-15s \tRReady\tn %2ds base spellcasting time\r\n", spell_info[i].name,
                          school_names_specific[spell_info[i].schoolOfMagic], spell_info[i].time);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
          /* SPELL PREPARATION HOOK (spellCircle) */
        }
        else if (class != CLASS_SORCERER && class != CLASS_BARD && class != CLASS_WIZARD && class != CLASS_INQUISITOR && class != CLASS_PSIONICIST &&
                 (BONUS_CASTER_LEVEL(ch, class) + CLASS_LEVEL(ch, class)) >= MIN_SPELL_LVL(i, class, domain_1) && compute_spells_circle(class, i, 0, domain_1) == slot &&
                 GET_SKILL(ch, i))
        {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                          "%-30s %2ds base spellcasting time\r\n", spell_info[i].name, spell_info[i].time);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
          /* SPELL PREPARATION HOOK (spellCircle) */
        }
        else if (class != CLASS_SORCERER && class != CLASS_BARD && class != CLASS_WIZARD && class != CLASS_INQUISITOR && class != CLASS_PSIONICIST &&
                 (BONUS_CASTER_LEVEL(ch, class) + CLASS_LEVEL(ch, class)) >= MIN_SPELL_LVL(i, class, domain_2) && compute_spells_circle(class, i, 0, domain_2) == slot &&
                 GET_SKILL(ch, i))
        {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                          "%-30s %2ds base spellcasting time\r\n", spell_info[i].name, spell_info[i].time);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
      }
    }
  }
  else
  {
    len = snprintf(buf2, sizeof(buf2), "\tCFull %s List\tn\r\n", is_psionic ? "Power" : "Spell");

    if (class == CLASS_PALADIN || class == CLASS_RANGER)
      slot = 4;
    if (class == CLASS_ALCHEMIST)
      slot = 6;
    else
      slot = 9;

    for (; slot > 0; slot--)
    {
      if ((circle != -1) && circle != slot)
        continue;
      nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                      "\r\n\tC%s Circle Level %d\tn\r\n", is_psionic ? "Power" : (class == CLASS_ALCHEMIST ? "Extract" : "Spell"), slot);
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;

      for (i = 1; i < MAX_SPELLS; i++)
      {
        sinfo = spell_info[i].min_level[class];

        /* SPELL PREPARATION HOOK (spellCircle) */
        if (compute_spells_circle(class, i, 0, DOMAIN_UNDEFINED) == slot)
        {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                          "%-20s %-15s\r\n", spell_info[i].name,
                          school_names_specific[spell_info[i].schoolOfMagic]);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
      }
    }
  }
  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */

  page_string(ch->desc, buf2, TRUE);
}

void list_crafting_skills(struct char_data *ch)
{
  int i, printed = 0;

  if (IS_NPC(ch))
    return;

  /* Crafting Skills */
  send_to_char(ch, "\tCCrafting Skills\tn\r\n\r\n");
  for (i = START_SKILLS; i < NUM_SKILLS; i++)
  {
    // Why is this level check here? Gicker Feb 8, 2021
    // if (GET_LEVEL(ch) >= spell_info[i].min_level[GET_CLASS(ch)] &&
    if (spell_info[i].schoolOfMagic == CRAFTING_SKILL)
    {
      if (meet_skill_reqs(ch, i))
      {
        send_to_char(ch, "%-24s %d          ", spell_info[i].name, GET_SKILL(ch, i));
        printed++;
        if (!(printed % 2))
          send_to_char(ch, "\r\n");
      }
    }
  }
  send_to_char(ch, "\r\n");
}

void list_skills(struct char_data *ch)
{
  int i, printed = 0;

  if (IS_NPC(ch))
    return;

  /* Active Skills */
  send_to_char(ch, "\tCActive Skills\tn\r\n\r\n");
  for (i = MAX_SPELLS + 1; i < TOP_SKILL_DEFINE; i++)
  {
    if (GET_LEVEL(ch) >= spell_info[i].min_level[GET_CLASS(ch)] &&
        spell_info[i].schoolOfMagic == ACTIVE_SKILL)
    {
      if (meet_skill_reqs(ch, i))
      {
        send_to_char(ch, "%-24s", spell_info[i].name);
        if (!GET_SKILL(ch, i))
          send_to_char(ch, "  \tYUnlearned\tn ");
        else if (GET_SKILL(ch, i) >= 99)
          send_to_char(ch, "  \tWMastered \tn ");
        else if (GET_SKILL(ch, i) >= 95)
          send_to_char(ch, "  \twSuperb \tn ");
        else if (GET_SKILL(ch, i) >= 90)
          send_to_char(ch, "  \tMExcellent \tn ");
        else if (GET_SKILL(ch, i) >= 85)
          send_to_char(ch, "  \tmAdvanced \tn ");
        else if (GET_SKILL(ch, i) >= 80)
          send_to_char(ch, "  \tBSkilled \tn ");
        else
          send_to_char(ch, "  \tGLearned  \tn ");
        printed++;
        if (!(printed % 2))
          send_to_char(ch, "\r\n");
      }
    }
  }
  send_to_char(ch, "\r\n\r\n");

  /* Passive Skills */
  send_to_char(ch, "\tCPassive Skills\tn\r\n\r\n");
  for (i = MAX_SPELLS + 1; i < NUM_SKILLS; i++)
  {
    if (GET_LEVEL(ch) >= spell_info[i].min_level[GET_CLASS(ch)] &&
        spell_info[i].schoolOfMagic == PASSIVE_SKILL)
    {
      if (meet_skill_reqs(ch, i))
      {
        send_to_char(ch, "%-24s", spell_info[i].name);
        if (!GET_SKILL(ch, i))
          send_to_char(ch, "  \tYUnlearned\tn ");
        else if (GET_SKILL(ch, i) >= 99)
          send_to_char(ch, "  \tWMastered \tn ");
        else if (GET_SKILL(ch, i) >= 95)
          send_to_char(ch, "  \twSuperb \tn ");
        else if (GET_SKILL(ch, i) >= 90)
          send_to_char(ch, "  \tMExcellent \tn ");
        else if (GET_SKILL(ch, i) >= 85)
          send_to_char(ch, "  \tmAdvanced \tn ");
        else if (GET_SKILL(ch, i) >= 80)
          send_to_char(ch, "  \tBSkilled \tn ");
        else
          send_to_char(ch, "  \tGLearned  \tn ");
        printed++;
        if (!(printed % 2))
          send_to_char(ch, "\r\n");
      }
    }
  }
  send_to_char(ch, "\r\n\r\n");

  /* Caster Skills */
  send_to_char(ch, "\tCCaster Skills\tn\r\n\r\n");
  for (i = MAX_SPELLS + 1; i < TOP_SKILL_DEFINE; i++)
  {
    if (GET_LEVEL(ch) >= spell_info[i].min_level[GET_CLASS(ch)] &&
        spell_info[i].schoolOfMagic == CASTER_SKILL)
    {
      if (meet_skill_reqs(ch, i))
      {
        send_to_char(ch, "%-24s", spell_info[i].name);
        if (!GET_SKILL(ch, i))
          send_to_char(ch, "  \tYUnlearned\tn ");
        else if (GET_SKILL(ch, i) >= 99)
          send_to_char(ch, "  \tWMastered \tn ");
        else if (GET_SKILL(ch, i) >= 95)
          send_to_char(ch, "  \twSuperb \tn ");
        else if (GET_SKILL(ch, i) >= 90)
          send_to_char(ch, "  \tMExcellent \tn ");
        else if (GET_SKILL(ch, i) >= 85)
          send_to_char(ch, "  \tmAdvanced \tn ");
        else if (GET_SKILL(ch, i) >= 80)
          send_to_char(ch, "  \tBSkilled \tn ");
        else
          send_to_char(ch, "  \tGLearned  \tn ");
        printed++;
        if (!(printed % 2))
          send_to_char(ch, "\r\n");
      }
    }
  }
  send_to_char(ch, "\r\n\r\n");

  /* Crafting Skills */
  send_to_char(ch, "\tCCrafting Skills\tn\r\n\r\n");
  for (i = START_SKILLS + 1; i < NUM_SKILLS; i++)
  {
    if (GET_LEVEL(ch) >= spell_info[i].min_level[GET_CLASS(ch)] &&
        spell_info[i].schoolOfMagic == CRAFTING_SKILL)
    {
      if (meet_skill_reqs(ch, i))
      {
        send_to_char(ch, "%-24s %d          ", spell_info[i].name, GET_SKILL(ch, i));
        printed++;
        if (!(printed % 2))
          send_to_char(ch, "\r\n");
      }
    }
  }
  send_to_char(ch, "\r\n\r\n");

  send_to_char(ch, "\tCPractice Session(s): %d\tn\r\n\r\n",
               GET_PRACTICES(ch));
}

int compute_ability(struct char_data *ch, int abilityNum)
{
  return compute_ability_full(ch, abilityNum, false);
}

int compute_ability_full(struct char_data *ch, int abilityNum, bool recursive)
{
  int value = 0;

  if (!ch)
    return -1;

  if (abilityNum < 1 || abilityNum > NUM_ABILITIES)
    return -1;

  /* this dummy check was added to to possible problems with checking
   affected_by_spell on a target that just died */
  if (GET_HIT(ch) <= 0 || GET_POS(ch) <= POS_STUNNED)
    return -1;

  // If it's not set as !recursive, we'll have an infinite loop.
  // See teamwork_best_stealth for context.
  if (!recursive)
  {
    if (abilityNum == ABILITY_STEALTH)
    {
      if (has_teamwork_feat(ch, FEAT_STEALTH_SYNERGY))
        return teamwork_best_stealth(ch, FEAT_STEALTH_SYNERGY);
    }
  }

  if (affected_by_spell(ch, PSIONIC_INFLICT_PAIN))
    value += get_char_affect_modifier(ch, PSIONIC_INFLICT_PAIN, APPLY_HITROLL); // this should return a negative number, so + a - is -
  if (affected_by_spell(ch, SPELL_HEROISM))
    value += 2;
  else if (affected_by_spell(ch, SPELL_GREATER_HEROISM))
    value += 4;
  if (affected_by_spell(ch, SKILL_PERFORM))
    value += SONG_AFF_VAL(ch);
  if (HAS_FEAT(ch, FEAT_ABLE_LEARNER))
    value += 1;
  if (HAS_SKILL_FEAT(ch, abilityNum, feat_to_skfeat(FEAT_SKILL_FOCUS)))
    value += 3;
  if (HAS_SKILL_FEAT(ch, abilityNum, feat_to_skfeat(FEAT_EPIC_SKILL_FOCUS)))
    value += 6;
  if (!IS_NPC(ch) && IS_DAYLIT(IN_ROOM(ch)) && HAS_FEAT(ch, FEAT_LIGHT_BLINDNESS))
    value -= 1;
  if (IS_FRIGHTENED(ch))
    value -= 2;
  if (AFF_FLAGGED(ch, AFF_SICKENED))
    value -= 2;
  if (char_has_mud_event(ch, eHOLYJAVELIN))
    value -= 2;
  // try to avoid sending NPC's here, but just in case:
  /* Note on this:  More and more it seems necessary to have some
   * sort of NPC skill system in place, either an actual set
   * of SKILLS or some way to translate level, race and class into
   * an appropriate set of skills, mostly for intellignet, humanoid
   * NPCs. For now, just use the level, although that will be difficult. */
  if (IS_NPC(ch))
    value += GET_LEVEL(ch) - 1;
  else
    value += GET_ABILITY(ch, abilityNum);

  /* Check for armor proficiency? */

  struct affected_type *af = NULL;

  for (af = ch->affected; af; af = af->next)
  {
    if (af->location == APPLY_SKILL)
    {
      if (af->spell == SKILL_INSPIRING_COGNATOGEN)
        value += af->modifier;
      else if (af->specific == abilityNum)
        value += af->modifier;
    }
  }

  int high_eq = 0, i = 0, j = 0;

  for (i = 0; i < NUM_WEARS; i++)
  {
    for (j = 0; j < 6; j++)
    {
      if (GET_EQ(ch, i) && GET_EQ(ch, i)->affected[j].location == APPLY_SKILL && GET_EQ(ch, i)->affected[j].specific == abilityNum)
      {
        if (GET_EQ(ch, i)->affected[j].modifier > high_eq)
          high_eq = GET_EQ(ch, i)->affected[j].modifier;
      }
    }
  }

  value += high_eq;

  switch (abilityNum)
  {

  case ABILITY_ACROBATICS:
    value += GET_DEX_BONUS(ch);
    value += compute_gear_armor_penalty(ch);
    if (HAS_FEAT(ch, FEAT_AGILE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_ACROBATIC))
    {
      /* Unnamed bonus */
      value += 3;
    }
    if (AFF_FLAGGED(ch, AFF_ACROBATIC))
      value += 10;
    return value;

  case ABILITY_STEALTH:
    value += GET_DEX_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_AFFINITY_MOVE_SILENT))
      value += 4;
    if (HAS_FEAT(ch, FEAT_STEALTHY))
      value += 2;
    if (GET_RACE(ch) == RACE_HALFLING)
      value += 2;
    if (IS_LICH(ch))
      value += 8;
    if (AFF_FLAGGED(ch, AFF_REFUGE))
      value += 15;
    if (IS_MORPHED(ch) && SUBRACE(ch) == PC_SUBRACE_PANTHER)
      value += 4;
    if (KNOWS_DISCOVERY(ch, ALC_DISC_CHAMELEON))
    {
      if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 10)
      {
        value += 8;
      }
      else
      {
        value += 4;
      }
    }
    value += compute_gear_armor_penalty(ch);
    return value;

  case ABILITY_PERCEPTION:
    value += GET_WIS_BONUS(ch);

    if (HAS_FEAT(ch, FEAT_AFFINITY_LISTEN))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_AFFINITY_SPOT))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_KEEN_SENSES))
    {
      /* Unnamed bonus, elves */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_ALERTNESS))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_INVESTIGATOR))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (affected_by_spell(ch, PSIONIC_UBIQUITUS_VISION))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (AFF_FLAGGED(ch, AFF_DAZZLED))
      value--;
    if (AFF_FLAGGED(ch, AFF_DEAF))
      value -= 4;
    if (IS_LICH(ch))
      value += 8;
    return value;
  case ABILITY_HEAL:
    value += GET_WIS_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_SELF_SUFFICIENT))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_INTIMIDATE:
    value += GET_CHA_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_PERSUASIVE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_STERN_GAZE))
    {
      /* Unnamed bonus */
      value += MAX(1, CLASS_LEVEL(ch, CLASS_INQUISITOR) / 2);
    }
    return value;
  case ABILITY_CONCENTRATION: /* not srd */
    if (GET_RACE(ch) == RACE_GNOME)
      value += 2;
    value += GET_CON_BONUS(ch);
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_ARCANA_GOLEM)
    {
      value += GET_LEVEL(ch) / 6;
    }
    if (is_judgement_possible(ch, FIGHTING(ch), INQ_JUDGEMENT_PIERCING))
      value += get_judgement_bonus(ch, INQ_JUDGEMENT_PIERCING);
    if (has_teamwork_feat(ch, FEAT_SHIELDED_CASTER))
    {
      value += 4 + teamwork_using_shield(ch, FEAT_SHIELDED_CASTER);
    }
    return value;
  case ABILITY_SPELLCRAFT:
    value += GET_INT_BONUS(ch);
    if (!IS_NPC(ch) && GET_RACE(ch) == RACE_ARCANA_GOLEM)
    {
      value += GET_LEVEL(ch) / 6;
    }
    if (HAS_FEAT(ch, FEAT_MAGICAL_APTITUDE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_APPRAISE:
    value += GET_INT_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_DILIGENT))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_DISCIPLINE: /* NOT SRD! */
    if (GET_RACE(ch) == RACE_H_ELF)
      value += 2;
    value += GET_STR_BONUS(ch);
    value += compute_gear_armor_penalty(ch);
    return value;
  case ABILITY_TOTAL_DEFENSE: /* not srd */
    value += GET_DEX_BONUS(ch);
    value += compute_gear_armor_penalty(ch);
    if (HAS_FEAT(ch, FEAT_PARRY))
      value += 4;
    return value;
  case ABILITY_LORE: /* NOT SRD! */
    if (HAS_FEAT(ch, FEAT_INVESTIGATOR))
      value += 2;
    if (GET_RACE(ch) == RACE_H_ELF)
      value += 2;
    value += GET_INT_BONUS(ch);
    return value;
  case ABILITY_RIDE:
    if (!HAS_FEAT(ch, FEAT_LEGENDARY_RIDER))
      value += compute_gear_armor_penalty(ch);
    if (!HAS_FEAT(ch, FEAT_GLORIOUS_RIDER))
      value += GET_DEX_BONUS(ch);
    else
      value += GET_CHA_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_ANIMAL_AFFINITY))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_CLIMB:
    value += GET_STR_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_ATHLETIC))
    {
      /* Unnamed bonus */
      value += 2;
    }
    value += compute_gear_armor_penalty(ch);
    return value;
  case ABILITY_SLEIGHT_OF_HAND:
    value += GET_DEX_BONUS(ch);
    value += compute_gear_armor_penalty(ch);
    if (HAS_FEAT(ch, FEAT_DEFT_HANDS))
    {
      /* Unnamed bonus */
      value += 3;
    }
    return value;
  case ABILITY_BLUFF:
    value += GET_CHA_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_PERSUASIVE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_DIPLOMACY:
    value += GET_CHA_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_NEGOTIATOR))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_DISABLE_DEVICE:
    value += GET_INT_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_NIMBLE_FINGERS))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_DISGUISE:
    value += GET_CHA_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_DECEITFUL))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_ESCAPE_ARTIST:
    value += GET_DEX_BONUS(ch);
    value += compute_gear_armor_penalty(ch);
    if (HAS_FEAT(ch, FEAT_AGILE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_HANDLE_ANIMAL:
    value += GET_CHA_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_ANIMAL_AFFINITY))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_SENSE_MOTIVE:
    value += GET_WIS_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_NEGOTIATOR))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_KEEN_SENSES))
    {
      /* Unnamed bonu, elves */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_ALERTNESS))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_STERN_GAZE))
    {
      /* Unnamed bonus */
      value += MAX(1, CLASS_LEVEL(ch, CLASS_INQUISITOR) / 2);
    }
    if (IS_LICH(ch))
      value += 8;
    return value;
  case ABILITY_SURVIVAL:
    value += GET_WIS_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_SELF_SUFFICIENT))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_SWIM:
    value += GET_STR_BONUS(ch);
    value += (2 * compute_gear_armor_penalty(ch));
    if (HAS_FEAT(ch, FEAT_ATHLETIC))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_USE_MAGIC_DEVICE:
    if (HAS_FEAT(ch, FEAT_MAGICAL_APTITUDE))
    {
      /* Unnamed bonus */
      value += (value >= 10 ? 4 : 2);
    }
    if (HAS_FEAT(ch, FEAT_DILIGENT))
    {
      /* Unnamed bonus */
      value += 2;
    }
    value += GET_CHA_BONUS(ch);
    return value;
  case ABILITY_PERFORM:
    value += GET_CHA_BONUS(ch);
    return value;

    /* Knowledge Skills */
  case ABILITY_CRAFT_WOODWORKING:
  case ABILITY_CRAFT_TAILORING:
  case ABILITY_CRAFT_ALCHEMY:
  case ABILITY_CRAFT_ARMORSMITHING:
  case ABILITY_CRAFT_WEAPONSMITHING:
  case ABILITY_CRAFT_BOWMAKING:
  case ABILITY_CRAFT_GEMCUTTING:
  case ABILITY_CRAFT_LEATHERWORKING:
  case ABILITY_CRAFT_TRAPMAKING:
  case ABILITY_CRAFT_POISONMAKING:
  case ABILITY_CRAFT_METALWORKING:
    value += GET_INT_BONUS(ch);
    return value;
  case ABILITY_KNOWLEDGE_ARCANA:
  case ABILITY_KNOWLEDGE_ENGINEERING:
  case ABILITY_KNOWLEDGE_DUNGEONEERING:
  case ABILITY_KNOWLEDGE_GEOGRAPHY:
  case ABILITY_KNOWLEDGE_HISTORY:
  case ABILITY_KNOWLEDGE_LOCAL:
  case ABILITY_KNOWLEDGE_NATURE:
  case ABILITY_KNOWLEDGE_NOBILITY:
  case ABILITY_KNOWLEDGE_RELIGION:
  case ABILITY_KNOWLEDGE_PLANES:
    value += GET_INT_BONUS(ch);
    return value;
  default:
    return -1;
  }
}

/** cross-class or not? **/
const char *cross_names[] = {
    "\tRNot Available to Your Class\tn",
    "\tcCross-Class Ability\tn",
    "\tWClass Ability\tn"};

void list_abilities(struct char_data *ch, int ability_type)
{

  int i, start_ability, end_ability;

  switch (ability_type)
  {
  case ABILITY_TYPE_ALL:
    start_ability = 1;
    end_ability = NUM_ABILITIES;
    break;
  case ABILITY_TYPE_GENERAL:
    start_ability = START_GENERAL_ABILITIES;
    end_ability = END_GENERAL_ABILITIES + 1;
    break;
  case ABILITY_TYPE_CRAFT:
    /* as of 10/30/2014 we decided to make crafting indepdent of the skill/ability system */
    send_to_char(ch, "\tRNOTE:\tn Type '\tYcraft\tn' to see your crafting skills, "
                     "skills/abilities will no longer affect your crafting abilities.\r\n");
    start_ability = START_CRAFT_ABILITIES;
    end_ability = END_CRAFT_ABILITIES + 1;
    break;
  case ABILITY_TYPE_KNOWLEDGE:
    start_ability = START_KNOWLEDGE_ABILITIES;
    end_ability = END_KNOWLEDGE_ABILITIES + 1;
    break;
  default:
    log("SYSERR: list_abilities called with invalid ability_type: %d", ability_type);
    start_ability = 1;
    end_ability = NUM_ABILITIES;
  }

  // if (IS_NPC(ch))
  // return;

  send_to_char(ch, "*Name of skill, invested points, total points with all active bonuses\tn\r\n"
                   "\tcSkill              Inve Tota Class/Cross/Unavailable  \tMUnspent trains: \tm%d\tn\r\n",
               GET_TRAINS(ch));

  for (i = start_ability; i < end_ability; i++)
  {
    /* we have some unused defines right now, we are going to skip over
       them manaully */
    switch (i)
    {
    case ABILITY_UNUSED_1:
    case ABILITY_UNUSED_2:
    case ABILITY_UNUSED_3:
    case ABILITY_UNUSED_4:
    case ABILITY_UNUSED_5:
    case ABILITY_UNUSED_6:
    case ABILITY_UNUSED_7:
    case ABILITY_UNUSED_8:
      continue;
    default:
      break;
    }
    send_to_char(ch, "%-18s [%2d] \tC[%2d]\tn %s\r\n",
                 ability_names[i], GET_ABILITY(ch, i), compute_ability(ch, i),
                 cross_names[modify_class_ability(ch, i, GET_CLASS(ch))]);
  }
}

// further expansion -zusuk

void process_skill(struct char_data *ch, int skillnum)
{
  switch (skillnum)
  {
    // epic spells

    /* Epic spells we need a way to learn them that is NOT based in the trainer.
     * Questing comes to mind. */
  case SKILL_MUMMY_DUST:
    send_to_char(ch, "\tMYou gained Epic Spell:  Mummy Dust!\tn\r\n");
    SET_SKILL(ch, SPELL_MUMMY_DUST, 99);
    return;
  case SKILL_DRAGON_KNIGHT:
    send_to_char(ch, "\tMYou gained Epic Spell:  Dragon Knight!\tn\r\n");
    SET_SKILL(ch, SPELL_DRAGON_KNIGHT, 99);
    return;
  case SKILL_GREATER_RUIN:
    send_to_char(ch, "\tMYou gained Epic Spell:  Greater Ruin!\tn\r\n");
    SET_SKILL(ch, SPELL_GREATER_RUIN, 99);
    return;
  case SKILL_HELLBALL:
    send_to_char(ch, "\tMYou gained Epic Spell:  Hellball!\tn\r\n");
    SET_SKILL(ch, SPELL_HELLBALL, 99);
    return;
  case SKILL_EPIC_MAGE_ARMOR:
    send_to_char(ch, "\tMYou gained Epic Spell:  Epic Mage Armor!\tn\r\n");
    SET_SKILL(ch, SPELL_EPIC_MAGE_ARMOR, 99);
    return;
  case SKILL_EPIC_WARDING:
    send_to_char(ch, "\tMYou gained Epic Spell:  Epic Warding!\tn\r\n");
    SET_SKILL(ch, SPELL_EPIC_WARDING, 99);
    return;

  default:
    return;
  }
  return;
}

/********************************************************************/
/******************** Mobile Procs    *******************************/
/********************************************************************/

/*************************************************/
/**** General special procedures for mobiles. ****/
/*************************************************/

/* Player owned shops - Original created by Jamdog - 22nd February 2007
 * Zusuk had to completely re-write it for usage in LuminariMUD and to add
 *   the requested functionality from staff/players
 * Mob Special Function: the mob must be in the ATRIUM of the house
 * Items to be sold will be everything on the ground of the house */

/* do we have a valid player-shop item?  currently checking:
     1) shopper can see the item [unless have holy light]
     2) the item is not hidden (desc including a period .) [unless have holy light]
     3) item type is not ITEM_MONEY
 */

/* debug, comment out to disable */ //#define PLAYER_SHOP_DEBUG

bool valid_player_shop_item(struct char_data *ch, struct obj_data *obj)
{

  if (!obj)
    return FALSE;

  if (!CAN_SEE_OBJ(ch, obj))
    return FALSE;

  if (!*obj->description)
    return FALSE;

  if (*obj->description == '.')
    return FALSE;

  if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
    return FALSE;

  /* made it! */
  return TRUE;
}

/* given shopper (ch), 'private room' and hopefully an argument
   return object based on argument
   will accept an index value that corresponds to the order of the items
     in the shop storage room (for argument) */
struct obj_data *find_player_shop_obj(struct char_data *ch, char *argument,
                                      room_rnum private_room)
{
  bool is_number = FALSE;
  int index = 0, num = 1 /*starting index*/;
  struct obj_data *obj = NULL;

  skip_spaces(&argument);

  /* we need to identify if the shopper used a number (reference) to buy -Zusuk */
  if (isdigit(*argument))
  {
    is_number = TRUE;
    index = atoi(argument);
  }

  if (is_number)
  {

#ifdef PLAYER_SHOP_DEBUG
    send_to_char(ch, "player_shops: %s looking for item index (%d) in room %d\r\n", GET_NAME(ch), index,
                 world[private_room].number);
#endif

    for (obj = world[private_room].contents; obj; obj = obj->next_content)
    {
      if (valid_player_shop_item(ch, obj))
      {
        if (num == index) /* found the item obj */
          break;
        num++;
      }
    }

    if (num != index) /* reached end of list without finding index */
      obj = NULL;
  }
  else
  { /* ARGUMENT */

#ifdef PLAYER_SHOP_DEBUG
    send_to_char(ch, "player_shops: %s looking for %s in room %d\r\n", GET_NAME(ch), argument,
                 world[private_room].number);
#endif

    obj = get_obj_in_list_vis(ch, argument, NULL, world[private_room].contents);

    if (!valid_player_shop_item(ch, obj))
      obj = NULL;
  }

  return obj;
}

SPECIAL(player_owned_shops)
{
  room_rnum private_room;
  room_vnum house_vnum;
  struct obj_data *i, *j;
  int num = 1, hse;
  char *temp, shop_owner[32], buf[MAX_STRING_LENGTH];
  bool found = FALSE;

  if (!cmd)
    return FALSE;

#ifdef PLAYER_SHOP_DEBUG
  send_to_char(ch, "IN_ROOM(ch): %d\r\n", IN_ROOM(ch));
#endif

  /* Grab the name of the shop owner */
  for (hse = 0; hse < num_of_houses; hse++)
  {
#ifdef PLAYER_SHOP_DEBUG
    send_to_char(ch, "House counter: %d, This-atrium: %d\r\n", hse, house_control[hse].atrium);
#endif
    if (real_room(house_control[hse].atrium) == IN_ROOM(ch))
    {
      /* Avoid seeing <UNDEF> entries from self-deleted people. */
      if ((temp = get_name_by_id(house_control[hse].owner)) == NULL)
      {
        snprintf(shop_owner, sizeof(shop_owner), "Someone");
      }
      else
      {
        snprintf(shop_owner, sizeof(shop_owner), "%s", CAP(get_name_by_id(house_control[hse].owner)));
      }
      found = TRUE;
      break;
    }
  }

  if (found == FALSE)
    snprintf(shop_owner, sizeof(shop_owner), "Invalid Shop - Tell an Imp");

  private_room = real_room(house_control[hse].vnum);
  house_vnum = house_control[hse].vnum;

#ifdef PLAYER_SHOP_DEBUG
  send_to_char(ch, "House VNum %d\r\n", house_control[hse].vnum);
#endif

  /** LIST COMMAND **/

  if (CMD_IS("list"))
  {

    if (IS_NPC(ch))
    {
      send_to_char(ch, "Mobiles can't buy from a player-owned shop!\r\n");
      return TRUE;
    }

    snprintf(buf, sizeof(buf), "Owner: \tW%s\tn", shop_owner);
    send_to_char(ch, "Player-owned Shop %*s\r\n", count_color_chars(buf) + 55, buf);
    send_to_char(ch,
                 "###    Item                                                Cost\r\n");
    send_to_char(ch,
                 "--------------------------------------------------------------------------------\r\n");

    for (i = world[private_room].contents; i; i = i->next_content)
    {
      if (valid_player_shop_item(ch, i))
      {
        send_to_char(ch, "%3d)   %-*s %11d\r\n", num++,
                     count_color_chars(i->short_description) + 44, i->short_description,
                     GET_OBJ_COST(i));
      }
    }

    return (TRUE);

    /** BUY COMMAND **/
  }
  else if (CMD_IS("buy"))
  {

    /* do we have an item?  accepts an index for the argument */
    i = find_player_shop_obj(ch, argument, private_room);

    if (i == NULL)
    {
      send_to_char(ch, "Can not find that item!  Try the index value in cases "
                       "were some objects have funky keywords.\r\n");
      return (TRUE);
    }

#ifdef PLAYER_SHOP_DEBUG
    send_to_char(ch, "player_shops: found %s (cost: %d)\r\n", i->short_description, GET_OBJ_COST(i));
#endif

    if (GET_GOLD(ch) < GET_OBJ_COST(i))
    {
      send_to_char(ch, "You don't have enough gold!\r\n");
      return (TRUE);
    }

    /* Just to avoid crashes, if the object has no cost, then don't
     * try to make a pile of no gold */
    if (GET_OBJ_COST(i) > 0)
    {
      /* Take gold from player */
      GET_GOLD(ch) -= GET_OBJ_COST(i);

      /* Put gold in stock-room */
      j = create_money(GET_OBJ_COST(i));
      obj_to_room(j, private_room);
    }

    /* Move item from stock-room to player's inventory */
    obj_from_room(i);
    obj_to_char(i, ch);

    /* Let everyone know what's happening */
    send_to_char(ch, "%s hands you %s, and takes your payment.\r\n",
                 CAP(GET_NAME((struct char_data *)me)), i->short_description);
    act("$n buys $p from $N.", FALSE, ch, i, (struct char_data *)me, TO_ROOM);
    send_to_char(ch, "%s thanks you for your business, 'please come again!'\r\n",
                 shop_owner);

#ifdef PLAYER_SHOP_DEBUG
    send_to_char(ch, "player_shops: item bought and paid for\r\n");
#endif

    /* we have to save here to cement the transaction, otherwise a well timed
       crash or whatnot will duplicate the item -Zusuk */
    save_char(ch, 0);
    Crash_crashsave(ch);
    House_crashsave(house_vnum);

    return (TRUE);

    /** IDENTIFY COMMAND **/
  }
  else if (CMD_IS("identify"))
  {

    /* do we have an item?  accepts an index for the argument */
    i = find_player_shop_obj(ch, argument, private_room);

    if (i == NULL)
    {
      send_to_char(ch, "Can not find that item!  Try the index value in cases "
                       "were some objects have funky keywords.\r\n");
      return (TRUE);
    }

    do_stat_object(ch, i, ITEM_STAT_MODE_IDENTIFY_SPELL);

    return (TRUE);
  }

  /* Exit! */
  return (FALSE);
}
#ifdef PLAYER_SHOP_DEBUG
#undef PLAYER_SHOP_DEBUG
#endif

static void npc_steal(struct char_data *ch, struct char_data *victim)
{
  int gold;

  if (IS_NPC(victim))
    return;
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    return;
  if (!CAN_SEE(ch, victim))
    return;

  if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0))
  {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  }
  else
  {
    /* Steal some gold coins */
    gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
    if (gold > 0)
    {
      increase_gold(ch, gold);
      decrease_gold(victim, gold);
    }
  }
}

/* this function will cause basically all the mobiles in the same zone
   to hunt someone down */
static void zone_yell(struct char_data *ch, const char *buf)
{
  struct char_data *i = NULL;
  struct char_data *vict = NULL;
  int num_targets = 0;

  for (i = character_list; i; i = i->next)
  {
    if (world[ch->in_room].zone == world[i->in_room].zone)
    {

      if (PROC_FIRED(ch) == FALSE)
      {
        send_to_char(i, buf);
      }

      if (i == ch || !IS_NPC(i))
        continue;

      if (((IS_EVIL(ch) && IS_EVIL(i)) || (IS_GOOD(ch) && IS_GOOD(i))) &&
          MOB_FLAGGED(i, MOB_HELPER))
      {
        if (i->in_room == ch->in_room && !FIGHTING(i))
        {
          for (vict = world[i->in_room].people; vict; vict = vict->next_in_room)
            if (FIGHTING(vict) == ch)
            {
              act("$n jumps to the aid of $N!", FALSE, i, 0, ch, TO_ROOM);
              hit(i, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
              break;
            }
        }
        else
        {
          /* retrieve random valid target and number of targets */
          if (!(vict = npc_find_target(ch, &num_targets)))
          {
            /* currently nothing to process if we can't find a target */
          }
          else
          {
            HUNTING(i) = vict;
            hunt_victim(i);
          }
        }
      }
    }
  }
  PROC_FIRED(ch) = TRUE;
}

/* another hl port, checks if object with given vnum is being worn */
bool is_wearing(struct char_data *ch, obj_vnum vnum)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
  {
    if (GET_EQ(ch, i))
      if (GET_OBJ_VNUM(GET_EQ(ch, i)) == vnum)
        return TRUE;
  }
  return FALSE;
}

/* from homeland */
bool yan_yell(struct char_data *ch)
{
  struct char_data *i;
  struct char_data *vict;
  struct descriptor_data *d;
  int room = 0;
  int zone = world[ch->in_room].zone;
  room_rnum start = 0;
  room_rnum end = 0;
  vict = FIGHTING(ch);

  if (!vict)
    return FALSE;

  // show yan-s yell message.
  if (PROC_FIRED(ch) == false)
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) == CON_PLAYING && d->character != NULL &&
          zone == world[d->character->in_room].zone)
      {
        send_to_char(d->character, "\tcYan-C-Bin the Master of Evil Air\tw shouts, '\tcI "
                                   "have been attacked! Come to me minions!\tw'\tn\r\n");
      }
    }
  }

  start = real_room(136100);
  end = real_room(136224);

  for (room = start; room <= end; room++)
  {
    for (i = world[room].people; i; i = i->next_in_room)
    {
      if (IS_NPC(i) && !FIGHTING(i))
      {
        switch (GET_MOB_VNUM(i))
        {
        case 136110:
        case 136111:
        case 136112:
        case 136113:
          if (i->in_room == ch->in_room)
          {
            act("$n jumps to the aid of $N!", FALSE, i, 0, ch, TO_ROOM);
            hit(i, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
          }
          else
          {
            HUNTING(i) = ch;
            hunt_victim(i);
          }
          break;
        }
      }
    }
  }

  if (PROC_FIRED(ch) == FALSE)
  {
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
void yan_maelstrom(struct char_data *ch)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int dam = 0;

  act("$N \tcbegins to spin in a circular motion, gathering speed at an alarming pace.\tn\r\n"
      "\tcAs the pace quickens, $E begins to gain in height as well, until $E forms into\tn\r\n"
      "\tcan eighty-foot tall whirlwind \tcof \tCs\tcw\twi\tcr\tCl\tci\twn\tCg \tcchaos.\tn",
      FALSE, ch, 0, ch, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;
    if (IS_NPC(vict) && !IS_PET(vict))
      continue;
    dam = 150 + dice(10, 20);
    if (GET_LEVEL(vict) < 20)
      dam = GET_MAX_HIT(vict);
    if (dam >= GET_HIT(vict))
    {
      dam += 25;
      act("\twAs you are spun about by $n's \twmaelstrom, your body is damaged beyond repair.\tn",
          FALSE, ch, 0, vict, TO_VICT);
      act("\twAs $N is spun about by $n's \twmaelstrom, $M body is damaged beyond repair.\tn",
          FALSE, ch, 0, vict, TO_NOTVICT);
    }
    else
    {
      act("\twYou are enveloped in $n's \tCs\tcw\twi\tcr\tCl\tci\twn\tCg \tcmaelstrom\tw, your body pelted by \twgusts\tc of wind.\tn", FALSE, ch, 0, vict, TO_VICT);
      act("\tw$N is enveloped in $n's \tCs\tcw\twi\tcr\tCl\tci\twn\tCg \tcmaelstrom\tw, $S body pelted by \twgusts\tc of wind.\tn",
          FALSE, ch, 0, vict, TO_NOTVICT);
    }
    damage(ch, vict, dam, -1, DAM_AIR, FALSE); // type -1 = no dam msg
  }
}

/* from homeland */
void yan_windgust(struct char_data *ch)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int dam = 0;
  struct affected_type af;

  act("\tc$n\tc opens $s cavernous maw and sends forth a \tCpowerful \twgust\tc of air.\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;
    if (IS_NPC(vict) && !IS_PET(vict))
      continue;

    dam = 30 + dice(3, 30);
    if (dam > GET_HIT(vict))
    {
      dam += 25;

      act("\twAs you are hit by the \tcgust\tw of wind sent by $n, \twyou feel your\r\n"
          "\twlife slip away.\tn",
          FALSE, ch, 0, vict, TO_VICT);
      act("\tw$N is blasted by $n's \tcgust\tw of wind, and suddenly keels over from\r\n"
          "\twthe damage.\tn",
          FALSE, ch, 0, vict, TO_NOTVICT);
      damage(ch, vict, dam, -1, DAM_AIR, FALSE); // type -1 = no dam msg
    }
    else
    {
      act("\twYou are blasted by a \tCf\tci\twer\tcc\tCe\tc gust\tw of wind hurled by $n.\tn",
          FALSE, ch, 0, vict, TO_VICT);
      act("\tw$N is blasted by a \tCf\tci\twer\tcc\tCe\tc gust\tw of wind hurled by $n.\tn",
          FALSE, ch, 0, vict, TO_NOTVICT);
      damage(ch, vict, dam, -1, DAM_AIR, FALSE); //-1 type = no dam mess
      if (dice(1, 40) > GET_CON(vict) && can_stun(vict))
      {
        new_affect(&af);
        af.spell = SKILL_CHARGE;
        SET_BIT_AR(af.bitvector, AFF_STUN);
        af.duration = dice(2, 4) + 1;
        affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
      }
    }
  }
}

/* from homeland */
bool chan_yell(struct char_data *ch)
{
  struct char_data *i;
  struct char_data *vict;
  struct descriptor_data *d;
  room_rnum room = 0;
  int zone = world[ch->in_room].zone;
  room_rnum start = 0;
  room_rnum end = 0;

  vict = FIGHTING(ch);
  if (!vict)
    return FALSE;

  // show yan-s yell message.
  if (PROC_FIRED(ch) == false)
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) == CON_PLAYING && d->character != NULL && zone == world[d->character->in_room].zone)
      {
        send_to_char(d->character,
                     "\tCChan, the Elemental Princess of Good Air\tw shouts, '\tcI have been attacked! Come to me my friends!\tw'\tn\r\n");
      }
    }
  }

  start = real_room(136100);
  end = real_room(136224);

  for (room = start; room <= end; room++)
  {
    for (i = world[room].people; i; i = i->next_in_room)
    {
      if (IS_NPC(i) && !FIGHTING(i))
      {
        switch (GET_MOB_VNUM(i))
        {
        case 136115:
        case 136116:
        case 136117:
        case 136118:
          if (i->in_room == ch->in_room)
          {
            act("$n jumps to the aid of $N!", FALSE, i, 0, ch, TO_ROOM);
            hit(i, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
          }
          else
          {
            HUNTING(i) = ch;
            hunt_victim(i);
          }
          break;
        }
      }
    }
  }

  if (PROC_FIRED(ch) == FALSE)
  {
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }

  return FALSE;
}

/****************************************************/
/******* end general procedures for mobile procs ****/
/****************************************************/

/****************************/
/** begin actual mob procs **/
/****************************/

/* from homeland */
SPECIAL(shadowdragon)
{
  struct char_data *vict;
  struct char_data *next_vict;

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
    return FALSE;

  if (rand_number(0, 4))
    return FALSE;

  act("$n \tLopens her mouth and let stream forth a black breath of de\tws\tWp\twa\tLir.\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;
    if (IS_NPC(vict) && !IS_PET(vict))
      continue;

    act("\tLDarkness envelopes you and you feel the hopelessness of fighting against this all powerful foe.\tn",
        FALSE, ch, 0, vict, TO_VICT);
    act("$N \tLseems to loose the will for fighting against this awesome foe.\tn",
        FALSE, ch, 0, vict, TO_NOTVICT);
    GET_MOVE(vict) -= (10 + dice(5, 4));
  }

  call_magic(ch, FIGHTING(ch), 0, SPELL_DARKNESS, 0, GET_LEVEL(ch), CAST_SPELL);

  return TRUE;
}

/* from homeland */
SPECIAL(imix)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch))
  {
    zone_yell(ch, "\r\n\tMImix \tnshouts, '\tRYou DARE attack me?!? Minions... to me now!!!\tn'\r\n");
  }

  if (!rand_number(0, 3) && FIGHTING(ch))
  {
    call_magic(ch, FIGHTING(ch), 0, SPELL_FIRE_BREATHE, 0, GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(olhydra)
{
  struct char_data *vict;
  struct char_data *next_vict;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch))
  {
    zone_yell(ch, "\r\n\tBOlhydra \tnshouts, '\tCYou DARE attack me?!? Minions... to me now!!!\tn'\r\n");
  }

  if (!rand_number(0, 3) && FIGHTING(ch))
  {
    act("$n \tLopens $s mouth and let stream forth a \tBwave of water.\tn",
        FALSE, ch, 0, 0, TO_ROOM);

    for (vict = world[ch->in_room].people; vict; vict = next_vict)
    {
      next_vict = vict->next_in_room;
      if (IS_NPC(vict) && !IS_PET(vict))
        continue;
      if (ch == vict)
        continue;

      if ((dice(1, 20) + 21) < GET_DEX(vict))
      {
        act("\tbThe wave hits \tCYOU\tb, knocking you backwards.\tn",
            FALSE, ch, 0, vict, TO_VICT);
        act("\tbThe wave hits $N\tb, knocking $M backwards.\tn",
            FALSE, ch, 0, vict, TO_NOTVICT);
        USE_MOVE_ACTION(vict);
      }
      else
      {
        act("\tbThe wave hits \tCYOU\tb with full \tBforce\tb, knocking you down.\tn",
            FALSE, ch, 0, vict, TO_VICT);
        act("\tbThe wave hits $N\tb with full \tBforce\tb, knocking $M down.\tn",
            FALSE, ch, 0, vict, TO_NOTVICT);
        change_position(vict, POS_SITTING);
        USE_FULL_ROUND_ACTION(ch);
      }
    }
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(banshee)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch)) // heheh  && GET_HIT(ch) == GET_MAX_HIT(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !rand_number(0, 40) && PROC_FIRED(ch) != TRUE)
  {
    act("\tW$n \tWlets out a piercing shriek so horrible that it makes your ears \trBLEED\tW!\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      if (!IS_NPC(vict) && !mag_savingthrow(ch, vict, SAVING_WILL, -4, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      {
        act("\tRThe brutal scream tears away at your life force,\r\n"
            "causing you to fall to your knees with pain!\tn",
            FALSE, vict, 0, 0, TO_CHAR);
        act("$n grabs $s ears and tumbles to the ground in pain!", FALSE, vict, 0, 0, TO_ROOM);
        GET_HIT(vict) = 1;
      }

    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(quicksand)
{
  struct affected_type af;

  if (cmd)
    return FALSE;

  if (IS_NPC(ch) && !IS_PET(ch))
    return FALSE;

  if (is_flying(ch))
    return FALSE;
  if (GET_LEVEL(ch) > LVL_IMMORT)
    return FALSE;

  if (GET_DEX(ch) > dice(1, 20) + 12)
  {
    act("\tyYou avoid getting stuck in the quicksand.\tn",
        FALSE, ch, 0, 0, TO_CHAR);
    act("\tn$n\ty avoids getting stuck in the quicksand.\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    return FALSE;
  }

  act("\tyThe marsh \tgla\tynd of the \twm\tye\tgr\tye opens up suddenly revealing quicksand!\tn\r\n"
      "\tnYou get sucked down.\tn",
      FALSE, ch, 0, 0, TO_CHAR);
  act("\tn$n\ty gets stuck in the quicksand of the marsh \tgla\tynd of the \twm\tye\tgr\tye.\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  new_affect(&af);
  af.spell = SPELL_HOLD_PERSON;
  SET_BIT_AR(af.bitvector, AFF_PARALYZED);
  af.duration = 5;
  affect_join(ch, &af, TRUE, FALSE, FALSE, FALSE);

  return TRUE;
}

/* from homeland */
SPECIAL(kt_kenjin)
{
  struct affected_type af;
  struct char_data *vict = 0;
  struct char_data *tch = 0;
  int val = 0;

  if (cmd)
    return FALSE;
  if (!FIGHTING(ch))
    return FALSE;

  if (GET_POS(ch) < POS_FIGHTING)
    return FALSE;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch) || IS_PET(tch))
    {
      if (!vict || !rand_number(0, 2))
      {
        vict = tch;
      }
    }
  }

  val = dice(1, 3);

  // turns you into stone
  if (val == 1)
  {
    act("$n\tc whips out a \tWbone-white\tn wand, and waves it in the air.\tn\r\n"
        "\tcSuddenly $e points it at \tn$N\tc, who slowly turns to stone.",
        FALSE,
        ch, 0, vict, TO_ROOM);
    new_affect(&af);
    af.spell = SPELL_HOLD_PERSON;
    SET_BIT_AR(af.bitvector, AFF_PARALYZED);
    af.duration = rand_number(2, 3);
    affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
    return TRUE;
  }

  // teleports you to the bottom of the shaft.
  if (val == 2)
  {
    act("$n\tc whips out a \trfiery red\tn wand, and waves it in the air.\tn\r\n"
        "\tcSuddenly $e points it at \tn$N\tc, who fades away suddenly.",
        FALSE,
        ch, 0, vict, TO_ROOM);
    char_from_room(vict);
    char_to_room(vict, real_room(132908));
    look_at_room(vict, 0);
    return TRUE;
  }

  // loads a new mob in 132919 :)
  if (val == 3)
  {
    act("$n\tc whips out a \tYgolden\tn wand, and waves it in the air.\tn\r\n"
        "\tcSuddenly $e taps in the air, and you see \tLshadow\tc coalesce behind you.",
        FALSE,
        ch, 0, vict, TO_ROOM);
    tch = read_mobile(132902, VIRTUAL);
    if (!tch)
      return FALSE;
    char_to_room(tch, real_room(132919));
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(kt_twister)
{
  struct char_data *mob;
  char l_name[256];
  char s_name[256];
  int temp;

  if (cmd)
    return FALSE;

  if (IS_NPC(ch) && !IS_PET(ch))
    return FALSE;

  temp = world[real_room(132901)].dir_option[0]->to_room;
  world[real_room(32901)].dir_option[0]->to_room =

      world[real_room(132901)].dir_option[1]->to_room;
  world[real_room(32901)].dir_option[1]->to_room =

      world[real_room(132901)].dir_option[2]->to_room;
  world[real_room(32901)].dir_option[2]->to_room =

      world[real_room(132901)].dir_option[3]->to_room;
  world[real_room(32901)].dir_option[3]->to_room = temp;

  send_to_room(real_room(132901), "\tCThe world seems to turn.\tn\r\n");

  mob = read_mobile(132901, VIRTUAL);

  if (!mob)
    return FALSE;

  char_to_room(mob, real_room(132906));

  snprintf(l_name, sizeof(l_name), "\tLThe shadow of \tw%s\tL stands here.\tn  ", GET_NAME(ch));
  snprintf(s_name, sizeof(s_name), "\tLa shadow of \tw%s\tn", GET_NAME(ch));

  mob->player.short_descr = strdup(s_name);
  mob->player.name = strdup("shadow");
  mob->player.long_descr = strdup(l_name);

  GET_LEVEL(mob) = GET_LEVEL(ch);
  GET_MAX_HIT(mob) = 1000 + GET_LEVEL(mob) * 50;
  GET_HIT(mob) = GET_MAX_HIT(mob);
  GET_CLASS(mob) = GET_CLASS(ch);

  send_to_char(ch, "You somehow feel \tWsplit\tn in half.\r\n");
  return TRUE;
}

/* from homeland */
SPECIAL(hive_death)
{
  if (cmd)
    return FALSE;
  if (!ch)
    return FALSE;

  send_to_char(ch,
               "\trAs you enter through the curtain, your body is ripped into two pieces, as your link\tn\r\n"
               "\trthrough the ethereal plane is severed.  You suddenly realise that your physical body\tn\r\n"
               "\tris at one place, and your mind in another part.\tn\r\n\r\n");
  char_from_room(ch);
  char_to_room(ch, real_room(129500));
  // make_corpse(ch, 0);
  send_to_char(ch, "\tWYou feel the link snap completely, leaving you body behind completely!\tn\r\n\r\n");
  look_at_room(ch, 0);
  char_from_room(ch);
  send_to_char(ch, "\tLYou focus your eyes back on the present.\tn\r\n\r\n");
  char_to_room(ch, real_room(139328));
  look_at_room(ch, 0);

  return TRUE;
}

/* from homeland */
SPECIAL(feybranche)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH];

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  struct char_data *enemy = FIGHTING(ch);

  if (!enemy)
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;
    snprintf(buf, sizeof(buf), "%s\tL shouts, '\tmCome to me!!' Fey-Branche is under attack!\tn\r\n",
             ch->player.short_descr);
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) && (GET_MOB_VNUM(i) == 135535 || GET_MOB_VNUM(i) == 135536 || GET_MOB_VNUM(i) == 135537 || GET_MOB_VNUM(i) == 135538 || GET_MOB_VNUM(i) == 135539 || GET_MOB_VNUM(i) == 135540) && ch != i)
      {
        if (FIGHTING(ch)->in_room != i->in_room)
        {
          if (GET_MOB_VNUM(i) != 135536)
          {
            HUNTING(i) = enemy;
            hunt_victim(i);
          }
          else
            cast_spell(i, enemy, 0, SPELL_TELEPORT, 0);
        }
        else
          hit(i, enemy, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      if (world[ch->in_room].zone == world[i->in_room].zone && !PROC_FIRED(ch))
        send_to_char(i, buf);
    }
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(abyssal_vortex)
{
  int temp;

  if (cmd)
    return FALSE;

  if (IS_NPC(ch) && !IS_PET(ch))
    return FALSE;

  if (!rand_number(0, 7))
  {
    temp = world[ch->in_room].dir_option[0]->to_room;
    world[ch->in_room].dir_option[0]->to_room = world[ch->in_room].dir_option[1]->to_room;
    world[ch->in_room].dir_option[1]->to_room = world[ch->in_room].dir_option[4]->to_room;
    world[ch->in_room].dir_option[4]->to_room = world[ch->in_room].dir_option[3]->to_room;
    world[ch->in_room].dir_option[3]->to_room = world[ch->in_room].dir_option[5]->to_room;
    world[ch->in_room].dir_option[5]->to_room = world[ch->in_room].dir_option[2]->to_room;
    world[ch->in_room].dir_option[2]->to_room = temp;

    send_to_room(ch->in_room, "\tLThe reality seems to \tCshift\tL as madness descends in the \tcvortex\tn\r\n");

    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(agrachdyrr)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH];

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  struct char_data *enemy = FIGHTING(ch);

  if (!enemy)
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {

    if (!rand_number(0, 4) && !ch->followers)
    {
      act("$n\tL looks to be extremely disspleased at being\r\n"
          "forced to fight such inferior beings in her own mansion. She raises her\r\n"
          "arms and cries out, '\tmAid me Lloth!\tL'",
          FALSE, ch, 0, 0, TO_ROOM);

      struct char_data *mob = read_mobile(135523, VIRTUAL);
      if (!mob)
        return FALSE;
      char_to_room(mob, ch->in_room);
      add_follower(mob, ch);
      return TRUE;
    }

    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;

    snprintf(buf, sizeof(buf), "%s\tL shouts, '\twTo me, \tcAgrach-Dyrr\tw is under attack!'\tn\r\n",
             ch->player.short_descr);
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) && (GET_MOB_VNUM(i) == 135521 || GET_MOB_VNUM(i) == 135522 || GET_MOB_VNUM(i) == 135510 || GET_MOB_VNUM(i) == 135524 || GET_MOB_VNUM(i) == 135525 || GET_MOB_VNUM(i) == 135512) && ch != i)
      {
        if (FIGHTING(ch)->in_room != i->in_room)
        {
          if (GET_MOB_VNUM(i) != 135522)
          {
            HUNTING(i) = enemy;
            hunt_victim(i);
          }
          else
            cast_spell(i, enemy, 0, SPELL_TELEPORT, 0);
        }
        else
          hit(i, enemy, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      if (world[ch->in_room].zone == world[i->in_room].zone && !PROC_FIRED(ch))
        send_to_char(i, buf);
    }
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  } // for loop
  return FALSE;
}

/* from homeland */
SPECIAL(shobalar)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH];

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  struct char_data *enemy = FIGHTING(ch);

  if (!enemy)
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (enemy->master && enemy->master->in_room == enemy->in_room)
      enemy = enemy->master;
    snprintf(buf, sizeof(buf), "%s\tL shouts, '\twTo me, \tmShobalar\tw is under attack!'\tn\r\n",
             ch->player.short_descr);
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) && (GET_MOB_VNUM(i) == 135506 || GET_MOB_VNUM(i) == 135500 || GET_MOB_VNUM(i) == 135504 || GET_MOB_VNUM(i) == 135507) && ch != i)
      {
        if (FIGHTING(ch)->in_room != i->in_room)
        {
          if (GET_MOB_VNUM(i) != 135506)
          {
            HUNTING(i) = enemy;
            hunt_victim(i);
          }
          else
            cast_spell(i, enemy, NULL, SPELL_TELEPORT, 0);
        }
        else
          hit(i, enemy, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      if (world[ch->in_room].zone == world[i->in_room].zone && !PROC_FIRED(ch))
        send_to_char(i, buf);
    }
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  } // for loop

  return FALSE;
}

/* from homeland */
SPECIAL(ogremoch)
{
  struct char_data *i;
  struct char_data *vict;
  struct descriptor_data *d;
  room_rnum room = 0;
  int zone = world[ch->in_room].zone;
  room_rnum start = 0;
  room_rnum end = 0;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  vict = FIGHTING(ch);
  if (!vict)
    return FALSE;

  // show yell message.
  if (PROC_FIRED(ch) == false)
  {
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) == CON_PLAYING && d->character != NULL && zone == world[d->character->in_room].zone)
      {
        send_to_char(d->character, "\tLOgremoch \tw shouts, '\tLI have been "
                                   "attacked! Come to me minions!\tw'\tn\r\n");
      }
    }
  }

  start = real_room(136700);
  end = real_room(136802);

  for (room = start; room <= end; room++)
  {
    for (i = world[room].people; i; i = i->next_in_room)
    {
      if (IS_NPC(i) && !FIGHTING(i))
      {
        switch (GET_MOB_VNUM(i))
        {
        case 136703:
        case 136704:
        case 136705:
        case 136706:
        case 136707:
        case 136708:
        case 136709:
          if (i->in_room == ch->in_room)
          {
            act("$n jumps to the aid of $N!", FALSE, i, 0, ch, TO_ROOM);
            hit(i, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
          }
          else
          {
            // either melt in directly or track
            if (dice(1, 10) < 2)
            {
              act("$n jumps into the pure rock, as $s lord calls for $m.",
                  FALSE, i, 0, 0, TO_ROOM);
              char_from_room(i);
              char_to_room(i, ch->in_room);
              act("$n comes out from the rock, to help $s lord.", FALSE, i,
                  0, 0, TO_ROOM);
            }
            else
            {
              HUNTING(i) = ch;
              hunt_victim(i);
            }
          }
          break;
        }
      }
    }
  }
  PROC_FIRED(ch) = TRUE;

  return TRUE;
}

/* from homeland */
SPECIAL(yan)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch))
  {
    if (yan_yell(ch))
      return TRUE;
    if (!rand_number(0, 50))
    {
      yan_maelstrom(ch);
      return TRUE;
    }
    if (!rand_number(0, 3))
    {
      yan_windgust(ch);
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(chan)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch))
  {
    if (chan_yell(ch))
      return TRUE;
    if (!rand_number(0, 3))
    {
      yan_windgust(ch);
      return TRUE;
    }
  }
  return FALSE;
}

SPECIAL(guild)
{
  int skill_num, percent;
  char arg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  char *ability_name = NULL;

  if (IS_NPC(ch) || (!CMD_IS("practice") && !CMD_IS("train") && !CMD_IS("boosts")))
    return (FALSE);

  skip_spaces(&argument);

  // Practice code
  if (CMD_IS("practice"))
  {

    list_crafting_skills(ch);
    return (TRUE);

    /***************************************/
    /* everything below this is deprecated */
    /***************************************/

    if (!*argument)
    {
      list_skills(ch);
      return (TRUE);
    }
    if (GET_PRACTICES(ch) <= 0)
    {
      send_to_char(ch, "You do not seem to be able to practice now.\r\n");
      return (TRUE);
    }

    skill_num = find_skill_num(argument);

    if (skill_num < 1 ||
        GET_LEVEL(ch) < spell_info[skill_num].min_level[(int)GET_CLASS(ch)])
    {
      send_to_char(ch, "You do not know of that skill.\r\n");
      return (TRUE);
    }

    /*
    if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
      send_to_char(ch, "You are already learned in that area.\r\n");
      return (TRUE);
    }
     */

    if (skill_num > SPELL_RESERVED_DBC && skill_num < MAX_SPELLS)
    {
      send_to_char(ch, "You can't practice spells.\r\n");
      return (TRUE);
    }

    if (!meet_skill_reqs(ch, skill_num))
    {
      send_to_char(ch, "You haven't met the pre-requisites for that skill.\r\n");
      return (TRUE);
    }

    /* added with addition of crafting system so you can't use your
     'practice points' for training your crafting skills which have
     a much lower base value than 75 */

    if (GET_SKILL(ch, skill_num))
    {
      send_to_char(ch, "You already have this skill trained.\r\n");
      return TRUE;
    }

    send_to_char(ch, "You practice '%s' with your trainer...\r\n",
                 spell_info[skill_num].name);
    GET_PRACTICES(ch)
    --;

    percent = GET_SKILL(ch, skill_num);
    percent += int_app[GET_INT(ch)].learn;

    SET_SKILL(ch, skill_num, percent);

    /*
    if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
      send_to_char(ch, "You are now \tGlearned\tn in '%s.'\r\n",
            spell_info[skill_num].name);
     */

    // for further expansion - zusuk
    process_skill(ch, skill_num);

    return (TRUE);
  }
  else if (CMD_IS("train"))
  {
    // training code

    if (!*argument)
    {
      list_abilities(ch, ABILITY_TYPE_GENERAL);
      return (TRUE);
    }

    if (GET_TRAINS(ch) <= 0)
    {
      send_to_char(ch, "You do not seem to be able to train now.\r\n");
      return (TRUE);
    }

    /* Parse argument and check for 'knowledge' or 'craft' as a first arg- */
    ability_name = one_argument_u(argument, arg);
    skip_spaces(&ability_name);

    if (is_abbrev(arg, "craft"))
    {

      if (!strcmp(ability_name, ""))
      {
        list_abilities(ch, ABILITY_TYPE_CRAFT);
        return (TRUE);
      }
      /* Crafting skill */
      snprintf(buf, sizeof(buf), "Craft (%s", ability_name);
      skill_num = find_ability_num(buf);
    }
    else if (is_abbrev(arg, "knowledge"))
    {

      if (!strcmp(ability_name, ""))
      {
        list_abilities(ch, ABILITY_TYPE_KNOWLEDGE);
        return (TRUE);
      }
      /* Knowledge skill */
      snprintf(buf, sizeof(buf), "Knowledge (%s", ability_name);
      skill_num = find_ability_num(buf);
    }
    else
    {
      skill_num = find_ability_num(argument);
    }

    if (skill_num < 1)
    {
      send_to_char(ch, "You do not know of that ability.\r\n");
      return (TRUE);
    }

    // ability not available to this class
    if (modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 0)
    {
      send_to_char(ch, "This ability is not available to your class...\r\n");
      return (TRUE);
    }

    // cross-class ability
    if (GET_TRAINS(ch) < 2 && modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      send_to_char(ch, "(Cross-Class) You don't have enough training sessions to train that ability...\r\n");
      return (TRUE);
    }
    if (GET_ABILITY(ch, skill_num) >= ((int)((GET_LEVEL(ch) + 3) / 2)) && modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      send_to_char(ch, "You are already trained in that area.\r\n");
      return (TRUE);
    }

    // class ability
    if (GET_ABILITY(ch, skill_num) >= (GET_LEVEL(ch) + 3) && modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 2)
    {
      send_to_char(ch, "You are already trained in that area.\r\n");
      return (TRUE);
    }

    send_to_char(ch, "You train for a while...\r\n");
    GET_TRAINS(ch)
    --;
    if (modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      GET_TRAINS(ch)
      --;
      send_to_char(ch, "You used two training sessions to train a cross-class ability...\r\n");
    }
    GET_ABILITY(ch, skill_num)
    ++;

    if (GET_ABILITY(ch, skill_num) >= (GET_LEVEL(ch) + 3))
      send_to_char(ch, "You are now trained in that area.\r\n");
    if (GET_ABILITY(ch, skill_num) >= ((int)((GET_LEVEL(ch) + 3) / 2)) && CLSLIST_ABIL(GET_CLASS(ch), skill_num) == 1)
      send_to_char(ch, "You are already trained in that area.\r\n");

    return (TRUE);
  }
  else if (CMD_IS("boosts"))
  {
    if (!argument || !*argument)
      send_to_char(ch, "\tCStat boost sessions remaining: %d\tn\r\n"
                       "\tcStats:\tn\r\n"
                       "Strength\r\n"
                       "Constitution\r\n"
                       "Dexterity\r\n"
                       "Intelligence\r\n"
                       "Wisdom\r\n"
                       "Charisma\r\n"
                       "\r\n",
                   GET_BOOSTS(ch));
    else if (!GET_BOOSTS(ch))
      send_to_char(ch, "You have no ability training sessions.\r\n");
    else if (!strncasecmp("strength", argument, strlen(argument)))
    {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour strength increases!\tn\r\n");
      GET_REAL_STR(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("constitution", argument, strlen(argument)))
    {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour constitution increases!\tn\r\n");
      GET_REAL_CON(ch) += 1;
      /* Give them retroactive hit points for constitution */
      if (!(GET_REAL_CON(ch) % 2))
      {
        GET_REAL_MAX_HIT(ch) += GET_LEVEL(ch);
        send_to_char(ch, "\tMYou gain %d hitpoints!\tn\r\n", GET_LEVEL(ch));
      }
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("dexterity", argument, strlen(argument)))
    {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour dexterity increases!\tn\r\n");
      GET_REAL_DEX(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("intelligence", argument, strlen(argument)))
    {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour intelligence increases!\tn\r\n");
      GET_REAL_INT(ch) += 1;
      GET_BOOSTS(ch) -= 1;
      /* Give them retroactive trains */
      if (!(GET_REAL_INT(ch) % 2))
      {
        GET_TRAINS(ch) += GET_LEVEL(ch);
        send_to_char(ch, "\tMYou gain %d trains!\tn\r\n", GET_LEVEL(ch));
      }
    }
    else if (!strncasecmp("wisdom", argument, strlen(argument)))
    {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour wisdom increases!\tn\r\n");
      GET_REAL_WIS(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("charisma", argument, strlen(argument)))
    {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour charisma increases!\tn\r\n");
      GET_REAL_CHA(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else
      send_to_char(ch, "\tCStat boost sessions remaining: %d\tn\r\n"
                       "\tcStats:\tn\r\n"
                       "Strength\r\n"
                       "Constitution\r\n"
                       "Dexterity\r\n"
                       "Intelligence\r\n"
                       "Wisdom\r\n"
                       "Charisma\r\n"
                       "\r\n",
                   GET_BOOSTS(ch));
    affect_total(ch);
    send_to_char(ch, "\tDType 'feats' to see your feats\tn\r\n");
    send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
    send_to_char(ch, "\tDType 'boost' to adjust your stats\tn\r\n");
    send_to_char(ch, "\tDType 'craft' to see your crafting proficiency\tn\r\n");
    send_to_char(ch, "\tDType 'spells <classname>' to see your currently known spells\tn\r\n");
    return (TRUE);
  }

  // should not be able to get here
  log("Reached the unreachable in SPECIAL(guild) in spec_procs.c");
  return (FALSE);
}

SPECIAL(mayor)
{
  char actbuf[MAX_INPUT_LENGTH];

  const char open_path[] =
      "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
  const char close_path[] =
      "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static const char *path = NULL;
  static int path_index;
  static bool move = FALSE;

  if (!move)
  {
    if (time_info.hours == 6)
    {
      move = TRUE;
      path = open_path;
      path_index = 0;
    }
    else if (time_info.hours == 20)
    {
      move = TRUE;
      path = close_path;
      path_index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) || FIGHTING(ch))
    return (FALSE);

  switch (path[path_index])
  {
  case '0':
  case '1':
  case '2':
  case '3':
    perform_move(ch, path[path_index] - '0', 1);
    break;

  case 'W':
    change_position(ch, POS_STANDING);
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    change_position(ch, POS_SLEEPING);
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'",
        FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
        FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgen closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_UNLOCK); /* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_OPEN);   /* strcpy: OK */
    break;

  case 'C':
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_CLOSE); /* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_LOCK);  /* strcpy: OK */
    break;

  case '.':
    move = FALSE;
    break;
  }

  path_index++;
  return (FALSE);
}

/* Quite lethal to low-level characters. */
SPECIAL(snake)
{
  if (cmd || !FIGHTING(ch))
    return (FALSE);

  if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || rand_number(0, GET_LEVEL(ch)) != 0)
    return (FALSE);

  act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
  act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
  call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, 0, GET_LEVEL(ch), CAST_SPELL);
  return (TRUE);
}

SPECIAL(hound)
{
  struct char_data *i;
  int door;
  room_rnum room;

  if (cmd || GET_POS(ch) != POS_STANDING || FIGHTING(ch))
    return (FALSE);

  /* first go through all the directions */
  for (door = 0; door < DIR_COUNT; door++)
  {
    if (CAN_GO(ch, door))
    {
      room = world[IN_ROOM(ch)].dir_option[door]->to_room;

      /* ok found a neighboring room, now cycle through the peeps */
      for (i = world[room].people; i; i = i->next_in_room)
      {
        /* is this guy a hostile? */
        if (i && IS_NPC(i) && MOB_FLAGGED(i, MOB_AGGRESSIVE))
        {
          act("$n howls a warning!", FALSE, ch, 0, 0, TO_ROOM);
          return (TRUE);
        }
      } // end peeps cycle
    }   // can_go
  }     // end room cycle

  return (FALSE);
}

SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd || GET_POS(ch) != POS_STANDING)
    return (FALSE);

  for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && GET_LEVEL(cons) < LVL_IMMORT && !rand_number(0, 4))
    {
      npc_steal(ch, cons);
      return (TRUE);
    }

  return (FALSE);
}

SPECIAL(wizard)
{
  struct char_data *vict;

  if (cmd || !FIGHTING(ch))
    return (FALSE);

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
    vict = FIGHTING(ch);

  /* Hm...didn't pick anyone...I'll wait a round. */
  if (vict == NULL)
    return (TRUE);

  if (GET_LEVEL(ch) > 13 && rand_number(0, 10) == 0)
    cast_spell(ch, vict, NULL, SPELL_POISON, 0);

  if (GET_LEVEL(ch) > 7 && rand_number(0, 8) == 0)
    cast_spell(ch, vict, NULL, SPELL_BLINDNESS, 0);

  if (GET_LEVEL(ch) > 12 && rand_number(0, 12) == 0)
  {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN, 0);
    else if (IS_GOOD(ch))
      cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, 0);
  }

  if (rand_number(0, 4))
    return (TRUE);

  switch (GET_LEVEL(ch))
  {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, 0);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH, 0);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS, 0);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, 0);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, 0);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY, 0);
    break;
  default:
    cast_spell(ch, vict, NULL, SPELL_FIREBALL, 0);
    break;
  }
  return (TRUE);
}

SPECIAL(wall)
{
  if (!IS_MOVE(cmd))
    return (FALSE);

  /* acceptable ways to avoid the wall */
  /* */

  /* failed to get past wall */
  send_to_char(ch, "You can't get by the magical wall!\r\n");
  act("$n fails to get past the magical wall!", FALSE, ch, 0, 0, TO_ROOM);
  return (TRUE);
}

SPECIAL(guild_guard)
{
  int i, direction;
  struct char_data *guard = (struct char_data *)me;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
    return (FALSE);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return (FALSE);

  /* find out what direction they are trying to go */
  for (direction = 0; direction < NUM_OF_DIRS; direction++)
    if (!strcmp(cmd_info[cmd].command, dirs[direction]))
      for (direction = 0; direction < DIR_COUNT; direction++)
        if (!strcmp(cmd_info[cmd].command, dirs[direction]) ||
            !strcmp(cmd_info[cmd].command, autoexits[direction]))
          break;

  for (i = 0; guild_info[i].guild_room != NOWHERE; i++)
  {
    /* Wrong guild. */
    if (GET_ROOM_VNUM(IN_ROOM(ch)) != guild_info[i].guild_room)
      continue;

    /* Wrong direction. */
    if (direction != guild_info[i].direction)
      continue;

    /* Allow the people of the guild through. */
    /* Can't use GET_CLASS anymore, need CLASS_LEVEL(ch, i)!!  - 04/08/2013 Ornir */
    if (!IS_NPC(ch) && (CLASS_LEVEL(ch, guild_info[i].pc_class) > 0))
      continue;

    send_to_char(ch, "%s", buf);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return (TRUE);
  }
  return (FALSE);
}

SPECIAL(puff)
{
  char actbuf[MAX_INPUT_LENGTH];

  if (cmd)
    return (FALSE);

  switch (rand_number(0, 60))
  {
  case 0:
    do_say(ch, strcpy(actbuf, "My god!  It's full of stars!"), 0, 0); /* strcpy: OK */
    return (TRUE);
  case 1:
    do_say(ch, strcpy(actbuf, "How'd all those fish get up here?"), 0, 0); /* strcpy: OK */
    return (TRUE);
  case 2:
    do_say(ch, strcpy(actbuf, "I'm a very female dragon."), 0, 0); /* strcpy: OK */
    return (TRUE);
  case 3:
    do_say(ch, strcpy(actbuf, "I've got a peaceful, easy feeling."), 0, 0); /* strcpy: OK */
    return (TRUE);
  default:
    return (FALSE);
  }
}

SPECIAL(fido)
{
  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
  {
    if (!IS_CORPSE(i))
      continue;

    act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
    for (temp = i->contains; temp; temp = next_obj)
    {
      next_obj = temp->next_content;
      obj_from_obj(temp);
      obj_to_room(temp, IN_ROOM(ch));
    }
    extract_obj(i);
    return (TRUE);
  }
  return (FALSE);
}

SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
  {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);
  }
  return (FALSE);
}

/* from homeland */
SPECIAL(fzoul)
{
  if (!ch && !cmd)
    return FALSE;

  if (cmd && CMD_IS("kneel"))
  {
    send_to_char(ch, "\tLFzoul tells you, '\tgSee how easy it is to kneel before the beauty of our god.\tL'\tn\r\n");
    return TRUE;
  }
  return FALSE;
}

SPECIAL(cityguard)
{
  struct char_data *tch, *evil, *spittle;
  int max_evil, min_cha;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  max_evil = 1000;
  min_cha = 6;
  spittle = evil = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!CAN_SEE(ch, tch))
      continue;
    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER))
    {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return (TRUE);
    }

    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF))
    {
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return (TRUE);
    }

    if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil && (IS_NPC(tch) || IS_NPC(FIGHTING(tch))))
    {
      max_evil = GET_ALIGNMENT(tch);
      evil = tch;
    }

    if (GET_CHA(tch) < min_cha)
    {
      spittle = tch;
      min_cha = GET_CHA(tch);
    }
  }

  /*
  if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0) {
    act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    return (TRUE);
  }
   */

  /* Reward the socially inept. */
  if (spittle && !rand_number(0, 9))
  {
    static int spit_social;

    if (!spit_social)
      spit_social = find_command("spit");

    if (spit_social > 0)
    {
      char spitbuf[MAX_NAME_LENGTH + 1];
      strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf)); /* strncpy: OK */
      spitbuf[sizeof(spitbuf) - 1] = '\0';
      do_action(ch, spitbuf, spit_social, 0);
      return (TRUE);
    }
  }
  return (FALSE);
}

SPECIAL(clan_cleric)
{
  int i;
  char buf[MAX_STRING_LENGTH];
  zone_vnum clanhall;
  clan_vnum clan;
  struct char_data *this_mob = (struct char_data *)me;

  struct price_info
  {
    short int number;
    char name[25];
    short int price;
  } clan_prices[] = {
      /* Spell Num (defined)      Name shown        Price  */
      {SPELL_ARMOR, "armor             ", 75},
      {SPELL_BLESS, "bless            ", 150},
      {SPELL_REMOVE_POISON, "remove poison    ", 525},
      {SPELL_CURE_BLIND, "cure blindness   ", 375},
      {SPELL_CURE_CRITIC, "critic           ", 525},
      {SPELL_SANCTUARY, "sanctuary       ", 3000},
      {SPELL_HEAL, "heal            ", 3500},

      /* The next line must be last, add new spells above. */
      {-1, "\r\n", -1}};

  if (CMD_IS("buy") || CMD_IS("list"))
  {
    argument = one_argument_u(argument, buf);

    /* Which clanhall is this cleric in? */
    clanhall = zone_table[(GET_ROOM_ZONE(IN_ROOM(this_mob)))].number;
    if ((clan = zone_is_clanhall(clanhall)) == NO_CLAN)
    {
      log("SYSERR: clan_cleric spec (%s) not in a known clanhall (room %d)", GET_NAME(this_mob), world[(IN_ROOM(this_mob))].number);
      return FALSE;
    }
    if (clan != GET_CLAN(ch))
    {
      snprintf(buf, sizeof(buf), "$n will only serve members of %s", CLAN_NAME(real_clan(clan)));
      act(buf, TRUE, this_mob, 0, ch, TO_VICT);
      return TRUE;
    }

    if (FIGHTING(ch))
    {
      send_to_char(ch, "You can't do that while fighting!\r\n");
      return TRUE;
    }

    if (*buf)
    {
      for (i = 0; clan_prices[i].number > SPELL_RESERVED_DBC; i++)
      {
        if (is_abbrev(buf, clan_prices[i].name))
        {
          if (GET_GOLD(ch) < clan_prices[i].price)
          {
            act("$n tells you, 'You don't have enough gold for that spell!'",
                FALSE, this_mob, 0, ch, TO_VICT);
            return TRUE;
          }
          else
          {

            act("$N gives $n some money.",
                FALSE, this_mob, 0, ch, TO_NOTVICT);
            send_to_char(ch, "You give %s %d coins.\r\n",
                         GET_NAME(this_mob), clan_prices[i].price);
            decrease_gold(ch, clan_prices[i].price);
            /* Uncomment the next line to make the mob get RICH! */
            /* increase_gold(this_mob, clan_prices[i].price); */

            cast_spell(this_mob, ch, NULL, clan_prices[i].number, 0);
            return TRUE;
          }
        }
      }
      act("$n tells you, 'I do not know of that spell!"
          "  Type 'buy' for a list.'",
          FALSE, this_mob,
          0, ch, TO_VICT);

      return TRUE;
    }
    else
    {
      act("$n tells you, 'Here is a listing of the prices for my services.'",
          FALSE, this_mob, 0, ch, TO_VICT);
      for (i = 0; clan_prices[i].number > SPELL_RESERVED_DBC; i++)
      {
        send_to_char(ch, "%s%d\r\n", clan_prices[i].name, clan_prices[i].price);
      }
      return TRUE;
    }
  }
  return FALSE;
}

SPECIAL(clan_guard)
{
  zone_vnum clanhall, to_zone;
  clan_vnum clan;
  struct char_data *guard = (struct char_data *)me;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || IS_AFFECTED(guard, AFF_BLIND))
    return FALSE;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  /* Which clanhall is this cleric in? */
  clanhall = zone_table[(GET_ROOM_ZONE(IN_ROOM(guard)))].number;
  if ((clan = zone_is_clanhall(clanhall)) == NO_CLAN)
  {
    log("SYSERR: clan_guard spec (%s) not in a known clanhall (room %d)", GET_NAME(guard), world[(IN_ROOM(guard))].number);
    return FALSE;
  }

  /* This is the player's clanhall, allow them to pass */
  if (GET_CLAN(ch) == clan)
  {
    return FALSE;
  }

  /* If the exit leads to another clanhall room, block it */
  /* NOTE: cmd equals the direction for directional commands */
  if (EXIT(ch, cmd) && EXIT(ch, cmd)->to_room && EXIT(ch, cmd)->to_room != NOWHERE)
  {
    to_zone = zone_table[(GET_ROOM_ZONE(EXIT(ch, cmd)->to_room))].number;
    if (to_zone == clanhall)
    {
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  /* If we get here, player is allowed to leave */
  return FALSE;
}

/* from homeland */
SPECIAL(shar_heart)
{
  struct char_data *vict = FIGHTING(ch);
  struct affected_type af;
  int dam = 0;

  if (!ch || cmd || !vict)
    return FALSE;

  if (rand_number(0, 15))
    return FALSE;

  act("\tmThe \tMHeart of Shar \tn\tmpulses erratically in\r\n"
      "your hand before striking $N \tmwith a beam of\r\n"
      "\tLmalevolent light\tn\tm, bathing and filling $M with\r\n"
      "the virulence of the \tLL\tMady of \tLL\tMoss.\tn",
      FALSE, ch, 0, vict, TO_CHAR);

  act("\tmThe amethyst orb wielded by \tL$n \tn\tmpulses\r\n"
      "erratically before a beam of \tLmalevolent light\r\n"
      "\tn\tmshoots from it, striking you in the chest!\tn",
      FALSE, ch, 0, vict, TO_VICT);

  act("\tL$n \tn\tmis bathed in an amethyst radiance as $s\r\n"
      "\tMHeart of Shar \tn\tmpulses erratically.  Suddenly a\r\n"
      "sickly beam of \tLmalevolent light \tn\tmblazes\r\n"
      "towards $N\tm, filling $S body with the \tLvirulence\r\n"
      "\tn\tmof the \tLL\tMady of \tLL\tMoss.\tn",
      FALSE, ch, 0, vict, TO_ROOM);

  af.duration = 5;
  af.modifier = -4;
  af.location = APPLY_STR;
  af.spell = SPELL_POISON;
  affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);

  dam = dice(6, 3) + 4;
  GET_HIT(vict) -= dam;
  return TRUE;
}

/* from homeland */
SPECIAL(shar_statue)
{
  struct char_data *mob;

  if (!FIGHTING(ch))
    return FALSE;
  if (cmd)
    return FALSE;

  if (!rand_number(0, 8) || !PROC_FIRED(ch))
  {
    PROC_FIRED(ch) = TRUE;
    send_to_room(ch->in_room,
                 "\tLThe statue raises her ebon arms, screaming out to\r\n"
                 "her deity in a booming voice, '\tn\tmLady of loss,\r\n"
                 "mistress of the night, smite those who befoul your\r\n"
                 "house.  Send forth your faithful to quench the light\r\n"
                 "of their moon!\tL'\tn\r\n");

    if (dice(1, 100) < 50)
      mob = read_mobile(106241, VIRTUAL);
    else
      mob = read_mobile(106240, VIRTUAL);

    if (!mob)
      return FALSE;

    char_to_room(mob, ch->in_room);
    add_follower(mob, ch);

    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(dog)
{
  int random = 0;
  struct affected_type af;
  struct char_data *pet = (struct char_data *)me;

  if (!argument)
    return FALSE;
  if (!cmd)
    return FALSE;

  skip_spaces(&argument);

  if (!isname(argument, GET_NAME(pet)))
    return FALSE;

  if (CMD_IS("pet") || CMD_IS("pat"))
  {
    random = dice(1, 3);
    switch (random)
    {
    case 3:
      act("$n tries to lick your hand as you pet $m.", FALSE, pet, 0, ch, TO_VICT);
      act("$n tries to lick the hand of $N as $E pet $m.", FALSE, pet, 0, ch, TO_NOTVICT);
      break;
    case 2:
      act("$n looks at you with adoring eyes as you pet $m.", FALSE, pet, 0, ch, TO_VICT);
      act("$n looks at $N with adoring eyes as $E pet $m.", FALSE, pet, 0, ch, TO_NOTVICT);
      break;
    case 1:
    default:
      act("$n wags $s tail happily, as you pet $m.", FALSE, pet, 0, ch, TO_VICT);
      act("$n wags $s tail happily, as $N pets $m.", FALSE, pet, 0, ch, TO_NOTVICT);
      break;
    }

    if (GET_LEVEL(pet) < 2 && ch->followers == 0 && ch->master == 0 && pet->master == 0 && !circle_follow(pet, ch))
    {
      add_follower(pet, ch);
      af.spell = SPELL_CHARM;
      af.duration = 24000;
      af.modifier = 0;
      af.location = 0;
      SET_BIT_AR(af.bitvector, AFF_CHARM);
      affect_to_char(pet, &af);
    }
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(illithid_gguard)
{
  const char *buf = "$N \tLsteps in front of you, blocking you from accessing the gate.\tn";
  const char *buf2 = "$N \tLsteps in front of $n\tL, blocking access the gate.\tn";

  if (!IS_MOVE(cmd))
    return FALSE;

  // if (cmd == SCMD_EAST && GET_RACE(ch) != RACE_ILLITHID) {
  if (cmd == SCMD_EAST)
  {
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);
    act(buf2, FALSE, ch, 0, (struct char_data *)me, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(duergar_guard)
{
  const char *buf = "$N steps into the opening and blocks your path.\r\n";
  const char *buf2 = "$N steps into the opening blocking it.";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (cmd == SCMD_DOWN)
  {
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);
    act(buf2, FALSE, ch, 0, (struct char_data *)me, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(bandit_guard)
{
  const char *buf = "$N blocks your access into the castle.\r\n";
  const char *buf2 = "$N blocks $n's access into the castle..";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (GET_LEVEL(ch) < 12)
    return FALSE;

  if (cmd == SCMD_EAST || cmd == SCMD_SOUTH || cmd == SCMD_WEST)
  {
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);
    act(buf2, FALSE, ch, 0, (struct char_data *)me, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(secomber_guard)
{
  const char *buf = "\tLThe doorguard steps before you, blocking your way with an upraised hand.\tn\r\n";
  const char *buf2 = "\tLThe doorguard blocks \tn$n\tL's way, placing one meaty hand on $s chest.\tn";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (cmd == SCMD_EAST)
  {
    send_to_char(ch, buf);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
/*
SPECIAL(guild_golem) {
  bool found = TRUE;
  const char *msg1 = "The golem humiliates you, and blocks your way.\r\n";
  const char *msg2 = "The golem humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd))
    return FALSE;

  int i = cmd - 1;

  if (i < 0) {
    send_to_char("Index error in guild golem\r\n", ch);
    return FALSE;
  }

  if (!EXIT(ch, i))
    found = FALSE;
  else {
    int room_number = world[ch->in_room].dir_option[i]->to_room;
    if (world[room_number].guild_index) {
      if (GET_GUILD(ch) != world[room_number].guild_index && GET_ALT(ch) != world[room_number].guild_index)
        found = FALSE;
    }
  }

  if (!found) {
    send_to_char(msg1, ch);
    act(msg2, FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}
 */

/* from Homeland */
/*
SPECIAL(guild_guard) {
  int i;
  bool found = TRUE;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  for (i = 0; guild_info[i][0] != -1; i++) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == guild_info[i][1] &&
            cmd == guild_info[i][2]) {
      if (IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) {
        found = FALSE;
      } else {
        found = TRUE;
        break;
      }
    }
  }

  if (!found) {
    send_to_char(buf, ch);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}
 */

/* from Homeland */
// doesnt work properly if multiple instances.. :) -V

SPECIAL(practice_dummy)
{
  int rounddam = 0;
  static int round_count;
  static int max_hit;
  char buf[MAX_INPUT_LENGTH];

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
  {
    GET_MAX_HIT(ch) = 20000;
    GET_HIT(ch) = 20000;
    max_hit = 0;
    round_count = 0;
  }
  else
  {
    rounddam = GET_MAX_HIT(ch) - GET_HIT(ch);
    max_hit += rounddam;
    round_count++;

    snprintf(buf, sizeof(buf), "\tP%d damage last round!\tn  \tc(total: %d rounds: %d)\tn\r\n",
             rounddam, max_hit, round_count);
    send_to_room(ch->in_room, buf);
    GET_HIT(ch) = GET_MAX_HIT(ch);
    return TRUE;
  }
  return FALSE;
}

/* from Homeland */
SPECIAL(wraith)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->master && ch->in_room == ch->master->in_room)
    if (FIGHTING(ch->master) && rand_number(0, 1))
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }

  return FALSE;
}

/* from Homeland */
SPECIAL(skeleton_zombie)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->master && ch->in_room == ch->master->in_room)
    if (FIGHTING(ch->master) && !rand_number(0, 2))
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }

  return FALSE;
}

/* from Homeland */
SPECIAL(vampire)
{
  struct char_data *vict;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->master && ch->in_room == ch->master->in_room)
  {
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    {
      if (FIGHTING(vict) == ch->master && !rand_number(0, 1))
      {
        perform_rescue(ch, ch->master);
        return TRUE;
      }
    }
  }

  return FALSE;
}

/* from Homeland */
SPECIAL(totemanimal)
{
  if (cmd)
    return FALSE;
  if (!ch->master)
    return FALSE;

  if (ch->master && ch->in_room == ch->master->in_room)
    if (FIGHTING(ch->master))
      perform_assist(ch, ch->master);
  return FALSE;
}

/* from Homeland */
SPECIAL(shades)
{
  if (cmd)
    return FALSE;

  if (GET_MAX_HIT(ch) > 1 && GET_HIT(ch) > 1)
  {
    GET_MAX_HIT(ch) = 1;
    GET_HIT(ch) = 1;
  }

  if (GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (GET_HIT(ch) < GET_MAX_HIT(ch) || !ch->master)
  {
    act("A shade evaporates into thin air.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (ch->in_room != ch->master->in_room)
  {
    HUNTING(ch) = ch->master;
    hunt_victim(ch);
    return TRUE;
  }
  return FALSE;
}

/* from Homeland */
SPECIAL(solid_elemental)
{
  struct char_data *vict;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || (!ch->master && !MOB_FLAGGED(ch, MOB_MEMORY)))
  {
    act("With a loud shriek, $n returns to $s home plane.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (GET_HIT(ch) > 0)
  {
    if (ch->master && ch->in_room == ch->master->in_room && !rand_number(0, 1))
    {
      for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      {
        if (FIGHTING(vict) == ch->master)
        {
          perform_rescue(ch, ch->master);
          return TRUE;
        }
      }
    }

    if (!FIGHTING(ch) && ch->master && FIGHTING(ch->master) && ch->in_room == ch->master->in_room)
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }
  }

  // auto stand if down
  if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) >= POS_STUNNED)
  {
    change_position(ch, POS_STANDING);
    act("$n clambers to $s feet.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  // we're fighting something we dont want to fight...
  if (!ch->master && FIGHTING(ch) && IS_NPC(FIGHTING(ch)) && !IS_PET(FIGHTING(ch)))
    do_flee(ch, 0, 0, 0);

  return FALSE;
}

/* from Homeland */
SPECIAL(wraith_elemental)
{
  struct char_data *vict;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) == POS_DEAD || (!ch->master && !MOB_FLAGGED(ch, MOB_MEMORY)))
  {
    act("With a loud shriek, $n returns to $s home plane.", FALSE, ch, NULL, 0, TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (GET_HIT(ch) > 0)
  {
    if (ch->master && ch->in_room == ch->master->in_room && !rand_number(0, 1))
    {
      for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      {
        if (FIGHTING(vict) == ch->master)
        {
          perform_rescue(ch, ch->master);
          return TRUE;
        }
      }
    }

    if (!FIGHTING(ch) && ch->master && FIGHTING(ch->master) && ch->in_room == ch->master->in_room)
    {
      perform_assist(ch, ch->master);
      return TRUE;
    }
  }

  // auto stand if down
  if (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) >= POS_STUNNED)
  {
    change_position(ch, POS_STANDING);
    act("$n clambers to $s feet.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  // we're fighting something we dont want to fight...
  if (!ch->master && FIGHTING(ch) && IS_NPC(FIGHTING(ch)) && !IS_PET(FIGHTING(ch)))
    do_flee(ch, 0, 0, 0);

  return FALSE;
}

/* from homeland */
SPECIAL(planewalker)
{
  if (cmd)
    return FALSE;

  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    act("$n looks around in panic when he realizes that his spells\r\n"
        "would fizzle. He reaches down into his pockets and pulls out an ancient\r\n"
        "rod. He taps the rod and suddenly disappears!",
        FALSE, ch, 0, 0, TO_ROOM);
    call_magic(ch, 0, 0, SPELL_TELEPORT, 0, 30, CAST_WAND);
    return TRUE;
  }
  if (!FIGHTING(ch) && GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    act("$n checks on his wounds, and grabs a potion from his pockets.", FALSE, ch, 0, 0, TO_ROOM);
    call_magic(ch, ch, 0, SPELL_HEAL, 0, 30, CAST_POTION);
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(phantom)
{
  struct char_data *vict;
  struct char_data *next_vict;
  int prob, percent;

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
    return FALSE;
  if (rand_number(0, 4))
    return FALSE;

  act("$n \tLlets out a \trfrightening\tL wail\tn",
      FALSE, ch, 0, 0, TO_ROOM);

  for (vict = world[ch->in_room].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (vict == ch)
      continue;
    if (IS_NPC(vict) && !IS_PET(vict))
      continue;

    percent = rand_number(1, 111); /* 101% is a complete failure */
    prob = GET_WIS(vict) + 5;
    if (FIGHTING(vict))
      prob *= 2;
    if (prob > 100)
      prob = 100;

    if (percent > prob)
      do_flee(vict, NULL, 0, 0);
  }
  return TRUE;
}

/* from homeland */
SPECIAL(lichdrain)
{
  struct char_data *tch = 0;
  struct char_data *vict = 0;
  int dam = 0;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (rand_number(0, 3))
    return FALSE;
  if (!FIGHTING(ch))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_PARALYZED))
    return FALSE;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch) || IS_PET(tch))
    {
      if (!vict || !rand_number(0, 2))
      {
        vict = tch;
      }
    }
  }

  if (!vict)
    return FALSE;

  act("\tn$n\tL looks deep into your soul with $s horrid gaze.\tn\r\n"
      "\tLand $e simply leeches your \tWlifeforce\tL out of you.\r\n",
      FALSE, ch, 0, vict, TO_VICT);

  act("\tn$n\tL looks deep into the eyes of $N\tL with $s horrid gaze.\tn\r\n"
      "\tLand $e simply leeches $S \tWlifeforce\tL out of $M.\r\n",
      TRUE, ch, 0, vict, TO_NOTVICT);

  act("\tWYou reach out and suck the life force away from $N!", TRUE, ch, 0,
      vict, TO_CHAR);
  dam = GET_HIT(vict) + 5;
  if (GET_HIT(ch) + dam < GET_MAX_HIT(ch))
    GET_HIT(ch) += dam;
  GET_HIT(vict) -= dam;
  USE_FULL_ROUND_ACTION(vict);
  return TRUE;
}

/* from homeland */
SPECIAL(harpell)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH];

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
  {
    if (AFF_FLAGGED(FIGHTING(ch), AFF_CHARM) && FIGHTING(ch)->master)
      snprintf(buf, sizeof(buf), "%s shouts, 'HELP! %s has ordered his pets to kill "
                                 "me!!'\r\n",
               ch->player.short_descr,
               GET_NAME(FIGHTING(ch)->master));
    else
      snprintf(buf, sizeof(buf), "%s shouts, 'HELP! %s is trying to kill me!\r\n",
               ch->player.short_descr, GET_NAME(FIGHTING(ch)));
    for (i = character_list; i; i = i->next)
    {
      if (!FIGHTING(i) && IS_NPC(i) && (GET_MOB_VNUM(i) == 106831 || GET_MOB_VNUM(i) == 106841 || GET_MOB_VNUM(i) == 106842 || GET_MOB_VNUM(i) == 106844 || GET_MOB_VNUM(i) == 106845 || GET_MOB_VNUM(i) == 106846) && ch != i && !rand_number(0, 2))
      {
        if (AFF_FLAGGED(FIGHTING(ch), AFF_CHARM) && FIGHTING(ch)->master &&
            (FIGHTING(ch)->master->in_room != FIGHTING(ch)->in_room))
        {
          if (FIGHTING(ch)->master->in_room != i->in_room)
            cast_spell(i, FIGHTING(ch)->master, NULL, SPELL_TELEPORT, 0);
          else
            hit(i, FIGHTING(ch)->master, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0,
                FALSE);
        }
        else
        {
          if (FIGHTING(ch)->in_room != i->in_room)
            cast_spell(i, FIGHTING(ch), NULL, SPELL_TELEPORT, 0);
          else
            hit(i, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        }
      }

      if (world[ch->in_room].zone == world[i->in_room].zone && !PROC_FIRED(ch))
        send_to_char(i, buf);
    }
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  } // for loop

  return FALSE;
}

/* from homeland */
SPECIAL(bonedancer)
{
  struct char_data *vict;
  struct char_data *next_vict;

  if (cmd)
    return FALSE;
  if (GET_POS(ch) == POS_DEAD || !ch->master)
  {
    act("With a loud shriek, $n crumbles into dust.", FALSE, ch, NULL, 0,
        TO_ROOM);
    extract_char(ch);
    return TRUE;
  }

  if (!FIGHTING(ch) && GET_HIT(ch) > 0)
  {
    for (vict = world[ch->in_room].people; vict; vict = next_vict)
    {
      next_vict = vict->next_in_room;
      if (vict != ch && CAN_SEE(ch, vict))
      {
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        return TRUE;
      }
    }
  }

  return FALSE;
}

/* from homeland */
SPECIAL(wallach)
{

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (GET_ROOM_VNUM(GET_MOB_LOADROOM(ch)) != 112638)
    GET_MOB_LOADROOM(ch) = real_room(112638);

  return FALSE;
}

/* from homeland */
SPECIAL(beltush)
{
  struct char_data *i;

  if (cmd || GET_POS(ch) == POS_DEAD || GET_ROOM_VNUM(ch->in_room) != 112648)
    return FALSE;

  for (i = character_list; i; i = i->next)
    if (!IS_NPC(i) && GET_ROOM_VNUM(i->in_room) == 112602)
    {
      do_enter(ch, "mirror", 0, 0);
      act("Beltush says, 'FOOLS!! How dare you attempt to enter the flaming "
          "tower!!",
          FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }

  return FALSE;
}

/* from homeland */
SPECIAL(mereshaman)
{
  if (cmd)
    return FALSE;

  if (FIGHTING(ch) && !PROC_FIRED(ch))
  {
    PROC_FIRED(ch) = TRUE;
    send_to_room(ch->in_room,
                 "\tLThe \tglizardman \tLshaman chants loudly, '\tGUktha slithiss "
                 "Semuanya! Ssithlarss sunggar uk!\tL'\tn\r\n"
                 "\tLThe monitor lizard statues shudder and vibrate then take on \tn\r\n"
                 "\tLa \tGbright green glow\tL. Each opens up like a cocoon releasing the\tn\r\n"
                 "\tLreptilian beast contained within.\tn\r\n");

    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(mercenary)
{
  int hit;
  int base = 1;

  if (!ch)
    return FALSE;
  if (cmd)
    return FALSE;

  // a recruited merc should get reasonable amounts of hp.
  if (PROC_FIRED(ch) == FALSE && IS_PET(ch))
  {
    switch (GET_CLASS(ch))
    {
    case CLASS_RANGER:
    case CLASS_PALADIN:
    case CLASS_BERSERKER:
    case CLASS_WARRIOR:
    case CLASS_WEAPON_MASTER:
    case CLASS_STALWART_DEFENDER:
    case CLASS_DUELIST:
      base = 8;
      break;
    case CLASS_ROGUE:
      //      case CLASS_SHADOW_DANCER:
      //      case CLASS_ASSASSIN:
    case CLASS_MONK:
    case CLASS_SACRED_FIST:
    case CLASS_SHIFTER:
      base = 5;
      break;

    default:
      base = 3;
      break;
    }

    hit = dice(GET_LEVEL(ch), (1 + GET_CON_BONUS(ch))) + GET_LEVEL(ch) * base;
    GET_MAX_HIT(ch) = hit;
    if (GET_HIT(ch) > hit)
      GET_HIT(ch) = hit;
    PROC_FIRED(ch) = TRUE;
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(battlemaze_guard)
{
  const char *buf = "$N \tL tells you, 'You don't want to go any farther, young one. \tn\r\n"
                    "\tL You must be at least level ten to go into the more advanced\tn\r\n"
                    "\tL parts of the battlemaze.'\tn";
  const char *buf2 = "$N \tLsteps in front of $n\tL, blocking access the gate.\tn";

  if (!IS_MOVE(cmd))
    return FALSE;

  if (cmd == SCMD_NORTH && GET_LEVEL(ch) < 10)
  {
    act(buf, FALSE, ch, 0, (struct char_data *)me, TO_CHAR);
    act(buf2, FALSE, ch, 0, (struct char_data *)me, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(willowisp)
{
  room_rnum room = real_room(126899);

  if (cmd)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  if (ch->in_room != room && weather_info.sunlight == SUN_LIGHT)
  {
    act("$n fades away in the sunlight!", FALSE, ch, 0, 0, TO_ROOM);
    ch->mob_specials.temp_room_data = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, room);

    return TRUE;
  }

  if (ch->in_room == room && weather_info.sunlight != SUN_LIGHT)
  {
    char_from_room(ch);
    char_to_room(ch, ch->mob_specials.temp_room_data);
    act("$n appears with the dark of the night!", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(naga_golem)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (!FIGHTING(ch))
    PROC_FIRED(ch) = FALSE;

  if (FIGHTING(ch))
  {
    zone_yell(ch,
              "\r\n\tLThe golem rings an alarm bell, which echoes through "
              "the pit.\tn\r\n");
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(naga)
{
  struct char_data *tch = 0;
  struct char_data *vict = 0;
  int dam = 0;

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (rand_number(0, 3))
    return FALSE;
  if (!FIGHTING(ch))
    return FALSE;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
  {
    if ((!IS_NPC(tch) || IS_PET(tch)) && !MOB_FLAGGED(tch, MOB_NOSLEEP))
    {
      if (!vict || !rand_number(0, 3))
      {
        vict = tch;
      }
    }
  }
  if (!vict)
    return FALSE;

  if (MOB_FLAGGED(vict, MOB_NOSLEEP))
    return FALSE;

  act("$n\tL thrusts its powerful barbed tail-stinger into your flesh causing\tn\r\n"
      "\tLyou to scream in agony.  As it snaps back its tail, poison oozes into the large\tn\r\n"
      "\tLwound that is opened.  You begin to fall into a drug induced "
      "sleep.\tn\r\n",
      FALSE, ch, 0, vict, TO_VICT);

  act("$n\tL thrusts its powerful barbed tail-stinger into $N's flesh causing\tn\r\n"
      "\tL$M\tL to scream in agony.  As it snaps back its tail, poison oozes into the large\tn\r\n"
      "\tLwound that is opened.  \tn$N\tL begin to fall into a drug "
      "induced sleep.\tn\r\n",
      TRUE, ch, 0, vict, TO_NOTVICT);

  act("\tLYour poison stinger hits $N!\tn", TRUE, ch, 0, vict, TO_CHAR);
  dam = GET_LEVEL(ch) * 2 + dice(2, GET_LEVEL(ch));
  if (dam > GET_HIT(vict))
    dam = GET_HIT(vict);
  if (dam < 0)
    dam = 0;
  GET_HIT(vict) -= dam;
  stop_fighting(vict);
  change_position(vict, POS_SLEEPING);
  /* Would be best to make this an affect that affects your ability to wake up, lasting a couple rounds. */
  USE_FULL_ROUND_ACTION(vict);
  return TRUE;
}

/* from homeland */
SPECIAL(ethereal_pet)
{

  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;
  if (FIGHTING(ch))
    return FALSE;

  if (ch->desc == 0)
  {
    extract_char(ch);
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(fp_invoker)
{
  struct char_data *victim;

  if (!ch)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  if (!IS_NPC(ch) && cmd && CMD_IS("cast") && GET_POS(ch) >= POS_FIGHTING)
  {
    victim = ch;
    ch = (struct char_data *)me;
    act("$n screams in rage, 'How DARE you cast a spell in my tower'", FALSE, ch, 0, 0, TO_ROOM);
    call_magic(ch, victim, 0, SPELL_MISSILE_STORM, 0, 30, CAST_SPELL);
    return FALSE;
  }

  return FALSE;
}

/* from homeland */
static bool gr_stalled = FALSE;

SPECIAL(gromph)
{
  struct char_data *victim;
  int dir = -1;

  if (!ch)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  if (!IS_NPC(ch) && cmd && CMD_IS("cast"))
  {
    victim = ch;
    ch = (struct char_data *)me;
    act("$n sighs at YOU and mutters, 'You insolent worm!'", FALSE, ch, 0, victim, TO_VICT);
    act("$n sighs at $N, 'You insolent worm!'", FALSE, ch, 0, victim, TO_NOTVICT);
    call_magic(ch, victim, 0, SPELL_MISSILE_STORM, 0, 30, CAST_SPELL);
    return TRUE;
  }

  if (PATH_DELAY(ch) > 0)
    PATH_DELAY(ch)
  --;
  PATH_DELAY(ch) = 4;

  if (cmd)
    return FALSE;

  {
    switch (PROC_FIRED(ch))
    {
    case 0:
      // move to sorcere
      dir = find_first_step(ch->in_room, real_room(135250));
      if (dir < 0)
        PROC_FIRED(ch) = 1;
      break;
    case 1:
      // move to narbondel
      dir = find_first_step(ch->in_room, real_room(135353));

      if (dir < 0)
      {
        if (time_info.hours == 0 && gr_stalled == TRUE)
        {
          send_to_zone(
              "\tLSuddenly the base of the gigantic rockpillar known as \trNar\tRbon\trdel\tL\r\n"
              "\tLlights up with intense \trheat\tL, as Gromph Baenre uses his magic to relit it to\r\n"
              "\tLmark the start of a new day in the city.\tn\r\n",
              ch->in_room);
          gr_stalled = FALSE;
          PROC_FIRED(ch) = 0;
        }
        else
          gr_stalled = TRUE;
      }
      break;
    }
    if (dir >= 0)
      perform_move(ch, dir, 1);
    return TRUE;
  }
  return FALSE;
}

/*************************/
/* end mobile procedures */
/*************************/

/********************************************************************/
/******************** Room Procs      *******************************/
/********************************************************************/

/*
SPECIAL(emporium) {

  if (!CMD_IS("emporium"))
    return FALSE;

  char arg[200] = {'\0'}, arg2[200] = {'\0'}, arg3[200] = {'\0'}, arg4[200] = {'\0'};
  int bt = 0, bonus = 0;

  one_argument(one_argument(one_argument(one_argument(argument, arg), arg2), arg3), arg4);

  if (!*arg) {
    send_to_char(ch, "The syntax for this command is: 'item buy <vnum> <bonustype> "
            "<bonus>' or 'item list <weapons|armor|other> <bonustype> <bonus>'.\r\n");
    send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
            "stack in d20 rules.\tn\r\n");
    return TRUE;
  }

  if (is_abbrev(arg, "buy")) {
    if (!*arg2) {
      send_to_char(ch, "Please specify the vnum of the item you wish to buy.  "
              "You may obtain the vnum from the 'item list' command.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    int vnum = atoi(arg2);

    if (!((vnum >= 30000 && vnum <= 30083) || (vnum >= 30085 && vnum <= 30092) ||
            vnum == 30095 || (vnum >= 30100 && vnum <= 30105))) {
      send_to_char(ch, "That is not a valid vnum.  Please select again.\r\n");
      return TRUE;
    }

    struct obj_data *obj = read_object(vnum, VIRTUAL);

    if (!obj) {
      send_to_char(ch, "There was an error buying your item.  Please inform a staff "
              "member with error code ITM_BUY_001.\r\n");
      return TRUE;
    }

    if (GET_OBJ_TYPE(obj) != ITEM_WEAPON && GET_OBJ_TYPE(obj) != ITEM_ARMOR &&
            GET_OBJ_TYPE(obj) != ITEM_WORN) {
      send_to_char(ch, "That is not a valid vnum.  Please select again.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY stack "
              "in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (!*arg3) {
      send_to_char(ch, list_bonus_types());
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if ((bt = get_bonus_type_int(arg3)) == 0) {
      send_to_char(ch, "That's an invalid bonus type.\r\n\r\n");
      send_to_char(ch, list_bonus_types());
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (bt == APPLY_ACCURACY && GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
      send_to_char(ch, "Only weapons can be given that bonus.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (bt == APPLY_AC_ARMOR && !CAN_WEAR(obj, ITEM_WEAR_BODY)) {
      send_to_char(ch, "Only body armor can be given that bonus.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (bt == APPLY_AC_SHIELD && !CAN_WEAR(obj, ITEM_WEAR_SHIELD)) {
      send_to_char(ch, "Only shields can be given that bonus.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (!*arg4) {
      send_to_char(ch, "How much would you like the bonus to be?\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if ((bonus = atoi(arg4)) <= 0) {
      send_to_char(ch, "The bonus must be greater than 0.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if ((bonus = atoi(arg4)) > 100) {
      send_to_char(ch, "The bonus must be less than 100.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    obj->affected[0].location = bt;
    obj->affected[0].modifier = atoi(arg4);

    if ((bt == APPLY_AC_SHIELD || bt == APPLY_AC_DEFLECTION || bt == APPLY_AC_NATURAL ||
            bt == APPLY_AC_ARMOR))
      obj->affected[0].modifier *= 10;

    if (bt == APPLY_ACCURACY) {
      obj->affected[1].location = APPLY_DAMAGE;
      obj->affected[1].modifier = atoi(arg4);
    }

    GET_OBJ_LEVEL(obj) = set_object_level(obj);
    GET_OBJ_COST(obj) = MAX(10, 100 + GET_OBJ_LEVEL(obj) * 50 *
            MAX(1, GET_OBJ_LEVEL(obj) - 1) + GET_OBJ_COST(obj));

    int cost = MAX(10, GET_OBJ_LEVEL(obj) * (GET_OBJ_LEVEL(obj) / 2) * 3);

    spell_identify(20, ch, ch, obj, NULL);

    if (GET_OBJ_LEVEL(obj) > GET_CLASS_LEVEL(ch)) {
      send_to_char(ch, "You cannot buy an item whose level is greater than yours.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY stack "
              "in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (GET_QUESTPOINTS(ch) < cost) {
      send_to_char(ch, "That item costs %d reputation points and you only have "
              "%d.\r\n", cost, GET_QUESTPOINTS(ch));
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    GET_QUESTPOINTS(ch) -= cost;

    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
    char buf[200] = {'\0'};
    snprintf(buf, sizeof(buf), "%s +%d %s", obj->short_description, bonus, get_bonus_type(arg3));
    obj->short_description = strdup(buf);
    obj->name = strdup(buf);
    snprintf(buf, sizeof(buf), "%s +%d %s lies here.", CAP(obj->short_description), bonus,
            get_bonus_type(arg3));
    obj->description = strdup(buf);

    obj_to_char(obj, ch);

    send_to_char(ch, "You purchase %s for %d reputation points.\r\n",
            obj->short_description, cost);
    return TRUE;
  } else if (is_abbrev(arg, "list")) {

    if (!*arg2) {
      send_to_char(ch, "Please specify either armor, weapon or other.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    int type = ITEM_OTHER;

    if (is_abbrev(arg2, "armor"))
      type = ITEM_ARMOR;
    else if (is_abbrev(arg2, "weapon"))
      type = ITEM_WEAPON;
    else if (is_abbrev(arg2, "other"))
      type = ITEM_WORN;
    else {
      send_to_char(ch, "Please specify either armor, weapon or other.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (!*arg3) {
      send_to_char(ch, list_bonus_types());
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if ((bt = get_bonus_type_int(arg3)) == 0) {
      send_to_char(ch, "That's an invalid bonus type.\r\n\r\n");
      send_to_char(ch, list_bonus_types());
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if (!*arg4) {
      send_to_char(ch, "How much would you like the bonus to be?\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    if ((bonus = atoi(arg4)) <= 0) {
      send_to_char(ch, "The bonus must be greater than 0.\r\n");
      send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
              "stack in d20 rules.\tn\r\n");
      return TRUE;
    }

    char buf[100] = {'\0'};
    int cost = 0;

    struct obj_data *obj = NULL;
    int i = 0;
    int vnum = 0;

    send_to_char(ch, "%-5s %-6s %-7s %-35s\r\n----- ------ -------------------------\r\n",
            "VNUM", "COST", "MIN-LVL", "ITEM");

    for (i = 30000; i < 30299; i++) {
      vnum = i;
      if (!((vnum >= 30000 && vnum <= 30083) || (vnum >= 30085 && vnum <= 30092) ||
              vnum == 30095 || (vnum >= 30100 && vnum <= 30105))) {
        continue;
      }
      if (obj)
        extract_obj(obj);
      obj = read_object(i, VIRTUAL);
      if (!obj)
        continue;
      if (GET_OBJ_TYPE(obj) != type)
        continue;
      obj->affected[0].location = bt;
      obj->affected[0].modifier = atoi(arg4);

      if ((bt == APPLY_AC_SHIELD || bt == APPLY_AC_DEFLECTION ||
              bt == APPLY_AC_NATURAL || bt == APPLY_AC_ARMOR))
        obj->affected[0].modifier *= 10;

      if (bt == APPLY_ACCURACY) {
        obj->affected[1].location = APPLY_DAMAGE;
        obj->affected[1].modifier = atoi(arg4);
      }

      GET_OBJ_LEVEL(obj) = set_object_level(obj);
      GET_OBJ_COST(obj) = MAX(10, 100 + GET_OBJ_LEVEL(obj) * 50 *
              MAX(1, GET_OBJ_LEVEL(obj) - 1) + GET_OBJ_COST(obj));

      cost = MAX(10, GET_OBJ_LEVEL(obj) * (GET_OBJ_LEVEL(obj) / 2) * 3);

      snprintf(buf, sizeof(buf), "%s +%d %s", obj->short_description, bonus, get_bonus_type(arg3));
      send_to_char(ch, "%-5d %-6d %d %-35s\r\n", i, cost, GET_OBJ_LEVEL(obj), buf);
    }
    send_to_char(ch, "\r\n");
    send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY stack "
            "in d20 rules.\tn\r\n");
    return TRUE;
  } else {
    send_to_char(ch, "The syntax for this command is: 'item buy <vnum>' or 'item "
            "list <weapons|armor|other>'.\r\n");
    send_to_char(ch, "\tYPlease note that bonuses of the same type \tRDO NOT\tY "
            "stack in d20 rules.\tn\r\n");
    return TRUE;
  }

  return TRUE;
}
 */

/* research spells for wizard - this is meant to fill the gap in the game we
   have for lack of scroll placement, special thanks to Stephen Squires for
   parts of the code */
SPECIAL(wizard_library)
{
  bool found = FALSE, full_spellbook = TRUE;
  struct obj_data *obj = NULL;
  int spellnum = SPELL_RESERVED_DBC, spell_level = 0, spell_circle = 0, cost = 100, i = 0;
  int class_level = 0;

  if (CMD_IS("research"))
  {

    if (!CLASS_LEVEL(ch, CLASS_WIZARD))
    {
      send_to_char(ch, "You are not a wizard!\r\n");
      return TRUE;
    }

    skip_spaces(&argument);

    if (!*argument)
    {
      send_to_char(ch, "You need to indicate which spell you want to research.\r\n");
      return TRUE;
    }

    spellnum = find_skill_num(argument);

    if (spellnum <= SPELL_RESERVED_DBC || spellnum >= NUM_SPELLS)
    {
      send_to_char(ch, "Invalid spell!\r\n");
      return TRUE;
    }

    spell_level = spell_info[spellnum].min_level[CLASS_WIZARD];
    spell_circle = (spell_level + 1) / 2;
    class_level = CLASS_LEVEL(ch, CLASS_WIZARD) +
                  BONUS_CASTER_LEVEL(ch, CLASS_WIZARD);

    if (spell_level <= 0 || spell_level >= LVL_IMMORT || spell_circle <= 0 || spell_circle > TOP_CIRCLE)
    {
      send_to_char(ch, "That spell is not available to wizards.\r\n");
      return TRUE;
    }

    if (spell_circle < 7)
    {
      cost = (spell_circle * 50) * (spell_circle);
    }
    else
      cost = (spell_circle * 300) * (spell_circle);

    if (GET_GOLD(ch) < cost)
    {
      send_to_char(ch, "You do not have enough coins to research this spell, you "
                       "need %d coins.\r\n",
                   cost);
      return TRUE;
    }

    if (class_level >= spell_level && GET_SKILL(ch, spellnum))
    {
      /* 1st make sure we have a spellbook handy */
      /* for-loop for inventory */
      for (obj = ch->carrying; obj && !found; obj = obj->next_content)
      {
        if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
        {
          if (spell_in_book(obj, spellnum))
          {
            send_to_char(ch, "You already have the spell '%s' in this spellbook.\r\n",
                         spell_info[spellnum].name);
            return TRUE;
          }
          /* found a spellbook that doesn't have the spell! */
          found = TRUE;
          break; /* our obj variable is now pointing to this spellbook */
        }
      }

      /* for-loop for gear */
      if (!found)
      {
        for (i = 0; i < NUM_WEARS; i++)
        {
          if (GET_EQ(ch, i))
            obj = GET_EQ(ch, i);
          else
            continue;

          if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
          {
            if (spell_in_book(obj, spellnum))
            {
              send_to_char(ch, "You already have the spell '%s' in this spellbook.\r\n",
                           spell_info[spellnum].name);
              return TRUE;
            }
            /* found a spellbook that doesn't have the spell! */
            found = TRUE;
            break; /* our obj variable is now pointing to this spellbook */
          }
        }
      }
    }
    else
    {
      send_to_char(ch, "You are not powerful enough to scribe that spell! (or this spell is from a restricted school)\r\n");
      return TRUE;
    }

    if (!found)
    {
      send_to_char(ch, "No ready spellbook found in your inventory or equipped...\r\n");
      return TRUE;
    }

    /* ok obj variable pointing to spellbook, let's make sure it has space */
    if (!obj->sbinfo)
    { /* un-initialized spellbook, allocate memory */
      CREATE(obj->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
      memset((char *)obj->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
    }
    for (i = 0; i < SPELLBOOK_SIZE; i++)
    { /* check for space */
      if (obj->sbinfo[i].spellname == 0)
      {
        full_spellbook = FALSE;
        break;
      }
      else
      {
        continue;
      }
    } /* i = location in spellbook */

    if (full_spellbook)
    {
      send_to_char(ch, "There is not enough space in that spellbook!\r\n");
      return TRUE;
    }

    /* we made it! */
    GET_GOLD(ch) -= cost;
    obj->sbinfo[i].spellname = spellnum;
    obj->sbinfo[i].pages = MAX(1, lowest_spell_level(spellnum) / 2);
    send_to_char(ch, "Your research is successful and you scribe the spell '%s' "
                     "into your spellbook, which takes up %d pages and cost %d coins.\r\n",
                 spell_info[spellnum].name, obj->sbinfo[i].pages, cost);

    USE_FULL_ROUND_ACTION(ch);
    return TRUE;
  }

  /* they did not type research */
  return FALSE;
}

SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents)
  {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return (FALSE);

  do_drop(ch, argument, cmd, SCMD_DROP);

  for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents)
  {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value)
  {
    send_to_char(ch, "You are awarded for outstanding performance.\r\n");
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value, GAIN_EXP_MODE_DUMP);
    else
      increase_gold(ch, value);
  }
  return (TRUE);
}

#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[MEDIUM_STRING];
  room_rnum pet_room;
  struct char_data *pet;

  /* Gross. */
  pet_room = IN_ROOM(ch) + 1;

  if (CMD_IS("list"))
  {
    send_to_char(ch, "Available pets are:\r\n");
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room)
    {
      /* No, you can't have the Implementor as a pet if he's in there. */
      if (!IS_NPC(pet))
        continue;
      send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
    }
    return (TRUE);
  }
  else if (CMD_IS("buy"))
  {

    struct follow_type *f = NULL;

    for (f = ch->followers; f; f = f->next)
    {
      if (IS_NPC(f->follower) && MOB_FLAGGED(f->follower, MOB_MERCENARY) && AFF_FLAGGED(f->follower, AFF_CHARM))
      {
        send_to_char(ch, "You may only have one purchased follower.\r\n");
        return 1;
      }
    }

    two_arguments(argument, buf, sizeof(buf), pet_name, sizeof(pet_name));

    /* disqualifiers */
    if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet))
    {
      send_to_char(ch, "There is no such pet!\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet))
    {
      send_to_char(ch, "You don't have enough gold!\r\n");
      return (TRUE);
    }
    if (has_pet_follower(ch))
    {
      send_to_char(ch, "You can't have any more pets!\r\n");
      return (TRUE);
    }

    /* success! */
    decrease_gold(ch, PET_PRICE(pet));

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
    if (GET_LEVEL(pet) <= 10)
    {
      GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) = GET_MAX_HIT(pet) * CONFIG_SUMMON_LEVEL_1_10_HP / 100;
      GET_REAL_AC(pet) = GET_REAL_AC(pet) * CONFIG_SUMMON_LEVEL_1_10_AC / 100;
      GET_HITROLL(pet) = GET_HITROLL(pet) * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      GET_DAMROLL(pet) = GET_DAMROLL(pet) * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      pet->mob_specials.damnodice = pet->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      pet->mob_specials.damsizedice = pet->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
    }
    else if (GET_LEVEL(pet) <= 20)
    {
      GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) = GET_MAX_HIT(pet) * CONFIG_SUMMON_LEVEL_11_20_HP / 100;
      GET_REAL_AC(pet) = GET_REAL_AC(pet) * CONFIG_SUMMON_LEVEL_11_20_AC / 100;
      GET_HITROLL(pet) = GET_HITROLL(pet) * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      GET_DAMROLL(pet) = GET_DAMROLL(pet) * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      pet->mob_specials.damnodice = pet->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      pet->mob_specials.damsizedice = pet->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
    }
    else
    {
      GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) = GET_MAX_HIT(pet) * CONFIG_SUMMON_LEVEL_21_30_HP / 100;
      GET_REAL_AC(pet) = GET_REAL_AC(pet) * CONFIG_SUMMON_LEVEL_21_30_AC / 100;
      GET_HITROLL(pet) = GET_HITROLL(pet) * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      GET_DAMROLL(pet) = GET_DAMROLL(pet) * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      pet->mob_specials.damnodice = pet->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      pet->mob_specials.damsizedice = pet->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
    }
    GET_HIT(pet) = GET_MAX_HIT(pet);

    if (*pet_name)
    {
      snprintf(buf, sizeof(buf), "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = strdup(buf);

      snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
               pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = strdup(buf);
    }
    char_to_room(pet, IN_ROOM(ch));
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char(ch, "May you enjoy your pet.\r\n");
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return (TRUE);
  }

  /* All commands except list and buy */
  return (FALSE);
}

/***********************/
/* end room procedures */
/***********************/

/********************************************************************/
/******************** Object Procs    *******************************/
/********************************************************************/

/*****************************************/
/****  object procs general functions ****/
/*****************************************/

/* NOT to be confused with the weapon-spells code used in OLC, etc */

/*  This was ported to accomodate the HL objects that were imported */
void weapons_spells(const char *to_ch, const char *to_vict, const char *to_room,
                    struct char_data *ch, struct char_data *vict,
                    struct obj_data *obj, int spl)
{
  int level;

  level = GET_LEVEL(ch);

  if (level > 30)
    level = 30;

  act(to_ch, FALSE, ch, obj, vict, TO_CHAR);
  act(to_vict, FALSE, ch, obj, vict, TO_VICT);
  act(to_room, FALSE, ch, obj, vict, TO_NOTVICT);
  call_magic(ch, vict, 0, spl, 0, level, CAST_WAND);
}

/* very simple ship code system */
#define NUM_OF_SHIPS 4

// shiproom, shipobject, room_seq_start, roomseq_end
int ship_info[NUM_OF_SHIPS][4] = {
    // chionthar ferry
    {104072, 104072, 104262, 104266},
    {104073, 104072, 104262, 104266},

    // alanthor ferry
    {126429, 126429, 126423, 126428},

    // md carpet
    {120013, 120010, 120036, 120040},
};

struct obj_data *find_ship(int room)
{
  int i, j;
  struct obj_data *obj;
  for (i = 0; i < NUM_OF_SHIPS; i++)
  {
    if (room == real_room(ship_info[i][0]))
    {
      for (j = ship_info[i][2]; j <= ship_info[i][3]; j++)
      {
        for (obj = world[real_room(j)].contents; obj; obj = obj->next_content)
        {
          if (GET_OBJ_VNUM(obj) == ship_info[i][1])
            return obj;
        }
      }
      return FALSE;
    }
  }
  return FALSE;
}

void move_ship(struct obj_data *ship, int dir)
{
  int new_room = 0;
  const char *msg = 0;
  int i;
  char buf2[MAX_INPUT_LENGTH];

  if (dir < 0 || dir >= 6)
    return;

  if (!world[ship->in_room].dir_option[dir])
    return;

  new_room = world[ship->in_room].dir_option[dir]->to_room;

  if (new_room == 0)
    return;

  snprintf(buf2, sizeof(buf2), "$p floats %s.", dirs[dir]);
  act(buf2, TRUE, 0, ship, 0, TO_ROOM);

  obj_from_room(ship);
  obj_to_room(ship, new_room);

  snprintf(buf2, sizeof(buf2), "The ship moves %s.\r\n", dirs[dir]);

  if (world[ship->in_room].sector_type == SECT_ZONE_START)
    msg = "Your ship docks here.\r\n";

  for (i = 0; i < NUM_OF_SHIPS; i++)
  {
    if (GET_OBJ_VNUM(ship) == ship_info[i][1])
    {
      send_to_room(real_room(ship_info[i][0]), buf2);
      if (msg)
        send_to_room(real_room(ship_info[i][0]), msg);
    }
  }

  snprintf(buf2, sizeof(buf2), "$p floats in from the %s.", dirs[rev_dir[dir]]);
  act(buf2, TRUE, 0, ship, 0, TO_ROOM);
}

// use timer for count.
// weight is wether towards start or end.

void update_ship(struct obj_data *ship, int start, int end, int movedelay, int waitdelay)
{
  int dest = real_room(end);

  if (!ship->obj_flags.weight)
    dest = real_room(start);

  ship->obj_flags.timer--;

  if (ship->obj_flags.timer >= 0)
    return;

  ship->obj_flags.timer = movedelay;

  if (dest != ship->in_room)
    move_ship(ship, find_first_step(ship->in_room, dest));

  if (ship->in_room == dest)
  {
    // turn around ship
    ship->obj_flags.weight = !ship->obj_flags.weight;
    ship->obj_flags.timer = waitdelay;
    return;
  }
}

void ship_lookout(struct char_data *ch)
{
  struct obj_data *ship = find_ship(ch->in_room);
  if (ship == 0)
  {
    send_to_char(ch, "But you are not at a ship to look out from!\r\n");
    return;
  }
  look_at_room_number(ch, 1, ship->in_room);
}

ACMD(do_disembark)
{
  struct obj_data *ship;
  ship = find_ship(ch->in_room);

  if (!ship)
  {
    send_to_char(ch, "But you are not on any ship.\r\n");
    return;
  }
  if (world[ship->in_room].sector_type != SECT_ZONE_START)
  {
    send_to_char(ch, "You can only disembark when the ship is docked.\r\n");
    return;
  }

  // int was_in = ch->in_room;
  act("$n disembarks.", TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, ship->in_room);
  act("$n disembarks from $p.", TRUE, ch, ship, 0, TO_ROOM);
  look_at_room(ch, 0);
}

/******************************************/
/*** end object procs general functions ***/
/******************************************/

/* testing glove procs for monks, obj vnum 224 */
SPECIAL(monk_glove)
{
  if (!ch)
    return FALSE;

  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Shock damage.\r\n");
    return TRUE;
  }

  if (cmd || !vict || rand_number(0, 15))
    return FALSE;

  weapons_spells(
      "\twYour $p\tw \tWsparks\tw as you hit $N causing $M to shudder violently from the \tYshock\tw!\tn",
      "$n\tw's $p\tw \tWsparks\tw as $e hits you causing you to shudder violently from the \tYshock\tw!\tn",
      "$n\tw's $p\tw \tWsparks\tw as $e hits $N causing $M to shudder violently from the \tYshock\tw!\tn",
      ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dice(2, 8), -1, DAM_ELECTRIC, FALSE);

  return TRUE;
}

SPECIAL(monk_glove_cold)
{
  if (!ch)
    return FALSE;

  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Cold damage.\r\n");
    return TRUE;
  }

  if (cmd || !vict || rand_number(0, 15))
    return FALSE;

  weapons_spells(
      "\twYour $p\tw \tWfrosts\tw as you hit $N causing $M to shudder violently from the \tBcold\tw!\tn",
      "$n\tw's $p\tw \tWfrosts\tw as $e hits you causing you to shudder violently from the \tBcold\tw!\tn",
      "$n\tw's $p\tw \tWfrosts\tw as $e hits $N causing $M to shudder violently from the \tBcold\tw!\tn",
      ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dice(2, 8), -1, DAM_COLD, FALSE);

  return TRUE;
}

/* from homeland */
SPECIAL(spikeshield)
{
  if (!ch)
    return FALSE;

  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "On shieldpunch, procs 'spikes', on shieldblock procs "
                     "'life steal.'\r\n");
    return TRUE;
  }

  if (!argument || cmd || !vict)
    return FALSE;

  // blocking
  if (!strcmp(argument, "shieldblock") && !rand_number(0, 2))
  {
    act("\tLYour \tcshield \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N \tLand transfers it back to you.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
    act("$n's \tcshield \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N\tL.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_NOTVICT);
    act("$n's \tcshield \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom you and transfers it back to $m.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
    damage(ch, vict, 15, -1, DAM_ENERGY, FALSE); // type -1 = no dam message
    call_magic(ch, ch, 0, SPELL_CURE_LIGHT, 0, 1, CAST_SPELL);
    return TRUE;
  }

  if (!strcmp(argument, "shieldpunch"))
  {
    act("\tLYou slam your \tcshield \tLinto $N\tL\tn\r\n"
        "\tLcausing the rows of\tr spikes \tLto drive into $S body.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
    act("$n \tLslams $s \tcshield\tL into $N\tL\tn\r\n"
        "\tLcausing the rows of \trspikes\tL to drive into $S body.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_NOTVICT);
    act("$n \tLslams $s \tcshield\tL into you\tn\r\n"
        "\tLcausing the rows of \trspikes\tL to drive into your body.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
    damage(ch, vict, (dice(3, 8) + 4), -1, DAM_PUNCTURE,
           FALSE); // type -1 = no dam message
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(viperdagger)
{
  struct char_data *victim;
  int spellnum = SPELL_SLOW;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Slowness or Harm.\r\n");
    return TRUE;
  }

  victim = FIGHTING(ch);
  if (!victim || cmd)
    return FALSE;

  if (AFF_FLAGGED(victim, AFF_SLOW))
    spellnum = SPELL_HARM;

  if (rand_number(0, 23))
    return FALSE;

  weapons_spells(
      "\tLThe jeweled eyes on your dagger glow \tRred \tLas it comes alive, writhes and\tn\r\n"
      "\tLforms into a \tYgolden viper\tL.  As it coils in your hand, its mouth opens wide\tn\r\n"
      "\tLas \tWhuge fangs \tLthrust out.  It strikes out violently and bites into \tn$N\tn\r\n"
      "\tLinjecting $M with venom then recoils and transforms back into a dagger.\tn",

      "\tLThe jeweled eyes on $n's dagger glow \tRred \tLas it comes alive, writhes and\tn\r\n"
      "\tLforms into a \tYgolden viper\tL.  As it coils in $s hand, its mouth opens wide\tn\r\n"
      "\tLas \tWhuge fangs \tLthrust out.  It strikes out violently and bites into you\tn\r\n"
      "\tLinjecting you with venom then recoils and transforms back into a dagger.\tn",

      "\tLThe jeweled eyes on $n\tL's dagger glow \tRred \tLas it comes alive, writhes and\tn\r\n"
      "\tLforms into a \tYgolden viper\tL.  As it coils in $s hand, its mouth opens wide\tn\r\n"
      "\tLas \tWhuge fangs \tLthrust out.  It strikes out violently and bites into \tn$N\tn\r\n"
      "\tLinjecting $M with venom then recoils and transforms back into a dagger.\tn",
      ch, victim, (struct obj_data *)me, spellnum);
  return TRUE;
}

/* from homeland */
SPECIAL(ches)
{
  struct char_data *vict;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke haste by keyword 'ches' once per day.  Procs shock "
                     "on critical.\r\n");
    return TRUE;
  }

  vict = FIGHTING(ch);

  // proc on critical
  if (!cmd && FIGHTING(ch) && argument)
  {
    skip_spaces(&argument);
    if (!strcmp(argument, "critical"))
    {
      act("\tLAs your \tcstiletto \tLplunges deep into $N,\tn\r\n"
          "\tcs\tCp\tcar\tCk\tcs \tLbegin to \tYcrackle\tL about the hilt.  Suddenly a\tn\r\n"
          "\tBbolt of lightning \tLraces down from above to meet up\tn\r\n"
          "\tLwith the \tChilt\tL of the \tcstiletto\tL.  The enormous voltage\tn\r\n"
          "\tLflows through the weapon into $N\tn\r\n"
          "\tLcausing $S hair to stand on end.\tn",
          FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
      act("\tLAs $n's \tcstiletto \tLplunges deep into your body,\tn\r\n"
          "\tcs\tCp\tcar\tCk\tcs \tLbegin to \tYcrackle\tL about the hilt.  Suddenly a\tn\r\n"
          "\tBbolt of lightning \tLraces down from above to meet up\tn\r\n"
          "\tLwith the \tChilt\tL of the \tcstiletto\tL.  The enormous voltage\tn\r\n"
          "\tLflows through the weapon into you, \tLcausing your hair to stand on end.\tn\r\n",
          FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
      act("\tLAs $n's \tcstiletto \tLplunges deep into $N, \tn\r\n"
          "\tcs\tCp\tcar\tCk\tcs \tLbegin to \tYcrackle\tL about the hilt.  Suddenly a\tn\r\n"
          "\tBbolt of lightning \tLraces down from above to meet up\tn\r\n"
          "\tLwith the \tChilt\tL of the \tcstiletto\tL.  The enormous voltage\tn\r\n"
          "\tLflows through the weapon into $N \tn\r\n"
          "\tLcausing $S hair to stand on end.\tn\tn\r\n",
          FALSE, ch, (struct obj_data *)me, vict, TO_NOTVICT);
      damage(ch, vict, 20 + dice(2, 8), -1, DAM_ELECTRIC, FALSE); // type -1 = no dam message
      return TRUE;
    }
  }

  // haste once a day on command
  if (cmd && argument && cmd_info[cmd].command_pointer == do_say)
  {
    if (!is_wearing(ch, 128106))
      return FALSE;
    skip_spaces(&argument);
    if (!strcmp(argument, "ches"))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "Nothing happens.\r\n");
        return TRUE;
      }
      act("\tcAs you quietly speak the word of power to your stiletto\tn\r\n"
          "\tcthe aquamarine on the hilt begins to fizzle and pop. The\tn\r\n"
          "\tcnoise continues to culminate until there is a loud crack.\tn\r\n"
          "\tcThe hilt flashes bright for a split second before a sharp\tn\r\n"
          "\tcelectric shock flows up your hand into your body.  You\tn\r\n"
          "\tcsuddenly feel your heart begin to race REALLY fast!\tn",
          FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
      act("\tC$n whispers to $s $p",
          FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);

      call_magic(ch, ch, 0, SPELL_HASTE, 0, 30, CAST_POTION);
      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 12;
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(courage)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke by 'courage' once a week.\r\n");
    return TRUE;
  }

  if (cmd && argument && cmd_info[cmd].command_pointer == do_say)
  {
    if (!is_wearing(ch, 139251) && !is_wearing(ch, 139250))
      return FALSE;

    skip_spaces(&argument);
    if (!strcmp(argument, "courage"))
    {

      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "Nothing happens.\r\n");
        return TRUE;
      }

      act("$n \tLinvokes $s \tygolden mace\tn.", FALSE, ch, 0, 0, TO_ROOM);
      act("\tLYou invoke your \tygolden mace\tn.", FALSE, ch, 0, 0, TO_CHAR);

      call_magic(ch, ch, 0, SPELL_PRAYER, 0, GET_LEVEL(ch), CAST_POTION);
      call_magic(ch, ch, 0, SPELL_MASS_ENHANCE, 0, GET_LEVEL(ch), CAST_POTION);

      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 12 * 5;
      return TRUE;
    }
    return FALSE;
  }
  if (cmd)
    return FALSE;

  struct char_data *vict = FIGHTING(ch);

  if (!vict)
    return FALSE;

  int power = 25;
  if (GET_OBJ_VNUM(((struct obj_data *)me)) == 139250)
    power = 15;
  if (rand_number(0, power))
  {
    // TODO, cool damage proc here..
  }
  return TRUE;
}

/* from homeland */
SPECIAL(flamingwhip)
{
  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Fire Damage.\r\n");
    return TRUE;
  }

  if (!ch || cmd || !vict || rand_number(0, 16))
    return FALSE;

  weapons_spells(
      "\trYour $p \trlashes out with infernal \tRfire\tr, burning $N\tr badly!\tn",
      "\tr$n\tr's $p \trlashes out with infernal \tRfire\tr, burning YOU\tr badly!\tn",
      "\tr$n\tr's $p \trlashes out with infernal \tRfire\tr, burning $N\tr badly!\tn",
      ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dice(6, 4), -1, DAM_FIRE, FALSE); // type -1 = no dam message

  return TRUE;
}

/* Helmblade vnum 121207
  only procs against evil,  a minor heal on wielder and a dispel_evil.
 */
SPECIAL(helmblade)
{
  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc vs Evil: Cure Serious or Dispel Evil.\r\n");
    return TRUE;
  }

  if (!ch || cmd || !vict)
    return FALSE;
  if (!IS_EVIL(vict))
    return FALSE;

  switch (rand_number(0, 40))
  {
  case 0:
    weapons_spells(
        "\tWThe \tYHOLY\tW power of \tYHelm\tW flows through your body, cleaning you of \tLevil\tW and nourishing you.\tn",
        "\tWThe \tYHOLY\tW power of \tYHelm\tW flows through $n's\tW body, cleaning $m of \tLevil\tW and nourishing $m.\tn",
        "\tWThe \tWHOLY\tW power of \tYHelm\tW flows through $n's\tW body, cleaning $m of \tLevil\tW and nourishing $m.\tn",
        ch, vict, (struct obj_data *)me, 0);
    call_magic(ch, ch, 0, SPELL_CURE_SERIOUS, 0, GET_LEVEL(ch), CAST_POTION);
    return TRUE;
  case 1:
    weapons_spells(
        "\tWThe power of \tYHelm\tW guides and strengthens thy hand, dispelling the \tLevil\tW of the world from $N.\tn",
        "\tWThe power of \tYHelm\tW guides and strengthen the hand of $n, dispelling the \tLevil\tW of the world from YOU!.\tn",
        "\tWThe power of \tYHelm\tW guides and strengthen the hand of $n, dispelling the \tLevil\tW of the world from $N.\tn",
        ch, vict, (struct obj_data *)me, SPELL_DISPEL_EVIL);
    return TRUE;
  default:
    return FALSE;
  }
}

/* from homeland */

/* obj - 113898 has special proc when combined with 113897 */
SPECIAL(flaming_scimitar)
{
  struct char_data *vict = FIGHTING(ch);
  struct obj_data *weepan = (struct obj_data *)me;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???");
    return TRUE;
  }

  if (cmd || !vict || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (GET_CLASS(ch) != CLASS_RANGER)
  {
    act("\tWA \trsearing hot\tW pain travels up your arm as $p \tWrips itself from your unworthy grasp!\tn", FALSE, ch,
        (struct obj_data *)me, NULL, TO_CHAR);
    act("\tW$n \tWscreams in pain as $p\tW rips itself from $s grasp!\tn", FALSE, ch, (struct obj_data *)me,
        NULL, TO_ROOM);
    obj_to_room(unequip_char(ch, weepan->worn_on), ch->in_room);
    GET_HIT(ch) = 0;
    return TRUE;
  }

  if (!rand_number(0, 22))
  {
    if (!is_wearing(ch, 113897))
    {
      weapons_spells(
          "\trYour longsword begins to \tLvibrate violently\tr in your hands as it "
          "forces your arm skyward and flashes with intense magical energy.\tn",

          "\tr$n's \trlongsword begins to \tLvibrate violently\tr in $s hands as it "
          "forces $s arm skyward and flashes with intense magical energy.\tn",

          "\tr$n's \trlongsword begins to \tLvibrate violently\tr in $s hands as it "
          "forces $s arm skyward and flashes with intense magical energy.\tn",
          ch, vict, (struct obj_data *)me, SPELL_FLAME_STRIKE);
      return TRUE;
    }
    else
    {
      weapons_spells("\tLIn a \trflurry \tLof movement, your swords cross over one another, "
                     "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
                     "powers combine, bringing down a violent storm of \trFIRE\tL upon all.\tn",

                     "\tLIn a \trflurry \tLof movement, \tn$n's\tL swords cross over one another, "
                     "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
                     "powers combine, bringing down a violent storm of \trFIRE\tL upon all.\tn",

                     "\tLIn a \trflurry \tLof movement, \tn$n's\tL swords cross over one another, "
                     "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
                     "powers combine, bringing down a violent storm of \trFIRE\tL upon all.\tn",
                     ch, vict, (struct obj_data *)me, SPELL_FIRE_STORM);
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */

/* obj - 113897 has special proc when combined with 113898 */
SPECIAL(frosty_scimitar)
{
  struct char_data *vict = FIGHTING(ch);
  struct obj_data *weepan = (struct obj_data *)me;

  if (!ch)
    return FALSE;
  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???");
    return TRUE;
  }
  if (cmd || !vict || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (GET_CLASS(ch) != CLASS_RANGER)
  {
    act("\tWA \trsearing hot\tW pain travels up your arm as $p \tWrips itself from your unworthy grasp!\tn", FALSE, ch,
        (struct obj_data *)me, NULL, TO_CHAR);
    act("\tW$n \tWscreams in pain as $p\tW rips itself from $s grasp!\tn", FALSE, ch, (struct obj_data *)me,
        NULL, TO_ROOM);
    obj_to_room(unequip_char(ch, weepan->worn_on), ch->in_room);
    GET_HIT(ch) = 0;
    return TRUE;
  }

  if (!rand_number(0, 18))
  {
    if (!is_wearing(ch, 113898))
    {
      weapons_spells(
          "\tcYour scimitar begins to \tWvibrate violently\tc in your hands as it "
          "forces your arm skyward and flashes with intense magical energy.\tn",
          "\tc$n's \tcscimitar begins to \tWvibrate violently\tc in $s hands as it "
          "forces $s arm skyward and flashes with intense magical energy.\tn",
          "\tc$n's \tcscimitar begins to \tWvibrate violently\tc in $s hands as it "
          "forces $s arm skyward and flashes with intense magical energy.\tn",
          ch, vict, (struct obj_data *)me, SPELL_CONE_OF_COLD);
      return TRUE;
    }
    else
    {
      weapons_spells(
          "\tLIn a \trflurry \tLof movement, your swords cross over one another, "
          "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
          "powers combine, bringing down a violent storm of \tCICE\tL upon all.\tn",

          "\tLIn a \trflurry \tLof movement, \tn$n\tL's swords cross over one another, "
          "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
          "powers combine, bringing down a violent storm of \tCICE\tL upon all.\tn",

          "\tLIn a \trflurry \tLof movement, \tn$n\tL's swords cross over one another, "
          "resulting in a \tCbrilliant \tWflash \tLof \tbmagical energy \tLas their "
          "powers combine, bringing down a violent storm of \tCICE\tL upon all.\tn",
          ch, vict, (struct obj_data *)me, SPELL_ICE_STORM);
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(disruption_mace)
{
  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Flame Strike.\r\n");
    return TRUE;
  }

  if (!ch || cmd || !vict || rand_number(0, 20))
    return FALSE;

  weapons_spells(
      "\trYour\tn $p \tris engulfed in flames!\tn",
      "$n's $p \tris engulfed in flames!\tn",
      "$n's $p \tris engulfed in flames!\tn",
      ch, vict, (struct obj_data *)me, SPELL_FLAME_STRIKE);
  return TRUE;
}

/*
// Does not seem to be used yet, was attached to non existant objects
SPECIAL(forest_idol)
{
  if( cmd )
    return FALSE;
  if( !ch )
    return FALSE;

  struct obj_data *obj = (struct obj_data *)me;

    if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???.\r\n");
    return TRUE;
  }

  if( obj->carried_by != ch )
    return FALSE;

  if( GET_CLASS(ch) == CLASS_ANTIPALADIN )
    return FALSE;

  send_to_char("\tgYour idol melts in your hands.\tn\r\n", ch );
  obj_from_char(obj);
  obj_to_room(obj, ch->in_room );
}
 */

/* from homeland */
SPECIAL(haste_bracers)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Haste once per week on keyword 'quicksilver.'\r\n");
    return TRUE;
  }

  if ((cmd && argument && CMD_IS("say")) || (cmd && argument && CMD_IS("'")))
  {
    if (!is_wearing(ch, 138415))
      return FALSE;

    skip_spaces(&argument);
    if (!strcmp(argument, "quicksilver"))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "\tWYour \tcbracers\tW have not regained their \tcessence\tW.\tn\r\n");
        return TRUE;
      }
      act("\tWYou whisper softly to your bracers.\tn\r\n"
          "\tWYour $p \tcglow with a blue aura.\tn\r\n"
          "\tWThe world \tcslows \tWto a \tCc\tcr\tCa\tcw\tCl\tW.\tn\r\n",
          FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
      act("\tW$n whispers softly to $s bracers.\tn\r\n"
          "\tW$n's $p \tcglow with a blue aura.\tn\r\n"
          "\tW$n moves with \tCl\tci\tCg\tch\tCt\tW speed.\tn\r\n",
          FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);
      call_magic(ch, ch, 0, SPELL_HASTE, 0, 30, CAST_SPELL);
      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 84;
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(xvim_normal)
{
  struct char_data *tch = NULL, *vict = FIGHTING(ch);
  int dam, i, num = dice(1, 4);

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???");
    return TRUE;
  }

  if (!cmd && vict)
    switch (rand_number(0, 40))
    {
    case 0:
    case 1:
      weapons_spells(
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of your body\r\n"
          "in a flash of \tmhatred\tL. Your arms begin to move with blinding\r\n"
          "speed as your accursed weapon begins to rend and tear apart\r\n"
          "the \tyflesh\tL of $N\tL in a spray of \trBLOOD\tL.\tn",
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of \tg$n's\tL body\r\n"
          "in a flash of \tmhatred\tL. $s arms begin to move with blinding\r\n"
          "speed as $s accursed weapon begins to rend and tear apart\r\n"
          "YOUR \tyflesh\tL in a spray of \trBLOOD\tL.\tn",
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of \tg$n's\tL body\r\n"
          "in a flash of \tmhatred\tL. $s arms begin to move with blinding\r\n"
          "speed as $s accursed weapon begins to rend and tear apart\r\n"
          "the \tyflesh\tL of $N\tL in a spray of \trBLOOD\tL.\tn",
          ch, vict, (struct obj_data *)me, 0);
      for (i = 0; i < num; i++)
        if (vict)
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return TRUE;
    case 2:
      if (!rand_number(0, 100))
      {
        weapons_spells(
            "\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
            "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
            "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
            "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
            "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
            "dark figure fades into nothingness, laughing all the while.\tn\r\n",
            "\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
            "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
            "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
            "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
            "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
            "dark figure fades into nothingness, laughing all the while.\tn\r\n",
            "\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
            "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
            "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
            "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
            "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
            "dark figure fades into nothingness, laughing all the while.\tn\r\n",
            ch, vict, (struct obj_data *)me, 0);
        dam = rand_number(100, 200);
        if (dam > GET_HIT(vict))
        {
          GET_HIT(vict) = -13;
          change_position(vict, POS_DEAD);
          update_pos(vict);
        }
        else
        {
          GET_HIT(vict) -= dam;
          change_position(vict, POS_SITTING);
        }
        for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
          USE_MOVE_ACTION(tch);
        return TRUE;
      }
      return FALSE;
    case 3:
    case 4:
    case 5:
    case 6:
      if (GET_HIT(ch) < GET_MAX_HIT(ch))
      {
        act("\tLYour avenger \tgglows\tL in your hands, and your \trblood\tL seems to flow back\tn\r\n"
            "\tLinto your wounds, \tWhealing\tL them by the unholy power of \tGXvim\tL.\tn",
            FALSE, ch, 0, 0, TO_CHAR);
        act("\tw$n\tL's avenger \tgglows\tL in $s hands, and $s \trblood\tL seems to flow back\tn\r\n"
            "\tLinto $s wounds, \tWhealing\tL them by the unholy power of \tGXvim\tL.\tn",
            FALSE, ch, 0, 0, TO_ROOM);
        GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + dice(10, 10));
        return TRUE;
      }
      return FALSE;
      break;
    default:
      return FALSE;
    }
  return FALSE;
}

/* from homeland */
SPECIAL(xvim_artifact)
{
  struct char_data *tch = NULL, *vict = FIGHTING(ch), *pet = NULL;
  int num = (dice(1, 4) + 2), dam, i = 0;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???");
    return TRUE;
  }

  if (!cmd && vict)
  {
    switch (rand_number(0, 35))
    {
    case 0:
    case 1:
      weapons_spells(
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of your body\r\n"
          "in a flash of \tmhatred\tL. Your arms begin to move with blinding\r\n"
          "speed as your accursed weapon begins to rend and tear apart\r\n"
          "the \tyflesh\tL of $N\tL in a spray of \trBLOOD\tL.\tn",
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of \tg$n's\tL body\r\n"
          "in a flash of \tmhatred\tL. $s arms begin to move with blinding\r\n"
          "speed as $s accursed weapon begins to rend and tear apart\r\n"
          "YOUR \tyflesh\tL in a spray of \trBLOOD\tL.\tn",
          "\tLThe \tGUNHOLY\tL power of \tgIy\tGach\tgtu X\tGvi\tgm \tLtakes hold of \tg$n's\tL body\r\n"
          "in a flash of \tmhatred\tL. $s arms begin to move with blinding\r\n"
          "speed as $s accursed weapon begins to rend and tear apart\r\n"
          "the \tyflesh\tL of $N\tL in a spray of \trBLOOD\tL.\tn",
          ch, vict, (struct obj_data *)me, 0);
      for (i = 0; i < num; i++)
        if (vict)
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return TRUE;
    case 2:
      if (!rand_number(0, 100))
      {
        weapons_spells(
            "\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
            "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
            "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
            "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
            "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
            "dark figure fades into nothingness, laughing all the while.\tn\r\n",
            "\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
            "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
            "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
            "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
            "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
            "dark figure fades into nothingness, laughing all the while.\tn\r\n",
            "\tf\tWBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM!!!\tn\r\n"
            "\r\n\r\n\tmAfter a blinding flash the entire area becomes enveloped\r\n"
            "in \tLdarkness\tm. With your limited vision you see an extremely\r\n"
            "tall man plunges a large scimitar into $N's\tm chest and cackles\r\n"
            "as $S \tMlife force\tm is stolen away. As the \tLdarkness\tm fades the\r\n"
            "dark figure fades into nothingness, laughing all the while.\tn\r\n",
            ch, vict, (struct obj_data *)me, 0);
        dam = rand_number(100, 200) + 50;
        if (dam > GET_HIT(vict))
        {
          GET_HIT(vict) = -13;
          change_position(vict, POS_DEAD);
        }
        else
        {
          GET_HIT(vict) -= dam;
          change_position(vict, POS_SITTING);
        }
        for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
          USE_MOVE_ACTION(tch);
        return TRUE;
      }
      return FALSE;
    case 3:
    case 4:
    case 5:
      if (GET_HIT(ch) < GET_MAX_HIT(ch))
      {
        act("\tLYour avenger \tgglows\tL in your hands, and your \trblood\tL seems to flow back\tn\r\n"
            "\tLinto your wounds, \tWhealing\tL them by the unholy power of \tGXvim\tL.\tn",
            FALSE, ch, 0, 0, TO_CHAR);
        act("\tw$n\tL's avenger \tgglows\tL in $s hands, and $s \trblood\tL seems to flow back\tn\r\n"
            "\tLinto $s wounds, \tWhealing\tL them by the unholy power of \tGXvim\tL.\tn",
            FALSE, ch, 0, 0, TO_ROOM);
        GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + dice(11, 10));
        return TRUE;
      }
      return FALSE;
      break;
    default:
      return FALSE;
    }
  }

  skip_spaces(&argument);

  if (!is_wearing(ch, 100501))
    return FALSE;

  if (!strcmp(argument, "nightmare") && CMD_IS("whisper"))
  {
    if (mob_index[real_mobile(100505)].number < 1)
    {
      act("\tLAs you whisper '\tmnightmare\tL' to your \trsword\tL, a \twthick\tW fog"
          "\tLforms in the area\r\naround you.  When it finally fades, the "
          "horrid visage of a \tmNightmare\tL\r\nstands before you.\tn",
          1, ch, 0, FIGHTING(ch), TO_CHAR);
      act("\tLAs $n whispers something to $s \trsword\tL, a \twthick\tW fog\r\n"
          "\tLforms in the area around you.  When it finally fades, the "
          "horrid visage\r\nof a \tmNightmare\tL stands before you.\tn",
          1, ch, 0, FIGHTING(ch), TO_ROOM);
      pet = read_mobile(real_mobile(100505), REAL);
      char_to_room(pet, ch->in_room);
      add_follower(pet, ch);
      GET_MAX_HIT(pet) = GET_HIT(ch) = GET_LEVEL(ch) * 10 + dice(GET_LEVEL(ch), 6);
      SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
      return TRUE;
    }
    else
    {
      send_to_char(ch, "\tLAs you whisper '\tmnightmare\tL' to your \trsword\tL, nothing seems to happen.\tn\r\n");
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(dragonbone_hammer)
{
  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Ice Dagger.\r\n");
    return TRUE;
  }

  if (!ch || cmd || !vict || rand_number(0, 10))
    return FALSE;

  weapons_spells(
      "Your $p \tCvibrates violently!\tn",
      "$n's $p \tCvibrates violently!\tn",
      "$n's $p \tCvibrates violently!\tn",
      ch, vict, (struct obj_data *)me, SPELL_ICE_DAGGER);
  return TRUE;
}

/* from homeland */
SPECIAL(prismorb)
{
  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Prismatic Spray.\r\n");
    return TRUE;
  }

  if (!ch || cmd || !vict || rand_number(0, 25))
    return FALSE;

  weapons_spells(
      "\tWYour \tn$p \tWpulsates violently.\tn",
      "\tW$n\tW's \tn$p \tWpulsates violently.\tn",
      "\tW$n\tW's \tn$p \tWpulsates violently.\tn",
      ch, vict, (struct obj_data *)me, SPELL_PRISMATIC_SPRAY);

  return TRUE;
}

/* from homeland */
SPECIAL(dorfaxe)
{
  struct char_data *vict = FIGHTING(ch);
  int num = 18;
  int dam = 0;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc vs Evil: Clangeddins Wrath.\r\n");
    return TRUE;
  }

  if (!ch || cmd || !vict)
    return FALSE;

  if (GET_RACE(ch) == RACE_DWARF)
    num = 12;

  if (!IS_GOOD(ch))
    return FALSE;
  if (!IS_EVIL(vict))
    return FALSE;
  if (rand_number(0, num))
    return FALSE;

  dam = rand_number(6, 12);

  if (dam > GET_HIT(vict))
    dam = GET_HIT(vict);

  weapons_spells(
      "\tWAs $p impacts with \tn$N\tW, a mortal enemy of\r\n"
      "\tWany righteous dwarf, the great god \tYClangeddin\tW infuses it,\r\n"
      "\tWand strikes with great power into \tn$M.\tn",

      "\tWAs $p impacts with YOU, a mortal enemy of\r\n"
      "\tWany righteous dwarf, the great god \tYClangeddin\tW infuses it,\r\n"
      "\tWand strikes with great power into YOU!\tn",

      "\tWAs $p impacts with \tn$N\tW, a mortal enemy of\r\n"
      "\tWany righteous dwarf, the great god \tYClangeddin\tW infuses it,\r\n"
      "\tWand strikes with great power into \tn$M.\tn",
      ch, vict, (struct obj_data *)me, 0);

  damage(ch, vict, dam, -1, DAM_HOLY, FALSE); // type -1 = no dam message
  return TRUE;
}

/* from homeland */
SPECIAL(acidstaff)
{
  struct char_data *victim = NULL;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Acid Arrow.\r\n");
    return TRUE;
  }

  if (!ch)
    return FALSE;

  victim = FIGHTING(ch);

  if (!victim || cmd)
    return FALSE;

  if (rand_number(0, 15))
    return FALSE;

  weapons_spells(
      "\tLYour staff vibrates and hums then glows \tGbright green\tL.\tn\r\n"
      "\tLThe tiny black dragons on your staff come alive and roar loudly\tn\r\n"
      "\tLthen spew forth vile \tgacid\tL at $N.\tn",

      "\tL$n\tL's staff vibrates and hums then glows \tGbright green\tL.\tn\r\n"
      "\tLThe tiny black dragons on $s staff come alive and roar loudly\tn\r\n"
      "\tLthen spew forth vile \tgacid\tL at you.\tn",

      "\tL$n\tL's staff vibrates and hums then glows \tGbright green\tL.\tn\r\n"
      "\tLThe tiny black dragons on $s staff come alive and roar loudly\tn\r\n"
      "\tLthen spew forth vile \tgacid\tL at \tn$N.\tn",
      ch, victim, (struct obj_data *)me, SPELL_ACID_ARROW);
  return TRUE;
}

/* from homeland */
SPECIAL(sarn)
{
  struct char_data *vict = FIGHTING(ch);
  int num = 18;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Harm, more effective for Duergar.\r\n");
    return TRUE;
  }

  if (!ch || cmd || !vict)
    return FALSE;

  if (GET_RACE(ch) == RACE_DUERGAR)
    num = 12;

  if (!IS_EVIL(ch))
    return FALSE;
  if (rand_number(0, num))
    return FALSE;

  weapons_spells(
      "\tLThe power of \twLad\tWu\twgu\tWe\twr\tL guides thine hand and \trstr\tRe\trngth\tRe\trns\tL it.\tn\r\n"
      "\tLAs the \traxe\tL impacts with \tn$N\tL, \twd\tWi\twv\tWi\twne\tL power is unleashed.\tn",

      "\tLThe power of \twLad\tWu\twgu\tWe\twr\tL guides $n's hand and \trstr\tRe\trngth\tRe\trns\tL it.\tn\r\n"
      "\tLAs the \traxe\tL impacts with YOU, \twd\tWi\twv\tWi\twne\tL power is unleashed.\tn",

      "\tLThe power of \twLad\tWu\twgu\tWe\twr\tL guides \tn$n\tL's hand and \trstr\tRe\trngth\tRe\trns\tL it.\tn\r\n"
      "\tLAs the \traxe\tL impacts with \tn$N\tL, \twd\tWi\twv\tWi\twne\tL power is unleashed.\tn",
      ch, vict, (struct obj_data *)me, SPELL_HARM);

  return TRUE;
}

/* from homeland */
SPECIAL(purity)
{
  int dam = 0;
  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Holy Light - in combat randomly inflict 2d24 holy damage.\r\n");
    return TRUE;
  }

  if (!ch || cmd || !vict || rand_number(0, 20))
    return FALSE;

  dam = dice(2, 24);
  if (dam < GET_HIT(vict) + 10)
  {
    act("\twThe head of your $p starts to \tYglow \twwith a \tWbright white light\tw.\r\n"
        "A beam of concetrated \tWholiness \twshoots towards $N.\r\n"
        "The \tWlightbeam \twsurrounds $N who howls in pain and fear.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
    act("$n's $p \twstarts to \tYglow \twwith a \tWbright white light\tw.\r\n"
        "A beam of concentrated \tWholiness \twshoots towards $N.\r\n"
        "The \tWlightbeam \twsurrounds $N who howls in pain and fear.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_NOTVICT);
    act("$n's $p \twstarts to \tYglow \twwith a \tWbright white light\tw.\r\n"
        "A beam of concentrated \tWholiness \twshoots towards you.\r\n"
        "The \tWlightbeam \twsurrounds you and you howl in pain and fear.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
  }
  else
  {
    act("\twThe head of your $p starts to \tYglow \twwith a \tWbright white light\t.w\r\n"
        "A beam of concentrated \tWholiness \twshoots towards $N.\r\n"
        "The \tWlightbeam \twburns a hole right through $N who falls lifeless to the ground.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_CHAR);
    act("$n's $p \twstarts to \tYglow \twwith a \tWbright white light\tw.\r\n"
        "A beam of concentrated \tWholiness \twshoots towards you.\r\n"
        "The \tWlightbeam \twburns a hole right through you and you fall lifeless to the ground.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_VICT);
    act("$n's $p \twstarts to \tYglow \twwith a \tWbright white light\tw.\r\n"
        "A beam of concentrated \tWholiness \twshoots towards $N.\r\n"
        "The \tWlightbeam \twburns a hole right through $N who falls lifeless to the ground.\tn",
        FALSE, ch, (struct obj_data *)me, vict, TO_NOTVICT);

    call_magic(ch, vict, 0, SPELL_BLINDNESS, 0, GET_LEVEL(ch), CAST_SPELL);
  }
  damage(ch, vict, dam, -1, DAM_HOLY, FALSE); // type -1 = no dam message
  return TRUE;
}

/* from homeland */
SPECIAL(etherealness)
{
  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Slow.\r\n");
    return TRUE;
  }

  if (!ch || cmd || !vict || rand_number(0, 15))
    return FALSE;

  weapons_spells(
      "\twWaves of \tWghostly \twenergy starts to flow from your $p.",
      "\twWaves of \tWghostly \twenergy starts to flow from $n's $p.",
      "\twWaves of \tWghostly \twenergy starts to flow from $n's $p.",
      ch, vict, (struct obj_data *)me, SPELL_SLOW);

  return TRUE;
}

/* from homeland */
SPECIAL(star_circlet)
{
  struct obj_data *circlet = (struct obj_data *)me;

  if (!circlet)
    return FALSE;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc in combat: Divine Arcane Recall\r\n");
    return TRUE;
  }

  if (DEBUGMODE)
    send_to_char(ch, "DEBUG MARK 1\r\n");

  if (!is_wearing(ch, 132104))
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "DEBUG MARK 2\r\n");

  if (!FIGHTING(ch))
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "DEBUG MARK 3\r\n");

  if (cmd)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "DEBUG MARK 4\r\n");

  if (rand_number(0, 5))
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "DEBUG MARK 5\r\n");

  if (star_circlet_proc(ch))
  {
    act("\twIn an instant \tYflare of power\tw, the \tBdisplaced stars\tw encircling $p draw in "
        "\tYarcane and divine energy\tw from the planes directly into your head!\tn",
        FALSE, ch, circlet, NULL, TO_CHAR);
    act("\twIn an instant \tYflare of power\tw, the \tBdisplaced stars\tw encircling \tn$n's\tn $p"
        "\tw draw in \tYarcane and divine energy\tw from the planes directly into $s head!\tn",
        FALSE, ch, circlet, NULL, TO_NOTVICT);
  }
  else
  {
    if (DEBUGMODE)
      send_to_char(ch, "DEBUG MARK 6\r\n");
  }

  return TRUE;
}

/* from homeland */
SPECIAL(greatsword)
{
  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Silver Flames.\r\n");
    return TRUE;
  }

  if (!ch || cmd || !vict || rand_number(0, 20))
    return FALSE;

  int dam = 30 + dice(5, 5);

  if (dam > GET_HIT(vict))
    dam = GET_HIT(vict);

  if (dam < 21)
    return FALSE;

  weapons_spells(
      "\tCSilvery flames shoots from your $p\tC towards $N\tC.\r\nThe flames sear and burn $N\tC who screams in pain.\tn",
      "\tCSilvery flames shoot from $n's $p\tC towards you\tC.\r\nThe flames sear and burn you and you scream in pain.\tn",
      "\tCSilvery flames shoot from $n's $p\tC towards $N\tC.\r\nThe flames sear and burn $N\tC who screams in pain.\tn",
      ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dam, -1, DAM_ENERGY, FALSE); // type -1 = no dam message
  return TRUE;
}

/* from homeland */
SPECIAL(fog_dagger)
{
  struct char_data *i, *vict;
  struct affected_type af;
  struct affected_type af2;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Procs paralysis on backstab, whisper 'haze' for foggy"
                     " cloud.\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 115003))
    return FALSE;

  if (!ch || !cmd)
    return FALSE;

  skip_spaces(&argument);

  // First check if they whispered haze
  if (!strcmp(argument, "haze") && CMD_IS("whisper") && (vict = FIGHTING(ch)))
  {
    if ((GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0))
    {
      act("\tLYou whisper a word to your\tn $p,\tL and nothing happens.\tn", FALSE, ch,
          (struct obj_data *)me, 0, TO_CHAR);
      return TRUE;
    }
    else
    {
      weapons_spells(
          "\tLA hazy cloud is emitted from your\tn $p\tL, and enshrouds \tn$N \tLin a dark mist!\tn",
          "\tLA hazy cloud is emitted from $n's\tn $p\tL, and enshrouds \tn$N \tLin a dark mist!\tn",
          "\tLA hazy cloud is emitted from $n's\tn $p\tL, and enshrouds you in a dark mist!\tn",
          ch, vict, (struct obj_data *)me, 0);

      // Sets the vict blind for 1-3 rounds
      if (!AFF_FLAGGED(vict, AFF_BLIND) && can_blind(vict))
      {
        new_affect(&af);
        af.spell = SPELL_BLINDNESS;
        SET_BIT_AR(af.bitvector, AFF_BLIND);
        af.duration = dice(1, 3);
        affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
      }
      for (i = world[vict->in_room].people; i; i = i->next_in_room)
      {
        if (FIGHTING(i) == vict)
        {
          stop_fighting(i);
          act("\tLThe haze around \tn$N \tLprevents you from touching \tn$M",
              FALSE, i, 0, vict, TO_CHAR);
        }
      }

      stop_fighting(vict);
      clearMemory(vict);

      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 24;
      return TRUE;
    }
    // Now check if they are trying to backstab
  }
  else if (CMD_IS("backstab") &&
           (vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
  {
    if (perform_backstab(ch, vict))
    {
      if (FIGHTING(ch) == vict &&
          !AFF_FLAGGED(vict, AFF_PARALYZED) &&
          !rand_number(0, 9))
      {
        new_affect(&af2);
        af2.spell = SPELL_HOLD_PERSON;
        SET_BIT_AR(af2.bitvector, AFF_PARALYZED);
        af2.duration = dice(1, 2);
        affect_join(vict, &af2, TRUE, FALSE, FALSE, FALSE);
      }
    }
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(tyrantseye)
{
  struct char_data *vict = FIGHTING(ch), *i = NULL, *in = NULL;

  if (!ch || cmd || !vict)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "???\r\n");
    return TRUE;
  }

  if (!IS_NPC(ch))
  {
    act("\tLA \tWbolt \tLof \tGgreen \tLLighting slams into $n from above!\tn",
        FALSE, ch, 0, 0, TO_ROOM);
    act("\tLA \tWbolt \tLof \tGgreen \tLLighting slams into you from above!\tn",
        FALSE, ch, 0, 0, TO_CHAR);
    die(ch, ch);
  }

  switch (rand_number(0, 35))
  {
  case 0:
  case 1:
    weapons_spells(
        "IF YOU SEE THIS, TALK TO A STAFF MEMBER",
        "\tgFzoul \tLturns his wicked gaze toward's you and utters arcane "
        "words to his \tgscepter\tL. You are blinded by a brilliant \tWFLASH\tn "
        "\tLas a \tpbolt\tL of crackling \tGgreen energy\tL is hurled toward you!\tn",
        "\tgFzoul \tLturns his wicked gaze toward's $N \tLand utters arcane "
        "words to his \tgscepter\tL. $N \tLis blinded by a brilliant \tWFLASH\tn "
        "\tLas a \tpbolt\tL of crackling \tGgreen energy\tL is hurled toward $M!\tn",
        ch, vict, (struct obj_data *)me, 0);
    call_magic(ch, vict, 0, SPELL_MISSILE_STORM, 0, 30, CAST_SPELL);
    call_magic(ch, vict, 0, SPELL_BLINDNESS, 0, 30, CAST_SPELL);
    call_magic(ch, vict, 0, SPELL_SLOW, 0, 30, CAST_SPELL);
    return TRUE;
  case 10:
    weapons_spells(
        "\tGIF YOU SEE THIS TALK TO A GOD",
        "\tGFzoul's \tLscepter springs to life in a \tWFLASH\tL, bathing your "
        "party in a misty \tGgreen glow! \tLYou scream in agony as you "
        "begin to lose control of your body!\tn",
        "\tGFzoul's \tLscepter springs to life in a \tWFLASH\tL, bathing the "
        "room in a misty \tGgreen glow! \tLYou scream in agony as you "
        "begin to lose control of your body!\tn",
        ch, vict, (struct obj_data *)me, 0);

    for (i = character_list; i; i = in)
    {
      in = i->next;
      if (!IS_NPC(i) || IS_PET(i))
      {
        call_magic(ch, i, 0, SPELL_CURSE, 0, 30, CAST_SPELL);
        call_magic(ch, i, 0, SPELL_POISON, 0, 30, CAST_SPELL);
      }
      return TRUE;
    }
  default:
    return FALSE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(spiderdagger)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Procs darkfire in combat and by Invoking Lloth she protects any drow.\r\n");
    return TRUE;
  }

  struct char_data *vict;

  vict = FIGHTING(ch);

  if (!cmd && vict && !rand_number(0, 9))
  {
    // proc darkfire
    weapons_spells(
        "\tLYour $p\tL starts to \tcglow\tL as it pierces \tn$N!",
        "$n\tL's $p\tL starts to \tcglow\tL as it pierces YOU!",
        "$n\tL's $p\tL starts to \tcglow\tL as it pierces \tn$N!",
        ch, vict, (struct obj_data *)me, SPELL_NEGATIVE_ENERGY_RAY);
    return TRUE;
  }
  // cloak of dark power once day on command
  if (cmd && argument && cmd_info[cmd].command_pointer == do_say)
  {
    if (!is_wearing(ch, 135535))
      return FALSE;

    skip_spaces(&argument);
    if (!strcmp(argument, "lloth"))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "Nothing happens.\r\n");
        return TRUE;
      }
      if (GET_RACE(ch) != RACE_DROW)
      {
        send_to_char(ch, "Nothing happens.\r\n");
        return TRUE;
      }
      send_to_char(ch, "\tLYou invoke \tmLloth\tw.\tn\r\n");
      act("\tw$n raises $s $p \tw high and calls on \tmLloth.\tn",
          FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);
      call_magic(ch, ch, 0, SPELL_NON_DETECTION, 0, 30, CAST_POTION);
      call_magic(ch, ch, 0, SPELL_CIRCLE_A_GOOD, 0, 30, CAST_POTION);

      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 24;
      return TRUE;
    }
  }

  return FALSE;
}

/* from homeland */
SPECIAL(sparksword)
{
  if (!ch)
    return FALSE;

  struct char_data *vict = FIGHTING(ch);

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Shock damage.\r\n");
    return TRUE;
  }

  if (cmd || !vict || rand_number(0, 20))
    return FALSE;

  weapons_spells(
      "\twYour $p\tw's blade \tWsparks\tw as you hit $N "
      "\twwith your slash, causing $M to shudder violently from the \tYshock\tw!\tn",
      "$n\tw's $p\tw's blade \tWsparks\tw as $e hits you "
      "with $s slash, causing you to shudder violently from the \tYshock\tw!\tn",
      "$n\tw's $p\tw's blade \tWsparks\tw as $e hits $N "
      "\twwith $s slash, causing $M to shudder violently from the \tYshock\tw!\tn",
      ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dice(9, 3), -1, DAM_ELECTRIC, FALSE);

  return TRUE;
}

/* from homeland */
SPECIAL(nutty_bracer)
{
  struct char_data *vict = NULL, *victim = NULL;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Randomly lash out...\r\n");
    return TRUE;
  }

  if (cmd)
    return FALSE;

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (IS_NPC(vict) && !IS_PET(vict) && (!victim || GET_LEVEL(vict) > GET_LEVEL(victim)))
      victim = vict;

  if (!FIGHTING(ch) && is_wearing(ch, 113803) && !rand_number(0, 1000) && victim)
  {
    act("\tLThe bracer on your arm begins to \tpvibrate\tL, sending a horrible pain\r\n"
        "up the back of your neck. You feel unable to control yourself as you\r\n"
        "lunge toward $N\tL with \tpi\tPn\tps\tpa\tPn\tpi\tPt\tpy\tL!!\tn",
        FALSE, ch, 0, victim, TO_CHAR);
    act("\tLThe bracer on $n\tL's arm begins to \tpvibrate\tL, sending a horrible shriek\r\n"
        "from his gut. You watch as $e lunges toward $N\tL with \tpi\tPn\tps\tpa\tPn\tpi\tPt\tpy\tL!!\tn",
        FALSE, ch, 0, victim, TO_ROOM);
    hit(ch, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(whisperwind)
{
  int s, i = 0;
  struct char_data *victim;
  struct char_data *vict = FIGHTING(ch);
  struct char_data *pet;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Procs cyclone, whisper 'blur' for attack blur, whisper"
                     " 'wind' to summon weapon spirit, whisper 'smite' for harm and "
                     "dispel evil.\r\n");
    return TRUE;
  }

  /* random cyclone proc */
  if (!cmd && !rand_number(0, 10) && vict)
  {
    weapons_spells("\tcYour \tWmoon\tCblade \tcbegins to gyrate violently in your hands, almost "
                   "causing you to fumble.  As soon as you regain control, the area is "
                   "suddenly overwhelmed with vicious northern \tWgales\tc!\tn",
                   "\tc$n's \tWmoon\tCblade \tcbegins to gyrate violently in $s hands, causing "
                   "$m to almost fumble.  As soon as $e regains control, the area is "
                   "suddenly overwhelmed with vicious northern \tWgales\tc!\tn",
                   "\tc$n's \tWmoon\tCblade \tcbegins to gyrate violently in $s hands, causing "
                   "$m to almost fumble.  As soon as $e regains control, the area is "
                   "suddenly overwhelmed with vicious northern \tWgales\tc!\tn",
                   ch, vict, (struct obj_data *)me, SPELL_WHIRLWIND);
    return TRUE;
  }

  skip_spaces(&argument);
  if (!is_wearing(ch, 109802))
    return FALSE;
  victim = ch->char_specials.fighting;
  if (!strcmp(argument, "blur") && CMD_IS("whisper"))
  {
    if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "\tcAs you whisper '\tCblur\tc' to your \tWmoon\tCblade\tc, nothing happens.\tn\r\n");
        return TRUE;
      }
      act("\tcAs you whisper '\tCblur\tc' to your "
          "\tWmoon\tCblade\tc, it calls upon the northern \tWgale\r\n"
          "\tcand envelops you in a sw\tCir\tWl\tCin\tcg cyclone making "
          "you move like the wind!\tn",
          FALSE, ch, 0, 0, TO_CHAR);
      act("\tcA northern \tWgale \tcblows in and envelops $n in a "
          "sw\tCir\tWl\tCin\tcg cyclone \r\nas $e invokes the power of "
          "$s \tWmoon\tCblade\tc, making $m move like the wind!\r\n"
          "$n \tCBLURS \tcas $e strikes $N \tcin rapid succession!\tn",
          1, ch, 0, FIGHTING(ch), TO_NOTVICT);
      act("\tcA northern \tWgale \tcblows in and envelops $n in a"
          "sw\tCir\tWl\tCin\tcg cyclone \r\nas $e invokes the power of "
          "$s \tWmoon\tCblade\tc, making $m move like the wind!\r\n"
          "$n \tCBLURS \tcas $e strikes YOU in rapid succession!\tn",
          1, ch, 0, FIGHTING(ch), TO_VICT);
      s = rand_number(8, 12);
      for (i = 0; i <= s; i++)
      {
        hit(ch, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        if (GET_POS(victim) == POS_DEAD)
          break;
      }
      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 1;
      return TRUE; /* end for */
    }              /* end if-fighting */
    else
      return FALSE;
  } /* end if-strcmp */

  else if (!strcmp(argument, "wind") && CMD_IS("whisper"))
  {
    if (mob_index[real_mobile(101225)].number < 1)
    {
      act("\tcAs you whisper '\tCwind\tc' to your \tWmoon\tCblade\tc, "
          "a \tWghostly mist \tcswirls\r\n"
          "in the area around you.  When it finally dissipates, the "
          "spirit of the\r\nblade has come to your calling in the "
          "form of a majestic \tBeagle\tc.",
          1, ch, 0, FIGHTING(ch), TO_CHAR);
      act("\tcAs $n whispers something to $s \tWmoon\tCblade\tc, "
          "a \tWghostly mist \tcswirls\r\n"
          "in the area around $m.  When it finally dissipates, the "
          "spirit of the \r\nblade has come to $s calling in the "
          "form of a majestic \tBeagle\tc.",
          1, ch, 0, FIGHTING(ch), TO_ROOM);
      pet = read_mobile(real_mobile(101225), REAL);
      char_to_room(pet, ch->in_room);
      add_follower(pet, ch);
      SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
      GET_LEVEL(pet) = GET_LEVEL(ch);
      GET_MAX_HIT(pet) = GET_MAX_HIT(ch);
      GET_HIT(pet) = GET_MAX_HIT(pet);
      return TRUE;
    }
    else
    {
      act("\tcAs you whisper '\tCwind\tc' to your \tWmoon\tCblade\tc, "
          "nothing seems to happen.\r\n"
          "The spirit of the blade is still somewhere in the realms!",
          1, ch, 0, FIGHTING(ch), TO_CHAR);
      return TRUE;
    }
  }
  else if (!strcmp(argument, "smite") && CMD_IS("whisper"))
  {
    if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room))
    {
      if (!IS_EVIL(FIGHTING(ch)))
      {
        act("\tcYour \tWmoon\tCblade \tctells you '\tWI will not harm "
            "non-evil beings with my power!\tc'\r\n"
            "You feel a seering burst of pain as you are \tCzapped \tcby "
            "your blade!\tn",
            1, ch, 0, FIGHTING(ch), TO_CHAR);
        act("\tc$n is \tCzapped \tcby his \tWmoon\tCblade \tcafter "
            "muttering something to it!",
            1, ch, 0, FIGHTING(ch), TO_ROOM);
        damage(ch, ch, rand_number(4, 12), -1, DAM_ENERGY, FALSE); // type -1 = no message
      }
      else
      {
        act("\tcAs you whisper '\tCsmite\tc' to your \tWmoon\tCblade\tc, "
            "it suddenly bursts into \trfl\tRam\tres\tc!\r\nYour blade flares "
            "angrily at $N \tcas it tries to smite $M \tcmightily!\tn",
            1, ch, 0, FIGHTING(ch), TO_CHAR);
        act("\tc$n mutters something to $s \tWmoon\tCblade \tcand it suddenly "
            "bursts into \tcfl\tRam\tres\tc!\r\n$n's blade seems to flare "
            "angrily at $N \tcas it tries to smite $M \tcmightily!\tn",
            1, ch, 0, FIGHTING(ch), TO_NOTVICT);
        act("\tc$n mutters something to $s \tWmoon\tCblade \tcand it suddenly "
            "bursts into \tcfl\tRam\tres\tc!\r\n$n's blade seems to flare "
            "angrily at you as it tries to smite you mightily!\tn",
            1, ch, 0, FIGHTING(ch), TO_VICT);

        call_magic(ch, FIGHTING(ch), 0, SPELL_HARM, 0, 30, CAST_SPELL);
        for (i = 0; i < 3; i++)
        {
          call_magic(ch, FIGHTING(ch), 0, SPELL_DISPEL_EVIL, 0, 30, CAST_SPELL);
          if (GET_POS(victim) == POS_DEAD)
            break;
        }
      }
      return TRUE;
    }
  }
  else
    return FALSE;
  return FALSE;
}

/* from homeland */
SPECIAL(chionthar_ferry)
{
  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a ferry.\r\n");
    return TRUE;
  }

  update_ship((struct obj_data *)me, 104262, 104266, 1, 4);
  return TRUE;
}

/* from homeland */
SPECIAL(alandor_ferry)
{
  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a ferry.\r\n");
    return TRUE;
  }

  update_ship((struct obj_data *)me, 126423, 126428, 1, 4);
  return TRUE;
}

/* from homeland */
SPECIAL(md_carpet)
{
  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a transport carpet.\r\n");
    return TRUE;
  }

  update_ship((struct obj_data *)me, 120036, 120040, 3, 10);
  return TRUE;
}

/* from homeland */
SPECIAL(floating_teleport)
{
  int door;
  struct obj_data *obj = (struct obj_data *)me;
  room_rnum roomnum;

  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a floating (moving) portal.\r\n");
    return TRUE;
  }

  if (((door = rand_number(0, 30)) < NUM_OF_DIRS) && CAN_GO(obj, door) &&
      (world[EXIT(obj, door)->to_room].zone == world[obj->in_room].zone))
  {
    roomnum = EXIT(obj, door)->to_room;
    act("$p floats away.", FALSE, 0, obj, 0, TO_ROOM);
    obj_from_room(obj);
    obj_to_room(obj, roomnum);
    act("$p floats in.", FALSE, 0, obj, 0, TO_ROOM);
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(vengeance)
{
  struct char_data *vict = FIGHTING(ch);

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Procs mass cure light and word of faith.\r\n");
    return TRUE;
  }

  if (cmd || !vict)
    return FALSE;

  int power = 10;
  if (GET_OBJ_VNUM(((struct obj_data *)me)) == 101199)
    power = 5;
  if (rand_number(0, power))
    return FALSE;

  if (GET_HIT(ch) < GET_MAX_HIT(ch) && rand_number(0, 4))
  {
    weapons_spells(
        "\tWYour sword begins to \tphum \tWloudly and then \tCglows\tW as it pours its healing powers into you.\tn",
        "\tWYour sword begins to \tphum \tWloudly and then \tCglows\tW as it pours its healing powers into you.\tn",
        "$n's \tWsword begings to \tphum \tWloudly and then \tCglow\tW as it pours its healing powers into $m\tW.\tn",
        ch, vict, (struct obj_data *)me, 0);
    call_magic(ch, 0, 0, SPELL_MASS_CURE_LIGHT, 0, GET_LEVEL(ch), CAST_WAND);
    return TRUE;
  }
  weapons_spells(
      "\tWYour blade starts to shake violently, nearly tearing itself from your grip,\tn\r\n"
      "\tWas it begins to \tCglow\tW with a \tcholy light\tW.  Suddenly a \tYblinding \tfflash\tn\tW of pure\tn\r\n"
      "\tWgoodness is released from the sword striking down any \trevil\tW in the area.\tn",

      "\tW$n's\tW blade starts to shake violently, nearly tearing itself from $s grip,\tn\r\n"
      "\tWas it begins to \tCglow\tW with a \tcholy light\tW.  Suddenly a \tYblinding \tfflash\tn\tW of pure\tn\r\n"
      "\tWgoodness is released from the sword striking down any \trevil\tW in the area.\tn",

      "\tW$n's\tW blade starts to shake violently, nearly tearing itself from $s grip,\tn\r\n"
      "\tWas it begins to \tCglow\tW with a \tcholy light\tW.  Suddenly a \tYblinding \tfflash\tn\tW of pure\tn\r\n"
      "\tWgoodness is released from the sword striking down any \trevil\tW in the area.\tn",
      ch, vict, (struct obj_data *)me, SPELL_WORD_OF_FAITH);
  return TRUE;
}

/* from homeland */
SPECIAL(neverwinter_button_control)
{
  struct obj_data *dummy = NULL;
  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *i = NULL;
  bool change = FALSE;

  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a button control...\r\n");
    return TRUE;
  }

  if (!CAN_GO(obj, EAST) && !CAN_GO(obj, SOUTH) && !CAN_GO(obj, WEST))
  {
    if (IS_CLOSED(real_room(123637), DOWN))
    {
      OPEN_DOOR(real_room(123637), dummy, DOWN);
      OPEN_DOOR(real_room(123641), dummy, UP);
      change = TRUE;
    }
  }

  if (change && GET_OBJ_SPECTIMER(obj, 0) == 0)
  {
    for (i = character_list; i; i = i->next)
      if (world[obj->in_room].zone == world[i->in_room].zone)
        send_to_char(i, "\tLYou hear a slight rumbling.\tn\r\n");
    GET_OBJ_SPECTIMER(obj, 0) = 9999;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(neverwinter_valve_control)
{
  /* A- North
     B- East
     C- South
     D- West
   */
  struct char_data *i = NULL;
  struct obj_data *dummy = 0;
  struct obj_data *obj = (struct obj_data *)me;
  bool avalve = FALSE, bvalve = FALSE, cvalve = FALSE, dvalve = FALSE, change = FALSE;

  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a valve control...\r\n");
    return TRUE;
  }

  if (!CAN_GO(obj, NORTH))
    avalve = TRUE;

  if (!CAN_GO(obj, EAST))
    bvalve = TRUE;

  if (!CAN_GO(obj, SOUTH))
    cvalve = TRUE;

  if (!CAN_GO(obj, WEST))
    dvalve = TRUE;

  if (avalve && bvalve && !cvalve && dvalve)
  {
    if (!IS_CLOSED(real_room(123440), EAST))
      OPEN_DOOR(real_room(123440), dummy, EAST);
    if (!IS_CLOSED(real_room(123441), WEST))
      OPEN_DOOR(real_room(123441), dummy, WEST);
    if (!IS_CLOSED(real_room(123469), EAST))
      OPEN_DOOR(real_room(123469), dummy, EAST);
    if (!IS_CLOSED(real_room(123470), WEST))
      OPEN_DOOR(real_room(123470), dummy, WEST);
    if (IS_CLOSED(real_room(123533), EAST))
    {
      OPEN_DOOR(real_room(123533), dummy, EAST);
      change = TRUE;
    }
    if (IS_CLOSED(real_room(123534), WEST))
      OPEN_DOOR(real_room(123534), dummy, WEST);
  }
  else if (avalve && !bvalve && cvalve && !dvalve)
  {
    if (IS_CLOSED(real_room(123440), EAST))
    {
      OPEN_DOOR(real_room(123440), dummy, EAST);
      change = TRUE;
    }
    if (IS_CLOSED(real_room(123441), WEST))
      OPEN_DOOR(real_room(123441), dummy, WEST);
    if (!IS_CLOSED(real_room(123469), EAST))
      OPEN_DOOR(real_room(123469), dummy, EAST);
    if (!IS_CLOSED(real_room(123470), WEST))
      OPEN_DOOR(real_room(123470), dummy, WEST);
    if (!IS_CLOSED(real_room(123533), EAST))
      OPEN_DOOR(real_room(123533), dummy, EAST);
    if (!IS_CLOSED(real_room(123534), WEST))
      OPEN_DOOR(real_room(123534), dummy, WEST);
  }
  else if (!avalve && bvalve && cvalve && dvalve)
  {
    if (!IS_CLOSED(real_room(123440), EAST))
      OPEN_DOOR(real_room(123440), dummy, EAST);
    if (!IS_CLOSED(real_room(123441), WEST))
      OPEN_DOOR(real_room(123441), dummy, WEST);
    if (IS_CLOSED(real_room(123469), EAST))
    {
      OPEN_DOOR(real_room(123469), dummy, EAST);
      change = TRUE;
    }
    if (IS_CLOSED(real_room(123470), WEST))
      OPEN_DOOR(real_room(123470), dummy, WEST);
    if (!IS_CLOSED(real_room(123533), EAST))
      OPEN_DOOR(real_room(123533), dummy, EAST);
    if (!IS_CLOSED(real_room(123534), WEST))
      OPEN_DOOR(real_room(123534), dummy, WEST);
  }
  else
  {
    if (!IS_CLOSED(real_room(123440), EAST))
      OPEN_DOOR(real_room(123440), dummy, EAST);
    if (!IS_CLOSED(real_room(123441), WEST))
      OPEN_DOOR(real_room(123441), dummy, WEST);
    if (!IS_CLOSED(real_room(123469), EAST))
      OPEN_DOOR(real_room(123469), dummy, EAST);
    if (!IS_CLOSED(real_room(123470), WEST))
      OPEN_DOOR(real_room(123470), dummy, WEST);
    if (!IS_CLOSED(real_room(123533), EAST))
      OPEN_DOOR(real_room(123533), dummy, EAST);
    if (!IS_CLOSED(real_room(123534), WEST))
      OPEN_DOOR(real_room(123534), dummy, WEST);
  }

  if (change)
    for (i = character_list; i; i = i->next)
      if (world[obj->in_room].zone == world[i->in_room].zone)
        send_to_char(i, "\tgYou hear the flow of rushing sewage somewhere.\tn\r\n");

  return FALSE;
}

/* from homeland */
SPECIAL(bloodaxe)
{
  int dam;
  struct char_data *vict = FIGHTING(ch);

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Bite.\r\n");
    return TRUE;
  }

  if (cmd || !vict || rand_number(0, 16))
    return FALSE;

  dam = rand_number(8, 8);

  GET_HIT(vict) -= dam;

  if (dam < GET_HIT(vict))
  {
    weapons_spells(
        "\tLYour $p \tLstarts \trhumming \tLlouder and louder. Suddenly "
        "the axehead reshapes into a powerful maw and bites $N\tL in the throat.\tRBlood \tLflows "
        "between the the canine jaws and spills to the ground and $N\tL howls in pain. With "
        "a satisfied grin the maw reverts back to an axehead.\tn",
        "$n's $p \tLstarts \trhumming \tLlouder and louder. Suddenly "
        "the axehead reshapes into a powerful maw and bites you in the throat. \tRBlood \tLflows "
        "between the canine jaws and spills to the ground and you howl in pain. With "
        "a satisfied grin the maw reverts back to an axehead.\tn",
        "$n's $p \tLstarts \trhumming \tLlouder and louder. Suddenly "
        "the axehead reshapes into a powerful maw and bites $N\tL in the throat. \tRBlood \tLflows "
        "between the the canine jaws and spills to the ground and $N\tL howls in pain. With "
        "a satisfied grin the maw reverts back to an axehead.\tn",
        ch, vict, (struct obj_data *)me, 0);
  }
  else
  {
    weapons_spells("\tLYour $p \tLstarts \trhumming \tLlouder and louder. Suddenly the "
                   "axehead reshapes into a powerful maw and bites $N\tL in the throat. $N\tL "
                   "looks at the \tRblood \tLflowing freely from the wound, then $S\tL eyes "
                   "\twglazes \tLover and $E falls to the ground, \trDEAD\tL. With a "
                   "satisfied grin the maw reverts back to an axehead.\tn",
                   "$n's $p \tLstarts \trhumming \tLlouder and louder. Suddenly the axehead "
                   "reshapes into a powerful maw and bites'you in the throat. You look at "
                   "the \tRblood \tLflowing freely from the wound, then your eyes \twglazes "
                   "\tLover and you fall to the ground, \trDEAD\tL.\tn",
                   "$n's $p \tLstarts \trhumming \tLlouder and louder. Suddenly the axehead "
                   "reshapes into a powerful maw and bites $N\tL in the throat. $N\tL looks at the "
                   "\tRblood \tLflowing freely from the wound, then $S eyes \twglazes "
                   "\tLover and $E falls to the ground, \trDEAD\tL. With a satisfied grin "
                   "the maw reverts back to an axehead.\tn",
                   ch, vict, (struct obj_data *)me, 0);
    GET_HIT(vict) = -100;
  }
  return TRUE;
}

/* from homeland */
SPECIAL(skullsmasher)
{
  struct char_data *vict = FIGHTING(ch);

  if (!ch || cmd || !vict)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Knockdown.\r\n");
    return TRUE;
  }

  int power = 25;

  if (GET_OBJ_VNUM(((struct obj_data *)me)) == 101850)
    power = 15;
  if (rand_number(0, power))
    return FALSE;
  if (MOB_FLAGGED(vict, MOB_NOBASH))
    return FALSE;
  if (AFF_FLAGGED(vict, AFF_IMMATERIAL))
    return FALSE;

  weapons_spells(
      "\tLAs you swing your maul at $N \tLit connects with $S head\tn\r\n"
      "\tLand suddenly \tWgl\two\tWws brigh\twt\tWly\tL.  A look of overwhelming \trpain\tL shows on\tn\r\n"
      "\tL$S face as $E slowly slumps to the ground.\tn",

      "\tLAs $n \tLswings $s maul at you it connects with your head\tn\r\n"
      "\tLand suddenly \tWgl\two\tWws brigh\twt\tWly\tL.  A feeling of overwhelming \trpain\tL courses\tn\r\n"
      "\tLthrough your body, and you feel yourself slump to the ground.\tn",

      "\tLAs $n \tLswings $s maul at $N \tLit connects with $S head\tn\r\n"
      "\tLand suddenly \tWgl\two\tWws brigh\twt\tWly\tL.  A look of overwhelming \trpain \tLshows on\tn\r\n"
      "\tL$S face as $E slowly slumps to the ground.\tn",
      ch, vict, (struct obj_data *)me, 0);
  change_position(vict, POS_SITTING);
  USE_FULL_ROUND_ACTION(vict);
  return TRUE;
}

/* from homeland */
SPECIAL(acidsword)
{
  int dam;
  struct char_data *vict = NULL;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Acid corrosion.\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 135199))
    return FALSE;

  vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 16))
    return FALSE;

  dam = dice(4, 3);

  GET_HIT(vict) -= dam;

  if (GET_HIT(vict) > -9)
  {
    weapons_spells(
        "\tLYour\tn $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
        "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
        "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\tn\r\n"
        "$N\tL, hissing as it starts to corrode.\tn",
        "$n's $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
        "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
        "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\tn\r\n"
        "you\tL, hissing as it starts to corrode.\tn",
        "$n's $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
        "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
        "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\r\n"
        "$N, hissing as it starts to corrode.\tn",
        ch, vict, (struct obj_data *)me, 0);
  }
  else
  {
    weapons_spells(
        "\tLYour\tn $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
        "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
        "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\tn\r\n"
        "$N\tL, hissing as it melts\tn $N \tLto o\twoz\tLing pulp.\tn",
        "$n's $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
        "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
        "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\tn\r\n"
        "$N\tL, hissing as it melts\tn $N \tLto o\twoz\tLing pulp.\tn",
        "$n's $p \tLstarts to \tGglow \tLwith a \tgd\tGi\tgm gr\tGe\tgen\r\n"
        "sh\tGe\tgen \tLand suddenly a \tgth\tGi\tgn str\tGea\tgm of ac\tGi\tgd\r\n"
        "sp\tGe\tgws fo\tGr\tgth \tLfrom the tip of the blade and strikes\r\n"
        "you, hissing as it melts you to o\twoz\tLing pulp.\tn",
        ch, vict, (struct obj_data *)me, 0);
    GET_HIT(vict) = -50;
  }
  return TRUE;
}

/* malevolence - this has a spec abillity vamp AND a blur attack -zusuk */
SPECIAL(malevolence)
{
  struct char_data *vict = NULL;
  int num_hits = 0, i = 0;
  struct obj_data *malevolence = (struct obj_data *)me;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Attack Blur (3-5 bonus attacks on proc)\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 132101))
    return FALSE;

  vict = FIGHTING(ch);

  if (cmd || !vict || rand_number(0, 15))
    return FALSE;

  act("$p\tn glows with a bright \tYyellow\tn sheen before pulsing with \tRblood red malevolent light\tn as your attacks begin to speed up!",
      TRUE, ch, malevolence, vict, TO_CHAR);
  act("$p\tn glows with a bright \tYyellow\tn sheen before pulsing with \tRblood red malevolent light\tn as $n's\tn attacks begin to speed up!",
      TRUE, ch, malevolence, vict, TO_VICT);
  act("$p\tn glows with a bright \tYyellow\tn sheen before pulsing with \tRblood red malevolent light\tn as $n's\tn attacks begin to speed up!",
      TRUE, ch, malevolence, vict, TO_NOTVICT);

  num_hits = rand_number(3, 5);

  for (i = 0; i <= num_hits; i++)
  {
    if (valid_fight_cond(ch, TRUE))
      hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }

  return TRUE;
}

SPECIAL(rune_scimitar)
{
  struct char_data *vict = NULL;
  int num_hits = 0, i = 0;
  struct obj_data *scimitar = (struct obj_data *)me;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Attack Blur (4-7 bonus attacks on proc)\r\n");
    send_to_char(ch, "Proc: Deft Parry - on parry will do a light vamp attack\r\n");
    send_to_char(ch, "Proc: Deft Dodge - on dodge will do a light vamp attack\r\n");
    return TRUE;
  }

  if (!is_wearing(ch, 132101))
    return FALSE;

  vict = FIGHTING(ch);

  if (cmd || !vict)
    return FALSE;

  /* blur proc */
  if (!rand_number(0, 10))
  {
    act("$p\tY glows with a \tLdark sheen\tY before pulsing with \tBblue arcane light\tY as your attacks begin to speed up!\tn",
        TRUE, ch, scimitar, vict, TO_CHAR);
    act("$p\tY glows with a \tLdark sheen\tY before pulsing with \tBblue arcane light\tn as $n's\tY attacks begin to speed up!\tn",
        TRUE, ch, scimitar, vict, TO_VICT);
    act("$p\tY glows with a \tLdark sheen\tY before pulsing with \tBblue arcane light\tn as $n's\tY attacks begin to speed up!\tn",
        TRUE, ch, scimitar, vict, TO_NOTVICT);

    num_hits = rand_number(4, 7);

    for (i = 0; i <= num_hits; i++)
    {
      if (valid_fight_cond(ch, TRUE))
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    }
    return TRUE;
  }

  /* parry proc */
  else if (!strcmp(argument, "parry") && rand_number(0, 2))
  {
    act("\tLAs you parry the attack, \tn$p \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N \tLand transfers it back to you.\tn",
        FALSE, ch, scimitar, vict, TO_CHAR);
    act("\tLAs \tn$n\tL parries your attack, \tn$p \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom you and transfers it back to $m.\tn",
        FALSE, ch, scimitar, vict, TO_VICT);
    act("\tLAs \tn$n\tL parries \tn$N's\tL attack, \tn$p \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N\tL.\tn",
        FALSE, ch, scimitar, vict, TO_NOTVICT);
    damage(ch, vict, dice(10, 5), -1, DAM_ENERGY, FALSE); // type -1 = no dam message
    call_magic(ch, ch, 0, SPELL_CURE_CRITIC, 0, 1, CAST_SPELL);
    return TRUE;
  }

  /* dodge proc */
  else if (!strcmp(argument, "dodge") && !rand_number(0, 3))
  {
    act("\tLAs you dodge the attack, \tn$p \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N \tLand transfers it back to you.\tn",
        FALSE, ch, scimitar, vict, TO_CHAR);
    act("\tLAs \tn$n\tL dodges your attack, \tn$p \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom you and transfers it back to $m.\tn",
        FALSE, ch, scimitar, vict, TO_VICT);
    act("\tLAs \tn$n\tL dodges \tn$N's\tL attack, \tn$p \tCglows brightly\tL as it steals some \trlifeforce\tn "
        "\tLfrom $N\tL.\tn",
        FALSE, ch, scimitar, vict, TO_NOTVICT);
    damage(ch, vict, dice(10, 5), -1, DAM_ENERGY, FALSE); // type -1 = no dam message
    call_magic(ch, ch, 0, SPELL_CURE_CRITIC, 0, 1, CAST_SPELL);
    return TRUE;
  }

  /* didn't do anything! */
  return FALSE;
}

/* from homeland */
SPECIAL(snakewhip)
{
  // struct affected_type af;
  struct char_data *vict = FIGHTING(ch);
  struct obj_data *weepan = (struct obj_data *)me;
  int dam;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Drow-only, Snake-Bite.\r\n");
    return TRUE;
  }

  if (cmd || !vict || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if ((GET_RACE(ch) != RACE_DROW || GET_SEX(ch) != SEX_FEMALE) && is_wearing(ch, 135500))
  {
    if (GET_HIT(ch) > 0)
    {
      act("\tLYour $p \tLh\tYi\tLss\tYe\tLs angrily as it turns against you.\r\n"
          "All three snakeheads suddenly lunges forwardand sink their fangs in you throat. \r\n"
          "You barely have time to feel the terrible pain before you fall over with \tRbl\tro\tRod\r\n"
          "\tLflowing freely from the wounds in your neck.\tn",
          FALSE, ch,
          weepan, 0, TO_CHAR);

      act("$n's $p \tLh\tYi\tLss\tYe\tLs angrily as it turns against $n\tL. \r\n"
          "All three snakeheads suddenly lunges forward and sink their fangs in $n's \tLthroat.\r\n"
          "$n \tLbarely have time to feel the terrible pain before falling over with \tRbl\tro\tRod\r\n"
          "\tLflowing freely from the wounds in the neck.\tn",
          FALSE, ch,
          weepan, 0, TO_ROOM);
      GET_HIT(ch) = -5;
      change_position(ch, POS_INCAP);
    }
    return TRUE;
  }
  if (rand_number(0, 15))
    return FALSE;

  dam = dice(GET_LEVEL(ch), 3);

  GET_HIT(vict) -= dam;

  if (GET_HIT(vict) > -9)
  {
    weapons_spells(
        "\tLYour $p \tLh\tYi\tLss\tYe\tLs with fury as all three snakeheads suddenly lunges for $N\tL.\r\n"
        "Their fangs sink deep into the \tRfl\tre\tRsh \tLand $N \tLcries out in pain.\tn",
        "$n's $p \tLh\tYi\tLss\tYe\tLs\r\nwith fury as all three snakeheads suddenly lunges for you.\r\n"
        "Their fangs sink deep into the \tRfl\tre\tRsh \tLand you cry out in pain.\tn",
        "$n's $p \tLh\tYi\tLss\tYe\tLs\r\n with fury as all three snakeheads suddenly lunges for $N\tL.\r\n"
        "Their fangs sink deep into the \tRfl\tre\tRsh \tLand $N \tLcries out in pain.\tn",
        ch, vict, (struct obj_data *)me, 0);
  }
  else
  {
    weapons_spells(
        "\tLYour $p \tLh\tYi\tLss\tYe\tLs with fury as all three snakeheads suddenly lunges for $N\tL.\r\n"
        "Their fangs sink deep into the \tRfl\tre\tRsh\tL, draining away the remaining life of $N \tLwho\r\n"
        "falls over dead.\tn",
        "$n's $p \tLh\tYi\tLss\tYe\tLs\r\n with fury as all three snakeheads suddenly lunges for you.\r\n"
        "Their fangs sink deep into the \tRfl\tre\tRsh\tL, draining away your remaining life and\r\n"
        "you fall over dead.\tn",
        "$n's $p \tLh\tYi\tLss\tYe\tLs\r\n with fury as all three snakeheads suddenly lunges for $N\tL.\r\n"
        "Their fangs sink deep into the \tRfl\tre\tRsh\tL, draining away the remaining life of $N \tLwho\r\n"
        "falls over dead.\tn",
        ch, vict, (struct obj_data *)me, 0);
    GET_HIT(vict) = -50;
  }
  return TRUE;
}

/* from homeland */
SPECIAL(tormblade)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc only vs Evil:  Dispel Magic randomly on hit.\r\n"
                     "                    Torms Protection of Evil on critical hits.\r\n");
    return TRUE;
  }

  struct char_data *vict;
  struct affected_type af;

  if (cmd)
    return FALSE;

  vict = FIGHTING(ch);
  if (!vict)
    return FALSE;
  if (!IS_EVIL(vict))
    return FALSE;

  if (argument)
  {
    skip_spaces(&argument);
    if (!strcmp(argument, "critical"))
    {
      // okies, we assume its a crit then.
      act("$n's $p shines as it protects $m.", FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);
      act("Your $p shines as it protects you.", FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
      new_affect(&af);
      af.spell = SPELL_PROT_FROM_EVIL;
      af.modifier = 2;
      af.location = APPLY_AC_NEW;
      af.duration = dice(1, 4);
      affect_join(ch, &af, TRUE, FALSE, FALSE, FALSE);
      return TRUE;
    }
  }
  if (!rand_number(0, 30))
  {
    act("$n's $p hums loudly.", FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);
    act("Your $p hums loudly.", FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
    call_magic(ch, vict, 0, SPELL_DISPEL_MAGIC, 0, GET_LEVEL(ch), CAST_WAND);
    return TRUE;
  }

  return FALSE;
}

/* from homeland */
SPECIAL(witherdirk)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Contagion\r\n");
    return TRUE;
  }

  struct char_data *vict;

  if (cmd)
    return FALSE;

  vict = FIGHTING(ch);
  if (!vict)
    return FALSE;

  if (rand_number(0, 30))
    return FALSE;

  weapons_spells(
      "\tLThe dirk \trwrithes\tL and \twtwists\tL as it bites deep into $N\tL's skin,\tn\r\n"
      "\tgputrid\tr blo\tro\trd\tL wells up in $S eyes, causing great pain and discomfort.\tn",
      "\tLThe dirk \trwrithes\tL and \twtwists\tL as it bites deep into YOUR skin,\tn\r\n"
      "\tgputrid\tr blo\tro\trd\tL wells up in YOUR eyes, causing great pain and discomfort.\tn",
      "\tLThe dirk \trwrithes\tL and \twtwists\tL as it bites deep into $N\tL's skin,\tn\r\n"
      "\tgputrid\tr blo\tro\trd\tL wells up in $S eyes, causing great pain and discomfort.\tn",
      ch, vict, (struct obj_data *)me, SPELL_CONTAGION);

  return TRUE;
}

/* from homeland */
SPECIAL(air_sphere)
{
  int dam = 0;
  struct char_data *vict;
  struct affected_type af;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: Electric Shock, on saying 'storm', haste and "
                     "chain lightning.\r\n");
    return TRUE;
  }

  if (!cmd && FIGHTING(ch) && !rand_number(0, 25))
  {
    vict = FIGHTING(ch);
    dam = 20 + dice(2, 8);
    act("\tbYour \tBsphere of lighting \tYglows bright\tb as electricity\r\n"
        "\tbc\tBr\tbackl\tBe\tbs \tball about its surface.\tn\r\n"
        "\tbSuddenly the \tYglow \tWintensifies\tb and a \tB\tfbolt of lightning\tn\r\n"
        "\tbshoots forth from the sphere band strikes $N \tbdead on!\tn",
        FALSE, ch, 0, vict, TO_CHAR);
    act("$n's \tBsphere of lighting \tYglows bright\tb as electricity\tn\r\n"
        "\tbc\tBr\tbackl\tBe\tbs \tball about its surface.\tn\r\n"
        "\tbSuddenly the \tYglow \tWintensifies\tb and a \tB\tfbolt of lightning\r\n"
        "\tbshoots forth from the sphere band strikes $N \tbdead on!\tn",
        FALSE, ch, 0, vict, TO_ROOM);
    damage(ch, vict, dam, -1, DAM_ELECTRIC, FALSE); // type -1 = no message
    return TRUE;
  }

  // haste/chain once a day on command
  if (cmd && argument && CMD_IS("say"))
  {
    if (!is_wearing(ch, 136100))
      return FALSE;

    skip_spaces(&argument);
    if (!strcmp(argument, "storm"))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "Nothing happens.\r\n");
        return TRUE;
      }

      act("\tcAs you speak to your \tbsphere of lightning\tc, it begins to \tWglow\tc and fill with violent\tn\r\n"
          "\tcenergy.  The energy builds until it \tba\twrc\tbs and \tBc\tbrackle\tBs\tc all over the sphere before\tn\r\n"
          "\tcit lets loose in a violent \tblightning storm\tc.  As the energy from the storm begins\tn\r\n"
          "\tcto fade, a jolt of \tYelectricity\tc flows up through your arms, causing your heart to\tn\r\n"
          "\tcrace really fast!\tn",
          FALSE, ch, 0, 0, TO_CHAR);
      act("\tcAs $n \tcspeaks a word of power to $s \tbsphere of lightning\tc,\tn\r\n"
          "\tcit \tWglows brightly\tc and violent energy begins to fill it. The sphere\tn\r\n"
          "\tba\twrc\tbs\tc and \tBc\tbrackle\tBs\tc before it lets loose a violent \tblightning\tn\r\n"
          "\tbstorm\tc.  The energy begins to fade, but before this can happen a jolt of \tYelectricity\tn\r\n"
          "\tcflows up $n's\tc arms and causes $s heart to race really fast!",
          FALSE, ch, 0, 0, TO_ROOM);

      new_affect(&af);
      af.spell = SPELL_HASTE;
      af.duration = 100;
      SET_BIT_AR(af.bitvector, AFF_HASTE);
      affect_join(ch, &af, TRUE, FALSE, FALSE, FALSE);

      call_magic(ch, 0, 0, SPELL_CHAIN_LIGHTNING, 0, 20, CAST_POTION);

      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 24;
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(bolthammer)
{
  int dam;
  struct char_data *vict = FIGHTING(ch);

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  Lightning bolt.\r\n");
    return TRUE;
  }

  if (cmd || !vict || rand_number(0, 18))
    return FALSE;

  dam = 25 + dice(1, 30);

  if (dam < GET_HIT(vict))
  {
    weapons_spells(
        "\tLYour\tn $p \tLstarts to \twth\tWr\twob \tLviolently and\tn\r\n"
        "\tLthe sound of th\tYun\tLder can be heard. Suddenly a bolt of \tclig\tChtn\tcing \tLleaps\tn\r\n"
        "\tLfrom the head of the warhammer and strikes\tn $N \tLwith full force.\tn",

        "$n'�s $p \tLstarts to \twth\tWr\twob \tLviolently and the soundof th\tYun\tLder can be heard.\tn\r\n"
        "\tLSuddenly a bolt of \tclig\tChtn\tcing \tLleaps from the head of the warhammer and strikes you\tn\r\n"
        "\tlwith full force.\tn",

        "$n'�s $p \tLstarts to \twth\tWr\twob \tLviolently and\tn\r\n"
        "\tLthe sound of th\tYun\tLder can be heard. Suddenly a bolt of \tclig\tChtn\tcing \tLleaps\tn\r\n"
        "\tLfrom the head of the warhammer and strikes $N \tLwith full force.\tn",
        ch, vict, (struct obj_data *)me, 0);
  }
  else
  {
    dam += 20;
    weapons_spells(
        "\tLYour\tn $p \tLstarts to \twth\tWr\twob \tLviolently and the sound of th\tYun\tLder can be\tn\r\n"
        "\tLheard. Suddenly a bolt of \tclig\tChtn\tcing \tLleaps from the head of the warhammer and strikes\tn\r\n"
        "$N \tLwith full force. When the flash is gone\r\n"

        "\tL you see the corpse of\tn $N \tLstill twitching on the ground.\tn",
        "$n'�s $p \tLstarts to \twth\tWr\twob \tLviolently and the soundof th\tYun\tLder can be heard.\tn\r\n"
        "\tLSuddenly a bolt of \tclig\tChtn\tcing \tLleaps from the head of the warhammer and strikes\tn\r\n"
        "\tLyou with full force. You twitch a few times before your body goes still forever.\tn",

        "$n's�s $p \tLstarts to \twth\tWr\twob \tLviolently and the soundof th\tYun\tLder can be heard.\tn\r\n"
        "\tLSuddenly a bolt of \tclig\tChtn\tcing \tLleaps from the head of the warhammer and strikes\tn \tn\r\n"
        "$N \tLwith full force. When the flash is gone you see\r\n"
        "\tLthe corpse of\tn $N \tLstill twitching on the ground.\tn",
        ch, vict, (struct obj_data *)me, 0);
  }

  damage(ch, vict, dam, -1, DAM_ELECTRIC, FALSE); // type -1 = no message
  return TRUE;
}

/* from homeland */
SPECIAL(rughnark)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: magical damage 25+10d4 for monk.\r\n");
    return TRUE;
  }

  int dam = 0;
  struct char_data *vict = 0;

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
    return FALSE;

  if (dice(1, 40) < 39)
    return FALSE;

  if (IS_GOOD(ch) && dice(1, 10) > 5)
    return FALSE;

  if (MONK_TYPE(ch) < 20 && !IS_NPC(ch))
    return FALSE;

  vict = FIGHTING(ch);
  dam = 25 + dice(10, 4);
  if (dam > GET_HIT(vict))
    dam = GET_HIT(vict);
  if (dam < 50)
    return FALSE;

  weapons_spells(
      "\tLAs you make contact with your opponent, the twin \tWmithril\tL blades rip apart\tn\r\n"
      "\tLthe flesh in a gory display of blood, tearing huge chunks of meat out of your\tn\r\n"
      "\tLopponent as $E screams in agony and pain.\tn",

      "\tLAs $n make contact with you, the twin \tWmithril\tL blades rip apart\tn\r\n"
      "\tLyour flesh in a gory display of blood, tearing huge chunks of meat out of your\tn\r\n"
      "\tLown body as you scream in agony.\tn",

      "\tLAs $n makes contact with $s opponent, the twin \tWmithril\tL blades rip apart\tn\r\n"
      "\tLthe flesh in a gory display of blood, tearing huge chunks of meat out of $s\tn\r\n"
      "\tLopponent as $E screams in agony and pain.\tn",
      ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dam, -1, DAM_SLICE, FALSE); // type -1 = no message
  return TRUE;
}

/* from homeland */
SPECIAL(magma)
{
  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc: magmaburst (fire damage for monk.)\r\n");
    return TRUE;
  }

  int dam = 0;
  struct char_data *vict = 0;

  if (cmd)
    return FALSE;

  if (!FIGHTING(ch))
    return FALSE;

  if (dice(1, 40) < 39)
    return FALSE;

  if (MONK_TYPE(ch) < 20 && !IS_NPC(ch))
    return FALSE;

  vict = FIGHTING(ch);
  dam = 50 + dice(30, 5);
  if (dam > GET_HIT(vict))
    dam = GET_HIT(vict);
  if (dam < 50)
    return FALSE;
  weapons_spells(
      "\tLAs your hands \twimpact\tL with your opponent, the \trflowing m\tRagm\tra\tn\r\n"
      "\trignites\tL and emits a \twburst\tL of \tRf\tYi\tRr\tre\tL and \tyr\tLo\tyck\tL.\tn",
      "\tLAs $n\tL's hands \twimpact\tL with YOU, the \trflowing m\tRagm\tra\tn\r\n"
      "\trignites\tL and emits a \twburst\tL of \tRf\tYi\tRr\tre\tL and \tyr\tLo\tyck\tL.\tn",
      "\tLAs $n\tL's hands \twimpact\tL with $s opponent, the \trflowing m\tRagm\tra\tn\r\n"
      "\trignites\tL and emits a \twburst\tL of \tRf\tYi\tRr\tre\tL and \tyr\tLo\tyck\tL.\tn",
      ch, vict, (struct obj_data *)me, 0);
  damage(ch, vict, dam, -1, DAM_FIRE, FALSE); // type -1 = no message
  return TRUE;
}

/* from homeland */
SPECIAL(halberd)
{
  struct char_data *vict = FIGHTING(ch);
  struct affected_type af;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Proc:  blur, stun, slow.\r\n");
    return TRUE;
  }

  if (cmd || !vict)
    return FALSE;

  switch (rand_number(0, 30))
  {
  case 27:
    if (!can_stun(vict))
      return FALSE;
    // A slight chance to stun
    weapons_spells(
        "\tcYour\tn $p \tcreverberates loudly as it sends\tn\r\n"
        "\tcforth a \tWthunderous blast \tcat \tn$N\tc.  $E is knocked backwards as the\tn\r\n"
        "\tcfull brunt of the blast hits $M squarely.\tn",
        "\tc$n\tn $p \tcreverberates loudly as it sends\tn\r\n"
        "\tcforth a \tWthunderous blast \tcat YOU.  You are knocked backwards as the\tn\r\n"
        "\tcfull brunt of the blast hits YOU squarely.\tn",
        "\tc$n\tn $p \tcreverberates loudly as it sends\tn\r\n"
        "\tcforth a \tWthunderous blast \tcat $N.  $E is knocked backwards as the\tn\r\n"
        "\tcfull brunt of the blast hits $M squarely.\tn",
        ch, vict, (struct obj_data *)me, 0);
    new_affect(&af);
    af.spell = SKILL_CHARGE;
    SET_BIT_AR(af.bitvector, AFF_STUN);
    af.duration = dice(1, 4);
    affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
    return TRUE;
    break;

  case 21:
    // blur
    weapons_spells(
        "\tcAs your\tn $p \tctumultuously resonates, a strange \twm\tWi\twst \tcemanates from\tn\r\n"
        "\tcit quickly enshrouding you.  The \twm\tWi\twst \tcinduces you into a deep trance as your\tn\r\n"
        "\tctrance as your body melds with your\tn $p. \r\n\tcYou begin to \tCmove "
        "\tCat a \tcrapid\tCly in\tccreas\tCing sp\tceed \tCblurring out of focus.\tn\tn",

        "\tcAs $n\tn $p \tctumultuously resonates, a strange \twm\tWi\twst \tcemanates from\tn\r\n"
        "\tcit quickly enshrouding $m.  The \twm\tWi\twst \tcinduces $m into a deep\tn\r\n"
        "\tctrance as $m body melds with $s\tn $p.  \r\n\tc$e begins to \tCmove \tn"
        "\tCat a \tcrapid\tCly in\tccreas\tCing sp\tceed then $e \tCblurs out of focus.\tn",

        "\tcAs $n\tn $p \tctumultuously resonates, a strange \twm\tWi\twst \tcemanates from\tn\r\n"
        "\tcit quickly enshrouding $m.  The \twm\tWi\twst \tcinduces $m into a deep\tn\r\n"
        "\tctrance as $m body melds with $s\tn $p.  \r\n\tc$e begins to \tCmove \tn"
        "\tCat a \tcrapid\tCly in\tccreas\tCing sp\tceed then $e \tCblurs out of focus.\tn",
        ch, vict, (struct obj_data *)me, 0);
    return TRUE;
    break;

  case 17:
  case 16:
    // damage, and a chance to slow.
    weapons_spells(
        "\tcYour\tn $p \tcravenously slashes deep into \tn$N\tn\r\n"
        "\tcinflicting life-threatening wounds causing $M to convulse and \tRbleed profusely.\tn",
        "\tc$n\tn $p \tMravenously slashes deep into YOU inflicting\tn\r\n"
        "\tclife-threatening wounds causing YOU to convulse and \tRbleed profusely.\tn",
        "\tc$n\tn $p \tcravenously slashes deep into \tn$N\tc\r\n"
        "\tcinflicting life-threatening wounds causing $M to convulse and \tRbleed profusely.\tn",
        ch, vict, (struct obj_data *)me, SPELL_SLOW);
    damage(ch, vict, 10 + dice(2, 4), -1, DAM_POISON, FALSE); // type -1 = no message
    return TRUE;
    break;

  default:
    return FALSE;
  }
  return FALSE;
}

SPECIAL(bank)
{
  int amount;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a bank.\r\n");
    return TRUE;
  }

  if (CMD_IS("balance"))
  {
    if (GET_BANK_GOLD(ch) > 0)
      send_to_char(ch, "\twYour current balance is \tW%d \tYcoins\tw.\tn\r\n", GET_BANK_GOLD(ch));
    else
      send_to_char(ch, "\twYou currently have \tWno\tw money deposited.\tn\r\n");
    return (TRUE);
  }
  else if (CMD_IS("deposit"))
  {

    /* code to accomdate "all" */
    skip_spaces(&argument);
    if (is_abbrev(argument, "all"))
    {
      amount = GET_GOLD(ch);
      send_to_char(ch, "\twYou deposit all (\tW%d\tw) your \tYcoins\tw.\tn\r\n", amount);
      act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
      decrease_gold(ch, amount);
      increase_bank(ch, amount);
      return (TRUE);
    }

    if ((amount = atoi(argument)) <= 0)
    {
      send_to_char(ch, "How much do you want to deposit?\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < amount)
    {
      send_to_char(ch, "You don't have that many coins!\r\n");
      return (TRUE);
    }
    decrease_gold(ch, amount);
    increase_bank(ch, amount);
    send_to_char(ch, "\twYou deposit \tW%d\tY coins\tw.\tn\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (TRUE);
  }
  else if (CMD_IS("withdraw"))
  {

    /* code to accomdate "all" */
    skip_spaces(&argument);
    if (is_abbrev(argument, "all"))
    {
      amount = GET_BANK_GOLD(ch);
      send_to_char(ch, "\twYou withdraw all (\tW%d\tw) your \tYcoins\tw.\tn\r\n", amount);
      act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
      increase_gold(ch, amount);
      decrease_bank(ch, amount);
      return (TRUE);
    }

    if ((amount = atoi(argument)) <= 0)
    {
      send_to_char(ch, "How much do you want to withdraw?\r\n");
      return (TRUE);
    }
    if (GET_BANK_GOLD(ch) < amount)
    {
      send_to_char(ch, "You don't have that many coins deposited!\r\n");
      return (TRUE);
    }
    increase_gold(ch, amount);
    decrease_bank(ch, amount);
    send_to_char(ch, "\twYou withdraw \tW%d \tYcoins\tw.\tn\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (TRUE);
  }
  else
    return (FALSE);
}

/*
   Portal that will jump to a player's clanhall
   Exit depends on which clan player belongs to
   Created by Jamdog - 4th July 2006
 */
SPECIAL(clanportal)
{
  int iPlayerClan = -1;
  struct obj_data *obj = (struct obj_data *)me;
  struct obj_data *port;
  zone_vnum z;
  room_vnum r;
  char obj_name[MAX_INPUT_LENGTH];
  room_rnum was_in = IN_ROOM(ch);
  struct follow_type *k;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a clan portal.\r\n");
    return TRUE;
  }

  if (!CMD_IS("enter"))
    return FALSE;

  argument = one_argument_u(argument, obj_name);

  /* Check that the player is trying to enter THIS portal */
  if (!(port = get_obj_in_list_vis(ch, obj_name, NULL, world[(IN_ROOM(ch))].contents)))
  {
    return (FALSE);
  }

  if (port != obj)
    return (FALSE);

  iPlayerClan = GET_CLAN(ch);

  if (iPlayerClan == NO_CLAN)
  {
    send_to_char(ch, "You try to enter the portal, but it returns you back to the same room!\n\r");
    return TRUE;
  }

  if ((z = get_clanhall_by_char(ch)) == NOWHERE)
  {
    send_to_char(ch, "Your clan does not have a clanhall!\n\r");
    log("Warning: Clan Portal - No clanhall (Player: %s, Clan ID: %d)", GET_NAME(ch), iPlayerClan);
    return TRUE;
  }

  //  r = (z * 100) + 1;    /* Get room xxx01 in zone xxx */
  /* for now lets have the exit room be 3000, until we get hometowns in, etc */
  r = 3000;

  if (!(real_room(r)))
  {
    send_to_char(ch, "Your clanhall is currently broken - contact an Imm!\n\r");
    log("Warning: Clan Portal failed (Player: %s, Clan ID: %d)", GET_NAME(ch), iPlayerClan);
    return TRUE;
  }

  /* First, move the player */
  if (!(House_can_enter(ch, r)))
  {
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
    return TRUE;
  }

  act("$n enters $p, and vanishes!", FALSE, ch, port, 0, TO_ROOM);
  act("You enter $p, and you are transported elsewhere", FALSE, ch, port, 0, TO_CHAR);
  char_from_room(ch);
  char_to_room(ch, real_room(r));
  look_at_room(ch, 0);
  act("$n appears from thin air!", FALSE, ch, 0, 0, TO_ROOM);

  /* Then, any followers should auto-follow (Jamdog 19th June 2006) */
  for (k = ch->followers; k; k = k->next)
  {
    if ((IN_ROOM(k->follower) == was_in) && (GET_POS(k->follower) >= POS_STANDING))
    {
      act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
      char_from_room(k->follower);
      char_to_room(k->follower, real_room(r));
      look_at_room(k->follower, 0);
      act("$n appears from thin air!", FALSE, k->follower, 0, 0, TO_ROOM);
    }
  }
  return TRUE;
}

/* from homeland */
SPECIAL(hellfire)
{
  int timer = 0;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke haste and fireshield on armor by saying 'Hellfire'.\r\n");
    return TRUE;
  }

  if (!cmd)
    return FALSE;
  if (!argument)
    return FALSE;

  skip_spaces(&argument);

  if (!is_wearing(ch, 132102))
    return FALSE;

  if (cmd && CMD_IS("say") && !strcmp(argument, "hellfire"))
  {
    timer = GET_OBJ_SPECTIMER((struct obj_data *)me, 0);
    if (timer > 0)
    {
      send_to_char(ch, "Nothing happens (recharge in %d hours).\r\n", timer);
      return TRUE;
    }

    act("\tLThe pure flames of your $p\tL is invoked.\tn\r\n"
        "\tLThe flames rise and protects YOU!\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);

    act("\tLThe pure flames of $n\tL's $p\tL is invoked.\tn\r\n"
        "\tLThe flames rise and protects $m!\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);

    call_magic(ch, ch, 0, SPELL_FIRE_SHIELD, 0, 26, CAST_POTION);
    call_magic(ch, ch, 0, SPELL_HASTE, 0, 26, CAST_POTION);

    GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 12;
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(angel_leggings)
{

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 1\r\n");

  if (!ch)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 2\r\n");

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke fly by keyword 'Elysium'.\r\n");
    return TRUE;
  }

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 3\r\n");

  if (!cmd)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 4\r\n");

  if (!argument)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 5\r\n");

  if (!is_wearing(ch, 106021))
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug 7 - Argument: %s\r\n", argument);

  skip_spaces(&argument);

  if (cmd && CMD_IS("say") && !strcmp(argument, "elysium"))
  {
    if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
    {
      if (DEBUGMODE)
        send_to_char(ch, "Debug - Mark 8\r\n");

      send_to_char(ch, "Nothing happens.\r\n");
      return TRUE;
    }

    act("\tWThe power of $p\tW is invoked.\tn\r\n"
        "\tcYour feet slowly raise off the ground.\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
    act("\tWThe power of $n\tW's $p\tW is invoked.\tn\r\n"
        "\tw$s feet slowly raise of the ground!\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);

    if (DEBUGMODE)
      send_to_char(ch, "Debug - Mark 9\r\n");

    call_magic(ch, ch, 0, SPELL_FLY, 0, 30, CAST_POTION);

    GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 48;

    return TRUE;
  }
  return FALSE;
}

/* zusuk's epic robes from cloud realms */
SPECIAL(dragon_robes)
{

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 1\r\n");

  if (!ch)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 2\r\n");

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Invoke displace by keyword 'Power'.\r\n");
    return TRUE;
  }

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 3\r\n");

  if (!cmd)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 4\r\n");

  if (!argument)
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug - Mark 5\r\n");

  /*[144669]*/
  if (!is_wearing(ch, 144669))
    return FALSE;

  if (DEBUGMODE)
    send_to_char(ch, "Debug 7 - Argument: %s\r\n", argument);

  skip_spaces(&argument);

  if (cmd && CMD_IS("say") && !strcmp(argument, "power"))
  {
    if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
    {
      if (DEBUGMODE)
        send_to_char(ch, "Debug - Mark 8\r\n");

      send_to_char(ch, "Nothing happens.\r\n");
      return TRUE;
    }

    act("\tWThe power of $p\tW is invoked.\tn\r\n"
        "\tcYour form begins to shimmer in and out of reality!\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_CHAR);
    act("\tWThe power of $n\tW's $p\tW is invoked.\tn\r\n"
        "\tw$s begins to shimmer in and out of reality!\tn\r\n",
        FALSE, ch, (struct obj_data *)me, 0, TO_ROOM);

    if (DEBUGMODE)
      send_to_char(ch, "Debug - Mark 9\r\n");

    call_magic(ch, ch, 0, SPELL_DISPLACEMENT, 0, 30, CAST_POTION);

    GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 48;

    return TRUE;
  }
  return FALSE;
}

/* from homeland, converts an object type PET into an actual
 * pet mobile follower, object vnum must match mobile vnum */
SPECIAL(bought_pet)
{

  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a pet.\r\n");
    return TRUE;
  }

  struct follow_type *f = NULL;

  for (f = ch->followers; f; f = f->next)
  {
    if (IS_NPC(f->follower) && MOB_FLAGGED(f->follower, MOB_MERCENARY) && AFF_FLAGGED(f->follower, AFF_CHARM))
    {
      send_to_char(ch, "You may only have one purchased follower.\r\n");
      return 1;
    }
  }

  struct obj_data *obj = (struct obj_data *)me;

  if (obj->carried_by == 0)
    return FALSE;

  if (IS_NPC(obj->carried_by))
    return FALSE;

  struct char_data *pet = NULL;

  pet = read_mobile(GET_OBJ_VNUM(obj), VIRTUAL);

  /* found matching vnum for obejct, loaded pet succesfully */
  if (pet)
  {
    char_to_room(pet, obj->carried_by->in_room);
    add_follower(pet, obj->carried_by);
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

    /* success message */
    send_to_char(obj->carried_by, "You have acquired a companion.\r\n");

    /* get rid of the purchased object */
    extract_obj(obj);
    return TRUE;
  }

  /* failed to load pet */
  return FALSE;
}

/* from homeland, i doubt we are going to port this, houses replace these */
/*
SPECIAL(storage_chest) {
  if (cmd)
    return FALSE;
  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a player storage chest.\r\n");
    return TRUE;
  }

  struct obj_data *chest = (struct obj_data*) me;
  if (GET_OBJ_VAL(chest, 3) != 0)
    return FALSE;
  ch = chest->carried_by;
  if (!ch) {
    REMOVE_BIT(GET_OBJ_EXTRA(chest), ITEM_INVISIBLE);
    return FALSE;
  }
  if (IS_NPC(ch) || IS_PET(ch))
    return FALSE;

  snprintf(buf2, sizeof(buf2), "chest storage %s", GET_NAME(ch));
  chest->name = str_dup(buf2);

  if (GET_OBJ_VNUM(chest) == 1291) {
    snprintf(buf2, sizeof(buf2), "\tLAn ornate \tcmithril\tL chest owned by \tw%s\tL rests here.\tn", GET_NAME(ch));
    chest->description = str_dup(buf2);
    snprintf(buf2, sizeof(buf2), "\tLan ornate \tcmithril\tL chest owned by \tw%s\tn", GET_NAME(ch));
    chest->short_description = str_dup(buf2);

  } else {
    snprintf(buf2, sizeof(buf2), "\tLA storage chest owned by \tW%s\tL is standing here.\tn", GET_NAME(ch));
    chest->description = str_dup(buf2);
    snprintf(buf2, sizeof(buf2), "\tLa chest owned by \tW%s\tn", GET_NAME(ch));
    chest->short_description = str_dup(buf2);
  }
  GET_OBJ_VAL(chest, 3) = -GET_IDNUM(ch);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_VALUES);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_NAME);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_DESC);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_SHORT);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_TYPE);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_WEAR);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_EXTRA);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_TIMER);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_WEIGHT);
  SET_BIT(GET_OBJ_SAVED(chest), SAVE_OBJ_COST);
  save_chests();
  return TRUE;
}
 */

/* from homeland */
SPECIAL(clang_bracer)
{
  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "Dwarf and Group Only.  Invoke battle prowess by saying 'argenoth'.\r\n");
    return TRUE;
  }

  if (ch && is_wearing(ch, 121456))
  {
    if (!cmd && GET_RACE(ch) != RACE_DWARF)
    {
      act("\tLThe bracer begins to glow on your arm, clenching tighter and "
          "tighter until you rip it off in agony.\tn",
          FALSE, ch, 0, 0,
          TO_CHAR);
      act("\tLA bracer on $n\tL's arm begins to glow brightly and a look of "
          "intense pain crosses $s face as $e rips the bracer free.\tn",
          FALSE, ch, 0, 0, TO_ROOM);

      if (GET_EQ(ch, WEAR_WRIST_R) == (obj_data *)me)
        obj_to_char(unequip_char(ch, WEAR_WRIST_R), ch);
      else if (GET_EQ(ch, WEAR_WRIST_L) == (obj_data *)me)
        obj_to_char(unequip_char(ch, WEAR_WRIST_L), ch);
      return TRUE;
    }

    // invoke it!
    if (!cmd)
      return FALSE;
    if (!argument)
      return FALSE;
    if (!CMD_IS("say"))
      return FALSE;
    skip_spaces(&argument);

    if (!strcmp(argument, "argenoth"))
    {
      if (GET_OBJ_SPECTIMER((struct obj_data *)me, 0) > 0)
      {
        send_to_char(ch, "You attempt to invoke your bracer, but nothing happens.\r\n");
        return TRUE;
      }

      struct group_data *group;

      if ((group = GROUP(ch)) == NULL)
      {
        send_to_char(ch, "You recall from lore this item will not work unless in a group.\r\n");
        return FALSE;
      }

      /* success! */
      send_to_group(NULL, group, "The memories of ancient battles fills your mind, each "
                                 "blow clear as if it were yesterday.  You feel your muscles tighten "
                                 "then relax as the skill of ancient warriors is merged with your own.\r\n");
      call_magic(ch, ch, 0, SPELL_MASS_ENHANCE, 0, 30, CAST_POTION);
      GET_OBJ_SPECTIMER((struct obj_data *)me, 0) = 24;
      return TRUE;
    }
  }
  return FALSE;
}

/* from homeland */
SPECIAL(menzo_chokers)
{
  struct affected_type *af2;
  struct affected_type af;

  if (!ch)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "For Drow, finding pair will give +1 to hitroll.\r\n");
    return TRUE;
  }

  for (af2 = ch->affected; af2; af2 = af2->next)
  {
    if (af2->spell == AFF_MENZOCHOKER)
    {
      if (!is_wearing(ch, 135626) || !is_wearing(ch, 135627))
      {
        send_to_char(ch, "\tLYou suddenly feel bereft of your \tmgoddess's\tL"
                         " touch.\tn\r\n");
        affect_from_char(ch, AFF_MENZOCHOKER);
      }
      return FALSE;
    }
  }

  if (is_wearing(ch, 135626) && is_wearing(ch, 135627))
  {
    if (GET_RACE(ch) == RACE_DROW)
    {
      send_to_char(ch, "\tLYour blood quickens, as if your soul has been touched "
                       "by a higher power.\tn\r\n");
      af.location = APPLY_HITROLL;
      af.duration = 5;
      af.modifier = 1;
      SET_BIT_AR(af.bitvector, AFF_MENZOCHOKER);
      affect_join(ch, &af, FALSE, FALSE, TRUE, FALSE);
      return FALSE;
    }
  }
  return FALSE;
}

int get_vendor_armor_cost(struct char_data *ch, int level, int armortype, sbyte masterwork)
{
  int cost = 0;

  cost += armor_list[armortype].cost;

  if (masterwork)
    cost += 200;

  switch (level)
  {
  case 1:
    cost += 5000;
    break;
  case 2:
    cost += 10000;
    break;
  case 3:
    cost += 20000;
    break;
  case 4:
    cost += 50000;
    break;
  }

  if (armor_list[armortype].armorType != ARMOR_TYPE_SHIELD && armor_list[armortype].armorType != ARMOR_TYPE_TOWER_SHIELD)
    cost /= 4;

  int mod = MIN(50, GET_CHA(ch) + compute_ability(ch, ABILITY_APPRAISE));
  cost = (cost * 100) / (100 + mod);

  return MAX(1, cost);
}

int get_vendor_weapon_cost(struct char_data *ch, int level, int weapontype, sbyte masterwork)
{
  int cost = 0;

  cost += weapon_list[weapontype].cost;

  if (masterwork)
    cost += 300;

  switch (level)
  {
  case 1:
    cost += 5000;
    break;
  case 2:
    cost += 10000;
    break;
  case 3:
    cost += 20000;
    break;
  case 4:
    cost += 50000;
    break;
  }

  int mod = MIN(50, GET_CHA(ch) + compute_ability(ch, ABILITY_APPRAISE));
  cost = (cost * 100) / (100 + mod);

  return MAX(1, cost);
}

void display_buy_armor_types(struct char_data *ch, int level, sbyte masterwork, char *type)
{

  int i = 0;
  int cost = 0;
  int wear = ITEM_WEAR_TAKE;
  char wear_str[50];

  if (is_abbrev(type, "body"))
  {
    wear = ITEM_WEAR_BODY;
    snprintf(wear_str, sizeof(wear_str), "BODY");
  }
  else if (is_abbrev(type, "arms"))
  {
    wear = ITEM_WEAR_ARMS;
    snprintf(wear_str, sizeof(wear_str), "ARMS");
  }
  else if (is_abbrev(type, "legs"))
  {
    wear = ITEM_WEAR_LEGS;
    snprintf(wear_str, sizeof(wear_str), "LEGS");
  }
  else if (is_abbrev(type, "head"))
  {
    wear = ITEM_WEAR_HEAD;
    snprintf(wear_str, sizeof(wear_str), "HEAD");
  }
  else if (is_abbrev(type, "shield"))
  {
    wear = ITEM_WEAR_SHIELD;
    snprintf(wear_str, sizeof(wear_str), "SHIELD");
  }
  else
  {
    send_to_char(ch, "Please specify body, arms, legs, head or shield.\r\n");
    return;
  }

  send_to_char(ch, "%s\r\n", wear_str);
  for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
  {
    if (armor_list[i].wear != wear)
      continue;
    cost = get_vendor_armor_cost(ch, level, i, masterwork);
    if (armor_list[i].armorType == ARMOR_TYPE_SHIELD || armor_list[i].armorType == ARMOR_TYPE_TOWER_SHIELD)
    {
      send_to_char(ch, "%-25s ", armor_list[i].name);
      send_to_char(ch, " %d gold\r\n", MAX(1, cost));
    }
    else
    {
      send_to_char(ch, "%-25s ", armor_list[i].name);
      send_to_char(ch, " %d gold each\r\n", MAX(1, cost));
    }
  }
  if (level > 0)
    send_to_char(ch, "These prices are for +%d items.\r\n\r\n", level);
}

void display_buy_weapon_types(struct char_data *ch, int level, sbyte masterwork)
{

  int i = 0, cost = 0;

  for (i = 2; i < NUM_WEAPON_TYPES; i++)
  {
    cost = get_vendor_weapon_cost(ch, level, i, masterwork);
    send_to_char(ch, "%25s %6d gold ", weapon_list[i].name, cost);
    if ((i % 2) == 1)
      send_to_char(ch, "\r\n");
  }
  if ((i % 2) != 1)
    send_to_char(ch, "\r\n");
  if (level > 0)
    send_to_char(ch, "These prices are for +%d items.\r\n\r\n", level);
}

#define MASTERWORK_MSG "Please specify whether you prefer mundane or masterwork items.\r\n"                             \
                       "Mundane items provide no bonuses.\r\n"                                                          \
                       "Masterwork weapons provide +1 to attack roll, but not damage. They cost an extra 300 gold.\r\n" \
                       "Masterwork armor reduces the armor check penalty for certain skills, by one.  They cost an extra 50 gold per piece.\r\n"

void set_weapon_name(struct obj_data *obj, int type)
{

  char buf[200];

  snprintf(buf, sizeof(buf), "%s %s", AN(weapon_list[type].name), weapon_list[type].name);
  obj->short_description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s %s lies here.", AN(weapon_list[type].name), weapon_list[type].name);
  obj->description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s", weapon_list[type].name);
  obj->name = strdup(buf);
}

void set_armor_name(struct obj_data *obj, int type)
{

  char buf[200];

  snprintf(buf, sizeof(buf), "%s %s", AN(armor_list[type].name), armor_list[type].name);
  obj->short_description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s %s lies here.", AN(armor_list[type].name), armor_list[type].name);
  obj->description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s", armor_list[type].name);
  obj->name = strdup(buf);
}

void set_masterwork_obj_name(struct obj_data *obj)
{

  char buf[200];

  snprintf(buf, sizeof(buf), "%s (masterwork)", obj->short_description);
  obj->short_description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s (masterwork)", obj->description);
  obj->description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s masterwork", obj->name);
  obj->name = strdup(buf);
}

void set_magical_obj_name(struct obj_data *obj, int level)
{

  char buf[200];
  int i = 0;

  snprintf(buf, sizeof(buf), "%s (+%d)", obj->short_description, level);
  obj->short_description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s (+%d)", obj->description, level);
  obj->description = strdup(buf);

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
  {
    snprintf(buf, sizeof(buf), "%s", weapon_list[GET_OBJ_VAL(obj, 0)].name);
    for (i = 0; i < strlen(buf); i++)
      if (buf[i] == ' ')
        buf[i] = '-';
    char res_buf[128];
    snprintf(res_buf, sizeof(res_buf), " %s +%d", obj->name, level);
    strlcat(buf, res_buf, sizeof(buf));
  }
  else
  {
    snprintf(buf, sizeof(buf), "%s +%d", obj->name, level);
  }
  obj->name = strdup(buf);
}

SPECIAL(buyarmor)
{

  if (!CMD_IS("buy") && !CMD_IS("list"))
    return 0;

  struct char_data *keeper = (struct char_data *)me;
  int level = 0;
  char arg1[100], // masterwork or mundane? if level is zero, if level > 0, then it's the armor's name
                  // which we'll copy into arg2 so we don't need to mess around with 2 variables, since
                  // vendors level 1 and up ONLY sell weapons of their level bonus
      arg2[100];  // name of the armor desired

  half_chop(argument, arg1, arg2);

  if (GET_LEVEL(keeper) <= 10)
    level = 0;
  else if (GET_LEVEL(keeper) <= 15)
    level = 1;
  else if (GET_LEVEL(keeper) <= 20)
    level = 2;
  else if (GET_LEVEL(keeper) <= 25)
    level = 3;
  else if (GET_LEVEL(keeper) <= 30)
    level = 4;

  if (CMD_IS("list"))
  {
    if (!*arg1)
    {
      if (level == 0)
      {
        send_to_char(ch, MASTERWORK_MSG);
        return 1;
      }
    }
    if (!*arg2 && level == 0)
    {
      send_to_char(ch, "Please specify body, arms, legs, head or shield.\r\n");
      return 1;
    }
    if (level == 0 && (!is_abbrev(arg1, "mundane") && !is_abbrev(arg1, "masterwork")))
    {
      send_to_char(ch, MASTERWORK_MSG);
      return 1;
    }
    display_buy_armor_types(ch, level, level == 0 ? !is_abbrev(arg1, "mundane") : false, level == 0 ? strdup(arg2) : strdup(arg1));
    return 1;
  }

  if (!*argument)
  {
    if (level == 0)
      send_to_char(ch, MASTERWORK_MSG);
    else
      send_to_char(ch, "Please specify the type of magical armor/shield you want to purchase.\r\n");
    return 1;
  }

  if (!*arg1)
  {
    if (level == 0)
      send_to_char(ch, MASTERWORK_MSG);
    else
      send_to_char(ch, "Please specify the type of magical armor/shield you want to purchase.\r\n");
    return 1;
  }

  if (!*arg2 && level == 0)
  {
    send_to_char(ch, "Please specify which armor piece you wish to buy.\r\n"
                     "Type buy (mundane|masterwork) (full name of armor piece/shield)\r\n"
                     "A list can be seen by typing: list (mundane|masterwork) (body|arms|legs|head|shield)\r\n");
    send_to_char(ch, "Masterwork armor costs 50 gold more per piece, or 200 gold more for shields and bucklers.\r\n");
    return 1;
  }

  if (level == 0 && (!is_abbrev(arg1, "mundane") && !is_abbrev(arg1, "masterwork")))
  {
    send_to_char(ch, MASTERWORK_MSG);
    return 1;
  }

  if (level > 0 && *argument)
  { // let's just work with arg2 to keep things easy
    skip_spaces(&argument);
    snprintf(arg2, sizeof(arg2), "%s", argument);
  }

  int i = 0, cost = 0;

  for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
  {
    if (!strcmp(arg2, armor_list[i].name))
      break;
  }

  if (i >= NUM_SPEC_ARMOR_TYPES)
  {
    send_to_char(ch, "Please specify which armor piece you wish to buy.  A list can be seen by typing: list (body|arms|legs|head|shield)\r\n");
    send_to_char(ch, "Masterwork armor costs 50 gold more per piece, or 200 gold more for shields and bucklers.\r\n");
    send_to_char(ch, "\r\nYou must specify the exact, full name of the armor you wish to buy, in lowercase.\r\n");
    return 1;
  }

  sbyte mundane = TRUE;

  // We want mundane to be the default since we're using is_abbrev and they both start with "m"
  if (!is_abbrev(arg1, "mundane"))
  {
    mundane = FALSE;
  }

  cost = get_vendor_armor_cost(ch, level, i, !mundane);

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d gold to buy %s, but you only have %d.\r\n", cost, armor_list[i].name, GET_GOLD(ch));
    return 1;
  }

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
  {
    act("You can't carry any more items.", FALSE, ch, 0, 0, TO_CHAR);
    return (1);
  }

  struct obj_data *obj = NULL;
  obj_vnum base_vnum = 0;

  if (armor_list[i].armorType == ARMOR_TYPE_SHIELD || armor_list[i].armorType == ARMOR_TYPE_TOWER_SHIELD)
  {
    base_vnum = 61;
  }
  else
  {
    switch (i % 4)
    {
    case 0:
      base_vnum = 55;
      break;
    case 1:
      base_vnum = 56;
      break;
    case 2:
      base_vnum = 60;
      break;
    case 3:
      base_vnum = 57;
      break;
    }
  }

  if ((obj = read_object(base_vnum, VIRTUAL)) == NULL)
  {
    send_to_char(ch, "There seems to be an error in purchasing %s.  Please inform a staff member.\r\n", armor_list[i].name);
    return 1;
  }

  set_armor_object(obj, i);
  GET_OBJ_COST(obj) = cost;
  set_armor_name(obj, i);
  if (!mundane && level == 0)
  {
    set_masterwork_obj_name(obj);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MASTERWORK);
  }
  if (level > 0)
  {
    set_magical_obj_name(obj, level);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MASTERWORK);
    GET_OBJ_VAL(obj, 4) = level; // Enhancement Bonus
  }

  GET_GOLD(ch) -= cost;
  obj_to_char(obj, ch);
  send_to_char(ch, "You purchase %s for %d gold coins.\r\n", obj->short_description, cost);

  return 1;
}

SPECIAL(buyweapons)
{

  if (!CMD_IS("buy") && !CMD_IS("list"))
    return 0;

  struct char_data *keeper = (struct char_data *)me;
  int level = 0;
  char arg1[100], // masterwork or mundane? if level is zero, if level > 0, then it's the weapon's name
                  // which we'll copy into arg2 so we don't need to mess around with 2 variables, since
                  // vendors level 1 and up ONLY sell weapons of their level bonus
      arg2[100];  // name of the weapon desired

  half_chop(argument, arg1, arg2);

  if (GET_LEVEL(keeper) <= 10)
    level = 0;
  else if (GET_LEVEL(keeper) <= 15)
    level = 1;
  else if (GET_LEVEL(keeper) <= 20)
    level = 2;
  else if (GET_LEVEL(keeper) <= 25)
    level = 3;
  else if (GET_LEVEL(keeper) <= 30)
    level = 4;

  if (CMD_IS("list"))
  {
    if (!*arg1)
    {
      if (level == 0)
      {
        send_to_char(ch, MASTERWORK_MSG);
        return 1;
      }
    }
    if (level == 0 && (!is_abbrev(arg1, "mundane") && !is_abbrev(arg1, "masterwork")))
    {
      send_to_char(ch, MASTERWORK_MSG);
      return 1;
    }
    display_buy_weapon_types(ch, level, !is_abbrev(arg1, "mundane"));
    return 1;
  }

  if (!*argument)
  {
    if (level == 0)
      send_to_char(ch, MASTERWORK_MSG);
    else
      send_to_char(ch, "Please specify the type of magical weapon you want to purchase.\r\n");
    return 1;
  }

  if (!*arg1)
  {
    if (level == 0)
      send_to_char(ch, MASTERWORK_MSG);
    else
      send_to_char(ch, "Please specify the type of magical weapon you want to purchase.\r\n");
    return 1;
  }

  if (!*arg2 && level == 0)
  {
    display_buy_weapon_types(ch, 0, false);
    send_to_char(ch, "Masterwork weapons cost 300 gold pieces more.\r\n");
    return 1;
  }

  if (level == 0 && (!is_abbrev(arg1, "mundane") && !is_abbrev(arg1, "masterwork")))
  {
    send_to_char(ch, MASTERWORK_MSG);
    return 1;
  }

  if (level > 0 && *argument)
  { // let's just work with arg2 to keep things easy
    skip_spaces(&argument);
    snprintf(arg2, sizeof(arg2), "%s", argument);
  }

  int i = 0, cost = 0;

  for (i = 0; i < NUM_WEAPON_TYPES; i++)
  {
    if (!strcmp(arg2, weapon_list[i].name))
      break;
  }

  if (i >= NUM_WEAPON_TYPES)
  {
    display_buy_weapon_types(ch, 0, false);
    send_to_char(ch, "Masterwork weapons cost 300 gold pieces more.\r\n");
    send_to_char(ch, "\r\nYou must specify the exact, full name of the weapon you wish to buy, in lowercase.\r\n");
    return 1;
  }

  sbyte mundane = TRUE;

  // We want mundane to be the default since we're using is_abbrev and they both start with "m"
  if (!is_abbrev(arg1, "mundane"))
  {
    mundane = FALSE;
  }

  cost = get_vendor_weapon_cost(ch, level, i, !mundane);

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d gold to buy %s, but you only have %d.\r\n", cost, weapon_list[i].name, GET_GOLD(ch));
    return 1;
  }

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
  {
    act("You can't carry any more items.", FALSE, ch, 0, 0, TO_CHAR);
    return (1);
  }

  struct obj_data *obj = NULL;
  obj_vnum base_vnum = 66;

  if ((obj = read_object(base_vnum, VIRTUAL)) == NULL)
  {
    send_to_char(ch, "There seems to be an error in purchasing %s.  Please inform a staff member.\r\n", weapon_list[i].name);
    return 1;
  }

  set_weapon_object(obj, i);
  GET_OBJ_COST(obj) = cost;
  set_weapon_name(obj, i);
  if (!mundane && level == 0)
  {
    set_masterwork_obj_name(obj);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MASTERWORK);
    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 1;
    obj->affected[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
  }
  if (level > 0)
  {
    set_magical_obj_name(obj, level);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MASTERWORK);
    GET_OBJ_VAL(obj, 4) = level; // Enhancement Bonus
  }

  GET_GOLD(ch) -= cost;
  obj_to_char(obj, ch);
  send_to_char(ch, "You purchase %s for %d gold coins.\r\n", obj->short_description, cost);

  return 1;
}

/*************************/
/* end object procedures */
/*************************/

#undef DEBUGMODE

/* EoF */
