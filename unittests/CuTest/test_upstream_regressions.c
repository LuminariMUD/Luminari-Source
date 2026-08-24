#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/modify.h"
#include "../../src/comms/boards.h"
#include "../../src/obj/item.h"
#include "../../src/olc/genolc.h"
#include "../../src/olc/oasis.h"
#include "../../src/character/class.h"
#include "../../src/character/feats.h"
#include "../../src/dgscript/dg_olc.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/net/protocol.h"
#include "../../src/quest/hlquest.h"
#include "../../src/wilderness/terrain_bridge.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static size_t longest_visible_output_line(const char *output)
{
  size_t current;
  size_t longest;

  current = 0;
  longest = 0;
  while (output != NULL && *output != '\0')
  {
    if (*output == '\033' && output[1] == '[')
    {
      output += 2;
      while (*output != '\0' && *output != 'm')
        output++;
      if (*output == 'm')
        output++;
      continue;
    }
    if (*output == '\r' || *output == '\n')
    {
      if (current > longest)
        longest = current;
      current = 0;
      output++;
      continue;
    }
    current++;
    output++;
  }
  return current > longest ? current : longest;
}

static void reset_test_descriptor_output(struct descriptor_data *descriptor)
{
  if (descriptor->large_outbuf != NULL)
  {
    free(descriptor->large_outbuf->text);
    free(descriptor->large_outbuf);
    descriptor->large_outbuf = NULL;
    if (buf_largecount > 0)
      buf_largecount--;
  }
  descriptor->output = descriptor->small_outbuf;
  descriptor->output[0] = '\0';
  descriptor->bufptr = 0;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
}

void Test_room_trigger_attachments_are_idempotent_and_reset_restorable(CuTest *tc)
{
  struct room_data room;
  struct trig_proto_list proto_attachment;
  struct index_data trigger_index_entry;
  struct trig_data trigger_prototype;
  struct index_data *test_trigger_index[1];
  struct index_data **saved_trigger_index;
  trig_data *runtime_trigger;
  int saved_top_of_trigt;
  int attached_count;
  bool initial_attachment_valid;
  bool duplicate_was_skipped;
  bool restored_after_detach;

  memset(&room, 0, sizeof(room));
  memset(&proto_attachment, 0, sizeof(proto_attachment));
  memset(&trigger_index_entry, 0, sizeof(trigger_index_entry));
  memset(&trigger_prototype, 0, sizeof(trigger_prototype));

  trigger_index_entry.vnum = 123;
  trigger_index_entry.proto = &trigger_prototype;
  trigger_prototype.nr = 0;
  trigger_prototype.attach_type = WLD_TRIGGER;
  trigger_prototype.name = "room trigger attachment fixture";
  proto_attachment.vnum = 123;
  room.number = 456;
  room.proto_script = &proto_attachment;
  test_trigger_index[0] = &trigger_index_entry;
  saved_trigger_index = trig_index;
  saved_top_of_trigt = top_of_trigt;
  trig_index = test_trigger_index;
  top_of_trigt = 1;

  assign_room_triggers(&room);
  initial_attachment_valid = room.script != NULL && dg_script_has_trigger_rnum(room.script, 0);

  assign_room_triggers(&room);
  attached_count = 0;
  for (runtime_trigger = room.script ? TRIGGERS(room.script) : NULL; runtime_trigger != NULL;
       runtime_trigger = runtime_trigger->next)
    attached_count++;
  duplicate_was_skipped = attached_count == 1;

  extract_script(&room.script);
  assign_room_triggers(&room);
  restored_after_detach = room.script != NULL && dg_script_has_trigger_rnum(room.script, 0) &&
                          TRIGGERS(room.script)->next == NULL;

  extract_script(&room.script);
  trig_index = saved_trigger_index;
  top_of_trigt = saved_top_of_trigt;

  CuAssertTrue(tc, initial_attachment_valid);
  CuAssertTrue(tc, duplicate_was_skipped);
  CuAssertTrue(tc, restored_after_detach);
}

void Test_skillset_reports_explicit_administrator_override_policy(CuTest *tc)
{
  struct char_data staff;
  struct char_data target;
  struct descriptor_data descriptor;
  struct player_special_data staff_specials;
  struct player_special_data target_specials;
  struct room_data test_room;
  struct char_data *saved_character_list;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  int saved_minimum_level;
  bool override_reported;
  bool override_applied;
  bool unavailable_reported;
  bool unavailable_rejected;

  if (spell_info[SPELL_MAGIC_MISSILE].name == NULL ||
      spell_info[SPELL_MAGIC_MISSILE].name == unused_spellname)
    mag_assign_spells();

  clear_char(&staff);
  clear_char(&target);
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&staff_specials, 0, sizeof(staff_specials));
  memset(&target_specials, 0, sizeof(target_specials));
  memset(&test_room, 0, sizeof(test_room));

  descriptor.character = &staff;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  staff.desc = &descriptor;
  staff.player_specials = &staff_specials;
  staff.player.name = "skillsetter";
  GET_LEVEL(&staff) = LVL_IMPL;
  IN_ROOM(&staff) = 0;

  target.player_specials = &target_specials;
  target.player.name = "skilltarget";
  GET_CLASS(&target) = CLASS_WIZARD;
  GET_LEVEL(&target) = 1;
  IN_ROOM(&target) = 0;
  target.next = NULL;

  if (descriptor.pProtocol == NULL)
  {
    staff.desc = NULL;
    CuFail(tc, "could not initialize the skillset output fixture");
    return;
  }

  saved_world = world;
  saved_top_of_world = top_of_world;
  test_room.number = 1;
  test_room.sector_type = SECT_INSIDE;
  world = &test_room;
  top_of_world = 0;
  saved_character_list = character_list;
  character_list = &target;
  saved_minimum_level = spell_info[SPELL_MAGIC_MISSILE].min_level[CLASS_WIZARD];
  spell_info[SPELL_MAGIC_MISSILE].min_level[CLASS_WIZARD] = 10;

  do_skillset(&staff, "skilltarget 'magic missile' 42", 0, 0);
  override_reported = strstr(descriptor.output, "Administrator override:") != NULL &&
                      strstr(descriptor.output, "assignment will continue") != NULL;
  override_applied = GET_SKILL(&target, SPELL_MAGIC_MISSILE) == 42;

  reset_test_descriptor_output(&descriptor);
  spell_info[SPELL_MAGIC_MISSILE].min_level[CLASS_WIZARD] = LVL_IMMORT;
  do_skillset(&staff, "skilltarget 'magic missile' 55", 0, 0);
  unavailable_reported = strstr(descriptor.output, "unavailable to mortal") != NULL &&
                         strstr(descriptor.output, "no change was made") != NULL;
  unavailable_rejected = GET_SKILL(&target, SPELL_MAGIC_MISSILE) == 42;

  spell_info[SPELL_MAGIC_MISSILE].min_level[CLASS_WIZARD] = saved_minimum_level;
  character_list = saved_character_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  staff.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  descriptor.pProtocol = NULL;
  reset_test_descriptor_output(&descriptor);

  CuAssertTrue(tc, override_reported);
  CuAssertTrue(tc, override_applied);
  CuAssertTrue(tc, unavailable_reported);
  CuAssertTrue(tc, unavailable_rejected);
}

void Test_room_objects_preserve_money_identity_and_newest_first_lookup(CuTest *tc)
{
  struct room_data test_room;
  struct room_data *saved_world;
  struct obj_data *first_pile;
  struct obj_data *second_pile;
  struct obj_data *older_corpse;
  struct obj_data *newer_corpse;
  room_rnum saved_top_of_world;
  bool money_piles_remained_distinct;
  bool custom_money_properties_remained_distinct;
  bool newest_corpse_was_first;

  memset(&test_room, 0, sizeof(test_room));
  saved_world = world;
  saved_top_of_world = top_of_world;
  test_room.number = 1;
  world = &test_room;
  top_of_world = 0;

  first_pile = create_money(100);
  second_pile = create_money(200);
  GET_OBJ_TIMER(first_pile) = 11;
  GET_OBJ_TIMER(second_pile) = 22;
  SET_BIT_AR(GET_OBJ_EXTRA(first_pile), ITEM_NODONATE);
  obj_to_room(first_pile, 0);
  obj_to_room(second_pile, 0);

  money_piles_remained_distinct =
      world[0].contents == second_pile && second_pile->next_content == first_pile &&
      first_pile->next_content == NULL && GET_OBJ_VAL(first_pile, 0) == 100 &&
      GET_OBJ_VAL(second_pile, 0) == 200;
  custom_money_properties_remained_distinct =
      GET_OBJ_TIMER(first_pile) == 11 && GET_OBJ_TIMER(second_pile) == 22 &&
      OBJ_FLAGGED(first_pile, ITEM_NODONATE) && !OBJ_FLAGGED(second_pile, ITEM_NODONATE);

  older_corpse = create_obj();
  newer_corpse = create_obj();
  older_corpse->name = strdup("corpse older");
  newer_corpse->name = strdup("corpse newer");
  GET_OBJ_TYPE(older_corpse) = ITEM_CONTAINER;
  GET_OBJ_TYPE(newer_corpse) = ITEM_CONTAINER;
  obj_to_room(older_corpse, 0);
  obj_to_room(newer_corpse, 0);
  newest_corpse_was_first = world[0].contents == newer_corpse &&
                            get_obj_in_list("corpse", world[0].contents) == newer_corpse;

  while (world[0].contents != NULL)
    extract_obj(world[0].contents);
  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertTrue(tc, money_piles_remained_distinct);
  CuAssertTrue(tc, custom_money_properties_remained_distinct);
  CuAssertTrue(tc, newest_corpse_was_first);
}

void Test_roomtransfer_command_is_implementor_only(CuTest *tc)
{
  const struct command_info *command;
  bool found = false;
  bool correct_handler = false;
  int minimum_level = -1;

  for (command = cmd_info; *command->command != '\n'; command++)
  {
    if (str_cmp(command->command, "roomtransfer"))
      continue;

    found = true;
    correct_handler = command->command_pointer == do_roomtransfer;
    minimum_level = command->minimum_level;
    break;
  }

  CuAssertTrue(tc, found);
  CuAssertTrue(tc, correct_handler);
  CuAssertIntEquals(tc, LVL_IMPL, minimum_level);
}

void Test_roomtransfer_moves_all_room_people_and_objects(CuTest *tc)
{
  struct room_data rooms[2];
  struct zone_data zone;
  struct char_data staff, player, mobile;
  struct char_data *room_character;
  struct player_special_data staff_specials, player_specials;
  struct descriptor_data staff_descriptor, player_descriptor;
  struct obj_data first_object, second_object;
  struct obj_data *room_object;
  struct room_data *saved_world;
  struct zone_data *saved_zone_table;
  struct char_data *saved_character_list;
  room_rnum saved_top_of_world;
  zone_rnum saved_top_of_zone_table;
  bool lower_level_was_denied;
  bool same_room_was_rejected;
  bool source_was_emptied;
  bool all_characters_arrived;
  bool all_objects_arrived;
  bool success_was_reported;
  bool player_found = false, mobile_found = false;
  bool first_object_found = false, second_object_found = false;

  memset(rooms, 0, sizeof(rooms));
  memset(&zone, 0, sizeof(zone));
  memset(&staff_descriptor, 0, sizeof(staff_descriptor));
  memset(&player_descriptor, 0, sizeof(player_descriptor));
  memset(&staff_specials, 0, sizeof(staff_specials));
  memset(&player_specials, 0, sizeof(player_specials));
  clear_char(&staff);
  clear_char(&player);
  clear_char(&mobile);
  clear_object(&first_object);
  clear_object(&second_object);

  staff_descriptor.character = &staff;
  staff_descriptor.connected = CON_PLAYING;
  staff_descriptor.output = staff_descriptor.small_outbuf;
  staff_descriptor.bufspace = SMALL_BUFSIZE - 1;
  staff_descriptor.pProtocol = ProtocolCreate();
  player_descriptor.character = &player;
  player_descriptor.connected = CON_PLAYING;
  player_descriptor.output = player_descriptor.small_outbuf;
  player_descriptor.bufspace = SMALL_BUFSIZE - 1;
  player_descriptor.pProtocol = ProtocolCreate();

  if (staff_descriptor.pProtocol == NULL || player_descriptor.pProtocol == NULL)
  {
    ProtocolDestroy(staff_descriptor.pProtocol);
    ProtocolDestroy(player_descriptor.pProtocol);
    CuFail(tc, "could not initialize roomtransfer descriptor fixtures");
    return;
  }

  staff.desc = &staff_descriptor;
  staff.player_specials = &staff_specials;
  staff.player.name = "roomtransfer implementor";
  staff.player.title = "";
  GET_LEVEL(&staff) = LVL_IMPL - 1;
  GET_POS(&staff) = POS_STANDING;
  IN_ROOM(&staff) = 1;

  player.desc = &player_descriptor;
  player.player_specials = &player_specials;
  player.player.name = "roomtransfer player";
  player.player.title = "";
  GET_LEVEL(&player) = 1;
  GET_POS(&player) = POS_STANDING;
  IN_ROOM(&player) = 0;

  mobile.player_specials = &dummy_mob;
  mobile.player.short_descr = "a roomtransfer mobile";
  SET_BIT_AR(MOB_FLAGS(&mobile), MOB_ISNPC);
  GET_LEVEL(&mobile) = 1;
  GET_POS(&mobile) = POS_STANDING;
  IN_ROOM(&mobile) = 0;

  rooms[0].number = 100;
  rooms[0].name = "Roomtransfer Source";
  rooms[0].description = "The source room used by the roomtransfer test.\r\n";
  rooms[0].zone = 0;
  rooms[0].sector_type = SECT_INSIDE;
  rooms[0].people = &player;
  player.next_in_room = &mobile;
  rooms[1].number = 200;
  rooms[1].name = "Roomtransfer Destination";
  rooms[1].description = "The destination room used by the roomtransfer test.\r\n";
  rooms[1].zone = 0;
  rooms[1].sector_type = SECT_INSIDE;
  rooms[1].people = &staff;

  zone.number = 1;
  zone.bot = 100;
  zone.top = 200;
  zone.min_level = -1;
  zone.max_level = LVL_IMPL;

  first_object.name = "first roomtransfer object";
  first_object.short_description = "the first roomtransfer object";
  first_object.description = "The first roomtransfer object is here.";
  GET_OBJ_TYPE(&first_object) = ITEM_OTHER;
  second_object.name = "second roomtransfer object";
  second_object.short_description = "the second roomtransfer object";
  second_object.description = "The second roomtransfer object is here.";
  GET_OBJ_TYPE(&second_object) = ITEM_OTHER;

  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_zone_table = zone_table;
  saved_top_of_zone_table = top_of_zone_table;
  saved_character_list = character_list;
  world = rooms;
  top_of_world = 1;
  zone_table = &zone;
  top_of_zone_table = 0;
  character_list = &staff;
  staff.next = &player;
  player.next = &mobile;

  obj_to_room(&first_object, 0);
  obj_to_room(&second_object, 0);

  do_roomtransfer(&staff, "100 200", 0, 0);
  lower_level_was_denied = IN_ROOM(&player) == 0 && IN_ROOM(&mobile) == 0 &&
                           IN_ROOM(&first_object) == 0 && IN_ROOM(&second_object) == 0;

  reset_test_descriptor_output(&staff_descriptor);
  GET_LEVEL(&staff) = LVL_IMPL;
  do_roomtransfer(&staff, "100 100", 0, 0);
  same_room_was_rejected = IN_ROOM(&player) == 0 && IN_ROOM(&mobile) == 0 &&
                           IN_ROOM(&first_object) == 0 && IN_ROOM(&second_object) == 0;

  reset_test_descriptor_output(&staff_descriptor);
  do_roomtransfer(&staff, "100 200", 0, 0);

  source_was_emptied = rooms[0].people == NULL && rooms[0].contents == NULL;
  for (room_character = rooms[1].people; room_character;
       room_character = room_character->next_in_room)
  {
    if (room_character == &player)
      player_found = true;
    else if (room_character == &mobile)
      mobile_found = true;
  }
  all_characters_arrived =
      player_found && mobile_found && IN_ROOM(&player) == 1 && IN_ROOM(&mobile) == 1;

  for (room_object = rooms[1].contents; room_object; room_object = room_object->next_content)
  {
    if (room_object == &first_object)
      first_object_found = true;
    else if (room_object == &second_object)
      second_object_found = true;
  }
  all_objects_arrived = first_object_found && second_object_found && IN_ROOM(&first_object) == 1 &&
                        IN_ROOM(&second_object) == 1;
  success_was_reported =
      strstr(staff_descriptor.output, "Transferred 2 characters and 2 objects") != NULL;

  character_list = saved_character_list;
  zone_table = saved_zone_table;
  top_of_zone_table = saved_top_of_zone_table;
  world = saved_world;
  top_of_world = saved_top_of_world;
  staff.desc = NULL;
  player.desc = NULL;
  reset_test_descriptor_output(&staff_descriptor);
  reset_test_descriptor_output(&player_descriptor);
  ProtocolDestroy(staff_descriptor.pProtocol);
  ProtocolDestroy(player_descriptor.pProtocol);

  CuAssertTrue(tc, lower_level_was_denied);
  CuAssertTrue(tc, same_room_was_rejected);
  CuAssertTrue(tc, source_was_emptied);
  CuAssertTrue(tc, all_characters_arrived);
  CuAssertTrue(tc, all_objects_arrived);
  CuAssertTrue(tc, success_was_reported);
}

void Test_uint32_indices_parse_and_render_high_stat_vnums(CuTest *tc)
{
  const IDXTYPE high_vnum = UINT32_C(4000000000);
  struct char_data staff;
  struct char_data mobile;
  struct descriptor_data descriptor;
  struct player_special_data staff_specials;
  struct room_data rooms[2];
  struct room_data *saved_world;
  struct zone_data zone;
  struct zone_data *saved_zone_table;
  struct reset_com zone_command;
  struct index_data mobile_index;
  struct index_data *saved_mob_index;
  struct index_data object_index;
  struct index_data *saved_obj_index;
  struct obj_data object;
  room_rnum saved_top_of_world;
  zone_rnum saved_top_of_zone_table;
  mob_rnum saved_top_of_mobt;
  obj_rnum saved_top_of_objt;
  int saved_top_of_p_table;
  char high_text[16];
  bool room_stat_valid;
  bool room_list_valid;
  bool mobile_stat_valid;
  bool object_stat_valid;

  clear_char(&staff);
  clear_char(&mobile);
  clear_object(&object);
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&staff_specials, 0, sizeof(staff_specials));
  memset(rooms, 0, sizeof(rooms));
  memset(&zone, 0, sizeof(zone));
  memset(&zone_command, 0, sizeof(zone_command));
  memset(&mobile_index, 0, sizeof(mobile_index));
  memset(&object_index, 0, sizeof(object_index));

  CuAssertIntEquals(tc, 4, (int)sizeof(IDXTYPE));
  CuAssertTrue(tc, atoidx("4000000000") == high_vnum);
  CuAssertTrue(tc, atoidx("4294967295") == NOWHERE);
  CuAssertTrue(tc, atoidx("4294967296") == NOWHERE);
  CuAssertTrue(tc, atoidx("-1") == NOWHERE);
  CuAssertTrue(tc, atoidx("12junk") == NOWHERE);
  sprintindex(high_vnum, high_text, sizeof(high_text));
  CuAssertStrEquals(tc, "4000000000", high_text);
  sprintindex(NOWHERE, high_text, sizeof(high_text));
  CuAssertStrEquals(tc, "NONE", high_text);
  sprintindex(high_vnum, high_text, sizeof(high_text));

  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &staff;
  descriptor.connected = CON_PLAYING;
  descriptor.pProtocol = ProtocolCreate();
  staff.desc = &descriptor;
  staff.player_specials = &staff_specials;
  staff.player.name = "indexstat";
  GET_LEVEL(&staff) = LVL_IMPL;
  GET_POS(&staff) = POS_STANDING;
  IN_ROOM(&staff) = 1;

  mobile.player_specials = &dummy_mob;
  mobile.player.name = "highmobile";
  mobile.player.short_descr = "a high-vnum mobile";
  mobile.player.long_descr = "A high-vnum mobile is here.\r\n";
  mobile.player.description = "A mobile used to test high VNUM stat output.\r\n";
  SET_BIT_AR(MOB_FLAGS(&mobile), MOB_ISNPC);
  GET_MOB_RNUM(&mobile) = 0;
  GET_LEVEL(&mobile) = 1;
  GET_POS(&mobile) = POS_STANDING;
  IN_ROOM(&mobile) = 1;

  rooms[0].number = 1;
  rooms[0].name = "Low VNUM Fixture Room";
  rooms[0].zone = 0;
  rooms[1].number = high_vnum;
  rooms[1].name = "High VNUM Stat Room";
  rooms[1].description = "A room used to test high VNUM stat output.\r\n";
  rooms[1].zone = 0;
  rooms[1].light = 1;
  rooms[1].people = &mobile;
  zone.number = UINT32_C(40000000);
  zone.bot = high_vnum;
  zone.top = high_vnum;
  zone.name = "High VNUM Zone";
  zone.builders = "Test";
  zone.cmd = &zone_command;
  zone_command.command = 'S';
  mobile_index.vnum = high_vnum;

  object.name = "high object";
  object.short_description = "a high-vnum object";
  object.description = "A high-vnum object lies here.";
  GET_OBJ_RNUM(&object) = 0;
  GET_OBJ_TYPE(&object) = ITEM_OTHER;
  object_index.vnum = high_vnum;

  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_zone_table = zone_table;
  saved_top_of_zone_table = top_of_zone_table;
  saved_mob_index = mob_index;
  saved_top_of_mobt = top_of_mobt;
  saved_obj_index = obj_index;
  saved_top_of_objt = top_of_objt;
  saved_top_of_p_table = top_of_p_table;
  world = rooms;
  top_of_world = 1;
  zone_table = &zone;
  top_of_zone_table = 0;
  mob_index = &mobile_index;
  top_of_mobt = 0;
  obj_index = &object_index;
  top_of_objt = 0;
  top_of_p_table = -1;

  if (descriptor.pProtocol == NULL)
  {
    room_stat_valid = FALSE;
    room_list_valid = FALSE;
    mobile_stat_valid = FALSE;
    object_stat_valid = FALSE;
  }
  else
  {
    do_stat(&staff, "room", 0, 0);
    room_stat_valid = strstr(descriptor.output, high_text) != NULL &&
                      longest_visible_output_line(descriptor.output) < 120;

    reset_test_descriptor_output(&descriptor);
    do_oasis_list(&staff, "4000000000 4000000000", 0, SCMD_OASIS_RLIST);
    room_list_valid = strstr(descriptor.output, high_text) != NULL &&
                      longest_visible_output_line(descriptor.output) < 120;

    reset_test_descriptor_output(&descriptor);
    do_stat(&staff, "mob highmobile", 0, 0);
    mobile_stat_valid = strstr(descriptor.output, high_text) != NULL &&
                        longest_visible_output_line(descriptor.output) < 120;

    reset_test_descriptor_output(&descriptor);
    do_stat_object(&staff, &object, ITEM_STAT_MODE_IMMORTAL);
    object_stat_valid = strstr(descriptor.output, high_text) != NULL &&
                        longest_visible_output_line(descriptor.output) < 120;
  }

  world = saved_world;
  top_of_world = saved_top_of_world;
  zone_table = saved_zone_table;
  top_of_zone_table = saved_top_of_zone_table;
  mob_index = saved_mob_index;
  top_of_mobt = saved_top_of_mobt;
  obj_index = saved_obj_index;
  top_of_objt = saved_top_of_objt;
  top_of_p_table = saved_top_of_p_table;
  staff.desc = NULL;
  if (descriptor.pProtocol != NULL)
    ProtocolDestroy(descriptor.pProtocol);
  reset_test_descriptor_output(&descriptor);

  CuAssertTrue(tc, room_stat_valid);
  CuAssertTrue(tc, room_list_valid);
  CuAssertTrue(tc, mobile_stat_valid);
  CuAssertTrue(tc, object_stat_valid);
}

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
  char empty[] = "";
  char cr_only[] = "\r\r";
  char lf_only[] = "\n\n";
  char newlines_only[] = "\r\n\r\n";

  prune_crlf(crlf);
  prune_crlf(lf);
  prune_crlf(repeated);
  prune_crlf(empty);
  prune_crlf(cr_only);
  prune_crlf(lf_only);
  prune_crlf(newlines_only);
  prune_crlf(NULL);
  CuAssertStrEquals(tc, "hello", crlf);
  CuAssertStrEquals(tc, "hello", lf);
  CuAssertStrEquals(tc, "hi", repeated);
  CuAssertStrEquals(tc, "", empty);
  CuAssertStrEquals(tc, "", cr_only);
  CuAssertStrEquals(tc, "", lf_only);
  CuAssertStrEquals(tc, "", newlines_only);

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
  CuAssertIntEquals(tc, 0, count_non_protocol_chars(NULL));
  CuAssertIntEquals(tc, 0, count_non_protocol_chars("@"));
  CuAssertIntEquals(tc, 0, count_non_protocol_chars("\t"));
  CuAssertIntEquals(tc, 0, count_non_protocol_chars("@["));
  CuAssertIntEquals(tc, 0, count_non_protocol_chars("@[unterminated"));
}

static bool read_upstream_test_file(const char *path, char *buffer, size_t buffer_size)
{
  FILE *file;
  size_t length;

  if (path == NULL || buffer == NULL || buffer_size == 0 || !(file = fopen(path, "r")))
    return false;
  length = fread(buffer, 1, buffer_size - 1, file);
  buffer[length] = '\0';
  if (ferror(file) || fclose(file) != 0)
    return false;
  return true;
}

void Test_finish_file_save_replaces_only_after_durable_success(CuTest *tc)
{
  char directory[] = "/tmp/luminari-file-save-XXXXXX";
  char temporary_path[PATH_MAX];
  char destination_path[PATH_MAX];
  char buffer[32];
  FILE *file;

  CuAssertPtrNotNull(tc, mkdtemp(directory));
  snprintf(temporary_path, sizeof(temporary_path), "%s/candidate.tmp", directory);
  snprintf(destination_path, sizeof(destination_path), "%s/live.dat", directory);

  file = fopen(destination_path, "w");
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, fputs("old", file) != EOF);
  CuAssertIntEquals(tc, 0, fclose(file));

  file = fopen(temporary_path, "w");
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, fputs("new", file) != EOF);
  CuAssertTrue(tc, finish_file_save(file, temporary_path, destination_path));
  CuAssertTrue(tc, access(temporary_path, F_OK) != 0);
  CuAssertTrue(tc, read_upstream_test_file(destination_path, buffer, sizeof(buffer)));
  CuAssertStrEquals(tc, "new", buffer);

  unlink(destination_path);
  rmdir(directory);
}

void Test_finish_file_save_preserves_live_and_temporary_files_on_failure(CuTest *tc)
{
  char directory[] = "/tmp/luminari-file-save-failure-XXXXXX";
  char actual_temporary_path[PATH_MAX];
  char missing_temporary_path[PATH_MAX];
  char destination_path[PATH_MAX];
  char buffer[32];
  FILE *file;

  CuAssertPtrNotNull(tc, mkdtemp(directory));
  snprintf(actual_temporary_path, sizeof(actual_temporary_path), "%s/candidate.tmp", directory);
  snprintf(missing_temporary_path, sizeof(missing_temporary_path), "%s/missing.tmp", directory);
  snprintf(destination_path, sizeof(destination_path), "%s/live.dat", directory);

  file = fopen(destination_path, "w");
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, fputs("old", file) != EOF);
  CuAssertIntEquals(tc, 0, fclose(file));

  file = fopen(actual_temporary_path, "w");
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, fputs("candidate", file) != EOF);
  CuAssertTrue(tc, !finish_file_save(file, missing_temporary_path, destination_path));
  CuAssertTrue(tc, access(actual_temporary_path, F_OK) == 0);
  CuAssertTrue(tc, read_upstream_test_file(destination_path, buffer, sizeof(buffer)));
  CuAssertStrEquals(tc, "old", buffer);
  CuAssertTrue(tc, read_upstream_test_file(actual_temporary_path, buffer, sizeof(buffer)));
  CuAssertStrEquals(tc, "candidate", buffer);

  unlink(actual_temporary_path);
  unlink(destination_path);
  rmdir(directory);
}

void Test_finish_file_save_preserves_files_on_write_failure(CuTest *tc)
{
#ifdef CIRCLE_WINDOWS
  CuAssertTrue(tc, TRUE);
#else
  char directory[] = "/tmp/luminari-file-save-write-failure-XXXXXX";
  char temporary_path[PATH_MAX];
  char destination_path[PATH_MAX];
  char buffer[32];
  FILE *file;

  CuAssertPtrNotNull(tc, mkdtemp(directory));
  snprintf(temporary_path, sizeof(temporary_path), "%s/candidate.tmp", directory);
  snprintf(destination_path, sizeof(destination_path), "%s/live.dat", directory);

  file = fopen(destination_path, "w");
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, fputs("old", file) != EOF);
  CuAssertIntEquals(tc, 0, fclose(file));

  file = fopen(temporary_path, "w");
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, fputs("candidate", file) != EOF);
  CuAssertIntEquals(tc, 0, fclose(file));

  file = fopen("/dev/full", "w");
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, fputs("unwritable", file) != EOF);
  CuAssertTrue(tc, !finish_file_save(file, temporary_path, destination_path));
  CuAssertTrue(tc, read_upstream_test_file(destination_path, buffer, sizeof(buffer)));
  CuAssertStrEquals(tc, "old", buffer);
  CuAssertTrue(tc, read_upstream_test_file(temporary_path, buffer, sizeof(buffer)));
  CuAssertStrEquals(tc, "candidate", buffer);

  unlink(temporary_path);
  unlink(destination_path);
  rmdir(directory);
#endif
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

void Test_upstream_script_formatter_requires_complete_control_tokens(CuTest *tc)
{
  const char *source = "ending\r\n"
                       "done_work\r\n"
                       "elsewhere\r\n"
                       "casefold value\r\n"
                       "defaulting\r\n"
                       "breakfast\r\n";
  char error[MAX_INPUT_LENGTH];
  char *formatted = NULL;

  CuAssertTrue(tc, dg_format_script_text(source, MAX_CMD_LENGTH, &formatted, error, sizeof(error)));
  CuAssertPtrNotNull(tc, formatted);
  CuAssertStrEquals(tc, source, formatted);
  free(formatted);
}

void Test_upstream_script_formatter_rejects_invalid_structure_without_mutation(CuTest *tc)
{
  struct descriptor_data descriptor = {0};
  char *script;
  char *original;

  script = strdup("if %enabled%\r\nsay yes\r\n");
  original = strdup(script);
  CuAssertPtrNotNull(tc, script);
  CuAssertPtrNotNull(tc, original);
  descriptor.str = &script;
  descriptor.max_str = MAX_CMD_LENGTH;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);

  CuAssertTrue(tc, !format_script(&descriptor));
  CuAssertStrEquals(tc, original, script);

  ProtocolDestroy(descriptor.pProtocol);
  free(original);
  free(script);
}

void Test_upstream_script_formatter_rejects_unmatched_and_incomplete_controls(CuTest *tc)
{
  static const char *const invalid_scripts[] = {"end\r\n",
                                                "done\r\n",
                                                "else\r\n",
                                                "case value\r\n",
                                                "default\r\n",
                                                "break\r\n",
                                                "if\r\n",
                                                "elseif value\r\n",
                                                "while\r\n",
                                                "switch\r\n",
                                                "switch value\r\ndefault\r\ndefault\r\nend\r\n",
                                                "if value\r\nelse\r\nelse\r\nend\r\n",
                                                NULL};
  char error[MAX_INPUT_LENGTH];
  char *formatted;
  int index;

  for (index = 0; invalid_scripts[index] != NULL; index++)
  {
    formatted = NULL;
    CuAssertTrue(tc, !dg_format_script_text(invalid_scripts[index], MAX_CMD_LENGTH, &formatted,
                                            error, sizeof(error)));
    CuAssertPtrEquals(tc, NULL, formatted);
    CuAssertTrue(tc, *error != '\0');
  }
}

void Test_upstream_script_formatter_preserves_depth_line_and_output_limits(CuTest *tc)
{
  char deep_script[MAX_CMD_LENGTH] = {'\0'};
  char long_line[READ_SIZE + 2];
  char error[MAX_INPUT_LENGTH];
  char *formatted;
  int index;

  for (index = 0; index <= 200; index++)
    strlcat(deep_script, "if 1\r\n", sizeof(deep_script));
  formatted = NULL;
  CuAssertTrue(tc, !dg_format_script_text(deep_script, sizeof(deep_script), &formatted, error,
                                          sizeof(error)));
  CuAssertPtrEquals(tc, NULL, formatted);

  memset(long_line, 'x', sizeof(long_line) - 3);
  long_line[sizeof(long_line) - 3] = '\r';
  long_line[sizeof(long_line) - 2] = '\n';
  long_line[sizeof(long_line) - 1] = '\0';
  formatted = NULL;
  CuAssertTrue(tc,
               !dg_format_script_text(long_line, MAX_CMD_LENGTH, &formatted, error, sizeof(error)));
  CuAssertPtrEquals(tc, NULL, formatted);

  formatted = NULL;
  CuAssertTrue(tc, !dg_format_script_text("if 1\r\nend\r\n", 8, &formatted, error, sizeof(error)));
  CuAssertPtrEquals(tc, NULL, formatted);
}
