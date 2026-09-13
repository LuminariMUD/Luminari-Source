#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "harvest.h"
#include "resource_system.h"
#include "resource_depletion.h"
#include "wilderness.h"
#include "activity_manager.h"
#include "actions.h"
#include "comm.h"
#include "constants.h"
#include "craft/crafting_new.h"
#include "craft/craft.h"
#include "db.h"
#include "domain_event_world.h"
#include "dotenv.h"
#include "harvest_vnums.h"
#include "magic/spells.h"

#include <limits.h>

struct wilderness_harvest_context
{
  int category;
  int x;
  int y;
};

bool wilderness_harvest_crafting_enabled(void)
{
  return get_env_bool("WILDERNESS_HARVEST_CRAFTING", TRUE);
}

static bool harvest_location_valid(struct char_data *ch)
{
  return world && zone_table && ch && !IS_NPC(ch) && ch->player_specials &&
         IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) <= top_of_world &&
         world[IN_ROOM(ch)].zone <= top_of_zone_table &&
         ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS);
}

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
      calculate_current_resource_level(category, x, y) < 0.1f)
  {
    if (verbose)
      send_to_char(ch, "There are insufficient %s resources here to harvest.\r\n",
                   resource_names[category]);
    return false;
  }
  return true;
}

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

/* Each column corresponds to Poor through Legendary. These are actual crafting
 * materials, whose grades are checked independently by the regression suite. */
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

/* Resources without graded crafting materials supply crafting motes instead.
 * Quality is preserved in their yield: one through five motes per raw unit. */
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
    if (GET_CRAFT_MAT(ch, material) < 0 || GET_CRAFT_MAT(ch, material) > INT_MAX - quantity)
      return 0;
    GET_CRAFT_MAT(ch, material) += quantity;
    send_to_char(ch, "You harvest %d units of %s (%s quality).\r\n", quantity,
                 crafting_materials[material], get_material_quality_name(quality));
    return quantity;
  }
  mote = wilderness_harvest_mote(category, subtype);
  if (!mote || quantity > INT_MAX / quality)
    return 0;
  amount = quantity * quality;
  if (GET_CRAFT_MOTES(ch, mote) < 0 || GET_CRAFT_MOTES(ch, mote) > INT_MAX - amount)
    return 0;
  GET_CRAFT_MOTES(ch, mote) += amount;
  send_to_char(ch, "You harvest %d units of %s, yielding %d %ss.\r\n", quantity,
               get_full_material_name(category, subtype, quality), amount, crafting_motes[mote]);
  return quantity;
}

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

static bool harvest_conditions(struct char_data *ch)
{
  return harvest_location_valid(ch) && !FIGHTING(ch) && GET_POS(ch) >= POS_STANDING &&
         !AFF_FLAGGED(ch, AFF_GRAPPLED) && !AFF_FLAGGED(ch, AFF_ENTANGLED) &&
         GET_CRAFTING_TICKS(ch) == 0 && GET_CRAFT(ch).craft_duration == 0;
}

static bool harvest_recheck(struct char_data *ch, void *target, void *context)
{
  struct wilderness_harvest_context *harvest = context;

  return harvest_conditions(ch) && wilderness_harvest_crafting_enabled() &&
         target == &world[IN_ROOM(ch)] && world[IN_ROOM(ch)].coords[0] == harvest->x &&
         world[IN_ROOM(ch)].coords[1] == harvest->y &&
         wilderness_harvest_available(ch, harvest->category, true);
}

static void complete_wilderness_harvest(struct char_data *ch, void *target, void *context)
{
  struct wilderness_harvest_context *harvest = context;
  int category = harvest->category;
  int skill, rank, roll, success, quality, subtype, quantity, mote, motes;
  float level;

  if (!harvest_recheck(ch, target, context))
    return;
  skill = crafting_harvest_skill(category);
  rank = get_craft_skill_value(ch, skill) + get_proficient_talent_bonus(ch, skill);
  if ((category == RESOURCE_MINERALS || category == RESOURCE_STONE ||
       category == RESOURCE_CRYSTAL) &&
      HAS_FEAT(ch, FEAT_MINER))
    rank += 4;
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

  /* Node-style bonus motes accompany material rewards; mote harvests already
   * received their quality-scaled payout above. */
  if (wilderness_harvest_material(category, subtype, quality) != CRAFT_MAT_NONE &&
      (roll == 100 || dice(1, 100) <= 20))
  {
    mote = dice(1, NUM_CRAFT_MOTES - 1);
    motes = dice(quality, 4) * 150 / 100;
    motes = MIN(motes, INT_MAX - MAX(0, GET_CRAFT_MOTES(ch, mote)));
    if (motes > 0 && GET_CRAFT_MOTES(ch, mote) >= 0)
    {
      GET_CRAFT_MOTES(ch, mote) += motes;
      send_to_char(ch, "You also extract %d %ss.\r\n", motes, crafting_motes[mote]);
    }
  }
  act("$n finishes harvesting.", FALSE, ch, NULL, NULL, TO_ROOM);
}

int start_wilderness_crafting_harvest(struct char_data *ch, int category)
{
  struct primary_activity_definition definition = {0};
  struct wilderness_harvest_context *harvest;
  char description[64];

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
  CREATE(harvest, struct wilderness_harvest_context, 1);
  harvest->category = category;
  harvest->x = world[IN_ROOM(ch)].coords[0];
  harvest->y = world[IN_ROOM(ch)].coords[1];
  snprintf(description, sizeof(description), "harvesting %s", resource_names[category]);
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
  send_to_char(ch, "You begin harvesting %s.\r\n", resource_names[category]);
  act("$n begins harvesting.", FALSE, ch, NULL, NULL, TO_ROOM);
  USE_FULL_ROUND_ACTION(ch);
  return 1;
}
