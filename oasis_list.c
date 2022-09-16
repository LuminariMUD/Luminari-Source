/**************************************************************************
 *  File: oasis_list.c                                 Part of LuminariMUD *
 *  Usage: Oasis OLC listings.                                             *
 *                                                                         *
 * By Levork. Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.        *
 * 2002 Kip Potter [Mythran].                                              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "shop.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "quest.h"
#include "modify.h"
#include "spells.h"
#include "race.h"
#include "genzon.h"
#include "class.h"
#include "genshp.h"
#include "wilderness.h"
#include "assign_wpn_armor.h"

#define MAX_OBJ_LIST 100

struct obj_list_item
{
  obj_vnum vobj;
  int val;
};
/* local functions */
static void list_triggers(struct char_data *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax);
static void list_rooms(struct char_data *ch, zone_rnum rnum, room_vnum vmin, room_vnum vmax);
static void list_mobiles(struct char_data *ch, zone_rnum rnum, mob_vnum vmin, mob_vnum vmax);
static void list_objects(struct char_data *ch, zone_rnum rnum, obj_vnum vmin, obj_vnum vmax);
static void list_shops(struct char_data *ch, zone_rnum rnum, shop_vnum vmin, shop_vnum vmax);
static void list_zones(struct char_data *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax, char *name);
static void list_regions(struct char_data *ch);
static void list_paths(struct char_data *ch);

/* unfinished, wanted to build a mobile name lookup (prototypes) */
void perform_mob_name_list(struct char_data *ch, char *arg)
{
  int num = 0, mob_flag = -1, found = 0, len = 0;
  struct char_data *mob = NULL;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  mob_flag = atoi(arg);

  if (mob_flag < 0 || mob_flag > NUM_MOB_FLAGS)
  {
    send_to_char(ch, "Invalid flag number!\r\n");
    return;
  }

  len = snprintf(buf, sizeof(buf), "Listing mobiles with %s%s%s flag set.\r\n", QYEL, action_bits[mob_flag], QNRM);

  for (num = 0; num <= top_of_mobt; num++)
  {
    if (IS_SET_AR((mob_proto[num].char_specials.saved.act), mob_flag))
    {

      if ((mob = read_mobile(num, REAL)) != NULL)
      {
        char_to_room(mob, 0);
        len += snprintf(buf + len, sizeof(buf) - len, "%s%3d. %s[%s%5d%s]%s Level %s%-3d%s %s%s\r\n", CCNRM(ch, C_NRM), ++found,
                        CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), GET_MOB_VNUM(mob), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                        CCYEL(ch, C_NRM), GET_LEVEL(mob), CCNRM(ch, C_NRM), GET_NAME(mob), CCNRM(ch, C_NRM));
        extract_char(mob); /* Finished with the mob - remove it from the MUD */
        if (len > sizeof(buf))
          break;
      }
    }
  }
  if (!found)
    send_to_char(ch, "None Found!\r\n");
  else
    page_string(ch->desc, buf, TRUE);
  return;
}

void perform_mob_flag_list(struct char_data *ch, char *arg)
{
  int num, mob_flag, found = 0, len;
  struct char_data *mob;
  char buf[MAX_STRING_LENGTH];

  mob_flag = atoi(arg);

  if (mob_flag < 0 || mob_flag > NUM_MOB_FLAGS)
  {
    send_to_char(ch, "Invalid flag number!\r\n");
    return;
  }

  len = snprintf(buf, sizeof(buf), "Listing mobiles with %s%s%s flag set.\r\n", QYEL, action_bits[mob_flag], QNRM);

  for (num = 0; num <= top_of_mobt; num++)
  {
    if (IS_SET_AR((mob_proto[num].char_specials.saved.act), mob_flag))
    {

      if ((mob = read_mobile(num, REAL)) != NULL)
      {
        char_to_room(mob, 0);
        len += snprintf(buf + len, sizeof(buf) - len, "%s%3d. %s[%s%5d%s]%s Level %s%-3d%s %s%s\r\n", CCNRM(ch, C_NRM), ++found,
                        CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), GET_MOB_VNUM(mob), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                        CCYEL(ch, C_NRM), GET_LEVEL(mob), CCNRM(ch, C_NRM), GET_NAME(mob), CCNRM(ch, C_NRM));
        extract_char(mob); /* Finished with the mob - remove it from the MUD */
        if (len > sizeof(buf))
          break;
      }
    }
  }
  if (!found)
    send_to_char(ch, "None Found!\r\n");
  else
    page_string(ch->desc, buf, TRUE);
  return;
}

void perform_mob_level_list(struct char_data *ch, char *arg)
{
  int num, mob_level, found = 0, len;
  struct char_data *mob;
  char buf[MAX_STRING_LENGTH];

  mob_level = atoi(arg);

  if (mob_level < 0 || mob_level > 99)
  {
    send_to_char(ch, "Invalid mob level!\r\n");
    return;
  }

  len = snprintf(buf, sizeof(buf), "Listing mobiles of level %s%d%s\r\n", QYEL, mob_level, QNRM);
  for (num = 0; num <= top_of_mobt; num++)
  {
    if ((mob_proto[num].player.level) == mob_level)
    {

      if ((mob = read_mobile(num, REAL)) != NULL)
      {
        char_to_room(mob, 0);
        len += snprintf(buf + len, sizeof(buf) - len, "%s%3d. %s[%s%5d%s]%s %s%s\r\n", CCNRM(ch, C_NRM), ++found,
                        CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), GET_MOB_VNUM(mob), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                        GET_NAME(mob), CCNRM(ch, C_NRM));
        extract_char(mob); /* Finished with the mob - remove it from the MUD */
        if (len > sizeof(buf))
          break;
      }
    }
  }
  if (!found)
    send_to_char(ch, "None Found!\r\n");
  else
    page_string(ch->desc, buf, TRUE);

  return;
}

void add_to_obj_list(struct obj_list_item *lst, int num_items, obj_vnum nvo, int nval)
{
  int j, tmp_v;
  obj_vnum tmp_ov;

  for (j = 0; j < num_items; j++)
  {
    if (nval > lst[j].val)
    {
      tmp_ov = lst[j].vobj;
      tmp_v = lst[j].val;

      lst[j].vobj = nvo;
      lst[j].val = nval;

      nvo = tmp_ov;
      nval = tmp_v;
    }
  }
}

/* list objects by type */
void perform_obj_type_list(struct char_data *ch, char *arg)
{
  int num, itemtype, v1, v2 = -1, v3 = -1, v4 = -1, v5 = -1, found = 0,
                         len = 0, tmp_len = 0;
  obj_vnum ov;
  obj_rnum r_num, target_obj = NOTHING;
  char buf[MAX_STRING_LENGTH];
  char buf2[256];

  *buf2 = '\0';
  itemtype = atoi(arg);

  len = snprintf(buf, sizeof(buf), "Listing all objects of type %s[%s]%s\r\n",
                 QYEL, item_types[itemtype], QNRM);

  for (num = 0; num <= top_of_objt; num++)
  {
    if (obj_proto[num].obj_flags.type_flag == itemtype)
    {
      if ((r_num = real_object(obj_index[num].vnum)) != NOTHING)
      { /* Seems silly? */
        /* Set default vals, which may be changed below */
        ov = obj_index[num].vnum;
        v1 = (obj_proto[num].obj_flags.value[0]);
        v2 = (obj_proto[num].obj_flags.value[1]);
        v3 = (obj_proto[num].obj_flags.value[2]);
        v4 = (obj_proto[num].obj_flags.value[3]);
        v5 = (obj_proto[num].obj_flags.value[4]);

        switch (itemtype)
        {

        case ITEM_SWITCH:
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%5d%s]%s "
                                                           "[%s, affecting room VNum %d, %s %s] "
                                                           "%s%s%s\r\n",
                             QGRN, ++found, QNRM, /**/ QCYN, QYEL, ov, QCYN, QNRM,
                             (v1 == 0) ? "Push switch" : (v1 == 1) ? "Pull switch"
                                                                   : "BROKEN switch type",
                             v2,
                             (v4 == 0) ? "Unhides" : (v4 == 1) ? "Unlocks"
                                                 : (v4 == 2)   ? "Opens"
                                                               : "BROKEN exit action",
                             (v3 == 0) ? "North" : (v3 == 1) ? "East"
                                               : (v3 == 2)   ? "South"
                                               : (v3 == 3)   ? "West"
                                               : (v3 == 4)   ? "Up"
                                               : (v3 == 5)   ? "Down"
                                                             : "BROKEN direction",
                             QCYN, obj_proto[r_num].short_description, QNRM);
          break;

          /* traps, big case */
        case ITEM_TRAP:
          target_obj = real_object(v2);
          /* v1 - object value (0) is the trap-type */
          /* v2 - object value (1) is the direction of the trap (TRAP_TYPE_OPEN_DOOR and TRAP_TYPE_UNLOCK_DOOR)
                 or the object-vnum (TRAP_TYPE_OPEN_CONTAINER and TRAP_TYPE_UNLOCK_CONTAINER and TRAP_TYPE_GET_OBJECT) */
          /* v3 - object value (2) is the effect */
          /* v4 - object value (3) is the trap difficulty */
          /* v5 - object value (4) is whether this trap has been "detected" yet */

          /* check disqualifications */
          if (v1 < 0 || v1 >= MAX_TRAP_TYPES)
          { /* invalid trap types */
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d INVALID, CHECK THIS OBJECT (trap-type)\r\n",
                               QGRN, ++found, QNRM, ov);
            break;
          }
          if (v3 <= 0 || v3 >= TOP_TRAP_EFFECTS)
          { /* invalid trap effects */
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d INVALID, CHECK THIS OBJECT (effect-range)\r\n",
                               QGRN, ++found, QNRM, ov);
            break;
          }
          if (v3 < TRAP_EFFECT_FIRST_VALUE && v3 >= LAST_SPELL_DEFINE)
          { /* invalid trap effects check 2 */
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d INVALID, CHECK THIS OBJECT (effect-range-2)\r\n",
                               QGRN, ++found, QNRM, ov);
            break;
          }
          if ((v1 == TRAP_TYPE_OPEN_CONTAINER ||
               v1 == TRAP_TYPE_UNLOCK_CONTAINER ||
               v1 == TRAP_TYPE_GET_OBJECT) &&
              target_obj == NOTHING)
          {
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d INVALID, CHECK THIS OBJECT (object vnum)\r\n",
                               QGRN, ++found, QNRM, ov);
            break;
          }
          /* end disqualifications */

          switch (v1)
          {
          case TRAP_TYPE_LEAVE_ROOM: /* display effect and difficulty */
            if (v3 >= TRAP_EFFECT_FIRST_VALUE)
            { /* not a normal spell effect */
              tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d %s%s | Type: %s | Effect: %s | DC: %d | Detected? %d\r\n",
                                 QGRN, ++found, QNRM, ov, obj_proto[r_num].short_description, QNRM, trap_type[v1], trap_effects[v3 - 1000], v4, v5);
            }
            else
            { /* spell effect */
              tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d %s%s | Type: %s |  Spell: %s | DC: %d | Detected? %d\r\n",
                                 QGRN, ++found, QNRM, ov, obj_proto[r_num].short_description, QNRM, trap_type[v1], spell_info[v3].name, v4, v5);
            }
            break;
          case TRAP_TYPE_ENTER_ROOM: /* display effect and difficulty */
            if (v3 >= TRAP_EFFECT_FIRST_VALUE)
            { /* not a normal spell effect */
              tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d %s%s | Type: %s | Effect: %s | DC: %d | Detected? %d\r\n",
                                 QGRN, ++found, QNRM, ov, obj_proto[r_num].short_description, QNRM, trap_type[v1], trap_effects[v3 - 1000], v4, v5);
            }
            else
            { /* spell effect */
              tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d %s%s | Type: %s |  Spell: %s | DC: %d | Detected? %d\r\n",
                                 QGRN, ++found, QNRM, ov, obj_proto[r_num].short_description, QNRM, trap_type[v1], spell_info[v3].name, v4, v5);
            }
            break;
          case TRAP_TYPE_OPEN_DOOR:
            /*fall through*/
          case TRAP_TYPE_UNLOCK_DOOR: /* display direction, effect, difficulty */
            if (v3 >= TRAP_EFFECT_FIRST_VALUE)
            { /* not a normal spell effect */
              tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d %s%s | Type: %s |  Direction: %s | Effect: %s | DC: %d | Detected? %d\r\n",
                                 QGRN, ++found, QNRM, ov, obj_proto[r_num].short_description, QNRM, trap_type[v1], dirs[v2], trap_effects[v3 - 1000], v4, v5);
            }
            else
            { /* spell effect */
              tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d %s%s | Type: %s |  Direction: %s | Spell: %s | DC: %d | Detected? %d\r\n",
                                 QGRN, ++found, QNRM, ov, obj_proto[r_num].short_description, QNRM, trap_type[v1], dirs[v2], spell_info[v3].name, v4, v5);
            }
            break;
          case TRAP_TYPE_OPEN_CONTAINER:
            /*fall through*/
          case TRAP_TYPE_UNLOCK_CONTAINER:
            /*fall through*/
          case TRAP_TYPE_GET_OBJECT: /* display vnum, effect, difficulty */
            if (v3 >= TRAP_EFFECT_FIRST_VALUE)
            { /* not a normal spell effect */
              tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d %s%s | Type: %s |  On Obj: %s | Effect: %s | DC: %d | Detected? %d\r\n",
                                 QGRN, ++found, QNRM, ov, obj_proto[r_num].short_description, QNRM, trap_type[v1], obj_proto[target_obj].short_description, trap_effects[v3 - 1000], v4, v5);
            }
            else
            { /* spell effect */
              tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %7d %s%s | Type: %s |  On Obj: %s | Spell: %s | DC: %d | Detected? %d\r\n",
                                 QGRN, ++found, QNRM, ov, obj_proto[r_num].short_description, QNRM, trap_type[v1], obj_proto[target_obj].short_description, spell_info[v3].name, v4, v5);
            }
            break;
          default: /* invalid type! we checked this already above */
            break;
          }
          break;

          /** END TRAPS **/

        case ITEM_LIGHT:
          v1 = (obj_proto[num].obj_flags.value[2]);
          if (v1 == -1)
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%5d%s]%s INFINITE%s %s%s\r\n",
                               QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QBRED, QCYN, obj_proto[r_num].short_description, QNRM);
          else
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%5d%s]%s (%-3dhrs) %s%s%s\r\n",
                               QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
          break;

        case ITEM_SCROLL:
        case ITEM_POTION:
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s] [%15s] %s%s\r\n",
                             QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN,
                             spell_info[obj_proto[num].obj_flags.value[1]].name,
                             obj_proto[r_num].short_description, QNRM);
          break;

        case ITEM_WAND:
        case ITEM_STAFF:
          v1 = (obj_proto[num].obj_flags.value[1]);
          v2 = (obj_proto[num].obj_flags.value[3]);
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s]%s (%dx%s) %s%s%s\r\n",
                             QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, spell_name(v2), QCYN, obj_proto[r_num].short_description, QNRM);
          break;

        case ITEM_POISON:
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s%7d%s (Poison:%s%s|Level:%d|Applications:%d|Hits/App:%d) %s%s\r\n",
                             QGRN, ++found, QNRM, QYEL, ov, QNRM, spell_name(v1), QNRM, v2, v3, v4, obj_proto[r_num].short_description, QNRM);
          break;

        case ITEM_WEAPON:
          v1 = ((obj_proto[num].obj_flags.value[2] + 1) * (obj_proto[r_num].obj_flags.value[1])) / 2;
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s]%s (%d Avg Dam) %s%s%s\r\n",
                             QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
          break;

        case ITEM_ARMOR:
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s]%s (%dAC) %s%s%s\r\n",
                             QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
          break;

        case ITEM_CONTAINER:
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s]%s (Max: %d) %s%s%s\r\n",
                             QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
          break;

        case ITEM_AMMO_POUCH:
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s]%s (Max: %d) %s%s%s\r\n",
                             QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, QNRM);
          break;

        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
          v2 = (obj_proto[num].obj_flags.value[3]);

          if (v2 != 0)
            snprintf(buf2, sizeof(buf2), " \tc[\tyspell %d\tn: %s\tc]\tn", v2, spell_info[v2].name);
          else
            *buf2 = '\0';

          if (v1 != -1)
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s]%s (Max: %d) %s%s%s%s\r\n",
                               QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, buf2, QNRM);
          else
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s] %sINFINITE%s %s%s%s\r\n",
                               QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QBRED, QCYN, obj_proto[r_num].short_description, buf2, QNRM);
          break;

        case ITEM_FOOD:
          v2 = (obj_proto[num].obj_flags.value[1]);
          v3 = (obj_proto[num].obj_flags.value[3]);

          if (v2 != 0)
            snprintf(buf2, sizeof(buf2), " \tc[\tyspell %d\tn: %s\tc]\tn", v2, spell_info[v2].name);
          else
            *buf2 = '\0';

          if (v3 != 0)
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s]%s (%2dhrs) %s%s%s %sPoisoned!%s\r\n",
                               QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, buf2, QBGRN, QNRM);
          else
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s]%s (%2dhrs) %s%s%s%s\r\n",
                               QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM, v1, QCYN, obj_proto[r_num].short_description, buf2, QNRM);
          break;

        case ITEM_MONEY:
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s] %s%s (%s%d coins%s)\r\n",
                             QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, obj_proto[r_num].short_description, QNRM, QYEL, v1, QNRM);
          break;

        case ITEM_PORTAL:
          v2 = (obj_proto[num].obj_flags.value[1]);
          v3 = (obj_proto[num].obj_flags.value[2]);
          if (v1 < 0 || v1 > NUM_PORTAL_TYPES)
          {
            tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s] %s%s (%s INVALID %s)\r\n",
                               QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, obj_proto[r_num].short_description, QNRM, QYEL, QNRM);
            break;
          }
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s] %s%s (%s%s/To: %d-%d%s)\r\n",
                             QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, obj_proto[r_num].short_description, QNRM, QYEL, portal_types[v1], v2, v3, QNRM);
          break;

        case ITEM_INSTRUMENT:
          tmp_len = snprintf(buf + len, sizeof(buf) - len,
                             "%s%3d%s) %s%7d%s (%s%s | Difficulty: %d | Level: %d | Breakability: %d) %s%s\r\n",
                             QGRN, ++found, QNRM, QYEL, ov, QNRM, instrument_names[v1], QNRM, v2, v3, v4, obj_proto[r_num].short_description, QNRM);
          break;

        /* The 'normal' items - don't provide extra info */
        case ITEM_TELEPORT:
        case ITEM_SUMMON:
        case ITEM_CRYSTAL:
        case ITEM_ESSENCE:
        case ITEM_CLANARMOR:
        case ITEM_MATERIAL:
        case ITEM_SPELLBOOK:
        case ITEM_PLANT:
        case ITEM_PICK:
        case ITEM_DISGUISE:
        case ITEM_WALL:
        case ITEM_BOWL:
        case ITEM_INGREDIENT:
        case ITEM_BLOCKER:
        case ITEM_WAGON:
        case ITEM_RESOURCE:
        case ITEM_PET:
        case ITEM_BLUEPRINT:
        case ITEM_TREASURE_CHEST:
        case ITEM_HUNT_TROPHY:
        case ITEM_WEAPON_OIL:

        /* stock item types */
        case ITEM_TREASURE:
        case ITEM_TRASH:
        case ITEM_OTHER:
        case ITEM_WORN:
        case ITEM_NOTE:
        case ITEM_PEN:
        case ITEM_BOAT:
        case ITEM_KEY:
        case ITEM_FURNITURE:
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s] %s%s\r\n",
                             QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, obj_proto[r_num].short_description, QNRM);
          break;

        default:
          send_to_char(ch, "Not a valid item type (request coders to add)");
          return;
        }
        if (len + tmp_len < sizeof(buf) - 1)
          len += tmp_len;
        else
        {
          buf[sizeof(buf) - 1] = '\0';
          break;
        }
      }
    }
  }
  page_string(ch->desc, buf, TRUE);
}

/* this function is ran for doing:  olist worn <slot> */
void perform_obj_worn_list(struct char_data *ch, char *arg)
{
  int num, wearloc, found = 0, len = 0, tmp_len = 0, i = 0;
  obj_vnum ov;
  char buf[MAX_STRING_LENGTH], bitbuf[MEDIUM_STRING];
  struct obj_data *obj = NULL;

  wearloc = atoi(arg);

  /* 0 = takeable */
  if (wearloc >= NUM_ITEM_WEARS || wearloc <= 0)
  {
    send_to_char(ch, "Out of bounds\r\n");
    return;
  }

  len = snprintf(buf, sizeof(buf), "Listing all objects with wear location %s[%s]%s\r\n",
                 QYEL, wear_bits[wearloc], QNRM);

  for (num = 0; num <= top_of_objt; num++)
  {
    /* set obj to the address of the proto */
    obj = &obj_proto[num];

    if (!obj) /* dummy check */
      break;

    if (IS_SET_AR(obj_proto[num].obj_flags.wear_flags, wearloc))
    {
      /* Display this object. */
      ov = obj_index[num].vnum;

      /* display index, vnum */
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d %7d ",
                         QNRM, ++found, ov);
      len += tmp_len;

      /* has affects? */
      /*
      tmp_len = snprintf(buf + len, sizeof (buf) - len, "%s ",
                         GET_OBJ_AFFECT(obj) ? "Y" : "N");
      len += tmp_len;
       */

      /* display short descrip */
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "%-*s%s | ",
                         32 + count_color_chars(obj_proto[num].short_description),
                         obj_proto[num].short_description, QNRM);
      len += tmp_len;

      /* has affect locations? */
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
      {
        if ((obj->affected[i].location != APPLY_NONE) &&
            (obj->affected[i].modifier != 0))
        {
          sprinttype(obj->affected[i].location, apply_types, bitbuf, sizeof(bitbuf));
          tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s %d ", bitbuf, obj->affected[i].modifier);
          len += tmp_len;
        }
      }

      /* sending a carrier return */
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "\r\n");
      len += tmp_len;
    }

    /* another dummy check */
    if (found >= 700)
    {
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "**OVERLOADED BUFF***\r\n");
      len += tmp_len;

      break;
    }
  }

  page_string(ch->desc, buf, TRUE);
  return;
}

void perform_obj_aff_list(struct char_data *ch, char *arg)
{
  int num, i, apply, v1 = 0, found = 0, len = 0, tmp_len = 0;
  struct obj_list_item lst[MAX_OBJ_LIST];
  obj_rnum r_num;
  obj_vnum ov;
  char buf[MAX_STRING_LENGTH];

  for (i = 0; i < MAX_OBJ_LIST; i++)
  {
    lst[i].vobj = NOTHING;
    lst[i].val = 0;
  }
  apply = atoi(arg);

  if (!(apply > 0 && apply < NUM_APPLIES))
  {
    send_to_char(ch, "Not a valid affect");
    return;
  }                                  /* Special cases below */
  else if ((apply == APPLY_CLASS) || /* olist affect 7 is Weapon Damage      */
           (apply == APPLY_LEVEL))
  { /* olist affect 8 is AC-Apply for Armor */
    for (num = 0; num <= top_of_objt; num++)
    {
      if ((apply == APPLY_CLASS && obj_proto[num].obj_flags.type_flag == ITEM_WEAPON) ||
          (apply == APPLY_LEVEL && obj_proto[num].obj_flags.type_flag == ITEM_ARMOR))
      {
        ov = obj_index[num].vnum;
        if (apply == APPLY_CLASS)
          v1 = ((obj_proto[num].obj_flags.value[2] + 1) * (obj_proto[num].obj_flags.value[1]) / 2);
        else
          v1 = (obj_proto[num].obj_flags.value[0]);

        if ((r_num = real_object(ov)) != NOTHING)
          add_to_obj_list(lst, MAX_OBJ_LIST, ov, v1);
      }
    }

    if (apply == APPLY_CLASS)
      len = snprintf(buf, sizeof(buf), "Highest average damage per hit for Weapons\r\n");
    else if (apply == APPLY_LEVEL)
      len = snprintf(buf, sizeof(buf), "Highest AC Apply for Armor\r\n");

    for (i = 0; i < MAX_OBJ_LIST; i++)
    {
      if ((r_num = real_object(lst[i].vobj)) != NOTHING)
      {
        tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%5d%s] %s%3d %s%-*s %s[%s]%s%s\r\n",
                           QGRN, ++found, QNRM, QCYN, QYEL, lst[i].vobj, QCYN,
                           QYEL, lst[i].val, QCYN, 42 + count_color_chars(obj_proto[num].short_description),
                           obj_proto[r_num].short_description,
                           QYEL, item_types[obj_proto[num].obj_flags.type_flag], QNRM,
                           obj_proto[num].proto_script ? " [TRIG]" : "");
        len += tmp_len;
        if (len >= (MAX_STRING_LENGTH - SMALL_STRING))
          break; /* zusuk put this check here */
      }
    }
    page_string(ch->desc, buf, TRUE);
    return; /* End of special-case handling */
  }
  /* Non-special cases, list objects by affect */
  for (num = 0; num <= top_of_objt; num++)
  {
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
      if (obj_proto[num].affected[i].modifier)
      {
        if (obj_proto[num].affected[i].location == apply)
        {
          ov = obj_index[num].vnum;
          v1 = obj_proto[num].affected[i].modifier;

          if ((r_num = real_object(ov)) != NOTHING)
            add_to_obj_list(lst, MAX_OBJ_LIST, ov, v1);
        }
      }
    }
  }
  len = snprintf(buf, sizeof(buf), "Objects with highest %s affect\r\n", apply_types[(apply)]);
  for (i = 0; i < MAX_OBJ_LIST; i++)
  {
    if ((r_num = real_object(lst[i].vobj)) != NOTHING)
    {
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%3d%s) %s[%s%8d%s] %s%3d %s%-*s %s[%s]%s%s\r\n",
                         QGRN, ++found, QNRM, QCYN, QYEL, lst[i].vobj, QCYN,
                         QYEL, lst[i].val, QCYN, 42 + count_color_chars(obj_proto[num].short_description),
                         obj_proto[r_num].short_description,
                         QYEL, item_types[obj_proto[r_num].obj_flags.type_flag], QNRM,
                         obj_proto[r_num].proto_script ? " [TRIG]" : "");
      len += tmp_len;
      if (len >= (MAX_STRING_LENGTH - SMALL_STRING))
        break; /* zusuk put this check here */
    }
  }
  page_string(ch->desc, buf, TRUE);
}

void perform_obj_name_list(struct char_data *ch, char *arg)
{
  int num, found = 0, len = 0, tmp_len = 0;
  obj_vnum ov;
  char buf[MAX_STRING_LENGTH];

  len = snprintf(buf, sizeof(buf), "Objects with the name '%s'\r\n"
                                   "Index VNum    Num   Object Name                                Object Type\r\n"
                                   "----- ------- ----- ------------------------------------------ ----------------\r\n",
                 arg);
  for (num = 0; num <= top_of_objt; num++)
  {
    if (is_name(arg, obj_proto[num].name))
    {
      ov = obj_index[num].vnum;
      tmp_len = snprintf(buf + len, sizeof(buf) - len, "%s%4d%s) %s[%s%5d%s] %s(%s%3d%s)%s %-*s%s [%s]%s%s\r\n",
                         QGRN, ++found, QNRM, QCYN, QYEL, ov, QCYN, QNRM,
                         QGRN, obj_index[num].number, QNRM, QCYN, 42 + count_color_chars(obj_proto[num].short_description),
                         obj_proto[num].short_description, QYEL, item_types[obj_proto[num].obj_flags.type_flag], QNRM,
                         obj_proto[num].proto_script ? " [TRIG]" : "");
      len += tmp_len;
      if (len >= (MAX_STRING_LENGTH - SMALL_STRING))
        break; /* zusuk put this check here */
    }
  }

  page_string(ch->desc, buf, TRUE);
}

/* Ingame Commands */
ACMD(do_oasis_list)
{
  zone_rnum rzone = NOWHERE;
  room_rnum vmin = NOWHERE;
  room_rnum vmax = NOWHERE;
  char smin[MAX_INPUT_LENGTH];
  char smax[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  bool use_name = FALSE;
  int i;

  two_arguments(argument, smin, sizeof(smin), smax, sizeof(smax));

  if (!*smin || *smin == '.')
  {
    rzone = world[IN_ROOM(ch)].zone;
  }
  else if (!*smax)
  {
    rzone = real_zone(atoi(smin));

    if ((rzone == NOWHERE || rzone == 0) && subcmd == SCMD_OASIS_ZLIST && !isdigit(*smin))
    {
      /* Must be zlist, with builder name as arg */
      use_name = TRUE;
    }
    else if (rzone == NOWHERE)
    {
      send_to_char(ch, "Sorry, there's no zone with that number\r\n");
      return;
    }
  }
  else
  {
    /* Listing by min vnum / max vnum.  Retrieve the numeric values. */
    vmin = atoi(smin);
    vmax = atoi(smax);

    if (vmin > vmax)
    {
      send_to_char(ch, "List from %d to %d - Aren't we funny today!\r\n", vmin, vmax);
      return;
    }
  }

  switch (subcmd)
  {
  case SCMD_OASIS_PATHLIST:
    two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));
    if (!IS_WILDERNESS_VNUM(world[IN_ROOM(ch)].number))
    {
      send_to_char(ch, "This command is only available while in the wilderness.\r\n");
      return;
    }
    if (is_abbrev(arg, "help"))
    {
      send_to_char(ch, "Usage: %spathlist <distance>%s    - List paths within a particular distance\r\n", QYEL, QNRM);
      send_to_char(ch, "       %spathlist type <num>%s    - List all paths with the specified type\r\n", QYEL, QNRM);
      send_to_char(ch, "Just type %spathlist types%s to view available path types.\r\n", QYEL, QNRM);
      return;
    }
    else if (is_abbrev(arg, "types"))
    {
      if (!*arg2)
      {
        send_to_char(ch, "Which type of path do you want to list?\r\n");
        send_to_char(ch, "Available types are:\r\n");
        send_to_char(ch, "\t1 - Road\r\n");
        send_to_char(ch, "\t2 - Dirt Road\r\n");
        send_to_char(ch, "\t5 - Water\r\n");
        send_to_char(ch, "\r\n");
        return;
      } /*else {
          perform_region_type_list(ch, arg2);
        }

        if (!*arg2 && is_number(arg))
          perform_region_dist_list(ch, arg);
        else*/
    }
    list_paths(ch);

    break;
  case SCMD_OASIS_REGLIST:
    two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));
    if (!IS_WILDERNESS_VNUM(world[IN_ROOM(ch)].number))
    {
      send_to_char(ch, "This command is only available while in the wilderness.\r\n");
      return;
    }
    if (is_abbrev(arg, "help"))
    {
      send_to_char(ch, "Usage: %sreglist <distance>%s    - List regions within a particular distance\r\n", QYEL, QNRM);
      send_to_char(ch, "       %sreglist type <num>%s    - List all regions with the specified type\r\n", QYEL, QNRM);
      send_to_char(ch, "Just type %sreglist types%s to view available region types.\r\n", QYEL, QNRM);
      return;
    }
    else if (is_abbrev(arg, "types"))
    {
      if (!*arg2)
      {
        send_to_char(ch, "Which type of region do you want to list?\r\n");
        send_to_char(ch, "Available types are:\r\n");
        send_to_char(ch, "\t1 - Geographic\r\n");
        send_to_char(ch, "\t2 - Encounter\r\n");
        send_to_char(ch, "\t3 - Sector Transform\r\n");
        send_to_char(ch, "\t4 - Sector\r\n");
        send_to_char(ch, "\r\n");
        return;
      } /*else {
          perform_region_type_list(ch, arg2);
        }

        if (!*arg2 && is_number(arg))
          perform_region_dist_list(ch, arg);
        else*/
    }
    list_regions(ch);

    break;

  case SCMD_OASIS_MLIST:

    two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

    if (is_abbrev(arg, "help"))
    {
      send_to_char(ch, "Usage: %smlist <zone>%s        - List mobiles in a zone\r\n", QYEL, QNRM);
      send_to_char(ch, "       %smlist <vnum> <vnum>%s - List a range of mobiles by vnum\r\n", QYEL, QNRM);
      send_to_char(ch, "       %smlist level <num>%s   - List all mobiles of a specified level\r\n", QYEL, QNRM);
      send_to_char(ch, "       %smlist flags <num>%s - List all mobiles with flag set\r\n", QYEL, QNRM);
      send_to_char(ch, "Just type %smlist flags%s to view available options.\r\n", QYEL, QNRM);
      return;
    }
    else if (is_abbrev(arg, "level") || is_abbrev(arg, "flags"))
    {
      int i;

      if (!*arg2)
      {
        send_to_char(ch, "Which mobile flag or level do you want to list?\r\n");
        for (i = 0; i < NUM_MOB_FLAGS; i++)
        {
          send_to_char(ch, "%s%2d%s-%s%-14s%s", CCNRM(ch, C_NRM), i, CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), action_bits[i], CCNRM(ch, C_NRM));
          if (!((i + 1) % 4))
            send_to_char(ch, "\r\n");
        }
        send_to_char(ch, "\r\n");
        send_to_char(ch, "Usage: %smlist flags <num>%s\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
        send_to_char(ch, "       %smlist level <num>%s\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
        send_to_char(ch, "Displays mobs with the selected flag, or at the selected level\r\n\r\n");

        return;
      }
      if (is_abbrev(arg, "level"))
        perform_mob_level_list(ch, arg2);
      else
        perform_mob_flag_list(ch, arg2);
    }
    else
      list_mobiles(ch, rzone, vmin, vmax);
    break;
  case SCMD_OASIS_OLIST:
    two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

    if (is_abbrev(arg, "help"))
    {
      send_to_char(ch, "Usage: %solist <zone>%s        - List objects in a zone\r\n", QYEL, QNRM);
      send_to_char(ch, "       %solist <vnum> <vnum>%s - List a range of objects by vnum\r\n", QYEL, QNRM);
      send_to_char(ch, "       %solist <name>%s        - List all named objects with count\r\n", QYEL, QNRM);
      send_to_char(ch, "       %solist type <num>%s    - List all objects of a specified type\r\n", QYEL, QNRM);
      send_to_char(ch, "       %solist affect <num>%s  - List top %d objects with affect\r\n", QYEL, QNRM, MAX_OBJ_LIST);
      send_to_char(ch, "       %solist worn <num>%s    - List all objects worn in the specified location.\r\n", QYEL, QNRM);
      send_to_char(ch, "Just type %solist affect%s, %solist type%s or %solist worn%s to view available options\r\n", QYEL, QNRM, QYEL, QNRM, QYEL, QNRM);
      return;
    }
    else if (is_abbrev(arg, "type") || is_abbrev(arg, "affect") || is_abbrev(arg, "worn"))
    {
      if (is_abbrev(arg, "type"))
      {
        if (!*arg2)
        {
          send_to_char(ch, "Which object type do you want to list?\r\n");
          for (i = 1; i < NUM_ITEM_TYPES; i++)
          {
            send_to_char(ch, "%s%2d%s-%s%-14s%s", QNRM, i, QNRM, QYEL, item_types[i], QNRM);
            if (!(i % 4))
              send_to_char(ch, "\r\n");
          }
          send_to_char(ch, "\r\n");
          send_to_char(ch, "Usage: %solist type <num>%s\r\n", QYEL, QNRM);
          send_to_char(ch, "Displays objects of the selected type.\r\n");

          return;
        }
        perform_obj_type_list(ch, arg2);
      }
      else if (is_abbrev(arg, "worn"))
      {
        if (!*arg2)
        {
          send_to_char(ch, "Which object wear location do you want to list?\r\n");
          for (i = 1; i < NUM_ITEM_WEARS; i++)
          {
            send_to_char(ch, "%s%2d%s-%s%-14s%s", QNRM, i, QNRM, QYEL, wear_bits[i], QNRM);
            if (!(i % 4))
              send_to_char(ch, "\r\n");
          }
          send_to_char(ch, "\r\n");
          send_to_char(ch, "Usage: %solist worn <num>%s\r\n", QYEL, QNRM);
          send_to_char(ch, "Displays objects worn in the selected location.\r\n");

          return;
        }
        perform_obj_worn_list(ch, arg2);
      }
      else
      { /* Assume arg = affect */
        if (!*arg2)
        {
          send_to_char(ch, "Which object affect do you want to list?\r\n");
          for (i = 0; i < NUM_APPLIES; i++)
          {
            if (i == APPLY_CLASS) /* Special Case 1 - Weapon Dam */
              send_to_char(ch, "%s%2d-%s%-14s%s", QNRM, i, QYEL, "Weapon Dam", QNRM);
            else if (i == APPLY_LEVEL) /* Special Case 2 - Armor AC Apply */
              send_to_char(ch, "%s%2d-%s%-14s%s", QNRM, i, QYEL, "AC Apply", QNRM);
            else
              send_to_char(ch, "%s%2d-%s%-14s%s", QNRM, i, QYEL, apply_types[i], QNRM);
            if (!((i + 1) % 4))
              send_to_char(ch, "\r\n");
          }
          send_to_char(ch, "\r\n");
          send_to_char(ch, "Usage: %solist affect <num>%s\r\n", QYEL, QNRM);
          send_to_char(ch, "Displays top %d objects, in order, with the selected affect.\r\n", MAX_OBJ_LIST);

          return;
        }
        perform_obj_aff_list(ch, arg2);
      }
    }
    else if (*arg && !isdigit(*arg))
    {
      perform_obj_name_list(ch, arg);
    }
    else
      list_objects(ch, rzone, vmin, vmax);
    break;
  case SCMD_OASIS_RLIST:
    list_rooms(ch, rzone, vmin, vmax);
    break;
  case SCMD_OASIS_TLIST:
    list_triggers(ch, rzone, vmin, vmax);
    break;
  case SCMD_OASIS_SLIST:
    list_shops(ch, rzone, vmin, vmax);
    break;
  case SCMD_OASIS_QLIST:
    list_quests(ch, rzone, vmin, vmax);
    break;
  case SCMD_OASIS_ZLIST:
    if (!*smin) /* No args - list all zones */
      list_zones(ch, NOWHERE, 0, zone_table[top_of_zone_table].number, NULL);
    else if (use_name) /* Builder name as arg */
      list_zones(ch, NOWHERE, 0, zone_table[top_of_zone_table].number, smin);
    else /* Numerical args */
      list_zones(ch, rzone, vmin, vmax, NULL);
    break;
  default:
    send_to_char(ch, "You can't list that!\r\n");
    mudlog(BRF, LVL_IMMORT, TRUE,
           "SYSERR: do_oasis_list: Unknown list option: %d", subcmd);
  }
}

ACMD(do_oasis_links)
{
  if (!ch)
    return;

  if (IN_ROOM(ch) == NOWHERE)
    return;

  zone_rnum zrnum;
  zone_vnum zvnum;
  room_rnum nr, to_room;
  room_vnum first, last;
  int j;
  char arg[MAX_INPUT_LENGTH];

  skip_spaces_c(&argument);
  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch,
                 "Syntax: links <zone_vnum> ('.' for zone you are standing in)\r\n");
    return;
  }

  if (!strcmp(arg, "."))
  {
    zrnum = world[IN_ROOM(ch)].zone;
    zvnum = zone_table[zrnum].number;
  }
  else
  {
    zvnum = atoi(arg);
    zrnum = real_zone(zvnum);
  }

  if (zrnum == NOWHERE || zvnum == NOWHERE)
  {
    send_to_char(ch, "No zone was found with that number.\n\r");
    return;
  }

  last = zone_table[zrnum].top;
  first = zone_table[zrnum].bot;

  send_to_char(ch, "Zone %d is linked to the following zones:\r\n", zvnum);
  for (nr = 0; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++)
  {
    if (GET_ROOM_VNUM(nr) >= first)
    {
      for (j = 0; j < DIR_COUNT; j++)
      {
        if (world[nr].dir_option[j])
        {
          to_room = world[nr].dir_option[j]->to_room;
          if (to_room != NOWHERE && (zrnum != world[to_room].zone))
            send_to_char(ch, "%3d %-30s%s at %5d (%-5s) ---> %5d\r\n",
                         zone_table[world[to_room].zone].number,
                         zone_table[world[to_room].zone].name, QNRM,
                         GET_ROOM_VNUM(nr), dirs[j], world[to_room].number);
        }
      }
    }
  }
}

/* Helper Functions */

/* List all regions in wilderness. */
static void list_regions(struct char_data *ch)
{
  int i;
  int counter = 0, len;
  char buf[MAX_STRING_LENGTH];

  len = strlcpy(buf,
                "Ind|VNum   | Name                                |Type        |Properties\r\n"
                "--- ------- ------------------------------------- ------------ ---------------\r\n",
                sizeof(buf));
  if (!top_of_region_table)
    return;

  for (i = 0; i <= top_of_region_table; i++)
  {
    counter++;

    len += snprintf(buf + len, sizeof(buf) - len,
                    "%s%3d%s|%s%-7d%s|%s%-37s%s|%s%12s%s|%s%-15s%s\r\n",
                    QGRN, counter, QNRM,
                    QGRN, region_table[i].vnum, QNRM,
                    QYEL, region_table[i].name, QNRM,
                    QYEL, (region_table[i].region_type == 1 ? "Geographic" : (region_table[i].region_type == 2 ? "Encounter" : (region_table[i].region_type == 3 ? "Sect.Transfm" : (region_table[i].region_type == 4 ? "Sector" : "UNKNOWN")))), QNRM,
                    QYEL, (region_table[i].region_type == 4 ? sector_types[region_table[i].region_props] : "[N/A]"), QNRM);

    if (len > sizeof(buf))
      break;
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
}

/* List all paths in wilderness. */
static void list_paths(struct char_data *ch)
{
  int i;
  int counter = 0, len;
  char buf[MAX_STRING_LENGTH];

  len = strlcpy(buf,
                "Ind|VNum   | Name                                |Type        |Glyphs\r\n"
                "--- ------- ------------------------------------- ------------ ---------------\r\n",
                sizeof(buf));
  if (!top_of_path_table)
    return;

  for (i = 0; i <= top_of_path_table; i++)
  {
    counter++;

    len += snprintf(buf + len, sizeof(buf) - len,
                    "%s%3d%s|%s%-7d%s|%s%-37s%s|%s%12s%s|%s%s%s%s%s\r\n",
                    QGRN, counter, QNRM,
                    QGRN, path_table[i].vnum, QNRM,
                    QYEL, path_table[i].name, QNRM,
                    QYEL, (path_table[i].path_type == 1 ? "Road" : (path_table[i].path_type == 2 ? "Dirt Road" : (path_table[i].path_type == 5 ? "Water" : "[UNKNOWN]"))), QNRM,
                    QYEL, path_table[i].glyphs[0], path_table[i].glyphs[1], path_table[i].glyphs[2], QNRM);

    if (len > sizeof(buf))
      break;
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
}

/* List all rooms in a zone. */
static void list_rooms(struct char_data *ch, zone_rnum rnum, room_vnum vmin, room_vnum vmax)
{
  room_rnum i;
  room_vnum bottom, top;
  int j, counter = 0, len, temp_num = 0, subcmd = 0;
  char buf[MAX_STRING_LENGTH];
  bool *has_zcmds = NULL;

  /* Expect a minimum / maximum number if the rnum for the zone is NOWHERE. */
  if (rnum != NOWHERE)
  {
    bottom = zone_table[rnum].bot;
    top = zone_table[rnum].top;
  }
  else
  {
    rnum = real_zone_by_thing(vmin);
    bottom = vmin;
    top = vmax;
  }

  len = strlcpy(buf,
                "Index  VNum    Room Name                                    Exits\r\n"
                "-----  ------- -------------------------------------------- -----\r\n",
                sizeof(buf));

  if (!top_of_world)
    return;

  CREATE(has_zcmds, bool, top - bottom);
  for (subcmd = 0; ZCMD(real_zone_by_thing(bottom), subcmd).command != 'S'; subcmd++)
  {
    switch (ZCMD(real_zone_by_thing(bottom), subcmd).command)
    {
    case 'D':
    case 'R':
      temp_num = GET_ROOM_VNUM(ZCMD(real_zone_by_thing(bottom), subcmd).arg1);
      // send_to_char(ch, "D/R subcmd: %d\r\n", temp_num);
      break;
    case 'O':
    case 'M':
      temp_num = GET_ROOM_VNUM(ZCMD(real_zone_by_thing(bottom), subcmd).arg3);
      // send_to_char(ch, "O/M subcmd: %d\r\n", temp_num);
      break;
    }
    if (temp_num >= bottom && temp_num <= top)
      has_zcmds[temp_num - bottom] = TRUE;
  }

  for (i = 0; i <= top_of_world; i++)
  {
    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((world[i].number >= bottom) && (world[i].number <= top))
    {
      counter++;

      len += snprintf(buf + len, sizeof(buf) - len, "%4d)%s%s%s [%s%-5d%s] %s%-*s%s %s",
                      counter, QYEL, (has_zcmds != NULL ? (has_zcmds[world[i].number - bottom] == TRUE ? "Z" : " ") : ""), QNRM,
                      QGRN, world[i].number, QNRM,
                      QCYN, count_color_chars(world[i].name) + 44, world[i].name, QNRM,
                      world[i].proto_script ? "[TRIG] " : "");

      for (j = 0; j < DIR_COUNT; j++)
      {
        if (W_EXIT(i, j) == NULL)
          continue;
        if (W_EXIT(i, j)->to_room == NOWHERE)
          continue;

        if (world[W_EXIT(i, j)->to_room].zone != world[i].zone)
          len += snprintf(buf + len, sizeof(buf) - len, "(%s%d%s)", QYEL, world[W_EXIT(i, j)->to_room].number, QNRM);
      }

      len += snprintf(buf + len, sizeof(buf) - len, "\r\n");

      if (len > sizeof(buf))
        break;

      /* still having issues with overflow, guessing it is a miscalculation with color codes -zusuk */
      if (counter >= 200)
      {
        len += snprintf(buf + len, sizeof(buf) - len, "\r\n OVERFLOW, use a range to view the rest! \r\n");
        break;
      }
    }
  }

  if (has_zcmds)
    free(has_zcmds);

  if (counter == 0)
    send_to_char(ch, "No rooms found for zone/range specified.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
}

/* List all mobiles in a zone. */
static void list_mobiles(struct char_data *ch, zone_rnum rnum, mob_vnum vmin, mob_vnum vmax)
{
  mob_rnum i;
  mob_vnum bottom, top;
  int counter = 0, len;
  char buf[MAX_STRING_LENGTH];

  if (rnum != NOWHERE)
  {
    bottom = zone_table[rnum].bot;
    top = zone_table[rnum].top;
  }
  else
  {
    bottom = vmin;
    top = vmax;
  }

  len = strlcpy(buf,
                "Ind|VNum   |Lv|T|Al|Rac|Cls|E|Mobile Name                                 \r\n"
                "--- ------- -- - -- --- --- - ------------------------------------------- \r\n",
                sizeof(buf));
  if (!top_of_mobt)
    return;

  for (i = 0; i <= top_of_mobt; i++)
  {
    if (mob_index[i].vnum >= bottom && mob_index[i].vnum <= top)
    {
      counter++;

      /* original
      len += snprintf(buf + len, sizeof(buf) - len,
              "%s%4d%s) [%s%-5d%s] %s%-*s %s[%4d]%s%s\r\n",
                   QGRN, counter, QNRM,
                   QGRN, mob_index[i].vnum, QNRM,
                   QCYN, count_color_chars(mob_proto[i].player.short_descr)+44,
                   mob_proto[i].player.short_descr,
                   QYEL, mob_proto[i].player.level, QNRM,
                   mob_proto[i].proto_script ? " [TRIG]" : ""
                   );*/
      len += snprintf(buf + len, sizeof(buf) - len,
                      "%s%3d%s|%s%-7d%s|%s%2d%s|%s|%s|%s|%s|%s|%s%-*s %s\r\n",
                      QGRN, counter, QNRM,
                      QGRN, mob_index[i].vnum, QNRM,
                      QYEL, mob_proto[i].player.level, QNRM,
                      mob_proto[i].proto_script ? "\tRY\tn" : "N",
                      get_align_by_num_cnd(mob_proto[i].char_specials.saved.alignment),
                      race_family_short[mob_proto[i].player.race],
                      IS_SET_AR(mob_proto[i].char_specials.saved.act, MOB_NOCLASS) ? "---" : CLSLIST_ABBRV(mob_proto[i].player.chclass),
                      mob_proto[i].mob_specials.echo_count > 0 ? "\tRY\tn" : "N",
                      QCYN, count_color_chars(mob_proto[i].player.short_descr) + 44,
                      mob_proto[i].player.short_descr, QNRM);

      if (len > sizeof(buf))
        break;
    }
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
}

/* List all objects in a zone. */
static void list_objects(struct char_data *ch, zone_rnum rnum, obj_vnum vmin, obj_vnum vmax)
{
  obj_rnum i = 0, j = 0;
  obj_vnum bottom = 0, top = 0;
  char buf[MAX_STRING_LENGTH];
  int counter = 0, num_found = 0, len = 0, len2 = 0;
  struct obj_data *l = NULL;
  char wears_text[LONG_STRING];

  if (rnum != NOWHERE)
  {
    bottom = zone_table[rnum].bot;
    top = zone_table[rnum].top;
  }
  else
  {
    bottom = vmin;
    top = vmax;
  }

  len = strlcpy(buf,
                "Index VNum    #  D Object Name                                              Object Type        Cost     Specific Type\r\n"
                "----- ------- -- - -------------------------------------------------------- -----------------  -------- -------------------------\r\n",
                sizeof(buf));

  if (!top_of_objt)
    return;

  /* "i" will be the real-number */
  for (i = 0; i <= top_of_objt; i++)
  {

    /* establish our range */
    if (obj_index[i].vnum >= bottom && obj_index[i].vnum <= top)
    {
      counter++;

      /* find how many of the same objects are in the game currently */
      for (num_found = 0, l = object_list; l; l = l->next)
      {
        if (CAN_SEE_OBJ(ch, l) && GET_OBJ_RNUM(l) == i)
        {
          num_found++;
        }
      }

      len2 = strlcpy(wears_text, "\ty", sizeof(wears_text));
      if (obj_proto[i].obj_flags.type_flag == ITEM_WORN)
      {
        for (j = 1; j < NUM_ITEM_WEARS; j++)
        {
          if (IS_SET_AR(obj_proto[i].obj_flags.wear_flags, j))
            len2 += snprintf(wears_text + len2, sizeof(wears_text) - len2, "%s ", wear_bits[j]);
        }
      }

      len += snprintf(buf + len, sizeof(buf) - len, "%s%4d%s) %s%-7d%s %2d %s %s%-*s %s[%-14s]%s %8d %s%s%s %s\r\n",
                      QGRN, counter, QNRM, QGRN, obj_index[i].vnum, QNRM, num_found,
                      (!obj_proto[i].ex_description ? "\tRN\tn" : "\tWY\tn"),
                      QCYN, count_color_chars(obj_proto[i].short_description) + 58,
                      obj_proto[i].short_description, QYEL,
                      item_types[obj_proto[i].obj_flags.type_flag], QNRM,
                      obj_proto[i].obj_flags.cost, QYEL,
                      obj_proto[i].obj_flags.type_flag == ITEM_WORN ? wears_text : (obj_proto[i].obj_flags.type_flag == ITEM_ARMOR ? armor_list[obj_proto[i].obj_flags.value[1]].name : (obj_proto[i].obj_flags.type_flag == ITEM_WEAPON ? weapon_list[obj_proto[i].obj_flags.value[0]].name : "")),
                      QNRM, obj_proto[i].proto_script ? " [TRIG]" : "");

      if (len > sizeof(buf))
        break;
    }
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
}

/* List all shops in a zone. */
static void list_shops(struct char_data *ch, zone_rnum rnum, shop_vnum vmin, shop_vnum vmax)
{
  shop_rnum i;
  shop_vnum bottom, top;
  int j, counter = 0;
  // mob_vnum mob_vnum = NOBODY;
  // struct char_data *mob = NULL;

  if (rnum != NOWHERE)
  {
    bottom = zone_table[rnum].bot;
    top = zone_table[rnum].top;
  }
  else
  {
    bottom = vmin;
    top = vmax;
  }

  send_to_char(ch,
               "Index VNum    RNum    Mob Name and Shop Room(s)\r\n"
               "----- ------- ------- -----------------------------------------\r\n");

  for (i = 0; i <= top_shop; i++)
  {
    if (SHOP_NUM(i) >= bottom && SHOP_NUM(i) <= top)
    {
      counter++;

      /* determine shopkeeper information -zusuk */
      //      if (SHOP_KEEPER(i) > -1 && SHOP_KEEPER(i) < top_of_mobt) {
      // mob_vnum = mob_index[SHOP_KEEPER(i)].vnum;
      /*
        if (!(mob = read_mobile(SHOP_KEEPER(i), REAL))) {
          send_to_char(ch, "Mob data possibly corrupt, please notify a coder.\r\n");
          mudlog(BRF, LVL_IMMORT, TRUE,
                 "SYSERR: list_shops() - unable to load mobile");
        }
       */
      //      }
      /*
      if (mob)
        char_to_room(mob, 0);
       */

      /* the +1 is strange but fits the rest of the shop code */
      send_to_char(ch, "%s%4d%s) [%s%-5d%s] [%s%-5d%s] %s%s",
                   QGRN, counter, QNRM, QGRN, SHOP_NUM(i), QNRM, QGRN, i + 1, QNRM,
                   (SHOP_KEEPER(i) < top_of_mobt) ? mob_proto[SHOP_KEEPER(i)].player.short_descr : "ERR!", QNRM);

      /* get rid of mob */
      /*
      if (mob)
        extract_char(mob);
       */

      /* Thanks to Ken Ray for this display fix. -Welcor */
      for (j = 0; SHOP_ROOM(i, j) != NOWHERE; j++)
        send_to_char(ch, "%s%s[%s%-5d%s]%s",
                     ((j > 0) && (j % 6 == 0)) ? "\r\n                      " : " ",
                     QCYN, QYEL, SHOP_ROOM(i, j), QCYN, QNRM);

      if (j == 0)
        send_to_char(ch, " %sNone.%s", QCYN, QNRM);

      send_to_char(ch, "\r\n");
    }
  }

  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

/* List all zones in the world (sort of like 'show zones'). */
static void list_zones(struct char_data *ch, zone_rnum rnum, zone_vnum vmin, zone_vnum vmax, char *name)
{
  int counter = 0, len = 0, tmp_len = 0;
  zone_rnum i;
  zone_vnum bottom, top;
  char buf[MAX_STRING_LENGTH];
  char *buf2;
  bool use_name = FALSE;

  bottom = vmin;
  top = vmax;

  if (rnum != NOWHERE)
  {
    /* Only one parameter was supplied - just list that zone */
    print_zone(ch, zone_table[rnum].number);
    return;
  }
  else if (name && *name)
  {
    use_name = TRUE;
    if (!vmin)
      bottom = zone_table[0].number; /* Lowest Zone  */
    if (!vmax)
      top = zone_table[top_of_zone_table].number; /* Highest Zone */
  }

  len = snprintf(buf, sizeof(buf),
                 "VNum  Zone Name                  Status    Builder(s)\r\n"
                 "----- -------------------------- ------ -------------------------------\r\n");

  len += snprintf(buf + len, sizeof(buf) - len, "NOTE:  <*> Means Reserved, See HELP RESERVED\r\n");

  if (!top_of_zone_table)
    return;

  for (i = 0; i <= top_of_zone_table; i++)
  {
    if (zone_table[i].number >= bottom && zone_table[i].number <= top)
    {
      if ((!use_name) || (is_name(name, zone_table[i].builders)))
      {

        /* status display added for head builder */
        if (ZONE_FLAGGED(i, ZONE_CLOSED))
          buf2 = strdup("\trIncomp\tn");
        else if (ZONE_FLAGGED(i, ZONE_GRID))
          buf2 = strdup("\tGReady\tn ");
        else
          buf2 = strdup("\tmN-Reva\tn");

        tmp_len = snprintf(buf + len, sizeof(buf) - len, "[%s%3d%s] %s%-*s %s %s%-1s%s\r\n",
                           QGRN, zone_table[i].number, QNRM, QCYN,
                           count_color_chars(zone_table[i].name) + 26, zone_table[i].name,
                           buf2, QYEL, zone_table[i].builders ? zone_table[i].builders : "None.", QNRM);
        len += tmp_len;
        counter++;
      }
    }
  }

  if (!counter)
    send_to_char(ch, "  None found within those parameters.\r\n");
  else
    page_string(ch->desc, buf, TRUE);
}

/* Prints all of the zone information for the selected zone. */
void print_zone(struct char_data *ch, zone_vnum vnum)
{
  zone_rnum rnum;
  int size_rooms, size_objects, size_mobiles, size_quests, size_shops, size_trigs, i, largest_table;
  room_vnum top, bottom;
  char buf[MAX_STRING_LENGTH];

  if ((rnum = real_zone(vnum)) == NOWHERE)
  {
    send_to_char(ch, "Zone #%d does not exist in the database.\r\n", vnum);
    return;
  }

  /* Locate the largest of the three, top_of_world, top_of_mobt, or top_of_objt. */
  if (top_of_world >= top_of_objt && top_of_world >= top_of_mobt)
    largest_table = top_of_world;
  else if (top_of_objt >= top_of_mobt && top_of_objt >= top_of_world)
    largest_table = top_of_objt;
  else
    largest_table = top_of_mobt;

  /* Initialize some of the variables. */
  size_rooms = 0;
  size_objects = 0;
  size_mobiles = 0;
  size_shops = 0;
  size_trigs = 0;
  size_quests = 0;
  top = zone_table[rnum].top;
  bottom = zone_table[rnum].bot;

  for (i = 0; i <= largest_table; i++)
  {
    if (i <= top_of_world)
      if (world[i].zone == rnum)
        size_rooms++;

    if (i <= top_of_objt)
      if (obj_index[i].vnum >= bottom && obj_index[i].vnum <= top)
        size_objects++;

    if (i <= top_of_mobt)
      if (mob_index[i].vnum >= bottom && mob_index[i].vnum <= top)
        size_mobiles++;
  }
  for (i = 0; i <= top_shop; i++)
    if (SHOP_NUM(i) >= bottom && SHOP_NUM(i) <= top)
      size_shops++;

  for (i = 0; i < top_of_trigt; i++)
    if (trig_index[i]->vnum >= bottom && trig_index[i]->vnum <= top)
      size_trigs++;

  size_quests = count_quests(bottom, top);
  sprintbitarray(zone_table[rnum].zone_flags, zone_bits, ZN_ARRAY_MAX, buf);

  /* Display all of the zone information at once. */
  send_to_char(ch,
               "%sVirtual Number = %s%d\r\n"
               "%sName of zone   = %s%s\r\n"
               "%sBuilders       = %s%s\r\n"
               "%sLifespan       = %s%d\r\n"
               "%sShow Weather   = %s%d\r\n"
               "%sAge            = %s%d\r\n"
               "%sBottom of Zone = %s%d\r\n"
               "%sTop of Zone    = %s%d\r\n"
               "%sReset Mode     = %s%s\r\n"
               "%sZone Flags     = %s%s\r\n"
               "%sMin Level      = %s%d\r\n"
               "%sMax Level      = %s%d\r\n"
               "%sSize\r\n"
               "%s   Rooms       = %s%d\r\n"
               "%s   Objects     = %s%d\r\n"
               "%s   Mobiles     = %s%d\r\n"
               "%s   Shops       = %s%d\r\n"
               "%s   Triggers    = %s%d\r\n"
               "%s   Quests      = %s%d%s\r\n",
               QGRN, QCYN, zone_table[rnum].number,
               QGRN, QCYN, zone_table[rnum].name,
               QGRN, QCYN, zone_table[rnum].builders,
               QGRN, QCYN, zone_table[rnum].lifespan,
               QGRN, QCYN, zone_table[rnum].show_weather,
               QGRN, QCYN, zone_table[rnum].age,
               QGRN, QCYN, zone_table[rnum].bot,
               QGRN, QCYN, zone_table[rnum].top,
               QGRN, QCYN, zone_table[rnum].reset_mode ? ((zone_table[rnum].reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.") : "Never reset",
               QGRN, QCYN, buf,
               QGRN, QCYN, zone_table[rnum].min_level,
               QGRN, QCYN, zone_table[rnum].max_level,
               QGRN,
               QGRN, QCYN, size_rooms,
               QGRN, QCYN, size_objects,
               QGRN, QCYN, size_mobiles,
               QGRN, QCYN, size_shops,
               QGRN, QCYN, size_trigs,
               QGRN, QCYN, size_quests, QNRM);
}

/* List code by Ronald Evers. */
static void list_triggers(struct char_data *ch, zone_rnum rnum, trig_vnum vmin, trig_vnum vmax)
{
  int i, bottom, top, counter = 0;
  char trgtypes[MEDIUM_STRING];

  /* Expect a minimum / maximum number if the rnum for the zone is NOWHERE. */
  if (rnum != NOWHERE)
  {
    bottom = zone_table[rnum].bot;
    top = zone_table[rnum].top;
  }
  else
  {
    bottom = vmin;
    top = vmax;
  }

  /* Store the header for the room listing. */
  send_to_char(ch,
               "Index VNum    Trigger Name                                  Type\r\n"
               "----- ------- --------------------------------------------- ---------\r\n");

  /* Loop through the world and find each room. */
  for (i = 0; i < top_of_trigt; i++)
  {
    /** Check to see if this room is one of the ones needed to be listed.    **/
    if ((trig_index[i]->vnum >= bottom) && (trig_index[i]->vnum <= top))
    {
      counter++;

      send_to_char(ch, "%4d) [%s%5d%s] %s%-45.45s%s ",
                   counter, QGRN, trig_index[i]->vnum, QNRM, QCYN, trig_index[i]->proto->name, QNRM);

      if (trig_index[i]->proto->attach_type == OBJ_TRIGGER)
      {
        sprintbit(GET_TRIG_TYPE(trig_index[i]->proto), otrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "obj %s%s%s\r\n", QYEL, trgtypes, QNRM);
      }
      else if (trig_index[i]->proto->attach_type == WLD_TRIGGER)
      {
        sprintbit(GET_TRIG_TYPE(trig_index[i]->proto), wtrig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "wld %s%s%s\r\n", QYEL, trgtypes, QNRM);
      }
      else
      {
        sprintbit(GET_TRIG_TYPE(trig_index[i]->proto), trig_types, trgtypes, sizeof(trgtypes));
        send_to_char(ch, "mob %s%s%s\r\n", QYEL, trgtypes, QNRM);
      }
    }
  }

  if (counter == 0)
  {
    if (rnum == NOWHERE)
      send_to_char(ch, "No triggers found from %d to %d\r\n", vmin, vmax);
    else
      send_to_char(ch, "No triggers found for zone #%d\r\n", zone_table[rnum].number);
  }
}
