#include "domain_event_types.h"

enum domain_event_status domain_event_register_foundation_types(struct domain_event_bus *bus)
{
  static const struct domain_event_type_config types[] = {
      {DOMAIN_EVENT_CHARACTER_MOVED, "CharacterMoved", sizeof(struct domain_character_moved)},
      {DOMAIN_EVENT_CHARACTER_DAMAGED, "CharacterDamaged", sizeof(struct domain_character_damaged)},
      {DOMAIN_EVENT_CHARACTER_DIED, "CharacterDied", sizeof(struct domain_character_died)},
      {DOMAIN_EVENT_ENTITY_EXTRACTED, "EntityExtracted", sizeof(struct domain_entity_extracted)},
      {DOMAIN_EVENT_COMBAT_STATE_CHANGED, "CombatStateChanged",
       sizeof(struct domain_combat_state_changed)},
      {DOMAIN_EVENT_OBJECT_MOVED, "ObjectMoved", sizeof(struct domain_object_moved)},
      {DOMAIN_EVENT_DOOR_STATE_CHANGED, "DoorStateChanged",
       sizeof(struct domain_door_state_changed)},
      {DOMAIN_EVENT_ACTIVITY_TRANSITIONED, "ActivityTransitioned",
       sizeof(struct domain_activity_transitioned)},
      {DOMAIN_EVENT_WORLD_PHENOMENON, "WorldPhenomenon", sizeof(struct domain_world_phenomenon)},
      {DOMAIN_EVENT_CASTING_STARTED, "CastingStarted", sizeof(struct domain_casting_started)},
      {DOMAIN_EVENT_ATTACK_COMMITTED, "AttackCommitted", sizeof(struct domain_attack_committed)},
      {DOMAIN_EVENT_PHENOMENON_PERCEIVED, "PhenomenonPerceived",
       sizeof(struct domain_phenomenon_perceived)},
      {DOMAIN_EVENT_CHARACTER_RESOLVED, "CharacterResolved",
       sizeof(struct domain_character_resolved)},
      {DOMAIN_EVENT_SKILL_RESOLVED, "SkillResolved", sizeof(struct domain_skill_resolved)},
  };
  size_t index;

  if (bus == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  for (index = 0; index < sizeof(types) / sizeof(types[0]); index++)
  {
    enum domain_event_status status = domain_event_register_type(bus, &types[index]);

    if (status != DOMAIN_EVENT_OK)
      return status;
  }
  return DOMAIN_EVENT_OK;
}

const char *domain_world_phenomenon_kind_name(enum domain_world_phenomenon_kind kind)
{
  static const char *const names[] = {"unspecified", "magic approach", "magic impact", "alarm",
                                      "fire",        "smoke",          "magic trace"};

  if (kind < DOMAIN_PHENOMENON_UNSPECIFIED || kind >= NUM_DOMAIN_PHENOMENON_KINDS)
    return "unknown";
  return names[kind];
}
