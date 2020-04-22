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

/** LOCAL DEFINES **/
// good/bad
#define G 1 //good
#define B 0 //bad
// yes/no
#define Y 1 //yes
#define N 0 //no
// high/medium/low
#define H 2 //high
#define M 1 //medium
#define L 0 //low
// spell vs skill
#define SP 0 //spell
#define SK 1 //skill
// skill availability by class
#define NA 0 //not available
#define CC 1 //cross class
#define CA 2 //class ability
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
  sprintf(buf, "%s: %d", attribute_abbr[attribute], value);
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
  sprintf(buf, "%s level %d", CLSLIST_NAME(cl), level);
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
    sprintf(buf, "%s (%d ranks)", feat_list[feat].name, ranks);
  else
    sprintf(buf, "%s", feat_list[feat].name);

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

  sprintf(buf, "%s (%s)", feat_list[feat].name, cfeat_special[special]);
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

  sprintf(buf, "%d ranks in %s", ranks, ability_names[ability]);
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

  sprintf(buf, "Has %s %s (circle) %d spells", casting_types[casting_type],
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

  sprintf(buf, "Race: %s", race_list[race].type);
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

  sprintf(buf, "Min. BAB +%d", bab);
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

  sprintf(buf, "Align: %s", alignment_names_nocolor[alignment]);
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

  sprintf(buf, "Proficiency in same weapon");
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
void classo(int class_num, char *name, char *abbrev, char *colored_abbrev,
            char *menu_name, int max_level, bool locked_class, int prestige_class,
            int base_attack_bonus, int hit_dice, int psp_gain, int move_gain,
            int trains_gain, bool in_game, int unlock_cost, int epic_feat_progression,
            char *spell_prog, char *descrip)
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
  class_list[class_num].descrip = descrip;
  /* list of prereqs */
  class_list[class_num].prereq_list = NULL;
  /* list of spell assignments */
  class_list[class_num].spellassign_list = NULL;
  /* list of feat assignments */
  class_list[class_num].featassign_list = NULL;
}

/* function used for assigning a classes titles */
void assign_class_titles(int class_num, char *title_4, char *title_9, char *title_14,
                         char *title_19, char *title_24, char *title_29, char *title_30, char *title_imm,
                         char *title_stf, char *title_gstf, char *title_default)
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
            IS_PALADIN(ch) ||
            IS_RANGER(ch)))
        return FALSE;
      if (prereq->values[2] > 0)
      {
        if (compute_slots_by_circle(ch, CLASS_CLERIC, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_PALADIN, prereq->values[2]) == 0 &&
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
bool display_class_prereqs(struct char_data *ch, char *classname)
{
  int class = CLASS_UNDEFINED;
  struct class_prerequisite *prereq = NULL;
  static int line_length = 80;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  bool meets_prereqs = FALSE, found = FALSE;

  skip_spaces(&classname);
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
    sprintf(buf, "\tn%s%s%s - %s\r\n",
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
  send_to_char(ch, "\tcNote: Epic races currently can not multi-class\tn\r\n\r\n");

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
  if (iarg == MODE_CLASSLIST_NORMAL)
  {
    for (i = 0; i < NUM_CLASSES; i++)
      if (CLASS_LEVEL(ch, i)) /* found char current class */
        break;
    switch (GET_RACE(ch))
    {
    case RACE_CRYSTAL_DWARF:
      if (classnum == i) /* char class selection and current class match? */
        ;
      else
        return FALSE;
    case RACE_TRELUX:
      if (classnum == i) /* char class selection and current class match? */
        ;
      else
        return FALSE;
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

/* display a specific classes details */
bool display_class_info(struct char_data *ch, char *classname)
{
  int class = -1, i = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  static int line_length = 80;
  bool first_skill = TRUE;
  size_t len = 0;

  skip_spaces(&classname);
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
               (CLSLIST_BAB(i) == 2) ? "High" : (CLSLIST_BAB(class) ? "Medium" : "Low"));
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
    send_to_char(ch, "\tcSpell Cast Command  : \tn%s\r\n", (class != CLASS_ALCHEMIST) ? "cast" : "imbibe");
    char spellList[30];
    sprintf(spellList, "spells %s", CLSLIST_NAME(class));
    for (i = 0; i < strlen(spellList); i++)
      spellList[i] = tolower(spellList[i]);
    send_to_char(ch, "\tcSpell List Command  : \tn%s\r\n", (class != CLASS_ALCHEMIST) ? spellList : "extracts");
  }

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /*  Here display the prerequisites */
  if (class_list[class].prereq_list == NULL)
  {
    sprintf(buf, "\tCPrerequisites : \tnnone\r\n");
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
        sprintf(buf, "\tcPrerequisites : %s%s%s",
                (meets_class_prerequisite(ch, prereq, NO_IARG) ? "\tn" : "\tr"), prereq->description, "\tn");
      }
      else
      {
        sprintf(buf2, ", %s%s%s",
                (meets_class_prerequisite(ch, prereq, NO_IARG) ? "\tn" : "\tr"), prereq->description, "\tn");
        strcat(buf, buf2);
      }
    }
  }
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* This we will need to buffer and wrap so that it will fit in the space provided. */
  sprintf(buf, "\tcDescription : \tn%s\r\n", class_list[class].descrip);
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
  char buf[MAX_STRING_LENGTH];
  size_t len = 0;

  send_to_char(ch, "# Name Abrv ClrAbrv | Menu | MaxLvl Lock Prestige BAB HPs Mvs Train InGame UnlockCost EFeatProg");
  send_to_char(ch, " DESCRIP\r\n");
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
                    "\r\n%d] %s %s %s | %s | %d %s %s %s %d %d %d %s %d %d\r\n     %s\r\n"
                    "  %s %s %s\r\n"
                    "     %s %s %s %s %s %s %s\r\n"
                    "     %s %s %s %s %s %s %s %s\r\n"
                    "     %s %s %s %s %s %s\r\n"
                    "     %s %s %s %s\r\n",
                    i, CLSLIST_NAME(i), CLSLIST_ABBRV(i), CLSLIST_CLRABBRV(i), CLSLIST_MENU(i),
                    CLSLIST_MAXLVL(i), CLSLIST_LOCK(i) ? "Y" : "N", CLSLIST_PRESTIGE(i) ? "Y" : "N",
                    (CLSLIST_BAB(i) == 2) ? "H" : (CLSLIST_BAB(i) ? "M" : "L"), CLSLIST_HPS(i),
                    CLSLIST_MVS(i), CLSLIST_TRAINS(i), CLSLIST_INGAME(i) ? "Y" : "N", CLSLIST_COST(i), CLSLIST_EFEATP(i),
                    CLSLIST_DESCRIP(i),
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

bool view_class_feats(struct char_data *ch, char *classname)
{
  int class = CLASS_UNDEFINED;
  struct class_feat_assign *feat_assign = NULL;

  skip_spaces(&classname);
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
  char *classname;

  /*  Have to process arguments like this
   *  because of the syntax - class info <classname> */
  classname = one_argument(argument, arg);
  one_argument(classname, arg2);

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
    //    case CLASS_SHADOW_DANCER:
  case CLASS_SORCERER:
  case CLASS_MYSTIC_THEURGE:
  case CLASS_SACRED_FIST:
  case CLASS_ALCHEMIST:
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
    "Istishia", //5
    "Kelemvor",
    "Kossuth",
    "Lathander",
    "Mystra",
    "Oghma", //10
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
    //    case 'j': return CLASS_SHADOW_DANCER;
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
    /* empty letters */
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
    /* empty letters */
    /* empty letters */
    /* empty letters */

  default:
    return CLASS_UNDEFINED;
  }
}

/* accept short descrip, return class */
int parse_class_long(char *arg)
{
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
  if (is_abbrev(arg, "weaponmaster"))
    return CLASS_WEAPON_MASTER;
  if (is_abbrev(arg, "weapon-master"))
    return CLASS_WEAPON_MASTER;
  //  if (is_abbrev(arg, "shadow-dancer")) return CLASS_SHADOW_DANCER;
  //  if (is_abbrev(arg, "shadowdancer")) return CLASS_SHADOW_DANCER;
  if (is_abbrev(arg, "arcanearcher"))
    return CLASS_ARCANE_ARCHER;
  if (is_abbrev(arg, "arcane-archer"))
    return CLASS_ARCANE_ARCHER;
  if (is_abbrev(arg, "arcane-shadow"))
    return CLASS_ARCANE_SHADOW;
  if (is_abbrev(arg, "arcaneshadow"))
    return CLASS_ARCANE_SHADOW;
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
    level = CLASS_LEVEL(ch, i);
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
    {CLASS_UNDEFINED, RACE_HUMAN, FALSE, 1, FEAT_QUICK_TO_MASTER},
    {CLASS_UNDEFINED, RACE_HUMAN, FALSE, 1, FEAT_SKILLED},

    /* class, race, stacks?, level, feat_ name */
    /* Dwarf */
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_POISON_RESIST},
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_STABILITY},
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_SPELL_HARDINESS},
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_DWARF, FALSE, 1, FEAT_DWARF_RACIAL_ADJUSTMENT},

    /* class, race, stacks?, level, feat_ name */
    /* Half-Troll */
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_TROLL_REGENERATION},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_WEAKNESS_TO_FIRE},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_WEAKNESS_TO_ACID},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_STRONG_AGAINST_POISON},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_STRONG_AGAINST_DISEASE},
    {CLASS_UNDEFINED, RACE_HALF_TROLL, FALSE, 1, FEAT_HALF_TROLL_RACIAL_ADJUSTMENT},

    /* class, race, stacks?, level, feat_ name */
    /* Halfling */
    {CLASS_UNDEFINED, RACE_HALFLING, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_HALFLING, FALSE, 1, FEAT_SHADOW_HOPPER},
    {CLASS_UNDEFINED, RACE_HALFLING, FALSE, 1, FEAT_LUCKY},
    {CLASS_UNDEFINED, RACE_HALFLING, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_HALFLING, FALSE, 1, FEAT_HALFLING_RACIAL_ADJUSTMENT},

    /* class, race, stacks?, level, feat_ name */
    /* Half-Elf */
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_WEAPON_PROFICIENCY_ELF},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_RESISTANCE_TO_ENCHANTMENTS},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_HALF_BLOOD},
    {CLASS_UNDEFINED, RACE_HALF_ELF, FALSE, 1, FEAT_KEEN_SENSES},

    /* class, race, stacks?, level, feat_ name */
    /* Half-Orc */
    {CLASS_UNDEFINED, RACE_HALF_ORC, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_HALF_ORC, FALSE, 1, FEAT_HALF_ORC_RACIAL_ADJUSTMENT},

    /* class, race, stacks?, level, feat_ name */
    /* Gnome */
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_COMBAT_TRAINING_VS_GIANTS},
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_RESISTANCE_TO_ILLUSIONS},
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_ILLUSION_AFFINITY},
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_TINKER_FOCUS},
    {CLASS_UNDEFINED, RACE_GNOME, FALSE, 1, FEAT_GNOME_RACIAL_ADJUSTMENT},

    /* class, race, stacks?, level, feat_ name */
    /* Trelux */
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_ULTRAVISION},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_VITAL},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_HARDY},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_VULNERABLE_TO_COLD},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_TRELUX_EXOSKELETON},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_LEAP},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_WINGS},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_TRELUX_EQ},
    {CLASS_UNDEFINED, RACE_TRELUX, FALSE, 1, FEAT_TRELUX_PINCERS},

    /* class, race, stacks?, level, feat_ name */
    /* elf */
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_INFRAVISION},
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_WEAPON_PROFICIENCY_ELF},
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_SLEEP_ENCHANTMENT_IMMUNITY},
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_KEEN_SENSES},
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_RESISTANCE_TO_ENCHANTMENTS},
    {CLASS_UNDEFINED, RACE_ELF, FALSE, 1, FEAT_ELF_RACIAL_ADJUSTMENT},

    /* class, race, stacks?, level, feat_ name */
    /* crystal dwarf */
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
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_SPELLBATTLE},
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_SPELL_VULNERABILITY},
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_ENCHANTMENT_VULNERABILITY},
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_PHYSICAL_VULNERABILITY},
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_MAGICAL_HERITAGE},
    {CLASS_UNDEFINED, RACE_ARCANA_GOLEM, FALSE, 1, FEAT_ARCANA_GOLEM_RACIAL_ADJUSTMENT},

    /* class, race, stacks?, level, feat_ name */
    /* Drow */
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
    //disadvantage
    {CLASS_UNDEFINED, RACE_DROW, FALSE, 1, FEAT_LIGHT_BLINDNESS},

    /*****************************************/
    /* This is always the last array element */
    /*****************************************/
    {CLASS_UNDEFINED, RACE_UNDEFINED, FALSE, 1, FEAT_UNDEFINED}};

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
      NOOB_TELEPORTER,
      NOOB_TORCH,
      NOOB_RATIONS,
      NOOB_RATIONS,
      NOOB_RATIONS,
      NOOB_RATIONS,
      NOOB_WATERSKIN,
      NOOB_CRAFTING_KIT,
      NOOB_BOW,
      NOOB_CRAFT_MAT,
      NOOB_CRAFT_MAT,
      NOOB_CRAFT_MAT,
      NOOB_CRAFT_MAT,
      NOOB_CRAFT_MOLD,
      -1 //had to end with -1
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
  default:
    break;
  } /*  end of race specific gear */

  /* class specific gear */
  switch (GET_CLASS(ch))
  {
  case CLASS_PALADIN:
    /*fallthrough*/
  case CLASS_CLERIC:
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
    obj_to_char(read_object(NOOB_WIZ_NOTE, VIRTUAL), ch);      //wizard note
    obj_to_char(read_object(NOOB_WIZ_SPELLBOOK, VIRTUAL), ch); //spellbook
    /* switch fallthrough */
  case CLASS_SORCERER:
    obj = read_object(NOOB_CLOTH_SLEEVES, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // cloth sleeves

    obj = read_object(NOOB_CLOTH_PANTS, VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch); // cloth pants

    obj = read_object(NOOB_DAGGER, VIRTUAL);
    //GET_OBJ_SIZE(obj) = GET_SIZE(ch);
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
  GET_REAL_MAX_PSP(ch) = 15;
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
  save_char(ch, 0);

  /* reset skills/abilities */
  /* we don't want players to lose their hard-earned crafting skills */
  for (i = 1; i <= NUM_SKILLS; i++)
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
  GET_REAL_CON(ch) += get_race_stat(GET_RACE(ch), R_CON_MOD);
  GET_REAL_STR(ch) += get_race_stat(GET_RACE(ch), R_STR_MOD);
  GET_REAL_DEX(ch) += get_race_stat(GET_RACE(ch), R_DEX_MOD);
  GET_REAL_INT(ch) += get_race_stat(GET_RACE(ch), R_INTEL_MOD);
  GET_REAL_WIS(ch) += get_race_stat(GET_RACE(ch), R_WIS_MOD);
  GET_REAL_CHA(ch) += get_race_stat(GET_RACE(ch), R_CHA_MOD);

  /* some racial modifications, size, etc */
  switch (GET_RACE(ch))
  {
  case RACE_HUMAN:
    GET_REAL_SIZE(ch) = SIZE_MEDIUM;
    GET_FEAT_POINTS(ch)
    ++;
    trains += 3;
    break;
  case RACE_ELF:
    GET_REAL_SIZE(ch) = SIZE_MEDIUM;
    break;
  case RACE_DWARF:
    GET_REAL_SIZE(ch) = SIZE_MEDIUM;
    break;
  case RACE_HALFLING:
    GET_REAL_SIZE(ch) = SIZE_SMALL;
    break;
  case RACE_H_ELF:
    GET_REAL_SIZE(ch) = SIZE_MEDIUM;
    break;
  case RACE_H_ORC:
    GET_REAL_SIZE(ch) = SIZE_MEDIUM;
    break;
  case RACE_GNOME:
    GET_REAL_SIZE(ch) = SIZE_SMALL;
    break;
  case RACE_HALF_TROLL:
    GET_REAL_SIZE(ch) = SIZE_LARGE;
    break;
  case RACE_CRYSTAL_DWARF:
    GET_REAL_SIZE(ch) = SIZE_MEDIUM;
    GET_MAX_HIT(ch) += 10;
    break;
  case RACE_TRELUX:
    GET_REAL_SIZE(ch) = SIZE_SMALL;
    GET_MAX_HIT(ch) += 10;
    break;
  case RACE_ARCANA_GOLEM:
    GET_REAL_SIZE(ch) = SIZE_MEDIUM;
    break;
  case RACE_DROW:
    GET_REAL_SIZE(ch) = SIZE_MEDIUM;
    break;
  default:
    GET_REAL_SIZE(ch) = SIZE_MEDIUM;
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
}

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  init_start_char(ch);

  //from level 0 -> level 1
  advance_level(ch, GET_CLASS(ch));
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_PSP(ch) = GET_MAX_PSP(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  GET_COND(ch, DRUNK) = 0;
  if (CONFIG_SITEOK_ALL)
    SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);
}

/* at each level we run this function to assign free CLASS feats */
void process_class_level_feats(struct char_data *ch, int class)
{
  char featbuf[MAX_STRING_LENGTH];
  struct class_feat_assign *feat_assign = NULL;
  int class_level = -1, effective_class_level = -1;
  //struct damage_reduction_type *dr = NULL, *temp = NULL, *ptr = NULL;

  /* deal with some instant disqualification */
  if (class < 0 || class >= NUM_CLASSES)
    return;
  class_level = CLASS_LEVEL(ch, class);
  if (class_level <= 0)
    return;
  if (class_list[class].featassign_list == NULL)
    return;

  sprintf(featbuf, "\tM");

  /*  This class has potential feat assignment! Traverse the list and assign. */
  for (feat_assign = class_list[class].featassign_list; feat_assign != NULL;
       feat_assign = feat_assign->next)
  {

    if (IS_SPELL_CIRCLE_FEAT(feat_assign->feat_num))
    {
      // Mystic Theurge levels stack with class levels for purposes of granting spell access.
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
      switch (feat_assign->feat_num)
      {

      case FEAT_SNEAK_ATTACK:
        sprintf(featbuf, "%s\tMYour sneak attack has increased to +%dd6!\tn\r\n", featbuf, HAS_FEAT(ch, FEAT_SNEAK_ATTACK) + 1);
        break;

      case FEAT_SHRUG_DAMAGE:

        /* the newer DR system, moved it out though for political reasons */
        /*
        for (dr = GET_DR(ch); dr != NULL; dr = dr->next)
        {
          if (dr->feat == FEAT_SHRUG_DAMAGE)
          {
            REMOVE_FROM_LIST(dr, GET_DR(ch), next);
          }
        }

        CREATE(ptr, struct damage_reduction_type, 1);

        ptr->spell = 0;
        ptr->feat = FEAT_SHRUG_DAMAGE;
        ptr->amount = HAS_FEAT(ch, FEAT_SHRUG_DAMAGE) + 1;
        ptr->max_damage = -1;

        ptr->bypass_cat[0] = DR_BYPASS_CAT_NONE;
        ptr->bypass_val[0] = 0;
        ptr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
        ptr->bypass_val[1] = 0; // Unused.
        ptr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
        ptr->bypass_val[2] = 0; // Unused.

        ptr->next = GET_DR(ch);
        GET_DR(ch) = ptr;
        */

        sprintf(featbuf, "%s\tMYou can now shrug off %d damage!\tn\r\n", featbuf, HAS_FEAT(ch, FEAT_SHRUG_DAMAGE) + 1);
        break;

      case FEAT_STRENGTH_BOOST:
        ch->real_abils.str += 2;
        sprintf(featbuf, "%s\tMYour natural strength has increased by +2!\r\n", featbuf);
        break;

      case FEAT_CHARISMA_BOOST:
        ch->real_abils.cha += 2;
        sprintf(featbuf, "%s\tMYour natural charisma has increased by +2!\r\n", featbuf);
        break;

      case FEAT_CONSTITUTION_BOOST:
        ch->real_abils.con += 2;
        sprintf(featbuf, "%s\tMYour natural constitution has increased by +2!\r\n", featbuf);
        break;

      case FEAT_INTELLIGENCE_BOOST:
        ch->real_abils.intel += 2;
        sprintf(featbuf, "%s\tMYour natural intelligence has increased by +2!\r\n", featbuf);
        break;

        /* no special handling */
      default:
        if (HAS_FEAT(ch, feat_assign->feat_num))
          sprintf(featbuf, "%s\tMYou have improved your %s %s!\tn\r\n", featbuf,
                  feat_list[feat_assign->feat_num].name,
                  feat_types[feat_list[feat_assign->feat_num].feat_type]);
        else
          sprintf(featbuf, "%s\tMYou have gained the %s %s!\tn\r\n", featbuf,
                  feat_list[feat_assign->feat_num].name,
                  feat_types[feat_list[feat_assign->feat_num].feat_type]);
        break;
      }

      /* now actually adjust the feat */
      SET_FEAT(ch, feat_assign->feat_num, HAS_REAL_FEAT(ch, feat_assign->feat_num) + 1);
    }
  }

  /* send our feat buffer to char */
  send_to_char(ch, "%s", featbuf);
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
    GRANT_SPELL_CIRCLE(CLASS_DRUID, FEAT_DRUID_1ST_CIRCLE, FEAT_DRUID_EPIC_SPELL);
    GRANT_SPELL_CIRCLE(CLASS_SORCERER, FEAT_SORCERER_1ST_CIRCLE, FEAT_SORCERER_EPIC_SPELL);
    GRANT_SPELL_CIRCLE(CLASS_PALADIN, FEAT_PALADIN_1ST_CIRCLE, FEAT_PALADIN_4TH_CIRCLE);
    GRANT_SPELL_CIRCLE(CLASS_RANGER, FEAT_RANGER_1ST_CIRCLE, FEAT_RANGER_4TH_CIRCLE);
  }
  break;
  }
}
#undef GRANT_SPELL_CIRCLE

/* TODO: rewrite this! */

/* at each level we run this function to assign free RACE feats */
void process_level_feats(struct char_data *ch, int class)
{
  char featbuf[MAX_STRING_LENGTH];
  int i = 0;

  sprintf(featbuf, "\tM");

  /* increment through the list, FEAT_UNDEFINED is our terminator */
  while (level_feats[i][LF_FEAT] != FEAT_UNDEFINED)
  {

    /* feat i doesnt matches our class or we don't meet the min-level (from if above) */
    /* non-class, racial feat and don't have it yet */
    if (level_feats[i][LF_CLASS] == CLASS_UNDEFINED &&
        level_feats[i][LF_RACE] == GET_RACE(ch) &&
        !HAS_FEAT(ch, level_feats[i][LF_FEAT]))
    {
      if (HAS_FEAT(ch, level_feats[i][LF_FEAT]))
        sprintf(featbuf, "%s\tMYou have improved your %s %s!\tn\r\n", featbuf,
                feat_list[level_feats[i][LF_FEAT]].name,
                feat_types[feat_list[level_feats[i][LF_FEAT]].feat_type]);
      else
        sprintf(featbuf, "%s\tMYou have gained the %s %s!\tn\r\n", featbuf,
                feat_list[level_feats[i][LF_FEAT]].name,
                feat_types[feat_list[level_feats[i][LF_FEAT]].feat_type]);
      SET_FEAT(ch, level_feats[i][LF_FEAT], HAS_REAL_FEAT(ch, level_feats[i][LF_FEAT]) + 1);
    }

    /* counter */
    i++;
  }

  send_to_char(ch, "%s", featbuf);
}

/* our function for leveling up */
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
  add_hp += rand_number(CLSLIST_HPS(class) / 2, CLSLIST_HPS(class));

  /* calculate moves gain */
  add_move += rand_number(1, CLSLIST_MVS(class));

  /* calculate psp gain */
  //add_psp += rand_number(CLSLIST_PSP(class)/2, CLSLIST_PSP(class));
  add_psp = 0;

  /* calculate trains gained */
  trains += MAX(1, (CLSLIST_TRAINS(class) + (GET_REAL_INT_BONUS(ch))));

  /* pre epic special class feat progression */
  if (class == CLASS_WIZARD && !(CLASS_LEVEL(ch, CLASS_WIZARD) % 5))
  {
    if (!IS_EPIC(ch))
      class_feats++; // wizards get a bonus class feat every 5 levels
    //else if (IS_EPIC(ch))
    //epic_class_feats++;
  }
  if (class == CLASS_WARRIOR && !(CLASS_LEVEL(ch, CLASS_WARRIOR) % 2))
  {
    if (!IS_EPIC(ch))
      class_feats++; // warriors get a bonus class feat every 2 levels
    //else if (IS_EPIC(ch))
    //epic_class_feats++;
  }
  if (class == CLASS_SORCERER && ((CLASS_LEVEL(ch, CLASS_SORCERER) - 1) % 6 == 0) &&
      CLASS_LEVEL(ch, CLASS_SORCERER) > 1)
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
    add_move += rand_number(1, 2);
  }
  if (HAS_FEAT(ch, FEAT_FAST_MOVEMENT))
  {
    add_move += rand_number(1, 2);
  }

  /* 'free' race feats gained (old system) */
  process_level_feats(ch, class);
  /* 'free' class feats gained (new system) */
  process_class_level_feats(ch, class);
  /* 'free' class feats gained that depend on previous class or feat choices */
  process_conditional_class_level_feats(ch, class);

  //Racial Bonuses
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
  default:
    break;
  }

  //base practice / boost improvement
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
    send_to_char(ch, "\tMYou gain a boost (to stats) point!\tn\r\n");
  }

  /* miscellaneous level-based bonuses */
  if (HAS_FEAT(ch, FEAT_TOUGHNESS))
  {
    /* SRD has this as +3 hp.  More fun as +1 per level. */
    for (i = HAS_FEAT(ch, FEAT_TOUGHNESS); i > 0; i--)
      add_hp++;
  }
  if (HAS_FEAT(ch, FEAT_EPIC_TOUGHNESS))
  {
    /* SRD has this listed as +30 hp.  More fun to do it by level perhaps. */
    for (i = HAS_FEAT(ch, FEAT_EPIC_TOUGHNESS); i > 0; i--)
      add_hp++;
  }

  // we're using more move points now
  add_move *= 10;

  /* adjust final and report changes! */
  GET_REAL_MAX_HIT(ch) += MAX(1, add_hp);
  send_to_char(ch, "\tMTotal HP:\tn %d\r\n", MAX(1, add_hp));
  GET_REAL_MAX_MOVE(ch) += MAX(1, add_move);
  send_to_char(ch, "\tMTotal Move:\tn %d\r\n", MAX(1, add_move));
  if (GET_LEVEL(ch) > 1)
  {
    GET_REAL_MAX_PSP(ch) += add_psp;
    send_to_char(ch, "\tMTotal PSP:\tn %d\r\n", add_psp);
  }
  GET_FEAT_POINTS(ch) += feats;
  if (feats)
    send_to_char(ch, "%d \tMFeat points gained.\tn\r\n", feats);
  GET_CLASS_FEATS(ch, class) += class_feats;
  if (class_feats)
    send_to_char(ch, "%d \tMClass feat points gained.\tn\r\n", class_feats);
  GET_EPIC_FEAT_POINTS(ch) += epic_feats;
  if (epic_feats)
    send_to_char(ch, "%d \tMEpic feat points gained.\tn\r\n", epic_feats);
  GET_EPIC_CLASS_FEATS(ch, class) += epic_class_feats;
  if (epic_class_feats)
    send_to_char(ch, "%d \tMEpic class feat points gained.\tn\r\n", epic_class_feats);
  GET_TRAINS(ch) += trains;
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
  save_char(ch, 0);
}

/* if you get multiplier for backstab, calculated here */
int backstab_mult(struct char_data *ch)
{
  if (HAS_FEAT(ch, FEAT_BACKSTAB))
    return 2;

  return 1;
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
  for (i = (MAX_SPELLS + 1); i < NUM_SKILLS; i++)
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
  case CLASS_MONK:
  case CLASS_DRUID:
  case CLASS_RANGER:
  case CLASS_WARRIOR:
  case CLASS_WEAPON_MASTER:
  case CLASS_STALWART_DEFENDER:
  case CLASS_SHIFTER:
  case CLASS_DUELIST:
    //    case CLASS_ASSASSIN:
    //    case CLASS_SHADOW_DANCER:
  case CLASS_ARCANE_ARCHER:
  case CLASS_ARCANE_SHADOW:
  case CLASS_SACRED_FIST:
  case CLASS_ROGUE:
  case CLASS_BARD:
  case CLASS_BERSERKER:
  case CLASS_CLERIC:
  case CLASS_MYSTIC_THEURGE:
  case CLASS_ALCHEMIST:
    level--;
    if (level < 0)
      level = 0;
    exp += (level * level * factor);
    break;

  default:
    log("SYSERR: Reached invalid class in class.c level()!");
    return 123456;
  }

  //can add other exp penalty/bonuses here
  switch (GET_REAL_RACE(ch))
  { /* funny bug: use to use disguised/wildshape race */
    //advanced races
  case RACE_HALF_TROLL:
    exp *= 2;
    break;
  case RACE_ARCANA_GOLEM:
    exp *= 2;
    break;
  case RACE_DROW:
    exp *= 2;
    break;
    //epic races
  case RACE_CRYSTAL_DWARF:
    exp *= 15;
    break;
  case RACE_TRELUX:
    exp *= 15;
    break;
  default:
    break;
  }

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
         -1, N, N, L, 4, 0, 1, 2, Y, 0, 5,
         /*prestige spell progression*/ "none",
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
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_6TH_CIRCLE, Y, 11, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_7TH_CIRCLE, Y, 13, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_8TH_CIRCLE, Y, 15, N);
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_9TH_CIRCLE, Y, 17, N);
  /*epic*/
  feat_assignment(CLASS_WIZARD, FEAT_WIZARD_EPIC_SPELL, Y, 21, N);
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
  spell_assignment(CLASS_WIZARD, SPELL_ENCHANT_WEAPON, 1);
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
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_WIZARD, SPELL_SHOCKING_GRASP, 3);
  spell_assignment(CLASS_WIZARD, SPELL_SCORCHING_RAY, 3);
  spell_assignment(CLASS_WIZARD, SPELL_CONTINUAL_FLAME, 3);
  spell_assignment(CLASS_WIZARD, SPELL_SUMMON_CREATURE_2, 3);
  spell_assignment(CLASS_WIZARD, SPELL_WEB, 3);
  spell_assignment(CLASS_WIZARD, SPELL_ACID_ARROW, 3);
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
  //spell_assignment(CLASS_WIZARD, SPELL_I_DARKNESS,        3);
  spell_assignment(CLASS_WIZARD, SPELL_RESIST_ENERGY, 3);
  spell_assignment(CLASS_WIZARD, SPELL_ENERGY_SPHERE, 3);
  spell_assignment(CLASS_WIZARD, SPELL_ENDURANCE, 3);
  spell_assignment(CLASS_WIZARD, SPELL_STRENGTH, 3);
  spell_assignment(CLASS_WIZARD, SPELL_GRACE, 3);
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
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_WIZARD, SPELL_FIRE_SHIELD, 7);
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
  spell_assignment(CLASS_WIZARD, SPELL_GATE, 17);
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
  //assign_feat_spell_slots(CLASS_WIZARD);
  /**/
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name      abrv   clr-abrv     menu-name*/
  classo(CLASS_CLERIC, "cleric", "Cle", "\tBCle\tn", "c) \tBCleric\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst eFeatp*/
         -1, N, N, M, 8, 0, 1, 2, Y, 0, 0,
         /*prestige spell progression*/ "none",
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
  feat_assignment(CLASS_CLERIC, FEAT_TURN_UNDEAD, Y, 1, N);
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
  spell_assignment(CLASS_CLERIC, SPELL_ARMOR, 1);
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
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_CLERIC, SPELL_AUGURY, 3);
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
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_CLERIC, SPELL_CURE_CRITIC, 7);
  spell_assignment(CLASS_CLERIC, SPELL_REMOVE_CURSE, 7);
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
  /*              class num      spell                   level acquired */
  /* 5th circle */
  spell_assignment(CLASS_CLERIC, SPELL_POISON, 9);
  spell_assignment(CLASS_CLERIC, SPELL_REMOVE_POISON, 9);
  spell_assignment(CLASS_CLERIC, SPELL_PROT_FROM_EVIL, 9);
  spell_assignment(CLASS_CLERIC, SPELL_GROUP_ARMOR, 9);
  spell_assignment(CLASS_CLERIC, SPELL_FLAME_STRIKE, 9);
  spell_assignment(CLASS_CLERIC, SPELL_PROT_FROM_GOOD, 9);
  spell_assignment(CLASS_CLERIC, SPELL_MASS_CURE_MODERATE, 9);
  spell_assignment(CLASS_CLERIC, SPELL_SUMMON_CREATURE_5, 9);
  spell_assignment(CLASS_CLERIC, SPELL_WATER_BREATHE, 9);
  spell_assignment(CLASS_CLERIC, SPELL_WATERWALK, 9);
  spell_assignment(CLASS_CLERIC, SPELL_REGENERATION, 9);
  spell_assignment(CLASS_CLERIC, SPELL_FREE_MOVEMENT, 9);
  spell_assignment(CLASS_CLERIC, SPELL_STRENGTHEN_BONE, 9);
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
  /*              class num      spell                   level acquired */
  /* 7th circle */
  spell_assignment(CLASS_CLERIC, SPELL_CALL_LIGHTNING, 13);
  //spell_assignment(CLASS_CLERIC, SPELL_CONTROL_WEATHER,    13);
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
  /*              class num      spell                   level acquired */
  /* 8th circle */
  //spell_assignment(CLASS_CLERIC, SPELL_SANCTUARY,       15);
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
  //spell_assignment(CLASS_CLERIC, death shield, 17);
  //spell_assignment(CLASS_CLERIC, command, 17);
  //spell_assignment(CLASS_CLERIC, air walker, 17);
  /*epic spells*/
  spell_assignment(CLASS_CLERIC, SPELL_MUMMY_DUST, 21);
  spell_assignment(CLASS_CLERIC, SPELL_DRAGON_KNIGHT, 21);
  spell_assignment(CLASS_CLERIC, SPELL_GREATER_RUIN, 21);
  spell_assignment(CLASS_CLERIC, SPELL_HELLBALL, 21);
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  //assign_feat_spell_slots(CLASS_CLERIC);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name     abrv   clr-abrv     menu-name*/
  classo(CLASS_ROGUE, "rogue", "Rog", "\twRog\tn", "t) \twRogue\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst eFeatp*/
         -1, N, N, M, 6, 0, 2, 8, Y, 0, 0,
         /*prestige spell progression*/ "none",
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
  feat_assignment(CLASS_ROGUE, FEAT_SNEAK_ATTACK, Y, 11, Y);
  feat_assignment(CLASS_ROGUE, FEAT_TRAP_SENSE, Y, 12, Y);
  /* talent lvl 12, apply poison */
  feat_assignment(CLASS_ROGUE, FEAT_APPLY_POISON, Y, 12, N);
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
         -1, N, N, M, 8, 0, 2, 4, Y, 0, 0,
         /*prestige spell progression*/ "none",
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
  feat_assignment(CLASS_MONK, FEAT_STILL_MIND, Y, 3, N);
  feat_assignment(CLASS_MONK, FEAT_KI_STRIKE, Y, 4, Y);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 4, Y);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 5, Y);
  feat_assignment(CLASS_MONK, FEAT_PURITY_OF_BODY, Y, 5, N);
  feat_assignment(CLASS_MONK, FEAT_SPRING_ATTACK, Y, 5, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 6, Y);
  feat_assignment(CLASS_MONK, FEAT_WHOLENESS_OF_BODY, Y, 7, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 8, Y);
  feat_assignment(CLASS_MONK, FEAT_IMPROVED_EVASION, Y, 9, N);
  feat_assignment(CLASS_MONK, FEAT_KI_STRIKE, Y, 10, Y);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 10, Y);
  feat_assignment(CLASS_MONK, FEAT_DIAMOND_BODY, Y, 11, N);
  feat_assignment(CLASS_MONK, FEAT_GREATER_FLURRY, Y, 11, N);
  feat_assignment(CLASS_MONK, FEAT_ABUNDANT_STEP, Y, 12, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 12, Y);
  feat_assignment(CLASS_MONK, FEAT_DIAMOND_SOUL, Y, 13, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 14, Y);
  feat_assignment(CLASS_MONK, FEAT_QUIVERING_PALM, Y, 15, N);
  feat_assignment(CLASS_MONK, FEAT_KI_STRIKE, Y, 15, Y);
  feat_assignment(CLASS_MONK, FEAT_TIMELESS_BODY, Y, 16, N);
  /* note this feat does nothing currently */
  feat_assignment(CLASS_MONK, FEAT_TONGUE_OF_THE_SUN_AND_MOON, Y, 17, N);
  feat_assignment(CLASS_MONK, FEAT_SLOW_FALL, Y, 18, Y);
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
                     CC, CC, CC, CA, CC, CA, CA,
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
  //spell_assignment(SPELL_REINCARNATE, 7);
  /*              class num      spell                   level acquired */
  /* 5th circle */
  //spell_assignment(CLASS_DRUID, SPELL_BALEFUL_POLYMORPH, 9);
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
  /*              class num      spell                   level acquired */
  /* 7th circle */
  spell_assignment(CLASS_DRUID, SPELL_CONTROL_WEATHER, 13);
  spell_assignment(CLASS_DRUID, SPELL_CREEPING_DOOM, 13);
  spell_assignment(CLASS_DRUID, SPELL_FIRE_STORM, 13);
  spell_assignment(CLASS_DRUID, SPELL_HEAL, 13);
  spell_assignment(CLASS_DRUID, SPELL_MASS_CURE_MODERATE, 13);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_7, 13);
  spell_assignment(CLASS_DRUID, SPELL_SUNBEAM, 13);
  //spell_assignment(CLASS_DRUID, SPELL_GREATER_SCRYING, 13);
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
  /*              class num      spell                   level acquired */
  /* 9th circle */
  spell_assignment(CLASS_DRUID, SPELL_ELEMENTAL_SWARM, 17);
  spell_assignment(CLASS_DRUID, SPELL_REGENERATION, 17);
  spell_assignment(CLASS_DRUID, SPELL_MASS_CURE_CRIT, 17);
  spell_assignment(CLASS_DRUID, SPELL_SHAMBLER, 17);
  spell_assignment(CLASS_DRUID, SPELL_POLYMORPH, 17);
  spell_assignment(CLASS_DRUID, SPELL_STORM_OF_VENGEANCE, 17);
  spell_assignment(CLASS_DRUID, SPELL_SUMMON_NATURES_ALLY_9, 17);
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
  //assign_feat_spell_slots(CLASS_DRUID);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number        name      abrv   clr-abrv           menu-name*/
  classo(CLASS_BERSERKER, "berserker", "Bes", "\trB\tRe\trs\tn", "b) \trBer\tRser\trker\tn",
         /* max-lvl  lock? prestige? BAB HD  psp move trains in-game? unlkCst, eFeatp */
         -1, N, N, H, 12, 0, 2, 4, Y, 0, 0,
         /*prestige spell progression*/ "none",
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
  feat_assignment(CLASS_BERSERKER, FEAT_RP_SUPRISE_ACCURACY, Y, 3, N);
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
         -1, N, N, L, 4, 0, 1, 2, Y, 0, 0,
         /*prestige spell progression*/ "none",
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
  spell_assignment(CLASS_SORCERER, SPELL_ENCHANT_WEAPON, 1);
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
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_SORCERER, SPELL_SHOCKING_GRASP, 4);
  spell_assignment(CLASS_SORCERER, SPELL_SCORCHING_RAY, 4);
  spell_assignment(CLASS_SORCERER, SPELL_CONTINUAL_FLAME, 4);
  spell_assignment(CLASS_SORCERER, SPELL_SUMMON_CREATURE_2, 4);
  spell_assignment(CLASS_SORCERER, SPELL_WEB, 4);
  spell_assignment(CLASS_SORCERER, SPELL_ACID_ARROW, 4);
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
  //spell_assignment(CLASS_SORCERER, SPELL_I_DARKNESS,        4);
  spell_assignment(CLASS_SORCERER, SPELL_RESIST_ENERGY, 4);
  spell_assignment(CLASS_SORCERER, SPELL_ENERGY_SPHERE, 4);
  spell_assignment(CLASS_SORCERER, SPELL_ENDURANCE, 4);
  spell_assignment(CLASS_SORCERER, SPELL_STRENGTH, 4);
  spell_assignment(CLASS_SORCERER, SPELL_GRACE, 4);
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
  /*              class num      spell                   level acquired */
  /* 4th circle */
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
  spell_assignment(CLASS_SORCERER, SPELL_GATE, 18);
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
  //assign_feat_spell_slots(CLASS_SORCERER);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number   name      abrv   clr-abrv     menu-name*/
  classo(CLASS_PALADIN, "paladin", "Pal", "\tWPal\tn", "p) \tWPaladin\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         -1, N, N, H, 10, 0, 1, 2, Y, 0, 0,
         /*prestige spell progression*/ "none",
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
  feat_assignment(CLASS_PALADIN, FEAT_TURN_UNDEAD, Y, 3, N);
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
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 12, Y);
  /* bonus feat - mounted archery 13 */
  feat_assignment(CLASS_PALADIN, FEAT_MOUNTED_ARCHERY, Y, 13, N);
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 14, Y);
  feat_assignment(CLASS_PALADIN, FEAT_SMITE_EVIL, Y, 15, Y);
  feat_assignment(CLASS_PALADIN, FEAT_REMOVE_DISEASE, Y, 18, Y);
  /* bonus feat - glorious rider 19 */
  feat_assignment(CLASS_PALADIN, FEAT_GLORIOUS_RIDER, Y, 19, N);
  feat_assignment(CLASS_PALADIN, FEAT_SMITE_EVIL, Y, 19, Y);
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
  /* paladin has no class feats */
  /**** spell assign ****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  spell_assignment(CLASS_PALADIN, SPELL_CURE_LIGHT, 6);
  spell_assignment(CLASS_PALADIN, SPELL_ENDURANCE, 6);
  spell_assignment(CLASS_PALADIN, SPELL_ARMOR, 6);
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_PALADIN, SPELL_CREATE_FOOD, 10);
  spell_assignment(CLASS_PALADIN, SPELL_CREATE_WATER, 10);
  spell_assignment(CLASS_PALADIN, SPELL_DETECT_POISON, 10);
  spell_assignment(CLASS_PALADIN, SPELL_CURE_MODERATE, 10);
  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_PALADIN, SPELL_DETECT_ALIGN, 12);
  spell_assignment(CLASS_PALADIN, SPELL_CURE_BLIND, 12);
  spell_assignment(CLASS_PALADIN, SPELL_BLESS, 12);
  spell_assignment(CLASS_PALADIN, SPELL_CURE_SERIOUS, 12);
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_PALADIN, SPELL_AID, 15);
  spell_assignment(CLASS_PALADIN, SPELL_INFRAVISION, 15);
  spell_assignment(CLASS_PALADIN, SPELL_REMOVE_CURSE, 15);
  spell_assignment(CLASS_PALADIN, SPELL_REMOVE_POISON, 15);
  spell_assignment(CLASS_PALADIN, SPELL_CURE_CRITIC, 15);
  spell_assignment(CLASS_PALADIN, SPELL_HOLY_SWORD, 15);
  /* class prerequisites */
  class_prereq_align(CLASS_PALADIN, LAWFUL_GOOD);
  /*****/
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  //assign_feat_spell_slots(CLASS_PALADIN);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name      abrv   clr-abrv     menu-name*/
  classo(CLASS_RANGER, "ranger", "Ran", "\tYRan\tn", "r) \tYRanger\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp */
         -1, N, N, H, 10, 0, 3, 4, Y, 0, 0,
         /*prestige spell progression*/ "none",
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
  feat_assignment(CLASS_RANGER, FEAT_SWIFT_TRACKER, Y, 9, N);
  feat_assignment(CLASS_RANGER, FEAT_EVASION, Y, 10, N);
  feat_assignment(CLASS_RANGER, FEAT_FAVORED_ENEMY_AVAILABLE, Y, 10, Y);
  /*CM*/
  feat_assignment(CLASS_RANGER, FEAT_GREATER_DUAL_WEAPON_FIGHTING, Y, 11, N);
  /*CM*/
  feat_assignment(CLASS_RANGER, FEAT_MANYSHOT, Y, 12, N);
  feat_assignment(CLASS_RANGER, FEAT_CAMOUFLAGE, Y, 13, N);
  feat_assignment(CLASS_RANGER, FEAT_TRACK, Y, 14, N);
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
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_RANGER, SPELL_ENDURANCE, 10);
  spell_assignment(CLASS_RANGER, SPELL_BARKSKIN, 10);
  spell_assignment(CLASS_RANGER, SPELL_GRACE, 10);
  spell_assignment(CLASS_RANGER, SPELL_HOLD_ANIMAL, 10);
  spell_assignment(CLASS_RANGER, SPELL_WISDOM, 10);
  spell_assignment(CLASS_RANGER, SPELL_STRENGTH, 10);
  spell_assignment(CLASS_RANGER, SPELL_SUMMON_NATURES_ALLY_2, 10);
  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_RANGER, SPELL_SPIKE_GROWTH, 12);
  spell_assignment(CLASS_RANGER, SPELL_GREATER_MAGIC_FANG, 12);
  spell_assignment(CLASS_RANGER, SPELL_CONTAGION, 12);
  spell_assignment(CLASS_RANGER, SPELL_CURE_MODERATE, 12);
  spell_assignment(CLASS_RANGER, SPELL_SUMMON_NATURES_ALLY_3, 12);
  spell_assignment(CLASS_RANGER, SPELL_REMOVE_DISEASE, 12);
  spell_assignment(CLASS_RANGER, SPELL_REMOVE_POISON, 12);
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_RANGER, SPELL_SUMMON_NATURES_ALLY_4, 15);
  spell_assignment(CLASS_RANGER, SPELL_FREE_MOVEMENT, 15);
  spell_assignment(CLASS_RANGER, SPELL_DISPEL_MAGIC, 15);
  spell_assignment(CLASS_RANGER, SPELL_CURE_SERIOUS, 15);
  /* no prereqs! */
  /*****/
  /* INIT spell slots, assignement of spell slots based on
     tables in constants.c */
  //assign_feat_spell_slots(CLASS_RANGER);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number  name   abrv   clr-abrv     menu-name*/
  classo(CLASS_BARD, "bard", "Bar", "\tCBar\tn", "a) \tCBard\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp */
         -1, N, N, M, 6, 0, 2, 6, Y, 0, 0,
         /*prestige spell progression*/ "none",
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
                     "the most, and being the best might they claim the treasures of each.");
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
  feat_assignment(CLASS_BARD, FEAT_BARD_1ST_CIRCLE, Y, 3, N);
  feat_assignment(CLASS_BARD, FEAT_BARD_2ND_CIRCLE, Y, 5, N);
  feat_assignment(CLASS_BARD, FEAT_BARD_3RD_CIRCLE, Y, 8, N);
  feat_assignment(CLASS_BARD, FEAT_BARD_4TH_CIRCLE, Y, 11, N);
  feat_assignment(CLASS_BARD, FEAT_BARD_5TH_CIRCLE, Y, 14, N);
  feat_assignment(CLASS_BARD, FEAT_BARD_6TH_CIRCLE, Y, 17, N);
  /*epic*/
  feat_assignment(CLASS_BARD, FEAT_BARD_EPIC_SPELL, Y, 21, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_DRAGONS, Y, 22, N);
  feat_assignment(CLASS_BARD, FEAT_SONG_OF_THE_MAGI, Y, 26, N);
  /* no class feat assignments */
  /**** spell assign ****/
  /*              class num      spell                   level acquired */
  /* 1st circle */
  spell_assignment(CLASS_BARD, SPELL_HORIZIKAULS_BOOM, 3);
  spell_assignment(CLASS_BARD, SPELL_SHIELD, 3);
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_1, 3);
  spell_assignment(CLASS_BARD, SPELL_CHARM, 3);
  spell_assignment(CLASS_BARD, SPELL_ENDURE_ELEMENTS, 3);
  spell_assignment(CLASS_BARD, SPELL_PROT_FROM_EVIL, 3);
  spell_assignment(CLASS_BARD, SPELL_PROT_FROM_GOOD, 3);
  spell_assignment(CLASS_BARD, SPELL_MAGIC_MISSILE, 3);
  spell_assignment(CLASS_BARD, SPELL_CURE_LIGHT, 3);
  /*              class num      spell                   level acquired */
  /* 2nd circle */
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_2, 5);
  spell_assignment(CLASS_BARD, SPELL_DEAFNESS, 5);
  spell_assignment(CLASS_BARD, SPELL_HIDEOUS_LAUGHTER, 5);
  spell_assignment(CLASS_BARD, SPELL_MIRROR_IMAGE, 5);
  spell_assignment(CLASS_BARD, SPELL_DETECT_INVIS, 5);
  spell_assignment(CLASS_BARD, SPELL_DETECT_MAGIC, 5);
  spell_assignment(CLASS_BARD, SPELL_INVISIBLE, 5);
  spell_assignment(CLASS_BARD, SPELL_ENDURANCE, 5);
  spell_assignment(CLASS_BARD, SPELL_STRENGTH, 5);
  spell_assignment(CLASS_BARD, SPELL_GRACE, 5);
  spell_assignment(CLASS_BARD, SPELL_CURE_MODERATE, 5);
  /*              class num      spell                   level acquired */
  /* 3rd circle */
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_3, 8);
  spell_assignment(CLASS_BARD, SPELL_LIGHTNING_BOLT, 8);
  spell_assignment(CLASS_BARD, SPELL_DEEP_SLUMBER, 8);
  spell_assignment(CLASS_BARD, SPELL_HASTE, 8);
  spell_assignment(CLASS_BARD, SPELL_CIRCLE_A_EVIL, 8);
  spell_assignment(CLASS_BARD, SPELL_CIRCLE_A_GOOD, 8);
  spell_assignment(CLASS_BARD, SPELL_CUNNING, 8);
  spell_assignment(CLASS_BARD, SPELL_WISDOM, 8);
  spell_assignment(CLASS_BARD, SPELL_CHARISMA, 8);
  spell_assignment(CLASS_BARD, SPELL_CURE_SERIOUS, 8);
  /*              class num      spell                   level acquired */
  /* 4th circle */
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_4, 11);
  spell_assignment(CLASS_BARD, SPELL_GREATER_INVIS, 11);
  spell_assignment(CLASS_BARD, SPELL_RAINBOW_PATTERN, 11);
  spell_assignment(CLASS_BARD, SPELL_REMOVE_CURSE, 11);
  spell_assignment(CLASS_BARD, SPELL_ICE_STORM, 11);
  spell_assignment(CLASS_BARD, SPELL_CURE_CRITIC, 11);
  /*              class num      spell                   level acquired */
  /* 5th circle */
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_5, 14);
  spell_assignment(CLASS_BARD, SPELL_ACID_SHEATH, 14);
  spell_assignment(CLASS_BARD, SPELL_CONE_OF_COLD, 14);
  spell_assignment(CLASS_BARD, SPELL_NIGHTMARE, 14);
  spell_assignment(CLASS_BARD, SPELL_MIND_FOG, 14);
  spell_assignment(CLASS_BARD, SPELL_MASS_CURE_LIGHT, 14);
  /*              class num      spell                   level acquired */
  /* 6th circle */
  spell_assignment(CLASS_BARD, SPELL_SUMMON_CREATURE_7, 17);
  spell_assignment(CLASS_BARD, SPELL_FREEZING_SPHERE, 17);
  spell_assignment(CLASS_BARD, SPELL_GREATER_HEROISM, 17);
  spell_assignment(CLASS_BARD, SPELL_MASS_CURE_MODERATE, 17);
  spell_assignment(CLASS_BARD, SPELL_STONESKIN, 17);
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
  //assign_feat_spell_slots(CLASS_BARD);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_WEAPON_MASTER, "weaponmaster", "WpM", "\tcWpM\tn", "e) \tcWeaponMaster\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, H, 10, 0, 1, 2, Y, 5000, 0,
         /*prestige spell progression*/ "none",
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
         10, Y, Y, M, 6, 0, 2, 4, Y, 5000, 0,
         /*prestige spell progression*/ "Arcane advancement every level",
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
                     CC, CC, CC, CA, CA, CA, CA, CC,
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
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_SNEAK_ATTACK, Y, 8, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_MAGICAL_AMBUSH, Y, 9, N);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_SNEAK_ATTACK, Y, 10, Y);
  feat_assignment(CLASS_ARCANE_SHADOW, FEAT_SURPRISE_SPELLS, Y, 10, N);
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
  classo(CLASS_SACRED_FIST, "sacredfist", "SaF", "\tGSa\tgF\tn", "n) \tGSacred\tgFist\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, M, 8, 0, 4, 4, Y, 5000, 0,
         /*prestige spell progression*/ "Divine advancement every level",
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
                     CC, CC, CC, CA, CA, CA, CC, CC,
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
         10, Y, Y, M, 8, 0, 1, 4, N, 5000, 0,
         /*prestige spell progression*/ "none",
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
                     CC, CA, CC, CC);
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
                      "the God of Shifting",      /* <= LVL_GRSTAFF */
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
  class_prereq_class_level(CLASS_SHIFTER, CLASS_DRUID, 6);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_DUELIST, "duelist", "Due", "\tcDue\tn", "i) \tcDuelist\tn",
         /* max-lvl  lock? prestige? BAB HD psp move trains in-game? unlkCst, eFeatp*/
         10, Y, Y, H, 10, 0, 1, 4, Y, 5000, 0,
         /*prestige spell progression*/ "none",
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
  spell_assignment(CLASS_ALCHEMIST, SPELL_RESIST_ENERGY, 4);
  spell_assignment(CLASS_ALCHEMIST, SPELL_DETECT_INVIS, 4);

  /* concoction circle 3 */
  spell_assignment(CLASS_ALCHEMIST, SPELL_CURE_SERIOUS, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_DISPLACEMENT, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_FLY, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_HASTE, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_CURE_BLIND, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_CURE_DEAFNESS, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_REMOVE_CURSE, 7);
  spell_assignment(CLASS_ALCHEMIST, SPELL_REMOVE_DISEASE, 7);

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
  feat_assignment(CLASS_ALCHEMIST, FEAT_APPLY_POISON, Y, 2, Y);
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

  /* no spell assignment */
  /* class prereqs */
  /****************************************************************************/

  /****************************************************************************/
  /****************************************************************************/

  /****************************************************************************/
  /****************************************************************************/
}

/** LOCAL UNDEFINES **/
// good/bad
#undef G //good
#undef B //bad
// yes/no
#undef Y //yes
#undef N //no
// high/medium/low
#undef H //high
#undef M //medium
#undef L //low
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
