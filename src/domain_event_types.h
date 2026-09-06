#ifndef DOMAIN_EVENT_TYPES_H
#define DOMAIN_EVENT_TYPES_H

#include "domain_events.h"

enum luminari_domain_event_type
{
  DOMAIN_EVENT_CHARACTER_MOVED = 0x1001,
  DOMAIN_EVENT_CHARACTER_DAMAGED = 0x1002,
  DOMAIN_EVENT_CHARACTER_DIED = 0x1003,
  DOMAIN_EVENT_ENTITY_EXTRACTED = 0x1004,
  DOMAIN_EVENT_COMBAT_STATE_CHANGED = 0x1005,
  DOMAIN_EVENT_OBJECT_MOVED = 0x1006,
  DOMAIN_EVENT_DOOR_STATE_CHANGED = 0x1007,
  DOMAIN_EVENT_ACTIVITY_TRANSITIONED = 0x1008,
  DOMAIN_EVENT_WORLD_PHENOMENON = 0x1009,
  DOMAIN_EVENT_CASTING_STARTED = 0x100a,
  DOMAIN_EVENT_ATTACK_COMMITTED = 0x100b
};

/* One legal normal strike has committed, before its combat consequences.
 * Misses and prevented strikes count; rejected commands do not. Consumers may
 * queue reactions, never execute combat recursively inside this publication. */
struct domain_attack_committed
{
  uint64_t attempt_id;
  struct domain_entity_handle attacker;
  struct domain_entity_handle defender;
  struct domain_entity_handle origin_room;
  int attack_kind;
};

enum domain_world_phenomenon_channel
{
  DOMAIN_WORLD_PHENOMENON_VISUAL = (1U << 0),
  DOMAIN_WORLD_PHENOMENON_AUDIBLE = (1U << 1)
};

enum domain_world_audio_frequency
{
  DOMAIN_WORLD_AUDIO_LOW = 0,
  DOMAIN_WORLD_AUDIO_MID,
  DOMAIN_WORLD_AUDIO_HIGH
};

enum domain_world_phenomenon_propagation
{
  DOMAIN_WORLD_PROPAGATE_COORDINATES = 0,
  DOMAIN_WORLD_PROPAGATE_ROOMS
};

enum domain_relocation_cause
{
  DOMAIN_RELOCATION_UNKNOWN = 0,
  DOMAIN_RELOCATION_WALK,
  DOMAIN_RELOCATION_TELEPORT,
  DOMAIN_RELOCATION_FORCED,
  DOMAIN_RELOCATION_SCRIPT,
  DOMAIN_RELOCATION_SPAWN,
  DOMAIN_RELOCATION_RESTORE,
  DOMAIN_RELOCATION_STAFF,
  DOMAIN_RELOCATION_TRANSPORT
};

struct domain_character_moved
{
  struct domain_entity_handle character;
  struct domain_entity_handle from_room;
  struct domain_entity_handle to_room;
  int direction;
  enum domain_relocation_cause cause;
  struct domain_entity_handle actor;
};

struct domain_character_damaged
{
  struct domain_entity_handle target;
  struct domain_entity_handle source;
  int amount;
  int damage_type;
};

struct domain_character_died
{
  struct domain_entity_handle character;
  struct domain_entity_handle killer;
  uint32_t cause;
};

struct domain_entity_extracted
{
  struct domain_entity_handle entity;
  uint32_t reason;
};

struct domain_combat_state_changed
{
  struct domain_entity_handle character;
  struct domain_entity_handle opponent;
  bool in_combat;
};

enum domain_holder_kind
{
  DOMAIN_HOLDER_NONE = 0,
  DOMAIN_HOLDER_ROOM,
  DOMAIN_HOLDER_INVENTORY,
  DOMAIN_HOLDER_EQUIPMENT,
  DOMAIN_HOLDER_CONTAINER,
  DOMAIN_HOLDER_BAG
};

struct domain_object_holder
{
  enum domain_holder_kind kind;
  struct domain_entity_handle entity;
  int slot; /* Equipment slot or character bag number; otherwise -1. */
};

enum domain_transfer_cause
{
  DOMAIN_TRANSFER_UNKNOWN = 0,
  DOMAIN_TRANSFER_COMMAND,
  DOMAIN_TRANSFER_SCRIPT,
  DOMAIN_TRANSFER_MAGIC,
  DOMAIN_TRANSFER_SHOP,
  DOMAIN_TRANSFER_RESET,
  DOMAIN_TRANSFER_RESTORE,
  DOMAIN_TRANSFER_EXTRACT
};

struct domain_object_moved
{
  uint64_t transfer_id; /* Process-local committed operation identity. */
  struct domain_entity_handle object;
  struct domain_entity_handle from_owner;
  struct domain_entity_handle to_owner;
  struct domain_object_holder source;
  struct domain_object_holder destination;
  struct domain_entity_handle actor;
  enum domain_transfer_cause cause;
};

enum domain_door_change_cause
{
  DOMAIN_DOOR_GAMEPLAY = 0,
  DOMAIN_DOOR_RESET,
  DOMAIN_DOOR_EDIT
};

struct domain_door_state_changed
{
  uint64_t exit_identity;
  enum domain_door_change_cause cause;
  struct domain_entity_handle room;
  int direction;
  uint32_t previous_state;
  uint32_t current_state;
};

struct domain_activity_transitioned
{
  uint64_t activity_id;
  uint32_t end_reason;
  struct domain_entity_handle actor;
  uint32_t activity_type;
  uint32_t previous_state;
  uint32_t current_state;
};

/* A committed timed cast. Consumers must revalidate cast_id before reacting.
 * This fact does not identify the spell to an observer or authorize an attack.
 * Handlers enqueue reactions; they must not execute combat inside publication. */
struct domain_casting_started
{
  uint64_t cast_id;
  struct domain_entity_handle caster;
  struct domain_entity_handle target;
  struct domain_entity_handle room;
  int spellnum;
  int casting_class;
};

/* Descriptions are borrowed for synchronous dispatch and are never retained. */
struct domain_world_phenomenon
{
  struct domain_entity_handle source_room;
  int source_x;
  int source_y;
  int source_z;
  int visual_range;
  int audio_range;
  int minimum_range;
  float intensity;
  uint32_t channels;
  uint32_t propagation;
  int audio_frequency;
  const char *visual_description;
  const char *audio_description;
};

enum domain_event_status domain_event_register_foundation_types(struct domain_event_bus *bus);

#define DOMAIN_EVENT_PUBLISH(BUS, TYPE, PAYLOAD_PTR)                                               \
  domain_event_publish((BUS), (TYPE), (PAYLOAD_PTR), sizeof(*(PAYLOAD_PTR)))

#endif /* DOMAIN_EVENT_TYPES_H */
