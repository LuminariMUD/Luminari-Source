/*
 * Premade build code for LuminariMUD by Gicker aka Steve Squires
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "modify.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "class.h"
#include "race.h"
#include "spec_procs.h"  // for compute_ability
#include "mud_event.h"  // for purgemob event
#include "feats.h"
#include "spec_abilities.h"
#include "assign_wpn_armor.h"
#include "wilderness.h"
#include "domains_schools.h"
#include "constants.h"
#include "dg_scripts.h"
#include "templates.h"
#include "oasis.h"
#include "premadebuilds.h"

void give_premade_skill(struct char_data *ch, bool verbose, int skill, int amount)
{
  SET_ABILITY(ch, skill, GET_ABILITY(ch, skill) + amount);
  if (verbose)
    send_to_char(ch, "You have improved your %s skill by %d.\r\n", spell_info[skill].name, amount);
}

void increase_skills(struct char_data *ch, int chclass, bool verbose, int level)
{

  int amount = 1;
  if (level == 1) amount = 4;

  switch (chclass) {
    case CLASS_WARRIOR:
      give_premade_skill(ch, verbose, ABILITY_HEAL, amount);
      give_premade_skill(ch, verbose, ABILITY_LORE, amount);
      give_premade_skill(ch, verbose, ABILITY_INTIMIDATE, amount);
      give_premade_skill(ch, verbose, ABILITY_DISCIPLINE, amount);
      if (GET_REAL_RACE(ch) == RACE_HUMAN)
        give_premade_skill(ch, verbose, ABILITY_TOTAL_DEFENSE, amount);
      break;
    case CLASS_ROGUE:
      give_premade_skill(ch, verbose, ABILITY_HEAL, amount);
      give_premade_skill(ch, verbose, ABILITY_LORE, amount);
      give_premade_skill(ch, verbose, ABILITY_ACROBATICS, amount);
      give_premade_skill(ch, verbose, ABILITY_BLUFF, amount);
      give_premade_skill(ch, verbose, ABILITY_DISABLE_DEVICE, amount);
      give_premade_skill(ch, verbose, ABILITY_PERCEPTION, amount);
      give_premade_skill(ch, verbose, ABILITY_STEALTH, amount);
      give_premade_skill(ch, verbose, ABILITY_SLEIGHT_OF_HAND, amount);
      give_premade_skill(ch, verbose, ABILITY_APPRAISE, amount);
      if (GET_REAL_RACE(ch) == RACE_HUMAN)
        give_premade_skill(ch, verbose, ABILITY_RIDE, amount);
      break;
    case CLASS_MONK:
      give_premade_skill(ch, verbose, ABILITY_HEAL, amount);
      give_premade_skill(ch, verbose, ABILITY_LORE, amount);
      give_premade_skill(ch, verbose, ABILITY_ACROBATICS, amount);
      give_premade_skill(ch, verbose, ABILITY_DISCIPLINE, amount);
      if (GET_REAL_RACE(ch) == RACE_HUMAN)
        give_premade_skill(ch, verbose, ABILITY_SWIM, amount);
      break;
    case CLASS_CLERIC:
      give_premade_skill(ch, verbose, ABILITY_LORE, amount);
      give_premade_skill(ch, verbose, ABILITY_SPELLCRAFT, amount);
      give_premade_skill(ch, verbose, ABILITY_CONCENTRATION, amount);
      if (GET_REAL_RACE(ch) == RACE_HUMAN)
        give_premade_skill(ch, verbose, ABILITY_HEAL, amount);
      break;
    case CLASS_BERSERKER:
      give_premade_skill(ch, verbose, ABILITY_HEAL, amount);
      give_premade_skill(ch, verbose, ABILITY_LORE, amount);
      give_premade_skill(ch, verbose, ABILITY_DISCIPLINE, amount);
      give_premade_skill(ch, verbose, ABILITY_INTIMIDATE, amount);
      give_premade_skill(ch, verbose, ABILITY_SWIM, amount);
      if (GET_REAL_RACE(ch) == RACE_HUMAN)
        give_premade_skill(ch, verbose, ABILITY_CLIMB, amount);
      break;
    case CLASS_WIZARD:
      give_premade_skill(ch, verbose, ABILITY_HEAL, amount);
      give_premade_skill(ch, verbose, ABILITY_LORE, amount);
      give_premade_skill(ch, verbose, ABILITY_DISCIPLINE, amount);
      give_premade_skill(ch, verbose, ABILITY_SPELLCRAFT, amount);
      give_premade_skill(ch, verbose, ABILITY_CONCENTRATION, amount);
      give_premade_skill(ch, verbose, ABILITY_APPRAISE, amount);
      if (CLASS_LEVEL(ch, CLASS_WIZARD) >= 4) //int goes up to 18
        give_premade_skill(ch, verbose, ABILITY_USE_MAGIC_DEVICE, amount);
      if (CLASS_LEVEL(ch, CLASS_WIZARD) >= 12) // int goes up to 20
        give_premade_skill(ch, verbose, ABILITY_SWIM, amount);
      if (CLASS_LEVEL(ch, CLASS_WIZARD) >= 20) // int goes up to 22
        give_premade_skill(ch, verbose, ABILITY_RIDE, amount);
      if (GET_REAL_RACE(ch) == RACE_HUMAN)
        give_premade_skill(ch, verbose, ABILITY_SENSE_MOTIVE, amount);
      break;

  }

}

void give_premade_feat(struct char_data *ch, bool verbose, int feat, int subfeat)
{

  if (subfeat > 0) {
    SET_FEAT(ch, feat, 1);
    SET_COMBAT_FEAT(ch, feat_to_cfeat(feat), subfeat);
    if (verbose) {
      if (feat_list[feat].combat_feat == TRUE)
        send_to_char(ch, "You have learned the %s (%s) feat.\r\n", feat_list[feat].name, weapon_family[subfeat]);
      else
        send_to_char(ch, "You have learned the %s feat.\r\n", feat_list[feat].name);
      do_help(ch, feat_list[feat].name, 0, 0);
    }
  } else {
    SET_FEAT(ch, feat, 1);
    if (verbose) {
      send_to_char(ch, "You have learned the %s feat.\r\n", feat_list[feat].name);
      do_help(ch, feat_list[feat].name, 0, 0);
    }
  }
}

void set_premade_stats(struct char_data *ch, int chclass, int level)
{

  switch (chclass) {
    case CLASS_WARRIOR:
        switch (level) {
          case 1:
            GET_REAL_STR(ch) = 16 + race_list[GET_REAL_RACE(ch)].ability_mods[0];
            GET_REAL_CON(ch) = 14 + race_list[GET_REAL_RACE(ch)].ability_mods[1];
            GET_REAL_INT(ch) = 14 + race_list[GET_REAL_RACE(ch)].ability_mods[2];
            GET_REAL_WIS(ch) = 10 + race_list[GET_REAL_RACE(ch)].ability_mods[3];
            GET_REAL_DEX(ch) = 14 + race_list[GET_REAL_RACE(ch)].ability_mods[4];
            GET_REAL_CHA(ch) =  8 + race_list[GET_REAL_RACE(ch)].ability_mods[5];
            if (GET_REAL_RACE(ch) == RACE_HUMAN) GET_REAL_CON(ch) += 2;
          break;
          case 4: case 8: case 12: case 16: case 20:
            GET_REAL_STR(ch)++;
            break;
        }
      break;
    case CLASS_ROGUE:
        switch (level) {
          case 1:
            GET_REAL_STR(ch) = 12 + race_list[GET_REAL_RACE(ch)].ability_mods[0];
            GET_REAL_CON(ch) = 10 + race_list[GET_REAL_RACE(ch)].ability_mods[1];
            GET_REAL_INT(ch) = 12 + race_list[GET_REAL_RACE(ch)].ability_mods[2];
            GET_REAL_WIS(ch) = 10 + race_list[GET_REAL_RACE(ch)].ability_mods[3];
            GET_REAL_DEX(ch) = 18 + race_list[GET_REAL_RACE(ch)].ability_mods[4];
            GET_REAL_CHA(ch) =  8 + race_list[GET_REAL_RACE(ch)].ability_mods[5];
            if (GET_REAL_RACE(ch) == RACE_HUMAN) { GET_REAL_WIS(ch) += 2; GET_REAL_STR(ch) += 2; }
          break;
          case 4: case 8: case 12: case 16: case 20:
            GET_REAL_DEX(ch)++;
            break;
        }
      break;
    case CLASS_MONK:
        switch (level) {
          case 1:
            GET_REAL_STR(ch) = 14 + race_list[GET_REAL_RACE(ch)].ability_mods[0];
            GET_REAL_CON(ch) = 10 + race_list[GET_REAL_RACE(ch)].ability_mods[1];
            GET_REAL_INT(ch) = 10 + race_list[GET_REAL_RACE(ch)].ability_mods[2];
            GET_REAL_WIS(ch) = 16 + race_list[GET_REAL_RACE(ch)].ability_mods[3];
            GET_REAL_DEX(ch) = 14 + race_list[GET_REAL_RACE(ch)].ability_mods[4];
            GET_REAL_CHA(ch) =  8 + race_list[GET_REAL_RACE(ch)].ability_mods[5];
            if (GET_REAL_RACE(ch) == RACE_HUMAN) { GET_REAL_DEX(ch) += 2; }
          break;
          case 4: case 8: case 12: case 16: case 20:
            GET_REAL_WIS(ch)++;
            break;
        }
      break;
    case CLASS_CLERIC:
        switch (level) {
          case 1:
            GET_REAL_STR(ch) = 14 + race_list[GET_REAL_RACE(ch)].ability_mods[0];
            GET_REAL_CON(ch) = 10 + race_list[GET_REAL_RACE(ch)].ability_mods[1];
            GET_REAL_INT(ch) = 12 + race_list[GET_REAL_RACE(ch)].ability_mods[2];
            GET_REAL_WIS(ch) = 16 + race_list[GET_REAL_RACE(ch)].ability_mods[3];
            GET_REAL_DEX(ch) = 14 + race_list[GET_REAL_RACE(ch)].ability_mods[4];
            GET_REAL_CHA(ch) =  8 + race_list[GET_REAL_RACE(ch)].ability_mods[5];
            if (GET_REAL_RACE(ch) == RACE_HUMAN) { GET_REAL_CON(ch) += 2; }
          break;
          case 4: case 8: case 12: 
            GET_REAL_WIS(ch)++;
            break;
          case 16: case 20:
            GET_REAL_STR(ch)++;
            break;
        }
      break;
    case CLASS_BERSERKER:
        switch (level) {
          case 1:
            GET_REAL_STR(ch) = 16 + race_list[GET_REAL_RACE(ch)].ability_mods[0];
            GET_REAL_CON(ch) = 14 + race_list[GET_REAL_RACE(ch)].ability_mods[1];
            GET_REAL_INT(ch) = 13 + race_list[GET_REAL_RACE(ch)].ability_mods[2];
            GET_REAL_WIS(ch) =  8 + race_list[GET_REAL_RACE(ch)].ability_mods[3];
            GET_REAL_DEX(ch) = 13 + race_list[GET_REAL_RACE(ch)].ability_mods[4];
            GET_REAL_CHA(ch) =  8 + race_list[GET_REAL_RACE(ch)].ability_mods[5];
            if (GET_REAL_RACE(ch) == RACE_HUMAN) { GET_REAL_CON(ch) += 2; }
          break;
          case 4: case 8: case 12: case 16: case 20:
            GET_REAL_STR(ch)++;
            break;
        }
      break;
    case CLASS_WIZARD:
        switch (level) {
          case 1:
            GET_REAL_STR(ch) = 10 + race_list[GET_REAL_RACE(ch)].ability_mods[0];
            GET_REAL_CON(ch) = 12 + race_list[GET_REAL_RACE(ch)].ability_mods[1];
            GET_REAL_INT(ch) = 17 + race_list[GET_REAL_RACE(ch)].ability_mods[2];
            GET_REAL_WIS(ch) = 10 + race_list[GET_REAL_RACE(ch)].ability_mods[3];
            GET_REAL_DEX(ch) = 13 + race_list[GET_REAL_RACE(ch)].ability_mods[4];
            GET_REAL_CHA(ch) =  8 + race_list[GET_REAL_RACE(ch)].ability_mods[5];
            if (GET_REAL_RACE(ch) == RACE_HUMAN) { GET_REAL_CON(ch) += 2; GET_REAL_DEX(ch) += 1; GET_REAL_WIS(ch) += 1;}
          break;
          case 4: case 8: case 12: case 16: case 20:
            GET_REAL_STR(ch)++;
            break;
        }
      break;
  }

}

void levelup_warrior(struct char_data *ch, int level, bool verbose)
{
  int chclass = CLASS_WARRIOR;
  switch (level) {
    case 1:
      set_premade_stats(ch, chclass, 1);
      increase_skills(ch, chclass, TRUE, 1);
      give_premade_feat(ch, verbose, FEAT_WEAPON_FOCUS, WEAPON_FAMILY_HEAVY_BLADE);
      give_premade_feat(ch, verbose, FEAT_LUCK_OF_HEROES, 0);
      if (GET_REAL_RACE(ch) == RACE_HUMAN) {
        give_premade_feat(ch, verbose, FEAT_GREAT_FORTITUDE, 0);
      }
      break;
    case 2:
      increase_skills(ch, chclass, TRUE, 2);
      give_premade_feat(ch, verbose, FEAT_POWER_ATTACK, 0);
      break;
    case 3:
      increase_skills(ch, chclass, TRUE, 3);
      give_premade_feat(ch, verbose, FEAT_TOUGHNESS, 0);
      break;
    case 4:
      set_premade_stats(ch, chclass, 4);
      increase_skills(ch, chclass, TRUE, 4);
      give_premade_feat(ch, verbose, FEAT_WEAPON_SPECIALIZATION, WEAPON_FAMILY_HEAVY_BLADE);
      break;
    case 5:
      increase_skills(ch, chclass, TRUE, 5);
      break;
    case 6:
      increase_skills(ch, chclass, TRUE, 6);
      give_premade_feat(ch, verbose, FEAT_LIGHTNING_REFLEXES, 0);
      give_premade_feat(ch, verbose, FEAT_DODGE, 0);
      break;
    case 7:
      increase_skills(ch, chclass, TRUE, 7);
      break;
    case 8:
      set_premade_stats(ch, chclass, 8);
      increase_skills(ch, chclass, TRUE, 8);
      give_premade_feat(ch, verbose, FEAT_IMPROVED_CRITICAL, WEAPON_FAMILY_HEAVY_BLADE);
      break;
    case 9:
      increase_skills(ch, chclass, TRUE, 9);
      give_premade_feat(ch, verbose, FEAT_POWER_CRITICAL, WEAPON_FAMILY_HEAVY_BLADE);
      break;
    case 10:
      increase_skills(ch, chclass, TRUE, 10);
      give_premade_feat(ch, verbose, FEAT_GREATER_WEAPON_FOCUS, WEAPON_FAMILY_HEAVY_BLADE);
      break;
    case 11:
      increase_skills(ch, chclass, TRUE, 11);
      break;
    case 12:
      set_premade_stats(ch, chclass, 12);
      increase_skills(ch, chclass, TRUE, 12);
      give_premade_feat(ch, verbose, FEAT_FAST_HEALER, 0);
      give_premade_feat(ch, verbose, FEAT_ARMOR_SPECIALIZATION_HEAVY, 0);
      break;
    case 13:
      increase_skills(ch, chclass, TRUE, 13);
      break;
    case 14:
      increase_skills(ch, chclass, TRUE, 14);
      give_premade_feat(ch, verbose, FEAT_GREATER_WEAPON_SPECIALIZATION, WEAPON_FAMILY_HEAVY_BLADE);
      break;
    case 15:
      increase_skills(ch, chclass, TRUE, 15);
      give_premade_feat(ch, verbose, FEAT_MOBILITY, 0);
      break;
    case 16:
      set_premade_stats(ch, chclass, 16);
      increase_skills(ch, chclass, TRUE, 16);
      give_premade_feat(ch, verbose, FEAT_SPRING_ATTACK, 0);
      break;
    case 17:
      increase_skills(ch, chclass, TRUE, 17);
      break;
    case 18:
      increase_skills(ch, chclass, TRUE, 18);
      give_premade_feat(ch, verbose, FEAT_IRON_WILL, 0);
      give_premade_feat(ch, verbose, FEAT_COMBAT_EXPERTISE, 0);
      break;
    case 19:
      increase_skills(ch, chclass, TRUE, 17);
      break;
    case 20:
      set_premade_stats(ch, chclass, 20);
      increase_skills(ch, chclass, TRUE, 20);
      give_premade_feat(ch, verbose, FEAT_WHIRLWIND_ATTACK, 0);
      break;
  }
}

void levelup_rogue(struct char_data *ch, int level, bool verbose)
{
  int chclass = CLASS_ROGUE;
  switch (level) {
    case 1:
      set_premade_stats(ch, chclass, 1);
      increase_skills(ch, chclass, TRUE, 1);
      give_premade_feat(ch, verbose, FEAT_TWO_WEAPON_FIGHTING, 0);
      if (GET_REAL_RACE(ch) == RACE_HUMAN) {
        give_premade_feat(ch, verbose, FEAT_DODGE, 0);
      }
      break;
    case 2:
      increase_skills(ch, chclass, TRUE, 2);
      break;
    case 3:
      increase_skills(ch, chclass, TRUE, 3);
      give_premade_feat(ch, verbose, FEAT_WEAPON_FOCUS, WEAPON_FAMILY_LIGHT_BLADE);
      break;
    case 4:
      set_premade_stats(ch, chclass, 4);
      increase_skills(ch, chclass, TRUE, 4);
      break;
    case 5:
      increase_skills(ch, chclass, TRUE, 5);
      break;
    case 6:
      increase_skills(ch, chclass, TRUE, 6);
      give_premade_feat(ch, verbose, FEAT_IMPROVED_TWO_WEAPON_FIGHTING, 0);
      break;
    case 7:
      increase_skills(ch, chclass, TRUE, 7);
      break;
    case 8:
      set_premade_stats(ch, chclass, 8);
      increase_skills(ch, chclass, TRUE, 8);
      break;
    case 9:
      increase_skills(ch, chclass, TRUE, 9);
      give_premade_feat(ch, verbose, FEAT_LUCK_OF_HEROES, 0);
      break;
    case 10:
      increase_skills(ch, chclass, TRUE, 10);
      break;
    case 11:
      increase_skills(ch, chclass, TRUE, 11);
      break;
    case 12:
      set_premade_stats(ch, chclass, 12);
      increase_skills(ch, chclass, TRUE, 12);
      give_premade_feat(ch, verbose, FEAT_GREATER_TWO_WEAPON_FIGHTING, 0);
      break;
    case 13:
      increase_skills(ch, chclass, TRUE, 13);
      break;
    case 14:
      increase_skills(ch, chclass, TRUE, 14);
      break;
    case 15:
      increase_skills(ch, chclass, TRUE, 15);
      give_premade_feat(ch, verbose, FEAT_MOBILITY, 0);
      break;
    case 16:
      set_premade_stats(ch, chclass, 16);
      increase_skills(ch, chclass, TRUE, 16);
      break;
    case 17:
      increase_skills(ch, chclass, TRUE, 17);
      break;
    case 18:
      increase_skills(ch, chclass, TRUE, 18);
      give_premade_feat(ch, verbose, FEAT_SPRING_ATTACK, 0);
      break;
    case 19:
      increase_skills(ch, chclass, TRUE, 19);
      break;
    case 20:
      set_premade_stats(ch, chclass, 20);
      increase_skills(ch, chclass, TRUE, 20);
      break;
  }
}

void levelup_monk(struct char_data *ch, int level, bool verbose)
{
  int chclass = CLASS_MONK;
  switch (level) {
    case 1:
      set_premade_stats(ch, chclass, 1);
      increase_skills(ch, chclass, TRUE, 1);
      give_premade_feat(ch, verbose, FEAT_LUCK_OF_HEROES, 0);
      if (GET_REAL_RACE(ch) == RACE_HUMAN) {
        give_premade_feat(ch, verbose, FEAT_TOUGHNESS, 0);
      }
      break;
    case 2:
      increase_skills(ch, chclass, TRUE, 2);
      break;
    case 3:
      increase_skills(ch, chclass, TRUE, 3);
      give_premade_feat(ch, verbose, FEAT_WEAPON_FOCUS, WEAPON_FAMILY_MONK);
      break;
    case 4:
      set_premade_stats(ch, chclass, 4);
      increase_skills(ch, chclass, TRUE, 4);
      break;
    case 5:
      increase_skills(ch, chclass, TRUE, 5);
      break;
    case 6:
      increase_skills(ch, chclass, TRUE, 6);
      give_premade_feat(ch, verbose, FEAT_POWER_CRITICAL, WEAPON_FAMILY_MONK);
      break;
    case 7:
      increase_skills(ch, chclass, TRUE, 7);
      break;
    case 8:
      set_premade_stats(ch, chclass, 8);
      increase_skills(ch, chclass, TRUE, 8);
      break;
    case 9:
      increase_skills(ch, chclass, TRUE, 9);
      give_premade_feat(ch, verbose, FEAT_POWER_ATTACK, 0);
      break;
    case 10:
      increase_skills(ch, chclass, TRUE, 10);
      break;
    case 11:
      increase_skills(ch, chclass, TRUE, 11);
      break;
    case 12:
      set_premade_stats(ch, chclass, 12);
      increase_skills(ch, chclass, TRUE, 12);
      give_premade_feat(ch, verbose, FEAT_IMPROVED_CRITICAL, WEAPON_FAMILY_MONK);
      break;
    case 13:
      increase_skills(ch, chclass, TRUE, 13);
      break;
    case 14:
      increase_skills(ch, chclass, TRUE, 14);
      break;
    case 15:
      increase_skills(ch, chclass, TRUE, 15);
      give_premade_feat(ch, verbose, FEAT_FAST_HEALER, 0);
      break;
    case 16:
      set_premade_stats(ch, chclass, 16);
      increase_skills(ch, chclass, TRUE, 16);
      break;
    case 17:
      increase_skills(ch, chclass, TRUE, 17);
      break;
    case 18:
      increase_skills(ch, chclass, TRUE, 18);
      give_premade_feat(ch, verbose, FEAT_DODGE, 0);
      break;
    case 19:
      increase_skills(ch, chclass, TRUE, 19);
      break;
    case 20:
      set_premade_stats(ch, chclass, 20);
      increase_skills(ch, chclass, TRUE, 20);
      break;
  }
}

void levelup_cleric(struct char_data *ch, int level, bool verbose)
{
  int chclass = CLASS_CLERIC;
  switch (level) {
    case 1:
      set_premade_stats(ch, chclass, 1);
      increase_skills(ch, chclass, TRUE, 1);
      give_premade_feat(ch, verbose, FEAT_LUCK_OF_HEROES, 0);
      if (GET_REAL_RACE(ch) == RACE_HUMAN) {
        give_premade_feat(ch, verbose, FEAT_TOUGHNESS, 0);
      }
      GET_1ST_DOMAIN(ch) = DOMAIN_WAR;
      GET_2ND_DOMAIN(ch) = DOMAIN_HEALING;
      /* set spells learned for domain */
      assign_domain_spells(ch);
      /* in case adding or changing clear domains, clean up and re-assign */
      clear_domain_feats(ch);
      add_domain_feats(ch);
      break;
    case 2:
      increase_skills(ch, chclass, TRUE, 2);
      break;
    case 3:
      increase_skills(ch, chclass, TRUE, 3);
      give_premade_feat(ch, verbose, FEAT_QUICK_CHANT, 0);
      break;
    case 4:
      set_premade_stats(ch, chclass, 4);
      increase_skills(ch, chclass, TRUE, 4);
      break;
    case 5:
      increase_skills(ch, chclass, TRUE, 5);
      break;
    case 6:
      increase_skills(ch, chclass, TRUE, 6);
      give_premade_feat(ch, verbose, FEAT_FASTER_MEMORIZATION, 0);
      break;
    case 7:
      increase_skills(ch, chclass, TRUE, 7);
      break;
    case 8:
      set_premade_stats(ch, chclass, 8);
      increase_skills(ch, chclass, TRUE, 8);
      break;
    case 9:
      increase_skills(ch, chclass, TRUE, 9);
      give_premade_feat(ch, verbose, FEAT_WEAPON_FOCUS, WEAPON_FAMILY_HAMMER);
      break;
    case 10:
      increase_skills(ch, chclass, TRUE, 10);
      break;
    case 11:
      increase_skills(ch, chclass, TRUE, 11);
      break;
    case 12:
      set_premade_stats(ch, chclass, 12);
      increase_skills(ch, chclass, TRUE, 12);
      give_premade_feat(ch, verbose, FEAT_IMPROVED_CRITICAL, WEAPON_FAMILY_HAMMER);
      break;
    case 13:
      increase_skills(ch, chclass, TRUE, 13);
      break;
    case 14:
      increase_skills(ch, chclass, TRUE, 14);
      break;
    case 15:
      increase_skills(ch, chclass, TRUE, 15);
      give_premade_feat(ch, verbose, FEAT_POWER_CRITICAL, WEAPON_FAMILY_HAMMER);
      break;
    case 16:
      set_premade_stats(ch, chclass, 16);
      increase_skills(ch, chclass, TRUE, 16);
      break;
    case 17:
      increase_skills(ch, chclass, TRUE, 17);
      break;
    case 18:
      increase_skills(ch, chclass, TRUE, 18);
      give_premade_feat(ch, verbose, FEAT_POWER_ATTACK, 0);
      break;
    case 19:
      increase_skills(ch, chclass, TRUE, 19);
      break;
    case 20:
      set_premade_stats(ch, chclass, 20);
      increase_skills(ch, chclass, TRUE, 20);
      break;
  }
}

void levelup_berserker(struct char_data *ch, int level, bool verbose)
{
  int chclass = CLASS_BERSERKER;
  switch (level) {
    case 1:
      set_premade_stats(ch, chclass, 1);
      increase_skills(ch, chclass, TRUE, 1);
      give_premade_feat(ch, verbose, FEAT_POWER_ATTACK, 0);
      if (GET_REAL_RACE(ch) == RACE_HUMAN) {
        give_premade_feat(ch, verbose, FEAT_LUCK_OF_HEROES, 0);
      }
      break;
    case 2:
      increase_skills(ch, chclass, TRUE, 2);
      break;
    case 3:
      increase_skills(ch, chclass, TRUE, 3);
      give_premade_feat(ch, verbose, FEAT_DODGE, 0);
      break;
    case 4:
      set_premade_stats(ch, chclass, 4);
      increase_skills(ch, chclass, TRUE, 4);
      break;
    case 5:
      increase_skills(ch, chclass, TRUE, 5);
      break;
    case 6:
      increase_skills(ch, chclass, TRUE, 6);
      give_premade_feat(ch, verbose, FEAT_MOBILITY, 0);
      break;
    case 7:
      increase_skills(ch, chclass, TRUE, 7);
      break;
    case 8:
      set_premade_stats(ch, chclass, 8);
      increase_skills(ch, chclass, TRUE, 8);
      break;
    case 9:
      increase_skills(ch, chclass, TRUE, 9);
      give_premade_feat(ch, verbose, FEAT_IMPROVED_CRITICAL, WEAPON_FAMILY_AXE);
      break;
    case 10:
      increase_skills(ch, chclass, TRUE, 10);
      break;
    case 11:
      increase_skills(ch, chclass, TRUE, 11);
      break;
    case 12:
      set_premade_stats(ch, chclass, 12);
      increase_skills(ch, chclass, TRUE, 12);
      give_premade_feat(ch, verbose, FEAT_SPRING_ATTACK, 0);
      break;
    case 13:
      increase_skills(ch, chclass, TRUE, 13);
      break;
    case 14:
      increase_skills(ch, chclass, TRUE, 14);
      break;
    case 15:
      increase_skills(ch, chclass, TRUE, 15);
      give_premade_feat(ch, verbose, FEAT_COMBAT_EXPERTISE, 0);
      break;
    case 16:
      set_premade_stats(ch, chclass, 16);
      increase_skills(ch, chclass, TRUE, 16);
      break;
    case 17:
      increase_skills(ch, chclass, TRUE, 17);
      break;
    case 18:
      increase_skills(ch, chclass, TRUE, 18);
      give_premade_feat(ch, verbose, FEAT_WHIRLWIND_ATTACK, 0);
      break;
    case 19:
      increase_skills(ch, chclass, TRUE, 19);
      break;
    case 20:
      set_premade_stats(ch, chclass, 20);
      increase_skills(ch, chclass, TRUE, 20);
      break;
  }
}

void levelup_wizard(struct char_data *ch, int level, bool verbose)
{
  int chclass = CLASS_WIZARD;
  switch (level) {
    case 1:
      set_premade_stats(ch, chclass, 1);
      increase_skills(ch, chclass, TRUE, 1);
      give_premade_feat(ch, verbose, FEAT_FASTER_MEMORIZATION, 0);
      if (GET_REAL_RACE(ch) == RACE_HUMAN) {
        give_premade_feat(ch, verbose, FEAT_LUCK_OF_HEROES, 0);
      }
      GET_FAMILIAR(ch) = 85; // imp
      break;
    case 2:
      increase_skills(ch, chclass, TRUE, 2);
      break;
    case 3:
      increase_skills(ch, chclass, TRUE, 3);
      give_premade_feat(ch, verbose, FEAT_COMBAT_CASTING, 0);
      break;
    case 4:
      set_premade_stats(ch, chclass, 4);
      increase_skills(ch, chclass, TRUE, 4);
      break;
    case 5:
      increase_skills(ch, chclass, TRUE, 5);
      give_premade_feat(ch, verbose, FEAT_ENHANCED_SPELL_DAMAGE, 0);
      break;
    case 6:
      increase_skills(ch, chclass, TRUE, 6);
      give_premade_feat(ch, verbose, FEAT_SPELL_PENETRATION, 0);
      break;
    case 7:
      increase_skills(ch, chclass, TRUE, 7);
      break;
    case 8:
      set_premade_stats(ch, chclass, 8);
      increase_skills(ch, chclass, TRUE, 8);
      break;
    case 9:
      increase_skills(ch, chclass, TRUE, 9);
      give_premade_feat(ch, verbose, FEAT_SPELL_FOCUS, EVOCATION);
      break;
    case 10:
      increase_skills(ch, chclass, TRUE, 10);
      give_premade_feat(ch, verbose, FEAT_MAXIMIZE_SPELL, 0);
      break;
    case 11:
      increase_skills(ch, chclass, TRUE, 11);
      break;
    case 12:
      set_premade_stats(ch, chclass, 12);
      increase_skills(ch, chclass, TRUE, 12);
      give_premade_feat(ch, verbose, FEAT_GREATER_SPELL_PENETRATION, 0);
      break;
    case 13:
      increase_skills(ch, chclass, TRUE, 13);
      break;
    case 14:
      increase_skills(ch, chclass, TRUE, 14);
      break;
    case 15:
      increase_skills(ch, chclass, TRUE, 15);
      give_premade_feat(ch, verbose, FEAT_GREATER_SPELL_FOCUS, EVOCATION);
      break;
    case 16:
      set_premade_stats(ch, chclass, 16);
      increase_skills(ch, chclass, TRUE, 16);
      break;
    case 17:
      increase_skills(ch, chclass, TRUE, 17);
      break;
    case 18:
      increase_skills(ch, chclass, TRUE, 18);
      give_premade_feat(ch, verbose, FEAT_TOUGHNESS, 0);
      break;
    case 19:
      increase_skills(ch, chclass, TRUE, 19);
      break;
    case 20:
      set_premade_stats(ch, chclass, 20);
      increase_skills(ch, chclass, TRUE, 20);
      give_premade_feat(ch, verbose, FEAT_IMPROVED_FAMILIAR, 0);
      break;
  }
}

void setup_premade_levelup(struct char_data *ch, int chclass)
{
  GET_FEAT_POINTS(ch) = 0;
  GET_CLASS_FEATS(ch, chclass) = 0;
  GET_EPIC_FEAT_POINTS(ch) = 0;
  GET_EPIC_CLASS_FEATS(ch, chclass) = 0;
  GET_PRACTICES(ch) = 0;
  GET_TRAINS(ch) = 0;
  GET_BOOSTS(ch) = 0;
}

void advance_premade_build(struct char_data *ch)
{
  int chclass = 0, level = 0;
  switch (GET_PREMADE_BUILD_CLASS(ch)) {
    case CLASS_WARRIOR:
      chclass = CLASS_WARRIOR;
      setup_premade_levelup(ch, chclass);
      level = CLASS_LEVEL(ch, chclass);
      levelup_warrior(ch, level, TRUE);
      break;
    case CLASS_ROGUE:
      chclass = CLASS_ROGUE;
      setup_premade_levelup(ch, chclass);
      level = CLASS_LEVEL(ch, chclass);
      levelup_rogue(ch, level, TRUE);
      break;
    case CLASS_MONK:
      chclass = CLASS_MONK;
      setup_premade_levelup(ch, chclass);
      level = CLASS_LEVEL(ch, chclass);
      levelup_monk(ch, level, TRUE);
      break;
    case CLASS_CLERIC:
      chclass = CLASS_CLERIC;
      setup_premade_levelup(ch, chclass);
      level = CLASS_LEVEL(ch, chclass);
      levelup_cleric(ch, level, TRUE);
      break;
    case CLASS_BERSERKER:
      chclass = CLASS_BERSERKER;
      setup_premade_levelup(ch, chclass);
      level = CLASS_LEVEL(ch, chclass);
      levelup_berserker(ch, level, TRUE);
      break;
  }

}
