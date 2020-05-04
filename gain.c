
/* ***********************************************************************
 *    File:   gain.c                                 Part of LuminariMUD  *
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

#undef NEWGAINREADY
#ifdef NEWGAINREADY
/*-------------------------------------------------------------------*/
/*. Function prototypes . */

/* New procedures for GAIN process - Ornir */
static void class_disp_menu(struct descriptor_data *d);
static void feat_disp_menu(struct descriptor_data *d);
static void skill_disp_menu(struct descriptor_data *d);

static void druid_disp_menu(struct descriptor_data *d);
static void sorc_disp_menu(struct descriptor_data *d);
static void bard_disp_menu(struct descriptor_data *d);
static void wizard_disp_menu(struct descriptor_data *d);
static void ranger_disp_menu(struct descriptor_data *d);
static void sorc_study_menu(struct descriptor_data *d, int circle);
static void bard_study_menu(struct descriptor_data *d, int circle);
static void favored_enemy_submenu(struct descriptor_data *d, int favored);
static void favored_enemy_menu(struct descriptor_data *d);
static void animal_companion_menu(struct descriptor_data *d);
static void familiar_menu(struct descriptor_data *d);
/*-------------------------------------------------------------------*/

// global
int global_circle = -1; // keep track of circle as we navigate menus
int global_class = -1;  // keep track of class as we navigate menus
int favored_slot = -1;

/*-------------------------------------------------------------------*/

/* list of possible animal companions, use in-game vnums for this */
#define C_BEAR 60
#define C_BOAR 61
#define C_LION 62
#define C_CROCODILE 63
#define C_HYENA 64
#define C_SNOW_LEOPARD 65
#define C_SKULL_SPIDER 66
#define C_FIRE_BEETLE 67
#define C_CAYHOUND 68
#define C_DRACAVES 69
/*--- paladin mount(s) -----*/
#define C_W_WARHORSE 70
#define C_B_DESTRIER 71
#define C_STALLION 72
#define C_A_DESTRIER 73
#define C_G_WARHORSE 74
#define C_P_WARHORSE 75
#define C_C_DESTRIER 76
#define C_WARDOG 77
#define C_WARPONY 78
#define C_GRIFFON 79
/*--- familiars -----*/
#define F_HUNTER 80
#define F_PANTHER 81
#define F_MOUSE 82
#define F_EAGLE 83
#define F_RAVEN 84
#define F_IMP 85
#define F_PIXIE 86
#define F_FAERIE_DRAGON 87
#define F_PSEUDO_DRAGON 88
#define F_HELLHOUND 89

/* make a list of vnums corresponding in order, first animals  */
int animal_vnums[] = {
    0,
    C_BEAR,         // 60, 1
    C_BOAR,         // 61, 2
    C_LION,         // 62, 3
    C_CROCODILE,    // 63, 4
    C_HYENA,        // 64, 5
    C_SNOW_LEOPARD, // 65, 6
    C_SKULL_SPIDER, // 66, 7
    C_FIRE_BEETLE,  // 67, 8
    C_CAYHOUND,     // 68, 9
    C_DRACAVES,     // 69, 10
    -1              /* end with this */
};
#define NUM_ANIMALS 10
/* now paladin mounts */
int mount_vnums[] = {
    0,
    C_W_WARHORSE, //70, 1
    C_B_DESTRIER, //71, 2
    C_STALLION,   //72, 3
    C_A_DESTRIER, //73, 4
    C_G_WARHORSE, //74, 5
    C_P_WARHORSE, //75, 6
    C_C_DESTRIER, //76, 7
    C_WARDOG,     //77, 8
    C_WARPONY,    //78, 9
    C_GRIFFON,    //79, 10
    -1            /* end with this */
};
#define NUM_MOUNTS 10
/* now familiars */
int familiar_vnums[] = {
    0,
    F_HUNTER,        //80, 1
    F_PANTHER,       // 81, 2
    F_MOUSE,         //82, 3
    F_EAGLE,         //83, 4
    F_RAVEN,         //84, 5
    F_IMP,           //85, 6
    F_PIXIE,         // 86, 7
    F_FAERIE_DRAGON, //87, 8
    F_PSEUDO_DRAGON, //88, 9
    F_HELLHOUND,     //89, 10
    -1               /* end with this */
};
#define NUM_FAMILIARS 10

/* DEBUG:  just checking first 8 animals right now -zusuk */
#define TOP_OF_C 8
/****************/

/* make a list of names in order, first animals */
const char *animal_names[] = {
    "Unknown",
    "1) Black Bear",
    "2) Boar",
    "3) Lion",
    "4) Crocodile",
    "5) Hyena",
    "6) Snow Leopard",
    "7) Skull Spider",
    "8) Fire Beetle",
    "\n" /* end with this */
};
/* ... now mounts */
const char *mount_names[] = {
    "Unknown",
    "1) Heavy White Warhorse",
    "2) Black Destrier",
    "3) Stallion",
    "4) Armored Destrier",
    "5) Golden Warhorse",
    "6) Painted Warhorse",
    "7) Bright Destrier",
    "8) Wardog",
    "9) Warpony",
    "\n" /* end with this */
};
/* ... now familiars */
const char *familiar_names[] = {
    "Unknown",
    "1) Night Hunter",
    "2) Black Panther",
    "3) Tiny Mouse",
    "4) Eagle",
    "5) Raven",
    "6) Imp",
    "7) Pixie",
    "8) Faerie Dragon",
    "\n" /* end with this */
};

/*-------------------------------------------------------------------*\
  utility functions
 \*-------------------------------------------------------------------*/

ACMDC(do_study)
{
  struct descriptor_data *d = NULL;
  int class = -1;

  skip_spaces(&argument);

  if (!argument)
  {
    send_to_char(ch, "Specify a class to edit known spells.\r\n");
    return;
  }
  else if (is_abbrev(argument, "sorcerer"))
  {
    if (IS_SORC_LEARNED(ch) && GET_LEVEL(ch) < LVL_IMMORT)
    {
      send_to_char(ch, "You can only modify your 'known' list once per level.\r\n"
                       "(You can also RESPEC to reset your character)\r\n");
      return;
    }
    if (!CLASS_LEVEL(ch, CLASS_SORCERER))
    {
      send_to_char(ch, "How?  You are not a sorcerer!\r\n");
      return;
    }
    class = CLASS_SORCERER;
  }
  else if (is_abbrev(argument, "bard"))
  {
    if (IS_BARD_LEARNED(ch) && GET_LEVEL(ch) < LVL_IMMORT)
    {
      send_to_char(ch, "You can only modify your 'known' list once per level.\r\n"
                       "(You can also RESPEC to reset your character)\r\n");
      return;
    }
    if (!CLASS_LEVEL(ch, CLASS_BARD))
    {
      send_to_char(ch, "How?  You are not a bard!\r\n");
      return;
    }
    class = CLASS_BARD;
  }
  else if (is_abbrev(argument, "druid"))
  {
    if (IS_DRUID_LEARNED(ch) && GET_LEVEL(ch) < LVL_IMMORT)
    {
      send_to_char(ch, "You can only modify your 'known' list once per level.\r\n"
                       "(You can also RESPEC to reset your character)\r\n");
      return;
    }
    if (!CLASS_LEVEL(ch, CLASS_DRUID))
    {
      send_to_char(ch, "How?  You are not a druid!\r\n");
      return;
    }
    class = CLASS_DRUID;
  }
  else if (is_abbrev(argument, "ranger"))
  {
    if (IS_RANG_LEARNED(ch) && GET_LEVEL(ch) < LVL_IMMORT)
    {
      send_to_char(ch, "You already adjusted your ranger "
                       "skills this level.\r\n");
      return;
    }
    if (!CLASS_LEVEL(ch, CLASS_RANGER))
    {
      send_to_char(ch, "How?  You are not a ranger!\r\n");
      return;
    }
    class = CLASS_RANGER;
  }
  else if (is_abbrev(argument, "wizard"))
  {
    if (IS_WIZ_LEARNED(ch) && GET_LEVEL(ch) < LVL_IMMORT)
    {
      send_to_char(ch, "You already adjusted your wizard "
                       "skills this level.\r\n");
      return;
    }
    if (!CLASS_LEVEL(ch, CLASS_WIZARD))
    {
      send_to_char(ch, "How?  You are not a wizard!\r\n");
      return;
    }
    class = CLASS_WIZARD;
  }
  else
  {
    send_to_char(ch, "Usage:  study <class name>\r\n");
    return;
  }

  if (class == -1)
  {
    send_to_char(ch, "Invalid class!\r\n");
    return;
  }

  d = ch->desc;

  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE,
           "SYSERR: do_study: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  STATE(d) = CON_STUDY;

  act("$n starts adjust studying $s skill-set.",
      TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  if (class == CLASS_SORCERER)
  {
    global_class = CLASS_SORCERER;
    sorc_disp_menu(d);
  }
  else if (class == CLASS_BARD)
  {
    global_class = CLASS_BARD;
    bard_disp_menu(d);
  }
  else if (class == CLASS_RANGER)
  {
    global_class = CLASS_RANGER;
    ranger_disp_menu(d);
  }
  else if (class == CLASS_DRUID)
  {
    global_class = CLASS_DRUID;
    druid_disp_menu(d);
  }
  else if (class == CLASS_WIZARD)
  {
    global_class = CLASS_WIZARD;
    wizard_disp_menu(d);
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
                  "\r\n"
                  "%s A%s) Familiar Selection\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "%sWhen you quit it finalizes all changes%s\r\n"
                  "%sYour 'known spells' can only be modified once per level%s\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn,
                  grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][0] - count_known_spells_by_circle(d->character, CLASS_SORCERER, 1),
                  grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][1] - count_known_spells_by_circle(d->character, CLASS_SORCERER, 2),
                  grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][2] - count_known_spells_by_circle(d->character, CLASS_SORCERER, 3),
                  grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][3] - count_known_spells_by_circle(d->character, CLASS_SORCERER, 4),
                  grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][4] - count_known_spells_by_circle(d->character, CLASS_SORCERER, 5),
                  grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][5] - count_known_spells_by_circle(d->character, CLASS_SORCERER, 6),
                  grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][6] - count_known_spells_by_circle(d->character, CLASS_SORCERER, 7),
                  grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][7] - count_known_spells_by_circle(d->character, CLASS_SORCERER, 8),
                  grn, nrm, yel, sorcererKnown[CLASS_LEVEL(d->character, CLASS_SORCERER)][8] - count_known_spells_by_circle(d->character, CLASS_SORCERER, 9),
                  grn, nrm,
                  grn, nrm,
                  mgn, nrm,
                  mgn, nrm);

  OLC_MODE(d) = SORC_MAIN_MENU;
}

/* the menu for each circle, sorcerer */

void sorc_study_menu(struct descriptor_data *d, int circle)
{
  int counter, columns = 0;

  global_circle = circle;

  get_char_colors(d->character);
  clear_screen(d);

  /* SPELL PREPARATION HOOK (spellCircle) */
  for (counter = 1; counter < NUM_SPELLS; counter++)
  {
    if (compute_spells_circle(CLASS_SORCERER, counter) == circle)
    {
      if (is_a_known_spell(d->character, CLASS_SORCERER, counter))
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
                      count_known_spells_by_circle(d->character, CLASS_SORCERER, circle));
  write_to_output(d, "%sEnter spell choice, to add or remove "
                     "(Q to exit to main menu) : ",
                  nrm);

  OLC_MODE(d) = STUDY_SPELLS;
}

/***********************end sorcerer******************************************/

/*-------------------------------------------------------------------*/

/*. Display main menu, Bard . */

static void bard_disp_menu(struct descriptor_data *d)
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
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "%sWhen you quit it finalizes all changes%s\r\n"
                  "%sYour 'known spells' can only be modified once per level%s\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn,
                  grn, nrm, yel, bardKnown[CLASS_LEVEL(d->character, CLASS_BARD)][0] - count_known_spells_by_circle(d->character, CLASS_BARD, 1),
                  grn, nrm, yel, bardKnown[CLASS_LEVEL(d->character, CLASS_BARD)][1] - count_known_spells_by_circle(d->character, CLASS_BARD, 2),
                  grn, nrm, yel, bardKnown[CLASS_LEVEL(d->character, CLASS_BARD)][2] - count_known_spells_by_circle(d->character, CLASS_BARD, 3),
                  grn, nrm, yel, bardKnown[CLASS_LEVEL(d->character, CLASS_BARD)][3] - count_known_spells_by_circle(d->character, CLASS_BARD, 4),
                  grn, nrm, yel, bardKnown[CLASS_LEVEL(d->character, CLASS_BARD)][4] - count_known_spells_by_circle(d->character, CLASS_BARD, 5),
                  grn, nrm, yel, bardKnown[CLASS_LEVEL(d->character, CLASS_BARD)][5] - count_known_spells_by_circle(d->character, CLASS_BARD, 6),
                  grn, nrm,
                  grn, nrm,
                  mgn, nrm);

  OLC_MODE(d) = BARD_MAIN_MENU;
}

/* the menu for each circle, sorcerer */

void bard_study_menu(struct descriptor_data *d, int circle)
{
  int counter, columns = 0;

  global_circle = circle;

  get_char_colors(d->character);
  clear_screen(d);

  /* SPELL PREPARATION HOOK (spellCircle) */
  for (counter = 1; counter < NUM_SPELLS; counter++)
  {
    if (compute_spells_circle(CLASS_BARD, counter) == circle)
    {
      if (is_a_known_spell(d->character, CLASS_BARD, counter))
        write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, mgn,
                        spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
      else
        write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                        spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(d, "\r\n");
  write_to_output(d, "%sNumber of slots availble:%s %d.\r\n", grn, nrm,
                  sorcererKnown[CLASS_LEVEL(d->character, CLASS_BARD)][circle - 1] -
                      count_known_spells_by_circle(d->character, CLASS_BARD, circle));
  write_to_output(d, "%sEnter spell choice, to add or remove "
                     "(Q to exit to main menu) : ",
                  nrm);

  OLC_MODE(d) = BARD_STUDY_SPELLS;
}

/***********************end bard******************************************/

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
                  mgn, nrm);

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
                  race_family_abbrevs[GET_FAVORED_ENEMY(d->character, favored)],
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
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 0)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 1)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 2)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 3)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 4)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 5)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 6)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 7)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 8)], nrm,
                  grn, nrm, race_family_abbrevs[GET_FAVORED_ENEMY(d->character, 9)], nrm,
                  /* empty line */
                  grn, nrm
                  /* empty line */
  );

  OLC_MODE(d) = FAVORED_ENEMY;
}

/* druid/ranger study sub-menu:  adjust animal companion */
static void animal_companion_menu(struct descriptor_data *d)
{
  int i = 1, found = 0;

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sDruid/Ranger Animal Companion Menu%s\r\n"
                  "\r\n",
                  mgn, nrm);

  for (i = 1; i <= TOP_OF_C; i++)
  {
    write_to_output(d, "%s\r\n", animal_names[i]);
  }

  write_to_output(d, "\r\n");
  /* find current animal */
  for (i = 1; i <= TOP_OF_C; i++)
  {
    if (GET_ANIMAL_COMPANION(d->character) == animal_vnums[i])
    {
      write_to_output(d, "Current Companion:  %s\r\n", animal_names[i]);
      found = 1;
      break;
    }
  }

  if (!found)
    write_to_output(d, "Currently No Companion Selected\r\n");

  write_to_output(d, "You can select 0 (Zero) to deselect the current "
                     "companion.\r\n");
  write_to_output(d, "\r\n"
                     "%s Q%s) Quit\r\n"
                     "\r\n"
                     "Enter Choice : ",
                  grn, nrm);

  OLC_MODE(d) = ANIMAL_COMPANION;
}

/*********************** end ranger ****************************************/

/***************************/
/* main menu for druid */

/***************************/
static void druid_disp_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sDruid Skill Menu\r\n"
                  "\r\n"
                  "\r\n"
                  "%s 1%s) Animal Companion Menu\r\n"
                  "\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "%sWhen you quit it finalizes all changes%s\r\n"
                  "%sYour druid skills can only be modified once per level%s\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn,
                  /* empty line */
                  /* empty line */
                  grn, nrm,
                  /* empty line */
                  /* empty line */
                  grn, nrm,
                  mgn, nrm,
                  mgn, nrm);

  OLC_MODE(d) = DRUID_MAIN_MENU;
}

/* animal companion is under ranger section */

/*********************** end druid ****************************************/

/*********************** wizard ******************************************/

/***************************/
/* main menu for wizard    */

/***************************/
static void wizard_disp_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sWizard Skill Menu\r\n"
                  "\r\n"
                  "\r\n"
                  "%s 1%s) Familiar Menu\r\n"
                  "\r\n"
                  "\r\n"
                  "%s Q%s) Quit\r\n"
                  "\r\n"
                  "%sWhen you quit it finalizes all changes%s\r\n"
                  "%sYour wizard skills can only be modified once per level%s\r\n"
                  "\r\n"
                  "Enter Choice : ",

                  mgn,
                  /* empty line */
                  /* empty line */
                  grn, nrm,
                  /* empty line */
                  /* empty line */
                  grn, nrm,
                  mgn, nrm,
                  mgn, nrm);

  OLC_MODE(d) = WIZ_MAIN_MENU;
}

/* wizard study sub-menu:  adjust familiar */
static void familiar_menu(struct descriptor_data *d)
{
  int i = 1, found = 0;

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "\r\n-- %sFamiliar Menu%s\r\n"
                  "\r\n",
                  mgn, nrm);

  for (i = 1; i <= TOP_OF_C; i++)
  {
    write_to_output(d, "%s\r\n", familiar_names[i]);
  }

  write_to_output(d, "\r\n");

  /* find current familiar */
  for (i = 1; i <= TOP_OF_C; i++)
  {
    if (GET_FAMILIAR(d->character) == familiar_vnums[i])
    {
      write_to_output(d, "Current Familiar:  %s\r\n", familiar_names[i]);
      found = 1;
      break;
    }
  }

  if (!found)
    write_to_output(d, "Currently No Familiar Selected\r\n");

  write_to_output(d, "You can select 0 (Zero) to deselect the current "
                     "familiar.\r\n");
  write_to_output(d, "\r\n"
                     "%s Q%s) Quit\r\n"
                     "\r\n"
                     "Enter Choice : ",
                  grn, nrm);

  OLC_MODE(d) = FAMILIAR_MENU;
}

/**************************************************************************
  The handler
 **************************************************************************/

void study_parse(struct descriptor_data *d, char *arg)
{
  int number = -1;
  int counter;

  switch (OLC_MODE(d))
  {

    /******* start sorcerer **********/

    /* familiar menu is shared with wizard and can
       * be found in 'wizard' section of choices  */

  case SORC_MAIN_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      write_to_output(d, "Your choices have been finalized!\r\n\r\n");
      if (global_class == CLASS_SORCERER)
        IS_SORC_LEARNED(d->character) = 1;
      save_char(d->character, 0);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'a': // familiar choices
    case 'A':
      familiar_menu(d);
      OLC_MODE(d) = FAMILIAR_MENU;
      break;
      /* here are our spell levels for 'spells known' */
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
    switch (*arg)
    {
    case 'q':
    case 'Q':
      sorc_disp_menu(d);
      return;

    default:
      number = atoi(arg);

      /* SPELL PREPARATION HOOK (spellCircle) */
      for (counter = 1; counter < NUM_SPELLS; counter++)
      {
        if (counter == number)
        {
          if (compute_spells_circle(CLASS_SORCERER, counter) == global_circle)
          {
            if (is_a_known_spell(d->character, CLASS_SORCERER, counter))
              known_spells_remove_by_class(d->character, CLASS_SORCERER, counter);
            else if (!known_spells_add(d->character, CLASS_SORCERER, counter, FALSE))
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

    /******* start bard **********/

  case BARD_MAIN_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      write_to_output(d, "Your choices have been finalized!\r\n\r\n");
      if (global_class == CLASS_BARD)
        IS_BARD_LEARNED(d->character) = 1;
      save_char(d->character, 0);
      cleanup_olc(d, CLEANUP_ALL);
      return;
      /* here are our spell levels for 'spells known' */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
      bard_study_menu(d, atoi(arg));
      OLC_MODE(d) = BARD_STUDY_SPELLS;
      break;
    default:
      write_to_output(d, "That is an invalid choice!\r\n");
      bard_disp_menu(d);
      break;
    }
    break;

  case BARD_STUDY_SPELLS:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      bard_disp_menu(d);
      return;

    default:
      number = atoi(arg);

      /* SPELL PREPARATION HOOK (spellCircle) */
      for (counter = 1; counter < NUM_SPELLS; counter++)
      {
        if (counter == number)
        {
          if (compute_spells_circle(CLASS_BARD, counter) == global_circle)
          {
            if (is_a_known_spell(d->character, CLASS_BARD, counter))
              known_spells_remove_by_class(d->character, CLASS_BARD, counter);
            else if (!known_spells_add(d->character, CLASS_BARD, counter, FALSE))
              write_to_output(d, "You are all FULL for spells!\r\n");
          }
        }
      }
      OLC_MODE(d) = BARD_MAIN_MENU;
      bard_study_menu(d, global_circle);
      break;
    }
    break;
    /******* end bard **********/

    /******* start druid **********/

  case DRUID_MAIN_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      write_to_output(d, "Your choices have been finalized!\r\n\r\n");
      if (global_class == CLASS_DRUID)
        IS_DRUID_LEARNED(d->character) = 1;
      save_char(d->character, 0);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1': // animal companion choice
      animal_companion_menu(d);
      OLC_MODE(d) = ANIMAL_COMPANION;
      break;
    default:
      write_to_output(d, "That is an invalid choice!\r\n");
      ranger_disp_menu(d);
      break;
    }
    break;

    /* animal companion is under ranger section */

    /******* end druid **********/

    /******* start ranger **********/

  case RANG_MAIN_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      write_to_output(d, "Your choices have been finalized!\r\n\r\n");
      if (global_class == CLASS_RANGER)
        IS_RANG_LEARNED(d->character) = 1;
      save_char(d->character, 0);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1': // favored enemy choice
      favored_enemy_menu(d);
      OLC_MODE(d) = FAVORED_ENEMY;
      break;
    case '2': // animal companion choice
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
    switch (*arg)
    {
    case 'q':
    case 'Q':
      ranger_disp_menu(d);
      OLC_MODE(d) = RANG_MAIN_MENU;
      return;

    default:
      number = atoi(arg);
      int ranger_level = CLASS_LEVEL(d->character, CLASS_RANGER);

      switch (number)
      {
      case 0:
        if (ranger_level)
        {
          favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        break;
      case 1:
        if (ranger_level >= 5)
        {
          favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             " modify this slot!\r\n");
        }
        break;
      case 2:
        if (ranger_level >= 10)
        {
          favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             "modify this slot!\r\n");
        }
        break;
      case 3:
        if (ranger_level >= 15)
        {
          favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             "modify this slot!\r\n");
        }
        break;
      case 4:
        if (ranger_level >= 20)
        {
          favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             "modify this slot!\r\n");
        }
        break;
      case 5:
        if (ranger_level >= 25)
        {
          favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
          write_to_output(d, "You are not a high enough level ranger to"
                             "modify this slot!\r\n");
        }
        break;
      case 6:
        if (ranger_level >= 30)
        {
          favored_slot = number;
          favored_enemy_submenu(d, number);
          OLC_MODE(d) = FAVORED_ENEMY_SUB;
          return;
        }
        else
        {
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
    switch (*arg)
    {
    case 'q':
    case 'Q':
      favored_enemy_menu(d);
      OLC_MODE(d) = FAVORED_ENEMY;
      return;

    default:
      number = atoi(arg);

      if (number < 0 || number >= NUM_RACE_TYPES)
        write_to_output(d, "Invalid race!\r\n");
      else
      {
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

    /* shared with druid */
  case ANIMAL_COMPANION:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      if (global_class == CLASS_RANGER)
      {
        ranger_disp_menu(d);
        OLC_MODE(d) = RANG_MAIN_MENU;
      }
      if (global_class == CLASS_DRUID)
      {
        druid_disp_menu(d);
        OLC_MODE(d) = DRUID_MAIN_MENU;
      }
      return;

    default:
      number = atoi(arg);

      if (number == 0)
      {
        GET_ANIMAL_COMPANION(d->character) = number;
        write_to_output(d, "Your companion has been set to OFF.\r\n");
      }
      else if (number < 0 || number >= NUM_ANIMALS)
      {
        write_to_output(d, "Not a valid choice!\r\n");
      }
      else
      {
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

    /******* wizard **********/

  case WIZ_MAIN_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      write_to_output(d, "Your choices have been finalized!\r\n\r\n");
      if (global_class == CLASS_WIZARD)
        IS_WIZ_LEARNED(d->character) = 1;
      save_char(d->character, 0);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1': // familiar choice
      familiar_menu(d);
      OLC_MODE(d) = FAMILIAR_MENU;
      break;
    default:
      write_to_output(d, "That is an invalid choice!\r\n");
      wizard_disp_menu(d);
      break;
    }
    break;

    /* shared with sorcerer */
  case FAMILIAR_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      if (global_class == CLASS_WIZARD)
      {
        wizard_disp_menu(d);
        OLC_MODE(d) = WIZ_MAIN_MENU;
      }
      if (global_class == CLASS_SORCERER)
      {
        sorc_disp_menu(d);
        OLC_MODE(d) = SORC_MAIN_MENU;
      }
      return;

    default:
      number = atoi(arg);

      if (!number)
      {
        GET_FAMILIAR(d->character) = number;
        write_to_output(d, "Your familiar has been set to OFF.\r\n");
      }
      else if (number < 0 || number >= NUM_FAMILIARS)
      {
        write_to_output(d, "Not a valid choice!\r\n");
      }
      else
      {
        GET_FAMILIAR(d->character) = familiar_vnums[number];
        write_to_output(d, "You have selected %s.\r\n",
                        familiar_names[number]);
      }

      OLC_MODE(d) = FAMILIAR_MENU;
      familiar_menu(d);
      break;
    }
    OLC_MODE(d) = FAMILIAR_MENU;
    break;

    /**** end wizard ******/

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

/* some undefines from top of file */
#undef C_BEAR
#undef C_BOAR
#undef C_LION
#undef C_CROCODILE
#undef C_HYENA
#undef C_SNOW_LEOPARD
#undef C_SKULL_SPIDER
#undef C_FIRE_BEETLE
#undef C_CAYHOUND
#undef C_DRACAVES
/*--- paladin mount(s) -----*/
#undef C_W_WARHORSE
#undef C_B_DESTRIER
#undef C_STALLION
#undef C_A_DESTRIER
#undef C_G_WARHORSE
#undef C_P_WARHORSE
#undef C_C_DESTRIER
#undef C_WARDOG
#undef C_WARPONY
#undef C_GRIFFON
/*--- familiars -----*/
#undef F_HUNTER
#undef F_PANTHER
#undef F_MOUSE
#undef F_EAGLE
#undef F_RAVEN
#undef F_IMP
#undef F_PIXIE
#undef F_FAERIE_DRAGON
#undef F_PSEUDO_DRAGON
#undef F_HELLHOUND

#endif
