/**************************************************************************
 *  File: obj/vendor.c                                  Part of LuminariMUD *
 *  Usage: Commerce and item-service special procedures.                   *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "constants.h"
#include "act.h"
#include "vendor.h"
#include "spec_procs.h"
#include "character/class.h"
#include "combat/fight.h"
#include "modify.h"
#include "obj/house.h"
#include "clan.h"
#include "mudlim.h"
#include "graph.h"
#include "dgscript/dg_scripts.h"
#include "mud_event.h"
#include "actions.h"
#include "combat/assign_wpn_armor.h"
#include "magic/domains_schools.h"
#include "character/feats.h"
#include "magic/spell_prep.h"
#include "obj/item.h"
#include "craft/alchemy.h"
#include "obj/treasure.h"
#include "mob/mob_utils.h"
#include "character/evolutions.h"
#include "olc/oasis.h"
#include "quest/quest.h"
#include "character/backgrounds.h"
#include "character/perks.h"
#include "spec/spec_dispatch.h"

SPECIAL(bank)
{
  (void)ch;
  (void)me;
  (void)cmd;
  (void)argument;

  log("SYSERR: Typed Bank adapter invoked outside a special-procedure gateway.");
  return FALSE;
}

int bank_typed(struct spec_event_context *context)
{
  struct char_data *ch;
  const char *argument;
  int amount;
  int cmd;

  ch = context->actor;
  argument = context->argument;
  cmd = context->command;

  if (context->event == SPEC_EVENT_ITEM_IDENTIFY)
  {
    send_to_char(ch, "This appears to be a bank.\r\n");
    return TRUE;
  }
  if (context->event != SPEC_EVENT_COMMAND)
    return FALSE;

  if (CMD_IS("balance"))
  {
    if (GET_BANK_GOLD(ch) > 0)
      send_to_char(ch, "\twYour current balance is \tW%d \tYcoins\tw.\tn\r\n", GET_BANK_GOLD(ch));
    else
      send_to_char(ch, "\twYou currently have \tWno\tw money deposited.\tn\r\n");
    return (TRUE);
  }
  else if (CMD_IS("deposit"))
  {
    /* code to accomdate "all" */
    skip_spaces_c(&argument);
    if (is_abbrev(argument, "all"))
    {
      amount = GET_GOLD(ch);
      send_to_char(ch, "\twYou deposit all (\tW%d\tw) your \tYcoins\tw.\tn\r\n", amount);
      act("$n makes a bank transaction.", TRUE, ch, 0, NULL, TO_ROOM);
      decrease_gold(ch, amount);
      increase_bank(ch, amount);
      return (TRUE);
    }

    if ((amount = atoi(argument)) <= 0)
    {
      send_to_char(ch, "How much do you want to deposit?\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < amount)
    {
      send_to_char(ch, "You don't have that many coins!\r\n");
      return (TRUE);
    }
    decrease_gold(ch, amount);
    increase_bank(ch, amount);
    send_to_char(ch, "\twYou deposit \tW%d\tY coins\tw.\tn\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, NULL, TO_ROOM);
    return (TRUE);
  }
  else if (CMD_IS("withdraw"))
  {
    /* code to accomdate "all" */
    skip_spaces_c(&argument);
    if (is_abbrev(argument, "all"))
    {
      amount = GET_BANK_GOLD(ch);
      send_to_char(ch, "\twYou withdraw all (\tW%d\tw) your \tYcoins\tw.\tn\r\n", amount);
      act("$n makes a bank transaction.", TRUE, ch, 0, NULL, TO_ROOM);
      increase_gold(ch, amount);
      decrease_bank(ch, amount);
      return (TRUE);
    }

    if ((amount = atoi(argument)) <= 0)
    {
      send_to_char(ch, "How much do you want to withdraw?\r\n");
      return (TRUE);
    }
    if (GET_BANK_GOLD(ch) < amount)
    {
      send_to_char(ch, "You don't have that many coins deposited!\r\n");
      return (TRUE);
    }
    increase_gold(ch, amount);
    decrease_bank(ch, amount);
    send_to_char(ch, "\twYou withdraw \tW%d \tYcoins\tw.\tn\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, NULL, TO_ROOM);
    return (TRUE);
  }
  else
    return (FALSE);
}


/* from homeland, converts an object type PET into an actual
 * pet mobile follower, object vnum must match mobile vnum */
SPECIAL(bought_pet)
{
  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a pet.\r\n");
    return TRUE;
  }

  struct obj_data *obj = (struct obj_data *)me;

  if (obj->carried_by == 0)
    return FALSE;

  if (IS_NPC(obj->carried_by))
    return FALSE;

  struct char_data *pet = NULL;

  // if (check_npc_followers(ch, NPC_MODE_SPARE, 0) <= 0)
  if (!can_add_follower(ch, GET_OBJ_VNUM(obj)))
  {
    send_to_char(ch, "Sorry, you already have enough followers.\r\n");
    return FALSE;
  }

  pet = read_mobile(GET_OBJ_VNUM(obj), VIRTUAL);

  /* found matching vnum for obejct, loaded pet succesfully */
  if (pet)
  {
    if (ZONE_FLAGGED(GET_ROOM_ZONE(obj->carried_by->in_room), ZONE_WILDERNESS))
    {
      X_LOC(pet) = world[obj->carried_by->in_room].coords[0];
      Y_LOC(pet) = world[obj->carried_by->in_room].coords[1];
    }

    char_to_room(pet, obj->carried_by->in_room);
    add_follower(pet, obj->carried_by);
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

    /* success message */
    send_to_char(obj->carried_by, "You have acquired a companion.\r\n");

    /* get rid of the purchased object */
    obj_from_char(obj);
    extract_obj(obj);

    /* bingo */
    return TRUE;
  }

  /* clean up */
  if (obj && obj->carried_by && !IS_NPC(obj->carried_by))
  {
    obj_from_char(obj);
    extract_obj(obj);
  }

  /* failed to load pet */
  return FALSE;
}


int get_vendor_armor_cost(struct char_data *ch, int level, int armortype, sbyte masterwork)
{
  int cost = 0;

  cost += armor_list[armortype].cost;

  if (masterwork)
    cost += 200;

  switch (level)
  {
  case 1:
    cost += 5000;
    break;
  case 2:
    cost += 10000;
    break;
  case 3:
    cost += 20000;
    break;
  case 4:
    cost += 50000;
    break;
  }

  if (armor_list[armortype].armorType != ARMOR_TYPE_SHIELD &&
      armor_list[armortype].armorType != ARMOR_TYPE_TOWER_SHIELD)
    cost /= 4;

  int mod = MIN(50, GET_CHA(ch) + compute_ability(ch, ABILITY_APPRAISE));
  cost = (cost * 100) / (100 + mod);

  return MAX(1, cost);
}

int get_vendor_weapon_cost(struct char_data *ch, int level, int weapontype, sbyte masterwork)
{
  int cost = 0;

  cost += weapon_list[weapontype].cost;

  if (masterwork)
    cost += 300;

  switch (level)
  {
  case 1:
    cost += 5000;
    break;
  case 2:
    cost += 10000;
    break;
  case 3:
    cost += 20000;
    break;
  case 4:
    cost += 50000;
    break;
  }

  int mod = MIN(50, GET_CHA(ch) + compute_ability(ch, ABILITY_APPRAISE));
  cost = (cost * 100) / (100 + mod);

  return MAX(1, cost);
}

void display_buy_armor_types(struct char_data *ch, int level, sbyte masterwork, char *type)
{
  int i = 0;
  int cost = 0;
  int wear = ITEM_WEAR_TAKE;
  char wear_str[50];

  if (is_abbrev(type, "body"))
  {
    wear = ITEM_WEAR_BODY;
    snprintf(wear_str, sizeof(wear_str), "BODY");
  }
  else if (is_abbrev(type, "arms"))
  {
    wear = ITEM_WEAR_ARMS;
    snprintf(wear_str, sizeof(wear_str), "ARMS");
  }
  else if (is_abbrev(type, "legs"))
  {
    wear = ITEM_WEAR_LEGS;
    snprintf(wear_str, sizeof(wear_str), "LEGS");
  }
  else if (is_abbrev(type, "head"))
  {
    wear = ITEM_WEAR_HEAD;
    snprintf(wear_str, sizeof(wear_str), "HEAD");
  }
  else if (is_abbrev(type, "shield"))
  {
    wear = ITEM_WEAR_SHIELD;
    snprintf(wear_str, sizeof(wear_str), "SHIELD");
  }
  else
  {
    send_to_char(ch, "Please specify body, arms, legs, head or shield.\r\n");
    return;
  }

  send_to_char(ch, "%s\r\n", wear_str);
  for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
  {
    if (armor_list[i].wear != wear)
      continue;
    cost = get_vendor_armor_cost(ch, level, i, masterwork);
    if (armor_list[i].armorType == ARMOR_TYPE_SHIELD ||
        armor_list[i].armorType == ARMOR_TYPE_TOWER_SHIELD)
    {
      send_to_char(ch, "%-25s ", armor_list[i].name);
      send_to_char(ch, " %d gold\r\n", MAX(1, cost));
    }
    else
    {
      send_to_char(ch, "%-25s ", armor_list[i].name);
      send_to_char(ch, " %d gold each\r\n", MAX(1, cost));
    }
  }
  if (level > 0)
    send_to_char(ch, "These prices are for +%d items.\r\n\r\n", level);
}

void display_buy_weapon_types(struct char_data *ch, int level, sbyte masterwork)
{
  int i = 0, cost = 0;

  for (i = 2; i < NUM_WEAPON_TYPES; i++)
  {
    cost = get_vendor_weapon_cost(ch, level, i, masterwork);
    send_to_char(ch, "%25s %6d gold ", weapon_list[i].name, cost);
    if ((i % 2) == 1)
      send_to_char(ch, "\r\n");
  }
  if ((i % 2) != 1)
    send_to_char(ch, "\r\n");
  if (level > 0)
    send_to_char(ch, "These prices are for +%d items.\r\n\r\n", level);
}

#define MASTERWORK_MSG                                                                             \
  "Please specify whether you prefer mundane or masterwork items.\r\n"                             \
  "Mundane items provide no bonuses.\r\n"                                                          \
  "Masterwork weapons provide +1 to attack roll, but not damage. They cost an extra 300 gold.\r\n" \
  "Masterwork armor reduces the armor check penalty for certain skills, by one.  They cost an "    \
  "extra 50 gold per piece.\r\n"

void set_weapon_name(struct obj_data *obj, int type)
{
  char buf[200];

  snprintf(buf, sizeof(buf), "%s %s", AN(weapon_list[type].name), weapon_list[type].name);
  obj->short_description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s %s lies here.", AN(weapon_list[type].name),
           weapon_list[type].name);
  obj->description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s", weapon_list[type].name);
  obj->name = strdup(buf);
}

void set_armor_name(struct obj_data *obj, int type)
{
  char buf[200];

  snprintf(buf, sizeof(buf), "%s %s", AN(armor_list[type].name), armor_list[type].name);
  obj->short_description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s %s lies here.", AN(armor_list[type].name), armor_list[type].name);
  obj->description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s", armor_list[type].name);
  obj->name = strdup(buf);
}

void set_masterwork_obj_name(struct obj_data *obj)
{
  char buf[200];

  snprintf(buf, sizeof(buf), "%s (masterwork)", obj->short_description);
  if (obj->short_description)
    free(obj->short_description);
  obj->short_description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s (masterwork)", obj->description);
  if (obj->description)
    free(obj->description);
  obj->description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s masterwork", obj->name);
  if (obj->name)
    free(obj->name);
  obj->name = strdup(buf);
}

void set_magical_obj_name(struct obj_data *obj, int level)
{
  char buf[200];
  int i = 0;

  snprintf(buf, sizeof(buf), "%s (+%d)", obj->short_description, level);
  if (obj->short_description)
    free(obj->short_description);
  obj->short_description = strdup(buf);

  snprintf(buf, sizeof(buf), "%s (+%d)", obj->description, level);
  if (obj->description)
    free(obj->description);
  obj->description = strdup(buf);

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
  {
    snprintf(buf, sizeof(buf), "%s", weapon_list[GET_OBJ_VAL(obj, 0)].name);
    for (i = 0; (size_t)i < strlen(buf); i++)
      if (buf[i] == ' ')
        buf[i] = '-';
    char res_buf[128];
    snprintf(res_buf, sizeof(res_buf), " %s +%d", obj->name, level);
    strlcat(buf, res_buf, sizeof(buf));
  }
  else
  {
    snprintf(buf, sizeof(buf), "%s +%d", obj->name, level);
  }
  if (obj->name)
    free(obj->name);
  obj->name = strdup(buf);
}

SPECIAL(buyarmor)
{
  if (!CMD_IS("buy") && !CMD_IS("list"))
    return 0;

  struct char_data *keeper = (struct char_data *)me;
  int level = 0;
  char arg1
      [100], // masterwork or mundane? if level is zero, if level > 0, then it's the armor's name
             // which we'll copy into arg2 so we don't need to mess around with 2 variables, since
             // vendors level 1 and up ONLY sell weapons of their level bonus
      arg2[100]; // name of the armor desired

  half_chop(argument, arg1, arg2);

  if (GET_LEVEL(keeper) <= 10)
    level = 0;
  else if (GET_LEVEL(keeper) <= 15)
    level = 1;
  else if (GET_LEVEL(keeper) <= 20)
    level = 2;
  else if (GET_LEVEL(keeper) <= 25)
    level = 3;
  else if (GET_LEVEL(keeper) <= 30)
    level = 4;

  if (CMD_IS("list"))
  {
    if (!*arg1)
    {
      if (level == 0)
      {
        send_to_char(ch, MASTERWORK_MSG);
        return 1;
      }
    }
    if (!*arg2 && level == 0)
    {
      send_to_char(ch, "Please specify body, arms, legs, head or shield.\r\n");
      return 1;
    }
    if (level == 0 && (!is_abbrev(arg1, "mundane") && !is_abbrev(arg1, "masterwork")))
    {
      send_to_char(ch, MASTERWORK_MSG);
      return 1;
    }
    display_buy_armor_types(ch, level, level == 0 ? !is_abbrev(arg1, "mundane") : false,
                            level == 0 ? strdup(arg2) : strdup(arg1));
    return 1;
  }

  if (!*argument)
  {
    if (level == 0)
      send_to_char(ch, MASTERWORK_MSG);
    else
      send_to_char(ch, "Please specify the type of magical armor/shield you want to purchase.\r\n");
    return 1;
  }

  if (!*arg1)
  {
    if (level == 0)
      send_to_char(ch, MASTERWORK_MSG);
    else
      send_to_char(ch, "Please specify the type of magical armor/shield you want to purchase.\r\n");
    return 1;
  }

  if (!*arg2 && level == 0)
  {
    send_to_char(
        ch,
        "Please specify which armor piece you wish to buy.\r\n"
        "Type buy (mundane|masterwork) (full name of armor piece/shield)\r\n"
        "A list can be seen by typing: list (mundane|masterwork) (body|arms|legs|head|shield)\r\n");
    send_to_char(ch, "Masterwork armor costs 50 gold more per piece, or 200 gold more for shields "
                     "and bucklers.\r\n");
    return 1;
  }

  if (level == 0 && (!is_abbrev(arg1, "mundane") && !is_abbrev(arg1, "masterwork")))
  {
    send_to_char(ch, MASTERWORK_MSG);
    return 1;
  }

  if (level > 0 && *argument)
  { // let's just work with arg2 to keep things easy
    skip_spaces(&argument);
    snprintf(arg2, sizeof(arg2), "%s", argument);
  }

  int i = 0, cost = 0;

  for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
  {
    if (!strcmp(arg2, armor_list[i].name))
      break;
  }

  if (i >= NUM_SPEC_ARMOR_TYPES)
  {
    send_to_char(ch, "Please specify which armor piece you wish to buy.  A list can be seen by "
                     "typing: list (body|arms|legs|head|shield)\r\n");
    send_to_char(ch, "Masterwork armor costs 50 gold more per piece, or 200 gold more for shields "
                     "and bucklers.\r\n");
    send_to_char(ch, "\r\nYou must specify the exact, full name of the armor you wish to buy, in "
                     "lowercase.\r\n");
    return 1;
  }

  sbyte mundane = TRUE;

  // We want mundane to be the default since we're using is_abbrev and they both start with "m"
  if (!is_abbrev(arg1, "mundane"))
  {
    mundane = FALSE;
  }

  cost = get_vendor_armor_cost(ch, level, i, !mundane);

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d gold to buy %s, but you only have %d.\r\n", cost,
                 armor_list[i].name, GET_GOLD(ch));
    return 1;
  }

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
  {
    act("You can't carry any more items.", FALSE, ch, 0, 0, TO_CHAR);
    return (1);
  }

  struct obj_data *obj = NULL;
  obj_vnum base_vnum = 0;

  if (armor_list[i].armorType == ARMOR_TYPE_SHIELD ||
      armor_list[i].armorType == ARMOR_TYPE_TOWER_SHIELD)
  {
    base_vnum = 61;
  }
  else
  {
    switch (i % 4)
    {
    case 0:
      base_vnum = 55;
      break;
    case 1:
      base_vnum = 56;
      break;
    case 2:
      base_vnum = 60;
      break;
    case 3:
      base_vnum = 57;
      break;
    }
  }

  if ((obj = read_object(base_vnum, VIRTUAL)) == NULL)
  {
    send_to_char(ch,
                 "There seems to be an error in purchasing %s.  Please inform a staff member.\r\n",
                 armor_list[i].name);
    return 1;
  }

  set_armor_object(obj, i);
  GET_OBJ_COST(obj) = cost;
  set_armor_name(obj, i);
  if (!mundane && level == 0)
  {
    set_masterwork_obj_name(obj);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MASTERWORK);
  }
  if (level > 0)
  {
    set_magical_obj_name(obj, level);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MASTERWORK);
    GET_OBJ_VAL(obj, 4) = level; // Enhancement Bonus
  }

  GET_GOLD(ch) -= cost;
  obj_to_char(obj, ch);
  send_to_char(ch, "You purchase %s for %d gold coins.\r\n", obj->short_description, cost);

  return 1;
}

#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH] = {'\0'}, pet_name[MEDIUM_STRING] = {'\0'};
  room_rnum pet_room;
  struct char_data *pet;

  /* Gross. */
  pet_room = IN_ROOM(ch) + 1;

  if (CMD_IS("list"))
  {
    send_to_char(ch, "Available pets are:\r\n");
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room)
    {
      /* No, you can't have the Implementor as a pet if he's in there. */
      if (!IS_NPC(pet))
        continue;
      send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
    }
    return (TRUE);
  }
  else if (CMD_IS("buy"))
  {
    two_arguments(argument, buf, sizeof(buf), pet_name, sizeof(pet_name));

    /* disqualifiers */
    if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet))
    {
      send_to_char(ch, "There is no such pet!\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet))
    {
      send_to_char(ch, "You don't have enough gold!\r\n");
      return (TRUE);
    }
    // if (check_npc_followers(ch, NPC_MODE_SPARE, 0) <= 0)
    if (!can_add_follower(ch, GET_MOB_VNUM(pet)))
    {
      send_to_char(ch, "You can't have any more pets!\r\n");
      return (TRUE);
    }

    /* success! */
    decrease_gold(ch, PET_PRICE(pet));

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
    if (GET_LEVEL(pet) <= 10)
    {
      GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) =
          GET_MAX_HIT(pet) * CONFIG_SUMMON_LEVEL_1_10_HP / 100;
      GET_REAL_AC(pet) = GET_REAL_AC(pet) * CONFIG_SUMMON_LEVEL_1_10_AC / 100;
      GET_HITROLL(pet) = GET_HITROLL(pet) * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      GET_DAMROLL(pet) = GET_DAMROLL(pet) * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      pet->mob_specials.damnodice =
          pet->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      pet->mob_specials.damsizedice =
          pet->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
    }
    else if (GET_LEVEL(pet) <= 20)
    {
      GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) =
          GET_MAX_HIT(pet) * CONFIG_SUMMON_LEVEL_11_20_HP / 100;
      GET_REAL_AC(pet) = GET_REAL_AC(pet) * CONFIG_SUMMON_LEVEL_11_20_AC / 100;
      GET_HITROLL(pet) = GET_HITROLL(pet) * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      GET_DAMROLL(pet) = GET_DAMROLL(pet) * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      pet->mob_specials.damnodice =
          pet->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      pet->mob_specials.damsizedice =
          pet->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
    }
    else
    {
      GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) =
          GET_MAX_HIT(pet) * CONFIG_SUMMON_LEVEL_21_30_HP / 100;
      GET_REAL_AC(pet) = GET_REAL_AC(pet) * CONFIG_SUMMON_LEVEL_21_30_AC / 100;
      GET_HITROLL(pet) = GET_HITROLL(pet) * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      GET_DAMROLL(pet) = GET_DAMROLL(pet) * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      pet->mob_specials.damnodice =
          pet->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      pet->mob_specials.damsizedice =
          pet->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
    }
    GET_HIT(pet) = GET_MAX_HIT(pet);

    if (*pet_name)
    {
      snprintf(buf, sizeof(buf), "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = strdup(buf);

      snprintf(buf, sizeof(buf),
               "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
               pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = strdup(buf);
    }

    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
    {
      X_LOC(pet) = world[IN_ROOM(ch)].coords[0];
      Y_LOC(pet) = world[IN_ROOM(ch)].coords[1];
    }

    char_to_room(pet, IN_ROOM(ch));

    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char(ch, "May you enjoy your pet.\r\n");
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return (TRUE);
  }

  /* All commands except list and buy */
  return (FALSE);
}

SPECIAL(buyweapons)
{
  if (!CMD_IS("buy") && !CMD_IS("list"))
    return 0;

  struct char_data *keeper = (struct char_data *)me;
  int level = 0;
  char arg1
      [100], // masterwork or mundane? if level is zero, if level > 0, then it's the weapon's name
             // which we'll copy into arg2 so we don't need to mess around with 2 variables, since
             // vendors level 1 and up ONLY sell weapons of their level bonus
      arg2[100]; // name of the weapon desired

  half_chop(argument, arg1, arg2);

  if (GET_LEVEL(keeper) <= 10)
    level = 0;
  else if (GET_LEVEL(keeper) <= 15)
    level = 1;
  else if (GET_LEVEL(keeper) <= 20)
    level = 2;
  else if (GET_LEVEL(keeper) <= 25)
    level = 3;
  else if (GET_LEVEL(keeper) <= 30)
    level = 4;
  else
    level = 5;

  if (CMD_IS("list"))
  {
    if (!*arg1)
    {
      if (level == 0)
      {
        send_to_char(ch, MASTERWORK_MSG);
        return 1;
      }
    }
    if (level == 0 && (!is_abbrev(arg1, "mundane") && !is_abbrev(arg1, "masterwork")))
    {
      send_to_char(ch, MASTERWORK_MSG);
      return 1;
    }
    display_buy_weapon_types(ch, level, !is_abbrev(arg1, "mundane"));
    return 1;
  }

  if (!*argument)
  {
    if (level == 0)
      send_to_char(ch, MASTERWORK_MSG);
    else
      send_to_char(ch, "Please specify the type of magical weapon you want to purchase.\r\n");
    return 1;
  }

  if (!*arg1)
  {
    if (level == 0)
      send_to_char(ch, MASTERWORK_MSG);
    else
      send_to_char(ch, "Please specify the type of magical weapon you want to purchase.\r\n");
    return 1;
  }

  if (!*arg2 && level == 0)
  {
    display_buy_weapon_types(ch, 0, false);
    send_to_char(ch, "Masterwork weapons cost 300 gold pieces more.\r\n");
    return 1;
  }

  if (level == 0 && (!is_abbrev(arg1, "mundane") && !is_abbrev(arg1, "masterwork")))
  {
    send_to_char(ch, MASTERWORK_MSG);
    return 1;
  }

  if (level > 0 && *argument)
  { // let's just work with arg2 to keep things easy
    skip_spaces(&argument);
    snprintf(arg2, sizeof(arg2), "%s", argument);
  }

  int i = 0, cost = 0;

  for (i = 0; i < NUM_WEAPON_TYPES; i++)
  {
    if (!strcmp(arg2, weapon_list[i].name))
      break;
  }

  if (i >= NUM_WEAPON_TYPES)
  {
    display_buy_weapon_types(ch, 0, false);
    send_to_char(ch, "Masterwork weapons cost 300 gold pieces more.\r\n");
    send_to_char(ch, "\r\nYou must specify the exact, full name of the weapon you wish to buy, in "
                     "lowercase.\r\n");
    return 1;
  }

  sbyte mundane = TRUE;

  // We want mundane to be the default since we're using is_abbrev and they both start with "m"
  if (!is_abbrev(arg1, "mundane"))
  {
    mundane = FALSE;
  }

  cost = get_vendor_weapon_cost(ch, level, i, !mundane);

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d gold to buy %s, but you only have %d.\r\n", cost,
                 weapon_list[i].name, GET_GOLD(ch));
    return 1;
  }

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
  {
    act("You can't carry any more items.", FALSE, ch, 0, 0, TO_CHAR);
    return (1);
  }

  struct obj_data *obj = NULL;
  obj_vnum base_vnum = 66;

  if ((obj = read_object(base_vnum, VIRTUAL)) == NULL)
  {
    send_to_char(ch,
                 "There seems to be an error in purchasing %s.  Please inform a staff member.\r\n",
                 weapon_list[i].name);
    return 1;
  }

  set_weapon_object(obj, i);
  GET_OBJ_COST(obj) = cost;
  set_weapon_name(obj, i);
  if (!mundane && level == 0)
  {
    set_masterwork_obj_name(obj);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MASTERWORK);
    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 1;
    obj->affected[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
  }
  if (level > 0)
  {
    set_magical_obj_name(obj, level);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MASTERWORK);
    GET_OBJ_VAL(obj, 4) = level; // Enhancement Bonus
  }

  GET_GOLD(ch) -= cost;
  obj_to_char(obj, ch);
  send_to_char(ch, "You purchase %s for %d gold coins.\r\n", obj->short_description, cost);

  return 1;
}


SPECIAL(identify_mob)
{
  if (!CMD_IS("identify") && !CMD_IS("value"))
    return 0;

  char arg1[200];
  struct obj_data *obj = NULL;
  int cost = 0;

  one_argument(argument, arg1, sizeof(arg1));

  if (!*arg1)
  {
    send_to_char(ch, "Which item do you wish to have identified? Or enter 'worn' to show basic "
                     "bonuses on equipped items.\r\n");
    return 1;
  }

  if (is_abbrev(arg1, "worn"))
  {
    cost = MIN(500, GET_LEVEL(ch) * 25);

    if (GET_GOLD(ch) < cost)
    {
      send_to_char(ch, "You need to pay %d coins to identify your worn equipment.\r\n", cost);
      return 1;
    }

    GET_GOLD(ch) -= cost;

    send_to_char(ch, "\r\nYour equipped items have been identified for %d coins.\r\n\r\n", cost);

    call_magic(ch, ch, 0, SPELL_MASS_IDENTIFY, 0, 30, CAST_SPELL);

    return 1;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))

    if (!obj)
    {
      send_to_char(ch, "You don't seem to have that item on hand.\r\n");
      return 1;
    }

  /* success! */
  if (obj)
  {
    int cost = MAX(1, GET_OBJ_LEVEL(obj) * 5);

    if (CMD_IS("identify"))
    {
      if (GET_GOLD(ch) < cost)
      {
        send_to_char(
            ch,
            "You don't have the coins to play for that. You need %d, but only have %d on hand.\r\n",
            cost, GET_GOLD(ch));
        return 1;
      }
      GET_GOLD(ch) -= cost;
      send_to_char(ch, "That will cost you %d coins.\r\n", cost);
      do_stat_object(ch, obj, ITEM_STAT_MODE_IDENTIFY_SPELL);
    }
    else
    {
      send_to_char(ch, "It will cost %d coins to identify that item.", cost);
    }
    return 1;
  }
  else
  {
    send_to_char(ch, "You don't seem to have that item in your inventory.\r\n");
    return 1;
  }

  return 1;
}
