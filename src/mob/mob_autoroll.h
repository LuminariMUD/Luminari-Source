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

void autoroll_mob(struct char_data *mob, bool realmode, bool summoned);

#endif /* MOB_AUTOROLL_H */
