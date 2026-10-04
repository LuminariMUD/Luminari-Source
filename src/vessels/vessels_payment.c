/* ************************************************************************
 *      File:   vessels_payment.c                     Part of LuminariMUD  *
 *   Purpose:   Checked vessel purchases and payouts (study step S15).     *
 *              A shipyard job, a fee or a sale writes the ship's side to  *
 *              MariaDB and the captain's gold to the player file. A       *
 *              purchase saves the gold first and is refunded when the     *
 *              ship's side cannot be written; a payout writes the ship's  *
 *              side first and is put back when the gold cannot be saved.  *
 *              Only a crash between a command's two writes splits them.   *
 * ********************************************************************** */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "character/rewards.h"
#include "database/mysql.h"
#include "vessels.h"

/**
 * Move a captain's gold and save the player file at once, as the passenger
 * fare does. A failed save puts the gold back, so the caller can refuse a
 * purchase or put a sold item back with nothing changed in either store.
 *
 * @param amount Gold to give; negative takes it
 * @return FALSE when the save failed and the gold went back
 */
bool vessel_gold_saved(struct char_data *ch, int amount)
{
  int old_gold;

  if (amount == 0)
  {
    return TRUE;
  }
  old_gold = GET_GOLD(ch);
  award_gold(ch, amount);
  if (save_char_checked(ch, 0))
  {
    return TRUE;
  }
  award_set_points(ch, AWARD_GOLD, old_gold);
  return FALSE;
}

/**
 * Take a purchase's price before the ship's side is written. A database that
 * does not answer refuses the purchase first: the ship's side could be
 * neither written nor put back.
 *
 * @return FALSE when the purchase is refused; the buyer is told, and nothing
 *         was taken
 */
bool vessel_charge(struct char_data *ch, int cost)
{
  if (!mysql_available || conn == NULL || !MYSQL_PING_CONN(conn))
  {
    send_to_char(ch, "The harbor's records cannot be reached; no gold was taken.\r\n");
    return FALSE;
  }
  if (vessel_gold_saved(ch, -cost))
  {
    return TRUE;
  }
  send_to_char(ch, "Your payment could not be recorded; no gold was taken.\r\n");
  return FALSE;
}

/**
 * Give back the price of a purchase whose ship's side could not be written.
 * A refund that cannot be saved stays with the captain: the per-minute save
 * of the player stores it.
 */
void vessel_refund(struct char_data *ch, int amount)
{
  if (amount <= 0)
  {
    return;
  }
  award_gold(ch, amount);
  if (!save_char_checked(ch, 0))
  {
    log("SYSERR: The refund of %d gold to %s is not saved yet; the next save of the player "
        "stores it",
        amount, GET_NAME(ch));
  }
}

/**
 * Record work on a hull whose price is already charged: write her
 * (vessel_save_hull()). If she cannot be written, she goes back to the copy
 * taken before the work, in memory and in her rows (a write of several
 * statements can fail part way), and the price is refunded.
 *
 * A write-back that fails lets the purchase stand instead: her rows may hold
 * it (a write whose reply is lost has been made), so she keeps the work, the
 * price stays paid, and her next save completes her rows. "Nothing was done"
 * is never said of a purchase her rows may still hold.
 *
 * @param before The hull as she was before the work
 * @return FALSE when the purchase was undone; the buyer is told
 */
bool vessel_purchase_recorded(struct char_data *ch, struct greyhawk_ship_data *ship,
                              const struct greyhawk_ship_data *before, int cost)
{
  struct greyhawk_ship_data bought;

  if (vessel_save_hull(ship))
  {
    return TRUE;
  }

  bought = *ship;
  *ship = *before;
  if (!vessel_save_hull(ship))
  {
    /* The write-back may have rewritten part of her rows before it failed. */
    *ship = bought;
    if (!vessel_save_hull(ship))
    {
      log("SYSERR: The work %s paid %d gold for on ship %d could be neither written nor written "
          "back; it stands, and her next save completes her rows",
          GET_NAME(ch), cost, ship->shipnum);
    }
    return TRUE;
  }
  vessel_refund(ch, cost);
  send_to_char(ch, "The harbor office could not record that, so nothing was done");
  if (cost > 0)
  {
    send_to_char(ch, " and your %d gold is returned", cost);
  }
  send_to_char(ch, ".\r\n");
  return FALSE;
}

/**
 * Record a sale off a hull, already taken out of her in memory: write her,
 * then pay. If she cannot be written, the item goes back and nothing is
 * paid. If the gold cannot be saved, it is taken back and the item is put
 * back in memory and in her rows.
 *
 * A put-back that cannot be written lets the sale stand instead: her rows
 * say sold, so the gold is paid in memory and the per-minute save of the
 * player stores it. "Undone" is never said of a sale her rows still hold.
 *
 * @param before The hull as she was before the sale
 * @return FALSE when the sale was undone; the seller is told
 */
bool vessel_sale_recorded(struct char_data *ch, struct greyhawk_ship_data *ship,
                          const struct greyhawk_ship_data *before, int amount)
{
  struct greyhawk_ship_data sold;

  if (!vessel_save_hull(ship))
  {
    *ship = *before;
    if (!vessel_save_hull(ship))
    {
      log("SYSERR: Ship %d could not be written back after a refused sale; her rows may hold "
          "part of it until she is saved again",
          ship->shipnum);
    }
    send_to_char(ch, "The harbor office could not record that, so nothing was sold.\r\n");
    return FALSE;
  }
  if (vessel_gold_saved(ch, amount))
  {
    return TRUE;
  }

  sold = *ship;
  *ship = *before;
  if (vessel_save_hull(ship))
  {
    send_to_char(ch, "Your payment could not be recorded, so the sale is undone.\r\n");
    return FALSE;
  }
  /* The put-back may have rewritten part of her rows before it failed. */
  *ship = sold;
  vessel_save_hull(ship);
  award_gold(ch, amount);
  log("SYSERR: The %d gold %s was paid for a sale off ship %d is not saved yet; the next save "
      "of the player stores it",
      amount, GET_NAME(ch), ship->shipnum);
  return TRUE;
}
