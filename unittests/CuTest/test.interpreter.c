#include "CuTest.h"

/* Include paths relative to src/ for CMake build */
#include "../../src/bool.h"
#include "../../src/utils.h"
#include "../../src/structs.h"
#include "../../src/interpreter.h"
#include "../../src/act.h"

#include <stdio.h>
#include <stddef.h>


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
  bool created_command_list;

  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  look_command = find_command("look");
  help_command = find_command("help");

  CuAssertTrue(tc, look_command >= 0);
  CuAssertTrue(tc, help_command >= 0);
  CuAssertTrue(tc, look_command != help_command);
  CuAssertIntEquals(tc, -1, find_command("not-a-real-command"));

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
  CuAssertIntEquals(tc, CMD_FEATURE_NONE, complete_cmd_info[look_command].feature_flags);
  CuAssertIntEquals(tc, CMD_FEATURE_NONE, complete_cmd_info[purge_command].feature_flags);

  if (created_command_list)
    free_command_list();
}
