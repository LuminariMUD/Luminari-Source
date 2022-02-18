/*****************************************************************************
 ** feats.c                                       Part of LuminariMUD        **
 ** Source code for the LuminariMUD Feats System.                            **
 ** Initial code by Gicker (Stephen Squires), Ported by Ornir to Luminari    **
 *****************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "modify.h"
#include "feats.h"
#include "class.h"
#include "mud_event.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "spell_prep.h"

/* Local Functions */
/* Prerequisite definition procedures */
/* Global Variables and Structures */
int feat_sort_info[MAX_FEATS];
struct feat_info feat_list[NUM_FEATS];
/* END */

/* START */
void free_feats(void)
{
} /* Nothing to do right now.  What, for shutdown maybe? */

/* Helper function for t sort_feats function - not very robust and should not be reused.
 * SCARY pointer stuff! */
int compare_feats(const void *x, const void *y)
{
  int a = *(const int *)x,
      b = *(const int *)y;

  return strcmp(feat_list[a].name, feat_list[b].name);
}

/* sort feats called at boot up */
void sort_feats(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a < NUM_FEATS; a++)
    feat_sort_info[a] = a;

  qsort(&feat_sort_info[1], NUM_FEATS, sizeof(int), compare_feats);
}

/* we use this for checking requirements and levelup struct */
int has_feat_requirement_check(struct char_data *ch, int featnum)
{
  if (ch->desc && LEVELUP(ch))
  { /* check if he's in study mode */
    return (HAS_FEAT(ch, featnum) + LEVELUP(ch)->feats[featnum]);
  }

  return (HAS_FEAT(ch, featnum));
}

/* our main tool for checking if someone has a feat is the macro:
     HAS_FEAT(ch, feat-number)
   which is just the utility function in utils.c:
     get_feat_value(ch, feat-number) */

/* checks if ch has feat (compare) as one of his/her combat feats (cfeat) */
bool has_combat_feat(struct char_data *ch, int cfeat, int compare)
{
  if (ch->desc && LEVELUP(ch))
  {
    if ((IS_SET_AR(LEVELUP(ch)->combat_feats[(cfeat)], (compare))))
      return TRUE;
  }

  if ((IS_SET_AR((ch)->char_specials.saved.combat_feats[(cfeat)], (compare))))
    return TRUE;

  return FALSE;
}

/* checks if ch has feat (compare) as one of his/her spells (school) feats (sfeat) [unfinished] */
/*
bool has_spell_feat(struct char_data *ch, int sfeat, int school) {
  if (ch->desc && LEVELUP(ch)) {
    if ((IS_SET_AR(LEVELUP(ch)->combat_feats[(sfeat)], (school))))
      return TRUE;
  }

  if ((IS_SET_AR((ch)->char_specials.saved.spell_feats[(sfeat)], (compare))))
    return TRUE;

  return FALSE;
}
 */

/* create/allocate memory for a pre-req struct, then assign the prereqs */
struct feat_prerequisite *create_prerequisite(int prereq_type, int val1, int val2, int val3)
{
  struct feat_prerequisite *prereq = NULL;

  CREATE(prereq, struct feat_prerequisite, 1);
  prereq->prerequisite_type = prereq_type;
  prereq->values[0] = val1;
  prereq->values[1] = val2;
  prereq->values[2] = val3;

  return prereq;
}

/*  The following procedures are used to define feat prerequisites.
 *  These prerequisites are automatically checked, if they exist.
 *  Dynamically assigning prerequisites also allows us to create
 *  dynamic 'help' and easier to read presentations of feat lists. */
void feat_prereq_attribute(int featnum, int attribute, int value)
{
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  const char *attribute_abbr[7] = {
      "None",
      "Str",
      "Dex",
      "Int",
      "Wis",
      "Con",
      "Cha"};

  prereq = create_prerequisite(FEAT_PREREQ_ATTRIBUTE, attribute, value, 0);

  /* Generate the description. */
  snprintf(buf, sizeof(buf), "%s : %d", attribute_abbr[attribute], value);
  prereq->description = strdup(buf);

  /*  Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}

void feat_prereq_class_level(int featnum, int cl, int level)
{
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_CLASS_LEVEL, cl, level, 0);

  /* Generate the description. */
  snprintf(buf, sizeof(buf), "%s level %d", class_names[cl], level);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}

void feat_prereq_feat(int featnum, int feat, int ranks)
{
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_FEAT, feat, ranks, 0);

  /* Generate the description. */
  if (ranks > 1)
    snprintf(buf, sizeof(buf), "%s (%d ranks)", feat_list[feat].name, ranks);
  else
    snprintf(buf, sizeof(buf), "%s", feat_list[feat].name);

  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}

void feat_prereq_cfeat(int featnum, int feat)
{
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_CFEAT, feat, 0, 0);

  snprintf(buf, sizeof(buf), "%s (may require same weapon)", feat_list[feat].name);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}

void feat_prereq_ability(int featnum, int ability, int ranks)
{
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_ABILITY, ability, ranks, 0);

  snprintf(buf, sizeof(buf), "%d ranks in %s", ranks, ability_names[ability]);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}

void feat_prereq_spellcasting(int featnum, int casting_type, int prep_type, int circle)
{
  struct feat_prerequisite *prereq = NULL;
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

  prereq = create_prerequisite(FEAT_PREREQ_SPELLCASTING, casting_type, prep_type,
                               circle);

  snprintf(buf, sizeof(buf), "Ability to cast %s %s spells", casting_types[casting_type],
           spell_preparation_types[prep_type]);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}

void feat_prereq_race(int featnum, int race)
{
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_RACE, race, 0, 0);

  snprintf(buf, sizeof(buf), "Race: %s", race_list[race].type);
  prereq->description = strdup(buf);

  /*   Link it up. */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}

void feat_prereq_bab(int featnum, int bab)
{
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_BAB, bab, 0, 0);

  snprintf(buf, sizeof(buf), "BAB +%d", bab);
  prereq->description = strdup(buf);

  /* Link it up */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}

void feat_prereq_weapon_proficiency(int featnum)
{
  struct feat_prerequisite *prereq = NULL;
  char buf[80];

  prereq = create_prerequisite(FEAT_PREREQ_WEAPON_PROFICIENCY, 0, 0, 0);

  snprintf(buf, sizeof(buf), "Proficiency in same weapon");
  prereq->description = strdup(buf);

  /*  Link it up */
  prereq->next = feat_list[featnum].prerequisite_list;
  feat_list[featnum].prerequisite_list = prereq;
}

/* ASSIGNING FEATS - Below are the various feat initializations :
 *   1) feat number, defined in structs.h
 *   2) displayed name of the feat
 *   3) in the game or not, and thus can be learned and displayed
 *   4) learned through a trainer (study menu) or whether it is a feat given automatically
 *      to certain classes or races.
 *   5) feat can stack with itself.
 *   6) feat type, for organization in the selection menu
 *   7) short description of the feat.
 *   8) long description of the feat.   */

/* utility functions for assigning "specials" to individual feats */
void epicfeat(int featnum)
{
  feat_list[featnum].epic = TRUE;
}

void combatfeat(int featnum)
{
  feat_list[featnum].combat_feat = TRUE;
}

void dailyfeat(int featnum, event_id event)
{
  feat_list[featnum].event = event;
}

/* function to assign basic attributes to feat */
static void feato(int featnum, const char *name, int in_game, int can_learn, int can_stack, int feat_type, const char *short_description, const char *description)
{
  feat_list[featnum].name = name;
  feat_list[featnum].in_game = in_game;
  feat_list[featnum].can_learn = can_learn;
  feat_list[featnum].can_stack = can_stack;
  feat_list[featnum].feat_type = feat_type;
  feat_list[featnum].short_description = short_description;
  feat_list[featnum].description = description;
  feat_list[featnum].prerequisite_list = NULL;
}

void initialize_feat_list(void)
{
  int i;

  /* initialize the list of feats */
  for (i = 0; i < NUM_FEATS; i++)
  {
    feat_list[i].name = "Unused Feat";
    feat_list[i].in_game = FALSE;
    feat_list[i].can_learn = FALSE;
    feat_list[i].can_stack = FALSE;
    feat_list[i].feat_type = FEAT_TYPE_NONE;
    feat_list[i].short_description = "ask staff";
    feat_list[i].description = "ask staff";
    feat_list[i].epic = FALSE;
    feat_list[i].combat_feat = FALSE;
    feat_list[i].prerequisite_list = NULL;
    feat_list[i].event = eNULL; /* Set all feats to eNULL event as default. */
  }
}

/* primary function for assigning feats */
void assign_feats(void)
{

  initialize_feat_list();

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /************************************/
  /* NPC Racial Feats (and wildshape) */

  feato(FEAT_NATURAL_ATTACK, "natural attack", TRUE, FALSE, TRUE, FEAT_TYPE_INNATE_ABILITY,
        "proficiency with your natural attack",
        "Depending on the level and size of the shifter, the natural attack of the "
        "shifted form will get more powerful and accurate.  The number of dice "
        "rolled for natural attack is determined by the number of ranks the wildshaper "
        "has in this feat.  The size of dice is determined by the size of the race the "
        "wildshaper turns into.  The wildshaper gets an additional natural-attack-ranks/2 "
        "bonus to their accuracy in the form of enhancement bonus OR whatever enhancement "
        "bonus the weapon they were wielding when they shapechanged - whichever is higher.");
  feato(FEAT_NATURAL_TRACKER, "natural tracker", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "naturally able to track",
        "Without this feat (or similar) you are unable to track opponents.");
  feato(FEAT_POISON_BITE, "poison bite", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "your bite can cause poison",
        "When attacking an opponent, you have a high chance of inflicting poison through your bite.");

  /*************************/
  /* NPC Racial Feats Shared Elsewhere */

  /******/
  /* Racial ability feats */

  /* Human */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_QUICK_TO_MASTER, "quick to master", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "extra starting feat",
        "You start off with an extra feat.");
  feato(FEAT_SKILLED, "skilled", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "gain bonus skill point",
        "4 extra skill points at 1st level, plus 1 additional skill point at each level up");

  /* Dwarf */
  feato(FEAT_SPELL_HARDINESS, "hardiness", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 spell save versus damaging spells",
        "+2 spell save versus damaging spells");
  feato(FEAT_DWARF_RACIAL_ADJUSTMENT, "dwarf racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 con -2 cha",
        "As a racial adjustment you have +2 to constitution and -2 to charisma");

  /* Halfling */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_SHADOW_HOPPER, "shadow hopper", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 to stealth",
        "+2 to stealth");
  feato(FEAT_LUCKY, "lucky", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+1 bonus to all saves",
        "+1 bonus to all saves");
  feato(FEAT_HALFLING_RACIAL_ADJUSTMENT, "halfling racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 dex -2 str",
        "You gain +2 to dexterity and -2 strength as racial stat adjustments.");

  /* Half-Elf */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_HALF_BLOOD, "half blood", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 discipline and lore",
        "+2 discipline and lore");

  /* Half-Orc */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_HALF_ORC_RACIAL_ADJUSTMENT, "halforc racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 str, -2 int/cha",
        "Half-Orcs as a racial adjustment have +2 strength and -2 intelligence/charisma.");

  /* Gnome */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_RESISTANCE_TO_ILLUSIONS, "resistance to illusions", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 saving throw bonus against illusions",
        "+2 saving throw bonus against illusions");
  feato(FEAT_ILLUSION_AFFINITY, "illusion affinity", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 DC opponents saves versus their illusions",
        "+2 DC opponents saves versus their illusions");
  feato(FEAT_TINKER_FOCUS, "tinker focus", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 concentration and use magic device",
        "+2 concentration and use magic device");
  feato(FEAT_GNOME_RACIAL_ADJUSTMENT, "gnome racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 con -2 str",
        "Gnomes as a racial adjustment have +2 constitution and -2 strength.");

  /* Elf */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_SLEEP_ENCHANTMENT_IMMUNITY, "sleep enchantment immunity", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "immunity to sleep enchantments",
        "immunity to sleep enchantments");
  feato(FEAT_ELF_RACIAL_ADJUSTMENT, "elf racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 dex -2 con",
        "Elven racial adjustment to stats are: +2 dexterity -2 constitution.");

  /* Half-Troll */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_TROLL_REGENERATION, "troll regeneration", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "rapid health regeneration",
        "Half-Trolls recover health much quicker than other races, and this effect is even more dramatic during combat.");
  feato(FEAT_WEAKNESS_TO_FIRE, "weakness to fire", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "50 percent vulnerability to fire",
        "50 percent vulnerability to fire");
  feato(FEAT_WEAKNESS_TO_ACID, "weakness to acid", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "25 percent vulnerability to acid",
        "25 percent vulnerability to acid");
  feato(FEAT_STRONG_AGAINST_POISON, "strong against poison", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "25 percent resist poison",
        "25 percent resist poison");
  feato(FEAT_STRONG_AGAINST_DISEASE, "strong against disease", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "50 percent resist disease",
        "50 percent resist disease");
  feato(FEAT_HALF_TROLL_RACIAL_ADJUSTMENT, "halftroll racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 str/dex/con -4 int/wis/cha",
        "As racial stat adjustments Half-Trolls get: +2 to strength/dexterity/constitution and -4 to intelligence/wisdom/charisma.");

  /* Arcana Golem */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_SPELLBATTLE, "spellbattle", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "strengthen your body with arcane power",
        "By channeling their inner magic, Arcana Golems can use it to provide a huge "
        "surge to their physical attributes in the rare cases in which they "
        "must resort to physical violence. While the eldritch energies cloud "
        "their mind and finesse, the bonus to durability and power can provide "
        "that edge when need.  Spell Battle is a mode. When this mode is activated, "
        "you receive a penalty to attack rolls equal to the argument and caster "
        "levels equal to half. In return, you gain a bonus to AC and saving throws "
        "equal to the penalty taken, a bonus to BAB equal to 2/3rds of this penalty "
        "(partially but not completely negating the attack penalty), and a bonus "
        "to maximum hit points equal to 10 * penalty taken. Arcana Golems take an "
        "additional -2 penalty to Intelligence, * Wisdom, and Charisma while in "
        "Spell Battle.  Spell battle without any arguments will cancel the mode, "
        "but an Arcana Golem can only cancel Spell Battle after spending 6 minutes "
        "in it. The surge of energy is not easily turned off.  You cannot use "
        "Spell-Battle at the same time you use Power Attack, Combat Expertise, "
        "or similar effects");
  feato(FEAT_SPELL_VULNERABILITY, "spell vulnerability", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        " -2 damaging spell saves",
        "You get a -2 penalty to all spell saves involving damage.");
  feato(FEAT_ENCHANTMENT_VULNERABILITY, "enchantment vulnerability", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "-2 enchantment saves",
        "You get a -2 penalty to all enchantment spells saves.");
  feato(FEAT_PHYSICAL_VULNERABILITY, "physical vulnerability", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        " -2 penalty to AC",
        "You get a -2 penalty to your natural armor class value.");
  feato(FEAT_MAGICAL_HERITAGE, "magical heritage", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "strong connection to magic",
        "Arcana Golem gain a 6th of their level as bonus to Caster-Level, Spellcraft Checks and Concentration Checks.");
  feato(FEAT_ARCANA_GOLEM_RACIAL_ADJUSTMENT, "arcanagolem racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "-2 con/str, +2 int/wis/cha",
        "Arcana Golem natural racial adjustment to stats are: -2 constitution/strength, +2 intelligence/wisdom/charisma.");

  /* Crystal Dwarf */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_CRYSTAL_BODY, "crystal body", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "damage reduction 3/- temporarily",
        "Allows you to harden your crystal-like body for a short time. "
        "(Damage reduction 3/-)");
  feato(FEAT_CRYSTAL_FIST, "crystal fist", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "melee +3 damage temporarily",
        "Allows you to innately grow jagged and sharp crystals on your arms and legs "
        "to enhance damage in melee. (+3 damage)");
  feato(FEAT_CRYSTAL_SKIN, "crystal skin", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "gain some resistances",
        "10 percent resistance to acid, puncture, poison and disease");
  feato(FEAT_CRYSTAL_DWARF_RACIAL_ADJUSTMENT, "crystaldwarf racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+4 con, +2 str, +2 wis, +2 cha",
        "As a natural racial bonus, crystal-dwarves start with +4 constituion, +2 to strength, wisdom and charisma.");

  /* Duergar */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_SLA_INVIS, "duergar invis", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "sla - invisibility 3/day",
        "Duergar have a spell-like ability to use invisibility (command: invisiduergar) on themselves three times per day");
  feato(FEAT_SLA_STRENGTH, "duergar strength", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "sla - strength 3/day",
        "Duergar have a spell-like ability to use strength (command: strength) on themselves three times per day");
  feato(FEAT_SLA_ENLARGE, "duergar enlarge", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "sla - enlarge 3/day",
        "Duergar have a spell-like ability to use enlarge (command: enlarge) on themselves three times per day");
  feato(FEAT_AFFINITY_MOVE_SILENT, "affinity - move silent", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+4 move silent check",
        "A strong affinity to move silently, receive a +4 bonus to all checks");
  feato(FEAT_AFFINITY_LISTEN, "affinity - listen", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 listen check",
        "An affinity to listen, receive a +2 bonus to all checks");
  feato(FEAT_AFFINITY_SPOT, "affinity - spot", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 spot check",
        "An affinity to spot, receive a +2 bonus to all checks");
  feato(FEAT_STRONG_SPELL_HARDINESS, "strong spell hardiness", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+4 to spell saves",
        "A strong hardiness in resisting spells, +4 bonus to saves against magic");
  feato(FEAT_DUERGAR_RACIAL_ADJUSTMENT, "duergar racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+4 con, -2 cha",
        "Duergar get a +4 bonus to their natural constitution and a -2 penalty to their charisma");
  feato(FEAT_PHANTASM_RESIST, "strong phantasm resist", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+4 to phantasm saves",
        "A strong hardiness in resisting phantasms, +4 bonus to saves");
  feato(FEAT_PARALYSIS_RESIST, "strong paralysis resist", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+4 to paralysis saves",
        "A strong hardiness in resisting paralysis, +4 bonus to saves - will also allow saves versus some spells/abilities "
        "that normally don't allow saves");

  /* Drow */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  // sleep enchantment immunity - shared
  // ultravision / darkvision - shared
  // keen senses - shared
  // resistance to enchantments - shared
  // light blindness - shared
  feato(FEAT_SLA_LEVITATE, "drow levitate", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "sla - levitate 3/day",
        "Drow have a spell-like ability to use 'levitate' on themselves three times per day");
  feato(FEAT_SLA_DARKNESS, "drow darkness", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "sla - darkness 3/day",
        "Drow have a spell-like ability to use 'darkness' on the current room three times per day");
  feato(FEAT_SLA_FAERIE_FIRE, "drow faerie fire", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "sla - faerie fire 3/day",
        "Drow have a spell-like ability to use 'faerie fire' on opponents three times per day");
  feato(FEAT_DROW_SPELL_RESISTANCE, "drow spell resist", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "10 + level spell resist",
        "Due to their magical nature and society, Drow have a strong natural resistance "
        "to magic.  A Drow's spell resistance is equal to 10 + their level.");
  feato(FEAT_DROW_RACIAL_ADJUSTMENT, "drow racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 dex +2 wis +2 int +2 cha -2 con",
        "Drow racial adjustment to stats are: +2 dexterity, +2 charisma, +2 wisdom, "
        "+2 intelligence and -2 constitution.");
  feato(FEAT_WEAPON_PROFICIENCY_DROW, "weapon proficiency - drow", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "gain bonus weapon proficiency",
        "As part of your drow upbringing, you were trained in the usage of "
        "hand-crossbows, rapiers and short-swords.");

  /* Lich */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  //  other lich innates that are shared, etc vital, hardy, armor skin +5, ultravision, is undead, damage resist +4, unarmed combag
  feato(FEAT_LICH_RACIAL_ADJUSTMENT, "lich racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 dex +2 con +6 int, +8 perception sense-motive stealth",
        "Lich racial adjustment to stats are: +2 dexterity, +2 constitution, +2 charisma, "
        "and +6 intelligence.  In addition they get class abilities of acrobatics and +8 racial bonus on Perception, Sense Motive, and Stealth checks");
  feato(FEAT_LICH_SPELL_RESIST, "lich spell resist", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "15 + level spell resist",
        "Due to their undead magical nature, Lich have a strong natural resistance "
        "to magic.  A Lich's spell resistance is equal to 10 + their level.");
  feato(FEAT_LICH_DAM_RESIST, "lich damage resist", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+4 general damage resist",
        "Due to their undead nature, Lich have a strong natural damage resistance. "
        " A Lich's damage resistance is 4.");
  feato(FEAT_LICH_TOUCH, "lich touch", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "lich unarmed damage and 'lichtouch'",
        "Liches have unarmed damage of 2d4 + level / 2. They can use 'lichtouch', as a standard action, "
        "to paralyze and cause negative damage 10 + level <dice> 4 + int bonus...  this touch used on undead heals "
        "double that amount.  This is usable 3x a day + int bonus");
  feato(FEAT_LICH_REJUV, "lich rejuvenation", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "saved from death 1x/day",
        "A lich upon being the target of a death blow will automatically be fully restored.  This represents "
        "the power of their phylactery.  A lich can only tap upon this awesome power 1x per day.");
  feato(FEAT_LICH_FEAR, "lich fear", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "cause fear 3x day",
        "The countenance of a Lich is so intimidating that one can cause fear (spell-like affect) up to 3x/day.");
  feato(FEAT_ELECTRIC_IMMUNITY, "immune to electricity", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "immune to electricity",
        "Completely immune to attacks based on electricity.");
  feato(FEAT_COLD_IMMUNITY, "immune to cold", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "immune to cold",
        "Completely immune to attacks based on cold.");

  /* Trelux */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_VULNERABLE_TO_COLD, "vulnerable to cold", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "20 percent weakness to cold attacks",
        "20 percent weakness to cold attacks");
  feato(FEAT_TRELUX_EXOSKELETON, "trelux exoskeleton", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "resistances and natural armor bonus",
        "Trelux have an extremely strong exoskeleton that gets harder as they mature. "
        "(They gain +1 AC-bonus every 3 levels)  Also the exoskeleton grants "
        "resistance to all damage forms except cold.");
  feato(FEAT_LEAP, "leap", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "20 percent damage avoidance",
        "Leap is a Trelux ability to completely avoid attacks 20% of the time by "
        "leaping away from danger with their powerful insect-like legs.");
  feato(FEAT_WINGS, "wings", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "able to fly",
        "Commands: 'fly' and 'land'  Those with wings are able to fly at will.");
  feato(FEAT_TRELUX_EQ, "trelux eq", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "can't use some eq slots",
        "Due to their pincer-like hands, Trelux cannot wield weapons, hold items, wear "
        "gloves, use shields or wear rings.  The shape of their head and antennae prevent "
        "the usage of helmets as well.  Finally, due to their insect-like legs, Trelux cannot "
        "wear items on their legs and feet.");
  feato(FEAT_TRELUX_PINCERS, "trelux pincers", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "trelux natural pincer weapon",
        "Trelux don't have hands, they have insect-like pincers.  These pincers can "
        "be used as dangerous weapons.  Trelux Monks gain an extra dice of damage "
        "due to these pincers.  Also, Trelux pincers have a chance of poisoning "
        "their victim - note Trelux naturally are 'dual wielding' and will have"
        "a partial but reduced penalty to hit with their 'off' pincer.  The vicousness of "
        "their pincers also gives a +1 bonus to their damroll bonus every 4 levels.");
  feato(FEAT_TRELUX_RACIAL_ADJUSTMENT, "trelux racial adjustment", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+4 dex, +2 str, +4 con",
        "As racial modifiers, Trelux gain 4 dexterity, 2 strength and 4 constitution as a natural starting bonus.");

  /* Shared - Various */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_LIGHT_BLINDNESS, "light blindness", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "penalties in daylight",
        "You receive penalties of -1 to hitroll, damroll, saves and skill checks when "
        "outdoors during the day.  Darkness spells and effects negate this penalty");
  feato(FEAT_KEEN_SENSES, "keen senses", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 perception and sense motive",
        "+2 perception and sense motive");
  feato(FEAT_WEAPON_PROFICIENCY_ELF, "weapon proficiency - elves", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "gain bonus weapon proficiency",
        "As part of your elf upbringing, you were trained in the usage of long swords, "
        "rapiers, long bows, composite bows, short bows and composite short bows.");
  feato(FEAT_ULTRAVISION, "ultravision", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "you can see perfectly even in complete dark",
        "you can see perfectly even in complete dark");
  feato(FEAT_TRUE_SIGHT, "true sight", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "you can see invisible persons or items and the true forms of shapechanged, wildshaped, disguised or polymorphed beings",
        "you can see invisible persons or items and the true forms of shapechanged, wildshaped, disguised or polymorphed beings");
  feato(FEAT_INFRAVISION, "infravision", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "you can see outlines of life in complete dark",
        "you can see outlines of life in complete dark");
  feato(FEAT_COMBAT_TRAINING_VS_GIANTS, "combat training vs giants", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+1 size bonus versus larger opponents",
        "+1 size bonus versus larger opponents");
  feato(FEAT_POISON_RESIST, "poison resist", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 saves versus poison",
        "+2 saves versus poison");
  feato(FEAT_VITAL, "vital", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "start with +10 hps",
        "start with +10 hps");
  feato(FEAT_HARDY, "hardy", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "gain an extra hit point per level",
        "gain an extra hit point per level");
  feato(FEAT_RESISTANCE_TO_ENCHANTMENTS, "resistance to enchantments", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+2 saving throw bonus against enchantments",
        "+2 saving throw bonus against enchantments");
  feato(FEAT_STABILITY, "stability", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "+4 resist to bash and trip skill",
        "+4 resist to bash and trip skill");

  /* End Racial ability feats */

  /***/
  /* Combat feats */
  /***/

  /* combat modes */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_POWER_ATTACK, "power attack", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "subtract a number from hit and add to dam.  If 2H weapon add 2x dam instead",
        "When active, take a value specified as penalty to attack roll and gain that "
        "value as damage bonus (melee / thrown weapons only).  Usage: powerattack VALUE, to turn off, "
        "just type powerattack with no argument.  VALUE must be between 1-5 and is also "
        "limited by your (BAB) base attack bonus.");
  feat_prereq_attribute(FEAT_POWER_ATTACK, AB_STR, 13);
  feato(FEAT_COMBAT_EXPERTISE, "combat expertise", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "subtract a number from attack roll and add it to AC",
        "When active, take a value specified as penalty to attack roll and gain that "
        "value as dodge bonus to your AC.  Your combat expertise value can not "
        "go over your unmodified base attack bonus.  Usage: expert VALUE, to turn off, "
        "just type expert with no argument.  VALUE must be between 1-5.");
  feat_prereq_attribute(FEAT_COMBAT_EXPERTISE, AB_INT, 13);
  /* required for whirlwind */

  /* cleave */
  feato(FEAT_CLEAVE, "cleave", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "extra attack against opponent once per round",
        "You get an extra attack against a second opponent you are fighting, once "
        "per round.");
  feat_prereq_attribute(FEAT_CLEAVE, AB_STR, 13);
  feat_prereq_feat(FEAT_CLEAVE, FEAT_POWER_ATTACK, 1);

  feato(FEAT_GREAT_CLEAVE, "great cleave", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "extra attack against opponent once per round",
        "You get an extra attack against a second opponent you are fighting, once "
        "per round.  This feat stacks with cleave, effectively giving you 2 "
        "bonus attacks on that target.");
  feat_prereq_feat(FEAT_GREAT_CLEAVE, FEAT_CLEAVE, 1);
  feat_prereq_feat(FEAT_GREAT_CLEAVE, FEAT_POWER_ATTACK, 1);
  feat_prereq_attribute(FEAT_GREAT_CLEAVE, AB_STR, 13);
  feat_prereq_bab(FEAT_GREAT_CLEAVE, 4);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* weapon focus feats */
  feato(FEAT_WEAPON_FOCUS, "weapon focus", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
        "+1 to hit rolls for selected weapon",
        "+1 to hit rolls for selected weapon");
  feat_prereq_bab(FEAT_WEAPON_FOCUS, 1);
  feat_prereq_weapon_proficiency(FEAT_WEAPON_FOCUS);
  feato(FEAT_GREATER_WEAPON_FOCUS, "greater weapon focus", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
        "+1 to hit rolls with weapon",
        "+1 to hit rolls with weapon");
  feat_prereq_cfeat(FEAT_GREATER_WEAPON_FOCUS, FEAT_WEAPON_FOCUS);
  feat_prereq_weapon_proficiency(FEAT_GREATER_WEAPON_FOCUS);
  feat_prereq_class_level(FEAT_GREATER_WEAPON_FOCUS, CLASS_WARRIOR, 8);
  feato(FEAT_WEAPON_SPECIALIZATION, "weapon specialization", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
        "+2 to dam rolls with chosen weapon",
        "Choose one type of weapon, such as greataxe, for which you have already "
        "selected the Weapon Focus feat. You can also choose unarmed strike as "
        "your weapon for purposes of this feat. You gain a +2 bonus on damage "
        "using the selected weapon.");
  feat_prereq_weapon_proficiency(FEAT_WEAPON_SPECIALIZATION);
  feat_prereq_cfeat(FEAT_WEAPON_SPECIALIZATION, FEAT_WEAPON_FOCUS);
  feat_prereq_class_level(FEAT_WEAPON_SPECIALIZATION, CLASS_WARRIOR, 4);
  feato(FEAT_GREATER_WEAPON_SPECIALIZATION, "greater weapon specialization", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "+4 damage with weapon",
        "Choose one type of weapon, such as halberd, for which you have already "
        "selected the Weapon Focus feat. You can also choose unarmed strike as "
        "your weapon for purposes of this feat. You gain a +4 bonus on damage "
        "using the selected weapon (stacks).");
  feat_prereq_weapon_proficiency(FEAT_GREATER_WEAPON_SPECIALIZATION);
  feat_prereq_cfeat(FEAT_GREATER_WEAPON_SPECIALIZATION, FEAT_WEAPON_FOCUS);
  feat_prereq_cfeat(FEAT_GREATER_WEAPON_SPECIALIZATION, FEAT_GREATER_WEAPON_FOCUS);
  feat_prereq_cfeat(FEAT_GREATER_WEAPON_SPECIALIZATION, FEAT_WEAPON_SPECIALIZATION);
  feat_prereq_class_level(FEAT_GREATER_WEAPON_SPECIALIZATION, CLASS_WARRIOR, 12);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* mobility feats */
  feato(FEAT_DODGE, "dodge", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "+1 dodge bonus to ac",
        "+1 dodge bonus to ac");
  feat_prereq_attribute(FEAT_DODGE, AB_DEX, 13);
  feato(FEAT_MOBILITY, "mobility", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "+4 dodge ac bonus against attacks of opportunity",
        "+4 dodge ac bonus against attacks of opportunity");
  feat_prereq_attribute(FEAT_MOBILITY, AB_DEX, 13);
  feat_prereq_feat(FEAT_MOBILITY, FEAT_DODGE, 1);
  feato(FEAT_SPRING_ATTACK, "spring attack", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "you can choose the direction you flee and gain springleap",
        "you can choose the direction you flee and gain springleap");
  feat_prereq_bab(FEAT_SPRING_ATTACK, 4);
  feat_prereq_attribute(FEAT_SPRING_ATTACK, AB_DEX, 13);
  feat_prereq_feat(FEAT_SPRING_ATTACK, FEAT_DODGE, 1);
  feat_prereq_feat(FEAT_SPRING_ATTACK, FEAT_MOBILITY, 1);
  feato(FEAT_WHIRLWIND_ATTACK, "whirlwind attack", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "lashes out at multiple enemies in one movement",
        "As a full round action, you can lash out and strike a number of foes based "
        "on the number of attacks you have +1.  All these attacks are made "
        "at your full attack bonus.");
  feat_prereq_attribute(FEAT_WHIRLWIND_ATTACK, AB_DEX, 13);
  feat_prereq_attribute(FEAT_WHIRLWIND_ATTACK, AB_INT, 13);
  feat_prereq_feat(FEAT_WHIRLWIND_ATTACK, FEAT_COMBAT_EXPERTISE, 1);
  feat_prereq_feat(FEAT_WHIRLWIND_ATTACK, FEAT_DODGE, 1);
  feat_prereq_feat(FEAT_WHIRLWIND_ATTACK, FEAT_MOBILITY, 1);
  feat_prereq_feat(FEAT_WHIRLWIND_ATTACK, FEAT_SPRING_ATTACK, 1);
  feat_prereq_bab(FEAT_WHIRLWIND_ATTACK, 4);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* critical feats */
  feato(FEAT_POWER_CRITICAL, "power critical", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
        "+4 to rolls to confirm critical hits.",
        "+4 to rolls to confirm critical hits.");
  feat_prereq_weapon_proficiency(FEAT_POWER_CRITICAL);
  feat_prereq_bab(FEAT_POWER_CRITICAL, 4);
  feato(FEAT_IMPROVED_CRITICAL, "improved critical", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
        "doubled critical threat rating for weapon chosen",
        "doubled critical threat rating for weapon chosen");
  feat_prereq_weapon_proficiency(FEAT_IMPROVED_CRITICAL);
  feat_prereq_bab(FEAT_IMPROVED_CRITICAL, 8);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* ranged attack feats */
  feato(FEAT_POINT_BLANK_SHOT, "point blank shot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "+1 to hit and dam rolls with ranged weapons in the same room",
        "+1 to hit and dam rolls with ranged weapons in the same room, can fight "
        "in close quarters with ranged weapon");
  feato(FEAT_FAR_SHOT, "far shot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "allows firing outside room",
        "When the 'far shot' feat is taken you are able to fire in any direction at "
        "a target in the next room.");
  feat_prereq_attribute(FEAT_FAR_SHOT, AB_DEX, 15);
  feat_prereq_feat(FEAT_FAR_SHOT, FEAT_POINT_BLANK_SHOT, 1);
  feato(FEAT_RAPID_SHOT, "rapid shot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "extra attack ranged weapon at -2 to all attacks",
        "can make extra attack per round with ranged weapon at -2 to all attacks");
  feat_prereq_attribute(FEAT_RAPID_SHOT, AB_DEX, 13);
  feat_prereq_feat(FEAT_RAPID_SHOT, FEAT_POINT_BLANK_SHOT, 1);
  feato(FEAT_MANYSHOT, "manyshot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "extra ranged attack when rapid shot turned on",
        "extra ranged attack when rapid shot turned on");
  feat_prereq_attribute(FEAT_MANYSHOT, AB_DEX, 15);
  feat_prereq_feat(FEAT_MANYSHOT, FEAT_RAPID_SHOT, 1);
  feato(FEAT_PRECISE_SHOT, "precise shot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "You may shoot in melee without the standard -4 to hit penalty",
        "You may shoot in melee without the standard -4 to hit penalty");
  feat_prereq_attribute(FEAT_PRECISE_SHOT, AB_DEX, 13);
  feato(FEAT_IMPROVED_PRECISE_SHOT, "improved precise shot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "+4 to hit on all close ranged attacks",
        "+4 to hit on all close ranged attacks");
  feat_prereq_feat(FEAT_IMPROVED_PRECISE_SHOT, FEAT_PRECISE_SHOT, 1);
  feat_prereq_bab(FEAT_IMPROVED_PRECISE_SHOT, 12);
  feato(FEAT_RAPID_RELOAD, "rapid reload", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "can load crossbows and slings rapidly",
        "The time required for you to reload your chosen type of weapon is reduced "
        "to a free action (for a hand or light crossbow), a move action (for heavy "
        "crossbow or one-handed firearm). If you have selected this feat for a hand "
        "crossbow or light crossbow, you may fire that weapon as many times in a "
        "full-attack action as you could attack if you were using a bow.");
  feato(FEAT_DEFLECT_ARROWS, "deflect arrows", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "can deflect one ranged attack per round",
        "You must have at least one hand free to use this feat. "
        "Once per round when you would normally be hit with an attack from a ranged "
        "weapon, you may deflect it so that you take no damage from it. You must be "
        "aware of the attack and not flat-footed. Attempting to deflect a ranged "
        "attack doesn't count as an action. Unusually massive ranged weapons (such "
        "as boulders or ballista bolts) and ranged attacks generated by natural "
        "attacks or spell effects can't be deflected.");
  feat_prereq_feat(FEAT_DEFLECT_ARROWS, FEAT_IMPROVED_UNARMED_STRIKE, 1);
  feat_prereq_attribute(FEAT_DEFLECT_ARROWS, AB_DEX, 13);
  feato(FEAT_SNATCH_ARROWS, "snatch arrows", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "instead of deflecting arrows you can snatch them out of the air",
        "instead of deflecting arrows you can snatch them out of the air");
  feat_prereq_attribute(FEAT_SNATCH_ARROWS, AB_DEX, 17);
  feat_prereq_feat(FEAT_SNATCH_ARROWS, FEAT_DEFLECT_ARROWS, 1);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* here is our mounted combat feats */
  feato(FEAT_MOUNTED_COMBAT, "mounted combat", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "once per round rider may negate a hit against him with a successful ride vs attack roll check",
        "once per round rider may negate a hit against him with a successful ride vs attack roll check");
  feat_prereq_ability(FEAT_MOUNTED_COMBAT, ABILITY_RIDE, 4);
  feato(FEAT_RIDE_BY_ATTACK, "ride by attack", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "normally use full round action on charge, now use move action",
        "normally use full round action on charge, now use move action");
  feat_prereq_feat(FEAT_RIDE_BY_ATTACK, FEAT_MOUNTED_COMBAT, 1);
  feat_prereq_ability(FEAT_RIDE_BY_ATTACK, ABILITY_RIDE, 6);
  feato(FEAT_SPIRITED_CHARGE, "spirited charge", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "When mounted and using charge, you deal double damage with a melee weapon (or triple damage with a lance).",
        "When mounted and using charge, you deal double damage with a melee weapon (or triple damage with a lance).");
  feat_prereq_ability(FEAT_SPIRITED_CHARGE, ABILITY_RIDE, 8);
  feat_prereq_feat(FEAT_SPIRITED_CHARGE, FEAT_MOUNTED_COMBAT, 1);
  feat_prereq_feat(FEAT_SPIRITED_CHARGE, FEAT_RIDE_BY_ATTACK, 1);
  /* end mounted combat feats */

  /* ranged attack + mounted combat feats */
  feato(FEAT_MOUNTED_ARCHERY, "mounted archery", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "no penalty for mounted archery attacks",
        "normally mounted archery combat imposes a -4 penalty to attacks, with this "
        "feat you have no penalty to your attacks");
  feat_prereq_feat(FEAT_MOUNTED_ARCHERY, FEAT_MOUNTED_COMBAT, 1);
  feat_prereq_ability(FEAT_MOUNTED_ARCHERY, ABILITY_RIDE, 6);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* shield feats */
  feato(FEAT_IMPROVED_SHIELD_PUNCH, "improved shield punch", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "retain your shield's AC bonus when you shield punch",
        "retain your shield's AC bonus when you shield punch");
  feat_prereq_feat(FEAT_IMPROVED_SHIELD_PUNCH, FEAT_ARMOR_PROFICIENCY_SHIELD, 1);
  feato(FEAT_SHIELD_CHARGE, "shield charge", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "charge with shield for damage and knockdown attempt (shieldcharge)",
        "Making an off-hand attack with your shield with a +2 bonus doing 1d6 + strength-bonus "
        "damage, you attempt to slam your shield and bodyweight "
        "into your opponent.  A successful hit will result in a knockdown attempt. "
        "To use this skill, just type: shieldcharge <opponent>");
  feat_prereq_bab(FEAT_SHIELD_CHARGE, 3);
  feat_prereq_feat(FEAT_SHIELD_CHARGE, FEAT_IMPROVED_SHIELD_PUNCH, 1);
  feato(FEAT_SHIELD_SLAM, "shield slam", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "daze an opponent of any size by slamming them with your shield",
        "You make an offhand attackroll against your opponent, if you land the attack "
        "with your shield, you will do 1d6 + strength-bonus/2 damage.  In addition if "
        "your opponent fails a fortitude save versus 10 + your-level/2 + your strength "
        "bonus, they will be dazed for a full combat round and unable to attack you.");
  feat_prereq_bab(FEAT_SHIELD_SLAM, 6);
  feat_prereq_feat(FEAT_SHIELD_SLAM, FEAT_SHIELD_CHARGE, 1);
  feat_prereq_feat(FEAT_SHIELD_SLAM, FEAT_IMPROVED_SHIELD_PUNCH, 1);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* two weapon fighting feats */
  feato(FEAT_TWO_WEAPON_FIGHTING, "two weapon fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "reduces penalty for two weapon fighting",
        "reduces penalty for two weapon fighting");
  feat_prereq_attribute(FEAT_TWO_WEAPON_FIGHTING, AB_DEX, 15);
  feato(FEAT_IMPROVED_TWO_WEAPON_FIGHTING, "improved two weapon fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "extra attack with offhand weapon at -5 penalty",
        "extra attack with offhand weapon at -5 penalty");
  feat_prereq_cfeat(FEAT_IMPROVED_TWO_WEAPON_FIGHTING, FEAT_TWO_WEAPON_FIGHTING);
  feat_prereq_attribute(FEAT_IMPROVED_TWO_WEAPON_FIGHTING, AB_DEX, 17);
  feato(FEAT_GREATER_TWO_WEAPON_FIGHTING, "greater two weapon fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "gives an additional offhand weapon attack at -10 penalty",
        "gives an additional offhand weapon attack at -10 penalty");
  feat_prereq_cfeat(FEAT_GREATER_TWO_WEAPON_FIGHTING, FEAT_IMPROVED_TWO_WEAPON_FIGHTING);
  feat_prereq_attribute(FEAT_GREATER_TWO_WEAPON_FIGHTING, AB_DEX, 19);
  feato(FEAT_TWO_WEAPON_DEFENSE, "two weapon defense", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "when wielding two weapons receive +1 shield ac bonus",
        "When dual-wielding, or using a double-weapon, you automatically get a +1 "
        "shield bonus to AC");
  feat_prereq_cfeat(FEAT_TWO_WEAPON_DEFENSE, FEAT_TWO_WEAPON_FIGHTING);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* uncategorized combat feats */
  feato(FEAT_BLIND_FIGHT, "blind fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "when fighting blind, retain dex bonus to AC and deny enemy +2 attack bonus for invisibility or other concealment.",
        "when fighting blind, retain dex bonus to AC and deny enemy +2 attack bonus for invisibility or other concealment.");

  feato(FEAT_COMBAT_REFLEXES, "combat reflexes", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "can make a number of attacks of opportunity equal to dex bonus",
        "can make a number of attacks of opportunity equal to dex bonus");

  feato(FEAT_IMPROVED_GRAPPLE, "improved grapple", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "improve your grappling",
        "You do not provoke an attack of opportunity when performing a grapple combat "
        "maneuver. In addition, you receive a +2 bonus on checks made to grapple "
        "a foe. You also receive a +2 bonus to your Combat Maneuver Defense "
        "whenever an opponent tries to grapple you.");
  feat_prereq_feat(FEAT_IMPROVED_GRAPPLE, FEAT_IMPROVED_UNARMED_STRIKE, 1);
  feat_prereq_attribute(FEAT_IMPROVED_GRAPPLE, AB_DEX, 13);

  feato(FEAT_IMPROVED_INITIATIVE, "improved initiative", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "+4 to initiative checks to see who attacks first each round",
        "+4 to initiative checks to see who attacks first each round");

  feato(FEAT_IMPROVED_TRIP, "improved trip", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "no AoO when tripping, +4 to trip, attack immediately",
        "no attack of opportunity when tripping, +4 to trip check, attack immediately "
        "on successful trip.");
  feat_prereq_attribute(FEAT_IMPROVED_TRIP, AB_INT, 13);
  feat_prereq_feat(FEAT_IMPROVED_TRIP, FEAT_COMBAT_EXPERTISE, 1);

  feato(FEAT_IMPROVED_DISARM, "improved disarm", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "opponent doesn't receive AoO",
        "You do not provoke an attack of opportunity when performing a disarm combat "
        "maneuver. In addition, you receive a +2 bonus on checks made to disarm "
        "a foe. You also receive a +2 bonus to your Combat Maneuver Defense "
        "whenever an opponent tries to disarm you.");
  feat_prereq_attribute(FEAT_IMPROVED_DISARM, AB_INT, 13);
  feat_prereq_feat(FEAT_IMPROVED_DISARM, FEAT_COMBAT_EXPERTISE, 1);
  feato(FEAT_GREATER_DISARM, "greater disarm", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "knock weapon to ground on successful disarm",
        "On a successful disarm, normally the weapon is knocked to the opponent's "
        "inventory, but with greater disarm, the weapon gets knocked into "
        "the room.");
  feat_prereq_attribute(FEAT_GREATER_DISARM, AB_INT, 13);
  feat_prereq_feat(FEAT_GREATER_DISARM, FEAT_COMBAT_EXPERTISE, 1);
  feat_prereq_feat(FEAT_GREATER_DISARM, FEAT_IMPROVED_DISARM, 1);

  feato(FEAT_IMPROVED_FEINT, "improved feint", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "feint checks get a +4 bonus, and penalties to check for low intelligence or non-humanoid targets is halved.",
        "feint checks get a +4 bonus, and penalties to check for low intelligence or non-humanoid targets is halved.");
  feat_prereq_attribute(FEAT_IMPROVED_FEINT, AB_INT, 13);
  feat_prereq_feat(FEAT_IMPROVED_FEINT, FEAT_COMBAT_EXPERTISE, 1);

  feato(FEAT_IMPROVED_TAUNTING, "improved taunting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "can taunt as a move action",
        "You can make a diplomacy check to taunt in combat as a move action as opposed "
        "to standard action.");
  feat_prereq_ability(FEAT_IMPROVED_TAUNTING, ABILITY_DIPLOMACY, 10);

  feato(FEAT_IMPROVED_INTIMIDATION, "improved intimidation", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "can intimidate as a move action",
        "You can make a intimidate check to taunt in combat as a move action as opposed "
        "to standard action.");
  feat_prereq_ability(FEAT_IMPROVED_INTIMIDATION, ABILITY_INTIMIDATE, 10);

  /* monks get this for free */
  feato(FEAT_IMPROVED_UNARMED_STRIKE, "improved unarmed strike", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "Unarmed attacks are considered to be weapons.",
        "You are considered to be armed even when unarmed, ou do not provoke attacks "
        "of opportunity when you attack foes while unarmed.  You can disarm foes without "
        "a penalty when fighting unarmed.  Also you get access to the headbutt combat maneuver.");

  /* note: monks get this for free */
  feato(FEAT_STUNNING_FIST, "stunning fist", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "may make unarmed attack to stun opponent for one round",
        "may make unarmed attack to stun opponent for one round");
  feat_prereq_attribute(FEAT_STUNNING_FIST, AB_DEX, 13);
  feat_prereq_attribute(FEAT_STUNNING_FIST, AB_WIS, 13);
  feat_prereq_feat(FEAT_STUNNING_FIST, FEAT_IMPROVED_UNARMED_STRIKE, 1);
  feat_prereq_bab(FEAT_STUNNING_FIST, 8);

  /* rogues get this feat for free */
  feato(FEAT_WEAPON_FINESSE, "weapon finesse", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "use dex for hit roll of weapons",
        "Use dexterity bonus for hit roll of weapons (if better than strength bonus), "
        "there is no benefit to this feat for archery.  Only works for 'light' classed "
        "weapons, or weapons that are a smaller size class than the character.");
  feat_prereq_bab(FEAT_WEAPON_FINESSE, 1);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* epic */
  feato(FEAT_DAMAGE_REDUCTION, "damage reduction", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
        "3/- damage reduction per rank of feat",
        "You get 3/- damage reduction per rank of feat, this stacks with other forms "
        "of damage reduction.  Note that damage reduction caps at 20.");
  feat_prereq_attribute(FEAT_DAMAGE_REDUCTION, AB_CON, 21);
  feato(FEAT_SELF_CONCEALMENT, "self concealment", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
        "10 percent miss chance for attacks against you per rank",
        "You get 10 percent miss chance for all incoming attacks against you per rank, "
        "concealment caps at 50 percent.");
  feat_prereq_ability(FEAT_SELF_CONCEALMENT, ABILITY_STEALTH, 25);
  feat_prereq_ability(FEAT_SELF_CONCEALMENT, ABILITY_ACROBATICS, 25);
  feat_prereq_attribute(FEAT_SELF_CONCEALMENT, AB_DEX, 21);
  feato(FEAT_EPIC_PROWESS, "epic prowess", TRUE, TRUE, TRUE, FEAT_TYPE_COMBAT,
        "+1 to all attacks per rank",
        "+1 to all attacks per rank");
  /* two weapon fighting feats, epic */
  feato(FEAT_PERFECT_TWO_WEAPON_FIGHTING, "perfect two weapon fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "Extra attack with offhand weapon",
        "Extra attack with offhand weapon with no penalty");
  // feat_prereq_cfeat(FEAT_PERFECT_TWO_WEAPON_FIGHTING, FEAT_GREATER_TWO_WEAPON_FIGHTING); // For some reason not returning true even when prereq is set.
  feat_prereq_feat(FEAT_PERFECT_TWO_WEAPON_FIGHTING, FEAT_GREATER_TWO_WEAPON_FIGHTING, 1);
  feat_prereq_attribute(FEAT_PERFECT_TWO_WEAPON_FIGHTING, AB_DEX, 21);
  /* archery epic feats */
  feato(FEAT_EPIC_MANYSHOT, "epic manyshot", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "extra ranged attack when rapid shot turned on",
        "extra ranged attack when rapid shot turned on");
  feat_prereq_attribute(FEAT_EPIC_MANYSHOT, AB_DEX, 19);
  feat_prereq_feat(FEAT_EPIC_MANYSHOT, FEAT_MANYSHOT, 1);
  feato(FEAT_BLINDING_SPEED, "blinding speed", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "get an extra attack, as if hasted",
        "You get an extra attack, as if hasted.");
  feat_prereq_attribute(FEAT_BLINDING_SPEED, AB_DEX, 25);
  feato(FEAT_EPIC_WEAPON_SPECIALIZATION, "epic weapon specialization", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "+3 hit/dam with weapon",
        "Choose one type of weapon, such as halberd, for which you have already "
        "selected the Weapon Focus feat. You can also choose unarmed strike as "
        "your weapon for purposes of this feat. You gain a +3 bonus on damage "
        "and attack bonus using the selected weapon (stacks).");
  feat_prereq_cfeat(FEAT_EPIC_WEAPON_SPECIALIZATION, FEAT_GREATER_WEAPON_SPECIALIZATION);
  feat_prereq_class_level(FEAT_EPIC_WEAPON_SPECIALIZATION, CLASS_WARRIOR, 20);

  /*****************/
  /* General feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* putting bard feats (not free) here */
  feato(FEAT_LINGERING_SONG, "lingering performance", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "extra round for bardic performance",
        "A bardic performance repeats every 7 seconds, and the effects of the performance "
        "last for 1 round (6 seconds).  This feat increases the duration of the "
        "effects to 2 rounds (12 seconds).");

  feato(FEAT_ENERGY_RESISTANCE, "energy resistance", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "reduces all energy related damage by 1 per rank",
        "Reduces all energy related damage by 1 per rank, this includes: fire, cold, "
        "air, earth, acid, holy, electric, unholy, slice, puncture, force, sound, "
        "poison, disease, negative, illusion, mental, light and energy.");

  feato(FEAT_FAST_HEALER, "fast healer", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "improve treatinjury ability",
        "You have become proficient in treating injuries, when using the treatinjury "
        "ability, the cooldown will be half as long.");

  /*skill focus*/
  feato(FEAT_SKILL_FOCUS, "skill focus", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "+3 in chosen skill",
        "Taking skill focus will give you +3 in chosen skill, this feat can be taken "
        "multiple times, but only once per skill chosen.");

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* weapon / armor proficiency */
  feato(FEAT_ARMOR_PROFICIENCY_LIGHT, "light armor proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "allows unpenalized use of light armor ",
        "allows unpenalized use of light armor ");
  feato(FEAT_ARMOR_PROFICIENCY_MEDIUM, "medium armor proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "allows unpenalized use of medium armor ",
        "allows unpenalized use of medium armor ");
  feat_prereq_feat(FEAT_ARMOR_PROFICIENCY_MEDIUM, FEAT_ARMOR_PROFICIENCY_LIGHT, 1);
  feato(FEAT_ARMOR_PROFICIENCY_HEAVY, "heavy armor proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "allows unpenalized use of heavy armor ",
        "allows unpenalized use of heavy armor ");
  feat_prereq_feat(FEAT_ARMOR_PROFICIENCY_HEAVY, FEAT_ARMOR_PROFICIENCY_LIGHT, 1);
  feat_prereq_feat(FEAT_ARMOR_PROFICIENCY_HEAVY, FEAT_ARMOR_PROFICIENCY_MEDIUM, 1);
  feato(FEAT_ARMOR_PROFICIENCY_SHIELD, "shield armor proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "able to use bucklers, shields without penalty ",
        "able to use bucklers, light, medium and heavy shields without penalty, does not include tower shields.  Without this proficiency, wearing a shield will incur");
  feato(FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD, "tower shield proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "can use tower shields without penalties",
        "can use tower shields without penalties");
  feat_prereq_feat(FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD, FEAT_ARMOR_PROFICIENCY_SHIELD, 1);
  feato(FEAT_SIMPLE_WEAPON_PROFICIENCY, "simple weapon proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "may use all simple weapons",
        "may use all simple weapons");
  feato(FEAT_MARTIAL_WEAPON_PROFICIENCY, "martial weapon proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "able to use all martial weapons",
        "able to use all martial weapons");
  feat_prereq_feat(FEAT_MARTIAL_WEAPON_PROFICIENCY, FEAT_SIMPLE_WEAPON_PROFICIENCY, 1);
  feato(FEAT_EXOTIC_WEAPON_PROFICIENCY, "exotic weapon proficiency", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "can use exotic weapons without penalties",
        "can use exotic weapons without penalties");
  feat_prereq_bab(FEAT_EXOTIC_WEAPON_PROFICIENCY, 2);
  feat_prereq_feat(FEAT_EXOTIC_WEAPON_PROFICIENCY, FEAT_MARTIAL_WEAPON_PROFICIENCY, 1);
  /********/

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* armor specialization */
  feato(FEAT_ARMOR_SPECIALIZATION_HEAVY, "armor specialization (heavy)", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "DR 2/- when wearing heavy armor",
        "DR 2/- when wearing heavy armor");
  feat_prereq_bab(FEAT_ARMOR_SPECIALIZATION_HEAVY, 11);
  feat_prereq_feat(FEAT_ARMOR_SPECIALIZATION_HEAVY, FEAT_ARMOR_PROFICIENCY_HEAVY, 1);
  feato(FEAT_ARMOR_SPECIALIZATION_LIGHT, "armor specialization (light)", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "DR 2/- when wearing light armor",
        "DR 2/- when wearing light armor");
  feat_prereq_bab(FEAT_ARMOR_SPECIALIZATION_LIGHT, 11);
  feat_prereq_feat(FEAT_ARMOR_SPECIALIZATION_LIGHT, FEAT_ARMOR_PROFICIENCY_LIGHT, 1);
  feato(FEAT_ARMOR_SPECIALIZATION_MEDIUM, "armor specialization (medium)", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "DR 2/- when wearing medium armor",
        "DR 2/- when wearing medium armor");
  feat_prereq_bab(FEAT_ARMOR_SPECIALIZATION_MEDIUM, 11);
  feat_prereq_feat(FEAT_ARMOR_SPECIALIZATION_MEDIUM, FEAT_ARMOR_PROFICIENCY_MEDIUM, 1);

  feato(FEAT_ABLE_LEARNER, "able learner", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+1 to all skills",
        "+1 to all skills");
  feato(FEAT_ACROBATIC, "acrobatic", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+3 to acrobatics skill checks",
        "+3 to acrobatics skill checks");
  feato(FEAT_AGILE, "agile", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to acrobatics and escape artist skill checks",
        "+2 to acrobatics and escape artist skill checks");
  feato(FEAT_ALERTNESS, "alertness", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to perception and sense motive skill checks ",
        "+2 to perception and sense motive skill checks ");
  feato(FEAT_ANIMAL_AFFINITY, "animal affinity", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to handle animal and ride skill checks",
        "+2 to handle animal and ride skill checks");

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_ATHLETIC, "athletic", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to swim and climb skill checks",
        "+2 to swim and climb skill checks");
  feato(FEAT_DECEITFUL, "deceitful", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to disguise and forgery skill checks",
        "+2 to disguise and forgery skill checks");
  feato(FEAT_DEFT_HANDS, "deft hands", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+3 to sleight of hand checks",
        "+3 to sleight of hand skill checks");
  feato(FEAT_DILIGENT, "diligent", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 bonus to appraise and use magical device skill checks",
        "+2 bonus to appraise and use magical device skill checks");
  feato(FEAT_ENDURANCE, "endurance", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "larger movement gain, and recovery",
        "One with endurance will gain extra movement points per level and in addition "
        "recover movement points quicker.");

  /* extra cleric feat */
  feato(FEAT_EXTRA_TURNING, "extra turning", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "2 extra turn attempts per day",
        "2 extra turn attempts per day");
  feat_prereq_feat(FEAT_EXTRA_TURNING, FEAT_TURN_UNDEAD, 1);

  feato(FEAT_GREAT_FORTITUDE, "great fortitude", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to all fortitude saving throw checks",
        "+2 to all fortitude saving throw checks");

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_INVESTIGATOR, "investigator", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to lore and perception checks",
        "+2 to lore and perception checks");
  feato(FEAT_LUCK_OF_HEROES, "luck of heroes", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+1 to dodge ac and +1 to willpower, fortitude and reflex saves",
        "+1 to dodge ac and +1 to willpower, fortitude and reflex saves");
  feato(FEAT_IRON_WILL, "iron will", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to all willpower saving throw checks",
        "+2 to all willpower saving throw checks");
  feato(FEAT_LIGHTNING_REFLEXES, "lightning reflexes", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to all reflex saving throw checks",
        "+2 to all reflex saving throw checks");
  feato(FEAT_MAGICAL_APTITUDE, "magical aptitude", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to spellcraft and use magical device skill checks",
        "+2 to spellcraft and use magical device skill checks");
  feato(FEAT_NEGOTIATOR, "negotiator", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to diplomacy and sense motive skills",
        "+2 to diplomacy and sense motive skills");
  feato(FEAT_NIMBLE_FINGERS, "nimble fingers", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to disable device skill checks",
        "+2 to disable device skill checks");
  feato(FEAT_PERSUASIVE, "persuasive", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to bluff and intimidate skill checks",
        "+2 to bluff and intimidate skill checks");
  feato(FEAT_SELF_SUFFICIENT, "self sufficient", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to heal and survival skill checks",
        "+2 to heal and survival skill checks");
  feato(FEAT_STEALTHY, "stealthy", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+2 to hide and move silently skill checks",
        "+2 to hide and move silently skill checks");
  feato(FEAT_TOUGHNESS, "toughness", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "+1 hp per level, +(level) hp upon taking",
        "+1 hp per level, +(level) hp upon taking");
  /* rangers get this for free */
  feato(FEAT_TRACK, "track", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
        "use survival skill to track others",
        "use survival skill to track others");
  feat_prereq_ability(FEAT_TRACK, ABILITY_SURVIVAL, 19);

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* Epic */
  feato(FEAT_FAST_HEALING, "fast healing", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "accelerated regeneration of hps",
        "Heals extra 3 hp per 5 seconds.  This feat stacks with itself and other "
        "regeneration abilities, spells and racial innates.");
  feat_prereq_attribute(FEAT_FAST_HEALING, AB_CON, 21);
  feato(FEAT_EPIC_SKILL_FOCUS, "epic skill focus", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "+6 in chosen skill",
        "Taking epic skill focus will give you +6 in chosen skill, this feat can be taken "
        "multiple times, but only once per skill chosen.");
  feato(FEAT_EPIC_TOUGHNESS, "epic toughness", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "gain 30 hps",
        "Gain 30 more maximum hit points");
  feato(FEAT_ARMOR_SKIN, "armor skin", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "Increases natural armor by 1",
        "Increases natural armor by 1");
  /* monk */ feato(FEAT_IMPROVED_SPELL_RESISTANCE, "improved spell resistance", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
                   "+2 to spell resistance",
                   "+2 to spell resistance");
  feat_prereq_feat(FEAT_IMPROVED_SPELL_RESISTANCE, FEAT_DIAMOND_SOUL, 1);
  feato(FEAT_GREAT_CONSTITUTION, "great constitution", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "Increases Constitution by 1",
        "Increases Constitution by 1");
  feato(FEAT_GREAT_DEXTERITY, "great dexterity", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "Increases Dexterity by 1",
        "Increases Dexterity by 1");
  feato(FEAT_GREAT_INTELLIGENCE, "great intelligence", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "Increases Intelligence by 1",
        "Increases Intelligence by 1");
  feato(FEAT_GREAT_STRENGTH, "great strength", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "Increases Strength by 1",
        "Increases Strength by 1");
  feato(FEAT_GREAT_WISDOM, "great wisdom", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "Increases Wisdom by 1",
        "Increases Wisdom by 1");
  feato(FEAT_GREAT_CHARISMA, "great charisma", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "Increases Charisma by 1",
        "Increases Charisma by 1");

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* MetaMagic Feats */
  feato(FEAT_MAXIMIZE_SPELL, "maximize spell", TRUE, TRUE, FALSE, FEAT_TYPE_METAMAGIC,
        "all spells cast while maximize enabled do maximum effect",
        "A spell prepared as 'maximized' will take a slot 3 circles higher and when "
        "cast it will do maximum possible effect/damage values.");
  feato(FEAT_QUICKEN_SPELL, "quicken spell", TRUE, TRUE, FALSE, FEAT_TYPE_METAMAGIC,
        "cast a spell with casting time instantly",
        "A spell prepared as 'quickened' will take a slot 4 circles higher and when "
        "cast it will remove all casting time for that given spell.");

  /* Spellcasting feats */

  /* divine spell access feats  */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_CLERIC_1ST_CIRCLE, "1st circle cleric spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 1st circle cleric spells",
        "You now have access to 1st circle cleric spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_CLERIC_2ND_CIRCLE, "2nd circle cleric spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 2nd circle cleric spells",
        "You now have access to 2nd circle cleric spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_CLERIC_3RD_CIRCLE, "3rd circle cleric spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 3rd circle cleric spells",
        "You now have access to 3rd circle cleric spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_CLERIC_4TH_CIRCLE, "4th circle cleric spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 4th circle cleric spells",
        "You now have access to 4th circle cleric spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_CLERIC_5TH_CIRCLE, "5th circle cleric spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 5th circle cleric spells",
        "You now have access to 5th circle cleric spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_CLERIC_6TH_CIRCLE, "6th circle cleric spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 6th circle cleric spells",
        "You now have access to 6th circle cleric spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_CLERIC_7TH_CIRCLE, "7th circle cleric spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 7th circle cleric spells",
        "You now have access to 7th circle cleric spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_CLERIC_8TH_CIRCLE, "8th circle cleric spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 8th circle cleric spells",
        "You now have access to 8th circle cleric spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_CLERIC_9TH_CIRCLE, "9th circle cleric spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 9th circle cleric spells",
        "You now have access to 9th circle cleric spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_CLERIC_EPIC_SPELL, "epic cleric spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic cleric spells",
        "You now have access to epic cleric spells.  The spells you gain access "
        "are determined by feat selection.  Epic spells are only usable once per "
        "game-day.");
  /* cleric SLOT feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_CLERIC_1ST_CIRCLE_SLOT, "1st circle cleric slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "cleric 1st circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CLERIC_2ND_CIRCLE_SLOT, "1st circle cleric slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "cleric 2nd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CLERIC_3RD_CIRCLE_SLOT, "1st circle cleric slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "cleric 3rd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CLERIC_4TH_CIRCLE_SLOT, "1st circle cleric slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "cleric 4th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CLERIC_5TH_CIRCLE_SLOT, "1st circle cleric slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "cleric 5th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CLERIC_6TH_CIRCLE_SLOT, "1st circle cleric slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "cleric 6th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CLERIC_7TH_CIRCLE_SLOT, "1st circle cleric slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "cleric 7th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CLERIC_8TH_CIRCLE_SLOT, "1st circle cleric slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "cleric 8th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CLERIC_9TH_CIRCLE_SLOT, "1st circle cleric slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "cleric 9th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CLERIC_EPIC_SPELL_SLOT, "1st circle cleric slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "cleric epic circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");

  /* druid spell access feats  */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_DRUID_1ST_CIRCLE, "1st circle druid spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 1st circle druid spells",
        "You now have access to 1st circle druid spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_DRUID_2ND_CIRCLE, "2nd circle druid spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 2nd circle druid spells",
        "You now have access to 2nd circle druid spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_DRUID_3RD_CIRCLE, "3rd circle druid spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 3rd circle druid spells",
        "You now have access to 3rd circle druid spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_DRUID_4TH_CIRCLE, "4th circle druid spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 4th circle druid spells",
        "You now have access to 4th circle druid spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_DRUID_5TH_CIRCLE, "5th circle druid spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 5th circle druid spells",
        "You now have access to 5th circle druid spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_DRUID_6TH_CIRCLE, "6th circle druid spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 6th circle druid spells",
        "You now have access to 6th circle druid spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_DRUID_7TH_CIRCLE, "7th circle druid spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 7th circle druid spells",
        "You now have access to 7th circle druid spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_DRUID_8TH_CIRCLE, "8th circle druid spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 8th circle druid spells",
        "You now have access to 8th circle druid spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_DRUID_9TH_CIRCLE, "9th circle druid spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 9th circle druid spells",
        "You now have access to 9th circle druid spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_DRUID_EPIC_SPELL, "epic druid spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic druid spells",
        "You now have access to epic druid spells.  The spells you gain access "
        "are determined by feat selection.  Epic spells are only usable once per "
        "game-day.");
  /* druid SLOT feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_DRUID_1ST_CIRCLE_SLOT, "1st circle druid slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "druid 1st circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_DRUID_2ND_CIRCLE_SLOT, "1st circle druid slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "druid 2nd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_DRUID_3RD_CIRCLE_SLOT, "1st circle druid slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "druid 3rd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_DRUID_4TH_CIRCLE_SLOT, "1st circle druid slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "druid 4th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_DRUID_5TH_CIRCLE_SLOT, "1st circle druid slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "druid 5th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_DRUID_6TH_CIRCLE_SLOT, "1st circle druid slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "druid 6th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_DRUID_7TH_CIRCLE_SLOT, "1st circle druid slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "druid 7th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_DRUID_8TH_CIRCLE_SLOT, "1st circle druid slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "druid 8th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_DRUID_9TH_CIRCLE_SLOT, "1st circle druid slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "druid 9th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_DRUID_EPIC_SPELL_SLOT, "1st circle druid slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "druid epic circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");

  /* paladin spell access feats  */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_PALADIN_1ST_CIRCLE, "1st circle paladin spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 1st circle paladin spells",
        "You now have access to 1st circle paladin spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_PALADIN_2ND_CIRCLE, "2nd circle paladin spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 2nd circle paladin spells",
        "You now have access to 2nd circle paladin spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_PALADIN_3RD_CIRCLE, "3rd circle paladin spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 3rd circle paladin spells",
        "You now have access to 3rd circle paladin spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_PALADIN_4TH_CIRCLE, "4th circle paladin spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 4th circle paladin spells",
        "You now have access to 4th circle paladin spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  /* paladin SLOT feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_PALADIN_1ST_CIRCLE_SLOT, "1st circle paladin slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "paladin 1st circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_PALADIN_2ND_CIRCLE_SLOT, "1st circle paladin slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "paladin 2nd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_PALADIN_3RD_CIRCLE_SLOT, "1st circle paladin slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "paladin 3rd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_PALADIN_4TH_CIRCLE_SLOT, "1st circle paladin slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "paladin 4th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");

  /* ranger spell access feats  */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_RANGER_1ST_CIRCLE, "1st circle ranger spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 1st circle ranger spells",
        "You now have access to 1st circle ranger spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_RANGER_2ND_CIRCLE, "2nd circle ranger spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 2nd circle ranger spells",
        "You now have access to 2nd circle ranger spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_RANGER_3RD_CIRCLE, "3rd circle ranger spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 3rd circle ranger spells",
        "You now have access to 3rd circle ranger spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_RANGER_4TH_CIRCLE, "4th circle ranger spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 4th circle ranger spells",
        "You now have access to 4th circle ranger spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  /* ranger SLOT feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_RANGER_1ST_CIRCLE_SLOT, "1st circle ranger slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "ranger 1st circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_RANGER_2ND_CIRCLE_SLOT, "1st circle ranger slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "ranger 2nd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_RANGER_3RD_CIRCLE_SLOT, "1st circle ranger slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "ranger 3rd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_RANGER_4TH_CIRCLE_SLOT, "1st circle ranger slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "ranger 4th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");

  /* wizard spell access feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_WIZARD_1ST_CIRCLE, "1st circle wizard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 1st circle wizard spells",
        "You now have access to 1st circle wizard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_WIZARD_2ND_CIRCLE, "2nd circle wizard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 2nd circle wizard spells",
        "You now have access to 2nd circle wizard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_WIZARD_3RD_CIRCLE, "3rd circle wizard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 3rd circle wizard spells",
        "You now have access to 3rd circle wizard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_WIZARD_4TH_CIRCLE, "4th circle wizard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 4th circle wizard spells",
        "You now have access to 4th circle wizard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_WIZARD_5TH_CIRCLE, "5th circle wizard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 5th circle wizard spells",
        "You now have access to 5th circle wizard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_WIZARD_6TH_CIRCLE, "6th circle wizard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 6th circle wizard spells",
        "You now have access to 6th circle wizard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_WIZARD_7TH_CIRCLE, "7th circle wizard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 7th circle wizard spells",
        "You now have access to 7th circle wizard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_WIZARD_8TH_CIRCLE, "8th circle wizard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 8th circle wizard spells",
        "You now have access to 8th circle wizard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_WIZARD_9TH_CIRCLE, "9th circle wizard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 9th circle wizard spells",
        "You now have access to 9th circle wizard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_WIZARD_EPIC_SPELL, "epic wizard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic wizard spells",
        "You now have access to epic wizard spells.  The spells you gain access "
        "are determined by feat selection.  Epic spells are only usable once per "
        "game-day.");
  /* wizard SLOT feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_WIZARD_1ST_CIRCLE_SLOT, "1st circle wizard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "wizard 1st circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_WIZARD_2ND_CIRCLE_SLOT, "1st circle wizard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "wizard 2nd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_WIZARD_3RD_CIRCLE_SLOT, "1st circle wizard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "wizard 3rd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_WIZARD_4TH_CIRCLE_SLOT, "1st circle wizard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "wizard 4th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_WIZARD_5TH_CIRCLE_SLOT, "1st circle wizard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "wizard 5th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_WIZARD_6TH_CIRCLE_SLOT, "1st circle wizard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "wizard 6th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_WIZARD_7TH_CIRCLE_SLOT, "1st circle wizard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "wizard 7th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_WIZARD_8TH_CIRCLE_SLOT, "1st circle wizard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "wizard 8th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_WIZARD_9TH_CIRCLE_SLOT, "1st circle wizard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "wizard 9th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_WIZARD_EPIC_SPELL_SLOT, "1st circle wizard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "wizard epic circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");

  /* sorcerer spell access feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_SORCERER_1ST_CIRCLE, "1st circle sorcerer spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 1st circle sorcerer spells",
        "You now have access to 1st circle sorcerer spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_SORCERER_2ND_CIRCLE, "2nd circle sorcerer spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 2nd circle sorcerer spells",
        "You now have access to 2nd circle sorcerer spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_SORCERER_3RD_CIRCLE, "3rd circle sorcerer spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 3rd circle sorcerer spells",
        "You now have access to 3rd circle sorcerer spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_SORCERER_4TH_CIRCLE, "4th circle sorcerer spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 4th circle sorcerer spells",
        "You now have access to 4th circle sorcerer spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_SORCERER_5TH_CIRCLE, "5th circle sorcerer spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 5th circle sorcerer spells",
        "You now have access to 5th circle sorcerer spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_SORCERER_6TH_CIRCLE, "6th circle sorcerer spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 6th circle sorcerer spells",
        "You now have access to 6th circle sorcerer spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_SORCERER_7TH_CIRCLE, "7th circle sorcerer spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 7th circle sorcerer spells",
        "You now have access to 7th circle sorcerer spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_SORCERER_8TH_CIRCLE, "8th circle sorcerer spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 8th circle sorcerer spells",
        "You now have access to 8th circle sorcerer spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_SORCERER_9TH_CIRCLE, "9th circle sorcerer spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 9th circle sorcerer spells",
        "You now have access to 9th circle sorcerer spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_SORCERER_EPIC_SPELL, "epic sorcerer spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic sorcerer spells",
        "You now have access to epic sorcerer spells.  The spells you gain access "
        "are determined by feat selection.  Epic spells are only usable once per "
        "game-day.");
  /* sorcerer SLOT feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_SORCERER_1ST_CIRCLE_SLOT, "1st circle sorcerer slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "sorcerer 1st circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_SORCERER_2ND_CIRCLE_SLOT, "1st circle sorcerer slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "sorcerer 2nd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_SORCERER_3RD_CIRCLE_SLOT, "1st circle sorcerer slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "sorcerer 3rd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_SORCERER_4TH_CIRCLE_SLOT, "1st circle sorcerer slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "sorcerer 4th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_SORCERER_5TH_CIRCLE_SLOT, "1st circle sorcerer slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "sorcerer 5th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_SORCERER_6TH_CIRCLE_SLOT, "1st circle sorcerer slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "sorcerer 6th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_SORCERER_7TH_CIRCLE_SLOT, "1st circle sorcerer slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "sorcerer 7th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_SORCERER_8TH_CIRCLE_SLOT, "1st circle sorcerer slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "sorcerer 8th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_SORCERER_9TH_CIRCLE_SLOT, "1st circle sorcerer slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "sorcerer 9th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_SORCERER_EPIC_SPELL_SLOT, "1st circle sorcerer slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "sorcerer epic circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");

  /* bard spell access feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_BARD_1ST_CIRCLE, "1st circle bard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 1st circle bard spells",
        "You now have access to 1st circle bard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_BARD_2ND_CIRCLE, "2nd circle bard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 2nd circle bard spells",
        "You now have access to 2nd circle bard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_BARD_3RD_CIRCLE, "3rd circle bard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 3rd circle bard spells",
        "You now have access to 3rd circle bard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_BARD_4TH_CIRCLE, "4th circle bard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 4th circle bard spells",
        "You now have access to 4th circle bard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_BARD_5TH_CIRCLE, "5th circle bard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 5th circle bard spells",
        "You now have access to 5th circle bard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_BARD_6TH_CIRCLE, "6th circle bard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 6th circle bard spells",
        "You now have access to 6th circle bard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_BARD_EPIC_SPELL, "epic bard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic bard spells",
        "You now have access to epic bard spells.  The spells you gain access "
        "are determined by feat selection.  Epic spells are only usable once per "
        "game-day.");
  /* bard SLOT feats */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_BARD_1ST_CIRCLE_SLOT, "1st circle bard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "bard 1st circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_BARD_2ND_CIRCLE_SLOT, "1st circle bard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "bard 2nd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_BARD_3RD_CIRCLE_SLOT, "1st circle bard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "bard 3rd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_BARD_4TH_CIRCLE_SLOT, "1st circle bard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "bard 4th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_BARD_5TH_CIRCLE_SLOT, "1st circle bard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "bard 5th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_BARD_6TH_CIRCLE_SLOT, "1st circle bard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "bard 6th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_BARD_EPIC_SPELL_SLOT, "1st circle bard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "bard epic circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");

  feato(FEAT_PSIONICIST_1ST_CIRCLE, "1st circle psionicist powers", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 1st circle psionicist powers",
        "You now have access to 1st circle psionicist powers.");
  feato(FEAT_PSIONICIST_2ND_CIRCLE, "2nd circle psionicist powers", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 2nd circle psionicist powers",
        "You now have access to 2nd circle psionicist powers.");
  feato(FEAT_PSIONICIST_3RD_CIRCLE, "3rd circle psionicist powers", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 3rd circle psionicist powers",
        "You now have access to 3rd circle psionicist powers.");
  feato(FEAT_PSIONICIST_4TH_CIRCLE, "4th circle psionicist powers", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 4th circle psionicist powers",
        "You now have access to 4th circle psionicist powers.");
  feato(FEAT_PSIONICIST_5TH_CIRCLE, "5th circle psionicist powers", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 5th circle psionicist powers",
        "You now have access to 5th circle psionicist powers.");
  feato(FEAT_PSIONICIST_6TH_CIRCLE, "6th circle psionicist powers", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 6th circle psionicist powers",
        "You now have access to 6th circle psionicist powers.");
  feato(FEAT_PSIONICIST_7TH_CIRCLE, "7th circle psionicist powers", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 7th circle psionicist powers",
        "You now have access to 7th circle psionicist powers.");
  feato(FEAT_PSIONICIST_8TH_CIRCLE, "8th circle psionicist powers", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 8th circle psionicist powers",
        "You now have access to 8th circle psionicist powers.");
  feato(FEAT_PSIONICIST_9TH_CIRCLE, "9th circle psionicist powers", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 9th circle psionicist powers",
        "You now have access to 9th circle psionicist powers.");

  feato(FEAT_BLACKGUARD_1ST_CIRCLE, "1st circle blackguard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 1st circle blackguard spells",
        "You now have access to 1st circle blackguard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_BLACKGUARD_2ND_CIRCLE, "2nd circle blackguard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 2nd circle blackguard spells",
        "You now have access to 2nd circle blackguard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_BLACKGUARD_3RD_CIRCLE, "3rd circle blackguard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 3rd circle blackguard spells",
        "You now have access to 3rd circle blackguard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_BLACKGUARD_4TH_CIRCLE, "4th circle blackguard spells", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to 4th circle blackguard spells",
        "You now have access to 4th circle blackguard spells.  The spells you gain access "
        "are determined by class.  Some classes gain access to all the spells "
        "instantly upon ataining this feat, others have to select the spells via "
        "the 'study' command, and others have to acquire the spells in their 'spellbook.'");
  feato(FEAT_BLACKGUARD_1ST_CIRCLE_SLOT, "1st circle blackguard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "blackguard 1st circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_BLACKGUARD_2ND_CIRCLE_SLOT, "1st circle blackguard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "blackguard 2nd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_BLACKGUARD_3RD_CIRCLE_SLOT, "1st circle blackguard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "blackguard 3rd circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_BLACKGUARD_4TH_CIRCLE_SLOT, "1st circle blackguard slot", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "blackguard 4th circle slot",
        "This gives you the ability to cast another spell of this slot for the respective "
        "class.  There may be other requirements for casting particular spells from this "
        "slot, some classes need the spell to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");

  feato(FEAT_SORCERER_BLOODLINE_DRACONIC, "draconic bloodline", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "1 level as sorcerer & select the draconic bloodline",
        "The draconic bloodline allows the sorcerer to take upon them traits "
        "of a dragon type of their choice.  This offers a claw attack starting "
        "at level 1, energy resistance and natural armor at level 3, a breath "
        "weapon at level 9, retractable wings at level 15, and blindsense plus "
        "immunity to paralysis, sleep and your draconic heritage energy type at level "
        "20.  The draconic bloodline also offers bonus spells, bonus feats and "
        "more.  See HELP DRACONIC-BLOODLINE for more information.");
  feato(FEAT_DRACONIC_HERITAGE_BREATHWEAPON, "draconic breath weapon", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "draconic bloodline and sorcerer level 9+",
        "Allows use of the dracbreath command which will allow the sorcerer to "
        "perform a draconic breath weapon attack, doing 1d6 damage for each "
        "level in the sorcerer class.  The damage type is determined by the "
        "dragon type chosen by the sorcerer when choosing their bloodline.");
  feato(FEAT_DRACONIC_HERITAGE_CLAWS, "draconic claws", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "draconic bloodline",
        "Allows use of the dracclaws command which will perform two claw attacks "
        "at 1d4 + strength bonus damage each.  At level 7, these claw attacks are "
        "considered magical when attempting to bypass certain damage reduction types. "
        "At level 9 the damage increases to 1d6, and at level 11, they deal an extra "
        "1d6 damage of the element type associated with your draconic heritage bloodline. "
        "These claw attacks can be used a # of times per day equal to 3 + your "
        "charisma modifier");
  feato(FEAT_DRACONIC_HERITAGE_DRAGON_RESISTANCES, "draconic resistances", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "draconic bloodline, sorcerer level 3",
        "At level 3 this offers energy resistance 5 for the element type associated "
        "with the draconic heritage subtype and +1 natural ac bonus.  At level 9 "
        "the energy resistance amount increases to 10 and natural armor +2.  At "
        "level 15 the natural armor bonus increases to +4.");
  feato(FEAT_DRACONIC_BLOODLINE_ARCANA, "draconic arcana", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "draconic bloodline",
        "When casting a spell that uses the same damage subtype as your draconic heritage, "
        "damage will be increased by +1 per damage die.");
  feato(FEAT_DRACONIC_HERITAGE_WINGS, "draconic wings", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "draconic bloodline, sorcerer level 15",
        "Allows the sorcerer to extend and retract dragon-like wings using the "
        "dracwings command.  Wings provide a fly effect with a speed of 60.");
  feato(FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS, "power of wyrms", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "draconic bloodline, sorcerer level 20",
        "Provides immunity to sleep, paralysis, and elemental damage that matches "
        "your draconic heritage.  Also provides blindsense with a range of 60 feet.");

  feato(FEAT_SORCERER_BLOODLINE_ARCANE, "arcane bloodline", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "level one sorcerer",
        "The arcane bloodline is developed through a long heritage of masterful "
        "arcane spellcasters whose gift has passed on to manifest naturally within "
        "the sorcerer.  The arcane bloodline offers additional spell knowledge, "
        "increased metamagic prowess, enhanced ability in a school of choice and an innate "
        "ability to use consumable magic such as wands and staves.");
  feato(FEAT_ARCANE_BLOODLINE_ARCANA, "arcane arcana", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "arcane bloodline selected",
        "The arcane bloodline arcana benefit enhances any spell being cast using "
        "any metamagic feat to be performed with an additional +1 to the spell "
        "dc to resist or reduce the spell effect.");
  feato(FEAT_METAMAGIC_ADEPT, "metamagic adept", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "arcane bloodline, sorcerer level 3",
        "Allows the sorcerer to negate extra casting times when casting a spell "
        "in combination with one or more metamagic effects, such as maximize spell. "
        "This ability is activated by prepending the spell name with 'metamagicadept'. "
        "\r\nExample: cast maximize metamagicadept 'fireball'");
  feato(FEAT_NEW_ARCANA, "new arcana", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "arcane bloodline, sorcerer level 9",
        "This feat allows the sorcerer to receive a bonus spell slot starting at "
        "spell circles 1-3, then 1-6 at feat rank 2 and finally 1-9 at feat rank "
        "3.");
  feato(FEAT_SCHOOL_POWER, "school power", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "arcane bloodline, sorcerer level 15",
        "This feat allows the sorcerer to increase the DCs by +2 for all spells cast "
        "from their associated arcane bloodline school.  This stacks with bonuses from "
        "the spell focus feat.");
  feato(FEAT_ARCANE_APOTHEOSIS, "arcane apotheosis", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "arcane bloodline, sorcerer level 20",
        "This feat automatically negates any extra casting time for spells that "
        "are cast using metamagic effects such as maximize spell.  In addition, the "
        "sorcerer can expend spell slots instead of item charges, when using items that "
        "expend charges, such as wands and staves.  For every three levels of spell "
        "slots used, one less charge will be expended.  See the apotheosis command and HELP "
        "APOTHEOSIS for more information.");

  // Sorcerer Fey Bloodline
  feato(FEAT_SORCERER_BLOODLINE_FEY, "fey bloodline", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "level one sorcerer",
        "The capricious nature of the fey runs in your family due to some intermingling of "
        "fey blood or magic. You are more emotional than most, prone to bouts of joy and rage. "
        "The fey bloodline offers a great knowledge of nature as well as enhanced abilities in "
        "traversing or dealing with nature.  It also offers abilities using fey magic.");
  feato(FEAT_FEY_BLOODLINE_ARCANA, "fey arcana", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "fey bloodline selected",
        "Whenever you cast a spell of the enchantment school, the dc to resist increases by 2.");
  feato(FEAT_LAUGHING_TOUCH, "laughing touch", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "fey bloodline",
        "Cause a target to burst out laughing for one round, during which they can only take "
        "a move action.  Useable as a swift action (3 + charisma modifier) times per day. "
        "Uses the 'fey' command.");
  feato(FEAT_WOODLAND_STRIDE, "wilderness stride", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "fey bloodline or druid level 2",
        "Reduced movement penalty when moving through wilderness areas.");
  feato(FEAT_FLEETING_GLANCE, "fleeting glance", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "fey bloodline, sorcerer level 9",
        "Can cast greater invisibility 3 times per day. Uses the 'fey' command.");
  feato(FEAT_FEY_MAGIC, "fey magic", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "fey bloodline, sorcerer level 15",
        "Whenever trying to overcome spell resistance, you will roll twice and take the better roll. "
        "Also refers the 'feymagic' command, which is used to perform various fey bloodling abilities. "
        "Type 'feymagic' by itself for a list of options.");
  feato(FEAT_SOUL_OF_THE_FEY, "soul of the fey", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "fey bloodline, sorcerer level 20",
        "Gain immunity to poison and +3 damage reduction. Creatures of the animal type will not "
        "aggro you. Can cast 'shadow walk' once per day using the 'fey' command.");

  // Sorcerer Undead Bloodline
  feato(FEAT_SORCERER_BLOODLINE_UNDEAD, "undead bloodline", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "level one sorcerer",
        "The taint of the grave runs through your family. Perhaps one of your ancestors became a "
        "powerful lich or vampire, or maybe you were born dead before suddenly returning to life. "
        "Either way, the forces of death move through you and touch your every action. The undead "
        "sorcerer bloodline allows you to cause fear in opponents, gain resistance to cold and "
        "innate damage reduction, cause the dead to reach forth from the earth to ravage your foes, "
        "become incorporeal and eventually become one of the undead yourself.");
  feato(FEAT_UNDEAD_BLOODLINE_ARCANA, "undead arcana", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "undead bloodline selected",
        "Undead can now be succeptible to your mind-affecting spells.");
  feato(FEAT_GRAVE_TOUCH, "grave touch", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "undead bloodline selected",
        "Use a move action to touch another being causing them to be shaken for a number "
        "of rounds equal to your sorcerer level divided by two.  Useable a number of times "
        "per day equal to your charisma modifier plus three.");
  feato(FEAT_DEATHS_GIFT, "death's gift", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "undead bloodline selected, level 3 sorcerer",
        "At sorcerer level 3 you gain 20 cold resist, 30 nonlethal resist and DR 1/- against all damage. At sorcerer "
        "level 9, this increases to 40 cold resist, 50 nonlethal resist and DR 2/- against all damage.");
  feato(FEAT_GRASP_OF_THE_DEAD, "grasp of the dead", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "undead bloodline selected, sorcerer level 9",
        "As a standard action, you can cause a swarm of skeletal arms to burst from the ground to rip and tear "
        "at your foes. This affects all enemies in the room, and does 1d6 slashing damage "
        "per sorcerer level. Victims can make a reflex save for half damage.  Those who fail "
        "their save are also unable to move for 1 round. The dc for this ability is 10 + 1/2 "
        "sorcerer level + your charisma modifier. You must be on a solid surface for this to work. "
        "At sorcerer level 9 this is useable once per day.  At sorcerer level 17, it can be "
        "used twice per day, and at sorcerer level 20, it can be used three times per day.");
  feato(FEAT_INCORPOREAL_FORM, "incorporeal form (undead bloodline)", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "undead bloodline selected, sorcerer level 15",
        "You become incorporeal for 3 rounds, during which you take 1/2 damage from all sources except spells, unless "
        "the weapon is a ghost touch weapon or your opponent is incorporeal as well.  Likewise, your "
        "non-spell attacks only deal 1/2 damage to corporeal opponents.  This ability can be used "
        "a number of times per day equal to your sorcerer level / 3. Using a ghost touch weapon will bypass "
        "this damage reduction for melee attacks.");
  feato(FEAT_ONE_OF_US, "one of us", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "undead bloodline selected, sorcerer level 20",
        "At sorcerer level 20, your physical form begins to rot, and undead see you as "
        "one of them. You gain immunity to cold damage, nonlethal damage, and DR 5/- from all other damage. "
        "You also gain immunity to paralysis and sleep affects. Undead NPCs will not aggro "
        "you. You receive a +4 morale bonus on saving throws made against spells and spell-like "
        "abilities cast by undead.");

  feato(FEAT_THEURGE_SPELLCASTING, "theurge spellcasting", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "mystic theurge",
        "This feat allows the mystic theurge to progress in both arcane and divine casting "
        "classes.  Each time its taken, it adds one level to two spellcasting classes the "
        "theurge previously had access to, but only for purposes of daily spells and spell "
        "circles available.");

  feato(FEAT_SPELL_CRITICAL, "spell critical", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Can cast a spell immediately after scoring a critical hit.",
        "Upon landing a successful critical hit with a weapon or unarmed attack, the "
        "eldritch knight can then cast a spell immediately, without waiting for the "
        "spell casting countdown to occur.");

  feato(FEAT_DIVERSE_TRAINING, "diverse training", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Adds eldritch knight levels to warrior levels when determining feat eligibility.",
        "Adds eldritch knight levels to warrior levels when determining feat eligibility.");

  // spellswords
  feato(FEAT_IGNORE_SPELL_FAILURE, "ignore spell failure", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Grants 10 percent lower arcane spell failure in armor, plus 5 percent per rank above one. ",
        "Grants 10 percent lower arcane spell failure in armor, plus 5 percent per rank above one.");
  feato(FEAT_CHANNEL_SPELL, "channel spell", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "You can channel a spell into your weapon.",
        "You can channel a spell into your weapon. You can only channel a harmful spell of a spell level "
        "the same level or lower as the number of ranks in this feat. Once imbued, the weapon has a 5% chance "
        "per hit to cast that spell when it hits, up to 5 times until it is expended. This ability "
        "can be used 3 times per day at level 4, 4 times at level 6 and 5 times at spellsword level 8. "
        "This feat uses the channelspell command. This effect does not save over play sessions, reboots, or copyovers.");
  feato(FEAT_MULTIPLE_CHANNEL_SPELL, "multiple channel spell", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "You can channel two spells into your weapon.",
        "As per channel spell, except allows you to channel two spells into a single weapon.");

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_SPELL_PENETRATION, "spell penetration", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "+2 bonus on caster level checks to defeat spell resistance",
        "+2 bonus on caster level checks to defeat spell resistance");
  feato(FEAT_GREATER_SPELL_PENETRATION, "greater spell penetration", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "+2 to caster level checks to defeat spell resistance",
        "+2 to caster level checks to defeat spell resistance");
  feat_prereq_feat(FEAT_GREATER_SPELL_PENETRATION, FEAT_SPELL_PENETRATION, 1);
  feato(FEAT_EPIC_SPELL_PENETRATION, "epic spell penetration", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "+2 to caster level checks to defeat spell resistance",
        "+2 to caster level checks to defeat spell resistance");
  feat_prereq_feat(FEAT_EPIC_SPELL_PENETRATION, FEAT_GREATER_SPELL_PENETRATION, 1);

  feato(FEAT_ARMORED_SPELLCASTING, "armored spellcasting", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "reduce penalty for casting arcane spells while armored",
        "reduces the arcane armor weight penalty by 5");

  feato(FEAT_FASTER_MEMORIZATION, "faster memorization", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "decreases spell memorization time",
        "Chance to decrease total memorization time and each memorization pulse to "
        "further reduce memorization time.");

  feato(FEAT_SPELL_FOCUS, "spell focus", TRUE, TRUE, TRUE, FEAT_TYPE_SPELLCASTING,
        "wizard only, +1 to all spell dcs for all spells in school/domain",
        "+1 to all spell dcs for all spells in school/domain.  Transmutation improves polymorph stats.  Conjuration increases summon creature spell stats. Necromancy increases undead follower stats.");
  feato(FEAT_GREATER_SPELL_FOCUS, "greater spell focus", TRUE, TRUE, TRUE, FEAT_TYPE_SPELLCASTING,
        "wizard only, +2 to all spell dcs for all spells in school/domain",
        "+2 to all spell dcs for all spells in school/domain. Transmutation improves polymorph stats.  Conjuration increases summon creature spell stats. Necromancy increases undead follower stats.");
  feat_prereq_feat(FEAT_GREATER_SPELL_FOCUS, FEAT_SPELL_FOCUS, 1);
  feato(FEAT_EPIC_SPELL_FOCUS, "epic spell focus", TRUE, TRUE, TRUE, FEAT_TYPE_SPELLCASTING,
        "wizard only, +3 to all spell dcs for all spells in school/domain",
        "+3 to all spell dcs for all spells in school/domain. Transmutation improves polymorph stats.  Conjuration increases summon creature spell stats. Necromancy increases undead follower stats.");
  feat_prereq_feat(FEAT_EPIC_SPELL_FOCUS, FEAT_GREATER_SPELL_FOCUS, 1);

  feato(FEAT_IMPROVED_FAMILIAR, "improved familiar", TRUE, TRUE, TRUE, FEAT_TYPE_SPELLCASTING,
        "your familiar gets more powerful",
        "Each rank in this feat will give your familiar: 1 AC, 10 Hit-points, +1 to "
        "strength, dexterity and constitution.");

  feato(FEAT_QUICK_CHANT, "quick chant", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "you can cast spells faster",
        "You can cast spells about 50 percent faster than normal with this feat.");

  feato(FEAT_AUGMENT_SUMMONING, "augment summoning", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "enhance summoned creatures",
        "Gives all creatures you have from summoning spells +4 to strength and "
        "constitution.  Note: this will not augment your familiar, called companions, "
        "or charmed/dominated victims.  Note: requires spell-focus in conjuration.");

  feato(FEAT_ENHANCED_SPELL_DAMAGE, "enhanced spell damage", TRUE, TRUE, TRUE, FEAT_TYPE_SPELLCASTING,
        "+1 spell damage per die rolled",
        "You gain +1 spell damage per die rolled, example:  if you are level 10 and "
        "normally create a 10d6 damage fireball, with this feat your fireball would "
        "do 10d6+10. Maximum of 3 ranks, rank 1-any spellcaster level, rank 2, spellcaster level 5+, rank 3, spellcaster level 10+");

  feato(FEAT_COMBAT_CASTING, "combat casting", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "+4 to spell concentration checks made in combat or when grappled ",
        "+4 to spell concentration checks made in combat or when grappled ");

  /* epic type spellcasting feats */
  feato(FEAT_MUMMY_DUST, "mummy dust", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic spell - mummy dust",
        "Once per game day, you can cast a spell that will conjure a powerful Mummy "
        "Lord to assist you in combat.");
  feat_prereq_ability(FEAT_MUMMY_DUST, ABILITY_SPELLCRAFT, 23);
  feato(FEAT_DRAGON_KNIGHT, "dragon knight", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic spell - dragon knight",
        "Once per game day, you can cast a spell that will conjure a small red dragon"
        " to assist you in combat.");
  feat_prereq_ability(FEAT_DRAGON_KNIGHT, ABILITY_SPELLCRAFT, 25);
  feato(FEAT_GREATER_RUIN, "greater ruin", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic spell - greater ruin",
        "Once per game day, you can cast a spell that will cause serious damage to "
        "a selected target.");
  feat_prereq_ability(FEAT_GREATER_RUIN, ABILITY_SPELLCRAFT, 27);
  feato(FEAT_HELLBALL, "hellball", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic spell - greater ruin",
        "Once per game day, you can cast a spell that will cause serious damage to "
        "all the targets in a room.");
  feat_prereq_ability(FEAT_HELLBALL, ABILITY_SPELLCRAFT, 29);
  feato(FEAT_EPIC_MAGE_ARMOR, "epic mage armor", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic spell - epic mage armor",
        "Once per game day, you can cast a spell that will give a 10 AC bonus to "
        "the caster and general damage reduction of 6.");
  feat_prereq_ability(FEAT_EPIC_MAGE_ARMOR, ABILITY_SPELLCRAFT, 31);
  feato(FEAT_EPIC_WARDING, "epic warding", TRUE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING,
        "gain access to epic spell - epic warding",
        "Once per game day, you can cast a spell that will absorb a massive amount "
        "of damage.");
  feat_prereq_ability(FEAT_EPIC_WARDING, ABILITY_SPELLCRAFT, 32);
  /* zusuk marker */

  /* Crafting feats */

  feato(FEAT_DRACONIC_CRAFTING, "draconic crafting", TRUE, FALSE, FALSE, FEAT_TYPE_CRAFT,
        "All magical items created gain higher bonuses w/o increasing level",
        "All magical items created gain higher bonuses w/o increasing level");
  feato(FEAT_DWARVEN_CRAFTING, "dwarven crafting", TRUE, FALSE, FALSE, FEAT_TYPE_CRAFT,
        "All weapons and armor made have higher bonuses",
        "All weapons and armor made have higher bonuses");
  feato(FEAT_ELVEN_CRAFTING, "elven crafting", TRUE, FALSE, FALSE, FEAT_TYPE_CRAFT,
        "All equipment made is 50 percent weight and uses 50 percent materials",
        "All equipment made is 50 percent weight and uses 50 percent materials");
  /* NOT IN GAME */ feato(FEAT_FAST_CRAFTER, "fast crafter", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT,
                          "Reduces crafting time",
                          "Reduces crafting time");

  /* Cleric Domain (ability) Feats */
  feato(FEAT_LIGHTNING_ARC, "lightning arc", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "you can unleash an arc of electricity",
        "As a standard action, you can unleash an arc of electricity. This arc of "
        "electricity deals 1d6+10 points of electric damage + 1 point for "
        "every two cleric levels you possess. You can use this ability a number "
        "of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_ACID_DART, "acid dart", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "you can unleash a dart of acid",
        "As a standard action, you can unleash a dart of acid. This dart of "
        "acid deals 1d6+10 points of acid damage + 1 point for "
        "every two cleric levels you possess. You can use this ability a number "
        "of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_FIRE_BOLT, "fire bolt", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "you can unleash a bolt of fire",
        "As a standard action, you can unleash a bolt of fire. This bolt of "
        "fire deals 1d6+10 points of fire damage + 1 point for "
        "every two cleric levels you possess. You can use this ability a number "
        "of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_ICICLE, "icicle", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "you can unleash an icicle",
        "As a standard action, you can unleash an icicle. This icicle "
        "deals 1d6+10 points of cold damage + 1 point for "
        "every two cleric levels you possess. You can use this ability a number "
        "of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_DOMAIN_ELECTRIC_RESIST, "domain electric resistance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "gain electricity resistance",
        "At 6th cleric levels, you gain resist electricity 10. This resistance increases "
        "to 20 at 12th level and to 50 at 20th level.");
  feato(FEAT_DOMAIN_ACID_RESIST, "domain acid resistance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "gain acid resistance",
        "At 6th cleric levels, you gain resist acid 10. This resistance increases "
        "to 20 at 12th level and to 50 at 20th level.");
  feato(FEAT_DOMAIN_FIRE_RESIST, "domain fire resistance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "gain fire resistance",
        "At 6th cleric levels, you gain resist fire 10. This resistance increases "
        "to 20 at 12th level and to 50 at 20th level.");
  feato(FEAT_DOMAIN_COLD_RESIST, "domain cold resistance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "gain cold resistance",
        "At 6th cleric levels, you gain resist cold 10. This resistance increases "
        "to 20 at 12th level and to 50 at 20th level.");
  feato(FEAT_CURSE_TOUCH, "curse touch", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "you can unleash a curse",
        "As a standard action, you can unleash a curse (like the spell).  You can use "
        "this ability a number of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_CHAOTIC_WEAPON, "chaotic weapon", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "your weapon becomes chaotic",
        "Any weapon you wield behaves as if it is chaotic, and will do additional 2d6 "
        "damage against lawful-raced and lawful-aligned opponents.");
  feato(FEAT_DESTRUCTIVE_SMITE, "destructive smite", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "add 1/2 cleric level to damage",
        "You gain the destructive smite power: the supernatural ability to make a "
        "single melee attack with a morale bonus on damage rolls equal to 1/2 your "
        "cleric level (minimum 1). You must declare the destructive smite before "
        "making the attack. You can use this ability a number of times per day "
        "equal to 3 + your Wisdom modifier.");
  feato(FEAT_DESTRUCTIVE_AURA, "destructive aura", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "add 1/2 cleric level to group's damage",
        "As a standard action, you can unleash a destructive aura which will give "
        "all your group companions cleric-level/2 bonus damage for 1 round.  You can use "
        "this ability a number of times per day equal to your Wisdom modifier.");
  feato(FEAT_EVIL_TOUCH, "evil touch", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "inflict disease with touch attack",
        "You can cause a creature to become sickened as a melee touch attack.  This "
        "ability lasts for a number of rounds equal to 1/2 "
        "your cleric level (minimum 1). You can use this ability a number of times "
        "per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_EVIL_SCYTHE, "evil scythe", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "empower your weapon with unholy damage",
        "At 8th level, you can give a weapon touched the unholy special weapon quality "
        "for a number of rounds equal to 1/2 your cleric level. You can use "
        "this ability once per day at 8th level, and an additional time per "
        "day for every four levels beyond 8th.");
  feato(FEAT_GOOD_TOUCH, "good touch", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "cure disease and poison with touch",
        "You can touch a creature as a standard action, removing one poison affliction "
        "and one disease per usage. You can use this ability "
        "a number of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_GOOD_LANCE, "good lance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "empower your weapon with holy damage",
        "At 8th level, you can give a weapon you touch the holy special weapon quality "
        "for a number of rounds equal to 1/2 your cleric level. You can use this "
        "ability once per day at 8th level, and an additional time per day for "
        "every four levels beyond 8th.");
  feato(FEAT_HEALING_TOUCH, "healing touch", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "you can heal with your touch",
        "You can touch a living creature as a standard action, healing it for 20 + 1d4 points "
        "of damage plus 1 for every two cleric levels you possess. You can only "
        "use this ability on a creature that is below half their total hit points. You can use "
        "this ability a number of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_CURING_TOUCH, "curing touch", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "you can heal with alchemical salves",
        "You can apply a salve to yourself as a swift action, healing for 5 hp "
        "You can use this ability a number of times per day equal to 1/2 your alchemist level. "
        "Requires the spontaneous healing alchemist discovery.  If you have the curing touch "
        "alchemist discovery as well, you can target other creatures and use it once per day "
        "per level of alchemist instead of for every two levels.");
  feato(FEAT_EMPOWERED_HEALING, "empowered healing", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "heal spells empowered",
        "At 6th level, all of your cure spells are treated as if they were empowered, "
        "increasing the amount of damage healed by half (+50%). This does not "
        "apply to damage dealt to undead with a cure spell. This does not stack "
        "with the Empower Spell metamagic feat.");
  feato(FEAT_KNOWLEDGE, "knowledge", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "your lore ability is enhanced",
        "You get +4 to your lore checks and you can apply your wisdom bonus to your "
        "lore checks.");
  feato(FEAT_EYE_OF_KNOWLEDGE, "eye of knowledge", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "summon a wizard eye",
        "Like the 'wizard eye' spell available to arcane casters, you can summon a "
        "wizard eye which you can control to scout for you.  You can use this "
        "ability a number of times per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_BLESSED_TOUCH, "blessed touch", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "can bless with your touch",
        "You can touch a willing creature as a standard action, infusing it with the "
        "power of divine order and allowing it to treat all attack rolls, skill "
        "checks, ability checks, and saving throws for 1 round as if the natural "
        "d20 roll resulted in an 11. You can use this ability a number of times "
        "per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_LAWFUL_WEAPON, "lawful weapon", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "your weapons becomes lawful",
        "Any weapon you wield behaves as if it is axiomatic, and will do additional 2d6 "
        "damage against chaotic-raced and chaotic-aligned opponents.");
  feato(FEAT_DECEPTION, "deception", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "gain stealth and disguise as class abilities",
        "Both stealth and disguise are now class abilities to you, making them twice "
        "as effective at training.");
  feato(FEAT_COPYCAT, "copycat", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "can innate create mirror images",
        "You can use 'mirror image', like the arcane spell.  You can use this ability "
        "a number of times per day equal to your Wisdom modifier.");
  feato(FEAT_MASS_INVIS, "mass invis", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "can use mass invisibility",
        "You can use 'mass invisibility', like the arcane spell.  You can use this ability "
        "a number of times per day equal to your Wisdom modifier.");
  feato(FEAT_RESISTANCE, "resistance", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "gain bonus to all resistances",
        "You get +1 to all your resistances per 6 cleric levels.");
  feato(FEAT_SAVES, "saves", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "gain bonus to all saves",
        "You get +1 to all your saves per 6 cleric levels.");
  feato(FEAT_AURA_OF_PROTECTION, "aura of protection", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "grant AC/sves bonus to group",
        "You can grant +1 to all saves and AC per 6 cleric levels to all your "
        "group members for 4 rounds.  You can use this ability a number of times "
        "per day equal to 3 + your Wisdom modifier.");
  feato(FEAT_ETH_SHIFT, "ethereal shift", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "shift between ethereal/prime planes at will",
        "You gain the ability to shift group members or yourself, at will, to the "
        "ethereal plane and back to the prime material plane. Command is: ethshift");
  feato(FEAT_BATTLE_RAGE, "battle rage", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "bonus to hitroll/damage",
        "You add your cleric level / 4 to both your hitroll and damroll for "
        "5 rounds.  You can use this ability a number of times per day equal "
        "to your Wisdom modifier.  Note: You need at least 4 levels in cleric "
        "class to use this ability.");
  feato(FEAT_WEAPON_EXPERT, "weapon expert", TRUE, FALSE, FALSE, FEAT_TYPE_DOMAIN_ABILITY,
        "+1 to attack rolls, can use all martial weapons",
        "This feat behaves like the martial weapon proficiency, granting you proficiency "
        "in all weapons except for exotic ones.  In addition you get a +1 to all "
        "attack rolls when wielding a weapon.");

  /* Wild Feats (druid) */
  feato(FEAT_NATURAL_SPELL, "natural spell", TRUE, TRUE, FALSE, FEAT_TYPE_WILD,
        "allows casting of spells while wildshaped",
        "Upon selecting this feat, the character is able to cast spells while wildshaped.");
  feat_prereq_attribute(FEAT_NATURAL_SPELL, AB_WIS, 13);
  feat_prereq_feat(FEAT_NATURAL_SPELL, FEAT_WILD_SHAPE, 1);

  /*****/
  /* Class ability feats */

  /* warrior */
  feato(FEAT_ARMOR_TRAINING, "armor training", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "reduced armor penalty, increased maxdex",
        "The penalty caused by wearing armor is reduced by 1 per rank of this feat.  "
        "In addition your maximum dexterity for your armoring increased 1 per rank.");
  feato(FEAT_WEAPON_TRAINING, "weapon training", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "+2 confirm crit, +1 min diceroll",
        "You get to reroll your attack diceroll if you roll 1, per rank of this feat "
        "(example: 4 ranks means you cannot roll lower than 5).  In addition, you "
        "get +2 to confirming critical hits per rank.");
  feato(FEAT_STALWART_WARRIOR, "stalwart warrior", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "apply class level to AC",
        "The warrior gets to apply all warrior-levels divided by 4 to AC.");
  feato(FEAT_ARMOR_MASTERY, "armor mastery i", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "gains Damage Reduction 5/all when armored",
        "Gain Damage Reduction 5/all whenever wearing armor or using a shield.");
  feato(FEAT_WEAPON_MASTERY, "weapon mastery i", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "cannot be disarmed, +2 ac when using any weapon",
        "Gain immunity to disarm attempts, in addition, while wielding any type of "
        "weapon, gain a bonus 2 to deflection AC.");
  feato(FEAT_ARMOR_MASTERY_2, "armor mastery ii", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "become extremely skilled with shields",
        "You gain 2 bonus AC while using a shield, in addition you gain 25 magic "
        "resistance while using a shield.");
  feato(FEAT_WEAPON_MASTERY_2, "weapon mastery ii", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "+6 to CMD and CMB",
        "While wielding a weapon, gain a +6 bonus to all combat maneuver attempts and +6 bonus to defending "
        "against any combat maneuver.");

  /* Cleric */
  /* turn undead is below, shared with paladin */

  /* Cleric / Paladin */
  feato(FEAT_TURN_UNDEAD, "turn undead", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "can cause fear in or destroy undead based on class level and charisma bonus",
        "can cause fear in or destroy undead based on class level and charisma bonus");

  feato(FEAT_CHANNEL_ENERGY, "channel energy", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "If good, will damage undead and heal living.  If evil, will damage living and heal undead.",
        "Channelling divine energy will cause 1d6 damage to all undead in the area per two cleric levels.  "
        "It will also heal all living creatures for the same amount in the same area. "
        "These effects are for channelers with good alignments.  If evil, then undead are healed "
        "and living are damaged.  Neutral channelers must decide, and this choice cannot "
        "be undone without a respec. "
        "Paladins and Blackguards can add their class levels to any cleric levels. Mystic theurges can add half. "
        "See the 'channelenergy' command.");

  /* Paladin / Champion of Torm */
  feato(FEAT_DIVINE_GRACE, "divine grace", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "charisma bonus added to all saving throw checks",
        "charisma bonus added to all saving throw checks");

  /* Paladin */
  /* divine grace is above, shared with champion of torm */
  /* turn undead is above, shared with cleric */
  feato(FEAT_AURA_OF_COURAGE, "aura of courage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Immunity to fear attacks, +4 bonus to fear saves for group members",
        "Immunity to fear attacks, +4 bonus to fear saves for group members");
  feato(FEAT_SMITE_EVIL, "smite evil", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "add charisma bonus to hit roll and paladin level to damage",
        "add charisma bonus to hit roll and paladin level to damage against good aligned targets. "
        "against evil outsiders, evil dragons, or undead, "
        "2x paladin level is added to damage instead.");
  feato(FEAT_DETECT_EVIL, "detect evil", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "able to detect evil alignments",
        "able to detect evil alignments");
  feato(FEAT_AURA_OF_GOOD, "aura of good", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "able to detect good alignments",
        "able to detect good alignments");
  feato(FEAT_AURA_OF_FAITH, "aura of faith", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Good allies in the paladin's presence gain a +1 to attack and damage rolls.",
        "Good allies in the paladin's presence gain a +1 to attack and damage rolls.");
  feato(FEAT_AURA_OF_JUSTICE, "aura of justice", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Use a smite evil slot to give all allies in your presence a single smite evil use. See 'auraofjustice' command.",
        "Use a smite evil slot to give all allies in your presence a single smite evil use. See 'auraofjustice' command.");
  feato(FEAT_DIVINE_HEALTH, "divine health", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "immune to disease",
        "immune to disease");
  feato(FEAT_LAYHANDS, "lay on hands", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Powerful divine healing ability usable a limited number of times a day",
        "Powerful divine healing ability usable a limited number of times a day");
  feato(FEAT_REMOVE_DISEASE, "remove disease", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "can cure diseases",
        "can cure diseases (purify)");
  feato(FEAT_CALL_MOUNT, "call mount", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Allows you to call a paladin mount",
        "Allows you to call a paladin mount");
  feato(FEAT_DIVINE_BOND, "divine bond", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "bonuses to attack and damage rolls",
        "Hitroll and Damage bonus of 1 + paladin-level/3 for levels above 5, the "
        "bonus caps at 6, plus other benefits as your paladin level increases. See the divinebond command.");
  feato(FEAT_GLORIOUS_RIDER, "glorious rider", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "use cha instead of dex for ride checks",
        "Normally ride checks for performing skills while mounted, uses your dexterity "
        "bonus to compute your chance.  With this feat you can use your charisma bonus instead.  "
        "This does NOT eliminate the armor penalty created by armor as related to dexterity.");
  feato(FEAT_LEGENDARY_RIDER, "legendary rider", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "no armor penalty to ride check, can block extra attack",
        "You can now block up to two attacks a round with successful ride checks "
        "against the incoming attacks.  In addition, you no longer suffer any "
        "penalty because of armor while mounted.");
  feato(FEAT_EPIC_MOUNT, "epic mount", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "you get a more powerful mount",
        "You get an epic level mount with some special abilities.");
  feato(FEAT_AURA_OF_RIGHTEOUSNESS, "aura of righteousness", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Allies get +4 bonus to enchantment spells.  Paladin gains damage reduction 1/-",
        "Allies get +4 bonus to enchantment spells.  Paladin gains damage reduction 1/-");
  feato(FEAT_HOLY_WARRIOR, "holy warrior", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "When using layonhands or channel energy, the healing amount is increased, or if used on undead the damage is increased.",
        "When using layonhands or channel energy, the healing amount is increased, or if used on undead the damage is increased. "
        "Additionally the paladin's damage reduction is increased by 2/-. This stacks with aura of righteousness");
  feato(FEAT_HOLY_CHAMPION, "holy champion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "When using layonhands or channel energy, the healing amount is maximized, or on undead the damage is maximized.",
        "When using layonhands or channel energy, the healing amount is maximized, or on undead the damage is maximized. "
        "Additionally the paladin's damage reduction is increased by 2/-. This stacks with aura of righteousness, and holy warrior. "
        "Finally, anytime smite evil is used on an evil outsider, the outsider has a chance to be banished.  This may not work on boss-type mobs.");

  // Blackguard
  feato(FEAT_TOUCH_OF_CORRUPTION, "touch of corruption", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "With a touch, a blackguard can inflict negative damage and add cruelty effects as they grow in power.",
        "With a touch, a blackguard can inflict negative damage and add cruelty effects as they grow in power. See 'corruptingtouch' command.");
  feato(FEAT_DETECT_GOOD, "detect good", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "able to detect good alignments",
        "able to detect good alignments");
  feato(FEAT_AURA_OF_EVIL, "aura of evil", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "able to detect evil alignments",
        "able to detect evil alignments");
  feato(FEAT_AURA_OF_DESPAIR, "aura of despair", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "All non-allies in your presence suffer a -2 penalty to saving throws.",
        "All non-allies in your presence suffer a -2 penalty to saving throws.");
  feato(FEAT_AURA_OF_COWARDICE, "aura of cowardice", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Imposes penalties to fear-based saving throws or fear-immunity abilities.",
        "All non-allies in your presence suffer a -4 penalty to fear-based saving throws. "
        "If the target is under the effect of some form of fear-immunity, this aura instead "
        "will cancel that immunity out, and not impose a penalty to fear saves.");
  feato(FEAT_AURA_OF_VENGEANCE, "aura of vengeance", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Use a smite good slot to give all allies in your presence a single smite good use. See 'auraofvengeance' command.",
        "Use a smite good slot to give all allies in your presence a single smite good use. See 'auraofvengeance' command.");
  feato(FEAT_UNHOLY_RESILIENCE, "unholy resilience", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "charisma bonus added to all saving throw checks",
        "charisma bonus added to all saving throw checks");
  feato(FEAT_PLAGUE_BRINGER, "plague bringer", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "the blackguard is immune to disease.",
        "the blackguard is immune to disease.");
  feato(FEAT_FIENDISH_BOON, "fiendish boon", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "the blackguard gains the ability to add certain affects to his weapons.  See 'fiendishboon' command.",
        "the blackguard gains the ability to add certain affects to his weapons.  See 'fiendishboon' command.");
  feato(FEAT_CRUELTY, "blackguard cruelties", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "the blackguard is able to add negative effects to his touch of corruption.  See 'cruelties' command.",
        "the blackguard is able to add negative effects to his touch of corruption.  See 'cruelties' command.");
  feato(FEAT_AURA_OF_SIN, "aura of sin", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Evil allies in the blackguard's presence gain a +1 to attack and damage rolls.",
        "Evil allies in the blackguard's presence gain a +1 to attack and damage rolls.");
  feato(FEAT_AURA_OF_DEPRAVITY, "aura of depravity", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Foes get -4 penalty to enchantment spells.  Blackguard gains damage reduction 1/-",
        "Foes get -4 penalty to enchantment spells.  Blackguard gains damage reduction 1/-");
  feato(FEAT_UNHOLY_WARRIOR, "unholy warrior", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "When using corrupting touch or channel energy, the healing amount on undead is increased, or otherwise the damage is increased.",
        "When using corrupting touch or channel energy, the healing amount on undead is increased, or otherwise the damage is increased. "
        "Additionally the blackguard's damage reduction is increased by 2/-. This stacks with aura of depravity.");
  feato(FEAT_UNHOLY_CHAMPION, "unholy champion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "When using corrupting touch or channel energy, the healing amount on undead is maximized, or otherwise the damage is maximized.",
        "When using corrupting touch or channel energy, the healing amount on undead is maximized, or otherwise the damage is maximized. "
        "Additionally the blackguard's damage reduction is increased by 2/-. This stacks with aura of depravity, and holy warrior. "
        "Finally, anytime smite good is used on a good outsider, the outsider has a chance to be banished.  This may not work on boss-type mobs.");

  /* Rogue */
  /* trap sense below (shared with berserker) */
  /* uncanny dodge below (shared with berserker) */
  /* improved uncanny dodge below (shared with berserker) */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /*talent*/ feato(FEAT_CRIPPLING_STRIKE, "crippling strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                   "Chance to do strength damage with a sneak attack.",
                   "Chance to do strength damage with a sneak attack.");
  /*talent*/ feato(FEAT_IMPROVED_EVASION, "improved evasion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                   "as evasion but half damage of failed save",
                   "as evasion but half damage of failed save");
  feato(FEAT_EVASION, "evasion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "on successful reflex save no damage from spells and effects",
        "on successful reflex save no damage from spells and effects");
  feato(FEAT_TRAPFINDING, "trapfinding", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "+4 to detecting traps",
        "+4 to detecting traps (detecttrap)");
  /*adv talent*/ feato(FEAT_DEFENSIVE_ROLL, "defensive roll", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                       "can survive a potentially fatal blow",
                       "can survive a potentially fatal blow, has long cooldown before usable "
                       "again (automatic usage)");
  /*epic talent*/ feato(FEAT_BACKSTAB, "backstab", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                        "bonus to attack and double damage",
                        "You get +4 to attack (and another +1 if sneaking, +1 if hiding).  Do double "
                        "damage as well.  In addition, backstab is a move-action instead of full "
                        "round action.  Backstab requires a piercing weapon to be a success.");
  /*epic talent*/ feato(FEAT_SAP, "sap", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                        "chance to knock opponent out",
                        "When both sneaking and hiding (in that order) and wielding a bludgeon weapon, you can perform a special sap attack "
                        "which on success, will knock an opponent down and if they fail a fortitude save "
                        "versus rogue-level / 2 + dex bonus they will be paralyzed for 2 rounds.  This type of "
                        "attack will get a -6 penalty to success, but can be negated by wielding a "
                        "2-handed bludgeoning weapon (+4 bonus) and if your opponent can't see you (+4 bonus).");
  /*epic talent*/ feato(FEAT_VANISH, "vanish", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                        "escape combat!",
                        "Once per day, you can use vanish as a free action to completely escape combat "
                        "effectively disengaging and entering a sneak/hidden mode.  This also heals "
                        "10 hitpoints and gives 25 percent concealment for 2 rounds.");
  /*epic talent*/ feato(FEAT_IMPROVED_VANISH, "improved vanish", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                        "improves vanish feat",
                        "Vanish now heals 20 hitpoints and gives 100 percent concealment for 2 rounds.");
  /*talent*/ feato(FEAT_SLIPPERY_MIND, "slippery mind", TRUE, TRUE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                   "extra chance for will saves vs mind affecting spells",
                   "extra chance for will saves vs mind affecting spells");
  /*talent*/ feato(FEAT_APPLY_POISON, "apply poison", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                   "can apply poison to weapons",
                   "can apply poison to weapons (applypoison)");
  /*talent*/ feato(FEAT_DIRT_KICK, "dirt kick", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                   "can kick dirt into opponents face (blindness)",
                   "Upon a successful unarmed attack roll you will kick dirt into your opponents "
                   "eyes causing minor damage.  In addition if your opponent fails his/her reflex "
                   "saving throw versus 10 + your-level/2 + dex-bonus, they will be blinded for 1 "
                   "to level/5 rounds.  To use this skill type: dirtkick <opponent>.");
  feato(FEAT_SNEAK_ATTACK, "sneak attack", TRUE, FALSE, TRUE, FEAT_TYPE_COMBAT,
        "+1d6 to damage when flanking",
        "+1d6/rank to damage when flanking, opponent is flat-footed, or opponent is without dexterity bonus");
  feat_prereq_class_level(FEAT_SNEAK_ATTACK, CLASS_ROGUE, 2);
  feato(FEAT_WEAPON_PROFICIENCY_ROGUE, "weapon proficiency - rogues", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
        "proficiency in rogue weapons",
        "You are proficient in the usage of hand-crossbows, rapiers, sap, short-sword "
        "and short-bow.");

  /* Rogue / Berserker */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_TRAP_SENSE, "trap sense", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "auto-sense traps",
        "Normally to find traps you have to actively try to detect them.  With "
        "this feat you will make a perception check to detect traps automatically. "
        "For every point you have in this feat your bonus to your check is increase by 1.");
  feato(FEAT_UNCANNY_DODGE, "uncanny dodge", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "retains dex bonus when flat footed",
        "retains dexterity bonus when flat footed");
  feato(FEAT_IMPROVED_UNCANNY_DODGE, "improved uncanny dodge", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "cannot be flanked",
        "cannot be flanked, unless opponents berserker/rogue levels are 4 or more");

  /* Ranger */
  /* the favored enemy mechanic is already built into the study system, not utilizing these two feats */
  /* unfinished */ feato(FEAT_FAVORED_ENEMY_AVAILABLE, "favored enemy available", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                         "can choose an enemy type as a favored enemy",
                         "can choose an enemy type as a favored enemy");
  /* unfinished */ feato(FEAT_FAVORED_ENEMY, "favored enemy", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
                         "Gain bonuses when fighting against a particular type of enemy",
                         "Gain bonuses when fighting against a particular type of enemy");
  /* modified from original */
  feato(FEAT_CAMOUFLAGE, "camouflage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "gain stealth bonus in nature",
        "Gains +6 bonus to sneak/hide in nature");
  feato(FEAT_HIDE_IN_PLAIN_SIGHT, "hide in plain sight", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "can hide in battle",
        "This feat grants the ability to perform the stealth maneuver: hide, even while in combat.  This check is made with a -8 penalty");
  /* unfinished */ feato(FEAT_SWIFT_TRACKER, "swift tracker", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                         "No penalty while autotracking.",
                         "No penalty while autotracking.");
  /* combat mastery should be either an archer set of feats or dual wielding set of feats, for now we give them both */
  /* we have to make separate feats for dual weapon fighting (two weapon fighting) */
  feato(FEAT_DUAL_WEAPON_FIGHTING, "dual weapon fighting", TRUE, FALSE, FALSE, FEAT_TYPE_COMBAT,
        "reduces penalty for two weapon fighting",
        "reduces penalty for two weapon fighting while wearing light or lighter armor");
  feato(FEAT_IMPROVED_DUAL_WEAPON_FIGHTING, "improved dual weapon fighting", TRUE, FALSE, FALSE, FEAT_TYPE_COMBAT,
        "extra attack with offhand weapon at -5 penalty",
        "extra attack with offhand weapon at -5 penalty while wearing light or lighter armor");
  feato(FEAT_GREATER_DUAL_WEAPON_FIGHTING, "greater dual weapon fighting", TRUE, FALSE, FALSE, FEAT_TYPE_COMBAT,
        "gives an additional offhand weapon attack at -10 penalty",
        "gives an additional offhand weapon attack at -10 penalty while wearing light or lighter armor");
  feat_prereq_feat(FEAT_GREATER_DUAL_WEAPON_FIGHTING, FEAT_DUAL_WEAPON_FIGHTING, 1);
  feat_prereq_attribute(FEAT_GREATER_DUAL_WEAPON_FIGHTING, AB_DEX, 19);
  /* point blank shot */
  /* rapid shot */
  /* manyshot */
  /* epic */
  /* epic manyshot */
  feato(FEAT_PERFECT_DUAL_WEAPON_FIGHTING, "perfect dual weapon fighting", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "Extra attack with offhand weapon",
        "Extra attack with offhand weapon while wearing light or lighter armor");
  feat_prereq_feat(FEAT_PERFECT_DUAL_WEAPON_FIGHTING, FEAT_GREATER_DUAL_WEAPON_FIGHTING, 1);
  feat_prereq_attribute(FEAT_PERFECT_DUAL_WEAPON_FIGHTING, AB_DEX, 21);
  epicfeat(FEAT_PERFECT_DUAL_WEAPON_FIGHTING);

  feato(FEAT_BANE_OF_ENEMIES, "bane of enemies", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "weapon acts as bane",
        "Any weapon you wield that strikes an opponent that is a favored enemy will "
        "act as a bane weapon and do an additional 2d6 damage.");
  feato(FEAT_EPIC_FAVORED_ENEMY, "epic favored enemy", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "gain extra bonus to dam against fav enemy",
        "You will gain an extra +4 to attack bonus and damage against any of your "
        "favored enemies.");

  /* Ranger / Druid */
  feato(FEAT_ANIMAL_COMPANION, "animal companion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Can call a loyal companion animal that accompanies the adventurer.",
        "Can call a loyal companion animal that accompanies the adventurer.");
  feato(FEAT_BOON_COMPANION, "boon companion", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "Your animal companion is stronger than normal.",
        "Your level is considered to be 5 higher when it comes to setting stats on your animal companion.");
  feat_prereq_feat(FEAT_BOON_COMPANION, FEAT_ANIMAL_COMPANION, 1);
  /* unfinished */ feato(FEAT_WILD_EMPATHY, "wild empathy", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                         "The adventurer can improve the attitude of an animal.",
                         "The adventurer can improve the attitude of an animal.");
  /* unfinished */

  /* Druid */
  feato(FEAT_VENOM_IMMUNITY, "venom immunity", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "grants immunity to poison",
        "grants immunity to poison");
  /* unfinished */ feato(FEAT_NATURE_SENSE, "nature sense", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                         "+2 to lore and survival skills",
                         "+2 to lore and survival skills");
  /* unfinished */ feato(FEAT_RESIST_NATURES_LURE, "resist nature's lure", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                         "+4 to resist spells and spell like abilities from fey creatures",
                         "+4 to resist spells and spell like abilities from fey creatures");
  /* unfinished */ feato(FEAT_THOUSAND_FACES, "a thousand faces", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                         "Can alter one's physical appearance, giving +10 to disguise checks.",
                         "Can alter one's physical appearance, giving +10 to disguise checks.");
  /* modified from original */
  feato(FEAT_TRACKLESS_STEP, "trackless step", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "bonus to hide/sneak in nature",
        "+4 bonus to hide/sneak in nature");
  feato(FEAT_WILD_SHAPE, "wild shape", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "Gain the ability to shapechange",
        "Gains the ability to turn into any small or medium animal and back again "
        "once per day. Options for new forms include all creatures with the "
        "animal type.  Changing form (to animal or back) is a standard action "
        "and doesn't provoke an attack of opportunity.");
  feato(FEAT_WILD_SHAPE_2, "wild shape ii", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Advance the ability to shapechange",
        "Can use wildshape to change into a Large or Tiny animal or a Small elemental.");
  feato(FEAT_WILD_SHAPE_3, "wild shape iii", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Advance the ability to shapechange",
        "Can use wildshape to change into a Huge or Diminutive animal, a Medium "
        "elemental, or a Small or Medium plant creature.");
  feato(FEAT_WILD_SHAPE_4, "wild shape iv", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Advance the ability to shapechange",
        "Can use wildshape to change into a Large elemental or a Large plant creature.");
  feato(FEAT_WILD_SHAPE_5, "wild shape v", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Advance the ability to shapechange",
        "Can use wildshape to change into a Huge elemental or a Huge plant creature.");
  feato(FEAT_WEAPON_PROFICIENCY_DRUID, "weapon proficiency - druids", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
        "proficiency in druid weapons",
        "You are proficient in the usage of clubs, daggers, quarterstaff, darts, sickle, scimitar, shortspear, spear and slings.");

  /* Druid / Monk */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_TIMELESS_BODY, "timeless body", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "immune to negative aging effects (unfinished)",
        "immune to negative aging effects (unfinished) - currently gives a flat 25 percent "
        "reduction to all incoming negative damage");

  /* Monk */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_UNARMED_STRIKE, "unarmed strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Unarmed attacks are considered to be weapons, unarmed damage",
        "Unarmed attacks are considered to be weapons regarding bonuses and penetration.  In addition "
        "the unarmed damage increases by monk level: Level 1-3: 1d6, 4-7: 1d8, 8-11: 1d10, 12-15: 2d6, "
        "16-19: 4d4, 20-24: 4d5, 25-29: 4d6, 30: 7d5.");
  feato(FEAT_WIS_AC_BONUS, "monk wisdom bonus to AC", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "With this feat, one gets to apply their wisdom bonus to AC",
        "With this feat, one gets to apply their wisdom bonus to AC.");
  feato(FEAT_LVL_AC_BONUS, "monk innate ac bonus", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Get AC bonus every 4 levels of monk",
        "Get +1 to AC Bonus at monk-levels 1, 4, 8, 12, 16, 20, 24, 28");
  /* improved unarmed strike monks get for free */
  /*unfinished*/ feato(FEAT_KI_STRIKE, "ki strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                       "unarmed attack considered a magical weapon (unfinished)",
                       "unarmed attack considered a magical weapon [note: until fixed this feat "
                       "just gives a +1 to hitroll/damroll]");
  feato(FEAT_STILL_MIND, "still mind", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "+2 bonus on saving throws vs. Enchantments",
        "+2 bonus on saving throws vs. Enchantments");
  feato(FEAT_WHOLENESS_OF_BODY, "wholeness of body", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "can heal class level*2 + 20 hp to self",
        "can heal class level*2 + 20 hp to self");
  feato(FEAT_SLOW_FALL, "slow fall", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "no damage for falling 1 room/feat rank",
        "no damage for falling 1 room/feat rank");
  feato(FEAT_ABUNDANT_STEP, "abundant step", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "magically move between tight spaces, as the spell dimension door",
        "Magically move between tight spaces, as the spell dimension door.  You "
        "can even go through doors.  To use, you must give directions from your "
        "current location.  Example: abundantstep w w n n e 2w n n e");
  feato(FEAT_DIAMOND_BODY, "diamond body", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "immune to disease",
        "immune to disease");
  feato(FEAT_DIAMOND_SOUL, "diamond soul", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "spell resistance equal to class level + 10",
        "spell resistance equal to class level + 10");
  feato(FEAT_EMPTY_BODY, "empty body", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "50 percent concealment for 1 round/monk level per day",
        "50 percent concealment for 1 round/monk level per day");
  feato(FEAT_FLURRY_OF_BLOWS, "flurry of blows", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "extra attack when fighting unarmed at -2 to all attacks",
        "Flurry of Blows is a special mode available to monks (type flurryofblows).  You get an extra attack "
        "at full BAB, but at a penalty to -2 to all your attacks.  At level 5 the "
        "penalty drops to -1, and at level 9 the penalty disappears completely. ");
  feato(FEAT_GREATER_FLURRY, "greater flurry", TRUE, FALSE, FALSE, FEAT_TYPE_COMBAT,
        "extra unarmed attack when using flurry of blows",
        "extra unarmed attack when using flurry of blows, at level 15 you get yet "
        "another bonus attack at full BAB");
  /*unfinished*/ feato(FEAT_PERFECT_SELF, "perfect self", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                       "Gain 10/magic damage reduction (unfinished)",
                       "Gain 10/magic damage reduction [note: until our damage reduction system is "
                       "changed, this feat will give a flat 3 damage reduction against ALL incoming attacks");
  feato(FEAT_PURITY_OF_BODY, "purity of body", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "immune to poison",
        "immune to poison");
  feato(FEAT_QUIVERING_PALM, "quivering palm", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "chance to kill on strike with unarmed attack",
        "You will do your wisdom bonus as bonus damage to your next unarmed strike, "
        "in addition, to opponents that are your level or lower, they have to make "
        "a fortitude save vs DC: your monk level/2 + your wisdom bonus + 10 in order "
        "to survive your quivering palm attack");
  feato(FEAT_WEAPON_PROFICIENCY_MONK, "weapon proficiency - monks", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
        "proficiency in monk weapons",
        "You are proficient in the usage of quarterstaff, kama, siangham, and shuriken.");
  /* not imped */ feato(FEAT_TONGUE_OF_THE_SUN_AND_MOON, "tongue of the sun and moon [not impd]", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "[not implemented] can speak any language", "[not implemented] can speak any language");
  /*epic*/ /* free blinding speed */
  /*epic*/ feato(FEAT_KEEN_STRIKE, "keen strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                 "critical more unarmed, better stunning fist",
                 "Your threat range with unarmed attacks increase by 1, in addition you get "
                 "a +4 bonus to your stunning fist DC.");
  /*epic*/ feato(FEAT_OUTSIDER, "outsider", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                 "you become an outsider",
                 "You gain the ability to shift at will to the ethereal plane and back "
                 "additionally you gain 15 percent concealment.");

  /* Bard */
  feato(FEAT_WEAPON_PROFICIENCY_BARD, "weapon proficiency - bards", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
        "proficiency in bard weapons",
        "You are proficient in the usage of long swords, rapiers, sap, short swords, short bows and whips.");
  feato(FEAT_BARDIC_KNOWLEDGE, "bardic knowledge", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "+Int modifier bonus on knowledge checks.",
        "+Int modifier bonus on knowledge checks.");
  feato(FEAT_BARDIC_MUSIC, "bardic music", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Use Perform skill to create various magical effects.",
        "Use Perform skill to create various magical effects.");
  /* unfinished */ feato(FEAT_COUNTERSONG, "countersong", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                         "(not yet implemented)Boost group members' resistance to sonic attacks.",
                         "(not yet implemented)Boost group members' resistance to sonic attacks.");
  /* 1*/ feato(FEAT_SONG_OF_HEALING, "song of healing", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "song to heal group members (lyre)",
               "When this song is played, its beautiful verse allows any present gain the "
               "benefit of its healing power. The amount of healing it provides is "
               "dependent on the level of the musician. To play this song, the bard must be "
               "holding a lyre.");
  /* 2*/ feato(FEAT_DANCE_OF_PROTECTION, "dance of protection", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "dance to protect group members (drum)",
               "When this dance is performed, it supplies a level of protection "
               "to its observer. It gives bonuses to armor, making it harder for opponents "
               "to hit the observer. The dance also gives the benefit of a heightened level "
               "of spell resistance. To perform this dance, a bard must be holding a drum.");
  /* 3*/ feato(FEAT_SONG_OF_FOCUSED_MIND, "song of focused mind", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "song to augment casters (harp)",
               "This song allows the bard to sing a song that speeds up the "
               "memorization and praying for spells by anyone in the room at the "
               "time that the song is sung.  This song lends itself to the harp.");
  /* 5*/ feato(FEAT_SONG_OF_HEROISM, "song of heroism", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "song to enhance combat abilities (drum)",
               "When this song is played, the listeners are enhanced with fighting "
               "abilities. In most cases, the listeners gain enhancements in their "
               "abilities to hit targets and to inflict damage upon them. However, if the "
               "singer is extremely proficient, the listeners may gain an extra attack per "
               "round. To play this song, a bard must be holding a drum.");
  /* 7*/ feato(FEAT_ORATORY_OF_REJUVENATION, "oratory of rejuvenation", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "oratory of light healing of hps/moves (lyre)",
               "When this oratory is performed, the listeners regain lost movement points. The "
               "oratory also provides a minor level of healing to its listeners, and has a "
               "slight chance of removing any poisons present in their system. To play this "
               "oratory, a bard must be holding a lyre.");
  /* 9*/ feato(FEAT_SONG_OF_FLIGHT, "song of flight", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "song bestows flight and restores moves (horn)",
               "When listeners hear this song playing, they are given the ability to fly for "
               "a period of time. Movement points are also slightly restored. To play this "
               "song, the bard must be holding a horn.");
  /*10*/ feato(FEAT_EFFICIENT_PERFORMANCE, "efficient performance", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
               "performance takes move instead of standard action",
               "A bardic performance now only takes a move action instead of a standard action.");
  /*11*/ feato(FEAT_SONG_OF_REVELATION, "song of revelation", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "song enhances perception (flute)",
               "When this song is played, the listeners begin to see things which they "
               "formerly could not see. Depending on the proficiency of the musical artist, "
               "listeners may see invisible beings, magical enchantments, alignments, hidden "
               "beings, and adjacent rooms. To play this song, a bard must be holding a "
               "flute.");
  /*13*/ feato(FEAT_SONG_OF_FEAR, "song of fear", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "song inspires fear in foes (harp)",
               "This songs put immense fear into the heart of the bard's enemies. They will "
               "fight less effectively and attempt to flee as quickly as possible.  This "
               "song lends itself well to the harp.");
  /*15*/ feato(FEAT_ACT_OF_FORGETFULNESS, "skit of forgetfulness", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "skit inspires forgetfulness in foes (flute)",
               "When this skit is performed, a mob may forget it has been attacked. To act "
               "this skit, a bard must be holding a flute.");
  /*17*/ feato(FEAT_SONG_OF_ROOTING, "song of rooting", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "song bogs down foes (mandolin)",
               "This song creates a strong sense of rooting amongst the enemies of the bard. "
               "Their get sucked into the ground, making them unable to leave the area, while "
               "also reducing their capabilities to fight effectively.  This song is appropriate "
               "for the mandolin.");
  /*epic*/
  /*21*/ feato(FEAT_SONG_OF_DRAGONS, "song of dragons", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "song prepares group for dragon-slaying (horn)",
               "When this song is played, it can give three different affects to it's "
               "listeners during each verse: It heals nearly half the affect of the heal "
               "song, or it grants a great deal of saving versus reflex, as well as enhancing "
               "armor class. In order to play this song the bard must be holding a horn.");
  /*25*/ feato(FEAT_SONG_OF_THE_MAGI, "song of the magi", TRUE, FALSE, FALSE, FEAT_TYPE_PERFORMANCE,
               "song hampers magic defense of foes (mandolin)",
               "When this song is played, it will strengthen the offensive magic of "
               "the group-members of the bard, by reducing the spell-save and magic resistance "
               "of all their enemies in the area.  Uses the mandolin.");

  /* Berserker */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /* uncanny dodge above (shared with rogue) */
  /* improved uncanny dodge above (shared with rogue) */
  /* trap sense above (shared with rogue) */
  feato(FEAT_INDOMITABLE_WILL, "indomitable will", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "bonus to will save while raging",
        "While in rage, gain a +4 bonus on Will saves");
  feato(FEAT_SHRUG_DAMAGE, "shrug damage", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "Shrug off damage, grants damage reduction",
        "Your extensive training and violent lifestyle allow you to shrug off a "
        "portion of incoming damage.  This ability grants you DR 1/- for every point"
        "invested in this feat.");
  feato(FEAT_RAGE, "rage", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "+4 bonus to con, str, and will for several rounds",
        "+4 bonus to constitution, strength and will-saves, but 2 penalty to AC, for "
        "(2 * constitution-bonus + 6) rounds");
  feato(FEAT_GREATER_RAGE, "greater rage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "+6 to str, con, and will when raging",
        "+6 to strength, constitution, and will-saves when raging");
  feato(FEAT_MIGHTY_RAGE, "mighty rage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "+9 to str, con and will when raging",
        "+9 to strength, constitution, and will-saves when raging");
  feato(FEAT_INDOMITABLE_RAGE, "indomitable rage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "+12 to str, con and will when raging",
        "+12 to strength, constitution, and will-saves when raging");
  feato(FEAT_TIRELESS_RAGE, "tireless rage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "no fatigue after raging",
        "no fatigue after raging");
  /*temporary mechanic*/ feato(FEAT_FAST_MOVEMENT, "fast movement", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
                               "reduces movement usage, and increases movement regen",
                               "Reduces movement usage, and increases movement regeneration.  This is a temporary mechanic.");
  /*rage power*/ feato(FEAT_RP_SURPRISE_ACCURACY, "surprise accuracy", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                       "bonus hitroll once/rage",
                       "Gain a +1 morale bonus on one attack roll.  This bonus increases by +1 for "
                       "every 4 berserker levels attained. This power is used as a swift action.  This power "
                       "can only be used once per rage.");
  /*rage power*/ feato(FEAT_RP_POWERFUL_BLOW, "powerful blow", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                       "bonus damage once/rage",
                       "Gain a +1 bonus on a single damage roll. This bonus increases by +1 for "
                       "every berserker level attained. This power is used as a swift action.  "
                       "This power can only be used once per rage.");
  /*rage power*/ feato(FEAT_RP_RENEWED_VIGOR, "renewed vigor", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                       "able to heal while raging",
                       "As a standard action, the berserker heals 3d8 points of damage + her "
                       "Constitution/Str/Dex modifier. For every level the berserker has attained "
                       "this amount of damage healed increases by 1d8.  This power can "
                       "be used only once per day and only while raging.");
  /*rage power*/ feato(FEAT_RP_HEAVY_SHRUG, "heavy shrug", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                       "increased DR while raging",
                       "The berserker's damage reduction increases by 3/-. This increase is always "
                       "active while the berserker is raging.");
  /*rage power*/ feato(FEAT_RP_FEARLESS_RAGE, "fearless rage", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                       "fearless while raging",
                       "While raging, the berserker is immune to the shaken and frightened conditions. ");
  /*rage power*/ feato(FEAT_RP_COME_AND_GET_ME, "come and get me", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                       "take it, but dish it out heavy",
                       "While raging, as a free action the berserker may leave herself open to "
                       "attack while preparing devastating counterattacks. Enemies gain a +4 bonus "
                       "on attack and damage rolls against the berserker until the beginning of "
                       "her next turn, but every attack against the berserker provokes an attack "
                       "of opportunity from her, which is resolved prior to resolving each enemy attack. ");
  /*epic*/ feato(FEAT_RAGING_CRITICAL, "raging critical", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                 "your criticals knock over opponents while raging",
                 "While raging, on successful criticals, against opponents that are standing, and not too "
                 "much larger or smaller than you, your powerful critical attacks will knock "
                 "them down.");
  /*epic*/ feato(FEAT_EATER_OF_MAGIC, "eater of magic", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                 "successful saving throws will heal rager",
                 "While raging, if the rager makes a successful saving throw, they recover "
                 "30 + their level * 2 + physical stat bonuses (dex, con, str) in hit points.");
  /*epic*/ feato(FEAT_RAGE_RESISTANCE, "rage resistance", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                 "gain boost in resistances while raging",
                 "While raging, you will gain a 15 bonus to all your resistances.");
  /*epic*/ feato(FEAT_DEATHLESS_FRENZY, "deathless frenzy", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                 "the rager is nearly unstoppable",
                 "While raging, you have to be brought to -121 or lower to be stopped.");

  /* Sorcerer/Wizard */
  feato(FEAT_SUMMON_FAMILIAR, "summon familiar", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "summon a magical pet",
        "summon a magical pet - help call familiar for more information");
  feato(FEAT_WEAPON_PROFICIENCY_WIZARD, "weapon proficiency - wizards", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
        "proficiency in wizard weapons",
        "You are proficient in the usage of daggers, quarterstaff, club, heavy and light crossbows.");

  /* Psionicist */
  feato(FEAT_WEAPON_PROFICIENCY_PSIONICIST, "weapon proficiency - psionicist", TRUE, FALSE, FALSE, FEAT_TYPE_GENERAL,
        "proficiency in psioncist weapons",
        "You are proficient in the usage of clubs, daggers, heavy crossbows, light crossbows quarterstaves and shortspears.");

  feato(FEAT_COMBAT_MANIFESTATION, "combat manifestation", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "+4 to manifestation concentration checks made in combat or when grappled ",
        "+4 to manifestation concentration checks made in combat or when grappled ");
  feat_prereq_class_level(FEAT_COMBAT_MANIFESTATION, CLASS_PSIONICIST, 1);

  feato(FEAT_ALIGNED_ATTACK_GOOD, "aligned attack (good)", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "Attacks against good targets receive a +1 bonus to hit and +2 bonus to damage.",
        "Attacks against good targets receive a +1 bonus to hit and +2 bonus to damage. "
        "This bonus will not stack with other aligned attack feats.  For example, if you "
        "have both aligned attack good and aligned attack lawful, you cannot gain double "
        "the bonus against a lawful good target.");
  feat_prereq_bab(FEAT_ALIGNED_ATTACK_GOOD, 6);
  feato(FEAT_ALIGNED_ATTACK_EVIL, "aligned attack (evil)", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "Attacks against evil targets receive a +1 bonus to hit and +2 bonus to damage.",
        "Attacks against evil targets receive a +1 bonus to hit and +2 bonus to damage."
        "This bonus will not stack with other aligned attack feats.  For example, if you "
        "have both aligned attack good and aligned attack lawful, you cannot gain double "
        "the bonus against a lawful good target.");
  feat_prereq_bab(FEAT_ALIGNED_ATTACK_EVIL, 6);
  feato(FEAT_ALIGNED_ATTACK_CHAOS, "aligned attack (chaotic)", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "Attacks against chaotic targets receive a +1 bonus to hit and +2 bonus to damage.",
        "Attacks against chaotic targets receive a +1 bonus to hit and +2 bonus to damage."
        "This bonus will not stack with other aligned attack feats.  For example, if you "
        "have both aligned attack good and aligned attack lawful, you cannot gain double "
        "the bonus against a lawful good target.");
  feat_prereq_bab(FEAT_ALIGNED_ATTACK_CHAOS, 6);
  feato(FEAT_ALIGNED_ATTACK_LAW, "aligned attack (lawful)", TRUE, TRUE, FALSE, FEAT_TYPE_COMBAT,
        "Attacks against lawful targets receive a +1 bonus to hit and +2 bonus to damage.",
        "Attacks against lawful targets receive a +1 bonus to hit and +2 bonus to damage."
        "This bonus will not stack with other aligned attack feats.  For example, if you "
        "have both aligned attack good and aligned attack lawful, you cannot gain double "
        "the bonus against a lawful good target.");
  feat_prereq_bab(FEAT_ALIGNED_ATTACK_LAW, 6);

  feato(FEAT_CRITICAL_FOCUS, "critical focus", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "When psionic focus is active, your critical hits land more often and for more damage.",
        "When psionic focus is active, your critical threat range increases by one and critical "
        "hit damage increases by +2.");
  feat_prereq_feat(FEAT_CRITICAL_FOCUS, FEAT_PSIONIC_FOCUS, 1);
  feat_prereq_feat(FEAT_CRITICAL_FOCUS, FEAT_IMPROVED_CRITICAL, 1);
  feat_prereq_bab(FEAT_CRITICAL_FOCUS, 8);
  feat_prereq_class_level(FEAT_CRITICAL_FOCUS, CLASS_PSIONICIST, 1);

  feato(FEAT_ELEMENTAL_FOCUS_ACID, "elemental focus (acid)", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "While psionic focus is active, your acid damage energy manifestations receive +1 damage to die and +1 to save dc.",
        "While psionic focus is active, your acid damage energy manifestations receive +1 damage to die and +1 to save dc.");
  feat_prereq_feat(FEAT_ELEMENTAL_FOCUS_ACID, FEAT_PSIONIC_FOCUS, 1);
  feat_prereq_class_level(FEAT_ELEMENTAL_FOCUS_ACID, CLASS_PSIONICIST, 1);
  feato(FEAT_ELEMENTAL_FOCUS_FIRE, "elemental focus (fire)", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "While psionic focus is active, your fire damage energy manifestations receive +1 damage to die and +1 to save dc.",
        "While psionic focus is active, your fire damage energy manifestations receive +1 damage to die and +1 to save dc.");
  feat_prereq_feat(FEAT_ELEMENTAL_FOCUS_FIRE, FEAT_PSIONIC_FOCUS, 1);
  feat_prereq_class_level(FEAT_ELEMENTAL_FOCUS_FIRE, CLASS_PSIONICIST, 1);
  feato(FEAT_ELEMENTAL_FOCUS_COLD, "elemental focus (cold)", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "While psionic focus is active, your cold damage energy manifestations receive +1 damage to die and +1 to save dc.",
        "While psionic focus is active, your cold damage energy manifestations receive +1 damage to die and +1 to save dc.");
  feat_prereq_feat(FEAT_ELEMENTAL_FOCUS_COLD, FEAT_PSIONIC_FOCUS, 1);
  feat_prereq_class_level(FEAT_ELEMENTAL_FOCUS_COLD, CLASS_PSIONICIST, 1);
  feato(FEAT_ELEMENTAL_FOCUS_ELECTRICITY, "elemental focus (electric)", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "While psionic focus is active, your electric damage energy manifestations receive +1 damage to die and +1 to save dc.",
        "While psionic focus is active, your electric damage energy manifestations receive +1 damage to die and +1 to save dc.");
  feat_prereq_feat(FEAT_ELEMENTAL_FOCUS_ELECTRICITY, FEAT_PSIONIC_FOCUS, 1);
  feat_prereq_class_level(FEAT_ELEMENTAL_FOCUS_ELECTRICITY, CLASS_PSIONICIST, 1);
  feato(FEAT_ELEMENTAL_FOCUS_SOUND, "elemental focus (sonic)", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "While psionic focus is active, your sonic damage energy manifestations receive +1 damage to die and +1 to save dc.",
        "While psionic focus is active, your sonic damage energy manifestations receive +1 damage to die and +1 to save dc.");
  feat_prereq_feat(FEAT_ELEMENTAL_FOCUS_SOUND, FEAT_PSIONIC_FOCUS, 1);
  feat_prereq_class_level(FEAT_ELEMENTAL_FOCUS_SOUND, CLASS_PSIONICIST, 1);

  feato(FEAT_POWER_PENETRATION, "power penetration", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "+2 bonus on manifester level checks to defeat power resistance",
        "+2 bonus on manifester level checks to defeat power resistance");
  feato(FEAT_GREATER_POWER_PENETRATION, "greater power penetration", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "+2 to manifester level checks to defeat power resistance",
        "+2 to manifester level checks to defeat power resistance");
  feat_prereq_feat(FEAT_GREATER_POWER_PENETRATION, FEAT_POWER_PENETRATION, 1);
  feato(FEAT_EPIC_POWER_PENETRATION, "greater power penetration", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "+2 to manifester level checks to defeat power resistance",
        "+2 to manifester level checks to defeat power resistance");
  feat_prereq_feat(FEAT_EPIC_POWER_PENETRATION, FEAT_GREATER_POWER_PENETRATION, 1);

  feato(FEAT_BREACH_POWER_RESISTANCE, "breach power resistance", TRUE, FALSE, FALSE, FEAT_TYPE_PSIONIC,
        "While psionic focus is active, gain your intelligence bonus on attempts to overcome power resistance.",
        "While psionic focus is active, gain your intelligence bonus on attempts to overcome power resistance.");

  feato(FEAT_DOUBLE_MANIFEST, "double manifest", TRUE, FALSE, FALSE, FEAT_TYPE_PSIONIC,
        "When activated (with the doublemanifest command) the next manifestation you perform will be executed twice.",
        "When activated (with the doublemanifest command) the next manifestation you perform will be executed twice.");
  feato(FEAT_PERPETUAL_FORESIGHT, "perpetual foresight", TRUE, FALSE, FALSE, FEAT_TYPE_PSIONIC,
        "While psionic focus is active, most d20 rolls have a 10 percent chance to add your intelligence bonus to the roll.",
        "While psionic focus is active, most d20 rolls have a 10 percent chance to add your intelligence bonus to the roll.");
  feato(FEAT_PSIONIC_FOCUS, "psionic focus", TRUE, FALSE, FALSE, FEAT_TYPE_PSIONIC,
        "When activated (with the psionicfocus command) power manifestation time is reduced, damage is increased by 10% and dcs are increased by +2.",
        "When activated (with the psionicfocus command) power manifestation time is reduced, "
        "damage is increased by 10% and dcs are increased by +1. It can also activate benefits "
        "from other feats that depend on psionic focus being active.");

  feato(FEAT_QUICK_MIND, "quick mind", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "Reduces power manifestation time.",
        "Reduces power manifestation time.");
  feat_prereq_class_level(FEAT_QUICK_MIND, CLASS_PSIONICIST, 1);

  feato(FEAT_PSIONIC_RECOVERY, "psionic recovery", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "Regain power points (psp) at a faster rate.",
        "Regain power points (psp) at a faster rate.");
  feat_prereq_class_level(FEAT_PSIONIC_RECOVERY, CLASS_PSIONICIST, 1);

  feato(FEAT_PROFICIENT_PSIONICIST, "proficient psionicist", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "Gain an extra power point (psp) per psionicist level.",
        "Gain an extra power point (psp) per psionicist level.");
  feat_prereq_class_level(FEAT_PROFICIENT_PSIONICIST, CLASS_PSIONICIST, 1);

  feato(FEAT_ENHANCED_POWER_DAMAGE, "enhanced power damage", TRUE, TRUE, TRUE, FEAT_TYPE_PSIONIC,
        "+1 power damage per die rolled",
        "You gain +1 power damage per die rolled, example:  if you are level 10 and "
        "normally create a 10d6 damage energy burst, with this feat your energy burst would "
        "do 10d6+10. Maximum of 3 ranks, rank 1-any psionic level, rank 2, psionic level 5+, rank 3, psionic level 10+");

  feato(FEAT_EMPOWERED_PSIONICS, "empowered magic", FALSE, TRUE, TRUE, FEAT_TYPE_PSIONIC,
        "+1 to all power dcs",
        "+1 to all power dcs. . Maximum of 3 ranks, rank 1-any psionic level, rank 2, psionic level 5+, rank 3, psionic level 10+");

  feato(FEAT_EXPANDED_KNOWLEDGE, "expanded knowledge", TRUE, TRUE, TRUE, FEAT_TYPE_PSIONIC,
        "Each rank gives you an extra power you can learn.",
        "Each rank gives you an extra power you can learn. Maximum 5 ranks.");
  feat_prereq_class_level(FEAT_EXPANDED_KNOWLEDGE, CLASS_PSIONICIST, 1);

  feato(FEAT_PSIONIC_ENDOWMENT, "psionic endowment", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "When psionic focus is active, save dcs for manifested powers are at +3.",
        "When psionic focus is active, save dcs for manifested powers are at +3.");
  feat_prereq_class_level(FEAT_PSIONIC_ENDOWMENT, CLASS_PSIONICIST, 1);

  feato(FEAT_PROFICIENT_AUGMENTING, "proficient augmenting", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "The manifesting time increase from augmenting a psionic power is reduced by 1.",
        "The manifesting time increase from augmenting a psionic power is reduced by 1. "
        "While psionic focus is active, your maximum augmenting limit is increased by 1."
        "This feat does not affect the 'augment' crafting command.");
  feat_prereq_class_level(FEAT_PROFICIENT_AUGMENTING, CLASS_PSIONICIST, 1);
  feato(FEAT_EXPERT_AUGMENTING, "expert augmenting", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "The manifesting time increase from augmenting a psionic power is reduced by 2.",
        "The manifesting time increase from augmenting a psionic power is reduced by 2. "
        "While psionic focus is active, your maximum augmenting limit is increased by 2."
        "This feat does not affect the 'augment' crafting command.");
  feat_prereq_class_level(FEAT_EXPERT_AUGMENTING, CLASS_PSIONICIST, 10);
  feat_prereq_feat(FEAT_EXPERT_AUGMENTING, FEAT_PROFICIENT_AUGMENTING, 1);
  feato(FEAT_MASTER_AUGMENTING, "master augmenting", TRUE, TRUE, FALSE, FEAT_TYPE_PSIONIC,
        "The manifesting time increase from augmenting a psionic power is reduced by 3.",
        "The manifesting time increase from augmenting a psionic power is reduced by 3. "
        "While psionic focus is active, your maximum augmenting limit is increased by 3."
        "This feat does not affect the 'augment' crafting command.");
  feat_prereq_class_level(FEAT_MASTER_AUGMENTING, CLASS_PSIONICIST, 20);
  feat_prereq_feat(FEAT_MASTER_AUGMENTING, FEAT_EXPERT_AUGMENTING, 1);

  /* weapon master */
  /*lvl 1*/ feato(FEAT_WEAPON_OF_CHOICE, "weapons of choice", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                  "All weapons with weapon focus gain special abilities",
                  "All weapons with weapon focus gain special abilities");
  /*lvl 2*/ feato(FEAT_SUPERIOR_WEAPON_FOCUS, "superior weapon focus", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                  "Weapons of choice have +1 to hit",
                  "Weapons of choice have +1 to hit");
  /*lvl 4*/ feato(FEAT_CRITICAL_SPECIALIST, "critical specialist", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
                  "Weapons of choice have +1 to threat range per rank",
                  "Weapons of choice have +1 to threat range per rank");
  /*lvl 6*/ feato(FEAT_UNSTOPPABLE_STRIKE, "unstoppable strike", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
                  "Weapons of Choice have 5 percent chance to deal max damage",
                  "Weapons of Choice have 5 percent chance to deal max damage per rank");
  /*lvl 8 - 2nd rank of critical specialist */
  /*lvl 10*/ feato(FEAT_INCREASED_MULTIPLIER, "increased multiplier", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                   "Weapons of choice have +1 to their critical multiplier",
                   "Weapons of choice have +1 to their critical multiplier");

  /* arcane archer */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /*lvl 1*/ feato(FEAT_ENHANCE_ARROW_MAGIC, "enhance arrow", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
                  "gain +1 enhancement bonus to arrows",
                  "All arrows fired from the arcane archer will gain a +1 enhancement bonus to"
                  "both attack bonus and damage.  This feat stacks.");
  /*lvl 2*/ feato(FEAT_SEEKER_ARROW, "seeker arrow", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
                  "free action shot that always hits",
                  "As a free action once per day per rank of the seeker arrow feat, the arcane "
                  "archer can fire an arrow that gains +20 to hit.  This feat stacks, useable "
                  "twice a day per feat.  Usage is: seekerarrow <target>");
  /*lvl 3 enhance arrow*/
  /*lvl 4*/ feato(FEAT_IMBUE_ARROW, "imbue arrow", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
                  "imbue an arrow with one of your spells",
                  "You can transfer your magical energy into one of your arrows.  This transfer "
                  "of power is temporary (approximately 8 game hours).  Upon the launching of "
                  "the arrow the magical energy from the arrow will be expended upon the "
                  "target.  This feat stacks, useable twice a day per feat.  Usage: imbuearrow "
                  "<arrow name> <spell name>");
  /*lvl 5 enhance, lvl 6 seeker, lvl 6 imbue, lvl 7 enhance, lvl 8 seeker*/
  /*lvl 8*/ feato(FEAT_SWARM_OF_ARROWS, "swarm of arrows", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                  "fire a barrage of arrows",
                  "As a standard action, once per day, you can fire an arrow at full attack bonus "
                  "at each and every target in the area.  You will need at least a single "
                  "projectile for each target.  Usage: arrowswarm");
  /*lvl 9 enhance*/
  /*lvl 10*/ feato(FEAT_ARROW_OF_DEATH, "arrow of death", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                   "one shot one kill",
                   "You will do your intelligence or charisma (whichever is higher) bonus as bonus "
                   "damage to your next arrow shot, "
                   "in addition, to opponents that are your level or lower, they have to make "
                   "a fortitude save vs DC: your arcane level + your intelligence or charisma "
                   "(whichver is higher) bonus + 10 in order "
                   "to survive your arrow of death.  Usage: deatharrow");

  /* arcane shadow */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /*lvl 1*/
  feato(FEAT_IMPROMPTU_SNEAK_ATTACK, "impromptu sneak attack", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "can do sneak attack at calling",
        "For each point in this feat, can once per day perform a sneak attack at will.  "
        "Will also get a sneak attack for any offhand weapon.");
  /*lvl 2*/
  /*sneak attack*/
  /*lvl 3*/
  /*impromptu sneak attack*/
  /*lvl 4*/
  /*sneak attack*/
  /*lvl 5*/
  /*impromptu sneak attack*/
  /*lvl 6*/
  /*sneak attack*/
  /*lvl 7*/
  feato(FEAT_INVISIBLE_ROGUE, "invisible rogue", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "1x spell like ability on self: greater invisibility",
        "Once per day, with this feat, one can become invisible exactly like the spell 'greater invisibility.'");
  /*lvl 8*/
  /*sneak attack*/
  /*lvl 9*/
  feato(FEAT_MAGICAL_AMBUSH, "magical ambush", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "casting doesn't break stealth",
        "As a mystical power, casting no longer breaks stealth.");
  /*lvl 10*/
  /*sneak attack*/
  feato(FEAT_SURPRISE_SPELLS, "surprise spells", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "add sneak attack damage to spells against unaware targets",
        "On conditions that would normally qualify one for a sneak attack, any damaging "
        "spells will now add sneak attack damage.");

  /* sacred fist */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_SACRED_FLAMES, "sacred flames", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "bonus unarmed sacred damage of class level + wis mod for 1 minute",
        "A sacred fist may invoke sacred flames around his hands and feet.  These flames add to the "
        "sacred fist's unarmed damage.  The additional damage is equal to the sacred fist's class levels "
        "plus his wisdom modifier (if any).  The sacred flame lasts for 1 minute.");
  feato(FEAT_INNER_FLAME, "inner flame", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "+4 bonus to AC, Saves, +25 spell resist for limited time",
        "A sacred fist may use inner armour once per day. This provides +4 "
        "sacred bonus to AC, +4 sacred bonus to all saves, 25 spell resistance for a number "
        "of rounds equal to his wisdom modifier plus class level.");

  /* stalwart defender */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /*1*/ feato(FEAT_AC_BONUS, "AC bonus", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
              "+1 dodge AC bonus",
              "A dodge bonus to AC of +1 - this feat stacks.");
  /*1*/ feato(FEAT_DEFENSIVE_STANCE, "defensive stance", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
              "a position of readiness and trance-like determination",
              "At 1st level, a stalwart defender can enter a defensive stance, a position of readiness and trance-like determination."
              "  A stalwart defender can maintain this stance for a number of rounds per day equal to 6 + his Constitution modifier."
              "  Also he can maintain the stance for 2 additional rounds per day * their class level.  The stalwart defender can enter "
              "and end a defensive stance as a free action.  While in a defensive stance, the stalwart defender gains a +2 dodge bonus to AC, "
              "a +4 morale bonus to his Strength and Constitution, as well as a +2 morale bonus on Will saves. The increase to "
              "Constitution grants the stalwart defender 2 hit points per Hit Die.  While in a defensive stance, a stalwart defender "
              "cannot move from his current position through any means.  After ending the stance, he is fatigued for 10 rounds "
              "A stalwart defender cannot enter a new defensive stance while fatigued or exhausted but can otherwise enter a stance multiple "
              "times during a single encounter or combat.  A defensive stance requires a level of emotional calm, and it may not be maintained "
              "by a character in a rage (such as from the rage class feature).  This feat stacks indicating additional daily usages.");
  /*2*/ feato(FEAT_FEARLESS_DEFENSE, "fearless defense", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "immune to fear effects while in defensive stance",
              "While in a defensive stance, the stalwart defender is immune to fear and fear-like effects.");
  /*3*/ /* uncanny dodge */
  /*4*/ /* AC bonus */
  /*4*/ feato(FEAT_IMMOBILE_DEFENSE, "immobile defense", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "bonus to CMD while in defensive stance",
              "While in a defensive stance, the stalwart defender adds his class-level / 2 to his Combat Maneuver Defense.");
  /*5*/ /* damage reduction 1 */
  /*6*/ feato(FEAT_DR_DEFENSE, "damage reduction defense", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "DR 1/- while in defensive stance",
              "While in a defensive stance, you are able to shrug off 1/- damage via damage reduction.");
  /*7*/ /* AC bonus */ /* damage reduction 3 */ /* improved uncanny dodge */
  /*8*/ feato(FEAT_RENEWED_DEFENSE, "renewed defense", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "can heal self while in defensive stance",
              "As a swift action, the stalwart defender heals 1d8 points of damage + his "
              "Constitution modifier + 10. For every two levels the stalwart defender "
              "has attained above 2nd, this healing increases by 1d8.  This power "
              "can be used only once per day and only while in a defensive stance.");
  /*9*/ feato(FEAT_MOBILE_DEFENSE, "mobile defense", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "Allows one to move while in defensive stance",
              "A stalwart defender can adjust his position while maintaining a defensive "
              "stance without losing the benefit of the stance.");
  /*10*/ /* AC bonus */ /* damage reduction 5 */
  /*10*/ feato(FEAT_SMASH_DEFENSE, "smash defense", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
               "free knockdown attack while in defensive stance",
               "While in defensive stance, once per round (every 6 seconds), the Stalwart "
               "Defender gets a free knock-down attempt against the opponent he is "
               "directly engaged with. This mode needs to be turned on first to use it "
               "by typing 'smashdefense'.");
  /*10*/ feato(FEAT_LAST_WORD, "last word", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
               "two extra attacks before suffering death blow in defensive stance",
               "Once per day, while in a defensive stance, a stalwart defender can make two "
               "melee attacks against an opponent within reach in response to an attack that would "
               "reduce him to negative hit points, knock him unconscious, or kill him. For example, "
               "a stalwart defender has 1 hit point left when a red dragon bites him; the defender may "
               "use this ability even if the dragon’s bite would otherwise kill him instantly. Once the "
               "defender’s attacks are resolved, he suffers the normal effect of the attack that provoked "
               "this ability.");

  /* Shadow Dancer */
  feato(FEAT_SHADOW_ILLUSION, "shadow illusion", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Grants the ability to cast the mirror image spell with the shadowcast command.",
        "Grants the ability to cast the mirror image spell with the shadowcast command.");
  feato(FEAT_SUMMON_SHADOW, "summon shadow", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Allows the shadowdancer to call a shadow to assist them in combat, using the call command.",
        "Allows the shadowdancer to call a shadow to assist them in combat, using the call command.");
  feato(FEAT_SHADOW_CALL, "shadow call", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Grants the ability to cast any wizard spell from the conjuration school or circle 3 or lower.  "
        "At shadow dancer level 10, the spell circle can be 6 or below. Uses the shadowcast command.",
        "Grants the ability to cast any wizard spell from the conjuration school or circle 3 or lower.  "
        "At shadow dancer level 10, the spell circle can be 6 or below. Uses the shadowcast command.");
  feato(FEAT_SHADOW_JUMP, "shadow jump", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Grants the ability to cast the shadow jump spell with the shadowcast command. This spell "
        "functions as the teleport spell, but only if both source room and target room are either "
        "outside at night, or inside without any magical light.",
        "Grants the ability to cast the shadow jump spell with the shadowcast command. This spell "
        "functions as the teleport spell, but only if both source room and target room are either "
        "outside at night, or inside without any magical light.");
  feato(FEAT_SHADOW_POWER, "shadow power", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Grants the ability to cast any wizard spell from the evocation school or circle 4 or lower.  "
        "At shadow dancer level 10, the spell circle can be 7 or below. Uses the shadowcast command.",
        "Grants the ability to cast any wizard spell from the evocation school or circle 4 or lower.  "
        "At shadow dancer level 10, the spell circle can be 7 or below. Uses the shadowcast command.");
  feato(FEAT_SHADOW_MASTER, "shadow master", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Whenever inside or outside at night, the shadow dancer has DR 5/- and a +2 luck bonus on "
        "all saving throws.  In addition any critical hit within these conditions will "
        "blind the foe for 1d6 rounds.",
        "Whenever inside or outside at night, the shadow dancer has DR 5/- and a +2 luck bonus on "
        "all saving throws.  In addition any critical hit within these conditions will "
        "blind the foe for 1d6 rounds.");
  feato(FEAT_WEAPON_PROFICIENCY_SHADOWDANCER, "weapon proficiency - shadowdancer", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Grants the character the ability to use shadowdancer weapons without penalty.  "
        "Use the weaponprof command for the list of specific weapons.",
        "Grants the character the ability to use shadowdancer weapons without penalty.  "
        "Use the weaponprof command for the list of specific weapons.");

  /* Shifter */
  /*1*/ feato(FEAT_LIMITLESS_SHAPES, "limitless shapes", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "no limit to number of times a shifter changes shapes",
              "no limit to number of times a shifter changes shapes");
  /*2*/ feato(FEAT_SHIFTER_SHAPES_1, "shifter shapes i", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "magical beast shape",
              "You can shift into the form of a manticore.");
  /*4*/ feato(FEAT_SHIFTER_SHAPES_2, "shifter shapes ii", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "fey shape",
              "You can shift into the form of a pixie.");
  /*6*/ feato(FEAT_SHIFTER_SHAPES_3, "shifter shapes iii", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "construct shape",
              "You can shift into the form of an iron golem.");
  /*8*/ feato(FEAT_SHIFTER_SHAPES_4, "shifter shapes iv", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "outsider shape",
              "You can shift into the form of an efreeti.");
  /*10*/ feato(FEAT_SHIFTER_SHAPES_5, "shifter shapes v", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
               "dragon shape",
               "You can shift into the form of a dragon.");
  feato(FEAT_IRON_GOLEM_IMMUNITY, "iron golem immunity", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "immunity to all magic, except fire and lightning.",
        "immunity to all magic, except fire and lightning. Fire will heal the golem instead.  Electric damage will slow the golem instead.");
  feato(FEAT_POISON_BREATH, "poison breath", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "allows breathing poison gas into the room",
        "dragon shape");
  feato(FEAT_TAIL_SPIKES, "tail spikes", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "can shoot spikes from your tail to each enemy in the room.",
        "can shoot spikes from your tail to each enemy in the room.  1d6+5 damage each, uses a swift action.");
  feato(FEAT_PIXIE_DUST, "pixie dust", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Can use pixie magic",
        "Can use pixie magic.  Command is pixiedust.");
  feato(FEAT_PIXIE_INVISIBILITY, "pixie invisibility", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Can use greater invisibility at will.",
        "Can use greater invisiblity at will.  Command is pixieinvis.");
  feato(FEAT_EFREETI_MAGIC, "efreeti magic", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Can use efreeti magic",
        "Can use efreeti magic.  Command is efreetimagic.");
  feato(FEAT_DRAGON_MAGIC, "dragon magic", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Can use dragon magic",
        "Can use dragon magic.  Command is dragonmagic.");
  feato(FEAT_EPIC_WILDSHAPE, "epic wildshape", TRUE, TRUE, TRUE, FEAT_TYPE_GENERAL,
        "Each rank gives +1 to strength, constitution, dexterity and natural armor class when wildshaped.",
        "Each rank gives +1 to strength, constitution, dexterity and natural armor class when wildshaped.  Max 5 ranks.");

  /* Assassin */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /*1*/

  /* Shadow Dancer (ShadowDancer) */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /*1*/

  /* Duelist */
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  /*1*/ feato(FEAT_CANNY_DEFENSE, "canny defense", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "add int bonus (max class level) to ac when using light or no armor and no shield",
              "add int bonus (max class level) to ac when using light or no armor and no shield");
  /*1*/ feato(FEAT_PRECISE_STRIKE, "precise strike", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "add duelist level to damage when using light or no armor and no shield",
              "A duelist gains the ability to strike precisely when light or not armored and no shield, "
              "adding her duelist level to her damage roll. When making a precise "
              "strike, a duelist cannot attack with a weapon in her other hand or use a "
              "shield. A duelist's precise strike only works against living creatures with "
              "discernible anatomies. Any creature that is immune to critical hits is also "
              "immune to a precise strike, and any item or ability that protects a creature "
              "from critical hits also protects a creature from a precise strike.");
  /*2*/ feato(FEAT_IMPROVED_REACTION, "improved reaction", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
              "+2 to initiative, stacks with improved initiative",
              "+2 to initiative, stacks with improved initiative");
  /*2*/ feato(FEAT_PARRY, "parry", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "+4 bonus to total defense",
              "+4 bonus to total defense");
  /*3*/ feato(FEAT_ENHANCED_MOBILITY, "enhanced mobility", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "+4 dodge bonus vs AOO",
              "gain an additional +4 dodge bonus to AC against "
              "attacks of opportunity provoked by movement. This bonus stacks with "
              "that granted by the Mobility feat.");
  /*4*/ /* combat reflexes */
  /*4*/ feato(FEAT_GRACE, "grace", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "+2 reflex saves",
              "gain a +2 bonus on Reflex saves");
  /*5*/ feato(FEAT_RIPOSTE, "riposte", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "+2 bonus to riposte attempts (totaldefense)",
              "+2 bonus to riposte attempts (totaldefense)");
  /*6*/ feato(FEAT_ACROBATIC_CHARGE, "dextrous charge", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "+4 damage when charging on foot",
              "+4 damage when charging on foot");
  /*7*/ feato(FEAT_ELABORATE_PARRY, "elaborate parry", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "when fighting total defense, gains +1 deflection ac per two class levels",
              "when fighting total defense, gains +1 deflection ac per two class levels");
  /*8*/ /* improved reaction */
  /*9*/ /* deflect arrows */
  /*9*/ feato(FEAT_NO_RETREAT, "no retreat", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
              "allows you to gain an AoO against retreating opponents",
              "allows you to gain an attack of opportunity against retreating opponents");
  /*10*/ feato(FEAT_CRIPPLING_CRITICAL, "crippling critical", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
               "allows your criticals to have progressive effects",
               "Your criticals will stack progressive negative effects on your opponent: 1d4"
               " strength damage, 1d4 dexterity damage, -4 penalty to each save, "
               "-4 penalty to AC, then finally doing 2d4 bleed and movement damage.");

  /* Blackguard */ /* knight of the skull (dragonlance) */
  feato(FEAT_SMITE_GOOD, "smite good", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "add charisma bonus to hit roll and blackguard level to damage",
        "add charisma bonus to hit roll and blackguard level to damage against good aligned targets. "
        "against good outsiders, good dragons, or any creature with cleric or paladin levels, "
        "2x blackguard level is added to damage instead.");

  /* Pale/Death Master */
  feato(FEAT_ANIMATE_DEAD, "animate dead", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "allows innate use of animate dead spell",
        "Allows innate use of animate dead spell once per day.  You get one use per ");

  feato(FEAT_CONCOCT_LVL_1, "1st circle alchemical concoctions", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "alchemist 1st circle slot",
        "This gives you the ability to concoct another extract of this slot for the respective "
        "class.  There may be other requirements for concocting extracts from this "
        "slot, some classes need the extract to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CONCOCT_LVL_2, "2nd circle alchemical concoctions", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "alchemist 2nd circle slot",
        "This gives you the ability to concoct another extract of this slot for the respective "
        "class.  There may be other requirements for concocting extracts from this "
        "slot, some classes need the extract to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CONCOCT_LVL_3, "3rd circle alchemical concoctions", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "alchemist 3rd circle slot",
        "This gives you the ability to concoct another extract of this slot for the respective "
        "class.  There may be other requirements for concocting extracts from this "
        "slot, some classes need the extract to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CONCOCT_LVL_4, "4th circle alchemical concoctions", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "alchemist 4th circle slot",
        "This gives you the ability to concoct another extract of this slot for the respective "
        "class.  There may be other requirements for concocting extracts from this "
        "slot, some classes need the extract to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CONCOCT_LVL_5, "5th circle alchemical concoctions", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "alchemist 5th circle slot",
        "This gives you the ability to concoct another extract of this slot for the respective "
        "class.  There may be other requirements for concocting extracts from this "
        "slot, some classes need the extract to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");
  feato(FEAT_CONCOCT_LVL_6, "6th circle alchemical concoctions", TRUE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING,
        "alchemist 6th circle slot",
        "This gives you the ability to concoct another extract of this slot for the respective "
        "class.  There may be other requirements for concocting extracts from this "
        "slot, some classes need the extract to be 'known' or 'scribed' for example.  Once "
        "the slot is used, you can 'prepare' to recover it.");

  feato(FEAT_MUTAGEN, "mutagen", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "allows the alchemist to create and imbibe mutagens",
        "Alchemists have the ability to create and imbibe mutagens that increase their "
        "physical stats and natural armor class at the expense of some mental ability. "
        "See HELP ALCHEMIST-MUTAGENS for more information.");
  feato(FEAT_PSYCHOKINETIC, "psychokinetic tincture", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Allows the alchemist to create a barrier of swirling spirits",
        "Alchemist can create a barrier of swirling spirits that give "
        "a bonus to deflection ac for each spirit.  Spirits can be launched "
        "at foes who will become frightened on a failed will save.  Alchemist "
        "can decide between frightened or shaken with the FRIGHTEN preference "
        "in prefedit.  See HELP FRIGHTENED and HELP SHAKEN.");
  feato(FEAT_BOMBS, "bombs", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "allows the alchemist to throw bombs that can damage single or multiple foes.",
        "Allows an alchemist to create and toss bombs at foes.  See HELP ALCHEMIST-BOMBS "
        "for more information. Also see HELP AOEBOMBS for information on how to target "
        "your bombs.");
  feato(FEAT_BOMB_MASTERY, "bomb mastery", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "All bombs have an increased effect.",
        "All bombs have an increased effect.");
  feato(FEAT_ALCHEMICAL_DISCOVERY, "alchemical discovery", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "allows the alchemist to select from a list of bomb, mutagen and other enhancements",
        "Allows an alchemist to select among a list of discoveries that will improve the "
        "effect of their bombs, mutagens or grant other abilities and/or benefits.");
  feato(FEAT_SWIFT_POISONING, "swift poisoning", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "can poison a weapon with a swift action",
        "Allows you to apply poison to a weapon without using up an action in combat.");
  feato(FEAT_SWIFT_ALCHEMY, "swift alchemy", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "can create alchemical items in half the normal time",
        "Allows the alchemist to create mundane alchemical items in half the normal time. "
        "Examples of such items are alchemist fire, tanglefoot bags, thunderstones, etc. "
        "This done not include bombs, extracts or mutagens.");
  feato(FEAT_PERSISTENT_MUTAGEN, "persistent mutagen", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Increases the duration of all mutagens to 1 hour / alchemist level.",
        "This enables alchemists to increase the duration of their mutagens to one hour "
        "per alchemist level, up from one minute per alchemist level.");
  feato(FEAT_POISON_IMMUNITY, "poison immunity", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "makes the alchemist completely immune to poisons and their effects.",
        "This ability makes the alchemist completely immune to poisons and their effects.");
  feato(FEAT_INSTANT_ALCHEMY, "instant alchemy", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "allows the alchemist to create mundane alchemical items in a single round",
        "This ability allows the alchemist to create mundane alchemical items, such as "
        "tanglefoot bags and thunderstones, in a single round, provided they have enough "
        "alchemical supplies on hand.");
  feato(FEAT_GRAND_ALCHEMICAL_DISCOVERY, "grand alchemical discovery", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "allows for the selection of a single grand discovery with the 'gdiscovery' command.",
        "Allows the alchemist to select a single grand discovery upon reaching level 20,"
        "with the 'gdiscovery' command.");

  /**************************/
  /* Disabled/Unimplemented */
  /**************************/

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */
  feato(FEAT_IMPROVED_OVERRUN, "improved overrun", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "ask staff", "ask staff");
  feato(FEAT_QUICK_DRAW, "quick draw", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "ask staff", "ask staff");
  feato(FEAT_SHOT_ON_THE_RUN, "shot on the run", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "ask staff", "ask staff");
  feato(FEAT_COMBAT_CHALLENGE, "combat challenge", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "allows you to make a mob focus their attention on you", "allows you to make a mob focus their attention on you");
  feato(FEAT_GREATER_COMBAT_CHALLENGE, "greater combat challenge", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "as improved combat challenge, but regular challenge is a minor action & challenge all is a move action", "as improved combat challenge, but regular challenge is a minor action & challenge all is a move action");
  feato(FEAT_IMPROVED_COMBAT_CHALLENGE, "improved combat challenge", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "allows you to make all mobs focus their attention on you", "allows you to make all mobs focus their attention on you");
  feato(FEAT_IMPROVED_NATURAL_WEAPON, "improved natural weapons", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "increase damage dice by one category for natural weapons", "increase damage dice by one category for natural weapons");
  feato(FEAT_IMPROVED_WEAPON_FINESSE, "improved weapon finesse", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "add dex bonus to damage instead of str for light weapons", "add dex bonus to damage instead of str for light weapons");
  feato(FEAT_KNOCKDOWN, "knockdown", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "when active, any melee attack that deals 10 damage or more invokes a free automatic trip attempt against your target", "when active, any melee attack that deals 10 damage or more invokes a free automatic trip attempt against your target");
  feato(FEAT_IMPROVED_BULL_RUSH, "improved bull rush", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_IMPROVED_SUNDER, "improved sunder", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_SUNDER, "sunder", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_MONKEY_GRIP, "monkey grip", FALSE, TRUE, TRUE, FEAT_TYPE_GENERAL, "can wield weapons one size larger than wielder in one hand with -2 to attacks.", "can wield weapons one size larger than wielder in one hand with -2 to attacks.");
  feato(FEAT_IMPROVED_INSTIGATION, "improved instigation", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_DIEHARD, "diehard", TRUE, TRUE, FALSE, FEAT_TYPE_GENERAL,
        "Gives you a 33% chance to avoid a killing blow.",
        "Gives you a 33% chance to avoid a killing blow.");
  feato(FEAT_STEADFAST_DETERMINATION, "steadfast determination", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "allows you to use your con bonus instead of your wis bonus for will saves", "allows you to use your con bonus instead of your wis bonus for will saves");

  feato(FEAT_IMPROVED_POWER_ATTACK, "improved power attack", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "unfinished", "unfinished");
  feato(FEAT_FINANCIAL_EXPERT, "financial expert", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "unfinished", "unfinished");
  feato(FEAT_THEORY_TO_PRACTICE, "theory to practice", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "unfinished", "unfinished");
  feato(FEAT_RUTHLESS_NEGOTIATOR, "ruthless negotiator", FALSE, FALSE, FALSE, FEAT_TYPE_COMBAT, "unfinished", "unfinished");

  feato(FEAT_PARALYSIS_IMMUNITY, "paralysis immunity", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "cannot be paralyzed.",
        "cannot be paralyzed");

  /* artisan */
  feato(FEAT_LEARNED_CRAFTER, "learned crafter", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Artisan gains exp for crafting items and harvesting", "Artisan gains exp for crafting items and harvesting");
  feato(FEAT_PROFICIENT_CRAFTER, "proficient crafter", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Increases all crafting skills", "Increases all crafting skills");
  feato(FEAT_PROFICIENT_HARVESTER, "proficient harvester", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Increases all harvesting skills", "Increases all harvesting skills");
  feato(FEAT_SCAVENGE, "scavenge", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "Can find materials on corpses", "Can find materials on corpses");
  feato(FEAT_BRANDING, "branding", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "All items made carry the artisan's brand", "All items made carry the artisan's brand");
  feato(FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR, "craft magical arms and armor", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can create magical weapons and armor ", "can create magical weapons and armor ");
  feato(FEAT_CRAFT_ROD, "craft rod", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can crate magical rods", "can crate magical rods");
  feato(FEAT_CRAFT_STAFF, "craft staff", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can create magical staves ", "can create magical staves ");
  feato(FEAT_CRAFT_WAND, "craft wand", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can create magical wands ", "can create magical wands ");
  feato(FEAT_CRAFT_WONDEROUS_ITEM, "craft wonderous item", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can crate miscellaneous magical items ", "can crate miscellaneous magical items ");
  feato(FEAT_FORGE_RING, "forge ring", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "ask staff ", "ask staff ");
  feato(FEAT_MASTERWORK_CRAFTING, "masterwork crafting", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "All equipment made is masterwork", "All equipment made is masterwork");

  /* epic */
  feato(FEAT_EPIC_COMBAT_CHALLENGE, "epic combat challenge", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "as improved combat challenge, but both regular challenges and challenge all are minor actions", "as improved combat challenge, but both regular challenges and challenge all are minor actions");
  feato(FEAT_EPIC_DODGE, "epic dodge", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "automatically dodge first attack against you each round", "automatically dodge first attack against you each round");

  /****/
  /* class feats */
  /*****/
  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* ranger */
  /* in this form, these ranger feats are not in the game, they are just given both sets of feats for free now */
  /* unfinished */ feato(FEAT_COMBAT_STYLE, "combat style", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                         "Either Rapid Shot or Two Weapon Fighting, depending on the chosen combat style.",
                         "Either Rapid Shot or Two Weapon Fighting, depending on the chosen combat style.");
  /* unfinished */ feato(FEAT_IMPROVED_COMBAT_STYLE, "improved combat style", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                         "Either Manyshot or Improved Two Weapon Fighting, depending on the chosen combat style.",
                         "Either Manyshot or Improved Two Weapon Fighting, depending on the chosen combat style.");
  /* unfinished */ feato(FEAT_COMBAT_STYLE_MASTERY, "combat style master", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
                         "Either Improved Precise Shot or Greater Two Weapon Fighting, depending on the chosen combat style.",
                         "Either Improved Precise Shot or Greater Two Weapon Fighting, depending on the chosen combat style.");

  /* Bard */
  feato(FEAT_EXTRA_MUSIC, "extra music", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "4 extra bard music uses per day", "4 extra bard music uses per day");

  /* paladin / cleric [shared] */
  feato(FEAT_DIVINE_MIGHT, "divine might", FALSE, TRUE, FALSE, FEAT_TYPE_DIVINE, "Add cha bonus to damage for number of rounds equal to cha bonus", "Add cha bonus to damage for number of rounds equal to cha bonus");
  feato(FEAT_DIVINE_SHIELD, "divine shield", FALSE, TRUE, FALSE, FEAT_TYPE_DIVINE, "Add cha bonus to armor class for number of rounds equal to cha bonus", "Add cha bonus to armor class for number of rounds equal to cha bonus");
  feato(FEAT_DIVINE_VENGEANCE, "divine vengeance", FALSE, TRUE, FALSE, FEAT_TYPE_DIVINE, "Add 2d6 damage against undead for number of rounds equal to cha bonus", "Add 2d6 damage against undead for number of rounds equal to cha bonus");
  feato(FEAT_EXCEPTIONAL_TURNING, "exceptional turning", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "+1d10 hit dice of undead turned", "+1d10 hit dice of undead turned");
  feato(FEAT_IMPROVED_TURNING, "improved turning", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");

  /* paladin / champion of torm [shared] */
  /* epic */
  feato(FEAT_GREAT_SMITING, "great smiting", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "For each rank in this feat you add your level in damage to all smite attacks", "For each rank in this feat you add your level in damage to all smite attacks");

  /* berserker */
  feato(FEAT_EXTEND_RAGE, "extend rage", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "Each of the uses of your rage or frenzy ability lasts an additional 5 rounds beyond its normal duration.", "Each of the uses of your rage or frenzy ability lasts an additional 5 rounds beyond its normal duration.");
  feato(FEAT_EXTRA_RAGE, "extra rage", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");

  /* fighter */
  feato(FEAT_WEAPON_FLURRY, "weapon flurry", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "2nd attack at -5 to hit with standard action or extra attack at full bonus with full round action", "2nd attack at -5 to hit with standard action or extra attack at full bonus with full round action");
  feato(FEAT_WEAPON_SUPREMACY, "weapon supremacy", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "+4 to resist disarm, ignore grapples, add +5 to hit roll when miss by 5 or less, can take 10 on attack rolls, +1 bonus to AC when wielding weapon", "+4 to resist disarm, ignore grapples, add +5 to hit roll when miss by 5 or less, can take 10 on attack rolls, +1 bonus to AC when wielding weapon");

  /* rogue (make talent or advanced talent?) */
  feato(FEAT_BLEEDING_ATTACK, "bleeding attack", FALSE, TRUE, FALSE, FEAT_TYPE_CLASS_ABILITY, "causes bleed damage on living targets who are hit by sneak attack.", "causes bleed damage on living targets who are hit by sneak attack.");
  feato(FEAT_OPPORTUNIST, "opportunist", FALSE, TRUE, FALSE, FEAT_TYPE_CLASS_ABILITY, "once per round the rogue may make an attack of opportunity against a foe an ally just struck", "once per round the rogue may make an attack of opportunity against a foe an ally just struck");
  feato(FEAT_IMPROVED_SNEAK_ATTACK, "improved sneak attack", FALSE, TRUE, TRUE, FEAT_TYPE_COMBAT, "each rank gives +5 percent chance per attack, per rank to be a sneak attack.", "each rank gives +5 percent chance per attack, per rank to be a sneak attack.");
  feato(FEAT_ROBILARS_GAMBIT, "robilars gambit", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "when active enemies gain +4 to hit and damage against you, but all melee attacks invoke an attack of opportunity from you.", "when active enemies gain +4 to hit and damage against you, but all melee attacks invoke an attack of opportunity from you.");
  feato(FEAT_POWERFUL_SNEAK, "powerful sneak", FALSE, TRUE, FALSE, FEAT_TYPE_GENERAL, "opt to take -2 to attacks and treat all sneak attack dice rolls of 1 as a 2", "opt to take -2 to attacks and treat all sneak attack dice rolls of 1 as a 2");
  /* epic */
  feato(FEAT_SNEAK_ATTACK_OF_OPPORTUNITY, "sneak attack of opportunity", FALSE, TRUE, FALSE, FEAT_TYPE_COMBAT, "makes all opportunity attacks sneak attacks", "makes all opportunity attacks sneak attacks");

  /* knight of the rose (dragonlance) */
  feato(FEAT_RALLYING_CRY, "rallying cry", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");

  /* knight of the sword */

  /* knight of the crown (dragonlance) */
  feato(FEAT_HONORABLE_WILL, "honorable will", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");

  /* knight of the crown / knight of the lily [SHARED] (dragonlance) */
  feato(FEAT_ARMORED_MOBILITY, "armored mobility", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "heavy armor is treated as medium armor", "heavy armor is treated as medium armor");

  /* knight of the lily (dragonlance) */
  feato(FEAT_UNBREAKABLE_WILL, "unbreakable will", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");

  /* knight of the thorn (dragonlance) */

  /* knight of the skull (dragonlance) */
  feato(FEAT_DARK_BLESSING, "dark blessing", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* Pale/Death Master */
  feato(FEAT_BONE_ARMOR, "bone armor", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows creation of bone armor and 10 percent arcane spell failure reduction in bone armor per rank.", "allows creation of bone armor and 10 percent arcane spell failure reduction in bone armor per rank.");
  feato(FEAT_ESSENCE_OF_UNDEATH, "essence of undeath", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "gives immunity to poison, disease, sneak attack and critical hits", "gives immunity to poison, disease, sneak attack and critical hits");
  feato(FEAT_SUMMON_GREATER_UNDEAD, "summon greater undead", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows innate use of summon greater undead spell 3x per day", "allows innate use of summon greater undead spell 3x per day");
  feato(FEAT_SUMMON_UNDEAD, "summon undead", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows innate use of summon undead spell 3x per day", "allows innate use of summon undead spell 3x per day");
  feato(FEAT_TOUCH_OF_UNDEATH, "touch of undeath", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows for paralytic or instant death touch", "allows for paralytic or instant death touch");
  feato(FEAT_UNDEAD_FAMILIAR, "undead familiar", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows for undead familiars", "allows for undead familiars");

  /* Assassin */
  feato(FEAT_DEATH_ATTACK, "death attack", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Allows assassin to mark an opponent and paralyze opponents when performing a backstab.",
        "Allows assassin to mark an opponent and paralyze opponents when performing a backstab. Also opens up effect of other assassin abilities. Uses the 'mark' command.");
  feato(FEAT_POISON_SAVE_BONUS, "poison save bonus", TRUE, FALSE, TRUE, FEAT_TYPE_CLASS_ABILITY,
        "Improves chance to resist or reduce effect of poisons and poison based damage.",
        "Improves chance to resist or reduce effect of poisons and poison based damage.");
  feato(FEAT_POISON_USE, "poison use", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Trained use in poisons without risk of failure or poisoning self. Also improves effect of weapon poisons.",
        "Trained use in poisons without risk of failure or poisoning self. Also increases number of hits and level by 50 percent for weapon applied poisons.");
  feato(FEAT_WEAPON_PROFICIENCY_ASSASSIN, "weapon proficiency - assassin", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Assassins can use any crossbow or short bow, daggers, rapiers, sap, darts and short swords.",
        "Assassins can use any crossbow or short bow, daggers, rapiers, sap, darts and short swords.");
  feato(FEAT_QUIET_DEATH, "quiet death", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Grants greater invisibility for 3 rounds when backstabbing a marked target.",
        "Grants greater invisibility for 3 rounds when backstabbing a marked target. Will not work if already invisible.");
  feato(FEAT_SWIFT_DEATH, "swift death", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Backstabs use a swift action.",
        "Backstabs use a swift action.");
  feato(FEAT_ANGEL_OF_DEATH, "angel of death", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Backstabs have a +3 to attack roll and +10 to damage.  Stacks with true death. Also removes requirement to wait 3 rounds to mark a target.",
        "Backstabs have a +3 to attack roll and +10 to damage.  Stacks with true death. Also removes requirement to wait 3 rounds to mark a target.");
  feato(FEAT_HIDDEN_WEAPONS, "hidden weapons", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Chance for a second, regular attack, when backstabbing.",
        "Chance for a second, regular attack, when backstabbing. Requires a sleight of hand vs. perception check.");
  feato(FEAT_TRUE_DEATH, "true death", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY,
        "Backstabs have a +2 to attack roll and +10 to damage. Stacks with angel of death.",
        "Backstabs have a +2 to attack roll and +10 to damage. Stacks with angel of death.");

  /* favored soul */
  feato(FEAT_DEITY_WEAPON_PROFICIENCY, "deity's weapon proficiency", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows you to use the weapon of your deity", "allows you to use the weapon of your deity");
  feato(FEAT_HASTE, "haste", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "can cast haste 3x per day", "can cast haste 3x per day");

  /* dragon disciple */
  feato(FEAT_DRAGON_APOTHEOSIS, "dragon apotheosis", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_ELEMENTAL_IMMUNITY, "elemental immunity", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "ask staff", "ask staff");
  feato(FEAT_BREATH_WEAPON, "breath weapon", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_CHARISMA_BOOST, "charisma boost", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_CLAWS_AND_BITE, "claws and bite", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_CONSTITUTION_BOOST, "constitution boost", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_INTELLIGENCE_BOOST, "intelligence boost", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_SLEEP_PARALYSIS_IMMUNITY, "sleep & paralysis immunity", TRUE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY,
        "cannot be put to sleep or paralyzed.",
        "cannot be put to sleep or paralyzed");
  feato(FEAT_STRENGTH_BOOST, "strength boost", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_TRAMPLE, "trample", FALSE, FALSE, FALSE, FEAT_TYPE_INNATE_ABILITY, "ask staff", "ask staff");
  feato(FEAT_NATURAL_ARMOR_INCREASE, "natural armor increase", FALSE, FALSE, FALSE, FEAT_TYPE_GENERAL, "ask staff", "ask staff");
  feato(FEAT_BLINDSENSE, "blindsense", TRUE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "draconic bloodline, sorcerer level 20",
        "Allows full vision even when there is no light or the character is blinded.");

  /* dragon rider */
  feato(FEAT_DRAGON_MOUNT_BOOST, "dragon mount boost", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "gives +18 hp, +1 ac, +1 hit and +1 damage per rank in the feat", "gives +18 hp, +10 ac, +1 hit and +1 damage per rank in the feat");
  feato(FEAT_DRAGON_MOUNT_BREATH, "dragon mount breath", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "allows you to use your dragon mount's breath weapon once per rank, per 10 minutes.", "allows you to use your dragon mount's breath weapon once per rank, per 10 minutes.");

  /* feat-number | name | in game? | learnable? | stackable? | feat-type | short-descrip | long descrip */

  /* arcane archer */
  feato(FEAT_ENHANCE_ARROW_ALIGNED, "enhance arrow (aligned)", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "+1d6 holy/unholy damage with bows against different aligned creatures.", "+1d6 holy/unholy damage with bows against different aligned creatures.");
  feato(FEAT_ENHANCE_ARROW_DISTANCE, "enhance arrow (distance)", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "doubles range increment on weapon.", "doubles range increment on weapon.");
  feato(FEAT_ENHANCE_ARROW_ELEMENTAL, "enhance arrow (elemental)", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "+1d6 elemental damage with bows", "+1d6 elemental damage with bows");
  feato(FEAT_ENHANCE_ARROW_ELEMENTAL_BURST, "enhance arrow (elemental burst)", FALSE, FALSE, FALSE, FEAT_TYPE_CLASS_ABILITY, "+2d10 on critical hits with bows", "+2d10 on critical hits with bows");

  /* wizard / sorc */
  /*craft*/
  feato(FEAT_BREW_POTION, "brew potion", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can create magical potions ", "can create magical potions ");
  feato(FEAT_SCRIBE_SCROLL, "scribe scroll", FALSE, FALSE, FALSE, FEAT_TYPE_CRAFT, "can scribe spells from memory onto scrolls", "can scribe spells from memory onto scrolls");
  /*metamagic*/
  feato(FEAT_ENLARGE_SPELL, "enlarge spell", FALSE, FALSE, FALSE, FEAT_TYPE_METAMAGIC, "ask staff ", "ask staff ");
  feato(FEAT_HEIGHTEN_SPELL, "heighten spell", FALSE, FALSE, FALSE, FEAT_TYPE_METAMAGIC, "ask staff ", "ask staff ");
  feato(FEAT_SILENT_SPELL, "silent spell", FALSE, FALSE, FALSE, FEAT_TYPE_METAMAGIC, "ask staff", "ask staff");
  feato(FEAT_STILL_SPELL, "still spell", FALSE, FALSE, FALSE, FEAT_TYPE_METAMAGIC, "ask staff", "ask staff");
  feato(FEAT_WIDEN_SPELL, "widen spell", FALSE, FALSE, FALSE, FEAT_TYPE_METAMAGIC, "ask staff", "ask staff");
  feato(FEAT_EMPOWER_SPELL, "empower spell", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "all variable numerical effects of a spell are increased by one half ", "all variable numerical effects of a spell are increased by one half ");
  feato(FEAT_EMPOWERED_MAGIC, "empowered magic", TRUE, TRUE, TRUE, FEAT_TYPE_METAMAGIC, "+1 to all spell dcs", "+1 to all spell dcs. . Maximum of 3 ranks, rank 1-any level, rank 2, level 5+, rank 3, level 10+");
  feato(FEAT_EXTEND_SPELL, "extend spell", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "durations of spells are 50 percent longer when enabled ", "durations of spells are 50 percent longer when enabled ");
  /*spellcasting*/
  feato(FEAT_ESCHEW_MATERIALS, "eschew materials", FALSE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING, "ask staff", "ask staff");
  feato(FEAT_IMPROVED_COUNTERSPELL, "improved counterspell", FALSE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING, "ask staff", "ask staff");
  feato(FEAT_SPELL_MASTERY, "spell mastery", FALSE, FALSE, FALSE, FEAT_TYPE_SPELLCASTING, "ask staff", "ask staff");
  /* epic */
  /*spellcasting*/
  feato(FEAT_EPIC_SPELLCASTING, "epic spellcasting", FALSE, TRUE, FALSE, FEAT_TYPE_SPELLCASTING, "allows you to cast epic spells", "allows you to cast epic spells");
  /*metamagic*/
  feato(FEAT_INTENSIFY_SPELL, "intensify spell", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "maximizes damage/healing and then doubles it.", "maximizes damage/healing and then doubles it.");
  feato(FEAT_ENHANCE_SPELL, "increase spell damage", FALSE, TRUE, FALSE, FEAT_TYPE_METAMAGIC, "increase max number of damage dice for certain damage based spell by 5", "increase max number of damage dice for certain damage based spell by 5");
  feato(FEAT_AUTOMATIC_QUICKEN_SPELL, "automatic quicken spell", FALSE, TRUE, TRUE, FEAT_TYPE_METAMAGIC, "You can cast level 0, 1, 2 & 3 spells automatically as if quickened.  Every addition rank increases the max spell level by 3.", "You can cast level 0, 1, 2 & 3 spells automatically as if quickened.  Every addition rank increases the max spell level by 3.");

  /* monk */

  /* End Class ability Feats */

  /* self explanatory */
  feato(FEAT_LAST_FEAT, "do not take me", FALSE, FALSE, FALSE, FEAT_TYPE_NONE, "placeholder feat", "placeholder feat");

  /****************************************************************************/
  /* some more assigning */

  /* Combat Feats */
  combatfeat(FEAT_IMPROVED_CRITICAL);
  combatfeat(FEAT_WEAPON_FINESSE);
  combatfeat(FEAT_WEAPON_FOCUS);
  combatfeat(FEAT_WEAPON_SPECIALIZATION);
  combatfeat(FEAT_GREATER_WEAPON_FOCUS);
  combatfeat(FEAT_GREATER_WEAPON_SPECIALIZATION);
  combatfeat(FEAT_IMPROVED_WEAPON_FINESSE);
  combatfeat(FEAT_MONKEY_GRIP);
  combatfeat(FEAT_POWER_CRITICAL);
  combatfeat(FEAT_WEAPON_FLURRY);
  combatfeat(FEAT_WEAPON_SUPREMACY);
  /*epic combat feat*/ combatfeat(FEAT_EPIC_WEAPON_SPECIALIZATION);

  /* Epic Feats */
  epicfeat(FEAT_BANE_OF_ENEMIES);
  epicfeat(FEAT_EPIC_WEAPON_SPECIALIZATION);
  epicfeat(FEAT_EPIC_PROWESS);
  epicfeat(FEAT_SWARM_OF_ARROWS);
  epicfeat(FEAT_SELF_CONCEALMENT);
  epicfeat(FEAT_INTENSIFY_SPELL);
  epicfeat(FEAT_EPIC_SPELLCASTING);
  epicfeat(FEAT_EPIC_SKILL_FOCUS);
  epicfeat(FEAT_EPIC_DODGE);
  epicfeat(FEAT_EPIC_COMBAT_CHALLENGE);
  epicfeat(FEAT_EPIC_TOUGHNESS);
  epicfeat(FEAT_GREAT_STRENGTH);
  epicfeat(FEAT_GREAT_CONSTITUTION);
  epicfeat(FEAT_GREAT_DEXTERITY);
  epicfeat(FEAT_GREAT_WISDOM);
  epicfeat(FEAT_GREAT_INTELLIGENCE);
  epicfeat(FEAT_GREAT_CHARISMA);
  epicfeat(FEAT_DAMAGE_REDUCTION);
  epicfeat(FEAT_ARMOR_SKIN);
  epicfeat(FEAT_FAST_HEALING);
  epicfeat(FEAT_ENHANCE_SPELL);
  epicfeat(FEAT_GREAT_SMITING);
  epicfeat(FEAT_PERFECT_TWO_WEAPON_FIGHTING);
  epicfeat(FEAT_SNEAK_ATTACK);
  epicfeat(FEAT_SNEAK_ATTACK_OF_OPPORTUNITY);
  epicfeat(FEAT_AUTOMATIC_QUICKEN_SPELL);
  epicfeat(FEAT_IMPROVED_SPELL_RESISTANCE);
  epicfeat(FEAT_BLINDING_SPEED);
  /* epic spell feats */
  epicfeat(FEAT_MUMMY_DUST);
  epicfeat(FEAT_DRAGON_KNIGHT);
  epicfeat(FEAT_GREATER_RUIN);
  epicfeat(FEAT_HELLBALL);
  epicfeat(FEAT_EPIC_MAGE_ARMOR);
  epicfeat(FEAT_EPIC_WARDING);
  epicfeat(FEAT_EPIC_WILDSHAPE);
  epicfeat(FEAT_EPIC_SPELL_FOCUS);
  epicfeat(FEAT_MASTER_AUGMENTING);

  epicfeat(FEAT_LAST_FEAT);

  /* Feats with "Daily Use" Mechanic, make sure to add to
   * EVENTFUNC(event_daily_use_cooldown) and mud_event.c */
  dailyfeat(FEAT_QUIVERING_PALM, eQUIVERINGPALM);
  dailyfeat(FEAT_ARROW_OF_DEATH, eDEATHARROW);
  dailyfeat(FEAT_STUNNING_FIST, eSTUNNINGFIST);
  dailyfeat(FEAT_ANIMATE_DEAD, eANIMATEDEAD);
  dailyfeat(FEAT_WILD_SHAPE, eWILD_SHAPE);
  dailyfeat(FEAT_CRYSTAL_BODY, eCRYSTALBODY);
  dailyfeat(FEAT_CRYSTAL_FIST, eCRYSTALFIST);
  dailyfeat(FEAT_SLA_STRENGTH, eSLA_STRENGTH);
  dailyfeat(FEAT_SLA_ENLARGE, eSLA_ENLARGE);
  dailyfeat(FEAT_SLA_INVIS, eSLA_INVIS);
  dailyfeat(FEAT_SLA_LEVITATE, eSLA_LEVITATE);
  dailyfeat(FEAT_SLA_DARKNESS, eSLA_DARKNESS);
  dailyfeat(FEAT_SLA_FAERIE_FIRE, eSLA_FAERIE_FIRE);
  dailyfeat(FEAT_LAYHANDS, eLAYONHANDS);
  dailyfeat(FEAT_LICH_TOUCH, eLICH_TOUCH);
  dailyfeat(FEAT_LICH_REJUV, eLICH_REJUV);
  dailyfeat(FEAT_LICH_FEAR, eLICH_FEAR);
  dailyfeat(FEAT_REMOVE_DISEASE, ePURIFY);
  dailyfeat(FEAT_RAGE, eRAGE);
  dailyfeat(FEAT_SACRED_FLAMES, eSACRED_FLAMES);
  dailyfeat(FEAT_INNER_FIRE, eINNER_FIRE);
  dailyfeat(FEAT_DEFENSIVE_STANCE, eDEFENSIVE_STANCE);
  dailyfeat(FEAT_VANISH, eVANISHED);
  dailyfeat(FEAT_INVISIBLE_ROGUE, eINVISIBLE_ROGUE);
  dailyfeat(FEAT_SMITE_EVIL, eSMITE_EVIL);
  dailyfeat(FEAT_IMBUE_ARROW, eIMBUE_ARROW);
  dailyfeat(FEAT_SEEKER_ARROW, eSEEKER_ARROW);
  dailyfeat(FEAT_IMPROMPTU_SNEAK_ATTACK, eIMPROMPT);
  dailyfeat(FEAT_SWARM_OF_ARROWS, eARROW_SWARM);
  dailyfeat(FEAT_SMITE_GOOD, eSMITE_GOOD);
  dailyfeat(FEAT_DESTRUCTIVE_SMITE, eSMITE_DESTRUCTION);
  dailyfeat(FEAT_TURN_UNDEAD, eTURN_UNDEAD);
  dailyfeat(FEAT_LIGHTNING_ARC, eLIGHTNING_ARC);
  dailyfeat(FEAT_ACID_DART, eACID_DART);
  dailyfeat(FEAT_FIRE_BOLT, eFIRE_BOLT);
  dailyfeat(FEAT_ICICLE, eICICLE);
  dailyfeat(FEAT_CURSE_TOUCH, eCURSE_TOUCH);
  dailyfeat(FEAT_DESTRUCTIVE_AURA, eDESTRUCTIVE_AURA);
  dailyfeat(FEAT_EVIL_TOUCH, eEVIL_TOUCH);
  dailyfeat(FEAT_GOOD_TOUCH, eGOOD_TOUCH);
  dailyfeat(FEAT_HEALING_TOUCH, eHEALING_TOUCH);
  dailyfeat(FEAT_CURING_TOUCH, eCURING_TOUCH);
  dailyfeat(FEAT_EYE_OF_KNOWLEDGE, eEYE_OF_KNOWLEDGE);
  dailyfeat(FEAT_BLESSED_TOUCH, eBLESSED_TOUCH);
  dailyfeat(FEAT_COPYCAT, eCOPYCAT);
  dailyfeat(FEAT_MASS_INVIS, eMASS_INVIS);
  dailyfeat(FEAT_AURA_OF_PROTECTION, eAURA_OF_PROTECTION);
  dailyfeat(FEAT_BATTLE_RAGE, eBATTLE_RAGE);
  dailyfeat(FEAT_DRACONIC_HERITAGE_BREATHWEAPON, eDRACBREATH);
  dailyfeat(FEAT_DRACONIC_HERITAGE_CLAWS, eDRACCLAWS);
  dailyfeat(FEAT_METAMAGIC_ADEPT, eARCANEADEPT);
  dailyfeat(FEAT_MUTAGEN, eMUTAGEN);
  dailyfeat(FEAT_PSYCHOKINETIC, ePSYCHOKINETIC);
  dailyfeat(FEAT_PIXIE_DUST, ePIXIEDUST);
  dailyfeat(FEAT_EFREETI_MAGIC, eEFREETIMAGIC);
  dailyfeat(FEAT_DRAGON_MAGIC, eDRAGONMAGIC);
  dailyfeat(FEAT_CHANNEL_SPELL, eCHANNELSPELL);
  dailyfeat(FEAT_PSIONIC_FOCUS, ePSIONICFOCUS);
  dailyfeat(FEAT_DOUBLE_MANIFEST, eDOUBLEMANIFEST);
  dailyfeat(FEAT_SHADOW_ILLUSION, eSHADOWILLUSION);
  dailyfeat(FEAT_SHADOW_CALL, eSHADOWCALL);
  dailyfeat(FEAT_SHADOW_JUMP, eSHADOWJUMP);
  dailyfeat(FEAT_SHADOW_POWER, eSHADOWPOWER);
  dailyfeat(FEAT_TOUCH_OF_CORRUPTION, eTOUCHOFCORRUPTION);
  dailyfeat(FEAT_CHANNEL_ENERGY, eCHANNELENERGY);
  /** END **/
}

/* Check to see if ch meets the provided feat prerequisite.
   w_type is the type of the wielded weapon, if any */
bool meets_prerequisite(struct char_data *ch, struct feat_prerequisite *prereq, int w_type)
{
  switch (prereq->prerequisite_type)
  {
  case FEAT_PREREQ_NONE:
    /* This is a NON-prereq. */
    break;
  case FEAT_PREREQ_ATTRIBUTE:
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
      log("SYSERR: meets_prerequisite() - Bad Attribute prerequisite %d", prereq->values[0]);
      return FALSE;
    }
    break;
  case FEAT_PREREQ_CLASS_LEVEL:
    if (prereq->values[0] == CLASS_WARRIOR)
    {
      if (WARRIOR_LEVELS(ch) < prereq->values[1])
        return FALSE;
    }
    else
    {
      if (CLASS_LEVEL(ch, prereq->values[0]) < prereq->values[1])
        return FALSE;
    }
    break;
  case FEAT_PREREQ_FEAT:
    if (has_feat_requirement_check(ch, prereq->values[0]) < prereq->values[1])
      return FALSE;
    break;
  case FEAT_PREREQ_ABILITY:
    if (GET_ABILITY(ch, prereq->values[0]) < prereq->values[1])
      return FALSE;
    break;
  case FEAT_PREREQ_SPELLCASTING:
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
      /* If they need a certain circle, and they don't have any in any class, fail. */
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
        if (compute_slots_by_circle(ch, CLASS_WIZARD, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_SORCERER, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_BARD, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_CLERIC, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_PALADIN, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_DRUID, prereq->values[2]) == 0 &&
            compute_slots_by_circle(ch, CLASS_RANGER, prereq->values[2]) == 0)
          return FALSE;
      }
      break;
    default:
      log("SYSERR: meets_prerequisite() - Bad Casting Type prerequisite %d", prereq->values[0]);
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
      log("SYSERR: meets_prerequisite() - Bad Preparation type prerequisite %d", prereq->values[1]);
      return FALSE;
    }
    break;
  case FEAT_PREREQ_RACE:
    if (!IS_NPC(ch) && GET_RACE(ch) != prereq->values[0])
      return FALSE;
    break;
  case FEAT_PREREQ_BAB:
    if (BAB(ch) < prereq->values[0])
      return FALSE;
    break;
  case FEAT_PREREQ_CFEAT:
    /*  SPECIAL CASE - You must have a feat, and it must be the cfeat for the chosen weapon. */
    if (w_type && !has_combat_feat(ch, feat_to_cfeat(prereq->values[0]), w_type))
      return FALSE;
    break;
  case FEAT_PREREQ_WEAPON_PROFICIENCY:
    if (w_type && !is_proficient_with_weapon(ch, w_type))
      return FALSE;
    break;
  default:
    log("SYSERR: meets_prerequisite() - Bad prerequisite type %d", prereq->prerequisite_type);
    return FALSE;
  }

  return TRUE;
}

/* The follwing function is used to check if the character satisfies the various prerequisite(s) (if any)
   of a feat in order to learn it. */
int feat_is_available(struct char_data *ch, int featnum, int iarg, char *sarg)
{
  struct feat_prerequisite *prereq = NULL;

  if (featnum > NUM_FEATS) /* even valid featnum? */
    return FALSE;

  if (feat_list[featnum].epic == TRUE && !IS_EPIC(ch)) /* epic only feat */
    return FALSE;

  if (has_feat_requirement_check(ch, featnum) && !feat_list[featnum].can_stack) /* stackable? */
    return FALSE;

  if (feat_list[featnum].in_game == FALSE) /* feat in the game at all? */
    return FALSE;

  if (feat_list[featnum].prerequisite_list != NULL)
  {
    /*  This feat has prerequisites. Traverse the list and check. */
    for (prereq = feat_list[featnum].prerequisite_list; prereq != NULL; prereq = prereq->next)
    {
      if (meets_prerequisite(ch, prereq, iarg) == FALSE)
        return FALSE;
    }
  }
  else
  { /* no pre-requisites */
    switch (featnum)
    {
    case FEAT_AUTOMATIC_QUICKEN_SPELL:
      if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) < 30)
        return FALSE;
      if (compute_slots_by_circle(ch, CLASS_SORCERER, 9) > 0)
        return TRUE;
      if (compute_slots_by_circle(ch, CLASS_WIZARD, 9) > 0)
        return TRUE;
      if (compute_slots_by_circle(ch, CLASS_CLERIC, 9) > 0)
        return TRUE;
      if (compute_slots_by_circle(ch, CLASS_DRUID, 9) > 0)
        return TRUE;
      return FALSE;

    case FEAT_INTENSIFY_SPELL:
      if (!has_feat_requirement_check(ch, FEAT_MAXIMIZE_SPELL))
        return FALSE;
      if (!has_feat_requirement_check(ch, FEAT_EMPOWER_SPELL))
        return FALSE;
      if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) < 30)
        return FALSE;
      if (compute_slots_by_circle(ch, CLASS_SORCERER, 9) > 0)
        return TRUE;
      if (compute_slots_by_circle(ch, CLASS_WIZARD, 9) > 0)
        return TRUE;
      if (compute_slots_by_circle(ch, CLASS_CLERIC, 9) > 0)
        return TRUE;
      if (compute_slots_by_circle(ch, CLASS_DRUID, 9) > 0)
        return TRUE;
      return FALSE;

    case FEAT_SWARM_OF_ARROWS:
      if (ch->real_abils.dex < 23)
        return FALSE;
      if (!has_feat_requirement_check(ch, FEAT_POINT_BLANK_SHOT))
        return FALSE;
      if (!has_feat_requirement_check(ch, FEAT_RAPID_SHOT))
        return FALSE;
      if (has_feat_requirement_check(ch, FEAT_WEAPON_FOCUS)) /* Need to check for BOW... */
        return FALSE;
      return TRUE;

    case FEAT_FAST_HEALING:
      if (ch->real_abils.con < 21)
        return FALSE;
      return TRUE;

    case FEAT_SELF_CONCEALMENT:
      if (GET_ABILITY(ch, ABILITY_STEALTH) < 25)
        return FALSE;
      if (GET_ABILITY(ch, ABILITY_ACROBATICS) < 25)
        return FALSE;
      if (ch->real_abils.dex < 21)
        return FALSE;
      return TRUE;

    case FEAT_ARMOR_SKIN:
      return TRUE;

    case FEAT_ANIMATE_DEAD:
      return TRUE;

    case FEAT_COMBAT_CHALLENGE:
      if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 5 &&
          GET_ABILITY(ch, ABILITY_INTIMIDATE) < 5 &&
          GET_ABILITY(ch, ABILITY_BLUFF) < 5)
        return false;
      return true;

    case FEAT_BLEEDING_ATTACK:
    case FEAT_POWERFUL_SNEAK:
      if (CLASS_LEVEL(ch, CLASS_ROGUE) > 1)
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_COMBAT_CHALLENGE:
      if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 10 &&
          GET_ABILITY(ch, ABILITY_INTIMIDATE) < 10 &&
          GET_ABILITY(ch, ABILITY_BLUFF) < 10)
        return false;
      if (!has_feat_requirement_check(ch, FEAT_COMBAT_CHALLENGE))
        return false;
      return true;

    case FEAT_GREATER_COMBAT_CHALLENGE:
      if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 15 &&
          GET_ABILITY(ch, ABILITY_INTIMIDATE) < 15 &&
          GET_ABILITY(ch, ABILITY_BLUFF) < 15)
        return false;
      if (!has_feat_requirement_check(ch, FEAT_IMPROVED_COMBAT_CHALLENGE))
        return false;
      return true;

    case FEAT_EPIC_PROWESS:
      if (has_feat_requirement_check(ch, FEAT_EPIC_PROWESS) >= 5)
        return FALSE;
      return TRUE;

    case FEAT_EPIC_WILDSHAPE:
      if (has_feat_requirement_check(ch, FEAT_EPIC_WILDSHAPE) >= 5)
        return FALSE;
      if (ch->real_abils.wis >= 21 && (CLASS_LEVEL(ch, CLASS_SHIFTER) + CLASS_LEVEL(ch, CLASS_DRUID)) >= 20)
        return TRUE;
      return FALSE;

    case FEAT_EPIC_COMBAT_CHALLENGE:
      if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 20 &&
          GET_ABILITY(ch, ABILITY_INTIMIDATE) < 20 &&
          GET_ABILITY(ch, ABILITY_BLUFF) < 20)
        return false;
      if (!has_feat_requirement_check(ch, FEAT_GREATER_COMBAT_CHALLENGE))
        return false;
      return true;

    case FEAT_NATURAL_SPELL:
      if (ch->real_abils.wis < 13)
        return false;
      if (!has_feat_requirement_check(ch, FEAT_WILD_SHAPE))
        return false;
      return true;

    case FEAT_EPIC_DODGE:
      if (ch->real_abils.dex >= 25 && has_feat_requirement_check(ch, FEAT_DODGE) && has_feat_requirement_check(ch, FEAT_DEFENSIVE_ROLL) && GET_ABILITY(ch, ABILITY_ACROBATICS) >= 30)
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_SNEAK_ATTACK:
      if (has_feat_requirement_check(ch, FEAT_SNEAK_ATTACK) >= 8)
        return TRUE;
      return FALSE;

    case FEAT_SNEAK_ATTACK:
      if (has_feat_requirement_check(ch, FEAT_SNEAK_ATTACK) < 8)
        return FALSE;
      return TRUE;

    case FEAT_SNEAK_ATTACK_OF_OPPORTUNITY:
      if (has_feat_requirement_check(ch, FEAT_SNEAK_ATTACK) < 8)
        return FALSE;
      if (!has_feat_requirement_check(ch, FEAT_OPPORTUNIST))
        return FALSE;
      return TRUE;

    case FEAT_STEADFAST_DETERMINATION:
      if (!has_feat_requirement_check(ch, FEAT_ENDURANCE))
        return FALSE;
      return TRUE;

    case FEAT_GREAT_SMITING:
      if (ch->real_abils.cha >= 25 && has_feat_requirement_check(ch, FEAT_SMITE_EVIL))
        return TRUE;
      return FALSE;

    case FEAT_DIVINE_MIGHT:
    case FEAT_DIVINE_SHIELD:
      if (has_feat_requirement_check(ch, FEAT_TURN_UNDEAD) && has_feat_requirement_check(ch, FEAT_POWER_ATTACK) &&
          ch->real_abils.cha >= 13 && ch->real_abils.str >= 13)
        return TRUE;
      return FALSE;

    case FEAT_DIVINE_VENGEANCE:
      if (has_feat_requirement_check(ch, FEAT_TURN_UNDEAD) && has_feat_requirement_check(ch, FEAT_EXTRA_TURNING))
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_EVASION:
    case FEAT_CRIPPLING_STRIKE:
    case FEAT_DEFENSIVE_ROLL:
    case FEAT_OPPORTUNIST:
      if (CLASS_LEVEL(ch, CLASS_ROGUE) < 10)
        return FALSE;
      return TRUE;

    case FEAT_EMPOWERED_MAGIC:
      if (!IS_SPELLCASTER(ch))
        return FALSE;
      if (GET_LEVEL(ch) < (HAS_REAL_FEAT(ch, FEAT_EMPOWERED_MAGIC) * 5))
        return FALSE;
      if (HAS_REAL_FEAT(ch, FEAT_EMPOWERED_MAGIC) >= 3)
        return FALSE;
      return TRUE;

    case FEAT_EMPOWERED_PSIONICS:
      if (!IS_PSIONIC(ch))
        return FALSE;
      if (CLASS_LEVEL(ch, CLASS_PSIONICIST) < (HAS_REAL_FEAT(ch, FEAT_EMPOWERED_PSIONICS) * 5))
        return FALSE;
      if (HAS_REAL_FEAT(ch, FEAT_EMPOWERED_PSIONICS) >= 3)
        return FALSE;
      return TRUE;

    case FEAT_ENHANCED_SPELL_DAMAGE:
      if (!IS_SPELLCASTER(ch))
        return FALSE;
      if (CASTER_LEVEL(ch) < (HAS_REAL_FEAT(ch, FEAT_ENHANCED_SPELL_DAMAGE) * 5))
        return FALSE;
      if (HAS_REAL_FEAT(ch, FEAT_ENHANCED_SPELL_DAMAGE) >= 3)
        return FALSE;
      return TRUE;

    case FEAT_ENHANCED_POWER_DAMAGE:
      if (!IS_PSIONIC(ch))
        return FALSE;
      if (CLASS_LEVEL(ch, CLASS_PSIONICIST) < (HAS_REAL_FEAT(ch, FEAT_ENHANCED_POWER_DAMAGE) * 5))
        return FALSE;
      if (HAS_REAL_FEAT(ch, FEAT_ENHANCED_POWER_DAMAGE) >= 3)
        return FALSE;
      return TRUE;

    case FEAT_AUGMENT_SUMMONING:
      if (has_feat_requirement_check(ch, FEAT_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_SPELL_FOCUS), CONJURATION))
        return TRUE;
      return FALSE;

    case FEAT_FASTER_MEMORIZATION:
      if (IS_CASTER(ch))
        return TRUE;
      return FALSE;

    case FEAT_DAMAGE_REDUCTION:
      if (ch->real_abils.con < 21)
        return FALSE;
      return TRUE;

    case FEAT_MONKEY_GRIP:
      /*    if (!iarg)
              return TRUE;
            if (!is_proficient_with_weapon(ch, iarg))
              return FALSE;
            return TRUE;
         */
      return false;

    case FEAT_LAST_FEAT:
      return FALSE;

    case FEAT_SLIPPERY_MIND:
      if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 11)
        return TRUE;
      return FALSE;

    case FEAT_LINGERING_SONG:
    case FEAT_EXTRA_MUSIC:
      if (CLASS_LEVEL(ch, CLASS_BARD) > 0)
        return TRUE;
      return FALSE;

    case FEAT_EXTEND_RAGE:
    case FEAT_EXTRA_RAGE:
      if (has_feat_requirement_check(ch, FEAT_RAGE))
        return TRUE;
      return FALSE;

    case FEAT_ABLE_LEARNER:
      return TRUE;

    case FEAT_FAVORED_ENEMY:
      if (has_feat_requirement_check(ch, FEAT_FAVORED_ENEMY_AVAILABLE))
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_INTIMIDATION:
      if (GET_ABILITY(ch, ABILITY_INTIMIDATE) < 10)
        return FALSE;
      return TRUE;

    case FEAT_IMPROVED_INSTIGATION:
      if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 10)
        return FALSE;
      return TRUE;

    case FEAT_IMPROVED_TAUNTING:
      if (GET_ABILITY(ch, ABILITY_DIPLOMACY) < 10)
        return FALSE;
      return TRUE;

    case FEAT_TWO_WEAPON_DEFENSE:
      if (!has_feat_requirement_check(ch, FEAT_TWO_WEAPON_FIGHTING))
        return FALSE;
      if (ch->real_abils.dex < 15)
        return FALSE;
      return TRUE;

    case FEAT_IMPROVED_FEINT:
      if (!has_feat_requirement_check(ch, FEAT_COMBAT_EXPERTISE))
        return false;
      return true;

    case FEAT_AURA_OF_GOOD:
      if (CLASS_LEVEL(ch, CLASS_PALADIN))
        return true;
      return false;

    case FEAT_DETECT_EVIL:
      if (CLASS_LEVEL(ch, CLASS_PALADIN))
        return true;
      return false;

    case FEAT_SMITE_EVIL:
      if (CLASS_LEVEL(ch, CLASS_PALADIN))
        return true;
      return false;

    case FEAT_DIVINE_GRACE:
      if (CLASS_LEVEL(ch, CLASS_PALADIN) > 1)
        return true;
      return false;

    case FEAT_LAYHANDS:
      if (CLASS_LEVEL(ch, CLASS_PALADIN) > 1)
        return true;
      return false;

    case FEAT_AURA_OF_COURAGE:
      if (CLASS_LEVEL(ch, CLASS_PALADIN) > 2)
        return true;
      return false;

    case FEAT_TOUCH_OF_CORRUPTION:
      if (CLASS_LEVEL(ch, CLASS_BLACKGUARD) > 1)
        return true;
      return false;

    case FEAT_DIVINE_HEALTH:
      if (CLASS_LEVEL(ch, CLASS_PALADIN) > 2)
        return true;
      return false;

    case FEAT_TURN_UNDEAD:
      if (CLASS_LEVEL(ch, CLASS_PALADIN) > 3 || CLASS_LEVEL(ch, CLASS_CLERIC))
        return true;
      return false;

    case FEAT_REMOVE_DISEASE:
      if (CLASS_LEVEL(ch, CLASS_PALADIN) > 5)
        return true;
      return false;

    case FEAT_LICH_TOUCH:
      if (IS_LICH(ch))
        return true;
      return false;

    case FEAT_LICH_REJUV:
      if (IS_LICH(ch))
        return true;
      return false;

    case FEAT_LICH_FEAR:
      if (IS_LICH(ch))
        return true;
      return false;

    case FEAT_ARMOR_PROFICIENCY_HEAVY:
      if (has_feat_requirement_check(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
        return TRUE;
      return FALSE;

    case FEAT_ARMOR_PROFICIENCY_MEDIUM:
      if (has_feat_requirement_check(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
        return TRUE;
      return FALSE;

    case FEAT_DODGE:
      if (ch->real_abils.dex >= 13)
        return TRUE;
      return FALSE;

    case FEAT_MOBILITY:
      if (has_feat_requirement_check(ch, FEAT_DODGE))
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_DISARM:
      if (has_feat_requirement_check(ch, FEAT_COMBAT_EXPERTISE) &&
          ch->real_abils.intel >= 13)
        return TRUE;
      return FALSE;

    case FEAT_GREATER_DISARM:
      if (has_feat_requirement_check(ch, FEAT_IMPROVED_DISARM) &&
          has_feat_requirement_check(ch, FEAT_COMBAT_EXPERTISE) &&
          ch->real_abils.intel >= 13)
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_TRIP:
      if (has_feat_requirement_check(ch, FEAT_COMBAT_EXPERTISE))
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_GRAPPLE:
      if (has_feat_requirement_check(ch, FEAT_IMPROVED_UNARMED_STRIKE) && ch->real_abils.intel >= 13)
        return TRUE;
      return FALSE;

    case FEAT_WHIRLWIND_ATTACK:
      if (!has_feat_requirement_check(ch, FEAT_DODGE))
        return FALSE;
      if (!has_feat_requirement_check(ch, FEAT_MOBILITY))
        return FALSE;
      if (!has_feat_requirement_check(ch, FEAT_SPRING_ATTACK))
        return FALSE;
      if (ch->real_abils.intel < 13)
        return FALSE;
      if (ch->real_abils.dex < 13)
        return FALSE;
      if (BAB(ch) < 4)
        return FALSE;
      return TRUE;

    case FEAT_STUNNING_FIST:
      if (has_feat_requirement_check(ch, FEAT_IMPROVED_UNARMED_STRIKE) && ch->real_abils.wis >= 13 && ch->real_abils.dex >= 13 && BAB(ch) >= 8)
        return TRUE;
      if (MONK_TYPE(ch) > 0)
        return TRUE;
      return FALSE;

    case FEAT_POWER_ATTACK:
      if (ch->real_abils.str >= 13)
        return TRUE;
      return FALSE;

    case FEAT_CLEAVE:
      if (has_feat_requirement_check(ch, FEAT_POWER_ATTACK))
        return TRUE;
      return FALSE;

    case FEAT_GREAT_CLEAVE:
      if (has_feat_requirement_check(ch, FEAT_POWER_ATTACK) &&
          has_feat_requirement_check(ch, FEAT_CLEAVE) &&
          (BAB(ch) >= 4) &&
          (ch->real_abils.str >= 13))
        return TRUE;
      else
        return FALSE;

    case FEAT_SUNDER:
      if (has_feat_requirement_check(ch, FEAT_POWER_ATTACK))
        return TRUE;
      return FALSE;

    case FEAT_TWO_WEAPON_FIGHTING:
      if (ch->real_abils.dex >= 15)
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_TWO_WEAPON_FIGHTING:
      if (ch->real_abils.dex >= 17 && has_feat_requirement_check(ch, FEAT_TWO_WEAPON_FIGHTING) && BAB(ch) >= 6)
        return TRUE;
      return FALSE;

    case FEAT_GREATER_TWO_WEAPON_FIGHTING:
      if (ch->real_abils.dex >= 19 && has_feat_requirement_check(ch, FEAT_TWO_WEAPON_FIGHTING) &&
          has_feat_requirement_check(ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING) && BAB(ch) >= 11)
        return TRUE;
      return FALSE;

    case FEAT_PERFECT_TWO_WEAPON_FIGHTING:
      if (ch->real_abils.dex >= 21 && has_feat_requirement_check(ch, FEAT_GREATER_TWO_WEAPON_FIGHTING))
        return TRUE;
      return FALSE;

    case FEAT_BLINDING_SPEED:
      if (ch->real_abils.dex >= 25)
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_CRITICAL:
      if (BAB(ch) < 8)
        return FALSE;
      if (!iarg || is_proficient_with_weapon(ch, iarg))
        return TRUE;
      return FALSE;

    case FEAT_POWER_CRITICAL:
      if (BAB(ch) < 4)
        return FALSE;
      if (!iarg || has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
        return TRUE;
      return FALSE;
      /*
                  case FEAT_WEAPON_FLURRY:
                    if (BAB(ch) < 14)
                      return FALSE;
                    if (!iarg)
                      return TRUE;
                    if (!is_proficient_with_weapon(ch, iarg))
                      return FALSE;
                    if (!has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
                      return FALSE;
                    if (!has_combat_feat(ch, CFEAT_WEAPON_SPECIALIZATION, iarg))
                      return FALSE;
                    if (!has_combat_feat(ch, CFEAT_WEAPON_MASTERY, iarg))
                      return FALSE;
                    return TRUE;

                  case FEAT_WEAPON_SUPREMACY:
                    if (CLASS_LEVEL(ch, CLASS_WARRIOR) < 17)
                      return FALSE;
                    if (!iarg)
                      return TRUE;
                    if (!is_proficient_with_weapon(ch, iarg))
                      return FALSE;
                    if (!has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
                      return FALSE;
                    if (!has_combat_feat(ch, CFEAT_WEAPON_SPECIALIZATION, iarg))
                      return FALSE;
                    if (!has_combat_feat(ch, CFEAT_GREATER_WEAPON_FOCUS, iarg))
                      return FALSE;
                    if (!has_combat_feat(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION, iarg))
                      return FALSE;
                    if (!has_combat_feat(ch, CFEAT_WEAPON_MASTERY, iarg))
                      return FALSE;
                    return TRUE;

                  case FEAT_ROBILARS_GAMBIT:
                    if (!HAS_REAL_FEAT(ch, FEAT_COMBAT_REFLEXES))
                      return FALSE;
                    if (BAB(ch) < 12)
                      return FALSE;
                    return TRUE;

                  case FEAT_KNOCKDOWN:
                    if (!HAS_REAL_FEAT(ch, FEAT_IMPROVED_TRIP))
                      return FALSE;
                    if (BAB(ch) < 4)
                      return FALSE;
                    return TRUE;
         */
    case FEAT_ARMOR_SPECIALIZATION_LIGHT:
      if (!has_feat_requirement_check(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
        return FALSE;
      if (BAB(ch) < 11)
        return FALSE;
      return TRUE;

    case FEAT_ARMOR_SPECIALIZATION_MEDIUM:
      if (!has_feat_requirement_check(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
        return FALSE;
      if (BAB(ch) < 11)
        return FALSE;
      return TRUE;

    case FEAT_ARMOR_SPECIALIZATION_HEAVY:
      if (!has_feat_requirement_check(ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
        return FALSE;
      if (BAB(ch) < 11)
        return FALSE;
      return TRUE;

    case FEAT_WEAPON_FINESSE:
      if (BAB(ch) < 1)
        return FALSE;
      return TRUE;

    case FEAT_EPIC_SKILL_FOCUS:
      if (!iarg)
        return TRUE;
      if (GET_ABILITY(ch, iarg) >= 20)
        return TRUE;
      return FALSE;
      /*
          case FEAT_WEAPON_FOCUS:
            if (BAB(ch) < 1)
              return FALSE;
            if (!iarg || is_proficient_with_weapon(ch, iarg))
              return TRUE;
            return FALSE;
          case FEAT_GREATER_WEAPON_FOCUS:
            if (CLASS_LEVEL(ch, CLASS_WARRIOR) < 8)
              return FALSE;
            if (!iarg)
              return TRUE;
            if (is_proficient_with_weapon(ch, iarg) && has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
              return TRUE;
            return FALSE;

          case  FEAT_IMPROVED_WEAPON_FINESSE:
             if (!has_feat_requirement_check(ch, FEAT_WEAPON_FINESSE))
               return FALSE;
             if (BAB(ch) < 4)
                  return FALSE;
                if (!iarg)
                  return TRUE;
             if (!has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
                  return FALSE;
                if (weapon_list[iarg].size >= get_size(ch))
                  return FALSE;

             return TRUE;
         */
    case FEAT_WEAPON_SPECIALIZATION:
      if (BAB(ch) < 4 || WARRIOR_LEVELS(ch) < 4)
        return FALSE;
      if (!iarg || is_proficient_with_weapon(ch, iarg))
        return TRUE;
      return FALSE;
    case FEAT_GREATER_WEAPON_SPECIALIZATION:
      if (WARRIOR_LEVELS(ch) < 12)
        return FALSE;
      if (!iarg)
        return TRUE;
      if (is_proficient_with_weapon(ch, iarg) &&
          has_combat_feat(ch, CFEAT_GREATER_WEAPON_FOCUS, iarg) &&
          has_combat_feat(ch, CFEAT_WEAPON_SPECIALIZATION, iarg) &&
          has_combat_feat(ch, CFEAT_WEAPON_FOCUS, iarg))
        return TRUE;
      return FALSE;
    case FEAT_EPIC_WEAPON_SPECIALIZATION:
      if (WARRIOR_LEVELS(ch) < 20)
        return FALSE;
      if (!iarg)
        return TRUE;
      if (is_proficient_with_weapon(ch, iarg) &&
          has_combat_feat(ch, FEAT_GREATER_WEAPON_SPECIALIZATION, iarg))
        return TRUE;
      return FALSE;

    case FEAT_MUMMY_DUST:
      if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 23 && CASTER_LEVEL(ch) >= 20)
        return TRUE;
      return FALSE;
    case FEAT_DRAGON_KNIGHT:
      if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 25 && CASTER_LEVEL(ch) >= 21 &&
          (CLASS_LEVEL(ch, CLASS_WIZARD) > 17 ||
           CLASS_LEVEL(ch, CLASS_SORCERER) > 19))
        return TRUE;
      return FALSE;
    case FEAT_GREATER_RUIN:
      if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 27 && CASTER_LEVEL(ch) >= 22)
        return TRUE;
      return FALSE;
    case FEAT_HELLBALL:
      if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 29 && CASTER_LEVEL(ch) >= 23 &&
          (CLASS_LEVEL(ch, CLASS_WIZARD) > 16 ||
           CLASS_LEVEL(ch, CLASS_SORCERER) > 18))
        return TRUE;
      return FALSE;
    case FEAT_EPIC_MAGE_ARMOR:
      if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 31 && CASTER_LEVEL(ch) >= 24 &&
          (CLASS_LEVEL(ch, CLASS_WIZARD) > 13 ||
           CLASS_LEVEL(ch, CLASS_SORCERER) > 13))
        return TRUE;
      return FALSE;
    case FEAT_EPIC_WARDING:
      if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 32 && CASTER_LEVEL(ch) >= 25 &&
          (CLASS_LEVEL(ch, CLASS_WIZARD) > 15 ||
           CLASS_LEVEL(ch, CLASS_SORCERER) > 15))
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_FAMILIAR:
      if (CLASS_LEVEL(ch, CLASS_WIZARD) || CLASS_LEVEL(ch, CLASS_SORCERER))
        return TRUE;
      return FALSE;

    case FEAT_SPELL_PENETRATION:
      if (CASTER_LEVEL(ch))
        return TRUE;
      return FALSE;

    case FEAT_BREW_POTION:
      if (CASTER_LEVEL(ch) >= 3 || CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 1)
        return TRUE;
      return FALSE;

    case FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR:
      if (GET_LEVEL(ch) >= 5)
        return TRUE;
      return FALSE;

    case FEAT_CRAFT_ROD:
      if (CASTER_LEVEL(ch) >= 9)
        return TRUE;
      return FALSE;

    case FEAT_CRAFT_STAFF:
      if (CASTER_LEVEL(ch) >= 12)
        return TRUE;
      return FALSE;

    case FEAT_CRAFT_WAND:
      if (CASTER_LEVEL(ch) >= 5)
        return TRUE;
      return FALSE;

    case FEAT_FORGE_RING:
      if (GET_LEVEL(ch) >= 5)
        return TRUE;
      return FALSE;

    case FEAT_SCRIBE_SCROLL:
      if (CASTER_LEVEL(ch) >= 1)
        return TRUE;
      return FALSE;

    case FEAT_EXTEND_SPELL:
      if (IS_SPELLCASTER(ch))
        return TRUE;
      return FALSE;

    case FEAT_HEIGHTEN_SPELL:
      if (CLASS_LEVEL(ch, CLASS_WIZARD))
        return TRUE;
      return FALSE;

    case FEAT_MAXIMIZE_SPELL:
    case FEAT_EMPOWER_SPELL:
      if (IS_SPELLCASTER(ch))
        return TRUE;
      return FALSE;

    case FEAT_QUICKEN_SPELL:
      if (IS_SPELLCASTER(ch))
        return TRUE;
      return FALSE;

    case FEAT_SILENT_SPELL:
      if (CLASS_LEVEL(ch, CLASS_WIZARD))
        return TRUE;
      return FALSE;

    case FEAT_STILL_SPELL:
      if (CLASS_LEVEL(ch, CLASS_WIZARD))
        return TRUE;
      return FALSE;

    case FEAT_EXTRA_TURNING:
      if (CLASS_LEVEL(ch, CLASS_CLERIC))
        return TRUE;
      return FALSE;

    case FEAT_SPELL_MASTERY:
      if (CLASS_LEVEL(ch, CLASS_WIZARD))
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_SPELL_RESISTANCE:
      if (has_feat_requirement_check(ch, FEAT_DIAMOND_SOUL))
        return TRUE;
      return FALSE;

    case FEAT_IMPROVED_SHIELD_PUNCH:
      if (has_feat_requirement_check(ch, FEAT_ARMOR_PROFICIENCY_SHIELD))
        return TRUE;
      return FALSE;

    case FEAT_SHIELD_CHARGE:
      if (!has_feat_requirement_check(ch, FEAT_IMPROVED_SHIELD_PUNCH) ||
          (BAB(ch) < 3))
        return FALSE;
      return TRUE;

    case FEAT_SHIELD_SLAM:
      if (!has_feat_requirement_check(ch, FEAT_SHIELD_CHARGE) ||
          !has_feat_requirement_check(ch, FEAT_IMPROVED_SHIELD_PUNCH) ||
          (BAB(ch) < 6))
        return FALSE;
      return TRUE;

      /* we're going to assume at this stage that this feat truly has
         no prerequisites */
    default:
      return TRUE;
    }
  }
  return TRUE;
}

/* simple debug command to make sure we have all our assigns set up */
ACMD(do_featlisting)
{
  int i = 0;

  for (i = 1; i < FEAT_LAST_FEAT; i++)
  {
    send_to_char(ch, "%d: %s\r\n", i, feat_list[i].name);
  }
}

/*
 *  --------------------------------Known Feats-------------------------------------
 *  Heavy Armor Proficiency       : You are proficient with heavy armor.
 *  Light Armor Proficiency       : You are proficient with light armor.
 *  Medium Armor Proficiency      : You are proficient with medium armor.
 *  Simple Weapon Proficiency     : You are proficient with simple weapons.
 *  Martial Weapon Proficiency    : You are proficient with martial weapons.
 *  Shield Proficiency            : You are proficient with shields and bucklers.
 *  Tower Shield Proficiency      : You are proficient with tower shields.
 *  Power Attack                  : You can make exceptionally powerful attacks.
 *  Cleave                        : You can follow through with powerful blows.
 *  Great Cleave                  : You can slay multiple enemies with each strike.
 *  Tongue of the Sun and the Moon: You can speak any language.
 *  Stacking Feat                 : This feat stacks.
 *  Stacking Feat                 : This feat stacks.
 *  --------------------------------------------------------------------------------
 *
 *  The short description of the feats must not be longer than 47 characters.
 *
 *  --------------------------------Known Feats-------------------------------------
 *  Heavy Armor Proficiency             Simple Weapon Proficiency
 *  Light Armor Proficiency             Martial Weapon Proficiency
 *  Medium Armor Proficiency            Shield Proficiency
 *  Shield Proficiency                  Stackable Feat
 *  Stackable Feat                      Weapon Focus (Greatsword)
 *  --------------------------------------------------------------------------------
 */
void list_feats(struct char_data *ch, const char *arg, int list_type, struct char_data *viewer)
{
  int i, sortpos, j;
  int none_shown = TRUE;
  int mode = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'}, buf2[MAX_STRING_LENGTH] = {'\0'}, buf3[150] = {'\0'};
  int count = 0;
  int subfeat;
  int line_length = 80; /* Width of the display. */
  // bool custom_output = FALSE;

  if (*arg && is_abbrev(arg, "descriptions"))
  {
    mode = 1;
  }

  /* Header bar */
  if (list_type == LIST_FEATS_KNOWN)
    sprintf(buf + strlen(buf), "\tC%s\tn", text_line_string("\tYKnown Feats\tC", line_length, '-', '-'));
  if (list_type == LIST_FEATS_AVAILABLE)
    sprintf(buf + strlen(buf), "\tC%s\tn", text_line_string("\tYAvailable Feats\tC", line_length, '-', '-'));
  if (list_type == LIST_FEATS_ALL)
    sprintf(buf + strlen(buf), "\tC%s\tn", text_line_string("\tYAll Feats\tC", line_length, '-', '-'));

  strlcpy(buf2, buf, sizeof(buf2));

  for (sortpos = 1; sortpos < NUM_FEATS; sortpos++)
  {

    if (strlen(buf2) > MAX_STRING_LENGTH - 180)
      break;

    i = feat_sort_info[sortpos];
    /*  Print the feat, depending on the type of list. */
    if (feat_list[i].in_game && (list_type == LIST_FEATS_KNOWN && (HAS_FEAT(ch, i))))
    {

      /* target listing of known feats, the following is customization of output
         for certain feats [such as: weapon focus (longsword)]*/

      if ((subfeat = feat_to_sfeat(i)) != -1)
      {
        /* This is a 'school feat' */
        for (j = 1; j < NUM_SCHOOLS; j++)
        {
          if (HAS_SCHOOL_FEAT(ch, subfeat, j))
          {
            if (mode == 1)
            { /* description mode */
              snprintf(buf3, sizeof(buf3), "%s (%s)", feat_list[i].name, spell_schools[j]);
              snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
            }
            else
            {
              snprintf(buf3, sizeof(buf3), "%s (%s)", feat_list[i].name, spell_schools[j]);
              count++;
              if (count % 2 == 0)
                snprintf(buf, sizeof(buf), "%-40s\r\n", buf3);
              else
                snprintf(buf, sizeof(buf), "%-40s ", buf3);
              // custom_output = TRUE;
            }
            strlcat(buf2, buf, sizeof(buf2));
            none_shown = FALSE;
          }
        }
      }
      else if ((subfeat = feat_to_cfeat(i)) != -1)
      {
        /* This is a 'combat feat' */
        for (j = 1; j < NUM_WEAPON_TYPES; j++)
        {

          /* we are not going to show extra composite bows */
          if (j == WEAPON_TYPE_COMPOSITE_LONGBOW_2 ||
              j == WEAPON_TYPE_COMPOSITE_LONGBOW_3 ||
              j == WEAPON_TYPE_COMPOSITE_LONGBOW_4 ||
              j == WEAPON_TYPE_COMPOSITE_LONGBOW_5 ||
              j == WEAPON_TYPE_COMPOSITE_SHORTBOW_2 ||
              j == WEAPON_TYPE_COMPOSITE_SHORTBOW_3 ||
              j == WEAPON_TYPE_COMPOSITE_SHORTBOW_4 ||
              j == WEAPON_TYPE_COMPOSITE_SHORTBOW_5)
            continue;

          if (HAS_COMBAT_FEAT(ch, subfeat, j))
          {
            if (mode == 1)
            {
              snprintf(buf3, sizeof(buf3), "%s (%s)", feat_list[i].name, j > NUM_WEAPON_FAMILIES ? "respec required" : weapon_family[j]);
              snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
            }
            else
            {
              snprintf(buf3, sizeof(buf3), "%s (%s)", feat_list[i].name, j > NUM_WEAPON_FAMILIES ? "respec required" : weapon_family[j]);
              count++;
              if (count % 2 == 0)
                snprintf(buf, sizeof(buf), "%-40s\r\n", buf3);
              else
                snprintf(buf, sizeof(buf), "%-40s ", buf3);
              // custom_output = TRUE;
            }
            strlcat(buf2, buf, sizeof(buf2));
            none_shown = FALSE;
          }
        }
      }
      else if ((subfeat = feat_to_skfeat(i)) != -1)
      {
        /* This is a 'skill' feat */
        for (j = 0; j < NUM_ABILITIES; j++)
        {
          if (ch->player_specials->saved.skill_focus[j][subfeat] != FALSE)
          {
            if (mode == 1)
            {
              snprintf(buf3, sizeof(buf3), "%s (%s)", feat_list[i].name, ability_names[j]);
              snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
            }
            else
            {
              snprintf(buf3, sizeof(buf3), "%s (%s) ", feat_list[i].name, ability_names[j]);
              count++;
              if (count % 2 == 0)
                snprintf(buf, sizeof(buf), "%-40s\r\n", buf3);
              else
                snprintf(buf, sizeof(buf), "%-40s ", buf3);
              // custom_output = TRUE;
            }
            strlcat(buf2, buf, sizeof(buf2));
            none_shown = FALSE;
          }
        }

        /* begin non special formats */
      }
      else if (i == FEAT_FAST_HEALING)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d hp/5 sec)", feat_list[i].name, HAS_FEAT(ch, FEAT_FAST_HEALING) * 3);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d hp/5 sec)", feat_list[i].name, HAS_FEAT(ch, FEAT_FAST_HEALING) * 3);
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_DAMAGE_REDUCTION)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d/-)", feat_list[i].name, 3 * HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%d/-)", feat_list[i].name, 3 * HAS_FEAT(ch, FEAT_DAMAGE_REDUCTION));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_IMPROVED_FAMILIAR)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_IMPROVED_FAMILIAR));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_IMPROVED_FAMILIAR));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_SHRUG_DAMAGE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d/-)", feat_list[i].name, HAS_FEAT(ch, FEAT_SHRUG_DAMAGE));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%d/-)", feat_list[i].name, HAS_FEAT(ch, FEAT_SHRUG_DAMAGE));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_ARMOR_SKIN)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d ac)", feat_list[i].name, HAS_FEAT(ch, FEAT_ARMOR_SKIN));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d ac)", feat_list[i].name, HAS_FEAT(ch, FEAT_ARMOR_SKIN));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_SORCERER_BLOODLINE_ARCANE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%s magic)", feat_list[i].name, spell_schools_lower[GET_BLOODLINE_SUBTYPE(ch)]);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%s magic)", feat_list[i].name, spell_schools_lower[GET_BLOODLINE_SUBTYPE(ch)]);
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_SCHOOL_POWER)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%s magic)", feat_list[i].name, spell_schools_lower[GET_BLOODLINE_SUBTYPE(ch)]);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%s magic)", feat_list[i].name, spell_schools_lower[GET_BLOODLINE_SUBTYPE(ch)]);
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_SORCERER_BLOODLINE_DRACONIC)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%s dragon)", feat_list[i].name, DRCHRTLIST_NAME(GET_BLOODLINE_SUBTYPE(ch)));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%s dragon)", feat_list[i].name, DRCHRTLIST_NAME(GET_BLOODLINE_SUBTYPE(ch)));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_DRACONIC_HERITAGE_BREATHWEAPON)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%s, %dx/day)", feat_list[i].name, DRCHRT_ENERGY_TYPE(GET_BLOODLINE_SUBTYPE(ch)), get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%s, %dx/day)", feat_list[i].name, DRCHRT_ENERGY_TYPE(GET_BLOODLINE_SUBTYPE(ch)), get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_EFREETI_MAGIC)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx/day)", feat_list[i].name, get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx/day)", feat_list[i].name, get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_DRAGON_MAGIC)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx/day)", feat_list[i].name, get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx/day)", feat_list[i].name, get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_PIXIE_DUST)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx/day)", feat_list[i].name, get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx/day)", feat_list[i].name, get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_DRACONIC_HERITAGE_CLAWS)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%s, %dx/day)", feat_list[i].name, DRCHRT_ENERGY_TYPE(GET_BLOODLINE_SUBTYPE(ch)), get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%s, %dx/day)", feat_list[i].name, DRCHRT_ENERGY_TYPE(GET_BLOODLINE_SUBTYPE(ch)), get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_NEW_ARCANA)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (extra circle slots: %d/%d/%d)", feat_list[i].name, NEW_ARCANA_SLOT(ch, 0), NEW_ARCANA_SLOT(ch, 1), NEW_ARCANA_SLOT(ch, 2));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (extra circle slots: %d/%d/%d)", feat_list[i].name, NEW_ARCANA_SLOT(ch, 0), NEW_ARCANA_SLOT(ch, 1), NEW_ARCANA_SLOT(ch, 2));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_DRACONIC_BLOODLINE_ARCANA)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%s damage)", feat_list[i].name, DRCHRT_ENERGY_TYPE(GET_BLOODLINE_SUBTYPE(ch)));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%s damage)", feat_list[i].name, DRCHRT_ENERGY_TYPE(GET_BLOODLINE_SUBTYPE(ch)));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_DRACONIC_HERITAGE_DRAGON_RESISTANCES || i == FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (resist %s)", feat_list[i].name, DRCHRT_ENERGY_TYPE(GET_BLOODLINE_SUBTYPE(ch)));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (resist %s)", feat_list[i].name, DRCHRT_ENERGY_TYPE(GET_BLOODLINE_SUBTYPE(ch)));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_IMPROVED_REACTION)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, 2 * HAS_FEAT(ch, FEAT_IMPROVED_REACTION));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, 2 * HAS_FEAT(ch, FEAT_IMPROVED_REACTION));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_NATURAL_ATTACK)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d attack dice rolls)", feat_list[i].name, HAS_FEAT(ch, FEAT_NATURAL_ATTACK));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d attack dice rolls)", feat_list[i].name, HAS_FEAT(ch, FEAT_NATURAL_ATTACK));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_AC_BONUS)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d ac)", feat_list[i].name, HAS_FEAT(ch, FEAT_AC_BONUS));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d ac)", feat_list[i].name, HAS_FEAT(ch, FEAT_AC_BONUS));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_UNSTOPPABLE_STRIKE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d percent)", feat_list[i].name, HAS_FEAT(ch, FEAT_UNSTOPPABLE_STRIKE) * 5);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d percent)", feat_list[i].name, HAS_FEAT(ch, FEAT_UNSTOPPABLE_STRIKE) * 5);
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_ARMOR_TRAINING)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d ranks)", feat_list[i].name, HAS_FEAT(ch, FEAT_ARMOR_TRAINING));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d ranks)", feat_list[i].name, HAS_FEAT(ch, FEAT_ARMOR_TRAINING));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_WEAPON_TRAINING)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d ranks)", feat_list[i].name, HAS_FEAT(ch, FEAT_WEAPON_TRAINING));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d ranks)", feat_list[i].name, HAS_FEAT(ch, FEAT_WEAPON_TRAINING));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_CRITICAL_SPECIALIST)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (-%d threat)", feat_list[i].name, HAS_FEAT(ch, FEAT_CRITICAL_SPECIALIST));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (-%d threat)", feat_list[i].name, HAS_FEAT(ch, FEAT_CRITICAL_SPECIALIST));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_SLOW_FALL)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d feet)", feat_list[i].name, 10 * HAS_FEAT(ch, FEAT_SLOW_FALL));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d feet)", feat_list[i].name, 10 * HAS_FEAT(ch, FEAT_SLOW_FALL));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_EPIC_PROWESS)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d attack bonus)", feat_list[i].name, HAS_FEAT(ch, FEAT_EPIC_PROWESS));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d attack bonus)", feat_list[i].name, HAS_FEAT(ch, FEAT_EPIC_PROWESS));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_EPIC_WILDSHAPE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_EPIC_TOUGHNESS)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d hp)", feat_list[i].name, (HAS_FEAT(ch, FEAT_EPIC_TOUGHNESS) * 30));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d hp)", feat_list[i].name, (HAS_FEAT(ch, FEAT_EPIC_TOUGHNESS) * 30));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_ENERGY_RESISTANCE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d/-)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENERGY_RESISTANCE) * 3);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%d/-)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENERGY_RESISTANCE) * 3);
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_HASTE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (3x/day)", feat_list[i].name);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (3x/day)", feat_list[i].name);
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_SACRED_FLAMES)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d / day)", feat_list[i].name, HAS_FEAT(ch, FEAT_SACRED_FLAMES));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%d / day)", feat_list[i].name, HAS_FEAT(ch, FEAT_SACRED_FLAMES));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_DRAGON_MOUNT_BREATH)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx/day)", feat_list[i].name, HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BREATH));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx/day)", feat_list[i].name, HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BREATH));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_DRAGON_MOUNT_BOOST)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BOOST));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_DRAGON_MOUNT_BOOST));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_BREATH_WEAPON)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%dd8 dmg|%dx/day)", feat_list[i].name, HAS_FEAT(ch, FEAT_BREATH_WEAPON), HAS_FEAT(ch, FEAT_BREATH_WEAPON));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%dd8 dmg|%dx/day)", feat_list[i].name, HAS_FEAT(ch, FEAT_BREATH_WEAPON), HAS_FEAT(ch, FEAT_BREATH_WEAPON));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_RAGE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d / day)", feat_list[i].name, HAS_FEAT(ch, FEAT_RAGE));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%d / day)", feat_list[i].name, HAS_FEAT(ch, FEAT_RAGE));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_DEFENSIVE_STANCE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d / day)", feat_list[i].name, HAS_FEAT(ch, FEAT_DEFENSIVE_STANCE));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%d / day)", feat_list[i].name, HAS_FEAT(ch, FEAT_DEFENSIVE_STANCE));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_ENHANCED_POWER_DAMAGE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d dam / die)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENHANCED_POWER_DAMAGE));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d dam / die)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENHANCED_POWER_DAMAGE));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_EMPOWERED_PSIONICS)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d to dcs)", feat_list[i].name, HAS_FEAT(ch, FEAT_EMPOWERED_PSIONICS));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d to dcs)", feat_list[i].name, HAS_FEAT(ch, FEAT_EMPOWERED_PSIONICS));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_ENHANCED_SPELL_DAMAGE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d dam / die)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENHANCED_SPELL_DAMAGE));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d dam / die)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENHANCED_SPELL_DAMAGE));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_EMPOWERED_MAGIC)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d to dcs)", feat_list[i].name, HAS_FEAT(ch, FEAT_EMPOWERED_MAGIC));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d to dcs)", feat_list[i].name, HAS_FEAT(ch, FEAT_EMPOWERED_MAGIC));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_ENHANCE_SPELL)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d dam dice)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENHANCE_SPELL) * 5);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d dam dice)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENHANCE_SPELL) * 5);
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_NATURAL_ARMOR_INCREASE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d ac)", feat_list[i].name, HAS_FEAT(ch, FEAT_NATURAL_ARMOR_INCREASE));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d ac)", feat_list[i].name, HAS_FEAT(ch, FEAT_NATURAL_ARMOR_INCREASE));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_GREAT_STRENGTH)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_STRENGTH));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_STRENGTH));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_GREAT_DEXTERITY)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_DEXTERITY));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_DEXTERITY));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_GREAT_CONSTITUTION)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_CONSTITUTION));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_CONSTITUTION));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_GREAT_INTELLIGENCE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_INTELLIGENCE));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_INTELLIGENCE));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_GREAT_WISDOM)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_WISDOM));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_WISDOM));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_GREAT_CHARISMA)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_CHARISMA));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_GREAT_CHARISMA));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_POISON_SAVE_BONUS)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_POISON_SAVE_BONUS));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_POISON_SAVE_BONUS));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_BOMBS)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%dd6)", feat_list[i].name, HAS_FEAT(ch, FEAT_BOMBS));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%dd6)", feat_list[i].name, HAS_FEAT(ch, FEAT_BOMBS));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_ALCHEMICAL_DISCOVERY)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (x%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_ALCHEMICAL_DISCOVERY));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (x%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_ALCHEMICAL_DISCOVERY));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_SNEAK_ATTACK)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%dd6)", feat_list[i].name, HAS_FEAT(ch, FEAT_SNEAK_ATTACK));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%dd6)", feat_list[i].name, HAS_FEAT(ch, FEAT_SNEAK_ATTACK));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_TOUCH_OF_CORRUPTION)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%dd6)", feat_list[i].name, CLASS_LEVEL(ch, CLASS_BLACKGUARD) / 2);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%dd6)", feat_list[i].name, CLASS_LEVEL(ch, CLASS_BLACKGUARD) / 2);
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_IGNORE_SPELL_FAILURE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d%%)", feat_list[i].name, 5 + (HAS_FEAT(ch, FEAT_IGNORE_SPELL_FAILURE) * 5));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%d%%)", feat_list[i].name, 5 + (HAS_FEAT(ch, FEAT_IGNORE_SPELL_FAILURE) * 5));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_CHANNEL_SPELL)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d/day)", feat_list[i].name, 2 + (HAS_FEAT(ch, FEAT_CHANNEL_SPELL)));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%d/day)", feat_list[i].name, 2 + (HAS_FEAT(ch, FEAT_CHANNEL_SPELL)));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_ANIMATE_DEAD)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_ANIMATE_DEAD));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_ANIMATE_DEAD));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_ARMORED_SPELLCASTING)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_ARMORED_SPELLCASTING));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_ARMORED_SPELLCASTING));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_TRAP_SENSE)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_TRAP_SENSE));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_TRAP_SENSE));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_SELF_CONCEALMENT)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d%% miss)", feat_list[i].name, HAS_FEAT(ch, FEAT_SELF_CONCEALMENT) * 10);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%d%% miss)", feat_list[i].name, HAS_FEAT(ch, FEAT_SELF_CONCEALMENT) * 10);
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_ENHANCE_ARROW_MAGIC)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENHANCE_ARROW_MAGIC));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_ENHANCE_ARROW_MAGIC));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_IMPROMPTU_SNEAK_ATTACK)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx / day)", feat_list[i].name, HAS_FEAT(ch, FEAT_IMPROMPTU_SNEAK_ATTACK));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx / day)", feat_list[i].name, HAS_FEAT(ch, FEAT_IMPROMPTU_SNEAK_ATTACK));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_SHADOW_ILLUSION || i == FEAT_SHADOW_CALL || i == FEAT_SHADOW_POWER || i == FEAT_SHADOW_JUMP)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx / day)", feat_list[i].name, get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (%dx / day)", feat_list[i].name, get_daily_uses(ch, i));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_FAST_CRAFTER)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d%% less time)", feat_list[i].name, HAS_FEAT(ch, FEAT_FAST_CRAFTER) * 10);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (-%d seconds)", feat_list[i].name, HAS_FEAT(ch, FEAT_FAST_CRAFTER) * 10);
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_PROFICIENT_CRAFTER)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d to checks)", feat_list[i].name, HAS_FEAT(ch, FEAT_PROFICIENT_CRAFTER));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d to checks)", feat_list[i].name, HAS_FEAT(ch, FEAT_PROFICIENT_CRAFTER));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_PROFICIENT_HARVESTER)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (+%d to checks)", feat_list[i].name, HAS_FEAT(ch, FEAT_PROFICIENT_HARVESTER));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%-20s (+%d to checks)", feat_list[i].name, HAS_FEAT(ch, FEAT_PROFICIENT_HARVESTER));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;
      }
      else if (i == FEAT_THEURGE_SPELLCASTING)
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s (%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_THEURGE_SPELLCASTING));
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf3, sizeof(buf3), "%-20s (%d)", feat_list[i].name, HAS_FEAT(ch, FEAT_THEURGE_SPELLCASTING));
          snprintf(buf, sizeof(buf), "%-40s ", buf3);
        }
        strlcat(buf2, buf, sizeof(buf2));
        none_shown = FALSE;

        /* DEFAULT output */
      }
      else
      {
        if (mode == 1)
        {
          snprintf(buf3, sizeof(buf3), "%s", feat_list[i].name);
          snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
        }
        else
        {
          snprintf(buf, sizeof(buf), "%-40s ", feat_list[i].name);
        }
        strlcat(buf2, buf, sizeof(buf2)); /* The above, ^ should always be safe to do. */
        none_shown = FALSE;
      }

      /*  If we are not in description mode, split the output up in columns. */
      // if (!mode && !custom_output) {
      if (!mode)
      {
        count++;
        if (count % 2 == 0)
          strlcat(buf2, "\r\n", sizeof(buf2));
      }

      /* alternatively, list available or full feat lists */
    }
    else if (feat_list[i].in_game &&
             ((list_type == LIST_FEATS_ALL) ||
              (list_type == LIST_FEATS_AVAILABLE && (feat_is_available(ch, i, 0, NULL) && feat_list[i].can_learn))))
    {

      /* Display a simple list of all feats. */
      if (mode == 1)
      {
        snprintf(buf3, sizeof(buf3), "%s", feat_list[i].name);
        snprintf(buf, sizeof(buf), "\tW%-30s\tC:\tn %s\r\n", buf3, feat_list[i].short_description);
      }
      else
      {
        snprintf(buf, sizeof(buf), "%-40s ", feat_list[i].name);
      }

      strlcat(buf2, buf, sizeof(buf2)); /* The above, ^ should always be safe to do. */
      none_shown = FALSE;

      /*  If we are not in description mode, split the output up in columns. */
      if (!mode)
      {
        count++;
        if (count % 2 == 0)
          strlcat(buf2, "\r\n", sizeof(buf2));
      }
    }

  } /*end for loop*/

  /* regardless of the listing type (known, all, available, etc), you will
   end up here*/

  if (none_shown)
  {
    snprintf(buf, sizeof(buf), "You do not know any feats at this time.\r\n");
    strlcat(buf2, buf, sizeof(buf2));
  }

  if (count % 2 == 1) /* Only one feat on last row */
    strlcat(buf2, "\r\n", sizeof(buf2));

  strlcat(buf2, "\tC", sizeof(buf2));
  strlcat(buf2, line_string(line_length, '-', '-'), sizeof(buf2));
  strlcat(buf2, "\tDSyntax: feats < known|available|all  <description> >\tn\r\n", sizeof(buf2));
  strlcat(buf2, "\tDType 'feat info <name of feat>' to get specific information about a particular feat.\tn\r\n", sizeof(buf2));
  strlcat(buf2, "\tDType 'class feats <name of class>' to get a list of free feats that class gets.\tn\r\n", sizeof(buf2));

  if (!viewer)
    viewer = ch;

  page_string(viewer->desc, buf2, 1);
}

int is_class_feat(int featnum, int class, struct char_data *ch)
{
  struct class_feat_assign *feat_assign = NULL;

  /* we now traverse the class's list of class-feats to see if 'featnum' matches */
  for (feat_assign = class_list[class].featassign_list; feat_assign != NULL;
       feat_assign = feat_assign->next)
  {
    /* is this a class feat?  and is this feat a match for 'featnum'? */
    if (feat_assign->is_classfeat && feat_assign->feat_num == featnum)
    {
      return TRUE; /* yep this is a class feat! */
    }
  }

  // Sorcerer bloodline class feats
  if (class == CLASS_SORCERER)
  {
    if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_DRACONIC))
    {
      switch (featnum)
      {
      case FEAT_BLIND_FIGHT:
      case FEAT_GREAT_FORTITUDE:
      case FEAT_IMPROVED_INITIATIVE:
      case FEAT_POWER_ATTACK:
      case FEAT_QUICKEN_SPELL:
      case FEAT_SKILL_FOCUS:
      case FEAT_TOUGHNESS:
        return TRUE;
      }
    }
    else if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_ARCANE))
    {
      switch (featnum)
      {
      case FEAT_COMBAT_CASTING:
      case FEAT_IMPROVED_COUNTERSPELL:
      case FEAT_IMPROVED_INITIATIVE:
      case FEAT_IRON_WILL:
      case FEAT_SCRIBE_SCROLL:
      case FEAT_SKILL_FOCUS:
      case FEAT_SPELL_FOCUS:
      case FEAT_STILL_SPELL:
        return TRUE;
      }
    }
    else if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_FEY))
    {
      switch (featnum)
      {
      case FEAT_DODGE:
      case FEAT_IMPROVED_INITIATIVE:
      case FEAT_LIGHTNING_REFLEXES:
      case FEAT_MOBILITY:
      case FEAT_POINT_BLANK_SHOT:
      case FEAT_PRECISE_SHOT:
      case FEAT_QUICKEN_SPELL:
      case FEAT_SKILL_FOCUS:
        return TRUE;
      }
    }
    else if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_UNDEAD))
    {
      switch (featnum)
      {
      case FEAT_COMBAT_CASTING:
      case FEAT_DIEHARD:
      case FEAT_ENDURANCE:
      case FEAT_IRON_WILL:
      case FEAT_SPELL_FOCUS:
      case FEAT_STILL_SPELL:
      case FEAT_TOUGHNESS:
      case FEAT_SKILL_FOCUS:
        return TRUE;
      }
    }
  }

  return FALSE;
}

int is_daily_feat(int featnum)
{
  return (feat_list[featnum].event != eNULL);
};

int find_feat_num(const char *name)
{
  int index, ok;
  const char *temp, *temp2;
  char first[256], first2[256];

  for (index = 1; index < NUM_FEATS; index++)
  {
    if (is_abbrev(name, feat_list[index].name))
      return (index);

    ok = TRUE;
    /* It won't be changed, but other uses of this function elsewhere may. */
    temp = any_one_arg_c(feat_list[index].name, first, sizeof(first));
    temp2 = any_one_arg_c(name, first2, sizeof(first2));
    while (*first && *first2 && ok)
    {
      if (!is_abbrev(first2, first))
        ok = FALSE;
      temp = any_one_arg_c(temp, first, sizeof(first));
      temp2 = any_one_arg_c(temp2, first2, sizeof(first2));
    }
    if (ok && !*first2)
      return (index);
  }

  return (-1);
}

/* display_feat_info()
 *
 * Show information about a particular feat, dynamically
 * generated to tailor the output to a particular player.
 *
 * Example feat info :
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * Feat    : Cleave
 * Type    : Combat
 * ---------------------------------------------------
 * Prerequisites : Power Attack, Str:13
 * Required for  : Greater Cleave
 * ---------------------------------------------------
 * Benefit : If you deal a creature enough damage to
 * make it drop (typically by dropping it to below 0
 * hit points or killing it), you get an immediate,
 * extra melee attack against another engaged
 * creature.  The extra attack is with the
 * same weapon and at the same bonus as the attack
 * that dropped the previous creature.  You can use
 * this ability once per round.
 *
 * Normal : (none)
 *
 * Special : A fighter may select Cleave as one of his
 * fighter bonus feats.
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *
 * (NOTE: The headers of the sections above will be colored
 * differently, making them stand out.) */
bool display_feat_info(struct char_data *ch, const char *featname)
{
  int feat = -1;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  //  static int line_length = 57;
  static int line_length = 80;

  skip_spaces_c(&featname);
  feat = find_feat_num(featname);

  if (feat == -1 || feat_list[feat].in_game == FALSE)
  {
    /* Not found - Maybe put in a soundex list here? */
    // send_to_char(ch, "Could not find that feat.\r\n");
    return FALSE;
  }

  /* We found the feat, and the feat number is stored in 'feat'. */
  /* Display the feat info, formatted. */
  send_to_char(ch, "\tC\r\n");
  // text_line(ch, "Feat Information", line_length, '-', '-');
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tcFeat    : \tn%s\r\n"
                   "\tcType    : \tn%s\r\n",
               //                   "\tcCommand : \tn%s\r\n",
               feat_list[feat].name,
               feat_types[feat_list[feat].feat_type]);
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /*  Here display the prerequisites */
  if (feat_list[feat].prerequisite_list == NULL)
  {
    snprintf(buf, sizeof(buf), "\tCPrerequisites : \tnnone\r\n");
  }
  else
  {
    bool first = TRUE;
    struct feat_prerequisite *prereq;

    /* Get the wielded weapon, so that we can use it for checking prerequesites.
     * Don't bother checking the offhand since we can only use one value.
     * Don't bother checking for special features like claws, since they can't
     *  be selected for feats.
     * DO bother checking if they don't have a weapon at all.
     */
    struct obj_data *weap = GET_EQ(ch, WEAR_WIELD_1);
    if (GET_EQ(ch, WEAR_WIELD_2H))
      weap = GET_EQ(ch, WEAR_WIELD_2H);
    int w_type = (weap == NULL) ? WEAPON_TYPE_UNARMED : GET_WEAPON_TYPE(weap);

    for (prereq = feat_list[feat].prerequisite_list; prereq != NULL; prereq = prereq->next)
    {
      if (first)
      {
        first = FALSE;
        snprintf(buf, sizeof(buf), "\tcPrerequisites : %s%s%s",
                 (meets_prerequisite(ch, prereq, w_type) ? "\tn" : "\tr"), prereq->description, "\tn");
      }
      else
      {
        snprintf(buf2, sizeof(buf2), ", %s%s%s",
                 (meets_prerequisite(ch, prereq, w_type) ? "\tn" : "\tr"), prereq->description, "\tn");
        strlcat(buf, buf2, sizeof(buf));
      }
    }
  }
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* This we will need to buffer and wrap so that it will fit in the space provided. */
  snprintf(buf, sizeof(buf), "\tcDescription : \tn%s\r\n",
           feat_list[feat].description);
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn\r\n");

  return TRUE;
}

/*  do_feats
 *  Overarching command for getting information out of the Luminari Feat system.
 *
 *  feats | feats known  - List all known (learnable) feats
 *  feat info <featname> - Show detailed information about a partcular feat.
 *  feat all             - List all feats.
 *  feat available       - List all feats for which you qualify
 *  feat category <category name> - List all feats in a specific category
 *
 *  Sample output :
 *
 *  feats known
 *  --------------------------------Known Feats-------------------------------------
 *  Heavy Armor Proficiency       : You are proficient with heavy armor.
 *  Light Armor Proficiency       : You are proficient with light armor.
 *  Medium Armor Proficiency      : You are proficient with medium armor.
 *  Simple Weapon Proficiency     : You are proficient with simple weapons.
 *  Martial Weapon Proficiency    : You are proficient with martial weapons.
 *  Shield Proficiency            : You are proficient with shields and bucklers.
 *  Tower Shield Proficiency      : You are proficient with tower shields.
 *  Power Attack                  : You can make exceptionally powerful attacks.
 *  Cleave                        : You can follow through with powerful blows.
 *  Great Cleave                  : You can slay multiple enemies with each strike.
 *  Tongue of the Sun and the Moon: You can speak any language.
 *  --------------------------------------------------------------------------------
 *
 *  --------------------------------Known Feats-------------------------------------
 *  Heavy Armor Proficiency             Simple Weapon Proficiency
 *  Light Armor Proficiency             Martial Weapon Proficiency
 *  Medium Armor Proficiency            Shield Proficiency
 *  Shield Proficiency                  Stackable Feat
 *  Stackable Feat
 *  --------------------------------------------------------------------------------
 *
 *  Use the same format for the other listings, other than info.
 *  */
ACMD(do_feats)
{
  char arg[80];
  char arg2[80];
  const char *featname;

  /*  Have to process arguments like this
   *  because of the syntax - feat info <featname> */
  featname = one_argument(argument, arg, sizeof(arg));
  one_argument(featname, arg2, sizeof(arg2));

  if (is_abbrev(arg, "known") || !*arg)
  {
    list_feats(ch, arg2, LIST_FEATS_KNOWN, ch);
  }
  else if (is_abbrev(arg, "info"))
  {

    if (!strcmp(featname, ""))
    {
      send_to_char(ch, "You must provide the name of a feat.\r\n");
    }
    else if (!display_feat_info(ch, featname))
    {
      send_to_char(ch, "Could not find that feat.\r\n");
    }
  }
  else if (is_abbrev(arg, "available"))
  {
    list_feats(ch, arg2, LIST_FEATS_AVAILABLE, ch);
  }
  else if (is_abbrev(arg, "all"))
  {
    list_feats(ch, arg2, LIST_FEATS_ALL, ch);
  }
}

int feat_to_cfeat(int feat)
{
  switch (feat)
  {
  case FEAT_IMPROVED_CRITICAL:
    return CFEAT_IMPROVED_CRITICAL;
  case FEAT_POWER_CRITICAL:
    return CFEAT_POWER_CRITICAL;
    //  case FEAT_WEAPON_FINESSE:
    //    return CFEAT_WEAPON_FINESSE;
  case FEAT_WEAPON_FOCUS:
    return CFEAT_WEAPON_FOCUS;
  case FEAT_WEAPON_SPECIALIZATION:
    return CFEAT_WEAPON_SPECIALIZATION;
  case FEAT_GREATER_WEAPON_FOCUS:
    return CFEAT_GREATER_WEAPON_FOCUS;
  case FEAT_GREATER_WEAPON_SPECIALIZATION:
    return CFEAT_GREATER_WEAPON_SPECIALIZATION;
  case FEAT_EPIC_WEAPON_SPECIALIZATION:
    return CFEAT_EPIC_WEAPON_SPECIALIZATION;
  case FEAT_IMPROVED_WEAPON_FINESSE:
    return CFEAT_IMPROVED_WEAPON_FINESSE;
    // case FEAT_EXOTIC_WEAPON_PROFICIENCY:
    //   return CFEAT_EXOTIC_WEAPON_PROFICIENCY;
  case FEAT_MONKEY_GRIP:
    return CFEAT_MONKEY_GRIP;
  case FEAT_WEAPON_FLURRY:
    return CFEAT_WEAPON_FLURRY;
  case FEAT_WEAPON_SUPREMACY:
    return CFEAT_WEAPON_SUPREMACY;
  default:
    return -1;
  }
}

int feat_to_sfeat(int feat)
{
  switch (feat)
  {
  case FEAT_SPELL_FOCUS:
    return SFEAT_SPELL_FOCUS;
  case FEAT_GREATER_SPELL_FOCUS:
    return SFEAT_GREATER_SPELL_FOCUS;
  case FEAT_EPIC_SPELL_FOCUS:
    return SFEAT_EPIC_SPELL_FOCUS;
  default:
    return -1;
  }
}

int feat_to_skfeat(int feat)
{
  switch (feat)
  {
  case FEAT_SKILL_FOCUS:
    return SKFEAT_SKILL_FOCUS;
  case FEAT_EPIC_SKILL_FOCUS:
    return SKFEAT_EPIC_SKILL_FOCUS;
  default:
    return -1;
  }
}

/* sorcerer draconic bloodline heritages */
int get_draconic_heritage_subfeat(int feat)
{
  switch (feat)
  {
  case FEAT_SORCERER_BLOODLINE_DRACONIC:
    return BLFEAT_DRACONIC;
  }
  return -1;
}

int get_sorcerer_bloodline_type(struct char_data *ch)
{
  int bl = 0;
  if (HAS_FEAT(ch, (bl = FEAT_SORCERER_BLOODLINE_DRACONIC)))
    return bl;
  else if (HAS_FEAT(ch, (bl = FEAT_SORCERER_BLOODLINE_ARCANE)))
    return bl;
  else if (HAS_FEAT(ch, (bl = FEAT_SORCERER_BLOODLINE_FEY)))
    return bl;
  else if (HAS_FEAT(ch, (bl = FEAT_SORCERER_BLOODLINE_UNDEAD)))
    return bl;
  else
    bl = 0;
  return bl;
}

int get_levelup_sorcerer_bloodline_type(struct char_data *ch)
{
  int bl = FEAT_SORCERER_BLOODLINE_DRACONIC;
  if (LEVELUP(ch)->feats[bl] > 0)
    return bl;
  bl = FEAT_SORCERER_BLOODLINE_ARCANE;
  if (LEVELUP(ch)->feats[bl] > 0)
    return bl;
  bl = FEAT_SORCERER_BLOODLINE_FEY;
  if (LEVELUP(ch)->feats[bl] > 0)
    return bl;
  bl = FEAT_SORCERER_BLOODLINE_UNDEAD;
  if (LEVELUP(ch)->feats[bl] > 0)
    return bl;
  return 0;
}

bool isSorcBloodlineFeat(int featnum)
{
  switch (featnum)
  {
  case FEAT_SORCERER_BLOODLINE_DRACONIC:
  case FEAT_SORCERER_BLOODLINE_ARCANE:
  case FEAT_SORCERER_BLOODLINE_FEY:
  case FEAT_SORCERER_BLOODLINE_UNDEAD:
    return TRUE;
  }
  return FALSE;
}

bool valid_item_feat(int featnum)
{
  if (featnum < 1 || featnum >= FEAT_LAST_FEAT)
    return false;

  if (feat_list[featnum].can_learn && feat_list[featnum].combat_feat == FALSE && feat_list[featnum].epic == FALSE &&
      feat_list[featnum].in_game && feat_to_skfeat(featnum) == -1 &&
      (feat_list[featnum].feat_type == FEAT_TYPE_COMBAT || feat_list[featnum].feat_type == FEAT_TYPE_CRAFT ||
       feat_list[featnum].feat_type == FEAT_TYPE_GENERAL || feat_list[featnum].feat_type == FEAT_TYPE_METAMAGIC ||
       feat_list[featnum].feat_type == FEAT_TYPE_SPELLCASTING))
    return true;

  return false;
}

/* EOF */
