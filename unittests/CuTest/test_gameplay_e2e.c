#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/db.h"
#include "../../src/dg_scripts.h"
#include "../../src/fight.h"
#include "../../src/genwld.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/movement.h"
#include "../../src/spells.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct gameplay_fixture
{
  struct room_data rooms[2];
  struct room_direction_data exits[2];
  struct zone_data zones[1];
  struct index_data mobile_index[1];
  struct char_data actor;
  struct char_data victim;

  struct room_data *saved_world;
  struct zone_data *saved_zone_table;
  struct index_data *saved_mob_index;
  room_rnum saved_top_of_world;
  zone_rnum saved_top_of_zone_table;
  mob_rnum saved_top_of_mobt;
};

static const char *test_source_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_ROOT");
  return root != NULL && *root != '\0' ? root : ".";
}

static void initialize_test_npc(struct char_data *ch, const char *name, room_rnum room)
{
  clear_char(ch);
  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &dummy_mob;
  ch->player.short_descr = (char *)name;
  GET_LEVEL(ch) = 10;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100;
  GET_MAX_HIT(ch) = 100;
  GET_MOVE(ch) = 100;
  GET_MAX_MOVE(ch) = 100;
  IN_ROOM(ch) = room;
}

static void begin_gameplay_fixture(struct gameplay_fixture *fixture)
{
  memset(fixture, 0, sizeof(*fixture));

  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_zone_table = zone_table;
  fixture->saved_top_of_zone_table = top_of_zone_table;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;

  fixture->rooms[0].number = 100;
  fixture->rooms[0].zone = 0;
  fixture->rooms[0].sector_type = SECT_INSIDE;
  fixture->rooms[0].name = "End-to-end origin";
  fixture->rooms[0].description = "A production-linked test room.\r\n";
  fixture->rooms[1].number = 101;
  fixture->rooms[1].zone = 0;
  fixture->rooms[1].sector_type = SECT_INSIDE;
  fixture->rooms[1].name = "End-to-end destination";
  fixture->rooms[1].description = "A second production-linked test room.\r\n";
  fixture->rooms[0].trail_tracks = calloc(1, sizeof(*fixture->rooms[0].trail_tracks));
  fixture->rooms[1].trail_tracks = calloc(1, sizeof(*fixture->rooms[1].trail_tracks));

  fixture->exits[0].key = NOTHING;
  fixture->exits[0].to_room = 1;
  fixture->exits[1].key = NOTHING;
  fixture->exits[1].to_room = 0;
  fixture->rooms[0].dir_option[NORTH] = &fixture->exits[0];
  fixture->rooms[1].dir_option[SOUTH] = &fixture->exits[1];

  fixture->zones[0].number = 0;
  fixture->zones[0].bot = 100;
  fixture->zones[0].top = 101;
  fixture->zones[0].min_level = -1;
  fixture->zones[0].max_level = LVL_IMPL;

  fixture->mobile_index[0].vnum = 1;

  world = fixture->rooms;
  top_of_world = 1;
  zone_table = fixture->zones;
  top_of_zone_table = 0;
  mob_index = fixture->mobile_index;
  top_of_mobt = 0;

  initialize_test_npc(&fixture->actor, "fixture actor", 0);
  initialize_test_npc(&fixture->victim, "fixture victim", 0);
  fixture->rooms[0].people = &fixture->actor;
  fixture->actor.next_in_room = &fixture->victim;
}

static void end_gameplay_fixture(struct gameplay_fixture *fixture)
{
  FIGHTING(&fixture->actor) = NULL;
  FIGHTING(&fixture->victim) = NULL;
  fixture->actor.last_attacker = NULL;
  fixture->victim.last_attacker = NULL;
  fixture->actor.next_in_room = NULL;
  fixture->victim.next_in_room = NULL;
  fixture->rooms[0].people = NULL;
  fixture->rooms[1].people = NULL;
  if (fixture->rooms[0].trail_tracks != NULL)
    free_trail_data_list(fixture->rooms[0].trail_tracks);
  if (fixture->rooms[1].trail_tracks != NULL)
    free_trail_data_list(fixture->rooms[1].trail_tracks);

  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  zone_table = fixture->saved_zone_table;
  top_of_zone_table = fixture->saved_top_of_zone_table;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
}

void Test_gameplay_e2e_combat_applies_real_damage(CuTest *tc)
{
  struct gameplay_fixture fixture;
  int damage_result;
  int remaining_hit_points;

  begin_gameplay_fixture(&fixture);
  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;

  damage_result = damage(&fixture.actor, &fixture.victim, 12, TYPE_HIT, DAM_BLUDGEON, FALSE);
  remaining_hit_points = GET_HIT(&fixture.victim);

  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, damage_result > 0);
  CuAssertTrue(tc, remaining_hit_points < 100);
  CuAssertTrue(tc, remaining_hit_points > 0);
}

void Test_gameplay_e2e_casting_dispatches_magic_missile(CuTest *tc)
{
  struct gameplay_fixture fixture;
  int cast_result;
  int remaining_hit_points;

  begin_gameplay_fixture(&fixture);
  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_UNLIMITED_SPELL_SLOTS);
  mag_assign_spells();

  cast_result =
      call_magic(&fixture.actor, &fixture.victim, NULL, SPELL_MAGIC_MISSILE, 0, 10, CAST_INNATE);
  remaining_hit_points = GET_HIT(&fixture.victim);

  end_gameplay_fixture(&fixture);

  CuAssertIntEquals(tc, 1, cast_result);
  CuAssertTrue(tc, remaining_hit_points < 100);
  CuAssertTrue(tc, remaining_hit_points > 0);
}

void Test_gameplay_e2e_movement_changes_room(CuTest *tc)
{
  struct gameplay_fixture fixture;
  int move_result;
  room_rnum destination;

  begin_gameplay_fixture(&fixture);

  move_result = perform_move(&fixture.actor, NORTH, FALSE);
  destination = IN_ROOM(&fixture.actor);

  end_gameplay_fixture(&fixture);

  CuAssertIntEquals(tc, 1, move_result);
  CuAssertIntEquals(tc, 1, destination);
}

void Test_gameplay_e2e_command_dispatch_reaches_movement(CuTest *tc)
{
  struct gameplay_fixture fixture;
  bool created_command_list;
  room_rnum destination;
  char command[] = "north";

  begin_gameplay_fixture(&fixture);
  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  command_interpreter(&fixture.actor, command);
  destination = IN_ROOM(&fixture.actor);

  if (created_command_list)
    free_command_list();
  end_gameplay_fixture(&fixture);

  CuAssertIntEquals(tc, 1, destination);
}

void Test_gameplay_e2e_player_file_round_trip(CuTest *tc)
{
  struct player_index_element fixture_index[1];
  struct player_index_element *saved_player_table;
  struct char_data *source;
  struct char_data *loaded;
  int saved_top_of_p_table;
  int load_result;
  int loaded_level;
  int loaded_gold;
  int restore_result;
  bool loaded_name_matches;
  bool changed_directory;
  bool filename_ready;
  char original_directory[PATH_MAX];
  char lib_directory[PATH_MAX];
  char filename[MAX_FILEPATH];
  char player_name[32];

  memset(fixture_index, 0, sizeof(fixture_index));
  memset(filename, 0, sizeof(filename));
  source = new_char();
  loaded = new_char();
  snprintf(player_name, sizeof(player_name), "Zzct%ld", (long)getpid());

  fixture_index[0].name = player_name;
  fixture_index[0].id = 4242;
  fixture_index[0].level = 7;
  fixture_index[0].last = 0;

  saved_player_table = player_table;
  saved_top_of_p_table = top_of_p_table;
  player_table = fixture_index;
  top_of_p_table = 0;

  source->player.name = strdup(player_name);
  GET_PFILEPOS(source) = 0;
  GET_IDNUM(source) = 4242;
  GET_LEVEL(source) = 7;
  GET_GOLD(source) = 12345;
  source->player.time.logon = 0;

  changed_directory = false;
  filename_ready = false;
  load_result = -1;
  loaded_level = -1;
  loaded_gold = -1;
  loaded_name_matches = false;
  restore_result = 0;

  if (getcwd(original_directory, sizeof(original_directory)) != NULL &&
      snprintf(lib_directory, sizeof(lib_directory), "%s/lib", test_source_root()) <
          (int)sizeof(lib_directory) &&
      chdir(lib_directory) == 0)
  {
    changed_directory = true;
    filename_ready = get_filename(filename, sizeof(filename), PLR_FILE, player_name);
    if (filename_ready)
    {
      save_char(source, TRUE);
      load_result = load_char(player_name, loaded);
      if (load_result >= 0)
      {
        loaded_level = GET_LEVEL(loaded);
        loaded_gold = GET_GOLD(loaded);
        loaded_name_matches =
            GET_NAME(loaded) != NULL && strcmp(GET_NAME(loaded), player_name) == 0;
      }
      unlink(filename);
    }
  }

  if (changed_directory)
    restore_result = chdir(original_directory);

  free_char(loaded);
  free_char(source);
  player_table = saved_player_table;
  top_of_p_table = saved_top_of_p_table;

  CuAssertTrue(tc, changed_directory);
  CuAssertIntEquals(tc, 0, restore_result);
  CuAssertTrue(tc, filename_ready);
  CuAssertTrue(tc, load_result >= 0);
  CuAssertTrue(tc, loaded_name_matches);
  CuAssertIntEquals(tc, 7, loaded_level);
  CuAssertIntEquals(tc, 12345, loaded_gold);
}

void Test_gameplay_e2e_dg_trigger_parse_and_execute(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct index_data **saved_trig_index;
  struct trig_data *saved_trigger_list;
  struct index_data *prototype_index;
  struct cmdlist_element *commands;
  struct cmdlist_element *next_command;
  struct trig_var_data *variable;
  int saved_top_of_trigt;
  int trigger_result;
  bool parsed;
  bool result_matches;
  FILE *trigger_file;

  begin_gameplay_fixture(&fixture);
  saved_trig_index = trig_index;
  saved_top_of_trigt = top_of_trigt;
  saved_trigger_list = trigger_list;
  trig_index = calloc(1, sizeof(*trig_index));
  top_of_trigt = 0;
  trigger_file = tmpfile();
  parsed = false;
  result_matches = false;
  trigger_result = 0;

  if (trig_index != NULL && trigger_file != NULL)
  {
    fprintf(trigger_file, "End-to-end command trigger~\n");
    fprintf(trigger_file, "2 c 100\n");
    fprintf(trigger_file, "probe~\n");
    fprintf(trigger_file, "set result 42\n");
    fprintf(trigger_file, "global result\n");
    fprintf(trigger_file, "~\n");
    rewind(trigger_file);

    parse_trigger(trigger_file, 9000);
    parsed = top_of_trigt == 1 && trig_index[0] != NULL;
    if (parsed)
    {
      fixture.rooms[0].script = calloc(1, sizeof(*fixture.rooms[0].script));
      if (fixture.rooms[0].script != NULL)
      {
        add_trigger(fixture.rooms[0].script, read_trigger(0), -1);
        trigger_result = command_wtrigger(&fixture.actor, "probe", "");
        for (variable = fixture.rooms[0].script->global_vars; variable != NULL;
             variable = variable->next)
        {
          if (strcmp(variable->name, "result") == 0 && strcmp(variable->value, "42") == 0)
          {
            result_matches = true;
            break;
          }
        }
        extract_script(&fixture.rooms[0], WLD_TRIGGER);
      }
    }
  }

  if (trigger_file != NULL)
    fclose(trigger_file);

  prototype_index = parsed ? trig_index[0] : NULL;
  if (prototype_index != NULL)
  {
    commands = ((struct trig_data *)prototype_index->proto)->cmdlist;
    free_trigger((struct trig_data *)prototype_index->proto);
    while (commands != NULL)
    {
      next_command = commands->next;
      free(commands->cmd);
      free(commands);
      commands = next_command;
    }
    free(prototype_index);
  }
  free(trig_index);
  trig_index = saved_trig_index;
  top_of_trigt = saved_top_of_trigt;
  trigger_list = saved_trigger_list;
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, parsed);
  CuAssertIntEquals(tc, 1, trigger_result);
  CuAssertTrue(tc, result_matches);
}

void Test_gameplay_e2e_damage_trigger_overrides_damage(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct index_data **saved_trig_index;
  struct trig_data *saved_trigger_list;
  struct index_data *prototype_index;
  struct cmdlist_element *commands;
  struct cmdlist_element *next_command;
  struct trig_var_data *variable;
  int saved_top_of_trigt;
  int damage_result;
  int remaining_hit_points;
  bool parsed;
  bool damage_variable_matches;
  FILE *trigger_file;

  begin_gameplay_fixture(&fixture);
  saved_trig_index = trig_index;
  saved_top_of_trigt = top_of_trigt;
  saved_trigger_list = trigger_list;
  trig_index = calloc(1, sizeof(*trig_index));
  top_of_trigt = 0;
  trigger_file = tmpfile();
  parsed = false;
  damage_variable_matches = false;
  damage_result = -1;
  remaining_hit_points = GET_HIT(&fixture.victim);

  if (trig_index != NULL && trigger_file != NULL)
  {
    fprintf(trigger_file, "End-to-end damage trigger~\n");
    fprintf(trigger_file, "0 u 100\n");
    fprintf(trigger_file, "~\n");
    fprintf(trigger_file, "set seen_damage %%damage%%\n");
    fprintf(trigger_file, "global seen_damage\n");
    fprintf(trigger_file, "return 5\n");
    fprintf(trigger_file, "~\n");
    rewind(trigger_file);

    parse_trigger(trigger_file, 9001);
    parsed = top_of_trigt == 1 && trig_index[0] != NULL;
    if (parsed)
    {
      fixture.victim.script = calloc(1, sizeof(*fixture.victim.script));
      if (fixture.victim.script != NULL)
      {
        add_trigger(fixture.victim.script, read_trigger(0), -1);
        FIGHTING(&fixture.actor) = &fixture.victim;
        FIGHTING(&fixture.victim) = &fixture.actor;
        damage_result = damage(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON, FALSE);
        remaining_hit_points = GET_HIT(&fixture.victim);
        for (variable = fixture.victim.script->global_vars; variable != NULL;
             variable = variable->next)
        {
          if (strcmp(variable->name, "seen_damage") == 0 && strcmp(variable->value, "17") == 0)
          {
            damage_variable_matches = true;
            break;
          }
        }
        extract_script(&fixture.victim, MOB_TRIGGER);
      }
    }
  }

  if (GET_ID(&fixture.actor) != 0)
    remove_from_lookup_table(GET_ID(&fixture.actor));
  if (GET_ID(&fixture.victim) != 0)
    remove_from_lookup_table(GET_ID(&fixture.victim));
  if (trigger_file != NULL)
    fclose(trigger_file);

  prototype_index = parsed ? trig_index[0] : NULL;
  if (prototype_index != NULL)
  {
    commands = ((struct trig_data *)prototype_index->proto)->cmdlist;
    free_trigger((struct trig_data *)prototype_index->proto);
    while (commands != NULL)
    {
      next_command = commands->next;
      free(commands->cmd);
      free(commands);
      commands = next_command;
    }
    free(prototype_index);
  }
  free(trig_index);
  trig_index = saved_trig_index;
  top_of_trigt = saved_top_of_trigt;
  trigger_list = saved_trigger_list;
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, parsed);
  CuAssertIntEquals(tc, 5, damage_result);
  CuAssertIntEquals(tc, 95, remaining_hit_points);
  CuAssertTrue(tc, damage_variable_matches);
}

void Test_gameplay_e2e_actual_minimal_world_parse(CuTest *tc)
{
  struct room_data *saved_world;
  struct zone_data *saved_zone_table;
  struct room_data *parsed_world;
  struct zone_data *parsed_zone;
  room_rnum saved_top_of_world;
  zone_rnum saved_top_of_zone_table;
  int room_count;
  int i;
  bool file_opened;
  bool rooms_match;
  bool exits_match;
  char world_path[PATH_MAX];
  FILE *world_file;

  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_zone_table = zone_table;
  saved_top_of_zone_table = top_of_zone_table;
  parsed_world = calloc(4, sizeof(*parsed_world));
  parsed_zone = calloc(1, sizeof(*parsed_zone));
  world_file = NULL;
  file_opened = false;
  rooms_match = false;
  exits_match = false;
  room_count = 0;

  if (parsed_world != NULL && parsed_zone != NULL &&
      snprintf(world_path, sizeof(world_path), "%s/lib/world/minimal/0.wld", test_source_root()) <
          (int)sizeof(world_path))
  {
    parsed_zone[0].number = 0;
    parsed_zone[0].bot = 0;
    parsed_zone[0].top = 3099;
    world = parsed_world;
    top_of_world = 0;
    zone_table = parsed_zone;
    top_of_zone_table = 0;
    world_file = fopen(world_path, "r");
    file_opened = world_file != NULL;
    if (file_opened)
    {
      discrete_load(world_file, DB_BOOT_WLD, world_path);
      fclose(world_file);
      world_file = NULL;
      renum_world();
      room_count = top_of_world + 1;
      rooms_match = top_of_world == 3 && world[0].number == 0 && world[1].number == 3000 &&
                    world[2].number == 3001 && world[3].number == 3002 &&
                    strcmp(world[1].name, "Arrival Platform") == 0 &&
                    strstr(world[2].description, "Starlight spills") != NULL;
      exits_match = world[1].dir_option[EAST] != NULL && world[1].dir_option[EAST]->to_room == 2 &&
                    world[2].dir_option[WEST] != NULL && world[2].dir_option[WEST]->to_room == 1 &&
                    world[2].dir_option[EAST] != NULL && world[2].dir_option[EAST]->to_room == 3;
    }
  }

  if (world_file != NULL)
    fclose(world_file);
  if (parsed_world != NULL)
  {
    for (i = 0; i < room_count; i++)
    {
      if (parsed_world[i].script != NULL)
        extract_script(&parsed_world[i], WLD_TRIGGER);
      free_proto_script(&parsed_world[i], WLD_TRIGGER);
      free_room_strings(&parsed_world[i]);
      if (parsed_world[i].trail_tracks != NULL)
        free_trail_data_list(parsed_world[i].trail_tracks);
    }
  }
  free(parsed_world);
  free(parsed_zone);
  world = saved_world;
  top_of_world = saved_top_of_world;
  zone_table = saved_zone_table;
  top_of_zone_table = saved_top_of_zone_table;

  CuAssertTrue(tc, file_opened);
  CuAssertIntEquals(tc, 4, room_count);
  CuAssertTrue(tc, rooms_match);
  CuAssertTrue(tc, exits_match);
}
