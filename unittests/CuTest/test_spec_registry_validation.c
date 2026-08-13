#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/act.h"
#include "../../src/character/backgrounds.h"
#include "../../src/character/guild_services.h"
#include "../../src/character/vampire_cloak.h"
#include "../../src/comms/mail.h"
#include "../../src/craft/craft.h"
#include "../../src/craft/crafting_new.h"
#include "../../src/magic/spellbook_scroll.h"
#include "../../src/obj/objsave.h"
#include "../../src/obj/player_shop.h"
#include "../../src/obj/treasure.h"
#include "../../src/obj/vendor.h"
#include "../../src/quest/hunts.h"
#include "../../src/quest/missions.h"
#include "../../src/quest/quest.h"
#include "../../src/spec/spec_mobile_archetypes.h"
#include "../../src/spec/spec_mobiles.h"
#include "../../src/spec/spec_registry.h"
#include "../../src/spec/spec_rol_avernus.h"
#include "../../src/spec/spec_rol_conversion.h"
#include "../../src/spec/spec_rol_darkhold.h"
#include "../../src/spec/spec_rol_lavatubes.h"
#include "../../src/spec/spec_rol_pilot.h"
#include "../../src/spec/spec_rol_tarrasque.h"
#include "../../src/spec/spec_rol_totem.h"
#include "../../src/spec/spec_rol_utility_objects.h"
#include "../../src/spec/spec_rooms.h"
#include "../../src/vessels/vessels_legacy.h"
#include "../../src/vessels/vessels_rol.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPEC_REGISTRY_SOURCE_LIMIT (1024L * 1024L)

struct spec_expected_definition
{
  const char *name;
  spec_legacy_handler handler;
  spec_owner_mask owner_mask;
  spec_event_mask event_mask;
  spec_binding_source_mask binding_source_mask;
};

static const struct spec_event_contract spec_test_command_event[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE}};

static int spec_test_legacy_handler(struct char_data *ch, void *me, int cmd, const char *argument)
{
  (void)ch;
  (void)me;
  (void)cmd;
  (void)argument;
  return 0;
}

static int spec_test_typed_handler(struct spec_event_context *context)
{
  (void)context;
  return 0;
}

static struct spec_definition spec_test_valid_definition(const char *name)
{
  struct spec_definition definition;

  definition.canonical_name = name;
  definition.display_name = name;
  definition.aliases = NULL;
  definition.alias_count = 0;
  definition.owner_mask = SPEC_OWNER_MOBILE;
  definition.events = spec_test_command_event;
  definition.event_count = 1;
  definition.binding_source_mask = SPEC_BINDING_SOURCE_WORLD;
  definition.builder_visibility = SPEC_BUILDER_VISIBLE;
  definition.category = "Test";
  definition.description = "Valid test definition.";
  definition.legacy_handler = spec_test_legacy_handler;
  definition.typed_adapter = NULL;
  definition.typed_handler = NULL;

  return definition;
}

static spec_event_mask spec_test_definition_event_mask(const struct spec_definition *definition)
{
  spec_event_mask event_mask;
  size_t event_index;

  event_mask = SPEC_EVENT_NONE;
  for (event_index = 0; event_index < definition->event_count; event_index++)
    event_mask |= definition->events[event_index].event;

  return event_mask;
}

static void spec_test_expect_invalid(CuTest *tc, const struct spec_definition *definitions,
                                     size_t count, const char *diagnostic_fragment)
{
  char error[512];
  bool valid;

  memset(error, 'X', sizeof(error));
  error[sizeof(error) - 1] = '\0';
  valid = spec_registry_validate_definitions(definitions, count, error, sizeof(error));

  CuAssertTrue(tc, !valid);
  CuAssertPtrNotNull(tc, strstr(error, diagnostic_fragment));
}

static const char *spec_test_source_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_ROOT");
  return root != NULL && *root != '\0' ? root : ".";
}

static bool spec_test_read_source(const char *relative_path, char **text)
{
  FILE *file;
  char path[PATH_MAX];
  char *buffer;
  long source_length;
  size_t bytes_read;
  bool success;

  *text = NULL;
  if (snprintf(path, sizeof(path), "%s/%s", spec_test_source_root(), relative_path) >=
      (int)sizeof(path))
    return false;

  file = fopen(path, "rb");
  if (file == NULL)
    return false;

  success = fseek(file, 0, SEEK_END) == 0;
  source_length = success ? ftell(file) : -1;
  if (source_length < 0 || source_length > SPEC_REGISTRY_SOURCE_LIMIT ||
      fseek(file, 0, SEEK_SET) != 0)
    success = false;

  buffer = NULL;
  if (success)
  {
    buffer = malloc((size_t)source_length + 1);
    success = buffer != NULL;
  }
  if (success)
  {
    bytes_read = fread(buffer, 1, (size_t)source_length, file);
    success = bytes_read == (size_t)source_length && ferror(file) == 0;
    buffer[bytes_read] = '\0';
  }
  if (fclose(file) != 0)
    success = false;

  if (!success)
  {
    free(buffer);
    return false;
  }

  *text = buffer;
  return true;
}

void Test_spec_registry_production_metadata_validates(CuTest *tc)
{
  char error[512];
  const struct spec_definition *definition;
  size_t definition_index;
  size_t alias_count;

  error[0] = '\0';
  CuAssert(tc, error, spec_registry_validate(error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
  CuAssertIntEquals(tc, 115, (int)spec_registry_count());
  CuAssertIntEquals(tc, 98, (int)spec_registry_legacy_count());
  CuAssertIntEquals(tc, 17, (int)spec_registry_typed_count());

  alias_count = 0;
  for (definition_index = 0; definition_index < spec_registry_count(); definition_index++)
  {
    definition = spec_registry_get((int)definition_index);
    CuAssertPtrNotNull(tc, definition);
    if (definition == NULL)
      return;
    CuAssertPtrNotNull(tc, definition->canonical_name);
    CuAssertPtrNotNull(tc, definition->display_name);
    CuAssertPtrNotNull(tc, definition->category);
    CuAssertPtrNotNull(tc, definition->description);
    CuAssertTrue(tc, definition->event_count > 0);
    CuAssertPtrNotNull(tc, spec_definition_callback(definition));
    CuAssertTrue(tc, (definition->legacy_handler != NULL) != (definition->typed_handler != NULL));
    if (definition->typed_handler != NULL)
      CuAssertPtrNotNull(tc, definition->typed_adapter);
    CuAssertIntEquals(tc, SPEC_BUILDER_VISIBLE, definition->builder_visibility);
    CuAssertTrue(tc, spec_definition_allows_binding(definition, SPEC_BINDING_SOURCE_WORLD));
    alias_count += definition->alias_count;
  }
  CuAssertIntEquals(tc, 1, (int)alias_count);
}

void Test_spec_registry_canonical_inventory_and_metadata(CuTest *tc)
{
  static const struct spec_expected_definition expected[] = {
      {"Bank", bank, SPEC_OWNER_MOBILE | SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_ITEM_IDENTIFY,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Bazaar", bazaar, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Bounty Missions", faction_mission, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Bulk Identify", eqstats, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Buy Armor", buyarmor, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Buy Weapons", buyweapons, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Crafting Kit", crafting_kit, SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_ITEM_IDENTIFY,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Crafting Quest", crafting_quest, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Cryogenicist", cryogenicist, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Dump", dump, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Guild Guard", guild_guard, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Guild", guild, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Hunts Master", huntsmaster, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Identify Mob", identify_mob, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Janitor", janitor, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"New Supply Orders", new_supply_orders, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"Pet Object", bought_pet, SPEC_OWNER_OBJECT,
       SPEC_EVENT_OBJECT_AUTO_PULSE | SPEC_EVENT_ITEM_IDENTIFY,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Pet Shop", pet_shops, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Player Shop", player_owned_shops, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Postmaster", postmaster, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Practice Dummy", practice_dummy, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Questmaster", questmaster, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_QUEST},
      {"Receptionist", receptionist, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Temple Healer", temple, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND, SPEC_BINDING_SOURCE_WORLD},
      {"Vampire Cloak", vampire_cloak, SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_ITEM_IDENTIFY,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Wizard Library", wizard_library, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"Greyhawk Ship", greyhawk_ship_object, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"Greyhawk Ship Commands", greyhawk_ship_commands, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT},
      {"breath_attack_fire", rol_breath_attack_fire, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"breath_attack_acid", rol_breath_attack_acid, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"breath_attack_lightning", rol_breath_attack_lightning, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"breath_weapon_fire", rol_breath_weapon_fire, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"breath_weapon_cold", rol_breath_weapon_cold, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"breath_weapon_acid", rol_breath_weapon_acid, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"breath_weapon_gas", rol_breath_weapon_gas, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_COMBAT_TURN,
       SPEC_BINDING_SOURCE_WORLD},
      {"breath_weapon_lightning", rol_breath_weapon_lightning, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"hulburg_beholder_major", rol_hulburg_beholder_major, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"hulburg_beholder_minor", rol_hulburg_beholder_minor, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"money_changer", rol_money_changer, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"plant_attacks_blindness", rol_plant_attacks_blindness, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"plant_attacks_paralysis", rol_plant_attacks_paralysis, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"cemetary_black_blade", rol_cemetery_black_blade, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"cemetary_cloakMeteors", rol_cemetery_cloak_meteors, SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_ITEM_IDENTIFY, SPEC_BINDING_SOURCE_WORLD},
      {"cemetary_disruption", rol_cemetery_disruption, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"cemetary_gleaming_blade", rol_cemetery_gleaming_blade, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"cemetary_lightsaber", rol_cemetery_lightsaber, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"cemetary_skeletal_hand", rol_cemetery_skeletal_hand, SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_ITEM_IDENTIFY, SPEC_BINDING_SOURCE_WORLD},
      {"flaming_tanthorian", rol_flaming_tanthorian, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"longsword_tanthorian", rol_longsword_tanthorian, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"murlynds_spoon", rol_murlynds_spoon, SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_ITEM_IDENTIFY, SPEC_BINDING_SOURCE_WORLD},
      {"muspel_bec_de_corbin", rol_muspel_bec_de_corbin, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"muspel_crystal_scimitar", rol_muspel_crystal_scimitar, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"muspel_dagger_whispers", rol_muspel_dagger_whispers, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"muspel_dragon_lance", rol_muspel_dragon_lance, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"muspel_duergar_battlehammer", rol_muspel_duergar_battlehammer, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"muspel_recurve_bow", rol_muspel_recurve_bow, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"muspel_spider_dagger", rol_muspel_spider_dagger, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT, SPEC_BINDING_SOURCE_WORLD},
      {"obj_drain", rol_obj_drain, SPEC_OWNER_OBJECT, SPEC_EVENT_OBJECT_AUTO_PULSE,
       SPEC_BINDING_SOURCE_WORLD},
      {"thorn_shield", rol_thorn_shield, SPEC_OWNER_OBJECT,
       SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_DEFENSE_REACTION, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Guild Room", guild, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Mage Guild Room", rol_mage_guild_room, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Thief Guild Room", rol_thief_guild_room, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Warrior Guild Room", rol_warrior_guild_room, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Cleric Guild Room", rol_cleric_guild_room, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Bard Guild Room", rol_bard_guild_room, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Waterdeep Guild Room", rol_waterdeep_guild_room, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Corpse Devourer", rol_corpse_devourer, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Poison Bite", rol_poison_bite, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Thief", rol_thief, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Bloodstone Portal", rol_bloodstone_portal, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Portal Door", rol_portal_door, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Travel Portal", rol_travel_portal, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Bloodstone Critter", rol_bloodstone_critter, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Designated Follower", rol_designated_follower, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Fixed Bodyguard", rol_fixed_bodyguard, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Floating Pool", rol_floating_pool, SPEC_OWNER_OBJECT, SPEC_EVENT_OBJECT_AUTO_PULSE,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Item Blocker", rol_item_blocker, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Magic Pool", rol_magic_pool, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Banana", rol_banana, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Auto Distributor", rol_auto_distributor, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Command Sentinel", rol_command_sentinel, SPEC_OWNER_MOBILE | SPEC_OWNER_ROOM,
       SPEC_EVENT_COMMAND, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Toll Keeper", rol_toll_keeper, SPEC_OWNER_MOBILE,
       SPEC_EVENT_COMMAND | SPEC_EVENT_MOBILE_ACTIVITY, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Shadow Giant", rol_shadow_giant, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Guild Guard", rol_guild_guard, SPEC_OWNER_MOBILE,
       SPEC_EVENT_COMMAND | SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN |
           SPEC_EVENT_MOBILE_HIT,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Major Beholder", rol_major_beholder, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_COMBAT_TURN,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Monster Combat", rol_monster_combat, SPEC_OWNER_MOBILE,
       SPEC_EVENT_COMMAND | SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN |
           SPEC_EVENT_MOBILE_DEATH | SPEC_EVENT_MOBILE_HIT | SPEC_EVENT_MOBILE_WAS_HIT,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Lich Energy Drain", rol_lich_energy_drain, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Lich Rite", rol_lich_rite, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Undead Drain", rol_undead_drain, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_COMBAT_TURN,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Trade Bandit", rol_bandit, SPEC_OWNER_MOBILE,
       SPEC_EVENT_COMMAND | SPEC_EVENT_MOBILE_ACTIVITY, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Sister Knight", rol_sister_knight, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Shaman Totem", rol_shaman_totem, SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_ITEM_IDENTIFY, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Totem Restorer", rol_totem_restorer, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Ship", rol_ship, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Ship Control", rol_ship_control, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Ship Exit", rol_ship_exit, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Ship Lookout", rol_ship_lookout, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Ship Navigator", rol_ship_navigator, SPEC_OWNER_MOBILE,
       SPEC_EVENT_COMMAND | SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Alert Caller", rol_alert_caller, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Yggdrasil Branch", rol_yggdrasil_branch, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Waterdeep Ambient", rol_waterdeep_ambient, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Waterdeep Peacekeeper", rol_waterdeep_peacekeeper, SPEC_OWNER_MOBILE,
       SPEC_EVENT_MOBILE_ACTIVITY, SPEC_BINDING_SOURCE_WORLD},
      {"RoL Darkhold Object", rol_darkhold_object, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Weapon Proc", rol_weapon_proc, SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_OBJECT_AUTO_PULSE | SPEC_EVENT_ITEM_IDENTIFY |
           SPEC_EVENT_WEAPON_HIT,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Avernus Object", rol_avernus_object, SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_OBJECT_AUTO_PULSE | SPEC_EVENT_ITEM_IDENTIFY |
           SPEC_EVENT_WEAPON_HIT,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Avernus Garden", rol_avernus_garden, SPEC_OWNER_ROOM, SPEC_EVENT_ROOM_ACTIVITY,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Source Periodic", rol_source_periodic, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Stateful Periodic", rol_state_periodic, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Lavatubes Mobile", rol_lavatubes_mobile, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Lavatubes Object", rol_lavatubes_object, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Lavatubes Room", rol_lavatubes_room, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Tarrasque Encounter", rol_tarrasque, SPEC_OWNER_MOBILE | SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN |
           SPEC_EVENT_MOBILE_DEATH | SPEC_EVENT_OBJECT_AUTO_PULSE,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Utility Object", rol_utility_object, SPEC_OWNER_OBJECT,
       SPEC_EVENT_COMMAND | SPEC_EVENT_OBJECT_AUTO_PULSE | SPEC_EVENT_ITEM_IDENTIFY |
           SPEC_EVENT_DEFENSE_REACTION | SPEC_EVENT_COMBAT_MANEUVER,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Utility Room", rol_utility_room, SPEC_OWNER_ROOM, SPEC_EVENT_COMMAND,
       SPEC_BINDING_SOURCE_WORLD},
      {"RoL Scheduled Mobile", rol_scheduled_mobile, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY,
       SPEC_BINDING_SOURCE_WORLD},
  };
  const struct spec_definition *definition;
  size_t definition_index;

  CuAssertIntEquals(tc, (int)(sizeof(expected) / sizeof(expected[0])), (int)spec_registry_count());
  for (definition_index = 0; definition_index < spec_registry_count(); definition_index++)
  {
    definition = spec_registry_get((int)definition_index);
    CuAssertPtrNotNull(tc, definition);
    if (definition == NULL)
      return;
    CuAssertStrEquals(tc, expected[definition_index].name, definition->canonical_name);
    CuAssertStrEquals(tc, expected[definition_index].name, definition->display_name);
    CuAssertTrue(tc, expected[definition_index].handler == spec_definition_callback(definition));
    CuAssertIntEquals(tc, (int)expected[definition_index].owner_mask, (int)definition->owner_mask);
    CuAssertIntEquals(tc, (int)expected[definition_index].event_mask,
                      (int)spec_test_definition_event_mask(definition));
    CuAssertIntEquals(tc, (int)expected[definition_index].binding_source_mask,
                      (int)definition->binding_source_mask);
  }
}

void Test_rol_ship_definition_contract(CuTest *tc)
{
  CuAssertIntEquals(tc, 7, rol_ship_definition_count());
  CuAssertTrue(tc, rol_ship_interior_contains(0, 2005998));
  CuAssertTrue(tc, rol_ship_interior_contains(0, 2005999));
  CuAssertTrue(tc, !rol_ship_interior_contains(0, 2006000));
  CuAssertTrue(tc, rol_ship_interior_contains(1, 2011136));
  CuAssertTrue(tc, !rol_ship_interior_contains(-1, 2005998));
  CuAssertIntEquals(tc, 30, rol_ship_move_delay_for_speed(0));
  CuAssertIntEquals(tc, 15, rol_ship_move_delay_for_speed(15));
  CuAssertIntEquals(tc, 0, rol_ship_move_delay_for_speed(30));
  CuAssertIntEquals(tc, 0, rol_ship_move_delay_for_speed(50));
  CuAssertTrue(tc, rol_ship_can_enter_sector(SECT_OCEAN, false));
  CuAssertTrue(tc, rol_ship_can_enter_sector(SECT_RIVER, false));
  CuAssertTrue(tc, rol_ship_can_enter_sector(SECT_FIELD, true));
  CuAssertTrue(tc, !rol_ship_can_enter_sector(SECT_FIELD, false));
}

void Test_spec_registry_alias_owner_and_reverse_lookup(CuTest *tc)
{
  const struct spec_definition *guild_definition;
  const struct spec_definition *alias_definition;
  const struct spec_definition *bank_definition;
  const struct spec_definition *quest_definition;

  guild_definition = spec_registry_find_by_name("gUiLd");
  alias_definition = spec_registry_find_by_name("GuIlDmAsTeR");
  CuAssertPtrNotNull(tc, guild_definition);
  if (guild_definition == NULL)
    return;
  CuAssertTrue(tc, guild_definition == alias_definition);
  CuAssertStrEquals(tc, "Guild", guild_definition->canonical_name);
  CuAssertIntEquals(tc, 1, (int)guild_definition->alias_count);
  CuAssertStrEquals(tc, "Guildmaster", guild_definition->aliases[0]);
  CuAssertTrue(tc, spec_registry_find_by_handler(guild) == guild_definition);
  CuAssertStrEquals(tc, "Guild", get_spec_func_name(guild));

  bank_definition = spec_registry_find_for_owner("bank", SPEC_OWNER_MOBILE);
  CuAssertPtrNotNull(tc, bank_definition);
  CuAssertTrue(tc, spec_registry_find_for_owner("BANK", SPEC_OWNER_OBJECT) == bank_definition);
  CuAssertTrue(tc, spec_registry_find_for_owner("Bank", SPEC_OWNER_ROOM) == NULL);
  CuAssertTrue(tc, spec_registry_find_for_owner("Crafting Quest", SPEC_OWNER_ROOM) != NULL);
  CuAssertTrue(tc, spec_registry_find_for_owner("Crafting Quest", SPEC_OWNER_MOBILE) == NULL);
  CuAssertTrue(tc, spec_registry_find_for_owner("Pet Shop", SPEC_OWNER_ROOM) != NULL);
  CuAssertTrue(tc, spec_registry_find_for_owner("Pet Shop", SPEC_OWNER_MOBILE) == NULL);

  quest_definition = spec_registry_find_by_name("Questmaster");
  CuAssertTrue(tc, spec_definition_allows_binding(quest_definition, SPEC_BINDING_SOURCE_QUEST));
  CuAssertTrue(
      tc, !spec_definition_allows_binding(quest_definition, SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT));
  CuAssertTrue(tc, spec_registry_find_by_name(NULL) == NULL);
  CuAssertTrue(tc, spec_registry_find_by_name("") == NULL);
  CuAssertTrue(tc, spec_registry_find_by_name("   ") == NULL);
  CuAssertTrue(tc, spec_registry_find_by_name("Unknown Procedure") == NULL);
}

void Test_spec_registry_event_contracts_and_prerequisites(CuTest *tc)
{
  const struct spec_definition *definition;
  const struct spec_event_contract *event;

  definition = spec_registry_find_by_name("Bank");
  CuAssertTrue(tc,
               spec_definition_supports_event(definition, SPEC_OWNER_MOBILE, SPEC_EVENT_COMMAND));
  CuAssertTrue(tc,
               spec_definition_supports_event(definition, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND));
  CuAssertTrue(
      tc, spec_definition_supports_event(definition, SPEC_OWNER_OBJECT, SPEC_EVENT_ITEM_IDENTIFY));
  CuAssertTrue(
      tc, !spec_definition_supports_event(definition, SPEC_OWNER_MOBILE, SPEC_EVENT_ITEM_IDENTIFY));

  definition = spec_registry_find_by_name("Janitor");
  event = spec_definition_get_event(definition, SPEC_EVENT_MOBILE_ACTIVITY);
  CuAssertPtrNotNull(tc, event);
  if (event == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_PROTOTYPE_MOB_SPEC, (int)event->required_prototype_flags);

  definition = spec_registry_find_by_name("Pet Object");
  event = spec_definition_get_event(definition, SPEC_EVENT_OBJECT_AUTO_PULSE);
  CuAssertPtrNotNull(tc, event);
  if (event == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_PROTOTYPE_ITEM_AUTOPROC, (int)event->required_prototype_flags);
  CuAssertIntEquals(tc, SPEC_PLACEMENT_CARRIED, (int)event->required_placement);

  definition = spec_registry_find_by_name("RoL Floating Pool");
  event = spec_definition_get_event(definition, SPEC_EVENT_OBJECT_AUTO_PULSE);
  CuAssertPtrNotNull(tc, event);
  if (event == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_PROTOTYPE_ITEM_AUTOPROC, (int)event->required_prototype_flags);
  CuAssertIntEquals(tc, SPEC_PLACEMENT_NONE, (int)event->required_placement);

  definition = spec_registry_find_by_name("RoL Bloodstone Portal");
  event = spec_definition_get_event(definition, SPEC_EVENT_COMMAND);
  CuAssertPtrNotNull(tc, event);
  if (event == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_PROTOTYPE_NONE, (int)event->required_prototype_flags);
  CuAssertIntEquals(tc, SPEC_PLACEMENT_NONE, (int)event->required_placement);

  definition = spec_registry_find_by_name("Practice Dummy");
  event = spec_definition_get_event(definition, SPEC_EVENT_MOBILE_COMBAT_TURN);
  CuAssertPtrNotNull(tc, event);
  if (event == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_PROTOTYPE_MOB_SPEC, (int)event->required_prototype_flags);
  CuAssertIntEquals(tc, SPEC_PLACEMENT_COMBAT, (int)event->required_placement);

  definition = spec_registry_find_by_name("RoL Monster Combat");
  event = spec_definition_get_event(definition, SPEC_EVENT_MOBILE_WAS_HIT);
  CuAssertPtrNotNull(tc, event);
  if (event == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_PROTOTYPE_MOB_SPEC, (int)event->required_prototype_flags);
  CuAssertIntEquals(tc, SPEC_PLACEMENT_COMBAT, (int)event->required_placement);

  definition = spec_registry_find_by_name("RoL Tarrasque Encounter");
  event = spec_definition_get_event(definition, SPEC_EVENT_MOBILE_DEATH);
  CuAssertPtrNotNull(tc, event);
  if (event == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_PROTOTYPE_MOB_SPEC, (int)event->required_prototype_flags);
  CuAssertIntEquals(tc, SPEC_PLACEMENT_NONE, (int)event->required_placement);

  definition = spec_registry_find_by_name("RoL Monster Combat");
  event = spec_definition_get_event(definition, SPEC_EVENT_MOBILE_HIT);
  CuAssertPtrNotNull(tc, event);
  if (event == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_PROTOTYPE_MOB_SPEC, (int)event->required_prototype_flags);
  CuAssertIntEquals(tc, SPEC_PLACEMENT_COMBAT, (int)event->required_placement);

  definition = spec_registry_find_by_name("Vampire Cloak");
  event = spec_definition_get_event(definition, SPEC_EVENT_COMMAND);
  CuAssertPtrNotNull(tc, event);
  if (event == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_PLACEMENT_EQUIPPED, (int)event->required_placement);
  CuAssertTrue(tc, spec_definition_get_event(definition, SPEC_EVENT_WEAPON_HIT) == NULL);
}

void Test_spec_registry_event_and_owner_names_cover_contract(CuTest *tc)
{
  static const spec_event_mask events[] = {
      SPEC_EVENT_COMMAND,
      SPEC_EVENT_MOBILE_ACTIVITY,
      SPEC_EVENT_MOBILE_COMBAT_TURN,
      SPEC_EVENT_OBJECT_AUTO_PULSE,
      SPEC_EVENT_ITEM_IDENTIFY,
      SPEC_EVENT_WEAPON_HIT,
      SPEC_EVENT_DEFENSE_REACTION,
      SPEC_EVENT_COMBAT_MANEUVER,
      SPEC_EVENT_MOUNT_CHARGE,
      SPEC_EVENT_MOVING_ROOM_RELOCATION,
      SPEC_EVENT_MOBILE_DEATH,
      SPEC_EVENT_MOBILE_HIT,
      SPEC_EVENT_MOBILE_WAS_HIT,
      SPEC_EVENT_ROOM_ACTIVITY,
  };
  size_t event_index;

  CuAssertStrEquals(tc, "mobile", spec_owner_name(SPEC_OWNER_MOBILE));
  CuAssertStrEquals(tc, "object", spec_owner_name(SPEC_OWNER_OBJECT));
  CuAssertStrEquals(tc, "room", spec_owner_name(SPEC_OWNER_ROOM));
  for (event_index = 0; event_index < sizeof(events) / sizeof(events[0]); event_index++)
    CuAssertPtrNotNull(tc, spec_event_name(events[event_index]));
}

void Test_spec_registry_accessors_reject_extreme_inputs(CuTest *tc)
{
  const struct spec_definition *definition;

  definition = spec_registry_find_by_name("Bank");
  CuAssertTrue(tc, spec_registry_get(INT_MIN) == NULL);
  CuAssertTrue(tc, spec_registry_get(-1) == NULL);
  CuAssertTrue(tc, spec_registry_get((int)spec_registry_count()) == NULL);
  CuAssertTrue(tc, spec_registry_get(INT_MAX) == NULL);
  CuAssertTrue(tc, get_spec_func_name_by_index(INT_MIN) == NULL);
  CuAssertTrue(tc, get_spec_func_by_index(INT_MIN) == NULL);
  CuAssertTrue(tc, get_spec_func_name_by_index(INT_MAX) == NULL);
  CuAssertTrue(tc, get_spec_func_by_index(INT_MAX) == NULL);
  CuAssertTrue(tc, spec_registry_find_by_handler(NULL) == NULL);
  CuAssertTrue(tc, spec_registry_find_for_owner("Bank", SPEC_OWNER_NONE) == NULL);
  CuAssertTrue(tc, spec_registry_find_for_owner("Bank", SPEC_OWNER_ALL) == NULL);
  CuAssertTrue(tc, !spec_definition_supports_event(definition, SPEC_OWNER_OBJECT,
                                                   SPEC_EVENT_COMMAND | SPEC_EVENT_ITEM_IDENTIFY));
  CuAssertTrue(tc, spec_definition_get_event(definition, UINT32_MAX) == NULL);
  CuAssertTrue(tc,
               !spec_definition_allows_binding(
                   definition, SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT));
  CuAssertTrue(tc, spec_owner_name(SPEC_OWNER_ALL) == NULL);
  CuAssertTrue(tc, spec_event_name(SPEC_EVENT_ALL) == NULL);
}

void Test_spec_registry_rejects_missing_table_and_text(CuTest *tc)
{
  struct spec_definition definition;

  spec_test_expect_invalid(tc, NULL, 1, "table is null");
  spec_test_expect_invalid(tc, &definition, 0, "table is empty");

  definition = spec_test_valid_definition("Valid");
  definition.canonical_name = "  ";
  spec_test_expect_invalid(tc, &definition, 1, "canonical_name");
  definition = spec_test_valid_definition("Valid");
  definition.display_name = NULL;
  spec_test_expect_invalid(tc, &definition, 1, "display_name");
  definition = spec_test_valid_definition("Valid");
  definition.category = "\t";
  spec_test_expect_invalid(tc, &definition, 1, "category");
  definition = spec_test_valid_definition("Valid");
  definition.description = "";
  spec_test_expect_invalid(tc, &definition, 1, "description");
  definition = spec_test_valid_definition("Valid");
  definition.alias_count = 1;
  spec_test_expect_invalid(tc, &definition, 1, "aliases is null");
  definition = spec_test_valid_definition("Valid");
  definition.event_count = 0;
  spec_test_expect_invalid(tc, &definition, 1, "event_count");
  definition = spec_test_valid_definition("Valid");
  definition.events = NULL;
  spec_test_expect_invalid(tc, &definition, 1, "events is null");
}

void Test_spec_registry_rejects_case_insensitive_name_collisions(CuTest *tc)
{
  struct spec_definition definitions[2];
  const char *first_aliases[1];
  const char *second_aliases[1];

  definitions[0] = spec_test_valid_definition("Alpha");
  definitions[1] = spec_test_valid_definition("aLpHa");
  spec_test_expect_invalid(tc, definitions, 2, "collides");

  first_aliases[0] = "ALPHA";
  definitions[0] = spec_test_valid_definition("Alpha");
  definitions[0].aliases = first_aliases;
  definitions[0].alias_count = 1;
  spec_test_expect_invalid(tc, definitions, 1, "collides");

  first_aliases[0] = "Beta";
  definitions[0] = spec_test_valid_definition("Alpha");
  definitions[0].aliases = first_aliases;
  definitions[0].alias_count = 1;
  definitions[1] = spec_test_valid_definition("bEtA");
  spec_test_expect_invalid(tc, definitions, 2, "collides");

  first_aliases[0] = "Shared";
  second_aliases[0] = "sHaReD";
  definitions[0] = spec_test_valid_definition("Alpha");
  definitions[0].aliases = first_aliases;
  definitions[0].alias_count = 1;
  definitions[1] = spec_test_valid_definition("Beta");
  definitions[1].aliases = second_aliases;
  definitions[1].alias_count = 1;
  spec_test_expect_invalid(tc, definitions, 2, "collides");

  first_aliases[0] = " ";
  definitions[0] = spec_test_valid_definition("Alpha");
  definitions[0].aliases = first_aliases;
  definitions[0].alias_count = 1;
  spec_test_expect_invalid(tc, definitions, 1, "alias[0] is empty");
}

void Test_spec_registry_allows_shared_handlers_for_distinct_identities(CuTest *tc)
{
  struct spec_definition definitions[2];
  char error[512];

  definitions[0] = spec_test_valid_definition("First Identity");
  definitions[1] = spec_test_valid_definition("Second Identity");
  error[0] = '\0';

  CuAssert(tc, error, spec_registry_validate_definitions(definitions, 2, error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
  CuAssertTrue(tc, definitions[0].legacy_handler == definitions[1].legacy_handler);
  CuAssertTrue(tc, strcmp(definitions[0].canonical_name, definitions[1].canonical_name) != 0);
}

void Test_spec_registry_rejects_invalid_masks_and_visibility(CuTest *tc)
{
  struct spec_definition definition;
  char error[512];

  definition = spec_test_valid_definition("Valid");
  definition.owner_mask = SPEC_OWNER_NONE;
  spec_test_expect_invalid(tc, &definition, 1, "owner_mask");
  definition = spec_test_valid_definition("Valid");
  definition.owner_mask = (1U << 30);
  spec_test_expect_invalid(tc, &definition, 1, "owner_mask");
  definition = spec_test_valid_definition("Valid");
  definition.binding_source_mask = SPEC_BINDING_SOURCE_NONE;
  spec_test_expect_invalid(tc, &definition, 1, "binding_source_mask");
  definition = spec_test_valid_definition("Valid");
  definition.binding_source_mask = (1U << 30);
  spec_test_expect_invalid(tc, &definition, 1, "binding_source_mask");
  definition = spec_test_valid_definition("Valid");
  definition.builder_visibility = SPEC_BUILDER_VISIBILITY_INVALID;
  spec_test_expect_invalid(tc, &definition, 1, "builder_visibility");

  definition = spec_test_valid_definition("Valid");
  definition.owner_mask = SPEC_OWNER_MOBILE | SPEC_OWNER_OBJECT;
  definition.builder_visibility = SPEC_BUILDER_HIDDEN;
  error[0] = '\0';
  CuAssert(tc, error, spec_registry_validate_definitions(&definition, 1, error, sizeof(error)));
}

void Test_spec_registry_rejects_invalid_event_contracts(CuTest *tc)
{
  struct spec_definition definition;
  struct spec_event_contract event;
  struct spec_event_contract duplicate_events[2];

  definition = spec_test_valid_definition("Valid");
  event = spec_test_command_event[0];
  definition.events = &event;

  event.event = SPEC_EVENT_NONE;
  spec_test_expect_invalid(tc, &definition, 1, "event[0] mask");
  event = spec_test_command_event[0];
  event.event = SPEC_EVENT_COMMAND | SPEC_EVENT_MOBILE_ACTIVITY;
  spec_test_expect_invalid(tc, &definition, 1, "event[0] mask");
  event = spec_test_command_event[0];
  event.event = (1U << 30);
  spec_test_expect_invalid(tc, &definition, 1, "event[0] mask");

  event.event = SPEC_EVENT_MOBILE_ACTIVITY;
  event.required_prototype_flags = SPEC_PROTOTYPE_MOB_SPEC;
  definition.owner_mask = SPEC_OWNER_OBJECT;
  spec_test_expect_invalid(tc, &definition, 1, "incompatible with owner_mask");

  definition.owner_mask = SPEC_OWNER_MOBILE;
  event.required_prototype_flags = SPEC_PROTOTYPE_NONE;
  spec_test_expect_invalid(tc, &definition, 1, "missing prototype flags");
  event.required_prototype_flags = (1U << 30);
  spec_test_expect_invalid(tc, &definition, 1, "prototype flags");

  event = spec_test_command_event[0];
  event.required_placement = (1U << 30);
  spec_test_expect_invalid(tc, &definition, 1, "placement mask");

  event.event = SPEC_EVENT_MOBILE_COMBAT_TURN;
  event.required_prototype_flags = SPEC_PROTOTYPE_MOB_SPEC;
  event.required_placement = SPEC_PLACEMENT_NONE;
  spec_test_expect_invalid(tc, &definition, 1, "missing placement flags");

  duplicate_events[0] = spec_test_command_event[0];
  duplicate_events[1] = spec_test_command_event[0];
  definition = spec_test_valid_definition("Valid");
  definition.events = duplicate_events;
  definition.event_count = 2;
  spec_test_expect_invalid(tc, &definition, 1, "duplicates command");
}

void Test_spec_registry_enforces_handler_exclusivity_and_bounded_diagnostics(CuTest *tc)
{
  struct spec_definition definition;
  struct spec_definition definitions[2];
  char error[8];

  definition = spec_test_valid_definition("Valid");
  definition.legacy_handler = NULL;
  spec_test_expect_invalid(tc, &definition, 1, "typed behavior requires");

  definition = spec_test_valid_definition("Valid");
  definition.typed_handler = spec_test_typed_handler;
  spec_test_expect_invalid(tc, &definition, 1, "legacy behavior cannot");

  definition.legacy_handler = NULL;
  spec_test_expect_invalid(tc, &definition, 1, "typed behavior requires");

  definition.typed_adapter = spec_test_legacy_handler;
  CuAssertTrue(tc, spec_registry_validate_definitions(&definition, 1, NULL, 0));

  definitions[0] = spec_test_valid_definition("First");
  definitions[1] = spec_test_valid_definition("Second");
  definitions[1].legacy_handler = NULL;
  definitions[1].typed_adapter = spec_test_legacy_handler;
  definitions[1].typed_handler = spec_test_typed_handler;
  spec_test_expect_invalid(tc, definitions, 2, "typed adapter collides");

  definition = spec_test_valid_definition("Valid");
  definition.description = NULL;
  memset(error, 'X', sizeof(error));
  CuAssertTrue(tc, !spec_registry_validate_definitions(&definition, 1, error, sizeof(error)));
  CuAssertIntEquals(tc, '\0', error[sizeof(error) - 1]);
  CuAssertTrue(tc, !spec_registry_validate_definitions(&definition, 1, NULL, 0));
}

void Test_spec_registry_boot_validation_precedes_world_parsing(CuTest *tc)
{
  char *source;
  char *boot_db_start;
  char *validation_call;
  char *boot_world_call;
  char *help_load;
  bool source_loaded;
  bool order_valid;

  source = NULL;
  source_loaded = spec_test_read_source("src/db.c", &source);
  order_valid = false;
  if (source_loaded)
  {
    boot_db_start = strstr(source, "void boot_db(void)");
    validation_call =
        boot_db_start != NULL ? strstr(boot_db_start, "spec_registry_boot_validate();") : NULL;
    boot_world_call = boot_db_start != NULL ? strstr(boot_db_start, "boot_world();") : NULL;
    help_load =
        boot_db_start != NULL ? strstr(boot_db_start, "log(\"Loading help entries.\")") : NULL;
    order_valid = validation_call != NULL && boot_world_call != NULL && help_load != NULL &&
                  validation_call < boot_world_call && boot_world_call < help_load;
  }
  free(source);

  CuAssertTrue(tc, source_loaded);
  CuAssertTrue(tc, order_valid);
}
