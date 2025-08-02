/**************************************************************************
 *  File: medit.c                                      Part of LuminariMUD *
 *  Usage: Oasis OLC - Mobiles.                                            *
 *                                                                         *
 * Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.                   *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "spells.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genmob.h"
#include "genzon.h"
#include "genshp.h"
#include "oasis.h"
#include "handler.h"
#include "constants.h"
#include "improved-edit.h"
#include "dg_olc.h"
#include "screen.h"
#include "fight.h"
#include "race.h"
#include "class.h"
#include "feats.h"
#include "modify.h" /* for smash_tilde */

/* local functions */
static void init_mobile(struct char_data *mob);
static void medit_save_to_disk(zone_vnum zone_num);
static void medit_disp_positions(struct descriptor_data *d);
static void medit_disp_sex(struct descriptor_data *d);
static void medit_disp_race(struct descriptor_data *d);
static void medit_disp_subrace1(struct descriptor_data *d);
static void medit_disp_subrace2(struct descriptor_data *d);
static void medit_disp_subrace3(struct descriptor_data *d);
static void medit_disp_class(struct descriptor_data *d);
static void medit_disp_size(struct descriptor_data *d);
static void medit_disp_attack_types(struct descriptor_data *d);
static bool medit_illegal_mob_flag(int fl);
static int medit_get_mob_flag_by_number(int num);
static void medit_disp_mob_flags(struct descriptor_data *d);
static void medit_disp_aff_flags(struct descriptor_data *d);
static void medit_disp_menu(struct descriptor_data *d);

void medit_add_class_feats(struct descriptor_data *d)
{
  int cl = 0, lvl = 0;
  struct char_data *mob = OLC_MOB(d);
  struct class_feat_assign *feat_assign = NULL;

  if (!mob) return;

  if (class_list[cl].featassign_list == NULL)
    return;

  cl = GET_CLASS(mob);

  for (lvl = 1; lvl <= GET_LEVEL(mob); lvl++)
  {
    for (feat_assign = class_list[cl].featassign_list; feat_assign != NULL; feat_assign = feat_assign->next) 
    {
      if (feat_assign->level_received == lvl)
      {
        MOB_SET_FEAT(mob, feat_assign->feat_num, MOB_HAS_FEAT(mob, feat_assign->feat_num) + 1);
      }
    }
  }
}

void medit_clear_all_feats(struct descriptor_data *d)
{
  struct char_data *mob = OLC_MOB(d);
  int i = 0;

  if (!mob) return;

  for (i = 0; i < FEAT_LAST_FEAT; i++)
  {
    MOB_SET_FEAT(mob, i, 0);
  }

}

void medit_clear_all_spells(struct descriptor_data *d)
{
  struct char_data *mob = OLC_MOB(d);
  int i = 0;

  if (!mob) return;

  for (i = 1; i < NUM_SPELLS; i++)
  {
    MOB_KNOWS_SPELL(mob, i) = 0;
  }

}

bool does_mob_have_feats(struct char_data *mob)
{
  if (!mob) return false;
  if (!IS_NPC(mob)) return false;

  int i = 0;
  for (i = 0; i < FEAT_LAST_FEAT; i++)
    if (MOB_HAS_FEAT(mob, i)) return true;

  return false;
}

bool does_mob_have_spells(struct char_data *mob)
{
  if (!mob) return false;
  if (!IS_NPC(mob)) return false;

  int i = 0;
  for (i = 1; i < NUM_SPELLS; i++)
    if (MOB_KNOWS_SPELL(mob, i) > 0) return true;
  return false;
}


void medit_disp_add_feats(struct descriptor_data *d)
{
  int i = 0, count = 0;

  write_to_output(d, "\r\nKnown Feats\r\n");

  for (i = 0; i < FEAT_LAST_FEAT; i++)
  {
    if (MOB_HAS_FEAT(OLC_MOB(d), i))
    {
      if (count > 0)
        write_to_output(d, ", ");
      write_to_output(d, "%s", feat_list[i].name);
      if (feat_list[i].can_stack)
        write_to_output(d, "(x%d)", MOB_HAS_FEAT(OLC_MOB(d), i));
      count++;
    }
  }
  write_to_output(d, "\r\n\r\nType addclassfeats to add all class feats for any classes the mob has.\r\n");
  write_to_output(d, "Enter erase to delete all feats, or quit to exit this screen.\r\n");
  write_to_output(d, "\r\nPlease enter the name of the feat you wish to toggle: ");
}

void medit_disp_add_spells(struct descriptor_data *d)
{
  int i = 0, count = 0;

  write_to_output(d, "\r\nKnown Spells\r\n");

  for (i = 1; i < NUM_SPELLS; i++)
  {
    if (MOB_KNOWS_SPELL(OLC_MOB(d), i))
    {
      if (count > 0)
        write_to_output(d, ", ");
      write_to_output(d, "%s", spell_info[i].name);
      count++;
    }
  }
  write_to_output(d, "\r\n\r\nEnter erase to delete all spells, or quit to exit this screen.\r\n");
  write_to_output(d, "Please understand that these spells are in addition to any the mob might know from having spellcaster class levels.\r\n");
  write_to_output(d, "\r\nPlease enter the name of the spell you wish to toggle: ");
}



/*  utility functions */
ACMD(do_oasis_medit)
{
  int number = NOBODY, save = 0, real_num;
  struct descriptor_data *d;
  const char *buf3;
  char buf1[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  /* Parse any arguments */
  buf3 = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

  if (!*buf1)
  {
    send_to_char(ch, "Specify a mobile VNUM to edit.\r\n");
    return;
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

  /* If a numeric argument was given (like a room number), get it. */
  if (number == NOBODY)
    number = atoi(buf1);

  /* Check that whatever it is isn't already being edited. */
  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) == CON_MEDIT)
    {
      if (d->olc && OLC_NUM(d) == number)
      {
        send_to_char(ch, "That mobile is currently being edited by %s.\r\n",
                     GET_NAME(d->character));
        return;
      }
    }
  }

  d = ch->desc;

  /* Give descriptor an OLC structure. */
  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE,
           "SYSERR: do_oasis_medit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  /* Find the zone. */
  OLC_ZNUM(d) = save ? real_zone(number) : real_zone_by_thing(number);
  if (OLC_ZNUM(d) == NOWHERE)
  {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* Everyone but IMPLs can only edit zones they have been assigned. */
  if (!can_edit_zone(ch, OLC_ZNUM(d)))
  {
    send_cannot_edit(ch, zone_table[OLC_ZNUM(d)].number);
    /* Free the OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* If save is TRUE, save the mobiles. */
  if (save)
  {
    send_to_char(ch, "Saving all mobiles in zone %d.\r\n",
                 zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE,
           "OLC: %s saves mobile info for zone %d.",
           GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);

    /* Save the mobiles. */
    save_mobiles(OLC_ZNUM(d));

    /* Free the olc structure stored in the descriptor. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;

  /* If this is a new mobile, setup a new one, otherwise, setup the
     existing mobile. */
  if ((real_num = real_mobile(number)) == NOBODY)
    medit_setup_new(d);
  else
    medit_setup_existing(d, real_num, QMODE_NONE);

  medit_disp_menu(d);
  STATE(d) = CON_MEDIT;

  /* Display the OLC messages to the players in the same room as the
     builder and also log it. */
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing zone %d allowed zone %d",
         GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

static void medit_save_to_disk(zone_vnum foo)
{
  save_mobiles(real_zone(foo));
}

void medit_setup_new(struct descriptor_data *d)
{
  struct char_data *mob;

  /* Allocate a scratch mobile structure. */
  CREATE(mob, struct char_data, 1);

  init_mobile(mob);

  GET_MOB_RNUM(mob) = NOBODY;
  /* Set up some default strings. */
  GET_ALIAS(mob) = strdup("mob unfinished");
  GET_SDESC(mob) = strdup("the unfinished mob");
  GET_LDESC(mob) = strdup("An unfinished mob stands here.\r\n");
  GET_DDESC(mob) = strdup("It looks unfinished.\r\n");
  /* these don't need to be set up with defaults, because if we
   * don't assign a value, they will use the stock messages*/
  GET_WALKIN(mob) = strdup("$n enters from");
  GET_WALKOUT(mob) = strdup("$n leaves");

  SCRIPT(mob) = NULL;
  mob->proto_script = OLC_SCRIPT(d) = NULL;

  OLC_MOB(d) = mob;
  /* Has changed flag. (It hasn't so far, we just made it.) */
  OLC_VAL(d) = FALSE;
  OLC_ITEM_TYPE(d) = MOB_TRIGGER;
}

void medit_setup_existing(struct descriptor_data *d, int rmob_num, int mode)
{
  struct char_data *mob;

  /* Allocate a scratch mobile structure. */
  CREATE(mob, struct char_data, 1);

  copy_mobile(mob, mob_proto + rmob_num);

  OLC_MOB(d) = mob;
  OLC_ITEM_TYPE(d) = MOB_TRIGGER;
  dg_olc_script_copy(d);
  /*
   * The edited mob must not have a script.
   * It will be assigned to the updated mob later, after editing.
   */
  SCRIPT(mob) = NULL;
  OLC_MOB(d)->proto_script = NULL;
}

/* Ideally, this function should be in db.c, but I'll put it here for portability. */
static void init_mobile(struct char_data *mob)
{
  clear_char(mob);

  GET_HIT(mob) = GET_PSP(mob) = 1;
  GET_MAX_PSP(mob) = GET_MAX_MOVE(mob) = 100;
  GET_NDD(mob) = GET_SDD(mob) = 1;
  GET_WEIGHT(mob) = 200;
  GET_HEIGHT(mob) = 200;

  GET_REAL_STR(mob) = 10;
  GET_REAL_CON(mob) = 10;
  GET_REAL_DEX(mob) = 10;
  GET_REAL_INT(mob) = 10;
  GET_REAL_WIS(mob) = 10;
  GET_REAL_CHA(mob) = 10;
  mob->aff_abils = mob->real_abils;
  reset_char_points(mob);

  SET_BIT_AR(MOB_FLAGS(mob), MOB_ISNPC);
  mob->player_specials = &dummy_mob;
}

/* Save new/edited mob to memory. */
void medit_save_internally(struct descriptor_data *d)
{
  int i;
  mob_rnum new_rnum;
  struct descriptor_data *dsc;
  struct char_data *mob;

  i = (real_mobile(OLC_NUM(d)) == NOBODY);

  if ((new_rnum = add_mobile(OLC_MOB(d), OLC_NUM(d))) == NOBODY)
  {
    log("medit_save_internally: add_mobile failed.");
    return;
  }

  /* Update triggers and free old proto list */
  if (mob_proto[new_rnum].proto_script &&
      mob_proto[new_rnum].proto_script != OLC_SCRIPT(d))
    free_proto_script(&mob_proto[new_rnum], MOB_TRIGGER);

  mob_proto[new_rnum].proto_script = OLC_SCRIPT(d);

  /* this takes care of the mobs currently in-game */
  for (mob = character_list; mob; mob = mob->next)
  {
    if (GET_MOB_RNUM(mob) != new_rnum)
      continue;

    /* remove any old scripts */
    if (SCRIPT(mob))
      extract_script(mob, MOB_TRIGGER);

    free_proto_script(mob, MOB_TRIGGER);
    copy_proto_script(&mob_proto[new_rnum], mob, MOB_TRIGGER);
    assign_triggers(mob, MOB_TRIGGER);
  }
  /* end trigger update */

  if (!i) /* Only renumber on new mobiles. */
    return;

  /* Update keepers in shops being edited and other mobs being edited. */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
  {
    if (STATE(dsc) == CON_SEDIT)
      S_KEEPER(OLC_SHOP(dsc)) += (S_KEEPER(OLC_SHOP(dsc)) != NOTHING && S_KEEPER(OLC_SHOP(dsc)) >= new_rnum);
    else if (STATE(dsc) == CON_MEDIT)
      GET_MOB_RNUM(OLC_MOB(dsc)) += (GET_MOB_RNUM(OLC_MOB(dsc)) != NOTHING && GET_MOB_RNUM(OLC_MOB(dsc)) >= new_rnum);
    else if (STATE(dsc) == CON_HLQEDIT)
      GET_MOB_RNUM(OLC_MOB(dsc)) += (GET_MOB_RNUM(OLC_MOB(dsc)) != NOTHING && GET_MOB_RNUM(OLC_MOB(dsc)) >= new_rnum);
  }

  /* Update other people in zedit too. From: C.Raehl 4/27/99 */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_ZEDIT)
      for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
        if (OLC_ZONE(dsc)->cmd[i].command == 'M')
          if (OLC_ZONE(dsc)->cmd[i].arg1 >= new_rnum)
            OLC_ZONE(dsc)->cmd[i].arg1++;
}

/* Menu functions
   Display positions. (sitting, standing, etc) */
static void medit_disp_positions(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  column_list(d->character, 0, position_types, NUM_POSITIONS, TRUE);
  write_to_output(d, "Enter position number : ");
}

/* Display the gender of the mobile. */
static void medit_disp_sex(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  column_list(d->character, 0, genders, NUM_GENDERS, TRUE);
  write_to_output(d, "Enter gender number : ");
}

void medit_disp_race(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_RACE_TYPES; counter++)
  {
    write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                    race_family_types[counter], !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%s(You can choose 99 for random)", nrm);
  write_to_output(d, "\r\n%sEnter race number : ", nrm);
}

void medit_disp_subrace1(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_SUB_RACES; counter++)
  {
    write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                    npc_subrace_types[counter], !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%s(You can choose 99 for random)", nrm);
  write_to_output(d, "\r\n%sEnter subrace number : ", nrm);
}

void medit_disp_subrace2(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_SUB_RACES; counter++)
  {
    write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                    npc_subrace_types[counter], !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%s(You can choose 99 for random)", nrm);
  write_to_output(d, "\r\n%sEnter subrace number : ", nrm);
}

void medit_disp_subrace3(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_SUB_RACES; counter++)
  {
    write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                    npc_subrace_types[counter], !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%s(You can choose 99 for random)", nrm);
  write_to_output(d, "\r\n%sEnter subrace number : ", nrm);
}

void medit_disp_class(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_CLASSES; counter++)
  {
    write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                    CLSLIST_NAME(counter), !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%s(You can choose 99 for random)", nrm);
  write_to_output(d, "\r\n%s(Set the classless MOBFLAG to turn off the class)", nrm);
  write_to_output(d, "\r\n%sEnter class number : ", nrm);
}

void medit_disp_size(struct descriptor_data *d)
{
  int i, columns = 0;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  clear_screen(d);
  for (i = -1; i < NUM_SIZES; i++)
  {
    snprintf(buf, sizeof(buf), "%2d) %-20.20s  %s", i,
             (i == SIZE_UNDEFINED) ? "DEFAULT" : size_names[i],
             !(++columns % 2) ? "\r\n" : "");
    write_to_output(d, "%s", buf);
  }
  write_to_output(d, "\r\nEnter size number (-1 for default): ");
}

/* Display attack types menu. */
static void medit_disp_attack_types(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; i < NUM_ATTACK_TYPES; i++)
  {
    write_to_output(d, "%s%2d%s) %s\r\n", grn, i, nrm, attack_hit_text[i].singular);
  }
  write_to_output(d, "Enter attack type : ");
}

/* Find mob flags that shouldn't be set by builders */
static bool medit_illegal_mob_flag(int fl)
{
  int i;

  /* add any other flags you dont want them setting */
  const int illegal_flags[] = {
      MOB_ISNPC,
      MOB_NOTDEADYET,
  };

  const int num_illegal_flags = sizeof(illegal_flags) / sizeof(int);

  for (i = 0; i < num_illegal_flags; i++)
    if (fl == illegal_flags[i])
      return (TRUE);

  return (FALSE);
}

/* Due to illegal mob flags not showing in the mob flags list,
   we need this to convert the list number back to flag value */
static int medit_get_mob_flag_by_number(int num)
{
  int i, count = 0;
  for (i = 0; i < NUM_MOB_FLAGS; i++)
  {
    if (medit_illegal_mob_flag(i))
      continue;
    if ((++count) == num)
      return i;
  }
  /* Return 'illegal flag' value */
  return -1;
}

/* Display mob-flags menu. */
static void medit_disp_mob_flags(struct descriptor_data *d)
{
  int i, count = 0, columns = 0;
  char flags[MAX_STRING_LENGTH] = {'\0'};

  get_char_colors(d->character);
  clear_screen(d);

  /* Mob flags has special handling to remove illegal flags from the list */
  for (i = 0; i < NUM_MOB_FLAGS; i++)
  {
    if (medit_illegal_mob_flag(i))
      continue;
    write_to_output(d, "%s%2d%s) %-20.20s  %s", grn, ++count, nrm, action_bits[i],
                    !(++columns % 2) ? "\r\n" : "");
  }

  sprintbitarray(MOB_FLAGS(OLC_MOB(d)), action_bits, AF_ARRAY_MAX, flags);
  write_to_output(d, "\r\nCurrent flags : %s%s%s\r\nEnter mob flags (0 to quit) : ", cyn, flags, nrm);
}

/* Display affection flags menu. */
static void medit_disp_aff_flags(struct descriptor_data *d)
{
  char flags[MAX_STRING_LENGTH] = {'\0'};

  get_char_colors(d->character);
  clear_screen(d);
  /* +1 since AFF_FLAGS don't start at 0. */
  column_list(d->character, 0, affected_bits + 1, NUM_AFF_FLAGS - 1, TRUE);
  sprintbitarray(AFF_FLAGS(OLC_MOB(d)), affected_bits, AF_ARRAY_MAX, flags);
  write_to_output(d, "\r\nCurrent flags   : %s%s%s\r\nEnter aff flags (0 to quit) : ",
                  cyn, flags, nrm);
}

// needs to be fixed/finished

void delete_echo_entry(struct char_data *mob, int entry_num)
{
  int i = 0;

  // delete the entry, then decrement the entries that follow
  for (i = entry_num - 1; i < ECHO_COUNT(mob) - 1; i++)
  {
    if (ECHO_ENTRIES(mob)[i] != NULL)
      free(ECHO_ENTRIES(mob)[i]);
    ECHO_ENTRIES(mob)
    [i] = strdup(ECHO_ENTRIES(mob)[i + 1]);
  }
  // free(ECHO_ENTRIES(mob)[ECHO_COUNT(mob) - 1]);
  // ECHO_ENTRIES(mob)[ECHO_COUNT(mob) - 1] = NULL;
  ECHO_COUNT(mob)
  --;

  // if (ECHO_COUNT(mob) == 0) {
  // free(ECHO_ENTRIES(mob));
  // ECHO_ENTRIES(mob) = NULL;
  // }
}

/* Display alignment choices */
static void disp_align_menu(struct descriptor_data *d)
{
  int i = 0;

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d, "\r\n");
  for (; i < NUM_ALIGNMENTS; i++)
  {
    write_to_output(d, "%d) %s\r\n", i, alignment_names[i]);
  }
  write_to_output(d, "\r\n");
}

/* Display main menu. */
static void medit_disp_menu(struct descriptor_data *d)
{
  struct char_data *mob = NULL;
  int i = 0;
  char flags[MAX_STRING_LENGTH] = {'\0'},
       flag2[MAX_STRING_LENGTH] = {'\0'},
       path[MAX_STRING_LENGTH] = {'\0'},
       buf[MAX_STRING_LENGTH] = {'\0'};

  mob = OLC_MOB(d);
  get_char_colors(d->character);
  clear_screen(d);

  if (PATH_SIZE(mob) == 0)
  {
    strlcpy(path, "No Path Defined", sizeof(path));
  }
  else
  {
    snprintf(path, sizeof(path), "Delay %d Path - ", PATH_RESET(mob));
    for (i = 0; i < PATH_SIZE(mob); i++)
    {
      snprintf(buf, sizeof(buf), "%d ", GET_PATH(mob, i));
      strlcat(path, buf, sizeof(path));
    }
  }

  write_to_output(d,
                  "-- Mob Number:  [%s%d%s]\r\n"
                  "%s1%s) Sex: %s%-7.7s%s	         %s2%s) Keywords: %s%s\r\n"
                  "%s3%s) S-Desc: %s%s\r\n"
                  "%s4%s) L-Desc:-\r\n%s%s\r\n"
                  "%s5%s) D-Desc:-\r\n%s%s\r\n",

                  cyn, OLC_NUM(d), nrm,
                  grn, nrm, yel, genders[(int)GET_SEX(mob)], nrm,
                  grn, nrm, yel, GET_ALIAS(mob),
                  grn, nrm, yel, GET_SDESC(mob),
                  grn, nrm, yel, GET_LDESC(mob),
                  grn, nrm, yel, GET_DDESC(mob));

  sprintbitarray(MOB_FLAGS(mob), action_bits, AF_ARRAY_MAX, flags);
  sprintbitarray(AFF_FLAGS(mob), affected_bits, AF_ARRAY_MAX, flag2);

  write_to_output(d,
                  "%s6%s) Position  : %s%s\r\n"
                  "%s7%s) Default   : %s%s\r\n"
                  "%s8%s) Attack    : %s%s\r\n"
                  "%s9%s) Stats Menu...\r\n"
                  "%sG%s) Resists   ...\r\n"
                  "%sR%s) Race      : %s%s\r\n"
                  "%sD%s) SubRace   : %s%s\r\n"
                  "%sE%s) SubRace   : %s%s\r\n"
                  "%sF%s) SubRace   : %s%s\r\n"
                  "%sC%s) Class     : %s%s\r\n"
                  "%sH%s) Feats     : %s%s\r\n"
                  "%sN%s) Spells    : %s%s\r\n"
                  "%sI%s) Size      : %s%s\r\n"
                  "%sJ%s) Walk-In   : %s%s\r\n"
                  "%sK%s) Walk-Out  : %s%s\r\n"
                  "%sL%s) Echo Menu...\r\n"
                  "%sM%s) Set Plot Mob Flags & Settings (Shopkeepers, Questmasters, Etc.)\r\n"
                  "%sO%s) Set Random Descriptions (Shopkeepers, Questmasters, Etc.)\r\n"
                  //          "%s-%s) Echo Menu : IS ZONE: %d FREQ: %d%% COUNT: %d Echo: %s\r\n"
                  "%sA%s) NPC Flags : %s%s\r\n"
                  "%sB%s) AFF Flags : %s%s\r\n"
                  "%sS%s) Script    : %s%s\r\n"
                  "%sV%s) Path Edit : %s%s%s\r\n"
                  "%sW%s) Copy mob\r\n"
                  "%sX%s) Delete mob\r\n"
                  "%sQ%s) Quit\r\n"
                  "Enter choice : ",

                  grn, nrm, yel, position_types[(int)GET_POS(mob)],
                  grn, nrm, yel, position_types[(int)GET_DEFAULT_POS(mob)],
                  grn, nrm, yel, attack_hit_text[(int)GET_ATTACK(mob)].singular,
                  grn, nrm,
                  grn, nrm,
                  grn, nrm, yel, race_family_types[GET_RACE(mob)],
                  grn, nrm, yel, npc_subrace_types[GET_SUBRACE(mob, 0)],
                  grn, nrm, yel, npc_subrace_types[GET_SUBRACE(mob, 1)],
                  grn, nrm, yel, npc_subrace_types[GET_SUBRACE(mob, 2)],
                  grn, nrm, yel, CLSLIST_NAME(GET_CLASS(mob)),
                  grn, nrm, yel, does_mob_have_feats(mob) ? "Set" : "None",
                  grn, nrm, yel, does_mob_have_spells(mob) ? "Set" : "None",
                  grn, nrm, yel, size_names[GET_SIZE(mob)],
                  grn, nrm, yel, GET_WALKIN(mob) ? GET_WALKIN(mob) : "Default.",
                  grn, nrm, yel, GET_WALKOUT(mob) ? GET_WALKOUT(mob) : "Default.",
                  grn, nrm,
                  grn, nrm,
                  grn, nrm,
                  //         grn, nrm, ECHO_IS_ZONE(mob), ECHO_FREQ(mob), ECHO_AMOUNT(mob),
                  //         (ECHO_ENTRIES(mob)[0] ? ECHO_ENTRIES(mob)[0] : "None."),
                  grn, nrm, cyn, flags,
                  grn, nrm, cyn, flag2,
                  grn, nrm, cyn, OLC_SCRIPT(d) ? "Set." : "Not Set.",
                  grn, nrm, cyn, path, nrm,
                  grn, nrm,
                  grn, nrm,
                  grn, nrm);
  OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/* mobile echoes, dispaly */
static void medit_disp_echo_menu(struct descriptor_data *d)
{
  struct char_data *mob;
  int i = 0;

  mob = OLC_MOB(d);
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d, "Mobile Echos:\r\n");
  if (ECHO_COUNT(mob) > 0 && ECHO_ENTRIES(mob))
  {
    // write_to_output(d, "debug: echo count: %d\r\n", ECHO_COUNT(mob));
    for (i = 0; i < ECHO_COUNT(mob); i++)
      if (ECHO_ENTRIES(mob)[i])
        write_to_output(d, "%d) %s\r\n", (i + 1), ECHO_ENTRIES(mob)[i]);
    write_to_output(d, "\r\n");
  }
  else
  {
    write_to_output(d, "-=- NONE -=-\r\n\r\n");
  }

  write_to_output(d, "%sA%s) Add Echo\r\n"
                     "%sD%s) Delete Echo\r\n"
                     "%sE%s) Edit Echo\r\n"
                     "%sF%s) Echo Frequency: %d%%\r\n"
                     "%sT%s) Echo Type: [%s%s%s]\r\n"
                     "%sZ%s) Zone Echo: [%s%s%s]\r\n\r\n"
                     "%sQ%s) Quit to main menu\r\n"
                     "Enter choice : ",
                  grn, nrm,
                  grn, nrm, grn, nrm, grn, nrm, ECHO_FREQ(mob),
                  grn, nrm, cyn, ECHO_SEQUENTIAL(mob) ? "SEQUENTIAL" : "RANDOM", nrm,
                  grn, nrm, cyn, ECHO_IS_ZONE(mob) ? "YES" : "NO", nrm,
                  grn, nrm);

  OLC_MODE(d) = MEDIT_ECHO_MENU;
}

/* Display resistances menu. */
static void medit_disp_resistances_menu(struct descriptor_data *d)
{
  struct char_data *mob;

  mob = OLC_MOB(d);
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d,
                  "-- RESISTANCES -- Mob Number:  %s[%s%d%s]%s\r\n"
                  "(%sA%s) Fire:     %s[%s%4d%s]%s   (%sK%s) Bludgeon: %s[%s%4d%s]%s\r\n"
                  "(%sB%s) Cold:     %s[%s%4d%s]%s   (%sL%s) Sound:    %s[%s%4d%s]%s\r\n"
                  "(%sC%s) Air:      %s[%s%4d%s]%s   (%sM%s) Poison:   %s[%s%4d%s]%s\r\n"
                  "(%sD%s) Earth:    %s[%s%4d%s]%s   (%sN%s) Disease:  %s[%s%4d%s]%s\r\n"
                  "(%sE%s) Acid:     %s[%s%4d%s]%s   (%sO%s) Negative: %s[%s%4d%s]%s\r\n"
                  "(%sF%s) Holy:     %s[%s%4d%s]%s   (%sP%s) Illusion: %s[%s%4d%s]%s\r\n"
                  "(%sG%s) Electric: %s[%s%4d%s]%s   (%sR%s) Mental:   %s[%s%4d%s]%s\r\n"
                  "(%sH%s) Unholy:   %s[%s%4d%s]%s   (%sS%s) Light:    %s[%s%4d%s]%s\r\n"
                  "(%sI%s) Slash:    %s[%s%4d%s]%s   (%sT%s) Energy:   %s[%s%4d%s]%s\r\n"
                  "(%sJ%s) Piercing: %s[%s%4d%s]%s   (%sU%s) Water:    %s[%s%4d%s]%s\r\n\r\n",
                  cyn, yel, OLC_NUM(d), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_RESISTANCES(mob, 1), cyn, nrm, cyn, nrm,
                  cyn, yel, GET_RESISTANCES(mob, 11), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_RESISTANCES(mob, 2), cyn, nrm, cyn, nrm,
                  cyn, yel, GET_RESISTANCES(mob, 12), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_RESISTANCES(mob, 3), cyn, nrm, cyn, nrm,
                  cyn, yel, GET_RESISTANCES(mob, 13), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_RESISTANCES(mob, 4), cyn, nrm, cyn, nrm,
                  cyn, yel, GET_RESISTANCES(mob, 14), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_RESISTANCES(mob, 5), cyn, nrm, cyn, nrm,
                  cyn, yel, GET_RESISTANCES(mob, 15), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_RESISTANCES(mob, 6), cyn, nrm, cyn, nrm,
                  cyn, yel, GET_RESISTANCES(mob, 16), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_RESISTANCES(mob, 7), cyn, nrm, cyn, nrm,
                  cyn, yel, GET_RESISTANCES(mob, 17), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_RESISTANCES(mob, 8), cyn, nrm, cyn, nrm,
                  cyn, yel, GET_RESISTANCES(mob, 18), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_RESISTANCES(mob, 9), cyn, nrm, cyn, nrm,
                  cyn, yel, GET_RESISTANCES(mob, 19), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_RESISTANCES(mob, 10), cyn, nrm, cyn, nrm,
                  cyn, yel, GET_RESISTANCES(mob, 20), cyn, nrm);

  /* Quit to previous menu option */
  write_to_output(d, "(%sQ%s) Quit to main menu\r\nEnter choice : ", cyn, nrm);

  OLC_MODE(d) = MEDIT_RESISTANCES_MENU;
}

/* Display main stats menu. */
static void medit_disp_stats_menu(struct descriptor_data *d)
{
  struct char_data *mob;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  mob = OLC_MOB(d);
  get_char_colors(d->character);
  clear_screen(d);

  /* Color codes have to be used here, for count_color_codes to work */
  snprintf(buf, sizeof(buf), "(range \ty%d\tn to \ty%d\tn)", GET_HIT(mob) + GET_MOVE(mob), (GET_HIT(mob) * GET_PSP(mob)) + GET_MOVE(mob));

  /* Top section - standard stats */
  write_to_output(d,
                  "-- Mob Number:  %s[%s%d%s]%s\r\n"
                  "(%s1%s) Level:       %s[%s%4d%s]%s\r\n"
                  "(%s2%s) %sAuto Set Stats (*set level/race/class first)%s\r\n\r\n"
                  "Hit Points  (xdy+z):        Bare Hand Damage (xdy+z): \r\n"
                  "(%s3%s) HP NumDice:  %s[%s%5d%s]%s    (%s6%s) BHD NumDice:  %s[%s%5d%s]%s\r\n"
                  "(%s4%s) HP SizeDice: %s[%s%5d%s]%s    (%s7%s) BHD SizeDice: %s[%s%5d%s]%s\r\n"
                  "(%s5%s) HP Addition: %s[%s%5d%s]%s    (%s8%s) DamRoll:      %s[%s%5d%s]%s\r\n"
                  "%-*s(range %s%d%s to %s%d%s)\r\n\r\n"

                  "(%sA%s) Armor Class: %s[%s%4d (%2d) %s]%s   (%sD%s) Hitroll:   %s[%s%5d%s]%s\r\n"
                  "(%sB%s) Exp Points:  %s[%s%10d%s]%s   (%sE%s) Alignment: %s[%s%s%s]%s\r\n"
                  "(%sC%s) Gold:        %s[%s%10d%s]%s   (%sR%s) Damage Reduction: %s[%s%d%s]%s\r\n\r\n",
                  cyn, yel, OLC_NUM(d), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_LEVEL(mob), cyn, nrm,
                  cyn, nrm, cyn, nrm,
                  cyn, nrm, cyn, yel, GET_HIT(mob), cyn, nrm, cyn, nrm, cyn, yel, GET_NDD(mob), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_PSP(mob), cyn, nrm, cyn, nrm, cyn, yel, GET_SDD(mob), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_MOVE(mob), cyn, nrm, cyn, nrm, cyn, yel, GET_DAMROLL(mob), cyn, nrm,

                  count_color_chars(buf) + 28, buf,
                  yel, GET_NDD(mob) + GET_DAMROLL(mob), nrm,
                  yel, (GET_NDD(mob) * GET_SDD(mob)) + GET_DAMROLL(mob), nrm,

                  cyn, nrm, cyn, yel, GET_AC(mob), compute_armor_class(NULL, mob, FALSE, MODE_ARMOR_CLASS_NORMAL), cyn, nrm, cyn, nrm, cyn, yel, GET_HITROLL(mob), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_EXP(mob), cyn, nrm, cyn, nrm, cyn, yel, get_align_by_num(GET_ALIGNMENT(mob)), cyn, nrm,
                  cyn, nrm, cyn, yel, GET_GOLD(mob), cyn, nrm, cyn, nrm, cyn, yel, GET_DR_MOD(mob), cyn, nrm);

  if (CONFIG_MEDIT_ADVANCED)
  {
    /* Bottom section - non-standard stats, togglable in cedit */
    write_to_output(d,
                    "(%sF%s) Str: %s[%s%2d/%3d%s]%s   Saving Throws\r\n"
                    "(%sG%s) Int: %s[%s%3d%s]%s      (%sL%s) Paralysis     %s[%s%3d%s]%s\r\n"
                    "(%sH%s) Wis: %s[%s%3d%s]%s      (%sM%s) Rods/Staves   %s[%s%3d%s]%s\r\n"
                    "(%sI%s) Dex: %s[%s%3d%s]%s      (%sN%s) Petrification %s[%s%3d%s]%s\r\n"
                    "(%sJ%s) Con: %s[%s%3d%s]%s      (%sO%s) Breath        %s[%s%3d%s]%s\r\n"
                    "(%sK%s) Cha: %s[%s%3d%s]%s      (%sP%s) Spells        %s[%s%3d%s]%s\r\n\r\n",
                    cyn, nrm, cyn, yel, GET_STR(mob), GET_ADD(mob), cyn, nrm,
                    cyn, nrm, cyn, yel, GET_INT(mob), cyn, nrm, cyn, nrm, cyn, yel, GET_SAVE(mob, SAVING_FORT), cyn, nrm,
                    cyn, nrm, cyn, yel, GET_WIS(mob), cyn, nrm, cyn, nrm, cyn, yel, GET_SAVE(mob, SAVING_REFL), cyn, nrm,
                    cyn, nrm, cyn, yel, GET_DEX(mob), cyn, nrm, cyn, nrm, cyn, yel, GET_SAVE(mob, SAVING_WILL), cyn, nrm,
                    cyn, nrm, cyn, yel, GET_CON(mob), cyn, nrm, cyn, nrm, cyn, yel, GET_SAVE(mob, SAVING_POISON), cyn, nrm,
                    cyn, nrm, cyn, yel, GET_CHA(mob), cyn, nrm, cyn, nrm, cyn, yel, GET_SAVE(mob, SAVING_DEATH), cyn, nrm);
  }

  /* Quit to previous menu option */
  write_to_output(d, "(%sQ%s) Quit to main menu\r\nEnter choice : ", cyn, nrm);

  OLC_MODE(d) = MEDIT_STATS_MENU;
}

void medit_parse(struct descriptor_data *d, char *arg)
{
  int i = -1, j, k = 0;
  char *oldtext = NULL;
  char t_buf[200];

  if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE)
  {
    i = atoi(arg);
    if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && !isdigit(arg[1]))))
    {
      write_to_output(d, "Try again : ");
      return;
    }
  }
  else
  { /* String response. */
    if (!genolc_checkstring(d, arg))
      return;
  }
  switch (OLC_MODE(d))
  {
  case MEDIT_CONFIRM_SAVESTRING:
    /* Ensure mob has MOB_ISNPC set. */
    SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
    switch (*arg)
    {
    case 'y':
    case 'Y':
      /* Save the mob in memory and to disk. */
      medit_save_internally(d);
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s edits mob %d", GET_NAME(d->character), OLC_NUM(d));
      if (CONFIG_OLC_SAVE)
      {
        medit_save_to_disk(zone_table[real_zone_by_thing(OLC_NUM(d))].number);
        write_to_output(d, "Mobile saved to disk.\r\n");
      }
      else
        write_to_output(d, "Mobile saved to memory.\r\n");
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'n':
    case 'N':
      /* If not saving, we must free the script_proto list. We do so by
       * assigning it to the edited mob and letting free_mobile in
       * cleanup_olc handle it. */
      OLC_MOB(d)->proto_script = OLC_SCRIPT(d);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Do you wish to save your changes? : ");
      return;
    }
    break;

  case MEDIT_MAIN_MENU:
    i = 0;
    switch (*arg)
    {
    case 'q':
    case 'Q':
      if (OLC_VAL(d))
      { /* Anything been changed? */
        write_to_output(d, "Do you wish to save your changes? : ");
        OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
      }
      else
        cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      OLC_MODE(d) = MEDIT_SEX;
      medit_disp_sex(d);
      return;
    case '2':
      OLC_MODE(d) = MEDIT_KEYWORD;
      i--;
      break;
    case '3':
      OLC_MODE(d) = MEDIT_S_DESC;
      i--;
      break;
    case '4':
      OLC_MODE(d) = MEDIT_L_DESC;
      i--;
      break;
    case '5':
      OLC_MODE(d) = MEDIT_D_DESC;
      send_editor_help(d);
      write_to_output(d, "Enter mob description:\r\n\r\n");
      if (OLC_MOB(d)->player.description)
      {
        write_to_output(d, "%s", OLC_MOB(d)->player.description);
        oldtext = strdup(OLC_MOB(d)->player.description);
      }
      string_write(d, &OLC_MOB(d)->player.description, MAX_MOB_DESC, 0, oldtext);
      OLC_VAL(d) = 1;
      return;
    case '6':
      OLC_MODE(d) = MEDIT_POS;
      medit_disp_positions(d);
      return;
    case '7':
      OLC_MODE(d) = MEDIT_DEFAULT_POS;
      medit_disp_positions(d);
      return;
    case '8':
      OLC_MODE(d) = MEDIT_ATTACK;
      medit_disp_attack_types(d);
      return;
    case '9':
      OLC_MODE(d) = MEDIT_STATS_MENU;
      medit_disp_stats_menu(d);
      return;
    case 'g':
    case 'G':
      OLC_MODE(d) = MEDIT_RESISTANCES_MENU;
      medit_disp_resistances_menu(d);
      return;
    case 'r':
    case 'R':
      OLC_MODE(d) = MEDIT_RACE;
      medit_disp_race(d);
      return;
    case 'd':
    case 'D':
      OLC_MODE(d) = MEDIT_SUB_RACE_1;
      medit_disp_subrace1(d);
      return;
    case 'e':
    case 'E':
      OLC_MODE(d) = MEDIT_SUB_RACE_2;
      medit_disp_subrace2(d);
      return;
    case 'f':
    case 'F':
      OLC_MODE(d) = MEDIT_SUB_RACE_3;
      medit_disp_subrace3(d);
      return;
    case 'c':
    case 'C':
      OLC_MODE(d) = MEDIT_CLASS;
      medit_disp_class(d);
      return;
    case 'h':
    case 'H':
      medit_disp_add_feats(d);
      OLC_MODE(d) = MEDIT_ADD_FEATS;
      i--;
      return;
    case 'n':
    case 'N':
      medit_disp_add_spells(d);
      OLC_MODE(d) = MEDIT_ADD_SPELLS;
      i--;
      return;
    case 'i':
    case 'I':
      OLC_MODE(d) = MEDIT_SIZE;
      medit_disp_size(d);
      return;
    case 'j':
    case 'J': // walk-in
      write_to_output(d, "Please enter a new walkin with proper format:\r\n"
                         "Example:  $n stomps in from\r\n"
                         "Result:   The monster stomps in from the north.\r\n"
                         "<leave blank for default>\r\n: ");
      OLC_MODE(d) = MEDIT_WALKIN;
      i--;
      return;
    case 'k':
    case 'K': // walk-out
      write_to_output(d, "Please enter a new walkout with proper format:\r\n"
                         "Example:  $n stomps\r\n"
                         "Result:   The monster stomps north.\r\n"
                         "<leave blank for default>\r\n: ");
      OLC_MODE(d) = MEDIT_WALKOUT;
      i--;
      return;
    case 'l':
    case 'L':
      OLC_MODE(d) = MEDIT_ECHO_MENU;
      medit_disp_echo_menu(d);
      return;
    case 'a':
    case 'A':
      OLC_MODE(d) = MEDIT_NPC_FLAGS;
      medit_disp_mob_flags(d);
      return;
    case 'b':
    case 'B':
      OLC_MODE(d) = MEDIT_AFF_FLAGS;
      medit_disp_aff_flags(d);
      return;
    case 'w':
    case 'W':
      write_to_output(d, "Copy what mob? ");
      OLC_MODE(d) = MEDIT_COPY;
      return;
    case 'x':
    case 'X':
      write_to_output(d, "Are you sure you want to delete this mobile? ");
      OLC_MODE(d) = MEDIT_DELETE;
      return;
    case 'm':
    case 'M':
      // We're setting this mob up with random descs/name and the right mob flags
      GET_REAL_RACE(OLC_MOB(d)) = RACE_TYPE_HUMANOID;
      GET_CLASS(OLC_MOB(d)) = CLASS_WARRIOR;
      GET_LEVEL(OLC_MOB(d)) = 10;
      medit_autoroll_stats(d);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_SENTIENT);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_SENTINEL);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_AWARE);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NOCHARM);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NOSUMMON);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NOSLEEP);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NOBASH);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NOBLIND);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NOKILL);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NODEAF);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NOFIGHT);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NOGRAPPLE);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NOSTEAL);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NO_AI);
      SET_BIT_AR(MOB_FLAGS(OLC_MOB(d)), MOB_NOCONFUSE);
      GET_REAL_SIZE(OLC_MOB(d)) = SIZE_MEDIUM;
      (OLC_MOB(d))->points.size = GET_REAL_SIZE(OLC_MOB(d));
      OLC_VAL(d) = TRUE;
      medit_disp_menu(d);
      write_to_output(d, "\r\nThe mob has been set with appropriate flags and settings for a plot mob.\r\n\r\n");
      return;
    case 'o':
    case 'O':
      GET_SEX(OLC_MOB(d)) = dice(1, 2);
      snprintf(t_buf, sizeof(t_buf), "%s %s", GET_SEX(OLC_MOB(d)) == SEX_MALE ? random_male_names[dice(1, NUM_MALE_NAMES) - 1] : random_female_names[dice(1, NUM_FEMALE_NAMES) - 1], random_surnames[dice(1, NUM_SURNAMES) - 1]);
      OLC_MOB(d)->player.name = strdup(t_buf);
      OLC_MOB(d)->player.short_descr = strdup(t_buf);
      snprintf(t_buf, sizeof(t_buf), "%s is here before you.", OLC_MOB(d)->player.short_descr);
      OLC_MOB(d)->player.long_descr = strdup(t_buf);
      snprintf(t_buf, sizeof(t_buf), "%s is a %s %s.\n", OLC_MOB(d)->player.short_descr, genders[GET_SEX(OLC_MOB(d))], race_list[dice(1, NUM_RACES) - 1].name);
      OLC_MOB(d)->player.description = strdup(t_buf);
      OLC_VAL(d) = TRUE;
      medit_disp_menu(d);
      write_to_output(d, "\r\nThe mob has been set with a random gender/name/description.\r\n\r\n");
      return;
    case 'v':
    case 'V':
      OLC_MODE(d) = MEDIT_PATH_DELAY;
      i++;
      write_to_output(d, "Enter Path Delay Count > ");
      return;
    case 's':
    case 'S':
      OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
      dg_script_menu(d);
      return;
    default:
      medit_disp_menu(d);
      return;
    }
    if (i == 0)
      break;
    else if (i == 1)
      write_to_output(d, "\r\nEnter new value : ");
    else if (i == -1)
      write_to_output(d, "\r\nEnter new text :\r\n] ");
    else
      write_to_output(d, "Oops...\r\n");
    return;

  case MEDIT_ADD_ECHO:
    smash_tilde(arg);
    if (ECHO_COUNT(OLC_MOB(d)) >= 20)
    { // arbitrary maximum echos
      write_to_output(d, "This mobile has the maximum amount of echos allowed.\r\n");
      OLC_MODE(d) = MEDIT_ECHO_MENU;
      medit_disp_echo_menu(d);
      return;
    }
    if (arg && *arg)
    {
      char buf[MAX_INPUT_LENGTH] = {'\0'};
      if (!ECHO_ENTRIES(OLC_MOB(d)))
        CREATE(ECHO_ENTRIES(OLC_MOB(d)), char *, 1);
      else
        RECREATE(ECHO_ENTRIES(OLC_MOB(d)), char *, ECHO_COUNT(OLC_MOB(d)) + 1);
      snprintf(buf, sizeof(buf), "%s", delete_doubledollar(arg));
      ECHO_ENTRIES(OLC_MOB(d))
      [ECHO_COUNT(OLC_MOB(d))++] = strdup(buf);
      OLC_VAL(d) = TRUE;
    }

    OLC_MODE(d) = MEDIT_ECHO_MENU;
    medit_disp_echo_menu(d);
    return;

  case MEDIT_DELETE_ECHO:
    if ((j = atoi(arg)) <= 0 || j > ECHO_COUNT(OLC_MOB(d)))
    {
      OLC_MODE(d) = MEDIT_ECHO_MENU;
      medit_disp_echo_menu(d);
      return;
    }

    OLC_VAL(d) = TRUE;
    delete_echo_entry(OLC_MOB(d), j);
    OLC_MODE(d) = MEDIT_ECHO_MENU;
    medit_disp_echo_menu(d);
    return;

  case MEDIT_EDIT_ECHO:
    if ((j = atoi(arg)) <= 0 || j > ECHO_COUNT(OLC_MOB(d)))
    {
      OLC_MODE(d) = MEDIT_ECHO_MENU;
      medit_disp_echo_menu(d);
      return;
    }

    OLC_VAL(d) = j;
    OLC_MODE(d) = MEDIT_EDIT_ECHO_TEXT;
    write_to_output(d, "\r\nEnter new text :\r\n] ");
    return;

  case MEDIT_EDIT_ECHO_TEXT:
    smash_tilde(arg);

    if (arg && *arg)
    {
      char buf[MAX_INPUT_LENGTH] = {'\0'};
      snprintf(buf, sizeof(buf), "%s", delete_doubledollar(arg));
      if (ECHO_ENTRIES(OLC_MOB(d)) && ECHO_ENTRIES(OLC_MOB(d)) != NULL &&
          ECHO_ENTRIES(OLC_MOB(d))[OLC_VAL(d) - 1])
        free(ECHO_ENTRIES(OLC_MOB(d))[OLC_VAL(d) - 1]);
      ECHO_ENTRIES(OLC_MOB(d))
      [OLC_VAL(d) - 1] = strdup(buf);
    }
    else
      delete_echo_entry(OLC_MOB(d), OLC_VAL(d));

    OLC_VAL(d) = TRUE;
    medit_disp_echo_menu(d);
    return;

  case MEDIT_ECHO_FREQUENCY:
    ECHO_FREQ(OLC_MOB(d)) = LIMIT(i, 0, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_echo_menu(d);
    return;

  case MEDIT_ECHO_MENU:
    i = 0;
    switch (*arg)
    {
    case 'a':
    case 'A':
      OLC_MODE(d) = MEDIT_ADD_ECHO;
      i--;
      break;
    case 'd':
    case 'D':
      if (ECHO_COUNT(OLC_MOB(d)) <= 0)
      {
        write_to_output(d, "\r\nNo echos to delete.  Enter choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_DELETE_ECHO;
      write_to_output(d, "Delete which echo? [1-%d] : ", ECHO_COUNT(OLC_MOB(d)));
      return;
    case 'e':
    case 'E':
      if (ECHO_COUNT(OLC_MOB(d)) <= 0)
      {
        write_to_output(d, "\r\nNo echos to edit.  Enter choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_EDIT_ECHO;
      write_to_output(d, "Edit which echo? [1-%d] : ", ECHO_COUNT(OLC_MOB(d)));
      return;
    case 'f':
    case 'F':
      OLC_MODE(d) = MEDIT_ECHO_FREQUENCY;
      i++;
      break;
    case 'q':
    case 'Q':
      medit_disp_menu(d);
      return;
    case 't':
    case 'T':
      ECHO_SEQUENTIAL(OLC_MOB(d)) = !ECHO_SEQUENTIAL(OLC_MOB(d));
      OLC_VAL(d) = TRUE;
      medit_disp_echo_menu(d);
      return;
    case 'z':
    case 'Z':
      if (GET_LEVEL(d->character) >= LVL_STAFF)
      {
        ECHO_IS_ZONE(OLC_MOB(d)) = !ECHO_IS_ZONE(OLC_MOB(d));
        OLC_VAL(d) = TRUE;
        medit_disp_echo_menu(d);
      }
      else
      {
        write_to_output(d, "You need someone more privileged than you to enable zone echo.\r\nEnter choice : ");
      }
      return;
    default:
      medit_disp_echo_menu(d);
      return;
    }
    if (i == 0)
      break;
    else if (i == 1)
      write_to_output(d, "\r\nEnter new value : ");
    else if (i == -1)
      write_to_output(d, "\r\nEnter new text :\r\n] ");
    else
      write_to_output(d, "Oops...\r\n");
    return;

  case MEDIT_STATS_MENU:
    i = 0;
    switch (*arg)
    {
    case 'q':
    case 'Q':
      medit_disp_menu(d);
      return;
    case '1': /* Edit level */
      OLC_MODE(d) = MEDIT_LEVEL;
      i++;
      break;
    case '2': /* Autoroll stats */
      medit_autoroll_stats(d);
      medit_disp_stats_menu(d);
      OLC_VAL(d) = TRUE;
      return;
    case '3':
      OLC_MODE(d) = MEDIT_NUM_HP_DICE;
      i++;
      break;
    case '4':
      OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
      i++;
      break;
    case '5':
      OLC_MODE(d) = MEDIT_ADD_HP;
      i++;
      break;
    case '6':
      OLC_MODE(d) = MEDIT_NDD;
      i++;
      break;
    case '7':
      OLC_MODE(d) = MEDIT_SDD;
      i++;
      break;
    case '8':
      OLC_MODE(d) = MEDIT_DAMROLL;
      i++;
      break;
    case 'a':
    case 'A':
      OLC_MODE(d) = MEDIT_AC;
      i++;
      break;
    case 'b':
    case 'B':
      OLC_MODE(d) = MEDIT_EXP;
      i++;
      break;
    case 'c':
    case 'C':
      OLC_MODE(d) = MEDIT_GOLD;
      i++;
      break;
    case 'd':
    case 'D':
      OLC_MODE(d) = MEDIT_HITROLL;
      i++;
      break;
    case 'e':
    case 'E':
      OLC_MODE(d) = MEDIT_ALIGNMENT;
      disp_align_menu(d);
      i++;
      break;
    case 'r':
    case 'R':
      OLC_MODE(d) = MEDIT_DR;
      i++;
      break;
    case 'f':
    case 'F':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_STR;
      i++;
      break;
    case 'g':
    case 'G':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_INT;
      i++;
      break;
    case 'h':
    case 'H':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_WIS;
      i++;
      break;
    case 'i':
    case 'I':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_DEX;
      i++;
      break;
    case 'j':
    case 'J':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_CON;
      i++;
      break;
    case 'k':
    case 'K':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_CHA;
      i++;
      break;
    case 'l':
    case 'L':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_PARA;
      i++;
      break;
    case 'm':
    case 'M':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_ROD;
      i++;
      break;
    case 'n':
    case 'N':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_PETRI;
      i++;
      break;
    case 'o':
    case 'O':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_BREATH;
      i++;
      break;
    case 'p':
    case 'P':
      if (!CONFIG_MEDIT_ADVANCED)
      {
        write_to_output(d, "Invalid Choice!\r\nEnter Choice : ");
        return;
      }
      OLC_MODE(d) = MEDIT_SPELL;
      i++;
      break;
    default:
      medit_disp_stats_menu(d);
      return;
    }
    if (i == 0)
      break;
    else if (i == 1)
      write_to_output(d, "\r\nEnter new value : ");
    else if (i == -1)
      write_to_output(d, "\r\nEnter new text :\r\n] ");
    else
      write_to_output(d, "Oops...\r\n");
    return;

  case MEDIT_RESISTANCES_MENU:
    i = 0;
    switch (*arg)
    {
    case 'q':
    case 'Q':
      medit_disp_menu(d);
      return;
    case 'a':
    case 'A':
      OLC_MODE(d) = MEDIT_DAM_FIRE;
      i++;
      break;
    case 'b':
    case 'B':
      OLC_MODE(d) = MEDIT_DAM_COLD;
      i++;
      break;
    case 'c':
    case 'C':
      OLC_MODE(d) = MEDIT_DAM_AIR;
      i++;
      break;
    case 'd':
    case 'D':
      OLC_MODE(d) = MEDIT_DAM_EARTH;
      i++;
      break;
    case 'e':
    case 'E':
      OLC_MODE(d) = MEDIT_DAM_ACID;
      i++;
      break;
    case 'f':
    case 'F':
      OLC_MODE(d) = MEDIT_DAM_HOLY;
      i++;
      break;
    case 'g':
    case 'G':
      OLC_MODE(d) = MEDIT_DAM_ELECTRIC;
      i++;
      break;
    case 'h':
    case 'H':
      OLC_MODE(d) = MEDIT_DAM_UNHOLY;
      i++;
      break;
    case 'i':
    case 'I':
      OLC_MODE(d) = MEDIT_DAM_SLICE;
      i++;
      break;
    case 'j':
    case 'J':
      OLC_MODE(d) = MEDIT_DAM_PUNCTURE;
      i++;
      break;
    case 'k':
    case 'K':
      OLC_MODE(d) = MEDIT_DAM_FORCE;
      i++;
      break;
    case 'l':
    case 'L':
      OLC_MODE(d) = MEDIT_DAM_SOUND;
      i++;
      break;
    case 'm':
    case 'M':
      OLC_MODE(d) = MEDIT_DAM_POISON;
      i++;
      break;
    case 'n':
    case 'N':
      OLC_MODE(d) = MEDIT_DAM_DISEASE;
      i++;
      break;
    case 'o':
    case 'O':
      OLC_MODE(d) = MEDIT_DAM_NEGATIVE;
      i++;
      break;
    case 'p':
    case 'P':
      OLC_MODE(d) = MEDIT_DAM_ILLUSION;
      i++;
      break;
    case 'r':
    case 'R':
      OLC_MODE(d) = MEDIT_DAM_MENTAL;
      i++;
      break;
    case 's':
    case 'S':
      OLC_MODE(d) = MEDIT_DAM_LIGHT;
      i++;
      break;
    case 't':
    case 'T':
      OLC_MODE(d) = MEDIT_DAM_ENERGY;
      i++;
      break;
    case 'u':
    case 'U':
      OLC_MODE(d) = MEDIT_DAM_WATER;
      i++;
      break;
    default:
      medit_disp_resistances_menu(d);
      return;
    }
    if (i == 0)
      break;
    else if (i == 1)
      write_to_output(d, "\r\nEnter new value : ");
    else
      write_to_output(d, "Oops...\r\n");
    return;

  case OLC_SCRIPT_EDIT:
    if (dg_script_edit_parse(d, arg))
      return;
    break;

  case MEDIT_KEYWORD:
    smash_tilde(arg);
    if (GET_ALIAS(OLC_MOB(d)))
      free(GET_ALIAS(OLC_MOB(d)));
    GET_ALIAS(OLC_MOB(d)) = str_udup(arg);
    break;

  case MEDIT_S_DESC:
    smash_tilde(arg);
    if (GET_SDESC(OLC_MOB(d)))
      free(GET_SDESC(OLC_MOB(d)));
    GET_SDESC(OLC_MOB(d)) = str_udup(arg);
    break;

  case MEDIT_L_DESC:
    smash_tilde(arg);
    if (GET_LDESC(OLC_MOB(d)))
      free(GET_LDESC(OLC_MOB(d)));
    if (arg && *arg)
    {
      char buf[MAX_INPUT_LENGTH] = {'\0'};
      snprintf(buf, sizeof(buf), "%s\r\n", arg);
      GET_LDESC(OLC_MOB(d)) = strdup(buf);
    }
    else
      GET_LDESC(OLC_MOB(d)) = strdup("undefined");

    break;

  case MEDIT_D_DESC:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: medit_parse(): Reached D_DESC case!");
    write_to_output(d, "Oops...\r\n");
    break;

  case MEDIT_WALKIN:
    smash_tilde(arg);
    if (GET_WALKIN(OLC_MOB(d)))
      free(GET_WALKIN(OLC_MOB(d)));
    if (arg && *arg)
    {
      char buf[MAX_INPUT_LENGTH] = {'\0'};
      snprintf(buf, sizeof(buf), "%s", delete_doubledollar(arg));
      GET_WALKIN(OLC_MOB(d)) = strdup(buf);
    }
    else
      GET_WALKIN(OLC_MOB(d)) = NULL;
    break;

  case MEDIT_WALKOUT:
    smash_tilde(arg);
    if (GET_WALKOUT(OLC_MOB(d)))
      free(GET_WALKOUT(OLC_MOB(d)));
    if (arg && *arg)
    {
      char buf[MAX_INPUT_LENGTH] = {'\0'};
      snprintf(buf, sizeof(buf), "%s", delete_doubledollar(arg));
      GET_WALKOUT(OLC_MOB(d)) = strdup(buf);
    }
    else
      GET_WALKOUT(OLC_MOB(d)) = NULL;
    break;

  case MEDIT_NPC_FLAGS:
    if ((i = atoi(arg)) <= 0)
      break;
    else if ((j = medit_get_mob_flag_by_number(i)) == -1)
    {
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Enter mob flags (0 to quit) :");
      return;
    }
    else if (j <= NUM_MOB_FLAGS)
    {
      TOGGLE_BIT_AR(MOB_FLAGS(OLC_MOB(d)), (j));
    }
    medit_disp_mob_flags(d);
    return;

  case MEDIT_AFF_FLAGS:
    if ((i = atoi(arg)) <= 0)
      break;
    else if (i <= NUM_AFF_FLAGS)
      TOGGLE_BIT_AR(AFF_FLAGS(OLC_MOB(d)), i);

    /* Remove unwanted bits right away. */
    REMOVE_BIT_AR(AFF_FLAGS(OLC_MOB(d)), AFF_CHARM);
    REMOVE_BIT_AR(AFF_FLAGS(OLC_MOB(d)), AFF_POISON);
    REMOVE_BIT_AR(AFF_FLAGS(OLC_MOB(d)), AFF_ACID_COAT);
    REMOVE_BIT_AR(AFF_FLAGS(OLC_MOB(d)), AFF_SLEEP);
    medit_disp_aff_flags(d);
    return;

    /* Numerical responses. */

  case MEDIT_SEX:
    GET_SEX(OLC_MOB(d)) = LIMIT(i - 1, 0, NUM_GENDERS - 1);
    break;

  case MEDIT_HITROLL:
    GET_HITROLL(OLC_MOB(d)) = LIMIT(i, 0, 50);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_DAMROLL:
    GET_DAMROLL(OLC_MOB(d)) = LIMIT(i, 0, 50);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_NDD:
    GET_NDD(OLC_MOB(d)) = LIMIT(i, 0, 30);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_SDD:
    GET_SDD(OLC_MOB(d)) = LIMIT(i, 0, 127);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_NUM_HP_DICE:
    GET_HIT(OLC_MOB(d)) = LIMIT(i, 0, 30);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_SIZE_HP_DICE:
    GET_PSP(OLC_MOB(d)) = LIMIT(i, 0, 1000);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_ADD_HP:
    GET_MOVE(OLC_MOB(d)) = LIMIT(i, 0, 30000);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_AC:
    (OLC_MOB(d))->points.armor = LIMIT(i, 0, 600);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_EXP:
    GET_EXP(OLC_MOB(d)) = LIMIT(i, 0, MAX_MOB_EXP);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_GOLD:
    GET_GOLD(OLC_MOB(d)) = LIMIT(i, 0, MAX_MOB_GOLD);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_STR:
    (OLC_MOB(d))->aff_abils.str = LIMIT(i, 3, 50);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_INT:
    GET_INT(OLC_MOB(d)) = LIMIT(i, 3, 50);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_WIS:
    GET_WIS(OLC_MOB(d)) = LIMIT(i, 3, 50);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_DEX:
    (OLC_MOB(d))->aff_abils.dex = LIMIT(i, 3, 50);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_CON:
    (OLC_MOB(d))->aff_abils.con = LIMIT(i, 3, 50);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_CHA:
    GET_CHA(OLC_MOB(d)) = LIMIT(i, 3, 50);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

    case MEDIT_DR:
    GET_DR_MOD(OLC_MOB(d)) = LIMIT(i, 0, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_PARA:
    GET_SAVE(OLC_MOB(d), SAVING_FORT) = LIMIT(i, 0, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_ROD:
    GET_SAVE(OLC_MOB(d), SAVING_REFL) = LIMIT(i, 0, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_PETRI:
    GET_SAVE(OLC_MOB(d), SAVING_WILL) = LIMIT(i, 0, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_BREATH:
    GET_SAVE(OLC_MOB(d), SAVING_POISON) = LIMIT(i, 0, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_SPELL:
    GET_SAVE(OLC_MOB(d), SAVING_DEATH) = LIMIT(i, 0, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_DAM_FIRE:
    GET_RESISTANCES(OLC_MOB(d), DAM_FIRE) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_COLD:
    GET_RESISTANCES(OLC_MOB(d), DAM_COLD) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_AIR:
    GET_RESISTANCES(OLC_MOB(d), DAM_AIR) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_EARTH:
    GET_RESISTANCES(OLC_MOB(d), DAM_EARTH) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_ACID:
    GET_RESISTANCES(OLC_MOB(d), DAM_ACID) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_HOLY:
    GET_RESISTANCES(OLC_MOB(d), DAM_HOLY) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_ELECTRIC:
    GET_RESISTANCES(OLC_MOB(d), DAM_ELECTRIC) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_UNHOLY:
    GET_RESISTANCES(OLC_MOB(d), DAM_UNHOLY) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_SLICE:
    GET_RESISTANCES(OLC_MOB(d), DAM_SLICE) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_PUNCTURE:
    GET_RESISTANCES(OLC_MOB(d), DAM_PUNCTURE) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_FORCE:
    GET_RESISTANCES(OLC_MOB(d), DAM_FORCE) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_SOUND:
    GET_RESISTANCES(OLC_MOB(d), DAM_SOUND) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_POISON:
    GET_RESISTANCES(OLC_MOB(d), DAM_POISON) = LIMIT(i, -100, 100);
    GET_RESISTANCES(OLC_MOB(d), DAM_CELESTIAL_POISON) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_DISEASE:
    GET_RESISTANCES(OLC_MOB(d), DAM_DISEASE) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_NEGATIVE:
    GET_RESISTANCES(OLC_MOB(d), DAM_NEGATIVE) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_ILLUSION:
    GET_RESISTANCES(OLC_MOB(d), DAM_ILLUSION) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_MENTAL:
    GET_RESISTANCES(OLC_MOB(d), DAM_MENTAL) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_LIGHT:
    GET_RESISTANCES(OLC_MOB(d), DAM_LIGHT) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_ENERGY:
    GET_RESISTANCES(OLC_MOB(d), DAM_ENERGY) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;
  case MEDIT_DAM_WATER:
    GET_RESISTANCES(OLC_MOB(d), DAM_WATER) = LIMIT(i, -100, 100);
    OLC_VAL(d) = TRUE;
    medit_disp_resistances_menu(d);
    return;

  case MEDIT_POS:
    /* the menu starts with value 1, which is 1 greater than defines */
    i--;
    if (i == POS_FIGHTING)
      i = POS_STANDING;
    GET_POS(OLC_MOB(d)) = LIMIT(i, 0, NUM_POSITIONS);
    break;

  case MEDIT_DEFAULT_POS:
    /* the menu starts with value 1, which is 1 greater than defines */
    i--;
    if (i == POS_FIGHTING)
      i = POS_STANDING;
    GET_DEFAULT_POS(OLC_MOB(d)) = LIMIT(i, 0, NUM_POSITIONS);
    break;

  case MEDIT_ATTACK:
    GET_ATTACK(OLC_MOB(d)) = LIMIT(i, 0, NUM_ATTACK_TYPES - 1);
    break;

  case MEDIT_LEVEL:
    GET_LEVEL(OLC_MOB(d)) = LIMIT(i, 1, LVL_IMPL);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

    /*
            "0) Lawful Good\r\n"
            "1) Neutral Good\r\n"
            "2) Chaotic Good\r\n"
            "3) Lawful Neutral\r\n"
            "4) True Neutral\r\n"
            "5) Chaotic Neutral\r\n"
            "6) Lawful Evil\r\n"
            "7) Neutral Evil\r\n"
            "8) Chaotic Evil\r\n\r\n"*/
  case MEDIT_ALIGNMENT:
    if (i < 0 || i > (NUM_ALIGNMENTS - 1))
    {
      write_to_output(d, "\r\nInvalid choice!\r\n");
      return;
    }
    set_alignment(OLC_MOB(d), i);
    OLC_VAL(d) = TRUE;
    medit_disp_stats_menu(d);
    return;

  case MEDIT_RACE:
    if (i == 99)
      GET_REAL_RACE(OLC_MOB(d)) = rand_number(1, NUM_RACE_TYPES - 1);
    else
      GET_REAL_RACE(OLC_MOB(d)) = LIMIT(i, 0, NUM_RACE_TYPES - 1);
    break;

  case MEDIT_SUB_RACE_1:
    if (i == 99)
      GET_SUBRACE(OLC_MOB(d), 0) = rand_number(1, NUM_SUB_RACES - 1);
    else
      GET_SUBRACE(OLC_MOB(d), 0) = LIMIT(i, 0, NUM_SUB_RACES - 1);
    break;

  case MEDIT_SUB_RACE_2:
    if (i == 99)
      GET_SUBRACE(OLC_MOB(d), 1) = rand_number(1, NUM_SUB_RACES - 1);
    else
      GET_SUBRACE(OLC_MOB(d), 1) = LIMIT(i, 0, NUM_SUB_RACES - 1);
    break;

  case MEDIT_SUB_RACE_3:
    if (i == 99)
      GET_SUBRACE(OLC_MOB(d), 2) = rand_number(1, NUM_SUB_RACES - 1);
    else
      GET_SUBRACE(OLC_MOB(d), 2) = LIMIT(i, 0, NUM_SUB_RACES - 1);
    break;

  case MEDIT_CLASS:
    if (i == 99)
      GET_CLASS(OLC_MOB(d)) = rand_number(0, NUM_CLASSES - 1);
    else
      GET_CLASS(OLC_MOB(d)) = LIMIT(i, 0, NUM_CLASSES - 1);
    break;

  
  case MEDIT_ADD_FEATS:
    
    if (arg && *arg)
    {
      if (is_abbrev(arg, "quit") || is_abbrev(arg, "QUIT"))
      {
        break;
      }

      if (is_abbrev(arg, "addclassfeats"))
      {
        medit_add_class_feats(d);
        medit_disp_add_feats(d);
        return;
      }

      if (is_abbrev(arg, "erase"))
      {
        medit_clear_all_feats(d);
        medit_disp_add_feats(d);
        return;
      }
      
      for (k = 0; k < FEAT_LAST_FEAT; k++)
      {
        if (is_abbrev(arg, feat_list[k].name))
        {
          
          if (feat_list[k].can_stack && feat_list[k].in_game)
          {
            
            write_to_output(d, "How many ranks do you want to add? (0 to remove feat): ");
            OLC_MOB(d)->mob_specials.temp_feat = k;
            OLC_MODE(d) = MEDIT_SET_FEAT_RANKS;
            return;
          }
          else
          {
            
            if (MOB_HAS_FEAT(OLC_MOB(d), k))
              MOB_SET_FEAT(OLC_MOB(d), k, 0);
            else
              MOB_SET_FEAT(OLC_MOB(d), k, 1);
          }
          medit_disp_add_feats(d);
          return;
        }
      }
    }
    else
    {
      write_to_output(d, "Please enter the name of the feat you would like to toggle.\r\n");
      medit_disp_add_feats(d);
      return;
    }
    write_to_output(d, "That is not a valid feat.");
    medit_disp_add_feats(d);
    return;

    case MEDIT_ADD_SPELLS:
    
    if (arg && *arg)
    {
      if (is_abbrev(arg, "quit") || is_abbrev(arg, "QUIT"))
      {
        break;
      }

      if (is_abbrev(arg, "erase"))
      {
        medit_clear_all_spells(d);
        medit_disp_add_spells(d);
        return;
      }
      
      for (k = 1; k < NUM_SPELLS; k++)
      {
        if (is_abbrev(arg, spell_info[k].name))
        {
          if (MOB_KNOWS_SPELL(OLC_MOB(d), k))
              MOB_KNOWS_SPELL(OLC_MOB(d), k) = 0;
            else
              MOB_KNOWS_SPELL(OLC_MOB(d), k) = 1;
          medit_disp_add_spells(d);
          return;
        }
      }
    }
    else
    {
      write_to_output(d, "Please enter the name of the spell you would like to toggle.\r\n");
      medit_disp_add_spells(d);
      return;
    }
    write_to_output(d, "That is not a valid spell.");
    medit_disp_add_spells(d);
    return;

  case MEDIT_SET_FEAT_RANKS:
    if (OLC_MOB(d)->mob_specials.temp_feat < 0 || 
        OLC_MOB(d)->mob_specials.temp_feat >= FEAT_LAST_FEAT)
    {
      write_to_output(d, "The feat selected is invalid.\r\n");
      break;
    }
    MOB_SET_FEAT(OLC_MOB(d), OLC_MOB(d)->mob_specials.temp_feat, i);
    medit_disp_add_feats(d);
    OLC_MODE(d) = MEDIT_ADD_FEATS;
    return;

  case MEDIT_SIZE:
    GET_REAL_SIZE(OLC_MOB(d)) = LIMIT(i, 0, NUM_SIZES - 1);
    (OLC_MOB(d))->points.size = GET_REAL_SIZE(OLC_MOB(d));
    break;

  case MEDIT_PATH_DELAY:
    PATH_SIZE(OLC_MOB(d)) = 0;
    PATH_RESET(OLC_MOB(d)) = atoi(arg);
    PATH_DELAY(OLC_MOB(d)) = PATH_RESET(OLC_MOB(d));
    write_to_output(d, "Begin path...\r\n");
    write_to_output(d, "Enter value for path (room vnum to move mobile)\r\n");
    OLC_MODE(d) = MEDIT_PATH_EDIT;
    return; /* this will jump immediately to path edit below */
    break;

  case MEDIT_PATH_EDIT:
    write_to_output(d, "Enter next value for path (terminate with 0)\r\n");
    if (atoi(arg) && PATH_SIZE(OLC_MOB(d)) < MAX_PATH - 1)
    {
      GET_PATH(OLC_MOB(d), PATH_SIZE(OLC_MOB(d))++) = atoi(arg);
      write_to_output(d, "Value received!  Continuing...\r\n");
      return;
    }
    break;

  case MEDIT_COPY:
    if ((i = real_mobile(atoi(arg))) != NOWHERE)
    {
      medit_setup_existing(d, i, QMODE_QCOPY);
    }
    else
      write_to_output(d, "That mob does not exist.\r\n");
    break;

  case MEDIT_DELETE:
    if (*arg == 'y' || *arg == 'Y')
    {
      if (delete_mobile(GET_MOB_RNUM(OLC_MOB(d))) != NOBODY)
        write_to_output(d, "Mobile deleted.\r\n");
      else
        write_to_output(d, "Couldn't delete the mobile!\r\n");

      cleanup_olc(d, CLEANUP_ALL);
      return;
    }
    else if (*arg == 'n' || *arg == 'N')
    {
      medit_disp_menu(d);
      OLC_MODE(d) = MEDIT_MAIN_MENU;
      return;
    }
    else
      write_to_output(d, "Please answer 'Y' or 'N': ");
    break;

  default:
    /* We should never get here. */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: medit_parse(): Reached default case!");
    write_to_output(d, "Oops...\r\n");
    break;
  }

  /* END OF CASE If we get here, we have probably changed something, and now want
     to return to main menu.  Use OLC_VAL as a 'has changed' flag */

  OLC_VAL(d) = TRUE;
  medit_disp_menu(d);
}

void medit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d))
  {

  case MEDIT_D_DESC:
  default:
    medit_disp_menu(d);
    break;
  }
}

/* function to set a ch (mob) to correct stats */
/* an important note about mobiles besides these values:
   1)  their attack rotation will match their class/level
   2)  their BAB will match their class/level
   3)  their saving-throws will match their class/level
 */
void autoroll_mob(struct char_data *mob, bool realmode, bool summoned)
{
  int level = 0, bonus = 0;
  int armor_class = 100; /* base 10 AC */

  /* this variable is to avoid confusion:  GET_MOVE() is actually hps for
   !realmode in this context */
  int mobs_hps = GET_MOVE(mob);

  if (realmode)
    mobs_hps = GET_MAX_HIT(mob);

  /* first cap level at LVL_IMPL */
  level = GET_LEVEL(mob);
  level = GET_LEVEL(mob) = LIMIT(level, 1, LVL_IMPL);

  /* hit points roll */
  GET_HIT(mob) = 1;              /* number of hitpoint dice */
  GET_PSP(mob) = dice(1, level); /* size of hitpoint dice   */

  /* damroll */
  GET_DAMROLL(mob) = (level / 6) + 1; /* damroll (dam bonus) 1-6 */

  /* hitroll (remember that mobiles are using their class BAB already) */
  GET_HITROLL(mob) = (level / 6) + 1;

  /* saving throws (bonus) */
  GET_SAVE(mob, SAVING_FORT) = level / 4;
  GET_SAVE(mob, SAVING_REFL) = level / 4;
  GET_SAVE(mob, SAVING_WILL) = level / 4;
  GET_SAVE(mob, SAVING_POISON) = level / 4;
  GET_SAVE(mob, SAVING_DEATH) = level / 4;

  /* stats, default */
  (mob)->aff_abils.str = 10;
  (mob)->aff_abils.dex = 10;
  (mob)->aff_abils.con = 10;
  GET_INT(mob) = 10;
  GET_WIS(mob) = 10;
  GET_CHA(mob) = 10;
  bonus = level / 2; // bonus applied to stats

  /* hp, default */
  mobs_hps = (level * level) + (level * 10);

  /* damage dice default */
  GET_NDD(mob) = 1;     /* number damage dice */
  GET_SDD(mob) = level; /* size of damage dice */

  /* armor class default, d20 system * 10 */
  armor_class += level * 10; // 110 (11) - 400 (40)

  /* exp and gold */
  GET_EXP(mob) = (level * level * 75);
  GET_GOLD(mob) = (level * 10);

  /* class modifications to base */
  switch (GET_CLASS(mob))
  {
  case CLASS_WIZARD:
    mobs_hps = mobs_hps * 2 / 5;
    GET_SDD(mob) = GET_SDD(mob) * 2 / 5;
    armor_class -= 60;
    GET_INT(mob) += bonus;
    (mob)->aff_abils.dex += bonus;
    break;
  case CLASS_PSIONICIST:
    mobs_hps = mobs_hps * 2 / 5;
    GET_SDD(mob) = GET_SDD(mob) * 2 / 5;
    armor_class -= 60;
    GET_INT(mob) += bonus;
    (mob)->aff_abils.dex += bonus;
    GET_PSP(mob) = GET_LEVEL(mob) * 5;
    break;
  case CLASS_SORCERER:
    GET_CHA(mob) += bonus;
    (mob)->aff_abils.dex += bonus;
    mobs_hps = mobs_hps * 2 / 5;
    GET_SDD(mob) = GET_SDD(mob) * 2 / 5;
    armor_class -= 60;
    break;
  case CLASS_NECROMANCER:
    GET_CHA(mob) += bonus;
    (mob)->aff_abils.dex += bonus;
    mobs_hps = mobs_hps * 2 / 5;
    GET_SDD(mob) = GET_SDD(mob) * 2 / 5;
    armor_class -= 60;
    break;
  case CLASS_ROGUE:
    //    case CLASS_ASSASSIN:
    //    case CLASS_SHADOW_DANCER:
    (mob)->aff_abils.dex += bonus;
    (mob)->aff_abils.str += bonus;
    mobs_hps = mobs_hps * 3 / 5;
    armor_class -= 50;
    break;
  case CLASS_BARD:
    GET_CHA(mob) += bonus;
    (mob)->aff_abils.dex += bonus;
    GET_SDD(mob) = GET_SDD(mob) * 4 / 5;
    mobs_hps = mobs_hps * 3 / 5;
    armor_class -= 50;
    break;
  case CLASS_MONK:
    GET_WIS(mob) += bonus;
    (mob)->aff_abils.dex += bonus;
    mobs_hps = mobs_hps * 4 / 5;
    armor_class -= 60; // they will still get wis bonus
    break;
  case CLASS_CLERIC:
    (mob)->aff_abils.str += bonus;
    GET_WIS(mob) += bonus;
    GET_SDD(mob) = GET_SDD(mob) * 4 / 5;
    mobs_hps = mobs_hps * 4 / 5;
    armor_class -= 10;
    break; 
  case CLASS_DRUID:
  case CLASS_SHIFTER:
    GET_WIS(mob) += bonus;
    (mob)->aff_abils.dex += bonus;
    GET_SDD(mob) = GET_SDD(mob) * 4 / 5;
    mobs_hps = mobs_hps * 4 / 5;
    armor_class -= 50;
    break;
  case CLASS_BERSERKER:
  case CLASS_STALWART_DEFENDER:
    (mob)->aff_abils.str += bonus;
    (mob)->aff_abils.con += bonus;
    mobs_hps = mobs_hps * 6 / 5;
    armor_class -= 40;
    break;
  case CLASS_RANGER:
  case CLASS_DUELIST:
    (mob)->aff_abils.str += bonus;
    (mob)->aff_abils.dex += bonus;
    armor_class -= 50;
    break;
  case CLASS_SACRED_FIST:
    (mob)->aff_abils.wis += bonus;
    (mob)->aff_abils.dex += bonus;
    armor_class -= 50;
    break;
  case CLASS_WARRIOR:
    (mob)->aff_abils.str += bonus;
    (mob)->aff_abils.con += bonus;
    break;
  case CLASS_WEAPON_MASTER:
    (mob)->aff_abils.str += bonus;
    (mob)->aff_abils.dex += bonus;
    break;
  case CLASS_ARCANE_ARCHER:
    GET_INT(mob) += bonus;
    (mob)->aff_abils.dex += bonus;
    (mob)->aff_abils.cha += bonus;
    break;
  case CLASS_ARCANE_SHADOW:
    GET_INT(mob) += bonus;
    (mob)->aff_abils.dex += bonus;
    GET_INT(mob) += bonus;
    break;
  case CLASS_ELDRITCH_KNIGHT:
    GET_INT(mob) += bonus;
    (mob)->aff_abils.str += bonus;
    GET_INT(mob) += bonus;
    break;
  case CLASS_PALADIN:
    (mob)->aff_abils.str += bonus;
    GET_CHA(mob) += bonus;
    break;
  case CLASS_MYSTIC_THEURGE:
    mobs_hps = mobs_hps * 3 / 5;         // Average of cleric (4) and wizard (2)
    GET_SDD(mob) = GET_SDD(mob) * 3 / 5; // Average of cleric (4) and wizard (2)
    armor_class -= 60;                   // Use wizard-level AC.
    // Wizard stat bonuses
    GET_INT(mob) += bonus;
    (mob)->aff_abils.dex += bonus;
    // Cleric stat bonuses
    (mob)->aff_abils.str += bonus;
    GET_WIS(mob) += bonus;
    break;

  default:
    /* if we ned up here, just using wizard stats as default */
    mobs_hps = mobs_hps * 2 / 5;
    GET_SDD(mob) = GET_SDD(mob) * 2 / 5;
    armor_class -= 60;
    break;
  }

  /* racial mods */
  switch (GET_RACE(mob))
  {
  case RACE_TYPE_HUMANOID:
    break;
  case RACE_TYPE_UNDEAD:
    break;
  case RACE_TYPE_ANIMAL:
    GET_INT(mob) -= 7;
    GET_WIS(mob) -= 7;
    GET_CHA(mob) -= 7;
    GET_SAVE(mob, SAVING_FORT) += 4;
    GET_SAVE(mob, SAVING_REFL) += 4;
    GET_GOLD(mob) = 0;
    break;
  case RACE_TYPE_DRAGON:
    (mob)->aff_abils.dex += 6;
    (mob)->aff_abils.str += 6;
    (mob)->aff_abils.con += 6;
    GET_CHA(mob) += 6;
    GET_INT(mob) += 6;
    GET_WIS(mob) += 6;
    GET_SAVE(mob, SAVING_FORT) += 4;
    GET_SAVE(mob, SAVING_REFL) += 4;
    GET_SAVE(mob, SAVING_WILL) += 4;
    GET_SPELL_RES(mob) = 10 + level;
    break;
  case RACE_TYPE_GIANT:
    (mob)->aff_abils.str += 4;
    (mob)->aff_abils.con += 4;
    (mob)->aff_abils.dex -= 7;
    if (GET_SIZE(mob) < SIZE_LARGE)
      GET_REAL_SIZE(mob) = SIZE_LARGE;
    break;
  case RACE_TYPE_ABERRATION:
    GET_SAVE(mob, SAVING_WILL) += 4;
    break;
  case RACE_TYPE_CONSTRUCT:
    (mob)->aff_abils.str += 4;
    (mob)->aff_abils.con += 4;
    GET_SAVE(mob, SAVING_WILL) -= 4;
    GET_SAVE(mob, SAVING_FORT) -= 4;
    GET_SAVE(mob, SAVING_REFL) -= 4;
    break;
  case RACE_TYPE_ELEMENTAL:
    break;
  case RACE_TYPE_FEY:
    GET_SAVE(mob, SAVING_REFL) += 4;
    GET_SAVE(mob, SAVING_WILL) += 4;
    break;
  case RACE_TYPE_MAGICAL_BEAST:
    GET_SAVE(mob, SAVING_FORT) += 4;
    GET_SAVE(mob, SAVING_REFL) += 4;
    break;
  case RACE_TYPE_MONSTROUS_HUMANOID:
    break;
  case RACE_TYPE_OOZE:
    GET_SAVE(mob, SAVING_WILL) -= 4;
    GET_SAVE(mob, SAVING_FORT) -= 4;
    GET_SAVE(mob, SAVING_REFL) -= 4;
    break;
  case RACE_TYPE_OUTSIDER:
    break;
  case RACE_TYPE_PLANT:
    GET_GOLD(mob) = 0;
    break;
  case RACE_TYPE_VERMIN:
    GET_GOLD(mob) = 0;
    break;
  default:
    break;
  }

  /* group-required mobiles will be levels 31-34 */
  if (GET_LEVEL(mob) > 30)
  {
    int bonus_level = GET_LEVEL(mob) - 30;

    mobs_hps *= (bonus_level * 2);
    GET_DAMROLL(mob) += bonus_level;
    GET_EXP(mob) += (bonus_level * 5000);
    GET_GOLD(mob) += (bonus_level * 50);
  }

  (mob)->points.armor = armor_class;

  /* make sure mobs do at least 1d4 damage */
  if (GET_SDD(mob) < 4)
    GET_SDD(mob) = 4;

  /* we're auto-statting a live mob */
  if (realmode)
  {
    GET_REAL_DAMROLL(mob) = GET_DAMROLL(mob);
    GET_REAL_HITROLL(mob) = GET_HITROLL(mob);
    GET_REAL_SAVE(mob, SAVING_FORT) = GET_SAVE(mob, SAVING_FORT);
    GET_REAL_SAVE(mob, SAVING_REFL) = GET_SAVE(mob, SAVING_REFL);
    GET_REAL_SAVE(mob, SAVING_WILL) = GET_SAVE(mob, SAVING_WILL);
    GET_REAL_SAVE(mob, SAVING_POISON) = GET_SAVE(mob, SAVING_POISON);
    GET_REAL_SAVE(mob, SAVING_DEATH) = GET_SAVE(mob, SAVING_DEATH);
    GET_REAL_AC(mob) = GET_AC(mob);
    GET_HIT(mob) = mobs_hps;
    GET_REAL_MAX_HIT(mob) = mobs_hps;
    GET_REAL_STR(mob) = GET_STR(mob);
    GET_REAL_INT(mob) = GET_INT(mob);
    GET_REAL_WIS(mob) = GET_WIS(mob);
    GET_REAL_DEX(mob) = GET_DEX(mob);
    GET_REAL_CON(mob) = GET_CON(mob);
    GET_REAL_CHA(mob) = GET_CHA(mob);
    GET_REAL_SIZE(mob) = GET_SIZE(mob);
    GET_REAL_SPELL_RES(mob) = GET_SPELL_RES(mob);

    /* so far realmode is only for mobiles that shouldn't give xp/gold */
    GET_EXP(mob) = 0;
    GET_GOLD(mob) = 0;
    affect_total(mob);
  }
  else
  {
    /* not realmode, gotta convert hps back to moves */
    GET_MOVE(mob) = mobs_hps;
  }
}
#undef mobs_hps

void medit_autoroll_stats(struct descriptor_data *d)
{

  autoroll_mob(OLC_MOB(d), FALSE, FALSE);
}
