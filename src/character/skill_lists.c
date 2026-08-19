/**************************************************************************
 *  File: character/skill_lists.c                     Part of LuminariMUD *
 *  Usage: Skill prerequisites, lists, and training effects.               *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "constants.h"
#include "act.h"
#include "character/abilities.h"
#include "character/skill_lists.h"
#include "character/class.h"
#include "combat/fight.h"
#include "modify.h"
#include "clan.h"
#include "mudlim.h"
#include "graph.h"
#include "dgscript/dg_scripts.h"
#include "mud_event.h"
#include "actions.h"
#include "combat/assign_wpn_armor.h"
#include "magic/domains_schools.h"
#include "character/feats.h"
#include "magic/spell_prep.h"
#include "obj/item.h"
#include "craft/alchemy.h"
#include "obj/treasure.h"
#include "mob/mob_utils.h"
#include "character/evolutions.h"
#include "olc/oasis.h"
#include "quest/quest.h"
#include "character/backgrounds.h"
#include "character/perks.h"

/* Special procedures for mobiles. */

#define LEARNED_LEVEL 0 /* % known which is considered "learned" */
#define MAX_PER_PRAC 1  /* max percent gain in skill per practice */
#define MIN_PER_PRAC 2  /* min percent gain in skill per practice */
#define PRAC_TYPE 3     /* should it say 'spell' or 'skill'?	 */


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
        (CLASS_LEVEL(ch, CLASS_WIZARD) > 17 || CLASS_LEVEL(ch, CLASS_SORCERER) > 19))
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
        (CLASS_LEVEL(ch, CLASS_WIZARD) > 16 || CLASS_LEVEL(ch, CLASS_SORCERER) > 18))
      return TRUE;
    else
      return FALSE;
    /* magical based epic spells (not accessable by divine) */
  case SKILL_EPIC_MAGE_ARMOR:
    if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 31 && GET_LEVEL(ch) >= 20 &&
        (CLASS_LEVEL(ch, CLASS_WIZARD) > 13 || CLASS_LEVEL(ch, CLASS_SORCERER) > 13))
      return TRUE;
    else
      return FALSE;
  case SKILL_EPIC_WARDING:
    if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 33 && GET_LEVEL(ch) >= 20 &&
        (CLASS_LEVEL(ch, CLASS_WIZARD) > 15 || CLASS_LEVEL(ch, CLASS_SORCERER) > 15))
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
    if (GET_LEVEL(ch) >= 20 && (GET_REAL_CON(ch) + GET_REAL_STR(ch) >= 32))
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
    if ((CLASS_LEVEL(ch, CLASS_ROGUE) >= 9) || (IS_CASTER(ch) && GET_LEVEL(ch) >= 2))
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
    if (CLASS_LEVEL(ch, CLASS_RANGER) >= 3 || CLASS_LEVEL(ch, CLASS_DRUID) >= 6)
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
    if (!spell_info[i].name || spell_info[i].name == unused_spellname)
      continue;
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
    if (!spell_info[i].name || spell_info[i].name == unused_spellname)
      continue;
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

  send_to_char(ch, "\tCPractice Session(s): %d\tn\r\n\r\n", GET_PRACTICES(ch));
}


const char *cross_names[] = {"\tRNot Available to Your Class\tn", "\tcCross-Class Ability\tn",
                             "\tWClass Ability\tn"};

const int skills_alphabetic[NUM_SKILLS_IN_GAME] = {
    ABILITY_ACROBATICS, ABILITY_APPRAISE,      ABILITY_ARCANA,          ABILITY_ATHLETICS,
    ABILITY_BOARDING,   ABILITY_CONCENTRATION, ABILITY_DECEPTION,       ABILITY_DISABLE_DEVICE,
    ABILITY_DISCIPLINE, ABILITY_DISGUISE,      ABILITY_HANDLE_ANIMAL,   ABILITY_HISTORY,
    ABILITY_INSIGHT,    ABILITY_INTIMIDATE,    ABILITY_LINGUISTICS,     ABILITY_MEDICINE,
    ABILITY_NATURE,     ABILITY_PERCEPTION,    ABILITY_PERFORM,         ABILITY_PERSUASION,
    ABILITY_RELIGION,   ABILITY_RIDE,          ABILITY_SLEIGHT_OF_HAND, ABILITY_SPELLCRAFT,
    ABILITY_STEALTH,    ABILITY_TOTAL_DEFENSE, ABILITY_USE_MAGIC_DEVICE};

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
    send_to_char(
        ch,
        "*Name of skill, invested points, total points with all active bonuses\tn\r\n"
        "\tcSkill              Inve Tota Class/Cross/Unavailable  \tMUnspent trains: \tm%d\tn\r\n",
        GET_TRAINS(ch));
    for (i = 0; i < NUM_SKILLS_IN_GAME; i++)
    {
      send_to_char(ch, "%-18s [%2d] \tC[%2d]\tn %s\r\n", ability_names[skills_alphabetic[i]],
                   GET_ABILITY(ch, skills_alphabetic[i]), compute_ability(ch, skills_alphabetic[i]),
                   cross_names[modify_class_ability(ch, skills_alphabetic[i], GET_CLASS(ch))]);
    }
    return;
  case ABILITY_TYPE_CRAFT:
    /* as of 10/30/2014 we decided to make crafting indepdent of the skill/ability system */
    send_to_char(ch, "\tRNOTE:\tn Type '\tYcraft\tn' to see your crafting skills, "
                     "skills/abilities will no longer affect your crafting abilities.\r\n");
    start_ability = START_CRAFT_ABILITIES;
    end_ability = END_CRAFT_ABILITIES + 1;
    break;
  default:
    log("SYSERR: list_abilities called with invalid ability_type: %d", ability_type);
    start_ability = 1;
    end_ability = NUM_ABILITIES;
  }

  // if (IS_NPC(ch))
  // return;

  send_to_char(
      ch,
      "*Name of skill, invested points, total points with all active bonuses\tn\r\n"
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
      continue;
    default:
      break;
    }
    send_to_char(ch, "%-18s [%2d] \tC[%2d]\tn %s\r\n", ability_names[i], GET_ABILITY(ch, i),
                 compute_ability(ch, i), cross_names[modify_class_ability(ch, i, GET_CLASS(ch))]);
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
