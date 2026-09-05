#ifndef AFFECTED_OWNERS_H
#define AFFECTED_OWNERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct char_data;
struct raff_node;

void affected_owners_init(void);
void affected_owners_shutdown(void);

bool affected_owner_events_enabled(void);
void affected_character_owner_sync(struct char_data *ch);
void affected_character_owner_forget(struct char_data *ch);
void affected_character_owner_refill(void);
void affected_room_owner_add(struct raff_node *raff);
void affected_room_owner_remove(struct raff_node *raff);
void affected_room_owners_remove_room(uint32_t room_index);
void affected_room_owners_prepare_world_reindex(void);
void affected_room_owners_finish_world_reindex(uint32_t pivot, bool inserted);

size_t affected_character_scheduled_count(void);
size_t affected_room_owner_count(void);
size_t affected_room_scheduled_count(void);
size_t affected_character_admission_limit(void);
size_t affected_room_admission_limit(void);
uint64_t affected_owner_admission_rejections(void);
uint64_t affected_character_callbacks(void);
uint64_t affected_room_callbacks(void);
uint64_t affected_character_nodes_processed(void);
uint64_t affected_room_nodes_processed(void);
uint64_t affected_room_behavior_executions(void);
uint64_t affected_room_behavior_nodes_processed(void);
size_t affected_room_registry_validate(void);

void affected_owners_reset_telemetry(void);

#ifdef LUMINARI_CUTEST
void affected_owners_reset_for_test(void);
void affected_owners_select_for_test(bool scheduled);
void affected_owners_set_limits_for_test(size_t character_limit, size_t room_limit);
#endif

#endif /* AFFECTED_OWNERS_H */
