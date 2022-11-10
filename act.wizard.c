/**************************************************************************
 *  File: act.wizard.c                                 Part of LuminariMUD *
 *  Usage: Player-level god commands and other goodies.                    *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "shop.h"
#include "act.h"
#include "mysql.h"
#include "genzon.h" /* for real_zone_by_thing */
#include "class.h"
#include "genolc.h"
#include "genobj.h"
#include "race.h"
#include "fight.h"
#include "house.h"
#include "modify.h"
#include "quest.h"
#include "ban.h"
#include "screen.h"
#include "mud_event.h"
#include "clan.h"
#include "craft.h"
#include "hlquest.h"
#include "mudlim.h"
#include "spec_abilities.h"
#include "wilderness.h"
#include "feats.h"
#include "assign_wpn_armor.h"
#include "item.h"
#include "feats.h"
#include "domains_schools.h"
#include "crafts.h" /* NewCraft */
#include "account.h"
#include "alchemy.h"
#include "mud_event.h"
#include "premadebuilds.h"
#include "perfmon.h"
#include "missions.h"
#include "deities.h"

/* local utility functions with file scope */
static int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
static void perform_immort_invis(struct char_data *ch, int level);
static void list_zone_commands_room(struct char_data *ch, room_vnum rvnum);
static void do_stat_room(struct char_data *ch, struct room_data *rm);
static void do_stat_scriptvar(struct char_data *ch, struct char_data *k);
static void do_stat_character(struct char_data *ch, struct char_data *k);
static void stop_snooping(struct char_data *ch);
static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall);
// static struct char_data *is_in_game(long idnum);
static void mob_checkload(struct char_data *ch, mob_vnum mvnum);
static void obj_checkload(struct char_data *ch, obj_vnum ovnum);
static void trg_checkload(struct char_data *ch, trig_vnum tvnum);
static void mod_llog_entry(struct last_entry *llast, int type);
static int get_max_recent(void);
static void clear_recent(struct recent_player *this);
static struct recent_player *create_recent(void);

const char *get_spec_func_name(SPECIAL_DECL(*func));
bool zedit_get_levels(struct descriptor_data *d, char *buf);

bool delete_path(region_vnum vnum);

/* Local Globals */
static struct recent_player *recent_list = NULL; /** Global list of recent players */

// external functions
void save_char_pets(struct char_data *ch);

int purge_room(room_rnum room)
{
  int j;
  struct char_data *vict;

  if (room == NOWHERE || room > top_of_world)
    return 0;

  for (vict = world[room].people; vict; vict = vict->next_in_room)
  {
    if (!IS_NPC(vict))
      continue;

    /* Dump inventory. */
    while (vict->carrying)
      extract_obj(vict->carrying);

    /* Dump equipment. */
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(vict, j))
        extract_obj(GET_EQ(vict, j));

    /* Dump character. */
    extract_char(vict);
  }

  /* Clear the ground. */
  while (world[room].contents)
    extract_obj(world[room].contents);

  return 1;
}

ACMD(do_echo)
{
  skip_spaces_c(&argument);

  if (!*argument)
    send_to_char(ch, "Yes.. but what?\r\n");
  else
  {
    char buf[MAX_INPUT_LENGTH + 4];

    if (subcmd == SCMD_EMOTE)
      snprintf(buf, sizeof(buf), "$n %s", argument);
    else
    {
      strlcpy(buf, argument, sizeof(buf));
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s echoed: %s", GET_NAME(ch), buf);
    }
    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
  }
}

ACMD(do_send)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'}, buf[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;

  half_chop_c(argument, arg, sizeof(arg), buf, sizeof(buf));

  if (!*arg)
  {
    send_to_char(ch, "Send what to who?\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
  {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }
  send_to_char(vict, "%s\r\n", buf);
  mudlog(CMP, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s sent %s: %s", GET_NAME(ch), GET_NAME(vict), buf);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "Sent.\r\n");
  else
    send_to_char(ch, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data *ch, const char *rawroomstr)
{
  room_rnum location = NOWHERE;
  char roomstr[MAX_INPUT_LENGTH] = {'\0'};

  one_argument(rawroomstr, roomstr, sizeof(roomstr));

  if (!*roomstr)
  {
    send_to_char(ch, "You must supply a room number or name.\r\n");
    return (NOWHERE);
  }

  if (isdigit(*roomstr) && !strchr(roomstr, '.'))
  {
    if ((location = real_room((room_vnum)atoi(roomstr))) >= NOWHERE)
    {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (NOWHERE);
    }
  }
  else
  {
    struct char_data *target_mob;
    struct obj_data *target_obj;
    char *mobobjstr = roomstr;
    int num;

    num = get_number(&mobobjstr);
    if ((target_mob = get_char_vis(ch, mobobjstr, &num, FIND_CHAR_WORLD)) != NULL)
    {
      if ((location = IN_ROOM(target_mob)) == NOWHERE)
      {
        send_to_char(ch, "That character is currently lost.\r\n");
        return (NOWHERE);
      }
    }
    else if ((target_obj = get_obj_vis(ch, mobobjstr, &num)) != NULL)
    {
      if (IN_ROOM(target_obj) != NOWHERE)
        location = IN_ROOM(target_obj);
      else if (target_obj->carried_by && IN_ROOM(target_obj->carried_by) != NOWHERE)
        location = IN_ROOM(target_obj->carried_by);
      else if (target_obj->worn_by && IN_ROOM(target_obj->worn_by) != NOWHERE)
        location = IN_ROOM(target_obj->worn_by);

      if (location == NOWHERE)
      {
        send_to_char(ch, "That object is currently not in a room.\r\n");
        return (NOWHERE);
      }
    }

    if (location == NOWHERE)
    {
      send_to_char(ch, "Nothing exists by that name.\r\n");
      return (NOWHERE);
    }
  }

  /* A location has been found -- if you're >= GRSTAFF, no restrictions. */
  if (GET_LEVEL(ch) >= LVL_GRSTAFF)
    return (location);

  if (ROOM_FLAGGED(location, ROOM_STAFFROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n");
  else if (ROOM_FLAGGED(location, ROOM_PRIVATE) && world[location].people && world[location].people->next_in_room)
    send_to_char(ch, "There's a private conversation going on in that room.\r\n");
  else if (ROOM_FLAGGED(location, ROOM_HOUSE) && !House_can_enter(ch, GET_ROOM_VNUM(location)))
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
  else
    return (location);

  return (NOWHERE);
}

ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH] = {'\0'}, buf[MAX_INPUT_LENGTH] = {'\0'};
  room_rnum location, original_loc;
  int orig_x, orig_y; /* Needed if 'at'ing in the wilderness. */

  half_chop_c(argument, buf, sizeof(buf), command, sizeof(command));
  if (!*buf)
  {
    send_to_char(ch, "You must supply a room number or a name.\r\n");
    return;
  }

  if (!*command)
  {
    send_to_char(ch, "What do you want to do there?\r\n");
    return;
  }

  if ((location = find_target_room(ch, buf)) == NOWHERE)
    return;

  /* a location has been found. */
  original_loc = IN_ROOM(ch);

  orig_x = X_LOC(ch);
  orig_y = Y_LOC(ch);

  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(location), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[location].coords[0];
    Y_LOC(ch) = world[location].coords[1];
  }

  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (IN_ROOM(ch) == location)
  {
    char_from_room(ch);

    X_LOC(ch) = orig_x;
    Y_LOC(ch) = orig_y;

    char_to_room(ch, original_loc);
  }
}

ACMD(do_goto)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char arg[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};
  room_rnum location;

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!*arg2)
  {
    if ((location = find_target_room(ch, argument)) == NOWHERE)
      return;
  }
  else
  {
    /* Have two args, that means coordinates (potentially) */
    if ((location = find_room_by_coordinates(atoi(arg), atoi(arg2))) == NOWHERE)
    {
      if ((location = find_available_wilderness_room()) == NOWHERE)
      {
        return;
      }
      else
      {
        /* Must set the coords, etc in the going_to room. */
        assign_wilderness_room(location, atoi(arg), atoi(arg2));
      }
    }
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(location), ZONE_NOIMMORT) && (GET_LEVEL(ch) >= LVL_IMMORT) && (GET_LEVEL(ch) < LVL_GRSTAFF))
  {
    send_to_char(ch, "Sorry, that zone is off-limits for immortals!");
    return;
  }

  snprintf(buf, sizeof(buf), "$n %s", POOFOUT(ch) ? POOFOUT(ch) : "disappears in a puff of smoke.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(location), ZONE_WILDERNESS))
  {
    //    char_to_coords(ch, world[location].coords[0], world[location].coords[1], 0);
    X_LOC(ch) = world[location].coords[0];
    Y_LOC(ch) = world[location].coords[1];
  }

  char_to_room(ch, location);

  snprintf(buf, sizeof(buf), "$n %s", POOFIN(ch) ? POOFIN(ch) : "appears with an ear-splitting bang.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  look_at_room(ch, 0);
  enter_wtrigger(&world[IN_ROOM(ch)], ch, -1);
}

ACMD(do_trans)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf, sizeof(buf));
  if (!*buf)
    send_to_char(ch, "Whom do you wish to transfer?\r\n");
  else if (str_cmp("all", buf))
  {
    if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (victim == ch)
      send_to_char(ch, "That doesn't make much sense, does it?\r\n");
    else
    {
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim))
      {
        send_to_char(ch, "Go transfer someone your own size.\r\n");
        return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);

      if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
      {
        X_LOC(victim) = world[IN_ROOM(ch)].coords[0];
        Y_LOC(victim) = world[IN_ROOM(ch)].coords[1];
      }

      char_to_room(victim, IN_ROOM(ch));
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(victim, 0);

      enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
    }
  }
  else
  { /* Trans All */
    if (GET_LEVEL(ch) < LVL_GRSTAFF)
    {
      send_to_char(ch, "I think not.\r\n");
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i->character && i->character != ch)
      {
        victim = i->character;
        if (GET_LEVEL(victim) >= GET_LEVEL(ch))
          continue;
        act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
        char_from_room(victim);

        if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
        {
          X_LOC(victim) = world[IN_ROOM(ch)].coords[0];
          Y_LOC(victim) = world[IN_ROOM(ch)].coords[1];
        }

        char_to_room(victim, IN_ROOM(ch));
        act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
        act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
        look_at_room(victim, 0);
        enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
      }
    send_to_char(ch, "%s", CONFIG_OK);
  }
}

ACMD(do_teleport)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *victim;
  room_rnum target;

  two_arguments(argument, buf, sizeof(buf), buf2, sizeof(buf2));

  if (!*buf)
    send_to_char(ch, "Whom do you wish to teleport?\r\n");
  else if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (victim == ch)
    send_to_char(ch, "Use 'goto' to teleport yourself.\r\n");
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char(ch, "Maybe you shouldn't do that.\r\n");
  else if (!*buf2)
    send_to_char(ch, "Where do you wish to send this person?\r\n");
  else if ((target = find_target_room(ch, buf2)) != NOWHERE)
  {
    send_to_char(ch, "%s", CONFIG_OK);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(target), ZONE_WILDERNESS))
    {
      X_LOC(victim) = world[target].coords[0];
      Y_LOC(victim) = world[target].coords[1];
    }

    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *)victim, TO_VICT);
    look_at_room(victim, 0);
    enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
  }
}

ACMD(do_vnum)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'};
  int good_arg = 0;

  half_chop_c(argument, buf, sizeof(buf), buf2, sizeof(buf2));

  if (!*buf || !*buf2)
  {
    send_to_char(ch, "Usage: vnum { obj | mob | room | trig } <name>\r\n");
    return;
  }
  if (is_abbrev(buf, "mob") && (good_arg = 1))
    if (!vnum_mobile(buf2, ch))
      send_to_char(ch, "No mobiles by that name.\r\n");

  if (is_abbrev(buf, "obj") && (good_arg = 1))
    if (!vnum_object(buf2, ch))
      send_to_char(ch, "No objects by that name.\r\n");

  if (is_abbrev(buf, "room") && (good_arg = 1))
    if (!vnum_room(buf2, ch))
      send_to_char(ch, "No rooms by that name.\r\n");

  if (is_abbrev(buf, "trig") && (good_arg = 1))
    if (!vnum_trig(buf2, ch))
      send_to_char(ch, "No triggers by that name.\r\n");

  if (!good_arg)
    send_to_char(ch, "Usage: vnum { obj | mob | room | trig } <name>\r\n");
}

#define ZOCMD zone_table[zrnum].cmd[subcmd]

static void list_zone_commands_room(struct char_data *ch, room_vnum rvnum)
{
  zone_rnum zrnum = real_zone_by_thing(rvnum);
  room_rnum rrnum = real_room(rvnum), cmd_room = NOWHERE;
  int subcmd = 0, count = 0;

  if (zrnum == NOWHERE || rrnum == NOWHERE)
  {
    send_to_char(ch, "No zone information available.\r\n");
    return;
  }

  get_char_colors(ch);

  send_to_char(ch, "Zone commands in this room:%s\r\n", yel);
  while (ZOCMD.command != 'S')
  {
    switch (ZOCMD.command)
    {
    case 'M':
    case 'O':
    case 'T':
    case 'V':
      cmd_room = ZOCMD.arg3;
      break;
    case 'D':
    case 'R':
      cmd_room = ZOCMD.arg1;
      break;
    default:
      break;
    }
    if (cmd_room == rrnum)
    {
      count++;
      /* start listing */
      switch (ZOCMD.command)
      {
      case 'I':
        send_to_char(ch, "%sGive it random treasure (%d%%)",
                     ZOCMD.if_flag ? " then " : "",
                     ZOCMD.arg1);
        break;
      case 'L':
        send_to_char(ch, "%sPut random treasure in %s [%s%d%s] (%d%%)",
                     ZOCMD.if_flag ? " then " : "",
                     obj_proto[ZOCMD.arg1].short_description,
                     cyn, obj_index[ZOCMD.arg1].vnum, yel,
                     ZOCMD.arg2);
        break;
      case 'M':
        send_to_char(ch, "%sLoad %s [%s%d%s], Max : %d\r\n",
                     ZOCMD.if_flag ? " then " : "",
                     mob_proto[ZOCMD.arg1].player.short_descr, cyn,
                     mob_index[ZOCMD.arg1].vnum, yel, ZOCMD.arg2);
        break;
      case 'G':
        send_to_char(ch, "%sGive it %s [%s%d%s], Max : %d\r\n",
                     ZOCMD.if_flag ? " then " : "",
                     obj_proto[ZOCMD.arg1].short_description,
                     cyn, obj_index[ZOCMD.arg1].vnum, yel,
                     ZOCMD.arg2);
        break;
      case 'O':
        send_to_char(ch, "%sLoad %s [%s%d%s], Max : %d\r\n",
                     ZOCMD.if_flag ? " then " : "",
                     obj_proto[ZOCMD.arg1].short_description,
                     cyn, obj_index[ZOCMD.arg1].vnum, yel,
                     ZOCMD.arg2);
        break;
      case 'E':
        send_to_char(ch, "%sEquip with %s [%s%d%s], %s, Max : %d\r\n",
                     ZOCMD.if_flag ? " then " : "",
                     obj_proto[ZOCMD.arg1].short_description,
                     cyn, obj_index[ZOCMD.arg1].vnum, yel,
                     equipment_types[ZOCMD.arg3],
                     ZOCMD.arg2);
        break;
      case 'P':
        send_to_char(ch, "%sPut %s [%s%d%s] in %s [%s%d%s], Max : %d\r\n",
                     ZOCMD.if_flag ? " then " : "",
                     obj_proto[ZOCMD.arg1].short_description,
                     cyn, obj_index[ZOCMD.arg1].vnum, yel,
                     obj_proto[ZOCMD.arg3].short_description,
                     cyn, obj_index[ZOCMD.arg3].vnum, yel,
                     ZOCMD.arg2);
        break;
      case 'R':
        send_to_char(ch, "%sRemove %s [%s%d%s] from room.\r\n",
                     ZOCMD.if_flag ? " then " : "",
                     obj_proto[ZOCMD.arg2].short_description,
                     cyn, obj_index[ZOCMD.arg2].vnum, yel);
        break;
      case 'D':
        send_to_char(ch, "%sSet door %s as %s.\r\n",
                     ZOCMD.if_flag ? " then " : "",
                     dirs[ZOCMD.arg2],
                     ZOCMD.arg3 ? ((ZOCMD.arg3 == 1) ? "closed" : "locked") : "open");
        break;
      case 'T':
        send_to_char(ch, "%sAttach trigger %s%s%s [%s%d%s] to %s\r\n",
                     ZOCMD.if_flag ? " then " : "",
                     cyn, trig_index[ZOCMD.arg2]->proto->name, yel,
                     cyn, trig_index[ZOCMD.arg2]->vnum, yel,
                     ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" : ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" : ((ZOCMD.arg1 == WLD_TRIGGER) ? "room" : "????"))));
        break;
      case 'V':
        send_to_char(ch, "%sAssign global %s:%d to %s = %s\r\n",
                     ZOCMD.if_flag ? " then " : "",
                     ZOCMD.sarg1, ZOCMD.arg2,
                     ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" : ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" : ((ZOCMD.arg1 == WLD_TRIGGER) ? "room" : "????"))),
                     ZOCMD.sarg2);
        break;
      default:
        send_to_char(ch, "<Unknown Command>\r\n");
        break;
      }
    }
    subcmd++;
  }
  send_to_char(ch, "%s", nrm);
  if (!count)
    send_to_char(ch, "None!\r\n");
}
#undef ZOCMD

static void do_stat_room(struct char_data *ch, struct room_data *rm)
{
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  struct extra_descr_data *desc;
  int i, found, column;
  struct obj_data *j;
  struct char_data *k;

  send_to_char(ch, "Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name, CCNRM(ch, C_NRM));

  sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
  send_to_char(ch, "Zone: [%3d], VNum: [%s%5d%s], RNum: [%5d], IDNum: [%5ld], Type: %s\r\n",
               zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number,
               CCNRM(ch, C_NRM), real_room(rm->number), (long)rm->number + ROOM_ID_BASE, buf2);
  send_to_char(ch, "Coordinate Location (Wilderness only): (%d, %d)\r\n", rm->coords[0], rm->coords[1]);
  sprintbitarray(rm->room_flags, room_bits, RF_ARRAY_MAX, buf2);
  send_to_char(ch, "SpecProc: %s, Flags: %s\r\n", rm->func == NULL ? "None" : get_spec_func_name(rm->func), buf2);

  sprintbit((long)rm->room_affections, room_affections, buf2, sizeof(buf2));
  send_to_char(ch, "Room affections: %s\r\n", buf2);

  send_to_char(ch, "Description:\r\n%s", rm->description ? rm->description : "  None.\r\n");

  if (rm->ex_description)
  {
    send_to_char(ch, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = rm->ex_description; desc; desc = desc->next)
      send_to_char(ch, " [%s]", desc->keyword);
    send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
  }

  send_to_char(ch, "Chars present:%s", CCYEL(ch, C_NRM));
  column = 14; /* ^^^ strlen ^^^ */
  for (found = FALSE, k = rm->people; k; k = k->next_in_room)
  {
    if (!CAN_SEE(ch, k))
      continue;

    column += send_to_char(ch, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
                           !IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB"));
    if (column >= 62)
    {
      send_to_char(ch, "%s\r\n", k->next_in_room ? "," : "");
      found = FALSE;
      column = 0;
    }
  }
  send_to_char(ch, "%s", CCNRM(ch, C_NRM));

  if (rm->contents)
  {
    send_to_char(ch, "Contents:%s", CCGRN(ch, C_NRM));
    column = 9; /* ^^^ strlen ^^^ */

    for (found = 0, j = rm->contents; j; j = j->next_content)
    {
      if (!CAN_SEE_OBJ(ch, j))
        continue;

      column += send_to_char(ch, "%s %s", found++ ? "," : "", j->short_description);
      if (column >= 62)
      {
        send_to_char(ch, "%s\r\n", j->next_content ? "," : "");
        found = FALSE;
        column = 0;
      }
    }
    send_to_char(ch, "%s", CCNRM(ch, C_NRM));
  }

  for (i = 0; i < DIR_COUNT; i++)
  {
    char buf1[128];

    if (!rm->dir_option[i])
      continue;

    if (rm->dir_option[i]->to_room == NOWHERE)
      snprintf(buf1, sizeof(buf1), " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    else
      snprintf(buf1, sizeof(buf1), "%s%5d%s", CCCYN(ch, C_NRM), GET_ROOM_VNUM(rm->dir_option[i]->to_room), CCNRM(ch, C_NRM));

    sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2, sizeof(buf2));

    send_to_char(ch, "Exit %s%-5s%s:  To: [%s], Key: [%5d], Keywords: %s, Type: %s\r\n%s",
                 CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1,
                 rm->dir_option[i]->key == NOTHING ? -1 : rm->dir_option[i]->key,
                 rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None", buf2,
                 rm->dir_option[i]->general_description ? rm->dir_option[i]->general_description : "  No exit description.\r\n");
  }

  /* check the room for a script */
  do_sstat_room(ch, rm);

  list_zone_commands_room(ch, rm->number);
}

static void do_featstat_character(struct char_data *ch, struct char_data *k)
{
  list_feats(k, "", LIST_FEATS_KNOWN, ch);
}

static void do_affstat_character(struct char_data *ch, struct char_data *k)
{
  perform_affects(ch, k);
  perform_cooldowns(ch, k);
  perform_resistances(ch, k);
  perform_abilities(ch, k);
}

static void do_stat_scriptvar(struct char_data *ch, struct char_data *k)
{

  /* check mobiles for a script */
  do_sstat_character(ch, k);
  if (SCRIPT_MEM(k))
  {
    struct script_memory *mem = SCRIPT_MEM(k);
    send_to_char(ch, "\tCScript memory:\r\n  Remember             Command\r\n\tn");
    while (mem)
    {
      struct char_data *mc = find_char(mem->id);
      if (!mc)
        send_to_char(ch, "  \tC** Corrupted!\tn\r\n");
      else
      {
        if (mem->cmd)
          send_to_char(ch, "  %-20.20s%s\r\n", GET_NAME(mc), mem->cmd);
        else
          send_to_char(ch, "  %-20.20s <default>\r\n", GET_NAME(mc));
      }
      mem = mem->next;
    }
    send_to_char(ch, "\r\n");
  }

  if (!(IS_NPC(k)))
  {
    /* this is a PC, display their global variables */
    if (k->script && k->script->global_vars)
    {
      struct trig_var_data *tv;
      char uname[MAX_INPUT_LENGTH] = {'\0'};

      send_to_char(ch, "\tCPC Global Variables:\tn\r\n");

      /* currently, variable context for players is always 0, so it is not
       * displayed here. in the future, this might change */
      for (tv = k->script->global_vars; tv; tv = tv->next)
      {
        if (*(tv->value) == UID_CHAR)
        {
          find_uid_name(tv->value, uname, sizeof(uname));
          send_to_char(ch, "    %10s:  \tC[UID]:\tn %s\r\n", tv->name, uname);
        }
        else
          send_to_char(ch, "    %10s:  %s\r\n", tv->name, tv->value);
      }
    }
  }
}

static void do_stat_character(struct char_data *ch, struct char_data *k)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  int i, i2, column, found = FALSE, w_type, counter = 0;
  struct obj_data *j, *wielded = GET_EQ(ch, WEAR_WIELD_1);
  struct follow_type *fol;
  clan_rnum c_n;
  int c_r, index = 0;
  int line_length = 80;

  // get some initial info beforehand
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else
  {
    if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  }

  send_to_char(ch,
               "\tC=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\tn\r\n");

  sprinttype(GET_SEX(k), genders, buf, sizeof(buf));
  send_to_char(ch,
               "\tC%s %s '\tn%s\tC'  IDNum: [\tn%5ld\tC], Loc [\tn%5d\tC/W(\tn%d\tC, \tn%d\tC)], Loadroom : [\tn%5d\tC]\tn\r\n",
               buf, (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
               GET_NAME(k), IS_NPC(k) ? GET_ID(k) : GET_IDNUM(k),
               GET_ROOM_VNUM(IN_ROOM(k)), k->coords[0], k->coords[1], IS_NPC(k) ? NOWHERE : GET_LOADROOM(k));
  if (IS_MOB(k))
  {
    send_to_char(ch, "\tCKeyword:\tn %s\tC, VNum: [\tn%5d\tC], RNum: [\tn%5d\tC]\r\n",
                 k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));
    send_to_char(ch, "\tCL-Des: \tn%s",
                 k->player.long_descr ? k->player.long_descr : "<None>\r\n");
  }

  if (!IS_MOB(k))
    send_to_char(ch, "\tCTitle:\tn %s\r\n", k->player.title ? k->player.title : "<None>");

  send_to_char(ch, "\tCD-Des: \tn%s", k->player.description ? k->player.description : "<None>\r\n");

  if (IS_NPC(k))
  {
    if (GET_RACE(k) >= 0)
      send_to_char(ch, "\tCMobile Race:\tn %s  ", race_family_types[GET_RACE(k)]);
    else
      send_to_char(ch, "\tCRace Undefined\tn  ");
  }
  else if (IS_MORPHED(k))
  {
    send_to_char(ch, "\tCMorphRace:\tn %s  ", race_family_types[IS_MORPHED(k)]);
  }
  else
  {
    send_to_char(ch, "\tCRace:\tn %s  ", RACE_ABBR(k));
  }

  if (IS_NPC(k))
  {
    if (GET_SUBRACE(k, 0))
      send_to_char(ch, "\tCSub-Race:\tn %s / ",
                   npc_subrace_types[GET_SUBRACE(k, 0)]);
    if (GET_SUBRACE(k, 1))
      send_to_char(ch, "%s / ",
                   npc_subrace_types[GET_SUBRACE(k, 1)]);
    if (GET_SUBRACE(k, 2))
      send_to_char(ch, "%s  ",
                   npc_subrace_types[GET_SUBRACE(k, 2)]);
    send_to_char(ch, "\r\n");
  }

  send_to_char(ch, "\tCCrntClass:\tn %s  ", CLSLIST_NAME(GET_CLASS(k)));
  send_to_char(ch, "\tCLvl: [\tn%d\tC]  XP: [\tn%d\tC]  "
                   "Algn: [\tn%s(%d)\tC]\tn\r\n",
               GET_LEVEL(k), GET_EXP(k),
               get_align_by_num(GET_ALIGNMENT(k)), GET_ALIGNMENT(k));

  if (!IS_NPC(k))
  {
    send_to_char(ch, "\tCClass Array:\tn ");
    for (i = 0; i < MAX_CLASSES; i++)
    {
      if (CLASS_LEVEL(k, i))
      {
        if (counter)
          send_to_char(ch, "/");
        send_to_char(ch, "%d%s", CLASS_LEVEL(k, i), CLSLIST_ABBRV(i));
        counter++;
      }
    }

    if (k && k->desc && k->desc->account)
    {
      struct account_data *acc = k->desc->account;

      if (acc && acc->name)
      {
        send_to_char(ch, " Pracs(U): %d, Trains: %d, Acct Name: %s\r\n", GET_PRACTICES(k),
                     GET_TRAINS(k), acc->name);
      }
      else
      {
        send_to_char(ch, " Pracs(U): %d, Trains: %d.\r\n", GET_PRACTICES(k),
                     GET_TRAINS(k));
      }
    }
    else
    {
      send_to_char(ch, " Pracs(U): %d, Trains: %d.\r\n", GET_PRACTICES(k),
                   GET_TRAINS(k));
    }
  }

  send_to_char(ch, "\tCCharacter size: \tn%s  ", size_names[GET_SIZE(k)]);
  send_to_char(ch, "\tCMounted: \tn%s  ", RIDING(k) ? GET_NAME(RIDING(k)) : "None");
  send_to_char(ch, "\tCRidden By: \tn%s\r\n", RIDDEN_BY(k) ? GET_NAME(RIDDEN_BY(k)) : "None");

  send_to_char(ch,
               "\tC=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\tn\r\n");

  if (!IS_NPC(k))
  {
    char buf1[64], buf2[64];

    strlcpy(buf1, asctime(localtime(&(k->player.time.birth))), sizeof(buf1));
    strlcpy(buf2, asctime(localtime(&(k->player.time.logon))), sizeof(buf2));
    buf1[10] = buf2[10] = '\0';

    send_to_char(ch,
                 "\tCCreated: [\tn%s\tC], Last Logon: [\tn%s\tC], Played [\tn%d\tCh \tn%d\tCm], Age [\tn%d\tC]\tn\r\n",
                 buf1, buf2, k->player.time.played / 3600,
                 ((k->player.time.played % 3600) / 60), age(k)->year);

    /* Display OLC zone for immorts. */
    if (GET_LEVEL(k) >= LVL_BUILDER)
    {
      if (GET_OLC_ZONE(k) == AEDIT_PERMISSION)
        send_to_char(ch, "\tC, OLC[\tnAedit\tC]\tn");
      else if (GET_OLC_ZONE(k) == HEDIT_PERMISSION)
        send_to_char(ch, "\tC, OLC[\tnHedit\tC]\tn");
      else if (GET_OLC_ZONE(k) == ALL_PERMISSION)
        send_to_char(ch, "\tC, OLC[\tnAll\tC]\tn");
      else if (GET_OLC_ZONE(k) == NOWHERE)
        send_to_char(ch, "\tC, OLC[\tnOFF\tC]\tn");
      else
        send_to_char(ch, "\tC, OLC[\tn%d\tC]\tn", GET_OLC_ZONE(k));
    }
    send_to_char(ch, "\r\n");
  }

  send_to_char(ch,
               "\tC=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\tn\r\n");

  send_to_char(ch, "\tCStr: [\tn%d\tC]  Int: [\tn%d\tC]  Wis: [\tn%d\tC]\tn  "
                   "\tCDex: [\tn%d\tC]  Con: [\tn%d\tC]  Cha: [\tn%d\tC]\tn\r\n",
               GET_STR(k), GET_INT(k), GET_WIS(k),
               GET_DEX(k), GET_CON(k), GET_CHA(k));

  send_to_char(ch, "\tCHit p.:[\tn%d\tC/\tn%d\tC+\tn%d\tC]  PSP p.:[\tn%d\tC/\tn%d\tC+\tn%d\tC]  Move p.:[\tn%d\tC/\tn%d\tC+\tn%d\tC]\tn\r\n",
               GET_HIT(k), GET_MAX_HIT(k), hit_gain(k),
               GET_PSP(k), GET_MAX_PSP(k), psp_gain(k),
               GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k));

  send_to_char(ch, "\tCGold: [\tn%9d\tC], Bank: [\tn%9d\tC] (Total: \tn%d\tC), \tn",
               GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));

  if (!IS_NPC(k))
    send_to_char(ch, "\tCScreen [\tn%d\tCx\tn%d\tC]\tn",
                 GET_SCREEN_WIDTH(k), GET_PAGE_LENGTH(k));
  send_to_char(ch, "\r\n");

  send_to_char(ch, "\tCAC: [\tn%d\tC/\tn%d\tC], Hitroll: [\tn%d\tC/\tn%d\tC], Damroll: [\tn%d\tC/\tn%d\tC],\tn "
                   "\tCSaving throws: [\tn%d\tC/\tn%d\tC/\tn%d\tC/\tn%d\tC/\tn%d\tC]\tn\r\n",
               GET_AC(k), compute_armor_class(NULL, k, FALSE, MODE_ARMOR_CLASS_NORMAL), GET_HITROLL(k), compute_attack_bonus(k, NULL, ATTACK_TYPE_PRIMARY),
               GET_DAMROLL(k), compute_damage_bonus(k, NULL, NULL, w_type, 0, 0, ATTACK_TYPE_PRIMARY), GET_SAVE(k, 0),
               GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3), GET_SAVE(k, 4));

  send_to_char(ch,
               "\tC=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\tn\r\n");

  if (CLASS_LEVEL(k, CLASS_CLERIC))
  {
    send_to_char(ch, "\tc1st Domain: \tn%s\tc, 2nd Domain: \tn%s\tc.\r\n",
                 domain_list[GET_1ST_DOMAIN(k)].name,
                 domain_list[GET_2ND_DOMAIN(k)].name);
    draw_line(ch, line_length, '-', '-');
  }
  else if (CLASS_LEVEL(ch, CLASS_INQUISITOR))
  {
    send_to_char(ch, "\tc1st Domain: \tn%s\tc.\r\n",
                 domain_list[GET_1ST_DOMAIN(ch)].name);
    draw_line(ch, line_length, '-', '-');
  }

  if (CLASS_LEVEL(k, CLASS_WIZARD))
  {
    send_to_char(ch, "\tcSpecialty School: \tn%s\tc, Restricted: \tn%s\tc.\r\n",
                 school_names[GET_SPECIALTY_SCHOOL(k)],
                 school_names[restricted_school_reference[GET_SPECIALTY_SCHOOL(k)]]);
    draw_line(ch, line_length, '-', '-');
  }

  sprinttype(GET_POS(k), position_types, buf, sizeof(buf));
  send_to_char(ch,
               "\tCPos: \tn%s\tC, Fighting: \tn%s", buf, FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody");

  if (IS_NPC(k))
    send_to_char(ch,
                 "\tC, Attack type: \tn%s", attack_hit_text[(int)k->mob_specials.attack_type].singular);

  if (k->desc)
  {
    sprinttype(STATE(k->desc), connected_types, buf, sizeof(buf));
    send_to_char(ch, "\tC, Connected: \tn%s", buf);
  }

  if (IS_NPC(k))
  {
    sprinttype(k->mob_specials.default_pos, position_types, buf, sizeof(buf));
    send_to_char(ch, "\tC, Default position: \tn%s\r\n", buf);
    sprintbitarray(MOB_FLAGS(k), action_bits, PM_ARRAY_MAX, buf);
    send_to_char(ch, "\tCNPC flags: \tn%s\r\n", buf);
  }
  else
  {
    send_to_char(ch, "\tC, Idle Timer (in tics) [\tn%d\tC]\tn\r\n", k->char_specials.timer);

    sprintbitarray(PLR_FLAGS(k), player_bits, PM_ARRAY_MAX, buf);
    send_to_char(ch, "\tCPLR: \tn%s\r\n", buf);

    sprintbitarray(PRF_FLAGS(k), preference_bits, PR_ARRAY_MAX, buf);
    send_to_char(ch, "\tCPRF: \tn%s\r\n", buf);

    send_to_char(ch, "\tCQuest Points: [\tn%9d\tC] Quests Completed: [\tn%5d\tC]\tn\r\n",
                 GET_QUESTPOINTS(k), GET_NUM_QUESTS(k));

    for (index = 0; index < MAX_CURRENT_QUESTS; index++)
    { /* loop through all the character's quest slots */
      if (GET_QUEST(k, index) == NOTHING)
      {
        send_to_char(ch, "\tCIndex %d, Currently not on a Quest.\tn\r\n", index);
      }
      else
      {
        send_to_char(ch, "\tCIndex %d - Quest: [\tn%5d\tC] Time Left: [\tn%5d\tC]\tn\r\n",
                     index, GET_QUEST(k, index), GET_QUEST_TIME(k, index));
      }
    }

    send_to_char(ch, "\tCacVnum:\tn %d \tC#:\tn %d\tC QP:\tn %d\tC xp:\tn %d\tC "
                     "G:\tn %d\tC Dsc:\tn %s\tC, Mat:\tn %s\r\n",
                 GET_AUTOCQUEST_VNUM(k),
                 GET_AUTOCQUEST_MAKENUM(k),
                 GET_AUTOCQUEST_QP(k),
                 GET_AUTOCQUEST_EXP(k),
                 GET_AUTOCQUEST_GOLD(k),
                 GET_AUTOCQUEST_DESC(k),
                 material_name[GET_AUTOCQUEST_MATERIAL(k)]);
  }

  if (IS_MOB(k))
    send_to_char(ch, "\tCMob Spec-Proc: \tn%s\tC, NPC Bare Hand Dam: \tn%d\tCd\tn%d\r\n",
                 (mob_index[GET_MOB_RNUM(k)].func ? get_spec_func_name(mob_index[GET_MOB_RNUM(k)].func) : "None"),
                 k->mob_specials.damnodice, k->mob_specials.damsizedice);

  for (i = 0, j = k->carrying; j; j = j->next_content, i++)
    ;
  send_to_char(ch, "\tCCarried: weight: \tn%d\tC, items: \tn%d\tC; Items in: inventory: \tn%d\tC, ", IS_CARRYING_W(k), IS_CARRYING_N(k), i);

  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (GET_EQ(k, i))
      i2++;
  send_to_char(ch, "\tCeq: \tn%d\r\n", i2);

  send_to_char(ch,
               "\tC=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\tn\r\n");

  if (!IS_NPC(k))
    send_to_char(ch, "\tCHunger: \tn%d\tC, Thirst: \tn%d\tC, Drunk: \tn%d\tC.\tn", GET_COND(k, HUNGER),
                 GET_COND(k, THIRST), GET_COND(k, DRUNK));

  send_to_char(ch, "  \tCDR:\tn %d\tC, CM%%:\tn %d\tC | Trlx WpnPsn: %d/%d/%d.\tn\r\n",
               compute_damage_reduction(k, -1),
               compute_concealment(k),
               TRLX_PSN_VAL(k),
               TRLX_PSN_LVL(k),
               TRLX_PSN_HIT(k));

  send_to_char(ch, "\tCStoneskin: \tn%d\tC, Mirror Images: \tn%d\tC, Cloudkill/Inc/Doom:"
                   " \tn%d/%d/%d\tC, Spell Resist: \tn%d\r\n",
               GET_STONESKIN(k), GET_IMAGES(k), CLOUDKILL(k), INCENDIARY(k), DOOM(k),
               compute_spell_res(ch, k, 0));

  send_to_char(ch, "\tCMemming? \tn%d\tC, Praying? \tn%d\tC, Communing? \tn%d\tC,"
                   " Meditating? \tn%d\tn\r\n",
               IS_PREPARING(k, 2), IS_PREPARING(k, 0), IS_PREPARING(k, 1), IS_PREPARING(k, 3));

  if (!IS_NPC(k))
    send_to_char(ch, "\tCWimpy:\tn %d  ", GET_WIMP_LEV(k));
  send_to_char(ch, "\tCDivLvl:\tn %d  \tCMgcLvl:\tn %d"
                   "  \tCCstrLvl:\tn %d\r\n",
               DIVINE_LEVEL(k), MAGIC_LEVEL(k),
               CASTER_LEVEL(k));

  send_to_char(ch,
               "\tC=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\tn\r\n");

  column = send_to_char(ch, "\tCMaster is: \tn%s\tC, Followers are:\tn", k->master ? GET_NAME(k->master) : "<none>");
  if (!k->followers)
    send_to_char(ch, " <none>\r\n");
  else
  {
    for (fol = k->followers; fol; fol = fol->next)
    {
      column += send_to_char(ch, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
      if (column >= 62)
      {
        send_to_char(ch, "%s\r\n", fol->next ? "," : "");
        found = FALSE;
        column = 0;
      }
    }
    if (column != 0)
      send_to_char(ch, "\r\n");
  }

  if (PATH_SIZE(k))
  {
    send_to_char(ch, "Path Index: \tc%d\tn  Delay/Reset \tc%d/%d\tn\r\n",
                 PATH_INDEX(k), PATH_DELAY(k), PATH_RESET(k));
    send_to_char(ch, "Path: \tc");
    for (i = 0; i < PATH_SIZE(k); i++)
      send_to_char(ch, "%d ", GET_PATH(k, i));
    send_to_char(ch, "\tn\r\n");
  }

  if (IS_NPC(k))
  {
    memory_rec *names;
    send_to_char(ch, "\tcMEMORY:\tC");
    for (names = MEMORY(k); names; names = names->next)
      send_to_char(ch, "%ld ", names->id);
    send_to_char(ch, "\tn\r\n");
  }

  send_to_char(ch,
               "\tC=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\tn\r\n");

  if (!IS_NPC(k) && (GET_LEVEL(k) >= LVL_IMMORT))
  {
    if (POOFIN(k))
      send_to_char(ch, "\tCPOOFIN:\tn  %s %s\tn\r\n", GET_NAME(k), POOFIN(k));
    else
      send_to_char(ch, "\tCPOOFIN:\tn  %s appears with an ear-splitting bang.\r\n", GET_NAME(k));

    if (POOFOUT(k))
      send_to_char(ch, "\tCPOOFOUT:\tn %s %s\tn\r\n", GET_NAME(k), POOFOUT(k));
    else
      send_to_char(ch, "\tCPOOFOUT:\tn %s disappears in a puff of smoke.\r\n", GET_NAME(k));
  }

  if (!IS_NPC(k) && IS_IN_CLAN(k))
  {
    c_n = real_clan(GET_CLAN(k));
    if ((c_r = GET_CLANRANK(k)) == NO_CLANRANK)
    {
      send_to_char(ch, "Applied to : %s%s\r\n", CLAN_NAME(c_n), QNRM);
      send_to_char(ch, "Status     : %sAwaiting Approval%s\r\n", QBRED, QNRM);
    }
    else
    {
      send_to_char(ch, "Current Clan : %s%s\r\n", CLAN_NAME(c_n), QNRM);
      send_to_char(ch, "Clan Rank    : %s%s (Rank %d)\r\n", clan_list[c_n].rank_name[(c_r - 1)], QNRM, c_r);
    }
    if (CLAN_LEADER(c_n) == GET_IDNUM(k))
    {
      send_to_char(ch, "Other Info   : %s%s is the leader of this clan!%s\r\n", QBWHT, GET_NAME(k), QNRM);
    }
    send_to_char(ch, "Clan Points  : %d\r\n", GET_CLANPOINTS(k));
  }

  send_to_char(ch,
               "\tC=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\tn\r\n");
}

ACMD(do_stat)
{
  char buf1[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *victim = NULL;
  struct obj_data *object = NULL;
  struct room_data *room = NULL;

  half_chop_c(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

  if (!*buf1)
  {
    send_to_char(ch, "Stats on who or what or where?\r\n");
    return;

    /* stat room */
  }
  else if (is_abbrev(buf1, "room"))
  {
    if (!*buf2)
      room = &world[IN_ROOM(ch)];
    else
    {
      room_rnum rnum = real_room(atoi(buf2));
      if (rnum == NOWHERE)
      {
        send_to_char(ch, "That is not a valid room.\r\n");
        return;
      }
      room = &world[rnum];
    }
    do_stat_room(ch, room);

    /* stat mobile */
  }
  else if (is_abbrev(buf1, "mob"))
  {
    if (!*buf2)
      send_to_char(ch, "Stats on which mobile?\r\n");
    else
    {
      if ((victim = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
        do_stat_character(ch, victim);
      else
        send_to_char(ch, "No such mobile around.\r\n");
    }

    /* stat player */
  }
  else if (is_abbrev(buf1, "player"))
  {
    if (!*buf2)
    {
      send_to_char(ch, "Stats on which player?\r\n");
    }
    else
    {
      if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
        do_stat_character(ch, victim);
      else
        send_to_char(ch, "No such player around.\r\n");
    }

    /* stat scripts / variables */
  }
  else if (is_abbrev(buf1, "scriptvar"))
  {
    if (!*buf2)
    {
      send_to_char(ch, "Scripts / Variables on which mobile / player?\r\n");
    }
    else
    {
      if ((victim = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
        do_stat_scriptvar(ch, victim);
      else
        send_to_char(ch, "No such player around.\r\n");
    }

    /* stat feat */
  }
  else if (is_abbrev(buf1, "feats"))
  {
    if (!*buf2)
    {
      send_to_char(ch, "Feats on which player/mobile?\r\n");
    }
    else
    {
      if ((victim = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
        do_featstat_character(ch, victim);
      else
        send_to_char(ch, "No such player around.\r\n");
    }

    /* stat affects */
  }
  else if (is_abbrev(buf1, "affect"))
  {
    if (!*buf2)
    {
      send_to_char(ch, "Affects on which player/mobile?\r\n");
    }
    else
    {
      if ((victim = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
        do_affstat_character(ch, victim);
      else
        send_to_char(ch, "No such player around.\r\n");
    }

    /* stat account */
  }
  else if (is_abbrev(buf1, "account"))
  {
    if (!*buf2)
    {
      send_to_char(ch, "Account on which player?\r\n");
    }
    else
    {
      if ((victim = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
        perform_do_account(ch, victim);
      else
        send_to_char(ch, "No such player around.\r\n");
    }

    /* stat (player rent) file */
  }
  else if (is_abbrev(buf1, "file"))
  {
    if (!*buf2)
      send_to_char(ch, "Stats on which player?\r\n");
    else if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
      do_stat_character(ch, victim);
    else
    {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      CREATE(victim->player_specials, struct player_special_data, 1);
      /* Allocate mobile event list */
      //      victim->events = create_list();
      new_mobile_data(victim);
      if (load_char(buf2, victim) >= 0)
      {
        char_to_room(victim, 0);
        if (GET_LEVEL(victim) > GET_LEVEL(ch))
          send_to_char(ch, "Sorry, you can't do that.\r\n");
        else
          do_stat_character(ch, victim);
        extract_char_final(victim);
      }
      else
      {
        send_to_char(ch, "There is no such player.\r\n");
        free_char(victim);
      }
    }

    /* stat object */
  }
  else if (is_abbrev(buf1, "object"))
  {
    if (!*buf2)
      send_to_char(ch, "Stats on which object?\r\n");
    else
    {
      if ((object = get_obj_vis(ch, buf2, NULL)) != NULL)
        do_stat_object(ch, object, ITEM_STAT_MODE_IMMORTAL);
      else
        send_to_char(ch, "No such object around.\r\n");
    }

    /* stat zone */
  }
  else if (is_abbrev(buf1, "zone"))
  {
    if (!*buf2)
    {
      print_zone(ch, zone_table[world[IN_ROOM(ch)].zone].number);
      return;
    }
    else
    {
      print_zone(ch, atoi(buf2));
      return;
    }

    /* generic search, object or characters */
  }
  else
  {
    char *name = buf1;
    int number = get_number(&name);

    if ((object = get_obj_in_equip_vis(ch, name, &number, ch->equipment)) != NULL)
      do_stat_object(ch, object, ITEM_STAT_MODE_IMMORTAL);
    else if ((object = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      do_stat_object(ch, object, ITEM_STAT_MODE_IMMORTAL);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_ROOM)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      do_stat_object(ch, object, ITEM_STAT_MODE_IMMORTAL);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_WORLD)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, name, &number)) != NULL)
      do_stat_object(ch, object, ITEM_STAT_MODE_IMMORTAL);
    else
      send_to_char(ch, "Nothing around by that name.\r\n");
  }
}

ACMD(do_shutdown)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  if (subcmd != SCMD_SHUTDOWN)
  {
    send_to_char(ch, "If you want to shut something down, say so!\r\n");
    return;
  }
  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    log("(GC) Shutdown by %s (SLOWBOOT).", GET_NAME(ch));
    send_to_all("Shutting dowscript will rebootn, script will reboot Luminari unless stated"
                " otherwise by %s.\r\n",
                GET_NAME(ch));
    circle_shutdown = 1;
  }
  else if (!str_cmp(arg, "reboot"))
  {
    log("(GC) Reboot by %s (FASTBOOT, OLC NOT SAVED).", GET_NAME(ch));
    send_to_all("Fastboot by %s.. come back in a few minutes.\r\n", GET_NAME(ch));
    touch(FASTBOOT_FILE);
    circle_shutdown = 1;
    circle_reboot = 2; /* do not autosave olc */
  }
  else if (!str_cmp(arg, "die"))
  {
    log("(GC) Shutdown by %s (KILLSCRIPT).", GET_NAME(ch));
    send_to_all("Shutting down for maintenance, please check back in 15 minutes.\r\n");
    touch(KILLSCRIPT_FILE);
    circle_shutdown = 1;
  }
  else if (!str_cmp(arg, "now"))
  {
    log("(GC) Shutdown NOW (OLC NOT SAVED) by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    circle_shutdown = 1;
    circle_reboot = 2; /* do not autosave olc */
  }
  else if (!str_cmp(arg, "pause"))
  {
    log("(GC) Shutdown PAUSE by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance, please check back in 10 minutes.\r\n");
    touch(PAUSE_FILE);
    circle_shutdown = 1;
  }
  else
    send_to_char(ch, "Unknown shutdown option.\r\n");
}

void snoop_check(struct char_data *ch)
{
  /*  This short routine is to ensure that characters that happen to be snooping
   *  (or snooped) and get advanced/demoted will not be snooping/snooped someone
   *  of a higher/lower level (and thus, not entitled to be snooping. */
  if (!ch || !ch->desc)
    return;
  if (ch->desc->snooping &&
      (GET_LEVEL(ch->desc->snooping->character) >= GET_LEVEL(ch)))
  {
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }

  if (ch->desc->snoop_by &&
      (GET_LEVEL(ch) >= GET_LEVEL(ch->desc->snoop_by->character)))
  {
    ch->desc->snoop_by->snooping = NULL;
    ch->desc->snoop_by = NULL;
  }
}

static void stop_snooping(struct char_data *ch)
{
  if (!ch->desc->snooping)
    send_to_char(ch, "You aren't snooping anyone.\r\n");
  else
  {
    send_to_char(ch, "You stop snooping.\r\n");

    if (GET_LEVEL(ch) < LVL_IMPL)
      mudlog(BRF, GET_LEVEL(ch), TRUE, "(GC) %s stops snooping", GET_NAME(ch));

    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}

ACMD(do_snoop)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such person around.\r\n");
  else if (!victim->desc)
    send_to_char(ch, "There's no link.. nothing to snoop.\r\n");
  else if (victim == ch)
    stop_snooping(ch);
  else if (victim->desc->snoop_by)
    send_to_char(ch, "Busy already. \r\n");
  else if (victim->desc->snooping == ch->desc)
    send_to_char(ch, "Don't be stupid.\r\n");
  else
  {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_LEVEL(tch) >= GET_LEVEL(ch))
    {
      send_to_char(ch, "You can't.\r\n");
      return;
    }
    send_to_char(ch, "%s", CONFIG_OK);

    if (GET_LEVEL(ch) < LVL_IMPL)
      mudlog(BRF, GET_LEVEL(ch), TRUE, "(GC) %s snoops %s", GET_NAME(ch), GET_NAME(victim));

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}

ACMD(do_switch)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *victim;

  /* temporarily disabled while adapting to wilderness */
  // send_to_char(ch, "Under construction.\r\n");   /**/
  // return;                                        /**/
  /* temporarily disabled while adapting to wilderness */

  one_argument(argument, arg, sizeof(arg));

  if (ch->desc->original)
    send_to_char(ch, "You're already switched.\r\n");
  else if (!*arg)
    send_to_char(ch, "Switch with who?\r\n");
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such character.\r\n");
  else if (ch == victim)
    send_to_char(ch, "Hee hee... we are jolly funny today, eh?\r\n");
  else if (victim->desc)
    send_to_char(ch, "You can't do that, the body is already in use!\r\n");
  else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
    send_to_char(ch, "You aren't holy enough to use a mortal's body.\r\n");
  else if (GET_LEVEL(ch) < LVL_GRSTAFF && ROOM_FLAGGED(IN_ROOM(victim), ROOM_STAFFROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n");
  else if (GET_LEVEL(ch) < LVL_GRSTAFF && ROOM_FLAGGED(IN_ROOM(victim), ROOM_HOUSE) && !House_can_enter(ch, GET_ROOM_VNUM(IN_ROOM(victim))))
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
  else
  {
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(CMP, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s Switched into: %s", GET_NAME(ch), GET_NAME(victim));
    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}

void do_cheat(struct char_data *ch)
{
  switch (GET_IDNUM(ch))
  {
  case 1: // IMP
    send_to_char(ch, "Your level has been restored, for now!\r\n");
    GET_LEVEL(ch) = LVL_IMPL;
    break;
  default:
    send_to_char(ch, "You do not have access to this command.\r\n");
    return;
  }
  /* just in case, this is called possibly in extract_char_final()
     this will keep from saving events */
  save_char(ch, 1);
}

ACMD(do_return)
{
  if (!IS_NPC(ch) && !ch->desc->original)
  {
    int level, newlevel;
    level = GET_LEVEL(ch);
    do_cheat(ch);
    newlevel = GET_LEVEL(ch);
    if (!PLR_FLAGGED(ch, PLR_NOWIZLIST) && level != newlevel)
      run_autowiz();
  }

  if (ch->desc && ch->desc->original)
  {
    send_to_char(ch, "You return to your original body.\r\n");

    /* If someone switched into your original body, disconnect them. - JE
     * Zmey: here we put someone switched in our body to disconnect state but
     * we must also NULL his pointer to our character, otherwise close_socket()
     * will damage our character's pointer to our descriptor (which is assigned
     * below in this function). */
    if (ch->desc->original->desc)
    {
      ch->desc->original->desc->character = NULL;
      STATE(ch->desc->original->desc) = CON_DISCONNECT;
    }

    /* Now our descriptor points to our original body. */
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    /* And our body's pointer to descriptor now points to our descriptor. */
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
}

ACMD(do_load)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'}, buf3[MAX_INPUT_LENGTH] = {'\0'};
  int i = 0, n = 1;

  one_argument(two_arguments(argument, buf, sizeof(buf), buf2, sizeof(buf2)), buf3, sizeof(buf3));

  if (!*buf || !*buf2 || !isdigit(*buf2))
  {
    send_to_char(ch, "Usage: load < obj | mob > <vnum> <number>\r\n");
    return;
  }
  if (!is_number(buf2))
  {
    send_to_char(ch, "That is not a number.\r\n");
    return;
  }

  if (atoi(buf3) > 0 && atoi(buf3) <= 100)
  {
    n = atoi(buf3);
  }
  else
  {
    n = 1;
  }

  if (is_abbrev(buf, "mob"))
  {
    struct char_data *mob = NULL;
    mob_rnum r_num;

    if (GET_LEVEL(ch) < LVL_GRSTAFF && !can_edit_zone(ch, world[IN_ROOM(ch)].zone))
    {
      send_to_char(ch, "Sorry, you can't load mobs here.\r\n");
      return;
    }

    if ((r_num = real_mobile(atoi(buf2))) == NOBODY)
    {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    for (i = 0; i < n; i++)
    {
      mob = read_mobile(r_num, REAL);

      if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
      {
        X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
        Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
      }

      char_to_room(mob, IN_ROOM(ch));

      act("$n makes a quaint, magical gesture with one hand.", TRUE, ch, 0, 0, TO_ROOM);
      act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
      act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
      load_mtrigger(mob);
    }
  }
  else if (is_abbrev(buf, "obj"))
  {
    struct obj_data *obj;
    obj_rnum r_num;

    if (GET_LEVEL(ch) < LVL_GRSTAFF && !can_edit_zone(ch, world[IN_ROOM(ch)].zone))
    {
      send_to_char(ch, "Sorry, you can't load objects here.\r\n");
      return;
    }

    if ((r_num = real_object(atoi(buf2))) == NOTHING)
    {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    for (i = 0; i < n; i++)
    {
      obj = read_object(r_num, REAL);
      if (CONFIG_LOAD_INVENTORY)
        obj_to_char(obj, ch);
      else
        obj_to_room(obj, IN_ROOM(ch));
      act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
      act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
      act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
      load_otrigger(obj);
    }
  }
  else
    send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_vstat)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *mob;
  struct obj_data *obj;
  int r_num;

  ACMD_DECL(do_tstat);

  two_arguments(argument, buf, sizeof(buf), buf2, sizeof(buf2));

  if (!*buf || !*buf2 || !isdigit(*buf2))
  {
    send_to_char(ch, "Usage: vstat { o | m | r | t | s | z } <number>\r\n");
    return;
  }
  if (!is_number(buf2))
  {
    send_to_char(ch, "That's not a valid number.\r\n");
    return;
  }

  switch (LOWER(*buf))
  {
  case 'm':
    if ((r_num = real_mobile(atoi(buf2))) == NOBODY)
    {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob);
    break;
  case 'o':
    if ((r_num = real_object(atoi(buf2))) == NOTHING)
    {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    obj = read_object(r_num, REAL);
    do_stat_object(ch, obj, ITEM_STAT_MODE_IMMORTAL);
    extract_obj(obj);
    break;
  case 'r':
    snprintf(buf2, sizeof(buf2), "room %d", atoi(buf2));
    do_stat(ch, buf2, 0, 0);
    break;
  case 'z':
    snprintf(buf2, sizeof(buf2), "zone %d", atoi(buf2));
    do_stat(ch, buf2, 0, 0);
    break;
  case 't':
    snprintf(buf2, sizeof(buf2), "%d", atoi(buf2));
    do_tstat(ch, buf2, 0, 0);
    break;
  case 's':
    snprintf(buf2, sizeof(buf2), "shops %d", atoi(buf2));
    do_show(ch, buf2, 0, 0);
    break;
  default:
    send_to_char(ch, "Syntax: vstat { r | m | o | z | t | s } <number>\r\n");
    break;
  }
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  char *t;
  struct char_data *vict;
  struct obj_data *obj;
  int number;

  one_argument(argument, buf, sizeof(buf));

  if (GET_LEVEL(ch) < LVL_GRSTAFF &&
      !can_edit_zone(ch, world[IN_ROOM(ch)].zone))
  {
    send_to_char(ch, "Sorry, you can't purge anything here.\r\n");
    return;
  }

  /* argument supplied. destroy single object or char */
  if (*buf)
  {
    t = buf;
    number = get_number(&t);
    if ((vict = get_char_vis(ch, buf, &number, FIND_CHAR_ROOM)) != NULL)
    {
      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict)))
      {
        send_to_char(ch, "You can't purge %s!\r\n", HMHR(vict));
        return;
      }

      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_STAFF)
      {
        mudlog(BRF, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE,
               "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
        if (vict->desc)
        {
          STATE(vict->desc) = CON_CLOSE;
          vict->desc->character = NULL;
          vict->desc = NULL;
        }
      }

      extract_char(vict);
    }
    else if ((obj = get_obj_in_list_vis(ch, buf, &number,
                                        world[IN_ROOM(ch)].contents)) != NULL)
    {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    }
    else
    {
      send_to_char(ch, "Nothing here by that name.\r\n");
      return;
    }

    send_to_char(ch, "%s", CONFIG_OK);
  }
  else
  { /* no argument. clean out the room */
    act("$n gestures... You are surrounded by scorching flames!",
        FALSE, ch, 0, 0, TO_ROOM);
    send_to_room(IN_ROOM(ch), "The world seems a little cleaner.\r\n");
    purge_room(IN_ROOM(ch));
  }
}

ACMD(do_advance)
{
  struct char_data *victim;
  char name[MAX_INPUT_LENGTH] = {'\0'}, level[MAX_INPUT_LENGTH] = {'\0'};
  int newlevel, oldlevel, i;

  two_arguments(argument, name, sizeof(name), level, sizeof(level));

  if (*name)
  {
    if (!(victim = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD)))
    {
      send_to_char(ch, "That player is not here.\r\n");
      return;
    }
  }
  else
  {
    send_to_char(ch, "Advance who?\r\n");
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim))
  {
    send_to_char(ch, "Maybe that's not such a great idea.\r\n");
    return;
  }
  if (IS_NPC(victim))
  {
    send_to_char(ch, "NO!  Not on NPC's.\r\n");
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0)
  {
    send_to_char(ch, "That's not a level!\r\n");
    return;
  }
  if (newlevel > LVL_IMPL)
  {
    send_to_char(ch, "%d is the highest possible level.\r\n", LVL_IMPL);
    return;
  }
  if (newlevel > GET_LEVEL(ch))
  {
    send_to_char(ch, "Yeah, right.\r\n");
    return;
  }
  if (newlevel == GET_LEVEL(victim))
  {
    send_to_char(ch, "They are already at that level.\r\n");
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim))
  {
    do_start(victim);
    if (newlevel == 1)
      newbieEquipment(victim);
    send_to_char(victim, "You are momentarily enveloped by darkness!\r\nYou feel somewhat diminished.\r\n");
  }
  else
  {

    act("$n makes some strange gestures. A strange feeling comes upon you,\r\n"
        "Like a giant hand, light comes down from above, grabbing your body,\r\n"
        "that begins to pulse with colored lights from inside.\r\n\r\n"
        "Your head seems to be filled with demons from another plane as\r\n"
        "your body dissolves to the elements of time and space itself.\r\n"
        "Suddenly a silent explosion of light snaps you back to reality.\r\n\r\n"
        "You feel slightly different.",
        FALSE, ch, 0, victim, TO_VICT);
  }

  send_to_char(ch, "%s", CONFIG_OK);

  if (newlevel < oldlevel)
    log("(GC) %s demoted %s from level %d to %d.",
        GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
  else
    log("(GC) %s has advanced %s to level %d (from %d)",
        GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

  if (oldlevel >= LVL_IMMORT && newlevel < LVL_IMMORT)
  {
    /* If they are no longer an immortal, remove the immortal only flags. */
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_LOG1);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_LOG2);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_NOHASSLE);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_HOLYLIGHT);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_SHOWVNUMS);
    if (!PLR_FLAGGED(victim, PLR_NOWIZLIST))
      run_autowiz();
  }
  else if (oldlevel < LVL_IMMORT && newlevel >= LVL_IMMORT)
  {
    SET_BIT_AR(PRF_FLAGS(victim), PRF_LOG2);
    SET_BIT_AR(PRF_FLAGS(victim), PRF_HOLYLIGHT);
    SET_BIT_AR(PRF_FLAGS(victim), PRF_SHOWVNUMS);
    SET_BIT_AR(PRF_FLAGS(victim), PRF_AUTOEXIT);
    for (i = 1; i < MAX_SKILLS; i++)
      SET_SKILL(victim, i, 100);
    for (i = 1; i <= MAX_ABILITIES; i++)
      SET_ABILITY(victim, i, 40);
    GET_OLC_ZONE(victim) = NOWHERE;
    GET_COND(victim, HUNGER) = -1;
    GET_COND(victim, THIRST) = -1;
    GET_COND(victim, DRUNK) = -1;
  }

  gain_exp_regardless(victim, level_exp(victim, newlevel) - GET_EXP(victim), FALSE);
  save_char(victim, 0);
}

ACMD(do_restore)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;
  struct descriptor_data *j;
  int i;

  one_argument(argument, buf, sizeof(buf));

  if (!*buf)
    send_to_char(ch, "Whom do you wish to restore?\r\n");
  else if (is_abbrev(buf, "all"))
  {
    mudlog(NRM, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s restored all", GET_NAME(ch));

    for (j = descriptor_list; j; j = j->next)
    {
      if (!IS_PLAYING(j) || !(vict = j->character) || GET_LEVEL(vict) >= LVL_IMMORT)
        continue;

      GET_HIT(vict) = GET_MAX_HIT(vict);
      GET_PSP(vict) = GET_MAX_PSP(vict);
      GET_MOVE(vict) = GET_MAX_MOVE(vict);

      if (GET_COND(vict, HUNGER) != -1)
        GET_COND(vict, HUNGER) = 24;
      if (GET_COND(vict, THIRST) != -1)
        GET_COND(vict, THIRST) = 24;

      update_pos(vict);
      send_to_char(ch, "%s has been fully healed.\r\n", GET_NAME(vict));
      act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
    }
  }
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (!IS_NPC(vict) && ch != vict && GET_LEVEL(vict) >= GET_LEVEL(ch))
    send_to_char(ch, "They don't need your help.\r\n");
  else
  {
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_PSP(vict) = GET_MAX_PSP(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);

    if (!IS_NPC(vict) && GET_COND(vict, HUNGER) != -1)
      GET_COND(vict, HUNGER) = 24;
    if (!IS_NPC(vict) && GET_COND(vict, THIRST) != -1)
      GET_COND(vict, THIRST) = 24;

    if (!IS_NPC(vict) && GET_LEVEL(ch) >= LVL_GRSTAFF)
    {
      if (GET_LEVEL(vict) >= LVL_IMMORT)
        for (i = 1; i < MAX_SKILLS; i++)
          SET_SKILL(vict, i, 100);

      if (GET_LEVEL(vict) >= LVL_IMMORT)
        for (i = 1; i <= MAX_ABILITIES; i++)
          SET_ABILITY(vict, i, 40);

      if (GET_LEVEL(vict) >= LVL_GRSTAFF)
      {
        if (GET_REAL_INT(vict) < 25)
          GET_REAL_INT(vict) = 25;
        if (GET_REAL_WIS(vict) < 25)
          GET_REAL_WIS(vict) = 25;
        if (GET_REAL_CHA(vict) < 25)
          GET_REAL_CHA(vict) = 25;
        if (GET_REAL_STR(vict) < 25)
          GET_REAL_STR(vict) = 25;
        if (GET_REAL_DEX(vict) < 25)
          GET_REAL_DEX(vict) = 25;
        if (GET_REAL_CON(vict) < 25)
          GET_REAL_CON(vict) = 25;
        GET_SPELL_RES(ch) = 0;
      }
    }

    /* this helps for testing */
    bool found = FALSE;
    if (GET_LEVEL(vict) >= LVL_IMMORT)
    {
      IS_MORPHED(ch) = 0;
      SUBRACE(ch) = 0;
      GET_DISGUISE_RACE(ch) = 0;
      for (i = 1; i < NUM_FEATS; i++)
      {
        if (!HAS_FEAT(ch, i))
        {
          SET_FEAT(ch, i, 1);
          found = TRUE;
        }
      }
    }
    if (found)
      send_to_char(ch, "Your feats have been updated.\r\n");

    update_pos(vict);
    affect_total(vict);
    send_to_char(ch, "%s", CONFIG_OK);
    act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
  }
}

void perform_immort_vis(struct char_data *ch)
{
  struct char_data *tch = NULL;

  if ((GET_INVIS_LEV(ch) == 0) && (!AFF_FLAGGED(ch, AFF_HIDE) &&
                                   !AFF_FLAGGED(ch, AFF_INVISIBLE)))
  {
    send_to_char(ch, "You are already fully visible.\r\n");
    return;
  }

  GET_INVIS_LEV(ch) = 0;
  appear(ch, TRUE);
  send_to_char(ch, "You are now fully visible.\r\n");
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch || IS_NPC(tch))
      continue;
    act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
        tch, TO_VICT);
  }
}

static void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch || IS_NPC(tch))
      continue;
    if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
      act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
          tch, TO_VICT);
    if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
      act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
          tch, TO_VICT);
  }

  GET_INVIS_LEV(ch) = level;
  send_to_char(ch, "Your invisibility level is %d.\r\n", level);
}

ACMD(do_invis)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int level;

  if (IS_NPC(ch))
  {
    send_to_char(ch, "You can't do that!\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_LEVEL(ch));
  }
  else
  {
    level = atoi(arg);
    if (level > GET_LEVEL(ch))
      send_to_char(ch, "You can't go invisible above your own level.\r\n");
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}

ACMDU(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char(ch, "That must be a mistake...\r\n");
  else
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (IS_PLAYING(pt) && pt->character && pt->character != ch)
        send_to_char(pt->character, "%s\r\n", argument);

    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s gechoed: %s", GET_NAME(ch), argument);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "%s\r\n", argument);
  }
}

ACMD(do_dc)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg, sizeof(arg));
  if (!(num_to_dc = atoi(arg)))
  {
    send_to_char(ch, "Usage: DC <user number> (type USERS for a list)\r\n");
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next)
    ;

  if (!d)
  {
    send_to_char(ch, "No such connection.\r\n");
    return;
  }
  if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch))
  {
    if (!CAN_SEE(ch, d->character))
      send_to_char(ch, "No such connection.\r\n");
    else
      send_to_char(ch, "Umm.. maybe that's not such a good idea...\r\n");
    return;
  }

  /* We used to just close the socket here using close_socket(), but various
   * people pointed out this could cause a crash if you're closing the person
   * below you on the descriptor list.  Just setting to CON_CLOSE leaves things
   * in a massively inconsistent state so I had to add this new flag to the
   * descriptor. -je It is a much more logical extension for a CON_DISCONNECT
   * to be used for in-game socket closes and CON_CLOSE for out of game
   * closings. This will retain the stability of the close_me hack while being
   * neater in appearance. -gg For those unlucky souls who actually manage to
   * get disconnected by two different immortals in the same 1/10th of a
   * second, we have the below 'if' check. -gg */
  if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
    send_to_char(ch, "They're already being disconnected.\r\n");
  else
  {
    /* Remember that we can disconnect people not in the game and that rather
     * confuses the code when it expected there to be a character context. */
    if (STATE(d) == CON_PLAYING)
      STATE(d) = CON_DISCONNECT;
    else
      STATE(d) = CON_CLOSE;

    send_to_char(ch, "Connection #%d closed.\r\n", num_to_dc);
    log("(GC) Connection closed by %s.", GET_NAME(ch));
  }
}

ACMD(do_wizlock)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int value;
  const char *when;

  one_argument(argument, arg, sizeof(arg));
  if (*arg)
  {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch))
    {
      send_to_char(ch, "Invalid wizlock value.\r\n");
      return;
    }
    circle_restrict = value;
    when = "now";
  }
  else
    when = "currently";

  switch (circle_restrict)
  {
  case 0:
    send_to_char(ch, "The game is %s completely open.\r\n", when);
    break;
  case 1:
    send_to_char(ch, "The game is %s closed to new players.\r\n", when);
    break;
  default:
    send_to_char(ch, "Only level %d and above may enter the game %s.\r\n", circle_restrict, when);
    break;
  }
}

ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  tmstr = (char *)asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE)
  {
    send_to_char(ch, "Current machine time: %s\r\n", tmstr);
  }
  else
  {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    send_to_char(ch, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d, d == 1 ? "" : "s", h, m);
  }
}

/* altered from stock to the following:
   last [name] [#]
   last without arguments displays the last 10 entries.
   last with a name only displays the 'stock' last entry.
   last with a number displays that many entries (combines with name) */
const char *last_array[11] = {
    "Connect",
    "Enter Game",
    "Reconnect",
    "Takeover",
    "Quit",
    "Idleout",
    "Disconnect",
    "Shutdown",
    "Reboot",
    "Crash",
    "Playing"};

struct last_entry *find_llog_entry(int punique, long idnum)
{
  FILE *fp;
  struct last_entry mlast;
  struct last_entry *llast;
  int size, recs, tmp;

  if (!(fp = fopen(LAST_FILE, "r")))
  {
    log("Error opening last_file for reading, will create.");
    return NULL;
  }
  fseek(fp, 0L, SEEK_END);
  size = ftell(fp);

  /* recs = number of records in the last file */
  recs = size / sizeof(struct last_entry);
  /* we'll search last to first, since it's faster than any thing else we can
   * do (like searching for the last shutdown/etc..) */
  for (tmp = recs - 1; tmp > 0; tmp--)
  {
    fseek(fp, -1 * (sizeof(struct last_entry)), SEEK_CUR);
    if (fread(&mlast, sizeof(struct last_entry), 1, fp) != 1)
      return NULL;
    /*another one to keep that stepback */
    fseek(fp, -1 * (sizeof(struct last_entry)), SEEK_CUR);

    if (mlast.idnum == idnum && mlast.punique == punique)
    {
      /* then we've found a match */
      CREATE(llast, struct last_entry, 1);
      memcpy(llast, &mlast, sizeof(struct last_entry));
      fclose(fp);
      return llast;
    }
    /*not the one we seek. next */
  }
  /*not found, no problem, quit */
  fclose(fp);
  return NULL;
}

/* mod_llog_entry assumes that llast is accurate */
static void mod_llog_entry(struct last_entry *llast, int type)
{
  FILE *fp;
  struct last_entry mlast;
  int size, recs, tmp, i, j;

  if (!(fp = fopen(LAST_FILE, "r+")))
  {
    log("Error opening last_file for reading and writing.");
    return;
  }
  fseek(fp, 0L, SEEK_END);
  size = ftell(fp);

  /* recs = number of records in the last file */
  recs = size / sizeof(struct last_entry);

  /* We'll search last to first, since it's faster than any thing else we can
   * do (like searching for the last shutdown/etc..) */
  for (tmp = recs; tmp > 0; tmp--)
  {
    fseek(fp, -1 * (sizeof(struct last_entry)), SEEK_CUR);
    i = fread(&mlast, sizeof(struct last_entry), 1, fp);
    /* Another one to keep that stepback. */
    fseek(fp, -1 * (sizeof(struct last_entry)), SEEK_CUR);

    if (mlast.idnum == llast->idnum && mlast.punique == llast->punique)
    {
      /* Then we've found a match, lets assume quit is inviolate, mainly
       * because disconnect is called after each of these */
      if (mlast.close_type != LAST_QUIT &&
          mlast.close_type != LAST_IDLEOUT &&
          mlast.close_type != LAST_REBOOT &&
          mlast.close_type != LAST_SHUTDOWN)
      {
        mlast.close_type = type;
      }
      mlast.close_time = time(0);
      /*write it, and we're done!*/
      j = fwrite(&mlast, sizeof(struct last_entry), 1, fp);
      fclose(fp);
      return;
    }
    /* Not the one we seek, next. */
  }
  fclose(fp);

  /* Not found, no problem, quit. */
  return;
}

void add_llog_entry(struct char_data *ch, int type)
{
  // Gicker - 2022/10/27 - Let's use the previously 'last complete' functionality to see last logins.
  return;

  FILE *fp;
  struct last_entry *llast;
  int i;

  /* so if a char enteres a name, but bad password, otherwise loses link before
   * he gets a pref assinged, we won't record it */
  if (GET_PREF(ch) <= 0)
  {
    return;
  }

  /* See if we have a login stored */
  llast = find_llog_entry(GET_PREF(ch), GET_IDNUM(ch));

  /* we didn't - make a new one */
  if (llast == NULL)
  { /* no entry found, add ..error if close! */
    CREATE(llast, struct last_entry, 1);
    strncpy(llast->username, GET_NAME(ch), 16);
    strncpy(llast->hostname, GET_HOST(ch), 128);
    llast->idnum = GET_IDNUM(ch);
    llast->punique = GET_PREF(ch);
    llast->time = time(0);
    llast->close_time = 0;
    llast->close_type = type;

    if (!(fp = fopen(LAST_FILE, "a")))
    {
      log("error opening last_file for appending");
      free(llast);
      return;
    }
    i = fwrite(llast, sizeof(struct last_entry), 1, fp);
    fclose(fp);
  }
  else
  {
    /* We've found a login - update it */
    mod_llog_entry(llast, type);
  }
  free(llast);
}

void clean_llog_entries(void)
{
  FILE *ofp, *nfp;
  struct last_entry mlast;
  int recs, i, j;

  if (!(ofp = fopen(LAST_FILE, "r")))
    return; /* no file, no gripe */

  fseek(ofp, 0L, SEEK_END);
  recs = ftell(ofp) / sizeof(struct last_entry);
  rewind(ofp);

  if (recs < MAX_LAST_ENTRIES)
  {
    fclose(ofp);
    return;
  }

  if (!(nfp = fopen("etc/nlast", "w")))
  {
    log("Error trying to open new last file.");
    fclose(ofp);
    return;
  }

  /* skip first entries */
  fseek(ofp, (recs - MAX_LAST_ENTRIES) * (sizeof(struct last_entry)), SEEK_CUR);

  /* copy the rest */
  while (!feof(ofp))
  {
    i = fread(&mlast, sizeof(struct last_entry), 1, ofp);
    j = fwrite(&mlast, sizeof(struct last_entry), 1, nfp);
  }
  fclose(ofp);
  fclose(nfp);

  remove(LAST_FILE);
  rename("etc/nlast", LAST_FILE);
}

/* debugging stuff, if you wanna see the whole file */
void list_llog_entries(struct char_data *ch)
{
  FILE *fp;
  struct last_entry llast;
  int i;

  if (!(fp = fopen(LAST_FILE, "r")))
  {
    log("bad things.");
    send_to_char(ch, "Error! - no last log");
  }
  send_to_char(ch, "Last log\r\n");
  i = fread(&llast, sizeof(struct last_entry), 1, fp);

  while (!feof(fp))
  {
    send_to_char(ch, "%10s     %d     %s     %s", llast.username, llast.punique,
                 last_array[llast.close_type], ctime(&llast.time));
    i = fread(&llast, sizeof(struct last_entry), 1, fp);
  }
}

// Gicker - 22/10/27 - Not needed right now as we're not using
// llog system for the last command anymore.
// static struct char_data *is_in_game(long idnum)
// {
//   struct descriptor_data *i;

//   for (i = descriptor_list; i; i = i->next)
//   {
//     if (i->character && GET_IDNUM(i->character) == idnum)
//     {
//       return i->character;
//     }
//   }
//   return NULL;
// }

// will show last 40 logins per character with account name, character name and last login time
void show_full_last_command(struct char_data *ch)
{
  char query[2048];
  MYSQL_RES *res;
  MYSQL_ROW row;

  send_to_char(ch, "%-20s %-20s %s\r\n", "ACCOUNT", "NAME", "LAST ONLINE (SERVER TIME)");
  snprintf(query, sizeof(query), "SELECT a.name, a.last_online, b.name AS account_name FROM player_data a LEFT JOIN account_data b ON a.account_id=b.id ORDER BY a.last_online DESC LIMIT 40;");
  mysql_query(conn, query);
  res = mysql_use_result(conn);
  if (res != NULL)
  {
    while ((row = mysql_fetch_row(res)) != NULL)
    {
      send_to_char(ch, "%-20s %-20s %-15s\r\n", row[2], row[0], row[1]);
    }
  }
  mysql_free_result(res);
}

// will only show latest login per account
void show_full_last_command_unique(struct char_data *ch)
{
  char query[2048];
  MYSQL_RES *res;
  MYSQL_ROW row;

  send_to_char(ch, "%-20s %-20s %s\r\n", "ACCOUNT", "NAME", "LAST ONLINE (SERVER TIME)");
  snprintf(query, sizeof(query), "SELECT a.name, a.last_online, b.name AS account_name FROM player_data a LEFT JOIN account_data b ON a.account_id=b.id GROUP BY b.name ORDER BY a.last_online DESC LIMIT 40;");
  mysql_query(conn, query);
  res = mysql_use_result(conn);
  if (res != NULL)
  {
    while ((row = mysql_fetch_row(res)) != NULL)
    {
      send_to_char(ch, "%-20s %-20s %-15s\r\n", row[2], row[0], row[1]);
    }
  }
  mysql_free_result(res);
}

ACMDU(do_last)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'}, name[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;
  // struct char_data *temp;
  int num = 0;
  // int recs, i;
  // FILE *fp;
  // time_t delta;
  // struct last_entry mlast;

  *name = '\0';

  if (*argument)
  { /* parse it */
    half_chop(argument, arg, argument);
    if (!strcmp(arg, "complete") || !strcmp(arg, "full"))
    {
      show_full_last_command(ch);
      return;
    }
    if (!strcmp(arg, "unique") || !strcmp(arg, "accounts"))
    {
      show_full_last_command_unique(ch);
      return;
    }
    while (*arg)
    {
      if ((*arg == '*') && (GET_LEVEL(ch) == LVL_IMPL))
      {
        list_llog_entries(ch);
        return;
      }
      if (isdigit(*arg))
      {
        num = atoi(arg);
        if (num < 0)
          num = 0;
      }
      else
        strncpy(name, arg, sizeof(name) - 1);
      half_chop(argument, arg, argument);
    }
  }

  if (*name && !num)
  {
    CREATE(vict, struct char_data, 1);
    clear_char(vict);
    CREATE(vict->player_specials, struct player_special_data, 1);
    new_mobile_data(vict);
    /* Allocate mobile event list */
    //    vict->events = create_list();
    if (load_char(name, vict) < 0)
    {
      send_to_char(ch, "There is no such player.\r\n");
      free_char(vict);
      return;
    }

    if ((GET_LEVEL(vict) > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_IMPL))
    {
      send_to_char(ch, "You are not sufficiently godly for that!\r\n");
      return;
    }

    send_to_char(ch, "[%5ld] [%2d %s %s] %-12s : %-18s : %-20s\r\n",
                 GET_IDNUM(vict), (int)GET_LEVEL(vict),
                 CLSLIST_ABBRV(GET_CLASS(vict)), race_list[(int)GET_RACE(vict)].abbrev_color, GET_NAME(vict),
                 GET_HOST(vict) && *GET_HOST(vict) ? GET_HOST(vict) : "(NOHOST)",
                 ctime(&vict->player.time.logon));
    free_char(vict);
    return;
  }

  if (num <= 0 || num >= 100)
  {
    num = 10;
  }

  // Gicker - 2022/10/27 - Using last "complete" for this now
  // As having issues with the llog system

  show_full_last_command(ch);

  send_to_char(ch, "\r\nType 'last unique' to see the most recent unique account logins.\r\n\r\n");

  // if (!(fp = fopen(LAST_FILE, "r")))
  // {
  //   send_to_char(ch, "No entries found.\r\n");
  //   return;
  // }
  // fseek(fp, 0L, SEEK_END);
  // recs = ftell(fp) / sizeof(struct last_entry);

  // send_to_char(ch, "Last log\r\n");
  // while (num > 0 && recs > 0)
  // {
  //   fseek(fp, -1 * (sizeof(struct last_entry)), SEEK_CUR);
  //   i = fread(&mlast, sizeof(struct last_entry), 1, fp);
  //   fseek(fp, -1 * (sizeof(struct last_entry)), SEEK_CUR);
  //   if (!*name || (*name && !str_cmp(name, mlast.username)))
  //   {
  //     send_to_char(ch, "%10.10s %20.20s %16.16s - ",
  //                  mlast.username, mlast.hostname, ctime(&mlast.time));
  //     if ((temp = is_in_game(mlast.idnum)) && mlast.punique == GET_PREF(temp))
  //     {
  //       send_to_char(ch, "Still Playing  ");
  //     }
  //     else
  //     {
  //       send_to_char(ch, "%5.5s ", ctime(&mlast.close_time) + 11);
  //       delta = mlast.close_time - mlast.time;
  //       send_to_char(ch, "(%5.5s) ", asctime(gmtime(&delta)) + 11);
  //       send_to_char(ch, "%s", last_array[mlast.close_type]);
  //     }

  //     send_to_char(ch, "\r\n");
  //     num--;
  //   }
  //   recs--;
  // }
  // fclose(fp);
  // send_to_char(ch, "\r\n"
  //                  "Type last complete to see the last 40 logins with account name, character name and login time.\r\n"
  //              //"Type last unique to see the last 40 logins, only showing the most recent login per account.\r\n"
  // );
}

ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char arg[MAX_INPUT_LENGTH] = {'\0'}, to_force[MAX_INPUT_LENGTH] = {'\0'}, buf1[MAX_INPUT_LENGTH + 32];

  half_chop_c(argument, arg, sizeof(arg), to_force, sizeof(to_force));

  snprintf(buf1, sizeof(buf1), "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char(ch, "Whom do you wish to force do what?\r\n");
  else if ((GET_LEVEL(ch) < LVL_GRSTAFF) || (str_cmp("all", arg) && str_cmp("room", arg)))
  {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_STAFF)
      send_to_char(ch, "You cannot force players.\r\n");
    else if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict))
      send_to_char(ch, "No, no, no!\r\n");
    else
    {
      send_to_char(ch, "%s", CONFIG_OK);
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      mudlog(CMP, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      command_interpreter(vict, to_force);
    }
  }
  else if (!str_cmp("room", arg))
  {
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(NRM, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced room %d to %s",
           GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);

    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_force)
    {
      next_force = vict->next_in_room;
      if (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch))
        continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
  else
  { /* force all */
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(NRM, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced all to %s", GET_NAME(ch), to_force);

    for (i = descriptor_list; i; i = next_desc)
    {
      next_desc = i->next;

      if (STATE(i) != CON_PLAYING || !(vict = i->character) || (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch)))
        continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}

ACMDU(do_wiznet)
{
  char buf1[MAX_INPUT_LENGTH] = {'\0'},
       buf2[MAX_INPUT_LENGTH] = {'\0'},
       buf3[MAX_INPUT_LENGTH] = {'\0'},
       buf4[MAX_INPUT_LENGTH] = {'\0'};
  struct descriptor_data *d = NULL;
  bool emote = FALSE;
  int level = LVL_IMMORT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
  {
    send_to_char(ch, "Usage: wiznet [ #<level> ] [<text> | *<emotetext> | @ ]\r\n");
    return;
  }

  switch (*argument)
  {
  case '*':
    emote = TRUE;

  case '#':
    one_argument(argument + 1, buf1, sizeof(buf1));
    if (is_number(buf1))
    {
      half_chop(argument + 1, buf1, argument);
      level = MAX(atoi(buf1), LVL_IMMORT);
      if (level > GET_LEVEL(ch))
      {
        send_to_char(ch, "You can't wizline above your own level.\r\n");
        return;
      }
    }
    else if (emote)
      argument++;
    break;

  case '@':
    send_to_char(ch, "God channel status:\r\n");
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) != CON_PLAYING || GET_LEVEL(d->character) < LVL_IMMORT)
        continue;
      if (!CAN_SEE(ch, d->character))
        continue;

      send_to_char(ch, "  %-*s%s%s%s\r\n", MAX_NAME_LENGTH, GET_NAME(d->character),
                   PLR_FLAGGED(d->character, PLR_WRITING) ? " (Writing)" : "",
                   PLR_FLAGGED(d->character, PLR_MAILING) ? " (Writing mail)" : "",
                   PRF_FLAGGED(d->character, PRF_NOWIZ) ? " (Offline)" : "");
    }
    return;

  case '\\':
    ++argument;
    break;

  default:
    break;
  }

  if (PRF_FLAGGED(ch, PRF_NOWIZ))
  {
    send_to_char(ch, "You are offline!\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Don't bother the gods like that!\r\n");
    return;
  }

  if (level > LVL_IMMORT)
  {
    snprintf(buf1, sizeof(buf1), "\tc[wiznet] %s: <%d> %s%s\tn",
             GET_NAME(ch), level, emote ? "<--- " : "", argument);
    snprintf(buf3, sizeof(buf1), "\tc[wiznet] %s: <%d> %s%s\tn\r\n",
             GET_NAME(ch), level, emote ? "<--- " : "", argument);

    snprintf(buf2, sizeof(buf1), "\tc[wiznet] Someone: <%d> %s%s\tn",
             level, emote ? "<--- " : "", argument);
    snprintf(buf4, sizeof(buf1), "\tc[wiznet] Someone: <%d> %s%s\tn\r\n",
             level, emote ? "<--- " : "", argument);
  }
  else
  {
    snprintf(buf1, sizeof(buf1), "\tc[wiznet] %s: %s%s\tn",
             GET_NAME(ch), emote ? "<--- " : "", argument);
    snprintf(buf3, sizeof(buf1), "\tc[wiznet] %s: %s%s\tn\r\n",
             GET_NAME(ch), emote ? "<--- " : "", argument);

    snprintf(buf2, sizeof(buf1), "\tc[wiznet] Someone: %s%s\tn",
             emote ? "<--- " : "", argument);
    snprintf(buf4, sizeof(buf1), "\tc[wiznet] Someone: %s%s\tn\r\n",
             emote ? "<--- " : "", argument);
  }

  for (d = descriptor_list; d; d = d->next)
  {
    if (d && ch)
    {
      if (d->character && ch->desc)
      {
        if (IS_PLAYING(d) && (GET_LEVEL(d->character) >= level) &&
            (!PRF_FLAGGED(d->character, PRF_NOWIZ)) && (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT))))
        {
          if (CAN_SEE(d->character, ch))
          {
            act(buf1, FALSE, d->character, 0, 0, TO_CHAR | DG_NO_TRIG);
            add_history(d->character, buf3, HIST_WIZNET);
          }
          else
          {
            act(buf2, FALSE, d->character, 0, 0, TO_CHAR | DG_NO_TRIG);
            add_history(d->character, buf4, HIST_WIZNET);
          }
        }
      }
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
}

ACMD(do_zreset)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  zone_rnum i;
  zone_vnum j;

  one_argument(argument, arg, sizeof(arg));

  if (*arg == '*')
  {
    if (GET_LEVEL(ch) < LVL_STAFF)
    {
      send_to_char(ch, "You do not have permission to reset the entire world.\r\n");
      return;
    }
    else
    {
      for (i = 0; i <= top_of_zone_table; i++)
        reset_zone(i);
      send_to_char(ch, "Reset world.\r\n");
      mudlog(NRM, MAX(LVL_GRSTAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s reset entire world.", GET_NAME(ch));
      return;
    }
  }
  else if (*arg == '.' || !*arg)
    i = world[IN_ROOM(ch)].zone;
  else
  {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
        break;
  }
  if (i <= top_of_zone_table && (can_edit_zone(ch, i) || GET_LEVEL(ch) > LVL_IMMORT))
  {
    reset_zone(i);
    send_to_char(ch, "Reset zone #%d: %s.\r\n", zone_table[i].number,
                 zone_table[i].name);
    mudlog(NRM, MAX(LVL_GRSTAFF, GET_INVIS_LEV(ch)), TRUE,
           "(GC) %s reset zone %d (%s)", GET_NAME(ch),
           zone_table[i].number, zone_table[i].name);
  }
  else
    send_to_char(ch, "You do not have permission to reset this zone. Try %d.\r\n",
                 GET_OLC_ZONE(ch));
}

/*  General fn for wizcommands of the sort: cmd <player> */
ACMD(do_wizutil)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;
  int taeller;
  long result;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
    send_to_char(ch, "Yes, but for whom?!?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "There is no such player.\r\n");
  else if (IS_NPC(vict))
    send_to_char(ch, "You can't do that to a mob!\r\n");
  else if (GET_LEVEL(vict) >= GET_LEVEL(ch) && vict != ch)
    send_to_char(ch, "Hmmm...you'd better not.\r\n");
  else
  {
    switch (subcmd)
    {
    case SCMD_REROLL:
      send_to_char(ch, "Rerolled...[not currently implemented]\r\n");
      roll_real_abils(vict);
      log("(GC) %s has rerolled %s.\tn", GET_NAME(ch), GET_NAME(vict));
      send_to_char(ch, "New stats: Str %d/%d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
                   GET_STR(vict), GET_ADD(vict), GET_INT(vict), GET_WIS(vict),
                   GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
      break;
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(vict, PLR_KILLER))
      {
        send_to_char(ch, "Your victim is not flagged.\r\n");
        return;
      }
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_THIEF);
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_KILLER);
      send_to_char(ch, "Pardoned.\r\n");
      send_to_char(vict, "You have been pardoned by the Gods!\r\n");
      mudlog(BRF, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      mudlog(NRM, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) Notitle %s for %s by %s.",
             ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Notitle %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_MUTE:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      mudlog(BRF, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) Mute %s for %s by %s.",
             ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Mute %s for %s by %s.\tn\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_FREEZE:
      if (ch == vict)
      {
        send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
        return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN))
      {
        send_to_char(ch, "Your victim is already pretty cold.\r\n");
        return;
      }
      SET_BIT_AR(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      send_to_char(vict, "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
      send_to_char(ch, "Frozen.\r\n");
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      mudlog(BRF, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN))
      {
        send_to_char(ch, "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
        return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch))
      {
        send_to_char(ch, "\tnSorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
                     GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
        return;
      }
      mudlog(BRF, MAX(LVL_STAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char(vict, "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
      send_to_char(ch, "Thawed.\r\n");
      act("\tnA sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      // clear event cooldowns and other timed effects built with the event system
      clear_char_event_list(vict);
      // reset their mission ready status
      GET_MISSION_COOLDOWN(vict) = 0;
      // Clear Misc Cooldowns
      clear_misc_cooldowns(vict);
      // clear affects
      if (vict->affected || AFF_FLAGS(vict))
      {
        while (vict->affected)
          affect_remove(vict, vict->affected);
        for (taeller = 0; taeller < AF_ARRAY_MAX; taeller++)
          AFF_FLAGS(vict)
        [taeller] = 0;
        send_to_char(vict, "There is a brief flash of light!\r\nYou feel slightly different.\r\n");
        send_to_char(ch, "All spells removed.\r\n");
      }
      else
      {
        send_to_char(ch, "Your victim does not have any affections!\r\n");
        return;
      }
      break;
    default:
      log("\tnSYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      /*  SYSERR_DESC: This is the same as the unhandled case in do_gen_ps(),
       *  but this function handles 'reroll', 'pardon', 'freeze', etc. */
      break;
    }
    save_char(vict, 0);
  }
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 FIXME: overflow possible
   listall = list all the zones in game or not */
static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall)
{
  size_t tmp;
  double avglvl = 0;
  int mcount = 0;
  mob_rnum mrnum;

  /* calculate average mob-level of zone, original by Ornir */
  for (mrnum = 0; mrnum <= top_of_mobt; mrnum++)
  {
    if (mob_index[mrnum].vnum >= zone_table[zone].bot && mob_index[mrnum].vnum <= zone_table[zone].top)
    {
      avglvl += mob_proto[mrnum].player.level;
      mcount++;
    }
  }
  avglvl = avglvl / (double)mcount;

  /* if you send listall */
  if (listall)
  {
    int i, j, k, l, m, n, o;
    char buf[MAX_STRING_LENGTH] = {'\0'};

    sprintbitarray(zone_table[zone].zone_flags, zone_bits, ZN_ARRAY_MAX, buf);

    tmp = snprintf(bufptr, left,
                   "%3d %-30.30s%s By: %-10.10s%s Age: %3d; Reset: %3d (%s);Show Weather %d; Range: %5d-%5d\r\n",
                   zone_table[zone].number, zone_table[zone].name, KNRM, zone_table[zone].builders, KNRM,
                   zone_table[zone].age, zone_table[zone].lifespan,
                   zone_table[zone].reset_mode ? ((zone_table[zone].reset_mode == 1) ? "Reset when no players are in zone" : "Normal reset") : "Never reset",
                   zone_table[zone].show_weather,
                   zone_table[zone].bot, zone_table[zone].top);
    i = j = k = l = m = n = o = 0;

    for (i = 0; i < top_of_world; i++)
      if (world[i].number >= zone_table[zone].bot && world[i].number <= zone_table[zone].top)
        j++;

    for (i = 0; i < top_of_objt; i++)
      if (obj_index[i].vnum >= zone_table[zone].bot && obj_index[i].vnum <= zone_table[zone].top)
        k++;

    for (i = 0; i < top_of_mobt; i++)
      if (mob_index[i].vnum >= zone_table[zone].bot && mob_index[i].vnum <= zone_table[zone].top)
        l++;

    for (i = 0; i <= top_shop; i++)
      if (SHOP_NUM(i) >= zone_table[zone].bot && SHOP_NUM(i) <= zone_table[zone].top)
        m++;

    for (i = 0; i < top_of_trigt; i++)
      if (trig_index[i]->vnum >= zone_table[zone].bot && trig_index[i]->vnum <= zone_table[zone].top)
        n++;

    o = count_quests(zone_table[zone].bot, zone_table[zone].top);

    tmp += snprintf(bufptr + tmp, left - tmp,
                    "       Zone stats:\r\n"
                    "       ---------------\r\n"
                    "         Flags:       %s\r\n"
                    "         RealNum:     %2d\r\n"
                    "         Min Lev:     %2d\r\n"
                    "         Max Lev:     %2d\r\n"
                    "         Rooms:       %2d\r\n"
                    "         Objects:     %2d\r\n"
                    "         Mobiles:     %2d\r\n"
                    "         Shops:       %2d\r\n"
                    "         Triggers:    %2d\r\n"
                    "         Quests:      %2d\r\n"
                    "         Avg MOB Lvl:  %2.3f\r\n",
                    buf, zone, zone_table[zone].min_level, zone_table[zone].max_level,
                    j, k, l, m, n, o, avglvl);

    return tmp;
  }

  return snprintf(bufptr, left,
                  "%3d %-*s%s By: %-10.10s%s Range: %5d-%5d, AvgLvl: %2.3f\r\n", zone_table[zone].number,
                  count_color_chars(zone_table[zone].name) + 30, zone_table[zone].name, KNRM,
                  zone_table[zone].builders, KNRM, zone_table[zone].bot, zone_table[zone].top, avglvl);
}

ACMD(do_show)
{
  int i = 0, j = 0, k = 0, l = 0, con = 0, builder = 0;
  size_t len = 0, nlen = 0;
  zone_rnum zrn = NOWHERE;
  zone_vnum zvn = NOWHERE;
  byte self = FALSE;
  struct char_data *vict = NULL;
  struct obj_data *obj = NULL;
  struct descriptor_data *d = NULL;
  char field[MAX_INPUT_LENGTH] = {'\0'}, value[MAX_INPUT_LENGTH] = {'\0'},
       arg[MAX_INPUT_LENGTH] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'};
  int r = 0, g = 0, b = 0;
  char colour[16] = {'\0'};
  int q_total = 0, q_approved = 0;
  struct quest_entry *quest = NULL;

  struct show_struct
  {
    const char *cmd;
    const char level;
  } fields[] = {
      {"nothing", 0},        /* 0 */
      {"zones", LVL_IMMORT}, /* 1 */
      {"player", LVL_IMMORT},
      {"rent", LVL_IMMORT},
      {"stats", LVL_IMMORT},
      {"errors", LVL_IMMORT}, /* 5 */
      {"death", LVL_IMMORT},
      {"godrooms", LVL_IMMORT},
      {"shops", LVL_IMMORT},
      {"houses", LVL_IMMORT},
      {"snoop", LVL_IMMORT}, /* 10 */
      {"claims", LVL_IMMORT},
      {"popularity", LVL_IMMORT},
      {"bab", LVL_IMMORT},
      {"exp", LVL_IMMORT},
      {"colour", LVL_IMMORT}, // 15
      {"citizen", LVL_IMMORT},
      {"guard", LVL_IMMORT},
      {"crafts", LVL_IMMORT},
      {"todo", LVL_IMMORT},
      {"\n", 0}};

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Show options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
        send_to_char(ch, "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    send_to_char(ch, "\r\n");
    return;
  }

  strlcpy(arg, two_arguments(argument, field, sizeof(field), value, sizeof(value)), sizeof(arg)); /* strcpy: OK (argument <= MAX_INPUT_LENGTH == arg) */

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_LEVEL(ch) < fields[l].level)
  {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return;
  }
  if (!strcmp(value, "."))
    self = TRUE;
  buf[0] = '\0';

  switch (l)
  {
    /* show zone */
  case 1:
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, sizeof(buf), world[IN_ROOM(ch)].zone, 1);
    else if (*value && is_number(value))
    {
      for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++)
        ;
      if (zrn <= top_of_zone_table)
        print_zone_to_buf(buf, sizeof(buf), zrn, 1);
      else
      {
        send_to_char(ch, "That is not a valid zone.\r\n");
        return;
      }
    }
    else
    {
      char *buf2;
      if (*value)
        builder = 1;
      for (len = zrn = 0; zrn <= top_of_zone_table; zrn++)
      {
        if (*value)
        {
          buf2 = strtok(strdup(zone_table[zrn].builders), " ");
          while (buf2)
          {
            if (!str_cmp(buf2, value))
            {
              if (builder == 1)
                builder++;
              break;
            }
            buf2 = strtok(NULL, " ");
          }
          if (!buf2)
            continue;
        }
        nlen = print_zone_to_buf(buf + len, sizeof(buf) - len, zrn, 0);
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    }
    if (builder == 1)
      send_to_char(ch, "%s has not built any zones here.\r\n", CAP(value));
    else if (builder == 2)
      send_to_char(ch, "The following zones have been built by: %s\r\n", CAP(value));
    page_string(ch->desc, buf, TRUE);
    break;

    /* show player */
  case 2:
    if (!*value)
    {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }

    CREATE(vict, struct char_data, 1);
    clear_char(vict);
    CREATE(vict->player_specials, struct player_special_data, 1);
    new_mobile_data(vict);
    /* Allocate mobile event list */
    // vict->events = create_list();
    if (load_char(value, vict) < 0)
    {
      send_to_char(ch, "There is no such player.\r\n");
      free_char(vict);
      return;
    }
    send_to_char(ch, "Player: %-12s (%s) [%2d %s %s]\r\n", GET_NAME(vict),
                 genders[(int)GET_SEX(vict)], GET_LEVEL(vict), CLSLIST_ABBRV(GET_CLASS(vict)), race_list[(int)GET_RACE(vict)].abbrev_color);
    send_to_char(ch, "Au: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Lessons: %-3d\r\n",
                 GET_GOLD(vict), GET_BANK_GOLD(vict), GET_EXP(vict),
                 GET_ALIGNMENT(vict), GET_PRACTICES(vict));

    /* ctime() uses static buffer: do not combine. */
    send_to_char(ch, "Started: %-20.16s  ", ctime(&vict->player.time.birth));
    send_to_char(ch, "Last: %-20.16s  Played: %3dh %2dm\r\n",
                 ctime(&vict->player.time.logon),
                 (int)(vict->player.time.played / 3600),
                 (int)(vict->player.time.played / 60 % 60));
    free_char(vict);
    break;

    /* show rent */
  case 3:
    if (!*value)
    {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }
    Crash_listrent(ch, value);
    break;

    /* show stats */
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next)
    {
      if (IS_NPC(vict))
        j++; // mobile in game count
      else if (CAN_SEE(ch, vict))
      {
        i++; // player in game count
        if (vict->desc)
          con++; // how many connected
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++; // number of objects in game
    for (l = 0; l < top_of_mobt; l++)
    {
      if (mob_proto[l].mob_specials.quest)
      {
        for (quest = mob_proto[l].mob_specials.quest; quest; quest = quest->next)
        {
          q_total++; // total homeland quests
          if (quest->approved)
            q_approved++; // homeland quests approved
        }
      }
    }
    send_to_char(ch,
                 "Current stats:\r\n"
                 "  %5d players in game  %5d connected\r\n"
                 "  %5d registered\r\n"
                 "  %5d mobiles          %5d prototypes\r\n"
                 "  %5d objects          %5d prototypes\r\n"
                 "  %5d rooms            %5d zones\r\n"
                 "  %5d triggers         %5d shops\r\n"
                 "  %5d large bufs       %5d autoquests\r\n"
                 "  %5d hlquests app     %5d total hl quests\r\n"
                 "  %5d buf switches     %5d overflows\r\n"
                 "  %5d lists\r\n",
                 i, con,
                 top_of_p_table + 1,
                 j, top_of_mobt + 1,
                 k, top_of_objt + 1,
                 top_of_world + 1, top_of_zone_table + 1,
                 top_of_trigt + 1, top_shop + 1,
                 buf_largecount, total_quests,
                 q_approved, q_total,
                 buf_switches, buf_overflows, global_lists->iSize);
    break;

    /* show errors */
  case 5:
    len = strlcpy(buf, "Errant Rooms\r\n------------\r\n", sizeof(buf));
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < DIR_COUNT; j++)
      {
        if (!W_EXIT(i, j))
          continue;
        if (W_EXIT(i, j)->to_room == 0)
        {
          nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (void   ) [%5d] %-*s%s (%s)\r\n", ++k, GET_ROOM_VNUM(i), count_color_chars(world[i].name) + 40, world[i].name, QNRM, dirs[j]);
          if (len + nlen >= sizeof(buf))
            break;
          len += nlen;
        }
        if (W_EXIT(i, j)->to_room == NOWHERE && !W_EXIT(i, j)->general_description)
        {
          nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (Nowhere) [%5d] %-*s%s (%s)\r\n", ++k, GET_ROOM_VNUM(i), count_color_chars(world[i].name) + 40, world[i].name, QNRM, dirs[j]);
          if (len + nlen >= sizeof(buf))
            break;
          len += nlen;
        }
      }
    page_string(ch->desc, buf, TRUE);
    break;

    /* show death */
  case 6:
    len = strlcpy(buf, "Death Traps\r\n-----------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
      {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s%s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name, QNRM);
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

    /* show godrooms */
  case 7:
    len = strlcpy(buf, "Godrooms\r\n--------------------------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_STAFFROOM))
      {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s%s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name, QNRM);
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

    /* show shops */
  case 8:
    show_shops(ch, value);
    break;

    /* show houses */
  case 9:
    hcontrol_list_houses(ch, value);
    break;

    /* show snoop */
  case 10:
    i = 0;
    send_to_char(ch, "People currently snooping:\r\n--------------------------\r\n");
    for (d = descriptor_list; d; d = d->next)
    {
      if (d->snooping == NULL || d->character == NULL)
        continue;
      if (STATE(d) != CON_PLAYING || GET_LEVEL(ch) < GET_LEVEL(d->character))
        continue;
      if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
        continue;
      i++;
      send_to_char(ch, "%-10s%s - snooped by %s%s.\r\n", GET_NAME(d->snooping->character), QNRM, GET_NAME(d->character), QNRM);
    }
    if (i == 0)
      send_to_char(ch, "No one is currently snooping.\r\n");
    break;
    /* show claims */
  case 11:
    show_claims(ch, value);
    break;
    /* show popularity */
  case 12:
    show_popularity(ch, value);
    break;

    /* show bab */
  case 13:
    break;

    /* show experience tables */
  case 14:
    send_to_char(ch, "This isn't relevant in the current code-base.\r\n");
    break;

  case 15:
    len = strlcpy(buf, "Colours\r\n--------------------------\r\n", sizeof(buf));
    k = 0;
    for (r = 0; r < 6; r++)
      for (g = 0; g < 6; g++)
        for (b = 0; b < 6; b++)
        {
          snprintf(colour, sizeof(colour), "F%d%d%d", r, g, b);
          nlen = snprintf(buf + len, sizeof(buf) - len, "%s%s%s", ColourRGB(ch->desc, colour), colour, ++k % 6 == 0 ? "\tn\r\n" : "    ");
          if (len + nlen >= sizeof(buf))
            break;
          len += nlen;
        }
    page_string(ch->desc, buf, TRUE);
    break;

  case 16: // show citizen
    if (*value && is_number(value))
      j = atoi(value);
    else
      j = zone_table[world[ch->in_room].zone].number;
    j *= 100;
    if (real_zone(j) <= 0)
    {
      snprintf(buf, sizeof(buf), "\tR%d \tris not in a defined zone.\tn\r\n", j);
      send_to_char(ch, buf);
      return;
    }
    k = real_zone(j);
    k = zone_table[k].top;
    snprintf(buf, sizeof(buf), "Citizens in this zone : From %d to %d\r\n", j, k);
    for (i = j; i <= k; i++)
    {
      if ((l = real_mobile(i)) >= 0)
      {
        if (MOB_FLAGGED(&mob_proto[l], MOB_CITIZEN))
        {
          char res_buf[128];
          snprintf(res_buf, sizeof(res_buf), "[%5d] %-40s\r\n", i, mob_proto[l].player.short_descr);
          strlcat(buf, res_buf, sizeof(buf));
        }
      }
    }
    page_string(ch->desc, buf, 1);
    break;

  case 17: // show guard
    if (*value && is_number(value))
      j = atoi(value);
    else
      j = zone_table[world[ch->in_room].zone].number;
    j *= 100;
    if (real_zone(j) <= 0)
    {
      snprintf(buf, sizeof(buf), "\tR%d \tris not in a defined zone.\tn\r\n", j);
      send_to_char(ch, buf);
      return;
    }
    k = real_zone(j);
    k = zone_table[k].top;
    snprintf(buf, sizeof(buf), "Guard in this zone : From %d to %d\r\n", j, k);
    for (i = j; i <= k; i++)
    {
      if ((l = real_mobile(i)) >= 0)
      {
        if (MOB_FLAGGED(&mob_proto[l], MOB_GUARD))
        {
          char res_buf[128];
          snprintf(res_buf, sizeof(res_buf), "[%5d] %-40s\r\n", i, mob_proto[l].player.short_descr);
          strlcat(buf, res_buf, sizeof(buf));
        }
      }
    }
    page_string(ch->desc, buf, 1);
    break;

    /* NewCraft */
  case 18: // show craft
    if (!*value)
      list_all_crafts(ch);
    else
      show_craft(ch, get_craft_from_arg(value), 0);
    break;

    /* show todo */
  case 19:
    if (!*value)
    {
      send_to_char(ch, "Usage: show todo <player>\r\n");
      return;
    }

    vict = new_char();

    if (load_char(value, vict) < 0)
    {
      send_to_char(ch, "No such player exists.\r\n");
      free_char(vict);
      return;
    }

    if (GET_TODO(vict) && GET_TODO(vict)->text && *GET_TODO(vict)->text)
      display_todo(ch, vict);
    else
      send_to_char(ch, "%s has nothing to do!\r\n", GET_NAME(vict));

    free_char(vict);

    break;

    /* show what? */
  default:
    send_to_char(ch, "Sorry, I don't understand that.\r\n");
    break;
  }
}

/* The do_set function */

#define PC 1
#define NPC 2
#define BOTH 3

#define MISC 0
#define BINARY 1
#define NUMBER 2
#define ADDER 3

#define SET_OR_REMOVE(flagset, flags) \
  {                                   \
    if (on)                           \
      SET_BIT_AR(flagset, flags);     \
    else if (off)                     \
      REMOVE_BIT_AR(flagset, flags);  \
  }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

/* The set options available */
const struct set_struct
{
  const char *cmd;
  const char level;
  const char pcnpc;
  const char type;
  const char *desc;
} set_fields[] = {
    {"ac", LVL_BUILDER, BOTH, NUMBER}, /* 0  */
    {"afk", LVL_BUILDER, PC, BINARY},  /* 1  */
    {"age", LVL_STAFF, BOTH, NUMBER},
    {"align", LVL_BUILDER, BOTH, NUMBER},
    {"bank", LVL_BUILDER, PC, NUMBER},
    {"brief", LVL_STAFF, PC, BINARY}, /* 5  */
    {"cha", LVL_IMPL, BOTH, NUMBER},
    {"clan", LVL_STAFF, PC, NUMBER},
    {"clanrank", LVL_STAFF, PC, NUMBER},
    {"class", LVL_IMPL, BOTH, MISC},
    {"color", LVL_STAFF, PC, BINARY},
    {"con", LVL_IMPL, BOTH, NUMBER},
    {"damroll", LVL_IMPL, BOTH, NUMBER}, /* 12 */
    {"deleted", LVL_IMPL, PC, BINARY},
    {"dex", LVL_BUILDER, BOTH, NUMBER},
    {"drunk", LVL_BUILDER, BOTH, MISC},
    {"exp", LVL_IMPL, BOTH, NUMBER},
    {"frozen", LVL_GRSTAFF, PC, BINARY}, /* 17 */
    {"gold", LVL_IMPL, BOTH, NUMBER},
    {"height", LVL_BUILDER, BOTH, NUMBER},
    {"hitpoints", LVL_IMPL, BOTH, NUMBER},
    {"hitroll", LVL_IMPL, BOTH, NUMBER},
    {"hunger", LVL_BUILDER, BOTH, MISC}, /* 22 */
    {"int", LVL_BUILDER, BOTH, NUMBER},
    {"invis", LVL_STAFF, PC, NUMBER},
    {"invstart", LVL_BUILDER, PC, BINARY},
    {"killer", LVL_STAFF, PC, BINARY},
    {"level", LVL_GRSTAFF, BOTH, NUMBER}, /* 27 */
    {"loadroom", LVL_BUILDER, PC, MISC},
    {"psp", LVL_IMPL, BOTH, NUMBER},
    {"maxhit", LVL_IMPL, BOTH, NUMBER},
    {"maxpsp", LVL_IMPL, BOTH, NUMBER},
    {"maxmove", LVL_IMPL, BOTH, NUMBER}, /* 32 */
    {"move", LVL_IMPL, BOTH, NUMBER},
    {"name", LVL_IMMORT, PC, MISC},
    {"nodelete", LVL_STAFF, PC, BINARY},
    {"nohassle", LVL_STAFF, PC, BINARY},
    {"nosummon", LVL_BUILDER, PC, BINARY}, /* 37 */
    {"nowizlist", LVL_GRSTAFF, PC, BINARY},
    {"olc", LVL_GRSTAFF, PC, MISC},
    {"password", LVL_GRSTAFF, PC, MISC},
    {"poofin", LVL_IMMORT, PC, MISC},
    {"poofout", LVL_IMMORT, PC, MISC}, /* 42 */
    {"practices", LVL_IMPL, PC, NUMBER},
    {"quest", LVL_STAFF, PC, BINARY},
    {"room", LVL_BUILDER, BOTH, NUMBER},
    {"screenwidth", LVL_STAFF, PC, NUMBER},
    {"sex", LVL_STAFF, BOTH, MISC}, /* 47 */
    {"showvnums", LVL_BUILDER, PC, BINARY},
    {"siteok", LVL_STAFF, PC, BINARY},
    {"str", LVL_IMPL, BOTH, NUMBER},
    {"stradd", LVL_IMPL, BOTH, NUMBER},
    {"thief", LVL_STAFF, PC, BINARY}, /* 52 */
    {"thirst", LVL_BUILDER, BOTH, MISC},
    {"title", LVL_STAFF, PC, MISC},
    {"variable", LVL_GRSTAFF, PC, MISC},
    {"weight", LVL_BUILDER, BOTH, NUMBER},
    {"wis", LVL_IMPL, BOTH, NUMBER}, /* 57 */
    {"questpoints", LVL_IMPL, PC, NUMBER},
    {"questhistory", LVL_STAFF, PC, NUMBER},
    {"trains", LVL_IMPL, PC, NUMBER}, /* 60 */
    {"race", LVL_IMPL, BOTH, MISC},
    {"spellres", LVL_IMPL, PC, NUMBER},         /* 62 */
    {"size", LVL_IMPL, PC, NUMBER},             /* 63 */
    {"wizard", LVL_IMPL, PC, NUMBER},           /* 64 */
    {"cleric", LVL_IMPL, PC, NUMBER},           /* 65 */
    {"rogue", LVL_IMPL, PC, NUMBER},            /* 66 */
    {"warrior", LVL_IMPL, PC, NUMBER},          /* 67 */
    {"monk", LVL_IMPL, PC, NUMBER},             /* 68 */
    {"druid", LVL_IMPL, PC, NUMBER},            /* 69 */
    {"boost", LVL_IMPL, PC, NUMBER},            /* 70 */
    {"berserker", LVL_IMPL, PC, NUMBER},        /* 71 */
    {"sorcerer", LVL_IMPL, PC, NUMBER},         /* 72 */
    {"paladin", LVL_IMPL, PC, NUMBER},          /* 73 */
    {"ranger", LVL_IMPL, PC, NUMBER},           /* 74 */
    {"bard", LVL_IMPL, PC, NUMBER},             /* 75 */
    {"featpoints", LVL_IMPL, PC, NUMBER},       /* 76 */
    {"epicfeatpoints", LVL_IMPL, PC, NUMBER},   /* 77 */
    {"classfeats", LVL_IMPL, PC, MISC},         /* 78 */
    {"epicclassfeats", LVL_IMPL, PC, MISC},     /* 79 */
    {"accexp", LVL_IMPL, PC, NUMBER},           /* 80 */
    {"weaponmaster", LVL_IMPL, PC, NUMBER},     /* 81 */
    {"arcanearcher", LVL_IMPL, PC, NUMBER},     /* 82 */
    {"stalwartdefender", LVL_IMPL, PC, NUMBER}, /* 83 */
    {"shifter", LVL_IMPL, PC, NUMBER},          /* 84 */
    {"duelist", LVL_IMPL, PC, NUMBER},          /* 85 */
    {"guimode", LVL_BUILDER, PC, BINARY},       /* 86 */
    {"rpmode", LVL_BUILDER, PC, BINARY},        /* 87 */
    {"mystictheurge", LVL_IMPL, PC, NUMBER},    /* 88 */
    {"addaccexp", LVL_IMPL, PC, ADDER},         /* 89 */
    {"alchemist", LVL_IMPL, PC, NUMBER},        /* 90 */
    {"arcaneshadow", LVL_IMPL, PC, NUMBER},     /* 91 */
    {"sacredfist", LVL_IMPL, PC, NUMBER},       /* 92 */
    {"premadebuild", LVL_STAFF, PC, MISC},      /* 93 */
    {"psionicist", LVL_IMPL, PC, NUMBER},       /* 94 */
    {"deity", LVL_BUILDER, PC, MISC},           /* 95 */
    {"eldritchknight", LVL_IMPL, PC, NUMBER},   /* 96 */
    {"spellsword", LVL_IMPL, PC, NUMBER},       /* 97 */
    {"shadowdancer", LVL_IMPL, PC, NUMBER},     /* 98 */
    {"blackguard", LVL_IMPL, PC, NUMBER},       /* 99 */
    {"assassin", LVL_IMPL, PC, NUMBER},         /* 100 */
    {"inquisitor", LVL_IMPL, PC, NUMBER},       /* 101 */

    {"\n", 0, BOTH, MISC},
};

/*  adding this to remind me to add new classes to the perform_set list
 * CLASS_WIZARD
 * CLASS_CLERIC
 * CLASS_ROGUE
 * CLASS_WARRIOR
 * CLASS_MONK
 * CLASS_DRUID
 * CLASS_BERSERKER
 * CLASS_SORCERER
 * CLASS_BARD
 * CLASS_PALADIN
 * CLASS_RANGER
 * CLASS_WEAPON_MASTER
 * CLASS_ARCANE_ARCHER
 * CLASS_ARCANE_SHADOW
 * CLASS_SACRED_FIST
 * CLASS_STALWART_DEFENDER
 * CLASS_SHIFTER
 * CLASS_DUELIST
 * CLASS_MYSTIC_THEURGE
 * CLASS_ALCHEMIST
 * CLASS_ARCANE_SHADOW
 * CLASS_SACRED_FIST
 * CLASS_ELDRITCH_KNIGHT
 * CLASS_PSIONICIST
 * CLASS_SPELLSWORD
 * CLASS_SHADOWDANCER CLASS_SHADOW_DANCER
 * CLASS_BLACKGUARD
 * CLASS_ASSASSIN
 * CLASS_INQUISITOR
 */

static int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg)
{
  int i, on = 0, off = 0, value = 0, qvnum;
  room_rnum rnum;
  room_vnum rvnum;
  char arg1[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};
  int class = CLASS_UNDEFINED;

  /* Check to make sure all the levels are correct */
  if (GET_LEVEL(ch) != LVL_IMPL)
  {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch)
    {
      send_to_char(ch, "Maybe that's not such a great idea...\r\n");
      return (0);
    }
  }
  if (GET_LEVEL(ch) < set_fields[mode].level)
  {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return (0);
  }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC))
  {
    send_to_char(ch, "You can't do that to a beast!\r\n");
    return (0);
  }
  else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC))
  {
    send_to_char(ch, "That can only be done to a beast!\r\n");
    return (0);
  }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY)
  {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off))
    {
      send_to_char(ch, "Value must be 'on' or 'off'.\r\n");
      return (0);
    }
  }
  else if (set_fields[mode].type == NUMBER || set_fields[mode].type == ADDER)
  {
    value = atoi(val_arg);
  }
  switch (mode)
  {
  case 0:                              /* ac */
    GET_REAL_AC(vict) = RANGE(0, 300); /* 0-30AC */
    affect_total(vict);
    break;
  case 1: /* afk */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_AFK);
    break;
  case 2: /* age */
    if (value < 2 || value > 200)
    { /* Arbitrary limits. */
      send_to_char(ch, "Ages 2 to 200 accepted.\r\n");
      return (0);
    }
    /* NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98 */
    vict->player.time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
    break;
  case 3: /* align */
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
    affect_total(vict);
    break;
  case 4: /* bank */
    GET_BANK_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 5: /* brief */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 6: /* cha */
    RANGE(3, 50);
    GET_REAL_CHA(vict) = value;
    affect_total(vict);
    break;
  case 7: /* clan */
    if (value == 0 || !str_cmp(val_arg, "off"))
    {
      GET_CLAN(vict) = 0;
      GET_CLANRANK(vict) = 0;

      /* Set value in pindex */
      for (i = 0; i < top_of_p_table; i++)
        if (player_table[i].id == GET_IDNUM(vict))
          player_table[i].clan = 0;
      save_player_index();
      send_to_char(ch, "%s is now in no clan.\r\n", GET_NAME(vict));
      break;
    }
    else if ((i = real_clan(value)) == NO_CLAN)
    {
      send_to_char(ch, "Invalid clan VNUM!\r\nSee clan info for a list.\r\n");
      return (0);
    }
    set_clan(vict, value);
    send_to_char(ch, "%s is now in the '%s%s' clan.\r\n", GET_NAME(vict), clan_list[i].clan_name, CCNRM(ch, C_NRM));
    break;
  case 8: /* clanrank */
    if ((i = real_clan(GET_CLAN(vict))) == NO_CLAN)
    {
      send_to_char(ch, "%s isn't in a clan, so can't have a rank!\r\n", GET_NAME(vict));
      return (0);
    }
    GET_CLANRANK(vict) = RANGE(0, clan_list[i].ranks);
    break;
  case 9: /* class */
    if ((i = parse_class_long(val_arg)) == CLASS_UNDEFINED)
    {
      send_to_char(ch, "That is not a class.\r\n");
      return (0);
    }
    GET_CLASS(vict) = i;
    break;
  case 10: /* color */
    SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1));
    SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_2));
    break;
  case 11: /* con */
    RANGE(3, 50);
    GET_REAL_CON(vict) = value;
    affect_total(vict);
    break;
  case 12: /* damroll */
    GET_REAL_DAMROLL(vict) = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 13: /* delete */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
    break;
  case 14: /* dex */
    RANGE(3, 50);
    GET_REAL_DEX(vict) = value;
    affect_total(vict);
    break;
  case 15: /* drunk */
    if (!str_cmp(val_arg, "off"))
    {
      GET_COND(vict, DRUNK) = -1;
      send_to_char(ch, "%s's drunkenness is now off.\r\n", GET_NAME(vict));
    }
    else if (is_number(val_arg))
    {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, DRUNK) = value;
      send_to_char(ch, "%s's drunkenness set to %d.\r\n", GET_NAME(vict), value);
    }
    else
    {
      send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
      return (0);
    }
    break;
  case 16: /* exp */
    vict->points.exp = value;
    break;
  case 17: /* frozen */
    if (ch == vict && on)
    {
      send_to_char(ch, "Better not -- could be a long winter!\r\n");
      return (0);
    }
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
    break;
  case 18: /* gold */
    GET_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 19: /* height */
    GET_HEIGHT(vict) = value;
    affect_total(vict);
    break;
  case 20: /* hit */
    GET_HIT(vict) = RANGE(-9, GET_MAX_HIT(vict));
    affect_total(vict);
    break;
  case 21: /* hitroll */
    GET_REAL_HITROLL(vict) = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 22: /* hunger */
    if (!str_cmp(val_arg, "off"))
    {
      GET_COND(vict, HUNGER) = -1;
      send_to_char(ch, "%s's hunger is now off.\r\n", GET_NAME(vict));
    }
    else if (is_number(val_arg))
    {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, HUNGER) = value;
      send_to_char(ch, "%s's hunger set to %d.\r\n", GET_NAME(vict), value);
    }
    else
    {
      send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
      return (0);
    }
    break;
  case 23: /* int */
    RANGE(3, 50);
    GET_REAL_INT(vict) = value;
    affect_total(vict);
    break;
  case 24: /* invis */
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict)
    {
      send_to_char(ch, "You aren't godly enough for that!\r\n");
      return (0);
    }
    GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
    break;
  case 25: /* invistart */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
    break;
  case 26: /* killer */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
    break;
  case 27: /* level */
    if ((!IS_NPC(vict) && value > GET_LEVEL(ch)) || value > LVL_IMPL)
    {
      send_to_char(ch, "You can't do that.\r\n");
      return (0);
    }
    RANGE(1, LVL_IMPL);
    GET_LEVEL(vict) = value;
    break;
  case 28: /* loadroom */
    if (!str_cmp(val_arg, "off"))
    {
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
    }
    else if (is_number(val_arg))
    {
      rvnum = atoi(val_arg);
      if (real_room(rvnum) != NOWHERE)
      {
        SET_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
        GET_LOADROOM(vict) = rvnum;
        send_to_char(ch, "%s will enter at room #%d.\r\n", GET_NAME(vict), GET_LOADROOM(vict));
      }
      else
      {
        send_to_char(ch, "That room does not exist!\r\n");
        return (0);
      }
    }
    else
    {
      send_to_char(ch, "Must be 'off' or a room's virtual number.\r\n");
      return (0);
    }
    break;
  case 29: /* psp */
    GET_PSP(vict) = RANGE(0, GET_MAX_PSP(vict));
    affect_total(vict);
    break;
  case 30: /* maxhit */
    GET_REAL_MAX_HIT(vict) = RANGE(1, GET_LEVEL(vict) * 500);
    affect_total(vict);
    break;
  case 31: /* maxpsp */
    GET_REAL_MAX_PSP(vict) = RANGE(1, GET_LEVEL(vict) * 500);
    affect_total(vict);
    break;
  case 32: /* maxmove */
    GET_REAL_MAX_MOVE(vict) = RANGE(1, GET_LEVEL(vict) * 500);
    affect_total(vict);
    break;
  case 33: /* move */
    GET_MOVE(vict) = RANGE(0, GET_MAX_MOVE(vict));
    affect_total(vict);
    break;
  case 34: /* name */
    if (ch != vict && GET_LEVEL(ch) < LVL_IMPL)
    {
      send_to_char(ch, "Only Imps can change the name of other players.\r\n");
      return (0);
    }
    if (!change_player_name(ch, vict, val_arg))
    {
      send_to_char(ch, "Name has not been changed!\r\n");
      return (0);
    }
    break;
  case 35: /* nodelete */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
    break;
  case 36: /* nohassle */
    if (GET_LEVEL(ch) < LVL_STAFF && ch != vict)
    {
      send_to_char(ch, "You aren't godly enough for that!\r\n");
      return (0);
    }
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 37: /* nosummon */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    send_to_char(ch, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
    break;
  case 38: /* nowiz */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 39: /* olc */
    if (is_abbrev(val_arg, "socials") || is_abbrev(val_arg, "actions") || is_abbrev(val_arg, "aedit"))
      GET_OLC_ZONE(vict) = AEDIT_PERMISSION;
    else if (is_abbrev(val_arg, "hedit") || is_abbrev(val_arg, "help"))
      GET_OLC_ZONE(vict) = HEDIT_PERMISSION;
    else if (*val_arg == '*' || is_abbrev(val_arg, "all"))
      GET_OLC_ZONE(vict) = ALL_PERMISSION;
    else if (is_abbrev(val_arg, "off"))
      GET_OLC_ZONE(vict) = NOWHERE;
    else if (!is_number(val_arg))
    {
      send_to_char(ch, "Value must be a zone number, 'aedit', 'hedit', 'off' or 'all'.\r\n");
      return (0);
    }
    else
      GET_OLC_ZONE(vict) = atoi(val_arg);
    break;
  case 40: /* password */
    if (GET_LEVEL(vict) >= LVL_GRSTAFF)
    {
      send_to_char(ch, "You cannot change that.\r\n");
      return (0);
    }
    strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH); /* strncpy: OK (G_P:MAX_PWD_LENGTH) */
    *(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
    send_to_char(ch, "Password changed to '%s'.\r\n", val_arg);
    break;
  case 41: /* poofin */
    if ((vict == ch) || (GET_LEVEL(ch) == LVL_IMPL))
    {
      skip_spaces(&val_arg);

      if (POOFIN(vict))
        free(POOFIN(vict));

      if (!*val_arg)
        POOFIN(vict) = NULL;
      else
        POOFIN(vict) = strdup(val_arg);
    }
    break;
  case 42: /* poofout */
    if ((vict == ch) || (GET_LEVEL(ch) == LVL_IMPL))
    {
      skip_spaces(&val_arg);

      if (POOFOUT(vict))
        free(POOFOUT(vict));

      if (!*val_arg)
        POOFOUT(vict) = NULL;
      else
        POOFOUT(vict) = strdup(val_arg);
    }
    break;
  case 43: /* practices */
    GET_PRACTICES(vict) = RANGE(0, 100);
    break;
  case 44: /* quest */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;
  case 45: /* room */
    if ((rnum = real_room(value)) == NOWHERE)
    {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (0);
    }
    if (IN_ROOM(vict) != NOWHERE)
      char_from_room(vict);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(rnum), ZONE_WILDERNESS))
    {
      X_LOC(vict) = world[rnum].coords[0];
      Y_LOC(vict) = world[rnum].coords[1];
    }

    char_to_room(vict, rnum);
    break;
  case 46: /* screenwidth */
    GET_SCREEN_WIDTH(vict) = RANGE(40, 200);
    break;
  case 47: /* sex */
    if ((i = search_block(val_arg, genders, FALSE)) < 0)
    {
      send_to_char(ch, "Must be 'male', 'female', or 'neutral'.\r\n");
      return (0);
    }
    GET_SEX(vict) = i;
    break;
  case 48: /* showvnums */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SHOWVNUMS);
    break;
  case 49: /* siteok */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
    break;
  case 50: /* str */
    RANGE(3, 50);
    GET_REAL_STR(vict) = value;
    affect_total(vict);
    break;
  case 51: /* stradd */
    send_to_char(ch, "Stradd has been taken out.\r\n");
    affect_total(vict);
    break;
  case 52: /* thief */
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
    break;
  case 53: /* thirst */
    if (!str_cmp(val_arg, "off"))
    {
      GET_COND(vict, THIRST) = -1;
      send_to_char(ch, "%s's thirst is now off.\r\n", GET_NAME(vict));
    }
    else if (is_number(val_arg))
    {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, THIRST) = value;
      send_to_char(ch, "%s's thirst set to %d.\r\n", GET_NAME(vict), value);
    }
    else
    {
      send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
      return (0);
    }
    break;
  case 54: /* title */
    set_title(vict, val_arg);
    send_to_char(ch, "%s's title is now: %s\r\n", GET_NAME(vict), GET_TITLE(vict));
    break;
  case 55: /* variable */
    return perform_set_dg_var(ch, vict, val_arg);
    break;
  case 56: /* weight */
    GET_WEIGHT(vict) = value;
    affect_total(vict);
    break;
  case 57: /* wis */
    RANGE(3, 50);
    GET_REAL_WIS(vict) = value;
    affect_total(vict);
    break;
  case 58: /* questpoints */
    GET_QUESTPOINTS(vict) = RANGE(0, 100000000);
    break;
  case 59: /* questhistory */
    qvnum = atoi(val_arg);
    if (real_quest(qvnum) == NOTHING)
    {
      send_to_char(ch, "That quest doesn't exist.\r\n");
      return FALSE;
    }
    else
    {
      if (is_complete(vict, qvnum))
      {
        remove_completed_quest(vict, qvnum);
        send_to_char(ch, "Quest %d removed from history for player %s.\r\n", qvnum, GET_NAME(vict));
      }
      else
      {
        add_completed_quest(vict, qvnum);
        send_to_char(ch, "Quest %d added to history for player %s.\r\n", qvnum, GET_NAME(vict));
      }
      break;
    }
    break;
  case 60: /* training sessions */
    GET_TRAINS(vict) = RANGE(0, 250);
    break;
  case 61: /* race */
    if ((i = parse_race_long(val_arg)) == RACE_UNDEFINED)
    {
      send_to_char(ch, "That is not a race.\r\n");
      return (0);
    }
    GET_REAL_RACE(vict) = i;
    break;
  case 62: /* spellres spell resistance */
    GET_REAL_SPELL_RES(vict) = RANGE(0, 99);
    affect_total(vict);
    break;
  case 63: // size
    GET_REAL_SIZE(vict) = RANGE(0, NUM_SIZES - 1);
    affect_total(vict);
    break;
  case 64: // wizard level
    CLASS_LEVEL(vict, CLASS_WIZARD) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 65: // cleric level
    CLASS_LEVEL(vict, CLASS_CLERIC) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 66: // rogue level
    CLASS_LEVEL(vict, CLASS_ROGUE) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 67: // warrior level
    CLASS_LEVEL(vict, CLASS_WARRIOR) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 68: // monk level
    CLASS_LEVEL(vict, CLASS_MONK) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 69: // druid level
    CLASS_LEVEL(vict, CLASS_DRUID) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 70: /* boosts */
    GET_BOOSTS(vict) = RANGE(0, 20);
    break;
  case 71: // zerker level
    CLASS_LEVEL(vict, CLASS_BERSERKER) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 72: // sorc level
    CLASS_LEVEL(vict, CLASS_SORCERER) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 73: // paladin level
    CLASS_LEVEL(vict, CLASS_PALADIN) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 74: // ranger level
    CLASS_LEVEL(vict, CLASS_RANGER) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 75: // bard level
    CLASS_LEVEL(vict, CLASS_BARD) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 76: /* featpoints */
    GET_FEAT_POINTS(vict) = RANGE(0, 20);
    break;
  case 77: /* epicfeatpoints */
    GET_EPIC_FEAT_POINTS(vict) = RANGE(0, 20);
    break;
  case 78:                                                          /* classfeats (points) */
    two_arguments(val_arg, arg1, sizeof(arg1), arg2, sizeof(arg2)); /* set <name> classfeats <class> <#> */
    class = parse_class_long(arg1);
    if (class == CLASS_UNDEFINED)
    {
      send_to_char(ch, "Invalid class! <example: set zusuk classfeat warrior 2>\r\n");
      return 0;
    }
    value = atoi(arg2);
    GET_CLASS_FEATS(vict, class) = RANGE(0, 20);
    send_to_char(ch, "%s's %s for %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, arg1, value);
    break;
  case 79:                                                          /* epicclassfeats (points) */
    two_arguments(val_arg, arg1, sizeof(arg1), arg2, sizeof(arg2)); /* set <name> epicclassfeats <class> <#> */
    class = parse_class_long(arg1);
    if (class == CLASS_UNDEFINED)
    {
      send_to_char(ch, "Invalid class! <example: set zusuk epicclassfeat warrior 2>\r\n");
      return 0;
    }
    value = atoi(arg2);
    GET_EPIC_CLASS_FEATS(vict, class) = RANGE(0, 20);
    send_to_char(ch, "%s's %s for %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, arg1, value);
    break;
  case 80: /* accexp - account experience */
    change_account_xp(vict, RANGE(0, 99999999));
    break;
  case 81: // weapon-master level
    CLASS_LEVEL(vict, CLASS_WEAPON_MASTER) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 82: // arcane archer level
    CLASS_LEVEL(vict, CLASS_ARCANE_ARCHER) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 83: // stalwart defender level
    CLASS_LEVEL(vict, CLASS_STALWART_DEFENDER) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 84: // shifter level
    CLASS_LEVEL(vict, CLASS_SHIFTER) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 85: // duelist
    CLASS_LEVEL(vict, CLASS_DUELIST) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 86: /* GUI Mode */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_GUI_MODE);
    send_to_char(ch, "GUI Mode %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
    break;
  case 87: /* PRF_RP */
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_RP);
    break;
  case 88: // mystic theurge level
    CLASS_LEVEL(vict, CLASS_MYSTIC_THEURGE) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 89: /* addaccexp - Adds *additional* account experience */
    change_account_xp(vict, RANGE(0, 9999999));
    break;
  case 90: // alchemist level
    CLASS_LEVEL(vict, CLASS_ALCHEMIST) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 91: // arcane shadow
    CLASS_LEVEL(vict, CLASS_ARCANE_SHADOW) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 92: // sacred fist
    CLASS_LEVEL(vict, CLASS_SACRED_FIST) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;
  case 93: /* premade build class */
    if ((i = parse_class_long(val_arg)) == CLASS_UNDEFINED)
    {
      send_to_char(ch, "That is not a premade build class.\r\n");
      return (0);
    }
    GET_PREMADE_BUILD_CLASS(vict) = i;
    break;
  case 94: // psionicist
    CLASS_LEVEL(vict, CLASS_PSIONICIST) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;

  case 95:
    for (i = 0; i < NUM_DEITIES; i++)
    {
      if (deity_list[i].pantheon != DEITY_PANTHEON_ALL)
        continue;
      if (is_abbrev(val_arg, deity_list[i].name))
      {
        break;
      }
    }
    if (i < 0 || i >= NUM_DEITIES)
    {
      send_to_char(ch, "There is no deity by that name.\r\n");
      return (0);
    }
    GET_DEITY(vict) = i;
    break;

  case 96: // eldritch knight
    CLASS_LEVEL(vict, CLASS_ELDRITCH_KNIGHT) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;

  case 97: // spellsword
    CLASS_LEVEL(vict, CLASS_SPELLSWORD) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;

  case 98: // shadowdancer
    CLASS_LEVEL(vict, CLASS_SHADOW_DANCER) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;

  case 99: // blackguard
    CLASS_LEVEL(vict, CLASS_BLACKGUARD) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;

  case 100: // assassin
    CLASS_LEVEL(vict, CLASS_ASSASSIN) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;

  case 101: // inquisitor
    CLASS_LEVEL(vict, CLASS_INQUISITOR) = RANGE(0, LVL_IMMORT - 1);
    affect_total(vict);
    break;

  default:
    send_to_char(ch, "Can't set that!\r\n");
    return (0);
  }
  /* Show the new value of the variable */
  if (set_fields[mode].type == BINARY)
  {
    send_to_char(ch, "%s %s for %s.\r\n", set_fields[mode].cmd, ONOFF(on), GET_NAME(vict));
  }
  else if (set_fields[mode].type == NUMBER)
  {
    send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
  }
  else if (set_fields[mode].type == ADDER)
  {
    send_to_char(ch, "%s's %s increased by %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
  }
  else
    send_to_char(ch, "%s", CONFIG_OK);

  return (1);
}

void show_set_help(struct char_data *ch)
{
  const char *const set_levels[] = {"Imm", "God", "GrGod", "IMP"};
  const char *const set_targets[] = {"PC", "NPC", "BOTH"};
  const char *const set_types[] = {"MISC", "BINARY", "NUMBER", "ADDER"};
  char buf[MAX_STRING_LENGTH] = {'\0'};
  int i, len = 0, add_len = 0;

  len = snprintf(buf, sizeof(buf), "%sCommand             Lvl    Who?  Type%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
  for (i = 0; *(set_fields[i].cmd) != '\n'; i++)
  {
    if (set_fields[i].level <= GET_LEVEL(ch))
    {
      add_len = snprintf(buf + len, sizeof(buf) - len, "%-20s%-5s  %-4s  %-6s\r\n", set_fields[i].cmd,
                         set_levels[((int)(set_fields[i].level) - LVL_IMMORT)],
                         set_targets[(int)(set_fields[i].pcnpc) - 1],
                         set_types[(int)(set_fields[i].type)]);
      len += add_len;
    }
  }
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  char field[MAX_INPUT_LENGTH] = {'\0'}, name[MAX_INPUT_LENGTH] = {'\0'}, buf[MAX_INPUT_LENGTH] = {'\0'};
  int mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  half_chop_c(argument, name, sizeof(name), buf, sizeof(buf));

  if (!strcmp(name, "file"))
  {
    is_file = 1;
    half_chop(buf, name, buf);
  }
  else if (!str_cmp(name, "help"))
  {
    show_set_help(ch);
    return;
  }
  else if (!str_cmp(name, "player"))
  {
    is_player = 1;
    half_chop(buf, name, buf);
  }
  else if (!str_cmp(name, "mob"))
    half_chop(buf, name, buf);

  half_chop(buf, field, buf);

  if (!*name || !*field)
  {
    send_to_char(ch, "Usage: set <victim> <field> <value>\r\n");
    send_to_char(ch, "       %sset help%s will display valid fields\r\n",
                 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
    return;
  }

  /* find the target */
  if (!is_file)
  {
    if (is_player)
    {
      if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD)))
      {
        send_to_char(ch, "There is no such player.\r\n");
        return;
      }
    }
    else
    { /* is_mob */
      if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD)))
      {
        send_to_char(ch, "There is no such creature.\r\n");
        return;
      }
    }
  }
  else if (is_file)
  {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    CREATE(cbuf->player_specials, struct player_special_data, 1);
    new_mobile_data(cbuf);
    /* Allocate mobile event list */
    // cbuf->events = create_list();
    if ((player_i = load_char(name, cbuf)) > -1)
    {
      if (GET_LEVEL(cbuf) > GET_LEVEL(ch))
      {
        free_char(cbuf);
        send_to_char(ch, "Sorry, you can't do that.\r\n");
        return;
      }
      vict = cbuf;
    }
    else
    {
      free_char(cbuf);
      send_to_char(ch, "There is no such player.\r\n");
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strncmp(field, set_fields[mode].cmd, len))
      break;

  if (*(set_fields[mode].cmd) == '\n')
  {
    retval = 0; /* skips saving below */
    send_to_char(ch, "Can't set that!\r\n");
  }
  else
    /* perform the set */
    retval = perform_set(ch, vict, mode, buf);

  /* save the character if a change was made */
  if (retval)
  {
    if (!is_file && !IS_NPC(vict))
      save_char(vict, 0);
    if (is_file)
    {
      GET_PFILEPOS(cbuf) = player_i;
      save_char(cbuf, 0);
      send_to_char(ch, "Saved in file.\r\n");
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}

// not yet implemented - zusuk

ACMD(do_savemobs)
{
  if (GET_LEVEL(ch) < LVL_IMPL)
    send_to_char(ch, "You are not holy enough to use this privelege.\n\r");
  else
  {
    send_to_char(ch, "Not yet implemented.\n\r");
  }
}

ACMD(do_saveall)
{
  if (GET_LEVEL(ch) < LVL_BUILDER)
    send_to_char(ch, "You are not holy enough to use this privelege.\n\r");
  else
  {
    save_all();
    House_save_all();
    // hlqedit_save_to_disk(OLC_ZNUM(d));
    send_to_char(ch, "World and house files saved.\n\r");
  }
}

/* for a given zone, find key vnums that don't match the zone
   just a simple utility to help clean up imported zones
   keycheck
   keycheck .
   keycheck <zone #>
-Zusuk
*/
ACMD(do_keycheck)
{
  obj_vnum keynum = NOWHERE;
  zone_rnum rzone = NOWHERE;
  room_vnum i = NOWHERE, bottom = NOWHERE, top = NOWHERE;
  int j, len = 0;
  char zone_num[MAX_INPUT_LENGTH] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'};
  struct obj_data *obj = NULL;

  one_argument(argument, zone_num, sizeof(zone_num));

  /* dummy check, completely unnecessary as far as I know :)  */
  if (!top_of_world)
  {
    send_to_char(ch, "Sorry, there's no top of the world?!\r\n");
    return;
  }

  /* figure out which zone we want to look at */
  if (!*zone_num || *zone_num == '.')
  {
    rzone = world[IN_ROOM(ch)].zone;
  }
  else
  {
    rzone = real_zone(atoi(zone_num));

    if (rzone == NOWHERE)
    {
      send_to_char(ch, "Sorry, there's no zone with that number\r\n");
      return;
    }
  }

  /* set the vnum range we are looking at */
  bottom = zone_table[rzone].bot;
  top = zone_table[rzone].top;

  /* start building the string */
  len = strlcpy(buf,
                "VNum     Name                                         Exit:Key-VNum\r\n"
                "-------- -------------------------------------------- -------------\r\n",
                sizeof(buf));

  /* here is a loop that will go through the list of rooms by vnum */
  for (i = bottom; i <= top; i++)
  {

    /* easy out, room doesn't exist */
    if (real_room(i) == NOWHERE)
      continue;

    for (j = 0; j < DIR_COUNT; j++)
    {
      /* easy exits */
      if (W_EXIT(real_room(i), j) == NULL)
        continue;
      if (W_EXIT(real_room(i), j)->to_room == NOWHERE)
        continue;

      keynum = W_EXIT(real_room(i), j)->key;

      if (keynum != NOWHERE && keynum > 0)
      {
        if (keynum < bottom || keynum > top)
        {
          len += snprintf(buf + len, sizeof(buf) - len, "[%s%-6d%s] %s%-*s%s %s%-5s:%d%s\r\n",
                          QGRN, i, QNRM,
                          QCYN, count_color_chars(world[real_room(i)].name) + 44, world[real_room(i)].name, QNRM,
                          QBRED, dirs[j], keynum, QNRM);
        }
      }
    }

    if (len > sizeof(buf))
      break;
  }

  /* going through the objects now! */
  len += snprintf(buf + len, sizeof(buf) - len,
                  "\r\n"
                  "VNum     Object Name                                  Key-VNum\r\n"
                  "-------- -------------------------------------------- --------\r\n");

  /* here is a loop that will go through the list of objects by vnum */
  for (i = bottom; i <= top; i++)
  {
    /* in case we overflowed earlier */
    if (len > sizeof(buf))
      break;

    /* easy out, object doesn't exist */
    if (real_object(i) == NOTHING)
      continue;

    /* reference the object for ease now */
    obj = &obj_proto[real_object(i)];

    if (!obj)
      continue;

    if (GET_OBJ_VAL(obj, 2) == -1 ||
        GET_OBJ_VAL(obj, 2) == 0 ||
        GET_OBJ_VAL(obj, 2) == 100000) /* the 'no key' values */
      continue;

    /* has to be a container and out of bottom/top range */
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER || GET_OBJ_TYPE(obj) == ITEM_AMMO_POUCH)
    {
      if (GET_OBJ_VAL(obj, 2) < bottom || GET_OBJ_VAL(obj, 2) > top)
      {
        len += snprintf(buf + len, sizeof(buf) - len, "[%s%-6d%s] %s%-*s%s %s%d%s\r\n",
                        QGRN, i, QNRM,
                        QCYN, count_color_chars(GET_OBJ_SHORT(obj)) + 44, GET_OBJ_SHORT(obj), QNRM,
                        QBRED, GET_OBJ_VAL(obj, 2), QNRM);
      }
    }
  }

  if (len <= 0)
    send_to_char(ch, "Nothing found for zone specified.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
}

/* a command to check all the external zones connected (via exits) to a given zone */
ACMD(do_links)
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  room_rnum nr, to_room;
  int first, last, j;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  skip_spaces_c(&argument);
  one_argument(argument, arg, sizeof(arg));

  if (!is_number(arg))
  {
    zrnum = world[IN_ROOM(ch)].zone;
    zvnum = zone_table[zrnum].number;
  }
  else
  {
    zvnum = atoi(arg);
    zrnum = real_zone(zvnum);
  }

  if (zrnum == NOWHERE || zvnum == NOWHERE)
  {
    send_to_char(ch, "No zone was found with that number.\n\r");
    return;
  }

  last = zone_table[zrnum].top;
  first = zone_table[zrnum].bot;

  send_to_char(ch, "Zone %d is linked to the following zones:\r\n", zvnum);
  for (nr = 0; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++)
  {
    if (GET_ROOM_VNUM(nr) >= first)
    {
      for (j = 0; j < DIR_COUNT; j++)
      {
        if (world[nr].dir_option[j])
        {
          to_room = world[nr].dir_option[j]->to_room;
          if (to_room != NOWHERE && (zrnum != world[to_room].zone))
            send_to_char(ch, "%3d %-30s at %5d (%-5s) ---> %5d\r\n",
                         zone_table[world[to_room].zone].number,
                         zone_table[world[to_room].zone].name,
                         GET_ROOM_VNUM(nr), dirs[j], world[to_room].number);
        }
      }
    }
  }
}

/* Zone Checker Code below */
/*mob limits*/
#define MAX_DAMROLL_ALLOWED MAX(GET_LEVEL(mob) / 5, 1)
#define MAX_HITROLL_ALLOWED MAX(GET_LEVEL(mob) / 3, 1)
#define MAX_MOB_GOLD_ALLOWED GET_LEVEL(mob) * 20
#define MAX_EXP_ALLOWED GET_LEVEL(mob) * GET_LEVEL(mob) * 120
#define MAX_LEVEL_ALLOWED LVL_IMPL
#define GET_OBJ_AVG_DAM(obj) (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1))
/* arbitrary limit for per round dam */
#define MAX_MOB_DAM_ALLOWED 500

#define ZCMD2 zone_table[zone].cmd[cmd_no] /*fom DB.C*/

/*item limits*/
#define MAX_DAM_ALLOWED 50 /* for weapons  - avg. dam*/
#define MAX_AFFECTS_ALLOWED 3
#define MAX_OBJ_GOLD_ALLOWED 10000

/* Armor class limits*/
#define TOTAL_WEAR_CHECKS (NUM_ITEM_WEARS - 1) /* no take flag */

const struct zcheck_armor
{
  bitvector_t bitvector; /* from Structs.h                       */
  int ac_allowed;        /* Max. AC allowed for this body part  */
  const char *message;   /* phrase for error message            */
} zarmor[TOTAL_WEAR_CHECKS] = {
    {ITEM_WEAR_FINGER, 1, "Ring"}, // 0
    {ITEM_WEAR_NECK, 1, "Necklace"},
    {ITEM_WEAR_BODY, 35, "Body armor"},
    {ITEM_WEAR_HEAD, 15, "Head gear"},
    {ITEM_WEAR_LEGS, 15, "Legwear"},
    {ITEM_WEAR_FEET, 1, "Footwear"}, // 5
    {ITEM_WEAR_HANDS, 1, "Glove"},
    {ITEM_WEAR_ARMS, 15, "Armwear"},
    {ITEM_WEAR_SHIELD, 40, "Shield"},
    {ITEM_WEAR_ABOUT, 1, "Cloak"},
    {ITEM_WEAR_WAIST, 1, "Belt"}, // 10
    {ITEM_WEAR_WRIST, 1, "Wristwear"},
    {ITEM_WEAR_WIELD, 1, "Weapon"},
    {ITEM_WEAR_HOLD, 1, "Held item"},
    {ITEM_WEAR_FACE, 1, "Face"},
    {ITEM_WEAR_AMMO_POUCH, 1, "Ammo pouch"}, // 15
    {ITEM_WEAR_EAR, 1, "Earring"},
    {ITEM_WEAR_EYES, 1, "Eyewear"},
    {ITEM_WEAR_BADGE, 1, "Badge"} // 18
};

/*These are strictly boolean*/
#define CAN_WEAR_WEAPONS 0  /* toggle - can a weapon also be a piece of armor? */
#define MAX_APPLIES_LIMIT 1 /* toggle - is there a limit at all?               */
#define CHECK_ITEM_RENT 0   /* do we check for rent cost == 0 ?                */
#define CHECK_ITEM_COST 0   /* do we check for item cost == 0 ?                */

/* Applies limits !! Very Important:  Keep these in the same order as in Structs.h.
 * To ignore an apply, set max_aff to -99. These will be ignored if MAX_APPLIES_LIMIT = 0 */
const struct zcheck_affs
{
  int aff_type;        /*from Structs.h*/
  int min_aff;         /*min. allowed value*/
  int max_aff;         /*max. allowed value*/
  const char *message; /*phrase for error message*/
} zaffs[NUM_APPLIES] = {
    {APPLY_NONE, 0, -99, "unused0"}, // 0
    {APPLY_STR, -5, 9, "strength"},
    {APPLY_DEX, -5, 9, "dexterity"},
    {APPLY_INT, -5, 9, "intelligence"},
    {APPLY_WIS, -5, 9, "wisdom"},
    {APPLY_CON, -5, 9, "constitution"}, // 5
    {APPLY_CHA, -5, 9, "charisma"},
    {APPLY_CLASS, 0, 0, "class"},
    {APPLY_LEVEL, 0, 0, "level"},
    {APPLY_AGE, 0, 0, "age"},
    {APPLY_CHAR_WEIGHT, 0, 0, "character weight"}, // 10
    {APPLY_CHAR_HEIGHT, 0, 0, "character height"},
    {APPLY_PSP, -90, 120, "psp"},
    {APPLY_HIT, -90, 120, "hit points"},
    {APPLY_MOVE, -90, 120, "movement"},
    {APPLY_GOLD, 0, 0, "gold"}, // 15
    {APPLY_EXP, 0, 0, "experience"},
    {APPLY_AC, -10, 10, "!Unused!"},
    {APPLY_HITROLL, 0, -99, "hitroll"},                     /* Handled seperately below */
    {APPLY_DAMROLL, 0, -99, "damroll"},                     /* Handled seperately below */
    {APPLY_SAVING_FORT, -5, 9, "saving throw (fortitude)"}, // 20
    {APPLY_SAVING_REFL, -5, 9, "saving throw (reflex)"},
    {APPLY_SAVING_WILL, -5, 9, "saving throw (willpower)"},
    {APPLY_SAVING_POISON, -5, 9, "saving throw (poison)"},
    {APPLY_SAVING_DEATH, -5, 9, "saving throw (death)"},
    {APPLY_SPELL_RES, -90, 99, "spell resistance"}, // 25
    {APPLY_SIZE, -1, 1, "size mod"},
    {APPLY_AC_NEW, -2, 2, "magical AC"},

    {APPLY_RES_FIRE, -20, 20, "fire resistance"},
    {APPLY_RES_COLD, -20, 20, "cold resistance"},
    {APPLY_RES_AIR, -20, 20, "air resistance"}, // 30
    {APPLY_RES_EARTH, -20, 20, "earth resistance"},
    {APPLY_RES_ACID, -20, 20, "acid resistance"},
    {APPLY_RES_HOLY, -20, 20, "holy resistance"},
    {APPLY_RES_ELECTRIC, -20, 20, "electric resistance"},
    {APPLY_RES_UNHOLY, -20, 20, "unholy resistance"}, // 35
    {APPLY_RES_SLICE, -20, 20, "slice resistance"},
    {APPLY_RES_PUNCTURE, -20, 20, "puncture resistance"},
    {APPLY_RES_FORCE, -20, 20, "force resistance"},
    {APPLY_RES_SOUND, -20, 20, "sound resistance"},
    {APPLY_RES_POISON, -20, 20, "poison resistance"}, // 40
    {APPLY_RES_DISEASE, -20, 20, "disease resistance"},
    {APPLY_RES_NEGATIVE, -20, 20, "negative resistance"},
    {APPLY_RES_ILLUSION, -20, 20, "illusion resistance"},
    {APPLY_RES_MENTAL, -20, 20, "mental resistance"},
    {APPLY_RES_LIGHT, -20, 20, "light resistance"}, // 45
    {APPLY_RES_ENERGY, -20, 20, "energy resistance"},
    {APPLY_RES_WATER, -20, 20, "water resistance"},
    {APPLY_DR, -20, 20, "damage reduction"},
    {APPLY_FEAT, 1, FEAT_LAST_FEAT, "grant feat"}};

/* These are ABS() values. */
#define MAX_APPLY_HITROLL_TOTAL 11
#define MAX_APPLY_DAMROLL_TOTAL 11

/*room limits*/
/* Off limit zones are any zones a player should NOT be able to walk to (ex. Limbo) */
const int offlimit_zones[] = {0, 12, 13, 14, -1}; /*what zones can no room connect to (virtual num) */
#define MIN_ROOM_DESC_LENGTH 250                  /* at least one line - set to 0 to not care. */
#define MAX_COLUMN_WIDTH 80                       /* at most 80 chars per line */

ACMD(do_zcheck)
{
  zone_rnum zrnum;
  struct obj_data *obj;
  struct char_data *mob = NULL;
  room_vnum exroom = 0;
  int ac = 0;
  int affs = 0, tohit, todam, value;
  int i = 0, j = 0, k = 0, l = 0, m = 0, found = 0; /* found is used as a 'send now' flag*/
  char buf[MAX_STRING_LENGTH] = {'\0'};
  float avg_dam;
  size_t len = 0;
  // struct extra_descr_data *ext, *ext2;
  one_argument(argument, buf, sizeof(buf));

  if (!is_number(buf) || !strcmp(buf, "."))
    zrnum = world[IN_ROOM(ch)].zone;
  else
    zrnum = real_zone(atoi(buf));

  if (zrnum == NOWHERE)
  {
    send_to_char(ch, "Check what zone ?\r\n");
    return;
  }
  else
    send_to_char(ch, "Checking zone %d!\r\n", zone_table[zrnum].number);

  /* Check mobs */

  send_to_char(ch, "Checking Mobs for limits...\r\n");
  /*check mobs first*/
  for (i = 0; i < top_of_mobt; i++)
  {
    if (real_zone_by_thing(mob_index[i].vnum) == zrnum)
    { /*is mob in this zone?*/
      mob = &mob_proto[i];
      if (!strcmp(mob->player.name, "mob unfinished") && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Alias hasn't been set.\r\n");

      if (!strcmp(mob->player.short_descr, "the unfinished mob") && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Short description hasn't been set.\r\n");

      if (!strncmp(mob->player.long_descr, "An unfinished mob stands here.", 30) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Long description hasn't been set.\r\n");

      if (mob->player.description && *mob->player.description)
      {
        if (!strncmp(mob->player.description, "It looks unfinished.", 20) && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Description hasn't been set.\r\n");
        /*else if (strncmp(mob->player.description, "   ", 3) && (found = 1))
          len += snprintf(buf + len, sizeof (buf) - len,
                "- Description hasn't been formatted. (/fi)\r\n");*/
      }

      if (GET_LEVEL(mob) > MAX_LEVEL_ALLOWED && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Is level %d (limit: 1-%d)\r\n",
                        GET_LEVEL(mob), MAX_LEVEL_ALLOWED);

      if (GET_DAMROLL(mob) > MAX_DAMROLL_ALLOWED && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Damroll of %d is too high (limit: %d)\r\n",
                        GET_DAMROLL(mob), MAX_DAMROLL_ALLOWED);

      if (GET_HITROLL(mob) > MAX_HITROLL_ALLOWED && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Hitroll of %d is too high (limit: %d)\r\n",
                        GET_HITROLL(mob), MAX_HITROLL_ALLOWED);

      /* avg. dam including damroll per round of combat */
      avg_dam = (((mob->mob_specials.damsizedice / 2.0) * mob->mob_specials.damnodice) + GET_DAMROLL(mob));
      if (avg_dam > MAX_MOB_DAM_ALLOWED && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- average damage of %4.1f is too high (limit: %d)\r\n",
                        avg_dam, MAX_MOB_DAM_ALLOWED);

      if (mob->mob_specials.damsizedice == 1 &&
          mob->mob_specials.damnodice == 1 &&
          GET_LEVEL(mob) == 0 &&
          (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Needs to be fixed - %sAutogenerate!%s\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));

      if (MOB_FLAGGED(mob, MOB_AGGRESSIVE) && (MOB_FLAGGED(mob, MOB_AGGR_GOOD) || MOB_FLAGGED(mob, MOB_AGGR_EVIL) || MOB_FLAGGED(mob, MOB_AGGR_NEUTRAL)) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Both aggresive and agressive to align.\r\n");

      if ((GET_GOLD(mob) > MAX_MOB_GOLD_ALLOWED) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Set to %d Gold (limit : %d).\r\n",
                        GET_GOLD(mob),
                        MAX_MOB_GOLD_ALLOWED);

      if (GET_EXP(mob) > MAX_EXP_ALLOWED && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Has %d experience (limit: %d)\r\n",
                        GET_EXP(mob), MAX_EXP_ALLOWED);
      if ((AFF_FLAGGED(mob, AFF_CHARM) || AFF_FLAGGED(mob, AFF_POISON)) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Has illegal affection bits set (%s %s)\r\n",
                        AFF_FLAGGED(mob, AFF_CHARM) ? "CHARM" : "",
                        AFF_FLAGGED(mob, AFF_POISON) ? "POISON" : "");

      if (!MOB_FLAGGED(mob, MOB_SENTINEL) && !MOB_FLAGGED(mob, MOB_STAY_ZONE) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Neither SENTINEL nor STAY_ZONE bits set.\r\n");

      if (MOB_FLAGGED(mob, MOB_SPEC) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- SPEC flag needs to be removed.\r\n");

      /* Additional mob checks.*/
      if (found)
      {
        send_to_char(ch,
                     "%s[%5d]%s %-30s: %s\r\n",
                     CCCYN(ch, C_NRM), GET_MOB_VNUM(mob),
                     CCYEL(ch, C_NRM), GET_NAME(mob),
                     CCNRM(ch, C_NRM));
        send_to_char(ch, "%s", buf);
      }
      /* reset buffers and found flag */
      strlcpy(buf, "", sizeof(buf));
      found = 0;
      len = 0;
    } /* mob is in zone */
  }   /* check mobs */

  /* Check objects */
  send_to_char(ch, "\r\nChecking Objects for limits...\r\n");
  for (i = 0; i < top_of_objt; i++)
  {
    if (real_zone_by_thing(obj_index[i].vnum) == zrnum)
    { /*is object in this zone?*/
      obj = &obj_proto[i];
      switch (GET_OBJ_TYPE(obj))
      {
      case ITEM_MONEY:
        if ((value = GET_OBJ_VAL(obj, 0)) > MAX_OBJ_GOLD_ALLOWED && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Is worth %d (money limit %d coins).\r\n",
                          value, MAX_OBJ_GOLD_ALLOWED);
        break;
      case ITEM_WEAPON:
        if (GET_OBJ_VAL(obj, 3) >= NUM_ATTACK_TYPES && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has out of range attack type %d.\r\n",
                          GET_OBJ_VAL(obj, 3));

        if (GET_OBJ_AVG_DAM(obj) > MAX_DAM_ALLOWED && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Damroll is %2.1f (limit %d)\r\n",
                          GET_OBJ_AVG_DAM(obj), MAX_DAM_ALLOWED);
        break;
      case ITEM_CLANARMOR:
        if (GET_OBJ_VAL(obj, 2) == NO_CLAN)
        {
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Clan ID not set on CLANARMOR\r\n");
        }
        else if (real_clan(GET_OBJ_VAL(obj, 2)) == NO_CLAN)
        {
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Invalid Clan ID on CLANARMOR\r\n");
        }
        /* drop through to normal armor checks */
      case ITEM_ARMOR:
        ac = GET_OBJ_VAL(obj, 0);
        for (j = 0; j < TOTAL_WEAR_CHECKS; j++)
        {
          if (CAN_WEAR(obj, zarmor[j].bitvector) && (ac > zarmor[j].ac_allowed) && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Has AC %d (%s limit is %d)\r\n",
                            ac, zarmor[j].message, zarmor[j].ac_allowed);
        }
        break;

      } /*switch on Item_Type*/

      if (!CAN_WEAR(obj, ITEM_WEAR_TAKE))
      {
        if ((GET_OBJ_COST(obj) || (GET_OBJ_WEIGHT(obj) && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) ||
             GET_OBJ_RENT(obj)) &&
            (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- is NO_TAKE, but has cost (%d) weight (%d) or rent (%d) set.\r\n",
                          GET_OBJ_COST(obj), GET_OBJ_WEIGHT(obj), GET_OBJ_RENT(obj));
      }
      else
      {
        if (GET_OBJ_COST(obj) == 0 && (found = 1) && GET_OBJ_TYPE(obj) != ITEM_TRASH)
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has 0 cost (min. 1).\r\n");

        if (GET_OBJ_WEIGHT(obj) == 0 && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has 0 weight (min. 1).\r\n");

        if (GET_OBJ_WEIGHT(obj) > MAX_OBJ_WEIGHT && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "  Weight is too high: %d (limit  %d).\r\n",
                          GET_OBJ_WEIGHT(obj), MAX_OBJ_WEIGHT);

        if (GET_OBJ_COST(obj) > MAX_OBJ_COST && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has %d cost (max %d).\r\n",
                          GET_OBJ_COST(obj), MAX_OBJ_COST);
      }

      if (GET_OBJ_LEVEL(obj) > LVL_IMMORT - 1 && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- has min level set to %d (max %d).\r\n",
                        GET_OBJ_LEVEL(obj), LVL_IMMORT - 1);

      if (obj->action_description && *obj->action_description &&
          GET_OBJ_TYPE(obj) != ITEM_STAFF &&
          GET_OBJ_TYPE(obj) != ITEM_WAND &&
          GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
          GET_OBJ_TYPE(obj) != ITEM_NOTE && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- has action_description set, but is inappropriate type.\r\n");

      /*first check for over-all affections*/
      for (affs = 0, j = 0; j < MAX_OBJ_AFFECT; j++)
        if (obj->affected[j].modifier)
          affs++;

      if (affs > MAX_AFFECTS_ALLOWED && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- has %d affects (limit %d).\r\n",
                        affs, MAX_AFFECTS_ALLOWED);

      /*check for out of range affections. */
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        if (zaffs[(int)obj->affected[j].location].max_aff != -99 && /* only care if a range is set */
            (obj->affected[j].modifier > zaffs[(int)obj->affected[j].location].max_aff ||
             obj->affected[j].modifier < zaffs[(int)obj->affected[j].location].min_aff ||
             zaffs[(int)obj->affected[j].location].min_aff == zaffs[(int)obj->affected[j].location].max_aff) &&
            (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- apply to %s is %d (limit %d - %d).\r\n",
                          zaffs[(int)obj->affected[j].location].message,
                          obj->affected[j].modifier,
                          zaffs[(int)obj->affected[j].location].min_aff,
                          zaffs[(int)obj->affected[j].location].max_aff);

      /* special handling of +hit and +dam because of +hit_n_dam */
      for (todam = 0, tohit = 0, j = 0; j < MAX_OBJ_AFFECT; j++)
      {
        if (obj->affected[j].location == APPLY_HITROLL)
          tohit += obj->affected[j].modifier;
        if (obj->affected[j].location == APPLY_DAMROLL)
          todam += obj->affected[j].modifier;
      }
      if (abs(todam) > MAX_APPLY_DAMROLL_TOTAL && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- total damroll %d out of range (limit +/-%d.\r\n",
                        todam, MAX_APPLY_DAMROLL_TOTAL);
      if (abs(tohit) > MAX_APPLY_HITROLL_TOTAL && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- total hitroll %d out of range (limit +/-%d).\r\n",
                        tohit, MAX_APPLY_HITROLL_TOTAL);

      /*for (ext2 = NULL, ext = obj->ex_description; ext; ext = ext->next)
        if (strncmp(ext->description, "   ", 3))
          ext2 = ext;

      if (ext2 && (found = 1))
        len += snprintf(buf + len, sizeof (buf) - len,
              "- has unformatted extra description\r\n"); */
      /* Additional object checks. */
      if (found)
      {
        send_to_char(ch, "[%5d] %-30s: \r\n", GET_OBJ_VNUM(obj), obj->short_description);
        send_to_char(ch, "%s", buf);
      }
      strlcpy(buf, "", sizeof(buf));
      len = 0;
      found = 0;
    } /*object is in zone*/
  }   /*check objects*/

  /* Check rooms */
  send_to_char(ch, "\r\nChecking Rooms for limits...\r\n");
  for (i = 0; i < top_of_world; i++)
  {
    if (world[i].zone == zrnum)
    {
      for (j = 0; j < DIR_COUNT; j++)
      {
        /*check for exit, but ignore off limits if you're in an offlimit zone*/
        if (!world[i].dir_option[j])
          continue;
        exroom = world[i].dir_option[j]->to_room;
        if (exroom == NOWHERE)
          continue;
        if (world[exroom].zone == zrnum)
          continue;
        if (world[exroom].zone == world[i].zone)
          continue;

        for (k = 0; offlimit_zones[k] != -1; k++)
        {
          if (world[exroom].zone == real_zone(offlimit_zones[k]) && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Exit %s cannot connect to %d (zone off limits).\r\n",
                            dirs[j], world[exroom].number);
        } /* for (k.. */
      }   /* cycle directions */

      if (ROOM_FLAGGED(i, ROOM_ATRIUM) || ROOM_FLAGGED(i, ROOM_HOUSE) || ROOM_FLAGGED(i, ROOM_HOUSE_CRASH) || ROOM_FLAGGED(i, ROOM_OLC) || ROOM_FLAGGED(i, ROOM_BFS_MARK))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Has illegal affection bits set (%s %s %s %s %s)\r\n",
                        ROOM_FLAGGED(i, ROOM_ATRIUM) ? "ATRIUM" : "",
                        ROOM_FLAGGED(i, ROOM_HOUSE) ? "HOUSE" : "",
                        ROOM_FLAGGED(i, ROOM_HOUSE_CRASH) ? "HCRSH" : "",
                        ROOM_FLAGGED(i, ROOM_OLC) ? "OLC" : "",
                        ROOM_FLAGGED(i, ROOM_BFS_MARK) ? "*" : "");

      if ((MIN_ROOM_DESC_LENGTH) && strlen(world[i].description) < MIN_ROOM_DESC_LENGTH && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description is too short. (%4.4d of min. %d characters).\r\n",
                        (int)strlen(world[i].description), MIN_ROOM_DESC_LENGTH);

      if (strncmp(world[i].description, "   ", 3) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description not formatted with indent (/fi in the editor).\r\n");

      /* strcspan = size of text in first arg before any character in second arg */
      if ((strcspn(world[i].description, "\r\n") > MAX_COLUMN_WIDTH) && (found = 1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description not wrapped at %d chars (/fi in the editor).\r\n",
                        MAX_COLUMN_WIDTH);

      /*for (ext2 = NULL, ext = world[i].ex_description; ext; ext = ext->next)
        if (strncmp(ext->description, "   ", 3))
          ext2 = ext;

      if (ext2 && (found = 1))
        len += snprintf(buf + len, sizeof (buf) - len,
              "- has unformatted extra description\r\n");*/

      if (found)
      {
        send_to_char(ch, "[%5d] %-30s: \r\n",
                     world[i].number, world[i].name ? world[i].name : "An unnamed room");
        send_to_char(ch, "%s", buf);
        strlcpy(buf, "", sizeof(buf));
        len = 0;
        found = 0;
      }
    } /*is room in this zone?*/
  }   /*checking rooms*/

  for (i = 0; i < top_of_world; i++)
  {
    if (world[i].zone == zrnum)
    {
      m++;
      for (j = 0, k = 0; j < DIR_COUNT; j++)
        if (!world[i].dir_option[j])
          k++;

      if (k == DIR_COUNT)
        l++;
    }
  }
  if (l * 3 > m)
    send_to_char(ch, "More than 1/3 of the rooms are not linked.\r\n");
}

static void mob_checkload(struct char_data *ch, mob_vnum mvnum)
{
  int cmd_no;
  zone_rnum zone;
  mob_rnum mrnum = real_mobile(mvnum);

  if (mrnum == NOBODY)
  {
    send_to_char(ch, "That mob does not exist.\r\n");
    return;
  }

  send_to_char(ch, "Checking load info for the mob %s...\r\n",
               mob_proto[mrnum].player.short_descr);

  for (zone = 0; zone <= top_of_zone_table; zone++)
  {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++)
    {
      if (ZCMD2.command != 'M')
        continue;

      /* read a mobile */
      if (ZCMD2.arg1 == mrnum)
      {
        send_to_char(ch, "  [%5d] %s (%d MAX)\r\n",
                     world[ZCMD2.arg3].number,
                     world[ZCMD2.arg3].name,
                     ZCMD2.arg2);
      }
    }
  }
}

static void obj_checkload(struct char_data *ch, obj_vnum ovnum)
{
  int cmd_no;
  zone_rnum zone;
  obj_rnum ornum = real_object(ovnum);
  room_vnum lastroom_v = 0;
  room_rnum lastroom_r = 0;
  mob_rnum lastmob_r = 0;

  if (ornum == NOTHING)
  {
    send_to_char(ch, "That object does not exist.\r\n");
    return;
  }

  send_to_char(ch, "Checking load info for the obj %s...\r\n",
               obj_proto[ornum].short_description);

  for (zone = 0; zone <= top_of_zone_table; zone++)
  {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++)
    {
      switch (ZCMD2.command)
      {
      case 'M':
        lastroom_v = world[ZCMD2.arg3].number;
        lastroom_r = ZCMD2.arg3;
        lastmob_r = ZCMD2.arg1;
        break;
      case 'O': /* read an object */
        lastroom_v = world[ZCMD2.arg3].number;
        lastroom_r = ZCMD2.arg3;
        if (ZCMD2.arg1 == ornum)
          send_to_char(ch, "  [%5d] %s (%d Max)\r\n",
                       lastroom_v,
                       world[lastroom_r].name,
                       ZCMD2.arg2);
        break;
      case 'P': /* object to object */
        if (ZCMD2.arg1 == ornum)
          send_to_char(ch, "  [%5d] %s (Put in another object [%d Max])\r\n",
                       lastroom_v,
                       world[lastroom_r].name,
                       ZCMD2.arg2);
        break;
      case 'G': /* obj_to_char */
        if (ZCMD2.arg1 == ornum)
          send_to_char(ch, "  [%5d] %s (Given to %s [%d][%d Max])\r\n",
                       lastroom_v,
                       world[lastroom_r].name,
                       mob_proto[lastmob_r].player.short_descr,
                       mob_index[lastmob_r].vnum,
                       ZCMD2.arg2);
        break;
      case 'E': /* object to equipment list */
        if (ZCMD2.arg1 == ornum)
          send_to_char(ch, "  [%5d] %s (Equipped to %s [%d][%d Max])\r\n",
                       lastroom_v,
                       world[lastroom_r].name,
                       mob_proto[lastmob_r].player.short_descr,
                       mob_index[lastmob_r].vnum,
                       ZCMD2.arg2);
        break;
      case 'R': /* rem obj from room */
        lastroom_v = world[ZCMD2.arg1].number;
        lastroom_r = ZCMD2.arg1;
        if (ZCMD2.arg2 == ornum)
          send_to_char(ch, "  [%5d] %s (Removed from room)\r\n",
                       lastroom_v,
                       world[lastroom_r].name);
        break;
      } /* switch */
    }   /*for cmd_no......*/
  }     /*for zone...*/
}

static void trg_checkload(struct char_data *ch, trig_vnum tvnum)
{
  int cmd_no, found = 0;
  zone_rnum zone;
  trig_rnum trnum = real_trigger(tvnum);
  room_vnum lastroom_v = 0;
  room_rnum lastroom_r = 0, k;
  mob_rnum lastmob_r = 0, i;
  obj_rnum lastobj_r = 0, j;
  struct trig_proto_list *tpl;

  if (trnum == NOTHING)
  {
    send_to_char(ch, "That trigger does not exist.\r\n");
    return;
  }

  send_to_char(ch, "Checking load info for the %s trigger '%s':\r\n",
               trig_index[trnum]->proto->attach_type == MOB_TRIGGER ? "mobile" : (trig_index[trnum]->proto->attach_type == OBJ_TRIGGER ? "object" : "room"),
               trig_index[trnum]->proto->name);

  for (zone = 0; zone <= top_of_zone_table; zone++)
  {
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++)
    {
      switch (ZCMD2.command)
      {
      case 'M':
        lastroom_v = world[ZCMD2.arg3].number;
        lastroom_r = ZCMD2.arg3;
        lastmob_r = ZCMD2.arg1;
        break;
      case 'O': /* read an object */
        lastroom_v = world[ZCMD2.arg3].number;
        lastroom_r = ZCMD2.arg3;
        lastobj_r = ZCMD2.arg1;
        break;
      case 'P': /* object to object */
        lastobj_r = ZCMD2.arg1;
        break;
      case 'G': /* obj_to_char */
        lastobj_r = ZCMD2.arg1;
        break;
      case 'E': /* object to equipment list */
        lastobj_r = ZCMD2.arg1;
        break;
      case 'R': /* rem obj from room */
        lastroom_v = 0;
        lastroom_r = 0;
        lastobj_r = 0;
        lastmob_r = 0;
      case 'T': /* trigger to something */
        if (ZCMD2.arg2 != trnum)
          break;
        if (ZCMD2.arg1 == MOB_TRIGGER)
        {
          send_to_char(ch, "mob [%5d] %-60s (zedit room %5d)\r\n",
                       mob_index[lastmob_r].vnum,
                       mob_proto[lastmob_r].player.short_descr,
                       lastroom_v);
          found = 1;
        }
        else if (ZCMD2.arg1 == OBJ_TRIGGER)
        {
          send_to_char(ch, "obj [%5d] %-60s  (zedit room %d)\r\n",
                       obj_index[lastobj_r].vnum,
                       obj_proto[lastobj_r].short_description,
                       lastroom_v);
          found = 1;
        }
        else if (ZCMD2.arg1 == WLD_TRIGGER)
        {
          send_to_char(ch, "room [%5d] %-60s (zedit)\r\n",
                       lastroom_v,
                       world[lastroom_r].name);
          found = 1;
        }
        break;
      } /* switch */
    }   /*for cmd_no......*/
  }     /*for zone...*/

  for (i = 0; i < top_of_mobt; i++)
  {
    if (!mob_proto[i].proto_script)
      continue;

    for (tpl = mob_proto[i].proto_script; tpl; tpl = tpl->next)
      if (tpl->vnum == tvnum)
      {
        send_to_char(ch, "mob [%5d] %s\r\n",
                     mob_index[i].vnum,
                     mob_proto[i].player.short_descr);
        found = 1;
      }
  }

  for (j = 0; j < top_of_objt; j++)
  {
    if (!obj_proto[j].proto_script)
      continue;

    for (tpl = obj_proto[j].proto_script; tpl; tpl = tpl->next)
      if (tpl->vnum == tvnum)
      {
        send_to_char(ch, "obj [%5d] %s\r\n",
                     obj_index[j].vnum,
                     obj_proto[j].short_description);
        found = 1;
      }
  }

  for (k = 0; k < top_of_world; k++)
  {
    if (!world[k].proto_script)
      continue;

    for (tpl = world[k].proto_script; tpl; tpl = tpl->next)
      if (tpl->vnum == tvnum)
      {
        send_to_char(ch, "room[%5d] %s\r\n",
                     world[k].number,
                     world[k].name);
        found = 1;
      }
  }

  if (!found)
    send_to_char(ch, "This trigger is not attached to anything.\r\n");
}

ACMD(do_checkloadstatus)
{
  char buf1[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'};

  two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

  if ((!*buf1) || (!*buf2) || (!isdigit(*buf2)))
  {
    send_to_char(ch, "Checkload <M | O | T> <vnum>\r\n");
    return;
  }

  if (LOWER(*buf1) == 'm')
  {
    mob_checkload(ch, atoi(buf2));
    return;
  }

  if (LOWER(*buf1) == 'o')
  {
    obj_checkload(ch, atoi(buf2));
    return;
  }

  if (LOWER(*buf1) == 't')
  {
    trg_checkload(ch, atoi(buf2));
    return;
  }
}
/* Zone Checker code above. */

/* copyover engine */
void perform_do_copyover()
{
  FILE *fp;
  struct descriptor_data *d, *d_next;
  char buf[100], buf2[100];
  int i;

  fp = fopen(COPYOVER_FILE, "w");
  if (!fp)
  {
    log("Copyover file not writeable, aborted.\n\r");
    return;
  }

  /* write boot_time as first line in file */
  fprintf(fp, "%ld\n", (long)boot_time);

  /* For each playing descriptor, save its state */
  for (d = descriptor_list; d; d = d_next)
  {
    struct char_data *och = d->character;
    /* We delete from the list , so need to save this */
    d_next = d->next;

    /* drop those logging on */
    if (!d->character || d->connected > CON_PLAYING)
    {
      write_to_descriptor(d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r");
      close_socket(d); /* throw'em out */
    }
    else
    {

      write_to_descriptor(d->descriptor, "\n\r *** Time stops for a moment as space and time folds upon itself! ***\n\r");
      write_to_descriptor(d->descriptor, "   *  .  . *       *    .        .        .   *    ..\r\n"
                                         " .    *        .   ###     .      .        .            *\r\n"
                                         "    *.   *        #####   .     *      *        *    .\r\n"
                                         "  ____       *  ######### *    .  *      .        .  *   .\r\n"
                                         " /   /\  .    ###\\#|#//###   ..    *    .      *  .  ..  *\r\n"
                                         "/___/  ^8/     ###\\|//###  *    *            .      *   *\r\n"
                                         "|   ||%%(        # }|{  #\r\n"
                                         "|___|,  ||  ejm    }|{\r\n");
      write_to_descriptor(d->descriptor,
                          "[The game will pause for about 30 seconds while new code is being imported, \r\n"
                          "you will need to reform if you were grouped.  There is no need to disconnect, \r\n"
                          "but if you get disconnected, you should be able to reconnect immediately or \r\n"
                          "within a few minutes.  Your character is being saved!]\r\n");

      /* and handling we need to do */

      save_char_pets(och);

      /* gonna clear some events for player convenience */
      if (char_has_mud_event(och, eMUMMYDUST))
      {
        event_cancel_specific(och, eMUMMYDUST);
      }
      if (char_has_mud_event(och, eDRAGONKNIGHT))
      {
        event_cancel_specific(och, eDRAGONKNIGHT);
      }
      if (char_has_mud_event(och, eGREATERRUIN))
      {
        event_cancel_specific(och, eGREATERRUIN);
      }
      if (char_has_mud_event(och, eHELLBALL))
      {
        event_cancel_specific(och, eHELLBALL);
      }
      if (char_has_mud_event(och, eEPICMAGEARMOR))
      {
        event_cancel_specific(och, eEPICMAGEARMOR);
      }
      if (char_has_mud_event(och, eEPICWARDING))
      {
        event_cancel_specific(och, eEPICWARDING);
      }
      if (char_has_mud_event(och, eC_ANIMAL))
      {
        event_cancel_specific(och, eC_ANIMAL);
      }
      if (char_has_mud_event(och, eC_FAMILIAR))
      {
        event_cancel_specific(och, eC_FAMILIAR);
      }
      if (char_has_mud_event(och, eC_MOUNT))
      {
        event_cancel_specific(och, eC_MOUNT);
      }
      if (char_has_mud_event(och, eSUMMONSHADOW))
      {
        event_cancel_specific(och, eSUMMONSHADOW);
      }

      /* end special handling */

      fprintf(fp, "%d %ld %s %s %s\n", d->descriptor, GET_PREF(och), GET_NAME(och), d->host, CopyoverGet(d));
      /* save och */
      GET_LOADROOM(och) = GET_ROOM_VNUM(IN_ROOM(och));
      Crash_rentsave(och, 0);
      save_char(och, 0);
    }
  } /* end descriptor loop */

  fprintf(fp, "-1\n");
  fclose(fp);

  /* exec - descriptors are inherited */
  snprintf(buf, sizeof(buf), "%d", port);
  snprintf(buf2, sizeof(buf2), "-C%d", mother_desc);

  /* Ugh, seems it is expected we are 1 step above lib - this may be dangerous! */
  i = chdir("..");

  /* Close reserve and other always-open files and release other resources */
  execl(EXE_FILE, "circle", buf2, buf, (char *)NULL);

  /* Failed - successful exec will not return */
  perror("do_copyover: execl");
  log("Copyover FAILED!\n\r");

  exit(1); /* too much trouble to try to recover! */
}

EVENTFUNC(event_copyover)
{
  struct mud_event_data *copyover_event = NULL;
  struct descriptor_data *pt = NULL;
  struct char_data *ch = NULL;
  int timer = 0;
  char buf[50] = {'\0'};

  /* initialize everything and dummy checks */
  if (event_obj == NULL)
    return 0;

  copyover_event = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)copyover_event->pStruct;

  /* in case our event owner decides to log out*/

  /* grab and clear initial timer from sVar */
  if (copyover_event->sVariables)
  {
    timer = atoi((char *)copyover_event->sVariables); /* in seconds */
    free(copyover_event->sVariables);
  }
  else
    timer = 0;

  /* all done, copyover (if we can)! */
  if (timer <= 0)
  {
    if (!ch || !ch->desc || !IS_PLAYING(ch->desc))
    { /*invalid state for copyover, cancel*/
      for (pt = descriptor_list; pt; pt = pt->next)
        if (pt->character)
          send_to_char(pt->character, "\r\n     \tW[Copyover has been CANCELLED]\tn\r\n");
      return 0;
    }
    perform_do_copyover();
    return 0;
  }
  else if (timer == 1)
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        send_to_char(pt->character, "\r\n     \tR[COPYOVER IMMINENT!]\tn\r\n");
    snprintf(buf, sizeof(buf), "%d", (timer - 1));
    copyover_event->sVariables = strdup(buf);
    return (1 * PASSES_PER_SEC);
  }
  else if (timer == 2)
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        send_to_char(pt->character, "\r\n     \tR[Copyover in less than 2 seconds]\tn\r\n");
    snprintf(buf, sizeof(buf), "%d", (timer - 1));
    copyover_event->sVariables = strdup(buf);
    return (1 * PASSES_PER_SEC);
  }
  else if (timer == 3)
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        send_to_char(pt->character, "\r\n     \tR[Copyover in less than 3 seconds]\tn\r\n");
    snprintf(buf, sizeof(buf), "%d", (timer - 1));
    copyover_event->sVariables = strdup(buf);
    return (1 * PASSES_PER_SEC);
  }
  else if (timer <= 10)
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        send_to_char(pt->character, "\r\n     \tR[Copyover in less than 10 seconds]\tn\r\n");
    snprintf(buf, sizeof(buf), "%d", (3));
    copyover_event->sVariables = strdup(buf);
    return ((timer - 3) * PASSES_PER_SEC);
  }
  else if (timer <= 30)
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        send_to_char(pt->character, "\r\n     \tR[Copyover in less than 30 seconds]\tn\r\n");
    snprintf(buf, sizeof(buf), "%d", (10));
    copyover_event->sVariables = strdup(buf);
    return ((timer - 10) * PASSES_PER_SEC);
  }
  else if (timer <= 60)
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        send_to_char(pt->character, "\r\n     \tR[Copyover in less than 1 minute, please disengage from combat and find a safe place to wait]\tn\r\n");
    snprintf(buf, sizeof(buf), "%d", (30));
    copyover_event->sVariables = strdup(buf);
    return ((timer - 30) * PASSES_PER_SEC);
  }
  else if (timer <= 180)
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        send_to_char(pt->character, "\r\n     \tR[Copyover in less than 3 minutes]\tn\r\n");
    snprintf(buf, sizeof(buf), "%d", (60));
    copyover_event->sVariables = strdup(buf);
    return ((timer - 60) * PASSES_PER_SEC);
  }
  else if (timer <= 300)
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        send_to_char(pt->character, "\r\n     \tR[Copyover in less than 5 minutes]\tn\r\n");
    snprintf(buf, sizeof(buf), "%d", (180));
    copyover_event->sVariables = strdup(buf);
    return ((timer - 180) * PASSES_PER_SEC);
  }
  else if (timer <= 600)
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        send_to_char(pt->character, "\r\n     \tR[Copyover in less than 10 minutes]\tn\r\n");
    snprintf(buf, sizeof(buf), "%d", (300));
    copyover_event->sVariables = strdup(buf);
    return ((timer - 300) * PASSES_PER_SEC);
  }
  else
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        send_to_char(pt->character, "\r\n     \tR[Copyover in about %d minutes]\tn\r\n",
                     timer / 60);
    copyover_event->sVariables = strdup("600");
    return ((timer - 600) * PASSES_PER_SEC);
  }
}

/* (c) 1996-97 Erwin S. Andreasen. Modified by Zusuk to accept countdown argument */
ACMD(do_copyover)
{
  int min_level_to_copyover = LVL_GRSTAFF;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int timer = 0;
  char buf[50] = {'\0'};
  struct descriptor_data *pt = NULL;

  if (port == CONFIG_DFLT_DEV_PORT)
  {
    min_level_to_copyover = LVL_IMMORT;
  }

  if (GET_LEVEL(ch) < min_level_to_copyover)
  {
    send_to_char(ch, "You are not high enough level staff to use this command.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    perform_do_copyover();
    return;
  }

  if (is_abbrev(arg, "cancel"))
  {
    send_to_char(ch, "If the copyover event was on you, it has been canceled.  "
                     "Only the initiator of the copyover event can cancel it.\r\n");
    if (char_has_mud_event(ch, eCOPYOVER))
    {
      event_cancel_specific(ch, eCOPYOVER);
      for (pt = descriptor_list; pt; pt = pt->next)
        if (pt->character)
          send_to_char(pt->character, "\r\n     \tW[Copyover has been CANCELLED]\tn\r\n");
    }
    return;
  }

  timer = atoi(arg);

  if (timer <= 0)
  {
    perform_do_copyover();
    return;
  }

  send_to_char(ch, "Event for copyover has started and will complete in %d "
                   "seconds.\r\n"
                   "To cancel type: copyover cancel\r\n",
               timer);

  snprintf(buf, sizeof(buf), "%d", timer); /* sVariable */
  NEW_EVENT(eCOPYOVER, ch, strdup(buf), (1 * PASSES_PER_SEC));
}

/* stop combat in the room you are in */
ACMD(do_peace)
{
  struct char_data *vict, *next_v;

  act("As $n makes a strange arcane gesture, a golden light descends\r\n"
      "from the heavens stopping all the fighting.\r\n",
      FALSE, ch, 0, 0, TO_ROOM);
  send_to_room(IN_ROOM(ch), "Everything is quite peaceful now.\r\n");
  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v)
  {
    next_v = vict->next_in_room;

    if (FIGHTING(vict))
    {
      if (char_has_mud_event(vict, eCOMBAT_ROUND))
      {
        event_cancel_specific(vict, eCOMBAT_ROUND);
      }

      stop_fighting(vict);
      resetCastingData(vict);
    }

    if (IS_NPC(vict))
      clearMemory(vict);
  }
}

ACMD(do_zpurge)
{
  int vroom, room, vzone = 0, zone = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int purge_all = FALSE;
  one_argument(argument, arg, sizeof(arg));
  if (*arg == '.' || !*arg)
  {
    zone = world[IN_ROOM(ch)].zone;
    vzone = zone_table[zone].number;
  }
  else if (is_number(arg))
  {
    vzone = atoi(arg);
    zone = real_zone(vzone);
    if (zone == NOWHERE || zone > top_of_zone_table)
    {
      send_to_char(ch, "That zone doesn't exist!\r\n");
      return;
    }
  }
  else if (*arg == '*')
  {
    purge_all = TRUE;
  }
  else
  {
    send_to_char(ch, "That isn't a valid zone number!\r\n");
    return;
  }
  if (GET_LEVEL(ch) < LVL_STAFF && !can_edit_zone(ch, zone))
  {
    send_to_char(ch, "You can only purge your own zone!\r\n");
    return;
  }
  if (!purge_all)
  {
    for (vroom = zone_table[zone].bot; vroom <= zone_table[zone].top; vroom++)
    {
      purge_room(real_room(vroom));
    }
    send_to_char(ch, "Purged zone #%d: %s.\r\n", zone_table[zone].number, zone_table[zone].name);
    mudlog(NRM, MAX(LVL_GRSTAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s purged zone %d (%s)", GET_NAME(ch), zone_table[zone].number, zone_table[zone].name);
  }
  else
  {
    for (room = 0; room <= top_of_world; room++)
    {
      purge_room(room);
    }
    send_to_char(ch, "Purged world.\r\n");
    mudlog(NRM, MAX(LVL_GRSTAFF, GET_INVIS_LEV(ch)), TRUE, "(GC) %s purged entire world.", GET_NAME(ch));
  }
}

/** Used to read and gather a bit of information about external log files while
 * in game.
 * Makes use of the '\t' color codes in the file status information.
 * Some of the methods used are a bit wasteful (reading through the file
 * multiple times to gather diagnostic information), but it is
 * assumed that the files read with this function will never be very large.
 * Files to be read are assumed to exist and be readable and if they aren't,
 * log the name of the missing file.
 */
ACMD(do_file)
{
  /* Local variables */
  int def_lines_to_read = 15;            /* Set the default num lines to be read. */
  int max_lines_to_read = 300;           /* Maximum number of lines to read. */
  FILE *req_file;                        /* Pointer to file to be read. */
  size_t req_file_size = 0;              /* Size of file to be read. */
  int req_file_lines = 0;                /* Number of total lines in file to be read. */
  int lines_read = 0;                    /* Counts total number of lines read from the file. */
  int req_lines = 0;                     /* Number of lines requested to be displayed. */
  int i, j;                              /* Generic loop counters. */
  int l;                                 /* Marks choice of file in fields array. */
  char field[MAX_INPUT_LENGTH] = {'\0'}; /* Holds users choice of file to be read. */
  char value[MAX_INPUT_LENGTH] = {'\0'}; /* Holds # lines to be read, if requested. */
  char buf[MAX_STRING_LENGTH] = {'\0'};  /* Display buffer for req_file. */

  /* Defines which files are available to read. */
  const struct file_struct
  {
    const char *cmd;    /* The 'name' of the file to view */
    char level;         /* Minimum level needed to view. */
    const char *file;   /* The file location, relative to the working dir. */
    int read_backwards; /* Should the file be read backwards by default? */
  } fields[] = {
      {"xnames", LVL_IMMORT, XNAME_FILE, TRUE},
      {"levels", LVL_IMMORT, LEVELS_LOGFILE, TRUE},
      {"rip", LVL_IMMORT, RIP_LOGFILE, TRUE},
      {"players", LVL_STAFF, NEWPLAYERS_LOGFILE, TRUE},
      {"rentgone", LVL_STAFF, RENTGONE_LOGFILE, TRUE},
      {"errors", LVL_STAFF, ERRORS_LOGFILE, TRUE},
      {"godcmds", LVL_STAFF, STAFFCMDS_LOGFILE, TRUE},
      {"syslog", LVL_STAFF, SYSLOG_LOGFILE, TRUE},
      {"crash", LVL_STAFF, CRASH_LOGFILE, TRUE},
      {"help", LVL_IMMORT, HELP_LOGFILE, TRUE},
      {"changelog", LVL_IMMORT, CHANGE_LOG_FILE, FALSE},
      {"deletes", LVL_STAFF, DELETES_LOGFILE, TRUE},
      {"restarts", LVL_STAFF, RESTARTS_LOGFILE, TRUE},
      {"usage", LVL_IMMORT, USAGE_LOGFILE, TRUE},
      {"badpws", LVL_STAFF, BADPWS_LOGFILE, TRUE},
      {"olc", LVL_STAFF, OLC_LOGFILE, TRUE},
      {"trigger", LVL_STAFF, TRIGGER_LOGFILE, TRUE},
      {"\n", 0, "\n", FALSE} /* This must be the last entry */
  };

  /* Initialize buffer */
  buf[0] = '\0';

  /**/
  /* End function variable set-up and initialization. */

  skip_spaces_c(&argument);

  /* Display usage if no argument. */
  if (!*argument)
  {
    send_to_char(ch, "USAGE: file <filename> <num lines>\r\n\r\nFile options:\r\n");
    for (j = 0, i = 0; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
        send_to_char(ch, "%-15s%s\r\n", fields[i].cmd, fields[i].file);
    return;
  }

  /* Begin validity checks. Is the file choice valid and accessible? */
  /**/
  /* There are some arguments, deal with them. */
  two_arguments(argument, field, sizeof(field), value, sizeof(value));

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
  {
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;
  }

  if (*(fields[l].cmd) == '\n')
  {
    send_to_char(ch, "'%s' is not a valid file.\r\n", field);
    return;
  }

  if (GET_LEVEL(ch) < fields[l].level)
  {
    send_to_char(ch, "You have not achieved a high enough level to view '%s'.\r\n",
                 fields[l].cmd);
    return;
  }

  /* Number of lines to view. Default is 15. */
  if (!*value)
    req_lines = def_lines_to_read;
  else if (!isdigit(*value))
  {
    /* This check forces the requisite positive digit and prevents negative
     * numbers of lines from being read. */
    send_to_char(ch, "'%s' is not a valid number of lines to view.\r\n", value);
    return;
  }
  else
  {
    req_lines = atoi(value);
    /* Limit the maximum number of lines */
    req_lines = MIN(req_lines, max_lines_to_read);
  }

  /* Must be able to access the file on disk. */
  if (!(req_file = fopen(fields[l].file, "r")))
  {
    send_to_char(ch, "The file %s can not be opened.\r\n", fields[l].file);
    mudlog(BRF, LVL_IMPL, TRUE,
           "SYSERR: Error opening file %s using 'file' command.",
           fields[l].file);
    return;
  }
  /**/
  /* End validity checks. From here on, the file should be viewable. */

  /* Diagnostic information about the file */
  req_file_size = file_sizeof(req_file);
  req_file_lines = file_numlines(req_file);

  snprintf(buf, sizeof(buf),
           "\tgFile:\tn %s\tg; Min. Level to read:\tn %d\tg; File Location:\tn %s\tg\r\n"
           "File size (bytes):\tn %ld\tg; Total num lines:\tn %d\r\n",
           fields[l].cmd, fields[l].level, fields[l].file, (long)req_file_size,
           req_file_lines);

  /* Should the file be 'headed' or 'tailed'? */
  if ((fields[l].read_backwards == TRUE) && (req_lines < req_file_lines))
  {
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
             "\tgReading from the tail of the file.\tn\r\n\r\n");
    lines_read = file_tail(req_file, buf, sizeof(buf), req_lines);
  }
  else
  {
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
             "\tgReading from the head of the file.\tn\r\n\r\n");
    lines_read = file_head(req_file, buf, sizeof(buf), req_lines);
  }

  /** Since file_head and file_tail will add the overflow message, we
   * don't check for status here. */
  if (lines_read == req_file_lines)
  {
    /* We're reading the entire file */
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
             "\r\n\tgEntire file returned (\tn%d \tglines).\tn\r\n",
             lines_read);
  }
  else if (lines_read == max_lines_to_read)
  {
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
             "\r\n\tgMaximum number of \tn%d \tglines returned.\tn\r\n",
             lines_read);
  }
  else
  {
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
             "\r\n%d \tglines returned.\tn\r\n",
             lines_read);
  }

  /* Clean up before return */
  fclose(req_file);

  page_string(ch->desc, buf, 1);
}

ACMD(do_changelog)
{
  time_t rawtime;
  char tmstr[MAX_INPUT_LENGTH] = {'\0'}, line[READ_SIZE], last_buf[READ_SIZE],
       buf[READ_SIZE];
  FILE *fl, *new;

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Usage: changelog <change>\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "%s.bak", CHANGE_LOG_FILE);
  if (rename(CHANGE_LOG_FILE, buf))
  {
    mudlog(BRF, LVL_IMPL, TRUE,
           "SYSERR: Error making backup changelog file (%s)", buf);
    return;
  }

  if (!(fl = fopen(buf, "r")))
  {
    mudlog(BRF, LVL_IMPL, TRUE,
           "SYSERR: Error opening backup changelog file (%s)", buf);
    return;
  }

  if (!(new = fopen(CHANGE_LOG_FILE, "w")))
  {
    mudlog(BRF, LVL_IMPL, TRUE,
           "SYSERR: Error opening new changelog file (%s)", CHANGE_LOG_FILE);
    return;
  }

  while (get_line(fl, line))
  {
    if (*line != '[')
      fprintf(new, "%s\n", line);
    else
    {
      strlcpy(last_buf, line, sizeof(last_buf));
      break;
    }
  }

  rawtime = time(0);
  strftime(tmstr, sizeof(tmstr), "%b %d %Y", localtime(&rawtime));

  snprintf(buf, sizeof(buf), "[%s] - %s", tmstr, GET_NAME(ch));

  fprintf(new, "%s\n", buf);
  fprintf(new, "  %s\n", argument);

  if (strcmp(buf, last_buf))
    fprintf(new, "%s\n", line);

  while (get_line(fl, line))
    fprintf(new, "%s\n", line);

  fclose(fl);
  fclose(new);
  send_to_char(ch, "Change added.\r\n");
}

#define PLIST_FORMAT \
  "Usage: plist [minlev[-maxlev]] [-n name] [-d days] [-h hours] [-i] [-m]"

ACMD(do_plist)
{
  int i, len = 0, count = 0;
  char mode, buf[MAX_STRING_LENGTH * 20], name_search[MAX_NAME_LENGTH], time_str[MAX_STRING_LENGTH] = {'\0'};
  struct time_info_data time_away;
  int low = 0, high = LVL_IMPL, low_day = 0, high_day = 10000, low_hr = 0, high_hr = 24;

  skip_spaces_c(&argument);
  strlcpy(buf, argument, sizeof(buf)); /* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  while (*buf)
  {
    char arg[MAX_INPUT_LENGTH] = {'\0'}, buf1[MAX_INPUT_LENGTH] = {'\0'};

    half_chop(buf, arg, buf1);
    if (isdigit(*arg))
    {
      if (sscanf(arg, "%d-%d", &low, &high) == 1)
        high = low;
      strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
    }
    else if (*arg == '-')
    {
      mode = *(arg + 1); /* just in case; we destroy arg in the switch */
      switch (mode)
      {
      case 'l':
        half_chop(buf1, arg, buf);
        sscanf(arg, "%d-%d", &low, &high);
        break;
      case 'n':
        half_chop(buf1, name_search, buf);
        break;
      case 'i':
        strlcpy(buf, buf1, sizeof(buf));
        low = LVL_IMMORT;
        break;
      case 'm':
        strlcpy(buf, buf1, sizeof(buf));
        high = LVL_IMMORT - 1;
        break;
      case 'd':
        half_chop(buf1, arg, buf);
        if (sscanf(arg, "%d-%d", &low_day, &high_day) == 1)
          high_day = low_day;
        break;
      case 'h':
        half_chop(buf1, arg, buf);
        if (sscanf(arg, "%d-%d", &low_hr, &high_hr) == 1)
          high_hr = low_hr;
        break;
      default:
        send_to_char(ch, "%s\r\n", PLIST_FORMAT);
        return;
      }
    }
    else
    {
      send_to_char(ch, "%s\r\n", PLIST_FORMAT);
      return;
    }
  }

  len = 0;
  len += snprintf(buf + len, sizeof(buf) - len, "\tW[ Id] (Lv) Name         Last\tn\r\n"
                                                "%s-------------------------------------%s\r\n",
                  CCCYN(ch, C_NRM),
                  CCNRM(ch, C_NRM));

  for (i = 0; i <= top_of_p_table; i++)
  {
    if (player_table[i].level < low || player_table[i].level > high)
      continue;

    time_away = *real_time_passed(time(0), player_table[i].last);

    if (*name_search && str_cmp(name_search, player_table[i].name))
      continue;

    if (time_away.day > high_day || time_away.day < low_day)
      continue;
    if (time_away.hours > high_hr || time_away.hours < low_hr)
      continue;

    strlcpy(time_str, asctime(localtime(&player_table[i].last)), sizeof(time_str));
    time_str[strlen(time_str) - 1] = '\0';

    len += snprintf(buf + len, sizeof(buf) - len, "[%3ld] (%2d) %c%-15s %s\r\n",
                    player_table[i].id, player_table[i].level,
                    UPPER(*player_table[i].name), player_table[i].name + 1, time_str);
    count++;
  }
  snprintf(buf + len, sizeof(buf) - len, "%s-------------------------------------%s\r\n"
                                         "%d players listed.\r\n",
           CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), count);
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_wizupdate)
{
  run_autowiz();
  send_to_char(ch, "Wizlists updated.\n\r");
}

/* NOTE: This is called from perform_set */
bool change_player_name(struct char_data *ch, struct char_data *vict, char *new_name)
{
  struct char_data *temp_ch = NULL;
  int plr_i = 0, i, j, k;
  char old_name[MAX_NAME_LENGTH], old_pfile[50], new_pfile[50], buf[MAX_STRING_LENGTH] = {'\0'};

  if (!ch)
  {
    log("SYSERR: No char passed to change_player_name.");
    return FALSE;
  }

  if (!vict)
  {
    log("SYSERR: No victim passed to change_player_name.");
    send_to_char(ch, "Invalid victim.\r\n");
    return FALSE;
  }

  if (!new_name || !(*new_name) || strlen(new_name) < 2 ||
      strlen(new_name) > MAX_NAME_LENGTH || !valid_name(new_name) ||
      fill_word(new_name) || reserved_word(new_name))
  {
    send_to_char(ch, "Invalid new name.\r\n");
    return FALSE;
  }

  // Check that someone with new_name isn't already logged in
  if ((temp_ch = get_player_vis(ch, new_name, NULL, FIND_CHAR_WORLD)) != NULL)
  {
    send_to_char(ch, "Sorry, the new name already exists.\r\n");
    return FALSE;
  }
  else
  {
    /* try to load the player off disk */
    CREATE(temp_ch, struct char_data, 1);
    clear_char(temp_ch);
    CREATE(temp_ch->player_specials, struct player_special_data, 1);
    new_mobile_data(temp_ch);
    /* Allocate mobile event list */
    // temp_ch->events = create_list();
    if ((plr_i = load_char(new_name, temp_ch)) > -1)
    {
      free_char(temp_ch);
      send_to_char(ch, "Sorry, the new name already exists.\r\n");
      return FALSE;
    }
  }

  /* New playername is OK - find the entry in the index */
  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == GET_IDNUM(vict))
      break;

  if (player_table[i].id != GET_IDNUM(vict))
  {
    send_to_char(ch, "Your target was not found in the player index.\r\n");
    log("SYSERR: Player %s, with ID %ld, could not be found in the player index.", GET_NAME(vict), GET_IDNUM(vict));
    return FALSE;
  }

  /* Set up a few variables that will be needed */
  snprintf(old_name, sizeof(old_name), "%s", GET_NAME(vict));
  if (!get_filename(old_pfile, sizeof(old_pfile), PLR_FILE, old_name))
  {
    send_to_char(ch, "Unable to ascertain player's old pfile name.\r\n");
    return FALSE;
  }
  if (!get_filename(new_pfile, sizeof(new_pfile), PLR_FILE, new_name))
  {
    send_to_char(ch, "Unable to ascertain player's new pfile name.\r\n");
    return FALSE;
  }

  /* Now start changing the name over - all checks and setup have passed */
  free(player_table[i].name);              // Free the old name in the index
  player_table[i].name = strdup(new_name); // Insert the new name into the index
  for (k = 0; (*(player_table[i].name + k) = LOWER(*(player_table[i].name + k))); k++)
    ;

  free(GET_PC_NAME(vict));
  GET_PC_NAME(vict) = strdup(CAP(new_name)); // Change the name in the victims char struct

  /* Rename the player's pfile */
  snprintf(buf, sizeof(buf), "mv %s %s", old_pfile, new_pfile);
  j = system(buf);

  /* Save the changed player index - the pfile is saved by perform_set */
  save_player_index();

  mudlog(BRF, LVL_IMMORT, TRUE, "(GC) %s changed the name of %s to %s", GET_NAME(ch), old_name, new_name);

  if (vict->desc) /* Descriptor is set if the victim is logged in */
    send_to_char(vict, "Your login name has changed from %s%s%s to %s%s%s.\r\n", CCYEL(vict, C_NRM), old_name, CCNRM(vict, C_NRM),
                 CCYEL(vict, C_NRM), new_name, CCNRM(vict, C_NRM));

  return TRUE;
}

ACMD(do_zlock)
{
  zone_vnum znvnum;
  zone_rnum zn;
  char arg[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};
  int counter = 0;
  bool fail = FALSE;

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!*arg)
  {
    send_to_char(ch, "Usage: %szlock <zone number>%s\r\n", QYEL, QNRM);
    send_to_char(ch, "%s       zlock list%s\r\n\r\n", QYEL, QNRM);
    send_to_char(ch, "Locks a zone so that building or editing is not possible.\r\n");
    send_to_char(ch, "The 'list' shows all currently locked zones.\r\n");
    send_to_char(ch, "'zlock all' will lock every zone with the GRID flag set.\r\n");
    send_to_char(ch, "'zlock all all' will lock every zone in the MUD.\r\n");
    return;
  }
  if (is_abbrev(arg, "all"))
  {
    if (GET_LEVEL(ch) < LVL_GRSTAFF)
    {
      send_to_char(ch, "You do not have sufficient access to lock all zones.\r\n");
      return;
    }
    if (!*arg2)
    {
      for (zn = 0; zn <= top_of_zone_table; zn++)
      {
        if (!ZONE_FLAGGED(zn, ZONE_NOBUILD) && ZONE_FLAGGED(zn, ZONE_GRID))
        {
          counter++;
          SET_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
          if (save_zone(zn))
          {
            log("(GC) %s has locked zone %d", GET_NAME(ch), zone_table[zn].number);
          }
          else
          {
            fail = TRUE;
          }
        }
      }
    }
    else if (is_abbrev(arg2, "all"))
    {
      for (zn = 0; zn <= top_of_zone_table; zn++)
      {
        if (!ZONE_FLAGGED(zn, ZONE_NOBUILD))
        {
          counter++;
          SET_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
          if (save_zone(zn))
          {
            log("(GC) %s has locked zone %d", GET_NAME(ch), zone_table[zn].number);
          }
          else
          {
            fail = TRUE;
          }
        }
      }
    }
    if (counter == 0)
    {
      send_to_char(ch, "There are no unlocked zones to lock!\r\n");
      return;
    }
    if (fail)
    {
      send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
      return;
    }
    send_to_char(ch, "%d zones have now been locked.\r\n", counter);
    mudlog(BRF, LVL_STAFF, TRUE, "(GC) %s has locked ALL zones!", GET_NAME(ch));
    return;
  }
  if (is_abbrev(arg, "list"))
  {
    /* Show all locked zones */
    for (zn = 0; zn <= top_of_zone_table; zn++)
    {
      if (ZONE_FLAGGED(zn, ZONE_NOBUILD))
      {
        if (!counter)
          send_to_char(ch, "Locked Zones\r\n");

        send_to_char(ch, "[%s%3d%s] %s%-*s %s%-1s%s\r\n",
                     QGRN, zone_table[zn].number, QNRM, QCYN, count_color_chars(zone_table[zn].name) + 30, zone_table[zn].name,
                     QYEL, zone_table[zn].builders ? zone_table[zn].builders : "None.", QNRM);
        counter++;
      }
    }
    if (counter == 0)
    {
      send_to_char(ch, "There are currently no locked zones!\r\n");
    }
    return;
  }
  else if ((znvnum = atoi(arg)) == 0)
  {
    send_to_char(ch, "Usage: %szlock <zone number>%s\r\n", QYEL, QNRM);
    return;
  }

  if ((zn = real_zone(znvnum)) == NOWHERE)
  {
    send_to_char(ch, "That zone does not exist!\r\n");
    return;
  }

  /* Check the builder list */
  if (GET_LEVEL(ch) < LVL_GRSTAFF && !is_name(GET_NAME(ch), zone_table[zn].builders) && GET_OLC_ZONE(ch) != znvnum)
  {
    send_to_char(ch, "You do not have sufficient access to lock that zone!\r\n");
    return;
  }

  /* If we get here, player has typed 'zlock <num>' */
  if (ZONE_FLAGGED(zn, ZONE_NOBUILD))
  {
    send_to_char(ch, "Zone %d is already locked!\r\n", znvnum);
    return;
  }
  SET_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
  if (save_zone(zn))
  {
    mudlog(NRM, LVL_GRSTAFF, TRUE, "(GC) %s has locked zone %d", GET_NAME(ch), znvnum);
  }
  else
  {
    send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
  }
}

ACMD(do_zunlock)
{
  zone_vnum znvnum;
  zone_rnum zn;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int counter = 0;
  bool fail = FALSE;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Usage: %szunlock <zone number>%s\r\n", QYEL, QNRM);
    send_to_char(ch, "%s       zunlock list%s\r\n\r\n", QYEL, QNRM);
    send_to_char(ch, "Unlocks a 'locked' zone to allow building or editing.\r\n");
    send_to_char(ch, "The 'list' shows all currently unlocked zones.\r\n");
    send_to_char(ch, "'zunlock all' will unlock every zone in the MUD.\r\n");
    return;
  }
  if (is_abbrev(arg, "all"))
  {
    if (GET_LEVEL(ch) < LVL_GRSTAFF)
    {
      send_to_char(ch, "You do not have sufficient access to lock zones.\r\n");
      return;
    }
    for (zn = 0; zn <= top_of_zone_table; zn++)
    {
      if (ZONE_FLAGGED(zn, ZONE_NOBUILD))
      {
        counter++;
        REMOVE_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
        if (save_zone(zn))
        {
          log("(GC) %s has unlocked zone %d", GET_NAME(ch), zone_table[zn].number);
        }
        else
        {
          fail = TRUE;
        }
      }
    }
    if (counter == 0)
    {
      send_to_char(ch, "There are no locked zones to unlock!\r\n");
      return;
    }
    if (fail)
    {
      send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
      return;
    }
    send_to_char(ch, "%d zones have now been unlocked.\r\n", counter);
    mudlog(BRF, LVL_STAFF, TRUE, "(GC) %s has unlocked ALL zones!", GET_NAME(ch));
    return;
  }
  if (is_abbrev(arg, "list"))
  {
    /* Show all unlocked zones */
    for (zn = 0; zn <= top_of_zone_table; zn++)
    {
      if (!ZONE_FLAGGED(zn, ZONE_NOBUILD))
      {
        if (!counter)
          send_to_char(ch, "Unlocked Zones\r\n");

        send_to_char(ch, "[%s%3d%s] %s%-*s %s%-1s%s\r\n",
                     QGRN, zone_table[zn].number, QNRM, QCYN, count_color_chars(zone_table[zn].name) + 30, zone_table[zn].name,
                     QYEL, zone_table[zn].builders ? zone_table[zn].builders : "None.", QNRM);
        counter++;
      }
    }
    if (counter == 0)
    {
      send_to_char(ch, "There are currently no unlocked zones!\r\n");
    }
    return;
  }
  else if ((znvnum = atoi(arg)) == 0)
  {
    send_to_char(ch, "Usage: %szunlock <zone number>%s\r\n", QYEL, QNRM);
    return;
  }

  if ((zn = real_zone(znvnum)) == NOWHERE)
  {
    send_to_char(ch, "That zone does not exist!\r\n");
    return;
  }

  /* Check the builder list */
  if (GET_LEVEL(ch) < LVL_GRSTAFF && !is_name(GET_NAME(ch), zone_table[zn].builders) && GET_OLC_ZONE(ch) != znvnum)
  {
    send_to_char(ch, "You do not have sufficient access to unlock that zone!\r\n");
    return;
  }

  /* If we get here, player has typed 'zunlock <num>' */
  if (!ZONE_FLAGGED(zn, ZONE_NOBUILD))
  {
    send_to_char(ch, "Zone %d is already unlocked!\r\n", znvnum);
    return;
  }
  REMOVE_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
  if (save_zone(zn))
  {
    mudlog(NRM, LVL_GRSTAFF, TRUE, "(GC) %s has unlocked zone %d", GET_NAME(ch), znvnum);
  }
  else
  {
    send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
  }
}

/* get highest vnum in recent player list  */
static int get_max_recent(void)
{
  struct recent_player *this;
  int iRet = 0;

  this = recent_list;

  while (this)
  {
    if (this->vnum > iRet)
      iRet = this->vnum;
    this = this->next;
  }

  return iRet;
}

/* clear an item in recent player list */
static void clear_recent(struct recent_player *this)
{
  this->vnum = 0;
  this->time = 0;
  strlcpy(this->name, "", sizeof(this->name));
  strlcpy(this->host, "", sizeof(this->host));
  this->next = NULL;
}

/* create new blank player in recent players list */
static struct recent_player *create_recent(void)
{
  struct recent_player *newrecent;

  CREATE(newrecent, struct recent_player, 1);
  clear_recent(newrecent);
  newrecent->next = recent_list;
  recent_list = newrecent;

  newrecent->vnum = get_max_recent();
  newrecent->vnum++;
  return newrecent;
}

/* Add player to recent player list */
bool AddRecentPlayer(char *chname, char *chhost, bool newplr, bool cpyplr)
{
  struct recent_player *this;
  time_t ct;
  int max_vnum;

  ct = time(0); /* Grab the current time */

  this = create_recent();

  if (!this)
    return FALSE;

  this->time = ct;
  this->new_player = newplr;
  this->copyover_player = cpyplr;
  strcpy(this->host, chhost);
  if (chname)
    strcpy(this->name, chname);
  max_vnum = get_max_recent();
  this->vnum = max_vnum; /* Possibly should be +1 ? */

  return TRUE;
}

void free_recent_players(void)
{
  struct recent_player *this;
  struct recent_player *temp;

  this = recent_list;

  while ((temp = this) != NULL)
  {
    this = this->next;
    free(temp);
  }
}

ACMD(do_recent)
{
  time_t ct;
  char *tmstr, arg[MAX_INPUT_LENGTH] = {'\0'};
  int hits = 0, limit = 0, count = 0;
  struct recent_player *this;
  bool loc;

  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    limit = 0;
  }
  else
  {
    limit = atoi(arg);
  }

  if (GET_LEVEL(ch) >= LVL_GRSTAFF)
  { /* If High-Level Imm, then show Host IP */
    send_to_char(ch, " ID | DATE/TIME           | HOST IP                               | Player Name\r\n");
  }
  else
  {
    send_to_char(ch, " ID | DATE/TIME           | Player Name\r\n");
  }

  this = recent_list;
  while (this)
  {
    loc = FALSE;
    hits++;
    ct = this->time;
    tmstr = asctime(localtime(&ct));
    *(tmstr + strlen(tmstr) - 1) = '\0'; /* Cut off last char */
    if (this->host && *(this->host))
    {
      if (!strcmp(this->host, "localhost"))
        loc = TRUE;
    }

    if ((limit == 0) || (count < limit))
    {
      if (GET_LEVEL(ch) >= LVL_GRSTAFF) /* If High-Level Imm, then show Host IP */
      {
        if (this->new_player == TRUE)
        {
          send_to_char(ch, "%3d | %-19.19s | %s%-37s%s | %s %s(New Player)%s\r\n", this->vnum, tmstr, loc ? QRED : "", this->host, QNRM, this->name, QYEL, QNRM);
        }
        else if (this->copyover_player == TRUE)
        {
          send_to_char(ch, "%3d | %-19.19s | %s%-37s%s | %s %s(Copyover)%s\r\n", this->vnum, tmstr, loc ? QRED : "", this->host, QNRM, this->name, QCYN, QNRM);
        }
        else
        {
          send_to_char(ch, "%3d | %-19.19s | %s%-37s%s | %s\r\n", this->vnum, tmstr, loc ? QRED : "", this->host, QNRM, this->name);
        }
      }
      else
      {
        if (this->new_player == TRUE)
        {
          send_to_char(ch, "%3d | %-19.19s | %s %s(New Player)%s\r\n", this->vnum, tmstr, this->name, QYEL, QNRM);
        }
        else if (this->copyover_player == TRUE)
        {
          send_to_char(ch, "%3d | %-19.19s | %s %s(Copyover)%s\r\n", this->vnum, tmstr, this->name, QCYN, QNRM);
        }
        else
        {
          send_to_char(ch, "%3d | %-19.19s | %s\r\n", this->vnum, tmstr, this->name);
        }
      }
      count++;

      this = this->next;
    }
    else
    {
      this = NULL;
    }
  }

  ct = time(0); /* Grab the current time */
  tmstr = asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  send_to_char(ch, "Current Server Time: %-19.19s\r\nShowing %d players since last copyover/reboot\r\n", tmstr, hits);
}

ACMD(do_oset)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  const char usage[] = "Usage: \r\n"
                       "Options: alias, apply, longdesc, shortdesc\r\n"
                       "> oset <object> <option> <value>\r\n";
  struct obj_data *obj;
  bool success = TRUE;

  if (IS_NPC(ch) || ch->desc == NULL)
  {
    send_to_char(ch, "oset is only usable by connected players.\r\n");
    return;
  }

  argument = one_argument(argument, arg, sizeof(arg));

  if (!*arg)
    send_to_char(ch, usage);
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)) &&
           !(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else
  {
    argument = one_argument(argument, arg2, sizeof(arg2));

    if (!*arg2)
      send_to_char(ch, usage);
    else
    {
      if (is_abbrev(arg2, "alias") && (success = oset_alias(obj, argument)))
        send_to_char(ch, "Object alias set.\r\n");
      else if (is_abbrev(arg2, "longdesc") && (success = oset_long_description(obj, argument)))
        send_to_char(ch, "Object long description set.\r\n");
      else if (is_abbrev(arg2, "shortdesc") && (success = oset_short_description(obj, argument)))
        send_to_char(ch, "Object short description set.\r\n");
      else if (is_abbrev(arg2, "apply") && (success = oset_apply(obj, argument)))
        send_to_char(ch, "Object apply set.\r\n");
      else
      {
        if (!success)
          send_to_char(ch, "%s was unsuccessful.\r\n", arg2);
        else
          send_to_char(ch, usage);
        return;
      }
    }
  }
}

/* this is deprecated by olist */
ACMD(do_objlist)
{
  bool quest = FALSE;

  int i, j, k, l, m;
  char value[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj;
  char buf[8192];
  char buf2[8192];
  char buf3[8192];
  char buf4[8192];
  char buf5[8192];
  char tmp_buf[1024];
  one_argument(argument, value, sizeof(value));

  if (*value && is_number(value))
    j = atoi(value);
  else
    j = zone_table[world[ch->in_room].zone].number;
  // j *= 100;
  if (real_zone(j) == NOWHERE)
  {
    snprintf(buf, sizeof(buf), "\tR%d \tris not in a defined zone.\tn\r\n", j);
    send_to_char(ch, buf);
    return;
  }
  k = real_zone(j);
  k = zone_table[k].top;
  snprintf(buf, sizeof(buf), "Detailed Object list : From %d to %d\r\n", j, k);
  for (i = j; i <= k; i++)
  {
    if ((l = real_object(i)) != NOWHERE)
    {
      obj = &obj_proto[l];

      quest = is_object_in_a_quest(obj);

      snprintf(tmp_buf, sizeof(tmp_buf), "[%5d] %s   %s\r\n", i,
               obj->short_description, (quest ? "(\tcUsed in Quests\tn)" : ""));
      strlcat(buf, tmp_buf, sizeof(buf));
      switch (GET_OBJ_TYPE(obj))
      {
      case ITEM_WEAPON:
        snprintf(tmp_buf, sizeof(tmp_buf), "      \tcWeapon\tn: %dd%d ", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
        strlcat(buf, tmp_buf, sizeof(buf));
        break;
      case ITEM_ARMOR:
        snprintf(tmp_buf, sizeof(tmp_buf), "      \tcAC\tn: %d ", GET_OBJ_VAL(obj, 0));
        strlcat(buf, tmp_buf, sizeof(buf));
        break;
      default:
        snprintf(tmp_buf, sizeof(tmp_buf), "      \tcValue\tn: %d/%d/%d/%d ",
                 GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
                 GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3));
        strlcat(buf, tmp_buf, sizeof(buf));
        break;
      }

      for (m = 0; m < MAX_OBJ_AFFECT; m++)
        if (obj->affected[m].modifier)
        {
          sprinttype(obj->affected[m].location, apply_types, buf2, sizeof(buf2));
          snprintf(tmp_buf, sizeof(tmp_buf), "\tc%s\tn%s%d ", buf2, (obj->affected[m].modifier > 0 ? "+" : ""),
                   obj->affected[m].modifier);
          strlcat(buf, tmp_buf, sizeof(buf));
        }
      strlcat(buf, "\r\n", sizeof(buf));

      sprintbit((long)obj->obj_flags.wear_flags, wear_bits, buf2, sizeof(buf2));

      sprintbit((long)obj->obj_flags.bitvector, affected_bits, buf3, sizeof(buf3));
      if (strcmp(buf3, "NOBITS ") == 0)
        buf3[0] = 0;
      if (strcmp(buf4, "NOBITS ") == 0)
        buf4[0] = 0;
      if (strcmp(buf5, "NOBITS ") == 0)
        buf5[0] = 0;
      if (buf3[0] == 0 && buf4[0] == 0 && buf5[0] == 0)
        strlcpy(buf3, "NOBITS ", sizeof(buf3));

      snprintf(tmp_buf, sizeof(tmp_buf), "      \tcWorn\tn: %s \tcAffects:\tn %s %s %s\r\n",
               buf2, buf3, buf4, buf5);
      strlcat(buf, tmp_buf, sizeof(buf));
    }
  }
  page_string(ch->desc, buf, 1);
}

/************************************************************************
 * do_hlqlist       by Kelemvor, rewritten for Luminari by Zusuk        *
 *    This function allows immortals to view homeland quest info        *
 ***********************************************************************/
ACMD(do_hlqlist)
{
  struct quest_entry *quest;
  mob_vnum bottom = NOBODY, top = NOBODY;
  mob_rnum realnum = 0;
  int temp_num = 0, num_found = 0;
  int i = 0, len = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf1[MAX_INPUT_LENGTH] = {'\0'};
  char buf2[MAX_INPUT_LENGTH] = {'\0'};

  /* parse any arguments */
  two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

  /* if no buf1, use current zone information */
  if (!*buf1)
  {
    bottom = zone_table[world[IN_ROOM(ch)].zone].bot;
    send_to_char(ch, "Bottom:  %d\r\n", bottom);
  } /* if buf1 is not a number send them back */
  else if (!isdigit(*buf1))
  {
    send_to_char(ch, "\tcFirst value must be a digit, or nothing.\tn\r\n");
    return;

    /* convert buf1 to an integer */
  }
  else
    bottom = atoi(buf1);

  /* if no buf2, use buf1, and top of zone information */
  if (!*buf2)
  {
    top = zone_table[world[IN_ROOM(ch)].zone].top;
    send_to_char(ch, "Top:  %d\r\n", top);
  } /* if buf2 is not a number send them back */
  else if (!isdigit(*buf2))
  {
    send_to_char(ch, "\tcSecond value must be a digit, or nothing.\tn\r\n");
    return;

  } /* convert buf2 to an integer */
  else
  {
    top = atoi(buf2);
    if (bottom > top)
    {
      send_to_char(ch, "\tcFirst number must be less than second.\tn\r\n");
      return;
    }
  }

  if (bottom >= NOWHERE || top >= NOWHERE)
  {
    send_to_char(ch, "Invalid values!\r\n");
    return;
  }

  if (top - bottom >= 999)
  {
    send_to_char(ch, "Too many at once, 999 limits.\r\n");
    return;
  }

  /* start engine */
  send_to_char(ch, "Quest Listings : From %d to %d\r\n", bottom, top);
  for (i = bottom; i <= top; i++)
  {
    if ((realnum = real_mobile(i)) != NOBODY)
    {
      if (mob_proto[realnum].mob_specials.quest)
      {
        temp_num = 0;
        num_found = 0;
        for (quest = mob_proto[realnum].mob_specials.quest; quest;
             quest = quest->next)
        {
          num_found++;
          if (quest->approved)
            temp_num++;
        }

        /* DEBUG */
        /*
        send_to_char(ch, "[%5d] %-40s %d/%d\r\n", i,
                mob_proto[realnum].player.short_descr, temp_num, num_found);
         */

        len += snprintf(buf + len, sizeof(buf) - len,
                        "[%5d] %-40s %d/%d\r\n", i,
                        mob_proto[realnum].player.short_descr, temp_num, num_found);

        /* Large buf can't hold that much memory so cut off list */
        if (len > sizeof(buf))
          break;
      }
      else
      {
        /* debug */
        // send_to_char(ch, "NO QUEST\r\n");
      }
      /* end has-quest check */
    }
    else
    {
      /* debug */
      // send_to_char(ch, "NOBODY\r\n");
    }
    /* end NOBODY check */
  }
  /* end of qlist */

  /* now send it all to the pager */
  page_string(ch->desc, buf, TRUE);
}

/* a simple function/command to check location of all SINGLEFILE rooms
 in-game */
ACMD(do_singlefile)
{
  room_rnum room = NOWHERE;
  int dirs = -1, num_exits = -1;
  char exits[24] = "NONE", buf[MAX_INPUT_LENGTH] = {'\0'};

  for (room = 0; room < top_of_world; room++)
  {
    if (ROOM_FLAGGED(room, ROOM_SINGLEFILE))
    {
      num_exits = 0;
      for (dirs = 0; dirs < NUM_OF_DIRS; dirs++)
        if (world[room].dir_option[dirs])
          num_exits++;

      snprintf(exits, sizeof(exits), "%d   ", num_exits);
      snprintf(buf, sizeof(buf), "[%5d] %-*s \tgExits: \tc%4s %s\tn\r\n", world[room].number,
               50 + color_count(world[room].name),
               world[room].name,
               num_exits == 0 ? "NONE" : exits, num_exits != 2 ? "\tRERROR!\tn" : "");

      send_to_char(ch, buf);
    }
  }
}

#include "wilderness.h"
#include "kdtree.h"
#include "mysql.h"

/* Command to generate a wilderness river. */
ACMD(do_genriver)
{
  char arg1[MAX_STRING_LENGTH] = {'\0'};
  char arg2[MAX_STRING_LENGTH] = {'\0'};
  const char *name = NULL;
  int dir = 0;
  region_vnum vnum;

  /* genreiver north 100011 FooBar River
   * genmap <arg1> <arg2> <name string> */

  name = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "1st argument requires direction: genriver \tRnorth\tn 4 FooBar River, "
                     "cardinal directions or equivalent values accepted.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "2nd argument requires VNum: genriver n \tR4\tn FooBar River, "
                     "unique vnums only, pathlist to view.\r\n");
    return;
  }

  skip_spaces_c(&name);

  if (!*name)
  {
    send_to_char(ch, "3rd argument requires river name: genriver n 4 \tRFooBar River\tn\r\n");
    return;
  }

  if (is_abbrev(arg1, "north"))
    dir = NORTH;
  else if (is_abbrev(arg1, "east"))
    dir = EAST;
  else if (is_abbrev(arg1, "south"))
    dir = SOUTH;
  else if (is_abbrev(arg1, "west"))
    dir = WEST;
  else
    dir = atoi(arg1);

  if (dir < NORTH || dir >= NUM_OF_DIRS)
  {
    send_to_char(ch, "Invalid direction.\r\n");
    return;
  }

  vnum = atoi(arg2);

  /* tested upper limited -zusuk */
  if (vnum < 0 || vnum > 1215752191)
  {
    send_to_char(ch, "Invalid VNum.\r\n");
    return;
  }

  generate_river(ch, dir, vnum, name);
  load_paths();

  send_to_char(ch, "River created!\r\n");
}

ACMD(do_deletepath)
{
  char arg1[MAX_STRING_LENGTH] = {'\0'};
  region_vnum vnum;

  one_argument(argument, arg1, sizeof(arg1));

  if (!*arg1)
  {
    send_to_char(ch, "\tnUsage: deletepath VNum\r\nWhere VNum is the VNum for the path you wish to delete. (see pathlist command.)\r\n");
    return;
  }

  vnum = atoi(arg1);

  if (vnum < 0 || vnum > 1215752191)
  {
    send_to_char(ch, "\tnInvalid VNum.\r\n");
    return;
  }

  if (delete_path(vnum))
  {
    send_to_char(ch, "\tnPath deleted!\r\n");
  }
  else
  {
    send_to_char(ch, "\tnPath not found.\r\n");
  }

  load_paths();
}

/* Test command to display a map, radius 4, generated using noise. */
ACMD(do_genmap)
{
  char arg1[MAX_STRING_LENGTH] = {'\0'};
  char arg2[MAX_STRING_LENGTH] = {'\0'};
  const char *name = NULL;
  int dir = 0;
  region_vnum vnum;

  /* genmap north 100011 FooBar River
   * genmap <arg1> <arg2> <name string> */

  name = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "1st argument requires direction: genmap \tRnorth\tn 4 FooBar River, "
                     "cardinal directions or equivalent values accepted.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "2nd argument requires VNum: genmap n \tR4\tn FooBar River, "
                     "unique vnums only, pathlist to view.\r\n");
    return;
  }

  skip_spaces_c(&name);

  if (!*name)
  {
    send_to_char(ch, "3rd argument requires river name: genmap n 4 \tRFooBar River\tn\r\n");
    return;
  }

  if (is_abbrev(arg1, "north"))
    dir = NORTH;
  else if (is_abbrev(arg1, "east"))
    dir = EAST;
  else if (is_abbrev(arg1, "south"))
    dir = SOUTH;
  else if (is_abbrev(arg1, "west"))
    dir = WEST;
  else
    dir = atoi(arg1);

  if (dir < NORTH || dir >= NUM_OF_DIRS)
  {
    send_to_char(ch, "Invalid direction.\r\n");
    return;
  }

  vnum = atoi(arg2);

  /* tested upper limited -zusuk */
  if (vnum < 0 || vnum > 1215752191)
  {
    send_to_char(ch, "Invalid VNum.\r\n");
    return;
  }

  /*debug*/ send_to_char(ch, "Debug- dir: %d, vnum: %d, name: %s\r\n",
                         dir, vnum, name);

  generate_river(ch, dir, vnum, name);
  load_paths();
}

/* do_acconvert - Commant to convert exising armor to the new (Sept 9, 2014)
 * AC system.  ONLY USE THIS ONE TIME! */
ACMD(do_acconvert)
{
  int num, found = 0, total = 0;

  for (num = 0; num <= top_of_objt; num++)
  {

    if (GET_OBJ_TYPE(&obj_proto[num]) != ITEM_ARMOR)
      continue;

    GET_OBJ_VAL(&obj_proto[num], 0) = (GET_OBJ_VAL(&obj_proto[num], 0) / 10) + (GET_OBJ_VAL(&obj_proto[num], 0) % 10 != 0);
    found++;
  }
  total += found;
  send_to_char(ch, "%d converted.\r\n", found);
}

/* do_oconvert - Command to convert existing objects to the new (Jan 13, 2014)
 * weapon/armor type and feat system.  This command should be executed once, or can be
 * executed mltiple times to clean up bad building. Uses a very simple system
 * to identify weapons that match certain weapon types - This will require help
 * via manual touching up/conversions over the course of months or years...
 *
 * Syntax :
 *
 *   oconvert armor|weapon <armortype|weapontype> <keyword>
 *
 * */
ACMD(do_oconvert)
{
  // struct object_data *obj = NULL;
  int i = 0, j = 0;
  int hitroll = 0, damroll = 0;
  int num, found = 0, total = 0;
  // obj_vnum ov;
  // char buf[MAX_STRING_LENGTH] = {'\0'};

  char arg[MAX_STRING_LENGTH] = {'\0'}, arg2[MAX_STRING_LENGTH] = {'\0'};
  int iarg;

  if (argument == NULL)
  {
    send_to_char(ch, "Usage: oconvert <weapon_type> <keyword>\r\n");
    return;
  }

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));
  iarg = atoi(arg);

  send_to_char(ch, "%d %s\r\n", iarg, arg2);

  const char *weapon_type_keywords[NUM_WEAPON_TYPES];

  /* Initialize the weapon keyword array. */
  for (i = 0; i < NUM_WEAPON_TYPES; i++)
    weapon_type_keywords[i] = NULL;

  weapon_type_keywords[WEAPON_TYPE_DAGGER] = "dagger";

  //  for (i = 0; i < NUM_WEAPON_TYPES; i++) {
  /* Skip weapon types for which we have no keywords. */
  //    if (weapon_type_keywords[i] == NULL)
  //      continue;

  found = 0;
  send_to_char(ch, "- Converting type %d : %s ... ", iarg, weapon_list[iarg].name);

  i = iarg;

  for (num = 0; num <= top_of_objt; num++)
  {
    /* init */
    hitroll = 0;
    damroll = 0;

    if (!(GET_OBJ_TYPE(&(obj_proto[num])) == ITEM_WEAPON))
      continue;

    if (is_name(arg2, obj_proto[num].name))
    {

      GET_OBJ_VAL(&obj_proto[num], 0) = i;                       /* Weapon type */
      GET_OBJ_VAL(&obj_proto[num], 1) = weapon_list[i].numDice;  /* Number of dice */
      GET_OBJ_VAL(&obj_proto[num], 2) = weapon_list[i].diceSize; /* Type of dice */

      for (j = 0; j < MAX_OBJ_AFFECT; j++)
      {
        if (obj_proto[num].affected[j].modifier)
        {
          if (obj_proto[num].affected[j].location == APPLY_HITROLL)
          {
            hitroll = obj_proto[num].affected[j].modifier;
            obj_proto[num].affected[j].modifier = 0;
            obj_proto[num].affected[j].location = 0;
          }
          else if (obj_proto[num].affected[j].location == APPLY_DAMROLL)
          {
            damroll = obj_proto[num].affected[j].modifier;
            obj_proto[num].affected[j].modifier = 0;
            obj_proto[num].affected[j].location = 0;
          }
        }
      }

      GET_OBJ_VAL(&obj_proto[num], 4) = MAX(hitroll, damroll);

      if (GET_OBJ_VAL(&obj_proto[num], 4) > 5)
        GET_OBJ_VAL(&obj_proto[num], 4) = 5;

      save_objects(real_zone_by_thing(obj_proto[num].item_number));

      found++;
    }
  }
  total += found;
  send_to_char(ch, "%d converted.\r\n", found);
  //  }
  send_to_char(ch, "Total of %d objects converted.\r\n", total);
}

#define DEBUG_EQ_SCORE TRUE
/* a function to "score" the value of equipment -zusuk
   what to take into consideration?
  type [object values], weapon spells, procs [have to be manually determined], spec abilities, affections, restrictions, weight,
  cost?, material, size, wear-slot, misc, ETC...
  --a very much ungoing project that is critical for balance, now have added a debug mode -zusuk 09/06/22
 */
int get_eq_score(obj_rnum a)
{
  int b, i;
  int score = 0;
  struct obj_data *obj = NULL;
  struct obj_special_ability *specab = NULL;
  struct descriptor_data *pt = NULL;
  bool found = FALSE;

  /* simplify life, and dummy checks */
  if (a == NOTHING)
    return -1;

  obj = &obj_proto[a];

  if (!obj)
    return -1;
  /* end dummy checks and we should have a valid object */

  /* first go through and score all the permanent affects, very powerful
   * either bonus OR penalty to the item score */
  for (i = 0; i < NUM_AFF_FLAGS; i++)
  {
    if (OBJAFF_FLAGGED(obj, i))
    {
      switch (i)
      {
      case AFF_AWARE:
        score += 600;
        break;

      case AFF_REGEN:
      case AFF_NOTRACK:
      case AFF_HASTE:
      case AFF_DISPLACE:
      case AFF_FREE_MOVEMENT:
        score += 550;
        break;

      case AFF_VAMPIRIC_TOUCH:
      case AFF_BLINKING:
      case AFF_TRUE_SIGHT:
      case AFF_SNEAK:
      case AFF_HIDE:
      case AFF_DEATH_WARD:
        score += 450;
        break;

      case AFF_BLUR:
      case AFF_SPELL_MANTLE:
      case AFF_INVISIBLE:
      case AFF_FLURRY_OF_BLOWS:
      case AFF_GLOBE_OF_INVULN:
      case AFF_EXPERTISE:
      case AFF_INERTIAL_BARRIER:
      case AFF_ULTRAVISION:
      case AFF_NOTELEPORT:
      case AFF_SPELL_TURNING:
      case AFF_MIND_BLANK:
      case AFF_SHADOW_SHIELD:
      case AFF_TIME_STOPPED:
      case AFF_ASHIELD:
        score += 350;
        break;

      case AFF_SENSE_LIFE:
      case AFF_DETECT_INVIS:
      case AFF_CLIMB:
      case AFF_FLYING:
      case AFF_NON_DETECTION:
      case AFF_FSHIELD:
      case AFF_CSHIELD:
      case AFF_MINOR_GLOBE:
      case AFF_DARKVISION:
      case AFF_SIZECHANGED:
      case AFF_SPELL_RESISTANT:
      case AFF_SPOT:
      case AFF_DANGERSENSE:
      case AFF_RAPID_SHOT:
        score += 300;
        break;

      case AFF_TFORM:
      case AFF_LISTEN:
      case AFF_SAFEFALL:
      case AFF_REFUGE:
      case AFF_BRAVERY:
      case AFF_BATTLETIDE:
      case AFF_DETECT_ALIGN:
      case AFF_DETECT_MAGIC:
      case AFF_WATERWALK:
      case AFF_LEVITATE:
      case AFF_SANCTUARY:
      case AFF_INFRAVISION:
      case AFF_PROTECT_EVIL:
      case AFF_PROTECT_GOOD:
      case AFF_SCUBA: /* includes AFF_WATERBREATH */
      case AFF_SPELLBATTLE:
      case AFF_TOWER_OF_IRON_WILL:
      case AFF_MAX_DAMAGE:
      case AFF_TOTAL_DEFENSE:
      case AFF_POWER_ATTACK:
      case AFF_CHARGING:
      case AFF_ELEMENT_PROT:
      case AFF_MAGE_FLAME:
      case AFF_ACROBATIC:
        score += 250;
        break;

        /* no value */
      case AFF_DONTUSE:
      case AFF_GROUP:
      case AFF_DUAL_WIELD:       /* currently this affection does nothing */
      case AFF_COUNTERSPELL:     /* currently this affection does nothing */
      case AFF_WHIRLWIND_ATTACK: /* currently this affection does nothing */
      case AFF_BODYWEAPONRY:     /* currently this affection does nothing */
        break;

        /* negative affections */
      case AFF_CONFUSED:
      case AFF_FAERIE_FIRE:
      case AFF_DIM_LOCK:
      case AFF_BLACKMANTLE:
      case AFF_BLIND:
      case AFF_CURSE:
      case AFF_POISON:
      case AFF_VAMPIRIC_CURSE:
      case AFF_NAUSEATED:
      case AFF_SLOW:
      case AFF_FATIGUED:
      case AFF_DISEASE:
      case AFF_TAMED:
      case AFF_GRAPPLED:
      case AFF_ENTANGLED:
      case AFF_DEAF:
      case AFF_FEAR:
      case AFF_STUN:
      case AFF_PARALYZED:
      case AFF_CHARM:
      case AFF_SLEEP:
      case AFF_DAZED:
      case AFF_FLAT_FOOTED:
      case AFF_CRIPPLING_CRITICAL:
        score -= 550;
        break;

        /* we are trying to handle every case */
      default:
        if (DEBUG_EQ_SCORE)
        {
          for (pt = descriptor_list; pt; pt = pt->next)
            if (pt->character)
              if (GET_LEVEL(pt->character) >= LVL_IMPL)
              {
                send_to_char(pt->character, "AFF_: %d | ", i);
                found = TRUE;
              }
        }
        score += 99999;
        break;
      }
    }
  }
  /* END affections */

  /* the "item" flags, rate them next */
  for (i = 0; i < NUM_ITEM_FLAGS; i++)
  {
    if (OBJ_FLAGGED(obj, i))
    {
      switch (i)
      {
        /* item can't have any real value in this state! */
      case ITEM_MOLD:
        score -= 999999;
        break;

        /* autoprocs have to be manually added later based on the item proc, a basic check is below though */
      case ITEM_AUTOPROC:
        /*score += 300;*/
        break;
      case ITEM_KI_FOCUS:
      case ITEM_FLAMING:
      case ITEM_FROST:
      case ITEM_SHOCK:
      case ITEM_AGILE:
      case ITEM_CORROSIVE:
      case ITEM_DEFENDING:
        score += 100;
        break;
      case ITEM_MAGLIGHT:
      case ITEM_MAGIC:
      case ITEM_BLESS:
      case ITEM_FLOAT:
      case ITEM_NOBURN:
      case ITEM_MASTERWORK:
        score += 50;
        break;
      case ITEM_GLOW:
      case ITEM_HUM:
      case ITEM_QUEST:
      case ITEM_NOINVIS:
        score += 10;
        break;

        /* doesn't affect score */

        /* reduce value of items below */
      case ITEM_INVISIBLE:
      case ITEM_NODONATE:
      case ITEM_NOLOCATE:
      case ITEM_HIDDEN:
      case ITEM_NOSELL:
        score -= 10;
        break;
      case ITEM_ANTI_HUMAN:
      case ITEM_ANTI_ELF:
      case ITEM_ANTI_DWARF:
      case ITEM_ANTI_HALF_TROLL:
      case ITEM_ANTI_MONK:
      case ITEM_ANTI_DRUID:
      case ITEM_ANTI_WIZARD:
      case ITEM_ANTI_CLERIC:
      case ITEM_ANTI_ROGUE:
      case ITEM_ANTI_WARRIOR:
      case ITEM_ANTI_WEAPONMASTER:
      case ITEM_ANTI_CRYSTAL_DWARF:
      case ITEM_ANTI_HALFLING:
      case ITEM_ANTI_H_ELF:
      case ITEM_ANTI_H_ORC:
      case ITEM_ANTI_GNOME:
      case ITEM_ANTI_BERSERKER:
      case ITEM_ANTI_TRELUX:
      case ITEM_ANTI_LICH:
      case ITEM_ANTI_VAMPIRE:
      case ITEM_VAMPIRE_ONLY:
      case ITEM_ANTI_SORCERER:
      case ITEM_ANTI_PALADIN:
      case ITEM_ANTI_RANGER:
      case ITEM_ANTI_BARD:
      case ITEM_ANTI_ARCANA_GOLEM:
      case ITEM_ANTI_DROW:
      case ITEM_ANTI_DUERGAR:
      case ITEM_ANTI_LAWFUL:
      case ITEM_ANTI_CHAOTIC:
      case ITEM_REQ_WIZARD:
      case ITEM_REQ_CLERIC:
      case ITEM_REQ_ROGUE:
      case ITEM_REQ_WARRIOR:
      case ITEM_REQ_MONK:
      case ITEM_REQ_DRUID:
      case ITEM_REQ_BERSERKER:
      case ITEM_REQ_SORCERER:
      case ITEM_REQ_PALADIN:
      case ITEM_REQ_RANGER:
      case ITEM_REQ_BARD:
      case ITEM_REQ_WEAPONMASTER:
      case ITEM_REQ_ARCANE_ARCHER:
      case ITEM_REQ_STALWART_DEFENDER:
      case ITEM_REQ_SHIFTER:
      case ITEM_REQ_DUELIST:
      case ITEM_REQ_MYSTIC_THEURGE:
      case ITEM_REQ_ALCHEMIST:
      case ITEM_REQ_ARCANE_SHADOW:
      case ITEM_REQ_SACRED_FIST:
      case ITEM_REQ_ELDRITCH_KNIGHT:
      case ITEM_ANTI_ARCANE_ARCHER:
      case ITEM_ANTI_STALWART_DEFENDER:
      case ITEM_ANTI_SHIFTER:
      case ITEM_ANTI_DUELIST:
      case ITEM_ANTI_MYSTIC_THEURGE:
      case ITEM_ANTI_ALCHEMIST:
      case ITEM_ANTI_ARCANE_SHADOW:
      case ITEM_ANTI_SACRED_FIST:
      case ITEM_ANTI_ELDRITCH_KNIGHT:
        score -= 15;
        break;
      case ITEM_ANTI_GOOD:
      case ITEM_ANTI_EVIL:
      case ITEM_ANTI_NEUTRAL:
        score -= 40;
        break;
      case ITEM_TRANSIENT:
      case ITEM_NODROP:
        score -= 75;
        break;
      case ITEM_NORENT:
      case ITEM_DECAY:
        score -= 450;
        break;

        /* we are attempting to handle every case */
      default:
        if (DEBUG_EQ_SCORE)
        {
          for (pt = descriptor_list; pt; pt = pt->next)
            if (pt->character)
              if (GET_LEVEL(pt->character) >= LVL_IMPL)
              {
                send_to_char(pt->character, "ITEM_: %d | ", i);
                found = TRUE;
              }
        }
        score += 99999;
        break;
      }
    }
  }
  /* END item types */

  /* now add up score for object affects (modifiers)
     added stacking bonus, that's critical!  -zusuk */
  for (b = 0; b < MAX_OBJ_AFFECT; b++)
  {
    if (obj_proto[a].affected[b].modifier)
    {
      switch (obj_proto[a].affected[b].location)
      {
      case APPLY_AGE:
      case APPLY_CHAR_WEIGHT:
      case APPLY_CHAR_HEIGHT:
        /* these are not worth so much*/
        score += obj_proto[a].affected[b].modifier;
        break;

      case APPLY_MOVE:
        /* these are not worth so much*/
        score += obj_proto[a].affected[b].modifier / 10;
        break;

      case APPLY_FEAT:
        score += 400;
        break;

      case APPLY_SKILL:
        score += 300;
        break;

      case APPLY_PSP:
      case APPLY_HIT:
        switch (obj_proto[a].affected[b].bonus_type)
        {
        default:
          score += obj_proto[a].affected[b].modifier * 8;
          if (BONUS_TYPE_STACKS(obj_proto[a].affected[b].bonus_type))
            score += obj_proto[a].affected[b].modifier * 8;
          break;
        }

        break;
      case APPLY_SAVING_FORT:
      case APPLY_SAVING_WILL:
      case APPLY_SAVING_REFL:
      case APPLY_SAVING_POISON:
      case APPLY_SAVING_DEATH:
      case APPLY_HITROLL:
      case APPLY_DAMROLL:
        switch (obj_proto[a].affected[b].bonus_type)
        {
        default:
          score += obj_proto[a].affected[b].modifier * 40;
          if (BONUS_TYPE_STACKS(obj_proto[a].affected[b].bonus_type))
            score += obj_proto[a].affected[b].modifier * 40;
          break;
        }
        break;
      case APPLY_AC:
        switch (obj_proto[a].affected[b].bonus_type)
        {
        default:
          score += obj_proto[a].affected[b].modifier * 10;
          if (BONUS_TYPE_STACKS(obj_proto[a].affected[b].bonus_type))
            score += obj_proto[a].affected[b].modifier * 10;
          break;
        }
        break;
      case APPLY_INT:
      case APPLY_WIS:
      case APPLY_CHA:
      case APPLY_AC_NEW:
        switch (obj_proto[a].affected[b].bonus_type)
        {
        default:
          score += obj_proto[a].affected[b].modifier * 80;
          if (BONUS_TYPE_STACKS(obj_proto[a].affected[b].bonus_type))
            score += obj_proto[a].affected[b].modifier * 80;
          break;
        }
        break;
      case APPLY_STR:
      case APPLY_DEX:
      case APPLY_CON:
        switch (obj_proto[a].affected[b].bonus_type)
        {
        default:
          score += obj_proto[a].affected[b].modifier * 90;
          if (BONUS_TYPE_STACKS(obj_proto[a].affected[b].bonus_type))
            score += obj_proto[a].affected[b].modifier * 90;
          break;
        }
        break;

      case APPLY_DR:
        switch (obj_proto[a].affected[b].bonus_type)
        {
        default:
          score += obj_proto[a].affected[b].modifier * 150;
          if (BONUS_TYPE_STACKS(obj_proto[a].affected[b].bonus_type))
            score += obj_proto[a].affected[b].modifier * 150;
          break;
        }
        break;

      case APPLY_SPELL_RES:
      case APPLY_RES_FIRE:
      case APPLY_RES_COLD:
      case APPLY_RES_AIR:
      case APPLY_RES_EARTH:
      case APPLY_RES_ACID:
      case APPLY_RES_HOLY:
      case APPLY_RES_ELECTRIC:
      case APPLY_RES_UNHOLY:
      case APPLY_RES_PUNCTURE:
      case APPLY_RES_SLICE:
      case APPLY_RES_FORCE:
      case APPLY_RES_SOUND:
      case APPLY_RES_POISON:
      case APPLY_RES_DISEASE:
      case APPLY_RES_NEGATIVE:
      case APPLY_RES_ILLUSION:
      case APPLY_RES_MENTAL:
      case APPLY_RES_LIGHT:
      case APPLY_RES_ENERGY:
      case APPLY_RES_WATER:
        switch (obj_proto[a].affected[b].bonus_type)
        {
        default:
          score += obj_proto[a].affected[b].modifier * 20;
          if (BONUS_TYPE_STACKS(obj_proto[a].affected[b].bonus_type))
            score += obj_proto[a].affected[b].modifier * 20;
          break;
        }
        break;

        /* these are not supposed to be used, so add them insane numbers */
      case APPLY_SIZE:
      case APPLY_CLASS:
      case APPLY_LEVEL:
      case APPLY_GOLD:
      case APPLY_EXP:
      default:
        if (DEBUG_EQ_SCORE)
        {
          for (pt = descriptor_list; pt; pt = pt->next)
            if (pt->character)
              if (GET_LEVEL(pt->character) >= LVL_IMPL)
              {
                send_to_char(pt->character, "APPLY_: %d | ", obj_proto[a].affected[b].location);
                found = TRUE;
              }
        }

        score += 99999;
        break;
      }
    }
  }
  /* END object affections*/

  /************/
  /* misc */

  /* ac-apply value */
  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    score += GET_OBJ_VAL(obj, 0) * 10;

  /* enhancement bonus */
  score += (GET_ENHANCEMENT_BONUS(obj) <= 10 ? GET_ENHANCEMENT_BONUS(obj) * 75 : 99999);

  /* spec abilities */
  for (specab = obj->special_abilities; specab != NULL; specab = specab->next)
  {
    score += special_ability_info[specab->ability].cost * 100;
  }

  /* weapons */
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
  {
    score += (GET_WEAPON_TYPE(obj) != 0 ? 0 : GET_OBJ_VAL(obj, 1) * GET_OBJ_VAL(obj, 2));
  }

  /* spec procs in our spec_procs/zone_procs.c files */
  if (obj_index[GET_OBJ_RNUM(obj)].func)
  {
    score += 500;
  }

  if (HAS_SPELLS(obj))
  {
    for (i = 0; i < MAX_WEAPON_SPELLS; i++)
    { /* increment this weapons spells */
      if (GET_WEAPON_SPELL(obj, i))
      {
        score += 250;
      }
    }
  }

  /* unimplemented, but maybe shield weight offers bonus to shieldslam? */
  /*
  if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
    score += GET_OBJ_WEIGHT(obj);
   */

  /* END misc */
  /************/

  /* for our debug, this will give the end of line for us */
  if (DEBUG_EQ_SCORE && found)
  {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (pt->character)
        if (GET_LEVEL(pt->character) >= LVL_IMPL)
          send_to_char(pt->character, "~~\r\n");
  }

  /* DONE! */
  return score;
}
#undef DEBUG_EQ_SCORE

/* a command meant to view the top end equipment of the game -zusuk */

/* takes one or two arguments, 2nd argument being optional:
   arg 1:  item slot
   arg 2:  zone
 */
ACMD(do_eqrating)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  char buf[MAX_STRING_LENGTH] = {'\0'}, bitbuf[MEDIUM_STRING] = {'\0'};

  int *index = NULL; /* one of two tables */
  int *score = NULL; /* one of two tables */

  int i = 0, j = 0;         /* counter */
  int a = 0, b = 0;         /* used for sorting */
  int len = 0, tmp_len = 0; /* string length */
  int wearloc = 0;          /* the wear-location of item */

  zone_vnum zone = 0;                           /* zone vnum to restrict search */
  room_vnum start_of_zone = 0, end_of_zone = 0; /* bottom/top of zone vnums */

  struct obj_data *obj = NULL;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "Which object wear location do you want to list?\r\n");
    for (i = 1; i < NUM_ITEM_WEARS; i++)
    {
      send_to_char(ch, "%s%2d%s-%s%-14s%s", QNRM, i, QNRM, QYEL, wear_bits[i], QNRM);
      if (!(i % 4))
        send_to_char(ch, "\r\n");
    }
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Usage: %seqrating <num> <optionally zone>%s\r\n", QYEL, QNRM);
    send_to_char(ch, "Displays objects worn in the selected location sorted by rating.\r\n");

    return;
  }

  wearloc = atoi(arg1);

  /* dummy check:  0 = takeable */
  if (wearloc >= NUM_ITEM_WEARS || wearloc <= 0)
  {
    send_to_char(ch, "Invalid slot!\r\n");
    return;
  }

  /* we now have a valid wear location, was a zone number also submitted? */
  if (isdigit(*arg2))
  {

    zone = atoi(arg2);

    for (i = 0; i <= top_of_zone_table; i++)
    {
      if (zone_table[i].number == zone) /* found a matching zone? */
        break;
    }

    if (i <= 0 || i > top_of_zone_table)
    {
      send_to_char(ch, "Zone %d does not exist.\r\n", zone);
      return;
    }

    start_of_zone = zone_table[i].bot;
    end_of_zone = zone_table[i].top;
  }

  /* allocate memory for our tables */
  CREATE(index, int, top_of_objt);
  CREATE(score, int, top_of_objt);

  /* Create tables of eq worn at that slot, with rating */

  /* the table index is going to be "j", the object real-num "i" will be
   * stored in the index-table along with the score in the score-table */
  for (i = 0; i <= top_of_objt; i++)
  {
    if (IS_SET_AR(obj_proto[i].obj_flags.wear_flags, wearloc))
    {
      if (zone)
      { /* zone values */
        if (obj_index[i].vnum >= start_of_zone &&
            obj_index[i].vnum <= end_of_zone)
        {
          score[j] = get_eq_score(i);
          index[j] = i;
          j++;
        }
      }
      else
      { /* full list */
        score[j] = get_eq_score(i);
        index[j] = i;
        j++;
      }
    }
  }

  /* variable note, "i" is being used as a temporary storage value here,
   not a counter like before */
  if (j > 1)
  {
    /* Sort the table, so that the best eq is at top*/
    for (a = 0; a < j - 1; a++)
    {
      for (b = a + 1; b < j; b++)
      {
        if (score[a] < score[b])
        {
          i = score[a];
          score[a] = score[b];
          score[b] = i;
          i = index[a];
          index[a] = index[b];
          index[b] = i;
        }
      }
    }
  }

  if (zone)
  {
    send_to_char(ch, "Objects in the range %ld - %ld.\r\n",
                 (long int)start_of_zone, (long int)end_of_zone);
  }

  /* show the table */
  len = snprintf(buf, sizeof(buf), "Listing all objects with wear location %s[%s]%s\r\n",
                 QYEL, wear_bits[wearloc], QNRM);
  for (i = 0; i < j; i++)
  {

    /* 'a' will refer to our "i'th" value in the table */
    a = index[i];

    /* start building our string, begin with listing vnum, score, and short
     description */
    tmp_len = snprintf(buf + len, sizeof(buf) - len, "%7ld | %5d | %-45s | ",
                       (long int)obj_index[a].vnum, get_eq_score(a), obj_proto[a].short_description);
    len += tmp_len;

    /* now, if we have a weapon, display dice */
    if (GET_OBJ_TYPE(&obj_proto[a]) == ITEM_WEAPON)
    {
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "%dD%d | ",
                         GET_OBJ_VAL(&obj_proto[a], 1),
                         GET_OBJ_VAL(&obj_proto[a], 2));
      len += tmp_len;
    }

    if (CAN_WEAR(&obj_proto[a], ITEM_WEAR_SHIELD))
    { /* shield weight */
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "wt %d | ",
                         GET_OBJ_WEIGHT(&obj_proto[a]));
      len += tmp_len;
    }

    if (GET_OBJ_TYPE(&obj_proto[a]) == ITEM_ARMOR)
    { /* ac-apply */
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "AC %d | ",
                         GET_OBJ_VAL(&obj_proto[a], 0));
      len += tmp_len;
    }

    if (GET_OBJ_AFFECT(&obj_proto[a]))
    { /* perm affects */
      sprintbitarray(GET_OBJ_AFFECT(&obj_proto[a]), affected_bits, AF_ARRAY_MAX, bitbuf);
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s | ",
                         bitbuf);
      len += tmp_len;
    }

    *bitbuf = '\0';

    /* has affect locations? */
    obj = &obj_proto[a];
    if (!obj)
      return; /* super dummy check */
    for (b = 0; b < MAX_OBJ_AFFECT; b++)
    {
      if ((obj->affected[b].location != APPLY_NONE) &&
          (obj->affected[b].modifier != 0))
      {
        sprinttype(obj->affected[b].location, apply_types, bitbuf, sizeof(bitbuf));
        tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s %d ", bitbuf, obj->affected[b].modifier);
        len += tmp_len;
      }
    }

    tmp_len = snprintf(buf + len, sizeof(buf) - len, "|\r\n");
    len += tmp_len;

    /* yeah we have to cap things, wish i had a better solution at this stage!
     -zusuk */
    if (i >= 350)
    {
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "\r\n**OVERLOADED BUFF***\r\n");
      len += tmp_len;
      break;
    }
  }

  page_string(ch->desc, buf, TRUE);

  free(index);
  free(score);
}

/* a little command to convert co-ordinates to pixel location on a graphic
     - used for marking zones on a map and vice versa */
ACMD(do_coordconvert)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  int tmp_x_value = -1025, tmp_y_value = -1025;
  int x_value = -1025, y_value = -1025;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  send_to_char(ch, "                            \tCBoundaries\tn\r\n"
                   "\tc              Pixel Location    In-Game Co-ordinates\r\n"
                   "\tcTop Left:\tW        0,    0       -1024,  1024\tn\r\n"
                   "\tcTop Right:\tW    2048,    0        1024,  1024\tn\r\n"
                   "\tcBottom Left:\tW     0, 2048       -1024, -1024\tn\r\n"
                   "\tcBottom Right: \tW2048, 2048        1024, -1024\tn\r\n\r\n");

  /* need two arguments */
  if (!*arg1)
  {
    send_to_char(ch, "You need two arguments: a co-ordinate or pixel X and Y values.\r\n");
    return;
  }
  if (!*arg2)
  {
    send_to_char(ch, "You need two arguments: a co-ordinate or pixel X and Y values.\r\n");
    return;
  }

  tmp_x_value = atoi(arg1);
  tmp_y_value = atoi(arg2);

  if (tmp_x_value < -1024 || tmp_y_value < -1024 ||
      tmp_x_value > 2048 || tmp_y_value > 2048)
  {
    send_to_char(ch, "Please try again, there is no reason to use a value below "
                     "-1024 or above 2048.\r\n");
    return;
  }

  x_value = tmp_x_value - 1024;
  y_value = 1024 - tmp_y_value;
  send_to_char(ch, "Converting pixel location to \tcmap co-ordinates\tn: %s%5d\tn %s%5d\tn\r\n",
               (x_value < -1024 || x_value > 1024) ? "\tR" : "\tW", x_value,
               (x_value < -1024 || x_value > 1024) ? "\tR" : "\tW", y_value);

  x_value = tmp_x_value + 1024;
  y_value = 1024 - tmp_y_value;
  send_to_char(ch, "Converting map co-ordinates to \tcpixel location\tn: %s%5d %s%5d\tn\r\n",
               (x_value < 0 || x_value > 2048) ? "\tR" : "\tW", x_value,
               (x_value < 0 || x_value > 2048) ? "\tR" : "\tW", y_value);
  send_to_char(ch, "\r\n\tRRed coloring\tn indicates you are above/below the boundaries for this particular category.\r\n");
}

/* findmagic command - finds scrolls, potions, wands or staves with a specified spell */
/* Written by Jamdog - 25th February 2007, ported by Zusuk                                             */
ACMD(do_findmagic)
{
  char spellname[MAX_INPUT_LENGTH] = {'\0'}, objname[MAX_INPUT_LENGTH] = {'\0'};
  int spellnum, hits = 0, r_num, num;
  struct obj_data *obj;

  half_chop_c(argument, objname, sizeof(objname), spellname, sizeof(spellname));

  if ((!*objname) || (!*spellname))
  {
    send_to_char(ch, "Usage: findmagic [potion|scroll|wand|staff] <spell>\r\n\r\n");
    send_to_char(ch, "<spell> can be the spell name or VNUM. Do NOT use quotes.\r\nSpell Name abbreviations are allowed.\r\n");
    send_to_char(ch, "Examples:\r\n   findmagic potion armor\r\n   findmagic scroll 56\r\n  findmagic staff sanc");
    return;
  }

  spellnum = find_skill_num(spellname);

  if (spellnum == -1)
  {
    spellnum = atoi(spellname);
    if (spellnum <= 0)
    {
      send_to_char(ch, "Invalid spell name or spell number\r\nUsage: findmagic [potion|scroll|wand|staff] <spell>");
      return;
    }
  }

  for (num = 0; num <= top_of_objt; num++)
  {

    if (((obj_proto[num].obj_flags.type_flag == ITEM_POTION) && (!strcmp(objname, "potion"))) ||
        ((obj_proto[num].obj_flags.type_flag == ITEM_SCROLL) && (!strcmp(objname, "scroll"))))
    {
      if ((obj_proto[num].obj_flags.value[1] == spellnum) ||
          (obj_proto[num].obj_flags.value[2] == spellnum) ||
          (obj_proto[num].obj_flags.value[3] == spellnum))
      {
        hits++;
        r_num = real_object(obj_index[num].vnum);
        obj = read_object(r_num, REAL);
        if (hits == 1)
          send_to_char(ch, "Showing %ss with the '%s' spell\r\nNum  VNUM    Name\r\n", objname, skill_name(spellnum));
        send_to_char(ch, "%4d %s[%s%5d%s]%s %s%s\r\n", hits, CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), obj_index[num].vnum, CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), obj->short_description, CCNRM(ch, C_NRM));
      }
    }
    if (((obj_proto[num].obj_flags.type_flag == ITEM_WAND) && (!strcmp(objname, "wand"))) ||
        ((obj_proto[num].obj_flags.type_flag == ITEM_STAFF) && (!strcmp(objname, "staff"))))
    {
      if (obj_proto[num].obj_flags.value[3] == spellnum)
      {
        hits++;
        r_num = real_object(obj_index[num].vnum);
        obj = read_object(r_num, REAL);
        if (hits == 1)
          send_to_char(ch, "Num  VNUM   Name\r\n");
        send_to_char(ch, "%4d %6d %s (%d charges)\r\n", hits, obj_index[num].vnum, obj->short_description, obj_proto[num].obj_flags.value[1]);
      }
    }
  }

  if (hits > 0)
  {
    send_to_char(ch, "%d %ss found with the '%s' spell\r\n", hits, objname, spell_name(spellnum));
  }
  else
  {
    send_to_char(ch, "Sorry, no %ss found with the '%s' spell\r\n", objname, spell_name(spellnum));
  }

  return;
}

/* temporarily change a command's level, adapted from ParagonMUD by Zusuk */
ACMD(do_cmdlev)
{
  int iCmd, iLev;
  char buf[MAX_STRING_LENGTH] = {'\0'}, buf2[MAX_STRING_LENGTH] = {'\0'};

  two_arguments(argument, buf, sizeof(buf), buf2, sizeof(buf2));

  if (!*buf)
  {
    send_to_char(ch, "Usage: cmdlev <command> <level>\r\nTemporarily sets the required level for a command.\r\n");
    send_to_char(ch, "Be careful with Imm commands!\r\nA reboot will reset all command levels to default.\r\n");
    return;
  }
  if (!*buf2)
  {
    send_to_char(ch, "Usage: cmdlev <command> <level>\r\nWhat level would you like to set this command at?.\r\n");
    return;
  }

  iCmd = find_command(buf);

  if (iCmd == -1)
  {
    send_to_char(ch, "That command does not exist!  Unable to change level!\r\n");
    return;
  }

  iLev = atoi(buf2);

  if ((iLev < 1) || (iLev > GET_LEVEL(ch)))
  {
    send_to_char(ch, "Invalid level (Range: 1-%d)\r\n", GET_LEVEL(ch));
    return;
  }
  if (complete_cmd_info[iCmd].minimum_level > GET_LEVEL(ch))
  {
    send_to_char(ch, "You cannot change the level on a command to which you don't have access!\r\n");
    return;
  }

  /* All checks done - set the command level */
  complete_cmd_info[iCmd].minimum_level = iLev;
  send_to_char(ch, "Command level changed (%s%s%s is now available to anyone level %d or higher)\r\n", CCYEL(ch, C_NRM), complete_cmd_info[iCmd].command, CCNRM(ch, C_NRM), iLev);
  send_to_char(ch, "NOTE: Command levels are restored to default during a reboot or copyover\r\n");
  mudlog(NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE, "(GC) %s set command level for %s to %d", GET_NAME(ch), complete_cmd_info[iCmd].command, iLev);
}

ACMD(do_unbind)
{
  char obj_name[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj;

  one_argument(argument, obj_name, sizeof(obj_name));
  if (!*obj_name)
  {
    send_to_char(ch, "What would you like to unbind?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, ch->carrying)))
  {
    send_to_char(ch, "You are not carrying a %s?\r\n", obj_name);
    return;
  }

  if (GET_OBJ_BOUND_ID(obj) == NOBODY)
  {
    send_to_char(ch, "But, the %s@n is not bound to anyone!\r\n", obj->short_description);
    return;
  }

  if (get_name_by_id(GET_OBJ_BOUND_ID(obj)) == NULL)
  {
    send_to_char(ch, "It would appear that the person this object is bound to has deleted!\r\n");
    send_to_char(ch, "This item has now been unbound!\r\n");
    GET_OBJ_BOUND_ID(obj) = NOBODY;
    return;
  }
  snprintf(obj_name, sizeof(obj_name), "%s", obj->short_description);
  send_to_char(ch, "%s%s was bound to %s\r\n", CAP(obj_name), CCNRM(ch, C_NRM), get_name_by_id(GET_OBJ_BOUND_ID(obj)));
  send_to_char(ch, "This item has now been unbound!\r\n");
  GET_OBJ_BOUND_ID(obj) = NOBODY;
}

ACMD(do_obind)
{
  char char_name[MAX_INPUT_LENGTH] = {'\0'}, obj_name[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj;
  struct char_data *vict;

  two_arguments(argument, obj_name, sizeof(obj_name), char_name, sizeof(char_name));

  if (!*obj_name)
  {
    send_to_char(ch, "What do you want to bind, and to whom?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, ch->carrying)))
  {
    send_to_char(ch, "You are not carrying a %s?\r\n", obj_name);
    return;
  }

  if (!*char_name)
  {
    send_to_char(ch, "To whom would you like to bind %s?\r\n", obj->short_description);
    return;
  }

  if (!(vict = get_char_vis(ch, char_name, NULL, FIND_CHAR_WORLD)))
  {
    send_to_char(ch, "No one by that name currently logged in.\r\n");
    return;
  }

  if (GET_OBJ_BOUND_ID(obj) != NOBODY)
  {
    if (GET_OBJ_BOUND_ID(obj) == GET_IDNUM(vict))
    {
      send_to_char(ch, "It is already bound to %s.\r\n", GET_NAME(vict));
      return;
    }
    else
    {
      send_to_char(ch, "It is currently bound to %s. Unbind it first!\r\n", get_name_by_id(GET_OBJ_BOUND_ID(obj)));
      return;
    }
  }
  GET_OBJ_BOUND_ID(obj) = GET_IDNUM(vict);
  send_to_char(ch, "%s is now bound to %s.", obj->short_description, GET_NAME(vict));
}

/*
#define PLIST_FORMAT \
  "plist [minlev[-maxlev]] [-n name] [-a] [-u] [-o] [-i] [-m]"

ACMDU(do_plist) {
  int i, len = 0, count = 0;
  char col, mode, buf[MAX_STRING_LENGTH] = {'\0'}, name_search[MAX_NAME_LENGTH], time_str[MAX_STRING_LENGTH] = {'\0'};
  struct time_info_data time_away;
  int low = 0, high = LVL_IMPL, low_day = 0, high_day = 10000, low_hr = 0, high_hr = 24;
  int active = 0, unactive = 0, old = 0, selected = 0;

  skip_spaces(&argument);
  strcpy(buf, argument); // strcpy: OK (sizeof: argument == buf)
  name_search[0] = '\0';

  while (*buf) {
    char arg[MAX_INPUT_LENGTH] = {'\0'}, buf1[MAX_INPUT_LENGTH] = {'\0'};

    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      if (sscanf(arg, "%d-%d", &low, &high) == 1)
        high = low;
      strcpy(buf, buf1); // strcpy: OK (sizeof: buf1 == buf)
    } else if (*arg == '-') {
      mode = *(arg + 1); // just in case; we destroy arg in the switch
      switch (mode) {
        case 'l':
          half_chop(buf1, arg, buf);
          sscanf(arg, "%d-%d", &low, &high);
          break;
        case 'n':
          half_chop(buf1, name_search, buf);
          break;
        case 'a':
          strcpy(buf, buf1);
          selected = active = TRUE;
          break;
        case 'u':
          strcpy(buf, buf1);
          selected = unactive = TRUE;
          break;
        case 'o':
          strcpy(buf, buf1);
          selected = old = TRUE;
          break;
        case 'd':
          half_chop(buf1, arg, buf);
          if (sscanf(arg, "%d-%d", &low_day, &high_day) == 1)
            high_day = low_day;
          break;
        case 'h':
          half_chop(buf1, arg, buf);
          if (sscanf(arg, "%d-%d", &low_hr, &high_hr) == 1)
            high_hr = low_hr;
          break;
        default:
          send_to_char(ch, "%s\r\n", PLIST_FORMAT);
          return;
      }
    } else {
      send_to_char(ch, "%s\r\n", PLIST_FORMAT);
      return;
    }
  }

  len = 0;
  len += snprintf(buf + len, sizeof (buf) - len, "\tM[ Id] (Lvl) Name         Last\tn\r\n"
          "\tc-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\tn\r\n");

  for (i = 0; i <= top_of_p_table; i++) {
    if (player_table[i].level < low || player_table[i].level > high)
      continue;

    if (*name_search && str_cmp(name_search, player_table[i].name))
      continue;

    time_away = *real_time_passed(time(0), player_table[i].last);

    if (time_away.day >= 60 || time_away.month >= 1 || time_away.year >= 1) {
      if (!old && selected)
        continue;
      col = 'd';
    } else if (time_away.day >= 21 && player_table[i].level >= LVL_IMMORT) {
      if (!old && selected)
        continue;
      col = 'd';
    } else if (time_away.day >= 14 || time_away.month >= 1 || time_away.year >= 1) {
      if (!unactive && selected)
        continue;
      col = 'D';
    } else if ((time_away.day >= 7 || time_away.month >= 1 || time_away.year >= 1) && player_table[i].level >= LVL_IMMORT) {
      if (!unactive && selected)
        continue;
      col = 'D';
    } else {
      if (!active && selected)
        continue;
      col = 'W';
    }

    if (time_away.day > high_day || time_away.day < low_day)
      continue;
    if (time_away.hours > high_hr || time_away.hours < low_hr)
      continue;

    strcpy(time_str, asctime(localtime(&player_table[i].last)));

    time_str[strlen(time_str) - 1] = '\0';

    len += snprintf(buf + len, sizeof (buf) - len, "\t%c[%3ld] (%3d) %-12s %s\tn\r\n",
            col, player_table[i].id, player_table[i].level, CAP(strdup(player_table[i].name)),
            time_str);
    count++;
  }
  snprintf(buf + len, sizeof (buf) - len, "@c-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-@n\r\n"
          "%d players listed.\r\n", count);
  page_string(ch->desc, buf, TRUE);
}
 */

/* do_finddoor, finds the door(s) that a key goes to */
ACMD(do_finddoor)
{
  int d, vnum = NOTHING, num = 0;
  size_t len, nlen;
  room_rnum i;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char buf[MAX_STRING_LENGTH] = {0};
  struct char_data *tmp_char;
  struct obj_data *obj;

  /* dummy check */
  if (!ch)
    return;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Format: finddoor <obj/vnum>\r\n");
  }
  else if (is_number(arg))
  {
    vnum = atoi(arg);
    obj = &obj_proto[real_object(vnum)];
  }
  else
  {
    generic_find(arg,
                 FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_WORLD | FIND_OBJ_EQUIP,
                 ch, &tmp_char, &obj);
    if (!obj)
      send_to_char(ch, "What key do you want to find a door for?\r\n");
    else
      vnum = GET_OBJ_VNUM(obj);
  }
  if (vnum != NOTHING)
  {
    len = snprintf(buf, sizeof(buf), "Doors unlocked by key %s[%s%d%s]%s %s%s are:\r\n",
                   CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), vnum, CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), obj->short_description, CCNRM(ch, C_NRM));
    for (i = 0; i <= top_of_world; i++)
    {
      for (d = 0; d < NUM_OF_DIRS; d++)
      {
        if (world[i].dir_option[d] && world[i].dir_option[d]->key &&
            world[i].dir_option[d]->key == vnum)
        {
          nlen = snprintf(buf + len, sizeof(buf) - len,
                          "[%3d] Room %d, %s (%s)\r\n",
                          ++num, world[i].number,
                          dirs[d], world[i].dir_option[d]->keyword);
          if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
          len += nlen;
        }
      } /* for all directions */
    }   /* for all rooms */
    if (num > 0)
      page_string(ch->desc, buf, 1);
    else
      send_to_char(ch, "No doors were found for key [%d] %s.\r\n", vnum, obj->short_description);
  }
}

ACMD(do_players)
{
  struct descriptor_data *d = NULL;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  char buf3[MAX_STRING_LENGTH] = {'\0'};
  int counter = 0, i = 0;

  snprintf(buf, sizeof(buf), "%-15s %-15s %-3s %-15s %-7s %-7s %-20s\r\n", "Name", "Account", "Lvl", "Alignment", "Room", "Race", "Class");
  send_to_char(ch, "%s", buf);

  snprintf(buf, sizeof(buf),
           "---------------------------------------------------------------------------\r\n");
  send_to_char(ch, "%s", buf);

  for (d = descriptor_list; d; d = d->next)
  {
    if (!d->character)
      continue;

    counter = 0;
    *buf2 = '\0';
    for (i = 0; i < MAX_CLASSES; i++)
    {
      if (d->character && CLASS_LEVEL(d->character, i))
      {
        if (counter)
          strlcat(buf2, " / ", sizeof(buf2));
        char res_buf[128];
        snprintf(res_buf, sizeof(res_buf), "%d %s", CLASS_LEVEL(d->character, i), CLSLIST_ABBRV(i));
        strlcat(buf2, res_buf, sizeof(buf2));

        counter++;
      }
    }

    *buf3 = '\0';
    snprintf(buf3, sizeof(buf3), "%s", get_align_by_num(GET_ALIGNMENT(d->character)));
    strip_colors(buf3);

    if (STATE(d) == CON_PLAYING)
    {
      snprintf(buf, sizeof(buf),
               "%-15s %-15s %-3d %-15s %-7d %-7s %s\r\n",
               GET_NAME(d->character),
               (d && d->account && d->account->name) ? d->account->name : "None",
               GET_LEVEL(d->character),
               buf3,
               GET_ROOM_VNUM(IN_ROOM(d->character)),
               race_list[GET_RACE(d->character)].abbrev,
               buf2);
      send_to_char(ch, "%s", buf);
    }

    else
    {
      snprintf(buf, sizeof(buf), "%-15s %-15s %-3d %-15s %-7s %-7s %s\r\n",
               GET_NAME(d->character),
               "Offline",
               GET_LEVEL(d->character),
               buf3,
               "Offline",
               race_list[GET_RACE(d->character)].abbrev,
               buf2);
      send_to_char(ch, "%s", buf);
    }
  }
}

ACMD(do_copyroom)
{

  skip_spaces_c(&argument);

  int i = 0;

  if (!*argument)
  {
    send_to_char(ch, "You need to specify the source room vnum.\r\n");
    return;
  }

  for (i = 0; i < top_of_world; i++)
  {
    if (GET_ROOM_VNUM(i) == atoi(argument))
      break;
  }

  if (i >= top_of_world)
  {
    send_to_char(ch, "That room vnum doesn't exist. You need to specify the source room vnum.\r\n");
    return;
  }

  world[IN_ROOM(ch)].sector_type = world[i].sector_type;
  world[IN_ROOM(ch)].name = world[i].name;
  world[IN_ROOM(ch)].description = world[i].description;
  world[IN_ROOM(ch)].room_flags[0] = world[i].room_flags[0];
  world[IN_ROOM(ch)].room_flags[1] = world[i].room_flags[1];
  world[IN_ROOM(ch)].room_flags[2] = world[i].room_flags[2];
  world[IN_ROOM(ch)].room_flags[3] = world[i].room_flags[3];

  send_to_char(ch, "You have copied this room with the name, description, sector and room flags of room vnum %d.\r\n", GET_ROOM_VNUM(i));

  add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);
}

void check_auto_shutdown(void)
{
  if (IS_HAPPYHOUR || IS_STAFF_EVENT)
    return;

  char *tmstr;
  time_t mytime;
  int h, m, hour;

  mytime = time(0);

  h = hour = (mytime / 3600) % 24;
  m = (mytime / 60) % 60;

  if ((h == 7) && m == 30)
  {
    send_to_all(
        "**************************************************************\r\n"
        "**************************************************************\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "***         THE MUD WILL BE REBOOTING IN 30 MINUTES!       ***\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "**************************************************************\r\n"
        "**************************************************************\r\n");
  }
  else if ((h == 7) && m == 45)
  {
    send_to_all(
        "**************************************************************\r\n"
        "**************************************************************\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "***         THE MUD WILL BE REBOOTING IN 15 MINUTES!       ***\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "**************************************************************\r\n"
        "**************************************************************\r\n");
  }
  else if ((h == 7) && m == 55)
  {
    send_to_all(
        "**************************************************************\r\n"
        "**************************************************************\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "***         THE MUD WILL BE REBOOTING IN 05 MINUTES!       ***\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "**************************************************************\r\n"
        "**************************************************************\r\n");
  }
  else if ((h == 7) && m == 58)
  {
    send_to_all(
        "**************************************************************\r\n"
        "**************************************************************\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "***         THE MUD WILL BE REBOOTING IN 02 MINUTES!       ***\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "**************************************************************\r\n"
        "**************************************************************\r\n");
  }
  else if ((h == 7) && m == 59)
  {
    send_to_all(
        "**************************************************************\r\n"
        "**************************************************************\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "***         THE MUD WILL BE REBOOTING IN 01 MINUTES!       ***\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "**************************************************************\r\n"
        "**************************************************************\r\n");
  }
  else if ((h == 8) && m == 0)
  {
    send_to_all(
        "**************************************************************\r\n"
        "**************************************************************\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "***                 THE MUD IS REBOOTING NOW!              ***\r\n"
        "***                                                        ***\r\n"
        "***                                                        ***\r\n"
        "**************************************************************\r\n"
        "**************************************************************\r\n");
    tmstr = (char *)asctime(localtime(&mytime));
    *(tmstr + strlen(tmstr) - 1) = '\0';
    log("Automated Copyover on %s.", tmstr);
    send_to_all("Executing Automated Copyover.\r\n");
    perform_do_copyover();
  }
}

ACMD(do_perfmon)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};

  argument = one_argument(argument, arg1, sizeof(arg1));

  if (arg1[0] == '\0')
  {
    send_to_char(ch,
                 "perfmon all             - Print all perfmon info.\r\n"
                 "perfmon summ            - Print summary,\r\n"
                 "perfmon prof            - Print profiling info.\r\n"
                 "perfmon sect <section>  - Print profiling info for section.\r\n");
    return;
  }

  if (!str_cmp(arg1, "all"))
  {
    char buf[MAX_STRING_LENGTH] = {'\0'};

    size_t written = PERF_repr(buf, sizeof(buf));
    written = PERF_prof_repr_total(buf + written, sizeof(buf) - written);

    page_string(ch->desc, buf, TRUE);

    return;
  }
  else if (!str_cmp(arg1, "summ"))
  {
    char buf[MAX_STRING_LENGTH] = {'\0'};

    PERF_repr(buf, sizeof(buf));
    page_string(ch->desc, buf, TRUE);
    return;
  }
  else if (!str_cmp(arg1, "prof"))
  {
    char buf[MAX_STRING_LENGTH] = {'\0'};

    PERF_prof_repr_total(buf, sizeof(buf));
    page_string(ch->desc, buf, TRUE);

    return;
  }
  else if (!str_cmp(arg1, "sect"))
  {
    char buf[MAX_STRING_LENGTH] = {'\0'};

    PERF_prof_repr_sect(buf, sizeof(buf), argument);
    page_string(ch->desc, buf, TRUE);

    return;
  }
  else
  {
    do_perfmon(ch, "", cmd, subcmd);
    return;
  }
}

ACMD(do_showwearoff)
{

  char arg1[MEDIUM_STRING] = {'\0'};

  one_argument(argument, arg1, sizeof(arg1));

  if (!*arg1)
  {
    send_to_char(ch, "Please specify a spell or skill.\r\n");
    return;
  }

  int i = 0;

  for (i = 0; i < TOP_SKILL_DEFINE + 1; i++)
  {
    if (is_abbrev(arg1, spell_info[i].name))
    {
      send_to_char(ch, "Spell/Skill: %s\r\n"
                       "Wearoff Msg: %s.\r\n",
                   spell_info[i].name,
                   get_wearoff(i));
      return;
    }
  }

  send_to_char(ch, "There is no spell or skill by that name.\r\n");
}

ACMD(do_resetpassword)
{

  char query[2048], arg1[MAX_NAME_LENGTH], arg2[MAX_PWD_LENGTH], password[MAX_PWD_LENGTH];
  MYSQL_RES *res;
  MYSQL_ROW row;
  bool account_found = false;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "Please specify the account for which you would like to change the password.\r\n");
    return;
  }
  if (!*arg2)
  {
    send_to_char(ch, "Please specify what you would like the new password to be.\r\n");
    return;
  }

  if (strstr(arg2, ";") || strstr(arg2, "'"))
  {
    send_to_char(ch, "Passwords cannot contain ' or ; symbols.\r\n");
    return;
  }

  arg1[0] = toupper(arg1[0]);

  snprintf(query, sizeof(query), "SELECT name from account_data WHERE name='%s'", arg1);
  mysql_query(conn, query);
  res = mysql_use_result(conn);
  if (res != NULL)
  {
    if ((row = mysql_fetch_row(res)) != NULL)
    {
      account_found = true;
    }
  }
  mysql_free_result(res);

  if (!account_found)
  {
    send_to_char(ch, "There is no account by that name.\r\n");
    return;
  }

  snprintf(password, sizeof(password), "%s", CRYPT(arg2, arg1));

  snprintf(query, sizeof(query), "UPDATE account_data SET password='%s' WHERE name='%s'", password, arg1);
  if (!mysql_query(conn, query))
  {
    send_to_char(ch, "You have updated account %s's password to '%s'.\r\n", arg1, arg2);
    return;
  }

  send_to_char(ch, "There was an error saving the new password to the database.  Password not changed.\r\n");
}

ACMD(do_award)
{

  char arg1[MEDIUM_STRING] = {'\0'}, arg2[MEDIUM_STRING] = {'\0'}, arg3[MEDIUM_STRING] = {'\0'};
  struct char_data *victim = NULL;
  int i = 0;
  long int amount = 0;

  three_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3));

  if (!*arg1)
  {
    send_to_char(ch, "Your need to specify who you want to award.\r\n");
    return;
  }

  if (!(victim = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD)))
  {
    send_to_char(ch, "There is no one online by that name.\r\n");
    return;
  }

  if (IS_NPC(victim))
  {
    send_to_char(ch, "You can only award players.\r\n");
    return;
  }

  if (!ch->desc || !ch->desc->account || STATE(ch->desc) != CON_PLAYING)
  {
    send_to_char(ch, "You can only award online players who are not in a menu of any sort.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "Please specify what you would like to award:\r\n");
    for (i = 0; i < NUM_AWARD_TYPES; i++)
    {
      send_to_char(ch, "%s\r\n", award_types[i]);
    }
    return;
  }

  for (i = 0; i < NUM_AWARD_TYPES; i++)
  {
    if (is_abbrev(arg2, award_types[i]))
      break;
  }

  if (i >= NUM_AWARD_TYPES)
  {
    send_to_char(ch, "That is not a valid award type.\r\n");
    send_to_char(ch, "Please specify what you would like to award:\r\n");
    for (i = 0; i < NUM_AWARD_TYPES; i++)
    {
      send_to_char(ch, "%s\r\n", award_types[i]);
    }
    return;
  }

  if (!*arg3)
  {
    send_to_char(ch, "Please specify how much you wish to award.\r\n");
    return;
  }

  amount = atol(arg3);

  if (amount <= 0)
  {
    send_to_char(ch, "That is not a valid amount to award.\r\n");
    return;
  }

  switch (i)
  {
  case 0: // experience
    GET_EXP(victim) += amount;
    break;
  case 1: // questpoints
    GET_QUESTPOINTS(victim) += amount;
    break;
  case 2: // accountexperience
    change_account_xp(ch, amount);
    break;
  case 3: // gold
    GET_GOLD(victim) += amount;
    break;
  case 4: // bank gold
    GET_BANK_GOLD(victim) += amount;
    break;
  case 5: // skill points
    GET_TRAINS(victim) += amount;
    break;
  case 6: // feats
    GET_FEAT_POINTS(victim) += amount;
    break;
  case 7: // class feats
    GET_CLASS_FEATS(victim, GET_CLASS(victim)) += amount;
    break;
  case 8: // epic feats
    GET_EPIC_FEAT_POINTS(victim) += amount;
    break;
  case 9: // epic class feats
    GET_EPIC_CLASS_FEATS(victim, GET_CLASS(victim)) += amount;
    break;
  case 10: // ability score boosts
    GET_BOOSTS(victim) += amount;
    break;
  default:
    send_to_char(ch, "That is not a valid award type.\r\n");
    send_to_char(ch, "Please specify what you would like to award:\r\n");
    for (i = 0; i < NUM_AWARD_TYPES; i++)
    {
      send_to_char(ch, "%s\r\n", award_types[i]);
    }
    return;
  }

  send_to_char(ch, "You have increased %s's %s by %ld.\r\n", GET_NAME(victim), award_types[i], amount);
  send_to_char(victim, "%s has increased your %s by %ld.\r\n", CAN_SEE(victim, ch) ? GET_NAME(ch) : "Someone", award_types[i], amount);
  save_char(victim, 0);
}

ACMDU(do_setroomname)
{

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "You must specify a new room name, less than 60 characters.\r\n");
    return;
  }

  if (strlen(argument) > 60)
  {
    send_to_char(ch, "You must specify a new room name, less than 60 characters.\r\n");
    return;
  }

  world[IN_ROOM(ch)].name = strdup(argument);
  add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);

  send_to_char(ch, "You have this room's name to: %s.\r\n", argument);
}

ACMDU(do_setroomdesc)
{

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "You must specify a new room description, less than 600 characters.\r\n");
    return;
  }

  if (strlen(argument) > 600)
  {
    send_to_char(ch, "You must specify a new room description, less than 600 characters.\r\n");
    return;
  }

  char buf[LONG_STRING] = {'\0'};

  snprintf(buf, sizeof(buf), "%s\n", argument);

  world[IN_ROOM(ch)].description = strdup(buf);

  add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);

  send_to_char(ch, "You have this room's description to: %s.\r\n", argument);
}

ACMDU(do_setworldsect)
{

  skip_spaces(&argument);

  int i = 0, j = 0;

  if (!*argument)
  {
    send_to_char(ch, "You must select a sector type from:\r\n");
    for (i = 0; i < NUM_ROOM_SECTORS; i++)
      send_to_char(ch, "%s\r\n", sector_types[i]);
    return;
  }

  char buf[200];
  char arg[200];
  sprintf(arg, "%s", argument);
  for (j = 0; j < strlen(arg); j++)
    arg[j] = tolower(arg[j]);

  for (i = 0; i < NUM_ROOM_SECTORS; i++)
  {
    sprintf(buf, "%s", sector_types[i]);
    for (j = 0; j < strlen(buf); j++)
      buf[j] = tolower(buf[j]);
    if (is_abbrev(arg, buf))
      break;
  }

  if (i >= NUM_ROOM_SECTORS)
  {
    send_to_char(ch, "You must select a sector type from:\r\n");
    for (i = 0; i < NUM_ROOM_SECTORS; i++)
      send_to_char(ch, "%s\r\n", sector_types[i]);
    return;
  }

  SET_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_WORLDMAP);
  world[IN_ROOM(ch)].sector_type = i;

  send_to_char(ch, "You have set this room as a worldmap room using sector type: %s.\r\n", sector_types[i]);

  add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);
}

ACMDU(do_setroomsect)
{

  skip_spaces(&argument);

  int i = 0, j = 0;

  if (!*argument)
  {
    send_to_char(ch, "You must select a sector type from:\r\n");
    for (i = 0; i < NUM_ROOM_SECTORS; i++)
      send_to_char(ch, "%s\r\n", sector_types[i]);
    return;
  }

  char buf[200];
  char arg[200];
  sprintf(arg, "%s", argument);
  for (j = 0; j < strlen(arg); j++)
    arg[j] = tolower(arg[j]);

  for (i = 0; i < NUM_ROOM_SECTORS; i++)
  {
    sprintf(buf, "%s", sector_types[i]);
    for (j = 0; j < strlen(buf); j++)
      buf[j] = tolower(buf[j]);
    if (is_abbrev(arg, buf))
      break;
  }

  if (i >= NUM_ROOM_SECTORS)
  {
    send_to_char(ch, "You must select a sector type from:\r\n");
    for (i = 0; i < NUM_ROOM_SECTORS; i++)
      send_to_char(ch, "%s\r\n", sector_types[i]);
    return;
  }

  world[IN_ROOM(ch)].sector_type = i;

  send_to_char(ch, "You have set this room as a worldmap room using sector type: %s.\r\n", sector_types[i]);

  add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);
}

/* EOF */
