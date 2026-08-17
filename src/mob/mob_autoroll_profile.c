#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "mob_autoroll.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

static bool checked_percent(int value, int percent, int *result)
{
  int64_t calculated;

  if (!result || percent < 1 || percent > 1000)
    return false;
  calculated = (int64_t)value * percent / 100;
  if (calculated < INT_MIN || calculated > INT_MAX)
    return false;
  *result = (int)calculated;
  return true;
}

static int clamp_value(int value, int minimum, int maximum)
{
  if (value < minimum)
    return minimum;
  if (value > maximum)
    return maximum;
  return value;
}

static bool valid_category_config(const struct mob_autoroll_category_config *config)
{
  if (!config)
    return false;
  return config->hit_points >= 1 && config->hit_points <= 1000 && config->armor_class >= 1 &&
         config->armor_class <= 1000 && config->attack_bonus >= 1 && config->attack_bonus <= 1000 &&
         config->damage_bonus >= 1 && config->damage_bonus <= 1000 && config->saving_throws >= 1 &&
         config->saving_throws <= 1000 && config->ability_scores >= 1 &&
         config->ability_scores <= 1000 && config->gold >= 1 && config->gold <= 1000;
}

void mob_autoroll_default_config(struct mob_autoroll_config *config)
{
  int category;

  if (!config)
    return;
  memset(config, 0, sizeof(*config));
  config->version = MOB_AUTOROLL_CONFIG_V1;
  for (category = 0; category < MOB_AUTOROLL_CATEGORY_COUNT; category++)
  {
    config->category[category].hit_points = 100;
    config->category[category].armor_class = 100;
    config->category[category].attack_bonus = 100;
    config->category[category].damage_bonus = 100;
    config->category[category].saving_throws = 100;
    config->category[category].ability_scores = 100;
    config->category[category].gold = 100;
  }
}

int mob_autoroll_class_category(int ch_class)
{
  switch (ch_class)
  {
  case CLASS_WIZARD:
  case CLASS_SORCERER:
  case CLASS_BARD:
  case CLASS_ARCANE_ARCHER:
  case CLASS_MYSTIC_THEURGE:
  case CLASS_ELDRITCH_KNIGHT:
  case CLASS_PSIONICIST:
  case CLASS_SUMMONER:
  case CLASS_WARLOCK:
  case CLASS_NECROMANCER:
  case CLASS_KNIGHT_OF_THE_THORN:
  case CLASS_ARTIFICER:
    return MOB_AUTOROLL_CATEGORY_ARCANE;

  case CLASS_CLERIC:
  case CLASS_DRUID:
  case CLASS_SHIFTER:
  case CLASS_SACRED_FIST:
  case CLASS_INQUISITOR:
  case CLASS_KNIGHT_OF_THE_SKULL:
    return MOB_AUTOROLL_CATEGORY_DIVINE;

  case CLASS_ROGUE:
  case CLASS_ALCHEMIST:
  case CLASS_ARCANE_SHADOW:
  case CLASS_SHADOW_DANCER:
  case CLASS_ASSASSIN:
    return MOB_AUTOROLL_CATEGORY_ROGUE;

  default:
    return MOB_AUTOROLL_CATEGORY_WARRIOR;
  }
}

static bool apply_category_config(struct mob_autoroll_stats *stats,
                                  const struct mob_autoroll_category_config *config)
{
  int armor_adjustment;
  int save_adjustment;

  if (!stats || !valid_category_config(config))
    return false;
  if (!checked_percent(stats->hit_points, config->hit_points, &stats->hit_points) ||
      !checked_percent(stats->hitroll, config->attack_bonus, &stats->hitroll) ||
      !checked_percent(stats->damage_bonus, config->damage_bonus, &stats->damage_bonus) ||
      !checked_percent(stats->gold, config->gold, &stats->gold))
    return false;

  armor_adjustment = (int)((int64_t)stats->armor_class * (100 - config->armor_class) / 100);
  stats->armor_class -= armor_adjustment;

#define APPLY_SAVE(field)                                                                          \
  do                                                                                               \
  {                                                                                                \
    save_adjustment = (int)((int64_t)(field) * (100 - config->saving_throws) / 100);               \
    (field) -= save_adjustment;                                                                    \
  } while (0)
  APPLY_SAVE(stats->saving_fortitude);
  APPLY_SAVE(stats->saving_reflex);
  APPLY_SAVE(stats->saving_will);
  APPLY_SAVE(stats->saving_poison);
  APPLY_SAVE(stats->saving_death);
#undef APPLY_SAVE

#define APPLY_ABILITY(field)                                                                       \
  do                                                                                               \
  {                                                                                                \
    if (!checked_percent((field), config->ability_scores, &(field)))                               \
      return false;                                                                                \
    (field) = clamp_value((field), 1, 50);                                                         \
  } while (0)
  APPLY_ABILITY(stats->strength);
  APPLY_ABILITY(stats->intelligence);
  APPLY_ABILITY(stats->wisdom);
  APPLY_ABILITY(stats->dexterity);
  APPLY_ABILITY(stats->constitution);
  APPLY_ABILITY(stats->charisma);
#undef APPLY_ABILITY
  return true;
}

static void apply_class_profile(struct mob_autoroll_stats *stats, int ch_class, int bonus)
{
  switch (ch_class)
  {
  case CLASS_WIZARD:
    stats->hit_points = stats->hit_points * 2 / 5;
    stats->damage_dice_size = stats->damage_dice_size * 2 / 5;
    stats->armor_class -= 60;
    stats->intelligence += bonus;
    stats->dexterity += bonus;
    break;
  case CLASS_PSIONICIST:
    stats->hit_points = stats->hit_points * 2 / 5;
    stats->damage_dice_size = stats->damage_dice_size * 2 / 5;
    stats->armor_class -= 60;
    stats->intelligence += bonus;
    stats->dexterity += bonus;
    break;
  case CLASS_SORCERER:
  case CLASS_NECROMANCER:
    stats->charisma += bonus;
    stats->dexterity += bonus;
    stats->hit_points = stats->hit_points * 2 / 5;
    stats->damage_dice_size = stats->damage_dice_size * 2 / 5;
    stats->armor_class -= 60;
    break;
  case CLASS_ROGUE:
    stats->dexterity += bonus;
    stats->strength += bonus;
    stats->hit_points = stats->hit_points * 3 / 5;
    stats->armor_class -= 50;
    break;
  case CLASS_BARD:
    stats->charisma += bonus;
    stats->dexterity += bonus;
    stats->damage_dice_size = stats->damage_dice_size * 4 / 5;
    stats->hit_points = stats->hit_points * 3 / 5;
    stats->armor_class -= 50;
    break;
  case CLASS_MONK:
    stats->wisdom += bonus;
    stats->dexterity += bonus;
    stats->hit_points = stats->hit_points * 4 / 5;
    stats->armor_class -= 60;
    break;
  case CLASS_CLERIC:
    stats->strength += bonus;
    stats->wisdom += bonus;
    stats->damage_dice_size = stats->damage_dice_size * 4 / 5;
    stats->hit_points = stats->hit_points * 4 / 5;
    stats->armor_class -= 10;
    break;
  case CLASS_DRUID:
  case CLASS_SHIFTER:
    stats->wisdom += bonus;
    stats->dexterity += bonus;
    stats->damage_dice_size = stats->damage_dice_size * 4 / 5;
    stats->hit_points = stats->hit_points * 4 / 5;
    stats->armor_class -= 50;
    break;
  case CLASS_BERSERKER:
  case CLASS_STALWART_DEFENDER:
    stats->strength += bonus;
    stats->constitution += bonus;
    stats->hit_points = stats->hit_points * 6 / 5;
    stats->armor_class -= 40;
    break;
  case CLASS_RANGER:
  case CLASS_DUELIST:
    stats->strength += bonus;
    stats->dexterity += bonus;
    stats->armor_class -= 50;
    break;
  case CLASS_SACRED_FIST:
    stats->wisdom += bonus;
    stats->dexterity += bonus;
    stats->armor_class -= 50;
    break;
  case CLASS_WARRIOR:
    stats->strength += bonus;
    stats->constitution += bonus;
    break;
  case CLASS_WEAPON_MASTER:
    stats->strength += bonus;
    stats->dexterity += bonus;
    break;
  case CLASS_ARCANE_ARCHER:
    stats->intelligence += bonus;
    stats->dexterity += bonus;
    stats->charisma += bonus;
    break;
  case CLASS_ARCANE_SHADOW:
    stats->intelligence += bonus * 2;
    stats->dexterity += bonus;
    break;
  case CLASS_ELDRITCH_KNIGHT:
    stats->intelligence += bonus * 2;
    stats->strength += bonus;
    break;
  case CLASS_PALADIN:
    stats->strength += bonus;
    stats->charisma += bonus;
    break;
  case CLASS_MYSTIC_THEURGE:
    stats->hit_points = stats->hit_points * 3 / 5;
    stats->damage_dice_size = stats->damage_dice_size * 3 / 5;
    stats->armor_class -= 60;
    stats->intelligence += bonus;
    stats->dexterity += bonus;
    stats->strength += bonus;
    stats->wisdom += bonus;
    break;
  default:
    stats->hit_points = stats->hit_points * 2 / 5;
    stats->damage_dice_size = stats->damage_dice_size * 2 / 5;
    stats->armor_class -= 60;
    break;
  }
}

static void apply_race_profile(struct mob_autoroll_stats *stats, int race, int level)
{
  switch (race)
  {
  case RACE_TYPE_ANIMAL:
    stats->intelligence -= 7;
    stats->wisdom -= 7;
    stats->charisma -= 7;
    stats->saving_fortitude += 4;
    stats->saving_reflex += 4;
    stats->gold = 0;
    break;
  case RACE_TYPE_DRAGON:
    stats->dexterity += 6;
    stats->strength += 6;
    stats->constitution += 6;
    stats->charisma += 6;
    stats->intelligence += 6;
    stats->wisdom += 6;
    stats->saving_fortitude += 4;
    stats->saving_reflex += 4;
    stats->saving_will += 4;
    stats->spell_resistance = 10 + level;
    break;
  case RACE_TYPE_GIANT:
    stats->strength += 4;
    stats->constitution += 4;
    stats->dexterity -= 7;
    break;
  case RACE_TYPE_ABERRATION:
    stats->saving_will += 4;
    break;
  case RACE_TYPE_CONSTRUCT:
    stats->strength += 4;
    stats->constitution += 4;
    stats->saving_will -= 4;
    stats->saving_fortitude -= 4;
    stats->saving_reflex -= 4;
    break;
  case RACE_TYPE_FEY:
    stats->saving_reflex += 4;
    stats->saving_will += 4;
    break;
  case RACE_TYPE_MAGICAL_BEAST:
    stats->saving_fortitude += 4;
    stats->saving_reflex += 4;
    break;
  case RACE_TYPE_OOZE:
    stats->saving_will -= 4;
    stats->saving_fortitude -= 4;
    stats->saving_reflex -= 4;
    break;
  case RACE_TYPE_PLANT:
  case RACE_TYPE_VERMIN:
    stats->gold = 0;
    break;
  default:
    break;
  }
}

bool mob_autoroll_calculate(const struct mob_autoroll_input *input,
                            const struct mob_autoroll_config *config,
                            struct mob_autoroll_result *result)
{
  struct mob_autoroll_stats *stats;
  int category;
  int level;
  int bonus;
  int configured_category;

  if (!input || !config || !result || config->version != MOB_AUTOROLL_CONFIG_V1 ||
      input->level < 1 || input->level > LVL_IMPL || input->race < 0 ||
      input->race >= NUM_RACE_TYPES || input->ch_class < 0 || input->ch_class >= NUM_CLASSES ||
      !mob_tier_is_valid(input->tier) || input->custom_profile < MOB_AUTOROLL_CUSTOM_NONE ||
      input->custom_profile >= NUM_MOB_AUTOROLL_CUSTOM_PROFILES)
    return false;
  for (configured_category = 0; configured_category < MOB_AUTOROLL_CATEGORY_COUNT;
       configured_category++)
    if (!valid_category_config(&config->category[configured_category]))
      return false;

  memset(result, 0, sizeof(*result));
  result->profile_version = MOB_AUTOROLL_PROFILE_V1;
  result->custom_profile = input->custom_profile;
  category = mob_autoroll_class_category(input->ch_class);
  result->category = category;
  stats = &result->persisted;
  level = input->level;
  bonus = level / 2;

  stats->hit_points = level * level + level * 10;
  stats->hitroll = level / 6 + 1;
  stats->armor_class = 100 + level * 10;
  stats->damage_dice_count = 1;
  stats->damage_dice_size = level;
  stats->damage_bonus = level / 6 + 1;
  stats->experience = level * level * 75;
  stats->gold = level * 10;
  stats->strength = 10;
  stats->intelligence = 10;
  stats->wisdom = 10;
  stats->dexterity = 10;
  stats->constitution = 10;
  stats->charisma = 10;
  stats->saving_fortitude = level / 4;
  stats->saving_reflex = level / 4;
  stats->saving_will = level / 4;
  stats->saving_poison = level / 4;
  stats->saving_death = level / 4;

  apply_class_profile(stats, input->ch_class, bonus);
  apply_race_profile(stats, input->race, level);
  if (!mob_tier_calculate_hit_points(stats->hit_points, input->tier, &stats->hit_points))
    return false;
  stats->damage_bonus += mob_tier_damage_bonus(input->tier);
  stats->armor_class += mob_tier_armor_bonus(input->tier) * 10;
  stats->damage_dice_size = stats->damage_dice_size < 4 ? 4 : stats->damage_dice_size;
  switch (input->custom_profile)
  {
  case MOB_AUTOROLL_CUSTOM_NONE:
    break;
  case MOB_AUTOROLL_CUSTOM_TIAMAT_LIVING_V1:
    stats->hit_points = 29999;
    break;
  case MOB_AUTOROLL_CUSTOM_TIAMAT_DRACOLICH_V1:
    stats->hit_points = 30000;
    break;
  default:
    return false;
  }

  result->expected_post_load = *stats;
  if (input->custom_profile == MOB_AUTOROLL_CUSTOM_NONE &&
      !apply_category_config(&result->expected_post_load, &config->category[category]))
    return false;
  return true;
}
