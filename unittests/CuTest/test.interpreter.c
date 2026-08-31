#include "CuTest.h"

/* Include paths relative to src/ for CMake build */
#include "../../src/bool.h"
#include "../../src/utils.h"
#include "../../src/structs.h"
#include "../../src/interpreter.h"
#include "../../src/act.h"
#include "../../src/craft/craft.h"
#include "../../src/magic/spells.h"
#include "../../src/net/protocol.h"

#include <limits.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>


void Test_three_arguments_u(CuTest *tc)
{
  // Simple cases
  {
    char inp[] = "HeLLo WOrld bongo turtles";
    char outp1[8] = "nada";
    char outp2[8] = "nada";
    char outp3[8] = "nada";

    char *exp_res = inp + 17;
    const char *const exp_outp1 = "hello";
    const char *const exp_outp2 = "world";
    const char *const exp_outp3 = "bongo";
    char *res = three_arguments_u(inp, outp1, outp2, outp3);

    CuAssertPtrEquals(tc, exp_res, res);
    CuAssertStrEquals(tc, exp_outp1, outp1);
    CuAssertStrEquals(tc, exp_outp2, outp2);
    CuAssertStrEquals(tc, exp_outp3, outp3);
  }
}

void Test_three_arguments(CuTest *tc)
{
  // Simple cases
  {
    const char *const inp = "HeLLo WOrld bongo turtles";
    char outp1[8] = "nada";
    char outp2[8] = "nada";
    char outp3[8] = "nada";

    const char *exp_res = inp + 17;
    const char *const exp_outp1 = "hello";
    const char *const exp_outp2 = "world";
    const char *const exp_outp3 = "bongo";
    const char *res =
        three_arguments(inp, outp1, sizeof(outp1), outp2, sizeof(outp2), outp3, sizeof(outp3));

    CuAssertTrue(tc, exp_res == res);
    CuAssertStrEquals(tc, exp_outp1, outp1);
    CuAssertStrEquals(tc, exp_outp2, outp2);
    CuAssertStrEquals(tc, exp_outp3, outp3);
  }
}

void Test_command_dispatch_lookup(CuTest *tc)
{
  int look_command;
  int help_command;
  int eventdebug_command;
  bool created_command_list;

  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  look_command = find_command("look");
  help_command = find_command("help");
  eventdebug_command = find_command("eventdebug");

  CuAssertTrue(tc, look_command >= 0);
  CuAssertTrue(tc, help_command >= 0);
  CuAssertTrue(tc, look_command != help_command);
  CuAssertTrue(tc, eventdebug_command >= 0);
  CuAssertIntEquals(tc, LVL_IMMORT,
                    complete_cmd_info[eventdebug_command].minimum_level);
  CuAssertIntEquals(tc, -1, find_command("not-a-real-command"));

  if (created_command_list)
    free_command_list();
}

void Test_command_table_excludes_unimplemented_commands(CuTest *tc)
{
  bool created_command_list;

  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  CuAssertIntEquals(tc, -1, find_command("exempt"));
  CuAssertIntEquals(tc, -1, find_command("unconjure"));
  CuAssertIntEquals(tc, -1, find_command("spellquests"));
  CuAssertIntEquals(tc, -1, find_command("shipload"));

  if (created_command_list)
    free_command_list();
}

void Test_stand_command_reaches_sleeping_handler_and_uses_move_action(CuTest *tc)
{
  int stand_command;
  bool created_command_list;

  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  stand_command = find_command("stand");
  CuAssertTrue(tc, stand_command >= 0);
  CuAssertIntEquals(tc, POS_SLEEPING, complete_cmd_info[stand_command].minimum_position);
  CuAssertIntEquals(tc, ACTION_MOVE, complete_cmd_info[stand_command].actions_required);

  if (created_command_list)
    free_command_list();
}

void Test_action_queue_requires_non_mutating_command_preflight(CuTest *tc)
{
  int bandage_command;
  int layonhands_command;
  bool created_command_list;

  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  bandage_command = find_command("bandage");
  layonhands_command = find_command("layonhands");
  CuAssertTrue(tc, bandage_command >= 0);
  CuAssertTrue(tc, layonhands_command >= 0);
  CuAssertTrue(tc, !command_has_queue_preflight(-1));
  CuAssertTrue(tc, !command_has_queue_preflight(INT_MAX));
  CuAssertTrue(tc, !command_has_queue_preflight(bandage_command));
  CuAssertTrue(tc, command_has_queue_preflight(layonhands_command));

  if (created_command_list)
    free_command_list();
}

void Test_abort_command_is_allowed_while_casting(CuTest *tc)
{
  int abort_command;
  int north_command;
  bool created_command_list;

  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  abort_command = find_command("abort");
  north_command = find_command("north");

  CuAssertTrue(tc, abort_command >= 0);
  CuAssertTrue(tc, north_command >= 0);
  CuAssertTrue(tc, complete_cmd_info[abort_command].command_pointer == do_abort);
  CuAssertTrue(tc, command_can_be_used_while_casting(abort_command));
  CuAssertTrue(tc, !command_can_be_used_while_casting(north_command));

  if (created_command_list)
    free_command_list();
}

void Test_harvest_command_uses_legacy_entry_point(CuTest *tc)
{
  int harvest_command;
  bool created_command_list;

  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  harvest_command = find_command("harvest");

  CuAssertTrue(tc, harvest_command >= 0);
  CuAssertTrue(tc, complete_cmd_info[harvest_command].command_pointer == do_harvest);

  if (created_command_list)
    free_command_list();
}

void Test_i3_server_configuration_requires_immortal_level(CuTest *tc)
{
  int admin_command;
  int config_command;
  bool created_command_list;

  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  admin_command = find_command("i3admin");
  config_command = find_command("i3config");

  CuAssertTrue(tc, admin_command >= 0);
  CuAssertTrue(tc, config_command >= 0);
  CuAssertIntEquals(tc, LVL_IMMORT, complete_cmd_info[admin_command].minimum_level);
  CuAssertIntEquals(tc, LVL_IMMORT, complete_cmd_info[config_command].minimum_level);

  if (created_command_list)
    free_command_list();
}

void Test_command_dispatch_numeric_and_reserved_parsing(CuTest *tc)
{
  char reserved_name[] = "self";
  char ordinary_name[] = "coverage_name";

  CuAssertTrue(tc, is_number("42"));
  CuAssertTrue(tc, is_number("-17"));
  CuAssertTrue(tc, !is_number(""));
  CuAssertTrue(tc, !is_number("4two"));
  CuAssertTrue(tc, reserved_word(reserved_name));
  CuAssertTrue(tc, !reserved_word(ordinary_name));
}

void Test_paralyzed_players_can_reach_safe_quit_handler(CuTest *tc)
{
  struct char_data ch;
  struct char_data opponent;
  struct descriptor_data descriptor;
  struct player_special_data player_specials;
  char quit_command[] = "quit";
  char quitlog_command[] = "quitlog";
  bool combat_blocked;

  CuAssertTrue(tc, is_valid_paralyzed_command(quit_command));
  CuAssertTrue(tc, !is_valid_paralyzed_command(quitlog_command));

  memset(&ch, 0, sizeof(ch));
  memset(&opponent, 0, sizeof(opponent));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  ch.player.name = "paralyzed quit test character";
  ch.desc = &descriptor;
  descriptor.character = &ch;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  if (descriptor.pProtocol == NULL)
  {
    CuFail(tc, "ProtocolCreate failed");
    return;
  }
  GET_LEVEL(&ch) = 10;
  GET_POS(&ch) = POS_STANDING;
  FIGHTING(&ch) = &opponent;

  do_quit(&ch, "", 0, SCMD_QUIT);
  combat_blocked = strstr(descriptor.output, "fighting for your life") != NULL;
  ch.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);

  CuAssertTrue(tc, combat_blocked);
}

void Test_vessel_commands_are_registered_for_runtime_gating(CuTest *tc)
{
  int speed_command;
  int drive_command;
  int board_command;
  int boardcheck_command;
  int boardfind_command;
  int look_command;
  int purge_command;
  bool created_command_list;

  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  speed_command = find_command("speed");
  drive_command = find_command("drive");
  board_command = find_command("board");
  boardcheck_command = find_command("boardcheck");
  boardfind_command = find_command("boardfind");
  look_command = find_command("look");
  purge_command = find_command("shippurge");

  CuAssertTrue(tc, speed_command >= 0);
  CuAssertTrue(tc, drive_command >= 0);
  CuAssertTrue(tc, board_command >= 0);
  CuAssertTrue(tc, boardcheck_command >= 0);
  CuAssertTrue(tc, boardfind_command >= 0);
  CuAssertTrue(tc, look_command >= 0);
  CuAssertTrue(tc, purge_command >= 0);
  CuAssertTrue(tc, complete_cmd_info[speed_command].feature_flags & CMD_FEATURE_VESSEL);
  CuAssertTrue(tc, complete_cmd_info[drive_command].feature_flags & CMD_FEATURE_VESSEL);
  CuAssertIntEquals(tc, CMD_FEATURE_NONE, complete_cmd_info[board_command].feature_flags);
  CuAssertIntEquals(tc, CMD_FEATURE_NONE, complete_cmd_info[boardcheck_command].feature_flags);
  CuAssertIntEquals(tc, CMD_FEATURE_NONE, complete_cmd_info[boardfind_command].feature_flags);
  CuAssertIntEquals(tc, CMD_FEATURE_ACTIVITY_INFORMATION,
                    complete_cmd_info[look_command].feature_flags);
  CuAssertIntEquals(tc, CMD_FEATURE_NONE, complete_cmd_info[purge_command].feature_flags);

  if (created_command_list)
    free_command_list();
}

void Test_activity_command_metadata_keeps_status_and_communication_responsive(CuTest *tc)
{
  static const char *information[] = {"abilities", "cooldowns", "craftscore", "exits",
                                      "feats", "resistances", "spelllist", "time",
                                      "tnl", "weather", "where", "wearapplies",
                                      "wearlocations"};
  static const char *speech[] = {"'", "ct", "emote", "grats", "greport", "say", "tell"};
  bool created_command_list = false;
  size_t index;
  int command;

  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }
  for (index = 0U; index < sizeof(information) / sizeof(information[0]); index++)
  {
    command = find_command(information[index]);
    CuAssertTrue(tc, command >= 0);
    CuAssertTrue(tc, complete_cmd_info[command].feature_flags &
                         CMD_FEATURE_ACTIVITY_INFORMATION);
  }
  for (index = 0U; index < sizeof(speech) / sizeof(speech[0]); index++)
  {
    command = find_command(speech[index]);
    CuAssertTrue(tc, command >= 0);
    CuAssertTrue(tc, complete_cmd_info[command].feature_flags &
                         CMD_FEATURE_ACTIVITY_SPEECH);
  }
  command = find_command("activity");
  CuAssertTrue(tc, command >= 0);
  CuAssertTrue(tc, complete_cmd_info[command].feature_flags &
                       CMD_FEATURE_ACTIVITY_CONTROL);

  if (created_command_list)
    free_command_list();
}
