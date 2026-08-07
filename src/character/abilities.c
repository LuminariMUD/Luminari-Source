/**************************************************************************
 *  File: character/abilities.c                       Part of LuminariMUD *
 *  Usage: Character ability calculations.                                *
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

extern struct background_data background_list[NUM_BACKGROUNDS];

int compute_ability(struct char_data *ch, int abilityNum)
{
  return compute_ability_full(ch, abilityNum, false);
}

int compute_ability_full(struct char_data *ch, int abilityNum, bool recursive)
{
  int value = 0;
  struct char_data *mobfol = NULL;

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

  if (affected_by_spell(ch, AFFECT_PRESCIENCE))
  {
    value += 2;
  }
  if (affected_by_spell(ch, AFFECT_PRESCIENCE_DEBUFF))
  {
    value -= 2;
  }

  if (GET_BACKGROUND(ch) >= 0)
  {
    if ((background_list[GET_BACKGROUND(ch)].skills[0] == abilityNum) ||
        (background_list[GET_BACKGROUND(ch)].skills[1] == abilityNum))
    {
      value += 2;
    }
  }

  if (HAS_FEAT(ch, FEAT_BG_TRADER) && is_crafting_skill(abilityNum))
  {
    value += 1;
  }

  if (HAS_FEAT(ch, FEAT_ELBOW_GREASE) && is_crafting_skill(abilityNum))
  {
    int artificer_level = CLASS_LEVEL(ch, CLASS_ARTIFICER);
    if (artificer_level >= 10)
      value += 6;
    else if (artificer_level >= 6)
      value += 4;
    else if (artificer_level >= 1)
      value += 2;
  }

  if (HAS_FEAT(ch, FEAT_BG_SAILOR) && abilityNum == ABILITY_CRAFT_FISHING)
    value += 5;

  if (affected_by_spell(ch, PSIONIC_INFLICT_PAIN))
    value += get_char_affect_modifier(
        ch, PSIONIC_INFLICT_PAIN,
        APPLY_HITROLL); // this should return a negative number, so + a - is -
  if (affected_by_spell(ch, SPELL_HEROISM))
    value += 2;
  else if (affected_by_spell(ch, SPELL_GREATER_HEROISM))
    value += 4;
  if (affected_by_spell(ch, SKILL_PERFORM))
    value += SONG_AFF_VAL(ch);
  if (HAS_FEAT(ch, FEAT_ABLE_LEARNER))
    value += 1;

  /* Jack of All Trades feat bonuses */
  if (HAS_FEAT(ch, FEAT_EXEMPLAR))
  {
    /* Exemplar: +1/2 artificer level to all skills */
    int artificer_level = CLASS_LEVEL(ch, CLASS_ARTIFICER);
    value += artificer_level / 2;
  }
  else if (HAS_FEAT(ch, FEAT_IMPROVED_JACK_OF_ALL_TRADES))
  {
    /* Improved Jack of All Trades: +6 to all skills */
    value += 6;
  }
  else if (HAS_FEAT(ch, FEAT_JACK_OF_ALL_TRADES))
  {
    /* Jack of All Trades: +3 to all skills */
    value += 3;
  }

  if (HAS_SKILL_FEAT(ch, abilityNum, feat_to_skfeat(FEAT_SKILL_FOCUS)))
    value += 3;
  if (HAS_SKILL_FEAT(ch, abilityNum, feat_to_skfeat(FEAT_EPIC_SKILL_FOCUS)))
    value += 6;
  if (affected_by_spell(ch, SPELL_EFFECT_GRAND_DESTINY))
    value += 4;
  if (!IS_NPC(ch) && IS_DAYLIT(IN_ROOM(ch)) && HAS_FEAT(ch, FEAT_LIGHT_BLINDNESS))
    value -= 1;
  if (IS_FRIGHTENED(ch))
    value -= 2;
  if (AFF_FLAGGED(ch, AFF_SICKENED))
    value -= 2;
  if (AFF_FLAGGED(ch, AFF_SHAKEN))
    value -= 2;
  if (AFF2_FLAGGED(ch, AFF2_COWERING))
    value -= 4;
  if (char_has_mud_event(ch, eHOLYJAVELIN))
    value -= 2;
  if (HAS_EVOLUTION(ch, EVOLUTION_SKILLED))
  {
    if (GET_CALL_EIDOLON_LEVEL(ch) >= 30)
      value += 5;
    else if (GET_CALL_EIDOLON_LEVEL(ch) >= 20)
      value += 4;
    else if (GET_CALL_EIDOLON_LEVEL(ch) >= 10)
      value += 3;
    else
      value += 2;
  }
  value -= get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);
  // vampire bonuses / penalties for feeding
  value += vampire_last_feeding_adjustment(ch);
  // try to avoid sending NPC's here, but just in case:
  /* Note on this:  More and more it seems necessary to have some
   * sort of NPC skill system in place, either an actual set
   * of SKILLS or some way to translate level, race and class into
   * an appropriate set of skills, mostly for intelligent, humanoid
   * NPCs. For now, just use the level, although that will be difficult. */
  if (IS_NPC(ch))
    value += GET_LEVEL(ch) * 0.75;
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
      if (GET_EQ(ch, i) && GET_EQ(ch, i)->affected[j].location == APPLY_SKILL &&
          GET_EQ(ch, i)->affected[j].specific == abilityNum)
      {
        if (GET_EQ(ch, i)->affected[j].modifier > high_eq)
          high_eq = GET_EQ(ch, i)->affected[j].modifier;
      }
    }
  }

  value += high_eq;

  /* Check for crafting tool bonuses */
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_CRAFTING_TOOL)
    {
      int tool_skill = GET_OBJ_VAL(GET_EQ(ch, i), 0);
      int tool_bonus = GET_OBJ_VAL(GET_EQ(ch, i), 1);

      /* Validate that the tool_skill is a valid crafting/harvest ability */
      if (tool_skill >= START_CRAFT_ABILITIES && tool_skill <= END_HARVEST_ABILITIES &&
          tool_skill == abilityNum && tool_bonus > 0)
      {
        /* Crafting tools provide their specified bonus to the associated skill */
        value += tool_bonus;
      }
    }
  }

  /* Add perk skill bonuses */
  if (!IS_NPC(ch))
  {
    value += get_perk_skill_bonus(ch, abilityNum);
  }

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

    /* Monk Acrobatic Defense perk bonus */
    if (!IS_NPC(ch))
      value += get_monk_acrobatic_defense_skill(ch);

    return value;

  case ABILITY_STEALTH:
    value += GET_DEX_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_KENDER_SKILL_MOD))
      value += 2;
    if (HAS_FEAT(ch, FEAT_SURVIVAL_INSTINCT))
      value += 3;
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
    if (HAS_FEAT(ch, FEAT_VAMPIRE_SKILL_BONUSES) && CAN_USE_VAMPIRE_ABILITY(ch))
      value += 8;
    if (HAS_FEAT(ch, FEAT_WOOD_ELF_MASK_OF_THE_WILD))
      value += 3;
    if (IN_NATURE(ch) && HAS_FEAT(ch, FEAT_MOON_ELF_BATHED_IN_MOONLIGHT))
    {
      if (weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET)
        value += 6;
    }
    if (HAS_REAL_FEAT(ch, FEAT_SHADOWFELL_MIND))
      value += 2;
    if (HAS_REAL_FEAT(ch, FEAT_TABAXI_CATS_TALENT))
      value += 2;
    if (HAS_FEAT(ch, FEAT_FAE_SENSES))
      value += 3;
    // if you're inside or in a forest, you can use spider climb to scale walls/trees and hide
    // from above.
    if (!OUTSIDE(ch) || (IN_ROOM(ch) != NOWHERE && world[IN_ROOM(ch)].sector_type == SECT_FOREST))
    {
      if (AFF_FLAGGED(ch, AFF_SPIDER_CLIMB))
        value += 8;
      else if (HAS_FEAT(ch, FEAT_VAMPIRE_SPIDER_CLIMB) && CAN_USE_VAMPIRE_ABILITY(ch))
        value += 8;
    }

    /* Inquisitor Favored Terrain: +2 Stealth in favored terrain */
    if (is_inquisitor_in_favored_terrain(ch))
      value += 2;

    value += (size_modifiers_inverse[GET_SIZE(ch)] * 4);
    value += compute_gear_armor_penalty(ch);

    /* Monk Improved Hide perk bonus */
    if (!IS_NPC(ch))
      value += get_monk_improved_hide_bonus(ch);

    return value;

  case ABILITY_PERCEPTION:
    value += GET_WIS_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_KENDER_SKILL_MOD))
      value += 2;
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
    if (HAS_FEAT(ch, FEAT_FAE_SENSES))
      value += 3;
    if (HAS_REAL_FEAT(ch, FEAT_TABAXI_CATS_TALENT))
      value += 2;
    if (AFF_FLAGGED(ch, AFF_DAZZLED))
      value--;
    if (AFF_FLAGGED(ch, AFF_DEAF))
      value -= 4;
    if (IS_LICH(ch))
      value += 8;
    if (HAS_FEAT(ch, FEAT_VAMPIRE_SKILL_BONUSES) && CAN_USE_VAMPIRE_ABILITY(ch))
      value += 8;
    /* Inquisitor Keen Senses: +2 per rank to Perception */
    if (!IS_NPC(ch))
    {
      int keen_senses_rank = get_inquisitor_keen_senses_rank(ch);
      if (keen_senses_rank > 0)
        value += (2 * keen_senses_rank);
      /* Inquisitor Investigator's Eye: +3 per rank to Perception (searching) */
      int investigators_eye_rank = get_inquisitor_investigators_eye_rank(ch);
      if (investigators_eye_rank > 0)
        value += (3 * investigators_eye_rank);
    }
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
    if (HAS_FEAT(ch, FEAT_DEMORALIZE))
      value += 2;

    if (HAS_FEAT(ch, FEAT_AUTHORITATIVE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_STERN_GAZE))
    {
      /* Unnamed bonus */
      value += MAX(1, CLASS_LEVEL(ch, CLASS_INQUISITOR) / 2);
    }
    if (HAS_FEAT(ch, FEAT_MINOTAUR_INTIMIDATING))
    {
      /* Unnamed bonus */
      value += 2;
    }

    if (HAS_REAL_FEAT(ch, FEAT_MENACING))
      value += 3;
    return value;

  case ABILITY_CONCENTRATION: /* not srd */
    if (GET_RACE(ch) == RACE_GNOME)
      value += 2;
    if (HAS_FEAT(ch, FEAT_KENDER_SKILL_MOD))
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

    /* a bit hackish */
    if (IS_CASTING(ch) && CLASS_LEVEL(ch, CLASS_SHADOW_DANCER) >= 1)
    {
      value += CLASS_LEVEL(ch, CLASS_SHADOW_DANCER) * 3;
    }

    /* Bard Spellsinger: Dirge of Dissonance - foes suffer concentration penalty */
    if (IN_ROOM(ch) != NOWHERE)
    {
      struct char_data *i = NULL;
      for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room)
      {
        if (i == ch)
          continue;
        if (!IS_NPC(i) && IS_PERFORMING(i) && has_bard_dirge_of_dissonance(i))
        {
          /* If not grouped with the performing bard, apply penalty */
          if (!GROUP(ch) || GROUP(ch) != GROUP(i))
          {
            value += get_bard_dirge_concentration_penalty(i); /* returns negative value */
            break;                                            /* Only apply once per room */
          }
        }
      }
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
    if (HAS_REAL_FEAT(ch, FEAT_SHADOWFELL_MIND))
      value += 2;
    if (HAS_FEAT(ch, FEAT_ELDRITCH_LORE))
      value += 2;
    return value;
  case ABILITY_APPRAISE:
    value += GET_INT_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_GRUBBY))
      value -= 6;
    if (HAS_FEAT(ch, FEAT_DILIGENT))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_ARTIFICERS_LORE))
      value += 2;
    return value;
  case ABILITY_DISCIPLINE: /* NOT SRD! */
    if (GET_RACE(ch) == RACE_H_ELF)
      value += 2;
    value += GET_STR_BONUS(ch);
    if (HAS_REAL_FEAT(ch, FEAT_HONORBOUND))
      value += 4;
    value += compute_gear_armor_penalty(ch);
    return value;
  case ABILITY_TOTAL_DEFENSE: /* not srd */
    value += GET_DEX_BONUS(ch);
    value += compute_gear_armor_penalty(ch);
    if (HAS_FEAT(ch, FEAT_PARRY))
      value += 4;
    return value;
  case ABILITY_LORE: /* NOT SRD! */
  case ABILITY_HISTORY:
  case ABILITY_RELIGION:
    if (HAS_FEAT(ch, FEAT_INVESTIGATOR))
      value += 2;
    if (HAS_FEAT(ch, FEAT_ARTIFICERS_LORE))
      value += 2;
    if (GET_RACE(ch) == RACE_H_ELF)
      value += 2;
    if (HAS_FEAT(ch, FEAT_ELDRITCH_LORE))
      value += 2;
    /* Inquisitor Lore Master: +1 per rank to Arcana, Wisdom, History */
    if (!IS_NPC(ch))
    {
      int lore_master_rank = get_inquisitor_lore_master_rank(ch);
      if (lore_master_rank > 0)
        value += lore_master_rank;
      /* Inquisitor Monster Knowledge: Add Wisdom modifier in addition to Intelligence for lore checks */
      if (has_inquisitor_monster_knowledge(ch))
        value += GET_WIS_BONUS(ch);
      /* Inquisitor Perfect Recall: +4 to all knowledge/lore skills */
      if (has_inquisitor_perfect_recall(ch))
        value += 4;
    }
    value += GET_INT_BONUS(ch);
    return value;
  case ABILITY_RIDE:
    if (!HAS_FEAT(ch, FEAT_LEGENDARY_RIDER))
      value += compute_gear_armor_penalty(ch);
    if (!HAS_FEAT(ch, FEAT_GLORIOUS_RIDER))
      value += GET_DEX_BONUS(ch);
    else
      value += GET_CHA_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_ADEPT_RIDER))
      value += 2;
    if (HAS_FEAT(ch, FEAT_SKILLED_RIDER))
      value += 2;
    if (HAS_FEAT(ch, FEAT_MASTER_RIDER))
      value += 2;
    if (HAS_FEAT(ch, FEAT_ANIMAL_AFFINITY))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if ((mobfol = get_mob_follower(ch, MOB_EIDOLON)))
    {
      if (IN_ROOM(ch) == IN_ROOM(mobfol) && HAS_EVOLUTION(mobfol, EVOLUTION_RIDER_BOND))
        value += MAX(1, GET_CALL_EIDOLON_LEVEL(ch) / 2);
    }
    return value;
  case ABILITY_BOARDING:
    /* Boarding rewards trained line work and close-quarters balance. A
     * character may rely on force or agility, but armor remains a burden. */
    value += MAX(GET_STR_BONUS(ch), GET_DEX_BONUS(ch));
    value += compute_gear_armor_penalty(ch);
    if (HAS_FEAT(ch, FEAT_MINOTAUR_SEAFARING))
      value += 2;
    return value;
  case ABILITY_SLEIGHT_OF_HAND:
    value += GET_DEX_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_KENDER_SKILL_MOD))
      value += 2;
    value += compute_gear_armor_penalty(ch);
    if (HAS_FEAT(ch, FEAT_DEFT_HANDS))
    {
      /* Unnamed bonus */
      value += 3;
    }
    if (HAS_FEAT(ch, FEAT_AGILE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_BLUFF:
    value += GET_CHA_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_PERSUASIVE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_KENDER_SKILL_MOD))
      value += 2;
    if (HAS_FEAT(ch, FEAT_VAMPIRE_SKILL_BONUSES) && CAN_USE_VAMPIRE_ABILITY(ch))
      value += 8;
    return value;
  case ABILITY_DIPLOMACY:
    value += GET_CHA_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_GRUBBY))
      value -= 6;
    if (HAS_FEAT(ch, FEAT_NEGOTIATOR))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_AUTHORITATIVE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    return value;
  case ABILITY_DISABLE_DEVICE:
    value += GET_INT_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_KENDER_SKILL_MOD))
      value += 2;
    if (HAS_FEAT(ch, FEAT_NIMBLE_FINGERS))
    {
      /* Unnamed bonus */
      value += 2;
    }
    /* Inquisitor Investigator's Eye: +3 per rank to Detect Trap checks */
    if (!IS_NPC(ch))
    {
      int investigators_eye_rank = get_inquisitor_investigators_eye_rank(ch);
      if (investigators_eye_rank > 0)
        value += (3 * investigators_eye_rank);
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
    if (HAS_REAL_FEAT(ch, FEAT_HONORBOUND))
      value += 4;
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
    if (HAS_FEAT(ch, FEAT_VAMPIRE_SKILL_BONUSES) && CAN_USE_VAMPIRE_ABILITY(ch))
      value += 8;
    /* Inquisitor Discern Lies: +2 per rank to Sense Motive (opposed Bluff) */
    if (!IS_NPC(ch))
    {
      int discern_lies_rank = get_inquisitor_discern_lies_rank(ch);
      if (discern_lies_rank > 0)
        value += (2 * discern_lies_rank);
    }
    return value;
  case ABILITY_SURVIVAL:
    value += GET_WIS_BONUS(ch);
    if (HAS_FEAT(ch, FEAT_SURVIVAL_INSTINCT))
      value += 3;
    if (HAS_FEAT(ch, FEAT_ARTIFICERS_LORE))
      value += 2;
    if (HAS_FEAT(ch, FEAT_SELF_SUFFICIENT))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_NATURAL_ATHLETE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    /* Inquisitor Lore Master: +1 per rank to Nature (Survival) */
    if (!IS_NPC(ch))
    {
      int lore_master_rank = get_inquisitor_lore_master_rank(ch);
      if (lore_master_rank > 0)
        value += lore_master_rank;
    }
    /* Inquisitor Terrain Mastery: +2 per rank to Survival checks in favored terrain */
    if (!IS_NPC(ch) && is_inquisitor_in_favored_terrain(ch))
    {
      int terrain_mastery_bonus = get_inquisitor_terrain_mastery_survival_bonus(ch);
      if (terrain_mastery_bonus > 0)
        value += terrain_mastery_bonus;
    }
    /* Inquisitor Track and Hunt: double Survival modifier when tracking */
    /* Note: This is context-specific and would be applied in tracking code */
    return value;
  case ABILITY_ATHLETICS:
    value += GET_STR_BONUS(ch);
    value += (2 * compute_gear_armor_penalty(ch));
    if (HAS_FEAT(ch, FEAT_ATHLETIC))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_NATURAL_ATHLETE))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_FEAT(ch, FEAT_MINOTAUR_SEAFARING))
    {
      /* Unnamed bonus */
      value += 2;
    }
    if (HAS_EVOLUTION(ch, EVOLUTION_SWIM))
      value += 20;
    if ((mobfol = get_mob_follower(ch, MOB_EIDOLON)))
    {
      if (IN_ROOM(ch) == IN_ROOM(mobfol) && HAS_EVOLUTION(mobfol, EVOLUTION_SWIM))
        value += 10;
    }
    if (AFF_FLAGGED(ch, AFF_SPIDER_CLIMB))
      value += 30;
    else if (HAS_FEAT(ch, FEAT_VAMPIRE_SPIDER_CLIMB) && CAN_USE_VAMPIRE_ABILITY(ch))
      value += 30;
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

  case ABILITY_LINGUISTICS:
    return value;

    // Crafting Skills
  case ABILITY_CRAFT_WOODWORKING:
  case ABILITY_CRAFT_TAILORING:
  case ABILITY_CRAFT_ALCHEMY:
  case ABILITY_CRAFT_ARMORSMITHING:
  case ABILITY_CRAFT_WEAPONSMITHING:
  case ABILITY_CRAFT_BOWMAKING:
  case ABILITY_CRAFT_JEWELCRAFTING:
  case ABILITY_CRAFT_LEATHERWORKING:
  case ABILITY_CRAFT_TRAPMAKING:
  case ABILITY_CRAFT_POISONMAKING:
  case ABILITY_CRAFT_METALWORKING:
  case ABILITY_CRAFT_FISHING:
  case ABILITY_CRAFT_COOKING:
  case ABILITY_HARVEST_MINING:
  case ABILITY_HARVEST_HUNTING:
  case ABILITY_HARVEST_FORESTRY:
  case ABILITY_HARVEST_GATHERING:
#if !defined(CAMPAIGN_DL)
    value += GET_INT_BONUS(ch);
#endif
    return value;
  default:
    return -1;
  }
}

/** cross-class or not? **/
