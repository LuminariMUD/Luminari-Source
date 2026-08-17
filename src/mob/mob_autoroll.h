#ifndef MOB_AUTOROLL_H
#define MOB_AUTOROLL_H

#include <stdbool.h>

struct char_data;

enum mob_tier
{
  MOB_TIER_STANDARD = 0,
  MOB_TIER_ELITE,
  MOB_TIER_SMALL_GROUP,
  MOB_TIER_BIG_GROUP,
  MOB_TIER_RAID,
  MOB_TIER_WORLD_BOSS,
  NUM_MOB_TIERS
};

#define MOB_TIER_UNSPECIFIED -1
#define MOB_TIER_FORMULA_V1 1
#define MOB_AUTOROLL_PROFILE_V1 1
#define MOB_AUTOROLL_CONFIG_V1 1
#define MOB_AUTOROLL_CATEGORY_COUNT 4

enum mob_autoroll_category
{
  MOB_AUTOROLL_CATEGORY_WARRIOR = 0,
  MOB_AUTOROLL_CATEGORY_ARCANE,
  MOB_AUTOROLL_CATEGORY_DIVINE,
  MOB_AUTOROLL_CATEGORY_ROGUE
};

enum mob_autoroll_custom_profile
{
  MOB_AUTOROLL_CUSTOM_NONE = 0,
  MOB_AUTOROLL_CUSTOM_TIAMAT_LIVING_V1,
  MOB_AUTOROLL_CUSTOM_TIAMAT_DRACOLICH_V1,
  NUM_MOB_AUTOROLL_CUSTOM_PROFILES
};

struct mob_autoroll_category_config
{
  int hit_points;
  int armor_class;
  int attack_bonus;
  int damage_bonus;
  int saving_throws;
  int ability_scores;
  int gold;
};

struct mob_autoroll_config
{
  int version;
  struct mob_autoroll_category_config category[MOB_AUTOROLL_CATEGORY_COUNT];
};

struct mob_autoroll_input
{
  int level;
  int race;
  int ch_class;
  int tier;
  int custom_profile;
};

struct mob_autoroll_stats
{
  int hit_points;
  int hitroll;
  int armor_class;
  int damage_dice_count;
  int damage_dice_size;
  int damage_bonus;
  int experience;
  int gold;
  int strength;
  int strength_add;
  int intelligence;
  int wisdom;
  int dexterity;
  int constitution;
  int charisma;
  int saving_fortitude;
  int saving_reflex;
  int saving_will;
  int saving_poison;
  int saving_death;
  int spell_resistance;
};

struct mob_autoroll_result
{
  int profile_version;
  int category;
  int custom_profile;
  struct mob_autoroll_stats persisted;
  struct mob_autoroll_stats expected_post_load;
};

const char *mob_tier_name(int tier);
bool mob_tier_is_valid(int tier);
int mob_tier_formula_rank(int tier);
int mob_effective_tier(const struct char_data *mob);
bool mob_tier_calculate_hit_points(int base_hit_points, int tier, int *result);
int mob_tier_attack_bonus(int tier);
int mob_tier_armor_bonus(int tier);
int mob_tier_damage_bonus(int tier);
int mob_tier_extra_attacks(int tier);
int mob_tier_critical_confirmation_bonus(int tier);
int mob_tier_defense_bypass_percent(int tier);
void mob_autoroll_default_config(struct mob_autoroll_config *config);
int mob_autoroll_class_category(int ch_class);
bool mob_autoroll_calculate(const struct mob_autoroll_input *input,
                            const struct mob_autoroll_config *config,
                            struct mob_autoroll_result *result);

void autoroll_mob(struct char_data *mob, bool realmode, bool summoned);

#endif /* MOB_AUTOROLL_H */
