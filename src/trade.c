/* Trading System
   Author:  Zusuk

   LuminariMUD
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "comm.h"
#include "race.h"
#include "spells.h"
#include "trade.h"

/*
extern struct room_data *world;
extern void add_follower(struct char_data * ch, struct char_data * leader);
extern struct index_data *mob_index;
extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
extern void sprintfcash(char *string, int p, int g, int s, int c);
extern bool can_loot_wagon(struct obj_data *obj, struct char_data *ch);
extern void lose_money(struct char_data *ch, int price);
extern bool perform_put(struct char_data * ch, struct obj_data *obj, struct obj_data *container);

struct position {
  int x;
  int y;
};

struct trade_info {
  int vnum;
  int data1;
  int data2;
  int data3;
  int x;
  int y;
};


struct trade_info supply[] = {
  { 22000, 22000, 22001, 22002, 0, 200}, //	Waterdeep....../jewelry, pottery, ale
  { 22001, 22003, 22004, 22005, 30, 20}, //	Luskan........./fish, timber, gems
  { 22002, 22006, 22007, 22008, 80, 5}, //	Mithril Hall.../Weapons, Armor, Steel
  { 22003, 22009, 22010, 22011, 10, 80}, //	Neverwinter..../fish, clocks, lamps
  { 22004, 22012, 22013, 22014, 380, 550}, //	Zhenthil Keep../iron, gems, timber
  { 22005, 22015, 22016, 22017, 180, 400}, //	Arabel........./raw textile, timber, grain
  { 22006, 22018, 22019, 22020, 250, 420}, //	Tilverton....../coal, wine, glass
  { 22007, 22021, 22022, 22023, 10, 500}, //	Beregost......./gems, raw textile, pottery
  { 22008, 22024, 22025, 22026, 100, 300}, //	Zzsessak Zuhl../artwork, spices, gold
  { 22009, 22027, 22028, 22029, 250, 70}, //	Menzo........../artwork, books, silk
  { 22010, 22030, 22031, 22032, 50, 100}, //	Grack........../weapons, armor, fish
  { -1, 0, 0, 0},

};




struct trade_info demand[] = {

  { 22000, TRADE_IRON, TRADE_CLOCKS, TRADE_ARTWORK}, //	Waterdeep......iron, clocks, artwork......
  { 22001, TRADE_BOOKS, TRADE_POTTERY, TRADE_SPICES}, //	Luskan.........books, pottery, spices.....
  { 22002, TRADE_GRAIN, TRADE_POTTERY, TRADE_WINE}, //	Mithril Hall...Grain, Pottery, Wine.......
  { 22003, TRADE_POTTERY, TRADE_SPICES, TRADE_GLASS}, //	Neverwinter....Pottery, Spices, glass.....
  { 22004, TRADE_GRAIN, TRADE_STEEL, TRADE_RAW_TEXTILE}, //	Zhenthil Keep..grain, steel, raw textile..
  { 22005, TRADE_LAMPS, TRADE_COAL, TRADE_JEWELRY}, //	Arabel.........lamps, coal, jewelry.......
  { 22006, TRADE_ARTWORK, TRADE_FISH, TRADE_GEMS}, //	Tilverton......artwork, fish, gems........
  { 22007, TRADE_SPICES, TRADE_IRON, TRADE_FISH}, //	Beregost.......spices, iron, fish.........
  { 22008, TRADE_WEAPONS, TRADE_ARMOR, TRADE_SILK}, //	Zzsessak Zuhl..weapons, armor, silk.......
  { 22009, TRADE_WEAPONS, TRADE_ARMOR, TRADE_WINE}, //	Menzo..........weapons, armor, wine.......
  { 22010, TRADE_ALE, TRADE_ARTWORK, TRADE_TIMBER}, //	Grack..........ale, artwork, timber.......

  { -1, 0, 0, 0},

};


char * trade_name[NUM_TRADE_GOODS] = {
                                      "Ale",
                                      "Armor",
                                      "Artwork",
                                      "Books",
                                      "Clocks",
                                      "Coal",
                                      "Fish",
                                      "Gems",
                                      "Glass",
                                      "Gold",
                                      "Grain",
                                      "Iron",
                                      "Jewelry",
                                      "Lamps",
                                      "Pottery",
                                      "Raw Textile",
                                      "Silk",
                                      "Spices",
                                      "Steel",
                                      "Timber",
                                      "Weapons",
                                      "Wine",
};

ACMDU(do_dropwagon) {
  send_to_char(ch, "You can't do that here.\r\n");
}

ACMDU(do_getwagon) {
  send_to_char(ch, "You can't do that here.\r\n");
}

ACMDU(do_claimwagon) {
  struct obj_data *obj;
  struct char_data *tmp_char;

  one_argument(argument, arg);
  if (!arg || !*arg) {
    send_to_char(ch, "Claim what wagon?\r\n");
    return;
  }

  generic_find(arg, FIND_OBJ_ROOM, ch, &tmp_char, &obj);

  if (!obj) {
    send_to_char(ch, "You do not see that here!\r\n");
    return;
  }
  if (GET_OBJ_TYPE(obj) != ITEM_WAGON) {
    send_to_char(ch, "But that is not a wagon.\r\n");
    return;
  }
  if (WAGON(ch)) {
    send_to_char(ch, "You can only have one wagon at a time.\r\n");
    return;
  }
  if (WAGON_OWNER(obj)) {
    send_to_char(ch, "This wagon is already claimed.\r\n");
    return;
  }


  if (GET_OBJ_VAL(obj, 3) != GET_IDNUM(ch)) {
    send_to_char(ch, "That is not your wagon and the owner have not allowed you to move it.\r\n");
    return;
  }

  WAGON_OWNER(obj) = ch;
  WAGON(ch) = obj;
  send_to_char(ch, "You hitch up a wagon.\r\n");
}

int get_profit_factor(struct char_data *ch, struct obj_data *obj, struct char_data *master) {
  int factor = 100;
  // if its a type produced here, then there is no profit...
  int read = 0;
  position our_pos;
  int min_dist = 100000;
  while (1) {
    if (supply[read].vnum == -1)
      break;
    if (supply[read].vnum == GET_MOB_VNUM(master)) {
      our_pos.x = supply[read].x;
      our_pos.y = supply[read].y;
    }
    read++;
  }

  read = 0;
  while (1) {
    if (supply[read].vnum == -1)
      break;
    if (supply[read].vnum == GET_MOB_VNUM(master)) {
      if (GET_OBJ_VAL(obj, 0) == GET_OBJ_VAL(&obj_proto[real_object(supply[read].data1)], 0))
        return 100;
      if (GET_OBJ_VAL(obj, 0) == GET_OBJ_VAL(&obj_proto[real_object(supply[read].data2)], 0))
        return 100;
      if (GET_OBJ_VAL(obj, 0) == GET_OBJ_VAL(&obj_proto[real_object(supply[read].data3)], 0))
        return 100;
    } else {
      if (GET_OBJ_VAL(obj, 0) == GET_OBJ_VAL(&obj_proto[real_object(supply[read].data1)], 0) ||
          GET_OBJ_VAL(obj, 0) == GET_OBJ_VAL(&obj_proto[real_object(supply[read].data2)], 0) ||
          GET_OBJ_VAL(obj, 0) == GET_OBJ_VAL(&obj_proto[real_object(supply[read].data3)], 0)) {
        int diff_x = our_pos.x - supply[read].x;
        int diff_y = our_pos.y - supply[read].y;
        if (diff_y < 0)
          diff_y = -diff_y;
        if (diff_x < 0)
          diff_x = -diff_x;
        int dist = diff_x + diff_y; // yes, but the worlds is not a 2d grid either...

        if (dist < min_dist)
          min_dist = dist;

      }
    }

    read++;
  }
  bool demanded = FALSE;
  read = 0;
  while (1) {
    if (demand[read].vnum == -1)
      break;
    if (demand[read].vnum == GET_MOB_VNUM(master)) {
      if (GET_OBJ_VAL(obj, 0) == demand[read].data1)
        demanded = TRUE;
      if (GET_OBJ_VAL(obj, 0) == demand[read].data2)
        demanded = TRUE;
      if (GET_OBJ_VAL(obj, 0) == demand[read].data3)
        demanded = TRUE;
    }
    read++;
  }

  if (min_dist > 1000)
    min_dist = 1000;

  int charisma = GET_R_CHA(ch);

  if (demanded)
    factor = 105 + charisma / 15 + min_dist / 75;
  else
    factor = 101 + charisma / 25 + min_dist / 100;

  return factor;
}

void show_trade_item(struct obj_data *obj, int index, struct char_data *ch) {
  int price = GET_OBJ_COST(obj);
  sprintfcash(buf2, price / 1000, (price % 1000) / 100, (price % 100) / 10, price % 10);
  snprintf(buf, sizeof(buf), "%d) %-40s %s\r\n", index, obj->short_description, buf2);
  send_to_char(ch, buf);
}

SPECIAL(trade_master) {
  if (cmd) {
    struct char_data *master = (struct char_data *) me;
    char arg1[MAX_INPUT_LENGTH] = {'\0'};
    char arg2[MAX_INPUT_LENGTH] = {'\0'};

    if (CMD_IS("list")) {
      int read = 0;
      while (1) {
        if (supply[read].vnum == -1)
          break;
        if (supply[read].vnum == GET_MOB_VNUM(master)) {
          send_to_char(ch, "We are producing the following in this city.\r\n");
          send_to_char(ch, "&cw=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=&c0\r\n");
          show_trade_item(&obj_proto[real_object(supply[read].data1)], 1, ch);
          show_trade_item(&obj_proto[real_object(supply[read].data2)], 2, ch);
          show_trade_item(&obj_proto[real_object(supply[read].data3)], 3, ch);
        }
        read++;
      }
      read = 0;
      while (1) {
        if (demand[read].vnum == -1)
          break;
        if (demand[read].vnum == GET_MOB_VNUM(master)) {
          send_to_char(ch, "&cw=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=&c0\r\n");
          snprintf(buf, sizeof(buf), "We are primarily looking for %s, %s & %s.\r\n"
                  , trade_name[ demand[read].data1 ]
                  , trade_name[ demand[read].data2 ]
                  , trade_name[ demand[read].data3 ]
                  );
          send_to_char(ch, buf);
        }
        read++;
      }

      return TRUE;
    } else if (CMD_IS("buy")) {
      two_arguments(argument, arg1, arg2);

      int count = 1;
      if (!*arg2) {
        count = 1;
        strcpy(arg, arg1);
      } else {
        count = atoi(arg1);
        strcpy(arg, arg2);
      }


      if (!arg || !*arg) {
        send_to_char(ch, "Buy what?\n");
        return TRUE;
      }
      if (count < 1) {
        send_to_char(ch, "You must at least buy 1 item.\r\n");
        return TRUE;
      }

      if (!WAGON(ch)) {
        send_to_char(ch, "But you have no wagon to buy to.\r\n");
        return TRUE;
      }

      int index = atoi(arg);
      if (index < 1 || index > 3) {
        send_to_char(ch, "Either buy resource 1, 2 or 3?\n");
        return TRUE;
      }
      struct obj_data *proto = 0;
      int read = 0;
      while (1) {
        if (supply[read].vnum == -1)
          break;
        if (supply[read].vnum == GET_MOB_VNUM(master)) {
          switch (index) {
            case 1: proto = &obj_proto[real_object(supply[read].data1)];
              break;
            case 2: proto = &obj_proto[real_object(supply[read].data2)];
              break;
            case 3: proto = &obj_proto[real_object(supply[read].data3)];
              break;
          }
        }
        read++;
      }
      if (!proto) {
        send_to_char(ch, "This trade center is broken, talk to a god immediately.\r\n");
        return TRUE;
      }
      int org_count = count;

      while (count) {

        int price = GET_OBJ_COST(proto);
        int cash = GET_COPPER(ch) + GET_SILVER(ch)*10 + GET_GOLD(ch)*100 + GET_PLAT(ch)*1000;

        if (cash < price) {
          send_to_char(ch, "You can't afford that.\r\n");
          break;
        } else {
          struct obj_data *obj = read_object(GET_OBJ_VNUM(proto), VIRTUAL);
          obj_to_obj(obj, WAGON(ch));
          if (org_count == 1)
            act("You buy $p.", FALSE, ch, obj, 0, TO_CHAR);
          lose_money(ch, price);
        }
        count--;
      }
      if (org_count > 1) {
        snprintf(buf, sizeof(buf), "You buy %d %s(s).\r\n", org_count - count, proto->short_description);
        send_to_char(ch, buf);
      }
      return TRUE;
    } else if (CMD_IS("sell")) {
      struct obj_data *obj;
      struct char_data *tmp_char;

      two_arguments(argument, arg1, arg2);

      int count = 1;
      if (!*arg2) {
        count = 1;
        strcpy(arg, arg1);
      } else {
        count = atoi(arg1);
        strcpy(arg, arg2);
      }
      if (!arg || !*arg) {
        send_to_char(ch, "Sell what resource?\r\n");
        return TRUE;
      }
      if (!WAGON(ch)) {
        send_to_char(ch, "But you have no wagon to sell from.\r\n");
        return TRUE;
      }

      if (count < 1) {
        send_to_char(ch, "You must at least sell 1 item.\r\n");
        return TRUE;
      }
      int org_count = count;
      char profit_info[MAX_INPUT_LENGTH] = {'\0'};
      int total_profit = 0;
      bool found = FALSE;
      while (count) {
        obj = get_obj_in_list_vis(ch, arg, WAGON(ch)->contains);

        if (!obj) {
          send_to_char(ch, "You do not see that here!\r\n");
          break;
        } else if (GET_OBJ_TYPE(obj) != ITEM_RESOURCE) {
          send_to_char(ch, "But that is not a resource.\r\n");
          return TRUE;
        } else {
          found = TRUE;
          int profit = get_profit_factor(ch, obj, master);
          snprintf(profit_info, sizeof(profit_info), "You sold at a profit of %d percent.\r\n", profit - 100);
          int price = GET_OBJ_COST(obj);
          price *= profit;
          price /= 100;
          total_profit += price;
          obj_from_obj(obj);
          extract_obj(obj);

          GET_PLAT(ch) += price / 1000;
          GET_GOLD(ch) += (price % 1000) / 100;
          GET_SILVER(ch) += (price % 100) / 10;
          GET_COPPER(ch) += price % 10;
        }
        count--;
      }
      if (found) {
        if (org_count > 1) {
          snprintf(buf, sizeof(buf), "You sell %d %s(s).\r\n", org_count - count, arg2);
          send_to_char(ch, buf);
        }
        send_to_char(ch, profit_info);
        sprintfcash(buf2, total_profit / 1000, (total_profit % 1000) / 100, (total_profit % 100) / 10, total_profit % 10);
        snprintf(buf, sizeof(buf), "You gained %s.\r\n", buf2);
        send_to_char(ch, buf);
      }
      return TRUE;
    } else if (CMD_IS("value")) {
      struct obj_data *obj;
      struct char_data *tmp_char;

      one_argument(argument, arg);
      if (!arg || !*arg) {
        send_to_char(ch, "Sell what resource?\r\n");
        return TRUE;
      }

      if (!WAGON(ch)) {
        send_to_char(ch, "But you have no wagon to sell from.\r\n");
        return TRUE;
      }

      //      generic_find(arg, FIND_OBJ_INV , ch, &tmp_char, &obj);

      obj = get_obj_in_list_vis(ch, arg, WAGON(ch)->contains);

      if (!obj) {
        send_to_char(ch, "You do not see that here!\r\n");
        return TRUE;
      }
      if (GET_OBJ_TYPE(obj) != ITEM_RESOURCE) {
        send_to_char(ch, "But that is not a resource.\r\n");
        return TRUE;
      }
      int profit = get_profit_factor(ch, obj, master);
      snprintf(buf, sizeof(buf), "You estimate you could sell this at a profit of %d percent.\r\n", profit - 100);
      send_to_char(ch, buf);
      return TRUE;
    } else if (CMD_IS("getwagon")) {
      if (WAGON(ch))
        send_to_char(ch, "You already have a wagon.\r\n");
      else {
        struct obj_data *obj = read_object(22099, VIRTUAL);
        WAGON(ch) = obj;
        WAGON_OWNER(obj) = ch;

        GET_OBJ_VAL(obj, 3) = GET_IDNUM(ch);
        SET_BIT(GET_OBJ_SAVED(obj), SAVE_OBJ_VALUES);

        obj_to_room(obj, ch->in_room);
        send_to_char(ch, "You hitch up a wagon.\r\n");
        return TRUE;
      }
    } else if (CMD_IS("dropwagon")) {
      if (!WAGON(ch))
        send_to_char(ch, "But you do not have any wagon attached.\r\n");
      else {
        if (WAGON(ch)->contains)
          send_to_char(ch, "You can't discard a wagon that contains resources.\r\n");
        else {
          send_to_char(ch, "You discard your wagon.\r\n");
          extract_obj(WAGON(ch));
          WAGON(ch) = 0;
        }
      }
      return TRUE;
    }

  } else {

  }
  return FALSE;
}

struct encounter_info {
  int leader;
  int follower;
  int min_cost;
  int fol_count;
};

struct encounter_info road_encounters[] = {
  { 22030, 22031, 0, 4},
  { 22030, 22031, 1, 4},
  { 22039, -1, 1, 1},
  { 22038, 22037, 1, 6},
  { 22038, 22037, 1, 6},
  { 22055, 22054, 1, 6},
  { 22064, 22063, 1, 3},
  { 22057, 22056, 5, 10},
  { 22033, 22034, 10, 8},
  { 22033, 22034, 10, 8},
  { 22060, 22059, 10, 4},
  { 22062, 22061, 10, 3},
  { 22051, 22050, 10, 3},
  { 22058, -1, 10, 10},
  { 22065, -1, 10, 10},
  { 22035, -1, 50, 1},
  { 22036, -1, 50, 1},
  { 22053, 22052, 100, 5},
  { 22068, 22067, 100, 5},
  { 22066, -1, 100, 10},

  { 22032, -1, 300, 1},
  { -1, -1, -1},
  { -1, -1, -1},
};

struct encounter_info mountain_encounters[] = {
  { 22030, 22031, 0, 4},
  { 22030, 22031, 1, 4},
  { 22039, -1, 1, 1},
  { 22038, 22037, 1, 6},
  { 22038, 22037, 1, 6},
  { 22055, 22054, 1, 6},
  { 22064, 22063, 1, 3},
  { 22057, 22056, 5, 10},
  { 22033, 22034, 10, 8},
  { 22033, 22034, 10, 8},

  { 22060, 22059, 10, 4},
  { 22062, 22061, 10, 3},
  { 22051, 22050, 10, 3},
  { 22058, -1, 10, 10},
  { 22065, -1, 10, 10},

  { 22035, -1, 50, 1},
  { 22036, -1, 50, 1},
  { 22053, 22052, 100, 5},
  { 22068, 22067, 100, 5},
  { 22066, -1, 100, 10},

  { 22032, -1, 300, 1},
  { -1, -1, -1},
  { -1, -1, -1},
};

struct encounter_info forest_encounters[] = {
  { 22030, 22031, 0, 4},
  { 22030, 22031, 1, 4},
  { 22039, -1, 1, 1},
  { 22038, 22037, 1, 6},
  { 22038, 22037, 1, 6},
  { 22055, 22054, 1, 6},
  { 22064, 22063, 1, 3},
  { 22057, 22056, 5, 10},
  { 22060, 22059, 10, 4},
  { 22062, 22061, 10, 3},
  { 22051, 22050, 10, 3},
  { 22058, -1, 10, 10},
  { 22065, -1, 10, 10},
  { 22033, 22034, 10, 8},
  { 22033, 22034, 10, 8},
  { 22035, -1, 50, 1},
  { 22036, -1, 50, 1},
  { 22053, 22052, 100, 5},
  { 22068, 22067, 100, 5},
  { 22066, -1, 100, 10},

  { 22032, -1, 300, 1},
  { -1, -1, -1},
};

struct encounter_info ud_encounters[] = {
  { 22040, -1, 0, 1},
  { 22041, -1, 1, 1},
  { 22044, -1, 1, 1},
  { 22048, -1, 1, 1},
  { 22045, 22046, 25, 3},
  { 22045, 22046, 25, 3},
  { 22047, -1, 50, 1},
  { 22042, 22043, 100, 1},
  { 22049, -1, 300, 1},
  { -1, -1, -1},
};

struct char_data *create_encounter_mob(struct char_data *ch, int vnum, struct char_data *master, int level) {
  struct char_data *mob = read_mobile(vnum, VIRTUAL, level);
  if (mob) {
    remember(mob, ch);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(ch->in_room), ZONE_WILDERNESS))
    {
      X_LOC(mob) = world[ch->in_room].coords[0];
      Y_LOC(mob) = world[ch->in_room].coords[1];
    }
    char_to_room(mob, ch->in_room);

    SET_BIT(AFF_FLAGS(mob), AFF_GROUP);
    SET_BIT(MOB_FLAGS(mob), MOB_HUNTER);
    if (master)
      add_follower(mob, master);
    else
      hit(mob, ch, TYPE_UNDEFINED, TRUE);
  }
  return mob;
}

void generate_encounter(struct char_data *ch, struct encounter_info *list) {
  send_to_char(ch, "It seems as if your trade caravan has attracted some interest.\r\n");
  int total_levels = 0;
  int cost = 0;
  struct obj_data *obj;
  for (obj = WAGON(ch)->contains; obj; obj = obj->next_content)
    cost += GET_OBJ_COST(obj);
  cost /= 5000; // one level for each 4 plat. (assume a 1plat profit in this.)

  if (cost < 10)
    cost = 10;
  total_levels = cost + dice(1, cost);

  if (total_levels < 5)
    total_levels = 5;

  int read = 0;

  int total_size = 0;
  while (list[read].leader > 0) {
    if (list[read].min_cost <= cost)
      total_size++;
    read++;
  }

  if (total_size < 1) {
    send_to_char(ch, "No possible trade encounters in this sector. Talk to a God.\r\n");
    return;
  }
  read = dice(1, total_size) - 1;

  if (list[read].follower == -1) {
    //single mob
    if (total_levels > 58)
      total_levels = 58;
    create_encounter_mob(ch, list[read].leader, 0, total_levels);
  }
  else {
    int level = total_levels / list[read].fol_count;
    if (level > GET_LEVEL(ch) + 5)
      level = GET_LEVEL(ch) + 5;
    if (level < GET_LEVEL(ch) - 5)
      level = GET_LEVEL(ch) - 5;
    struct char_data *master = create_encounter_mob(ch, list[read].leader, 0, level);
    int base = total_levels / list[read].fol_count;
    if (base < GET_LEVEL(ch) - 20)
      base = GET_LEVEL(ch) - 20;
    if (base < 1)
      base = 1;

    while (total_levels > 0) {
      int level = base;
      if (level > GET_LEVEL(ch) - 1)
        level = GET_LEVEL(ch) - 1;
      if (level < GET_LEVEL(ch) - 10)
        level = GET_LEVEL(ch) - 10;

      create_encounter_mob(ch, list[read].follower, master, level);
      total_levels -= base;
    }
  }

}

bool is_in_underdark(struct char_data *ch) {
  switch (SECT(ch->in_room)) {
    case SECT_UD_CITY:
    case SECT_UD_INSIDE:
    case SECT_UD_NOGROUND:
    case SECT_UD_WILD:
    case SECT_UD_WATER:
    case SECT_UD_NOSWIM:
      return TRUE;
  }
  return FALSE;
}

void test_for_encounter(struct char_data *ch) {
  int chance = 20;

  if (is_in_underdark(ch)) {
    chance *= 2;
    chance /= 3;
  } else if (weather_info.sunlight == SUN_LIGHT)
    chance *= 2;

  if (number(0, chance))
    return;

  struct encounter_info *list = 0;
  switch (SECT(ch->in_room)) {
    case SECT_INSIDE:
    case SECT_CITY:
    case SECT_FLYING:
    case SECT_SHIPREQUIRED:
    case SECT_OCEAN:
    case SECT_UD_CITY:
    case SECT_UD_INSIDE:
    case SECT_UD_NOGROUND:
      //no encounters here..
      break;
    case SECT_FIREPLANE:
    case SECT_AIR_PLANE:
    case SECT_ASTRAL_PLANE:
    case SECT_EARTH_PLANE:
    case SECT_ETHEREAL:
    case SECT_ICE_PLANE:
    case SECT_LAVA:
    case SECT_WATER_PLANE:
    case SECT_CLOUDS:
    case SECT_SHADOWPLANE:
    case SECT_LIGHTNING:
      //planar.. if we add extra-planar things, we will have encounters here..
      break;

    case SECT_FOREST:
      list = forest_encounters;
      break;

    case SECT_HILLS:
    case SECT_MOUNTAIN:
      list = mountain_encounters;
      break;

    case SECT_UD_WILD:
    case SECT_UD_WATER:
    case SECT_UD_NOSWIM:
      list = ud_encounters;
      break;

    case SECT_UNDERWATER:
    case SECT_UNDERWATER_GR:
      //TODO    list    = underwater_encounters;
      break;

    case SECT_FIELD:
    case SECT_LONG_ROAD:
    case SECT_WATER_SWIM:
    case SECT_WATER_NOSWIM:
      list = road_encounters;
      break;
  }

  if (list)
    generate_encounter(ch, list);
}

SPECIAL(trade_bandit) {
  if (cmd)
    return 0;

  long secs = time(0);

  int curr = secs / 60;
  if (YELLED(ch) == 0)
    YELLED(ch) = curr;

  if (FIGHTING(ch))
    return 0;
  if (GET_POS(ch) < POS_STANDING)
    return 0;

  if (!number(0, 5) || ch->master)
    return 0;

  struct obj_data *obj;
  memory_rec *names;

  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content) {
    if (GET_OBJ_TYPE(obj) == ITEM_WAGON && obj->contains) {
      for (names = MEMORY(ch); names; names = names->next) {
        if (names->id == GET_OBJ_VAL(obj, 3)) {
          struct obj_data *looted = obj->contains;
          act("$n loots $p from a wagon.", FALSE, ch, looted, 0, TO_ROOM);
          obj_from_obj(looted);
          obj_to_char(ch, looted);
          WAIT_STATE(ch, PULSE_VIOLENCE * 5);
          save_wagons();
          return 1;
        }
      }
    }
  }

  if (YELLED(ch) != -1) {
    int time = curr - YELLED(ch);
    if (time > 15 && !ch->carrying) {
      whack_object(0, ch, 1);
      YELLED(ch) = -1;
    }
  }

  return 0;
}

SPECIAL(trade_object) {
  struct obj_data *obj = (struct obj_data *) me;

  if (cmd)
    return 0;
  if (!ch)
    return 0;
  if (!obj->carried_by)
    return 0;

  if (!WAGON(ch))
    return 0;
  if (WAGON(ch)->in_room != ch->in_room)
    return 0;

  obj_from_char(obj);
  obj_to_obj(obj, WAGON(ch));

  act("You put $p in $P.", FALSE, ch, obj, WAGON(ch), TO_CHAR);
  act("$n puts $p in $P.", FALSE, ch, obj, WAGON(ch), TO_ROOM);
  save_wagons();

  return 1;
}

*/