/**************************************************************************
 *  File: dg_olc.c                                     Part of LuminariMUD *
 *  Usage: This source file is used in extending Oasis OLC for trigedit.   *
 *                                                                         *
 *  $Author: Chris Jacobsen/Mark A. Heilpern/egreen/Welcor $               *
 *  $Date: 2004/10/11 12:07:00$                                            *
 *  $Revision: 1.0.14 $                                                    *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "olc/genolc.h"
#include "interpreter.h"
#include "olc/oasis.h"
#include "dg_olc.h"
#include "dg_event.h"
#include "olc/genzon.h" /* for real_zone_by_thing */
#include "constants.h"  /* for the *trig_types */
#include "modify.h"     /* for smash_tilde */

/* local functions */
static void trigedit_disp_menu(struct descriptor_data *d);
static void trigedit_disp_types(struct descriptor_data *d);
static int trigedit_create_index(int znum, const char *type);
static void trigedit_setup_new(struct descriptor_data *d);

static const char **trigedit_types_for_attachment(int attach_type)
{
  switch (attach_type)
  {
  case WLD_TRIGGER:
    return wtrig_types;
  case OBJ_TRIGGER:
    return otrig_types;
  case MOB_TRIGGER:
  default:
    return trig_types;
  }
}

static int trigedit_type_count(int attach_type)
{
  const char **types;
  int count;

  types = trigedit_types_for_attachment(attach_type);
  for (count = 0; strcmp(types[count], "\n") != 0; count++)
    ;

  return count;
}

static void warn_room_trigger_source_overlap(const struct room_data *room)
{
  const struct trig_proto_list *proto_trigger;
  room_rnum room_rnum_value;
  int command_number;

  if (room == NULL || room->zone == NOWHERE || room->zone > top_of_zone_table)
    return;

  room_rnum_value = real_room(room->number);
  if (room_rnum_value == NOWHERE)
    return;

  for (proto_trigger = room->proto_script; proto_trigger != NULL;
       proto_trigger = proto_trigger->next)
  {
    trig_rnum trigger_rnum;

    trigger_rnum = real_trigger(proto_trigger->vnum);
    if (trigger_rnum == NOTHING)
      continue;

    for (command_number = 0; zone_table[room->zone].cmd[command_number].command != 'S';
         command_number++)
    {
      const struct reset_com *command;

      command = &zone_table[room->zone].cmd[command_number];
      if (command->command == 'T' && command->arg1 == WLD_TRIGGER &&
          command->arg2 == (int)trigger_rnum && command->arg3 == (int)room_rnum_value)
      {
        mudlog(BRF, LVL_BUILDER, TRUE,
               "TRIGGER WARNING: Room #%" PRI_IDX " and zone #%d line %d both attach trigger #%d"
               "; keep the redit attachment and remove the redundant zedit command.",
               room->number, zone_table[room->zone].number, command->line, proto_trigger->vnum);
        break;
      }
    }
  }
}

/* Trigedit */
ACMD(do_oasis_trigedit)
{
  int number, real_num;
  struct descriptor_data *d;

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  /* Parse any arguments. */
  skip_spaces_c(&argument);
  if (!*argument || !isdigit(*argument))
  {
    send_to_char(ch, "Specify a trigger VNUM to edit.\r\n");
    return;
  }

  number = atoi(argument);

  /* Check that it isn't already being edited. */
  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) == CON_TRIGEDIT)
    {
      if (d->olc && (int)OLC_NUM(d) == number)
      {
        send_to_char(ch, "That trigger is currently being edited by %s.\r\n",
                     GET_NAME(d->character));
        return;
      }
    }
  }
  d = ch->desc;
  /* Give descriptor an OLC structure. */
  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_oasis_trigedit: Player already had olc structure.");
    free(d->olc);
  }
  CREATE(d->olc, struct oasis_olc_data, 1);

  /* Find the zone. */
  if ((OLC_ZNUM(d) = real_zone_by_thing(number)) == NOWHERE)
  {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* Everyone but IMPLs can only edit zones they have been assigned.*/
  if (!can_edit_zone(ch, OLC_ZNUM(d)))
  {
    send_cannot_edit(ch, zone_table[OLC_ZNUM(d)].number);
    /* Free the OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }
  OLC_NUM(d) = number;

  /* If this is a new trigger, setup a new one, otherwise, setup the a copy of
   * the existing trigger. */
  if ((real_num = (int)real_trigger(number)) == (int)NOTHING)
    trigedit_setup_new(d);
  else
    trigedit_setup_existing(d, real_num, QMODE_NONE);

  trigedit_disp_menu(d);
  STATE(d) = CON_TRIGEDIT;

  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing zone %d [trigger](allowed zone %d)",
         GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

/* Called when a mob or object is being saved to disk, so its script can be
 * saved. */
void script_save_to_disk(FILE *fp, void *item, int type)
{
  struct trig_proto_list *t;

  if (type == MOB_TRIGGER)
    t = ((struct char_data *)item)->proto_script;
  else if (type == OBJ_TRIGGER)
    t = ((struct obj_data *)item)->proto_script;
  else if (type == WLD_TRIGGER)
  {
    warn_room_trigger_source_overlap((const struct room_data *)item);
    t = ((struct room_data *)item)->proto_script;
  }
  else
  {
    log("SYSERR: Invalid type passed to script_save_to_disk()");
    return;
  }

  while (t)
  {
    fprintf(fp, "T %d\n", t->vnum);
    t = t->next;
  }
}

static void trigedit_setup_new(struct descriptor_data *d)
{
  struct trig_data *trig;

  /* Allocate a scratch trigger structure. */
  CREATE(trig, struct trig_data, 1);

  trig->nr = NOWHERE;

  /* Set up some defaults. */
  trig->name = strdup("new trigger");
  trig->trigger_type = MTRIG_GREET;

  /* cmdlist will be a large char string until the trigger is saved */
  CREATE(OLC_STORAGE(d), char, MAX_CMD_LENGTH);
  strncpy(OLC_STORAGE(d), "%echo% This trigger commandlist is not complete!\r\n",
          MAX_CMD_LENGTH - 1);
  trig->narg = 100;

  OLC_TRIG(d) = trig;
  OLC_VAL(d) = 0; /* Has changed flag. (It hasn't so far, we just made it.) */
}

void trigedit_setup_existing(struct descriptor_data *d, int rtrg_num,
                             int mode __attribute__((unused)))
{
  struct trig_data *trig;
  struct cmdlist_element *c;
  /* Allocate a scratch trigger structure. */
  CREATE(trig, struct trig_data, 1);

  trig_data_copy(trig, trig_index[rtrg_num]->proto);

  /* convert cmdlist to a char string */
  c = trig->cmdlist;
  CREATE(OLC_STORAGE(d), char, MAX_CMD_LENGTH);
  OLC_STORAGE(d)[0] = '\0';

  while (c)
  {
    strlcat(OLC_STORAGE(d), c->cmd, MAX_CMD_LENGTH);
    strlcat(OLC_STORAGE(d), "\r\n", MAX_CMD_LENGTH);
    c = c->next;
  }
  /* Now trig->cmdlist is something to pass to the text editor it will be
   * converted back to a real cmdlist_element list later. */

  OLC_TRIG(d) = trig;
  OLC_VAL(d) = 0; /* Has changed flag. (It hasn't so far, we just made it.) */
}

static void trigedit_disp_menu(struct descriptor_data *d)
{
  struct trig_data *trig = OLC_TRIG(d);
  const char *attach_type;
  char trgtypes[MEDIUM_STRING] = {'\0'};

  get_char_colors(d->character);

  if (trig->attach_type == OBJ_TRIGGER)
  {
    attach_type = "Objects";
    sprintbit(GET_TRIG_TYPE(trig), otrig_types, trgtypes, sizeof(trgtypes));
  }
  else if (trig->attach_type == WLD_TRIGGER)
  {
    attach_type = "Rooms";
    sprintbit(GET_TRIG_TYPE(trig), wtrig_types, trgtypes, sizeof(trgtypes));
  }
  else
  {
    attach_type = "Mobiles";
    sprintbit(GET_TRIG_TYPE(trig), trig_types, trgtypes, sizeof(trgtypes));
  }

  clear_screen(d);

  write_to_output(d,
                  "Trigger Editor [%s%d%s]\r\n\r\n"
                  "%s1)%s Name         : %s%s\r\n"
                  "%s2)%s Intended for : %s%s\r\n"
                  "%s3)%s Trigger types: %s%s\r\n"
                  "%s4)%s Numeric Arg  : %s%d\r\n"
                  "%s5)%s Arguments    : %s%s\r\n"
                  "%s6)%s Commands:\r\n%s%s\r\n"
                  "%sW%s) Copy Trigger\r\n"
                  "%sQ)%s Quit\r\n"
                  "Enter Choice :",

                  grn, OLC_NUM(d), nrm,                              /* vnum on the title line */
                  grn, nrm, yel, GET_TRIG_NAME(trig),                /* name                   */
                  grn, nrm, yel, attach_type,                        /* attach type            */
                  grn, nrm, yel, trgtypes,                           /* greet/drop/etc         */
                  grn, nrm, yel, trig->narg,                         /* numeric arg            */
                  grn, nrm, yel, trig->arglist ? trig->arglist : "", /* strict arg             */
                  grn, nrm, cyn, OLC_STORAGE(d),                     /* the command list       */
                  grn, nrm, grn, nrm);                               /* quit colors            */

  OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
}

static void trigedit_disp_types(struct descriptor_data *d)
{
  int i, columns = 0, type_count;
  const char **types;
  char bitbuf[MAX_STRING_LENGTH] = {'\0'};

  types = trigedit_types_for_attachment(OLC_TRIG(d)->attach_type);
  type_count = trigedit_type_count(OLC_TRIG(d)->attach_type);

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; i < type_count; i++)
  {
    write_to_output(d, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, types[i],
                    !(++columns % 2) ? "\r\n" : "");
  }
  sprintbit(GET_TRIG_TYPE(OLC_TRIG(d)), types, bitbuf, sizeof(bitbuf));
  write_to_output(d, "\r\nCurrent types : %s%s%s\r\nEnter type (0 to quit) : ", cyn, bitbuf, nrm);
}

void trigedit_parse(struct descriptor_data *d, char *arg)
{
  int i = 0;

  switch (OLC_MODE(d))
  {
  case TRIGEDIT_MAIN_MENU:
    switch (tolower(*arg))
    {
    case 'q':
      if (OLC_VAL(d))
      { /* Anything been changed? */
        if (!GET_TRIG_TYPE(OLC_TRIG(d)))
        {
          write_to_output(d, "Invalid Trigger Type! Answer a to abort quit!\r\n");
        }
        write_to_output(d, "Do you wish to save your changes? : ");
        OLC_MODE(d) = TRIGEDIT_CONFIRM_SAVESTRING;
      }
      else
        cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      OLC_MODE(d) = TRIGEDIT_NAME;
      write_to_output(d, "Name: ");
      break;
    case '2':
      OLC_MODE(d) = TRIGEDIT_INTENDED;
      write_to_output(d, "0: Mobiles, 1: Objects, 2: Rooms: ");
      break;
    case '3':
      OLC_MODE(d) = TRIGEDIT_TYPES;
      trigedit_disp_types(d);
      break;
    case '4':
      OLC_MODE(d) = TRIGEDIT_NARG;
      write_to_output(d, "Numeric argument: ");
      break;
    case '5':
      OLC_MODE(d) = TRIGEDIT_ARGUMENT;
      write_to_output(d, "Argument: ");
      break;
    case '6':
      OLC_MODE(d) = TRIGEDIT_COMMANDS;
      write_to_output(d, "Enter trigger commands: (/s saves /h for help)\r\n\r\n");
      d->backstr = NULL;
      if (OLC_STORAGE(d))
      {
        write_to_output(d, "%s", OLC_STORAGE(d));
        d->backstr = strdup(OLC_STORAGE(d));
      }
      d->str = &OLC_STORAGE(d);
      d->max_str = MAX_CMD_LENGTH;
      d->mail_to = 0;
      OLC_VAL(d) = 1;

      break;
    case 'w':
    case 'W':
      write_to_output(d, "Copy what trigger? ");
      OLC_MODE(d) = TRIGEDIT_COPY;
      break;
    default:
      trigedit_disp_menu(d);
      return;
    }
    return;

  case TRIGEDIT_CONFIRM_SAVESTRING:
    switch (tolower(*arg))
    {
    case 'y':
      trigedit_save(d);
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s edits trigger %d",
             GET_NAME(d->character), OLC_NUM(d));
      /* fall through */
    case 'n':
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'a': /* abort quitting */
      break;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Do you wish to save your changes? : ");
      return;
    }
    break;

  case TRIGEDIT_NAME:
    smash_tilde(arg);
    if (OLC_TRIG(d)->name)
      free(OLC_TRIG(d)->name);
    OLC_TRIG(d)->name = strdup((arg && *arg) ? arg : "undefined");
    OLC_VAL(d)
    ++;
    break;

  case TRIGEDIT_INTENDED:
    if ((atoi(arg) >= MOB_TRIGGER) || (atoi(arg) <= WLD_TRIGGER))
      OLC_TRIG(d)->attach_type = atoi(arg);
    OLC_VAL(d)
    ++;
    break;

  case TRIGEDIT_NARG:
    OLC_TRIG(d)->narg = LIMIT(atoi(arg), 0, 100);
    OLC_VAL(d)
    ++;
    break;

  case TRIGEDIT_ARGUMENT:
    smash_tilde(arg);
    OLC_TRIG(d)->arglist = (*arg ? strdup(arg) : NULL);
    OLC_VAL(d)
    ++;
    break;

  case TRIGEDIT_TYPES:
    if ((i = atoi(arg)) == 0)
      break;
    else if (i >= 1 && i <= trigedit_type_count(OLC_TRIG(d)->attach_type))
      TOGGLE_BIT((GET_TRIG_TYPE(OLC_TRIG(d))), 1 << (i - 1));
    OLC_VAL(d)
    ++;
    trigedit_disp_types(d);
    return;

  case TRIGEDIT_COPY:
    if ((i = (int)real_trigger(atoi(arg))) != (int)NOWHERE)
    {
      trigedit_setup_existing(d, i, QMODE_QCOPY);
    }
    else
      write_to_output(d, "That trigger does not exist.\r\n");
    break;

  case TRIGEDIT_COMMANDS:
    break;
  }

  OLC_MODE(d) = TRIGEDIT_MAIN_MENU;
  trigedit_disp_menu(d);
}

/* save the zone's triggers to internal memory and to disk */
void trigedit_save(struct descriptor_data *d)
{
  int i;
  trig_rnum rnum;
  int found = 0;
  char *s;
  trig_data *proto;
  trig_data *trig = OLC_TRIG(d);
  trig_data *live_trig;
  struct cmdlist_element *cmd, *next_cmd;
  struct index_data **new_index;
  struct descriptor_data *dsc;
  FILE *trig_file;
  int zone, top;
  char buf[MAX_CMD_LENGTH];
  char bitBuf[MAX_INPUT_LENGTH] = {'\0'};
  char fname[MAX_INPUT_LENGTH] = {'\0'};

  if ((rnum = real_trigger(OLC_NUM(d))) != NOTHING)
  {
    proto = trig_index[rnum]->proto;
    for (cmd = proto->cmdlist; cmd; cmd = next_cmd)
    {
      next_cmd = cmd->next;
      if (cmd->cmd)
        free(cmd->cmd);
      free(cmd);
    }

    free(proto->arglist);
    free(proto->name);

    /* Recompile the command list from the new script */
    s = OLC_STORAGE(d);

    CREATE(trig->cmdlist, struct cmdlist_element, 1);
    if (s)
    {
      char *t = strtok(s, "\n\r"); /* strtok returns NULL if s is "\r\n" */
      if (t)
        trig->cmdlist->cmd = strdup(t);
      else
        trig->cmdlist->cmd = strdup("* No script");

      cmd = trig->cmdlist;
      while ((s = strtok(NULL, "\n\r")))
      {
        CREATE(cmd->next, struct cmdlist_element, 1);
        cmd = cmd->next;
        cmd->cmd = strdup(s);
      }
    }
    else
      trig->cmdlist->cmd = strdup("* No Script");

    /* make the prorotype look like what we have */
    trig_data_copy(proto, trig);

    /* go through the mud and replace existing triggers         */
    live_trig = trigger_list;
    while (live_trig)
    {
      if (GET_TRIG_RNUM(live_trig) == rnum)
      {
        if (live_trig->arglist)
        {
          free(live_trig->arglist);
          live_trig->arglist = NULL;
        }
        if (live_trig->name)
        {
          free(live_trig->name);
          live_trig->name = NULL;
        }

        if (proto->arglist)
          live_trig->arglist = strdup(proto->arglist);
        if (proto->name)
          live_trig->name = strdup(proto->name);

        /* anything could have happened so we don't want to keep these */
        if (GET_TRIG_WAIT(live_trig))
        {
          event_cancel(GET_TRIG_WAIT(live_trig));
          GET_TRIG_WAIT(live_trig) = NULL;
        }
        if (live_trig->var_list)
        {
          free_varlist(live_trig->var_list);
          live_trig->var_list = NULL;
        }

        live_trig->cmdlist = proto->cmdlist;
        live_trig->curr_state = live_trig->cmdlist;
        live_trig->trigger_type = proto->trigger_type;
        live_trig->attach_type = proto->attach_type;
        live_trig->narg = proto->narg;
        live_trig->data_type = proto->data_type;
        live_trig->depth = 0;
      }

      live_trig = live_trig->next_in_world;
    }
  }
  else
  {
    /* this is a new trigger */
    CREATE(new_index, struct index_data *, top_of_trigt + 2);

    /* Recompile the command list from the new script */

    s = OLC_STORAGE(d);

    CREATE(trig->cmdlist, struct cmdlist_element, 1);
    if (s)
    {
      /* strtok returns NULL if s is "\r\n" */
      char *t = strtok(s, "\n\r");
      trig->cmdlist->cmd = strdup(t ? t : "* No script");
      cmd = trig->cmdlist;

      while ((s = strtok(NULL, "\n\r")))
      {
        CREATE(cmd->next, struct cmdlist_element, 1);
        cmd = cmd->next;
        cmd->cmd = strdup(s);
      }
    }
    else
      trig->cmdlist->cmd = strdup("* No Script");

    for (i = 0; i < top_of_trigt; i++)
    {
      if (!found)
      {
        if (trig_index[i]->vnum > OLC_NUM(d))
        {
          found = TRUE;
          rnum = i;

          CREATE(new_index[rnum], struct index_data, 1);
          GET_TRIG_RNUM(OLC_TRIG(d)) = rnum;
          new_index[rnum]->vnum = OLC_NUM(d);
          new_index[rnum]->number = 0;
          new_index[rnum]->func = NULL;
          CREATE(proto, struct trig_data, 1);
          new_index[rnum]->proto = proto;
          trig_data_copy(proto, trig);

          new_index[rnum + 1] = trig_index[rnum];

          proto = trig_index[rnum]->proto;
          proto->nr = rnum + 1;
        }
        else
        {
          new_index[i] = trig_index[i];
        }
      }
      else
      {
        new_index[i + 1] = trig_index[i];
        proto = trig_index[i]->proto;
        proto->nr = i + 1;
      }
    }

    if (!found)
    {
      rnum = i;
      CREATE(new_index[rnum], struct index_data, 1);
      GET_TRIG_RNUM(OLC_TRIG(d)) = rnum;
      new_index[rnum]->vnum = OLC_NUM(d);
      new_index[rnum]->number = 0;
      new_index[rnum]->func = NULL;

      CREATE(proto, struct trig_data, 1);
      new_index[rnum]->proto = proto;
      trig_data_copy(proto, trig);
    }

    free(trig_index);

    trig_index = new_index;
    top_of_trigt++;

    /* HERE IT HAS TO GO THROUGH AND FIX ALL SCRIPTS/TRIGS OF HIGHER RNUM */
    for (live_trig = trigger_list; live_trig; live_trig = live_trig->next_in_world)
      GET_TRIG_RNUM(live_trig) +=
          (GET_TRIG_RNUM(live_trig) != NOTHING && GET_TRIG_RNUM(live_trig) > rnum);

    /* Update other trigs being edited. */
    for (dsc = descriptor_list; dsc; dsc = dsc->next)
      if (STATE(dsc) == CON_TRIGEDIT)
        if (GET_TRIG_RNUM(OLC_TRIG(dsc)) >= rnum)
          GET_TRIG_RNUM(OLC_TRIG(dsc))
    ++;
  }

  /* now write the trigger out to disk, along with the rest of the triggers for
   * this zone. We write this to disk NOW instead of letting the builder have
   * control because if we lose this after having assigned a new trigger to an
   * item, we will get SYSERR's upton reboot that could make things hard to
   * debug. */
  zone = zone_table[OLC_ZNUM(d)].number;
  top = zone_table[OLC_ZNUM(d)].top;

#ifdef CIRCLE_MAC
  snprintf(fname, sizeof(fname), "%s:%i.new", TRG_PREFIX, zone);
#else
  snprintf(fname, sizeof(fname), "%s/%i.new", TRG_PREFIX, zone);
#endif

  if (!(trig_file = fopen_restricted(fname, "w")))
  {
    mudlog(BRF, MAX(LVL_STAFF, GET_INVIS_LEV(d->character)), TRUE,
           "SYSERR: OLC: Can't open trig file \"%s\"", fname);
    return;
  }

  for (i = zone_table[OLC_ZNUM(d)].bot; i <= top; i++)
  {
    if ((rnum = real_trigger(i)) != NOTHING)
    {
      trig = trig_index[rnum]->proto;

      if (fprintf(trig_file, "#%d\n", i) < 0)
      {
        mudlog(BRF, MAX(LVL_STAFF, GET_INVIS_LEV(d->character)), TRUE,
               "SYSERR: OLC: Can't write trig file!");
        fclose(trig_file);
        return;
      }
      sprintascii(bitBuf, GET_TRIG_TYPE(trig));
      fprintf(trig_file,
              "%s%c\n"
              "%d %s %d\n"
              "%s%c\n",
              (GET_TRIG_NAME(trig)) ? (GET_TRIG_NAME(trig)) : "unknown trigger", STRING_TERMINATOR,
              trig->attach_type, *bitBuf ? bitBuf : "0", GET_TRIG_NARG(trig),
              GET_TRIG_ARG(trig) ? GET_TRIG_ARG(trig) : "", STRING_TERMINATOR);

      /* Build the text for the script */
      strlcpy(buf, "", sizeof(buf));
      for (cmd = trig->cmdlist; cmd; cmd = cmd->next)
      {
        strlcat(buf, cmd->cmd, sizeof(buf));
        strlcat(buf, "\n", sizeof(buf));
      }

      if (!buf[0])
        strlcpy(buf, "* Empty script", sizeof(buf));

      fprintf(trig_file, "%s%c\n", buf, STRING_TERMINATOR);
      *buf = '\0';
    }
  }

  fprintf(trig_file, "$%c\n", STRING_TERMINATOR);

#ifdef CIRCLE_MAC
  snprintf(buf, sizeof(buf), "%s:%d.trg", TRG_PREFIX, zone);
#else
  snprintf(buf, sizeof(buf), "%s/%d.trg", TRG_PREFIX, zone);
#endif

  if (!finish_file_save(trig_file, fname, buf))
  {
    write_to_output(d, "Trigger save failed; the previous file is unchanged.\r\n");
    return;
  }

  if (!trigedit_create_index(zone, "trg"))
  {
    write_to_output(d, "Trigger data saved, but the index update failed.\r\n");
    return;
  }
  write_to_output(d, "Trigger saved to disk.\r\n");
}

static int trigedit_create_index(int znum, const char *type)
{
  FILE *newfile, *oldfile;
  char new_name[128], old_name[128];
  const char *prefix;
  char buf[MAX_STRING_LENGTH] = {'\0'}, buf1[MAX_STRING_LENGTH] = {'\0'};
  int num, found = FALSE;

  prefix = TRG_PREFIX;

  snprintf(old_name, sizeof(old_name), "%s/index", prefix);
  snprintf(new_name, sizeof(new_name), "%s/newindex", prefix);

  if (!(oldfile = fopen(old_name, "r")))
  {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: DG_OLC: Failed to open %s", old_name);
    return FALSE;
  }
  else if (!(newfile = fopen_restricted(new_name, "w")))
  {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: DG_OLC: Failed to open %s", new_name);
    fclose(oldfile);
    return FALSE;
  }

  /* Index contents must be in order: search through the old file for the right
   * place, insert the new file, then copy the rest over. */
  snprintf(buf1, sizeof(buf1), "%d.%s", znum, type);
  while (get_line(oldfile, buf))
  {
    if (*buf == '$')
    {
      fprintf(newfile, "%s\n$\n", (!found ? buf1 : ""));
      break;
    }
    else if (!found)
    {
      if (sscanf(buf, "%d", &num) != 1)
      {
        mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: Invalid trigger index entry: %s", buf);
        continue;
      }
      if (num == znum)
        found = TRUE;
      else if (num > znum)
      {
        found = TRUE;
        fprintf(newfile, "%s\n", buf1);
      }
    }
    fprintf(newfile, "%s\n", buf);
  }

  fclose(oldfile);

  if (!finish_file_save(newfile, new_name, old_name))
    return FALSE;
  return TRUE;
}

void dg_olc_script_copy(struct descriptor_data *d)
{
  struct trig_proto_list *origscript, *editscript;
  struct trig_proto_list *temp, *next_temp;

  /* Free any existing OLC_SCRIPT list to prevent memory leaks */
  for (temp = OLC_SCRIPT(d); temp; temp = next_temp)
  {
    next_temp = temp->next;
    free(temp);
  }
  OLC_SCRIPT(d) = NULL;

  if (OLC_ITEM_TYPE(d) == MOB_TRIGGER)
    origscript = OLC_MOB(d)->proto_script;
  else if (OLC_ITEM_TYPE(d) == OBJ_TRIGGER)
    origscript = OLC_OBJ(d)->proto_script;
  else
    origscript = OLC_ROOM(d)->proto_script;

  if (origscript)
  {
    CREATE(editscript, struct trig_proto_list, 1);
    OLC_SCRIPT(d) = editscript;

    while (origscript)
    {
      editscript->vnum = origscript->vnum;
      origscript = origscript->next;
      if (origscript)
        CREATE(editscript->next, struct trig_proto_list, 1);
      editscript = editscript->next;
    }
  }
  else
    OLC_SCRIPT(d) = NULL;
}

void dg_script_menu(struct descriptor_data *d)
{
  struct trig_proto_list *editscript;
  int i = 0;

  /* make sure our input parser gets used */
  OLC_MODE(d) = OLC_SCRIPT_EDIT;
  OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;

  clear_screen(d);
  write_to_output(d, "     Triggers Attached:\r\n");

  editscript = OLC_SCRIPT(d);

  while (editscript)
  {
    write_to_output(d, "     %2d) [%s%d%s] %s%s%s", ++i, cyn, editscript->vnum, nrm, cyn,
                    trig_index[real_trigger(editscript->vnum)]->proto->name, nrm);
    if (trig_index[real_trigger(editscript->vnum)]->proto->attach_type != OLC_ITEM_TYPE(d))
      write_to_output(d, "   %s** Mis-matched Trigger Type **%s\r\n", grn, nrm);
    else
      write_to_output(d, "\r\n");

    editscript = editscript->next;
  }
  if (i == 0)
    write_to_output(d, "     <none>\r\n");

  write_to_output(d,
                  "\r\n"
                  " %sN%s)  Attach trigger\r\n"
                  " %sX%s)  Detach trigger\r\n"
                  " %sQ%s)  Quit\r\n\r\n"
                  "     Enter choice :",
                  grn, nrm, grn, nrm, grn, nrm);
}

int dg_script_edit_parse(struct descriptor_data *d, char *arg)
{
  struct trig_proto_list *trig, *currtrig;
  int count, pos, vnum;

  switch (OLC_SCRIPT_EDIT_MODE(d))
  {
  case SCRIPT_MAIN_MENU:
    switch (tolower(*arg))
    {
    case 'q':
      /* This was buggy. First we created a copy of a thing, but maintained
       * pointers to scripts, then if we altered the scripts, we freed the
       * pointers and added new ones to the OLC_THING. If we then choose NOT
       * to save the changes, the pointers in the original pointed to
       * garbage. If we saved changes the pointers were updated correctly.
       * Solution: Here we just point the working copies to the new
       * proto_scripts. We only update the original when choosing to save
       * internally, then free the unused memory there. -Welcor
       * Thanks to Jeremy Stanley and Torgny Bjers for the bug report.
       * After updating to OasisOLC 2.0.3 I discovered some malfunctions
       * in this code, so I restructured it a bit. Now things work like
       * this: OLC_SCRIPT(d) is assigned a copy of the edited things'
       * proto_script. OLC_OBJ(d), etc.. are initalized with proto_script =
       * NULL; On save, the saved copy is updated with OLC_SCRIPT(d) as new
       * proto_script (freeing the old one). On quit/nosave, OLC_SCRIPT is
       * free()'d, and the prototype not touched. */
      return 0;
    case 'n':
      write_to_output(d, "\r\nPlease enter position, vnum   (ex: 1, 200):");
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_NEW_TRIGGER;
      break;
    case 'x':
      write_to_output(d, "     Which entry should be deleted?  0 to abort :");
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_DEL_TRIGGER;
      break;
    default:
      dg_script_menu(d);
      break;
    }
    return 1;

  case SCRIPT_NEW_TRIGGER:
    pos = 0;
    vnum = -1;
    count = sscanf(arg, "%d, %d", &pos, &vnum);
    if (count == 1)
    {
      vnum = pos;
      pos = 999;
    }
    else if (count != 2)
    {
      write_to_output(d, "Please enter position, vnum   (ex: 1, 200):");
      return 1;
    }

    if (pos <= 0)
      break; /* this aborts a new trigger entry */

    if (vnum == 0)
      break; /* this aborts a new trigger entry */

    if (real_trigger(vnum) == NOTHING)
    {
      write_to_output(d, "Invalid Trigger VNUM!\r\n"
                         "Please enter position, vnum   (ex: 1, 200):");
      return 1;
    }

    if (OLC_ITEM_TYPE(d) == WLD_TRIGGER)
    {
      for (currtrig = OLC_SCRIPT(d); currtrig != NULL; currtrig = currtrig->next)
      {
        if (currtrig->vnum == vnum)
        {
          write_to_output(d, "That trigger is already attached to this room.\r\n");
          return 1;
        }
      }
    }

    /* add the new info in position */
    currtrig = OLC_SCRIPT(d);
    CREATE(trig, struct trig_proto_list, 1);
    trig->vnum = vnum;

    if (pos == 1 || !currtrig)
    {
      trig->next = OLC_SCRIPT(d);
      OLC_SCRIPT(d) = trig;
    }
    else
    {
      while (currtrig->next && --pos)
      {
        currtrig = currtrig->next;
      }
      trig->next = currtrig->next;
      currtrig->next = trig;
    }
    OLC_VAL(d)
    ++;
    break;

  case SCRIPT_DEL_TRIGGER:
    pos = atoi(arg);
    if (pos <= 0)
      break;

    if (pos == 1 && OLC_SCRIPT(d))
    {
      OLC_VAL(d)
      ++;
      currtrig = OLC_SCRIPT(d);
      OLC_SCRIPT(d) = currtrig->next;
      free(currtrig);
      break;
    }

    pos--;
    currtrig = OLC_SCRIPT(d);
    while (--pos && currtrig)
      currtrig = currtrig->next;
    /* now curtrig points one before the target */
    if (currtrig && currtrig->next)
    {
      OLC_VAL(d)
      ++;
      trig = currtrig->next;
      currtrig->next = trig->next;
      free(trig);
    }
    break;
  }

  dg_script_menu(d);
  return 1;
}

void trigedit_string_cleanup(struct descriptor_data *d, int terminator __attribute__((unused)))
{
  switch (OLC_MODE(d))
  {
  case TRIGEDIT_COMMANDS:
    trigedit_disp_menu(d);
    break;
  }
}

enum format_block_type
{
  FORMAT_BLOCK_IF,
  FORMAT_BLOCK_WHILE,
  FORMAT_BLOCK_SWITCH
};

enum format_control_type
{
  FORMAT_CONTROL_NONE,
  FORMAT_CONTROL_IF,
  FORMAT_CONTROL_ELSEIF,
  FORMAT_CONTROL_ELSE,
  FORMAT_CONTROL_END,
  FORMAT_CONTROL_WHILE,
  FORMAT_CONTROL_DONE,
  FORMAT_CONTROL_SWITCH,
  FORMAT_CONTROL_CASE,
  FORMAT_CONTROL_DEFAULT,
  FORMAT_CONTROL_BREAK
};

struct format_block
{
  enum format_block_type type;
  int base_indent;
  int opening_line;
  bool else_seen;
  bool default_seen;
};

#define FORMAT_SCRIPT_MAX_NESTING 200

static bool format_token_equals(const char *token, size_t token_length, const char *expected)
{
  return strlen(expected) == token_length && !strn_cmp(token, expected, token_length);
}

static enum format_control_type format_control_command(char **line, char **argument)
{
  char *command, *end;
  size_t command_length;

  skip_spaces(line);
  command = *line;
  end = command;
  while (*end && !isspace((unsigned char)*end))
    end++;
  command_length = (size_t)(end - command);
  *argument = end;
  skip_spaces(argument);

  if (format_token_equals(command, command_length, "if"))
    return FORMAT_CONTROL_IF;
  if (format_token_equals(command, command_length, "elseif"))
    return FORMAT_CONTROL_ELSEIF;
  if (format_token_equals(command, command_length, "else"))
    return FORMAT_CONTROL_ELSE;
  if (format_token_equals(command, command_length, "end"))
    return FORMAT_CONTROL_END;
  if (format_token_equals(command, command_length, "while"))
    return FORMAT_CONTROL_WHILE;
  if (format_token_equals(command, command_length, "done"))
    return FORMAT_CONTROL_DONE;
  if (format_token_equals(command, command_length, "switch"))
    return FORMAT_CONTROL_SWITCH;
  if (format_token_equals(command, command_length, "case"))
    return FORMAT_CONTROL_CASE;
  if (format_token_equals(command, command_length, "default"))
    return FORMAT_CONTROL_DEFAULT;
  if (format_token_equals(command, command_length, "break"))
    return FORMAT_CONTROL_BREAK;
  return FORMAT_CONTROL_NONE;
}

static bool format_control_requires_argument(enum format_control_type control)
{
  return control == FORMAT_CONTROL_IF || control == FORMAT_CONTROL_ELSEIF ||
         control == FORMAT_CONTROL_WHILE || control == FORMAT_CONTROL_SWITCH ||
         control == FORMAT_CONTROL_CASE;
}

static bool validate_script_structure(const char *source, char *error, size_t error_size)
{
  char *copy, *line, *argument;
  struct format_block blocks[FORMAT_SCRIPT_MAX_NESTING];
  enum format_control_type control;
  int block_top = -1, line_number = 0, scan;

  if (source == NULL || error == NULL || error_size == 0)
    return FALSE;
  *error = '\0';

  copy = strdup(source);
  if (copy == NULL)
  {
    strlcpy(error, "Out of memory while validating the script.", error_size);
    return FALSE;
  }

  for (line = strtok(copy, "\n\r"); line; line = strtok(NULL, "\n\r"))
  {
    line_number++;
    control = format_control_command(&line, &argument);

    if (format_control_requires_argument(control) && !*argument)
    {
      snprintf(error, error_size, "Line %d: control command requires an argument.", line_number);
      free(copy);
      return FALSE;
    }

    switch (control)
    {
    case FORMAT_CONTROL_IF:
    case FORMAT_CONTROL_WHILE:
    case FORMAT_CONTROL_SWITCH:
      if (block_top + 1 >= FORMAT_SCRIPT_MAX_NESTING)
      {
        snprintf(error, error_size, "Line %d: script nesting is too deep.", line_number);
        free(copy);
        return FALSE;
      }
      block_top++;
      blocks[block_top].type = control == FORMAT_CONTROL_IF      ? FORMAT_BLOCK_IF
                               : control == FORMAT_CONTROL_WHILE ? FORMAT_BLOCK_WHILE
                                                                 : FORMAT_BLOCK_SWITCH;
      blocks[block_top].base_indent = 0;
      blocks[block_top].opening_line = line_number;
      blocks[block_top].else_seen = false;
      blocks[block_top].default_seen = false;
      break;
    case FORMAT_CONTROL_ELSEIF:
      if (block_top < 0 || blocks[block_top].type == FORMAT_BLOCK_WHILE)
      {
        snprintf(error, error_size, "Line %d: 'elseif' without a matching 'if'.", line_number);
        free(copy);
        return FALSE;
      }
      if (blocks[block_top].type != FORMAT_BLOCK_IF || blocks[block_top].else_seen)
      {
        snprintf(error, error_size, "Line %d: 'elseif' is not valid after 'else'.", line_number);
        free(copy);
        return FALSE;
      }
      break;
    case FORMAT_CONTROL_ELSE:
      if (block_top < 0 || blocks[block_top].type != FORMAT_BLOCK_IF)
      {
        snprintf(error, error_size, "Line %d: 'else' without a matching 'if'.", line_number);
        free(copy);
        return FALSE;
      }
      if (blocks[block_top].else_seen)
      {
        snprintf(error, error_size, "Line %d: duplicate 'else'.", line_number);
        free(copy);
        return FALSE;
      }
      blocks[block_top].else_seen = true;
      break;
    case FORMAT_CONTROL_END:
      if (block_top < 0 || blocks[block_top].type == FORMAT_BLOCK_WHILE)
      {
        snprintf(error, error_size, "Line %d: unmatched 'end'.", line_number);
        free(copy);
        return FALSE;
      }
      block_top--;
      break;
    case FORMAT_CONTROL_DONE:
      if (block_top < 0 || blocks[block_top].type != FORMAT_BLOCK_WHILE)
      {
        snprintf(error, error_size, "Line %d: unmatched 'done'.", line_number);
        free(copy);
        return FALSE;
      }
      block_top--;
      break;
    case FORMAT_CONTROL_CASE:
    case FORMAT_CONTROL_DEFAULT:
      if (block_top < 0 || blocks[block_top].type != FORMAT_BLOCK_SWITCH)
      {
        snprintf(error, error_size, "Line %d: 'case/default' outside a 'switch'.", line_number);
        free(copy);
        return FALSE;
      }
      if (control == FORMAT_CONTROL_CASE && blocks[block_top].default_seen)
      {
        snprintf(error, error_size, "Line %d: 'case' cannot follow 'default'.", line_number);
        free(copy);
        return FALSE;
      }
      if (control == FORMAT_CONTROL_DEFAULT)
      {
        if (blocks[block_top].default_seen)
        {
          snprintf(error, error_size, "Line %d: duplicate 'default'.", line_number);
          free(copy);
          return FALSE;
        }
        blocks[block_top].default_seen = true;
      }
      break;
    case FORMAT_CONTROL_BREAK:
      for (scan = block_top; scan >= 0; scan--)
        if (blocks[scan].type == FORMAT_BLOCK_WHILE || blocks[scan].type == FORMAT_BLOCK_SWITCH)
          break;
      if (scan < 0)
      {
        snprintf(error, error_size, "Line %d: 'break' outside a 'while' or 'switch'.", line_number);
        free(copy);
        return FALSE;
      }
      break;
    case FORMAT_CONTROL_NONE:
      break;
    }
  }

  if (block_top >= 0)
  {
    const char *block_name = blocks[block_top].type == FORMAT_BLOCK_IF      ? "if"
                             : blocks[block_top].type == FORMAT_BLOCK_WHILE ? "while"
                                                                            : "switch";
    snprintf(error, error_size, "Line %d: block '%s' was never closed.",
             blocks[block_top].opening_line, block_name);
    free(copy);
    return FALSE;
  }

  free(copy);
  return TRUE;
}

bool dg_format_script_text(const char *source, size_t max_length, char **formatted, char *error,
                           size_t error_size)
{
  char nsc[MAX_CMD_LENGTH], *copy, *text, *argument, line[READ_SIZE];
  struct format_block blocks[FORMAT_SCRIPT_MAX_NESTING];
  enum format_control_type control;
  size_t len = 0, nlen, llen;
  int block_top = -1, indent = 0, line_indent, line_number = 0, ret;

  if (formatted == NULL || error == NULL || error_size == 0)
    return FALSE;
  *formatted = NULL;
  *error = '\0';

  if (source == NULL || max_length == 0)
  {
    strlcpy(error, "The script or output limit is invalid.", error_size);
    return FALSE;
  }
  if (!validate_script_structure(source, error, error_size))
    return FALSE;

  copy = strdup(source);
  if (copy == NULL)
  {
    strlcpy(error, "Out of memory while formatting the script.", error_size);
    return FALSE;
  }
  *nsc = '\0';

  for (text = strtok(copy, "\n\r"); text; text = strtok(NULL, "\n\r"))
  {
    line_number++;
    control = format_control_command(&text, &argument);
    line_indent = indent;

    if (control == FORMAT_CONTROL_END || control == FORMAT_CONTROL_DONE)
    {
      line_indent = blocks[block_top].base_indent;
      indent = line_indent;
      block_top--;
    }
    else if (control == FORMAT_CONTROL_ELSE || control == FORMAT_CONTROL_ELSEIF)
    {
      line_indent = blocks[block_top].base_indent;
      indent = line_indent + 1;
    }
    else if (control == FORMAT_CONTROL_CASE || control == FORMAT_CONTROL_DEFAULT)
    {
      line_indent = blocks[block_top].base_indent + 1;
      indent = line_indent + 1;
    }

    if (line_indent < 0 || (size_t)line_indent > (sizeof(line) - 1) / 2)
    {
      snprintf(error, error_size, "Line %d: indentation is too deep.", line_number);
      free(copy);
      return FALSE;
    }

    nlen = (size_t)line_indent * 2;
    memset(line, ' ', nlen);
    line[nlen] = '\0';
    ret = snprintf(line + nlen, sizeof(line) - nlen, "%s\r\n", text);
    if (ret < 0 || (size_t)ret >= sizeof(line) - nlen)
    {
      snprintf(error, error_size, "Line %d: line is too long.", line_number);
      free(copy);
      return FALSE;
    }
    llen = (size_t)ret;
    if (nlen + llen >= sizeof(nsc) - len || len >= max_length || nlen + llen > max_length - len - 1)
    {
      strlcpy(error, "The formatted script would exceed the allowed limit.", error_size);
      free(copy);
      return FALSE;
    }
    memcpy(nsc + len, line, nlen + llen + 1);
    len += nlen + llen;

    if (control == FORMAT_CONTROL_IF || control == FORMAT_CONTROL_WHILE ||
        control == FORMAT_CONTROL_SWITCH)
    {
      block_top++;
      blocks[block_top].base_indent = line_indent;
      if (control == FORMAT_CONTROL_IF)
        blocks[block_top].type = FORMAT_BLOCK_IF;
      else if (control == FORMAT_CONTROL_WHILE)
        blocks[block_top].type = FORMAT_BLOCK_WHILE;
      else
        blocks[block_top].type = FORMAT_BLOCK_SWITCH;
      indent = line_indent + 1;
    }
  }

  *formatted = strdup(nsc);
  free(copy);
  if (*formatted == NULL)
  {
    strlcpy(error, "Out of memory while formatting the script.", error_size);
    return FALSE;
  }
  return TRUE;
}

int format_script(struct descriptor_data *d)
{
  char error[MAX_INPUT_LENGTH];
  char *formatted;

  if (d == NULL || d->str == NULL || *d->str == NULL)
    return FALSE;
  if (!dg_format_script_text(*d->str, d->max_str, &formatted, error, sizeof(error)))
  {
    write_to_output(d, "Script not formatted: %s\r\n", error);
    return FALSE;
  }

  free(*d->str);
  *d->str = formatted;
  return TRUE;
}
