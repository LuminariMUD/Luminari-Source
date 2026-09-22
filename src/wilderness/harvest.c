#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "harvest.h"
#include "resource_system.h"
#include "resource_depletion.h"
#include "wilderness.h"
#include "events/activity_manager.h"
#include "events/actions.h"
#include "core/comm.h"
#include "core/constants.h"
#include "craft/crafting_new.h"
#include "craft/craft.h"
#include "core/db.h"
#include "events/domain_event_world.h"
#include "config/harvest_vnums.h"
#include "magic/spells.h"
#include "core/interpreter.h"

#include <limits.h>

/* One wilderness attempt: a named material (targeted gathering) or, with material zero, a
 * mote category harvest. */
struct wilderness_harvest_context
{
  int category;
  int material;
  int x;
  int y;
};

/** @brief A currently valid wilderness room, for any character. */
static bool in_wilderness_room(struct char_data *ch)
{
  return world && zone_table && ch && IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) <= top_of_world &&
         world[IN_ROOM(ch)].zone <= top_of_zone_table &&
         ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS);
}

/** @brief Accept only player characters in a currently valid wilderness room. */
static bool harvest_location_valid(struct char_data *ch)
{
  return world && zone_table && ch && !IS_NPC(ch) && ch->player_specials &&
         IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) <= top_of_world &&
         world[IN_ROOM(ch)].zone <= top_of_zone_table &&
         ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS);
}

/** @brief Check terrain and remaining resources, optionally explaining an unavailable category. */
bool wilderness_harvest_available(struct char_data *ch, int category, bool verbose)
{
  int x, y, sector;

  if (!harvest_location_valid(ch) || category < 0 || category >= NUM_RESOURCE_TYPES)
    return false;
  x = world[IN_ROOM(ch)].coords[0];
  y = world[IN_ROOM(ch)].coords[1];
  sector = get_modified_sector_type(world[IN_ROOM(ch)].zone, x, y);
  if (!can_harvest_resource_in_terrain(category, sector))
  {
    if (verbose)
      send_to_char(ch, "You cannot harvest %s in this terrain.\r\n", resource_names[category]);
    return false;
  }
  if (should_harvest_fail_due_to_depletion(IN_ROOM(ch), category) ||
      calculate_current_resource_level(category, x, y) < 0.1)
  {
    if (verbose)
      send_to_char(ch, "There are insufficient %s resources here to harvest.\r\n",
                   resource_names[category]);
    return false;
  }
  return true;
}

/** @brief Convert a real tool prototype VNUM to its quality floor, or zero for another object. */
static int tool_quality(struct obj_data *obj)
{
  obj_vnum vnum;

  if (!obj_index || !obj || GET_OBJ_RNUM(obj) == NOTHING || GET_OBJ_RNUM(obj) > top_of_objt)
    return 0;
  vnum = GET_OBJ_VNUM(obj);
  if (vnum < HARVEST_TOOL_FIRST || vnum > HARVEST_TOOL_LAST)
    return 0;
  return MATERIAL_QUALITY_POOR + vnum - HARVEST_TOOL_FIRST;
}

/** @brief Return the best tool floor in top-level inventory or equipment without consuming it. */
int wilderness_harvest_tool_quality(struct char_data *ch)
{
  struct obj_data *obj;
  int quality = 0, slot;

  if (!ch)
    return 0;
  for (obj = ch->carrying; obj; obj = obj->next_content)
    quality = MAX(quality, tool_quality(obj));
  for (slot = 0; slot < NUM_WEARS; slot++)
    quality = MAX(quality, tool_quality(GET_EQ(ch, slot)));
  return quality;
}

/**
 * @brief The pre-merge quality ladder: map a validated wilderness quality to a crafting
 * material, or CRAFT_MAT_NONE. Live harvesting no longer uses it; it is the frozen
 * compatibility reader for old wilderness holdings (consolidation Decision 10).
 */
int wilderness_harvest_material(int category, int subtype, int quality)
{
  static const int cloth[] = {CRAFT_MAT_HEMP, CRAFT_MAT_FLAX, CRAFT_MAT_WOOL, CRAFT_MAT_SILK,
                              CRAFT_MAT_SATIN};
  static const int ore[] = {CRAFT_MAT_TIN, CRAFT_MAT_BRONZE, CRAFT_MAT_IRON, CRAFT_MAT_STEEL,
                            CRAFT_MAT_MITHRIL};
  static const int wood[] = {CRAFT_MAT_ASH_WOOD, CRAFT_MAT_MAPLE_WOOD, CRAFT_MAT_MAHAGONY_WOOD,
                             CRAFT_MAT_VALENWOOD, CRAFT_MAT_IRONWOOD};
  static const int hides[] = {CRAFT_MAT_LOW_GRADE_HIDE, CRAFT_MAT_MEDIUM_GRADE_HIDE,
                              CRAFT_MAT_HIGH_GRADE_HIDE, CRAFT_MAT_PRISTINE_GRADE_HIDE,
                              CRAFT_MAT_DRAGONSCALE};
  int tier;

  if (!validate_material_data(category, subtype, quality))
    return CRAFT_MAT_NONE;
  tier = quality - MATERIAL_QUALITY_POOR;
  switch (category)
  {
  case RESOURCE_VEGETATION:
    return cloth[tier];
  case RESOURCE_MINERALS:
    if (quality == MATERIAL_QUALITY_LEGENDARY && subtype == ORE_ADAMANTINE)
      return CRAFT_MAT_ADAMANTINE;
    if (quality == MATERIAL_QUALITY_RARE && subtype == ORE_COLD_IRON)
      return CRAFT_MAT_COLD_IRON;
    return ore[tier];
  case RESOURCE_WOOD:
    return wood[tier];
  case RESOURCE_GAME:
    return hides[tier];
  default:
    return CRAFT_MAT_NONE;
  }
}

/**
 * @brief Select a useful mote for resources without graded materials; zero means no mapping.
 * Payout preserves quality as one through five motes per raw unit.
 */
int wilderness_harvest_mote(int category, int subtype)
{
  static const int crystals[NUM_CRYSTAL_SUBTYPES] = {
      CRAFTING_MOTE_AIR,   CRAFTING_MOTE_DARK, CRAFTING_MOTE_FIRE, CRAFTING_MOTE_DARK,
      CRAFTING_MOTE_LIGHT, CRAFTING_MOTE_FIRE, CRAFTING_MOTE_ICE,  CRAFTING_MOTE_LIGHTNING};

  if (!validate_material_data(category, subtype, MATERIAL_QUALITY_POOR))
    return 0;
  switch (category)
  {
  case RESOURCE_WATER:
    return CRAFTING_MOTE_WATER;
  case RESOURCE_HERBS:
    return CRAFTING_MOTE_LIGHT;
  case RESOURCE_CRYSTAL:
    return crystals[subtype];
  case RESOURCE_SALT:
    return CRAFTING_MOTE_ICE;
  case RESOURCE_STONE:
  case RESOURCE_CLAY:
    return CRAFTING_MOTE_EARTH;
  default:
    return 0;
  }
}

/**
 * @brief Credit one crafting balance and return raw units delivered, or zero on invalid/capped data.
 * This function does not deplete resources, grant experience, or duplicate wilderness storage.
 */
int award_wilderness_harvest(struct char_data *ch, int category, int subtype, int quality,
                             int quantity)
{
  int material, mote, amount;

  if (!ch || IS_NPC(ch) || !ch->player_specials || quantity <= 0 ||
      !validate_material_data(category, subtype, quality))
    return 0;
  material = wilderness_harvest_material(category, subtype, quality);
  if (material != CRAFT_MAT_NONE)
  {
    if (!craft_balance_add(ch, material, quantity))
      return 0;
    send_to_char(ch, "You harvest %d units of %s (%s quality).\r\n", quantity,
                 crafting_materials[material], get_material_quality_name(quality));
    return quantity;
  }
  mote = wilderness_harvest_mote(category, subtype);
  if (!mote || quantity > INT_MAX / quality)
    return 0;
  amount = quantity * quality;
  if (!craft_mote_add(ch, mote, amount))
    return 0;
  send_to_char(ch, "You harvest %d units of %s, yielding %d %ss.\r\n", quantity,
               get_full_material_name(category, subtype, quality), amount, crafting_motes[mote]);
  return quantity;
}

/** @brief Select the existing crafting ability that governs a resource category. */
static int crafting_harvest_skill(int category)
{
  switch (category)
  {
  case RESOURCE_MINERALS:
  case RESOURCE_STONE:
  case RESOURCE_CRYSTAL:
  case RESOURCE_CLAY:
  case RESOURCE_SALT:
    return ABILITY_HARVEST_MINING;
  case RESOURCE_WOOD:
    return ABILITY_HARVEST_FORESTRY;
  case RESOURCE_GAME:
    return ABILITY_HARVEST_HUNTING;
  default:
    return ABILITY_HARVEST_GATHERING;
  }
}

/* ---- The material pool ---- */

/** @brief The resource category a pool material is harvested from, or -1. */
int wilderness_material_category(int material)
{
  if (!wilderness_pool_material(material))
    return -1;
  if (material == CRAFT_MAT_COAL) /* mined, but outside the metal groups */
    return RESOURCE_MINERALS;
  switch (craft_group_by_material(material))
  {
  case CRAFT_GROUP_HARD_METALS:
  case CRAFT_GROUP_SOFT_METALS:
  case CRAFT_GROUP_STONE:
    return RESOURCE_MINERALS;
  case CRAFT_GROUP_WOOD:
    return RESOURCE_WOOD;
  case CRAFT_GROUP_HIDES:
    return RESOURCE_GAME;
  case CRAFT_GROUP_CLOTH:
    return RESOURCE_VEGETATION;
  default:
    return -1;
  }
}

/** @brief True for the four categories that yield named materials rather than motes. */
static bool material_category(int category)
{
  return category == RESOURCE_VEGETATION || category == RESOURCE_MINERALS ||
         category == RESOURCE_WOOD || category == RESOURCE_GAME;
}

static int pool_order(const void *left, const void *right)
{
  int a = *(const int *)left, b = *(const int *)right;
  int category_a = wilderness_material_category(a), category_b = wilderness_material_category(b);

  if (category_a != category_b)
    return category_a - category_b;
  if (material_grade(a) != material_grade(b))
    return material_grade(a) - material_grade(b);
  return strcmp(crafting_materials[a], crafting_materials[b]);
}

/** @brief The pool materials a sector allows, in category, grade, then name order. */
int wilderness_sector_material_pool(int sector, int *out, int max)
{
  int material, category, count = 0;

  if (!out || max <= 0)
    return 0;
  for (material = 1; material < NUM_CRAFT_MATS && count < max; material++)
  {
    category = wilderness_material_category(material);
    if (category < 0 || !can_harvest_resource_in_terrain(category, sector))
      continue;
    out[count++] = material;
  }
  qsort(out, count, sizeof(int), pool_order);
  return count;
}

/** @brief The pool at the character's coordinate, optionally limited to one category. */
int wilderness_material_pool(struct char_data *ch, int category, int *out, int max)
{
  int all[NUM_CRAFT_MATS], total, i, count = 0, sector;

  if (!harvest_location_valid(ch) || !out || max <= 0)
    return 0;
  sector = get_modified_sector_type(world[IN_ROOM(ch)].zone, world[IN_ROOM(ch)].coords[0],
                                    world[IN_ROOM(ch)].coords[1]);
  total = wilderness_sector_material_pool(sector, all, NUM_CRAFT_MATS);
  for (i = 0; i < total && count < max; i++)
    if (category < 0 || wilderness_material_category(all[i]) == category)
      out[count++] = all[i];
  return count;
}

/* ---- Quality tier and difficulty ---- */

/** @brief The richness tier of a resource level: the 0.3, 0.5, 0.7, 0.9 bands. */
int wilderness_richness_tier(double level)
{
  if (level >= 0.9)
    return MATERIAL_QUALITY_LEGENDARY;
  if (level >= 0.7)
    return MATERIAL_QUALITY_RARE;
  if (level >= 0.5)
    return MATERIAL_QUALITY_UNCOMMON;
  if (level >= 0.3)
    return MATERIAL_QUALITY_COMMON;
  return MATERIAL_QUALITY_POOR;
}

/** @brief The tier for one attempt: the highest of the skill roll, the richness, and the tool. */
int wilderness_quality_tier_from(int skill_tier, double level, int tool_tier)
{
  int tier = MAX(MATERIAL_QUALITY_POOR, MAX(skill_tier, wilderness_richness_tier(level)));

  tier = MAX(tier, tool_tier);
  return MIN(MATERIAL_QUALITY_LEGENDARY, tier);
}

int wilderness_quality_tier(struct char_data *ch, int category, int success, int rank)
{
  double level;

  if (!harvest_location_valid(ch))
    return MATERIAL_QUALITY_POOR;
  level = calculate_current_resource_level(category, world[IN_ROOM(ch)].coords[0],
                                           world[IN_ROOM(ch)].coords[1]);
  return wilderness_quality_tier_from(calculate_harvest_quality(ch, category, success, rank), level,
                                      wilderness_harvest_tool_quality(ch));
}

/** @brief Rarer is harder: the category difficulty plus five per grade. */
int wilderness_material_difficulty(int category, double level, int material)
{
  return get_harvest_difficulty(category, level) + material_grade(material) * 5;
}

bool wilderness_material_reachable(int material, int tier)
{
  return material_grade(material) <= tier;
}

/** @brief Why a material cannot be harvested here, or NULL when it can. */
const char *wilderness_material_refusal(struct char_data *ch, int material)
{
  int category, sector;

  if (!wilderness_pool_material(material))
    return "is not something you can harvest in the wilderness";
  category = wilderness_material_category(material);
  if (!harvest_location_valid(ch))
    return "can only be harvested in the wilderness";
  sector = get_modified_sector_type(world[IN_ROOM(ch)].zone, world[IN_ROOM(ch)].coords[0],
                                    world[IN_ROOM(ch)].coords[1]);
  if (!can_harvest_resource_in_terrain(category, sector))
    return "cannot be found in this terrain";
  if (!wilderness_harvest_available(ch, category, false))
    return "is depleted here";
  return NULL;
}

/** @brief Match a pool material by name: an exact name first, then the first abbreviation. */
int wilderness_parse_material(const char *arg)
{
  int material;

  if (!arg || !*arg)
    return CRAFT_MAT_NONE;
  for (material = 1; material < NUM_CRAFT_MATS; material++)
    if (wilderness_pool_material(material) && !str_cmp(arg, crafting_materials[material]))
      return material;
  for (material = 1; material < NUM_CRAFT_MATS; material++)
    if (wilderness_pool_material(material) && is_abbrev(arg, crafting_materials[material]))
      return material;
  return CRAFT_MAT_NONE;
}

/* ---- Listing and command dispatch ---- */

/** @brief Whether a verb accepts a category: gather takes plants and game, mine the ground. */
static bool mode_accepts(int mode, int category)
{
  switch (mode)
  {
  case WILDERNESS_CMD_GATHER:
    return category == RESOURCE_VEGETATION || category == RESOURCE_GAME ||
           category == RESOURCE_HERBS;
  case WILDERNESS_CMD_MINE:
    return category == RESOURCE_MINERALS || category == RESOURCE_CRYSTAL ||
           category == RESOURCE_STONE || category == RESOURCE_SALT;
  default:
    return true;
  }
}

static const char *mode_verb(int mode)
{
  return mode == WILDERNESS_CMD_GATHER ? "gather"
         : mode == WILDERNESS_CMD_MINE ? "mine"
                                       : "harvest";
}

/** @brief One category's pool line: grade after each name, a star on guaranteed grades. */
static void show_pool_line(struct char_data *ch, int category, int guaranteed)
{
  int pool[NUM_CRAFT_MATS], count, i, x, y;
  double level, depletion;

  x = world[IN_ROOM(ch)].coords[0];
  y = world[IN_ROOM(ch)].coords[1];
  level = calculate_current_resource_level(category, x, y);
  depletion = get_resource_depletion_level(IN_ROOM(ch), category);
  count = wilderness_material_pool(ch, category, pool, NUM_CRAFT_MATS);
  send_to_char(ch, "  \tG%-12s\tn (%s):", resource_names[category],
               get_abundance_description(level * depletion));
  for (i = 0; i < count; i++)
    send_to_char(ch, "%s %s[%d]%s", i ? "," : "", crafting_materials[pool[i]],
                 material_grade(pool[i]), material_grade(pool[i]) <= guaranteed ? "*" : "");
  send_to_char(ch, "\r\n");
}

/** @brief List what the verb can take here: material pools with grades, mote categories. */
void wilderness_show_pools(struct char_data *ch, int mode)
{
  static const int materials[] = {RESOURCE_VEGETATION, RESOURCE_MINERALS, RESOURCE_WOOD,
                                  RESOURCE_GAME};
  int i, x, y, guaranteed, listed = 0;
  double level;

  if (!in_wilderness_room(ch))
  {
    send_to_char(ch, "You can only %s materials in the wilderness.\r\n", mode_verb(mode));
    return;
  }
  x = world[IN_ROOM(ch)].coords[0];
  y = world[IN_ROOM(ch)].coords[1];
  send_to_char(ch, "Harvestable resources at this location:\r\n");
  send_to_char(ch, "=====================================\r\n");
  for (i = 0; i < 4; i++)
  {
    if (!mode_accepts(mode, materials[i]) || !wilderness_harvest_available(ch, materials[i], false))
      continue;
    level = calculate_current_resource_level(materials[i], x, y);
    guaranteed = wilderness_quality_tier_from(MATERIAL_QUALITY_POOR, level,
                                              wilderness_harvest_tool_quality(ch));
    show_pool_line(ch, materials[i], guaranteed);
    listed++;
  }
  for (i = 0; i < NUM_RESOURCE_TYPES; i++)
  {
    if (material_category(i) || !mode_accepts(mode, i) ||
        !wilderness_harvest_available(ch, i, false))
      continue;
    level =
        calculate_current_resource_level(i, x, y) * get_resource_depletion_level(IN_ROOM(ch), i);
    send_to_char(ch, "  \tG%-12s\tn: %s (%s %s)\r\n", resource_names[i],
                 get_abundance_description(level), mode_verb(mode), resource_names[i]);
    listed++;
  }
  if (!listed)
    send_to_char(ch, "  Nothing can be harvested here right now.\r\n");
  send_to_char(ch,
               "\r\nGrades in brackets; * marks grades your tools and this spot guarantee.\r\n");
  send_to_char(ch, "Usage: %s <material>   (a category name lists its materials)\r\n",
               mode_verb(mode));
}

/** @brief harvest, gather, and mine share this: name a material to start, a category to list. */
void wilderness_harvest_command(struct char_data *ch, const char *argument, int mode)
{
  char arg[MAX_INPUT_LENGTH];
  const char *reason;
  int material, category, pool[NUM_CRAFT_MATS];

  if (!in_wilderness_room(ch))
  {
    send_to_char(ch, "You can only %s materials in the wilderness.\r\n", mode_verb(mode));
    return;
  }
  while (argument && *argument == ' ')
    argument++;
  snprintf(arg, sizeof(arg), "%s", argument ? argument : "");
  if (!*arg)
  {
    wilderness_show_pools(ch, mode);
    return;
  }

  /* "stone" is the material where minerals are allowed, otherwise the earth-mote category. */
  category = parse_resource_type(arg);
  if (category == RESOURCE_STONE && mode_accepts(mode, RESOURCE_MINERALS) &&
      wilderness_material_refusal(ch, CRAFT_MAT_STONE) == NULL)
    category = -1;

  material = category < 0 ? wilderness_parse_material(arg) : CRAFT_MAT_NONE;
  if (material != CRAFT_MAT_NONE)
  {
    if (!mode_accepts(mode, wilderness_material_category(material)))
    {
      send_to_char(ch, "You cannot %s %s; try 'harvest %s'.\r\n", mode_verb(mode),
                   crafting_materials[material], crafting_materials[material]);
      return;
    }
    reason = wilderness_material_refusal(ch, material);
    if (reason != NULL)
    {
      send_to_char(ch, "%c%s %s.\r\n", UPPER(*crafting_materials[material]),
                   crafting_materials[material] + 1, reason);
      return;
    }
    start_wilderness_material_harvest(ch, material);
    return;
  }

  if (category < 0)
  {
    send_to_char(ch, "You cannot %s that. Type '%s' to see what is available here.\r\n",
                 mode_verb(mode), mode_verb(mode));
    return;
  }
  if (!mode_accepts(mode, category))
  {
    send_to_char(ch, "You cannot %s %s here; try 'harvest %s'.\r\n", mode_verb(mode),
                 resource_names[category], resource_names[category]);
    return;
  }
  if (material_category(category))
  {
    /* A material category lists its pool and starts nothing. */
    if (!wilderness_harvest_available(ch, category, true))
      return;
    if (wilderness_material_pool(ch, category, pool, NUM_CRAFT_MATS) == 0)
    {
      send_to_char(ch, "Nothing of that kind can be harvested here.\r\n");
      return;
    }
    show_pool_line(ch, category,
                   wilderness_quality_tier_from(
                       MATERIAL_QUALITY_POOR,
                       calculate_current_resource_level(category, world[IN_ROOM(ch)].coords[0],
                                                        world[IN_ROOM(ch)].coords[1]),
                       wilderness_harvest_tool_quality(ch)));
    send_to_char(ch, "Name one of them to start, for example '%s %s'.\r\n", mode_verb(mode),
                 crafting_materials[pool[0]]);
    return;
  }
  start_wilderness_crafting_harvest(ch, category);
}

/* ---- The activity ---- */

/** @brief Require standing, unrestrained players outside combat and legacy crafting work. */
static bool harvest_conditions(struct char_data *ch)
{
  return harvest_location_valid(ch) && !FIGHTING(ch) && GET_POS(ch) >= POS_STANDING &&
         !AFF_FLAGGED(ch, AFF_GRAPPLED) && !AFF_FLAGGED(ch, AFF_ENTANGLED) &&
         GET_CRAFT(ch).craft_duration == 0;
}

/** @brief Cancel eligibility when the toggle, target room, coordinates, or resources change. */
static bool harvest_recheck(struct char_data *ch, void *target, void *context)
{
  struct wilderness_harvest_context *harvest = context;

  return harvest_conditions(ch) && target == &world[IN_ROOM(ch)] &&
         world[IN_ROOM(ch)].coords[0] == harvest->x && world[IN_ROOM(ch)].coords[1] == harvest->y &&
         wilderness_harvest_available(ch, harvest->category, true) &&
         (harvest->material == CRAFT_MAT_NONE ||
          wilderness_material_refusal(ch, harvest->material) == NULL);
}

/** @brief The rank an attempt rolls with: ability, proficient talent, and the Miner feat. */
static int harvest_rank(struct char_data *ch, int category, int skill)
{
  int rank = get_craft_skill_value(ch, skill) + get_proficient_talent_bonus(ch, skill);

  if ((category == RESOURCE_MINERALS || category == RESOURCE_STONE ||
       category == RESOURCE_CRYSTAL) &&
      HAS_FEAT(ch, FEAT_MINER))
    rank += 4;
  return rank;
}

/** @brief Node-style bonus motes accompany material rewards on a natural 100 or one in five. */
static void harvest_bonus_motes(struct char_data *ch, int roll, int quality)
{
  int mote, motes;

  if (roll != 100 && dice(1, 100) > 20)
    return;
  mote = dice(1, NUM_CRAFT_MOTES - 1);
  motes = dice(quality, 4) * 150 / 100;
  if (craft_mote_add(ch, mote, motes))
    send_to_char(ch, "You also extract %d %ss.\r\n", motes, crafting_motes[mote]);
}

/** @brief Resolve a named material after the full round: roll, grade access, credit, deplete. */
static void complete_material_harvest(struct char_data *ch,
                                      struct wilderness_harvest_context *harvest)
{
  int category = harvest->category, material = harvest->material;
  int skill, rank, roll, success, tier, quantity, grade;
  double level;

  skill = harvesting_skill_by_material(material);
  rank = harvest_rank(ch, category, skill);
  level = calculate_current_resource_level(category, harvest->x, harvest->y);
  roll = dice(1, 100);
  success = (int)((roll + rank) * get_harvest_success_modifier(IN_ROOM(ch), category));
  if (success < wilderness_material_difficulty(category, level, material))
  {
    send_to_char(ch, "You fail to harvest any usable %s.\r\n", crafting_materials[material]);
    apply_harvest_depletion(IN_ROOM(ch), category, 1);
    gain_craft_exp(ch, 10, skill, true);
    return;
  }
  grade = material_grade(material);
  tier = wilderness_quality_tier(ch, category, success, rank);
  if (!wilderness_material_reachable(material, tier))
  {
    send_to_char(ch, "The %s here is beyond your reach this time (grade %d, you reached %d).\r\n",
                 crafting_materials[material], grade, tier);
    gain_craft_exp(ch, 10, skill, true);
    return;
  }
  quantity = dice(2, 2);
  if (roll == 100)
    quantity += dice(2, 2);
  if (rand_number(1, 100) <= get_efficient_talent_bonus(ch, skill))
    quantity += 2;
  if (!craft_balance_add(ch, material, quantity))
  {
    send_to_char(ch, "Your crafting storage cannot hold this harvest.\r\n");
    return;
  }
  send_to_char(ch, "You harvest %d units of %s (grade %d).\r\n", quantity,
               crafting_materials[material], grade);
  apply_harvest_depletion_with_cascades(IN_ROOM(ch), category, quantity);
  update_conservation_score(ch, category, quantity <= 2);
  gain_craft_exp(ch, 20 + 10 * grade, skill, true);
  harvest_bonus_motes(ch, roll, tier);
}

/** @brief Resolve a mote category after the full round, as before targeted gathering. */
static void complete_mote_harvest(struct char_data *ch, struct wilderness_harvest_context *harvest)
{
  int category = harvest->category;
  int skill, rank, roll, success, quality, subtype, quantity;
  double level;

  skill = crafting_harvest_skill(category);
  rank = harvest_rank(ch, category, skill);
  level = calculate_current_resource_level(category, harvest->x, harvest->y);
  roll = dice(1, 100);
  success = (int)((roll + rank) * get_harvest_success_modifier(IN_ROOM(ch), category));
  if (success < get_harvest_difficulty(category, level))
  {
    send_to_char(ch, "You fail to harvest any usable %s.\r\n", resource_names[category]);
    apply_harvest_depletion(IN_ROOM(ch), category, 1);
    gain_craft_exp(ch, 10, skill, true);
    return;
  }
  subtype = determine_harvested_material_subtype(category, harvest->x, harvest->y, level);
  quality = calculate_harvest_quality(ch, category, success, rank);
  quality = MAX(quality, wilderness_harvest_tool_quality(ch));
  quantity = dice(2, 2);
  if (roll == 100)
    quantity += dice(2, 2);
  if (rand_number(1, 100) <= get_efficient_talent_bonus(ch, skill))
    quantity += 2;
  if (!award_wilderness_harvest(ch, category, subtype, quality, quantity))
  {
    send_to_char(ch, "Your crafting storage cannot hold this harvest.\r\n");
    return;
  }
  apply_harvest_depletion_with_cascades(IN_ROOM(ch), category, quantity);
  update_conservation_score(ch, category, quantity <= 2);
  gain_craft_exp(ch, 20 + (roll == 100 ? 100 : 50) * quality, skill, true);
}

/** @brief Resolve success, current tools, payout, depletion, and advancement after the full round. */
static void complete_wilderness_harvest(struct char_data *ch, void *target, void *context)
{
  struct wilderness_harvest_context *harvest = context;

  if (!harvest_recheck(ch, target, context))
    return;
  if (harvest->material != CRAFT_MAT_NONE)
    complete_material_harvest(ch, harvest);
  else
    complete_mote_harvest(ch, harvest);
  act("$n finishes harvesting.", FALSE, ch, NULL, NULL, TO_ROOM);
}

/**
 * @brief Schedule one interruptible full-round attempt, returning one only when accepted.
 * The activity owns its context after a successful start; rewards are deferred to completion.
 */
static int start_harvest(struct char_data *ch, int category, int material)
{
  struct primary_activity_definition definition = {0};
  struct wilderness_harvest_context *harvest;
  char description[64];
  const char *what;

  if (!harvest_conditions(ch))
  {
    if (ch)
      send_to_char(ch, "You cannot begin wilderness harvesting right now.\r\n");
    return 0;
  }
  if (!is_action_available(ch, atSTANDARD, FALSE) || !is_action_available(ch, atMOVE, FALSE))
  {
    send_to_char(ch, "You must recover your full round before harvesting.\r\n");
    return 0;
  }
  if (!wilderness_harvest_available(ch, category, true))
    return 0;
  if (material != CRAFT_MAT_NONE && wilderness_material_refusal(ch, material) != NULL)
    return 0;
  what = material != CRAFT_MAT_NONE ? crafting_materials[material] : resource_names[category];
  CREATE(harvest, struct wilderness_harvest_context, 1);
  harvest->category = category;
  harvest->material = material;
  harvest->x = world[IN_ROOM(ch)].coords[0];
  harvest->y = world[IN_ROOM(ch)].coords[1];
  snprintf(description, sizeof(description), "harvesting %s", what);
  definition.type = PRIMARY_ACTIVITY_HARVEST;
  definition.display_name = description;
  definition.capabilities = PRIMARY_ACTIVITY_CAP_HANDS | PRIMARY_ACTIVITY_CAP_ATTENTION |
                            PRIMARY_ACTIVITY_CAP_STANDARD | PRIMARY_ACTIVITY_CAP_MOVE;
  definition.traits = PRIMARY_ACTIVITY_TRAIT_STATIONARY | PRIMARY_ACTIVITY_TRAIT_HANDS_OCCUPIED |
                      PRIMARY_ACTIVITY_TRAIT_OBVIOUS;
  definition.progress_model = PRIMARY_ACTIVITY_PROGRESS_ATOMIC;
  definition.progress_owner = PRIMARY_ACTIVITY_PROGRESS_CHARACTER;
  definition.total_steps = 1U;
  definition.step_interval = PULSE_VIOLENCE;
  definition.combat_actions_required = ACTION_STANDARD | ACTION_MOVE;
  definition.movement_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.damage_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.target_loss_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.command_response = PRIMARY_ACTIVITY_RESPONSE_REJECT;
  definition.cannot_pause = true;
  definition.recheck = harvest_recheck;
  definition.complete = complete_wilderness_harvest;
  definition.cleanup_context = free;
  definition.context = harvest;
  if (!primary_activity_start(ch, domain_event_room_handle(IN_ROOM(ch)), &definition))
  {
    free(harvest);
    send_to_char(ch, "You are already occupied or cannot begin harvesting right now.\r\n");
    return 0;
  }
  send_to_char(ch, "You begin harvesting %s.\r\n", what);
  act("$n begins harvesting.", FALSE, ch, NULL, NULL, TO_ROOM);
  USE_FULL_ROUND_ACTION(ch);
  return 1;
}

int start_wilderness_crafting_harvest(struct char_data *ch, int category)
{
  return start_harvest(ch, category, CRAFT_MAT_NONE);
}

/** @brief The rank a category's mote harvest rolls with (its ability, talent, and the Miner feat). */
int wilderness_harvest_rank(struct char_data *ch, int category)
{
  return harvest_rank(ch, category, crafting_harvest_skill(category));
}

int start_wilderness_material_harvest(struct char_data *ch, int material)
{
  int category = wilderness_material_category(material);

  if (category < 0)
    return 0;
  return start_harvest(ch, category, material);
}
