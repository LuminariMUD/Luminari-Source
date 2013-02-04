
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
static void favored_enemy_submenu(struct descriptor_data *d, int favored);
static void favored_enemy_menu(struct descriptor_data *d);
static void animal_companion_menu(struct descriptor_data *d);
/*-------------------------------------------------------------------*/

// global
int global_circle = -1;  // keep track of circle as we navigate menus
int global_class = -1;   // keep track of class as we navigate menus
int favored_slot = -1;

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

/* ranger study sub-sub-menu:  select list of favored enemies (sub) */
static void favored_enemy_submenu(struct descriptor_data *d, int favored)
{
  get_char_colors(d->character);
  clear_screen(d);
  
  write_to_output(d,
    "\r\n-- %sRanger Favored Enemy Sub-Menu%s\r\n"
    "Slot:  %d\r\n"
    "\r\n"
    "%s"
    "\r\n"
    "Current Favored Enemy:  %s\r\n"
    "You can select 0 (Zero) to deselect an enemy for this slot.\r\n"
    "\r\n"
    "%s Q%s) Quit\r\n"
    "\r\n"
    "Enter Choice : ",

    mgn, nrm,
          favored,
          /* empty line */
          npc_race_menu,
          /* empty line */
          npc_race_abbrevs[GET_FAVORED_ENEMY(d->character, favored)],
    grn, nrm
    /* empty line */
    );
  
  OLC_MODE(d) = FAVORED_ENEMY_SUB;  
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
    "%s Q%s) Quit\r\n"
    "\r\n"
    "Enter Choice : ",

    mgn, nrm,
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
    grn, nrm
    /* empty line */
    );
  
  OLC_MODE(d) = FAVORED_ENEMY;  
}


/* list of possible animal companions, use in-game vnums for this */
#define DIRE_BADGER    60
#define DIRE_BOAR      61
#define DIRE_WOLF      62
#define DIRE_SPIDER    63
#define DIRE_BEAR      64
#define DIRE_TIGER     65
/*--------*/
#define MOB_PALADIN_MOUNT 70
/* make a list of vnums corresponding in order */
int animal_vnums[] = {
  0,
  DIRE_BADGER,   // 1
  DIRE_BOAR,     // 2
  DIRE_WOLF,     // 3
  DIRE_SPIDER,   // 4
  DIRE_BEAR,     // 5
  DIRE_TIGER,    // 6
  -1   /* end with this */
};
#define NUM_ANIMALS 7
/* make a list of names in order */
char *animal_names[] = {
  "Unknown",
  "1) Dire Badger",
  "2) Dire Boar",
  "3) Dire Wolf",
  "4) Dire Spider",
  "5) Dire Bear",
  "6) Dire Tiger",
  "\n"   /* end with this */
  
};
/* ranger study sub-menu:  adjust animal companion */
static void animal_companion_menu(struct descriptor_data *d)
{
  int i = 1, found = 0;
  
  get_char_colors(d->character);
  clear_screen(d);
    
  write_to_output(d,
    "\r\n-- %sRanger Animal Companion Menu%s\r\n"
    "\r\n", mgn, nrm);

  for (i = 1; animal_vnums[i] != -1; i++) {
    write_to_output(d, "%s\r\n", animal_names[i]);
  }
  
  write_to_output(d, "\r\n");
  /* find current animal */
  for (i = 1; animal_vnums[i] != -1; i++) {
    if (GET_ANIMAL_COMPANION(d->character) == animal_vnums[i]) {
      write_to_output(d, "Current Companion:  %s\r\n", animal_names[i]);
      found = 1;
      break;
    }
  }
  
  if (!found)
    write_to_output(d, "Current No Companion Selected\r\n");
  
  write_to_output(d, "You can select 0 (Zero) to deselect the current "
          "companion.\r\n");
  write_to_output(d, "\r\n"
                     "%s Q%s) Quit\r\n"
                     "\r\n"
                     "Enter Choice : ",
                     grn, nrm
                     );
  
  OLC_MODE(d) = ANIMAL_COMPANION;    
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
          OLC_MODE(d) = RANG_MAIN_MENU;
          return;          
          
        default:                
          number = atoi(arg);
          int ranger_level = CLASS_LEVEL(d->character, CLASS_RANGER);
          
          switch (number) {
            case 0:
              if (ranger_level) {
                favored_slot = number;
                favored_enemy_submenu(d, number);
                OLC_MODE(d) = FAVORED_ENEMY_SUB;
                return;
              }
              break;                
            case 1:
              if (ranger_level >= 5) {
                favored_slot = number;
                favored_enemy_submenu(d, number);
                OLC_MODE(d) = FAVORED_ENEMY_SUB;
                return;
              } else {
                write_to_output(d, "You are not a high enough level ranger to"
                        " modify this slot!\r\n");
              }
              break;                
            case 2:
              if (ranger_level >= 10) {
                favored_slot = number;
                favored_enemy_submenu(d, number);
                OLC_MODE(d) = FAVORED_ENEMY_SUB;
                return;
              } else {
                write_to_output(d, "You are not a high enough level ranger to"
                        "modify this slot!\r\n");
              }
              break;                
            case 3:
              if (ranger_level >= 15) {
                favored_slot = number;
                favored_enemy_submenu(d, number);
                OLC_MODE(d) = FAVORED_ENEMY_SUB;
                return;
              } else {
                write_to_output(d, "You are not a high enough level ranger to"
                        "modify this slot!\r\n");
              }
              break;                
            case 4:
              if (ranger_level >= 20) {
                favored_slot = number;
                favored_enemy_submenu(d, number);
                OLC_MODE(d) = FAVORED_ENEMY_SUB;
                return;
              } else {
                write_to_output(d, "You are not a high enough level ranger to"
                        "modify this slot!\r\n");
              }
              break;                
            case 5:
              if (ranger_level >= 25) {
                favored_slot = number;
                favored_enemy_submenu(d, number);
                OLC_MODE(d) = FAVORED_ENEMY_SUB;
                return;
              } else {
                write_to_output(d, "You are not a high enough level ranger to"
                        "modify this slot!\r\n");
              }
              break;                
            case 6:
              if (ranger_level >= 30) {
                favored_slot = number;
                favored_enemy_submenu(d, number);
                OLC_MODE(d) = FAVORED_ENEMY_SUB;
                return;
              } else {
                write_to_output(d, "You are not a high enough level ranger to"
                        "modify this slot!\r\n");
              }
              break;                
            case 7:
            case 8:
            case 9:
              write_to_output(d, "This slot is not currently modifyable.\r\n");
              break;
              
          }
          
          OLC_MODE(d) = FAVORED_ENEMY;
          favored_enemy_menu(d);
          break;          
      }      
      OLC_MODE(d) = FAVORED_ENEMY;
      break;

    case FAVORED_ENEMY_SUB:
      switch (*arg) {
        case 'q':
        case 'Q':
          favored_enemy_menu(d);
          OLC_MODE(d) = FAVORED_ENEMY;
          return;          
          
        default:    
          number = atoi(arg);
          
          if (number < 0 || number >= NUM_NPC_RACES)
            write_to_output(d, "Invalid race!\r\n");
          else {
            GET_FAVORED_ENEMY(d->character, favored_slot) =
                    number;
            favored_enemy_menu(d);
            OLC_MODE(d) = FAVORED_ENEMY;
            return;                      
          }

          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          favored_enemy_submenu(d, favored_slot);
          break;                    
      }
      OLC_MODE(d) = FAVORED_ENEMY_SUB;
      break;
      
    case ANIMAL_COMPANION:
      switch (*arg) {
        case 'q':
        case 'Q':
          ranger_disp_menu(d);
          OLC_MODE(d) = RANG_MAIN_MENU;
          return;          

        default:                
          number = atoi(arg);
          
          if (!number) {
            GET_ANIMAL_COMPANION(d->character) = number;
            write_to_output(d, "Your companion has been set to OFF.\r\n");
          } else if (number < 0 || number >= NUM_ANIMALS) {
            write_to_output(d, "Not a valid choice!\r\n");            
          } else {
            GET_ANIMAL_COMPANION(d->character) = animal_vnums[number];
            write_to_output(d, "You have selected %s.\r\n",
                    animal_names[number]);
          }
          
          OLC_MODE(d) = ANIMAL_COMPANION;
          animal_companion_menu(d);
          break;          
      }      
      OLC_MODE(d) = ANIMAL_COMPANION;
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

/* some undefines from animal companion */
#undef DIRE_BADGER
#undef DIRE_BOAR
#undef DIRE_WOLF
#undef DIRE_SPIDER
#undef DIRE_BEAR
#undef DIRE_TIGER
#undef NUM_ANIMALS
#undef MOB_PALADIN_MOUNT



