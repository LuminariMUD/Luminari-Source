/**
 * @file spec_registry.h
 * Immutable special-procedure definition metadata and lookup API.
 */

#ifndef LUMINARI_SPEC_REGISTRY_H
#define LUMINARI_SPEC_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct char_data;
struct spec_event_context;

/** Legacy callback type retained by the compatibility registry. */
typedef int (*spec_legacy_handler)(struct char_data *ch, void *me, int cmd, const char *argument);
/** Typed callback shape used after an event gateway has built complete context. */
typedef int (*spec_typed_handler)(struct spec_event_context *context);

typedef uint32_t spec_owner_mask;
enum spec_owner_flag
{
  SPEC_OWNER_NONE = 0,
  SPEC_OWNER_MOBILE = (1U << 0),
  SPEC_OWNER_OBJECT = (1U << 1),
  SPEC_OWNER_ROOM = (1U << 2)
};

#define SPEC_OWNER_ALL (SPEC_OWNER_MOBILE | SPEC_OWNER_OBJECT | SPEC_OWNER_ROOM)

typedef uint32_t spec_event_mask;
enum spec_event_flag
{
  SPEC_EVENT_NONE = 0,
  SPEC_EVENT_COMMAND = (1U << 0),
  SPEC_EVENT_MOBILE_ACTIVITY = (1U << 1),
  SPEC_EVENT_MOBILE_COMBAT_TURN = (1U << 2),
  SPEC_EVENT_OBJECT_AUTO_PULSE = (1U << 3),
  SPEC_EVENT_ITEM_IDENTIFY = (1U << 4),
  SPEC_EVENT_WEAPON_HIT = (1U << 5),
  SPEC_EVENT_DEFENSE_REACTION = (1U << 6),
  SPEC_EVENT_COMBAT_MANEUVER = (1U << 7),
  SPEC_EVENT_MOUNT_CHARGE = (1U << 8),
  SPEC_EVENT_MOVING_ROOM_RELOCATION = (1U << 9),
  SPEC_EVENT_MOBILE_DEATH = (1U << 10)
};

#define SPEC_EVENT_ALL                                                                             \
  (SPEC_EVENT_COMMAND | SPEC_EVENT_MOBILE_ACTIVITY | SPEC_EVENT_MOBILE_COMBAT_TURN |               \
   SPEC_EVENT_OBJECT_AUTO_PULSE | SPEC_EVENT_ITEM_IDENTIFY | SPEC_EVENT_WEAPON_HIT |               \
   SPEC_EVENT_DEFENSE_REACTION | SPEC_EVENT_COMBAT_MANEUVER | SPEC_EVENT_MOUNT_CHARGE |            \
   SPEC_EVENT_MOVING_ROOM_RELOCATION | SPEC_EVENT_MOBILE_DEATH)

typedef uint32_t spec_prototype_flag_mask;
enum spec_prototype_flag
{
  SPEC_PROTOTYPE_NONE = 0,
  SPEC_PROTOTYPE_MOB_SPEC = (1U << 0),
  SPEC_PROTOTYPE_ITEM_AUTOPROC = (1U << 1)
};

#define SPEC_PROTOTYPE_ALL (SPEC_PROTOTYPE_MOB_SPEC | SPEC_PROTOTYPE_ITEM_AUTOPROC)

typedef uint32_t spec_placement_mask;
enum spec_placement_flag
{
  SPEC_PLACEMENT_NONE = 0,
  SPEC_PLACEMENT_CARRIED = (1U << 0),
  SPEC_PLACEMENT_EQUIPPED = (1U << 1),
  SPEC_PLACEMENT_COMBAT = (1U << 2),
  SPEC_PLACEMENT_MOUNTED = (1U << 3),
  SPEC_PLACEMENT_MOVING_ROOM = (1U << 4)
};

#define SPEC_PLACEMENT_ALL                                                                         \
  (SPEC_PLACEMENT_CARRIED | SPEC_PLACEMENT_EQUIPPED | SPEC_PLACEMENT_COMBAT |                      \
   SPEC_PLACEMENT_MOUNTED | SPEC_PLACEMENT_MOVING_ROOM)

typedef uint32_t spec_binding_source_mask;
enum spec_binding_source_flag
{
  SPEC_BINDING_SOURCE_NONE = 0,
  SPEC_BINDING_SOURCE_WORLD = (1U << 0),
  SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT = (1U << 1),
  SPEC_BINDING_SOURCE_PARSER_HOOK = (1U << 2),
  SPEC_BINDING_SOURCE_SHOP = (1U << 3),
  SPEC_BINDING_SOURCE_QUEST = (1U << 4)
};

#define SPEC_BINDING_SOURCE_ALL                                                                    \
  (SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT |                             \
   SPEC_BINDING_SOURCE_PARSER_HOOK | SPEC_BINDING_SOURCE_SHOP | SPEC_BINDING_SOURCE_QUEST)

enum spec_builder_visibility
{
  SPEC_BUILDER_VISIBILITY_INVALID = 0,
  SPEC_BUILDER_HIDDEN,
  SPEC_BUILDER_VISIBLE
};

struct spec_event_contract
{
  /** Exactly one SPEC_EVENT_* bit. */
  spec_event_mask event;
  /** Prototype flags required for this event to be scheduled. */
  spec_prototype_flag_mask required_prototype_flags;
  /** Owner or actor placement required when this event is invoked. */
  spec_placement_mask required_placement;
};

/** Immutable identity, compatibility, and invocation metadata for one procedure. */
struct spec_definition
{
  const char *canonical_name;
  const char *display_name;
  const char *const *aliases;
  size_t alias_count;
  spec_owner_mask owner_mask;
  const struct spec_event_contract *events;
  size_t event_count;
  spec_binding_source_mask binding_source_mask;
  enum spec_builder_visibility builder_visibility;
  const char *category;
  const char *description;
  /** Complete legacy behavior, or NULL for a typed definition. */
  spec_legacy_handler legacy_handler;
  /**
   * Legacy-shaped callback-slot identity for a typed definition.
   *
   * Existing prototypes still store SPECIAL pointers. The event gateways
   * recognize this adapter and invoke typed_handler with the context captured
   * at the call site. The adapter must fail safely if called directly.
   */
  spec_legacy_handler typed_adapter;
  spec_typed_handler typed_handler;
};

/** Return the number of canonical definitions; aliases are not counted. */
size_t spec_registry_count(void);
/** Return a canonical definition by signed index, or NULL when out of range. */
const struct spec_definition *spec_registry_get(int index);
/** Resolve a canonical name or alias case-insensitively. */
const struct spec_definition *spec_registry_find_by_name(const char *name);
/** Resolve a name only when its definition supports exactly one requested owner type. */
const struct spec_definition *spec_registry_find_for_owner(const char *name, spec_owner_mask owner);
/** Reverse-resolve a callback-slot pointer to its first canonical definition. */
const struct spec_definition *spec_registry_find_by_handler(spec_legacy_handler handler);

/** Return the callback stored in legacy prototype slots for this definition. */
spec_legacy_handler spec_definition_callback(const struct spec_definition *definition);
/** Return the number of definitions still implemented as legacy callbacks. */
size_t spec_registry_legacy_count(void);
/** Return the number of definitions implemented as typed handlers. */
size_t spec_registry_typed_count(void);

/** Report whether a definition supports exactly one requested owner type. */
bool spec_definition_supports_owner(const struct spec_definition *definition,
                                    spec_owner_mask owner);
/** Return the contract for exactly one event bit, or NULL when unsupported or invalid. */
const struct spec_event_contract *
spec_definition_get_event(const struct spec_definition *definition, spec_event_mask event);
/** Report whether an owner-specific definition supports an event. */
bool spec_definition_supports_event(const struct spec_definition *definition, spec_owner_mask owner,
                                    spec_event_mask event);
/** Report whether a definition permits exactly one requested binding source. */
bool spec_definition_allows_binding(const struct spec_definition *definition,
                                    spec_binding_source_mask source);

/** Return a stable diagnostic name for exactly one owner bit. */
const char *spec_owner_name(spec_owner_mask owner);
/** Return a stable diagnostic name for exactly one event bit. */
const char *spec_event_name(spec_event_mask event);

/** Validate an explicit immutable definition table and write the first bounded diagnostic. */
bool spec_registry_validate_definitions(const struct spec_definition *definitions, size_t count,
                                        char *error, size_t error_size);
/** Validate the production registry and its legacy compatibility projection. */
bool spec_registry_validate(char *error, size_t error_size);
/** Fail process startup with a SYSERR diagnostic when production metadata is invalid. */
void spec_registry_boot_validate(void);

/** Legacy registry projection retained for OLC and persisted-name compatibility. */
const char *get_spec_func_name(spec_legacy_handler func);
int get_spec_func_count(void);
const char *get_spec_func_name_by_index(int idx);
spec_legacy_handler get_spec_func_by_index(int idx);
spec_legacy_handler find_spec_func_by_name(const char *name);

#endif /* LUMINARI_SPEC_REGISTRY_H */
