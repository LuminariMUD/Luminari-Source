
/* ***********************************************************************
*    File:   study.c                                Part of LuminariMUD  *
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
#include "screen.h"
#include "interpreter.h"
#include "modify.h"
#include "spells.h"

/*-------------------------------------------------------------------*/
/*. Function prototypes . */

static void sorc_disp_menu(struct descriptor_data *d);
static void ranger_disp_menu(struct descriptor_data *d);
static void sorc_study_menu(struct descriptor_data *d, int circle);
static void favored_enemy_menu(struct descriptor_data *d);
static void animal_companion_menu(struct descriptor_data *d);
/*-------------------------------------------------------------------*/

// global
int global_circle = -1;  // keep track of circle as we navigate menus
int global_class = -1;   // keep track of class as we navigate menus


/*-------------------------------------------------------------------*\
  utility functions
 \*-------------------------------------------------------------------*/

ACMD(do_study)
{
  struct descriptor_data *d = NULL;
  int class = -1;

  if (!argument) {
    send_to_char(ch, "Specify a class to edit known spells.\r\n");
    return;
  } else if (is_abbrev(argument, " sorcerer")) {
    if (IS_SORC_LEARNED(ch) && GET_LEVEL(ch) < LVL_IMPL) {
      send_to_char(ch, "You can only modify your 'known' list once per level.\r\n"
                       "(You can also RESPEC to reset your character)\r\n");
      return;
    }
    if (!CLASS_LEVEL(ch, CLASS_SORCERER)) {
      send_to_char(ch, "How?  You are not a sorcerer!\r\n");
      return;
    }
    class = CLASS_SORCERER;
  } else if (is_abbrev(argument, " ranger")) {
    if (IS_RANG_LEARNED(ch) && GET_LEVEL(ch) < LVL_IMPL) {
      send_to_char(ch, "You already adjusted your ranger "
            "skills this level.\r\n");
      return;
    }
    class = CLASS_RANGER;
  } else {
    send_to_char(ch, "Usage:  study <class name>\r\n");
    return;
  }
  
  if (class == -1) {
    send_to_char(ch, "Invalid class!\r\n");
    return;
  }

  d = ch->desc;

  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE,
      "SYSERR: do_study: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  STATE(d) = CON_STUDY;

  act("$n starts adjust studying $s skill-set.",
          TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
  
  if (class == CLASS_SORCERER) {
    global_class = CLASS_SORCERER;
    sorc_disp_menu(d);    
  } else if (class == CLASS_RANGER) {
    global_class = CLASS_RANGER;
    ranger_disp_menu(d);
  }

}

/*-------------------------------------------------------------------*/

/**************************************************************************
 Menu functions
**************************************************************************/

/*-------------------------------------------------------------------*/
/*. Display main menu, Sorcerer . */

static void sorc_disp_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  
  write_to_output(d,
    "\r\n-- %sSpells Known Menu\r\n"
    "\r\n"
    "%s 1%s) 1st Circle     : %s%d\r\n"
    "%s 2%s) 2nd Circle     : %s%d\r\n"
    "%s 3%s) 3rd Circle     : %s%d\r\n"
    "%s 4%s) 4th Circle     : %s%d\r\n"
    "%s 5%s) 5th Circle     : %s%d\r\n"
    "%s 6%s) 6th Circle     : %s%d\r\n"
    "%s 7%s) 7th Circle     : %s%d\r\n"
    "%s 8%s) 8th Circle     : %s%d\r\n"         
    "%s 9%s) 9th Circle     : %s%d\r\n"
    "%s Q%s) Quit\r\n"
    "\r\n"
    "%sWhen you quit it finalizes all changes%s\r\n"
    "%sYour 'known spells' can only be modified once per level%s\r\n"
    "\r\n"
    "Enter Choice : ",

    mgn,
    grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][0] -
          count_sorc_known(d->character, 1),
    grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][1] -
          count_sorc_known(d->character, 2),
    grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][2] -
          count_sorc_known(d->character, 3),
    grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][3] -
          count_sorc_known(d->character, 4),
    grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][4] -
          count_sorc_known(d->character, 5),
    grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][5] -
          count_sorc_known(d->character, 6),
    grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][6] -
          count_sorc_known(d->character, 7),
    grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][7] -
          count_sorc_known(d->character, 8),
    grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][8] -
          count_sorc_known(d->character, 9),
    grn, nrm,
    mgn, nrm,
    mgn, nrm
          );
  
  OLC_MODE(d) = SORC_MAIN_MENU;
}


/* the menu for each circle, sorcerer */


void sorc_study_menu(struct descriptor_data *d, int circle)
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
        write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
            spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(d, "\r\n");
  write_to_output(d, "%sNumber of slots availble:%s %d.\r\n", grn, nrm,
      sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][circle - 1] -
      count_sorc_known(d->character, circle));
  write_to_output(d, "%sEnter spell choice, to add or remove "
          "(Q to exit to main menu) : ", nrm);
  
  OLC_MODE(d) = STUDY_SPELLS;
}

/***********************end sorcerer******************************************/

/***************************/
/* main menu for ranger */
/***************************/
static void ranger_disp_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  
  write_to_output(d,
    "\r\n-- %sRanger Skill Menu\r\n"
    "\r\n"
    "\r\n"
    "%s 1%s) Favored Enemy Menu\r\n"
    "\r\n"
    "%s 2%s) Animal Companion Menu\r\n"
    "\r\n"
    "\r\n"
    "%s Q%s) Quit\r\n"
    "\r\n"
    "%sWhen you quit it finalizes all changes%s\r\n"
    "%sYour ranger skills can only be modified once per level%s\r\n"
    "\r\n"
    "Enter Choice : ",

    mgn,
          /* empty line */
          /* empty line */
    grn, nrm,
    /* empty line */
    grn, nrm,
    /* empty line */
    /* empty line */
    grn, nrm,
    mgn, nrm,
    mgn, nrm
    );
  
  OLC_MODE(d) = RANG_MAIN_MENU;
}

/* ranger study sub-menu:  select list of favored enemies */
static void favored_enemy_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  
  write_to_output(d,
    "\r\n-- %sRanger Favored Enemy Menu%s\r\n"
    "\r\n"
    "\r\n"
    "%s 0%s) Favored Enemy #1  (Min. Level  1):  %s%s\r\n"
    "%s 1%s) Favored Enemy #2  (Min. Level  5):  %s%s\r\n"
    "%s 2%s) Favored Enemy #3  (Min. Level 10):  %s%s\r\n"
    "%s 3%s) Favored Enemy #4  (Min. Level 15):  %s%s\r\n"
    "%s 4%s) Favored Enemy #5  (Min. Level 20):  %s%s\r\n"
    "%s 5%s) Favored Enemy #6  (Min. Level 25):  %s%s\r\n"
    "%s 6%s) Favored Enemy #7  (Min. Level 30):  %s%s\r\n"
    "%s 7%s) Favored Enemy #8  (Min. Level xx):  %s%s\r\n"
    "%s 8%s) Favored Enemy #9  (Min. Level xx):  %s%s\r\n"
    "%s 9%s) Favored Enemy #10 (Min. Level xx):  %s%s\r\n"
    "\r\n"
    "\r\n"
    "%s Q%s) Quit\r\n"
    "\r\n"
    "\r\n"
    "Enter Choice : ",

    mgn, nrm,
          /* empty line */
          /* empty line */
    grn, nrm, npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, 0)], nrm,
    grn, nrm, npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, 1)], nrm,
    grn, nrm, npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, 2)], nrm,
    grn, nrm, npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, 3)], nrm,
    grn, nrm, npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, 4)], nrm,
    grn, nrm, npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, 5)], nrm,
    grn, nrm, npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, 6)], nrm,
    grn, nrm, npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, 7)], nrm,
    grn, nrm, npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, 8)], nrm,
    grn, nrm, npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, 9)], nrm,
    /* empty line */
    /* empty line */
    grn, nrm
    /* empty line */
    /* empty line */
    );
  
  OLC_MODE(d) = RANG_MAIN_MENU;  
}

/* ranger study sub-menu:  adjust animal companion */
static void animal_companion_menu(struct descriptor_data *d)
{
  
}

/*********************** end ranger ****************************************/


/**************************************************************************
  The handler
**************************************************************************/


void study_parse(struct descriptor_data *d, char *arg)
{
  int number = -1;
  int counter;    

  switch (OLC_MODE(d)) {
    
    /******* start sorcerer **********/

    case SORC_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q':
          write_to_output(d, "Your choices have been finalized!\r\n\r\n");
          if (global_class == CLASS_SORCERER)
            IS_SORC_LEARNED(d->character) = 1;
          save_char(d->character, 0);
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
          sorc_study_menu(d, atoi(arg));
          OLC_MODE(d) = STUDY_SPELLS;
          break;
        default:
          write_to_output(d, "That is an invalid choice!\r\n");
          sorc_disp_menu(d);
          break;
      }
      break;
      
    case STUDY_SPELLS:
      switch (*arg) {
        case 'q':
        case 'Q':
          sorc_disp_menu(d);
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
          OLC_MODE(d) = SORC_MAIN_MENU;
          sorc_study_menu(d, global_circle);
          break;
        }
      break;
    /******* end sorcerer **********/
      
      
    /******* start ranger **********/

    case RANG_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q':
          write_to_output(d, "Your choices have been finalized!\r\n\r\n");
          if (global_class == CLASS_RANGER)
            IS_RANG_LEARNED(d->character) = 1;
          save_char(d->character, 0);
          cleanup_olc(d, CLEANUP_ALL);
          return;
        case '1':  // favored enemy choice
          favored_enemy_menu(d);
          OLC_MODE(d) = FAVORED_ENEMY;
          break;
        case '2':  // animal companion choice
          animal_companion_menu(d);
          OLC_MODE(d) = ANIMAL_COMPANION;
          break;
        default:
          write_to_output(d, "That is an invalid choice!\r\n");
          ranger_disp_menu(d);
          break;
      }
      break;
      
    case FAVORED_ENEMY:
      switch (*arg) {
        case 'q':
        case 'Q':
          ranger_disp_menu(d);
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
          
          OLC_MODE(d) = FAVORED_ENEMY;
          favored_enemy_menu(d);
          break;          
      }      


      break;
      
    case ANIMAL_COMPANION:
      switch (*arg) {
        case 'q':
        case 'Q':
          ranger_disp_menu(d);
          return;

        default:                
          number = atoi(arg);
          
          OLC_MODE(d) = ANIMAL_COMPANION;
          animal_companion_menu(d);
          break;          
      }      
      break;
    /******* end ranger **********/
      
    /* We should never get here, but just in case... */      
    default:
      cleanup_olc(d, CLEANUP_CONFIG);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: study_parse(): Reached default case!");
      write_to_output(d, "Oops...\r\n");
      break;      
  }
  /*-------------------------------------------------------------------*/
  /*. END OF CASE */
}



