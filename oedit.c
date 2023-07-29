/**************************************************************************
 *  File: oedit.c                                      Part of LuminariMUD *
 *  Usage: Oasis OLC - Objects.                                            *
 *                                                                         *
 * By Levork. Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "spells.h"
#include "db.h"
#include "boards.h"
#include "constants.h"
#include "shop.h"
#include "genolc.h"
#include "genobj.h"
#include "genzon.h"
#include "oasis.h"
#include "improved-edit.h"
#include "dg_olc.h"
#include "fight.h"
#include "modify.h"
#include "clan.h"
#include "craft.h"
#include "spec_abilities.h"
#include "feats.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "treasure.h" /* set_weapon_object */
#include "act.h"      /* get_eq_score() */
#include "feats.h"
#include "handler.h"

/* local functions */
static void oedit_disp_size_menu(struct descriptor_data *d);
static void oedit_disp_mob_recipient_menu(struct descriptor_data *d);
static void oedit_setup_new(struct descriptor_data *d);
static void oedit_disp_container_flags_menu(struct descriptor_data *d);
static void oedit_disp_extradesc_menu(struct descriptor_data *d);
static void oedit_disp_weapon_spells(struct descriptor_data *d);
static void oedit_disp_prompt_apply_menu(struct descriptor_data *d);
// static void oedit_disp_apply_spec_menu(struct descriptor_data *d);
static void oedit_liquid_type(struct descriptor_data *d);
static void oedit_disp_apply_menu(struct descriptor_data *d);
// static void oedit_disp_weapon_menu(struct descriptor_data *d);
static void oedit_disp_spells_menu(struct descriptor_data *d);
static void oedit_disp_val1_menu(struct descriptor_data *d);
static void oedit_disp_val2_menu(struct descriptor_data *d);
static void oedit_disp_val3_menu(struct descriptor_data *d);
static void oedit_disp_val4_menu(struct descriptor_data *d);
static void oedit_disp_val5_menu(struct descriptor_data *d);
static void oedit_disp_val6_menu(struct descriptor_data *d);
// static void oedit_disp_prof_menu(struct descriptor_data *d);
static void oedit_disp_mats_menu(struct descriptor_data *d);
static void oedit_disp_type_menu(struct descriptor_data *d);
static void oedit_disp_extra_menu(struct descriptor_data *d);
static void oedit_disp_wear_menu(struct descriptor_data *d);
static void oedit_disp_menu(struct descriptor_data *d);
static void oedit_disp_perm_menu(struct descriptor_data *d);
static void oedit_save_to_disk(int zone_num);
static void oedit_disp_spellbook_menu(struct descriptor_data *d);
static void oedit_disp_weapon_special_abilities_menu(struct descriptor_data *d);
static void oedit_disp_assign_weapon_specab_menu(struct descriptor_data *d);

/* handy macro */
#define S_PRODUCT(s, i) ((s)->producing[(i)])

/* Utility and exported functions */
ACMD(do_oasis_oedit)
{
  int number = NOWHERE, save = 0, real_num;
  struct descriptor_data *d;
  const char *buf3;
  char buf1[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  /* Parse any arguments. */
  buf3 = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

  /* If there aren't any arguments they can't modify anything. */
  if (!*buf1)
  {
    send_to_char(ch, "Specify an object VNUM to edit.\r\n");
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

  /* If a numeric argument was given, get it. */
  if (number == NOWHERE)
    number = atoi(buf1);

  /* Check that whatever it is isn't already being edited. */
  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) == CON_OEDIT)
    {
      if (d->olc && OLC_NUM(d) == number)
      {
        send_to_char(ch, "That object is currently being edited by %s.\r\n",
                     PERS(d->character, ch));
        return;
      }
    }
  }

  /* Point d to the builder's descriptor (for easier typing later). */
  d = ch->desc;

  /* Give the descriptor an OLC structure. */
  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE,
           "SYSERR: do_oasis: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  /* Find the zone. */
  OLC_ZNUM(d) = save ? real_zone(number) : real_zone_by_thing(number);
  if (OLC_ZNUM(d) == NOWHERE)
  {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");

    /* Free the descriptor's OLC structure. */
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

  /* If we need to save, save the objects. */
  if (save)
  {
    send_to_char(ch, "Saving all objects in zone %d.\r\n",
                 zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE,
           "OLC: %s saves object info for zone %d.", GET_NAME(ch),
           zone_table[OLC_ZNUM(d)].number);

    /* Save the objects in this zone. */
    save_objects(OLC_ZNUM(d));

    /* Free the descriptor's OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;

  /* If a new object, setup new, otherwise setup the existing object. */
  if ((real_num = real_object(number)) != NOTHING)
    oedit_setup_existing(d, real_num, QMODE_NONE);
  else
    oedit_setup_new(d);

  oedit_disp_menu(d);
  STATE(d) = CON_OEDIT;

  /* Send the OLC message to the players in the same room as the builder. */
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  /* Log the OLC message. */
  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing zone %d allowed zone %d",
         GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

static void oedit_setup_new(struct descriptor_data *d)
{
  CREATE(OLC_OBJ(d), struct obj_data, 1);

  clear_object(OLC_OBJ(d));
  OLC_OBJ(d)->name = strdup("unfinished object");
  OLC_OBJ(d)->description = strdup("An unfinished object is lying here.");
  OLC_OBJ(d)->short_description = strdup("an unfinished object");
  SET_BIT_AR(GET_OBJ_WEAR(OLC_OBJ(d)), ITEM_WEAR_TAKE);
  GET_OBJ_BOUND_ID(OLC_OBJ(d)) = NOBODY;
  OLC_VAL(d) = 0;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  GET_OBJ_MATERIAL(OLC_OBJ(d)) = 0;
  GET_OBJ_PROF(OLC_OBJ(d)) = 0;
  GET_OBJ_SIZE(OLC_OBJ(d)) = SIZE_MEDIUM;
  OLC_OBJ(d)->mob_recepient = 0;
  SCRIPT(OLC_OBJ(d)) = NULL;
  OLC_OBJ(d)->proto_script = OLC_SCRIPT(d) = NULL;
  OLC_SPECAB(d) = NULL;
}

void oedit_setup_existing(struct descriptor_data *d, int real_num, int mode)
{
  struct obj_data *obj;

  /* Allocate object in memory. */
  CREATE(obj, struct obj_data, 1);
  copy_object(obj, &obj_proto[real_num]);

  /* Attach new object to player's descriptor. */
  OLC_OBJ(d) = obj;
  OLC_VAL(d) = 0;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  dg_olc_script_copy(d);
  /* The edited obj must not have a script. It will be assigned to the updated
   * obj later, after editing. */
  SCRIPT(obj) = NULL;
  OLC_OBJ(d)->proto_script = NULL;
}

void oedit_save_internally(struct descriptor_data *d)
{
  int i;
  obj_rnum robj_num;
  struct descriptor_data *dsc;
  struct obj_data *obj;

  i = (real_object(OLC_NUM(d)) == NOTHING);

  if ((robj_num = add_object(OLC_OBJ(d), OLC_NUM(d))) == NOTHING)
  {
    log("oedit_save_internally: add_object failed.");
    return;
  }

  /* Update triggers and free old proto list  */
  if (obj_proto[robj_num].proto_script &&
      obj_proto[robj_num].proto_script != OLC_SCRIPT(d))
    free_proto_script(&obj_proto[robj_num], OBJ_TRIGGER);
  /* this will handle new instances of the object: */
  obj_proto[robj_num].proto_script = OLC_SCRIPT(d);

  /* this takes care of the objects currently in-game */
  for (obj = object_list; obj; obj = obj->next)
  {
    if (obj->item_number != robj_num)
      continue;
    /* remove any old scripts */
    if (SCRIPT(obj))
      extract_script(obj, OBJ_TRIGGER);

    free_proto_script(obj, OBJ_TRIGGER);
    copy_proto_script(&obj_proto[robj_num], obj, OBJ_TRIGGER);
    assign_triggers(obj, OBJ_TRIGGER);
  }
  /* end trigger update */

  if (!i) /* If it's not a new object, don't renumber. */
    return;

  /* Renumber produce in shops being edited. */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_SEDIT)
      for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != NOTHING; i++)
        if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
          S_PRODUCT(OLC_SHOP(dsc), i)
  ++;

  /* Update other people in zedit too. From: C.Raehl 4/27/99 */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_ZEDIT)
      for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
        switch (OLC_ZONE(dsc)->cmd[i].command)
        {
        case 'P':
          OLC_ZONE(dsc)->cmd[i].arg3 += (OLC_ZONE(dsc)->cmd[i].arg3 >= robj_num);
          /* Fall through. */
        case 'E':
        case 'G':
        case 'O':
          OLC_ZONE(dsc)->cmd[i].arg1 += (OLC_ZONE(dsc)->cmd[i].arg1 >= robj_num);
          break;
        case 'R':
          OLC_ZONE(dsc)->cmd[i].arg2 += (OLC_ZONE(dsc)->cmd[i].arg2 >= robj_num);
          break;
        default:
          break;
        }
}

static void oedit_save_to_disk(int zone_num)
{
  save_objects(zone_num);
}

void oedit_disp_weapon_spells(struct descriptor_data *d)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int counter;
  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < MAX_WEAPON_SPELLS; counter++)
  {
    snprintf(buf, sizeof(buf), "[%s%d%s] Spell: %s%20s%s Level: %s%3d%s Percent: %s%3d%s Combat: %s%3d%s\r\n",
             cyn, counter + 1, nrm,
             cyn, spell_info[OLC_OBJ(d)->wpn_spells[counter].spellnum].name, nrm,
             cyn, OLC_OBJ(d)->wpn_spells[counter].level, nrm,
             cyn, OLC_OBJ(d)->wpn_spells[counter].percent, nrm,
             cyn, OLC_OBJ(d)->wpn_spells[counter].inCombat, nrm);
    send_to_char(d->character, "%s", buf);
  }
  send_to_char(d->character, "Enter spell to edit : ");
}

static void oedit_disp_lootbox_levels(struct descriptor_data *d)
{
  write_to_output(d,
                  "This will determine the maximum bonus to be found on the items in the chest.\r\n"
                  "Please choose the maximum grade of equipment that can drop from this chest.\r\n"
                  "1) Mundane\r\n"
                  "2) Minor (level 10 or less)\r\n"
                  "3) Typical(level 15 or less)\r\n"
                  "4) Medium (level 20 or less)\r\n"
                  "5) Major (level 25 or less)\r\n"
                  "6) Superior (level 26 or higher)\r\n"
                  "\r\nYour Choice: ");
}

static void oedit_disp_lootbox_types(struct descriptor_data *d)
{
  write_to_output(d,
                  "The type guarantees one item of the specified type.\r\n"
                  "Generic has equal chance for any type.  Gold provides 5x as much money.\r\n"
                  "Please choose the type of lootbox you'd like to create:\r\n"
                  "1) Generic, equal chance for all item types.\r\n"
                  "2) Weapons, guaranteed weapon, low chance for other items.\r\n"
                  "3) Armor, guaranteed armor, low chance for other items.\r\n"
                  "4) Consumables, guaranteed at least one consumable, low chance for other items.\r\n"
                  "5) Trinkets, guaranteed trinket (rings, bracers, etc), low chance for other items.\r\n"
                  "6) Gold, much more gold, low chance for other items.\r\n"
                  "7) Crystal, garaunteed %s, low chance for other items.\r\n"
                  "\r\nYour Choice: ",
                  CRAFTING_CRYSTAL);
}

/* Menu functions */

/* For container flags. */
static void oedit_disp_container_flags_menu(struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH] = {'\0'};
  get_char_colors(d->character);
  clear_screen(d);

  sprintbit(GET_OBJ_VAL(OLC_OBJ(d), 1), container_bits, bits, sizeof(bits));
  write_to_output(d,
                  "%s1%s) CLOSEABLE\r\n"
                  "%s2%s) PICKPROOF\r\n"
                  "%s3%s) CLOSED\r\n"
                  "%s4%s) LOCKED\r\n"
                  "Container flags: %s%s%s\r\n"
                  "Enter flag, 0 to quit : ",
                  grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, bits, nrm);
}

/* For extra descriptions. */
static void oedit_disp_extradesc_menu(struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);

  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d,
                  "Extra desc menu\r\n"
                  "%s1%s) Keywords: %s%s\r\n"
                  "%s2%s) Description:\r\n%s%s\r\n"
                  "%s3%s) Goto next description: %s\r\n"
                  "%s0%s) Quit\r\n"
                  "Enter choice : ",

                  grn, nrm, yel, (extra_desc->keyword && *extra_desc->keyword) ? extra_desc->keyword : "<NONE>",
                  grn, nrm, yel, (extra_desc->description && *extra_desc->description) ? extra_desc->description : "<NONE>",
                  grn, nrm, !extra_desc->next ? "Not set." : "Set.", grn, nrm);
  OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/* Ask for the bonus type for this apply. */
static void oedit_disp_apply_prompt_bonus_type_menu(struct descriptor_data *d)
{
  int i = 0;
  for (i = 0; i < NUM_BONUS_TYPES; i++)
  {
    write_to_output(d,
                    " %s%2d%s) %-20s",
                    nrm, i, nrm, bonus_types[i]);
    if (((i + 1) % 3) == 0)
      write_to_output(d, "\r\n");
  }
  write_to_output(d, "\r\nEnter the bonus type for this affect : ");
  OLC_MODE(d) = OEDIT_APPLY_BONUS_TYPE;
}

/* Ask for *which* apply to edit. */
static void oedit_disp_prompt_apply_menu(struct descriptor_data *d)
{
  char apply_buf[MAX_STRING_LENGTH] = {'\0'};
  int counter;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < MAX_OBJ_AFFECT; counter++)
  {
    if (OLC_OBJ(d)->affected[counter].modifier)
    {
      sprinttype(OLC_OBJ(d)->affected[counter].location, apply_types, apply_buf, sizeof(apply_buf));
      if (OLC_OBJ(d)->affected[counter].location == APPLY_FEAT)
      {
        write_to_output(d, " %s%d%s) Grant Feat %s (%s)\r\n", grn, counter + 1, nrm,
                        feat_list[(OLC_OBJ(d)->affected[counter].modifier < NUM_FEATS &&
                                           OLC_OBJ(d)->affected[counter].modifier > 0
                                       ? OLC_OBJ(d)->affected[counter].modifier
                                       : 0)]
                            .name,
                        bonus_types[OLC_OBJ(d)->affected[counter].bonus_type]);
      }
      else if (OLC_OBJ(d)->affected[counter].location == APPLY_SKILL)
      {
        write_to_output(d, " %s%d%s) Improves Skill %s by %d (%s)\r\n", grn, counter + 1, nrm,
                        ability_names[OLC_OBJ(d)->affected[counter].specific],
                        OLC_OBJ(d)->affected[counter].modifier,
                        bonus_types[OLC_OBJ(d)->affected[counter].bonus_type]);
      }
      else
      {
        write_to_output(d, " %s%d%s) %+d to %s (%s)\r\n", grn, counter + 1, nrm,
                        OLC_OBJ(d)->affected[counter].modifier, apply_buf, bonus_types[OLC_OBJ(d)->affected[counter].bonus_type]);
      }
    }
    else
    {
      write_to_output(d, " %s%d%s) None.\r\n", grn, counter + 1, nrm);
    }
  }
  write_to_output(d, "\r\nEnter affection to modify (0 to quit) : ");
  OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

void oedit_disp_prompt_spellbook_menu(struct descriptor_data *d)
{
  int counter, columns, i, u = 0;

  clear_screen(d);

  for (i = 1; i <= 9; i++)
  {
    columns = 0;
    write_to_output(d, "%s", !(columns % 3) ? "\r\n" : "");
    write_to_output(d, "---Circle %d Spells---===============================================---\r\n", i);
    for (counter = 0; counter < SPELLBOOK_SIZE; counter++)
    {
      if (OLC_OBJ(d)->sbinfo && OLC_OBJ(d)->sbinfo[counter].spellname != 0 &&
          OLC_OBJ(d)->sbinfo[counter].spellname < MAX_SPELLS &&
          ((spell_info[OLC_OBJ(d)->sbinfo[counter].spellname].min_level[CLASS_WIZARD] + 1) / 2) == i)
      {
        write_to_output(d, " %3d) %-20.20s %s", counter + 1, spell_info[OLC_OBJ(d)->sbinfo[counter].spellname].name, !(++columns % 3) ? "\r\n" : "");
        u++;
      }
    }
  }
  u++;
  if (u > SPELLBOOK_SIZE)
  {
    write_to_output(d, "\r\nEnter spell slot to modify (0 to quit) : ");
  }
  else
  {
    write_to_output(d, "\r\nEnter spell slot to modify [ next empty slot is %2d ] (0 to quit) : ", u);
  }
  OLC_MODE(d) = OEDIT_PROMPT_SPELLBOOK;
}

void oedit_disp_spellbook_menu(struct descriptor_data *d)
{
  int counter, columns, i;

  clear_screen(d);

  for (i = 1; i <= 9; i++)
  {
    columns = 0;
    write_to_output(d, "%s", !(columns % 3) ? "\n" : "");
    write_to_output(d, "---Circle %d Spells---==============================================---\r\n", i);
    for (counter = 0; counter < NUM_SPELLS; counter++)
    {
      if (((spell_info[counter].min_level[CLASS_WIZARD] + 1) / 2) == i &&
          spell_info[counter].schoolOfMagic != NOSCHOOL)
        write_to_output(d, "%3d) %-20.20s%s", counter, spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
    }
  }
  write_to_output(d, "\r\nEnter spell number (0 is no spell) : ");
  OLC_MODE(d) = OEDIT_SPELLBOOK;
}

static void oedit_disp_weapon_special_abilities_menu(struct descriptor_data *d)
{
  struct obj_special_ability *specab;
  bool found = FALSE;
  char actmtds[MAX_STRING_LENGTH] = {'\0'};
  int counter = 0;

  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d,
                  "Weapon special abilities menu\r\n");

  for (specab = OLC_OBJ(d)->special_abilities; specab != NULL; specab = specab->next)
  {
    counter++;
    found = TRUE;
    sprintbit(specab->activation_method, activation_methods, actmtds, MAX_STRING_LENGTH);
    write_to_output(d,
                    "%s%d%s) Ability: %s%s%s Level: %s%d%s\r\n"
                    "   Activation Methods: %s%s%s\r\n"
                    "   CommandWord: %s%s%s\r\n"
                    "   Values: [%s%d%s] [%s%d%s] [%s%d%s] [%s%d%s]\r\n",
                    grn, counter, nrm, yel, special_ability_info[specab->ability].name, nrm, yel, specab->level, nrm,
                    yel, actmtds, nrm,
                    yel, (specab->command_word == NULL ? "Not set." : specab->command_word), nrm,
                    yel, specab->value[0], nrm,
                    yel, specab->value[1], nrm,
                    yel, specab->value[2], nrm,
                    yel, specab->value[3], nrm);
  }
  if (!found)
    write_to_output(d, "No weapon special abilities assigned.\r\n");

  write_to_output(d,
                  "\r\n"
                  "%sN%s) Assign a new ability\r\n"
                  "%sE%s) Edit an assigned ability\r\n"
                  "%sD%s) Delete an assigned ability\r\n"
                  "%sQ%s) Quit\r\n"
                  "Enter choice : ",

                  grn, nrm,
                  grn, nrm,
                  grn, nrm,
                  grn, nrm);

  OLC_MODE(d) = OEDIT_WEAPON_SPECAB_MENU;
}

static void oedit_disp_assign_weapon_specab_menu(struct descriptor_data *d)
{

  struct obj_special_ability *specab;
  char actmtds[MAX_STRING_LENGTH] = {'\0'};

  specab = OLC_SPECAB(d);
  if (specab == NULL)
  {
    write_to_output(d, "Could not retrieve new weapon special ability.  Exiting.\r\n");
    oedit_disp_menu(d);
    return;
  }

  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d,
                  "Weapon special abilities menu\r\n");

  sprintbit(OLC_SPECAB(d)->activation_method, activation_methods, actmtds, MAX_STRING_LENGTH);

  write_to_output(d,
                  "%sA%s) Ability: %s%s%s\r\n"
                  "%sL%s) Level: %s%d%s\r\n"
                  "%sM%s) Activation Methods: %s%s%s\r\n"
                  "%sC%s) Command Word: %s%s%s\r\n"
                  "%sV%s) Values: [%s%d%s] [%s%d%s] [%s%d%s] [%s%d%s]\r\n"
                  "%sQ%s) Quit\r\n"
                  "Enter Choice : ",
                  grn, nrm, yel, special_ability_info[specab->ability].name, nrm,
                  grn, nrm, yel, specab->level, nrm,
                  grn, nrm, yel, actmtds, nrm,
                  grn, nrm, yel, (specab->command_word == NULL ? "Not set." : specab->command_word), nrm,
                  grn, nrm, yel, specab->value[0], nrm,
                  yel, specab->value[1], nrm,
                  yel, specab->value[2], nrm,
                  yel, specab->value[3], nrm,
                  grn, nrm);

  OLC_MODE(d) = OEDIT_ASSIGN_WEAPON_SPECAB_MENU;
}

static void oedit_weapon_specab(struct descriptor_data *d)
{
  const char *specab_names[NUM_SPECABS - 1]; /* We are ignoring the first, 0 value. */
  int i = 0;

  get_char_colors(d->character);
  clear_screen(d);

  /* we want to use column_list here, but we don't have a pre made list
   * of string values.  Make one, and make sure it is in order. */
  for (i = 0; i < NUM_SPECABS - 1; i++)
  {
    specab_names[i] = special_ability_info[i + 1].name;
  }

  column_list(d->character, 0, specab_names, NUM_SPECABS - 1, TRUE);
  write_to_output(d, "\r\n%sEnter weapon special ability : ", nrm);
  OLC_MODE(d) = OEDIT_WEAPON_SPECAB;
}

static void oedit_disp_specab_activation_method_menu(struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH] = {'\0'};
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ACTIVATION_METHODS; counter++)
  { /* added the -3 to prevent eyes/ears/badge */
    write_to_output(d, "%s%d%s) %-20.20s %s", grn, counter + 1, nrm,
                    activation_methods[counter], !(++columns % 2) ? "\r\n" : "");
  }

  sprintbit(OLC_SPECAB(d)->activation_method, activation_methods, bits, MAX_STRING_LENGTH);
  write_to_output(d, "\r\nActivation Methods: %s%s%s\r\n"
                     "Enter Activation Method, 0 to quit : ",
                  cyn, bits, nrm);
}

void oedit_disp_specab_bane_race(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_RACE_TYPES; counter++)
  {
    write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                    race_family_types[counter], !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%sEnter race number : ", nrm);
}

void oedit_disp_specab_bane_subrace(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_SUB_RACES; counter++)
  {
    write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                    npc_subrace_types[counter], !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%sEnter subrace number : ", nrm);
}

/* Menu for APPLY_FEAT */
#if 0

void oedit_disp_apply_spec_menu(struct descriptor_data *d) {
  char *buf;
  int i, count = 0;

  switch (OLC_OBJ(d)->affected[OLC_VAL(d)].location) {
    case APPLY_FEAT:
      for (i = 0; i < NUM_FEATS; i++) {
        if (feat_list[i].in_game) {
          count++;
          write_to_output(d, "%s%3d%s) %s%-14.14s ", grn, i, nrm, yel, feat_list[i].name);
          if (count % 4 == 3)
            write_to_output(d, "\r\n");
        }
      }

      buf = "\r\nWhat feat should be modified : ";
      break;
      /*
      case APPLY_SKILL:
        buf = "What skill should be modified : ";
        break;
       */
    default:
      oedit_disp_prompt_apply_menu(d);
      return;
  }

  write_to_output(d, "\r\n%s", buf);
  OLC_MODE(d) = OEDIT_APPLYSPEC;
}
#endif

/* Ask for liquid type. */
static void oedit_liquid_type(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  column_list(d->character, 0, drinks, NUM_LIQ_TYPES, TRUE);
  write_to_output(d, "\r\n%sEnter drink type : ", nrm);
  OLC_MODE(d) = OEDIT_VALUE_3;
}

/* The actual apply to set. */
static void oedit_disp_apply_menu(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  column_list(d->character, 0, apply_types, NUM_APPLIES, TRUE);
  write_to_output(d, "\r\nEnter apply type (0 is no apply)\r\n(for 'grant feat' select featnum here, 'featlist' out of editor for master list) : ");
  OLC_MODE(d) = OEDIT_APPLY;
}

/* Weapon type. */

/*
static void oedit_disp_weapon_menu(struct descriptor_data *d) {
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ATTACK_TYPES; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
            attack_hit_text[counter].singular,
            !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter weapon type : ");
}
 */

static void oedit_disp_portaltypes_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_PORTAL_TYPES; counter++)
  {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
                    portal_types[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter portal type : ");
}

/* ranged combat, weapon-type (like bow vs crossbow) */
static void oedit_disp_ranged_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_RANGED_WEAPONS; counter++)
  {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
                    ranged_weapons[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter ranged-weapon type : ");
}

/* instruments for bardic performance */
static void oedit_disp_instrument_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < MAX_INSTRUMENTS; counter++)
  {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
                    instrument_names[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nSelect instrument type : ");
}

/* ranged combat, missile-type (like arrow vs bolt) */
static void oedit_disp_missile_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_AMMO_TYPES; counter++)
  {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
                    ammo_types[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter missile-weapon type : ");
}

/* Spell type. */
static void oedit_disp_spells_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 1; counter < NUM_SPELLS; counter++)
  {
    write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                    spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%sEnter spell choice (-1 for none) : ", nrm);
}

static void oedit_disp_poisons_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = POISON_TYPE_START; counter <= POISON_TYPE_END; counter++)
  {
    write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
                    spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%sEnter poison choice (-1 for none) : ", nrm);
}

static void oedit_disp_trap_type(struct descriptor_data *d)
{
  int counter = 0;

  write_to_output(d, "\r\n");
  for (counter = 0; counter < MAX_TRAP_TYPES; counter++)
  {
    write_to_output(d, "%d) %s\r\n", counter, trap_type[counter]);
  }

  write_to_output(d, "\r\n%sEnter trap choice # : ", nrm);
}

static void oedit_disp_trap_effects(struct descriptor_data *d)
{
  int counter = 0;

  write_to_output(d, "\r\n");
  for (counter = TRAP_EFFECT_FIRST_VALUE; counter < TOP_TRAP_EFFECTS; counter++)
  {
    write_to_output(d, "%d) %s\r\n", counter, trap_effects[counter - 1000]);
  }

  write_to_output(d, "\r\n%s(You can also choose any spellnum)\r\n", nrm);
  write_to_output(d, "%sEnter effect # : ", nrm);
}

static void oedit_disp_trap_direction(struct descriptor_data *d)
{
  int counter = 0;

  write_to_output(d, "\r\n");
  for (counter = 0; counter < NUM_OF_INGAME_DIRS; counter++)
  {
    write_to_output(d, "%d) %s\r\n", counter, dirs[counter]);
  }

  write_to_output(d, "\r\n%sEnter direction # : ", nrm);
}

void oedit_disp_armor_type_menu(struct descriptor_data *d)
{
  const char *armor_types[NUM_SPEC_ARMOR_TYPES - 1];
  int i = 0;

  /* we want to use column_list here, but we don't have a pre made list
   * of string values (without undefined).  Make one, and make sure it is in order. */
  for (i = 0; i < NUM_SPEC_ARMOR_TYPES - 1; i++)
  {
    armor_types[i] = armor_list[i + 1].name;
  }

  column_list(d->character, 3, armor_types, NUM_SPEC_ARMOR_TYPES - 1, TRUE);
}

void oedit_disp_weapon_type_menu(struct descriptor_data *d)
{
  const char *weapon_types[NUM_WEAPON_TYPES - 1];
  int i = 0;

  /* we want to use column_list here, but we don't have a pre made list
   * of string values (without undefined).  Make one, and make sure it is in order. */
  for (i = 0; i < NUM_WEAPON_TYPES - 1; i++)
  {
    weapon_types[i] = weapon_list[i + 1].name;
  }

  column_list(d->character, 3, weapon_types, NUM_WEAPON_TYPES - 1, TRUE);
}

int compute_ranged_weapon_actual_value(int list_value)
{
  int weapon_types[NUM_WEAPON_TYPES];
  int i = 1, counter = 0;

  for (i = 1; i < NUM_WEAPON_TYPES; i++)
  {
    if (IS_SET(weapon_list[i].weaponFlags, WEAPON_FLAG_RANGED))
    {
      weapon_types[counter] = i; /* place weapon type into the array */
      counter++;
    }
  }

  for (i = 1; i < counter - 1; i++)
  {
    if (i == list_value)
    {
      return weapon_types[i];
    }
  }

  return -1; /* failed */
}

/*
static void oedit_disp_ranged_weapons_menu(struct descriptor_data *d) {
  const char *weapon_types[NUM_WEAPON_TYPES];
  int i = 1, counter = 0;

  // we want to use column_list here, but we don't have a pre made list
  // of string values (without undefined).  Make one, and make sure it is in order.
  for (i = 1; i < NUM_WEAPON_TYPES; i++) {
    if (IS_SET(weapon_list[i].weaponFlags, WEAPON_FLAG_RANGED)) {
      weapon_types[counter] = weapon_list[i].name;
      counter++;
    }
  }

  column_list(d->character, 3, weapon_types, counter, TRUE);
}
 */

/* Object value #1 */
static void oedit_disp_val1_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_1;
  switch (GET_OBJ_TYPE(OLC_OBJ(d)))
  {
  case ITEM_GEAR_OUTFIT:
    write_to_output(d, "\r\n"
                       "1) Weapon\r\n"
                       "2) Armor Set\r\n"
                       "Choose the Type of Outfit: ");
    break;
  case ITEM_SWITCH:
    write_to_output(d, "What command to activate switch? (0=pull, 1=push) : ");
    break;
  case ITEM_TRAP:
    oedit_disp_trap_type(d);
    break;
  case ITEM_LIGHT:
    /* values 0 and 1 are unused.. jump to 2 */
    oedit_disp_val3_menu(d);
    break;
  case ITEM_SCROLL:
  case ITEM_WAND:
  case ITEM_STAFF:
  case ITEM_POTION:
    write_to_output(d, "Spell level : ");
    break;
  case ITEM_WEAPON:
    /* Weapon Type - Onir */
    oedit_disp_weapon_type_menu(d);
    write_to_output(d, "\r\nChoose a weapon type : ");
    break;
  case ITEM_POISON:
    oedit_disp_spells_menu(d);
    write_to_output(d, "\r\n");
    oedit_disp_poisons_menu(d);
    break;
  case ITEM_ARMOR:
  case ITEM_CLANARMOR:
    /* values 0 is reserved for Apply to AC */
    oedit_disp_val2_menu(d);
    break;
  case ITEM_CONTAINER:
  case ITEM_AMMO_POUCH:
    write_to_output(d, "Max weight to contain (-1 for unlimited) : ");
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    write_to_output(d, "Max drink units (-1 for unlimited) : ");
    break;
  case ITEM_FOOD:
  case ITEM_DRINK:
    write_to_output(d, "How many rounds (6 seconds) will it last? : ");
    break;
  case ITEM_MONEY:
    write_to_output(d, "Number of gold coins : ");
    break;
  case ITEM_PORTAL:
    oedit_disp_portaltypes_menu(d);
    break;
  case ITEM_FURNITURE:
    write_to_output(d, "Number of people it can hold : ");
    break;
    /* NewCraft */
  case ITEM_BLUEPRINT:
    write_to_output(d, "Enter Craft ID number : ");
    break;
  case ITEM_FIREWEAPON:
    oedit_disp_ranged_menu(d);
    break;
  case ITEM_MISSILE:
    oedit_disp_missile_menu(d);
    break;
  case ITEM_INSTRUMENT:
    oedit_disp_instrument_menu(d);
    break;
  case ITEM_WORN:
    write_to_output(d, "Special value for worn gear (example gloves for monk-gloves enhancement: ");
    break;
  case ITEM_BOAT: // these object types have no 'values' so go back to menu
  case ITEM_KEY:
  case ITEM_NOTE:
  case ITEM_OTHER:
  case ITEM_PLANT:
  case ITEM_PEN:
  case ITEM_TRASH:
  case ITEM_TREASURE:
    oedit_disp_menu(d);
    break;
  case ITEM_TREASURE_CHEST:
    oedit_disp_lootbox_levels(d);
    break;
  default:
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Reached default case in oedit_disp_val1_menu()!");
    break;
  }
}

/* Object value #2 */
static void oedit_disp_val2_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_2;
  switch (GET_OBJ_TYPE(OLC_OBJ(d)))
  {
  case ITEM_GEAR_OUTFIT:
    write_to_output(d, "What is the enhancement bonus? ");
    break;
  case ITEM_SWITCH:
    write_to_output(d, "Which room vnum to manipulate? : ");
    break;
  case ITEM_TRAP:
    switch (GET_OBJ_VAL(OLC_OBJ(d), 0))
    {
      break;
    case TRAP_TYPE_OPEN_DOOR:
    case TRAP_TYPE_UNLOCK_DOOR:
      oedit_disp_trap_direction(d);
      break;
    case TRAP_TYPE_OPEN_CONTAINER:
    case TRAP_TYPE_UNLOCK_CONTAINER:
    case TRAP_TYPE_GET_OBJECT:
      write_to_output(d, "VNUM of object trap should apply to : ");
      break;
    case TRAP_TYPE_LEAVE_ROOM:
    case TRAP_TYPE_ENTER_ROOM:
    default:
      write_to_output(d, "Press ENTER to continue.");
      break;
    }
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_POISON:
    write_to_output(d, "Level of poison : ");
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    write_to_output(d, "Max number of charges : ");
    break;
    /* Changed to use standard values for weapons. */
    /*case ITEM_WEAPON:
        write_to_output(d, "Number of damage dice (%d) : ", GET_OBJ_VAL(OLC_OBJ(d), 1));
        break;
       */
  case ITEM_ARMOR:
    // case ITEM_CLANARMOR:
    /* Armor Type - zusuk */
    oedit_disp_armor_type_menu(d);
    write_to_output(d, "\r\nChoose an armor type : ");
    break;
  case ITEM_FIREWEAPON:
    write_to_output(d, "Number of damage dice : ");
    break;
  case ITEM_MISSILE:
    // write_to_output(d, "Size of damage dice : ");
    break;
  case ITEM_AMMO_POUCH:
  case ITEM_CONTAINER:
    /* These are flags, needs a bit of special handling. */
    oedit_disp_container_flags_menu(d);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    write_to_output(d, "Initial drink units : ");
    break;
  case ITEM_CLANARMOR:
    write_to_output(d, "Clan ID Number: ");
    break;
  case ITEM_INSTRUMENT:
    write_to_output(d, "Enter how much instrument reduces difficulty (0-30): ");
    break;
  case ITEM_PORTAL:
    switch (GET_OBJ_VAL(OLC_OBJ(d), 0))
    {
    case PORTAL_NORMAL:
    case PORTAL_CHECKFLAGS:
      write_to_output(d, "Room VNUM portal points to : ");
      break;

    case PORTAL_RANDOM:
      write_to_output(d, "Lowest room VNUM in range : ");
      break;

      /* Always sends player to their own clanhall - no room required */
    case PORTAL_CLANHALL:
      oedit_disp_menu(d);
      break;
    }
    break;
  case ITEM_TREASURE_CHEST:
    oedit_disp_lootbox_types(d);
    break;

  default:
    oedit_disp_menu(d);
  }
}

/* Object value #3 */
static void oedit_disp_val3_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_3;
  switch (GET_OBJ_TYPE(OLC_OBJ(d)))
  {
  case ITEM_GEAR_OUTFIT:
    oedit_disp_mats_menu(d);
    break;
  case ITEM_SWITCH:
    write_to_output(d, "Which direction? (0=n, 1=e, 2=s, 3=w, 4=u, 5=d) : ");
    break;
  case ITEM_TRAP:
    oedit_disp_trap_effects(d);
    break;
  case ITEM_LIGHT:
    write_to_output(d, "Number of hours (0 = burnt, -1 is infinite) : ");
    break;
  case ITEM_POISON:
    write_to_output(d, "Applications : ");
    break;
  case ITEM_INSTRUMENT:
    write_to_output(d, "Instrument Level (0-10): ");
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    write_to_output(d, "Number of charges remaining : ");
    break;
    /* Use standard values for weapons */
    /*
      case ITEM_WEAPON:
        write_to_output(d, "Size of damage dice : ");
        break;
       */
  case ITEM_FIREWEAPON:
    write_to_output(d, "Breaking probability : ");
    break;
  case ITEM_MISSILE:
    write_to_output(d, "Breaking probability : ");
    break;
  case ITEM_CONTAINER:
  case ITEM_AMMO_POUCH:
    write_to_output(d, "Vnum of key to open container (-1 for no key) : ");
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    oedit_liquid_type(d);
    break;
  case ITEM_TREASURE_CHEST:
    write_to_output(d, "Is this a randomly loading chest? 1 if yes, 0 if no.\r\n"
                       "A randomly loading chest will only load in rooms and/or zones flagged with RANDOM_CHEST.\r\n"
                      "They will not have a cooldown timer to loot, and will be extracted when looted.\r\n");
    break;
  case ITEM_PORTAL:
    switch (GET_OBJ_VAL(OLC_OBJ(d), 0))
    {
    case PORTAL_NORMAL:
    case PORTAL_CHECKFLAGS:
      oedit_disp_menu(d); /* We are done for these portal types */
      break;

    case PORTAL_RANDOM:
      write_to_output(d, "Highest room VNUM in range : ");
      break;
    }
    break;

  default:
    oedit_disp_menu(d);
  }
}

/* Object value #4 */
static void oedit_disp_val4_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_4;
  switch (GET_OBJ_TYPE(OLC_OBJ(d)))
  {
  case ITEM_TREASURE_CHEST:
    write_to_output(d, "Search DC (0 for not hidden): ");
    break;
  case ITEM_GEAR_OUTFIT:
    get_char_colors(d->character);
    clear_screen(d);
    column_list(d->character, 0, apply_types, NUM_APPLIES, TRUE);
    write_to_output(d, "\r\nEnter apply type (0 is no apply)\r\n");
    write_to_output(d, "Please select an apply type to add to the item, or select none for nothing.\r\n");
    write_to_output(d, "Enter your choice: ");
    break;
  case ITEM_SWITCH:
    write_to_output(d, "Which command (0=unhide, 1=unlock, 2=open) : ");
    break;
  case ITEM_TRAP:
    write_to_output(d, "Recommendations:\r\n");
    write_to_output(d, "DC = 20 + level-of-trap for a normal trap\r\n");
    write_to_output(d, "DC = 30 + level-of-trap for a hard trap\r\n");
    write_to_output(d, "DC = 40 + level-of-trap for an epic trap\r\n");
    write_to_output(d, "Enter trap difficulty class (DC) : ");
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
  case ITEM_WAND:
  case ITEM_STAFF:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_POISON:
    write_to_output(d, "Hits per Application : ");
    break;
  case ITEM_INSTRUMENT:
    write_to_output(d, "Instrument Breakability (0 = unbreakable, 2000 = will "
                       "break on first use) (recommended values 0-30): ");
    break;
  case ITEM_WEAPON:
    // oedit_disp_weapon_menu(d);
    break;
  case ITEM_MISSILE:
    // oedit_disp_weapon_menu(d);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    write_to_output(d, "Spell # (0 = no spell) : ");
    break;
  default:
    oedit_disp_menu(d);
  }
}

/* Object value #5 */
static void oedit_disp_val5_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_VALUE_5;
  switch (GET_OBJ_TYPE(OLC_OBJ(d)))
  {
  case ITEM_TREASURE_CHEST:
    write_to_output(d, "Pick Lock DC (0 for not locked): ");
    break;
  case ITEM_GEAR_OUTFIT:
    write_to_output(d, "Apply modifier amount: ");
    break;
  case ITEM_WEAPON:
    write_to_output(d, "Enhancement bonus : ");
    break;
  case ITEM_ARMOR:
    write_to_output(d, "Enhancement bonus : ");
    break;
  case ITEM_MISSILE:
    write_to_output(d, "Enhancement bonus : ");
    break;
  default:
    oedit_disp_menu(d);
  }
}

/* Object value #6 */
static void oedit_disp_val6_menu(struct descriptor_data *d)
{
  int i = 0;
  OLC_MODE(d) = OEDIT_VALUE_6;
  switch (GET_OBJ_TYPE(OLC_OBJ(d)))
  {
  case ITEM_TREASURE_CHEST:
    write_to_output(d, "Trap Type (0 for no trap):\r\n");
    column_list(d->character, 0, trap_effects, MAX_TRAP_EFFECTS, TRUE);
    break;
  case ITEM_GEAR_OUTFIT:
    for (i = 0; i < NUM_BONUS_TYPES; i++)
    {
      write_to_output(d, " %s%2d%s) %-20s", nrm, i, nrm, bonus_types[i]);
      if (((i + 1) % 3) == 0)
        write_to_output(d, "\r\n");
    }
    write_to_output(d, "\r\nEnter the bonus type for this affect : ");
    break;
  default:
    oedit_disp_menu(d);
  }
}

static void oedit_disp_specab_val1_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_SPECAB_VALUE_1;
  switch (OLC_SPECAB(d)->ability)
  {
  case WEAPON_SPECAB_BANE:
    oedit_disp_specab_bane_race(d);
    break;
  case ITEM_SPECAB_HORN_OF_SUMMONING:
    write_to_output(d, "Enter the vnum of the mob to summon : ");
    break;
  case ITEM_SPECAB_ITEM_SUMMON:
    write_to_output(d, "Enter the vnum of the mob to summon : ");
    break;
  default:
    OLC_MODE(d) = OEDIT_ASSIGN_WEAPON_SPECAB_MENU;
    oedit_disp_assign_weapon_specab_menu(d);
  }
}

static void oedit_disp_specab_val2_menu(struct descriptor_data *d)
{
  OLC_MODE(d) = OEDIT_SPECAB_VALUE_2;
  switch (OLC_SPECAB(d)->ability)
  {
  case WEAPON_SPECAB_BANE:
    oedit_disp_specab_bane_subrace(d);
    break;
  default:
    OLC_MODE(d) = OEDIT_ASSIGN_WEAPON_SPECAB_MENU;
    oedit_disp_assign_weapon_specab_menu(d);
  }
}

/* Object type. */
static void oedit_disp_type_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ITEM_TYPES; counter++)
  {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
                    item_types[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter object type : ");
}

// item proficiency
/*
static void oedit_disp_prof_menu(struct descriptor_data *d) {
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ITEM_PROFS; counter++) {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
            item_profs[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter object proficiency : ");
}
 */

// item material

static void oedit_disp_mats_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_MATERIALS; counter++)
  {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
                    material_name[counter], !(++columns % 2) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter object material : ");
}

/* Object extra flags. */
static void oedit_disp_extra_menu(struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH] = {'\0'};
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ITEM_FLAGS; counter++)
  {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
                    extra_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbitarray(GET_OBJ_EXTRA(OLC_OBJ(d)), extra_bits, EF_ARRAY_MAX, bits);
  write_to_output(d, "\r\nObject flags: %s%s%s\r\n"
                     "Enter object extra flag (0 to quit) : ",
                  cyn, bits, nrm);
}

/* Object perm flags. */
static void oedit_disp_perm_menu(struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH] = {'\0'};
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 1; counter < NUM_AFF_FLAGS; counter++)
  {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm, affected_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbitarray(GET_OBJ_PERM(OLC_OBJ(d)), affected_bits, EF_ARRAY_MAX, bits);
  write_to_output(d, "\r\nObject permanent flags: %s%s%s\r\n"
                     "Enter object perm flag (0 to quit) : ",
                  cyn, bits, nrm);
}

/* Object size */
void oedit_disp_size_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  clear_screen(d);

  for (counter = 0; counter < NUM_SIZES; counter++)
  {
    write_to_output(d, "%2d) %-20.20s%s", counter + 1,
                    size_names[counter], !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\nEnter object size : ");
}

/* Object Mob Recipient */
void oedit_disp_mob_recipient_menu(struct descriptor_data *d)
{

  clear_screen(d);

  write_to_output(d, "The mob recipient is the vnum of the mob that object belongs to.\r\n"
                     "The object cannot be given to any mob except the mob with this vnum.\r\n"
                     "Enter 0 if you want this object to be freely given to any mob.  This\r\n"
                     "will not prevent trading the object between players.\r\n"
                     "Enter mob vnum: ");
}

/* Object wear flags. */
static void oedit_disp_wear_menu(struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH] = {'\0'};
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = 0; counter < NUM_ITEM_WEARS; counter++)
  {
    write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
                    wear_bits[counter], !(++columns % 2) ? "\r\n" : "");
  }
  sprintbitarray(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, TW_ARRAY_MAX, bits);
  write_to_output(d, "\r\nWear flags: %s%s%s\r\n"
                     "Enter wear flag, 0 to quit : ",
                  cyn, bits, nrm);
}

bool remove_special_ability(struct obj_data *obj, int number)
{
  bool deleted = FALSE;
  int i;
  struct obj_special_ability *specab, *prev_specab;

  specab = obj->special_abilities;
  prev_specab = NULL;

  for (i = 1; (i < number) && (specab != NULL); i++)
  {
    prev_specab = specab;
    specab = specab->next;
  }
  /* Check to see if we found the ability. */
  if ((i == number) && (specab != NULL))
  {

    deleted = TRUE;

    /* Remove it from the list. */
    if (prev_specab == NULL)
      obj->special_abilities = specab->next;
    else
      prev_specab->next = specab->next;

    /* Free up the memory. */
    if (specab->command_word != NULL)
      free(specab->command_word);
    free(specab);
  }

  return deleted;
}

struct obj_special_ability *get_specab_by_position(struct obj_data *obj, int position)
{
  int i;
  struct obj_special_ability *specab, *prev_specab;

  specab = obj->special_abilities;
  prev_specab = NULL;

  for (i = 1; (i < position) && (specab != NULL); i++)
  {
    prev_specab = specab;
    specab = specab->next;
  }

  return specab;
}

/* Display main menu. */
static void oedit_disp_menu(struct descriptor_data *d)
{
  char buf1[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  char buf3[MAX_STRING_LENGTH] = {'\0'};
  struct obj_data *obj = OLC_OBJ(d);
  // int i = 0;
  size_t len = 0;

  get_char_colors(d->character);
  clear_screen(d);

  /* Build buffers for object type */
  sprinttype(GET_OBJ_TYPE(obj), item_types, buf1, sizeof(buf1));

  /* build buffer for obj extras */
  sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, EF_ARRAY_MAX, buf2);

  /* Build first half of menu. */
  write_to_output(d,
                  "-- Item number : [%s%d%s]\r\n"
                  "%s1%s) Keywords : %s%s\r\n"
                  "%s2%s) S-Desc   : %s%s\r\n"
                  "%s3%s) L-Desc   :-\r\n%s%s\r\n"
                  "%s4%s) A-Desc   :-\r\n%s%s"
                  "%s5%s) Type        : %s%s\r\n"
                  //"%sG%s) Proficiency : %s%s\r\n"
                  "%s6%s) Extra flags : %s%s\r\n",

                  cyn, OLC_NUM(d), nrm,
                  grn, nrm, yel, (obj->name && *obj->name) ? obj->name : "undefined",
                  grn, nrm, yel, (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
                  grn, nrm, yel, (obj->description && *obj->description) ? obj->description : "undefined",
                  grn, nrm, yel, (obj->action_description && *obj->action_description) ? obj->action_description : "Not Set.\r\n",
                  grn, nrm, cyn, buf1,
                  // grn, nrm, cyn, item_profs[GET_OBJ_PROF(obj)],
                  grn, nrm, cyn, buf2);

  /* Send first half then build second half of menu. */

  /* wear slots of gear */
  sprintbitarray(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, EF_ARRAY_MAX, buf1);
  /* permanent affections of gear */
  sprintbitarray(GET_OBJ_PERM(OLC_OBJ(d)), affected_bits, EF_ARRAY_MAX, buf2);

  /* build a buffer for displaying suggested worn eq stats -zusuk */
  /* we have to fix this so treasure + here are synced! */
  if (GET_OBJ_RNUM(obj) != NOTHING)
  {
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_FINGER))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-finger:wis,will,hp,res-fire,res-punc,res-illus,res-energy] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_NECK))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-neck:int,save-ref,res-cold,res-air,res-force,res-mental,res-water] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_BODY))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-body:NONE] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HEAD))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-head:NONE] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_LEGS))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-legs:NONE] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_FEET))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-feet:res-poison,dex,moves] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HANDS))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-hands:res-disease,res-slice,str] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_ARMS))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-arms:NONE] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_SHIELD))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-shield:NONE] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_ABOUT))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-about:res-acid,cha,res-negative] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WAIST))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-waist:res-holy,con,res-earth] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WRIST))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-wrist:save-fort,psp,res-elec,res-unholy,res-sound,res-light] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WIELD))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-wield:NONE] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HOLD))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-hold:int,cha,hps] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_FACE))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-face:NONE] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_AMMO_POUCH))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-ammopouch:NONE] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_EAR))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-ear:NONE] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_EYES))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-eyes:NONE] ");
    if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_BADGE))
      len += snprintf(buf3 + len, sizeof(buf3) - len,
                      "[wear-badge:NONE] ");
  }
  /* end eq-wear suggestions */

  write_to_output(d,
                  "%s7%s) Wear flags  : %s%s\r\n"
                  "%sH%s) Material    : %s%s\r\n"
                  "%s8%s) Weight      : %s%d\r\n"
                  "%sI%s) Size        : %s%s\r\n"
                  "%s9%s) Cost        : %s%d\r\n"
                  "%sA%s) Cost/Day    : %s%d\r\n"
                  "%sB%s) Timer       : %s%d\r\n"
                  "%sC%s) Values      : %s%d %d %d %d %d %d %d %d\r\n"
                  "                 %d %d %d %d %d %d %d %d\r\n"
                  "%s%s%s"
                  "%sD%s) Applies menu\r\n"
                  "%sE%s) Extra descriptions menu: %s%s%s\r\n"
                  "%sF%s) Weapon Spells          : %s%s\r\n"
                  "%sJ%s) Special Abilities      : %s%s\r\n"
                  "%sM%s) Min Level              : %s%d\r\n"
                  "%sP%s) Perm Affects           : %s%s\r\n"
                  "%sR%s) Mob Recipient          : %s%d\r\n"
                  "%sS%s) Script                 : %s%s\r\n"
                  "%sT%s) Spellbook menu\r\n"
                  "%sEQ Rating (save/exit to update, under development): %s%d\r\n"
                  "%sSuggested affections (save/exit first): %s%s\r\n"
                  "%sW%s) Copy object\r\n"
                  "%sX%s) Delete object\r\n"
                  "%sQ%s) Quit\r\n"
                  "Enter choice : ",

                  grn, nrm, cyn, buf1,
                  grn, nrm, cyn, material_name[GET_OBJ_MATERIAL(obj)],
                  grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
                  grn, nrm, cyn, size_names[GET_OBJ_SIZE(obj)],
                  grn, nrm, cyn, GET_OBJ_COST(obj),
                  grn, nrm, cyn, GET_OBJ_RENT(obj),
                  grn, nrm, cyn, GET_OBJ_TIMER(obj),
                  grn, nrm, cyn,
                  GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3),
                  GET_OBJ_VAL(obj, 4), GET_OBJ_VAL(obj, 5), GET_OBJ_VAL(obj, 6), GET_OBJ_VAL(obj, 7),
                  GET_OBJ_VAL(obj, 8), GET_OBJ_VAL(obj, 9), GET_OBJ_VAL(obj, 10), GET_OBJ_VAL(obj, 11),
                  GET_OBJ_VAL(obj, 12), GET_OBJ_VAL(obj, 13), GET_OBJ_VAL(obj, 14), GET_OBJ_VAL(obj, 15),
                  GET_OBJ_TYPE(obj) == ITEM_ARMOR ? "Armor Type: " : (GET_OBJ_TYPE(obj) == ITEM_WEAPON ? "Weapon Type: " : ""),
                  GET_OBJ_TYPE(obj) == ITEM_ARMOR ? armor_list[GET_OBJ_VAL(obj, 1)].name : (GET_OBJ_TYPE(obj) == ITEM_WEAPON ? weapon_list[GET_OBJ_VAL(obj, 0)].name : ""),
                  (GET_OBJ_TYPE(obj) == ITEM_ARMOR || GET_OBJ_TYPE(obj) == ITEM_WEAPON) ? "\r\n" : "",
                  grn, nrm, grn, nrm, cyn, obj->ex_description ? "Set." : "Not Set.", grn,
                  grn, nrm, cyn, HAS_SPELLS(obj) ? "Set." : "Not set.",
                  grn, nrm, cyn, HAS_SPECIAL_ABILITIES(obj) ? "Set." : "Not Set.",
                  grn, nrm, cyn, GET_OBJ_LEVEL(obj),
                  grn, nrm, cyn, buf2,
                  grn, nrm, cyn, (obj)->mob_recepient,
                  grn, nrm, cyn, OLC_SCRIPT(d) ? "Set." : "Not Set.",
                  grn, nrm,                                                                          /* spellbook */
                  nrm, cyn, (GET_OBJ_RNUM(obj) == NOTHING) ? -999 : get_eq_score(GET_OBJ_RNUM(obj)), /* eq rating */
                  nrm, cyn, (GET_OBJ_RNUM(obj) == NOTHING) ? "save/exit first" : buf3,               /* suggestions */
                  grn, nrm,                                                                          /* copy object */
                  grn, nrm,                                                                          /* delete object */
                  grn, nrm                                                                           /* quite */
  );
  OLC_MODE(d) = OEDIT_MAIN_MENU;
}

/* main loop (of sorts).. basically interpreter throws all input to here. */
void oedit_parse(struct descriptor_data *d, char *arg)
{
  int number, min_val = 0, i = 0, count = 0;
  long max_val = 0;
  char *oldtext = NULL;
  struct obj_data *obj;
  obj_rnum robj;
  // int this_missile = -1;

  switch (OLC_MODE(d))
  {

  case OEDIT_CONFIRM_SAVESTRING:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      oedit_save_internally(d);
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE,
             "OLC: %s edits obj %d", GET_NAME(d->character), OLC_NUM(d));
      if (CONFIG_OLC_SAVE)
      {
        oedit_save_to_disk(real_zone_by_thing(OLC_NUM(d)));
        write_to_output(d, "Object saved to disk.\r\n");
      }
      else
        write_to_output(d, "Object saved to memory.\r\n");
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'n':
    case 'N':
      /* If not saving, we must free the script_proto list. */
      OLC_OBJ(d)->proto_script = OLC_SCRIPT(d);
      free_proto_script(OLC_OBJ(d), OBJ_TRIGGER);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'a': /* abort quit */
    case 'A':
      oedit_disp_menu(d);
      return;
    default:
      write_to_output(d, "Invalid choice!\r\n");
      write_to_output(d, "Do you wish to save your changes? : \r\n");
      return;
    }

  case OEDIT_MAIN_MENU:
    /* Throw us out to whichever edit mode based on user input. */
    switch (*arg)
    {
    case 'q':
    case 'Q':
      if (STATE(d) != CON_IEDIT)
      {
        if (OLC_VAL(d))
        { /* Something has been modified. */

          write_to_output(d, "Do you wish to save this object? : ");

          OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
        }
        else

          cleanup_olc(d, CLEANUP_ALL);
      }
      else
      {
        send_to_char(d->character, "\r\nCommitting iedit changes.\r\n");

        obj = OLC_IOBJ(d);

        *obj = *(OLC_OBJ(d));

        GET_ID(obj) = max_obj_id++;

        /* find_obj helper */

        add_to_lookup_table(GET_ID(obj), (void *)obj);

        if (GET_OBJ_VNUM(obj) != NOTHING)
        {
          /* remove any old scripts */

          if (SCRIPT(obj))
          {
            extract_script(obj, OBJ_TRIGGER);

            SCRIPT(obj) = NULL;
          }

          free_proto_script(obj, OBJ_TRIGGER);

          robj = real_object(GET_OBJ_VNUM(obj));

          copy_proto_script(&obj_proto[robj], obj, OBJ_TRIGGER);

          assign_triggers(obj, OBJ_TRIGGER);
        }

        log("OLC: %s iedit a unique #%d", GET_NAME(d->character), GET_OBJ_VNUM(obj));

        if (d->character)
        {
          REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);

          STATE(d) = CON_PLAYING;

          act("$n stops using OLC.", true, d->character, 0, 0, TO_ROOM);
        }

        free(d->olc);

        d->olc = NULL;
      }
      return;
    case '1':
      write_to_output(d, "Enter keywords : ");
      OLC_MODE(d) = OEDIT_KEYWORD;
      break;
    case '2':
      write_to_output(d, "Enter short desc : ");
      OLC_MODE(d) = OEDIT_SHORTDESC;
      break;
    case '3':
      write_to_output(d, "Enter long desc :-\r\n| ");
      OLC_MODE(d) = OEDIT_LONGDESC;
      break;
    case '4':
      OLC_MODE(d) = OEDIT_ACTDESC;
      send_editor_help(d);
      write_to_output(d, "Enter action description:\r\n\r\n");
      if (OLC_OBJ(d)->action_description)
      {
        write_to_output(d, "%s", OLC_OBJ(d)->action_description);
        oldtext = strdup(OLC_OBJ(d)->action_description);
      }
      string_write(d, &OLC_OBJ(d)->action_description, MAX_MESSAGE_LENGTH, 0, oldtext);
      OLC_VAL(d) = 1;
      break;
    case '5':
      oedit_disp_type_menu(d);
      OLC_MODE(d) = OEDIT_TYPE;
      break;
      // case 'g':
      // case 'G':
      // oedit_disp_prof_menu(d);
      // OLC_MODE(d) = OEDIT_PROF;
      // break;
    case 'h':
    case 'H':
      oedit_disp_mats_menu(d);
      OLC_MODE(d) = OEDIT_MATERIAL;
      break;
    case '6':
      oedit_disp_extra_menu(d);
      OLC_MODE(d) = OEDIT_EXTRAS;
      break;
    case '7':
      oedit_disp_wear_menu(d);
      OLC_MODE(d) = OEDIT_WEAR;
      break;
    case '8':
      write_to_output(d, "Enter weight : ");
      OLC_MODE(d) = OEDIT_WEIGHT;
      break;
    case 'i':
    case 'I':
      oedit_disp_size_menu(d);
      OLC_MODE(d) = OEDIT_SIZE;
      break;
    case 'r':
    case 'R':
      oedit_disp_mob_recipient_menu(d);
      OLC_MODE(d) = OEDIT_MOB_RECIPIENT;
      break;
    case '9':
      write_to_output(d, "Enter cost : ");
      OLC_MODE(d) = OEDIT_COST;
      break;
    case 'a':
    case 'A':
      write_to_output(d, "Enter cost per day : ");
      OLC_MODE(d) = OEDIT_COSTPERDAY;
      break;
    case 'b':
    case 'B':
      write_to_output(d, "Enter timer : ");
      OLC_MODE(d) = OEDIT_TIMER;
      break;
    case 'c':
    case 'C':
      /* Clear any old values */
      GET_OBJ_VAL(OLC_OBJ(d), 0) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 1) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 2) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 3) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 4) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 5) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 6) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 7) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 8) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 9) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 10) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 11) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 12) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 13) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 14) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 15) = 0;
      OLC_VAL(d) = 1;
      oedit_disp_val1_menu(d);
      break;
    case 'd':
    case 'D':
      oedit_disp_prompt_apply_menu(d);
      break;
    case 'e':
    case 'E':
      /* If extra descriptions don't exist. */
      if (OLC_OBJ(d)->ex_description == NULL)
      {
        CREATE(OLC_OBJ(d)->ex_description, struct extra_descr_data, 1);
        OLC_OBJ(d)->ex_description->next = NULL;
      }
      OLC_DESC(d) = OLC_OBJ(d)->ex_description;
      oedit_disp_extradesc_menu(d);
      break;
    case 'm':
    case 'M':
      write_to_output(d, "Enter new minimum level: ");
      OLC_MODE(d) = OEDIT_LEVEL;
      break;
    case 'p':
    case 'P':
      oedit_disp_perm_menu(d);
      OLC_MODE(d) = OEDIT_PERM;
      break;
    case 's':
    case 'S':
      if (STATE(d) != CON_IEDIT)
      {
        OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;

        dg_script_menu(d);
      }
      else
      {
        write_to_output(d, "\r\nScripts cannot be modified on individual objects.\r\nEnter choice : ");
      }
      return;
    case 't':
    case 'T':
      oedit_disp_prompt_spellbook_menu(d);
      break;
    case 'w':
    case 'W':
      write_to_output(d, "Copy what object? ");
      OLC_MODE(d) = OEDIT_COPY;
      break;
    case 'x':
    case 'X':
      write_to_output(d, "Are you sure you want to delete this object? ");
      OLC_MODE(d) = OEDIT_DELETE;
      break;
    case 'f':
    case 'F':
      oedit_disp_weapon_spells(d);
      OLC_MODE(d) = OEDIT_WEAPON_SPELL_MENU;
      break;
    case 'j':
    case 'J':
      oedit_disp_weapon_special_abilities_menu(d);
      OLC_MODE(d) = OEDIT_WEAPON_SPECAB_MENU;
      break;
    default:
      oedit_disp_menu(d);
      break;
    }
    return; /* end of OEDIT_MAIN_MENU */

  case OLC_SCRIPT_EDIT:
    if (dg_script_edit_parse(d, arg))
      return;
    break;

  case OEDIT_KEYWORD:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_OBJ(d)->name)
      free(OLC_OBJ(d)->name);
    OLC_OBJ(d)->name = str_udup(arg);
    break;

  case OEDIT_SHORTDESC:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_OBJ(d)->short_description)
      free(OLC_OBJ(d)->short_description);
    OLC_OBJ(d)->short_description = str_udup(arg);
    break;

  case OEDIT_LONGDESC:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_OBJ(d)->description)
      free(OLC_OBJ(d)->description);
    OLC_OBJ(d)->description = str_udup(arg);
    break;

  case OEDIT_TYPE:
    number = atoi(arg);
    if ((number < 0) || (number >= NUM_ITEM_TYPES))
    {
      write_to_output(d, "Invalid choice, try again : ");
      return;
    }
    else
      GET_OBJ_TYPE(OLC_OBJ(d)) = number;
    /* what's the boundschecking worth if we don't do this ? -- Welcor */
    GET_OBJ_VAL(OLC_OBJ(d), 0) = GET_OBJ_VAL(OLC_OBJ(d), 1) =
        GET_OBJ_VAL(OLC_OBJ(d), 2) = GET_OBJ_VAL(OLC_OBJ(d), 3) = 0;
    break;

  case OEDIT_PROF:
    number = atoi(arg);
    if ((number < 0) || (number >= NUM_ITEM_PROFS))
    {
      write_to_output(d, "Invalid choice, try again : ");
      return;
    }
    else
      GET_OBJ_PROF(OLC_OBJ(d)) = number;
    break;

  case OEDIT_MATERIAL:
    number = atoi(arg);
    if ((number < 1) || (number >= NUM_MATERIALS))
    {
      write_to_output(d, "Invalid choice, try again : ");
      return;
    }
    else
      GET_OBJ_MATERIAL(OLC_OBJ(d)) = number;
    break;

  case OEDIT_EXTRAS:
    number = atoi(arg);
    if ((number < 0) || (number > NUM_ITEM_FLAGS))
    {
      oedit_disp_extra_menu(d);
      return;
    }
    else if (number == 0)
      break;
    else
    {
      TOGGLE_BIT_AR(GET_OBJ_EXTRA(OLC_OBJ(d)), (number - 1));
      oedit_disp_extra_menu(d);
      return;
    }

  case OEDIT_WEAR:
    number = atoi(arg);
    if ((number < 0) || (number > NUM_ITEM_WEARS))
    {
      write_to_output(d, "That's not a valid choice!\r\n");
      oedit_disp_wear_menu(d);
      return;
    }
    else if (number == 0) /* Quit. */
      break;
    else
    {
      TOGGLE_BIT_AR(GET_OBJ_WEAR(OLC_OBJ(d)), (number - 1));
      oedit_disp_wear_menu(d);
      return;
    }

  case OEDIT_WEIGHT:
    GET_OBJ_WEIGHT(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_WEIGHT);
    break;

  case OEDIT_SIZE:
    number = atoi(arg) - 1;
    GET_OBJ_SIZE(OLC_OBJ(d)) = LIMIT(number, 0, NUM_SIZES - 1);
    break;

  case OEDIT_MOB_RECIPIENT:
    number = atoi(arg);
    (OLC_OBJ(d)->mob_recepient) = number;
    break;

  case OEDIT_COST:
    GET_OBJ_COST(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_COST);
    break;

  case OEDIT_COSTPERDAY:
    GET_OBJ_RENT(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_RENT);
    break;

  case OEDIT_TIMER:
    GET_OBJ_TIMER(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_TIMER);
    break;

  case OEDIT_LEVEL:
    GET_OBJ_LEVEL(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, LVL_IMPL);
    break;

  case OEDIT_PERM:
    if ((number = atoi(arg)) == 0)
      break;
    if (number > 0 && number <= NUM_AFF_FLAGS)
    {
      /* Setting AFF_CHARM on objects like this is dangerous. */
      if (number != AFF_CHARM)
      {
        TOGGLE_BIT_AR(GET_OBJ_PERM(OLC_OBJ(d)), number);
      }
    }
    oedit_disp_perm_menu(d);
    return;

  case OEDIT_VALUE_1:
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d)))
    {

    case ITEM_INSTRUMENT:
      GET_OBJ_VAL(OLC_OBJ(d), 0) = MIN(MAX(atoi(arg), 0), MAX_INSTRUMENTS - 1);
      break;

    case ITEM_SWITCH:
      GET_OBJ_VAL(OLC_OBJ(d), 0) = MIN(MAX(atoi(arg), 0), 1);
      break;

    case ITEM_GEAR_OUTFIT:
      if (number < 1 || number > NUM_OUTFIT_TYPES)
        oedit_disp_val1_menu(d);
      else
      {
        GET_OBJ_VAL(OLC_OBJ(d), OUTFIT_VAL_TYPE) = number;
        oedit_disp_val2_menu(d);
      }

    case ITEM_FURNITURE:
      if (number < 0 || number > MAX_PEOPLE)
        oedit_disp_val1_menu(d);
      else
      {
        GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
        oedit_disp_val2_menu(d);
      }
      break;

      /* val[0] is AC from old system setup */
      /* ITEM_ARMOR */

    case ITEM_WEAPON:
      /* function from treasure.c */
      set_weapon_object(OLC_OBJ(d),
                        MIN(MAX(atoi(arg), 0), NUM_WEAPON_TYPES - 1));

      /*  Skip a few. */
      oedit_disp_val5_menu(d);
      return;

    case ITEM_TREASURE_CHEST:
      if (atoi(arg) <= LOOTBOX_LEVEL_UNDEFINED || atoi(arg) >= NUM_LOOTBOX_LEVELS)
      {
        write_to_output(d, "Invalid option.  Try again: ");
        return;
      }
      GET_OBJ_VAL(OLC_OBJ(d), 0) = atoi(arg);
      oedit_disp_val2_menu(d);
      return;

    case ITEM_FIREWEAPON:
      GET_OBJ_VAL(OLC_OBJ(d), 0) = MIN(MAX(atoi(arg), 0), NUM_RANGED_WEAPONS - 1);
      break;

    case ITEM_MISSILE:
      GET_OBJ_VAL(OLC_OBJ(d), 0) = LIMIT(atoi(arg), 1, NUM_AMMO_TYPES - 1);
      /* jump to break probability */
      oedit_disp_val3_menu(d);
      return;

    case ITEM_CONTAINER:
    case ITEM_AMMO_POUCH:
      GET_OBJ_VAL(OLC_OBJ(d), 0) = LIMIT(atoi(arg), -1, MAX_CONTAINER_SIZE);
      break;

      /* NewCraft */
    case ITEM_BLUEPRINT:
      GET_OBJ_VAL(OLC_OBJ(d), 0) = LIMIT(atoi(arg), 0, 1000);
      break;

      /* special values for worn gear, example monk-gloves will apply
             an enhancement bonus to damage */
    case ITEM_WORN:
      GET_OBJ_VAL(OLC_OBJ(d), 0) = LIMIT(atoi(arg), 1, 10);
      break;

    default:
      GET_OBJ_VAL(OLC_OBJ(d), 0) = atoi(arg);
    }
    /* proceed to menu 2 */
    oedit_disp_val2_menu(d);
    return;

  case OEDIT_VALUE_2:
    /* Here, I do need to check for out of range values. */
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d)))
    {
    case ITEM_GEAR_OUTFIT:
      if (number < 0 || number > 20)
        oedit_disp_val2_menu(d);
      else
      {
        GET_OBJ_VAL(OLC_OBJ(d), OUTFIT_VAL_BONUS) = number;
        oedit_disp_val3_menu(d);
      }
      break;
    case ITEM_INSTRUMENT: /* reduce difficulty */
      GET_OBJ_VAL(OLC_OBJ(d), 1) = LIMIT(number, 0, 30);
      oedit_disp_val3_menu(d);
      break;
    case ITEM_TREASURE_CHEST:
      if (atoi(arg) <= LOOTBOX_TYPE_UNDEFINED || atoi(arg) >= NUM_LOOTBOX_TYPES)
      {
        write_to_output(d, "Invalid option.  Try again: ");
        return;
      }
      GET_OBJ_VAL(OLC_OBJ(d), 1) = atoi(arg);
      oedit_disp_val3_menu(d);
      break;
    case ITEM_SCROLL:
    case ITEM_POTION:
      if (number == 0 || number == -1)
        GET_OBJ_VAL(OLC_OBJ(d), 1) = -1;
      else
        GET_OBJ_VAL(OLC_OBJ(d), 1) = LIMIT(number, 1, NUM_SPELLS);

      oedit_disp_val3_menu(d);
      break;
    case ITEM_CONTAINER:
    case ITEM_AMMO_POUCH:
      /* Needs some special handling since we are dealing with flag values here. */
      if (number < 0 || number > 4)
        oedit_disp_container_flags_menu(d);
      else if (number != 0)
      {
        TOGGLE_BIT(GET_OBJ_VAL(OLC_OBJ(d), 1), 1 << (number - 1));
        OLC_VAL(d) = 1;
        oedit_disp_val2_menu(d);
      }
      else
        oedit_disp_val3_menu(d);
      break;
    case ITEM_WEAPON:
      GET_OBJ_VAL(OLC_OBJ(d), 1) = LIMIT(number, 1, MAX_WEAPON_NDICE);
      oedit_disp_val3_menu(d);
      break;
    case ITEM_ARMOR: /* val[0] is AC from old system setup */
      /* from treasure.c - auto set some values of this item now! */
      set_armor_object(OLC_OBJ(d),
                       MIN(MAX(atoi(arg), 0), NUM_SPEC_ARMOR_TYPES - 1));

      /*  Skip to enhancement menu. */
      oedit_disp_val5_menu(d);
      return;
    case ITEM_FIREWEAPON:
      GET_OBJ_VAL(OLC_OBJ(d), 1) = LIMIT(number, 1, MAX_WEAPON_NDICE);
      oedit_disp_val3_menu(d);
      break;
    case ITEM_MISSILE:
      // GET_OBJ_VAL(OLC_OBJ(d), 1) = LIMIT(number, 1, MAX_WEAPON_SDICE);
      /*  Skip to enhancement menu. */
      // oedit_disp_val5_menu(d);
      // return;
      break;
    case ITEM_CLANARMOR:
      GET_OBJ_VAL(OLC_OBJ(d), 1) = LIMIT(number, 1, num_of_clans);
      oedit_disp_val3_menu(d);
      break;
    case ITEM_SWITCH:
      GET_OBJ_VAL(OLC_OBJ(d), 1) = LIMIT(number, 1, 999999);
      ;
      oedit_disp_val3_menu(d);
      break;
    default:
      GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
      oedit_disp_val3_menu(d);
    }
    return;

  case OEDIT_VALUE_3:
    number = atoi(arg);
    /* Quick'n'easy error checking. */
    switch (GET_OBJ_TYPE(OLC_OBJ(d)))
    {
    case ITEM_GEAR_OUTFIT:
      if (number < 1 || number > NUM_MATERIALS)
      {
        write_to_output(d, "That is not a valid selection.\r\n");
        return;
      }
      min_val = 1;
      max_val = NUM_MATERIALS - 1;
      break;
    case ITEM_TREASURE_CHEST:
      if (number < 0 || number > 1)
      {
        write_to_output(d, "Please select 0 for a regular treasure chest or 1 for a random treasure chest.\r\n");
        return;
      }
      min_val = 0;
      max_val = 1;
      break;
    case ITEM_SCROLL:
    case ITEM_POTION:
      if (number == 0 || number == -1)
      {
        GET_OBJ_VAL(OLC_OBJ(d), 2) = -1;
        oedit_disp_val4_menu(d);
        return;
      }
      min_val = 1;
      max_val = NUM_SPELLS;
      break;
    case ITEM_INSTRUMENT: /* instrument level */
      min_val = 0;
      max_val = 10;
      break;
    case ITEM_WEAPON:
      min_val = 1;
      max_val = MAX_WEAPON_SDICE;
      break;
    case ITEM_FIREWEAPON:
      min_val = 2;
      max_val = 98;
      break;
    case ITEM_MISSILE:
      /* break probability */
      min_val = 2;
      max_val = 98;
      GET_OBJ_VAL(OLC_OBJ(d), 2) = LIMIT(number, min_val, max_val);

      /* jump to enhancement bonus */
      oedit_disp_val5_menu(d);
      return;
    case ITEM_WAND:
    case ITEM_STAFF:
      min_val = 0;
      max_val = 20;
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      min_val = 0;
      max_val = NUM_LIQ_TYPES - 1;
      number--;
      break;
    case ITEM_KEY:
      min_val = 0;
      max_val = 400000000;
      break;
    case ITEM_PORTAL:
      min_val = 1;
      max_val = 400000000;
      break;
    case ITEM_SWITCH:
      min_val = 0;
      max_val = 5;
      break;
    default:
      min_val = -200000000;
      max_val = 200000000;
    }
    GET_OBJ_VAL(OLC_OBJ(d), 2) = LIMIT(number, min_val, max_val);
    oedit_disp_val4_menu(d);
    return;

  case OEDIT_VALUE_4:
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d)))
    {
    case ITEM_TREASURE_CHEST:
      if (number < 0 || number > 80)
      {
        write_to_output(d, "Please select 0 if the chest is not hidden, otherwise enter the search dc (max 80).\r\n");
        return;
      }
      min_val = 0;
      max_val = 80;
      break;
    case ITEM_GEAR_OUTFIT:
      if (number == APPLY_SKILL || number == APPLY_FEAT)
      {
        write_to_output(d, "You cannot use those apply types on outfit items.\r\n");
        return;
      }
      min_val = 0;
      max_val = NUM_APPLIES;
      number--;
      break;
    case ITEM_SCROLL:
    case ITEM_POTION:
      if (number == 0 || number == -1)
      {
        GET_OBJ_VAL(OLC_OBJ(d), 3) = -1;
        oedit_disp_menu(d);
        return;
      }
      min_val = 1;
      max_val = NUM_SPELLS;
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      min_val = 1;
      max_val = NUM_SPELLS;
      break;
    case ITEM_INSTRUMENT:
      /* breakability: 0 = indestructable, 2000 = break on first use
       * recommended values are 0-30 */
      min_val = 0;
      max_val = 2000;
      break;
    case ITEM_WEAPON:
      min_val = 0;
      max_val = NUM_ATTACK_TYPES - 1;
      break;
    case ITEM_FIREWEAPON:
      min_val = 0;
      max_val = NUM_ATTACK_TYPES - 1;
      break;
    case ITEM_SWITCH:
      GET_OBJ_VAL(OLC_OBJ(d), 3) = LIMIT(number, 0, 2);
      oedit_disp_menu(d);
      return;
      /*
        case ITEM_MISSILE:
          min_val = 0;
          max_val = NUM_ATTACK_TYPES - 1;
          break;*/
    default:
      min_val = -65000;
      max_val = 65000;
      break;
    }
    GET_OBJ_VAL(OLC_OBJ(d), 3) = LIMIT(number, min_val, max_val);
    oedit_disp_val5_menu(d);
    return;

    /*this is enhancement bonus so far*/
  case OEDIT_VALUE_5:
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d)))
    {
    case ITEM_TREASURE_CHEST:
      if (number < 0 || number > 80)
      {
        write_to_output(d, "Please select 0 if the chest is not locked, otherwise enter the pick lock dc (max 80).\r\n");
        return;
      }
      min_val = 0;
      max_val = 80;
      GET_OBJ_VAL(OLC_OBJ(d), 4) = LIMIT(number, min_val, max_val);
      oedit_disp_val6_menu(d);
      return;
    case ITEM_GEAR_OUTFIT:
      GET_OBJ_VAL(OLC_OBJ(d), OUTFIT_VAL_APPLY_MOD) = number;
      oedit_disp_val6_menu(d);
      return;
    case ITEM_MISSILE:
      min_val = 0;
      max_val = 10;
      break;
    case ITEM_WEAPON:
      min_val = 0;
      max_val = 10;
      break;
    case ITEM_ARMOR:
    case ITEM_CLANARMOR:
      min_val = 0;
      max_val = 10;
      break;
    default:
      min_val = -65000;
      max_val = 65000;
      break;
    }
    GET_OBJ_VAL(OLC_OBJ(d), 4) = LIMIT(number, min_val, max_val);
    break;

  case OEDIT_VALUE_6:
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d)))
    {
    case ITEM_TREASURE_CHEST:
      if (number < 0 || number > MAX_TRAP_EFFECTS)
      {
        write_to_output(d, "Please select 0 if the chest has no trap, otherwise enter the trap type desired.\r\n");
        return;
      }
      min_val = 0;
      max_val = MAX_TRAP_EFFECTS;
      break;
    case ITEM_GEAR_OUTFIT:
      min_val = 0;
      max_val = NUM_BONUS_TYPES - 1;
      break;
    default:
      min_val = -65000;
      max_val = 65000;
      break;
    }
    GET_OBJ_VAL(OLC_OBJ(d), 5) = LIMIT(number, min_val, max_val);
    break;

    //    }
    //  }

  case OEDIT_PROMPT_APPLY:
    if ((number = atoi(arg)) == 0)
      break;
    else if (number < 0 || number > MAX_OBJ_AFFECT)
    {
      oedit_disp_prompt_apply_menu(d);
      return;
    }
    OLC_VAL(d) = number - 1;
    OLC_MODE(d) = OEDIT_APPLY;
    oedit_disp_apply_menu(d);
    return;

  case OEDIT_APPLY:
    if (((number = atoi(arg)) == 0) || ((number = atoi(arg)) == 1))
    {
      OLC_OBJ(d)->affected[OLC_VAL(d)].location = 0;
      OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = 0;
      oedit_disp_prompt_apply_menu(d);
    }
    else if (number < 0 || number > NUM_APPLIES)
      oedit_disp_apply_menu(d);
    else
    {
      int counter;

      /* add in check here if already applied.. deny builders another */
      if (GET_LEVEL(d->character) < LVL_IMPL)
      {
        for (counter = 0; counter < MAX_OBJ_AFFECT; counter++)
        {
          if (OLC_OBJ(d)->affected[counter].location == number)
          {
            write_to_output(d, "Object already has that apply.");
            return;
          }
        }
      }

      OLC_OBJ(d)->affected[OLC_VAL(d)].location = number - 1;
      if ((number - 1) == APPLY_FEAT)
      {
        write_to_output(d, "Select Feat : \r\n");
        for (i = 1; i < FEAT_LAST_FEAT; i++)
        {
          if (valid_item_feat(i))
          {
            count++;
            send_to_char(d->character, "%3d) %-30s ", i, feat_list[i].name);
            if ((count % 3) == 2)
              send_to_char(d->character, "\r\n");
          }
        }
        send_to_char(d->character, "\r\n");
      }
      else
      {
        write_to_output(d, "Modifier : ");
      }
      OLC_MODE(d) = OEDIT_APPLYMOD;
    }
    return;

  case OEDIT_APPLYMOD:
    if (OLC_OBJ(d)->affected[OLC_VAL(d)].location == APPLY_FEAT)
    {
      if (!valid_item_feat(atoi(arg)))
      {
        send_to_char(d->character, "You can't assign that feat to an item.\r\n");
        return;
      }
    }
    OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = atoi(arg);
    oedit_disp_apply_prompt_bonus_type_menu(d);
    return;

  case OEDIT_APPLYSPEC:
    if (isdigit(*arg))
      OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = atoi(arg);
    else
      switch (OLC_OBJ(d)->affected[OLC_VAL(d)].location)
      {
        /*
                      case APPLY_SKILL:
                        number = find_skill_num(arg, SKTYPE_SKILL);
                        if (number > -1)
                          OLC_OBJ(d)->affected[OLC_VAL(d)].specific = number;
                        break;
             */
      case APPLY_FEAT:
        number = find_feat_num(arg);
        if (number > -1)
          OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = number;
        break;
      default:
        OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = 0;
        break;
      }
    oedit_disp_apply_prompt_bonus_type_menu(d);
    return;

  case OEDIT_APPLY_BONUS_TYPE:
    number = atoi(arg);
    if (number < 0 || number > NUM_BONUS_TYPES)
    {
      write_to_output(d, "Invalid bonus type, please enter a valid bonus type.");
      oedit_disp_apply_prompt_bonus_type_menu(d);
      return;
    }
    OLC_OBJ(d)->affected[OLC_VAL(d)].bonus_type = atoi(arg);
    if (OLC_OBJ(d)->affected[OLC_VAL(d)].location == APPLY_SKILL)
    {
      write_to_output(d, "\r\nSelect which skill to affect:\r\n\r\n");
      for (i = START_GENERAL_ABILITIES; i <= END_CRAFT_ABILITIES; i++)
      {
        write_to_output(d, "%2d) %-21s ", i, ability_names[i]);
        if ((i % 3) == 0)
          write_to_output(d, "\r\n");
      }
      write_to_output(d, "\r\n");
      write_to_output(d, "Skill: ");
      OLC_MODE(d) = OEDIT_APPLY_SPECIFIC;
    }
    else
    {
      oedit_disp_prompt_apply_menu(d);
    }
    return;
  case OEDIT_APPLY_SPECIFIC:
    number = atoi(arg);
    if (number < START_GENERAL_ABILITIES || number > END_CRAFT_ABILITIES)
    {
      write_to_output(d, "That is not a valid skill.\r\n");
      return;
    }
    OLC_OBJ(d)->affected[OLC_VAL(d)].specific = number;
    oedit_disp_prompt_apply_menu(d);
    return;
  case OEDIT_EXTRADESC_KEY:
    if (genolc_checkstring(d, arg))
    {
      if (OLC_DESC(d)->keyword)
        free(OLC_DESC(d)->keyword);
      OLC_DESC(d)->keyword = str_udup(arg);
    }
    oedit_disp_extradesc_menu(d);
    return;

  case OEDIT_EXTRADESC_MENU:
    switch ((number = atoi(arg)))
    {
    case 0:
      if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description)
      {
        struct extra_descr_data *temp;

        if (OLC_DESC(d)->keyword)
          free(OLC_DESC(d)->keyword);
        if (OLC_DESC(d)->description)
          free(OLC_DESC(d)->description);

        /* Clean up pointers */
        REMOVE_FROM_LIST(OLC_DESC(d), OLC_OBJ(d)->ex_description, next);
        free(OLC_DESC(d));
        OLC_DESC(d) = NULL;
      }
      break;

    case 1:
      OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
      write_to_output(d, "Enter keywords, separated by spaces :-\r\n| ");
      return;

    case 2:
      OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
      send_editor_help(d);
      write_to_output(d, "Enter the extra description:\r\n\r\n");
      if (OLC_DESC(d)->description)
      {
        write_to_output(d, "%s", OLC_DESC(d)->description);
        oldtext = strdup(OLC_DESC(d)->description);
      }
      string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, oldtext);
      OLC_VAL(d) = 1;
      return;

    case 3:
      /* Only go to the next description if this one is finished. */
      if (OLC_DESC(d)->keyword && OLC_DESC(d)->description)
      {
        struct extra_descr_data *new_extra;

        if (OLC_DESC(d)->next)
          OLC_DESC(d) = OLC_DESC(d)->next;
        else
        { /* Make new extra description and attach at end. */
          CREATE(new_extra, struct extra_descr_data, 1);
          OLC_DESC(d)->next = new_extra;
          OLC_DESC(d) = OLC_DESC(d)->next;
        }
      }
      /* No break - drop into default case. */
    default:
      oedit_disp_extradesc_menu(d);
      return;
    }
    break;

  case OEDIT_COPY:
    if ((number = real_object(atoi(arg))) != NOTHING)
    {
      oedit_setup_existing(d, number, QMODE_QCOPY);
    }
    else
      write_to_output(d, "That object does not exist.\r\n");
    break;

  case OEDIT_DELETE:
    if (*arg == 'y' || *arg == 'Y')
    {
      if (delete_object(GET_OBJ_RNUM(OLC_OBJ(d))) != NOTHING)
        write_to_output(d, "Object deleted.\r\n");
      else
        write_to_output(d, "Couldn't delete the object!\r\n");

      cleanup_olc(d, CLEANUP_ALL);
    }
    else if (*arg == 'n' || *arg == 'N')
    {
      oedit_disp_menu(d);
      OLC_MODE(d) = OEDIT_MAIN_MENU;
    }
    else
      write_to_output(d, "Please answer 'Y' or 'N': ");
    return;

  case OEDIT_WEAPON_SPELL_MENU:
    if ((number = atoi(arg)) == -1)
      break;
    else if (number < 1 || number > MAX_WEAPON_SPELLS)
    {
      OLC_MODE(d) = OEDIT_MAIN_MENU;
      OLC_VAL(d) = 1;
      oedit_disp_menu(d);
      return;
    }
    OLC_VAL(d) = number - 1;
    OLC_MODE(d) = OEDIT_WEAPON_SPELLS;
    oedit_disp_spells_menu(d);
    return;

  case OEDIT_WEAPON_SPELLS:
    if ((number = atoi(arg)) == -1)
      break;
    else if (number < -1 || number > MAX_SPELLS)
    {
      oedit_disp_spells_menu(d);
      return;
    }
    OLC_OBJ(d)->wpn_spells[OLC_VAL(d)].spellnum = number;
    OLC_MODE(d) = OEDIT_WEAPON_SPELL_LEVEL;
    send_to_char(d->character, "At what level should it be cast: ");
    return;

  case OEDIT_WEAPON_SPELL_LEVEL:
    if ((number = atoi(arg)) == -1)
      break;
    if (number < 1)
    {
      send_to_char(d->character, "Invalid level.\r\n");
      send_to_char(d->character, "What level should the spell be cast at: ");
      return;
    }
    OLC_OBJ(d)->wpn_spells[OLC_VAL(d)].level = number;
    send_to_char(d->character, "What percent of rounds should it go off: ");
    OLC_MODE(d) = OEDIT_WEAPON_SPELL_PERCENT;
    return;

  case OEDIT_WEAPON_SPELL_PERCENT:
    if ((number = atoi(arg)) == -1)
      break;
    if (number < 1 || number > 50)
    {
      send_to_char(d->character, "Invalid percent, must be 1-50.\r\nPlease enter the percent: ");
      return;
    }
    OLC_OBJ(d)->wpn_spells[OLC_VAL(d)].percent = number;
    OLC_OBJ(d)->has_spells = TRUE;
    send_to_char(d->character, "1 for offensive, 0 for defensive spell: ");
    OLC_MODE(d) = OEDIT_WEAPON_SPELL_INCOMBAT;
    return;

  case OEDIT_WEAPON_SPELL_INCOMBAT:
    if ((number = atoi(arg)) == -1)
      break;
    if (number != 1 && number != 0)
    {
      send_to_char(d->character, "Invalid value!\r\n");
      send_to_char(d->character, "1 = on = Spell will cast in combat exclusively (offensive)\r\n");
      send_to_char(d->character, "0 = off = Spell will cast randomly (defensive)\r\n");
      return;
    }
    OLC_OBJ(d)->wpn_spells[OLC_VAL(d)].inCombat = number;

    /* Got the last of it, now go back in case of more */
    OLC_MODE(d) = OEDIT_WEAPON_SPELL_MENU;
    oedit_disp_weapon_spells(d);
    return;

  case OEDIT_PROMPT_SPELLBOOK:
    if ((number = atoi(arg)) == 0)
      break;
    else if (number < 0 || number > SPELLBOOK_SIZE)
    {
      oedit_disp_prompt_spellbook_menu(d);
      return;
    }
    int counter;

    if (!OLC_OBJ(d)->sbinfo)
    {
      CREATE(OLC_OBJ(d)->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
      memset((char *)OLC_OBJ(d)->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
    }

    /* look for empty spot in book */
    for (counter = 0; counter < SPELLBOOK_SIZE; counter++)
      if (OLC_OBJ(d)->sbinfo && OLC_OBJ(d)->sbinfo[counter].spellname == 0)
        break;

    /* oops no space */
    if (OLC_OBJ(d)->sbinfo && counter == SPELLBOOK_SIZE)
    {
      write_to_output(d, "This spellbook is full!\r\n");
      return;
    }

    OLC_VAL(d) = counter;
    OLC_MODE(d) = OEDIT_SPELLBOOK;
    oedit_disp_spellbook_menu(d);
    return;

  case OEDIT_SPELLBOOK:
    if ((number = atoi(arg)) == 0)
    {
      if (OLC_OBJ(d)->sbinfo)
      {
        OLC_OBJ(d)->sbinfo[OLC_VAL(d)].spellname = 0;
        OLC_OBJ(d)->sbinfo[OLC_VAL(d)].pages = 0;
      }
      else
      {
        CREATE(OLC_OBJ(d)->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
        memset((char *)OLC_OBJ(d)->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
        OLC_OBJ(d)->sbinfo[OLC_VAL(d)].spellname = 0;
        OLC_OBJ(d)->sbinfo[OLC_VAL(d)].pages = 0;
      }
      oedit_disp_prompt_spellbook_menu(d);
    }
    else if (number < 0 || number >= NUM_SPELLS)
    {
      oedit_disp_spellbook_menu(d);
    }
    else
    {
      int counter;

      /* add in check here if already applied.. deny builders another */
      for (counter = 0; counter < SPELLBOOK_SIZE; counter++)
      {
        if (OLC_OBJ(d)->sbinfo && OLC_OBJ(d)->sbinfo[counter].spellname == number)
        {
          write_to_output(d, "Object already has that spell.");
          return;
        }
      }

      if (!OLC_OBJ(d)->sbinfo)
      {
        CREATE(OLC_OBJ(d)->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
        memset((char *)OLC_OBJ(d)->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
      }

      OLC_OBJ(d)->sbinfo[OLC_VAL(d)].spellname = number;
      OLC_OBJ(d)->sbinfo[OLC_VAL(d)].pages = MAX(1, lowest_spell_level(number) / 2);
      ;
      oedit_disp_prompt_spellbook_menu(d);
    }
    return;
  case OEDIT_WEAPON_SPECAB_MENU:
    switch (*arg)
    {
    case 'N':
    case 'n':
      /* Create a new special ability and assign it to the list. */
      CREATE(OLC_SPECAB(d), struct obj_special_ability, 1);
      OLC_SPECAB(d)->next = OLC_OBJ(d)->special_abilities;
      OLC_OBJ(d)->special_abilities = OLC_SPECAB(d);

      OLC_MODE(d) = OEDIT_ASSIGN_WEAPON_SPECAB_MENU;
      OLC_VAL(d) = 1;
      oedit_disp_assign_weapon_specab_menu(d);
      break;
    case 'E':
    case 'e':
      write_to_output(d, "Edit which ability? : ");
      OLC_MODE(d) = OEDIT_EDIT_WEAPON_SPECAB;
      OLC_VAL(d) = 1;
      break;
    case 'D':
    case 'd':
      write_to_output(d, "Delete which ability? (-1 to cancel) : ");
      OLC_MODE(d) = OEDIT_DELETE_WEAPON_SPECAB;
      break;
    case 'Q':
    case 'q':
      OLC_MODE(d) = OEDIT_MAIN_MENU;
      oedit_disp_menu(d);
      break;
    default:
      write_to_output(d, "Invalid choice, try again : \r\n");
      break;
    }
    return;
  case OEDIT_EDIT_WEAPON_SPECAB:
    /* Editing is the same as assign - just load the chosen specab. */
    number = atoi(arg);
    OLC_SPECAB(d) = get_specab_by_position(OLC_OBJ(d), number);

    if (OLC_SPECAB(d) == NULL)
    {
      write_to_output(d, "Invalid special ability number. \r\n");
      OLC_MODE(d) = OEDIT_WEAPON_SPECAB_MENU;
      oedit_disp_weapon_special_abilities_menu(d);
    }
    else
    {
      OLC_MODE(d) = OEDIT_ASSIGN_WEAPON_SPECAB_MENU;
      oedit_disp_assign_weapon_specab_menu(d);
    }
    return;
  case OEDIT_ASSIGN_WEAPON_SPECAB_MENU:
    switch (*arg)
    {
    case 'A':
    case 'a':
      /* Choose ability. */
      oedit_weapon_specab(d);
      break;
    case 'L':
    case 'l':
      /* Set ability level */
      write_to_output(d, "Enter special ability level (1-34) : ");
      OLC_MODE(d) = OEDIT_WEAPON_SPECAB_LEVEL;
      OLC_VAL(d) = 1;
      break;
    case 'M':
    case 'm':
      /* Set activation methods */
      oedit_disp_specab_activation_method_menu(d);
      OLC_MODE(d) = OEDIT_WEAPON_SPECAB_ACTMTD;
      OLC_VAL(d) = 1;
      break;
    case 'C':
    case 'c':
      /* Set command word */
      write_to_output(d, "Enter command word : ");
      OLC_MODE(d) = OEDIT_WEAPON_SPECAB_CMDWD;
      OLC_VAL(d) = 1;
      break;
    case 'V':
    case 'v':
      /* Go into value setting questions */
      oedit_disp_specab_val1_menu(d);
      OLC_VAL(d) = 1;
      break;
    case 'Q':
    case 'q':
      OLC_MODE(d) = OEDIT_WEAPON_SPECAB_MENU;
      oedit_disp_weapon_special_abilities_menu(d);
      break;
    default:
      write_to_output(d, "Invalid choice, try again : \r\n");
      break;
    }
    return;
  case OEDIT_DELETE_WEAPON_SPECAB:
    if ((number = atoi(arg)) == -1)
    {
      oedit_disp_weapon_special_abilities_menu(d);
      OLC_MODE(d) = OEDIT_WEAPON_SPECAB_MENU;
      OLC_VAL(d) = 1;
      return;
    }
    OLC_SPECAB(d) = NULL;

    if (remove_special_ability(OLC_OBJ(d), number))
      write_to_output(d, "Ability deleted.\r\n");
    else
      write_to_output(d, "That ability does not exist!\r\n");

    oedit_disp_weapon_special_abilities_menu(d);
    OLC_MODE(d) = OEDIT_WEAPON_SPECAB_MENU;
    return;
  case OEDIT_WEAPON_SPECAB:
    /* The user has chosen a special ability for this weapon. */
    number = atoi(arg); /* No need to decrement number, we adjusted it already. */
    if ((number < 0) || (number >= NUM_SPECABS))
    {
      write_to_output(d, "Invalid choice, try again : ");
      return;
    }

    OLC_SPECAB(d)->ability = number;
    OLC_SPECAB(d)->level = special_ability_info[number].level;
    OLC_SPECAB(d)->activation_method = special_ability_info[number].activation_method;

    OLC_MODE(d) = OEDIT_ASSIGN_WEAPON_SPECAB_MENU;
    oedit_disp_assign_weapon_specab_menu(d);
    return;
  case OEDIT_WEAPON_SPECAB_LEVEL:
    number = atoi(arg);
    if ((number < 1) || (number > 34))
    {
      write_to_output(d, "Invalid level, try again : ");
      return;
    }

    OLC_SPECAB(d)->level = number;

    oedit_disp_weapon_special_abilities_menu(d);
    OLC_MODE(d) = OEDIT_WEAPON_SPECAB_MENU;

    return;
  case OEDIT_WEAPON_SPECAB_CMDWD:
    OLC_SPECAB(d)->command_word = strdup(arg);
    OLC_MODE(d) = OEDIT_ASSIGN_WEAPON_SPECAB_MENU;
    oedit_disp_assign_weapon_specab_menu(d);
    return;
  case OEDIT_WEAPON_SPECAB_ACTMTD:
    number = atoi(arg);
    if ((number < 0) || (number > NUM_ACTIVATION_METHODS))
    { // added -3 to prevent eyes, ears, badge
      write_to_output(d, "That's not a valid choice!\r\n");
      oedit_disp_specab_activation_method_menu(d);
      return;
    }
    else if (number == 0)
    {
      /* Quit. */
      OLC_MODE(d) = OEDIT_ASSIGN_WEAPON_SPECAB_MENU;
      oedit_disp_assign_weapon_specab_menu(d);
      return;
    }
    else
    {
      TOGGLE_BIT(OLC_SPECAB(d)->activation_method, (1 << (number - 1)));
      oedit_disp_specab_activation_method_menu(d);
      return;
    }
  case OEDIT_SPECAB_VALUE_1:
    switch (OLC_SPECAB(d)->ability)
    {
    case WEAPON_SPECAB_BANE: /* Val 1: NPC RACE */
      number = atoi(arg);
      if ((number < 0) || (number >= NUM_RACE_TYPES))
      {
        /* Value out of range. */
        write_to_output(d, "Invalid choice, try again : ");
        return;
      }
      OLC_SPECAB(d)->value[0] = number;
      OLC_MODE(d) = OEDIT_SPECAB_VALUE_2;
      oedit_disp_specab_val2_menu(d);
      return;
    case ITEM_SPECAB_HORN_OF_SUMMONING: /* Val 1: VNUM of mob summoned. */
      number = atoi(arg);
      if ((number < 0) || (number >= 12157521))
      {
        /* Value out of range. */
        write_to_output(d, "Invalid vnum, try again : ");
        return;
      }
      OLC_SPECAB(d)->value[0] = number;
      OLC_MODE(d) = OEDIT_WEAPON_SPECAB_MENU;
      oedit_disp_assign_weapon_specab_menu(d);
      return;
    case ITEM_SPECAB_ITEM_SUMMON: /* Val 1: VNUM of mob summoned. */
      number = atoi(arg);
      if ((number < 0) || (number >= 12157521))
      {
        /* Value out of range. */
        write_to_output(d, "Invalid vnum, try again : ");
        return;
      }
      OLC_SPECAB(d)->value[0] = number;
      OLC_MODE(d) = OEDIT_WEAPON_SPECAB_MENU;
      oedit_disp_assign_weapon_specab_menu(d);
      return;
    case WEAPON_SPECAB_SPELL_STORING: /* Val 1: SPELL NUMBER */
        ;
    default:;
    }
  case OEDIT_SPECAB_VALUE_2:
    switch (OLC_SPECAB(d)->ability)
    {
    case WEAPON_SPECAB_BANE: /* Val 2: NPC SUBRACE */
      number = atoi(arg);
      if ((number < 0) || (number >= NUM_SUB_RACES))
      {
        /* Value out of range. */
        write_to_output(d, "Invalid choice, try again : ");
        return;
      }
      OLC_SPECAB(d)->value[1] = number;
      /* Finished. */
      OLC_MODE(d) = OEDIT_ASSIGN_WEAPON_SPECAB_MENU;
      oedit_disp_assign_weapon_specab_menu(d);

      return;
    case WEAPON_SPECAB_SPELL_STORING: /* Val 2: SPELL LEVEL */
        ;
    default:;
    }

  case OEDIT_SPECAB_VALUE_3:
    switch (OLC_SPECAB(d)->ability)
    {
    default:;
    }

  case OEDIT_SPECAB_VALUE_4:
    switch (OLC_SPECAB(d)->ability)
    {
    default:;
    }

  default:
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Reached default case in oedit_parse()!");
    write_to_output(d, "Oops...\r\n");
    break;
  }

  /* If we get here, we have changed something. */
  OLC_VAL(d) = 1;
  oedit_disp_menu(d);
}

void oedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d))
  {
  case OEDIT_ACTDESC:
    oedit_disp_menu(d);
    break;
  case OEDIT_EXTRADESC_DESCRIPTION:
    oedit_disp_extradesc_menu(d);
    break;
  }
}

/* this is all iedit stuff */

void iedit_setup_existing(struct descriptor_data *d,
                          struct obj_data *real_num)

{
  struct obj_data *obj;

  OLC_IOBJ(d) = real_num;

  obj = create_obj();

  copy_object(obj, real_num);

  /* free any assigned scripts */

  if (SCRIPT(obj))

    extract_script(obj, OBJ_TRIGGER);

  SCRIPT(obj) = NULL;

  /* find_obj helper */

  remove_from_lookup_table(GET_ID(obj));

  OLC_OBJ(d) = obj;

  OLC_IOBJ(d) = real_num;

  OLC_VAL(d) = 0;

  oedit_disp_menu(d);
}

ACMD(do_iedit)
{
  struct obj_data *k;
  int found = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  one_argument(argument, arg, sizeof(arg));

  if (!*arg || !*argument)
  {
    send_to_char(ch, "You must supply an object name.\r\n");
  }

  if ((k = get_obj_in_equip_vis(ch, arg, NULL, ch->equipment)))
  {
    found = 1;
  }
  else if ((k = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
  {
    found = 1;
  }
  else if ((k = get_obj_in_list_vis(ch, arg, NULL,
                                    world[IN_ROOM(ch)].contents)))
  {
    found = 1;
  }
  else if ((k = get_obj_vis(ch, arg, NULL)))
  {
    found = 1;
  }

  if (!found)
  {
    send_to_char(ch, "Couldn't find that object. Sorry.\r\n");

    return;
  }

  /* set up here */

  CREATE(OLC(ch->desc), struct oasis_olc_data, 1);

  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  iedit_setup_existing(ch->desc, k);

  OLC_VAL(ch->desc) = 0;

  act("$n starts using OLC.", true, ch, 0, 0, TO_ROOM);

  STATE(ch->desc) = CON_IEDIT;

  return;
}
