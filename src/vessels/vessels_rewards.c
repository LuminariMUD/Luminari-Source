/* ************************************************************************
 *      File:   vessels_rewards.c                     Part of LuminariMUD  *
 *   Purpose:   Renown and the rewards of a sinking (vessels-ships study   *
 *              S7, 3.3.7): salvage, bounties, and the renown board        *
 * ********************************************************************** */

/*
 * When a hull goes down, the player's hull that sank her and her allies in
 * sight share out what she was worth, as DurisMUD's sink_ship() does: salvage
 * from what is left of her, a bounty on her renown, and the bounty on her
 * owner if the owner went down with her. Each share is paid to the sharing
 * hull's owner through the vessel settlement queue. Renown lives on the hull
 * and passes only between players' hulls: the sharers split her hull weight
 * and she loses it.
 */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/interpreter.h"
#include "vessels.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/** The hull's owner, when in the game and aboard her; else NULL. */
struct char_data *vessel_owner_aboard(const struct greyhawk_ship_data *ship)
{
  struct char_data *owner;

  owner = vessel_find_online_player(ship->owner);
  return owner != NULL && get_ship_from_room(IN_ROOM(owner)) == ship ? owner : NULL;
}

/**
 * What can be salvaged from a hull sunk (Duris calc_salvage()): her price
 * times the share of her armor and structure left, plus half the price of
 * each weapon not destroyed, divided by 8.
 */
int vessel_salvage_value(const struct greyhawk_ship_data *ship)
{
  const struct vessel_weapon_type *weapon;
  long long left;
  long long most;
  long long value;
  int i;

  left = (long long)ship->farmor + ship->rarmor + ship->parmor + ship->sarmor +
         vessel_total_internal(ship);
  most = (long long)ship->maxfarmor + ship->maxrarmor + ship->maxparmor + ship->maxsarmor +
         vessel_max_internal(ship);
  value = most > 0 ? vessel_hull_price(ship) * left / most : 0;
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    weapon = vessel_slot_weapon(&ship->slot[i]);
    if (weapon != NULL && ship->slot[i].damage < VESSEL_WEAPON_DESTROYED)
    {
      value += weapon->price / 2;
    }
  }
  return (int)(value / VESSEL_SALVAGE_DIVIDER);
}

/**
 * Settle a sinking (study 3.3.7). The victor, when another player's hull,
 * and her allies in sight (players' hulls whose online owners share the
 * victor's owner's group) split the salvage, the bounty on her renown, and
 * the bounty on her owner if the owner is aboard, which is then cleared.
 * When she is a player's hull the sharers also split her hull weight in
 * renown, and she loses it.
 *
 * @return the renown she loses, her hull weight, when another player's hull
 *         sank her; else 0
 */
int vessel_settle_sinking(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *victor)
{
  struct greyhawk_ship_data *sharers[GREYHAWK_MAXSHIPS];
  struct greyhawk_ship_data *other;
  struct char_data *captain;
  struct char_data *owner;
  char letter[MAX_STRING_LENGTH];
  bool player_hull;
  int renown_bounty;
  int salvage;
  int bounty;
  int weight;
  int sight;
  int count;
  int share;
  int i;

  if (victor == NULL || victor->owner[0] == '\0' || !str_cmp(victor->owner, ship->owner))
  {
    return 0;
  }

  sharers[0] = victor;
  count = 1;
  sight = vessel_sight_range(ship);
  captain = vessel_find_online_player(victor->owner);
  for (i = 0; i < GREYHAWK_MAXSHIPS && captain != NULL && GROUP(captain) != NULL; i++)
  {
    other = &greyhawk_ships[i];
    if (other == victor || other == ship || !is_valid_ship(other) || other->owner[0] == '\0' ||
        !str_cmp(other->owner, ship->owner) || vessel_range_between(ship, other) > sight)
    {
      continue;
    }
    owner = vessel_find_online_player(other->owner);
    if (owner != NULL && GROUP(owner) == GROUP(captain))
    {
      sharers[count++] = other;
    }
  }

  player_hull = ship->owner[0] != '\0';
  weight = vessel_class_handling(ship->vessel_type)->hull_weight;
  salvage = vessel_salvage_value(ship);
  renown_bounty = ship->renown > VESSEL_RENOWN_BOUNTY_FLOOR ? ship->renown * 5 / 2 : 0;
  bounty = player_hull && vessel_owner_aboard(ship) != NULL ? vessel_get_bounty(ship->owner) : 0;
  if (bounty < BOUNTY_WANTED || !vessel_clear_bounty(ship->owner))
  {
    bounty = 0;
  }
  share = (salvage + renown_bounty + bounty) / count;
  snprintf(letter, sizeof(letter),
           "The prize court rewards the sinking of %s: %d gold in salvage, %d gold on her renown, "
           "and %d gold posted on her captain, shared among %d hull%s. Your hull's share is %d "
           "gold.",
           ship->name, salvage, renown_bounty, bounty, count, count == 1 ? "" : "s", share);

  for (i = 0; i < count; i++)
  {
    if (player_hull)
    {
      sharers[i]->renown += weight / count;
      send_to_ship(sharers[i], "%s wins %d renown for sinking %s.", sharers[i]->name,
                   weight / count, ship->name);
      vessel_db_save_runtime(sharers[i]);
    }
    vessel_pay_prize(sharers[i], share, letter);
  }
  log("Info: Ship %d '%s' sunk by ship %d: %d salvage, %d renown bounty, %d bounty, %d sharer%s",
      ship->shipnum, ship->name, victor->shipnum, salvage, renown_bounty, bounty, count,
      count == 1 ? "" : "s");

  if (!player_hull)
  {
    return 0;
  }
  ship->renown = MAX(0, ship->renown - weight);
  return weight;
}

/**
 * shiprenown - the players' hulls with the most renown, afloat or awaiting
 * a summons.
 */
ACMD(do_shiprenown)
{
  struct greyhawk_ship_data *board[VESSEL_RENOWN_BOARD_ROWS];
  struct greyhawk_ship_data *ship;
  int rows;
  int row;
  int i;

  rows = 0;
  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if ((!is_valid_ship(ship) && !ship->stowed) || ship->owner[0] == '\0' || ship->renown <= 0 ||
        (rows == VESSEL_RENOWN_BOARD_ROWS &&
         board[VESSEL_RENOWN_BOARD_ROWS - 1]->renown >= ship->renown))
    {
      continue;
    }
    row = rows < VESSEL_RENOWN_BOARD_ROWS ? rows++ : VESSEL_RENOWN_BOARD_ROWS - 1;
    while (row > 0 && board[row - 1]->renown < ship->renown)
    {
      board[row] = board[row - 1];
      row--;
    }
    board[row] = ship;
  }

  if (rows == 0)
  {
    send_to_char(ch, "No hull has won renown yet.\r\n");
    return;
  }
  send_to_char(ch, "The most renowned hulls:\r\n");
  for (row = 0; row < rows; row++)
  {
    send_to_char(ch, "%2d. %-30s %-10s %-16s %6d renown\r\n", row + 1, board[row]->name,
                 get_vessel_type_name(board[row]->vessel_type), board[row]->owner,
                 board[row]->renown);
  }
}
