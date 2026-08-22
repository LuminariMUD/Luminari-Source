#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/comm.h"
#include "../../src/handler.h"
#include "../../src/magic/spells.h"
#include "../../src/modify.h"
#include "../../src/comms/boards.h"
#include "../../src/olc/genolc.h"
#include "../../src/character/class.h"
#include "../../src/character/feats.h"
#include "../../src/dgscript/dg_olc.h"
#include "../../src/net/protocol.h"
#include "../../src/quest/hlquest.h"
#include "../../src/wilderness/terrain_bridge.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

void Test_zone_export_filename_rejects_shell_and_path_metacharacters(CuTest *tc)
{
  const char *allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-";
  const char *hostile = "Tiamat's Lair;|&`$()<>/\\\r\n.tar.gz";
  char sanitized[128];
  char too_small[8];

  CuAssertTrue(tc, genolc_sanitize_export_filename(hostile, sanitized, sizeof(sanitized)));
  CuAssertIntEquals(tc, (int)strlen(sanitized), (int)strspn(sanitized, allowed));
  CuAssertPtrEquals(tc, NULL, strpbrk(sanitized, ";|&`$()<>/\\\r\n'\" "));
  CuAssertTrue(tc, strstr(sanitized, ".tar.gz") != NULL);

  CuAssertTrue(tc,
               !genolc_sanitize_export_filename("filename-too-long", too_small, sizeof(too_small)));
  CuAssertStrEquals(tc, "", too_small);
  CuAssertTrue(tc, !genolc_sanitize_export_filename("", sanitized, sizeof(sanitized)));
}

void Test_feat_sort_contains_each_valid_feat_once(CuTest *tc)
{
  bool seen[FEAT_LAST_FEAT] = {false};
  int feat;
  int position;

  sort_feats();
  for (position = 1; position < FEAT_LAST_FEAT; position++)
  {
    feat = feat_sort_info[position];
    CuAssertTrue(tc, feat > FEAT_UNDEFINED);
    CuAssertTrue(tc, feat < FEAT_LAST_FEAT);
    CuAssertTrue(tc, !seen[feat]);
    seen[feat] = true;
  }
  for (feat = 1; feat < FEAT_LAST_FEAT; feat++)
    CuAssertTrue(tc, seen[feat]);
}

void Test_hlquest_exact_coin_and_duplicate_item_contracts(CuTest *tc)
{
  struct char_data quest_mob;
  struct quest_entry quest;
  struct quest_command first;
  struct quest_command second;

  memset(&quest_mob, 0, sizeof(quest_mob));
  memset(&quest, 0, sizeof(quest));
  memset(&first, 0, sizeof(first));
  memset(&second, 0, sizeof(second));

  GET_GOLD(&quest_mob) = 500;
  CuAssertTrue(tc, hlquest_consume_coins(&quest_mob, 125));
  CuAssertIntEquals(tc, 375, GET_GOLD(&quest_mob));
  CuAssertTrue(tc, !hlquest_consume_coins(&quest_mob, 400));
  CuAssertIntEquals(tc, 375, GET_GOLD(&quest_mob));
  CuAssertTrue(tc, !hlquest_consume_coins(&quest_mob, -1));

  first.type = QUEST_COMMAND_ITEM;
  first.value = 1234;
  first.next = &second;
  second.type = QUEST_COMMAND_ITEM;
  second.value = 1234;
  quest.in = &first;
  CuAssertIntEquals(tc, 2, hlquest_required_item_count(&quest, 1234));
  CuAssertIntEquals(tc, 0, hlquest_required_item_count(&quest, 9999));
}

void Test_partial_output_writes_preserve_buffer_accounting(CuTest *tc)
{
  struct descriptor_data descriptor;

  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.output = descriptor.small_outbuf;
  strlcpy(descriptor.output, "abcdef", sizeof(descriptor.small_outbuf));
  descriptor.bufptr = 6;
  descriptor.bufspace = SMALL_BUFSIZE - 1 - descriptor.bufptr;

  comm_test_retain_unsent_output(&descriptor, "abcdef", 2);
  CuAssertStrEquals(tc, "cdef", descriptor.output);
  CuAssertIntEquals(tc, 4, descriptor.bufptr);
  CuAssertIntEquals(tc, SMALL_BUFSIZE - 5, descriptor.bufspace);

  strlcpy(descriptor.output, "abcdef", sizeof(descriptor.small_outbuf));
  descriptor.bufptr = 6;
  descriptor.bufspace = SMALL_BUFSIZE - 1 - descriptor.bufptr;

  comm_test_retain_unsent_output(&descriptor, "abcdef> ", 6);
  CuAssertStrEquals(tc, "> ", descriptor.output);
  CuAssertIntEquals(tc, 2, descriptor.bufptr);
  CuAssertIntEquals(tc, SMALL_BUFSIZE - 3, descriptor.bufspace);
  CuAssertTrue(tc, write_to_output_raw_atomic(&descriptor, "next", 4, 0));
  CuAssertStrEquals(tc, "> next", descriptor.output);
  CuAssertIntEquals(tc, 6, descriptor.bufptr);
  CuAssertIntEquals(tc, SMALL_BUFSIZE - 7, descriptor.bufspace);
}

void Test_terrain_bridge_rejects_unbounded_batch_ranges(CuTest *tc)
{
  char *response;

  response = process_terrain_request("{\"command\":null}");
  CuAssertPtrNotNull(tc, response);
  CuAssertPtrNotNull(tc, strstr(response, "Missing or invalid command field"));
  free(response);

  response = process_terrain_request(
      "{\"command\":\"get_terrain_batch\",\"params\":{"
      "\"x_min\":-2147483648,\"x_max\":2147483647,\"y_min\":0,\"y_max\":0}}");
  CuAssertPtrNotNull(tc, response);
  CuAssertPtrNotNull(tc, strstr(response, "Batch coordinates out of bounds"));
  free(response);

  response = process_terrain_request("{\"command\":\"get_terrain_batch\",\"params\":{"
                                     "\"x_min\":0,\"x_max\":1000,\"y_min\":0,\"y_max\":1000}}");
  CuAssertPtrNotNull(tc, response);
  CuAssertPtrNotNull(tc, strstr(response, "Batch too large"));
  free(response);
}

void Test_terrain_bridge_http_health_contract(CuTest *tc)
{
  char *response;
  int status_code;
  bool head_only;

  response = process_terrain_http_request("GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n", true,
                                          42, &status_code, &head_only);
  CuAssertPtrNotNull(tc, response);
  CuAssertIntEquals(tc, 200, status_code);
  CuAssertTrue(tc, !head_only);
  CuAssertPtrNotNull(tc, strstr(response, "\"status\":\"healthy\""));
  CuAssertPtrNotNull(tc, strstr(response, "\"database\":\"healthy\""));
  CuAssertPtrNotNull(tc, strstr(response, "\"uptime_seconds\":42"));
  free(response);

  response = process_terrain_http_request("GET /health/ready HTTP/1.1\r\n\r\n", false, 7,
                                          &status_code, &head_only);
  CuAssertPtrNotNull(tc, response);
  CuAssertIntEquals(tc, 503, status_code);
  CuAssertPtrNotNull(tc, strstr(response, "\"status\":\"unhealthy\""));
  CuAssertPtrNotNull(tc, strstr(response, "\"database\":\"unhealthy\""));
  free(response);

  response = process_terrain_http_request("HEAD /health/live HTTP/1.0\r\n\r\n", false, 9,
                                          &status_code, &head_only);
  CuAssertPtrNotNull(tc, response);
  CuAssertIntEquals(tc, 200, status_code);
  CuAssertTrue(tc, head_only);
  CuAssertPtrNotNull(tc, strstr(response, "\"database\":\"not_checked\""));
  free(response);

  response = process_terrain_http_request("POST /health HTTP/1.1\r\n\r\n", true, 1, &status_code,
                                          &head_only);
  CuAssertPtrNotNull(tc, response);
  CuAssertIntEquals(tc, 405, status_code);
  free(response);

  response = process_terrain_http_request("GET /not-health HTTP/1.1\r\n\r\n", true, 1, &status_code,
                                          &head_only);
  CuAssertPtrNotNull(tc, response);
  CuAssertIntEquals(tc, 404, status_code);
  free(response);
}

void Test_upstream_random_generator_sequence(CuTest *tc)
{
  unsigned long first, second;

  circle_srandom(1);
  CuAssertTrue(tc, circle_random() == 16807UL);
  CuAssertTrue(tc, circle_random() == 282475249UL);
  CuAssertTrue(tc, circle_random() == 1622650073UL);

  circle_srandom(42);
  first = circle_random();
  second = circle_random();
  circle_srandom(42);
  CuAssertTrue(tc, circle_random() == first);
  CuAssertTrue(tc, circle_random() == second);

  circle_srandom((unsigned long)time(NULL));
}

void Test_clan_armor_uses_builder_value_two(CuTest *tc)
{
  struct obj_data obj = {0};

  GET_OBJ_VAL(&obj, 1) = 42;
  GET_OBJ_VAL(&obj, 2) = 7;

  CuAssertIntEquals(tc, 42, GET_OBJ_CLAN(&obj));
}

void Test_upstream_random_range_and_dice(CuTest *tc)
{
  int i, value;

  circle_srandom(12345);
  for (i = 0; i < 200; i++)
  {
    value = rand_number(1, 10);
    CuAssertTrue(tc, value >= 1 && value <= 10);
  }
  CuAssertIntEquals(tc, 7, rand_number(7, 7));

  value = rand_number(10, 1);
  CuAssertTrue(tc, value >= 1 && value <= 10);

  CuAssertIntEquals(tc, 0, dice(0, 6));
  CuAssertIntEquals(tc, 0, dice(3, 0));
  CuAssertIntEquals(tc, 1, dice(1, 1));
  for (i = 0; i < 200; i++)
  {
    value = dice(2, 6);
    CuAssertTrue(tc, value >= 2 && value <= 12);
  }

  circle_srandom((unsigned long)time(NULL));
}

void Test_upstream_string_comparison_and_pruning(CuTest *tc)
{
  char crlf[] = "hello\r\n";
  char lf[] = "hello\n";
  char repeated[] = "hi\r\n\r\n";

  prune_crlf(crlf);
  prune_crlf(lf);
  prune_crlf(repeated);
  CuAssertStrEquals(tc, "hello", crlf);
  CuAssertStrEquals(tc, "hello", lf);
  CuAssertStrEquals(tc, "hi", repeated);

  CuAssertIntEquals(tc, 0, str_cmp("Hello", "hello"));
  CuAssertTrue(tc, str_cmp("a", "b") < 0);
  CuAssertTrue(tc, str_cmp("b", "a") > 0);
  CuAssertIntEquals(tc, 0, strn_cmp("hello!", "hellox", 5));
  CuAssertTrue(tc, strn_cmp("abc", "xyz", 3) != 0);
}

static void copy_visible_output_line(char *dest, size_t dest_size, const char *source)
{
  size_t written;

  if (!dest || dest_size == 0)
    return;

  written = 0;
  while (source && *source && *source != '\r' && *source != '\n' && written + 1 < dest_size)
  {
    if (*source == '\033' && source[1] == '[')
    {
      source += 2;
      while (*source && *source != 'm')
        source++;
      if (*source == 'm')
        source++;
      continue;
    }

    dest[written++] = *source++;
  }
  dest[written] = '\0';
}

void Test_column_list_applies_uses_item_width_for_auto_columns(CuTest *tc)
{
  const char *items[] = {"Strength", "Dexterity",    "Resist-Bludgeoning",
                         "Wisdom",   "Constitution", "Spell-Penetration"};
  struct char_data ch;
  struct descriptor_data descriptor;
  struct player_special_data player_specials;
  struct obj_data obj;
  const char *first_break;
  const char *second_row;
  char first_visible_row[81];
  char second_visible_row[81];
  bool first_row_aligned;
  bool second_row_aligned;

  memset(&ch, 0, sizeof(ch));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&obj, 0, sizeof(obj));

  descriptor.character = &ch;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  ch.desc = &descriptor;
  ch.player_specials = &player_specials;
  ch.player.name = "column list test character";
  GET_SCREEN_WIDTH(&ch) = 80;
  GET_PAGE_LENGTH(&ch) = 100;

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize the column list fixture");
    return;
  }

  column_list_applies(&ch, &obj, 0, items, 6, TRUE);

  first_break = strstr(descriptor.output, "\r\n");
  second_row = first_break ? first_break + 2 : NULL;
  copy_visible_output_line(first_visible_row, sizeof(first_visible_row), descriptor.output);
  copy_visible_output_line(second_visible_row, sizeof(second_visible_row), second_row);
  first_row_aligned =
      strlen(first_visible_row) == 78 &&
      strstr(first_visible_row, " 1) Strength") == first_visible_row &&
      strstr(first_visible_row, " 3) Resist-Bludgeoning") == first_visible_row + 26 &&
      strstr(first_visible_row, " 5) Constitution") == first_visible_row + 52;
  second_row_aligned =
      second_row != NULL && strstr(second_visible_row, " 2) Dexterity") == second_visible_row &&
      strstr(second_visible_row, " 4) Wisdom") == second_visible_row + 26 &&
      strstr(second_visible_row, " 6) Spell-Penetration") == second_visible_row + 52;

  ch.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);

  CuAssert(tc, "the first apply-list row should contain three aligned columns", first_row_aligned);
  CuAssert(tc, "the second apply-list row should contain three aligned columns",
           second_row_aligned);
}

void Test_column_list_pages_complete_output_with_visible_separators(CuTest *tc)
{
  const char *items[] = {"abcdefghijklmn", "opqrstuvwxyz12"};
  struct char_data ch;
  struct descriptor_data descriptor;
  struct player_special_data player_specials;
  const char *full_output;

  memset(&ch, 0, sizeof(ch));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&player_specials, 0, sizeof(player_specials));
  descriptor.character = &ch;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  ch.desc = &descriptor;
  ch.player_specials = &player_specials;
  ch.player.name = "column separator test character";
  GET_SCREEN_WIDTH(&ch) = 30;
  GET_PAGE_LENGTH(&ch) = 100;

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize the column separator fixture");
    return;
  }

  column_list(&ch, 0, items, 2, FALSE);
  full_output = descriptor.output;

  CuAssertPtrNotNull(tc, full_output);
  CuAssertPtrNotNull(tc, strstr(full_output, "abcdefghijklmn opqrstuvwxyz12"));
  CuAssertPtrEquals(tc, NULL, strstr(full_output, "OVERFLOW"));

  ch.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
}

void Test_column_list_maximum_page_settings_stay_within_descriptor_capacity(CuTest *tc)
{
  const char *items[300];
  struct char_data ch;
  struct descriptor_data descriptor;
  struct player_special_data player_specials;
  bool all_pages_fit = true;
  bool saw_last_item = false;
  int initial_page_count;
  int i;

  memset(&ch, 0, sizeof(ch));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&player_specials, 0, sizeof(player_specials));
  for (i = 0; i < 300; i++)
    items[i] = "maximum pager boundary item";

  descriptor.character = &ch;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  ch.desc = &descriptor;
  ch.player_specials = &player_specials;
  ch.player.name = "maximum pager boundary test character";
  GET_SCREEN_WIDTH(&ch) = 200;
  GET_PAGE_LENGTH(&ch) = 255;

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize the maximum pager boundary fixture");
    return;
  }

  column_list(&ch, 1, items, 300, TRUE);
  initial_page_count = descriptor.showstr_count;

  while (descriptor.showstr_count > 0)
  {
    if (descriptor.bufspace == 0 || strstr(descriptor.output, "OVERFLOW") != NULL)
      all_pages_fit = false;
    if (strstr(descriptor.output, "300) maximum pager boundary item") != NULL)
      saw_last_item = true;

    descriptor.output[0] = '\0';
    descriptor.bufptr = 0;
    descriptor.bufspace = descriptor.large_outbuf ? LARGE_BUFSIZE - 1 : SMALL_BUFSIZE - 1;
    if (descriptor.showstr_count > 0)
      show_string(&descriptor, "");
  }

  if (descriptor.bufspace == 0 || strstr(descriptor.output, "OVERFLOW") != NULL)
    all_pages_fit = false;
  if (strstr(descriptor.output, "300) maximum pager boundary item") != NULL)
    saw_last_item = true;

  ch.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  if (descriptor.large_outbuf != NULL)
  {
    free(descriptor.large_outbuf->text);
    free(descriptor.large_outbuf);
    if (buf_largecount > 0)
      buf_largecount--;
  }

  CuAssertTrue(tc, initial_page_count > 1);
  CuAssertTrue(tc, all_pages_fit);
  CuAssertTrue(tc, saw_last_item);
}

void Test_add_commas_supports_multiple_values_in_one_format(CuTest *tc)
{
  const char *first;
  const char *second;

  first = add_commas(1234);
  second = add_commas(987654);

  CuAssertStrEquals(tc, "1,234", first);
  CuAssertStrEquals(tc, "987,654", second);
}

void Test_staff_simplex_board_has_legacy_board_storage(CuTest *tc)
{
  CuAssertIntEquals(tc, 2, NUM_OF_BOARDS);
  CuAssertIntEquals(tc, 3098, board_info[1].vnum);
}

void Test_upstream_type_and_bit_formatting(CuTest *tc)
{
  const char *names[] = {"FLAG_A", "FLAG_B", "\n"};
  char result[256];

  sprintbit(0, names, result, sizeof(result));
  CuAssertStrEquals(tc, "None ", result);
  sprintbit(1, names, result, sizeof(result));
  CuAssertStrEquals(tc, "FLAG_A ", result);
  sprintbit(3, names, result, sizeof(result));
  CuAssertStrEquals(tc, "FLAG_A FLAG_B ", result);
  sprintbit(4, names, result, sizeof(result));
  CuAssertStrEquals(tc, "UNDEFINED ", result);

  sprinttype(0, names, result, sizeof(result));
  CuAssertStrEquals(tc, "FLAG_A", result);
  sprinttype(1, names, result, sizeof(result));
  CuAssertStrEquals(tc, "FLAG_B", result);
  sprinttype(5, names, result, sizeof(result));
  CuAssertStrEquals(tc, "UNDEFINED", result);
}

void Test_upstream_text_measurement_helpers(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, levenshtein_distance("hello", "hello"));
  CuAssertIntEquals(tc, 5, levenshtein_distance("", "hello"));
  CuAssertIntEquals(tc, 5, levenshtein_distance("hello", ""));
  CuAssertIntEquals(tc, 1, levenshtein_distance("abc", "abcd"));
  CuAssertIntEquals(tc, 1, levenshtein_distance("abcd", "abc"));
  CuAssertIntEquals(tc, 1, levenshtein_distance("abc", "axc"));

  CuAssertIntEquals(tc, 0, count_color_chars("hello"));
  CuAssertIntEquals(tc, 2, count_color_chars("\tRhello"));
  CuAssertIntEquals(tc, 1, count_color_chars("\t\t"));
  CuAssertIntEquals(tc, 5, count_non_protocol_chars("hello"));
  CuAssertIntEquals(tc, 5, count_non_protocol_chars("\r\nhello"));
  CuAssertIntEquals(tc, 2, count_non_protocol_chars("@[bold]hi"));
}

void Test_upstream_index_helpers(CuTest *tc)
{
  CuAssertIntEquals(tc, 42, (int)atoidx("42"));
  CuAssertIntEquals(tc, 0, (int)atoidx("0"));
  CuAssertIntEquals(tc, (int)NOWHERE, (int)atoidx("-1"));
  CuAssertIntEquals(tc, (int)NOWHERE, (int)atoidx("99999999999999999999999999999999"));
}

void Test_blackguard_mount_vnums_are_callable(CuTest *tc)
{
  CuAssertTrue(tc, ok_call_mob_vnum(MOB_BLACKGUARD_MOUNT));
  CuAssertTrue(tc, ok_call_mob_vnum(MOB_ADV_BLACKGUARD_MOUNT));
  CuAssertTrue(tc, ok_call_mob_vnum(MOB_EPIC_BLACKGUARD_MOUNT));
}

void Test_boon_companion_increases_the_rolled_level(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;

  CuAssertIntEquals(tc, 7, test_animal_companion_level(&ch, 7));

  SET_FEAT(&ch, FEAT_BOON_COMPANION, 1);

  CuAssertIntEquals(tc, 12, test_animal_companion_level(&ch, 7));
  CuAssertIntEquals(tc, 20, test_animal_companion_level(&ch, 18));
}

void Test_max_hp_uses_current_object_affect_when_selecting_bonus(CuTest *tc)
{
  struct char_data ch;
  struct descriptor_data descriptor;
  struct player_special_data player_specials;
  struct obj_data stronger_item;
  struct obj_data weaker_item;
  int base_max_hit_points;

  clear_char(&ch);
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&stronger_item, 0, sizeof(stronger_item));
  memset(&weaker_item, 0, sizeof(weaker_item));
  ch.player_specials = &player_specials;
  ch.desc = &descriptor;
  GET_LEVEL(&ch) = 1;
  GET_REAL_RACE(&ch) = RACE_HUMAN;
  GET_REAL_CON(&ch) = 10;
  ch.aff_abils.con = 10;

  calculate_max_hp(&ch, false);
  base_max_hit_points = GET_MAX_HIT(&ch);

  stronger_item.affected[0].location = APPLY_HIT;
  stronger_item.affected[0].modifier = 120;
  stronger_item.affected[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
  weaker_item.affected[0].location = APPLY_STR;
  weaker_item.affected[0].modifier = 200;
  weaker_item.affected[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
  weaker_item.affected[1].location = APPLY_HIT;
  weaker_item.affected[1].modifier = 72;
  weaker_item.affected[1].bonus_type = BONUS_TYPE_ENHANCEMENT;
  GET_EQ(&ch, WEAR_FINGER_R) = &stronger_item;
  GET_EQ(&ch, WEAR_FINGER_L) = &weaker_item;

  calculate_max_hp(&ch, false);

  CuAssertIntEquals(tc, base_max_hit_points + 120, GET_MAX_HIT(&ch));
}

static void assert_undead_respec_preserves_size(CuTest *tc, int race, int class_num,
                                                int original_size)
{
  struct char_data ch;
  struct player_special_data player_specials;
  int actual_size;
  int saved_move_gain;

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  ch.player.name = "undead respec test character";
  GET_REAL_RACE(&ch) = race;
  GET_REAL_SIZE(&ch) = original_size;
  GET_LEVEL(&ch) = 30;
  GET_EXP(&ch) = 1;

  saved_move_gain = class_list[class_num].move_gain;
  class_list[class_num].move_gain = 1;
  respec_engine(&ch, class_num, NULL, TRUE);
  actual_size = GET_REAL_SIZE(&ch);
  class_list[class_num].move_gain = saved_move_gain;

  while (ch.affected != NULL)
    affect_remove_no_total(&ch, ch.affected);
  free(GET_TITLE(&ch));

  CuAssertIntEquals(tc, original_size, actual_size);
}

void Test_undead_respec_preserves_original_size(CuTest *tc)
{
  assert_undead_respec_preserves_size(tc, RACE_LICH, CLASS_WIZARD, SIZE_SMALL);
  assert_undead_respec_preserves_size(tc, RACE_VAMPIRE, CLASS_WARRIOR, SIZE_LARGE);
}

void Test_upstream_new_affect_initializes_all_fields(CuTest *tc)
{
  struct affected_type affect;
  int i;

  memset(&affect, 0xA5, sizeof(affect));
  new_affect(&affect);

  CuAssertIntEquals(tc, 0, affect.spell);
  CuAssertIntEquals(tc, 0, affect.duration);
  CuAssertIntEquals(tc, 0, affect.modifier);
  CuAssertIntEquals(tc, APPLY_NONE, affect.location);
  CuAssertIntEquals(tc, BONUS_TYPE_ENHANCEMENT, affect.bonus_type);
  CuAssertIntEquals(tc, 0, affect.specific);
  CuAssertTrue(tc, affect.next == NULL);
  for (i = 0; i < AF_ARRAY_MAX; i++)
  {
    CuAssertIntEquals(tc, 0, affect.bitvector[i]);
    CuAssertIntEquals(tc, 0, affect.bitvector2[i]);
  }
}

void Test_upstream_time_helpers(CuTest *tc)
{
  struct time_info_data *elapsed;
  time_t base = 1000000;

  elapsed = real_time_passed(base + 3 * SECS_PER_REAL_HOUR, base);
  CuAssertIntEquals(tc, 3, elapsed->hours);
  CuAssertIntEquals(tc, 0, elapsed->day);
  elapsed = real_time_passed(base + 2 * SECS_PER_REAL_DAY + SECS_PER_REAL_HOUR, base);
  CuAssertIntEquals(tc, 1, elapsed->hours);
  CuAssertIntEquals(tc, 2, elapsed->day);

  elapsed = mud_time_passed(base + 2 * SECS_PER_MUD_HOUR, base);
  CuAssertIntEquals(tc, 2, elapsed->hours);
  elapsed = mud_time_passed(base + SECS_PER_MUD_DAY, base);
  CuAssertIntEquals(tc, 1, elapsed->day);
  elapsed = mud_time_passed(base + SECS_PER_MUD_MONTH, base);
  CuAssertIntEquals(tc, 1, elapsed->month);
}

void Test_upstream_script_formatter_nested_blocks(CuTest *tc)
{
  struct descriptor_data descriptor = {0};
  char *script;
  const char *expected;

  script = strdup("switch %mode%\r\n"
                  "case one\r\n"
                  "if %enabled%\r\n"
                  "say yes\r\n"
                  "else\r\n"
                  "say no\r\n"
                  "end\r\n"
                  "break\r\n"
                  "default\r\n"
                  "while %waiting%\r\n"
                  "wait 1 sec\r\n"
                  "done\r\n"
                  "end\r\n");
  CuAssertPtrNotNull(tc, script);
  descriptor.str = &script;
  descriptor.max_str = MAX_CMD_LENGTH;

  CuAssertTrue(tc, format_script(&descriptor));
  expected = "switch %mode%\r\n"
             "  case one\r\n"
             "    if %enabled%\r\n"
             "      say yes\r\n"
             "    else\r\n"
             "      say no\r\n"
             "    end\r\n"
             "    break\r\n"
             "  default\r\n"
             "    while %waiting%\r\n"
             "      wait 1 sec\r\n"
             "    done\r\n"
             "end\r\n";
  CuAssertStrEquals(tc, expected, script);
  free(script);
}
