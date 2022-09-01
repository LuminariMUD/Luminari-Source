/* *************************************************************************
 *   File: spec_abilities.c                            Part of LuminariMUD *
 *  Usage: Code file for special abilities for weapons, armor and          *
 *         shields.                                                        *
 * Author: Ornir                                                           *
 ***************************************************************************
 *                                                                         *
 * In d20/Dungeons and Dragons, special abilities are what make magic      *
 * items -magical-.  These abilities, being wreathed in fire, exploding    *
 * with frost on a critical hit etc. are part of what defineds D&D.        *
 *                                                                         *
 * In order to implement these thing in LuminariMUD, some additions to the *
 * stock object model have been made (in structs.h).  These changes allow  *
 * the addition of any number of the defined special abilities to be added *
 * to the weapon, armor or shield in addition to any APPLY_ values that    *
 * the object has.  Additionally, an activation method must be defined.    *
 *                                                                         *
 * The code is defined similarly to the spells and commands in stock code, *
 * in that macros and an array of structures are used to define new        *
 * special abilities.                                                      *
 ***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "dg_event.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "class.h"
#include "fight.h"
#include "utils.h"
#include "mud_event.h"
#include "act.h" //perform_wildshapes
#include "mudlim.h"
#include "oasis.h" // mob autoroller
#include "assign_wpn_armor.h"
#include "feats.h"
#include "race.h"
#include "spec_abilities.h"
#include "domains_schools.h"

struct special_ability_info_type special_ability_info[NUM_SPECABS];

const char *unused_specabname = "!UNUSED!"; /* So we can get &unused_specabname */

const char *activation_methods[NUM_ACTIVATION_METHODS + 1] = {"None",
                                                              "On Wear",
                                                              "On Use",
                                                              "Command Word",
                                                              "On Hit",
                                                              "On Crit",
                                                              "\n"};

/* Procedures for loading and managing the special abilities on boot. */
static void add_weapon_special_ability(int specab, const char *name, int level, int actmtd, int targets, int violent, int time, int school, int cost, SPECAB_PROC_DEF(specab_proc))
{
  special_ability_info[specab].type = SPECAB_TYPE_WEAPON;
  special_ability_info[specab].level = level;
  special_ability_info[specab].activation_method = actmtd;
  special_ability_info[specab].targets = targets;
  special_ability_info[specab].violent = violent;
  special_ability_info[specab].name = name;
  special_ability_info[specab].time = time;
  special_ability_info[specab].school = school;
  special_ability_info[specab].cost = cost;
  special_ability_info[specab].special_ability_proc = specab_proc;
}

static void add_armor_special_ability(int specab, const char *name, int level, int actmtd, int targets, int violent, int time, int school, int cost, SPECAB_PROC_DEF(specab_proc))
{
  special_ability_info[specab].type = SPECAB_TYPE_ARMOR;
  special_ability_info[specab].level = level;
  special_ability_info[specab].activation_method = actmtd;
  special_ability_info[specab].targets = targets;
  special_ability_info[specab].violent = violent;
  special_ability_info[specab].name = name;
  special_ability_info[specab].time = time;
  special_ability_info[specab].school = school;
  special_ability_info[specab].cost = cost;
  special_ability_info[specab].special_ability_proc = specab_proc;
}

static void add_item_special_ability(int specab, const char *name, int level, int actmtd, int targets, int violent, int time, int school, int cost, SPECAB_PROC_DEF(specab_proc))
{
  special_ability_info[specab].type = SPECAB_TYPE_ITEM;
  special_ability_info[specab].level = level;
  special_ability_info[specab].activation_method = actmtd;
  special_ability_info[specab].targets = targets;
  special_ability_info[specab].violent = violent;
  special_ability_info[specab].name = name;
  special_ability_info[specab].time = time;
  special_ability_info[specab].school = school;
  special_ability_info[specab].cost = cost;
  special_ability_info[specab].special_ability_proc = specab_proc;
}

void daily_item_specab(int specab, event_id event, int daily_uses)
{
  special_ability_info[specab].daily_uses = daily_uses;
  special_ability_info[specab].event = event;
}

static void add_unused_special_ability(int specab)
{
  special_ability_info[specab].type = SPECAB_TYPE_NONE;
  special_ability_info[specab].level = 0;
  special_ability_info[specab].activation_method = 0;
  special_ability_info[specab].targets = 0;
  special_ability_info[specab].violent = 0;
  special_ability_info[specab].name = unused_specabname;
  special_ability_info[specab].time = 0;
  special_ability_info[specab].school = NOSCHOOL;
  special_ability_info[specab].cost = 0;
  special_ability_info[specab].daily_uses = 0;
  special_ability_info[specab].event = eNULL;
  special_ability_info[specab].special_ability_proc = NULL;
}

/**  (Targeting re-used from spells.h)
 **  TAR_IGNORE    : IGNORE TARGET.
 **  TAR_CHAR_ROOM : PC/NPC in room.
 **  TAR_CHAR_WORLD: PC/NPC in world.
 **  TAR_FIGHT_SELF: If fighting, and no argument, select tar_char as self.
 **  TAR_FIGHT_VICT: If fighting, and no argument, select tar_char as victim (fighting).
 **  TAR_SELF_ONLY : If no argument, select self, if argument check that it IS self.
 **  TAR_NOT_SELF  : Target is anyone else besides self.
 **  TAR_OBJ_INV   : Object in inventory.
 **  TAR_OBJ_ROOM  : Object in room.
 **  TAR_OBJ_WORLD : Object in world.
 **  TAR_OBJ_EQUIP : Object held.
 **/

void initialize_special_abilities(void)
{
  int i;

  /* Initialize all abilities to UNUSED. */
  /* Do not change the loops below. */
  for (i = 0; i < NUM_SPECABS; i++)
    add_unused_special_ability(i);
  /* Do not change the loop above. */

  add_item_special_ability(ITEM_SPECAB_HORN_OF_SUMMONING, "Horn of Summoning", 10, ACTMTD_USE | ACTMTD_WEAR,
                           TAR_IGNORE, FALSE, 0, CONJURATION, 1, item_specab_horn_of_summoning);

  daily_item_specab(ITEM_SPECAB_HORN_OF_SUMMONING, eITEM_SPECAB_HORN_OF_SUMMONING, 2);

  add_item_special_ability(ITEM_SPECAB_ITEM_SUMMON, "Summoning Item", 10, ACTMTD_USE | ACTMTD_WEAR,
                           TAR_IGNORE, FALSE, 0, CONJURATION, 1, item_specab_item_summon);

  daily_item_specab(ITEM_SPECAB_ITEM_SUMMON, eITEM_SPECAB_ITEM_SUMMON, 2);

  add_armor_special_ability(ARMOR_SPECAB_BLINDING, "Blinding (Armor)", 7, ACTMTD_COMMAND_WORD | ACTMTD_WEAR,
                            TAR_IGNORE, TRUE, 0, EVOCATION, 1, armor_specab_blinding);

  daily_item_specab(ARMOR_SPECAB_BLINDING, eARMOR_SPECAB_BLINDING, 2);

  add_weapon_special_ability(WEAPON_SPECAB_ADAPTIVE, "Adaptive", 12, ACTMTD_ON_HIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, TRANSMUTATION, 1, weapon_specab_adaptive);

  add_weapon_special_ability(WEAPON_SPECAB_AGILE, "Agile", 12, ACTMTD_ON_HIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, TRANSMUTATION, 1, weapon_specab_agile);

  add_weapon_special_ability(WEAPON_SPECAB_ANARCHIC, "Anarchic", 7, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 2, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_AXIOMATIC, "Axiomatic", 7, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 2, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_BANE, "Bane", 8, ACTMTD_ON_HIT | ACTMTD_ON_CRIT,
                             TAR_FIGHT_VICT, FALSE, 0, CONJURATION, 1, weapon_specab_bane);

  add_weapon_special_ability(WEAPON_SPECAB_BEWILDERING, "Bewildering", 8, ACTMTD_ON_CRIT,
                             TAR_FIGHT_VICT, FALSE, 0, ENCHANTMENT, 1, weapon_specab_bewildering);

  add_weapon_special_ability(WEAPON_SPECAB_BLINDING, "Blinding (Weapon)", 8, ACTMTD_ON_CRIT | ACTMTD_COMMAND_WORD,
                             TAR_FIGHT_VICT, FALSE, 0, CONJURATION, 1, weapon_specab_blinding);

  add_weapon_special_ability(WEAPON_SPECAB_BRILLIANT_ENERGY, "Brilliant Energy", 16, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, TRANSMUTATION, 4, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_CORROSIVE, "Corrosive", 10, ACTMTD_ON_HIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 1, weapon_specab_corrosive);

  add_weapon_special_ability(WEAPON_SPECAB_CORROSIVE_BURST, "Corrosive Burst", 12, ACTMTD_ON_HIT | ACTMTD_ON_CRIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 2, weapon_specab_corrosive_burst);

  add_weapon_special_ability(WEAPON_SPECAB_DANCING, "Dancing", 15, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, TRANSMUTATION, 4, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_DEFENDING, "Defending", 8, ACTMTD_ON_HIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, ABJURATION, 1, weapon_specab_defending);

  add_weapon_special_ability(WEAPON_SPECAB_DISRUPTION, "Disruption", 14, ACTMTD_ON_HIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, CONJURATION, 2, weapon_specab_disruption);

  add_weapon_special_ability(WEAPON_SPECAB_DISTANCE, "Distance", 6, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, DIVINATION, 1, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_EXHAUSTING, "Exhausting", 8, ACTMTD_ON_CRIT,
                             TAR_FIGHT_VICT, FALSE, 0, CONJURATION, 1, weapon_specab_exhausting);

  add_weapon_special_ability(WEAPON_SPECAB_FLAMING, "Flaming", 10, ACTMTD_ON_HIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 1, weapon_specab_flaming);

  add_weapon_special_ability(WEAPON_SPECAB_FLAMING_BURST, "Flaming Burst", 12, ACTMTD_ON_HIT | ACTMTD_ON_CRIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 2, weapon_specab_flaming_burst);

  add_weapon_special_ability(WEAPON_SPECAB_FROST, "Frost", 8, ACTMTD_ON_HIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 1, weapon_specab_frost);

  add_weapon_special_ability(WEAPON_SPECAB_GHOST_TOUCH, "Ghost Touch", 9, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, CONJURATION, 1, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_HOLY, "Holy", 7, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 2, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_ICY_BURST, "Icy Burst", 10, ACTMTD_ON_HIT | ACTMTD_ON_CRIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 2, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_INVIGORATING, "Invigorating", 18, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, NECROMANCY /* TRANSMUTATION TOO */, 5, weapon_specab_invigorating);

  add_weapon_special_ability(WEAPON_SPECAB_KEEN, "Keen", 10, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, TRANSMUTATION, 1, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_KI_FOCUS, "Ki Focus", 8, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, TRANSMUTATION, 1, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_LUCKY, "Lucky", 6, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, ENCHANTMENT, 1, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_MERCIFUL, "Merciful", 5, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, CONJURATION, 1, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_MIGHTY_CLEAVING, "Mighty Cleaving", 8, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 1, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_RETURNING, "Returning", 7, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, TRANSMUTATION, 1, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_SEEKING, "Seeking", 12, ACTMTD_ON_HIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, DIVINATION, 1, weapon_specab_seeking);

  add_weapon_special_ability(WEAPON_SPECAB_SHOCK, "Shock", 8, ACTMTD_ON_HIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 1, weapon_specab_shock);

  add_weapon_special_ability(WEAPON_SPECAB_SHOCKING_BURST, "Shocking Burst", 9, ACTMTD_ON_HIT | ACTMTD_ON_CRIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 2, weapon_specab_shocking_burst);

  add_weapon_special_ability(WEAPON_SPECAB_SPEED, "Speed", 7, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, TRANSMUTATION, 3, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_SPELL_STORING, "Spell Storing", 12, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 1, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_THUNDERING, "Thundering", 10, ACTMTD_ON_CRIT,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 1, weapon_specab_thundering);

  add_weapon_special_ability(WEAPON_SPECAB_THROWING, "Throwing", 5, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, TRANSMUTATION, 1, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_UNHOLY, "Unholy", 7, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 2, NULL);

  add_weapon_special_ability(WEAPON_SPECAB_VAMPIRIC, "Vampiric", 18, ACTMTD_ON_HIT | ACTMTD_ON_CRIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, NECROMANCY /* TRANSMUTATION TOO */, 5, weapon_specab_vampiric);

  add_weapon_special_ability(WEAPON_SPECAB_VICIOUS, "Vicious", 9, ACTMTD_ON_HIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, NECROMANCY, 1, weapon_specab_vicious);

  add_weapon_special_ability(WEAPON_SPECAB_VORPAL, "Vorpal", 18, ACTMTD_ON_CRIT | ACTMTD_COMMAND_WORD,
                             TAR_IGNORE, FALSE, 0, NECROMANCY /* TRANSMUTATION TOO */, 5, weapon_specab_vorpal);

  add_weapon_special_ability(WEAPON_SPECAB_WOUNDING, "Wounding", 10, ACTMTD_NONE,
                             TAR_IGNORE, FALSE, 0, EVOCATION, 2, weapon_specab_wounding);
}

bool obj_has_special_ability(struct obj_data *obj, int ability)
{
  struct obj_special_ability *specab = NULL;

  for (specab = obj->special_abilities; specab != NULL; specab = specab->next)
  {
    if (specab->ability == ability)
      return TRUE;
  }

  return FALSE;
}

struct obj_special_ability *get_obj_special_ability(struct obj_data *obj, int ability)
{
  struct obj_special_ability *specab = NULL;

  for (specab = obj->special_abilities; specab != NULL; specab = specab->next)
  {
    if (specab->ability == ability)
      return specab;
  }

  return NULL;
}

/* Returns the number of activated abilites. */
int process_weapon_abilities(struct obj_data *weapon,  /* The weapon to check for special abilities. */
                             struct char_data *ch,     /* The wielder of the weapon. */
                             struct char_data *victim, /* The target of the ability (either fighting or
                                                        * specified explicitly. */
                             int actmtd,               /* Activation method */
                             const char *cmdword)      /* Command word (optional, NULL if none. */
{
  int activated_abilities = 0;
  struct obj_special_ability *specab; /* struct for iterating through the object's abilities. */
  int alcFire = FALSE, alcBurst = FALSE;
  struct affected_type af;

  if (!weapon)
  {
    if (GET_EQ(ch, WEAR_HANDS))
      weapon = GET_EQ(ch, WEAR_HANDS);
    else
      return 0;
  }

  /* Run the 'callbacks' for each of the special abilities on weapon that match the activation method. */
  for (specab = weapon->special_abilities; specab != NULL; specab = specab->next)
  {
    /* Only deal with weapon special abilities */
    if (special_ability_info[specab->ability].type != SPECAB_TYPE_WEAPON)
      continue;
    /* So we have an ability, check the activation method. */
    if (IS_SET(specab->activation_method, actmtd))
    { /* Match! */
      if (actmtd == ACTMTD_COMMAND_WORD)
      {                                            /* check the command word */
        if (strcmp(specab->command_word, cmdword)) /* No Match */
          continue;                                /* Skip this ability, no match. */
      }
      if (special_ability_info[specab->ability].special_ability_proc == NULL)
      {
        log("SYSERR: PROCESS_WEAPON_ABILITIES: ability '%s' has no callback function!", special_ability_info[specab->ability].name);
        continue;
      }

      //  Fire brand bombs from alchemists
      if (affected_by_spell(ch, BOMB_AFFECT_FIRE_BRAND))
      {
        if (specab->ability == TYPE_SPECAB_FLAMING_BURST)
          alcBurst = TRUE;
        if (specab->ability == TYPE_SPECAB_FLAMING)
          alcFire = TRUE;
      }

      activated_abilities++;
      (*special_ability_info[specab->ability].special_ability_proc)(specab, weapon, ch, victim, actmtd);
    }
  }

  //  Fire brand bombs from alchemists
  if (affected_by_spell(ch, BOMB_AFFECT_FIRE_BRAND))
  {
    if (actmtd == ACTMTD_ON_CRIT && !alcBurst && CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 10)
    {
      damage(ch, victim, dice((weapon ? weapon_list[GET_OBJ_VAL(weapon, 0)].critMult - 1 : 1), 10), TYPE_SPECAB_FLAMING_BURST, DAM_FIRE, FALSE);
    }
    else if (actmtd == ACTMTD_ON_HIT && !alcFire && CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 0 && victim)
    {
      damage(ch, victim, dice(1, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
    }
  }
  //  Sun metal spell
  else if (affected_by_spell(ch, SPELL_SUN_METAL))
  {
    damage(ch, victim, dice(1, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
  }

  //  Paladin divine bond
  if (victim && HAS_FEAT(ch, FEAT_DIVINE_BOND))
  {
    if (actmtd == ACTMTD_ON_HIT)
    {
      if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 20)
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
      if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 10 && !IS_GOOD(victim))
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_HOLY, DAM_HOLY, FALSE);
    }
    if (actmtd == ACTMTD_ON_CRIT && CLASS_LEVEL(ch, CLASS_PALADIN) >= 30)
    {
      damage(ch, victim, dice(2, 10), TYPE_SPECAB_FLAMING_BURST, DAM_FIRE, FALSE);
    }
  }

  if (victim)
  {
    if (actmtd == ACTMTD_ON_HIT)
    {
      // flaming
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_FLAMING))
      {
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
      }
      // vicious
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_VICIOUS))
      {
        damage(ch, victim, dice(2, 6), TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE);
        damage(ch, ch, dice(1, 6), TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE);
      }
      // anarchic
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_ANARCHIC) && IS_LAWFUL(victim))
      {
        damage(ch, victim, dice(2, 6), TYPE_SPECAB_ANARCHIC, DAM_NEGATIVE, FALSE);
      }
      // unholy
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_UNHOLY) && IS_GOOD(victim))
      {
        damage(ch, victim, dice(2, 6), TYPE_SPECAB_UNHOLY, DAM_NEGATIVE, FALSE);
      }
      // wounding
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_WOUNDING) && !((GET_NPC_RACE(victim) == RACE_TYPE_CONSTRUCT) ||
                                                                (GET_NPC_RACE(victim) == RACE_TYPE_UNDEAD) || (GET_NPC_RACE(victim) == RACE_TYPE_OOZE)))
      {

        new_affect(&af);

        af.spell = TYPE_SPECAB_BLEEDING;
        af.location = APPLY_SPECIAL;
        af.modifier = dice(1, 4);
        af.duration = 3;
        af.bonus_type = BONUS_TYPE_UNDEFINED;
        SET_BIT_AR(af.bitvector, AFF_BLEED);

        if (AFF_FLAGGED(victim, AFF_BLEED))
        {
          act("Your bleeding worsens.", FALSE, victim, 0, ch, TO_CHAR);
          act("$n's bleeding worsens.", TRUE, victim, 0, ch, TO_ROOM);
        }
        else
        {
          act("You start to bleed.", FALSE, victim, 0, ch, TO_CHAR);
          act("$n starts to bleed.", TRUE, victim, 0, ch, TO_ROOM);
        }

        affect_to_char(victim, &af);
      }
    }
    if (actmtd == ACTMTD_ON_CRIT)
    {
      // flaming burst
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_FLAMING_BURST))
      {
        damage(ch, victim, dice((weapon ? weapon_list[GET_OBJ_VAL(weapon, 0)].critMult - 1 : 1), 10), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
      }
      // flaming
      else if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_FLAMING))
      {
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
      }
      // vicious
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_VICIOUS))
      {
        damage(ch, victim, dice(3, 6), TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE);
        damage(ch, ch, dice(1, 6), TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE);
      }
      // anarchic
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_ANARCHIC) && IS_LAWFUL(victim))
      {
        damage(ch, victim, dice(2, 6), TYPE_SPECAB_ANARCHIC, DAM_NEGATIVE, FALSE);
      }
      // unholy
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_UNHOLY) && IS_GOOD(victim))
      {
        damage(ch, victim, dice(2, 6), TYPE_SPECAB_UNHOLY, DAM_NEGATIVE, FALSE);
      }
      // wounding
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_WOUNDING) && !((GET_NPC_RACE(victim) == RACE_TYPE_CONSTRUCT) ||
                                                                (GET_NPC_RACE(victim) == RACE_TYPE_UNDEAD) || (GET_NPC_RACE(victim) == RACE_TYPE_OOZE)))
      {

        new_affect(&af);

        af.spell = TYPE_SPECAB_BLEEDING;
        af.location = APPLY_SPECIAL;
        af.modifier = dice(1, 4);
        af.duration = 3;
        af.bonus_type = BONUS_TYPE_UNDEFINED;
        SET_BIT_AR(af.bitvector, AFF_BLEED);

        if (AFF_FLAGGED(victim, AFF_BLEED))
        {
          act("Your bleeding worsens.", FALSE, victim, 0, ch, TO_CHAR);
          act("$n's bleeding worsens.", TRUE, victim, 0, ch, TO_ROOM);
        }
        else
        {
          act("You start to bleed.", FALSE, victim, 0, ch, TO_CHAR);
          act("$n starts to bleed.", TRUE, victim, 0, ch, TO_ROOM);
        }

        affect_to_char(victim, &af);
      }
      // vorpal
      if (FIENDISH_BOON_ACTIVE(ch, FIENDISH_BOON_VORPAL))
      {
        if (dice(1, 20) == 1)
        { // 5% chance on a critical hit
          if ((GET_NPC_RACE(victim) != RACE_TYPE_UNDEAD) &&
              (GET_NPC_RACE(victim) != RACE_TYPE_CONSTRUCT) &&
              (GET_NPC_RACE(victim) != RACE_TYPE_OOZE))
          { // they need to have or a head or not be able to function without a head
            if (!MOB_FLAGGED(victim, MOB_NOCHARM))
            {                                                                                       // a fail safe for boss type mobs and shopkeepers, etc.
              damage(ch, victim, GET_HIT(victim) + 100, TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE); // should kill them outright
            }
          }
        }
      }
    }
  }

  if (HAS_FEAT(ch, FEAT_HOLY_CHAMPION) && victim && IS_OUTSIDER(victim) && IS_EVIL(victim) &&
      (!IS_NPC(victim) || (!MOB_FLAGGED(victim, MOB_NOCHARM) && GET_MAX_HIT(victim) < 1000)))
  {
    if (victim->player_specials->has_banishment_been_attempted)
      ;
    else if (mag_resistance(ch, victim, 0))
      victim->player_specials->has_banishment_been_attempted = true;
    else if (mag_savingthrow(ch, victim, SAVING_WILL, 0, CAST_WEAPON_SPELL, CLASS_LEVEL(ch, CLASS_PALADIN), SCHOOL_NOSCHOOL))
      victim->player_specials->has_banishment_been_attempted = true;
    else
    {
      damage(ch, victim, GET_HIT(victim) * 10, TYPE_SPECAB_HOLY, DAM_HOLY, FALSE); // should kill them outright
      act("You have banished $N!", FALSE, ch, 0, victim, TO_CHAR);
      act("$n has banished YOU!", FALSE, ch, 0, victim, TO_VICT);
      act("$n has banished $N!", FALSE, ch, 0, victim, TO_NOTVICT);
    }
  }

  if (HAS_FEAT(ch, FEAT_UNHOLY_CHAMPION) && victim && IS_OUTSIDER(victim) && IS_GOOD(victim) &&
      (!IS_NPC(victim) || (!MOB_FLAGGED(victim, MOB_NOCHARM) && GET_MAX_HIT(victim) < 1000)))
  {
    if (victim->player_specials->has_banishment_been_attempted)
      ;
    else if (mag_resistance(ch, victim, 0))
      victim->player_specials->has_banishment_been_attempted = true;
    else if (mag_savingthrow(ch, victim, SAVING_WILL, 0, CAST_WEAPON_SPELL, CLASS_LEVEL(ch, CLASS_BLACKGUARD), SCHOOL_NOSCHOOL))
      victim->player_specials->has_banishment_been_attempted = true;
    else
    {
      damage(ch, victim, GET_HIT(victim) * 10, TYPE_SPECAB_UNHOLY, DAM_UNHOLY, FALSE); // should kill them outright
      act("You have banished $N!", FALSE, ch, 0, victim, TO_CHAR);
      act("$n has banished YOU!", FALSE, ch, 0, victim, TO_VICT);
      act("$n has banished $N!", FALSE, ch, 0, victim, TO_NOTVICT);
    }
  }

  return activated_abilities;
}

int process_armor_abilities(struct char_data *ch,     /* The player wearing the armor. */
                            struct char_data *victim, /* The target of the ability (either fighting or specified explicitly. */
                            int actmtd,               /* Activation method */
                            const char *cmdword)      /* Command word (optional, NULL if none. */
{
  int i = 0;
  int activated_abilities = 0;
  struct obj_data *obj;

  /* Check every piece of armor/equipment that the player is wearing. */
  for (i = 0; i < NUM_WEARS; i++)
  {

    if ((i == WEAR_WIELD_1) ||
        (i == WEAR_WIELD_OFFHAND) ||
        (i == WEAR_WIELD_2H))
    {
      /* Skip weapons */
      continue;
    }

    obj = GET_EQ(ch, i);
    if (obj != NULL)
    {
      struct obj_special_ability *specab; /* struct for iterating through the object's abilities. */
      /* Run the 'callbacks' for each of the special abilities on the object that match the activation method. */
      for (specab = obj->special_abilities; specab != NULL; specab = specab->next)
      {
        /* Only deal with armor special abilities */
        if (special_ability_info[specab->ability].type != SPECAB_TYPE_ARMOR)
          continue;

        /* So we have an ability, check the activation method. */
        if (IS_SET(specab->activation_method, actmtd))
        { /* Match! */
          if (actmtd == ACTMTD_COMMAND_WORD)
          {                                            /* check the command word */
            if (strcmp(specab->command_word, cmdword)) /* No Match */
              continue;                                /* Skip this ability, no match. */
          }
          if (special_ability_info[specab->ability].special_ability_proc == NULL)
          {
            log("SYSERR: PROCESS_ARMOR_ABILITIES: ability '%s' has no callback function!", special_ability_info[specab->ability].name);
            continue;
          }
          activated_abilities++;
          (*special_ability_info[specab->ability].special_ability_proc)(specab, obj, ch, victim, actmtd);
        }
      }
    }
  }
  return activated_abilities;
}

/* Returns the number of activated abilites. */
int process_item_abilities(struct obj_data *obj,     /* The obj to check for special abilities. */
                           struct char_data *ch,     /* The wielder of the weapon. */
                           struct char_data *victim, /* The target of the ability (either fighting or
                                                      * specified explicitly. */
                           int actmtd,               /* Activation method */
                           char *cmdword)            /* Command word (optional, NULL if none. */
{
  int activated_abilities = 0;
  struct obj_special_ability *specab; /* struct for iterating through the object's abilities. */
  /* Run the 'callbacks' for each of the special abilities on weapon that match the activation method. */
  for (specab = obj->special_abilities; specab != NULL; specab = specab->next)
  {
    /* Only deal with weapon special abilities */
    if (special_ability_info[specab->ability].type != SPECAB_TYPE_ITEM)
      continue;
    /* So we have an ability, check the activation method. */
    if (IS_SET(specab->activation_method, actmtd))
    { /* Match! */
      if (actmtd == ACTMTD_COMMAND_WORD)
      {                                            /* check the command word */
        if (strcmp(specab->command_word, cmdword)) /* No Match */
          continue;                                /* Skip this ability, no match. */
      }
      if (special_ability_info[specab->ability].special_ability_proc == NULL)
      {
        log("SYSERR: PROCESS_ITEM_ABILITIES: ability '%s' has no callback function!", special_ability_info[specab->ability].name);
        continue;
      }
      activated_abilities++;
      (*special_ability_info[specab->ability].special_ability_proc)(specab, obj, ch, victim, actmtd);
    }
  }

  return activated_abilities;
}

ITEM_SPECIAL_ABILITY(item_specab_horn_of_summoning)
{
  /* specab
   * level
   * obj
   * ch
   * victim
   */
  struct char_data *mob = NULL;
  mob_vnum mob_num = 0;
  int temp_level = 0;

  switch (actmtd)
  {
  case ACTMTD_USE: /* User USEs the item. */

    if (daily_item_specab_uses_remaining(obj, ITEM_SPECAB_HORN_OF_SUMMONING) == 0)
    {
      /* No uses remaining... */
      send_to_char(ch, "The item must regain its energies before invoking this ability.\r\n");
      break;
    }

    if (!IN_ROOM(ch))
      break;

    /* start off with some possible fail conditions */
    if (AFF_FLAGGED(ch, AFF_CHARM))
    {
      send_to_char(ch, "You are too giddy to have any followers!\r\n");
      break;
    }

    if (HAS_PET(ch))
    {
      send_to_char(ch, "You can't control more followers!\r\n");
      break;
    }
    mob_num = specab->value[0]; /* Val 0 is mob VNUM */

    /* Display the message for the ability. */
    act("You bring your $o to your mouth and blow.", FALSE, ch, obj, ch, TO_CHAR);
    act("$N brings $S $o to $S mouth and blows.", TRUE, ch, obj, ch, TO_ROOM);

    /* Echo to the zone. */
    send_to_zone("The single clarion note of a horn reverberates throughout the area.\r\n", world[IN_ROOM(ch)].zone);

    if (!(mob = read_mobile(mob_num, VIRTUAL)))
    {
      send_to_char(ch, "You don't quite remember how to make that creature.\r\n");
      return;
    }

    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
    {
      X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
      Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
    }

    char_to_room(mob, IN_ROOM(ch));
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);

    temp_level = MIN(GET_LEVEL(ch), GET_LEVEL(mob));
    GET_LEVEL(mob) = MIN(20, temp_level);
    autoroll_mob(mob, TRUE, TRUE);
    GET_LEVEL(mob) = temp_level;

    /* summon augmentation feat */
    if (HAS_FEAT(ch, FEAT_AUGMENT_SUMMONING))
    {
      send_to_char(ch, "*augmented* ");
      GET_REAL_STR(mob) = (mob)->aff_abils.str += 4;
      GET_REAL_CON(mob) = (mob)->aff_abils.con += 4;
      GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob) += 2 * GET_LEVEL(mob); /* con bonus */
    }

    act("$N glides into the area, seemingly from nowhere!", FALSE, ch, 0, mob, TO_ROOM);
    act("$N glides into the area, seemingly from nowhere!", FALSE, ch, 0, mob, TO_CHAR);

    load_mtrigger(mob);
    add_follower(mob, ch);

    if (GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
      join_group(mob, GROUP(ch));

    start_item_specab_daily_use_cooldown(obj, ITEM_SPECAB_HORN_OF_SUMMONING);

    break;
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
    break;
  case ACTMTD_ON_HIT: /* Called whenever a weapon hits an enemy. */
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    break;
  case ACTMTD_WEAR: /* Called whenever the item is worn. */
    if (!ch->mute_equip_messages)
      send_to_char(ch, "The horn speaks in your mind, \"%s\"\r\n",
                   (daily_item_specab_uses_remaining(obj, ITEM_SPECAB_HORN_OF_SUMMONING) == 0 ? "Your companion is too tired to answer your summons."
                                                                                              : "Your companion is ready to answer your summons!"));
    break;
  default:
    /* Do nothing. */
    break;
  }
}

ITEM_SPECIAL_ABILITY(item_specab_item_summon)
{
  /* specab
   * level
   * obj
   * ch
   * victim
   */
  struct char_data *mob = NULL;
  mob_vnum mob_num = 0;
  int temp_level = 0;

  switch (actmtd)
  {
  case ACTMTD_USE: /* User USEs the item. */

    if (daily_item_specab_uses_remaining(obj, ITEM_SPECAB_ITEM_SUMMON) == 0)
    {
      /* No uses remaining... */
      send_to_char(ch, "The item must regain its energies before invoking this ability.\r\n");
      break;
    }

    if (!IN_ROOM(ch))
      break;

    /* start off with some possible fail conditions */
    if (AFF_FLAGGED(ch, AFF_CHARM))
    {
      send_to_char(ch, "You are too giddy to have any followers!\r\n");
      break;
    }

    if (HAS_PET(ch))
    {
      send_to_char(ch, "You can't control more followers!\r\n");
      break;
    }
    mob_num = specab->value[0]; /* Val 0 is mob VNUM */

    /* Display the message for the ability. */
    act("You raise $o high to invokes its power....", FALSE, ch, obj, ch, TO_CHAR);
    act("$N raises $S $o high in $S hands, invoking its power...", TRUE, ch, obj, ch, TO_ROOM);

    /* Echo to the zone. */
    send_to_zone("Thundering sound of conjuring power reverberates throughout the area.\r\n", world[IN_ROOM(ch)].zone);

    if (!(mob = read_mobile(mob_num, VIRTUAL)))
    {
      send_to_char(ch, "You don't quite remember how to make that creature (report to STAFF).\r\n");
      return;
    }

    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
    {
      X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
      Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
    }

    char_to_room(mob, IN_ROOM(ch));
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);

    temp_level = MIN(GET_LEVEL(ch), GET_LEVEL(mob));
    GET_LEVEL(mob) = MIN(20, temp_level);
    autoroll_mob(mob, TRUE, TRUE);
    GET_LEVEL(mob) = temp_level;

    /* summon augmentation feat */
    if (HAS_FEAT(ch, FEAT_AUGMENT_SUMMONING))
    {
      send_to_char(ch, "*augmented* ");
      GET_REAL_STR(mob) = (mob)->aff_abils.str += 4;
      GET_REAL_CON(mob) = (mob)->aff_abils.con += 4;
      GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob) += 2 * GET_LEVEL(mob); /* con bonus */
    }

    act("$N glides into the area, seemingly from nowhere!", FALSE, ch, 0, mob, TO_ROOM);
    act("$N glides into the area, seemingly from nowhere!", FALSE, ch, 0, mob, TO_CHAR);

    load_mtrigger(mob);
    add_follower(mob, ch);

    if (GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
      join_group(mob, GROUP(ch));

    start_item_specab_daily_use_cooldown(obj, ITEM_SPECAB_ITEM_SUMMON);

    break;
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
    break;
  case ACTMTD_ON_HIT: /* Called whenever a weapon hits an enemy. */
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    break;
  case ACTMTD_WEAR: /* Called whenever the item is worn. */
    if (!ch->mute_equip_messages)
      send_to_char(ch, "The horn speaks in your mind, \"%s\"\r\n",
                   (daily_item_specab_uses_remaining(obj, ITEM_SPECAB_ITEM_SUMMON) == 0 ? "Your companion is too tired to answer your summons."
                                                                                        : "Your companion is ready to answer your summons!"));
    break;
  default:
    /* Do nothing. */
    break;
  }
}

ARMOR_SPECIAL_ABILITY(armor_specab_blinding)
{
  /*
   * level
   * armor
   * ch
   * victim
   */
  struct char_data *tch = NULL;
  bool found = FALSE;
  struct affected_type af[2];
  int i = 0;

  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
    /* Activate the blinding ability.
     *  - Check the cooldown - This ability can be used 2x a day, so set a cooldown on the shield using events.
     *  - Send a message to the room, then attempt to blind engaged creatures.
     */
    if (daily_item_specab_uses_remaining(armor, ARMOR_SPECAB_BLINDING) == 0)
    {
      /* No uses remaining... */
      send_to_char(ch, "The item must regain its energies before invoking this ability.\r\n");
      break;
    }

    if (!IN_ROOM(ch))
      break;

    /* Display the message for the ability. */
    act("Your $o flashes brightly, bathing the area in intense light!", FALSE, ch, armor, ch, TO_CHAR);
    act("$N's $o flashes brightly, bathing the area in intense light!", TRUE, ch, armor, ch, TO_ROOM);

    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
      if (FIGHTING(tch) != ch)
      {
        continue;
      }

      found = TRUE;
      if (!can_blind(tch))
      {
        continue;
      }

      if (mag_savingthrow(ch, tch, SAVING_REFL, 0, CAST_WEAPON_SPELL, 10, SCHOOL_NOSCHOOL))
      {
        act("You look away just in time to avoid getting blinded!", FALSE, tch, armor, ch, TO_CHAR);
        act("$n looks away just in time to avoid getting blinded!", TRUE, tch, armor, ch, TO_ROOM);
        continue;
      }

      af[0].spell = SPELL_BLINDNESS;
      af[0].location = APPLY_HITROLL;
      af[0].modifier = -4;
      af[0].duration = 4;
      af[0].bonus_type = BONUS_TYPE_UNDEFINED;
      SET_BIT_AR(af[0].bitvector, AFF_BLIND);

      af[1].spell = SPELL_BLINDNESS;
      af[1].location = APPLY_AC_NEW;
      af[1].modifier = -4;
      af[1].duration = 4;
      af[1].bonus_type = BONUS_TYPE_UNDEFINED;
      SET_BIT_AR(af[1].bitvector, AFF_BLIND);

      act("You have been blinded!", FALSE, tch, armor, ch, TO_CHAR);
      act("$n seems to be blinded!", TRUE, tch, 0, ch, TO_ROOM);

      for (i = 0; i < 2; i++)
      {
        if (af[i].bitvector[0] || af[i].bitvector[1] ||
            af[i].bitvector[2] || af[i].bitvector[3] ||
            (af[i].location != APPLY_NONE))
        {
          affect_join(tch, af + i, FALSE, FALSE, FALSE, FALSE);
        }
      }
    }

    if (!found)
    {
      send_to_char(ch, "No enemies are engaged with you!\r\n");
    }

    start_item_specab_daily_use_cooldown(armor, ARMOR_SPECAB_BLINDING);

    break;
  case ACTMTD_USE: /* User USEs the item. */
    break;
  case ACTMTD_ON_HIT: /* Called whenever a weapon hits an enemy. */
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    break;
  case ACTMTD_WEAR: /* Called whenever the item is worn. */
    if (!ch->mute_equip_messages)
      send_to_char(ch, "The shield speaks in your mind, \"I will blind your enemies!  Utter 'Lumia'!  FOR LUMIA!\"\r\n");
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_flaming)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */

  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the flaming ability.
     *  - Set the FLAMING bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_FLAMING))
    {
      /* Flaming is on, turn it off. */
      send_to_char(ch, "The magical flames wreathing your weapon vanish.\r\n");
      act("The magical flames wreathing $n's $o vanish.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_FLAMING);
    }
    else
    {
      /* FLAME ON! */
      send_to_char(ch, "Magical flames spread down the length of your weapon!\r\n");
      act("Magical flames spread down the length of $n's $o!", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_FLAMING);
    }
    break;
  case ACTMTD_ON_HIT:                      /* Called whenever a weapon hits an enemy. */
    if (OBJ_FLAGGED(weapon, ITEM_FLAMING)) /* Burn 'em. */
      if (victim)
      {
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
      }
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
  case ACTMTD_WEAR:    /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

/* A weapon with Flaming burst functions as a flaming weapon, except on critical hits it
 * performs a flame burst for 1d10 extra damage. */
WEAPON_SPECIAL_ABILITY(weapon_specab_flaming_burst)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the flaming ability.
     *  - Set the FLAMING bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_FLAMING))
    {
      /* Flaming is on, turn it off. */
      send_to_char(ch, "The magical flames wreathing your weapon vanish.\r\n");
      act("The magical flames wreathing $n's $o vanish.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_FLAMING);
    }
    else
    {
      /* FLAME ON! */
      send_to_char(ch, "Magical flames spread down the length of your weapon!\r\n");
      act("Magical flames spread down the length of $n's $o!", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_FLAMING);
    }
    break;
  case ACTMTD_ON_HIT:                      /* Called whenever a weapon hits an enemy. */
    if (OBJ_FLAGGED(weapon, ITEM_FLAMING)) /* Burn 'em. */
      if (victim)
      {
        /*send_to_char(ch, "\tr[spcab]\tn");*/
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_FLAMING, DAM_FIRE, FALSE);
      }
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    /* We don't care if the flaming property is active, it bursts anyway! */
    if (victim)
    {
      /* send_to_char(ch,"\tr[burst]\tn");*/
      damage(ch, victim, dice((weapon ? weapon_list[GET_OBJ_VAL(weapon, 0)].critMult - 1 : 1), 10), TYPE_SPECAB_FLAMING_BURST, DAM_FIRE, FALSE);
    }
    break;
  case ACTMTD_WEAR: /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_corrosive_burst)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the CORROSIVE ability.
     *  - Set the CORROSIVE bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_CORROSIVE))
    {
      /* CORROSIVE is on, turn it off. */
      send_to_char(ch, "The magical acid dripping off your weapon vanish.\r\n");
      act("The magical acid dripping off $n's $o vanish.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_CORROSIVE);
    }
    else
    {
      /* FLAME ON! */
      send_to_char(ch, "Magical acid starts dripping down the length of your weapon!\r\n");
      act("Magical acid starts dripping down the length of $n's $o!", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_CORROSIVE);
    }
    break;
  case ACTMTD_ON_HIT:                        /* Called whenever a weapon hits an enemy. */
    if (OBJ_FLAGGED(weapon, ITEM_CORROSIVE)) /* Burn 'em. */
      if (victim)
      {
        /*send_to_char(ch, "\tr[spcab]\tn");*/
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_CORROSIVE, DAM_ACID, FALSE);
      }
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    /* We don't care if the CORROSIVE property is active, it bursts anyway! */
    if (victim)
    {
      /* send_to_char(ch,"\tr[burst]\tn");*/
      damage(ch, victim, dice((weapon ? weapon_list[GET_OBJ_VAL(weapon, 0)].critMult - 1 : 1), 10), TYPE_SPECAB_CORROSIVE, DAM_ACID, FALSE);
    }
    break;
  case ACTMTD_WEAR: /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_vicious)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD:
  case ACTMTD_USE:
    if (OBJ_FLAGGED(weapon, ITEM_VICIOUS))
    {
      send_to_char(ch, "The tiny whirls of black smoke surrounding your weapon dissipate.\r\n");
      act("The tiny whirls of black smoke surrounding $n's weapon dissipate.", FALSE, ch, weapon, NULL, TO_ROOM);
      REMOVE_OBJ_FLAG(weapon, ITEM_VICIOUS);
    }
    else
    {
      send_to_char(ch, "Tiny whirls of black smoke suddenly surround your weapon.\r\n");
      act("Tiny whirls of black smoke suddenly surround $n's weapon.", FALSE, ch, weapon, NULL, TO_ROOM);
      SET_OBJ_FLAG(weapon, ITEM_VICIOUS);
    }
    break;
  case ACTMTD_ON_HIT:
    if (OBJ_FLAGGED(weapon, ITEM_VICIOUS))
      if (victim)
      {
        damage(ch, victim, dice(2, 6), TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE);
        damage(ch, ch, dice(1, 6), TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE);
      }
    break;
  case ACTMTD_ON_CRIT:
    if (victim)
    {
      damage(ch, victim, dice(3, 6), TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE);
      damage(ch, ch, dice(1, 6), TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE);
    }
    break;
  case ACTMTD_WEAR:
  default:
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_vorpal)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD:
  case ACTMTD_USE:
    if (OBJ_FLAGGED(weapon, ITEM_VORPAL))
    {
      send_to_char(ch, "The tiny whirls of black smoke surrounding your weapon dissipate.\r\n");
      act("The tiny whirls of black smoke surrounding $n's weapon dissipate.", FALSE, ch, weapon, NULL, TO_ROOM);
      REMOVE_OBJ_FLAG(weapon, ITEM_VORPAL);
    }
    else
    {
      send_to_char(ch, "Tiny whirls of black smoke suddenly surround your weapon.\r\n");
      act("Tiny whirls of black smoke suddenly surround $n's weapon.", FALSE, ch, weapon, NULL, TO_ROOM);
      SET_OBJ_FLAG(weapon, ITEM_VORPAL);
    }
    break;
  case ACTMTD_ON_CRIT:
    if (victim)
    {
      if (dice(1, 20) == 1)
      { // 5% chance on a critical hit
        if ((GET_NPC_RACE(victim) != RACE_TYPE_UNDEAD) &&
            (GET_NPC_RACE(victim) != RACE_TYPE_CONSTRUCT) &&
            (GET_NPC_RACE(victim) != RACE_TYPE_OOZE))
        { // they need to have or a head or not be able to function without a head
          if (!MOB_FLAGGED(victim, MOB_NOCHARM))
          {                                                                                       // a fail safe for boss type mobs and shopkeepers, etc.
            damage(ch, victim, GET_HIT(victim) + 100, TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE); // should kill them outright
          }
        }
      }
    }
    break;
  case ACTMTD_ON_HIT:
  case ACTMTD_WEAR:
  default:
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_vampiric)
{
  int dam = 0;
  int heal = 0;
  // char buf[200]; // uncomment if we decide we want to show a message for hp healed.  Commented out because considered too spammy
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_ON_HIT:
  case ACTMTD_ON_CRIT:
    if (victim)
    {
      if ((GET_NPC_RACE(victim) != RACE_TYPE_UNDEAD) &&
          (GET_NPC_RACE(victim) != RACE_TYPE_CONSTRUCT))
      { // has to be alive

        if (actmtd == ACTMTD_ON_HIT)
          dam = dice(1, 8);
        else /* crit */
          dam = dice(2, 8) + 2;

        /* hit them with the vampiric damage first and store the result in the heal variable */
        heal = damage(ch, victim, dam, TYPE_SPECAB_BLEEDING, DAM_NEGATIVE, FALSE);

        if (heal > 0)
        {
          /* too powerful */
          // heal += (int)(compute_damage_bonus(ch, victim, weapon, ATTACK_TYPE_PRIMARY, 0, MODE_NORMAL_HIT, ATTACK_TYPE_PRIMARY) / 10);
          process_healing(ch, ch, -1, heal, 0);
          // snprintf(buf, sizeof(buf), "Your $o has healed you %d hit points!", heal);
          // act(buf, false, ch, weapon, 0, TO_CHAR);
        }
      }
    }
    break;
  case ACTMTD_COMMAND_WORD:
  case ACTMTD_USE:
  case ACTMTD_WEAR:
  default:
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_invigorating)
{

  int stamina = GET_OBJ_VAL(weapon, 4);

  // char buf[200]; // uncomment if we decide we want to show a message for hp healed.  Commented out because considered too spammy
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_ON_HIT:
  case ACTMTD_ON_CRIT:
    if (victim)
    {
      if (actmtd == ACTMTD_ON_CRIT)
        stamina *= 3;
      GET_MOVE(ch) += stamina / 2;
      GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch));
    }
    break;
  case ACTMTD_COMMAND_WORD:
  case ACTMTD_USE:
  case ACTMTD_WEAR:
  default:
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_corrosive)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the CORROSIVE ability.
     *  - Set the CORROSIVE bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_CORROSIVE))
    {
      /* CORROSIVE is on, turn it off. */
      send_to_char(ch, "The magical acid dripping off your weapon vanish.\r\n");
      act("The magical acid dripping off $n's $o vanish.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_CORROSIVE);
    }
    else
    {
      /* FLAME ON! */
      send_to_char(ch, "Magical acid starts dripping down the length of your weapon!\r\n");
      act("Magical acid starts dripping down the length of $n's $o!", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_CORROSIVE);
    }
    break;
  case ACTMTD_ON_HIT:                        /* Called whenever a weapon hits an enemy. */
    if (OBJ_FLAGGED(weapon, ITEM_CORROSIVE)) /* Burn 'em. */
      if (victim)
      {
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_CORROSIVE, DAM_ACID, FALSE);
      }
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
  case ACTMTD_WEAR:    /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

/* A weapon wne prints messages when fighting it's favored enemy... */
WEAPON_SPECIAL_ABILITY(weapon_specab_bane)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_ON_HIT: /* Called whenever a weapon hits an enemy. */
    if ((dice(1, 6) > 4) && ((GET_RACE(victim) == specab->value[0]) && (HAS_SUBRACE(victim, specab->value[1]))))
    {

      act("Your $o hums happily as you fight $N!", FALSE, ch, weapon, victim, TO_CHAR);
      act("$n's $o hums happily as $e fights you!", FALSE, ch, weapon, victim, TO_VICT);
      act("$n's $o hums happily as $e fights $N!", FALSE, ch, weapon, victim, TO_NOTVICT);
    }
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    if ((GET_RACE(victim) == specab->value[0]) && (HAS_SUBRACE(victim, specab->value[1])))
    {
      act("Waves of pleasure course into you from your $o as you strike $N!", FALSE, ch, weapon, victim, TO_CHAR);
    }
    break;
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_disruption)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the DISRUPTION ability.
     *  - Set the DISRUPTION bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_DISRUPTION))
    {
      /* Flaming is on, turn it off. */
      send_to_char(ch, "The field of holy energy on your weapon dissipates.\r\n");
      act("The field of holy energy on $n's weapon dissipates.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_DISRUPTION);
    }
    else
    {
      /* DISRUPTION ON! */
      send_to_char(ch, "A field of holy energy envelops your weapon.\r\n");
      act("A field of holy energy envelops $n's weapon.", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_DISRUPTION);
    }
    break;
  case ACTMTD_ON_HIT: /* Called whenever a weapon hits an enemy. */
    if (GET_RACE(victim) == RACE_TYPE_UNDEAD)
    {
      if (OBJ_FLAGGED(weapon, ITEM_DISRUPTION))
        if (victim)
        {
          damage(ch, victim, dice(2, 6), TYPE_SPECAB_HOLY, DAM_HOLY, FALSE);
        }
    }
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    if (GET_RACE(victim) == RACE_TYPE_UNDEAD)
    {
      if (OBJ_FLAGGED(weapon, ITEM_DISRUPTION))
        if (victim)
        {
          if (!mag_savingthrow(ch, victim, SAVING_FORT, 0, CAST_WEAPON_SPELL, GET_LEVEL(ch), SCHOOL_NOSCHOOL))
          {
            send_to_char(ch, "Your weapon flashes with brilliant light!\r\n");
            act("$o carried by $n flashes with brilliant light", FALSE, ch, weapon, NULL, TO_ROOM);
            damage(ch, victim, dice(GET_LEVEL(ch) / 2 + 3, 6), TYPE_SPECAB_HOLY, DAM_HOLY, FALSE);
          }
          else
          {
            damage(ch, victim, dice(3, 6), TYPE_SPECAB_HOLY, DAM_HOLY, FALSE);
          }
        }
    }
    break;
  case ACTMTD_WEAR: /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

/* A weapon with the frost special ability generates cold, becoming encrusted with frost and dealing
 * cold damage on a regular hit. */
WEAPON_SPECIAL_ABILITY(weapon_specab_frost)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the frost ability.
     *  - Set the FROST bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_FROST))
    {
      /* Flaming is on, turn it off. */
      send_to_char(ch, "The magical frost sheathing your weapon vanishes.\r\n");
      act("The magical frost sheathing $n's $o vanishes.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_FROST);
    }
    else
    {
      /* FROST ON! */
      send_to_char(ch, "Magical frost spreads down the length of your weapon!\r\n");
      act("Magical frost spreads down the length of $n's $o!", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_FROST);
    }
    break;
  case ACTMTD_ON_HIT:                    /* Called whenever a weapon hits an enemy. */
    if (OBJ_FLAGGED(weapon, ITEM_FROST)) /* Freeze 'em. */
      if (victim)
      {
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_FROST, DAM_COLD, FALSE);
      }
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
  case ACTMTD_WEAR:    /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

/* A weapon with the shock special ability generates cold, becoming enveloped with electric energy and dealing
 * electric damage on a regular hit. */
WEAPON_SPECIAL_ABILITY(weapon_specab_shock)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the shock ability.
     *  - Set the shock bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_SHOCK))
    {
      /* Shocking is on, turn it off. */
      send_to_char(ch, "The magical lightning sheathing your weapon vanishes.\r\n");
      act("The magical lightning sheathing $n's $o vanishes.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_SHOCK);
    }
    else
    {
      /* lightning ON! */
      send_to_char(ch, "Magical lightning spreads down the length of your weapon!\r\n");
      act("Magical lightning spreads down the length of $n's $o!", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_SHOCK);
    }
    break;
  case ACTMTD_ON_HIT:                    /* Called whenever a weapon hits an enemy. */
    if (OBJ_FLAGGED(weapon, ITEM_SHOCK)) /* Freeze 'em. */
      if (victim)
      {
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_SHOCK, DAM_ELECTRIC, FALSE);
      }
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
  case ACTMTD_WEAR:    /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

/* A weapon with shocking burst functions as a shocking weapon, except on critical hits it
 * performs an electric burst for 1d10 extra damage. */
WEAPON_SPECIAL_ABILITY(weapon_specab_shocking_burst)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the shocking ability.
     *  - Set the shocking bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_SHOCK))
    {
      /* shocking is on, turn it off. */
      send_to_char(ch, "The magical lightning wreathing your weapon vanish.\r\n");
      act("The magical lightning wreathing $n's $o vanish.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_SHOCK);
    }
    else
    {
      /* FLAME ON! */
      send_to_char(ch, "Magical lightning spreads down the length of your weapon!\r\n");
      act("Magical lightning spreads down the length of $n's $o!", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_SHOCK);
    }
    break;
  case ACTMTD_ON_HIT:                    /* Called whenever a weapon hits an enemy. */
    if (OBJ_FLAGGED(weapon, ITEM_SHOCK)) /* shcok 'em. */
      if (victim)
      {
        /*send_to_char(ch, "\tr[spcab]\tn");*/
        damage(ch, victim, dice(1, 6), TYPE_SPECAB_SHOCK, DAM_ELECTRIC, FALSE);
      }
    break;
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    /* We don't care if the shocking property is active, it bursts anyway! */
    if (victim)
    {
      /* send_to_char(ch,"\tr[burst]\tn");*/
      damage(ch, victim, dice((weapon ? weapon_list[GET_OBJ_VAL(weapon, 0)].critMult - 1 : 1), 10), TYPE_SPECAB_SHOCKING_BURST, DAM_ELECTRIC, FALSE);
    }
    break;
  case ACTMTD_WEAR: /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

int add_draconic_claws_elemental_damage(struct char_data *ch, struct char_data *victim)
{
  int dam = dice(1, 6);
  int damtype = draconic_heritage_energy_types[GET_BLOODLINE_SUBTYPE(ch)];
  dam -= compute_damtype_reduction(ch, damtype);
  return MAX(0, dam);
}

WEAPON_SPECIAL_ABILITY(weapon_specab_seeking)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the seeking ability.
     *  - Set the SEEKING bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_SEEKING))
    {
      /* Seeking is on, turn it off. */
      send_to_char(ch, "A cloud of darkness covers your entire weapon.\r\n");
      act("A cloud of darkness covers $o wielded by $n.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_SEEKING);
    }
    else
    {
      /* Seeking On */
      send_to_char(ch, "A flash of light covers your entire weapon\r\n");
      act("A flash of light covers $o wielded by $n.", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_SEEKING);
    }
    break;
  case ACTMTD_ON_HIT:  /* Called whenever a weapon hits an enemy. */
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
  case ACTMTD_WEAR:    /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_adaptive)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the ADAPTIVE ability.
     *  - Set the ADAPTIVE bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_ADAPTIVE))
    {
      /* ADAPTIVE is on, turn it off. */
      send_to_char(ch, "The surface of your weapon loosens slightly.\r\n");
      act("The surface of $o wielded by $n loosens slightly.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_ADAPTIVE);
    }
    else
    {
      /* ADAPTIVE On */
      send_to_char(ch, "The surface of your weapon constricts slightly.\r\n");
      act("The surface of $o wielded by $n constricts slightly.", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_ADAPTIVE);
    }
    break;
  case ACTMTD_ON_HIT:  /* Called whenever a weapon hits an enemy. */
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
  case ACTMTD_WEAR:    /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_agile)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    /* Activate the AGILE ability.
     *  - Set the AGILE bit on the weapon (this affects the display,
     *    and is used to toggle the effect.)
     */
    if (OBJ_FLAGGED(weapon, ITEM_AGILE))
    {
      /* AGILE is on, turn it off. */
      send_to_char(ch, "The balance of your weapon has decreased dramatically.\r\n");
      act("$o, wielded by $n, looks a little heavier in $s hands.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_AGILE);
    }
    else
    {
      /* AGILE On */
      send_to_char(ch, "The balance of your weapon has increased dramatically.\r\n");
      act("$o, wielded by $n, looks a little lighter in $s hands.", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_AGILE);
    }
    break;
  case ACTMTD_ON_HIT:  /* Called whenever a weapon hits an enemy. */
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
  case ACTMTD_WEAR:    /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_defending)
{
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
    if (OBJ_FLAGGED(weapon, ITEM_DEFENDING))
    {
      send_to_char(ch, "Your weapon stops moving on its own accord.\r\n");
      act("$o, wielded by $n, stops moving on its own accord.", FALSE, ch, weapon, NULL, TO_ROOM);

      REMOVE_OBJ_FLAG(weapon, ITEM_DEFENDING);
    }
    else
    {
      send_to_char(ch, "Your weapon starts moving on its own accord.\r\n");
      act("$o, wielded by $n, starts moving on its own accord.", FALSE, ch, weapon, NULL, TO_ROOM);

      SET_OBJ_FLAG(weapon, ITEM_DEFENDING);
    }
    break;
  case ACTMTD_ON_HIT:  /* Called whenever a weapon hits an enemy. */
  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
  case ACTMTD_WEAR:    /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_blinding)
{

  struct affected_type af, af2;
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {

  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    if (!can_blind(victim))
    {
      return;
    }

    if (mag_savingthrow(ch, victim, SAVING_REFL, 0, CAST_WEAPON_SPELL, GET_LEVEL(ch), SCHOOL_NOSCHOOL))
    {
      act("You look away just in time to avoid getting blinded!", FALSE, victim, weapon, ch, TO_CHAR);
      act("$n looks away just in time to avoid getting blinded!", TRUE, victim, weapon, ch, TO_ROOM);
      return;
    }

    new_affect(&af);
    new_affect(&af2);

    af.spell = SPELL_BLINDNESS;
    af.location = APPLY_HITROLL;
    af.modifier = -4;
    af.duration = 1;
    af.bonus_type = BONUS_TYPE_UNDEFINED;
    SET_BIT_AR(af.bitvector, AFF_BLIND);

    af2.spell = SPELL_BLINDNESS;
    af2.location = APPLY_AC_NEW;
    af2.modifier = -4;
    af2.duration = 1;
    af2.bonus_type = BONUS_TYPE_UNDEFINED;
    SET_BIT_AR(af2.bitvector, AFF_BLIND);

    act("You have been blinded!", FALSE, victim, 0, ch, TO_CHAR);
    act("$n seems to be blinded!", TRUE, victim, 0, ch, TO_ROOM);

    affect_to_char(victim, &af);
    affect_to_char(victim, &af2);
    break;
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
  case ACTMTD_ON_HIT:       /* Called whenever a weapon hits an enemy. */
  case ACTMTD_WEAR:         /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_exhausting)
{

  struct affected_type af;
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {

  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */

    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, CAST_WEAPON_SPELL, GET_LEVEL(ch), SCHOOL_NOSCHOOL))
    {
      act("You resist the wave of exhaustion from the blow of $o.", FALSE, victim, weapon, ch, TO_CHAR);
      act("$n resists the wave of exhaustion from the blow of $o.", TRUE, victim, weapon, ch, TO_ROOM);
      return;
    }

    new_affect(&af);

    af.duration = 2;
    af.location = SPELL_WAVES_OF_EXHAUSTION;
    SET_BIT_AR(af.bitvector, AFF_FATIGUED);
    GET_MOVE(victim) -= 20;
    if (GET_MOVE(victim) < 0)
      GET_MOVE(victim) = 0;

    act("You have been inflicted with heavy fatigue!", FALSE, victim, 0, ch, TO_CHAR);
    act("$n seems to be inflicted with heavy fatigue!", TRUE, victim, 0, ch, TO_ROOM);

    affect_to_char(victim, &af);
    break;
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
  case ACTMTD_ON_HIT:       /* Called whenever a weapon hits an enemy. */
  case ACTMTD_WEAR:         /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_thundering)
{

  struct affected_type af;
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {

  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */

    if (!victim)
      break;

    damage(ch, victim, dice(2, 10), TYPE_SPECAB_THUNDERING, DAM_SOUND, FALSE);

    if (!can_deafen(victim))
    {
      send_to_char(ch, "Your opponent doesn't seem deafable.\r\n");
      return;
    }

    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, CAST_WEAPON_SPELL, GET_LEVEL(ch), SCHOOL_NOSCHOOL))
    {
      act("You resist the thunderlcap from the blow of $o.", FALSE, victim, weapon, ch, TO_CHAR);
      act("$n resists the thunderclap from the blow of $o.", TRUE, victim, weapon, ch, TO_ROOM);
      return;
    }

    new_affect(&af);
    af.duration = 2;
    af.spell = SPELL_DEAFNESS;
    SET_BIT_AR(af.bitvector, AFF_DEAF);

    act("You have been deafened!", FALSE, victim, 0, ch, TO_CHAR);
    act("$n seems to have been deafened!", TRUE, victim, 0, ch, TO_ROOM);

    affect_to_char(victim, &af);

    break;
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
  case ACTMTD_ON_HIT:       /* Called whenever a weapon hits an enemy. */
  case ACTMTD_WEAR:         /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_bewildering)
{

  struct affected_type af;
  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {

  case ACTMTD_ON_CRIT: /* Called whenever a weapon hits critically. */
    if (MOB_FLAGGED(victim, MOB_NOCONFUSE))
    {
      return;
    }

    if (!can_confuse(victim))
    {
      send_to_char(ch, "Your opponent seems to be immune to confusion effects.\r\n");
      return;
    }

    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, CAST_WEAPON_SPELL, GET_LEVEL(ch), ENCHANTMENT))
    {
      act("You shake off a cloud of confusion settling over your mind.", FALSE, victim, weapon, ch, TO_CHAR);
      act("$n looks confused for a moment, but shakes it off.", TRUE, victim, weapon, ch, TO_ROOM);
      return;
    }

    new_affect(&af);

    af.spell = SPELL_CONFUSION;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = 1;
    af.bonus_type = BONUS_TYPE_UNDEFINED;
    SET_BIT_AR(af.bitvector, AFF_CONFUSED);

    act("You feel extremely confused.", FALSE, victim, 0, ch, TO_CHAR);
    act("$n seems extremely confused.", TRUE, victim, 0, ch, TO_ROOM);

    affect_to_char(victim, &af);

    break;
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
  case ACTMTD_ON_HIT:       /* Called whenever a weapon hits an enemy. */
  case ACTMTD_WEAR:         /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

WEAPON_SPECIAL_ABILITY(weapon_specab_wounding)
{

  struct affected_type af;

  /*
   * level
   * weapon
   * ch
   * victim
   * obj
   */
  switch (actmtd)
  {

  case ACTMTD_ON_HIT:
  case ACTMTD_ON_CRIT:

    if ((GET_NPC_RACE(victim) == RACE_TYPE_CONSTRUCT) ||
        (GET_NPC_RACE(victim) == RACE_TYPE_UNDEAD) ||
        (GET_NPC_RACE(victim) == RACE_TYPE_OOZE))
      return;

    new_affect(&af);

    af.spell = TYPE_SPECAB_BLEEDING;
    af.location = APPLY_SPECIAL;
    af.modifier = dice(1, 4);
    af.duration = 3;
    af.bonus_type = BONUS_TYPE_UNDEFINED;
    SET_BIT_AR(af.bitvector, AFF_BLEED);

    if (AFF_FLAGGED(victim, AFF_BLEED))
    {
      act("Your bleeding worsens.", FALSE, victim, 0, ch, TO_CHAR);
      act("$n's bleeding worsens.", TRUE, victim, 0, ch, TO_ROOM);
    }
    else
    {
      act("You start to bleed.", FALSE, victim, 0, ch, TO_CHAR);
      act("$n starts to bleed.", TRUE, victim, 0, ch, TO_ROOM);
    }

    affect_to_char(victim, &af);
    break;
  case ACTMTD_COMMAND_WORD: /* User UTTERs the command word. */
  case ACTMTD_USE:          /* User USEs the item. */
  case ACTMTD_WEAR:         /* Called whenever the item is worn. */
  default:
    /* Do nothing. */
    break;
  }
}

char *get_weapon_specab_default_command_word(int specab)
{
  switch (specab)
  {
  case WEAPON_SPECAB_BLINDING:
    return strdup("obscure");
  case WEAPON_SPECAB_FLAMING:
  case WEAPON_SPECAB_FLAMING_BURST:
    return strdup("blaze");
  case WEAPON_SPECAB_CORROSIVE:
  case WEAPON_SPECAB_CORROSIVE_BURST:
    return strdup("corrode");
  case WEAPON_SPECAB_FROST:
  case WEAPON_SPECAB_ICY_BURST:
    return strdup("glacier");
  case WEAPON_SPECAB_SHOCK:
  case WEAPON_SPECAB_SHOCKING_BURST:
    return strdup("spark");
  case WEAPON_SPECAB_VICIOUS:
    return strdup("ferocity");
  case WEAPON_SPECAB_VORPAL:
    return strdup("decapitate");
  case WEAPON_SPECAB_DISRUPTION:
    return strdup("exorcise");
  case WEAPON_SPECAB_SEEKING:
    return strdup("snipe");
  case WEAPON_SPECAB_ADAPTIVE:
    return strdup("propel");
  case WEAPON_SPECAB_AGILE:
    return strdup("fleet");
  case WEAPON_SPECAB_DEFENDING:
    return strdup("aegis");
  case WEAPON_SPECAB_VAMPIRIC:
    return strdup("drain");
  }
  return NULL;
}
