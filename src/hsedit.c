/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari House OLC System
/  Created By: Jamdog, Copyright 2007 Stefan Cole (aka Jamdog)
\              Ported to LuminariMUD by Zusuk
/
\
/
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#include "conf.h"
#include "sysdep.h"
#include <time.h>
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "oasis.h"
#include "genolc.h"
#include "genzon.h"
#include "house.h"
#include "screen.h"

/*------------------------------------------------------------------------*/
/*. External data .*/

extern struct zone_data *zone_table;
extern struct house_control_rec house_control[MAX_HOUSES]; /* house.c */
extern int num_of_houses;                                  /* house.c */
extern const char *dirs[];                                 /* constants.c */
/*------------------------------------------------------------------------*/
/* external function protos */
house_rnum find_house(room_vnum vnum);  /* house.c */
void House_save_control(void);          /* house.c */
void House_delete_file(room_vnum vnum); /* house.c */
extern void strip_string(char *buffer);

/*------------------------------------------------------------------------*/
/* local function protos */
void hsedit_setup_new(struct descriptor_data *d);
void hsedit_setup_existing(struct descriptor_data *d, int real_num);
void hsedit_save_internally(struct descriptor_data *d);
void hsedit_save_to_disk(void);
void hsedit_disp_type_menu(struct descriptor_data *d);
void hsedit_disp_menu(struct descriptor_data *d);
void hsedit_parse(struct descriptor_data *d, char *arg);
void hsedit_disp_flags_menu(struct descriptor_data *d);
void hsedit_disp_val0_menu(struct descriptor_data *d);
void hsedit_disp_val1_menu(struct descriptor_data *d);
void hsedit_disp_val2_menu(struct descriptor_data *d);
void hsedit_disp_val3_menu(struct descriptor_data *d);
void free_house(struct house_control_rec *house);

/*------------------------------------------------------------------------*/
/* internal globals */
const char *house_flags[] = {
    "!GUEST",
    "FREE",
    "!IMM",
    "IMP_ONLY",
    "RENTFREE",
    "SAVE_!RENT",
    "!SAVE",
    "\n"};

const char *house_types[] = {
    "PLAYER_OWNED",
    "IMM_OWNED",
    "CLAN_OWNED",
    "\n"};

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

void hsedit_setup_new(struct descriptor_data *d)
{
  int i;
  CREATE(OLC_HOUSE(d), struct house_control_rec, 1);
  OLC_HOUSE(d)->vnum = OLC_NUM(d);
  OLC_HOUSE(d)->owner = 0;
  OLC_HOUSE(d)->atrium = 0;
  OLC_HOUSE(d)->exit_num = -1;
  OLC_HOUSE(d)->built_on = time(0);
  OLC_HOUSE(d)->mode = HOUSE_PRIVATE;
  OLC_HOUSE(d)->bitvector = 0;
  OLC_HOUSE(d)->builtby = GET_ID(d->character);
  OLC_HOUSE(d)->last_payment = 0;
  OLC_HOUSE(d)->num_of_guests = 0;
  for (i = 0; i < MAX_GUESTS; i++)
    OLC_HOUSE(d)->guests[i] = 0;
  OLC_VAL(d) = 0;
  hsedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/

void hsedit_setup_existing(struct descriptor_data *d, int real_num)
{
  struct house_control_rec *house;
  int i;

  /*. Build a copy of the house .*/
  CREATE(house, struct house_control_rec, 1);
  *house = house_control[real_num];

  /* allocate space for all strings  */

  /* Copy all non-string variables */
  house->vnum = house_control[real_num].vnum;
  house->atrium = house_control[real_num].atrium;
  house->owner = house_control[real_num].owner;
  house->exit_num = house_control[real_num].exit_num;
  house->built_on = house_control[real_num].built_on;
  house->mode = house_control[real_num].mode;
  house->bitvector = house_control[real_num].bitvector;
  house->last_payment = house_control[real_num].last_payment;
  house->num_of_guests = house_control[real_num].num_of_guests;
  for (i = 0; i < MAX_GUESTS; i++)
    house->guests[i] = house_control[real_num].guests[i];

  /*. Attach room copy to players descriptor .*/
  OLC_HOUSE(d) = house;
  OLC_VAL(d) = 0;
  hsedit_disp_menu(d);
}

/*-----------------------------------------------------------1-------------*/
void hsedit_save_internally(struct descriptor_data *d)
{
  house_rnum house_rnum;

  /* this is done rather differently from the other OLCs */
  /* Houses have a pre-allocated list size, so just copy */
  /* the OLC house back into it */

  house_rnum = find_house(OLC_NUM(d));
  if (house_rnum != NOWHERE)
  {
    /* This house VNUM is already in the list */
    /* Replace the old data                   */
    free_house(house_control + house_rnum);
    house_control[house_rnum] = *OLC_HOUSE(d);
  }
  else
  {
    /*. House doesn't exist, hafta add it .*/
    house_rnum = num_of_houses++;
    if (house_rnum < MAX_HOUSES)
    {
      house_control[house_rnum] = *(OLC_HOUSE(d));
      house_control[house_rnum].vnum = OLC_NUM(d);
    }
    else
    {
      send_to_char(d->character, "MAX House limit reached - Unable to save this house!");
      mudlog(NRM, LVL_IMPL, TRUE, "HSEDIT: Max houses limit reached - Unable to save OLC data");
    }
  }
  /* The new house is stored - now to ensure the roomsflags are correct */
  if (real_room(OLC_HOUSE(d)->vnum) != NOWHERE)
    SET_BIT_AR(ROOM_FLAGS(real_room(OLC_HOUSE(d)->vnum)), ROOM_HOUSE | ROOM_PRIVATE);

  if (real_room(OLC_HOUSE(d)->atrium) != NOWHERE)
    SET_BIT_AR(ROOM_FLAGS(real_room(OLC_HOUSE(d)->atrium)), ROOM_ATRIUM);

  //  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_HOUSE);
}

/*------------------------------------------------------------------------*/

void hsedit_save_to_disk(void)
{
  /* Why bother writing a new function when there is already one that does the job */
  House_save_control();
}

/*------------------------------------------------------------------------*/

void free_house(struct house_control_rec *house)
{
  /* House structure has no strings to free                */
  /* This function is here in case someone adds some later */
}

/*------------------------------------------------------------------------*/

void hsedit_delete_house(struct descriptor_data *d, int house_vnum)
{
  house_rnum i;
  int j;
  room_rnum real_atrium, real_house;

  if ((i = find_house(house_vnum)) == NOWHERE)
  {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: hsedit: Invalid house vnum in hedit_delete_house\r\n");
    cleanup_olc(d, CLEANUP_STRUCTS);
    return;
  }
  if ((real_atrium = real_room(house_control[i].atrium)) == NOWHERE)
    log("SYSERR: House %d had invalid atrium %d!", house_vnum, house_control[i].atrium);
  else
    REMOVE_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

  if ((real_house = real_room(house_control[i].vnum)) == NOWHERE)
    log("SYSERR: House %d had invalid vnum %d!", house_vnum, house_control[i].vnum);
  else
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE | ROOM_PRIVATE | ROOM_HOUSE_CRASH);

  House_delete_file(house_control[i].vnum);

  for (j = i; j < num_of_houses - 1; j++)
    house_control[j] = house_control[j + 1];

  num_of_houses--;

  send_to_char(d->character, "House deleted.\r\n");
  House_save_control();

  /*
   * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
   * just in case the house we just deleted shared an atrium with another
   * house.  --JE 9/19/94
   */
  for (i = 0; i < num_of_houses; i++)
    if ((real_atrium = real_room(house_control[i].atrium)) != NOWHERE)
      SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

  cleanup_olc(d, CLEANUP_ALL);
}

/**************************************************************************
 Menu functions
 **************************************************************************/

void hsedit_disp_flags_menu(struct descriptor_data *d)
{
  int counter, columns = 0;
  char buf1[MAX_STRING_LENGTH] = {'\0'};

  clear_screen(d);
  for (counter = 0; counter < HOUSE_NUM_FLAGS; counter++)
  {
    send_to_char(d->character, "%s%2d%s) %-20.20s ",
                 CBGRN(d->character, C_NRM), counter + 1, CCNRM(d->character, C_NRM), house_flags[counter]);
    if (!(++columns % 2))
      send_to_char(d->character, "\r\n");
  }
  sprintbit(OLC_HOUSE(d)->bitvector, house_flags, buf1, sizeof(buf1));
  send_to_char(d->character,
               "\r\nHouse flags: %s%s%s\r\n"
               "Enter house flags, 0 to quit : ",
               CCCYN(d->character, C_NRM), buf1, CCNRM(d->character, C_NRM));
  OLC_MODE(d) = HSEDIT_FLAGS;
}

void hsedit_owner_menu(struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  struct house_control_rec *house;

  house = OLC_HOUSE(d);

  snprintf(buf, sizeof(buf),
           "%s1%s) Owner Name : %s%s%s\r\n"
           "%s2%s) Owner ID   : %s%ld%s\r\n"
           "%sQ%s) Back to main menu\r\n"
           "Enter choice : ",

           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->owner), CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), house->owner, CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM));
  send_to_char(d->character, "%s", buf);

  OLC_MODE(d) = HSEDIT_OWNER_MENU;
}

void hsedit_dir_menu(struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  struct house_control_rec *house;
  int house_rnum, newroom[6], i;

  house = OLC_HOUSE(d);

  house_rnum = real_room(house->vnum);

  if ((house_rnum < 0) || (house_rnum == NOWHERE))
  {
    snprintf(buf, sizeof(buf),
             "%sWARNING%s: %sYou cannot set an atium direction before selecting a valid room vnum%s\r\n"
             "(Press Enter)\r\n",
             CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM));
    OLC_MODE(d) = HSEDIT_NOVNUM;
  }
  else
  {
    /* Grab exit rooms */
    for (i = 0; i < 6; i++)
    {
      if (world[house_rnum].dir_option[i])
        newroom[i] = world[house_rnum].dir_option[i]->to_room;
      else
        newroom[i] = NOWHERE;
    }

    snprintf(buf, sizeof(buf),
             "%s1%s) North  : (%s%s%s)\r\n"
             "%s2%s) East   : (%s%s%s)\r\n"
             "%s3%s) South  : (%s%s%s)\r\n"
             "%s4%s) West   : (%s%s%s)\r\n"
             "%s5%s) Up     : (%s%s%s)\r\n"
             "%s6%s) Down   : (%s%s%s)\r\n"
             "%sQ%s) Back to main menu\r\n"
             "Enter atrium direction : ",

             CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), newroom[0] == NOWHERE ? "NO ROOM" : world[(newroom[0])].name, CCNRM(d->character, C_NRM),
             CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), newroom[1] == NOWHERE ? "NO ROOM" : world[(newroom[1])].name, CCNRM(d->character, C_NRM),
             CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), newroom[2] == NOWHERE ? "NO ROOM" : world[(newroom[2])].name, CCNRM(d->character, C_NRM),
             CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), newroom[3] == NOWHERE ? "NO ROOM" : world[(newroom[3])].name, CCNRM(d->character, C_NRM),
             CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), newroom[4] == NOWHERE ? "NO ROOM" : world[(newroom[4])].name, CCNRM(d->character, C_NRM),
             CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), newroom[5] == NOWHERE ? "NO ROOM" : world[(newroom[5])].name, CCNRM(d->character, C_NRM),
             CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM));
    OLC_MODE(d) = HSEDIT_DIR_MENU;
  }
  send_to_char(d->character, "%s", buf);
}

void hsedit_disp_type_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(d);
  for (counter = 0; counter < NUM_HOUSE_TYPES; counter++)
  {
    send_to_char(d->character, "%s%2d%s) %-20.20s ",
                 CBCYN(d->character, C_NRM), counter, CCNRM(d->character, C_NRM), house_types[counter]);
    if (!(++columns % 2))
      send_to_char(d->character, "\r\n");
  }
  send_to_char(d->character, "\r\nEnter house type : ");
  OLC_MODE(d) = HSEDIT_TYPE;
}

void hsedit_disp_guest_menu(struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char not_set[128];
  struct house_control_rec *house;

  house = OLC_HOUSE(d);

  snprintf(not_set, sizeof(not_set), "%s<NOT SET>%s", CCYEL(d->character, C_NRM), CCNRM(d->character, C_NRM));

  snprintf(buf, sizeof(buf),
           "%s 1%s) %s%s%s (%sID: %ld%s)\r\n"
           "%s 2%s) %s%s%s (%sID: %ld%s)\r\n"
           "%s 3%s) %s%s%s (%sID: %ld%s)\r\n"
           "%s 4%s) %s%s%s (%sID: %ld%s)\r\n"
           "%s 5%s) %s%s%s (%sID: %ld%s)\r\n"
           "%s 6%s) %s%s%s (%sID: %ld%s)\r\n"
           "%s 7%s) %s%s%s (%sID: %ld%s)\r\n"
           "%s 8%s) %s%s%s (%sID: %ld%s)\r\n"
           "%s 9%s) %s%s%s (%sID: %ld%s)\r\n"
           "%s10%s) %s%s%s (%sID: %ld%s)\r\n\r\n"
           "%sA%s) Add a guest\r\n"
           "%sD%s) Delete a guest\r\n"
           "%sC%s) Clear guest list\r\n"
           "%sQ%s) Back to main menu\r\n"
           "Enter selection (A/D/C/Q): ",

           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->guests[0]) == NULL ? not_set : get_name_by_id(house->guests[0]), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house->guests[0] < 1 ? 0 : house->guests[0], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->guests[1]) == NULL ? not_set : get_name_by_id(house->guests[1]), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house->guests[1] < 1 ? 0 : house->guests[1], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->guests[2]) == NULL ? not_set : get_name_by_id(house->guests[2]), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house->guests[2] < 1 ? 0 : house->guests[2], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->guests[3]) == NULL ? not_set : get_name_by_id(house->guests[3]), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house->guests[3] < 1 ? 0 : house->guests[3], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->guests[4]) == NULL ? not_set : get_name_by_id(house->guests[4]), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house->guests[4] < 1 ? 0 : house->guests[4], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->guests[5]) == NULL ? not_set : get_name_by_id(house->guests[5]), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house->guests[5] < 1 ? 0 : house->guests[5], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->guests[6]) == NULL ? not_set : get_name_by_id(house->guests[6]), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house->guests[6] < 1 ? 0 : house->guests[6], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->guests[7]) == NULL ? not_set : get_name_by_id(house->guests[7]), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house->guests[7] < 1 ? 0 : house->guests[7], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->guests[8]) == NULL ? not_set : get_name_by_id(house->guests[8]), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house->guests[8] < 1 ? 0 : house->guests[8], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), get_name_by_id(house->guests[9]) == NULL ? not_set : get_name_by_id(house->guests[9]), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house->guests[9] < 1 ? 0 : house->guests[9], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM));
  send_to_char(d->character, "%s", buf);

  OLC_MODE(d) = HSEDIT_GUEST_MENU;
}

void hsedit_disp_val0_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = HSEDIT_VALUE_0;
  switch (OLC_HOUSE(d)->mode)
  {
  case HOUSE_CLAN:
    send_to_char(d->character, "Enter id of the clan: ");
    break;
  case HOUSE_GOD:
    hsedit_disp_val3_menu(d); /* Skip to 4th variable */
    break;
  default:
    hsedit_disp_menu(d);
  }
}

void hsedit_disp_val1_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = HSEDIT_VALUE_1;
  switch (OLC_HOUSE(d)->mode)
  {
  default:
    hsedit_disp_menu(d);
  }
}

void hsedit_disp_val2_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = HSEDIT_VALUE_2;
  switch (OLC_HOUSE(d)->mode)
  {
  default:
    hsedit_disp_menu(d);
  }
}

void hsedit_disp_val3_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = HSEDIT_VALUE_3;
  switch (OLC_HOUSE(d)->mode)
  {
  case HOUSE_GOD:
    send_to_char(d->character, "Enter minimum level of guests: ");
    break;
  default:
    hsedit_disp_menu(d);
  }
}

static const char *hsedit_list_guests(struct house_control_rec *thishouse, char *guestlist, size_t sz)
{
  int j, num_printed;
  const char *temp;

  if (thishouse->num_of_guests == 0)
  {
    strlcpy(guestlist, "NONE", sz);
    return (guestlist);
  }

  for (num_printed = j = 0; j < thishouse->num_of_guests; j++)
  {
    /* Avoid <UNDEF>. -gg 6/21/98 */
    if ((temp = get_name_by_id(thishouse->guests[j])) == NULL)
      continue;

    num_printed++;
    char cap_buf[2] = {UPPER(*temp), '\0'};
    strlcat(guestlist, cap_buf, sz);
    strlcat(guestlist, temp + 1, sz);
  }

  if (num_printed == 0)
    strlcpy(guestlist, "all dead", sz);

  return (guestlist);
}

/* the main menu */
void hsedit_disp_menu(struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH] = {'\0'}, buf1[MAX_STRING_LENGTH] = {'\0'}, built_on[128], last_pay[128], buf2[MAX_STRING_LENGTH] = {'\0'};
  char *timestr, no_name[128];
  struct house_control_rec *house;

  clear_screen(d);
  house = OLC_HOUSE(d);

  if (house->built_on)
  {
    timestr = asctime(localtime(&(house->built_on)));
    *(timestr + 10) = '\0';
    strlcpy(built_on, timestr, sizeof(built_on));
  }
  else
    strlcpy(built_on, "Unknown", sizeof(built_on)); /* strcpy: OK (for 'strlen("Unknown") < 128') */

  if (house->last_payment)
  {
    timestr = asctime(localtime(&(house->last_payment)));
    *(timestr + 10) = '\0';
    strlcpy(last_pay, timestr, sizeof(last_pay));
  }
  else
    strlcpy(last_pay, "None", sizeof(last_pay)); /* strcpy: OK (for 'strlen("None") < 128') */

  *buf2 = '\0';
  sprintbit(house->bitvector, house_flags, buf1, sizeof(buf1));
  //  snprintf(buf2, sizeof(buf2), "%d %d %d %d", house->value[0], house->value[1], house->value[2], house->value[3]);
  snprintf(no_name, sizeof(no_name), "%s<NOBODY>%s", CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM));
  snprintf(buf, sizeof(buf),
           "%s                                               %s\r\n"
           "-- House number : [%s%d%s]  	House zone: [%s%d%s]\r\n"
           "%s1%s) Owner       : %s%ld -- %s%s\r\n"
           "%s2%s) Atrium      : %s%d%s\r\n"
           "%s3%s) Direction   : %s%s%s\r\n"
           "%s4%s) House Type  : %s%s%s\r\n"
           "%s5%s) Built on    : %s%s%s\r\n"
           "%s6%s) Payment     : %s%s%s\r\n"
           "%s7%s) Guests      : %s%s%s\r\n"
           "%s8%s) Flags       : %s%s%s\r\n"
           //      "%s9%s) Values      : %s%s%s\r\n"
           "%sX%s) Delete this house\r\n"
           "%sQ%s) Quit\r\n"
           "Enter choice : ",

           CBGRN(d->character, C_NRM), CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), OLC_NUM(d), CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), zone_table[OLC_ZNUM(d)].number, CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), house->owner, get_name_by_id(house->owner) == NULL ? no_name : get_name_by_id(house->owner), CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBYEL(d->character, C_NRM), house->atrium, CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), ((house->exit_num >= 0) && (house->exit_num <= 5)) ? dirs[(house->exit_num)] : "NONE", CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), house_types[(house->mode)], CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), built_on, CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), last_pay, CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), hsedit_list_guests(house, buf2, sizeof(buf2)), CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), buf1, CCNRM(d->character, C_NRM),
           //	CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM), CBGRN(d->character, C_NRM), buf2, CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM),
           CBCYN(d->character, C_NRM), CCNRM(d->character, C_NRM));
  send_to_char(d->character, "%s", buf);

  OLC_MODE(d) = HSEDIT_MAIN_MENU;
}

/**************************************************************************
  The main loop
 **************************************************************************/

void hsedit_parse(struct descriptor_data *d, char *arg)
{
  int number = 0, id = 0, i, room_rnum;
  char *tmp;
  bool found = FALSE;

  if (!d)
    return;

  if (!d->character)
    return;

  switch (OLC_MODE(d))
  {
  case HSEDIT_CONFIRM_SAVESTRING:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      hsedit_save_internally(d);
      mudlog(CMP, LVL_BUILDER, TRUE, "OLC: %s edits house %d", GET_NAME(d->character), OLC_NUM(d));
      if (CONFIG_OLC_SAVE)
      {
        hsedit_save_to_disk();
        write_to_output(d, "House saved to disk.\r\n");
      }
      else
        write_to_output(d, "House saved to memory.\r\n");
      /*. Do NOT free strings! just the room structure .*/
      cleanup_olc(d, CLEANUP_STRUCTS);
      break;
    case 'n':
    case 'N':
      /* free everything up, including strings etc */
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      send_to_char(d->character, "Invalid choice!\r\n");
      send_to_char(d->character, "Do you wish to save this house internally? : ");
      break;
    }
    return;

  case HSEDIT_MAIN_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      if (OLC_VAL(d))
      { /*. Something has been modified .*/
        send_to_char(d->character, "Do you wish to save this house internally? : ");
        OLC_MODE(d) = HSEDIT_CONFIRM_SAVESTRING;
      }
      else
        cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      hsedit_owner_menu(d);
      break;

    case '2':
      if ((OLC_HOUSE(d)->vnum == NOWHERE) || (real_room(OLC_HOUSE(d)->vnum) == NOWHERE))
      {
        send_to_char(d->character, "ERROR: Invalid house VNUM\r\n(Press Enter)\r\n");
        mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: Invalid house VNUM in hsedit");
      }
      else
      {
        send_to_char(d->character, "Enter atrium room vnum:");
        OLC_MODE(d) = HSEDIT_ATRIUM;
      }
      break;

    case '3':
      if ((OLC_HOUSE(d)->vnum == NOWHERE) || (real_room(OLC_HOUSE(d)->vnum) == NOWHERE))
      {
        send_to_char(d->character, "ERROR: Invalid house VNUM\r\n(Press Enter)\r\n");
        mudlog(NRM, LVL_GRSTAFF, TRUE, "SYSERR: Invalid house VNUM in hsedit");
      }
      else
      {
        hsedit_dir_menu(d);
      }
      break;

    case '4':
      hsedit_disp_type_menu(d);
      break;

    case '5':
      send_to_char(d->character, "Set build date to now? (Y/N):");
      OLC_MODE(d) = HSEDIT_BUILD_DATE;
      break;

    case '6':
      send_to_char(d->character, "Set last payment as now? (Y/N) : ");
      OLC_MODE(d) = HSEDIT_PAYMENT;
      break;

    case '7':
      hsedit_disp_guest_menu(d);
      break;

    case '8':
      hsedit_disp_flags_menu(d);
      break;

      //  case '9':
      //    hsedit_disp_val0_menu(d);
      //    break;

    case 'x':
    case 'X':
      send_to_char(d->character, "Are you sure you want to delete this house? (Y/N) : ");
      OLC_MODE(d) = HSEDIT_DELETE;
      break;

    default:
      send_to_char(d->character, "Invalid choice!\r\n");
      hsedit_disp_menu(d);
      break;
    }
    return;

  case HSEDIT_OWNER_MENU:
    switch (*arg)
    {
    case '1':
      send_to_char(d->character, "Enter the name of the owner : ");
      OLC_MODE(d) = HSEDIT_OWNER_NAME;
      return;

    case '2':
      send_to_char(d->character, "Enter the user id of the owner : ");
      OLC_MODE(d) = HSEDIT_OWNER_ID;
      return;

    case 'Q':
      hsedit_disp_menu(d);
      break;
    }
    break;

  case HSEDIT_OWNER_NAME:
    if ((id = get_id_by_name(arg)) < 0)
    {
      send_to_char(d->character, "There is no such player.\r\n");
      hsedit_owner_menu(d);
      return;
    }
    else
    {
      OLC_HOUSE(d)->owner = id;
    }
    break;

  case HSEDIT_OWNER_ID:
    id = atoi(arg);
    if ((tmp = get_name_by_id(id)) == NULL)
    {
      send_to_char(d->character, "There is no such player.\r\n");
      hsedit_owner_menu(d);
      return;
    }
    else
    {
      OLC_HOUSE(d)->owner = id;
    }
    break;

  case HSEDIT_ATRIUM:
    number = atoi(arg);
    if (number == 0)
    {
      /* '0' chosen - go back to main menu */
      hsedit_disp_menu(d);
      return;
    }
    room_rnum = real_room(OLC_HOUSE(d)->vnum);
    if (real_room(number) == NOWHERE)
    {
      send_to_char(d->character, "Room VNUM does not exist.\r\nEnter a valid room VNUM for this atrium (0 to exit) : ");
      return;
    }
    else
    {
      for (i = 0; i < 6; i++)
      {
        if (world[room_rnum].dir_option[i])
        {
          if (world[room_rnum].dir_option[i]->to_room == real_room(number))
          {
            found = TRUE;
            id = i;
          }
        }
      }

      if (found == FALSE)
      {
        send_to_char(d->character, "Atrium MUST be an adjoining room.\r\nEnter a valid room VNUM for this atrium (0 to exit) : ");
        return;
      }
      else
      {
        OLC_HOUSE(d)->atrium = number;
        OLC_HOUSE(d)->exit_num = id;
      }
    }
    break;

  case HSEDIT_DIR_MENU:

    number = atoi(arg) - 1;

    if ((*arg == 'q') || (*arg == 'Q') || (number == -1))
    {
      hsedit_disp_menu(d);
      return;
    }
    if ((number < 0) || (number > 5))
    {
      send_to_char(d->character, "Invalid choice, Please select a direction (1-6, Q to quit) : ");
      return;
    }
    id = real_room(OLC_HOUSE(d)->vnum);
    if (!(world[id].dir_option[number]))
    {
      send_to_char(d->character, "%sYou cannot set the atrium to a room that doesn't exist!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      hsedit_dir_menu(d);
      return;
    }
    else if ((world[id].dir_option[number]->to_room) == NOWHERE)
    {
      send_to_char(d->character, "%sYou cannot set the atrium to nowhere!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      hsedit_dir_menu(d);
      return;
    }
    else
    {
      OLC_HOUSE(d)->exit_num = number;

      room_rnum = world[id].dir_option[number]->to_room;
      OLC_HOUSE(d)->atrium = world[room_rnum].number;
    }
    break;

  case HSEDIT_NOVNUM:
    /* Just an 'enter' keypress - don't do anything */
    break;

  case HSEDIT_BUILD_DATE:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      OLC_HOUSE(d)->built_on = time(0);
      break;
    case 'n':
    case 'N':
      send_to_char(d->character, "Build Date not changed\r\n");
      hsedit_disp_menu(d);
      return;
      break;
    }
    break;

  case HSEDIT_DELETE:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      hsedit_delete_house(d, OLC_HOUSE(d)->vnum);
      return;
    case 'n':
    case 'N':
      send_to_char(d->character, "House not deleted!\r\n");
      hsedit_disp_menu(d);
      return;
      break;
    }
    break;

  case HSEDIT_BUILDER:
    if ((id = get_id_by_name(arg)) < 0)
    {
      send_to_char(d->character, "No such player.\r\n");
      return;
    }
    else
    {
      OLC_HOUSE(d)->builtby = id;
      send_to_char(d->character, "Builder changed.\r\n");
    }
    break;

  case HSEDIT_PAYMENT:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      OLC_HOUSE(d)->last_payment = time(0);
      break;
    case 'n':
    case 'N':
      send_to_char(d->character, "Last Payment Date not changed\r\n");
      hsedit_disp_menu(d);
      return;
      break;
    }
    break;

  case HSEDIT_GUEST_MENU:
    switch (*arg)
    {
    case 'a':
    case 'A':
      if (OLC_HOUSE(d)->num_of_guests > (MAX_GUESTS - 1))
      {
        send_to_char(d->character, "%sGuest List Full! - delete some before adding more%s\r\nEnter selection (A/D/C/Q) : ", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      else
      {
        send_to_char(d->character, "Name of guest to add: ");
        OLC_MODE(d) = HSEDIT_GUEST_ADD;
      }
      break;

    case 'd':
    case 'D':
      if (OLC_HOUSE(d)->num_of_guests < 1)
      {
        send_to_char(d->character, "%sGuest List Empty! - add a guest before trying to delete one%s\r\nEnter selection (A/D/C/Q) : ", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      else
      {
        send_to_char(d->character, "Name of guest to delete : ");
        OLC_MODE(d) = HSEDIT_GUEST_DELETE;
      }
      break;

    case 'c':
    case 'C':
      send_to_char(d->character, "Clear guest list? (Y/N) : ");
      OLC_MODE(d) = HSEDIT_GUEST_CLEAR;
      break;

    case 'q':
    case 'Q':
      hsedit_disp_menu(d);
      break;

    default:
      send_to_char(d->character, "Invalid choice!\r\n\r\n");
      hsedit_disp_guest_menu(d);
      break;
    }
    return;
    break;

  case HSEDIT_GUEST_ADD:
    if ((id = get_id_by_name(arg)) < 0)
    {
      send_to_char(d->character, "No such player.\r\n");
      hsedit_disp_guest_menu(d);
      return;
    }
    else if (id == GET_IDNUM(d->character))
    {
      send_to_char(d->character, "House owner should not be in the guest list!\r\n");
      hsedit_disp_guest_menu(d);
      return;
    }
    else
    {
      for (i = 0; i < OLC_HOUSE(d)->num_of_guests; i++)
      {
        if (OLC_HOUSE(d)->guests[i] == id)
        {
          send_to_char(d->character, "That player is already in the guest list!.\r\n");
          hsedit_disp_guest_menu(d);
          return;
        }
      }
      i = OLC_HOUSE(d)->num_of_guests++;
      OLC_HOUSE(d)->guests[i] = id;
      send_to_char(d->character, "Guest added.\r\n");
      hsedit_disp_guest_menu(d);
      return;
    }
    break;

  case HSEDIT_GUEST_DELETE:
    if ((id = get_id_by_name(arg)) < 0)
    {
      send_to_char(d->character, "No such player.\r\n");
      hsedit_disp_guest_menu(d);
      return;
    }
    else if (id == GET_IDNUM(d->character))
    {
      send_to_char(d->character, "House owner should not be in the guest list!\r\n");
      hsedit_disp_guest_menu(d);
      return;
    }
    else
    {
      for (i = 0; i < OLC_HOUSE(d)->num_of_guests; i++)
      {
        if (OLC_HOUSE(d)->guests[i] == id)
        {
          for (; i < OLC_HOUSE(d)->num_of_guests; i++)
            OLC_HOUSE(d)->guests[i] = OLC_HOUSE(d)->guests[i + 1];
          OLC_HOUSE(d)->num_of_guests--;
          send_to_char(d->character, "Guest deleted.\r\n");
          OLC_VAL(d) = 1;
          hsedit_disp_guest_menu(d);
          return;
        }
      }
      send_to_char(d->character, "That player isn't in the guest list!\r\n");
      hsedit_disp_guest_menu(d);
      return;
    }
    break;

  case HSEDIT_GUEST_CLEAR:
    switch (*arg)
    {
    case 'n':
    case 'N':
      send_to_char(d->character, "Guest List not cleared!\r\n");
      hsedit_disp_guest_menu(d);
      return;

    case 'y':
    case 'Y':
      OLC_HOUSE(d)->num_of_guests = 0;
      for (i = 0; i < MAX_GUESTS; i++)
        OLC_HOUSE(d)->guests[i] = 0;

      send_to_char(d->character, "Guest List Cleared!\r\n");
      OLC_VAL(d) = 1;
      hsedit_disp_guest_menu(d);
      return;

    default:
      send_to_char(d->character, "Invalid choice!\r\nClear Guest List? (Y/N) : ");
      return;
    }
    break;

  case HSEDIT_TYPE:
    number = atoi(arg);
    if (number < 0 || number >= NUM_HOUSE_TYPES)
    {
      send_to_char(d->character, "Invalid choice!");
      hsedit_disp_type_menu(d);
      return;
    }
    else
      OLC_HOUSE(d)->mode = number;
    break;

  case HSEDIT_FLAGS:
    number = atoi(arg);
    if ((number < 0) || (number > HOUSE_NUM_FLAGS))
    {
      send_to_char(d->character, "That's not a valid choice!\r\n");
      hsedit_disp_flags_menu(d);
    }
    else
    {
      if (number == 0)
        break;
      else
      {
        /* toggle bits */
        if (IS_SET(OLC_HOUSE(d)->bitvector, 1 << (number - 1)))
          REMOVE_BIT(OLC_HOUSE(d)->bitvector, 1 << (number - 1));
        else
          SET_BIT(OLC_HOUSE(d)->bitvector, 1 << (number - 1));
        hsedit_disp_flags_menu(d);
      }
    }
    return;

    /* Houses have no 'values', but this is left commented out for future expansion
        case HSEDIT_VALUE_0:
          number = atoi(arg);
          OLC_HOUSE(d)->value[0] = number;
          hsedit_disp_val1_menu(d);
          return;

        case HSEDIT_VALUE_1:
          OLC_HOUSE(d)->value[1] = atoi(arg);
          hsedit_disp_val2_menu(d);
          return;

        case HSEDIT_VALUE_2:
          OLC_HOUSE(d)->value[2] = atoi(arg);
          hsedit_disp_val3_menu(d);
          return;

        case HSEDIT_VALUE_3:
          OLC_HOUSE(d)->value[3] = atoi(arg);
          break;
       */

  default:
    /* we should never get here */
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Reached default case in parse_hsedit");
    break;
  }
  /*. If we get this far, something has been changed .*/
  OLC_VAL(d) = 1;
  hsedit_disp_menu(d);
}

void hsedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d))
  {
    /* There are no strings to be edited in houses - if there are any added later, add them here */
  }
}

ACMD(do_oasis_hsedit)
{
  int number = NOWHERE, save = 0;
  house_rnum real_num;
  struct descriptor_data *d;
  char buf1[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};

  /****************************************************************************/
  /** Parse any arguments.                                                   **/
  /****************************************************************************/
  two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

  /****************************************************************************/
  /** If there aren't any arguments...grab the number of the current room... **/
  /****************************************************************************/
  if (!*buf1)
  {
    number = GET_ROOM_VNUM(IN_ROOM(ch));
  }
  else if (!isdigit(*buf1))
  {
    if (str_cmp("save", buf1) != 0)
    {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    save = TRUE;

    if (is_number(buf2))
      number = atoi(buf2);
    else if (GET_OLC_ZONE(ch) > 0)
    {
      zone_rnum zlok;

      if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(zlok);
    }

    if (number == NOWHERE)
    {
      send_to_char(ch, "Save which zone?\r\n");
      return;
    }
  }

  /****************************************************************************/
  /** If a numeric argument was given, get it.                               **/
  /****************************************************************************/
  if (number == NOWHERE)
    number = atoi(buf1);

  /****************************************************************************/
  /** Check that whatever it is isn't already being edited.                  **/
  /****************************************************************************/
  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) == CON_HSEDIT)
    {
      if (d->olc && OLC_NUM(d) == number)
      {
        send_to_char(ch, "That house is currently being edited by %s.\r\n",
                     PERS(d->character, ch));
        return;
      }
    }
  }

  /****************************************************************************/
  /** Point d to the builder's descriptor (for easier typing later).         **/
  /****************************************************************************/
  d = ch->desc;

  /****************************************************************************/
  /** Give the descriptor an OLC structure.                                  **/
  /****************************************************************************/
  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_oasis_hsedit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  /****************************************************************************/
  /** Find the zone.                                                         **/
  /****************************************************************************/
  OLC_ZNUM(d) = save ? real_zone(number) : real_zone_by_thing(number);
  if (OLC_ZNUM(d) == NOWHERE)
  {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                 **/
    /**************************************************************************/
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /****************************************************************************/
  /** Everyone but IMPLs can only edit zones they have been assigned.        **/
  /****************************************************************************/
  if (!can_edit_zone(ch, OLC_ZNUM(d)))
  {
    send_to_char(ch, " You do not have permission to edit zone %d. Try zone %d.\r\n", zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
    mudlog(BRF, LVL_IMPL, TRUE, "OLC: %s tried to edit zone %d allowed zone %d",
           GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                 **/
    /**************************************************************************/
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /****************************************************************************/
  /** If we need to save, save the houses.                                  **/
  /****************************************************************************/
  if (save)
  {
    send_to_char(ch, "Saving all houses in zone %d.\r\n",
                 zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE,
           "OLC: %s saves house info for zone %d.", GET_NAME(ch),
           zone_table[OLC_ZNUM(d)].number);

    /**************************************************************************/
    /** Save the houses in this zone.                                       **/
    /**************************************************************************/
    hsedit_save_to_disk();

    /**************************************************************************/
    /** Free the descriptor's OLC structure.                                 **/
    /**************************************************************************/
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;

  /****************************************************************************/
  /** If this is a new house, setup a new house, otherwise setup the        **/
  /** existing house.                                                       **/
  /****************************************************************************/
  real_num = find_house(number);
  if (real_num == NOWHERE)
  {
    /* Do a quick check to ensure there is room for more */
    if (num_of_houses >= MAX_HOUSES)
    {
      send_to_char(ch, "MAX houses limit reached (%d) - Unable to create more.\r\n", MAX_HOUSES);
      mudlog(NRM, LVL_IMPL, TRUE, "HSEDIT: MAX houses limit reached (%d)\r\n", MAX_HOUSES);
      return;
    }
    else
    {
      hsedit_setup_new(d);
    }
  }
  else
  {
    hsedit_setup_existing(d, real_num);
  }

  STATE(d) = CON_HSEDIT;

  /****************************************************************************/
  /** Send the OLC message to the players in the same room as the builder.   **/
  /****************************************************************************/
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  /****************************************************************************/
  /** Log the OLC message.                                                   **/
  /****************************************************************************/
  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: (hsedit) %s starts editing zone %d allowed zone %d",
         GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}
