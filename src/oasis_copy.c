/**************************************************************************
 *  File: oasis_copy.c                                 Part of LuminariMUD *
 *  Usage: Oasis OLC copying.                                              *
 *                                                                         *
 * By Levork. Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.        *
 * 2002 Kip Potter [Mythran].                                              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "shop.h"
#include "genshp.h"
#include "genolc.h"
#include "genzon.h"
#include "genwld.h"
#include "oasis.h"
#include "improved-edit.h"
#include "constants.h"
#include "dg_scripts.h"
#include "wilderness.h"
#include "quest.h"
#include "act.h"

/* Local, filescope function prototypes */
/* Utility function for buildwalk */
static room_vnum redit_find_new_vnum(zone_rnum zone);

/***********************************************************
 * This function makes use of the high level OLC functions  *
 * to copy most types of mud objects. The type of data is   *
 * determined by the subcmd variable and the functions are  *
 * looked up in a table.                                    *
 ***********************************************************/
ACMD(do_oasis_copy)
{
  int i, src_vnum, src_rnum, dst_vnum, dst_rnum; //, copy_mode = QMODE_QCOPY;
  char buf1[MAX_INPUT_LENGTH] = {'\0'};
  char buf2[MAX_INPUT_LENGTH] = {'\0'};
  struct descriptor_data *d;

  struct
  {
    int con_type;
    IDXTYPE(*binary_search)
    (IDXTYPE vnum);
    void (*save_func)(struct descriptor_data *d);
    void (*setup_existing)(struct descriptor_data *d, int rnum, int mode);
    const char *command;
    const char *text;
  } oasis_copy_info[] = {
      {CON_REDIT, real_room, redit_save_internally, redit_setup_existing, "rcopy", "room"},
      {CON_OEDIT, real_object, oedit_save_internally, oedit_setup_existing, "ocopy", "object"},
      {CON_MEDIT, real_mobile, medit_save_internally, medit_setup_existing, "mcopy", "mobile"},
      {CON_SEDIT, real_shop, sedit_save_internally, sedit_setup_existing, "scopy", "shop"},
      {CON_TRIGEDIT, real_trigger, trigedit_save, trigedit_setup_existing, "tcopy", "trigger"},
      {CON_QEDIT, real_quest, qedit_save_internally, qedit_setup_existing, "qcopy", "quest"},
      {-1, NULL, NULL, NULL, "\n", "\n"}};

  /* Find the given connection type in the table (passed in subcmd). */
  for (i = 0; *(oasis_copy_info[i].text) != '\n'; i++)
    if (subcmd == oasis_copy_info[i].con_type)
      break;
  /* If not found, we don't support copying that type of data. */
  if (*(oasis_copy_info[i].text) == '\n')
    return;

  /* No copying as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  /* We need two arguments. */
  two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

  /* Both arguments are required and they must be numeric. */
  if (!*buf2 || !is_number(buf1) || !is_number(buf2))
  {
    send_to_char(ch, "Syntax: %s <source vnum> <target vnum>\r\n", oasis_copy_info[i].command);
    return;
  }

  /* We can't copy non-existing data. */
  /* Note: the source data can be in any zone. It's not restricted */
  /* to the builder's designated OLC zone. */
  src_vnum = atoi(buf1);
  src_rnum = (*oasis_copy_info[i].binary_search)(src_vnum);
  if (src_rnum == NOWHERE)
  {
    send_to_char(ch, "The source %s (#%d) does not exist.\r\n", oasis_copy_info[i].text, src_vnum);
    return;
  }

  /* Don't copy if the target already exists. */
  dst_vnum = atoi(buf2);
  dst_rnum = (*oasis_copy_info[i].binary_search)(dst_vnum);
  if (dst_rnum != NOWHERE)
  {
    send_to_char(ch, "The target %s (#%d) already exists.\r\n", oasis_copy_info[i].text, dst_vnum);
    return;
  }

  /* Check that whatever it is isn't already being edited. */
  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) == subcmd)
    {
      if (d->olc && OLC_NUM(d) == dst_vnum)
      {
        send_to_char(ch, "The target %s (#%d) is currently being edited by %s.\r\n",
                     oasis_copy_info[i].text, dst_vnum, GET_NAME(d->character));
        return;
      }
    }
  }

  d = ch->desc;

  /* Give the descriptor an OLC structure. */
  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_oasis_copy: Player already had olc structure.");
    free(d->olc);
  }

  /* Create the OLC structure. */
  CREATE(d->olc, struct oasis_olc_data, 1);

  /* Find the zone. */
  if ((OLC_ZNUM(d) = real_zone_by_thing(dst_vnum)) == NOWHERE)
  {
    send_to_char(ch, "Sorry, there is no zone for the given vnum (#%d)!\r\n", dst_vnum);
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* Make sure the builder is allowed to modify the target zone. */
  if (!can_edit_zone(ch, OLC_ZNUM(d)))
  {
    send_cannot_edit(ch, zone_table[OLC_ZNUM(d)].number);
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* We tell the OLC functions that we want to save to the target vnum. */
  OLC_NUM(d) = dst_vnum;

  /* Perform the copy. */
  send_to_char(ch, "Copying %s: source: #%d, dest: #%d.\r\n", oasis_copy_info[i].text, src_vnum, dst_vnum);

  /* we are sending the vnum of the destination as our "mode" variable, this is an attempt to fix qcopy -zusuk */
  (*oasis_copy_info[i].setup_existing)(d, src_rnum, dst_vnum);
  (*oasis_copy_info[i].save_func)(d);

  /* Currently CLEANUP_ALL should be used for everything. */
  cleanup_olc(d, CLEANUP_ALL);
  send_to_char(ch, "Done.\r\n");
}

/* Commands */
ACMD(do_dig)
{
  char sdir[MAX_INPUT_LENGTH] = {'\0'}, sroom[MAX_INPUT_LENGTH] = {'\0'};
  const char *new_room_name;
  room_vnum rvnum = NOWHERE;
  room_rnum rrnum = NOWHERE;
  zone_rnum zone;
  int dir = 0, rawvnum;
  struct descriptor_data *d = ch->desc; /* will save us some typing */

  /* Grab the room's name (if available). */
  new_room_name = two_arguments(argument, sdir, sizeof(sdir), sroom, sizeof(sroom));
  skip_spaces_c(&new_room_name);

  /* Can't dig if we don't know where to go. */
  if (!*sdir || !*sroom)
  {
    send_to_char(ch, "Format: dig <direction> <room> - to create an exit\r\n"
                     "        dig <direction> -1     - to delete an exit\r\n");
    return;
  }

  

  /* set up some variables */
  rawvnum = atoi(sroom);
  if (rawvnum == -1)
    rvnum = NOWHERE;
  else
    rvnum = (room_vnum)rawvnum;
  rrnum = real_room(rvnum);

  /* wilderness dyanmic room dummy checks */
  if (real_room(IN_ROOM(ch)) != NOWHERE && IS_DYNAMIC(IN_ROOM(ch)))
  {
    send_to_char(ch, "You cannot use the 'dig' command from a dynamic room.\r\n");
    return;
  }
  if (real_room(rrnum) != NOWHERE && IS_DYNAMIC(rrnum))
  {
    send_to_char(ch, "You cannot use the 'dig' command to a dynamic room.\r\n");
    return;
  }

  /* continue setting up some variables */
  dir = search_block(sdir, dirs, FALSE);
  zone = world[IN_ROOM(ch)].zone;

  if (dir < 0)
  {
    dir = search_block(sdir, dirs_short, FALSE);
    if (dir < 0)
    {
      send_to_char(ch, "You cannot create an exit to the '%s'.\r\n", sdir);
      return;
    }
  }
  /* Make sure that the builder has access to the zone he's in. */
  if ((zone == NOWHERE) || !can_edit_zone(ch, zone))
  {
    send_to_char(ch, "You do not have permission to edit this zone.\r\n");
    return;
  }
  /* Lets not allow digging to limbo. After all, it'd just get us more errors
   * on 'show errors.' */
  if (rvnum == 0)
  {
    send_to_char(ch, "The target exists, but you can't dig to limbo!\r\n");
    return;
  }
  /* Target room == -1 removes the exit. */
  if (rvnum == NOTHING)
  {
    if (W_EXIT(IN_ROOM(ch), dir))
    {
      /* free the old pointers, if any */
      if (W_EXIT(IN_ROOM(ch), dir)->general_description)
        free(W_EXIT(IN_ROOM(ch), dir)->general_description);
      if (W_EXIT(IN_ROOM(ch), dir)->keyword)
        free(W_EXIT(IN_ROOM(ch), dir)->keyword);
      free(W_EXIT(IN_ROOM(ch), dir));
      W_EXIT(IN_ROOM(ch), dir) = NULL;
      add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);
      send_to_char(ch, "You remove the exit to the %s.\r\n", dirs[dir]);
      return;
    }
    send_to_char(ch, "There is no exit to the %s.\r\n"
                     "No exit removed.\r\n",
                 dirs[dir]);
    return;
  }
  /* Can't dig in a direction, if it's already a door. */
  if (W_EXIT(IN_ROOM(ch), dir))
  {
    send_to_char(ch, "There already is an exit to the %s.\r\n", dirs[dir]);
    return;
  }

  /* Make sure that the builder has access to the zone he's linking to. */
  zone = real_zone_by_thing(rvnum);
  if (zone == NOWHERE)
  {
    send_to_char(ch, "You cannot link to a non-existing zone!\r\n");
    return;
  }
  if (!can_edit_zone(ch, zone))
  {
    send_to_char(ch, "You do not have permission to edit room #%d.\r\n", rvnum);
    return;
  }
  /* Now we know the builder is allowed to make the link. */
  /* If the room doesn't exist, create it.*/
  if (rrnum == NOWHERE)
  {
    /* Give the descriptor an olc struct. This way we can let
     * redit_save_internally handle the room adding. */
    if (d->olc)
    {
      mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_dig: Player already had olc structure.");
      free(d->olc);
    }
    CREATE(d->olc, struct oasis_olc_data, 1);
    OLC_ZNUM(d) = zone;
    OLC_NUM(d) = rvnum;
    CREATE(OLC_ROOM(d), struct room_data, 1);

    /* Copy the room's name. */
    if (*new_room_name)
      OLC_ROOM(d)->name = strdup(new_room_name);
    else
      OLC_ROOM(d)->name = strdup("An unfinished room");

    /* Copy the room's description.*/
    OLC_ROOM(d)->description = strdup("You are in an unfinished room.\r\n");
    OLC_ROOM(d)->zone = OLC_ZNUM(d);
    OLC_ROOM(d)->number = NOWHERE;

    /* Save the new room to memory. redit_save_internally handles adding the
     * room in the right place, etc. */
    redit_save_internally(d);
    OLC_VAL(d) = 0;

    send_to_char(ch, "New room (%d) created.\r\n", rvnum);
    cleanup_olc(d, CLEANUP_ALL);
    /* Update rrnum to the correct room rnum after adding the room. */
    rrnum = real_room(rvnum);
  }

  /* Now dig. */
  CREATE(W_EXIT(IN_ROOM(ch), dir), struct room_direction_data, 1);
  W_EXIT(IN_ROOM(ch), dir)->general_description = NULL;
  W_EXIT(IN_ROOM(ch), dir)->keyword = NULL;
  W_EXIT(IN_ROOM(ch), dir)->to_room = rrnum;
  add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);

  send_to_char(ch, "You make an exit %s to room %d (%s).\r\n",
               dirs[dir], rvnum, world[rrnum].name);

  /* Check if we can dig from there to here. */
  if (W_EXIT(rrnum, rev_dir[dir]))
    send_to_char(ch, "You cannot dig from %d to here. The target room already has an exit to the %s.\r\n",
                 rvnum, dirs[rev_dir[dir]]);
  else
  {
    CREATE(W_EXIT(rrnum, rev_dir[dir]), struct room_direction_data, 1);
    W_EXIT(rrnum, rev_dir[dir])->general_description = NULL;
    W_EXIT(rrnum, rev_dir[dir])->keyword = NULL;
    W_EXIT(rrnum, rev_dir[dir])->to_room = IN_ROOM(ch);
    add_to_save_list(zone_table[world[rrnum].zone].number, SL_WLD);
  }
}

/* BuildWalk - OasisOLC Extension by D. Tyler Barnes. */

/* For buildwalk. Finds the next free vnum in the zone */
static room_vnum redit_find_new_vnum(zone_rnum zone)
{
  room_vnum vnum;
  room_rnum rnum;

  /* Handle wilderness limits differently. */
  if (ZONE_FLAGGED(zone, ZONE_WILDERNESS))
  {
    vnum = WILD_ROOM_VNUM_START;
  }
  else
  {
    vnum = genolc_zone_bottom(zone);
  }

  rnum = real_room(vnum);

  if (rnum == NOWHERE)
    return vnum;

  for (;;)
  {
    if (vnum > zone_table[zone].top)
      return (NOWHERE);
    if (rnum > top_of_world || world[rnum].number > vnum)
      break;
    rnum++;
    vnum++;
  }
  return (vnum);
}

int buildwalk(struct char_data *ch, int dir)
{
  int new_x = 0, new_y = 0;
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  room_vnum vnum;
  room_rnum rnum;

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_BUILDWALK) &&
      GET_LEVEL(ch) >= LVL_BUILDER)
  {

    get_char_colors(ch);

    /* If this is a wilderness zone, set the coordinates */
    if (ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS))
    {

      new_x = X_LOC(ch);
      new_y = Y_LOC(ch);

      switch (dir)
      {
      case NORTH:
        new_y++;
        break;
      case SOUTH:
        new_y--;
        break;
      case EAST:
        new_x++;
        break;
      case WEST:
        new_x--;
        break;
      default:
        /* Bad direction for wilderness travel.*/
        send_to_char(ch, "Invalid direction for wilderness buildwalk.\r\n");
        return 0;
      }
    }

    if (!can_edit_zone(ch, world[IN_ROOM(ch)].zone))
    {
      send_to_char(ch, "You do not have build permissions in this zone.\r\n");
    }
    else if ((vnum = redit_find_new_vnum(world[IN_ROOM(ch)].zone)) == NOWHERE)
    {
      send_to_char(ch, "No free vnums are available in this zone!\r\n");
    }
    else
    {
      struct descriptor_data *d = ch->desc;
      /* Give the descriptor an olc struct. This way we can let
       * redit_save_internally handle the room adding. */
      if (d->olc)
      {
        mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: buildwalk(): Player already had olc structure.");
        free(d->olc);
      }
      CREATE(d->olc, struct oasis_olc_data, 1);
      OLC_ZNUM(d) = world[IN_ROOM(ch)].zone;
      OLC_NUM(d) = vnum;
      CREATE(OLC_ROOM(d), struct room_data, 1);

      if (GET_BUILDWALK_NAME(ch) == NULL)
        OLC_ROOM(d)->name = strdup("New BuildWalk Room");
      else
        OLC_ROOM(d)->name = strdup(GET_BUILDWALK_NAME(ch));

      if (GET_BUILDWALK_DESC(ch) == NULL)
        snprintf(buf, sizeof(buf), "This unfinished room was created by %s.\r\n", GET_NAME(ch));
      else
        snprintf(buf, sizeof(buf), "%s\r\n", GET_BUILDWALK_DESC(ch));
      OLC_ROOM(d)->description = strdup(buf);
      OLC_ROOM(d)->zone = OLC_ZNUM(d);
      OLC_ROOM(d)->number = NOWHERE;
      OLC_ROOM(d)->sector_type = GET_BUILDWALK_SECTOR(ch);
      OLC_ROOM(d)->room_flags[0] = ch->player_specials->buildwalk_flags[0];
      OLC_ROOM(d)->room_flags[1] = ch->player_specials->buildwalk_flags[1];
      OLC_ROOM(d)->room_flags[2] = ch->player_specials->buildwalk_flags[2];
      OLC_ROOM(d)->room_flags[3] = ch->player_specials->buildwalk_flags[3];

      /* If this is a wilderness zone, set the coordinates */
      if (ZONE_FLAGGED(OLC_ZNUM(d), ZONE_WILDERNESS))
      {
        OLC_ROOM(d)->coords[0] = new_x;
        OLC_ROOM(d)->coords[1] = new_y;
      }

      /* Save the new room to memory. redit_save_internally handles adding the
       * room in the right place, etc. */
      redit_save_internally(d);
      OLC_VAL(d) = 0;

      /* Link rooms */
      rnum = real_room(vnum);

      if (ZONE_FLAGGED(GET_ROOM_ZONE(rnum), ZONE_WILDERNESS))
      {
        /* Wilderness exits default to the 'link' vnum. */
        CREATE(world[rnum].dir_option[NORTH], struct room_direction_data, 1);
        CREATE(world[rnum].dir_option[SOUTH], struct room_direction_data, 1);
        CREATE(world[rnum].dir_option[EAST], struct room_direction_data, 1);
        CREATE(world[rnum].dir_option[WEST], struct room_direction_data, 1);

        world[rnum].dir_option[NORTH]->to_room = real_room(1000000);
        world[rnum].dir_option[SOUTH]->to_room = real_room(1000000);
        world[rnum].dir_option[EAST]->to_room = real_room(1000000);
        world[rnum].dir_option[WEST]->to_room = real_room(1000000);

        send_to_char(ch, "%sWilderness Room #%d created by BuildWalk.%s\r\n", yel, vnum, nrm);
      }
      else
      {

        CREATE(EXIT(ch, dir), struct room_direction_data, 1);
        EXIT(ch, dir)->to_room = rnum;
        CREATE(world[rnum].dir_option[rev_dir[dir]], struct room_direction_data, 1);
        world[rnum].dir_option[rev_dir[dir]]->to_room = IN_ROOM(ch);

        /* Report room creation to user */
        send_to_char(ch, "%sRoom #%d created by BuildWalk.%s\r\n", yel, vnum, nrm);
      }

      cleanup_olc(d, CLEANUP_STRUCTS);

      return (1);
    }
  }

  return (0);
}

ACMDU(do_buildwalk)
{

  char arg1[200], arg2[800], temp[200];
  int i = 0, j = 0, cnt = 0;

  half_chop(argument, arg1, arg2);

  if (!*arg1)
  {
    do_gen_tog(ch, argument, cmd, SCMD_BUILDWALK);
    return;
  }

  if (is_abbrev(arg1, "reset"))
  {
    GET_BUILDWALK_SECTOR(ch) = SECT_INSIDE;
    ch->player_specials->buildwalk_flags[0] = 0;
    ch->player_specials->buildwalk_flags[1] = 0;
    ch->player_specials->buildwalk_flags[2] = 0;
    ch->player_specials->buildwalk_flags[3] = 0;
    GET_BUILDWALK_NAME(ch) = NULL;
    GET_BUILDWALK_DESC(ch) = NULL;
    send_to_char(ch, "All buildwalk extra settings removed.\r\n");
    if (!IS_NPC(ch) && PRF_FLAGGED((ch), PRF_BUILDWALK))
      send_to_char(ch, "Buildwalk still on.\r\n");
    else
      send_to_char(ch, "Buildwalk still turned off.\r\n");
  }
  else if (is_abbrev(arg1, "sector"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Valid room sectors:\r\n");
      for (i = 0; i < NUM_ROOM_SECTORS; i++)
      {
        send_to_char(ch, "-- %s\r\n", sector_types[i]);
      }
      return;
    }
    for (i = 0; i < NUM_ROOM_SECTORS; i++)
    {
      if (is_abbrev(arg2, sector_types[i]))
      {
        send_to_char(ch, "You've set your buildwalk room sector to '%s'.\r\n", sector_types[i]);
        GET_BUILDWALK_SECTOR(ch) = i;
        return;
      }
    }
    send_to_char(ch, "That is not a valid sector type.\r\n");
  }
  else if (is_abbrev(arg1, "flags"))
  {
    if (!*(arg2))
    {
      send_to_char(ch, "Valid room flags:\r\n");
      for (i = 0; i < NUM_ROOM_FLAGS; i++)
      {
        snprintf(temp, sizeof(temp), "%s", room_bits[i]);
        for (j = 0; j < strlen(room_bits[i]); j++)
        {
          temp[j] = LOWER(temp[j]);
          if (temp[j] == '-')
            temp[j] = ' ';
        }
        send_to_char(ch, "-- %s\r\n", temp);
      }
    }
    for (i = 0; i < NUM_ROOM_FLAGS; i++)
    {
      snprintf(temp, sizeof(temp), "%s", room_bits[i]);
      for (j = 0; j < strlen(room_bits[i]); j++)
      {
        temp[j] = LOWER(temp[j]);
        if (temp[j] == '-')
          temp[j] = ' ';
      }
      if (is_abbrev(arg2, temp))
      {
        if (IS_SET_AR(GET_BUILDWALK_FLAGS(ch), i))
        {
          send_to_char(ch, "You've removed buildwalk room flag '%s'.\r\n", room_bits[i]);
        }
        else
        {
          send_to_char(ch, "You've added buildwalk room flag '%s'.\r\n", room_bits[i]);
        }
        TOGGLE_BIT_AR(GET_BUILDWALK_FLAGS(ch), i);
        return;
      }
    }
    send_to_char(ch, "That is not a valid room flag.\r\n");
  }
  else if (is_abbrev(arg1, "name"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Please specify the room name to use on new buildwalk rooms.\r\n");
      return;
    }
    if (strlen(arg2) > MAX_ROOM_NAME)
    {
      send_to_char(ch, "That room name is too long.\r\n");
      return;
    }
    GET_BUILDWALK_NAME(ch) = strdup(arg2);
    send_to_char(ch, "Your buildwalk room name is now '%s'\r\n", arg2);
  }
  else if (is_abbrev(arg1, "desc"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Please specify the room description to use on new buildwalk rooms.\r\n");
      return;
    }
    if (strlen(arg2) > 800)
    {
      send_to_char(ch, "That room description is too long.\r\n");
      return;
    }
    GET_BUILDWALK_DESC(ch) = strdup(arg2);
    send_to_char(ch, "Your buildwalk room description is now '%s'\r\n", arg2);
  }
  else if (is_abbrev(arg1, "settings"))
  {
    send_to_char(ch, "Here are your current buildwalk settings:\r\n");
    send_to_char(ch, "Buildwalk: %s\r\n", IS_SET_AR(PRF_FLAGS(ch), PRF_BUILDWALK) ? "ON" : "OFF");
    send_to_char(ch, "Sector: %s\r\n", sector_types[GET_BUILDWALK_SECTOR(ch)]);
    send_to_char(ch, "Room Flags: ");
    for (i = 0; i < NUM_ROOM_FLAGS; i++)
    {
      if (IS_SET_AR(GET_BUILDWALK_FLAGS(ch), i))
      {
        snprintf(temp, sizeof(temp), "%s", room_bits[i]);
        for (j = 0; j < strlen(room_bits[i]); j++)
        {
          temp[j] = LOWER(temp[j]);
          if (temp[j] == '-')
            temp[j] = ' ';
        }
        if (cnt > 0)
        {
          send_to_char(ch, ", ");
        }
        send_to_char(ch, "%s", temp);
        cnt++;
      }
    }
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Room Name: %s\r\n", GET_BUILDWALK_NAME(ch));
    send_to_char(ch, "Room Desc: %s\r\n", GET_BUILDWALK_DESC(ch));
    send_to_char(ch, "\r\n\r\n");
  }
  else
  {
    send_to_char(ch, "Choose one of the following:\r\n"
                     "buildwalk               : toggle buildwalk on or off\r\n"
                     "buildwalk settings      : show all current buildwalk settings\r\n"
                     "buildwalk reset         : reset/remove all extra buildwalk settings (will not toggle on/off though)\r\n"
                     "buildwalk sector (type) : set the default buildwalk sector type\r\n"
                     "buildwalk flag (flag)   : add/remove a specific room flag for new buildwalk rooms\r\n"
                     "buildwalk name (name)   : set a default room name for new buildwalk rooms\r\n"
                     "buildwalk desc (desc)   : set a default room description for new buildwalk rooms\r\n");
  }
}
