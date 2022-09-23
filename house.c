/**************************************************************************
 *  File: house.c                                      Part of LuminariMUD *
 *  Usage: Handling of player houses.                                      *
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
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "modify.h"
#include "mysql.h"
#include "clan.h"
#include "act.h"        /* for perform_save() */
#include "dg_scripts.h" /* for load_otriggers() */

#define MAX_BAG_ROWS 5

/* external functions */
obj_save_data *objsave_parse_objects(FILE *fl);
zone_rnum real_zone_by_thing(room_vnum vznum);

/* globals */
struct house_control_rec house_control[MAX_HOUSES];
int num_of_houses = 0;

/* functions */
void House_delete_file(room_vnum vnum);
house_rnum find_house(room_vnum vnum);
void House_save_control(void);

/* local functions */
static int House_get_filename(room_vnum vnum, char *filename, size_t maxlen);
static int House_load(room_vnum vnum);
static void House_restore_weight(struct obj_data *obj);
static void hcontrol_build_house(struct char_data *ch, char *arg);
static void hcontrol_destroy_house(struct char_data *ch, char *arg);
static void hcontrol_pay_house(struct char_data *ch, char *arg);
static void House_listrent(struct char_data *ch, room_vnum vnum);

/* CONVERSION code starts here -- see comment below. */
static int ascii_convert_house(struct char_data *ch, obj_vnum vnum);
static void hcontrol_convert_houses(struct char_data *ch);
static struct obj_data *Obj_from_store(struct obj_file_elem object, int *location);
/* CONVERSION code ends here -- see comment below. */

/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
static int House_get_filename(room_vnum vnum, char *filename, size_t maxlen)
{
  if (vnum == NOWHERE)
    return (0);

  snprintf(filename, maxlen, LIB_HOUSE "%d.house", vnum);
  return (1);
}

/* Handle objects (containers) when loading to a house */
/*
static int handle_house_obj(struct obj_data *temp, room_vnum vnum, int locate, struct obj_data **cont_row) {
  int j;
  struct obj_data *obj1;
  room_rnum rnum;

  if (!temp) // this should never happen, but....
    return FALSE;

  if ((rnum = real_room(vnum)) == NOWHERE)
    return (0);

  log("object: %s at location: %d", GET_OBJ_SHORT(temp), locate);

  for (j = MAX_BAG_ROWS - 1; j > 0; j--)
    if (cont_row[j])
      log("cont_row[%d]: %s", j, GET_OBJ_SHORT(cont_row[j]));
    else
      log("cont_row[%d] is null.", j);

  // What to do with a new loaded item:
   // If there's a list with <locate> less than 1 below this
   // then its container has disappeared from the file
   // *gasp* -> put all the list back to the room if
   // there's a list of contents with <locate> 1 below this: check if it's a
   // container - if so: get it from ch, fill it, and give it back to ch (this
   // way the container has its correct weight before modifying ch) - if not:
   // the container is missing -> put all the list to ch's inventory. For items
   // with negative <locate>: If there's already a list of contents with the
   // same <locate> put obj to it if not, start a new list. Since <locate> for
   // contents is < 0 the list indices are switched to non-negative.

  for (j = MAX_BAG_ROWS - 1; j > -locate; j--)
    if (cont_row[j]) { // no container -> back to room
      for (; cont_row[j]; cont_row[j] = obj1) {
        obj1 = cont_row[j]->next_content;
        obj_to_room(cont_row[j], rnum);
        log("adding obj to room 3...");
      }
      cont_row[j] = NULL;
    }

  if (j == -locate && cont_row[j]) { // content list existing
    if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER ||
            GET_OBJ_TYPE(temp) == ITEM_AMMO_POUCH) {
      // take item ; fill ; give to char again
      //obj_from_room(temp);
      temp->contains = NULL;
      for (; cont_row[j]; cont_row[j] = obj1) {
        obj1 = cont_row[j]->next_content;
        obj_to_obj(cont_row[j], temp);
        log("adding obj to obj 2...");
      }
      obj_to_room(temp, rnum); // add to room first ...
      log("adding obj to room 4...");
    } else { // object isn't container -> empty content list

      for (; cont_row[j]; cont_row[j] = obj1) {
        obj1 = cont_row[j]->next_content;
        obj_to_room(cont_row[j], rnum);
        log("adding obj to room 5...");
      }
      cont_row[j] = NULL;
    }
  }

  if (locate < 0 && locate >= -MAX_BAG_ROWS) {
    // let obj be part of content list
     //  but put it at the list's end thus having the items
      // in the same order as before renting
    //obj_from_room(temp);
    if ((obj1 = cont_row[-locate - 1])) {
      while (obj1->next_content)
        obj1 = obj1->next_content;
      obj1->next_content = temp;
    } else
      cont_row[-locate - 1] = temp;
  }

  return TRUE;
}
*/

/* Load all objects for a house */
static int House_load(room_vnum vnum)
{
  FILE *fl;
  int i = 0;
  // int num_objs = 0;
  char filename[MAX_STRING_LENGTH];
  obj_save_data *loaded, *current;
  struct obj_data *cont_row[MAX_BAG_ROWS];
  room_rnum rnum;

  for (i = 0; i < MAX_BAG_ROWS; i++)
    cont_row[i] = NULL;

  if ((rnum = real_room(vnum)) == NOWHERE)
    return (0);
  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return (0);
  if (!(fl = fopen(filename, "r"))) /* no file found */
    return (0);

  loaded = objsave_parse_objects_db(NULL, vnum);

  for (current = loaded; current != NULL; current = current->next)
    obj_to_room(current->obj, rnum);

  /* for (current = loaded; current != NULL; current = current->next)
    num_objs += handle_house_obj(current->obj, vnum, current->locate, cont_row);
   */
  /* now it's safe to free the obj_save_data list - all members of it
   * have been put in the correct lists by obj_to_room()
   */
  while (loaded != NULL)
  {
    current = loaded;
    loaded = loaded->next;
    free(current);
  }

  fclose(fl);

  return (1);
}

/* Save all objects for a house (recursive; initial call must be followed by a
 * call to House_restore_weight)  Assumes file is open already. */
int House_save(struct obj_data *obj, room_vnum vnum, FILE *fp, int location)
{
  struct obj_data *tmp;
  int result;

  if (obj)
  {
    House_save(obj->next_content, vnum, fp, location);
    House_save(obj->contains, vnum, fp, MIN(0, location) - 1);

    /* save a single item to file */
    result = objsave_save_obj_record_db(obj, NULL, vnum, fp, location);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return (0);
  }
  return (1);
}

/* restore weight of containers after House_save has changed them for saving */
static void House_restore_weight(struct obj_data *obj)
{
  if (obj)
  {
    House_restore_weight(obj->contains);
    House_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}

/* Save all objects in a house */
void House_crashsave(room_vnum vnum)
{
  int rnum;
  char buf[MAX_STRING_LENGTH];
  FILE *fp;
  char del_buf[2048];

  if (mysql_query(conn, "start transaction;"))
  {
    log("SYSERR: Unable to start transaction for saving of house data: %s",
        mysql_error(conn));
    return;
  }
  /* Delete existing save data.  In the future may just flag these for deletion. */
  snprintf(del_buf, sizeof(del_buf), "delete from house_data where vnum = '%d';",
           vnum);
  if (mysql_query(conn, del_buf))
  {
    log("SYSERR: Unable to delete house data: %s",
        mysql_error(conn));
    return;
  }

  if ((rnum = real_room(vnum)) == NOWHERE)
    return;
  if (!House_get_filename(vnum, buf, sizeof(buf)))
    return;
  if (!(fp = fopen(buf, "wb")))
  {
    perror("SYSERR: Error saving house file");
    return;
  }
  if (!House_save(world[rnum].contents, vnum, fp, 0))
  {
    fclose(fp);
    return;
  }
  fclose(fp);

  House_restore_weight(world[rnum].contents);

  if (mysql_query(conn, "commit;"))
  {
    log("SYSERR: Unable to commit transaction for saving of house data: %s",
        mysql_error(conn));
    return;
  }

  REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_HOUSE_CRASH);
}

/* Delete a house save file */
void House_delete_file(room_vnum vnum)
{
  char filename[MAX_INPUT_LENGTH];
  FILE *fl;

  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return;

  if (!(fl = fopen(filename, "rb")))
  {
    if (errno != ENOENT)
      log("SYSERR: Error deleting house file #%d. (1): %s", vnum, strerror(errno));
    return;
  }

  fclose(fl);

  if (remove(filename) < 0)
    log("SYSERR: Error deleting house file #%d. (2): %s", vnum, strerror(errno));
}

/* List all objects in a house file */
static void House_listrent(struct char_data *ch, room_vnum vnum)
{
  FILE *fl;
  char filename[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  obj_save_data *loaded, *current;
  int len = 0;

  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return;

  if (!(fl = fopen(filename, "rb")))
  {
    send_to_char(ch, "No objects on file for house #%d.\r\n", vnum);
    return;
  }

  *buf = '\0';

  len = snprintf(buf, sizeof(buf), "filename: %s\r\n", filename);

  loaded = objsave_parse_objects_db(NULL, vnum);

  for (current = loaded; current != NULL; current = current->next)
    len += snprintf(buf + len, sizeof(buf) - len, " [%5d] (%5dau) %s\r\n",
                    GET_OBJ_VNUM(current->obj), GET_OBJ_RENT(current->obj), current->obj->short_description);

  /* now it's safe to free the obj_save_data list - all members of it
   * have been put in the correct lists by obj_to_room()
   */
  while (loaded != NULL)
  {
    current = loaded;
    loaded = loaded->next;
    extract_obj(current->obj);
    free(current);
  }

  page_string(ch->desc, buf, 0);
  fclose(fl);
}

/* Functions for house administration (creation, deletion, etc. */
house_rnum find_house(room_vnum vnum)
{
  house_rnum i;

  for (i = 0; i < num_of_houses; i++)
    if (house_control[i].vnum == vnum)
      return (i);

  return (NOWHERE);
}

/* Save the house control information */
void House_save_control(void)
{
  FILE *fl;

  if (!(fl = fopen(HCONTROL_FILE, "wb")))
  {
    perror("SYSERR: Unable to open house control file.");
    return;
  }

  /* write all the house control recs in one fell swoop.  Pretty nifty, eh? */
  if (fwrite(house_control, sizeof(struct house_control_rec), num_of_houses, fl) != num_of_houses)
  {
    perror("SYSERR: Unable to save house control file.");
    return;
  }

  fclose(fl);
}

/* Call from boot_db - will load control recs, load objs, set atrium bits.
 * Should do sanity checks on vnums & remove invalid records. */
void House_boot(void)
{
  struct house_control_rec temp_house;
  room_rnum real_house, real_atrium;
  FILE *fl;

  memset((char *)house_control, 0, sizeof(struct house_control_rec) * MAX_HOUSES);

  if (!(fl = fopen(HCONTROL_FILE, "rb")))
  {
    if (errno == ENOENT)
      log("   No houses to load. File '%s' does not exist.", HCONTROL_FILE);
    else
      perror("SYSERR: " HCONTROL_FILE);
    return;
  }

  while (!feof(fl) && num_of_houses < MAX_HOUSES)
  {
    if (fread(&temp_house, sizeof(struct house_control_rec), 1, fl) != 1)
      break;

    if (feof(fl))
      break;

    if (get_name_by_id(temp_house.owner) == NULL)
      continue; /* owner no longer exists -- skip */

    if ((real_house = real_room(temp_house.vnum)) == NOWHERE)
      continue; /* this vnum doesn't exist -- skip */

    if (find_house(temp_house.vnum) != NOWHERE)
      continue; /* this vnum is already a house -- skip */

    if ((real_atrium = real_room(temp_house.atrium)) == NOWHERE)
      continue; /* house doesn't have an atrium -- skip */

    if (temp_house.exit_num < 0 || temp_house.exit_num >= DIR_COUNT)
      continue; /* invalid exit num -- skip */

    if (TOROOM(real_house, temp_house.exit_num) != real_atrium)
      continue; /* exit num mismatch -- skip */

    house_control[num_of_houses++] = temp_house;

    SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
    SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
    SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
    House_load(temp_house.vnum);
  }

  fclose(fl);
  House_save_control();
}

/* "House Control" functions */
const char *HCONTROL_FORMAT =
    "Usage: hcontrol build <house vnum> <exit direction> <player name>\r\n"
    "       hcontrol destroy <house vnum>\r\n"
    "       hcontrol pay <house vnum>\r\n"
    "       hcontrol show [house vnum | .]\r\n";

void hcontrol_list_houses(struct char_data *ch, char *arg)
{
  house_rnum i;
  char *timestr, *temp;
  char built_on[128], last_pay[128], own_name[MAX_NAME_LENGTH + 1];

  if (arg && *arg)
  {
    room_vnum toshow;

    if (*arg == '.')
      toshow = GET_ROOM_VNUM(IN_ROOM(ch));
    else
      toshow = atoi(arg);

    if ((i = find_house(toshow)) == NOWHERE)
    {
      send_to_char(ch, "Unknown house, \"%s\".\r\n", arg);
      return;
    }
    House_listrent(ch, toshow);
    return;
  }

  if (!num_of_houses)
  {
    send_to_char(ch, "No houses have been defined.\r\n");
    return;
  }

  send_to_char(ch,
               "Address  Atrium  Build Date  Guests  Owner        Last Paymt\r\n"
               "-------  ------  ----------  ------  ------------ ----------\r\n");

  for (i = 0; i < num_of_houses; i++)
  {
    /* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
    if ((temp = get_name_by_id(house_control[i].owner)) == NULL)
      continue;

    if (house_control[i].built_on)
    {
      timestr = asctime(localtime(&(house_control[i].built_on)));
      *(timestr + 10) = '\0';
      strlcpy(built_on, timestr, sizeof(built_on));
    }
    else
      strlcpy(built_on, "Unknown", sizeof(built_on)); /* strcpy: OK (for 'strlen("Unknown") < 128') */

    if (house_control[i].last_payment)
    {
      timestr = asctime(localtime(&(house_control[i].last_payment)));
      *(timestr + 10) = '\0';
      strlcpy(last_pay, timestr, sizeof(last_pay));
    }
    else
      strlcpy(last_pay, "None", sizeof(last_pay)); /* strcpy: OK (for 'strlen("None") < 128') */

    /* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
    strlcpy(own_name, temp, sizeof(own_name)); /* strcpy: OK (names guaranteed <= MAX_NAME_LENGTH+1) */
    send_to_char(ch, "%7d %7d  %-10s    %2d    %-12s %s\r\n",
                 house_control[i].vnum, house_control[i].atrium, built_on,
                 house_control[i].num_of_guests, CAP(own_name), last_pay);

    House_list_guests(ch, i, TRUE);
  }
}

/* building a house */
static void hcontrol_build_house(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH];
  struct house_control_rec temp_house;
  room_vnum virt_house, virt_atrium;
  room_rnum real_house, real_atrium;
  sh_int exit_num;
  long owner;

  if (num_of_houses >= MAX_HOUSES)
  {
    send_to_char(ch, "Max houses already defined.\r\n");
    return;
  }

  /* first arg: house's vnum */
  arg = one_argument_u(arg, arg1);

  if (!*arg1)
  {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }

  virt_house = atoi(arg1);

  if ((real_house = real_room(virt_house)) == NOWHERE)
  {
    send_to_char(ch, "No such room exists.\r\n");
    return;
  }

  if ((find_house(virt_house)) != NOWHERE)
  {
    send_to_char(ch, "House already exists.\r\n");
    return;
  }

  /* second arg: direction of house's exit */
  arg = one_argument_u(arg, arg1);

  if (!*arg1)
  {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }

  if ((exit_num = search_block(arg1, dirs, FALSE)) < 0)
  {
    send_to_char(ch, "'%s' is not a valid direction.\r\n", arg1);
    return;
  }

  if (TOROOM(real_house, exit_num) == NOWHERE)
  {
    send_to_char(ch, "There is no exit %s from room %d.\r\n", dirs[exit_num], virt_house);
    return;
  }

  real_atrium = TOROOM(real_house, exit_num);
  virt_atrium = GET_ROOM_VNUM(real_atrium);

  if (TOROOM(real_atrium, rev_dir[exit_num]) != real_house)
  {
    send_to_char(ch, "A house's exit must be a two-way door.\r\n");
    return;
  }

  /* third arg: player's name */
  one_argument(arg, arg1, sizeof(arg1));

  if (!*arg1)
  {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }

  if ((owner = get_id_by_name(arg1)) < 0)
  {
    send_to_char(ch, "Unknown player '%s'.\r\n", arg1);
    return;
  }

  temp_house.mode = HOUSE_PRIVATE;
  temp_house.vnum = virt_house;
  temp_house.atrium = virt_atrium;
  temp_house.exit_num = exit_num;
  temp_house.built_on = time(0);
  temp_house.last_payment = 0;
  temp_house.owner = owner;
  temp_house.num_of_guests = 0;

  house_control[num_of_houses++] = temp_house;

  SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
  SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
  SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
  House_crashsave(virt_house);

  send_to_char(ch, "House built.  Mazel tov!\r\n");
  House_save_control();
}

/* destroying a house */
static void hcontrol_destroy_house(struct char_data *ch, char *arg)
{
  house_rnum i;
  int j;
  room_rnum real_atrium, real_house;

  if (!*arg)
  {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }

  if ((i = find_house(atoi(arg))) == NOWHERE)
  {
    send_to_char(ch, "Unknown house.\r\n");
    return;
  }

  if ((real_atrium = real_room(house_control[i].atrium)) == NOWHERE)
    log("SYSERR: House %d had invalid atrium %d!", atoi(arg), house_control[i].atrium);
  else
    REMOVE_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

  if ((real_house = real_room(house_control[i].vnum)) == NOWHERE)
    log("SYSERR: House %d had invalid vnum %d!", atoi(arg), house_control[i].vnum);
  else
  {
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE_CRASH);
  }

  House_delete_file(house_control[i].vnum);

  for (j = i; j < num_of_houses - 1; j++)
    house_control[j] = house_control[j + 1];

  num_of_houses--;

  send_to_char(ch, "House deleted.\r\n");
  House_save_control();

  /* Now, reset the ROOM_ATRIUM flag on all existing houses' atriums, just in
   * case the house we just deleted shared an atrium with another house. -JE */
  for (i = 0; i < num_of_houses; i++)
    if ((real_atrium = real_room(house_control[i].atrium)) != NOWHERE)
      SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
}

/* function to handle paying house expenses */
static void hcontrol_pay_house(struct char_data *ch, char *arg)
{
  house_rnum i;

  if (!*arg)
    send_to_char(ch, "%s", HCONTROL_FORMAT);
  else if ((i = find_house(atoi(arg))) == NOWHERE)
    send_to_char(ch, "Unknown house.\r\n");
  else
  {
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "Payment for house %s collected by %s.", arg, GET_NAME(ch));

    house_control[i].last_payment = time(0);
    House_save_control();
    send_to_char(ch, "Payment recorded.\r\n");
  }
}

/* Misc. administrative functions */

/* crash-save all the houses */
void House_save_all(void)
{
  int i;
  room_rnum real_house;

  for (i = 0; i < num_of_houses; i++)
    if ((real_house = real_room(house_control[i].vnum)) != NOWHERE)
      if (ROOM_FLAGGED(real_house, ROOM_HOUSE_CRASH))
        House_crashsave(house_control[i].vnum);
}

/* note: arg passed must be house vnum, so there. */
int House_can_enter(struct char_data *ch, room_vnum house)
{
  house_rnum i;
  int j;
  zone_vnum zvnum;

  /* Not a house */
  if ((i = find_house(house)) == NOWHERE)
    return (1);

  /* Not a god house, and player is a god */
  if ((GET_LEVEL(ch) >= LVL_GRSTAFF) && (house_control[i].mode != HOUSE_GOD)) /* Even gods can't just walk into Imm-owned houses */
    return (1);

  switch (house_control[i].mode)
  {
  case HOUSE_PRIVATE:
  case HOUSE_GOD: /* A god's house can ONLY be entered by the owner and guests - already checked above */

    if (GET_IDNUM(ch) == house_control[i].owner)
      return (1);

    for (j = 0; j < house_control[i].num_of_guests; j++)
      if (GET_IDNUM(ch) == house_control[i].guests[j])
        return (1);

    break;

  case HOUSE_CLAN: /* Clan-owned houses - Only clan members may enter */

    zvnum = zone_table[real_zone_by_thing(house_control[i].vnum)].number;

    log("(HCE) Zone: %d, Clan ID: %d, Clanhall Zone: %d", zvnum, GET_CLAN(ch), clan_list[GET_CLAN(ch)].hall);

    if ((GET_CLAN(ch) > 0) && (clan_list[GET_CLAN(ch)].hall == zvnum))
      return (1);

    break;

  default:
    mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Invalid house type in room %d", house_control[i].vnum);
    break;
  }

  return (0);
}

/* list the guests of house */
void House_list_guests(struct char_data *ch, int i, int quiet)
{
  int j, num_printed;
  char *temp;

  if (house_control[i].num_of_guests == 0)
  {
    if (!quiet)
      send_to_char(ch, "  Guests: None\r\n");
    return;
  }

  send_to_char(ch, "  Guests: ");

  for (num_printed = j = 0; j < house_control[i].num_of_guests; j++)
  {
    /* Avoid <UNDEF>. -gg 6/21/98 */
    if ((temp = get_name_by_id(house_control[i].guests[j])) == NULL)
      continue;

    num_printed++;
    send_to_char(ch, "%c%s ", UPPER(*temp), temp + 1);
  }

  if (num_printed == 0)
    send_to_char(ch, "all dead");

  send_to_char(ch, "\r\n");
}

bool is_house_owner(struct char_data *ch, int room_vnum)
{
  int i;
  bool bRet = FALSE;

  for (i = 0; i < num_of_houses; i++)
  {
    if ((house_control[i].vnum == room_vnum) || (house_control[i].atrium == room_vnum))
    {
      if (house_control[i].owner == GET_IDNUM(ch))
      {
        bRet = TRUE;
      }
    }
  }
  return (bRet);
}

/* commands */

/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  half_chop_c(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (is_abbrev(arg1, "build"))
    hcontrol_build_house(ch, arg2);
  else if (is_abbrev(arg1, "destroy"))
    hcontrol_destroy_house(ch, arg2);
  else if (is_abbrev(arg1, "pay"))
    hcontrol_pay_house(ch, arg2);
  else if (is_abbrev(arg1, "show"))
    hcontrol_list_houses(ch, arg2);
  /* CONVERSION code starts here -- see comment below not in hcontrol_format. */
  else if (!str_cmp(arg1, "asciiconvert"))
    hcontrol_convert_houses(ch);
  /* CONVERSION ends here -- read more below. */
  else
    send_to_char(ch, "%s", HCONTROL_FORMAT);
}

/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{
  char arg[MAX_INPUT_LENGTH];
  house_rnum i;
  int j, id;

  one_argument(argument, arg, sizeof(arg));

  if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
    send_to_char(ch, "You must be in your house to set guests.\r\n");
  else if ((i = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NOWHERE)
    send_to_char(ch, "Um.. this house seems to be screwed up.\r\n");
  else if (GET_IDNUM(ch) != house_control[i].owner)
    send_to_char(ch, "Only the primary owner can set guests.\r\n");
  else if (!*arg)
    House_list_guests(ch, i, FALSE);
  else if ((id = get_id_by_name(arg)) < 0)
    send_to_char(ch, "No such player.\r\n");
  else if (id == GET_IDNUM(ch))
    send_to_char(ch, "It's your house!\r\n");
  else
  {
    for (j = 0; j < house_control[i].num_of_guests; j++)
      if (house_control[i].guests[j] == id)
      {
        for (; j < house_control[i].num_of_guests; j++)
          house_control[i].guests[j] = house_control[i].guests[j + 1];
        house_control[i].num_of_guests--;
        House_save_control();
        send_to_char(ch, "Guest deleted.\r\n");
        return;
      }
    if (house_control[i].num_of_guests == MAX_GUESTS)
    {
      send_to_char(ch, "You have too many guests.\r\n");
      return;
    }
    j = house_control[i].num_of_guests++;
    house_control[i].guests[j] = id;
    House_save_control();
    send_to_char(ch, "Guest added.\r\n");
  }
}

/* a command to sort one's house, will set up containers and dump items
   in those containers -Zusuk */
#define CONT_TRINKETS 868
#define CONT_CONSUMABLES 869
#define CONT_WEAPONS 870
#define CONT_ARMOR 871
#define CONT_CRAFTING 872
#define CONT_MISC 873

int can_hsort(struct char_data *ch, room_rnum location, bool silent)
{

  if (location == NOWHERE)
  {
    return 0;
  }

  /* make sure everything is in place! */
  if (!ROOM_FLAGGED(location, ROOM_HOUSE))
  {
    if (!silent && ch)
      send_to_char(ch, "You must be in your house to sort it.\r\n");

    return 0;
  }

  house_rnum i = NOWHERE;

  if ((i = find_house(GET_ROOM_VNUM(location))) == NOWHERE)
  {
    if (!silent && ch)
      send_to_char(ch, "Um.. this house seems to be screwed up.  Report to staff: (hsort001)!\r\n");

    return 0;
  }

  /* we got the house num */
  if (GET_IDNUM(ch) != house_control[i].owner)
  {
    if (!silent && ch)
      send_to_char(ch, "Only the primary owner can sort it.\r\n");

    return 0;
  }

  /* should be clear */
  return 1;
}

int perform_hsort(struct char_data *ch, room_rnum location, bool silent)
{

  if (location == NOWHERE)
  {
    return 0;
  }

  struct obj_data *trinkets = NULL, *consumables = NULL, *weapons = NULL,
                  *armor = NULL, *crafting = NULL, *misc = NULL,
                  *obj = NULL, *next_obj = NULL;
  bool found = FALSE;

  /* should be valid conditions to start */

  /* we are setting up various containers in the room now (if they don't exist) */

  /* trinkets container */
  trinkets = get_obj_in_list_num(real_object(CONT_TRINKETS), world[location].contents);
  if (!trinkets) /* need to load object */
  {
    trinkets = read_object(CONT_TRINKETS, VIRTUAL);
    if (trinkets)
    {
      obj_to_room(trinkets, location);
      load_otrigger(trinkets);
      found = TRUE;
    }
    else /* problem here! */
    {
      if (!silent && ch)
        send_to_char(ch, "PROBLEM!  Report to staff: (container001)!\r\n");
      return 0;
    }
  }

  /* consumables container */
  consumables = get_obj_in_list_num(real_object(CONT_CONSUMABLES), world[location].contents);
  if (!consumables) /* need to load object */
  {
    consumables = read_object(CONT_CONSUMABLES, VIRTUAL);
    if (consumables)
    {
      obj_to_room(consumables, location);
      load_otrigger(consumables);
      found = TRUE;
    }
    else /* problem here! */
    {
      if (!silent && ch)
        send_to_char(ch, "PROBLEM!  Report to staff: (container002)!\r\n");
      return 0;
    }
  }

  /* weapons container */
  weapons = get_obj_in_list_num(real_object(CONT_WEAPONS), world[location].contents);
  if (!weapons) /* need to load object */
  {
    weapons = read_object(CONT_WEAPONS, VIRTUAL);
    if (weapons)
    {
      obj_to_room(weapons, location);
      load_otrigger(weapons);
      found = TRUE;
    }
    else /* problem here! */
    {
      if (!silent && ch)
        send_to_char(ch, "PROBLEM!  Report to staff: (container003)!\r\n");
      return 0;
    }
  }

  /* armor container */
  armor = get_obj_in_list_num(real_object(CONT_ARMOR), world[location].contents);
  if (!armor) /* need to load object */
  {
    armor = read_object(CONT_ARMOR, VIRTUAL);
    if (armor)
    {
      obj_to_room(armor, location);
      load_otrigger(armor);
      found = TRUE;
    }
    else /* problem here! */
    {
      if (!silent && ch)
        send_to_char(ch, "PROBLEM!  Report to staff: (container004)!\r\n");
      return 0;
    }
  }

  /* crafting container */
  crafting = get_obj_in_list_num(real_object(CONT_CRAFTING), world[location].contents);
  if (!crafting) /* need to load object */
  {
    crafting = read_object(CONT_CRAFTING, VIRTUAL);
    if (crafting)
    {
      obj_to_room(crafting, location);
      load_otrigger(crafting);
      found = TRUE;
    }
    else /* problem here! */
    {
      if (!silent && ch)
        send_to_char(ch, "PROBLEM!  Report to staff: (container005)!\r\n");
      return 0;
    }
  }

  /* misc container */
  misc = get_obj_in_list_num(real_object(CONT_MISC), world[location].contents);
  if (!misc) /* need to load object */
  {
    misc = read_object(CONT_MISC, VIRTUAL);
    if (misc)
    {
      obj_to_room(misc, location);
      load_otrigger(misc);
      found = TRUE;
    }
    else /* problem here! */
    {
      if (!silent && ch)
        send_to_char(ch, "PROBLEM!  Report to staff: (container006)!\r\n");
      return 0;
    }
  }

  /* message/save and out if the containers are already here! */
  if (found)
  {
    if (!silent && ch)
    {
      send_to_char(ch, "You gesture casually summoning a pair of worker-golems, "
                       "magical animated dustpan and broom!  In a blur, they quickly "
                       "go to work stomping, whizzing, flying about setting up "
                       "containers then disappear!\r\n");
      act("You watch as $n gestures casually summoning a pair of worker-golems, "
          "magical animated dustpan and broom!  In a blur, they quickly "
          "go to work stomping, whizzing, flying about setting up containers in "
          "the area then disappearing!\r\n",
          FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG);
    }

    if (ch)
    {
      perform_save(ch, 0);
    }
    else
    {
      House_crashsave(GET_ROOM_VNUM(location));
    }

    return 1;
  }

  /* done handling containers */
  found = FALSE;

  /* now just plop anything in the room that is takeable and
       not the containers into the containers! */

  /* looping through the room's objects */
  for (obj = world[location].contents; obj; obj = next_obj)
  {
    next_obj = obj->next_content; /* increment */

    /* debug */ /*send_to_char(ch, "| %s", GET_OBJ_SHORT(obj));*/

    if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)))
      continue;

    switch (GET_OBJ_TYPE(obj))
    {

      /* we aren't sorting these items */
    case ITEM_CONTAINER: /*fallthrough*/
    case ITEM_FURNITURE: /*fallthrough*/
    case ITEM_FOUNTAIN:  /*fallthrough*/
    case ITEM_PORTAL:    /*fallthrough*/
    case ITEM_WALL:
      break;

      /* trinkets, fallthrough */
    case ITEM_LIGHT:
    case ITEM_WORN:
    case ITEM_CLANARMOR:
    case ITEM_GEAR_OUTFIT:
    case ITEM_AMMO_POUCH:
      found = TRUE;
      obj_from_room(obj);
      obj_to_obj(obj, trinkets);
      break;

      /* consumables, fallthrough */
    case ITEM_SPELLBOOK:
    case ITEM_DISGUISE:
    case ITEM_PICK:
    case ITEM_FOOD:
    case ITEM_DRINKCON:
    case ITEM_WEAPON_OIL:
    case ITEM_POISON:
    case ITEM_POTION:
    case ITEM_STAFF:
    case ITEM_WAND:
    case ITEM_SCROLL:
      found = TRUE;
      obj_from_room(obj);
      obj_to_obj(obj, consumables);
      break;

      /* weapons, fallthrough */
    case ITEM_WEAPON:
    case ITEM_FIREWEAPON:
    case ITEM_MISSILE:
      found = TRUE;
      obj_from_room(obj);
      obj_to_obj(obj, weapons);
      break;

      /* armor */
    case ITEM_ARMOR:
      found = TRUE;
      obj_from_room(obj);
      obj_to_obj(obj, armor);
      break;

      /* crafting, fallthrough */
    case ITEM_CRYSTAL:
    case ITEM_ESSENCE:
    case ITEM_MATERIAL:
    case ITEM_RESOURCE:
    case ITEM_BLUEPRINT:
    case ITEM_HUNT_TROPHY:
    case ITEM_BOWL:
    case ITEM_INGREDIENT:
      found = TRUE;
      obj_from_room(obj);
      obj_to_obj(obj, crafting);
      break;

    default: /* misc container */
      found = TRUE;
      obj_from_room(obj);
      obj_to_obj(obj, misc);
      break;
    }
  } /* end for */

  /* last message/save! */
  if (found)
  {
    if (!silent)
    {
      send_to_char(ch, "You gesture casually summoning a pair of worker-golems, "
                       "magical animated dustpan and broom!  In a blur, they quickly "
                       "go to work stomping, whizzing, flying about sorting and "
                       "cleaning the area then disappear!\r\n");
      act("You watch as $n gestures casually summoning a pair of worker-golems, "
          "magical animated dustpan and broom!  In a blur, they quickly "
          "go to work stomping, whizzing, flying about sorting and "
          "cleaning the area then disappear!\r\n",
          FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG);
    }
    perform_save(ch, 0);
  }
  else if (!silent)
  {
    send_to_char(ch, "You gesture casually summoning a pair of worker-golems, "
                     "magical animated dustpan and broom!  They look about hovering "
                     "briefly, noticing that there is nothing to do, they smirk in your "
                     "general direction then disappear!\r\n");
    act("You watch as $n gestures casually summoning a pair of worker-golems, "
        "magical animated dustpan and broom!  They look about hovering "
        "briefly, noticing that there is nothing to do, they smirk in $n's "
        "general direction then disappear!\r\n",
        FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG);
    return 0; /* sort of a fail */
  }

  /* made it! */
  return 1;
}

/* here is the command entry point */
ACMD(do_hsort)
{

  /* this was here for debugging/testing */
  /*
  if (port != 4101)
    return;
  */

  /* grab the players' room! */
  room_rnum location = NOWHERE;

  /* grab ch's location */
  location = IN_ROOM(ch);

  /* make sure everything is in place! */
  if (!can_hsort(ch, location, FALSE))
  {
    return;
  }

  perform_hsort(ch, location, FALSE);

  /* we are done! */
  return;
}
#undef CONT_TRINKETS
#undef CONT_CONSUMABLES
#undef CONT_WEAPONS
#undef CONT_ARMOR
#undef CONT_CRAFTING
#undef CONT_MISC

/*************************************************************************
 * All code below this point and the code above, marked "CONVERSION"     *
 * can be removed after you have converted your house rent files using   *
 * the command                                                           *
 *   hcontrol asciiconvert                                               *
 *                                                                       *
 * You can only use this command as implementor.                         *
 * After you have converted your house files, I suggest a reboot, which  *
 * will let your house files load on the next bootup. -Welcor            *
 ************************************************************************/

/* Code for conversion to ascii house rent files. */
static void hcontrol_convert_houses(struct char_data *ch)
{
  int i;

  if (GET_LEVEL(ch) < LVL_IMPL)
  {
    send_to_char(ch, "Sorry, but you are not powerful enough to do that.\r\n");
    return;
  }

  if (!num_of_houses)
  {
    send_to_char(ch, "No houses have been defined.\r\n");
    return;
  }

  send_to_char(ch, "Converting houses:\r\n");

  for (i = 0; i < num_of_houses; i++)
  {
    send_to_char(ch, "  %d", house_control[i].vnum);

    if (!ascii_convert_house(ch, house_control[i].vnum))
    {
      /* Let ascii_convert_house() tell about the error. */
      return;
    }
    else
    {
      send_to_char(ch, "...done\r\n");
    }
  }
  send_to_char(ch, "All done.\r\n");
}

static int ascii_convert_house(struct char_data *ch, obj_vnum vnum)
{
  FILE *in, *out;
  char infile[MAX_INPUT_LENGTH], *outfile;
  struct obj_data *tmp;
  int i, j = 0;

  House_get_filename(vnum, infile, sizeof(infile));

  CREATE(outfile, char, strlen(infile) + 7);
  sprintf(outfile, "%s.ascii", infile);

  if (!(in = fopen(infile, "r+b"))) /* no file found */
  {
    send_to_char(ch, "...no object file found\r\n");
    free(outfile);
    return (0);
  }

  if (!(out = fopen(outfile, "w")))
  {
    send_to_char(ch, "...cannot open output file\r\n");
    free(outfile);
    fclose(in);
    return (0);
  }

  while (!feof(in))
  {
    struct obj_file_elem object;
    if (fread(&object, sizeof(struct obj_file_elem), 1, in) != 1)
      return (0);
    if (ferror(in))
    {
      perror("SYSERR: Reading house file in House_load");
      send_to_char(ch, "...read error in house rent file.\r\n");
      free(outfile);
      fclose(in);
      fclose(out);
      return (0);
    }
    if (!feof(in))
    {
      tmp = Obj_from_store(object, &i);
      if (!objsave_save_obj_record_db(tmp, NULL, vnum, out, i))
      { /* save a single item to file */
        send_to_char(ch, "...write error in house rent file.\r\n");
        free(outfile);
        fclose(in);
        fclose(out);
        return (0);
      }
      j++;
    }
  }

  fprintf(out, "$~\n");

  fclose(in);
  fclose(out);

  free(outfile);

  send_to_char(ch, "...%d items", j);
  return 1;
}

/* The circle 3.1 function for reading rent files. No longer used by the rent system. */
static struct obj_data *Obj_from_store(struct obj_file_elem object, int *location)
{
  struct obj_data *obj;
  obj_rnum itemnum;
  int j, taeller;

  *location = 0;
  if ((itemnum = real_object(object.item_number)) == NOTHING)
    return (NULL);

  obj = read_object(itemnum, REAL);
#if USE_AUTOEQ
  *location = object.location;
#endif
  GET_OBJ_VAL(obj, 0) = object.value[0];
  GET_OBJ_VAL(obj, 1) = object.value[1];
  GET_OBJ_VAL(obj, 2) = object.value[2];
  GET_OBJ_VAL(obj, 3) = object.value[3];
  for (taeller = 0; taeller < EF_ARRAY_MAX; taeller++)
    GET_OBJ_EXTRA(obj)
  [taeller] = object.extra_flags[taeller];
  GET_OBJ_WEIGHT(obj) = object.weight;
  GET_OBJ_TIMER(obj) = object.timer;
  for (taeller = 0; taeller < AF_ARRAY_MAX; taeller++)
    GET_OBJ_AFFECT(obj)
  [taeller] = object.bitvector[taeller];

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    obj->affected[j] = object.affected[j];

  return (obj);
}

/* EOF */
