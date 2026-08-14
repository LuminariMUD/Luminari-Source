#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/combat/fight.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/perfmon.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static struct char_data *perfmon_test_mobile(void)
{
  struct char_data *ch;

  ch = create_char();
  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &dummy_mob;
  ch->player.name = mob_proto[0].player.name;
  ch->player.short_descr = mob_proto[0].player.short_descr;
  GET_MOB_RNUM(ch) = 0;
  GET_POS(ch) = POS_STANDING;
  GET_DEFAULT_POS(ch) = POS_STANDING;
  char_to_room(ch, 0);
  return ch;
}

void Test_perfmon_percentiles_use_linear_interpolation(CuTest *tc)
{
  const uint64_t samples[] = {40, 10, 30, 20};

  CuAssertDblEquals(tc, 10.0, PERF_calculate_percentile(samples, 4, 0.0), 0.001);
  CuAssertDblEquals(tc, 25.0, PERF_calculate_percentile(samples, 4, 50.0), 0.001);
  CuAssertDblEquals(tc, 38.5, PERF_calculate_percentile(samples, 4, 95.0), 0.001);
  CuAssertDblEquals(tc, 39.7, PERF_calculate_percentile(samples, 4, 99.0), 0.001);
  CuAssertDblEquals(tc, 40.0, PERF_calculate_percentile(samples, 4, 100.0), 0.001);
}

void Test_perfmon_percentiles_reject_invalid_input(CuTest *tc)
{
  const uint64_t sample = 10;

  CuAssertDblEquals(tc, 0.0, PERF_calculate_percentile(NULL, 1, 50.0), 0.001);
  CuAssertDblEquals(tc, 0.0, PERF_calculate_percentile(&sample, 0, 50.0), 0.001);
  CuAssertDblEquals(tc, 0.0, PERF_calculate_percentile(&sample, 1, -1.0), 0.001);
  CuAssertDblEquals(tc, 0.0, PERF_calculate_percentile(&sample, 1, 101.0), 0.001);
}

void Test_perfmon_hour_rollup_promotes_once_with_source_maximum(CuTest *tc)
{
  char report[4096];
  char hour_line[512];
  char *hour_start;
  char *hour_end;
  size_t hour_length;
  unsigned long pulse_count;
  unsigned long i;

  PERF_reset();
  pulse_count = (unsigned long)PERF_pulse_per_second * 60UL * 60UL;
  for (i = 0; i < pulse_count; i++)
  {
    PERF_log_pulse(i == pulse_count / 2 ? 321.0 : 1.0);
  }

  PERF_repr(report, sizeof(report));
  hour_start = strstr(report, "  1 Hours:");
  CuAssertPtrNotNull(tc, hour_start);
  hour_end = strchr(hour_start, '\n');
  CuAssertPtrNotNull(tc, hour_end);
  hour_length = (size_t)(hour_end - hour_start);
  CuAssertTrue(tc, hour_length < sizeof(hour_line));
  memcpy(hour_line, hour_start, hour_length);
  hour_line[hour_length] = '\0';
  CuAssertPtrNotNull(tc, strstr(hour_line, "321.00%"));

  PERF_log_pulse(1.0);
  PERF_repr(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "  1 Hours:"));
  CuAssertPtrEquals(tc, NULL, strstr(report, "  2 Hours:"));
}

void Test_perfmon_csv_reports_and_resets_section_samples(CuTest *tc)
{
  static struct PERF_prof_sect *section = NULL;
  char report[4096];
  volatile uint64_t accumulator;
  uint64_t i;

  PERF_reset();
  PERF_prof_sect_init(&section, "perfmon_test_section");
  PERF_prof_sect_enable_sampling(section);
  PERF_prof_sect_enter(section);
  accumulator = 0;
  for (i = 0; i < 10000; i++)
  {
    accumulator += i;
  }
  PERF_prof_sect_exit(section);
  CuAssertTrue(tc, accumulator > 0);

  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "section,calls,total_usec"));
  CuAssertPtrNotNull(tc, strstr(report, "perfmon_test_section,1,"));

  PERF_reset();
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrEquals(tc, NULL, strstr(report, "perfmon_test_section,"));
}

void Test_perfmon_reports_saturate_tiny_buffers(CuTest *tc)
{
  char report[8];
  size_t written;

  memset(report, 'x', sizeof(report));
  written = PERF_prof_repr_csv(report, sizeof(report));

  CuAssertIntEquals(tc, (int)sizeof(report) - 1, (int)written);
  CuAssertIntEquals(tc, '\0', report[sizeof(report) - 1]);
}

void Test_perfmon_ignores_stale_and_duplicate_section_exits(CuTest *tc)
{
  static struct PERF_prof_sect *section = NULL;
  char report[4096];

  PERF_reset();
  PERF_prof_sect_init(&section, "perfmon_exit_guard");
  PERF_prof_sect_enable_sampling(section);
  PERF_prof_sect_enter(section);
  PERF_prof_sect_exit(section);
  PERF_prof_sect_exit(section);
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "perfmon_exit_guard,1,"));
  CuAssertPtrEquals(tc, NULL, strstr(report, "perfmon_exit_guard,2,"));

  PERF_prof_sect_enter(section);
  PERF_reset();
  PERF_prof_sect_exit(section);
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrEquals(tc, NULL, strstr(report, "perfmon_exit_guard,"));
}

void Test_perfmon_csv_reports_and_resets_event_counters(CuTest *tc)
{
  char report[4096];

  PERF_reset();
  PERF_note_missed_pulses(3);
  PERF_note_vessel_message_throttled();
  PERF_note_vessel_message_throttled();

  CuAssertTrue(tc, PERF_missed_pulse_count() == 3);
  CuAssertTrue(tc, PERF_vessel_message_throttled_count() == 2);
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "# missed_pulses=3"));
  CuAssertPtrNotNull(tc, strstr(report, "# vessel_messages_throttled=2"));

  PERF_reset();
  CuAssertTrue(tc, PERF_missed_pulse_count() == 0);
  CuAssertTrue(tc, PERF_vessel_message_throttled_count() == 0);
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "# missed_pulses=0"));
  CuAssertPtrNotNull(tc, strstr(report, "# vessel_messages_throttled=0"));
}

void Test_perfmon_reports_bounded_game_loop_telemetry(CuTest *tc)
{
  char report[16384];
  int event_profile;

  PERF_reset();
  event_profile = PERF_register_event_callback("slow,event\ncallback");
  CuAssertTrue(tc, event_profile >= 0);
  PERF_note_event_callback(event_profile, 11);
  PERF_note_event_callback(event_profile, 17);
  PERF_note_event_process(5, 3, 2, 1);
  PERF_note_pending_extractions(2, 2, 0);
  PERF_note_catchup_pass(9, 7, 2, 1);

  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "# event_process_calls=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_callbacks_processed=2"));
  CuAssertPtrNotNull(tc, strstr(report, "# events_created_during_processing=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_queue_depth_initial=5"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_queue_depth_latest=3"));
  CuAssertPtrNotNull(tc, strstr(report, "# extraction_calls=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# extractions_processed=2"));
  CuAssertPtrNotNull(tc, strstr(report, "# max_extractions_per_call=2"));
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_requested_missed=9"));
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_replayed_missed=7"));
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_remaining_backlog=2"));
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_budget_exhausted_passes=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_max_requested_missed=9"));
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_max_remaining_backlog=2"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_profile_registered="));
  CuAssertPtrNotNull(tc, strstr(report, "# event_profile_capacity=512"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_profile_report_limit=16"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_profile_overflow_calls=0"));
  CuAssertPtrNotNull(tc, strstr(report, "slow event callback,2,28,14.00,17"));

  PERF_prof_reset();
  PERF_prof_repr_pulse(report, sizeof(report));
  CuAssertPtrEquals(tc, NULL, strstr(report, "slow event callback"));

  PERF_reset();
  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "# event_process_calls=0"));
  CuAssertPtrNotNull(tc, strstr(report, "# extractions_processed=0"));
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_requested_missed=0"));
  CuAssertPtrEquals(tc, NULL, strstr(report, "slow event callback,"));
}

void Test_pending_extraction_batch_clears_cross_character_references(CuTest *tc)
{
  struct room_data room;
  struct zone_data zone;
  struct index_data mobile_index;
  struct char_data mobile_prototype;
  struct room_data *saved_world;
  struct zone_data *saved_zone_table;
  struct index_data *saved_mob_index;
  struct char_data *saved_mob_proto;
  struct char_data *saved_character_list;
  struct char_data *saved_combat_list;
  struct char_data *first_target;
  struct char_data *second_target;
  struct char_data *observer;
  room_rnum saved_top_of_world;
  zone_rnum saved_top_of_zone_table;
  mob_rnum saved_top_of_mobt;

  memset(&room, 0, sizeof(room));
  memset(&zone, 0, sizeof(zone));
  memset(&mobile_index, 0, sizeof(mobile_index));
  memset(&mobile_prototype, 0, sizeof(mobile_prototype));
  room.number = 1;
  room.zone = 0;
  zone.number = 0;
  zone.bot = 0;
  zone.top = 99;
  mobile_index.vnum = 1;
  mobile_index.number = 3;
  mobile_prototype.player.name = (char *)"extraction test mobile";
  mobile_prototype.player.short_descr = (char *)"an extraction test mobile";
  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_zone_table = zone_table;
  saved_top_of_zone_table = top_of_zone_table;
  saved_mob_index = mob_index;
  saved_mob_proto = mob_proto;
  saved_top_of_mobt = top_of_mobt;
  saved_character_list = character_list;
  saved_combat_list = combat_list;
  world = &room;
  top_of_world = 0;
  zone_table = &zone;
  top_of_zone_table = 0;
  mob_index = &mobile_index;
  mob_proto = &mobile_prototype;
  top_of_mobt = 0;
  character_list = NULL;
  combat_list = NULL;

  first_target = perfmon_test_mobile();
  second_target = perfmon_test_mobile();
  observer = perfmon_test_mobile();
  observer->last_attacker = first_target;
  GUARDING(observer) = second_target;
  HUNTING(observer) = first_target;
  FIGHTING(observer) = second_target;
  combat_list = observer;

  extract_char(first_target);
  extract_char(second_target);
  extract_pending_chars();

  CuAssertPtrEquals(tc, observer, character_list);
  CuAssertPtrEquals(tc, NULL, observer->next);
  CuAssertPtrEquals(tc, NULL, observer->last_attacker);
  CuAssertPtrEquals(tc, NULL, GUARDING(observer));
  CuAssertPtrEquals(tc, NULL, HUNTING(observer));
  CuAssertPtrEquals(tc, NULL, FIGHTING(observer));
  CuAssertPtrEquals(tc, NULL, combat_list);

  extract_char(observer);
  extract_pending_chars();
  world = saved_world;
  top_of_world = saved_top_of_world;
  zone_table = saved_zone_table;
  top_of_zone_table = saved_top_of_zone_table;
  mob_index = saved_mob_index;
  mob_proto = saved_mob_proto;
  top_of_mobt = saved_top_of_mobt;
  character_list = saved_character_list;
  combat_list = saved_combat_list;
}
