#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/comm.h"
#include "../../src/combat/fight.h"
#include "../../src/db.h"
#include "../../src/interpreter.h"
#include "../../src/obj/item.h"
#include "../../src/obj/shop.h"
#include "../../src/obj/vendor.h"
#include "../../src/quest/quest.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPEC_COMBAT_ROOM_COUNT 1
#define SPEC_COMBAT_MOBILE_COUNT 2
#define SPEC_COMBAT_OBJECT_COUNT 2
#define SPEC_COMBAT_MAX_CALLS 8
#define SPEC_COMBAT_SOURCE_LIMIT (1024L * 1024L)

int weapon_special(struct obj_data *wpn, struct char_data *ch, struct char_data *target, int damage,
                   int attack_type, bool critical, char *hit_msg);

struct spec_combat_call
{
  struct char_data *actor;
  void *owner;
  int command;
  bool argument_is_null;
  char argument[MAX_INPUT_LENGTH];
};

struct spec_combat_recorder
{
  struct spec_combat_call calls[SPEC_COMBAT_MAX_CALLS];
  int callback_return;
  int call_count;
};

struct spec_combat_fixture
{
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct index_data *saved_mob_index;
  mob_rnum saved_top_of_mobt;
  struct index_data *saved_obj_index;
  obj_rnum saved_top_of_objt;
  struct shop_data *saved_shop_index;
  int saved_top_shop;
  struct aq_data *saved_aquest_table;
  qst_rnum saved_total_quests;
  struct command_info *saved_complete_cmd_info;
  int saved_no_specials;

  struct room_data rooms[SPEC_COMBAT_ROOM_COUNT];
  struct index_data mob_indexes[SPEC_COMBAT_MOBILE_COUNT];
  struct index_data obj_indexes[SPEC_COMBAT_OBJECT_COUNT];
  struct shop_data shops[1];
  struct aq_data quests[1];
  struct command_info commands[2];
  room_vnum shop_rooms[1];
  struct char_data characters[3];
  struct obj_data objects[SPEC_COMBAT_OBJECT_COUNT];
  struct spec_combat_recorder recorder;
};

struct spec_combat_source_region
{
  char *source;
  const char *begin;
  const char *end;
};

static struct spec_combat_fixture *active_spec_combat_fixture;

static SPECIAL_DECL(spec_combat_record_callback)
{
  struct spec_combat_call *call;
  int call_index;

  if (active_spec_combat_fixture == NULL)
    return 0;

  call_index = active_spec_combat_fixture->recorder.call_count;
  if (call_index < SPEC_COMBAT_MAX_CALLS)
  {
    call = &active_spec_combat_fixture->recorder.calls[call_index];
    call->actor = ch;
    call->owner = me;
    call->command = cmd;
    call->argument_is_null = argument == NULL;
    if (argument != NULL)
      snprintf(call->argument, sizeof(call->argument), "%s", argument);
  }
  active_spec_combat_fixture->recorder.call_count++;
  return active_spec_combat_fixture->recorder.callback_return;
}

static void spec_combat_set_mobile_flags(struct char_data *mobile, bool has_special)
{
  memset(MOB_FLAGS(mobile), 0, sizeof(mobile->char_specials.saved.act));
  SET_BIT_AR(MOB_FLAGS(mobile), MOB_ISNPC);
  if (has_special)
    SET_BIT_AR(MOB_FLAGS(mobile), MOB_SPEC);
}

static bool spec_combat_fixture_begin(struct spec_combat_fixture *fixture)
{
  int index;

  if (fixture == NULL || active_spec_combat_fixture != NULL)
    return false;

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->saved_obj_index = obj_index;
  fixture->saved_top_of_objt = top_of_objt;
  fixture->saved_shop_index = shop_index;
  fixture->saved_top_shop = top_shop;
  fixture->saved_aquest_table = aquest_table;
  fixture->saved_total_quests = total_quests;
  fixture->saved_complete_cmd_info = complete_cmd_info;
  fixture->saved_no_specials = no_specials;

  fixture->rooms[0].number = 100;
  for (index = 0; index < SPEC_COMBAT_MOBILE_COUNT; index++)
    fixture->mob_indexes[index].vnum = 1000 + index;
  for (index = 0; index < SPEC_COMBAT_OBJECT_COUNT; index++)
    fixture->obj_indexes[index].vnum = 2000 + index;

  fixture->shop_rooms[0] = NOWHERE;
  fixture->shops[0].keeper = 0;
  fixture->shops[0].in_room = fixture->shop_rooms;
  fixture->quests[0].qm = fixture->mob_indexes[0].vnum;

  fixture->commands[0].command = "noop";
  fixture->commands[0].sort_as = "noop";
  fixture->commands[1].command = "\n";
  fixture->commands[1].sort_as = "\n";

  world = fixture->rooms;
  top_of_world = SPEC_COMBAT_ROOM_COUNT - 1;
  mob_index = fixture->mob_indexes;
  top_of_mobt = SPEC_COMBAT_MOBILE_COUNT - 1;
  obj_index = fixture->obj_indexes;
  top_of_objt = SPEC_COMBAT_OBJECT_COUNT - 1;
  shop_index = fixture->shops;
  top_shop = 0;
  aquest_table = fixture->quests;
  total_quests = 1;
  complete_cmd_info = fixture->commands;
  no_specials = 0;
  active_spec_combat_fixture = fixture;
  return true;
}

static void spec_combat_fixture_end(struct spec_combat_fixture *fixture)
{
  if (fixture == NULL || active_spec_combat_fixture != fixture)
    return;

  active_spec_combat_fixture = NULL;
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
  obj_index = fixture->saved_obj_index;
  top_of_objt = fixture->saved_top_of_objt;
  shop_index = fixture->saved_shop_index;
  top_shop = fixture->saved_top_shop;
  aquest_table = fixture->saved_aquest_table;
  total_quests = fixture->saved_total_quests;
  complete_cmd_info = fixture->saved_complete_cmd_info;
  no_specials = fixture->saved_no_specials;
}

static void spec_combat_recorder_reset(struct spec_combat_fixture *fixture)
{
  memset(&fixture->recorder, 0, sizeof(fixture->recorder));
}

static bool spec_combat_call_matches(const struct spec_combat_fixture *fixture, int call_index,
                                     struct char_data *actor, void *owner, int command,
                                     const char *argument)
{
  const struct spec_combat_call *call;

  if (fixture == NULL || call_index < 0 || call_index >= fixture->recorder.call_count ||
      call_index >= SPEC_COMBAT_MAX_CALLS)
    return false;

  call = &fixture->recorder.calls[call_index];
  return call->actor == actor && call->owner == owner && call->command == command &&
         !call->argument_is_null && strcmp(call->argument, argument) == 0;
}

static void spec_combat_prepare_mobile_turn(struct spec_combat_fixture *fixture)
{
  struct char_data *mobile;
  struct char_data *victim;

  mobile = &fixture->characters[0];
  victim = &fixture->characters[1];

  spec_combat_set_mobile_flags(mobile, true);
  spec_combat_set_mobile_flags(victim, false);
  mobile->nr = 0;
  victim->nr = 1;
  mobile->player.short_descr = "combat special mobile";
  victim->player.short_descr = "combat target";
  IN_ROOM(mobile) = 0;
  IN_ROOM(victim) = 0;
  GET_POS(mobile) = POS_FIGHTING;
  GET_POS(victim) = POS_FIGHTING;
  GET_HIT(mobile) = 10;
  GET_MAX_HIT(mobile) = 10;
  GET_HIT(victim) = 10;
  GET_MAX_HIT(victim) = 10;
  FIGHTING(mobile) = victim;
  IS_CASTING(mobile) = true;
  mobile->next_in_room = victim;
  fixture->rooms[0].people = mobile;
  fixture->mob_indexes[0].func = spec_combat_record_callback;
}

static const char *spec_combat_source_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_ROOT");
  return root != NULL && *root != '\0' ? root : ".";
}

static bool spec_combat_read_source(const char *relative_path, char **text)
{
  FILE *file;
  char path[PATH_MAX];
  char *buffer;
  long source_length;
  size_t bytes_read;
  bool success;

  *text = NULL;
  if (snprintf(path, sizeof(path), "%s/%s", spec_combat_source_root(), relative_path) >=
      (int)sizeof(path))
    return false;

  file = fopen(path, "rb");
  if (file == NULL)
    return false;

  success = fseek(file, 0, SEEK_END) == 0;
  source_length = success ? ftell(file) : -1;
  if (source_length < 0 || source_length > SPEC_COMBAT_SOURCE_LIMIT ||
      fseek(file, 0, SEEK_SET) != 0)
    success = false;

  buffer = NULL;
  if (success)
  {
    buffer = malloc((size_t)source_length + 1);
    success = buffer != NULL;
  }
  if (success)
  {
    bytes_read = fread(buffer, 1, (size_t)source_length, file);
    success = bytes_read == (size_t)source_length && ferror(file) == 0;
    buffer[bytes_read] = '\0';
  }
  if (fclose(file) != 0)
    success = false;

  if (!success)
  {
    free(buffer);
    return false;
  }

  *text = buffer;
  return true;
}

static bool spec_combat_load_region(const char *relative_path, const char *start_marker,
                                    const char *end_marker,
                                    struct spec_combat_source_region *region)
{
  memset(region, 0, sizeof(*region));
  if (!spec_combat_read_source(relative_path, &region->source))
    return false;

  region->begin = strstr(region->source, start_marker);
  region->end =
      region->begin != NULL ? strstr(region->begin + strlen(start_marker), end_marker) : NULL;
  if (region->begin == NULL || region->end == NULL || region->begin >= region->end)
  {
    free(region->source);
    memset(region, 0, sizeof(*region));
    return false;
  }
  return true;
}

static void spec_combat_release_region(struct spec_combat_source_region *region)
{
  free(region->source);
  memset(region, 0, sizeof(*region));
}

static const char *spec_combat_region_find(const struct spec_combat_source_region *region,
                                           const char *needle)
{
  const char *match;

  if (region == NULL || region->begin == NULL || region->end == NULL)
    return NULL;
  match = strstr(region->begin, needle);
  return match != NULL && match < region->end ? match : NULL;
}

static bool spec_combat_region_has_statement(const struct spec_combat_source_region *region,
                                             const char *statement)
{
  const char *line;
  const char *next_line;
  const char *trimmed;
  size_t statement_length;

  if (region == NULL || region->begin == NULL || region->end == NULL)
    return false;

  statement_length = strlen(statement);
  line = region->begin;
  while (line < region->end)
  {
    next_line = memchr(line, '\n', (size_t)(region->end - line));
    if (next_line == NULL)
      next_line = region->end;
    trimmed = line;
    while (trimmed < next_line && (*trimmed == ' ' || *trimmed == '\t'))
      trimmed++;
    if ((size_t)(next_line - trimmed) == statement_length &&
        strncmp(trimmed, statement, statement_length) == 0)
      return true;
    line = next_line < region->end ? next_line + 1 : region->end;
  }
  return false;
}

void Test_spec_item_identification_uses_exact_ignored_notification(CuTest *tc)
{
  struct spec_combat_fixture fixture;
  struct char_data *actor;
  struct obj_data *item;
  bool setup_ok;
  bool zero_return_matches;
  bool nonzero_return_matches;
  bool pointer_gate_matches;

  setup_ok = spec_combat_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize item-identification fixture");
    return;
  }

  actor = &fixture.characters[0];
  item = &fixture.objects[0];
  GET_OBJ_RNUM(item) = 0;
  GET_OBJ_TYPE(item) = ITEM_TREASURE;
  fixture.obj_indexes[0].func = spec_combat_record_callback;
  no_specials = 1;

  fixture.recorder.callback_return = 0;
  display_item_object_values(actor, item, ITEM_STAT_MODE_IMMORTAL);
  zero_return_matches = fixture.recorder.call_count == 1 &&
                        spec_combat_call_matches(&fixture, 0, actor, item, 0, "identify");

  spec_combat_recorder_reset(&fixture);
  fixture.recorder.callback_return = 73;
  display_item_object_values(actor, item, ITEM_STAT_MODE_IMMORTAL);
  nonzero_return_matches = fixture.recorder.call_count == 1 &&
                           spec_combat_call_matches(&fixture, 0, actor, item, 0, "identify");

  spec_combat_recorder_reset(&fixture);
  fixture.obj_indexes[0].func = NULL;
  display_item_object_values(actor, item, ITEM_STAT_MODE_IMMORTAL);
  pointer_gate_matches = fixture.recorder.call_count == 0;
  spec_combat_fixture_end(&fixture);

  CuAssertTrue(tc, zero_return_matches);
  CuAssertTrue(tc, nonzero_return_matches);
  CuAssertTrue(tc, pointer_gate_matches);
}

void Test_spec_weapon_special_forwards_token_and_callback_return(CuTest *tc)
{
  struct spec_combat_fixture fixture;
  struct char_data *actor;
  struct char_data *target;
  struct obj_data *weapon;
  char hit_token[] = "session03-hit-token";
  bool setup_ok;
  bool payload_matches;
  bool pointer_gates_match;
  int callback_result;
  int missing_result;
  int null_result;

  setup_ok = spec_combat_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize weapon fixture");
    return;
  }

  actor = &fixture.characters[0];
  target = &fixture.characters[1];
  weapon = &fixture.objects[0];
  GET_OBJ_RNUM(weapon) = 0;
  fixture.obj_indexes[0].func = spec_combat_record_callback;
  fixture.recorder.callback_return = 37;
  no_specials = 1;

  callback_result =
      weapon_special(weapon, actor, target, 17, ATTACK_TYPE_PRIMARY, false, hit_token);
  payload_matches = callback_result == 37 && fixture.recorder.call_count == 1 &&
                    spec_combat_call_matches(&fixture, 0, actor, weapon, 0, hit_token);

  fixture.obj_indexes[0].func = NULL;
  missing_result = weapon_special(weapon, actor, target, 17, ATTACK_TYPE_PRIMARY, false, hit_token);
  null_result = weapon_special(NULL, actor, target, 17, ATTACK_TYPE_PRIMARY, false, hit_token);
  pointer_gates_match = missing_result == 0 && null_result == 0 && fixture.recorder.call_count == 1;
  spec_combat_fixture_end(&fixture);

  CuAssertTrue(tc, payload_matches);
  CuAssertTrue(tc, pointer_gates_match);
}

void Test_spec_weapon_hit_caller_discards_wrapper_return(CuTest *tc)
{
  struct spec_combat_source_region region;
  bool source_loaded;
  bool wielded_ignored;
  bool gloves_ignored;
  bool no_specials_absent;

  source_loaded = spec_combat_load_region(
      "src/combat/fight.c",
      "int handle_successful_attack(struct char_data *ch, struct char_data *victim,",
      "#ifdef LUMINARI_CUTEST", &region);
  wielded_ignored =
      source_loaded &&
      spec_combat_region_has_statement(
          &region, "weapon_special(wielded, ch, victim, dam, attack_type, is_critical, hit_msg);");
  gloves_ignored =
      source_loaded &&
      spec_combat_region_has_statement(
          &region, "weapon_special(GET_EQ(ch, WEAR_HANDS), ch, victim, dam, attack_type, "
                   "is_critical, hit_msg);");
  no_specials_absent = source_loaded && spec_combat_region_find(&region, "no_specials") == NULL;
  if (source_loaded)
    spec_combat_release_region(&region);

  CuAssertTrue(tc, source_loaded);
  CuAssertTrue(tc, wielded_ignored);
  CuAssertTrue(tc, gloves_ignored);
  CuAssertTrue(tc, no_specials_absent);
}

void Test_spec_defense_reactions_preserve_exact_ignored_tokens(CuTest *tc)
{
  struct spec_combat_source_region region;
  bool source_loaded;
  bool shieldblock_matches;
  bool parry_matches;
  bool glance_matches;
  bool dodge_matches;
  bool pointer_gates_match;
  bool no_specials_absent;

  source_loaded = spec_combat_load_region(
      "src/combat/fight.c",
      "int skill_message(int dam, struct char_data *ch, struct char_data *vict, int attacktype,",
      "int compute_energy_absorb(struct char_data *ch, int dam_type)", &region);
  /* Phase 01 routes every defense reaction through the gateway, which owns the
   * pointer gate. The defender stays the actor and the attacker becomes the
   * typed target; the legacy token is unchanged. */
  shieldblock_matches =
      source_loaded &&
      spec_combat_region_has_statement(
          &region, "spec_gateway_defense_reaction(vict, shield, ch, \"shieldblock\");");
  parry_matches =
      source_loaded &&
      spec_combat_region_has_statement(
          &region, "spec_gateway_defense_reaction(vict, opponent_weapon, ch, \"parry\");");
  glance_matches =
      source_loaded && spec_combat_region_has_statement(
                           &region, "spec_gateway_defense_reaction(vict, armor, ch, \"glance\");");
  dodge_matches =
      source_loaded &&
      spec_combat_region_has_statement(
          &region, "spec_gateway_defense_reaction(vict, opponent_weapon, ch, \"dodge\");");
  pointer_gates_match =
      source_loaded &&
      spec_combat_region_find(&region, "obj_index[GET_OBJ_RNUM(shield)].func") == NULL &&
      spec_combat_region_find(&region, "if (name)") == NULL;
  no_specials_absent = source_loaded && spec_combat_region_find(&region, "no_specials") == NULL;
  if (source_loaded)
    spec_combat_release_region(&region);

  CuAssertTrue(tc, source_loaded);
  CuAssertTrue(tc, shieldblock_matches);
  CuAssertTrue(tc, parry_matches);
  CuAssertTrue(tc, glance_matches);
  CuAssertTrue(tc, dodge_matches);
  CuAssertTrue(tc, pointer_gates_match);
  CuAssertTrue(tc, no_specials_absent);
}

void Test_spec_shield_maneuvers_preserve_exact_ignored_tokens(CuTest *tc)
{
  struct spec_combat_source_region punch_region;
  struct spec_combat_source_region charge_region;
  struct spec_combat_source_region slam_region;
  bool punch_loaded;
  bool charge_loaded;
  bool slam_loaded;
  bool punch_matches;
  bool charge_matches;
  bool slam_matches;
  bool pointer_gates_match;
  bool no_specials_absent;

  punch_loaded = spec_combat_load_region(
      "src/combat/act.offensive.c",
      "bool perform_shieldpunch(struct char_data *ch, struct char_data *vict)",
      "bool perform_shieldcharge(struct char_data *ch, struct char_data *vict)", &punch_region);
  charge_loaded = spec_combat_load_region(
      "src/combat/act.offensive.c",
      "bool perform_shieldcharge(struct char_data *ch, struct char_data *vict)",
      "bool perform_shieldslam(struct char_data *ch, struct char_data *vict)", &charge_region);
  slam_loaded = spec_combat_load_region(
      "src/combat/act.offensive.c",
      "bool perform_shieldslam(struct char_data *ch, struct char_data *vict)",
      "void perform_headbutt(struct char_data *ch, struct char_data *vict)", &slam_region);

  /* Phase 01 routes each shield maneuver through the gateway; the shield stays
   * the owner and the maneuver token is unchanged. */
  punch_matches =
      punch_loaded &&
      spec_combat_region_has_statement(
          &punch_region, "spec_gateway_combat_maneuver(ch, shield, vict, \"shieldpunch\");");
  charge_matches =
      charge_loaded &&
      spec_combat_region_has_statement(
          &charge_region, "spec_gateway_combat_maneuver(ch, shield, vict, \"shieldcharge\");");
  slam_matches =
      slam_loaded &&
      spec_combat_region_has_statement(
          &slam_region, "spec_gateway_combat_maneuver(ch, shield, vict, \"shieldslam\");");
  pointer_gates_match =
      punch_loaded && charge_loaded && slam_loaded &&
      spec_combat_region_find(&punch_region, "obj_index[GET_OBJ_RNUM(shield)].func") == NULL &&
      spec_combat_region_find(&charge_region, "obj_index[GET_OBJ_RNUM(shield)].func") == NULL &&
      spec_combat_region_find(&slam_region, "obj_index[GET_OBJ_RNUM(shield)].func") == NULL;
  no_specials_absent = punch_loaded && charge_loaded && slam_loaded &&
                       spec_combat_region_find(&punch_region, "no_specials") == NULL &&
                       spec_combat_region_find(&charge_region, "no_specials") == NULL &&
                       spec_combat_region_find(&slam_region, "no_specials") == NULL;

  if (punch_loaded)
    spec_combat_release_region(&punch_region);
  if (charge_loaded)
    spec_combat_release_region(&charge_region);
  if (slam_loaded)
    spec_combat_release_region(&slam_region);

  CuAssertTrue(tc, punch_loaded);
  CuAssertTrue(tc, charge_loaded);
  CuAssertTrue(tc, slam_loaded);
  CuAssertTrue(tc, punch_matches);
  CuAssertTrue(tc, charge_matches);
  CuAssertTrue(tc, slam_matches);
  CuAssertTrue(tc, pointer_gates_match);
  CuAssertTrue(tc, no_specials_absent);
}

void Test_spec_mounted_charge_preserves_exact_ignored_token(CuTest *tc)
{
  struct spec_combat_source_region region;
  bool source_loaded;
  bool payload_matches;
  bool pointer_gate_matches;
  bool no_specials_absent;

  source_loaded = spec_combat_load_region(
      "src/combat/act.offensive.c",
      "void perform_charge(struct char_data *ch, struct char_data *vict)",
      "bool perform_knockdown(struct char_data *ch, struct char_data *vict, int skill,", &region);
  /* Phase 01 routes the mounted charge through the gateway, which owns the
   * pointer gate and carries the charge target the caller already holds. */
  payload_matches =
      source_loaded &&
      spec_combat_region_has_statement(&region, "spec_gateway_mount_charge(ch, RIDING(ch), vict);");
  pointer_gate_matches =
      source_loaded &&
      spec_combat_region_find(&region, "mob_index[GET_MOB_RNUM(RIDING(ch))].func") == NULL &&
      spec_combat_region_find(&region, "if (name)") == NULL;
  no_specials_absent = source_loaded && spec_combat_region_find(&region, "no_specials") == NULL;
  if (source_loaded)
    spec_combat_release_region(&region);

  CuAssertTrue(tc, source_loaded);
  CuAssertTrue(tc, payload_matches);
  CuAssertTrue(tc, pointer_gate_matches);
  CuAssertTrue(tc, no_specials_absent);
}

void Test_spec_mobile_combat_turn_uses_exact_ignored_payload(CuTest *tc)
{
  struct spec_combat_fixture fixture;
  struct char_data *mobile;
  bool setup_ok;
  bool nonzero_return_matches;
  bool zero_return_matches;

  setup_ok = spec_combat_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize mobile-combat fixture");
    return;
  }

  spec_combat_prepare_mobile_turn(&fixture);
  mobile = &fixture.characters[0];
  no_specials = 1;

  fixture.recorder.callback_return = 91;
  perform_violence(mobile, 2);
  nonzero_return_matches = fixture.recorder.call_count == 1 &&
                           spec_combat_call_matches(&fixture, 0, mobile, mobile, 0, "");

  spec_combat_recorder_reset(&fixture);
  fixture.recorder.callback_return = 0;
  perform_violence(mobile, 2);
  zero_return_matches = fixture.recorder.call_count == 1 &&
                        spec_combat_call_matches(&fixture, 0, mobile, mobile, 0, "");
  spec_combat_fixture_end(&fixture);

  CuAssertTrue(tc, nonzero_return_matches);
  CuAssertTrue(tc, zero_return_matches);
}

void Test_spec_mobile_combat_turn_preserves_activation_gates(CuTest *tc)
{
  struct spec_combat_fixture fixture;
  struct char_data *mobile;
  bool setup_ok;
  bool flag_gate;
  bool pointer_gate;
  bool pending_gate;
  bool hit_point_gate;

  setup_ok = spec_combat_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize mobile-combat gate fixture");
    return;
  }

  spec_combat_prepare_mobile_turn(&fixture);
  mobile = &fixture.characters[0];

  REMOVE_BIT_AR(MOB_FLAGS(mobile), MOB_SPEC);
  perform_violence(mobile, 2);
  flag_gate = fixture.recorder.call_count == 0;

  SET_BIT_AR(MOB_FLAGS(mobile), MOB_SPEC);
  fixture.mob_indexes[0].func = NULL;
  perform_violence(mobile, 2);
  pointer_gate = fixture.recorder.call_count == 0;

  fixture.mob_indexes[0].func = spec_combat_record_callback;
  SET_BIT_AR(MOB_FLAGS(mobile), MOB_NOTDEADYET);
  perform_violence(mobile, 2);
  pending_gate = fixture.recorder.call_count == 0;

  REMOVE_BIT_AR(MOB_FLAGS(mobile), MOB_NOTDEADYET);
  GET_HIT(mobile) = 0;
  perform_violence(mobile, 2);
  hit_point_gate = fixture.recorder.call_count == 0;
  spec_combat_fixture_end(&fixture);

  CuAssertTrue(tc, flag_gate);
  CuAssertTrue(tc, pointer_gate);
  CuAssertTrue(tc, pending_gate);
  CuAssertTrue(tc, hit_point_gate);
}

void Test_spec_mobile_combat_turn_follows_attacks_and_cleave(CuTest *tc)
{
  struct spec_combat_source_region region;
  const char *attack_call;
  const char *cleave_call;
  const char *callback_call;
  bool source_loaded;
  bool order_matches;
  bool activation_matches;
  bool return_ignored;
  bool no_specials_absent;

  source_loaded = spec_combat_load_region(
      "src/combat/fight.c", "void perform_violence(struct char_data *ch, int phase)",
      "int get_monk_stunning_fist_dc(struct char_data *ch)", &region);
  attack_call =
      source_loaded
          ? spec_combat_region_find(&region, "perform_attacks(ch, NORMAL_ATTACK_ROUTINE, phase);")
          : NULL;
  cleave_call = source_loaded ? spec_combat_region_find(&region, "handle_cleave(ch);") : NULL;
  callback_call = source_loaded
                      ? spec_combat_region_find(&region, "spec_gateway_mobile_combat_turn(ch);")
                      : NULL;
  order_matches = attack_call != NULL && cleave_call != NULL && callback_call != NULL &&
                  attack_call < cleave_call && cleave_call < callback_call;
  activation_matches =
      source_loaded &&
      spec_combat_region_find(
          &region, "if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, "
                   "MOB_NOTDEADYET) &&") != NULL &&
      spec_combat_region_find(&region, "GET_HIT(ch) > 0)") != NULL;
  /* The gateway returns void, so the combat caller still cannot act on a
   * return value. */
  return_ignored = source_loaded && spec_combat_region_has_statement(
                                        &region, "spec_gateway_mobile_combat_turn(ch);");
  no_specials_absent = source_loaded && spec_combat_region_find(&region, "no_specials") == NULL;
  if (source_loaded)
    spec_combat_release_region(&region);

  CuAssertTrue(tc, source_loaded);
  CuAssertTrue(tc, order_matches);
  CuAssertTrue(tc, activation_matches);
  CuAssertTrue(tc, return_ignored);
  CuAssertTrue(tc, no_specials_absent);
}

void Test_spec_shop_secondary_propagates_exact_incoming_context(CuTest *tc)
{
  struct spec_combat_fixture fixture;
  struct char_data *actor;
  struct char_data *keeper;
  const char *argument = " shop-secondary exact payload";
  bool setup_ok;
  bool normal_matches;
  bool no_specials_matches;
  int handled;

  setup_ok = spec_combat_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize shop fixture");
    return;
  }

  actor = &fixture.characters[0];
  keeper = &fixture.characters[1];
  keeper->nr = 0;
  fixture.shops[0].func = spec_combat_record_callback;
  fixture.recorder.callback_return = 29;

  handled = shop_keeper(actor, keeper, 41, argument);
  normal_matches = handled == TRUE && fixture.recorder.call_count == 1 &&
                   spec_combat_call_matches(&fixture, 0, actor, keeper, 41, argument);

  spec_combat_recorder_reset(&fixture);
  fixture.recorder.callback_return = 29;
  no_specials = 1;
  handled = shop_keeper(actor, keeper, 41, argument);
  no_specials_matches = handled == TRUE && fixture.recorder.call_count == 1 &&
                        spec_combat_call_matches(&fixture, 0, actor, keeper, 41, argument);
  spec_combat_fixture_end(&fixture);

  CuAssertTrue(tc, normal_matches);
  CuAssertTrue(tc, no_specials_matches);
}

void Test_spec_shop_secondary_zero_falls_through(CuTest *tc)
{
  struct spec_combat_fixture fixture;
  struct char_data *actor;
  struct char_data *keeper;
  const char *argument = " shop-secondary zero payload";
  bool setup_ok;
  bool fallthrough_matches;
  int handled;

  setup_ok = spec_combat_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize shop fallthrough fixture");
    return;
  }

  actor = &fixture.characters[0];
  keeper = &fixture.characters[1];
  keeper->nr = 0;
  IN_ROOM(actor) = 0;
  fixture.shops[0].func = spec_combat_record_callback;
  fixture.recorder.callback_return = 0;
  no_specials = 1;

  handled = shop_keeper(actor, keeper, 0, argument);
  fallthrough_matches = handled == FALSE && fixture.recorder.call_count == 1 &&
                        spec_combat_call_matches(&fixture, 0, actor, keeper, 0, argument);
  spec_combat_fixture_end(&fixture);

  CuAssertTrue(tc, fallthrough_matches);
}

void Test_spec_quest_secondary_propagates_and_falls_through(CuTest *tc)
{
  struct spec_combat_fixture fixture;
  struct char_data *actor;
  struct char_data *questmaster_mobile;
  const char *argument = " quest-secondary exact payload";
  bool setup_ok;
  bool handled_matches;
  bool fallthrough_matches;
  int handled;

  setup_ok = spec_combat_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize quest fixture");
    return;
  }

  actor = &fixture.characters[0];
  questmaster_mobile = &fixture.characters[1];
  spec_combat_set_mobile_flags(questmaster_mobile, false);
  questmaster_mobile->nr = 0;
  fixture.quests[0].func = spec_combat_record_callback;
  fixture.recorder.callback_return = 47;
  no_specials = 1;

  handled = questmaster(actor, questmaster_mobile, 0, argument);
  handled_matches = handled == TRUE && fixture.recorder.call_count == 1 &&
                    spec_combat_call_matches(&fixture, 0, actor, questmaster_mobile, 0, argument);

  spec_combat_recorder_reset(&fixture);
  fixture.recorder.callback_return = 0;
  handled = questmaster(actor, questmaster_mobile, 0, argument);
  fallthrough_matches =
      handled == FALSE && fixture.recorder.call_count == 1 &&
      spec_combat_call_matches(&fixture, 0, actor, questmaster_mobile, 0, argument);
  spec_combat_fixture_end(&fixture);

  CuAssertTrue(tc, handled_matches);
  CuAssertTrue(tc, fallthrough_matches);
}

void Test_spec_quest_shop_original_nesting_propagates_context(CuTest *tc)
{
  struct spec_combat_fixture fixture;
  struct char_data *actor;
  struct char_data *keeper;
  const char *argument = " nested-secondary exact payload";
  bool setup_ok;
  bool nesting_matches;
  int handled;

  setup_ok = spec_combat_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize nested secondary fixture");
    return;
  }

  actor = &fixture.characters[0];
  keeper = &fixture.characters[1];
  spec_combat_set_mobile_flags(keeper, false);
  keeper->nr = 0;
  fixture.shops[0].func = spec_combat_record_callback;
  fixture.quests[0].func = shop_keeper;
  fixture.recorder.callback_return = 61;
  no_specials = 1;

  handled = questmaster(actor, keeper, 0, argument);
  nesting_matches = handled == TRUE && fixture.recorder.call_count == 1 &&
                    spec_combat_call_matches(&fixture, 0, actor, keeper, 0, argument);
  spec_combat_fixture_end(&fixture);

  CuAssertTrue(tc, nesting_matches);
}

void Test_spec_typed_bank_survives_quest_shop_compatibility_nesting(CuTest *tc)
{
  struct spec_combat_fixture fixture;
  struct char_data *actor;
  struct char_data *keeper;
  bool setup_ok;
  bool nesting_matches;
  int handled;

  setup_ok = spec_combat_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize typed nested secondary fixture");
    return;
  }

  actor = &fixture.characters[0];
  keeper = &fixture.characters[1];
  actor->player.name = "phase six depositor";
  keeper->player.short_descr = "phase six shopkeeper";
  spec_combat_set_mobile_flags(keeper, false);
  keeper->nr = 0;
  IN_ROOM(actor) = 0;
  IN_ROOM(keeper) = 0;
  actor->next_in_room = keeper;
  fixture.rooms[0].people = actor;
  fixture.commands[0].command = "deposit";
  fixture.commands[0].sort_as = "deposit";
  fixture.shops[0].func = bank;
  fixture.quests[0].func = shop_keeper;
  GET_GOLD(actor) = 125;
  GET_BANK_GOLD(actor) = 75;
  no_specials = 1;

  handled = questmaster(actor, keeper, 0, "all");
  nesting_matches = handled == TRUE && GET_GOLD(actor) == 0 && GET_BANK_GOLD(actor) == 200;
  spec_combat_fixture_end(&fixture);

  CuAssertTrue(tc, nesting_matches);
}

void Test_spec_secondary_assignment_and_boot_gates_preserve_composition(CuTest *tc)
{
  struct spec_combat_source_region shop_region;
  struct spec_combat_source_region quest_region;
  struct spec_combat_source_region world_region;
  struct spec_combat_source_region boot_region;
  const char *shop_save;
  const char *shop_install;
  const char *quest_save;
  const char *quest_install;
  const char *assignment_gate;
  const char *mobile_assignment;
  const char *shop_assignment;
  const char *object_assignment;
  const char *room_assignment;
  const char *quest_assignment;
  const char *shop_load_gate;
  const char *shop_load;
  bool shop_loaded;
  bool quest_loaded;
  bool world_loaded;
  bool boot_loaded;
  bool shop_order_matches;
  bool quest_order_matches;
  bool shop_load_gate_matches;
  bool assignment_gate_matches;

  shop_loaded = spec_combat_load_region("src/obj/shop.c", "void assign_the_shopkeepers(void)",
                                        "static char *customer_string(int shop_nr, int detailed)",
                                        &shop_region);
  quest_loaded =
      spec_combat_load_region("src/quest/quest.c", "void assign_the_quests(void)",
                              "void set_quest(struct char_data *ch, qst_rnum rnum,", &quest_region);
  world_loaded = spec_combat_load_region("src/db.c", "void boot_world(void)",
                                         "static void free_extra_descriptions(", &world_region);
  boot_loaded = spec_combat_load_region("src/db.c", "void boot_db(void)",
                                        "static void reset_time(void)", &boot_region);

  shop_save = shop_loaded
                  ? spec_combat_region_find(
                        &shop_region, "SHOP_FUNC(cindex) = mob_index[SHOP_KEEPER(cindex)].func;")
                  : NULL;
  shop_install = shop_loaded
                     ? spec_combat_region_find(&shop_region,
                                               "mob_index[SHOP_KEEPER(cindex)].func = shop_keeper;")
                     : NULL;
  shop_order_matches = shop_save != NULL && shop_install != NULL && shop_save < shop_install;

  quest_save = quest_loaded ? spec_combat_region_find(&quest_region,
                                                      "QST_FUNC(rnum) = mob_index[(mrnum)].func;")
                            : NULL;
  quest_install = quest_loaded ? spec_combat_region_find(&quest_region,
                                                         "mob_index[(mrnum)].func = questmaster;")
                               : NULL;
  quest_order_matches = quest_save != NULL && quest_install != NULL && quest_save < quest_install;

  shop_load_gate =
      world_loaded ? spec_combat_region_find(&world_region, "if (!no_specials)") : NULL;
  shop_load =
      world_loaded ? spec_combat_region_find(&world_region, "index_boot(DB_BOOT_SHP);") : NULL;
  shop_load_gate_matches =
      shop_load_gate != NULL && shop_load != NULL && shop_load_gate < shop_load &&
      spec_combat_region_find(&world_region, "index_boot(DB_BOOT_SHP);\n\n#if") != NULL;

  assignment_gate = boot_loaded ? spec_combat_region_find(&boot_region, "if (!no_specials)") : NULL;
  mobile_assignment =
      boot_loaded ? spec_combat_region_find(&boot_region, "assign_mobiles();") : NULL;
  shop_assignment =
      boot_loaded ? spec_combat_region_find(&boot_region, "assign_the_shopkeepers();") : NULL;
  object_assignment =
      boot_loaded ? spec_combat_region_find(&boot_region, "assign_objects();") : NULL;
  room_assignment = boot_loaded ? spec_combat_region_find(&boot_region, "assign_rooms();") : NULL;
  quest_assignment =
      boot_loaded ? spec_combat_region_find(&boot_region, "assign_the_quests();") : NULL;
  assignment_gate_matches =
      assignment_gate != NULL && mobile_assignment != NULL && shop_assignment != NULL &&
      object_assignment != NULL && room_assignment != NULL && quest_assignment != NULL &&
      assignment_gate < mobile_assignment && mobile_assignment < shop_assignment &&
      shop_assignment < object_assignment && object_assignment < room_assignment &&
      room_assignment < quest_assignment &&
      spec_combat_region_find(&boot_region, "assign_the_quests();\n  }") != NULL;

  if (shop_loaded)
    spec_combat_release_region(&shop_region);
  if (quest_loaded)
    spec_combat_release_region(&quest_region);
  if (world_loaded)
    spec_combat_release_region(&world_region);
  if (boot_loaded)
    spec_combat_release_region(&boot_region);

  CuAssertTrue(tc, shop_loaded);
  CuAssertTrue(tc, quest_loaded);
  CuAssertTrue(tc, world_loaded);
  CuAssertTrue(tc, boot_loaded);
  CuAssertTrue(tc, shop_order_matches);
  CuAssertTrue(tc, quest_order_matches);
  CuAssertTrue(tc, shop_load_gate_matches);
  CuAssertTrue(tc, assignment_gate_matches);
}
