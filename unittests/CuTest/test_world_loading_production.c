#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/db.h"
#include "../../src/core/constants.h"
#include "../../src/core/comm.h"
#include "../../src/core/handler.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/movement/movement_validation.h"
#include "../../src/obj/shop.h"
#include "../../src/olc/genobj.h"
#include "../../src/olc/genolc.h"
#include "../../src/olc/genshp.h"
#include "../../src/olc/genzon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>


void Test_world_loading_production_real_room_lookup(CuTest *tc)
{
  struct room_data fixture[3];
  struct room_data *saved_world;
  room_rnum saved_top_of_world;

  memset(fixture, 0, sizeof(fixture));
  fixture[0].number = 100;
  fixture[1].number = 200;
  fixture[2].number = 300;

  saved_world = world;
  saved_top_of_world = top_of_world;
  world = fixture;
  top_of_world = 2;

  CuAssertIntEquals(tc, 0, real_room(100));
  CuAssertIntEquals(tc, 1, real_room(200));
  CuAssertIntEquals(tc, 2, real_room(300));
  CuAssertIntEquals(tc, NOWHERE, real_room(250));

  world = saved_world;
  top_of_world = saved_top_of_world;
}

void Test_world_loading_production_real_mobile_and_object_lookup(CuTest *tc)
{
  struct index_data mob_fixture[2];
  struct index_data obj_fixture[2];
  struct index_data *saved_mob_index;
  struct index_data *saved_obj_index;
  mob_rnum saved_top_of_mobt;
  obj_rnum saved_top_of_objt;

  memset(mob_fixture, 0, sizeof(mob_fixture));
  memset(obj_fixture, 0, sizeof(obj_fixture));
  mob_fixture[0].vnum = 101;
  mob_fixture[1].vnum = 303;
  obj_fixture[0].vnum = 202;
  obj_fixture[1].vnum = 404;

  saved_mob_index = mob_index;
  saved_obj_index = obj_index;
  saved_top_of_mobt = top_of_mobt;
  saved_top_of_objt = top_of_objt;
  mob_index = mob_fixture;
  obj_index = obj_fixture;
  top_of_mobt = 1;
  top_of_objt = 1;

  CuAssertIntEquals(tc, 0, real_mobile(101));
  CuAssertIntEquals(tc, 1, real_mobile(303));
  CuAssertIntEquals(tc, NOBODY, real_mobile(999));
  CuAssertIntEquals(tc, 0, real_object(202));
  CuAssertIntEquals(tc, 1, real_object(404));
  CuAssertIntEquals(tc, NOTHING, real_object(999));

  mob_index = saved_mob_index;
  obj_index = saved_obj_index;
  top_of_mobt = saved_top_of_mobt;
  top_of_objt = saved_top_of_objt;
}

void Test_world_loading_production_real_trigger_lookup(CuTest *tc)
{
  struct index_data trigger_data[2];
  struct index_data *trigger_fixture[2];
  struct index_data **saved_trig_index;
  trig_rnum saved_top_of_trigt;

  memset(trigger_data, 0, sizeof(trigger_data));
  trigger_data[0].vnum = 501;
  trigger_data[1].vnum = 777;
  trigger_fixture[0] = &trigger_data[0];
  trigger_fixture[1] = &trigger_data[1];

  saved_trig_index = trig_index;
  saved_top_of_trigt = top_of_trigt;
  trig_index = trigger_fixture;
  top_of_trigt = 2;

  CuAssertIntEquals(tc, 0, real_trigger(501));
  CuAssertIntEquals(tc, 1, real_trigger(777));
  CuAssertIntEquals(tc, NOTHING, real_trigger(999));

  trig_index = saved_trig_index;
  top_of_trigt = saved_top_of_trigt;
}

void Test_world_loading_production_alphabetic_bit_31_does_not_sign_extend(CuTest *tc)
{
  bitvector_t flags;

  flags = asciiflag_conv("F");

  CuAssertTrue(tc, flags == ((bitvector_t)1 << 31));
  CuAssertTrue(tc, (flags >> 32) == 0);
}

void Test_world_loading_production_rol_calendar_predicates(CuTest *tc)
{
  CuAssertTrue(tc, rol_reset_calendar_matches_at(2, 0, 0, 0, 2, 10, 4));
  CuAssertTrue(tc, rol_reset_calendar_matches_at(-1, 11, 0, 5, 9, 10, 4));
  CuAssertTrue(tc, rol_reset_calendar_matches_at(-1, 0, 5, 0, 9, 10, 4));
  CuAssertTrue(tc, !rol_reset_calendar_matches_at(2, 0, 0, 0, 3, 10, 4));
  CuAssertTrue(tc, !rol_reset_calendar_matches_at(-1, 12, 0, 0, 9, 10, 4));
}

void Test_world_loading_production_rol_legacy_door_flags(CuTest *tc)
{
  bitvector_t flags;

  flags = rol_reset_legacy_door_flags(EX_ISDOOR | EX_PICKPROOF | EX_CLOSED, 4);
  CuAssertTrue(tc, IS_SET(flags, EX_ISDOOR));
  CuAssertTrue(tc, IS_SET(flags, EX_PICKPROOF));
  CuAssertTrue(tc, IS_SET(flags, EX_HIDDEN));
  CuAssertTrue(tc, !IS_SET(flags, EX_CLOSED));

  flags = rol_reset_legacy_door_flags(flags, 10);
  CuAssertTrue(tc, IS_SET(flags, EX_CLOSED));
  CuAssertTrue(tc, IS_SET(flags, EX_LOCKED_EASY));
  CuAssertTrue(tc, IS_SET(flags, EX_BLOCKED));
  CuAssertTrue(tc, !IS_SET(flags, EX_HIDDEN));
}

void Test_world_loading_production_rol_reset_mobile_chain(CuTest *tc)
{
  CuAssertTrue(tc, rol_reset_command_ready(true, 'E', false, true));
  CuAssertTrue(tc, rol_reset_command_ready(true, 'G', false, true));
  CuAssertTrue(tc, !rol_reset_command_ready(true, 'E', true, false));
  CuAssertTrue(tc, !rol_reset_command_ready(true, 'G', true, false));
  CuAssertTrue(tc, rol_reset_command_ready(false, 'E', true, false));
  CuAssertTrue(tc, !rol_reset_command_ready(false, 'E', false, true));
  CuAssertTrue(tc, rol_reset_command_ready(true, 'O', true, false));
  CuAssertTrue(tc, !rol_reset_command_ready(true, 'O', false, true));
}

void Test_world_loading_production_global_removal_counts_guarded_and_pending_mobiles(CuTest *tc)
{
  pid_t child;
  int status;

  /* Keep the extraction queue and stack-backed world fixture inside this child. */
  child = fork();
  CuAssertTrue(tc, child >= 0);
  if (child == 0)
  {
    struct char_data characters[7];
    struct char_data *guarded = &characters[0];
    struct char_data *pending = &characters[1];
    struct char_data *first = &characters[2];
    struct char_data *other = &characters[3];
    struct char_data *last = &characters[4];
    struct index_data prototypes[3] = {0};
    int initial_pending = pending_extractions_count();
    int i;

    for (i = 0; i < 7; i++)
    {
      struct char_data *current = &characters[i];

      clear_char(current);
      SET_BIT_AR(MOB_FLAGS(current), MOB_ISNPC);
      GET_MOB_RNUM(current) = i < 3 || i == 4 ? 0 : 1;
      current->next = i < 6 ? &characters[i + 1] : NULL;
    }
    character_list = characters;
    mob_index = prototypes;
    top_of_mobt = 2;
    prototypes[0].number = 4;
    prototypes[1].number = 3;
    FIGHTING(guarded) = other;
    extract_char(pending);

    if (!test_rol_reset_remove_mobile(NOWHERE, 0, true) || MOB_FLAGGED(guarded, MOB_NOTDEADYET) ||
        MOB_FLAGGED(other, MOB_NOTDEADYET) || !MOB_FLAGGED(first, MOB_NOTDEADYET) ||
        !MOB_FLAGGED(last, MOB_NOTDEADYET) || pending_extractions_count() != initial_pending + 3)
      CuTestChildExit(1);
    if (!test_rol_reset_remove_mobile(NOWHERE, 0, false) || !MOB_FLAGGED(guarded, MOB_NOTDEADYET) ||
        pending_extractions_count() != initial_pending + 4)
      CuTestChildExit(2);
    if (!test_rol_reset_remove_mobile(NOWHERE, 0, false) ||
        !test_rol_reset_remove_mobile(NOWHERE, 2, false) ||
        test_rol_reset_remove_mobile(NOWHERE, NOBODY, false) ||
        pending_extractions_count() != initial_pending + 4)
      CuTestChildExit(3);
    CuTestChildExit(0);
  }
  CuAssertTrue(tc, waitpid(child, &status, 0) == child);
  CuAssertTrue(tc, WIFEXITED(status));
  CuAssertIntEquals(tc, 0, WEXITSTATUS(status));
}

void Test_world_loading_production_room_level_entry_contract(CuTest *tc)
{
  struct room_data fixture[1];
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct char_data rider;
  struct char_data mount;

  memset(fixture, 0, sizeof(fixture));
  clear_char(&rider);
  clear_char(&mount);
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = fixture;
  top_of_world = 0;

  GET_LEVEL(&rider) = 14;
  fixture[0].minimum_level = 15;
  fixture[0].maximum_level = -1;
  CuAssertTrue(tc, !room_level_allows_entry(&rider, 0, false));
  GET_LEVEL(&rider) = 15;
  CuAssertTrue(tc, room_level_allows_entry(&rider, 0, false));

  fixture[0].minimum_level = -1;
  fixture[0].maximum_level = 20;
  GET_LEVEL(&rider) = 21;
  CuAssertTrue(tc, !room_level_allows_entry(&rider, 0, false));
  GET_LEVEL(&rider) = LVL_IMMORT;
  CuAssertTrue(tc, room_level_allows_entry(&rider, 0, false));

  GET_LEVEL(&mount) = 21;
  RIDING(&rider) = &mount;
  CuAssertTrue(tc, !room_level_allows_entry(&rider, 0, false));
  RIDING(&rider) = NULL;

  fixture[0].minimum_level = 0;
  fixture[0].maximum_level = 0;
  GET_LEVEL(&rider) = 1;
  CuAssertTrue(tc, room_level_allows_entry(&rider, 0, false));

  world = saved_world;
  top_of_world = saved_top_of_world;
}

void Test_world_loading_production_rol_object_flag_capacity(CuTest *tc)
{
  CuAssertIntEquals(tc, 125, NUM_ITEM_FLAGS);
  CuAssertTrue(tc, NUM_ITEM_FLAGS <= EF_ARRAY_MAX * 32);
}

void Test_world_loading_production_rol_bearer_protection_flags(CuTest *tc)
{
  struct char_data ch;
  struct obj_data carried;
  struct obj_data worn;

  clear_char(&ch);
  clear_object(&carried);
  clear_object(&worn);
  ch.carrying = &carried;
  GET_EQ(&ch, WEAR_BODY) = &worn;
  SET_OBJ_FLAG(&carried, ITEM_ROL_NO_SLEEP);
  SET_OBJ_FLAG(&carried, ITEM_ROL_NO_CHARM);
  SET_OBJ_FLAG(&worn, ITEM_ROL_NO_SUMMON);

  CuAssertTrue(tc, sleep_immunity(&ch));
  CuAssertTrue(tc, char_has_object_flag(&ch, ITEM_ROL_NO_CHARM));
  CuAssertTrue(tc, char_has_worn_object_flag(&ch, ITEM_ROL_NO_SUMMON));
  CuAssertTrue(tc, !char_has_worn_object_flag(&ch, ITEM_ROL_NO_CHARM));
}

void Test_world_loading_production_rol_two_handed_flag_overrides_size(CuTest *tc)
{
  struct char_data ch;
  struct obj_data weapon;

  clear_char(&ch);
  clear_object(&weapon);
  ch.points.size = SIZE_MEDIUM;
  GET_OBJ_SIZE(&weapon) = SIZE_TINY;
  GET_OBJ_TYPE(&weapon) = ITEM_WEAPON;
  SET_OBJ_FLAG(&weapon, ITEM_ROL_TWO_HANDED);

  CuAssertIntEquals(tc, 2, hands_needed_full(&ch, &weapon, FALSE));
  CuAssertTrue(tc, is_weapon_wielded_two_handed(&weapon, &ch));
}

void Test_world_loading_production_rol_race_item_restrictions(CuTest *tc)
{
  struct char_data ch;
  struct obj_data item;

  clear_char(&ch);
  clear_object(&item);
  GET_LEVEL(&ch) = 1;
  ch.player.race = RACE_HUMAN;
  SET_OBJ_FLAG(&item, ITEM_ROL_ANTI_GOOD_RACE);
  CuAssertTrue(tc, invalid_align(&ch, &item));

  ch.player.race = RACE_DROW;
  CuAssertTrue(tc, !invalid_align(&ch, &item));
  REMOVE_OBJ_FLAG(&item, ITEM_ROL_ANTI_GOOD_RACE);
  SET_OBJ_FLAG(&item, ITEM_ROL_ANTI_EVIL_RACE);
  CuAssertTrue(tc, invalid_align(&ch, &item));
}

void Test_world_loading_production_rol_whole_armor_conflicts(CuTest *tc)
{
  struct char_data ch;
  struct obj_data body;
  struct obj_data arms;
  struct obj_data head;
  struct obj_data face;

  clear_char(&ch);
  clear_object(&body);
  clear_object(&arms);
  clear_object(&head);
  clear_object(&face);
  SET_OBJ_FLAG(&body, ITEM_ROL_WHOLE_BODY);
  SET_OBJ_FLAG(&head, ITEM_ROL_WHOLE_HEAD);

  GET_EQ(&ch, WEAR_ARMS) = &arms;
  CuAssertTrue(tc, rol_object_wear_conflicts(&ch, &body, WEAR_BODY));
  GET_EQ(&ch, WEAR_ARMS) = NULL;
  GET_EQ(&ch, WEAR_BODY) = &body;
  CuAssertTrue(tc, rol_object_wear_conflicts(&ch, &arms, WEAR_ARMS));

  GET_EQ(&ch, WEAR_BODY) = NULL;
  GET_EQ(&ch, WEAR_FACE) = &face;
  CuAssertTrue(tc, rol_object_wear_conflicts(&ch, &head, WEAR_HEAD));
  GET_EQ(&ch, WEAR_FACE) = NULL;
  GET_EQ(&ch, WEAR_HEAD) = &head;
  CuAssertTrue(tc, rol_object_wear_conflicts(&ch, &face, WEAR_FACE));
}

/* Fuzz finding: an uppercase affect letter from F on shifted a 32-bit int past
 * its width. The conversion is done in the flag word's own width, so every
 * letter maps to its bit and the numeric form is untouched. */
void Test_world_loading_production_affect_flag_letters_convert_in_flag_width(CuTest *tc)
{
  char lower[] = "az";
  char upper[] = "FZ";
  char numeric[] = "12";

  CuAssertTrue(tc,
               test_asciiflag_conv_aff(lower) == (((bitvector_t)1 << 1) | ((bitvector_t)1 << 26)));
  CuAssertTrue(tc,
               test_asciiflag_conv_aff(upper) == (((bitvector_t)1 << 32) | ((bitvector_t)1 << 52)));
  CuAssertTrue(tc, test_asciiflag_conv_aff(numeric) == 12);
}

/** The production loader exits on malformed input and retains its zone index.
 * Fork fixtures so both behaviors are exercised without contaminating the suite. */
static void assert_world_loader_child(CuTest *tc, pid_t child, int expected_status)
{
  int status;

  CuAssertTrue(tc, child > 0);
  CuAssertTrue(tc, waitpid(child, &status, 0) == child);
  CuAssertTrue(tc, WIFEXITED(status));
  CuAssertIntEquals(tc, expected_status, WEXITSTATUS(status));
}

/** Verify I and both R forms survive whitespace and legacy saved placeholders. */
void Test_world_loading_production_zone_reset_dispatch_and_whitespace(CuTest *tc)
{
  pid_t child;

  child = fork();
  if (child == 0)
  {
    FILE *input = tmpfile();
    struct reset_com *commands;

    if (input == NULL)
      CuTestChildExit(2);
    fputs("#100\nBuilder~\nReset parsing~\n10000 10099 30 2\n"
          "   I 1 75\n"
          "\tI\t0 50 -1 -1 -1 (legacy saved form)\n"
          "  * indented comment\n \t \n"
          " R 0 10000 10001\n"
          "\tR 1 10000 10001 0\n"
          "R 0 10000 10001 100\n"
          "R 0 10000 10001 -1 -1 (legacy saved form)\n"
          " \tS  \n$\n",
          input);
    CuAssertTrue(tc, rewind_stream(input));
    zone_table = calloc(1, sizeof(*zone_table));
    if (zone_table == NULL)
      CuTestChildExit(2);
    test_load_zones(input, CuMutableString("reset-fixture.zon"));
    commands = zone_table[0].cmd;
    if (top_of_zone_table != 0 || commands[0].command != 'I' || commands[0].if_flag != 1 ||
        commands[0].arg1 != 75 || commands[0].line != 5 || commands[1].command != 'I' ||
        commands[1].if_flag != 0 || commands[1].arg1 != 50 || commands[1].line != 6)
      CuTestChildExit(3);
    if (commands[2].command != 'R' || commands[2].arg1 != 10000 || commands[2].arg2 != 10001 ||
        commands[2].arg3 != 100 || commands[2].arg4 != 0 || commands[2].line != 9 ||
        commands[3].if_flag != 1 || commands[3].arg3 != 0 || commands[3].arg4 != 1 ||
        commands[4].arg3 != 100 || commands[4].arg4 != 1 || commands[5].arg3 != 100 ||
        commands[5].arg4 != 0 || commands[6].command != 'S')
      CuTestChildExit(4);
    CuTestChildExit(0);
  }
  assert_world_loader_child(tc, child, 0);
}

/** Check supported header defaults and physical-line warnings for ignored suffixes. */
void Test_world_loading_production_zone_header_forms_and_diagnostics(CuTest *tc)
{
  pid_t child;

  child = fork();
  if (child == 0)
  {
    const int values[] = {10000, 10099, 30, 2, 1, 2, 4, 8, 3, 20, 0, 7, 8, 9, 999};
    int count, i, used;
    FILE *input, *capture;
    struct zone_data *zone;
    char output[READ_SIZE], expected[READ_SIZE];
    size_t length;

    zone_table = calloc(12, sizeof(*zone_table));
    if (zone_table == NULL)
      CuTestChildExit(2);
    for (count = 4; count <= 15; count++)
    {
      input = tmpfile();
      capture = tmpfile();
      if (input == NULL || capture == NULL)
        CuTestChildExit(2);
      logfile = capture;
      fputs("#100\nBuilder~\nHeader parsing~\n* comment\n\n", input);
      for (i = 0; i < count; i++)
        fprintf(input, "%d ", values[i]);
      fputs("\nS\n$\n", input);
      CuAssertTrue(tc, rewind_stream(input));
      test_load_zones(input, CuMutableString("header-fixture.zon"));
      zone = &zone_table[count - 4];
      used = count >= 14 ? 14 : count >= 11 ? 11 : count >= 10 ? 10 : 4;
      if (zone->bot != 10000 || zone->top != 10099 || zone->lifespan != 30 ||
          zone->reset_mode != 2 || zone->cmd[0].command != 'S' ||
          zone->min_level != (used >= 10 ? 3 : -1) || zone->max_level != (used >= 10 ? 20 : -1) ||
          zone->show_weather != (used >= 11 ? 0 : 1) || zone->region != (used == 14 ? 7 : 0) ||
          zone->faction != (used == 14 ? 8 : 0) || zone->city != (used == 14 ? 9 : 0))
        CuTestChildExit(count + 10);
      for (i = 0; i < ZN_ARRAY_MAX; i++)
        if (zone->zone_flags[i] != (used >= 10 ? values[i + 4] : 0))
          CuTestChildExit(count + 30);
      CuAssertTrue(tc, rewind_stream(capture));
      length = fread(output, 1, sizeof(output) - 1, capture);
      output[length] = '\0';
      if (used == count && length != 0)
        CuTestChildExit(count + 50);
      if (used != count)
      {
        snprintf(
            expected, sizeof(expected),
            "ZONE WARNING: Zone #100, header-fixture.zon, line 6: numeric header uses %d fields;",
            used);
        if (strstr(output, expected) == NULL || strstr(output, "ignoring trailing data:") == NULL)
          CuTestChildExit(count + 70);
      }
      fclose(capture);
      fclose(input);
    }
    CuTestChildExit(0);
  }
  assert_world_loader_child(tc, child, 0);
}

/** Keep the first reset and its line number when repairing a missing builder line. */
void Test_world_loading_production_zone_without_builder_preserves_first_reset(CuTest *tc)
{
  pid_t child;

  child = fork();
  if (child == 0)
  {
    FILE *input = tmpfile();

    if (input == NULL)
      CuTestChildExit(2);
    fputs("#100\nLegacy zone~\n10000 10099 30 2\nI 0 100\nS\n$\n", input);
    CuAssertTrue(tc, rewind_stream(input));
    zone_table = calloc(1, sizeof(*zone_table));
    if (zone_table == NULL)
      CuTestChildExit(2);
    test_load_zones(input, CuMutableString("legacy-fixture.zon"));
    if (strcmp(zone_table[0].name, "Legacy zone") != 0 ||
        strcmp(zone_table[0].builders, "None.") != 0 || zone_table[0].cmd[0].command != 'I' ||
        zone_table[0].cmd[0].line != 4 || zone_table[0].cmd[1].command != 'S')
      CuTestChildExit(3);
    CuTestChildExit(0);
  }
  assert_world_loader_child(tc, child, 0);
}

/** Reject the removed L reset with the source filename and physical line. */
void Test_world_loading_production_unsupported_zone_reset_reports_line(CuTest *tc)
{
  pid_t child;
  FILE *capture;
  char output[READ_SIZE];
  size_t length;

  capture = tmpfile();
  CuAssertPtrNotNull(tc, capture);
  child = fork();
  if (child == 0)
  {
    FILE *input = tmpfile();

    if (input == NULL)
      CuTestChildExit(2);
    logfile = capture;
    fputs("#100\nBuilder~\nUnsupported reset~\n10000 10099 30 2\n"
          " L 0 10001 50\nS\n$\n",
          input);
    CuAssertTrue(tc, rewind_stream(input));
    zone_table = calloc(1, sizeof(*zone_table));
    if (zone_table == NULL)
      CuTestChildExit(2);
    test_load_zones(input, CuMutableString("unsupported-fixture.zon"));
    CuTestChildExit(0);
  }
  assert_world_loader_child(tc, child, 1);
  CuAssertTrue(tc, rewind_stream(capture));
  length = fread(output, 1, sizeof(output) - 1, capture);
  output[length] = '\0';
  fclose(capture);
  CuAssertPtrNotNull(
      tc, strstr(output, "Unknown zone reset command 'L' in unsupported-fixture.zon, line 5"));
}

/** Report each missing exit destination while preserving valid and NOWHERE exits. */
void Test_world_loading_production_exit_diagnostics(CuTest *tc)
{
  struct room_data rooms[2] = {0};
  struct room_direction_data exits[NUM_OF_DIRS] = {0};
  struct room_data *saved_world = world;
  room_rnum saved_top = top_of_world;
  FILE *saved_logfile = logfile;
  FILE *capture = tmpfile();
  char output[MAX_STRING_LENGTH], expected[READ_SIZE];
  size_t length;
  int direction;

  CuAssertPtrNotNull(tc, capture);
  rooms[0].number = 100;
  rooms[1].number = 200;
  for (direction = 0; direction < NUM_OF_DIRS; direction++)
  {
    rooms[0].dir_option[direction] = &exits[direction];
    exits[direction].to_room = 900 + direction;
  }
  exits[NORTH].to_room = 200;
  exits[EAST].to_room = NOWHERE;
  world = rooms;
  top_of_world = 1;
  logfile = capture;
  renum_world();
  world = saved_world;
  top_of_world = saved_top;
  logfile = saved_logfile;
  CuAssertTrue(tc, rewind_stream(capture));
  length = fread(output, 1, sizeof(output) - 1, capture);
  output[length] = '\0';
  fclose(capture);

  CuAssertIntEquals(tc, 1, exits[NORTH].to_room);
  CuAssertIntEquals(tc, NOWHERE, exits[EAST].to_room);
  CuAssertTrue(tc, strstr(output, "exit north (") == NULL);
  CuAssertTrue(tc, strstr(output, "exit east (") == NULL);
  for (direction = SOUTH; direction < NUM_OF_DIRS; direction++)
  {
    CuAssertIntEquals(tc, NOWHERE, exits[direction].to_room);
    snprintf(expected, sizeof(expected), "Room #100, exit %s (%d): destination #%d",
             dirs[direction], direction, 900 + direction);
    CuAssertPtrNotNull(tc, strstr(output, expected));
  }
}

/* Every file the zone and shop round trip creates inside its sandbox. */
static const char *const olc_round_trip_files[] = {"world/zon/1.zon",
                                                   "world/zon/1.new",
                                                   "world/zon/index",
                                                   "world/zon/newindex",
                                                   "world/shp/1.shp",
                                                   "world/shp/1.new",
                                                   "world/shp/index",
                                                   "world/shp/newindex",
                                                   "world/zon",
                                                   "world/shp",
                                                   "world"};

static bool olc_round_trip_write_index(const char *path)
{
  FILE *index = fopen(path, "w");

  if (index == NULL)
    return false;
  fputs("$\n", index);
  return fclose(index) == 0;
}

static bool olc_command_is(const struct reset_com *command, char letter, int if_flag, int arg1,
                           int arg2, int arg3)
{
  return command->command == letter && command->if_flag == if_flag && command->arg1 == arg1 &&
         command->arg2 == arg2 && command->arg3 == arg3;
}

/* Runs in a child: builds one zone and one shop in memory, saves both with the
 * OLC writers, reloads the files with the boot loaders into empty tables, and
 * returns 0 when every field survived or the number of the first difference. */
static int olc_round_trip_child(const char *sandbox)
{
  static struct room_data rooms[2];
  static struct index_data mobiles[1];
  static struct index_data objects[1];
  static struct char_data mobile_prototypes[1];
  static struct obj_data object_prototypes[1];
  static struct zone_data zones[1];
  static struct reset_com commands[] = {{'M', 0, 0, 1, 0, 100, 0, NULL, NULL},
                                        {'G', 1, 0, 5, 75, -1, 0, NULL, NULL},
                                        {'E', 1, 0, 5, WEAR_WIELD_1, 60, 0, NULL, NULL},
                                        {'O', 0, 0, 2, 1, 40, 0, NULL, NULL},
                                        {'P', 1, 0, 3, 0, 20, 0, NULL, NULL},
                                        {'D', 0, 0, 0, 1, -1, 0, NULL, NULL},
                                        {'R', 0, 1, 0, 50, 1, 0, NULL, NULL},
                                        {'S', 0, 0, 0, 0, 0, 0, NULL, NULL}};
  static obj_vnum products[] = {0, NOTHING};
  static struct shop_buy_data trades[2];
  static room_vnum shop_rooms[] = {100, NOWHERE};
  static struct shop_data shops[1];
  const struct reset_com *loaded;
  const struct shop_data *shop;
  FILE *file;

  rooms[0].number = 100;
  rooms[0].name = CuMutableString("a round trip hall");
  rooms[1].number = 101;
  rooms[1].name = CuMutableString("a round trip vault");
  world = rooms;
  top_of_world = 1;
  mobiles[0].vnum = 100;
  mob_index = mobiles;
  top_of_mobt = 0;
  mobile_prototypes[0].player.short_descr = CuMutableString("a round trip keeper");
  mob_proto = mobile_prototypes;
  objects[0].vnum = 100;
  obj_index = objects;
  top_of_objt = 0;
  object_prototypes[0].short_description = CuMutableString("a round trip sword");
  obj_proto = object_prototypes;

  zones[0].number = 1;
  zones[0].bot = 100;
  zones[0].top = 199;
  zones[0].name = CuMutableString("Round Trip");
  zones[0].builders = CuMutableString("Builder");
  zones[0].lifespan = 30;
  zones[0].reset_mode = 2;
  zones[0].min_level = -1;
  zones[0].max_level = -1;
  zones[0].cmd = commands;
  zone_table = zones;
  top_of_zone_table = 0;

  trades[0].type = ITEM_WEAPON;
  trades[0].keywords = CuMutableString("sword");
  trades[1].type = (int)NOTHING;
  shops[0].vnum = 100;
  shops[0].producing = products;
  shops[0].profit_buy = 1.25;
  shops[0].profit_sell = 0.75;
  shops[0].type = trades;
  shops[0].no_such_item1 = CuMutableString("%s I have none of those.");
  shops[0].no_such_item2 = CuMutableString("%s You have none of those.");
  shops[0].do_not_buy = CuMutableString("%s I do not buy that.");
  shops[0].missing_cash1 = CuMutableString("%s I cannot afford it.");
  shops[0].missing_cash2 = CuMutableString("%s You cannot afford it.");
  shops[0].message_buy = CuMutableString("%s That costs %d coins.");
  shops[0].message_sell = CuMutableString("%s I pay %d coins.");
  shops[0].temper1 = 1;
  shops[0].bitvector = 2;
  shops[0].keeper = 0;
  shops[0].with_who = 4;
  shops[0].in_room = shop_rooms;
  shops[0].open1 = 6;
  shops[0].close1 = 20;
  shops[0].open2 = 21;
  shops[0].close2 = 28;
  shops[0].rol_cheat_with = 3;
  shop_index = shops;
  top_shop = 0;

  if (chdir(sandbox) != 0 || mkdir("world", 0700) != 0 || mkdir("world/zon", 0700) != 0 ||
      mkdir("world/shp", 0700) != 0 || !olc_round_trip_write_index("world/zon/index") ||
      !olc_round_trip_write_index("world/shp/index"))
    return 2;
  if (!save_zone(0))
    return 3;
  if (!save_shops(0))
    return 4;

  zone_table = calloc(1, sizeof(*zone_table));
  if (zone_table == NULL)
    return 5;
  file = fopen("world/zon/1.zon", "r");
  if (file == NULL)
    return 5;
  test_load_zones(file, CuMutableString("1.zon"));
  fclose(file);
  if (zone_table[0].number != 1 || zone_table[0].bot != 100 || zone_table[0].top != 199 ||
      strcmp(zone_table[0].name, "Round Trip") != 0 ||
      strcmp(zone_table[0].builders, "Builder") != 0 || zone_table[0].lifespan != 30 ||
      zone_table[0].reset_mode != 2 || zone_table[0].min_level != -1)
    return 6;
  /* The loader keeps virtual numbers until the zone table is renumbered. */
  loaded = zone_table[0].cmd;
  if (!olc_command_is(&loaded[0], 'M', 0, 100, 1, 100) || loaded[0].arg4 != 100 ||
      !olc_command_is(&loaded[1], 'G', 1, 100, 5, 75) ||
      !olc_command_is(&loaded[2], 'E', 1, 100, 5, WEAR_WIELD_1) || loaded[2].arg4 != 60 ||
      !olc_command_is(&loaded[3], 'O', 0, 100, 2, 101) || loaded[3].arg4 != 40 ||
      !olc_command_is(&loaded[4], 'P', 1, 100, 3, 100) || loaded[4].arg4 != 20 ||
      !olc_command_is(&loaded[5], 'D', 0, 100, 0, 1) ||
      !olc_command_is(&loaded[6], 'R', 0, 101, 100, 50) || loaded[6].arg4 != 1 ||
      loaded[7].command != 'S')
    return 7;

  shop_index = NULL;
  top_shop = -1;
  file = fopen("world/shp/1.shp", "r");
  if (file == NULL)
    return 8;
  boot_the_shops(file, CuMutableString("1.shp"), 1);
  fclose(file);
  if (top_shop != 0)
    return 9;
  shop = &shop_index[0];
  if (shop->vnum != 100 || shop->producing[0] != 0 || shop->producing[1] != NOTHING ||
      shop->profit_buy < 1.2499 || shop->profit_buy > 1.2501 || shop->profit_sell < 0.7499 ||
      shop->profit_sell > 0.7501)
    return 10;
  if (shop->type[0].type != ITEM_WEAPON || shop->type[0].keywords == NULL ||
      strcmp(shop->type[0].keywords, "sword") != 0 || shop->type[1].type != (int)NOTHING)
    return 11;
  if (strcmp(shop->no_such_item1, "%s I have none of those.") != 0 ||
      strcmp(shop->message_buy, "%s That costs %d coins.") != 0 ||
      strcmp(shop->message_sell, "%s I pay %d coins.") != 0)
    return 12;
  if (shop->temper1 != 1 || shop->bitvector != 2 || shop->keeper != 0 || shop->with_who != 4 ||
      shop->in_room[0] != 100 || shop->in_room[1] != NOWHERE || shop->open1 != 6 ||
      shop->close1 != 20 || shop->open2 != 21 || shop->close2 != 28 || shop->rol_cheat_with != 3)
    return 13;
  return 0;
}

/** The OLC zone and shop writers produce files the boot loaders read back to
 * the same zone header, reset commands, and shop definition. */
void Test_olc_zone_and_shop_files_round_trip_through_the_loaders(CuTest *tc)
{
  char sandbox[] = "/tmp/luminari-olc-round-trip.XXXXXX";
  char path[sizeof(sandbox) + 32];
  size_t i;
  pid_t child = -1;
  int status = -1;
  bool created;

  created = mkdtemp(sandbox) != NULL;
  if (created)
  {
    child = fork();
    if (child == 0)
      CuTestChildExit(olc_round_trip_child(sandbox));
    if (child > 0 && waitpid(child, &status, 0) != child)
      status = -1;
    for (i = 0; i < sizeof(olc_round_trip_files) / sizeof(olc_round_trip_files[0]); i++)
    {
      snprintf(path, sizeof(path), "%s/%s", sandbox, olc_round_trip_files[i]);
      if (unlink(path) != 0)
        rmdir(path);
    }
    rmdir(sandbox);
  }

  CuAssertTrue(tc, created);
  CuAssertTrue(tc, child > 0);
  CuAssertTrue(tc, WIFEXITED(status));
  CuAssertIntEquals(tc, 0, WEXITSTATUS(status));
}

void Test_world_loading_production_mob_path_espec_stops_at_array_bound(CuTest *tc)
{
  struct char_data prototype;
  struct char_data *saved_mob_proto;
  struct index_data prototype_index;
  struct index_data *saved_mob_index;
  mob_rnum saved_top_of_mobt;
  char value[1024];
  size_t used;
  int room;

  clear_char(&prototype);
  memset(&prototype_index, 0, sizeof(prototype_index));
  saved_mob_proto = mob_proto;
  saved_mob_index = mob_index;
  saved_top_of_mobt = top_of_mobt;
  mob_proto = &prototype;
  mob_index = &prototype_index;
  top_of_mobt = 0;
  prototype_index.vnum = 1234;
  prototype.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_ISNPC);

  /* A path line listing more rooms than the prototype array holds. */
  used = (size_t)snprintf(value, sizeof(value), "10:");
  for (room = 1; room <= MAX_PATH + 5; room++)
  {
    used += (size_t)snprintf(value + used, sizeof(value) - used, "%d ", 100 + room);
  }
  CuAssertTrue(tc, used < sizeof(value));

  test_interpret_mobile_espec("Path", value, 0, 1234);

  CuAssertIntEquals(tc, MAX_PATH, PATH_SIZE(&prototype));
  CuAssertIntEquals(tc, 10, PATH_RESET(&prototype));
  CuAssertIntEquals(tc, 101, GET_PATH(&prototype, 0));
  CuAssertIntEquals(tc, 100 + MAX_PATH, GET_PATH(&prototype, MAX_PATH - 1));

  /* A path that fits is still stored in full. */
  test_interpret_mobile_espec("Path", "5:201 202 203", 0, 1234);
  CuAssertIntEquals(tc, 3, PATH_SIZE(&prototype));
  CuAssertIntEquals(tc, 203, GET_PATH(&prototype, 2));

  mob_proto = saved_mob_proto;
  mob_index = saved_mob_index;
  top_of_mobt = saved_top_of_mobt;
}

void Test_oset_apply_parses_the_modifier_and_rejects_zero(CuTest *tc)
{
  struct obj_data obj;

  memset(&obj, 0, sizeof(obj));
  CuAssertTrue(tc, !oset_apply(&obj, "strength 0"));
  CuAssertTrue(tc, !oset_apply(&obj, "strength"));
  CuAssertTrue(tc, oset_apply(&obj, "strength 3"));
  CuAssertIntEquals(tc, APPLY_STR, obj.affected[0].location);
  CuAssertIntEquals(tc, 3, obj.affected[0].modifier);
  CuAssertTrue(tc, !oset_apply(&obj, "nosuchapply 3"));
}

/* Both export commands parse the zone number before looking it up. */
void Test_export_commands_reject_an_unknown_zone(CuTest *tc)
{
  struct char_data *ch = new_char();
  struct zone_data *saved_zone_table = zone_table;
  zone_rnum saved_top = top_of_zone_table;
  struct zone_data only_zone;

  /* A one-zone table so real_zone() has something to search. */
  memset(&only_zone, 0, sizeof(only_zone));
  only_zone.number = 1;
  zone_table = &only_zone;
  top_of_zone_table = 0;

  GET_LEVEL(ch) = LVL_IMPL;
  do_export_zone(ch, "999999", 0, 0);
  do_export_map(ch, "999999 map.html", 0, 0);
  do_export_zone(ch, "", 0, 0);
  do_export_map(ch, "", 0, 0);
  CuAssertIntEquals(tc, NOWHERE, real_zone(999999));

  zone_table = saved_zone_table;
  top_of_zone_table = saved_top;
  free_char(ch);
}
