#include "conf.h"
#include "ready_action.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "modify.h"
#include "domain_event_runtime.h"
#include "domain_event_world.h"
#include "domain_events.h"
#include "event_debug.h"
#include "event_runtime.h"
#include "combat/combat_encounters.h"
#include "active_world.h"
#include "activity_manager.h"
#include "ai_service.h"
#include "character_periodic.h"
#include "dgscript/dg_scripts.h"
#include "movement/movement_tracks.h"
#include "mob/mob_act.h"
#include "net/i3_client.h"
#include "perfmon.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>

#define EVENT_DEBUG_DEFAULT_WIDTH 80
#define EVENT_DEBUG_MIN_WIDTH 40
#define EVENT_DEBUG_MAX_WIDTH 120
#define EVENT_DEBUG_DEFAULT_LIMIT 20U
#define EVENT_DEBUG_MAX_LIMIT 100U
#define EVENT_DEBUG_DOMAIN_TYPE_LIMIT 128U
#define EVENT_DEBUG_DOMAIN_HANDLER_LIMIT 128U

struct event_debug_output
{
  char *buffer;
  size_t capacity;
  size_t length;
  size_t width;
};

static const char *event_debug_mode(bool scheduled)
{
  if (scheduled)
    return "scheduled";
  return "unavailable";
}

int event_debug_effective_width(int configured_width)
{
  if (configured_width < EVENT_DEBUG_MIN_WIDTH)
    return EVENT_DEBUG_DEFAULT_WIDTH;
  return MIN(configured_width, EVENT_DEBUG_MAX_WIDTH);
}

static void debug_output_init(struct event_debug_output *output, char *buffer, size_t capacity,
                              int width)
{
  memset(output, 0, sizeof(*output));
  output->buffer = buffer;
  output->capacity = capacity;
  output->width = (size_t)event_debug_effective_width(width);
  if (buffer != NULL && capacity > 0)
    buffer[0] = '\0';
}

static void debug_output_line(struct event_debug_output *output, const char *format, ...)
{
  char line[512];
  size_t line_length;
  size_t available;
  va_list arguments;

  if (output == NULL || output->buffer == NULL || output->capacity == 0 ||
      output->length >= output->capacity - 1)
    return;
  va_start(arguments, format);
  vsnprintf(line, sizeof(line), format, arguments);
  va_end(arguments);
  line[sizeof(line) - 1] = '\0';
  line_length = strlen(line);
  if (line_length > output->width)
  {
    line_length = output->width;
    line[line_length] = '\0';
    if (line_length > 0)
      line[line_length - 1] = '~';
  }
  available = output->capacity - output->length;
  output->length += (size_t)snprintf(output->buffer + output->length, available, "%s\r\n", line);
  if (output->length >= output->capacity)
  {
    output->length = output->capacity - 1;
    output->buffer[output->length] = '\0';
  }
}

static void debug_output_rule(struct event_debug_output *output)
{
  char rule[EVENT_DEBUG_MAX_WIDTH + 1];

  memset(rule, '-', output->width);
  rule[output->width] = '\0';
  debug_output_line(output, "%s", rule);
}

static void debug_output_title(struct event_debug_output *output, const char *title)
{
  debug_output_line(output, "%s", title);
  debug_output_rule(output);
}

static bool parse_uint64(const char *text, uint64_t *value)
{
  unsigned long long parsed;
  char *end;

  if (text == NULL || *text == '\0' || value == NULL || *text == '-')
    return false;
  errno = 0;
  end = NULL;
  parsed = strtoull(text, &end, 0);
  if (errno != 0 || end == text || *end != '\0')
    return false;
  *value = (uint64_t)parsed;
  return true;
}

static size_t parse_limit(const char *text, size_t fallback)
{
  uint64_t parsed;

  if (text == NULL || *text == '\0')
    return fallback;
  if (!parse_uint64(text, &parsed) || parsed == 0)
    return 0;
  return (size_t)MIN(parsed, EVENT_DEBUG_MAX_LIMIT);
}

size_t event_debug_render_help(char *buffer, size_t capacity, int width)
{
  struct event_debug_output output;

  debug_output_init(&output, buffer, capacity, width);
  debug_output_title(&output, "Event Debug Commands");
  debug_output_line(&output, "eventdebug");
  debug_output_line(&output, "eventdebug queue [limit]");
  debug_output_line(&output, "eventdebug id <id>");
  debug_output_line(&output, "eventdebug type <text> [limit]");
  debug_output_line(&output, "eventdebug owner <kind> <id> [gen]");
  debug_output_line(&output, "eventdebug player <name> [limit]");
  debug_output_line(&output, "eventdebug mob <name> [limit]");
  debug_output_line(&output, "eventdebug object <name> [limit]");
  debug_output_line(&output, "eventdebug room <here|vnum> [limit]");
  debug_output_line(&output, "eventdebug scripts <kind> <target> [limit]");
  debug_output_line(&output, "  [limit]; kinds: player mob object room");
  debug_output_line(&output, "eventdebug due <max-pulses> [limit]");
  debug_output_line(&output, "eventdebug range <min> <max> [limit]");
  debug_output_line(&output, "eventdebug state <state> [limit]");
  debug_output_line(&output, "eventdebug types [limit]");
  debug_output_line(&output, "eventdebug domain [type]");
  debug_output_line(&output, "eventdebug ready [reset]");
  debug_output_line(&output, "eventdebug subscriptions [limit]");
  debug_output_line(&output, "eventdebug subscriptions <kind> <target> [limit]");
  debug_output_line(&output, "eventdebug help");
  debug_output_line(&output, "");
  debug_output_line(&output, "Owner kinds:");
  debug_output_line(&output, " world descriptor character room");
  debug_output_line(&output, " region object zone encounter vessel");
  debug_output_line(&output, " service");
  debug_output_line(&output, "States: queued ready running cancel-pending");
  debug_output_line(&output, "Payloads are always redacted.");
  return output.length;
}

static void render_live_owner_counts(struct event_debug_output *output,
                                     const struct event_debug_stats *stats)
{
  enum game_event_owner_kind kind;

  debug_output_line(output, "Live events by owner");
  for (kind = GAME_EVENT_OWNER_NONE; kind < GAME_EVENT_OWNER_KIND_COUNT; kind++)
    if (stats->owner_event_counts[kind] > 0)
      debug_output_line(output, "  %s: %zu", event_debug_owner_kind_name(kind),
                        stats->owner_event_counts[kind]);
}

size_t event_debug_render_summary(char *buffer, size_t capacity, int width)
{
  struct event_debug_output output;
  struct event_debug_stats event_stats;
  struct domain_event_bus_stats domain_stats;
  struct i3_ingress_stats ingress_stats;
  struct ai_event_ingress_stats ai_ingress_stats;
  struct combat_encounter_stats encounter_stats;
  struct primary_activity_stats activity_stats;
  struct runtime_service_stats service_stats;
  struct domain_event_bus *bus;

  debug_output_init(&output, buffer, capacity, width);
  event_debug_get_stats(&event_stats);
  debug_output_title(&output, "Event Debug Summary");
  debug_output_line(&output, "Backend: %s", event_backend_name());
  debug_output_line(&output, "Pulse: %" PRIu64, event_stats.current_pulse);
  debug_output_line(&output, "Display width: %zu", output.width);
  debug_output_line(&output, "Live events: %zu", event_stats.live_events);
  debug_output_line(&output, "High-water events: %zu", event_stats.high_water_events);
  debug_output_line(&output, "Registry mismatch: %zu", event_stats.registry_mismatches);
  debug_output_line(&output, "Stale-owner outcomes: %" PRIu64, event_stats.stale_owner_outcomes);
  render_live_owner_counts(&output, &event_stats);
  memset(&service_stats, 0, sizeof(service_stats));
  runtime_services_get_stats(&service_stats);
  debug_output_line(&output, "");
  debug_output_line(&output, "Runtime services");
  debug_output_line(&output, "  mode: %s",
                    service_stats.scheduled ? "named scheduled events" : event_debug_mode(false));
  debug_output_line(&output, "  live/configured: %zu/%zu", service_stats.live_services,
                    service_stats.configured_services);
  debug_output_line(&output, "  callbacks: %" PRIu64, service_stats.callbacks);
  debug_output_line(&output, "  schedule failures: %" PRIu64, service_stats.schedule_failures);
  if (event_stats.scheduler_stats_available)
  {
    debug_output_line(&output, "");
    debug_output_line(&output, "Scheduler queues");
    debug_output_line(&output, "  types: %zu (%s)", event_stats.scheduler.registered_type_count,
                      event_runtime_types_are_sealed() ? "sealed" : "open");
    debug_output_line(&output, "  ready: %zu", event_stats.scheduler.ready_count);
    debug_output_line(&output, "  oldest overdue: %" PRIu64 " pulses",
                      event_stats.scheduler.oldest_overdue_ticks);
    debug_output_line(&output, "  wheel L0: %zu", event_stats.scheduler.wheel_level_counts[0]);
    debug_output_line(&output, "  wheel L1: %zu", event_stats.scheduler.wheel_level_counts[1]);
    debug_output_line(&output, "  wheel L2: %zu", event_stats.scheduler.wheel_level_counts[2]);
    debug_output_line(&output, "  wheel L3: %zu", event_stats.scheduler.wheel_level_counts[3]);
    debug_output_line(&output, "  wheel L4: %zu", event_stats.scheduler.wheel_level_counts[4]);
    debug_output_line(&output, "  overflow: %zu", event_stats.scheduler.overflow_count);
    debug_output_line(&output, "  owner records: %zu", event_stats.scheduler.owner_count);
    debug_output_line(&output, "  timed ingress: main thread only");
    debug_output_line(&output, "");
    debug_output_line(&output, "Scheduler lifecycle");
    debug_output_line(&output, "  scheduled: %" PRIu64, event_stats.scheduler.total_scheduled);
    debug_output_line(&output, "  callbacks: %" PRIu64, event_stats.scheduler.total_callbacks);
    debug_output_line(&output, "  completed: %" PRIu64, event_stats.scheduler.total_completed);
    debug_output_line(&output, "  cancelled: %" PRIu64, event_stats.scheduler.total_cancelled);
    debug_output_line(&output, "  failed: %" PRIu64, event_stats.scheduler.total_failed);
    debug_output_line(&output, "  rescheduled: %" PRIu64, event_stats.scheduler.total_rescheduled);
    debug_output_line(&output, "  late callbacks: %" PRIu64,
                      event_stats.scheduler.total_late_callbacks);
    debug_output_line(&output, "  missed: %" PRIu64,
                      event_stats.scheduler.total_missed_occurrences);
    debug_output_line(&output, "  skipped: %" PRIu64,
                      event_stats.scheduler.total_skipped_occurrences);
    debug_output_line(&output, "  coalesced: %" PRIu64,
                      event_stats.scheduler.total_coalesced_occurrences);
    debug_output_line(&output, "Scheduler structural work");
    debug_output_line(&output, "  cascade slots: %" PRIu64,
                      event_stats.scheduler.total_cascade_slots);
    debug_output_line(&output, "  cascaded: %" PRIu64, event_stats.scheduler.total_cascaded_events);
    debug_output_line(&output, "  largest cascade: %" PRIu64,
                      event_stats.scheduler.largest_cascade);
    debug_output_line(&output, "  overflow promotions: %" PRIu64,
                      event_stats.scheduler.total_overflow_promotions);
    debug_output_line(&output, "  large advances: %" PRIu64,
                      event_stats.scheduler.total_large_advances);
    debug_output_line(&output, "  events reclassified: %" PRIu64,
                      event_stats.scheduler.total_large_advance_events);
    debug_output_line(&output, "Admission rejections");
    debug_output_line(&output, "  global limit: %" PRIu64,
                      event_stats.scheduler.total_capacity_rejections);
    debug_output_line(&output, "  type limit: %" PRIu64,
                      event_stats.scheduler.total_type_capacity_rejections);
    debug_output_line(&output, "  invalid owner: %" PRIu64,
                      event_stats.scheduler.total_invalid_owner_rejections);
    debug_output_line(&output, "  owner limit: %" PRIu64,
                      event_stats.scheduler.total_owner_capacity_rejections);
    debug_output_line(&output, "  owner/type limit: %" PRIu64,
                      event_stats.scheduler.total_owner_type_capacity_rejections);
  }
  else
    debug_output_line(&output, "Legacy storage metrics unavailable.");
  memset(&encounter_stats, 0, sizeof(encounter_stats));
  combat_encounter_get_stats(&encounter_stats);
  debug_output_line(&output, "");
  debug_output_line(&output, "Combat encounters");
  debug_output_line(&output, "  mode: %s",
                    !encounter_stats.encounter_mode   ? "character rollback"
                    : encounter_stats.semantic_rounds ? "six-second semantic"
                                                      : "compatibility phases");
  debug_output_line(&output, "  active: %zu encounters / %zu participants",
                    encounter_stats.active_encounters, encounter_stats.active_participants);
  debug_output_line(&output, "  scheduled events: %zu", encounter_stats.scheduled_events);
  debug_output_line(&output, "  created/ended/merged: %" PRIu64 "/%" PRIu64 "/%" PRIu64,
                    encounter_stats.encounters_created, encounter_stats.encounters_ended,
                    encounter_stats.encounters_merged);
  debug_output_line(&output, "  callbacks: %" PRIu64, encounter_stats.encounter_callbacks);
  if (encounter_stats.semantic_rounds)
  {
    debug_output_line(&output, "  semantic rounds/turns: %" PRIu64 "/%" PRIu64,
                      encounter_stats.semantic_rounds_resolved,
                      encounter_stats.semantic_turns_resolved);
    debug_output_line(&output, "  intents sent/held: %" PRIu64 "/%" PRIu64,
                      encounter_stats.intents_dispatched, encounter_stats.intent_dispatch_blocks);
    debug_output_line(&output, "  action/reaction spend: %" PRIu64 "/%" PRIu64,
                      encounter_stats.action_budgets_spent, encounter_stats.reactions_spent);
  }
  else
  {
    debug_output_line(&output, "  phases/terminal: %" PRIu64 "/%" PRIu64,
                      encounter_stats.compatibility_phases, encounter_stats.compatibility_terminal);
    debug_output_line(&output, "  compatibility attempts: %" PRIu64,
                      encounter_stats.compatibility_attempts);
  }
  debug_output_line(&output, "  comparison mismatch: %" PRIu64,
                    encounter_stats.compatibility_mismatches);
  debug_output_line(&output, "  admission/stale: %" PRIu64 "/%" PRIu64,
                    encounter_stats.admission_failures, encounter_stats.stale_encounter_callbacks);
  memset(&activity_stats, 0, sizeof(activity_stats));
  primary_activity_get_stats(&activity_stats);
  debug_output_line(&output, "");
  debug_output_line(&output, "Primary activities");
  debug_output_line(&output, "  active/high-water: %zu/%zu", activity_stats.active,
                    activity_stats.high_water);
  debug_output_line(&output, "  started/completed: %" PRIu64 "/%" PRIu64, activity_stats.started,
                    activity_stats.completed);
  debug_output_line(&output, "  cancelled/paused/resumed: %" PRIu64 "/%" PRIu64 "/%" PRIu64,
                    activity_stats.cancelled, activity_stats.paused, activity_stats.resumed);
  debug_output_line(&output, "  delayed/rejected: %" PRIu64 "/%" PRIu64, activity_stats.delayed,
                    activity_stats.rejected_commands);
  debug_output_line(&output, "  stale callbacks: %" PRIu64, activity_stats.stale_callbacks);
  debug_output_line(&output, "");
  debug_output_line(&output, "Character owners");
  debug_output_line(&output, "  mode: %s", event_debug_mode(character_periodic_events_enabled()));
  debug_output_line(&output, "  members/scheduled/mismatch: %zu/%zu/%zu",
                    character_periodic_owner_count(), character_periodic_scheduled_count(),
                    character_periodic_registry_validate());
  debug_output_line(&output, "  owner callbacks: %" PRIu64, character_periodic_callbacks());
  debug_output_line(&output, "  d20/devices/quests: %" PRIu64 "/%" PRIu64 "/%" PRIu64,
                    character_periodic_d20_round_executions(),
                    character_periodic_device_executions(),
                    character_periodic_timed_quest_executions());
  debug_output_line(&output, "");
  debug_output_line(&output, "Autonomous mobile agendas");
  debug_output_line(&output, "  mode: %s", event_debug_mode(active_world_enabled()));
  debug_output_line(&output, "  active/cooling: %zu/%zu",
                    active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE),
                    active_world_mobile_count(ACTIVE_WORLD_MOBILE_COOLING));
  debug_output_line(&output, "  spec/echo/scavenge: %zu/%zu/%zu",
                    active_world_mobile_reason_count(MOBILE_WORK_SPEC_ACTIVITY),
                    active_world_mobile_reason_count(MOBILE_WORK_ECHO),
                    active_world_mobile_reason_count(MOBILE_WORK_SCAVENGE));
  debug_output_line(&output, "  patrol/hunt/wander: %zu/%zu/%zu",
                    active_world_mobile_reason_count(MOBILE_WORK_PATROL),
                    active_world_mobile_reason_count(MOBILE_WORK_HUNT),
                    active_world_mobile_reason_count(MOBILE_WORK_WANDER));
  debug_output_line(&output, "  posture/room/combat: %zu/%zu/%zu",
                    active_world_mobile_reason_count(MOBILE_WORK_POSTURE),
                    active_world_mobile_reason_count(MOBILE_WORK_ROOM_REACTION),
                    active_world_mobile_reason_count(MOBILE_WORK_COMBAT_REACTION));
  debug_output_line(&output, "  resource recovery: %zu",
                    active_world_mobile_reason_count(MOBILE_WORK_RESOURCE_RECOVERY));
  debug_output_line(&output, "  agenda callbacks: %" PRIu64, active_world_mobile_callbacks());
  debug_output_line(&output, "  capacity/rejected: %zu/%" PRIu64,
                    active_world_mobile_admission_limit(),
                    active_world_mobile_admission_rejections());
  debug_output_line(&output, "");
  debug_output_line(&output, "Discovery registries");
  debug_output_line(&output, "  DG time members m/o/r: %zu/%zu/%zu",
                    dg_time_registry_count(MOB_TRIGGER), dg_time_registry_count(OBJ_TRIGGER),
                    dg_time_registry_count(WLD_TRIGGER));
  debug_output_line(&output, "  DG time mismatch m/o/r: %zu/%zu/%zu",
                    dg_time_registry_validate(MOB_TRIGGER), dg_time_registry_validate(OBJ_TRIGGER),
                    dg_time_registry_validate(WLD_TRIGGER));
  debug_output_line(&output, "  DG time visited m/o/r: %" PRIu64 "/%" PRIu64 "/%" PRIu64,
                    dg_time_registry_visited(MOB_TRIGGER), dg_time_registry_visited(OBJ_TRIGGER),
                    dg_time_registry_visited(WLD_TRIGGER));
  debug_output_line(&output, "  DG time executed m/o/r: %" PRIu64 "/%" PRIu64 "/%" PRIu64,
                    dg_time_registry_executed(MOB_TRIGGER), dg_time_registry_executed(OBJ_TRIGGER),
                    dg_time_registry_executed(WLD_TRIGGER));
  debug_output_line(&output, "  trail locations/mismatch: %zu/%zu",
                    movement_trail_active_location_count(), movement_trail_registry_validate());
  debug_output_line(&output,
                    "  trail cleanup runs/visited/removed: %" PRIu64 "/%" PRIu64 "/%" PRIu64,
                    movement_trail_cleanup_runs(), movement_trail_locations_visited(),
                    movement_trail_entries_removed());
  memset(&ingress_stats, 0, sizeof(ingress_stats));
  i3_get_ingress_stats(&ingress_stats);
  debug_output_line(&output, "");
  debug_output_line(&output, "Worker ingress (I3)");
  debug_output_line(&output, "  status: %s", ingress_stats.available ? "online" : "offline");
  debug_output_line(&output, "  depth: %zu/%zu", ingress_stats.depth, ingress_stats.capacity);
  debug_output_line(&output, "  high-water: %" PRIu64, ingress_stats.high_water);
  debug_output_line(&output, "  rejected: %" PRIu64, ingress_stats.rejections);
  debug_output_line(&output, "  wake failures: %" PRIu64, ingress_stats.wake_failures);
  memset(&ai_ingress_stats, 0, sizeof(ai_ingress_stats));
  ai_events_get_ingress_stats(&ai_ingress_stats);
  debug_output_line(&output, "");
  debug_output_line(&output, "Worker ingress (AI)");
  debug_output_line(&output, "  status: %s", ai_ingress_stats.available ? "online" : "offline");
  debug_output_line(&output, "  depth: %zu/%zu", ai_ingress_stats.depth, ai_ingress_stats.capacity);
  debug_output_line(&output, "  high-water: %" PRIu64, ai_ingress_stats.high_water);
  debug_output_line(&output, "  accepted/processed: %" PRIu64 "/%" PRIu64,
                    ai_ingress_stats.accepted, ai_ingress_stats.processed);
  debug_output_line(&output, "  rejected: %" PRIu64, ai_ingress_stats.rejected);
  debug_output_line(&output, "  wake/schedule failures: %" PRIu64 "/%" PRIu64,
                    ai_ingress_stats.wake_failures, ai_ingress_stats.schedule_failures);
  debug_output_line(&output, "");
  bus = domain_event_runtime_bus();
  memset(&domain_stats, 0, sizeof(domain_stats));
  domain_event_bus_get_stats(bus, &domain_stats);
  debug_output_line(&output, "Domain event bus");
  debug_output_line(&output, "  status: %s", bus != NULL ? "online" : "offline");
  debug_output_line(&output, "  sealed: %s", domain_stats.sealed ? "yes" : "no");
  debug_output_line(&output, "  types: %zu", domain_stats.registered_type_count);
  debug_output_line(&output, "  handlers: %zu", domain_stats.registered_handler_count);
  debug_output_line(&output, "  subscriptions live/high: %zu/%zu",
                    domain_stats.live_subscription_count, domain_stats.subscription_high_water);
  debug_output_line(&output, "  publications: %" PRIu64, domain_stats.publications);
  debug_output_line(&output, "  handler calls: %" PRIu64, domain_stats.handler_calls);
  debug_output_line(&output, "  sub deliveries/cancels: %" PRIu64 "/%" PRIu64,
                    domain_stats.subscription_deliveries, domain_stats.subscription_cancellations);
  debug_output_line(&output, "  rejected chains: %" PRIu64, domain_stats.rejected_causal_chains);
  debug_output_line(&output, "  max depth: %u", domain_stats.maximum_depth);
  debug_output_line(&output, "");
  debug_output_line(&output, "Use 'eventdebug help' for filters.");
  return output.length;
}

static void render_event_snapshot(struct event_debug_output *output,
                                  const struct event_debug_snapshot *snapshot)
{
  debug_output_line(output, "#%" PRIu64 " %s", snapshot->event_id, snapshot->type_name);
  debug_output_line(output, "  state: %s", event_debug_state_name(snapshot->state));
  debug_output_line(output, "  due pulses: %" PRIu64, snapshot->remaining_pulses);
  debug_output_line(output, "  due seconds: %.1f",
                    (double)snapshot->remaining_pulses / (double)PERF_pulse_per_second);
  debug_output_line(output, "  owner: %s", event_debug_owner_kind_name(snapshot->owner.kind));
  if (!game_event_owner_is_none(snapshot->owner))
  {
    debug_output_line(output, "  owner id: %" PRIu64, snapshot->owner.runtime_id);
    debug_output_line(output, "  generation: %" PRIu64, snapshot->owner.generation);
  }
}

size_t event_debug_render_queue(char *buffer, size_t capacity, int width,
                                const struct event_debug_filter *filter, size_t limit)
{
  struct event_debug_output output;
  struct event_debug_snapshot snapshots[EVENT_DEBUG_MAX_LIMIT];
  size_t returned;
  size_t matched;
  size_t index;

  limit = MIN(MAX(limit, 1U), EVENT_DEBUG_MAX_LIMIT);
  memset(snapshots, 0, sizeof(snapshots));
  returned = 0;
  matched = event_debug_inspect(filter, snapshots, limit, &returned);
  debug_output_init(&output, buffer, capacity, width);
  debug_output_title(&output, "Event Queue");
  debug_output_line(&output, "Matched: %zu", matched);
  debug_output_line(&output, "Showing: %zu", returned);
  debug_output_line(&output, "Payloads: redacted");
  for (index = 0; index < returned; index++)
  {
    debug_output_line(&output, "");
    render_event_snapshot(&output, &snapshots[index]);
  }
  if (matched > returned)
  {
    debug_output_line(&output, "");
    debug_output_line(&output, "%zu more event(s) matched.", matched - returned);
  }
  return output.length;
}

size_t event_debug_render_profiles(char *buffer, size_t capacity, int width, size_t limit)
{
  struct event_debug_output output;
  struct PERF_event_profile_snapshot snapshots[EVENT_DEBUG_MAX_LIMIT];
  size_t total;
  size_t shown;
  size_t index;
  struct event_debug_filter filter;
  game_event_type_id_t event_type;
  size_t live;

  limit = MIN(MAX(limit, 1U), EVENT_DEBUG_MAX_LIMIT);
  memset(snapshots, 0, sizeof(snapshots));
  total = PERF_get_event_profiles(snapshots, limit);
  shown = MIN(total, limit);
  debug_output_init(&output, buffer, capacity, width);
  debug_output_title(&output, "Event Callback Types");
  debug_output_line(&output, "Registered: %zu", total);
  debug_output_line(&output, "Showing: %zu", shown);
  debug_output_line(&output, "Sorted by total callback time.");
  for (index = 0; index < shown; index++)
  {
    if (event_runtime_find_type(snapshots[index].identity, &event_type) != GAME_SCHEDULER_OK ||
        event_runtime_type_live_count(event_type, &live) != GAME_SCHEDULER_OK)
    {
      memset(&filter, 0, sizeof(filter));
      filter.type_equals = snapshots[index].identity;
      live = event_debug_inspect(&filter, NULL, 0, NULL);
    }
    debug_output_line(&output, "");
    debug_output_line(&output, "%s", snapshots[index].identity);
    debug_output_line(&output, "  live: %zu", live);
    debug_output_line(&output, "  calls: %" PRIu64, snapshots[index].calls);
    debug_output_line(&output, "  total usec: %" PRIu64, snapshots[index].total_usec);
    debug_output_line(&output, "  max usec: %" PRIu64, snapshots[index].maximum_usec);
    debug_output_line(
        &output, "  lateness ticks p50/p95/p99/max: %" PRIu64 "/%" PRIu64 "/%" PRIu64 "/%" PRIu64,
        snapshots[index].lateness_p50_ticks, snapshots[index].lateness_p95_ticks,
        snapshots[index].lateness_p99_ticks, snapshots[index].lateness_maximum_ticks);
    debug_output_line(&output, "  lateness samples/seen/late: %zu/%" PRIu64 "/%" PRIu64,
                      snapshots[index].lateness_samples, snapshots[index].lateness_samples_seen,
                      snapshots[index].late_callbacks);
    debug_output_line(&output, "  scheduled: %" PRIu64, snapshots[index].scheduled);
    debug_output_line(&output, "  cancelled: %" PRIu64, snapshots[index].cancelled);
    debug_output_line(&output, "  recurrences: %" PRIu64, snapshots[index].rescheduled);
  }
  return output.length;
}

static bool domain_type_matches(const struct domain_event_type_stats *stats, const char *filter)
{
  uint64_t parsed;

  if (filter == NULL || *filter == '\0')
    return true;
  if (parse_uint64(filter, &parsed) && stats->type == (domain_event_type_id_t)parsed)
    return true;
  return strcasestr(stats->name, filter) != NULL;
}

size_t event_debug_render_domain(char *buffer, size_t capacity, int width, const char *type_filter)
{
  struct event_debug_output output;
  struct domain_event_bus *bus;
  struct domain_event_bus_stats bus_stats;
  struct domain_event_type_stats types[EVENT_DEBUG_DOMAIN_TYPE_LIMIT];
  struct domain_event_handler_stats handlers[EVENT_DEBUG_DOMAIN_HANDLER_LIMIT];
  size_t total_types;
  size_t type_count;
  size_t handler_count;
  size_t handler_shown;
  size_t type_index;
  size_t handler_index;
  size_t matched;

  debug_output_init(&output, buffer, capacity, width);
  debug_output_title(&output, "Domain Events");
  bus = domain_event_runtime_bus();
  if (bus == NULL)
  {
    debug_output_line(&output, "Domain event bus is offline.");
    return output.length;
  }
  memset(&bus_stats, 0, sizeof(bus_stats));
  domain_event_bus_get_stats(bus, &bus_stats);
  debug_output_line(&output, "Types: %zu", bus_stats.registered_type_count);
  debug_output_line(&output, "Handlers: %zu", bus_stats.registered_handler_count);
  debug_output_line(&output, "Subscriptions: %zu live / %zu high",
                    bus_stats.live_subscription_count, bus_stats.subscription_high_water);
  debug_output_line(&output, "Publications: %" PRIu64, bus_stats.publications);
  debug_output_line(&output, "Rejected chains: %" PRIu64, bus_stats.rejected_causal_chains);
  memset(types, 0, sizeof(types));
  total_types = domain_event_inspect_types(bus, types, EVENT_DEBUG_DOMAIN_TYPE_LIMIT);
  type_count = MIN(total_types, EVENT_DEBUG_DOMAIN_TYPE_LIMIT);
  matched = 0;
  for (type_index = 0; type_index < type_count; type_index++)
  {
    if (!domain_type_matches(&types[type_index], type_filter))
      continue;
    matched++;
    debug_output_line(&output, "");
    debug_output_line(&output, "0x%08" PRIx32 " %s", types[type_index].type,
                      types[type_index].name);
    debug_output_line(&output, "  payload bytes: %zu", types[type_index].payload_size);
    debug_output_line(&output, "  publications: %" PRIu64, types[type_index].publications);
    debug_output_line(&output, "  rejected: %" PRIu64, types[type_index].rejected_publications);
    debug_output_line(&output, "  handler calls: %" PRIu64, types[type_index].handler_calls);
    debug_output_line(&output, "  live subscriptions: %zu",
                      types[type_index].live_subscription_count);
    debug_output_line(&output, "  total usec: %" PRIu64, types[type_index].total_handler_usec);
    debug_output_line(&output, "  max usec: %" PRIu64, types[type_index].maximum_handler_usec);
    debug_output_line(&output, "  slow calls: %" PRIu64, types[type_index].slow_handler_calls);
    debug_output_line(&output, "  max depth: %u", types[type_index].maximum_depth);
    memset(handlers, 0, sizeof(handlers));
    handler_count = domain_event_inspect_handlers(bus, types[type_index].type, handlers,
                                                  EVENT_DEBUG_DOMAIN_HANDLER_LIMIT);
    handler_shown = handler_count < EVENT_DEBUG_DOMAIN_HANDLER_LIMIT
                        ? handler_count
                        : EVENT_DEBUG_DOMAIN_HANDLER_LIMIT;
    for (handler_index = 0; handler_index < handler_shown; handler_index++)
    {
      debug_output_line(&output, "  handler: %s", handlers[handler_index].identity);
      debug_output_line(&output, "    priority: %d", handlers[handler_index].priority);
      debug_output_line(&output, "    calls: %" PRIu64, handlers[handler_index].calls);
      debug_output_line(&output, "    total usec: %" PRIu64, handlers[handler_index].total_usec);
      debug_output_line(&output, "    max usec: %" PRIu64, handlers[handler_index].maximum_usec);
      debug_output_line(&output, "    slow calls: %" PRIu64, handlers[handler_index].slow_calls);
    }
  }
  if (matched == 0)
    debug_output_line(&output, "No domain event type matched '%s'.",
                      type_filter != NULL ? type_filter : "");
  if (total_types > type_count)
    debug_output_line(&output, "%zu type(s) omitted by the safety bound.",
                      total_types - type_count);
  return output.length;
}

static const char *domain_topic_role_name(enum domain_event_topic_role role)
{
  switch (role)
  {
  case DOMAIN_EVENT_TOPIC_ANY:
    return "any";
  case DOMAIN_EVENT_TOPIC_SUBJECT:
    return "subject";
  case DOMAIN_EVENT_TOPIC_SOURCE:
    return "source";
  case DOMAIN_EVENT_TOPIC_DESTINATION:
    return "destination";
  case DOMAIN_EVENT_TOPIC_LOCATION:
    return "location";
  case DOMAIN_EVENT_TOPIC_OWNER:
    return "owner";
  }
  return "unknown";
}

static const char *domain_entity_kind_name(enum domain_entity_kind kind)
{
  static const char *const names[] = {
      "none",   "world", "descriptor", "character", "room",    "region",
      "object", "zone",  "encounter",  "vessel",    "service",
  };

  if (kind < DOMAIN_ENTITY_NONE || kind >= DOMAIN_ENTITY_KIND_COUNT)
    return "unknown";
  return names[kind];
}

size_t event_debug_render_subscriptions(char *buffer, size_t capacity, int width,
                                        const struct domain_entity_handle *entity, size_t limit)
{
  struct event_debug_output output;
  struct domain_event_bus *bus = domain_event_runtime_bus();
  struct domain_event_bus_stats bus_stats;
  struct domain_event_subscription_stats subscriptions[EVENT_DEBUG_MAX_LIMIT];
  struct domain_event_type_stats type_stats;
  size_t matched;
  size_t shown;
  size_t index;

  limit = MIN(MAX(limit, 1U), EVENT_DEBUG_MAX_LIMIT);
  memset(subscriptions, 0, sizeof(subscriptions));
  debug_output_init(&output, buffer, capacity, width);
  debug_output_title(&output, "Domain Subscriptions");
  if (bus == NULL)
  {
    debug_output_line(&output, "Domain event bus is offline.");
    return output.length;
  }
  memset(&bus_stats, 0, sizeof(bus_stats));
  domain_event_bus_get_stats(bus, &bus_stats);
  debug_output_line(&output, "Live/high-water: %zu/%zu", bus_stats.live_subscription_count,
                    bus_stats.subscription_high_water);
  debug_output_line(&output, "Deliveries/cancellations: %" PRIu64 "/%" PRIu64,
                    bus_stats.subscription_deliveries, bus_stats.subscription_cancellations);
  if (entity != NULL && domain_entity_handle_is_valid(*entity))
    matched = domain_event_inspect_entity_subscriptions(bus, *entity, subscriptions, limit);
  else
    matched = domain_event_inspect_subscriptions(bus, NULL, subscriptions, limit);
  shown = MIN(matched, limit);
  debug_output_line(&output, "Matched/showing: %zu/%zu", matched, shown);
  for (index = 0U; index < shown; index++)
  {
    const char *type_name = "unknown";

    memset(&type_stats, 0, sizeof(type_stats));
    if (domain_event_get_type_stats(bus, subscriptions[index].type, &type_stats) == DOMAIN_EVENT_OK)
      type_name = type_stats.name;
    debug_output_line(&output, "");
    debug_output_line(&output, "#%" PRIu64 " %s", subscriptions[index].handle.id,
                      subscriptions[index].identity);
    debug_output_line(&output, "  event: %s (0x%08" PRIx32 ")", type_name,
                      subscriptions[index].type);
    debug_output_line(&output, "  topic: %s %s %" PRIu64 ":%" PRIu64,
                      domain_topic_role_name(subscriptions[index].topic.role),
                      domain_entity_kind_name(subscriptions[index].topic.entity.kind),
                      subscriptions[index].topic.entity.runtime_id,
                      subscriptions[index].topic.entity.generation);
    debug_output_line(&output, "  owner: %s %" PRIu64 ":%" PRIu64,
                      domain_entity_kind_name(subscriptions[index].owner.kind),
                      subscriptions[index].owner.runtime_id, subscriptions[index].owner.generation);
    debug_output_line(&output, "  priority/calls: %d/%" PRIu64 "%s", subscriptions[index].priority,
                      subscriptions[index].calls,
                      (subscriptions[index].flags & DOMAIN_EVENT_SUBSCRIPTION_ONCE) != 0U ? " once"
                                                                                          : "");
  }
  if (matched > shown)
    debug_output_line(&output, "%zu more subscription(s) matched.", matched - shown);
  return output.length;
}

static void event_debug_page(struct char_data *ch, char *buffer)
{
  if (ch->desc != NULL)
    page_string(ch->desc, buffer, TRUE);
  else
    send_to_char(ch, "%s", buffer);
}

enum event_debug_entity_kind
{
  EVENT_DEBUG_ENTITY_PLAYER = 0,
  EVENT_DEBUG_ENTITY_MOBILE,
  EVENT_DEBUG_ENTITY_OBJECT,
  EVENT_DEBUG_ENTITY_ROOM
};

static bool event_debug_parse_entity_kind(const char *name, enum event_debug_entity_kind *kind)
{
  if (name == NULL || kind == NULL)
    return false;
  if (!strcasecmp(name, "player") || !strcasecmp(name, "character") || !strcasecmp(name, "char"))
    *kind = EVENT_DEBUG_ENTITY_PLAYER;
  else if (!strcasecmp(name, "mob") || !strcasecmp(name, "mobile"))
    *kind = EVENT_DEBUG_ENTITY_MOBILE;
  else if (!strcasecmp(name, "object") || !strcasecmp(name, "obj"))
    *kind = EVENT_DEBUG_ENTITY_OBJECT;
  else if (!strcasecmp(name, "room"))
    *kind = EVENT_DEBUG_ENTITY_ROOM;
  else
    return false;
  return true;
}

static bool event_debug_select_entity(struct char_data *ch, enum event_debug_entity_kind kind,
                                      char *target, struct event_debug_filter *filter,
                                      struct domain_entity_handle *domain_entity)
{
  struct char_data *character;
  struct obj_data *object;
  room_rnum room;
  uint64_t vnum;

  if (ch == NULL || filter == NULL)
    return false;
  if (target == NULL)
    target = "";
  if (domain_entity != NULL)
    *domain_entity = domain_entity_handle_none();
  filter->owner_set = true;
  filter->owner = game_event_owner_none();
  switch (kind)
  {
  case EVENT_DEBUG_ENTITY_PLAYER:
  case EVENT_DEBUG_ENTITY_MOBILE:
    if (*target == '\0' || (character = get_char_vis(ch, target, NULL, FIND_CHAR_WORLD)) == NULL ||
        (kind == EVENT_DEBUG_ENTITY_PLAYER && IS_NPC(character)) ||
        (kind == EVENT_DEBUG_ENTITY_MOBILE && !IS_NPC(character)))
    {
      send_to_char(ch, "No visible %s matches '%s'.\r\n",
                   kind == EVENT_DEBUG_ENTITY_PLAYER ? "online player" : "mobile", target);
      return false;
    }
    filter->owner.kind = GAME_EVENT_OWNER_CHARACTER;
    filter->owner.runtime_id = (uint64_t)(uintptr_t)character;
    if (domain_entity != NULL)
      *domain_entity = domain_event_character_handle(character);
    break;
  case EVENT_DEBUG_ENTITY_OBJECT:
    if (*target == '\0' || (object = get_obj_vis(ch, target, NULL)) == NULL)
    {
      send_to_char(ch, "No visible object matches '%s'.\r\n", target);
      return false;
    }
    filter->owner.kind = GAME_EVENT_OWNER_OBJECT;
    filter->owner.runtime_id = (uint64_t)(uintptr_t)object;
    if (domain_entity != NULL)
      *domain_entity = domain_event_object_handle(object);
    break;
  case EVENT_DEBUG_ENTITY_ROOM:
    if (*target == '\0' || !strcasecmp(target, "here"))
      room = IN_ROOM(ch);
    else if (!parse_uint64(target, &vnum) || vnum > INT_MAX ||
             (room = real_room((room_vnum)vnum)) == NOWHERE)
    {
      send_to_char(ch, "No room with vnum '%s' is loaded.\r\n", target);
      return false;
    }
    if (room == NOWHERE || room > top_of_world)
    {
      send_to_char(ch, "You are not in a loaded room.\r\n");
      return false;
    }
    filter->owner.kind = GAME_EVENT_OWNER_ROOM;
    filter->owner.runtime_id = (uint64_t)(uint32_t)GET_ROOM_VNUM(room) + 1U;
    if (domain_entity != NULL)
      *domain_entity = domain_event_room_handle(room);
    break;
  }
  filter->owner.generation = 0U;
  filter->owner_generation_set = false;
  return true;
}

ACMD(do_eventdebug)
{
  char action[MAX_INPUT_LENGTH] = {'\0'};
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  char arg3[MAX_INPUT_LENGTH] = {'\0'};
  char arg4[MAX_INPUT_LENGTH] = {'\0'};
  char buffer[MAX_STRING_LENGTH] = {'\0'};
  struct event_debug_filter filter;
  struct domain_entity_handle domain_entity;
  enum event_debug_entity_kind entity_kind;
  enum game_event_owner_kind owner_kind;
  enum event_debug_state state;
  uint64_t value;
  uint64_t maximum;
  size_t limit;
  int width;

  argument = one_argument(argument, action, sizeof(action));
  argument = one_argument(argument, arg1, sizeof(arg1));
  argument = one_argument(argument, arg2, sizeof(arg2));
  argument = one_argument(argument, arg3, sizeof(arg3));
  one_argument(argument, arg4, sizeof(arg4));
  width = event_debug_effective_width(GET_SCREEN_WIDTH(ch));
  memset(&filter, 0, sizeof(filter));

  if (*action == '\0' || !strcasecmp(action, "summary") || !strcasecmp(action, "status"))
    event_debug_render_summary(buffer, sizeof(buffer), width);
  else if (!strcasecmp(action, "ready"))
  {
    struct ready_action_latency stats;

    if (*arg1 != '\0' && strcasecmp(arg1, "reset"))
    {
      send_to_char(ch, "Usage: eventdebug ready [reset]\r\n");
      return;
    }
    if (!strcasecmp(arg1, "reset"))
      ready_action_latency_reset();
    ready_action_latency_read(&stats);
    snprintf(buffer, sizeof(buffer),
             "Ready native deadline lateness (pulses; last 1024 callbacks)\r\n"
             "Samples: %zu  Callbacks since reset: %" PRIu64 "\r\n"
             "p50: %" PRIu64 "  p95: %" PRIu64 "  p99: %" PRIu64 "  max: %" PRIu64 "\r\n"
             "The intentional one-pulse ready delay is excluded.\r\n",
             stats.samples, stats.callbacks, stats.p50, stats.p95, stats.p99, stats.maximum);
  }
  else if (!strcasecmp(action, "help"))
    event_debug_render_help(buffer, sizeof(buffer), width);
  else if (!strcasecmp(action, "queue"))
  {
    limit = parse_limit(arg1, EVENT_DEBUG_DEFAULT_LIMIT);
    if (limit == 0)
    {
      send_to_char(ch, "Usage: eventdebug queue [1-%u]\r\n", EVENT_DEBUG_MAX_LIMIT);
      return;
    }
    event_debug_render_queue(buffer, sizeof(buffer), width, &filter, limit);
  }
  else if (!strcasecmp(action, "id"))
  {
    if (!parse_uint64(arg1, &value) || value == 0)
    {
      send_to_char(ch, "Usage: eventdebug id <positive-id>\r\n");
      return;
    }
    filter.event_id_set = true;
    filter.event_id = value;
    event_debug_render_queue(buffer, sizeof(buffer), width, &filter, 1);
  }
  else if (!strcasecmp(action, "type"))
  {
    limit = parse_limit(arg2, EVENT_DEBUG_DEFAULT_LIMIT);
    if (*arg1 == '\0' || limit == 0)
    {
      send_to_char(ch,
                   "Usage: eventdebug type <text>\r\n"
                   "       [1-%u]\r\n",
                   EVENT_DEBUG_MAX_LIMIT);
      return;
    }
    filter.type_contains = arg1;
    event_debug_render_queue(buffer, sizeof(buffer), width, &filter, limit);
  }
  else if (!strcasecmp(action, "owner"))
  {
    if (!event_debug_parse_owner_kind(arg1, &owner_kind) || !parse_uint64(arg2, &value))
    {
      send_to_char(ch, "Usage: eventdebug owner <kind> <id>\r\n"
                       "       [generation]\r\n");
      return;
    }
    filter.owner_set = true;
    filter.owner.kind = owner_kind;
    filter.owner.runtime_id = value;
    if (*arg3 != '\0')
    {
      if (!parse_uint64(arg3, &filter.owner.generation))
      {
        send_to_char(ch, "Generation must be a non-negative\r\n"
                         "integer.\r\n");
        return;
      }
      filter.owner_generation_set = true;
    }
    event_debug_render_queue(buffer, sizeof(buffer), width, &filter, EVENT_DEBUG_DEFAULT_LIMIT);
  }
  else if (event_debug_parse_entity_kind(action, &entity_kind))
  {
    limit = parse_limit(arg2, EVENT_DEBUG_DEFAULT_LIMIT);
    if (limit == 0)
    {
      send_to_char(ch, "Usage: eventdebug %s <target> [1-%u]\r\n", action, EVENT_DEBUG_MAX_LIMIT);
      return;
    }
    if (!event_debug_select_entity(ch, entity_kind, arg1, &filter, NULL))
      return;
    event_debug_render_queue(buffer, sizeof(buffer), width, &filter, limit);
  }
  else if (!strcasecmp(action, "scripts"))
  {
    limit = parse_limit(arg3, EVENT_DEBUG_DEFAULT_LIMIT);
    if (!event_debug_parse_entity_kind(arg1, &entity_kind) || *arg2 == '\0' || limit == 0)
    {
      send_to_char(ch,
                   "Usage: eventdebug scripts <kind> <target>\r\n"
                   "       [1-%u]\r\n"
                   "Kinds: player mob object room\r\n",
                   EVENT_DEBUG_MAX_LIMIT);
      return;
    }
    if (!event_debug_select_entity(ch, entity_kind, arg2, &filter, NULL))
      return;
    filter.type_contains = "dg.";
    event_debug_render_queue(buffer, sizeof(buffer), width, &filter, limit);
  }
  else if (!strcasecmp(action, "due"))
  {
    limit = parse_limit(arg2, EVENT_DEBUG_DEFAULT_LIMIT);
    if (!parse_uint64(arg1, &value) || limit == 0)
    {
      send_to_char(ch,
                   "Usage: eventdebug due <max-pulses>\r\n"
                   "       [1-%u]\r\n",
                   EVENT_DEBUG_MAX_LIMIT);
      return;
    }
    filter.maximum_remaining_set = true;
    filter.maximum_remaining = value;
    event_debug_render_queue(buffer, sizeof(buffer), width, &filter, limit);
  }
  else if (!strcasecmp(action, "range"))
  {
    limit = parse_limit(arg3, EVENT_DEBUG_DEFAULT_LIMIT);
    if (!parse_uint64(arg1, &value) || !parse_uint64(arg2, &maximum) || value > maximum ||
        limit == 0 || *arg4 != '\0')
    {
      send_to_char(ch,
                   "Usage: eventdebug range <min> <max>\r\n"
                   "       [1-%u]\r\n",
                   EVENT_DEBUG_MAX_LIMIT);
      return;
    }
    filter.minimum_remaining_set = true;
    filter.minimum_remaining = value;
    filter.maximum_remaining_set = true;
    filter.maximum_remaining = maximum;
    event_debug_render_queue(buffer, sizeof(buffer), width, &filter, limit);
  }
  else if (!strcasecmp(action, "state"))
  {
    limit = parse_limit(arg2, EVENT_DEBUG_DEFAULT_LIMIT);
    if (!event_debug_parse_state(arg1, &state) || limit == 0)
    {
      send_to_char(ch, "Usage: eventdebug state <state>\r\n"
                       "States: queued ready running\r\n"
                       "        cancel-pending\r\n");
      return;
    }
    filter.state_set = true;
    filter.state = state;
    event_debug_render_queue(buffer, sizeof(buffer), width, &filter, limit);
  }
  else if (!strcasecmp(action, "types"))
  {
    limit = parse_limit(arg1, 10U);
    if (limit == 0)
    {
      send_to_char(ch, "Usage: eventdebug types [1-%u]\r\n", EVENT_DEBUG_MAX_LIMIT);
      return;
    }
    event_debug_render_profiles(buffer, sizeof(buffer), width, limit);
  }
  else if (!strcasecmp(action, "domain"))
    event_debug_render_domain(buffer, sizeof(buffer), width, arg1);
  else if (!strcasecmp(action, "subscriptions") || !strcasecmp(action, "subs"))
  {
    domain_entity = domain_entity_handle_none();
    if (*arg1 == '\0' || parse_uint64(arg1, &value))
    {
      limit = parse_limit(arg1, EVENT_DEBUG_DEFAULT_LIMIT);
      if (limit == 0)
      {
        send_to_char(ch, "Usage: eventdebug subscriptions [1-%u]\r\n", EVENT_DEBUG_MAX_LIMIT);
        return;
      }
      event_debug_render_subscriptions(buffer, sizeof(buffer), width, NULL, limit);
    }
    else
    {
      limit = parse_limit(arg3, EVENT_DEBUG_DEFAULT_LIMIT);
      if (!event_debug_parse_entity_kind(arg1, &entity_kind) || *arg2 == '\0' || limit == 0)
      {
        send_to_char(ch,
                     "Usage: eventdebug subscriptions <kind> <target>\r\n"
                     "       [1-%u]\r\n"
                     "Kinds: player mob object room\r\n",
                     EVENT_DEBUG_MAX_LIMIT);
        return;
      }
      if (!event_debug_select_entity(ch, entity_kind, arg2, &filter, &domain_entity))
        return;
      event_debug_render_subscriptions(buffer, sizeof(buffer), width, &domain_entity, limit);
    }
  }
  else
  {
    event_debug_render_help(buffer, sizeof(buffer), width);
  }
  event_debug_page(ch, buffer);
}
