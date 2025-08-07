/**************************************************************************
 *  File: act.informative.c                            Part of LuminariMUD *
 *  Usage: Player-level commands of an informative nature.                 *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include <time.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "pfdefaults.h"
#include "constants.h"
#include "dg_scripts.h"
#include "mud_event.h"
#include "mail.h" /**< For the has_mail function */
#include "act.h"
#include "class.h"
#include "race.h"
#include "fight.h"
#include "modify.h"
#include "asciimap.h"
#include "spells.h"
#include "clan.h"
#include "craft.h" // auto crafting quest
#include "wilderness.h"
#include "quest.h" /* so you can identify questmaster mobiles */
#include "feats.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "desc_engine.h"
#include "crafts.h"
#include "alchemy.h"
#include "premadebuilds.h"
#include "staff_events.h"
#include "missions.h"
#include "spec_procs.h"
#include "transport.h"
#include "encounters.h"
#include "deities.h"
#include "treasure.h"
#include "roleplay.h"
#include "spell_prep.h"
#include "boards.h"
#include "perfmon.h"

/* prototypes of local functions */
/* do_diagnose utility functions */
static void diag_char_to_char(struct char_data *i, struct char_data *ch);
/* do_look and do_examine utility functions */
static void do_auto_exits(struct char_data *ch);
static void list_char_to_char(struct char_data *list, struct char_data *ch);
static void list_one_char(struct char_data *i, struct char_data *ch);
static void look_at_char(struct char_data *i, struct char_data *ch);
static void look_at_target(struct char_data *ch, char *arg);
static void look_in_direction(struct char_data *ch, int dir);
static void look_in_obj(struct char_data *ch, char *arg);
/* do_look, do_equipment, do_examine, do_inventory */
static void show_obj_modifiers(struct obj_data *obj, struct char_data *ch);
/* do_where utility functions */
static void perform_immort_where(struct char_data *ch, char *arg);
static void perform_mortal_where(struct char_data *ch, char *arg);
static void print_object_location(int num, struct obj_data *obj, struct char_data *ch, int recur);

/* globals */
int boot_high = 0;

/* file level defines */
/* weapon types */
#define WPT_SIMPLE 1
#define WPT_MARTIAL 2
#define WPT_EXOTIC 3
#define WPT_MONK 4
#define WPT_DRUID 5
#define WPT_BARD 6
#define WPT_ROGUE 7
#define WPT_WIZARD 8
#define WPT_DROW 9
#define WPT_ELF 10
#define WPT_DWARF 11
#define WPT_DUERGAR 11
#define WPT_PSIONICIST 12
#define WPT_SHADOWDANCER 13
#define WPT_ASSASSIN 14

const int eq_ordering_1[NUM_WEARS] = {
    WEAR_LIGHT,         //<used as light>
    WEAR_BADGE,         //<worn as a badge>
    WEAR_HEAD,          //<worn on head>
    WEAR_EYES,          //<worn on eyes>
    WEAR_EAR_R,         //<worn in ear>
    WEAR_EAR_L,         //<worn in ear>
    WEAR_FACE,          //<worn on face>
    WEAR_NECK_1,        //<worn around neck>
    WEAR_NECK_2,        //<worn around neck>
    WEAR_SHOULDERS,     //<worn on shoulders>
    WEAR_BODY,          //<worn on body>
    WEAR_ABOUT,         //<worn about body>
    WEAR_AMMO_POUCH,    //<worn as ammo pouch>
    WEAR_WAIST,         //<worn about waist>
    WEAR_SHEATH,        //<worn as sheath>
    WEAR_ARMS,          //<worn on arms>
    WEAR_WRIST_R,       //<worn around wrist>
    WEAR_WRIST_L,       //<worn around wrist>
    WEAR_HANDS,         //<worn on hands>
    WEAR_FINGER_R,      //<worn on finger>
    WEAR_FINGER_L,      //<worn on finger>
    WEAR_WIELD_1,       //<wielding/held slots>
    WEAR_HOLD_1,        //<wielding/held slots>
    WEAR_WIELD_OFFHAND, //<wielding/held slots>
    WEAR_HOLD_2,        //<wielding/held slots>
    WEAR_WIELD_2H,      //<wielding/held slots>
    WEAR_HOLD_2H,       //<wielding/held slots>
    WEAR_INSTRUMENT,    //<worn as instrument>
    WEAR_SHIELD,        //<worn as shield>
    WEAR_LEGS,          //<worn on legs>
    WEAR_ANKLE_R,       //<worn on ankle>
    WEAR_ANKLE_L,       //<worn on ankle>
    WEAR_FEET,          //<worn on feet>
};

/*******  UTILITY FUNCTIONS ***********/

/* function to display some basic info about a mobile that is 'identified' or
 victim of 'lore' */
void lore_id_vict(struct char_data *ch, struct char_data *tch)
{
  int i = 0;
  size_t len = 0;
  int count = 0, dcount = 0;
  bool has_subrace = false;
  char subraces[MEDIUM_STRING] = {'\0'};

  if (IS_NPC(tch))
  {
    if (HAS_FEAT(ch, FEAT_BG_SAGE))
      GET_SAGE_MOB_VNUM(ch) = GET_MOB_VNUM(tch);
  }

  count = snprintf(subraces + len, sizeof(subraces) - len, ", Subrace(s): ");
  if (count > 0)
    len += count;
  if (GET_SUBRACE(tch, 0))
  {
    count = snprintf(subraces + len, sizeof(subraces) - len, "%s", npc_subrace_types[GET_SUBRACE(tch, 0)]);
    if (count > 0)
      len += count;
    has_subrace = true;
  }
  if (GET_SUBRACE(tch, 1))
  {
    count = snprintf(subraces + len, sizeof(subraces) - len, "/%s", npc_subrace_types[GET_SUBRACE(tch, 1)]);
    if (count > 0)
      len += count;
  }
  if (GET_SUBRACE(tch, 2))
  {
    count = snprintf(subraces + len, sizeof(subraces) - len, "/%s", npc_subrace_types[GET_SUBRACE(tch, 2)]);
    if (count > 0)
      len += count;
  }

  send_to_char(ch, "Name: %s\r\n", GET_NAME(tch));
  if (!IS_NPC(tch))
    send_to_char(ch, "%s is %d years, %d months, %d days and %d hours old.\r\n",
                 GET_NAME(tch), age(tch)->year, age(tch)->month,
                 age(tch)->day, age(tch)->hours);
  send_to_char(ch, "Race: %s%s.\r\n", !IS_NPC(tch) ? CAP(race_list[GET_RACE(tch)].name) : race_family_types[GET_RACE(tch)],
               has_subrace ? subraces : "");
  if (!AFF_FLAGGED(tch, AFF_HIDE_ALIGNMENT))
    send_to_char(ch, "Alignment: %s.\r\n", get_align_by_num(GET_ALIGNMENT(tch)));
  if (IS_NPC(tch))
    send_to_char(ch, "Class: %s.\r\n", class_list[GET_CLASS(tch)].name);
  send_to_char(ch, "Level: %d, Hits: %d/%d, PSP: %d\r\n", GET_LEVEL(tch),
               GET_HIT(tch), GET_MAX_HIT(tch), GET_PSP(tch));
  send_to_char(ch, "AC: %d, Hitroll: %d, Damroll: %d\r\n",
               compute_armor_class(NULL, tch, FALSE, MODE_ARMOR_CLASS_NORMAL),
               GET_HITROLL(tch), GET_DAMROLL(tch));
  if (IS_NPC(tch))
    send_to_char(ch, "Will: %d, Fort: %d, Refl: %d\r\n",
                 compute_mag_saves(tch, SAVING_WILL, 0), compute_mag_saves(tch, SAVING_FORT, 0), compute_mag_saves(tch, SAVING_REFL, 0));
  send_to_char(ch, "Str: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
               GET_STR(tch), GET_ADD(tch), GET_INT(tch),
               GET_WIS(tch), GET_DEX(tch), GET_CON(tch), GET_CHA(tch));
  text_line(ch, "\tYDamage Type Resistance / Vulnerability\tC", 80, '-', '-');
  for (i = 0; i < NUM_DAM_TYPES - 1; i++)
  {
    if (can_dam_be_resisted(i+1))
    {
      send_to_char(ch, "     %-15s: %-4d%% (%-2d)         ", damtype_display[i + 1],
                   compute_damtype_reduction(tch, i + 1), compute_energy_absorb(tch, i + 1));
      dcount++;
      if (dcount % 2)
        send_to_char(ch, "\r\n");
    }
    
  }
}

/* special affect that allows you to sense 'aggro' enemies */
void check_dangersense(struct char_data *ch, room_rnum room)
{
  struct char_data *tch;
  bool danger = FALSE;

  if (!AFF_FLAGGED(ch, AFF_DANGERSENSE) || room == NOWHERE)
    return;

  for (tch = world[room].people; tch && danger == FALSE; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch))
      continue;

    if ((MOB_FLAGGED(tch, MOB_AGGRESSIVE)) ||
        (MOB_FLAGGED(tch, MOB_AGGR_EVIL) && IS_EVIL(ch)) ||
        (MOB_FLAGGED(tch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(ch)) ||
        (MOB_FLAGGED(tch, MOB_AGGR_GOOD) && IS_GOOD(ch)))
      danger = TRUE;
  }
  if (danger)
    send_to_char(ch, "\tRYou feel \trdanger\tR there.\tn\r\n");
}

void show_obj_info(struct obj_data *obj, struct char_data *ch)
{
  int size = GET_OBJ_SIZE(obj);
  int material = GET_OBJ_MATERIAL(obj);
  int type = GET_OBJ_TYPE(obj);
  int weapon_type = GET_WEAPON_TYPE(obj);
  int armor_val = GET_OBJ_VAL(obj, 1);
  int i = 0;

  /* dummy checks due to old stock items */
  if (size < 0 || size >= NUM_SIZES)
    size = 0;
  if (material < 0 || material >= NUM_MATERIALS)
    material = 0;
  if (type < 0 || type >= NUM_ITEM_TYPES)
    type = 0;
  if (weapon_type < 0 || weapon_type >= NUM_WEAPON_TYPES)
    weapon_type = 0;
  if (armor_val < 0 || armor_val >= NUM_SPEC_ARMOR_TYPES)
    armor_val = 0;

  /* show object size and material */
  send_to_char(ch, "[Size: %s, Material: %s] ", size ? sizes[size] : "???",
               material ? material_name[material] : "???");

  /* displaying weapon / armor info */
  switch (type)
  {

  case ITEM_WEAPON:
    send_to_char(ch, "Weapon: %s ", weapon_type ? weapon_list[weapon_type].name : "???");

    /* check load-status of a reloadable weapon (such as crossbow) */
    if (is_reloading_weapon(ch, obj, TRUE))
    {
      send_to_char(ch, "| Loaded ammo: %d ", GET_OBJ_VAL(obj, 5));
    }

    break;

  case ITEM_ARMOR:
    send_to_char(ch, "Armor: %s ", armor_val ? armor_list[armor_val].name : "???");
    break;
  }

  /* spec proc system for items */
  for (i = 0; i < SPEC_TIMER_MAX; i++)
  {
    if (GET_OBJ_SPECTIMER(obj, i))
    {
      send_to_char(ch, "ImbuedPower Cooldown %d: %d hours | ", i, GET_OBJ_SPECTIMER(obj, i));
    }
  }
}

/* Subcommands */
/* For show_obj_to_char 'mode'.	/-- arbitrary */
#define SHOW_OBJ_LONG 0
#define SHOW_OBJ_SHORT 1
#define SHOW_OBJ_ACTION 2

void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode, int mxp_type)
{
  char keyword[100], keyword1[128], sendcmd[20];
  int found = 0, item_num = 0;
  struct char_data *temp;
  struct obj_data *temp_obj;

  // mxp_type 1 = do_inventory
  // mxp_type 2 = do_equipment
  // maybe change these to defines in protocol.h or something
  // these will be used to give click options i.e. click an item in inventory
  // to equip it, or right click with context menu, to equip/drop/lore/etc.

  if (!obj || !ch)
  {
    log("SYSERR: NULL pointer in show_obj_to_char(): obj=%p ch=%p", obj, ch);
    /*  SYSERR_DESC: Somehow a NULL pointer was sent to show_obj_to_char() in
     *  either the 'obj' or the 'ch' variable.  The error will indicate which
     *  was NULL by listing both of the pointers passed to it.  This is often a
     *  difficult one to trace, and may require stepping through a debugger. */
    return;
  }

  if ((mode == 0) && obj->description)
  {
    if (!GET_OBJ_VAL(obj, 1) == 0 || OBJ_SAT_IN_BY(obj))
    {
      temp = OBJ_SAT_IN_BY(obj);
      for (temp = OBJ_SAT_IN_BY(obj); temp; temp = NEXT_SITTING(temp))
      {
        if (temp == ch)
          found++;
      }
      if (found)
      {
        send_to_char(ch, "You are %s upon %s.", GET_POS(ch) == POS_SITTING ? "sitting" : "resting", obj->short_description);
        goto end;
      }
    }
  }

  switch (mode)
  {
  case SHOW_OBJ_LONG:
    /* Hide objects starting with . from non-holylighted people. - Elaseth */
    if (*obj->description == '.' && (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
      return;

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
    {
      send_to_char(ch, "[%d] ", GET_OBJ_VNUM(obj));
      if (SCRIPT(obj))
      {
        if (!TRIGGERS(SCRIPT(obj))->next)
          send_to_char(ch, "[T%d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(obj))));
        else
          send_to_char(ch, "[TRIGS] ");
      }
    }
    send_to_char(ch, "%s", CCGRN(ch, C_NRM));
    send_to_char(ch, "%s", obj->description);
    break;

  case SHOW_OBJ_SHORT:
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
    {
      send_to_char(ch, "[%d] ", GET_OBJ_VNUM(obj));
      if (SCRIPT(obj))
      {
        if (!TRIGGERS(SCRIPT(obj))->next)
          send_to_char(ch, "[T%d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(obj))));
        else
          send_to_char(ch, "[TRIGS] ");
      }
    }

    if (mxp_type != 0)
    {
      one_argument(obj->name, keyword, sizeof(keyword));

      switch (mxp_type)
      {
      case 1: // inventory
        // loop through to ensure correct item, i.e. 2.dagger, 3.armor, etc.
        for (temp_obj = ch->carrying; temp_obj; temp_obj = temp_obj->next_content)
        {
          // check if the temp_obj contains keyword in the name list
          if (isname(keyword, temp_obj->name))
          {
            if (temp_obj->short_description == obj->short_description)
              // this is the item they are trying to interact with
              // or at least has the same short description
              break;
            else
              item_num++;
          }
        }
        if (item_num > 0)
        {
          snprintf(keyword1, sizeof(keyword1), "%d.%s", (item_num + 1), keyword);
          strlcpy(keyword, keyword1, sizeof(keyword));
        }
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
          strlcpy(sendcmd, "wield", sizeof(sendcmd));
        else if (GET_OBJ_TYPE(obj) == ITEM_SCROLL)
          strlcpy(sendcmd, "recite", sizeof(sendcmd));
        else if (GET_OBJ_TYPE(obj) == ITEM_POTION)
          strlcpy(sendcmd, "quaff", sizeof(sendcmd));
        else if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
          strlcpy(sendcmd, "wear", sizeof(sendcmd));
        else if (GET_OBJ_TYPE(obj) == ITEM_WORN)
          strlcpy(sendcmd, "wear", sizeof(sendcmd));
        else if (GET_OBJ_TYPE(obj) == ITEM_FOOD)
          strlcpy(sendcmd, "eat", sizeof(sendcmd));
        else if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
          strlcpy(sendcmd, "drink", sizeof(sendcmd));
        else if (GET_OBJ_TYPE(obj) == ITEM_NOTE)
          strlcpy(sendcmd, "read", sizeof(sendcmd));
        else if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
          strlcpy(sendcmd, "look in", sizeof(sendcmd));
        else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
          strlcpy(sendcmd, "look in", sizeof(sendcmd));
        else if (GET_OBJ_TYPE(obj) == ITEM_AMMO_POUCH)
          strlcpy(sendcmd, "look in", sizeof(sendcmd));
        else
          strlcpy(sendcmd, "hold", sizeof(sendcmd));
        send_to_char(ch, "\t<send href='%s %s|drop %s|eat %s|hold %s|lore %s' hint='use/equip %s|drop %s|eat %s|hold %s|lore %s'>%s\t</send>", sendcmd, keyword,
                     keyword, keyword, keyword, keyword, keyword, keyword, keyword, keyword, keyword, obj->short_description);
        break;
      case 2: // equipment
        send_to_char(ch, "\t<send href='remove %s'>%s\t</send>", keyword, obj->short_description);
        break;
      }
    }
    else
    {
      send_to_char(ch, "%s", obj->short_description);
    }
    break;

  case SHOW_OBJ_ACTION:

    switch (GET_OBJ_TYPE(obj))
    {
    case ITEM_NOTE:
      if (obj->action_description)
      {
        char notebuf[MAX_NOTE_LENGTH + 64];

        snprintf(notebuf, sizeof(notebuf), "There is something written on it:\r\n\r\n%s", obj->action_description);
        page_string(ch->desc, notebuf, TRUE);
      }
      else
        send_to_char(ch, "It's blank.\r\n");
      return;

    case ITEM_DRINKCON:
      send_to_char(ch, "It looks like a drink container.");
      break;
    case ITEM_BLUEPRINT:
      show_craft(ch, get_craft_from_id(GET_OBJ_VAL(obj, 0)), 0);
      break;
    default:
      send_to_char(ch, "You see nothing special..");
      break;
    }

    /* obj size, material, weapon/armor */
    show_obj_info(obj, ch);

    if (GET_OBJ_TYPE(obj) == ITEM_TREASURE_CHEST)
    {
      act("\r\n$p can be looted with the loot command.", TRUE, ch, obj, 0, TO_CHAR);
    }

    break;

  default:
    log("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
    /*  SYSERR_DESC:  show_obj_to_char() has some predefined 'mode's (argument
     *  #3) to tell it what to display to the character when it is called.  If
     *  the mode is not one of these, it will output this error, and indicate
     *  what mode was passed to it.  To correct it, you will need to find the
     *  call with the incorrect mode and change it to an acceptable mode. */
    return;
  }
end:

  show_obj_modifiers(obj, ch);
  send_to_char(ch, "\r\n");
}

/* default is just showing object flags here, we've added:
 1) special, such as poison
 2) size */
static void show_obj_modifiers(struct obj_data *obj, struct char_data *ch)
{
  if (OBJ_FLAGGED(obj, ITEM_INVISIBLE))
    send_to_char(ch, " \tw(invisible)\tn");

  if (OBJ_FLAGGED(obj, ITEM_FROST))
    send_to_char(ch, " \tB(frost)\tn");

  if (OBJ_FLAGGED(obj, ITEM_FLAMING))
    send_to_char(ch, " \tR(flaming)\tn");

  if (obj->weapon_poison.poison)
    send_to_char(ch, " \tG(poisoned)\tn");

  if (OBJ_FLAGGED(obj, ITEM_BLESS) && (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_AURA_OF_GOOD)))
    send_to_char(ch, " \tn..It glows \tBblue\tn!");

  if (OBJ_FLAGGED(obj, ITEM_NODROP) && HAS_FEAT(ch, FEAT_AURA_OF_EVIL))
    send_to_char(ch, " \tn..It glows \tRred\tn!");

  if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
    send_to_char(ch, " \tn..It glows \tYyellow\tn!");

  if (OBJ_FLAGGED(obj, ITEM_GLOW))
    send_to_char(ch, " \tW..It has a soft glowing aura!\tn");

  if (OBJ_FLAGGED(obj, ITEM_HUM))
    send_to_char(ch, " \tn..It emits a faint \tChumming\tn sound!");

  if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 2) == 0)
    send_to_char(ch, " \tD(burned out)\tn");
}

void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show, int mxp_type)
{
  list_obj_to_char_full(list, ch, mode, show, mxp_type, false);
}

void list_obj_to_char_full(struct obj_data *list, struct char_data *ch, int mode, int show, int mxp_type, bool can_see_always)
{
  struct obj_data *i = NULL, *j = NULL, *display = NULL;
  bool found = FALSE;
  int num = -1;

  /* Loop through the list of objects */
  for (i = list; i; i = i->next_content)
  {
    num = 0;

    /* Check the list to see if we've already counted this object */
    for (j = list; j != i; j = j->next_content)
      if (
          (j->short_description == i->short_description && j->name == i->name) ||
          (!strcmp(j->short_description, i->short_description) && !strcmp(j->name, i->name)))
        break; /* found a matching object */
    if (j != i)
      continue; /* we counted object i earlier in the list */

    if ((display = j = i) != NULL)
    {
      /* Count matching objects, including this one */
      for (display = j = i; j; j = j->next_content)
        /* This if-clause should be exactly the same as the one in the loop above */
        if ((j->short_description == i->short_description && j->name == i->name) ||
            (!strcmp(j->short_description, i->short_description) && !strcmp(j->name, i->name)))
          if (CAN_SEE_OBJ(ch, j) || can_see_always /*|| (!AFF_FLAGGED(ch, AFF_BLIND) && OBJ_FLAGGED(j, ITEM_GLOW))*/)
          {
            /* added the ability for players to see glowing items in their inventory in the dark
             * as long as they are not blind! maybe add this to CAN_SEE_OBJ macro? */
            ++num;
            /* If the original item can't be seen, switch it for this one */
            if (display == i && !CAN_SEE_OBJ(ch, display))
              display = j;
          }

      /* When looking in room, hide objects starting with '.', except for holylight */
      if (num > 0 && (mode != SHOW_OBJ_LONG || *display->description != '.' ||
                      (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT))))
      {
        if (mode == SHOW_OBJ_LONG)
          send_to_char(ch, "%s", CCGRN(ch, C_NRM));
        if (num != 1)
          send_to_char(ch, "(%2i) ", num);
        show_obj_to_char(display, ch, mode, mxp_type);
        send_to_char(ch, "%s", CCNRM(ch, C_NRM));
        found = TRUE;
      }
    } /* end loop */
    }

  if (!found && show)
    send_to_char(ch, "  Nothing.\r\n");
}

static void diag_char_to_char(struct char_data *i, struct char_data *ch)
{

  const struct
  {
    byte percent;
    const char *text;
  } diagnosis[] = {
      {100, "is in excellent condition."},
      {90, "has a few scratches."},
      {75, "has some small wounds and bruises."},
      {50, "has quite a few wounds."},
      {30, "has some big nasty wounds and scratches."},
      {15, "looks pretty hurt."},
      {0, "is in awful condition."},
      {-1, "is bleeding awfully from big wounds."},
  };
  
  if (!ch || !i) return;

  int percent, ar_index;
  char *pers = strdup(PERS(i, ch));
  int is_disguised = GET_DISGUISE_RACE(i);

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = -1; /* How could MAX_HIT be < 1?? */

  /* nab diagnosis message */
  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  /* time to display! */
  /* show disguise race info */
  if (is_disguised)
  {
    send_to_char(ch, "%s \tn[%s %s\tn] %s\r\n", race_list[is_disguised].type,
                 size_names[GET_SIZE(i)], RACE_ABBR(i), diagnosis[ar_index].text);
    /* PC race info */
  }
  else if (!IS_NPC(i))
  {
    send_to_char(ch, "%s \tn[%s %s\tn] %s\r\n", CAP(pers), size_names[GET_SIZE(i)],
                 RACE_ABBR(i), diagnosis[ar_index].text);
    /* NPC with no race info */
  }
  else if (IS_NPC(i) && GET_RACE(i) <= RACE_TYPE_UNKNOWN)
  {
    send_to_char(ch, "%s %s\r\n", CAP(pers),
                 diagnosis[ar_index].text);
    /* NPC with no sub-race info */
  }
  else if (IS_NPC(i) && GET_SUBRACE(i, 0) <= SUBRACE_UNKNOWN && GET_SUBRACE(i, 1) <= SUBRACE_UNKNOWN && GET_SUBRACE(i, 2) <= SUBRACE_UNKNOWN)
  {
    send_to_char(ch, "%s \tn[%s %s\tn] %s\r\n", CAP(pers),
                 size_names[GET_SIZE(i)], RACE_ABBR(i), diagnosis[ar_index].text);
    /* NPC */
  }
  else
  {
    send_to_char(ch, "%s \tn[%s %s/%s/%s %s\tn] %s\r\n", CAP(pers),
                 size_names[GET_SIZE(i)], npc_subrace_abbrevs[GET_SUBRACE(i, 0)],
                 npc_subrace_abbrevs[GET_SUBRACE(i, 1)],
                 npc_subrace_abbrevs[GET_SUBRACE(i, 2)],
                 RACE_ABBR(i), diagnosis[ar_index].text);
  }

  /* some spell and spell-like affects that we want to show up */
  /* Some sort of automated system is needed here, as part of the affect system
     to facilitate these kinds of messages so that we don't have to edit so many
     things when we add an affect. - JTM 15/12/17 */
  if (affected_by_spell(i, SPELL_BARKSKIN))
    act("$s skin appears to be made of bark.", FALSE, i, 0, ch, TO_VICT);
  if (affected_by_spell(i, SPELL_STONESKIN))
    act("$s skin appears to be made of stone.", FALSE, i, 0, ch, TO_VICT);
  if (affected_by_spell(i, SPELL_IRONSKIN))
    act("$s skin appears to be made of iron.", FALSE, i, 0, ch, TO_VICT);
  if (TRLX_PSN_VAL(i) > 0 && TRLX_PSN_VAL(i) < NUM_SPELLS)
    act("$s claws are dripping with \tgpoison\tn.", FALSE, i, 0, ch, TO_VICT);

  /* clean up */
  free(pers);
  pers = NULL;
}

/*
 * These next functions/procedures are where we need to implement the customized color system!
 * To start with, just providing sane colors for things like room descriptions would go a long way.
 */

static void look_at_char(struct char_data *i, struct char_data *ch)
{
  int j, found, is_disguised = FALSE;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (!ch->desc)
    return;

  if (GET_DISGUISE_RACE(i))
    is_disguised = GET_DISGUISE_RACE(i);

  if (is_disguised)
  {
    ; /*todo, put in descriptions!*/
  }
  else if (i->player.description)
    send_to_char(ch, "%s", i->player.description);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  if (IS_NPC(i) && MOB_FLAGGED(i, MOB_IS_OBJ))
    return;

  diag_char_to_char(i, ch);

  // mounted
  if (RIDING(i) && RIDING(i)->in_room == i->in_room)
  {
    if (RIDING(i) == ch)
      act("$e is mounted on you.", FALSE, i, 0, ch, TO_VICT);
    else
    {
      snprintf(buf, sizeof(buf), "$e is mounted upon %s.", PERS(RIDING(i), ch));
      act(buf, FALSE, i, 0, ch, TO_VICT);
    }
  }
  else if (RIDDEN_BY(i) && RIDDEN_BY(i)->in_room == i->in_room)
  {
    if (RIDDEN_BY(i) == ch)
      act("You are mounted upon $m.", FALSE, i, 0, ch, TO_VICT);
    else
    {
      snprintf(buf, sizeof(buf), "$e is mounted by %s.", PERS(RIDDEN_BY(i), ch));
      act(buf, FALSE, i, 0, ch, TO_VICT);
    }
  }

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = TRUE;

  if (found && !is_disguised)
  {
    send_to_char(ch, "\r\n"); /* act() does capitalization. */
    act("$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
    {
      if (GET_EQ(i, eq_ordering_1[j]) && CAN_SEE_OBJ(ch, GET_EQ(i, eq_ordering_1[j])))
      {
        send_to_char(ch, "%s", wear_where[eq_ordering_1[j]]);
        show_obj_to_char(GET_EQ(i, eq_ordering_1[j]), ch, SHOW_OBJ_SHORT, 0);
      }
    }
  }

  if (ch != i && (IS_ROGUE(ch) || GET_LEVEL(ch) >= LVL_IMMORT))
  {
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    list_obj_to_char(i->carrying, ch, SHOW_OBJ_SHORT, TRUE, 0);
  }
}

static void list_one_char(struct char_data *i, struct char_data *ch)
{
  struct obj_data *furniture;
  char *short_descr;
  const char *const positions[NUM_POSITIONS] = {
      " is lying here, dead.",
      " is lying here, mortally wounded.",
      " is lying here, incapacitated.",
      " is lying here, stunned.",
      " is sleeping here.",
      " is reclining here.",
      " is resting here.",
      " is sitting here.",
      "!FIGHTING!", /* message elsewhere */
      " is standing here."};

  /* start display of info BEFORE short-descrip/name/title */

  /* npcs: show vnum / trig info */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
  {
    if (IS_NPC(i))
      send_to_char(ch, "[%d] ", GET_MOB_VNUM(i));
    send_to_char(ch, "[%2d] ", GET_LEVEL(i));
    if (SCRIPT(i) && TRIGGERS(SCRIPT(i)))
    {
      if (!TRIGGERS(SCRIPT(i))->next)
        send_to_char(ch, "[T%d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(i))));
      else
        send_to_char(ch, "[TRIGS] ");
    }
  }

  /* pcs: show if groupped */
  if (!IS_NPC(i) && GROUP(i))
  {
    if (GROUP(i) == GROUP(ch))
      send_to_char(ch, "(%s) ",
                   GROUP_LEADER(GROUP(i)) == i ? "leader" : "group");
    else
      send_to_char(ch, "%s(%s%s%s) ", CCNRM(ch, C_NRM), CBRED(ch, C_NRM),
                   GROUP_LEADER(GROUP(i)) == i ? "leader" : "group",
                   CCNRM(ch, C_NRM));
  }

  /* npcs: default position, not fighting */
  if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i) &&
      !FIGHTING(i))
  {
    if (AFF_FLAGGED(i, AFF_INVISIBLE))
      send_to_char(ch, "*");

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOCON) && IS_NPC(i) && !MOB_FLAGGED(i, MOB_IS_OBJ))
    {
      int level_diff = GET_LEVEL(i) - GET_LEVEL(ch);
      if (level_diff < -5)
      {
        send_to_char(ch, "[--] ");
      }
      else if (level_diff < 0)
      {
        send_to_char(ch, "[%d] ", level_diff);
      }
      else if (level_diff == 0)
      {
        send_to_char(ch, "[==] ");
      }
      else if (level_diff < 6)
      {
        send_to_char(ch, "[+%d] ", level_diff);
      }
      else
      {
        send_to_char(ch, "[!!] ");
      }
    }

    if (IS_EVIL(i) && !AFF_FLAGGED(i, AFF_HIDE_ALIGNMENT))
    {
      if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_DETECT_EVIL) || HAS_FEAT(ch, FEAT_AURA_OF_EVIL))
      {
        send_to_char(ch, "\tR(Red Aura)\tn ");
      }
    }
    else if (IS_GOOD(i) && !AFF_FLAGGED(i, AFF_HIDE_ALIGNMENT))
    {
      if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_DETECT_GOOD) || HAS_FEAT(ch, FEAT_AURA_OF_GOOD))
      {
        send_to_char(ch, "\tB(Blue Aura)\tn ");
      }
    }

    if (IS_NPC(i) && (i->mob_specials.quest))
      send_to_char(ch, "\tn(\tR!\tn) ");
    if (IS_NPC(i) && (GET_MOB_SPEC(i) == questmaster))
      send_to_char(ch, "\tn(\tY!\tn) ");

    if (strstr(i->player.long_descr, "\n"))
      send_to_char(ch, "\ty%s", i->player.long_descr);
    else
      send_to_char(ch, "\ty%s\r\n", i->player.long_descr);

    // Removed PRF_FLAGGED check - NPCs cannot have player preferences
    // if (!IS_NPC(ch) && PRF_FLAGGED(i, PRF_NON_ROLEPLAYER))
    //   act("...$e is a non-roleplayer.", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SPELL_BANISHING_BLADE))
      act("...a green blade of pure energy dances at $s side!", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SPELL_GREATER_BLACK_TENTACLES))
      act("...$e is being curshed by huge, black tentacles sprouting from the ground!", FALSE, i, 0, ch, TO_VICT);
    else if (affected_by_spell(i, SPELL_BLACK_TENTACLES))
      act("...$e is being curshed by large, black tentacles sprouting from the ground!", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_BLIND) && GET_LEVEL(i) < LVL_IMMORT)
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_FAERIE_FIRE))
      act("...$e is surrounded by a pale blue light!", FALSE, i, 0, ch, TO_VICT);
    if (KNOWS_DISCOVERY(i, ALC_DISC_VESTIGIAL_ARM))
      act("...$e has an additional arm on $s torso.", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SKILL_DRHRT_WINGS))
    {
      char wings[150];
      snprintf(wings, sizeof(wings), "...$e has two large %s wings sprouting from $s back.", DRCHRTLIST_NAME(GET_BLOODLINE_SUBTYPE(i)));
      act(wings, FALSE, i, 0, ch, TO_VICT);
    }
    else if (KNOWS_DISCOVERY(i, ALC_DISC_WINGS))
    {
      act("...$e has two large wings sprouting from $s back.", FALSE, i, 0, ch, TO_VICT);
    }
    if (affected_by_spell(i, PSIONIC_OAK_BODY))
      act("...$s skin is like that of an oak tree.", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, PSIONIC_BODY_OF_IRON))
      act("...$s skin is like a sheet of thick iron.", FALSE, i, 0, ch, TO_VICT);

    return;

    /* npcs: for non fighting mobiles */
  }
  else if (!MOB_CAN_FIGHT(i) && i->player.long_descr)
  {

    if (AFF_FLAGGED(i, AFF_INVISIBLE))
      send_to_char(ch, "*");

    if (IS_EVIL(i) && !AFF_FLAGGED(i, AFF_HIDE_ALIGNMENT))
    {
      if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_DETECT_EVIL) || HAS_FEAT(ch, FEAT_AURA_OF_EVIL))
      {
        send_to_char(ch, "\tR(Red Aura)\tn ");
      }
    }
    else if (IS_GOOD(i) && !AFF_FLAGGED(i, AFF_HIDE_ALIGNMENT))
    {
      if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_DETECT_GOOD) || HAS_FEAT(ch, FEAT_AURA_OF_GOOD))
      {
        send_to_char(ch, "\tB(Blue Aura)\tn ");
      }
    }

    send_to_char(ch, "%s", i->player.long_descr);

    // Removed PRF_FLAGGED check - NPCs cannot have player preferences
    // if !IS_NPC(ch) && (i, PRF_NON_ROLEPLAYER))
    //   act("...$e is a non-roleplayer.", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SPELL_BANISHING_BLADE))
      act("...a green blade of pure energy dances at $s side!", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SPELL_GREATER_BLACK_TENTACLES))
      act("...$e is being curshed by huge, black tentacles sprouting from the ground!", FALSE, i, 0, ch, TO_VICT);
    else if (affected_by_spell(i, SPELL_BLACK_TENTACLES))
      act("...$e is being curshed by large, black tentacles sprouting from the ground!", FALSE, i, 0, ch, TO_VICT);
     else if (affected_by_spell(i, WARLOCK_CHILLING_TENTACLES))
      act("...$e is being curshed by large, chilling black tentacles sprouting from the ground!", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_BLIND) && GET_LEVEL(i) < LVL_IMMORT)
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_FAERIE_FIRE))
      act("...$e is surrounded by a pale blue light!", FALSE, i, 0, ch, TO_VICT);
    if (KNOWS_DISCOVERY(i, ALC_DISC_VESTIGIAL_ARM))
      act("...$e has an additional arm on $s torso.", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SKILL_DRHRT_WINGS))
    {
      char wings[150];
      snprintf(wings, sizeof(wings), "...$e has two large %s wings sprouting from $s back.", DRCHRTLIST_NAME(GET_BLOODLINE_SUBTYPE(i)));
      act(wings, FALSE, i, 0, ch, TO_VICT);
    }
    else if (KNOWS_DISCOVERY(i, ALC_DISC_WINGS))
    {
      act("...$e has two large wings sprouting from $s back.", FALSE, i, 0, ch, TO_VICT);
    }
    if (affected_by_spell(i, PSIONIC_OAK_BODY))
      act("...$s skin is like that of an oak tree.", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, PSIONIC_BODY_OF_IRON))
      act("...$s skin is like a sheet of thick iron.", FALSE, i, 0, ch, TO_VICT);

    return;
  }

  /* END display of info BEFORE short-descrip/name/title */

  /* start display of "middle": short-descrip/name/title etc */

  /* npc: send short descrip */
  if (IS_NPC(i))
  {
    short_descr = strdup(i->player.short_descr);
    send_to_char(ch, "%s", CAP(short_descr));
    free(short_descr);
    short_descr = NULL;

    /* pc: name/title if not disguise, otherwise disguise info */
  }
  else
  {
    if (!GET_DISGUISE_RACE(i))
      send_to_char(ch, "\tn[%s] %s", RACE_ABBR(i),
                  // i->player.name, // This is before we switched to a title system containing the character's name
                  //  *GET_TITLE(i) ? " " : "", 
                   GET_TITLE(i));
    else if (AFF_FLAGGED(i, AFF_WILD_SHAPE))
    {
      char *an_a, *race_name;
      an_a = strdup(AN(race_list[GET_DISGUISE_RACE(i)].type));
      race_name = strdup(race_list[GET_DISGUISE_RACE(i)].type);
      *race_name = LOWER(*race_name);
      send_to_char(ch, "%s %s", CAP(an_a), race_name);
      free(an_a);
      free(race_name);
      an_a = NULL;
      race_name = NULL;
    }
    else
    {
      char *a_an;
      a_an = strdup(AN(race_list[GET_DISGUISE_RACE(i)].type));
      send_to_char(ch, "%s %s", CAP(a_an), race_list[GET_DISGUISE_RACE(i)].type);
      free(a_an);
      a_an = NULL;
    }
  }

  /* end display of "middle": short-descrip/name/title etc */

  /* start display of "ebd": info AFTER short-descrip/name/title etc */
  if (AFF_FLAGGED(i, AFF_INVISIBLE))
    send_to_char(ch, " (invisible)");
  if (AFF_FLAGGED(i, AFF_HIDE))
    send_to_char(ch, " (hidden)");
  if (!IS_NPC(i) && !i->desc)
    send_to_char(ch, " (linkless)");
  if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
    send_to_char(ch, " (writing)");
  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK))
    send_to_char(ch, " (buildwalk)");
  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_AFK))
    send_to_char(ch, " (AFK)");
  if (char_has_mud_event(i, eBARDIC_PERFORMANCE))
    send_to_char(ch, " (performing)");
  if (char_has_mud_event(i, eTAUNTED))
    send_to_char(ch, " (taunted)");
  if (char_has_mud_event(i, eINTIMIDATED))
    send_to_char(ch, " (intimidated)");
  if (char_has_mud_event(i, eVANISH))
    send_to_char(ch, " (vanished)");

  if (RIDING(i) && RIDING(i)->in_room == i->in_room)
  {
    send_to_char(ch, " is here, mounted upon ");
    if (RIDING(i) == ch)
      send_to_char(ch, "you");
    else
      send_to_char(ch, "%s", PERS(RIDING(i), ch));
    send_to_char(ch, ".");
  }
  else if (!FIGHTING(i))
  {
    if (!SITTING(i))
      send_to_char(ch, "%s", positions[GET_POS(i)]);
    else
    {
      furniture = SITTING(i);
      send_to_char(ch, " is %s upon %s.", (GET_POS(i) == POS_SLEEPING ? "sleeping" : (GET_POS(i) == POS_RECLINING ? "reclining" : (GET_POS(i) == POS_RESTING ? "resting" : "sitting"))),
                   OBJS(furniture, ch));
    }
  }
  else
  {
    if (FIGHTING(i))
    {
      send_to_char(ch, " is here, fighting ");
      if (FIGHTING(i) == ch)
        send_to_char(ch, "YOU!");
      else
      {
        if (IN_ROOM(i) == IN_ROOM(FIGHTING(i)))
          send_to_char(ch, "%s!", PERS(FIGHTING(i), ch));
        else
          send_to_char(ch, "someone who has already left!");
      }
    }
    else /* NIL fighting pointer */
      send_to_char(ch, " is here struggling with thin air.");
  }

  if (IS_EVIL(i) && !AFF_FLAGGED(i, AFF_HIDE_ALIGNMENT))
  {
    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_DETECT_EVIL) || HAS_FEAT(ch, FEAT_AURA_OF_EVIL))
    {
      send_to_char(ch, "\tR(Red Aura)\tn ");
    }
  }
  else if (IS_GOOD(i) && !AFF_FLAGGED(i, AFF_HIDE_ALIGNMENT))
  {
    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) || HAS_FEAT(ch, FEAT_DETECT_GOOD) || HAS_FEAT(ch, FEAT_AURA_OF_GOOD))
    {
      send_to_char(ch, "\tB(Blue Aura)\tn ");
    }
  }
  /* CARRIER RETURN! */
  send_to_char(ch, "\r\n");

  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_NON_ROLEPLAYER))
      act("...$e is a non-roleplayer.", FALSE, i, 0, ch, TO_VICT);
  if (AFF_FLAGGED(i, AFF_SANCTUARY))
    act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
  if (affected_by_spell(i, SPELL_BANISHING_BLADE))
    act("...a green blade of pure energy dances at $s side!", FALSE, i, 0, ch, TO_VICT);
  if (affected_by_spell(i, SPELL_GREATER_BLACK_TENTACLES))
      act("...$e is being curshed by huge, black tentacles sprouting from the ground!", FALSE, i, 0, ch, TO_VICT);
  else if (affected_by_spell(i, SPELL_BLACK_TENTACLES))
    act("...$e is being curshed by large, black tentacles sprouting from the ground!", FALSE, i, 0, ch, TO_VICT);
  if (AFF_FLAGGED(i, AFF_BLIND) && GET_LEVEL(i) < LVL_IMMORT)
    act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
  if (AFF_FLAGGED(i, AFF_FAERIE_FIRE))
    act("...$e is surrounded by a pale blue light!", FALSE, i, 0, ch, TO_VICT);
  if (KNOWS_DISCOVERY(i, ALC_DISC_VESTIGIAL_ARM))
    act("...$e has an additional arm on $s torso.", FALSE, i, 0, ch, TO_VICT);
  if (affected_by_spell(i, SKILL_DRHRT_WINGS))
  {
    char wings[150];
    snprintf(wings, sizeof(wings), "...$e has two large %s wings sprouting from $s back.", DRCHRTLIST_NAME(GET_BLOODLINE_SUBTYPE(i)));
    act(wings, FALSE, i, 0, ch, TO_VICT);
  }
  else if (KNOWS_DISCOVERY(i, ALC_DISC_WINGS))
  {
    act("...$e has two large wings sprouting from $s back.", FALSE, i, 0, ch, TO_VICT);
  }
  if (affected_by_spell(i, PSIONIC_OAK_BODY))
    act("...$s skin is like that of an oak tree.", FALSE, i, 0, ch, TO_VICT);
  if (affected_by_spell(i, PSIONIC_BODY_OF_IRON))
    act("...$s skin is like a sheet of thick iron.", FALSE, i, 0, ch, TO_VICT);
}

/*  The CAN_SEE and CAN_INFRA macros are both going to do a hide-check
 *  so we added a rule that infra won't work for hidden targets
 *    -zusuk
 */
static void list_char_to_char(struct char_data *list, struct char_data *ch)
{
  struct char_data *i;

  for (i = list; i; i = i->next_in_room)
    if (ch != i)
    {
      /* hide npcs whose description starts with a '.' from non-holylighted people - Idea from Elaseth of TBA */
      if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) &&
          IS_NPC(i) && i->player.long_descr && *i->player.long_descr == '.')
        continue;
      send_to_char(ch, "%s", CCYEL(ch, C_NRM));
      if (RIDDEN_BY(i) && RIDDEN_BY(i)->in_room == i->in_room)
        continue;
      if (CAN_SEE(ch, i))
        list_one_char(i, ch);
      else if (CAN_INFRA(ch, i) && !AFF_FLAGGED(i, AFF_HIDE))
        send_to_char(ch, "\tnYou see the \trred\tn outline of someone or something.\r\n");
      else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) &&
               char_has_infra(i) && INVIS_OK(ch, i))
        send_to_char(ch, "You see a pair of glowing red eyes looking your way.\r\n");
      else if ((!IS_DARK(IN_ROOM(ch)) || CAN_SEE_IN_DARK(ch)) &&
               AFF_FLAGGED(ch, AFF_SENSE_LIFE))
        send_to_char(ch, "You sense a life-form.\r\n");
      send_to_char(ch, "%s", CCNRM(ch, C_NRM));
    }
}

static void do_auto_exits(struct char_data *ch)
{
  int door, slen = 0;

  send_to_char(ch, "%s[ Exits: ", CCCYN(ch, C_NRM));

  for (door = 0; door < DIR_COUNT; door++)
  {
    if (!EXIT(ch, door) || EXIT(ch, door)->to_room == NOWHERE)
      continue;
    if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) && !CONFIG_DISP_CLOSED_DOORS)
      continue;
    if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) &&
        !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
      continue;
    if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) && !EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN))
      send_to_char(ch, "%s(%s)%s ", EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) ? CCWHT(ch, C_NRM) : CCRED(ch, C_NRM), autoexits[door], CCCYN(ch, C_NRM));
    else if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) && GET_LEVEL(ch) >= LVL_IMMORT)
      send_to_char(ch, "%s%s%s ", CCWHT(ch, C_NRM), autoexits[door], CCCYN(ch, C_NRM));
    else
      send_to_char(ch, "\t(%s\t) ", autoexits[door]);
    slen++;
  }

  send_to_char(ch, "%s]%s\r\n", slen ? "" : "None!", CCNRM(ch, C_NRM));
}

/* Kel: Function used by farseeing characters (later clair, wizeye) to
  look in a room, takes on the real room number, NOT vnum (from homeland) */
void look_at_room_number(struct char_data *ch, int ignore_brief, long room_number)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  char buf2[MAX_INPUT_LENGTH] = {'\0'};

  if (!ch->desc)
    return;
  if (room_number < 0)
    return;
  if (IS_SET_AR(ROOM_FLAGS(room_number), ROOM_FOG) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "A fog makes it impossible to look far.\r\n");
    return;
  }

  if (ROOM_FLAGGED(room_number, ROOM_MAGICDARK) ||
      (IS_DARK(room_number) && !CAN_SEE_IN_DARK(ch) && !char_has_infra(ch)))
  {
    send_to_char(ch, "\tLIt is pitch black...\tn\r\n");
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_MAGICDARK) ||
      (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !char_has_infra(ch)))
  {
    send_to_char(ch, "\tLIt is pitch black...\tn\r\n");
    return;
  }
  if (IS_DARK(room_number) && !CAN_SEE_IN_DARK(ch) && !char_has_infra(ch))
  {
    send_to_char(ch, "\tLIt is pitch black...\tn\r\n");
    list_char_to_char(world[room_number].people, ch);
    return;
  }
  else if (AFF_FLAGGED(ch, AFF_BLIND) && !has_blindsense(ch))
  {
    send_to_char(ch, "You're blind, you can't see anything!\r\n");
    if (AFF_FLAGGED(ch, AFF_SENSE_LIFE))
      list_char_to_char(world[room_number].people, ch);
    return;
  }
  else if (IS_DARK(room_number) && char_has_infra(ch))
  {
    send_to_char(ch, "%s", world[room_number].name);
    send_to_char(ch, "\r\n");
    // do_auto_exits(ch, room_number);
    list_char_to_char(world[room_number].people, ch);
    return;
  }
  else if (!IS_DARK(ch->in_room) && ultra_blind(ch, room_number))
  {
    send_to_char(ch, "\tWIt is far too bright to see anything...\tn\r\n");
    return;
  }

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
  {
    sprintbitarray(ROOM_FLAGS(room_number), room_bits, RF_ARRAY_MAX, buf);
    snprintf(buf2, sizeof(buf2), "\tc[%5d]\tn %s \tc[ %s] %s\tn",
             GET_ROOM_VNUM(room_number),
             world[room_number].name, buf,
             sector_types[(world[room_number].sector_type)]);
    send_to_char(ch, "%s", buf2);
  }
  else
    send_to_char(ch, "%s", world[room_number].name);
  // if (is_water_room(room_number))
  // send_to_char(" \tw(\tBWater\tw)\tn", ch);

  send_to_char(ch, "\r\n");
  if (IS_SET_AR(ROOM_FLAGS(room_number), ROOM_FOG))
    send_to_char(ch, "\tLA hazy \tWfog\tL enshrouds the area.\tn\r\n");

  if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
      ROOM_FLAGGED(room_number, ROOM_DEATH))
    send_to_char(ch, "%s", world[room_number].description);

  /* autoexits */
  // if (!IS_NPC(ch) && !IS_SET(ROOM_FLAGS(room_number), ROOM_FOG))
  // do_auto_exits(ch, room_number);

  /* now list characters & objects */
  list_obj_to_char(world[room_number].contents, ch, SHOW_OBJ_LONG, FALSE, 0);
  list_char_to_char(world[room_number].people, ch);
}
/* End of Kel's look_at_room_number function */

/* Main function to display room information when a player looks around */
void look_at_room(struct char_data *ch, int ignore_brief)
{
  trig_data *t;
  struct room_data *rm = &world[IN_ROOM(ch)];
  room_vnum target_room;
  int can_infra_in_dark = FALSE, world_map = FALSE, room_dark = FALSE;
  zone_rnum zn;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char *generated_desc = NULL;

  /* Player must have active connection to see anything */
  if (!ch->desc)
    return;

  /* Get current location info */
  target_room = IN_ROOM(ch);
  zn = GET_ROOM_ZONE(target_room);

  /* Check if room is dark (magical darkness or normal darkness) */
  if (ROOM_FLAGGED(target_room, ROOM_MAGICDARK) || IS_DARK(target_room))
    room_dark = TRUE;

  /* Override darkness if player has special vision abilities */
  if (room_dark)
  {
    if (CAN_SEE_IN_DARK(ch))  /* Has darkvision or similar */
      room_dark = FALSE;
    else if (CAN_INFRA_IN_DARK(ch))  /* Has infravision (heat vision) */
    {
      room_dark = FALSE;
      can_infra_in_dark = TRUE;
      ignore_brief = FALSE;  /* Force showing limited description */
    }
  }

  /* Special room range (66700-66799) for travel transitions */
  if (rm->number >= 66700 && rm->number <= 66799)
  {
    /* Dynamically generate room description based on travel type */
    if (ch->player_specials->travel_type == TRAVEL_CARRIAGE)
    {
      if (rm->name)
        free(rm->name);
      rm->name = strdup("A Horse-Drawn Carriage");
      snprintf(buf, sizeof(buf), "This small carriage is pulled by two brown draft horses, led by a weathered old man smoking a pipe and wearing a long, brown overcoat.\r\n"
                                 "The inside of the carriage is big enough for about 6 people, with benches on either side, and a small table in the middle.  An oil based lantern\r\n"
                                 "hangs overhead, supplying light, and windows are real glass, and able to be opened or closed, with small rain canopies keeping the weather out for the most part.\r\n"
                                 "Judging by how far you've gone so far, you have about you have \r\n"
                                 "about %d minutes and %d seconds to go until you get to %s.\r\n",
               ch->player_specials->travel_timer / 60, ch->player_specials->travel_timer % 60, carriage_locales[ch->player_specials->travel_locale][0]);
      if (rm->description)
        free(rm->description);
      rm->description = strdup(buf);
    }
    else if (ch->player_specials->travel_type == TRAVEL_SAILING)
    {
      if (rm->name)
        free(rm->name);
      rm->name = strdup("A Large Caravel");
      snprintf(buf, sizeof(buf),
               "This large passenger ship has been built with reasonably good comforts given the limited space on board.\r\n"
               "The wind fills the sails as it pushes the vessel along to your destination. The sway of the sea is steady\r\n"
               "and rhythmic, and you find you sea legs quickly.  Judging by how far you've gone so far you should arrive in\r\n"
               "about %d minutes and %d seconds to your destination: %s.\r\n",
               ch->player_specials->travel_timer / 60, ch->player_specials->travel_timer % 60, sailing_locales[ch->player_specials->travel_locale][0]);
      if (rm->description)
        free(rm->description);
      rm->description = strdup(buf);
    }
    else if (ch->player_specials->travel_type == TRAVEL_OVERLAND_FLIGHT)
    {
      if (rm->name)
        free(rm->name);
      rm->name = strdup("Flying High Above in the Sky");
      snprintf(buf, sizeof(buf),
               "Soaring through the air, high above in the sky, the landscape below stretches\r\n"
               "out endlessly, a tapestry of green forests, blue oceans, and sprawling cities.\r\n"
               "In the distance, your destination beckons forward, a faint outline on the horizon.\r\n"
               "Judging by how far you've gone so far you should arrive in\r\n"
               "about %d minutes and %d seconds to your destination: %s.\r\n",
               ch->player_specials->travel_timer / 60, ch->player_specials->travel_timer % 60,
#ifdef CAMPAIGN_FR
               zone_entrances[ch->player_specials->travel_locale][0]);
#elif defined(CAMPAIGN_DL)
               zone_entrances[ch->player_specials->travel_locale][0]);
#else
               carriage_locales[ch->player_specials->travel_locale][0]);
#endif
      if (rm->description)
        free(rm->description);
      rm->description = strdup(buf);
    }
    else if (ch->player_specials->travel_type == TRAVEL_OVERLAND_FLIGHT_SAIL)
    {
      if (rm->name)
        free(rm->name);
      rm->name = strdup("Flying High Above in the Sky");
      snprintf(buf, sizeof(buf),
               "Soaring through the air, high above in the sky, the landscape below stretches\r\n"
               "out endlessly, a tapestry of green forests, blue oceans, and sprawling cities.\r\n"
               "In the distance, your destination beckons forward, a faint outline on the horizon.\r\n"
               "Judging by how far you've gone so far you should arrive in\r\n"
               "about %d minutes and %d seconds to your destination: %s.\r\n",
               ch->player_specials->travel_timer / 60, ch->player_specials->travel_timer % 60,
#ifdef CAMPAIGN_FR
               zone_entrances[ch->player_specials->travel_locale][0]);
#elif defined(CAMPAIGN_DL)
               sailing_locales[ch->player_specials->travel_locale][0]);
#else
               carriage_locales[ch->player_specials->travel_locale][0]);
#endif
      if (rm->description)
        free(rm->description);
      rm->description = strdup(buf);
    }
  }

  /* exit conditions */
  if (room_dark)
  {
    send_to_char(ch, "It is pitch black...\r\n");
    return;
  }
  else if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT &&
           !has_blindsense(ch))
  {
    send_to_char(ch, "You see nothing but infinite darkness...\r\n");
    return;
  }
  else if (ROOM_AFFECTED(ch->in_room, RAFF_FOG))
  {
    send_to_char(ch, "Your view is obscured by a thick fog...\r\n");
    return;
  }

  /*
  if(!IS_DARK(target_room) && ULTRA_BLIND(ch, target_room)) {
    send_to_char(ch, "\tWIt is far too bright to see anything...\tn\r\n");
    return;
  }
   */

  // staff can see some extra details
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
  {
    sprintbitarray(ROOM_FLAGS(IN_ROOM(ch)), room_bits, RF_ARRAY_MAX, buf);
    send_to_char(ch, "%s", CCCYN(ch, C_NRM));
    send_to_char(ch, "[%5d]%s ", GET_ROOM_VNUM(IN_ROOM(ch)), CCNRM(ch, C_NRM));
    send_to_char(ch, "%s %s[ %s] ", world[IN_ROOM(ch)].name, CCCYN(ch, C_NRM), buf);

    if (SCRIPT(rm))
    {
      send_to_char(ch, "[T");
      for (t = TRIGGERS(SCRIPT(rm)); t; t = t->next)
        send_to_char(ch, " %d", GET_TRIG_VNUM(t));
      send_to_char(ch, "]");
    }
  }
  else // non-staffers just see the name
    send_to_char(ch, "%s", world[IN_ROOM(ch)].name);

  send_to_char(ch, " \tc[%s]\tn", sector_types[(world[IN_ROOM(ch)].sector_type)]);

  // room affections
  sprintbit((long)rm->room_affections, room_affections, buf, sizeof(buf));
  send_to_char(ch, " ( %s)", buf);

    
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
  {
    if (world[IN_ROOM(ch)].harvest_material != 0 && world[IN_ROOM(ch)].harvest_material_amount > 0)
    {
      send_to_char(ch, "\r\n\ty[MATS: %d %s] \r\n\tn",
              world[IN_ROOM(ch)].harvest_material_amount, 
              crafting_materials[world[IN_ROOM(ch)].harvest_material]);
    }
  }

  /* === SPECIAL ROOM FLAGS DISPLAY === */
  send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM)); /* End line and reset color */
  
  /* Show fog effect */
  if (IS_SET_AR(ROOM_FLAGS(target_room), ROOM_FOG))
    send_to_char(ch, "\tDA hazy \tWfog\tD enshrouds the area.\tn\r\n");
  /* Show underwater air bubbles */
  if (IS_SET_AR(ROOM_FLAGS(target_room), ROOM_AIRY))
    send_to_char(ch, "\tBLarge bubbles of air float through the water\tn\r\n");

  /* Check if this is a wilderness zone (shows special map) */
  if (ZONE_FLAGGED(zn, ZONE_WILDERNESS))
  {
    world_map = TRUE;
    /* Optional: clear screen for map display */
    /* send_to_char(ch, "\033[H\033[J"); */
  }

  /* === ROOM DESCRIPTION DISPLAY LOGIC === */
  /* Different display modes based on:
   * - worldmap vs normal room
   * - brief mode on/off  
   * - automap preference
   * - vision type (normal/infravision)
   */

  /* Show wilderness map if in wilderness zone with automap enabled */
  if ((!room_dark || can_infra_in_dark) && world_map && PRF_FLAGGED(ch, PRF_AUTOMAP))
  {
    show_wilderness_map(ch, 21, ch->coords[0], ch->coords[1]);
  }
  /* Show full room description if: not brief mode, forced to show, or death room */
  else if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF) && !can_infra_in_dark) || ignore_brief || ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH))
  {
    /* Player wants automap alongside description */
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOMAP) && can_see_map(ch))
    {
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_GUI_MODE))
      { 
        /* GUI mode: send map with XML tags */
        send_to_char(ch,
                     "<ROOM_MAP>\n"
                     "%s"
                     "</ROOM_MAP>\n"
                     "\tn%s\tn\n",
                     get_map_string(ch, target_room),
                     world[IN_ROOM(ch)].description);
      }
      else
      {
        /* Display description with map on the side */
        str_and_map(world[target_room].description, ch, target_room);
      }
    }
    /* Wilderness without automap: generate dynamic description */
    else if (world_map && !PRF_FLAGGED(ch, PRF_AUTOMAP))
    {
      generated_desc = gen_room_description(ch, IN_ROOM(ch));
      send_to_char(ch, "%s", generated_desc);
      free(generated_desc);
    }
    /* Standard room description */
    else
    {
      send_to_char(ch, "%s", world[IN_ROOM(ch)].description);
    }
  }
  /* Limited vision with infravision only */
  else if (can_infra_in_dark)
  {
    send_to_char(ch, "\tDIt is hard to make out too much detail with just \trinfravision\tD.\r\n");
  }

  /* === SPECIAL LOCATION NOTIFICATIONS === */
  
  /* Check if room is a carriage stop */
  int i = 0;
  while (atoi(carriage_locales[i][1]) != 0)
  {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == atoi(carriage_locales[i][1]))
    {
      send_to_char(ch, "\r\nThis room is a carriage stop.  Use the \tYcarriage\tn command to see options.\r\n");
      break;
    }
    i++;
  }
  
  /* Check if room is a ferry dock */
  i = 0;
  while (atoi(sailing_locales[i][1]) != 0)
  {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == atoi(sailing_locales[i][1]))
    {
      send_to_char(ch, "\r\nThis room is a ferry dock.  Use the \tYsail\tn command to see options.\r\n");
      break;
    }
    i++;
  }

  /* Notify if random encounter spawned */
  if (in_encounter_room(ch))
  {
    send_to_char(ch, "\r\nYou have spawned a random encounter. See \tYHELP ENCOUNTER\tn for more information.\r\n");
  }

  /* Show available exits (unless fog blocks view) */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT) &&
      (!IS_SET_AR(ROOM_FLAGS(target_room), ROOM_FOG) || GET_LEVEL(ch) >= LVL_IMMORT))
    do_auto_exits(ch);

  /* Special hazard: acid pool (blocks seeing contents) */
  if (ROOM_AFFECTED(ch->in_room, RAFF_KAPAK_ACID))
  {
    send_to_char(ch, "\tMA pool of acid covers the area.\tn\r\n");
    return;
  }

  /* === LIST ROOM CONTENTS === */
  /* Show all objects on the ground */
  list_obj_to_char(world[IN_ROOM(ch)].contents, ch, SHOW_OBJ_LONG, FALSE, 0);
  /* Show all characters/NPCs in room */
  list_char_to_char(world[IN_ROOM(ch)].people, ch);
}

static void look_in_direction(struct char_data *ch, int dir)
{
  if (IS_SET_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_FOG) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "A fog makes it impossible to look far.\r\n");
    return;
  }

  if (EXIT(ch, dir))
  {
    if (EXIT_FLAGGED(EXIT(ch, dir), EX_HIDDEN) && GET_LEVEL(ch) < LVL_IMMORT)
      ;
    else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && EXIT(ch, dir)->keyword)
      send_to_char(ch, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
    else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) && EXIT(ch, dir)->keyword)
      send_to_char(ch, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));

    if (EXIT(ch, dir)->general_description)
      send_to_char(ch, "%s", EXIT(ch, dir)->general_description);
    else
      send_to_char(ch, "You do not see anything particularly special in that direction.\r\n");

    check_dangersense(ch, EXIT(ch, dir)->to_room);
  }
  else
    send_to_char(ch, "You do not see anything special in that direction...\r\n");
}

static void look_in_obj(struct char_data *ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char(ch, "Look in what?\r\n");
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj)))
  {
    send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
  }
  else if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
  {
    if (GET_LEVEL(ch) < LVL_IMMORT)
      display_spells(ch, obj, 0);
    else
      display_spells(ch, obj, 0);
  }
  else if (GET_OBJ_TYPE(obj) == ITEM_SCROLL)
  {
    display_scroll(ch, obj);
  }
  else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
           (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
           (GET_OBJ_TYPE(obj) != ITEM_AMMO_POUCH) &&
           (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
    send_to_char(ch, "There's nothing inside that!\r\n");
  else
  {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER ||
        GET_OBJ_TYPE(obj) == ITEM_AMMO_POUCH)
    {
      if (OBJVAL_FLAGGED(obj, CONT_CLOSED) && (GET_LEVEL(ch) < LVL_IMMORT || !PRF_FLAGGED(ch, PRF_NOHASSLE)))
        send_to_char(ch, "It is closed.\r\n");
      else
      {
        send_to_char(ch, "%s", fname(obj->name));
        switch (bits)
        {
        case FIND_OBJ_INV:
          send_to_char(ch, " (carried): \r\n");
          break;
        case FIND_OBJ_ROOM:
          send_to_char(ch, " (here): \r\n");
          break;
        case FIND_OBJ_EQUIP:
          send_to_char(ch, " (used): \r\n");
          break;
        }

        list_obj_to_char(obj->contains, ch, SHOW_OBJ_SHORT, TRUE, 0);
      }
    }
    else
    { /* item must be a fountain or drink container */
      if ((GET_OBJ_VAL(obj, 1) == 0) && (GET_OBJ_VAL(obj, 0) != -1))
        send_to_char(ch, "It is empty.\r\n");
      else
      {
        if (GET_OBJ_VAL(obj, 0) < 0)
        {
          char buf2[MAX_STRING_LENGTH] = {'\0'};
          sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2, sizeof(buf2));
          send_to_char(ch, "It's full of a %s liquid.\r\n", buf2);
        }
        else if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0))
          send_to_char(ch, "Its contents seem somewhat murky.\r\n"); /* BUG */
        else
        {
          char buf2[MAX_STRING_LENGTH] = {'\0'};
          amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
          sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2, sizeof(buf2));
          send_to_char(ch, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
        }
      }
    }
  }
}

char *find_exdesc(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (*i->keyword == '.' ? isname(word, i->keyword + 1) : isname(word, i->keyword))
      return (i->description);

  return (NULL);
}

static void perform_mortal_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct descriptor_data *d;
  int j;

  if (!*arg)
  {
    j = world[(IN_ROOM(ch))].zone;
    send_to_char(ch, "\tb--\tB= \tCPlayers in %s \tB=\tb--\r\n\tc-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\tn\r\n", zone_table[j].name);
    for (d = descriptor_list; d; d = d->next)
    {
      if (STATE(d) != CON_PLAYING || d->character == ch)
        continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
        continue;
      if (IN_ROOM(i) == NOWHERE || !CAN_SEE(ch, i))
        continue;
      if (world[IN_ROOM(ch)].zone != world[IN_ROOM(i)].zone)
        continue;
      send_to_char(ch, "\tB[\tc%-20s%s\tB]\tW - %s%s\tn\r\n", GET_NAME(i), QNRM, world[IN_ROOM(i)].name, QNRM);
    }
  }
  else
  { /* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next)
    {
      if (IN_ROOM(i) == NOWHERE || i == ch)
        continue;
      if (!CAN_SEE(ch, i) || world[IN_ROOM(i)].zone != world[IN_ROOM(ch)].zone)
        continue;
      if (!isname(arg, i->player.name))
        continue;
      send_to_char(ch, "\tB[\tc%-25s%s\tB]\tW - %s%s\tn\r\n", GET_NAME(i), QNRM, world[IN_ROOM(i)].name, QNRM);
      return;
    }
    send_to_char(ch, "Nobody around by that name.\r\n");
  }
}

static void print_object_location(int num, struct obj_data *obj, struct char_data *ch,
                                  int recur)
{
  if (num > 0)
    send_to_char(ch, "O%3d. %-25s%s - ", num, obj->short_description, QNRM);
  else
    send_to_char(ch, "%33s", " - ");

  if (SCRIPT(obj))
  {
    if (!TRIGGERS(SCRIPT(obj))->next)
      send_to_char(ch, "[T%d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(obj))));
    else
      send_to_char(ch, "[TRIGS] ");
  }

  if (IN_ROOM(obj) != NOWHERE)
    send_to_char(ch, "[%5d] %s%s\r\n", GET_ROOM_VNUM(IN_ROOM(obj)), world[IN_ROOM(obj)].name, QNRM);
  else if (obj->carried_by)
    send_to_char(ch, "carried by %s%s\r\n", PERS(obj->carried_by, ch), QNRM);
  else if (obj->worn_by)
    send_to_char(ch, "worn by %s%s\r\n", PERS(obj->worn_by, ch), QNRM);
  else if (obj->in_obj)
  {
    send_to_char(ch, "inside %s%s%s\r\n", obj->in_obj->short_description, QNRM, (recur ? ", which is" : " "));
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  }
  else
    send_to_char(ch, "in an unknown location\r\n");
}

static void perform_immort_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;

  if (!*arg)
  {
    send_to_char(ch, "Players  Room    Location                       Zone\r\n");
    send_to_char(ch, "-------- ------- ------------------------------ -------------------\r\n");
    for (d = descriptor_list; d; d = d->next)
      if (IS_PLAYING(d))
      {
        i = (d->original ? d->original : d->character);
        if (i && CAN_SEE(ch, i) && (IN_ROOM(i) != NOWHERE))
        {
          if (d->original)
            send_to_char(ch, "%-8s%s - [%5d] %s%s (in %s%s)\r\n",
                         GET_NAME(i), QNRM, GET_ROOM_VNUM(IN_ROOM(d->character)),
                         world[IN_ROOM(d->character)].name, QNRM, GET_NAME(d->character), QNRM);
          else
            send_to_char(ch, "%-8s%s %s[%s%5d%s]%s %-*s%s %s%s\r\n", GET_NAME(i), QNRM,
                         QCYN, QYEL, GET_ROOM_VNUM(IN_ROOM(i)), QCYN, QNRM,
                         30 + count_color_chars(world[IN_ROOM(i)].name), world[IN_ROOM(i)].name, QNRM,
                         zone_table[(world[IN_ROOM(i)].zone)].name, QNRM);
        }
      }
  }
  else
  {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && IN_ROOM(i) != NOWHERE && isname(arg, i->player.name))
      {
        found = 1;
        send_to_char(ch, "M%3d. %-25s%s - [%5d] %-25s%s", ++num, GET_NAME(i), QNRM,
                     GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name, QNRM);
        if (SCRIPT(i) && TRIGGERS(SCRIPT(i)))
        {
          if (!TRIGGERS(SCRIPT(i))->next)
            send_to_char(ch, "[T%d] ", GET_TRIG_VNUM(TRIGGERS(SCRIPT(i))));
          else
            send_to_char(ch, "[TRIGS] ");
        }
        send_to_char(ch, "%s\r\n", QNRM);
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name))
      {
        found = 1;
        print_object_location(++num, k, ch, TRUE);
      }
    if (!found)
      send_to_char(ch, "Couldn't find any such thing.\r\n");
  }
}

/* Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room with
 * the name.  Then check local objs for exdescs. Thanks to Angus Mezick for
 * the suggested fix to this problem. */
static void look_at_target(struct char_data *ch, char *arg)
{
  int bits, found = FALSE, j, fnum, i = 0, x = 0;
  struct char_data *found_char = NULL;
  struct obj_data *obj, *found_obj = NULL;
  char *desc;
  char desc_out[MAX_MESSAGE_LENGTH] = {'\0'};

  if (!ch->desc)
    return;

  if (!*arg)
  {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL)
  {
    look_at_char(found_char, ch);
    if (ch != found_char)
    {
      if (CAN_SEE(found_char, ch))
        act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    return;
  }

  /* Strip off "number." from 2.foo and friends. */
  if (!(fnum = get_number(&arg)))
  {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[IN_ROOM(ch)].ex_description)) != NULL && ++i == fnum)
  {
    page_string(ch->desc, desc, FALSE);
    return;
  }

  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
      if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL && ++i == fnum)
      {

        /* obj size, material, weapon/armor */
        show_obj_info(GET_EQ(ch, j), ch);
        send_to_char(ch, "\r\n");

        if (OBJ_FLAGGED(GET_EQ(ch, j), ITEM_CRAFTED))
        {
          snprintf(desc_out, sizeof(desc_out), "%s", strfrmt(desc, 80, 1, FALSE, FALSE, FALSE));
          for (x = 0; x < strlen(desc_out); x++)
          {
            if (desc_out[x] == '~')
              desc_out[x] = '\0';
          }
        }
        else
        {
          snprintf(desc_out, sizeof(desc_out), "%s", desc);
        }

        send_to_char(ch, "%s\n", desc_out);

        if (j == WEAR_SHEATH)
        {
          if (GET_EQ(ch, j)->sheath_primary || GET_EQ(ch, j)->sheath_secondary)
          {
            send_to_char(ch, "It contains:\r\n");

            if (GET_EQ(ch, j)->sheath_primary)
              send_to_char(ch, "  %s\r\n", GET_EQ(ch, j)->sheath_primary->short_description);

            if (GET_EQ(ch, j)->sheath_secondary)
              send_to_char(ch, "  %s\r\n", GET_EQ(ch, j)->sheath_secondary->short_description);
          }
          else
          {
            send_to_char(ch, "It is currently empty.\r\n");
          }
        }
        found = TRUE;
      }

  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content)
  {
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum)
      {

        /* obj size, material, weapon/armor */
        if (!found)
        {
          show_obj_info(obj, ch);
          send_to_char(ch, "\r\n");
        }

        if (OBJ_FLAGGED(obj, ITEM_CRAFTED))
        {
          snprintf(desc_out, sizeof(desc_out), "%s", strfrmt(desc, 80, 1, FALSE, FALSE, FALSE));
          for (x = 0; x < strlen(desc_out); x++)
          {
            if (desc_out[x] == '~')
              desc_out[x] = '\0';
          }
        }
        else
        {
          snprintf(desc_out, sizeof(desc_out), "%s", desc);
        }

        send_to_char(ch, "%s\n", desc_out);

        if (CAN_WEAR(obj, ITEM_WEAR_SHEATH))
        {
          if (obj->sheath_primary || obj->sheath_secondary)
          {
            send_to_char(ch, "It contains:\r\n");

            if (obj->sheath_primary)
              send_to_char(ch, "  %s\r\n", obj->sheath_primary->short_description);

            if (obj->sheath_secondary)
              send_to_char(ch, "  %s\r\n", obj->sheath_secondary->short_description);
          }
          else
          {
            send_to_char(ch, "It is currently empty.\r\n");
          }
        }

        found = TRUE;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[IN_ROOM(ch)].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum)
      {

        /* obj size, material, weapon/armor */
        if (!found)
        {
          show_obj_info(obj, ch);
          send_to_char(ch, "\r\n");
        }

        if (OBJ_FLAGGED(obj, ITEM_CRAFTED))
        {
          snprintf(desc_out, sizeof(desc_out), "%s", strfrmt(desc, 80, 1, FALSE, FALSE, FALSE));
          for (x = 0; x < strlen(desc_out); x++)
          {
            if (desc_out[x] == '~')
              desc_out[x] = '\0';
          }
        }
        else
        {
          snprintf(desc_out, sizeof(desc_out), "%s", desc);
        }

        send_to_char(ch, "%s\n", desc_out);

        if (CAN_WEAR(obj, ITEM_WEAR_SHEATH))
        {
          if (obj->sheath_primary || obj->sheath_secondary)
          {
            send_to_char(ch, "It contains:\r\n");

            if (obj->sheath_primary)
              send_to_char(ch, "  %s\r\n", obj->sheath_primary->short_description);

            if (obj->sheath_secondary)
              send_to_char(ch, "  %s\r\n", obj->sheath_secondary->short_description);
          }
          else
          {
            send_to_char(ch, "It is currently empty.\r\n");
          }
        }

        found = TRUE;
      }

  /* If an object was found back in generic_find */
  if (bits)
  {
    if (!found)
      show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION, 0);
    else
    {
      show_obj_modifiers(found_obj, ch);
      send_to_char(ch, "\r\n");
    }
  }
  else if (!found)
    send_to_char(ch, "You do not see that here.\r\n");
}

void perform_cooldowns(struct char_data *ch, struct char_data *k)
{
  struct mud_event_data *pMudEvent = NULL;

  send_to_char(ch, "\tC");
  text_line(ch, "\tYCooldowns\tC", 80, '-', '-');
  send_to_char(ch, "\tn");

  if ((pMudEvent = char_has_mud_event(k, eINTIMIDATE_COOLDOWN)))
    send_to_char(ch, "Intimidate Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eTAUNT)))
    send_to_char(ch, "Taunt Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eVANISHED)))
    send_to_char(ch, "Vanish Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eINVISIBLE_ROGUE)))
    send_to_char(ch, "Invisible Rogue Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eRAGE)))
    send_to_char(ch, "Rage Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSACRED_FLAMES)))
    send_to_char(ch, "Sacred Flames Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eINNER_FIRE)))
    send_to_char(ch, "Inner Fire Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eMUTAGEN)))
    send_to_char(ch, "Mutagen/Cognatogen Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, ePSYCHOKINETIC)))
    send_to_char(ch, "Psychokinetic Tincture Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCRIPPLING_CRITICAL)))
    send_to_char(ch, "Crippling Critical Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eDEFENSIVE_STANCE)))
    send_to_char(ch, "Defensive Stance Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eINSECTBEING)))
    send_to_char(ch, "Insect Being Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCRYSTALFIST)))
    send_to_char(ch, "Crystal Fist Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCRYSTALBODY)))
    send_to_char(ch, "Crystal Body Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eMASTERMIND)))
    send_to_char(ch, "Mastermind Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eTINKER)))
    send_to_char(ch, "Tinker Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSLA_LEVITATE)))
    send_to_char(ch, "Levitate Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSLA_DARKNESS)))
    send_to_char(ch, "Darkness Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSLA_FAERIE_FIRE)))
    send_to_char(ch, "Faerie Fire Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eLAYONHANDS)))
    send_to_char(ch, "Lay on Hands Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eWHOLENESSOFBODY)))
    send_to_char(ch, "Wholeness of Body Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eRENEWEDVIGOR)))
    send_to_char(ch, "Renewed Vigor Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eRENEWEDDEFENSE)))
    send_to_char(ch, "Renewed Defense Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if (AFF_FLAGGED(ch, AFF_GRAPPLED)) /*no need for message if not grappling*/
    if ((pMudEvent = char_has_mud_event(k, eSTRUGGLE)))
      send_to_char(ch, "Struggle Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eTREATINJURY)))
    send_to_char(ch, "Treat Injuries Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eEMPTYBODY)))
    send_to_char(ch, "Empty Body Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eMUMMYDUST)))
    send_to_char(ch, "Epic Spell Cooldown :  Mummy Dust - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eDRAGONKNIGHT)))
    send_to_char(ch, "Epic Spell Cooldown :  Dragon Knight - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eGREATERRUIN)))
    send_to_char(ch, "Epic Spell Cooldown :  Greater Ruin - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eHELLBALL)))
    send_to_char(ch, "Epic Spell Cooldown :  Hellball - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eEPICMAGEARMOR)))
    send_to_char(ch, "Epic Spell Cooldown :  Epic Mage Armor - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eEPICWARDING)))
    send_to_char(ch, "Epic Spell Cooldown :  Epic Warding - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eANIMATEDEAD)))
    send_to_char(ch, "Animate Dead Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSTUNNINGFIST)))
    send_to_char(ch, "Stunning Fist Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSURPRISE_ACCURACY)))
    send_to_char(ch, "Surprise Accuracy Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCOME_AND_GET_ME)))
    send_to_char(ch, "Come and Get Me! Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, ePOWERFUL_BLOW)))
    send_to_char(ch, "Powerful Blow Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eDEATHARROW)))
    send_to_char(ch, "Death Arrow Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eQUIVERINGPALM)))
    send_to_char(ch, "Quivering Palm Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eD_ROLL)))
    send_to_char(ch, "Defensive Roll Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eLICH_REJUV)))
    send_to_char(ch, "Lich Rejuvenation Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eLAST_WORD)))
    send_to_char(ch, "Last Word Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, ePURIFY)))
    send_to_char(ch, "Purify Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eC_ANIMAL)))
    send_to_char(ch, "Call Companion Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eC_DRAGONMOUNT)))
    send_to_char(ch, "Call Dragon Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eC_EIDOLON)))
    send_to_char(ch, "Call Eidolon Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eC_FAMILIAR)))
    send_to_char(ch, "Call Familiar Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eC_MOUNT)))
    send_to_char(ch, "Call Mount Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSUMMONSHADOW)))
    send_to_char(ch, "Call Shadow Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eIMBUE_ARROW)))
    send_to_char(ch, "Imbue Arrow Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eARROW_SWARM)))
    send_to_char(ch, "Arrow Swarm Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSEEKER_ARROW)))
    send_to_char(ch, "Seeker Arrow Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSMITE_EVIL)))
    send_to_char(ch, "Smite Evil Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSMITE_GOOD)))
    send_to_char(ch, "Smite Good Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSMITE_DESTRUCTION)))
    send_to_char(ch, "Smite Destruction Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, ePERFORM)))
    send_to_char(ch, "Perform Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eTURN_UNDEAD)))
    send_to_char(ch, "Turn Undead Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSPELLBATTLE)))
    send_to_char(ch, "Spellbattle Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eWILD_SHAPE)))
    send_to_char(ch, "Wild Shape Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSHIELD_RECOVERY)))
    send_to_char(ch, "Shield Recovery Cooldown  - Duration %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eLIGHTNING_ARC)))
    send_to_char(ch, "Lightning Arc Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eACID_DART)))
    send_to_char(ch, "Acid Dart Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eFIRE_BOLT)))
    send_to_char(ch, "Fire Bolt Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eICICLE)))
    send_to_char(ch, "Icicle Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCURSE_TOUCH)))
    send_to_char(ch, "Curse Touch Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eDESTRUCTIVE_AURA)))
    send_to_char(ch, "Destructive Aura Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eEVIL_TOUCH)))
    send_to_char(ch, "Evil Touch Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eGOOD_TOUCH)))
    send_to_char(ch, "Good Touch Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eHEALING_TOUCH)))
    send_to_char(ch, "Healing Touch Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCURING_TOUCH)))
    send_to_char(ch, "Curing Touch Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eEYE_OF_KNOWLEDGE)))
    send_to_char(ch, "Eye of Knowledge Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eBLESSED_TOUCH)))
    send_to_char(ch, "Blessed Touch Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCOPYCAT)))
    send_to_char(ch, "Copycat Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eMASS_INVIS)))
    send_to_char(ch, "Mass Invis Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eAURA_OF_PROTECTION)))
    send_to_char(ch, "Aura of Protection Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eBATTLE_RAGE)))
    send_to_char(ch, "Battle Rage Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eDRACBREATH)))
    send_to_char(ch, "Draconic Heritage Breath Weapon Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eDRACCLAWS)))
    send_to_char(ch, "Draconic Heritage Claws Attack Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eDRAGBREATH)))
    send_to_char(ch, "Dragon Heritage Breath Weapon Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCATSCLAWS)))
    send_to_char(ch, "Tabaxi Cats Claws Attack Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSLA_STRENGTH)))
    send_to_char(ch, "Strength Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSLA_ENLARGE)))
    send_to_char(ch, "Enlarge Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSLA_INVIS)))
    send_to_char(ch, "Invisibility Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCHANNELSPELL)))
    send_to_char(ch, "Channel Spell Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, ePSIONICFOCUS)))
    send_to_char(ch, "Psionic Focus Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eDOUBLEMANIFEST)))
    send_to_char(ch, "Double Manifest Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSHADOWILLUSION)))
    send_to_char(ch, "Shadow Illusion Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSHADOWCALL)))
    send_to_char(ch, "Shadow Call Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSHADOWPOWER)))
    send_to_char(ch, "Shadow Power Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSHADOWJUMP)))
    send_to_char(ch, "Shadow Jump Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eTOUCHOFCORRUPTION)))
    send_to_char(ch, "Touch of Corruption Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eTOUCHOFUNDEATH)))
    send_to_char(ch, "Touch of Undeath Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCHANNELENERGY)))
    send_to_char(ch, "Channel Energy Cooldown - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eEVOBREATH)))
    send_to_char(ch, "Eidolon Breath Weapon Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSTRENGTHOFHONOR)))
    send_to_char(ch, "Strength of Honor Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCROWNOFKNIGHTHOOD)))
    send_to_char(ch, "Crown of Knighthood Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSOULOFKNIGHTHOOD)))
    send_to_char(ch, "Crown of Knighthood Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCOSMICUNDERSTANDING)))
    send_to_char(ch, "Cosmic Understanding Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eDRAGOONPOINTS)))
    send_to_char(ch, "Dragoon Points Cooldown  - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));

  if (GET_SETCLOAK_TIMER(ch) > 0)
    send_to_char(ch, "Vampire 'Setcloak' Cooldown - Duration: %d seconds\r\n", GET_SETCLOAK_TIMER(ch) * 6);
  if (PIXIE_DUST_TIMER(ch) > 0)
    send_to_char(ch, "Pixie Dust Cooldown - Duration: %d seconds\r\n", PIXIE_DUST_TIMER(ch) * 6);
  if (EFREETI_MAGIC_TIMER(ch) > 0)
    send_to_char(ch, "Efreeti Magic Cooldown - Duration: %d seconds\r\n", EFREETI_MAGIC_TIMER(ch) * 6);
  if (DRAGON_MAGIC_TIMER(ch) > 0)
    send_to_char(ch, "Dragon Magic Cooldown - Duration: %d seconds\r\n", DRAGON_MAGIC_TIMER(ch) * 6);
  if (LAUGHING_TOUCH_TIMER(ch) > 0)
    send_to_char(ch, "Laughing Touch Cooldown - Duration: %d seconds\r\n", LAUGHING_TOUCH_TIMER(ch) * 6);
  if (FLEETING_GLANCE_TIMER(ch) > 0)
    send_to_char(ch, "Fleeting Glance Cooldown - Duration: %d seconds\r\n", FLEETING_GLANCE_TIMER(ch) * 6);
  if (FEY_SHADOW_WALK_TIMER(ch) > 0)
    send_to_char(ch, "Fey Shadow Walk Cooldown - Duration: %d seconds\r\n", FEY_SHADOW_WALK_TIMER(ch) * 6);
  if (GRAVE_TOUCH_TIMER(ch) > 0)
    send_to_char(ch, "Grave Touch Cooldown - Duration: %d seconds\r\n", GRAVE_TOUCH_TIMER(ch) * 6);
  if (GRASP_OF_THE_DEAD_TIMER(ch) > 0)
    send_to_char(ch, "Grasp of the Dead Cooldown - Duration: %d seconds\r\n", GRASP_OF_THE_DEAD_TIMER(ch) * 6);
  if (INCORPOREAL_FORM_TIMER(ch) > 0)
    send_to_char(ch, "Incorporeal Form (Undead Bloodline) Cooldown - Duration: %d seconds\r\n", INCORPOREAL_FORM_TIMER(ch) * 6);
  if (GET_MISSION_COOLDOWN(k) > 0)
    send_to_char(ch, "Mission Ready Cooldown - Duration: %d seconds\r\n", GET_MISSION_COOLDOWN(k) * 6);
  if (GET_KAPAK_SALIVA_HEALING_COOLDOWN(k) > 0)
    send_to_char(ch, "Kapak Saliva Healing - Duration: %d seconds\r\n", GET_KAPAK_SALIVA_HEALING_COOLDOWN(k) * 6);
  if (GET_FIGHT_TO_THE_DEATH_COOLDOWN(k) > 0)
    send_to_char(ch, "Fight to the Death - Duration: %d seconds\r\n", GET_FIGHT_TO_THE_DEATH_COOLDOWN(k) * 6);
  if (ch->char_specials.terror_cooldown > 0)
    send_to_char(ch, "Aura of Terror Immunity - Duration: %d seconds\r\n",  ch->char_specials.terror_cooldown * 6);
  if (GET_FORAGE_COOLDOWN(k) > 0)
    send_to_char(ch, "Forage - Duration: %d seconds\r\n", GET_FORAGE_COOLDOWN(k) * 6);
  if (GET_RETAINER_COOLDOWN(k) > 0)
    send_to_char(ch, "Call Retainer - Duration: %d seconds\r\n", GET_RETAINER_COOLDOWN(k) * 6);
  if (GET_SCROUNGE_COOLDOWN(k) > 0)
    send_to_char(ch, "Scrounge - Duration: %d seconds\r\n", GET_SCROUNGE_COOLDOWN(k) * 6);

  list_item_activate_ability_cooldowns(ch);

  send_to_char(ch, "\tC");
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "\tn");

  /* leads to other related commands */
  if (ch == k)
  {
    send_to_char(ch, "\tDType 'affects' to see your affects and conditions.\tn\r\n");
    send_to_char(ch, "\tDType 'resistances' to see your resistances and damage reduction.\tn\r\n");
    send_to_char(ch, "\tDType 'abilities' to see your class and innate abilities.\tn\r\n");
    send_to_char(ch, "\tDType 'damagereduction' to see your damage reduction breakdown.\tn\r\n");
    send_to_char(ch, "\tDType 'maxhp' to see hown your maximum hit points are calculated.\tn\r\n");
  }
}

void perform_damage_reduction(struct char_data *ch, struct char_data *k)
{
  send_to_char(ch, "\tC");
  text_line(ch, "\tYDamage Reduction Breakdown\tC", 80, '-', '-');
  send_to_char(ch, "\tn");
  compute_damage_reduction_full(k, 0, true);
}

void perform_resistances(struct char_data *ch, struct char_data *k)
{
  int i = 0, dcount = 0;
  // char buf[MAX_STRING_LENGTH] = {'\0'};

  send_to_char(ch, "\tC");
  text_line(ch, "\tYDamage Type Resistance / Vulnerability\tC", 80, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_DAM_TYPES - 1; i++)
  {
    if (can_dam_be_resisted(i+1))
    {
      send_to_char(ch, "     %-15s: %-4d%% (%-2d)         ", damtype_display[i + 1],
                   compute_damtype_reduction(k, i + 1), compute_energy_absorb(k, i + 1));
      dcount++;
      if (dcount % 2)
        send_to_char(ch, "\r\n");
    }
  }

  send_to_char(ch, "\r\n\tC");
  text_line(ch, "\tYSpell Resistance\tC", 80, '-', '-');
  send_to_char(ch, "\tn");
  send_to_char(ch, "Spell Resist: %d\r\n", compute_spell_res(k, k, 0));

  send_to_char(ch, "\tC");
  text_line(ch, "\tYConcealment\tC", 80, '-', '-');
  send_to_char(ch, "\tn");
  send_to_char(ch, "Conceal Percent: %d\r\n", compute_concealment(k, NULL));

  send_to_char(ch, "\tC");
  text_line(ch, "\tYDamage Reduction\tC", 80, '-', '-');
  send_to_char(ch, "\tn");

  /* old damage reduction is still used */
  send_to_char(ch, "General damage reduction: %d\r\n", compute_damage_reduction(k, 0));

  /* new DR */
  struct damage_reduction_type *dr;
  dr = GET_DR(k);
  while (dr != NULL)
  {
    if (dr->spell != SPELL_RESERVED_DBC)
    {
      /* This is from a spell */
      send_to_char(ch, "%s%-19s%s ",
                   CCCYN(ch, C_NRM), spell_name(dr->spell), CCNRM(ch, C_NRM));
    }
    else if (dr->feat != FEAT_UNDEFINED)
    {
      /* This is from a feat */
      send_to_char(ch, "%s%-19s%s ",
                   CCCYN(ch, C_NRM), feat_list[dr->feat].name, CCNRM(ch, C_NRM));
    }

    send_to_char(ch, "DR %d/", dr->amount);

    for (i = 0; i < MAX_DR_BYPASS; i++)
    {
      if (dr->bypass_cat[i] != DR_BYPASS_CAT_UNUSED)
      {
        if (i > 0)
        {
          send_to_char(ch, " or ");
        }
        switch (dr->bypass_cat[i])
        {
        case DR_BYPASS_CAT_NONE:
          /* Nothing bypasses this dr. */
          send_to_char(ch, "-");
          break;
        case DR_BYPASS_CAT_MATERIAL:
          send_to_char(ch, "%s", material_name[dr->bypass_val[i]]);
          break;
        case DR_BYPASS_CAT_MAGIC:
          send_to_char(ch, "magic");
          break;
        case DR_BYPASS_CAT_DAMTYPE:
          send_to_char(ch, "%s", damtypes[dr->bypass_val[i]]);
          break;
        case DR_BYPASS_CAT_ALIGNMENT:
          send_to_char(ch, "%s", dr_aligns[dr->bypass_val[i]]);
        default:
          send_to_char(ch, "???");
        }
      }
    }
    if (dr->max_damage > 0)
     send_to_char(ch, " %d damage left", dr->max_damage);
    send_to_char(ch, "\r\n");
    dr = dr->next;
  }
  send_to_char(ch, "\tC");
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "\tn");

  /* leads to other related commands */
  if (ch == k)
  {
    send_to_char(ch, "\tDType 'affects' to see your affects and conditions.\tn\r\n");
    send_to_char(ch, "\tDType 'cooldowns' to see your cooldowns.\tn\r\n");
    send_to_char(ch, "\tDType 'abilities' to see your class and innate abilities.\tn\r\n");
    send_to_char(ch, "\tDType 'damagereduction' to see your damage reduction breakdown.\tn\r\n");
  }
}

void perform_affects(struct char_data *ch, struct char_data *k)
{
  int i = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  char buf3[MAX_STRING_LENGTH] = {'\0'};

  struct affected_type *aff = NULL;
  struct mud_event_data *pMudEvent = NULL;

  send_to_char(ch, "\tC");
  text_line(ch, "\tYAffected By\tC", 90, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_AFF_FLAGS; i++)
  {
    if (IS_SET_AR(AFF_FLAGS(k), i))
    {
      send_to_char(ch, "%s%-20s%s - %s%s%s\r\n",
                   CCNRM(ch, C_NRM), affected_bits[i], CCNRM(ch, C_NRM),
                   CCNRM(ch, C_NRM), affected_bit_descs[i], CCNRM(ch, C_NRM));
    }
  }

  send_to_char(ch, "\tC");
  text_line(ch, "\tYSpell/Skill-like Affects\tC", 90, '-', '-');
  send_to_char(ch, "\tn");

  buf[0] = '\0'; // Reset the string buffer for later use.

  /* Bonus Type has been implemented for affects.  This has the following
   * ramifications -
   * - Bonuses of the same type (other than Untyped, Dodge, Circumstance and Racial bonus
   *   types) OVERLAP.  They do not stack.  Effectively, the highest bonus is in
   *   effect at any one time.  If a bonus is NEGATIVE, that is, it is a penalty,
   *   then that penalty DOES stack.
   * - Display of affects becomes a bit problematic, since the bonus type means so much.
   *   It is important to display the bonus type, but we don't have a lot of room on the
   *   screen.
   *
   * Solution: (?)
   *   -----Spell-Like Affects---
   *   [Deflection]
   *   Affect name      +X to AC
   *   Affect name      +Y to AC            Where X > Y.  This line is a muted color vs above.
   *   [Enhancement Bonus]
   *   Bull's Strength  +4 to Strength
   *   Cat's Grace      +4 to Dexterity     These 2 lines are the same color since both apply.
   *
   * In order to implement this, we have to change how we process the effects, potentially
   * adding the affect descriptions to strings, one for each affect type, then concatenating
   * them together for the final display.
   *
   */
  /* Routine to show what spells a char is affected by */
  if (k->affected)
  {
    for (aff = k->affected; aff; aff = aff->next)
    {
      if (aff->duration + 1 >= 900)
      { // how many rounds in an hour?
        snprintf(buf, sizeof(buf), "[%2d hour%s   ] ", (int)((aff->duration + 1) / 900), ((int)((aff->duration + 1) / 900) > 1 ? "s" : " "));
      }
      else if (aff->duration + 1 >= 15)
      { // how many rounds in a minute?
        snprintf(buf, sizeof(buf), "[%2d minute%s ] ", (int)((aff->duration + 1) / 15), ((int)((aff->duration + 1) / 15) > 1 ? "s" : " "));
      }
      else
      { // rounds
        snprintf(buf, sizeof(buf), "[%2d round%s  ] ", (aff->duration + 1), ((aff->duration + 1) > 1 ? "s" : " "));
      }

      /* name */
      snprintf(buf2, sizeof(buf2), "%s%-25s%s ",
               CCCYN(ch, C_NRM), spell_info[aff->spell].name, CCNRM(ch, C_NRM));
      strlcat(buf, buf2, sizeof(buf));

      buf2[0] = '\0';

      if (aff->location == APPLY_DR)
      { /* Handle DR a bit differently */
        snprintf(buf3, sizeof(buf3), "%s", "(see DR)");
      }
      else if (aff->location == APPLY_SKILL)
      {
        snprintf(buf3, sizeof(buf3), "%+d to %s (%s)", aff->modifier, apply_types[(int)aff->location], ability_names[aff->specific]);
      }
      else if (aff->location >= APPLY_SPELL_CIRCLE_1 && aff->location <= APPLY_SPELL_CIRCLE_9)
      {
        snprintf(buf3, sizeof(buf3), "%+d to %s (%s)", aff->modifier, apply_types[(int)aff->location], class_names[aff->specific]);
      }
      else if (aff->location == APPLY_SPELL_POTENCY || aff->location == APPLY_SPELL_DURATION)
      {
        snprintf(buf3, sizeof(buf3), "%+d%% to %s", aff->modifier, apply_types[(int)aff->location]);
      }
      else
      {
        snprintf(buf3, sizeof(buf3), "%+d to %s", aff->modifier, apply_types[(int)aff->location]);
      }

      if (aff->bitvector[0] || aff->bitvector[1] ||
          aff->bitvector[2] || aff->bitvector[3])
      {
        snprintf(buf2, sizeof(buf2), "%s (see affected by)", ((aff->modifier) ? ", " : ""));
        strlcat(buf3, buf2, sizeof(buf3));
      }

      buf2[0] = '\0';
      snprintf(buf2, sizeof(buf2), "%-34s", buf3);
      buf3[0] = '\0';
      /* Add the Bonus type. */
      send_to_char(ch, "%s %s \tc(%s)\tn\r\n", buf, buf2, bonus_types[aff->bonus_type]);
    } /* end for */
  }

  send_to_char(ch, "\tC");
  text_line(ch, "\tYOther Affects\tC", 90, '-', '-');
  send_to_char(ch, "\tn");

  /* Check to see if the victim is affected by an AURA OF COURAGE */
  if (has_aura_of_courage(ch))
  {
    send_to_char(ch, "Aura of Courage (bonus resistance against fear-affects)\r\n");
  }
  if (GET_NODAZE_COOLDOWN(ch) > 0)
    send_to_char(ch, "Temporary Daze Immunity.\r\n");

  if (affected_by_aura_of_cowardice(ch))
  {
    send_to_char(ch, "Aura of Cowardice (penalty against fear-affects)\r\n");
  }
  if ((pMudEvent = char_has_mud_event(k, eDANCINGWEAPON)))
  {
    send_to_char(ch, "Dancing Weapon (a weapon of force attacks your foe each round)\r\n");
  }
  if ((pMudEvent = char_has_mud_event(k, eSPIRITUALWEAPON)))
  {
    send_to_char(ch, "Spiritual Weapon (a weapon of force attacks your foe each round)\r\n");
  }
  /* salvation */
  if (CLASS_LEVEL(k, CLASS_CLERIC) >= 14)
  {
    if (PLR_FLAGGED(k, PLR_SALVATION))
    {
      if (GET_SALVATION_NAME(k) != NULL)
      {
        send_to_char(ch, "Salvation:  Set at %s\r\n", GET_SALVATION_NAME(k));
      }
      else
      {
        send_to_char(ch, "Salvation:  Not set.\r\n");
      }
    }
    else
    {
      send_to_char(ch, "Salvation:  Not set.\r\n");
    }
  }

  if ((pMudEvent = char_has_mud_event(k, eVANISH)))
    send_to_char(ch, "\tRVanished!\tn - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eINTIMIDATED)))
    send_to_char(ch, "\tRIntimidated!\tn - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eTAUNTED)))
    send_to_char(ch, "\tRTaunted!\tn - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eSTUNNED)))
    send_to_char(ch, "\tRStunned!\tn - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eACIDARROW)))
    send_to_char(ch, "\tRAcid Arrow!\tn - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eAQUEOUSORB)))
    send_to_char(ch, "\tRAqueous Orb!\tn - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eHOLYJAVELIN)))
    send_to_char(ch, "\tRHoly Javelin!\tn - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eIMPLODE)))
    send_to_char(ch, "\tRImplode!\tn - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));
  if ((pMudEvent = char_has_mud_event(k, eCONCUSSIVEONSLAUGHT)))
    send_to_char(ch, "\tRConcussive Onslaught!\tn - Duration: %d rounds\r\n", ch->player_specials->concussive_onslaught_duration);
  if ((pMudEvent = char_has_mud_event(k, eMOONBEAM)))
    send_to_char(ch, "\tRMoonbeam!\tn - Duration: %d seconds\r\n", (int)(event_time(pMudEvent->pEvent) / 10));

  if (vampire_last_feeding_adjustment(k) > 0)
    send_to_char(ch, "You have recently fed and receive special bonuses. See HELP RECENTLY FED.\r\n");
  if (vampire_last_feeding_adjustment(k) < 0)
    send_to_char(ch, "You are blood starved and receive penalties to some abilities.  See HELP BLOOD STARVED.\r\n");

  send_to_char(ch, "\tC");
  draw_line(ch, 90, '-', '-');
  send_to_char(ch, "\tn");

  /* leads to other commands */
  if (ch == k)
  {
    send_to_char(ch, "\tDType 'cooldowns' to see your cooldowns.\tn\r\n");
    send_to_char(ch, "\tDType 'resistances' to see your resistances and damage reduction.\tn\r\n");
    send_to_char(ch, "\tDType 'abilities' to see your class and innate abilities.\tn\r\n");
    send_to_char(ch, "\tDType 'damagereduction' to see your damage reduction breakdown.\tn\r\n");
  }
}

void free_history(struct char_data *ch, int type)
{
  struct txt_block *tmp = GET_HISTORY(ch, type), *ftmp;

  while ((ftmp = tmp))
  {
    tmp = tmp->next;
    if (ftmp->text)
      free(ftmp->text);
    free(ftmp);
  }
  GET_HISTORY(ch, type) = NULL;
}

#define HIST_LENGTH 100

void add_history(struct char_data *ch, const char *str, int type)
{
  int i = 0;
  char time_str[MAX_STRING_LENGTH] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'};
  struct txt_block *tmp;
  time_t ct;

  if (IS_NPC(ch))
    return;

  tmp = GET_HISTORY(ch, type);
  ct = time(0);
  strftime(time_str, sizeof(time_str), "%H:%M ", localtime(&ct));

  snprintf(buf, sizeof(buf), "%s%s", time_str, str);

  if (!tmp)
  {
    CREATE(GET_HISTORY(ch, type), struct txt_block, 1);
    GET_HISTORY(ch, type)->text = strdup(buf);
  }
  else
  {
    while (tmp->next)
      tmp = tmp->next;
    CREATE(tmp->next, struct txt_block, 1);
    tmp->next->text = strdup(buf);

    for (tmp = GET_HISTORY(ch, type); tmp; tmp = tmp->next, i++)
      ;

    for (; i > HIST_LENGTH && GET_HISTORY(ch, type); i--)
    {
      tmp = GET_HISTORY(ch, type);
      GET_HISTORY(ch, type) = tmp->next;
      if (tmp->text)
        free(tmp->text);
      free(tmp);
    }
  }
  /* add this history message to ALL */
  if (type != HIST_ALL)
    add_history(ch, str, HIST_ALL);
}

void list_scanned_chars(struct char_data *list, struct char_data *ch, int distance, int door)
{
  char buf[MAX_STRING_LENGTH] = {'\0'}, buf2[MAX_STRING_LENGTH] = {'\0'};

  const char *const how_far[] = {
      "close by",
      "a ways off",
      "far off to the"};

  struct char_data *i;
  int count = 0;
  *buf = '\0';

  /* this loop is a quick, easy way to help make a grammatical sentence
     (i.e., "You see x, x, y, and z." with commas, "and", etc.) */

  for (i = list; i; i = i->next_in_room)

    /* put any other conditions for scanning someone in this if statement -
       i.e., if (CAN_SEE(ch, i) && condition2 && condition3) or whatever */

    if (CAN_SEE(ch, i))
      count++;

  if (!count)
    return;

  for (i = list; i; i = i->next_in_room)
  {

    /* make sure to add changes to the if statement above to this one also, using
       or's to join them.. i.e.,
       if (!CAN_SEE(ch, i) || !condition2 || !condition3) */

    if (!CAN_SEE(ch, i))
      continue;
    if (!*buf)
      snprintf(buf, sizeof(buf), "You see %s", GET_NAME(i));
    else
      strlcat(buf, GET_NAME(i), sizeof(buf));
    if (--count > 1)
      strlcat(buf, ", ", sizeof(buf));
    else if (count == 1)
      strlcat(buf, " and ", sizeof(buf));
    else
    {
      snprintf(buf2, sizeof(buf2), " %s %s.\r\n", how_far[distance], dirs[door]);
      strlcat(buf, buf2, sizeof(buf));
    }
  }
  send_to_char(ch, "%s", buf);
}

/*****  End Utility Functions *****/

/****  Commands ACMD ******/

ACMD(do_masterlist)
{
  size_t len = 0, nlen = 0;
  int bottom = 1, top = TOP_SKILL_DEFINE, counter = 0, i = 0;
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  const char *overflow = "\r\n**OVERFLOW**\r\n";
  bool is_spells = FALSE;

  if (IS_NPC(ch))
    return;

  skip_spaces_c(&argument);

  if (!argument || !*argument)
  {
    send_to_char(ch, "Specify 'spells' or 'skills' list.\r\n");
    return;
  }

  if (is_abbrev(argument, "skills"))
  {
    is_spells = FALSE;
  }
  else if (is_abbrev(argument, "spells"))
  {
    is_spells = TRUE;
  }
  else
  {
    send_to_char(ch, "Specify 'spells' or 'skills' list.\r\n");
    return;
  }

  len = snprintf(buf2, sizeof(buf2), "\tCMaster List\tn\r\n");

  for (; bottom < top; bottom++)
  {
    i = spell_sort_info[bottom]; /* make sure spell_sort_info[] define is big enough! */

    if (spell_info[i].min_position == POS_DEAD)
      continue;
    if (is_spells && i > NUM_SPELLS)
      continue;
    if (!is_spells && i < START_SKILLS)
      continue;
    if (!is_spells && i > TOP_SKILL_DEFINE)
      continue;

    nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                    "%3d) %s\r\n", i, spell_info[i].name);

    if (len + nlen >= sizeof(buf2) || nlen < 0)
      break;

    len += nlen;
    counter++;

    /* debugging this issue */
    /*
    if (counter >= 7000000)
    {
      send_to_char(ch, "error, report to staff masterlist001\r\n");
      break;
    }
    */
  }
  nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                  "\r\n\tCTotal:\tn  %d\r\n", counter);

  /* strcpy: OK */
  if (len >= sizeof(buf2))
    strlcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow, sizeof(buf2 + sizeof(buf2) - strlen(overflow) - 1));

  page_string(ch->desc, buf2, TRUE);
}

ACMD(do_look)
{
  struct obj_data *tmp_object;
  struct char_data *tmp_char;
  int look_type;
  int found = 0;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char(ch, "You can't see anything but stars!\r\n");
  else if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT && !has_blindsense(ch))
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
  else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) && !CAN_INFRA_IN_DARK(ch))
  {
    send_to_char(ch, "It is pitch black...\r\n");
    list_char_to_char(world[IN_ROOM(ch)].people, ch); /* glowing red eyes */
  }
  else
  {
    char arg[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};

    half_chop_c(argument, arg, sizeof(arg), arg2, sizeof(arg2));

    if (subcmd == SCMD_READ)
    {
      if (!*arg)
        send_to_char(ch, "Read what?\r\n");
      else
      {
        generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
        if (tmp_object)
          look_at_target(ch, arg);
        else
          do_examine(ch, argument, cmd, subcmd);
      }
      return;
    }
    else if (subcmd == SCMD_HERE)
    {
      list_obj_to_char(world[IN_ROOM(ch)].contents, ch, SHOW_OBJ_LONG, FALSE, 0);
      list_char_to_char(world[IN_ROOM(ch)].people, ch);
      return;
    }
    if (!*arg) /* "look" alone, without an argument at all */
      look_at_room(ch, 1);
    else if (is_abbrev(arg, "in"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, dirs_short, FALSE)) >= 0)
      look_in_direction(ch, look_type);
    else if (is_abbrev(arg, "at"))
      look_at_target(ch, arg2);
    else if (is_abbrev(arg, "out"))
      ship_lookout(ch);
    else if (is_abbrev(arg, "around"))
    {
      struct extra_descr_data *i;

      for (i = world[IN_ROOM(ch)].ex_description; i; i = i->next)
      {
        if (*i->keyword != '.')
        {
          send_to_char(ch, "%s%s:\r\n%s",
                       (found ? "\r\n" : ""), i->keyword, i->description);
          found = 1;
        }
      }
      if (!found)
        send_to_char(ch, "You couldn't find anything noticeable.\r\n");
    }
    else
      look_at_target(ch, arg);
  }
}

ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  char tempsave[MAX_INPUT_LENGTH] = {'\0'}, arg[MAX_INPUT_LENGTH] = {'\0'};

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Examine what?\r\n");
    return;
  }

  strlcpy(tempsave, arg, sizeof(tempsave));

  /* look_at_target() eats the number. */
  look_at_target(ch, tempsave);

  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object)
  {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
        (GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
        (GET_OBJ_TYPE(tmp_object) == ITEM_AMMO_POUCH) ||
        (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER))
    {
      send_to_char(ch, "When you look inside, you see:\r\n");
      look_in_obj(ch, arg);
    }
  }
}

/* command to check your stat caps! */
ACMD(do_statcap)
{
  compute_char_cap(ch, 1);
}

/* commnand to check your gold balance! */
ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char(ch, "You're broke!\r\n");
  else if (GET_GOLD(ch) == 1)
    send_to_char(ch, "You have one miserable little gold coin.\r\n");
  else
    send_to_char(ch, "You have %d gold coins.\r\n", GET_GOLD(ch));
}

/* Name: do_abilities
 * Author: Jamie Mclaughlin (Ornir)
 * Desc: This procedure displays the abilities of the character, both racial and
 *       class.  If an ability has a limited number of uses, both the current
 *       remaining number of uses and the maximum number of uses are displayed.
 *       the time remaining to regenerate another use of the ability is
 *       displayed using the 'cooldown' command, although there is a possibility
 *       that this may be a better place to display that information.
 *
 *       (The figure below is adjusted and is not exactly to scale.)
 *
 *       --------------------------- Abilities ---------------------------------
 *       Ability name  (Ability Type)(uses remaining)/(max uses) uses remaining.
 *       Ability name  (Ability Type)(description of static bonus)
 *       -----------------------------------------------------------------------
 */
void perform_abilities(struct char_data *ch, struct char_data *k)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  int line_length = 90;
  int i = 0, remaining = 0, total = 0;

  /* Set up the output. */
  send_to_char(ch, "\tC");
  text_line(ch, "\tYAbilities\tC", line_length, '-', '-');
  send_to_char(ch, "\tn");

  for (i = 0; i < NUM_FEATS; i++)
  {
    if (HAS_FEAT(k, i) && is_daily_feat(i))
    {
      snprintf(buf, sizeof(buf), "%s", feat_types[feat_list[i].feat_type]);
      remaining = daily_uses_remaining(k, i);
      total = get_daily_uses(k, i);
      send_to_char(ch,
                   "%-30s \tc%-14s\tn %s%2d\tn/%-2d uses remaining\r\n",
                   feat_list[i].name,
                   buf,
                   (remaining > (total / 2) ? "\tn" : (remaining <= 1 ? "\tR" : "\tY")),
                   remaining,
                   total);
    }
    buf[0] = '\0';
  }

  list_item_activate_abilities(ch);

  /* Close the output, reset the colors to prevent bleed. */
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn");

  /* leads to related commands */
  if (ch == k)
  {
    send_to_char(ch, "\tDType 'cooldowns' to see your cooldowns.\tn\r\n");
    send_to_char(ch, "\tDType 'resistances' to see your resistances and damage reduction.\tn\r\n");
    send_to_char(ch, "\tDType 'affects' to see your affects and conditions.\tn\r\n");
  }
}

/* see your abilities */
ACMD(do_abilities)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  one_argument(argument, arg, sizeof(arg));

  /* find the victim */
  vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);

  /* needs to be a group member or it won't work */
  if (!vict)
  {
    vict = ch;
  }
  else if (!GROUP(ch) || !GROUP(vict))
  {
    vict = ch;
  }
  else if (GROUP(ch) != GROUP(vict))
  {
    vict = ch;
  }

  /* moved this into a function so we can share it with do_stat() */
  perform_abilities(ch, vict);
}

/* this is really deprecated */
ACMD(do_innates)
{
  int race = -1;

  send_to_char(ch, "\tmAbility\tn Innates:\r\n\r\n");
  send_to_char(ch, "\tMRacial:\tn\r\n");
  if (IS_NPC(ch) || IS_MORPHED(ch))
  {
    if (IS_MORPHED(ch))
      race = IS_MORPHED(ch);
    else
      race = GET_RACE(ch);
    switch (race)
    {
    case RACE_TYPE_DRAGON:
      send_to_char(ch, "tailsweep\r\n");
      send_to_char(ch, "breathe\r\n");
      send_to_char(ch, "frightful\r\n");
      break;
    case RACE_TYPE_ANIMAL:
      send_to_char(ch, "rage\r\n");
      break;
    default:
      send_to_char(ch, "None (yet)\r\n");
      break;
    }
  }
  else
  { // PC
    switch (GET_RACE(ch))
    {
    case RACE_ELF:
      send_to_char(ch, "elven dexterity (+2 dex)\r\n");
      send_to_char(ch, "\tRelven constitution (-2 con)\tn\r\n");
      send_to_char(ch, "basic weapon proficiency (free skill)\r\n");
      send_to_char(ch, "sleep enchantment immunity\r\n");
      send_to_char(ch, "infravision\r\n");
      send_to_char(ch, "keen senses (+2 listen/spot ability)\r\n");
      send_to_char(ch, "resistance to enchantments (+2 save bonus)\r\n");
      break;
    case RACE_DWARF:
      send_to_char(ch, "dwarven constitution (+2 con)\r\n");
      send_to_char(ch, "\tRdwarven charisma (-2 cha)\tn\r\n");
      send_to_char(ch, "infravision\r\n");
      send_to_char(ch, "poison resist (+2 poison save)\r\n");
      send_to_char(ch, "stability (+4 resist bash/trip)\r\n");
      send_to_char(ch, "spell hardiness (+2 spell save vs. "
                       "damaging spells)\r\n");
      send_to_char(ch, "combat training versus giants "
                       "(+1 size bonus vs. larger opponents)\r\n");
      break;
    case RACE_H_ELF:
      send_to_char(ch, "basic weapon proficiency (free skill)\r\n");
      send_to_char(ch, "infravision\r\n");
      send_to_char(ch, "resistance to enchantments (+2 save bbnus)\r\n");
      send_to_char(ch, "half-blood (+2 discipline/lore)");
      break;
    case RACE_H_ORC:
      send_to_char(ch, "half orc strength (+2 str)\r\n");
      send_to_char(ch, "\tRhalf orc charisma (-2 cha)\tn\r\n");
      send_to_char(ch, "\tRhalf orc intelligence (-2 int)\tn\r\n");
      send_to_char(ch, "ultravision\r\n");
      break;
    case RACE_HALFLING:
      send_to_char(ch, "halfling dexterity (+2 dex)\r\n");
      send_to_char(ch, "\tRhalfling strength (-2 str)\tn\r\n");
      send_to_char(ch, "infravision\r\n");
      send_to_char(ch, "shadow hopper (+2 sneak/hide)\r\n");
      send_to_char(ch, "lucky (+1 all saves)\r\n");
      send_to_char(ch, "infravision\r\n");
      send_to_char(ch, "combat training versus giants "
                       "(+1 size bonus vs. larger opponents)\r\n");
      break;
    case RACE_GNOME:
      send_to_char(ch, "gnomish constitution (+2 con)\r\n");
      send_to_char(ch, "\tRgnomish strength (-2 str)\tn\r\n");
      send_to_char(ch, "illusion resist (+2 save bonus)\r\n");
      send_to_char(ch, "illusion affinity (+2 DC on illusions)\r\n");
      send_to_char(ch, "tinker focus (+2 concentration/listen)\r\n");
      send_to_char(ch, "infravision\r\n");
      send_to_char(ch, "combat training versus giants "
                       "(+1 size bonus vs. larger opponents)\r\n");
      break;
    case RACE_HUMAN:
      send_to_char(ch, "diverse (+3 training sessions, 1st level)\r\n");
      send_to_char(ch, "quick learner (+1 training per level)\r\n");
      send_to_char(ch, "well trained (+1 practice session, 1st level)\r\n");
      break;
    case RACE_TRELUX:
      send_to_char(ch, "vital (start with +10 hps bonus)\r\n");
      send_to_char(ch, "hardy (+4 hps bonus per level)\r\n");
      send_to_char(ch, "trelux constitution (+4 con)\r\n");
      send_to_char(ch, "trelux strength (+2 str)\r\n");
      send_to_char(ch, "trelux dexterity (+8 dex)\r\n");
      send_to_char(ch, "trelux small size\r\n");
      send_to_char(ch, "ultravision\r\n");
      send_to_char(ch, "\tRvulnerable cold (20%%)\tn\r\n");
      send_to_char(ch, "resist everything else (20%%)\r\n");
      send_to_char(ch, "leap (help LEAP)\r\n");
      send_to_char(ch, "fly (help TRELUX-FLY)\r\n");
      send_to_char(ch, "\tRtrelux eq (help TRELUX-EQ)\tn\r\n");
      send_to_char(ch, "trelux exoskeleton (help TRELUX-EXOSKELETON)\r\n");
      send_to_char(ch, "trelux pincers (help TRELUX-PINCERS)\r\n");
      send_to_char(ch, "insectbeing (help INSECTBEING)\r\n");

      break;
    case RACE_CRYSTAL_DWARF:
      send_to_char(ch, "vital (start with +10 hps bonus)\r\n");
      send_to_char(ch, "hardy (+4 hps bonus per level)\r\n");
      send_to_char(ch, "resist acid (10%%)\r\n");
      send_to_char(ch, "resist puncture (10%%)\r\n");
      send_to_char(ch, "resist poison (10%%)\r\n");
      send_to_char(ch, "resist disease (10%%)\r\n");
      send_to_char(ch, "crystal dwarf constitution (+8 con)\r\n");
      send_to_char(ch, "crystal dwarf strength (+2 str)\r\n");
      send_to_char(ch, "crystal dwarf wisdom (+2 wis)\r\n");
      send_to_char(ch, "crystal dwarf charisma (+2 cha)\r\n");
      send_to_char(ch, "infravision\r\n");
      send_to_char(ch, "poison resist (+2 poison save)\r\n");
      send_to_char(ch, "stability (+4 resist bash/trip)\r\n");
      send_to_char(ch, "spell hardiness (+2 spell save)\r\n");
      send_to_char(ch, "crystalbody (help CRYSTALBODY)\r\n");
      send_to_char(ch, "crystalfist (help CRYSTALFIST)\r\n");
      send_to_char(ch, "combat training versus giants "
                       "(+1 size bonus vs. larger opponents)\r\n");
      break;
    case RACE_HALF_TROLL:
      send_to_char(ch, "regeneration\r\n");
      send_to_char(ch, "\tRweakness to acid (25%%)\tn\r\n");
      send_to_char(ch, "\tRweakness to fire (50%%)\tn\r\n");
      send_to_char(ch, "resist poison (25%%)\r\n");
      send_to_char(ch, "resist disease (50%%)\r\n");
      send_to_char(ch, "troll constitution (+2 con)\r\n");
      send_to_char(ch, "troll strength (+2 str)\r\n");
      send_to_char(ch, "troll dexterity (+2 dex)\r\n");
      send_to_char(ch, "\tRtroll charisma (-4 cha)\tn\r\n");
      send_to_char(ch, "\tRtroll intelligence (-4 cha)\tn\r\n");
      send_to_char(ch, "\tRtroll wisdom (-4 cha)\tn\r\n");
      send_to_char(ch, "ultravision\r\n");
      break;
    case RACE_ARCANA_GOLEM:
      send_to_char(ch, "\tRarcana golem spell vulnerability "
                       "(-2 penalty against damaging spell saves)\tn\r\n");
      send_to_char(ch, "\tRarcana golem enchantment vulnerability "
                       "(-2 penalty against enchantment saves)\tn\r\n");
      send_to_char(ch, "\tRarcana golem constitution (-2 con)\tn\r\n");
      send_to_char(ch, "\tRarcana golem strength (-2 str)\tn\r\n");
      send_to_char(ch, "arcana intelligence (+2 int)\r\n");
      send_to_char(ch, "arcana wisdom (+2 wis)\r\n");
      send_to_char(ch, "arcana charisma (+2 cha)\r\n");
      send_to_char(ch, "Magical Heritage (a 6th of level bonus to: "
                       "caster-level, concentration and spellcraft\r\n");
      send_to_char(ch, "spellbattle (help SPELLBATTLE)\r\n");
      break;
    case RACE_DROW:
      send_to_char(ch, "sleep enchantment immunity\r\n");
      send_to_char(ch, "darkvision\r\n");
      send_to_char(ch, "keen senses (+2 listen/spot ability)\r\n");
      send_to_char(ch, "resistance to enchantments (+2 save bonus)\r\n");
      send_to_char(ch, "spell resistance (10 + level)\r\n");
      send_to_char(ch, "light blindness - -1 to hitroll, damroll, saves and "
                       "skill checks when outdoors during the day, darkness spells and "
                       "effects negate this penalty\r\n");
      send_to_char(ch, "drow weapon proficiency - hand-crossbow, rapier, and short-swords\r\n");
      send_to_char(ch, "drow spell-like ability 3/day: faerie fire\r\n");
      send_to_char(ch, "drow spell-like ability 3/day: levitate\r\n");
      send_to_char(ch, "drow spell-like ability 3/day: darkness\r\n");
      send_to_char(ch, "drow intelligence (+2 int)\r\n");
      send_to_char(ch, "drow wisdom (+2 wis)\r\n");
      send_to_char(ch, "drow charisma (+2 cha)\r\n");
      send_to_char(ch, "drow weak constitution (-2 con)\r\n");
      break;
    case RACE_DUERGAR:
      send_to_char(ch, "\tRlight blindness\tn - -1 to hitroll, damroll, saves and "
                       "skill checks when outdoors during the day, darkness spells and "
                       "effects negate this penalty\r\n");
      send_to_char(ch, "darkvision\r\n");
      send_to_char(ch, "\tRduergar charisma (-2 cha)\tn\r\n");
      send_to_char(ch, "duergar constitution (+4 con)\r\n");
      send_to_char(ch, "poison resist (+2 poison save)\r\n");
      send_to_char(ch, "strong phantasm resist (+4 phantasm saves)\r\n");
      send_to_char(ch, "strong paralysis resist (+4 paralysis saves)\r\n");
      send_to_char(ch, "stability (+4 resist bash/trip)\r\n");
      send_to_char(ch, "spell hardiness (+4 spell save vs. "
                       "damaging spells)\r\n");
      send_to_char(ch, "combat training versus giants "
                       "(+1 size bonus vs. larger opponents)\r\n");
      send_to_char(ch, "proficiency with dwarven waraxes\r\n");
      send_to_char(ch, "Skill Affinity (Move Silently): +2 racial bonus to move silently\r\n");
      send_to_char(ch, "Partial Skill Affinity (Listen): +1 racial bonus on listen checks\r\n");
      send_to_char(ch, "Partial Skill Affinity (Spot): +1 racial bonus on spot checks\r\n");
      send_to_char(ch, "spell-like ability 3/day: invisibility\r\n");
      send_to_char(ch, "spell-like ability 3/day: enlarge\r\n");
      send_to_char(ch, "spell-like ability 3/day: strength\r\n");
      break;
    case RACE_LICH:
      send_to_char(ch, "vital (start with +10 hps bonus)\r\n");
      send_to_char(ch, "hardy (+4 hps bonus per level)\r\n");
      send_to_char(ch, "lich constitution (+2 con)\r\n");
      send_to_char(ch, "lich dexterity (+2 dex)\r\n");
      send_to_char(ch, "lich intelligence (+6 int)\r\n");
      send_to_char(ch, "armor skin +5\r\n");
      send_to_char(ch, "ultravision\r\n");
      send_to_char(ch, "is undead\r\n");
      send_to_char(ch, "spell resist 15 + level\r\n");
      send_to_char(ch, "damage resist 4\r\n");
      send_to_char(ch, "immunity cold\r\n");
      send_to_char(ch, "immunity electricity\r\n");
      send_to_char(ch, "lich touch\r\n");
      send_to_char(ch, "rejuvenation\r\n");
      send_to_char(ch, "fear aura\r\n");
      send_to_char(ch, "unarmed combat\r\n");
      send_to_char(ch, "improve unarmed combat\r\n");
      send_to_char(ch, "+8 racial bonus on Perception, Sense Motive, and Stealth checks\r\n");
      break;
    default:
      send_to_char(ch, "No Racials (yet)\r\n");
      break;
    }

    /* other innates */
    send_to_char(ch, "\r\n");
    if (CLASS_LEVEL(ch, CLASS_BERSERKER) >= 4)
    {
      send_to_char(ch, "Berserker Innates:\r\n");
      send_to_char(ch, "berserker shrug (level / 4 damage reduction)\r\n");
    }
  }
}

/* compartmentalized affects, so wizard command (stat affect)
 *  and this can share */
ACMD(do_affects)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  one_argument(argument, arg, sizeof(arg));

  /* find the victim */
  vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);

  /* needs to be a group member or it won't work */
  if (!vict)
  {
    vict = ch;
  }
  else if (!GROUP(ch) || !GROUP(vict))
  {
    vict = ch;
  }
  else if (GROUP(ch) != GROUP(vict))
  {
    vict = ch;
  }

  if (subcmd == SCMD_AFFECTS)
  {
    perform_affects(ch, vict);
  }
  else if (subcmd == SCMD_COOLDOWNS)
    perform_cooldowns(ch, vict);
  else if (subcmd == SCMD_RESISTANCES)
    perform_resistances(ch, vict);
  else if (subcmd == SCMD_DAMAGE_REDUCTION)
    perform_damage_reduction(ch, vict);
  else
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: Invalid subcmd sent to do_affects: %d", subcmd);
}

ACMD(do_damage)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int mode = -1, attack_type = -1;
  int line_length = 80;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    /* show cmb/d info */
    send_to_char(ch, "Combat Maneuver Bonus: %d, Combat Maneuver Defense: %d.\r\n\r\n",
                 compute_cmb(ch, 0), compute_cmd(ch, 0));
#define DISPLAY_ROUTINE_POTENTIAL 2
    perform_attacks(ch, DISPLAY_ROUTINE_POTENTIAL, 0);
#undef DISPLAY_ROUTINE_POTENTIAL
    send_to_char(ch, "\tC");
    text_line(ch, "\tYTo view bonus breakdown: \tC", line_length, '-', '-');
    text_line(ch, "\tYattacks unarmed|primary|offhand|ranged|bomb|psionic\tC", line_length, '-', '-');
    text_line(ch, "\tYattacks primary-sneak|offhand-sneak|eldritch|twohand|evolution\tC", line_length, '-', '-');
    text_line(ch, "\tYdamage hit|primary|offhand|ranged\tC", line_length, '-', '-');
    send_to_char(ch, "\tn");

    return;
  }
  else if (is_abbrev(arg, "hit"))
  {
    mode = MODE_NORMAL_HIT;
    attack_type = ATTACK_TYPE_UNARMED;
  }
  else if (is_abbrev(arg, "primary"))
  {
    mode = MODE_DISPLAY_PRIMARY;
    attack_type = ATTACK_TYPE_PRIMARY;
  }
  else if (is_abbrev(arg, "offhand"))
  {
    mode = MODE_DISPLAY_OFFHAND;
    attack_type = ATTACK_TYPE_OFFHAND;
  }
  else if (is_abbrev(arg, "ranged"))
  {
    mode = MODE_DISPLAY_RANGED;
    attack_type = ATTACK_TYPE_RANGED;
  }
  else
  {
    send_to_char(ch, "Valid arguments: hit/primary/offhand/ranged.\r\n");
    return;
  }

  struct char_data *attacker = FIGHTING(ch);

  send_to_char(ch, "\tC");
  text_line(ch, "\tYDamage\tC", line_length, '-', '-');
  send_to_char(ch, "\tn");

  /* sending -1 for w_type will signal display mode */
  compute_damage_bonus(ch, attacker, NULL, -1, 0, mode, attack_type);

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn\r\n");
}

ACMD(do_attacks)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int attack_type = -1;
  int line_length = 80;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    /* show cmb/d info */
    send_to_char(ch, "Combat Maneuver Bonus: %d, Combat Maneuver Defense: %d.\r\n\r\n",
                 compute_cmb(ch, 0), compute_cmd(ch, 0));
#define DISPLAY_ROUTINE_POTENTIAL 2
    perform_attacks(ch, DISPLAY_ROUTINE_POTENTIAL, 0);
#undef DISPLAY_ROUTINE_POTENTIAL
    send_to_char(ch, "\tC");
    text_line(ch, "\tYTo view bonus breakdown: \tC", line_length, '-', '-');
    text_line(ch, "\tYattacks unarmed|primary|offhand|ranged|bomb|psionic\tC", line_length, '-', '-');
    text_line(ch, "\tYattacks primary-sneak|offhand-sneak|eldritch|twohand|evolution\tC", line_length, '-', '-');
    text_line(ch, "\tYdamage hit|primary|offhand|ranged\tC", line_length, '-', '-');
    send_to_char(ch, "\tn");

    return;
  }
  else if (is_abbrev(arg, "unarmed"))
  {
    attack_type = ATTACK_TYPE_UNARMED;
  }
  else if (is_abbrev(arg, "primary"))
  {
    attack_type = ATTACK_TYPE_PRIMARY;
  }
  else if (is_abbrev(arg, "offhand"))
  {
    attack_type = ATTACK_TYPE_OFFHAND;
  }
  else if (is_abbrev(arg, "primary-sneak"))
  {
    attack_type = ATTACK_TYPE_PRIMARY_SNEAK;
  }
  else if (is_abbrev(arg, "offhand-sneak"))
  {
    attack_type = ATTACK_TYPE_OFFHAND_SNEAK;
  }
  else if (is_abbrev(arg, "ranged"))
  {
    attack_type = ATTACK_TYPE_RANGED;
  }
  else if (is_abbrev(arg, "bomb"))
  {
    attack_type = ATTACK_TYPE_BOMB_TOSS;
  }
  else if (is_abbrev(arg, "psionic"))
  {
    attack_type = ATTACK_TYPE_PSIONICS;
  }
  else if (is_abbrev(arg, "eldritch"))
  {
    attack_type = ATTACK_TYPE_ELDRITCH_BLAST;
  }
  else if (is_abbrev(arg, "twohand"))
  {
    attack_type = ATTACK_TYPE_TWOHAND;
  }
  else if (is_abbrev(arg, "evolution"))
  {
    attack_type = ATTACK_TYPE_PRIMARY_EVO_BITE;
  }
  else
  {
    send_to_char(ch, "Valid arguments: unarmed/primary/offhand/ranged/bomb/psionic/primary-sneak/offhand-sneak/eldritch/twohand/evolution.\r\n");
    return;
  }

  struct char_data *attacker = FIGHTING(ch);

  /* sending -1 for w_type will signal display mode */
  compute_attack_bonus_full(ch, attacker, attack_type, true);

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn\r\n");
}

ACMD(do_defenses)
{
  struct char_data *attacker = FIGHTING(ch);
  int line_length = 80;

  send_to_char(ch, "\tC");
  text_line(ch, "\tYDefenses\tC", line_length, '-', '-');
  send_to_char(ch, "\tn");

  compute_armor_class(attacker, ch, FALSE, MODE_ARMOR_CLASS_DISPLAY);

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn\r\nNote that AC caps at %d, but having over %d is beneficial due to position changes and debuffs.\r\n", CONFIG_PLAYER_AC_CAP, CONFIG_PLAYER_AC_CAP);
  text_line(ch, "\tYFast Healing\tC", line_length, '-', '-');
  send_to_char(ch, "Fast Healing Amount: %d\r\n", get_fast_healing_amount(ch));
send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn");
}

/**
 * @file act.informative.c
 * @brief Implementation of the 'score' command
 * 
 * COMMAND: score
 * USAGE: score
 * 
 * The score command displays comprehensive character information including:
 *  - Basic identity (name, title, race, class, etc)
 *  - Vital statistics (HP, movement, speed)
 *  - Experience and level progression
 *  - Ability scores and saving throws
 *  - Combat statistics (BAB, AC, position)
 *  - Special abilities (psionics, spellcasting)
 *  - Quest information
 *  - Wealth (gold carried and banked)
 *  - Class-specific information (domains, bloodlines, schools)
 *  - Status conditions (hunger, thirst, intoxication)
 *  - Available class-specific commands
 * 
 * Example output:
 * -------------------------------Score Information--------------------------------
 * Name      : Leonidas             Title   : the distracted do-gooder
 * Alignment : Lawful Good          Classes : 1 War / 1 Pal
 * Race      : Humn                 Sex     : Male
 * Age       : 18 yrs / 0 mths      Played  : 1 days / 0 hrs
 * Size      : Medium               Load    : 41/920 lbs
 * --------------------------------------------------------------------------------
 * Hit points: 38(38)    Moves: 84(84)    Speed: 30    Initiative: +1
 * ----------------------------------Experience------------------------------------
 * Level: 2                          CstrLvl : 0   DivLvl: 0   MgcLvl: 0
 * Exp  : 2000                       ExpTNL  : 9000
 * -------------Ability Scores--------------------------Saving Throws--------------
 * Str: 16[ 3]  Dex: 12[ 1]  Con: 12[ 1]  |  Fort    : 2    Will    : 2
 * Int: 12[ 1]  Wis: 12[ 1]  Cha: 12[ 1]  |  Reflex  : 2
 * -------------------------------------Combat-------------------------------------
 * BAB: 2    # of Attacks: 1    ArmorClass: 10    Wimpy: 0    Pos: Standing
 * --------------------------------------------------------------------------------
 * Gold: 999615                      Gold in Bank : 0
 * --------------------------------------------------------------------------------
 * 
 * @param ch The character executing the command
 * @param argument Command arguments (unused - score takes no arguments)
 * @param cmd The command number
 * @param subcmd Subcommand flags (unused)
 */
ACMD(do_score)
{
  /* ========================================================================= */
  /* VARIABLE DECLARATIONS                                                     */
  /* ========================================================================= */
  
  /* String buffers for building output */
  char buf[MAX_INPUT_LENGTH] = {'\0'};        /* General purpose string buffer */
  char dname[SMALL_STRING] = {'\0'};          /* Deity name buffer */
  
  /* Character statistics and information */
  struct time_info_data playing_time;         /* Time played calculation */
  int calc_bab = 0;                           /* Base Attack Bonus */
  int i = 0, counter = 0;                     /* Loop counters */
  
  /* Display formatting constants */
  const int line_length = 80;                 /* Standard line width */
  
  /* ========================================================================= */
  /* INITIALIZATION - Prepare data for display                                 */
  /* ========================================================================= */
  
  /* Calculate Base Attack Bonus (capped at MAX_BAB) */
  calc_bab = MIN(MAX_BAB, ACTUAL_BAB(ch));
  
  /* Get primary wielded weapon (moved to local scope where needed) */
  /* wielded = GET_EQ(ch, WEAR_WIELD_1); */
  
  /* Determine weapon type (moved to local scope where needed) */
  /* if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  } else {
    if (IS_NPC(ch) && ch->mob_specials.attack_type != 0) {
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    } else {
      w_type = TYPE_HIT;
    }
  } */
  
  /* Calculate total time played */
  playing_time = *real_time_passed((time(0) - ch->player.time.logon) +
                                   ch->player.time.played, 0);
  
  /* Convert height to inches (moved to display_vitals_section) */
  /* height = GET_HEIGHT(ch) * 0.393700787402; */
  
  /* ========================================================================= */
  /* SECTION 1: BASIC IDENTITY INFORMATION                                     */
  /* ========================================================================= */
  
  /* Start with cyan color and draw header */
  send_to_char(ch, "\tC");
  text_line(ch, "\tYScore Information\tC", line_length, '-', '-');
  
  /* Display name and title */
  send_to_char(ch, "\tcName : \tn%-20s \tcTitle   : \tn%s\r\n",
               GET_NAME(ch), 
               GET_TITLE(ch) ? GET_TITLE(ch) : "None.");
  
#if defined(CAMPAIGN_DL)
  /* Display race with bounds checking */
  send_to_char(ch, "\tcRace : \tn%-20s ", 
               (GET_RACE(ch) >= DL_RACE_START && GET_RACE(ch) < DL_RACE_END) ? 
                 race_list[GET_RACE(ch)].type : "Unknown");
#else
  /* Display race with bounds checking */
  send_to_char(ch, "\tcRace : \tn%-20s ", 
               (GET_RACE(ch) >= 0 && GET_RACE(ch) < NUM_RACES) ? 
                 race_list[GET_RACE(ch)].type : "Unknown");
#endif

  /* Build class string - shows all classes for multiclass characters */
  *buf = '\0';
  counter = 0;
  
  if (!IS_NPC(ch)) {
    /* Player characters - check each possible class */
    for (i = 0; i < MAX_CLASSES; i++) {
      if (CLASS_LEVEL(ch, i)) {
        /* Add separator for multiple classes */
        if (counter) {
          strlcat(buf, " / ", sizeof(buf));
        }
        
        /* Add class level and abbreviation */
        char res_buf[32];
        snprintf(res_buf, sizeof(res_buf), "%d %s", 
                 CLASS_LEVEL(ch, i), CLSLIST_ABBRV(i));
        strlcat(buf, res_buf, sizeof(buf));
        counter++;
      }
    }
  } else {
    /* NPCs - use simple class abbreviation */
    strlcpy(buf, CLASS_ABBR(ch), sizeof(buf));
  }
  
  /* Override with premade build if applicable */
  if (GET_PREMADE_BUILD_CLASS(ch) != CLASS_UNDEFINED) {
    snprintf(buf, sizeof(buf), "%d %s (premade build)", 
             CLASS_LEVEL(ch, GET_PREMADE_BUILD_CLASS(ch)), 
             class_list[GET_PREMADE_BUILD_CLASS(ch)].name);
  }
  
  /* Display classes with proper pluralization */
  send_to_char(ch, "\tcClass%s : \tn%s\r\n", 
               (counter == 1 ? "  " : "es"), buf);
  
  /* Display sex/gender */
  send_to_char(ch, "\tcSex  : \tn%-20s ",
               (GET_SEX(ch) == SEX_MALE ? "Male" : 
                (GET_SEX(ch) == SEX_FEMALE ? "Female" : "Neutral")));
  
  /* Display deity with bounds checking */
  snprintf(dname, sizeof(dname), "%s", 
           (GET_DEITY(ch) >= 0 && GET_DEITY(ch) < NUM_DEITIES) ? 
             deity_list[GET_DEITY(ch)].name : "None");
  send_to_char(ch, "\tcDeity: \tn%-20s ", CAP(dname));
  
  /* Display alignment with numeric value */
  send_to_char(ch, "\tcAlignment : \tn%s (%d)\r\n", 
               get_align_by_num(GET_ALIGNMENT(ch)), GET_ALIGNMENT(ch));
  
  /* Display age category and time played */
  send_to_char(ch, "\tcAge  : \tn%-10s    \tcPlayed  : \tn%d days / %d hrs\r\n",
               (GET_CH_AGE(ch) >= 0 && GET_CH_AGE(ch) < NUM_CHARACTER_AGES) ? 
                 character_ages[GET_CH_AGE(ch)] : "Unknown", 
               playing_time.day, playing_time.hours);
  
  /* Display size and carrying capacity */
  send_to_char(ch, "\tcSize : \tn%-20s \tcLoad    : \tn%d\tc/\tn%d \tclbs \tcNum Items: \tn%d\tc/\tn%d \tn\r\n",
               (GET_SIZE(ch) >= 0 && GET_SIZE(ch) < NUM_SIZES) ? 
                 size_names[GET_SIZE(ch)] : "Unknown", 
               IS_CARRYING_W(ch), CAN_CARRY_W(ch), 
               IS_CARRYING_N(ch), CAN_CARRY_N(ch));

  /* ========================================================================= */
  /* SECTION 2: VITAL STATISTICS                                               */
  /* ========================================================================= */
  
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  
  /* Display health, movement, speed, and initiative */
  send_to_char(ch, "\tcHit points:\tn %d(%d)   \tcMoves:\tn %d(%d)   \tcSpeed:\tn %-3d   \tcInitiative:\tn %s%d\r\n",
               GET_HIT(ch), GET_MAX_HIT(ch),           /* Current/Max HP */
               GET_MOVE(ch), GET_MAX_MOVE(ch),         /* Current/Max Movement */
               get_speed(ch, TRUE),                     /* Movement speed */
               get_initiative_modifier(ch) >= 0 ? "+" : "",  /* + sign for positive */
               get_initiative_modifier(ch));            /* Initiative modifier */
  
  /* ========================================================================= */
  /* SECTION 3: EXPERIENCE AND LEVEL INFORMATION                              */
  /* ========================================================================= */
  
  send_to_char(ch, "\tC");
  text_line(ch, "\tyExperience\tC", line_length, '-', '-');
  
  /* Display character level and caster levels */
  send_to_char(ch, "\tcLevel : \tn%-2d                       "
               "\tcCstrLvl : \tn%-2d  \tcDivLvl : \tn%-2d  \tcMgcLvl : \tn%-2d\r\n",
               GET_LEVEL(ch),        /* Total character level */
               CASTER_LEVEL(ch),     /* Combined caster level */
               DIVINE_LEVEL(ch),     /* Divine spellcaster level */
               MAGIC_LEVEL(ch));     /* Arcane spellcaster level */
  
  /* Display experience points and experience to next level */
  send_to_char(ch, "\tcExp   : \tn%-24d \tcExpTNL  : \tn%d\r\n",
               GET_EXP(ch),          /* Current experience points */
               (GET_LEVEL(ch) >= LVL_IMMORT ? 0 :     /* Immortals don't need XP */
                level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch))); /* XP to next level */
  
  /* ========================================================================= */
  /* SECTION 4: ABILITY SCORES AND SAVING THROWS                              */
  /* ========================================================================= */
  
  send_to_char(ch, "\tC-------------\tyAbility Scores\tC--------------------------\tySaving Throws\tC--------------\r\n");
  
  /* Display ability scores with modifiers in brackets */
  send_to_char(ch, "\tcStr:\tn %2d[%2d]  \tcDex:\tn %2d[%2d]  \tcCon:\tn %2d[%2d]  \tC|  \tcFort    : \tn%-2d  \tcWill    : \tn%-2d\tn\r\n",
               GET_STR(ch), GET_STR_BONUS(ch),        /* Strength & modifier */
               GET_DEX(ch), GET_DEX_BONUS(ch),        /* Dexterity & modifier */
               GET_CON(ch), GET_CON_BONUS(ch),        /* Constitution & modifier */
               compute_mag_saves(ch, SAVING_FORT, 0), /* Fortitude save */
               compute_mag_saves(ch, SAVING_WILL, 0)); /* Will save */
  
  send_to_char(ch, "\tcInt:\tn %2d[%2d]  \tcWis:\tn %2d[%2d]  \tcCha:\tn %2d[%2d]  \tC|  \tcReflex  : \tn%-2d\tn\r\n",
               GET_INT(ch), GET_INT_BONUS(ch),        /* Intelligence & modifier */
               GET_WIS(ch), GET_WIS_BONUS(ch),        /* Wisdom & modifier */
               GET_CHA(ch), GET_CHA_BONUS(ch),        /* Charisma & modifier */
               compute_mag_saves(ch, SAVING_REFL, 0)); /* Reflex save */

  /* ========================================================================= */
  /* SECTION 5: COMBAT STATISTICS                                              */
  /* ========================================================================= */
  
  send_to_char(ch, "\tC");
  text_line(ch, "\tyCombat\tC", line_length, '-', '-');
  
  /* Display combat statistics */
#define RETURN_NUM_ATTACKS 1    /* Flag to get attack count instead of performing attacks */
  send_to_char(ch, "\tcBAB: \tn%-4d \tc# of Attacks: \tn%-3d \tcArmorClass: \tn%-4d \tcWimpy: \tn%-3d \tcPos: \tn",
               calc_bab,                                              /* Base Attack Bonus */
               perform_attacks(ch, RETURN_NUM_ATTACKS, 0),           /* Number of attacks per round */
               compute_armor_class(NULL, ch, FALSE, MODE_ARMOR_CLASS_NORMAL), /* Armor Class */
               GET_WIMP_LEV(ch));                                    /* Wimpy level (auto-flee HP) */
#undef RETURN_NUM_ATTACKS
  
  /* Add fighting indicator if in combat */
  if (FIGHTING(ch)) {
    send_to_char(ch, "(Fighting) - ");
  }
  
  /* Display current position with descriptive text */
  switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char(ch, "Dead\r\n");
      break;
      
    case POS_MORTALLYW:
      send_to_char(ch, "Mortally wounded\r\n");
      break;
      
    case POS_INCAP:
      send_to_char(ch, "Incapacitated\r\n");
      break;
      
    case POS_STUNNED:
      send_to_char(ch, "Stunned\r\n");
      break;
      
    case POS_SLEEPING:
      send_to_char(ch, "Sleeping\r\n");
      break;
      
    case POS_RECLINING:
      send_to_char(ch, "Prone\r\n");
      break;
      
    case POS_RESTING:
      send_to_char(ch, "Resting\r\n");
      break;
      
    case POS_SITTING:
      if (!SITTING(ch)) {
        /* Just sitting on ground */
        send_to_char(ch, "Sitting\r\n");
      } else {
        /* Sitting on furniture */
        struct obj_data *furniture = SITTING(ch);
        if (furniture && furniture->short_description) {
          send_to_char(ch, "Sitting upon %s.\r\n", furniture->short_description);
        } else {
          send_to_char(ch, "Sitting.\r\n");
        }
      }
      break;
      
    case POS_FIGHTING:
      send_to_char(ch, "Fighting\r\n");
      break;
      
    case POS_STANDING:
      send_to_char(ch, "Standing\r\n");
      break;
      
    default:
      send_to_char(ch, "Floating\r\n");
      break;
  }

  /* ========================================================================= */
  /* SECTION 6: SPECIAL ABILITIES (PSIONICS AND SPELLCASTING)                  */
  /* ========================================================================= */
  
  /* Display psionic information if character has psionic levels */
  if (GET_PSIONIC_LEVEL(ch) > 0) {
    text_line(ch, "\tyPsionic Info\tC", line_length, '-', '-');
    send_to_char(ch, "\tcPower Points:\tn %d(%d)   \tcPsionic Level:\tn %d   \tcEnergy Type:\tn %s\r\n"
                     "\tcMax Augment PSP:\tn %d - power psp cost\r\n",
                 GET_PSP(ch), GET_MAX_PSP(ch),                /* Current/Max power points */
                 GET_PSIONIC_LEVEL(ch),                        /* Psionic manifester level */
                 (GET_PSIONIC_ENERGY_TYPE(ch) >= 0 && 
                  GET_PSIONIC_ENERGY_TYPE(ch) < NUM_DAM_TYPES) ? 
                   damtypes[GET_PSIONIC_ENERGY_TYPE(ch)] : "Unknown", /* Energy specialization */
                 base_augment_psp_allowed(ch));                /* Max PSP augmentation */
  }
  
  /* Display spellcasting bonuses if character is a spellcaster */
  if (IS_SPELLCASTER(ch)) {
    text_line(ch, "\tySpellcaster Bonuses\tC", line_length, '-', '-');
    send_to_char(ch, "\tcSpell DC Bonus:\tn %d \tcSpell Potency Multiplier:\tn %d%% \tcSpell Duration Multiplier:\tn %d%%\r\n",
                 get_spell_dc_bonus(ch),       /* Bonus to spell difficulty class */
                 get_spell_potency_bonus(ch),  /* Damage/healing multiplier */
                 get_spell_duration_bonus(ch)); /* Duration multiplier */
  }

  /* ========================================================================= */
  /* SECTION 7: QUEST INFORMATION                                              */
  /* ========================================================================= */
  
  text_line(ch, "\tyQuest Info\tC", line_length, '-', '-');
  
  /* Display quest statistics (NPCs always show 0) */
  send_to_char(ch, "\tcQuests completed : \tn%d\tc, Quest points     : \tn%d\r\n",
               (!IS_NPC(ch) ? GET_NUM_QUESTS(ch) : 0),    /* Number of completed quests */
               (!IS_NPC(ch) ? GET_QUESTPOINTS(ch) : 0));   /* Total quest points earned */
  
  /* NOTE: Active quest display code is commented out - preserved for future use */
  /*
    if (!IS_NPC(ch) && GET_QUEST(ch, index) != NOTHING)
      send_to_char(ch, "%-60s\r\n", GET_QUEST(ch, index) == NOTHING ? 
                   "None" : QST_NAME(real_quest(GET_QUEST(ch, index))));
    else
      send_to_char(ch, "None\r\n");
  */
  
  /* Display active crafting quest if any */
  if (!IS_NPC(ch) && GET_AUTOCQUEST_VNUM(ch)) {
    send_to_char(ch, "\tcOn Crafting Job: (\tn%d\tc) \tn%s\tc, using: \tn%s\r\n",
                 GET_AUTOCQUEST_MAKENUM(ch),               /* Number to craft */
                 GET_AUTOCQUEST_DESC(ch),                  /* Item description */
                 (GET_AUTOCQUEST_MATERIAL(ch) >= 0 && 
                  GET_AUTOCQUEST_MATERIAL(ch) < NUM_MATERIALS) ? 
                   material_name[GET_AUTOCQUEST_MATERIAL(ch)] : "Unknown"); /* Material type */
  }

  /* ========================================================================= */
  /* SECTION 8: WEALTH AND FINANCES                                            */
  /* ========================================================================= */
  
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  
  /* Display gold carried and banked */
  send_to_char(ch, "\tcGold:\tn %d                \tcGold in Bank:\tn %d\r\n",
               GET_GOLD(ch),       /* Gold coins carried */
               GET_BANK_GOLD(ch)); /* Gold stored in bank */
  
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* ========================================================================= */
  /* SECTION 9: IMMORTAL-ONLY INFORMATION                                      */
  /* ========================================================================= */
  
  /* Display immortal-specific information */
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    /* Display custom entrance message (poofin) */
    if (POOFIN(ch)) {
      send_to_char(ch, "%sPOOFIN : %s%s %s%s\r\n", 
                   QCYN, QNRM, GET_NAME(ch), POOFIN(ch), QNRM);
    } else {
      send_to_char(ch, "%sPOOFIN : %s%s appears with an ear-splitting bang.%s\r\n", 
                   QCYN, QNRM, GET_NAME(ch), QNRM);
    }
    
    /* Display custom exit message (poofout) */
    if (POOFOUT(ch)) {
      send_to_char(ch, "%sPOOFOUT: %s%s %s%s\r\n", 
                   QCYN, QNRM, GET_NAME(ch), POOFOUT(ch), QNRM);
    } else {
      send_to_char(ch, "%sPOOFOUT: %s%s disappears in a puff of smoke.%s\r\n", 
                   QCYN, QNRM, GET_NAME(ch), QNRM);
    }
    
    /* Display OLC (online creation) zone assignment */
    send_to_char(ch, "\tcYour current zone:\tn %s%d%s\r\n", 
                 CCCYN(ch, C_NRM), GET_OLC_ZONE(ch), CCNRM(ch, C_NRM));
    
    send_to_char(ch, "\tC");
    draw_line(ch, line_length, '-', '-');
  }

  /* ========================================================================= */
  /* SECTION 10: CLASS-SPECIFIC FEATURES                                       */
  /* ========================================================================= */
  
  /* Display Inquisitor Bane effect if active */
  if (affected_by_spell(ch, ABILITY_AFFECT_BANE_WEAPON)) {
    send_to_char(ch, "\tcIniquisitor Bane Effect:\tn +%dd6 against %s.\r\n",
                 HAS_REAL_FEAT(ch, FEAT_PERFECT_JUDGEMENT) ? 6 :    /* Perfect: 6d6 */
                 (HAS_REAL_FEAT(ch, FEAT_GREATER_BANE) ? 4 : 2),    /* Greater: 4d6, Normal: 2d6 */
                 (GET_BANE_TARGET_TYPE(ch) >= 0 && 
                  GET_BANE_TARGET_TYPE(ch) < NUM_RACE_TYPES) ? 
                   race_family_types_plural[GET_BANE_TARGET_TYPE(ch)] : "Unknown");
  }

  /* Display Cleric domains (2 domains) */
  if (CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "\tc1st Domain: \tn%s\tc, 2nd Domain: \tn%s\tc.\r\n",
                 (GET_1ST_DOMAIN(ch) >= 0 && GET_1ST_DOMAIN(ch) < NUM_DOMAINS) ? 
                   domain_list[GET_1ST_DOMAIN(ch)].name : "None",
                 (GET_2ND_DOMAIN(ch) >= 0 && GET_2ND_DOMAIN(ch) < NUM_DOMAINS) ? 
                   domain_list[GET_2ND_DOMAIN(ch)].name : "None");
    draw_line(ch, line_length, '-', '-');
  }
  /* Display Inquisitor domain (1 domain only) */
  else if (CLASS_LEVEL(ch, CLASS_INQUISITOR)) {
    send_to_char(ch, "\tc1st Domain: \tn%s\tc.\r\n",
                 (GET_1ST_DOMAIN(ch) >= 0 && GET_1ST_DOMAIN(ch) < NUM_DOMAINS) ? 
                   domain_list[GET_1ST_DOMAIN(ch)].name : "None");
    draw_line(ch, line_length, '-', '-');
  }

  /* Display Sorcerer bloodline information */
  if (HAS_REAL_FEAT(ch, FEAT_SORCERER_BLOODLINE_DRACONIC)) {
    /* Draconic bloodline - shows dragon type and energy damage */
    int subtype = GET_BLOODLINE_SUBTYPE(ch);
    send_to_char(ch, "\tcSorcerer Bloodline: \tnDraconic (%s/%s).\r\n", 
                 (subtype >= 0 && subtype < NUM_DRACONIC_HERITAGE_TYPES) ? 
                   DRCHRTLIST_NAME(subtype) : "Unknown",     /* Dragon type name */
                 (subtype >= 0 && subtype < NUM_DRACONIC_HERITAGE_TYPES) ? 
                   DRCHRT_ENERGY_TYPE(subtype) : "Unknown"); /* Energy damage type */
    draw_line(ch, line_length, '-', '-');
  }
  else if (HAS_REAL_FEAT(ch, FEAT_SORCERER_BLOODLINE_ARCANE)) {
    /* Arcane bloodline - shows school specialization */
    int subtype = GET_BLOODLINE_SUBTYPE(ch);
    send_to_char(ch, "\tcSorcerer Bloodline: \tnArcane (%s magic).\r\n", 
                 (subtype >= 0 && subtype < NUM_SCHOOLS) ? 
                   spell_schools_lower[subtype] : "Unknown");
    draw_line(ch, line_length, '-', '-');
  }
  else if (HAS_REAL_FEAT(ch, FEAT_SORCERER_BLOODLINE_FEY)) {
    /* Fey bloodline */
    send_to_char(ch, "\tcSorcerer Bloodline: \tnFey.\r\n");
    draw_line(ch, line_length, '-', '-');
  }
  else if (HAS_REAL_FEAT(ch, FEAT_SORCERER_BLOODLINE_UNDEAD)) {
    /* Undead bloodline */
    send_to_char(ch, "\tcSorcerer Bloodline: \tnUndead.\r\n");
    draw_line(ch, line_length, '-', '-');
  }

  /* Display Wizard school specialization */
  if (CLASS_LEVEL(ch, CLASS_WIZARD)) {
    int school = GET_SPECIALTY_SCHOOL(ch);
    send_to_char(ch, "\tcSpecialty School: \tn%s\tc, Restricted: \tn%s\tc.\r\n",
                 (school >= 0 && school < NUM_SCHOOLS) ? 
                   school_names[school] : "None",              /* Specialized school */
                 (school >= 0 && school < NUM_SCHOOLS && 
                  restricted_school_reference[school] >= 0 && 
                  restricted_school_reference[school] < NUM_SCHOOLS) ? 
                   school_names[restricted_school_reference[school]] : "None"); /* Opposing school */
    draw_line(ch, line_length, '-', '-');
  }

  /* ========================================================================= */
  /* SECTION 11: STATUS CONDITIONS (PLAYERS ONLY)                              */
  /* ========================================================================= */
  
  if (!IS_NPC(ch)) {
    send_to_char(ch, "\tc");
    
    /* Display intoxication status */
    if (GET_COND(ch, DRUNK) > 10) {
      send_to_char(ch, "You are intoxicated.\r\n");
    } else {
      send_to_char(ch, "You are sober.\r\n");
    }
    
    /* Display hunger status */
    if (GET_COND(ch, HUNGER) == 0) {
      send_to_char(ch, "You are hungry.\r\n");
    }
    
    /* Display thirst status */
    if (GET_COND(ch, THIRST) == 0) {
      send_to_char(ch, "You are thirsty.\r\n");
    }
    
    /* Display summonable status */
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SUMMONABLE)) {
      send_to_char(ch, "You are summonable by other players.\r\n");
    } else {
      send_to_char(ch, "You are NOT summonable by other players.\r\n");
    }
    
    /* Display monk gear restriction warning */
    if (MONK_TYPE(ch) && !monk_gear_ok(ch)) {
      send_to_char(ch, "Your worn gear is interfering with your ki.\r\n");
    }
    
    send_to_char(ch, "\tC");
    draw_line(ch, line_length, '-', '-');
    
    /* Display Dragon Rider information if applicable */
    if (CLASS_LEVEL(ch, CLASS_DRAGONRIDER) && GET_DRAGON_RIDER_DRAGON_TYPE(ch)) {
      send_to_char(ch, "\tc");
      int dragon_type = GET_DRAGON_RIDER_DRAGON_TYPE(ch);
      
      /* Display dragon mount type and damage */
      send_to_char(ch, "Dragon Mount Type: %s (%s damage) ",
        (dragon_type >= 0 && dragon_type < NUM_DRACONIC_HERITAGE_TYPES) ?
          DRCHRTLIST_NAME(dragon_type) : "Unknown",
        (dragon_type >= 0 && dragon_type < NUM_DRACONIC_HERITAGE_TYPES) ?
          DRCHRT_ENERGY_TYPE(dragon_type) : "Unknown");
      
      /* Display dragon bond type if any */
      if (GET_DRAGON_BOND_TYPE(ch)) {
        int bond_type = GET_DRAGON_BOND_TYPE(ch);
        send_to_char(ch, "Dragon Bond Type: %s", 
                     (bond_type >= 0 && bond_type < NUM_DRAGON_BOND_TYPES) ?
                       dragon_bond_types[bond_type] : "Unknown");
      }
      
      send_to_char(ch, "\r\n");
      send_to_char(ch, "\tC");
      draw_line(ch, line_length, '-', '-');
    }
  }

  /* ========================================================================= */
  /* SECTION 12: AVAILABLE COMMANDS AND HELPFUL HINTS                          */
  /* ========================================================================= */
  
  /* Universal combat commands */
  send_to_char(ch, "\tDType 'attacks' or 'defenses' to see your melee offense and defense\tn\r\n");
  send_to_char(ch, "\tDType 'affects' to see what you are affected by\tn\r\n");
  
  /* Class-specific spell/ability interfaces */
  if (CLASS_LEVEL(ch, CLASS_WIZARD)) {
    send_to_char(ch, "\tDType 'memorize' to see your Wizard spell interface\tn\r\n");
  }
  
  if (CLASS_LEVEL(ch, CLASS_SORCERER)) {
    send_to_char(ch, "\tDType 'meditate' to see your Sorcerer spell interface\tn\r\n");
  }
  
  if (CLASS_LEVEL(ch, CLASS_PSIONICIST)) {
    send_to_char(ch, "\tDType 'powers' to see your Psionicist powers, and 'manifest' to perform them.\tn\r\n");
  }
  
  if (CLASS_LEVEL(ch, CLASS_INQUISITOR)) {
    send_to_char(ch, "\tDType 'compel' to see your Inquisitor spell interface\tn\r\n");
  }
  
  if (CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "\tDType 'prayer' to see your Cleric spell interface\tn\r\n");
  }
  
  if (CLASS_LEVEL(ch, CLASS_RANGER)) {
    send_to_char(ch, "\tDType 'adjure' to see your Ranger spell interface\tn\r\n");
  }
  
  if (CLASS_LEVEL(ch, CLASS_BARD)) {
    send_to_char(ch, "\tDType 'compose' to see your Bard spell interface\tn\r\n");
  }
  
  if (CLASS_LEVEL(ch, CLASS_DRUID)) {
    send_to_char(ch, "\tDType 'commune' to see your Druid spell interface\tn\r\n");
  }
  
  if (CLASS_LEVEL(ch, CLASS_PALADIN)) {
    send_to_char(ch, "\tDType 'chant' to see your Paladin spell interface\tn\r\n");
  }
  
  if (CLASS_LEVEL(ch, CLASS_BLACKGUARD)) {
    send_to_char(ch, "\tDType 'condemn' to see your BlackGuard spell interface\tn\r\n");
  }
  
  if (CLASS_LEVEL(ch, CLASS_ALCHEMIST)) {
    send_to_char(ch, "\tDType 'extracts' to see your Alchemist extract interface\tn\r\n");
    send_to_char(ch, "\tDType 'imbibe' to use an extract, and 'concoct' to prepare an extract.\tn\r\n");
    send_to_char(ch, "\tDType 'discoveries' to see your alchemist discoveries.\tn\r\n");
    send_to_char(ch, "\tDType 'swallow' to use a mutagen or cognatogen (if you have cognatogen discovery).\tn\r\n");
  }
  
  /* Ranger favored enemy system */
  if (GET_FAVORED_ENEMY(ch, 0) > 0) {
    send_to_char(ch, "\tDType 'favoredenemies' to get a list of your favored enemies.\tn\r\n");
  }
  
  /* End of score display */
}

/* Enhanced score command implementing Phase 1 MVP features */

/* Enhanced formatting utility functions */
static void skore_progress_bar(struct char_data *ch, const char *label, int current, int max, const char *color)
{
  char bar[21] = {'\0'}; // 20 chars + null terminator
  int filled = 0;
  int percentage = 0;

  if (max > 0) {
    percentage = (current * 100) / max;
    filled = (current * 20) / max;
    if (filled > 20) filled = 20;
  }

  // Build the progress bar
  int i;
  for (i = 0; i < 20; i++) {
    if (i < filled) {
      bar[i] = '='; // Use = for compatibility
    } else {
      bar[i] = '-'; // Use - for compatibility
    }
  }
  bar[20] = '\0';

  send_to_char(ch, "\tc%-12s\tn %s[%s%s\tn] \tc%d/%d \tn(\tc%d%%\tn)\r\n",
               label, color, color, bar, current, max, percentage);
}

static void skore_section_header(struct char_data *ch, const char *title, int width, const char *color)
{
  send_to_char(ch, "%s", color);
  text_line(ch, title, width, '=', '=');
  send_to_char(ch, "\tn");
}

/* Removed unused function skore_subsection_header */

static const char *get_health_color(struct char_data *ch, int current, int max)
{
  int theme, percentage;
  
  if (max <= 0) return "\tn";

  // Check if colors are disabled
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SCORE_NOCOLOR)) {
    return "\tn";
  }

  percentage = (current * 100) / max;
  theme = !IS_NPC(ch) ? GET_SCORE_COLOR_THEME(ch) : SCORE_THEME_ENHANCED;

  switch (theme) {
    case SCORE_THEME_CLASSIC:
      // Classic: Simple red when critical, white otherwise
      return (percentage < 25) ? "\tR" : "\tn";
      
    case SCORE_THEME_MINIMAL:
      // Minimal: Only two states
      return (percentage < 50) ? "\tR" : "\tG";
      
    case SCORE_THEME_HIGHCONTRAST:
      // High contrast: Bold colors
      if (percentage >= 75) return "\tW"; // Bright white for healthy
      if (percentage >= 50) return "\tY"; // Yellow for wounded
      if (percentage >= 25) return "\tM"; // Magenta for badly wounded
      return "\tR"; // Red for critical
      
    case SCORE_THEME_DARK:
      // Dark theme: Muted colors
      if (percentage >= 75) return "\tc"; // Cyan for healthy
      if (percentage >= 50) return "\ty"; // Dark yellow for wounded
      if (percentage >= 25) return "\tr"; // Dark red for badly wounded
      return "\tR"; // Bright red for critical
      
    case SCORE_THEME_COLORBLIND:
      // Colorblind-friendly: Avoid red-green
      if (percentage >= 75) return "\tB"; // Blue for healthy
      if (percentage >= 50) return "\tW"; // White for wounded
      if (percentage >= 25) return "\tY"; // Yellow for badly wounded
      return "\tM"; // Magenta for critical
      
    case SCORE_THEME_ENHANCED:
    default:
      // Enhanced: Full color gradient
      if (percentage >= 75) return "\tG"; // Green for healthy
      if (percentage >= 50) return "\tY"; // Yellow for wounded
      if (percentage >= 25) return "\tO"; // Orange for badly wounded
      return "\tR"; // Red for critical
  }
}

static const char *get_class_color(struct char_data *ch, int class_num)
{
  int theme;
  
  // Check if colors are disabled
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SCORE_NOCOLOR)) {
    return "\tn";
  }

  theme = !IS_NPC(ch) ? GET_SCORE_COLOR_THEME(ch) : SCORE_THEME_ENHANCED;

  switch (theme) {
    case SCORE_THEME_CLASSIC:
      // Classic: Minimal coloring
      return "\tn";
      
    case SCORE_THEME_MINIMAL:
      // Minimal: Simple two-tone
      switch (class_num) {
        case CLASS_WIZARD:
        case CLASS_SORCERER:
        case CLASS_CLERIC:
        case CLASS_DRUID:
        case CLASS_BARD:
          return "\tB"; // Blue for casters
        default:
          return "\tR"; // Red for non-casters
      }
      
    case SCORE_THEME_HIGHCONTRAST:
      // High contrast: Bold distinct colors
      switch (class_num) {
        case CLASS_WIZARD:
        case CLASS_SORCERER:
          return "\tW"; // White for arcane
        case CLASS_CLERIC:
        case CLASS_DRUID:
          return "\tY"; // Yellow for divine
        case CLASS_WARRIOR:
        case CLASS_BERSERKER:
          return "\tR"; // Red for warriors
        case CLASS_ROGUE:
          return "\tM"; // Magenta for rogues
        default:
          return "\tC"; // Bright cyan for others
      }
      
    case SCORE_THEME_DARK:
      // Dark theme: Muted colors
      switch (class_num) {
        case CLASS_WIZARD:
        case CLASS_SORCERER:
        case CLASS_BARD:
          return "\tb"; // Dark blue for arcane
        case CLASS_CLERIC:
        case CLASS_DRUID:
        case CLASS_PALADIN:
        case CLASS_RANGER:
          return "\tg"; // Dark green for divine
        case CLASS_WARRIOR:
        case CLASS_BERSERKER:
        case CLASS_MONK:
          return "\tr"; // Dark red for martial
        case CLASS_ROGUE:
          return "\tm"; // Dark magenta for skill-based
        default:
          return "\tc"; // Dark cyan for others
      }
      
    case SCORE_THEME_COLORBLIND:
      // Colorblind-friendly: Avoid problematic combos
      switch (class_num) {
        case CLASS_WIZARD:
        case CLASS_SORCERER:
        case CLASS_BARD:
          return "\tB"; // Blue for arcane
        case CLASS_CLERIC:
        case CLASS_DRUID:
        case CLASS_PALADIN:
        case CLASS_RANGER:
          return "\tW"; // White for divine
        case CLASS_WARRIOR:
        case CLASS_BERSERKER:
        case CLASS_MONK:
          return "\tY"; // Yellow for martial
        case CLASS_ROGUE:
          return "\tC"; // Cyan for skill-based
        default:
          return "\tM"; // Magenta for others
      }
      
    case SCORE_THEME_ENHANCED:
    default:
      // Enhanced: Original full colors
      switch (class_num) {
        case CLASS_WIZARD:
        case CLASS_SORCERER:
        case CLASS_BARD:
          return "\tB"; // Blue for arcane casters
        case CLASS_CLERIC:
        case CLASS_DRUID:
        case CLASS_PALADIN:
        case CLASS_RANGER:
          return "\tG"; // Green for divine casters
        case CLASS_WARRIOR:
        case CLASS_BERSERKER:
        case CLASS_MONK:
          return "\tR"; // Red for martial classes
        case CLASS_ROGUE:
          return "\tM"; // Magenta for skill-based
        default:
          return "\tc"; // Cyan for others
      }
  }
}

/* Border style arrays for different classes */
static const char *warrior_border[] = {
  "\tR╔════════════════════════════════════════════════════════════════════════════╗\tn",
  "\tR║                                                                            ║\tn",
  "\tR╚════════════════════════════════════════════════════════════════════════════╝\tn"
};

static const char *mage_border[] = {
  "\tB╔═══════════════════════════════════════════════════════════════════════════╗\tn",
  "\tB║                                                                            ║\tn",
  "\tB╚═══════════════════════════════════════════════════════════════════════════╝\tn"
};

static const char *cleric_border[] = {
  "\tG╔════════════════════════════════════════════════════════════════════════════╗\tn",
  "\tG║                                                                            ║\tn", 
  "\tG╚════════════════════════════════════════════════════════════════════════════╝\tn"
};

static const char *rogue_border[] = {
  "\tM╔════════════════════════════════════════════════════════════════════════════╗\tn",
  "\tM║                                                                            ║\tn",
  "\tM╚════════════════════════════════════════════════════════════════════════════╝\tn"
};

static const char *default_border[] = {
  "\tc╔════════════════════════════════════════════════════════════════════════════╗\tn",
  "\tc║                                                                            ║\tn",
  "\tc╚════════════════════════════════════════════════════════════════════════════╝\tn"
};

/* Get the border style for a character's primary class */
static const char **get_class_border_style(struct char_data *ch)
{
  int primary_class = -1;
  int highest_level = 0;
  int i;
  
  // Check if borders are disabled
  if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_SCORE_BORDERS)) {
    return NULL;
  }
  
  // Find the highest level class
  for (i = 0; i < NUM_CLASSES; i++) {
    if (CLASS_LEVEL(ch, i) > highest_level) {
      highest_level = CLASS_LEVEL(ch, i);
      primary_class = i;
    }
  }
  
  // If premade build, use that class
  if (GET_PREMADE_BUILD_CLASS(ch) != CLASS_UNDEFINED) {
    primary_class = GET_PREMADE_BUILD_CLASS(ch);
  }
  
  // Return appropriate border style
  switch (primary_class) {
    case CLASS_WARRIOR:
    case CLASS_BERSERKER:
    case CLASS_MONK:
      return warrior_border;
    case CLASS_WIZARD:
    case CLASS_SORCERER:
    case CLASS_BARD:
      return mage_border;
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_PALADIN:
    case CLASS_RANGER:
      return cleric_border;
    case CLASS_ROGUE:
      return rogue_border;
    default:
      return default_border;
  }
}

/* Race symbol definitions */
static const char *race_symbols[] = {
  /* RACE_HUMAN */          "☺",
  /* RACE_ELF */            "♠",
  /* RACE_DWARF */          "♦",
  /* RACE_H_TROLL */        "Ω",
  /* RACE_CRYSTAL_DWARF */  "◊",
  /* RACE_HALFLING */       "•",
  /* RACE_H_ELF */          "♣",
  /* RACE_H_ORC */          "○",
  /* RACE_GNOME */          "♥",
  /* RACE_TRELUX */         "∞",
  /* RACE_ARCANA_GOLEM */   "◙",
  /* RACE_DROW */           "▼",
  /* RACE_DUERGAR */        "■",
  /* RACE_HIGH_ELF */       "♀",
  /* RACE_WOOD_ELF */       "♂",
  /* RACE_HALF_DROW */      "▲",
  /* RACE_TABAXI */         "♪",
  /* RACE_TIEFLING */       "Ψ",
  /* RACE_AASIMAR */        "☼",
  /* RACE_WARFORGED */      "⌂",
  /* RACE_DRAGONBORN */     "∆",
  /* RACE_STOUT_HALFLING */ "◘",
  /* RACE_FOREST_GNOME */   "♫",
  /* RACE_GOLD_DWARF */     "◄",
  /* RACE_AVARIEL */        "↑",
  /* RACE_YUAN_TI */        "§",
  /* RACE_CENTAUR */        "¤",
  /* RACE_AARAKOCRA */      "↔",
  /* RACE_H_OGRE */         "◦"
};

/* Get the symbol for a character's race */
static const char *get_race_symbol(struct char_data *ch)
{
  int race;
  
  // Check if symbols are disabled
  if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_SCORE_RACE_SYMBOLS)) {
    return "";
  }
  
  race = GET_RACE(ch);
  
  // Bounds check
  if (race < 0 || race >= (int)(sizeof(race_symbols) / sizeof(race_symbols[0]))) {
    return "?";
  }
  
  return race_symbols[race];
}

/* Layout template section order definitions */
static const byte layout_default[8] = {
  SECTION_IDENTITY,     /* 0 */
  SECTION_VITALS,       /* 1 */
  SECTION_EXPERIENCE,   /* 2 */
  SECTION_ABILITIES,    /* 3 */
  SECTION_COMBAT,       /* 4 */
  SECTION_MAGIC,        /* 5 */
  SECTION_WEALTH,       /* 6 */
  SECTION_EQUIPMENT     /* 7 */
};

static const byte layout_combat[8] = {
  SECTION_COMBAT,       /* 0 - Combat stats first */
  SECTION_VITALS,       /* 1 - HP/Move next */
  SECTION_ABILITIES,    /* 2 - Stats matter in combat */
  SECTION_EQUIPMENT,    /* 3 - What gear you have */
  SECTION_MAGIC,        /* 4 - Spells/abilities */
  SECTION_IDENTITY,     /* 5 - Who you are */
  SECTION_EXPERIENCE,   /* 6 - Level info */
  SECTION_WEALTH        /* 7 - Gold last */
};

static const byte layout_roleplay[8] = {
  SECTION_IDENTITY,     /* 0 - Who you are matters most */
  SECTION_ABILITIES,    /* 2 - Your capabilities */
  SECTION_WEALTH,       /* 3 - Your resources */
  SECTION_EQUIPMENT,    /* 4 - What you carry */
  SECTION_VITALS,       /* 5 - Basic health */
  SECTION_EXPERIENCE,   /* 6 - Progress */
  SECTION_MAGIC,        /* 7 - Magic abilities */
  SECTION_COMBAT        /* 8 - Combat stats last */
};

static const byte layout_explorer[8] = {
  SECTION_VITALS,       /* 0 - Health/movement primary */
  SECTION_ABILITIES,    /* 1 - Physical stats important */
  SECTION_EQUIPMENT,    /* 2 - Gear for exploring */
  SECTION_IDENTITY,     /* 3 - Who you are */
  SECTION_EXPERIENCE,   /* 4 - Progress tracking */
  SECTION_WEALTH,       /* 5 - Resources */
  SECTION_MAGIC,        /* 6 - Magic abilities */
  SECTION_COMBAT        /* 7 - Combat less important */
};

static const byte layout_caster[8] = {
  SECTION_MAGIC,        /* 0 - Spells/PSP first */
  SECTION_VITALS,       /* 1 - HP/Mana next */
  SECTION_ABILITIES,    /* 2 - Mental stats important */
  SECTION_IDENTITY,     /* 3 - Character info */
  SECTION_EXPERIENCE,   /* 4 - Level progression */
  SECTION_EQUIPMENT,    /* 5 - Magic items */
  SECTION_COMBAT,       /* 6 - Combat secondary */
  SECTION_WEALTH        /* 7 - Gold last */
};

/* Get template section order based on layout template */
static const byte *get_template_section_order(int template)
{
  switch (template) {
    case LAYOUT_COMBAT:
      return layout_combat;
    case LAYOUT_ROLEPLAY:
      return layout_roleplay;
    case LAYOUT_EXPLORER:
      return layout_explorer;
    case LAYOUT_CASTER:
      return layout_caster;
    case LAYOUT_DEFAULT:
    default:
      return layout_default;
  }
}

/* Get the layout template for a character */
/* TODO: Hook this into the score display system */
__attribute__((unused)) static const byte *get_layout_template(struct char_data *ch)
{
  byte template;
  
  if (IS_NPC(ch)) {
    return layout_default;
  }
  
  template = GET_SCORE_LAYOUT_TEMPLATE(ch);
  
  switch (template) {
    case LAYOUT_COMBAT:
      return layout_combat;
    case LAYOUT_ROLEPLAY:
      return layout_roleplay;
    case LAYOUT_EXPLORER:
      return layout_explorer;
    case LAYOUT_CASTER:
      return layout_caster;
    case LAYOUT_DEFAULT:
    default:
      return layout_default;
  }
}

/* Display individual score sections */
static void display_identity_section(struct char_data *ch, int line_length);
static void display_vitals_section(struct char_data *ch, int line_length);
static void display_experience_section(struct char_data *ch, int line_length);
static void display_abilities_section(struct char_data *ch, int line_length);
static void display_combat_section(struct char_data *ch, int line_length);
static void display_magic_section(struct char_data *ch, int line_length);
static void display_wealth_section(struct char_data *ch, int line_length);
static void display_equipment_section(struct char_data *ch, int line_length);
static void display_active_effects(struct char_data *ch);
static void display_score_section(struct char_data *ch, int section_id, int line_length);

/* Implementation of score section display functions */
static void display_identity_section(struct char_data *ch, int line_length)
{
  char class_buf[MAX_STRING_LENGTH] = {'\0'};
  char dname[SMALL_STRING] = {'\0'};
  int i = 0, counter = 0;
  
  skore_section_header(ch, "\tY*** CHARACTER IDENTITY ***\tC", line_length, "\tC");

  send_to_char(ch, "\tc+-- Personal Information -------------------------------------------------------+\tn\r\n");
  send_to_char(ch, "\tc|\tn \tcName:\tn %-20s \tc|\tn \tcTitle:\tn %-30s \tc|\tn\r\n",
               GET_NAME(ch), GET_TITLE(ch) ? GET_TITLE(ch) : "None");

  snprintf(dname, sizeof(dname), "%s", deity_list[GET_DEITY(ch)].name);
  
  /* Build race string with optional symbol */
  char race_display[64];
  const char *race_symbol = get_race_symbol(ch);
  if (race_symbol && *race_symbol) {
    snprintf(race_display, sizeof(race_display), "%s %s", race_symbol, race_list[GET_RACE(ch)].type);
  } else {
    snprintf(race_display, sizeof(race_display), "%s", race_list[GET_RACE(ch)].type);
  }
  
  send_to_char(ch, "\tc|\tn \tcRace:\tn %-20s \tc|\tn \tcDeity:\tn %-30s \tc|\tn\r\n",
               race_display,
               deity_list[GET_DEITY(ch)].name ? CAP(dname) : "None");

  /* Build enhanced class display with colors */
  *class_buf = '\0';
  if (!IS_NPC(ch))
  {
    for (i = 0; i < MAX_CLASSES; i++)
    {
      if (CLASS_LEVEL(ch, i))
      {
        if (counter > 0)
          strlcat(class_buf, " \tc/\tn ", sizeof(class_buf));

        char temp_buf[64];
        snprintf(temp_buf, sizeof(temp_buf), "%s%d %s\tn",
                get_class_color(ch, i), CLASS_LEVEL(ch, i), CLSLIST_ABBRV(i));
        strlcat(class_buf, temp_buf, sizeof(class_buf));
        counter++;
      }
    }
  }
  else
    strlcpy(class_buf, CLASS_ABBR(ch), sizeof(class_buf));

  if (GET_PREMADE_BUILD_CLASS(ch) != CLASS_UNDEFINED)
  {
    snprintf(class_buf, sizeof(class_buf), "%s%d %s (premade build)\tn",
             get_class_color(ch, GET_PREMADE_BUILD_CLASS(ch)),
             CLASS_LEVEL(ch, GET_PREMADE_BUILD_CLASS(ch)),
             class_list[GET_PREMADE_BUILD_CLASS(ch)].name);
  }

  send_to_char(ch, "\tc|\tn \tcClass%s:\tn %-50s \tc|\tn\r\n",
               (counter == 1 ? "" : "es"), class_buf);

  send_to_char(ch, "\tc|\tn \tcAlignment:\tn %-15s \tc|\tn \tcAge:\tn %-8s \tc|\tn \tcSize:\tn %-12s \tc|\tn\r\n",
               get_align_by_num(GET_ALIGNMENT(ch)),
               character_ages[GET_CH_AGE(ch)],
               size_names[GET_SIZE(ch)]);

  send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
}

static void display_vitals_section(struct char_data *ch, int line_length)
{
  struct time_info_data playing_time;
  float height = GET_HEIGHT(ch);
  
  skore_section_header(ch, "\tR*** VITALS & CONDITION ***\tC", line_length, "\tC");

  /* Health-based color for vitals */
  const char *hp_color = get_health_color(ch, GET_HIT(ch), GET_MAX_HIT(ch));
  const char *mv_color = get_health_color(ch, GET_MOVE(ch), GET_MAX_MOVE(ch));

  skore_progress_bar(ch, "Hit Points", GET_HIT(ch), GET_MAX_HIT(ch), hp_color);
  skore_progress_bar(ch, "Movement", GET_MOVE(ch), GET_MAX_MOVE(ch), mv_color);

  /* PSP for psionicists */
  if (GET_MAX_PSP(ch) > 0) {
    const char *psp_color = get_health_color(ch, GET_PSP(ch), GET_MAX_PSP(ch));
    skore_progress_bar(ch, "PSP", GET_PSP(ch), GET_MAX_PSP(ch), psp_color);
  }

  playing_time = *real_time_passed((time(0) - ch->player.time.logon) +
                                       ch->player.time.played,
                                   0);
  height *= 0.393700787402;

  send_to_char(ch, "\r\n\tc+-- Physical Status ------------------------------------------------------------+\tn\r\n");
  send_to_char(ch, "\tc|\tn \tcHeight:\tn %5.1f inches \tc|\tn \tcWeight:\tn %4d lbs \tc|\tn \tcSpeed:\tn %3d \tc|\tn \tcInit:\tn %s%d \tc|\tn\r\n",
               height, GET_WEIGHT(ch), get_speed(ch, TRUE),
               get_initiative_modifier(ch) >= 0 ? "+" : "", get_initiative_modifier(ch));

  send_to_char(ch, "\tc|\tn \tcCarrying:\tn %d/%d lbs \tc|\tn \tcItems:\tn %d/%d \tc|\tn \tcPlayed:\tn %dd %dh \tc|\tn\r\n",
               IS_CARRYING_W(ch), CAN_CARRY_W(ch), IS_CARRYING_N(ch), CAN_CARRY_N(ch),
               playing_time.day, playing_time.hours);
  send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
}

static void display_experience_section(struct char_data *ch, int line_length)
{
  skore_section_header(ch, "\tY*** EXPERIENCE & PROGRESSION ***\tC", line_length, "\tC");

  /* Experience progress bar */
  int exp_needed = (GET_LEVEL(ch) >= LVL_IMMORT ? 0 : level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch));
  int exp_current = (GET_LEVEL(ch) >= LVL_IMMORT ? 1 : GET_EXP(ch) - (GET_LEVEL(ch) > 1 ? level_exp(ch, GET_LEVEL(ch)) : 0));
  int exp_total = (GET_LEVEL(ch) >= LVL_IMMORT ? 1 : level_exp(ch, GET_LEVEL(ch) + 1) - (GET_LEVEL(ch) > 1 ? level_exp(ch, GET_LEVEL(ch)) : 0));

  if (GET_LEVEL(ch) < LVL_IMMORT) {
    skore_progress_bar(ch, "Experience", exp_current, exp_total, "\tY");
    send_to_char(ch, "\tc             \tn \tcTotal EXP:\tn %s \tc|\tn \tcNeeded:\tn %s \tc|\tn \tcLevel:\tn %d \tn\r\n",
                 add_commas(GET_EXP(ch)), add_commas(exp_needed), GET_LEVEL(ch));
  } else {
    send_to_char(ch, "\tc             \tn \tcTotal EXP:\tn %s \tc|\tn \tcLevel:\tn %d (IMMORTAL) \tn\r\n",
                 add_commas(GET_EXP(ch)), GET_LEVEL(ch));
  }

  send_to_char(ch, "\r\n\tc+-- Caster Levels --------------------------------------------------------------+\tn\r\n");
  send_to_char(ch, "\tc|\tn \tcCaster Level:\tn %-3d \tc|\tn \tcDivine Level:\tn %-3d \tc|\tn \tcMagic Level:\tn %-3d \tc|\tn\r\n",
               CASTER_LEVEL(ch), DIVINE_LEVEL(ch), MAGIC_LEVEL(ch));
  send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
}

static void display_abilities_section(struct char_data *ch, int line_length)
{
  skore_section_header(ch, "\tG*** ABILITY SCORES & SAVES ***\tC", line_length, "\tC");

  send_to_char(ch, "\tc+-- Ability Scores -------------------------+-- Saving Throws ---------------+\tn\r\n");
  send_to_char(ch, "\tc|\tn \tcStr:\tn %2d[%2d] \tcDex:\tn %2d[%2d] \tcCon:\tn %2d[%2d] \tc|\tn \tcFort:\tn %-3d \tcWill:\tn %-3d \tc|\tn\r\n",
               GET_STR(ch), GET_STR_BONUS(ch), GET_DEX(ch), GET_DEX_BONUS(ch), GET_CON(ch), GET_CON_BONUS(ch),
               compute_mag_saves(ch, SAVING_FORT, 0), compute_mag_saves(ch, SAVING_WILL, 0));
  send_to_char(ch, "\tc|\tn \tcInt:\tn %2d[%2d] \tcWis:\tn %2d[%2d] \tcCha:\tn %2d[%2d] \tc|\tn \tcReflex:\tn %-3d       \tc|\tn\r\n",
               GET_INT(ch), GET_INT_BONUS(ch), GET_WIS(ch), GET_WIS_BONUS(ch), GET_CHA(ch), GET_CHA_BONUS(ch),
               compute_mag_saves(ch, SAVING_REFL, 0));
  send_to_char(ch, "\tc+--------------------------------------------+--------------------------------+\tn\r\n");
}

static void display_combat_section(struct char_data *ch, int line_length)
{
  int calc_bab = MIN(MAX_BAB, ACTUAL_BAB(ch));
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD_1);
  
  skore_section_header(ch, "\tR*** COMBAT STATISTICS ***\tC", line_length, "\tC");

#define RETURN_NUM_ATTACKS 1
  int num_attacks = perform_attacks(ch, RETURN_NUM_ATTACKS, 0);
  int armor_class = compute_armor_class(NULL, ch, FALSE, MODE_ARMOR_CLASS_NORMAL);
#undef RETURN_NUM_ATTACKS

  send_to_char(ch, "\tc+-- Combat Stats ---------------------------------------------------------------+\tn\r\n");
  send_to_char(ch, "\tc|\tn \tcBAB:\tn %-4d \tc|\tn \tcAttacks:\tn %-3d \tc|\tn \tcAC:\tn %-3d \tc|\tn \tcWimpy:\tn %-3d \tc|\tn \tcPos:\tn %-10s \tc|\tn\r\n",
               calc_bab, num_attacks, armor_class, GET_WIMP_LEV(ch),
               FIGHTING(ch) ? "Fighting" : position_types[GET_POS(ch)]);

  send_to_char(ch, "\tc|\tn \tcHitroll:\tn %s%-3d \tc|\tn \tcDamroll:\tn %s%-3d \tc|\tn \tcDR:\tn %-3d \tc|\tn \tcSR:\tn %-3d \tc|\tn\r\n",
               GET_HITROLL(ch) >= 0 ? "+" : "", GET_HITROLL(ch),
               GET_DAMROLL(ch) >= 0 ? "+" : "", GET_DAMROLL(ch),
               compute_damage_reduction(ch, DAM_RESERVED_DBC),
               compute_spell_res(ch, ch, 0));

  /* Weapon information */
  if (wielded) {
    send_to_char(ch, "\tc|\tn \tcWeapon:\tn %-30s \tc|\tn \tcDamage:\tn %dd%d%s%d \tc|\tn\r\n",
                 wielded->short_description,
                 GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2),
                 GET_DAMROLL(ch) >= 0 ? "+" : "", GET_DAMROLL(ch));
  } else {
    send_to_char(ch, "\tc|\tn \tcWeapon:\tn %-30s \tc|\tn \tcDamage:\tn %dd%d%s%d \tc|\tn\r\n",
                 "Unarmed", 1, 3, GET_DAMROLL(ch) >= 0 ? "+" : "", GET_DAMROLL(ch));
  }
  send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
}

static void display_magic_section(struct char_data *ch, int line_length)
{
  
  skore_section_header(ch, "\tB*** MAGIC & PSIONICS ***\tC", line_length, "\tC");

  if (IS_SPELLCASTER(ch)) {
    send_to_char(ch, "\tc+-- Spellcaster Bonuses --------------------------------------------------------+\tn\r\n");
    send_to_char(ch, "\tc|\tn \tcSpell DC Bonus:\tn %-3d \tc|\tn \tcPotency:\tn %-3d%% \tc|\tn \tcDuration:\tn %-3d%% \tc|\tn\r\n",
                 get_spell_dc_bonus(ch), get_spell_potency_bonus(ch), get_spell_duration_bonus(ch));
    send_to_char(ch, "\tc+-- Spell Slots ----------------------------------------------------------------+\tn\r\n");
    
    /* Display spell slots for each casting class */
    int class_idx, circle;
    
    for (class_idx = 0; class_idx < MAX_CLASSES; class_idx++) {
      if (CLASS_LEVEL(ch, class_idx) > 0) {
        /* Check if this class has any spell slots */
        int has_slots = FALSE;
        for (circle = 1; circle <= 9; circle++) {
          if (compute_slots_by_circle(ch, class_idx, circle) > 0) {
            has_slots = TRUE;
            break;
          }
        }
        if (!has_slots) continue;
        /* This class can cast spells */
        send_to_char(ch, "\tc|\tn \tc%s:\tn ", class_list[class_idx].name);
        
        for (circle = 1; circle <= 9; circle++) {
          int total_slots = compute_slots_by_circle(ch, class_idx, circle);
          if (total_slots > 0) {
            int used_slots = count_circle_collection(ch, class_idx, circle) +
                            count_circle_innate_magic(ch, class_idx, circle) +
                            count_circle_prep_queue(ch, class_idx, circle);
            int remaining = total_slots - used_slots;
            
            /* Color code based on remaining slots */
            const char *color = "\tg"; /* Green if slots available */
            if (remaining == 0) color = "\tr"; /* Red if no slots */
            else if (remaining <= total_slots / 3) color = "\ty"; /* Yellow if low */
            
            send_to_char(ch, "%s[%d: %d/%d]\tn ", color, circle, remaining, total_slots);
            has_slots = TRUE;
          }
        }
        if (has_slots) {
          send_to_char(ch, "\tc|\tn\r\n");
        }
      }
    }
    
    send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
  }

  if (GET_PSIONIC_LEVEL(ch) > 0) {
    send_to_char(ch, "\tc+-- Psionic Information --------------------------------------------------------+\tn\r\n");
    send_to_char(ch, "\tc|\tn \tcPsionic Level:\tn %-3d \tc|\tn \tcEnergy Type:\tn %-15s \tc|\tn\r\n",
                 GET_PSIONIC_LEVEL(ch), damtypes[GET_PSIONIC_ENERGY_TYPE(ch)]);
    send_to_char(ch, "\tc|\tn \tcMax Augment PSP:\tn %-3d (power psp cost) \tc|\tn\r\n",
                 base_augment_psp_allowed(ch));
    send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
  }
  
  /* Display active effects */
  display_active_effects(ch);
}

static void display_wealth_section(struct char_data *ch, int line_length)
{
  skore_section_header(ch, "\tY*** WEALTH & ACHIEVEMENTS ***\tC", line_length, "\tC");

  send_to_char(ch, "\tc+-- Wealth ---------------------------------------------------------------------+\tn\r\n");
  send_to_char(ch, "\tc|\tn \tcGold:\tn %-20s \tc|\tn \tcBank:\tn %-20s \tc|\tn\r\n",
               add_commas(GET_GOLD(ch)), add_commas(GET_BANK_GOLD(ch)));
  send_to_char(ch, "\tc+-- Quests & Achievements ------------------------------------------------------+\tn\r\n");
  send_to_char(ch, "\tc|\tn \tcQuests Completed:\tn %-6d \tc|\tn \tcQuest Points:\tn %-6d \tc|\tn\r\n",
               (!IS_NPC(ch) ? GET_NUM_QUESTS(ch) : 0),
               (!IS_NPC(ch) ? GET_QUESTPOINTS(ch) : 0));

  if (!IS_NPC(ch) && GET_AUTOCQUEST_VNUM(ch)) {
    send_to_char(ch, "\tc|\tn \tcCrafting Job:\tn (%d) %s, using: %s \tc|\tn\r\n",
                 GET_AUTOCQUEST_MAKENUM(ch), GET_AUTOCQUEST_DESC(ch),
                 material_name[GET_AUTOCQUEST_MATERIAL(ch)]);
  }
  send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
}

static void display_equipment_section(struct char_data *ch, int line_length)
{
  int i, equipped_items = 0;
  
  skore_section_header(ch, "\tM*** EQUIPMENT STATUS ***\tC", line_length, "\tC");

  send_to_char(ch, "\tc+-- Key Equipment --------------------------------------------------------------+\tn\r\n");

  /* Show main weapon */
  if (GET_EQ(ch, WEAR_WIELD_1)) {
    struct obj_data *weapon = GET_EQ(ch, WEAR_WIELD_1);
    send_to_char(ch, "\tc|\tn \tcMain Weapon:\tn %-45s \tc|\tn\r\n",
                 weapon->short_description);
  } else {
    send_to_char(ch, "\tc|\tn \tcMain Weapon:\tn %-45s \tc|\tn\r\n", "None (unarmed)");
  }

  /* Show armor */
  if (GET_EQ(ch, WEAR_BODY)) {
    struct obj_data *armor = GET_EQ(ch, WEAR_BODY);
    send_to_char(ch, "\tc|\tn \tcBody Armor:\tn %-46s \tc|\tn\r\n",
                 armor->short_description);
  } else {
    send_to_char(ch, "\tc|\tn \tcBody Armor:\tn %-46s \tc|\tn\r\n", "None");
  }

  /* Show shield */
  if (GET_EQ(ch, WEAR_SHIELD)) {
    struct obj_data *shield = GET_EQ(ch, WEAR_SHIELD);
    send_to_char(ch, "\tc|\tn \tcShield:\tn %-50s \tc|\tn\r\n",
                 shield->short_description);
  } else {
    send_to_char(ch, "\tc|\tn \tcShield:\tn %-50s \tc|\tn\r\n", "None");
  }

  /* Equipment condition summary */
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) equipped_items++;
  }

  send_to_char(ch, "\tc|\tn \tcEquipped Items:\tn %-3d / %d slots                                  \tc|\tn\r\n",
               equipped_items, NUM_WEARS);
  send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
}

/* Display active effects with duration bars */
static void display_active_effects(struct char_data *ch)
{
  struct affected_type *aff;
  int count = 0;
  const char *color;
  int bar_length = 20; // Length of progress bar
  int filled_length;
  int i;
  
  send_to_char(ch, "\tc+-- Active Effects -------------------------------------------------------------+\tn\r\n");
  
  if (!ch->affected) {
    send_to_char(ch, "\tc|\tn \tcNo active effects\tn                                                     \tc|\tn\r\n");
    send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
    return;
  }
  
  for (aff = ch->affected; aff; aff = aff->next) {
    if (aff->spell <= 0) continue;
    
    count++;
    
    // For now, just show spell number - can be enhanced later
    char spell_name[32];
    snprintf(spell_name, sizeof(spell_name), "Spell #%d", aff->spell);
    
    // Calculate progress bar based on duration
    if (aff->duration < 0) {
      // Permanent effect
      send_to_char(ch, "\tc|\tn \tc%-20s:\tn \tc[\tW====================\tc]\tn Permanent \tc|\tn\r\n", spell_name);
    } else if (aff->duration == 0) {
      // About to expire
      send_to_char(ch, "\tc|\tn \tc%-20s:\tn \tc[\tr!                  \tc]\tn Expiring! \tc|\tn\r\n", spell_name);
    } else {
      // Calculate filled portion of bar
      // Assume max duration is 24 hours (24 * 60 = 1440 ticks)
      int max_duration = 1440;
      filled_length = (aff->duration * bar_length) / max_duration;
      if (filled_length > bar_length) filled_length = bar_length;
      if (filled_length < 1 && aff->duration > 0) filled_length = 1;
      
      // Determine color based on remaining time
      if (aff->duration > 60) {
        color = "\tG"; // Green for > 1 hour
      } else if (aff->duration > 10) {
        color = "\tY"; // Yellow for > 10 minutes
      } else {
        color = "\tR"; // Red for <= 10 minutes
      }
      
      // Build progress bar
      send_to_char(ch, "\tc|\tn \tc%-20s:\tn \tc[%s", spell_name, color);
      for (i = 0; i < filled_length; i++) {
        send_to_char(ch, "=");
      }
      send_to_char(ch, "\tn");
      for (i = filled_length; i < bar_length; i++) {
        send_to_char(ch, " ");
      }
      send_to_char(ch, "\tc]\tn %3d min \tc|\tn\r\n", aff->duration);
    }
  }
  
  if (count == 0) {
    send_to_char(ch, "\tc|\tn \tcNo active effects\tn                                                     \tc|\tn\r\n");
  }
  
  send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
}

/* Get the current display context for a character */
static int get_display_context(struct char_data *ch)
{
  /* Combat takes highest priority */
  if (ch->char_specials.fighting) {
    return CONTEXT_COMBAT;
  }
  
  /* Check if character is moving/exploring */
  if (AFF_FLAGGED(ch, AFF_SNEAK) || AFF_FLAGGED(ch, AFF_HIDE)) {
    return CONTEXT_EXPLORING;
  }
  
  /* Check for roleplay mode - preference flag */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_RP)) {
    return CONTEXT_ROLEPLAY;
  }
  
  /* Default context */
  return CONTEXT_NORMAL;
}

ACMD(do_skore)
{
  PERF_PROF_ENTER(pr_skore_, "do_skore");
  
  int line_length = 80;
  int i = 0;

  /* Check for section-specific display */
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg, sizeof(arg));
  
  /* Check for classic score preference */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SCORE_CLASSIC)) {
    PERF_PROF_EXIT(pr_skore_);
    do_score(ch, argument, cmd, subcmd);
    return;
  }

  /* Determine display width based on preferences */
  if (!IS_NPC(ch)) {
    int pref_width = GET_SCORE_DISPLAY_WIDTH(ch);
    if (pref_width == 120 || PRF_FLAGGED(ch, PRF_SCORE_WIDE)) {
      line_length = 120;
    } else if (pref_width == 160) {
      line_length = 160;
    } else {
      line_length = 80; /* Default */
    }
  }

  /* Handle section-specific displays */
  if (*arg) {
    if (!str_cmp(arg, "combat")) {
      /* Show detailed combat information */
      int calc_bab = MIN(MAX_BAB, ACTUAL_BAB(ch));
      struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD_1);
      int w_type = 0;
      
      if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
        w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
      else {
        if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
          w_type = ch->mob_specials.attack_type + TYPE_HIT;
        else
          w_type = TYPE_HIT;
      }
      
      skore_section_header(ch, "\tR*** DETAILED COMBAT STATISTICS ***\tC", line_length, "\tC");
      
      /* Calculate detailed combat stats */
      int armor_class = compute_armor_class(NULL, ch, FALSE, MODE_ARMOR_CLASS_NORMAL);
      int num_attacks = 1 + MAX(0, (calc_bab - 1) / 5);
      
      send_to_char(ch, "\tc+-- Basic Combat Stats ---------------------------------------------------------+\tn\r\n");
      send_to_char(ch, "\tc|\tn \tcBase Attack Bonus:\tn %-4d                                              \tc|\tn\r\n", calc_bab);
      send_to_char(ch, "\tc|\tn \tcNumber of Attacks:\tn %-3d                                               \tc|\tn\r\n", num_attacks);
      send_to_char(ch, "\tc|\tn \tcArmor Class:\tn %-3d                                                      \tc|\tn\r\n", armor_class);
      send_to_char(ch, "\tc|\tn \tcHitroll:\tn %s%-3d \tc|\tn \tcDamroll:\tn %s%-3d                                    \tc|\tn\r\n",
                   GET_HITROLL(ch) >= 0 ? "+" : "", GET_HITROLL(ch),
                   GET_DAMROLL(ch) >= 0 ? "+" : "", GET_DAMROLL(ch));
      
      send_to_char(ch, "\tc+-- Damage Reduction & Resistance ----------------------------------------------+\tn\r\n");
      send_to_char(ch, "\tc|\tn \tcDamage Reduction:\tn %-3d                                                  \tc|\tn\r\n",
                   compute_damage_reduction(ch, DAM_RESERVED_DBC));
      send_to_char(ch, "\tc|\tn \tcSpell Resistance:\tn %-3d                                                  \tc|\tn\r\n",
                   compute_spell_res(ch, ch, 0));
      
      send_to_char(ch, "\tc+-- Weapon Information ---------------------------------------------------------+\tn\r\n");
      if (wielded) {
        send_to_char(ch, "\tc|\tn \tcWeapon:\tn %-54s \tc|\tn\r\n", wielded->short_description);
        send_to_char(ch, "\tc|\tn \tcDamage:\tn %dd%d%s%d                                                     \tc|\tn\r\n",
                     GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2),
                     GET_DAMROLL(ch) >= 0 ? "+" : "", GET_DAMROLL(ch));
        send_to_char(ch, "\tc|\tn \tcDamage Type:\tn %-20s                                      \tc|\tn\r\n",
                     attack_hit_text[w_type - TYPE_HIT].singular);
      } else {
        send_to_char(ch, "\tc|\tn \tcWeapon:\tn Unarmed                                                      \tc|\tn\r\n");
        send_to_char(ch, "\tc|\tn \tcDamage:\tn 1d3%s%d                                                       \tc|\tn\r\n",
                     GET_DAMROLL(ch) >= 0 ? "+" : "", GET_DAMROLL(ch));
      }
      
      send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
      send_to_char(ch, "\tC");
      draw_line(ch, line_length, '=', '=');
      send_to_char(ch, "\tn");
      
      PERF_PROF_EXIT(pr_skore_);
      return;
    }
    else if (!str_cmp(arg, "magic")) {
      // Show detailed magic information
      skore_section_header(ch, "\tB*** DETAILED MAGIC & PSIONICS ***\tC", line_length, "\tC");
      
      if (!IS_SPELLCASTER(ch) && GET_PSIONIC_LEVEL(ch) <= 0) {
        send_to_char(ch, "\tcYou have no magical or psionic abilities.\tn\r\n");
      } else {
        if (IS_SPELLCASTER(ch)) {
          send_to_char(ch, "\tc+-- Spellcasting Details -------------------------------------------------------+\tn\r\n");
          send_to_char(ch, "\tc|\tn \tcSpell DC Bonus:\tn %-3d                                                    \tc|\tn\r\n",
                       get_spell_dc_bonus(ch));
          send_to_char(ch, "\tc|\tn \tcSpell Potency:\tn %-3d%%                                                    \tc|\tn\r\n",
                       get_spell_potency_bonus(ch));
          send_to_char(ch, "\tc|\tn \tcSpell Duration:\tn %-3d%%                                                   \tc|\tn\r\n",
                       get_spell_duration_bonus(ch));
          
          // Show detailed spell slots by class
          send_to_char(ch, "\tc+-- Spell Slots by Class -------------------------------------------------------+\tn\r\n");
          for (i = 0; i < MAX_CLASSES; i++) {
            if (CLASS_LEVEL(ch, i) > 0) {
              // Check if this class has any spell slots
              int has_spells = FALSE;
              int circle;
              for (circle = 1; circle <= 9; circle++) {
                if (compute_slots_by_circle(ch, i, circle) > 0) {
                  has_spells = TRUE;
                  break;
                }
              }
              if (!has_spells) continue;
              send_to_char(ch, "\tc|\tn \tc%s (Level %d):\tn\r\n", 
                           class_list[i].name, CLASS_LEVEL(ch, i));
              for (circle = 1; circle <= 9; circle++) {
                int total_slots = compute_slots_by_circle(ch, i, circle);
                if (total_slots > 0) {
                  int used_slots = count_circle_collection(ch, i, circle) +
                                  count_circle_innate_magic(ch, i, circle) +
                                  count_circle_prep_queue(ch, i, circle);
                  int remaining = total_slots - used_slots;
                  
                  send_to_char(ch, "\tc|\tn   Circle %d: %d/%d slots available                                           \tc|\tn\r\n",
                               circle, remaining, total_slots);
                }
              }
            }
          }
          send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
        }
        
        if (GET_PSIONIC_LEVEL(ch) > 0) {
          send_to_char(ch, "\tc+-- Psionic Details ------------------------------------------------------------+\tn\r\n");
          send_to_char(ch, "\tc|\tn \tcPsionic Level:\tn %-3d                                                     \tc|\tn\r\n",
                       GET_PSIONIC_LEVEL(ch));
          send_to_char(ch, "\tc|\tn \tcEnergy Type:\tn %-20s                                      \tc|\tn\r\n",
                       damtypes[GET_PSIONIC_ENERGY_TYPE(ch)]);
          send_to_char(ch, "\tc|\tn \tcMax Augment PSP:\tn %-3d per power                                         \tc|\tn\r\n",
                       base_augment_psp_allowed(ch));
          send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
        }
      }
      
      send_to_char(ch, "\tC");
      draw_line(ch, line_length, '=', '=');
      send_to_char(ch, "\tn");
      
      PERF_PROF_EXIT(pr_skore_);
      return;
    }
    else if (!str_cmp(arg, "stats")) {
      // Show detailed ability scores and saves
      skore_section_header(ch, "\tG*** DETAILED ABILITIES & SAVES ***\tC", line_length, "\tC");
      
      send_to_char(ch, "\tc+-- Ability Scores & Modifiers ------------------------------------------------+\tn\r\n");
      send_to_char(ch, "\tc|\tn \tcSTR:\tn %2d (%s%d) \tc|\tn \tcDEX:\tn %2d (%s%d) \tc|\tn \tcCON:\tn %2d (%s%d) \tc|\tn\r\n",
                   GET_STR(ch), GET_STR_BONUS(ch) >= 0 ? "+" : "", GET_STR_BONUS(ch),
                   GET_DEX(ch), GET_DEX_BONUS(ch) >= 0 ? "+" : "", GET_DEX_BONUS(ch),
                   GET_CON(ch), GET_CON_BONUS(ch) >= 0 ? "+" : "", GET_CON_BONUS(ch));
      send_to_char(ch, "\tc|\tn \tcINT:\tn %2d (%s%d) \tc|\tn \tcWIS:\tn %2d (%s%d) \tc|\tn \tcCHA:\tn %2d (%s%d) \tc|\tn\r\n",
                   GET_INT(ch), GET_INT_BONUS(ch) >= 0 ? "+" : "", GET_INT_BONUS(ch),
                   GET_WIS(ch), GET_WIS_BONUS(ch) >= 0 ? "+" : "", GET_WIS_BONUS(ch),
                   GET_CHA(ch), GET_CHA_BONUS(ch) >= 0 ? "+" : "", GET_CHA_BONUS(ch));
      
      send_to_char(ch, "\tc+-- Saving Throws -------------------------------------------------------------+\tn\r\n");
      send_to_char(ch, "\tc|\tn \tcFortitude:\tn %s%-3d \tc|\tn \tcReflex:\tn %s%-3d \tc|\tn \tcWill:\tn %s%-3d                  \tc|\tn\r\n",
                   compute_mag_saves(ch, SAVING_FORT, 0) >= 0 ? "+" : "", compute_mag_saves(ch, SAVING_FORT, 0),
                   compute_mag_saves(ch, SAVING_REFL, 0) >= 0 ? "+" : "", compute_mag_saves(ch, SAVING_REFL, 0),
                   compute_mag_saves(ch, SAVING_WILL, 0) >= 0 ? "+" : "", compute_mag_saves(ch, SAVING_WILL, 0));
      
      send_to_char(ch, "\tc+-- Skill Points & Languages --------------------------------------------------+\tn\r\n");
      send_to_char(ch, "\tc|\tn \tcAvailable Skill Points:\tn %-3d                                            \tc|\tn\r\n",
                   GET_PRACTICES(ch));
      send_to_char(ch, "\tc|\tn \tcLanguages Known:\tn %-3d                                                   \tc|\tn\r\n",
                   GET_SKILL(ch, SKILL_LANG_COMMON) ? 1 : 0); // This is simplified - would need more work
      
      send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
      send_to_char(ch, "\tC");
      draw_line(ch, line_length, '=', '=');
      send_to_char(ch, "\tn");
      
      PERF_PROF_EXIT(pr_skore_);
      return;
    }
    else {
      send_to_char(ch, "Valid sections are: combat, magic, stats\r\n");
      send_to_char(ch, "Use 'skore' without arguments for the full display.\r\n");
      PERF_PROF_EXIT(pr_skore_);
      return;
    }
  }

  /* Empty - section ordering handled below */

  // ═══════════════════════════════════════════════════════════════════════════════
  // HEADER WITH CLASS BORDERS
  // ═══════════════════════════════════════════════════════════════════════════════
  
  const char **border_style = get_class_border_style(ch);
  const char *class_color = "\tc";
  
  if (border_style) {
    // Get the color code from the border
    if (strstr(warrior_border[0], border_style[0])) {
      class_color = "\tR";
    } else if (strstr(mage_border[0], border_style[0])) {
      class_color = "\tB";
    } else if (strstr(cleric_border[0], border_style[0])) {
      class_color = "\tG";
    } else if (strstr(rogue_border[0], border_style[0])) {
      class_color = "\tM";
    }
    
    // Display top border
    send_to_char(ch, "%s\r\n", border_style[0]);
    
    // Display title with border sides
    send_to_char(ch, "%s║\tn                  \tW*** ENHANCED CHARACTER INFORMATION ***\tn                   %s║\tn\r\n", 
                 class_color, class_color);
    
    // Display bottom border
    send_to_char(ch, "%s\r\n", border_style[2]);
    send_to_char(ch, "\r\n");
  }

  /* Display sections based on layout ordering */
  const byte *section_order;
  int section_idx;
  bool needs_init = TRUE;
  bool has_custom_order = FALSE;
  byte section_count[8] = {0};
  int j, section;
  
  /* Get section order based on player's preferences or context */
  if (!IS_NPC(ch)) {
    /* First, check if section order is properly initialized */
    for (i = 0; i < 8; i++) {
      if (GET_SCORE_SECTION_ORDER(ch, i) >= 0 && GET_SCORE_SECTION_ORDER(ch, i) < 8) {
        needs_init = FALSE;
        break;
      }
    }
    
    /* Initialize if needed - this handles legacy players */
    if (needs_init) {
      for (i = 0; i < 8; i++) {
        GET_SCORE_SECTION_ORDER(ch, i) = i;
      }
    }
    
    /* Additional validation: ensure all sections are present and unique */
    for (i = 0; i < 8; i++) {
      section_count[i] = 0;
    }
    for (i = 0; i < 8; i++) {
      section = GET_SCORE_SECTION_ORDER(ch, i);
      if (section >= 0 && section < 8) {
        section_count[section]++;
      }
    }
    
    /* If any section is missing or duplicated, reset to default */
    for (i = 0; i < 8; i++) {
      if (section_count[i] != 1) {
        /* Reset to default order */
        for (j = 0; j < 8; j++) {
          GET_SCORE_SECTION_ORDER(ch, j) = j;
        }
        break;
      }
    }
    
    /* Check if player has custom section order set */
    for (i = 0; i < 8; i++) {
      if (GET_SCORE_SECTION_ORDER(ch, i) != i) {
        has_custom_order = TRUE;
        break;
      }
    }
    
    if (has_custom_order) {
      /* Use custom section order */
      section_order = ch->player_specials->saved.score_section_order;
    } else {
      /* Use template order based on layout or context */
      int context = get_display_context(ch);
      
      /* Check if player has set a layout template */
      if (GET_SCORE_LAYOUT_TEMPLATE(ch) != LAYOUT_DEFAULT) {
        section_order = get_template_section_order(GET_SCORE_LAYOUT_TEMPLATE(ch));
      } else {
        /* Use context-based ordering */
        switch (context) {
          case CONTEXT_COMBAT:
            section_order = layout_combat;
            break;
          case CONTEXT_SHOPPING:
            section_order = layout_roleplay;
            break;
          case CONTEXT_EXPLORING:
            section_order = layout_explorer;
            break;
          case CONTEXT_ROLEPLAY:
            section_order = layout_roleplay;
            break;
          default:
            section_order = layout_default;
            break;
        }
      }
    }
  } else {
    /* NPCs use default ordering */
    section_order = layout_default;
  }
  
  /* Display each section in the specified order */
  for (section_idx = 0; section_idx < 8; section_idx++) {
    display_score_section(ch, section_order[section_idx], line_length);
  }

  // ═══════════════════════════════════════════════════════════════════════════════
  // FOOTER
  // ═══════════════════════════════════════════════════════════════════════════════

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '=', '=');
  send_to_char(ch, "\tn");
  send_to_char(ch, "\tcUse 'scoreconfig' to customize your display settings.\tn\r\n");
  
  PERF_PROF_EXIT(pr_skore_);
}

/* Display a specific score section based on section ID */
static void display_score_section(struct char_data *ch, int section_id, int line_length)
{
  switch (section_id) {
    case SECTION_IDENTITY:
      display_identity_section(ch, line_length);
      break;
    case SECTION_VITALS:
      display_vitals_section(ch, line_length);
      break;
    case SECTION_EXPERIENCE:
      display_experience_section(ch, line_length);
      break;
    case SECTION_ABILITIES:
      display_abilities_section(ch, line_length);
      break;
    case SECTION_COMBAT:
      display_combat_section(ch, line_length);
      break;
    case SECTION_MAGIC:
      if (IS_SPELLCASTER(ch) || GET_PSIONIC_LEVEL(ch) > 0) {
        display_magic_section(ch, line_length);
      }
      break;
    case SECTION_WEALTH:
      display_wealth_section(ch, line_length);
      break;
    case SECTION_EQUIPMENT:
      if (IS_NPC(ch) || GET_SCORE_INFO_DENSITY(ch) == 0) { /* Full density only */
        display_equipment_section(ch, line_length);
      }
      break;
  }
}

/* Score configuration command */
ACMD(do_scoreconfig)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  if (IS_NPC(ch)) {
    send_to_char(ch, "NPCs cannot configure score display.\r\n");
    return;
  }

  /* Use half_chop_c instead of two_arguments to preserve "on" as a value */
  half_chop_c(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1) {
    send_to_char(ch, "\tcScore Display Configuration:\tn\r\n");
    send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
    send_to_char(ch, "\tc|\tn \tcCurrent Settings:\tn                                                     \tc|\tn\r\n");
    send_to_char(ch, "\tc|\tn   \tcWidth:\tn %-3d characters                                           \tc|\tn\r\n",
                 GET_SCORE_DISPLAY_WIDTH(ch) ? GET_SCORE_DISPLAY_WIDTH(ch) : 80);
    send_to_char(ch, "\tc|\tn   \tcTheme:\tn %-15s                                           \tc|\tn\r\n",
                 GET_SCORE_COLOR_THEME(ch) == SCORE_THEME_CLASSIC ? "Classic" :
                 GET_SCORE_COLOR_THEME(ch) == SCORE_THEME_MINIMAL ? "Minimal" :
                 GET_SCORE_COLOR_THEME(ch) == SCORE_THEME_HIGHCONTRAST ? "High Contrast" :
                 GET_SCORE_COLOR_THEME(ch) == SCORE_THEME_DARK ? "Dark" :
                 GET_SCORE_COLOR_THEME(ch) == SCORE_THEME_COLORBLIND ? "Colorblind" : "Enhanced");
    send_to_char(ch, "\tc|\tn   \tcDensity:\tn %-15s                                         \tc|\tn\r\n",
                 GET_SCORE_INFO_DENSITY(ch) == 1 ? "Compact" :
                 GET_SCORE_INFO_DENSITY(ch) == 2 ? "Minimal" : "Full");
    send_to_char(ch, "\tc|\tn   \tcClassic Mode:\tn %-3s                                              \tc|\tn\r\n",
                 PRF_FLAGGED(ch, PRF_SCORE_CLASSIC) ? "ON" : "OFF");
    send_to_char(ch, "\tc|\tn   \tcColors:\tn %-3s                                                   \tc|\tn\r\n",
                 PRF_FLAGGED(ch, PRF_SCORE_NOCOLOR) ? "OFF" : "ON");
    send_to_char(ch, "\tc|\tn   \tcBorders:\tn %-3s                                                  \tc|\tn\r\n",
                 PRF_FLAGGED(ch, PRF_SCORE_BORDERS) ? "ON" : "OFF");
    send_to_char(ch, "\tc|\tn   \tcRace Symbols:\tn %-3s                                             \tc|\tn\r\n",
                 PRF_FLAGGED(ch, PRF_SCORE_RACE_SYMBOLS) ? "ON" : "OFF");
    send_to_char(ch, "\tc|\tn   \tcLayout Template:\tn %-15s                                     \tc|\tn\r\n",
                 GET_SCORE_LAYOUT_TEMPLATE(ch) == LAYOUT_COMBAT ? "Combat" :
                 GET_SCORE_LAYOUT_TEMPLATE(ch) == LAYOUT_ROLEPLAY ? "Roleplay" :
                 GET_SCORE_LAYOUT_TEMPLATE(ch) == LAYOUT_EXPLORER ? "Explorer" :
                 GET_SCORE_LAYOUT_TEMPLATE(ch) == LAYOUT_CASTER ? "Caster" : "Default");
    send_to_char(ch, "\tc+----------------------------------------------------------------------------+\tn\r\n");
    send_to_char(ch, "\r\n\tcUsage:\tn\r\n");
    send_to_char(ch, "  \tcscoreconfig width <80|120|160>\tn     - Set display width\r\n");
    send_to_char(ch, "  \tcscoreconfig theme <enhanced|classic|minimal|highcontrast|dark|colorblind>\tn\r\n");
    send_to_char(ch, "  \tcscoreconfig density <full|compact|minimal>\tn - Set information density\r\n");
    send_to_char(ch, "  \tcscoreconfig classic <on/yes|off/no>\tn  - Toggle classic score display\r\n");
    send_to_char(ch, "  \tcscoreconfig colors <on/yes|off/no>\tn   - Toggle color display\r\n");
    send_to_char(ch, "  \tcscoreconfig borders <on/yes|off/no>\tn  - Toggle class-themed borders\r\n");
    send_to_char(ch, "  \tcscoreconfig symbols <on/yes|off/no>\tn  - Toggle race symbols display\r\n");
    send_to_char(ch, "  \tcscoreconfig template <default|combat|roleplay|explorer|caster>\tn\r\n");
    send_to_char(ch, "  \tcscoreconfig order <section> <position>\tn - Set custom section order\r\n");
    send_to_char(ch, "  \tcscoreconfig reset\tn                   - Reset to defaults\r\n");
    send_to_char(ch, "\r\n\tcSection names:\tn identity, vitals, experience, abilities, combat, magic, wealth, equipment\r\n");
    send_to_char(ch, "\tcPositions:\tn 1-8 (1 = first displayed, 8 = last displayed)\r\n");
    return;
  }

  if (!str_cmp(arg1, "width")) {
    int width = atoi(arg2);
    if (width != 80 && width != 120 && width != 160) {
      send_to_char(ch, "Valid widths are: 80, 120, or 160 characters.\r\n");
      return;
    }
    GET_SCORE_DISPLAY_WIDTH(ch) = width;

    // Update preference flags for compatibility
    if (width == 120 || width == 160) {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_WIDE);
    } else {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_WIDE);
    }

    send_to_char(ch, "Score display width set to %d characters.\r\n", width);
    save_char(ch, 0);
    return;
  }

  if (!str_cmp(arg1, "theme")) {
    if (!str_cmp(arg2, "enhanced") || !str_cmp(arg2, "default")) {
      GET_SCORE_COLOR_THEME(ch) = SCORE_THEME_ENHANCED;
      send_to_char(ch, "Score color theme set to Enhanced.\r\n");
    } else if (!str_cmp(arg2, "classic")) {
      GET_SCORE_COLOR_THEME(ch) = SCORE_THEME_CLASSIC;
      send_to_char(ch, "Score color theme set to Classic.\r\n");
    } else if (!str_cmp(arg2, "minimal")) {
      GET_SCORE_COLOR_THEME(ch) = SCORE_THEME_MINIMAL;
      send_to_char(ch, "Score color theme set to Minimal.\r\n");
    } else if (!str_cmp(arg2, "highcontrast") || !str_cmp(arg2, "high-contrast")) {
      GET_SCORE_COLOR_THEME(ch) = SCORE_THEME_HIGHCONTRAST;
      send_to_char(ch, "Score color theme set to High Contrast.\r\n");
    } else if (!str_cmp(arg2, "dark")) {
      GET_SCORE_COLOR_THEME(ch) = SCORE_THEME_DARK;
      send_to_char(ch, "Score color theme set to Dark.\r\n");
    } else if (!str_cmp(arg2, "colorblind") || !str_cmp(arg2, "color-blind")) {
      GET_SCORE_COLOR_THEME(ch) = SCORE_THEME_COLORBLIND;
      send_to_char(ch, "Score color theme set to Colorblind.\r\n");
    } else {
      send_to_char(ch, "Valid themes are: enhanced, classic, minimal, highcontrast, dark, or colorblind.\r\n");
      return;
    }
    save_char(ch, 0);
    return;
  }

  if (!str_cmp(arg1, "density")) {
    if (!str_cmp(arg2, "full") || !str_cmp(arg2, "default")) {
      GET_SCORE_INFO_DENSITY(ch) = 0;
      send_to_char(ch, "Score information density set to Full.\r\n");
    } else if (!str_cmp(arg2, "compact")) {
      GET_SCORE_INFO_DENSITY(ch) = 1;
      send_to_char(ch, "Score information density set to Compact.\r\n");
    } else if (!str_cmp(arg2, "minimal")) {
      GET_SCORE_INFO_DENSITY(ch) = 2;
      send_to_char(ch, "Score information density set to Minimal.\r\n");
    } else {
      send_to_char(ch, "Valid densities are: full, compact, or minimal.\r\n");
      return;
    }
    save_char(ch, 0);
    return;
  }

  if (!str_cmp(arg1, "classic")) {
    if (!str_cmp(arg2, "on") || !str_cmp(arg2, "yes")) {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_CLASSIC);
      send_to_char(ch, "Classic score display enabled. Use 'score' for classic display.\r\n");
    } else if (!str_cmp(arg2, "off") || !str_cmp(arg2, "no")) {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_CLASSIC);
      send_to_char(ch, "Classic score display disabled. Enhanced display will be used.\r\n");
    } else {
      send_to_char(ch, "Use 'on/yes' or 'off/no' to toggle classic mode.\r\n");
      return;
    }
    save_char(ch, 0);
    return;
  }

  if (!str_cmp(arg1, "colors")) {
    if (!str_cmp(arg2, "on") || !str_cmp(arg2, "yes")) {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_NOCOLOR);
      send_to_char(ch, "Score display colors enabled.\r\n");
    } else if (!str_cmp(arg2, "off") || !str_cmp(arg2, "no")) {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_NOCOLOR);
      send_to_char(ch, "Score display colors disabled.\r\n");
    } else {
      send_to_char(ch, "Use 'on/yes' or 'off/no' to toggle colors.\r\n");
      return;
    }
    save_char(ch, 0);
    return;
  }

  if (!str_cmp(arg1, "borders")) {
    /* DEBUG: Log what we're getting */
    log("DEBUG scoreconfig borders: arg2='%s' len=%d", arg2, (int)strlen(arg2));
    if (!str_cmp(arg2, "on") || !str_cmp(arg2, "yes")) {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_BORDERS);
      send_to_char(ch, "Class-themed borders enabled in score display.\r\n");
    } else if (!str_cmp(arg2, "off") || !str_cmp(arg2, "no")) {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_BORDERS);
      send_to_char(ch, "Class-themed borders disabled in score display.\r\n");
    } else {
      send_to_char(ch, "Use 'on/yes' or 'off/no' to toggle borders.\r\n");
      return;
    }
    save_char(ch, 0);
    return;
  }

  if (!str_cmp(arg1, "symbols")) {
    if (!str_cmp(arg2, "on") || !str_cmp(arg2, "yes")) {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_RACE_SYMBOLS);
      send_to_char(ch, "Race symbols enabled in score display.\r\n");
    } else if (!str_cmp(arg2, "off") || !str_cmp(arg2, "no")) {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_RACE_SYMBOLS);
      send_to_char(ch, "Race symbols disabled in score display.\r\n");
    } else {
      send_to_char(ch, "Use 'on/yes' or 'off/no' to toggle race symbols.\r\n");
      return;
    }
    save_char(ch, 0);
    return;
  }

  if (!str_cmp(arg1, "template")) {
    if (!str_cmp(arg2, "default")) {
      GET_SCORE_LAYOUT_TEMPLATE(ch) = LAYOUT_DEFAULT;
      send_to_char(ch, "Score layout template set to Default.\r\n");
    } else if (!str_cmp(arg2, "combat")) {
      GET_SCORE_LAYOUT_TEMPLATE(ch) = LAYOUT_COMBAT;
      send_to_char(ch, "Score layout template set to Combat (prioritizes combat stats).\r\n");
    } else if (!str_cmp(arg2, "roleplay") || !str_cmp(arg2, "rp")) {
      GET_SCORE_LAYOUT_TEMPLATE(ch) = LAYOUT_ROLEPLAY;
      send_to_char(ch, "Score layout template set to Roleplay (prioritizes identity).\r\n");
    } else if (!str_cmp(arg2, "explorer") || !str_cmp(arg2, "explore")) {
      GET_SCORE_LAYOUT_TEMPLATE(ch) = LAYOUT_EXPLORER;
      send_to_char(ch, "Score layout template set to Explorer (prioritizes movement).\r\n");
    } else if (!str_cmp(arg2, "caster") || !str_cmp(arg2, "magic")) {
      GET_SCORE_LAYOUT_TEMPLATE(ch) = LAYOUT_CASTER;
      send_to_char(ch, "Score layout template set to Caster (prioritizes magic).\r\n");
    } else {
      send_to_char(ch, "Valid templates are: default, combat, roleplay, explorer, or caster.\r\n");
      return;
    }
    save_char(ch, 0);
    return;
  }

  if (!str_cmp(arg1, "order")) {
    char arg3[MAX_INPUT_LENGTH];
    int section_id = -1;
    int position, old_position;
    int i;
    
    /* Get third argument */
    half_chop(arg2, arg2, arg3);
    
    if (!*arg2 || !*arg3) {
      send_to_char(ch, "Usage: scoreconfig order <section> <position>\r\n");
      send_to_char(ch, "Sections: identity, vitals, experience, abilities, combat, magic, wealth, equipment\r\n");
      send_to_char(ch, "Positions: 1-8\r\n");
      return;
    }
    
    /* Parse section name */
    if (!str_cmp(arg2, "identity")) section_id = SECTION_IDENTITY;
    else if (!str_cmp(arg2, "vitals")) section_id = SECTION_VITALS;
    else if (!str_cmp(arg2, "experience") || !str_cmp(arg2, "exp")) section_id = SECTION_EXPERIENCE;
    else if (!str_cmp(arg2, "abilities") || !str_cmp(arg2, "stats")) section_id = SECTION_ABILITIES;
    else if (!str_cmp(arg2, "combat")) section_id = SECTION_COMBAT;
    else if (!str_cmp(arg2, "magic") || !str_cmp(arg2, "spells")) section_id = SECTION_MAGIC;
    else if (!str_cmp(arg2, "wealth") || !str_cmp(arg2, "gold")) section_id = SECTION_WEALTH;
    else if (!str_cmp(arg2, "equipment") || !str_cmp(arg2, "gear")) section_id = SECTION_EQUIPMENT;
    else {
      send_to_char(ch, "Invalid section name. Valid sections: identity, vitals, experience, abilities, combat, magic, wealth, equipment\r\n");
      return;
    }
    
    /* Parse position */
    position = atoi(arg3);
    if (position < 1 || position > 8) {
      send_to_char(ch, "Position must be between 1 and 8.\r\n");
      return;
    }
    position--; /* Convert to 0-based */
    
    /* Find current position of this section */
    old_position = -1;
    for (i = 0; i < 8; i++) {
      if (GET_SCORE_SECTION_ORDER(ch, i) == section_id) {
        old_position = i;
        break;
      }
    }
    
    /* If section not found in current order, initialize order first */
    if (old_position == -1) {
      for (i = 0; i < 8; i++) {
        GET_SCORE_SECTION_ORDER(ch, i) = i;
      }
      old_position = section_id;
    }
    
    /* Swap positions */
    if (old_position != position) {
      byte temp = GET_SCORE_SECTION_ORDER(ch, position);
      GET_SCORE_SECTION_ORDER(ch, position) = section_id;
      GET_SCORE_SECTION_ORDER(ch, old_position) = temp;
    }
    
    send_to_char(ch, "Section '%s' moved to position %d.\r\n", arg2, position + 1);
    save_char(ch, 0);
    return;
  }

  if (!str_cmp(arg1, "reset")) {
    int i;
    GET_SCORE_DISPLAY_WIDTH(ch) = PFDEF_SCORE_DISPLAY_WIDTH;
    GET_SCORE_COLOR_THEME(ch) = PFDEF_SCORE_COLOR_THEME;
    GET_SCORE_INFO_DENSITY(ch) = PFDEF_SCORE_INFO_DENSITY;
    GET_SCORE_LAYOUT_TEMPLATE(ch) = LAYOUT_DEFAULT;
    /* Reset custom section order to default */
    for (i = 0; i < 8; i++) {
      GET_SCORE_SECTION_ORDER(ch, i) = i;
    }
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_CLASSIC);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_NOCOLOR);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_WIDE);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_BORDERS);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_SCORE_RACE_SYMBOLS);
    send_to_char(ch, "Score display configuration reset to defaults.\r\n");
    save_char(ch, 0);
    return;
  }

  send_to_char(ch, "Unknown option. Use 'scoreconfig' without arguments for help.\r\n");
}

ACMD(do_inventory)
{
  send_to_char(ch, "You are carrying:\r\n");
  list_obj_to_char_full(ch->carrying, ch, SHOW_OBJ_SHORT, TRUE, 1, true);

  if (!IS_NPC(ch))
    if (ch->desc)
      if (ch->desc->pProtocol)
        if (ch->desc->pProtocol->pVariables[eMSDP_MXP])
          if (ch->desc->pProtocol->pVariables[eMSDP_MXP]->ValueInt)
            send_to_char(ch, "\r\n\t<send href='equipment'>View equipped items\t</send>\r\n");

  if (!IS_NPC(ch))
  {
    send_to_char(ch, "\r\n\tcUse the 'bags' command to see what is in your bags. See help files for: GET, PUT and SORT.\tn\r\n");
  }
}

int count_bag_contents(struct char_data *ch, int bagnum)
{
  struct obj_data *obj;
  int count = 0;

  switch (bagnum)
  {
    case 1:
      for (obj = ch->bags->bag1; obj; obj = obj->next_content)
        count++;
      return count;
    case 2:
      for (obj = ch->bags->bag2; obj; obj = obj->next_content)
        count++;
      return count;
    case 3:
      for (obj = ch->bags->bag3; obj; obj = obj->next_content)
        count++;
      return count;
    case 4:
      for (obj = ch->bags->bag4; obj; obj = obj->next_content)
        count++;
      return count;
    case 5:
      for (obj = ch->bags->bag5; obj; obj = obj->next_content)
        count++;
      return count;
    case 6:
      for (obj = ch->bags->bag6; obj; obj = obj->next_content)
        count++;
      return count;
    case 7:
      for (obj = ch->bags->bag7; obj; obj = obj->next_content)
        count++;
      return count;
    case 8:
      for (obj = ch->bags->bag8; obj; obj = obj->next_content)
        count++;
      return count;
    case 9:
      for (obj = ch->bags->bag9; obj; obj = obj->next_content)
        count++;
      return count;
    case 10:
      for (obj = ch->bags->bag10; obj; obj = obj->next_content)
        count++;
      return count;
  }
  return count;
}

void show_bags_summary(struct char_data *ch)
{
  if (!ch) return;

  char out[MEDIUM_STRING];
  int i = 0;

  send_to_char(ch, "Your bags summary:\r\n\r\n");
  for (i = 1; i <= MAX_BAGS; i++)
  {
    snprintf(out, sizeof(out), "Bag %s#%d: %3d Items (%s)\r\n", i < 10 ? " " : "", i, count_bag_contents(ch, i), bagnames[i-1]);
    send_to_char(ch, "%s", out);
  }

}

#define BAGS_CMD_SYNTAX "\r\nUsage is: bags (bag number)|(bag name)\r\nEg. bags 1 - will show contents of bag 1.\r\nEg. bags materials - will show contents of a bag named 'materials'\r\n"

ACMD(do_bags)
{
  char arg[MEDIUM_STRING] = {'\0'};
  // char bagname[MEDIUM_STRING] = {'\0'};
  int bagnum = 0, i = 0;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    show_bags_summary(ch);
    send_to_char(ch, "%s", BAGS_CMD_SYNTAX);
    return;
  }

  for (i = 1; i <= MAX_BAGS; i++)
  {
    if (GET_BAG_NAME(ch, i)  == NULL || !strcmp(GET_BAG_NAME(ch, i), "unused"))
      continue;
    if (is_abbrev(arg, GET_BAG_NAME(ch, i)))
      break;
  }

  bagnum = i;

  if (i > MAX_BAGS)
  {
    bagnum = atoi(arg);
  }

  if (bagnum < 1 || bagnum > MAX_BAGS)
  {
    send_to_char(ch, "%s", BAGS_CMD_SYNTAX);
    return;
  }

  // if (GET_BAG_NAME(ch, i)  != NULL)
  //   snprintf(bagname, sizeof(bagname), " '%s'", GET_BAG_NAME(ch, i));
  // else
  //   snprintf(bagname, sizeof(bagname), " 'bag%d'", bagnum);

  // send_to_char(ch, "Your bag #%d%s contains:\r\n", bagnum, GET_BAG_NAME(ch, i)  != NULL ? bagname : "");
  send_to_char(ch, "Your bag #%d (%s) contains:\r\n", bagnum, bagnames[bagnum-1]);

  switch (bagnum)
  {
    case 1:
      list_obj_to_char(ch->bags->bag1, ch, SHOW_OBJ_SHORT, TRUE, 1);
      break;
    case 2:
      list_obj_to_char(ch->bags->bag2, ch, SHOW_OBJ_SHORT, TRUE, 1);
      break;
    case 3:
      list_obj_to_char(ch->bags->bag3, ch, SHOW_OBJ_SHORT, TRUE, 1);
      break;
    case 4:
      list_obj_to_char(ch->bags->bag4, ch, SHOW_OBJ_SHORT, TRUE, 1);
      break;
    case 5:
      list_obj_to_char(ch->bags->bag5, ch, SHOW_OBJ_SHORT, TRUE, 1);
      break;
    case 6:
      list_obj_to_char(ch->bags->bag6, ch, SHOW_OBJ_SHORT, TRUE, 1);
      break;
    case 7:
      list_obj_to_char(ch->bags->bag7, ch, SHOW_OBJ_SHORT, TRUE, 1);
      break;
    case 8:
      list_obj_to_char(ch->bags->bag8, ch, SHOW_OBJ_SHORT, TRUE, 1);
      break;
    case 9:
      list_obj_to_char(ch->bags->bag9, ch, SHOW_OBJ_SHORT, TRUE, 1);
      break;
    case 10:
      list_obj_to_char(ch->bags->bag10, ch, SHOW_OBJ_SHORT, TRUE, 1);
      break;
  }

}

ACMD(do_equipment)
{
  int i, found = 0;
  int mxp_type = 2;
  char dex_max[20] = "No-Max";
  int j = compute_gear_max_dex(ch);

  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
  {
    send_to_char(ch, "Nekked!\r\n");
    return;
  }

  if (j < 99) // 99 is our signal for no max dex
    snprintf(dex_max, sizeof(dex_max), "%d", j);

  send_to_char(ch, "You are using:\r\n");
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (GET_EQ(ch, eq_ordering_1[i]))
    {
      found = TRUE;
#if defined(CAMPAIGN_DL)
       // In Dragonlance, we always want to be able to see our equipment unless it's invis and we can't see invis
      if (!OBJ_FLAGGED(GET_EQ(ch, eq_ordering_1[i]), ITEM_INVISIBLE) || AFF_FLAGGED(ch, AFF_DETECT_INVIS) || AFF_FLAGGED(ch, AFF_TRUE_SIGHT))
#else
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, eq_ordering_1[i])))
#endif
      {
        send_to_char(ch, "%s", wear_where[eq_ordering_1[i]]);
        /* added this as a clue to players */
        switch (eq_ordering_1[i])
        {
        case WEAR_WIELD_1:
        case WEAR_WIELD_OFFHAND:
        case WEAR_WIELD_2H:
          if (!is_proficient_with_weapon(ch, GET_WEAPON_TYPE(GET_EQ(ch, eq_ordering_1[i]))))
            send_to_char(ch, "(not proficient) ");
          break;
        case WEAR_SHIELD:
          if (!is_proficient_with_shield(ch))
            send_to_char(ch, "(not proficient) ");
          break;
        case WEAR_BODY:
          if (!is_proficient_with_body_armor(ch))
            send_to_char(ch, "(not proficient) ");
          break;
        case WEAR_HEAD:
          if (!is_proficient_with_helm(ch))
            send_to_char(ch, "(not proficient) ");
          break;
        case WEAR_ARMS:
          if (!is_proficient_with_sleeves(ch))
            send_to_char(ch, "(not proficient) ");
          break;
        case WEAR_LEGS:
          if (!is_proficient_with_leggings(ch))
            send_to_char(ch, "(not proficient) ");
          break;
        default:
          break;
        }
        show_obj_to_char(GET_EQ(ch, eq_ordering_1[i]), ch, SHOW_OBJ_SHORT, mxp_type);
      }
      else
      {
        send_to_char(ch, "%s", wear_where[eq_ordering_1[i]]);
        send_to_char(ch, "Something.\r\n");
      }
    }
  }
  if (!found)
    send_to_char(ch, " Nothing.\r\n");

  send_to_char(ch, "\tCArmr: %s, Shld: %s, Ench: +%d, Pnlty: %d, MaxDex: %s, SpellFail:"
                   " %d.\tn\r\n",
               armor_type[compute_gear_armor_type(ch)],
               armor_type[compute_gear_shield_type(ch)],
               compute_gear_enhancement_bonus(ch),
               compute_gear_armor_penalty(ch),
               dex_max,
               compute_gear_spell_failure(ch));

  if (ch && ch->desc && ch->desc->pProtocol && ch->desc->pProtocol->pVariables[eMSDP_MXP] &&
      ch->desc->pProtocol->pVariables[eMSDP_MXP]->ValueInt)
    send_to_char(ch, "\r\n\t<send href='inventory'>View inventory\t</send>\r\n");
}

ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  /* day in [1..35] */
  day = time_info.day + 1;

  /* 35 days in a month, 7 days a week */
  weekday = ((35 * time_info.month) + day) % 7;

  send_to_char(ch, "It is %d o'clock %s, on %s.\r\n",
               (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12),
               time_info.hours >= 12 ? "pm" : "am", weekdays[weekday]);

  /* Peter Ajamian supplied the following as a fix for a bug introduced in the
   * ordinal display that caused 11, 12, and 13 to be incorrectly displayed as
   * 11st, 12nd, and 13rd.  Nate Winters had already submitted a fix, but it
   * hard-coded a limit on ordinal display which I want to avoid. -dak */
  suf = "th";

  if (((day % 100) / 10) != 1)
  {
    switch (day % 10)
    {
    case 1:
      suf = "st";
      break;
    case 2:
      suf = "nd";
      break;
    case 3:
      suf = "rd";
      break;
    }
  }

  send_to_char(ch, "The %d%s Day of the %s, Year %d.\r\n",
               day, suf, month_name[time_info.month], time_info.year);
}

ACMD(do_weather)
{
  const char *sky_look[] = {
      "cloudless",
      "cloudy",
      "rainy",
      "lit by flashes of lightning"};

  if (OUTSIDE(ch))
  {
    send_to_char(ch, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
                 weather_info.change >= 0 ? "you feel a warm wind from south" : "your foot tells you bad weather is due");
    if (GET_LEVEL(ch) >= LVL_STAFF)
      send_to_char(ch, "Pressure: %d (change: %d), Sky: %d (%s)\r\n",
                   weather_info.pressure,
                   weather_info.change,
                   weather_info.sky,
                   sky_look[weather_info.sky]);
  }
  else
    send_to_char(ch, "You have no feeling about the weather at all.\r\n");
}

#define WHO_FORMAT \
  "Usage: who [minlev[-maxlev]] [-n name] [-c classlist] [-t racelist] [-k] [-l] [-n] [-q] [-r] [-s] [-z]\r\n"

/* Written by Rhade */
ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  int i, num_can_see = 0;
#if !defined(CAMPAIGN_DL)  
  int class_len = 0;
#else
  char clan_name[MAX_CLAN_NAME] = {'\0'}; /* Currently unused */
  int length = 0, padding = 0;
#endif
  char name_search[MAX_INPUT_LENGTH] = {'\0'}, buf[MAX_INPUT_LENGTH] = {'\0'}, classes_list[MAX_INPUT_LENGTH] = {'\0'};
  char mode;
  int low = 0, high = LVL_IMPL, localwho = 0, questwho = 0;
  int showclass = 0, short_list = 0, outlaws = 0;
  int who_room = 0, showgroup = 0, showleader = 0;
  int showrace = 0;
  int mortals = 0, staff = 0;
  clan_rnum c_n;
  size_t len = 0;
  /* char clan_name[50]; */ /* Currently unused */
  /* int length = 0; */ /* Currently unused */
  /* int padding = 0; */ /* Currently unused */

  char *account_names[CONFIG_MAX_PLAYING];
  int num_accounts = 0, x = 0, y = 0;

  for (i = 0; i < CONFIG_MAX_PLAYING; i++)
    account_names[i] = NULL;

  struct
  {
    const char *const disp;
    const int min_level;
    const int max_level;
    int count; /* must always start as 0 */
  } rank[] = {
  #if defined(CAMPAIGN_DL)
      {"\tb--\tB= \tCChronicles of Krynn Staff \tB=\tb--\tn\r\n\tc-=-=-=-=-=-=-=-=-=-=-=-\tn\r\n", LVL_IMMORT, LVL_IMPL, 0},
  #elif defined(CAMPAIGN_FR)
      {"\tb--\tB= \tCFaerun Staff \tB=\tb--\tn\r\n\tc-=-=-=-=-=-=-=-=-=-=-=-\tn\r\n", LVL_IMMORT, LVL_IMPL, 0},
  #else
      {"\tb--\tB= \tCLuminari Staff \tB=\tb--\tn\r\n\tc-=-=-=-=-=-=-=-=-=-=-=-\tn\r\n", LVL_IMMORT, LVL_IMPL, 0},
  #endif
      {"\tb--\tB=\tC Mortals \tB=\tb--\tn\r\n\tc-=-=-=-=-=-=-=-=-=-=-=-\tn\r\n", 1, LVL_IMMORT - 1, 0},
      {"\n", 0, 0, 0}};

  // remove spaces in front of argument
  skip_spaces_c(&argument);
  // copy argument -> buf
  strlcpy(buf, argument, sizeof(buf)); /* strcpy: OK (sizeof: argument == buf) */
  // first char of name_search is now NULL
  name_search[0] = '\0';
  *classes_list = '\0';

  // move along the buf array until '\0'
  while (*buf)
  {
    char arg[MAX_INPUT_LENGTH] = {'\0'}, buf1[MAX_INPUT_LENGTH] = {'\0'};

    // take buf, first delimit in arg, rest in buf1
    half_chop(buf, arg, buf1);
    // if arg is a digit...
    if (isdigit(*arg))
    {
      sscanf(arg, "%d-%d", &low, &high);
      strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */

      // arg isn't a digit, only acceptable input is '-' and a letter
    }
    else if (*arg == '-')
    {
      mode = *(arg + 1); /* just in case; we destroy arg in the switch */
      switch (mode)
      {
      case 'k':
        outlaws = 1;
        strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'z':
        localwho = 1;
        strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 's':
        short_list = 1;
        strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'q':
        questwho = 1;
        strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'n':
        half_chop(buf1, name_search, buf);
        break;
      case 'r':
        who_room = 1;
        strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'c':
        half_chop(buf1, arg, buf);
        showclass = find_class_bitvector(arg);
        break;
      case 'l':
        showleader = 1;
        strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'g':
        showgroup = 1;
        strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 't':
        half_chop(buf1, arg, buf);
        showrace = find_race_bitvector(arg);
        break;
      default:
        send_to_char(ch, "%s", WHO_FORMAT);
        return;
      }
    }
    else
    {
      send_to_char(ch, "%s", WHO_FORMAT);
      return;
    }
  }

  // first counting the "ranks" which will display how many chars are viewed with do_who call
  for (d = descriptor_list; d && !short_list; d = d->next)
  {
    if (d->original) // if !switched
      tch = d->original;
    else if (!(tch = d->character)) // if switched, make sure d->character
      continue;

    if (CAN_SEE(ch, tch) && IS_PLAYING(d))
    {
      if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
          !strstr(GET_TITLE(tch), name_search))
        continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
        continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) && !PLR_FLAGGED(tch, PLR_THIEF))
        continue;
      if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
        continue;
      if (localwho && world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone)
        continue;
      if (who_room && (IN_ROOM(tch) != IN_ROOM(ch)))
        continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
        continue;
      if (showrace && !(showrace & (1 << GET_RACE(tch))))
        continue;
      if (showgroup && !GROUP(tch))
        continue;
      if (showleader && (!GROUP(tch) || GROUP_LEADER(GROUP(tch)) != tch))
        continue;

      // our struct above, we are adjusting the count value based on players found
      for (i = 0; *rank[i].disp != '\n'; i++)
        if (GET_LEVEL(tch) >= rank[i].min_level && GET_LEVEL(tch) <= rank[i].max_level)
          rank[i].count++;

      if (d->account)
      {
        for (x = 0; x < CONFIG_MAX_PLAYING; x++)
        {
          if (account_names[x] == NULL)
          {
            if (x > 0)
            {
              for (y = 0; y < x; y++)
              {
                if (!strcmp(account_names[y], d->account->name))
                {
                  break;
                }
              }
              if (y == x)
              {
                account_names[x] = strdup(d->account->name);
              }
            }
            else
            {
              account_names[x] = strdup(d->account->name);
            }
          }
        }
        x = 0;
        while (account_names[x] != NULL)
        {
          x++;
        }
      }

      num_accounts = x;
    }
  }

  for (i = 0; *rank[i].disp != '\n'; i++)
  {
    // go through list of ranks, don't continue if this rank has no players
    if (!rank[i].count && !short_list)
      continue;

    // display top of who list
    if (short_list)
      send_to_char(ch, "Players\r\n-------\r\n");
    else
      send_to_char(ch, "%s", rank[i].disp);

    for (d = descriptor_list; d; d = d->next)
    {
      *classes_list = '\0';
      len = 0;
      if (d->original)
        tch = d->original;
      else if (!(tch = d->character))
        continue;

      if ((GET_LEVEL(tch) < rank[i].min_level || GET_LEVEL(tch) > rank[i].max_level) && !short_list)
        continue;
      if (!IS_PLAYING(d))
        continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
          !strstr(GET_TITLE(tch), name_search))
        continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
        continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) && !PLR_FLAGGED(tch, PLR_THIEF))
        continue;
      if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
        continue;
      if (localwho && world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone)
        continue;
      if (who_room && (IN_ROOM(tch) != IN_ROOM(ch)))
        continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
        continue;
      if (showrace && !(showrace & (1 << GET_RACE(tch))))
        continue;
      if (showgroup && !GROUP(tch))
        continue;
      if (showleader && (!GROUP(tch) || GROUP_LEADER(GROUP(tch)) != tch))
        continue;
#if defined(CAMPAIGN_DL)
      if (TRUE)
      {
        if (GET_LEVEL(tch) >= LVL_IMMORT)
          snprintf(clan_name, sizeof(clan_name), "%s", GET_IMM_TITLE(tch));
        else
          snprintf(clan_name, sizeof(clan_name), "%s", ((c_n = real_clan(GET_CLAN(tch))) != NO_CLAN && GET_CLANRANK(tch) > 0) ? CLAN_NAME(c_n) : "Adventurer");
      length = strlen(clan_name);
      length += count_color_chars(clan_name);
      padding = 28 - length;
      
      // Move characters to make room for padding at the front
      for (x = length; x >= 0; x--)
      {
          clan_name[x + padding / 2] = clan_name[x];
      }
      // Append spaces at the front
      for (x = 0; x < padding / 2; x++) {
          clan_name[x] = ' ';
      }
      // Append spaces at the end
      for (x = length + padding / 2; x < 28; x++) {
          clan_name[x] = ' ';
      }
      if (padding % 2 != 0) {
          // If padding is odd, add one more space at the end
          clan_name[28] = ' ';
      }
      if (GET_LEVEL(tch) >= LVL_IMMORT)
        send_to_char(ch, "\tW[\tC%28.28s \tW]\tn %s", clan_name, GET_TITLE(tch));
      else
        send_to_char(ch, "\tW[ \tC%2d %-4.4s %-20.20s \tW]\tn %s", GET_LEVEL(tch), race_list[GET_REAL_RACE(tch)].abbrev,
        ((c_n = real_clan(GET_CLAN(tch))) != NO_CLAN && GET_CLANRANK(tch) > 0) ? CLAN_NAME(c_n) : "Adventurer", GET_TITLE(tch));

        // num_can_see++;
        if (GET_LEVEL(tch) >= LVL_IMMORT)
        {
          staff++;
        }
        else
        {
          mortals++;
        }
#else
      if (short_list)
      {
        /* changed this to force showing char real race */
        send_to_char(ch, "[%2d %8s] %-12.12s%s%s",
                     GET_LEVEL(tch), RACE_ABBR_REAL(tch), GET_NAME(tch),
                     CCNRM(ch, C_SPR), ((!(++num_can_see % 4)) ? "\r\n" : ""));
      }
      else
      {
        // num_can_see++;
        if (GET_LEVEL(tch) >= LVL_IMMORT)
        {
          staff++;
          send_to_char(ch, "%13s", admin_level_names[(GET_LEVEL(tch) - LVL_IMMORT)]);
        }
        else
        {
          mortals++;
          /* changed this to force showing char real race */
          send_to_char(ch, "[%2d %4s ",
                       GET_LEVEL(tch), RACE_ABBR_REAL(tch));
        }

        if (GET_LEVEL(tch) < LVL_IMMORT)
        {
          int inc, classCount = 0;
          for (inc = 0; inc < MAX_CLASSES; inc++)
          {
            if (CLASS_LEVEL(tch, inc))
            {
              if (classCount)
                len += snprintf(classes_list + len, sizeof(classes_list) - len, "/");
              len += snprintf(classes_list + len, sizeof(classes_list) - len, "%s",
                              CLSLIST_CLRABBRV(inc));
              classCount++;
            }
          }
          class_len = strlen(classes_list) - count_color_chars(classes_list);
          while (class_len < 11)
          {
            len += snprintf(classes_list + len, sizeof(classes_list) - len, " ");
            class_len++;
          }
          send_to_char(ch, "%s]", classes_list);
        }

        send_to_char(ch, " %s%s%s%s",
                     GET_NAME(tch), (*GET_TITLE(tch) ? " " : ""), GET_TITLE(tch),
                     CCNRM(ch, C_SPR));

        if (IS_IN_CLAN(tch) && !(GET_CLANRANK(tch) == NO_CLANRANK))
        {
          if (GET_CLAN(tch))
          {
            c_n = real_clan(GET_CLAN(tch));
            if (c_n != NO_CLAN)
              send_to_char(ch, " %s[%s%s%s]%s", QBRED, QBYEL, clan_list[c_n].abrev ? CLAN_ABREV(c_n) : "Unknown", QBRED, QNRM);
          }
        }
#endif
        if (GET_INVIS_LEV(tch))
          send_to_char(ch, " (i%d)", GET_INVIS_LEV(tch));
        else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
          send_to_char(ch, " (invis)");

        if (PLR_FLAGGED(tch, PLR_MAILING))
          send_to_char(ch, " (mailing)");
        else if (d->olc && d->character && GET_LEVEL(d->character) >= LVL_IMMORT)
          send_to_char(ch, " (OLC)");
        else if (PLR_FLAGGED(tch, PLR_WRITING))
          send_to_char(ch, " (writing)");

        if (d->original)
          send_to_char(ch, " (out of body)");

        if (d->connected == CON_OEDIT)
          send_to_char(ch, " (Object Edit)");
        if (d->connected == CON_IEDIT)
          send_to_char(ch, " (Object Edit)");
        if (d->connected == CON_MEDIT)
          send_to_char(ch, " (Mobile Edit)");
        if (d->connected == CON_ZEDIT)
          send_to_char(ch, " (Zone Edit)");
        if (d->connected == CON_SEDIT)
          send_to_char(ch, " (Shop Edit)");
        if (d->connected == CON_REDIT)
          send_to_char(ch, " (Room Edit)");
        if (d->connected == CON_TEDIT)
          send_to_char(ch, " (Text Edit)");
        if (d->connected == CON_TRIGEDIT)
          send_to_char(ch, " (Trigger Edit)");
        if (d->connected == CON_AEDIT)
          send_to_char(ch, " (Social Edit)");
        if (d->connected == CON_CEDIT)
          send_to_char(ch, " (Configuration Edit)");
        if (d->connected == CON_HEDIT)
          send_to_char(ch, " (Help edit)");
        if (d->connected == CON_QEDIT)
          send_to_char(ch, " (Quest Edit)");
        if (d->connected == CON_HLQEDIT)
          send_to_char(ch, " (HLQuest Edit)");
        if (d->connected == CON_STUDY)
          send_to_char(ch, " (Studying)");
        if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_BUILDWALK))
          send_to_char(ch, " (Buildwalking)");
        if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_AFK))
          send_to_char(ch, " (AFK)");
        if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_RP))
          send_to_char(ch, " (RP)");
        if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_NOGOSS))
          send_to_char(ch, " (nogos)");
        if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_NOWIZ))
          send_to_char(ch, " (nowiz)");
        if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_NOSHOUT))
          send_to_char(ch, " (noshout)");
        if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_NOTELL))
          send_to_char(ch, " (notell)");
        if (!IS_NPC(ch) && PRF_FLAGGED(tch, PRF_QUEST))
          send_to_char(ch, " (quest)");
        if (PLR_FLAGGED(tch, PLR_THIEF))
          send_to_char(ch, " (THIEF)");
        if (PLR_FLAGGED(tch, PLR_KILLER))
          send_to_char(ch, " (KILLER)");
        if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_NON_ROLEPLAYER))
          send_to_char(ch, " (NoRP)");
        send_to_char(ch, "\r\n");
      }
    }
    send_to_char(ch, "\r\n");
    if (short_list)
      break;
  }
  if (short_list && num_can_see % 4)
    send_to_char(ch, "\r\n");
  // if (!num_can_see)
  //   send_to_char(ch, "Nobody at all!\r\n");
  // else if (num_can_see == 1)
  //   send_to_char(ch, "One lonely character displayed.\r\n");
  // else
  //   send_to_char(ch, "%d characters displayed.\r\n", num_can_see);
  send_to_char(ch, "Total visible players: %d.\r\n", mortals + staff);
  if ((mortals + staff) > boot_high)
    boot_high = mortals + staff;
  send_to_char(ch, "Maximum visible players this boot: %d.\r\n", boot_high);

  if (IS_IMMORTAL(ch))
  {
    send_to_char(ch, "Number of unique accounts connected: %d.\r\n", num_accounts);
  }

  send_to_char(ch, "Type 'listraces' to understand race abbreviations.\r\n");

  if (IS_HAPPYHOUR > 0)
  {
    send_to_char(ch, "\tWIt's a Happy Hour! Type \tRhappyhour\tW to see the current bonuses.\tn\r\n");
  }
  if (IS_STAFF_EVENT)
  {
    send_to_char(ch, "\tWA staff-ran event is taking place! Type \tRstaffevent\tW to see the current event info.\tn\r\n");
  }
}

#define USERS_FORMAT \
  "format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"

ACMD(do_users)
{
  char line[200], line2[220], idletime[10], classname[20];
  char state[30], *timeptr, mode;
  char name_search[MAX_INPUT_LENGTH] = {'\0'}, host_search[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *tch;
  struct descriptor_data *d;
  int low = 0, high = LVL_IMPL, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;
  char buf[MAX_INPUT_LENGTH] = {'\0'}, arg[MAX_INPUT_LENGTH] = {'\0'};

  host_search[0] = name_search[0] = '\0';

  strlcpy(buf, argument, sizeof(buf)); /* strcpy: OK (sizeof: argument == buf) */
  while (*buf)
  {
    char buf1[MAX_INPUT_LENGTH] = {'\0'};

    half_chop(buf, arg, buf1);
    if (*arg == '-')
    {
      mode = *(arg + 1); /* just in case; we destroy arg in the switch */
      switch (mode)
      {
      case 'o':
      case 'k':
        outlaws = 1;
        playing = 1;
        strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'p':
        playing = 1;
        strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'd':
        deadweight = 1;
        strlcpy(buf, buf1, sizeof(buf)); /* strcpy: OK (sizeof: buf1 == buf) */
        break;
      case 'l':
        playing = 1;
        half_chop(buf1, arg, buf);
        sscanf(arg, "%d-%d", &low, &high);
        break;
      case 'n':
        playing = 1;
        half_chop(buf1, name_search, buf);
        break;
      case 'h':
        playing = 1;
        half_chop(buf1, host_search, buf);
        break;
      case 'c':
        playing = 1;
        half_chop(buf1, arg, buf);
        showclass = find_class_bitvector(arg);
        break;
      default:
        send_to_char(ch, "%s", USERS_FORMAT);
        return;
      } /* end of switch */
    }
    else
    { /* endif */
      send_to_char(ch, "%s", USERS_FORMAT);
      return;
    }
  } /* end while (parser) */
  send_to_char(ch,
               "Num Class    Name         State          Idl   Login\t*   Site\r\n"
               "--- -------- ------------ -------------- ----- -------- ------------------------\r\n");

  one_argument(argument, arg, sizeof(arg));

  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) != CON_PLAYING && playing)
      continue;
    if (STATE(d) == CON_PLAYING && deadweight)
      continue;
    if (IS_PLAYING(d))
    {
      if (d->original)
        tch = d->original;
      else if (!(tch = d->character))
        continue;

      if (*host_search && !strstr(d->host, host_search))
        continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
        continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
        continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
          !PLR_FLAGGED(tch, PLR_THIEF))
        continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
        continue;
      if (GET_INVIS_LEV(tch) > GET_LEVEL(ch))
        continue;

      if (d->original)
        snprintf(classname, sizeof(classname), "[%2d %s]", GET_LEVEL(d->original),
                 CLASS_ABBR(d->original));
      else
        snprintf(classname, sizeof(classname), "[%2d %s]", GET_LEVEL(d->character),
                 CLASS_ABBR(d->character));
    }
    else
      strlcpy(classname, "   -    ", sizeof(classname));

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (STATE(d) == CON_PLAYING && d->original)
      strlcpy(state, "Switched", sizeof(state));
    else
      strlcpy(state, connected_types[STATE(d)], sizeof(state));

    if (d->character && STATE(d) == CON_PLAYING)
      snprintf(idletime, sizeof(idletime), "%5d", d->character->char_specials.timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strlcpy(idletime, "     ", sizeof(idletime));

    snprintf(line, sizeof(line), "%3d %-7s %-12s %-14s %-3s %-8s ", d->desc_num, classname,
             d->original && d->original->player.name ? d->original->player.name : d->character && d->character->player.name ? d->character->player.name
                                                                                                                            : "UNDEFINED",
             state, idletime, timeptr);

    if (*d->host)
      sprintf(line + strlen(line), "[%s]\r\n", d->host);
    else
      strlcat(line, "[Hostname unknown]\r\n", sizeof(line));

    if (STATE(d) != CON_PLAYING)
    {
      snprintf(line2, sizeof(line2), "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
      strlcpy(line, line2, sizeof(line));
    }
    if (STATE(d) != CON_PLAYING ||
        (STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character)))
    {
      send_to_char(ch, "%s", line);
      num_can_see++;
    }
  }

  send_to_char(ch, "\r\n%d visible sockets connected.\r\n", num_can_see);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  if (IS_NPC(ch))
  {
    send_to_char(ch, "Not for mobiles!\r\n");
    return;
  }

  switch (subcmd)
  {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    GET_LAST_NEWS(ch) = time(0);
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    GET_LAST_MOTD(ch) = time(0);
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char(ch, "\033[H\033[J");
    break;
  case SCMD_VERSION:
    send_to_char(ch, "%s\r\n", luminari_version);
    if (IS_IMMORTAL(ch))
    {
      send_to_char(ch, "%s", luminari_build);
    }
    break;
  case SCMD_WHOAMI:
    send_to_char(ch, "%s\r\n", GET_NAME(ch));
    break;
  default:
    log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
    /* SYSERR_DESC: General page string function for such things as 'credits',
     * 'news', 'wizlist', 'clear', 'version'.  This occurs when a call is made
     * to this routine that is not one of the predefined calls.  To correct it,
     * either a case needs to be added into the function to account for the
     * subcmd that is being passed to it, or the call to the function needs to
     * have the correct subcmd put into place. */
    return;
  }
}

ACMD(do_where)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  one_argument(argument, arg, sizeof(arg));

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}

ACMD(do_levels)
{
  char buf[MAX_STRING_LENGTH] = {'\0'}, arg[MAX_STRING_LENGTH] = {'\0'};
  size_t len = 0, nlen;
  int i, ret, min_lev = 1, max_lev = LVL_IMMORT, val;

  if (IS_NPC(ch))
  {
    send_to_char(ch, "You ain't nothin' but a hound-dog.\r\n");
    return;
  }
  one_argument(argument, arg, sizeof(arg));

  if (*arg)
  {
    if (isdigit(*arg))
    {
      ret = sscanf(arg, "%d-%d", &min_lev, &max_lev);
      if (ret == 0)
      {
        /* No valid args found */
        min_lev = 1;
        max_lev = LVL_IMMORT;
      }
      else if (ret == 1)
      {
        /* One arg = range is (num) either side of current level */
        val = min_lev;
        max_lev = MIN(GET_LEVEL(ch) + val, LVL_IMMORT);
        min_lev = MAX(GET_LEVEL(ch) - val, 1);
      }
      else if (ret == 2)
      {
        /* Two args = min-max range limit - just do sanity checks */
        min_lev = MAX(min_lev, 1);
        max_lev = MIN(max_lev + 1, LVL_IMMORT);
      }
    }
    else
    {
      send_to_char(ch, "Usage: %slevels [<min>-<max> | <range>]%s\r\n\r\n", QYEL, QNRM);
      send_to_char(ch, "Displays exp required for levels.\r\n");
      send_to_char(ch, "%slevels       %s- shows all levels (1-%d)\r\n", QCYN, QNRM, (LVL_IMMORT - 1));
      send_to_char(ch, "%slevels 5     %s- shows 5 levels either side of your current level\r\n", QCYN, QNRM);
      send_to_char(ch, "%slevels 10-40 %s- shows level 10 to level 40\r\n", QCYN, QNRM);
      return;
    }
  }

  for (i = min_lev; i < max_lev; i++)
  {
    nlen = snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d-%-8d : ", (int)i,
                    level_exp(ch, i), level_exp(ch, i + 1) - 1);
    if (len + nlen >= sizeof(buf))
      break;
    len += nlen;

    // why are we checking sex for titles?  titles used to be sex
    // dependent...  -zusuk
    switch (GET_SEX(ch))
    {
    case SEX_MALE:
    case SEX_NEUTRAL:
      nlen = snprintf(buf + len, sizeof(buf) - len, "%s\r\n", titles(GET_CLASS(ch), i));
      break;
    case SEX_FEMALE:
      nlen = snprintf(buf + len, sizeof(buf) - len, "%s\r\n", titles(GET_CLASS(ch), i));
      break;
    default:
      nlen = snprintf(buf + len, sizeof(buf) - len, "Oh dear.  You seem to be sexless.\r\n");
      break;
    }
    if (len + nlen >= sizeof(buf))
      break;
    len += nlen;
  }

  if (len < sizeof(buf) && max_lev == LVL_IMMORT)
    snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d          : Immortality\r\n",
             LVL_IMMORT, level_exp(ch, LVL_IMMORT));
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_consider)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *victim;
  int diff;

  one_argument(argument, buf, sizeof(buf));

  if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "Consider killing who?\r\n");
    return;
  }
  if (victim == ch)
  {
    send_to_char(ch, "Easy!  Very easy indeed!\r\n");
    return;
  }
  if (!IS_NPC(victim))
  {
    send_to_char(ch, "Would you like to borrow a cross and a shovel?\r\n");
    return;
  }
  if (GET_LEVEL(victim) >= LVL_IMMORT)
  {
    /* mobiles level 31+ are 'group-needed' generally */
    send_to_char(ch, "Don't even think about it without some help!\r\n");
    return;
  }
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char(ch, "Now where did that chicken go?\r\n");
  else if (diff <= -5)
    send_to_char(ch, "You could do it with a needle!\r\n");
  else if (diff <= -2)
    send_to_char(ch, "Easy.\r\n");
  else if (diff <= -1)
    send_to_char(ch, "Fairly easy.\r\n");
  else if (diff == 0)
    send_to_char(ch, "The perfect match!\r\n");
  else if (diff <= 1)
    send_to_char(ch, "You would need some luck!\r\n");
  else if (diff <= 2)
    send_to_char(ch, "You would need a lot of luck!\r\n");
  else if (diff <= 3)
    send_to_char(ch, "You would need a lot of luck and great equipment!\r\n");
  else if (diff <= 5)
    send_to_char(ch, "Do you feel lucky, punk?\r\n");
  else if (diff <= 10)
    send_to_char(ch, "Are you mad!?\r\n");
  else if (diff <= 100)
    send_to_char(ch, "You ARE mad!\r\n");
}

ACMD(do_diagnose)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;

  one_argument(argument, buf, sizeof(buf));

  if (*buf)
  {
    if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else
      diag_char_to_char(vict, ch);
  }
  else
  {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char(ch, "Diagnose who?\r\n");
  }
}

ACMD(do_toggle)
{
  char buf2[4], arg[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};
  int toggle, tp, wimp_lev, result = 0, len = 0, i;
  const char *const types[] = {"OFF", "Brief", "Normal", "ON", "\n"};

  const struct
  {
    const char *command;
    bitvector_t toggle; /* this needs changing once hashmaps are implemented */
    char min_level;
    const char *disable_msg;
    const char *enable_msg;
  } tog_messages[] = {
      /*0*/
      {"summonable", PRF_SUMMONABLE, 0,
       "You are now safe from summoning by other players.\r\n",
       "You may now be summoned by other players.\r\n"},
      /*1*/
      {"nohassle", PRF_NOHASSLE, LVL_IMMORT,
       "Nohassle disabled.\r\n",
       "Nohassle enabled.\r\n"},
      /*2*/
      {"brief", PRF_BRIEF, 0,
       "Brief mode off.\r\n",
       "Brief mode on.\r\n"},
      /*3*/
      {"compact", PRF_COMPACT, 0,
       "Compact mode off.\r\n",
       "Compact mode on.\r\n"},
      /*4*/
      {"notell", PRF_NOTELL, 0,
       "You can now hear tells.\r\n",
       "You are now deaf to tells.\r\n"},
      /*5*/
      {"noauction", PRF_NOAUCT, 0,
       "You can now hear auctions.\r\n",
       "You are now deaf to auctions.\r\n"},
      /*6*/
      {"noshout", PRF_NOSHOUT, 0,
       "You can now hear shouts.\r\n",
       "You are now deaf to shouts.\r\n"},
      /*7*/
      {"nogossip", PRF_NOGOSS, 0,
       "You can now hear gossip.\r\n",
       "You are now deaf to gossip.\r\n"},
      /*8*/
      {"nograts", PRF_NOGRATZ, 0,
       "You can now hear gratz.\r\n",
       "You are now deaf to gratz.\r\n"},
      /*9*/
      {"nowiz", PRF_NOWIZ, LVL_IMMORT,
       "You can now hear the Wiz-channel.\r\n",
       "You are now deaf to the Wiz-channel.\r\n"},
      /*10*/
      {"quest", PRF_QUEST, 0,
       "You are no longer part of the Quest.\r\n",
       "Okay, you are part of the Quest.\r\n"},
      /*11*/
      {"showvnums", PRF_SHOWVNUMS, LVL_IMMORT,
       "You will no longer see the vnums.\r\n",
       "You will now see the vnums.\r\n"},
      /*12*/
      {"norepeat", PRF_NOREPEAT, 0,
       "You will now have your communication repeated.\r\n",
       "You will no longer have your communication repeated.\r\n"},
      /*13*/
      {"holylight", PRF_HOLYLIGHT, LVL_IMMORT,
       "HolyLight mode off.\r\n",
       "HolyLight mode on.\r\n"},
      /*14*/
      {"slownameserver", 0, LVL_IMPL,
       "Nameserver_is_slow changed to OFF; IP addresses will now be resolved.\r\n",
       "Nameserver_is_slow changed to ON; sitenames will no longer be resolved.\r\n"},
      /*15*/
      {"autoexits", PRF_AUTOEXIT, 0,
       "Autoexits disabled.\r\n",
       "Autoexits enabled.\r\n"},
      /*16*/
      {"trackthru", 0, LVL_IMPL,
       "Players can no longer track through doors.\r\n",
       "Players can now track through doors.\r\n"},
      /*17*/
      {"clsolc", PRF_CLS, LVL_BUILDER,
       "You will no longer clear screen in OLC.\r\n",
       "You will now clear screen in OLC.\r\n"},
      /*18*/
      {"buildwalk", PRF_BUILDWALK, LVL_BUILDER,
       "Buildwalk is now Off.\r\n",
       "Buildwalk is now On.\r\n"},
      /*19*/
      {"afk", PRF_AFK, 0,
       "AFK is now Off.\r\n",
       "AFK is now On.\r\n"},
      /*20*/
      {"autoloot", PRF_AUTOLOOT, 0,
       "Autoloot disabled.\r\n",
       "Autoloot enabled.\r\n"},
      /*21*/
      {"autogold", PRF_AUTOGOLD, 0,
       "Autogold disabled.\r\n",
       "Autogold enabled.\r\n"},
      /*22*/
      {"autosplit", PRF_AUTOSPLIT, 0,
       "Autosplit disabled.\r\n",
       "Autosplit enabled.\r\n"},
      /*23*/
      {"autosac", PRF_AUTOSAC, 0,
       "Autosac disabled.\r\n",
       "Autosac enabled.\r\n"},
      /*24*/
      {"autoassist", PRF_AUTOASSIST, 0,
       "Autoassist disabled.\r\n",
       "Autoassist enabled.\r\n"},
      /*25*/
      {"automap", PRF_AUTOMAP, 1,
       "You will no longer see the mini-map.\r\n",
       "You will now see a mini-map at the side of room descriptions.\r\n"},
      /*26*/
      {"autokey", PRF_AUTOKEY, 0,
       "You will now have to unlock doors manually before opening.\r\n",
       "You will now automatically unlock doors when opening them (if you have the key).\r\n"},
      /*27*/
      {"autodoor", PRF_AUTODOOR, 0,
       "You will now need to specify a door direction when opening, closing and unlocking.\r\n",
       "You will now find the next available door when opening, closing or unlocking.\r\n"},
      /*28*/
      {"clantalk", PRF_NOCLANTALK, LVL_STAFF,
       "You can now hear all clan's clantalk.\r\n",
       "All clantalk will now be hidden.\r\n"},
      /*29*/
      {"color", 0, 0, "\n", "\n"},
      /*30*/
      {"syslog", 0, LVL_IMMORT, "\n", "\n"},
      /*31*/
      {"wimpy", 0, 0, "\n", "\n"},
      /*32*/
      {"pagelength", 0, 0, "\n", "\n"},
      /*33*/
      {"screenwidth", 0, 0, "\n", "\n"},
      /*34*/
      {"autoscan", PRF_AUTOSCAN, 0,
       "Autoscan disabled.\r\n",
       "Autoscan enabled.\r\n"},
      /*35*/
      {"autoreload", PRF_AUTORELOAD, 0,
       "Autoreload disabled.\r\n",
       "Autoreload enabled.\r\n"},
      /*36*/
      {"combatroll", PRF_COMBATROLL, 0,
       "CombatRoll disabled.\r\n",
       "CombatRoll enabled, you will see behind the scene rolls behind combat.\r\n"},
      /*37*/
      {"guimode", PRF_GUI_MODE, 0,
       "GUI Mode disabled.\r\n",
       "GUI Mode enabled, make sure you have MSDP enabled in your client.\r\n"},
      /*38*/
      {"nohint", PRF_NOHINT, 0,
       "You will now see approximately every 5 minutes a in-game hint.\r\n",
       "You will no longer see in-game hints.\r\n"},
      /*39*/
      {"autocollect", PRF_AUTOCOLLECT, 0,
       "You will no longer automatically collect your ammo after combat.\r\n",
       "You will now automatically collect your ammo after combat.\r\n"},
      /*40*/
      {"rp", PRF_RP, 0,
       "You will no longer display to others that you would like to Role-play.\r\n",
       "You will now display to others that you would like to Role-play.\r\n"},
      /* 41 */
      {"aoebomb", PRF_AOE_BOMBS, 0,
       "Your bombs will now only affect single targets.\r\n",
       "Your bombs will now affect multiple targets.\r\n"},
      /*42*/
      {"autocon", PRF_AUTOCON, 0,
       "You will no longer see level differences between you and mobs when you type look.\r\n",
       "You will now see level differences between you and mobs when you type look.\r\n"},
      /* 43 */
      {"smashdefense", PRF_SMASH_DEFENSE, 0,
       "You will no longer use smash defense in combat.\r\n",
       "You will now use smash defense in combat (if you know it).\r\n"},
      /* 44 */
      {"charmierescue", PRF_NO_CHARMIE_RESCUE, 0,
       "You will now allow charmies to rescue you and other group members.\r\n",
       "You will no longer allow charmies to rescue you and other group members\r\n"},
      /* 45 */
      {"storedconsumables", PRF_USE_STORED_CONSUMABLES, 0,
       "You will now use the stored consumables system (HELP CONSUMABLES).\r\n",
       "You will no use the stock consumables system (HELP USE).\r\n"},
      /* 46 */
      {"autostand", PRF_AUTO_STAND, 0,
       "You will no longer automatically stand if knocked down in combat.\r\n",
       "You will now automatically stand if knocked down in combat.\r\n"},
      /* 47 */
      {"autohit", PRF_AUTOHIT, 0,
       "You will no longer automatically hit mobs when typing 'hit' by itself.\r\n",
       "You will now automatically hit the first eligible mob in the room by typing 'hit' by itself.\r\n"},
      /*48*/
      {"nofollow", PRF_NO_FOLLOW, 0,
       "Players can now follow you.\r\n",
       "Players can no longer follow you!\r\n"},
      /*49*/
      {"condensed", PRF_CONDENSED, 0,
       "You will now see full combat details.\r\n",
       "You will now see condensed combat messages.\r\n"},
      /*50*/
      {"carefulpet", PRF_CAREFUL_PET, 0,
       "You will no longer be careful with your pets (and vice versa).\r\n",
       "You will now be careful with your pets (and vice versa).\r\n"},

      /*LAST*/
      {"\n", 0, -1, "\n", "\n"} /* must be last */
  };

  if (IS_NPC(ch))
    return;

  argument = one_argument(argument, arg, sizeof(arg));
  any_one_arg_c(argument, arg2, sizeof(arg2)); /* so that we don't skip 'on' */

  if (!*arg)
  {
    if (!GET_WIMP_LEV(ch))
      strlcpy(buf2, "OFF", sizeof(buf2)); /* strcpy: OK */
    else
      snprintf(buf2, sizeof(buf2), "%-3.3d", GET_WIMP_LEV(ch)); /* sprintf: OK */

    if (GET_LEVEL(ch) == LVL_IMPL)
    {
      send_to_char(ch, "Forger Toggles:\r\n");
      send_to_char(ch,
                   " SlowNameserver:  %-3s   "
                   " Trackthru Doors: %-3s\r\n",

                   ONOFF(CONFIG_NS_IS_SLOW),
                   ONOFF(CONFIG_TRACK_T_DOORS));
    }

    if (GET_LEVEL(ch) >= LVL_IMMORT)
    {
      send_to_char(ch, "Staff Toggles:\r\n");
      send_to_char(ch,
                   "      Buildwalk: %-3s    "
                   "          NoWiz: %-3s    "
                   "         ClsOLC: %-3s\r\n"
                   "       NoHassle: %-3s    "
                   "      Holylight: %-3s    "
                   "      ShowVnums: %-3s\r\n"
                   "     NoClanTalk: %-3s    "
                   "         Syslog: %-3s\r\n",

                   ONOFF(PRF_FLAGGED(ch, PRF_BUILDWALK)),
                   ONOFF(PRF_FLAGGED(ch, PRF_NOWIZ)),
                   ONOFF(PRF_FLAGGED(ch, PRF_CLS)),
                   ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
                   ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
                   ONOFF(PRF_FLAGGED(ch, PRF_SHOWVNUMS)),
                   ONOFF(PRF_FLAGGED(ch, PRF_NOCLANTALK)),
                   types[(PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0)]);
    }

    send_to_char(ch,
                 "Hit Pnt Display: %-3s    "
                 "          Brief: %-3s    "
                 "     Summonable: %-3s\r\n"

                 "   Move Display: %-3s    "
                 "        Compact: %-3s    "
                 "          Quest: %-3s\r\n"

                 "   PSP Display: %-3s    "
                 "         NoTell: %-3s    "
                 "       NoRepeat: %-3s\r\n"

                 "      AutoExits: %-3s    "
                 "        NoShout: %-3s    "
                 "          Wimpy: %-3s\r\n"

                 "       NoGossip: %-3s    "
                 "      NoAuction: %-3s    "
                 "        NoGrats: %-3s\r\n"

                 "       AutoLoot: %-3s    "
                 "       AutoGold: %-3s    "
                 "      AutoSplit: %-3s\r\n"

                 "        AutoSac: %-3s    "
                 "     AutoAssist: %-3s    "
                 "        AutoMap: %-3s\r\n"

                 "     Pagelength: %-3d    "
                 "    Screenwidth: %-3d    "
                 "            AFK: %-3s\r\n"

                 "        Autokey: %-3s    "
                 "       Autodoor: %-3s    "
                 "          Color: %s\r\n"

                 "       Autoscan: %-3s    "
                 "    EXP Display: %-3s    "
                 "  Exits Display: %-3s\r\n"

                 "   Room Display: %-3s    "
                 "Memtime Display: %-3s    "
                 "Actions Display: %-3s\r\n"

                 "     AutoReload: %-3s    "
                 "     CombatRoll: %-3s    "
                 "       GUI Mode: %-3s\r\n"

                 "        NoHints: %-3s    "
                 "   Auto Collect: %-3s    "
                 "   Role-Playing: %-3s\r\n"

                 "*NOTE: The PREFEDIT command is preferred method of optimizing your toggle switches.\r\n",

                 ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
                 ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
                 ONOFF(PRF_FLAGGED(ch, PRF_SUMMONABLE)),

                 ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
                 ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
                 ONOFF(PRF_FLAGGED(ch, PRF_QUEST)),

                 ONOFF(PRF_FLAGGED(ch, PRF_DISPPSP)),
                 ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
                 ONOFF(PRF_FLAGGED(ch, PRF_NOREPEAT)),

                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
                 ONOFF(PRF_FLAGGED(ch, PRF_NOSHOUT)),
                 buf2,

                 ONOFF(PRF_FLAGGED(ch, PRF_NOGOSS)),
                 ONOFF(PRF_FLAGGED(ch, PRF_NOAUCT)),
                 ONOFF(PRF_FLAGGED(ch, PRF_NOGRATZ)),

                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),

                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOSAC)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOMAP)),

                 GET_PAGE_LENGTH(ch),
                 GET_SCREEN_WIDTH(ch),
                 ONOFF(PRF_FLAGGED(ch, PRF_AFK)),

                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOKEY)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTODOOR)),
                 types[COLOR_LEV(ch)],

                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOSCAN)),
                 ONOFF(PRF_FLAGGED(ch, PRF_DISPEXP)),
                 ONOFF(PRF_FLAGGED(ch, PRF_DISPEXITS)),

                 ONOFF(PRF_FLAGGED(ch, PRF_DISPROOM)),
                 ONOFF(PRF_FLAGGED(ch, PRF_DISPMEMTIME)),
                 ONOFF(PRF_FLAGGED(ch, PRF_DISPACTIONS)),

                 ONOFF(PRF_FLAGGED(ch, PRF_AUTORELOAD)),
                 ONOFF(PRF_FLAGGED(ch, PRF_COMBATROLL)),
                 ONOFF(PRF_FLAGGED(ch, PRF_GUI_MODE)),
                 ONOFF(PRF_FLAGGED(ch, PRF_NOHINT)),
                 ONOFF(PRF_FLAGGED(ch, PRF_AUTOCOLLECT)),
                 ONOFF(PRF_FLAGGED(ch, PRF_RP))
                 /*end*/);
    return;
  }

  len = strlen(arg);
  for (toggle = 0; *tog_messages[toggle].command != '\n'; toggle++)
    if (!strncmp(arg, tog_messages[toggle].command, len))
      break;

  if (*tog_messages[toggle].command == '\n' || tog_messages[toggle].min_level > GET_LEVEL(ch))
  {
    send_to_char(ch, "You can't toggle that!\r\n");
    return;
  }

  switch (toggle)
  {
  case SCMD_COLOR:
    if (!*arg2)
    {
      send_to_char(ch, "Your current color level is %s.\r\n", types[COLOR_LEV(ch)]);
      return;
    }

    if (((tp = search_block(arg2, types, FALSE)) == -1))
    {
      send_to_char(ch, "Usage: toggle color { Off | Brief | Normal | On }\r\n");
      return;
    }
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);
    if (tp & 1)
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
    if (tp & 2)
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);

    send_to_char(ch, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR), CCNRM(ch, C_OFF), types[tp]);
    return;
  case SCMD_SYSLOG:
    if (!*arg2)
    {
      send_to_char(ch, "Your syslog is currently %s.\r\n",
                   types[(PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0)]);
      return;
    }
    if (((tp = search_block(arg2, types, FALSE)) == -1))
    {
      send_to_char(ch, "Usage: toggle syslog { Off | Brief | Normal | On }\r\n");
      return;
    }
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
    if (tp & 1)
      SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
    if (tp & 2)
      SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);

    send_to_char(ch, "Your syslog is now %s.\r\n", types[tp]);
    return;
  case SCMD_SLOWNS:
    result = (CONFIG_NS_IS_SLOW = !CONFIG_NS_IS_SLOW);
    break;
  case SCMD_TRACK:
    result = (CONFIG_TRACK_T_DOORS = !CONFIG_TRACK_T_DOORS);
    break;
  case SCMD_BUILDWALK:
    if (GET_LEVEL(ch) < LVL_BUILDER)
    {
      send_to_char(ch, "Builders only, sorry.\r\n");
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_BUILDWALK))
    {
      for (i = 0; *arg2 && *(sector_types[i]) != '\n'; i++)
        if (is_abbrev(arg2, sector_types[i]))
          break;
      if (*(sector_types[i]) == '\n')
        i = 0;
      GET_BUILDWALK_SECTOR(ch) = i;
      send_to_char(ch, "Default sector type is %s\r\n", sector_types[i]);
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk on.  Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    }
    else
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk off.  Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    break;
  case SCMD_AFK:
    if ((result = PRF_TOG_CHK(ch, PRF_AFK)))
      act("$n is now away from $s keyboard.", TRUE, ch, 0, 0, TO_ROOM);
    else
    {
      act("$n has returned to $s keyboard.", TRUE, ch, 0, 0, TO_ROOM);
      if (has_mail(GET_IDNUM(ch)))
        send_to_char(ch, "You have mail waiting.\r\n");
    }
    break;
  case SCMD_RP:
    if ((result = PRF_TOG_CHK(ch, PRF_RP)))
      act("$n is interested in Role-playing.", TRUE, ch, 0, 0, TO_ROOM);
    else
    {
      act("$n is now OOC.", TRUE, ch, 0, 0, TO_ROOM);
    }
    break;
  case SCMD_WIMPY:
    if (!*arg2)
    {
      if (GET_WIMP_LEV(ch))
      {
        send_to_char(ch, "Your current wimp level is %d hit points.\r\n", GET_WIMP_LEV(ch));
        return;
      }
      else
      {
        send_to_char(ch, "At the moment, you're not a wimp.  (sure, sure...)\r\n");
        return;
      }
    }
    if (isdigit(*arg2))
    {
      if ((wimp_lev = atoi(arg2)) != 0)
      {
        if (wimp_lev < 0)
          send_to_char(ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
        else if (wimp_lev > GET_MAX_HIT(ch))
          send_to_char(ch, "That doesn't make much sense, now does it?\r\n");
        else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
          send_to_char(ch, "You can't set your wimp level above half your hit points.\r\n");
        else
        {
          send_to_char(ch, "Okay, you'll wimp out if you drop below %d hit points.", wimp_lev);
          GET_WIMP_LEV(ch) = wimp_lev;
        }
      }
      else
      {
        send_to_char(ch, "Okay, you'll now tough out fights to the bitter end.");
        GET_WIMP_LEV(ch) = 0;
      }
    }
    else
      send_to_char(ch, "Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n");
    break;
  case SCMD_PAGELENGTH:
    if (!*arg2)
      send_to_char(ch, "You current page length is set to %d lines.", GET_PAGE_LENGTH(ch));
    else if (is_number(arg2))
    {
      GET_PAGE_LENGTH(ch) = MIN(MAX(atoi(arg2), 5), 255);
      send_to_char(ch, "Okay, your page length is now set to %d lines.", GET_PAGE_LENGTH(ch));
    }
    else
      send_to_char(ch, "Please specify a number of lines (5 - 255).");
    break;
  case SCMD_SCREENWIDTH:
    if (!*arg2)
      send_to_char(ch, "Your current screen width is set to %d characters.", GET_SCREEN_WIDTH(ch));
    else if (is_number(arg2))
    {
      GET_SCREEN_WIDTH(ch) = MIN(MAX(atoi(arg2), 40), 200);
      send_to_char(ch, "Okay, your screen width is now set to %d characters.", GET_SCREEN_WIDTH(ch));
    }
    else
      send_to_char(ch, "Please specify a number of characters (40 - 200).");
    break;
  case SCMD_AUTOMAP:
    if (can_see_map(ch))
    {
      if (!*arg2)
      {
        TOGGLE_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
        result = (PRF_FLAGGED(ch, tog_messages[toggle].toggle));
      }
      else if (!strcmp(arg2, "on"))
      {
        SET_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
        result = 1;
      }
      else if (!strcmp(arg2, "off"))
      {
        REMOVE_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
      }
      else
      {
        send_to_char(ch, "Value for %s must either be 'on' or 'off'.\r\n", tog_messages[toggle].command);
        return;
      }
    }
    else
      send_to_char(ch, "Sorry, automap is currently disabled.\r\n");
    break;
  default:
    if (!*arg2)
    {
      TOGGLE_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
      result = (PRF_FLAGGED(ch, tog_messages[toggle].toggle));
    }
    else if (!strcmp(arg2, "on"))
    {
      SET_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
      result = 1;
    }
    else if (!strcmp(arg2, "off"))
    {
      REMOVE_BIT_AR(PRF_FLAGS(ch), tog_messages[toggle].toggle);
    }
    else
    {
      send_to_char(ch, "Value for %s must either be 'on' or 'off'.\r\n", tog_messages[toggle].command);
      return;
    }
  }
  if (result)
    send_to_char(ch, "%s", tog_messages[toggle].enable_msg);
  else
    send_to_char(ch, "%s", tog_messages[toggle].disable_msg);
}

/* new wizhelp function, courtesy of paragon codebase -zusuk */
void do_wizhelp(struct char_data *ch)
{
  extern int *cmd_sort_info;
  int no = 1, i, cmd_num;
  int level;

  send_to_char(ch, "The following privileged commands are available:\r\n");

  for (level = LVL_IMPL; level >= LVL_IMMORT; level--)
  {
    send_to_char(ch, "%sLevel %d%s:\r\n", CCCYN(ch, C_NRM), level, CCNRM(ch, C_NRM));
    for (no = 1, cmd_num = 1; complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++)
    {
      i = cmd_sort_info[cmd_num];

      if (complete_cmd_info[i].minimum_level != level)
        continue;

      send_to_char(ch, "%-14s%s", complete_cmd_info[i].command, no++ % 7 == 0 ? "\r\n" : "");
    }
    if (no % 7 != 1)
      send_to_char(ch, "\r\n");
    if (level != LVL_IMMORT)
      send_to_char(ch, "\r\n");
  }
}

ACMD(do_commands)
{
  int no, i, cmd_num, can_cmd;
  int wizhelp = 0, socials = 0, maneuvers = 0;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  const char *commands[1000];
  int overflow = sizeof(commands) / sizeof(commands[0]);

  if (!ch->desc)
    return;

  one_argument(argument, arg, sizeof(arg));

  if (*arg)
  {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) || IS_NPC(vict))
    {
      send_to_char(ch, "Who is that?\r\n");
      return;
    }
  }
  else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
  {
    wizhelp = 1;
    do_wizhelp(ch);
    return;
  }
  else if (subcmd == SCMD_MANEUVERS)
    maneuvers = 1;

  send_to_char(ch, "The following %s%s are available to %s:\r\n",
               wizhelp ? "privileged " : "",
               socials     ? "socials"
               : maneuvers ? "combat maneuvers"
                           : "commands",
               vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 0, cmd_num = 1;
       complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n';
       ++cmd_num)
  {

    i = cmd_sort_info[cmd_num];

    if (complete_cmd_info[i].minimum_level < 0 || GET_LEVEL(vict) < complete_cmd_info[i].minimum_level)
      continue;

    if ((complete_cmd_info[i].minimum_level >= LVL_IMMORT) != wizhelp)
      continue;

    if (!wizhelp && socials != (complete_cmd_info[i].command_pointer == do_action))
      continue;

    if (wizhelp && complete_cmd_info[i].command_pointer == do_action)
      continue;

    if (--overflow < 0)
      continue;

    if (maneuvers)
    {
      if (complete_cmd_info[i].command_check_pointer == NULL)
        continue;

      can_cmd = complete_cmd_info[i].command_check_pointer(ch, false);

      if (can_cmd == CANT_CMD_PERM) // char can't use this command, skip it.
        continue;

      // char has access to the command, copy to commands list with a useful color:
      // Red if they can't use it right now, green if they can.
      // Just send it to the character instead of using the column_list(), so that we can color.
      send_to_char(ch, "%s%-14s\tn\r\n", can_cmd == CAN_CMD ? "\tG" : "\tr", complete_cmd_info[i].command);
    }
    else
    {
      /* matching command: copy to commands list */
      commands[no++] = complete_cmd_info[i].command;
    }
  }
  /* display commands list in a nice columnized format */
  if (!maneuvers)
    column_list(ch, 0, commands, no, FALSE);
}

ACMDU(do_homelands)
{

  int i = 0;
  skip_spaces(&argument);

  if (!*argument)
  {
    
    send_to_char(ch, "\tcRegions of Faerun\tn\r\n\r\n");
    for (i = 1; i < NUM_REGIONS; i++)
    {
      send_to_char(ch, "%-20s ", regions[i]);
      if (((i - 1) % 3) == 2)
        send_to_char(ch, "\r\n");
    }
    if (((i - 1) % 3) != 2)
      send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\nTo view information on a specific region, type: 'homelands (region name)'.\r\n");
    return;
  }

  // capitalize first character of words
  for (i = 0; argument[i] != '\0'; i++)
  {
    // check first character is lowercase alphabet
    if (i == 0)
    {
      if ((argument[i] >= 'a' && argument[i] <= 'z'))
        argument[i] = argument[i] - 32; // subtract 32 to make it capital
      continue;                         // continue to the loop
    }
    if (argument[i] == ' ') // check space
    {
      // if space is found, check next character
      ++i;
      // check next character is lowercase alphabet
      if (argument[i] >= 'a' && argument[i] <= 'z')
      {
        argument[i] = argument[i] - 32; // subtract 32 to make it capital
        continue;                       // continue to the loop
      }
    }
    else
    {
      // all other uppercase characters should be in lowercase
      if (argument[i] >= 'A' && argument[i] <= 'Z')
        argument[i] = argument[i] + 32; // subtract 32 to make it small/lowercase
    }
  }

  for (i = 1; i < NUM_REGIONS; i++)
  {
    if (is_abbrev(argument, regions[i]))
    {
      display_region_info(ch, i);
      return;
    }
  }

  if (i >= NUM_REGIONS)
  {
    send_to_char(ch, "That is not a valid region.  Type 'homelands' to see a list.\r\n");
  }
}

ACMD(do_history)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int type;

  one_argument(argument, arg, sizeof(arg));

  if (is_abbrev(arg, "chat"))
    strlcpy(arg, "gossip", sizeof(arg));
  type = search_block(arg, history_types, FALSE);

  if (!*arg || type < 0)
  {
    int i;

    send_to_char(ch, "Usage: history <");
    for (i = 0; *history_types[i] != '\n'; i++)
    {
      send_to_char(ch, " %s ", history_types[i]);
      if (*history_types[i + 1] == '\n')
        send_to_char(ch, ">\r\n");
      else
        send_to_char(ch, "|");
    }
    return;
  }

  if (GET_HISTORY(ch, type) && GET_HISTORY(ch, type)->text && *GET_HISTORY(ch, type)->text)
  {
    struct txt_block *tmp;
    for (tmp = GET_HISTORY(ch, type); tmp; tmp = tmp->next)
      send_to_char(ch, "%s", tmp->text);
      /* Make this a 1 if you want history to clear after viewing */
#if 0
    free_history(ch, type);
#endif
  }
  else
    send_to_char(ch, "You have no history in that channel.\r\n");
}

ACMD(do_whois)
{
  struct char_data *victim = 0;
  int hours;
  int got_from_file = 0, c_r;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  clan_rnum c_n;

  one_argument(argument, buf, sizeof(buf));

  if (!*buf)
  {
    send_to_char(ch, "Whois who?\r\n");
    return;
  }

  if (!(victim = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
  {
    CREATE(victim, struct char_data, 1);
    clear_char(victim);

    new_mobile_data(victim);
    /* Allocate mobile event list */
    // victim->events = create_list();

    CREATE(victim->player_specials, struct player_special_data, 1);

    if (load_char(buf, victim) > -1)
      got_from_file = 1;
    else
    {
      send_to_char(ch, "There is no such player.\r\n");
      free_char(victim);
      return;
    }
  }

  if (IS_IN_CLAN(victim))
  {
    c_n = real_clan(GET_CLAN(victim));
    send_to_char(ch, "[Clan Data]\r\n");
    if ((c_r = GET_CLANRANK(victim)) == NO_CLANRANK)
    {
      send_to_char(ch, "Applied to : %s%s\r\n", CLAN_NAME(c_n), QNRM);
      send_to_char(ch, "Status     : %sAwaiting Approval%s\r\n", QBRED, QNRM);
    }
    else
    {
      send_to_char(ch, "Current Clan : %s%s\r\n", CLAN_NAME(c_n), QNRM);
      send_to_char(ch, "Clan Rank    : %s%s (Rank %d)\r\n", clan_list[c_n].rank_name[(c_r - 1)], QNRM, c_r);
    }
  }

  /* We either have our victim from file or he's playing or function has returned. */
  sprinttype(GET_SEX(victim), genders, buf, sizeof(buf));
  send_to_char(ch, "Name: %s %s\r\nSex: %s\r\n", GET_NAME(victim),
               (victim->player.title ? victim->player.title : ""), buf);

  /* Show immortals account information */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    send_to_char(ch, "Account Name: %s\r\n", get_char_account_name(GET_NAME(victim)));
  }

  // sprinttype(victim->player.chclass, CLSLIST_NAME, buf, sizeof (buf));
  snprintf(buf, sizeof(buf), "%s", CLSLIST_NAME(victim->player.chclass));
  send_to_char(ch, "Current Class: %s\r\n", buf);

  send_to_char(ch, "\tCClass(es):\tn ");

  int i, counter = 0;
  for (i = 0; i < MAX_CLASSES; i++)
  {
    if (CLASS_LEVEL(victim, i))
    {
      if (counter)
        send_to_char(ch, " / ");
      send_to_char(ch, "%d %s", CLASS_LEVEL(victim, i), CLSLIST_ABBRV(i));
      counter++;
    }
  }
  send_to_char(ch, "\r\n");

  if (IS_MORPHED(victim))
    send_to_char(ch, "Race : %s\r\n",
                 race_family_types[IS_MORPHED(victim)]);
  else if (GET_DISGUISE_RACE(victim))
    send_to_char(ch, "Race : %s\r\n",
                 race_list[GET_DISGUISE_RACE(ch)].name);
  else
    send_to_char(ch, "Race : %s\r\n",
                 race_list[GET_RACE(victim)].type_color);

  send_to_char(ch, "Level: %d\r\n", GET_LEVEL(victim));

  if (!(GET_LEVEL(victim) < LVL_IMMORT) ||
      (GET_LEVEL(ch) >= GET_LEVEL(victim)))
  {
    strlcpy(buf, (char *)asctime(localtime(&(victim->player.time.logon))), sizeof(buf));
    buf[10] = '\0';

    hours = (time(0) - victim->player.time.logon) / 3600;

    if (!got_from_file)
    {
      send_to_char(ch, "Last Logon: They're playing now!  (Idle %d Minutes)",
                   victim->char_specials.timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);

      if (!victim->desc)
        send_to_char(ch, "  (Linkless)\r\n");
      else
        send_to_char(ch, "\r\n");

      if (!IS_NPC(ch) && PRF_FLAGGED(victim, PRF_AFK))
        send_to_char(ch, "%s%s is afk right now, so %s may not respond to communication.%s\r\n", CBGRN(ch, C_NRM), GET_NAME(victim), GET_SEX(victim) == SEX_NEUTRAL ? "it" : (GET_SEX(victim) == SEX_MALE ? "he" : "she"), CCNRM(ch, C_NRM));
    }
    else if (hours > 0)
      send_to_char(ch, "Last Logon: %s (%d days & %d hours ago.)\r\n", buf, hours / 24, hours % 24);
    else
      send_to_char(ch, "Last Logon: %s (0 hours & %d minutes ago.)\r\n",
                   buf, (int)(time(0) - victim->player.time.logon) / 60);
  }

  if (has_mail(GET_IDNUM(victim)))
    send_to_char(ch, "They have mail waiting.\r\n");
  else
    send_to_char(ch, "They have no mail waiting.\r\n");

  if (!IS_NPC(victim) && victim->player.background)
  {
    send_to_char(ch, "\r\n");
    send_to_char(ch, "%s", victim->player.background);
    send_to_char(ch, "\r\n");
  }

  if (PLR_FLAGGED(victim, PLR_DELETED))
    send_to_char(ch, "***DELETED***\r\n");

  if (!got_from_file && victim->desc != NULL && GET_LEVEL(ch) >= LVL_STAFF)
  {
    protocol_t *prot = victim->desc->pProtocol;
    send_to_char(ch, "Client:  %s [%s]\r\n",
                 prot->pVariables[eMSDP_CLIENT_ID]->pValueString,
                 prot->pVariables[eMSDP_CLIENT_VERSION]->pValueString ? prot->pVariables[eMSDP_CLIENT_VERSION]->pValueString : "Unknown");
    send_to_char(ch, "Color:   %s\r\n", prot->pVariables[eMSDP_256_COLORS]->ValueInt ? "256 Color" : (prot->pVariables[eMSDP_ANSI_COLORS]->ValueInt ? "Ansi" : "None"));
    send_to_char(ch, "MXP:     %s\r\n", prot->bMXP ? "Yes" : "No");
    send_to_char(ch, "Charset: %s\r\n", prot->bCHARSET ? "Yes" : "No");
    send_to_char(ch, "MSP:     %s\r\n", prot->bMSP ? "Yes" : "No");
    send_to_char(ch, "GMCP:    %s\r\n", prot->bGMCP ? "Yes" : "No");
    send_to_char(ch, "MSDP:    %s\r\n", prot->bMSDP ? "Yes" : "No");
  }

  if (got_from_file)
    free_char(victim);
}

bool get_zone_levels(zone_rnum znum, char *buf)
{
  /* Create a string for the level restrictions for this zone. */
  if ((zone_table[znum].min_level == -1) && (zone_table[znum].max_level == -1))
  {
    sprintf(buf, "<Not Set!>");
    return FALSE;
  }

  if (zone_table[znum].min_level == -1)
  {
    sprintf(buf, "Up to level %d", zone_table[znum].max_level);
    return TRUE;
  }

  if (zone_table[znum].max_level == -1)
  {
    sprintf(buf, "Above level %d", zone_table[znum].min_level);
    return TRUE;
  }

  sprintf(buf, "Levels %d to %d", zone_table[znum].min_level, zone_table[znum].max_level);
  return TRUE;
}

ACMD(do_areas)
{
  int i, hilev = -1, lolev = -1, zcount = 0, lev_set, len = 0, tmp_len = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'}, *second, lev_str[MAX_INPUT_LENGTH] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'};
  //  char zvn[MAX_INPUT_LENGTH] = {'\0'};
  bool show_zone = FALSE, overlap = FALSE, overlap_shown = FALSE;
  //  float pop;
  //  clan_rnum ocr;
  int num_areas = 0;
  char zone_num[15];

  char areas[300][LONG_STRING];

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    lolev = GET_LEVEL(ch);
  }
  else if (is_abbrev(arg, "all"))
  {
    ;
  }
  else
  {
    /* There was an arg typed - check for level range */
    second = strchr(arg, '-');
    if (second)
    {
      /* Check for 1st value */
      if (second == arg)
        lolev = 0;
      else
        lolev = atoi(arg);

      /* Check for 2nd value */
      if (*(second + 1) == '\0' || !isdigit(*(second + 1)))
        hilev = 100;
      else
        hilev = atoi(second + 1);
    }
    else
    {
      /* No range - single number */
      lolev = atoi(arg);
      hilev = -1; /* No high level - indicates single level */
    }
  }
  if (hilev != -1 && lolev > hilev)
  {
    /* Swap hi and lo lev if needed */
    i = lolev;
    lolev = hilev;
    hilev = i;
  }

  if (GET_CLAN(ch) && GET_CLAN(ch) != NO_CLAN && GET_CLANRANK(ch) != NO_CLANRANK)
  {
    if (real_clan(GET_CLAN(ch)) != NO_CLAN)
    {
      /* show_popularity = TRUE; */
    }
  }

  if (hilev != -1)
    len = snprintf(buf, sizeof(buf), "Checking range: %s%d to %d%s\r\n", QYEL, lolev, hilev, QNRM);
  else if (lolev != -1)
    len = snprintf(buf, sizeof(buf), "Checking level: %s%d%s\r\n", QYEL, lolev, QNRM);
  else
    len = snprintf(buf, sizeof(buf), "Checking all areas.\r\n");

  for (i = 0; i <= top_of_zone_table; i++)
  { /* Go through the whole zone table */
    show_zone = FALSE;
    overlap = FALSE;

    if (ZONE_FLAGGED(i, ZONE_GRID))
    { 
      /* Is this zone 'on the grid' ?    */
      if (lolev == -1)
      {
        /* No range supplied, show all zones */
        show_zone = TRUE;
      }
      else if ((hilev == -1) && (lolev >= ZONE_MINLVL(i)) && (lolev <= ZONE_MAXLVL(i)))
      {
        /* Single number supplied, it's in this zone's range */
        show_zone = TRUE;
      }
      else if ((hilev != -1) && (lolev >= ZONE_MINLVL(i)) && (hilev <= ZONE_MAXLVL(i)))
      {
        /* Range supplied, it's completely within this zone's range (no overlap) */
        show_zone = TRUE;
      }
      else if ((hilev != -1) && ((lolev >= ZONE_MINLVL(i) && lolev <= ZONE_MAXLVL(i)) || (hilev <= ZONE_MAXLVL(i) && hilev >= ZONE_MINLVL(i))))
      {
        /* Range supplied, it overlaps this zone's range */
        show_zone = TRUE;
        overlap = TRUE;
      }
      else if (ZONE_MAXLVL(i) < 0 && (lolev >= ZONE_MINLVL(i)))
      {
        /* Max level not set for this zone, but specified min in range */
        show_zone = TRUE;
      }
      else if (ZONE_MAXLVL(i) < 0 && (hilev >= ZONE_MINLVL(i)))
      {
        /* Max level not set for this zone, so just display it as red */
        show_zone = TRUE;
        overlap = TRUE;
      }
    }
    /* need to edit the below to include the owning clan and popularity % of the clan */
    /* snprintf(zvn, sizeof(zvn), "%s[%s%3d%s]%s ", QCYN, QYEL, zone_table[i].number, QCYN, QNRM);
      if (show_popularity) {
        // Get the VNUM of the zone the player is currently in
        pop = get_popularity(zone_table[i].number, GET_CLAN(ch));
        ocr = real_clan(get_owning_clan(zone_table[i].number));
        send_to_char(ch, "@n(%3d) %s%s%-*s@n %s%s%s  Popularity: %s[%s%3.2f%%%s]%s  Owning Clan: %s%s@n\r\n", ++zcount,
                   PRF_FLAGGED(ch, PRF_SHOWVNUMS) ? zvn : "", overlap ? QRED : QCYN,
                   count_color_chars(zone_table[i].name)+30, zone_table[i].name,
                   lev_set ? "@c" : "@n", lev_set ? lev_str : "All Levels", QNRM,
                   QCYN, QYEL, pop, QCYN, QNRM, (ocr == NO_CLAN) ? "None!" : clan_list[ocr].clan_name, QNRM);
      } else {
        send_to_char(ch, "@n(%3d) %s%s%-*s@n %s%s@n\r\n", ++zcount,
                   PRF_FLAGGED(ch, PRF_SHOWVNUMS) ? zvn : "", overlap ? QRED : QCYN,
                     count_color_chars(zone_table[i].name)+30, zone_table[i].name,
                 lev_set ? "@c" : "@n", lev_set ? lev_str : "All Levels");   */

    if (show_zone)
    {
      if (overlap)
        overlap_shown = TRUE;
      lev_set = get_zone_levels(i, lev_str);
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "\tn(%3d) %s%-*s\tn %s%s\tn\r\n", ++zcount, overlap ? QRED : QCYN,
                         count_color_chars(zone_table[i].name) + 40, zone_table[i].name,
                         lev_set ? "\tc" : "\tn", lev_set ? lev_str : "All Levels");
      snprintf(zone_num, sizeof(zone_num), " \tc[%3d]\tn  ", zone_table[i].number);
      snprintf(areas[num_areas], sizeof(areas[num_areas]), "\tn %-*s\tn %s%s%s\tn\r\n",
                         count_color_chars(zone_table[i].name) + 40, zone_table[i].name,
                         zone_num, lev_set ? "\tc" : "\tn", lev_set ? lev_str : "All Levels");
      num_areas++;
      len += tmp_len;
    }
  }
  
  tmp_len = snprintf(buf + len, sizeof(buf) - len, "\r\n\r\n");
  len += tmp_len;

  tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%d%s area%s found.\r\n", QYEL, zcount, QNRM, zcount == 1 ? "" : "s");
  len += tmp_len;

  if (overlap_shown)
  {
    tmp_len = snprintf(buf + len, sizeof(buf) - len, "Areas shown in \trred\tn may have some creatures outside the specified range.\r\n");
    len += tmp_len;
  }

#if defined(CAMPAIGN_DL)
  tmp_len = snprintf(buf + len, sizeof(buf) - len, "To show all areas type 'areas all', or to filter zones by level see HELP AREAS.\r\n");
  len += tmp_len;
  tmp_len = snprintf(buf + len, sizeof(buf) - len, "To show more information on a specific zone, type HELP (zone name as shown in areas command).\r\n");
  len += tmp_len;
#else
  tmp_len = snprintf(buf + len, sizeof(buf) - len, "More areas are listed in HELP ZONES");
  len += tmp_len;
#endif

  // if (zcount == 0)
  if (num_areas == 0)
    send_to_char(ch, "No areas found.\r\n");
  else
  {
    send_to_char(ch, "        %-40s %6s %s\r\n", "Area Name", "Number", "Level Range");
    send_to_char(ch, "        %-40s %5s %s\r\n", "---------", "------", "-----------");
    int n, j;
    char temp[LONG_STRING];
    n = num_areas;

    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - i - 1; j++) {
            if (strcmp(areas[j], areas[j + 1]) > 0)
            {
                strcpy(temp, areas[j]);
                strcpy(areas[j], areas[j + 1]);
                strcpy(areas[j + 1], temp);
            }
        }
    }
    for (i = 0; i < num_areas; i++)
      send_to_char(ch, "(%3d) %s", i, areas[i]);
  }
}

ACMD(do_scan)
{
  int door;
  bool found = FALSE;
  int range;
  int maxrange = 3;
  room_rnum scanned_room = NOWHERE;

  if (ch)
    scanned_room = IN_ROOM(ch);
  else
    return;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_NOMAP) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "A magical force prevents you from seeing beyond this room.\r\n");
    return;
  }

  if (IS_AFFECTED(ch, AFF_BLIND) && !has_blindsense(ch))
  {
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
    return;
  }
  if (GET_POS(ch) < POS_SLEEPING)
  {
    send_to_char(ch, "You can't see anything but stars!\r\n");
    return;
  }
  if (IS_SET_AR(ROOM_FLAGS(scanned_room), ROOM_FOG) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "A fog makes it impossible to look far.\r\n");
    return;
  }
  if (IS_SET_AR(ROOM_FLAGS(scanned_room), ROOM_MAGICDARK))
  {
    send_to_char(ch, "Its too dark to see.\r\n");
    return;
  }

  /*
  if (char_has_ultra(ch) && ultra_blind(ch, ch->in_room)) {
    send_to_char("Its too bright to see.\r\n", ch);
    return;
  }
   */

  for (door = 0; door < DIR_COUNT; door++)
  {

    if (world[scanned_room].dir_option[door] &&
        !IS_SET(world[scanned_room].dir_option[door]->exit_info, EX_HIDDEN))
    {
      send_to_char(ch, "\tCScanning %s:\tn\r\n", dirs[door]);
      look_in_direction(ch, door);
    }

    for (range = 1; range <= maxrange; range++)
    {
      if (world[scanned_room].dir_option[door] &&
          world[scanned_room].dir_option[door]->to_room != NOWHERE &&
          !IS_SET(world[scanned_room].dir_option[door]->exit_info, EX_HIDDEN) &&
          !IS_SET(world[scanned_room].dir_option[door]->exit_info, EX_CLOSED))
      {

        scanned_room = world[scanned_room].dir_option[door]->to_room;

        if (IS_DARK(scanned_room) && !CAN_SEE_IN_DARK(ch))
        {
          if (world[scanned_room].people)
            send_to_char(ch, "%s: It's too dark to see, but you can hear shuffling.\r\n", dirs[door]);
          else
            send_to_char(ch, "%s: It is too dark to see anything.\r\n", dirs[door]);
          found = TRUE;
        }
        else
        {
          if (world[scanned_room].people)
          {
            list_scanned_chars(world[scanned_room].people, ch, range - 1, door);
            found = TRUE;
          }
        }
      }
      else
        break;
    } // end of range
    scanned_room = IN_ROOM(ch);
  } // end of directions
  if (!found)
  {
    send_to_char(ch, "\tcYou don't see anybody nearby.\tn\r\n");
  }
} // end of do_scan

/* informational command requested by screenreader users */
ACMD(do_hp)
{
  send_to_char(ch, "You have %d hit points out of %d total.\r\n",
               GET_HIT(ch), GET_MAX_HIT(ch));
}

/* informational command requested by screenreader users */
ACMD(do_tnl)
{
  send_to_char(ch, "You need %d experience points to reach your next level.\r\n",
               level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch));
}

/* informational command requested by screenreader users */
ACMD(do_moves)
{
  send_to_char(ch, "You have %d movement points left.\r\n", GET_MOVE(ch));
}

#ifdef CAMPAIGN_FR

ACMD(do_survey)
{

  int x, y, room, i = 0;

  if (!ch || IN_ROOM(ch) == NOWHERE)
    return;

  if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_WORLDMAP))
  {
    send_to_char(ch, "This command can only be used on the worldmap.\r\n");
    return;
  }

  set_x_y_coords(world[IN_ROOM(ch)].number, &x, &y, &room);

  send_to_char(ch, "\tAYou are in room x=%d, y=%d\r\n", x, y);
  for (i = 0; i < 80; i++)
    send_to_char(ch, "-");
  send_to_char(ch, "\tn\r\n");

  for (i = 0; i < NUM_MAP_POINTS; i++)
  {
    set_x_y_coords(atoi(asciimap_points[i][1]), &x, &y, &room);
    send_to_char(ch, "-- %-40s (%d,%d)\r\n", asciimap_points[i][0], x, y);
  }

  for (i = 0; i < 80; i++)
    send_to_char(ch, "-");
  send_to_char(ch, "\tn\r\n\r\n");
}

#else

/* survey - get information on zone locations and current position, a ?temporary?
            solution for screen readers, etc
            ToDo:  limit to a certain distance based on perception maybe?
            -Zusuk */
ACMD(do_survey)
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  room_rnum nr, to_room;
  int first, last, j;
  struct room_data *target_room = NULL;

  zrnum = world[IN_ROOM(ch)].zone;
  zvnum = zone_table[zrnum].number;

  if (!ZONE_FLAGGED(zrnum, ZONE_WILDERNESS))
  {
    send_to_char(ch, "You can only survey in the wilderness.\n\r");
    return;
  }

  if (zrnum == NOWHERE || zvnum == NOWHERE)
  {
    send_to_char(ch, "Let a staff know about this, error with survey.\n\r");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT &&
      !has_blindsense(ch))
  {
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
    return;
  }

  if (IS_SET_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_FOG) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "A fog makes the task of surveying impossible!\r\n");
    return;
  }

  if (char_has_ultra(ch) && ULTRA_BLIND(ch, IN_ROOM(ch)))
  {
    send_to_char(ch, "Its too bright to survey!\r\n");
    return;
  }

  last = zone_table[zrnum].top;
  first = zone_table[zrnum].bot;

  send_to_char(ch, "You survey the wilderness:\r\n\r\n");

  for (nr = 0; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++)
  {
    if (GET_ROOM_VNUM(nr) >= first)
    {
      for (j = 0; j < DIR_COUNT; j++)
      {
        if (world[nr].dir_option[j])
        {
          target_room = &world[nr];
          to_room = world[nr].dir_option[j]->to_room;

          if (to_room != NOWHERE && (zrnum != world[to_room].zone) && target_room)
          {
            send_to_char(ch, "%s at (\tC%d\tn, \tC%d\tn) to the [%s]\r\n",
                         zone_table[world[to_room].zone].name,
                         target_room->coords[0], target_room->coords[1], dirs[j]);
          }
        }
      }
    }
  }

  send_to_char(ch, "\r\nYour Current Location : (\tC%d\tn, \tC%d\tn)\r\n", ch->coords[0], ch->coords[1]);
}

#endif

/* see exits */
ACMD(do_exits)
{
  int door, len = 0;

  if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT &&
      !has_blindsense(ch))
  {
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
    return;
  }

  if (IS_SET_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_FOG) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "A fog makes any exits indistinguishable.\r\n");
    return;
  }

  if (char_has_ultra(ch) && ULTRA_BLIND(ch, IN_ROOM(ch)))
  {
    send_to_char(ch, "Its too bright to see.\r\n");
    return;
  }

  send_to_char(ch, "Obvious exits:\r\n");

  for (door = 0; door < DIR_COUNT; door++)
  {
    /* zusuk debug, testing get_direction_vnum() */
    /*
    send_to_char(ch, "What vnum is in direction: %s?  %d.\r\n", dirs[door],
                  get_direction_vnum(IN_ROOM(ch), door));
     */
    /***************/
    if (!EXIT(ch, door) || EXIT(ch, door)->to_room == NOWHERE)
      continue;
    if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) && !CONFIG_DISP_CLOSED_DOORS)
      continue;
    if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
      continue;

    len++;

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS) && !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
      send_to_char(ch, "%-5s - [%5d]%s %s\r\n", dirs[door], GET_ROOM_VNUM(EXIT(ch, door)->to_room),
                   EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) ? " [HIDDEN]" : "", world[EXIT(ch, door)->to_room].name);
    else if (CONFIG_DISP_CLOSED_DOORS && EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
    {
      /* But we tell them the door is closed */
      send_to_char(ch, "%-5s - The %s is closed%s\r\n", dirs[door],
                   (EXIT(ch, door)->keyword) ? fname(EXIT(ch, door)->keyword) : "opening",
                   EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) ? " and hidden." : ".");
    }
    else
      send_to_char(ch, "%-5s - %s\r\n", dirs[door], IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch) ? "Too dark to tell." : world[EXIT(ch, door)->to_room].name);
  }

  if (!len)
    send_to_char(ch, " None.\r\n");
}

/* work in progress by Ornir */
/*
ACMD(do_track) {

  send_to_char(ch, "This skill is under construction currently..\r\n");
  return;

  struct event * pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  char creator_race[20]; // The RACE of what created the tracks.
  char creator_name[20]; // The NAME of what created the tracks.
  int track_age = 0;     // The AGE of this set of tracks.
  char track_dir[6];     // The direction the track leads.

  const char* track_age_names[7] = {"extremely old",
                                    "very old",
                                    "old",
                                    "fairly recent",
                                    "recent",
                                    "fairly fresh",
                                    "fresh"};

  // The character must have the track skill.
  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_TRACK)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  one_argument(argument, arg);
  send_to_char(ch, "You search the area for tracks...\r\n");

  // Check if there are any tracks to see.
  // Need to iterate over all of this room's events, picking out the tracking ones.
  // What a PAIN, this really should be easier.
  //for (pMudEvent = room_has_mud_event(world[IN_ROOM(ch)], eTRACKS);
  //     pMudEvent != NULL;
  //     pMudEvent = room_has_mud_event(world[IN_ROOM(ch)], eTRACKS)) {

    if ((!room_has_mud_event(&world[IN_ROOM(ch)], eTRACKS)) ||
        ((world[IN_ROOM(ch)].events == NULL) || (world[IN_ROOM(ch)].events->iSize == 0))) {
      send_to_char(ch, "You can't find any tracks.\r\n");
      return;
    }

    simple_list(NULL);

    while ((pEvent = (struct event *) simple_list(world[IN_ROOM(ch)].events)) != NULL) {
      if (!pEvent->isMudEvent)
        continue;

      pMudEvent = (struct mud_event_data *) pEvent->event_obj;

      if (pMudEvent->iId == eTRACKS) {

        // Get the track information from the sVariables.
        if (pMudEvent->sVariables)
          sscanf(pMudEvent->sVariables, "%d \"%19[^\"]\" \"%19[^\"]\" %s", &track_age, creator_race, creator_name, track_dir);

        // Skill check.
        send_to_char(ch, "%s\r\n", pMudEvent->sVariables);
        send_to_char(ch, "%d %s %s %s\r\n", track_age, creator_race, creator_name, track_dir);

        if (*arg && isname(arg, creator_name)) {
          //Found our victim's tracks.
          send_to_char(ch, "  You find %s tracks of %s leading %s.\r\n", track_age_names[track_age],
                       creator_name,
                       track_dir);

        } else if ((!*arg) || (*arg && isname(arg, creator_race))) {
          send_to_char(ch, "  You find %s tracks of %s %s leading %s.\r\n", track_age_names[track_age],
                       a_or_an(creator_race),
                       creator_race,
                       track_dir);
        }
      }
    }
  }
//}
 */

/* Event function for tracks, causing decay and eventual removal. */
EVENTFUNC(event_tracks)
{
  struct mud_event_data *pMudEvent = NULL;
  char buf[128];

  char creator_race[20]; /* The RACE of what created the tracks. */
  char creator_name[20]; /* The NAME of what created the tracks. */
  int track_age = 0;     /* The AGE of this set of tracks. */
  char track_dir[6];     /* The direction the track leads. */

  /* Unpack the mud data. */
  pMudEvent = (struct mud_event_data *)event_obj;

  if (!pMudEvent)
    return 0;

  if (!pMudEvent->iId)
    return 0;

  /* room = (struct room_data *)pMudEvent->pStruct; */

  /* Get the track information from the sVariables. */
  if (pMudEvent->sVariables)
    sscanf(pMudEvent->sVariables, "%d \"%19[^\"]\" \"%19[^\"]\" %s", &track_age, creator_race, creator_name, track_dir);

  if (track_age == 0) /* Time for this track to disappear. */
    return 0;
  else
    track_age--; /* Age the track. */

  /* Now change the age in the sVariables, and resubmit the tracks. */
  if (pMudEvent->sVariables)
    free(pMudEvent->sVariables);
  snprintf(buf, sizeof(buf), "%d \"%s\" \"%s\" %s", track_age, creator_race, creator_name, track_dir);
  pMudEvent->sVariables = strdup(buf);

  return 60 RL_SEC; /* Decay tracks every 60 seconds, subject to change :) */
}

/* rank command, in rank.c */
ACMD(do_rank)
{
  do_slug_rank(ch, argument);
}

void display_weapon_families(struct char_data *ch)
{
  int i = 0;

  for (i = 0; i < NUM_WEAPON_FAMILIES; i++)
  {
    send_to_char(ch, "%s\r\n", weapon_family[i]);
  }
}

ACMD(do_weapontypes)
{
  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Current weapon families are:\r\n");
    display_weapon_families(ch);
    send_to_char(ch, "Type: 'weapontypes (family name)' to view weapons in that family.\r\n");
    return;
  }

  int i = 0, j = 0;

  for (i = 0; i < NUM_WEAPON_FAMILIES; i++)
  {
    if (is_abbrev(argument, weapon_family[i]))
      break;
  }

  if (i >= NUM_WEAPON_FAMILIES)
  {
    send_to_char(ch, "Current weapon families are:\r\n");
    display_weapon_families(ch);
    send_to_char(ch, "There is no weapon family by that name.\r\n");
    send_to_char(ch, "Type: 'weapontypes (family name)' to view weapons in that family.\r\n");
    return;
  }

  const char *family_weapons[NUM_WEAPON_TYPES];
  int counter = 0;

  send_to_char(ch, "The %s weapon family includes the following weapons:\r\n\r\n", weapon_family[i]);
  for (j = 0; j < NUM_WEAPON_TYPES; j++)
  {
    if (weapon_list[j].weaponFamily == i)
    {
      family_weapons[counter] = weapon_list[j].name;
      counter++;
    }
  }

  column_list(ch, 2, family_weapons, counter, TRUE);
}

ACMD(do_weaponlist)
{

  const char *cmd_weapon_names[NUM_WEAPON_TYPES];

  int j = 0;

  for (j = 0; j < NUM_WEAPON_TYPES; j++)
  {
    cmd_weapon_names[j] = weapon_list[j].name;
  }

  column_list(ch, 4, cmd_weapon_names, j, TRUE);

  send_to_char(ch, "\tDType 'weaponinfo <name of weapon>' to see details\tn\r\n");
  send_to_char(ch, "\tDType 'armorlist' to see a list of armor types\tn\r\n");
}

ACMD(do_armorlist)
{

  const char *cmd_armor_names[NUM_SPEC_ARMOR_SUIT_TYPES - 1];

  int j = 0;

  for (j = 1; j < NUM_SPEC_ARMOR_SUIT_TYPES; j++)
  {
    cmd_armor_names[j - 1] = armor_list[j].name;
  }

  column_list(ch, 3, cmd_armor_names, j - 1, TRUE);

  send_to_char(ch, "\tDType 'armorinfo <name of armor>' to see details\tn\r\n");
  send_to_char(ch, "\tDType 'weaponlist' to see a list of weapon types\tn\r\n");
}

int is_weapon_proficient(int weapon, int type)
{

  if (type == WPT_SIMPLE)
  {
    if (IS_SET(weapon_list[weapon].weaponFlags, WEAPON_FLAG_SIMPLE))
      return true;
  }
  else if (type == WPT_MARTIAL)
  {
    if (IS_SET(weapon_list[weapon].weaponFlags, WEAPON_FLAG_MARTIAL))
      return true;
  }
  else if (type == WPT_EXOTIC)
  {
    if (IS_SET(weapon_list[weapon].weaponFlags, WEAPON_FLAG_EXOTIC))
      return true;
  }
  else if (type == WPT_MONK)
  {
    if (weapon == WEAPON_TYPE_UNARMED)
      return true;
    if (weapon_list[weapon].weaponFamily == WEAPON_FAMILY_MONK)
      return true;
  }
  else if (type == WPT_DRUID)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_CLUB:
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_KNIFE:
    case WEAPON_TYPE_QUARTERSTAFF:
    case WEAPON_TYPE_SCIMITAR:
    case WEAPON_TYPE_SCYTHE:
    case WEAPON_TYPE_SICKLE:
    case WEAPON_TYPE_SHORTSPEAR:
    case WEAPON_TYPE_SLING:
    case WEAPON_TYPE_SPEAR:
      return TRUE;
    }
  }
  else if (type == WPT_BARD)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_LONG_SWORD:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_SAP:
    case WEAPON_TYPE_SHORT_SWORD:
    case WEAPON_TYPE_SHORT_BOW:
    case WEAPON_TYPE_WHIP:
      return TRUE;
    }
  }
  else if (type == WPT_ROGUE)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_SAP:
    case WEAPON_TYPE_SHORT_SWORD:
    case WEAPON_TYPE_SHORT_BOW:
      return TRUE;
    }
  }
  else if (type == WPT_WIZARD)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_KNIFE:
    case WEAPON_TYPE_QUARTERSTAFF:
    case WEAPON_TYPE_CLUB:
    case WEAPON_TYPE_HEAVY_CROSSBOW:
    case WEAPON_TYPE_LIGHT_CROSSBOW:
      return TRUE;
    }
  }
  else if (type == WPT_PSIONICIST)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_KNIFE:
    case WEAPON_TYPE_QUARTERSTAFF:
    case WEAPON_TYPE_CLUB:
    case WEAPON_TYPE_HEAVY_CROSSBOW:
    case WEAPON_TYPE_LIGHT_CROSSBOW:
    case WEAPON_TYPE_SHORTSPEAR:
      return TRUE;
    }
  }
  else if (type == WPT_SHADOWDANCER)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_CLUB:
    case WEAPON_TYPE_HEAVY_CROSSBOW:
    case WEAPON_TYPE_LIGHT_CROSSBOW:
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_KNIFE:
    case WEAPON_TYPE_KUKRI:
    case WEAPON_TYPE_DART:
    case WEAPON_TYPE_LIGHT_MACE:
    case WEAPON_TYPE_HEAVY_MACE:
    case WEAPON_TYPE_MORNINGSTAR:
    case WEAPON_TYPE_QUARTERSTAFF:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_SAP:
    case WEAPON_TYPE_SHORT_BOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
    case WEAPON_TYPE_SHORT_SWORD:
      return TRUE;
    }
  }
  else if (type == WPT_DROW)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_SHORT_SWORD:
      return TRUE;
    }
  }
  else if (type == WPT_ELF)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_LONG_SWORD:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_LONG_BOW:
    case WEAPON_TYPE_COMPOSITE_LONGBOW:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_2:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_3:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_4:
    case WEAPON_TYPE_COMPOSITE_LONGBOW_5:
    case WEAPON_TYPE_SHORT_BOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
      return TRUE;
    }
  }
  else if (type == WPT_DWARF)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_BATTLE_AXE:
    case WEAPON_TYPE_HEAVY_PICK:
    case WEAPON_TYPE_WARHAMMER:
    case WEAPON_TYPE_DWARVEN_WAR_AXE:
    case WEAPON_TYPE_DWARVEN_URGOSH:
      return TRUE;
    }
  }
  else if (type == WPT_DUERGAR)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_BATTLE_AXE:
    case WEAPON_TYPE_HEAVY_PICK:
    case WEAPON_TYPE_WARHAMMER:
    case WEAPON_TYPE_DWARVEN_WAR_AXE:
    case WEAPON_TYPE_DWARVEN_URGOSH:
      return TRUE;
    }
  }
  else if (type == WPT_ASSASSIN)
  {
    switch (weapon)
    {
    case WEAPON_TYPE_HAND_CROSSBOW:
    case WEAPON_TYPE_LIGHT_CROSSBOW:
    case WEAPON_TYPE_HEAVY_CROSSBOW:
    case WEAPON_TYPE_DAGGER:
    case WEAPON_TYPE_KNIFE:
    case WEAPON_TYPE_DART:
    case WEAPON_TYPE_RAPIER:
    case WEAPON_TYPE_SHORT_BOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_2:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_3:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_4:
    case WEAPON_TYPE_COMPOSITE_SHORTBOW_5:
    case WEAPON_TYPE_SHORT_SWORD:
      return TRUE;
    }
  }

  /* nothing! */
  return false;
}

ACMD(do_weaponproficiencies)
{
  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Please specify one of the following weapon proficiency types:\r\n"
                     "simple\r\n"
                     "martial\r\n"
                     "exotic\r\n"
                     "monk\r\n"
                     "druid\r\n"
                     "bard\r\n"
                     "rogue\r\n"
                     "wizard\r\n"
                     "drow\r\n"
                     "elf\r\n"
                     "dwarf\r\n"
                     "duergar\r\n"
                     "psionicist\r\n"
                     "shadowdancer\r\n"
                     "assassin\r\n"
                     "\r\n");
    return;
  }

  int type = 0;

  if (is_abbrev(argument, "simple"))
  {
    type = WPT_SIMPLE;
  }
  else if (is_abbrev(argument, "martial"))
  {
    type = WPT_MARTIAL;
  }
  else if (is_abbrev(argument, "exotic"))
  {
    type = WPT_EXOTIC;
  }
  else if (is_abbrev(argument, "monk"))
  {
    type = WPT_MONK;
  }
  else if (is_abbrev(argument, "druid"))
  {
    type = WPT_DRUID;
  }
  else if (is_abbrev(argument, "bard"))
  {
    type = WPT_BARD;
  }
  else if (is_abbrev(argument, "rogue"))
  {
    type = WPT_ROGUE;
  }
  else if (is_abbrev(argument, "wizard"))
  {
    type = WPT_WIZARD;
  }
  else if (is_abbrev(argument, "drow"))
  {
    type = WPT_DROW;
  }
  else if (is_abbrev(argument, "elf"))
  {
    type = WPT_ELF;
  }
  else if (is_abbrev(argument, "dwarf"))
  {
    type = WPT_DWARF;
  }
  else if (is_abbrev(argument, "duergar"))
  {
    type = WPT_DUERGAR;
  }
  else if (is_abbrev(argument, "psionicist"))
  {
    type = WPT_PSIONICIST;
  }
  else if (is_abbrev(argument, "shadowdancer"))
  {
    type = WPT_SHADOWDANCER;
  }
  else if (is_abbrev(argument, "assassin"))
  {
    type = WPT_ASSASSIN;
  }
  else
  {
    send_to_char(ch, "Please specify one of the following weapon proficiency types:\r\n"
                     "simple\r\n"
                     "martial\r\n"
                     "exotic\r\n"
                     "monk\r\n"
                     "druid\r\n"
                     "bard\r\n"
                     "rogue\r\n"
                     "wizard\r\n"
                     "drow\r\n"
                     "elf\r\n"
                     "dwarf\r\n"
                     "duergar\r\n"
                     "psionicist\r\n"
                     "shadowdancer\r\n"
                     "assassin\r\n"
                     "\r\n");
    return;
  }

  int i = 0;

  send_to_char(ch, "Weapons available for proficiency '%s'\r\n", argument);
  for (i = 1; i < NUM_WEAPON_TYPES; i++)
  {
    if (is_weapon_proficient(i, type))
      send_to_char(ch, "--%s\r\n", weapon_list[i].name);
  }
}

ACMD(do_weaponinfo)
{

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Please specify a weapon type.\r\n"
                     "A list can be seen by using the weaponlist command.\r\n");
    return;
  }

  int type = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf2[100];
  char buf3[100];
  char buf4[100];
  char buf5[800];
  size_t len = 0;
  int crit_multi = 0;
  sbyte found = false;

  for (type = 0; type < NUM_WEAPON_TYPES; type++)
  {

    if (!is_abbrev(argument, weapon_list[type].name))
      continue;

    switch (type)
    {
    case WEAPON_TYPE_WHIP:
      snprintf(buf4, sizeof(buf4), "+5 to disarm and trip attempts");
      break;
    default:
      snprintf(buf4, sizeof(buf4), "N/A");
      break;
    }

    snprintf(buf5, sizeof(buf5), "%s", weapon_list[type].description);

    /* have to do some calculations beforehand */
    switch (weapon_list[type].critMult)
    {
    case CRIT_X2:
      crit_multi = 2;
      break;
    case CRIT_X3:
      crit_multi = 3;
      break;
    case CRIT_X4:
      crit_multi = 4;
      break;
    case CRIT_X5:
      crit_multi = 5;
      break;
    case CRIT_X6:
      crit_multi = 6;
      break;
    }
    sprintbit(weapon_list[type].weaponFlags, weapon_flags, buf2, sizeof(buf2));
    sprintbit(weapon_list[type].damageTypes, weapon_damage_types, buf3, sizeof(buf3));

    len += snprintf(buf + len, sizeof(buf) - len,
                    "\tCType       : \tW%s\tn\n"
                    "\tCDam        : \tW%dd%d\tn\n"
                    "\tCThreat     : \tW%d%s\tn\n"
                    "\tCCrit-Multi : \tWx%d\tn\n"
                    "\tCFlags      : \tW%s\tn\n"
                    "\tCCost       : \tW%d\tn\n"
                    "\tCDam-Types  : \tW%s\tn\n"
                    "\tCWeight     : \tW%d\tn\n"
                    "\tCRange      : \tW%d\tn\n"
                    "\tCFamily     : \tW%s\tn\n"
                    "\tCSize       : \tW%s\tn\n"
                    "\tCMaterial   : \tW%s\tn\n"
                    "\tCHandle     : \tW%s\tn\n"
                    "\tCHead       : \tW%s\tn\n"
                    "\tCSpecial    : \tW%s\tn\n"
                    "\tCDescription: \r\n\tn%s\tn\n",
                    weapon_list[type].name, weapon_list[type].numDice, weapon_list[type].diceSize,
                    (20 - weapon_list[type].critRange), weapon_list[type].critRange > 0 ? "-20" : "", crit_multi, buf2, weapon_list[type].cost,
                    buf3, weapon_list[type].weight, weapon_list[type].range,
                    weapon_family[weapon_list[type].weaponFamily],
                    sizes[weapon_list[type].size], material_name[weapon_list[type].material],
                    weapon_handle_types[weapon_list[type].handle_type],
                    weapon_head_types[weapon_list[type].head_type],
                    buf4, strfrmt(buf5, 80, 1, FALSE, FALSE, FALSE));
    found = true;
    break;
  }

  if (!found)
  {
    send_to_char(ch, "That is not a valid weapon type.\r\n");
    return;
  }

  page_string(ch->desc, buf, 1);
}

ACMD(do_armorinfo)
{

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Please specify an armor suit type.\r\n"
                     "A list can be seen by using the armorlist command.\r\n");
    return;
  }

  int type = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf2[800];
  size_t len = 0;
  sbyte found = false;

  for (type = 1; type < NUM_SPEC_ARMOR_SUIT_TYPES; type++)
  {

    if (!is_abbrev(argument, armor_list[type].name))
      continue;

    snprintf(buf2, sizeof(buf2), "%s", armor_list[type].description);

    len += snprintf(buf + len, sizeof(buf) - len,
                    "\tCName          : \tW%s\tn\n"
                    "\tCType          : \tW%s\tn\n"
                    "\tCCost          : \tW%d\tn\n"
                    "\tCAC            : \tW%d\tn\n"
                    "\tCMax Dex       : \tW%d\tn\n"
                    "\tCArmor Penalty : \tW%s%d\tn\n"
                    "\tCSpell Fail    : \tW%d%%\tn\n"
                    "\tCWeight        : \tW%d\tn\n"
                    "\tCMaterial      : \tW%s\tn\n"
                    "\tCDescription   : \r\n\tn%s\tn\n",
                    armor_list[type].name, armor_type[armor_list[type].armorType],
                    armor_list[type].cost, armor_suit_ac_bonus[type],
                    armor_list[type].dexBonus, armor_list[type].armorCheck > 0 ? "-" : "",
                    armor_list[type].armorCheck, armor_list[type].spellFail,
                    armor_suit_weight[type], material_name[armor_list[type].material],
                    strfrmt(buf2, 80, 1, FALSE, FALSE, FALSE));

    found = true;
    break;
  }

  if (!found)
  {
    send_to_char(ch, "That is not a valid armor suit type.\r\n");
    return;
  }

  page_string(ch->desc, buf, 1);
}

/* interface to see your npc army! */
ACMD(do_pets)
{
  check_npc_followers(ch, NPC_MODE_DISPLAY, 0);
}

/* brief display on left side of mobile indicating their toughness */
ACMD(do_autocon)
{
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOCON))
  {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_AUTOCON);
    send_to_char(ch, "Auto-consider has been turned off.\r\n");
    return;
  }
  else
  {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOCON);
    send_to_char(ch, "Auto-consider has been turned on.\r\n");
    return;
  }
}

ACMD(do_favoredenemies)
{
  if (GET_FAVORED_ENEMY(ch, 0) <= 0)
  {
    send_to_char(ch, "You do not have any favored enemies.\r\n");
    return;
  }

  int i = 0;

  send_to_char(ch, "Your favored enemies are: \r\n");

  for (i = 0; i < MAX_ENEMIES; i++)
  {
    if (GET_FAVORED_ENEMY(ch, i) > 0)
      send_to_char(ch, "-- %s\r\n", race_family_types_plural[GET_FAVORED_ENEMY(ch, i)]);
  }

  i = (CLASS_LEVEL(ch, CLASS_RANGER) / 5 + 2) + (HAS_FEAT(ch, FEAT_EPIC_FAVORED_ENEMY) ? 4 : 0);

  send_to_char(ch, "\r\n");
  send_to_char(ch, "When fighting a favored enemy, you benefit from:\r\n");
  send_to_char(ch, "-- +%d dodge bonus to armor class.\r\n", i);
  send_to_char(ch, "-- +%d bonus to weapon or unarmed damage.\r\n", i);
  send_to_char(ch, "-- +%d morale bonus to weapon or unarmed attack rolls.\r\n", i);
  send_to_char(ch, "\r\n");
}

SPECIAL(eqstats)
{

  if (!CMD_IS("eqstats"))
    return FALSE;

  int cost = GET_LEVEL(ch) * 100;

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "It costs %d gold coins to be able to show a summary of your worn equipment enchantments.\r\n", cost);
    return TRUE;
  }

  GET_GOLD(ch) -= cost;

  int i, k, lore_bonus = 0;
  int found = false;
  struct obj_data *obj = NULL;
  char buf2[300], bitbuf[300];

  send_to_char(ch, "You are using:\r\n");
  for (i = 0; i < NUM_WEARS; i++)
  {
    found = false;
    if (GET_EQ(ch, i))
    {
      obj = GET_EQ(ch, i);
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
      {
        send_to_char(ch, "%-30s", wear_where[i]);

        if (HAS_FEAT(ch, FEAT_KNOWLEDGE))
        {
          lore_bonus += 4;
          if (GET_WIS_BONUS(ch) > 0)
            lore_bonus += GET_WIS_BONUS(ch);
        }
        if (CLASS_LEVEL(ch, CLASS_BARD) && HAS_FEAT(ch, FEAT_BARDIC_KNOWLEDGE))
        {
          lore_bonus += CLASS_LEVEL(ch, CLASS_BARD);
        }

        /* good enough lore for object? */
        if (GET_EQ(ch, i) && GET_OBJ_COST(GET_EQ(ch, i)) > lore_app[(compute_ability(ch, ABILITY_LORE) + lore_bonus)])
        {
          send_to_char(ch, " (couldn't identify)\r\n");
          continue;
        }
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON || GET_OBJ_TYPE(obj) == ITEM_ARMOR)
          send_to_char(ch, " %s Enhancement: %d ",
                       GET_OBJ_TYPE(obj) == ITEM_ARMOR ? (CAN_WEAR(obj, ITEM_WEAR_SHIELD) ? "Shield" : "Armor") : "Weapon",
                       GET_ENHANCEMENT_BONUS(obj));

        for (k = 0; k < MAX_OBJ_AFFECT; k++)
        {
          if ((obj->affected[k].location != APPLY_NONE) && (obj->affected[k].modifier != 0))
          {
            if (!found)
            {
              found = true;
            }
            sprinttype(obj->affected[k].location, apply_types,
                       bitbuf, sizeof(bitbuf));
            switch (obj->affected[k].location)
            {
            case APPLY_FEAT:
              snprintf(buf2, sizeof(buf2), " (%s)",
                       feat_list[obj->affected[k].modifier].name);
              send_to_char(ch, " %s%s", bitbuf, buf2);
              break;
            default:
              buf2[0] = 0;
              send_to_char(ch, " %s%s %s%d (%s)", bitbuf, buf2,
                           (obj->affected[k].modifier > 0) ? "+"
                                                           : "",
                           obj->affected[k].modifier,
                           bonus_types[obj->affected[k].bonus_type]);
              break;
            }
          }
        }
        send_to_char(ch, "\r\n");
      }
      else
      {
        send_to_char(ch, "%-30s", wear_where[i]);
        send_to_char(ch, "Something.\r\n");
      }
    }
    else
    {
      if (!GET_EQ(ch, i))
      {
        send_to_char(ch, "%-30s<empty>\r\n", wear_where[i]);
      }
    }
  }
  return TRUE;
}

ACMD(do_divine_bond)
{

  if (!HAS_FEAT(ch, FEAT_DIVINE_BOND))
  {
    send_to_char(ch, "You do not have the paladin feat divine bond.\r\n");
    return;
  }

  send_to_char(ch, "Your divine bond with your weapon gives you the following bonuses:\r\n\r\n");

  send_to_char(ch, "+%d to hit and damage rolls when wielding a weapon.\r\n",
               MIN(6, 1 + MAX(0, (CLASS_LEVEL(ch, CLASS_PALADIN) - 5) / 3)));

  if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 10)
    send_to_char(ch, "+1d6 holy damage against non-good foes.\r\n");

  if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 20)
    send_to_char(ch, "+1d6 fire damage.\r\n");

  if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 30)
    send_to_char(ch, "+2d10 fire damage on critical hits.\r\n");

  send_to_char(ch, "\r\n");
}

ACMD(do_mercies)
{
  if (CLASS_LEVEL(ch, CLASS_PALADIN) < 3)
  {
    send_to_char(ch, "You don't know any paladin mercies.\r\n");
    return;
  }

  int i = 0;

  send_to_char(ch, "Paladin Mercies\r\n");
  for (i = 0; i < 80; i++)
    send_to_char(ch, "-");
  send_to_char(ch, "\r\n");

  for (i = 1; i < NUM_PALADIN_MERCIES; i++)
  {
    send_to_char(ch, "[");
    if (KNOWS_MERCY(ch, i))
    {
      send_to_char(ch, "\tg%-7s\tn", "KNOWN");
    }
    else
    {
      send_to_char(ch, "\tr%-7s\tn", "UNKNOWN");
    }
    send_to_char(ch, "] %-15s : %s\r\n", paladin_mercies[i], paladin_mercy_descriptions[i]);
  }

  send_to_char(ch, "\r\nSee HELP MERCIES and HELP LAYONHANDS for more information on how mercies work.\r\n\r\n");
}

ACMD(do_cruelties)
{
  if (CLASS_LEVEL(ch, CLASS_BLACKGUARD) < 3)
  {
    send_to_char(ch, "You don't know any blackguard cruelties.\r\n");
    return;
  }

  int i = 0;

  send_to_char(ch, "Blackguard Cruelties\r\n");
  for (i = 0; i < 80; i++)
    send_to_char(ch, "-");
  send_to_char(ch, "\r\n");

  for (i = 1; i < NUM_BLACKGUARD_CRUELTIES; i++)
  {
    send_to_char(ch, "[");
    if (KNOWS_CRUELTY(ch, i))
    {
      send_to_char(ch, "\tg%-7s\tn", "KNOWN");
    }
    else
    {
      send_to_char(ch, "\tr%-7s\tn", "UNKNOWN");
    }
    send_to_char(ch, "] %-15s : %s\r\n", blackguard_cruelties[i], blackguard_cruelty_descriptions[i]);
  }

  send_to_char(ch, "\r\nSee HELP CRUELTIES and HELP TOUCH-OF-CORRUPTION for more information on how cruelties work.\r\n\r\n");
}

ACMD(do_maxhp)
{
  calculate_max_hp(ch, true);
}

ACMD(do_flightlist)
{

  int i = 0;

  #ifdef CAMPAIGN_FR

    char zone[200];

    text_line(ch, "\tYOverland Flight Spell Destinations\tC", 80, '-', '-');
    for (i = 0; i < NUM_ZONE_ENTRANCES; i++)
    {
      snprintf(zone, sizeof(zone), "%s (%s)", zone_entrances[i][0], zone_entrances[i][1]);
      send_to_char(ch, "%-39s ", zone);
      if ((i % 2) == 1)
        send_to_char(ch, "\r\n");
    }
    if ((i % 2) != 1)
        send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");
  #else
    i = 0;
    text_line(ch, "\tYOverland Flight Spell Destinations\tC", 80, '-', '-');
    #if defined(CAMPAIGN_DL)
    text_line(ch, "\tYCarriage Stops:\tC", 80, '-', '-');
#endif
    while (atoi(carriage_locales[i][1]) != 0)
    {
      send_to_char(ch, "%-39s ", carriage_locales[i][0]);
      if ((i % 2) == 1)
        send_to_char(ch, "\r\n");
      i++;
    }
    if ((i % 2) != 1)
      send_to_char(ch, "\r\n");
    send_to_char(ch, "\r\n");
    i = 0;
    #if defined(CAMPAIGN_DL)
    text_line(ch, "\tYSailing Ports\tC", 80, '-', '-');
    while (atoi(sailing_locales[i][1]) != 0)
    {
      send_to_char(ch, "%-39s ", sailing_locales[i][0]);
      if ((i % 2) == 1)
        send_to_char(ch, "\r\n");
      i++;
      }
      if ((i % 2) != 1)
          send_to_char(ch, "\r\n");
      send_to_char(ch, "\r\n");
    #endif
  #endif
}

ACMD(do_touch_spells)
{
  int i;

  send_to_char(ch, "List of touch spells:\r\n");

  for (i = 0; i < NUM_SPELLS; i++)
  {
    if (spell_info[i].touch_spell)
    {
      send_to_char(ch, "-- %-25s (%s)\r\n", spell_info[i].name, spell_schools[spell_info[i].schoolOfMagic]);
    }
  }
}

bool char_has_any_item_activation_abilities(struct char_data *ch)
{
  if (!ch) return false;

  struct obj_data *obj = NULL;
  int i = 0;

  for (i = 0; i < NUM_WEARS; i++)
  {
    if ((obj = GET_EQ(ch, i)))
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
        return true;
    }
  }
  return false;
}

bool char_has_any_item_activation_ability_cooldowns(struct char_data *ch)
{
  if (!ch) return false;

  struct obj_data *obj = NULL;
  int i = 0;

  for (i = 0; i < NUM_WEARS; i++)
  {
    if ((obj = GET_EQ(ch, i)))
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0 && obj->activate_spell[ACT_SPELL_COOLDOWN] > 0)
        return true;
    }
  }
  return false;
}

void list_item_activate_abilities(struct char_data *ch)
{

  int i = 0, total = 0, remaining = 0;
  struct obj_data *obj = NULL;
  char obj_desc[200];

  if (!char_has_any_item_activation_abilities(ch))
  {
    return;
  }

  text_line(ch, "\tYItem Activations\tC", 90, '-', '-');

  for (i = 0; i < NUM_WEARS; i++)
  {
    if ((obj = GET_EQ(ch, i)))
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_COOLDOWN] == 0 && obj->activate_spell[ACT_SPELL_CURRENT_USES] == 0)
          obj->activate_spell[ACT_SPELL_CURRENT_USES] = obj->activate_spell[ACT_SPELL_MAX_USES];
        remaining = obj->activate_spell[ACT_SPELL_CURRENT_USES];
        total = obj->activate_spell[ACT_SPELL_MAX_USES];
        snprintf(obj_desc, sizeof(obj_desc), "%s", obj->short_description);
        strip_colors(obj_desc);
        send_to_char(ch,
                    "%-30.30s \tc%-20s\tn %s%2d\tn/%-2d uses remaining\r\n",
                    obj_desc,
                    spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name,
                    (remaining > (total / 2) ? "\tn" : (remaining <= 1 ? "\tR" : "\tY")),
                    remaining,
                    total);
      }
    }
  }
}

void list_item_activate_ability_cooldowns(struct char_data *ch)
{

  int i = 0;
  struct obj_data *obj = NULL;
  char obj_desc[200];

  if (!char_has_any_item_activation_abilities(ch))
  {
    return;
  }

  text_line(ch, "\tYItem Activation Cooldowns\tC", 80, '-', '-');

  for (i = 0; i < NUM_WEARS; i++)
  {
    if ((obj = GET_EQ(ch, i)))
    {
      if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        if (obj->activate_spell[ACT_SPELL_COOLDOWN] == 0 && obj->activate_spell[ACT_SPELL_CURRENT_USES] == 0)
          obj->activate_spell[ACT_SPELL_CURRENT_USES] = obj->activate_spell[ACT_SPELL_MAX_USES];
        if (obj->activate_spell[ACT_SPELL_COOLDOWN] <= 0)
          continue;
        snprintf(obj_desc, sizeof(obj_desc), "%s", obj->short_description);
        strip_colors(obj_desc);
        send_to_char(ch,
                    "%-30.30s \tc%-20s\tn %d mins for +1 use\r\n",
                    obj_desc, spell_info[obj->activate_spell[ACT_SPELL_SPELLNUM]].name,
                    obj->activate_spell[ACT_SPELL_COOLDOWN]);
      }
    }
  }
}

ACMD(do_roomvnum)
{
  if (IN_ROOM(ch) == NOWHERE)
  {
    send_to_char(ch, "Error. Please tell staff ERRVNUMROOM001\r\n");
    return;
  }

  send_to_char(ch, "This room's vnum is %d.\r\n", world[IN_ROOM(ch)].number);
  return;
}

ACMD(do_wearlocations)
{
  int i;

  send_to_char(ch, "\tCWear Locations:\tn\r\n");

  for (i = 1; i < NUM_ITEM_WEARS; i++)
  {
    column_list(ch, 3, wear_bits, NUM_ITEM_WEARS, FALSE);
    return;
  }
}

ACMDU(do_wearapplies)
{

  skip_spaces(&argument);
  int i, j, wear_loc, count = 0;
  char wears[100];
  char *apply_list[NUM_APPLIES];

  if (!*argument)
  {
    send_to_char(ch, "You need to specify a wear location. You can see a list by typing: wearlocations.\r\n");
    return;
  }

  for (i = 1; i < NUM_ITEM_WEARS; i++)
  {
    snprintf(wears, sizeof(wears), "%s", wear_bits[i]);
    for (j = 0; j < strlen(wears); j++)
      wears[j] = tolower(wears[j]);

    if (is_abbrev(argument, wears))
      break;
  }

  if (i >= NUM_ITEM_WEARS)
  {
    send_to_char(ch, "That is not a valid wear location. You can see a list by typing: wearlocations.\r\n");
    return;
  }

  wear_loc = i;

  for (i = 0; i < NUM_APPLIES; i++)
  {
    if (is_bonus_valid_for_where_slot(i, wear_loc))
    {
      apply_list[count] = strdup(apply_types[i]);
      count++;
    }
  }

  send_to_char(ch, "\tCApply Types for Wear Location %s:\tn\r\n", wear_bits[wear_loc]);
  column_list(ch, 3, (const char **)apply_list, count, FALSE);
  send_to_char(ch, "\r\n");

}

#undef WPT_SIMPLE
#undef WPT_MARTIAL
#undef WPT_EXOTIC
#undef WPT_MONK
#undef WPT_DRUID
#undef WPT_BARD
#undef WPT_ROGUE
#undef WPT_WIZARD
#undef WPT_DROW
#undef WPT_ELF
#undef WPT_DWARF
#undef WPT_DUERGAR

/*EOF*/
