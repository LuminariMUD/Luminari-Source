/**
 * @file spec_registry.c
 * Immutable special-procedure definitions, validation, and compatibility accessors.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

#include "act.h"
#include "character/backgrounds.h"
#include "spec/spec_registry.h"
#include "comms/mail.h"
#include "craft/craft.h"
#include "craft/crafting_new.h"
#include "character/guild_services.h"
#include "character/vampire_cloak.h"
#include "magic/spellbook_scroll.h"
#include "obj/objsave.h"
#include "obj/player_shop.h"
#include "obj/treasure.h"
#include "obj/vendor.h"
#include "quest/hunts.h"
#include "quest/missions.h"
#include "quest/quest.h"
#include "spec/spec_mobile_archetypes.h"
#include "spec/spec_mobiles.h"
#include "spec/spec_rol_conversion.h"
#include "spec/spec_rol_lavatubes.h"
#include "spec/spec_rol_pilot.h"
#include "spec/spec_rol_tarrasque.h"
#include "spec/spec_rol_totem.h"
#include "spec/spec_rol_utility_objects.h"
#include "spec/spec_rooms.h"
#include "spec/spec_zone_abyss.h"
#include "spec/spec_zone_celestial_leviathan.h"
#include "spec/spec_zone_crimson_flame.h"
#include "spec/spec_zone_fire_giant.h"
#include "spec/spec_zone_jot.h"
#include "spec/spec_zone_mad_drow.h"
#include "spec/spec_zone_prisoner.h"
#include "vessels/vessels_legacy.h"
#include "vessels/vessels_rol.h"

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>

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

static const struct spec_event_contract rol_mobile_combat_events[] = {{
    SPEC_EVENT_MOBILE_COMBAT_TURN,
    SPEC_PROTOTYPE_MOB_SPEC,
    SPEC_PLACEMENT_COMBAT,
}};

static const struct spec_event_contract rol_mobile_activity_combat_events[] = {
    {SPEC_EVENT_MOBILE_ACTIVITY, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_COMBAT},
};

static const struct spec_event_contract rol_monster_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_MOBILE_ACTIVITY, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_COMBAT},
};

static const struct spec_event_contract rol_ship_navigator_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_COMBAT},
};

static const struct spec_event_contract rol_guild_guard_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_MOBILE_ACTIVITY, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_COMBAT},
};

static const struct spec_event_contract rol_bandit_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_MOBILE_ACTIVITY, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_NONE},
};

static const struct spec_event_contract rol_object_hit_events[] = {
    {SPEC_EVENT_ITEM_IDENTIFY, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_WEAPON_HIT, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_EQUIPPED | SPEC_PLACEMENT_COMBAT},
};

static const struct spec_event_contract rol_weapon_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_EQUIPPED},
    {SPEC_EVENT_OBJECT_AUTO_PULSE, SPEC_PROTOTYPE_ITEM_AUTOPROC, SPEC_PLACEMENT_EQUIPPED},
    {SPEC_EVENT_ITEM_IDENTIFY, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_WEAPON_HIT, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_EQUIPPED | SPEC_PLACEMENT_COMBAT},
};

static const struct spec_event_contract rol_object_command_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_EQUIPPED},
    {SPEC_EVENT_ITEM_IDENTIFY, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
};

static const struct spec_event_contract rol_object_pulse_events[] = {{
    SPEC_EVENT_OBJECT_AUTO_PULSE,
    SPEC_PROTOTYPE_ITEM_AUTOPROC,
    SPEC_PLACEMENT_EQUIPPED,
}};

static const struct spec_event_contract rol_room_object_pulse_events[] = {{
    SPEC_EVENT_OBJECT_AUTO_PULSE,
    SPEC_PROTOTYPE_ITEM_AUTOPROC,
    SPEC_PLACEMENT_NONE,
}};

static const struct spec_event_contract rol_tarrasque_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_MOBILE_ACTIVITY, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_MOBILE_COMBAT_TURN, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_COMBAT},
    {SPEC_EVENT_MOBILE_DEATH, SPEC_PROTOTYPE_MOB_SPEC, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_OBJECT_AUTO_PULSE, SPEC_PROTOTYPE_ITEM_AUTOPROC, SPEC_PLACEMENT_NONE},
};

static const struct spec_event_contract rol_utility_object_events[] = {
    {SPEC_EVENT_COMMAND, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_OBJECT_AUTO_PULSE, SPEC_PROTOTYPE_ITEM_AUTOPROC, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_ITEM_IDENTIFY, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_DEFENSE_REACTION, SPEC_PROTOTYPE_NONE,
     SPEC_PLACEMENT_EQUIPPED | SPEC_PLACEMENT_COMBAT},
    {SPEC_EVENT_COMBAT_MANEUVER, SPEC_PROTOTYPE_NONE,
     SPEC_PLACEMENT_EQUIPPED | SPEC_PLACEMENT_COMBAT},
};

static const struct spec_event_contract rol_object_defense_events[] = {
    {SPEC_EVENT_ITEM_IDENTIFY, SPEC_PROTOTYPE_NONE, SPEC_PLACEMENT_NONE},
    {SPEC_EVENT_DEFENSE_REACTION, SPEC_PROTOTYPE_NONE,
     SPEC_PLACEMENT_EQUIPPED | SPEC_PLACEMENT_COMBAT},
};

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
        .legacy_handler = NULL,
        .typed_adapter = bank,
        .typed_handler = bank_typed,
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
        .legacy_handler = NULL,
        .typed_adapter = vampire_cloak,
        .typed_handler = vampire_cloak_typed,
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
    {
        .canonical_name = "breath_attack_fire",
        .display_name = "breath_attack_fire",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Breathes half-strength fire at the current opponent every fourth turn.",
        .legacy_handler = rol_breath_attack_fire,
    },
    {
        .canonical_name = "breath_attack_acid",
        .display_name = "breath_attack_acid",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Breathes half-strength acid at the current opponent every fourth turn.",
        .legacy_handler = rol_breath_attack_acid,
    },
    {
        .canonical_name = "breath_attack_lightning",
        .display_name = "breath_attack_lightning",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description =
            "Breathes half-strength lightning at the current opponent every fourth turn.",
        .legacy_handler = rol_breath_attack_lightning,
    },
    {
        .canonical_name = "breath_weapon_fire",
        .display_name = "breath_weapon_fire",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_activity_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_activity_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Breathes full-strength fire across the room every fourth turn.",
        .legacy_handler = rol_breath_weapon_fire,
    },
    {
        .canonical_name = "breath_weapon_cold",
        .display_name = "breath_weapon_cold",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Breathes full-strength cold across the room every fourth turn.",
        .legacy_handler = rol_breath_weapon_cold,
    },
    {
        .canonical_name = "breath_weapon_acid",
        .display_name = "breath_weapon_acid",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Breathes full-strength acid across the room every fourth turn.",
        .legacy_handler = rol_breath_weapon_acid,
    },
    {
        .canonical_name = "breath_weapon_gas",
        .display_name = "breath_weapon_gas",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Breathes full-strength gas across the room every fourth turn.",
        .legacy_handler = rol_breath_weapon_gas,
    },
    {
        .canonical_name = "breath_weapon_lightning",
        .display_name = "breath_weapon_lightning",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_activity_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_activity_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Breathes full-strength lightning across the room every fourth turn.",
        .legacy_handler = rol_breath_weapon_lightning,
    },
    {
        .canonical_name = "hulburg_beholder_major",
        .display_name = "hulburg_beholder_major",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Uses the major Hulburg beholder eye-ray spell set in combat.",
        .legacy_handler = rol_hulburg_beholder_major,
    },
    {
        .canonical_name = "hulburg_beholder_minor",
        .display_name = "hulburg_beholder_minor",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Uses the minor Hulburg beholder eye-ray spell set in combat.",
        .legacy_handler = rol_hulburg_beholder_minor,
    },
    {
        .canonical_name = "money_changer",
        .display_name = "money_changer",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Adapts the legacy money changer to Luminari currency exchange.",
        .legacy_handler = rol_money_changer,
    },
    {
        .canonical_name = "plant_attacks_blindness",
        .display_name = "plant_attacks_blindness",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Sprays blinding pollen at eligible players in the room.",
        .legacy_handler = rol_plant_attacks_blindness,
    },
    {
        .canonical_name = "plant_attacks_paralysis",
        .display_name = "plant_attacks_paralysis",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Wraps eligible players in paralyzing vines.",
        .legacy_handler = rol_plant_attacks_paralysis,
    },
    {
        .canonical_name = "cemetary_black_blade",
        .display_name = "cemetary_black_blade",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Produces the cemetery black blade's single-target or area shadow burst.",
        .legacy_handler = rol_cemetery_black_blade,
    },
    {
        .canonical_name = "cemetary_cloakMeteors",
        .display_name = "cemetary_cloakMeteors",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_command_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Invokes a weekly starfall attack from the worn cemetery cloak.",
        .legacy_handler = rol_cemetery_cloak_meteors,
    },
    {
        .canonical_name = "cemetary_disruption",
        .display_name = "cemetary_disruption",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Deals disruption damage to undead targets.",
        .legacy_handler = rol_cemetery_disruption,
    },
    {
        .canonical_name = "cemetary_gleaming_blade",
        .display_name = "cemetary_gleaming_blade",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Produces the cemetery gleaming blade's single-target or area light burst.",
        .legacy_handler = rol_cemetery_gleaming_blade,
    },
    {
        .canonical_name = "cemetary_lightsaber",
        .display_name = "cemetary_lightsaber",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Fires energy beams and critical-hit light effects.",
        .legacy_handler = rol_cemetery_lightsaber,
    },
    {
        .canonical_name = "cemetary_skeletal_hand",
        .display_name = "cemetary_skeletal_hand",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_command_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Invokes Kelemvor's weekly protective blessing.",
        .legacy_handler = rol_cemetery_skeletal_hand,
    },
    {
        .canonical_name = "flaming_tanthorian",
        .display_name = "flaming_tanthorian",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Displays the source blue-flame effect on critical hits.",
        .legacy_handler = rol_flaming_tanthorian,
    },
    {
        .canonical_name = "longsword_tanthorian",
        .display_name = "longsword_tanthorian",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Displays the source deep-blue blade effect on critical hits.",
        .legacy_handler = rol_longsword_tanthorian,
    },
    {
        .canonical_name = "murlynds_spoon",
        .display_name = "murlynds_spoon",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_command_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Conjures a satiating feast once per day when invoked.",
        .legacy_handler = rol_murlynds_spoon,
    },
    {
        .canonical_name = "muspel_bec_de_corbin",
        .display_name = "muspel_bec_de_corbin",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Makes a reverse spike strike after a qualifying hit.",
        .legacy_handler = rol_muspel_bec_de_corbin,
    },
    {
        .canonical_name = "muspel_crystal_scimitar",
        .display_name = "muspel_crystal_scimitar",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Drains mental power from psionic opponents.",
        .legacy_handler = rol_muspel_crystal_scimitar,
    },
    {
        .canonical_name = "muspel_dagger_whispers",
        .display_name = "muspel_dagger_whispers",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Produces the sentient dagger's combat whispers.",
        .legacy_handler = rol_muspel_dagger_whispers,
    },
    {
        .canonical_name = "muspel_dragon_lance",
        .display_name = "muspel_dragon_lance",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Deals electrical critical damage when mounted against a dragon.",
        .legacy_handler = rol_muspel_dragon_lance,
    },
    {
        .canonical_name = "muspel_duergar_battlehammer",
        .display_name = "muspel_duergar_battlehammer",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Releases weakening dark energy on critical hits.",
        .legacy_handler = rol_muspel_duergar_battlehammer,
    },
    {
        .canonical_name = "muspel_recurve_bow",
        .display_name = "muspel_recurve_bow",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Produces the bow's sonic feedback damage.",
        .legacy_handler = rol_muspel_recurve_bow,
    },
    {
        .canonical_name = "muspel_spider_dagger",
        .display_name = "muspel_spider_dagger",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_hit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_hit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Releases spider venom on qualifying critical hits.",
        .legacy_handler = rol_muspel_spider_dagger,
    },
    {
        .canonical_name = "obj_drain",
        .display_name = "obj_drain",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_pulse_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_pulse_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Periodically drains life and movement from its wearer.",
        .legacy_handler = rol_obj_drain,
    },
    {
        .canonical_name = "thorn_shield",
        .display_name = "thorn_shield",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_defense_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_defense_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Pilot",
        .description = "Damages an attacker with thorns after a shield block.",
        .legacy_handler = rol_thorn_shield,
    },
    {
        .canonical_name = "RoL Guild Room",
        .display_name = "RoL Guild Room",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Provides current training services from a converted RoL guild room.",
        .legacy_handler = guild,
    },
    {
        .canonical_name = "RoL Mage Guild Room",
        .display_name = "RoL Mage Guild Room",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Provides current guild services only to members of a mage class family.",
        .legacy_handler = rol_mage_guild_room,
    },
    {
        .canonical_name = "RoL Thief Guild Room",
        .display_name = "RoL Thief Guild Room",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Provides current guild services only to members of a thief class family.",
        .legacy_handler = rol_thief_guild_room,
    },
    {
        .canonical_name = "RoL Warrior Guild Room",
        .display_name = "RoL Warrior Guild Room",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Provides current guild services only to members of a warrior class family.",
        .legacy_handler = rol_warrior_guild_room,
    },
    {
        .canonical_name = "RoL Cleric Guild Room",
        .display_name = "RoL Cleric Guild Room",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Provides current guild services only to members of a cleric class family.",
        .legacy_handler = rol_cleric_guild_room,
    },
    {
        .canonical_name = "RoL Bard Guild Room",
        .display_name = "RoL Bard Guild Room",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Provides current guild services only to members of the Bard class.",
        .legacy_handler = rol_bard_guild_room,
    },
    {
        .canonical_name = "RoL Waterdeep Guild Room",
        .display_name = "RoL Waterdeep Guild Room",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Provides current guild services through converted Waterdeep class rooms.",
        .legacy_handler = rol_waterdeep_guild_room,
    },
    {
        .canonical_name = "RoL Corpse Devourer",
        .display_name = "RoL Corpse Devourer",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Consumes food and non-player corpses while preserving corpse contents.",
        .legacy_handler = rol_corpse_devourer,
    },
    {
        .canonical_name = "RoL Poison Bite",
        .display_name = "RoL Poison Bite",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_activity_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_activity_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Uses the RoL level-scaled chance to poison the current opponent.",
        .legacy_handler = rol_poison_bite,
    },
    {
        .canonical_name = "RoL Thief",
        .display_name = "RoL Thief",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Attempts the RoL theft behavior against every eligible player each pulse.",
        .legacy_handler = rol_thief,
    },
    {
        .canonical_name = "RoL Bloodstone Portal",
        .display_name = "RoL Bloodstone Portal",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Transports characters through converted Bloodstone portals with stress.",
        .legacy_handler = rol_bloodstone_portal,
    },
    {
        .canonical_name = "RoL Portal Door",
        .display_name = "RoL Portal Door",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Peers through and enters converted alignment-gated RoL portals.",
        .legacy_handler = rol_portal_door,
    },
    {
        .canonical_name = "RoL Travel Portal",
        .display_name = "RoL Travel Portal",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Enters converted RoL folds, gates, fountains, spores, and travel portals.",
        .legacy_handler = rol_travel_portal,
    },
    {
        .canonical_name = "RoL Bloodstone Critter",
        .display_name = "RoL Bloodstone Critter",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Performs the converted Bloodstone critter snarl and growl cadence.",
        .legacy_handler = rol_bloodstone_critter,
    },
    {
        .canonical_name = "RoL Designated Follower",
        .display_name = "RoL Designated Follower",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Attaches converted Icecrag guards to their designated mobile leader.",
        .legacy_handler = rol_designated_follower,
    },
    {
        .canonical_name = "RoL Fixed Bodyguard",
        .display_name = "RoL Fixed Bodyguard",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Rescues the converted Icecrag mobile assigned to this bodyguard.",
        .legacy_handler = rol_fixed_bodyguard,
    },
    {
        .canonical_name = "RoL Floating Pool",
        .display_name = "RoL Floating Pool",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_room_object_pulse_events,
        .event_count = SPEC_ARRAY_SIZE(rol_room_object_pulse_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Moves a converted ethereal pool through eligible exits on auto-pulses.",
        .legacy_handler = rol_floating_pool,
    },
    {
        .canonical_name = "RoL Item Blocker",
        .display_name = "RoL Item Blocker",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Blocks one converted exit while an aggressive mobile occupies the room.",
        .legacy_handler = rol_item_blocker,
    },
    {
        .canonical_name = "RoL Magic Pool",
        .display_name = "RoL Magic Pool",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Damages and transports characters that enter a converted RoL magic pool.",
        .legacy_handler = rol_magic_pool,
    },
    {
        .canonical_name = "RoL Banana",
        .display_name = "RoL Banana",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Turns converted bananas into temporary hazardous peels when eaten.",
        .typed_adapter = rol_banana,
        .typed_handler = rol_banana_typed,
    },
    {
        .canonical_name = "RoL Auto Distributor",
        .display_name = "RoL Auto Distributor",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Randomly redistributes non-staff characters within the current zone.",
        .legacy_handler = rol_auto_distributor,
    },
    {
        .canonical_name = "RoL Command Sentinel",
        .display_name = "RoL Command Sentinel",
        .owner_mask = SPEC_OWNER_MOBILE | SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Enforces converted RoL passage guards, command wards, and class glyphs.",
        .typed_adapter = rol_command_sentinel,
        .typed_handler = rol_command_sentinel_typed,
    },
    {
        .canonical_name = "RoL Toll Keeper",
        .display_name = "RoL Toll Keeper",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_bandit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_bandit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Collects converted tolls and tickets at fixed RoL passages.",
        .typed_adapter = rol_toll_keeper,
        .typed_handler = rol_toll_keeper_typed,
    },
    {
        .canonical_name = "RoL Shadow Giant",
        .display_name = "RoL Shadow Giant",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Periodically spooks every eligible player and pet while fighting.",
        .legacy_handler = rol_shadow_giant,
    },
    {
        .canonical_name = "RoL Guild Guard",
        .display_name = "RoL Guild Guard",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_guild_guard_events,
        .event_count = SPEC_ARRAY_SIZE(rol_guild_guard_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Enforces converted RoL guild gates, retaliates against attackers, and "
                       "emits composed Waterdeep guard flavor.",
        .typed_adapter = rol_guild_guard,
        .typed_handler = rol_guild_guard_typed,
    },
    {
        .canonical_name = "RoL Major Beholder",
        .display_name = "RoL Major Beholder",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Fires ten independently cooling converted beholder eye rays in combat.",
        .legacy_handler = rol_major_beholder,
    },
    {
        .canonical_name = "RoL Monster Combat",
        .display_name = "RoL Monster Combat",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_monster_events,
        .event_count = SPEC_ARRAY_SIZE(rol_monster_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Runs source-profiled combat effects for converted RoL monsters.",
        .typed_adapter = rol_monster_combat,
        .typed_handler = rol_monster_combat_typed,
    },
    {
        .canonical_name = "RoL Lich Energy Drain",
        .display_name = "RoL Lich Energy Drain",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_activity_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_activity_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Drains a current opponent or party member and transfers its life force.",
        .legacy_handler = rol_lich_energy_drain,
    },
    {
        .canonical_name = "RoL Lich Rite",
        .display_name = "RoL Lich Rite",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description =
            "Consumes the converted offerings and safely rebuilds a level-30 Necromancer as a "
            "Lich.",
        .legacy_handler = rol_lich_rite,
    },
    {
        .canonical_name = "RoL Undead Drain",
        .display_name = "RoL Undead Drain",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Applies source-profiled weakening, sickening, and slowing undead attacks.",
        .legacy_handler = rol_undead_drain,
    },
    {
        .canonical_name = "RoL Trade Bandit",
        .display_name = "RoL Trade Bandit",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_bandit_events,
        .event_count = SPEC_ARRAY_SIZE(rol_bandit_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Extorts converted wagon cargo through seven source-specific tolls.",
        .legacy_handler = rol_bandit,
    },
    {
        .canonical_name = "RoL Sister Knight",
        .display_name = "RoL Sister Knight",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_activity_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_activity_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Alerts converted Sister Knights and sends eligible sisters in pursuit.",
        .legacy_handler = rol_sister_knight,
    },
    {
        .canonical_name = "RoL Shaman Totem",
        .display_name = "RoL Shaman Totem",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_object_command_events,
        .event_count = SPEC_ARRAY_SIZE(rol_object_command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Bonds a Cleric to one converted RoL spirit totem and summons its spirit.",
        .legacy_handler = rol_shaman_totem,
    },
    {
        .canonical_name = "RoL Totem Restorer",
        .display_name = "RoL Totem Restorer",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description =
            "Replaces a qualifying Cleric's lost converted spirit totem for 10,000 gold.",
        .legacy_handler = rol_totem_restorer,
    },
    {
        .canonical_name = "RoL Ship",
        .display_name = "RoL Ship",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Boards one of the seven converted RoL fixed-interior ships.",
        .legacy_handler = rol_ship,
    },
    {
        .canonical_name = "RoL Ship Control",
        .display_name = "RoL Ship Control",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Provides instruments, steering, speed, combat, and docking orders.",
        .legacy_handler = rol_ship_control,
    },
    {
        .canonical_name = "RoL Ship Exit",
        .display_name = "RoL Ship Exit",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Lets passengers look outside and disembark from a converted RoL ship.",
        .legacy_handler = rol_ship_exit,
    },
    {
        .canonical_name = "RoL Ship Lookout",
        .display_name = "RoL Ship Lookout",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Shows the exterior room from converted RoL ship lookout rooms.",
        .legacy_handler = rol_ship_lookout,
    },
    {
        .canonical_name = "RoL Ship Navigator",
        .display_name = "RoL Ship Navigator",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_ship_navigator_events,
        .event_count = SPEC_ARRAY_SIZE(rol_ship_navigator_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description =
            "Protects ship orders, drives scheduled routes, and calls crew during combat.",
        .legacy_handler = rol_ship_navigator,
    },
    {
        .canonical_name = "RoL Alert Caller",
        .display_name = "RoL Alert Caller",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_activity_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_activity_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Broadcasts a source-specific combat alert and sends eligible helpers in "
                       "pursuit.",
        .legacy_handler = rol_alert_caller,
    },
    {
        .canonical_name = "RoL Yggdrasil Branch",
        .display_name = "RoL Yggdrasil Branch",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = rol_mobile_combat_events,
        .event_count = SPEC_ARRAY_SIZE(rol_mobile_combat_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Entangles a current or vulnerable opponent for four to twelve combat "
                       "rounds.",
        .legacy_handler = rol_yggdrasil_branch,
    },
    {
        .canonical_name = "RoL Waterdeep Ambient",
        .display_name = "RoL Waterdeep Ambient",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Emits source-authored periodic speech and actions for converted Waterdeep "
                       "citizens.",
        .legacy_handler = rol_waterdeep_ambient,
    },
    {
        .canonical_name = "RoL Waterdeep Peacekeeper",
        .display_name = "RoL Waterdeep Peacekeeper",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Returns displaced Waterdeep bouncers home, ejects aggressors, and lets "
                       "off-duty guards intervene in fights.",
        .legacy_handler = rol_waterdeep_peacekeeper,
    },
    {
        .canonical_name = "RoL Weapon Proc",
        .display_name = "RoL Weapon Proc",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_weapon_events,
        .event_count = SPEC_ARRAY_SIZE(rol_weapon_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Dispatches converted RoL weapon behavior by exact object identity, "
                       "including critical and attack payloads.",
        .typed_adapter = rol_weapon_proc,
        .typed_handler = rol_weapon_proc_typed,
    },
    {
        .canonical_name = "RoL Source Periodic",
        .display_name = "RoL Source Periodic",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Emits generated source-authored periodic speech and room actions for "
                       "converted RoL mobiles.",
        .legacy_handler = rol_source_periodic,
    },
    {
        .canonical_name = "RoL Stateful Periodic",
        .display_name = "RoL Stateful Periodic",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Emits generated source-authored idle or fighting speech and room actions "
                       "for converted RoL mobiles; some profiles compose through other adapters.",
        .legacy_handler = rol_state_periodic,
    },
    {
        .canonical_name = "RoL Lavatubes Mobile",
        .display_name = "RoL Lavatubes Mobile",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Runs converted snow-vulture activity and the cellar automaton reset.",
        .typed_adapter = rol_lavatubes_mobile,
        .typed_handler = rol_lavatubes_mobile_typed,
    },
    {
        .canonical_name = "RoL Lavatubes Object",
        .display_name = "RoL Lavatubes Object",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Runs the converted crystal spike, skeleton key, and automaton lever.",
        .typed_adapter = rol_lavatubes_object,
        .typed_handler = rol_lavatubes_object_typed,
    },
    {
        .canonical_name = "RoL Lavatubes Room",
        .display_name = "RoL Lavatubes Room",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Closes and blocks the converted automaton-cellar trapdoor after descent.",
        .typed_adapter = rol_lavatubes_room,
        .typed_handler = rol_lavatubes_room_typed,
    },
    {
        .canonical_name = "RoL Tarrasque Encounter",
        .display_name = "RoL Tarrasque Encounter",
        .owner_mask = SPEC_OWNER_MOBILE | SPEC_OWNER_OBJECT,
        .events = rol_tarrasque_events,
        .event_count = SPEC_ARRAY_SIZE(rol_tarrasque_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Runs the converted Tarrasque combat, stomach acid, special death loot, "
                       "and corpse entrance as one encounter contract.",
        .typed_adapter = rol_tarrasque,
        .typed_handler = rol_tarrasque_typed,
    },
    {
        .canonical_name = "RoL Utility Object",
        .display_name = "RoL Utility Object",
        .owner_mask = SPEC_OWNER_OBJECT,
        .events = rol_utility_object_events,
        .event_count = SPEC_ARRAY_SIZE(rol_utility_object_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Runs converted utility-object services by exact object identity, "
                       "including disease reservoirs and protected loot.",
        .typed_adapter = rol_utility_object,
        .typed_handler = rol_utility_object_typed,
    },
    {
        .canonical_name = "RoL Utility Room",
        .display_name = "RoL Utility Room",
        .owner_mask = SPEC_OWNER_ROOM,
        .events = command_events,
        .event_count = SPEC_ARRAY_SIZE(command_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Runs converted newbie routing and room-weight threshold behavior by "
                       "exact room identity.",
        .typed_adapter = rol_utility_room,
        .typed_handler = rol_utility_room_typed,
    },
    {
        .canonical_name = "RoL Scheduled Mobile",
        .display_name = "RoL Scheduled Mobile",
        .owner_mask = SPEC_OWNER_MOBILE,
        .events = janitor_events,
        .event_count = SPEC_ARRAY_SIZE(janitor_events),
        .binding_source_mask = SPEC_BINDING_SOURCE_WORLD,
        .builder_visibility = SPEC_BUILDER_VISIBLE,
        .category = "RoL Conversion",
        .description = "Runs converted timed gate, lighthouse, naval, and Waterdeep town-crier "
                       "behavior by exact mobile identity.",
        .legacy_handler = rol_scheduled_mobile,
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
  SPEC_DEFINITION_ROL_BREATH_ATTACK_FIRE,
  SPEC_DEFINITION_ROL_BREATH_ATTACK_ACID,
  SPEC_DEFINITION_ROL_BREATH_ATTACK_LIGHTNING,
  SPEC_DEFINITION_ROL_BREATH_WEAPON_FIRE,
  SPEC_DEFINITION_ROL_BREATH_WEAPON_COLD,
  SPEC_DEFINITION_ROL_BREATH_WEAPON_ACID,
  SPEC_DEFINITION_ROL_BREATH_WEAPON_GAS,
  SPEC_DEFINITION_ROL_BREATH_WEAPON_LIGHTNING,
  SPEC_DEFINITION_ROL_HULBURG_BEHOLDER_MAJOR,
  SPEC_DEFINITION_ROL_HULBURG_BEHOLDER_MINOR,
  SPEC_DEFINITION_ROL_MONEY_CHANGER,
  SPEC_DEFINITION_ROL_PLANT_ATTACKS_BLINDNESS,
  SPEC_DEFINITION_ROL_PLANT_ATTACKS_PARALYSIS,
  SPEC_DEFINITION_ROL_CEMETARY_BLACK_BLADE,
  SPEC_DEFINITION_ROL_CEMETARY_CLOAK_METEORS,
  SPEC_DEFINITION_ROL_CEMETARY_DISRUPTION,
  SPEC_DEFINITION_ROL_CEMETARY_GLEAMING_BLADE,
  SPEC_DEFINITION_ROL_CEMETARY_LIGHTSABER,
  SPEC_DEFINITION_ROL_CEMETARY_SKELETAL_HAND,
  SPEC_DEFINITION_ROL_FLAMING_TANTHORIAN,
  SPEC_DEFINITION_ROL_LONGSWORD_TANTHORIAN,
  SPEC_DEFINITION_ROL_MURLYNDS_SPOON,
  SPEC_DEFINITION_ROL_MUSPEL_BEC_DE_CORBIN,
  SPEC_DEFINITION_ROL_MUSPEL_CRYSTAL_SCIMITAR,
  SPEC_DEFINITION_ROL_MUSPEL_DAGGER_WHISPERS,
  SPEC_DEFINITION_ROL_MUSPEL_DRAGON_LANCE,
  SPEC_DEFINITION_ROL_MUSPEL_DUERGAR_BATTLEHAMMER,
  SPEC_DEFINITION_ROL_MUSPEL_RECURVE_BOW,
  SPEC_DEFINITION_ROL_MUSPEL_SPIDER_DAGGER,
  SPEC_DEFINITION_ROL_OBJ_DRAIN,
  SPEC_DEFINITION_ROL_THORN_SHIELD,
  SPEC_DEFINITION_ROL_GUILD_ROOM,
  SPEC_DEFINITION_ROL_MAGE_GUILD_ROOM,
  SPEC_DEFINITION_ROL_THIEF_GUILD_ROOM,
  SPEC_DEFINITION_ROL_WARRIOR_GUILD_ROOM,
  SPEC_DEFINITION_ROL_CLERIC_GUILD_ROOM,
  SPEC_DEFINITION_ROL_BARD_GUILD_ROOM,
  SPEC_DEFINITION_ROL_WATERDEEP_GUILD_ROOM,
  SPEC_DEFINITION_ROL_CORPSE_DEVOURER,
  SPEC_DEFINITION_ROL_POISON_BITE,
  SPEC_DEFINITION_ROL_THIEF,
  SPEC_DEFINITION_ROL_BLOODSTONE_PORTAL,
  SPEC_DEFINITION_ROL_PORTAL_DOOR,
  SPEC_DEFINITION_ROL_TRAVEL_PORTAL,
  SPEC_DEFINITION_ROL_BLOODSTONE_CRITTER,
  SPEC_DEFINITION_ROL_DESIGNATED_FOLLOWER,
  SPEC_DEFINITION_ROL_FIXED_BODYGUARD,
  SPEC_DEFINITION_ROL_FLOATING_POOL,
  SPEC_DEFINITION_ROL_ITEM_BLOCKER,
  SPEC_DEFINITION_ROL_MAGIC_POOL,
  SPEC_DEFINITION_ROL_BANANA,
  SPEC_DEFINITION_ROL_AUTO_DISTRIBUTOR,
  SPEC_DEFINITION_ROL_COMMAND_SENTINEL,
  SPEC_DEFINITION_ROL_TOLL_KEEPER,
  SPEC_DEFINITION_ROL_SHADOW_GIANT,
  SPEC_DEFINITION_ROL_GUILD_GUARD,
  SPEC_DEFINITION_ROL_MAJOR_BEHOLDER,
  SPEC_DEFINITION_ROL_MONSTER_COMBAT,
  SPEC_DEFINITION_ROL_LICH_ENERGY_DRAIN,
  SPEC_DEFINITION_ROL_LICH_RITE,
  SPEC_DEFINITION_ROL_UNDEAD_DRAIN,
  SPEC_DEFINITION_ROL_TRADE_BANDIT,
  SPEC_DEFINITION_ROL_SISTER_KNIGHT,
  SPEC_DEFINITION_ROL_SHAMAN_TOTEM,
  SPEC_DEFINITION_ROL_TOTEM_RESTORER,
  SPEC_DEFINITION_ROL_SHIP,
  SPEC_DEFINITION_ROL_SHIP_CONTROL,
  SPEC_DEFINITION_ROL_SHIP_EXIT,
  SPEC_DEFINITION_ROL_SHIP_LOOKOUT,
  SPEC_DEFINITION_ROL_SHIP_NAVIGATOR,
  SPEC_DEFINITION_ROL_ALERT_CALLER,
  SPEC_DEFINITION_ROL_YGGDRASIL_BRANCH,
  SPEC_DEFINITION_ROL_WATERDEEP_AMBIENT,
  SPEC_DEFINITION_ROL_WATERDEEP_PEACEKEEPER,
  SPEC_DEFINITION_ROL_WEAPON_PROC,
  SPEC_DEFINITION_ROL_SOURCE_PERIODIC,
  SPEC_DEFINITION_ROL_STATEFUL_PERIODIC,
  SPEC_DEFINITION_ROL_LAVATUBES_MOBILE,
  SPEC_DEFINITION_ROL_LAVATUBES_OBJECT,
  SPEC_DEFINITION_ROL_LAVATUBES_ROOM,
  SPEC_DEFINITION_ROL_TARRASQUE_ENCOUNTER,
  SPEC_DEFINITION_ROL_UTILITY_OBJECT,
  SPEC_DEFINITION_ROL_UTILITY_ROOM,
  SPEC_DEFINITION_ROL_SCHEDULED_MOBILE,
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
    {SPEC_DEFINITION_ROL_BREATH_ATTACK_FIRE, -1},
    {SPEC_DEFINITION_ROL_BREATH_ATTACK_ACID, -1},
    {SPEC_DEFINITION_ROL_BREATH_ATTACK_LIGHTNING, -1},
    {SPEC_DEFINITION_ROL_BREATH_WEAPON_FIRE, -1},
    {SPEC_DEFINITION_ROL_BREATH_WEAPON_COLD, -1},
    {SPEC_DEFINITION_ROL_BREATH_WEAPON_ACID, -1},
    {SPEC_DEFINITION_ROL_BREATH_WEAPON_GAS, -1},
    {SPEC_DEFINITION_ROL_BREATH_WEAPON_LIGHTNING, -1},
    {SPEC_DEFINITION_ROL_HULBURG_BEHOLDER_MAJOR, -1},
    {SPEC_DEFINITION_ROL_HULBURG_BEHOLDER_MINOR, -1},
    {SPEC_DEFINITION_ROL_MONEY_CHANGER, -1},
    {SPEC_DEFINITION_ROL_PLANT_ATTACKS_BLINDNESS, -1},
    {SPEC_DEFINITION_ROL_PLANT_ATTACKS_PARALYSIS, -1},
    {SPEC_DEFINITION_ROL_CEMETARY_BLACK_BLADE, -1},
    {SPEC_DEFINITION_ROL_CEMETARY_CLOAK_METEORS, -1},
    {SPEC_DEFINITION_ROL_CEMETARY_DISRUPTION, -1},
    {SPEC_DEFINITION_ROL_CEMETARY_GLEAMING_BLADE, -1},
    {SPEC_DEFINITION_ROL_CEMETARY_LIGHTSABER, -1},
    {SPEC_DEFINITION_ROL_CEMETARY_SKELETAL_HAND, -1},
    {SPEC_DEFINITION_ROL_FLAMING_TANTHORIAN, -1},
    {SPEC_DEFINITION_ROL_LONGSWORD_TANTHORIAN, -1},
    {SPEC_DEFINITION_ROL_MURLYNDS_SPOON, -1},
    {SPEC_DEFINITION_ROL_MUSPEL_BEC_DE_CORBIN, -1},
    {SPEC_DEFINITION_ROL_MUSPEL_CRYSTAL_SCIMITAR, -1},
    {SPEC_DEFINITION_ROL_MUSPEL_DAGGER_WHISPERS, -1},
    {SPEC_DEFINITION_ROL_MUSPEL_DRAGON_LANCE, -1},
    {SPEC_DEFINITION_ROL_MUSPEL_DUERGAR_BATTLEHAMMER, -1},
    {SPEC_DEFINITION_ROL_MUSPEL_RECURVE_BOW, -1},
    {SPEC_DEFINITION_ROL_MUSPEL_SPIDER_DAGGER, -1},
    {SPEC_DEFINITION_ROL_OBJ_DRAIN, -1},
    {SPEC_DEFINITION_ROL_THORN_SHIELD, -1},
    {SPEC_DEFINITION_ROL_GUILD_ROOM, -1},
    {SPEC_DEFINITION_ROL_MAGE_GUILD_ROOM, -1},
    {SPEC_DEFINITION_ROL_THIEF_GUILD_ROOM, -1},
    {SPEC_DEFINITION_ROL_WARRIOR_GUILD_ROOM, -1},
    {SPEC_DEFINITION_ROL_CLERIC_GUILD_ROOM, -1},
    {SPEC_DEFINITION_ROL_BARD_GUILD_ROOM, -1},
    {SPEC_DEFINITION_ROL_WATERDEEP_GUILD_ROOM, -1},
    {SPEC_DEFINITION_ROL_CORPSE_DEVOURER, -1},
    {SPEC_DEFINITION_ROL_POISON_BITE, -1},
    {SPEC_DEFINITION_ROL_THIEF, -1},
    {SPEC_DEFINITION_ROL_BLOODSTONE_PORTAL, -1},
    {SPEC_DEFINITION_ROL_PORTAL_DOOR, -1},
    {SPEC_DEFINITION_ROL_TRAVEL_PORTAL, -1},
    {SPEC_DEFINITION_ROL_BLOODSTONE_CRITTER, -1},
    {SPEC_DEFINITION_ROL_DESIGNATED_FOLLOWER, -1},
    {SPEC_DEFINITION_ROL_FIXED_BODYGUARD, -1},
    {SPEC_DEFINITION_ROL_FLOATING_POOL, -1},
    {SPEC_DEFINITION_ROL_ITEM_BLOCKER, -1},
    {SPEC_DEFINITION_ROL_MAGIC_POOL, -1},
    {SPEC_DEFINITION_ROL_BANANA, -1},
    {SPEC_DEFINITION_ROL_AUTO_DISTRIBUTOR, -1},
    {SPEC_DEFINITION_ROL_COMMAND_SENTINEL, -1},
    {SPEC_DEFINITION_ROL_TOLL_KEEPER, -1},
    {SPEC_DEFINITION_ROL_SHADOW_GIANT, -1},
    {SPEC_DEFINITION_ROL_GUILD_GUARD, -1},
    {SPEC_DEFINITION_ROL_MAJOR_BEHOLDER, -1},
    {SPEC_DEFINITION_ROL_MONSTER_COMBAT, -1},
    {SPEC_DEFINITION_ROL_LICH_ENERGY_DRAIN, -1},
    {SPEC_DEFINITION_ROL_LICH_RITE, -1},
    {SPEC_DEFINITION_ROL_UNDEAD_DRAIN, -1},
    {SPEC_DEFINITION_ROL_TRADE_BANDIT, -1},
    {SPEC_DEFINITION_ROL_SISTER_KNIGHT, -1},
    {SPEC_DEFINITION_ROL_SHAMAN_TOTEM, -1},
    {SPEC_DEFINITION_ROL_TOTEM_RESTORER, -1},
    {SPEC_DEFINITION_ROL_SHIP, -1},
    {SPEC_DEFINITION_ROL_SHIP_CONTROL, -1},
    {SPEC_DEFINITION_ROL_SHIP_EXIT, -1},
    {SPEC_DEFINITION_ROL_SHIP_LOOKOUT, -1},
    {SPEC_DEFINITION_ROL_SHIP_NAVIGATOR, -1},
    {SPEC_DEFINITION_ROL_ALERT_CALLER, -1},
    {SPEC_DEFINITION_ROL_YGGDRASIL_BRANCH, -1},
    {SPEC_DEFINITION_ROL_WATERDEEP_AMBIENT, -1},
    {SPEC_DEFINITION_ROL_WATERDEEP_PEACEKEEPER, -1},
    {SPEC_DEFINITION_ROL_WEAPON_PROC, -1},
    {SPEC_DEFINITION_ROL_SOURCE_PERIODIC, -1},
    {SPEC_DEFINITION_ROL_STATEFUL_PERIODIC, -1},
    {SPEC_DEFINITION_ROL_LAVATUBES_MOBILE, -1},
    {SPEC_DEFINITION_ROL_LAVATUBES_OBJECT, -1},
    {SPEC_DEFINITION_ROL_LAVATUBES_ROOM, -1},
    {SPEC_DEFINITION_ROL_TARRASQUE_ENCOUNTER, -1},
    {SPEC_DEFINITION_ROL_UTILITY_OBJECT, -1},
    {SPEC_DEFINITION_ROL_UTILITY_ROOM, -1},
    {SPEC_DEFINITION_ROL_SCHEDULED_MOBILE, -1},
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
  case SPEC_EVENT_MOBILE_DEATH:
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
  case SPEC_EVENT_MOBILE_DEATH:
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

  if (definition->legacy_handler != NULL)
  {
    if (definition->typed_adapter != NULL || definition->typed_handler != NULL)
      return spec_validation_error(
          error, error_size,
          "definition[%zu] '%s': legacy behavior cannot also define a typed adapter or handler",
          index, definition->canonical_name);
  }
  else if (definition->typed_adapter == NULL || definition->typed_handler == NULL)
    return spec_validation_error(
        error, error_size,
        "definition[%zu] '%s': typed behavior requires both typed_adapter and typed_handler", index,
        definition->canonical_name);

  return true;
}

static bool spec_validate_unique_callbacks(const struct spec_definition *definitions, size_t count,
                                           char *error, size_t error_size)
{
  spec_legacy_handler callback;
  spec_legacy_handler other_callback;
  size_t definition_index;
  size_t other_index;

  for (definition_index = 0; definition_index < count; definition_index++)
  {
    callback = spec_definition_callback(&definitions[definition_index]);
    for (other_index = definition_index + 1U; other_index < count; other_index++)
    {
      other_callback = spec_definition_callback(&definitions[other_index]);
      if (callback == other_callback && (definitions[definition_index].typed_handler != NULL ||
                                         definitions[other_index].typed_handler != NULL))
        return spec_validation_error(
            error, error_size,
            "definition[%zu] '%s': typed adapter collides with definition[%zu] '%s'",
            definition_index, definitions[definition_index].canonical_name, other_index,
            definitions[other_index].canonical_name);
    }
  }

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

size_t spec_registry_legacy_count(void)
{
  size_t count;
  size_t definition_index;

  count = 0;
  for (definition_index = 0; definition_index < SPEC_ARRAY_SIZE(spec_definitions);
       definition_index++)
    if (spec_definitions[definition_index].legacy_handler != NULL)
      count++;

  return count;
}

size_t spec_registry_typed_count(void)
{
  return spec_registry_count() - spec_registry_legacy_count();
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
    if (spec_definition_callback(&spec_definitions[definition_index]) == handler)
      return &spec_definitions[definition_index];
  }

  return NULL;
}

spec_legacy_handler spec_definition_callback(const struct spec_definition *definition)
{
  if (definition == NULL)
    return NULL;

  return definition->legacy_handler != NULL ? definition->legacy_handler
                                            : definition->typed_adapter;
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
  case SPEC_EVENT_MOBILE_DEATH:
    return "mobile death";
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

  if (!spec_validate_unique_callbacks(definitions, count, error, error_size))
    return false;

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

  log("Special-procedure registry: validated %zu canonical definitions (%zu legacy, %zu typed).",
      spec_registry_count(), spec_registry_legacy_count(), spec_registry_typed_count());
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

spec_legacy_handler get_spec_func_by_index(int idx)
{
  const struct spec_compatibility_name *entry;

  if (idx < 0 || (size_t)idx >= SPEC_ARRAY_SIZE(compatibility_names))
    return NULL;

  entry = &compatibility_names[idx];
  if (entry->definition_index < 0 || entry->definition_index >= SPEC_DEFINITION_INDEX_COUNT)
    return NULL;

  return spec_definition_callback(&spec_definitions[entry->definition_index]);
}

spec_legacy_handler find_spec_func_by_name(const char *name)
{
  const struct spec_definition *definition;

  definition = spec_registry_find_by_name(name);
  return spec_definition_callback(definition);
}

const char *get_spec_func_name(spec_legacy_handler func)
{
  const struct spec_definition *definition;

  definition = spec_registry_find_by_handler(func);
  return definition == NULL ? NULL : definition->canonical_name;
}

#undef SPEC_BIND_WORLD_LEGACY
#undef SPEC_ARRAY_SIZE
