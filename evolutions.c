/*****************************************************************************
 ** evolutions.c                                       Part of LuminariMUD   **
 ** Source code for the LuminariMUD Feats System.                            **
 ** Initial code by Gicker (Stephen Squires)                                 **
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
#include "evolutions.h"
#include "fight.h"
#include "oasis.h"
#include "genolc.h"

int evolution_sort_info[NUM_EVOLUTIONS];
struct evolution_info evolution_list[NUM_EVOLUTIONS];

int compare_evolutions(const void *x, const void *y)
{
  int a = *(const int *)x,
      b = *(const int *)y;

  return strcmp(feat_list[a].name, feat_list[b].name);
}

/* sort evolutions called at boot up */
void sort_evolutions(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a < NUM_EVOLUTIONS; a++)
    evolution_sort_info[a] = a;

  qsort(&evolution_sort_info[1], NUM_EVOLUTIONS, sizeof(int), compare_evolutions);
}

static void evolutiono(int evolution, const char *name, int points, bool stacks, int stack_level, bool pc_avail, 
                       int req1, int req2, int req3, int req4, int req5, int req6, int req_type, const char *desc)
{
    evolution_list[evolution].name = name;
    evolution_list[evolution].evolution_points = points;
    evolution_list[evolution].stacks = stacks;
    evolution_list[evolution].stack_level = stack_level;
    evolution_list[evolution].pc_avail = pc_avail;
    evolution_list[evolution].evolution_requirements[0] = req1;
    evolution_list[evolution].evolution_requirements[1] = req2;
    evolution_list[evolution].evolution_requirements[2] = req3;
    evolution_list[evolution].evolution_requirements[3] = req4;
    evolution_list[evolution].evolution_requirements[4] = req5;
    evolution_list[evolution].evolution_requirements[5] = req6;
    evolution_list[evolution].requirement_type = req_type;
    evolution_list[evolution].desc = desc;
}

void initialize_evolution_list(void)
{
  int i;

  /* initialize the list of feats */
  for (i = 0; i < NUM_EVOLUTIONS; i++)
  {
    evolution_list[i].name = "Unused Evolution";
    evolution_list[i].evolution_points = 0;
    evolution_list[i].stacks = false;
    evolution_list[i].stack_level = 1;
    evolution_list[i].pc_avail = false;
    evolution_list[i].evolution_requirements[0] = 0;
    evolution_list[i].evolution_requirements[1] = 0;
    evolution_list[i].evolution_requirements[2] = 0;
    evolution_list[i].evolution_requirements[3] = 0;
    evolution_list[i].evolution_requirements[4] = 0;
    evolution_list[i].evolution_requirements[5] = 0;
    evolution_list[i].requirement_type = EVOLUTION_REQ_TYPE_NONE;
    evolution_list[i].desc = "Ask Staff";
  }
}

void assign_evolutions(void)
{

    initialize_evolution_list();

    // evolution, name, points, stacks, stack_level, pc_avail, req1, req2, req3, req4, req5, req6, req7, req8, req9, req10, reqtype, 
    // desc

    evolutiono(EVOLUTION_BASIC_MAGIC, "basic magic", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE, 
               "The eidolon is able to cast the following spells at-will: daze monster, detect magic, ball of light, "
               "acid splash, ray of frost, touch of fatigue.");
    evolutiono(EVOLUTION_BITE, "bite attack", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE, 
               "The eidolon gains a bite attack that deals 1d6 + 1/2 strength modifier piercing damage. 1d8 if large, 1d10 if huge.");
    evolutiono(EVOLUTION_BLEED, "bleeding attack", 3, false, 1, true, 
               EVOLUTION_BITE, EVOLUTION_CLAWS, EVOLUTION_PINCERS, EVOLUTION_GORE, EVOLUTION_RAKE, EVOLUTION_REND, EVOLUTION_REQ_TYPE_ANY,
               "The eidolon can cause 2 points of bleeding damage for 3 rounds with the following attack types: "
               "bite, claws, pincers, gore, rake and rend.");
    evolutiono(EVOLUTION_CLAWS, "claw attacks", 4, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains two claw attacks that deal 1d4 + 1/2 str mod slashing damage each. 1d6 if large, 1d8 if huge.");
    evolutiono(EVOLUTION_GILLS, "gills", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains gills and can breathe underwater.");
    evolutiono(EVOLUTION_HOOVES, "hoof attacks", 4, false, 1, false, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains two hoof attacks that deal 1d4 + 1/2 str mod bludgeoning damage each. 1d6 if large, 1d8 if huge.");
    evolutiono(EVOLUTION_IMPROVED_DAMAGE, "improved damage", 2, true, 4, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains +1 damage to all of their natural weapon attacks. Can be taken once every 4 summoner levels.");
    evolutiono(EVOLUTION_IMPROVED_NATURAL_ARMOR, "improved natural armor", 2, true, 5, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains +2 natural armor bonus per rank.  Can be taken once every 5 summoner levels.");
    evolutiono(EVOLUTION_MAGIC_ATTACKS, "magic attacks", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon's natural attacks are considered magic for the purpose of overcoming damage reduction. At summoner "
               "level 10, their attacks are considered aligned, using the same alignment as the summoner, for the purpose of "
               "overcoming damage reduction.");
    evolutiono(EVOLUTION_MOUNT, "mountable", 1, false, 1, false, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon can be used as a mount, as long as it is one size category larger than the person mounting.");
    evolutiono(EVOLUTION_PINCERS, "pincer attacks", 5, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains two pincer attacks, which deal 1d4 + 1/2 strength mod bludgeoning damage each, and also offer "
               "a +4 bonus to attacks to grapple the opponent. 1d6 if large, 1d8 if huge.");
    evolutiono(EVOLUTION_PUSH, "bullrush", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains the ability to attempt to push another creature out of the room they are in. The "
               "target must be smaller than the eidolon and the eidolon must succeed at a combat maneuver check. "
               "Failure results in combat. Note that this does not work on targets that have freedom of movement "
               "or mobiles with the sentinel flag. Uses the bullrush command.");
    evolutiono(EVOLUTION_REACH, "reach attacks", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolons attacks are considered reach weapons, allowing them an extra attack in the first round "
               "of any battle.");
    evolutiono(EVOLUTION_FIRE_RESIST, "fire resistance", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains resistance 5 to fire damage.  At summoner level 5, this increases to 10. At summoner "
               "level 10, this increases to 15.");
    evolutiono(EVOLUTION_COLD_RESIST, "cold resistance", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains resistance 5 to cold damage.  At summoner level 5, this increases to 10. At summoner "
               "level 10, this increases to 15.");
    evolutiono(EVOLUTION_ACID_RESIST, "acid resistance", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains resistance 5 to acid damage.  At summoner level 5, this increases to 10. At summoner "
               "level 10, this increases to 15.");
    evolutiono(EVOLUTION_ELECTRIC_RESIST, "electric resistance", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains resistance 5 to electric damage.  At summoner level 5, this increases to 10. At summoner "
               "level 10, this increases to 15.");
    evolutiono(EVOLUTION_SONIC_RESIST, "sonic resistance", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains resistance 5 to sonic damage.  At summoner level 5, this increases to 10. At summoner "
               "level 10, this increases to 15.");
    evolutiono(EVOLUTION_SCENT, "scent", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains the ability to track enemies by scent.");
    evolutiono(EVOLUTION_SKILLED, "skilled", 3, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains +2 to all skills.  At summoner level 10 this is +3.  At summoner level 20 this is "
               "+4 bonus and at summoner level 30, this is a +5 bonus.");
    evolutiono(EVOLUTION_STING, "sting attack", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains a sting attack, which deals 1d4 + 1/2 strength mod piercing damage. "
               "1d6 if large, 1d8 if huge.");
    evolutiono(EVOLUTION_SWIM, "swimmer", 1, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon can swim with ease, gaining a +20 to swim skill checks. They also bestow their owners "
               "with a +10 bonus to their swim checks.");
    evolutiono(EVOLUTION_TAIL_SLAP, "tail slap", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains a tail slap attack that deals 1d6 + 1/2 strength mod bludgeoning damage. "
               "1d6 if large, 1d8 if huge.");
    evolutiono(EVOLUTION_TENTACLE, "tentacle", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains a tentacle attack which deals 1d4 + 1/2 strength mod bludgeoning damage and "
               "offers a +4 to grapple checks made by the eidolon. Damage is 1d6 if large and 1d8 if huge.");
    evolutiono(EVOLUTION_WING_BUFFET, "wing buffet", 5, false, 1, true,  EVOLUTION_FLIGHT, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_ALL,
               "The eidolon gains two wing buffet attacks that deal 1d3 + 1/2 strength mod bludgeoning damage. "
               "This attack will also attempt to knock down any creatures smaller than the eidolon at a -4 to the "
               "check if one size category smaller, -2 for two size categories smaller, and no penalty for smaller. "
               "This will not work on targets with freedom of movement, or mobs with the NoBash flag.  The check is "
               "a CMB vs. CMD check.");
    evolutiono(EVOLUTION_STR_INCREASE, "strength increase", 2, true, 6, false, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
              "The eidolon will gain +2 to their strength score for each rank.  Can be taken once every six "
              "summoner levels.");
    evolutiono(EVOLUTION_DEX_INCREASE, "dexterity increase", 2, true, 6, false, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
              "The eidolon will gain +2 to their dexterity score for each rank.  Can be taken once every six "
              "summoner levels.");
    evolutiono(EVOLUTION_CON_INCREASE, "constitution increase", 2, true, 6, false, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
              "The eidolon will gain +2 to their constitution score for each rank.  Can be taken once every six "
              "summoner levels.");
    evolutiono(EVOLUTION_INT_INCREASE, "intelligence increase", 2, true, 6, false, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
              "The eidolon will gain +2 to their intelligence score for each rank.  Can be taken once every six "
              "summoner levels.");
    evolutiono(EVOLUTION_WIS_INCREASE, "wisdom increase", 2, true, 6, false, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
              "The eidolon will gain +2 to their wisdom score for each rank.  Can be taken once every six "
              "summoner levels.");
    evolutiono(EVOLUTION_CHA_INCREASE, "charisma increase", 2, true, 6, false, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
              "The eidolon will gain +2 to their charisma score for each rank.  Can be taken once every six "
              "summoner levels.");
    evolutiono(EVOLUTION_THRASH_EVIL, "thrash evil", 2, true, 10, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon inflicts 1d6 holy damage on any successful attacks against evil foes.");
    evolutiono(EVOLUTION_THRASH_GOOD, "thrash good", 2, true, 10, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon inflicts 1d6 unholy damage on any successful attacks against good foes.");
    evolutiono(EVOLUTION_CONSTRICT, "constrict", 2, false, 1, true, EVOLUTION_TENTACLE, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_ALL,
               "The eidolon adds an additional +4 damage to any tentacle attacks it performs.");
    evolutiono(EVOLUTION_FIRE_ATTACK, "flaming attacks", 4, false, 1, true, 
              EVOLUTION_COLD_ATTACK, EVOLUTION_ELECTRIC_ATTACK, EVOLUTION_ACID_ATTACK, EVOLUTION_SONIC_ATTACK, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
              "The eidolon deals 1d6 fire damage on all of their natural attacks.");
    evolutiono(EVOLUTION_COLD_ATTACK, "freezing attacks", 4, false, 1, true, 
              EVOLUTION_FIRE_ATTACK, EVOLUTION_ELECTRIC_ATTACK, EVOLUTION_ACID_ATTACK, EVOLUTION_SONIC_ATTACK, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
              "The eidolon deals 1d6 cold damage on all of their natural attacks.");
    evolutiono(EVOLUTION_ELECTRIC_ATTACK, "lightning attacks", 4, false, 1, true, 
              EVOLUTION_COLD_ATTACK, EVOLUTION_FIRE_ATTACK, EVOLUTION_ACID_ATTACK, EVOLUTION_SONIC_ATTACK, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
              "The eidolon deals 1d6 electric damage on all of their natural attacks.");
    evolutiono(EVOLUTION_ACID_ATTACK, "acidic attacks", 4, false, 1, true, 
              EVOLUTION_COLD_ATTACK, EVOLUTION_ELECTRIC_ATTACK, EVOLUTION_FIRE_ATTACK, EVOLUTION_SONIC_ATTACK, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
              "The eidolon deals 1d6 acid damage on all of their natural attacks.");
    evolutiono(EVOLUTION_SONIC_ATTACK, "sonic attacks", 4, false, 1, true, 
              EVOLUTION_COLD_ATTACK, EVOLUTION_ELECTRIC_ATTACK, EVOLUTION_ACID_ATTACK, EVOLUTION_FIRE_ATTACK, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
              "The eidolon deals 1d6 sonic damage on all of their natural attacks.");
    evolutiono(EVOLUTION_FLIGHT, "flight", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains the ability to fly at a speed of 50' per round.");
    evolutiono(EVOLUTION_GORE, "gore attack", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains a gore attack that deals 1d6 + 1/2 strength mod piercing damage. 1d8 if large "
               "and 1d10 if huge.");
    evolutiono(EVOLUTION_MINOR_MAGIC, "minor magic", 3, false, 1, true,  EVOLUTION_BASIC_MAGIC, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_ANY,
              "The eidolon gains the ability to cast the following spells at-will:  burning hands, "
              "detect alignment, magic missile, obscuring mist, minor image.");
    evolutiono(EVOLUTION_POISON, "poison attack", 2, false, 1, true,  EVOLUTION_BITE, EVOLUTION_STING, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_ANY,
               "Any time a bite or sting attack lands, the target will be afflicted as if the poison spell was cast "
               "upon them, unless they succeed with a fortitude saving throw against a dc equal to 10 + 1/2 the eidolon's "
               "level plus the eidolon's constitution modifier.");
    evolutiono(EVOLUTION_RAKE, "rake attacks", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gets two rake attacks dealing 1d6 + 1/2 strength mod slashing damage whenever the eidolon "
               "is grappling a target.");
    evolutiono(EVOLUTION_REND, "rend attack", 2, false, 1, true, EVOLUTION_CLAWS, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_ALL,
               "The eidolon is able to deal 1d4 + 1.5x strength modifier slashing damage whenever they hit on both "
               "claw attacks. 1d6 if large, 1d8 if huge.");
    evolutiono(EVOLUTION_RIDER_BOND, "rider bond", 2, false, 1, false, EVOLUTION_MOUNT, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_ALL,
               "The eidolon offers a bonus to ride skill equal to 1/2 summoner level, and also provides the mounted "
               "combat feat to the summoner when ridden by them.");
    evolutiono(EVOLUTION_SHADOW_BLEND, "shadow blend", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains 20 percent concealment when not in bright light.  If the eidolon also has the "
               "shadow form evolution, this benefit increases to 50 percent.");
    evolutiono(EVOLUTION_SHADOW_FORM, "shadow form", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidilon can become incorporeal at will. This effect can be toggled on and off with the shadowform command. "
               "Your eidolon will need to do this themselves. Eg. order eidolon shadowform. "
               "This effect will only work indoors or outside at night, dusk or dawn.");
    evolutiono(EVOLUTION_SICKENING, "sickening aura", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon emits a foul odour that affects everyone except the summoner and their party. "
               "Any creature in the same room as the eidolon must save vs. fortitude each round or be "
               "sickened for 1 round.  Once they have saved successfully, they are immune for 10 rounds. "
               "This ability uses the 'sickeningaura' command to turn on and off the effect.");
    evolutiono(EVOLUTION_TRAMPLE, "trample attack", 2, false, 1, true, EVOLUTION_HOOVES, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_ALL,
               "The eidolon is able to deal 1d4 + 1.5x strength modifier bludgeoning damage whenever they hit on both "
               "hoof attacks. 1d6 if large, 1d8 if huge.");
    evolutiono(EVOLUTION_TRIP, "trip attack", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "Whenever the eidolon makes a successful bite attack it will attempt to trip the opponent, if and only "
               "if the target is of the same size as the eidolon, or smaller.");
  evolutiono(EVOLUTION_UNDEAD_APPEARANCE, "undead appearance", 2, false, 1, false, EVOLUTION_CELESTIAL_APPEARANCE, EVOLUTION_FIENDISH_APPEARANCE, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
               "The eidolon appears as an undead creature. Negative energy heals and positive energy damages. "
               "Is treated as undead for certain spells and effects that affect undead. Gains +2 racial bonus to saving "
               "throws against disease, exhaustion, fatigue, paralysis, poison, sleep and stunning. At summoner "
               "level 7, this increases to +4. At summoner level 12 this becomes immunity. Although the eidolon "
               "appears as an undead they are still treated as an outsider.");
  evolutiono(EVOLUTION_BLINDSIGHT, "blindsight", 4, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon's gains blindsense, allowing them to be able to 'see' even if blinded, in darkness, or if the entity is invisible.");
  evolutiono(EVOLUTION_CELESTIAL_APPEARANCE, "celestial appearance", 3, false, 1, false, EVOLUTION_UNDEAD_APPEARANCE, EVOLUTION_FIENDISH_APPEARANCE, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
               "The eidolon appears as a celestial being and gains some of their benefits. The eidolon gains a "
               "+2 bonus on saves against disease, petrification, poison, and electricity spells and effects. It "
               "also gains spell resistance equal to 5 + its HD. At 7th level the save bonuses increase to +4. At "
               "12th level the protection becomes immunity and the spell resistance is increased to 10 + HD.");
  evolutiono(EVOLUTION_DAMAGE_REDUCTION, "damage reduction", 3, true, 5, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains 3/- damage reduction per rank.");
  evolutiono(EVOLUTION_FIENDISH_APPEARANCE, "fiendish appearance", 3, false, 1, false, EVOLUTION_UNDEAD_APPEARANCE, EVOLUTION_CELESTIAL_APPEARANCE, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
               "The eidolon appears as a celestial being and gains some of their benefits. The eidolon gains a +2 "
               "bonus on saving throws against acid, disease, fire, and poison spells and effects.  It "
               "also gains spell resistance equal to 5 + its HD. At 7th level the save bonuses increase to +4. At "
               "12th level the protection becomes immunity and the spell resistance is increased to 10 + HD.");
  evolutiono(EVOLUTION_FRIGHTFUL_PRESENCE, "frightful presence", 3, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "An eidolon becomes unsettling to its foes, gaining the frightful presence ability. Opponents in combat with the "
               "eidolon must make a Will save or become shaken for 3d6 rounds. The DC of this save is equal to 10 + 1/2 "
               "the eidolon's HD + the eidolon's Charisma modifier. Once a successful save has been made, the opponent "
               "is immune for 10 rounds. If the eidolon has at least 4 more Hit Dice than an opponent, that opponent "
               "becomes frightened instead. Foes with more HD than the eidolon are immune to this effect.");
  evolutiono(EVOLUTION_MAJOR_MAGIC, "major magic", 5, false, 1, true, EVOLUTION_BASIC_MAGIC, EVOLUTION_MINOR_MAGIC, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_ALL,
               "The eidolon can cast the following spells at-will: acid arrow, darkness, invisibility, lesser "
               "restoration, levitate, scorching ray, detect invisibility and spider climb.");
  evolutiono(EVOLUTION_SACRIFICE, "sacrifice", 3, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon can sacrifice its own hp to heal another at a ratio of 2:1 using the sachp command.");
  evolutiono(EVOLUTION_WEB, "web", 2, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon can cast the web spell at will as a swift action.");
  evolutiono(EVOLUTION_ACID_BREATH, "acid breath", 4, true, 10, true, EVOLUTION_FIRE_BREATH, EVOLUTION_COLD_BREATH, EVOLUTION_ELECTRIC_BREATH, 0, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
               "The eidolon can breathe acid dealing 1d6 / level acid damage to all enemies. The cooldown on this "
               "ability starts at 60 seconds. For every extra rank of this feat the cooldown is reduced by 18 seconds. "
               "This ability uses the evobreath command.");
  evolutiono(EVOLUTION_FIRE_BREATH, "fire breath", 4, true, 10, true, EVOLUTION_ACID_BREATH, EVOLUTION_COLD_BREATH, EVOLUTION_ELECTRIC_BREATH, 0, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
               "The eidolon can breathe fire dealing 1d6 / level fire damage to all enemies. The cooldown on this "
               "ability starts at 60 seconds. For every extra rank of this feat the cooldown is reduced by 18 seconds. "
               "This ability uses the evobreath command.");
  evolutiono(EVOLUTION_COLD_BREATH, "cold breath", 4, true, 10, true, EVOLUTION_FIRE_BREATH, EVOLUTION_ACID_BREATH, EVOLUTION_ELECTRIC_BREATH, 0, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
               "The eidolon can breathe frigid ice dealing 1d6 / level cold damage to all enemies. The cooldown on"
               " this ability starts at 60 seconds. For every extra rank of this feat the cooldown is reduced by 18 sec"
               "onds. This ability uses the evobreath command.");
  evolutiono(EVOLUTION_ELECTRIC_BREATH, "electric breath", 4, true, 10, true, EVOLUTION_FIRE_BREATH, EVOLUTION_COLD_BREATH, EVOLUTION_ACID_BREATH, 0, 0, 0, EVOLUTION_REQ_TYPE_UNIQUE,
               "The eidolon can breathe electricity dealing 1d6 / level electric damage to all enemies. The cooldo"
               "wn on this ability starts at 60 seconds. For every extra rank of this feat the cooldown is reduced by 1"
               "8 seconds. This ability uses the evobreath command.");
  evolutiono(EVOLUTION_FAST_HEALING, "fast healing", 4, true, 6, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gets fast healing 2 per rank of this evolution.");
  evolutiono(EVOLUTION_INCORPOREAL_FORM, "incorporeal form", 4, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon can become incorporeal at will.  While incorporeal the eidolon receives half damage "
               "from non-incorporeal foes and deals 1/2 melee damage when attacking non-incorporeal foes.");
  evolutiono(EVOLUTION_LARGE, "large sized", 4, false, 1, false, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon's base size increases to large, giving them +8 to strength and +4 to constitution as "
               "well as a +2 bonus to natural armor. It takes a -2 penalty to dexterity.  The size change causes "
               "a -1 penalty to AC and attack rolls, a +1 bonus to CMB and CMD, and a -4 penalty to stealth "
               "skill checks. ");
  evolutiono(EVOLUTION_HUGE, "huge sized", 6, false, 1, false, EVOLUTION_LARGE, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_ALL,
               "The eidolon's base size increases to huge, giving them +16 to strength and +8 to constitution as "
               "well as a +5 bonus to natural armor. It takes a -4 penalty to dexterity.  The size change causes "
               "a -2 penalty to AC and attack rolls, a +2 bonus to CMB and CMD, and a -8 penalty to stealth "
               "skill checks. ");
  evolutiono(EVOLUTION_SPELL_RESISTANCE, "spell resistance", 4, false, 1, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon gains spell resistance equal to 10 + their level. If the eidolon also has the celestial or fiendish "
               "appearance evolutions, the spell resistance increases to 15 + level.");
  evolutiono(EVOLUTION_ULTIMATE_MAGIC, "ultimate magic", 8, false, 1, true, EVOLUTION_BASIC_MAGIC, EVOLUTION_MINOR_MAGIC, EVOLUTION_MAJOR_MAGIC, 0, 0, 0, EVOLUTION_REQ_TYPE_ALL,
               "The eidolon can cast the following spells at-will: daylight, fireball, gaseous form, lightning bolt, "
               "fly, stinking cloud, tongues and water breathing.");
  evolutiono(EVOLUTION_KEEN_SCENT, "keen scent", 4, false, 1, true, EVOLUTION_SCENT, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_ALL,
               "The eidolon improves their ability to track enemies by scent. The tracking dc is reduced by 5.");
  evolutiono(EVOLUTION_PENETRATE_DR, "penetrate damage reduction", 1, true, 3, true, 0, 0, 0, 0, 0, 0, EVOLUTION_REQ_TYPE_NONE,
               "The eidolon is able to reduce damage reduction of its opponent for each attack by 3 points per rank.");

}

// This will return true if the character passes all of the requirement checks
// otherwise it will return false.
bool qualifies_for_evolution(struct char_data *ch, int evolution)
{
  int i = 0;
  int level = IS_NPC(ch) ? GET_LEVEL(ch) : GET_CALL_EIDOLON_LEVEL(ch);

  // if it's not available to PCs and 'ch' is a pc, they can't take it
  if (!IS_NPC(ch) && evolution_list[evolution].pc_avail != true)
    return false;

  // if the evolution can stack and the current ranks are greater than the stack level interval, they can't take it
  if (evolution_list[evolution].stacks && KNOWS_EVOLUTION(ch, evolution) > (level / evolution_list[evolution].stack_level))
    return false;

  // If there's no other requirements, they can take it
  if (evolution_list[evolution].requirement_type == EVOLUTION_REQ_TYPE_NONE)
    return true;
  // if It requires 'any' of the listed feats and they have any of them, they can take it
  // otherwise they can't
  else if (evolution_list[evolution].requirement_type == EVOLUTION_REQ_TYPE_ANY)
  {
    for (i = 0; i < 6; i++)
    {
      if (i == 0)
        continue;
      if (KNOWS_EVOLUTION(ch, evolution_list[evolution].evolution_requirements[i]))
        return true;
    }
  }
  // if It requires 'all' of the listed feats and they have all of them, they can take it
  // otherwise they can't
  else if (evolution_list[evolution].requirement_type == EVOLUTION_REQ_TYPE_ALL)
  {
    for (i = 0; i < 6; i++)
    {
      if (i == 0)
        continue;
      if (!KNOWS_EVOLUTION(ch, evolution_list[evolution].evolution_requirements[i]))
        return false;
    }
    return true;
  }
  // if it is 'unique', then if they have any of the listed feats, they can take it,
  // otherwise they can't
  else if (evolution_list[evolution].requirement_type == EVOLUTION_REQ_TYPE_UNIQUE)
  {
    for (i = 0; i < 6; i++)
    {
      if (i == 0)
        continue;
      if (KNOWS_EVOLUTION(ch, evolution_list[evolution].evolution_requirements[i]))
        return false;
    }
    return true;
  }

  return false;
}

bool study_evolution_already_taken_or_maxxed(struct char_data *ch, int evolution)
{
  if (IS_NPC(ch))
    return true;

  if (!LEVELUP(ch))
    return true;

  int level = GET_CALL_EIDOLON_LEVEL(ch);

  // if the evolution can stack and the current ranks are greater than the stack level interval, they can't take it
  if (evolution_list[evolution].stacks && LEVELUP(ch)->eidolon_evolutions[evolution] > (level / evolution_list[evolution].stack_level))
    return true;

  return false;
}

// This will return true if the character passes all of the requirement checks
// otherwise it will return false.
bool study_qualifies_for_evolution(struct char_data *ch, int evolution, bool is_pc)
{
  if (IS_NPC(ch)) return false;

  int i = 0;
  int level = GET_CALL_EIDOLON_LEVEL(ch);

  if (!LEVELUP(ch))
    return false;

  if (!evolution_list[evolution].stacks && LEVELUP(ch)->eidolon_evolutions[evolution])
    return false;

  // if the evolution can stack and the current ranks are greater than the stack level interval, they can't take it
  if (evolution_list[evolution].stacks && LEVELUP(ch)->eidolon_evolutions[evolution] > (level / evolution_list[evolution].stack_level))
    return false;

  if (study_num_free_evolution_points(ch) < evolution_list[evolution].evolution_points)
    return false;

  // if it's not available to PCs and we're checking for aspects return false
  if (is_pc && evolution_list[evolution].pc_avail != true)
    return false;

  // If there's no other requirements, they can take it
  if (evolution_list[evolution].requirement_type == EVOLUTION_REQ_TYPE_NONE)
    return true;
    
  // if It requires 'any' of the listed feats and they have any of them, they can take it
  // otherwise they can't
  else if (evolution_list[evolution].requirement_type == EVOLUTION_REQ_TYPE_ANY)
  {
    for (i = 0; i < 6; i++)
    {
      if (evolution_list[evolution].evolution_requirements[i] == 0)
        continue;
      if (LEVELUP(ch)->eidolon_evolutions[evolution_list[evolution].evolution_requirements[i]])
        return true;
    }
  }
  // if It requires 'all' of the listed feats and they have all of them, they can take it
  // otherwise they can't
  else if (evolution_list[evolution].requirement_type == EVOLUTION_REQ_TYPE_ALL)
  {
    for (i = 0; i < 6; i++)
    {
      if (evolution_list[evolution].evolution_requirements[i] == 0)
        continue;
      if (!LEVELUP(ch)->eidolon_evolutions[evolution_list[evolution].evolution_requirements[i]])
        return false;
    }
    return true;
  }
  // if it is 'unique', then if they have any of the listed feats, they can take it,
  // otherwise they can't
  else if (evolution_list[evolution].requirement_type == EVOLUTION_REQ_TYPE_UNIQUE)
  {
    for (i = 0; i < 6; i++)
    {
      if (evolution_list[evolution].evolution_requirements[i] == 0)
        continue;
      if (LEVELUP(ch)->eidolon_evolutions[evolution_list[evolution].evolution_requirements[i]])
        return false;
    }
    return true;
  }

  return false;
}

bool is_evolution_attack(int attack_type)
{
  switch (attack_type)
  {
    case ATTACK_TYPE_PRIMARY_EVO_BITE:
    case ATTACK_TYPE_PRIMARY_EVO_CLAWS:
    case ATTACK_TYPE_PRIMARY_EVO_HOOVES:
    case ATTACK_TYPE_PRIMARY_EVO_PINCERS:
    case ATTACK_TYPE_PRIMARY_EVO_STING:
    case ATTACK_TYPE_PRIMARY_EVO_TAIL_SLAP:
    case ATTACK_TYPE_PRIMARY_EVO_TENTACLE:
    case ATTACK_TYPE_PRIMARY_EVO_WING_BUFFET:
    case ATTACK_TYPE_PRIMARY_EVO_GORE:
    case ATTACK_TYPE_PRIMARY_EVO_RAKE:
    case ATTACK_TYPE_PRIMARY_EVO_REND:
    case ATTACK_TYPE_PRIMARY_EVO_TRAMPLE:
      return true;
  }
  return false;
}

int get_evolution_attack_w_type(int attack_type)
{
  switch (attack_type)
  {
    case ATTACK_TYPE_PRIMARY_EVO_BITE:
      return TYPE_BITE;
    case ATTACK_TYPE_PRIMARY_EVO_CLAWS:
    case ATTACK_TYPE_PRIMARY_EVO_REND:
      return TYPE_CLAW;
    case ATTACK_TYPE_PRIMARY_EVO_HOOVES:
      return TYPE_BLUDGEON;
    case ATTACK_TYPE_PRIMARY_EVO_PINCERS:
      return TYPE_CRUSH;
    case ATTACK_TYPE_PRIMARY_EVO_STING:
      return TYPE_STING;
    case ATTACK_TYPE_PRIMARY_EVO_TAIL_SLAP:
    case ATTACK_TYPE_PRIMARY_EVO_TENTACLE:
      return TYPE_SMASH;
    case ATTACK_TYPE_PRIMARY_EVO_WING_BUFFET:
      return TYPE_BLAST;
    case ATTACK_TYPE_PRIMARY_EVO_GORE:
      return TYPE_GORE;
    case ATTACK_TYPE_PRIMARY_EVO_RAKE:
      return TYPE_RAKE;    
    case ATTACK_TYPE_PRIMARY_EVO_TRAMPLE:
      return TYPE_TRAMPLE;
  }
  return TYPE_HIT;
}

int determine_evolution_attack_damage_dice(struct char_data *ch, int attack_type)
{
  switch (attack_type)
  {
    case ATTACK_TYPE_PRIMARY_EVO_BITE:
    case ATTACK_TYPE_PRIMARY_EVO_GORE:
    case ATTACK_TYPE_PRIMARY_EVO_RAKE:
    case ATTACK_TYPE_PRIMARY_EVO_TAIL_SLAP:
      switch(GET_SIZE(ch)) { case SIZE_LARGE: return dice(1, 8); case SIZE_HUGE: return dice(1, 10); 
                            case SIZE_COLOSSAL: return dice(1, 12); default: return dice(1, 6); }
    case ATTACK_TYPE_PRIMARY_EVO_CLAWS:
    case ATTACK_TYPE_PRIMARY_EVO_HOOVES:
    case ATTACK_TYPE_PRIMARY_EVO_PINCERS:
    case ATTACK_TYPE_PRIMARY_EVO_STING:
    case ATTACK_TYPE_PRIMARY_EVO_TENTACLE:
    case ATTACK_TYPE_PRIMARY_EVO_REND:
    case ATTACK_TYPE_PRIMARY_EVO_TRAMPLE:
      switch(GET_SIZE(ch)) { case SIZE_LARGE: return dice(1, 6); case SIZE_HUGE: return dice(1, 8); 
                            case SIZE_COLOSSAL: return dice(1, 10); default: return dice(1, 4); }
    case ATTACK_TYPE_PRIMARY_EVO_WING_BUFFET:
      switch(GET_SIZE(ch)) { case SIZE_LARGE: return dice(1, 4); case SIZE_HUGE: return dice(1, 6); 
                            case SIZE_COLOSSAL: return dice(1, 8); default: return dice(1, 3); }
  }
  return 0;
}

#define NORMAL_ATTACK_ROUTINE 0
#define DISPLAY_ROUTINE_POTENTIAL 2
#define MODE_DISPLAY_PRIMARY 2
#define PHASE_0 0
#define PHASE_1 1
void perform_evolution_attack(struct char_data *ch, int mode, int phase, int attack_type, int dam_type)
{
  
  if (!ch) return;
  
  if (mode == NORMAL_ATTACK_ROUTINE)
  { 
    if (valid_fight_cond(ch, FALSE))
      if (phase == PHASE_0 || phase == PHASE_1)
      {
        hit(ch, FIGHTING(ch), TYPE_UNDEFINED, dam_type, 0, attack_type);
      }
        
  }
  else if (mode == DISPLAY_ROUTINE_POTENTIAL)
  {
    /* display hitroll and damage bonuses */
    send_to_char(ch, "%s, Attack Bonus:  %d; ", attack_types[attack_type], compute_attack_bonus(ch, ch, attack_type));
    compute_hit_damage(ch, ch, TYPE_UNDEFINED_WTYPE, NO_DICEROLL, MODE_DISPLAY_PRIMARY, FALSE, attack_type, dam_type);
  }
}
#undef NORMAL_ATTACK_ROUTINE
#undef DISPLAY_ROUTINE_POTENTIAL
#undef MODE_DISPLAY_PRIMARY
#undef PHASE_0
#undef PHASE_1

void apply_evolution_bleed(struct char_data *ch)
{

  if (!ch) return;

  struct affected_type af;
  bool message = true;
  
  if (affected_by_spell(ch, EVOLUTION_BLEED_EFFECT))
  {
    affect_from_char(ch, EVOLUTION_BLEED_EFFECT);
    message = false;
  }

  new_affect(&af);

  af.spell = EVOLUTION_BLEED_EFFECT;
  af.modifier = 2;
  af.duration = 3;
  SET_BIT_AR(af.bitvector, AFF_BLEED);

  affect_to_char(ch, &af);

  if (message)
  {
    act("You start to bleed!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n starts to bleed!", TRUE, ch, 0, 0, TO_ROOM);
  }
}

void apply_evolution_poison(struct char_data *ch, struct char_data *vict)
{

  if (!ch) return;
  if (!vict) return;

  struct affected_type af;
  
  if (affected_by_spell(vict, SPELL_POISON))
  {
    return;
  }

  if (!can_poison(vict))
    return;

  if (check_poison_resist(ch, vict, CAST_INNATE, GET_CALL_EIDOLON_LEVEL(ch)))
    return;

  new_affect(&af);

  af.spell = SPELL_POISON;
  af.modifier = -4;
  af.location = APPLY_CON;
  af.duration = 5;
  SET_BIT_AR(af.bitvector, AFF_POISON);

  affect_to_char(vict, &af);

  act("You start to feel very unwell!", TRUE, vict, 0, 0, TO_CHAR);
  act("$n looks very unwell!", TRUE, vict, 0, 0, TO_ROOM);
}

void process_evolution_elemental_damage(struct char_data *ch, struct char_data *victim)
{
  if (!ch || !victim)
    return;
  
  if (HAS_EVOLUTION(ch, EVOLUTION_FIRE_ATTACK))
    damage(ch, victim, dice(1, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
  else if (HAS_EVOLUTION(ch, EVOLUTION_ACID_ATTACK))
    damage(ch, victim, dice(1, 6), TYPE_SPECAB_CORROSIVE, DAM_ACID, FALSE);
  else if (HAS_EVOLUTION(ch, EVOLUTION_COLD_ATTACK))
    damage(ch, victim, dice(1, 6), TYPE_SPECAB_FROST, DAM_COLD, FALSE);
  else if (HAS_EVOLUTION(ch, EVOLUTION_ELECTRIC_ATTACK))
    damage(ch, victim, dice(1, 6), TYPE_SPECAB_SHOCK, DAM_ELECTRIC, FALSE);
  else if (HAS_EVOLUTION(ch, EVOLUTION_SONIC_ATTACK))
    damage(ch, victim, dice(1, 6), TYPE_SPECAB_THUNDERING, DAM_SOUND, FALSE);
}

void process_evolution_thrash_alignment_damage(struct char_data *ch, struct char_data *victim)
{
  if (!ch || !victim)
    return;
  
  if (HAS_EVOLUTION(ch, EVOLUTION_THRASH_EVIL) && IS_EVIL(victim))
    damage(ch, victim, dice(1, 6), TYPE_SPECAB_HOLY, DAM_HOLY, FALSE);
  if (HAS_EVOLUTION(ch, EVOLUTION_THRASH_GOOD) && IS_GOOD(victim))
    damage(ch, victim, dice(1, 6), TYPE_SPECAB_UNHOLY, DAM_UNHOLY, FALSE);
}

void process_evolution_breath_damage(struct char_data *ch)
{
  if (!ch)
    return;

  int level = GET_CALL_EIDOLON_LEVEL(ch);

  if (HAS_EVOLUTION(ch, EVOLUTION_FIRE_BREATH))
    AoEDamageRoom(ch, dice(level, 6), SPELL_FIRE_BREATHE, DAM_FIRE);
  else if (HAS_EVOLUTION(ch, EVOLUTION_ACID_BREATH))
    AoEDamageRoom(ch, dice(level, 6), SPELL_ACID_BREATHE, DAM_ACID);
  else if (HAS_EVOLUTION(ch, EVOLUTION_COLD_BREATH))
    AoEDamageRoom(ch, dice(level, 6), SPELL_FROST_BREATHE, DAM_COLD);
  else if (HAS_EVOLUTION(ch, EVOLUTION_ELECTRIC_BREATH))
    AoEDamageRoom(ch, dice(level, 6), SPELL_LIGHTNING_BREATHE, DAM_ELECTRIC);
}

void assign_eidolon_evolutions(struct char_data *ch, struct char_data *mob, bool from_db)
{
  if (!ch || !mob)
    return;
    
  int i = 0, mlev = GET_LEVEL(mob), amt = 0;

  for (i = 0; i < NUM_EVOLUTIONS; i++)
  {
    if (KNOWS_EVOLUTION(ch, i))
    {
      HAS_REAL_EVOLUTION(mob, i) = KNOWS_EVOLUTION(ch, i);
    }      
  }

  // mob and aff flags
  SET_BIT_AR(MOB_FLAGS(mob), MOB_EIDOLON);
  if (HAS_EVOLUTION(mob, EVOLUTION_MOUNT))
    SET_BIT_AR(MOB_FLAGS(mob), MOB_MOUNTABLE);
  if (HAS_EVOLUTION(mob, EVOLUTION_FLIGHT))
    SET_BIT_AR(AFF_FLAGS(mob), AFF_FLYING);
  if (HAS_EVOLUTION(mob, EVOLUTION_GILLS))
    SET_BIT_AR(AFF_FLAGS(mob), AFF_WATER_BREATH);

  // resistances
  if (mlev >= 10)
    amt = 50;
  else if (mlev >= 5)
    amt = 25;
  else
    amt = 5;

  if (HAS_EVOLUTION(mob, EVOLUTION_FIRE_RESIST))
  {
    GET_RESISTANCES(ch, DAM_FIRE) += amt;
  }
  if (HAS_EVOLUTION(mob, EVOLUTION_COLD_RESIST))
  {
    GET_RESISTANCES(ch, DAM_COLD) += amt;
  }
  if (HAS_EVOLUTION(mob, EVOLUTION_ACID_RESIST))
  {
    GET_RESISTANCES(ch, DAM_ACID) += amt;
  }
  if (HAS_EVOLUTION(mob, EVOLUTION_ELECTRIC_RESIST))
  {
    GET_RESISTANCES(ch, DAM_ELECTRIC) += amt;
  }
  if (HAS_EVOLUTION(mob, EVOLUTION_SONIC_RESIST))
  {
    GET_RESISTANCES(ch, DAM_SOUND) += amt;
  }

  // ability scores
  if (!from_db)
  {
    GET_REAL_STR(mob) += HAS_EVOLUTION(mob, EVOLUTION_STR_INCREASE) * 2;
    GET_REAL_DEX(mob) += HAS_EVOLUTION(mob, EVOLUTION_DEX_INCREASE) * 2;
    GET_REAL_CON(mob) += HAS_EVOLUTION(mob, EVOLUTION_CON_INCREASE) * 2;
    GET_REAL_INT(mob) += HAS_EVOLUTION(mob, EVOLUTION_INT_INCREASE) * 2;
    GET_REAL_WIS(mob) += HAS_EVOLUTION(mob, EVOLUTION_WIS_INCREASE) * 2;
    GET_REAL_CHA(mob) += HAS_EVOLUTION(mob, EVOLUTION_CHA_INCREASE) * 2;
  }

  // fast healing
  GET_FAST_HEALING_MOD(mob) += HAS_EVOLUTION(mob, EVOLUTION_FAST_HEALING) * 2;

  // eidolon size
  if (HAS_EVOLUTION(mob, EVOLUTION_HUGE))
  {
    if (!from_db)
    {
      GET_REAL_STR(mob) += 16;
      GET_REAL_CON(mob) += 8;
      GET_REAL_DEX(mob) -= 4;
    }
    GET_REAL_SIZE(mob) = SIZE_HUGE;
  }
  else if (HAS_EVOLUTION(mob, EVOLUTION_LARGE))
  {
    if (!from_db)
    {
      GET_REAL_STR(mob) += 8;
      GET_REAL_CON(mob) += 4;
      GET_REAL_DEX(mob) -= 2;
    }
    GET_REAL_SIZE(mob) = SIZE_LARGE;
  }

  // base form
  switch (GET_EIDOLON_BASE_FORM(ch))
  {
  case EIDOLON_BASE_FORM_AVIAN:
    HAS_REAL_FEAT(mob, FEAT_IRON_WILL) = true;
    HAS_REAL_FEAT(mob, FEAT_LIGHTNING_REFLEXES) = true;
    if (!from_db)
    {

    }
    GET_REAL_STR(mob) += 4;
    GET_REAL_CON(mob) += 4;
    GET_REAL_DEX(mob) += 6;
    break;
  case EIDOLON_BASE_FORM_BIPED:
    HAS_REAL_FEAT(mob, FEAT_IRON_WILL) = true;
    HAS_REAL_FEAT(mob, FEAT_GREAT_FORTITUDE) = true;
    if (!from_db)
    {
      GET_REAL_STR(mob) += 6;
      GET_REAL_CON(mob) += 4;
      GET_REAL_DEX(mob) += 4;
    }
    break;
  case EIDOLON_BASE_FORM_QUADRUPED:
    HAS_REAL_FEAT(mob, FEAT_IRON_WILL) = true;
    HAS_REAL_FEAT(mob, FEAT_LIGHTNING_REFLEXES) = true;
    if (!from_db)
    {
      GET_REAL_STR(mob) += 6;
      GET_REAL_CON(mob) += 4;
      GET_REAL_DEX(mob) += 6;
    }
    break;
  case EIDOLON_BASE_FORM_SERPENTINE:
    HAS_REAL_FEAT(mob, FEAT_IRON_WILL) = true;
    HAS_REAL_FEAT(mob, FEAT_LIGHTNING_REFLEXES) = true;
    if (!from_db)
    {
      GET_REAL_STR(mob) += 4;
      GET_REAL_CON(mob) += 4;
      GET_REAL_DEX(mob) += 6;
    }
    break;
  case EIDOLON_BASE_FORM_TAURIC:
    HAS_REAL_FEAT(mob, FEAT_IRON_WILL) = true;
    HAS_REAL_FEAT(mob, FEAT_IRON_WILL) = true;
    if (!from_db)
    {
      GET_REAL_STR(mob) += 4;
      GET_REAL_CON(mob) += 6;
      GET_REAL_DEX(mob) += 2;
    }
    break;
  } 

  if (HAS_REAL_FEAT(ch, FEAT_GRAND_EIDOLON))
  {
    if (!from_db)
    {
      GET_REAL_STR(mob) += 2;
      GET_REAL_CON(mob) += 2;
      GET_REAL_DEX(mob) += 2;
      (mob)->aff_abils.intel += 2;
      (mob)->aff_abils.wis += 2;
      (mob)->aff_abils.cha += 2;
    }
  }
  if (HAS_REAL_FEAT(ch, FEAT_EPIC_EIDOLON))
  {
    if (!from_db)
    {
      GET_REAL_STR(mob) += 4;
      GET_REAL_CON(mob) += 4;
      GET_REAL_DEX(mob) += 4;
      (mob)->aff_abils.intel += 4;
      (mob)->aff_abils.wis += 4;
      (mob)->aff_abils.cha += 4;
    }
  } 
}

void merge_eidolon_evolutions(struct char_data *ch)
{
  if (!ch)
    return;

  int i = 0, mlev = GET_LEVEL(ch), amt = 0;
  struct affected_type af;
  struct affected_type af2[6];

  // aff flags
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_FLIGHT))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_GILLS))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_WATER_BREATH);

  // resistances
  if (mlev >= 10)
    amt = 50;
  else if (mlev >= 5)
    amt = 25;
  else
    amt = 5;

  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_FIRE_RESIST))
  {
    new_affect(&af);
    af.location = APPLY_RES_FIRE;
    af.modifier = amt;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_COLD_RESIST))
  {
    new_affect(&af);
    af.location = APPLY_RES_COLD;
    af.modifier = amt;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_ACID_RESIST))
  {
    new_affect(&af);
    af.location = APPLY_RES_ACID;
    af.modifier = amt;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_ELECTRIC_RESIST))
  {
    new_affect(&af);
    af.location = APPLY_RES_ELECTRIC;
    af.modifier = amt;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_SONIC_RESIST))
  {
    new_affect(&af);
    af.location = APPLY_RES_SOUND;
    af.modifier = amt;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }

  // ability scores
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_STR_INCREASE))
  {
    new_affect(&af);
    af.location = APPLY_STR;
    af.modifier = HAS_TEMP_EVOLUTION(ch, EVOLUTION_STR_INCREASE) * 2;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_CON_INCREASE))
  {
    new_affect(&af);
    af.location = APPLY_CON;
    af.modifier = HAS_TEMP_EVOLUTION(ch, EVOLUTION_CON_INCREASE) * 2;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_DEX_INCREASE))
  {
    new_affect(&af);
    af.location = APPLY_DEX;
    af.modifier = HAS_TEMP_EVOLUTION(ch, EVOLUTION_DEX_INCREASE) * 2;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_INT_INCREASE))
  {
    new_affect(&af);
    af.location = APPLY_INT;
    af.modifier = HAS_TEMP_EVOLUTION(ch, EVOLUTION_INT_INCREASE) * 2;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_WIS_INCREASE))
  {
    new_affect(&af);
    af.location = APPLY_WIS;
    af.modifier = HAS_TEMP_EVOLUTION(ch, EVOLUTION_WIS_INCREASE) * 2;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_CHA_INCREASE))
  {
    new_affect(&af);
    af.location = APPLY_CHA;
    af.modifier = HAS_TEMP_EVOLUTION(ch, EVOLUTION_CHA_INCREASE) * 2;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }

  // fast healing
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_FAST_HEALING))
  {
    new_affect(&af);
    af.location = APPLY_FAST_HEALING;
    af.modifier = HAS_TEMP_EVOLUTION(ch, EVOLUTION_FAST_HEALING) * 2;
    af.duration = mlev;
    af.bonus_type = BONUS_TYPE_RACIAL;
    af.spell = EIDOLON_MERGE_FORMS_EFFECT;
    affect_to_char(ch, &af);
  }

  // eidolon size
  if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_HUGE))
  {
    for (i = 0; i < 6; i++)
      new_affect(&(af2[i]));

    af2[0].location = APPLY_STR;
    af2[0].modifier = 16;
    af2[0].duration = mlev;
    af2[0].bonus_type = BONUS_TYPE_SIZE;
    af2[0].spell = EIDOLON_MERGE_FORMS_EFFECT;

    af2[0].location = APPLY_CON;
    af2[0].modifier = 8;
    af2[0].duration = mlev;
    af2[0].bonus_type = BONUS_TYPE_SIZE;
    af2[0].spell = EIDOLON_MERGE_FORMS_EFFECT;

    af2[0].location = APPLY_DEX;
    af2[0].modifier = -4;
    af2[0].duration = mlev;
    af2[0].bonus_type = BONUS_TYPE_SIZE;
    af2[0].spell = EIDOLON_MERGE_FORMS_EFFECT;

    af2[0].location = APPLY_SIZE;
    af2[0].modifier = 2;
    af2[0].duration = mlev;
    af2[0].bonus_type = BONUS_TYPE_SIZE;
    af2[0].spell = EIDOLON_MERGE_FORMS_EFFECT;

    for (i = 0; i < 6; i++)
      affect_to_char(ch, (&(af2[i])));
  }
  else if (HAS_TEMP_EVOLUTION(ch, EVOLUTION_LARGE))
  {
    for (i = 0; i < 6; i++)
      new_affect(&(af2[i]));

    af2[0].location = APPLY_STR;
    af2[0].modifier = 8;
    af2[0].duration = mlev;
    af2[0].bonus_type = BONUS_TYPE_SIZE;
    af2[0].spell = EIDOLON_MERGE_FORMS_EFFECT;

    af2[0].location = APPLY_CON;
    af2[0].modifier = 4;
    af2[0].duration = mlev;
    af2[0].bonus_type = BONUS_TYPE_SIZE;
    af2[0].spell = EIDOLON_MERGE_FORMS_EFFECT;

    af2[0].location = APPLY_DEX;
    af2[0].modifier = -2;
    af2[0].duration = mlev;
    af2[0].bonus_type = BONUS_TYPE_SIZE;
    af2[0].spell = EIDOLON_MERGE_FORMS_EFFECT;

    af2[0].location = APPLY_SIZE;
    af2[0].modifier = 1;
    af2[0].duration = mlev;
    af2[0].bonus_type = BONUS_TYPE_SIZE;
    af2[0].spell = EIDOLON_MERGE_FORMS_EFFECT;

    for (i = 0; i < 6; i++)
      affect_to_char(ch, (&(af2[i])));
  }
}

int num_evo_breaths(struct char_data *ch)
{

  if (!ch) return 0;

  if (HAS_EVOLUTION(ch, EVOLUTION_FIRE_BREATH))
    return HAS_EVOLUTION(ch, EVOLUTION_FIRE_BREATH);

  if (HAS_EVOLUTION(ch, EVOLUTION_COLD_BREATH))
    return HAS_EVOLUTION(ch, EVOLUTION_COLD_BREATH);

  if (HAS_EVOLUTION(ch, EVOLUTION_ACID_BREATH))
    return HAS_EVOLUTION(ch, EVOLUTION_ACID_BREATH);

  if (HAS_EVOLUTION(ch, EVOLUTION_ELECTRIC_BREATH))
    return HAS_EVOLUTION(ch, EVOLUTION_ELECTRIC_BREATH);

  return 0;
}

// returns true if the summoner's eidolon is in the same room as they
bool is_eidolon_in_room(struct char_data *ch)
{
  if (!ch)
    return false;
  if (IN_ROOM(ch) == NOWHERE)
    return false;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch) && tch->master == ch && MOB_FLAGGED(tch, MOB_EIDOLON))
      return true;
  }
  return false;
}

struct char_data *get_eidolon_in_room(struct char_data *ch)
{
  if (!ch)
    return NULL;
  if (IN_ROOM(ch) == NOWHERE)
    return NULL;

  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch) && tch->master == ch && MOB_FLAGGED(tch, MOB_EIDOLON))
      return tch;
  }
  return NULL;
}

ACMD(do_eidolon)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'}, buf[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *eidolon = NULL;
  char * desc = NULL;
  int i = 0, count = 0;

  half_chop_c(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!*arg)
  {
    send_to_char(ch, "Please use one of the following options:\r\n"
                     "-- evolutions - shows the evolutions you have learned.\r\n"
                     "-- aspects - shows the aspects you have learned.\r\n"
                     "-- shortdesc  - change the eidolon's short description.\r\n"
                     "-- longdesc   - change the eidolon's long description.\r\n"
                     "-- bondsenses - take control of your eidolon.  Type 'return' to return control to your character.\r\n"
                     "-- mergeforms - merge your eidolon with your own form and gain all of their evolutions.\r\n"
                     "-- mergedevos - shows the evolutions you have gained through the mergeforms ability.\r\n"
                     "\r\n"
                     "To call your eidolon type 'call eidolon'.  If your eidolon is not in your room, type 'summon'.\r\n");
    return;
  }

  if (is_abbrev(arg, "evolutions"))
  {
    send_to_char(ch, "Evolutions Known:\r\n");
    for (i = 1; i < NUM_EVOLUTIONS; i++)
    {
      if (KNOWS_EVOLUTION(ch, i))
      {
        count++;
        send_to_char(ch, "%-30s ", evolution_list[i].name);
        if (count % 2 == 0)
          send_to_char(ch, "\r\n");
      }
    }
    if (count % 2 == 1)
      send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Type: help (evolution name) for more info on specific evolutions.\r\n");
    send_to_char(ch, "\r\n");
    return;
  }

  if (is_abbrev(arg, "mergedevos"))
  {
    send_to_char(ch, "Merged Evolutions:\r\n");
    for (i = 1; i < NUM_EVOLUTIONS; i++)
    {
      if (HAS_TEMP_EVOLUTION(ch, i))
      {
        count++;
        send_to_char(ch, "%-30s ", evolution_list[i].name);
        if (count % 2 == 0)
          send_to_char(ch, "\r\n");
      }
    }
    if (count % 2 == 1)
      send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Type: help (evolution name) for more info on specific evolutions.\r\n");
    send_to_char(ch, "\r\n");
    return;
  }

  if (is_abbrev(arg, "aspects"))
  {
    send_to_char(ch, "Aspects Known:\r\n");
    for (i = 1; i < NUM_EVOLUTIONS; i++)
    {
      if (HAS_REAL_EVOLUTION(ch, i))
      {
        count++;
        send_to_char(ch, "%-30s ", evolution_list[i].name);
        if (count % 2 == 0)
          send_to_char(ch, "\r\n");
      }
    }
    if (count % 2 == 1)
      send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Type: help (aspect name) for more info on specific aspects.\r\n");
    send_to_char(ch, "\r\n");
    return;
  }

  if (!(eidolon = get_eidolon_in_room(ch)))
  {
    send_to_char(ch, "You can only use this command if you have summoner your eidolon and it is in the same room as you.\r\n"
                     "To call your eidolon type 'call eidolon'.  If your eidolon is not in your room, type 'summon'.\r\n");
    return;
  }

  if (is_abbrev(arg, "shortdesc"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Please specify what you would like the short desc to be as well.\r\n"
                       "Eg. eidolon shortdesc a massive black panther with deep purple eyes\r\n"
                       "Type 'eidolon shortdesc reset' to reset the eidolon short description to the default.\r\n");
      return;
    }

    if (strlen(arg2) > 60)
    {
      send_to_char(ch, "You cannot provide a short description length greater than 60.\r\n");
      return;
    }

    if (!strcmp(arg2, "reset"))
    {
      GET_EIDOLON_SHORT_DESCRIPTION(ch) = NULL;
      send_to_char(ch, "You've reset your eidolon short description.  This will be reflected next time you summon your eidolon.\r\n");
      return;
    }
    
    if(!valid_pet_name(arg2))
    {
      send_to_char(ch, "Pet descriptions cannot contain the \" or ; characters.\r\n");
      return;
    }

    strip_cr(arg2);

    desc = strdup(arg2);
    GET_EIDOLON_SHORT_DESCRIPTION(ch) = desc;
    GET_SHORT(eidolon) = desc;
    desc = strdup(arg2);
    (eidolon)->player.name = desc;
    send_to_char(ch, "You change your eidilon's short description to: %s\r\n", desc);
    return;
  }
  else if (is_abbrev(arg, "longdesc"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Please specify what you would like the long desc to be as well.\r\n"
                       "Eg. eidolon longdesc A massive black panther with deep purple eyes paces back and forth here.\r\n"
                       "Type 'eidolon longdesc reset' to reset the eidolon long description to the default.\r\n");
      return;
    }
    if (strlen(arg2) > 120)
    {
      send_to_char(ch, "You cannot provide a long description length greater than 120.\r\n");
      return;
    }
    if (!strcmp(arg2, "reset"))
    {
      GET_EIDOLON_LONG_DESCRIPTION(ch) = NULL;
      send_to_char(ch, "You've reset your eidolon long description.  This will be reflected next time you summon your eidolon.\r\n");
      return;
    }

    if(!valid_pet_name(arg2))
    {
      send_to_char(ch, "Pet descriptions cannot contain the \" or ; characters.\r\n");
      return;
    }

    strip_cr(arg2);

    snprintf(buf, sizeof(buf), "%s\r\n", arg2);

    desc = strdup(buf);

    GET_EIDOLON_LONG_DESCRIPTION(ch) = desc;
    eidolon->player.long_descr = desc;
    snprintf(buf, sizeof(buf), "%s\n", GET_EIDOLON_LONG_DESCRIPTION(ch));
    eidolon->player.description = strdup(buf);
    send_to_char(ch, "You change your eidilon's long description to: %s\r\n", arg2);
    return;
  }
  else if (is_abbrev(arg, "bondsenses"))
  {
    if (!HAS_FEAT(ch, FEAT_BOND_SENSES))
    {
      send_to_char(ch, "You don't have the ability to bond senses with an eidolon.\r\n");
      return;
    }
    send_to_char(ch, "You take control of your eidolon. (\tDType 'return' to return to your body\tn)\r\n");
    ch->desc->character = eidolon;
    ch->desc->original = ch;
    eidolon->desc = ch->desc;
    ch->desc = NULL;
  }
  else if (is_abbrev(arg, "mergeforms"))
  {
    if (!HAS_FEAT(ch, FEAT_MERGE_FORMS))
    {
      send_to_char(ch, "You do not know how to merge forms with your eidolon.\r\n");
      return;
    }
    
    // approx 10 minutes before they can call their eidolon again
    CALL_EIDOLON_COOLDOWN(ch) = 100;

    for (i = 1; i < NUM_EVOLUTIONS; i++)
    {
      if (HAS_EVOLUTION(eidolon, i) && evolution_list[i].pc_avail && !HAS_REAL_EVOLUTION(ch, i))
        HAS_TEMP_EVOLUTION(ch, i) = HAS_EVOLUTION(eidolon, i);
    }

    merge_eidolon_evolutions(ch);

    act("You draw $N's form into your own.", FALSE, ch, 0, eidolon, TO_CHAR);
    act("$n draws Your form into $s own.", FALSE, ch, 0, eidolon, TO_VICT);
    act("$n draws $N's form into $s own.", FALSE, ch, 0, eidolon, TO_NOTVICT);

    MERGE_FORMS_TIMER(ch) = GET_CALL_EIDOLON_LEVEL(ch);

    extract_char(eidolon);
    return;
  }
  else
  {
    send_to_char(ch, "Please use one of the following options:\r\n"
                     "-- evolutions - shows the evolutions you have learned.\r\n"
                     "-- aspects    - shows the aspects you have learned.\r\n"
                     "-- shortdesc  - change the eidolon's short description.\r\n"
                     "-- longdesc   - change the eidolon's long description.\r\n"
                     "-- bondsenses - take control of your eidolon.  Type 'return' to return control to your character.\r\n"
                     "-- mergeforms - merge your eidolon with your own form and gain all of their evolutions.\r\n"
                     "-- mergedevos - shows the evolutions you have gained through the mergeforms ability.\r\n"
                     "\r\n"
                     "To call your eidolon type 'call eidolon'.  If your eidolon is not in your room, type 'summon'\r\n");
    return;
  }
}

int get_shield_ally_bonus(struct char_data *ch)
{
  struct char_data *tch = NULL;
  int bonus = 0;

  if (IN_ROOM(ch) == NOWHERE)
    return 0;

  if (HAS_FEAT(ch, FEAT_GREATER_SHIELD_ALLY))
    bonus = 4;
  else if (HAS_FEAT(ch, FEAT_SHIELD_ALLY))
    bonus = 2;

  if (!get_eidolon_in_room(ch))
    return 0;

  if (get_eidolon_in_room(ch) && can_act(ch) && bonus > 0)
  {
    return bonus;
  }

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (GROUP(ch) == GROUP(tch) && get_eidolon_in_room(tch) && can_act(tch) && HAS_FEAT(tch, FEAT_GREATER_SHIELD_ALLY))
    {
      bonus = 4;
      break;
    }
  }

  return bonus;
}

int char_has_evolution(struct char_data *ch, int evo)
{
  if (HAS_REAL_EVOLUTION(ch, evo))
    return HAS_REAL_EVOLUTION(ch, evo);

  if (HAS_TEMP_EVOLUTION(ch, evo))
    return HAS_TEMP_EVOLUTION(ch, evo);

  return 0;
}

void display_evolution_requirements(struct char_data *ch, int evo)
{

  int i = 0, count = 0;
  int line_length = 80;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  if (evo < 1 || evo >= NUM_EVOLUTIONS)
  {
    return;
  }

  snprintf(buf, sizeof(buf), "\tnRequires %d evolution points.\r\n", evolution_list[evo].evolution_points);
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

  if (evolution_list[evo].stacks)
  {
    snprintf(buf, sizeof(buf), "\tnCan be taken multiple times once every %d summoner levels.\r\n", evolution_list[evo].stack_level);
    send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  }

  if (evolution_list[evo].requirement_type == EVOLUTION_REQ_TYPE_ALL)
  {
    snprintf(buf, sizeof(buf), "\tnRequires ALL of the following evolutions to take: ");
    for (i = 0; i < 6; i++)
    {
      if (evolution_list[evo].evolution_requirements[i] > 0)
      {
        snprintf(buf2, sizeof(buf2), "%s%s", count > 0 ? ", " : "", evolution_list[evolution_list[evo].evolution_requirements[i]].name);
        strlcat(buf, buf2, sizeof(buf));
        count++;
      }
    }
    send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  }

  if (evolution_list[evo].requirement_type == EVOLUTION_REQ_TYPE_ANY)
  {
    count = 0;
    snprintf(buf, sizeof(buf), "\tnRequires ANY of the following evolutions to take: ");
    for (i = 0; i < 6; i++)
    {
      if (evolution_list[evo].evolution_requirements[i] > 0)
      {
        snprintf(buf2, sizeof(buf2), "%s%s", count > 0 ? ", " : "", evolution_list[evolution_list[evo].evolution_requirements[i]].name);
        strlcat(buf, buf2, sizeof(buf));
        count++;
      }
    }
    send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  }

  if (evolution_list[evo].requirement_type == EVOLUTION_REQ_TYPE_UNIQUE)
  {
    count = 0;
    snprintf(buf, sizeof(buf), "\tnCANNOT BE TAKEN if any of the following evolutions has already been chosen: ");
    for (i = 0; i < 6; i++)
    {
      if (evolution_list[evo].evolution_requirements[i] > 0)
      {
        snprintf(buf2, sizeof(buf2), "%s%s", count > 0 ? ", " : "", evolution_list[evolution_list[evo].evolution_requirements[i]].name);
        strlcat(buf, buf2, sizeof(buf));
        count++;
      }
    }
    send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  }

  if (!evolution_list[evo].pc_avail)
  {
    snprintf(buf, sizeof(buf), "\tnCannot be taken as aspects by pcs; eidolons only.\r\n");
    send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  }
}

bool display_evolution_info(struct char_data *ch, const char *evoname)
{

  int line_length = 80;
  char buf[MAX_STRING_LENGTH];
  int evo = 0;

  skip_spaces_c(&evoname);
  evo = find_evolution_num(evoname);

  if (evo < 1 || evo >= NUM_EVOLUTIONS)
    return false;

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  snprintf(buf, sizeof(buf), "Evolution:\tn %s\r\n", evolution_list[evo].name);
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  send_to_char(ch, "\tC");
  snprintf(buf, sizeof(buf), "Description:\tn %s\r\n", evolution_list[evo].desc);
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  snprintf(buf, sizeof(buf), "Requirements:\tn\r\n");
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  display_evolution_requirements(ch, evo);
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  return TRUE;
}

int find_evolution_num(const char *name)
{
  int index, ok;
  const char *temp, *temp2;
  char first[256], first2[256];

  /* PHASE 1: Check for exact match first (case-insensitive) */
  for (index = 1; index < NUM_EVOLUTIONS; index++)
  {
    if (!strcasecmp(name, evolution_list[index].name))
      return (index);
  }

  /* PHASE 2: Try word-by-word matching for multi-word names */
  for (index = 1; index < NUM_EVOLUTIONS; index++)
  {
    ok = TRUE;
    /* It won't be changed, but other uses of this function elsewhere may. */
    temp = any_one_arg_c(evolution_list[index].name, first, sizeof(first));
    temp2 = any_one_arg_c(name, first2, sizeof(first2));
    while (*first && *first2 && ok)
    {
      if (!is_abbrev(first2, first))
        ok = FALSE;
      temp = any_one_arg_c(temp, first, sizeof(first));
      temp2 = any_one_arg_c(temp2, first2, sizeof(first2));
    }
    if (ok && !*first2 && !*first)
      return (index);
  }

  /* PHASE 3: Finally try abbreviation matching as fallback */
  for (index = 1; index < NUM_EVOLUTIONS; index++)
  {
    if (is_abbrev(name, evolution_list[index].name))
      return (index);
  }

  return (-1);
}

bool is_eidolon_base_form_evolution(int form, int evo)
{
  switch (form)
  {
  case EIDOLON_BASE_FORM_AVIAN:
    if (evo == EVOLUTION_CLAWS)
      return true;
    if (evo == EVOLUTION_FLIGHT)
      return true;
    break;
  case EIDOLON_BASE_FORM_BIPED:
    if (evo == EVOLUTION_CLAWS)
      return true;
    break;
  case EIDOLON_BASE_FORM_QUADRUPED:
    if (evo == EVOLUTION_BITE)
      return true;
    break;
  case EIDOLON_BASE_FORM_SERPENTINE:
    if (evo == EVOLUTION_BITE)
      return true;
    if (evo == EVOLUTION_TAIL_SLAP)
      return true;
    break;
  case EIDOLON_BASE_FORM_TAURIC:
    if (evo == EVOLUTION_HOOVES)
      return true;
  }
  return false;
}

void study_assign_eidolon_base_form(struct char_data *ch, int form)
{
  if (!ch || !LEVELUP(ch))
    return;

  // already chosen
  if (GET_EIDOLON_BASE_FORM(ch) || (LEVELUP(ch) && LEVELUP(ch)->eidolon_base_form))
    return;

  // not a valid form
  if (form <= EIDOLON_BASE_FORM_NONE || form >= NUM_EIDOLON_BASE_FORMS)
    return;

  int i = 0;

  for (i = 1; i < NUM_EVOLUTIONS; i++)
  {
    if (is_eidolon_base_form_evolution(form, i))
    {
      LEVELUP(ch)->eidolon_evolutions[i]++;
    }
  }
  LEVELUP(ch)->eidolon_base_form = form;
}

int study_num_aspects_chosen(struct descriptor_data *d)
{
  if (!d || !d->character) return 0;
  
  if (!LEVELUP(d->character)) return 0;

  int i = 0, num = 0;

  for (i = 1; i < NUM_EVOLUTIONS; i++)
  {
    if (LEVELUP(d->character)->summoner_aspects[i] > 0)
      num++;
  }

  return num;

}

bool study_has_aspects_unchosen(struct descriptor_data *d)
{
  if (!d || !d->character)
    return false;

  if (IS_NPC(d->character))
    return false;

  if (!LEVELUP(d->character))
    return false;

  // Needs to set this first
  if (GET_EIDOLON_BASE_FORM(d->character) == 0 && LEVELUP(d->character)->eidolon_base_form == 0)
    return false;

  int num_evos = HAS_REAL_FEAT(d->character, FEAT_ASPECT) + HAS_REAL_FEAT(d->character, FEAT_GREATER_ASPECT) + HAS_REAL_FEAT(d->character, FEAT_EPIC_ASPECT);
  int num_chosen = study_num_aspects_chosen(d);

  if ((num_evos - num_chosen) > 0) return true;

  return false;
}
