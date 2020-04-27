/**
* @file house.h                                    Part of LuminariMUD
* Player house structures, prototypes and defines.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               
*/
#ifndef _HOUSE_H_
#define _HOUSE_H_

/* NOTE: learned the hard way, changing (one or both) of these will destroy the houses in the
         game apparently -Zusuk */
#define MAX_HOUSES 999
#define MAX_GUESTS 99

#define HOUSE_PRIVATE 0
#define HOUSE_GOD 1  /* Imm owned house */
#define HOUSE_CLAN 2 /* Clan crash-save room */
#define NUM_HOUSE_TYPES 3

struct house_control_rec
{
   room_vnum vnum;          /* vnum of this house		*/
   room_vnum atrium;        /* vnum of atrium		*/
   sh_int exit_num;         /* direction of house's exit	*/
   time_t built_on;         /* date this house was built	*/
   int mode;                /* mode of ownership		*/
   long owner;              /* idnum of house's owner	*/
   int num_of_guests;       /* how many guests for house	*/
   long guests[MAX_GUESTS]; /* idnums of house's guests	*/
   time_t last_payment;     /* date of last house payment   */
   long bitvector;          /* bitvector for the house */
   long builtby;            /* who created this hosue */
   long spare2;
   long spare3;
   long spare4;
   long spare5;
   long spare6;
   long spare7;
};

/* House can have up to 31 bitvectors - don't go higher */
#define HOUSE_NOGUESTS (1 << 0)   /* Owner cannot add guests                    */
#define HOUSE_FREE (1 << 1)       /* House does not require payments            */
#define HOUSE_NOIMMS (1 << 2)     /* Imms below level 33 cannot enter          */
#define HOUSE_IMPONLY (1 << 3)    /* Imms below level 34 cannot enter         */
#define HOUSE_RENTFREE (1 << 4)   /* No rent is charged on items left here      */
#define HOUSE_SAVENORENT (1 << 5) /* NORENT items are crashsaved too            */
#define HOUSE_NOSAVE (1 << 6)     /* Do not crash save this room - private only */

#define HOUSE_NUM_FLAGS 7

#define HOUSE_FLAGS(house) (house).bitvector
#define TOROOM(room, dir) (world[room].dir_option[dir] ? world[room].dir_option[dir]->to_room : NOWHERE)

/* Functions in house.c made externally available */
/* Utility Functions */
void House_boot(void);
void House_save_all(void);
int House_can_enter(struct char_data *ch, room_vnum house);
void House_crashsave(room_vnum vnum);
void House_list_guests(struct char_data *ch, int i, int quiet);
int House_save(struct obj_data *obj, room_vnum vnum, FILE *fp, int location);
void hcontrol_list_houses(struct char_data *ch, char *arg);
/* In game Commands */
ACMD_DECL(do_hcontrol);
ACMD_DECL(do_house);

#endif /* _HOUSE_H_ */
