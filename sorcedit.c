/* ***********************************************************************
*    File:   sorcedit.c                             Part of LuminariMUD  *
* Purpose:   To provide menus for sorc-type casters known spells         *
*            Header info in oasis.h                                      *
*  Author:   Zusuk                                                       *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "comm.h"
#include "db.h"
#include "oasis.h"
#include "improved-edit.h"
#include "screen.h"
#include "genolc.h"
#include "genzon.h"
#include "interpreter.h"
#include "modify.h"
#include "spells.h"

/*-------------------------------------------------------------------*/
/*. Function prototypes . */

static void sorcedit_disp_menu(struct descriptor_data *d);
void sorcedit_menu(struct descriptor_data *d, int circle);
/*-------------------------------------------------------------------*/

// global
int global_circle;


/*-------------------------------------------------------------------*\
  utility functions
 \*-------------------------------------------------------------------*/

ACMD(do_sorcedit)
{
  struct descriptor_data *d;

  if (!argument) {
    send_to_char(ch, "Specify a class to edit known spells.\r\n");
    return;
  } else if (is_abbrev(argument, " sorcerer")) {
    if (IS_SORC_LEARNED(ch)) {
      send_to_char(ch, "You already adjusted your sorcerer "
            "spells this level\r\n");
      return;
    }
  } else {
    send_to_char(ch, "This command is not available to you!\r\n");
    return;
  }

  d = ch->desc;

  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE,
      "SYSERR: do_sorcedit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  STATE(d) = CON_SORCEDIT;

  act("$n starts adjust $s spells known.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  
  sorcedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/**************************************************************************
 Menu functions
**************************************************************************/

/*-------------------------------------------------------------------*/
/*. Display main menu . */

static void sorcedit_disp_menu(struct descriptor_data *d)
{
  clear_screen(d);
  
  write_to_output(d,
    "\r\n-- \tCSpells Known Menu\r\n"
    "\tg 1\tn) 1st Circle     : \ty%d\r\n"
    "\tg 2\tn) 2nd Circle     : \ty%d\r\n"
    "\tg 3\tn) 3rd Circle     : \ty%d\r\n"
    "\tg 4\tn) 4th Circle     : \ty%d\r\n"
    "\tg 5\tn) 5th Circle     : \ty%d\r\n"
    "\tg 6\tn) 6th Circle     : \ty%d\r\n"
    "\tg 7\tn) 7th Circle     : \ty%d\r\n"
    "\tg 8\tn) 8th Circle     : \ty%d\r\n"         
    "\tg 9\tn) 9th Circle     : \ty%d\r\n"
    "\tg Q\tn) Quit\r\n"
    "***Note***  When you quit it finalizes all changes!\r\n"
    "Enter Choice : ",
    sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][0] -
          count_sorc_known(d->character, 1),
    sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][1] -
          count_sorc_known(d->character, 2),
    sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][2] -
          count_sorc_known(d->character, 3),
    sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][3] -
          count_sorc_known(d->character, 4),
    sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][4] -
          count_sorc_known(d->character, 5),
    sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][5] -
          count_sorc_known(d->character, 6),
    sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][6] -
          count_sorc_known(d->character, 7),
    sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][7] -
          count_sorc_known(d->character, 8),
    sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][8] -
          count_sorc_known(d->character, 9)
          );
  
  OLC_MODE(d) = SORCEDIT_MAIN_MENU;
}


/* the menu for each circle */


void sorcedit_menu(struct descriptor_data *d, int circle)
{
  int counter, columns = 0;

  global_circle = circle;
  
  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 1; counter < NUM_SPELLS; counter++) {
    if (spellCircle(CLASS_SORCERER, counter) == circle) {
      if (sorcKnown(d->character, counter))
        write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, mgn,
            spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
      else
        write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, mgn,
            spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(d, "\r\n%sEnter spell choice, to add or remove "
          "(Q to exit to main menu) : ", nrm);
  
  OLC_MODE(d) = SORCEDIT_SPELLS;
}


/**************************************************************************
  The handler
**************************************************************************/


void sorcedit_parse(struct descriptor_data *d, char *arg)
{
  int number = -1;
  int counter;    

  switch (OLC_MODE(d)) {

    case SORCEDIT_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q':
          cleanup_olc(d, CLEANUP_ALL);
          return;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          sorcedit_menu(d, atoi(arg));
          OLC_MODE(d) = SORCEDIT_SPELLS;
          break;
        default:
          write_to_output(d, "That is an invalid choice!\r\n");
          sorcedit_disp_menu(d);
          break;
      }
      break;
      
    case SORCEDIT_SPELLS:
      switch (*arg) {
        case 'q':
        case 'Q':
          sorcedit_disp_menu(d);
          return;

        default:
          number = atoi(arg);
      
          for (counter = 1; counter < NUM_SPELLS; counter++) {
            if (counter == number) {
              if (spellCircle(CLASS_SORCERER, counter) == global_circle) {
                if (sorcKnown(d->character, counter))
                  sorc_extract_known(d->character, counter);
                else if (!sorc_add_known(d->character, counter))
                  write_to_output(d, "You are all FULL for spells!\r\n");
              }
            }
          }
          OLC_MODE(d) = SORCEDIT_MAIN_MENU;
          sorcedit_menu(d, global_circle);
          break;
        }
      break;

    /* We should never get here, but just in case... */      
    default:
      cleanup_olc(d, CLEANUP_CONFIG);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: cedit_parse(): Reached default case!");
      write_to_output(d, "Oops...\r\n");
      break;      
  }
  /*-------------------------------------------------------------------*/
  /*. END OF CASE
  If we get here, we have probably changed something, and now want to
  return to main menu.  Use OLC_VAL as a 'has changed' flag . */

//  OLC_VAL(d) = 1;
//  sorcedit_disp_menu(d);
}

