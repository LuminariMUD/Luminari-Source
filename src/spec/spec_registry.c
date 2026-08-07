/**
 * @file spec_registry.c
 * Immutable special-procedure definitions, validation, and compatibility accessors.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

#include "spec/spec_registry.h"
#include "spec_procs.h"
#include "character/guild_services.h"
#include "magic/spellbook_scroll.h"
#include "obj/vendor.h"
#include "spec/spec_mobiles.h"
#include "spec/spec_rooms.h"

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>

SPECIAL_DECL(bazaar);
SPECIAL_DECL(faction_mission);
SPECIAL_DECL(eqstats);
SPECIAL_DECL(huntsmaster);
SPECIAL_DECL(new_supply_orders);
SPECIAL_DECL(temple);
SPECIAL_DECL(vampire_cloak);

#define SPEC_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define SPEC_BIND_WORLD_LEGACY (SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT)

static const struct spec_event_contract command_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE}};

static const struct spec_event_contract bank_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_ITEM_IDENTIFY, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE}};

static const struct spec_event_contract crafting_kit_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_CARRIED},
    {SPEC_EVENT_ITEM_IDENTIFY, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE}};

static const struct spec_event_contract janitor_events[] = {
    {SPEC_EVENT_MOBILE_ACTIVITY, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_NONE}};

static const struct spec_event_contract pet_object_events[] = {
    {SPEC_EVENT_OBJECT_AUTO_PULSE, SPEC_PROTOTYPE_ITEM_AUTOPROC, SPEC_PLACEMENT_CARRIED},
    {SPEC_EVENT_ITEM_IDENTIFY, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE}};

static const struct spec_event_contract practice_dummy_events[] = {
    {SPEC_EVENT_MOBILE_ACTIVITY, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_COMBAT}};

static const struct spec_event_contract vampire_cloak_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_EQUIPPED},
    {SPEC_EVENT_ITEM_IDENTIFY, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE}};

static const char *const guild_aliases[] = {"Guildmaster"};

static const struct spec_definition spec_definitions[] = {
    {
        .canonical_name = "Bank",
        .display_name = "Bank",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE | SPEC_OWNER_OBJECT,
        .events = bank_events,
        .event_count = SPEC_ARRAY_SIZE(bank_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Services",
        .description = "Provides balance, deposit, and withdrawal services; bank objects also "
                       "describe themselves when identified.",
        .legacy_handler = bank,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Bazaar",
        .display_name = "Bazaar",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Commerce",
        .description = "Offers generated armor, weapons, and miscellaneous equipment through the "
                       "bazaar command.",
        .legacy_handler = bazaar,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Bounty Missions",
        .display_name = "Bounty Missions",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Quests",
        .description = "Lets players request, accept, decline, and manage faction bounty missions.",
        .legacy_handler = faction_mission,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Bulk Identify",
        .display_name = "Bulk Identify",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Services",
        .description = "Charges a level-based fee to summarize enchantments on worn equipment.",
        .legacy_handler = eqstats,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Buy Armor",
        .display_name = "Buy Armor",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Commerce",
        .description = "Sells generated mundane, masterwork, and enchanted armor and shields.",
        .legacy_handler = buyarmor,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Buy Weapons",
        .display_name = "Buy Weapons",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Commerce",
        .description = "Sells generated mundane, masterwork, and enchanted weapons.",
        .legacy_handler = buyweapons,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Crafting Kit",
        .display_name = "Crafting Kit",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = crafting_kit_events,
        .event_count = SPEC_ARRAY_SIZE(crafting_kit_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Crafting",
        .description = "Handles carried crafting-kit commands and explains kit use during "
                       "identification.",
        .legacy_handler = crafting_kit,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Crafting Quest",
        .display_name = "Crafting Quest",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Crafting",
        .description = "Provides the legacy supply-order crafting quest service in designated "
                       "rooms.",
        .legacy_handler = crafting_quest,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Cryogenicist",
        .display_name = "Cryogenicist",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Player Services",
        .description = "Provides cryogenic rent, storage, and character logout services.",
        .legacy_handler = cryogenicist,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Dump",
        .display_name = "Dump",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "World",
        .description = "Destroys objects dropped in the room and may reward low-level players.",
        .legacy_handler = dump,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Guild Guard",
        .display_name = "Guild Guard",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Training",
        .description = "Blocks configured guild entrances when a character lacks the required "
                       "class.",
        .legacy_handler = guild_guard,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Guild",
        .display_name = "Guild",
        .aliases = guild_aliases,
        .alias_count = SPEC_ARRAY_SIZE(guild_aliases),
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Training",
        .description = "Provides class training, practice, and boost information through a guild "
                       "trainer.",
        .legacy_handler = guild,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Hunts Master",
        .display_name = "Hunts Master",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Quests",
        .description = "Lists active hunts and exchanges hunt trophies for rewards.",
        .legacy_handler = huntsmaster,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Identify Mob",
        .display_name = "Identify Mob",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Services",
        .description = "Identifies carried or worn equipment for a fee.",
        .legacy_handler = identify_mob,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Janitor",
        .display_name = "Janitor",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "World",
        .description = "Picks up low-value trash during mobile activity pulses.",
        .legacy_handler = janitor,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "New Supply Orders",
        .display_name = "New Supply Orders",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Crafting",
        .description = "Provides the current request, progress, material, completion, and reset "
                       "workflow for supply orders.",
        .legacy_handler = new_supply_orders,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Pet Object",
        .display_name = "Pet Object",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = pet_object_events,
        .event_count = SPEC_ARRAY_SIZE(pet_object_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Companions",
        .description = "Converts a carried pet object into its matching mobile companion on an "
                       "automatic object pulse.",
        .legacy_handler = bought_pet,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Pet Shop",
        .display_name = "Pet Shop",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Commerce",
        .description = "Lists and sells companion mobiles housed in the following room.",
        .legacy_handler = pet_shops,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Player Shop",
        .display_name = "Player Shop",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Commerce",
        .description = "Lists and sells inventory from a configured player-owned shop.",
        .legacy_handler = player_owned_shops,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Postmaster",
        .display_name = "Postmaster",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Player Services",
        .description = "Sends, checks, and receives player mail.",
        .legacy_handler = postmaster,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Practice Dummy",
        .display_name = "Practice Dummy",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = practice_dummy_events,
        .event_count = SPEC_ARRAY_SIZE(practice_dummy_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Combat",
        .description = "Restores its health and reports accumulated damage during activity and "
                       "combat turns.",
        .legacy_handler = practice_dummy,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Questmaster",
        .display_name = "Questmaster",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_QUEST,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Quests",
        .description = "Provides quest list and join commands while preserving a saved secondary "
                       "mobile procedure.",
        .legacy_handler = questmaster,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Receptionist",
        .display_name = "Receptionist",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Player Services",
        .description = "Provides rent, storage, and character logout services.",
        .legacy_handler = receptionist,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Temple Healer",
        .display_name = "Temple Healer",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Services",
        .description = "Sells healing, restoration, and condition-removal blessings.",
        .legacy_handler = temple,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Vampire Cloak",
        .display_name = "Vampire Cloak",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = vampire_cloak_events,
        .event_count = SPEC_ARRAY_SIZE(vampire_cloak_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Equipment",
        .description = "Lets a vampire customize an equipped cloak and explains the feature during "
                       "identification.",
        .legacy_handler = vampire_cloak,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Wizard Library",
        .display_name = "Wizard Library",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Magic",
        .description = "Lets eligible wizards research spells into a prepared spellbook.",
        .legacy_handler = wizard_library,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Greyhawk Ship",
        .display_name = "Greyhawk Ship",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Vessels",
        .description = "Validates a Greyhawk vessel entrance, collects fare, and boards the linked "
                       "interior room.",
        .legacy_handler = greyhawk_ship_object,
        .typed_handler = NULL,
    },
    {
        .canonical_name = "Greyhawk Ship Commands",
        .display_name = "Greyhawk Ship Commands",
        .aliases = NULL,
        .alias_count = 0,
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BIND_WORLD_LEGACY,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "Vessels",
        .description = "Recognizes bridge-only Greyhawk vessel control commands in a ship control "
                       "room.",
        .legacy_handler = greyhawk_ship_commands,
        .typed_handler = NULL,
    },
};

enum
{
  SPEC_DEFINITION_BANK = 0,
  SPEC_DEFINITION_BAZAAR,
  SPEC_DEFINITION_BOUNTY_MISSIONS,
  SPEC_DEFINITION_BULK_IDENTIFY,
  SPEC_DEFINITION_BUY_ARMOR,
  SPEC_DEFINITION_BUY_WEAPONS,
  SPEC_DEFINITION_CRAFTING_KIT,
  SPEC_DEFINITION_CRAFTING_QUEST,
  SPEC_DEFINITION_CRYOGENICIST,
  SPEC_DEFINITION_DUMP,
  SPEC_DEFINITION_GUILD_GUARD,
  SPEC_DEFINITION_GUILD,
  SPEC_DEFINITION_HUNTS_MASTER,
  SPEC_DEFINITION_IDENTIFY_MOB,
  SPEC_DEFINITION_JANITOR,
  SPEC_DEFINITION_NEW_SUPPLY_ORDERS,
  SPEC_DEFINITION_PET_OBJECT,
  SPEC_DEFINITION_PET_SHOP,
  SPEC_DEFINITION_PLAYER_SHOP,
  SPEC_DEFINITION_POSTMASTER,
  SPEC_DEFINITION_PRACTICE_DUMMY,
  SPEC_DEFINITION_QUESTMASTER,
  SPEC_DEFINITION_RECEPTIONIST,
  SPEC_DEFINITION_TEMPLE_HEALER,
  SPEC_DEFINITION_VAMPIRE_CLOAK,
  SPEC_DEFINITION_WIZARD_LIBRARY,
  SPEC_DEFINITION_GREYHAWK_SHIP,
  SPEC_DEFINITION_GREYHAWK_SHIP_COMMANDS,
  SPEC_DEFINITION_INDEX_COUNT
};

_Static_assert(SPEC_DEFINITION_INDEX_COUNT == SPEC_ARRAY_SIZE(spec_definitions),
               "special-procedure definition indexes must match the registry");

struct spec_compatibility_name
{
  int definition_index;
  int alias_index;
};

static const struct spec_compatibility_name compatibility_names[] = {
    {SPEC_DEFINITION_BANK, -1},
    {SPEC_DEFINITION_BAZAAR, -1},
    {SPEC_DEFINITION_BOUNTY_MISSIONS, -1},
    {SPEC_DEFINITION_BULK_IDENTIFY, -1},
    {SPEC_DEFINITION_BUY_ARMOR, -1},
    {SPEC_DEFINITION_BUY_WEAPONS, -1},
    {SPEC_DEFINITION_CRAFTING_KIT, -1},
    {SPEC_DEFINITION_CRAFTING_QUEST, -1},
    {SPEC_DEFINITION_CRYOGENICIST, -1},
    {SPEC_DEFINITION_DUMP, -1},
    {SPEC_DEFINITION_GUILD_GUARD, -1},
    {SPEC_DEFINITION_GUILD, -1},
    {SPEC_DEFINITION_GUILD, 0},
    {SPEC_DEFINITION_HUNTS_MASTER, -1},
    {SPEC_DEFINITION_IDENTIFY_MOB, -1},
    {SPEC_DEFINITION_JANITOR, -1},
    {SPEC_DEFINITION_NEW_SUPPLY_ORDERS, -1},
    {SPEC_DEFINITION_PET_OBJECT, -1},
    {SPEC_DEFINITION_PET_SHOP, -1},
    {SPEC_DEFINITION_PLAYER_SHOP, -1},
    {SPEC_DEFINITION_POSTMASTER, -1},
    {SPEC_DEFINITION_PRACTICE_DUMMY, -1},
    {SPEC_DEFINITION_QUESTMASTER, -1},
    {SPEC_DEFINITION_RECEPTIONIST, -1},
    {SPEC_DEFINITION_TEMPLE_HEALER, -1},
    {SPEC_DEFINITION_VAMPIRE_CLOAK, -1},
    {SPEC_DEFINITION_WIZARD_LIBRARY, -1},
    {SPEC_DEFINITION_GREYHAWK_SHIP, -1},
    {SPEC_DEFINITION_GREYHAWK_SHIP_COMMANDS, -1},
};

_Static_assert(SPEC_ARRAY_SIZE(compatibility_names) <= INT_MAX,
               "legacy special-procedure count must fit in int");

static bool spec_mask_is_one_bit(uint32_t mask)
{
  return mask != 0 && (mask & (mask - 1U)) == 0;
}

static bool spec_text_present(const char *text)
{
  const unsigned char *cursor;

  if (text == NULL)
    return false;

  for (cursor = (const unsigned char *)text; *cursor != '\0'; cursor++)
  {
    if (!isspace(*cursor))
      return true;
  }

  return false;
}

static bool spec_name_equals(const char *left, const char *right)
{
  const unsigned char *left_cursor;
  const unsigned char *right_cursor;

  if (left == NULL || right == NULL)
    return false;

  left_cursor = (const unsigned char *)left;
  right_cursor = (const unsigned char *)right;

  while (*left_cursor != '\0' && *right_cursor != '\0')
  {
    if (tolower(*left_cursor) != tolower(*right_cursor))
      return false;
    left_cursor++;
    right_cursor++;
  }

  return *left_cursor == *right_cursor;
}

static bool spec_validation_error(char *error, size_t error_size, const char *format, ...)
{
  va_list arguments;

  if (error != NULL && error_size > 0)
  {
    va_start(arguments, format);
    /* NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized) -- va_start initializes arguments. */
    vsnprintf(error, error_size, format, arguments);
    va_end(arguments);
  }

  return false;
}

static const char *spec_definition_identity(const struct spec_definition *definition)
{
  if (definition == NULL || !spec_text_present(definition->canonical_name))
    return "<unnamed>";

  return definition->canonical_name;
}

static const char *spec_definition_identity_name(const struct spec_definition *definition,
                                                 size_t name_index)
{
  if (name_index == 0)
    return definition->canonical_name;

  return definition->aliases[name_index - 1];
}

static spec_owner_mask spec_event_owner_mask(spec_event_mask event)
{
  switch (event)
  {
  case SPEC_EVENT_COMMAND:
    return SPEC_OWNER_ALL;
  case SPEC_EVENT_MOBILE_ACTIVITY:
  case SPEC_EVENT_MOBILE_COMBAT_TURN:
  case SPEC_EVENT_MOUNT_CHARGE:
    return SPEC_OWNER_MOBILE;
  case SPEC_EVENT_OBJECT_AUTO_PULSE:
  case SPEC_EVENT_ITEM_IDENTIFY:
  case SPEC_EVENT_WEAPON_HIT:
  case SPEC_EVENT_DEFENSE_REACTION:
  case SPEC_EVENT_COMBAT_MANEUVER:
    return SPEC_OWNER_OBJECT;
  case SPEC_EVENT_MOVING_ROOM_RELOCATION:
    return SPEC_OWNER_ROOM;
  default:
    return SPEC_OWNER_NONE;
  }
}

static spec_prototype_flag_mask spec_event_required_prototype_flags(spec_event_mask event)
{
  switch (event)
  {
  case SPEC_EVENT_MOBILE_ACTIVITY:
  case SPEC_EVENT_MOBILE_COMBAT_TURN:
    return SPEC_PROTOTYPE_MOB_SPEC;
  case SPEC_EVENT_OBJECT_AUTO_PULSE:
    return SPEC_PROTOTYPE_ITEM_AUTOPROC;
  default:
    return SPEC_PROTOTYPE_NONE;
  }
}

static spec_placement_mask spec_event_required_placement(spec_event_mask event)
{
  switch (event)
  {
  case SPEC_EVENT_MOBILE_COMBAT_TURN:
    return SPEC_PLACEMENT_COMBAT;
  case SPEC_EVENT_WEAPON_HIT:
  case SPEC_EVENT_DEFENSE_REACTION:
  case SPEC_EVENT_COMBAT_MANEUVER:
    return SPEC_PLACEMENT_EQUIPPED | SPEC_PLACEMENT_COMBAT;
  case SPEC_EVENT_MOUNT_CHARGE:
    return SPEC_PLACEMENT_MOUNTED | SPEC_PLACEMENT_COMBAT;
  case SPEC_EVENT_MOVING_ROOM_RELOCATION:
    return SPEC_PLACEMENT_MOVING_ROOM;
  default:
    return SPEC_PLACEMENT_NONE;
  }
}

static bool spec_validate_definition(const struct spec_definition *definition, size_t index,
                                     char *error, size_t error_size)
{
  size_t alias_index;
  size_t event_index;

  if (!spec_text_present(definition->canonical_name))
    return spec_validation_error(error, error_size, "definition[%zu]: canonical_name is empty",
                                 index);
  if (!spec_text_present(definition->display_name))
    return spec_validation_error(error, error_size, "definition[%zu] '%s': display_name is empty",
                                 index, definition->canonical_name);
  if (!spec_text_present(definition->category))
    return spec_validation_error(error, error_size, "definition[%zu] '%s': category is empty",
                                 index, definition->canonical_name);
  if (!spec_text_present(definition->description))
    return spec_validation_error(error, error_size, "definition[%zu] '%s': description is empty",
                                 index, definition->canonical_name);

  if (definition->alias_count > 0 && definition->aliases == NULL)
    return spec_validation_error(error, error_size,
                                 "definition[%zu] '%s': aliases is null with alias_count %zu",
                                 index, definition->canonical_name, definition->alias_count);
  for (alias_index = 0; alias_index < definition->alias_count; alias_index++)
  {
    if (!spec_text_present(definition->aliases[alias_index]))
      return spec_validation_error(error, error_size, "definition[%zu] '%s': alias[%zu] is empty",
                                   index, definition->canonical_name, alias_index);
  }

  if (definition->owner_mask == SPEC_OWNER_NONE || (definition->owner_mask & ~SPEC_OWNER_ALL) != 0)
    return spec_validation_error(error, error_size,
                                 "definition[%zu] '%s': owner_mask 0x%x is invalid", index,
                                 definition->canonical_name, definition->owner_mask);

  if (definition->event_count == 0)
    return spec_validation_error(error, error_size, "definition[%zu] '%s': event_count is zero",
                                 index, definition->canonical_name);
  if (definition->events == NULL)
    return spec_validation_error(error, error_size, "definition[%zu] '%s': events is null", index,
                                 definition->canonical_name);

  for (event_index = 0; event_index < definition->event_count; event_index++)
  {
    const struct spec_event_contract *event;
    spec_owner_mask allowed_owners;
    spec_prototype_flag_mask required_prototype_flags;
    spec_placement_mask required_placement;
    size_t previous_index;

    event = &definition->events[event_index];
    if (!spec_mask_is_one_bit(event->event) || (event->event & ~SPEC_EVENT_ALL) != 0)
      return spec_validation_error(error, error_size,
                                   "definition[%zu] '%s': event[%zu] mask 0x%x is invalid", index,
                                   definition->canonical_name, event_index, event->event);

    allowed_owners = spec_event_owner_mask(event->event);
    if ((definition->owner_mask & allowed_owners) == 0)
      return spec_validation_error(
          error, error_size,
          "definition[%zu] '%s': event[%zu] %s is incompatible with owner_mask 0x%x", index,
          definition->canonical_name, event_index, spec_event_name(event->event),
          definition->owner_mask);

    if ((event->required_prototype_flags & ~SPEC_PROTOTYPE_ALL) != 0)
      return spec_validation_error(
          error, error_size, "definition[%zu] '%s': event[%zu] prototype flags 0x%x are invalid",
          index, definition->canonical_name, event_index, event->required_prototype_flags);
    required_prototype_flags = spec_event_required_prototype_flags(event->event);
    if ((event->required_prototype_flags & required_prototype_flags) != required_prototype_flags)
      return spec_validation_error(
          error, error_size, "definition[%zu] '%s': event[%zu] %s is missing prototype flags 0x%x",
          index, definition->canonical_name, event_index, spec_event_name(event->event),
          required_prototype_flags);

    if ((event->required_placement & ~SPEC_PLACEMENT_ALL) != 0)
      return spec_validation_error(
          error, error_size, "definition[%zu] '%s': event[%zu] placement mask 0x%x is invalid",
          index, definition->canonical_name, event_index, event->required_placement);
    required_placement = spec_event_required_placement(event->event);
    if ((event->required_placement & required_placement) != required_placement)
      return spec_validation_error(
          error, error_size, "definition[%zu] '%s': event[%zu] %s is missing placement flags 0x%x",
          index, definition->canonical_name, event_index, spec_event_name(event->event),
          required_placement);

    for (previous_index = 0; previous_index < event_index; previous_index++)
    {
      if (definition->events[previous_index].event == event->event)
        return spec_validation_error(
            error, error_size, "definition[%zu] '%s': event[%zu] duplicates %s", index,
            definition->canonical_name, event_index, spec_event_name(event->event));
    }
  }

  if (definition->binding_source_mask == SPEC_BINDING_SOURCE_NONE ||
      (definition->binding_source_mask & ~SPEC_BINDING_SOURCE_ALL) != 0)
    return spec_validation_error(error, error_size,
                                 "definition[%zu] '%s': binding_source_mask 0x%x is invalid", index,
                                 definition->canonical_name, definition->binding_source_mask);

  if (definition->builder_visibility != SPEC_BUILDER_HIDDEN &&
      definition->builder_visibility != SPEC_BUILDER_VISIBLE)
    return spec_validation_error(error, error_size,
                                 "definition[%zu] '%s': builder_visibility %d is invalid", index,
                                 definition->canonical_name, definition->builder_visibility);

  if ((definition->legacy_handler == NULL) == (definition->typed_handler == NULL))
    return spec_validation_error(
        error, error_size,
        "definition[%zu] '%s': exactly one legacy_handler or typed_handler is required", index,
        definition->canonical_name);

  return true;
}

static bool spec_validate_unique_names(const struct spec_definition *definitions, size_t count,
                                       char *error, size_t error_size)
{
  size_t definition_index;

  for (definition_index = 0; definition_index < count; definition_index++)
  {
    const struct spec_definition *definition;
    size_t name_index;

    definition = &definitions[definition_index];
    for (name_index = 0; name_index <= definition->alias_count; name_index++)
    {
      const char *name;
      size_t other_definition_index;

      name = spec_definition_identity_name(definition, name_index);
      for (other_definition_index = definition_index; other_definition_index < count;
           other_definition_index++)
      {
        const struct spec_definition *other_definition;
        size_t other_name_index;
        size_t first_other_name;

        other_definition = &definitions[other_definition_index];
        first_other_name = other_definition_index == definition_index ? name_index + 1 : 0;
        for (other_name_index = first_other_name; other_name_index <= other_definition->alias_count;
             other_name_index++)
        {
          const char *other_name;

          other_name = spec_definition_identity_name(other_definition, other_name_index);
          if (spec_name_equals(name, other_name))
            return spec_validation_error(
                error, error_size,
                "definition[%zu] '%s': name '%s' collides with definition[%zu] '%s' name '%s'",
                definition_index, spec_definition_identity(definition), name,
                other_definition_index, spec_definition_identity(other_definition), other_name);
        }
      }
    }
  }

  return true;
}

static const char *spec_compatibility_name_at(size_t index)
{
  const struct spec_compatibility_name *entry;
  const struct spec_definition *definition;

  if (index >= SPEC_ARRAY_SIZE(compatibility_names))
    return NULL;

  entry = &compatibility_names[index];
  if (entry->definition_index < 0 || entry->definition_index >= SPEC_DEFINITION_INDEX_COUNT)
    return NULL;

  definition = &spec_definitions[entry->definition_index];
  if (entry->alias_index < 0)
    return definition->canonical_name;
  if ((size_t)entry->alias_index >= definition->alias_count)
    return NULL;

  return definition->aliases[entry->alias_index];
}

static bool spec_validate_compatibility_names(char *error, size_t error_size)
{
  size_t definition_index;
  size_t compatibility_index;

  for (compatibility_index = 0; compatibility_index < SPEC_ARRAY_SIZE(compatibility_names);
       compatibility_index++)
  {
    if (spec_compatibility_name_at(compatibility_index) == NULL)
      return spec_validation_error(error, error_size,
                                   "compatibility_names[%zu] references an invalid definition or "
                                   "alias",
                                   compatibility_index);
  }

  for (definition_index = 0; definition_index < SPEC_ARRAY_SIZE(spec_definitions);
       definition_index++)
  {
    const struct spec_definition *definition;
    size_t name_index;

    definition = &spec_definitions[definition_index];
    for (name_index = 0; name_index <= definition->alias_count; name_index++)
    {
      const char *name;
      size_t matches;

      name = spec_definition_identity_name(definition, name_index);
      matches = 0;
      for (compatibility_index = 0; compatibility_index < SPEC_ARRAY_SIZE(compatibility_names);
           compatibility_index++)
      {
        if (spec_name_equals(name, spec_compatibility_name_at(compatibility_index)))
          matches++;
      }
      if (matches != 1)
        return spec_validation_error(error, error_size,
                                     "definition[%zu] '%s': compatibility name '%s' occurs %zu "
                                     "times",
                                     definition_index, definition->canonical_name, name, matches);
    }
  }

  return true;
}

size_t spec_registry_count(void)
{
  return SPEC_ARRAY_SIZE(spec_definitions);
}

const struct spec_definition *spec_registry_get(int index)
{
  if (index < 0 || (size_t)index >= SPEC_ARRAY_SIZE(spec_definitions))
    return NULL;

  return &spec_definitions[index];
}

const struct spec_definition *spec_registry_find_by_name(const char *name)
{
  size_t definition_index;

  if (!spec_text_present(name))
    return NULL;

  for (definition_index = 0; definition_index < SPEC_ARRAY_SIZE(spec_definitions);
       definition_index++)
  {
    const struct spec_definition *definition;
    size_t alias_index;

    definition = &spec_definitions[definition_index];
    if (spec_name_equals(name, definition->canonical_name))
      return definition;

    for (alias_index = 0; alias_index < definition->alias_count; alias_index++)
    {
      if (spec_name_equals(name, definition->aliases[alias_index]))
        return definition;
    }
  }

  return NULL;
}

const struct spec_definition *spec_registry_find_for_owner(const char *name, spec_owner_mask owner)
{
  const struct spec_definition *definition;

  definition = spec_registry_find_by_name(name);
  if (!spec_definition_supports_owner(definition, owner))
    return NULL;

  return definition;
}

const struct spec_definition *spec_registry_find_by_handler(spec_legacy_handler handler)
{
  size_t definition_index;

  if (handler == NULL)
    return NULL;

  for (definition_index = 0; definition_index < SPEC_ARRAY_SIZE(spec_definitions);
       definition_index++)
  {
    if (spec_definitions[definition_index].legacy_handler == handler)
      return &spec_definitions[definition_index];
  }

  return NULL;
}

bool spec_definition_supports_owner(const struct spec_definition *definition, spec_owner_mask owner)
{
  if (definition == NULL || !spec_mask_is_one_bit(owner) || (owner & ~SPEC_OWNER_ALL) != 0)
    return false;

  return (definition->owner_mask & owner) != 0;
}

const struct spec_event_contract *
spec_definition_get_event(const struct spec_definition *definition, spec_event_mask event)
{
  size_t event_index;

  if (definition == NULL || !spec_mask_is_one_bit(event) || (event & ~SPEC_EVENT_ALL) != 0)
    return NULL;

  for (event_index = 0; event_index < definition->event_count; event_index++)
  {
    if (definition->events[event_index].event == event)
      return &definition->events[event_index];
  }

  return NULL;
}

bool spec_definition_supports_event(const struct spec_definition *definition, spec_owner_mask owner,
                                    spec_event_mask event)
{
  if (!spec_definition_supports_owner(definition, owner))
    return false;
  if ((spec_event_owner_mask(event) & owner) == 0)
    return false;

  return spec_definition_get_event(definition, event) != NULL;
}

bool spec_definition_allows_binding(const struct spec_definition *definition,
                                    spec_binding_source_mask source)
{
  if (definition == NULL || !spec_mask_is_one_bit(source) ||
      (source & ~SPEC_BINDING_SOURCE_ALL) != 0)
    return false;

  return (definition->binding_source_mask & source) != 0;
}

const char *spec_owner_name(spec_owner_mask owner)
{
  switch (owner)
  {
  case SPEC_OWNER_MOBILE:
    return "mobile";
  case SPEC_OWNER_OBJECT:
    return "object";
  case SPEC_OWNER_ROOM:
    return "room";
  default:
    return NULL;
  }
}

const char *spec_event_name(spec_event_mask event)
{
  switch (event)
  {
  case SPEC_EVENT_COMMAND:
    return "command";
  case SPEC_EVENT_MOBILE_ACTIVITY:
    return "mobile activity";
  case SPEC_EVENT_MOBILE_COMBAT_TURN:
    return "mobile combat turn";
  case SPEC_EVENT_OBJECT_AUTO_PULSE:
    return "object auto-pulse";
  case SPEC_EVENT_ITEM_IDENTIFY:
    return "item identification";
  case SPEC_EVENT_WEAPON_HIT:
    return "weapon hit";
  case SPEC_EVENT_DEFENSE_REACTION:
    return "defense reaction";
  case SPEC_EVENT_COMBAT_MANEUVER:
    return "combat maneuver";
  case SPEC_EVENT_MOUNT_CHARGE:
    return "mounted charge";
  case SPEC_EVENT_MOVING_ROOM_RELOCATION:
    return "moving-room relocation";
  default:
    return NULL;
  }
}

bool spec_registry_validate_definitions(const struct spec_definition *definitions, size_t count,
                                        char *error, size_t error_size)
{
  size_t definition_index;

  if (error != NULL && error_size > 0)
    error[0] = '\0';

  if (definitions == NULL)
    return spec_validation_error(error, error_size, "definition table is null");
  if (count == 0)
    return spec_validation_error(error, error_size, "definition table is empty");

  for (definition_index = 0; definition_index < count; definition_index++)
  {
    if (!spec_validate_definition(&definitions[definition_index], definition_index, error,
                                  error_size))
      return false;
  }

  return spec_validate_unique_names(definitions, count, error, error_size);
}

bool spec_registry_validate(char *error, size_t error_size)
{
  if (!spec_registry_validate_definitions(spec_definitions, SPEC_ARRAY_SIZE(spec_definitions),
                                          error, error_size))
    return false;

  return spec_validate_compatibility_names(error, error_size);
}

void spec_registry_boot_validate(void)
{
  char error[MAX_STRING_LENGTH];

  if (!spec_registry_validate(error, sizeof(error)))
  {
    log("SYSERR: Special-procedure registry validation failed: %s", error);
    exit(EXIT_FAILURE);
  }

  log("Special-procedure registry: validated %zu canonical definitions.", spec_registry_count());
}

int get_spec_func_count(void)
{
  return (int)SPEC_ARRAY_SIZE(compatibility_names);
}

const char *get_spec_func_name_by_index(int idx)
{
  if (idx < 0 || (size_t)idx >= SPEC_ARRAY_SIZE(compatibility_names))
    return NULL;

  return spec_compatibility_name_at((size_t)idx);
}

SPECIAL_DECL(*get_spec_func_by_index(int idx))
{
  const struct spec_compatibility_name *entry;

  if (idx < 0 || (size_t)idx >= SPEC_ARRAY_SIZE(compatibility_names))
    return NULL;

  entry = &compatibility_names[idx];
  if (entry->definition_index < 0 || entry->definition_index >= SPEC_DEFINITION_INDEX_COUNT)
    return NULL;

  return spec_definitions[entry->definition_index].legacy_handler;
}

SPECIAL_DECL(*find_spec_func_by_name(const char *name))
{
  const struct spec_definition *definition;

  definition = spec_registry_find_by_name(name);
  return definition == NULL ? NULL : definition->legacy_handler;
}

const char *get_spec_func_name(SPECIAL_DECL(*func))
{
  const struct spec_definition *definition;

  definition = spec_registry_find_by_handler(func);
  return definition == NULL ? NULL : definition->canonical_name;
}

#undef SPEC_BIND_WORLD_LEGACY
#undef SPEC_ARRAY_SIZE
