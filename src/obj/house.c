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
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/handler.h"
#include "core/db.h"
#include "core/interpreter.h"
#include "house.h"
#include "core/constants.h"
#include "core/modify.h"
#include "database/mysql.h"
#include "clan/clan.h"
#include "act/act.h"             /* for perform_save() */
#include "dgscript/dg_scripts.h" /* for load_otriggers() */
#include "olc/genzon.h"          /* for real_zone_by_thing() */
#include "core/perfmon.h"
#include "core/binary_formats.h"

#define MAX_BAG_ROWS 5

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

_Static_assert(HOUSE_FILE_MAX_GUESTS == MAX_GUESTS, "house control files hold MAX_GUESTS guests");

/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
static int House_get_filename(room_vnum vnum, char *filename, size_t maxlen)
{
  if (vnum == NOWHERE)
    return (0);

  snprintf(filename, maxlen, LIB_HOUSE "%" PRI_IDX ".house", vnum);
  return (1);
}

/* Load all objects for a house.
 *
 * Objects are loaded flat into the room, ignoring the per-record <locate>
 * nesting that House_save() writes. This is deliberate: perform_hsort() runs
 * at the end of this function and re-files everything into the standard
 * category containers, so any nesting reconstructed here would be undone
 * immediately. The historical container-rebuilding helper (handle_house_obj)
 * was retired when sorting was introduced; see git history if the old
 * cont_row algorithm is ever needed again. */
static int House_load(room_vnum vnum)
{
  FILE *fl;
  char filename[MAX_STRING_LENGTH] = {'\0'};
  obj_save_data *loaded, *current;
  room_rnum rnum;

  if ((rnum = real_room(vnum)) == NOWHERE)
    return (0);
  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return (0);
  if (!(fl = fopen(filename, "r"))) /* no file found */
    return (0);

  loaded = objsave_parse_objects_db(NULL, vnum);

  for (current = loaded; current != NULL; current = current->next)
    obj_to_room(current->obj, rnum);

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

  /* we are sorting the house here */
  /* make sure everything is in place! */
  if (can_hsort(NULL, real_room(vnum), TRUE))
  {
    /* engine! */
    perform_hsort(NULL, real_room(vnum), TRUE);
  }
  /***** end sorting ***************/

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
bool House_crashsave(room_vnum vnum)
{
  room_rnum rnum;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  FILE *fp;
  char del_buf[2048];
  enum perf_sql_category previous_sql_category;
  bool success;

  PERF_PROF_ENTER_SAMPLED(pr_house_save_, "save.house");
  previous_sql_category = PERF_sql_scope_set(PERF_SQL_HOUSE);
  success = false;

  if (mysql_query(conn, "start transaction;"))
  {
    log("SYSERR: Unable to start transaction for saving of house data: %s", mysql_error(conn));
    goto cleanup;
  }
  /* Delete existing save data.  In the future may just flag these for deletion. */
  snprintf(del_buf, sizeof(del_buf), "delete from house_data where vnum = '%u';", vnum);
  if (mysql_query(conn, del_buf))
  {
    log("SYSERR: Unable to delete house data: %s", mysql_error(conn));
    mysql_query(conn, "rollback;");
    goto cleanup;
  }

  if ((rnum = real_room(vnum)) == NOWHERE)
  {
    mysql_query(conn, "rollback;");
    goto cleanup;
  }
  if (!House_get_filename(vnum, buf, sizeof(buf)))
  {
    mysql_query(conn, "rollback;");
    goto cleanup;
  }
  if (!(fp = fopen_restricted(buf, "wb")))
  {
    perror("SYSERR: Error saving house file");
    mysql_query(conn, "rollback;");
    goto cleanup;
  }
  if (!House_save(world[rnum].contents, vnum, fp, 0))
  {
    fclose(fp);
    mysql_query(conn, "rollback;");
    goto cleanup;
  }
  fclose(fp);

  House_restore_weight(world[rnum].contents);

  if (mysql_query(conn, "commit;"))
  {
    log("SYSERR: Unable to commit transaction for saving of house data: %s", mysql_error(conn));
    mysql_query(conn, "rollback;");
    goto cleanup;
  }

  REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_HOUSE_CRASH);
  success = true;

cleanup:
  PERF_sql_scope_restore(previous_sql_category);
  PERF_PROF_EXIT(pr_house_save_);
  return success;
}

/* Delete a house save file */
void House_delete_file(room_vnum vnum)
{
  char filename[MAX_INPUT_LENGTH] = {'\0'};
  FILE *fl;

  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return;

  if (!(fl = fopen(filename, "rb")))
  {
    if (errno != ENOENT)
      log("SYSERR: Error deleting house file #%" PRI_IDX ". (1): %s", vnum, strerror(errno));
    return;
  }

  fclose(fl);

  if (remove(filename) < 0)
    log("SYSERR: Error deleting house file #%" PRI_IDX ". (2): %s", vnum, strerror(errno));
}

/* List all objects in a house file */
static void House_listrent(struct char_data *ch, room_vnum vnum)
{
  FILE *fl;
  char filename[MAX_STRING_LENGTH] = {'\0'};
  char buf[MAX_STRING_LENGTH] = {'\0'};
  obj_save_data *loaded, *current;
  int len = 0;

  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return;

  if (!(fl = fopen(filename, "rb")))
  {
    send_to_char(ch, "No objects on file for house #%" PRI_IDX ".\r\n", vnum);
    return;
  }

  *buf = '\0';

  len = snprintf(buf, sizeof(buf), "filename: %s\r\n", filename);

  loaded = objsave_parse_objects_db(NULL, vnum);

  for (current = loaded; current != NULL; current = current->next)
    len =
        snprintf_append(buf, sizeof(buf), len, " [%5u] (%5dau) %s\r\n", GET_OBJ_VNUM(current->obj),
                        GET_OBJ_RENT(current->obj), current->obj->short_description);

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
  int i;

  for (i = 0; i < num_of_houses; i++)
    if (house_control[i].vnum == vnum)
      return ((house_rnum)i);

  return (NOWHERE);
}

static void house_record_from_control(const struct house_control_rec *house,
                                      struct house_file_record *record)
{
  int guest;

  memset(record, 0, sizeof(*record));
  record->vnum = house->vnum;
  record->atrium = house->atrium;
  record->exit_num = house->exit_num;
  record->mode = house->mode;
  record->built_on = house->built_on;
  record->owner = house->owner;
  record->last_payment = house->last_payment;
  record->bitvector = house->bitvector;
  record->builtby = house->builtby;
  record->num_of_guests = house->num_of_guests;
  for (guest = 0; guest < house->num_of_guests && guest < MAX_GUESTS; guest++)
    record->guests[guest] = house->guests[guest];
}

static void house_control_from_record(const struct house_file_record *record,
                                      struct house_control_rec *house)
{
  int guest;

  memset(house, 0, sizeof(*house));
  house->vnum = record->vnum;
  house->atrium = record->atrium;
  house->exit_num = record->exit_num;
  house->mode = record->mode;
  house->built_on = (time_t)record->built_on;
  house->owner = (long)record->owner;
  house->last_payment = (time_t)record->last_payment;
  house->bitvector = (long)record->bitvector;
  house->builtby = (long)record->builtby;
  house->num_of_guests = record->num_of_guests;
  for (guest = 0; guest < record->num_of_guests; guest++)
    house->guests[guest] = (long)record->guests[guest];
}

/* Save the house control information */
void House_save_control(void)
{
  struct house_file_record *records = NULL;
  enum binary_format_status status;
  unsigned char *data = NULL;
  size_t size = 0;
  int i;

  if (num_of_houses > 0)
    CREATE(records, struct house_file_record, num_of_houses);
  for (i = 0; i < num_of_houses; i++)
    house_record_from_control(&house_control[i], &records[i]);

  status = house_file_encode(records, (size_t)num_of_houses, &data, &size);
  free(records);
  if (status != BINARY_FORMAT_OK)
  {
    log("SYSERR: Unable to encode the house control file: %s.", binary_format_status_name(status));
    return;
  }
  if (!replace_durable_file(HCONTROL_FILE, HOUSE_FILE_MAGIC, house_file_max_size(MAX_HOUSES), data,
                            size))
    log("SYSERR: Unable to save the house control file %s.", HCONTROL_FILE);
  free(data);
}

/* Call from boot_db - will load control recs, load objs, set atrium bits.
 * Invalid records are skipped and dropped by the save at the end. A file that
 * cannot be decoded is moved aside instead, and nothing is saved over it. */
void House_boot(void)
{
  struct house_file_record *records = NULL;
  struct house_control_rec temp_house;
  enum binary_format_status status;
  room_rnum real_house, real_atrium;
  unsigned char *data = NULL;
  size_t size = 0, count = 0, i;
  int version = BINARY_FORMAT_LEGACY;

  memset((char *)house_control, 0, sizeof(struct house_control_rec) * MAX_HOUSES);

  switch (read_durable_file(HCONTROL_FILE, house_file_max_size(MAX_HOUSES), &data, &size))
  {
  case DURABLE_FILE_ABSENT:
    log("   No houses to load. File '%s' does not exist.", HCONTROL_FILE);
    return;
  case DURABLE_FILE_UNREADABLE:
    quarantine_durable_file(HCONTROL_FILE);
    return;
  case DURABLE_FILE_READ:
    break;
  }

  status = house_file_decode(data, size, MAX_HOUSES, &records, &count, &version);
  free(data);
  if (status != BINARY_FORMAT_OK)
  {
    log("SYSERR: Rejected house control file %s: %s.", HCONTROL_FILE,
        binary_format_status_name(status));
    quarantine_durable_file(HCONTROL_FILE);
    return;
  }
  if (version == BINARY_FORMAT_LEGACY && size > 0)
    log("House control file %s uses the legacy native layout; saving it now upgrades it and "
        "keeps a backup.",
        HCONTROL_FILE);

  for (i = 0; i < count && num_of_houses < MAX_HOUSES; i++)
  {
    house_control_from_record(&records[i], &temp_house);

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

    if (temp_house.mode < 0 || temp_house.mode >= NUM_HOUSE_TYPES)
      continue; /* unknown ownership mode -- skip */

    house_control[num_of_houses++] = temp_house;

    SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
    SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
    SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
    House_load(temp_house.vnum);
  }
  free(records);

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
  house_rnum house;
  int i;
  const char *timestr, *temp;
  char built_on[128], last_pay[128], own_name[MAX_NAME_LENGTH + 1];

  if (arg && *arg)
  {
    room_vnum toshow;

    if (*arg == '.')
      toshow = GET_ROOM_VNUM(IN_ROOM(ch));
    else
      toshow = atoi(arg);

    if ((house = find_house(toshow)) == NOWHERE)
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

  send_to_char(ch, "Address  Atrium  Build Date  Guests  Owner        Last Paymt\r\n"
                   "-------  ------  ----------  ------  ------------ ----------\r\n");

  for (i = 0; i < num_of_houses; i++)
  {
    /* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
    if ((temp = get_name_by_id(house_control[i].owner)) == NULL)
      continue;

    if (house_control[i].built_on)
    {
      timestr = format_time_ymd_hms(house_control[i].built_on);
      /* Copy only YYYY-MM-DD */
      strlcpy(built_on, timestr, sizeof(built_on));
      built_on[10] = '\0';
    }
    else
      strlcpy(built_on, "Unknown",
              sizeof(built_on)); /* strcpy: OK (for 'strlen("Unknown") < 128') */

    if (house_control[i].last_payment)
    {
      timestr = format_time_ymd_hms(house_control[i].last_payment);
      /* Copy only YYYY-MM-DD */
      strlcpy(last_pay, timestr, sizeof(last_pay));
      last_pay[10] = '\0';
    }
    else
      strlcpy(last_pay, "None", sizeof(last_pay)); /* strcpy: OK (for 'strlen("None") < 128') */

    /* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
    strlcpy(own_name, temp,
            sizeof(own_name)); /* strcpy: OK (names guaranteed <= MAX_NAME_LENGTH+1) */
    send_to_char(ch, "%7" PRI_IDX " %7" PRI_IDX "  %-10s    %2d    %-12s %s\r\n",
                 house_control[i].vnum, house_control[i].atrium, built_on,
                 house_control[i].num_of_guests, CAP(own_name), last_pay);

    House_list_guests(ch, i, TRUE);
  }
}

/* building a house */
static void hcontrol_build_house(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
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

  if ((exit_num = (sh_int)search_block(arg1, dirs, FALSE)) < 0)
  {
    send_to_char(ch, "'%s' is not a valid direction.\r\n", arg1);
    return;
  }

  if (TOROOM(real_house, exit_num) == NOWHERE)
  {
    send_to_char(ch, "There is no exit %s from room %" PRI_IDX ".\r\n", dirs[exit_num], virt_house);
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
  house_rnum house;
  int i, j;
  room_rnum real_atrium, real_house;

  if (!*arg)
  {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }

  if ((house = find_house(atoi(arg))) == NOWHERE)
  {
    send_to_char(ch, "Unknown house.\r\n");
    return;
  }

  if ((real_atrium = real_room(house_control[house].atrium)) == NOWHERE)
    log("SYSERR: House %d had invalid atrium %" PRI_IDX "!", atoi(arg),
        house_control[house].atrium);
  else
    REMOVE_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

  if ((real_house = real_room(house_control[house].vnum)) == NOWHERE)
    log("SYSERR: House %d had invalid vnum %" PRI_IDX "!", atoi(arg), house_control[house].vnum);
  else
  {
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE_CRASH);
  }

  House_delete_file(house_control[house].vnum);

  for (j = (int)house; j < num_of_houses - 1; j++)
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
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "Payment for house %s collected by %s.",
           arg, GET_NAME(ch));

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

enum persistence_step_result House_save_incremental(int max_saves)
{
  static int next_house = 0;
  static int remaining_to_check = 0;
  room_rnum real_house;
  int saved_count;

  if (num_of_houses <= 0)
  {
    next_house = 0;
    remaining_to_check = 0;
    return PERSISTENCE_STEP_COMPLETE;
  }
  if (remaining_to_check <= 0 || next_house < 0 || next_house >= num_of_houses)
  {
    next_house = 0;
    remaining_to_check = num_of_houses;
  }

  saved_count = 0;
  while (remaining_to_check > 0 && (max_saves <= 0 || saved_count < max_saves))
  {
    real_house = real_room(house_control[next_house].vnum);
    if (real_house != NOWHERE && ROOM_FLAGGED(real_house, ROOM_HOUSE_CRASH))
    {
      if (!House_crashsave(house_control[next_house].vnum))
        return PERSISTENCE_STEP_FAILURE;
      saved_count++;
    }
    next_house = (next_house + 1) % num_of_houses;
    remaining_to_check--;
  }
  if (remaining_to_check == 0)
  {
    next_house = 0;
    return PERSISTENCE_STEP_COMPLETE;
  }
  return saved_count > 0 ? PERSISTENCE_STEP_PROGRESS : PERSISTENCE_STEP_IDLE;
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
  if ((GET_LEVEL(ch) >= LVL_GRSTAFF) &&
      (house_control[i].mode != HOUSE_GOD)) /* Even gods can't just walk into Imm-owned houses */
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

    log("(HCE) Zone: %" PRI_IDX ", Clan ID: %" PRI_IDX ", Clanhall Zone: %" PRI_IDX, zvnum,
        GET_CLAN(ch), clan_list[GET_CLAN(ch)].hall);

    if ((GET_CLAN(ch) > 0) && (clan_list[GET_CLAN(ch)].hall == zvnum))
      return (1);

    break;

  default:
    mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Invalid house type in room %" PRI_IDX,
           house_control[i].vnum);
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

bool is_house_owner(struct char_data *ch, room_vnum room)
{
  int i;
  bool bRet = FALSE;

  for (i = 0; i < num_of_houses; i++)
  {
    if ((house_control[i].vnum == room) || (house_control[i].atrium == room))
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
  char arg1[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};

  half_chop_c(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (is_abbrev(arg1, "build"))
    hcontrol_build_house(ch, arg2);
  else if (is_abbrev(arg1, "destroy"))
    hcontrol_destroy_house(ch, arg2);
  else if (is_abbrev(arg1, "pay"))
    hcontrol_pay_house(ch, arg2);
  else if (is_abbrev(arg1, "show"))
    hcontrol_list_houses(ch, arg2);
  else
    send_to_char(ch, "%s", HCONTROL_FORMAT);
}

/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
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
  else if ((id = (int)get_id_by_name(arg)) < 0)
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

  /* player shops can't be sorted */
  if (ROOM_FLAGGED(location, ROOM_PLAYER_SHOP))
  {
    if (!silent && ch)
      send_to_char(ch, "You can't use this command in a player shop.\r\n");

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
  if (ch && GET_IDNUM(ch) != house_control[i].owner)
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

  struct obj_data *trinkets = NULL, *consumables = NULL, *weapons = NULL, *armor = NULL,
                  *crafting = NULL, *misc = NULL, *obj = NULL, *next_obj = NULL;
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
    if (!silent && ch)
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
      perform_save(ch, 0);
    }
    else
    {
      House_crashsave(GET_ROOM_VNUM(location));
    }
  }
  else if (!silent && ch)
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
  UNUSED(argument);

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
  if (can_hsort(ch, location, FALSE))
  {
    /* engine! */
    perform_hsort(ch, location, FALSE);
  }

  /* we are done! */
  return;
}
#undef CONT_TRINKETS
#undef CONT_CONSUMABLES
#undef CONT_WEAPONS
#undef CONT_ARMOR
#undef CONT_CRAFTING
#undef CONT_MISC

/* EOF */
