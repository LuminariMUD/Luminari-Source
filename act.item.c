/**************************************************************************
 *  File: act.item.c                                   Part of LuminariMUD *
 *  Usage: Object handling routines -- get/drop and container handling.    *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "screen.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"
#include "oasis.h"
#include "act.h"
#include "quest.h"
#include "spec_procs.h"
#include "clan.h"
#include "mud_event.h"
#include "hlquest.h"
#include "fight.h"
#include "mudlim.h"
#include "handler.h"
#include "actions.h"
#include "traps.h" /* for check_traps() */
#include "assign_wpn_armor.h"
#include "spec_abilities.h"
#include "item.h"
#include "feats.h"
#include "alchemy.h"
#include "mysql.h"
#include "treasure.h"
#include "crafts.h"

/* local function prototypes */
/* do_get utility functions */
static int can_take_obj(struct char_data *ch, struct obj_data *obj);
static void get_check_money(struct char_data *ch, struct obj_data *obj);
static void get_from_container(struct char_data *ch, struct obj_data *cont, char *arg, int mode, int amount);
static void get_from_room(struct char_data *ch, char *arg, int amount);
static void perform_get_from_container(struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int mode);
static int perform_get_from_room(struct char_data *ch, struct obj_data *obj);
/* do_give utility functions */
static struct char_data *give_find_vict(struct char_data *ch, char *arg);
static void perform_give_gold(struct char_data *ch, struct char_data *vict, int amount);
/* do_drop utility functions */
static int perform_drop(struct char_data *ch, struct obj_data *obj, byte mode, const char *sname, room_rnum RDR);
static void perform_drop_gold(struct char_data *ch, int amount, byte mode, room_rnum RDR);
/* do_put utility functions */
static void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont);
/* do_remove utility functions */
/* do_wear utility functions */
static int hands_have(struct char_data *ch);
int hands_used(struct char_data *ch);
int hands_available(struct char_data *ch);
static void wear_message(struct char_data *ch, struct obj_data *obj, int where);

/**** start file code *****/

/*
        case ITEM_CLANARMOR:
          if (GET_OBJ_VAL(obj, 2) == NO_CLAN) {
            len += snprintf(buf + len, sizeof (buf) - len,
                    "- Clan ID not set on CLANARMOR\r\n");
          } else if (real_clan(GET_OBJ_VAL(obj, 2)) == NO_CLAN) {
            len += snprintf(buf + len, sizeof (buf) - len,
                    "- Invalid Clan ID on CLANARMOR\r\n");
          }
 */
/* values 0 is reserved for Apply to AC */
/*
    case ITEM_CLANARMOR:
      write_to_output(d, "Clan ID Number: ");
      break;
 */

/* assistant function for statting/identify/lore of objects */
void display_item_object_values(struct char_data *ch, struct obj_data *item, int mode)
{
  struct char_data *tempch = NULL, *pet = NULL;
  struct obj_special_ability *specab;
  obj_rnum target_obj = NOTHING;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int line_length = 80, i = 0;
  char actmtds[MAX_STRING_LENGTH];
  int (*name)(struct char_data * ch, void *me, int cmd, char *argument);
  bool found = FALSE;

  /* zusuk set these up for quicker setup of new items */
  int v1 = GET_OBJ_VAL(item, 0);
  int v2 = GET_OBJ_VAL(item, 1);
  int v3 = GET_OBJ_VAL(item, 2);
  int v4 = GET_OBJ_VAL(item, 3);

  text_line(ch, "\tcItem-Type Specific Values:\tn", line_length, '-', '-');

  switch (GET_OBJ_TYPE(item))
  {

  case ITEM_SWITCH: /* 35 */ /* activation mechanism */
    if (mode == ITEM_STAT_MODE_IMMORTAL)
    {
      send_to_char(ch, "[%s, affecting room VNum %d, %s %s]\r\n",
                   (v1 == 0) ? "Push switch" : (v1 == 1) ? "Pull switch" : "BROKEN switch type",
                   v2,
                   (v4 == 0) ? "Unhides" : (v4 == 1) ? "Unlocks" : (v4 == 2) ? "Opens" : "BROKEN exit action",
                   (v3 == 0) ? "North" : (v3 == 1) ? "East" : (v3 == 2) ? "South" : (v3 == 3) ? "West" : (v3 == 4) ? "Up" : (v3 == 5) ? "Down" : "BROKEN direction");
    }
    else
    {
      send_to_char(ch, "This appears to be a %s switch...\r\n",
                   v1 == 0 ? "\tcpush\tn" : v1 == 1 ? "\tcpull\tn" : "(broken! report to staff)");
    }
    break;

  case ITEM_TRAP: /* 31 */ /* punish those rogue-less groups! */
    /* object value (0) is the trap-type */
    /* object value (1) is the direction of the trap (TRAP_TYPE_OPEN_DOOR and TRAP_TYPE_UNLOCK_DOOR)
           or the object-vnum (TRAP_TYPE_OPEN_CONTAINER and TRAP_TYPE_UNLOCK_CONTAINER and TRAP_TYPE_GET_OBJECT) */
    /* object value (2) is the effect */
    /* object value (3) is the trap difficulty */
    /* object value (4) is whether this trap has been "detected" yet */
    send_to_char(ch, "Trap type: %s\r\n", trap_type[GET_OBJ_VAL(item, 0)]);
    switch (GET_OBJ_VAL(item, 0))
    {
    case TRAP_TYPE_ENTER_ROOM:
      break;
    case TRAP_TYPE_LEAVE_ROOM:
      break;
    case TRAP_TYPE_OPEN_DOOR:
      /*fall-through*/
    case TRAP_TYPE_UNLOCK_DOOR:
      send_to_char(ch, "Direction: %s\r\n", dirs[GET_OBJ_VAL(item, 1)]);
      break;
    case TRAP_TYPE_OPEN_CONTAINER:
      /*fall-through*/
    case TRAP_TYPE_UNLOCK_CONTAINER:
      /*fall-through*/
    case TRAP_TYPE_GET_OBJECT:
      target_obj = real_object(GET_OBJ_VAL(item, 1));

      send_to_char(ch, "Target Object: %s\r\n",
                   (target_obj == NOTHING) ? "Nothing" : obj_proto[target_obj].short_description);
      break;
    }
    if (GET_OBJ_VAL(item, 2) <= 0 || GET_OBJ_VAL(item, 2) >= TOP_TRAP_EFFECTS)
    {
      send_to_char(ch, "Invalid trap effect on this object [1]\r\n");
    }
    else if (GET_OBJ_VAL(item, 2) < TRAP_EFFECT_FIRST_VALUE && GET_OBJ_VAL(item, 2) >= LAST_SPELL_DEFINE)
    {
      send_to_char(ch, "Invalid trap effect on this object [2]\r\n");
    }
    else if (GET_OBJ_VAL(item, 2) >= TRAP_EFFECT_FIRST_VALUE)
    {
      /* homeland port */
      send_to_char(ch, "Trap effect: %s\r\n", trap_effects[GET_OBJ_VAL(item, 2) - 1000]);
    }
    else
    {
      send_to_char(ch, "Spell effect: %s\r\n", spell_info[GET_OBJ_VAL(item, 2)].name);
    }
    send_to_char(ch, "Trap DC: %d\r\n", GET_OBJ_VAL(item, 3));
    break;

  case ITEM_LIGHT: /* 1 */ /**< Item is a light source */
    if (GET_OBJ_VAL(item, 2) == -1)
      send_to_char(ch, "Hours left: Infinite\r\n");
    else
      send_to_char(ch, "Hours left: [%d]\r\n", GET_OBJ_VAL(item, 2));
    break;
  case ITEM_SCROLL: /* 2 */ /* fallthrough */
  case ITEM_POTION:         /* 10 */
    send_to_char(ch, "Spells: (Level %d) %s, %s, %s\r\n", GET_OBJ_VAL(item, 0),
                 skill_name(GET_OBJ_VAL(item, 1)), skill_name(GET_OBJ_VAL(item, 2)),
                 skill_name(GET_OBJ_VAL(item, 3)));
    break;

  case ITEM_WAND: /* 3 */ /* fallthrough */
  case ITEM_STAFF:        /* 4 */
    send_to_char(ch, "Spell: %s at level %d, %d (of %d) charges remaining\r\n",
                 skill_name(GET_OBJ_VAL(item, 3)), GET_OBJ_VAL(item, 0),
                 GET_OBJ_VAL(item, 2), GET_OBJ_VAL(item, 1));
    break;

  case ITEM_FIREWEAPON: /* 7 */
    send_to_char(ch,
                 "**Deprecated, report to staff to fix this item**\r\n"
                 "Type:                   %s\r\n"
                 "Damage:                 %d\r\n"
                 "Breaking Probability:   %d percent\r\n",
                 ranged_weapons[GET_OBJ_VAL(item, 0)], GET_OBJ_VAL(item, 1),
                 GET_OBJ_VAL(item, 2));
    break;

  case ITEM_WEAPON: /* 5 */
    /* weapon poison */
    if (item->weapon_poison.poison)
    {
      send_to_char(ch, "Weapon Poisoned: %s, Level of Poison: %d, Applications Left: %d\r\n",
                   spell_info[item->weapon_poison.poison].name,
                   item->weapon_poison.poison_level,
                   item->weapon_poison.poison_hits);
    }

    send_to_char(ch, "Weapon Type: %s (%d) Enhancement Bonus: %d\r\n",
                 weapon_list[GET_WEAPON_TYPE(item)].name,
                 GET_WEAPON_TYPE(item),
                 GET_ENHANCEMENT_BONUS(item));
    send_to_char(ch, "Todam: %dd%d, Avg Damage: %.1f.\r\n",
                 GET_OBJ_VAL(item, 1), GET_OBJ_VAL(item, 2),
                 ((GET_OBJ_VAL(item, 2) + 1) / 2.0) * GET_OBJ_VAL(item, 1));

    /* weapon special abilities*/
    send_to_char(ch, "Special Abilities:\r\n");
    for (specab = item->special_abilities; specab != NULL; specab = specab->next)
    {
      found = TRUE;
      sprintbit(specab->activation_method, activation_methods, actmtds, MAX_STRING_LENGTH);
      send_to_char(ch, "Ability: %s Level: %d\r\n"
                       "    Activation Methods: %s\r\n"
                       "    CommandWord: %s\r\n"
                       "    Values: [%d] [%d] [%d] [%d]\r\n",
                   special_ability_info[specab->ability].name,
                   specab->level, actmtds,
                   (specab->command_word == NULL ? "Not set." : specab->command_word),
                   specab->value[0], specab->value[1], specab->value[2], specab->value[3]);
    }
    if (!found)
      send_to_char(ch, "No weapon special abilities assigned.\r\n");

    /* weapon spells */
    send_to_char(ch, "Weapon Spells:\r\n");
    if (!item->has_spells)
      send_to_char(ch, "No weapon spells on this weapon!\r\n");
    else
    {
      for (i = 0; i < MAX_WEAPON_SPELLS; i++)
      { /* increment this weapons spells */
        if (GET_WEAPON_SPELL(item, i))
        {
          send_to_char(ch, "%s, Level: %d, Percent: %d, Procs in combat?: %s\r\n",
                       spell_info[GET_WEAPON_SPELL(item, i)].name, GET_WEAPON_SPELL_LVL(item, i),
                       GET_WEAPON_SPELL_PCT(item, i),
                       GET_WEAPON_SPELL_AGG(item, i) ? "Yes" : "No");
        }
      }
    }

    /* values defined by weapon type */
    int weapon_val = GET_OBJ_VAL(item, 0);
    int crit_multi = 0;
    switch (weapon_list[weapon_val].critMult)
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
    send_to_char(ch, "Values defined by weapon type:\r\n");
    sprintbit(weapon_list[weapon_val].weaponFlags, weapon_flags, buf, sizeof(buf));
    if (mode == ITEM_STAT_MODE_IMMORTAL)
    {
      send_to_char(ch, "Damage: %dD%d, Threat: %d, Crit. Multi: %d, Weapon Flags: %s\r\n",
                   weapon_list[weapon_val].numDice, weapon_list[weapon_val].diceSize,
                   (20 - weapon_list[weapon_val].critRange),
                   crit_multi, buf);
    }
    else
    {
      send_to_char(ch, "Damage: %dD%d, Threat: %d, Crit. Multi: %d, Weapon Flags: %s\r\n",
                   weapon_list[weapon_val].numDice, weapon_list[weapon_val].diceSize,
                   (20 - weapon_list[weapon_val].critRange),
                   crit_multi, buf);
    }
    sprintbit(weapon_list[weapon_val].damageTypes, weapon_damage_types, buf2, sizeof(buf2));
    if (mode == ITEM_STAT_MODE_IMMORTAL)
    {
      send_to_char(ch, "Sugg. Cost: %d, Damage-Types: %s, Sugg. Weight: %d\r\n",
                   weapon_list[weapon_val].cost, buf2, weapon_list[weapon_val].weight);
    }
    else
    {
      send_to_char(ch, "Damage-Types: %s\r\n", buf2);
    }
    send_to_char(ch, "Range: %d, Family: %s\r\n",
                 weapon_list[weapon_val].range, weapon_family[weapon_list[weapon_val].weaponFamily]);
    if (mode == ITEM_STAT_MODE_IMMORTAL)
    {
      send_to_char(ch, "Sugg. Size: %s, Sugg. Material: %s, Handle Type: %s, Head Type: %s\r\n",
                   sizes[weapon_list[weapon_val].size], material_name[weapon_list[weapon_val].material],
                   weapon_handle_types[weapon_list[weapon_val].handle_type],
                   weapon_head_types[weapon_list[weapon_val].head_type]);
    }
    else
    {
      send_to_char(ch, "Handle Type: %s, Head Type: %s\r\n",
                   weapon_handle_types[weapon_list[weapon_val].handle_type],
                   weapon_head_types[weapon_list[weapon_val].head_type]);
    }

    break;

  case ITEM_ARMOR: /* 9 */
    if (mode == ITEM_STAT_MODE_IMMORTAL)
    {
      send_to_char(ch, "AC-apply: [%d], Enhancement Bonus: +%d\r\n",
                   GET_OBJ_VAL(item, 0), GET_ENHANCEMENT_BONUS(item));
    }
    else
    {
      /* players should see the float value */
      send_to_char(ch, "AC-apply: [%.1f], Enhancement Bonus: +%d\r\n",
                   (float)GET_OBJ_VAL(item, 0) / 10.0, GET_ENHANCEMENT_BONUS(item));
    }
    /* values defined by armor type */
    int armor_val = GET_OBJ_VAL(item, 1);
    send_to_char(ch, "Values defined by armor type:\r\n");
    if (mode == ITEM_STAT_MODE_IMMORTAL)
    {
      send_to_char(ch, "Armor: %s, Type: %s, Sugg. Cost: %d, Sugg. AC: %d,\r\n",
                   armor_list[armor_val].name,
                   armor_type[armor_list[armor_val].armorType],
                   armor_list[armor_val].cost,
                   armor_list[armor_val].armorBonus);
    }
    else
    {
      send_to_char(ch, "Armor-Proficiency: %s, Armor-Type: %s\r\n", armor_list[armor_val].name,
                   armor_type[armor_list[armor_val].armorType]);
    }
    send_to_char(ch, "Max Dex Bonus: %d, Armor-Check: %d, Spell-Fail: %d, 30ft: %d, 20ft: %d,\r\n",
                 armor_list[armor_val].dexBonus,
                 armor_list[armor_val].armorCheck,
                 armor_list[armor_val].spellFail,
                 armor_list[armor_val].thirtyFoot, armor_list[armor_val].twentyFoot);
    if (mode == ITEM_STAT_MODE_IMMORTAL)
    {
      send_to_char(ch, "Sugg. Weight: %d, Sugg. Material: %s, Sugg. Wear-Slot: %s\r\n",
                   armor_list[armor_val].weight,
                   material_name[armor_list[armor_val].material],
                   wear_bits[armor_list[armor_val].wear]);
    }
    /* Special abilities*/
    found = FALSE;
    send_to_char(ch, "Special Abilities:\r\n");
    for (specab = item->special_abilities; specab != NULL; specab = specab->next)
    {
      found = TRUE;
      sprintbit(specab->activation_method, activation_methods, actmtds, MAX_STRING_LENGTH);
      send_to_char(ch, "Ability: %s Level: %d\r\n"
                       "    Activation Methods: %s\r\n"
                       "    CommandWord: %s\r\n"
                       "    Values: [%d] [%d] [%d] [%d]\r\n",
                   special_ability_info[specab->ability].name,
                   specab->level, actmtds,
                   (specab->command_word == NULL ? "Not set." : specab->command_word),
                   specab->value[0], specab->value[1], specab->value[2], specab->value[3]);
    }
    if (!found)
      send_to_char(ch, "No special abilities assigned.\r\n");

    break;

  case ITEM_CONTAINER: /* 15 */
    sprintbit(GET_OBJ_VAL(item, 1), container_bits, buf, sizeof(buf));
    send_to_char(ch, "Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse: %s\r\n",
                 GET_OBJ_VAL(item, 0), buf, GET_OBJ_VAL(item, 2),
                 YESNO(GET_OBJ_VAL(item, 3)));
    break;

  case ITEM_AMMO_POUCH: /* 36 */
    sprintbit(GET_OBJ_VAL(item, 1), container_bits, buf, sizeof(buf));
    send_to_char(ch, "Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse?: %s\r\n",
                 GET_OBJ_VAL(item, 0), buf, GET_OBJ_VAL(item, 2),
                 YESNO(GET_OBJ_VAL(item, 3)));
    break;

  case ITEM_DRINKCON: /* fallthrough */ /* 17 */
  case ITEM_FOUNTAIN:                   /* 23 */
    sprinttype(GET_OBJ_VAL(item, 2), drinks, buf, sizeof(buf));
    send_to_char(ch, "Capacity: %d, Contains: %d, Spell: %s:%s, Liquid: %s\r\n",
                 GET_OBJ_VAL(item, 0), GET_OBJ_VAL(item, 1), YESNO(GET_OBJ_VAL(item, 3)),
                 (GET_OBJ_VAL(item, 3) > 0) ? spell_info[GET_OBJ_VAL(item, 3)].name : "none", buf);
    break;

  case ITEM_NOTE: /* 16 */
    send_to_char(ch, "Tongue: %d\r\n", GET_OBJ_VAL(item, 0));
    break;

  case ITEM_BOAT: /* 22 */
    break;

  case ITEM_KEY: /* 18 */
    break;

  case ITEM_FOOD: /* 19 */
    send_to_char(ch, "Makes full: %d, Spellnum: %d (%s), Poisoned: %s\r\n", GET_OBJ_VAL(item, 0), GET_OBJ_VAL(item, 1), spell_info[GET_OBJ_VAL(item, 1)].name, YESNO(GET_OBJ_VAL(item, 3)));
    break;

  case ITEM_MONEY: /* 20 */
    send_to_char(ch, "Coins: %d\r\n", GET_OBJ_VAL(item, 0));
    break;

  case ITEM_PORTAL: /* 29 */
    if (mode == ITEM_STAT_MODE_IMMORTAL)
    {
      if (GET_OBJ_VAL(item, 0) == PORTAL_NORMAL)
        send_to_char(ch, "Type: Normal Portal to %d\r\n", GET_OBJ_VAL(item, 1));
      else if (GET_OBJ_VAL(item, 0) == PORTAL_RANDOM)
        send_to_char(ch, "Type: Random Portal to range %d-%d\r\n", GET_OBJ_VAL(item, 1), GET_OBJ_VAL(item, 2));
      else if (GET_OBJ_VAL(item, 0) == PORTAL_CLANHALL)
        send_to_char(ch, "Type: Clanportal (destination depends on player)\r\n");
      else if (GET_OBJ_VAL(item, 0) == PORTAL_CHECKFLAGS)
        send_to_char(ch, "Type: Checkflags Portal to %d\r\n", GET_OBJ_VAL(item, 1));
    }
    break;

  case ITEM_FURNITURE: /* 6 */
    send_to_char(ch, "Can hold: [%d] Num. of People in: [%d]\r\n", GET_OBJ_VAL(item, 0), GET_OBJ_VAL(item, 1));
    send_to_char(ch, "Holding : ");
    for (tempch = OBJ_SAT_IN_BY(item); tempch; tempch = NEXT_SITTING(tempch))
      send_to_char(ch, "%s ", GET_NAME(tempch));
    send_to_char(ch, "\r\n");
    break;

  case ITEM_MISSILE: /* 14 */
    /* weapon poison */
    if (item->weapon_poison.poison)
    {
      send_to_char(ch, "Weapon Poisoned: %s, Level of Poison: %d, Applications Left: %d\r\n",
                   spell_info[item->weapon_poison.poison].name,
                   item->weapon_poison.poison_level,
                   item->weapon_poison.poison_hits);
    }
    send_to_char(ch,
                 "Type:                   %s\r\n"
                 "Enhancement:            %d\r\n"
                 "Imbued with spell:      %d\r\n"
                 "Duration left on imbue: %d hours\r\n"
                 "Breaking Probability:   %d percent\r\n",
                 ammo_types[GET_OBJ_VAL(item, 0)], GET_OBJ_VAL(item, 4), GET_OBJ_VAL(item, 1),
                 GET_OBJ_TIMER(item), GET_OBJ_VAL(item, 2));
    if (mode == ITEM_STAT_MODE_IMMORTAL)
      send_to_char(ch, "Missile belongs to: %ld\r\n", MISSILE_ID(item));
    break;

  case ITEM_SPELLBOOK: /* 28 */
    display_spells(ch, item);
    break;

  case ITEM_POISON: /* 33 */
    send_to_char(ch, "Poison:       %s\r\n", skill_name(GET_OBJ_VAL(item, 0)));
    send_to_char(ch, "Level:        %d\r\n", GET_OBJ_VAL(item, 1));
    send_to_char(ch, "Applications: %d\r\n", GET_OBJ_VAL(item, 2));
    send_to_char(ch, "Hits/App:     %d\r\n", GET_OBJ_VAL(item, 3));
    break;

  case ITEM_WORN: /* 11 */
    /* monk glove */
    if (CAN_WEAR(item, ITEM_WEAR_HANDS) && GET_OBJ_VAL(item, 0))
      send_to_char(ch, "Monk Glove Enchantment: %d\r\n", GET_OBJ_VAL(item, 0));
    /* default */
    else
    {
      send_to_char(ch, "Wearable item.\r\n");
    }
    break;

  case ITEM_CRYSTAL: /* 25 */
    send_to_char(ch, "Arcanite crafting crystal - can be used in crafting.\r\n");
    break;

  case ITEM_TREASURE: /* 8 */
    break;

  case ITEM_OTHER: /* 12 */
    break;

  case ITEM_TRASH: /* 13 */
    break;

  case ITEM_PEN: /* 21 */
    break;

  case ITEM_CLANARMOR: /* 24 */
    break;

  case ITEM_ESSENCE: /* 26 */
    break;

  case ITEM_MATERIAL: /* 27 */
    break;

  case ITEM_PLANT: /* 30 */
    break;

  case ITEM_TELEPORT: /* 32 */
    /* portal replaced this */
    break;

  case ITEM_SUMMON: /* 34 */
    /* needs to be implemented from HL! */
    break;

  case ITEM_PICK: /* 37 */
    break;

  case ITEM_INSTRUMENT: /* 38 */
    send_to_char(ch, "Instrument class: %s\r\n", instrument_names[GET_OBJ_VAL(item, 0)]);
    send_to_char(ch, "Difficulty:       %d\r\n", GET_OBJ_VAL(item, 1));
    send_to_char(ch, "Level:            %d\r\n", GET_OBJ_VAL(item, 2));
    send_to_char(ch, "Breakability:     %d\r\n", GET_OBJ_VAL(item, 3));

  case ITEM_DISGUISE: /* 39 */
    break;

  case ITEM_WALL: /* 40 */
    /* quick out */
    if (GET_OBJ_VAL(item, WALL_TYPE) >= NUM_WALL_TYPES ||
        GET_OBJ_VAL(item, WALL_TYPE) < 0)
    {
      send_to_char(ch, "Invalid wall type, let staff know please.\r\n");
      break;
    }

    //char *wall_lname = wallinfo[GET_OBJ_VAL(item, WALL_TYPE)].longname;
    //char *wall_keyword = wallinfo[GET_OBJ_VAL(item, WALL_TYPE)].keyword;
    int wall_spellnum = wallinfo[GET_OBJ_VAL(item, WALL_TYPE)].spell_num;
    bool wall_stopmove = wallinfo[GET_OBJ_VAL(item, WALL_TYPE)].stops_movement;
    char *wall_sname = wallinfo[GET_OBJ_VAL(item, WALL_TYPE)].shortname;
    int wall_duration = wallinfo[GET_OBJ_VAL(item, WALL_TYPE)].duration;
    int wall_level = 0;

    struct char_data *wall_creator = find_char(GET_OBJ_VAL(item, WALL_IDNUM));
    bool found_player = FALSE;

    /* lets see if we can find the wall creator! */
    if (!wall_creator)
    {
      /* player probably logged out, so just use WALL_LEVEL to determine
       * damage */
      found_player = FALSE;
      wall_level = GET_OBJ_VAL(item, WALL_LEVEL);
    }
    else
    {
      wall_level = GET_LEVEL(wall_creator);
      found_player = TRUE;
    }

    send_to_char(ch, "Wall Type:                  %s\r\n", wall_sname);
    send_to_char(ch, "Stops movement? :           %s\r\n", wall_stopmove ? "yes" : "no");
    send_to_char(ch, "Direction wall is blocking: %s\r\n", dirs[GET_OBJ_VAL(item, WALL_DIR)]);
    send_to_char(ch, "Level:                      %d\r\n", wall_level);
    /* duration = 0 is default:  */
    send_to_char(ch, "Duration:                   %d\r\n", wall_duration ? wall_duration : 1 + wall_level / 10);

    /* if we found the player, we can also check if this victim is a friend! */
    if (found_player && !aoeOK(wall_creator, ch, wall_spellnum))
    {
      send_to_char(ch, "*An ally created this wall, you can pass it safely.\r\n");
    }

    break;

  case ITEM_BOWL: /* 41 */
    break;

  case ITEM_INGREDIENT: /* 42 */
    break;

  case ITEM_BLOCKER: /* 43 */
    /* needs to be implemented from HL! */
    break;

  case ITEM_WAGON: /* 44 */
    break;

  case ITEM_RESOURCE: /* 45 */
    break;

  case ITEM_PET: /* 46 */
    pet = read_mobile(GET_OBJ_VNUM(item), VIRTUAL);

    if (!pet)
    {
      send_to_char(ch, "Broken item, let staff know!\r\n");
    }
    else
    {
      char_to_room(pet, 0);
      send_to_char(ch, "When bought, makes follower: %s\r\n", GET_NAME(pet));
      extract_char(pet);
    }

    break;

  case ITEM_BLUEPRINT: /* 47 */
    show_craft(ch, get_craft_from_id(GET_OBJ_VAL(item, 0)));
    break;

  case ITEM_TREASURE_CHEST: /* 48 */

    /*  The type guarantees one item of the specified type.
        Generic has equal chance for any type.  Gold provides 5x as much money. */
    switch (LOOTBOX_TYPE(item))
    {
    case LOOTBOX_TYPE_GENERIC:
      send_to_char(ch, "Generic treasure, equal chance for all item types.\r\n");
      break;
    case LOOTBOX_TYPE_WEAPON:
      send_to_char(ch, "Treasure: Weapons, guaranteed weapon, low chance for other items.\r\n");
      break;
    case LOOTBOX_TYPE_ARMOR:
      send_to_char(ch, "Treasure: Armor, guaranteed armor, low chance for other items.\r\n");
      break;
    case LOOTBOX_TYPE_CONSUMABLE:
      send_to_char(ch, "Treasure: Consumables, guaranteed at least one consumable, low chance for other items.\r\n");
      break;
    case LOOTBOX_TYPE_TRINKET:
      send_to_char(ch, "Treasure: Trinkets, guaranteed trinket (rings, bracers, etc), low chance for other items.\r\n");
      break;
    case LOOTBOX_TYPE_GOLD:
      send_to_char(ch, "Treasure: Gold, much more gold, low chance for other items.\r\n");
      break;
    case LOOTBOX_TYPE_CRYSTAL:
      send_to_char(ch, "Treasure: Crystal, garaunteed arcanite crystal, low chance for other items.\r\n");
      break;
    case LOOTBOX_TYPE_UNDEFINED:
    default:
      send_to_char(ch, "Treasure type is broken, let staff know please.\r\n");
      break;
    }

    /* This will determine the maximum bonus to be found on the items in the chest. */
    switch (LOOTBOX_LEVEL(item))
    {
    case LOOTBOX_LEVEL_MUNDANE:
      send_to_char(ch, "Grade: Mundane\r\n");
      break;

    case LOOTBOX_LEVEL_MINOR:
      send_to_char(ch, "Grade: Minor (level 10 or less)\r\n");
      break;

    case LOOTBOX_LEVEL_TYPICAL:
      send_to_char(ch, "Grade: Typical(level 15 or less)\r\n");
      break;

    case LOOTBOX_LEVEL_MEDIUM:
      send_to_char(ch, "Grade: Medium (level 20 or less)\r\n");
      break;

    case LOOTBOX_LEVEL_MAJOR:
      send_to_char(ch, "Grade: Major (level 25 or less)\r\n");
      break;

    case LOOTBOX_LEVEL_SUPERIOR:
      send_to_char(ch, "Grade: Superior (level 26 or higher)\r\n");
      break;

    case LOOTBOX_TYPE_UNDEFINED:
    default:
      send_to_char(ch, "Treasure grade is broken, let staff know please.\r\n");
      break;
    }

    break;

  default:
    send_to_char(ch, "Report this item to a coder to add the ITEM_type\r\n");
    break;
  }

  /* universal */
  if (mode == ITEM_STAT_MODE_IMMORTAL)
  {
    send_to_char(ch, "Values: ");
    for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
    {
      send_to_char(ch, "[%d] ", GET_OBJ_VAL(item, i));
    }
    send_to_char(ch, "\r\n");
  }
  //code to support proc information..
  if (GET_OBJ_RNUM(item) != NOTHING)
  {
    name = obj_index[GET_OBJ_RNUM(item)].func;
    if (mode == ITEM_STAT_MODE_IMMORTAL)
    {
      send_to_char(ch, "Special Procedure 'identify' tag:\r\n");
      if (name)
        (name)(ch, item, 0, "identify"); /* show identify info tagged in the actual proc */
    }
    else
    {
      send_to_char(ch, "Special 'identify' tag:\r\n");
      if (name)
        (name)(ch, item, 0, "identify"); /* show identify info tagged in the actual proc */
    }
  }
}

/* a central location for identification/statting of items */
void do_stat_object(struct char_data *ch, struct obj_data *j, int mode)
{
  int i, found, feat_num;
  obj_vnum vnum = GET_OBJ_VNUM(j);
  struct obj_data *j2;
  struct extra_descr_data *desc;
  char buf[MAX_STRING_LENGTH];
  int line_length = 80;

  text_line(ch, "\tcObject Information\tn", line_length, '-', '-');

  /* display id# related values */
  /* put object type in buf */
  sprinttype(GET_OBJ_TYPE(j), item_types, buf, sizeof(buf));
  if (mode == ITEM_STAT_MODE_IMMORTAL)
  {
    send_to_char(ch, "\tCType:\tn %s, VNum: [%5d], RNum: [%5d], Idnum: [%5ld], SpecProc: %s\r\n",
                 buf, vnum, GET_OBJ_RNUM(j), GET_ID(j),
                 GET_OBJ_SPEC(j) ? (get_spec_func_name(GET_OBJ_SPEC(j))) : "None");
  }
  else
  {
    send_to_char(ch, "\tCItem Type:\tn %s, Special Feature: %s\r\n", buf,
                 GET_OBJ_SPEC(j) ? (get_spec_func_name(GET_OBJ_SPEC(j))) : "None");
  }

  /* display description information */
  text_line(ch, "\tcDescription Information\tn", line_length, '-', '-');
  send_to_char(ch, "Name: '%s'\r\n",
               j->short_description ? j->short_description : "<None>");
  send_to_char(ch, "Keywords: %s\r\n", j->name);
  if (mode == ITEM_STAT_MODE_IMMORTAL)
  {
    send_to_char(ch, "L-Desc: '%s'\r\n",
                 j->description ? j->description : "<None>");
    send_to_char(ch, "A-Desc: '%s'\r\n",
                 j->action_description ? j->action_description : "<None>");
    if (j->ex_description)
    {
      send_to_char(ch, "Extra descriptions:");
      for (desc = j->ex_description; desc; desc = desc->next)
        send_to_char(ch, " [%s]", desc->keyword);
      send_to_char(ch, "\r\n");
    }
  }

  /* various variables */
  text_line(ch, "\tcVarious Variables\tn", line_length, '-', '-');
  send_to_char(ch, "Weight: %d, Value: %d, Cost/day: %d, Timer: %d, Min level: %d\r\n",
               GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j), GET_OBJ_LEVEL(j));
  send_to_char(ch, "Size: %s, Material: %s, Bound to: %s\r\n",
               size_names[GET_OBJ_SIZE(j)],
               material_name[GET_OBJ_MATERIAL(j)],
               (get_name_by_id(GET_OBJ_BOUND_ID(j)) != NULL) ? CAP(get_name_by_id(GET_OBJ_BOUND_ID(j))) : "(Unknown)");
  if (mode == ITEM_STAT_MODE_IMMORTAL)
  {
    for (i = 0; i < SPEC_TIMER_MAX; i++)
    {
      send_to_char(ch, "SpecTimer %d: %d | ", i, GET_OBJ_SPECTIMER(j, i));
    }
    send_to_char(ch, "\r\n");
  }
  else
  {
    bool display = FALSE;
    for (i = 0; i < SPEC_TIMER_MAX; i++)
    {
      if (GET_OBJ_SPECTIMER(j, i))
      {
        send_to_char(ch, "Special Timer %d: %d Game-Hours | ", (i + 1), GET_OBJ_SPECTIMER(j, i));
        display = TRUE;
      }
    }
    if (display)
      send_to_char(ch, "\r\n");
  }

  /* flags */
  text_line(ch, "\tcObject Bits / Affections\tn", line_length, '-', '-');
  sprintbitarray(GET_OBJ_WEAR(j), wear_bits, TW_ARRAY_MAX, buf);
  send_to_char(ch, "Can be worn on: %s\r\n", buf);
  sprintbitarray(GET_OBJ_AFFECT(j), affected_bits, AF_ARRAY_MAX, buf);
  send_to_char(ch, "Set char bits : %s\r\n", buf);
  sprintbitarray(GET_OBJ_EXTRA(j), extra_bits, EF_ARRAY_MAX, buf);
  send_to_char(ch, "Extra flags   : %s\r\n", buf);
  /* affections */
  found = FALSE;
  send_to_char(ch, "Affections:");
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (j->affected[i].modifier)
    {
      sprinttype(j->affected[i].location, apply_types, buf, sizeof(buf));
      if (j->affected[i].location == APPLY_FEAT)
      {
        feat_num = j->affected[i].modifier;
        if (feat_num < 0 || feat_num >= NUM_FEATS)
          feat_num = FEAT_UNDEFINED;
        send_to_char(ch, "%s %s:(%d)%s (%s)", found++ ? "," : "", buf, j->affected[i].modifier, feat_list[feat_num].name, bonus_types[j->affected[i].bonus_type]);
      }
      else
        send_to_char(ch, "%s %+d to %s (%s)", found++ ? "," : "", j->affected[i].modifier, buf, bonus_types[j->affected[i].bonus_type]);
    }
  if (!found)
    send_to_char(ch, " None");
  send_to_char(ch, "\r\n");

  /* location info */
  if (mode == ITEM_STAT_MODE_IMMORTAL)
  {
    text_line(ch, "\tcLocation Information\tn", line_length, '-', '-');
    send_to_char(ch, "In room: %d (%s), ", GET_ROOM_VNUM(IN_ROOM(j)),
                 IN_ROOM(j) == NOWHERE ? "Nowhere" : world[IN_ROOM(j)].name);
    /* In order to make it this far, we must already be able to see the character
     * holding the object. Therefore, we do not need CAN_SEE(). */
    send_to_char(ch, "In object: %s, ", j->in_obj ? j->in_obj->short_description : "None");
    send_to_char(ch, "Carried by: %s, ", j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
    send_to_char(ch, "Worn by: %s\r\n", j->worn_by ? GET_NAME(j->worn_by) : "Nobody");
  }

  /* object values, lines drawn over there */
  display_item_object_values(ch, j, mode);

  if (mode == ITEM_STAT_MODE_IMMORTAL)
  {
    /* display contents */
    text_line(ch, "\tcItem Contains:\tn", line_length, '-', '-');
    if (j->contains)
    {
      int column = 0;

      for (found = 0, j2 = j->contains; j2; j2 = j2->next_content)
      {
        column += send_to_char(ch, "%s %s", found++ ? "," : "", j2->short_description);
        if (column >= 79)
        {
          send_to_char(ch, "%s\r\n", j2->next_content ? "," : "");
          found = FALSE;
          column = 0;
        }
      }
    }
  }

  if (mode == ITEM_STAT_MODE_IMMORTAL)
  {
    text_line(ch, "\tcObject Scripts:\tn", line_length, '-', '-');
    /* check the object for a script */
    do_sstat_object(ch, j);
  }
  draw_line(ch, line_length, '-', '-');
}

/*
byte object_saving_throws(int material_type, int type)
{
  switch (type) {
  case SAVING_OBJ_IMPACT:
    switch (material_type) {
    case MATERIAL_GLASS:
      return 20;
    case MATERIAL_CERAMIC:
      return 35;
    case MATERIAL_ORGANIC:
      return 40;
    case MATERIAL_WOOD:
      return 50;
    case MATERIAL_IRON:
    case MATERIAL_LEATHER:
      return 70;
    case MATERIAL_STEEL:
    case MATERIAL_DARKWOOD:
    case MATERIAL_COLD_IRON:
      return 85;
    case MATERIAL_MITHRIL:
    case MATERIAL_STONE:
    case MATERIAL_ALCHEMAL_SILVER:
    case MATERIAL_DRAGONHIDE:
      return 90;
    case MATERIAL_DIAMOND:
    case MATERIAL_ADAMANTINE:
      return 95;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_HEAT:
    switch (material_type) {
    case MATERIAL_WOOL:
      return 15;
    case MATERIAL_PAPER:
      return 20;
    case MATERIAL_COTTON:
    case MATERIAL_SATIN:
    case MATERIAL_SILK:
    case MATERIAL_BURLAP:
    case MATERIAL_VELVET:
    case MATERIAL_WOOD:
      return 25;
    case MATERIAL_ONYX:
    case MATERIAL_CURRENCY:
      return 45;
    case MATERIAL_GLASS:
      return 55;
    case MATERIAL_PLATINUM:
      return 75;
    case MATERIAL_ADAMANTINE:
    case MATERIAL_DIAMOND:
    case MATERIAL_DRAGONHIDE:
      return 85;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_COLD:
    switch (material_type) {
    case MATERIAL_GLASS:
      return 35;
    case MATERIAL_ORGANIC:
    case MATERIAL_CURRENCY:
      return 45;
    case MATERIAL_DRAGONHIDE:
      return 80;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_BREATH:
    switch (material_type) {
    case MATERIAL_DRAGONHIDE:
      return 85;
    default:
      return 50;
      break;
    }
  case SAVING_OBJ_SPELL:
    switch (material_type) {
    default:
      return 50;
      break;
    }
  default:
    log("SYSERR: Invalid object saving throw type.");
    break;
  }
  // Should not get here unless something is wrong.
  return 100;
}
 */

/*
int obj_savingthrow(int material, int type)
{
  int save, rnum;

  save = object_saving_throws(material, type);

  rnum = rand_number(1,100);

  if (rnum < save) {
    return (true);
  }

  return (false);
}
 */

/* function needs to do two things, attacker's weapon could take damage
     and the attackee could take damage to their armor. */
/*
void damage_object(struct char_data *ch, struct char_data *victim) {

  struct obj_data *object = NULL;

  int dnum, rnum, snum, wnum;

  object = GET_EQ(ch, WEAR_WIELD1);

  snum = 90;

  rnum = rand_number(1, 101);

  if (object && GET_OBJ_TYPE(object) == ITEM_WEAPON) {
    if (rnum > snum) {
      if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) &&
          !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
        dnum = dice(1, 3);
        GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) = GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) - dnum;
        if (GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) < 0) {
          TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
          send_to_char(ch, "@CYour %s has broken beyond use!@n\r\n", object->short_description);
          perform_remove(ch, WEAR_WIELD1);
        }
      }
    }
  }

  object = GET_EQ(ch, WEAR_WIELD2);

  snum = 90;

  rnum = rand_number(1, 101);

  if (object && GET_OBJ_TYPE(object) == ITEM_WEAPON) {
    if (rnum > snum) {
      if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) &&
          !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
        dnum = dice(1, 3);
        GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) = GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) - dnum;
        if (GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) < 0) {
          TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
          send_to_char(ch, "@CYour %s has broken beyond use!@n\r\n", object->short_description);
          perform_remove(ch, WEAR_WIELD1);
        }
      }
    }
  }

  snum = GET_DEX(victim);

  object = GET_EQ(victim, WEAR_BODY);

  rnum = rand_number(1, 20);

  if (rand_number(1, 100) < 10) {
    if (object && dice(1, 3) != 3) {
      if (rnum > snum || TRUE) {
        if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) && \
            !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
          dnum = dice(1, 3);
          GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) = GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) - dnum;
          if (GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) < 0) {
            TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
            send_to_char(victim, "Your %s has broken beyond use!\r\n", object->short_description);
            perform_remove(victim, WEAR_BODY);
          }
        }
      }
    }
  }

  rnum = dice(1, 101);
  snum = 90;

  wnum = rand_number(0, NUM_WEARS - 1);
  object = GET_EQ(victim, wnum);
  if (object) {
    if (rnum > snum) {
      if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) && \
        !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
        dnum = dice(1, 3);
        GET_OBJ_VAL(object, VAL_ALL_HEALTH) = GET_OBJ_VAL(object, VAL_ALL_HEALTH) - dnum;
        if (GET_OBJ_VAL(object, VAL_ALL_HEALTH) < 0) {
          TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
          send_to_char(victim, "Your %s has broken beyond use!\r\n", object->short_description);
          perform_remove(victim, wnum);
        }
      }
    }
  }


  object = GET_EQ(victim, WEAR_SHIELD);

  rnum = rand_number(1, 20);

  if (rand_number(1, 100) < 10) {
    if (object) {
      if (rnum > snum || TRUE) {
        if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) && \
            !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
          dnum = dice(1, 3);
          GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) = GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) - dnum;
          if (GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) < 0) {
            TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
            send_to_char(victim, "Your %s has broken beyond use!\r\n", object->short_description);
            perform_remove(victim, WEAR_SHIELD);
          }
        }
      }
    }
  }

  return;
}
 */

/* function to update number of lights in a room */
void check_room_lighting_special(room_rnum room, struct char_data *ch,
                                 struct obj_data *light_source, bool take_out_of_container)
{

  /* this object isn't a potential light source, so ignore */
  if (!ch || !light_source || room == NOWHERE)
    return;

  if (OBJ_FLAGGED(light_source, ITEM_MAGLIGHT) ||
      OBJ_FLAGGED(light_source, ITEM_GLOW))
  {
    if (take_out_of_container)
    {
      world[room].light++;
      //world[room].globe += val2;
    }
    else
    {
      world[room].light--;
      //world[room].globe -= val2;
      if (world[room].light < 0)
        world[room].light = 0;
      //if (world[room].globe < 0)
      //world[room].globe = 0;
    }
  }
}

static void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont)
{
  char buf[MEDIUM_STRING] = {'\0'};

  if (!drop_otrigger(obj, ch))
    return;

  if (!obj) /* object might be extracted by drop_otrigger */
    return;

  if ((GET_OBJ_BOUND_ID(cont) != NOBODY) && (GET_OBJ_BOUND_ID(cont) != GET_IDNUM(ch)))
  {
    if (get_name_by_id(GET_OBJ_BOUND_ID(cont)) != NULL)
    {
      snprintf(buf, sizeof(buf), "$p belongs to %s.  You cannot put anything inside it.\r\n", CAP(get_name_by_id(GET_OBJ_BOUND_ID(cont))));
      act(buf, FALSE, ch, cont, 0, TO_CHAR);
      return;
    }
  }

  if (GET_OBJ_TYPE(cont) == ITEM_AMMO_POUCH && GET_OBJ_TYPE(obj) != ITEM_MISSILE)
  {
    act("You can only put ammo into $P.", FALSE, ch, obj, cont, TO_CHAR);
    return;
  }

  if (GET_OBJ_TYPE(cont) == ITEM_AMMO_POUCH &&
      GET_OBJ_VAL(cont, 0) <= num_obj_in_obj(cont))
  {
    snprintf(buf, sizeof(buf), "You can only fit %d $p into $P.", GET_OBJ_VAL(cont, 0));
    act(buf, FALSE, ch, obj, cont, TO_CHAR);
    return;
  }

  if ((GET_OBJ_VAL(cont, 0) > 0) &&
      (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0)))
    act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
  else if (OBJ_FLAGGED(obj, ITEM_NODROP) && IN_ROOM(cont) != NOWHERE)
    act("You can't get $p out of your hand.", FALSE, ch, obj, NULL, TO_CHAR);
  else
  {
    obj_from_char(obj);
    obj_to_obj(obj, cont);

    act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);

    /* Yes, I realize this is strange until we have auto-equip on rent. -gg */
    if (OBJ_FLAGGED(obj, ITEM_NODROP) && !OBJ_FLAGGED(cont, ITEM_NODROP))
    {
      SET_BIT_AR(GET_OBJ_EXTRA(cont), ITEM_NODROP);
      act("You get a strange feeling as you put $p in $P.", FALSE,
          ch, obj, cont, TO_CHAR);
    }
    else
    {

      //15 is DC
      if (FIGHTING(ch))
        update_pos(FIGHTING(ch));
      if (FIGHTING(ch) && GET_HIT(FIGHTING(ch)) >= 1)
      {
        if (dice(1, 20) + compute_ability(ch, ABILITY_ACROBATICS) <= 15)
        {
          send_to_char(ch, "You fumble putting away the item:  ");
          USE_SWIFT_ACTION(ch);
        }
        else
        {
          send_to_char(ch, "*Acrobatics Success*  ");
        }
      }

      act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
    }

    /* in case you put a light in your container */
    check_room_lighting_special(IN_ROOM(ch), ch, obj, FALSE);
  }
}

/* The following put modes are supported:
     1) put <object> <container>
     2) put all.<object> <container>
     3) put all <container>
   The <container> must be equipped, in inventory or on ground. All objects to be put
   into container must be in inventory. */
ACMD(do_put)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  char arg3[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj = NULL, *next_obj = NULL, *cont = NULL;
  struct char_data *tmp_char = NULL;
  int obj_dotmode = 0, cont_dotmode = 0, found = 0, howmany = 1;
  char *theobj = NULL, *thecont = NULL;

  one_argument(two_arguments(argument, arg1, arg2), arg3); /* three_arguments */

  if (*arg3 && is_number(arg1))
  {
    howmany = atoi(arg1);
    theobj = arg2;
    thecont = arg3;
  }
  else
  {
    theobj = arg1;
    thecont = arg2;
  }
  obj_dotmode = find_all_dots(theobj);
  cont_dotmode = find_all_dots(thecont);

  if (!*theobj)
    send_to_char(ch, "Put what in what?\r\n");
  else if (cont_dotmode != FIND_INDIV)
    send_to_char(ch, "You can only put things into one container at a time.\r\n");
  else if (!*thecont)
  {
    send_to_char(ch, "What do you want to put %s in?\r\n", obj_dotmode == FIND_INDIV ? "it" : "them");
  }
  else
  {
    generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
    if (!cont)
      send_to_char(ch, "You don't see %s %s here.\r\n", AN(thecont), thecont);
    else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER &&
             GET_OBJ_TYPE(cont) != ITEM_AMMO_POUCH)
      act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
    else if (OBJVAL_FLAGGED(cont, CONT_CLOSED) && (GET_LEVEL(ch) < LVL_IMMORT || !PRF_FLAGGED(ch, PRF_NOHASSLE)))
      send_to_char(ch, "You'd better open it first!\r\n");
    else
    {
      if (obj_dotmode == FIND_INDIV)
      { /* put <obj> <container> */
        if (!(obj = get_obj_in_list_vis(ch, theobj, NULL, ch->carrying)))
          send_to_char(ch, "You aren't carrying %s %s.\r\n", AN(theobj), theobj);
        else if (obj == cont && howmany == 1)
          send_to_char(ch, "You attempt to fold it into itself, but fail.\r\n");
        else
        {
          while (obj && howmany)
          {
            if (OBJ_FLAGGED(obj, ITEM_NODROP))
              act("You can't let go of $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
            else
            {
              next_obj = obj->next_content;
              if (obj != cont)
              {
                howmany--;
                perform_put(ch, obj, cont);
              }
            }
            obj = get_obj_in_list_vis(ch, theobj, NULL, next_obj);
          }
        }
      }
      else
      {
        for (obj = ch->carrying; obj; obj = next_obj)
        {
          next_obj = obj->next_content;
          if (OBJ_FLAGGED(obj, ITEM_NODROP))
            act("You can't let go of $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
          else
          {
            if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
                (obj_dotmode == FIND_ALL || isname(theobj, obj->name)))
            {
              found = 1;
              perform_put(ch, obj, cont);
            }
          }
        }
        if (!found)
        {
          if (obj_dotmode == FIND_ALL)
            send_to_char(ch, "You don't seem to have anything to put in it.\r\n");
          else
            send_to_char(ch, "You don't seem to have any %ss.\r\n", theobj);
        }
      }
    }
  }
}

static int can_take_obj(struct char_data *ch, struct obj_data *obj)
{

  if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)))
  {
    act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }

  if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE) &&
      /* request that coins be ignored in this check */
      GET_OBJ_TYPE(obj) != ITEM_MONEY)
  {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    {
      act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
      return (0);
    }
    else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
    {
      act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
      return (0);
    }
  }

  if (OBJ_SAT_IN_BY(obj))
  {
    act("It appears someone is sitting on $p..", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }

  return (1);
}

static void get_check_money(struct char_data *ch, struct obj_data *obj)
{
  int value = GET_OBJ_VAL(obj, 0);

  if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
    return;

  extract_obj(obj);

  increase_gold(ch, value);

  if (value == 1)
    send_to_char(ch, "There was 1 coin.\r\n");
  else
    send_to_char(ch, "There were %d coins.\r\n", value);
}

static void perform_get_from_container(struct char_data *ch, struct obj_data *obj,
                                       struct obj_data *cont, int mode)
{
  bool is_corpse = FALSE, is_clan = FALSE;
  int ct = 0;
  char buf[MAX_INPUT_LENGTH];

  if ((GET_OBJ_BOUND_ID(cont) != NOBODY) && (GET_OBJ_BOUND_ID(cont) != GET_IDNUM(ch)))
  {
    if (get_name_by_id(GET_OBJ_BOUND_ID(cont)) != NULL)
    {
      snprintf(buf, sizeof(buf), "$p belongs to %s.  You cannot get anything out of it.\r\n", CAP(get_name_by_id(GET_OBJ_BOUND_ID(cont))));
      act(buf, FALSE, ch, cont, 0, TO_CHAR);
      return;
    }
  }

  if (!strncmp(cont->name, "corpse ", 7))
    is_corpse = TRUE;
  if (GET_CLAN(ch) != NO_CLAN && GET_CLANRANK(ch) != NO_CLANRANK)
    is_clan = TRUE;

  if (mode == FIND_OBJ_INV || can_take_obj(ch, obj))
  {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) && GET_OBJ_TYPE(obj) != ITEM_MONEY)
      act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
    else if (get_otrigger(obj, ch))
    {
      /* if this is getting money from a coprse, check for clan taxes */
      if (is_corpse && is_clan && GET_OBJ_TYPE(obj) == ITEM_MONEY)
      {
        ct = GET_OBJ_VAL(obj, 0);
      }
      obj_from_obj(obj);
      obj_to_char(obj, ch);

      //15 is DC
      if (FIGHTING(ch))
        update_pos(FIGHTING(ch));
      if (FIGHTING(ch) && GET_HIT(FIGHTING(ch)) >= 1)
      {
        if (dice(1, 20) + compute_ability(ch, ABILITY_ACROBATICS) <= 15)
        {
          send_to_char(ch, "You fumble putting away the item:  ");
          USE_SWIFT_ACTION(ch);
        }
        else
        {
          send_to_char(ch, "*Acrobatics Success*  ");
        }
      }

      act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
      act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
      get_check_money(ch, obj);
      if (ct > 0)
        do_clan_tax_losses(ch, ct);
      //delay for taking items out in combat (and fail tumble check)
    }
  }

  /* in case you get a light from your container */
  check_room_lighting_special(IN_ROOM(ch), ch, obj, TRUE);
}

void get_from_container(struct char_data *ch, struct obj_data *cont,
                        char *arg, int mode, int howmany)
{
  struct obj_data *obj = NULL, *next_obj = NULL;
  int obj_dotmode = 0, found = 0;

  obj_dotmode = find_all_dots(arg);

  if (OBJVAL_FLAGGED(cont, CONT_CLOSED) && (GET_LEVEL(ch) < LVL_IMMORT || !PRF_FLAGGED(ch, PRF_NOHASSLE)))
    act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
  else if (obj_dotmode == FIND_INDIV)
  {
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, cont->contains)))
    {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "There doesn't seem to be %s %s in $p.", AN(arg), arg);
      act(buf, FALSE, ch, cont, 0, TO_CHAR);
    }
    else
    {
      struct obj_data *obj_next;
      while (obj && howmany--)
      {
        obj_next = obj->next_content;
        perform_get_from_container(ch, obj, cont, mode);
        obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
      }
    }
  }
  else
  {
    if (obj_dotmode == FIND_ALLDOT && !*arg)
    {
      send_to_char(ch, "Get all of what?\r\n");
      return;
    }
    for (obj = cont->contains; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
          (obj_dotmode == FIND_ALL || isname(arg, obj->name)))
      {
        found = 1;
        perform_get_from_container(ch, obj, cont, mode);
      }
    }
    if (!found)
    {
      if (obj_dotmode == FIND_ALL)
        act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
      else
      {
        char buf[MAX_STRING_LENGTH];

        snprintf(buf, sizeof(buf), "You can't seem to find any %ss in $p.", arg);
        act(buf, FALSE, ch, cont, 0, TO_CHAR);
      }
    }
  }
}

static int perform_get_from_room(struct char_data *ch, struct obj_data *obj)
{
  if (check_trap(ch, TRAP_TYPE_GET_OBJECT, ch->in_room, obj, 0))
    return 0;

  if (can_take_obj(ch, obj) && get_otrigger(obj, ch))
  {
    obj_from_room(obj);
    obj_to_char(obj, ch);
    act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);
    get_check_money(ch, obj);

    /* this is necessary because of disarm */
    if (FIGHTING(ch))
    {
      USE_MOVE_ACTION(ch);
    }

    return (1);
  }
  return (0);
}

static void get_from_room(struct char_data *ch, char *arg, int howmany)
{
  struct obj_data *obj, *next_obj;
  int dotmode, found = 0;

  dotmode = find_all_dots(arg);

  if (dotmode == FIND_INDIV)
  {
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
      send_to_char(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
    else
    {
      struct obj_data *obj_next;
      while (obj && howmany--)
      {
        obj_next = obj->next_content;
        perform_get_from_room(ch, obj);
        obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
      }
    }
  }
  else
  {
    if (dotmode == FIND_ALLDOT && !*arg)
    {
      send_to_char(ch, "Get all of what?\r\n");
      return;
    }
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
          (dotmode == FIND_ALL || isname(arg, obj->name)))
      {
        found = 1;
        perform_get_from_room(ch, obj);
      }
    }
    if (!found)
    {
      if (dotmode == FIND_ALL)
        send_to_char(ch, "There doesn't seem to be anything here.\r\n");
      else
        send_to_char(ch, "You don't see any %ss here.\r\n", arg);
    }
  }
}

ACMD(do_get)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  char arg3[MAX_INPUT_LENGTH] = {'\0'};

  int cont_dotmode = 0, found = 0, mode = 0;
  struct obj_data *cont = NULL;
  struct char_data *tmp_char = NULL;

  one_argument(two_arguments(argument, arg1, arg2), arg3); /* three_arguments */

  if (!*arg1)
    send_to_char(ch, "Get what?\r\n");
  else if (!*arg2)
    get_from_room(ch, arg1, 1);
  else if (is_number(arg1) && !*arg3)
    get_from_room(ch, arg2, atoi(arg1));
  else
  {
    int amount = 1;
    if (is_number(arg1))
    {
      amount = atoi(arg1);
      strcpy(arg1, arg2); /* strcpy: OK (sizeof: arg1 == arg2) */
      strcpy(arg2, arg3); /* strcpy: OK (sizeof: arg2 == arg3) */
    }
    cont_dotmode = find_all_dots(arg2);
    if (cont_dotmode == FIND_INDIV)
    {
      /* TODO: we want a case for finding light sources even in darkness */
      mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &cont);
      if (!cont)
        send_to_char(ch, "You don't have %s %s.\r\n", AN(arg2), arg2);
      else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER &&
               GET_OBJ_TYPE(cont) != ITEM_AMMO_POUCH)
        act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
      else
        get_from_container(ch, cont, arg1, mode, amount);
    }
    else
    {
      if (cont_dotmode == FIND_ALLDOT && !*arg2)
      {
        send_to_char(ch, "Get from all of what?\r\n");
        return;
      }
      for (cont = ch->carrying; cont; cont = cont->next_content)
        if (CAN_SEE_OBJ(ch, cont) &&
            (cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
        {
          if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER ||
              GET_OBJ_TYPE(cont) == ITEM_AMMO_POUCH)
          {
            found = 1;
            get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount);
          }
          else if (cont_dotmode == FIND_ALLDOT)
          {
            found = 1;
            act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
          }
        }

      for (cont = world[IN_ROOM(ch)].contents; cont; cont = cont->next_content)
        if (CAN_SEE_OBJ(ch, cont) &&
            (cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
        {
          if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER ||
              GET_OBJ_TYPE(cont) == ITEM_AMMO_POUCH)
          {
            get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount);
            found = 1;
          }
          else if (cont_dotmode == FIND_ALLDOT)
          {
            act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
            found = 1;
          }
        }

      if (!found)
      {
        if (cont_dotmode == FIND_ALL)
          send_to_char(ch, "You can't seem to find any containers.\r\n");
        else
          send_to_char(ch, "You can't seem to find any %ss here.\r\n", arg2);
      }
    }
  }
}

static void perform_drop_gold(struct char_data *ch, int amount, byte mode, room_rnum RDR)
{
  struct obj_data *obj;

  if (amount <= 0)
    send_to_char(ch, "Heh heh heh.. we are jolly funny today, eh?\r\n");
  else if (GET_GOLD(ch) < amount)
    send_to_char(ch, "You don't have that many coins!\r\n");
  else
  {
    if (mode != SCMD_JUNK)
    {
      USE_SWIFT_ACTION(ch); /* to prevent coin-bombing */
      obj = create_money(amount);
      if (mode == SCMD_DONATE)
      {
        send_to_char(ch, "You throw some gold into the air where it disappears in a puff of smoke!\r\n");
        act("$n throws some gold into the air where it disappears in a puff of smoke!",
            FALSE, ch, 0, 0, TO_ROOM);
        obj_to_room(obj, RDR);
        act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
      }
      else
      {
        char buf[MAX_STRING_LENGTH];

        if (!drop_wtrigger(obj, ch))
        {
          extract_obj(obj);
          return;
        }

        snprintf(buf, sizeof(buf), "$n drops %s.", money_desc(amount));
        act(buf, TRUE, ch, 0, 0, TO_ROOM);

        send_to_char(ch, "You drop some gold.\r\n");
        obj_to_room(obj, IN_ROOM(ch));
      }
    }
    else
    {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$n drops %s which disappears in a puff of smoke!", money_desc(amount));
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      send_to_char(ch, "You drop some gold which disappears in a puff of smoke!\r\n");
    }
    decrease_gold(ch, amount);
  }
}

#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? "  It vanishes in a puff of smoke!" : "")

static int perform_drop(struct char_data *ch, struct obj_data *obj,
                        byte mode, const char *sname, room_rnum RDR)
{
  char buf[MAX_STRING_LENGTH];
  int value;

  if (!drop_otrigger(obj, ch))
    return 0;

  if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
    return 0;

  if (OBJ_FLAGGED(obj, ITEM_NODROP) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
  {
    snprintf(buf, sizeof(buf), "You can't %s $p, it must be CURSED!", sname);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }

  snprintf(buf, sizeof(buf), "You %s $p.%s", sname, VANISH(mode));
  act(buf, FALSE, ch, obj, 0, TO_CHAR);

  snprintf(buf, sizeof(buf), "$n %ss $p.%s", sname, VANISH(mode));
  act(buf, TRUE, ch, obj, 0, TO_ROOM);

  obj_from_char(obj);

  if ((mode == SCMD_DONATE) && OBJ_FLAGGED(obj, ITEM_NODONATE))
    mode = SCMD_JUNK;

  switch (mode)
  {
  case SCMD_DROP:
    obj_to_room(obj, IN_ROOM(ch));
    return (0);
  case SCMD_DONATE:
    obj_to_room(obj, RDR);
    act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0, TO_ROOM);
    return (0);
  case SCMD_JUNK:
    value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
    extract_obj(obj);
    return (value);
  default:
    log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
    /* SYSERR_DESC: This error comes from perform_drop() and is output when
       * perform_drop() is called with an illegal 'mode' argument. */
    break;
  }

  return (0);
}

ACMD(do_drop)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj = NULL, *next_obj = NULL;
  room_rnum RDR = 0;
  byte mode = SCMD_DROP;
  int dotmode = 0, amount = 0, multi = 0, num_don_rooms = 0;
  const char *sname = NULL;

  switch (subcmd)
  {
  case SCMD_JUNK:
    sname = "junk";
    mode = SCMD_JUNK;
    break;
  case SCMD_DONATE:
    sname = "donate";
    mode = SCMD_DONATE;
    /* fail + double chance for room 1   */
    num_don_rooms = (CONFIG_DON_ROOM_1 != NOWHERE) * 2 +
                    (CONFIG_DON_ROOM_2 != NOWHERE) +
                    (CONFIG_DON_ROOM_3 != NOWHERE) + 1;
    switch (rand_number(0, num_don_rooms))
    {
    case 0:
      mode = SCMD_JUNK;
      break;
    case 1:
    case 2:
      RDR = real_room(CONFIG_DON_ROOM_1);
      break;
    case 3:
      RDR = real_room(CONFIG_DON_ROOM_2);
      break;
    case 4:
      RDR = real_room(CONFIG_DON_ROOM_3);
      break;
    }
    if (RDR == NOWHERE)
    {
      send_to_char(ch, "Sorry, you can't donate anything right now.\r\n");
      return;
    }
    break;
  default:
    sname = "drop";
    break;
  }

  argument = one_argument(argument, arg);

  if (!*arg)
  {
    send_to_char(ch, "What do you want to %s?\r\n", sname);
    return;
  }
  else if (is_number(arg))
  {
    multi = atoi(arg);
    one_argument(argument, arg);
    if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
      perform_drop_gold(ch, multi, mode, RDR);
    else if (multi <= 0)
      send_to_char(ch, "Yeah, that makes sense.\r\n");
    else if (!*arg)
      send_to_char(ch, "What do you want to %s %d of?\r\n", sname, multi);
    else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
    else
    {
      do
      {
        next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
        amount += perform_drop(ch, obj, mode, sname, RDR);
        obj = next_obj;
      } while (obj && --multi);
    }
  }
  else
  {
    dotmode = find_all_dots(arg);

    /* Can't junk or donate all */
    if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE))
    {
      if (subcmd == SCMD_JUNK)
        send_to_char(ch, "Go to the dump if you want to junk EVERYTHING!\r\n");
      else
        send_to_char(ch, "Go do the donation room if you want to donate EVERYTHING!\r\n");
      return;
    }
    if (dotmode == FIND_ALL)
    {
      if (!ch->carrying)
        send_to_char(ch, "You don't seem to be carrying anything.\r\n");
      else
        for (obj = ch->carrying; obj; obj = next_obj)
        {
          next_obj = obj->next_content;
          amount += perform_drop(ch, obj, mode, sname, RDR);
        }
    }
    else if (dotmode == FIND_ALLDOT)
    {
      if (!*arg)
      {
        send_to_char(ch, "What do you want to %s all of?\r\n", sname);
        return;
      }
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
        send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);

      while (obj)
      {
        next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
        amount += perform_drop(ch, obj, mode, sname, RDR);
        obj = next_obj;
      }
    }
    else
    {
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      else
        amount += perform_drop(ch, obj, mode, sname, RDR);
    }
  }

  if (amount && (subcmd == SCMD_JUNK))
  {
    send_to_char(ch, "You have been rewarded by the gods!\r\n");
    act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
    GET_GOLD(ch) += amount;
  }
}

bool perform_give(struct char_data *ch, struct char_data *vict,
                  struct obj_data *obj)
{
  if (!give_otrigger(obj, ch, vict))
    return FALSE;
  if (!receive_mtrigger(vict, ch, obj))
    return FALSE;

  if (OBJ_FLAGGED(obj, ITEM_NODROP) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
  {
    act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
    return FALSE;
  }
  if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict) && GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(vict) < LVL_IMMORT)
  {
    act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
    return FALSE;
  }
  if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict) && GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(vict) < LVL_IMMORT)
  {
    act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
    return FALSE;
  }
  obj_from_char(obj);
  obj_to_char(obj, vict);
  act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
  act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
  act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);

  autoquest_trigger_check(ch, vict, obj, AQ_OBJ_RETURN);
  return TRUE;
}

/* utility function for give */
static struct char_data *give_find_vict(struct char_data *ch, char *arg)
{
  struct char_data *vict;

  skip_spaces(&arg);
  if (!*arg)
    send_to_char(ch, "To who?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (vict == ch)
    send_to_char(ch, "What's the point of that?\r\n");
  else
    return (vict);

  return (NULL);
}

static void perform_give_gold(struct char_data *ch, struct char_data *vict,
                              int amount)
{
  char buf[MAX_STRING_LENGTH];

  if (amount <= 0)
  {
    send_to_char(ch, "Heh heh heh ... we are jolly funny today, eh?\r\n");
    return;
  }
  if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_STAFF)))
  {
    send_to_char(ch, "You don't have that many coins!\r\n");
    return;
  }
  send_to_char(ch, "%s", CONFIG_OK);

  snprintf(buf, sizeof(buf), "$n gives you %d gold coin%s.", amount, amount == 1 ? "" : "s");
  act(buf, FALSE, ch, 0, vict, TO_VICT);

  snprintf(buf, sizeof(buf), "$n gives %s to $N.", money_desc(amount));
  act(buf, TRUE, ch, 0, vict, TO_NOTVICT);

  if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_STAFF))
    decrease_gold(ch, amount);

  increase_gold(vict, amount);
  bribe_mtrigger(vict, ch, amount);
}

ACMD(do_give)
{
  char arg[MAX_STRING_LENGTH] = {'\0'};
  int amount = 0, dotmode = 0;
  struct char_data *vict = NULL;
  struct obj_data *obj = NULL, *next_obj = NULL;

  if (!ch)
    return;

  argument = one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Give what to who?\r\n");

  else if (is_number(arg))
  {
    /* ok we received a number value */
    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
    {
      one_argument(argument, arg);
      if ((vict = give_find_vict(ch, arg)) != NULL)
      {
        perform_give_gold(ch, vict, amount);
        quest_give(ch, vict);
      }
      return;
    }
    else if (!*arg) /* Give multiple code. */
      send_to_char(ch, "What do you want to give %d of?\r\n", amount);
    else if (!(vict = give_find_vict(ch, argument)))
      return;
    else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
    else
    {
      while (obj && amount--)
      {
        next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
        perform_give(ch, vict, obj);
        obj = next_obj;
      }
    }
  }
  else
  {
    char buf1[MAX_INPUT_LENGTH];

    one_argument(argument, buf1);
    if (!(vict = give_find_vict(ch, buf1)))
      return;
    dotmode = find_all_dots(arg);
    if (dotmode == FIND_INDIV)
    {
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      else
        perform_give(ch, vict, obj);
    }
    else
    {
      if (dotmode == FIND_ALLDOT && !*arg)
      {
        send_to_char(ch, "All of what?\r\n");
        return;
      }
      if (!ch->carrying)
        send_to_char(ch, "You don't seem to be holding anything.\r\n");
      else
        for (obj = ch->carrying; obj; obj = next_obj)
        {
          next_obj = obj->next_content;
          if (CAN_SEE_OBJ(ch, obj) &&
              ((dotmode == FIND_ALL || isname(arg, obj->name))))
            perform_give(ch, vict, obj);
        }
    }
  }

  if (ch && vict)
  {
    quest_give(ch, vict);
    save_char(ch, 0);
    save_char(vict, 0);
  }
}

void weight_change_object(struct obj_data *obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (IN_ROOM(obj) != NOWHERE)
  {
    GET_OBJ_WEIGHT(obj) += weight;
  }
  else if ((tmp_ch = obj->carried_by))
  {
    obj_from_char(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_char(obj, tmp_ch);
  }
  else if ((tmp_obj = obj->in_obj))
  {
    obj_from_obj(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_obj(obj, tmp_obj);
  }
  else
  {
    log("SYSERR: Unknown attempt to subtract weight from an object.");
    /* SYSERR_DESC: weight_change_object() outputs this error when weight is
     * attempted to be removed from an object that is not carried or in
     * another object. */
  }
}

void name_from_drinkcon(struct obj_data *obj)
{
  char *new_name, *cur_name, *next;
  const char *liqname;
  int liqlen, cpylen;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
    return;

  liqname = drinknames[GET_OBJ_VAL(obj, 2)];
  if (!isname(liqname, obj->name))
  {
    log("SYSERR: Can't remove liquid '%s' from '%s' (%d) item.", liqname, obj->name, obj->item_number);
    /* SYSERR_DESC: From name_from_drinkcon(), this error comes about if the
     * object noted (by keywords and item vnum) does not contain the liquid
     * string being searched for. */
    return;
  }

  liqlen = strlen(liqname);
  CREATE(new_name, char, strlen(obj->name) - strlen(liqname)); /* +1 for NUL, -1 for space */

  for (cur_name = obj->name; cur_name; cur_name = next)
  {
    if (*cur_name == ' ')
      cur_name++;

    if ((next = strchr(cur_name, ' ')))
      cpylen = next - cur_name;
    else
      cpylen = strlen(cur_name);

    if (!strn_cmp(cur_name, liqname, liqlen))
      continue;

    if (*new_name)
      strcat(new_name, " ");             /* strcat: OK (size precalculated) */
    strncat(new_name, cur_name, cpylen); /* strncat: OK (size precalculated) */
  }

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);
  obj->name = new_name;
}

void name_to_drinkcon(struct obj_data *obj, int type)
{
  char *new_name;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
    return;

  CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
  sprintf(new_name, "%s %s", obj->name, drinknames[type]); /* sprintf: OK */

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);

  obj->name = new_name;
}

ACMD(do_drink)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *temp;
  // struct affected_type af;
  int amount, weight;
  int on_ground = 0;
  char buf[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (IS_NPC(ch)) /* Cannot use GET_COND() on mobs. */
    return;

  if (!*arg)
  {
    char buf[MAX_STRING_LENGTH];
    switch (SECT(IN_ROOM(ch)))
    {
    case SECT_WATER_SWIM:
    case SECT_WATER_NOSWIM:
    case SECT_UD_WATER:
    case SECT_UD_NOSWIM:
    case SECT_UNDERWATER:
      if ((GET_COND(ch, HUNGER) > 20) && (GET_COND(ch, THIRST) > 0))
      {
        send_to_char(ch, "Your stomach can't contain anymore!\r\n");
      }
      snprintf(buf, sizeof(buf), "$n takes a refreshing drink.");
      act(buf, TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You take a refreshing drink.\r\n");
      gain_condition(ch, THIRST, 1);
      if (GET_COND(ch, THIRST) > 20)
        send_to_char(ch, "You don't feel thirsty any more.\r\n");
      return;
    default:
      send_to_char(ch, "Drink from what?\r\n");
      return;
    }
  }

  if (!(temp = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
  {
    if (!(temp = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
    {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    else
      on_ground = 1;
  }

  if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
      (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN))
  {
    send_to_char(ch, "You can't drink from that!\r\n");
    return;
  }

  if (GET_OBJ_BOUND_ID(temp) != NOBODY)
  {
    if (GET_OBJ_BOUND_ID(temp) != GET_IDNUM(ch))
    {
      if (get_name_by_id(GET_OBJ_BOUND_ID(temp)) == NULL)
        snprintf(buf, sizeof(buf), "$p%s belongs to someone else.  You can't drink from it.", CCNRM(ch, C_NRM));
      else
        snprintf(buf, sizeof(buf), "$p%s belongs to %s.  You can't drink from it.", CCNRM(ch, C_NRM), CAP(get_name_by_id(GET_OBJ_BOUND_ID(temp))));

      act(buf, FALSE, ch, temp, 0, TO_CHAR);
    }
  }

  /*if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
    send_to_char(ch, "You have to be holding that to drink from it.\r\n");
    return;
  }*/

  if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0))
  {
    /* The pig is drunk */
    send_to_char(ch, "You can't seem to get close enough to your mouth.\r\n");
    act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  if ((GET_COND(ch, HUNGER) > 22) && (GET_COND(ch, THIRST) > 4))
  {
    send_to_char(ch, "Your stomach can't contain anymore!\r\n");
    return;
  }

  if (GET_OBJ_VAL(temp, 1) == 0)
  {
    send_to_char(ch, "It is empty.\r\n");
    return;
  }

  /*
  if (!(GET_OBJ_VAL(temp, 0) == 1)) {
    send_to_char(ch, "It is empty.\r\n");
    return;
  }
   */

  if (GET_COND(ch, THIRST) > 20)
  {
    send_to_char(ch, "You are not thirsty.\r\n");
    return;
  }

  if (GET_OBJ_VAL(temp, 3) != 0 && char_has_mud_event(ch, eMAGIC_FOOD))
  {
    send_to_char(ch, "You cannot drink any more magical liquids right now.\r\n");
    return;
  }

  if (!consume_otrigger(temp, ch, OCMD_DRINK)) /* check trigger */
    return;

  if (subcmd == SCMD_DRINK)
  {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp, 2)]);
    act(buf, TRUE, ch, temp, 0, TO_ROOM);

    send_to_char(ch, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);

    if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
      amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
    else
      amount = rand_number(3, 10);
  }
  else
  {
    act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
    send_to_char(ch, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
    amount = 1;
  }

  amount = MIN(amount, GET_OBJ_VAL(temp, 1));

  /* You can't subtract more than the object weighs, unless its unlimited. */
  if (GET_OBJ_VAL(temp, 0) > 0)
  {
    weight = MIN(amount, GET_OBJ_WEIGHT(temp));
    weight_change_object(temp, -weight); /* Subtract amount */
  }

  gain_condition(ch, DRUNK, drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount / 4);
  gain_condition(ch, HUNGER, drink_aff[GET_OBJ_VAL(temp, 2)][HUNGER] * amount / 4);
  gain_condition(ch, THIRST, drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount / 4);

  if (GET_COND(ch, DRUNK) > 10)
    send_to_char(ch, "You feel drunk.\r\n");

  if (GET_COND(ch, THIRST) > 20)
    send_to_char(ch, "You don't feel thirsty any more.\r\n");

  if (GET_COND(ch, HUNGER) > 20)
    send_to_char(ch, "You are full.\r\n");

  if (GET_OBJ_VAL(temp, 1) != 0)
  {
    // this drink has a spell attached to it
    // call the spell, ch as target
    call_magic(ch, ch, NULL, GET_OBJ_VAL(temp, 3), 0, GET_LEVEL(ch), CAST_FOOD_DRINK);
    /* attach event to character to prevent over-eating magical food/drink */
    if (GET_LEVEL(ch) < LVL_IMMORT || !PRF_FLAGGED(ch, PRF_NOHASSLE))
      attach_mud_event(new_mud_event(eMAGIC_FOOD, ch, NULL), 3000);
  }

  /* removed poison value from drink containers, replaced with spellnum
   * so now if you want a poisoned drink container, just use poison spell for
   * value 3 -Nashak
   * 
  if (GET_OBJ_VAL(temp, 3) && GET_LEVEL(ch) < LVL_IMMORT) { // The crap was poisoned !
    send_to_char(ch, "Oops, it tasted rather strange!\r\n");
    act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);

    new_affect(&af);
    af.spell = SPELL_POISON;
    af.duration = amount * 3;
    SET_BIT_AR(af.bitvector, AFF_POISON);
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  } */

  /* Empty the container (unless unlimited), and no longer poison. */
  if (GET_OBJ_VAL(temp, 0) > 0)
  {
    GET_OBJ_VAL(temp, 1) -= amount;
    if (!GET_OBJ_VAL(temp, 1))
    { /* The last bit */
      name_from_drinkcon(temp);
      GET_OBJ_VAL(temp, 2) = 0;
      GET_OBJ_VAL(temp, 3) = 0;
    }
  }

  /* Use a move action, but regen in half a round. */
  start_action_cooldown(ch, atMOVE, 3 RL_SEC);

  return;
}

ACMD(do_eat)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *food;
  struct affected_type af;
  int amount;
  char buf[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (IS_NPC(ch)) /* Cannot use GET_COND() on mobs. */
    return;

  if (!*arg)
  {
    send_to_char(ch, "Eat what?\r\n");
    return;
  }
  if (!(food = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
  {
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    return;
  }
  if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
                               (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN)))
  {
    do_drink(ch, argument, 0, SCMD_SIP);
    return;
  }
  if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && (GET_LEVEL(ch) < LVL_IMMORT))
  {
    send_to_char(ch, "You can't eat THAT!\r\n");
    return;
  }
  if (GET_OBJ_BOUND_ID(food) != NOBODY)
  {
    if (GET_OBJ_BOUND_ID(food) != GET_IDNUM(ch))
    {
      if (get_name_by_id(GET_OBJ_BOUND_ID(food)) == NULL)
        snprintf(buf, sizeof(buf), "$p%s belongs to someone else.  You can't eat it.", CCNRM(ch, C_NRM));
      else
        snprintf(buf, sizeof(buf), "$p%s belongs to %s.  You can't eat it.", CCNRM(ch, C_NRM), CAP(get_name_by_id(GET_OBJ_BOUND_ID(food))));

      act(buf, FALSE, ch, food, 0, TO_CHAR);
    }
  }
  if (GET_COND(ch, HUNGER) > 20)
  { /* Stomach full */
    send_to_char(ch, "You are too full to eat more!\r\n");
    return;
  }
  if (GET_OBJ_VAL(food, 1) != 0 && char_has_mud_event(ch, eMAGIC_FOOD))
  {
    send_to_char(ch, "You cannot eat any more magical food right now.\r\n");
    return;
  }
  if (!consume_otrigger(food, ch, OCMD_EAT)) /* check trigger */
    return;

  if (subcmd == SCMD_EAT)
  {
    act("You eat $p.", FALSE, ch, food, 0, TO_CHAR);
    act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
  }
  else
  {
    act("You nibble a little bit of $p.", FALSE, ch, food, 0, TO_CHAR);
    act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
  }

  amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

  gain_condition(ch, HUNGER, amount);

  if (GET_COND(ch, HUNGER) > 20)
    send_to_char(ch, "You are full.\r\n");

  if (GET_OBJ_TYPE(food) == ITEM_FOOD && GET_OBJ_VAL(food, 1) != 0)
  {
    // this food has a spell attached to it
    // call the spell, ch as target
    call_magic(ch, ch, NULL, GET_OBJ_VAL(food, 1), 0, GET_LEVEL(ch), CAST_FOOD_DRINK);
    /* attach event to character to prevent over-eating magical food/drink */
    if (GET_LEVEL(ch) < LVL_IMMORT || !PRF_FLAGGED(ch, PRF_NOHASSLE))
      attach_mud_event(new_mud_event(eMAGIC_FOOD, ch, NULL), 3000);
  }
  if (GET_OBJ_VAL(food, 3) && (GET_LEVEL(ch) < LVL_IMMORT))
  {
    /* The crap was poisoned ! */
    send_to_char(ch, "Oops, that tasted rather strange!\r\n");
    act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);

    new_affect(&af);
    af.spell = SPELL_POISON;
    af.duration = amount * 2;
    SET_BIT_AR(af.bitvector, AFF_POISON);
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }
  if (subcmd == SCMD_EAT)
    extract_obj(food);
  else
  {
    if (!(--GET_OBJ_VAL(food, 0)))
    {
      send_to_char(ch, "There's nothing left now.\r\n");
      extract_obj(food);
    }
  }
}

ACMD(do_pour)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj = NULL, *to_obj = NULL;
  int amount = 0;

  two_arguments(argument, arg1, arg2);

  if (subcmd == SCMD_POUR)
  {
    if (!*arg1)
    { /* No arguments */
      send_to_char(ch, "From what do you want to pour?\r\n");
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
    {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON)
    {
      send_to_char(ch, "You can't pour from that!\r\n");
      return;
    }
  }
  if (subcmd == SCMD_FILL)
  {
    if (!*arg1)
    { /* no arguments */
      send_to_char(ch, "What do you want to fill?  And what are you filling it from?\r\n");
      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
    {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON)
    {
      act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!*arg2)
    { /* no 2nd argument */
      act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg2, NULL, world[IN_ROOM(ch)].contents)))
    {
      send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN)
    {
      act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
      return;
    }
  }
  if (GET_OBJ_VAL(from_obj, 1) == 0)
  {
    act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR)
  { /* pour */
    if (!*arg2)
    {
      send_to_char(ch, "Where do you want it?  Out or in what?\r\n");
      return;
    }
    if (!str_cmp(arg2, "out"))
    {
      if (GET_OBJ_VAL(from_obj, 0) > 0)
      {
        act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
        act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

        weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1)); /* Empty */

        name_from_drinkcon(from_obj);
        GET_OBJ_VAL(from_obj, 1) = 0;
        GET_OBJ_VAL(from_obj, 2) = 0;
        GET_OBJ_VAL(from_obj, 3) = 0;
      }
      else
        send_to_char(ch, "You can't possibly pour that container out!\r\n");

      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying)))
    {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
        (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN))
    {
      send_to_char(ch, "You can't pour anything into that.\r\n");
      return;
    }
  }
  if (to_obj == from_obj)
  {
    send_to_char(ch, "A most unproductive effort.\r\n");
    return;
  }
  if ((GET_OBJ_VAL(to_obj, 0) < 0) ||
      (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0))))
  {
    send_to_char(ch, "There is already another liquid in it!\r\n");
    return;
  }
  if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0)))
  {
    send_to_char(ch, "There is no room for more.\r\n");
    return;
  }
  if (subcmd == SCMD_POUR)
    send_to_char(ch, "You pour the %s into the %s.", drinks[GET_OBJ_VAL(from_obj, 2)], arg2);

  if (subcmd == SCMD_FILL)
  {
    act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
    act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
  }
  /* New alias */
  if (GET_OBJ_VAL(to_obj, 1) == 0)
    name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

  /* First same type liq. */
  GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

  /* Then how much to pour */
  if (GET_OBJ_VAL(from_obj, 0) > 0)
  {
    GET_OBJ_VAL(from_obj, 1) -= (amount =
                                     (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));

    GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

    if (GET_OBJ_VAL(from_obj, 1) < 0)
    { /* There was too little */
      GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
      amount += GET_OBJ_VAL(from_obj, 1);
      name_from_drinkcon(from_obj);
      GET_OBJ_VAL(from_obj, 1) = 0;
      GET_OBJ_VAL(from_obj, 2) = 0;
      GET_OBJ_VAL(from_obj, 3) = 0;
    }
  }
  else
  {
    GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);
    amount = GET_OBJ_VAL(to_obj, 0);
  }
  /* Poisoned? */
  GET_OBJ_VAL(to_obj, 3) = (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));
  /* Weight change, except for unlimited. */
  if (GET_OBJ_VAL(from_obj, 0) > 0)
  {
    weight_change_object(from_obj, -amount);
  }
  weight_change_object(to_obj, amount); /* Add weight */
}

static void wear_message(struct char_data *ch, struct obj_data *obj, int where)
{
  const char * const wear_messages[][2] = {
      {"$n lights $p and holds it.",
       "You light $p and hold it."},

      {"$n slides $p on to $s right ring finger.",
       "You slide $p on to your right ring finger."},

      {"$n slides $p on to $s left ring finger.",
       "You slide $p on to your left ring finger."},

      {"$n wears $p around $s neck.",
       "You wear $p around your neck."},

      {"$n wears $p around $s neck.",
       "You wear $p around your neck."},

      {"$n wears $p on $s body.",
       "You wear $p on your body."},

      {"$n wears $p on $s head.",
       "You wear $p on your head."},

      {"$n puts $p on $s legs.",
       "You put $p on your legs."},

      {"$n wears $p on $s feet.",
       "You wear $p on your feet."},

      {"$n puts $p on $s hands.",
       "You put $p on your hands."},

      {"$n wears $p on $s arms.",
       "You wear $p on your arms."},

      {"$n straps $p around $s arm as a shield.",
       "You start to use $p as a shield."},

      {"$n wears $p about $s body.",
       "You wear $p around your body."},

      {"$n wears $p around $s waist.",
       "You wear $p around your waist."},

      {"$n puts $p on around $s right wrist.",
       "You put $p on around your right wrist."},

      {"$n puts $p on around $s left wrist.",
       "You put $p on around your left wrist."},

      {"$n wields $p.",
       "You wield $p."},

      {"$n grabs $p.",
       "You grab $p."},

      {"$n wields $p.",
       "You wield $p."},

      {"$n grabs $p.",
       "You grab $p."},

      {"$n wields $p with two hands.",
       "You wield $p with two hands."},

      {"$n holds $p with two hands.",
       "You hold $p with two hands."},

      {"$n places $p on $s face.",
       "You place $p on your face."},

      {"$n straps $p on $s back.",
       "You strap $p on your back."},

      {"$n attaches $p to $s ear.",
       "You attach $p to your ear."},

      {"$n attaches $p to $s ear.",
       "You attach $p to your ear."},

      {"$n covers $s eye(s) with $p.",
       "You cover your eye(s) with $p."},

      {"$n wears $p as a badge.",
       "You wear $p as a badge."},

  };

  /* extinguished light! */
  if (where == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT &&
      GET_OBJ_VAL(obj, 2) == 0)
  {
    act("$n holds $p.", TRUE, ch, obj, 0, TO_ROOM);
    act("You hold $p.", FALSE, ch, obj, 0, TO_CHAR);
  }
  else
  {
    act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
    act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
  }
}

int hands_used(struct char_data *ch)
{
  int num = 0;
  if (GET_EQ(ch, WEAR_WIELD_1))
    num++;
  if (is_two_handed_ranged_weapon(GET_EQ(ch, WEAR_WIELD_1)))
    num++; // bows and crossbows and some others will always need 2 hands regardless of size.  Also checks if obj exists
  if (GET_EQ(ch, WEAR_WIELD_OFFHAND))
    num++;
  if (is_two_handed_ranged_weapon(GET_EQ(ch, WEAR_WIELD_OFFHAND)))
    num++; // bows and crossbows and some others will always need 2 hands regardless of size.  Also checks if obj exists
  if (GET_EQ(ch, WEAR_HOLD_1))
    num++;
  if (GET_EQ(ch, WEAR_HOLD_2))
    num++;
  if (GET_EQ(ch, WEAR_SHIELD))
    num++;
  if (GET_EQ(ch, WEAR_WIELD_2H))
    num += 2;
  if (GET_EQ(ch, WEAR_HOLD_2H))
    num += 2;
  return (num);
}

static int hands_have(struct char_data *ch)
{
  int num = 2;
  switch (GET_RACE(ch))
  {
  default:
    num = 2;
    break;
  }

  if (KNOWS_DISCOVERY(ch, ALC_DISC_VESTIGIAL_ARM))
    num++;

  //if (GET_LEVEL(ch) >= LVL_IMPL)
  //num = 4;
  return (num);
}

int hands_available(struct char_data *ch)
{
  //do our error checking here
  if (hands_used(ch) > hands_have(ch))
  {
    log("SYSERR: perform_remove: hands_available problem");
    return -1;
  }
  return (hands_have(ch) - hands_used(ch));
}

/* when handling wielding, holding, wearing of shields, we have to know
   how may hands you have available to equipping these items.  Also some
   held and wielded items will take two hands based on their size.  */
//  should only return 1 or 2, -1 if problematic

static int hands_needed(struct char_data *ch, struct obj_data *obj)
{
  /* example:  huge sword,   medium human, size =  2
   *           large sword,  medium human, size =  1
   *           medium sword, medium human, size =  0
   *           small sword,  medium human, size = -1
   *           tiny dagger,  medium human, size = -2 */
  int size = GET_OBJ_SIZE(obj) - GET_SIZE(ch);

  //size check
  if (size == 1) // two handed
    return 2;
  if (size >= 2) /* not enough hands!! */
    return -1;

  return 1; /* items is equal or smaller than char size, easily used with 1 hand */
}

int is_wielding_type(struct char_data *ch)
{

  if (GET_EQ(ch, WEAR_WIELD_1))
    return GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD_1));

  if (GET_EQ(ch, WEAR_WIELD_OFFHAND))
    return GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD_OFFHAND));

  if (GET_EQ(ch, WEAR_WIELD_2H))
    return GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD_2H));

  return -1;
}

/* the guts of the 'wear' mechanic for equipping gear */
void perform_wear(struct char_data *ch, struct obj_data *obj, int where)
{
  char buf[MAX_INPUT_LENGTH];

  /* TAKE is used for objects that don't require special bits, ex. HOLD */
  int wear_bitvectors[] = {
      ITEM_WEAR_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
      ITEM_WEAR_NECK, ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS,
      ITEM_WEAR_FEET, ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD,
      ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WEAR_WRIST,
      ITEM_WEAR_WIELD, ITEM_WEAR_TAKE, ITEM_WEAR_WIELD, ITEM_WEAR_TAKE,
      ITEM_WEAR_WIELD, ITEM_WEAR_TAKE, ITEM_WEAR_FACE, ITEM_WEAR_AMMO_POUCH,
      ITEM_WEAR_EAR, ITEM_WEAR_EYES, ITEM_WEAR_BADGE};

  const char * const already_wearing[NUM_WEARS] = {
      "You're already using a light.\r\n",                                  //0
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",             //1
      "You're already wearing something on both of your ring fingers.\r\n", //2
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",             //3
      "You can't wear anything else around your neck.\r\n",                 //4
      "You're already wearing something on your body.\r\n",                 //5
      "You're already wearing something on your head.\r\n",                 //6
      "You're already wearing something on your legs.\r\n",                 //7
      "You're already wearing something on your feet.\r\n",                 //8
      "You're already wearing something on your hands.\r\n",                //9
      "You're already wearing something on your arms.\r\n",                 //10
      "Your hands are full.\r\n",                                           //11
      "You're already wearing something about your body.\r\n",              //12
      "You already have something around your waist.\r\n",                  //13
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",             //14
      "You're already wearing something around both of your wrists.\r\n",   //15
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",             //16
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",             //17
      "Your hands are full.\r\n",                                           //18
      "Your hands are full.\r\n",                                           //19
      "Your hands are full.\r\n",                                           //20
      "Your hands are full.\r\n",                                           //21
      "You are already wearing something on your face.\r\n",
      "You are already wearing an ammo pouch.\r\n",
      "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
      "You are already wearing an item on each ear.\r\n", //25
      "You are already wearing something on your eyes.\r\n",
      "You are already wearing a badge.\r\n",
  };

  /* we are looking for some quick exits */
  if (IS_ANIMAL(ch))
  {
    send_to_char(ch, "You are animal, how you going to wear that?\r\n");
    return;
  }

  /* see hands_needed() for notes */
  int handsNeeded = hands_needed(ch, obj);
  if (handsNeeded == -1)
  {
    send_to_char(ch, "There is no way this item will fit you!\r\n");
    return;
  }

  if (OBJ_FLAGGED(obj, ITEM_MOLD))
  {
    send_to_char(ch, "You can't wear an object mold!\r\n");
    return;
  }

  /* check to make sure you don't mix melee/ranged */
  if (where == WEAR_WIELD_1 && is_wielding_type(ch) != -1)
  {
    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON &&
        is_wielding_type(ch) != ITEM_WEAPON)
    {
      send_to_char(ch, "You can't mix-and-match ranged/melee weapons.\r\n");
      return;
    }
    if (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON &&
        is_wielding_type(ch) != ITEM_FIREWEAPON)
    {
      send_to_char(ch, "You can't mix-and-match ranged/melee weapons.\r\n");
      return;
    }
  }

  /* first, make sure that the wear position is valid. */
  if (!CAN_WEAR(obj, wear_bitvectors[where]))
  {
    act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  if (GET_OBJ_BOUND_ID(obj) != NOBODY)
  {
    if (GET_OBJ_BOUND_ID(obj) != GET_IDNUM(ch))
    {
      if (get_name_by_id(GET_OBJ_BOUND_ID(obj)) == NULL)
        snprintf(buf, sizeof(buf), "$p%s belongs to someone else.  You can't use it.", CCNRM(ch, C_NRM));
      else
        snprintf(buf, sizeof(buf), "$p%s belongs to %s.  You can't use it.", CCNRM(ch, C_NRM), CAP(get_name_by_id(GET_OBJ_BOUND_ID(obj))));

      act(buf, FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
  }

  if (GET_RACE(ch) == RACE_TRELUX && (where == WEAR_FINGER_R ||
                                      where == WEAR_FINGER_L || where == WEAR_HEAD ||
                                      where == WEAR_LEGS || where == WEAR_FEET || where == WEAR_HANDS))
  {
    send_to_char(ch, "Your anatomy does not allow you to wear that...\r\n");
    return;
  }

  // size for gear, not in-hands
  if (where != WEAR_WIELD_1 && where != WEAR_WIELD_OFFHAND &&
      where != WEAR_HOLD_1 && where != WEAR_HOLD_2 &&
      where != WEAR_SHIELD && where != WEAR_WIELD_2H &&
      where != WEAR_HOLD_2H && where != WEAR_LIGHT &&
      where != WEAR_NECK_1 && where != WEAR_NECK_2 &&
      where != WEAR_WAIST && where != WEAR_WRIST_R &&
      where != WEAR_WRIST_L && where != WEAR_AMMO_POUCH &&
      where != WEAR_FINGER_R && where != WEAR_FINGER_L &&
      where != WEAR_EAR_R && where != WEAR_EAR_L &&
      where != WEAR_EYES && where != WEAR_BADGE)
  {
    if (GET_OBJ_SIZE(obj) < GET_SIZE(ch))
    {
      send_to_char(ch, "This item is too small for you (HELP RESIZE).\r\n");
      return;
    }
    if (GET_OBJ_SIZE(obj) > GET_SIZE(ch))
    {
      send_to_char(ch, "This item is too large for you (HELP RESIZE).\r\n");
      return;
    }
  }

  // code for gear with 2 possible slots, and next to each other in array
  if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) ||
      (where == WEAR_WRIST_R) || (where == WEAR_EAR_R))
    if (GET_EQ(ch, where))
      where++;

  //juggling with hands code -zusuk
  if (where == WEAR_WIELD_1 || where == WEAR_WIELD_OFFHAND ||
      where == WEAR_HOLD_1 || where == WEAR_HOLD_2 ||
      where == WEAR_SHIELD || where == WEAR_WIELD_2H ||
      where == WEAR_HOLD_2H)
  {
    if (GET_RACE(ch) == RACE_TRELUX)
    {
      send_to_char(ch, "You have no hands!\r\n");
      return;
    }

    if (handsNeeded == 2 && where == WEAR_WIELD_1)
      where = WEAR_WIELD_2H;
    if (handsNeeded == 2 && where == WEAR_HOLD_1)
      where = WEAR_HOLD_2H;

    //first check if you have any hands available
    if (handsNeeded > hands_available(ch))
    {
      send_to_char(ch, "You would need an extra hand to do that.\r\n");
      return;
    }

    //next throw the item in the first available slot
    // is the item in one of the primary slots?
    if ((where == WEAR_WIELD_1) || (where == WEAR_HOLD_1))
      if (GET_EQ(ch, where))
        where += 2;
  }
  //end juggling hands code

  if (GET_EQ(ch, where))
  {
    send_to_char(ch, "%s", already_wearing[where]);
    return;
  }

  /* See if a trigger disallows it */
  if (!wear_otrigger(obj, ch, where) || (obj->carried_by != ch))
    return;

  wear_message(ch, obj, where);
  obj_from_char(obj);
  equip_char(ch, obj, where);
}

/* given character and object:
   if argument is empty, check where this object is "wear"able
   if argument, match argument with keyword list to determine "wear" spot
   returns:  where (which position), 0-value means can't find where  */
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg)
{
  int where = -1;

  /* this is lined up with equipment_types, it SHOULD be lined
   up with wear_bits (constants.c) probably, but not changing the status quo */
  const char * const keywords[NUM_WEARS + 1] = {
      "!RESERVED!", //0 (takeable)
      "finger",
      "!RESERVED!", // (2nd finger)
      "neck",
      "!RESERVED!", // (2nd neck)
      "body",       //5
      "head",
      "legs",
      "feet",
      "hands",
      "arms", //10
      "shield",
      "about",
      "waist",
      "wrist",
      "!RESERVED!", //15 (2nd wrist)
      "!RESERVED!", // (wielded)
      "!RESERVED!", // (held)
      "!RESERVED!", // (wielded offhand)
      "!RESERVED!", // (held offhand)
      "!RESERVED!", //20 (wielded twohanded)
      "!RESERVED!", // (held twohanded)
      "face",
      "ammo-pouch",
      "ear",
      "!RESERVED!", //25 (2nd ear)
      "eyes",
      "badge",
      "\n"};

  if (!arg || !*arg)
  {
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
      where = WEAR_FINGER_R;
    if (CAN_WEAR(obj, ITEM_WEAR_NECK))
      where = WEAR_NECK_1;
    if (CAN_WEAR(obj, ITEM_WEAR_BODY))
      where = WEAR_BODY;
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
      where = WEAR_HEAD;
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
      where = WEAR_LEGS;
    if (CAN_WEAR(obj, ITEM_WEAR_FEET))
      where = WEAR_FEET;
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
      where = WEAR_HANDS;
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
      where = WEAR_ARMS;
    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
      where = WEAR_SHIELD;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
      where = WEAR_ABOUT;
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
      where = WEAR_WAIST;
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
      where = WEAR_WRIST_R;
    if (CAN_WEAR(obj, ITEM_WEAR_FACE))
      where = WEAR_FACE;
    if (CAN_WEAR(obj, ITEM_WEAR_AMMO_POUCH))
      where = WEAR_AMMO_POUCH;
    if (CAN_WEAR(obj, ITEM_WEAR_EAR))
      where = WEAR_EAR_R;
    if (CAN_WEAR(obj, ITEM_WEAR_EYES))
      where = WEAR_EYES;
    if (CAN_WEAR(obj, ITEM_WEAR_BADGE))
      where = WEAR_BADGE;

    /* this means we have an argument, does it match our keywords-array ?*/
  }
  else if ((where = search_block(arg, keywords, FALSE)) < 0)
  {
    send_to_char(ch, "'%s'?  What part of your body is THAT?\r\n", arg);
  }

  return (where);
}

ACMD(do_wear)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj = NULL, *next_obj = NULL;
  int where = 0, dotmode = 0, items_worn = 0;

  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
  {
    send_to_char(ch, "Why would you want to wear something? (wildshape/polymorph)\r\n");
    return;
  }

  two_arguments(argument, arg1, arg2);

  if (!*arg1)
  {
    send_to_char(ch, "Wear what?\r\n");
    return;
  }

  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV))
  {
    send_to_char(ch, "You can't specify the same body location for more than one item!\r\n");
    return;
  }

  /* wear all */
  if (dotmode == FIND_ALL)
  {

    /* go through all carried objects */
    for (obj = ch->carrying; obj; obj = next_obj)
    {
      next_obj = obj->next_content;

      /* where does this gear fit? */
      if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0)
      {
        if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
          send_to_char(ch, "You are not experienced enough to use %s.\r\n", GET_OBJ_SHORT(obj));
        else if (GET_OBJ_TYPE(obj) == ITEM_CLANARMOR && (GET_CLAN(ch) == NO_CLAN || (GET_OBJ_VAL(obj, 2) + 1) != GET_CLAN(ch)))
          send_to_char(ch, "You are in clan %d, This belongs to clan %d.\r\n", GET_CLAN(ch), GET_OBJ_VAL(obj, 2));
        else
        {
          items_worn++; /* counting how many items we equipped */
          perform_wear(ch, obj, where);
        }
      }
    }
    if (!items_worn)
      send_to_char(ch, "You don't seem to have anything wearable.\r\n");
  }
  /* wear all.X */
  else if (dotmode == FIND_ALLDOT)
  {

    if (!*arg1)
    {
      send_to_char(ch, "Wear all of what?\r\n");
      return;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use %s.\r\n", GET_OBJ_SHORT(obj));
    else if (GET_OBJ_TYPE(obj) == ITEM_CLANARMOR &&
             (GET_CLAN(ch) == NO_CLAN || (GET_OBJ_VAL(obj, 2) + 1) != GET_CLAN(ch)))
      send_to_char(ch, "You are in clan %d, That belongs to clan %d.\r\n", GET_CLAN(ch), GET_OBJ_VAL(obj, 2));
    else
    { /* engine! */
      while (obj)
      {
        next_obj = get_obj_in_list_vis(ch, arg1, NULL, obj->next_content);
        if ((where = find_eq_pos(ch, obj, 0)) >= 0)
          perform_wear(ch, obj, where);
        else
          act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
        obj = next_obj;
      }
    }
  }
  /* not wear all, or wear all.X */
  else
  {
    if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use %s.\r\n", GET_OBJ_SHORT(obj));
    else if (GET_OBJ_TYPE(obj) == ITEM_CLANARMOR &&
             (GET_CLAN(ch) == NO_CLAN || (GET_OBJ_VAL(obj, 2) + 1) != GET_CLAN(ch)))
      send_to_char(ch, "You are in clan %d, That belongs to clan %d.\r\n", GET_CLAN(ch), GET_OBJ_VAL(obj, 2));
    else
    {
      if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
        perform_wear(ch, obj, where);
      else if (!*arg2)
        act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }

  /* end */
}

/* the actual engine for wielding an object
   returns TRUE for success, FALSE for failure
   the not_silent variable indicates if this function is vocal or not */
bool perform_wield(struct char_data *ch, struct obj_data *obj, bool not_silent)
{
  if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
  {
    if (not_silent)
      send_to_char(ch, "You can't wield that.\r\n");
  }
  else if (OBJ_FLAGGED(obj, ITEM_MOLD))
  {
    if (not_silent)
      send_to_char(ch, "You can't wield an object mold!\r\n");
  }
  else if (GET_OBJ_WEIGHT(obj) > str_app[GET_STR(ch)].wield_w)
  {
    if (not_silent)
      send_to_char(ch, "It's too heavy for you to use.\r\n");
  }
  else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
  {
    if (not_silent)
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
  }
  else if (GET_OBJ_TYPE(obj) == ITEM_CLANARMOR &&
           (GET_CLAN(ch) == NO_CLAN || GET_OBJ_VAL(obj, 2) != GET_CLAN(ch)))
  {
    if (not_silent)
      send_to_char(ch, "You are not in the right clan to use that.\r\n");
  }
  else
  {
    perform_wear(ch, obj, WEAR_WIELD_1);
    return TRUE;
  }

  return FALSE;
}

/* entry point for the 'wield' command */
ACMD(do_wield)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
  {
    send_to_char(ch, "Why would you want to wield something? (wildshape/polymorph)\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Wield what?\r\n");
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  /* need test to make sure we don't wield a 2nd missile weapon */
  else if (is_using_ranged_weapon(ch, TRUE))
  {
    send_to_char(ch, "You are already using a ranged weapon!\r\n");
    /* wielding a weapon, now trying to dual wield a ranged weapon */
  }
  else if (obj && (GET_EQ(ch, WEAR_WIELD_1) || GET_EQ(ch, WEAR_WIELD_OFFHAND)) &&
           IS_SET(weapon_list[GET_OBJ_VAL(obj, 0)].weaponFlags, WEAPON_FLAG_RANGED))
  {
    send_to_char(ch, "You can't equip a ranged weapon while already wielding another weapon!\r\n");
  }
  else
  {
    perform_wield(ch, obj, TRUE);
  }
}

ACMD(do_grab)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
  {
    send_to_char(ch, "Why would you want to grab something? (wildshape/polymorphed)\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Hold what?\r\n");
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
    send_to_char(ch, "You are not experienced enough to use that.\r\n");
  else if (GET_OBJ_TYPE(obj) == ITEM_CLANARMOR &&
           (GET_CLAN(ch) == NO_CLAN || GET_OBJ_VAL(obj, 2) != GET_CLAN(ch)))
    send_to_char(ch, "You are not in the right clan to use that.\r\n");
  else
  {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      perform_wear(ch, obj, WEAR_LIGHT);
    else
    {
      if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND &&
          GET_OBJ_TYPE(obj) != ITEM_STAFF && GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
          GET_OBJ_TYPE(obj) != ITEM_POTION)
        send_to_char(ch, "You can't hold that.\r\n");
      else
        perform_wear(ch, obj, WEAR_HOLD_1);
    }
  }
}

void perform_remove(struct char_data *ch, int pos, bool forced)
{
  struct obj_data *obj;

  if (!(obj = GET_EQ(ch, pos)))
    log("SYSERR: perform_remove: bad pos %d passed.", pos);
  /*  This error occurs when perform_remove() is passed a bad 'pos'
     *  (location) to remove an object from. */
  else if (OBJ_FLAGGED(obj, ITEM_NODROP) &&
           !PRF_FLAGGED(ch, PRF_NOHASSLE) && !forced)
    act("You can't remove $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
  else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) &&
           !PRF_FLAGGED(ch, PRF_NOHASSLE) && !forced)
    act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
  else
  {
    if (!remove_otrigger(obj, ch))
      return;

    obj_to_char(unequip_char(ch, pos), ch);
    act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
  }
}

ACMD(do_remove)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int i = 0, dotmode = 0, found = 0;

  if (IS_WILDSHAPED(ch) || IS_MORPHED(ch))
  {
    send_to_char(ch, "Why would you want to remove something? (wildshape/polymorphed)\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg)
  {
    send_to_char(ch, "Remove what?\r\n");
    return;
  }
  dotmode = find_all_dots(arg);

  if (dotmode == FIND_ALL)
  {
    found = 0;
    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
      {
        perform_remove(ch, i, FALSE);
        found = 1;
      }
    if (!found)
      send_to_char(ch, "You're not using anything.\r\n");
  }
  else if (dotmode == FIND_ALLDOT)
  {
    if (!*arg)
      send_to_char(ch, "Remove all of what?\r\n");
    else
    {
      found = 0;
      for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) &&
            isname(arg, GET_EQ(ch, i)->name))
        {
          perform_remove(ch, i, FALSE);
          found = 1;
        }
      if (!found)
        send_to_char(ch, "You don't seem to be using any %ss.\r\n", arg);
    }
  }
  else
  {
    if ((i = get_obj_pos_in_equip_vis(ch, arg, NULL, ch->equipment)) < 0)
      send_to_char(ch, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
    else
      perform_remove(ch, i, FALSE);
  }
}

ACMD(do_sac)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *j, *jj, *next_thing2;

  one_argument(argument, arg);

  if (!*arg)
  {
    send_to_char(ch, "Sacrifice what?\n\r");
    return;
  }

  if (!(j = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)) && (!(j = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))))
  {
    send_to_char(ch, "It doesn't seem to be here.\n\r");
    return;
  }

  if (!CAN_WEAR(j, ITEM_WEAR_TAKE))
  {
    send_to_char(ch, "You can't sacrifice that!\n\r");
    return;
  }

  if (GET_OBJ_TYPE(j) == ITEM_CONTAINER && j->contains != NULL)
  {
    send_to_char(ch, "You should empty that first!\n\r");
    return;
  }

  act("$n sacrifices $p.", FALSE, ch, j, 0, TO_ROOM);

  switch (rand_number(0, 5))
  {
  case 0:
    send_to_char(ch, "You sacrifice %s to the gods.\r\nYou receive one gold coin for your humility.\r\n", GET_OBJ_SHORT(j));
    increase_gold(ch, 1);
    break;
  case 1:
    send_to_char(ch, "You sacrifice %s to the gods.\r\nThe gods ignore your sacrifice.\r\n", GET_OBJ_SHORT(j));
    break;
  case 2:
    send_to_char(ch, "You sacrifice %s to the gods.\r\nThe gods give you %d experience points.\r\n", GET_OBJ_SHORT(j), (GET_OBJ_COST(j)));
    GET_EXP(ch) += (GET_OBJ_COST(j));
    break;
  case 3:
    send_to_char(ch, "You sacrifice %s to the gods.\r\nYou receive %d experience points.\r\n", GET_OBJ_SHORT(j), GET_OBJ_COST(j) / 2);
    GET_EXP(ch) += GET_OBJ_COST(j) / 2;
    break;
  case 4:
    send_to_char(ch, "Your sacrifice to the gods is rewarded with %d gold coins.\r\n", GET_OBJ_COST(j) / 4);
    increase_gold(ch, GET_OBJ_COST(j) / 4);
    break;
  case 5:
    send_to_char(ch, "Your sacrifice to the gods is rewarded with %d gold coins\r\n", (GET_OBJ_COST(j) / 2));
    increase_gold(ch, (GET_OBJ_COST(j) / 2));
    break;
  default: /* should not get here */
    send_to_char(ch, "You sacrifice %s to the gods.\r\nYou receive one gold coin for your humility.\r\n", GET_OBJ_SHORT(j));
    increase_gold(ch, 1);
    break;
  }
  for (jj = j->contains; jj; jj = next_thing2)
  {
    next_thing2 = jj->next_content; /* Next in inventory */
    obj_from_obj(jj);

    if (j->carried_by)
      obj_to_room(jj, IN_ROOM(j));
    else if (IN_ROOM(j) != NOWHERE)
      obj_to_room(jj, IN_ROOM(j));
    else
      assert(FALSE);
  }
  extract_obj(j);
}

struct obj_data *find_lootbox_in_room_vis(struct char_data *ch)
{

  struct obj_data *obj = NULL;

  for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
  {
    if (GET_OBJ_TYPE(obj) == ITEM_TREASURE_CHEST)
      if (CAN_SEE_OBJ(ch, obj))
        return obj;
  }

  return NULL;
}

// Used with treasure chests that allow each individual character to loot it once every 4 hours
ACMD(do_loot)
{
  struct obj_data *obj = find_lootbox_in_room_vis(ch);

  if (!obj)
  {
    send_to_char(ch, "There are no visible loot boxes here.\r\n");
    return;
  }

  if (IS_NPC(ch))
  {
    send_to_char(ch, "Lootboxes can only be opened by player characters.\r\n");
    return;
  }

  if (((LOOTBOX_LEVEL(obj) * 5) - 10) > GET_LEVEL(ch))
  {
    send_to_char(ch, "You are too low level to open that lootbox.\r\n");
    return;
  }

  struct char_data *tch = NULL;
  sbyte pilfer = false;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch))
      continue;
    if (AFF_FLAGGED(tch, AFF_CHARM))
      continue;
    if (subcmd == SCMD_PILFER)
    {
      if (skill_check(ch, ABILITY_SLEIGHT_OF_HAND, dice(1, 20) + (GET_LEVEL(tch) * 0.75)))
      {
        pilfer = true;
        continue;
      }
      else
      {
        act("$N notices your attempt to pilfer the treasure and attacks!", TRUE, ch, 0, tch, TO_CHAR);
        act("You notice $n's attempt to pilfer the treasure and attacks!", TRUE, ch, 0, tch, TO_VICT);
        act("$N notices $n's attempt to pilfer the treasure and attacks!", TRUE, ch, 0, tch, TO_NOTVICT);
        hit(tch, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    }
    send_to_char(ch, "You must defeat all non-charmie mobs in this room before you can loot anything.\r\n");
    return;
  }

  if (pilfer)
  {
    send_to_char(ch, "You deftly maneuver your way to the treasure unseen.\r\n");
  }

  obj_vnum vnum = GET_OBJ_VNUM(obj);

  char query[500], last[20], curr[20];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row = NULL;
  int found = FALSE;

  /* Check the connection, reconnect if necessary. */
  //	mysql_ping(conn);

  snprintf(query, sizeof(query), "SELECT last_loot, DATE_ADD(last_loot, INTERVAL 4 HOUR) as curr_time, DATE_ADD(last_loot, INTERVAL 4 HOUR) as reloot "
                 "FROM loot_chests WHERE chest_vnum='%d' AND character_name='%s' AND DATE_ADD(last_loot, INTERVAL 4 HOUR) > NOW()",
          vnum, GET_NAME(ch));

  mysql_query(conn, query);
  res = mysql_use_result(conn);
  if (res != NULL)
  {
    if ((row = mysql_fetch_row(res)) != NULL)
    {
      snprintf(last, sizeof(last), "%s", row[0]);
      snprintf(curr, sizeof(curr), "%s", row[1]);
      found = TRUE;
    }
  }

  mysql_free_result(res);

  if (found && GET_LEVEL(ch) < LVL_IMMORT)
  { // they've looted it less than four hours ago, so no go.
    char *tmstr;
    time_t mytime;

    mytime = time(0);

    tmstr = (char *)asctime(localtime(&mytime));
    *(tmstr + strlen(tmstr) - 1) = '\0';
    send_to_char(ch, "You've already looted this chest.  Your last loot was %s, and you can loot again at %s, server time. ", last, curr);
    send_to_char(ch, "Current machine time: %s\r\n", tmstr);
    //		  send_to_char(ch, "\r\nQUERY: %s\r\n", query);
    return;
  }

  snprintf(query, sizeof(query), "DELETE FROM loot_chests WHERE chest_vnum='%d' AND character_name='%s'", vnum, GET_NAME(ch));
  mysql_query(conn, query);

  snprintf(query, sizeof(query), "INSERT INTO loot_chests (loot_id, chest_vnum, character_name, last_loot) VALUES(NULL,'%d','%s',NOW())", vnum, GET_NAME(ch));
  mysql_query(conn, query);

  int level = 0, max_grade = LOOTBOX_LEVEL_MUNDANE;

  level = LOOTBOX_LEVEL(obj);

  if (level >= 6)
  {
    max_grade = LOOTBOX_LEVEL_SUPERIOR;
  }
  else if (level >= 5)
  {
    max_grade = LOOTBOX_LEVEL_MAJOR;
  }
  else if (level >= 4)
  {
    max_grade = LOOTBOX_LEVEL_MEDIUM;
  }
  else if (level >= 3)
  {
    max_grade = LOOTBOX_LEVEL_TYPICAL;
  }
  else if (level >= 2)
  {
    max_grade = LOOTBOX_LEVEL_MINOR;
  }
  else
  {
    max_grade = LOOTBOX_LEVEL_MUNDANE;
  }

  int gold = 50;

  gold += dice(level * 5, 10) * 2;

  if (LOOTBOX_TYPE(obj) == LOOTBOX_TYPE_GOLD)
    gold *= 5;

  GET_GOLD(ch) += gold;
  send_to_char(ch, "You find %d gold coins in the chest.\r\n", gold);

  sbyte recMagic = false;
  sbyte recArmor, recWeapon, recTrinket, recConsumable, recCrystal;
  byte chance = 8; // 1 in 8 chance by default for generic chests

  recArmor = recWeapon = recConsumable = recTrinket = recCrystal = false;

  switch (LOOTBOX_TYPE(obj))
  {
  case LOOTBOX_TYPE_WEAPON:
    award_magic_weapon(ch, max_grade);
    chance = 12;
    recWeapon = true;
    recMagic = true;
    break;
  case LOOTBOX_TYPE_ARMOR:
    award_magic_armor(ch, max_grade, -1);
    chance = 12;
    recArmor = true;
    recMagic = true;
    break;
  case LOOTBOX_TYPE_CONSUMABLE:
    switch (dice(1, 4))
    {
    case 1:
      award_expendable_item(ch, max_grade, TYPE_SCROLL);
      break;
    case 2:
      award_expendable_item(ch, max_grade, TYPE_POTION);
      break;
    case 3:
      award_expendable_item(ch, max_grade, TYPE_WAND);
      break;
    case 4:
      award_expendable_item(ch, max_grade, TYPE_STAFF);
      break;
    }
    chance = 12;
    recConsumable = true;
    recMagic = true;
    break;
  case LOOTBOX_TYPE_TRINKET:
    award_misc_magic_item(ch, determine_rnd_misc_cat(), cp_convert_grade_enchantment(max_grade));
    chance = 12;
    recTrinket = true;
    recMagic = true;
    break;
  case LOOTBOX_TYPE_CRYSTAL:
    award_random_crystal(ch, max_grade);
    chance = 12;
    recCrystal = true;
    recMagic = true;
    break;
  case LOOTBOX_TYPE_GOLD:
    chance = 12; // less of a chance to get more than one item in addition to the gold.
    recMagic = false;
    break;
  case LOOTBOX_TYPE_GENERIC:
  default: // generic type
    chance = 8;
    recMagic = false;
    break;
  }

  do
  {
    if (dice(1, chance) == 1 && !recCrystal)
    {
      award_random_crystal(ch, max_grade);
      recMagic = true;
    }
    if (dice(1, chance) == 1 && !recWeapon)
    {
      award_magic_weapon(ch, max_grade);
      recMagic = true;
    }
    if (dice(1, chance) == 1 && !recConsumable)
    {
      award_expendable_item(ch, max_grade, TYPE_SCROLL);
      recMagic = true;
    }
    if (dice(1, chance) == 1 && !recConsumable)
    {
      award_expendable_item(ch, max_grade, TYPE_POTION);
      recMagic = true;
    }
    if (dice(1, chance) == 1 && !recConsumable)
    {
      award_expendable_item(ch, max_grade, TYPE_WAND);
      recMagic = true;
    }
    if (dice(1, chance) == 1 && !recConsumable)
    {
      award_expendable_item(ch, max_grade, TYPE_STAFF);
      recMagic = true;
    }
    if (dice(1, chance) == 1 && !recConsumable)
    {
      award_magic_ammo(ch, max_grade);
      recMagic = true;
    }
    if (dice(1, chance) == 1 && !recTrinket)
    {
      award_misc_magic_item(ch, determine_rnd_misc_cat(), cp_convert_grade_enchantment(max_grade));
      recMagic = true;
    }
    if (dice(1, chance) == 1 && !recArmor)
    {
      award_magic_armor(ch, max_grade, -1);
      recMagic = true;
    }
  } while (!recMagic);

  //mysql_close(conn);
}

/* EOF */
