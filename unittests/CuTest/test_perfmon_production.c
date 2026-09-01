#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/combat/fight.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/handler.h"
#include "../../src/magic/spells.h"
#include "../../src/olc/hedit.h"
#include "../../src/perfmon.h"
#include "../../src/point_update_periodic.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int perfmon_count_text_occurrences(const char *text, const char *needle)
{
  int count;
  size_t needle_length;

  if (text == NULL || needle == NULL || *needle == '\0')
    return 0;

  count = 0;
  needle_length = strlen(needle);
  while ((text = strstr(text, needle)) != NULL)
  {
    count++;
    text += needle_length;
  }
  return count;
}

static int perfmon_file_contains(const char *path, const char *needle)
{
  FILE *input;
  char line[4096];

  input = fopen(path, "r");
  if (input == NULL)
    return FALSE;
  while (fgets(line, sizeof(line), input) != NULL)
  {
    if (strstr(line, needle) != NULL)
    {
      fclose(input);
      return TRUE;
    }
  }
  fclose(input);
  return FALSE;
}

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

void Test_helpcheck_keyword_snapshot_preserves_prefix_matching(CuTest *tc)
{
  const char *keywords[] = {"affects", "cast", "casting", "help", "perfmon"};
  size_t keyword_count;

  keyword_count = sizeof(keywords) / sizeof(keywords[0]);
  CuAssertIntEquals(tc, TRUE, test_helpcheck_keyword_has_prefix(keywords, keyword_count, "aff"));
  CuAssertIntEquals(tc, TRUE, test_helpcheck_keyword_has_prefix(keywords, keyword_count, "HELP"));
  CuAssertIntEquals(tc, TRUE,
                    test_helpcheck_keyword_has_prefix(keywords, keyword_count, "perfmon"));
  CuAssertIntEquals(tc, FALSE,
                    test_helpcheck_keyword_has_prefix(keywords, keyword_count, "casual"));
  CuAssertIntEquals(tc, FALSE, test_helpcheck_keyword_has_prefix(keywords, keyword_count, "where"));
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
  usleep(1000);
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
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_dropped_missed=2"));
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_budget_exhausted_passes=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_max_requested_missed=9"));
  CuAssertPtrNotNull(tc, strstr(report, "# catchup_max_dropped_missed=2"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_profile_registered="));
  CuAssertPtrNotNull(tc, strstr(report, "# event_profile_capacity=512"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_profile_report_limit=16"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_profile_overflow_calls=0"));
  CuAssertPtrNotNull(tc, strstr(report, "slow event callback,2,28,14.00,14.00,16.70,16.94,17"));

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

void Test_perfmon_slow_pulse_correlates_schedule_sql_events_and_sections(CuTest *tc)
{
  static struct PERF_prof_sect *section = NULL;
  char report[16384];
  enum perf_sql_category previous_category;
  int event_profile;

  PERF_reset();
  PERF_prof_sect_init(&section, "minute.character_save");
  PERF_prof_sect_enable_sampling(section);
  PERF_prof_sect_enter(section);
  PERF_prof_sect_exit(section);
  PERF_note_heartbeat(600);
  PERF_note_schedule(PERF_SCHEDULE_60_SECONDS | PERF_SCHEDULE_AUTOSAVE);
  previous_category = PERF_sql_scope_set(PERF_SQL_ACCOUNT);
  PERF_note_sql_query("UPDATE player_data SET account_id=7 WHERE name='hidden'", 2500, 0);
  PERF_sql_scope_restore(previous_category);
  event_profile = PERF_register_event_callback("Combat Round");
  PERF_note_event_callback(event_profile, 4000);
  PERF_note_event_process(2, 1, 1, 0);
  PERF_note_catchup_pass(2, 1, 1, 1);
  PERF_log_pulse(250.0);

  PERF_slow_repr(report, sizeof(report), 1, FALSE);
  CuAssertPtrNotNull(tc, strstr(report, "pulse=600"));
  CuAssertPtrNotNull(tc, strstr(report, "duration=250.000 ms"));
  CuAssertPtrNotNull(tc, strstr(report, "schedules=60s+autosave"));
  CuAssertPtrNotNull(tc, strstr(report, "SQL=1/2.500 ms"));
  CuAssertPtrNotNull(tc, strstr(report, "slowest=Combat Round/4.000 ms"));
  CuAssertPtrNotNull(tc, strstr(report, "catchup=2/1/1"));
  CuAssertPtrNotNull(tc, strstr(report, "minute.character_save"));

  PERF_slow_repr(report, sizeof(report), 1, TRUE);
  CuAssertPtrNotNull(tc, strstr(report, "timestamp_utc,monotonic_usec"));
  CuAssertPtrNotNull(tc, strstr(report, ",600,250000,60s+autosave,1,2500,"));

  PERF_sql_repr(report, sizeof(report), FALSE);
  CuAssertPtrNotNull(tc, strstr(report, "SQL Telemetry"));
  CuAssertPtrNotNull(tc, strstr(report, "account:update.player_data"));

  PERF_reset();
  PERF_slow_repr(report, sizeof(report), 1, FALSE);
  CuAssertPtrNotNull(tc, strstr(report, "retained 0/128"));
  PERF_sql_repr(report, sizeof(report), FALSE);
  CuAssertPtrEquals(tc, NULL, strstr(report, "account:update.player_data"));
}

void Test_perfmon_summary_labels_window_budget_and_dropped_heartbeats(CuTest *tc)
{
  char report[8192];

  PERF_reset();
  PERF_note_heartbeat(1);
  PERF_note_catchup_pass(3, 2, 1, 1);
  PERF_log_pulse(150.0);
  PERF_repr(report, sizeof(report));

  CuAssertPtrNotNull(tc, strstr(report, "Measurement started:"));
  CuAssertPtrNotNull(tc, strstr(report, "Pulse budget: 100.00 ms"));
  CuAssertPtrNotNull(tc, strstr(report, "Executed heartbeats: 1"));
  CuAssertPtrNotNull(tc, strstr(report, "Catch-up: requested=3 replayed=2 dropped=1"));
  CuAssertPtrNotNull(tc, strstr(report, "Max pulse:      150.00 ms (150.00%)"));
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
  world = &room;
  top_of_world = 0;
  zone_table = &zone;
  top_of_zone_table = 0;
  mob_index = &mobile_index;
  mob_proto = &mobile_prototype;
  top_of_mobt = 0;
  character_list = NULL;

  first_target = perfmon_test_mobile();
  second_target = perfmon_test_mobile();
  observer = perfmon_test_mobile();
  observer->last_attacker = first_target;
  GUARDING(observer) = second_target;
  HUNTING(observer) = first_target;
  FIGHTING(observer) = second_target;
  IS_CASTING(observer) = TRUE;
  CASTING_TIME(observer) = 2;
  CASTING_TIME_MAX(observer) = 2;
  CASTING_SPELLNUM(observer) = 1;
  CASTING_TCH(observer) = second_target;

  extract_char(first_target);
  extract_char(second_target);
  extract_pending_chars();

  CuAssertPtrEquals(tc, observer, character_list);
  CuAssertPtrEquals(tc, NULL, observer->next);
  CuAssertPtrEquals(tc, NULL, observer->last_attacker);
  CuAssertPtrEquals(tc, NULL, GUARDING(observer));
  CuAssertPtrEquals(tc, NULL, HUNTING(observer));
  CuAssertPtrEquals(tc, NULL, FIGHTING(observer));
  CuAssertIntEquals(tc, FALSE, IS_CASTING(observer));
  CuAssertIntEquals(tc, 0, CASTING_TIME(observer));
  CuAssertPtrEquals(tc, NULL, CASTING_TCH(observer));

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
}

void Test_perfmon_memory_sampling_populates_os_and_allocator_metrics(CuTest *tc)
{
  struct perf_memory_stats stats;
  int ok;

  memset(&stats, 0, sizeof(stats));
  ok = PERF_sample_memory(&stats);

  CuAssertIntEquals(tc, 1, ok);
  CuAssertTrue(tc, stats.timestamp_sec > 0);
  /* On Linux, VmRSS or max_rss_kib should be populated */
  CuAssertTrue(tc, stats.vm_rss_kib > 0 || stats.max_rss_kib > 0);
}

void Test_perfmon_memory_sampling_counts_affects_and_npc_followers(CuTest *tc)
{
  struct char_data npc;
  struct char_data companion;
  struct char_data pc;
  struct affected_type first_affect;
  struct affected_type second_affect;
  struct char_data *saved_character_list;
  struct perf_memory_stats stats;

  clear_char(&npc);
  clear_char(&companion);
  clear_char(&pc);
  memset(&first_affect, 0, sizeof(first_affect));
  memset(&second_affect, 0, sizeof(second_affect));
  SET_BIT_AR(MOB_FLAGS(&npc), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&companion), MOB_ISNPC);
  SET_BIT_AR(AFF_FLAGS(&companion), AFF_CHARM);
  companion.master = &pc;
  npc.affected = &first_affect;
  first_affect.next = &second_affect;
  npc.next = &companion;
  companion.next = &pc;

  saved_character_list = character_list;
  character_list = &npc;
  memset(&stats, 0, sizeof(stats));
  PERF_sample_memory(&stats);
  character_list = saved_character_list;

  CuAssertIntEquals(tc, 3, (int)stats.count_chars);
  CuAssertIntEquals(tc, 2, (int)stats.count_mobs);
  CuAssertIntEquals(tc, 1, (int)stats.count_pcs);
  CuAssertIntEquals(tc, 1, (int)stats.count_affected_chars);
  CuAssertIntEquals(tc, 2, (int)stats.count_affects);
  CuAssertIntEquals(tc, 1, (int)stats.count_npc_followers);
  CuAssertIntEquals(tc, 1, (int)stats.count_charmed_npcs);
}

void Test_perfmon_memory_repr_dashboard_contains_key_sections(CuTest *tc)
{
  char report[16384];
  size_t written;

  PERF_reset();
  written = PERF_memory_repr(report, sizeof(report));

  CuAssertTrue(tc, written > 0);
  CuAssertPtrNotNull(tc, strstr(report, "Memory Monitoring Dashboard"));
  CuAssertPtrNotNull(tc, strstr(report, "Operating System Memory"));
  CuAssertPtrNotNull(tc, strstr(report, "Heap Allocator"));
  CuAssertPtrNotNull(tc, strstr(report, "Memory Growth Analysis"));
  CuAssertPtrNotNull(tc, strstr(report, "Live Game Entity Inventory"));
  CuAssertPtrNotNull(tc, strstr(report, "Spell Affect Nodes"));
  CuAssertPtrNotNull(tc, strstr(report, "NPC Followers"));
}

void Test_perfmon_memory_csv_reports_memory_and_entity_metrics(CuTest *tc)
{
  char report[8192];
  size_t written;

  PERF_reset();
  written = PERF_memory_csv(report, sizeof(report));

  CuAssertTrue(tc, written > 0);
  CuAssertPtrNotNull(tc, strstr(report, "# memory_timestamp_sec="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_vm_rss_kib="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_rss_anon_kib="));
  CuAssertIntEquals(tc, 1, perfmon_count_text_occurrences(report, "# memory_vm_data_kib="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_vm_swap_kib="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_heap_inuse_kib="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_count_descriptors="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_count_chars="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_count_affected_chars="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_count_affects="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_count_npc_followers="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_count_charmed_npcs="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_count_objs="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_count_events="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_delta_count_affects_since_reset="));
  CuAssertPtrNotNull(tc, strstr(report, "# memory_delta_count_charmed_npcs_since_reset="));
}

void Test_perfmon_combat_guard_bounds_attack_and_proc_chains(CuTest *tc)
{
  char report[4096];
  int i;

  PERF_reset();
  PERF_combat_round_begin(NULL);
  for (i = 0; i < 128; i++)
  {
    CuAssertIntEquals(tc, TRUE, PERF_combat_allow_attack());
    CuAssertIntEquals(tc, TRUE, PERF_combat_allow_proc());
  }
  CuAssertIntEquals(tc, FALSE, PERF_combat_allow_attack());
  CuAssertIntEquals(tc, FALSE, PERF_combat_allow_proc());
  PERF_combat_round_end();

  PERF_combat_repr(report, sizeof(report), 1, FALSE);
  CuAssertPtrNotNull(tc, strstr(report, "Callbacks=1"));
  CuAssertPtrNotNull(tc, strstr(report, "limited=1"));
  CuAssertPtrNotNull(tc, strstr(report, "rejected_attacks=1"));
  CuAssertPtrNotNull(tc, strstr(report, "rejected_procs=1"));

  PERF_reset();
  PERF_combat_repr(report, sizeof(report), 1, TRUE);
  CuAssertPtrNotNull(tc, strstr(report, "combat_timestamp,elapsed_usec"));
  CuAssertPtrEquals(tc, NULL, strstr(report, ",128,128,1,1"));
}

void Test_autoproc_registry_tracks_flags_and_safe_removal(CuTest *tc)
{
  struct obj_data *saved_object_list;
  struct obj_data *first;
  struct obj_data *second;
  struct obj_data *iterated;
  size_t baseline;

  autoproc_registry_reset_for_test();
  saved_object_list = object_list;
  baseline = autoproc_registry_count();
  first = create_obj();
  second = create_obj();
  SET_BIT_AR(GET_OBJ_EXTRA(first), ITEM_AUTOPROC);
  SET_BIT_AR(GET_OBJ_EXTRA(second), ITEM_AUTOPROC);
  autoproc_registry_sync(first);
  autoproc_registry_sync(second);

  CuAssertIntEquals(tc, (int)baseline + 2, (int)autoproc_registry_count());
  CuAssertIntEquals(tc, 0, (int)autoproc_registry_validate());
  iterated = autoproc_registry_iteration_begin();
  CuAssertPtrEquals(tc, second, iterated);
  autoproc_registry_remove(first);
  iterated = autoproc_registry_iteration_next();
  CuAssertTrue(tc, iterated != first);
  autoproc_registry_iteration_end();

  autoproc_registry_remove(second);
  object_list = saved_object_list;
  free(first);
  free(second);
  CuAssertIntEquals(tc, (int)baseline, (int)autoproc_registry_count());
}

void Test_affected_registry_tracks_membership_and_safe_removal(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct char_data *saved_character_list;
  struct char_data *iterated;
  struct affected_type first_affect;
  struct affected_type second_affect;
  size_t baseline;

  affected_registry_reset_for_test();
  clear_char(&first);
  clear_char(&second);
  memset(&first_affect, 0, sizeof(first_affect));
  memset(&second_affect, 0, sizeof(second_affect));
  saved_character_list = character_list;
  baseline = affected_registry_count();
  first.next = saved_character_list;
  second.next = &first;
  character_list = &second;
  first.affected = &first_affect;
  second.affected = &second_affect;
  affected_registry_attach(&first);
  affected_registry_attach(&second);

  CuAssertIntEquals(tc, (int)baseline + 2, (int)affected_registry_count());
  CuAssertIntEquals(tc, 0, (int)affected_registry_validate());
  iterated = affected_registry_iteration_begin();
  CuAssertPtrEquals(tc, &second, iterated);
  affected_registry_detach(&first);
  iterated = affected_registry_iteration_next();
  CuAssertTrue(tc, iterated != &first);
  affected_registry_iteration_end();

  affected_registry_detach(&second);
  character_list = saved_character_list;
  CuAssertIntEquals(tc, (int)baseline, (int)affected_registry_count());
}

void Test_player_live_entry_registers_loaded_timed_affects(CuTest *tc)
{
  struct affected_type affect;
  struct char_data player;
  struct char_data *saved_character_list;
  size_t count_before_attach;
  size_t count_after_attach;
  bool entry_hook_present;
  bool still_affected;
  int remaining_duration;

  affected_registry_reset_for_test();
  clear_char(&player);
  player.player_specials = &dummy_mob;
  player.player.short_descr = (char *)"affected registry player";
  new_affect(&affect);
  affect.duration = 2;
  affect_to_char(&player, &affect);

  saved_character_list = character_list;
  character_list = NULL;
  player.next = NULL;
  character_list = &player;
  entry_hook_present =
      perfmon_file_contains("src/interpreter.c", "affected_registry_attach(d->character);");

  count_before_attach = affected_registry_count();
  affected_registry_attach(&player);
  count_after_attach = affected_registry_count();
  affect_update();
  still_affected = player.affected != NULL;
  remaining_duration = still_affected ? player.affected->duration : -1;

  while (player.affected != NULL)
    affect_remove_no_total(&player, player.affected);
  affected_registry_detach(&player);
  character_list = saved_character_list;

  CuAssertTrue(tc, entry_hook_present);
  CuAssertIntEquals(tc, 0, (int)count_before_attach);
  CuAssertIntEquals(tc, 1, (int)count_after_attach);
  CuAssertTrue(tc, still_affected);
  CuAssertIntEquals(tc, 1, remaining_duration);
}

void Test_player_live_entry_registers_for_point_updates(CuTest *tc)
{
  struct char_data player;
  struct char_data *saved_character_list;
  bool entry_hook_present;

  event_free_all();
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  point_update_periodic_init();
  clear_char(&player);
  player.player_specials = &dummy_mob;
  player.player.short_descr = (char *)"point update player";

  saved_character_list = character_list;
  character_list = NULL;
  player.next = NULL;
  character_list = &player;
  entry_hook_present =
      perfmon_file_contains("src/interpreter.c", "point_update_character_sync(d->character);");
  point_update_character_sync(&player);

  CuAssertTrue(tc, entry_hook_present);
  CuAssertIntEquals(tc, 1, (int)point_update_character_count());
  CuAssertIntEquals(tc, 0, (int)point_update_character_registry_validate());

  point_update_character_forget(&player);
  character_list = saved_character_list;
  point_update_periodic_reset_for_test();
  event_free_all();
}

void Test_world_cleanup_owns_stable_location_trail_registry(CuTest *tc)
{
  CuAssertTrue(tc,
               perfmon_file_contains("src/db.c", "movement_trail_registry_shutdown();"));
  CuAssertTrue(tc, !perfmon_file_contains("src/structs.h", "trail_tracks"));
}

void Test_dg_random_registry_tracks_owners_and_safe_removal(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct char_data *saved_character_list;
  struct script_data first_script;
  struct script_data second_script;
  void *iterated;
  size_t baseline;

  dg_random_registry_reset_for_test();
  clear_char(&first);
  clear_char(&second);
  memset(&first_script, 0, sizeof(first_script));
  memset(&second_script, 0, sizeof(second_script));
  saved_character_list = character_list;
  baseline = dg_random_registry_count(MOB_TRIGGER);
  first.next = saved_character_list;
  second.next = &first;
  character_list = &second;
  SCRIPT(&first) = &first_script;
  SCRIPT(&second) = &second_script;
  first_script.types = MTRIG_RANDOM;
  second_script.types = MTRIG_RANDOM;
  dg_script_bind_owner(&first_script, &first, MOB_TRIGGER);
  dg_script_bind_owner(&second_script, &second, MOB_TRIGGER);

  CuAssertIntEquals(tc, (int)baseline + 2, (int)dg_random_registry_count(MOB_TRIGGER));
  CuAssertIntEquals(tc, 0, (int)dg_random_registry_validate(MOB_TRIGGER));
  iterated = dg_random_registry_iteration_begin(MOB_TRIGGER);
  CuAssertPtrEquals(tc, &second, iterated);
  first_script.owner = NULL;
  dg_random_registry_sync(&first_script);
  iterated = dg_random_registry_iteration_next();
  CuAssertTrue(tc, iterated != &first);
  dg_random_registry_iteration_end();

  second_script.owner = NULL;
  dg_random_registry_sync(&second_script);
  SCRIPT(&first) = NULL;
  SCRIPT(&second) = NULL;
  character_list = saved_character_list;
  CuAssertIntEquals(tc, (int)baseline, (int)dg_random_registry_count(MOB_TRIGGER));
}

void Test_perfmon_entity_and_sweep_reports_are_actionable(CuTest *tc)
{
  char report[16384];
  const char *character_section;
  const char *character_section_end;
  const char *point_section;
  const char *point_section_end;
  const char *vessel_section;
  const char *vessel_section_end;
  const char *line;
  const char *line_end;

  PERF_reset();
  PERF_note_mobile_created(1234, 12, PERF_ENTITY_ENCOUNTER);
  PERF_note_object_created(5678, 12, PERF_ENTITY_QUEST);
  PERF_note_zone_reset(12, 25000, 2, 1, 3, 1);
  PERF_note_sweep(PERF_SWEEP_AUTOPROC, 4, 4, 3);
  PERF_entities_repr(report, sizeof(report), FALSE);

  CuAssertPtrNotNull(tc, strstr(report, "encounter"));
  CuAssertPtrNotNull(tc, strstr(report, "1234"));
  CuAssertPtrNotNull(tc, strstr(report, "5678"));
  CuAssertPtrNotNull(tc, strstr(report, "Population sweep telemetry"));
  CuAssertPtrNotNull(tc, strstr(report, "autoproc"));
  character_section = strstr(report, "Character owners:");
  character_section_end =
      character_section != NULL ? strstr(character_section, "Point update:") : NULL;
  CuAssertPtrNotNull(tc, character_section);
  CuAssertPtrNotNull(tc, character_section_end);
  CuAssertPtrNotNull(tc, strstr(character_section, "  registry: members="));
  CuAssertPtrNotNull(tc, strstr(character_section, "  validation: mismatch="));
  CuAssertPtrNotNull(tc, strstr(character_section, "  capacity: limit="));
  CuAssertPtrNotNull(tc, strstr(character_section, "  callbacks: owners="));
  CuAssertPtrNotNull(tc, strstr(character_section, "  work: luminari="));
  CuAssertPtrNotNull(tc, strstr(character_section, "  work: player-misc="));
  for (line = character_section; line != NULL && line < character_section_end; line = line_end + 2)
  {
    line_end = strstr(line, "\n\r");
    CuAssertTrue(tc, line_end != NULL && line_end <= character_section_end);
    if (line_end == NULL || line_end > character_section_end)
      break;
    CuAssertTrue(tc, (size_t)(line_end - line) <= 80U);
  }
  point_section = character_section_end;
  point_section_end = point_section != NULL ? strstr(point_section, "Vessel owners:") : NULL;
  CuAssertPtrNotNull(tc, point_section);
  CuAssertPtrNotNull(tc, point_section_end);
  CuAssertPtrNotNull(tc, strstr(point_section, "  registry: players="));
  CuAssertPtrNotNull(tc, strstr(point_section, "  validation: players="));
  CuAssertPtrNotNull(tc, strstr(point_section, "  callbacks: service="));
  CuAssertPtrNotNull(tc, strstr(point_section, "  work: players="));
  for (line = point_section; line != NULL && line < point_section_end; line = line_end + 2)
  {
    line_end = strstr(line, "\n\r");
    CuAssertTrue(tc, line_end != NULL && line_end <= point_section_end);
    if (line_end == NULL || line_end > point_section_end)
      break;
    CuAssertTrue(tc, (size_t)(line_end - line) <= 80U);
  }
  vessel_section = point_section_end;
  vessel_section_end = vessel_section != NULL ? strstr(vessel_section, "Active world:") : NULL;
  CuAssertPtrNotNull(tc, vessel_section);
  CuAssertPtrNotNull(tc, vessel_section_end);
  CuAssertPtrNotNull(tc, strstr(vessel_section, "  registry: members="));
  CuAssertPtrNotNull(tc, strstr(vessel_section, "  validation: mismatch="));
  CuAssertPtrNotNull(tc, strstr(vessel_section, "  callbacks: owners="));
  CuAssertPtrNotNull(tc, strstr(vessel_section, "  RoL fixed: loaded="));
  CuAssertPtrNotNull(tc, strstr(vessel_section, "  RoL check: mismatch="));
  for (line = vessel_section; line != NULL && line < vessel_section_end; line = line_end + 2)
  {
    line_end = strstr(line, "\n\r");
    CuAssertTrue(tc, line_end != NULL && line_end <= vessel_section_end);
    if (line_end == NULL || line_end > vessel_section_end)
      break;
    CuAssertTrue(tc, (size_t)(line_end - line) <= 80U);
  }
}

void Test_perfmon_copyover_snapshot_replaces_one_complete_file(CuTest *tc)
{
  static const char stale_marker[] = "stale snapshot marker\n";
  char snapshot_path[] = "/tmp/luminari-perfmon-snapshot-XXXXXX";
  char temp_path[sizeof(snapshot_path) + 4];
  struct char_data *saved_character_list;
  ssize_t seed_written;
  int snapshot_fd;
  int write_result;
  int has_header;
  int has_complete_footer;
  int has_stale_marker;
  int has_temp_file;

  snapshot_fd = mkstemp(snapshot_path);
  CuAssertTrue(tc, snapshot_fd >= 0);
  if (snapshot_fd < 0)
    return;
  seed_written = write(snapshot_fd, stale_marker, sizeof(stale_marker) - 1);
  close(snapshot_fd);
  CuAssertIntEquals(tc, (int)(sizeof(stale_marker) - 1), (int)seed_written);

  saved_character_list = character_list;
  character_list = NULL;
  PERF_reset();
  write_result = PERF_write_copyover_snapshot(snapshot_path);
  character_list = saved_character_list;
  snprintf(temp_path, sizeof(temp_path), "%s.tmp", snapshot_path);
  has_header = perfmon_file_contains(snapshot_path, "pre-copyover PERFMON snapshot");
  has_complete_footer = perfmon_file_contains(snapshot_path, "# snapshot_complete=1");
  has_stale_marker = perfmon_file_contains(snapshot_path, "stale snapshot marker");
  has_temp_file = access(temp_path, F_OK) == 0;
  unlink(snapshot_path);
  unlink(temp_path);

  CuAssertIntEquals(tc, TRUE, write_result);
  CuAssertIntEquals(tc, TRUE, has_header);
  CuAssertIntEquals(tc, TRUE, has_complete_footer);
  CuAssertIntEquals(tc, FALSE, has_stale_marker);
  CuAssertIntEquals(tc, FALSE, has_temp_file);
}

void Test_perfmon_copyover_snapshot_preserves_previous_file_on_failure(CuTest *tc)
{
  static const char previous_marker[] = "previous complete snapshot\n";
  char snapshot_path[] = "/tmp/luminari-perfmon-snapshot-XXXXXX";
  char temp_path[sizeof(snapshot_path) + 4];
  ssize_t seed_written;
  int snapshot_fd;
  int write_result;
  int has_previous_marker;

  snapshot_fd = mkstemp(snapshot_path);
  CuAssertTrue(tc, snapshot_fd >= 0);
  if (snapshot_fd < 0)
    return;
  seed_written = write(snapshot_fd, previous_marker, sizeof(previous_marker) - 1);
  close(snapshot_fd);
  CuAssertIntEquals(tc, (int)(sizeof(previous_marker) - 1), (int)seed_written);

  snprintf(temp_path, sizeof(temp_path), "%s.tmp", snapshot_path);
  CuAssertIntEquals(tc, 0, mkdir(temp_path, 0700));
  write_result = PERF_write_copyover_snapshot(snapshot_path);
  has_previous_marker = perfmon_file_contains(snapshot_path, "previous complete snapshot");
  rmdir(temp_path);
  unlink(snapshot_path);

  CuAssertIntEquals(tc, FALSE, write_result);
  CuAssertIntEquals(tc, TRUE, has_previous_marker);
}
