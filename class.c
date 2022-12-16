/**************************************************************************
 *  File: class.c                                      Part of LuminariMUD *
 *  Usage: Source file for class-specific code.                            *
 *  Author:  Circle, rewritten by Zusuk                                    *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

/** Help buffer the global variable definitions */
#define __CLASS_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "handler.h"
#include "comm.h"
#include "mud_event.h"
#include "mudlim.h"
#include "feats.h"
#include "class.h"
#include "assign_wpn_armor.h"
#include "pfdefaults.h"
#include "domains_schools.h"
#include "modify.h"
#include "spell_prep.h"
#include "race.h"
#include "alchemy.h"
#include "premadebuilds.h"

/** LOCAL DEFINES **/
// good/bad
#define G 1 // good
#define B 0 // bad
// yes/no
#define Y 1 // yes
#define N 0 // no
// high/medium/low
#define H 2 // high
#define M 1 // medium
#define L 0 // low
// spell vs skill
#define SP 0 // spell
#define SK 1 // skill
// skill availability by class
#define NA 0 // not available
#define CC 1 // cross class
#define CA 2 // class ability
/* absolute xp cap */
#define EXP_MAX 2100000000
/* modes for dealing with class listing / restrictions */
#define MODE_CLASSLIST_NORMAL 0
#define MODE_CLASSLIST_RESPEC 1

/* here is our class_list declare */
struct class_table class_list[NUM_CLASSES];

/* SET OF UTILITY FUNCTIONS for the purpose of class prereqs */

/* create/allocate memory for a pre-req struct, then assign the prereqs */
struct class_prerequisite *create_prereq(int prereq_type, int val1,
                                         int val2, int val3)
{
  struct class_prerequisite *prereq = NULL;

  CREATE(prereq, struct class_prerequisite, 1);
  prereq->prerequisite_type = prereq_type;
  prereq->values[0] = val1;
  prereq->values[1] = val2;
  prereq->values[2] = val3;

  return prereq;
}

void class_prereq_attribute(int class_num, int attribute, int value)
{
  struct class_prerequisite *prereq = NULL;
  char buf[80];

  const char *attribute_abbr[7] = {
      "None",
      "Str",
      "Dex",
      "Int",
      "Wis",
      "Con",
      "Cha"};

  prereq = create_prereq(CLASS_PREREQ_ATTRIBUTE, attribute, value, 0);

  /* Generate the description. */
  snprintf(buf, sizeof(buf), "%s: %d", attribute_abbr[attribute], value);
  prereq->description = strdup(buf);

  /*  Link it up. */
  prereq->next = class_list[class_num].prereq_list;
  class_list[class_num].prereq_list = prereq;
}

void class_prereq_class_level(int class_num, int cl, int level)
{
  struct class_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prereq(CLASS_PREREQ_CLASS_LEVEL, cl, level, 0);

  /* Generate the description */
  snprintf(buf, sizeof(buf), "%s level %d", CLSLIST_NAME(cl), level);
  prereq->description = strdup(buf);

  /* Link it up */
  prereq->next = class_list[class_num].prereq_list;
  class_list[class_num].prereq_list = prereq;
}

void class_prereq_feat(int class_num, int feat, int ranks)
{
  struct class_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prereq(CLASS_PREREQ_FEAT, feat, ranks, 0);

  /* Generate the description. */
  if (ranks > 1)
    snprintf(buf, sizeof(buf), "%s (%d ranks)", feat_list[feat].name, ranks);
  else
    snprintf(buf, sizeof(buf), "%s", feat_list[feat].name);

  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = class_list[class_num].prereq_list;
  class_list[class_num].prereq_list = prereq;
}

void class_prereq_cfeat(int class_num, int feat, int special)
{
  struct class_prerequisite *prereq = NULL;
  char buf[80];

  const char *cfeat_special[NUM_CFEAT_SPECIAL] = {
      "no special circumstance",
      "in ranged weapons",
  };

  prereq = create_prereq(CLASS_PREREQ_CFEAT, feat, special, 0);

  snprintf(buf, sizeof(buf), "%s (%s)", feat_list[feat].name, cfeat_special[special]);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = class_list[class_num].prereq_list;
  class_list[class_num].prereq_list = prereq;
}

void class_prereq_ability(int class_num, int ability, int ranks)
{
  struct class_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prereq(CLASS_PREREQ_ABILITY, ability, ranks, 0);

  snprintf(buf, sizeof(buf), "%d ranks in %s", ranks, ability_names[ability]);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = class_list[class_num].prereq_list;
  class_list[class_num].prereq_list = prereq;
}

void class_prereq_spellcasting(int class_num, int casting_type, int prep_type, int circle)
{
  struct class_prerequisite *prereq = NULL;
  char buf[80];

  const char *casting_types[4] = {
      "None",
      "Arcane",
      "Divine",
      "Any"};

  const char *spell_preparation_types[4] = {
      "None",
      "Prepared",
      "Spontaneous",
      "Any"};

  prereq = create_prereq(CLASS_PREREQ_SPELLCASTING, casting_type, prep_type, circle);

  snprintf(buf, sizeof(buf), "Has %s %s (circle) %d spells", casting_types[casting_type],
           spell_preparation_types[prep_type], circle);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = class_list[class_num].prereq_list;
  class_list[class_num].prereq_list = prereq;
}

void class_prereq_race(int class_num, int race)
{
  struct class_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prereq(CLASS_PREREQ_RACE, race, 0, 0);

  snprintf(buf, sizeof(buf), "Race: %s", race_list[race].type);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = class_list[class_num].prereq_list;
  class_list[class_num].prereq_list = prereq;
}

void class_prereq_bab(int class_num, int bab)
{
  struct class_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prereq(CLASS_PREREQ_BAB, bab, 0, 0);

  snprintf(buf, sizeof(buf), "Min. BAB +%d", bab);
  prereq->description = strdup(buf);

  /* Link it up */
  prereq->next = class_list[class_num].prereq_list;
  class_list[class_num].prereq_list = prereq;
}

/* alignment is a list of RESTRICTED alignments */
void class_prereq_align(int class_num, int alignment)
{
  struct class_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prereq(CLASS_PREREQ_ALIGN, alignment, 0, 0);

  /* #define LAWFUL_GOOD         0
     #define NEUTRAL_GOOD        1
     #define CHAOTIC_GOOD        2
     #define LAWFUL_NEUTRAL      3
     #define TRUE_NEUTRAL        4
     #define CHAOTIC_NEUTRAL     5
     #define LAWFUL_EVIL         6
     #define NEUTRAL_EVIL        7
     #define CHAOTIC_EVIL        8 */

  snprintf(buf, sizeof(buf), "Align: %s", alignment_names_nocolor[alignment]);
  prereq->description = strdup(buf);

  /* Link it up */
  prereq->next = class_list[class_num].prereq_list;
  class_list[class_num].prereq_list = prereq;
}

void class_prereq_weapon_proficiency(int class_num)
{
  struct class_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prereq(CLASS_PREREQ_WEAPON_PROFICIENCY, 0, 0, 0);

  snprintf(buf, sizeof(buf), "Proficiency in same weapon");
  prereq->description = strdup(buf);

  /*  Link it up */
  prereq->next = class_list[class_num].prereq_list;
  class_list[class_num].prereq_list = prereq;
}

/* our little mini struct series for assigning spells to a class and to assigning
   minimum-level for those spells */

/* create/allocate memory for the spellassign struct */
struct class_spell_assign *create_spell_assign(int spell_num, int level)
{
  struct class_spell_assign *spell_asign = NULL;

  CREATE(spell_asign, struct class_spell_assign, 1);
  spell_asign->spell_num = spell_num;
  spell_asign->level = level;

  return spell_asign;
}

/* actual function called to perform the spell assignment */
void spell_assignment(int class_num, int spell_num, int level)
{
  struct class_spell_assign *spell_asign = NULL;

  spell_asign = create_spell_assign(spell_num, level);

  /*   Link it up. */
  spell_asign->next = class_list[class_num].spellassign_list;
  class_list[class_num].spellassign_list = spell_asign;
}

/* our little mini struct series for assigning feats to a class and to assigning
   class-feats to a class */

/* create/allocate memory for the spellassign struct */
struct class_feat_assign *create_feat_assign(int feat_num, bool is_classfeat,
                                             int level_received, bool stacks)
{
  struct class_feat_assign *feat_assign = NULL;

  CREATE(feat_assign, struct class_feat_assign, 1);
  feat_assign->feat_num = feat_num;
  feat_assign->is_classfeat = is_classfeat;
  feat_assign->level_received = level_received;
  feat_assign->stacks = stacks;

  return feat_assign;
}
/* actual function called to perform the feat assignment */

/* when assigning class feats use this format:
   feat_assignment(CLASS_blah, FEAT_blah_blah, Y, NOASSIGN_FEAT, N); */
void feat_assignment(int class_num, int feat_num, bool is_classfeat,
                     int level_received, bool stacks)
{
  struct class_feat_assign *feat_assign = NULL;

  feat_assign = create_feat_assign(feat_num, is_classfeat, level_received, stacks);

  /*   Link it up. */
  feat_assign->next = class_list[class_num].featassign_list;
  class_list[class_num].featassign_list = feat_assign;
}

/* function that will assign a list of values to a given class */
void classo(int class_num, const char *name, const char *abbrev, const char *colored_abbrev,
            const char *menu_name, int max_level, bool locked_class, int prestige_class,
            int base_attack_bonus, int hit_dice, int psp_gain, int move_gain,
            int trains_gain, bool in_game, int unlock_cost, int epic_feat_progression,
            const char *spell_prog, const char *primary_attribute, const char *descrip)
{
  class_list[class_num].name = name;
  class_list[class_num].abbrev = abbrev;
  class_list[class_num].colored_abbrev = colored_abbrev;
  class_list[class_num].menu_name = menu_name;
  class_list[class_num].max_level = max_level;
  class_list[class_num].locked_class = locked_class;
  class_list[class_num].prestige_class = prestige_class;
  class_list[class_num].base_attack_bonus = base_attack_bonus;
  class_list[class_num].hit_dice = hit_dice;
  class_list[class_num].psp_gain = psp_gain;
  class_list[class_num].move_gain = move_gain;
  class_list[class_num].trains_gain = trains_gain;
  class_list[class_num].in_game = in_game;
  class_list[class_num].unlock_cost = unlock_cost;
  class_list[class_num].epic_feat_progression = epic_feat_progression;
  class_list[class_num].prestige_spell_progression = spell_prog;
  class_list[class_num].primary_attribute = primary_attribute;
  class_list[class_num].descrip = descrip;
  /* list of prereqs */
  class_list[class_num].prereq_list = NULL;
  /* list of spell assignments */
  class_list[class_num].spellassign_list = NULL;
  /* list of feat assignments */
  class_list[class_num].featassign_list = NULL;
}

/* function used for assigning a classes titles */
static void assign_class_titles(
    int class_num, const char *title_4, const char *title_9, const char *title_14,
    const char *title_19, const char *title_24, const char *title_29, const char *title_30, const char *title_imm,
    const char *title_stf, const char *title_gstf, const char *title_default)
{
  class_list[class_num].titles[0] = title_4;
  class_list[class_num].titles[1] = title_9;
  class_list[class_num].titles[2] = title_14;
  class_list[class_num].titles[3] = title_19;
  class_list[class_num].titles[4] = title_24;
  class_list[class_num].titles[5] = title_29;
  class_list[class_num].titles[6] = title_30;
  class_list[class_num].titles[7] = title_imm;
  class_list[class_num].titles[8] = title_stf;
  class_list[class_num].titles[9] = title_gstf;
  class_list[class_num].titles[10] = title_default;
}

/* function used for assigned a classes 'preferred' saves */
void assign_class_saves(int class_num, int save_fort, int save_refl, int save_will,
                        int save_posn, int save_deth)
{
  class_list[class_num].preferred_saves[SAVING_FORT] = save_fort;
  class_list[class_num].preferred_saves[SAVING_REFL] = save_refl;
  class_list[class_num].preferred_saves[SAVING_WILL] = save_will;
  class_list[class_num].preferred_saves[SAVING_POISON] = save_posn;
  class_list[class_num].preferred_saves[SAVING_DEATH] = save_deth;
}

/* function used for assigning whether a given ability is not-available, cross-class
 or class-skill */
void assign_class_abils(int class_num,
                        int acrobatics, int stealth, int perception, int heal, int intimidate,
                        int concentration, int spellcraft, int appraise, int discipline,
                        int total_defense, int lore, int ride, int climb, int sleight_of_hand,
                        int bluff, int diplomacy, int disable_device, int disguise, int escape_artist,
                        int handle_animal, int sense_motive, int survival, int swim, int use_magic_device,
                        int perform)
{
  class_list[class_num].class_abil[ABILITY_ACROBATICS] = acrobatics;
  class_list[class_num].class_abil[ABILITY_STEALTH] = stealth;
  class_list[class_num].class_abil[ABILITY_PERCEPTION] = perception;
  class_list[class_num].class_abil[ABILITY_HEAL] = heal;
  class_list[class_num].class_abil[ABILITY_INTIMIDATE] = intimidate;
  class_list[class_num].class_abil[ABILITY_CONCENTRATION] = concentration;
  class_list[class_num].class_abil[ABILITY_SPELLCRAFT] = spellcraft;
  class_list[class_num].class_abil[ABILITY_APPRAISE] = appraise;
  class_list[class_num].class_abil[ABILITY_DISCIPLINE] = discipline;
  class_list[class_num].class_abil[ABILITY_TOTAL_DEFENSE] = total_defense;
  class_list[class_num].class_abil[ABILITY_LORE] = lore;
  class_list[class_num].class_abil[ABILITY_RIDE] = ride;
  class_list[class_num].class_abil[ABILITY_CLIMB] = climb;
  class_list[class_num].class_abil[ABILITY_SLEIGHT_OF_HAND] = sleight_of_hand;
  class_list[class_num].class_abil[ABILITY_BLUFF] = bluff;
  class_list[class_num].class_abil[ABILITY_DIPLOMACY] = diplomacy;
  class_list[class_num].class_abil[ABILITY_DISABLE_DEVICE] = disable_device;
  class_list[class_num].class_abil[ABILITY_DISGUISE] = disguise;
  class_list[class_num].class_abil[ABILITY_ESCAPE_ARTIST] = escape_artist;
  class_list[class_num].class_abil[ABILITY_HANDLE_ANIMAL] = handle_animal;
  class_list[class_num].class_abil[ABILITY_SENSE_MOTIVE] = sense_motive;
  class_list[class_num].class_abil[ABILITY_SURVIVAL] = survival;
  class_list[class_num].class_abil[ABILITY_SWIM] = swim;
  class_list[class_num].class_abil[ABILITY_USE_MAGIC_DEVICE] = use_magic_device;
  class_list[class_num].class_abil[ABILITY_PERFORM] = perform;
  class_list[class_num].class_abil[ABILITY_LINGUISTICS] = CA;
}

/* function to give default values for a class before assignment */
void init_class_list(int class_num)
{
  class_list[class_num].name = "unusedclass";
  class_list[class_num].abbrev = "???";
  class_list[class_num].colored_abbrev = "???";
  class_list[class_num].menu_name = "???";
  class_list[class_num].max_level = -1;
  class_list[class_num].locked_class = N;
  class_list[class_num].prestige_class = N;
  class_list[class_num].base_attack_bonus = L;
  class_list[class_num].hit_dice = 4;
  class_list[class_num].psp_gain = 0;
  class_list[class_num].move_gain = 0;
  class_list[class_num].trains_gain = 2;
  class_list[class_num].in_game = N;
  class_list[class_num].unlock_cost = 0;
  class_list[class_num].epic_feat_progression = 5;
  class_list[class_num].prestige_spell_progression = "no progression";
  class_list[class_num].primary_attribute = "no primary attribute";
  class_list[class_num].descrip = "undescribed class";

  int i = 0;
  for (i = 0; i < NUM_PREFERRED_SAVES; i++)
    class_list[class_num].preferred_saves[i] = B;
  for (i = 0; i < NUM_ABILITIES; i++)
    class_list[class_num].class_abil[i] = NA;
  for (i = 0; i < MAX_NUM_TITLES; i++)
    class_list[class_num].titles[i] = "";

  class_list[class_num].prereq_list = NULL;
  class_list[class_num].spellassign_list = NULL;
  class_list[class_num].featassign_list = NULL;
}

/* this was created to handle special scenarios for combat feat requirements
   for classes */
bool has_special_cfeat(struct char_data *ch, int featnum, int mode)
{
  switch (mode)
  {

    /* featnum in any bow */
  case CFEAT_SPECIAL_BOW:
    /*
      if (!HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_LONG_BOW) &&
              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_COMPOSITE_LONGBOW) &&
              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_COMPOSITE_LONGBOW_2) &&
              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_COMPOSITE_LONGBOW_3) &&
              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_COMPOSITE_LONGBOW_4) &&
              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_COMPOSITE_LONGBOW_5) &&

              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_SHORT_BOW) &&
              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_COMPOSITE_SHORTBOW) &&
              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_COMPOSITE_SHORTBOW_2) &&
              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_COMPOSITE_SHORTBOW_3) &&
              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_COMPOSITE_SHORTBOW_4) &&
              !HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum),
              WEAPON_TYPE_COMPOSITE_SHORTBOW_5)
              ) { */
    if (!HAS_COMBAT_FEAT(ch, feat_to_cfeat(featnum), WEAPON_FAMILY_RANGED))
    {
      return FALSE;
    }
    break;

    /* auto pass by default */
  case CFEAT_SPECIAL_NONE:
    break;

  default:
    return FALSE;
  }

  /* success! */
  return TRUE;
}

/* Check to see if ch meets the provided class prerequisite.
   iarg is for external comparison */
bool meets_class_prerequisite(struct char_data *ch, struct class_prerequisite *prereq, int iarg)
{

  switch (prereq->prerequisite_type)
  {

  case CLASS_PREREQ_NONE:
    /* This is a NON-prereq. */
    break;

    /* valid alignments - special handling since list of valids */
  case CLASS_PREREQ_ALIGN:
    if (prereq->values[0] != convert_alignment(GET_ALIGNMENT(ch)))
      return FALSE;
    break;

    /* minimum stats */
  case CLASS_PREREQ_ATTRIBUTE:
    switch (prereq->values[0])
    {
    case AB_STR:
      if (GET_REAL_STR(ch) < prereq->values[1])
        return FALSE;
      break;
    case AB_DEX:
      if (GET_REAL_DEX(ch) < prereq->values[1])
        return FALSE;
      break;
    case AB_CON:
      if (GET_REAL_CON(ch) < prereq->values[1])
        return FALSE;
      break;
    case AB_WIS:
      if (GET_REAL_WIS(ch) < prereq->values[1])
        return FALSE;
      break;
    case AB_INT:
      if (GET_REAL_INT(ch) < prereq->values[1])
        return FALSE;
      break;
    case AB_CHA:
      if (GET_REAL_CHA(ch) < prereq->values[1])
        return FALSE;
      break;
    default:
      log("SYSERR: meets_class_prerequisite() - Bad Attribute prerequisite %d", prereq->values[0]);
      return FALSE;
    }
    break;

    /* min. class level */
  case CLASS_PREREQ_CLASS_LEVEL:
    if (CLASS_LEVEL(ch, prereq->values[0]) < prereq->values[1])
      return FALSE;
    break;

    /* feat requirement */
  case CLASS_PREREQ_FEAT:
    if (has_feat_requirement_check(ch, prereq->values[0]) < prereq->values[1])
      return FALSE;
    break;

    /* required ability */
  case CLASS_PREREQ_ABILITY:
    if (GET_ABILITY(ch, prereq->values[0]) < prereq->values[1])
      return FALSE;
    break;

    /* spellcasting type and preparation type */
  case CLASS_PREREQ_SPELLCASTING:
    switch (prereq->values[0])
    {
    case CASTING_TYPE_NONE:
      if (IS_SPELLCASTER(ch))
        return FALSE;
      break;
    case CASTING_TYPE_ARCANE:
      if (!(IS_WIZARD(ch) ||
            IS_SORCERER(ch) ||
            IS_BARD(ch)))
        return FALSE;
      /* This stuff is all messed up - fix. */
      if (prereq->values[2] > 0)
      {
        if (compute_slots_by_circle(ch, CLASS_WIZARD, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_SORCERER, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_BARD, prereq->values[2]) == 0)
          return FALSE;
      }
      break;
    case CASTING_TYPE_DIVINE:
      if (!(IS_CLERIC(ch) ||
            IS_DRUID(ch) ||
            IS_INQUISITOR(ch) ||
            IS_PALADIN(ch) ||
            (CLASS_LEVEL(ch, CLASS_BLACKGUARD) > 0) ||
            IS_RANGER(ch)))
        return FALSE;
      if (prereq->values[2] > 0)
      {
        if (compute_slots_by_circle(ch, CLASS_CLERIC, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_PALADIN, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_INQUISITOR, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_BLACKGUARD, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_DRUID, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_RANGER, prereq->values[2]) == 0)
          return FALSE;
      }
      break;
    case CASTING_TYPE_ANY:
      if (!IS_SPELLCASTER(ch))
        return FALSE;
      if (prereq->values[2] > 0)
      {
        if (!(compute_slots_by_circle(ch, CLASS_WIZARD, prereq->values[2]) == 0 ||
              compute_slots_by_circle(ch, CLASS_SORCERER, prereq->values[2]) == 0 ||
              compute_slots_by_circle(ch, CLASS_BARD, prereq->values[2]) == 0 ||
              compute_slots_by_circle(ch, CLASS_INQUISITOR, prereq->values[2]) == 0 ||
              compute_slots_by_circle(ch, CLASS_CLERIC, prereq->values[2]) == 0 ||
              compute_slots_by_circle(ch, CLASS_PALADIN, prereq->values[2]) == 0 ||
              compute_slots_by_circle(ch, CLASS_DRUID, prereq->values[2]) == 0 ||
              compute_slots_by_circle(ch, CLASS_RANGER, prereq->values[2]) == 0))
          return FALSE;
      }
      break;
    default:
      log("SYSERR: meets_class_prerequisite() - Bad Casting Type prerequisite %d", prereq->values[0]);
      return FALSE;
    }

    switch (prereq->values[1])
    {
    case PREP_TYPE_NONE:
      return FALSE;
    case PREP_TYPE_PREPARED:
      if (!IS_MEM_BASED_CASTER(ch))
        return FALSE;
      break;
    case PREP_TYPE_SPONTANEOUS:
      if (IS_MEM_BASED_CASTER(ch))
        return FALSE;
      break;
    case PREP_TYPE_ANY:
      break;
    default:
      log("SYSERR: meets_class_prerequisite() - Bad Preparation type prerequisite %d", prereq->values[1]);
      return FALSE;
    }
    break;

    /* valid races - special handling since list of valids */
  case CLASS_PREREQ_RACE:
    if (!IS_NPC(ch) && GET_RACE(ch) != prereq->values[0])
      return FALSE;
    break;

    /* minimum BAB requirement */
  case CLASS_PREREQ_BAB:
    if (BAB(ch) < prereq->values[0])
      return FALSE;
    break;

    /* combat feat */
  case CLASS_PREREQ_CFEAT:
    /*  SPECIAL CASE - You must have a feat, and it must be the cfeat for the chosen weapon. */
    if (iarg && !has_combat_feat(ch, feat_to_cfeat(prereq->values[0]), iarg))
      return FALSE;
    /* when not using iarg, we use the 'special' value - CFEAT_SPECIAL_ and
         use has_special_cfeat() for processing */
    if (!iarg && !has_special_cfeat(ch, prereq->values[0], prereq->values[1]))
      return FALSE;
    break;

    /* weapon proficiency requirement */
  case CLASS_PREREQ_WEAPON_PROFICIENCY:
    if (iarg && !is_proficient_with_weapon(ch, iarg))
      return FALSE;
    break;

  default:
    log("SYSERR: meets_class_prerequisite() - Bad prerequisite type %d", prereq->prerequisite_type);
    return FALSE;
  }

  return TRUE;
}

/* a display specific to identify prereqs for a given class */
bool display_class_prereqs(struct char_data *ch, const char *classname)
{
  int class = CLASS_UNDEFINED;
  struct class_prerequisite *prereq = NULL;
  static int line_length = 80;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  bool meets_prereqs = FALSE, found = FALSE;

  skip_spaces_c(&classname);
  class = parse_class_long(classname);

  if (class == CLASS_UNDEFINED)
  {
    return FALSE;
  }

  /* do some math to check if we have max levels in a given class */
  int max_class_level = CLSLIST_MAXLVL(class);
  if (max_class_level == -1) /* no limit */
    max_class_level = LVL_IMMORT - 1;

  /* display top */
  send_to_char(ch, "\tC\r\n");
  draw_line(ch, line_length, '-', '-');

  /* basic info */
  send_to_char(ch, "\tcClass Name        : \tn%s\r\n", CLSLIST_NAME(class));
  send_to_char(ch, "\tcMax Level in Class: \tn%d - %s\r\n", max_class_level,
               (CLASS_LEVEL(ch, class) >= max_class_level) ? "\trCap reached!\tn" : "\tWLevel cap not reached!\tn");
  send_to_char(ch, "\tcUnlock Cost       : \tn%d Account XP - %s\r\n", CLSLIST_COST(class),
               has_unlocked_class(ch, class) ? "\tWUnlocked!\tn" : "\trLocked!\tn");
  send_to_char(ch, "\tcClass in the Game?: \tn%s\r\n", CLSLIST_INGAME(class) ? "\tWYes\tn" : "\trNo\tn");

  /* prereqs, start with text line */
  send_to_char(ch, "\tC");
  send_to_char(ch, "%s", text_line_string("\tcRequirements\tC", line_length, '-', '-'));
  send_to_char(ch, "\tn");
  send_to_char(ch, "\tcNote: you only need to meet one prereq for race and align:\tn\r\n\r\n");

  /* here we process our prereq linked list for each class */
  for (prereq = class_list[class].prereq_list; prereq != NULL; prereq = prereq->next)
  {
    meets_prereqs = FALSE;
    if (meets_class_prerequisite(ch, prereq, NO_IARG))
      meets_prereqs = TRUE;
    snprintf(buf, sizeof(buf), "\tn%s%s%s - %s\r\n",
             (meets_prereqs ? "\tn" : "\tr"), prereq->description, "\tn",
             (meets_prereqs ? "\tWFulfilled!\tn" : "\trMissing\tn"));
    send_to_char(ch, "%s", buf);
    found = TRUE;
  }

  if (!found)
    send_to_char(ch, "\tWNo requirements!\tn\r\n");

  /* close prereq display */
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn");

  if (class_is_available(ch, class, 0, NULL))
  {
    send_to_char(ch, "\tWClass IS AVAILABLE!\tn\r\n");
  }
  else
  {
    send_to_char(ch, "\trClass is not available!\tn\r\n");
  }

  /* close display */
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn");
  // send_to_char(ch, "\tcNote: Epic races currently can not multi-class\tn\r\n\r\n");

  return TRUE;
}

/* this will be a general list of all classes and indication whether
 selectable by CH based on prereqs */
void display_all_classes(struct char_data *ch)
{
  struct descriptor_data *d = ch->desc;
  int counter, columns = 0;

  write_to_output(d, "\r\n");

  for (counter = 0; counter < NUM_CLASSES; counter++)
  {
    write_to_output(d, "%s%-20.20s %s",
                    class_is_available(ch, counter, MODE_CLASSLIST_NORMAL, NULL) ? " " : "*",
                    CLSLIST_NAME(counter),
                    !(++columns % 3) ? "\r\n" : "");
  }

  write_to_output(d, "\r\n");
  write_to_output(d, "* - not qualified 'class prereqs <class name>' for details\r\n");
  write_to_output(d, "\r\n");
}

/* determines if ch qualifies for a class */
bool class_is_available(struct char_data *ch, int classnum, int iarg, char *sarg)
{
  struct class_prerequisite *prereq = NULL;
  int i = 0, max_class_level = CLSLIST_MAXLVL(classnum);
  bool has_alignment_restrictions = FALSE, has_valid_alignment = FALSE;
  bool has_race_restrictions = FALSE, has_valid_race = FALSE;

  /* dumb-dumb check */
  if (classnum < 0 || classnum >= NUM_CLASSES)
    return FALSE;

  /* is this class even in the game? */
  if (!CLSLIST_INGAME(classnum))
    return FALSE;

  /* cap for class ranks */
  if (max_class_level == -1) /* no limit */
    max_class_level = LVL_IMMORT - 1;
  if (CLASS_LEVEL(ch, classnum) >= max_class_level)
  {
    return FALSE;
  }

  /* prevent epic race from currently multi-classing */
  /* disabled! */
  if (iarg == MODE_CLASSLIST_NORMAL)
  {
    for (i = 0; i < NUM_CLASSES; i++)
      if (CLASS_LEVEL(ch, i)) /* found char current class */
        break;
    switch (GET_REAL_RACE(ch))
    {
      /*
    case RACE_CRYSTAL_DWARF:
      if (classnum == i) // char class selection and current class match?
        ;
      else
        return FALSE;
    case RACE_TRELUX:
      if (classnum == i) // char class selection and current class match?
        ;
      else
        return FALSE;
        */
    default:
      break;
    }
  }

  /* locked class that has been unlocked yet? */
  if (!has_unlocked_class(ch, classnum))
    return FALSE;

  /* class prerequisites list */
  if (class_list[classnum].prereq_list != NULL)
  {

    /*  This class has prerequisites. Traverse the list and check. */
    for (prereq = class_list[classnum].prereq_list; prereq != NULL; prereq = prereq->next)
    {

      /* we have to check for valid lists, like a list of valid alignments or races */
      switch (prereq->prerequisite_type)
      {

        /* has align restriction?  well any qualification will work */
      case CLASS_PREREQ_ALIGN:
        has_alignment_restrictions = TRUE;
        if (meets_class_prerequisite(ch, prereq, iarg) == TRUE)
          has_valid_alignment = TRUE;
        break;

        /* has race restriction?  well any qualification will work */
      case CLASS_PREREQ_RACE:
        has_race_restrictions = TRUE;
        if (meets_class_prerequisite(ch, prereq, iarg) == TRUE)
          has_valid_race = TRUE;
        break;

        /* our default normal case, instant disqualification */
      default:
        if (meets_class_prerequisite(ch, prereq, iarg) == FALSE)
          return FALSE; /* these are instant disqualifications */
        break;
      }
    } /* finished transversing list */

    /* final check for 'valid lists' such as alignment / race list */
    if (has_alignment_restrictions && !has_valid_alignment)
      return FALSE; /* doesn't mean alignment reqs */
    if (has_race_restrictions && !has_valid_race)
      return FALSE; /* doesn't mean race reqs */
  }

  /* made it! */
  return TRUE;
}

/* display a specific weapon details */
bool display_weapon_info(struct char_data *ch, const char *weapon)
{
  int i = 0;

  for (i = 0; i < NUM_WEAPON_TYPES; i++)
  {
    if (is_abbrev(weapon, weapon_list[i].name))
      break;
  }
  if (i < 0 || i >= NUM_WEAPON_TYPES)
  {
    return FALSE;
  }
  do_weaponinfo(ch, weapon, 0, 0);
  return TRUE;
}

/* display specific region details */
bool display_region_info(struct char_data *ch, int region)
{
  if (!ch || region <= REGION_NONE || region > NUM_REGIONS)
  {
    return FALSE;
  }

  char buf[MAX_STRING_LENGTH];

  /* This we will need to buffer and wrap so that it will fit in the space provided. */
  send_to_char(ch, "\tc%s\r\n", regions[region]);
  send_to_char(ch, "\tcLanguage: \tn%s\r\n", languages[get_region_language(region)]);
  send_to_char(ch, "\tcDescription: \tn\r\n");
  snprintf(buf, sizeof(buf), "%s", get_region_info(region));
  send_to_char(ch, "%s", strfrmt(buf, 80, 1, FALSE, FALSE, FALSE));

  return TRUE;
}

/* display a specific armor type details */
bool display_armor_info(struct char_data *ch, const char *armor)
{
  int i = 0;

  for (i = 1; i < NUM_SPEC_ARMOR_SUIT_TYPES; i++)
  {
    if (is_abbrev(armor, armor_list[i].name))
      break;
  }
  if (i < 0 || i >= NUM_SPEC_ARMOR_SUIT_TYPES)
  {
    return FALSE;
  }
  do_armorinfo(ch, armor, 0, 0);
  return TRUE;
}

/* display a specific classes details */
bool display_class_info(struct char_data *ch, const char *classname)
{
  int class = -1, i = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  static int line_length = 80;
  bool first_skill = TRUE;
  size_t len = 0;

  skip_spaces_c(&classname);
  class = parse_class_long(classname);

  if (class == -1 || class >= NUM_CLASSES)
  {
    /* Not found - Maybe put in a soundex list here? */
    return FALSE;
  }

  /* We found the class, and the class number is stored in 'class'. */
  /* Display the class info, formatted. */
  send_to_char(ch, "\tC\r\n");
  draw_line(ch, line_length, '-', '-');

  send_to_char(ch, "\tcClass Name       : \tn%s\r\n", CLSLIST_NAME(class));
  send_to_char(ch, "\tcPrestige Class?  : \tn%s\r\n", CLSLIST_PRESTIGE(class) ? "Yes" : "No");
  send_to_char(ch, "\tcMaximum Levels   : \tn%d\r\n", CLSLIST_MAXLVL(class));
  send_to_char(ch, "\tcUnlock Cost      : \tn%d Account XP\r\n", CLSLIST_COST(class));
  send_to_char(ch, "\tcBAB Progression  : \tn%s\r\n",
               (CLSLIST_BAB(class) == H) ? "High" : ((CLSLIST_BAB(class) == M) ? "Medium" : "Low"));
  send_to_char(ch, "\tcHitpoint Gain    : \tn%d-%d plus constitution bonus\r\n",
               CLSLIST_HPS(class) / 2, CLSLIST_HPS(class));
  send_to_char(ch, "\tcMovement Gain    : \tn0-%d\r\n", CLSLIST_MVS(class));
  send_to_char(ch, "\tcTraining Sessions: \tn%d plus Intelligence Mod (4x this value at 1st "
                   "level)\r\n",
               CLSLIST_TRAINS(class));
  send_to_char(ch, "\tcEpic Feat Prog   : \tnGain an epic feat every %d levels\r\n",
               CLSLIST_EFEATP(class));
  send_to_char(ch, "\tcClass in Game?   : \tn%s\r\n", CLSLIST_INGAME(class) ? "\tnYes\tn" : "\trNo, ask staff\tn");
  send_to_char(ch, "\tcPrestige Spell   : \tn%s\r\n", class_list[class].prestige_spell_progression);
  send_to_char(ch, "\tcPrimary Attribute: \tn%s\r\n", CLSLIST_ATTRIBUTE(class));

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  send_to_char(ch, "\tcWillpower Save Progression: \tn%s\r\n",
               CLSLIST_SAVES(class, SAVING_WILL) ? "Good" : "Poor");
  send_to_char(ch, "\tcFortitude Save Progression: \tn%s\r\n",
               CLSLIST_SAVES(class, SAVING_FORT) ? "Good" : "Poor");
  send_to_char(ch, "\tcReflex Save Progression   : \tn%s\r\n",
               CLSLIST_SAVES(class, SAVING_REFL) ? "Good" : "Poor");

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* This we will need to buffer and wrap so that it will fit in the space provided. */
  /* first build the list of skills */
  for (i = 0; i < NUM_ABILITIES; i++)
  {
    if (CLSLIST_ABIL(class, i) == 2)
    {
      if (first_skill)
      {
        len += snprintf(buf + len, sizeof(buf) - len, "\tcClass Skills:\tn  %s",
                        ability_names[i]);
        first_skill = FALSE;
      }
      else
      {
        len += snprintf(buf + len, sizeof(buf) - len, ", %s", ability_names[i]);
      }
    }
  }
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

  // Draw spellcasting commands if applicable
  if (strlen(spell_prep_dict[class][0]) > 0)
  {
    send_to_char(ch, "\tC");
    draw_line(ch, line_length, '-', '-');
    send_to_char(ch, "\tcSpell Prep Command  : \tn%s\r\n", spell_prep_dict[class][0]);
    send_to_char(ch, "\tcSpell Forget Command: \tn%s\r\n", spell_consign_dict[class][0]);
    send_to_char(ch, "\tcSpell Cast Command  : \tn%s\r\n", (class != CLASS_ALCHEMIST) ? ((class != CLASS_PSIONICIST) ? "cast" : "manifest") : "imbibe");
    char spellList[30];
    snprintf(spellList, sizeof(spellList), "spells %s", CLSLIST_NAME(class));
    for (i = 0; i < strlen(spellList); i++)
      spellList[i] = tolower(spellList[i]);
    send_to_char(ch, "\tcSpell List Command  : \tn%s\r\n", (class != CLASS_ALCHEMIST) ? ((class != CLASS_PSIONICIST) ? spellList : "powers psionicist") : "extracts");
  }
  else if (class == CLASS_SHADOW_DANCER)
  {
    send_to_char(ch, "\tC");
    draw_line(ch, line_length, '-', '-');
    send_to_char(ch, "\tcSpell Cast Command  : \tnshadowcast (spell name)\r\n");
    send_to_char(ch, "\tcSpell List Command  : \tnshadowcast\r\n");
  }

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /*  Here display the prerequisites */
  if (class_list[class].prereq_list == NULL)
  {
    snprintf(buf, sizeof(buf), "\tCPrerequisites : \tnnone\r\n");
  }
  else
  {
    bool first = TRUE;
    struct class_prerequisite *prereq;

    for (prereq = class_list[class].prereq_list; prereq != NULL; prereq = prereq->next)
    {
      if (first)
      {
        first = FALSE;
        snprintf(buf, sizeof(buf), "\tcPrerequisites : %s%s%s",
                 (meets_class_prerequisite(ch, prereq, NO_IARG) ? "\tn" : "\tr"), prereq->description, "\tn");
      }
      else
      {
        snprintf(buf2, sizeof(buf2), ", %s%s%s",
                 (meets_class_prerequisite(ch, prereq, NO_IARG) ? "\tn" : "\tr"), prereq->description, "\tn");
        strlcat(buf, buf2, sizeof(buf));
      }
    }
  }
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* This we will need to buffer and wrap so that it will fit in the space provided. */
  snprintf(buf, sizeof(buf), "\tcDescription : \tn%s\r\n", class_list[class].descrip);
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  send_to_char(ch, "\tYType: \tRclass feats %s\tY for this class's feat info.\tn\r\n",
               CLSLIST_NAME(class));

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  send_to_char(ch, "\tn\r\n");

  return TRUE;
}

/* this was created for debugging the class command and new classes added to the
 class list */
void display_imm_classlist(struct char_data *ch)
{
  int i = 0, j = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  size_t len = 0;

  send_to_char(ch, "# Name Abrv ClrAbrv | Menu | MaxLvl Lock Prestige BAB HPs Mvs Train InGame UnlockCost EFeatProg");
  send_to_char(ch, " Attribute DESCRIP\r\n");
  send_to_char(ch, " Sv-Fort Sv-Refl Sv-Will\r\n");
  send_to_char(ch, "    acrobatics,stealth,perception,heal,intimidate,concentration,spellcraft\r\n");
  send_to_char(ch, "    appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff\r\n");
  send_to_char(ch, "    diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive\r\n");
  send_to_char(ch, "    survival,swim,use_magic_device,perform\r\n");
  send_to_char(ch, "Class Titles\r\n");
  send_to_char(ch, "============================================");

  for (i = 0; i < NUM_CLASSES; i++)
  {
    len += snprintf(buf + len, sizeof(buf) - len,
                    "\r\n%d] %s %s %s | %s | %d %s %s %s %d %d %d %s %d %d %s\r\n     %s\r\n"
                    "  %s %s %s\r\n"
                    "     %s %s %s %s %s %s %s\r\n"
                    "     %s %s %s %s %s %s %s %s\r\n"
                    "     %s %s %s %s %s %s\r\n"
                    "     %s %s %s %s\r\n",
                    i, CLSLIST_NAME(i), CLSLIST_ABBRV(i), CLSLIST_CLRABBRV(i), CLSLIST_MENU(i),
                    CLSLIST_MAXLVL(i), CLSLIST_LOCK(i) ? "Y" : "N", CLSLIST_PRESTIGE(i) ? "Y" : "N",
                    (CLSLIST_BAB(i) == 2) ? "H" : (CLSLIST_BAB(i) ? "M" : "L"), CLSLIST_HPS(i),
                    CLSLIST_MVS(i), CLSLIST_TRAINS(i), CLSLIST_INGAME(i) ? "Y" : "N", CLSLIST_COST(i), CLSLIST_EFEATP(i),
                    CLSLIST_ATTRIBUTE(i), CLSLIST_DESCRIP(i),
                    CLSLIST_SAVES(i, 0) ? "G" : "B", CLSLIST_SAVES(i, 1) ? "G" : "B", CLSLIST_SAVES(i, 2) ? "G" : "B",
                    (CLSLIST_ABIL(i, ABILITY_ACROBATICS) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_ACROBATICS) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_STEALTH) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_STEALTH) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_PERCEPTION) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_PERCEPTION) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_HEAL) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_HEAL) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_INTIMIDATE) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_INTIMIDATE) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_CONCENTRATION) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_CONCENTRATION) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_SPELLCRAFT) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_SPELLCRAFT) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_APPRAISE) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_APPRAISE) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_DISCIPLINE) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_DISCIPLINE) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_TOTAL_DEFENSE) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_TOTAL_DEFENSE) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_LORE) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_LORE) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_RIDE) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_RIDE) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_CLIMB) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_CLIMB) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_SLEIGHT_OF_HAND) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_SLEIGHT_OF_HAND) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_BLUFF) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_BLUFF) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_DIPLOMACY) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_DIPLOMACY) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_DISABLE_DEVICE) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_DISABLE_DEVICE) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_DISGUISE) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_DISGUISE) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_ESCAPE_ARTIST) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_ESCAPE_ARTIST) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_HANDLE_ANIMAL) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_HANDLE_ANIMAL) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_SENSE_MOTIVE) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_SENSE_MOTIVE) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_SURVIVAL) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_SURVIVAL) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_SWIM) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_SWIM) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_USE_MAGIC_DEVICE) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_USE_MAGIC_DEVICE) ? "CC" : "NA"),
                    (CLSLIST_ABIL(i, ABILITY_PERFORM) == 2) ? "CA" : (CLSLIST_ABIL(i, ABILITY_PERFORM) ? "CC" : "NA"));
    for (j = 0; j < MAX_NUM_TITLES; j++)
    {
      len += snprintf(buf + len, sizeof(buf) - len, "%s\r\n", CLSLIST_TITLE(i, j));
    }
    len += snprintf(buf + len, sizeof(buf) - len, "============================================\r\n");
  }
  page_string(ch->desc, buf, 1);
}

bool view_class_feats(struct char_data *ch, const char *classname)
{
  int class = CLASS_UNDEFINED;
  struct class_feat_assign *feat_assign = NULL;

  skip_spaces_c(&classname);
  class = parse_class_long(classname);

  if (class == CLASS_UNDEFINED)
  {
    return FALSE;
  }

  if (class == CLASS_WARRIOR)
  {
    send_to_char(ch, "The warrior class gets a bonus class feat every two "
                     "levels.\r\n");
  }
  if (class == CLASS_WIZARD)
  {
    send_to_char(ch, "The wizard class gets a bonus class feat every five "
                     "levels.\r\n");
  }

  /* level feats */
  if (class_list[class].featassign_list != NULL)
  {
    /*  This class has feat assignment! Traverse the list and list. */
    for (feat_assign = class_list[class].featassign_list; feat_assign != NULL;
         feat_assign = feat_assign->next)
    {
      if (feat_assign->level_received > 0) /* -1 is just class feat assign */
        send_to_char(ch, "Level: %-2d, Stacks: %-3s, Feat: %s\r\n",
                     feat_assign->level_received,
                     feat_assign->stacks ? "Yes" : "No",
                     feat_list[feat_assign->feat_num].name);
    }
  }
  send_to_char(ch, "\r\n");

  return TRUE;
}

/* entry point for class command - getting class info */
ACMD(do_class)
{
  char arg[80];
  char arg2[80];
  const char *classname;

  /*  Have to process arguments like this
   *  because of the syntax - class info <classname> */
  classname = one_argument(argument, arg, sizeof(arg));
  one_argument(classname, arg2, sizeof(arg2));

  /* no argument, or general list of classes */
  if (is_abbrev(arg, "list") || !*arg)
  {
    display_all_classes(ch);

    /* class info - specific info on given class */
  }
  else if (is_abbrev(arg, "info"))
  {

    if (!strcmp(classname, ""))
    {
      send_to_char(ch, "\r\nYou must provide the name of a class.\r\n");
    }
    else if (!display_class_info(ch, classname))
    {
      send_to_char(ch, "Could not find that class.\r\n");
    }

    /* class feat - list of free feats for given class */
  }
  else if (is_abbrev(arg, "feats"))
  {

    if (!strcmp(classname, ""))
    {
      send_to_char(ch, "\r\nYou must provide the name of a class.\r\n");
    }
    else if (!view_class_feats(ch, classname))
    {
      send_to_char(ch, "Could not find that class.\r\n");
    }

    /* cryptic class listing for staff :) */
  }
  else if (is_abbrev(arg, "staff"))
  {
    display_imm_classlist(ch);

    /* class listing just to view pre-requisites for a given class */
  }
  else if (is_abbrev(arg, "prereqs"))
  {

    if (!strcmp(classname, ""))
    {
      send_to_char(ch, "\r\nYou must provide the name of a class.\r\n");
    }
    else if (!display_class_prereqs(ch, classname))
    {
      send_to_char(ch, "Could not find that class.\r\n");
    }
  }

  send_to_char(ch, "\tDUsage: class <list|info|feats|staff|prereqs> <class name>\tn\r\n");
}

/* TODO: phase this out using classo prereqs */
/* does the ch have a valid alignment for proposed class?  currently only used
 in interpreter.c for starting chars */
/* returns 1 for valid alignment, returns 0 for problem with alignment */
int valid_align_by_class(int alignment, int class)
{
  switch (class)
  {
    /* any lawful alignment */
  case CLASS_MONK:
    switch (alignment)
    {
    case LAWFUL_GOOD:
    case LAWFUL_NEUTRAL:
    case LAWFUL_EVIL:
      return TRUE;
    default:
      return FALSE;
    }
    /* any 'neutral' alignment */
  case CLASS_DRUID:
    switch (alignment)
    {
    case NEUTRAL_GOOD:
    case LAWFUL_NEUTRAL:
    case TRUE_NEUTRAL:
    case CHAOTIC_NEUTRAL:
    case NEUTRAL_EVIL:
      return TRUE;
    default:
      return FALSE;
    }

    /* evil only */
    //    case CLASS_ASSASSIN:
    //      switch (alignment) {
    //        case LAWFUL_EVIL:
    //        case NEUTRAL_EVIL:
    //        case CHAOTIC_EVIL:
    //          return TRUE;
    //        default:
    //          return FALSE;
    //      }
    //      break;

    /* any 'non-lawful' alignment */
  case CLASS_BERSERKER:
  case CLASS_BARD:
    switch (alignment)
    {
      /* we are checking for invalids */
    case LAWFUL_GOOD:
    case LAWFUL_NEUTRAL:
    case LAWFUL_EVIL:
      return FALSE;
    default:
      return TRUE;
    }
    /* only lawful good */
  case CLASS_PALADIN:
    if (alignment == LAWFUL_GOOD)
      return TRUE;
    else
      return FALSE;
  case CLASS_BLACKGUARD:
    if (alignment == LAWFUL_EVIL)
      return TRUE;
    else if (alignment == NEUTRAL_EVIL)
      return TRUE;
    else if (alignment == CHAOTIC_EVIL)
      return TRUE;
    else
      return FALSE;
    /* default, no alignment restrictions */
  case CLASS_WIZARD:
  case CLASS_CLERIC:
  case CLASS_RANGER:
  case CLASS_ROGUE:
  case CLASS_WARRIOR:
  case CLASS_WEAPON_MASTER:
  case CLASS_ARCANE_ARCHER:
  case CLASS_ARCANE_SHADOW:
  case CLASS_STALWART_DEFENDER:
  case CLASS_DUELIST:
  case CLASS_SHIFTER:
  case CLASS_SHADOW_DANCER:
  case CLASS_SORCERER:
  case CLASS_MYSTIC_THEURGE:
  case CLASS_SACRED_FIST:
  case CLASS_ALCHEMIST:
  case CLASS_ELDRITCH_KNIGHT:
  case CLASS_SPELLSWORD:
  case CLASS_PSIONICIST:
  case CLASS_ASSASSIN:
  case CLASS_INQUISITOR:
    return TRUE;
  }
  /* shouldn't get here if we got all classes listed above */
  return TRUE;
}

/* homeland-port currently unused */
const char *church_types[] = {
    "Ao",
    "Akadi",
    "Chauntea",
    "Cyric",
    "Grumbar",
    "Istishia", // 5
    "Kelemvor",
    "Kossuth",
    "Lathander",
    "Mystra",
    "Oghma", // 10
    "Shar",
    "Silvanus",
    "\n"}; // 14

/* The code to interpret a class letter -- just used in who list */
int parse_class(char arg)
{
  arg = LOWER(arg);

  switch (arg)
  {
  case 'a':
    return CLASS_BARD;
  case 'b':
    return CLASS_BERSERKER;
  case 'c':
    return CLASS_CLERIC;
  case 'd':
    return CLASS_DRUID;
  case 'e':
    return CLASS_WEAPON_MASTER;
  case 'f':
    return CLASS_ARCANE_ARCHER;
  case 'g':
    return CLASS_STALWART_DEFENDER;
  case 'h':
    return CLASS_SHIFTER;
  case 'i':
    return CLASS_DUELIST;
  case 'j':
    return CLASS_SHADOW_DANCER;
    //    case 'k': return CLASS_ASSASSIN;
  case 'l':
    return CLASS_MYSTICTHEURGE;
  case 'm':
    return CLASS_WIZARD;
  case 'n':
    return CLASS_ARCANE_SHADOW;
  case 'o':
    return CLASS_MONK;
  case 'p':
    return CLASS_PALADIN;
  case 'q':
    return CLASS_BLACKGUARD;
  case 'r':
    return CLASS_RANGER;
  case 's':
    return CLASS_SORCERER;
  case 't':
    return CLASS_ROGUE;
  case 'u':
    return CLASS_ALCHEMIST;
  case 'v':
    return CLASS_SACRED_FIST;
  case 'w':
    return CLASS_WARRIOR;
  case 'x':
    return CLASS_ELDRITCH_KNIGHT;
  case 'y':
    return CLASS_SPELLSWORD;
  case 'z':
    return CLASS_PSIONICIST;
  case '1':
    return CLASS_INQUISITOR;
    /* empty letters */
    /* empty letters */
    /* empty letters */

  default:
    return CLASS_UNDEFINED;
  }
}

/* accept short descrip, return class */
int parse_class_long(const char *arg_in)
{
  size_t arg_sz = strlen(arg_in) + 1;
  char arg_buf[arg_sz];
  strlcpy(arg_buf, arg_in, arg_sz);
  char *arg = arg_buf;

  int l = 0; /* string length */

  for (l = 0; *(arg + l); l++) /* convert to lower case */
    *(arg + l) = LOWER(*(arg + l));

  if (is_abbrev(arg, "wizard"))
    return CLASS_WIZARD;
  if (is_abbrev(arg, "cleric"))
    return CLASS_CLERIC;
  if (is_abbrev(arg, "warrior"))
    return CLASS_WARRIOR;
  if (is_abbrev(arg, "fighter"))
    return CLASS_WARRIOR;
  if (is_abbrev(arg, "rogue"))
    return CLASS_ROGUE;
  if (is_abbrev(arg, "monk"))
    return CLASS_MONK;
  if (is_abbrev(arg, "druid"))
    return CLASS_DRUID;
  if (is_abbrev(arg, "berserker"))
    return CLASS_BERSERKER;
  if (is_abbrev(arg, "sorcerer"))
    return CLASS_SORCERER;
  if (is_abbrev(arg, "paladin"))
    return CLASS_PALADIN;
  if (is_abbrev(arg, "ranger"))
    return CLASS_RANGER;
  if (is_abbrev(arg, "bard"))
    return CLASS_BARD;
  if (is_abbrev(arg, "inquisitor"))
    return CLASS_INQUISITOR;
  if (is_abbrev(arg, "weaponmaster"))
    return CLASS_WEAPON_MASTER;
  if (is_abbrev(arg, "weapon-master"))
    return CLASS_WEAPON_MASTER;
  if (is_abbrev(arg, "shadow-dancer"))
    return CLASS_SHADOW_DANCER;
  if (is_abbrev(arg, "shadowdancer"))
    return CLASS_SHADOW_DANCER;
  if (is_abbrev(arg, "arcanearcher"))
    return CLASS_ARCANE_ARCHER;
  if (is_abbrev(arg, "arcane-archer"))
    return CLASS_ARCANE_ARCHER;
  if (is_abbrev(arg, "arcane-shadow"))
    return CLASS_ARCANE_SHADOW;
  if (is_abbrev(arg, "arcaneshadow"))
    return CLASS_ARCANE_SHADOW;
  if (is_abbrev(arg, "eldritchknight"))
    return CLASS_ELDRITCH_KNIGHT;
  if (is_abbrev(arg, "eldritch-knight"))
    return CLASS_ELDRITCH_KNIGHT;
  if (is_abbrev(arg, "spellsword"))
    return CLASS_SPELLSWORD;
  if (is_abbrev(arg, "spell-sword"))
    return CLASS_SPELLSWORD;
  if (is_abbrev(arg, "stalwartdefender"))
    return CLASS_STALWART_DEFENDER;
  if (is_abbrev(arg, "stalwart-defender"))
    return CLASS_STALWART_DEFENDER;
  if (is_abbrev(arg, "shifter"))
    return CLASS_SHIFTER;
  if (is_abbrev(arg, "sacred-fist"))
    return CLASS_SACRED_FIST;
  if (is_abbrev(arg, "sacredfist"))
    return CLASS_SACRED_FIST;
  if (is_abbrev(arg, "duelist"))
    return CLASS_DUELIST;
  //  if (is_abbrev(arg, "assassin")) return CLASS_ASSASSIN;
  if (is_abbrev(arg, "mystictheurge"))
    return CLASS_MYSTICTHEURGE;
  if (is_abbrev(arg, "mystic-theurge"))
    return CLASS_MYSTICTHEURGE;
  if (is_abbrev(arg, "alchemist"))
    return CLASS_ALCHEMIST;
  if (is_abbrev(arg, "psionicist"))
    return CLASS_PSIONICIST;
  if (is_abbrev(arg, "blackguard"))
    return CLASS_BLACKGUARD;
  if (is_abbrev(arg, "assassin"))
    return CLASS_ASSASSIN;

  return CLASS_UNDEFINED;
}

/* bitvectors (i.e., powers of two) for each class, mainly for use in do_who
 * and do_users.  Add new classes at the end so that all classes use sequential
 * powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, etc.) up to
 * the limit of your bitvector_t, typically 0-31. */
bitvector_t find_class_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_class(arg[rpos]));

  return (ret);
}

/* guild guards: stops classes from going in certain directions,
 currently being phased out */
struct guild_info_type guild_info[] = {
    /* Midgaard */
    {-999 /* all */, 3004, NORTH},
    {-999 /* all */, 3017, SOUTH},
    {-999 /* all */, 3021, EAST},
    {-999 /* all */, 3027, EAST},
    /* Brass Dragon */
    {-999 /* all */, 5065, WEST},
    /* this must go last -- add new guards above! */
    {-1, NOWHERE, -1}};

/* certain feats/etc can affect what is an actual class/cross-class ability */
int modify_class_ability(struct char_data *ch, int ability, int class)
{
  int ability_value = CLSLIST_ABIL(class, ability);

  if (IS_LICH(ch))
  {
    if (ability == ABILITY_ACROBATICS)
      ability_value = CA;
  }

  if (HAS_FEAT(ch, FEAT_DECEPTION))
  {
    if (ability == ABILITY_DISGUISE || ability == ABILITY_STEALTH)
      ability_value = CA;
  }

  if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_DRACONIC))
  {
    if (ability == ABILITY_PERCEPTION)
      ability_value = CA;
  }

  if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_ARCANE))
  {
    if (ability == ABILITY_APPRAISE)
      ability_value = CA;
  }

  if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_FEY))
  {
    if (ability == ABILITY_SURVIVAL)
      ability_value = CA;
  }

  if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_UNDEAD))
  {
    if (ability == ABILITY_INTIMIDATE)
      ability_value = CA;
  }

  return ability_value;
}

/* given ch, and saving throw we need computed - do so here */
byte saving_throws(struct char_data *ch, int type)
{
  if (IS_NPC(ch))
  {
    if (CLSLIST_SAVES(GET_CLASS(ch), type))
      return (GET_LEVEL(ch) / 2 + 1);
    else
      return (GET_LEVEL(ch) / 4 + 1);
  }

  int i, save = 0;
  float counter = 1.1;

  /* actual pc calculation, added float for more(?) accuracy */
  for (i = 0; i < MAX_CLASSES; i++)
  {
    if (CLASS_LEVEL(ch, i))
    { // found class and level
      if (CLSLIST_SAVES(i, type))
        counter += (float)CLASS_LEVEL(ch, i) / 2.0;
      else
        counter += (float)CLASS_LEVEL(ch, i) / 4.0;
    }
  }

  save = (int)counter;
  return save;
}

// base attack bonus, replacement for THAC0 system

int BAB(struct char_data *ch)
{

  /* gnarly huh? */
  if (IS_AFFECTED(ch, AFF_TFORM))
    return (GET_LEVEL(ch));

  /* npc is simple */
  if (IS_NPC(ch))
  {
    switch (CLSLIST_BAB(GET_CLASS(ch)))
    {
    case H:
      return ((int)(GET_LEVEL(ch)));
    case M:
      return ((int)(GET_LEVEL(ch) * 3 / 4));
    case L:
    default:
      return ((int)(GET_LEVEL(ch) / 2));
    }
  }

  int i, bab = 0, level, wildshape_level = 0;
  float counter = 0.0;

  /* wildshape */
  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
    wildshape_level = CLASS_LEVEL(ch, CLASS_DRUID) + CLASS_LEVEL(ch, CLASS_SHIFTER);

  /* pc: loop through all the possible classes the char could be */
  /* added float for more(?) accuracy */
  for (i = 0; i < MAX_CLASSES; i++)
  {
    level = MIN(20, CLASS_LEVEL(ch, i));
    if (level)
    {
      switch (CLSLIST_BAB(i))
      {
      case M:
        counter += (float)level * 3.0 / 4.0;
        break;
      case H:
        counter += (float)level;
        break;
      case L:
      default:
        counter += (float)level / 2.0;
        break;
      }
    }
  }

  bab += CLASS_LEVEL(ch, i) - 20;

  bab = (int)counter;

  if (char_has_mud_event(ch, eSPELLBATTLE) && SPELLBATTLE(ch) > 0)
  {
    bab += MAX(1, (SPELLBATTLE(ch) * 2 / 3));
  }

  if (wildshape_level > bab)
    bab = wildshape_level;

  if (!IS_NPC(ch)) /* cap pc bab at 30 */
    return (MIN(bab, LVL_IMMORT - 1));

  return bab;
}

/* Default titles system, simplified from stock -zusuk */
const char *titles(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Being";
  if (level == LVL_IMPL)
    return "the Implementor";
  /* Default title for classes which do not have titles defined */
  if (chclass < 0 || chclass >= NUM_CLASSES)
    return "the Classless";

  int title_num = 0;

  if (level <= 4)
    title_num = 0;
  else if (level <= 9)
    title_num = 1;
  else if (level <= 14)
    title_num = 2;
  else if (level <= 19)
    title_num = 3;
  else if (level <= 24)
    title_num = 4;
  else if (level <= 29)
    title_num = 5;
  else if (level <= 30)
    title_num = 6;
  else if (level <= LVL_IMMORT)
    title_num = 7;
  else if (level <= LVL_STAFF)
    title_num = 8;
  else if (level <= LVL_GRSTAFF)
    title_num = 9;
  else
    title_num = 10;

  return (CLSLIST_TITLE(chclass, title_num));
}

/* use to be old rolling system, now its just used to initialize base stats */
void roll_real_abils(struct char_data *ch)
{
  GET_REAL_INT(ch) = 8;
  GET_REAL_WIS(ch) = 8;
  GET_REAL_CHA(ch) = 8;
  GET_REAL_STR(ch) = 8;
  GET_REAL_DEX(ch) = 8;
  GET_REAL_CON(ch) = 8;
  ch->aff_abils = ch->real_abils;
}

// Mostly deprecated... still used for racefix
/* Information required for character leveling in regards to free feats
   1) required class
   2) required race
   3) stacks?
   4) level received
   5) feat name
   This function also assigns all our (starting) racial feats */
static int level_feats[][LEVEL_FEATS] = {

    /****************/
    /* Racial feats */
    /****************/

    /* class, race, stacks?, level, feat_ name */
    /* Human */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_HUMAN, FALSE, 1, FEAT_QUICK_TO_MASTER},
    {CLASS_UNDEFINED, RACE_HUMAN, FALSE, 1, FEAT_SKILLED},

    /* class, race, stacks?, level, feat_ name */
    /* Dwarf */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_POISON_RESIST},
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_STABILITY},
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_SPELL_HARDINESS},
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_DWARF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_SHIELD_DWARF, FALSE, 1, FEAT_SHIELD_DWARF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_SHIELD_DWARF, FALSE, 1, FEAT_SHIELD_DWARF_ARMOR_TRAINING},
    {CLASS_UNDEFINED, RACE_SHIELD_DWARF, FALSE, 1, FEAT_ARMOR_PROFICIENCY_LIGHT},
    {CLASS_UNDEFINED, RACE_SHIELD_DWARF, FALSE, 1, FEAT_ARMOR_PROFICIENCY_MEDIUM},
    {CLASS_UNDEFINED, RACE_SHIELD_DWARF, FALSE, 1, FEAT_DWARVEN_WEAPON_PROFICIENCY},
    {CLASS_UNDEFINED, RACE_SHIELD_DWARF, FALSE, 1, FEAT_ENCUMBERED_RESILIENCE},

    /* class, race, stacks?, level, feat_ name */
    /* Half-Troll */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_TROLL_REGENERATION},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_WEAKNESS_TO_FIRE},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_WEAKNESS_TO_ACID},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_STRONG_AGAINST_POISON},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_STRONG_AGAINST_DISEASE},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_HALF_TROLL_RACIAL_ADJUSTMENT},

    /* Half-Drow */
    {CLASS_UNDEFINED, RACE_HALF_DROW, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_HALF_DROW, FALSE, 1, FEAT_WEAPON_PROFICIENCY_DROW},
    {CLASS_UNDEFINED, RACE_HALF_DROW, FALSE, 1, FEAT_RESISTANCE_TO_ENCHANTMENTS},
    {CLASS_UNDEFINED, RACE_HALF_DROW, FALSE, 1, FEAT_HALF_BLOOD},
    {CLASS_UNDEFINED, RACE_HALF_DROW, FALSE, 1, FEAT_KEEN_SENSES},
    {CLASS_UNDEFINED, RACE_HALF_DROW, FALSE, 1, FEAT_HALF_DROW_SPELL_RESISTANCE},
    {CLASS_UNDEFINED, RACE_HALF_DROW, FALSE, 1, FEAT_HALF_DROW_RACIAL_ADJUSTMENT},

    /* Dragonborn */
    {CLASS_UNDEFINED, RACE_DRAGONBORN, FALSE, 1, FEAT_DRAGONBORN_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_DRAGONBORN, FALSE, 1, FEAT_DRAGONBORN_BREATH},
    {CLASS_UNDEFINED, RACE_DRAGONBORN, FALSE, 1, FEAT_DRAGONBORN_FURY},
    {CLASS_UNDEFINED, RACE_DRAGONBORN, FALSE, 1, FEAT_DRAGONBORN_RESISTANCE},

    // Tiefling
    {CLASS_UNDEFINED, RACE_TIEFLING, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_TIEFLING, FALSE, 1, FEAT_TIEFLING_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_TIEFLING, FALSE, 1, FEAT_BLOODHUNT},
    {CLASS_UNDEFINED, RACE_TIEFLING, FALSE, 1, FEAT_TIEFLING_HELLISH_RESISTANCE},
    {CLASS_UNDEFINED, RACE_TIEFLING, FALSE, 1, FEAT_TIEFLING_MAGIC},

    // Tabaxi
    {CLASS_UNDEFINED, RACE_TABAXI, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_TABAXI, FALSE, 1, FEAT_TABAXI_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_TABAXI, FALSE, 1, FEAT_MOBILITY},
    {CLASS_UNDEFINED, RACE_TABAXI, FALSE, 1, FEAT_TABAXI_FELINE_AGILITY},
    {CLASS_UNDEFINED, RACE_TABAXI, FALSE, 1, FEAT_TABAXI_CATS_CLAWS},
    {CLASS_UNDEFINED, RACE_TABAXI, FALSE, 1, FEAT_TABAXI_CATS_TALENT},

    // Fae
    {CLASS_UNDEFINED, RACE_FAE, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_FAE, FALSE, 1, FEAT_DODGE},
    {CLASS_UNDEFINED, RACE_FAE, FALSE, 1, FEAT_FAE_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_FAE, FALSE, 1, FEAT_FAE_RESISTANCE},
    {CLASS_UNDEFINED, RACE_FAE, FALSE, 1, FEAT_FAE_MAGIC},
    {CLASS_UNDEFINED, RACE_FAE, FALSE, 1, FEAT_FAE_FLIGHT},
    {CLASS_UNDEFINED, RACE_FAE, FALSE, 1, FEAT_FAE_SENSES},

    // Goliath
    {CLASS_UNDEFINED, RACE_GOLIATH, FALSE, 1, FEAT_NATURAL_ATHLETE},
    {CLASS_UNDEFINED, RACE_GOLIATH, FALSE, 1, FEAT_GOLIATH_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_GOLIATH, FALSE, 1, FEAT_MOUNTAIN_BORN},
    {CLASS_UNDEFINED, RACE_GOLIATH, FALSE, 1, FEAT_POWERFUL_BUILD},
    {CLASS_UNDEFINED, RACE_GOLIATH, FALSE, 1, FEAT_STONES_ENDURANCE},

    /* high elf */
    {CLASS_UNDEFINED, RACE_HIGH_ELF, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_HIGH_ELF, FALSE, 1, FEAT_WEAPON_PROFICIENCY_ELF},
    {CLASS_UNDEFINED, RACE_HIGH_ELF, FALSE, 1, FEAT_SLEEP_ENCHANTMENT_IMMUNITY},
    {CLASS_UNDEFINED, RACE_HIGH_ELF, FALSE, 1, FEAT_KEEN_SENSES},
    {CLASS_UNDEFINED, RACE_HIGH_ELF, FALSE, 1, FEAT_RESISTANCE_TO_ENCHANTMENTS},
    {CLASS_UNDEFINED, RACE_HIGH_ELF, FALSE, 1, FEAT_ELF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_HIGH_ELF, FALSE, 1, FEAT_HIGH_ELF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_HIGH_ELF, FALSE, 1, FEAT_HIGH_ELF_CANTRIP},
    {CLASS_UNDEFINED, RACE_HIGH_ELF, FALSE, 1, FEAT_HIGH_ELF_LINGUIST},

    /* Shade */
    {CLASS_UNDEFINED, RACE_SHADE, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_SHADE, FALSE, 1, FEAT_SHADE_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_SHADE, FALSE, 1, FEAT_SHADOWFELL_MIND},
    {CLASS_UNDEFINED, RACE_SHADE, FALSE, 1, FEAT_PRACTICED_SNEAK},
    {CLASS_UNDEFINED, RACE_SHADE, FALSE, 1, FEAT_ONE_WITH_SHADOW},

    // Aasimar
    {CLASS_UNDEFINED, RACE_AASIMAR, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_AASIMAR, FALSE, 1, FEAT_ASTRAL_MAJESTY},
    {CLASS_UNDEFINED, RACE_AASIMAR, FALSE, 1, FEAT_CELESTIAL_RESISTANCE},
    {CLASS_UNDEFINED, RACE_AASIMAR, FALSE, 1, FEAT_AASIMAR_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_AASIMAR, FALSE, 1, FEAT_AASIMAR_HEALING_HANDS},
    {CLASS_UNDEFINED, RACE_AASIMAR, FALSE, 1, FEAT_AASIMAR_LIGHT_BEARER},

    /* STOUT_HALFLING */
    {CLASS_UNDEFINED, RACE_STOUT_HALFLING, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_STOUT_HALFLING, FALSE, 1, FEAT_SHADOW_HOPPER},
    {CLASS_UNDEFINED, RACE_STOUT_HALFLING, FALSE, 1, FEAT_LUCKY},
    {CLASS_UNDEFINED, RACE_STOUT_HALFLING, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_STOUT_HALFLING, FALSE, 1, FEAT_HALFLING_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_STOUT_HALFLING, FALSE, 1, FEAT_STOUT_HALFLING_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_STOUT_HALFLING, FALSE, 1, FEAT_STOUT_RESILIENCE},

    /* Forest Gnome */
    {CLASS_UNDEFINED, RACE_FOREST_GNOME, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_FOREST_GNOME, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_FOREST_GNOME, FALSE, 1, FEAT_RESISTANCE_TO_ILLUSIONS},
    {CLASS_UNDEFINED, RACE_FOREST_GNOME, FALSE, 1, FEAT_ILLUSION_AFFINITY},
    {CLASS_UNDEFINED, RACE_FOREST_GNOME, FALSE, 1, FEAT_TINKER_FOCUS},
    {CLASS_UNDEFINED, RACE_FOREST_GNOME, FALSE, 1, FEAT_GNOME_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_FOREST_GNOME, FALSE, 1, FEAT_FOREST_GNOME_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_FOREST_GNOME, FALSE, 1, FEAT_SPEAK_WITH_BEASTS},
    {CLASS_UNDEFINED, RACE_FOREST_GNOME, FALSE, 1, FEAT_NATURAL_ILLUSIONIST},

    /* GOLD_DWARF */
    {CLASS_UNDEFINED, RACE_GOLD_DWARF, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_GOLD_DWARF, FALSE, 1, FEAT_POISON_RESIST},
    {CLASS_UNDEFINED, RACE_GOLD_DWARF, FALSE, 1, FEAT_STABILITY},
    {CLASS_UNDEFINED, RACE_GOLD_DWARF, FALSE, 1, FEAT_SPELL_HARDINESS},
    {CLASS_UNDEFINED, RACE_GOLD_DWARF, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_GOLD_DWARF, FALSE, 1, FEAT_DWARF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_GOLD_DWARF, FALSE, 1, FEAT_GOLD_DWARF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_GOLD_DWARF, FALSE, 1, FEAT_GOLD_DWARF_TOUGHNESS},
    {CLASS_UNDEFINED, RACE_GOLD_DWARF, FALSE, 1, FEAT_DWARVEN_WEAPON_PROFICIENCY},
    {CLASS_UNDEFINED, RACE_GOLD_DWARF, FALSE, 1, FEAT_ENCUMBERED_RESILIENCE},

    /* class, race, stacks?, level, feat_ name */
    /* Halfling */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_HALFLING, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_HALFLING, FALSE, 1, FEAT_SHADOW_HOPPER},
    {CLASS_UNDEFINED, RACE_HALFLING, FALSE, 1, FEAT_LUCKY},
    {CLASS_UNDEFINED, RACE_HALFLING, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_HALFLING, FALSE, 1, FEAT_HALFLING_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_LIGHTFOOT_HALFLING, FALSE, 1, FEAT_LIGHTFOOT_HALFLING_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_LIGHTFOOT_HALFLING, FALSE, 1, FEAT_NATURALLY_STEALTHY},

    /* class, race, stacks?, level, feat_ name */
    /* Half-Elf */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_WEAPON_PROFICIENCY_ELF},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_RESISTANCE_TO_ENCHANTMENTS},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_HALF_BLOOD},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_KEEN_SENSES},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_ADAPTABILITY},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_HALF_ELF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_SLEEP_ENCHANTMENT_IMMUNITY},

    /* class, race, stacks?, level, feat_ name */
    /* Half-Orc */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_HALF_ORC, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_HALF_ORC, FALSE, 1, FEAT_HALF_ORC_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_HALF_ORC, FALSE, 1, FEAT_MENACING},
    {CLASS_UNDEFINED, RACE_HALF_ORC, FALSE, 1, FEAT_RELENTLESS_ENDURANCE},
    {CLASS_UNDEFINED, RACE_HALF_ORC, FALSE, 1, FEAT_SAVAGE_ATTACKS},

    /* class, race, stacks?, level, feat_ name */
    /* Gnome */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_RESISTANCE_TO_ILLUSIONS},
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_ILLUSION_AFFINITY},
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_TINKER_FOCUS},
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_GNOME_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_ROCK_GNOME, FALSE, 1, FEAT_ROCK_GNOME_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_ROCK_GNOME, FALSE, 1, FEAT_ARTIFICERS_LORE},
    {CLASS_UNDEFINED, RACE_ROCK_GNOME, FALSE, 1, FEAT_TINKER},

    /* class, race, stacks?, level, feat_ name */
    /* Trelux */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_VITAL},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_HARDY},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_VULNERABLE_TO_COLD},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_TRELUX_EXOSKELETON},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_LEAP},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_WINGS},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_TRELUX_EQ},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_TRELUX_PINCERS},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_INSECTBEING},

    /* class, race, stacks?, level, feat_ name */
    /* Lich */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_LICH_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_VITAL},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_HARDY},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_UNARMED_STRIKE},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_IMPROVED_UNARMED_STRIKE},
    {CLASS_UNDEFINED, RACE_LICH, TRUE, 1, FEAT_ARMOR_SKIN},
    {CLASS_UNDEFINED, RACE_LICH, TRUE, 1, FEAT_ARMOR_SKIN},
    {CLASS_UNDEFINED, RACE_LICH, TRUE, 1, FEAT_ARMOR_SKIN},
    {CLASS_UNDEFINED, RACE_LICH, TRUE, 1, FEAT_ARMOR_SKIN},
    {CLASS_UNDEFINED, RACE_LICH, TRUE, 1, FEAT_ARMOR_SKIN},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_LICH_SPELL_RESIST},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_LICH_DAM_RESIST},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_LICH_TOUCH},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_LICH_REJUV},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_LICH_FEAR},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_ELECTRIC_IMMUNITY},
    {CLASS_UNDEFINED, RACE_LICH, FALSE, 1, FEAT_COLD_IMMUNITY},

    /* class, race, stacks?, level, feat_ name */
    /* elf */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_WEAPON_PROFICIENCY_ELF},
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_SLEEP_ENCHANTMENT_IMMUNITY},
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_KEEN_SENSES},
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_RESISTANCE_TO_ENCHANTMENTS},
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_ELF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_MOON_ELF, FALSE, 1, FEAT_MOON_ELF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_MOON_ELF, FALSE, 1, FEAT_MOON_ELF_LUNAR_MAGIC},
    {CLASS_UNDEFINED, RACE_MOON_ELF, FALSE, 1, FEAT_MOON_ELF_BATHED_IN_MOONLIGHT},

    /* class, race, stacks?, level, feat_ name */
    /* crystal dwarf */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_CRYSTAL_BODY},
    {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_CRYSTAL_FIST},
    {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_VITAL},
    {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_HARDY},
    {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_CRYSTAL_SKIN},
    {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_POISON_RESIST},
    {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_CRYSTAL_DWARF, FALSE, 1, FEAT_CRYSTAL_DWARF_RACIAL_ADJUSTMENT},

    /* class, race, stacks?, level, feat_ name */
    /* Arcana Golem */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_SPELLBATTLE},
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_SPELL_VULNERABILITY},
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_ENCHANTMENT_VULNERABILITY},
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_PHYSICAL_VULNERABILITY},
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_MAGICAL_HERITAGE},
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_ARCANA_GOLEM_RACIAL_ADJUSTMENT},

    /* class, race, stacks?, level, feat_ name */
    /* Drow */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_SLEEP_ENCHANTMENT_IMMUNITY},
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_KEEN_SENSES},
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_RESISTANCE_TO_ENCHANTMENTS},
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_WEAPON_PROFICIENCY_DROW},
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_DROW_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_DROW_SPELL_RESISTANCE},
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_SLA_FAERIE_FIRE},
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_SLA_LEVITATE},
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_SLA_DARKNESS},
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_DROW_INNATE_MAGIC},
    // disadvantage
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_LIGHT_BLINDNESS},

    /* class, race, stacks?, level, feat_ name */
    /* Duergar */
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_POISON_RESIST},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_PHANTASM_RESIST},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_PARALYSIS_RESIST},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_STABILITY},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_STRONG_SPELL_HARDINESS},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_DUERGAR_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_SLA_INVIS},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_SLA_STRENGTH},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_SLA_ENLARGE},
    {CLASS_UNDEFINED, RACE_DUERGAR, FALSE, 1, FEAT_DUERGAR_MAGIC},

    /* wood elf */
    {CLASS_UNDEFINED, RACE_WOOD_ELF, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_WOOD_ELF, FALSE, 1, FEAT_WEAPON_PROFICIENCY_ELF},
    {CLASS_UNDEFINED, RACE_WOOD_ELF, FALSE, 1, FEAT_SLEEP_ENCHANTMENT_IMMUNITY},
    {CLASS_UNDEFINED, RACE_WOOD_ELF, FALSE, 1, FEAT_KEEN_SENSES},
    {CLASS_UNDEFINED, RACE_WOOD_ELF, FALSE, 1, FEAT_RESISTANCE_TO_ENCHANTMENTS},
    {CLASS_UNDEFINED, RACE_WOOD_ELF, FALSE, 1, FEAT_ELF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_WOOD_ELF, FALSE, 1, FEAT_WOOD_ELF_RACIAL_ADJUSTMENT},
    {CLASS_UNDEFINED, RACE_WOOD_ELF, FALSE, 1, FEAT_WOOD_ELF_FLEETNESS},
    {CLASS_UNDEFINED, RACE_WOOD_ELF, FALSE, 1, FEAT_WOOD_ELF_MASK_OF_THE_WILD},

    /*****************************************/
    /* This is always the last array element */
    /*****************************************/
    // Mostly deprecated... still used for racefix
    {CLASS_UNDEFINED, RACE_UNDEFINED, FALSE, 1, FEAT_UNDEFINED}
    // Mostly deprecated... still used for racefix

};

//   give newbie's some eq to start with
#define NUM_NOOB_ARROWS 40
#define NUM_NOOB_DROW_BOLTS 30
#define NOOB_TELEPORTER 82
#define NOOB_TORCH 867
#define NOOB_RATIONS 804
#define NOOB_WATERSKIN 803
#define NOOB_BP 857
#define NOOB_CRAFTING_KIT 3118
#define NOOB_BOW 814
#define NOOB_QUIVER 816
#define NOOB_ARROW 815
#define NOOB_CRAFT_MAT 3135
#define NOOB_CRAFT_MOLD 3176
/* various general items (not gear) */
#define NOOB_WIZ_NOTE 850
#define NOOB_WIZ_SPELLBOOK 812
/* various general gear */
#define NOOB_LEATHER_SLEEVES 854
#define NOOB_LEATHER_LEGGINGS 855
#define NOOB_IRON_MACE 861
#define NOOB_IRON_SHIELD 863
#define NOOB_SCALE_MAIL 807
#define NOOB_STEEL_SCIMITAR 862
#define NOOB_WOOD_SHIELD 864
#define NOOB_STUD_LEATHER 851
#define NOOB_LONG_SWORD 808
#define NOOB_CLOTH_ROBES 809
#define NOOB_DAGGER 852
#define NOOB_CLOTH_SLEEVES 865
#define NOOB_CLOTH_PANTS 866
/* dwarf racial */
#define NOOB_DWARF_WARAXE 806
/* drow racial */
#define NOOB_DROW_XBOW 832
#define NOOB_DROW_BOLT 831
#define NOOB_DROW_POUCH 833
/* bard instrument vnums */
#define LYRE 825
#define FLUTE 826
#define DRUM 827
#define HORN 828
#define HARP 829
#define MANDOLIN 830

/* function that gives chars starting gear */
void newbieEquipment(struct char_data *ch)
{
  int objNums[] = {
      NOOB_BP, /* HAS to be first */
      NOOB_BOW,
      NOOB_TORCH,
      NOOB_RATIONS,
      NOOB_RATIONS,
      NOOB_RATIONS,
      NOOB_RATIONS,
      NOOB_WATERSKIN,
#ifndef CAMPAIGN_FR
      NOOB_TELEPORTER,
      NOOB_CRAFTING_KIT,
      NOOB_CRAFT_MAT,
      NOOB_CRAFT_MAT,
      NOOB_CRAFT_MAT,
      NOOB_CRAFT_MAT,
      NOOB_CRAFT_MOLD,
#endif
      -1 // had to end with -1
  };
  int x;
  struct obj_data *obj = NULL, *quiver = NULL, *pouch = NULL, *bp = NULL;

  send_to_char(ch, "\tMYou are given a set of starting equipment...\tn\r\n");

  // give everyone torch, rations, skin, backpack, bow, etc
  for (x = 0; objNums[x] != -1; x++)
  {
    obj = read_object(objNums[x], VIRTUAL);
    if (obj)
    {

      /* backpack first please! */
      if (objNums[x] == NOOB_BP)
      {
        bp = obj;
        GET_OBJ_SIZE(bp) = GET_SIZE(ch);
        obj_to_char(bp, ch);
      }
      else if (bp)
      { /* we should have a bp already! */
        obj_to_obj(obj, bp);
      }
      else
      { /* problem */
        obj_to_char(bp, ch);
      }
    }
  }

  /* starting quiver/arrows */
  quiver = read_object(NOOB_QUIVER, VIRTUAL);
  if (quiver)
    obj_to_char(quiver, ch);

  for (x = 0; x < NUM_NOOB_ARROWS; x++)
  {
    obj = read_object(NOOB_ARROW, VIRTUAL);
    if (quiver && obj)
      obj_to_obj(obj, quiver);
  }
  /* end items everyone gets */

  /* race specific goodies */
  switch (GET_RACE(ch))
  {
  case RACE_DWARF:
  case RACE_DUERGAR:
    obj = read_object(NOOB_DWARF_WARAXE, VIRTUAL);
    obj_to_char(obj, ch); // dwarven waraxe
    break;
  case RACE_DROW:
    obj = read_object(NOOB_DROW_XBOW, VIRTUAL);
    obj_to_char(obj, ch); // drow hand xbow

    /* pouch and bolts for xbow */
    pouch = read_object(NOOB_DROW_POUCH, VIRTUAL);
    if (pouch)
      obj_to_char(pouch, ch);

    for (x = 0; x < NUM_NOOB_DROW_BOLTS; x++)
    {
      obj = read_object(NOOB_DROW_BOLT, VIRTUAL);
      if (pouch && obj)
        obj_to_obj(obj, pouch);
    }
    break;
#ifdef CAMPAIGN_FR    
  case RACE_VAMPIRE:
    obj = read_object(VAMPIRE_CLOAK_OBJ_VNUM, VIRTUAL);
    obj_to_char(obj, ch); // vampire cloak
    break;
#endif
  default:
    break;
  } /*  end of race specific gear */

  /* class specific gear */
  switch (GET_CLASS(ch))
  {
  case CLASS_PALADIN:
  case CLASS_BLACKGUARD:
    /*fallthrough*/
  case CLASS_CLERIC:
  case CLASS_INQUISITOR:
    // holy symbol, not implemented so took out

    obj = read_object(NOOB_LEATHER_SLEEVES, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // leather sleeves

    obj = read_object(NOOB_LEATHER_LEGGINGS, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // leather leggings

    obj = read_object(NOOB_IRON_MACE, VIRTUAL);
    obj_to_char(obj, ch); // slender iron mace

    obj = read_object(NOOB_IRON_SHIELD, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // shield

    obj = read_object(NOOB_SCALE_MAIL, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // scale mail

    break;

  case CLASS_DRUID:

    obj = read_object(NOOB_LEATHER_SLEEVES, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // leather sleeves

    obj = read_object(NOOB_LEATHER_LEGGINGS, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // leather leggings

    obj = read_object(NOOB_STEEL_SCIMITAR, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // steel scimitar

    obj = read_object(NOOB_WOOD_SHIELD, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // shield (wooden))

    obj = read_object(NOOB_STUD_LEATHER, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // studded leather

    break;

  case CLASS_BERSERKER:
  case CLASS_WARRIOR:
    obj = read_object(NOOB_SCALE_MAIL, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // scale mail
    /*fallthrough!*/
  case CLASS_RANGER:

    obj = read_object(NOOB_STUD_LEATHER, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // studded leather

    obj = read_object(NOOB_LEATHER_SLEEVES, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // leather sleeves

    obj = read_object(NOOB_LEATHER_LEGGINGS, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // leather leggings

    obj = read_object(NOOB_LONG_SWORD, VIRTUAL);
    obj_to_char(obj, ch); // long sword

    obj = read_object(NOOB_IRON_SHIELD, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // shield

    break;

  case CLASS_MONK:
    obj = read_object(NOOB_CLOTH_ROBES, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // cloth robes

    break;

  case CLASS_BARD:
    /* instruments */
    obj = read_object(LYRE, VIRTUAL);
    obj_to_char(obj, ch);
    obj = read_object(FLUTE, VIRTUAL);
    obj_to_char(obj, ch);
    obj = read_object(DRUM, VIRTUAL);
    obj_to_char(obj, ch);
    obj = read_object(HORN, VIRTUAL);
    obj_to_char(obj, ch);
    obj = read_object(HARP, VIRTUAL);
    obj_to_char(obj, ch);
    obj = read_object(MANDOLIN, VIRTUAL);
    obj_to_char(obj, ch);

    /*FALL THROUGH*/
  case CLASS_ROGUE:
    obj = read_object(NOOB_LEATHER_SLEEVES, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // leather sleeves

    obj = read_object(NOOB_LEATHER_LEGGINGS, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // leather leggings

    obj = read_object(NOOB_STUD_LEATHER, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // studded leather

    obj = read_object(NOOB_DAGGER, VIRTUAL);
    obj_to_char(obj, ch); // dagger

    obj = read_object(NOOB_DAGGER, VIRTUAL);
    obj_to_char(obj, ch); // dagger

    break;

  case CLASS_ALCHEMIST:

    obj = read_object(NOOB_LEATHER_SLEEVES, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // leather sleeves

    obj = read_object(NOOB_LEATHER_LEGGINGS, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // leather leggings

    obj = read_object(NOOB_IRON_MACE, VIRTUAL);
    obj_to_char(obj, ch); // slender iron mace

    obj = read_object(NOOB_STUD_LEATHER, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // scale mail

  case CLASS_WIZARD:
    obj_to_char(read_object(NOOB_WIZ_NOTE, VIRTUAL), ch);      // wizard note
    obj_to_char(read_object(NOOB_WIZ_SPELLBOOK, VIRTUAL), ch); // spellbook
    /* switch fallthrough */
  case CLASS_SORCERER:
  case CLASS_PSIONICIST:
    obj = read_object(NOOB_CLOTH_SLEEVES, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // cloth sleeves

    obj = read_object(NOOB_CLOTH_PANTS, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // cloth pants

    obj = read_object(NOOB_DAGGER, VIRTUAL);
    // GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // dagger

    obj = read_object(NOOB_CLOTH_ROBES, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // cloth robes

    break;
  default:
    log("Invalid class sent to newbieEquipment class!");
    break;
  } /*  end of class specific gear */
}
#undef LYRE
#undef FLUTE
#undef DRUM
#undef HORN
#undef HARP
#undef MANDOLIN
/**/
#undef NUM_NOOB_ARROWS
#undef NUM_NOOB_DROW_BOLTS
#undef NOOB_TELEPORTER
#undef NOOB_TORCH
#undef NOOB_RATIONS
#undef NOOB_WATERSKIN
#undef NOOB_BP
#undef NOOB_CRAFTING_KIT
#undef NOOB_BOW
#undef NOOB_QUIVER
#undef NOOB_ARROW
/**/
#undef NOOB_WIZ_NOTE
#undef NOOB_WIZ_SPELLBOOK
/**/
#undef NOOB_LEATHER_SLEEVES
#undef NOOB_LEATHER_LEGGINGS
#undef NOOB_IRON_MACE
#undef NOOB_IRON_SHIELD
#undef NOOB_SCALE_MAIL
#undef NOOB_STEEL_SCIMITAR
#undef NOOB_WOOD_SHIELD
#undef NOOB_STUD_LEATHER
#undef NOOB_LONG_SWORD
#undef NOOB_CLOTH_ROBES
#undef NOOB_DAGGER
#undef NOOB_CLOTH_SLEEVES
#undef NOOB_CLOTH_PANTS
/**/
#undef NOOB_DWARF_WARAXE
/**/
#undef NOOB_DROW_XBOW
#undef NOOB_DROW_BOLT
#undef NOOB_DROW_POUCH

/* this is used to assign all the spells */
void init_class(struct char_data *ch, int class, int level)
{
  struct class_spell_assign *spell_assign = NULL;

  if (class_list[class].spellassign_list != NULL)
  {
    /*  This class has spell assignment! Traverse the list and check. */
    for (spell_assign = class_list[class].spellassign_list; spell_assign != NULL;
         spell_assign = spell_assign->next)
    {
      SET_SKILL(ch, spell_assign->spell_num, 99);
    }
  }

  switch (class)
  {
  case CLASS_CLERIC:
  case CLASS_INQUISITOR:
    /* we also have to add this to study where we set our domains */
    assign_domain_spells(ch);
    break;
  default:
    break;
  }
}

/* not to be confused with init_char, this is exclusive right now for do_start */
void init_start_char(struct char_data *ch)
{
  int trains = 0, i = 0, j = 0;

  /* leave group */
  if (GROUP(ch))
    leave_group(ch);

  /* handle followers */
  if (ch->followers || ch->master)
    die_follower(ch);

  /* clear polymorph, affections cleared below */
  SUBRACE(ch) = 0;
  IS_MORPHED(ch) = 0;
  GET_DISGUISE_RACE(ch) = 0;
  cleanup_disguise(ch);
  GET_SPECIALTY_SCHOOL(ch) = 0;

  for (i = 0; i < NUM_PALADIN_MERCIES; i++)
    KNOWS_MERCY(ch, i) = 0;
  for (i = 0; i < NUM_BLACKGUARD_CRUELTIES; i++)
    KNOWS_CRUELTY(ch, i) = 0;
  for (i = 0; i < NUM_LANGUAGES; i++)
    ch->player_specials->saved.languages_known[i] = FALSE;
  ch->player_specials->saved.fiendish_boons = 0;
  ch->player_specials->saved.channel_energy_type = 0;
  // this is here so that new characters can't get extra stat points from racefix command.
  ch->player_specials->saved.new_race_stats = true;

  /* clear immortal flags */
  if (PRF_FLAGGED(ch, PRF_HOLYLIGHT))
    i = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
  if (PRF_FLAGGED(ch, PRF_NOHASSLE))
    i = PRF_TOG_CHK(ch, PRF_NOHASSLE);
  if (PRF_FLAGGED(ch, PRF_SHOWVNUMS))
    i = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
  if (PRF_FLAGGED(ch, PRF_BUILDWALK))
    i = PRF_TOG_CHK(ch, PRF_BUILDWALK);

  /* clear gear for clean start */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      perform_remove(ch, i, TRUE);

  /* clear affects for clean start */
  if (ch->affected || AFF_FLAGS(ch))
  {
    while (ch->affected)
      affect_remove(ch, ch->affected);
    for (i = 0; i < AF_ARRAY_MAX; i++)
      AFF_FLAGS(ch)
    [i] = 0;
  }

  /* initialize all levels and spec_abil array */
  for (i = 0; i < MAX_CLASSES; i++)
  {
    CLASS_LEVEL(ch, i) = 0;
    GET_SPEC_ABIL(ch, i) = 0;
  }
  for (i = 0; i < MAX_ENEMIES; i++)
    GET_FAVORED_ENEMY(ch, i) = 0;

  /* for sorcerer bloodlines subtypes */
  GET_BLOODLINE_SUBTYPE(ch) = 0;

  // alchemist discoveries
  for (i = 0; i < NUM_ALC_DISCOVERIES; i++)
    KNOWS_DISCOVERY(ch, i) = 0;
  GET_GRAND_DISCOVERY(ch) = 0;

  for (i = 0; i < 3; i++)
    NEW_ARCANA_SLOT(ch, i) = 0;

  /* a bit silly, but go ahead make sure no stone-skin like spells */
  for (i = 0; i < MAX_WARDING; i++)
    GET_WARDING(ch, i) = 0;

  /* start at level 1 */
  GET_LEVEL(ch) = 1;
  CLASS_LEVEL(ch, GET_CLASS(ch)) = 1;
  GET_EXP(ch) = 1;

  /* reset title */
  set_title(ch, NULL);

  /* reset stats */
  roll_real_abils(ch);
  GET_REAL_AC(ch) = 100; /* base AC of 10 */
  GET_REAL_HITROLL(ch) = 0;
  GET_REAL_DAMROLL(ch) = 0;
  GET_REAL_MAX_HIT(ch) = 20;
  GET_REAL_MAX_PSP(ch) = 3;
  GET_REAL_MAX_MOVE(ch) = 820;
  GET_PRACTICES(ch) = 0;
  GET_TRAINS(ch) = 0;
  GET_BOOSTS(ch) = 0;
  GET_FEAT_POINTS(ch) = 0;
  GET_EPIC_FEAT_POINTS(ch) = 0;

  for (i = 0; i < NUM_CLASSES; i++)
  {
    GET_CLASS_FEATS(ch, i) = 0;
    GET_EPIC_CLASS_FEATS(ch, i) = 0;
  }

  GET_REAL_SPELL_RES(ch) = 0;

  /* gotta clear DR */
  if (GET_DR(ch) != NULL)
  {
    struct damage_reduction_type *dr, *tmp;
    dr = GET_DR(ch);
    while (dr != NULL)
    {
      tmp = dr;
      dr = dr->next;
      free(tmp);
    }
  }
  GET_DR(ch) = NULL;

  /* gonna clear the condensed combat data if it exists */
  if (CNDNSD(ch))
    free(CNDNSD(ch));
  CNDNSD(ch) = NULL;

  /* reset skills/abilities */
  /* we don't want players to lose their hard-earned crafting skills */
  for (i = START_SKILLS; i < NUM_SKILLS; i++)
    if (spell_info[i].schoolOfMagic != CRAFTING_SKILL)
      SET_SKILL(ch, i, 0);
  for (i = 1; i <= NUM_ABILITIES; i++)
    SET_ABILITY(ch, i, 0);
  for (i = 1; i < NUM_FEATS; i++)
    SET_FEAT(ch, i, 0);
  for (i = 0; i < NUM_CFEATS; i++)
    for (j = 0; j < FT_ARRAY_MAX; j++)
      (ch)->char_specials.saved.combat_feats[(i)][j] = 0;
  for (i = 0; i < NUM_SFEATS; i++)
    (ch)->char_specials.saved.school_feats[(i)] = 0;
  for (i = 0; i < MAX_ABILITIES; i++)
    for (j = 0; j < NUM_SKFEATS; j++)
      (ch)->player_specials->saved.skill_focus[(i)][j] = 0;
  for (i = 0; i < MAX_BOMBS_ALLOWED; i++)
    GET_BOMB(ch, i) = 0;

  /* initialize spell prep data, allow adjustment of spells known */
  destroy_spell_prep_queue(ch);
  destroy_innate_magic_queue(ch);
  destroy_spell_collection(ch);
  destroy_known_spells(ch);

  IS_SORC_LEARNED(ch) = 0;
  IS_BARD_LEARNED(ch) = 0;
  IS_RANG_LEARNED(ch) = 0;
  IS_WIZ_LEARNED(ch) = 0;
  IS_DRUID_LEARNED(ch) = 0;

  /* hunger and thirst are off */
  GET_COND(ch, HUNGER) = -1;
  GET_COND(ch, THIRST) = -1;

  /* stat modifications */
  /* removed by gicker june 8 2020 for alternate system */
  /*
  GET_REAL_CON(ch) += get_race_stat(GET_RACE(ch), R_CON_MOD);
  GET_REAL_STR(ch) += get_race_stat(GET_RACE(ch), R_STR_MOD);
  GET_REAL_DEX(ch) += get_race_stat(GET_RACE(ch), R_DEX_MOD);
  GET_REAL_INT(ch) += get_race_stat(GET_RACE(ch), R_INTEL_MOD);
  GET_REAL_WIS(ch) += get_race_stat(GET_RACE(ch), R_WIS_MOD);
  GET_REAL_CHA(ch) += get_race_stat(GET_RACE(ch), R_CHA_MOD);
  */

  /* setting racial size here */
  GET_REAL_SIZE(ch) = race_list[GET_RACE(ch)].size;

  /* some racial related modifications */
  switch (GET_RACE(ch))
  {
  case RACE_HUMAN:
    GET_FEAT_POINTS(ch)
    ++;
    trains += 3;
    break;
  case RACE_CRYSTAL_DWARF:
    GET_MAX_HIT(ch) += 10; /* vital */
    break;
  case RACE_TRELUX:
    GET_MAX_HIT(ch) += 10; /* vital */
    break;
  case RACE_LICH:
    GET_MAX_HIT(ch) += 10; /* vital */
    break;
  case RACE_VAMPIRE:
    GET_MAX_HIT(ch) += 10; /* vital */
    break;
  case RACE_HALF_TROLL:
  case RACE_ARCANA_GOLEM:
  case RACE_DROW:
  case RACE_ELF:
  case RACE_DUERGAR:
  case RACE_DWARF:
  case RACE_HALFLING:
  case RACE_H_ELF:
  case RACE_H_ORC:
  case RACE_GNOME:
  default:
    break;
  }

  /* warrior bonus */
  if (GET_CLASS(ch) == CLASS_WARRIOR)
    GET_CLASS_FEATS(ch, CLASS_WARRIOR)
  ++; /* Bonus Feat */

  /* when you study it reinitializes your trains now */
  int int_bonus = GET_INT_BONUS(ch); /* this is the way it should be */

  /* assign trains, this gets over-written anyhow during study session at lvl 1 */
  trains += MAX(1, (CLSLIST_TRAINS(GET_CLASS(ch)) + (int)(int_bonus)) * 3);

  /* finalize */
  GET_FEAT_POINTS(ch)
  ++; /* 1st level feat. */
  send_to_char(ch, "%d \tMFeat points gained.\tn\r\n", GET_FEAT_POINTS(ch));
  send_to_char(ch, "%d \tMClass Feat points gained.\tn\r\n", GET_CLASS_FEATS(ch, GET_CLASS(ch)));
  GET_TRAINS(ch) += trains;
  send_to_char(ch, "%d \tMTraining sessions gained.\tn\r\n", trains);

  save_char(ch, 1);
}

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  init_start_char(ch);

  // from level 0 -> level 1
  /* our function for leveling up, takes in class that is being advanced */
  advance_level(ch, GET_CLASS(ch));
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_PSP(ch) = GET_MAX_PSP(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  GET_COND(ch, DRUNK) = 0;
  if (CONFIG_SITEOK_ALL)
    SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);
  if (GET_PREMADE_BUILD_CLASS(ch) != CLASS_UNDEFINED)
    advance_premade_build(ch);
}

bool special_handling_level_feats(struct char_data *ch, int feat_num)
{

  switch (feat_num)
  {
  case FEAT_SNEAK_ATTACK:
    send_to_char(ch, "\tMYour sneak attack has increased to +%dd6!\tn\r\n", HAS_FEAT(ch, FEAT_SNEAK_ATTACK) + 1);
    return TRUE;

  case FEAT_SHRUG_DAMAGE:
    send_to_char(ch, "\tMYou can now shrug off %d damage!\tn\r\n", HAS_FEAT(ch, FEAT_SHRUG_DAMAGE) + 1);
    return TRUE;

  case FEAT_STRENGTH_BOOST:
    ch->real_abils.str += 2;
    send_to_char(ch, "\tMYour natural strength has increased by +2!\r\n");
    return TRUE;

  case FEAT_CHARISMA_BOOST:
    ch->real_abils.cha += 2;
    send_to_char(ch, "\tMYour natural charisma has increased by +2!\r\n");
    return TRUE;

  case FEAT_CONSTITUTION_BOOST:
    ch->real_abils.con += 2;
    send_to_char(ch, "\tMYour natural constitution has increased by +2!\r\n");
    return TRUE;

  case FEAT_INTELLIGENCE_BOOST:
    ch->real_abils.intel += 2;
    send_to_char(ch, "\tMYour natural intelligence has increased by +2!\r\n");
    return TRUE;

  default:
    break;
  }

  return FALSE;
}

/* at each level we run this function to assign free CLASS feats */
void process_class_level_feats(struct char_data *ch, int class)
{
  struct class_feat_assign *feat_assign = NULL;
  int class_level = -1, effective_class_level = -1;

  /* deal with some instant disqualification */
  if (class < 0 || class >= NUM_CLASSES)
    return;

  class_level = CLASS_LEVEL(ch, class);

  if (class_level <= 0)
    return;

  if (class_list[class].featassign_list == NULL)
    return;

  /*  This class has potential feat assignment! Traverse the list and assign. */
  for (feat_assign = class_list[class].featassign_list; feat_assign != NULL;
       feat_assign = feat_assign->next)
  {

    /* Mystic Theurge levels stack with class levels for purposes of granting spell access. */
    if (IS_SPELL_CIRCLE_FEAT(feat_assign->feat_num))
    {
      effective_class_level = class_level + CLASS_LEVEL(ch, CLASS_MYSTIC_THEURGE);
    }
    else
    {
      effective_class_level = class_level;
    }

    /* appropriate level to receive this feat? */
    if (feat_assign->level_received == effective_class_level)
    {

      /* any special handling for this feat? */
      if (!special_handling_level_feats(ch, feat_assign->feat_num))
      {

        /* no special handling */
        if (HAS_FEAT(ch, feat_assign->feat_num))
        {
          send_to_char(ch, "\tM[class] You have improved your %s %s!\tn\r\n",
                       feat_list[feat_assign->feat_num].name,
                       feat_types[feat_list[feat_assign->feat_num].feat_type]);
        }
        else
        {
          send_to_char(ch, "\tM[class] You have gained the %s %s!\tn\r\n",
                       feat_list[feat_assign->feat_num].name,
                       feat_types[feat_list[feat_assign->feat_num].feat_type]);
        }
      }

      /* now actually adjust the feat */
      SET_FEAT(ch, feat_assign->feat_num, HAS_REAL_FEAT(ch, feat_assign->feat_num) + 1);
    }
  }
}

/* at each level we run this function to assign free RACE feats */
void process_race_level_feats(struct char_data *ch)
{
  struct race_feat_assign *feat_assign = NULL;

  if (race_list[GET_RACE(ch)].featassign_list == NULL)
    return;

  /*  This race has potential feat assignment! Traverse the list and assign. */
  for (feat_assign = race_list[GET_RACE(ch)].featassign_list; feat_assign != NULL;
       feat_assign = feat_assign->next)
  {

    /* appropriate level to receive this feat? */
    if (feat_assign->level_received == GET_LEVEL(ch))
    {

      /* any special handling for this feat? */
      if (!special_handling_level_feats(ch, feat_assign->feat_num))
      {

        /* no special handling */
        if (HAS_FEAT(ch, feat_assign->feat_num))
        {
          send_to_char(ch, "\tM[race] You have improved your %s %s!\tn\r\n",
                       feat_list[feat_assign->feat_num].name,
                       feat_types[feat_list[feat_assign->feat_num].feat_type]);
        }
        else
        {
          send_to_char(ch, "\tM[race] You have gained the %s %s!\tn\r\n",
                       feat_list[feat_assign->feat_num].name,
                       feat_types[feat_list[feat_assign->feat_num].feat_type]);
        }
      }

      /* now actually adjust the feat */
      SET_FEAT(ch, feat_assign->feat_num, HAS_REAL_FEAT(ch, feat_assign->feat_num) + 1);
    }
  }
}

#define GRANT_SPELL_CIRCLE(class, first, epic)                                                                             \
  if ((lvl = CLASS_LEVEL(ch, class)) > 0)                                                                                  \
  {                                                                                                                        \
    for (feat_assign = class_list[class].featassign_list; feat_assign != NULL;                                             \
         feat_assign = feat_assign->next)                                                                                  \
    {                                                                                                                      \
      feat_num = feat_assign->feat_num;                                                                                    \
      if (feat_num >= first && feat_num <= epic && !HAS_FEAT(ch, feat_num) && feat_assign->level_received == lvl + mystic) \
      {                                                                                                                    \
        SET_FEAT(ch, feat_num, 1);                                                                                         \
        send_to_char(ch, "You have gained access to %s!\r\n", feat_list[feat_num].name);                                   \
      }                                                                                                                    \
    }                                                                                                                      \
  }

void process_conditional_class_level_feats(struct char_data *ch, int class)
{

  switch (class)
  {
  case CLASS_SORCERER:
    //  Mostly Bloodlines
    if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_DRACONIC))
    {
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 9 && !HAS_REAL_FEAT(ch, FEAT_DRACONIC_HERITAGE_BREATHWEAPON))
      {
        SET_FEAT(ch, FEAT_DRACONIC_HERITAGE_BREATHWEAPON, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_DRACONIC_HERITAGE_BREATHWEAPON].name);
      }
      if (!HAS_REAL_FEAT(ch, FEAT_DRACONIC_HERITAGE_CLAWS))
      {
        SET_FEAT(ch, FEAT_DRACONIC_HERITAGE_CLAWS, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_DRACONIC_HERITAGE_CLAWS].name);
      }
      if (!HAS_REAL_FEAT(ch, FEAT_DRACONIC_HERITAGE_DRAGON_RESISTANCES) && CLASS_LEVEL(ch, CLASS_SORCERER) >= 3)
      {
        SET_FEAT(ch, FEAT_DRACONIC_HERITAGE_DRAGON_RESISTANCES, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_DRACONIC_HERITAGE_DRAGON_RESISTANCES].name);
      }
      if (!HAS_REAL_FEAT(ch, FEAT_DRACONIC_BLOODLINE_ARCANA))
      {
        SET_FEAT(ch, FEAT_DRACONIC_BLOODLINE_ARCANA, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_DRACONIC_BLOODLINE_ARCANA].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 15 && !HAS_REAL_FEAT(ch, FEAT_DRACONIC_HERITAGE_WINGS))
      {
        SET_FEAT(ch, FEAT_DRACONIC_HERITAGE_WINGS, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_DRACONIC_HERITAGE_WINGS].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 20 && !HAS_REAL_FEAT(ch, FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS))
      {
        SET_FEAT(ch, FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS].name);
        SET_FEAT(ch, FEAT_BLINDSENSE, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_BLINDSENSE].name);
      }
    }
    else if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_ARCANE))
    {
      if (!HAS_REAL_FEAT(ch, FEAT_ARCANE_BLOODLINE_ARCANA))
      {
        SET_FEAT(ch, FEAT_ARCANE_BLOODLINE_ARCANA, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_ARCANE_BLOODLINE_ARCANA].name);
      }
      if (!HAS_REAL_FEAT(ch, FEAT_IMPROVED_FAMILIAR))
      {
        SET_FEAT(ch, FEAT_IMPROVED_FAMILIAR, 1);
        send_to_char(ch, "You have gained the %s feat as a benefit of your arcane bloodline!\r\n", feat_list[FEAT_IMPROVED_FAMILIAR].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 3 && !HAS_REAL_FEAT(ch, FEAT_METAMAGIC_ADEPT))
      {
        SET_FEAT(ch, FEAT_METAMAGIC_ADEPT, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_METAMAGIC_ADEPT].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 9 && !HAS_REAL_FEAT(ch, FEAT_NEW_ARCANA))
      {
        SET_FEAT(ch, FEAT_NEW_ARCANA, 1);
        send_to_char(ch, "You have gained the %s feat!  You can now learn a bonus spell from among the spell circles you can currently cast.\r\n", feat_list[FEAT_NEW_ARCANA].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 13 && HAS_REAL_FEAT(ch, FEAT_NEW_ARCANA) == 1)
      {
        SET_FEAT(ch, FEAT_NEW_ARCANA, 2);
        send_to_char(ch, "You have improved the %s feat!  You can now learn a bonus spell from among the spell circles you can currently cast.\r\n", feat_list[FEAT_NEW_ARCANA].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 17 && HAS_REAL_FEAT(ch, FEAT_NEW_ARCANA) == 2)
      {
        SET_FEAT(ch, FEAT_NEW_ARCANA, 3);
        send_to_char(ch, "You have improved the %s feat!  You can now learn a bonus spell from among the spell circles you can currently cast.\r\n", feat_list[FEAT_NEW_ARCANA].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 15 && !HAS_REAL_FEAT(ch, FEAT_SCHOOL_POWER))
      {
        SET_FEAT(ch, FEAT_SCHOOL_POWER, 1);
        send_to_char(ch, "You have gained the %s (%s) feat!\r\n", feat_list[FEAT_SCHOOL_POWER].name, school_names[GET_BLOODLINE_SUBTYPE(ch)]);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 20 && !HAS_REAL_FEAT(ch, FEAT_ARCANE_APOTHEOSIS))
      {
        SET_FEAT(ch, FEAT_ARCANE_APOTHEOSIS, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_ARCANE_APOTHEOSIS].name);
      }
    }
    else if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_FEY))
    {
      if (!HAS_REAL_FEAT(ch, FEAT_FEY_BLOODLINE_ARCANA))
      {
        SET_FEAT(ch, FEAT_FEY_BLOODLINE_ARCANA, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_FEY_BLOODLINE_ARCANA].name);
      }
      if (!HAS_REAL_FEAT(ch, FEAT_LAUGHING_TOUCH))
      {
        SET_FEAT(ch, FEAT_LAUGHING_TOUCH, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_LAUGHING_TOUCH].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 3 && !HAS_REAL_FEAT(ch, FEAT_WOODLAND_STRIDE))
      {
        SET_FEAT(ch, FEAT_WOODLAND_STRIDE, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_WOODLAND_STRIDE].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 9 && !HAS_REAL_FEAT(ch, FEAT_FLEETING_GLANCE))
      {
        SET_FEAT(ch, FEAT_FLEETING_GLANCE, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_FLEETING_GLANCE].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 15 && !HAS_REAL_FEAT(ch, FEAT_FEY_MAGIC))
      {
        SET_FEAT(ch, FEAT_FEY_MAGIC, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_FEY_MAGIC].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 20 && !HAS_REAL_FEAT(ch, FEAT_SOUL_OF_THE_FEY))
      {
        SET_FEAT(ch, FEAT_SOUL_OF_THE_FEY, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_SOUL_OF_THE_FEY].name);
      }
    }
    else if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_UNDEAD))
    {
      if (!HAS_REAL_FEAT(ch, FEAT_UNDEAD_BLOODLINE_ARCANA))
      {
        SET_FEAT(ch, FEAT_UNDEAD_BLOODLINE_ARCANA, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_UNDEAD_BLOODLINE_ARCANA].name);
      }
      if (!HAS_REAL_FEAT(ch, FEAT_GRAVE_TOUCH))
      {
        SET_FEAT(ch, FEAT_GRAVE_TOUCH, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_GRAVE_TOUCH].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 3 && !HAS_REAL_FEAT(ch, FEAT_DEATHS_GIFT))
      {
        SET_FEAT(ch, FEAT_DEATHS_GIFT, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_DEATHS_GIFT].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 9 && !HAS_REAL_FEAT(ch, FEAT_GRASP_OF_THE_DEAD))
      {
        SET_FEAT(ch, FEAT_GRASP_OF_THE_DEAD, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_GRASP_OF_THE_DEAD].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 15 && !HAS_REAL_FEAT(ch, FEAT_INCORPOREAL_FORM))
      {
        SET_FEAT(ch, FEAT_INCORPOREAL_FORM, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_INCORPOREAL_FORM].name);
      }
      if (CLASS_LEVEL(ch, CLASS_SORCERER) >= 20 && !HAS_REAL_FEAT(ch, FEAT_ONE_OF_US))
      {
        SET_FEAT(ch, FEAT_ONE_OF_US, 1);
        send_to_char(ch, "You have gained the %s feat!\r\n", feat_list[FEAT_ONE_OF_US].name);
      }
    }
    break;
  case CLASS_MYSTIC_THEURGE:
  {
    // This is how extra circles are assigned.
    struct class_feat_assign *feat_assign = NULL;
    int mystic, feat_num, lvl;
    mystic = HAS_FEAT(ch, FEAT_THEURGE_SPELLCASTING);

    GRANT_SPELL_CIRCLE(CLASS_WIZARD, FEAT_WIZARD_1ST_CIRCLE, FEAT_WIZARD_EPIC_SPELL);
    GRANT_SPELL_CIRCLE(CLASS_CLERIC, FEAT_CLERIC_1ST_CIRCLE, FEAT_CLERIC_EPIC_SPELL);
    GRANT_SPELL_CIRCLE(CLASS_BARD, FEAT_BARD_1ST_CIRCLE, FEAT_BARD_EPIC_SPELL);
    GRANT_SPELL_CIRCLE(CLASS_INQUISITOR, FEAT_INQUISITOR_1ST_CIRCLE, FEAT_INQUISITOR_EPIC_SPELL);
    GRANT_SPELL_CIRCLE(CLASS_DRUID, FEAT_DRUID_1ST_CIRCLE, FEAT_DRUID_EPIC_SPELL);
    GRANT_SPELL_CIRCLE(CLASS_SORCERER, FEAT_SORCERER_1ST_CIRCLE, FEAT_SORCERER_EPIC_SPELL);
    GRANT_SPELL_CIRCLE(CLASS_PALADIN, FEAT_PALADIN_1ST_CIRCLE, FEAT_PALADIN_4TH_CIRCLE);
    GRANT_SPELL_CIRCLE(CLASS_RANGER, FEAT_RANGER_1ST_CIRCLE, FEAT_RANGER_4TH_CIRCLE);
    GRANT_SPELL_CIRCLE(CLASS_BLACKGUARD, FEAT_BLACKGUARD_1ST_CIRCLE, FEAT_BLACKGUARD_4TH_CIRCLE);
  }
  break;
  }
}
#undef GRANT_SPELL_CIRCLE

/* deprecated */
/* at each level we run this function to assign free RACE feats */

void process_level_feats(struct char_data *ch, int class)
{
  char featbuf[MAX_STRING_LENGTH] = {'\0'};
  char tmp_buf[MAX_STRING_LENGTH] = {'\0'};
  int i = 0;

  snprintf(featbuf, sizeof(featbuf), "\tM");

  /* increment through the list, FEAT_UNDEFINED is our terminator */
  while (level_feats[i][LF_FEAT] != FEAT_UNDEFINED)
  {

    /* feat i doesnt matches our class or we don't meet the min-level (from if above) */
    /* non-class, racial feat and don't have it yet */
    if (level_feats[i][LF_CLASS] == CLASS_UNDEFINED &&
        level_feats[i][LF_RACE] == GET_RACE(ch) &&
        level_feats[i][LF_MIN_LVL] == GET_LEVEL(ch))
    {
      if (HAS_FEAT(ch, level_feats[i][LF_FEAT]))
      {
        snprintf(tmp_buf, sizeof(tmp_buf), "\tMYou have improved your %s %s!\tn\r\n",
                 feat_list[level_feats[i][LF_FEAT]].name,
                 feat_types[feat_list[level_feats[i][LF_FEAT]].feat_type]);
        strlcat(featbuf, tmp_buf, sizeof(featbuf));
        SET_FEAT(ch, level_feats[i][LF_FEAT], HAS_REAL_FEAT(ch, level_feats[i][LF_FEAT]) + 1);
      }
      else if (!HAS_FEAT(ch, level_feats[i][LF_FEAT]))
      {
        snprintf(tmp_buf, sizeof(tmp_buf), "\tMYou have gained the %s %s!\tn\r\n",
                 feat_list[level_feats[i][LF_FEAT]].name,
                 feat_types[feat_list[level_feats[i][LF_FEAT]].feat_type]);
        strlcat(featbuf, tmp_buf, sizeof(featbuf));
        SET_FEAT(ch, level_feats[i][LF_FEAT], HAS_REAL_FEAT(ch, level_feats[i][LF_FEAT]) + 1);
      }
    }

    /* counter */
    i++;
  }

  send_to_char(ch, "%s", featbuf);
}

/* our function for leveling up, takes in class that is being advanced */
void advance_level(struct char_data *ch, int class)
{
  int add_hp = 0, at_armor = 100,
      add_psp = 0, add_move = 0, k, trains = 0;
  int feats = 0, class_feats = 0, epic_feats = 0, epic_class_feats = 0;
  int i = 0;

  /**because con items / spells are affecting based on level, we have to
  unaffect before we level up -zusuk */
  at_armor = affect_total_sub(ch); /* at_armor stores ac */
  /* done unaffecting */

  add_hp = GET_CON_BONUS(ch);

  add_hp += CONFIG_EXTRA_PLAYER_HP_PER_LEVEL;
  add_move += CONFIG_EXTRA_PLAYER_MV_PER_LEVEL;

  if (class == CLASS_PSIONICIST)
  {
    add_psp += GET_LEVEL(ch) + 2;
    if (CLASS_LEVEL(ch, class) == 1)
      add_psp += GET_REAL_INT_BONUS(ch);
    if (HAS_REAL_FEAT(ch, FEAT_PROFICIENT_PSIONICIST))
      add_psp++;
  }

  /* first level in a class?  might have some inits to do! */
  if (CLASS_LEVEL(ch, class) == 1)
  {
    send_to_char(ch, "\tMInitializing class:  \r\n");
    init_class(ch, class, CLASS_LEVEL(ch, class));
    send_to_char(ch, "\r\n");
  }

  /* start our primary advancement block */
  send_to_char(ch, "\tMGAINS:\tn\r\n");

  /* calculate hps gain */
  // we're not doing random amounts anymore.  Instead they get full hit dice each level
  // add_hp += rand_number(CLSLIST_HPS(class) / 2, CLSLIST_HPS(class));
  add_hp += CLSLIST_HPS(class);

  /* calculate moves gain */
  add_move += rand_number(1, CLSLIST_MVS(class));

  /* calculate trains gained */
  trains += MAX(1, (CLSLIST_TRAINS(class) + (GET_REAL_INT_BONUS(ch))));

  /* pre epic special class feat progression */
  if (class == CLASS_WIZARD && !(CLASS_LEVEL(ch, CLASS_WIZARD) % 5))
  {
    if (CLASS_LEVEL(ch, CLASS_WIZARD) <= 20)
      class_feats++; // wizards get a bonus class feat every 5 levels
    // else if (IS_EPIC(ch))
    // epic_class_feats++;
  }

  if (class == CLASS_PSIONICIST && (!(CLASS_LEVEL(ch, CLASS_PSIONICIST) % 5) || CLASS_LEVEL(ch, CLASS_PSIONICIST) == 1))
  {
    if (CLASS_LEVEL(ch, CLASS_PSIONICIST) <= 20)
      class_feats++; // psionicists get a bonus class feat every 5 levels
  }

  if (class == CLASS_WARRIOR)
  {
    if (CLASS_LEVEL(ch, CLASS_WARRIOR) <= 20 && !(CLASS_LEVEL(ch, CLASS_WARRIOR) % 2))
      class_feats++; // warriors get a bonus class feat every 2 levels
    // else if (IS_EPIC(ch))
    // epic_class_feats++;
  }

  if (class == CLASS_BARD)
  {
    if (CLASS_LEVEL(ch, CLASS_BARD) <= 20 && !(CLASS_LEVEL(ch, CLASS_BARD) % 3))
      feats++; // bards get a bonus feat every 3 levels
    else if (IS_EPIC(ch))
      epic_feats++;
  }

  if (class == CLASS_ELDRITCH_KNIGHT && (CLASS_LEVEL(ch, CLASS_ELDRITCH_KNIGHT) == 1 ||
                                         CLASS_LEVEL(ch, CLASS_ELDRITCH_KNIGHT) == 5 || CLASS_LEVEL(ch, CLASS_ELDRITCH_KNIGHT) == 9))
  {
    class_feats++; // Eldritch Knights get a bonus feat on levels 1, 5, and 9
  }

  if (class == CLASS_SORCERER && ((CLASS_LEVEL(ch, CLASS_SORCERER) - 1) % 6 == 0) &&
      CLASS_LEVEL(ch, CLASS_SORCERER) > 1)
  {
    class_feats++;
  }

  if (class == CLASS_SPELLSWORD && CLASS_LEVEL(ch, CLASS_SPELLSWORD) == 2)
  {
    class_feats++;
  }

  /* epic class feat progresion */
  if (CLSLIST_EFEATP(class) && !(CLASS_LEVEL(ch, class) % CLSLIST_EFEATP(class)) && IS_EPIC(ch))
  {
    epic_class_feats++;
  }

  /* further movement modifications */
  if (HAS_FEAT(ch, FEAT_ENDURANCE))
  {
    add_move += rand_number(10, 20);
  }
  if (HAS_FEAT(ch, FEAT_FAST_MOVEMENT))
  {
    add_move += rand_number(10, 20);
  }
  if (HAS_FEAT(ch, FEAT_WOOD_ELF_FLEETNESS))
  {
    add_move += 2;
  }

  /* 'free' race feats gained (old system) */
  // process_level_feats(ch, class);

  /* 'free' race feats gained */
  process_race_level_feats(ch);
  /* 'free' class feats gained */
  process_class_level_feats(ch, class);
  /* 'free' class feats gained that depend on previous class or feat choices */
  process_conditional_class_level_feats(ch, class);

  // Racial Bonuses
  switch (GET_RACE(ch))
  {
  case RACE_HUMAN:
    trains++;
    break;
  case RACE_CRYSTAL_DWARF:
    add_hp += 4;
    break;
  case RACE_TRELUX:
    add_hp += 4;
    break;
  case RACE_LICH:
    add_hp += 4;
    break;
  case RACE_VAMPIRE:
    add_hp += 4;
    break;
  default:
    break;
  }

  // base practice / boost improvement
  if (!(GET_LEVEL(ch) % 3) && !IS_EPIC(ch))
  {
    feats++;
  }
  if (!(GET_LEVEL(ch) % 3) && IS_EPIC(ch))
  {
    epic_feats++;
  }
  if (!(GET_LEVEL(ch) % 4))
  {
    GET_BOOSTS(ch)
    ++;
    if (GET_PREMADE_BUILD_CLASS(ch) != CLASS_UNDEFINED)
      send_to_char(ch, "\tMYou gain a boost (to stats) point!\tn\r\n");
  }

  /* miscellaneous level-based bonuses */
  if (HAS_FEAT(ch, FEAT_TOUGHNESS))
  {
    /* SRD has this as +3 hp.  More fun as +1 per level. */
    for (i = HAS_FEAT(ch, FEAT_TOUGHNESS); i > 0; i--)
      add_hp++;
  }

  if (HAS_FEAT(ch, FEAT_GOLD_DWARF_TOUGHNESS))
  {
    add_hp++;
  }

  // we're using more move points now
  add_move *= 10;

  /* adjust final and report changes! */
  GET_REAL_MAX_HIT(ch) += MAX(1, add_hp);
  send_to_char(ch, "\tMTotal HP:\tn %d\r\n", MAX(1, add_hp));
  GET_REAL_MAX_MOVE(ch) += MAX(1, add_move);
  send_to_char(ch, "\tMTotal Move:\tn %d\r\n", MAX(1, add_move));
  if (add_psp > 0)
  {
    GET_REAL_MAX_PSP(ch) += MAX(1, add_psp);
    send_to_char(ch, "\tMTotal Power Points:\tn %d\r\n", MAX(1, add_psp));
  }
  /*
  if (GET_LEVEL(ch) > 1)
  {
    GET_REAL_MAX_PSP(ch) += add_psp;
    if (GET_PREMADE_BUILD_CLASS(ch) != CLASS_UNDEFINED)
      send_to_char(ch, "\tMTotal PSP:\tn %d\r\n", add_psp);
  }
  */
  GET_FEAT_POINTS(ch) += feats;
  if (feats)
    if (GET_PREMADE_BUILD_CLASS(ch) == CLASS_UNDEFINED)
      send_to_char(ch, "%d \tMFeat points gained.\tn\r\n", feats);
  GET_CLASS_FEATS(ch, class) += class_feats;
  if (class_feats)
    if (GET_PREMADE_BUILD_CLASS(ch) == CLASS_UNDEFINED)
      send_to_char(ch, "%d \tMClass feat points gained.\tn\r\n", class_feats);
  GET_EPIC_FEAT_POINTS(ch) += epic_feats;
  if (epic_feats)
    send_to_char(ch, "%d \tMEpic feat points gained.\tn\r\n", epic_feats);
  GET_EPIC_CLASS_FEATS(ch, class) += epic_class_feats;
  if (epic_class_feats)
    send_to_char(ch, "%d \tMEpic class feat points gained.\tn\r\n", epic_class_feats);
  GET_TRAINS(ch) += trains;
  if (GET_PREMADE_BUILD_CLASS(ch) == CLASS_UNDEFINED)
    send_to_char(ch, "%d \tMTraining sessions gained.\tn\r\n", trains);
  /*******/
  /* end advancement block */
  /*******/

  /**** reaffect ****/
  affect_total_plus(ch, at_armor);
  /* end reaffecting */

  /* give immorts some goodies */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    for (k = 0; k < 3; k++)
      GET_COND(ch, k) = (char)-1;
    SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    send_to_char(ch, "Setting \tRHOLYLIGHT\tn on.\r\n");
    SET_BIT_AR(PRF_FLAGS(ch), PRF_NOHASSLE);
    send_to_char(ch, "Setting \tRNOHASSLE\tn on.\r\n");
    SET_BIT_AR(PRF_FLAGS(ch), PRF_SHOWVNUMS);
    send_to_char(ch, "Setting \tRSHOWVNUMS\tn on.\r\n");
  }

  /* make sure you aren't snooping someone you shouldn't with new level */
  snoop_check(ch);
  save_char(ch, 1);
}

/* if you get multiplier for backstab, calculated here */
int backstab_mult(struct char_data *ch)
{
  int multiplier = 0;

  if (IS_ROGUE_TYPE(ch))
    multiplier += 2;

  if (HAS_FEAT(ch, FEAT_BACKSTAB))
    multiplier += 2;

  return multiplier;
}

// used by handler.c, completely depreacted function right now

int invalid_class(struct char_data *ch, struct obj_data *obj)
{
  return FALSE;
}

// vital min level info!

void init_spell_levels(void)
{
  int i = 0, j = 0, class = 0;
  struct class_spell_assign *spell_assign = NULL;

  // simple loop to init min-level 1 for all the SKILL_x to all classes
  for (i = (MAX_SPELLS + 1); i < TOP_SKILL_DEFINE; i++)
  {
    for (j = 0; j < NUM_CLASSES; j++)
    {
      spell_level(i, j, 1);
    }
  }

  for (class = CLASS_WIZARD; class < NUM_CLASSES; class ++)
  {
    if (class_list[class].spellassign_list != NULL)
    {
      /*  This class has spell assignment! Traverse the list and check. */
      for (spell_assign = class_list[class].spellassign_list; spell_assign != NULL;
           spell_assign = spell_assign->next)
      {
        spell_level(spell_assign->spell_num, class, spell_assign->level);
      }
    }
  }
}

// level_exp ran with level+1 will give xp to next level
// level_exp+1 - level_exp = exp to next level

int level_exp(struct char_data *ch, int level)
{
  int chclass = GET_CLASS(ch);
  int exp = 0, factor = 0;

  if (level > (LVL_IMPL + 1) || level < 0)
  {
    log("SYSERR: Requesting exp for invalid level %d!", level);
    return 0;
  }

  /* Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist. */
  if (level > LVL_IMMORT)
  {
    return EXP_MAX - ((LVL_IMPL - level) * 1000);
  }

  factor = 2000 + (level - 2) * 750;

  /* Exp required for normal mortals is below */
  switch (chclass)
  {
  case CLASS_WIZARD:
  case CLASS_SORCERER:
  case CLASS_PALADIN:
  case CLASS_BLACKGUARD:
  case CLASS_MONK:
  case CLASS_DRUID:
  case CLASS_RANGER:
  case CLASS_WARRIOR:
  case CLASS_WEAPON_MASTER:
  case CLASS_STALWART_DEFENDER:
  case CLASS_SHIFTER:
  case CLASS_DUELIST:
  case CLASS_ASSASSIN:
  case CLASS_SHADOW_DANCER:
  case CLASS_ARCANE_ARCHER:
  case CLASS_ARCANE_SHADOW:
  case CLASS_ELDRITCH_KNIGHT:
  case CLASS_SACRED_FIST:
  case CLASS_ROGUE:
  case CLASS_BARD:
  case CLASS_BERSERKER:
  case CLASS_CLERIC:
  case CLASS_MYSTIC_THEURGE:
  case CLASS_ALCHEMIST:
  case CLASS_SPELLSWORD:
  case CLASS_PSIONICIST:
  case CLASS_INQUISITOR:
    level--;
    if (level < 0)
      level = 0;
    exp += (level * level * factor);
    break;

  default:
    log("SYSERR: Reached invalid class in class.c level()!");
    return 123456;
  }

  // can add other exp penalty/bonuses here
  switch (GET_REAL_RACE(ch))
  {
    /* funny bug: used to use disguised/wildshape race */

#ifdef CAMPAIGN_FR
  case RACE_DROW:
    exp *= 2;
    break;
  case RACE_DUERGAR:
    exp *= 2;
    break;

  case RACE_FAE:
    exp *= 5;
    break;

  case RACE_LICH:
    exp *= 10;
    break;

  case RACE_VAMPIRE:
    exp *= 10;
    break;
#else
    // advanced races
  case RACE_HALF_TROLL:
    exp *= 2;
    break;
  case RACE_ARCANA_GOLEM:
    exp *= 2;
    break;
  case RACE_DROW:
    exp *= 2;
    break;
  case RACE_DUERGAR:
    exp *= 2;
    break;

    /* epic races */
  case RACE_CRYSTAL_DWARF:
    exp *= 7;
    break;

  case RACE_FAE:
    exp *= 7;
    break;

  case RACE_TRELUX:
    exp *= 7;
    break;

  case RACE_LICH:
    exp *= 10;
    break;

  case RACE_VAMPIRE:
    exp *= 10;
    break;
#endif
  default:
    break;
  }

#ifdef CAMPAIGN_FR
  // This is the final multiplier.  This will change all exp requirements across the board.
  // To keep it at the original -LuminariMUD based levels, comment out this line entirely
  exp *= 2;
#endif

  return exp;
}

/* papa function loaded on game boot to assign all the class data */
void load_class_list(void)
{
  /* initialize a FULL sized list with some default values for safety */
  int i = 0;
  for (i = 0; i < NUM_CLASSES; i++)
    init_class_list(i);

  /* here goes assignment, for sanity we arranged it the assignments accordingly:
   *  classo
   *  preferred saves
   *  class abilities
   *  class titles
   *  free feat assignment
   *  classfeat assignment
   *  class spell assignment (if necessary)
   *  prereqs   */

  /****************************************************************************/
  /*     class-number  name      abrv   clr-abrv     menu-name*/
  classo(CLASS_WIZARD, "wizard", "Wiz", "\tmWiz\tn", "m) \tmWizard\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCost efeatp*/
         -1, N, N, L, 6, 0, 1, 2, Y, 0, 5,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Intelligence, Con/Dex for survivability",
         /*Descrip*/ "Beyond the veil of the mundane hide the secrets of absolute "
                     "power. The works of beings beyond mortals, the legends of realms where titans "
                     "and spirits tread, the lore of creations both wondrous and terrible—such "
                     "mysteries call to those with the ambition and the intellect to rise above "
                     "the common folk to grasp true might. Such is the path of the wizard. These "
                     "shrewd magic-users seek, collect, and covet esoteric knowledge, drawing on "
                     "cultic arts to work wonders beyond the abilities of mere mortals. While some "
                     "might choose a particular field of magical study and become masters of such "
                     "powers, others embrace versatility, reveling in the unbounded wonders of all "
                     "magic. In either case, wizards prove a cunning and potent lot, capable of "
                     "smiting their foes, empowering their allies, and shaping the world to their "
                     "every desire.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_WIZARD, B, B, G, B, B);
  assign_class_abils(CLASS_WIZARD, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CA, CC, CC, CA, CA, CC, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CC, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_WIZARD,                  /* class number */
                      "",                            /* <= 4  */
                      "the Reader of Arcane Texts",  /* <= 9  */
                      "the Ever-Learning",           /* <= 14 */
                      "the Advanced Student",        /* <= 19 */
                      "the Channel of Power",        /* <= 24 */
                      "the Delver of Mysteries",     /* <= 29 */
                      "the Knower of Hidden Things", /* <= 30 */
                      "the Immortal Warlock",        /* <= LVL_IMMORT */
                      "the Avatar of Magic",         /* <= LVL_STAFF */
                      "the God of Magic",            /* <= LVL_GRSTAFF */
                      "the Wizard"                   /* default */
  );
  /* feat assignment */
  /*              class num     feat                            cfeat lvl stack */
  feat_assignment(CLASS_WIZARD, FEAT_WEAPON_PROFICIENCY_WIZARD, Y, 1, N);
  feat_assignment(CLASS_WIZARD, FEAT_SCRIBE_SCROLL, Y, 1, N);
  feat_assignment(CLASS_WIZARD, FEAT_SUMMON_FAMILIAR, Y, 1, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_1ST_CIRCLE, Y, 1, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_2ND_CIRCLE, Y, 3, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_3RD_CIRCLE, Y, 5, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_4TH_CIRCLE, Y, 7, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_5TH_CIRCLE, Y, 9, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZ_MEMORIZATION, Y, 10, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_6TH_CIRCLE, Y, 11, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_7TH_CIRCLE, Y, 13, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_8TH_CIRCLE, Y, 15, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_9TH_CIRCLE, Y, 17, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZ_CHANT, Y, 19, N);
  /*epic*/
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_EPIC_SPELL, Y, 21, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZ_DEBUFF, Y, 30, N);
  /* list of class feats */
  feat_assignment(CLASS_WIZARD, FEAT_COMBAT_CASTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_SPELL_PENETRATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_GREATER_SPELL_PENETRATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_ARMORED_SPELLCASTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_FASTER_MEMORIZATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_SPELL_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_GREATER_SPELL_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_IMPROVED_FAMILIAR, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_QUICK_CHANT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_AUGMENT_SUMMONING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_ENHANCED_SPELL_DAMAGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_MAXIMIZE_SPELL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_QUICKEN_SPELL, Y, NOASSIGN_FEAT, N);
  /* epic class */
  feat_assignment(CLASS_WIZARD, FEAT_MUMMY_DUST, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_GREATER_RUIN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_DRAGON_KNIGHT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_HELLBALL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_EPIC_MAGE_ARMOR, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_EPIC_WARDING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WIZARD, FEAT_GREAT_INTELLIGENCE, Y, NOASSIGN_FEAT, N);
  /**** spell assign ****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  spell_assignment(CLASS_WIZARD, SPELL_HORIZIKAULS_BOOM, 1);
  spell_assignment(CLASS_WIZARD, SPELL_MAGIC_MISSILE, 1);
  spell_assignment(CLASS_WIZARD, SPELL_BURNING_HANDS, 1);
  spell_assignment(CLASS_WIZARD, SPELL_ICE_DAGGER, 1);
  spell_assignment(CLASS_WIZARD, SPELL_MAGE_ARMOR, 1);
  spell_assignment(CLASS_WIZARD, SPELL_SUMMON_CREATURE_1, 1);
  spell_assignment(CLASS_WIZARD, SPELL_CHILL_TOUCH, 1);
  spell_assignment(CLASS_WIZARD, SPELL_NEGATIVE_ENERGY_RAY, 1);
  spell_assignment(CLASS_WIZARD, SPELL_RAY_OF_ENFEEBLEMENT, 1);
  spell_assignment(CLASS_WIZARD, SPELL_CHARM, 1);
  spell_assignment(CLASS_WIZARD, SPELL_ENCHANT_ITEM, 1);
  spell_assignment(CLASS_WIZARD, SPELL_SLEEP, 1);
  spell_assignment(CLASS_WIZARD, SPELL_COLOR_SPRAY, 1);
  spell_assignment(CLASS_WIZARD, SPELL_SCARE, 1);
  spell_assignment(CLASS_WIZARD, SPELL_TRUE_STRIKE, 1);
  spell_assignment(CLASS_WIZARD, SPELL_IDENTIFY, 1);
  spell_assignment(CLASS_WIZARD, SPELL_SHELGARNS_BLADE, 1);
  spell_assignment(CLASS_WIZARD, SPELL_GREASE, 1);
  spell_assignment(CLASS_WIZARD, SPELL_ENDURE_ELEMENTS, 1);
  spell_assignment(CLASS_WIZARD, SPELL_PROT_FROM_EVIL, 1);
  spell_assignment(CLASS_WIZARD, SPELL_PROT_FROM_GOOD, 1);
  spell_assignment(CLASS_WIZARD, SPELL_EXPEDITIOUS_RETREAT, 1);
  spell_assignment(CLASS_WIZARD, SPELL_IRON_GUTS, 1);
  spell_assignment(CLASS_WIZARD, SPELL_SHIELD, 1);
  spell_assignment(CLASS_WIZARD, SPELL_STUNNING_BARRIER, 1);
  spell_assignment(CLASS_WIZARD, SPELL_RESISTANCE, 1);
  spell_assignment(CLASS_WIZARD, SPELL_ANT_HAUL, 1);
  spell_assignment(CLASS_WIZARD, SPELL_CORROSIVE_TOUCH, 1);
  spell_assignment(CLASS_WIZARD, SPELL_PLANAR_HEALING, 1);
  spell_assignment(CLASS_WIZARD, SPELL_MOUNT, 1);
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_WIZARD, SPELL_SHOCKING_GRASP, 3);
  spell_assignment(CLASS_WIZARD, SPELL_SCORCHING_RAY, 3);
  spell_assignment(CLASS_WIZARD, SPELL_CONTINUAL_FLAME, 3);
  spell_assignment(CLASS_WIZARD, SPELL_SUMMON_CREATURE_2, 3);
  spell_assignment(CLASS_WIZARD, SPELL_WEB, 3);
  spell_assignment(CLASS_WIZARD, SPELL_ACID_ARROW, 3);
  spell_assignment(CLASS_WIZARD, SPELL_DANCING_WEAPON, 3);
  spell_assignment(CLASS_WIZARD, SPELL_BLINDNESS, 3);
  spell_assignment(CLASS_WIZARD, SPELL_DEAFNESS, 3);
  spell_assignment(CLASS_WIZARD, SPELL_FALSE_LIFE, 3);
  spell_assignment(CLASS_WIZARD, SPELL_DAZE_MONSTER, 3);
  spell_assignment(CLASS_WIZARD, SPELL_HIDEOUS_LAUGHTER, 3);
  spell_assignment(CLASS_WIZARD, SPELL_TOUCH_OF_IDIOCY, 3);
  spell_assignment(CLASS_WIZARD, SPELL_BLUR, 3);
  spell_assignment(CLASS_WIZARD, SPELL_MIRROR_IMAGE, 3);
  spell_assignment(CLASS_WIZARD, SPELL_INVISIBLE, 3);
  spell_assignment(CLASS_WIZARD, SPELL_DETECT_INVIS, 3);
  spell_assignment(CLASS_WIZARD, SPELL_DETECT_MAGIC, 3);
  spell_assignment(CLASS_WIZARD, SPELL_DARKNESS, 3);
  // spell_assignment(CLASS_WIZARD, SPELL_I_DARKNESS,        3);
  spell_assignment(CLASS_WIZARD, SPELL_RESIST_ENERGY, 3);
  spell_assignment(CLASS_WIZARD, SPELL_ENERGY_SPHERE, 3);
  spell_assignment(CLASS_WIZARD, SPELL_ENDURANCE, 3);
  spell_assignment(CLASS_WIZARD, SPELL_STRENGTH, 3);
  spell_assignment(CLASS_WIZARD, SPELL_GRACE, 3);
  spell_assignment(CLASS_WIZARD, SPELL_TACTICAL_ACUMEN, 3);
  spell_assignment(CLASS_WIZARD, SPELL_BESTOW_WEAPON_PROFICIENCY, 3);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_ANT_HAUL, 3);
  spell_assignment(CLASS_WIZARD, SPELL_CUSHIONING_BANDS, 3);
  spell_assignment(CLASS_WIZARD, SPELL_GIRD_ALLIES, 3);
  spell_assignment(CLASS_WIZARD, SPELL_GLITTERDUST, 3);
  spell_assignment(CLASS_WIZARD, SPELL_SPIDER_CLIMB, 3);
  spell_assignment(CLASS_WIZARD, SPELL_WARDING_WEAPON, 3);
  spell_assignment(CLASS_WIZARD, SPELL_PROTECTION_FROM_ARROWS, 3);
  spell_assignment(CLASS_WIZARD, SPELL_COMMUNAL_MOUNT, 3);
  spell_assignment(CLASS_WIZARD, SPELL_HUMAN_POTENTIAL, 3);
  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_WIZARD, SPELL_LIGHTNING_BOLT, 5);
  spell_assignment(CLASS_WIZARD, SPELL_FIREBALL, 5);
  spell_assignment(CLASS_WIZARD, SPELL_WATER_BREATHE, 5);
  spell_assignment(CLASS_WIZARD, SPELL_SUMMON_CREATURE_3, 5);
  spell_assignment(CLASS_WIZARD, SPELL_PHANTOM_STEED, 5);
  spell_assignment(CLASS_WIZARD, SPELL_STINKING_CLOUD, 5);
  spell_assignment(CLASS_WIZARD, SPELL_HALT_UNDEAD, 5);
  spell_assignment(CLASS_WIZARD, SPELL_VAMPIRIC_TOUCH, 5);
  spell_assignment(CLASS_WIZARD, SPELL_HEROISM, 5);
  spell_assignment(CLASS_WIZARD, SPELL_FLY, 5);
  spell_assignment(CLASS_WIZARD, SPELL_HOLD_PERSON, 5);
  spell_assignment(CLASS_WIZARD, SPELL_DEEP_SLUMBER, 5);
  spell_assignment(CLASS_WIZARD, SPELL_WALL_OF_FOG, 5);
  spell_assignment(CLASS_WIZARD, SPELL_INVISIBILITY_SPHERE, 5);
  spell_assignment(CLASS_WIZARD, SPELL_DAYLIGHT, 5);
  spell_assignment(CLASS_WIZARD, SPELL_CLAIRVOYANCE, 5);
  spell_assignment(CLASS_WIZARD, SPELL_NON_DETECTION, 5);
  spell_assignment(CLASS_WIZARD, SPELL_DISPEL_MAGIC, 5);
  spell_assignment(CLASS_WIZARD, SPELL_HASTE, 5);
  spell_assignment(CLASS_WIZARD, SPELL_SLOW, 5);
  spell_assignment(CLASS_WIZARD, SPELL_CIRCLE_A_EVIL, 5);
  spell_assignment(CLASS_WIZARD, SPELL_CIRCLE_A_GOOD, 5);
  spell_assignment(CLASS_WIZARD, SPELL_CUNNING, 5);
  spell_assignment(CLASS_WIZARD, SPELL_WISDOM, 5);
  spell_assignment(CLASS_WIZARD, SPELL_CHARISMA, 5);
  spell_assignment(CLASS_WIZARD, SPELL_GREATER_MAGIC_WEAPON, 5);
  spell_assignment(CLASS_WIZARD, SPELL_WEAPON_OF_IMPACT, 5);
  spell_assignment(CLASS_WIZARD, SPELL_KEEN_EDGE, 5);
  spell_assignment(CLASS_WIZARD, SPELL_PROTECTION_FROM_ENERGY, 5);
  spell_assignment(CLASS_WIZARD, SPELL_WIND_WALL, 5);
  spell_assignment(CLASS_WIZARD, SPELL_GASEOUS_FORM, 5);
  spell_assignment(CLASS_WIZARD, SPELL_AQUEOUS_ORB, 5);
  spell_assignment(CLASS_WIZARD, SPELL_RAGE, 5);
  spell_assignment(CLASS_WIZARD, SPELL_SIPHON_MIGHT, 5);
  spell_assignment(CLASS_WIZARD, SPELL_CONTROL_SUMMONED_CREATURE, 5);
  spell_assignment(CLASS_WIZARD, SPELL_COMMUNAL_PROTECTION_FROM_ARROWS, 5);
  spell_assignment(CLASS_WIZARD, SPELL_COMMUNAL_RESIST_ENERGY, 5);
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_WIZARD, SPELL_FIRE_SHIELD, 7);
  spell_assignment(CLASS_WIZARD, SPELL_LESSER_MISSILE_STORM, 7);
  spell_assignment(CLASS_WIZARD, SPELL_COLD_SHIELD, 7);
  spell_assignment(CLASS_WIZARD, SPELL_ICE_STORM, 7);
  spell_assignment(CLASS_WIZARD, SPELL_BILLOWING_CLOUD, 7);
  spell_assignment(CLASS_WIZARD, SPELL_SUMMON_CREATURE_4, 7);
  spell_assignment(CLASS_WIZARD, SPELL_ANIMATE_DEAD, 7);
  spell_assignment(CLASS_WIZARD, SPELL_CURSE, 7);
  spell_assignment(CLASS_WIZARD, SPELL_INFRAVISION, 7);
  spell_assignment(CLASS_WIZARD, SPELL_POISON, 7);
  spell_assignment(CLASS_WIZARD, SPELL_GREATER_INVIS, 7);
  spell_assignment(CLASS_WIZARD, SPELL_RAINBOW_PATTERN, 7);
  spell_assignment(CLASS_WIZARD, SPELL_WIZARD_EYE, 7);
  spell_assignment(CLASS_WIZARD, SPELL_LOCATE_CREATURE, 7);
  spell_assignment(CLASS_WIZARD, SPELL_MINOR_GLOBE, 7);
  spell_assignment(CLASS_WIZARD, SPELL_REMOVE_CURSE, 7);
  spell_assignment(CLASS_WIZARD, SPELL_STONESKIN, 7);
  spell_assignment(CLASS_WIZARD, SPELL_ENLARGE_PERSON, 7);
  spell_assignment(CLASS_WIZARD, SPELL_SHRINK_PERSON, 7);
  spell_assignment(CLASS_WIZARD, SPELL_CONFUSION, 7);
  spell_assignment(CLASS_WIZARD, SPELL_FEAR, 7);
  spell_assignment(CLASS_WIZARD, SPELL_SHADOW_JUMP, 7);
  spell_assignment(CLASS_WIZARD, SPELL_COMMUNAL_PROTECTION_FROM_ENERGY, 7);
  spell_assignment(CLASS_WIZARD, SPELL_GHOST_WOLF, 7);
  spell_assignment(CLASS_WIZARD, SPELL_BLACK_TENTACLES, 7);
  spell_assignment(CLASS_WIZARD, SPELL_CHARM_MONSTER, 7);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_ENLARGE_PERSON, 7);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_REDUCE_PERSON, 7);
  /*              class num      spell                   level acquired */
  /* 5th circle */
  spell_assignment(CLASS_WIZARD, SPELL_INTERPOSING_HAND, 9);
  spell_assignment(CLASS_WIZARD, SPELL_WALL_OF_FORCE, 9);
  spell_assignment(CLASS_WIZARD, SPELL_BALL_OF_LIGHTNING, 9);
  spell_assignment(CLASS_WIZARD, SPELL_CLOUDKILL, 9);
  spell_assignment(CLASS_WIZARD, SPELL_SUMMON_CREATURE_5, 9);
  spell_assignment(CLASS_WIZARD, SPELL_WAVES_OF_FATIGUE, 9);
  spell_assignment(CLASS_WIZARD, SPELL_SYMBOL_OF_PAIN, 9);
  spell_assignment(CLASS_WIZARD, SPELL_DOMINATE_PERSON, 9);
  spell_assignment(CLASS_WIZARD, SPELL_FEEBLEMIND, 9);
  spell_assignment(CLASS_WIZARD, SPELL_NIGHTMARE, 9);
  spell_assignment(CLASS_WIZARD, SPELL_MIND_FOG, 9);
  spell_assignment(CLASS_WIZARD, SPELL_ACID_SHEATH, 9);
  spell_assignment(CLASS_WIZARD, SPELL_FAITHFUL_HOUND, 9);
  spell_assignment(CLASS_WIZARD, SPELL_DISMISSAL, 9);
  spell_assignment(CLASS_WIZARD, SPELL_CONE_OF_COLD, 9);
  spell_assignment(CLASS_WIZARD, SPELL_TELEKINESIS, 9);
  spell_assignment(CLASS_WIZARD, SPELL_FIREBRAND, 9);
  /*              class num      spell                   level acquired */
  /* 6th circle */
  spell_assignment(CLASS_WIZARD, SPELL_FREEZING_SPHERE, 11);
  spell_assignment(CLASS_WIZARD, SPELL_ACID_FOG, 11);
  spell_assignment(CLASS_WIZARD, SPELL_SUMMON_CREATURE_6, 11);
  spell_assignment(CLASS_WIZARD, SPELL_TRANSFORMATION, 11);
  spell_assignment(CLASS_WIZARD, SPELL_EYEBITE, 11);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_HASTE, 11);
  spell_assignment(CLASS_WIZARD, SPELL_GREATER_HEROISM, 11);
  spell_assignment(CLASS_WIZARD, SPELL_ANTI_MAGIC_FIELD, 11);
  spell_assignment(CLASS_WIZARD, SPELL_GREATER_MIRROR_IMAGE, 11);
  spell_assignment(CLASS_WIZARD, SPELL_LOCATE_OBJECT, 11);
  spell_assignment(CLASS_WIZARD, SPELL_TRUE_SEEING, 11);
  spell_assignment(CLASS_WIZARD, SPELL_GLOBE_OF_INVULN, 11);
  spell_assignment(CLASS_WIZARD, SPELL_GREATER_DISPELLING, 11);
  spell_assignment(CLASS_WIZARD, SPELL_CLONE, 11);
  spell_assignment(CLASS_WIZARD, SPELL_WATERWALK, 11);
  spell_assignment(CLASS_WIZARD, SPELL_LEVITATE, 11);
  spell_assignment(CLASS_WIZARD, SPELL_SHADOW_WALK, 11);
  spell_assignment(CLASS_WIZARD, SPELL_CIRCLE_OF_DEATH, 11);
  spell_assignment(CLASS_WIZARD, SPELL_UNDEATH_TO_DEATH, 11);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_HUMAN_POTENTIAL, 11);
  spell_assignment(CLASS_WIZARD, SPELL_GREATER_BLACK_TENTACLES, 11);
  /*              class num      spell                   level acquired */
  /* 7th circle */
  spell_assignment(CLASS_WIZARD, SPELL_MISSILE_STORM, 13);
  spell_assignment(CLASS_WIZARD, SPELL_GRASPING_HAND, 13);
  spell_assignment(CLASS_WIZARD, SPELL_SUMMON_CREATURE_7, 13);
  spell_assignment(CLASS_WIZARD, SPELL_CONTROL_WEATHER, 13);
  spell_assignment(CLASS_WIZARD, SPELL_POWER_WORD_BLIND, 13);
  spell_assignment(CLASS_WIZARD, SPELL_WAVES_OF_EXHAUSTION, 13);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_HOLD_PERSON, 13);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_FLY, 13);
  spell_assignment(CLASS_WIZARD, SPELL_DISPLACEMENT, 13);
  spell_assignment(CLASS_WIZARD, SPELL_PRISMATIC_SPRAY, 13);
  spell_assignment(CLASS_WIZARD, SPELL_DETECT_POISON, 13);
  spell_assignment(CLASS_WIZARD, SPELL_POWER_WORD_STUN, 13);
  spell_assignment(CLASS_WIZARD, SPELL_PROTECT_FROM_SPELLS, 13);
  spell_assignment(CLASS_WIZARD, SPELL_THUNDERCLAP, 13);
  spell_assignment(CLASS_WIZARD, SPELL_SPELL_MANTLE, 13);
  spell_assignment(CLASS_WIZARD, SPELL_TELEPORT, 13);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_WISDOM, 13);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_CHARISMA, 13);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_CUNNING, 13);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_FALSE_LIFE, 13);
  spell_assignment(CLASS_WIZARD, SPELL_FINGER_OF_DEATH, 13);
  /*              class num      spell                   level acquired */
  /* 8th circle */
  spell_assignment(CLASS_WIZARD, SPELL_CLENCHED_FIST, 15);
  spell_assignment(CLASS_WIZARD, SPELL_CHAIN_LIGHTNING, 15);
  spell_assignment(CLASS_WIZARD, SPELL_INCENDIARY_CLOUD, 15);
  spell_assignment(CLASS_WIZARD, SPELL_SUMMON_CREATURE_8, 15);
  spell_assignment(CLASS_WIZARD, SPELL_HORRID_WILTING, 15);
  spell_assignment(CLASS_WIZARD, SPELL_GREATER_ANIMATION, 15);
  spell_assignment(CLASS_WIZARD, SPELL_IRRESISTIBLE_DANCE, 15);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_DOMINATION, 15);
  spell_assignment(CLASS_WIZARD, SPELL_SCINT_PATTERN, 15);
  spell_assignment(CLASS_WIZARD, SPELL_REFUGE, 15);
  spell_assignment(CLASS_WIZARD, SPELL_BANISH, 15);
  spell_assignment(CLASS_WIZARD, SPELL_SUNBURST, 15);
  spell_assignment(CLASS_WIZARD, SPELL_SPELL_TURNING, 15);
  spell_assignment(CLASS_WIZARD, SPELL_MIND_BLANK, 15);
  spell_assignment(CLASS_WIZARD, SPELL_IRONSKIN, 15);
  spell_assignment(CLASS_WIZARD, SPELL_PORTAL, 15);
  /*              class num      spell                   level acquired */
  /* 9th circle */
  spell_assignment(CLASS_WIZARD, SPELL_METEOR_SWARM, 17);
  spell_assignment(CLASS_WIZARD, SPELL_BLADE_OF_DISASTER, 17);
  spell_assignment(CLASS_WIZARD, SPELL_SUMMON_CREATURE_9, 17);
#ifndef CAMPAIGN_FR
  spell_assignment(CLASS_WIZARD, SPELL_GATE, 17);
#endif
  spell_assignment(CLASS_WIZARD, SPELL_ENERGY_DRAIN, 17);
  spell_assignment(CLASS_WIZARD, SPELL_WAIL_OF_THE_BANSHEE, 17);
  spell_assignment(CLASS_WIZARD, SPELL_POWER_WORD_KILL, 17);
  spell_assignment(CLASS_WIZARD, SPELL_ENFEEBLEMENT, 17);
  spell_assignment(CLASS_WIZARD, SPELL_WEIRD, 17);
  spell_assignment(CLASS_WIZARD, SPELL_SHADOW_SHIELD, 17);
  spell_assignment(CLASS_WIZARD, SPELL_PRISMATIC_SPHERE, 17);
  spell_assignment(CLASS_WIZARD, SPELL_IMPLODE, 17);
  spell_assignment(CLASS_WIZARD, SPELL_TIMESTOP, 17);
  spell_assignment(CLASS_WIZARD, SPELL_GREATER_SPELL_MANTLE, 17);
  spell_assignment(CLASS_WIZARD, SPELL_POLYMORPH, 17);
  spell_assignment(CLASS_WIZARD, SPELL_MASS_ENHANCE, 17);
  /*epic*/
  spell_assignment(CLASS_WIZARD, SPELL_MUMMY_DUST, 21);
  spell_assignment(CLASS_WIZARD, SPELL_DRAGON_KNIGHT, 21);
  spell_assignment(CLASS_WIZARD, SPELL_GREATER_RUIN, 21);
  spell_assignment(CLASS_WIZARD, SPELL_HELLBALL, 21);
  spell_assignment(CLASS_WIZARD, SPELL_EPIC_MAGE_ARMOR, 21);
  spell_assignment(CLASS_WIZARD, SPELL_EPIC_WARDING, 21);
  /* no prereqs!  woo! */
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  // assign_feat_spell_slots(CLASS_WIZARD);
  /**/
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name      abrv   clr-abrv     menu-name*/
  classo(CLASS_CLERIC, "cleric", "Cle", "\tBCle\tn", "c) \tBCleric\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst eFeatp*/
         -1, N, N, M, 8, 0, 1, 2, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Wisdom, Cha affects some of their abilities..  Con for survivability, Str for combat",
         /*descrip*/ "In faith and the miracles of the divine, many find a greater "
                     "purpose. Called to serve powers beyond most mortal understanding, all priests "
                     "preach wonders and provide for the spiritual needs of their people. Clerics "
                     "are more than mere priests, though; these emissaries of the divine work the "
                     "will of the greater powers through strength of arms and the magic of their "
                     "divine channels. Devoted to the tenets of the religions and philosophies that "
                     "inspire them, these ecclesiastics quest to spread the knowledge and influence "
                     "of their faith. Yet while they might share similar abilities, clerics prove as "
                     "different from one another as the powers they serve, with some offering healing "
                     "and redemption, others judging law and truth, and still others spreading "
                     "conflict and corruption. The ways of the cleric are varied, yet all who tread "
                     "these paths walk with the mightiest of allies and bear the arms of the divine "
                     "themselves.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_CLERIC, G, B, G, B, B);
  assign_class_abils(CLASS_CLERIC, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CA, CC, CA, CA, CA, CC, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CC, CC, CC, CC, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CC, CC);
  assign_class_titles(CLASS_CLERIC,                  /* class number */
                      "",                            /* <= 4  */
                      "the Devotee",                 /* <= 9  */
                      "the Example",                 /* <= 14 */
                      "the Truly Pious",             /* <= 19 */
                      "the Mighty in Faith",         /* <= 24 */
                      "the God-Favored",             /* <= 29 */
                      "the One Who Moves Mountains", /* <= 30 */
                      "the Immortal Cardinal",       /* <= LVL_IMMORT */
                      "the Inquisitor",              /* <= LVL_STAFF */
                      "the God of Good and Evil",    /* <= LVL_GRSTAFF */
                      "the Cleric"                   /* default */
  );
  /* feat assignment */
  /*              class num     feat                            cfeat lvl stack */
  feat_assignment(CLASS_CLERIC, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_CLERIC, FEAT_ARMOR_PROFICIENCY_HEAVY, Y, 1, N);
  feat_assignment(CLASS_CLERIC, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_CLERIC, FEAT_ARMOR_PROFICIENCY_MEDIUM, Y, 1, N);
  feat_assignment(CLASS_CLERIC, FEAT_ARMOR_PROFICIENCY_SHIELD, Y, 1, N);
  feat_assignment(CLASS_CLERIC, FEAT_CHANNEL_ENERGY, Y, 1, N);
  feat_assignment(CLASS_CLERIC, FEAT_CLERIC_1ST_CIRCLE, Y, 1, N);
  feat_assignment(CLASS_CLERIC, FEAT_CLERIC_2ND_CIRCLE, Y, 3, N);
  feat_assignment(CLASS_CLERIC, FEAT_CLERIC_3RD_CIRCLE, Y, 5, N);
  feat_assignment(CLASS_CLERIC, FEAT_CLERIC_4TH_CIRCLE, Y, 7, N);
  feat_assignment(CLASS_CLERIC, FEAT_CLERIC_5TH_CIRCLE, Y, 9, N);
  feat_assignment(CLASS_CLERIC, FEAT_CLERIC_6TH_CIRCLE, Y, 11, N);
  feat_assignment(CLASS_CLERIC, FEAT_CLERIC_7TH_CIRCLE, Y, 13, N);
  feat_assignment(CLASS_CLERIC, FEAT_CLERIC_8TH_CIRCLE, Y, 15, N);
  feat_assignment(CLASS_CLERIC, FEAT_CLERIC_9TH_CIRCLE, Y, 17, N);
  /*epic*/
  feat_assignment(CLASS_CLERIC, FEAT_CLERIC_EPIC_SPELL, Y, 21, N);
  /**** spell assign ****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  spell_assignment(CLASS_CLERIC, SPELL_SHIELD_OF_FAITH, 1);
  spell_assignment(CLASS_CLERIC, SPELL_CURE_LIGHT, 1);
  spell_assignment(CLASS_CLERIC, SPELL_ENDURANCE, 1);
  spell_assignment(CLASS_CLERIC, SPELL_CAUSE_LIGHT_WOUNDS, 1);
  spell_assignment(CLASS_CLERIC, SPELL_NEGATIVE_ENERGY_RAY, 1);
  spell_assignment(CLASS_CLERIC, SPELL_ENDURE_ELEMENTS, 1);
  spell_assignment(CLASS_CLERIC, SPELL_PROT_FROM_GOOD, 1);
  spell_assignment(CLASS_CLERIC, SPELL_PROT_FROM_EVIL, 1);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON_CREATURE_1, 1);
  spell_assignment(CLASS_CLERIC, SPELL_STRENGTH, 1);
  spell_assignment(CLASS_CLERIC, SPELL_GRACE, 1);
  spell_assignment(CLASS_CLERIC, SPELL_REMOVE_FEAR, 1);
  spell_assignment(CLASS_CLERIC, SPELL_DOOM, 1);
  spell_assignment(CLASS_CLERIC, SPELL_DIVINE_FAVOR, 1);
  spell_assignment(CLASS_CLERIC, SPELL_SUN_METAL, 1);
  spell_assignment(CLASS_CLERIC, SPELL_STUNNING_BARRIER, 1);
  spell_assignment(CLASS_CLERIC, SPELL_HEDGING_WEAPONS, 1);
  spell_assignment(CLASS_CLERIC, SPELL_EFFORTLESS_ARMOR, 1);
  spell_assignment(CLASS_CLERIC, SPELL_ANT_HAUL, 1);
  spell_assignment(CLASS_CLERIC, SPELL_PLANAR_HEALING, 1);
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_CLERIC, SPELL_AUGURY, 3);
  spell_assignment(CLASS_CLERIC, SPELL_UNDETECTABLE_ALIGNMENT, 3);
  spell_assignment(CLASS_CLERIC, SPELL_SPIRITUAL_WEAPON, 3);
  spell_assignment(CLASS_CLERIC, SPELL_REMOVE_PARALYSIS, 3);
  spell_assignment(CLASS_CLERIC, SPELL_CREATE_FOOD, 3);
  spell_assignment(CLASS_CLERIC, SPELL_CREATE_WATER, 3);
  spell_assignment(CLASS_CLERIC, SPELL_DETECT_POISON, 3);
  spell_assignment(CLASS_CLERIC, SPELL_CAUSE_MODERATE_WOUNDS, 3);
  spell_assignment(CLASS_CLERIC, SPELL_CURE_MODERATE, 3);
  spell_assignment(CLASS_CLERIC, SPELL_SCARE, 3);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON_CREATURE_2, 3);
  spell_assignment(CLASS_CLERIC, SPELL_DETECT_MAGIC, 3);
  spell_assignment(CLASS_CLERIC, SPELL_DARKNESS, 3);
  spell_assignment(CLASS_CLERIC, SPELL_RESIST_ENERGY, 3);
  spell_assignment(CLASS_CLERIC, SPELL_WISDOM, 3);
  spell_assignment(CLASS_CLERIC, SPELL_CHARISMA, 3);
  spell_assignment(CLASS_CLERIC, SPELL_BESTOW_WEAPON_PROFICIENCY, 3);
  spell_assignment(CLASS_CLERIC, SPELL_SHIELD_OF_FORTIFICATION, 3);
  spell_assignment(CLASS_CLERIC, SPELL_LESSER_RESTORATION, 3);
  spell_assignment(CLASS_CLERIC, SPELL_WEAPON_OF_AWE, 3);
  spell_assignment(CLASS_CLERIC, SPELL_BLINDING_RAY, 3);
  spell_assignment(CLASS_CLERIC, SPELL_SILENCE, 3);
  spell_assignment(CLASS_CLERIC, SPELL_VIGORIZE_LIGHT, 3);
  spell_assignment(CLASS_CLERIC, SPELL_MASS_ANT_HAUL, 3);
  spell_assignment(CLASS_CLERIC, SPELL_GIRD_ALLIES, 3);
  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_CLERIC, SPELL_BLESS, 5);
  spell_assignment(CLASS_CLERIC, SPELL_CURE_BLIND, 5);
  spell_assignment(CLASS_CLERIC, SPELL_DETECT_ALIGN, 5);
  spell_assignment(CLASS_CLERIC, SPELL_CAUSE_SERIOUS_WOUNDS, 5);
  spell_assignment(CLASS_CLERIC, SPELL_CURE_SERIOUS, 5);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON_CREATURE_3, 5);
  spell_assignment(CLASS_CLERIC, SPELL_BLINDNESS, 5);
  spell_assignment(CLASS_CLERIC, SPELL_DEAFNESS, 5);
  spell_assignment(CLASS_CLERIC, SPELL_CURE_DEAFNESS, 5);
  spell_assignment(CLASS_CLERIC, SPELL_CUNNING, 5);
  spell_assignment(CLASS_CLERIC, SPELL_DISPEL_MAGIC, 5);
  spell_assignment(CLASS_CLERIC, SPELL_ANIMATE_DEAD, 5);
  spell_assignment(CLASS_CLERIC, SPELL_FAERIE_FOG, 5);
  spell_assignment(CLASS_CLERIC, SPELL_MAGIC_VESTMENT, 5);
  spell_assignment(CLASS_CLERIC, SPELL_LIFE_SHIELD, 5);
  spell_assignment(CLASS_CLERIC, SPELL_HOLY_JAVELIN, 5);
  spell_assignment(CLASS_CLERIC, SPELL_INVISIBILITY_PURGE, 5);
  spell_assignment(CLASS_CLERIC, SPELL_WEAPON_OF_IMPACT, 5);
  spell_assignment(CLASS_CLERIC, SPELL_PROTECTION_FROM_ENERGY, 5);
  spell_assignment(CLASS_CLERIC, SPELL_SEARING_LIGHT, 5);
  spell_assignment(CLASS_CLERIC, SPELL_WIND_WALL, 5);
  spell_assignment(CLASS_CLERIC, SPELL_CONTROL_SUMMONED_CREATURE, 5);
  spell_assignment(CLASS_CLERIC, SPELL_COMMUNAL_RESIST_ENERGY, 5);
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_CLERIC, SPELL_CURE_CRITIC, 7);
  spell_assignment(CLASS_CLERIC, SPELL_REMOVE_CURSE, 7);
  spell_assignment(CLASS_CLERIC, SPELL_DISPEL_INVIS, 7);
  spell_assignment(CLASS_CLERIC, SPELL_INFRAVISION, 7);
  spell_assignment(CLASS_CLERIC, SPELL_CAUSE_CRITICAL_WOUNDS, 7);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON_CREATURE_4, 7);
  spell_assignment(CLASS_CLERIC, SPELL_CIRCLE_A_EVIL, 7);
  spell_assignment(CLASS_CLERIC, SPELL_CIRCLE_A_GOOD, 7);
  spell_assignment(CLASS_CLERIC, SPELL_CURSE, 7);
  spell_assignment(CLASS_CLERIC, SPELL_DAYLIGHT, 7);
  spell_assignment(CLASS_CLERIC, SPELL_MASS_CURE_LIGHT, 7);
  spell_assignment(CLASS_CLERIC, SPELL_AID, 7);
  spell_assignment(CLASS_CLERIC, SPELL_BRAVERY, 7);
  spell_assignment(CLASS_CLERIC, SPELL_GREATER_MAGIC_WEAPON, 7);
  spell_assignment(CLASS_CLERIC, SPELL_COMMUNAL_PROTECTION_FROM_ENERGY, 7);
  spell_assignment(CLASS_CLERIC, SPELL_DIVINE_POWER, 7);
  spell_assignment(CLASS_CLERIC, SPELL_AIR_WALK, 7);
  spell_assignment(CLASS_CLERIC, SPELL_VIGORIZE_SERIOUS, 7);
  /*              class num      spell                   level acquired */
  /* 5th circle */
  spell_assignment(CLASS_CLERIC, SPELL_POISON, 9);
  spell_assignment(CLASS_CLERIC, SPELL_REMOVE_POISON, 9);
  spell_assignment(CLASS_CLERIC, SPELL_PROT_FROM_EVIL, 9);
  spell_assignment(CLASS_CLERIC, SPELL_GROUP_SHIELD_OF_FAITH, 9);
  spell_assignment(CLASS_CLERIC, SPELL_FLAME_STRIKE, 9);
  spell_assignment(CLASS_CLERIC, SPELL_PROT_FROM_GOOD, 9);
  spell_assignment(CLASS_CLERIC, SPELL_MASS_CURE_MODERATE, 9);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON_CREATURE_5, 9);
  spell_assignment(CLASS_CLERIC, SPELL_WATER_BREATHE, 9);
  spell_assignment(CLASS_CLERIC, SPELL_WATERWALK, 9);
  spell_assignment(CLASS_CLERIC, SPELL_REGENERATION, 9);
  spell_assignment(CLASS_CLERIC, SPELL_FREE_MOVEMENT, 9);
  spell_assignment(CLASS_CLERIC, SPELL_STRENGTHEN_BONE, 9);
  spell_assignment(CLASS_CLERIC, SPELL_FEAR, 9);
  spell_assignment(CLASS_CLERIC, SPELL_VIGORIZE_CRITICAL, 9);
  /*              class num      spell                   level acquired */
  /* 6th circle */
  spell_assignment(CLASS_CLERIC, SPELL_DISPEL_EVIL, 11);
  spell_assignment(CLASS_CLERIC, SPELL_HARM, 11);
  spell_assignment(CLASS_CLERIC, SPELL_HEAL, 11);
  spell_assignment(CLASS_CLERIC, SPELL_DISPEL_GOOD, 11);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON_CREATURE_6, 11);
  spell_assignment(CLASS_CLERIC, SPELL_MASS_CURE_SERIOUS, 11);
  spell_assignment(CLASS_CLERIC, SPELL_EYEBITE, 11);
  spell_assignment(CLASS_CLERIC, SPELL_PRAYER, 11);
  spell_assignment(CLASS_CLERIC, SPELL_MASS_WISDOM, 11);
  spell_assignment(CLASS_CLERIC, SPELL_MASS_CHARISMA, 11);
  spell_assignment(CLASS_CLERIC, SPELL_MASS_CUNNING, 11);
  spell_assignment(CLASS_CLERIC, SPELL_REMOVE_DISEASE, 11);
  spell_assignment(CLASS_CLERIC, SPELL_LEVITATE, 11);
  spell_assignment(CLASS_CLERIC, SPELL_UNDEATH_TO_DEATH, 11);
  /*              class num      spell                   level acquired */
  /* 7th circle */
  spell_assignment(CLASS_CLERIC, SPELL_CALL_LIGHTNING, 13);
  // spell_assignment(CLASS_CLERIC, SPELL_CONTROL_WEATHER,    13);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON, 13);
  spell_assignment(CLASS_CLERIC, SPELL_WORD_OF_RECALL, 13);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON_CREATURE_7, 13);
  spell_assignment(CLASS_CLERIC, SPELL_MASS_CURE_CRIT, 13);
  spell_assignment(CLASS_CLERIC, SPELL_GREATER_DISPELLING, 13);
  spell_assignment(CLASS_CLERIC, SPELL_MASS_ENHANCE, 13);
  spell_assignment(CLASS_CLERIC, SPELL_BLADE_BARRIER, 13);
  spell_assignment(CLASS_CLERIC, SPELL_BATTLETIDE, 13);
  spell_assignment(CLASS_CLERIC, SPELL_SPELL_RESISTANCE, 13);
  spell_assignment(CLASS_CLERIC, SPELL_SENSE_LIFE, 13);
  spell_assignment(CLASS_CLERIC, SPELL_RESTORATION, 13);
  /*              class num      spell                   level acquired */
  /* 8th circle */
  // spell_assignment(CLASS_CLERIC, SPELL_SANCTUARY,       15);
  spell_assignment(CLASS_CLERIC, SPELL_DESTRUCTION, 15);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON_CREATURE_8, 15);
  spell_assignment(CLASS_CLERIC, SPELL_SPELL_MANTLE, 15);
  spell_assignment(CLASS_CLERIC, SPELL_TRUE_SEEING, 15);
  spell_assignment(CLASS_CLERIC, SPELL_WORD_OF_FAITH, 15);
  spell_assignment(CLASS_CLERIC, SPELL_GREATER_ANIMATION, 15);
  spell_assignment(CLASS_CLERIC, SPELL_EARTHQUAKE, 15);
  spell_assignment(CLASS_CLERIC, SPELL_ANTI_MAGIC_FIELD, 15);
  spell_assignment(CLASS_CLERIC, SPELL_DIMENSIONAL_LOCK, 15);
  spell_assignment(CLASS_CLERIC, SPELL_SALVATION, 15);
  spell_assignment(CLASS_CLERIC, SPELL_SPRING_OF_LIFE, 15);
  spell_assignment(CLASS_CLERIC, SPELL_RESURRECT, 15);

  /*              class num      spell                   level acquired */
  /* 9th circle */
  spell_assignment(CLASS_CLERIC, SPELL_SUNBURST, 17);
  spell_assignment(CLASS_CLERIC, SPELL_ENERGY_DRAIN, 17);
  spell_assignment(CLASS_CLERIC, SPELL_GROUP_HEAL, 17);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON_CREATURE_9, 17);
  spell_assignment(CLASS_CLERIC, SPELL_PLANE_SHIFT, 17);
  spell_assignment(CLASS_CLERIC, SPELL_STORM_OF_VENGEANCE, 17);
  spell_assignment(CLASS_CLERIC, SPELL_IMPLODE, 17);
  spell_assignment(CLASS_CLERIC, SPELL_REFUGE, 17);
  spell_assignment(CLASS_CLERIC, SPELL_GROUP_SUMMON, 17);
  // spell_assignment(CLASS_CLERIC, death shield, 17);
  // spell_assignment(CLASS_CLERIC, command, 17);
  // spell_assignment(CLASS_CLERIC, air walker, 17);
  /*epic spells*/
  spell_assignment(CLASS_CLERIC, SPELL_MUMMY_DUST, 21);
  spell_assignment(CLASS_CLERIC, SPELL_DRAGON_KNIGHT, 21);
  spell_assignment(CLASS_CLERIC, SPELL_GREATER_RUIN, 21);
  spell_assignment(CLASS_CLERIC, SPELL_HELLBALL, 21);
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  // assign_feat_spell_slots(CLASS_CLERIC);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name     abrv   clr-abrv     menu-name*/
  classo(CLASS_ROGUE, "rogue", "Rog", "\twRog\tn", "t) \twRogue\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst eFeatp*/
         -1, N, N, H, 8, 0, 2, 8, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Dexterity, Con for survivability, Int for skills, Str for combat",
         /*descrip*/ "Life is an endless adventure for those who live by their wits. "
                     "Ever just one step ahead of danger, rogues bank on their cunning, skill, and "
                     "charm to bend fate to their favor. Never knowing what to expect, they prepare "
                     "for everything, becoming masters of a wide variety of skills, training "
                     "themselves to be adept manipulators, agile acrobats, shadowy stalkers, or "
                     "masters of any of dozens of other professions or talents. Thieves and gamblers, "
                     "fast talkers and diplomats, bandits and bounty hunters, and explorers and "
                     "investigators all might be considered rogues, as well as countless other "
                     "professions that rely upon wits, prowess, or luck. Although many rogues favor "
                     "cities and the innumerable opportunities of civilization, some embrace lives "
                     "on the road, journeying far, meeting exotic people, and facing fantastic "
                     "danger in pursuit of equally fantastic riches. In the end, any who desire to "
                     "shape their fates and live life on their own terms might come to be called "
                     "rogues.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_ROGUE, B, G, B, B, B);
  assign_class_abils(CLASS_ROGUE, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CA, CA, CA, CA, CC, CC, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CA, CC, CA, CA, CA, CA, CA, CA,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CA, CA, CA, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_ROGUE,                        /* class number */
                      "",                                 /* <= 4  */
                      "the Rover",                        /* <= 9  */
                      "the Multifarious",                 /* <= 14 */
                      "the Illusive",                     /* <= 19 */
                      "the Swindler",                     /* <= 24 */
                      "the Marauder",                     /* <= 29 */
                      "the Volatile",                     /* <= 30 */
                      "the Immortal Assassin",            /* <= LVL_IMMORT */
                      "the Demi God of Thieves",          /* <= LVL_STAFF */
                      "the God of Thieves and Tradesmen", /* <= LVL_GRSTAFF */
                      "the Rogue"                         /* default */
  );
  /* feat assignment */
  /*              class num     feat                           cfeat lvl stack */
  feat_assignment(CLASS_ROGUE, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_ROGUE, FEAT_WEAPON_PROFICIENCY_ROGUE, Y, 1, N);
  feat_assignment(CLASS_ROGUE, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_ROGUE, FEAT_WEAPON_FINESSE, Y, 1, N);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 1, Y);
  feat_assignment(CLASS_ROGUE, FEAT_TRAPFINDING, Y, 1, N);
  feat_assignment(CLASS_ROGUE, FEAT_EVASION, Y, 2, N);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 3, Y);
  feat_assignment(CLASS_ROGUE, FEAT_TRAP_SENSE, Y, 3, Y);
  /* talent lvl 3, slippery mind*/
  feat_assignment(CLASS_ROGUE, FEAT_SLIPPERY_MIND, Y, 3, N);
  feat_assignment(CLASS_ROGUE, FEAT_UNCANNY_DODGE, Y, 4, N);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 5, Y);
  feat_assignment(CLASS_ROGUE, FEAT_TRAP_SENSE, Y, 6, Y);
  /* talent lvl 6, crippling strike*/
  feat_assignment(CLASS_ROGUE, FEAT_CRIPPLING_STRIKE, Y, 6, N);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 7, Y);
  feat_assignment(CLASS_ROGUE, FEAT_IMPROVED_UNCANNY_DODGE, Y, 8, N);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 9, Y);
  feat_assignment(CLASS_ROGUE, FEAT_TRAP_SENSE, Y, 9, Y);
  /* talent lvl 9, improved evasion*/
  feat_assignment(CLASS_ROGUE, FEAT_IMPROVED_EVASION, Y, 9, N);
  /* talent lvl 10, apply poison */
  feat_assignment(CLASS_ROGUE, FEAT_APPLY_POISON, Y, 10, N);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 11, Y);
  feat_assignment(CLASS_ROGUE, FEAT_TRAP_SENSE, Y, 12, Y);
  /* talent lvl 12, able learner */
  feat_assignment(CLASS_ROGUE, FEAT_ABLE_LEARNER, Y, 12, Y);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 13, Y);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 15, Y);
  feat_assignment(CLASS_ROGUE, FEAT_TRAP_SENSE, Y, 15, Y);
  /* advanced talent lvl 15, defensive roll */
  feat_assignment(CLASS_ROGUE, FEAT_DEFENSIVE_ROLL, Y, 15, N);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 17, Y);
  feat_assignment(CLASS_ROGUE, FEAT_TRAP_SENSE, Y, 18, Y);
  /* talent lvl 18, dirtkick */
  feat_assignment(CLASS_ROGUE, FEAT_DIRT_KICK, Y, 18, N);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 19, Y);
  /*epic*/
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 21, Y);
  /* talent lvl 21, backstab */
  feat_assignment(CLASS_ROGUE, FEAT_BACKSTAB, Y, 21, N);
  feat_assignment(CLASS_ROGUE, FEAT_TRAP_SENSE, Y, 22, Y);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 23, Y);
  /* talent lvl 24, sap */
  feat_assignment(CLASS_ROGUE, FEAT_SAP, Y, 24, N);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 25, Y);
  feat_assignment(CLASS_ROGUE, FEAT_TRAP_SENSE, Y, 26, Y);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 27, Y);
  /* talent lvl 27, vanish */
  feat_assignment(CLASS_ROGUE, FEAT_VANISH, Y, 27, N);
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 29, Y);
  feat_assignment(CLASS_ROGUE, FEAT_TRAP_SENSE, Y, 30, Y);
  /* talent lvl 30, improved vanish */
  feat_assignment(CLASS_ROGUE, FEAT_IMPROVED_VANISH, Y, 30, N);
  /* rogues don't currently have any class feats */
  /* no prereqs! */
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name        abrv   clr-abrv       menu-name*/
  classo(CLASS_WARRIOR, "warrior", "War", "\tRWar\tn", "w) \tRWarrior\tn",
         /* max-lvl  lock? prestige? BAB HD  psp move trains in-game? unlkCst, eFeatp */
         -1, N, N, H, 10, 0, 1, 2, Y, 0, 2,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Strength, alternatively Dex... Con for survivability, 13 Int unlocks feat chains",
         /*descrip*/ "Some take up arms for glory, wealth, or revenge. Others do "
                     "battle to prove themselves, to protect others, or because they know nothing "
                     "else. Still others learn the ways of weaponcraft to hone their bodies in "
                     "battle and prove their mettle in the forge of war. Lords of the battlefield, "
                     "warriors are a disparate lot, training with many weapons or just one, perfecting "
                     "the uses of armor, learning the fighting techniques of exotic masters, and "
                     "studying the art of combat, all to shape themselves into living weapons. Far "
                     "more than mere thugs, these skilled combatants reveal the true deadliness of "
                     "their weapons, turning hunks of metal into arms capable of taming kingdoms, "
                     "slaughtering monsters, and rousing the hearts of armies. Soldiers, knights, "
                     "hunters, and artists of war, warriors are unparalleled champions, and woe to "
                     "those who dare stand against them.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_WARRIOR, G, B, B, B, B);
  assign_class_abils(CLASS_WARRIOR, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CA, CA, CA, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CA, CA, CA, CA, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CC, CC, CC, CC, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CC, CC);
  assign_class_titles(CLASS_WARRIOR,                 /* class number */
                      "",                            /* <= 4  */
                      "the Mostly Harmless",         /* <= 9  */
                      "the Useful in Bar-Fights",    /* <= 14 */
                      "the Friend to Violence",      /* <= 19 */
                      "the Strong",                  /* <= 24 */
                      "the Bane of All Enemies",     /* <= 29 */
                      "the Exceptionally Dangerous", /* <= 30 */
                      "the Immortal Warlord",        /* <= LVL_IMMORT */
                      "the Extirpator",              /* <= LVL_STAFF */
                      "the God of War",              /* <= LVL_GRSTAFF */
                      "the Warrior"                  /* default */
  );
  /* feat assignment */
  /* bonus: they select from a master list of combat feats every 2 levels */
  /*              class num     feat                            cfeat lvl stack */
  feat_assignment(CLASS_WARRIOR, FEAT_MARTIAL_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_PROFICIENCY_HEAVY, Y, 1, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_PROFICIENCY_MEDIUM, Y, 1, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_PROFICIENCY_SHIELD, Y, 1, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD, Y, 1, N);
  feat_assignment(CLASS_WARRIOR, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_TRAINING, Y, 3, Y);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_TRAINING, Y, 5, Y);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_TRAINING, Y, 7, Y);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_TRAINING, Y, 9, Y);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_TRAINING, Y, 11, Y);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_TRAINING, Y, 13, Y);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_TRAINING, Y, 15, Y);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_TRAINING, Y, 17, Y);
  feat_assignment(CLASS_WARRIOR, FEAT_STALWART_WARRIOR, Y, 19, N);
  /*epic*/
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_MASTERY, Y, 21, N);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_MASTERY, Y, 24, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_MASTERY_2, Y, 27, N);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_MASTERY_2, Y, 30, N);
  /* list of class feats */
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_SKIN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_SPECIALIZATION_LIGHT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_SPECIALIZATION_MEDIUM, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ARMOR_SPECIALIZATION_HEAVY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_BLIND_FIGHT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_CLEAVE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_COMBAT_EXPERTISE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_COMBAT_REFLEXES, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_DEFLECT_ARROWS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_DAMAGE_REDUCTION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_DODGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_EXOTIC_WEAPON_PROFICIENCY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_FAR_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_GREAT_CLEAVE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_GREATER_TWO_WEAPON_FIGHTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_GREATER_WEAPON_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_GREATER_WEAPON_SPECIALIZATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_BULL_RUSH, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_CRITICAL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_DISARM, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_FEINT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_GRAPPLE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_INITIATIVE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_OVERRUN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_PRECISE_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_SHIELD_PUNCH, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_KNOCKDOWN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_SHIELD_CHARGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_SHIELD_SLAM, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_SUNDER, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_TRIP, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_TWO_WEAPON_FIGHTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_IMPROVED_UNARMED_STRIKE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_MANYSHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_MOBILITY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_MOUNTED_ARCHERY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_MOUNTED_COMBAT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_POINT_BLANK_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_POWER_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_PRECISE_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_QUICK_DRAW, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_RAPID_RELOAD, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_RAPID_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_RIDE_BY_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_ROBILARS_GAMBIT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_SHOT_ON_THE_RUN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_SNATCH_ARROWS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_SPIRITED_CHARGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_SPRING_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_STUNNING_FIST, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_SWARM_OF_ARROWS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_TRAMPLE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_TWO_WEAPON_DEFENSE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_TWO_WEAPON_FIGHTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_FINESSE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_SPECIALIZATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_WHIRLWIND_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_FAST_HEALING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_MASTERY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_FLURRY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_WEAPON_SUPREMACY, Y, NOASSIGN_FEAT, N);
  /* epic class */
  feat_assignment(CLASS_WARRIOR, FEAT_EPIC_PROWESS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_GREAT_STRENGTH, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_GREAT_DEXTERITY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_GREAT_CONSTITUTION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_EPIC_TOUGHNESS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_WARRIOR, FEAT_EPIC_WEAPON_SPECIALIZATION, Y, NOASSIGN_FEAT, N);
  /* no spell assign */
  /* no prereqs! */
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name    abrv   clr-abrv     menu-name*/
  classo(CLASS_MONK, "monk", "Mon", "\tgMon\tn", "o) \tgMonk\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp */
         -1, N, N, H, 8, 0, 2, 4, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Wisdom, Con/Dex for survivability, Str for combat",
         /*descrip*/ "For the truly exemplary, martial skill transcends the "
                     "battlefield—it is a lifestyle, a doctrine, a state of mind. These warrior-"
                     "artists search out methods of battle beyond swords and shields, finding "
                     "weapons within themselves just as capable of crippling or killing as any "
                     "blade. These monks (so called since they adhere to ancient philosophies and "
                     "strict martial disciplines) elevate their bodies to become weapons of war, "
                     "from battle-minded ascetics to self-taught brawlers. Monks tread the path of "
                     "discipline, and those with the will to endure that path discover within "
                     "themselves not what they are, but what they are meant to be.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_MONK, G, G, G, B, B);
  assign_class_abils(CLASS_MONK, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CA, CA, CA, CA, CC, CA, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CA, CA, CA, CA, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CA, CC, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CC, CC);
  assign_class_titles(CLASS_MONK,                /* class number */
                      "",                        /* <= 4  */
                      "of the Crushing Fist",    /* <= 9  */
                      "of the Stomping Foot",    /* <= 14 */
                      "of the Directed Motions", /* <= 19 */
                      "of the Disciplined Body", /* <= 24 */
                      "of the Disciplined Mind", /* <= 29 */
                      "of the Mastered Self",    /* <= 30 */
                      "the Immortal Monk",       /* <= LVL_IMMORT */
                      "the Inquisitor Monk",     /* <= LVL_STAFF */
                      "the God of the Fist",     /* <= LVL_GRSTAFF */
                      "the Monk"                 /* default */
  );
  /* feat assignment */
  /*              class num     feat                           cfeat lvl stack */
  feat_assignment(CLASS_MONK, FEAT_WEAPON_PROFICIENCY_MONK, Y, 1, N);
  feat_assignment(CLASS_MONK, FEAT_WIS_AC_BONUS, Y, 1, N);
  feat_assignment(CLASS_MONK, FEAT_LVL_AC_BONUS, Y, 1, N);
  feat_assignment(CLASS_MONK, FEAT_UNARMED_STRIKE, Y, 1, N);
  feat_assignment(CLASS_MONK, FEAT_IMPROVED_UNARMED_STRIKE, Y, 1, N);
  feat_assignment(CLASS_MONK, FEAT_FLURRY_OF_BLOWS, Y, 1, N);
  feat_assignment(CLASS_MONK, FEAT_STUNNING_FIST, Y, 1, N);
  feat_assignment(CLASS_MONK, FEAT_EVASION, Y, 2, N);
  feat_assignment(CLASS_MONK, FEAT_KI_STRIKE, Y, 3, Y);
  feat_assignment(CLASS_MONK, FEAT_STILL_MIND, Y, 3, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 4, Y);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 5, Y);
  feat_assignment(CLASS_MONK, FEAT_PURITY_OF_BODY, Y, 5, N);
  feat_assignment(CLASS_MONK, FEAT_SPRING_ATTACK, Y, 5, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 6, Y);
  feat_assignment(CLASS_MONK, FEAT_KI_STRIKE, Y, 6, Y);
  feat_assignment(CLASS_MONK, FEAT_WHOLENESS_OF_BODY, Y, 7, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 8, Y);
  feat_assignment(CLASS_MONK, FEAT_KI_STRIKE, Y, 9, Y);
  feat_assignment(CLASS_MONK, FEAT_IMPROVED_EVASION, Y, 9, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 10, Y);
  feat_assignment(CLASS_MONK, FEAT_DIAMOND_BODY, Y, 11, N);
  feat_assignment(CLASS_MONK, FEAT_GREATER_FLURRY, Y, 11, N);
  feat_assignment(CLASS_MONK, FEAT_ABUNDANT_STEP, Y, 12, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 12, Y);
  feat_assignment(CLASS_MONK, FEAT_KI_STRIKE, Y, 12, Y);
  feat_assignment(CLASS_MONK, FEAT_DIAMOND_SOUL, Y, 13, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 14, Y);
  feat_assignment(CLASS_MONK, FEAT_QUIVERING_PALM, Y, 15, N);
  feat_assignment(CLASS_MONK, FEAT_KI_STRIKE, Y, 15, Y);
  feat_assignment(CLASS_MONK, FEAT_TIMELESS_BODY, Y, 16, N);
  feat_assignment(CLASS_MONK, FEAT_TONGUE_OF_THE_SUN_AND_MOON, Y, 17, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 18, Y);
  feat_assignment(CLASS_MONK, FEAT_KI_STRIKE, Y, 18, Y);
  feat_assignment(CLASS_MONK, FEAT_EMPTY_BODY, Y, 19, N);
  feat_assignment(CLASS_MONK, FEAT_PERFECT_SELF, Y, 20, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 20, Y);
  /*epic*/
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 21, Y);
  /* 23 bonus free epic feat */
  feat_assignment(CLASS_MONK, FEAT_KEEN_STRIKE, Y, 23, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 24, Y);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 26, Y);
  /* 26 bonus free epic feat */
  feat_assignment(CLASS_MONK, FEAT_BLINDING_SPEED, Y, 26, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 28, Y);
  /* 29 bonus free epic feat */
  feat_assignment(CLASS_MONK, FEAT_OUTSIDER, Y, 29, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 30, Y);
  /* monks get no class feats */
  /* prereqs */
  class_prereq_align(CLASS_MONK, LAWFUL_GOOD);
  class_prereq_align(CLASS_MONK, LAWFUL_NEUTRAL);
  class_prereq_align(CLASS_MONK, LAWFUL_EVIL);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name      abrv   clr-abrv          menu-name*/
  classo(CLASS_DRUID, "druid", "Dru", "\tGD\tgr\tGu\tn", "d) \tGD\tgr\tGu\tgi\tGd\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         -1, N, N, M, 8, 0, 3, 4, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Wisdom, Con/Dex for survivability, Str for combat",
         /*descrip*/ "Within the purity of the elements and the order of the wilds "
                     "lingers a power beyond the marvels of civilization. Furtive yet undeniable, "
                     "these primal magics are guarded over by servants of philosophical balance "
                     "known as druids. Allies to beasts and manipulators of nature, these often "
                     "misunderstood protectors of the wild strive to shield their lands from all "
                     "who would threaten them and prove the might of the wilds to those who lock "
                     "themselves behind city walls. Rewarded for their devotion with incredible "
                     "powers, druids gain unparalleled shape-shifting abilities, the companionship "
                     "of mighty beasts, and the power to call upon nature's wrath. The mightiest "
                     "temper powers akin to storms, earthquakes, and volcanoes with primeval wisdom "
                     "long abandoned and forgotten by civilization.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_DRUID, G, B, G, B, B);
  assign_class_abils(CLASS_DRUID, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CA, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CC, CA, CA, CA, CC, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CC, CC, CC, CA, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CA, CA, CC, CC);
  assign_class_titles(CLASS_DRUID,                  /* class number */
                      "",                           /* <= 4  */
                      "the Walker on Loam",         /* <= 9  */
                      "the Speaker for Beasts",     /* <= 14 */
                      "the Watcher from Shade",     /* <= 19 */
                      "the Whispering Winds",       /* <= 24 */
                      "the Balancer",               /* <= 29 */
                      "the Still Waters",           /* <= 30 */
                      "the Avatar of Nature",       /* <= LVL_IMMORT */
                      "the Wrath of Nature",        /* <= LVL_STAFF */
                      "the Storm of Earth's Voice", /* <= LVL_GRSTAFF */
                      "the Druid"                   /* default */
  );
  /* feat assignment */
  /*              class num     feat                          cfeat lvl stack */
  feat_assignment(CLASS_DRUID, FEAT_WEAPON_PROFICIENCY_DRUID, Y, 1, N);
  feat_assignment(CLASS_DRUID, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_DRUID, FEAT_ARMOR_PROFICIENCY_MEDIUM, Y, 1, N);
  feat_assignment(CLASS_DRUID, FEAT_ARMOR_PROFICIENCY_SHIELD, Y, 1, N);
  feat_assignment(CLASS_DRUID, FEAT_ANIMAL_COMPANION, Y, 1, N);
  feat_assignment(CLASS_DRUID, FEAT_NATURE_SENSE, Y, 1, N);
  feat_assignment(CLASS_DRUID, FEAT_WILD_EMPATHY, Y, 2, N);
  feat_assignment(CLASS_DRUID, FEAT_WOODLAND_STRIDE, Y, 2, N);
  feat_assignment(CLASS_DRUID, FEAT_TRACKLESS_STEP, Y, 3, N);
  feat_assignment(CLASS_DRUID, FEAT_RESIST_NATURES_LURE, Y, 4, N);
  /* FEAT_WILD_SHAPE is the first level of wildshape forms AND cooldown */
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 4, Y);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 6, Y);
  /* FEAT_WILD_SHAPE_x is the xth level of wildshape forms, does not affect cooldown */
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE_2, Y, 6, N);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 8, Y);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE_3, Y, 8, N);
  feat_assignment(CLASS_DRUID, FEAT_VENOM_IMMUNITY, Y, 9, N);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 10, Y);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE_4, Y, 10, N);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 12, Y);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE_5, Y, 12, N);
  feat_assignment(CLASS_DRUID, FEAT_THOUSAND_FACES, Y, 13, N);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 14, Y);
  feat_assignment(CLASS_DRUID, FEAT_TIMELESS_BODY, Y, 15, N);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 16, Y);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 18, Y);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 20, Y);
  /* spell circles */
  feat_assignment(CLASS_DRUID, FEAT_DRUID_1ST_CIRCLE, Y, 1, N);
  feat_assignment(CLASS_DRUID, FEAT_DRUID_2ND_CIRCLE, Y, 3, N);
  feat_assignment(CLASS_DRUID, FEAT_DRUID_3RD_CIRCLE, Y, 5, N);
  feat_assignment(CLASS_DRUID, FEAT_DRUID_4TH_CIRCLE, Y, 7, N);
  feat_assignment(CLASS_DRUID, FEAT_DRUID_5TH_CIRCLE, Y, 9, N);
  feat_assignment(CLASS_DRUID, FEAT_DRUID_6TH_CIRCLE, Y, 11, N);
  feat_assignment(CLASS_DRUID, FEAT_DRUID_7TH_CIRCLE, Y, 13, N);
  feat_assignment(CLASS_DRUID, FEAT_DRUID_8TH_CIRCLE, Y, 15, N);
  feat_assignment(CLASS_DRUID, FEAT_DRUID_9TH_CIRCLE, Y, 17, N);
  /*epic*/
  feat_assignment(CLASS_DRUID, FEAT_DRUID_EPIC_SPELL, Y, 21, N);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 22, Y);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 24, Y);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 26, Y);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 28, Y);
  feat_assignment(CLASS_DRUID, FEAT_WILD_SHAPE, Y, 30, Y);
  /* no class feats */
  /**** spell assign ****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  spell_assignment(CLASS_DRUID, SPELL_CHARM_ANIMAL, 1);
  spell_assignment(CLASS_DRUID, SPELL_CURE_LIGHT, 1);
  spell_assignment(CLASS_DRUID, SPELL_FAERIE_FIRE, 1);
  spell_assignment(CLASS_DRUID, SPELL_GOODBERRY, 1);
  spell_assignment(CLASS_DRUID, SPELL_JUMP, 1);
  spell_assignment(CLASS_DRUID, SPELL_MAGIC_FANG, 1);
  spell_assignment(CLASS_DRUID, SPELL_MAGIC_STONE, 1);
  spell_assignment(CLASS_DRUID, SPELL_OBSCURING_MIST, 1);
  spell_assignment(CLASS_DRUID, SPELL_PRODUCE_FLAME, 1);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_1, 1);
  spell_assignment(CLASS_DRUID, SPELL_ENTANGLE, 1);
  spell_assignment(CLASS_DRUID, SPELL_RESISTANCE, 1);
  spell_assignment(CLASS_DRUID, SPELL_VIGORIZE_LIGHT, 1);
  spell_assignment(CLASS_DRUID, SPELL_ANT_HAUL, 1);
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_DRUID, SPELL_BARKSKIN, 3);
  spell_assignment(CLASS_DRUID, SPELL_ENDURANCE, 3);
  spell_assignment(CLASS_DRUID, SPELL_STRENGTH, 3);
  spell_assignment(CLASS_DRUID, SPELL_GRACE, 3);
  spell_assignment(CLASS_DRUID, SPELL_FLAME_BLADE, 3);
  spell_assignment(CLASS_DRUID, SPELL_FLAMING_SPHERE, 3);
  spell_assignment(CLASS_DRUID, SPELL_HOLD_ANIMAL, 3);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_2, 3);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_SWARM, 3);
  spell_assignment(CLASS_DRUID, SPELL_WISDOM, 3);
  spell_assignment(CLASS_DRUID, SPELL_LESSER_RESTORATION, 3);
  spell_assignment(CLASS_DRUID, SPELL_VIGORIZE_SERIOUS, 3);
  spell_assignment(CLASS_DRUID, SPELL_MASS_ANT_HAUL, 3);
  spell_assignment(CLASS_DRUID, SPELL_GIRD_ALLIES, 3);
  spell_assignment(CLASS_DRUID, SPELL_SPIDER_CLIMB, 3);
  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_DRUID, SPELL_CALL_LIGHTNING, 5);
  spell_assignment(CLASS_DRUID, SPELL_CURE_MODERATE, 5);
  spell_assignment(CLASS_DRUID, SPELL_CONTAGION, 5);
  spell_assignment(CLASS_DRUID, SPELL_GREATER_MAGIC_FANG, 5);
  spell_assignment(CLASS_DRUID, SPELL_POISON, 5);
  spell_assignment(CLASS_DRUID, SPELL_REMOVE_DISEASE, 5);
  spell_assignment(CLASS_DRUID, SPELL_REMOVE_POISON, 5);
  spell_assignment(CLASS_DRUID, SPELL_SPIKE_GROWTH, 5);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_3, 5);
  spell_assignment(CLASS_DRUID, SPELL_LIFE_SHIELD, 5);
  spell_assignment(CLASS_DRUID, SPELL_PROTECTION_FROM_ENERGY, 5);
  spell_assignment(CLASS_DRUID, SPELL_WIND_WALL, 5);
  spell_assignment(CLASS_DRUID, SPELL_VIGORIZE_CRITICAL, 5);
  spell_assignment(CLASS_DRUID, SPELL_MOONBEAM, 5);
  spell_assignment(CLASS_DRUID, SPELL_AQUEOUS_ORB, 5);
  spell_assignment(CLASS_DRUID, SPELL_SIPHON_MIGHT, 5);
  spell_assignment(CLASS_DRUID, SPELL_COMMUNAL_RESIST_ENERGY, 5);
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_DRUID, SPELL_BLIGHT, 7);
  spell_assignment(CLASS_DRUID, SPELL_CURE_SERIOUS, 7);
  spell_assignment(CLASS_DRUID, SPELL_DISPEL_MAGIC, 7);
  spell_assignment(CLASS_DRUID, SPELL_FLAME_STRIKE, 7);
  spell_assignment(CLASS_DRUID, SPELL_FREE_MOVEMENT, 7);
  spell_assignment(CLASS_DRUID, SPELL_ICE_STORM, 7);
  spell_assignment(CLASS_DRUID, SPELL_LOCATE_CREATURE, 7);
  spell_assignment(CLASS_DRUID, SPELL_SPIKE_STONES, 7);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_4, 7);
  spell_assignment(CLASS_DRUID, SPELL_COMMUNAL_PROTECTION_FROM_ENERGY, 7);
  spell_assignment(CLASS_DRUID, SPELL_AIR_WALK, 7);
  spell_assignment(CLASS_DRUID, SPELL_DISPEL_INVIS, 7);
  spell_assignment(CLASS_DRUID, SPELL_GROUP_VIGORIZE, 7);
  // spell_assignment(SPELL_REINCARNATE, 7);
  /*              class num      spell                   level acquired */
  /* 5th circle */
  // spell_assignment(CLASS_DRUID, SPELL_BALEFUL_POLYMORPH, 9);
  spell_assignment(CLASS_DRUID, SPELL_CALL_LIGHTNING_STORM, 9);
  spell_assignment(CLASS_DRUID, SPELL_CURE_CRITIC, 9);
  spell_assignment(CLASS_DRUID, SPELL_DEATH_WARD, 9);
  spell_assignment(CLASS_DRUID, SPELL_HALLOW, 9);
  spell_assignment(CLASS_DRUID, SPELL_INSECT_PLAGUE, 9);
  spell_assignment(CLASS_DRUID, SPELL_STONESKIN, 9);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_5, 9);
  spell_assignment(CLASS_DRUID, SPELL_UNHALLOW, 9);
  spell_assignment(CLASS_DRUID, SPELL_WALL_OF_FIRE, 9);
  spell_assignment(CLASS_DRUID, SPELL_WALL_OF_THORNS, 9);
  /*              class num      spell                   level acquired */
  /* 6th circle */
  spell_assignment(CLASS_DRUID, SPELL_FIRE_SEEDS, 11);
  spell_assignment(CLASS_DRUID, SPELL_GREATER_DISPELLING, 11);
  spell_assignment(CLASS_DRUID, SPELL_MASS_ENDURANCE, 11);
  spell_assignment(CLASS_DRUID, SPELL_MASS_STRENGTH, 11);
  spell_assignment(CLASS_DRUID, SPELL_MASS_GRACE, 11);
  spell_assignment(CLASS_DRUID, SPELL_MASS_WISDOM, 11);
  spell_assignment(CLASS_DRUID, SPELL_SPELLSTAFF, 11);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_6, 11);
  spell_assignment(CLASS_DRUID, SPELL_TRANSPORT_VIA_PLANTS, 11);
  spell_assignment(CLASS_DRUID, SPELL_MASS_CURE_LIGHT, 11);
  spell_assignment(CLASS_DRUID, SPELL_GREATER_BLACK_TENTACLES, 11);
  /*              class num      spell                   level acquired */
  /* 7th circle */
  spell_assignment(CLASS_DRUID, SPELL_CONTROL_WEATHER, 13);
  spell_assignment(CLASS_DRUID, SPELL_CREEPING_DOOM, 13);
  spell_assignment(CLASS_DRUID, SPELL_FIRE_STORM, 13);
  spell_assignment(CLASS_DRUID, SPELL_HEAL, 13);
  spell_assignment(CLASS_DRUID, SPELL_MASS_CURE_MODERATE, 13);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_7, 13);
  spell_assignment(CLASS_DRUID, SPELL_SUNBEAM, 13);
  // spell_assignment(CLASS_DRUID, SPELL_GREATER_SCRYING, 13);
  /*              class num      spell                   level acquired */
  /* 8th circle */
  spell_assignment(CLASS_DRUID, SPELL_ANIMAL_SHAPES, 15);
  spell_assignment(CLASS_DRUID, SPELL_CONTROL_PLANTS, 15);
  spell_assignment(CLASS_DRUID, SPELL_EARTHQUAKE, 15);
  spell_assignment(CLASS_DRUID, SPELL_FINGER_OF_DEATH, 15);
  spell_assignment(CLASS_DRUID, SPELL_MASS_CURE_SERIOUS, 15);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_8, 15);
  spell_assignment(CLASS_DRUID, SPELL_SUNBURST, 15);
  spell_assignment(CLASS_DRUID, SPELL_WHIRLWIND, 15);
  spell_assignment(CLASS_DRUID, SPELL_WORD_OF_RECALL, 15);
  spell_assignment(CLASS_DRUID, SPELL_FINGER_OF_DEATH, 15);
  /*              class num      spell                   level acquired */
  /* 9th circle */
  spell_assignment(CLASS_DRUID, SPELL_ELEMENTAL_SWARM, 17);
  spell_assignment(CLASS_DRUID, SPELL_REGENERATION, 17);
  spell_assignment(CLASS_DRUID, SPELL_MASS_CURE_CRIT, 17);
  spell_assignment(CLASS_DRUID, SPELL_SHAMBLER, 17);
  spell_assignment(CLASS_DRUID, SPELL_POLYMORPH, 17);
  spell_assignment(CLASS_DRUID, SPELL_STORM_OF_VENGEANCE, 17);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_9, 17);
  spell_assignment(CLASS_DRUID, SPELL_RESURRECT, 17);

  /*epic*/
  spell_assignment(CLASS_DRUID, SPELL_DRAGON_KNIGHT, 21);
  spell_assignment(CLASS_DRUID, SPELL_GREATER_RUIN, 21);
  spell_assignment(CLASS_DRUID, SPELL_HELLBALL, 21);
  /* class prerequisites */
  class_prereq_align(CLASS_DRUID, NEUTRAL_GOOD);
  class_prereq_align(CLASS_DRUID, LAWFUL_NEUTRAL);
  class_prereq_align(CLASS_DRUID, TRUE_NEUTRAL);
  class_prereq_align(CLASS_DRUID, CHAOTIC_NEUTRAL);
  class_prereq_align(CLASS_DRUID, NEUTRAL_EVIL);
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  // assign_feat_spell_slots(CLASS_DRUID);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number        name      abrv   clr-abrv           menu-name*/
  classo(CLASS_BERSERKER, "berserker", "Bes", "\trB\tRe\trs\tn", "b) \trBer\tRser\trker\tn",
         /* max-lvl  lock? prestige? BAB HD  psp move trains in-game? unlkCst, eFeatp */
         -1, N, N, H, 12, 0, 2, 4, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Strength, Con/Dex for survivability - Con helps some of their skills",
         /*descrip*/ "For some, there is only rage. In the ways of their people, in "
                     "the fury of their passion, in the howl of battle, conflict is all these brutal "
                     "souls know. Savages, hired muscle, masters of vicious martial techniques, they "
                     "are not soldiers or professional warriors—they are the battle possessed, "
                     "creatures of slaughter and spirits of war. Known as berserkers, these warmongers "
                     "know little of training, preparation, or the rules of warfare; for them, only "
                     "the moment exists, with the foes that stand before them and the knowledge that "
                     "the next moment might hold their death. They possess a sixth sense in regard to "
                     "danger and the endurance to weather all that might entail. These brutal warriors "
                     "might rise from all walks of life, both civilized and savage, though whole "
                     "societies embracing such philosophies roam the wild places of the world. Within "
                     "berserkers storms the primal spirit of battle, and woe to those who face their "
                     "rage.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_BERSERKER, G, B, B, B, B);
  assign_class_abils(CLASS_BERSERKER, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CA, CA, CC, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CC, CA, CA, CA, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CA, CC, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CC, CC);
  assign_class_titles(CLASS_BERSERKER,          /* class number */
                      "",                       /* <= 4  */
                      "the Ripper of Flesh",    /* <= 9  */
                      "the Shatterer of Bone",  /* <= 14 */
                      "the Cleaver of Organs",  /* <= 19 */
                      "the Wrecker of Hope",    /* <= 24 */
                      "the Effulgence of Rage", /* <= 29 */
                      "the Foe-Hewer",          /* <= 30 */
                      "the Immortal Warlord",   /* <= LVL_IMMORT */
                      "the Extirpator",         /* <= LVL_STAFF */
                      "the God of Rage",        /* <= LVL_GRSTAFF */
                      "the Berserker"           /* default */
  );
  /* feat assignment */
  /*              class num     feat                               cfeat lvl stack */
  feat_assignment(CLASS_BERSERKER, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_BERSERKER, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_BERSERKER, FEAT_ARMOR_PROFICIENCY_MEDIUM, Y, 1, N);
  feat_assignment(CLASS_BERSERKER, FEAT_ARMOR_PROFICIENCY_SHIELD, Y, 1, N);
  feat_assignment(CLASS_BERSERKER, FEAT_MARTIAL_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_BERSERKER, FEAT_FAST_MOVEMENT, Y, 1, N);
  feat_assignment(CLASS_BERSERKER, FEAT_RAGE, Y, 1, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_UNCANNY_DODGE, Y, 2, N);
  feat_assignment(CLASS_BERSERKER, FEAT_TRAP_SENSE, Y, 3, Y);
  /* rage power (level 3) */
  feat_assignment(CLASS_BERSERKER, FEAT_RP_SURPRISE_ACCURACY, Y, 3, N);
  feat_assignment(CLASS_BERSERKER, FEAT_RAGE, Y, 4, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_SHRUG_DAMAGE, Y, 4, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_IMPROVED_UNCANNY_DODGE, Y, 5, N);
  feat_assignment(CLASS_BERSERKER, FEAT_TRAP_SENSE, Y, 6, Y);
  /* rage power (level 6) */
  feat_assignment(CLASS_BERSERKER, FEAT_RP_POWERFUL_BLOW, Y, 6, N);
  feat_assignment(CLASS_BERSERKER, FEAT_SHRUG_DAMAGE, Y, 7, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_RAGE, Y, 8, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_TRAP_SENSE, Y, 9, Y);
  /* rage power (level 9) */
  feat_assignment(CLASS_BERSERKER, FEAT_RP_RENEWED_VIGOR, Y, 9, N);
  feat_assignment(CLASS_BERSERKER, FEAT_SHRUG_DAMAGE, Y, 10, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_GREATER_RAGE, Y, 11, N);
  feat_assignment(CLASS_BERSERKER, FEAT_RAGE, Y, 11, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_TRAP_SENSE, Y, 12, Y);
  /* rage power (level 12) */
  feat_assignment(CLASS_BERSERKER, FEAT_RP_HEAVY_SHRUG, Y, 12, N);
  feat_assignment(CLASS_BERSERKER, FEAT_SHRUG_DAMAGE, Y, 13, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_INDOMITABLE_WILL, Y, 14, N);
  feat_assignment(CLASS_BERSERKER, FEAT_TRAP_SENSE, Y, 15, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_RAGE, Y, 15, Y);
  /* rage power (level 15) */
  feat_assignment(CLASS_BERSERKER, FEAT_RP_FEARLESS_RAGE, Y, 15, N);
  feat_assignment(CLASS_BERSERKER, FEAT_SHRUG_DAMAGE, Y, 16, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_TIRELESS_RAGE, Y, 17, N);
  feat_assignment(CLASS_BERSERKER, FEAT_TRAP_SENSE, Y, 18, Y);
  /* rage power (level 18) */
  feat_assignment(CLASS_BERSERKER, FEAT_RP_COME_AND_GET_ME, Y, 18, N);
  feat_assignment(CLASS_BERSERKER, FEAT_SHRUG_DAMAGE, Y, 19, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_RAGE, Y, 20, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_MIGHTY_RAGE, Y, 20, N);
  /*epic*/
  /* rage power lvl 22*/
  feat_assignment(CLASS_BERSERKER, FEAT_EATER_OF_MAGIC, Y, 22, N);
  feat_assignment(CLASS_BERSERKER, FEAT_SHRUG_DAMAGE, Y, 22, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_RAGE, Y, 24, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_SHRUG_DAMAGE, Y, 25, Y);
  /* rage power lvl 26*/
  feat_assignment(CLASS_BERSERKER, FEAT_RAGE_RESISTANCE, Y, 26, N);
  feat_assignment(CLASS_BERSERKER, FEAT_INDOMITABLE_RAGE, Y, 27, N);
  feat_assignment(CLASS_BERSERKER, FEAT_SHRUG_DAMAGE, Y, 28, Y);
  feat_assignment(CLASS_BERSERKER, FEAT_RAGE, Y, 29, Y);
  /* rage power lvl 30*/
  feat_assignment(CLASS_BERSERKER, FEAT_DEATHLESS_FRENZY, Y, 30, N);
  feat_assignment(CLASS_BERSERKER, FEAT_RAGING_CRITICAL, Y, 30, N);
  /* no spell assignment */
  /* class prerequisites */
  class_prereq_align(CLASS_BERSERKER, NEUTRAL_GOOD);
  class_prereq_align(CLASS_BERSERKER, TRUE_NEUTRAL);
  class_prereq_align(CLASS_BERSERKER, NEUTRAL_EVIL);
  class_prereq_align(CLASS_BERSERKER, CHAOTIC_EVIL);
  class_prereq_align(CLASS_BERSERKER, CHAOTIC_GOOD);
  class_prereq_align(CLASS_BERSERKER, CHAOTIC_NEUTRAL);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number     name      abrv   clr-abrv     menu-name*/
  classo(CLASS_SORCERER, "sorcerer", "Sor", "\tMSor\tn", "s) \tMSorcerer\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         -1, N, N, L, 6, 0, 1, 2, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Charisma, Con/Dex for survivability",
         /*descrip*/ "Scions of innately magical bloodlines, the chosen of deities, "
                     "the spawn of monsters, pawns of fate and destiny, or simply flukes of fickle "
                     "magic, sorcerers look within themselves for arcane prowess and draw forth might "
                     "few mortals can imagine. Emboldened by lives ever threatening to be consumed "
                     "by their innate powers, these magic-touched souls endlessly indulge in and "
                     "refine their mysterious abilities, gradually learning how to harness their "
                     "birthright and coax forth ever greater arcane feats. Just as varied as these "
                     "innately powerful spellcasters' abilities and inspirations are the ways in "
                     "which they choose to utilize their gifts. While some seek to control their "
                     "abilities through meditation and discipline, becoming masters of their "
                     "fantastic birthright, others give in to their magic, letting it rule their "
                     "lives with often explosive results. Regardless, sorcerers live and breathe "
                     "that which other spellcasters devote their lives to mastering, and for them "
                     "magic is more than a boon or a field of study; it is life itself.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_SORCERER, B, B, G, B, B);
  assign_class_abils(CLASS_SORCERER, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CC, CC, CA, CA, CC, CC, CA,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CC, CC, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_SORCERER,               /* class number */
                      "",                           /* <= 4  */
                      "the Awakened",               /* <= 9  */
                      "the Torch",                  /* <= 14 */
                      "the Firebrand",              /* <= 19 */
                      "the Destroyer",              /* <= 24 */
                      "the Crux of Power",          /* <= 29 */
                      "the Near-Divine",            /* <= 30 */
                      "the Immortal Magic Weaver",  /* <= LVL_IMMORT */
                      "the Avatar of the Flow",     /* <= LVL_STAFF */
                      "the Hand of Mystical Might", /* <= LVL_GRSTAFF */
                      "the Sorcerer"                /* default */
  );
  /* feat assignment */
  /*              class num     feat                            cfeat lvl stack */
  feat_assignment(CLASS_SORCERER, FEAT_WEAPON_PROFICIENCY_WIZARD, Y, 1, N);
  feat_assignment(CLASS_SORCERER, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_SORCERER, FEAT_SUMMON_FAMILIAR, Y, 1, N);
  feat_assignment(CLASS_SORCERER, FEAT_SORCERER_1ST_CIRCLE, Y, 1, N);
  feat_assignment(CLASS_SORCERER, FEAT_SORCERER_2ND_CIRCLE, Y, 4, N);
  feat_assignment(CLASS_SORCERER, FEAT_SORCERER_3RD_CIRCLE, Y, 6, N);
  feat_assignment(CLASS_SORCERER, FEAT_SORCERER_4TH_CIRCLE, Y, 8, N);
  feat_assignment(CLASS_SORCERER, FEAT_SORCERER_5TH_CIRCLE, Y, 10, N);
  feat_assignment(CLASS_SORCERER, FEAT_SORCERER_6TH_CIRCLE, Y, 12, N);
  feat_assignment(CLASS_SORCERER, FEAT_SORCERER_7TH_CIRCLE, Y, 14, N);
  feat_assignment(CLASS_SORCERER, FEAT_SORCERER_8TH_CIRCLE, Y, 16, N);
  feat_assignment(CLASS_SORCERER, FEAT_SORCERER_9TH_CIRCLE, Y, 18, N);
  /*epic*/
  feat_assignment(CLASS_SORCERER, FEAT_SORCERER_EPIC_SPELL, Y, 21, N);
  /* list of class feats */
  feat_assignment(CLASS_SORCERER, FEAT_COMBAT_CASTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_SPELL_PENETRATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_GREATER_SPELL_PENETRATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_ARMORED_SPELLCASTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_FASTER_MEMORIZATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_SPELL_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_GREATER_SPELL_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_IMPROVED_FAMILIAR, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_QUICK_CHANT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_AUGMENT_SUMMONING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_ENHANCED_SPELL_DAMAGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_MAXIMIZE_SPELL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SORCERER, FEAT_QUICKEN_SPELL, Y, NOASSIGN_FEAT, N);
  /**** spell assign ****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  spell_assignment(CLASS_SORCERER, SPELL_HORIZIKAULS_BOOM, 1);
  spell_assignment(CLASS_SORCERER, SPELL_MAGIC_MISSILE, 1);
  spell_assignment(CLASS_SORCERER, SPELL_BURNING_HANDS, 1);
  spell_assignment(CLASS_SORCERER, SPELL_ICE_DAGGER, 1);
  spell_assignment(CLASS_SORCERER, SPELL_MAGE_ARMOR, 1);
  spell_assignment(CLASS_SORCERER, SPELL_SUMMON_CREATURE_1, 1);
  spell_assignment(CLASS_SORCERER, SPELL_CHILL_TOUCH, 1);
  spell_assignment(CLASS_SORCERER, SPELL_NEGATIVE_ENERGY_RAY, 1);
  spell_assignment(CLASS_SORCERER, SPELL_RAY_OF_ENFEEBLEMENT, 1);
  spell_assignment(CLASS_SORCERER, SPELL_CHARM, 1);
  spell_assignment(CLASS_SORCERER, SPELL_ENCHANT_ITEM, 1);
  spell_assignment(CLASS_SORCERER, SPELL_SLEEP, 1);
  spell_assignment(CLASS_SORCERER, SPELL_COLOR_SPRAY, 1);
  spell_assignment(CLASS_SORCERER, SPELL_SCARE, 1);
  spell_assignment(CLASS_SORCERER, SPELL_TRUE_STRIKE, 1);
  spell_assignment(CLASS_SORCERER, SPELL_IDENTIFY, 1);
  spell_assignment(CLASS_SORCERER, SPELL_SHELGARNS_BLADE, 1);
  spell_assignment(CLASS_SORCERER, SPELL_GREASE, 1);
  spell_assignment(CLASS_SORCERER, SPELL_ENDURE_ELEMENTS, 1);
  spell_assignment(CLASS_SORCERER, SPELL_PROT_FROM_EVIL, 1);
  spell_assignment(CLASS_SORCERER, SPELL_PROT_FROM_GOOD, 1);
  spell_assignment(CLASS_SORCERER, SPELL_EXPEDITIOUS_RETREAT, 1);
  spell_assignment(CLASS_SORCERER, SPELL_IRON_GUTS, 1);
  spell_assignment(CLASS_SORCERER, SPELL_SHIELD, 1);
  spell_assignment(CLASS_SORCERER, SPELL_STUNNING_BARRIER, 1);
  spell_assignment(CLASS_SORCERER, SPELL_RESISTANCE, 1);
  spell_assignment(CLASS_SORCERER, SPELL_ANT_HAUL, 1);
  spell_assignment(CLASS_SORCERER, SPELL_CORROSIVE_TOUCH, 1);
  spell_assignment(CLASS_SORCERER, SPELL_PLANAR_HEALING, 1);
  spell_assignment(CLASS_SORCERER, SPELL_MOUNT, 1);
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_SORCERER, SPELL_SHOCKING_GRASP, 4);
  spell_assignment(CLASS_SORCERER, SPELL_SCORCHING_RAY, 4);
  spell_assignment(CLASS_SORCERER, SPELL_CONTINUAL_FLAME, 4);
  spell_assignment(CLASS_SORCERER, SPELL_SUMMON_CREATURE_2, 4);
  spell_assignment(CLASS_SORCERER, SPELL_WEB, 4);
  spell_assignment(CLASS_SORCERER, SPELL_ACID_ARROW, 4);
  spell_assignment(CLASS_SORCERER, SPELL_DANCING_WEAPON, 4);
  spell_assignment(CLASS_SORCERER, SPELL_BLINDNESS, 4);
  spell_assignment(CLASS_SORCERER, SPELL_DEAFNESS, 4);
  spell_assignment(CLASS_SORCERER, SPELL_FALSE_LIFE, 4);
  spell_assignment(CLASS_SORCERER, SPELL_DAZE_MONSTER, 4);
  spell_assignment(CLASS_SORCERER, SPELL_HIDEOUS_LAUGHTER, 4);
  spell_assignment(CLASS_SORCERER, SPELL_TOUCH_OF_IDIOCY, 4);
  spell_assignment(CLASS_SORCERER, SPELL_BLUR, 4);
  spell_assignment(CLASS_SORCERER, SPELL_MIRROR_IMAGE, 4);
  spell_assignment(CLASS_SORCERER, SPELL_INVISIBLE, 4);
  spell_assignment(CLASS_SORCERER, SPELL_DETECT_INVIS, 4);
  spell_assignment(CLASS_SORCERER, SPELL_DETECT_MAGIC, 4);
  spell_assignment(CLASS_SORCERER, SPELL_DARKNESS, 4);
  // spell_assignment(CLASS_SORCERER, SPELL_I_DARKNESS,        4);
  spell_assignment(CLASS_SORCERER, SPELL_RESIST_ENERGY, 4);
  spell_assignment(CLASS_SORCERER, SPELL_ENERGY_SPHERE, 4);
  spell_assignment(CLASS_SORCERER, SPELL_ENDURANCE, 4);
  spell_assignment(CLASS_SORCERER, SPELL_STRENGTH, 4);
  spell_assignment(CLASS_SORCERER, SPELL_GRACE, 4);
  spell_assignment(CLASS_SORCERER, SPELL_TACTICAL_ACUMEN, 4);
  spell_assignment(CLASS_SORCERER, SPELL_BESTOW_WEAPON_PROFICIENCY, 4);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_ANT_HAUL, 4);
  spell_assignment(CLASS_SORCERER, SPELL_CUSHIONING_BANDS, 4);
  spell_assignment(CLASS_SORCERER, SPELL_GIRD_ALLIES, 4);
  spell_assignment(CLASS_SORCERER, SPELL_GLITTERDUST, 4);
  spell_assignment(CLASS_SORCERER, SPELL_SPIDER_CLIMB, 4);
  spell_assignment(CLASS_SORCERER, SPELL_WARDING_WEAPON, 4);
  spell_assignment(CLASS_SORCERER, SPELL_PROTECTION_FROM_ARROWS, 4);
  spell_assignment(CLASS_SORCERER, SPELL_COMMUNAL_MOUNT, 4);
  spell_assignment(CLASS_SORCERER, SPELL_HUMAN_POTENTIAL, 4);
  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_SORCERER, SPELL_LIGHTNING_BOLT, 6);
  spell_assignment(CLASS_SORCERER, SPELL_FIREBALL, 6);
  spell_assignment(CLASS_SORCERER, SPELL_WATER_BREATHE, 6);
  spell_assignment(CLASS_SORCERER, SPELL_SUMMON_CREATURE_3, 6);
  spell_assignment(CLASS_SORCERER, SPELL_PHANTOM_STEED, 6);
  spell_assignment(CLASS_SORCERER, SPELL_STINKING_CLOUD, 6);
  spell_assignment(CLASS_SORCERER, SPELL_HALT_UNDEAD, 6);
  spell_assignment(CLASS_SORCERER, SPELL_VAMPIRIC_TOUCH, 6);
  spell_assignment(CLASS_SORCERER, SPELL_HEROISM, 6);
  spell_assignment(CLASS_SORCERER, SPELL_FLY, 6);
  spell_assignment(CLASS_SORCERER, SPELL_RAGE, 6);
  spell_assignment(CLASS_SORCERER, SPELL_HOLD_PERSON, 6);
  spell_assignment(CLASS_SORCERER, SPELL_DEEP_SLUMBER, 6);
  spell_assignment(CLASS_SORCERER, SPELL_WALL_OF_FOG, 6);
  spell_assignment(CLASS_SORCERER, SPELL_INVISIBILITY_SPHERE, 6);
  spell_assignment(CLASS_SORCERER, SPELL_DAYLIGHT, 6);
  spell_assignment(CLASS_SORCERER, SPELL_CLAIRVOYANCE, 6);
  spell_assignment(CLASS_SORCERER, SPELL_NON_DETECTION, 6);
  spell_assignment(CLASS_SORCERER, SPELL_DISPEL_MAGIC, 6);
  spell_assignment(CLASS_SORCERER, SPELL_HASTE, 6);
  spell_assignment(CLASS_SORCERER, SPELL_SLOW, 6);
  spell_assignment(CLASS_SORCERER, SPELL_CIRCLE_A_EVIL, 6);
  spell_assignment(CLASS_SORCERER, SPELL_CIRCLE_A_GOOD, 6);
  spell_assignment(CLASS_SORCERER, SPELL_CUNNING, 6);
  spell_assignment(CLASS_SORCERER, SPELL_WISDOM, 6);
  spell_assignment(CLASS_SORCERER, SPELL_CHARISMA, 6);
  spell_assignment(CLASS_SORCERER, SPELL_GREATER_MAGIC_WEAPON, 6);
  spell_assignment(CLASS_SORCERER, SPELL_WEAPON_OF_IMPACT, 6);
  spell_assignment(CLASS_SORCERER, SPELL_KEEN_EDGE, 6);
  spell_assignment(CLASS_SORCERER, SPELL_PROTECTION_FROM_ENERGY, 6);
  spell_assignment(CLASS_SORCERER, SPELL_WIND_WALL, 6);
  spell_assignment(CLASS_SORCERER, SPELL_SIPHON_MIGHT, 6);
  spell_assignment(CLASS_SORCERER, SPELL_GASEOUS_FORM, 6);
  spell_assignment(CLASS_SORCERER, SPELL_AQUEOUS_ORB, 6);
  spell_assignment(CLASS_SORCERER, SPELL_CONTROL_SUMMONED_CREATURE, 6);
  spell_assignment(CLASS_SORCERER, SPELL_COMMUNAL_PROTECTION_FROM_ARROWS, 6);
  spell_assignment(CLASS_SORCERER, SPELL_COMMUNAL_RESIST_ENERGY, 6);
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_SORCERER, SPELL_LESSER_MISSILE_STORM, 8);
  spell_assignment(CLASS_SORCERER, SPELL_FIRE_SHIELD, 8);
  spell_assignment(CLASS_SORCERER, SPELL_COLD_SHIELD, 8);
  spell_assignment(CLASS_SORCERER, SPELL_ICE_STORM, 8);
  spell_assignment(CLASS_SORCERER, SPELL_BILLOWING_CLOUD, 8);
  spell_assignment(CLASS_SORCERER, SPELL_SUMMON_CREATURE_4, 8);
  spell_assignment(CLASS_SORCERER, SPELL_ANIMATE_DEAD, 8);
  spell_assignment(CLASS_SORCERER, SPELL_CURSE, 8);
  spell_assignment(CLASS_SORCERER, SPELL_INFRAVISION, 8);
  spell_assignment(CLASS_SORCERER, SPELL_POISON, 8);
  spell_assignment(CLASS_SORCERER, SPELL_GREATER_INVIS, 8);
  spell_assignment(CLASS_SORCERER, SPELL_RAINBOW_PATTERN, 8);
  spell_assignment(CLASS_SORCERER, SPELL_WIZARD_EYE, 8);
  spell_assignment(CLASS_SORCERER, SPELL_LOCATE_CREATURE, 8);
  spell_assignment(CLASS_SORCERER, SPELL_MINOR_GLOBE, 8);
  spell_assignment(CLASS_SORCERER, SPELL_REMOVE_CURSE, 8);
  spell_assignment(CLASS_SORCERER, SPELL_STONESKIN, 8);
  spell_assignment(CLASS_SORCERER, SPELL_ENLARGE_PERSON, 8);
  spell_assignment(CLASS_SORCERER, SPELL_SHRINK_PERSON, 8);
  spell_assignment(CLASS_SORCERER, SPELL_CONFUSION, 8);
  spell_assignment(CLASS_SORCERER, SPELL_FEAR, 8);
  spell_assignment(CLASS_SORCERER, SPELL_SHADOW_JUMP, 8);
  spell_assignment(CLASS_SORCERER, SPELL_COMMUNAL_PROTECTION_FROM_ENERGY, 8);
  spell_assignment(CLASS_SORCERER, SPELL_GHOST_WOLF, 8);
  spell_assignment(CLASS_SORCERER, SPELL_BLACK_TENTACLES, 8);
  spell_assignment(CLASS_SORCERER, SPELL_CHARM_MONSTER, 8);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_ENLARGE_PERSON, 8);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_REDUCE_PERSON, 8);
  /*              class num      spell                   level acquired */
  /* 5th circle */
  spell_assignment(CLASS_SORCERER, SPELL_INTERPOSING_HAND, 10);
  spell_assignment(CLASS_SORCERER, SPELL_WALL_OF_FORCE, 10);
  spell_assignment(CLASS_SORCERER, SPELL_BALL_OF_LIGHTNING, 10);
  spell_assignment(CLASS_SORCERER, SPELL_CLOUDKILL, 10);
  spell_assignment(CLASS_SORCERER, SPELL_SUMMON_CREATURE_5, 10);
  spell_assignment(CLASS_SORCERER, SPELL_WAVES_OF_FATIGUE, 10);
  spell_assignment(CLASS_SORCERER, SPELL_SYMBOL_OF_PAIN, 10);
  spell_assignment(CLASS_SORCERER, SPELL_DOMINATE_PERSON, 10);
  spell_assignment(CLASS_SORCERER, SPELL_FEEBLEMIND, 10);
  spell_assignment(CLASS_SORCERER, SPELL_NIGHTMARE, 10);
  spell_assignment(CLASS_SORCERER, SPELL_MIND_FOG, 10);
  spell_assignment(CLASS_SORCERER, SPELL_ACID_SHEATH, 10);
  spell_assignment(CLASS_SORCERER, SPELL_FAITHFUL_HOUND, 10);
  spell_assignment(CLASS_SORCERER, SPELL_DISMISSAL, 10);
  spell_assignment(CLASS_SORCERER, SPELL_CONE_OF_COLD, 10);
  spell_assignment(CLASS_SORCERER, SPELL_TELEKINESIS, 10);
  spell_assignment(CLASS_SORCERER, SPELL_FIREBRAND, 10);
  /*              class num      spell                   level acquired */
  /* 6th circle */
  spell_assignment(CLASS_SORCERER, SPELL_FREEZING_SPHERE, 12);
  spell_assignment(CLASS_SORCERER, SPELL_ACID_FOG, 12);
  spell_assignment(CLASS_SORCERER, SPELL_SUMMON_CREATURE_6, 12);
  spell_assignment(CLASS_SORCERER, SPELL_TRANSFORMATION, 12);
  spell_assignment(CLASS_SORCERER, SPELL_EYEBITE, 12);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_HASTE, 12);
  spell_assignment(CLASS_SORCERER, SPELL_GREATER_HEROISM, 12);
  spell_assignment(CLASS_SORCERER, SPELL_ANTI_MAGIC_FIELD, 12);
  spell_assignment(CLASS_SORCERER, SPELL_GREATER_MIRROR_IMAGE, 12);
  spell_assignment(CLASS_SORCERER, SPELL_LOCATE_OBJECT, 12);
  spell_assignment(CLASS_SORCERER, SPELL_TRUE_SEEING, 12);
  spell_assignment(CLASS_SORCERER, SPELL_GLOBE_OF_INVULN, 12);
  spell_assignment(CLASS_SORCERER, SPELL_GREATER_DISPELLING, 12);
  spell_assignment(CLASS_SORCERER, SPELL_CLONE, 12);
  spell_assignment(CLASS_SORCERER, SPELL_WATERWALK, 12);
  spell_assignment(CLASS_SORCERER, SPELL_LEVITATE, 12);
  spell_assignment(CLASS_SORCERER, SPELL_SHADOW_WALK, 12);
  spell_assignment(CLASS_SORCERER, SPELL_CIRCLE_OF_DEATH, 12);
  spell_assignment(CLASS_SORCERER, SPELL_UNDEATH_TO_DEATH, 12);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_HUMAN_POTENTIAL, 12);
  spell_assignment(CLASS_SORCERER, SPELL_GREATER_BLACK_TENTACLES, 12);
  /*              class num      spell                   level acquired */
  /* 7th circle */
  spell_assignment(CLASS_SORCERER, SPELL_MISSILE_STORM, 14);
  spell_assignment(CLASS_SORCERER, SPELL_GRASPING_HAND, 14);
  spell_assignment(CLASS_SORCERER, SPELL_SUMMON_CREATURE_7, 14);
  spell_assignment(CLASS_SORCERER, SPELL_CONTROL_WEATHER, 14);
  spell_assignment(CLASS_SORCERER, SPELL_POWER_WORD_BLIND, 14);
  spell_assignment(CLASS_SORCERER, SPELL_WAVES_OF_EXHAUSTION, 14);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_HOLD_PERSON, 14);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_FLY, 14);
  spell_assignment(CLASS_SORCERER, SPELL_DISPLACEMENT, 14);
  spell_assignment(CLASS_SORCERER, SPELL_PRISMATIC_SPRAY, 14);
  spell_assignment(CLASS_SORCERER, SPELL_DETECT_POISON, 14);
  spell_assignment(CLASS_SORCERER, SPELL_POWER_WORD_STUN, 14);
  spell_assignment(CLASS_SORCERER, SPELL_PROTECT_FROM_SPELLS, 14);
  spell_assignment(CLASS_SORCERER, SPELL_THUNDERCLAP, 14);
  spell_assignment(CLASS_SORCERER, SPELL_SPELL_MANTLE, 14);
  spell_assignment(CLASS_SORCERER, SPELL_TELEPORT, 14);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_WISDOM, 14);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_CHARISMA, 14);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_CUNNING, 14);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_FALSE_LIFE, 14);
  spell_assignment(CLASS_SORCERER, SPELL_FINGER_OF_DEATH, 14);
  /*              class num      spell                   level acquired */
  /* 8th circle */
  spell_assignment(CLASS_SORCERER, SPELL_CLENCHED_FIST, 16);
  spell_assignment(CLASS_SORCERER, SPELL_CHAIN_LIGHTNING, 16);
  spell_assignment(CLASS_SORCERER, SPELL_INCENDIARY_CLOUD, 16);
  spell_assignment(CLASS_SORCERER, SPELL_SUMMON_CREATURE_8, 16);
  spell_assignment(CLASS_SORCERER, SPELL_HORRID_WILTING, 16);
  spell_assignment(CLASS_SORCERER, SPELL_GREATER_ANIMATION, 16);
  spell_assignment(CLASS_SORCERER, SPELL_IRRESISTIBLE_DANCE, 16);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_DOMINATION, 16);
  spell_assignment(CLASS_SORCERER, SPELL_SCINT_PATTERN, 16);
  spell_assignment(CLASS_SORCERER, SPELL_REFUGE, 16);
  spell_assignment(CLASS_SORCERER, SPELL_BANISH, 16);
  spell_assignment(CLASS_SORCERER, SPELL_SUNBURST, 16);
  spell_assignment(CLASS_SORCERER, SPELL_SPELL_TURNING, 16);
  spell_assignment(CLASS_SORCERER, SPELL_MIND_BLANK, 16);
  spell_assignment(CLASS_SORCERER, SPELL_IRONSKIN, 16);
  spell_assignment(CLASS_SORCERER, SPELL_PORTAL, 16);
  /*              class num      spell                   level acquired */
  /* 9th circle */
  spell_assignment(CLASS_SORCERER, SPELL_METEOR_SWARM, 18);
  spell_assignment(CLASS_SORCERER, SPELL_BLADE_OF_DISASTER, 18);
  spell_assignment(CLASS_SORCERER, SPELL_SUMMON_CREATURE_9, 18);
#ifndef CAMPAIGN_FR
  spell_assignment(CLASS_SORCERER, SPELL_GATE, 18);
#endif
  spell_assignment(CLASS_SORCERER, SPELL_ENERGY_DRAIN, 18);
  spell_assignment(CLASS_SORCERER, SPELL_WAIL_OF_THE_BANSHEE, 18);
  spell_assignment(CLASS_SORCERER, SPELL_POWER_WORD_KILL, 18);
  spell_assignment(CLASS_SORCERER, SPELL_ENFEEBLEMENT, 18);
  spell_assignment(CLASS_SORCERER, SPELL_WEIRD, 18);
  spell_assignment(CLASS_SORCERER, SPELL_SHADOW_SHIELD, 18);
  spell_assignment(CLASS_SORCERER, SPELL_PRISMATIC_SPHERE, 18);
  spell_assignment(CLASS_SORCERER, SPELL_IMPLODE, 18);
  spell_assignment(CLASS_SORCERER, SPELL_TIMESTOP, 18);
  spell_assignment(CLASS_SORCERER, SPELL_GREATER_SPELL_MANTLE, 18);
  spell_assignment(CLASS_SORCERER, SPELL_POLYMORPH, 18);
  spell_assignment(CLASS_SORCERER, SPELL_MASS_ENHANCE, 18);
  /*epic*/
  spell_assignment(CLASS_SORCERER, SPELL_MUMMY_DUST, 21);
  spell_assignment(CLASS_SORCERER, SPELL_DRAGON_KNIGHT, 21);
  spell_assignment(CLASS_SORCERER, SPELL_GREATER_RUIN, 21);
  spell_assignment(CLASS_SORCERER, SPELL_HELLBALL, 21);
  spell_assignment(CLASS_SORCERER, SPELL_EPIC_MAGE_ARMOR, 21);
  spell_assignment(CLASS_SORCERER, SPELL_EPIC_WARDING, 21);
  /* PREREQS here */
  /*****/
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  // assign_feat_spell_slots(CLASS_SORCERER);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number   name      abrv   clr-abrv     menu-name*/
  classo(CLASS_PALADIN, "paladin", "Pal", "\tWPal\tn", "p) \tWPaladin\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         -1, N, N, H, 10, 0, 1, 2, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Charisma, Con for survivability, Str for combat",
         /*descrip*/ "Through a select, worthy few shines the power of the divine. "
                     "Called paladins, these noble souls dedicate their swords and lives to the "
                     "battle against evil. Knights, crusaders, and law-bringers, paladins seek not "
                     "just to spread divine justice but to embody the teachings of the virtuous "
                     "deities they serve. In pursuit of their lofty goals, they adhere to ironclad "
                     "laws of morality and discipline. As reward for their righteousness, these "
                     "holy champions are blessed with boons to aid them in their quests: powers "
                     "to banish evil, heal the innocent, and inspire the faithful. Although their "
                     "convictions might lead them into conflict with the very souls they would "
                     "save, paladins weather endless challenges of faith and dark temptations, "
                     "risking their lives to do right and fighting to bring about a brighter "
                     "future.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_PALADIN, B, B, G, B, B);
  assign_class_abils(CLASS_PALADIN, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CA, CA, CA, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CA, CA, CA, CC, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CC, CC, CC, CA, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CC, CC);
  assign_class_titles(CLASS_PALADIN,                    /* class number */
                      "",                               /* <= 4  */
                      "the Initiated",                  /* <= 9  */
                      "the Accepted",                   /* <= 14 */
                      "the Hand of Mercy",              /* <= 19 */
                      "the Sword of Justice",           /* <= 24 */
                      "who Walks in the Light",         /* <= 29 */
                      "the Defender of the Faith",      /* <= 30 */
                      "the Immortal Justicar",          /* <= LVL_IMMORT */
                      "the Immortal Sword of Light",    /* <= LVL_STAFF */
                      "the Immortal Hammer of Justice", /* <= LVL_GRSTAFF */
                      "the Paladin"                     /* default */
  );
  /* feat assignment */
  /*              class num     feat                            cfeat lvl stack */
  feat_assignment(CLASS_PALADIN, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_PALADIN, FEAT_ARMOR_PROFICIENCY_HEAVY, Y, 1, N);
  feat_assignment(CLASS_PALADIN, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_PALADIN, FEAT_ARMOR_PROFICIENCY_MEDIUM, Y, 1, N);
  feat_assignment(CLASS_PALADIN, FEAT_ARMOR_PROFICIENCY_SHIELD, Y, 1, N);
  feat_assignment(CLASS_PALADIN, FEAT_MARTIAL_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_PALADIN, FEAT_AURA_OF_GOOD, Y, 1, N);
  feat_assignment(CLASS_PALADIN, FEAT_DETECT_EVIL, Y, 1, N);
  feat_assignment(CLASS_PALADIN, FEAT_SMITE_EVIL, Y, 1, Y);
  feat_assignment(CLASS_PALADIN, FEAT_DIVINE_GRACE, Y, 2, N);
  feat_assignment(CLASS_PALADIN, FEAT_LAYHANDS, Y, 3, N);
  feat_assignment(CLASS_PALADIN, FEAT_CHANNEL_ENERGY, Y, 4, N);
  feat_assignment(CLASS_PALADIN, FEAT_AURA_OF_COURAGE, Y, 4, N);
  feat_assignment(CLASS_PALADIN, FEAT_DIVINE_HEALTH, Y, 5, N);
  /* bonus feat - mounted combat 5 */
  feat_assignment(CLASS_PALADIN, FEAT_MOUNTED_COMBAT, Y, 5, N);
  feat_assignment(CLASS_PALADIN, FEAT_SMITE_EVIL, Y, 5, Y);
  /* bonus feat - ride by attack 6 */
  feat_assignment(CLASS_PALADIN, FEAT_RIDE_BY_ATTACK, Y, 6, N);
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 6, Y);
  feat_assignment(CLASS_PALADIN, FEAT_CALL_MOUNT, Y, 7, N);
  feat_assignment(CLASS_PALADIN, FEAT_DIVINE_BOND, Y, 8, N);
  /* bonus feat - spirited charge 9 */
  feat_assignment(CLASS_PALADIN, FEAT_SPIRITED_CHARGE, Y, 9, N);
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 9, Y);
  feat_assignment(CLASS_PALADIN, FEAT_SMITE_EVIL, Y, 10, Y);
  feat_assignment(CLASS_PALADIN, FEAT_AURA_OF_JUSTICE, Y, 11, Y);
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 12, Y);
  /* bonus feat - mounted archery 13 */
  feat_assignment(CLASS_PALADIN, FEAT_MOUNTED_ARCHERY, Y, 13, N);
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 14, Y);
  feat_assignment(CLASS_PALADIN, FEAT_AURA_OF_FAITH, Y, 14, Y);
  feat_assignment(CLASS_PALADIN, FEAT_SMITE_EVIL, Y, 15, Y);
  feat_assignment(CLASS_PALADIN, FEAT_AURA_OF_RIGHTEOUSNESS, Y, 17, Y);
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 18, Y);
  /* bonus feat - glorious rider 19 */
  feat_assignment(CLASS_PALADIN, FEAT_GLORIOUS_RIDER, Y, 19, N);
  feat_assignment(CLASS_PALADIN, FEAT_SMITE_EVIL, Y, 19, Y);
  feat_assignment(CLASS_PALADIN, FEAT_HOLY_WARRIOR, Y, 20, Y);
  /* spell circles */
  feat_assignment(CLASS_PALADIN, FEAT_PALADIN_1ST_CIRCLE, Y, 6, N);
  feat_assignment(CLASS_PALADIN, FEAT_PALADIN_2ND_CIRCLE, Y, 10, N);
  feat_assignment(CLASS_PALADIN, FEAT_PALADIN_3RD_CIRCLE, Y, 12, N);
  feat_assignment(CLASS_PALADIN, FEAT_PALADIN_4TH_CIRCLE, Y, 15, N);
  /*epic*/
  /* bonus epic feat - legendary rider 21 */
  feat_assignment(CLASS_PALADIN, FEAT_LEGENDARY_RIDER, Y, 21, N);
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 22, Y);
  feat_assignment(CLASS_PALADIN, FEAT_SMITE_EVIL, Y, 25, Y);
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 26, Y);
  /* bonus epic feat - epic mount 27 */
  feat_assignment(CLASS_PALADIN, FEAT_EPIC_MOUNT, Y, 27, N);
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 30, Y);
  feat_assignment(CLASS_PALADIN, FEAT_SMITE_EVIL, Y, 30, Y);
  feat_assignment(CLASS_PALADIN, FEAT_HOLY_CHAMPION, Y, 30, Y);
  /* paladin has no class feats */
  /**** spell assign ****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  spell_assignment(CLASS_PALADIN, SPELL_CURE_LIGHT, 6);
  spell_assignment(CLASS_PALADIN, SPELL_ENDURANCE, 6);
  spell_assignment(CLASS_PALADIN, SPELL_SHIELD_OF_FAITH, 6);
  spell_assignment(CLASS_PALADIN, SPELL_DIVINE_FAVOR, 6);
  spell_assignment(CLASS_PALADIN, SPELL_ENDURE_ELEMENTS, 6);
  spell_assignment(CLASS_PALADIN, SPELL_PROT_FROM_EVIL, 6);
  spell_assignment(CLASS_PALADIN, SPELL_RESISTANCE, 6);
  spell_assignment(CLASS_PALADIN, SPELL_LESSER_RESTORATION, 6);
  spell_assignment(CLASS_PALADIN, SPELL_HEDGING_WEAPONS, 6);
  spell_assignment(CLASS_PALADIN, SPELL_HONEYED_TONGUE, 6);
  spell_assignment(CLASS_PALADIN, SPELL_SHIELD_OF_FORTIFICATION, 6);
  spell_assignment(CLASS_PALADIN, SPELL_STUNNING_BARRIER, 6);
  spell_assignment(CLASS_PALADIN, SPELL_SUN_METAL, 6);
  spell_assignment(CLASS_PALADIN, SPELL_TACTICAL_ACUMEN, 6);
  spell_assignment(CLASS_PALADIN, SPELL_VEIL_OF_POSITIVE_ENERGY, 6);
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_PALADIN, SPELL_CREATE_FOOD, 10);
  spell_assignment(CLASS_PALADIN, SPELL_CREATE_WATER, 10);
  spell_assignment(CLASS_PALADIN, SPELL_DETECT_POISON, 10);
  spell_assignment(CLASS_PALADIN, SPELL_CURE_MODERATE, 10);
  spell_assignment(CLASS_PALADIN, SPELL_STRENGTH, 10);
  spell_assignment(CLASS_PALADIN, SPELL_CHARISMA, 10);
  spell_assignment(CLASS_PALADIN, SPELL_WISDOM, 10);
  spell_assignment(CLASS_PALADIN, SPELL_RESIST_ENERGY, 10);
  spell_assignment(CLASS_PALADIN, SPELL_BESTOW_WEAPON_PROFICIENCY, 10);
  spell_assignment(CLASS_PALADIN, SPELL_EFFORTLESS_ARMOR, 10);
  spell_assignment(CLASS_PALADIN, SPELL_FIRE_OF_ENTANGLEMENT, 10);
  spell_assignment(CLASS_PALADIN, SPELL_LIFE_SHIELD, 10);
  spell_assignment(CLASS_PALADIN, SPELL_LITANY_OF_DEFENSE, 10);
  spell_assignment(CLASS_PALADIN, SPELL_LITANY_OF_RIGHTEOUSNESS, 10);
  spell_assignment(CLASS_PALADIN, SPELL_REMOVE_PARALYSIS, 10);
  spell_assignment(CLASS_PALADIN, SPELL_UNDETECTABLE_ALIGNMENT, 10);
  spell_assignment(CLASS_PALADIN, SPELL_WEAPON_OF_AWE, 10);
  spell_assignment(CLASS_PALADIN, SPELL_BLINDING_RAY, 10);
  spell_assignment(CLASS_PALADIN, SPELL_HOLY_JAVELIN, 10);
  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_PALADIN, SPELL_DETECT_ALIGN, 12);
  spell_assignment(CLASS_PALADIN, SPELL_CURE_BLIND, 12);
  spell_assignment(CLASS_PALADIN, SPELL_BLESS, 12);
  spell_assignment(CLASS_PALADIN, SPELL_CURE_SERIOUS, 12);
  spell_assignment(CLASS_PALADIN, SPELL_HEAL_MOUNT, 12);
  spell_assignment(CLASS_PALADIN, SPELL_GREATER_MAGIC_WEAPON, 12);
  spell_assignment(CLASS_PALADIN, SPELL_COMMUNAL_RESIST_ENERGY, 12);
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_PALADIN, SPELL_AID, 15);
  spell_assignment(CLASS_PALADIN, SPELL_INFRAVISION, 15);
  spell_assignment(CLASS_PALADIN, SPELL_REMOVE_CURSE, 15);
  spell_assignment(CLASS_PALADIN, SPELL_REMOVE_POISON, 15);
  spell_assignment(CLASS_PALADIN, SPELL_CURE_CRITIC, 15);
  spell_assignment(CLASS_PALADIN, SPELL_HOLY_SWORD, 15);
  spell_assignment(CLASS_PALADIN, SPELL_STONESKIN, 15);

  /* class prerequisites */
  class_prereq_align(CLASS_PALADIN, LAWFUL_GOOD);
  /*****/
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  // assign_feat_spell_slots(CLASS_PALADIN);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number   name      abrv   clr-abrv     menu-name*/
  classo(CLASS_BLACKGUARD, "blackguard", "BkG", "\tDBkG\tn", "z) \tDBlackguard\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         -1, N, N, H, 10, 0, 1, 2, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Charisma, Con for survivability, Str for combat",
         /*descrip*/ "Blackguards, also referred to as antipaladins, are the quintessential "
                     "champions of evil in Faerun. They lead armies of dread forces such as undead, "
                     "fiends, and other extra-planar beings, often in the name of the more malevolent "
                     "deities. These individuals had the reputation as some of most reviled villains. "
                     "They are just as equally feared as they are despised by the free folk of the "
                     "Realms. They acted as killers, led as commanders, and even served as agents "
                     "for more forces that are even more malignant than they. They accomplished their "
                     "goals by any means necessary, whether through subterfuge, dark magic, anarchic "
                     "destruction, or overwhelming force. Blackguards possess distinct auras of evil "
                     "and despair, carrying blessings bestowed upon them by the forces of darkness. "
                     "They are particularly adept at instilling fear in their foes. They are quite adept "
                     "at readily uncovering good beings, and can also smite them in battle. Blackguards "
                     "can carry out smiting more often as the longer they carried out their dark deeds.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_BLACKGUARD, B, B, G, B, B);
  assign_class_abils(CLASS_BLACKGUARD, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CA, CC, CA, CA, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CA, CA, CA, CC, CC, CA,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CA, CC, CA, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CC, CC);
  assign_class_titles(CLASS_BLACKGUARD,          /* class number */
                      "",                        /* <= 4  */
                      "the Novice Blackguard",   /* <= 9  */
                      "the Adept Blackguard",   /* <= 14 */
                      "the Veteran Blackguard",  /* <= 19 */
                      "the Master Blackguard",   /* <= 24 */
                      "the Champion Blackguard", /* <= 29 */
                      "the Chosen Blackguard",   /* <= 30 */
                      "the Immortal Blackguard", /* <= LVL_IMMORT */
                      "the Immortal Blackguard", /* <= LVL_STAFF */
                      "the Immortal Blackguard", /* <= LVL_GRSTAFF */
                      "the Blackguard"           /* default */
  );
  /* feat assignment */
  /*              class num     feat                            cfeat lvl stack */
  feat_assignment(CLASS_BLACKGUARD, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_ARMOR_PROFICIENCY_HEAVY, Y, 1, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_ARMOR_PROFICIENCY_MEDIUM, Y, 1, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_ARMOR_PROFICIENCY_SHIELD, Y, 1, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_MARTIAL_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_AURA_OF_EVIL, Y, 1, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_DETECT_GOOD, Y, 1, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_SMITE_GOOD, Y, 1, Y);

  feat_assignment(CLASS_BLACKGUARD, FEAT_UNHOLY_RESILIENCE, Y, 2, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_TOUCH_OF_CORRUPTION, Y, 2, N);

  feat_assignment(CLASS_BLACKGUARD, FEAT_AURA_OF_COWARDICE, Y, 3, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_PLAGUE_BRINGER, Y, 3, N);
  // cruelty slot - 3

  feat_assignment(CLASS_BLACKGUARD, FEAT_CHANNEL_ENERGY, Y, 4, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_SMITE_GOOD, Y, 4, Y);

  feat_assignment(CLASS_BLACKGUARD, FEAT_FIENDISH_BOON, Y, 5, N);

  // cruelty slot - 6

  feat_assignment(CLASS_BLACKGUARD, FEAT_SMITE_GOOD, Y, 7, Y);

  feat_assignment(CLASS_BLACKGUARD, FEAT_AURA_OF_DESPAIR, Y, 8, N);

  // cruelty slot - 9

  feat_assignment(CLASS_BLACKGUARD, FEAT_SMITE_GOOD, Y, 10, Y);

  feat_assignment(CLASS_BLACKGUARD, FEAT_AURA_OF_VENGEANCE, Y, 11, N);

  // cruelty slot - 12

  feat_assignment(CLASS_BLACKGUARD, FEAT_SMITE_GOOD, Y, 13, Y);

  feat_assignment(CLASS_BLACKGUARD, FEAT_AURA_OF_SIN, Y, 14, Y);

  // cruelty slot - 15

  feat_assignment(CLASS_BLACKGUARD, FEAT_SMITE_GOOD, Y, 16, Y);

  feat_assignment(CLASS_BLACKGUARD, FEAT_AURA_OF_DEPRAVITY, Y, 14, Y);

  // cruelty slot - 18

  feat_assignment(CLASS_BLACKGUARD, FEAT_SMITE_GOOD, Y, 19, Y);

  feat_assignment(CLASS_BLACKGUARD, FEAT_UNHOLY_WARRIOR, Y, 20, Y);

  // epic

  // cruelty slot - 21

  feat_assignment(CLASS_BLACKGUARD, FEAT_SMITE_GOOD, Y, 22, Y);

  // cruelty slot - 24

  feat_assignment(CLASS_BLACKGUARD, FEAT_SMITE_GOOD, Y, 25, Y);

  // cruelty slot - 27

  feat_assignment(CLASS_BLACKGUARD, FEAT_SMITE_GOOD, Y, 28, Y);

  // cruelty slot - 30
  feat_assignment(CLASS_BLACKGUARD, FEAT_UNHOLY_CHAMPION, Y, 30, Y);

  /* spell circles */
  feat_assignment(CLASS_BLACKGUARD, FEAT_BLACKGUARD_1ST_CIRCLE, Y, 6, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_BLACKGUARD_2ND_CIRCLE, Y, 10, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_BLACKGUARD_3RD_CIRCLE, Y, 12, N);
  feat_assignment(CLASS_BLACKGUARD, FEAT_BLACKGUARD_4TH_CIRCLE, Y, 15, N);
  /**** spell assign ****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  // spell_assignment(CLASS_BLACKGUARD, SPELL_COMMAND, 6);
  spell_assignment(CLASS_BLACKGUARD, SPELL_DETECT_POISON, 6);
  spell_assignment(CLASS_BLACKGUARD, SPELL_DOOM, 6);
  spell_assignment(CLASS_BLACKGUARD, SPELL_CAUSE_LIGHT_WOUNDS, 6);
  spell_assignment(CLASS_BLACKGUARD, SPELL_PROT_FROM_GOOD, 6);
  spell_assignment(CLASS_BLACKGUARD, SPELL_SUMMON_CREATURE_3, 6);
  spell_assignment(CLASS_BLACKGUARD, SPELL_ENDURANCE, 6);
  spell_assignment(CLASS_BLACKGUARD, SPELL_NEGATIVE_ENERGY_RAY, 6);
  spell_assignment(CLASS_BLACKGUARD, SPELL_ENDURE_ELEMENTS, 6);
  spell_assignment(CLASS_BLACKGUARD, SPELL_HEDGING_WEAPONS, 6);
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_BLACKGUARD, SPELL_BLINDNESS, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_DEAFNESS, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_STRENGTH, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_DARKNESS, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_INFRAVISION, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_CHARISMA, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_HOLD_PERSON, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_INVISIBLE, 10);
  // spell_assignment(CLASS_BLACKGUARD, SPELL_SILENCE, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_SUMMON_CREATURE_4, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_BESTOW_WEAPON_PROFICIENCY, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_LITANY_OF_DEFENSE, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_UNDETECTABLE_ALIGNMENT, 10);
  spell_assignment(CLASS_BLACKGUARD, SPELL_SILENCE, 10);

  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_BLACKGUARD, SPELL_ANIMATE_DEAD, 12);
  spell_assignment(CLASS_BLACKGUARD, SPELL_CURSE, 12);
  spell_assignment(CLASS_BLACKGUARD, SPELL_CONTAGION, 12);
  spell_assignment(CLASS_BLACKGUARD, SPELL_DISPEL_MAGIC, 12);
  spell_assignment(CLASS_BLACKGUARD, SPELL_CAUSE_MODERATE_WOUNDS, 12);
  spell_assignment(CLASS_BLACKGUARD, SPELL_CIRCLE_A_GOOD, 12);
  spell_assignment(CLASS_BLACKGUARD, SPELL_SUMMON_CREATURE_6, 12);
  spell_assignment(CLASS_BLACKGUARD, SPELL_VAMPIRIC_TOUCH, 12);
  spell_assignment(CLASS_BLACKGUARD, SPELL_GREATER_MAGIC_WEAPON, 12);

  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_BLACKGUARD, SPELL_DISPEL_GOOD, 15);
  spell_assignment(CLASS_BLACKGUARD, SPELL_FEAR, 15);
  spell_assignment(CLASS_BLACKGUARD, SPELL_CAUSE_SERIOUS_WOUNDS, 15);
  spell_assignment(CLASS_BLACKGUARD, SPELL_GREATER_INVIS, 15);
  spell_assignment(CLASS_BLACKGUARD, SPELL_POISON, 15);
  spell_assignment(CLASS_BLACKGUARD, SPELL_SUMMON_CREATURE_8, 15);
  spell_assignment(CLASS_BLACKGUARD, SPELL_UNHOLY_SWORD, 15);
  spell_assignment(CLASS_BLACKGUARD, SPELL_STONESKIN, 15);

  /* class prerequisites */
  class_prereq_align(CLASS_BLACKGUARD, LAWFUL_EVIL);
  class_prereq_align(CLASS_BLACKGUARD, NEUTRAL_EVIL);
  class_prereq_align(CLASS_BLACKGUARD, CHAOTIC_EVIL);
  /*****/
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  // assign_feat_spell_slots(CLASS_PALADIN);
  /**************************************************************************/

  /****************************************************************************/
  /*     class-number  name      abrv   clr-abrv     menu-name*/
  classo(CLASS_RANGER, "ranger", "Ran", "\tYRan\tn", "r) \tYRanger\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp */
         -1, N, N, H, 10, 0, 3, 4, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Dexterity or Str, Con for survivability, they need a little Wis for spellcasting",
         /*descrip*/ "For those who relish the thrill of the hunt, there are only "
                     "predators and prey. Be they scouts, trackers, or bounty hunters, rangers share "
                     "much in common: unique mastery of specialized weapons, skill at stalking even "
                     "the most elusive game, and the expertise to defeat a wide range of quarries. "
                     "Knowledgeable, patient, and skilled hunters, these rangers hound man, beast, "
                     "and monster alike, gaining insight into the way of the predator, skill in "
                     "varied environments, and ever more lethal martial prowess. While some track "
                     "man-eating creatures to protect the frontier, others pursue more cunning "
                     "game—even fugitives among their own people.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_RANGER, G, B, B, B, B);
  assign_class_abils(CLASS_RANGER, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CA, CA, CA, CC, CA, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CA, CA, CA, CA, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CA, CA, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CA, CA, CC, CC);
  assign_class_titles(CLASS_RANGER,             /* class number */
                      "",                       /* <= 4  */
                      "the Dirt-watcher",       /* <= 9  */
                      "the Hunter",             /* <= 14 */
                      "the Tracker",            /* <= 19 */
                      "the Finder of Prey",     /* <= 24 */
                      "the Hidden Stalker",     /* <= 29 */
                      "the Great Seeker",       /* <= 30 */
                      "the Avatar of the Wild", /* <= LVL_IMMORT */
                      "the Wrath of the Wild",  /* <= LVL_STAFF */
                      "the Cyclone of Nature",  /* <= LVL_GRSTAFF */
                      "the Ranger"              /* default */
  );
  /* feat assignment */
  /*              class num     feat                               cfeat lvl stack */
  feat_assignment(CLASS_RANGER, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_RANGER, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_RANGER, FEAT_ARMOR_PROFICIENCY_MEDIUM, Y, 1, N);
  feat_assignment(CLASS_RANGER, FEAT_ARMOR_PROFICIENCY_SHIELD, Y, 1, N);
  feat_assignment(CLASS_RANGER, FEAT_MARTIAL_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_RANGER, FEAT_FAVORED_ENEMY_AVAILABLE, Y, 1, Y);
  feat_assignment(CLASS_RANGER, FEAT_WILD_EMPATHY, Y, 2, N);
  /*CM*/
  feat_assignment(CLASS_RANGER, FEAT_DUAL_WEAPON_FIGHTING, Y, 3, N);
  /*CM*/
  feat_assignment(CLASS_RANGER, FEAT_POINT_BLANK_SHOT, Y, 4, N);
  feat_assignment(CLASS_RANGER, FEAT_ENDURANCE, Y, 4, N);
  feat_assignment(CLASS_RANGER, FEAT_ANIMAL_COMPANION, Y, 4, N);
  feat_assignment(CLASS_RANGER, FEAT_FAVORED_ENEMY_AVAILABLE, Y, 5, Y);
  /*CM*/
  feat_assignment(CLASS_RANGER, FEAT_IMPROVED_DUAL_WEAPON_FIGHTING, Y, 6, N);
  /*CM*/
  feat_assignment(CLASS_RANGER, FEAT_RAPID_SHOT, Y, 7, N);
  feat_assignment(CLASS_RANGER, FEAT_WOODLAND_STRIDE, Y, 8, N);
  feat_assignment(CLASS_RANGER, FEAT_TRACK, Y, 9, N);
  feat_assignment(CLASS_RANGER, FEAT_EVASION, Y, 10, N);
  feat_assignment(CLASS_RANGER, FEAT_FAVORED_ENEMY_AVAILABLE, Y, 10, Y);
  /*CM*/
  feat_assignment(CLASS_RANGER, FEAT_GREATER_DUAL_WEAPON_FIGHTING, Y, 11, N);
  /*CM*/
  feat_assignment(CLASS_RANGER, FEAT_MANYSHOT, Y, 12, N);
  feat_assignment(CLASS_RANGER, FEAT_CAMOUFLAGE, Y, 13, N);
  feat_assignment(CLASS_RANGER, FEAT_SWIFT_TRACKER, Y, 14, N);
  feat_assignment(CLASS_RANGER, FEAT_FAVORED_ENEMY_AVAILABLE, Y, 15, Y);
  feat_assignment(CLASS_RANGER, FEAT_HIDE_IN_PLAIN_SIGHT, Y, 17, N);
  feat_assignment(CLASS_RANGER, FEAT_FAVORED_ENEMY_AVAILABLE, Y, 20, Y);
  /* spell circles */
  feat_assignment(CLASS_RANGER, FEAT_RANGER_1ST_CIRCLE, Y, 6, N);
  feat_assignment(CLASS_RANGER, FEAT_RANGER_2ND_CIRCLE, Y, 10, N);
  feat_assignment(CLASS_RANGER, FEAT_RANGER_3RD_CIRCLE, Y, 12, N);
  feat_assignment(CLASS_RANGER, FEAT_RANGER_4TH_CIRCLE, Y, 15, N);
  /* epic */
  /*CM*/
  feat_assignment(CLASS_RANGER, FEAT_PERFECT_DUAL_WEAPON_FIGHTING, Y, 21, N);
  /*CM*/
  feat_assignment(CLASS_RANGER, FEAT_EPIC_MANYSHOT, Y, 22, N);
  /* bonus feat - improved evasion 23 */
  feat_assignment(CLASS_RANGER, FEAT_IMPROVED_EVASION, Y, 23, N);
  feat_assignment(CLASS_RANGER, FEAT_FAVORED_ENEMY_AVAILABLE, Y, 25, Y);
  /* bonus feat - bane of enemies 26 */
  feat_assignment(CLASS_RANGER, FEAT_BANE_OF_ENEMIES, Y, 26, N);
  /* bonus feat - epic favored enemy 29 */
  feat_assignment(CLASS_RANGER, FEAT_EPIC_FAVORED_ENEMY, Y, 29, N);
  feat_assignment(CLASS_RANGER, FEAT_FAVORED_ENEMY_AVAILABLE, Y, 30, Y);
  /* no classfeats */
  /**** spell assignment *****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  spell_assignment(CLASS_RANGER, SPELL_CURE_LIGHT, 6);
  spell_assignment(CLASS_RANGER, SPELL_CHARM_ANIMAL, 6);
  spell_assignment(CLASS_RANGER, SPELL_FAERIE_FIRE, 6);
  spell_assignment(CLASS_RANGER, SPELL_JUMP, 6);
  spell_assignment(CLASS_RANGER, SPELL_MAGIC_FANG, 6);
  spell_assignment(CLASS_RANGER, SPELL_SUMMON_NATURES_ALLY_1, 6);
  spell_assignment(CLASS_RANGER, SPELL_ENTANGLE, 6);
  spell_assignment(CLASS_RANGER, SPELL_SUN_METAL, 6);
  spell_assignment(CLASS_RANGER, SPELL_VIGORIZE_LIGHT, 6);
  spell_assignment(CLASS_RANGER, SPELL_ANT_HAUL, 6);
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_RANGER, SPELL_ENDURANCE, 10);
  spell_assignment(CLASS_RANGER, SPELL_BARKSKIN, 10);
  spell_assignment(CLASS_RANGER, SPELL_GRACE, 10);
  spell_assignment(CLASS_RANGER, SPELL_HOLD_ANIMAL, 10);
  spell_assignment(CLASS_RANGER, SPELL_WISDOM, 10);
  spell_assignment(CLASS_RANGER, SPELL_STRENGTH, 10);
  spell_assignment(CLASS_RANGER, SPELL_SUMMON_NATURES_ALLY_2, 10);
  spell_assignment(CLASS_RANGER, SPELL_EFFORTLESS_ARMOR, 10);
  spell_assignment(CLASS_RANGER, SPELL_PROTECTION_FROM_ENERGY, 10);
  spell_assignment(CLASS_RANGER, SPELL_WIND_WALL, 10);
  spell_assignment(CLASS_RANGER, SPELL_VIGORIZE_SERIOUS, 10);
  spell_assignment(CLASS_RANGER, SPELL_MASS_ANT_HAUL, 10);
  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_RANGER, SPELL_SPIKE_GROWTH, 12);
  spell_assignment(CLASS_RANGER, SPELL_GREATER_MAGIC_FANG, 12);
  spell_assignment(CLASS_RANGER, SPELL_CONTAGION, 12);
  spell_assignment(CLASS_RANGER, SPELL_CURE_MODERATE, 12);
  spell_assignment(CLASS_RANGER, SPELL_SUMMON_NATURES_ALLY_3, 12);
  spell_assignment(CLASS_RANGER, SPELL_REMOVE_DISEASE, 12);
  spell_assignment(CLASS_RANGER, SPELL_REMOVE_POISON, 12);
  spell_assignment(CLASS_RANGER, SPELL_COMMUNAL_PROTECTION_FROM_ENERGY, 12);
  spell_assignment(CLASS_RANGER, SPELL_VIGORIZE_CRITICAL, 12);
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_RANGER, SPELL_SUMMON_NATURES_ALLY_4, 15);
  spell_assignment(CLASS_RANGER, SPELL_FREE_MOVEMENT, 15);
  spell_assignment(CLASS_RANGER, SPELL_DISPEL_MAGIC, 15);
  spell_assignment(CLASS_RANGER, SPELL_CURE_SERIOUS, 15);
  spell_assignment(CLASS_RANGER, SPELL_GROUP_VIGORIZE, 15);
  /* no prereqs! */
  /*****/
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  // assign_feat_spell_slots(CLASS_RANGER);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name   abrv   clr-abrv     menu-name*/
  classo(CLASS_BARD, "bard", "Bar", "\tCBar\tn", "a) \tCBard\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp */
         -1, N, N, M, 8, 0, 2, 6, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Charisma, Int for skills, Con/Dex for survivability",
         /*descrip*/ "Untold wonders and secrets exist for those skillful enough to "
                     "discover them. Through cleverness, talent, and magic, these cunning few unravel "
                     "the wiles of the world, becoming adept in the arts of persuasion, manipulation, "
                     "and inspiration. Typically masters of one or many forms of artistry, bards "
                     "possess an uncanny ability to know more than they should and use what they "
                     "learn to keep themselves and their allies ever one step ahead of danger. Bards "
                     "are quick-witted and captivating, and their skills might lead them down many "
                     "paths, be they gamblers or jacks-of-all-trades, scholars or performers, leaders "
                     "or scoundrels, or even all of the above. For bards, every day brings its own "
                     "opportunities, adventures, and challenges, and only by bucking the odds, knowing "
                     "the most, and being the best might they claim the treasures of each.  "
                     "Bards get a bonus feat every 3 levels.");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_BARD, B, G, G, B, B);
  assign_class_abils(CLASS_BARD, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CA, CA, CA, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CA, CC, CA, CA, CA, CA, CA, CA,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CC, CA, CA, CC, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CA);
  assign_class_titles(CLASS_BARD,                /* class number */
                      "",                        /* <= 4  */
                      "the Melodious",           /* <= 9  */
                      "the Hummer of Harmonies", /* <= 14 */
                      "Weaver of Song",          /* <= 19 */
                      "Keeper of Chords",        /* <= 24 */
                      "the Composer",            /* <= 29 */
                      "the Maestro",             /* <= 30 */
                      "the Immortal Songweaver", /* <= LVL_IMMORT */
                      "the Master of Sound",     /* <= LVL_STAFF */
                      "the Lord of Dance",       /* <= LVL_GRSTAFF */
                      "the Bard"                 /* default */
  );
  /* feat assignment */
  /*              class num     feat                          cfeat lvl stack */
  feat_assignment(CLASS_BARD, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_BARD, FEAT_WEAPON_PROFICIENCY_BARD, Y, 1, N);
  feat_assignment(CLASS_BARD, FEAT_WEAPON_PROFICIENCY_ROGUE, Y, 1, N);
  feat_assignment(CLASS_BARD, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_BARD, FEAT_ARMOR_PROFICIENCY_SHIELD, Y, 1, N);
  feat_assignment(CLASS_BARD, FEAT_BARDIC_MUSIC, Y, 1, N);
  feat_assignment(CLASS_BARD, FEAT_BARDIC_KNOWLEDGE, Y, 1, N);
  feat_assignment(CLASS_BARD, FEAT_COUNTERSONG, Y, 1, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_HEALING, Y, 1, N);
  feat_assignment(CLASS_BARD, FEAT_DANCE_OF_PROTECTION, Y, 2, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_FOCUSED_MIND, Y, 3, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_HEROISM, Y, 5, N);
  feat_assignment(CLASS_BARD, FEAT_ORATORY_OF_REJUVENATION, Y, 7, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_FLIGHT, Y, 9, N);
  feat_assignment(CLASS_BARD, FEAT_EFFICIENT_PERFORMANCE, Y, 10, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_REVELATION, Y, 11, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_FEAR, Y, 13, N);
  feat_assignment(CLASS_BARD, FEAT_ACT_OF_FORGETFULNESS, Y, 15, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_ROOTING, Y, 17, N);
  /* spell circles */
  feat_assignment(CLASS_BARD, FEAT_BARD_1ST_CIRCLE, Y, 1, N);
  feat_assignment(CLASS_BARD, FEAT_BARD_2ND_CIRCLE, Y, 4, N);
  feat_assignment(CLASS_BARD, FEAT_BARD_3RD_CIRCLE, Y, 7, N);
  feat_assignment(CLASS_BARD, FEAT_BARD_4TH_CIRCLE, Y, 10, N);
  feat_assignment(CLASS_BARD, FEAT_BARD_5TH_CIRCLE, Y, 13, N);
  feat_assignment(CLASS_BARD, FEAT_BARD_6TH_CIRCLE, Y, 16, N);
  /*epic*/
  feat_assignment(CLASS_BARD, FEAT_BARD_EPIC_SPELL, Y, 21, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_DRAGONS, Y, 22, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_THE_MAGI, Y, 26, N);
  /* no class feat assignments */
  /**** spell assign ****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  spell_assignment(CLASS_BARD, SPELL_HORIZIKAULS_BOOM, 1);
  spell_assignment(CLASS_BARD, SPELL_SHIELD, 1);
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_1, 1);
  spell_assignment(CLASS_BARD, SPELL_CHARM, 1);
  spell_assignment(CLASS_BARD, SPELL_ENDURE_ELEMENTS, 1);
  spell_assignment(CLASS_BARD, SPELL_PROT_FROM_EVIL, 1);
  spell_assignment(CLASS_BARD, SPELL_PROT_FROM_GOOD, 1);
  spell_assignment(CLASS_BARD, SPELL_MAGIC_MISSILE, 1);
  spell_assignment(CLASS_BARD, SPELL_CURE_LIGHT, 1);
  spell_assignment(CLASS_BARD, SPELL_RESISTANCE, 1);
  spell_assignment(CLASS_BARD, SPELL_UNDETECTABLE_ALIGNMENT, 1);
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_2, 4);
  spell_assignment(CLASS_BARD, SPELL_DEAFNESS, 4);
  spell_assignment(CLASS_BARD, SPELL_HIDEOUS_LAUGHTER, 4);
  spell_assignment(CLASS_BARD, SPELL_MIRROR_IMAGE, 4);
  spell_assignment(CLASS_BARD, SPELL_DETECT_INVIS, 4);
  spell_assignment(CLASS_BARD, SPELL_DETECT_MAGIC, 4);
  spell_assignment(CLASS_BARD, SPELL_INVISIBLE, 4);
  spell_assignment(CLASS_BARD, SPELL_ENDURANCE, 4);
  spell_assignment(CLASS_BARD, SPELL_STRENGTH, 4);
  spell_assignment(CLASS_BARD, SPELL_GRACE, 4);
  spell_assignment(CLASS_BARD, SPELL_CURE_MODERATE, 4);
  spell_assignment(CLASS_BARD, SPELL_HONEYED_TONGUE, 4);
  spell_assignment(CLASS_BARD, SPELL_TACTICAL_ACUMEN, 4);
  spell_assignment(CLASS_BARD, SPELL_SILENCE, 4);
  spell_assignment(CLASS_BARD, SPELL_GLITTERDUST, 4);
  spell_assignment(CLASS_BARD, SPELL_HUMAN_POTENTIAL, 4);
  spell_assignment(CLASS_BARD, SPELL_RAGE, 4);
  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_3, 7);
  spell_assignment(CLASS_BARD, SPELL_LIGHTNING_BOLT, 7);
  spell_assignment(CLASS_BARD, SPELL_DEEP_SLUMBER, 7);
  spell_assignment(CLASS_BARD, SPELL_HASTE, 7);
  spell_assignment(CLASS_BARD, SPELL_CIRCLE_A_EVIL, 7);
  spell_assignment(CLASS_BARD, SPELL_CIRCLE_A_GOOD, 7);
  spell_assignment(CLASS_BARD, SPELL_CUNNING, 7);
  spell_assignment(CLASS_BARD, SPELL_WISDOM, 7);
  spell_assignment(CLASS_BARD, SPELL_CHARISMA, 7);
  spell_assignment(CLASS_BARD, SPELL_CURE_SERIOUS, 7);
  spell_assignment(CLASS_BARD, SPELL_CONFUSION, 7);
  spell_assignment(CLASS_BARD, SPELL_WEAPON_OF_IMPACT, 7);
  spell_assignment(CLASS_BARD, SPELL_GASEOUS_FORM, 7);
  spell_assignment(CLASS_BARD, SPELL_CONTROL_SUMMONED_CREATURE, 7);
  spell_assignment(CLASS_BARD, SPELL_CHARM_MONSTER, 7);
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_4, 10);
  spell_assignment(CLASS_BARD, SPELL_GREATER_INVIS, 10);
  spell_assignment(CLASS_BARD, SPELL_RAINBOW_PATTERN, 10);
  spell_assignment(CLASS_BARD, SPELL_REMOVE_CURSE, 10);
  spell_assignment(CLASS_BARD, SPELL_ICE_STORM, 10);
  spell_assignment(CLASS_BARD, SPELL_CURE_CRITIC, 10);
  spell_assignment(CLASS_BARD, SPELL_SHADOW_JUMP, 10);
  /*              class num      spell                   level acquired */
  /* 5th circle */
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_5, 13);
  spell_assignment(CLASS_BARD, SPELL_ACID_SHEATH, 13);
  spell_assignment(CLASS_BARD, SPELL_CONE_OF_COLD, 13);
  spell_assignment(CLASS_BARD, SPELL_NIGHTMARE, 13);
  spell_assignment(CLASS_BARD, SPELL_MIND_FOG, 13);
  spell_assignment(CLASS_BARD, SPELL_MASS_CURE_LIGHT, 13);
  spell_assignment(CLASS_BARD, SPELL_SHADOW_WALK, 13);
  /*              class num      spell                   level acquired */
  /* 6th circle */
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_7, 16);
  spell_assignment(CLASS_BARD, SPELL_FREEZING_SPHERE, 16);
  spell_assignment(CLASS_BARD, SPELL_GREATER_HEROISM, 16);
  spell_assignment(CLASS_BARD, SPELL_MASS_CURE_MODERATE, 16);
  spell_assignment(CLASS_BARD, SPELL_STONESKIN, 16);
  spell_assignment(CLASS_BARD, SPELL_MASS_HUMAN_POTENTIAL, 16);
  /*epic*/
  spell_assignment(CLASS_BARD, SPELL_MUMMY_DUST, 21);
  spell_assignment(CLASS_BARD, SPELL_GREATER_RUIN, 21);
  /* class prerequisites */
  class_prereq_align(CLASS_BARD, NEUTRAL_GOOD);
  class_prereq_align(CLASS_BARD, TRUE_NEUTRAL);
  class_prereq_align(CLASS_BARD, NEUTRAL_EVIL);
  class_prereq_align(CLASS_BARD, CHAOTIC_EVIL);
  class_prereq_align(CLASS_BARD, CHAOTIC_GOOD);
  class_prereq_align(CLASS_BARD, CHAOTIC_NEUTRAL);
  /*****/
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  // assign_feat_spell_slots(CLASS_BARD);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_PSIONICIST, "psionicist", "Psn", "\tCPsn\tn", "f) \tCPsionicist\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         30, N, N, L, 6, 0, 1, 2, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Intelligence, Dex/Con for survivability",
         /*descrip*/ "The powers of the mind are varied and limitless, and the psion "
                     "learns how to unlock them. The psion learns to manifest psionic "
                     "powers that alter himself and the world around him. Due to the "
                     "limited powers that any one psion knows, each psion is unique in "
                     "his capabilities, as his latent abilities are drawn out and shaped "
                     "into the psionic powers that define the psion.");

  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_PSIONICIST, B, B, G, B, G);
  assign_class_abils(CLASS_PSIONICIST, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CA, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CA, CA, CC, CA, CA, CC, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CC, CC, CC, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_PSIONICIST,         /* class number */
                      "",                       /* <= 4  */
                      "the Novice of the Mind", /* <= 9  */
                      "the Adept of the Mind",  /* <= 14 */
                      "the Psionicist",         /* <= 19 */
                      "the Mind Shaper",        /* <= 24 */
                      "the Master of the Mind", /* <= 29 */
                      "the Master of the Mind", /* <= 30 */
                      "the Immortal Mind",      /* <= LVL_IMMORT */
                      "the Master of Minds",    /* <= LVL_STAFF */
                      "the Omniscient",         /* <= LVL_GRSTAFF */
                      "the Psionicist"          /* default */
  );

  /* feat assignment */
  /*              class num     feat                            cfeat lvl stack */
  feat_assignment(CLASS_PSIONICIST, FEAT_WEAPON_PROFICIENCY_PSIONICIST, Y, 1, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONIC_FOCUS, Y, 1, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONICIST_1ST_CIRCLE, Y, 1, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONICIST_2ND_CIRCLE, Y, 3, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONICIST_3RD_CIRCLE, Y, 5, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONICIST_4TH_CIRCLE, Y, 7, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_BREACH_POWER_RESISTANCE, Y, 8, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONICIST_5TH_CIRCLE, Y, 9, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONICIST_6TH_CIRCLE, Y, 11, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONICIST_7TH_CIRCLE, Y, 13, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_DOUBLE_MANIFEST, Y, 14, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONICIST_8TH_CIRCLE, Y, 15, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONICIST_9TH_CIRCLE, Y, 17, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PERPETUAL_FORESIGHT, Y, 20, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_EPIC_PSIONICS, Y, 21, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_EPIC_AUGMENTING, Y, 23, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_EPIC_PSIONICS, Y, 25, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_EPIC_AUGMENTING, Y, 27, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_EPIC_PSIONICS, Y, 29, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_MASTER_OF_THE_MIND, Y, 30, N);
  /*epic*/
  /* list of class feats */
  feat_assignment(CLASS_PSIONICIST, FEAT_COMBAT_MANIFESTATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_ALIGNED_ATTACK_GOOD, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_ALIGNED_ATTACK_EVIL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_ALIGNED_ATTACK_CHAOS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_ALIGNED_ATTACK_LAW, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_CRITICAL_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_ELEMENTAL_FOCUS_FIRE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_ELEMENTAL_FOCUS_ACID, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_ELEMENTAL_FOCUS_COLD, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_ELEMENTAL_FOCUS_ELECTRICITY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_ELEMENTAL_FOCUS_SOUND, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_POWER_PENETRATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_GREATER_POWER_PENETRATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_MIGHTY_POWER_PENETRATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_QUICK_MIND, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONIC_RECOVERY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PROFICIENT_PSIONICIST, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_ENHANCED_POWER_DAMAGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_EXPANDED_KNOWLEDGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PSIONIC_ENDOWMENT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_EMPOWERED_PSIONICS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_PROFICIENT_AUGMENTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_EXPERT_AUGMENTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_PSIONICIST, FEAT_MASTER_AUGMENTING, Y, NOASSIGN_FEAT, N);

  /**** spell assign ****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  spell_assignment(CLASS_PSIONICIST, PSIONIC_BROKER, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_CALL_TO_MIND, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_CATFALL, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_CRYSTAL_SHARD, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_DECELERATION, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_DEMORALIZE, 1);
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_ECTOPLASMIC_SHEEN, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ENERGY_RAY, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_FORCE_SCREEN, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_FORTIFY, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_INERTIAL_ARMOR, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_INEVITABLE_STRIKE, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_MIND_THRUST, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_DEFENSIVE_PRECOGNITION, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_OFFENSIVE_PRECOGNITION, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_OFFENSIVE_PRESCIENCE, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_SLUMBER, 1);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_VIGOR, 1);
  /* 2nd circle */
  spell_assignment(CLASS_PSIONICIST, PSIONIC_BESTOW_POWER, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_BIOFEEDBACK, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_BODY_EQUILIBRIUM, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_BREACH, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_CONCEALING_AMORPHA, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_CONCUSSION_BLAST, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_DETECT_HOSTILE_INTENT, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ELFSIGHT, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ENERGY_ADAPTATION_SPECIFIED, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ENERGY_PUSH, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ENERGY_STUN, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_INFLICT_PAIN, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_MENTAL_DISRUPTION, 3);
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_PSIONIC_LOCK, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_PSYCHIC_BODYGUARD, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_RECALL_AGONY, 3);
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_SHARE_PAIN, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_SWARM_OF_CRYSTALS, 3);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_THOUGHT_SHIELD, 3);
  /* 3rd circle */
  spell_assignment(CLASS_PSIONICIST, PSIONIC_BODY_ADJUSTMENT, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_CONCUSSIVE_ONSLAUGHT, 5);
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_DISPEL_PSIONICS, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ENDORPHIN_SURGE, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ENERGY_BURST, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ENERGY_RETORT, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ERADICATE_INVISIBILITY, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_HEIGHTENED_VISION, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_MENTAL_BARRIER, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_MIND_TRAP, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_PSIONIC_BLAST, 5);
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_FORCED_SHARED_PAIN, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_SHARPENED_EDGE, 5);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_UBIQUITUS_VISION, 5);
  /* 4th circle */
  spell_assignment(CLASS_PSIONICIST, PSIONIC_DEADLY_FEAR, 7);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_DEATH_URGE, 7);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_EMPATHIC_FEEDBACK, 7);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ENERGY_ADAPTATION, 7);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_INCITE_PASSION, 7);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_INTELLECT_FORTRESS, 7);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_MOMENT_OF_TERROR, 7);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_POWER_LEECH, 7);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_SLIP_THE_BONDS, 7);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_WITHER, 7);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_WALL_OF_ECTOPLASM, 7);
  /* 5th circle Psi */
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_ADAPT_BODY, 9);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ECTOPLASMIC_SHAMBLER, 9);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_PIERCE_VEIL, 9);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_PLANAR_TRAVEL, 9);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_POWER_RESISTANCE, 9);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_PSYCHIC_CRUSH, 9);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_PSYCHOPORTATION, 9);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_SHATTER_MIND_BLANK, 9);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_SHRAPNEL_BURST, 9);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_TOWER_OF_IRON_WILL, 9);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_UPHEAVAL, 9);
  /* 6th circle Psi */
  spell_assignment(CLASS_PSIONICIST, PSIONIC_BREATH_OF_THE_BLACK_DRAGON, 11);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_BRUTALIZE_WOUNDS, 11);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_DISINTEGRATION, 11);
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_REMOTE_VIEW_TRAP, 11);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_SUSTAINED_FLIGHT, 11);
  /* 7th circle Psi */
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_BARRED_MIND, 13);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_COSMIC_AWARENESS, 13);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ENERGY_CONVERSION, 13);
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_ENERGY_WAVE, 13);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_EVADE_BURST, 13);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_OAK_BODY, 13);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_PSYCHOSIS, 13);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ULTRABLAST, 13);
  /* 8th circle Psi */
  spell_assignment(CLASS_PSIONICIST, PSIONIC_BODY_OF_IRON, 15);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_RECALL_DEATH, 15);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_SHADOW_BODY, 15);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_TRUE_METABOLISM, 15);
  /* 9th circle Psi */
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_APOPSI, 17);
  spell_assignment(CLASS_PSIONICIST, PSIONIC_ASSIMILATE, 17);
  // spell_assignment(CLASS_PSIONICIST, PSIONIC_TIMELESS_BODY, 17);
  /**** end psi power assignment *****/

  /* end PSI class */
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_WEAPON_MASTER, "weaponmaster", "WpM", "\tcWpM\tn", "e) \tcWeaponMaster\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, H, 10, 0, 1, 2, Y, 5000, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Strength, Con/Dex for survivability",
         /*descrip*/ "For the weapon master, perfection is found in the mastery of a "
                     "single melee weapon. A weapon master seeks to unite this weapon of choice with "
                     "his body, to make them one, and to use the weapon as naturally and without "
                     "thought as any other limb.");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_WEAPON_MASTER, B, G, B, B, B);
  assign_class_abils(CLASS_WEAPON_MASTER, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CA, CA, CC, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CA, CA, CA, CC, CC, CA,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CC, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CC, CC);
  assign_class_titles(CLASS_WEAPON_MASTER,         /* class number */
                      "",                          /* <= 4  */
                      "the Inexperienced Weapon",  /* <= 9  */
                      "the Weapon",                /* <= 14 */
                      "the Skilled Weapon",        /* <= 19 */
                      "the Master Weapon",         /* <= 24 */
                      "the Master of All Weapons", /* <= 29 */
                      "the Unmatched Weapon",      /* <= 30 */
                      "the Immortal WeaponMaster", /* <= LVL_IMMORT */
                      "the Relentless Weapon",     /* <= LVL_STAFF */
                      "the God of Weapons",        /* <= LVL_GRSTAFF */
                      "the WeaponMaster"           /* default */
  );
  /* feat assignment */
  /*              class num     feat                              cfeat lvl stack */
  feat_assignment(CLASS_WEAPON_MASTER, FEAT_WEAPON_OF_CHOICE, Y, 1, N);
  feat_assignment(CLASS_WEAPON_MASTER, FEAT_SUPERIOR_WEAPON_FOCUS, Y, 2, N);
  feat_assignment(CLASS_WEAPON_MASTER, FEAT_UNSTOPPABLE_STRIKE, Y, 3, Y);
  feat_assignment(CLASS_WEAPON_MASTER, FEAT_CRITICAL_SPECIALIST, Y, 4, Y);
  feat_assignment(CLASS_WEAPON_MASTER, FEAT_UNSTOPPABLE_STRIKE, Y, 5, Y);
  feat_assignment(CLASS_WEAPON_MASTER, FEAT_UNSTOPPABLE_STRIKE, Y, 6, Y);
  feat_assignment(CLASS_WEAPON_MASTER, FEAT_UNSTOPPABLE_STRIKE, Y, 7, Y);
  feat_assignment(CLASS_WEAPON_MASTER, FEAT_CRITICAL_SPECIALIST, Y, 8, Y);
  feat_assignment(CLASS_WEAPON_MASTER, FEAT_UNSTOPPABLE_STRIKE, Y, 9, Y);
  feat_assignment(CLASS_WEAPON_MASTER, FEAT_INCREASED_MULTIPLIER, Y, 10, N);
  /* no class feats */
  /* no spell assignment */
  /* class prereqs */
  class_prereq_feat(CLASS_WEAPON_MASTER, FEAT_WEAPON_FOCUS, 1);
  class_prereq_feat(CLASS_WEAPON_MASTER, FEAT_COMBAT_EXPERTISE, 1);
  class_prereq_feat(CLASS_WEAPON_MASTER, FEAT_DODGE, 1);
  class_prereq_feat(CLASS_WEAPON_MASTER, FEAT_MOBILITY, 1);
  class_prereq_feat(CLASS_WEAPON_MASTER, FEAT_SPRING_ATTACK, 1);
  class_prereq_feat(CLASS_WEAPON_MASTER, FEAT_WHIRLWIND_ATTACK, 1);
  class_prereq_bab(CLASS_WEAPON_MASTER, 5);
  class_prereq_ability(CLASS_WEAPON_MASTER, ABILITY_INTIMIDATE, 4);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_ARCANE_ARCHER, "arcanearcher", "ArA", "\tGArA\tn", "f) \tGArcaneArcher\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, H, 10, 0, 1, 4, Y, 5000, 0,
         /*prestige spell progression*/ "arcane: 3/4 of arcane archer level",
         /*primary attributes*/ "Dexterity, your primary casting class stats",
         /*descrip*/ "Many who seek to perfect the use of the bow sometimes pursue "
                     "the path of the arcane archer. Arcane archers are masters of ranged combat, "
                     "as they possess the ability to strike at targets with unerring accuracy and "
                     "can imbue their arrows with powerful spells. Arrows fired by arcane archers "
                     "can fell even the most powerful foes with a single, deadly shot.");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_ARCANE_ARCHER, G, G, B, B, B);
  assign_class_abils(CLASS_ARCANE_ARCHER, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CA, CA, CA, CA, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CC, CC, CA, CA, CC, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CC, CC, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CC, CC);
  assign_class_titles(CLASS_ARCANE_ARCHER,         /* class number */
                      "",                          /* <= 4  */
                      "the Precise Shot",          /* <= 9  */
                      "the Magical Archer",        /* <= 14 */
                      "the Masterful Archer",      /* <= 19 */
                      "the Mystical Arrow",        /* <= 24 */
                      "the Arrow Wizard",          /* <= 29 */
                      "the Arrow Storm",           /* <= 30 */
                      "the Immortal ArcaneArcher", /* <= LVL_IMMORT */
                      "the Limitless Archer",      /* <= LVL_STAFF */
                      "the God of Archery",        /* <= LVL_GRSTAFF */
                      "the ArcaneArcher"           /* default */
  );
  /* feat assignment */
  /*              class num     feat                             cfeat lvl stack */
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_ENHANCE_ARROW_MAGIC, Y, 1, Y);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_SEEKER_ARROW, Y, 2, Y);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_ENHANCE_ARROW_MAGIC, Y, 3, Y);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_IMBUE_ARROW, Y, 4, Y);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_ENHANCE_ARROW_MAGIC, Y, 5, Y);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_SEEKER_ARROW, Y, 6, Y);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_IMBUE_ARROW, Y, 6, Y);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_ENHANCE_ARROW_MAGIC, Y, 7, Y);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_SEEKER_ARROW, Y, 8, Y);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_SWARM_OF_ARROWS, Y, 8, N);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_ENHANCE_ARROW_MAGIC, Y, 9, Y);
  feat_assignment(CLASS_ARCANE_ARCHER, FEAT_ARROW_OF_DEATH, Y, 10, N);
  /* no spell assignment */
  /* class prereqs */
  class_prereq_bab(CLASS_ARCANE_ARCHER, 5);
  /* elf, half-elf, drow only */
  class_prereq_race(CLASS_ARCANE_ARCHER, RACE_DROW);
  class_prereq_race(CLASS_ARCANE_ARCHER, RACE_ELF);
  class_prereq_race(CLASS_ARCANE_ARCHER, RACE_HALF_ELF);
  class_prereq_feat(CLASS_ARCANE_ARCHER, FEAT_POINT_BLANK_SHOT, 1);
  class_prereq_feat(CLASS_ARCANE_ARCHER, FEAT_PRECISE_SHOT, 1);
  class_prereq_spellcasting(CLASS_ARCANE_ARCHER, CASTING_TYPE_ARCANE,
                            PREP_TYPE_ANY, 1 /*circle*/);
  class_prereq_cfeat(CLASS_ARCANE_ARCHER, FEAT_WEAPON_FOCUS, CFEAT_SPECIAL_BOW);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_ARCANE_SHADOW, "arcaneshadow", "ArS", "\tGAr\tDS\tn", "n) \tGArcane\tDShadow\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, M, 8, 0, 2, 5, Y, 5000, 0,
         /*prestige spell progression*/ "Arcane advancement every level",
         /*primary attributes*/ "Dexterity, Con for survivability, Int for skills, your primary casting class stats",
         /*descrip*/ "Few can match the guile and craftiness of arcane shadows. These "
                     "prodigious rogues blend the subtlest aspects of the arcane with the natural cunning "
                     "of the bandit and the scoundrel, using spells to enhance their natural rogue abilities. "
                     "Arcane shadows as often as not seek humiliation as a goal to triumph over "
                     "their foes than more violent solutions.");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_ARCANE_SHADOW, B, G, G, B, B);
  assign_class_abils(CLASS_ARCANE_SHADOW, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CA, CA, CA, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CC, CA, CA, CA, CA, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CA, CA, CA, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_ARCANE_SHADOW,          /* class number */
                      "",                           /* <= 4  */
                      "the Hidden Cantrip",         /* <= 9  */
                      "the Magical Rogue",          /* <= 14 */
                      "the Arcane Shadow",          /* <= 19 */
                      "the Mystical Shadow",        /* <= 24 */
                      "the Rogue Magician",         /* <= 29 */
                      "the Stealth Tornado",        /* <= 30 */
                      "the Immortal ArcaneShadow",  /* <= LVL_IMMORT */
                      "the Limitless ArcaneShadow", /* <= LVL_STAFF */
                      "the God of ArcaneShadow",    /* <= LVL_GRSTAFF */
                      "the ArcaneShadow"            /* default */
  );
  /* feat assignment */
  /*              class num     feat                             cfeat lvl stack */
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_IMPROMPTU_SNEAK_ATTACK, Y, 1, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_SNEAK_ATTACK, Y, 2, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_IMPROMPTU_SNEAK_ATTACK, Y, 3, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_SNEAK_ATTACK, Y, 4, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_IMPROMPTU_SNEAK_ATTACK, Y, 5, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_SNEAK_ATTACK, Y, 6, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_INVISIBLE_ROGUE, Y, 7, N);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_IMPROMPTU_SNEAK_ATTACK, Y, 7, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_SNEAK_ATTACK, Y, 8, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_MAGICAL_AMBUSH, Y, 9, N);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_IMPROMPTU_SNEAK_ATTACK, Y, 9, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_SNEAK_ATTACK, Y, 10, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_SURPRISE_SPELLS, Y, 10, N);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_IMPROMPTU_SNEAK_ATTACK, Y, 10, Y);
  /* no spell assignment */
  /* class prereqs */
  class_prereq_ability(CLASS_ARCANE_SHADOW, ABILITY_DISABLE_DEVICE, 4);
  class_prereq_ability(CLASS_ARCANE_SHADOW, ABILITY_ESCAPE_ARTIST, 4);
  class_prereq_ability(CLASS_ARCANE_SHADOW, ABILITY_SPELLCRAFT, 4);
  class_prereq_spellcasting(CLASS_ARCANE_SHADOW, CASTING_TYPE_ARCANE,
                            PREP_TYPE_ANY, 2 /*circle*/);
  class_prereq_feat(CLASS_ARCANE_SHADOW, FEAT_SNEAK_ATTACK, 2);
  class_prereq_align(CLASS_ARCANE_SHADOW, NEUTRAL_GOOD);
  class_prereq_align(CLASS_ARCANE_SHADOW, TRUE_NEUTRAL);
  class_prereq_align(CLASS_ARCANE_SHADOW, NEUTRAL_EVIL);
  class_prereq_align(CLASS_ARCANE_SHADOW, CHAOTIC_EVIL);
  class_prereq_align(CLASS_ARCANE_SHADOW, CHAOTIC_GOOD);
  class_prereq_align(CLASS_ARCANE_SHADOW, CHAOTIC_NEUTRAL);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_ELDRITCH_KNIGHT, "eldritchknight", "EKn", "\tWE\tCKn\tn", "n) \tWEldritch\tCKnight\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, H, 10, 0, 2, 2, Y, 5000, 0,
         /*prestige spell progression*/ "Arcane advancement every level",
         /*primary attributes*/ "Strength, Con/Dex for survivability, your primary casting class stats",
         /*descrip*/ "Fearsome warriors and spellcasters, eldritch knights are rare among magic-users "
                     "in their ability to wade into battle alongside fighters, barbarians, and other "
                     "martial classes. Those who must face eldritch knights in combat fear them greatly, "
                     "for their versatility on the battlefield is tremendous; against heavily armed "
                     "and armored opponents they may level crippling spells, while opposing spellcasters "
                     "meet their ends on an eldritch knight's blade.");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_ELDRITCH_KNIGHT, G, B, B, G, B);
  assign_class_abils(CLASS_ELDRITCH_KNIGHT, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CC, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CA, CA, CA, CA, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CC, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_ELDRITCH_KNIGHT, /* class number */
                      "",                    /* <= 4  */
                      "the Eldritch Knight", /* <= 9  */
                      "the Eldritch Knight", /* <= 14 */
                      "the Eldritch Knight", /* <= 19 */
                      "the Eldritch Knight", /* <= 24 */
                      "the Eldritch Knight", /* <= 29 */
                      "the Eldritch Knight", /* <= 30 */
                      "the Eldritch Knight", /* <= LVL_IMMORT */
                      "the Eldritch Knight", /* <= LVL_STAFF */
                      "the Eldritch Knight", /* <= LVL_GRSTAFF */
                      "the Eldritch Knight"  /* default */
  );
  /* feat assignment */
  /*              class num     feat                             cfeat lvl stack */
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_DIVERSE_TRAINING, Y, 1, Y);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_SPELL_CRITICAL, Y, 10, N);
  /* list of class feats */
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_ARMOR_SKIN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_ARMOR_SPECIALIZATION_LIGHT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_ARMOR_SPECIALIZATION_MEDIUM, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_ARMOR_SPECIALIZATION_HEAVY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_BLIND_FIGHT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_CLEAVE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_COMBAT_EXPERTISE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_COMBAT_REFLEXES, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_DEFLECT_ARROWS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_DAMAGE_REDUCTION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_DODGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_EXOTIC_WEAPON_PROFICIENCY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_FAR_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_GREAT_CLEAVE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_GREATER_TWO_WEAPON_FIGHTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_GREATER_WEAPON_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_GREATER_WEAPON_SPECIALIZATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_BULL_RUSH, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_CRITICAL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_DISARM, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_FEINT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_GRAPPLE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_INITIATIVE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_OVERRUN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_PRECISE_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_SHIELD_PUNCH, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_KNOCKDOWN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_SHIELD_CHARGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_SHIELD_SLAM, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_SUNDER, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_TRIP, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_TWO_WEAPON_FIGHTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_IMPROVED_UNARMED_STRIKE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_MANYSHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_MOBILITY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_MOUNTED_ARCHERY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_MOUNTED_COMBAT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_POINT_BLANK_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_POWER_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_PRECISE_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_QUICK_DRAW, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_RAPID_RELOAD, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_RAPID_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_RIDE_BY_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_ROBILARS_GAMBIT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_SHOT_ON_THE_RUN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_SNATCH_ARROWS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_SPIRITED_CHARGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_SPRING_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_STUNNING_FIST, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_SWARM_OF_ARROWS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_TRAMPLE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_TWO_WEAPON_DEFENSE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_TWO_WEAPON_FIGHTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_WEAPON_FINESSE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_WEAPON_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_WEAPON_SPECIALIZATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_WHIRLWIND_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_FAST_HEALING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_WEAPON_MASTERY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_WEAPON_FLURRY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_WEAPON_SUPREMACY, Y, NOASSIGN_FEAT, N);
  /* epic class */
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_EPIC_PROWESS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_GREAT_STRENGTH, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_GREAT_DEXTERITY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_GREAT_CONSTITUTION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_EPIC_TOUGHNESS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_ELDRITCH_KNIGHT, FEAT_EPIC_WEAPON_SPECIALIZATION, Y, NOASSIGN_FEAT, N);
  /* no spell assignment */
  /* class prereqs */
  class_prereq_spellcasting(CLASS_ELDRITCH_KNIGHT, CASTING_TYPE_ARCANE, PREP_TYPE_ANY, 3 /*circle*/);
  class_prereq_feat(CLASS_ELDRITCH_KNIGHT, FEAT_MARTIAL_WEAPON_PROFICIENCY, 1);

  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_SPELLSWORD, "spellsword", "SSw", "\tWS\tGSw\tn", "n) \tWSpell\tGSword\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, H, 8, 0, 2, 2, Y, 5000, 0,
         /*prestige spell progression*/ "Arcane advancement at level one and every second level after",
         /*primary attributes*/ "Strength, Con/Dex for survivability, your primary casting class stats",
         /*descrip*/ "The dream of melding magic and weaponplay is fulfilled in the person "
                     "of the spellsword. A student of both arcane rituals and martial techniques, "
                     "the spellsword gradually learns to cast spells in armor with less chance of "
                     "failure. Moreover, he can cast spells through his weapon, bypassing his "
                     "opponent's defenses. Despite the class's name, a spellsword can use any "
                     "weapon or even switch weapons. 'Spellaxe,' 'spellspear,' and other appellations "
                     "for this prestige class are certainly possible but not commonly used. The "
                     "requirements for this prestige class make it most attractive to multiclass "
                     "wizard/warriors or sorcerer/warriors, although bard/warriors can meet the "
                     "requirements just as easily. Feared by other martial characters because of "
                     "his ability to use spells, and feared by spellcasters because of his ability "
                     "to cast those spells while wearing armor, a spellsword often walks the world alone.");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_SPELLSWORD, G, B, G, G, B);
  assign_class_abils(CLASS_SPELLSWORD, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CC, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CA, CA, CA, CA, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CC, CC, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_SPELLSWORD,  /* class number */
                      "",                /* <= 4  */
                      "the Spell Sword", /* <= 9  */
                      "the Spell Sword", /* <= 14 */
                      "the Spell Sword", /* <= 19 */
                      "the Spell Sword", /* <= 24 */
                      "the Spell Sword", /* <= 29 */
                      "the Spell Sword", /* <= 30 */
                      "the Spell Sword", /* <= LVL_IMMORT */
                      "the Spell Sword", /* <= LVL_STAFF */
                      "the Spell Sword", /* <= LVL_GRSTAFF */
                      "the Spell Sword"  /* default */
  );
  /* feat assignment */
  /*              class num     feat                             cfeat lvl stack */
  feat_assignment(CLASS_SPELLSWORD, FEAT_IGNORE_SPELL_FAILURE, Y, 1, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IGNORE_SPELL_FAILURE, Y, 3, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_CHANNEL_SPELL, Y, 4, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IGNORE_SPELL_FAILURE, Y, 5, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_CHANNEL_SPELL, Y, 6, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IGNORE_SPELL_FAILURE, Y, 6, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IGNORE_SPELL_FAILURE, Y, 7, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_CHANNEL_SPELL, Y, 8, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IGNORE_SPELL_FAILURE, Y, 8, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IGNORE_SPELL_FAILURE, Y, 9, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_MULTIPLE_CHANNEL_SPELL, Y, 10, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IGNORE_SPELL_FAILURE, Y, 10, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IGNORE_SPELL_FAILURE, Y, 10, Y);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IGNORE_SPELL_FAILURE, Y, 10, Y);
  /* list of class feats */
  feat_assignment(CLASS_SPELLSWORD, FEAT_ARMOR_SKIN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_ARMOR_SPECIALIZATION_LIGHT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_ARMOR_SPECIALIZATION_MEDIUM, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_ARMOR_SPECIALIZATION_HEAVY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_BLIND_FIGHT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_CLEAVE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_COMBAT_EXPERTISE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_COMBAT_REFLEXES, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_DEFLECT_ARROWS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_DAMAGE_REDUCTION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_DODGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_EXOTIC_WEAPON_PROFICIENCY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_FAR_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_GREAT_CLEAVE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_GREATER_TWO_WEAPON_FIGHTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_GREATER_WEAPON_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_GREATER_WEAPON_SPECIALIZATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_BULL_RUSH, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_CRITICAL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_DISARM, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_FEINT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_GRAPPLE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_INITIATIVE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_OVERRUN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_PRECISE_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_SHIELD_PUNCH, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_KNOCKDOWN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_SHIELD_CHARGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_SHIELD_SLAM, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_SUNDER, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_TRIP, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_TWO_WEAPON_FIGHTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_IMPROVED_UNARMED_STRIKE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_MANYSHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_MOBILITY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_MOUNTED_ARCHERY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_MOUNTED_COMBAT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_POINT_BLANK_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_POWER_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_PRECISE_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_QUICK_DRAW, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_RAPID_RELOAD, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_RAPID_SHOT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_RIDE_BY_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_ROBILARS_GAMBIT, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_SHOT_ON_THE_RUN, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_SNATCH_ARROWS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_SPIRITED_CHARGE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_SPRING_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_STUNNING_FIST, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_SWARM_OF_ARROWS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_TRAMPLE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_TWO_WEAPON_DEFENSE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_TWO_WEAPON_FIGHTING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_WEAPON_FINESSE, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_WEAPON_FOCUS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_WEAPON_SPECIALIZATION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_WHIRLWIND_ATTACK, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_FAST_HEALING, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_WEAPON_MASTERY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_WEAPON_FLURRY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_WEAPON_SUPREMACY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_WEAPON_SUPREMACY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_QUICKEN_SPELL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_MAXIMIZE_SPELL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_EXTEND_SPELL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_ENHANCE_SPELL, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_SILENT_SPELL, Y, NOASSIGN_FEAT, N);
  /* epic class */
  feat_assignment(CLASS_SPELLSWORD, FEAT_EPIC_PROWESS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_GREAT_STRENGTH, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_GREAT_DEXTERITY, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_GREAT_CONSTITUTION, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_EPIC_TOUGHNESS, Y, NOASSIGN_FEAT, N);
  feat_assignment(CLASS_SPELLSWORD, FEAT_EPIC_WEAPON_SPECIALIZATION, Y, NOASSIGN_FEAT, N);
  /* no spell assignment */
  /* class prereqs */
  class_prereq_spellcasting(CLASS_SPELLSWORD, CASTING_TYPE_ARCANE, PREP_TYPE_ANY, 2 /*circle*/);
  class_prereq_bab(CLASS_SPELLSWORD, 4);
  class_prereq_ability(CLASS_SPELLSWORD, ABILITY_LORE, 6);
  class_prereq_feat(CLASS_SPELLSWORD, FEAT_MARTIAL_WEAPON_PROFICIENCY, 1);
  class_prereq_feat(CLASS_SPELLSWORD, FEAT_SIMPLE_WEAPON_PROFICIENCY, 1);
  class_prereq_feat(CLASS_SPELLSWORD, FEAT_ARMOR_PROFICIENCY_LIGHT, 1);
  class_prereq_feat(CLASS_SPELLSWORD, FEAT_ARMOR_PROFICIENCY_MEDIUM, 1);
  class_prereq_feat(CLASS_SPELLSWORD, FEAT_ARMOR_PROFICIENCY_HEAVY, 1);

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_SACRED_FIST, "sacredfist", "SaF", "\tGSa\tgF\tn", "n) \tGSacred\tgFist\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, M, 8, 0, 4, 4, Y, 5000, 0,
         /*prestige spell progression*/ "Divine advancement every level",
         /*primary attributes*/ "Wisdom, Con/Dex for survivability",
         /*descrip*/ "Sacred Fists are independent organizations found within many temples.  "
                     "Their ascetic members have turned their divine magic inward, bringing their bodies "
                     "and wills into harmony.  They consider their bodies and minds gifts from their deity, "
                     "and they believe that not developing those gifts to their fullest potential is a sin. "
                     "Spellcasting does not dishonor them or their deity. Sacred Fists are strong in faith, "
                     "will and body.");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_SACRED_FIST, G, G, B, B, B);
  assign_class_abils(CLASS_SACRED_FIST, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CA, CC, CC, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CC, CA, CA, CA, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CA, CC, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_SACRED_FIST,          /* class number */
                      "",                         /* <= 4  */
                      "the Adept Fist",           /* <= 9  */
                      "the Holy Hand",            /* <= 14 */
                      "the Sacred Fist",          /* <= 19 */
                      "the Holy Martial Artist",  /* <= 24 */
                      "the Fist of Holy Fire",    /* <= 29 */
                      "the Divine Flurry",        /* <= 30 */
                      "the Immortal SacredFist",  /* <= LVL_IMMORT */
                      "the Limitless SacredFist", /* <= LVL_STAFF */
                      "the God of SacredFists",   /* <= LVL_GRSTAFF */
                      "the SacredFist"            /* default */
  );
  /* feat assignment */
  /*              class num     feat                             cfeat lvl stack */
  feat_assignment(CLASS_SACRED_FIST, FEAT_WEAPON_PROFICIENCY_MONK, Y, 1, N);
  feat_assignment(CLASS_SACRED_FIST, FEAT_WIS_AC_BONUS, Y, 1, N);
  feat_assignment(CLASS_SACRED_FIST, FEAT_LVL_AC_BONUS, Y, 1, N);
  feat_assignment(CLASS_SACRED_FIST, FEAT_UNARMED_STRIKE, Y, 1, N);
  feat_assignment(CLASS_SACRED_FIST, FEAT_IMPROVED_UNARMED_STRIKE, Y, 1, N);
  feat_assignment(CLASS_SACRED_FIST, FEAT_AC_BONUS, Y, 2, Y);
  feat_assignment(CLASS_SACRED_FIST, FEAT_SACRED_FLAMES, Y, 2, Y);
  feat_assignment(CLASS_SACRED_FIST, FEAT_SACRED_FLAMES, Y, 3, Y);
  feat_assignment(CLASS_SACRED_FIST, FEAT_SACRED_FLAMES, Y, 4, Y);
  feat_assignment(CLASS_SACRED_FIST, FEAT_SACRED_FLAMES, Y, 5, Y);
  feat_assignment(CLASS_SACRED_FIST, FEAT_UNCANNY_DODGE, Y, 6, N);
  feat_assignment(CLASS_SACRED_FIST, FEAT_AC_BONUS, Y, 7, Y);
  feat_assignment(CLASS_SACRED_FIST, FEAT_SACRED_FLAMES, Y, 7, Y);
  feat_assignment(CLASS_SACRED_FIST, FEAT_SACRED_FLAMES, Y, 8, Y);
  feat_assignment(CLASS_SACRED_FIST, FEAT_INNER_FIRE, Y, 9, N);
  feat_assignment(CLASS_SACRED_FIST, FEAT_GREATER_FLURRY, Y, 10, N);
  feat_assignment(CLASS_SACRED_FIST, FEAT_AC_BONUS, Y, 10, Y);
  /* no spell assignment */
  /* class prereqs */
  class_prereq_spellcasting(CLASS_SACRED_FIST, CASTING_TYPE_DIVINE,
                            PREP_TYPE_ANY, 1 /*circle*/);
  class_prereq_feat(CLASS_SACRED_FIST, FEAT_KI_STRIKE, 1);
  class_prereq_feat(CLASS_SACRED_FIST, FEAT_COMBAT_CASTING, 1);
  class_prereq_bab(CLASS_SACRED_FIST, 4);
  class_prereq_ability(CLASS_SACRED_FIST, ABILITY_LORE, 8);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_STALWART_DEFENDER, "stalwartdefender", "SDe", "\tWS\tcDe\tn", "g) \tWStalwart \tcDefender\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, H, 12, 0, 1, 2, Y, 5000, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Strength, Con/Dex for survivability",
         /*descrip*/ "Drawn from the ranks of guards, knights, mercenaries, and "
                     "thugs alike, stalwart defenders are masters of claiming an area and refusing "
                     "to relinquish it. This behavior is more than a tactical decision for stalwart "
                     "defenders; it is an obsessive, stubborn expression of the need to be undefeated. "
                     "When stalwart defenders set themselves in a defensive stance, they place their "
                     "whole effort into weathering whatever foe, conflict, or threat comes their way.");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_STALWART_DEFENDER, G, B, G, B, B);
  assign_class_abils(CLASS_STALWART_DEFENDER, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CA, CA, CA, CC, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CA, CA, CA, CA, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CC, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CC, CC);
  assign_class_titles(CLASS_STALWART_DEFENDER,   /* class number */
                      "",                        /* <= 4  */
                      "the Defender",            /* <= 9  */
                      "the Immovable",           /* <= 14 */
                      "the Wall",                /* <= 19 */
                      "the Stalwart Wall",       /* <= 24 */
                      "the Wall of Iron",        /* <= 29 */
                      "the Wall of Steel",       /* <= 30 */
                      "the Immortal Defender",   /* <= LVL_IMMORT */
                      "the Indestructible Wall", /* <= LVL_STAFF */
                      "the God of Defense",      /* <= LVL_GRSTAFF */
                      "the Stalwart Defender"    /* default */
  );
  /* feat assignment */
  /*              class num     feat                                   cfeat lvl stack */
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_AC_BONUS, Y, 1, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_DEFENSIVE_STANCE, Y, 1, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_FEARLESS_DEFENSE, Y, 2, N);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_UNCANNY_DODGE, Y, 3, N);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_DEFENSIVE_STANCE, Y, 3, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_AC_BONUS, Y, 4, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_IMMOBILE_DEFENSE, Y, 4, N);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_SHRUG_DAMAGE, Y, 5, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_DEFENSIVE_STANCE, Y, 5, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_AC_BONUS, Y, 7, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_SHRUG_DAMAGE, Y, 7, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_SHRUG_DAMAGE, Y, 7, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_IMPROVED_UNCANNY_DODGE, Y, 7, N);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_DEFENSIVE_STANCE, Y, 7, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_RENEWED_DEFENSE, Y, 8, N);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_MOBILE_DEFENSE, Y, 9, N);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_DEFENSIVE_STANCE, Y, 9, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_LAST_WORD, Y, 10, N);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_AC_BONUS, Y, 10, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_SHRUG_DAMAGE, Y, 10, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_SHRUG_DAMAGE, Y, 10, Y);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_SMASH_DEFENSE, Y, 10, N);
  feat_assignment(CLASS_STALWART_DEFENDER, FEAT_DEFENSIVE_STANCE, Y, 10, Y);
  /* no class feats */
  /* no spell assignment */
  /* class prereqs */
  class_prereq_bab(CLASS_STALWART_DEFENDER, 7);
  class_prereq_feat(CLASS_STALWART_DEFENDER, FEAT_DODGE, 1);
  class_prereq_feat(CLASS_STALWART_DEFENDER, FEAT_ENDURANCE, 1);
  class_prereq_feat(CLASS_STALWART_DEFENDER, FEAT_TOUGHNESS, 1);
  class_prereq_feat(CLASS_STALWART_DEFENDER, FEAT_ARMOR_PROFICIENCY_MEDIUM, 1);
  class_prereq_feat(CLASS_STALWART_DEFENDER, FEAT_ARMOR_PROFICIENCY_LIGHT, 1);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_SHIFTER, "shifter", "Shf", "\twS\tWh\twf\tn", "f) \twSh\tWift\twer\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, M, 8, 0, 1, 4, Y, 5000, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Wisdom, Con/Dex for survivability, Str for combat",
         /*descrip*/ "A shifter has no form they call their own. Instead, they clothe "
                     "themselves in whatever shape is most expedient at the time. While others base "
                     "their identities largely on their external forms, the shifter actually comes "
                     "closer to their true self through all of their transformations. Of necessity, "
                     "their sense of self is based not on their outward form, but on their soul, which "
                     "is truly the only constant about them. It is the inner strength of that soul "
                     "that enables them to take on any shape and remain themselves within.");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_SHIFTER, G, G, B, G, B);
  assign_class_abils(CLASS_SHIFTER, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CA, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CC, CC, CA, CA, CC, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CC, CA, CC, CA, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CA, CA, CC, CC);
  assign_class_titles(CLASS_SHIFTER,              /* class number */
                      "",                         /* <= 4  */
                      "the Shape Changer",        /* <= 9  */
                      "the Changeling",           /* <= 14 */
                      "the Formless",             /* <= 19 */
                      "the Shape Shifter",        /* <= 24 */
                      "the Form Changer",         /* <= 29 */
                      "the Perpetually Changing", /* <= 30 */
                      "the Immortal Shifter",     /* <= LVL_IMMORT */
                      "the Limitless Changeling", /* <= LVL_STAFF */
                      "the Guru of Shifting",     /* <= LVL_GRSTAFF */
                      "the Shifter"               /* default */
  );
  /* feat assignment */
  /*              class num     feat                                 cfeat lvl stack */
  feat_assignment(CLASS_SHIFTER, FEAT_LIMITLESS_SHAPES, Y, 1, N);
  feat_assignment(CLASS_SHIFTER, FEAT_SHIFTER_SHAPES_1, Y, 2, N);
  feat_assignment(CLASS_SHIFTER, FEAT_SHIFTER_SHAPES_2, Y, 4, N);
  feat_assignment(CLASS_SHIFTER, FEAT_SHIFTER_SHAPES_3, Y, 6, N);
  feat_assignment(CLASS_SHIFTER, FEAT_SHIFTER_SHAPES_4, Y, 8, N);
  feat_assignment(CLASS_SHIFTER, FEAT_SHIFTER_SHAPES_5, Y, 10, N);
  /* no class feats */
  /* no spell assignment */
  /* class prereqs */
  class_prereq_class_level(CLASS_SHIFTER, CLASS_DRUID, 10);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_SHADOWDANCER, "shadowdancer", "ShD", "\trSh\tDd\tn", "f) \trSh\tDd\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, M, 8, 0, 1, 6, Y, 5000, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Dexterity, Con for survivability, Int for skills, Str for combat",
         /*descrip*/ "Shadowdancers exist in the boundary between light and darkness, "
                     "where they weave together the shadows to become half-seen artists "
                     "of deception. Unbound by any specified morality or traditional code, "
                     "shadowdancers encompass a wide variety of adventuring types who have "
                     "seen the value of the dark.");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_SHADOWDANCER, B, G, B, G, B);
  assign_class_abils(CLASS_SHADOWDANCER, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CA, CA, CA, CA, CC, CC, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CC, CC, CA, CA, CC, CA, CA,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CC, CA, CA, CC, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CC, CC, CA);
  assign_class_titles(CLASS_SHADOWDANCER,  /* class number */
                      "",                  /* <= 4  */
                      "the Shadow Dancer", /* <= 9  */
                      "the Shadow Dancer", /* <= 14 */
                      "the Shadow Dancer", /* <= 19 */
                      "the Shadow Dancer", /* <= 24 */
                      "the Shadow Dancer", /* <= 29 */
                      "the Shadow Dancer", /* <= 30 */
                      "the Shadow Dancer", /* <= LVL_IMMORT */
                      "the Shadow Dancer", /* <= LVL_STAFF */
                      "the Shadow Dancer", /* <= LVL_GRSTAFF */
                      "the Shadow Dancer"  /* default */
  );
  /* feat assignment */
  /*              class num     feat                                 cfeat lvl stack */
  feat_assignment(CLASS_SHADOWDANCER, FEAT_HIDE_IN_PLAIN_SIGHT, Y, 1, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_WEAPON_PROFICIENCY_SHADOWDANCER, Y, 1, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_EVASION, Y, 2, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_INFRAVISION, Y, 2, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_UNCANNY_DODGE, Y, 2, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SNEAK_ATTACK, Y, 3, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SHADOW_ILLUSION, Y, 3, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SUMMON_SHADOW, Y, 3, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SHADOW_CALL, Y, 4, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SHADOW_JUMP, Y, 4, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_DEFENSIVE_ROLL, Y, 5, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_IMPROVED_UNCANNY_DODGE, Y, 5, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SNEAK_ATTACK, Y, 6, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SHADOW_JUMP, Y, 6, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SLIPPERY_MIND, Y, 7, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SHADOW_JUMP, Y, 8, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SHADOW_POWER, Y, 8, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SNEAK_ATTACK, Y, 9, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_IMPROVED_EVASION, Y, 10, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SHADOW_JUMP, Y, 10, N);
  feat_assignment(CLASS_SHADOWDANCER, FEAT_SHADOW_MASTER, Y, 10, N);

  /* no class feats */
  /* no spell assignment */
  /* class prereqs */
  class_prereq_feat(CLASS_SHADOWDANCER, FEAT_COMBAT_REFLEXES, 1);
  class_prereq_feat(CLASS_SHADOWDANCER, FEAT_DODGE, 1);
  class_prereq_ability(CLASS_SHADOWDANCER, ABILITY_ACROBATICS, 5);
  class_prereq_ability(CLASS_SHADOWDANCER, ABILITY_PERFORM, 2);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_DUELIST, "duelist", "Due", "\tcDue\tn", "i) \tcDuelist\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, H, 10, 0, 1, 4, Y, 5000, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Dexterity/Intelligence, Con for survivability, Str for combat",
         /*descrip*/ "Duelists represent the pinnacle of elegant swordplay. They "
                     "move with a grace unmatched by most foes, parrying blows and countering attacks "
                     "with swift thrusts of their blades. They may wear armor, but generally eschew "
                     "such bulky protection as their grace allows them to dodge their opponents with "
                     "ease. While others flounder on treacherous terrain, duelists charge nimbly "
                     "across the battlefield, leaping and tumbling into the fray. They thrive in "
                     "melee, where their skill with the blade allows them to make sudden attacks "
                     "against clumsy foes and to cripple opponents with particularly well-placed "
                     "thrusts of the blade.");
  /* class-number then saves:  fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_DUELIST, B, G, B, B, B);
  assign_class_abils(CLASS_DUELIST, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CA, CC, CA, CA, CC, CC, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CC, CA, CA, CA, CC, CC, CA,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CA, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CC, CA);
  assign_class_titles(CLASS_DUELIST,             /* class number */
                      "",                        /* <= 4  */
                      "the Acrobatic",           /* <= 9  */
                      "the Nimble Maneuver",     /* <= 14 */
                      "the Agile Duelist",       /* <= 19 */
                      "the Crossed Blade",       /* <= 24 */
                      "the Graceful Weapon",     /* <= 29 */
                      "the Elaborate Parry",     /* <= 30 */
                      "the Immortal Duelist",    /* <= LVL_IMMORT */
                      "the Untouchable Duelist", /* <= LVL_STAFF */
                      "the God of Duelist",      /* <= LVL_GRSTAFF */
                      "the Duelist"              /* default */
  );
  /* feat assignment */
  /*              class num      feat                      cfeat lvl stack */
  feat_assignment(CLASS_DUELIST, FEAT_CANNY_DEFENSE, Y, 1, N);
  feat_assignment(CLASS_DUELIST, FEAT_PRECISE_STRIKE, Y, 1, N);
  feat_assignment(CLASS_DUELIST, FEAT_IMPROVED_REACTION, Y, 2, Y);
  feat_assignment(CLASS_DUELIST, FEAT_PARRY, Y, 2, N);
  feat_assignment(CLASS_DUELIST, FEAT_ENHANCED_MOBILITY, Y, 3, N);
  feat_assignment(CLASS_DUELIST, FEAT_GRACE, Y, 4, N);
  feat_assignment(CLASS_DUELIST, FEAT_COMBAT_REFLEXES, Y, 4, N);
  feat_assignment(CLASS_DUELIST, FEAT_RIPOSTE, Y, 5, N);
  feat_assignment(CLASS_DUELIST, FEAT_ACROBATIC_CHARGE, Y, 6, N);
  feat_assignment(CLASS_DUELIST, FEAT_ELABORATE_PARRY, Y, 7, N);
  feat_assignment(CLASS_DUELIST, FEAT_IMPROVED_REACTION, Y, 8, Y);
  feat_assignment(CLASS_DUELIST, FEAT_DEFLECT_ARROWS, Y, 9, Y);
  feat_assignment(CLASS_DUELIST, FEAT_NO_RETREAT, Y, 9, N);
  feat_assignment(CLASS_DUELIST, FEAT_CRIPPLING_CRITICAL, Y, 10, N);
  /* no spell assignment */
  /* class prereqs */
  class_prereq_bab(CLASS_DUELIST, 6);
  class_prereq_ability(CLASS_DUELIST, ABILITY_ACROBATICS, 2);
  class_prereq_ability(CLASS_DUELIST, ABILITY_PERFORM, 2);
  class_prereq_feat(CLASS_DUELIST, FEAT_DODGE, 1);
  class_prereq_feat(CLASS_DUELIST, FEAT_MOBILITY, 1);
  class_prereq_feat(CLASS_DUELIST, FEAT_WEAPON_FINESSE, 1);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_MYSTIC_THEURGE, "mystictheurge", "MTh", "\tmM\tBTh\tn", "f) \tmMystic\tBTheurge\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, L, 4, 0, 1, 2, Y, 5000, 0,
         /*prestige spell progression*/ "each level in -both- divine/arcane choice",
         /*primary attributes*/ "Your primary casting class stats, Con/Dex for survivability",
         /*descrip*/ "Mystic theurges place no boundaries on their magical abilities "
                     "and find no irreconcilable paradox in devotion to the arcane as well as the "
                     "divine. They seek magic in all of its forms, finding no reason or logic in "
                     "denying themselves instruction by limiting their knowledge to one stifling "
                     "paradigm, though many are simply hungry for limitless power. No matter what "
                     "their motivations, mystic theurges believe that perception is reality, and "
                     "through the divine forces and astral energies of the multiverse, that "
                     "perception can be used to manipulate and control not only the nature of "
                     "this reality, but destiny itself");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_MYSTIC_THEURGE, B, B, G, B, B);
  assign_class_abils(CLASS_MYSTIC_THEURGE, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CA, CC, CC, CA, CA, CC, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CC, CC, CC, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_MYSTIC_THEURGE,         /* class number */
                      "",                           /* <= 4  */
                      "Acolyte of Duality",         /* <= 9  */
                      "",                           /* <= 14 */
                      "",                           /* <= 19 */
                      "",                           /* <= 24 */
                      "",                           /* <= 29 */
                      "",                           /* <= 30 */
                      "the Immortal MysticTheurge", /* <= LVL_IMMORT */
                      "the Limitless Caster",       /* <= LVL_STAFF */
                      "the God of Magic",           /* <= LVL_GRSTAFF */
                      "the MysticTheurge"           /* default */
  );
  /* feat assignment */
  /*              class num     feat                             cfeat lvl stack */
  feat_assignment(CLASS_MYSTIC_THEURGE, FEAT_THEURGE_SPELLCASTING, Y, 1, Y);
  feat_assignment(CLASS_MYSTIC_THEURGE, FEAT_THEURGE_SPELLCASTING, Y, 2, Y);
  feat_assignment(CLASS_MYSTIC_THEURGE, FEAT_THEURGE_SPELLCASTING, Y, 3, Y);
  feat_assignment(CLASS_MYSTIC_THEURGE, FEAT_THEURGE_SPELLCASTING, Y, 4, Y);
  feat_assignment(CLASS_MYSTIC_THEURGE, FEAT_THEURGE_SPELLCASTING, Y, 5, Y);
  feat_assignment(CLASS_MYSTIC_THEURGE, FEAT_THEURGE_SPELLCASTING, Y, 6, Y);
  feat_assignment(CLASS_MYSTIC_THEURGE, FEAT_THEURGE_SPELLCASTING, Y, 7, Y);
  feat_assignment(CLASS_MYSTIC_THEURGE, FEAT_THEURGE_SPELLCASTING, Y, 8, Y);
  feat_assignment(CLASS_MYSTIC_THEURGE, FEAT_THEURGE_SPELLCASTING, Y, 9, Y);
  feat_assignment(CLASS_MYSTIC_THEURGE, FEAT_THEURGE_SPELLCASTING, Y, 10, Y);

  /* no spell assignment */
  /* class prereqs */
  class_prereq_spellcasting(CLASS_MYSTIC_THEURGE, CASTING_TYPE_ARCANE,
                            PREP_TYPE_ANY, 2 /*circle*/);
  class_prereq_spellcasting(CLASS_MYSTIC_THEURGE, CASTING_TYPE_DIVINE,
                            PREP_TYPE_ANY, 2 /*circle*/);
  class_prereq_ability(CLASS_MYSTIC_THEURGE, ABILITY_LORE, 6);
  class_prereq_ability(CLASS_MYSTIC_THEURGE, ABILITY_SPELLCRAFT, 6);
  /****************************************************************************/

  /****************************************************************************/
  /****************************************************************************/

  /****************************************************************************/
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_ALCHEMIST, "alchemist", "Alc", "\tWA\tClc\tn", "f) \tWAlchemist\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         -1, N, N, M, 8, 0, 1, 4, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Intelligence, Con/Dex for survivability, Str for combat",
         /*descrip*/
         "Whether secreted away in a smoky basement laboratory or gleefully experimenting"
         " in a well-respected school of magic, the alchemist is often regarded as being "
         " just as unstable, unpredictable, and dangerous as the concoctions he brews. "
         " While some creators of alchemical items content themselves with sedentary "
         " lives as merchants, providing tindertwigs and smokesticks, the true "
         " alchemist answers a deeper calling. Rather than cast magic like a "
         " spellcaster, the alchemist captures his own magic potential within "
         " liquids and extracts he creates, infusing his chemicals with virulent power "
         " to grant him impressive skill with poisons, explosives, and all manner of "
         " self-transformative magic.\r\n\r\n");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_ALCHEMIST, G, G, B, G, B);
  assign_class_abils(CLASS_ALCHEMIST, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CA, CA, CC, CC, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CA, CA, CC, CA, CA, CC, CA, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CA, CC, CC, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CA, CA, CA, CC);
  assign_class_titles(CLASS_ALCHEMIST,             /* class number */
                      "the Novice Alchemist",      /* <= 4  */
                      "the Apprentice Alchemist",  /* <= 9  */
                      "the Alchemist Adept",       /* <= 14 */
                      "the Journeyman Alchemist",  /* <= 19 */
                      "the Master Alchemist",      /* <= 24 */
                      "the Grandmaster Alchemist", /* <= 29 */
                      "the Alchemy Guru",          /* <= 30 */
                      "the Immortal Alchemist",    /* <= LVL_IMMORT */
                      "the Limitless Alchemist",   /* <= LVL_STAFF */
                      "the God of Alchemy",        /* <= LVL_GRSTAFF */
                      "the Alchemist"              /* default */
  );
  /* spell/concoction assigns */

  /* concoction circle 1 */
  spell_assignment(CLASS_ALCHEMIST, SPELL_CURE_LIGHT, 1);
  spell_assignment(CLASS_ALCHEMIST, SPELL_ENLARGE_PERSON, 1);
  spell_assignment(CLASS_ALCHEMIST, SPELL_EXPEDITIOUS_RETREAT, 1);
  spell_assignment(CLASS_ALCHEMIST, SPELL_IDENTIFY, 1);
  spell_assignment(CLASS_ALCHEMIST, SPELL_DETECT_ALIGN, 1);
  spell_assignment(CLASS_ALCHEMIST, SPELL_SHIELD, 1);
  spell_assignment(CLASS_ALCHEMIST, SPELL_ANT_HAUL, 1);

  /* concoction circle 2 */
  spell_assignment(CLASS_ALCHEMIST, SPELL_AID, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_BARKSKIN, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_ENDURANCE, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_BLUR, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_STRENGTH, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_GRACE, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_CURE_MODERATE, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_INFRAVISION, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_CHARISMA, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_FALSE_LIFE, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_CUNNING, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_INVISIBLE, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_WISDOM, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_BESTOW_WEAPON_PROFICIENCY, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_RESIST_ENERGY, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_DETECT_INVIS, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_LESSER_RESTORATION, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_UNDETECTABLE_ALIGNMENT, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_MASS_ANT_HAUL, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_SPIDER_CLIMB, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_HUMAN_POTENTIAL, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_PROTECTION_FROM_ARROWS, 4);
  /* concoction circle 3 */
  spell_assignment(CLASS_ALCHEMIST, SPELL_CURE_SERIOUS, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_DISPLACEMENT, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_FLY, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_HASTE, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_CURE_BLIND, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_CURE_DEAFNESS, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_REMOVE_CURSE, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_REMOVE_DISEASE, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_GASEOUS_FORM, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_PROTECTION_FROM_ENERGY, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_COMMUNAL_PROTECTION_FROM_ARROWS, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_COMMUNAL_RESIST_ENERGY, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_RAGE, 7);

  /* concoction circle 4 */
  spell_assignment(CLASS_ALCHEMIST, SPELL_CURE_CRITIC, 10);
  spell_assignment(CLASS_ALCHEMIST, SPELL_DEATH_WARD, 10);
  spell_assignment(CLASS_ALCHEMIST, SPELL_FIRE_SHIELD, 10);
  spell_assignment(CLASS_ALCHEMIST, SPELL_GREATER_INVIS, 10);
  spell_assignment(CLASS_ALCHEMIST, SPELL_REMOVE_POISON, 10);
  spell_assignment(CLASS_ALCHEMIST, SPELL_MINOR_GLOBE, 10);
  spell_assignment(CLASS_ALCHEMIST, SPELL_STONESKIN, 10);

  /* concoction circle 5 */
  spell_assignment(CLASS_ALCHEMIST, SPELL_NIGHTMARE, 13);
  spell_assignment(CLASS_ALCHEMIST, SPELL_POLYMORPH, 13);

  /* concoction circle 6 */
  spell_assignment(CLASS_ALCHEMIST, SPELL_EYEBITE, 16);
  spell_assignment(CLASS_ALCHEMIST, SPELL_HEAL, 16);
  spell_assignment(CLASS_ALCHEMIST, SPELL_TRANSFORMATION, 16);
  spell_assignment(CLASS_ALCHEMIST, SPELL_TRUE_SEEING, 16);
  spell_assignment(CLASS_ALCHEMIST, SPELL_SHADOW_WALK, 16);

  /* starting feats and proficiencies */
  feat_assignment(CLASS_ALCHEMIST, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);

  /* feat assignment */
  /*              class num     feat                             cfeat lvl stack */
  /* concontions */
  feat_assignment(CLASS_ALCHEMIST, FEAT_CONCOCT_LVL_1, Y, 1, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_CONCOCT_LVL_2, Y, 4, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_CONCOCT_LVL_3, Y, 7, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_CONCOCT_LVL_4, Y, 10, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_CONCOCT_LVL_5, Y, 13, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_CONCOCT_LVL_6, Y, 16, Y);
  /* level 1 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_BREW_POTION, Y, 1, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_MUTAGEN, Y, 1, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 1, Y);
  /* level 2 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_POISON_RESIST, Y, 2, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 2, Y);
  /* level 3 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_SWIFT_ALCHEMY, Y, 3, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 3, Y);
  /* level 4 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 4, Y);
  /* level 5 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_POISON_RESIST, Y, 5, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 5, Y);
  /* level 6 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_SWIFT_POISONING, Y, 6, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 6, Y);
  /* level 7 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 7, Y);
  /* level 8 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_POISON_RESIST, Y, 8, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 8, Y);
  /* level 9 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 9, Y);
  /* level 10 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_POISON_IMMUNITY, Y, 10, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 10, Y);
  /* level 11 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 11, Y);
  /* level 12 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_APPLY_POISON, Y, 12, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 12, Y);
  /* level 13 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 13, Y);
  /* level 14 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_PERSISTENT_MUTAGEN, Y, 14, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 14, Y);
  /* level 15 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 15, Y);
  /* level 16 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 16, Y);
  /* level 17 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 17, Y);
  /* level 18 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_INSTANT_ALCHEMY, Y, 18, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 18, Y);
  /* level 19 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 19, Y);
  /* level 20 class feats */
  feat_assignment(CLASS_ALCHEMIST, FEAT_GRAND_ALCHEMICAL_DISCOVERY, Y, 20, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 20, Y);
  // epic feats level 21+
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 21, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 22, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 22, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 23, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 24, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 24, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 25, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 26, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 26, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 27, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 28, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 28, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 29, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMBS, Y, 30, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_ALCHEMICAL_DISCOVERY, Y, 30, Y);
  feat_assignment(CLASS_ALCHEMIST, FEAT_BOMB_MASTERY, Y, 30, Y);
  /* this we may be taking away from them */
  spell_assignment(CLASS_ALCHEMIST, SPELL_GREATER_RUIN, 21);
  spell_assignment(CLASS_ALCHEMIST, SPELL_MUMMY_DUST, 21);

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_INQUISITOR, "inquisitor", "Inq", "\tDI\tWnq\tn", "f) \tDInquisitor\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         -1, N, N, M, 8, 0, 1, 6, Y, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Wisdom, Con/Dex for survivability, Str for combat",
         /*descrip*/
         "Grim and determined, the inquisitor roots out enemies of the faith, "
         "using trickery and guile when righteousness and purity is not enough. "
         "Although inquisitors are dedicated to a deity, they are above many of "
         "the normal rules and conventions of the church. They answer to their "
         "deity and their own sense of justice alone, and are willing to take "
         "extreme measures to meet their goals.\r\n\r\n");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_INQUISITOR, G, B, G, B, G);

  assign_class_abils(CLASS_INQUISITOR, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CA, CA, CA, CA, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CC, CA, CA, CA, CC, CA,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CC, CA, CC, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CA, CA, CC, CC);
  assign_class_titles(CLASS_INQUISITOR,             /* class number */
                      "the Novice Inquisitor",      /* <= 4  */
                      "the Apprentice Inquisitor",  /* <= 9  */
                      "the Inquisitor Adept",       /* <= 14 */
                      "the Journeyman Inquisitor",  /* <= 19 */
                      "the Master Inquisitor",      /* <= 24 */
                      "the Grandmaster Inquisitor", /* <= 29 */
                      "the Inquisition Guru",       /* <= 30 */
                      "the Immortal Inquisitor",    /* <= LVL_IMMORT */
                      "the Limitless Inquisitor",   /* <= LVL_STAFF */
                      "the God of Inquisition",     /* <= LVL_GRSTAFF */
                      "the Inquisitor"              /* default */
  );

  /* no spell assignment */

  /* spell circle 1 */
  spell_assignment(CLASS_INQUISITOR, SPELL_BLESS, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_CURE_LIGHT, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_DETECT_ALIGN, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_DIVINE_FAVOR, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_DOOM, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_EXPEDITIOUS_RETREAT, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_HEDGING_WEAPONS, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_CAUSE_LIGHT_WOUNDS, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_PROT_FROM_EVIL, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_PROT_FROM_GOOD, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_REMOVE_FEAR, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_SHIELD_OF_FAITH, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_SHIELD_OF_FORTIFICATION, 1);
  spell_assignment(CLASS_INQUISITOR, SPELL_TRUE_STRIKE, 1);

  /* spell circle 2 */
  spell_assignment(CLASS_INQUISITOR, SPELL_AID, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_WEAPON_OF_AWE, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_UNDETECTABLE_ALIGNMENT, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_SPIRITUAL_WEAPON, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_BESTOW_WEAPON_PROFICIENCY, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_CURE_MODERATE, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_VIGORIZE_LIGHT, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_DARKNESS, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_EFFORTLESS_ARMOR, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_HOLD_PERSON, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_HONEYED_TONGUE, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_CAUSE_MODERATE_WOUNDS, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_INVISIBLE, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_LITANY_OF_DEFENSE, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_RESIST_ENERGY, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_LESSER_RESTORATION, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_DETECT_INVIS, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_SILENCE, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_REMOVE_PARALYSIS, 4);
  spell_assignment(CLASS_INQUISITOR, SPELL_TACTICAL_ACUMEN, 4);

  /* spell circle 3 */
  spell_assignment(CLASS_INQUISITOR, SPELL_CONTINUAL_FLAME, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_BLINDING_RAY, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_HOLY_JAVELIN, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_CURE_SERIOUS, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_DAYLIGHT, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_DISPEL_MAGIC, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_HEROISM, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_CAUSE_SERIOUS_WOUNDS, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_LITANY_OF_RIGHTEOUSNESS, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_LOCATE_OBJECT, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_NON_DETECTION, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_PRAYER, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_INVISIBILITY_PURGE, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_REMOVE_CURSE, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_REMOVE_DISEASE, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_RIGHTEOUS_VIGOR, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_WEAPON_OF_IMPACT, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_KEEN_EDGE, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_SEARING_LIGHT, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_GREATER_MAGIC_WEAPON, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_MAGIC_VESTMENT, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_PROTECTION_FROM_ENERGY, 7);
  spell_assignment(CLASS_INQUISITOR, SPELL_COMMUNAL_RESIST_ENERGY, 7);

  /* spell circle 4 */
  spell_assignment(CLASS_INQUISITOR, SPELL_CURE_CRITIC, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_DEATH_WARD, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_DISMISSAL, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_FEAR, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_FREE_MOVEMENT, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_HOLD_ANIMAL, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_CAUSE_CRITICAL_WOUNDS, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_GREATER_INVIS, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_VIGORIZE_SERIOUS, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_REMOVE_POISON, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_STONESKIN, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_DIVINE_POWER, 10);
  spell_assignment(CLASS_INQUISITOR, SPELL_COMMUNAL_PROTECTION_FROM_ENERGY, 10);

  /* spell circle 5 */
  spell_assignment(CLASS_INQUISITOR, SPELL_BANISH, 13);
  spell_assignment(CLASS_INQUISITOR, SPELL_MASS_CURE_LIGHT, 13);
  spell_assignment(CLASS_INQUISITOR, SPELL_DISPEL_EVIL, 13);
  spell_assignment(CLASS_INQUISITOR, SPELL_DISPEL_GOOD, 13);
  spell_assignment(CLASS_INQUISITOR, SPELL_FLAME_STRIKE, 13);
  spell_assignment(CLASS_INQUISITOR, SPELL_SPELL_RESISTANCE, 13);
  spell_assignment(CLASS_INQUISITOR, SPELL_TRUE_SEEING, 13);
  spell_assignment(CLASS_INQUISITOR, SPELL_VIGORIZE_CRITICAL, 13);

  /* spell circle 6 */
  spell_assignment(CLASS_INQUISITOR, SPELL_BLADE_BARRIER, 16);
  spell_assignment(CLASS_INQUISITOR, SPELL_GREATER_DISPELLING, 16);
  spell_assignment(CLASS_INQUISITOR, SPELL_HARM, 16);
  spell_assignment(CLASS_INQUISITOR, SPELL_HEAL, 16);
  spell_assignment(CLASS_INQUISITOR, SPELL_UNDEATH_TO_DEATH, 16);

  spell_assignment(CLASS_INQUISITOR, SPELL_MUMMY_DUST, 21);
  spell_assignment(CLASS_INQUISITOR, SPELL_GREATER_RUIN, 21);

  /* starting feats and proficiencies */
  feat_assignment(CLASS_INQUISITOR, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_INQUISITOR_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_ARMOR_PROFICIENCY_MEDIUM, Y, 1, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_ARMOR_PROFICIENCY_SHIELD, Y, 1, N);

  /* feat assignment */
  /*              class num     feat                             cfeat lvl stack */
  /* concontions */
  feat_assignment(CLASS_INQUISITOR, FEAT_INQUISITOR_1ST_CIRCLE, Y, 1, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_INQUISITOR_2ND_CIRCLE, Y, 4, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_INQUISITOR_3RD_CIRCLE, Y, 7, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_INQUISITOR_4TH_CIRCLE, Y, 10, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_INQUISITOR_5TH_CIRCLE, Y, 13, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_INQUISITOR_6TH_CIRCLE, Y, 16, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_INQUISITOR_EPIC_SPELL, Y, 21, N);

  /* class feats */
  feat_assignment(CLASS_INQUISITOR, FEAT_JUDGEMENT, Y, 1, Y);
  feat_assignment(CLASS_INQUISITOR, FEAT_MONSTER_LORE, Y, 1, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_STERN_GAZE, Y, 1, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_CUNNING_INITIATIVE, Y, 2, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_DETECT_EVIL, Y, 2, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_DETECT_GOOD, Y, 2, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_TRACK, Y, 2, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_SOLO_TACTICS, Y, 3, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_TEAMWORK, Y, 3, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_JUDGEMENT, Y, 4, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_BANE, Y, 5, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_TEAMWORK, Y, 6, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_JUDGEMENT, Y, 7, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_SECOND_JUDGEMENT, Y, 8, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_TEAMWORK, Y, 9, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_JUDGEMENT, Y, 10, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_STALWART, Y, 11, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_GREATER_BANE, Y, 12, N);
  feat_assignment(CLASS_INQUISITOR, FEAT_TEAMWORK, Y, 12, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_JUDGEMENT, Y, 13, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_EXPLOIT_WEAKNESS, Y, 14, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_TEAMWORK, Y, 15, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_JUDGEMENT, Y, 16, Y);
  feat_assignment(CLASS_INQUISITOR, FEAT_THIRD_JUDGEMENT, Y, 16, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_SLAYER, Y, 17, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_TEAMWORK, Y, 18, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_JUDGEMENT, Y, 19, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_TRUE_JUDGEMENT, Y, 19, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_JUDGEMENT, Y, 22, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_FOURTH_JUDGEMENT, Y, 24, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_JUDGEMENT, Y, 25, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_JUDGEMENT, Y, 28, Y);

  feat_assignment(CLASS_INQUISITOR, FEAT_FIFTH_JUDGEMENT, Y, 29, N);

  feat_assignment(CLASS_INQUISITOR, FEAT_PERFECT_JUDGEMENT, Y, 30, N);

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_SUMMONER, "summoner", "Sum", "\tCS\tcum\tn", "f) \tCSummoner\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         -1, N, N, M, 8, 0, 1, 2, N, 0, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Charisma, Con/Dex for survivability, Str for combat",
         /*descrip*/
         "While many who dabble in the arcane become adept at beckoning monsters from the farthest "
         "reaches of the planes, none are more skilled at it than the summoner. This practitioner of "
         "the arcane arts forms a close bond with one particular outsider, known as an eidolon, who "
         "gains power as the summoner becomes more proficient at his summoning. Over time, the two "
         "become linked, eventually even sharing a shard of the same soul. But this power comes with "
         "a price: the summoner’s spells and abilities are limited due to his time spent enhancing the "
         "power and exploring the nature of his eidolon.\r\n\r\n");
  /* class-number then saves:        fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_SUMMONER, B, B, G, B, G);

  assign_class_abils(CLASS_SUMMONER, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CC, CC, CC, CA, CC, CA, CA,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CA, CC, CC, CA, CA, CC, CC, CC,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CA, CC, CC, CC, CA, CC,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_SUMMONER,             /* class number */
                      "the Novice Summoner",      /* <= 4  */
                      "the Apprentice Summoner",  /* <= 9  */
                      "the Summoner Adept",       /* <= 14 */
                      "the Journeyman Summoner",  /* <= 19 */
                      "the Master Summoner",      /* <= 24 */
                      "the Grandmaster Summoner", /* <= 29 */
                      "the Summoning Guru",       /* <= 30 */
                      "the Immortal Summoner",    /* <= LVL_IMMORT */
                      "the Limitless Summoner",   /* <= LVL_STAFF */
                      "the God of Summoning",     /* <= LVL_GRSTAFF */
                      "the Summoner"              /* default */
  );

  /* no spell assignment */

  /* spell circle 1 */
  spell_assignment(CLASS_SUMMONER, SPELL_ANT_HAUL, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_CORROSIVE_TOUCH, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_ENLARGE_PERSON, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_EXPEDITIOUS_RETREAT, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_GREASE, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_IDENTIFY, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_MAGE_ARMOR, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_MAGIC_FANG, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_MOUNT, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_PROT_FROM_EVIL, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_PROT_FROM_GOOD, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_SHRINK_PERSON, 1);
  // spell_assignment(CLASS_SUMMONER, SPELL_LESSER_EIDOLON_REJUVENATE, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_MAGE_SHIELD, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_SUMMON_CREATURE_1, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_DAZE_MONSTER, 1);
  spell_assignment(CLASS_SUMMONER, SPELL_PLANAR_HEALING, 1);

  // spell circle 2
  spell_assignment(CLASS_SUMMONER, SPELL_MASS_ANT_HAUL, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_BARKSKIN, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_ENDURANCE, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_BLUR, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_STRENGTH, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_GRACE, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_CUSHIONING_BANDS, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_CHARISMA, 4);
  // spell_assignment(CLASS_SUMMONER, SPELL_LESSER_EVOLUTION_SURGE, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_CUNNING, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_GHOST_WOLF, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_GIRD_ALLIES, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_GLITTERDUST, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_HASTE, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_INVISIBLE, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_LEVITATE, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_COMMUNAL_MOUNT, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_WISDOM, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_PROTECTION_FROM_ARROWS, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_CIRCLE_A_GOOD, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_CIRCLE_A_EVIL, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_RESIST_ENERGY, 4);
  // spell_assignment(CLASS_SUMMONER, SPELL_LESSER_RESTORE_EIDOLON, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_DETECT_INVIS, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_SLOW, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_SPIDER_CLIMB, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_SUMMON_CREATURE_2, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_WARDING_WEAPON, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_WIND_WALL, 4);
  spell_assignment(CLASS_SUMMONER, SPELL_HUMAN_POTENTIAL, 4);

  // spell circle 3
  spell_assignment(CLASS_SUMMONER, SPELL_AQUEOUS_ORB, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_BLACK_TENTACLES, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_CHARM_MONSTER, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_CONTROL_SUMMONED_CREATURE, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_DISPEL_MAGIC, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_DISPLACEMENT, 7);
   spell_assignment(CLASS_SUMMONER, SPELL_MASS_ENLARGE_PERSON, 7);
  // spell_assignment(CLASS_SUMMONER, SPELL_EVOLUTION_SURGE, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_FIRE_SHIELD, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_FLY, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_HEROISM, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_GREATER_INVIS, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_LOCATE_CREATURE, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_GREATER_MAGIC_FANG, 7);
   spell_assignment(CLASS_SUMMONER, SPELL_NON_DETECTION, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_COMMUNAL_PROTECTION_FROM_ARROWS, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_PROTECTION_FROM_ENERGY, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_RAGE, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_MASS_REDUCE_PERSON, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_COMMUNAL_RESIST_ENERGY, 7);
  // spell_assignment(CLASS_SUMMONER, SPELL_REJUVENATE_EIDOLON, 7);
  // spell_assignment(CLASS_SUMMONER, SPELL_RESTORE_EIDOLON, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_SIPHON_MIGHT, 7);
  // spell_assignment(CLASS_SUMMONER, SPELL_COMMUNAL_SPIDER_CLIMB, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_STONESKIN, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_WALL_OF_FIRE, 7);
  spell_assignment(CLASS_SUMMONER, SPELL_WATER_BREATHE, 7);

  // spell circle 4
  spell_assignment(CLASS_SUMMONER, SPELL_MASS_ENDURANCE, 10);
  spell_assignment(CLASS_SUMMONER, SPELL_MASS_STRENGTH, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_CALM_AIR, 10);
  spell_assignment(CLASS_SUMMONER, SPELL_MASS_GRACE, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_CAUSTIC_BLOOD, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_GREATER_PLANAR_HEALING, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_MASS_DAZE_MONSTER, 10);
  spell_assignment(CLASS_SUMMONER, SPELL_DISMISSAL, 10);
  spell_assignment(CLASS_SUMMONER, SPELL_MASS_CHARISMA, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_GREATER_EVOLUTION_SURGE, 10);
  spell_assignment(CLASS_SUMMONER, SPELL_MASS_CUNNING, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_HOLD_MONSTER, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_OVERLAND_FLIGHT, 10);
  spell_assignment(CLASS_SUMMONER, SPELL_MASS_WISDOM, 10);
  spell_assignment(CLASS_SUMMONER, SPELL_COMMUNAL_PROTECTION_FROM_ENERGY, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_PURIFIED_CALLING, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_MASS_STONESKIN, 10);
  spell_assignment(CLASS_SUMMONER, SPELL_TELEPORT, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_VITRIOLIC_MIST, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_WALL_OF_STONE, 10);
  // spell_assignment(CLASS_SUMMONER, SPELL_HOSTILE_JUXTAPOSITION, 10);

  // spell circle 5
  // spell_assignment(CLASS_SUMMONER, SPELL_BANISHING_BLADE, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_BANISHMENT, 13);
  spell_assignment(CLASS_SUMMONER, SPELL_CREEPING_DOOM, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_GREATER_DISPEL_MAGIC, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_EAGLESOUL, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_ETHEREAL_JAUNT, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_GENIEKIND, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_GRAND_DESTINY, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_GREATERR_HEROISM, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_MASS_INVISIBILITY, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_GREATER_REJUVENATE_EIDOLON, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_REPULSION, 13);
  spell_assignment(CLASS_SUMMONER, SPELL_SPELL_TURNING, 13);
  spell_assignment(CLASS_SUMMONER, SPELL_TRUE_SEEING, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_WALL_OF_IRON, 13);
  // spell_assignment(CLASS_SUMMONER, SPELL_WREATH_OF_BLADES, 13);
  
  // spell circle 6
  // spell_assignment(CLASS_SUMMONER, SPELL_MASS_CHARM_MONSTER, 16);
  spell_assignment(CLASS_SUMMONER, SPELL_DIMENSIONAL_LOCK, 16);
  // spell_assignment(CLASS_SUMMONER, SPELL_DISCERN_LOCATION, 16);
  // spell_assignment(CLASS_SUMMONER, SPELL_DOMINATE_MONSTER, 16);
  // spell_assignment(CLASS_SUMMONER, SPELL_GREATER_HOSTILE_JUXTAPOSITION, 16);
  spell_assignment(CLASS_SUMMONER, SPELL_MASS_HUMAN_POTENTIAL, 16);
  spell_assignment(CLASS_SUMMONER, SPELL_INCENDIARY_CLOUD, 16);
  // spell_assignment(CLASS_SUMMONER, SPELL_PROTECTION_FROM_SPELLS, 16);

  /* starting feats and proficiencies */
  feat_assignment(CLASS_SUMMONER, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
  feat_assignment(CLASS_SUMMONER, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);

  /* feat assignment */
  /*              class num     feat                             cfeat lvl stack */
  /* concontions */
  // feat_assignment(CLASS_SUMMONER, FEAT_INQUISITOR_1ST_CIRCLE, Y, 1, N);
  // feat_assignment(CLASS_SUMMONER, FEAT_INQUISITOR_2ND_CIRCLE, Y, 4, N);
  // feat_assignment(CLASS_SUMMONER, FEAT_INQUISITOR_3RD_CIRCLE, Y, 7, N);
  // feat_assignment(CLASS_SUMMONER, FEAT_INQUISITOR_4TH_CIRCLE, Y, 10, N);
  // feat_assignment(CLASS_SUMMONER, FEAT_INQUISITOR_5TH_CIRCLE, Y, 13, N);
  // feat_assignment(CLASS_SUMMONER, FEAT_INQUISITOR_6TH_CIRCLE, Y, 16, N);
  // feat_assignment(CLASS_SUMMONER, FEAT_INQUISITOR_EPIC_SPELL, Y, 21, N);

  /* class feats */
  //feat_assignment(CLASS_SUMMONER, FEAT_JUDGEMENT, Y, 1, Y);
  

  /* no spell assignment */
  /* class prereqs */
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name     abrv   clr-abrv     menu-name*/
  classo(CLASS_ASSASSIN, "assassin", "Asn", "\tDAsn\tn", "t) \tDAssassin\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst eFeatp*/
         10, Y, Y, M, 8, 0, 2, 4, Y, 5000, 0,
         /*prestige spell progression*/ "none",
         /*primary attributes*/ "Dexterity, Con for survivability, Str for combat, Int for skills",
         /*descrip*/ "A mercenary undertaking his task with cold, professional detachment, the assassin "
                     "is equally adept at espionage, bounty hunting, and terrorism. At his core, an "
                     "assassin is an artisan, and his medium is death. Trained in a variety of killing "
                     "techniques, assassins are among the most feared classes. Assassins tend to be "
                     "loners by nature, seeing companions as liabilities at best. Sometimes an "
                     "assassin's missions put him in the company of adventurers for long stretches at "
                     "a time, but few people are comfortable trusting a professional assassin to watch "
                     "their backs in a fight, and are more likely to let the emotionless killer scout "
                     "ahead or help prepare ambushes.  (Also see 'help mark')");
  /* class-number then saves: fortitude, reflex, will, poison, death */
  assign_class_saves(CLASS_ASSASSIN, B, G, B, B, B);
  assign_class_abils(CLASS_ASSASSIN, /* class number */
                     /*acrobatics,stealth,perception,heal,intimidate,concentration, spellcraft*/
                     CA, CA, CA, CA, CA, CC, CC,
                     /*appraise,discipline,total_defense,lore,ride,climb,sleight_of_hand,bluff*/
                     CC, CA, CC, CA, CA, CA, CA, CA,
                     /*diplomacy,disable_device,disguise,escape_artist,handle_animal,sense_motive*/
                     CC, CA, CA, CA, CC, CA,
                     /*survival,swim,use_magic_device,perform*/
                     CC, CA, CA, CC);
  assign_class_titles(CLASS_ASSASSIN, /* class number */
                      "",             /* <= 4  */
                      "the Assassin", /* <= 9  */
                      "the Assassin", /* <= 14 */
                      "the Assassin", /* <= 19 */
                      "the Assassin", /* <= 24 */
                      "the Assassin", /* <= 29 */
                      "the Assassin", /* <= 30 */
                      "the Assassin", /* <= LVL_IMMORT */
                      "the Assassin", /* <= LVL_STAFF */
                      "the Assassin", /* <= LVL_GRSTAFF */
                      "the Assassin"  /* default */
  );
  /* feat assignment */
  /*              class num     feat                           cfeat lvl stack */
  feat_assignment(CLASS_ASSASSIN, FEAT_WEAPON_PROFICIENCY_ASSASSIN, Y, 1, N);
  feat_assignment(CLASS_ASSASSIN, FEAT_ARMOR_PROFICIENCY_LIGHT, Y, 1, N);
  feat_assignment(CLASS_ASSASSIN, FEAT_SNEAK_ATTACK, Y, 1, Y);
  feat_assignment(CLASS_ASSASSIN, FEAT_DEATH_ATTACK, Y, 1, Y);

  feat_assignment(CLASS_ASSASSIN, FEAT_POISON_SAVE_BONUS, Y, 2, Y);
  feat_assignment(CLASS_ASSASSIN, FEAT_UNCANNY_DODGE, Y, 2, N);

  feat_assignment(CLASS_ASSASSIN, FEAT_SNEAK_ATTACK, Y, 3, Y);

  feat_assignment(CLASS_ASSASSIN, FEAT_HIDDEN_WEAPONS, Y, 4, N);
  feat_assignment(CLASS_ASSASSIN, FEAT_TRUE_DEATH, Y, 4, N);
  feat_assignment(CLASS_ASSASSIN, FEAT_POISON_SAVE_BONUS, Y, 4, Y);

  feat_assignment(CLASS_ASSASSIN, FEAT_SNEAK_ATTACK, Y, 5, Y);
  feat_assignment(CLASS_ASSASSIN, FEAT_IMPROVED_UNCANNY_DODGE, Y, 5, N);

  feat_assignment(CLASS_ASSASSIN, FEAT_POISON_SAVE_BONUS, Y, 6, Y);
  feat_assignment(CLASS_ASSASSIN, FEAT_QUIET_DEATH, Y, 6, N);

  feat_assignment(CLASS_ASSASSIN, FEAT_SNEAK_ATTACK, Y, 7, Y);
  feat_assignment(CLASS_ASSASSIN, FEAT_APPLY_POISON, Y, 7, N);

  feat_assignment(CLASS_ASSASSIN, FEAT_POISON_SAVE_BONUS, Y, 8, Y);
  feat_assignment(CLASS_ASSASSIN, FEAT_HIDE_IN_PLAIN_SIGHT, Y, 8, N);

  feat_assignment(CLASS_ASSASSIN, FEAT_SNEAK_ATTACK, Y, 9, Y);
  feat_assignment(CLASS_ASSASSIN, FEAT_SWIFT_DEATH, Y, 9, N);

  feat_assignment(CLASS_ASSASSIN, FEAT_POISON_SAVE_BONUS, Y, 10, Y);
  feat_assignment(CLASS_ASSASSIN, FEAT_ANGEL_OF_DEATH, Y, 10, N);

  /* pre reqs to take assassin class */
  class_prereq_ability(CLASS_ASSASSIN, ABILITY_STEALTH, 5);
  class_prereq_ability(CLASS_ASSASSIN, ABILITY_SENSE_MOTIVE, 2);
  class_prereq_feat(CLASS_ASSASSIN, FEAT_TWO_WEAPON_FIGHTING, 1);
  /****************************************************************************/
  /****************************************************************************/

  /****************************************************************************/
  /****************************************************************************/
}

/* This will check a character to see if the object reference has any anti-class
 * flags associated with the character's class make-up. returns true if
 * the character has an associated class and anti-flag.  */
bool is_class_anti_object(struct char_data *ch, struct obj_data *obj, bool output)
{

  /* this is not compatible with the new homeland zones!  for now we are just skipping this check -zusuk */
  return false; /* do not remove this without checking with zusuk first please */
  /* remove the above line to restore this function to its previous state */

  if ((IS_WIZARD(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_WIZARD)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by wizards.\r\n");
    return true;
  }
  if ((IS_CLERIC(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by clerics.\r\n");
    return true;
  }
  if ((IS_RANGER(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_RANGER)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by rangers.\r\n");
    return true;
  }
  if ((IS_PALADIN(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_PALADIN)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by paladins.\r\n");
    return true;
  }
  if ((IS_ROGUE(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_ROGUE)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by rogues.\r\n");
    return true;
  }
  if ((IS_MONK(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_MONK)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by monks.\r\n");
    return true;
  }
  if ((IS_DRUID(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_DRUID)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by druids.\r\n");
    return true;
  }
  if ((IS_BERSERKER(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_BERSERKER)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by berserkers.\r\n");
    return true;
  }
  if ((IS_SORCERER(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_SORCERER)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by sorcerers.\r\n");
    return true;
  }
  if ((IS_BARD(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_BARD)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by bards.\r\n");
    return true;
  }
  if ((IS_WARRIOR(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_WARRIOR)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by warriors.\r\n");
    return true;
  }
  if ((IS_WEAPONMASTER(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_WEAPONMASTER)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by weapon masters.\r\n");
    return true;
  }
  if ((IS_ARCANE_ARCHER(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_ARCANE_ARCHER)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by arcane archers.\r\n");
    return true;
  }
  if ((IS_STALWARTDEFENDER(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_STALWART_DEFENDER)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by stalwart defenders.\r\n");
    return true;
  }
  if ((IS_SHIFTER(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_SHIFTER)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by shifters.\r\n");
    return true;
  }
  if ((IS_DUELIST(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_DUELIST)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by duelists.\r\n");
    return true;
  }
  if ((IS_MYSTICTHEURGE(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_MYSTIC_THEURGE)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by mystic theurges.\r\n");
    return true;
  }
  if ((IS_ALCHEMIST(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_ALCHEMIST)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by alchemists.\r\n");
    return true;
  }
  if ((IS_ARCANE_SHADOW(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_ARCANE_SHADOW)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by arcane shadows.\r\n");
    return true;
  }
  if ((IS_SACRED_FIST(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_SACRED_FIST)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by sacred fists.\r\n");
    return true;
  }
  if ((IS_ELDRITCH_KNIGHT(ch)) && (OBJ_FLAGGED(obj, ITEM_ANTI_ELDRITCH_KNIGHT)))
  {
    if (output)
      send_to_char(ch, "This object cannot be used by eldritch knights.\r\n");
    return true;
  }

  return false;
}

/* this will check a character to see if they have any levels in any classes
 * associated with the object's class-required flags. returns true if
 * the character has an associated class and required class */
bool is_class_req_object(struct char_data *ch, struct obj_data *obj, bool output)
{
  if (!(IS_WIZARD(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_WIZARD)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a wizard to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_CLERIC(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_CLERIC)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a cleric to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_RANGER(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_RANGER)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a ranger to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_PALADIN(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_PALADIN)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a paladin to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_ROGUE(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_ROGUE)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a rogue to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_MONK(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_MONK)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a monk to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_DRUID(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_DRUID)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a druid to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_BERSERKER(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_BERSERKER)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a berserker to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_SORCERER(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_SORCERER)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a sorcerer to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_BARD(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_BARD)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a bard to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_WARRIOR(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_WARRIOR)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a warrior to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_WEAPONMASTER(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_WEAPONMASTER)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a weapon master to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_ARCANE_ARCHER(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_ARCANE_ARCHER)))
  {
    if (output)
      send_to_char(ch, "You must have levels as an arcane archer to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_STALWARTDEFENDER(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_STALWART_DEFENDER)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a stalwart defender to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_SHIFTER(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_SHIFTER)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a shifter to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_DUELIST(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_DUELIST)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a duelist to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_MYSTICTHEURGE(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_MYSTIC_THEURGE)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a mystic theurge to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_ALCHEMIST(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_ALCHEMIST)))
  {
    if (output)
      send_to_char(ch, "You must have levels as an alchemist to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_ARCANE_SHADOW(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_ARCANE_SHADOW)))
  {
    if (output)
      send_to_char(ch, "You must have levels as an arcane shadow to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_SACRED_FIST(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_SACRED_FIST)))
  {
    if (output)
      send_to_char(ch, "You must have levels as a sacred fist to use %s.\r\n", obj->short_description);
    return false;
  }
  if (!(IS_ELDRITCH_KNIGHT(ch)) && (OBJ_FLAGGED(obj, ITEM_REQ_ELDRITCH_KNIGHT)))
  {
    if (output)
      send_to_char(ch, "You must have levels as an eldritch knight to use %s.\r\n", obj->short_description);
    return false;
  }

  return true;
}

int num_paladin_mercies_known(struct char_data *ch)
{
  /* dummy check */
  if (!ch)
    return 0;

  int i = 0;
  int num_chosen = 0;

  for (i = 0; i < NUM_PALADIN_MERCIES; i++)
    if (KNOWS_MERCY(ch, i))
      num_chosen++;
  return num_chosen;
}

sbyte has_paladin_mercies_unchosen(struct char_data *ch)
{

  if (!ch)
    return false;

  if (IS_NPC(ch))
    return false;

  int num_avail = CLASS_LEVEL(ch, CLASS_PALADIN) / 3;
  int num_chosen = num_paladin_mercies_known(ch);

  if ((num_avail - num_chosen) > 0)
    return TRUE;

  return FALSE;
}

sbyte has_paladin_mercies_unchosen_study(struct char_data *ch)
{

  if (!ch)
    return false;

  if (IS_NPC(ch))
    return false;

  int num_avail = CLASS_LEVEL(ch, CLASS_PALADIN) / 3;
  int num_chosen = num_paladin_mercies_known(ch);
  int i = 0;
  int num_study = 0;

  for (i = 0; i < NUM_PALADIN_MERCIES; i++)
    if (LEVELUP(ch)->paladin_mercies[i])
      num_study++;

  if ((num_avail - num_chosen - num_study) > 0)
    return TRUE;

  return FALSE;
}

bool can_learn_paladin_mercy(struct char_data *ch, int mercy)
{
  if (!ch)
    return false;

  switch (mercy)
  {
  case PALADIN_MERCY_DECEIVED:
  case PALADIN_MERCY_FATIGUED:
  case PALADIN_MERCY_SHAKEN:
    if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 3)
      return true;
    break;
  case PALADIN_MERCY_DAZED:
  case PALADIN_MERCY_ENFEEBLED:
  case PALADIN_MERCY_STAGGERED:
  case PALADIN_MERCY_CONFUSED:
    if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 6)
      return true;
    break;
  case PALADIN_MERCY_CURSED:
  case PALADIN_MERCY_FRIGHTENED:
  case PALADIN_MERCY_INJURED:
  case PALADIN_MERCY_NAUSEATED:
  case PALADIN_MERCY_POISONED:
  case PALADIN_MERCY_BLINDED:
    if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 9)
      return true;
    break;
  case PALADIN_MERCY_DEAFENED:
  case PALADIN_MERCY_ENSORCELLED:
  case PALADIN_MERCY_PARALYZED:
  case PALADIN_MERCY_STUNNED:
    if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 12)
      return true;
    break;
  }

  return false;
}

int num_languages_learned(struct char_data *ch)
{
  int i = 0, num = 0;

  for (i = 0; i < NUM_LANGUAGES; i++)
  {
    if (ch->player_specials->saved.languages_known[i] > 0)
      num++;
  }
  return num;
}

bool has_unchosen_languages(struct char_data *ch)
{
  if (!ch)
    return false;

  if (IS_NPC(ch))
    return false;

  int num_avail = MAX(0, GET_REAL_INT_BONUS(ch)) + MAX(0, GET_ABILITY(ch, ABILITY_LINGUISTICS));
  int num_chosen = num_languages_learned(ch);
  /*
  send_to_char(ch, "\r\nAVAIL: %d, KNOWN: %d\r\n", num_avail, num_chosen);
  send_to_char(ch, "Int: %d\r\n", MAX(0, GET_REAL_INT_BONUS(ch)));
  send_to_char(ch, "Skill: %d\r\n", MAX(0, GET_ABILITY(ch, ABILITY_LINGUISTICS)));
  */

  if ((num_avail - num_chosen) > 0)
    return TRUE;

  return FALSE;
}

int num_blackguard_cruelties_known(struct char_data *ch)
{
  /* dummy check */
  if (!ch)
    return 0;

  int i = 0;
  int num_chosen = 0;

  for (i = 0; i < NUM_BLACKGUARD_CRUELTIES; i++)
    if (KNOWS_CRUELTY(ch, i))
      num_chosen++;
  return num_chosen;
}

sbyte has_blackguard_cruelties_unchosen(struct char_data *ch)
{

  if (!ch)
    return false;

  if (IS_NPC(ch))
    return false;

  int num_avail = CLASS_LEVEL(ch, CLASS_BLACKGUARD) / 3;
  int num_chosen = num_blackguard_cruelties_known(ch);

  if ((num_avail - num_chosen) > 0)
    return TRUE;

  return FALSE;
}

sbyte has_blackguard_cruelties_unchosen_study(struct char_data *ch)
{

  if (!ch)
    return false;

  if (IS_NPC(ch))
    return false;

  int num_avail = CLASS_LEVEL(ch, CLASS_BLACKGUARD) / 3;
  int num_chosen = num_blackguard_cruelties_known(ch);
  int i = 0;
  int num_study = 0;

  for (i = 0; i < NUM_BLACKGUARD_CRUELTIES; i++)
    if (LEVELUP(ch)->blackguard_cruelties[i])
      num_study++;

  if ((num_avail - num_chosen - num_study) > 0)
    return TRUE;

  return FALSE;
}

bool can_learn_blackguard_cruelty(struct char_data *ch, int mercy)
{
  if (!ch)
    return false;

  switch (mercy)
  {
  case BLACKGUARD_CRUELTY_FATIGUED:
  case BLACKGUARD_CRUELTY_SHAKEN:
  case BLACKGUARD_CRUELTY_SICKENED:
    if (CLASS_LEVEL(ch, CLASS_BLACKGUARD) >= 3)
      return true;
    break;
  case BLACKGUARD_CRUELTY_DAZED:
  case BLACKGUARD_CRUELTY_DISEASED:
  case BLACKGUARD_CRUELTY_STAGGERED:
    if (CLASS_LEVEL(ch, CLASS_BLACKGUARD) >= 6)
      return true;
    break;
  case BLACKGUARD_CRUELTY_CURSED:
  case BLACKGUARD_CRUELTY_FRIGHTENED:
  case BLACKGUARD_CRUELTY_NAUSEATED:
  case BLACKGUARD_CRUELTY_POISONED:
    if (CLASS_LEVEL(ch, CLASS_BLACKGUARD) >= 9)
      return true;
    break;
  case BLACKGUARD_CRUELTY_BLINDED:
  case BLACKGUARD_CRUELTY_DEAFENED:
  case BLACKGUARD_CRUELTY_PARALYZED:
  case BLACKGUARD_CRUELTY_STUNNED:
    if (CLASS_LEVEL(ch, CLASS_BLACKGUARD) >= 12)
      return true;
    break;
  }

  return false;
}

ACMD(do_racefix)
{
  int i = 0;
  bool found = false;

  while (level_feats[i][4] != FEAT_UNDEFINED)
  {
    if (level_feats[i][1] == GET_REAL_RACE(ch) && !HAS_REAL_FEAT(ch, level_feats[i][4]))
    {
      send_to_char(ch, "You have gained the %s racial feat.\r\n", feat_list[level_feats[i][4]].name);
      SET_FEAT(ch, level_feats[i][4], 1);
      found = true;
    }
    i++;
  }

  // We want to fix the stats, because new stats are better than old
  if (ch->player_specials->saved.new_race_stats == false)
  {
    switch (GET_REAL_RACE(ch))
    {
    case RACE_MOON_ELF:
      found = true;
      ch->real_abils.str += 2;
      ch->real_abils.wis += 1;
      send_to_char(ch, "Your strength has been increased by two and your wisdom by one.\r\n");
      break;
    case RACE_SHIELD_DWARF:
      found = true;
      ch->real_abils.cha += 2;
      ch->real_abils.str += 2;
      send_to_char(ch, "Your strength and your charisma have increased by two.\r\n");
      break;
    case RACE_ROCK_GNOME:
      found = true;
      ch->real_abils.con -= 1;
      ch->real_abils.intel += 2;
      ch->real_abils.str += 2;
      send_to_char(ch, "Your strength has been increased by two and your intelligence by two. Your consitiution has been reduced by one.\r\n");
      break;
    case RACE_LIGHTFOOT_HALFLING:
      found = true;
      ch->real_abils.cha += 1;
      ch->real_abils.str += 2;
      send_to_char(ch, "Your strength has been increased by two and your charisma by one.\r\n");
      break;
    case RACE_HALF_ELF:
      found = true;
      ch->real_abils.cha += 2;
      send_to_char(ch, "Your charisma has been increased by two.\r\n");
      break;
    case RACE_HALF_ORC:
      found = true;
      ch->real_abils.cha += 2;
      ch->real_abils.intel += 2;
      ch->real_abils.con += 1;
      send_to_char(ch, "Your charisma and intelligence have been increased by two, and your constitution by one.\r\n");
      break;
    case RACE_HALF_TROLL:
      found = true;
      ch->real_abils.cha += 2;
      ch->real_abils.intel += 2;
      ch->real_abils.wis += 2;
      ch->real_abils.con += 2;
      send_to_char(ch, "Your penalties to int, wis and cha have been improved from -4 to -2.  Your con has been improved by 2.\r\n");
      break;
    case RACE_ARCANA_GOLEM:
      found = true;
      ch->real_abils.cha += 1;
      ch->real_abils.intel += 1;
      ch->real_abils.wis += 1;
      ch->real_abils.con += 2;
      ch->real_abils.str += 2;
      send_to_char(ch, "Your penalties to str and con have been negated.  Your str, con and dex bonuses have been improved by 1.\r\n");
      break;
    case RACE_DROW:
      found = true;
      ch->real_abils.intel += 2;
      ch->real_abils.con += 2;
      send_to_char(ch, "Your penalty to con has been negated and your int has increased by two.\r\n");
      break;
    case RACE_DUERGAR:
      found = true;
      ch->real_abils.cha += 2;
      ch->real_abils.str += 2;
      send_to_char(ch, "Your penalty to cha has been negated and your str has increased by two.\r\n");
      break;
    case RACE_CRYSTAL_DWARF:
      found = true;
      ch->real_abils.dex += 2;
      ch->real_abils.wis += 2;
      send_to_char(ch, "Your dex and wis have been increased by two.\r\n");
      break;
    case RACE_TRELUX:
      found = true;
      ch->real_abils.str += 2;
      send_to_char(ch, "Your strength has been increased by two.\r\n");
      break;
    case RACE_VAMPIRE:
      found = true;
      ch->real_abils.con += 2;
      send_to_char(ch, "Your constitution has been increased by two.\r\n");
      break;
    case RACE_LICH:
      found = true;
      ch->real_abils.str += 2;
      ch->real_abils.con += 2;
      ch->real_abils.dex += 2;
      send_to_char(ch, "Your strength, dexterity and constituion have been increased by two.\r\n");
      break;
    }
  }

  if (!found)
  {
    send_to_char(ch, "You already have all your racial feats and stat adjustments.\r\n");
  }
  else
  {
    // So they an only get extra stats once.
    // But if there were no changes, we won't set the flag,
    // Just in case we make changes down the line.
    ch->player_specials->saved.new_race_stats = true;
  }
}

/** LOCAL UNDEFINES **/
// good/bad
#undef G // good
#undef B // bad
// yes/no
#undef Y // yes
#undef N // no
// high/medium/low
#undef H // high
#undef M // medium
#undef L // low
#undef SP
#undef SK
#undef NA
#undef CC
#undef CA
#undef NOOB_TELEPORTER
#undef NOOB_TORCH
#undef NOOB_RATIONS
#undef NOOB_WATERSKIN
#undef NOOB_BP
#undef NOOB_CRAFTING_KIT
#undef NOOB_BOW
#undef NOOB_QUIVER
#undef NOOB_ARROW
#undef NUM_NOOB_ARROWS
#undef NOOB_WIZ_NOTE
#undef EXP_MAX
#undef MODE_CLASSLIST_NORMAL
#undef MODE_CLASSLIST_RESPEC

/* EOF */
