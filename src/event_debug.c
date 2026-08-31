#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "modify.h"
#include "domain_event_runtime.h"
#include "domain_events.h"
#include "event_debug.h"
#include "combat/combat_encounters.h"
#include "net/i3_client.h"
#include "perfmon.h"

#include <errno.h>
#include <inttypes.h>
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
  debug_output_line(&output, "eventdebug due <max-pulses> [limit]");
  debug_output_line(&output, "eventdebug range <min> <max> [limit]");
  debug_output_line(&output, "eventdebug state <state> [limit]");
  debug_output_line(&output, "eventdebug types [limit]");
  debug_output_line(&output, "eventdebug domain [type]");
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
  struct PERF_event_summary perf_stats;
  struct domain_event_bus_stats domain_stats;
  struct i3_ingress_stats ingress_stats;
  struct combat_encounter_stats encounter_stats;
  struct domain_event_bus *bus;

  debug_output_init(&output, buffer, capacity, width);
  event_debug_get_stats(&event_stats);
  PERF_get_event_summary(&perf_stats);
  debug_output_title(&output, "Event Debug Summary");
  debug_output_line(&output, "Backend: %s", event_backend_name());
  debug_output_line(&output, "Pulse: %" PRIu64, event_stats.current_pulse);
  debug_output_line(&output, "Display width: %zu", output.width);
  debug_output_line(&output, "Live events: %zu", event_stats.live_events);
  debug_output_line(&output, "High-water events: %zu", event_stats.high_water_events);
  debug_output_line(&output, "Registry mismatch: %zu", event_stats.registry_mismatches);
  debug_output_line(&output, "Stale-owner outcomes: %" PRIu64,
                    event_stats.stale_owner_outcomes);
  render_live_owner_counts(&output, &event_stats);
  debug_output_line(&output, "");
  debug_output_line(&output, "Compatibility adapter");
  debug_output_line(&output, "  process passes: %" PRIu64, perf_stats.process_calls);
  debug_output_line(&output, "  callbacks: %" PRIu64, perf_stats.callbacks);
  debug_output_line(&output, "  max batch: %" PRIu64, perf_stats.maximum_batch);
  debug_output_line(&output, "  scheduled: %" PRIu64, perf_stats.scheduled);
  debug_output_line(&output, "  cancelled: %" PRIu64, perf_stats.cancelled);
  debug_output_line(&output, "  recurrences: %" PRIu64, perf_stats.rescheduled);
  debug_output_line(&output, "  profiles: %zu", perf_stats.registered_profiles);
  debug_output_line(&output, "  overflow calls: %" PRIu64, perf_stats.overflow_calls);
  if (event_stats.scheduler_stats_available)
  {
    debug_output_line(&output, "");
    debug_output_line(&output, "Scheduler queues");
    debug_output_line(&output, "  ready: %zu", event_stats.scheduler.ready_count);
    debug_output_line(&output, "  oldest overdue: %" PRIu64 " pulses",
                      event_stats.scheduler.oldest_overdue_ticks);
    debug_output_line(&output, "  wheel L0: %zu",
                      event_stats.scheduler.wheel_level_counts[0]);
    debug_output_line(&output, "  wheel L1: %zu",
                      event_stats.scheduler.wheel_level_counts[1]);
    debug_output_line(&output, "  wheel L2: %zu",
                      event_stats.scheduler.wheel_level_counts[2]);
    debug_output_line(&output, "  wheel L3: %zu",
                      event_stats.scheduler.wheel_level_counts[3]);
    debug_output_line(&output, "  wheel L4: %zu",
                      event_stats.scheduler.wheel_level_counts[4]);
    debug_output_line(&output, "  overflow: %zu", event_stats.scheduler.overflow_count);
    debug_output_line(&output, "  owner records: %zu", event_stats.scheduler.owner_count);
    debug_output_line(&output, "  timed ingress: main thread only");
    debug_output_line(&output, "");
    debug_output_line(&output, "Scheduler lifecycle");
    debug_output_line(&output, "  scheduled: %" PRIu64,
                      event_stats.scheduler.total_scheduled);
    debug_output_line(&output, "  callbacks: %" PRIu64,
                      event_stats.scheduler.total_callbacks);
    debug_output_line(&output, "  completed: %" PRIu64, event_stats.scheduler.total_completed);
    debug_output_line(&output, "  cancelled: %" PRIu64,
                      event_stats.scheduler.total_cancelled);
    debug_output_line(&output, "  failed: %" PRIu64, event_stats.scheduler.total_failed);
    debug_output_line(&output, "  rescheduled: %" PRIu64,
                      event_stats.scheduler.total_rescheduled);
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
    debug_output_line(&output, "  cascaded: %" PRIu64,
                      event_stats.scheduler.total_cascaded_events);
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
  debug_output_line(
      &output, "  mode: %s",
      !encounter_stats.encounter_mode
          ? "character rollback"
          : encounter_stats.semantic_rounds ? "six-second semantic" : "compatibility phases");
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
                      encounter_stats.intents_dispatched,
                      encounter_stats.intent_dispatch_blocks);
    debug_output_line(&output, "  action/reaction spend: %" PRIu64 "/%" PRIu64,
                      encounter_stats.action_budgets_spent, encounter_stats.reactions_spent);
  }
  else
  {
    debug_output_line(&output, "  phases/terminal: %" PRIu64 "/%" PRIu64,
                      encounter_stats.compatibility_phases,
                      encounter_stats.compatibility_terminal);
    debug_output_line(&output, "  compatibility attempts: %" PRIu64,
                      encounter_stats.compatibility_attempts);
  }
  debug_output_line(&output, "  comparison mismatch: %" PRIu64,
                    encounter_stats.compatibility_mismatches);
  debug_output_line(&output, "  admission/stale: %" PRIu64 "/%" PRIu64,
                    encounter_stats.admission_failures,
                    encounter_stats.stale_encounter_callbacks);
  memset(&ingress_stats, 0, sizeof(ingress_stats));
  i3_get_ingress_stats(&ingress_stats);
  debug_output_line(&output, "");
  debug_output_line(&output, "Worker ingress (I3)");
  debug_output_line(&output, "  status: %s", ingress_stats.available ? "online" : "offline");
  debug_output_line(&output, "  depth: %zu/%zu", ingress_stats.depth,
                    ingress_stats.capacity);
  debug_output_line(&output, "  high-water: %" PRIu64, ingress_stats.high_water);
  debug_output_line(&output, "  rejected: %" PRIu64, ingress_stats.rejections);
  debug_output_line(&output, "  wake failures: %" PRIu64, ingress_stats.wake_failures);
  debug_output_line(&output, "");
  bus = domain_event_runtime_bus();
  memset(&domain_stats, 0, sizeof(domain_stats));
  domain_event_bus_get_stats(bus, &domain_stats);
  debug_output_line(&output, "Domain event bus");
  debug_output_line(&output, "  status: %s", bus != NULL ? "online" : "offline");
  debug_output_line(&output, "  sealed: %s", domain_stats.sealed ? "yes" : "no");
  debug_output_line(&output, "  types: %zu", domain_stats.registered_type_count);
  debug_output_line(&output, "  handlers: %zu", domain_stats.registered_handler_count);
  debug_output_line(&output, "  publications: %" PRIu64, domain_stats.publications);
  debug_output_line(&output, "  handler calls: %" PRIu64, domain_stats.handler_calls);
  debug_output_line(&output, "  rejected chains: %" PRIu64,
                    domain_stats.rejected_causal_chains);
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
    memset(&filter, 0, sizeof(filter));
    filter.type_equals = snapshots[index].identity;
    live = event_debug_inspect(&filter, NULL, 0, NULL);
    debug_output_line(&output, "");
    debug_output_line(&output, "%s", snapshots[index].identity);
    debug_output_line(&output, "  live: %zu", live);
    debug_output_line(&output, "  calls: %" PRIu64, snapshots[index].calls);
    debug_output_line(&output, "  total usec: %" PRIu64, snapshots[index].total_usec);
    debug_output_line(&output, "  max usec: %" PRIu64, snapshots[index].maximum_usec);
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

size_t event_debug_render_domain(char *buffer, size_t capacity, int width,
                                 const char *type_filter)
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
      debug_output_line(&output, "    total usec: %" PRIu64,
                        handlers[handler_index].total_usec);
      debug_output_line(&output, "    max usec: %" PRIu64,
                        handlers[handler_index].maximum_usec);
      debug_output_line(&output, "    slow calls: %" PRIu64,
                        handlers[handler_index].slow_calls);
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

static void event_debug_page(struct char_data *ch, char *buffer)
{
  if (ch->desc != NULL)
    page_string(ch->desc, buffer, TRUE);
  else
    send_to_char(ch, "%s", buffer);
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
      send_to_char(ch, "Usage: eventdebug type <text>\r\n"
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
    event_debug_render_queue(buffer, sizeof(buffer), width, &filter,
                             EVENT_DEBUG_DEFAULT_LIMIT);
  }
  else if (!strcasecmp(action, "due"))
  {
    limit = parse_limit(arg2, EVENT_DEBUG_DEFAULT_LIMIT);
    if (!parse_uint64(arg1, &value) || limit == 0)
    {
      send_to_char(ch, "Usage: eventdebug due <max-pulses>\r\n"
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
    if (!parse_uint64(arg1, &value) || !parse_uint64(arg2, &maximum) ||
        value > maximum || limit == 0 || *arg4 != '\0')
    {
      send_to_char(ch, "Usage: eventdebug range <min> <max>\r\n"
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
  else
  {
    event_debug_render_help(buffer, sizeof(buffer), width);
  }
  event_debug_page(ch, buffer);
}
