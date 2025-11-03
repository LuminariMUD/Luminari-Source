/**************************************************************************
 *  File: mob_spells.c                                Part of LuminariMUD *
 *  Usage: Mobile spell casting functions and data                        *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "fight.h"
#include "mud_event.h"
#include "shop.h"        /* for shop_keeper */
#include "spec_procs.h"  /* for questmaster */
#include "dg_scripts.h"  /* for SCRIPT and TRIGGERS */
#include "quest.h"       /* for questmaster checks */
#include "evolutions.h"  /* for EVOLUTION_UNDEAD_APPEARANCE */
#include "mob_utils.h"
#include "mob_class.h"   /* for npc_class_behave */
#include "mob_psionic.h" /* for psionic functions */
#include "mob_spells.h"
#include "mob_spellslots.h" /* for has_sufficient_slots_for_buff */
#include "assign_wpn_armor.h" /* for weapon_list */

/* local defines */
#define SINFO spell_info[spellnum]
#define OFFENSIVE_SPELLS 60
#define OFFENSIVE_AOE_SPELLS 16
#define OFFENSIVE_AOE_POWERS 6


#if defined(CAMPAIGN_DL)
#define SPELLUP_SPELLS 53
/* list of spells mobiles will use for spellups */
int valid_spellup_spell[SPELLUP_SPELLS] = {
    SPELL_SHIELD_OF_FAITH, // 0
    SPELL_BLESS,
    SPELL_DETECT_ALIGN,
    SPELL_DETECT_INVIS,
    SPELL_DETECT_MAGIC,
    SPELL_DETECT_POISON,
    SPELL_PROT_FROM_EVIL,
    SPELL_SANCTUARY,
    SPELL_STRENGTH,
    SPELL_SENSE_LIFE,
    SPELL_INFRAVISION,
    SPELL_WATERWALK,
    SPELL_FLY,
    SPELL_BLUR,
    SPELL_MIRROR_IMAGE,
    SPELL_STONESKIN,
    SPELL_ENDURANCE,
    SPELL_PROT_FROM_GOOD,
    SPELL_ENDURE_ELEMENTS,
    SPELL_EXPEDITIOUS_RETREAT,
    SPELL_IRON_GUTS,
    SPELL_MAGE_ARMOR,
    SPELL_SHIELD,
    SPELL_TRUE_STRIKE,
    SPELL_FALSE_LIFE,
    SPELL_GRACE,
    SPELL_RESIST_ENERGY,
    SPELL_WATER_BREATHE,
    SPELL_HEROISM,
    SPELL_NON_DETECTION,
    SPELL_HASTE,
    SPELL_CUNNING,
    SPELL_WISDOM,
    SPELL_CHARISMA,
    SPELL_FIRE_SHIELD,
    SPELL_COLD_SHIELD,
    SPELL_MINOR_GLOBE,
    SPELL_GREATER_HEROISM,
    SPELL_TRUE_SEEING,
    SPELL_GLOBE_OF_INVULN,
    SPELL_GREATER_MIRROR_IMAGE,
    SPELL_DISPLACEMENT,
    SPELL_PROTECT_FROM_SPELLS,
    SPELL_SPELL_MANTLE,
    SPELL_IRONSKIN,
    SPELL_MIND_BLANK,
    SPELL_SHADOW_SHIELD,
    SPELL_GREATER_SPELL_MANTLE,
    SPELL_REGENERATION,
    SPELL_DEATH_SHIELD,
    SPELL_BARKSKIN,
    SPELL_SPELL_RESISTANCE,
    SPELL_WATERWALK
  };
#else
/* SPELLUP_SPELLS already defined in mob_spells.h */
/* list of spells mobiles will use for spellups */
int valid_spellup_spell[SPELLUP_SPELLS] = {
    SPELL_SHIELD_OF_FAITH, // 0
    SPELL_BLESS,
    SPELL_DETECT_ALIGN,
    SPELL_DETECT_INVIS,
    SPELL_DETECT_MAGIC,
    SPELL_DETECT_POISON, // 5
    SPELL_INVISIBLE,
    SPELL_PROT_FROM_EVIL,
    SPELL_SANCTUARY,
    SPELL_STRENGTH,
    SPELL_SENSE_LIFE, // 10
    SPELL_INFRAVISION,
    SPELL_WATERWALK,
    SPELL_FLY,
    SPELL_BLUR,
    SPELL_MIRROR_IMAGE, // 15
    SPELL_STONESKIN,
    SPELL_ENDURANCE,
    SPELL_PROT_FROM_GOOD,
    SPELL_ENDURE_ELEMENTS,
    SPELL_EXPEDITIOUS_RETREAT, // 20
    SPELL_IRON_GUTS,
    SPELL_MAGE_ARMOR,
    SPELL_SHIELD,
    SPELL_TRUE_STRIKE,
    SPELL_FALSE_LIFE, // 25
    SPELL_GRACE,
    SPELL_RESIST_ENERGY,
    SPELL_WATER_BREATHE,
    SPELL_HEROISM,
    SPELL_NON_DETECTION, // 30
    SPELL_HASTE,
    SPELL_CUNNING,
    SPELL_WISDOM,
    SPELL_CHARISMA,
    SPELL_FIRE_SHIELD, // 35
    SPELL_COLD_SHIELD,
    SPELL_GREATER_INVIS,
    SPELL_MINOR_GLOBE,
    SPELL_GREATER_HEROISM,
    SPELL_TRUE_SEEING, // 40
    SPELL_GLOBE_OF_INVULN,
    SPELL_GREATER_MIRROR_IMAGE,
    SPELL_DISPLACEMENT,
    SPELL_PROTECT_FROM_SPELLS,
    SPELL_SPELL_MANTLE, // 45
    SPELL_IRONSKIN,
    SPELL_MIND_BLANK,
    SPELL_SHADOW_SHIELD,
    SPELL_GREATER_SPELL_MANTLE,
    SPELL_REGENERATION, // 50
    SPELL_DEATH_SHIELD,
    SPELL_BARKSKIN,
    SPELL_SPELL_RESISTANCE,
    SPELL_WATERWALK};
#endif

/* Wizard/Sorcerer group buff spells (MAG_GROUPS) */
int wizard_group_buff_spells[WIZARD_GROUP_BUFFS] = {
    SPELL_MASS_WISDOM,        // Mass stat buffs
    SPELL_MASS_CHARISMA,
    SPELL_MASS_CUNNING,
    SPELL_MASS_STRENGTH,
    SPELL_MASS_GRACE,
    SPELL_MASS_ENDURANCE,
    SPELL_MASS_HASTE,         // Mass combat buffs
    SPELL_MASS_FLY,
    SPELL_MASS_INVISIBILITY,
    SPELL_CIRCLE_A_EVIL       // Protection circles (MAG_GROUPS)
};

/* Wizard/Sorcerer AoE spells (MAG_AREAS) - offensive focus */
int wizard_aoe_spells[WIZARD_AOE_SPELLS] = {
    SPELL_ICE_STORM,          // Evocation damage
    SPELL_METEOR_SWARM,
    SPELL_CHAIN_LIGHTNING,
    SPELL_PRISMATIC_SPRAY,
    SPELL_INCENDIARY_CLOUD,
    SPELL_HORRID_WILTING,
    SPELL_WAIL_OF_THE_BANSHEE,
    SPELL_MASS_HOLD_PERSON,   // Crowd control
    SPELL_WAVES_OF_EXHAUSTION,
    SPELL_WAVES_OF_FATIGUE,
    SPELL_THUNDERCLAP,
    SPELL_COLOR_SPRAY,
    SPELL_BURNING_HANDS,
    SPELL_FIREBALL,           // Classic AoE
    SPELL_LIGHTNING_BOLT
};

/* list of spells mobiles will use for offense (aoe) */
int valid_aoe_spell[OFFENSIVE_AOE_SPELLS] = {
    /* aoe */
    SPELL_EARTHQUAKE, // 0
    SPELL_ICE_STORM,
    SPELL_METEOR_SWARM,
    SPELL_CHAIN_LIGHTNING,
    SPELL_SYMBOL_OF_PAIN,
    SPELL_MASS_HOLD_PERSON, // 5
    SPELL_PRISMATIC_SPRAY,
    SPELL_THUNDERCLAP,
    SPELL_INCENDIARY_CLOUD,
    SPELL_HORRID_WILTING,
    SPELL_WAIL_OF_THE_BANSHEE, // 10
    SPELL_STORM_OF_VENGEANCE,
    SPELL_CALL_LIGHTNING_STORM,
    SPELL_CREEPING_DOOM,
    SPELL_FIRE_STORM,
    SPELL_SUNBEAM // 15
};

/* list of spells mobiles will use for offense (aoe) */
int valid_aoe_power[OFFENSIVE_AOE_POWERS] = {
    /* aoe */
    PSIONIC_CONCUSSION_BLAST,
    PSIONIC_ENERGY_RETORT,
    PSIONIC_SHRAPNEL_BURST,
    PSIONIC_UPHEAVAL,
    PSIONIC_BREATH_OF_THE_BLACK_DRAGON,
    PSIONIC_ULTRABLAST
};

/* list of spells mobiles will use for offense */
int valid_offensive_spell[OFFENSIVE_SPELLS] = {
    /* single target */
    SPELL_BLINDNESS, // 0
    SPELL_BURNING_HANDS,
    SPELL_CALL_LIGHTNING,
    SPELL_CHILL_TOUCH,
    SPELL_COLOR_SPRAY,
    SPELL_CURSE, // 5
    SPELL_ENERGY_DRAIN,
    SPELL_FIREBALL,
    SPELL_HARM,
    SPELL_LIGHTNING_BOLT,
    SPELL_MAGIC_MISSILE, // 10
    SPELL_POISON,
    SPELL_SHOCKING_GRASP,
    SPELL_CAUSE_LIGHT_WOUNDS,
    SPELL_CAUSE_MODERATE_WOUNDS,
    SPELL_CAUSE_SERIOUS_WOUNDS, // 15
    SPELL_CAUSE_CRITICAL_WOUNDS,
    SPELL_FLAME_STRIKE,
    SPELL_DESTRUCTION,
    SPELL_BALL_OF_LIGHTNING,
    SPELL_MISSILE_STORM, // 20
    SPELL_HORIZIKAULS_BOOM,
    SPELL_ICE_DAGGER,
    SPELL_NEGATIVE_ENERGY_RAY,
    SPELL_RAY_OF_ENFEEBLEMENT,
    SPELL_SCARE, // 25
    SPELL_ACID_ARROW,
    SPELL_DAZE_MONSTER,
    SPELL_HIDEOUS_LAUGHTER,
    SPELL_TOUCH_OF_IDIOCY,
    SPELL_SCORCHING_RAY, // 30
    SPELL_DEAFNESS,
    SPELL_ENERGY_SPHERE,
    SPELL_VAMPIRIC_TOUCH,
    SPELL_HOLD_PERSON,
    SPELL_SLOW, // 35
    SPELL_FEEBLEMIND,
    SPELL_NIGHTMARE,
    SPELL_MIND_FOG,
    SPELL_CONE_OF_COLD,
    SPELL_TELEKINESIS, // 40
    SPELL_FIREBRAND,
    SPELL_FREEZING_SPHERE,
    SPELL_EYEBITE,
    SPELL_GRASPING_HAND,
    SPELL_POWER_WORD_BLIND, // 45
    SPELL_POWER_WORD_STUN,
    SPELL_CLENCHED_FIST,
    SPELL_IRRESISTIBLE_DANCE,
    SPELL_SCINT_PATTERN,
    SPELL_SUNBURST, // 50
    SPELL_WEIRD,
    SPELL_IMPLODE,
    SPELL_FAERIE_FIRE,
    SPELL_FLAMING_SPHERE,
    SPELL_BLIGHT, // 55
    SPELL_FINGER_OF_DEATH,
    SPELL_WHIRLWIND,
    SPELL_HOLD_MONSTER,
    SPELL_POWER_WORD_SILENCE
    };

bool mob_knows_assigned_spells(struct char_data *ch)
{
  int i = 0;

  for (i = 0; i < NUM_SPELLS; i++)
  {
    if (MOB_KNOWS_SPELL(ch, i))
    {
      return true;
    }
  }
  return false;
}

void npc_assigned_spells(struct char_data *ch)
{
  int i = 0, spellnum = 0;
  bool found = false;
  struct char_data *victim = NULL;

  while (!found)
  {
    for (i = 0; i < NUM_SPELLS; i++)
    {
      if (MOB_KNOWS_SPELL(ch, i))
      {
        if (dice(1, 10) == 1)
        {
          found = true;
          spellnum = i;
          break;
        }
      }
    }
  }

  if (spellnum < 0 || spellnum >= NUM_SPELLS)
    return;

  if (spell_info[spellnum].violent == FALSE)
  {
    /* determine victim (someone in group, including self) */
    if (GROUP(ch) && GROUP(ch)->members->iSize)
    {
      victim = (struct char_data *)random_from_list(GROUP(ch)->members);
      if (!victim)
        victim = ch;
    }
    if (!victim)
      victim = ch;
    cast_spell(ch, victim, 0, spellnum, 0);
    return;
  }
  else
  {
    if (!FIGHTING(ch))
      return;
    victim = FIGHTING(ch);
    if (IN_ROOM(ch) != IN_ROOM(victim))
      return;
    cast_spell(ch, victim, 0, spellnum, 0);
  }
}

/* this defines maximum amount of times the function will check the
 spellup array for a valid spell
 note:  npc_offensive_spells() uses this define as well */
#define MAX_LOOPS 40

/* Helper: Check if character is wielding a slashing weapon */
static bool has_slashing_weapon(struct char_data *ch)
{
  struct obj_data *wielded = NULL;
  
  if (!ch)
    return false;
  
  /* Check primary weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (wielded && (GET_OBJ_TYPE(wielded) == ITEM_WEAPON || GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON))
  {
    if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_SLASHING))
      return true;
  }
  
  /* Check two-handed weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_2H);
  if (wielded && (GET_OBJ_TYPE(wielded) == ITEM_WEAPON || GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON))
  {
    if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_SLASHING))
      return true;
  }
  
  /* Check offhand weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_OFFHAND);
  if (wielded && (GET_OBJ_TYPE(wielded) == ITEM_WEAPON || GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON))
  {
    if (IS_SET(weapon_list[GET_OBJ_VAL(wielded, 0)].damageTypes, DAMAGE_TYPE_SLASHING))
      return true;
  }
  
  return false;
}

/* generic function for spelling up as a caster */
void npc_spellup(struct char_data *ch)
{
  struct obj_data *obj = NULL;
  struct char_data *victim = ch;
  int level, spellnum = -1, loop_counter = 0;
  int buff_count = 0;
  /* our priorities are going to be in this order:
   1)  call familiar/eidolon/shadow if appropriate
   2)  get a charmee
   3)  heal (heal group?), condition issues
   4)  spellup (spellup group?)
   */
  if (!ch)
    return;
  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;
  if (!can_continue(ch, FALSE))
    return;
  
  /* Check if we should call a companion first */
  switch (GET_CLASS(ch))
  {
    case CLASS_DRUID:
      if (npc_should_call_companion(ch, MOB_C_ANIMAL))
        perform_call(ch, MOB_C_ANIMAL, GET_LEVEL(ch));
      break;
    case CLASS_WIZARD:
    case CLASS_SORCERER:
      if (npc_should_call_companion(ch, MOB_C_FAMILIAR))
        perform_call(ch, MOB_C_FAMILIAR, GET_LEVEL(ch));
      break;
    case CLASS_SUMMONER:
    case CLASS_NECROMANCER:
      if (npc_should_call_companion(ch, MOB_EIDOLON))
        perform_call(ch, MOB_EIDOLON, GET_LEVEL(ch));
      break;
    case CLASS_SHADOWDANCER:
      if (npc_should_call_companion(ch, MOB_SHADOW))
        perform_call(ch, MOB_SHADOW, GET_LEVEL(ch));
      break;
  }
  
  /* Check buff saturation - count existing defensive buffs */
  if (affected_by_spell(ch, SPELL_STONESKIN)) buff_count++;
  if (affected_by_spell(ch, SPELL_SANCTUARY)) buff_count++;
  if (affected_by_spell(ch, SPELL_SHIELD)) buff_count++;
  if (affected_by_spell(ch, SPELL_MAGE_ARMOR)) buff_count++;
  if (affected_by_spell(ch, SPELL_BLUR)) buff_count++;
  if (affected_by_spell(ch, SPELL_MIRROR_IMAGE)) buff_count++;
  if (affected_by_spell(ch, SPELL_GREATER_MIRROR_IMAGE)) buff_count++;
  if (affected_by_spell(ch, SPELL_DISPLACEMENT)) buff_count++;
  if (affected_by_spell(ch, SPELL_HASTE)) buff_count++;
  if (affected_by_spell(ch, SPELL_GLOBE_OF_INVULN)) buff_count++;
  
  /* If already well-buffed (5+ defensive spells), reduce buff frequency */
  if (buff_count >= 5) {
    if (rand_number(0, 3)) /* 75% chance to skip buffing */
      return;
  }

  /* we're checking spell min-levels so this is a necessity */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    level = LVL_IMMORT - 1;
  else
    level = GET_LEVEL(ch);

  /* try animate undead first - reduced frequency */
  /* UPDATE: plans to add a mob flag for this, for now restrict to mobs
   over level 30 -zusuk */
  if (GET_LEVEL(ch) > 30 && !can_add_follower_by_flag(ch, MOB_ANIMATED_DEAD) && !rand_number(0, 3) && !ch->master &&
      level >= spell_info[SPELL_GREATER_ANIMATION].min_level[GET_CLASS(ch)])
  {
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
    {
      if (!IS_CORPSE(obj))
        continue;
      if (GET_OBJ_VAL(obj, 4)) /* pcorpse */
        continue;

      if (!GROUP(ch))
        create_group(ch);
      cast_spell(ch, NULL, obj, SPELL_GREATER_ANIMATION, 0);
      return;
    }
  }

  /* try for an elemental - reduced frequency and combined level check */
  /* UPDATE: plans to add a mob flag for this, for now restrict to mobs
   over level 30 -zusuk */
  if (GET_LEVEL(ch) > 30 && !can_add_follower_by_flag(ch, MOB_ELEMENTAL) && !rand_number(0, 10) && !ch->master)
  {
    if (level >= spell_info[SPELL_SUMMON_CREATURE_9].min_level[GET_CLASS(ch)])
    {
      if (!GROUP(ch))
        create_group(ch);
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_9, 0);
      return;
    }
    else if (level >= spell_info[SPELL_SUMMON_CREATURE_8].min_level[GET_CLASS(ch)])
    {
      if (!GROUP(ch))
        create_group(ch);
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_8, 0);
      return;
    }
    else if (level >= spell_info[SPELL_SUMMON_CREATURE_7].min_level[GET_CLASS(ch)])
    {
      if (!GROUP(ch))
        create_group(ch);
      cast_spell(ch, NULL, NULL, SPELL_SUMMON_CREATURE_7, 0);
      return;
    }
  }

  /* determine victim (someone in group, including self) */
  if (GROUP(ch) && GROUP(ch)->members->iSize)
  {
    victim = (struct char_data *)random_from_list(GROUP(ch)->members);
    if (!victim)
      victim = ch;
  }
  /* If no formal group, check for allied mobs in the room (following logic) */
  else if (IS_NPC(ch))
  {
    struct char_data *tch, *best_victim = ch;
    int worst_health_pct = 100;
    
    /* Check all characters in the room for allies that need healing */
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
      if (tch == ch)
        continue;
      if (!IS_NPC(tch))
        continue;
      if (!are_grouped(ch, tch)) /* Use our new function! */
        continue;
      
      /* Calculate health percentage */
      if (GET_HIT(tch) > 0 && GET_MAX_HIT(tch) > 0)
      {
        int health_pct = (GET_HIT(tch) * 100) / GET_MAX_HIT(tch);
        if (health_pct < worst_health_pct)
        {
          worst_health_pct = health_pct;
          best_victim = tch;
        }
      }
    }
    victim = best_victim;
  }

  /* try healing */
  if (GET_HIT(victim) && (GET_MAX_HIT(victim) / GET_HIT(victim)) >= 2)
  {
    if (GROUP(ch) && level >= spell_info[SPELL_GROUP_HEAL].min_level[GET_CLASS(ch)])
    {
      cast_spell(ch, victim, NULL, SPELL_GROUP_HEAL, 0);
      return;
    }
    else if (level >= spell_info[SPELL_HEAL].min_level[GET_CLASS(ch)])
    {
      cast_spell(ch, victim, NULL, SPELL_HEAL, 0);
      return;
    }
    else if (level >= spell_info[SPELL_CURE_CRITIC].min_level[GET_CLASS(ch)])
    {
      cast_spell(ch, victim, NULL, SPELL_CURE_CRITIC, 0);
      return;
    }
  }

  /* try to fix condition issues (blindness, etc) */
  /* TODO */

  /* Priority buffs - try important combat buffs first */
  int priority_buffs[] = {
    SPELL_STONESKIN, SPELL_SANCTUARY, SPELL_HASTE, 
    SPELL_GLOBE_OF_INVULN, SPELL_MIRROR_IMAGE, SPELL_BLUR
  };
  int i;
  
  for (i = 0; i < 6; i++) {
    if (level >= spell_info[priority_buffs[i]].min_level[GET_CLASS(ch)] &&
        !affected_by_spell(victim, priority_buffs[i])) {
      spellnum = priority_buffs[i];
      loop_counter = 0; /* found priority buff */
      break;
    }
  }
  
  /* If no priority buff needed, pick random buff */
  if (spellnum == -1) {
    do
    {
      spellnum = valid_spellup_spell[rand_number(0, SPELLUP_SPELLS - 1)];
      loop_counter++;
      if (loop_counter >= (MAX_LOOPS))
        break;
    } while (level < spell_info[spellnum].min_level[GET_CLASS(ch)] || 
             affected_by_spell(victim, spellnum) ||
             !has_sufficient_slots_for_buff(ch, spellnum)); /* Don't buff if low on slots */
  }
  /* If we found a priority buff, still check if we have sufficient slots */
  else if (!has_sufficient_slots_for_buff(ch, spellnum)) {
    spellnum = -1; /* Cancel casting if insufficient slots */
    loop_counter = MAX_LOOPS; /* Skip to end */
  }

  /* we're putting some special restrictions here */

  // we don't want wizards going invisible unless it's on their dedicated spell list.
  if (spellnum == SPELL_INVISIBLE && !ch->mob_specials.spells_known[spellnum])
    return;
  if (spellnum == SPELL_GREATER_INVIS && !ch->mob_specials.spells_known[spellnum])
    return;

  if (IS_MOB(ch) && mob_index[GET_MOB_RNUM(ch)].func == shop_keeper &&
      (spellnum == SPELL_GREATER_INVIS ||
       spellnum == SPELL_INVISIBLE))
  {
    /* shopkeepers invising themselves is silly :p  -zusuk */
    return;
  }

  /* mobs with triggers / autoquest / quest will no longer invis */
  if (IS_MOB(ch) && ((SCRIPT(ch) && TRIGGERS(SCRIPT(ch))) || (ch->mob_specials.quest) || (GET_MOB_SPEC(ch) == questmaster)) &&
      (spellnum == SPELL_GREATER_INVIS ||
       spellnum == SPELL_INVISIBLE))
  {
    /* these type of mobs casting invis is problematic */
    return;
  }

  /* casters that serve as mounts no longer cast invis */
  if (IS_MOB(ch) && MOB_FLAGGED(ch, MOB_C_MOUNT) &&
      AFF_FLAGGED(ch, AFF_CHARM) &&
      (spellnum == SPELL_GREATER_INVIS ||
       spellnum == SPELL_INVISIBLE))
  {
    /* these type of mobs casting invis is problematic */
    return;
  }

  // charmies should not cast invis
  if (IS_MOB(ch) && AFF_FLAGGED(ch, AFF_CHARM) &&
      (spellnum == SPELL_GREATER_INVIS ||
       spellnum == SPELL_INVISIBLE))
  {
    return;
  }

  /* keen edge requires a slashing weapon */
  if (spellnum == SPELL_KEEN_EDGE && !has_slashing_weapon(ch))
    return;

  /* end special restrictions */

  if (loop_counter < (MAX_LOOPS))
    // found a spell, cast it
    cast_spell(ch, victim, NULL, spellnum, 0);

  return;
}


/* note MAX_LOOPS used here too */

void npc_offensive_spells(struct char_data *ch)
{
  struct char_data *tch = NULL;
  int level, spellnum = -1, loop_counter = 0;
  int use_aoe = 0;

  if (!ch)
    return;

  if (MOB_FLAGGED(ch, MOB_NOCLASS))
    return;

  /* capping */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    level = LVL_IMMORT - 1;
  else
    level = GET_LEVEL(ch);

  /* Check if we should call a companion first when in combat */
  switch (GET_CLASS(ch))
  {
    case CLASS_DRUID:
      if (npc_should_call_companion(ch, MOB_C_ANIMAL))
      {
        perform_call(ch, MOB_C_ANIMAL, GET_LEVEL(ch));
        return;
      }
      break;
    case CLASS_WIZARD:
    case CLASS_SORCERER:
      if (npc_should_call_companion(ch, MOB_C_FAMILIAR))
      {
        perform_call(ch, MOB_C_FAMILIAR, GET_LEVEL(ch));
        return;
      }
      break;
    case CLASS_SUMMONER:
    case CLASS_NECROMANCER:
      if (npc_should_call_companion(ch, MOB_EIDOLON))
      {
        perform_call(ch, MOB_EIDOLON, GET_LEVEL(ch));
        return;
      }
      break;
    case CLASS_SHADOWDANCER:
      if (npc_should_call_companion(ch, MOB_SHADOW))
      {
        perform_call(ch, MOB_SHADOW, GET_LEVEL(ch));
        return;
      }
      break;
    case CLASS_DRAGONRIDER:
      if (npc_should_call_companion(ch, MOB_C_DRAGON))
      {
        perform_call(ch, MOB_C_DRAGON, GET_LEVEL(ch));
        return;
      }
      break;
  }

  /* 25% of spellup instead of offensive spell */
  if (!rand_number(0, 3))
  {
    npc_spellup(ch);
    return;
  }

  /* our semi-casters will rarely use this function */
  switch (GET_CLASS(ch))
  {
  case CLASS_RANGER:
  case CLASS_PALADIN: // 10 out of 11 times will not cast
    if (rand_number(0, 10))
    {
      npc_class_behave(ch);
      return;
    }
    break;
  case CLASS_BARD: // bards 33% will not cast
    if (!rand_number(0, 2))
    {
      npc_class_behave(ch);
      return;
    }
    break;
  case CLASS_DRUID:
    /* our 'healing' types will do another check for spellup */
    /* additional 25% of spellup instead of offensive spell */
    if (!rand_number(0, 3))
    {
      npc_spellup(ch);
      return;
    }

    break;

  case CLASS_CLERIC:
    /* our 'healing' types will do another check for spellup */
    /* additional 25% of spellup instead of offensive spell */
    if (!rand_number(0, 3))
    {
      npc_spellup(ch);
      return;
    }

    break;
  }

  /* find random target, and num targets */
  if (!(tch = npc_find_target(ch, &use_aoe)))
    return;

  /* random offensive spell */
  if (use_aoe >= 2)
  {
    do
    {
      spellnum = valid_aoe_spell[rand_number(0, OFFENSIVE_AOE_SPELLS - 1)];
      loop_counter++;
      if (loop_counter >= (MAX_LOOPS / 2))
        break;
    } while (level < spell_info[spellnum].min_level[GET_CLASS(ch)]);

    if (loop_counter < (MAX_LOOPS / 2) && spellnum != -1)
    {
      // found a spell, cast it
      cast_spell(ch, tch, NULL, spellnum, 0);
      return;
    }
  }

  /* our clerics will try to turn here, paladins do it in paladin_behave() */
  if (GET_CLASS(ch) == CLASS_CLERIC && !rand_number(0, 2) && IS_UNDEAD(tch))
  {
    perform_turnundead(ch, tch, GET_LEVEL(ch));
    return;
  }

  /* we intentionally fall through here,
   some (a lot?) of mobiles will not have aoe spells */
  loop_counter = 0;

  do
  {
    spellnum = valid_offensive_spell[rand_number(0, OFFENSIVE_SPELLS - 1)];
    loop_counter++;
    if (loop_counter >= (MAX_LOOPS / 2))
      break;
  } while (level < spell_info[spellnum].min_level[GET_CLASS(ch)] ||
           affected_by_spell(tch, spellnum));

  if (loop_counter < (MAX_LOOPS / 2) && spellnum != -1)
    // found a spell, cast it
    cast_spell(ch, tch, NULL, spellnum, 0);

  return;
}

/* Wizard AI: Check if a spell is suitable for pre-buffing (long duration) */
bool wizard_is_long_duration_buff(int spellnum)
{
  /* Only cast buffs with duration > 1 round per level */
  /* These are beneficial MAG_AFFECTS spells with extended durations */
  /* Check if spell has MAG_AFFECTS routine (it's a buff/debuff spell) */
  if (!(SINFO.routines & MAG_AFFECTS))
    return false;
  
  /* Check if spell is violent (harmful) - we only want beneficial buffs */
  if (SINFO.violent)
    return false;

  /* List of wizard buff spells with duration > 1 round/level */
  switch (spellnum)
  {
    /* Fixed long durations (300-600 rounds) */
    case SPELL_MAGE_ARMOR:           /* 600 rounds */
    case SPELL_STONESKIN:            /* 600 rounds */
    case SPELL_RESIST_ENERGY:        /* 600 rounds */
    case SPELL_FLY:                  /* 600 rounds */
    case SPELL_ENDURE_ELEMENTS:      /* 600 rounds */
    case SPELL_PROT_FROM_EVIL:       /* 600 rounds */
    case SPELL_PROT_FROM_GOOD:       /* 600 rounds */
    case SPELL_MIRROR_IMAGE:         /* 300 rounds */
    case SPELL_BLUR:                 /* 300 rounds */
    case SPELL_SHIELD:               /* 300 rounds */
    case SPELL_FALSE_LIFE:           /* 300 rounds */
    case SPELL_DISPLACEMENT:         /* 100 rounds */
    
    /* Level-based long durations (level * 10+) */
    case SPELL_HASTE:                /* level * 12 */
    case SPELL_STRENGTH:             /* level * 12 + 100 */
    case SPELL_ENDURANCE:            /* level * 12 + 100 */
    case SPELL_GRACE:                /* level * 12 + 100 */
    case SPELL_CUNNING:              /* level * 12 + 100 */
    case SPELL_WISDOM:               /* level * 12 + 100 */
    case SPELL_CHARISMA:             /* level * 12 + 100 */
    case SPELL_PROTECTION_FROM_ENERGY:  /* 10 * 10 * level */
    case SPELL_GREATER_MAGIC_WEAPON:    /* 60 * 10 * level */
    case SPELL_KEEN_EDGE:               /* 10 * 10 * level */
    
    /* Detection/utility long durations */
    case SPELL_DETECT_INVIS:         /* 300 + level * 25 */
    case SPELL_DETECT_MAGIC:         /* 300 + level * 25 */
    case SPELL_INVISIBLE:            /* 300 + level * 6 */
    case SPELL_NON_DETECTION:        /* 25 + level * 12 */
    case SPELL_SPELL_RESISTANCE:     /* 50 + level */
    
    /* Circle spells (if wizard has them) */
    case SPELL_CIRCLE_A_EVIL:        /* Similar to prot from evil */
    case SPELL_CIRCLE_A_GOOD:        /* Similar to prot from good */
    
    /* Other long-duration defensive spells */
    case SPELL_FIRE_SHIELD:          /* If wizard has it */
    case SPELL_COLD_SHIELD:          /* If wizard has it */
    case SPELL_MINOR_GLOBE:          /* If wizard has it */
    case SPELL_GLOBE_OF_INVULN:      /* If wizard has it */
    case SPELL_GREATER_MIRROR_IMAGE: /* If wizard has it */
    case SPELL_SPELL_MANTLE:         /* If wizard has it */
    case SPELL_GREATER_SPELL_MANTLE: /* If wizard has it */
    case SPELL_SHADOW_SHIELD:        /* If wizard has it */
    case SPELL_MIND_BLANK:           /* If wizard has it */
    case SPELL_IRONSKIN:             /* If wizard has it */
      return true;
    default:
      return false;
  }
}

/* Wizard AI: Categorize spell by type */
int wizard_get_spell_category(int spellnum)
{
  /* MAG_DAMAGE - direct damage spells */
  if (SINFO.routines & MAG_DAMAGE)
    return MAG_DAMAGE;
    
  /* MAG_SUMMONS - summoning spells */
  if (SINFO.routines & MAG_SUMMONS)
    return MAG_SUMMONS;
    
  /* MAG_POINTS - healing/restoration spells */
  if (SINFO.routines & MAG_POINTS)
    return MAG_POINTS;
    
  /* MAG_AFFECTS - buffs and debuffs */
  if (SINFO.routines & MAG_AFFECTS)
    return MAG_AFFECTS;
    
  return 0;
}

/* Wizard AI: Cast pre-combat buffs */
void wizard_cast_prebuff(struct char_data *ch)
{
  int spellnum = -1;
  int loop_counter = 0;
  int level = MIN(GET_LEVEL(ch), LVL_IMMORT - 1);
  int char_class = GET_CLASS(ch);
  struct char_data *tch;
  bool has_allies = FALSE;
  
  if (!ch || !IS_MOB(ch))
    return;
  
  /* Default to wizard if not a recognized arcane caster */
  if (char_class != CLASS_WIZARD && char_class != CLASS_SORCERER)
    char_class = CLASS_WIZARD;
  
  /* Check if we have allies in the room (formal group or followers) */
  if (GROUP(ch) && GROUP(ch)->members->iSize > 1)
  {
    has_allies = TRUE;
  }
  else if (IS_NPC(ch))
  {
    /* Check for allied mobs in the room */
    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
      if (tch != ch && are_grouped(ch, tch))
      {
        has_allies = TRUE;
        break;
      }
    }
  }
  
  /* 30% chance to cast group buff if we have allies */
  if (has_allies && !rand_number(0, 2))
  {
    do
    {
      spellnum = wizard_group_buff_spells[rand_number(0, WIZARD_GROUP_BUFFS - 1)];
      loop_counter++;
      
      if (loop_counter >= (MAX_LOOPS / 2))
        break;
        
    } while (level < SINFO.min_level[char_class] ||
             affected_by_spell(ch, spellnum) || /* Don't recast if already affected */
             !has_sufficient_slots_for_buff(ch, spellnum)); /* Don't buff if low on slots */
    
    if (loop_counter < (MAX_LOOPS / 2) && spellnum != -1)
    {
      cast_spell(ch, ch, NULL, spellnum, 0);
      return;
    }
  }
    
  /* Try to find a long-duration self-buff we don't already have */
  loop_counter = 0;
  do
  {
    /* Pick a random spell from our spellup list */
    spellnum = valid_spellup_spell[rand_number(0, SPELLUP_SPELLS - 1)];
    loop_counter++;
    
    if (loop_counter >= (MAX_LOOPS / 2))
      break;
      
  } while (level < SINFO.min_level[char_class] ||
           !wizard_is_long_duration_buff(spellnum) ||
           affected_by_spell(ch, spellnum) ||
           !has_sufficient_slots_for_buff(ch, spellnum) || /* Don't buff if low on slots */
           (spellnum == SPELL_KEEN_EDGE && !has_slashing_weapon(ch))); /* Keen edge needs slashing weapon */
  
  if (loop_counter < (MAX_LOOPS / 2) && spellnum != -1)
  {
    cast_spell(ch, ch, NULL, spellnum, 0);
  }
}

/* Wizard AI: Intelligent combat spell selection */
void wizard_combat_ai(struct char_data *ch)
{
  struct char_data *tch = NULL;
  int level, spellnum = -1, loop_counter = 0;
  int use_aoe = 0;
  int spell_category = 0;
  int category_choice = 0;
  int char_class = GET_CLASS(ch);
  
  if (!ch)
    return;
  
  /* Default to wizard if not a recognized arcane caster */
  if (char_class != CLASS_WIZARD && char_class != CLASS_SORCERER)
    char_class = CLASS_WIZARD;
    
  /* Cap level */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    level = LVL_IMMORT - 1;
  else
    level = GET_LEVEL(ch);
  
  /* Check if we should call familiar first */
  if (npc_should_call_companion(ch, MOB_C_FAMILIAR))
  {
    perform_call(ch, MOB_C_FAMILIAR, GET_LEVEL(ch));
    return;
  }
  
  /* 20% chance to buff in combat */
  if (!rand_number(0, 4))
  {
    npc_spellup(ch);
    return;
  }
  
  /* Find target and determine if we should use AoE */
  if (!(tch = npc_find_target(ch, &use_aoe)))
    return;
  
  /* Decide what type of spell to cast based on situation */
  /* 70% damage, 20% debuff, 5% summon, 5% heal */
  category_choice = rand_number(1, 100);
  
  if (category_choice <= 70)
    spell_category = MAG_DAMAGE;
  else if (category_choice <= 90)
    spell_category = MAG_AFFECTS;
  else if (category_choice <= 95)
    spell_category = MAG_SUMMONS;
  else
    spell_category = MAG_POINTS;
  
  /* If fighting multiple enemies (2+), use wizard AoE spells */
  if (use_aoe >= 2)
  {
    do
    {
      spellnum = wizard_aoe_spells[rand_number(0, WIZARD_AOE_SPELLS - 1)];
      loop_counter++;
      if (loop_counter >= (MAX_LOOPS / 2))
        break;
    } while (level < SINFO.min_level[char_class]);
    
    if (loop_counter < (MAX_LOOPS / 2) && spellnum != -1)
    {
      cast_spell(ch, tch, NULL, spellnum, 0);
      return;
    }
  }
  
  /* Find a spell matching our chosen category */
  loop_counter = 0;
  do
  {
    spellnum = valid_offensive_spell[rand_number(0, OFFENSIVE_SPELLS - 1)];
    loop_counter++;
    
    if (loop_counter >= (MAX_LOOPS / 2))
      break;
      
  } while (level < SINFO.min_level[char_class] ||
           affected_by_spell(tch, spellnum) ||
           (wizard_get_spell_category(spellnum) != spell_category && spell_category != 0));
  
  if (loop_counter < (MAX_LOOPS / 2) && spellnum != -1)
  {
    cast_spell(ch, tch, NULL, spellnum, 0);
  }
}

