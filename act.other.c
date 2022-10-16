/**************************************************************************
 *  File: act.other.c                                  Part of LuminariMUD *
 *  Usage: Miscellaneous player-level commands.                            *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

/* needed by sysdep.h to allow for definition of <sys/stat.h> */
#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "mail.h" /* for has_mail() */
#include "shop.h"
#include "quest.h"
#include "modify.h"
#include "race.h"
#include "clan.h"
#include "mud_event.h"
#include "craft.h"
#include "treasure.h"
#include "mudlim.h"
#include "spec_abilities.h"
#include "actions.h"
#include "feats.h"
#include "assign_wpn_armor.h"
#include "item.h"
#include "oasis.h"
#include "domains_schools.h"
#include "spells.h"
#include "spell_prep.h"
#include "premadebuilds.h"
#include "staff_events.h"
#include "account.h"

/* some defines for gain/respec */
#define MODE_CLASSLIST_NORMAL 0
#define MODE_CLASSLIST_RESPEC 1
#define MULTICAP 3
#define WILDSHAPE_AFFECTS 4
#define TOG_OFF 0
#define TOG_ON 1

/* debugging */
#define DEBUG_MODE FALSE

/* Local defined utility functions */
/* do_group utility functions */
static void print_group(struct char_data *ch);
static void display_group_list(struct char_data *ch);

// external functions
void save_char_pets(struct char_data *ch);

/*****************/

/* exchange code */

#define EXP_EXCHANGE_RATE 0.00001   /* how much doex exp cost (purchase currency = qp) */
#define ACCEXP_EXCHANGE_RATE 8000.0 /* how much does accexp cost (purchase currency = xp) */
#define GOLD_EXCHANGE_RATE 10.0     /* how much does gold cost (purchase currency = account xp) */
#define QP_EXCHANGE_RATE 300.0      /* how much does qp cost (purchase currency = gold) */

#define SRC_DST_ACCEXP 1
#define SRC_DST_QP 2
#define SRC_DST_GOLD 3
#define SRC_DST_EXP 4
#define NUM_EXCHANGE_TYPES 5 // one more than the last above

const char *exchange_types[NUM_EXCHANGE_TYPES] =
    {
        "",
        "account experience",
        "quest points",
        "gold coins",
        "experience"};

#if 0
/* previous version */
void show_exchange_rates(struct char_data *ch)
{
  send_to_char(ch, "Usage: exchange <currency source> <currency purchasing> <amount to purchase>\r\n");
  send_to_char(ch, "       This command is used to exchange in-game currencies including your "
                   "experience points, gold coins, account experience or quest points.\r\n");
  send_to_char(ch, "       currencies: accexp | qp | gold | exp\r\n");
  send_to_char(ch, "       current exchange rates: accexp: %d, qp: %d, gold: %d, exp: %d",
               (int)ACCEXP_EXCHANGE_RATE,
               (int)QP_EXCHANGE_RATE,
               (int)GOLD_EXCHANGE_RATE,
               (int)EXP_EXCHANGE_RATE);

  return;
}
#endif

void show_exchange_rates(struct char_data *ch)
{
  send_to_char(ch, "Usage: cexchange <currency source> <amount to purchase of exchange currency>\r\n");
  send_to_char(ch, "This command is used to exchange in-game currencies including your "
                   "experience points, gold coins, account experience or quest points.\r\n");
  send_to_char(ch, "The exchange order is: exp -> accexp -> gold -> qp -> (back to start) exp -> ...\r\n");
  send_to_char(ch, "       currencies: exp | accexp | gold | qp \r\n");
  send_to_char(ch, "       current exchange rates (cost in source currency): xp->accexp: %lf, accexp->gold: %lf, gold->qp: %lf, qp->exp: %lf\r\n",
               ACCEXP_EXCHANGE_RATE,
               GOLD_EXCHANGE_RATE,
               QP_EXCHANGE_RATE,
               EXP_EXCHANGE_RATE);

  return;
}

/* currency exchange code, second version, only direct exchange in currency to designated currency */
ACMD(do_cexchange)
{
  char arg1[MAX_STRING_LENGTH] = {'\0'};
  char arg2[MAX_STRING_LENGTH] = {'\0'};
  float amount = 0.0, cost = 0.0;
  int source = 0, xp_excess = 0, xp_profit = 0;

  /*debug*/
  if (DEBUG_MODE)
  {
    if (GET_LEVEL(ch) < LVL_STAFF)
    {
      send_to_char(ch, "Under construction!\r\n");
      return;
    }
  }
  /* end debug */

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1 || !*arg2)
  {
    show_exchange_rates(ch);
    return;
  }

  /* valid arguments */

  if (!is_number(arg2))
  {
    show_exchange_rates(ch);
    send_to_char(ch, "The second argument is invalid, it needs to be a number.\r\n");
    return;
  }

  amount = atoi(arg2);

  if (amount <= 0.0)
  {
    show_exchange_rates(ch);
    send_to_char(ch, "The second argument needs to be above 0.\r\n");
    return;
  }

  if (is_abbrev(arg1, "accexp"))
  {
    source = SRC_DST_ACCEXP;
  }
  else if (is_abbrev(arg1, "qp"))
  {
    source = SRC_DST_QP;
  }
  else if (is_abbrev(arg1, "gold"))
  {
    source = SRC_DST_GOLD;
  }
  else if (is_abbrev(arg1, "exp"))
  {
    source = SRC_DST_EXP;
  }
  else
  {
    show_exchange_rates(ch);
    send_to_char(ch, "The first argument is invalid or missing.\r\n");
    return;
  }

  /* everything should be valid by now, so time for some math */

  switch (source)
  {
  case SRC_DST_EXP:

    /* amount limitation */
    if (amount >= 99999)
    {
      send_to_char(ch, "The second argument (amount) needs to be below 99999.\r\n");
      return;
    }

    /* how much is this transaction going to "cost" */
    cost = ACCEXP_EXCHANGE_RATE * amount;

    /* cap for account xp currently */
    if ((amount + (float)GET_ACCEXP_DESC(ch)) > 99999999.9)
    {
      send_to_char(ch, "Account experience caps at 100mil.\r\n");
      return;
    }

    /* xp has to be overflow! */
    xp_excess = (GET_EXP(ch) - level_exp(ch, (LVL_IMMORT - 1)));

    /* can we afford it? if so, go ahead and make exchange */
    if (xp_excess < cost)
    {
      send_to_char(ch, "You do not have enough experience points, you need an xp-excess of %d (you have %d).\r\n",
                   (int)cost, xp_excess);
      return;
    }

    /* bingo! */
    GET_EXP(ch) -= (int)cost;           /* loss*/
    change_account_xp(ch, (int)amount); /* gain */
    send_to_char(ch, "You exchange %d experience points for %d accexp\r\n", (int)cost, (int)amount);

    break;

  case SRC_DST_ACCEXP:

    /* amount limitation */
    if (amount >= 99999)
    {
      send_to_char(ch, "The second argument (amount) needs to be below 99999.\r\n");
      return;
    }

    /* how much is this transaction going to "cost" */
    cost = GOLD_EXCHANGE_RATE * amount;

    /* can we afford it? if so, go ahead and make exchange */
    if ((float)GET_ACCEXP_DESC(ch) < cost)
    {
      send_to_char(ch, "You do not have enough account exp, you need %d total (you have %d).\r\n",
                   (int)cost, GET_ACCEXP_DESC(ch));
      return;
    }

    /* bingo! */
    change_account_xp(ch, -(int)cost); /* loss */
    increase_gold(ch, (int)amount);    /* gain */
    send_to_char(ch, "You exchange %d account exp for %d gold\r\n", (int)cost, (int)amount);

    break;

  case SRC_DST_GOLD:

    /* amount limitation */
    if (amount >= 99999)
    {
      send_to_char(ch, "The second argument (amount) needs to be below 99999.\r\n");
      return;
    }

    /* how much is this transaction going to "cost" */
    cost = QP_EXCHANGE_RATE * amount;

    /* can we afford it? if so, go ahead and make exchange */
    if ((float)GET_GOLD(ch) < cost)
    {
      send_to_char(ch, "You do not have enough gold on hand, you need %d total on "
                       "hand (not in bank) to make the exchange (you have %d).\r\n",
                   (int)cost, GET_GOLD(ch));
      return;
    }

    /* bingo! */
    increase_gold(ch, -(int)cost);
    GET_QUESTPOINTS(ch) += (int)amount;
    send_to_char(ch, "You exchange %d gold for %d qp\r\n", (int)cost, (int)amount);

    break;

  case SRC_DST_QP:

    if (GET_LEVEL(ch) < (LVL_IMMORT - 1))
    {
      send_to_char(ch, "This exchange is reserved for level 30 chars!\r\n");
      return;
    }

    /* amount limitation */
    if (amount >= 1000000000)
    {
      send_to_char(ch, "The second argument (amount) needs to be below 1 bil.\r\n");
      return;
    }

    /* this is the inverse of our exchange rate */
    xp_profit = ((int)(1.0 / EXP_EXCHANGE_RATE) + 1);

    /* there is a "profit" in this exchange */
    if ((int)amount % xp_profit)
    {
      send_to_char(ch, "To exchange qp to exp, you need to pick increments of %d.\r\n", xp_profit);
      return;
    }

    /* how much is this transaction going to "cost" */
    cost = EXP_EXCHANGE_RATE * amount;

    /* just in case of a math error? */
    if (cost < 1.0)
    {
      send_to_char(ch, "ERR: Report to Staff (cexchange001)\r\n");
      return;
    }

    /* can we afford it? if so, go ahead and make exchange */
    if ((float)GET_QUESTPOINTS(ch) < cost)
    {
      send_to_char(ch, "You do not have enough quest points, you need %d total (you have %d).\r\n",
                   (int)cost, GET_QUESTPOINTS(ch));
      return;
    }

    /* bingo! */
    GET_QUESTPOINTS(ch) -= (int)cost;
    GET_EXP(ch) += (int)amount;
    send_to_char(ch, "You exchange %d quest points for %d exp\r\n", (int)cost, (int)amount);

    break;

  default: /* should never get here */
    show_exchange_rates(ch);
    send_to_char(ch, "Please report to staff: reached default case in exchange switch in do_cexchange.\r\n");
    return;
  }

  /* debugging */
  if (DEBUG_MODE)
  {
    send_to_char(ch, "cexchange() debug 1: source: %d, amount: %lf, cost: %lf.\r\n", source, amount, cost);
  }

  /* done! */
  return;
}

#if 0
/* currency exchange code, first version, unfinished */
ACMD(do_cexchange)
{
  char arg1[MAX_STRING_LENGTH] = {'\0'};
  char arg2[MAX_STRING_LENGTH] = {'\0'};
  char arg3[MAX_STRING_LENGTH] = {'\0'};
  float amount = 0.0, cost = 0.0, pool = 0.0;
  int source = 0, exchange = 0;

  /*temp*/
  if (GET_LEVEL(ch) < LVL_STAFF)
  {
    send_to_char(ch, "Under construction!\r\n");
    return;
  }
  /*TEMP*/

  three_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3));

  if (!*arg1 || !*arg2 || !*arg3)
  {
    send_to_char(ch, "You need to provide more information.\r\n");
    show_exchange_rates(ch);
    return;
  }

  /* valid arguments */

  if (!is_number(arg3))
  {
    send_to_char(ch, "The third argument is invalid, it needs to be a number.\r\n");
    show_exchange_rates(ch);
    return;
  }

  amount = atoi(arg3);

  if (amount <= 0)
  {
    send_to_char(ch, "The third argument needs to be above 0.\r\n");
    show_exchange_rates(ch);
    return;
  }

  if (is_abbrev(arg1, "accexp"))
    source = SRC_DST_ACCEXP;
  else if (is_abbrev(arg1, "qp"))
    source = SRC_DST_QP;
  else if (is_abbrev(arg1, "gold"))
    source = SRC_DST_GOLD;
  else if (is_abbrev(arg1, "exp"))
    source = SRC_DST_EXP;
  else
  {
    send_to_char(ch, "The first argument is invalid.\r\n");
    show_exchange_rates(ch);
    return;
  }

  if (is_abbrev(arg2, "accexp"))
    exchange = SRC_DST_ACCEXP;
  else if (is_abbrev(arg2, "qp"))
    exchange = SRC_DST_QP;
  else if (is_abbrev(arg2, "gold"))
    exchange = SRC_DST_GOLD;
  else if (is_abbrev(arg2, "exp"))
    exchange = SRC_DST_EXP;
  else
  {
    send_to_char(ch, "The second argument is invalid.\r\n");
    show_exchange_rates(ch);
    return;
  }

  if (source == exchange)
  {
    send_to_char(ch, "Your source and desired currency types cannot be the same.\r\n");
    show_exchange_rates(ch);
    return;
  }

  /* everything should be valid by now, so time for some math */

  /* how much is this transaction going to "cost" */
  switch (exchange)
  {
  case SRC_DST_ACCEXP:
    cost = (float)ACCEXP_EXCHANGE_RATE * amount;

    /* cap for account xp currently */
    if ((amount + (float)GET_ACCEXP_DESC(ch)) > 99999999.9)
    {
      send_to_char(ch, "Account experience caps at 100mil.\r\n");
      return;
    }
    break;
  case SRC_DST_QP:
    cost = (float)QP_EXCHANGE_RATE * amount;
    break;
  case SRC_DST_GOLD:
    cost = (float)GOLD_EXCHANGE_RATE * amount;
    break;
  case SRC_DST_EXP:
    cost = (float)EXP_EXCHANGE_RATE * amount;
    break;

  default: /* should never get here */
    show_exchange_rates(ch);
    send_to_char(ch, "Please report to staff: reached default case in 1st exchange switch in do_cexchange.\r\n");
    return;
  }

  /* can we afford it? if so, go ahead and charge 'em */
  switch (source)
  {

  case SRC_DST_ACCEXP:
    pool = cost / ((float)ACCEXP_EXCHANGE_RATE); /* amount we need */

    if (pool < 1.0)
    {
      send_to_char(ch, "You need to increase the amount for this exchange.\r\n");
      return;
    }

    if (GET_ACCEXP_DESC(ch) < pool)
    {
      send_to_char(ch, "You do not have enough account exp, you need %d total.\r\n", (int)pool);
      return;
    }

    /* bingo! */
    change_account_xp(ch, -pool);
    save_account(ch->desc->account);
    send_to_char(ch, "You exchange %d account exp for ", (int)pool);
    break;

  case SRC_DST_QP:
    pool = cost / ((float)QP_EXCHANGE_RATE); /* amount we need */

    if (pool < 1.0)
    {
      send_to_char(ch, "You need to increase the amount for this exchange.\r\n");
      return;
    }

    if (GET_QUESTPOINTS(ch) < pool)
    {
      send_to_char(ch, "You do not have enough quest points, you need %d total.\r\n", (int)pool);
      return;
    }

    /* bingo! */
    GET_QUESTPOINTS(ch) -= pool;
    send_to_char(ch, "You exchange %d quest points for ", (int)pool);
    break;

  case SRC_DST_GOLD:
    pool = cost / ((float)GOLD_EXCHANGE_RATE); /* amount we need */

    if (pool < 1.0)
    {
      send_to_char(ch, "You need to increase the amount for this exchange.\r\n");
      return;
    }

    if (GET_GOLD(ch) < pool)
    {
      send_to_char(ch, "You do not have enough gold on hand, you need %d total on "
                       "hand (not in bank) to make the exchange.\r\n",
                   (int)pool);
      return;
    }

    /* bingo! */
    GET_GOLD(ch) -= pool;
    send_to_char(ch, "You exchange %d gold for ", (int)pool);
    break;

  case SRC_DST_EXP:
    pool = cost / ((float)EXP_EXCHANGE_RATE); /* amount we need */

    if (pool <= 0.0)
    {
      send_to_char(ch, "You need to increase the amount for this exchange.\r\n");
      return;
    }

    if (GET_EXP(ch) < pool)
    {
      send_to_char(ch, "You do not have enough experience points, you need %d total.\r\n", (int)pool);
      return;
    }

    /* bingo! */
    GET_EXP(ch) -= pool;
    send_to_char(ch, "You exchange %d experience points for ", (int)pool);
    break;

  default: /*shouldn't get here*/
    show_exchange_rates(ch);
    send_to_char(ch, "Please report to staff: reached default case in source switch in do_exchange.\r\n");
    return;
  }

  /* final portion of transaction, give them their purchase */
  switch (exchange)
  {
  case SRC_DST_ACCEXP:
    send_to_char(ch, "%d account experience.", (int)amount);
    change_account_xp(ch, (int)amount);
    break;
  case SRC_DST_QP:
    send_to_char(ch, "%d quest points.", (int)amount);
    GET_QUESTPOINTS(ch) += (int)amount;
    break;
  case SRC_DST_GOLD:
    send_to_char(ch, "%d gold coins.", (int)amount);
    GET_GOLD(ch) += (int)amount;
    break;
  case SRC_DST_EXP:
    send_to_char(ch, "%d experience points.", (int)amount);
    GET_EXP(ch) += (int)amount;
    break;
  default: /* should never get here */
    show_exchange_rates(ch);
    send_to_char(ch, "Please report to staff: reached default case in 2nd exchange switch in do_cexchange.\r\n");
    return;
  }

  return;
}
#endif

#undef ACCEXP_EXCHANGE_RATE
#undef QP_EXCHANGE_RATE
#undef GOLD_EXCHANGE_RATE
#undef EXP_EXCHANGE_RATE
#undef SRC_DST_ACCEXP
#undef SRC_DST_QP
#undef SRC_DST_GOLD
#undef SRC_DST_EXP

/*end exchange code */

ACMD(do_nop)
{
  send_to_char(ch, "\r\n");
}

ACMDU(do_handleanimal)
{
  struct char_data *vict = NULL;
  int dc = 0;

  if (!GET_ABILITY(ch, ABILITY_HANDLE_ANIMAL))
  {
    send_to_char(ch, "You have no idea how to do that!\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "You need a target to attempt this.\r\n");
    return;
  }
  else
  {
    /* there is an argument, lets make sure it is valid */
    if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "Whom do you wish to shift?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "You are trying to animal-handle yourself?\r\n");
    return;
  }

  if (!IS_ANIMAL(vict))
  {
    send_to_char(ch, "You can only 'handle animals' that are actually animals.\r\n");
    return;
  }

  /* you have to be higher level to have a chance */
  if (GET_LEVEL(vict) >= GET_LEVEL(ch))
  {
    dc += 999; /* impossible */
  }

  USE_STANDARD_ACTION(ch);

  /* skill check */
  /* dc = hit-dice + 20 */
  dc = GET_LEVEL(vict) + 20;
  if (!skill_check(ch, ABILITY_HANDLE_ANIMAL, dc))
  {
    /* failed, do another check to see if you pissed off the animal */
    if (!skill_check(ch, ABILITY_HANDLE_ANIMAL, dc))
    {
      send_to_char(ch, "You failed to properly train the animal to follow your "
                       "commands, and the animal now seems aggressive.\r\n");
      hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return;
    }
    /* escaped unscathed :P */
    send_to_char(ch, "You failed to properly train the animal to follow your commands.\r\n");
    return;
  }

  /* success! but they now get a saving throw */

  effect_charm(ch, vict, SPELL_CHARM_ANIMAL, CAST_INNATE, GET_LEVEL(ch));

  return;
}

/* innate animate dead ability */
ACMD(do_animatedead)
{
  int uses_remaining = 0;
  struct char_data *mob = NULL;
  mob_vnum mob_num = 0;

  if (!HAS_FEAT(ch, FEAT_ANIMATE_DEAD))
  {
    send_to_char(ch, "You do not know how to animate dead!\r\n");
    return;
  }

  if (IS_HOLY(IN_ROOM(ch)))
  {
    send_to_char(ch, "This place is too holy for such blasphemy!");
    return;
  }

  if (check_npc_followers(ch, NPC_MODE_FLAG, MOB_ANIMATED_DEAD))
  {
    send_to_char(ch, "You can't control more undead!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
  {
    send_to_char(ch, "You are too giddy to have any followers!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_ANIMATE_DEAD)) == 0)
  {
    send_to_char(ch, "You must recover the energy required to animate the dead.\r\n");
    return;
  }
  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  /* success! */

  if (CASTER_LEVEL(ch) >= 30)
    mob_num = MOB_MUMMY;
  else if (CASTER_LEVEL(ch) >= 20)
    mob_num = MOB_GIANT_SKELETON;
  else if (CASTER_LEVEL(ch) >= 10)
    mob_num = MOB_GHOUL;
  else
    mob_num = MOB_ZOMBIE;

  if (!(mob = read_mobile(mob_num, VIRTUAL)))
  {
    send_to_char(ch, "You don't quite remember how to make that creature.\r\n");
    return;
  }
  char_to_room(mob, IN_ROOM(ch));
  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;
  SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);

  act("$n animates a corpse!", FALSE, ch, 0, mob, TO_ROOM);
  act("You animate a corpse!", FALSE, ch, 0, mob, TO_CHAR);
  load_mtrigger(mob);
  add_follower(mob, ch);
  if (GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
    join_group(mob, GROUP(ch));

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_ANIMATE_DEAD);

  USE_STANDARD_ACTION(ch);
}

ACMD(do_abundantstep)
{
  int steps = 0, i = 0, j, repeat = 0, max = 0;
  room_rnum room_tracker = NOWHERE, nextroom = NOWHERE;
  char buf[MAX_INPUT_LENGTH] = {'\0'}, tc = '\0';
  const char *p = NULL;

  if (!HAS_FEAT(ch, FEAT_ABUNDANT_STEP))
  {
    send_to_char(ch, "You do not know that martial art skill!\r\n");
    return;
  }

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You can't focus enough in combat to use this martial art skill!\r\n");
    return;
  }

  if (GET_MOVE(ch) < 300)
  {
    send_to_char(ch, "You are too tired to use this martial art skill!\r\n");
    return;
  }

  /* 3.23.18 Ornir, bugfix. */
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTELEPORT))
  {
    send_to_char(ch, "You are unable to shift to the ethereal plane in this place!\r\n");
    return;
  }

  steps = 0;
  room_tracker = IN_ROOM(ch); /* start the room tracker in current location */
  p = argument;
  max = 5 + CLASS_LEVEL(ch, CLASS_MONK) / 2; /* maximum steps */

  while (p && *p && !isdigit(*p) && !isalpha(*p))
    p++; /* looking for first number or letter */

  if (!p || !*p)
  { /* empty argument */
    send_to_char(ch, "You must give directions from your current location.  Examples:\r\n"
                     "  w w n n e\r\n"
                     "  2w n n e\r\n");
    return;
  }

  /* step through our string */
  while (*p)
  {

    while (*p && !isdigit(*p) && !isalpha(*p))
      p++; /* skipping spaces, and anything not a letter or number */

    if (isdigit(*p))
    { /* value a number?  if so it will be our repeat */
      repeat = atoi(p);

      while (isdigit(*p)) /* get rid of extra numbers */
        p++;
    }
    else /* value isn't a number, so we are moving just a single space */
      repeat = 1;

    /* indication we haven't found a direction to move yet */
    i = -1;

    if (isalpha(*p))
    { /* ok found a letter, and repeat is set */

      for (i = 0; isalpha(*p); i++, p++)
        buf[i] = LOWER(*p); /* turn a string of letters into lower case  buf */

      j = i;       /* how many letters we found */
      tc = buf[i]; /* the non-alpha that terminated us */
      buf[i] = 0;  /* placing a '0' in that last spot in this mini buf */

      for (i = 1; complete_cmd_info[i].command_pointer == do_move && strcmp(complete_cmd_info[i].sort_as, buf); i++)
        ; /* looking for a move command that matches our buf */

      if (complete_cmd_info[i].command_pointer == do_move)
      {
        i = complete_cmd_info[i].subcmd;
      }
      else
        i = -1;
      /* so now i is either our direction to move (define) or -1 */

      buf[j] = tc; /* replace the terminating character in this mini buff */
      // send_to_char(ch, "i: %d\r\n", i);
    }

    if (i > -1)
    { /* we have a direction to move! */
      while (repeat > 0)
      {
        repeat--;

        if (++steps > max) /* reached our limit of steps! */
          break;

        if (!W_EXIT(room_tracker, i))
        { /* is i a valid direction? */
          send_to_char(ch, "Invalid step. Skipping.\r\n");
          break;
        }

        nextroom = W_EXIT(room_tracker, i)->to_room;

        if (nextroom == NOWHERE)
          break;

        /* 3.23.18 Ornir, bugfix. */
        if (ROOM_FLAGGED(nextroom, ROOM_NOTELEPORT) || ROOM_FLAGGED(nextroom, ROOM_HOUSE))
        {
          send_to_char(ch, "You can not access your destination through the ethereal plane!  At least part of the path is obstructed...\r\n");
          return;
        }

        room_tracker = nextroom;
      }
    }
    if (steps > max)
      break;
  } /* finished stepping through the string */

  if (IN_ROOM(ch) != room_tracker)
  {
    send_to_char(ch, "Your will bends reality as you travel through the ethereal plane.\r\n");
    act("$n is suddenly absent.", TRUE, ch, 0, 0, TO_ROOM);

    char_from_room(ch);
    char_to_room(ch, room_tracker);

    act("$n is suddenly present.", TRUE, ch, 0, 0, TO_ROOM);

    look_at_room(ch, 0);
    GET_MOVE(ch) -= 300;
    USE_MOVE_ACTION(ch);
  }
  else
  {
    send_to_char(ch, "You failed!\r\n");
  }

  return;
}

/* ethereal shift - monk epic feat skill/cleric travel domain power/feat, allows
 * shifting of self and group members to the ethereal plane and back */
ACMDU(do_ethshift)
{
  struct char_data *shiftee = NULL;
  room_rnum shift_dest = NOWHERE;
  // int counter = 0;

  skip_spaces(&argument);

  if (!HAS_FEAT(ch, FEAT_OUTSIDER) && !HAS_FEAT(ch, FEAT_ETH_SHIFT))
  {
    send_to_char(ch, "You do not know how!\r\n");
    return;
  }

  if (!*argument)
  {
    shiftee = ch;
  }
  else
  {
    /* there is an argument, lets make sure it is valid */
    if (!(shiftee = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "Whom do you wish to shift?\r\n");
      return;
    }

    /* ok we have a target, is this target grouped? */
    if (!GROUP(ch) || (GROUP(shiftee) != GROUP(ch)))
    {
      send_to_char(ch, "You can only shift someone else if they are in the same "
                       "group as you.\r\n");
      return;
    }
  }

  /* ok make sure it is ok to teleport */
  if (!valid_mortal_tele_dest(shiftee, IN_ROOM(ch), FALSE))
  {
    send_to_char(ch, "Something in this area is stopping your power!\r\n");
    return;
  }

  /* check if shiftee is already on eth */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE))
  {
    send_to_char(ch, "Attempting to shift to the prime plane...\r\n");

    do
    {
      shift_dest = rand_number(0, top_of_world);
      // counter++;
      // send_to_char(ch, "%d | %d, ", counter, shift_dest);
    } while ((ZONE_FLAGGED(GET_ROOM_ZONE(shift_dest), ZONE_ELEMENTAL) ||
              ZONE_FLAGGED(GET_ROOM_ZONE(shift_dest), ZONE_ETH_PLANE) ||
              ZONE_FLAGGED(GET_ROOM_ZONE(shift_dest), ZONE_ASTRAL_PLANE)));

    /* check if shiftee is on prime */
  }
  else if (!ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE) &&
           !ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE) &&
           !ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL))
  {
    send_to_char(ch, "Attempting to shift to the ethereal plane...\r\n");

    do
    {
      shift_dest = rand_number(0, top_of_world);
      // counter++;
      // send_to_char(ch, "%d | %d, ", counter, shift_dest);
    } while (!ZONE_FLAGGED(GET_ROOM_ZONE(shift_dest), ZONE_ETH_PLANE));
  }
  else
  {
    send_to_char(ch, "This power only works when you are on the prime or ethereal "
                     "planes!\r\n");
    return;
  }

  /*
  if (shift_dest >= top_of_world || shift_dest <= -1) {
    send_to_char(ch, "You fail to successfully shift!\r\n");
    return;
  }
   */

  if (!valid_mortal_tele_dest(shiftee, shift_dest, FALSE))
  {
    send_to_char(ch, "Your power is being block at the destination (try again)!\r\n");
    USE_MOVE_ACTION(ch);
    return;
  }

  /* should be successful now */
  send_to_char(shiftee, "You slowly fade out of existence...\r\n");
  act("$n slowly fades out of existence and is gone.",
      FALSE, shiftee, 0, 0, TO_ROOM);
  char_from_room(shiftee);
  char_to_room(shiftee, shift_dest);
  act("$n slowly fades into existence.", FALSE, shiftee, 0, 0, TO_ROOM);
  send_to_char(shiftee, "You slowly fade back into existence...\r\n");
  look_at_room(shiftee, 0);
  entry_memory_mtrigger(shiftee);
  greet_mtrigger(shiftee, -1);
  greet_memory_mtrigger(shiftee);

  USE_FULL_ROUND_ACTION(ch);
}

/* imbue an arrow with one of your spells -zusuk */
ACMD(do_imbuearrow)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *arrow = NULL;
  int spell_num = 0, class = -1;
  int uses_remaining = 0;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  /* need two arguments */
  if (!*arg1)
  {
    send_to_char(ch, "Apply on which ammo?  Usage: imbuearrow <ammo name> <spell name>\r\n");
    return;
  }
  if (!*arg2)
  {
    send_to_char(ch, "Imbue what spell?  Usage: imbuearrow <ammo name> <spell name>\r\n");
    return;
  }

  /* feat related restrictions / daily use */
  if (!HAS_FEAT(ch, FEAT_IMBUE_ARROW))
  {
    send_to_char(ch, "You do not know how!\r\n");
    return;
  }
  if ((uses_remaining = daily_uses_remaining(ch, FEAT_IMBUE_ARROW)) == 0)
  {
    send_to_char(ch, "You must recover the arcane energy required to imbue and arrow.\r\n");
    return;
  }
  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  /* confirm we actually have an arrow targeted */
  arrow = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying);
  if (!arrow)
  {
    if (GET_EQ(ch, WEAR_AMMO_POUCH))
    {
      arrow = get_obj_in_list_vis(ch, arg1, NULL, GET_EQ(ch, WEAR_AMMO_POUCH)->contains);
      if (!arrow)
      {
        send_to_char(ch, "You do not carry that ammo!\r\n");
        return;
      }
    }
    else
    {
      send_to_char(ch, "You do not carry that ammo!\r\n");
      return;
    }
  }
  if (GET_OBJ_TYPE(arrow) != ITEM_MISSILE)
  {
    send_to_char(ch, "You can only imbue ammo!\r\n");
    return;
  }

  /* confirm we have a valid spell */
  spell_num = find_skill_num(arg2);
  if (spell_num < 1 || spell_num > MAX_SPELLS)
  {
    send_to_char(ch, "Imbue with which spell??\r\n");
    return;
  }
  /* make sure arrow isn't already imbued! (obj val 1)*/
  if (GET_OBJ_VAL(arrow, 1))
  {
    send_to_char(ch, "This arrow is already imbued!\r\n");
    return;
  }

  if (spell_prep_gen_check(ch, spell_num, METAMAGIC_NONE) == CLASS_UNDEFINED)
  {
    send_to_char(ch, "You have to have the spell prepared in order to imbue!!\r\n");
    return;
  }

  /* this is where we make sure the spell is a valid one to put on your arrow -zusuk */
  switch (spell_num)
  {

  case SPELL_BURNING_HANDS:
  case SPELL_CALL_LIGHTNING:
  case SPELL_CHILL_TOUCH:
  case SPELL_COLOR_SPRAY:
  case SPELL_CURSE:
  case SPELL_DISPEL_EVIL:
  case SPELL_EARTHQUAKE:
  case SPELL_ENERGY_DRAIN:
  case SPELL_FIREBALL:
  case SPELL_HARM:
  case SPELL_LIGHTNING_BOLT:
  case SPELL_MAGIC_MISSILE:
  case SPELL_POISON:
  case SPELL_SHOCKING_GRASP:
  case SPELL_SLEEP:
  case SPELL_DISPEL_GOOD:
  case SPELL_IDENTIFY:
  case SPELL_CAUSE_LIGHT_WOUNDS:
  case SPELL_CAUSE_MODERATE_WOUNDS:
  case SPELL_CAUSE_SERIOUS_WOUNDS:
  case SPELL_CAUSE_CRITICAL_WOUNDS:
  case SPELL_FLAME_STRIKE:
  case SPELL_DESTRUCTION:
  case SPELL_ICE_STORM:
  case SPELL_BALL_OF_LIGHTNING:
  case SPELL_MISSILE_STORM:
  case SPELL_CHAIN_LIGHTNING:
  case SPELL_METEOR_SWARM:
  case SPELL_GREASE:
  case SPELL_HORIZIKAULS_BOOM:
  case SPELL_ICE_DAGGER:
  case SPELL_NEGATIVE_ENERGY_RAY:
  case SPELL_RAY_OF_ENFEEBLEMENT:
  case SPELL_SCARE:
  case SPELL_SHELGARNS_BLADE:
  case SPELL_WALL_OF_FOG:
  case SPELL_DARKNESS:
  case SPELL_WEB:
  case SPELL_ACID_ARROW:
  case SPELL_DAZE_MONSTER:
  case SPELL_HIDEOUS_LAUGHTER:
  case SPELL_TOUCH_OF_IDIOCY:
  case SPELL_SCORCHING_RAY:
  case SPELL_DEAFNESS:
  case SPELL_ENERGY_SPHERE:
  case SPELL_STINKING_CLOUD:
  case SPELL_HALT_UNDEAD:
  case SPELL_VAMPIRIC_TOUCH:
  case SPELL_HOLD_PERSON:
  case SPELL_DEEP_SLUMBER:
  case SPELL_DAYLIGHT:
  case SPELL_SLOW:
  case SPELL_DISPEL_MAGIC:
  case SPELL_STENCH:
  case SPELL_ACID_SPLASH:
  case SPELL_RAY_OF_FROST:
  case SPELL_BILLOWING_CLOUD:
  case SPELL_RAINBOW_PATTERN:
  case SPELL_FSHIELD_DAM:
  case SPELL_CSHIELD_DAM:
  case SPELL_ASHIELD_DAM:
  case SPELL_ESHIELD_DAM:
  case SPELL_INTERPOSING_HAND:
  case SPELL_WALL_OF_FORCE:
  case SPELL_CLOUDKILL:
  case SPELL_WAVES_OF_FATIGUE:
  case SPELL_SYMBOL_OF_PAIN:
  case SPELL_DOMINATE_PERSON:
  case SPELL_FEEBLEMIND:
  case SPELL_NIGHTMARE:
  case SPELL_MIND_FOG:
  case SPELL_DISMISSAL:
  case SPELL_CONE_OF_COLD:
  case SPELL_TELEKINESIS:
  case SPELL_FIREBRAND:
  case SPELL_DEATHCLOUD:
  case SPELL_FREEZING_SPHERE:
  case SPELL_ACID_FOG:
  case SPELL_EYEBITE:
  case SPELL_ANTI_MAGIC_FIELD:
  case SPELL_GREATER_DISPELLING:
  case SPELL_GRASPING_HAND:
  case SPELL_POWER_WORD_BLIND:
  case SPELL_WAVES_OF_EXHAUSTION:
  case SPELL_MASS_HOLD_PERSON:
  case SPELL_PRISMATIC_SPRAY:
  case SPELL_POWER_WORD_STUN:
  case SPELL_THUNDERCLAP:
  case SPELL_CLENCHED_FIST:
  case SPELL_INCENDIARY_CLOUD:
  case SPELL_HORRID_WILTING:
  case SPELL_IRRESISTIBLE_DANCE:
  case SPELL_MASS_DOMINATION:
  case SPELL_SCINT_PATTERN:
  case SPELL_BANISH:
  case SPELL_SUNBURST:
  case SPELL_WAIL_OF_THE_BANSHEE:
  case SPELL_POWER_WORD_KILL:
  case SPELL_ENFEEBLEMENT:
  case SPELL_WEIRD:
  case SPELL_PRISMATIC_SPHERE:
  case SPELL_IMPLODE:
  case SPELL_ACID:
  case SPELL_INCENDIARY:
  case SPELL_FAERIE_FOG:
  case SPELL_WORD_OF_FAITH:
  case SPELL_DIMENSIONAL_LOCK:
  case SPELL_STORM_OF_VENGEANCE:
  case SPELL_CHARM_ANIMAL:
  case SPELL_FAERIE_FIRE:
  case SPELL_PRODUCE_FLAME:
  case SPELL_FLAME_BLADE:
  case SPELL_FLAMING_SPHERE:
  case SPELL_CALL_LIGHTNING_STORM:
  case SPELL_CONTAGION:
  case SPELL_FROST_BREATHE:
  case SPELL_GAS_BREATHE:
  case SPELL_SPIKE_GROWTH:
  case SPELL_BLIGHT:
  case SPELL_REINCARNATE:
  case SPELL_LIGHTNING_BREATHE:
  case SPELL_SPIKE_STONES:
  case SPELL_INSECT_PLAGUE:
  case SPELL_UNHALLOW:
  case SPELL_WALL_OF_FIRE:
  case SPELL_WALL_OF_THORNS:
  case SPELL_FIRE_SEEDS:
  case SPELL_CREEPING_DOOM:
  case SPELL_FIRE_STORM:
  case SPELL_SUNBEAM:
  case SPELL_FINGER_OF_DEATH:
  case SPELL_BLADE_BARRIER:
  case SPELL_BLADES:
  case SPELL_DOOM:
  case SPELL_WHIRLWIND:
    send_to_char(ch, "You attempt to prep the spell.\r\n");
    break;

  /* no good! */
  default:
    send_to_char(ch, "Not a valid spell to imbue your ammo with.\r\n");
    return;
  }

  class = spell_prep_gen_extract(ch, spell_num, METAMAGIC_NONE);
  if (class == CLASS_UNDEFINED)
  {
    send_to_char(ch, "ERR:  Report BUG771 to an IMM!\r\n");
    log("spell_prep_gen_extract() failed in imbue_arrow");
    return;
  }

  /* SUCCESS! */
  start_daily_use_cooldown(ch, FEAT_IMBUE_ARROW);

  /* store the spell in the arrow, object value 1 */
  GET_OBJ_VAL(arrow, 1) = spell_num;

  /* start wear-off timer for the spell placed on the arrow */
  GET_OBJ_TIMER(arrow) = 8; /* should be 8 hours right? */

  USE_MOVE_ACTION(ch);

  /* message */
  act("$n briefly concentrates and you watch as $p flashes with arcane energy.", FALSE, ch, arrow, 0, TO_ROOM);
  act("You concentrate arcane energy and focus it into $p.", FALSE, ch, arrow, 0, TO_CHAR);
}

/* apply poison to a weapon */
ACMD(do_applypoison)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *poison = NULL, *weapon = NULL;
  int amount = 1;
  bool is_trelux = FALSE;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!HAS_FEAT(ch, FEAT_APPLY_POISON))
  {
    send_to_char(ch, "You do not know how!\r\n");
    return;
  }

  if (!*arg1)
  {
    send_to_char(ch, "Apply what poison? [applypoison <poison name> <weapon-name|ammo-name|primary|offhand|claws>]\r\n");
    return;
  }
  if (!*arg2)
  {
    send_to_char(ch, "Apply on which weapon/ammo?  [applypoison <poison name> <weapon-name|ammo-name|primary|offhand|claws>]\r\n");
    return;
  }

  poison = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying);

  if (!poison)
  {
    send_to_char(ch, "You do not carry that poison!\r\n");
    return;
  }

  if (GET_RACE(ch) == RACE_TRELUX)
  {
    is_trelux = TRUE;
  }

  if (is_abbrev(arg2, "claws") && !is_trelux)
  {
    send_to_char(ch, "Only trelux can do that!!\r\n");
    return;
  }
  else if (!is_abbrev(arg2, "claws") && is_trelux)
  {
    send_to_char(ch, "Trelux must apply poison only to their claws!!\r\n");
    return;
  }

  /* checking for equipped weapons */
  if (is_abbrev(arg2, "claws") && is_trelux)
  {
    ;
  }
  else if (is_abbrev(arg2, "primary"))
  {
    if (GET_EQ(ch, WEAR_WIELD_2H))
      weapon = GET_EQ(ch, WEAR_WIELD_2H);
    else if (GET_EQ(ch, WEAR_WIELD_1))
      weapon = GET_EQ(ch, WEAR_WIELD_1);
  }
  else if (is_abbrev(arg2, "offhand"))
  {
    if (GET_EQ(ch, WEAR_WIELD_OFFHAND))
      weapon = GET_EQ(ch, WEAR_WIELD_OFFHAND);
  }
  else
  {
    weapon = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying);
  }

  if (!weapon && !is_trelux)
  {
    send_to_char(ch, "You do not carry that weapon! [applypoison <poison name> <weapon-name|ammo-name|primary|offhand|claws>]\r\n");
    return;
  }

  if (GET_OBJ_TYPE(poison) != ITEM_POISON)
  {
    send_to_char(ch, "But that is not a poison! [applypoison <poison name> <weapon-name|ammo-name|primary|offhand|claws>]\r\n");
    return;
  }
  if (GET_OBJ_VAL(poison, 2) <= 0)
  {
    send_to_char(ch, "That vial is empty!\r\n");
    return;
  }

  if (is_trelux)
  {
    ;
  }
  else if (GET_OBJ_TYPE(weapon) != ITEM_WEAPON && GET_OBJ_TYPE(weapon) != ITEM_MISSILE)
  {
    send_to_char(ch, "But that is not a weapon/ammo! [applypoison <poison name> <weapon-name|ammo-name|primary|offhand|claws>]\r\n");
    return;
  }
  if (is_trelux && TRLX_PSN_VAL(ch) > 0 && TRLX_PSN_VAL(ch) < NUM_SPELLS)
  {
    send_to_char(ch, "Your claws are already poisoned!\r\n");
    return;
  }
  else if (!is_trelux && weapon->weapon_poison.poison)
  {
    send_to_char(ch, "That weapon/ammo is already poisoned!\r\n");
    return;
  }
  if (!is_trelux && IS_SET(weapon_list[GET_OBJ_VAL(weapon, 0)].weaponFlags, WEAPON_FLAG_RANGED))
  {
    send_to_char(ch, "You can't apply poison to that, try applying directly to the ammo.\r\n");
    return;
  }

  /* 5% of failure */
  if (rand_number(0, 19) || HAS_FEAT(ch, FEAT_POISON_USE))
  {
    char buf1[MEDIUM_STRING] = {'\0'};
    char buf2[MEDIUM_STRING] = {'\0'};

    if (is_trelux)
    {
      TRLX_PSN_VAL(ch) = GET_OBJ_VAL(poison, 0);
      TRLX_PSN_LVL(ch) = GET_OBJ_VAL(poison, 1);
      TRLX_PSN_HIT(ch) = GET_OBJ_VAL(poison, 3);
      snprintf(buf1, sizeof(buf1), "\tnYou carefully apply the contents of %s \tnonto your claws\tn...",
               poison->short_description);
      snprintf(buf2, sizeof(buf2), "$n \tncarefully applies the contents of %s \tnonto $s claws\tn...",
               poison->short_description);
    }
    else
    {
      weapon->weapon_poison.poison_hits = (int)(GET_OBJ_VAL(poison, 3) * (HAS_FEAT(ch, FEAT_POISON_USE) ? 1.5 : 1));
      weapon->weapon_poison.poison = GET_OBJ_VAL(poison, 0);
      weapon->weapon_poison.poison_level = (int)MIN(30, GET_OBJ_VAL(poison, 1) * (HAS_FEAT(ch, FEAT_POISON_USE) ? 1.5 : 1));
      snprintf(buf1, sizeof(buf1), "\tnYou carefully apply the contents of %s \tnonto $p\tn...",
               poison->short_description);
      snprintf(buf2, sizeof(buf2), "$n \tncarefully applies the contents of %s \tnonto $p\tn...",
               poison->short_description);
    }

    act(buf1, FALSE, ch, weapon, 0, TO_CHAR);
    act(buf2, FALSE, ch, weapon, 0, TO_ROOM);

    if (GET_LEVEL(ch) < LVL_IMMORT)
      USE_FULL_ROUND_ACTION(ch);
  }
  else
  {
    /* fail! suppose to be a chance to poison yourself, might change in the future */
    if (is_trelux)
    {
      act("$n fails to apply the \tGpoison\tn onto $s claws.", FALSE, ch, NULL, 0, TO_ROOM);
      act("You fail to \tGpoison\tn your claws.", FALSE, ch, NULL, 0, TO_CHAR);
    }
    else
    {
      act("$n fails to apply the \tGpoison\tn onto $p.", FALSE, ch, weapon, 0, TO_ROOM);
      act("You fail to \tGpoison\tn your $p.", FALSE, ch, weapon, 0, TO_CHAR);
    }
  }

  GET_OBJ_VAL(poison, 2) -= amount;
}

ACMD(do_sorcerer_arcane_apotheosis)
{
  char arg[MAX_INPUT_LENGTH];
  int circle = -1, prep_time = 99;

  if (!HAS_FEAT(ch, FEAT_ARCANE_APOTHEOSIS))
  {
    send_to_char(ch, "You do not have access to this ability.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "What circle?  Usage: apotheosis <spell circle>\r\n");
    return;
  }
  else
  {
    circle = atoi(arg);
    if (circle < 1 || circle > 9)
    {
      send_to_char(ch, "That is an invalid spell circle!\r\n");
      return;
    }
  }

  // Check if they have room - the cap is 9 charges = 3 uses, although they can always top up after using it.
  if (APOTHEOSIS_SLOTS(ch) + circle > 9)
  {
    send_to_char(ch, "That would give you more arcane energy than you can hold!");
    return;
  }

  // Check that they have that spell slot available in their sorcerer queue.
  if (compute_slots_by_circle(ch, CLASS_SORCERER, circle) - count_total_slots(ch, CLASS_SORCERER, circle) <= 0)
  {
    send_to_char(ch, "You don't have any spell slots of that circle remaining!");
    return;
  }

  /* If we got here, we know that everything is good:
   * The circle is valid, the character has slots of that circle, and they can actually
   *   use the command.  Now, spend the spell and increase their pool. */

  prep_time = compute_spells_prep_time(ch, CLASS_SORCERER, circle, false);
  innate_magic_add(ch, CLASS_SORCERER, circle, METAMAGIC_NONE, prep_time, false);

  APOTHEOSIS_SLOTS(ch) += circle;

  act("You focus your arcane power.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n focuses $s arcane power.", FALSE, ch, 0, 0, TO_ROOM);
}

/* bardic performance moved to: bardic_performance.c */

/* this is still being used by NPCs */
void perform_perform(struct char_data *ch)
{
  struct affected_type af[BARD_AFFECTS];
  int level = 0, i = 0, duration = 0;
  struct char_data *tch = NULL;
  long cooldown;

  if (char_has_mud_event(ch, ePERFORM))
  {
    send_to_char(ch, "You must wait longer before you can use this ability "
                     "again.\r\n");
    return;
  }

  if (IS_NPC(ch))
    level = GET_LEVEL(ch);
  else
    level = CLASS_LEVEL(ch, CLASS_BARD) + GET_CHA_BONUS(ch);

  duration = 14 + GET_CHA_BONUS(ch);

  // init affect array
  for (i = 0; i < BARD_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SKILL_PERFORM;
    af[i].duration = duration;
  }

  af[0].location = APPLY_HITROLL;
  af[0].modifier = MAX(1, level / 5);

  af[1].location = APPLY_DAMROLL;
  af[1].modifier = MAX(1, level / 5);

  af[2].location = APPLY_SAVING_WILL;
  af[2].modifier = MAX(1, level / 5);

  af[3].location = APPLY_SAVING_FORT;
  af[3].modifier = MAX(1, level / 5);

  af[4].location = APPLY_SAVING_REFL;
  af[4].modifier = MAX(1, level / 5);

  af[5].location = APPLY_AC_NEW;
  af[5].modifier = 2 + (level / 10);

  af[6].location = APPLY_HIT;
  af[6].modifier = 10 + level;

  USE_STANDARD_ACTION(ch);

  act("$n sings a rousing tune!", FALSE, ch, NULL, NULL, TO_ROOM);
  act("You sing a rousing tune!", FALSE, ch, NULL, NULL, TO_CHAR);

  cooldown = (2 * SECS_PER_MUD_DAY) - (level * 100);
  attach_mud_event(new_mud_event(ePERFORM, ch, NULL), cooldown);

  if (!GROUP(ch))
  {
    if (affected_by_spell(ch, SKILL_PERFORM))
      return;
    SONG_AFF_VAL(ch) = MAX(1, level / 5);
    GET_HIT(ch) += 20 + level;
    for (i = 0; i < BARD_AFFECTS; i++)
      affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);
    return;
  }

  while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) !=
         NULL)
  {
    if (IN_ROOM(tch) != IN_ROOM(ch))
      continue;
    if (affected_by_spell(tch, SKILL_PERFORM))
      continue;
    SONG_AFF_VAL(tch) = MAX(1, level / 5);
    GET_HIT(tch) += 20 + level;
    for (i = 0; i < BARD_AFFECTS; i++)
      affect_join(tch, af + i, FALSE, FALSE, FALSE, FALSE);
    act("A song from $n enhances you!", FALSE, ch, NULL, tch, TO_VICT);
  }
}

/* this has been replaced in bardic_performance.c */

/*
ACMDU(do_perform) {

  if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_BARDIC_MUSIC)) {
    send_to_char(ch, "You don't know how to perform.\r\n");
    return;
  }

  perform_perform(ch);
}
 */

void perform_call(struct char_data *ch, int call_type, int level)
{
  int i = 0;
  struct follow_type *k = NULL, *next = NULL;
  struct char_data *mob = NULL;
  mob_vnum mob_num = NOBODY;
  /* tests for whether you can actually call a companion */

  /* companion here already ? */
  for (k = ch->followers; k; k = next)
  {
    next = k->next;
    if (IS_NPC(k->follower) && AFF_FLAGGED(k->follower, AFF_CHARM) &&
        MOB_FLAGGED(k->follower, call_type))
    {
      if (IN_ROOM(ch) == IN_ROOM(k->follower))
      {
        send_to_char(ch, "Your companion has already been summoned!\r\n");
        return;
      }
      else
      {
        char_from_room(k->follower);

        if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
        {
          X_LOC(k->follower) = world[IN_ROOM(ch)].coords[0];
          Y_LOC(k->follower) = world[IN_ROOM(ch)].coords[1];
        }

        char_to_room(k->follower, IN_ROOM(ch));
        act("$n calls $N!", FALSE, ch, 0, k->follower, TO_ROOM);
        act("You call forth $N!", FALSE, ch, 0, k->follower, TO_CHAR);
        return;
      }
    }
  }

  /* doing two disqualifying tests in this switch block */
  switch (call_type)
  {
  case MOB_C_ANIMAL:
    /* do they even have a valid selection yet? */
    if (!IS_NPC(ch) && GET_ANIMAL_COMPANION(ch) <= 0)
    {
      send_to_char(ch, "You have to select your companion via the 'study' "
                       "command.\r\n");
      return;
    }

    /* is the ability on cooldown? */
    if (char_has_mud_event(ch, eC_ANIMAL))
    {
      send_to_char(ch, "You must wait longer before you can use this ability again.\r\n");
      return;
    }

    /* todo:  seriously, fix this */
    if (!(mob_num = GET_ANIMAL_COMPANION(ch)))
      mob_num = 63; // meant for npc's

    break;
  case MOB_C_FAMILIAR:
    /* do they even have a valid selection yet? */
    if (!IS_NPC(ch) && GET_FAMILIAR(ch) <= 0)
    {
      send_to_char(ch, "You have to select your companion via the 'study' "
                       "command.\r\n");
      return;
    }

    /* is the ability on cooldown? */
    if (char_has_mud_event(ch, eC_FAMILIAR))
    {
      send_to_char(ch, "You must wait longer before you can use this ability again.\r\n");
      return;
    }

    mob_num = GET_FAMILIAR(ch);

    break;

  case MOB_SHADOW:
    /* do they even have a valid selection yet? */
    if (IS_NPC(ch) || !HAS_REAL_FEAT(ch, FEAT_SUMMON_SHADOW))
    {
      send_to_char(ch, "You cannot summon a shadow");
      return;
    }

    /* is the ability on cooldown? */
    if (char_has_mud_event(ch, eSUMMONSHADOW))
    {
      send_to_char(ch, "You must wait longer before you can use this ability again.\r\n");
      return;
    }

    mob_num = 60289;

    break;

  case MOB_C_MOUNT:
    /* for now just one selection for paladins, soon to be changed */
    if (HAS_FEAT(ch, FEAT_EPIC_MOUNT))
    {
      if (GET_SIZE(ch) < SIZE_MEDIUM)
        GET_MOUNT(ch) = MOB_EPIC_PALADIN_MOUNT_SMALL;
      else
        GET_MOUNT(ch) = MOB_EPIC_PALADIN_MOUNT;
    }
    else
    {
      if (GET_SIZE(ch) < SIZE_MEDIUM)
        GET_MOUNT(ch) = MOB_PALADIN_MOUNT_SMALL;
      else
        GET_MOUNT(ch) = MOB_PALADIN_MOUNT;
    }

    /* do they even have a valid selection yet? */
    if (GET_MOUNT(ch) <= 0)
    {
      send_to_char(ch, "You have to select your companion via the 'study' "
                       "command.\r\n");
      return;
    }

    /* is the ability on cooldown? */
    if (char_has_mud_event(ch, eC_MOUNT))
    {
      send_to_char(ch, "You must wait longer before you can use this ability again.\r\n");
      return;
    }

    mob_num = GET_MOUNT(ch);

    break;
  }

  /* couple of dummy checks */
  if ((mob_num <= 0 || mob_num > 99) && mob_num != 60289) // zone 0 for mobiles, except for the shadow
    return;
  if (level >= LVL_IMMORT)
    level = LVL_IMMORT - 1;

  /* passed all the tests, bring on the companion! */
  /* HAVE to make sure the mobiles for the lists of
     companions / familiars / etc have the proper
     MOB_C_x flag set via medit */
  if (!(mob = read_mobile(mob_num, VIRTUAL)))
  {
    send_to_char(ch, "You don't quite remember how to call that creature.\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
  {
    X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
    Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
  }

  char_to_room(mob, IN_ROOM(ch));
  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  /* setting mob strength according to 'level' */
  switch (call_type)
  {
  case MOB_C_ANIMAL:
    GET_LEVEL(mob) = MIN(20, level);
    if (HAS_FEAT(ch, FEAT_BOON_COMPANION))
      level += 5;
    autoroll_mob(mob, true, true);
    GET_REAL_MAX_HIT(mob) += 20;
    GET_HIT(mob) = GET_REAL_MAX_HIT(mob);
    break;
  case MOB_SHADOW:
    GET_LEVEL(mob) = MIN(25, level);
    autoroll_mob(mob, true, true);
    GET_REAL_MAX_HIT(mob) += 20;
    GET_HIT(mob) = GET_REAL_MAX_HIT(mob);
    break;
  case MOB_C_FAMILIAR:
    GET_REAL_MAX_HIT(mob) += 10;
    for (i = 0; i < level; i++)
      GET_REAL_MAX_HIT(mob) += dice(2, 4) + 1;
    if (HAS_FEAT(ch, FEAT_IMPROVED_FAMILIAR))
    {
      GET_REAL_MAX_HIT(mob) += HAS_FEAT(ch, FEAT_IMPROVED_FAMILIAR) * 10;
      GET_REAL_AC(mob) += HAS_FEAT(ch, FEAT_IMPROVED_FAMILIAR) * 10;
      GET_REAL_STR(mob) += HAS_FEAT(ch, FEAT_IMPROVED_FAMILIAR);
      GET_REAL_DEX(mob) += HAS_FEAT(ch, FEAT_IMPROVED_FAMILIAR);
      GET_REAL_CON(mob) += HAS_FEAT(ch, FEAT_IMPROVED_FAMILIAR);
    }
    GET_LEVEL(mob) = MIN(20, level);
    break;
  case MOB_C_MOUNT:
    if (mob_num == MOB_EPIC_PALADIN_MOUNT || mob_num == MOB_EPIC_PALADIN_MOUNT_SMALL)
      GET_LEVEL(mob) = MIN(27, level);
    else
      GET_LEVEL(mob) = MIN(20, level);
    autoroll_mob(mob, true, true);
    GET_REAL_MAX_HIT(mob) += 20;
    GET_HIT(mob) = GET_REAL_MAX_HIT(mob);
    GET_REAL_SIZE(mob) = GET_SIZE(ch) + 1;
    GET_MOVE(mob) = GET_REAL_MAX_MOVE(mob) = 500;
    break;
  }
  GET_HIT(mob) = GET_REAL_MAX_HIT(mob);

  affect_total(mob);

  SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);
  act("$n calls $N!", FALSE, ch, 0, mob, TO_ROOM);
  act("You call forth $N!", FALSE, ch, 0, mob, TO_CHAR);
  load_mtrigger(mob);
  add_follower(mob, ch);
  if (GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
    join_group(mob, GROUP(ch));
  save_char_pets(ch);

  /* finally attach cooldown, approximately 14 minutes right now */
  if (call_type == MOB_C_ANIMAL)
  {
    attach_mud_event(new_mud_event(eC_ANIMAL, ch, NULL), 4 * SECS_PER_MUD_DAY);
  }
  else if (call_type == MOB_C_FAMILIAR)
  {
    attach_mud_event(new_mud_event(eC_FAMILIAR, ch, NULL), 4 * SECS_PER_MUD_DAY);
  }
  else if (call_type == MOB_C_MOUNT)
  {
    attach_mud_event(new_mud_event(eC_MOUNT, ch, NULL), 4 * SECS_PER_MUD_DAY);
  }
  else if (call_type == MOB_SHADOW)
  {
    attach_mud_event(new_mud_event(eSUMMONSHADOW, ch, NULL), 4 * SECS_PER_MUD_DAY);
  }

  send_to_char(ch, "You can 'call' your companion even if you get separated.  "
                   "You can also 'dismiss' your companion to reduce your cooldown drastically.\r\n");
}

ACMD(do_call)
{
  int call_type = -1, level = 0;

  skip_spaces_c(&argument);

  /* call types
     MOB_C_ANIMAL -> animal companion
     MOB_C_FAMILIAR -> familiar
     MOB_C_MOUNT -> paladin mount
     MOB_SHADOW -> shadow for shadow dancer
   */
  if (!argument)
  {
    send_to_char(ch, "Usage:  call <companion/familiar/mount/shadow>\r\n");
    return;
  }
  else if (is_abbrev(argument, "companion"))
  {
    level = CLASS_LEVEL(ch, CLASS_DRUID);
    if (CLASS_LEVEL(ch, CLASS_RANGER) >= 4)
      level += CLASS_LEVEL(ch, CLASS_RANGER) - 3;

    if (!HAS_FEAT(ch, FEAT_ANIMAL_COMPANION))
    {
      send_to_char(ch, "You do not have an animal companion.\r\n");
      return;
    }

    if (level <= 0)
    {
      send_to_char(ch, "You are too inexperienced to use this ability!\r\n");
      return;
    }
    call_type = MOB_C_ANIMAL;
  }
  else if (is_abbrev(argument, "familiar"))
  {
    level = CLASS_LEVEL(ch, CLASS_SORCERER) + CLASS_LEVEL(ch, CLASS_WIZARD);

    if (!HAS_FEAT(ch, FEAT_SUMMON_FAMILIAR))
    {
      send_to_char(ch, "You do not have a familiar.\r\n");
      return;
    }

    if (level <= 0)
    {
      send_to_char(ch, "You are too inexperienced to use this ability!\r\n");
      return;
    }

    call_type = MOB_C_FAMILIAR;
  }
  else if (is_abbrev(argument, "mount"))
  {
    level = CLASS_LEVEL(ch, CLASS_PALADIN);

    if (!HAS_FEAT(ch, FEAT_CALL_MOUNT))
    {
      send_to_char(ch, "You do not have a mount that you can call.\r\n");
      return;
    }

    if (level <= 0)
    {
      send_to_char(ch, "You are too inexperienced to use this ability!\r\n");
      return;
    }

    call_type = MOB_C_MOUNT;
  }
  else if (is_abbrev(argument, "shadow"))
  {
    level = MIN(GET_LEVEL(ch), CLASS_LEVEL(ch, CLASS_SHADOWDANCER) + 8);

    if (HAS_REAL_FEAT(ch, FEAT_SHADOW_MASTER))
    {
      level += dice(1, 4) + 1;
    }

    if (!HAS_REAL_FEAT(ch, FEAT_SUMMON_SHADOW))
    {
      send_to_char(ch, "You do not have a shadow that you can call.\r\n");
      return;
    }

    if (level <= 0)
    {
      send_to_char(ch, "You are too inexperienced to use this ability!\r\n");
      return;
    }

    call_type = MOB_SHADOW;
  }
  else
  {
    send_to_char(ch, "Usage:  call <companion/familiar/mount/shadow>\r\n");
    return;
  }

  perform_call(ch, call_type, level);
}

ACMD(do_purify)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;
  int uses_remaining = 0;

  if (IS_NPC(ch) || !HAS_FEAT(ch, FEAT_REMOVE_DISEASE))
  {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "Whom do you want to purify?\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_REMOVE_DISEASE)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to remove disease.\r\n");
    return;
  }

  if (!IS_AFFECTED(vict, AFF_DISEASE) &&
      !affected_by_spell(vict, SPELL_EYEBITE))
  {
    send_to_char(ch, "Your target isn't diseased!\r\n");
    return;
  }

  send_to_char(ch, "Your hands flash \tWbright white\tn as you reach out...\r\n");
  act("You are \tWhealed\tn by $N!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n \tWheals\tn $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  if (affected_by_spell(vict, SPELL_EYEBITE))
    affect_from_char(vict, SPELL_EYEBITE);
  if (IS_AFFECTED(vict, AFF_DISEASE))
    REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_DISEASE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_REMOVE_DISEASE);

  update_pos(vict);
}

/* this is a temporary command, a simple cheesy way
   to get rid of your followers in a bind */
ACMD(do_dismiss)
{
  struct follow_type *k = NULL, *next = NULL;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;
  int found = 0;
  struct mud_event_data *pMudEvent = NULL;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "You dismiss your non-present followers.\r\n");
    snprintf(buf, sizeof(buf), "$n dismisses $s non present followers.");
    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    for (k = ch->followers; k; k = next)
    {
      next = k->next;

      if (IN_ROOM(ch) != IN_ROOM(k->follower))
      {
        if (IS_PET(k->follower))
        {
          extract_char(k->follower);
        }
      }
    }

    save_char_pets(ch);

    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "Whom do you want to dismiss?\r\n");
    return;
  }

  /* is this follower the target? */
  if ((vict->master == ch))
  {
    /* is this follower charmed? */
    if (AFF_FLAGGED(vict, AFF_CHARM))
    {
      /* is this a special companion?
       * if so, modify event cooldown (if it exits) */
      if (MOB_FLAGGED(vict, MOB_C_ANIMAL))
      {
        if ((pMudEvent = char_has_mud_event(ch, eC_ANIMAL)) &&
            event_time(pMudEvent->pEvent) > (59 * PASSES_PER_SEC))
        {
          change_event_duration(ch, eC_ANIMAL, (59 * PASSES_PER_SEC));
        }
      }
      else if (MOB_FLAGGED(vict, MOB_C_FAMILIAR))
      {
        if ((pMudEvent = char_has_mud_event(ch, eC_FAMILIAR)) &&
            event_time(pMudEvent->pEvent) > (59 * PASSES_PER_SEC))
        {
          change_event_duration(ch, eC_FAMILIAR, (59 * PASSES_PER_SEC));
        }
      }
      else if (MOB_FLAGGED(vict, MOB_C_MOUNT))
      {
        if ((pMudEvent = char_has_mud_event(ch, eC_MOUNT)) &&
            event_time(pMudEvent->pEvent) > (59 * PASSES_PER_SEC))
        {
          change_event_duration(ch, eC_MOUNT, (59 * PASSES_PER_SEC));
        }
      }
      else if (MOB_FLAGGED(vict, MOB_SHADOW))
      {
        if ((pMudEvent = char_has_mud_event(ch, eSUMMONSHADOW)) &&
            event_time(pMudEvent->pEvent) > (59 * PASSES_PER_SEC))
        {
          change_event_duration(ch, eSUMMONSHADOW, (59 * PASSES_PER_SEC));
        }
      }

      extract_char(vict);
      found = 1;
    }
  }

  if (!found)
  {
    send_to_char(ch, "Your target is not valid!\r\n");
    return;
  }
  else
  {
    act("With a wave of your hand, you dismiss $N.",
        FALSE, ch, 0, vict, TO_CHAR);
    act("$n waves at you, indicating your dismissal.",
        FALSE, ch, 0, vict, TO_VICT);
    act("With a wave, $n dismisses $N.",
        TRUE, ch, 0, vict, TO_NOTVICT);

    save_char_pets(ch);
  }
}

/* recharge allows the refilling of charges for wands and staves
   for a price */
ACMD(do_recharge)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj = NULL;
  int maxcharge = 0, mincharge = 0, chargeval = 0;

  if (!IS_NPC(ch))
    ;
  else
  {
    send_to_char(ch, "You don't know how to do that!\r\n");
    return;
  }

  argument = one_argument(argument, buf, sizeof(buf));

  if (!(obj = get_obj_in_list_vis(ch, buf, NULL, ch->carrying)))
  {
    send_to_char(ch, "You don't have that!\r\n");
    return;
  }

  if (GET_OBJ_TYPE(obj) != ITEM_STAFF &&
      GET_OBJ_TYPE(obj) != ITEM_WAND)
  {
    send_to_char(ch, "Are you daft!  You can't recharge that!\r\n");
    return;
  }

  if (((GET_OBJ_TYPE(obj) == ITEM_STAFF) && !HAS_FEAT(ch, FEAT_CRAFT_STAFF)) ||
      ((GET_OBJ_TYPE(obj) == ITEM_WAND) && !HAS_FEAT(ch, FEAT_CRAFT_WAND)))
  {
    send_to_char(ch, "You don't know how to recharge that.\r\n");
    return;
  }

  if (GET_GOLD(ch) < 5000)
  {
    send_to_char(ch, "You don't have enough gold on hand!\r\n");
    return;
  }

  maxcharge = GET_OBJ_VAL(obj, 1);
  mincharge = GET_OBJ_VAL(obj, 2);

  if (mincharge < maxcharge)
  {
    chargeval = maxcharge - mincharge;
    GET_OBJ_VAL(obj, 2) += chargeval;
    GET_GOLD(ch) -= 5000;
    send_to_char(ch, "The %s glows blue for a moment.\r\n", (GET_OBJ_TYPE(obj) == ITEM_STAFF ? "staff" : "wand"));
    snprintf(buf, sizeof(buf), "The item now has %d charges remaining.\r\n", maxcharge);
    send_to_char(ch, buf);
    act("$p glows with a subtle blue light as $n recharges it.",
        FALSE, ch, obj, 0, TO_ROOM);
  }
  else
  {
    send_to_char(ch, "The item does not need recharging.\r\n");
  }
  return;
}

ACMD(do_mount)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Mount who?\r\n");
    return;
  }
  else if (!(vict = get_char_room_vis(ch, arg, NULL)))
  {
    send_to_char(ch, "There is no-one by that name here.\r\n");
    return;
  }
  else if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "Ehh... no.\r\n");
    return;
  }
  else if (RIDING(ch) || RIDDEN_BY(ch))
  {
    send_to_char(ch, "You are already mounted.\r\n");
    return;
  }
  else if (RIDING(vict) || RIDDEN_BY(vict))
  {
    send_to_char(ch, "It is already mounted.\r\n");
    return;
  }
  else if (GET_LEVEL(ch) < LVL_IMMORT && IS_NPC(vict) && !MOB_FLAGGED(vict, MOB_MOUNTABLE))
  {
    send_to_char(ch, "You can't mount that!\r\n");
    return;
  }
  else if (!GET_ABILITY(ch, ABILITY_RIDE))
  {
    send_to_char(ch, "First you need to learn *how* to mount.\r\n");
    return;
  }
  else if (GET_SIZE(vict) < (GET_SIZE(ch) + 1))
  {
    send_to_char(ch, "The mount is too small for you!\r\n");
    return;
  }
  else if (GET_SIZE(vict) > (GET_SIZE(ch) + 2))
  {
    send_to_char(ch, "The mount is too large for you!\r\n");
    return;
  }
  else if ((compute_ability(ch, ABILITY_RIDE) + 1) <= rand_number(1, GET_LEVEL(vict)))
  {
    act("You try to mount $N, but slip and fall off.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n tries to mount you, but slips and falls off.", FALSE, ch, 0, vict, TO_VICT);
    act("$n tries to mount $N, but slips and falls off.", TRUE, ch, 0, vict, TO_NOTVICT);
    USE_MOVE_ACTION(ch);
    return;
  }

  act("You mount $N.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n mounts you.", FALSE, ch, 0, vict, TO_VICT);
  act("$n mounts $N.", TRUE, ch, 0, vict, TO_NOTVICT);
  mount_char(ch, vict);

  USE_MOVE_ACTION(ch);

  if (IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_TAMED) &&
      (compute_ability(ch, ABILITY_RIDE) + d20(ch)) <= rand_number(1, GET_LEVEL(vict)))
  {
    act("$N suddenly bucks upwards, throwing you violently to the ground!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n is thrown to the ground as $N violently bucks!", TRUE, ch, 0, vict, TO_NOTVICT);
    act("You buck violently and throw $n to the ground.", FALSE, ch, 0, vict, TO_VICT);
    dismount_char(ch);
    USE_MOVE_ACTION(ch);
  }
}

ACMD(do_dismount)
{
  if (!RIDING(ch))
  {
    send_to_char(ch, "You aren't even riding anything.\r\n");
    return;
  }
  else if (SECT(ch->in_room) == SECT_WATER_NOSWIM && !has_boat(ch, IN_ROOM(ch)))
  {
    send_to_char(ch, "Yah, right, and then drown...\r\n");
    return;
  }

  act("You dismount $N.", FALSE, ch, 0, RIDING(ch), TO_CHAR);
  act("$n dismounts from you.", FALSE, ch, 0, RIDING(ch), TO_VICT);
  act("$n dismounts $N.", TRUE, ch, 0, RIDING(ch), TO_NOTVICT);
  dismount_char(ch);
}

ACMD(do_buck)
{

  if (!RIDDEN_BY(ch))
  {
    send_to_char(ch, "You're not even being ridden!\r\n");
    return;
  }
  else if (AFF_FLAGGED(ch, AFF_TAMED))
  {
    send_to_char(ch, "But you're tamed!\r\n");
    return;
  }

  act("You quickly buck, throwing $N to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_CHAR);
  act("$n quickly bucks, throwing you to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_VICT);
  act("$n quickly bucks, throwing $N to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_NOTVICT);

  send_to_char(RIDDEN_BY(ch), "You hit the ground hard!\r\n");
  change_position(RIDDEN_BY(ch), POS_SITTING);
  start_action_cooldown(ch, atSTANDARD, 7 RL_SEC);
  dismount_char(ch);
}

ACMD(do_tame)
{
  char arg[MAX_INPUT_LENGTH];
  struct affected_type af;
  struct char_data *vict;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Tame who?\r\n");
    return;
  }
  else if (!(vict = get_char_room_vis(ch, arg, NULL)))
  {
    send_to_char(ch, "They're not here.\r\n");
    return;
  }
  else if (GET_LEVEL(ch) < LVL_IMMORT && IS_NPC(vict) && !MOB_FLAGGED(vict, MOB_MOUNTABLE))
  {
    send_to_char(ch, "You can't do that to them.\r\n");
    return;
  }
  else if (!GET_ABILITY(ch, ABILITY_RIDE))
  {
    send_to_char(ch, "You don't even know how to tame something.\r\n");
    return;
  }
  else if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "You can't do that.\r\n");
    return;
  }
  else if (GET_SKILL(ch, ABILITY_RIDE) <= rand_number(1, GET_LEVEL(vict)))
  {
    send_to_char(ch, "You fail to tame it.\r\n");

    /* todo:  probably should be some sort of punishment for failing! */

    return;
  }

  new_affect(&af);
  af.duration = 50 + compute_ability(ch, ABILITY_HANDLE_ANIMAL) * 4;
  SET_BIT_AR(af.bitvector, AFF_TAMED);
  affect_to_char(vict, &af);

  act("You tame $N.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n tames you.", FALSE, ch, 0, vict, TO_VICT);
  act("$n tames $N.", FALSE, ch, 0, vict, TO_NOTVICT);
}

/* the guts of the respec mechanic */
void respec_engine(struct char_data *ch, int class, char *arg, bool silent)
{
  struct affected_type af;

  /* in the clear! */
  int tempXP = GET_EXP(ch);

  GET_CLASS(ch) = class;
  GET_PREMADE_BUILD_CLASS(ch) = CLASS_UNDEFINED;

  if (GET_REAL_RACE(ch) != RACE_LICH && GET_REAL_RACE(ch) != RACE_VAMPIRE)
  {
    if (*arg && is_abbrev(arg, "premade"))
      GET_PREMADE_BUILD_CLASS(ch) = class;
  }

  /* Make sure that players can't make wildshaped forms permanent.*/
  SUBRACE(ch) = 0;
  IS_MORPHED(ch) = 0;
  GET_DISGUISE_RACE(ch) = -1; // 0 is human

  if (affected_by_spell(ch, SKILL_WILDSHAPE))
  {
    affect_from_char(ch, SKILL_WILDSHAPE);
    send_to_char(ch, "You return to your normal form.\r\n");
  }

  // removed because we checked for group and followers above already -- gicker apr 8 2021
  // leave_group(ch);
  // stop_follower(ch);

  do_start(ch);
  HAS_SET_STATS_STUDY(ch) = FALSE;
  GET_EXP(ch) = tempXP;

  new_affect(&af);
  af.spell = AFFECT_RECENTLY_RESPECED;
  af.location = APPLY_NONE;
  af.modifier = 0;
  af.duration = 100; // 10 minutes
  affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);

  if (!silent)
  {
    send_to_char(ch, "\tMYou have respec'd!\tn\r\n");
    if (GET_PREMADE_BUILD_CLASS(ch) != CLASS_UNDEFINED)
      send_to_char(ch, "\tMYou have chosen a premade %s build\tn\r\n", class_list[class].name);
    send_to_char(ch, "\tDType 'gain' to regain your level(s)...\tn\r\n");
  }

  save_char(ch, 1);

  return;
}

/* reset character to level 1, but preserve xp */
ACMD(do_respec)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  int class = -1;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (GROUP(ch) || ch->master || ch->followers)
  {
    send_to_char(ch, "You cannot be part of a group, be following someone, or have followers of your own to respec.\r\n"
                     "You can dismiss npc followers with the 'dismiss' command.\r\n");
    return;
  }

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!*arg)
  {
    send_to_char(ch, "You need to select a starting class to respec to, as well as specify a premade build "
                     "if you desire.  Syntax for a custom build is respec (class name).  Syntax for a premade "
                     "build is respec (class name) premade.\r\n\r\n"
                     " Here are your options:\r\n");
    display_all_classes(ch);
    return;
  }
  else
  {
    class = get_class_by_name(arg);
    if (class == -1)
    {
      send_to_char(ch, "Invalid class.\r\n");
      display_all_classes(ch);
      return;
    }
    if (class >= NUM_CLASSES ||
        !class_is_available(ch, class, MODE_CLASSLIST_RESPEC, NULL))
    {
      send_to_char(ch, "That is not a valid class!\r\n");
      display_all_classes(ch);
      return;
    }
    if (GET_LEVEL(ch) < 2)
    {
      send_to_char(ch, "You need to be at least 2nd level to respec...\r\n");
      return;
    }
    if (GET_LEVEL(ch) >= LVL_IMMORT)
    {
      send_to_char(ch, "Sorry staff can't respec...\r\n");
      return;
    }
    if (CLSLIST_LOCK(class))
    {
      send_to_char(ch, "You cannot respec into a prestige class, you must respec "
                       "to a base class and meet all the prestige-class requirements to "
                       "advance in it.\r\n");
      return;
    }
  }

  respec_engine(ch, class, arg2, FALSE);
}

/* level advancement, with multi-class support */
ACMD(do_gain)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int is_altered = FALSE, num_levels = 0;
  int class = -1, i = 0, classCount = 0;

  /* easy outs */
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* limitations, have to be in 'normal' form to advance */
  if (GET_DISGUISE_RACE(ch) || IS_MORPHED(ch))
  {
    send_to_char(ch, "You have to remove disguises, wildshape and/or polymorph "
                     "before advancing.\r\n");
    return;
  }

  /* level limits */
  if (GET_LEVEL(ch) >= LVL_IMMORT - 1)
  {
    send_to_char(ch, "You have reached the level limit! You can not go above level %d!\r\n", LVL_IMMORT - 1);
    return;
  }

  /* have enough xp? */
  if (!(GET_LEVEL(ch) < LVL_IMMORT - CONFIG_NO_MORT_TO_IMMORT &&
        GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1)))
  {
    send_to_char(ch, "You are not experienced enough to gain a level.\r\n");
    return;
  }

  if (GET_PREMADE_BUILD_CLASS(ch) < 0)
    one_argument(argument, arg, sizeof(arg));
  else
    snprintf(arg, MAX_INPUT_LENGTH, "%s", class_list[GET_PREMADE_BUILD_CLASS(ch)].name);

  if (!*arg)
  {
    send_to_char(ch, "You may gain a level in one of the following classes:\r\n\r\n");
    display_all_classes(ch);
    send_to_char(ch, "Type 'gain <classname>' to gain a level in the chosen class.\r\n");
    return;
  }
  else
  {
    class = get_class_by_name(arg);
    if (class == -1)
    {
      send_to_char(ch, "Invalid class.\r\n");
      display_all_classes(ch);
      return;
    }

    if (class < 0 || class >= NUM_CLASSES ||
        !class_is_available(ch, class, MODE_CLASSLIST_NORMAL, NULL))
    {
      send_to_char(ch, "That is not a valid class!\r\n");
      display_all_classes(ch);
      return;
    }

    if (class == CLASS_CLERIC && CLASS_LEVEL(ch, CLASS_INQUISITOR))
    {
      send_to_char(ch, "You cannot take levels in cleric if you have levels in inquisitor.\r\n");
      return;
    }
    if (class == CLASS_INQUISITOR && CLASS_LEVEL(ch, CLASS_CLERIC))
    {
      send_to_char(ch, "You cannot take levels in inquisitor if you have levels in cleric.\r\n");
      return;
    }

    // multi class cap
    for (i = 0; i < MAX_CLASSES; i++)
    {
      if (CLASS_LEVEL(ch, i) && i != class)
        classCount++;
    }
    if (classCount >= MULTICAP)
    {
      send_to_char(ch, "Current cap on multi-classing is %d.\r\n", MULTICAP);
      send_to_char(ch, "Please select one of the classes you already have!\r\n");
      return;
    }

    /* cap for class ranks */
    int max_class_level = CLSLIST_MAXLVL(class);

    if (max_class_level == -1)
      max_class_level = LVL_IMMORT - 1;

    if (CLASS_LEVEL(ch, class) >= max_class_level)
    {
      send_to_char(ch, "You have reached the maximum amount of levels for that class.\r\n");
      return;
    }

    /* need to spend their points before advancing */
    if ((GET_PRACTICES(ch) != 0) ||
        (GET_TRAINS(ch) > 1) ||
        (GET_BOOSTS(ch) != 0) ||
        (GET_CLASS_FEATS(ch, class) != 0) ||
        (GET_EPIC_CLASS_FEATS(ch, class) != 0) ||
        (GET_FEAT_POINTS(ch) != 0) ||
        (GET_EPIC_FEAT_POINTS(ch) != 0))
    { //    ||
      /*         ((CLASS_LEVEL(ch, CLASS_SORCERER) && !IS_SORC_LEARNED(ch)) ||
               (CLASS_LEVEL(ch, CLASS_WIZARD)   && !IS_WIZ_LEARNED(ch))  ||
               (CLASS_LEVEL(ch, CLASS_BARD)     && !IS_BARD_LEARNED(ch)) ||
               (CLASS_LEVEL(ch, CLASS_DRUID)    && !IS_DRUID_LEARNED(ch))||
               (CLASS_LEVEL(ch, CLASS_RANGER)   && !IS_RANG_LEARNED(ch)))) {
       */
      /* The last level has not been completely gained yet - The player must
       * use all trains, pracs, boosts and choose spells and other benefits
       * vis 'study' before they can gain a level. */
      //      if (GET_PRACTICES(ch) != 0)
      //        send_to_char(ch, "You must use all practices before gaining another level.  You have %d practice%s remaining.\r\n", GET_PRACTICES(ch), (GET_PRACTICES(ch) > 1 ? "s" : ""));
      if (GET_TRAINS(ch) > 1)
        send_to_char(ch, "You must use all but one of your skill points before gaining "
                         "another level.  You have %d skill point%s remaining.\r\n",
                     GET_TRAINS(ch), (GET_TRAINS(ch) > 1 ? "s" : ""));
      if (GET_BOOSTS(ch) != 0)
        send_to_char(ch, "You must use all ability score boosts before gaining another level.  "
                         "You have %d boost%s remaining.\r\n",
                     GET_BOOSTS(ch), (GET_BOOSTS(ch) > 1 ? "s" : ""));
      if (GET_FEAT_POINTS(ch) != 0)
        send_to_char(ch, "You must use all feat points before gaining another level.  "
                         "You have %d feat point%s remaining.\r\n",
                     GET_FEAT_POINTS(ch), (GET_FEAT_POINTS(ch) > 1 ? "s" : ""));
      if (GET_CLASS_FEATS(ch, class) != 0)
        send_to_char(ch, "You must use all class feat points before gaining another level.  "
                         "You have %d class feat%s remaining.\r\n",
                     GET_CLASS_FEATS(ch, class), (GET_CLASS_FEATS(ch, class) > 1 ? "s" : ""));
      if (GET_EPIC_CLASS_FEATS(ch, class) != 0)
        send_to_char(ch, "You must use all epic class feat points before gaining another level.  "
                         "You have %d epic class feat%s remaining.\r\n",
                     GET_EPIC_CLASS_FEATS(ch, class), (GET_EPIC_CLASS_FEATS(ch, class) > 1 ? "s" : ""));
      if (GET_EPIC_FEAT_POINTS(ch) != 0)
        send_to_char(ch, "You must use all epic feat points before gaining another level.  "
                         "You have %d epic feat point%s remaining.\r\n",
                     GET_EPIC_FEAT_POINTS(ch), (GET_EPIC_FEAT_POINTS(ch) > 1 ? "s" : ""));
      return;
    }
    else if (GET_LEVEL(ch) < LVL_IMMORT - CONFIG_NO_MORT_TO_IMMORT &&
             GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
    {
      GET_LEVEL(ch) += 1;
      CLASS_LEVEL(ch, class)
      ++;
      GET_CLASS(ch) = class;
      num_levels++;
      /* our function for leveling up, takes in class that is being advanced */
      advance_level(ch, class);
      is_altered = TRUE;
    }
    else
    {
      send_to_char(ch, "You are unable to gain a level.\r\n");
      return;
    }

    if (is_altered)
    {
      mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
             "%s advanced %d level%s to level %d.", GET_NAME(ch),
             num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
      if (num_levels == 1)
        send_to_char(ch, "You rise a level!\r\n");
      else
        send_to_char(ch, "You rise %d levels!\r\n", num_levels);
      set_title(ch, NULL);
      if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
        run_autowiz();

      if (GET_PREMADE_BUILD_CLASS(ch) < 0)
        send_to_char(ch, "\tMDon't forget to \tmSTUDY\tM to improve"
                         " your abilities, feats and stats!\tn\r\n");
      else
        advance_premade_build(ch);
    }
  }
}

/*************************/
/* shapechange functions */
/*************************/
struct wild_shape_mods
{
  byte strength;
  byte constitution;
  byte dexterity;
  byte natural_armor;
};

void set_bonus_attributes(struct char_data *ch, int str, int con, int dex, int ac)
{
  GET_DISGUISE_STR(ch) = str;
  GET_DISGUISE_CON(ch) = con;
  GET_DISGUISE_DEX(ch) = dex;
  GET_DISGUISE_AC(ch) = ac;
}

void init_wild_shape_mods(struct wild_shape_mods *abil_mods)
{
  abil_mods->strength = 0;
  abil_mods->constitution = 0;
  abil_mods->dexterity = 0;
  abil_mods->natural_armor = 0;
}

/* stat modifications for wildshape! */
struct wild_shape_mods *set_wild_shape_mods(int race)
{
  struct wild_shape_mods *abil_mods;

  CREATE(abil_mods, struct wild_shape_mods, 1);
  init_wild_shape_mods(abil_mods);

  /* racial-SIZE and default */
  switch (race_list[race].family)
  {
  case RACE_TYPE_ANIMAL:
    switch (race_list[race].size)
    {
    case SIZE_DIMINUTIVE:
      abil_mods->dexterity = 6;
      abil_mods->strength = -4;
      abil_mods->natural_armor = 1;
      break;
    case SIZE_TINY:
      abil_mods->dexterity = 4;
      abil_mods->strength = -2;
      abil_mods->natural_armor = 1;
      break;
    case SIZE_SMALL:
      abil_mods->dexterity = 2;
      abil_mods->natural_armor = 1;
      break;
    case SIZE_MEDIUM:
      abil_mods->strength = 2;
      abil_mods->natural_armor = 2;
      break;
    case SIZE_LARGE:
      abil_mods->dexterity = -2;
      abil_mods->strength = 4;
      abil_mods->natural_armor = 4;
      break;
    case SIZE_HUGE:
      abil_mods->dexterity = -4;
      abil_mods->strength = 6;
      abil_mods->natural_armor = 6;
      break;
    }
    break;
  case RACE_TYPE_MAGICAL_BEAST:
    switch (race_list[race].size)
    {
    case SIZE_TINY:
      abil_mods->dexterity = 8;
      abil_mods->strength = -2;
      abil_mods->natural_armor = 3;
      break;
    case SIZE_SMALL:
      abil_mods->dexterity = 4;
      abil_mods->natural_armor = 2;
      break;
    case SIZE_MEDIUM:
      abil_mods->strength = 4;
      abil_mods->natural_armor = 4;
      break;
    case SIZE_LARGE:
      abil_mods->dexterity = -2;
      abil_mods->strength = 6;
      abil_mods->constitution = 2;
      abil_mods->natural_armor = 6;
      break;
    }
    break;
  case RACE_TYPE_FEY:
    switch (race_list[race].size)
    {
    case SIZE_DIMINUTIVE:
      abil_mods->dexterity = 12;
      abil_mods->strength = -4;
      abil_mods->natural_armor = 4;
      break;
    case SIZE_TINY:
      abil_mods->dexterity = 8;
      abil_mods->strength = -2;
      abil_mods->natural_armor = 3;
      break;
    case SIZE_SMALL:
      abil_mods->dexterity = 4;
      abil_mods->natural_armor = 2;
      break;
    }
    break;
  case RACE_TYPE_CONSTRUCT:
    switch (race_list[race].size)
    {
    case SIZE_MEDIUM:
      abil_mods->strength = 4;
      abil_mods->natural_armor = 4;
      break;
    case SIZE_LARGE:
      abil_mods->dexterity = -2;
      abil_mods->strength = 6;
      abil_mods->constitution = 2;
      abil_mods->natural_armor = 6;
      break;
    case SIZE_HUGE:
      abil_mods->dexterity = -4;
      abil_mods->strength = 10;
      abil_mods->constitution = 4;
      abil_mods->natural_armor = 7;
      break;
    case SIZE_GARGANTUAN:
      abil_mods->dexterity = -8;
      abil_mods->strength = 16;
      abil_mods->constitution = 8;
      abil_mods->natural_armor = 8;
      break;
    }
    break;
  case RACE_TYPE_OUTSIDER:
    switch (race_list[race].size)
    {
    case SIZE_DIMINUTIVE:
      abil_mods->dexterity = 10;
      abil_mods->strength = -4;
      abil_mods->natural_armor = 4;
      break;
    case SIZE_TINY:
      abil_mods->dexterity = 8;
      abil_mods->strength = -2;
      abil_mods->natural_armor = 3;
      break;
    case SIZE_SMALL:
      abil_mods->dexterity = 4;
      abil_mods->natural_armor = 2;
      break;
    case SIZE_MEDIUM:
      abil_mods->strength = 4;
      abil_mods->natural_armor = 4;
      break;
    case SIZE_LARGE:
      abil_mods->dexterity = -2;
      abil_mods->strength = 6;
      abil_mods->constitution = 2;
      abil_mods->natural_armor = 6;
      break;
    case SIZE_HUGE:
      abil_mods->dexterity = -4;
      abil_mods->strength = 8;
      abil_mods->constitution = 4;
      abil_mods->natural_armor = 6;
      break;
    case SIZE_GARGANTUAN:
      abil_mods->dexterity = -8;
      abil_mods->strength = 10;
      abil_mods->constitution = 6;
      abil_mods->natural_armor = 7;
      break;
    }
    break;
  case RACE_TYPE_DRAGON:
    switch (race_list[race].size)
    {
    case SIZE_LARGE:
      abil_mods->dexterity = 0;
      abil_mods->strength = 6;
      abil_mods->constitution = 6;
      abil_mods->natural_armor = 18;
      break;
    case SIZE_HUGE:
      abil_mods->dexterity = 0;
      abil_mods->strength = 8;
      abil_mods->constitution = 8;
      abil_mods->natural_armor = 20;
      break;
    case SIZE_GARGANTUAN:
      abil_mods->dexterity = 0;
      abil_mods->strength = 10;
      abil_mods->constitution = 10;
      abil_mods->natural_armor = 22;
      break;
    }
    break;
  case RACE_TYPE_PLANT:
    switch (race_list[race].size)
    {
    case SIZE_SMALL:
      abil_mods->constitution = 2;
      abil_mods->natural_armor = 2;
      break;
    case SIZE_MEDIUM:
      abil_mods->strength = 2;
      abil_mods->constitution = 2;
      abil_mods->natural_armor = 2;
      break;
    case SIZE_LARGE:
      abil_mods->strength = 4;
      abil_mods->constitution = 2;
      abil_mods->natural_armor = 4;
      break;
    case SIZE_HUGE:
      abil_mods->strength = 8;
      abil_mods->dexterity = -2;
      abil_mods->constitution = 4;
      abil_mods->natural_armor = 6;
      break;
    }
    break;
  case RACE_TYPE_ELEMENTAL:
    switch (race)
    {
    case RACE_SMALL_FIRE_ELEMENTAL:
      abil_mods->strength = 0;
      abil_mods->dexterity = 2;
      abil_mods->constitution = 0;
      abil_mods->natural_armor = 2;
      break;
    case RACE_MEDIUM_FIRE_ELEMENTAL:
      abil_mods->strength = 0;
      abil_mods->dexterity = 4;
      abil_mods->constitution = 0;
      abil_mods->natural_armor = 3;
      break;
    case RACE_LARGE_FIRE_ELEMENTAL:
      abil_mods->strength = 0;
      abil_mods->dexterity = 4;
      abil_mods->constitution = 2;
      abil_mods->natural_armor = 4;
      break;
    case RACE_HUGE_FIRE_ELEMENTAL:
      abil_mods->strength = 0;
      abil_mods->dexterity = 6;
      abil_mods->constitution = 4;
      abil_mods->natural_armor = 4;
      break;
    case RACE_SMALL_AIR_ELEMENTAL:
      abil_mods->strength = 0;
      abil_mods->dexterity = 2;
      abil_mods->constitution = 0;
      abil_mods->natural_armor = 2;
      break;
    case RACE_MEDIUM_AIR_ELEMENTAL:
      abil_mods->strength = 0;
      abil_mods->dexterity = 4;
      abil_mods->constitution = 0;
      abil_mods->natural_armor = 3;
      break;
    case RACE_LARGE_AIR_ELEMENTAL:
      abil_mods->strength = 2;
      abil_mods->dexterity = 4;
      abil_mods->constitution = 0;
      abil_mods->natural_armor = 4;
      break;
    case RACE_HUGE_AIR_ELEMENTAL:
      abil_mods->strength = 4;
      abil_mods->dexterity = 6;
      abil_mods->constitution = 0;
      abil_mods->natural_armor = 4;
      break;
    case RACE_SMALL_EARTH_ELEMENTAL:
      abil_mods->strength = 0;
      abil_mods->dexterity = 2;
      abil_mods->constitution = 0;
      abil_mods->natural_armor = 4;
      break;
    case RACE_MEDIUM_EARTH_ELEMENTAL:
      abil_mods->strength = 0;
      abil_mods->dexterity = 4;
      abil_mods->constitution = 0;
      abil_mods->natural_armor = 5;
      break;
    case RACE_LARGE_EARTH_ELEMENTAL:
      abil_mods->strength = 6;
      abil_mods->dexterity = -2;
      abil_mods->constitution = 2;
      abil_mods->natural_armor = 6;
      break;
    case RACE_HUGE_EARTH_ELEMENTAL:
      abil_mods->strength = 8;
      abil_mods->dexterity = -2;
      abil_mods->constitution = 4;
      abil_mods->natural_armor = 6;
      break;
    case RACE_SMALL_WATER_ELEMENTAL:
      abil_mods->strength = 0;
      abil_mods->dexterity = 0;
      abil_mods->constitution = 2;
      abil_mods->natural_armor = 4;
      break;
    case RACE_MEDIUM_WATER_ELEMENTAL:
      abil_mods->strength = 0;
      abil_mods->dexterity = 0;
      abil_mods->constitution = 4;
      abil_mods->natural_armor = 5;
      break;
    case RACE_LARGE_WATER_ELEMENTAL:
      abil_mods->strength = 2;
      abil_mods->dexterity = -2;
      abil_mods->constitution = 6;
      abil_mods->natural_armor = 6;
      break;
    case RACE_HUGE_WATER_ELEMENTAL:
      abil_mods->strength = 4;
      abil_mods->dexterity = -2;
      abil_mods->constitution = 8;
      abil_mods->natural_armor = 6;
      break;
    }
    break;
  default:
    abil_mods->strength = race_list[race].ability_mods[0];
    abil_mods->constitution = race_list[race].ability_mods[1];
    abil_mods->dexterity = race_list[race].ability_mods[4];
    break;
  }

  /* individual race modifications */
  switch (race)
  {
  case RACE_CHEETAH:
    // abil_mods->dexterity += 8; // This should not be here. Gicker June 5, 2020
    break;
  case RACE_WOLF:
  case RACE_HYENA:
    break;
  case RACE_IRON_GOLEM:
    abil_mods->strength = 8;
    abil_mods->dexterity = -2;
    abil_mods->constitution = 8;
    abil_mods->natural_armor = 20;
    break;
  case RACE_EFREETI:
    abil_mods->strength = 8;
    abil_mods->dexterity = 8;
    abil_mods->constitution = 8;
    abil_mods->natural_armor = 20;
    break;
  case RACE_MANTICORE:
    abil_mods->strength = 6;
    abil_mods->dexterity = 2;
    abil_mods->constitution = 4;
    abil_mods->natural_armor = 20;
    break;
  case RACE_PIXIE:
    abil_mods->strength = -2;
    abil_mods->dexterity = 8;
    abil_mods->constitution = 2;
    abil_mods->natural_armor = 20;
    break;
  default:
    break;
  }

  return abil_mods;
}

/* At 6th level, a druid can use wild shape to change into a Large or Tiny animal
 * or a Small elemental. When taking the form of an animal, a druid's wild shape
 * now functions as beast shape II. When taking the form of an elemental, the
 * druid's wild shape functions as elemental body I.

At 8th level, a druid can use wild shape to change into a Huge or Diminutive
 * animal, a Medium elemental, or a Small or Medium plant creature. When taking
 * the form of animals, a druid's wild shape now functions as beast shape III.
 * When taking the form of an elemental, the druid's wild shape now functions
 * as elemental body II. When taking the form of a plant creature, the druid's
 * wild shape functions as plant shape I.

At 10th level, a druid can use wild shape to change into a Large elemental or a
 * Large plant creature. When taking the form of an elemental, the druid's wild
 * shape now functions as elemental body III. When taking the form of a plant,
 * the druid's wild shape now functions as plant shape II.

At 12th level, a druid can use wild shape to change into a Huge elemental or a
 * Huge plant creature. When taking the form of an elemental, the druid's wild
 * shape now functions as elemental body IV. When taking the form of a plant, the
 * druid's wild shape now functions as plant shape III.
 */
int display_eligible_wildshape_races(struct char_data *ch, const char *argument, int silent, int mode)
{
  int i = 0;
  struct wild_shape_mods *abil_mods;

  CREATE(abil_mods, struct wild_shape_mods, 1);
  init_wild_shape_mods(abil_mods);

  /* human = 0, we gotta fix it, but we think a ZERO value is no disguise */
  for (i = 1; i < NUM_EXTENDED_RACES; i++)
  {

    if (mode == 1)
    { /*polymorph spell*/
      // everything but shifter shapes
      switch (race_list[i].family)
      {
      case RACE_TYPE_MAGICAL_BEAST:
      case RACE_TYPE_FEY:
      case RACE_TYPE_CONSTRUCT:
      case RACE_TYPE_OUTSIDER:
      case RACE_TYPE_DRAGON:
        continue;
      }
    }
    else if (mode == 2)
    {
      // Vampire Form - Wolf or Bat
      if (i != RACE_WOLF && i != RACE_BAT)
        continue;
    }
    else if (mode == 0)
    { /*druid*/
      switch (race_list[i].family)
      {

      case RACE_TYPE_ANIMAL: /* animals! */
        switch (race_list[i].size)
        {
          /* fall through all the way down */
        case SIZE_SMALL:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE))
            break;
        case SIZE_MEDIUM:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE))
            break;
        case SIZE_TINY:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_2))
            break;
        case SIZE_LARGE:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_2))
            break;
        case SIZE_DIMINUTIVE:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_3))
            break;
        case SIZE_HUGE:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_3))
            break;

        case SIZE_FINE:
        case SIZE_GARGANTUAN:
        case SIZE_COLOSSAL:
        default:
          continue;
        }
        break;

      case RACE_TYPE_PLANT: /* plants! */
        switch (race_list[i].size)
        {
          /* fall through all the way down */
        case SIZE_SMALL:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_3))
            break;
        case SIZE_MEDIUM:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_3))
            break;
        case SIZE_LARGE:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_4))
            break;
        case SIZE_HUGE:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_5))
            break;

        case SIZE_DIMINUTIVE:
        case SIZE_TINY:
        case SIZE_FINE:
        case SIZE_GARGANTUAN:
        case SIZE_COLOSSAL:
        default:
          continue;
        }
        break;

      case RACE_TYPE_ELEMENTAL: /* elementals! */
        switch (race_list[i].size)
        {
          /* fall through all the way down */
        case SIZE_SMALL:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_2))
            break;
        case SIZE_MEDIUM:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_3))
            break;
        case SIZE_LARGE:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_4))
            break;
        case SIZE_HUGE:
          if (HAS_FEAT(ch, FEAT_WILD_SHAPE_5))
            break;

        case SIZE_DIMINUTIVE:
        case SIZE_TINY:
        case SIZE_FINE:
        case SIZE_GARGANTUAN:
        case SIZE_COLOSSAL:
        default:
          continue;
        }
        break;

      case RACE_TYPE_MAGICAL_BEAST:
        if (HAS_FEAT(ch, FEAT_SHIFTER_SHAPES_1))
          break;
        continue;

      case RACE_TYPE_FEY:
        if (HAS_FEAT(ch, FEAT_SHIFTER_SHAPES_2))
          break;
        continue;

      case RACE_TYPE_CONSTRUCT:
        if (HAS_FEAT(ch, FEAT_SHIFTER_SHAPES_3))
          break;
        continue;

      case RACE_TYPE_OUTSIDER:
        if (HAS_FEAT(ch, FEAT_SHIFTER_SHAPES_4))
          break;
        continue;

      case RACE_TYPE_DRAGON:
        if (HAS_FEAT(ch, FEAT_SHIFTER_SHAPES_5))
          break;
        continue;

      default:
        continue;
      } /* end family switch */
    }
    abil_mods = set_wild_shape_mods(i);
    if (HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE) && mode == 0)
    {
      abil_mods->strength += HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE);
      abil_mods->dexterity += HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE);
      abil_mods->constitution += HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE);
      abil_mods->natural_armor += HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE);
    }
    if (HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_SPELL_FOCUS), TRANSMUTATION) && mode == 1)
    { // polymorph
      abil_mods->strength += 2;
      abil_mods->dexterity += 2;
      abil_mods->constitution += 2;
      abil_mods->natural_armor += 1;
    }
    if (HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_GREATER_SPELL_FOCUS), TRANSMUTATION) && mode == 1)
    { // polymorph
      abil_mods->strength += 2;
      abil_mods->dexterity += 2;
      abil_mods->constitution += 2;
      abil_mods->natural_armor += 1;
    }
    if (HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_EPIC_SPELL_FOCUS), TRANSMUTATION) && mode == 1)
    { // polymorph
      abil_mods->strength += 2;
      abil_mods->dexterity += 2;
      abil_mods->constitution += 2;
      abil_mods->natural_armor += 1;
    }
    if (!silent && race_list[i].name != NULL)
    {
      send_to_char(ch, "%-40s Str [%s%-2d] Con [%s%-2d] Dex [%s%-2d] NatAC [%s%-2d]\r\n", race_list[i].name,
                   abil_mods->strength >= 0 ? "+" : " ", abil_mods->strength,
                   abil_mods->constitution >= 0 ? "+" : " ", abil_mods->constitution,
                   abil_mods->dexterity >= 0 ? "+" : " ", abil_mods->dexterity,
                   abil_mods->natural_armor >= 0 ? "+" : " ", abil_mods->natural_armor);
    }

    if (race_list[i].name != NULL && is_abbrev(argument, race_list[i].name)) /* match argument? */
      break;
  } /* end race list loop */

  // free(abil_mods);

  if (i >= NUM_EXTENDED_RACES || i < 0)
    return -1; /* failed to find anything */
  else
    return i; /* specific race */
}

void set_bonus_stats(struct char_data *ch, int str, int con, int dex, int ac)
{
  struct affected_type af[WILDSHAPE_AFFECTS];
  int i = 0;

  /* init affect array */
  for (i = 0; i < WILDSHAPE_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SKILL_WILDSHAPE;
    af[i].duration = 32766; /*what cheese*/
  }

  af[0].location = APPLY_STR;
  af[0].modifier = str;

  af[1].location = APPLY_DEX;
  af[1].modifier = dex;

  af[2].location = APPLY_CON;
  af[2].modifier = con;

  af[3].location = APPLY_AC_NEW;
  af[3].modifier = ac;

  for (i = 0; i < WILDSHAPE_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  return;
}

/* also clean up anything else assigned such as affections */
void cleanup_wildshape_feats(struct char_data *ch)
{
  int counter = 0;
  int race = GET_DISGUISE_RACE(ch);

  for (counter = 0; counter < NUM_FEATS; counter++)
    MOB_SET_FEAT((ch), counter, 0);

  /* racial family cleanup here */
  switch (race_list[race].family)
  {
  default:
    break;
  }

  /* race specific cleanup here */
  switch (race)
  {
  case RACE_BLINK_DOG:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_BLINKING);
    break;
  case RACE_CROCODILE:
  case RACE_GIANT_CROCODILE:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SCUBA);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_WATER_BREATH);
    break;
  case RACE_EAGLE:
  case RACE_PIXIE:
  case RACE_BAT:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    break;
  case RACE_MANTICORE:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    break;
  case RACE_RED_DRAGON:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FSHIELD);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    break;
  case RACE_EFREETI:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FSHIELD);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    break;
  case RACE_BLUE_DRAGON:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ESHIELD);
    break;
  case RACE_GREEN_DRAGON:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_WATER_BREATH);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ASHIELD);
    break;
  case RACE_BLACK_DRAGON:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_WATER_BREATH);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ASHIELD);
    break;
  case RACE_WHITE_DRAGON:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_CSHIELD);
    break;
  case RACE_SMALL_FIRE_ELEMENTAL:
  case RACE_MEDIUM_FIRE_ELEMENTAL:
  case RACE_LARGE_FIRE_ELEMENTAL:
  case RACE_HUGE_FIRE_ELEMENTAL:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FSHIELD);
    break;
  case RACE_SMALL_EARTH_ELEMENTAL:
  case RACE_MEDIUM_EARTH_ELEMENTAL:
  case RACE_LARGE_EARTH_ELEMENTAL:
  case RACE_HUGE_EARTH_ELEMENTAL:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ASHIELD);
    break;
  case RACE_SMALL_AIR_ELEMENTAL:
  case RACE_MEDIUM_AIR_ELEMENTAL:
  case RACE_LARGE_AIR_ELEMENTAL:
  case RACE_HUGE_AIR_ELEMENTAL:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_CSHIELD);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    break;
  case RACE_SMALL_WATER_ELEMENTAL:
  case RACE_MEDIUM_WATER_ELEMENTAL:
  case RACE_LARGE_WATER_ELEMENTAL:
  case RACE_HUGE_WATER_ELEMENTAL:
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SCUBA);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_WATER_BREATH);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_MINOR_GLOBE);
    break;
  }
}

/* we also set other special abilities here */
void assign_wildshape_feats(struct char_data *ch)
{
  int counter = 0;
  int shifter_level = CLASS_LEVEL(ch, CLASS_DRUID) + CLASS_LEVEL(ch, CLASS_SHIFTER);
  int shifted_race = GET_DISGUISE_RACE(ch);

  if (shifter_level > 30)
    shifter_level = 30;
  if (shifter_level < 1)
    shifter_level = 1;

  /* just to be on the safe side, doing a cleanup before assignment*/
  for (counter = 0; counter < NUM_FEATS; counter++)
    MOB_SET_FEAT((ch), counter, 0);

  /***************************************************/
  /* make sure -=these=- feats transfer over! -zusuk */
  if (HAS_REAL_FEAT(ch, FEAT_NATURAL_SPELL))
    MOB_SET_FEAT(ch, FEAT_NATURAL_SPELL, 1);
  /** end transferable feats ***/
  /***************************************************/

  /* trying to keep general racial type assignments here */
  switch (race_list[GET_DISGUISE_RACE(ch)].family)
  {
  case RACE_TYPE_ANIMAL:
    MOB_SET_FEAT(ch, FEAT_RAGE, shifter_level / 5 + 1);
    break;
  case RACE_TYPE_PLANT:
    MOB_SET_FEAT(ch, FEAT_ARMOR_SKIN, shifter_level / 5 + 1);
    break;
  default:
    break;
  }

  /* want to make general assignments due to size? */
  switch (GET_SIZE(ch))
  {
  case SIZE_FINE:
  case SIZE_DIMINUTIVE:
  case SIZE_TINY:
  case SIZE_SMALL:
  case SIZE_MEDIUM:
  case SIZE_LARGE:
  case SIZE_HUGE:
  case SIZE_GARGANTUAN:
  case SIZE_COLOSSAL:
  default:
    break;
  }

  /* all fall through */
  switch (shifter_level)
  {
  case 40:
  case 39:
  case 38:
  case 37:
  case 36:
  case 35:
  case 34:
  case 33:
  case 32:
  case 31:
  case 30:
  case 29:
    MOB_SET_FEAT(ch, FEAT_NATURAL_ATTACK, MOB_HAS_FEAT(ch, FEAT_NATURAL_ATTACK) + 1);
  case 28:
  case 27:
  case 26:
  case 25:
    MOB_SET_FEAT(ch, FEAT_NATURAL_ATTACK, MOB_HAS_FEAT(ch, FEAT_NATURAL_ATTACK) + 1);
  case 24:
  case 23:
  case 22:
  case 21:
    MOB_SET_FEAT(ch, FEAT_NATURAL_ATTACK, MOB_HAS_FEAT(ch, FEAT_NATURAL_ATTACK) + 1);
  case 20:
  case 19:
  case 18:
  case 17:
    MOB_SET_FEAT(ch, FEAT_NATURAL_ATTACK, MOB_HAS_FEAT(ch, FEAT_NATURAL_ATTACK) + 1);
  case 16:
  case 15:
  case 14:
  case 13:
    MOB_SET_FEAT(ch, FEAT_NATURAL_ATTACK, MOB_HAS_FEAT(ch, FEAT_NATURAL_ATTACK) + 1);
  case 12:
  case 11:
  case 10:
  case 9:
    MOB_SET_FEAT(ch, FEAT_NATURAL_ATTACK, MOB_HAS_FEAT(ch, FEAT_NATURAL_ATTACK) + 1);
  case 8:
  case 7:
  case 6:
  case 5:
    MOB_SET_FEAT(ch, FEAT_NATURAL_ATTACK, MOB_HAS_FEAT(ch, FEAT_NATURAL_ATTACK) + 1);
  case 4:
  case 3:
  case 2:
  case 1:
    MOB_SET_FEAT(ch, FEAT_NATURAL_ATTACK, MOB_HAS_FEAT(ch, FEAT_NATURAL_ATTACK) + 1);
  default:
    break;
  }

  /* individual race specific assignments, feats, affections, etc */
  switch (shifted_race)
  {
  case RACE_WHITE_DRAGON:
    MOB_SET_FEAT(ch, FEAT_TRUE_SIGHT, 1);
    MOB_SET_FEAT(ch, FEAT_PARALYSIS_IMMUNITY, 1);
    MOB_SET_FEAT(ch, FEAT_ULTRAVISION, 1);
    MOB_SET_FEAT(ch, FEAT_BLINDSENSE, 1);
    MOB_SET_FEAT(ch, FEAT_WINGS, 1);
    MOB_SET_FEAT(ch, FEAT_ALERTNESS, 1);
    MOB_SET_FEAT(ch, FEAT_IMPROVED_INITIATIVE, 1);
    MOB_SET_FEAT(ch, FEAT_POWER_ATTACK, 1);
    MOB_SET_FEAT(ch, FEAT_DRAGON_MAGIC, 1);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_CSHIELD);
    break;
  case RACE_BLACK_DRAGON:
    MOB_SET_FEAT(ch, FEAT_TRUE_SIGHT, 1);
    MOB_SET_FEAT(ch, FEAT_PARALYSIS_IMMUNITY, 1);
    MOB_SET_FEAT(ch, FEAT_ULTRAVISION, 1);
    MOB_SET_FEAT(ch, FEAT_BLINDSENSE, 1);
    MOB_SET_FEAT(ch, FEAT_WINGS, 1);
    MOB_SET_FEAT(ch, FEAT_ALERTNESS, 1);
    MOB_SET_FEAT(ch, FEAT_IMPROVED_INITIATIVE, 1);
    MOB_SET_FEAT(ch, FEAT_POWER_ATTACK, 1);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_WATER_BREATH);
    MOB_SET_FEAT(ch, FEAT_DRAGON_MAGIC, 1);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_ASHIELD);
    break;
  case RACE_GREEN_DRAGON:
    MOB_SET_FEAT(ch, FEAT_TRUE_SIGHT, 1);
    MOB_SET_FEAT(ch, FEAT_PARALYSIS_IMMUNITY, 1);
    MOB_SET_FEAT(ch, FEAT_ULTRAVISION, 1);
    MOB_SET_FEAT(ch, FEAT_BLINDSENSE, 1);
    MOB_SET_FEAT(ch, FEAT_WINGS, 1);
    MOB_SET_FEAT(ch, FEAT_ALERTNESS, 1);
    MOB_SET_FEAT(ch, FEAT_CLEAVE, 1);
    MOB_SET_FEAT(ch, FEAT_GREAT_CLEAVE, 1);
    MOB_SET_FEAT(ch, FEAT_IRON_WILL, 1);
    MOB_SET_FEAT(ch, FEAT_POWER_ATTACK, 1);
    MOB_SET_FEAT(ch, FEAT_TRACKLESS_STEP, 1);
    MOB_SET_FEAT(ch, FEAT_WOODLAND_STRIDE, 1);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_WATER_BREATH);
    MOB_SET_FEAT(ch, FEAT_DRAGON_MAGIC, 1);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_ASHIELD);
    break;
  case RACE_BLUE_DRAGON:
    MOB_SET_FEAT(ch, FEAT_TRUE_SIGHT, 1);
    MOB_SET_FEAT(ch, FEAT_PARALYSIS_IMMUNITY, 1);
    MOB_SET_FEAT(ch, FEAT_ULTRAVISION, 1);
    MOB_SET_FEAT(ch, FEAT_BLINDSENSE, 1);
    MOB_SET_FEAT(ch, FEAT_WINGS, 1);
    MOB_SET_FEAT(ch, FEAT_COMBAT_CASTING, 1);
    MOB_SET_FEAT(ch, FEAT_IMPROVED_INITIATIVE, 1);
    MOB_SET_FEAT(ch, FEAT_DRAGON_MAGIC, 1);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_ESHIELD);
    break;
  case RACE_RED_DRAGON:
    MOB_SET_FEAT(ch, FEAT_TRUE_SIGHT, 1);
    MOB_SET_FEAT(ch, FEAT_PARALYSIS_IMMUNITY, 1);
    MOB_SET_FEAT(ch, FEAT_ULTRAVISION, 1);
    MOB_SET_FEAT(ch, FEAT_BLINDSENSE, 1);
    MOB_SET_FEAT(ch, FEAT_WINGS, 1);
    MOB_SET_FEAT(ch, FEAT_IRON_WILL, 1);
    MOB_SET_FEAT(ch, FEAT_POWER_ATTACK, 1);
    MOB_SET_FEAT(ch, FEAT_CLEAVE, 1);
    MOB_SET_FEAT(ch, FEAT_IMPROVED_INITIATIVE, 1);
    MOB_SET_FEAT(ch, FEAT_DRAGON_MAGIC, 1);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_FSHIELD);
    break;
  case RACE_IRON_GOLEM:
    MOB_SET_FEAT(ch, FEAT_IRON_GOLEM_IMMUNITY, 1);
    MOB_SET_FEAT(ch, FEAT_POISON_BREATH, 1);
    MOB_SET_FEAT(ch, FEAT_ULTRAVISION, 1);
    break;
  case RACE_BLINK_DOG:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_BLINKING);
    break;
  case RACE_CHEETAH:
    MOB_SET_FEAT(ch, FEAT_DODGE, 1);
    break;
  case RACE_WOLF:
  case RACE_HYENA:
    MOB_SET_FEAT(ch, FEAT_NATURAL_TRACKER, 1);
    MOB_SET_FEAT(ch, FEAT_INFRAVISION, 1);
    break;
  case RACE_CROCODILE:
  case RACE_GIANT_CROCODILE:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_SCUBA);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_WATER_BREATH);
    break;
  case RACE_MEDIUM_VIPER:
  case RACE_LARGE_VIPER:
  case RACE_HUGE_VIPER:
    MOB_SET_FEAT(ch, FEAT_POISON_BITE, 1);
    break;
  case RACE_CONSTRICTOR_SNAKE:
  case RACE_GIANT_CONSTRICTOR_SNAKE:
    MOB_SET_FEAT(ch, FEAT_IMPROVED_GRAPPLE, 1);
    break;
  case RACE_MANTICORE:
    MOB_SET_FEAT(ch, FEAT_WINGS, 1);
    MOB_SET_FEAT(ch, FEAT_TAIL_SPIKES, 1);
    MOB_SET_FEAT(ch, FEAT_ULTRAVISION, 1);
    break;
  case RACE_PIXIE:
    MOB_SET_FEAT(ch, FEAT_WINGS, 1);
    MOB_SET_FEAT(ch, FEAT_PIXIE_DUST, 1);
    MOB_SET_FEAT(ch, FEAT_PIXIE_INVISIBILITY, 1);
    MOB_SET_FEAT(ch, FEAT_DODGE, 1);
    MOB_SET_FEAT(ch, FEAT_WEAPON_FINESSE, 1);
    MOB_SET_FEAT(ch, FEAT_INFRAVISION, 1);
    break;
  case RACE_EAGLE:
  case RACE_BAT:
    MOB_SET_FEAT(ch, FEAT_WINGS, 1);
    break;
  case RACE_EFREETI:
    MOB_SET_FEAT(ch, FEAT_EFREETI_MAGIC, 1);
    MOB_SET_FEAT(ch, FEAT_ULTRAVISION, 1);
    MOB_SET_FEAT(ch, FEAT_COMBAT_CASTING, 1);
    MOB_SET_FEAT(ch, FEAT_COMBAT_REFLEXES, 1);
    MOB_SET_FEAT(ch, FEAT_DODGE, 1);
    MOB_SET_FEAT(ch, FEAT_IMPROVED_INITIATIVE, 1);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_FSHIELD);
    MOB_SET_FEAT(ch, FEAT_WINGS, 1);
    break;
  case RACE_SMALL_FIRE_ELEMENTAL:
  case RACE_MEDIUM_FIRE_ELEMENTAL:
  case RACE_LARGE_FIRE_ELEMENTAL:
  case RACE_HUGE_FIRE_ELEMENTAL:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_FSHIELD);
    break;
  case RACE_SMALL_EARTH_ELEMENTAL:
  case RACE_MEDIUM_EARTH_ELEMENTAL:
  case RACE_LARGE_EARTH_ELEMENTAL:
  case RACE_HUGE_EARTH_ELEMENTAL:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_ASHIELD);
    break;
  case RACE_SMALL_AIR_ELEMENTAL:
  case RACE_MEDIUM_AIR_ELEMENTAL:
  case RACE_LARGE_AIR_ELEMENTAL:
  case RACE_HUGE_AIR_ELEMENTAL:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_CSHIELD);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    break;
  case RACE_SMALL_WATER_ELEMENTAL:
  case RACE_MEDIUM_WATER_ELEMENTAL:
  case RACE_LARGE_WATER_ELEMENTAL:
  case RACE_HUGE_WATER_ELEMENTAL:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_SCUBA);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_WATER_BREATH);
    SET_BIT_AR(AFF_FLAGS(ch), AFF_MINOR_GLOBE);
    break;
  }
}

/* function for clearing wildshape */
void wildshape_return(struct char_data *ch)
{

  if (!AFF_FLAGGED(ch, AFF_WILD_SHAPE) && !GET_DISGUISE_RACE(ch))
    return;

  /* cleanup bonuses */
  affect_from_char(ch, SKILL_WILDSHAPE);

  /* clear mobile feats, this needs to come before resetting disguise-race
   * because we need to know what race we are cleaning up */
  cleanup_wildshape_feats(ch);

  /* stat modifications are cleaned up in affect_total() */
  GET_DISGUISE_RACE(ch) = 0;
  IS_MORPHED(ch) = 0;
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_WILD_SHAPE);

  FIRING(ch) = FALSE; /*just in case*/

  /* affect total, and save */
  affect_total(ch);
  save_char(ch, 0);
  Crash_crashsave(ch);

  return;
}

/* moved the engine out of do_wildshape so we can use it in other places */
/* mode = 0, druid */
/* mode = 1, polymorph spell (spells.c) */
bool wildshape_engine(struct char_data *ch, const char *argument, int mode)
{
  int i = 0;
  char buf[200];
  struct wild_shape_mods *abil_mods;

  skip_spaces_c(&argument);

  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE))
  {
    send_to_char(ch, "You must return to your normal shape before assuming a new form.\r\n");
    return FALSE;
  }

  if (GET_DISGUISE_RACE(ch))
  {
    send_to_char(ch, "You must remove your disguise before using wildshape.\r\n");
    return FALSE;
  }

  if (IS_MORPHED(ch))
  {
    send_to_char(ch, "You can't wildshape while shape-changed!\r\n");
    return FALSE;
  }

  /* if we are in druid-mode... */
  if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_LIMITLESS_SHAPES) && mode == 0)
    start_daily_use_cooldown(ch, FEAT_WILD_SHAPE);

  /* try to match argument to the list */
  i = display_eligible_wildshape_races(ch, argument, TRUE, mode);

  if (i == -1)
  { /* failed to find the race! (0 is human) */
    send_to_char(ch, "Please select a race to wildshape/polymorph/vampireform to or select 'return'.\r\n");
    display_eligible_wildshape_races(ch, argument, FALSE, mode);
    return FALSE;
  }

  snprintf(buf, sizeof(buf), "You change shape into a %s.", race_list[i].name);
  act(buf, true, ch, 0, 0, TO_CHAR);
  snprintf(buf, sizeof(buf), "$n changes shape into a %s.", race_list[i].name);
  act(buf, true, ch, 0, 0, TO_ROOM);

  send_to_char(ch, "Type 'wildshape return' to shift back to your normal form.\r\n");

  /* we're in the clear, set the wildshape race! */
  SET_BIT_AR(AFF_FLAGS(ch), AFF_WILD_SHAPE);
  GET_DISGUISE_RACE(ch) = i;
  /* determine modifiers */
  abil_mods = set_wild_shape_mods(GET_DISGUISE_RACE(ch));
  if (HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE) && mode == 0) // wildshape
  {
    abil_mods->strength += HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE);
    abil_mods->dexterity += HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE);
    abil_mods->constitution += HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE);
    abil_mods->natural_armor += HAS_FEAT(ch, FEAT_EPIC_WILDSHAPE);
  }
  if (HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_SPELL_FOCUS), TRANSMUTATION) && mode == 1)
  { // polymorph
    abil_mods->strength += 2;
    abil_mods->dexterity += 2;
    abil_mods->constitution += 2;
    abil_mods->natural_armor += 1;
  }
  if (HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_GREATER_SPELL_FOCUS), TRANSMUTATION) && mode == 1)
  { // polymorph
    abil_mods->strength += 2;
    abil_mods->dexterity += 2;
    abil_mods->constitution += 2;
    abil_mods->natural_armor += 1;
  }
  if (HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_EPIC_SPELL_FOCUS), TRANSMUTATION) && mode == 1)
  { // polymorph
    abil_mods->strength += 2;
    abil_mods->dexterity += 2;
    abil_mods->constitution += 2;
    abil_mods->natural_armor += 1;
  }
  /* set the bonuses */
  set_bonus_stats(ch, abil_mods->strength, abil_mods->constitution,
                  abil_mods->dexterity, abil_mods->natural_armor);
  /* all stat modifications are done */

  /* assign appropriate racial/mobile feats here */
  assign_wildshape_feats(ch);

  FIRING(ch) = FALSE; /*just in case*/

  /* minor healing */
  GET_HIT(ch) += GET_LEVEL(ch);
  GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));

  IS_MORPHED(ch) = race_list[GET_DISGUISE_RACE(ch)].family;
  affect_total(ch);
  save_char(ch, 0);
  Crash_crashsave(ch);

  return TRUE;
}

/* wildshape!  druids cup o' tea */
ACMD(do_wildshape)
{
  char buf[200];
  int uses_remaining = 0;

  skip_spaces_c(&argument);

  if (!*argument && HAS_FEAT(ch, FEAT_WILD_SHAPE))
  {
    send_to_char(ch, "Please select a race to switch to or select 'return'.\r\n");
    display_eligible_wildshape_races(ch, argument, FALSE, 0);
    return;
  }

  if (strlen(argument) > 100)
  {
    send_to_char(ch, "The race name argument cannot be any longer than 100 characters.\r\n");
    return;
  }

  if (!strcmp(argument, "return"))
  {
    if (!AFF_FLAGGED(ch, AFF_WILD_SHAPE) /*&& !GET_DISGUISE_RACE(ch)*/)
    {
      send_to_char(ch, "You are not wild shaped.\r\n");
      return;
    }

    /* do most of the clearing in this function */
    wildshape_return(ch);

    /* messages */
    snprintf(buf, sizeof(buf), "You change shape into a %s.", race_list[GET_REAL_RACE(ch)].type);
    act(buf, true, ch, 0, 0, TO_CHAR);
    snprintf(buf, sizeof(buf), "$n changes shape into a %s.", race_list[GET_REAL_RACE(ch)].type);
    act(buf, true, ch, 0, 0, TO_ROOM);

    /* a little bit of healing */
    GET_HIT(ch) += GET_LEVEL(ch);
    GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));

    USE_STANDARD_ACTION(ch);

    return;
  }
  /* END wildshape-return */

  /* BEGIN wildshape! */

  /* can we even use this? */
  if (!HAS_FEAT(ch, FEAT_WILD_SHAPE) && !HAS_REAL_FEAT(ch, FEAT_WILD_SHAPE))
  {
    send_to_char(ch, "You do not have the ability to shapechange using wild shape.\r\n");
    return;
  }
  uses_remaining = daily_uses_remaining(ch, FEAT_WILD_SHAPE);
  if (HAS_FEAT(ch, FEAT_LIMITLESS_SHAPES))
    ;
  else if (uses_remaining <= 0)
  {
    send_to_char(ch, "You must recover the energy required to take a wild shape.\r\n");
    return;
  }

  /* here is the engine, there are some more exit checks over there */
  if (wildshape_engine(ch, argument, 0))
  {
    USE_STANDARD_ACTION(ch);
  }

  return;
}

/* header file:  act.h */
void list_forms(struct char_data *ch)
{
  send_to_char(ch, "%s\r\n", npc_race_menu);
}

/*    FIRST version of shapechange/wildshape; TODO: phase out completely
 *  shapechange function
 * mode = 1 = druid
 * mode = 2 = polymorph spell
 * header file:  act.h */
void perform_shapechange(struct char_data *ch, char *arg, int mode)
{
  int form = -1;

  if (!*arg)
  {
    if (!IS_MORPHED(ch))
    {
      send_to_char(ch, "You are already in your natural form!\r\n");
    }
    else
    {
      send_to_char(ch, "You shift back into your natural form...\r\n");
      act("$n shifts back to his natural form.", TRUE, ch, 0, 0, TO_ROOM);
      IS_MORPHED(ch) = 0;
    }
    if (CLASS_LEVEL(ch, CLASS_DRUID) >= 6)
      list_forms(ch);
  }
  else
  {
    form = atoi(arg);
    if (form < 1 || form > NUM_RACE_TYPES - 1)
    {
      send_to_char(ch, "That is not a valid race!\r\n");
      list_forms(ch);
      return;
    }
    IS_MORPHED(ch) = form;
    if (mode == 1)
      GET_SHAPECHANGES(ch)
    --;

    /* the morph_to_x are in race.c */
    send_to_char(ch, "You transform into a %s!\r\n", RACE_ABBR(ch));
    act(morph_to_char[IS_MORPHED(ch)], TRUE, ch, 0, 0, TO_CHAR);
    send_to_char(ch, "\tDType 'innates' to see your abilities.  Type 'shapechange' to revert forms.\tn\r\n");
    act("$n shapechanges!", TRUE, ch, 0, 0, TO_ROOM);
    act(morph_to_room[IS_MORPHED(ch)], TRUE, ch, 0, 0, TO_ROOM);
  }
}

/* engine for shapechanging / wildshape
   turned this into a sub-function in case we want
   to use the engine for spells (like 'animal shapes')
 */
void perform_wildshape(struct char_data *ch, int form_num, int spellnum)
{
  struct affected_type af[SHAPE_AFFECTS];
  int i = 0;

  /* some dummy checks */
  if (!ch)
    return;
  if (spellnum <= 0 || spellnum >= NUM_SKILLS)
    return;
  if (form_num <= 0 || form_num >= NUM_SHAPE_TYPES)
    return;
  if (affected_by_spell(ch, spellnum))
    affect_from_char(ch, spellnum);

  /* should be ok to apply */
  for (i = 0; i < SHAPE_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = spellnum;
    if (spellnum == SKILL_WILDSHAPE)
      af[i].duration = 50 + CLASS_LEVEL(ch, CLASS_DRUID) * GET_WIS_BONUS(ch);
    else
      af[i].duration = 100;
  }

  /* determine stat bonuses, etc */
  SUBRACE(ch) = form_num;
  switch (SUBRACE(ch))
  {
  case PC_SUBRACE_BADGER:
    af[0].location = APPLY_DEX;
    af[0].modifier = 2;
    break;
  case PC_SUBRACE_PANTHER:
    af[0].location = APPLY_DEX;
    af[0].modifier = 8;
    break;
  case PC_SUBRACE_BEAR:
    af[0].location = APPLY_STR;
    af[0].modifier = 8;
    af[1].location = APPLY_CON;
    af[1].modifier = 6;
    af[2].location = APPLY_HIT;
    af[2].modifier = 60;
    break;
  case PC_SUBRACE_G_CROCODILE:
    SET_BIT_AR(af[0].bitvector, AFF_SCUBA);
    SET_BIT_AR(af[0].bitvector, AFF_WATERWALK);
    af[1].location = APPLY_STR;
    af[1].modifier = 6;
    break;
  }

  for (i = 0; i < SHAPE_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  IS_MORPHED(ch) = RACE_TYPE_ANIMAL;

  act(shape_to_char[SUBRACE(ch)], TRUE, ch, 0, 0, TO_CHAR);
  act(shape_to_room[SUBRACE(ch)], TRUE, ch, 0, 0, TO_ROOM);

  if (!IS_NPC(ch) && (spellnum == SKILL_WILDSHAPE))
    start_daily_use_cooldown(ch, FEAT_WILD_SHAPE);

  USE_STANDARD_ACTION(ch);
}

/* a trivial shapechange code for druids, replaced by wildshape */
ACMD(do_shapechange)
{

  int form_num = -1, i = 0, uses_remaining = 0;

  if (!ch->desc || IS_NPC(ch))
    return;

  /*********  added to factor out this command for the time being ******/
  if (IS_MORPHED(ch))
  {
    send_to_char(ch, "You shift back into your natural form...\r\n");
    act("$n shifts back to his natural form.", TRUE, ch, 0, 0, TO_ROOM);
    IS_MORPHED(ch) = 0;
  }

  send_to_char(ch, "This command has been replaced with 'wildshape'\r\n");
  return;
  /*********************************************************************/

  skip_spaces_c(&argument);

  if (!HAS_FEAT(ch, FEAT_WILD_SHAPE))
  {
    send_to_char(ch, "You do not have a wild shape.\r\n");
    return;
  }

  if (((uses_remaining = daily_uses_remaining(ch, FEAT_WILD_SHAPE)) == 0) && *argument)
  {
    send_to_char(ch, "You must recover the energy required to take a wild shape.\r\n");
    return;
  }

  if (!*argument)
  {
    if (CLASS_LEVEL(ch, CLASS_DRUID) < 10)
      form_num = 1;
    if (CLASS_LEVEL(ch, CLASS_DRUID) < 14)
      form_num = 2;
    if (CLASS_LEVEL(ch, CLASS_DRUID) < 14)
      form_num = 3;
    if (CLASS_LEVEL(ch, CLASS_DRUID) >= 14)
      form_num = 4;
    send_to_char(ch, "Available Forms:\r\n\r\n");
    for (i = 1; i <= form_num; i++)
    {
      send_to_char(ch, shape_types[i]);
      send_to_char(ch, "\r\n");
    }
    send_to_char(ch, "\r\nYou can return to your normal form by typing:  "
                     "shapechange normal\r\n");
    return;
  }

  /* should be OK at this point */
  if (is_abbrev(argument, shape_types[1]))
  {
    /* badger */
    form_num = PC_SUBRACE_BADGER;
  }
  else if (is_abbrev(argument, shape_types[2]))
  {
    /* panther */
    form_num = PC_SUBRACE_PANTHER;
  }
  else if (is_abbrev(argument, shape_types[3]))
  {
    /* bear */
    form_num = PC_SUBRACE_BEAR;
  }
  else if (is_abbrev(argument, shape_types[4]))
  {
    /* giant crocodile */
    form_num = PC_SUBRACE_G_CROCODILE;
  }
  else if (is_abbrev(argument, "normal"))
  {
    /* return to normal form */
    SUBRACE(ch) = 0;
    IS_MORPHED(ch) = 0;
    if (affected_by_spell(ch, SKILL_WILDSHAPE))
      affect_from_char(ch, SKILL_WILDSHAPE);
    send_to_char(ch, "You return to your normal form..\r\n");
    return;
  }
  else
  {
    /* invalid */
    send_to_char(ch, "This is not a valid form to shapechange into!\r\n");
    return;
  }

  perform_wildshape(ch, form_num, SKILL_WILDSHAPE);
}

/*****************************/
/* end shapechange functions */

/*****************************/

int display_eligible_disguise_races(struct char_data *ch, const char *argument, int silent)
{
  int i = 0;

  for (i = 0; i < NUM_EXTENDED_RACES; i++)
  {
    switch (race_list[i].family)
    {
    case RACE_TYPE_HUMANOID:
      if (race_list[i].size == GET_SIZE(ch))
      {
        break;
      }
    default:
      continue;
    }

    if (!silent)
    {
      send_to_char(ch, "%s\r\n", race_list[i].name);
    }

    if (!strcmp(argument, race_list[i].name)) /* match argument? */
      break;
  }

  if (i >= NUM_EXTENDED_RACES)
    return 0; /* failed to find anything */
  else
    return i;
}

ACMD(do_disguise)
{
  int i = 0;
  char buf[200];

  skip_spaces_c(&argument);

  if (!GET_ABILITY(ch, ABILITY_DISGUISE))
  {
    send_to_char(ch, "You do not have the ability to disguise!\r\n");
    return;
  }

  if (IS_WILDSHAPED(ch))
  {
    send_to_char(ch, "You cannot disguise while wild shaped.\r\n");
    return;
  }

  if (!*argument)
  {
    send_to_char(ch, "Please select a race to disguise or type 'disguise remove' to remove your disguise.\r\n");
    display_eligible_disguise_races(ch, argument, FALSE);
    return;
  }

  if (strlen(argument) > 100)
  {
    send_to_char(ch, "The race name argument cannot be any longer than 100 characters.\r\n");
    return;
  }

  if (!strcmp(argument, "remove"))
  {
    if (!GET_DISGUISE_RACE(ch))
    {
      send_to_char(ch, "You are not currently disguised.\r\n");
      return;
    }

    GET_DISGUISE_RACE(ch) = 0;
    affect_total(ch);
    save_char(ch, 0);
    Crash_crashsave(ch);

    snprintf(buf, sizeof(buf), "You remove your disguise and now again appear like a: %s.", race_list[GET_RACE(ch)].type);
    act(buf, true, ch, 0, 0, TO_CHAR);
    snprintf(buf, sizeof(buf), "$n removes $s disguise, revealing $s race: %s.", race_list[GET_RACE(ch)].type);
    act(buf, true, ch, 0, 0, TO_ROOM);

    USE_STANDARD_ACTION(ch);

    return;
  }

  /*attempting to apply a disguise*/

  if (AFF_FLAGGED(ch, AFF_WILD_SHAPE))
  {
    send_to_char(ch, "You must 'wildshape return' to return to your normal shape before trying to assume a disguise.\r\n");
    return;
  }
  if (GET_DISGUISE_RACE(ch))
  {
    send_to_char(ch, "You must 'disguise remove' to your normal race before assuming a new disguise.\r\n");
    return;
  }

  /* try to match argument to the list */
  i = display_eligible_disguise_races(ch, argument, TRUE);

  if (i == 0)
  { /* failed to find the race */
    send_to_char(ch, "Please select a race to disguise to or type 'disguise remove'.\r\n");
    display_eligible_disguise_races(ch, argument, FALSE);
    return;
  }

  /* we're in the clear, set the disguise race! */
  GET_DISGUISE_RACE(ch) = i;
  affect_total(ch);
  save_char(ch, 0);
  Crash_crashsave(ch);

  snprintf(buf, sizeof(buf), "You disguises into a %s.", race_list[GET_DISGUISE_RACE(ch)].name);
  act(buf, true, ch, 0, 0, TO_CHAR);
  snprintf(buf, sizeof(buf), "$n disguises into a %s.", race_list[GET_DISGUISE_RACE(ch)].name);
  act(buf, true, ch, 0, 0, TO_ROOM);

  USE_STANDARD_ACTION(ch);

  return;
}

ACMD(do_quit)
{
  int index = 0;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (FIGHTING(ch))
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (GET_POS(ch) < POS_STUNNED)
  {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  }
  else
  {
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has quit the game.", GET_NAME(ch));
    save_char_pets(ch);
    dismiss_all_followers(ch);

    for (index = 0; index < MAX_CURRENT_QUESTS; index++)
    { /* loop through all the character's quest slots */
      if (GET_QUEST_TIME(ch, index) != -1)
        quest_timeout(ch, index);
    }

    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");

    /* We used to check here for duping attempts, but we may as well do it right
     * in extract_char(), since there is no check if a player rents out and it
     * can leave them in an equally screwy situation. */

    if (CONFIG_FREE_RENT)
      Crash_rentsave(ch, 0);

    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    /* Stop snooping so you can't see passwords during deletion or change. */
    if (ch->desc->snoop_by)
    {
      write_to_output(ch->desc->snoop_by, "Your victim is no longer among us.\r\n");
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }

    extract_char(ch); /* Char is saved before extracting. */
  }
}

void perform_save(struct char_data *ch, int mode)
{
  save_char_pets(ch);
  save_char(ch, mode);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
    House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
  GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
}

ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  send_to_char(ch, "Saving %s.\r\n", GET_NAME(ch));

  perform_save(ch, 0);
}

/* Generic function for commands which are normally overridden by special
 * procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}

/* check if we can lore the target */
int can_lore_target(struct char_data *ch, struct char_data *target_ch, struct obj_data *target_obj, bool silent)
{
  bool knowledge = FALSE;
  int lore_bonus = 0;

  /* establish any lore bonus */
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
  if (target_obj && GET_OBJ_COST(target_obj) <=
                        lore_app[(compute_ability(ch, ABILITY_LORE) + lore_bonus)])
  {
    knowledge = TRUE;
  }

  if (target_obj && !knowledge)
  {
    if (!silent)
      send_to_char(ch, "Your knowledge is not extensive enough to know about this object!\r\n");
    return 0;
  }

  /* good enough lore for mobile? */
  knowledge = FALSE;
  lore_bonus += HAS_FEAT(ch, FEAT_MONSTER_LORE) ? GET_WIS_BONUS(ch) : 0;

  if (target_ch && (GET_LEVEL(target_ch) * 2) <= lore_app[(compute_ability(ch, ABILITY_LORE) + lore_bonus)])
  {
    knowledge = TRUE;
  }

  if (target_ch && !knowledge)
  {
    if (!silent)
      send_to_char(ch, "Your knowledge is not extensive enough to know about this creature!\r\n");
    return 0;
  }

  /* we made it! */
  return 1;
}

/* ability lore, functions like identify */
ACMD(do_lore)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  int target = 0;

  if (IS_NPC(ch))
    return;

  if (!IS_NPC(ch) && !GET_ABILITY(ch, ABILITY_LORE))
  {
    send_to_char(ch, "You have no ability to do that!\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  target = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tch, &tobj);

  if (*arg)
  {
    if (!target)
    {
      act("There is nothing to here to use your Lore ability on...", FALSE,
          ch, NULL, NULL, TO_CHAR);
      return;
    }
  }
  else
  {
    tch = ch;
  }

  send_to_char(ch, "You attempt to utilize your vast knowledge of lore...\r\n");
  USE_STANDARD_ACTION(ch);

  if (!can_lore_target(ch, tch, tobj, FALSE))
  {
    return; /* message sent in can_lore_target() */
  }

  /* success! */
  if (tobj)
  {
    do_stat_object(ch, tobj, ITEM_STAT_MODE_LORE_SKILL);
  }
  else if (tch)
  {
    /* victim */
    lore_id_vict(ch, tch);
  }
}

/* ability group lore, functions like identify for all items in bag */
ACMD(do_glore)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *tobj = NULL;

  if (IS_NPC(ch))
    return;

  if (!IS_NPC(ch) && !GET_ABILITY(ch, ABILITY_LORE))
  {
    send_to_char(ch, "You have no ability to do that!\r\n");
    return;
  }

  if (!GROUP(ch))
  {
    send_to_char(ch, "You need to be in a group to use this.\r\n");
    return;
  }

  if (!*argument)
  {
    act("You need to select a target container...", FALSE,
        ch, NULL, NULL, TO_CHAR);
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    act("You need a container to target...", FALSE,
        ch, NULL, NULL, TO_CHAR);
    return;
  }

  if (!(tobj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
  {
    act("You don't see that container...", FALSE,
        ch, NULL, NULL, TO_CHAR);
    return;
  }

  if (GET_OBJ_TYPE(tobj) != ITEM_CONTAINER)
  {
    send_to_char(ch, "You need to target a container bub!\r\n");
    return;
  }

  if (OBJVAL_FLAGGED(tobj, CONT_CLOSED))
  {
    send_to_char(ch, "It is closed.\r\n");
    return;
  }

  send_to_char(ch, "You attempt to utilize your vast knowledge of lore...\r\n");
  USE_STANDARD_ACTION(ch);

  struct obj_data *i = NULL;

  for (i = tobj->contains; i; i = i->next_content)
  {
    if (i && can_lore_target(ch, NULL, i, TRUE))
    {
      do_stat_object(ch, i, ITEM_STAT_MODE_G_LORE);
    }
  }
}

/* a generic command to get rid of a fly / levitate flag */
ACMD(do_land)
{
  bool msg = FALSE;

  if (affected_by_spell(ch, SPELL_FLY))
  {
    affect_from_char(ch, SPELL_FLY);
    msg = TRUE;
  }

  if (affected_by_spell(ch, SKILL_SONG_OF_FLIGHT))
  {
    affect_from_char(ch, SPELL_FLY);
    msg = TRUE;
  }

  if (affected_by_spell(ch, SKILL_DRHRT_WINGS))
  {
    affect_from_char(ch, SKILL_DRHRT_WINGS);
    msg = TRUE;
  }

  if (affected_by_spell(ch, SPELL_LEVITATE))
  {
    affect_from_char(ch, SPELL_LEVITATE);
    msg = TRUE;
  }

  if AFF_FLAGGED (ch, AFF_FLYING)
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    msg = TRUE;
  }

  if AFF_FLAGGED (ch, AFF_LEVITATE)
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_LEVITATE);
    msg = TRUE;
  }

  if (msg)
  {
    send_to_char(ch, "You land on the ground.\r\n");
    act("$n lands on the ground.", TRUE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    send_to_char(ch, "You are not flying or levitating.\r\n");
  }
}

/* enlarge ability (duergar) */
ACMD(do_enlarge)
{
  int uses_remaining = 0;

  if (!HAS_FEAT(ch, FEAT_SLA_ENLARGE))
  {
    send_to_char(ch, "You don't have this ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, SPELL_ENLARGE_PERSON))
  {
    send_to_char(ch, "You already have enhanced strength!\r\n");
    return;
  }

  /*
  if (AFF_FLAGGED(ch, AFF_ENLARGE))
  {
    send_to_char(ch, "You are already enlarged!\r\n");
    return;
  }
  */

  if (!IS_NPC(ch) && ((uses_remaining = daily_uses_remaining(ch, FEAT_SLA_ENLARGE)) == 0))
  {
    send_to_char(ch, "You must recover before you can use this ability again.\r\n");
    return;
  }

  call_magic(ch, ch, NULL, SPELL_ENLARGE_PERSON, 0, GET_LEVEL(ch), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SLA_ENLARGE);
}

/* invisibility (duergar) */
ACMD(do_invisduergar)
{
  int uses_remaining = 0;

  if (!HAS_FEAT(ch, FEAT_SLA_INVIS))
  {
    send_to_char(ch, "You don't have this ability.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
  {
    send_to_char(ch, "You are already invsible!\r\n");
    return;
  }

  if (affected_by_spell(ch, SPELL_INVISIBILITY_SPHERE) || affected_by_spell(ch, SPELL_GREATER_INVIS) ||
      affected_by_spell(ch, SPELL_GREATER_INVIS))
  {
    send_to_char(ch, "You already affected by an invisibility spell!\r\n");
    return;
  }

  if (!IS_NPC(ch) && ((uses_remaining = daily_uses_remaining(ch, FEAT_SLA_INVIS)) == 0))
  {
    send_to_char(ch, "You must recover before you can use this ability again.\r\n");
    return;
  }

  send_to_char(ch, "You invoke your innate ability and slowly begin to fade from sight...  ");
  call_magic(ch, ch, NULL, SPELL_INVISIBLE, 0, GET_LEVEL(ch), CAST_SPELL);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SLA_INVIS);
}

/* strength ability (duergar) */
ACMD(do_strength)
{
  int uses_remaining = 0;

  if (!HAS_FEAT(ch, FEAT_SLA_STRENGTH))
  {
    send_to_char(ch, "You don't have this ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, SPELL_MASS_STRENGTH) || affected_by_spell(ch, SPELL_STRENGTH))
  {
    send_to_char(ch, "You already have enhanced strength!\r\n");
    return;
  }

  if (!IS_NPC(ch) && ((uses_remaining = daily_uses_remaining(ch, FEAT_SLA_STRENGTH)) == 0))
  {
    send_to_char(ch, "You must recover before you can use this ability again.\r\n");
    return;
  }

  call_magic(ch, ch, NULL, SPELL_STRENGTH, 0, GET_LEVEL(ch), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SLA_STRENGTH);
}

/* levitate ability (drow) */
ACMD(do_levitate)
{
  int uses_remaining = 0;

  if (!HAS_FEAT(ch, FEAT_SLA_LEVITATE))
  {
    send_to_char(ch, "You don't have this ability.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_LEVITATE))
  {
    send_to_char(ch, "You are already levitating!\r\n");
    return;
  }

  if (!IS_NPC(ch) && ((uses_remaining = daily_uses_remaining(ch, FEAT_SLA_LEVITATE)) == 0))
  {
    send_to_char(ch, "You must recover before you can use this ability again.\r\n");
    return;
  }

  /*
  SET_BIT_AR(AFF_FLAGS(ch), AFF_FLOAT);
  act("$n begins to levitate above the ground!", TRUE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "You slow rise above the ground and begin to levitate!\r\n");
   */

  // int call_magic(struct char_data *caster, struct char_data *cvict,
  // struct obj_data *ovict, int spellnum, int metamagic, int level, int casttype);
  call_magic(ch, ch, NULL, SPELL_LEVITATE, 0, GET_LEVEL(ch), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SLA_LEVITATE);
}

/* darkness ability (drow) */
ACMD(do_darkness)
{
  int uses_remaining = 0;

  if (!HAS_FEAT(ch, FEAT_SLA_DARKNESS))
  {
    send_to_char(ch, "You don't have this ability.\r\n");
    return;
  }

  if (ROOM_AFFECTED(IN_ROOM(ch), RAFF_DARKNESS))
  {
    send_to_char(ch, "The area is already dark!\r\n");
    return;
  }

  if (!IS_NPC(ch) && ((uses_remaining = daily_uses_remaining(ch, FEAT_SLA_DARKNESS)) == 0))
  {
    send_to_char(ch, "You must recover before you can use this ability again.\r\n");
    return;
  }

  // int call_magic(struct char_data *caster, struct char_data *cvict,
  // struct obj_data *ovict, int spellnum, int metamagic, int level, int casttype);
  call_magic(ch, ch, NULL, SPELL_DARKNESS, 0, GET_LEVEL(ch), CAST_SPELL);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SLA_DARKNESS);
}

/* invisible rogue feat */
ACMD(do_invisiblerogue)
{
  int uses_remaining = 0;

  if (!HAS_FEAT(ch, FEAT_INVISIBLE_ROGUE))
  {
    send_to_char(ch, "You don't have this ability.\r\n");
    return;
  }

  if (!IS_NPC(ch) && ((uses_remaining = daily_uses_remaining(ch, FEAT_INVISIBLE_ROGUE)) == 0))
  {
    send_to_char(ch, "You must recover before you can use this ability again.\r\n");
    return;
  }

  send_to_char(ch, "You invoke your arcane rogue ability and slowly begin to fade from sight...  ");
  call_magic(ch, ch, NULL, SPELL_GREATER_INVIS, 0, GET_LEVEL(ch), CAST_SPELL);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_INVISIBLE_ROGUE);
}

/* race trelux innate ability */
ACMD(do_fly)
{

  if (!can_fly(ch))
  {
    send_to_char(ch, "You don't have this ability.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_FLYING))
  {
    send_to_char(ch, "You are already flying!\r\n");
    return;
  }
  else
  {
    SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    act("$n begins to fly above the ground!", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(ch, "You take off and begin to fly!\r\n");
  }
  // old version just called the spell, but not as nice methinks
  // call_magic(ch, ch, NULL, SPELL_FLY, GET_LEVEL(ch), CAST_SPELL);
}

/* Helper function for 'search' command.
 * Returns the DC of the search attempt to find the specified door. */
int get_hidden_door_dc(struct char_data *ch, int door)
{

  /* (Taken from the d&d 3.5e SRD)
   * Task	                                                Search DC
   * -----------------------------------------------------------------------
   * Ransack a chest full of junk to find a certain item	   10
   * Notice a typical secret door or a simple trap	   20
   * Find a difficult nonmagical trap (rogue only)1	21 or higher
   * Find a magic trap (rogue only)(1)             	25 + lvl of spell
   *                                                     used to create trap
   * Find a footprint	                                 Varies(2)
   * Notice a well-hidden secret door                        30
   * -----------------------------------------------------------------------
   * (1) Dwarves (even if they are not rogues) can use Search to find traps built
   *     into or out of stone.
   * (2) A successful Search check can find a footprint or similar sign of a
   *     creature's passage, but it won't let you find or follow a trail. See the
   *     Track feat for the appropriate DC. */

  /* zusuk bumped up these values slightly from the commented, srd-values because
   of the naturally much higher stats in our MUD world */
  if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN_EASY))
    return 15;
  // return 10;
  if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN_MEDIUM))
    return 30;
  // return 20;
  if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN_HARD))
    return 45;
  // return 30;

  /* If we get here, the door is not hidden. */
  return 0;
}

/* 'search' command, uses the rogue's search skill, if available, although
 * the command is available to all.  */
ACMD(do_search)
{
  int door, found = FALSE;
  //  int val;
  //  struct char_data *i; // for player/mob
  //  struct char_data *list = world[ch->in_room].people; // for player/mob
  //  struct obj_data *objlist = world[ch->in_room].contents;
  //  struct obj_data *obj = NULL;
  //  struct obj_data *cont = NULL;
  //  struct obj_data *next_obj = NULL;
  int search_dc = 0;
  int search_roll = 0;

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You can't do that in combat!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
  {
    send_to_char(ch, "You are unable to move to make your attempt!\r\n");
    return;
  }

  if (!LIGHT_OK(ch))
  {
    send_to_char(ch, "You can't see a thing!\r\n");
    return;
  }

  skip_spaces_c(&argument);

  if (!*argument || true) // we're only doing full room searches right now -- gicker july 12, 2021
  {
    /*
        for (obj = objlist; obj; obj = obj->next_content) {
          if (OBJ_FLAGGED(obj, ITEM_HIDDEN)) {
            SET_BIT(GET_OBJ_SAVED(obj), SAVE_OBJ_EXTRA);
            REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_HIDDEN);
            act("You find $P.", FALSE, ch, 0, obj, TO_CHAR);
            act("$n finds $P.", FALSE, ch, 0, obj, TO_NOTVICT);
            found = TRUE;
            break;
          }
        }*/
    /* find a player/mob */
    /*    if(!found) {
          for (i = list; i; i = i->next_in_room) {
            if ((ch != i) && AFF_FLAGGED(i, AFF_HIDE) && (val < ochance)) {
              affect_from_char(i, SPELL_VACANCY);
              affect_from_char(i, SPELL_MIRAGE_ARCANA);
              affect_from_char(i, SPELL_STONE_BLEND);
              REMOVE_BIT(AFF_FLAGS(i), AFF_HIDE);
              act("You find $N lurking here!", FALSE, ch, 0, i, TO_CHAR);
              act("$n finds $N lurking here!", FALSE, ch, 0, i, TO_NOTVICT);
              act("You have been spotted by $n!", FALSE, ch, 0, i, TO_VICT);
              found = TRUE;
              break;
            }
          }
        }
     */
    if (!found)
    {
      /* find a hidden door */
      for (door = 0; door < NUM_OF_DIRS && found == FALSE; door++)
      {
        if (EXIT(ch, door) && EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN))
        {
          /* Get the DC */
          search_dc = get_hidden_door_dc(ch, door);
          search_roll = d20(ch) + compute_ability(ch, ABILITY_PERCEPTION);
          /* Roll the dice... */
          if (search_roll >= search_dc)
          {
            send_to_char(ch, "roll %d vs. dc %d\r\n", search_roll, search_dc);
            act("You find a secret entrance!", FALSE, ch, 0, 0, TO_CHAR);
            act("$n finds a secret entrance!", FALSE, ch, 0, 0, TO_ROOM);
            REMOVE_BIT(EXIT(ch, door)->exit_info, EX_HIDDEN);
            found = TRUE;
          }
        }
      }
    }
  } /*else {
    generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &i, &cont);
    if(cont) {
      for (obj = cont->contains; obj; obj = next_obj) {
        next_obj = obj->next_content;
        if (IS_OBJ_STAT(obj, ITEM_HIDDEN) && (val < ochance)) {
          SET_BIT(GET_OBJ_SAVED(obj), SAVE_OBJ_EXTRA);
          REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_HIDDEN);
          act("You find $P.", FALSE, ch, 0, obj, TO_CHAR);
          act("$n finds $P.", FALSE, ch, 0, obj, TO_NOTVICT);
          found = TRUE;
          break;
        }
      }
    } else {
      send_to_char("Search what?!?!?!?\r\n", ch);
      found = TRUE;
    }
  } */

  if (!found)
  {
    send_to_char(ch, "You don't find anything you didn't see before.\r\n");
  }

  send_to_char(ch, "Your next action will be delayed up to 6 seconds.\r\n");
  WAIT_STATE(ch, PULSE_VIOLENCE * 1);
  USE_FULL_ROUND_ACTION(ch);
}

/* vanish - epic rogue talent ; free action */
ACMD(do_vanish)
{
  struct char_data *vict, *next_v;
  int uses_remaining = 0;

  if (!HAS_FEAT(ch, FEAT_VANISH))
  {
    send_to_char(ch, "You do not know how to vanish!\r\n");
    return;
  }

  if (((uses_remaining = daily_uses_remaining(ch, FEAT_VANISH)) == 0))
  {
    send_to_char(ch, "You must recover before you can vanish again.\r\n");
    return;
  }

  if (char_has_mud_event(ch, eVANISHED))
  {
    send_to_char(ch, "You must wait longer before you can use this ability again.\r\n");
    return;
  }

  /* success! */
  send_to_char(ch, "You vanish!\r\n");
  act("With an audible pop, you watch as $n vanishes!", FALSE, ch, 0, 0, TO_ROOM);
  start_daily_use_cooldown(ch, FEAT_VANISH);

  /* 12 seconds = 2 rounds */
  attach_mud_event(new_mud_event(eVANISH, ch, NULL), 12 * PASSES_PER_SEC);

  /* stop vanishers combat */
  if (char_has_mud_event(ch, eCOMBAT_ROUND))
  {
    event_cancel_specific(ch, eCOMBAT_ROUND);
  }
  stop_fighting(ch);

  /* stop all those who are fighting vanisher */
  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v)
  {
    next_v = vict->next_in_room;

    if (FIGHTING(vict) == ch)
    {
      if (char_has_mud_event(vict, eCOMBAT_ROUND))
      {
        event_cancel_specific(vict, eCOMBAT_ROUND);
      }
      stop_fighting(vict);
    }

    if (IS_NPC(vict))
      clearMemory(vict);
  }

  GET_HIT(ch) += 10;
  if (HAS_FEAT(ch, FEAT_IMPROVED_VANISH))
    GET_HIT(ch) += 20;

  /* enter stealth mode */
  if (!AFF_FLAGGED(ch, AFF_SNEAK))
  {
    SET_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
  }
  if (!AFF_FLAGGED(ch, AFF_HIDE))
  {
    SET_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
  }
}

/* entry point for sneak, the command just flips the flag */
ACMD(do_sneak)
{

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You can't do that in combat!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
  {
    send_to_char(ch, "You are unable to move to make your attempt!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_SNEAK))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
    send_to_char(ch, "You stop sneaking...\r\n");
    return;
  }

  if (IS_NPC(ch) || !GET_ABILITY(ch, ABILITY_STEALTH))
  {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  send_to_char(ch, "Okay, you'll try to move silently for a while.\r\n");
  SET_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
  USE_SWIFT_ACTION(ch); /*not really necessary honestly*/
}

/* entry point for hide, the command just flips the flag */
ACMD(do_hide)
{

  if (FIGHTING(ch) && !AFF_FLAGGED(ch, AFF_GRAPPLED) && !AFF_FLAGGED(ch, AFF_ENTANGLED))
  {
    if (HAS_FEAT(ch, FEAT_HIDE_IN_PLAIN_SIGHT))
    {
      USE_STANDARD_ACTION(ch);
      if ((skill_roll(FIGHTING(ch), ABILITY_PERCEPTION)) < (skill_roll(ch, ABILITY_STEALTH) - 8))
      {
        /* don't forget to remove the fight event! */
        if (char_has_mud_event(FIGHTING(ch), eCOMBAT_ROUND))
        {
          event_cancel_specific(FIGHTING(ch), eCOMBAT_ROUND);
        }
        stop_fighting(FIGHTING(ch));
        if (char_has_mud_event(ch, eCOMBAT_ROUND))
        {
          event_cancel_specific(ch, eCOMBAT_ROUND);
        }
        stop_fighting(ch);
      }
      else
      {
        send_to_char(ch, "You failed to hide in plain sight!\r\n");
        return;
      }
    }
    else
    {
      send_to_char(ch, "You can't do that in combat!\r\n");
      return;
    }
  }

  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
  {
    send_to_char(ch, "You are unable to move to make your attempt!\r\n");
    return;
  }

  if (IS_NPC(ch) || !GET_ABILITY(ch, ABILITY_STEALTH))
  {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_HIDE))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    send_to_char(ch, "You step out of the shadows...\r\n");
    return;
  }

  send_to_char(ch, "You attempt to hide yourself.\r\n");
  SET_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
  USE_MOVE_ACTION(ch); /* protect from sniping abuse */
}

/* listen-mode, similar to search - try to find hidden/sneaking targets */
ACMD(do_listen)
{
  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
  {
    send_to_char(ch, "You are unable to move to make your attempt!\r\n");
    return;
  }

  /* note, you do not require training in perception to attempt */
  /*
  if (IS_NPC(ch) || !GET_ABILITY(ch, ABILITY_PERCEPTION)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
   */

  if (AFF_FLAGGED(ch, AFF_LISTEN))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_LISTEN);
    send_to_char(ch, "You stop trying to listen...\r\n");
    return;
  }

  send_to_char(ch, "You enter listen mode... (movement cost is doubled)\r\n");
  SET_BIT_AR(AFF_FLAGS(ch), AFF_LISTEN);
}

/* spot-mode, similar to search - try to find hidden/sneaking targets */
ACMD(do_spot)
{
  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
  {
    send_to_char(ch, "You are unable to move to make your attempt!\r\n");
    return;
  }

  /* note, you do not require training in perception to attempt */
  /*
  if (IS_NPC(ch) || !GET_ABILITY(ch, ABILITY_PERCEPTION)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
   */

  if (AFF_FLAGGED(ch, AFF_SPOT))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SPOT);
    send_to_char(ch, "You stop trying to spot...\r\n");
    return;
  }

  send_to_char(ch, "You enter spot mode... (movement cost is doubled)\r\n");
  SET_BIT_AR(AFF_FLAGS(ch), AFF_SPOT);
}

/* fairly stock steal command */
ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (IS_NPC(ch) || !GET_ABILITY(ch, ABILITY_SLEIGHT_OF_HAND))
  {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, sizeof(obj_name), vict_name, sizeof(vict_name));

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "Steal what from who?\r\n");
    return;
  }
  else if (vict == ch)
  {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }

  /* 101% is a complete failure */
  percent = rand_number(1, 35);

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1; /* ALWAYS SUCCESS, unless heavy object. */

  if (!CONFIG_PT_ALLOWED && !IS_NPC(vict))
    pcsteal = 1;

  if (!AWAKE(vict)) /* Easier to steal from sleeping people. */
    percent -= 17;

  /* No stealing if not allowed. If it is no stealing from Imm's or Shopkeepers. */
  if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal || GET_MOB_SPEC(vict) == shop_keeper)
    percent = 100; /* Failure */

  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOSTEAL))
  {
    send_to_char(ch, "Something about this victim makes it clear this will "
                     "not work...\r\n");
    percent = 100;
  }

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold"))
  {

    if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying)))
    {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
        if (GET_EQ(vict, eq_pos) &&
            (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
            CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos)))
        {
          obj = GET_EQ(vict, eq_pos);
          break;
        }
      if (!obj)
      {
        act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
        return;
      }
      else
      { /* It is equipment */
        if ((GET_POS(vict) > POS_STUNNED))
        {
          send_to_char(ch, "Steal the equipment now?  Impossible!\r\n");
          return;
        }
        else
        {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj))
          {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
          act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
          act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
          obj_to_char(unequip_char(vict, eq_pos), ch);
        }
      }
    }
    else
    { /* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj); /* Make heavy harder */

      if (percent > compute_ability(ch, ABILITY_SLEIGHT_OF_HAND))
      {
        ohoh = TRUE;
        send_to_char(ch, "Oops..\r\n");
        act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
        act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      }
      else
      { /* Steal the item */
        if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))
        {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj))
          {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
          if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch))
          {
            obj_from_char(obj);
            obj_to_char(obj, ch);
            send_to_char(ch, "Got it!\r\n");
          }
        }
        else
          send_to_char(ch, "You cannot carry that much.\r\n");
      }
    }
  }
  else
  { /* Steal some coins */
    if (AWAKE(vict) && (percent > compute_ability(ch, ABILITY_SLEIGHT_OF_HAND)))
    {
      ohoh = TRUE;
      send_to_char(ch, "Oops..\r\n");
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    }
    else
    {
      /* Steal some gold coins */
      gold = (GET_GOLD(vict) * rand_number(1, 10)) / 100;
      gold = MIN(1782, gold);
      if (gold > 0)
      {
        increase_gold(ch, gold);
        decrease_gold(vict, gold);
        if (gold > 1)
          send_to_char(ch, "Bingo!  You got %d gold coins.\r\n", gold);
        else
          send_to_char(ch, "You manage to swipe a solitary gold coin.\r\n");
      }
      else
      {
        send_to_char(ch, "You couldn't get any gold...\r\n");
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

  if (affected_by_spell(ch, PSIONIC_BREACH))
  {
    affect_from_char(ch, PSIONIC_BREACH);
  }

  /* Add wait state, stealing isn't free! */
  USE_STANDARD_ACTION(ch);
}

/* entry point for listing spells, the rest of the code is in spec_procs.c */

/* this only lists spells castable for a given class */

/* Ornir:  17.03.16 - Add 'circle' parameter to the command */
ACMD(do_spells)
{
  char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];
  int class = -1, circle = -1;

  if (IS_NPC(ch))
    return;

  two_arguments(argument, arg, sizeof(arg), arg1, sizeof(arg1));

  if (!*arg && subcmd != SCMD_CONCOCT && subcmd != SCMD_POWERS)
  {
    send_to_char(ch, "The spells command requires at least one argument - Usage:  spells <class name> <circle>\r\n");
  }
  else
  {
    if (subcmd == SCMD_CONCOCT)
      class = CLASS_ALCHEMIST;
    else if (subcmd == SCMD_POWERS)
      class = CLASS_PSIONICIST;
    else
      class = get_class_by_name(arg);
    if (class < 0 || class >= NUM_CLASSES)
    {
      send_to_char(ch, "That is not a valid class!\r\n");
      return;
    }
    if (*arg1)
    {
      circle = atoi(arg1);
      if (circle < 1 || circle > 9)
      {
        send_to_char(ch, "That is an invalid %s circle!\r\n", class == CLASS_ALCHEMIST ? "extract" : "spell");
        return;
      }
    }
    if (CLASS_LEVEL(ch, class))
    {
      list_spells(ch, 0, class, circle);
    }
    else
    {
      send_to_char(ch, "You don't have any levels in that class.\r\n");
    }
  }

  send_to_char(ch, "\tDType 'feats' to see your feats\tn\r\n");
  send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
  send_to_char(ch, "\tDType 'boost' to adjust your stats\tn\r\n");
  if (subcmd == SCMD_CONCOCT)
    send_to_char(ch, "\tDType 'extractlist' to see all of your extracts.\tn\r\n");
  else if (subcmd == SCMD_POWERS)
    send_to_char(ch, "\tDType 'powerslist' to see all possible powers.\tn\r\n");
  else
    send_to_char(ch, "\tDType 'spelllist <classname>' to see all your class spells\tn\r\n");
}

/* entry point for listing spells, the rest of the code is in spec_procs.c */

/* this lists all spells attainable for given class */

/* Ornir:  17.03.16 - Add 'circle' parameter to the command */
ACMD(do_spelllist)
{
  char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];
  int class = -1, circle = -1;

  if (IS_NPC(ch))
    return;

  two_arguments(argument, arg, sizeof(arg), arg1, sizeof(arg1));

  if (subcmd == SCMD_CONCOCT)
  {
    class = CLASS_ALCHEMIST;
    if (*arg)
    {
      circle = atoi(arg);
      if (circle < 1 || circle > 9)
      {
        send_to_char(ch, "That is an invalid extract circle!\r\n");
        return;
      }
    }
  }
  else if (subcmd == SCMD_POWERS)
  {
    class = CLASS_PSIONICIST;
    if (*arg)
    {
      circle = atoi(arg);
      if (circle < 1 || circle > 9)
      {
        send_to_char(ch, "That is an invalid power circle!\r\n");
        return;
      }
    }
  }
  else
  {
    if (!*arg)
    {
      send_to_char(ch, "Spelllist requires at least one argument - Usage:  spelllist <class name> <circle>\r\n");
    }
    else
    {
      class = get_class_by_name(arg);
      if (class < 0 || class >= NUM_CLASSES)
      {
        send_to_char(ch, "That is not a valid class!\r\n");
        return;
      }
      if (*arg1)
      {
        circle = atoi(arg1);
        if (circle < 1 || circle > 9)
        {
          send_to_char(ch, "That is an invalid spell circle!\r\n");
          return;
        }
      }
    }
  }

  list_spells(ch, 1, class, circle);

  send_to_char(ch, "\tDType 'feats' to see your feats\tn\r\n");
  send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
  send_to_char(ch, "\tDType 'boost' to adjust your stats\tn\r\n");
  if (subcmd == SCMD_CONCOCT)
    send_to_char(ch, "\tDType 'extracts' to see your currently known extracts\tn\r\n");
  if (subcmd == SCMD_POWERS)
    send_to_char(ch, "\tDType 'powers' to see your currently known powers\tn\r\n");
  else
    send_to_char(ch, "\tDType 'spells <classname>' to see your currently known spells\tn\r\n");
}

/* entry point for boost (stat training), the rest of code is in
   the guild code in spec_procs */
ACMD(do_boosts)
{

  send_to_char(ch, "Boosts are now performed in the study menu.\r\n");
  return;

  // let's keep the old code just in case -- Gicker

  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg, sizeof(arg));

  if (*arg)
    send_to_char(ch, "You can only boost stats with a trainer.\r\n");
  else
    send_to_char(ch, "\tCStat boost sessions remaining: %d\tn\r\n"
                     "\tcStats:\tn\r\n"
                     "Strength\r\n"
                     "Constitution\r\n"
                     "Dexterity\r\n"
                     "Intelligence\r\n"
                     "Wisdom\r\n"
                     "Charisma\r\n"
                     "\tC*Reminder that you can only boost your stats with a trainer.\tn\r\n"
                     "\r\n",
                 GET_BOOSTS(ch));

  send_to_char(ch, "\tDType 'feats' to see your feats\tn\r\n");
  send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
  send_to_char(ch, "\tDType 'boost' to adjust your stats\tn\r\n");
  send_to_char(ch, "\tDType 'craft' to see your crafting proficiency\tn\r\n");
  send_to_char(ch, "\tDType 'spells <classname>' to see your currently known spells\tn\r\n");
}

/* skill practice entry point, the rest of the
 * code is in spec_procs.c guild code */
ACMD(do_practice)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg, sizeof(arg));

  if (*arg)
    ; // send_to_char(ch, "Type '\tYcraft\tn' without an argument to view your crafting skills.\r\n");
  else
    list_crafting_skills(ch);

  send_to_char(ch, "\tDType 'feats' to see your feats\tn\r\n");
  send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
  send_to_char(ch, "\tDType 'boost' to adjust your stats\tn\r\n");
  if (IS_CASTER(ch))
  {
    send_to_char(ch, "\tDType 'spells' to see your spells\tn\r\n");
  }
}

/* ability training entry point, the rest of the
 * code is in spec_procs.c guild code */
ACMD(do_train)
{
  char arg[MAX_INPUT_LENGTH];

  // if (IS_NPC(ch))
  // return;

  one_argument(argument, arg, sizeof(arg));

  if (*arg && is_abbrev(arg, "knowledge"))
  {
    /* Display knowledge abilities. */
    list_abilities(ch, ABILITY_TYPE_KNOWLEDGE);
  }
  else if (*arg && is_abbrev(arg, "craft"))
  {
    /* Display craft abilities. */
    list_abilities(ch, ABILITY_TYPE_CRAFT);
  }
  else if (*arg)
  {
    send_to_char(ch, "Skills are now trained in the study menu.\r\n");
    return;
    // send_to_char(ch, "You can only train abilities with a trainer.\r\n");
  }
  else
    list_abilities(ch, ABILITY_TYPE_GENERAL);

  /* no immediate plans to use knowledge */
  // send_to_char(ch, "\tDType 'train knowledge' to see your knowledge abilities\tn\r\n");
  /* as of 10/30/2014, we have decided to make sure crafting is an indepedent system */
  // send_to_char(ch, "\tDType 'train craft' to see your crafting abilities\tn\r\n");
  send_to_char(ch, "\tDType 'practice' to see your crafting abilities\tn\r\n");
  // send_to_char(ch, "\tDType 'boost' to adjust your stats\tn\r\n");
  if (IS_CASTER(ch))
  {
    send_to_char(ch, "\tDType 'spells' to see your spells\tn\r\n");
  }
}

/* general command to drop any invisibility affects */
ACMD(do_visible)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED (ch, AFF_INVISIBLE)
  {
    appear(ch, TRUE); // forced for greater invis
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  }
  else
    send_to_char(ch, "You are already visible.\r\n");
}

ACMDU(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);
  parse_at(argument);

  if (IS_NPC(ch))
    send_to_char(ch, "Your title is fine... go away.\r\n");
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char(ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (strlen(argument) > MAX_TITLE_LENGTH)
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
  else
  {
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're now %s%s%s.\r\n", GET_NAME(ch), *GET_TITLE(ch) ? " " : "", GET_TITLE(ch));
  }
}

static void print_group(struct char_data *ch)
{
  struct char_data *k = NULL;
  const char *hp_clr = NULL, *psp_clr = NULL, *mv_clr = NULL;
  float hp_pct = 0.0, psp_pct = 0.0, mv_pct = 0.0;

  send_to_char(ch, "Your group consists of:\r\n");

  while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
  {
    hp_pct = ((float)GET_HIT(k)) / ((float)GET_MAX_HIT(k)) * 100.00;
    if (hp_pct >= 100.0)
      hp_clr = CBWHT(ch, C_NRM);
    else if (hp_pct >= 95.0)
      hp_clr = CCNRM(ch, C_NRM);
    else if (hp_pct >= 75.0)
      hp_clr = CBGRN(ch, C_NRM);
    else if (hp_pct >= 55.0)
      hp_clr = CBBLK(ch, C_NRM);
    else if (hp_pct >= 35.0)
      hp_clr = CBMAG(ch, C_NRM);
    else if (hp_pct >= 15.0)
      hp_clr = CBBLU(ch, C_NRM);
    else if (hp_pct >= 1.0)
      hp_clr = CBRED(ch, C_NRM);
    else
      hp_clr = CBFRED(ch, C_NRM);

    mv_pct = ((float)GET_MOVE(k)) / ((float)GET_MAX_MOVE(k)) * 100.00;
    if (mv_pct >= 100.0)
      mv_clr = CBWHT(ch, C_NRM);
    else if (mv_pct >= 95.0)
      mv_clr = CCNRM(ch, C_NRM);
    else if (mv_pct >= 75.0)
      mv_clr = CBGRN(ch, C_NRM);
    else if (mv_pct >= 55.0)
      mv_clr = CBBLK(ch, C_NRM);
    else if (mv_pct >= 35.0)
      mv_clr = CBMAG(ch, C_NRM);
    else if (mv_pct >= 15.0)
      mv_clr = CBBLU(ch, C_NRM);
    else if (mv_pct >= 1.0)
      mv_clr = CBRED(ch, C_NRM);
    else
      mv_clr = CBFRED(ch, C_NRM);

    psp_pct = ((float)GET_PSP(k)) / ((float)GET_MAX_PSP(k)) * 100.00;
    if (psp_pct >= 100.0)
      psp_clr = CBWHT(ch, C_NRM);
    else if (psp_pct >= 95.0)
      psp_clr = CCNRM(ch, C_NRM);
    else if (psp_pct >= 75.0)
      psp_clr = CBGRN(ch, C_NRM);
    else if (psp_pct >= 55.0)
      psp_clr = CBBLK(ch, C_NRM);
    else if (psp_pct >= 35.0)
      psp_clr = CBMAG(ch, C_NRM);
    else if (psp_pct >= 15.0)
      psp_clr = CBBLU(ch, C_NRM);
    else if (psp_pct >= 1.0)
      psp_clr = CBRED(ch, C_NRM);
    else
      psp_clr = CBFRED(ch, C_NRM);

    send_to_char(ch, "%s%-*s: [%s%4d\tn/%-4d]H [%s%4d\tn/%-4d]P [%s%4d\tn/%-4d]V [%d XP TNL]%s\r\n",
                 GROUP_LEADER(GROUP(ch)) == k ? "\tG*\tn" : " ",
                 count_color_chars(GET_NAME(k)) + 28, GET_NAME(k),
                 hp_clr, GET_HIT(k), GET_MAX_HIT(k),
                 psp_clr, GET_PSP(k), GET_MAX_PSP(k),
                 mv_clr, GET_MOVE(k), GET_MAX_MOVE(k),
                 MAX(0, level_exp(k, GET_LEVEL(k) + 1) - GET_EXP(k)),
                 CCNRM(ch, C_NRM));
  }
}

/* Putting this here - no better place to put it really. */
void update_msdp_group(struct char_data *ch)
{
  char msdp_buffer[MAX_STRING_LENGTH];
  struct char_data *k;

  /* MSDP */

  msdp_buffer[0] = '\0';
  if (ch && ch->desc)
  {
    if (ch->group)
    {
      while ((k = (struct char_data *)simple_list(ch->group->members)) != NULL)
      {
        char buf[4000]; // Buffer for building the group table for MSDP
        // send_to_char(ch, "DEBUG: group member: %s", GET_NAME(k));
        snprintf(buf, sizeof(buf), "%c%c"
                                   "%c%s%c%s"
                                   "%c%s%c%d"
                                   "%c%s%c%d"
                                   "%c%s%c%d"
                                   "%c%s%c%d"
                                   "%c%s%c%d"
                                   "%c%s%c%d"
                                   "%c",
                 (char)MSDP_VAL,
                 (char)MSDP_TABLE_OPEN,
                 (char)MSDP_VAR, "NAME", (char)MSDP_VAL, GET_NAME(k),
                 (char)MSDP_VAR, "LEVEL", (char)MSDP_VAL, GET_LEVEL(k),
                 (char)MSDP_VAR, "IS_LEADER", (char)MSDP_VAL, (GROUP_LEADER(GROUP(k)) == k ? 1 : 0),
                 (char)MSDP_VAR, "HEALTH", (char)MSDP_VAL, GET_HIT(k),
                 (char)MSDP_VAR, "HEALTH_MAX", (char)MSDP_VAL, GET_MAX_HIT(k),
                 (char)MSDP_VAR, "MOVEMENT", (char)MSDP_VAL, GET_MOVE(k),
                 (char)MSDP_VAR, "MOVEMENT_MAX", (char)MSDP_VAL, GET_MAX_MOVE(k),
                 (char)MSDP_TABLE_CLOSE);
        strlcat(msdp_buffer, buf, sizeof(msdp_buffer));
      }
    }
    // send_to_char(ch,"%s", msdp_buffer);
    strip_colors(msdp_buffer);
    MSDPSetArray(ch->desc, eMSDP_GROUP, msdp_buffer);
  }
}

void update_msdp_inventory(struct char_data *ch)
{
  char msdp_buffer[MAX_STRING_LENGTH];
  obj_data *obj;
  int i = 0;

  /* Inventory */
  msdp_buffer[0] = '\0';
  if (ch && ch->desc)
  {
    /* --------- Comment out the following if you don't want to mix eq and worn ---------- */
    for (i = 0; i < NUM_WEARS; i++)
    {
      if (GET_EQ(ch, i))
      {
        if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
        {
          char buf[4000]; // Buffer for building the inventory table for MSDP
          obj = GET_EQ(ch, i);
          snprintf(buf, sizeof(buf), "%c%c"
                                     "%c%s%c%s"
                                     "%c%s%c%s"
                                     "%c",
                   (char)MSDP_VAL,
                   (char)MSDP_TABLE_OPEN,
                   (char)MSDP_VAR, "LOCATION", (char)MSDP_VAL, equipment_types[i],
                   (char)MSDP_VAR, "NAME", (char)MSDP_VAL, obj->short_description,
                   (char)MSDP_TABLE_CLOSE);
          strlcat(msdp_buffer, buf, sizeof(msdp_buffer));
        }
      }
    }
    /* ---------- End Worn Equipment ----------*/

    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
      if (CAN_SEE_OBJ(ch, obj))
      {
        char buf[4000]; // Buffer for building the inventory table for MSDP
        snprintf(buf, sizeof(buf), "%c%c"
                                   "%c%s%c%s"
                                   "%c%s%c%s"
                                   "%c",
                 (char)MSDP_VAL,
                 (char)MSDP_TABLE_OPEN,
                 (char)MSDP_VAR, "LOCATION", (char)MSDP_VAL, "Inventory",
                 (char)MSDP_VAR, "NAME", (char)MSDP_VAL, obj->short_description,
                 (char)MSDP_TABLE_CLOSE);
        strlcat(msdp_buffer, buf, sizeof(msdp_buffer));
      }
    }
    strip_colors(msdp_buffer);
    MSDPSetArray(ch->desc, eMSDP_INVENTORY, msdp_buffer);
  }
}

static void display_group_list(struct char_data *ch)
{
  struct group_data *group = NULL;
  int count = 0;

  if (group_list->iSize)
  {
    send_to_char(ch,
                 "#   Group Leader     # of Mem  Open?  In Zone\r\n"
                 "-------------------------------------------------------------------\r\n");

    while ((group = (struct group_data *)simple_list(group_list)) != NULL)
    {
      /* we don't display npc groups */
      if (IS_SET(GROUP_FLAGS(group), GROUP_NPC))
        continue;
      if (GROUP_LEADER(group) && !IS_SET(GROUP_FLAGS(group), GROUP_ANON))
        send_to_char(ch, "%-2d) %s%-12s     %-2d        %-3s    %s%s\r\n",
                     ++count, IS_SET(GROUP_FLAGS(group), GROUP_OPEN) ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM), GET_NAME(GROUP_LEADER(group)),
                     group->members->iSize, IS_SET(GROUP_FLAGS(group), GROUP_OPEN) ? "\tWYes\tn" : "\tRNo \tn",
                     zone_table[world[IN_ROOM(GROUP_LEADER(group))].zone].name,
                     CCNRM(ch, C_NRM));
      else
        send_to_char(ch, "%-2d) Hidden\r\n", ++count);
    }
  }

  if (count)
    send_to_char(ch, "\r\n");
  /*
                       "%sSeeking Members%s\r\n"
                       "%sClosed%s\r\n",
                       CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                       CCRED(ch, C_NRM), CCNRM(ch, C_NRM));*/
  else
    send_to_char(ch, "\r\n"
                     "Currently no groups formed.\r\n");
}

// vatiken's group system 1.2, installed 08/08/12 by zusuk

ACMDU(do_group)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  argument = one_argument_u(argument, buf);

  if (!*buf)
  {
    if (GROUP(ch))
      print_group(ch);
    else
      send_to_char(ch, "You must specify a group option, or type HELP GROUP for more info.\r\n");
    return;
  }

  /* creating a new group */
  if (is_abbrev(buf, "new"))
  {
    if (GROUP(ch))
      send_to_char(ch, "You are already in a group.\r\n");
    else
      create_group(ch);
  }

  /* see all the viewable groups in the game */
  else if (is_abbrev(buf, "list"))
    display_group_list(ch);

  /* try to join a group by name of member */
  else if (is_abbrev(buf, "join"))
  {
    skip_spaces(&argument);
    if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_WORLD)))
    {
      send_to_char(ch, "Join who?\r\n");
      return;
    }
    else if (vict == ch)
    {
      send_to_char(ch, "That would be one lonely grouping.\r\n");
      return;
    }
    else if (GROUP(ch))
    {
      send_to_char(ch, "But you are already part of a group.\r\n");
      return;
    }
    else if (!GROUP(vict))
    {
      send_to_char(ch, "They are not a part of a group!\r\n");
      return;
    }
    else if (IS_NPC(vict))
    {
      send_to_char(ch, "You can't join that group!\r\n");
      return;
    }
    else if (!IS_SET(GROUP_FLAGS(GROUP(vict)), GROUP_OPEN))
    {
      send_to_char(ch, "That group isn't accepting members.\r\n");
      return;
    }

    /* made it! */
    join_group(ch, GROUP(vict));
  }

  /* leader can kick members */
  else if (is_abbrev(buf, "kick"))
  {
    skip_spaces(&argument);
    if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "Kick out who?\r\n");
      return;
    }
    else if (vict == ch)
    {
      send_to_char(ch, "There are easier ways to leave the group.\r\n");
      return;
    }
    else if (!GROUP(ch))
    {
      send_to_char(ch, "But you are not part of a group.\r\n");
      return;
    }
    else if (GROUP_LEADER(GROUP(ch)) != ch)
    {
      send_to_char(ch, "Only the group's leader can kick members out.\r\n");
      return;
    }
    else if (GROUP(vict) != GROUP(ch))
    {
      send_to_char(ch, "They are not a member of your group!\r\n");
      return;
    }
    send_to_char(ch, "You have kicked %s out of the group.\r\n", GET_NAME(vict));
    send_to_char(vict, "You have been kicked out of the group.\r\n");
    leave_group(vict);
  }

  /* member can leave group */
  else if (is_abbrev(buf, "leave"))
  {
    if (!GROUP(ch))
    {
      send_to_char(ch, "But you aren't apart of a group!\r\n");
      return;
    }
    leave_group(ch);
  }

  /* options for this particular group */
  else if (is_abbrev(buf, "option"))
  {
    skip_spaces(&argument);

    if (!GROUP(ch))
    {
      send_to_char(ch, "But you aren't part of a group!\r\n");
      return;
    }
    else if (GROUP_LEADER(GROUP(ch)) != ch)
    {
      send_to_char(ch, "Only the group leader can adjust the group flags.\r\n");
      return;
    }

    /* whether new members can join this group */
    if (is_abbrev(argument, "open"))
    {
      TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN);
      send_to_char(ch, "The group is now %s to new members.\r\n",
                   IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN) ? "OPEN" : "CLOSED");
    }

    /* whether this group will show up in 'group list' */
    else if (is_abbrev(argument, "anonymous"))
    {
      TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_ANON);
      send_to_char(ch, "The group location is now %s to other players.\r\n",
                   IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_ANON) ? "INVISIBLE" : "VISIBLE");
    }

    /* whether this group will be using the group loot system */
    /*
    else if (is_abbrev(argument, "lootz"))
    {
      TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_LOOTZ);
      send_to_char(ch, "The group loot system is now %s for the group.\r\n",
                   IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_LOOTZ) ? "ON" : "OFF");
    }
    */

    /* invalid input given for option */
    else
    {
      send_to_char(ch, "The flag options are:\r\n"
                       " Open - whether new members can join this group\r\n"
                       " Anonymous - whether this group will show up in the 'group list' command\r\n"
                       /*" Lootz - whether this group will be using the group loot system\r\n"*/
                       "\r\n");
    }
  }

  /* invalid input */
  else
  {
    send_to_char(ch, "You must specify a group option, or type HELP GROUP for more info.\r\n");
  }

  update_msdp_group(ch);
  if (ch->desc)
    MSDPFlush(ch->desc, eMSDP_GROUP);
}

/* the actual group report command */
ACMD(do_greport)
{
  struct group_data *group = NULL;

  if ((group = GROUP(ch)) == NULL)
  {
    send_to_char(ch, "But you are not a member of any group!\r\n");
    return;
  }

  const char *hp_clr = NULL, *psp_clr = NULL, *mv_clr = NULL;
  float hp_pct = 0.0, psp_pct = 0.0, mv_pct = 0.0;

  hp_pct = ((float)GET_HIT(ch)) / ((float)GET_MAX_HIT(ch)) * 100.00;
  if (hp_pct >= 100.0)
    hp_clr = CBWHT(ch, C_NRM);
  else if (hp_pct >= 95.0)
    hp_clr = CCNRM(ch, C_NRM);
  else if (hp_pct >= 75.0)
    hp_clr = CBGRN(ch, C_NRM);
  else if (hp_pct >= 55.0)
    hp_clr = CBBLK(ch, C_NRM);
  else if (hp_pct >= 35.0)
    hp_clr = CBMAG(ch, C_NRM);
  else if (hp_pct >= 15.0)
    hp_clr = CBBLU(ch, C_NRM);
  else if (hp_pct >= 1.0)
    hp_clr = CBRED(ch, C_NRM);
  else
    hp_clr = CBFRED(ch, C_NRM);

  mv_pct = ((float)GET_MOVE(ch)) / ((float)GET_MAX_MOVE(ch)) * 100.00;
  if (mv_pct >= 100.0)
    mv_clr = CBWHT(ch, C_NRM);
  else if (mv_pct >= 95.0)
    mv_clr = CCNRM(ch, C_NRM);
  else if (mv_pct >= 75.0)
    mv_clr = CBGRN(ch, C_NRM);
  else if (mv_pct >= 55.0)
    mv_clr = CBBLK(ch, C_NRM);
  else if (mv_pct >= 35.0)
    mv_clr = CBMAG(ch, C_NRM);
  else if (mv_pct >= 15.0)
    mv_clr = CBBLU(ch, C_NRM);
  else if (mv_pct >= 1.0)
    mv_clr = CBRED(ch, C_NRM);
  else
    mv_clr = CBFRED(ch, C_NRM);

  if (IS_PSI_TYPE(ch))
  {

    psp_pct = ((float)GET_PSP(ch)) / ((float)GET_MAX_PSP(ch)) * 100.00;
    if (psp_pct >= 100.0)
      psp_clr = CBWHT(ch, C_NRM);
    else if (psp_pct >= 95.0)
      psp_clr = CCNRM(ch, C_NRM);
    else if (psp_pct >= 75.0)
      psp_clr = CBGRN(ch, C_NRM);
    else if (psp_pct >= 55.0)
      psp_clr = CBBLK(ch, C_NRM);
    else if (psp_pct >= 35.0)
      psp_clr = CBMAG(ch, C_NRM);
    else if (psp_pct >= 15.0)
      psp_clr = CBBLU(ch, C_NRM);
    else if (psp_pct >= 1.0)
      psp_clr = CBRED(ch, C_NRM);
    else
      psp_clr = CBFRED(ch, C_NRM);

    send_to_group(NULL, group, "%s \tnreports: %s%d/%d\tnH, %s%d/%d\tnP, %s%d/%d\tnV\r\n",
                  GET_NAME(ch), hp_clr, GET_HIT(ch), GET_MAX_HIT(ch),
                  psp_clr, GET_PSP(ch), GET_MAX_PSP(ch),
                  mv_clr, GET_MOVE(ch), GET_MAX_MOVE(ch));
  }
  else
  {
    send_to_group(NULL, group, "%s \tnreports: %s%d/%d\tnH, %s%d/%d\tnV\r\n",
                  GET_NAME(ch), hp_clr, GET_HIT(ch), GET_MAX_HIT(ch),
                  mv_clr, GET_MOVE(ch), GET_MAX_MOVE(ch));
  }
}

/* this use to be group report, switched it to general */
ACMD(do_report)
{

  /* generalized output due to send_to_room */
  // send_to_room(IN_ROOM(ch), "%s status: %d/%dH, %d/%dM, %d/%dV\r\n",
  send_to_room(IN_ROOM(ch), "%s status: %d/%dH, %d/%dV, %d/%dP, %d XP TNL\r\n",
               GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
               GET_MOVE(ch), GET_MAX_MOVE(ch),
               GET_PSP(ch), GET_MAX_PSP(ch),
               MAX(0, level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch)));
}

ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num = 0, share, rest;
  size_t len;
  struct char_data *k;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf, sizeof(buf));

  if (is_number(buf))
  {
    amount = atoi(buf);
    if (amount <= 0)
    {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > GET_GOLD(ch))
    {
      send_to_char(ch, "You don't seem to have that much gold to split.\r\n");
      return;
    }

    if (GROUP(ch))
      while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
        if (IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k))
          num++;

    if (num && GROUP(ch))
    {
      share = amount / num;
      rest = amount % num;
    }
    else
    {
      send_to_char(ch, "With whom do you wish to share your gold?\r\n");
      return;
    }

    decrease_gold(ch, share * (num - 1));

    /* Abusing signed/unsigned to make sizeof work. */
    len = snprintf(buf, sizeof(buf), "%s splits %d coins; you receive %d.\r\n",
                   GET_NAME(ch), amount, share);
    if (rest && len < sizeof(buf))
    {
      snprintf(buf + len, sizeof(buf) - len,
               "%d coin%s %s not splitable, so %s keeps the money.\r\n", rest,
               (rest == 1) ? "" : "s", (rest == 1) ? "was" : "were", GET_NAME(ch));
    }

    while ((k = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
      if (k != ch && IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k))
      {
        increase_gold(k, share);
        send_to_char(k, "%s", buf);
      }
    send_to_char(ch, "You split %d coins among %d members -- %d coins each.\r\n",
                 amount, num, share);

    if (rest)
    {
      send_to_char(ch, "%d coin%s %s not splitable, so you keep the money.\r\n",
                   rest, (rest == 1) ? "" : "s", (rest == 1) ? "was" : "were");
      increase_gold(ch, rest);
    }
  }
  else
  {
    send_to_char(ch, "How many coins do you wish to split with your group?\r\n");
    return;
  }
}

/* lazy hack to fix some troublesome staves in-game */
bool invalid_staff_spell(int spell_num)
{
  bool is_invalid = FALSE;

  switch (spell_num)
  {
  case SPELL_EARTHQUAKE: /* fallthrough */
  case SPELL_WIZARD_EYE:
    is_invalid = TRUE;
    break;

  default:
    break;
  }

  return is_invalid;
}

ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'}, arg[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *mag_item = NULL;
  int dc = 10;
  int check_result;
  int spell;
  int umd_ability_score;

  if (subcmd == SCMD_QUAFF && affected_by_spell(ch, PSIONIC_OAK_BODY))
  {
    send_to_char(ch, "You can't quaff potions while in oak body form.\r\n");
    return;
  }

  if (subcmd == SCMD_QUAFF && affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
  {
    send_to_char(ch, "You can't quaff potions while in iron body form.\r\n");
    return;
  }

  half_chop_c(argument, arg, sizeof(arg), buf, sizeof(buf));

  if (!*arg)
  {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }

  // Find the item - check both held slots, as well as the 2H slot.
  mag_item = GET_EQ(ch, WEAR_HOLD_1);
  if (!mag_item || !isname(arg, mag_item->name))
    mag_item = GET_EQ(ch, WEAR_HOLD_2);
  if (!mag_item || !isname(arg, mag_item->name))
    mag_item = GET_EQ(ch, WEAR_HOLD_2H);

  if (!mag_item || !isname(arg, mag_item->name))
  {
    switch (subcmd)
    {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
      {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        return;
      }
      break;
    case SCMD_USE:
      send_to_char(ch, "You don't seem to be holding %s %s.\r\n",
                   AN(arg), arg);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      /* SYSERR_DESC: This is the same as the unhandled case in do_gen_ps(),
       * but in the function which handles 'quaff', 'recite', and 'use'. */
      return;
    }
  }

  /* Check for object existence. */
  switch (subcmd)
  {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION)
    {
      send_to_char(ch, "You can only quaff potions.\r\n");
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL)
    {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
        (GET_OBJ_TYPE(mag_item) != ITEM_STAFF) &&
        (!HAS_SPECIAL_ABILITIES(mag_item)) &&
        (GET_OBJ_VNUM(mag_item) != PRISTINEHORN_PRIZE))
    {
      send_to_char(ch, "You can't seem to figure out how to use it.\r\n");
      return;
    }
    break;
  }

  if ((GET_OBJ_BOUND_ID(mag_item) != NOBODY) && (GET_OBJ_BOUND_ID(mag_item) != GET_IDNUM(ch)))
  {
    if (get_name_by_id(GET_OBJ_BOUND_ID(mag_item)) != NULL)
    {
      switch (subcmd)
      {
      case SCMD_QUAFF:
        send_to_char(ch, "That potion belongs to %s, go quaff your own!\r\n", CAP(get_name_by_id(GET_OBJ_BOUND_ID(mag_item))));
        break;

      case SCMD_RECITE:
        send_to_char(ch, "That scroll belongs to %s, go recite your own!\r\n", CAP(get_name_by_id(GET_OBJ_BOUND_ID(mag_item))));
        break;

      case SCMD_USE:
        if (GET_OBJ_TYPE(mag_item) == ITEM_WAND)
          send_to_char(ch, "That wand belongs to %s, go wave your own about!\r\n", CAP(get_name_by_id(GET_OBJ_BOUND_ID(mag_item))));
        else
          send_to_char(ch, "That staff belongs to %s, go wave your own about!\r\n", CAP(get_name_by_id(GET_OBJ_BOUND_ID(mag_item))));
        break;
      }
      return;
    }
  }

  /* Check if we can actually use the item in question... */
  switch (subcmd)
  {

  case SCMD_RECITE:

    spell = GET_OBJ_VAL(mag_item, 1);

    /* remove curse, dispel invis and identify you can use regardless */
    if (spell == SPELL_REMOVE_CURSE || spell == SPELL_IDENTIFY || SPELL_DISPEL_INVIS)
      break;

    /* 1. Decipher Writing
     *    Spellcraft check: DC 20 + spell level */

    dc = 20 + GET_OBJ_VAL(mag_item, 0);
    if (((check_result = skill_check(ch, ABILITY_SPELLCRAFT, dc)) < 0) &&
        ((check_result = skill_check(ch, ABILITY_USE_MAGIC_DEVICE, dc + 5)) < 0))
    {
      send_to_char(ch, "You are unable to decipher the magical writings!\r\n");
      return;
    }

    /* 2. Activate the Spell */

    /* 2.a. Check the spell type
     *      ARCANE - Wizard, Sorcerer, Bard
     *      DIVINE - Cleric, Druid, Paladin, Ranger */
    if ((check_result = skill_check(ch, ABILITY_USE_MAGIC_DEVICE, dc)) < 0)
    {
      if (spell_info[spell].min_level[CLASS_WIZARD] < LVL_STAFF ||
          spell_info[spell].min_level[CLASS_SORCERER] < LVL_STAFF ||
          spell_info[spell].min_level[CLASS_BARD] < LVL_STAFF)
      {
        if (!(CLASS_LEVEL(ch, CLASS_WIZARD) > 0 ||
              CLASS_LEVEL(ch, CLASS_SORCERER) > 0 ||
              CLASS_LEVEL(ch, CLASS_BARD) > 0))
        {
          send_to_char(ch, "You must be able to use arcane magic to recite this scroll.\r\n");
          return;
        }
      }
      else
      {
        if (!(CLASS_LEVEL(ch, CLASS_CLERIC) > 0 ||
              CLASS_LEVEL(ch, CLASS_DRUID) > 0 ||
              CLASS_LEVEL(ch, CLASS_INQUISITOR) > 0 ||
              CLASS_LEVEL(ch, CLASS_PALADIN) > 0 ||
              CLASS_LEVEL(ch, CLASS_RANGER) > 0))
        {
          send_to_char(ch, "You must be able to cast divine magic to recite this scroll.\r\n");
          return;
        }
      }

      int i;
      i = MIN_SPELL_LVL(spell, CLASS_CLERIC, DOMAIN_AIR);

      /* 2.b. Check the spell is on class spell list */
      if (!(((spell_info[spell].min_level[CLASS_WIZARD] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_WIZARD) > 0) ||
            ((spell_info[spell].min_level[CLASS_SORCERER] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_SORCERER) > 0) ||
            ((spell_info[spell].min_level[CLASS_BARD] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_BARD) > 0) ||
            ((MIN_SPELL_LVL(spell, CLASS_INQUISITOR, GET_1ST_DOMAIN(ch)) < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_INQUISITOR) > 0) ||
            ((MIN_SPELL_LVL(spell, CLASS_CLERIC, GET_1ST_DOMAIN(ch)) < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_CLERIC) > 0) ||
            ((MIN_SPELL_LVL(spell, CLASS_CLERIC, GET_2ND_DOMAIN(ch)) < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_CLERIC) > 0) ||
            ((spell_info[spell].min_level[CLASS_DRUID] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_DRUID) > 0) ||
            ((spell_info[spell].min_level[CLASS_PALADIN] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_PALADIN) > 0) ||
            ((spell_info[spell].min_level[CLASS_RANGER] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_RANGER) > 0)))
      {
        send_to_char(ch, "The spell on the scroll is outside your realm of knowledge.\r\n");
        return;
      }
    }

    /* 2.c. Check the relevant ability score */
    /* SPELL PREPARATION HOOK (spellCircle) */
    umd_ability_score = (skill_check(ch, ABILITY_USE_MAGIC_DEVICE, 15));
    bool passed = FALSE;
    if (spell_info[spell].min_level[CLASS_WIZARD] < LVL_STAFF)
      passed = (((GET_INT(ch) > umd_ability_score) ? GET_INT(ch) : umd_ability_score) > (10 + compute_spells_circle(CLASS_WIZARD, spell, 0, DOMAIN_UNDEFINED)) ? TRUE : passed);
    if (spell_info[spell].min_level[CLASS_SORCERER] < LVL_STAFF)
      passed = (((GET_CHA(ch) > umd_ability_score) ? GET_CHA(ch) : umd_ability_score) > (10 + compute_spells_circle(CLASS_SORCERER, spell, 0, DOMAIN_UNDEFINED)) ? TRUE : passed);
    if (spell_info[spell].min_level[CLASS_BARD] < LVL_STAFF)
      passed = (((GET_CHA(ch) > umd_ability_score) ? GET_CHA(ch) : umd_ability_score) > (10 + compute_spells_circle(CLASS_BARD, spell, 0, DOMAIN_UNDEFINED)) ? TRUE : passed);
    if (MIN_SPELL_LVL(spell, CLASS_INQUISITOR, GET_1ST_DOMAIN(ch)) < LVL_STAFF)
      passed = (((GET_WIS(ch) > umd_ability_score) ? GET_WIS(ch) : umd_ability_score) > (10 + compute_spells_circle(CLASS_INQUISITOR, spell, 0, GET_1ST_DOMAIN(ch))) ? TRUE : passed);
    if (MIN_SPELL_LVL(spell, CLASS_CLERIC, GET_1ST_DOMAIN(ch)) < LVL_STAFF)
      passed = (((GET_WIS(ch) > umd_ability_score) ? GET_WIS(ch) : umd_ability_score) > (10 + compute_spells_circle(CLASS_CLERIC, spell, 0, GET_1ST_DOMAIN(ch))) ? TRUE : passed);
    if (MIN_SPELL_LVL(spell, CLASS_CLERIC, GET_2ND_DOMAIN(ch)) < LVL_STAFF)
      passed = (((GET_WIS(ch) > umd_ability_score) ? GET_WIS(ch) : umd_ability_score) > (10 + compute_spells_circle(CLASS_CLERIC, spell, 0, GET_2ND_DOMAIN(ch))) ? TRUE : passed);
    if (spell_info[spell].min_level[CLASS_DRUID] < LVL_STAFF)
      passed = (((GET_WIS(ch) > umd_ability_score) ? GET_WIS(ch) : umd_ability_score) > (10 + compute_spells_circle(CLASS_DRUID, spell, 0, DOMAIN_UNDEFINED)) ? TRUE : passed);
    if (spell_info[spell].min_level[CLASS_PALADIN] < LVL_STAFF)
      passed = (((GET_CHA(ch) > umd_ability_score) ? GET_CHA(ch) : umd_ability_score) > (10 + compute_spells_circle(CLASS_PALADIN, spell, 0, DOMAIN_UNDEFINED)) ? TRUE : passed);
    if (spell_info[spell].min_level[CLASS_RANGER] < LVL_STAFF)
      passed = (((GET_WIS(ch) > umd_ability_score) ? GET_WIS(ch) : umd_ability_score) > (10 + compute_spells_circle(CLASS_RANGER, spell, 0, DOMAIN_UNDEFINED)) ? TRUE : passed);
    if (passed == FALSE)
    {
      send_to_char(ch, "You are physically incapable of casting the spell inscribed on the scroll.\r\n");
      return;
    }

    /* 3. Check caster level */
    if ((CASTER_LEVEL(ch) < GET_OBJ_VAL(mag_item, 0)) &&
        (check_result && GET_LEVEL(ch) < GET_OBJ_VAL(mag_item, 0)))
    {
      /* Perform caster level check */
      dc = GET_OBJ_VAL(mag_item, 0) + 1;
      if (d20(ch) + (((check_result >= 0) && (CASTER_LEVEL(ch) < GET_LEVEL(ch))) ? GET_LEVEL(ch) : CASTER_LEVEL(ch)) < dc)
      {
        /* Fail */
        send_to_char(ch, "You try, but the spell on the scroll is far to powerful for you to cast.\r\n");
        return;
      }
      else
      {
        send_to_char(ch, "You release the powerful magics inscribed the scroll!\r\n");
      }
    }
    break;

  case SCMD_USE:

    /*special cases*/
    if (GET_OBJ_VNUM(mag_item) == PRISTINEHORN_PRIZE)
    {
      if (GET_OBJ_VAL(mag_item, 0) >= 0)
      {
        send_to_char(ch, "You release the powerful magics inscribed on the token!\r\n");
        act("$n raises $p in the air releasing powerful magic...", TRUE, ch, mag_item, 0, TO_ROOM);

        GET_OBJ_VAL(mag_item, 0) = -1; /* just a marker that it has been used */

        invoke_happyhour(ch);

        return;
      }
      else
      {
        send_to_char(ch, "Apparently the power in this token has already been consumed...\r\n");
        return;
      }
    }
    /*end special cases */

    /* Check the item type */
    switch (GET_OBJ_TYPE(mag_item))
    {

    case ITEM_INSTRUMENT: /* OMG WHAT A HACK */
    case ITEM_SUMMON:     /* OMG WHAT A HACK - fallthrough -zusuk */
      if (HAS_SPECIAL_ABILITIES(mag_item))
      {
        process_item_abilities(mag_item, ch, NULL, ACTMTD_USE, NULL);
        return;
      }
      break;
    case ITEM_WEAPON:
      /* Special Abilities */
      break;
    case ITEM_WAND:
    case ITEM_STAFF:

      spell = GET_OBJ_VAL(mag_item, 3);

      /* zusuk lazy solution to stop some troublesome spells */
      if (invalid_staff_spell(spell))
      {
        send_to_char(ch, "Tell a staff member that this staff is using an invalid spell (%d) please.\r\n", spell);
        return;
      }

      /* Check requirements for using a wand: Spell Trigger method */
      /* 1. Class must be able to cast the spell stored in the wand. Use Magic Device can bluff this. */

      dc = 20;

      if ((check_result = skill_check(ch, ABILITY_USE_MAGIC_DEVICE, dc)) < 0)
      {
        if (spell_info[spell].min_level[CLASS_WIZARD] < LVL_STAFF ||
            spell_info[spell].min_level[CLASS_SORCERER] < LVL_STAFF ||
            spell_info[spell].min_level[CLASS_BARD] < LVL_STAFF)
        {
          if (!(CLASS_LEVEL(ch, CLASS_WIZARD) > 0 ||
                CLASS_LEVEL(ch, CLASS_SORCERER) > 0 ||
                CLASS_LEVEL(ch, CLASS_BARD) > 0))
          {
            send_to_char(ch, "You must be able to use arcane magic to use this %s.\r\n",
                         GET_OBJ_TYPE(mag_item) == ITEM_WAND ? "wand" : "staff");
            return;
          }
        }
        else
        {
          if (!(CLASS_LEVEL(ch, CLASS_CLERIC) > 0 ||
                CLASS_LEVEL(ch, CLASS_DRUID) > 0 ||
                CLASS_LEVEL(ch, CLASS_INQUISITOR) > 0 ||
                CLASS_LEVEL(ch, CLASS_PALADIN) > 0 ||
                CLASS_LEVEL(ch, CLASS_RANGER) > 0))
          {
            send_to_char(ch, "You must be able to cast divine magic to use this %s.\r\n",
                         GET_OBJ_TYPE(mag_item) == ITEM_WAND ? "wand" : "staff");
            return;
          }
        }

        /* 1.b. Check the spell is on class spell list */
        if (!(((spell_info[spell].min_level[CLASS_WIZARD] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_WIZARD) > 0) ||
              ((spell_info[spell].min_level[CLASS_SORCERER] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_SORCERER) > 0) ||
              ((spell_info[spell].min_level[CLASS_BARD] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_BARD) > 0) ||
              ((MIN_SPELL_LVL(spell, CLASS_INQUISITOR, GET_1ST_DOMAIN(ch)) < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_INQUISITOR) > 0) ||
              ((MIN_SPELL_LVL(spell, CLASS_CLERIC, GET_1ST_DOMAIN(ch)) < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_CLERIC) > 0) ||
              ((MIN_SPELL_LVL(spell, CLASS_CLERIC, GET_2ND_DOMAIN(ch)) < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_CLERIC) > 0) ||
              ((spell_info[spell].min_level[CLASS_DRUID] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_DRUID) > 0) ||
              ((spell_info[spell].min_level[CLASS_PALADIN] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_PALADIN) > 0) ||
              ((spell_info[spell].min_level[CLASS_RANGER] < LVL_STAFF) && CLASS_LEVEL(ch, CLASS_RANGER) > 0)))
        {
          send_to_char(ch, "The spell stored in the %s is outside your realm of knowledge.\r\n",
                       GET_OBJ_TYPE(mag_item) == ITEM_WAND ? "wand" : "staff");
          return;
        }
      }

      break;

    default:
      send_to_char(ch, "Reached the end of switch for obj-type in 'use' command.\r\n");
      break;

    } /* end switch for obj-type */

    break; /* break for 'use' case above */
  }

  mag_objectmagic(ch, mag_item, buf);
}

/* Activate a magic item with a COMMAND WORD! */
ACMD(do_utter)
{
  int i = 0;
  int found = 0;
  struct obj_data *mag_item = NULL;

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Utter what?\r\n");
    return;
  }
  else
  {
    send_to_char(ch, "You utter '%s'.\r\n", argument);
    act("$n utters a command word, too quietly to hear.", TRUE, ch, 0, 0, TO_ROOM);
  }

  /* Check all worn/wielded items and see if they have a command word. */
  for (i = 0; i < NUM_WEARS; i++)
  {
    mag_item = GET_EQ(ch, i);
    if (mag_item != NULL)
    {
      switch (i)
      { /* Different procedures for weapons and armors. */
      case WEAR_WIELD_1:
      case WEAR_WIELD_OFFHAND:
      case WEAR_WIELD_2H:
      case WEAR_HANDS:
        found += process_weapon_abilities(mag_item, ch, NULL, ACTMTD_COMMAND_WORD, argument);
        break;
      default:
        break;
      }
    }
  }
  found += process_armor_abilities(ch, NULL, ACTMTD_COMMAND_WORD, argument);
  if (found == 0)
    send_to_char(ch, "Nothing happens.\r\n");
  else
    USE_STANDARD_ACTION(ch);
}

/* in order to handle issues with the prompt this became a necessary function */
bool is_prompt_empty(struct char_data *ch)
{
  bool prompt_is_empty = TRUE;

  if (IS_SET_AR(PRF_FLAGS(ch), PRF_DISPAUTO))
    prompt_is_empty = FALSE;
  if (IS_SET_AR(PRF_FLAGS(ch), PRF_DISPHP))
    prompt_is_empty = FALSE;
  if (IS_SET_AR(PRF_FLAGS(ch), PRF_DISPPSP))
    prompt_is_empty = FALSE;
  if (IS_SET_AR(PRF_FLAGS(ch), PRF_DISPMOVE))
    prompt_is_empty = FALSE;
  if (IS_SET_AR(PRF_FLAGS(ch), PRF_DISPEXP))
    prompt_is_empty = FALSE;
  if (IS_SET_AR(PRF_FLAGS(ch), PRF_DISPEXITS))
    prompt_is_empty = FALSE;
  if (IS_SET_AR(PRF_FLAGS(ch), PRF_DISPROOM))
    prompt_is_empty = FALSE;
  if (IS_SET_AR(PRF_FLAGS(ch), PRF_DISPMEMTIME))
    prompt_is_empty = FALSE;
  if (IS_SET_AR(PRF_FLAGS(ch), PRF_DISPACTIONS))
    prompt_is_empty = FALSE;

  return prompt_is_empty;
}

ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch))
  {
    send_to_char(ch, "Monsters don't need displays.  Go away.\r\n");
    return;
  }
  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Usage: prompt { { H | P | V | X | T | R | E | A } | all |"
                     " auto | none }\r\n");
    send_to_char(ch, "Notice this command is deprecated, we recommend using "
                     " PREFEDIT instead.\r\n");
    return;
  }

  if (!str_cmp(argument, "auto"))
  {
    TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);
    send_to_char(ch, "Auto prompt %sabled.\r\n", PRF_FLAGGED(ch, PRF_DISPAUTO) ? "en" : "dis");
    return;
  }

  if (!str_cmp(argument, "on") || !str_cmp(argument, "all"))
  {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);

    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPPSP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXITS);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPROOM);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMEMTIME);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPACTIONS);
  }
  else if (!str_cmp(argument, "off") || !str_cmp(argument, "none"))
  {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);

    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPPSP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXITS);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPROOM);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMEMTIME);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPACTIONS);
  }
  else
  {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);

    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPPSP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXITS);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPROOM);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMEMTIME);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPACTIONS);

    for (i = 0; i < strlen(argument); i++)
    {
      switch (LOWER(argument[i]))
      {
      case 'h':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
        break;
      case 'p':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPPSP);
        break;
      case 'v':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
        break;
      case 'x':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXP);
        break;
      case 't':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXITS);
        break;
      case 'r':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPROOM);
        break;
      case 'e':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMEMTIME);
        break;
      case 'a':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPACTIONS);
        break;
      default:
        send_to_char(ch, "Usage: prompt { { H | M | V | X | T | R | E | A } | all"
                         " | auto | none }\r\n");
        return;
      }
    }
  }

  send_to_char(ch, "%s", CONFIG_OK);
}

ACMD(do_gen_tog)
{
  long result;
  int i;
  char arg[MAX_INPUT_LENGTH];
  struct follow_type *j = NULL, *k = NULL;
  struct condensed_combat_data *combat_data = NULL;

  const char *const tog_messages[][2] = {
      /*0*/
      {"You are now safe from summoning by other players.\r\n",
       "You may now be summoned by other players.\r\n"},
      /*1*/
      {"Nohassle disabled.\r\n",
       "Nohassle enabled.\r\n"},
      /*2*/
      {"Brief mode off.\r\n",
       "Brief mode on.\r\n"},
      /*3*/
      {"Compact mode off.\r\n",
       "Compact mode on.\r\n"},
      /*4*/
      {"You can now hear tells.\r\n",
       "You are now deaf to tells.\r\n"},
      /*5*/
      {"You can now hear auctions.\r\n",
       "You are now deaf to auctions.\r\n"},
      /*6*/
      {"You can now hear shouts.\r\n",
       "You are now deaf to shouts.\r\n"},
      /*7*/
      {"You can now hear gossip.\r\n",
       "You are now deaf to gossip.\r\n"},
      /*8*/
      {"You can now hear the congratulation messages.\r\n",
       "You are now deaf to the congratulation messages.\r\n"},
      /*9*/
      {"You can now hear the Wiz-channel.\r\n",
       "You are now deaf to the Wiz-channel.\r\n"},
      /*10*/
      {"You are no longer part of the Quest.\r\n",
       "Okay, you are part of the Quest!\r\n"},
      /*11*/
      {"You will no longer see the room flags.\r\n",
       "You will now see the room flags.\r\n"},
      /*12*/
      {"You will now have your communication repeated.\r\n",
       "You will no longer have your communication repeated.\r\n"},
      /*13*/
      {"HolyLight mode off.\r\n",
       "HolyLight mode on.\r\n"},
      /*14*/
      {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
       "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
      /*15*/
      {"Autoexits disabled.\r\n",
       "Autoexits enabled.\r\n"},
      /*16*/
      {"Will no longer track through doors.\r\n",
       "Will now track through doors.\r\n"},
      /*17*/
      {"Will no longer clear screen in OLC.\r\n",
       "Will now clear screen in OLC.\r\n"},
      /*18*/
      {"Buildwalk Off.\r\n",
       "Buildwalk On.\r\n"},
      /*19*/
      {"AFK flag is now off.\r\n",
       "AFK flag is now on.\r\n"},
      /*20*/
      {"Autoloot disabled.\r\n",
       "Autoloot enabled.\r\n"},
      /*21*/
      {"Autogold disabled.\r\n",
       "Autogold enabled.\r\n"},
      /*22*/
      {"Autosplit disabled.\r\n",
       "Autosplit enabled.\r\n"},
      /*23*/
      {"Autosacrifice disabled.\r\n",
       "Autosacrifice enabled.\r\n"},
      /*24*/
      {"Autoassist disabled.\r\n",
       "Autoassist enabled.\r\n"},
      /*25*/
      {"Automap disabled.\r\n",
       "Automap enabled.\r\n"},
      /*26*/
      {"Autokey disabled.\r\n",
       "Autokey enabled.\r\n"},
      /*27*/
      {"Autodoor disabled.\r\n",
       "Autodoor enabled.\r\n"},
      /*28*/
      {"You are now able to see all clantalk.\r\n",
       "Clantalk channels disabled.\r\n"},
      /*29*/
      {"COLOR DISABLE\r\n",
       "COLOR ENABLE\r\n"},
      /*30*/
      {"SYSLOG DISABLE\r\n",
       "SYSLOG ENABLE\r\n"},
      /*31*/
      {"WIMPY DISABLE\r\n",
       "WIMPY ENABLE\r\n"},
      /*32*/
      {"PAGELENGTH DISABLE\r\n",
       "PAGELENGTH ENABLE\r\n"},
      /*33*/
      {"SCREENWIDTH DISABLE\r\n",
       "SCREENWIDTH DISABLE\r\n"},
      /*34*/
      {"Autoscan disabled.\r\n",
       "Autoscan enabled.\r\n"},
      /*35*/
      {"Autoreload disabled.\r\n",
       "Autoreload enabled.\r\n"},
      /*36*/
      {"CombatRoll disabled.\r\n",
       "CombatRoll enabled, you now will see details behind the combat rolls during combat.\r\n"},
      /*37*/
      {"GUI Mode disabled.\r\n",
       "GUI Mode enabled, make sure you have MSDP enabled in your client.\r\n"},
      /*38*/
      {"You will now see approximately every 5 minutes a in-game hint.\r\n",
       "You will no longer see in-game hints.\r\n"},
      /*39*/
      {"You will no longer automatically collect your ammo after combat.\r\n",
       "You will now automatically collect your ammo after combat.\r\n"},
      /*40*/
      {"You will no longer display to others that you would like to Role-play.\r\n",
       "You will now display to others that you would like to Role-play.\r\n"},
      /* 41 */
      {"Your bombs will now only affect single targets.\r\n",
       "Your bombs will now affect multiple targets.\r\n"},
      /*42*/
      {"You will no longer see level differences between you and mobs when you type look.\r\n",
       "You will now see level differences between you and mobs when you type look.\r\n"},
      /* 43 */
      {"You will no longer use smash defense in combat.\r\n",
       "You will now use smash defense in combat (if you know it).\r\n"},
      /* 44 */
      {"You will now allow charmies to rescue you and other group members.\r\n",
       "You will no longer allow charmies to rescue you and other group members\r\n"},
      /* 45 */
      {"You will now use the stored consumables system (HELP CONSUMABLES).\r\n",
       "You will no use the stock consumables system (HELP USE).\r\n"},
      /* 46 */
      {"You will no longer automatically stand if knocked down in combat.\r\n",
       "You will now automatically stand if knocked down in combat.\r\n"},
      /* 47 */
      {"You will no longer automatically hit mobs when typing 'hit' by itself.\r\n",
       "You will now automatically hit the first eligible mob in the room by typing 'hit' by itself.\r\n"},
      /*48*/
      {"Players can now follow you.\r\n",
       "Players can no longer follow you!\r\n"},
      /*49*/
      {"You will now see full combat details.\r\n",
       "You will now see condensed combat messages.\r\n"},

  };

  if (IS_NPC(ch))
    return;

  switch (subcmd)
  {
  case SCMD_USE_STORED_CONSUMABLES:
    result = PRF_TOG_CHK(ch, PRF_USE_STORED_CONSUMABLES);
    break;
  case SCMD_AUTO_STAND:
    result = PRF_TOG_CHK(ch, PRF_AUTO_STAND);
    break;
  case SCMD_AUTOHIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOHIT);
    break;

  case SCMD_CONDENSED:

    /* if we don't have a structure allocated, do it now */
    if (!CNDNSD(ch))
    {
      CREATE(combat_data, struct condensed_combat_data, 1);

      CNDNSD(ch) = combat_data;
    }

    /* initialize the combat data every time we hit this toggle */
    CNDNSD(ch)->num_times_attacking = 0;
    CNDNSD(ch)->num_times_hit_targets = 0;
    CNDNSD(ch)->num_times_hit_by_others = 0;
    CNDNSD(ch)->num_times_others_attack_you = 0;
    CNDNSD(ch)->num_targets_hit_by_your_spells = 0;
    CNDNSD(ch)->num_times_hit_by_spell = 0;

    /* go ahead toggle too! */
    result = PRF_TOG_CHK(ch, PRF_CONDENSED);

    break;

  case SCMD_NO_FOLLOW:
    /* this command on usage will drop all your PC followers -zusuk */
    for (k = ch->followers; k; k = j)
    {
      j = k->next;

      if (!IS_NPC(k->follower))
        stop_follower(k->follower);
    }

    result = PRF_TOG_CHK(ch, PRF_NO_FOLLOW);
    break;
  case SCMD_NOCHARMIERESCUES:
    result = PRF_TOG_CHK(ch, PRF_NO_CHARMIE_RESCUE);
    break;
  case SCMD_SMASH_DEFENSE:
    result = PRF_TOG_CHK(ch, PRF_SMASH_DEFENSE);
    break;
  case SCMD_AOE_BOMBS:
    result = PRF_TOG_CHK(ch, PRF_AOE_BOMBS);
    break;
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_GUI_MODE:
    result = PRF_TOG_CHK(ch, PRF_GUI_MODE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_NOSHOUT:
    result = PRF_TOG_CHK(ch, PRF_NOSHOUT);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOHINT:
    result = PRF_TOG_CHK(ch, PRF_NOHINT);
    break;
  case SCMD_AUTOCOLLECT:
    result = PRF_TOG_CHK(ch, PRF_AUTOCOLLECT);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_SHOWVNUMS:
    result = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_CLS:
    result = PRF_TOG_CHK(ch, PRF_CLS);
    break;
  case SCMD_BUILDWALK:
    if (GET_LEVEL(ch) < LVL_BUILDER)
    {
      send_to_char(ch, "Builders only, sorry.\r\n");
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    if (PRF_FLAGGED(ch, PRF_BUILDWALK))
    {
      one_argument(argument, arg, sizeof(arg));
      for (i = 0; *arg && *(sector_types[i]) != '\n'; i++)
        if (is_abbrev(arg, sector_types[i]))
          break;
      if (*(sector_types[i]) == '\n')
        i = 0;
      GET_BUILDWALK_SECTOR(ch) = i;
      send_to_char(ch, "Default sector type is %s\r\n", sector_types[i]);

      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    }
    else
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    if (PRF_FLAGGED(ch, PRF_AFK))
      act("$n has gone AFK.", TRUE, ch, 0, 0, TO_ROOM);
    else
    {
      act("$n has come back from AFK.", TRUE, ch, 0, 0, TO_ROOM);
      if (has_mail(GET_IDNUM(ch)))
        send_to_char(ch, "You have mail waiting.\r\n");
    }
    break;
  case SCMD_RP:
    result = PRF_TOG_CHK(ch, PRF_RP);
    if (PRF_FLAGGED(ch, PRF_RP))
      act("$n is interested in Role-play!.", TRUE, ch, 0, 0, TO_ROOM);
    else
    {
      act("$n is now OOC.", TRUE, ch, 0, 0, TO_ROOM);
    }
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTORELOAD:
    result = PRF_TOG_CHK(ch, PRF_AUTORELOAD);
    break;
  case SCMD_COMBATROLL:
    result = PRF_TOG_CHK(ch, PRF_COMBATROLL);
    break;
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  case SCMD_AUTOSAC:
    result = PRF_TOG_CHK(ch, PRF_AUTOSAC);
    break;
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  case SCMD_AUTOMAP:
    result = PRF_TOG_CHK(ch, PRF_AUTOMAP);
    break;
  case SCMD_AUTOKEY:
    result = PRF_TOG_CHK(ch, PRF_AUTOKEY);
    break;
  case SCMD_AUTODOOR:
    result = PRF_TOG_CHK(ch, PRF_AUTODOOR);
    break;
  case SCMD_NOCLANTALK:
    result = PRF_TOG_CHK(ch, PRF_NOCLANTALK);
    break;
  case SCMD_AUTOSCAN:
    result = PRF_TOG_CHK(ch, PRF_AUTOSCAN);
    break;
  case SCMD_AUTOCONSIDER:
    result = PRF_TOG_CHK(ch, PRF_AUTOCON);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
  else
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

  return;
}

/*a general diplomacy skill - popularity increase is determined by SCMD */
const struct diplomacy_data diplomacy_types[] = {
    {SCMD_MURMUR, SKILL_MURMUR, 0.75, 1},         /**< Murmur skill, 0.75% increase, 1 tick wait  */
    {SCMD_PROPAGANDA, SKILL_PROPAGANDA, 2.32, 3}, /**< Propaganda skill, 2.32% increase, 3 tick wait */
    {SCMD_LOBBY, SKILL_LOBBY, 6.0, 8},            /**< Lobby skill, 10% increase, 8 tick wait     */

    {0, 0, 0.0, 0} /**< This must be the last line */
};

ACMD(do_diplomacy)
{
  // need to make this do something Zusuk :P
}

void show_happyhour(struct char_data *ch)
{
  char happyexp[80], happygold[80], happyqp[80], happytreasure[80];
  int secs_left;

  if ((IS_HAPPYHOUR) || (GET_LEVEL(ch) >= LVL_GRSTAFF))
  {
    if (HAPPY_TIME)
      secs_left = ((HAPPY_TIME - 1) * SECS_PER_MUD_HOUR) + next_tick;
    else
      secs_left = 0;

    snprintf(happyqp, sizeof(happyqp), "%s+%d%%%s to Questpoints per quest\r\n", CCYEL(ch, C_NRM), HAPPY_QP, CCNRM(ch, C_NRM));
    snprintf(happygold, sizeof(happygold), "%s+%d%%%s to Gold gained per kill\r\n", CCYEL(ch, C_NRM), HAPPY_GOLD, CCNRM(ch, C_NRM));
    snprintf(happyexp, sizeof(happyexp), "%s+%d%%%s to Experience per kill\r\n", CCYEL(ch, C_NRM), HAPPY_EXP, CCNRM(ch, C_NRM));
    snprintf(happytreasure, sizeof(happytreasure), "%s+%d%%%s to Treasure Drop rate\r\n", CCYEL(ch, C_NRM), HAPPY_TREASURE, CCNRM(ch, C_NRM));

    send_to_char(ch, "LuminariMUD Happy Hour!\r\n"
                     "------------------\r\n"
                     "%s%s%s%sTime Remaining: %s%d%s hours %s%d%s mins %s%d%s secs\r\n",
                 (IS_HAPPYEXP || (GET_LEVEL(ch) >= LVL_STAFF)) ? happyexp : "",
                 (IS_HAPPYGOLD || (GET_LEVEL(ch) >= LVL_STAFF)) ? happygold : "",
                 (IS_HAPPYTREASURE || (GET_LEVEL(ch) >= LVL_STAFF)) ? happytreasure : "",
                 (IS_HAPPYQP || (GET_LEVEL(ch) >= LVL_STAFF)) ? happyqp : "",
                 CCYEL(ch, C_NRM), (secs_left / 3600), CCNRM(ch, C_NRM),
                 CCYEL(ch, C_NRM), (secs_left % 3600) / 60, CCNRM(ch, C_NRM),
                 CCYEL(ch, C_NRM), (secs_left % 60), CCNRM(ch, C_NRM));
  }
  else
  {
    send_to_char(ch, "Sorry, there is currently no happy hour!\r\n");
  }
}

/* set this up so we can just start a happy hour with a token */
void invoke_happyhour(struct char_data *ch)
{
  if (!ch)
    return;

  HAPPY_EXP = 100;
  HAPPY_GOLD = 50;
  HAPPY_QP = 50;
  HAPPY_TREASURE = 20;
  HAPPY_TIME = 48;

  game_info("A Happyhour has been started by %s!", GET_NAME(ch));

  return;
}

ACMD(do_happyhour)
{
  char arg[MAX_INPUT_LENGTH], val[MAX_INPUT_LENGTH];
  int num;

  if (GET_LEVEL(ch) < LVL_STAFF)
  {
    show_happyhour(ch);
    return;
  }

  /* Only Imms get here, so check args */
  two_arguments(argument, arg, sizeof(arg), val, sizeof(val));

  if (is_abbrev(arg, "experience"))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    HAPPY_EXP = num;
    send_to_char(ch, "Happy Hour Exp rate set to +%d%%\r\n", HAPPY_EXP);
  }
  else if (is_abbrev(arg, "treasure"))
  {
    num = MIN(MAX((atoi(val)), TREASURE_PERCENT + 1), 99 - TREASURE_PERCENT);
    HAPPY_TREASURE = num;
    send_to_char(ch, "Happy Hour Treasure drop-rate set to +%d%%\r\n",
                 HAPPY_TREASURE);
  }
  else if ((is_abbrev(arg, "gold")) || (is_abbrev(arg, "coins")))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    HAPPY_GOLD = num;
    send_to_char(ch, "Happy Hour Gold rate set to +%d%%\r\n", HAPPY_GOLD);
  }
  else if ((is_abbrev(arg, "time")) || (is_abbrev(arg, "ticks")))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    if (HAPPY_TIME && !num)
    {
      game_info("Happyhour has been stopped!");
      set_db_happy_hour(2);
    }
    else if (!HAPPY_TIME && num)
    {
      game_info("A Happyhour has started!");
      set_db_happy_hour(1);
    }

    HAPPY_TIME = num;
    send_to_char(ch, "Happy Hour Time set to %d ticks (%d hours %d mins and %d secs)\r\n",
                 HAPPY_TIME,
                 (HAPPY_TIME * SECS_PER_MUD_HOUR) / 3600,
                 ((HAPPY_TIME * SECS_PER_MUD_HOUR) % 3600) / 60,
                 (HAPPY_TIME * SECS_PER_MUD_HOUR) % 60);
  }
  else if ((is_abbrev(arg, "qp")) || (is_abbrev(arg, "questpoints")))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    HAPPY_QP = num;
    send_to_char(ch, "Happy Hour Questpoints rate set to +%d%%\r\n", HAPPY_QP);
  }
  else if (is_abbrev(arg, "show"))
  {
    show_happyhour(ch);
  }
  else if (is_abbrev(arg, "default"))
  {
    HAPPY_EXP = 100;
    HAPPY_GOLD = 50;
    HAPPY_QP = 50;
    HAPPY_TREASURE = 20;
    HAPPY_TIME = 48;
    game_info("A Happyhour has started!");
    set_db_happy_hour(1);
  }
  else
  {
    send_to_char(ch,
                 "Usage: %shappyhour                 %s- show usage (this info)\r\n"
                 "       %shappyhour show            %s- display current settings (what mortals see)\r\n"
                 "       %shappyhour time <ticks>    %s- set happyhour time and start timer\r\n"
                 "       %shappyhour qp <num>        %s- set qp percentage gain\r\n"
                 "       %shappyhour exp <num>       %s- set exp percentage gain\r\n"
                 "       %shappyhour gold <num>      %s- set gold percentage gain\r\n"
                 "       %shappyhour treasure <num>  %s- set treasure drop-rate gain\r\n"
                 "       \tyhappyhour default      \tw- sets a default setting for happyhour\r\n\r\n"
                 "Configure the happyhour settings and start a happyhour.\r\n"
                 "Currently 1 hour IRL = %d ticks\r\n"
                 "If no number is specified, 0 (off) is assumed.\r\nThe command \tyhappyhour time\tn will therefore stop the happyhour timer.\r\n",
                 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                 (3600 / SECS_PER_MUD_HOUR));
  }
}

/****  little hint system *******/
/* i am surrounding hints with this:
   \tR[HINT]:\tn \ty
   [use nohint or prefedit to deactivate this]\tn\r\n
 */

static const char *const hints[] = {
    /* 1*/ "\tR[HINT]:\tn \ty"
           "Different spell casting classes use different commands "
           "to cast their spells.  Typing score will show you the appropriate "
           "commands.  For helpful information see: HELP CAST and HELP MEMORIZE.  "
           "You will also need to PREPARE spells either after or before using them."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /* 2*/ "\tR[HINT]:\tn \ty"
           "You will gain more experience from grouping.  Monster "
           "experience is increased for groups, with larger groups receiving "
           "more total experience.  This total experience is divided amongst "
           "all members based on their level.  "
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /* 3*/ "\tR[HINT]:\tn \ty"
           "To view vital information about your character, type SCORE.  "
           "Additional helpful information commands include: ATTACKS, DEFENSES, "
           "AFF, COOLDOWN, ABILITIES and RESISTANCE."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /* 4*/ "\tR[HINT]:\tn \ty"
           "Once you have enough experience to advance, you can type GAIN "
           "<class choice> to level up, you can gain in any class you qualify for, up "
           "to 3 total classes.  Leveling up will result in an increase in "
           "training points for skills, more hit points, and possibly more feat points, "
           "movement points, boosts, PSP points.  To spend your feat points, you will "
           "use the STUDY menu.  To spend boosts and skill points, you will use a trainer.  "
           "If you make a mistake in choices when you advance, you can always RESPEC."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /* 5*/ "\tR[HINT]:\tn \ty"
           "The combination of races, classes (MULTICLASS), stat boosts "
           "and feat selection make for nearly limitless choices.  Do not be afraid "
           "to experiment!  Have fun reading up on feats using commands.  FEATS with no "
           "argument will show your current feats and show you a bunch of other command "
           "options on the bottom section."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /* 6*/ "\tR[HINT]:\tn \ty"
           "Worried about missing messages while in a menu system such "
           "as STUDY?  Do not fret, we have a HISTORY command so you can view "
           "what was said on all the channels while you were away!  You can "
           "type HISTORY ALL to view everything!"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /* 7*/ "\tR[HINT]:\tn \ty"
           "QUEST PROGRESS and QUEST HISTORY are your best friends when "
           "working on a quest, they can help remind you exactly what you have done "
           "and what you need to do.  You can also view details of each individual quest "
           "you have completed by typing QUEST HISTORY <number>.  Some quests are "
           "initiated with the ASK command, commonly ASK HI will initiate conversation."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /* 8*/ "\tR[HINT]:\tn \ty"
           "If you have an idea for a hint, please let us know on the Discord Server: "
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /* 9*/ "\tR[HINT]:\tn \ty"
           "LuminariMUD is considered a 'younger' MUD and is under heavy "
           "and constant development.  If you run into a bug, our policy is to simply "
           "report the bug using the BUG command.  Example, BUG SUBMIT <header>, then "
           "use the in-game text editor that pops up to write details about the bug, then "
           "you can type /s to save the submission."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*10*/ "\tR[HINT]:\tn \tyThe beginning of your adventure will be focused in on the Ashenport "
           "Region of the world.  The region is relatively fairly small, maybe 1/10th of the "
           "surface space of Lumia, yet expands thousands of rooms in our WILDERNESS.  At the "
           "current time you are able to get between zones quickly using a TELEPORTER.  To use "
           "the teleporter, just type: TELEPORT <target>.  To get a list of target zones, you can "
           "type: HELP ZONES."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*11*/ "\tR[HINT]:\tn \tyYou can find a lot of gear in various shops throughout the "
           "world.  Some shops even include special powerful magic items that can "
           "be purchased with quest points.  In addition to finding normal loot "
           "that are on your foes, you may also find 'random treasure' on them "
           "as well with every victory, there is a slight chance you will find "
           "this bonus loot.  Also, there is a special BAZAAR in Ashenport where "
           "you can purchase magic items to fill your missing equipment slots."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*12*/ "\tR[HINT]:\tn \tyIf you find items of value that you do not want to deal with, "
           "you can always DONATE them.  This will cause them to appear in a "
           "donation pit throughout the realm.  Alternatively if you find an item"
           "that is of no value, you can JUNK it to permanently get rid of it.  "
           "Although not necessary since corpses decompose after a short time, you "
           "can SACrifice corpses to get rid of them."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*13*/ "\tR[HINT]:\tn \tyBe aware during combat!  Enemies can do various things "
           "to try and disable you, including knocking you down! You can check AFF "
           "to see affections both negative and positive that are affecting you."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*14*/ "\tR[HINT]:\tn \tyLuminariMUD has a 'main' quest line for the Ashenport Region "
           "that starts with the Mosswood Elder, in Mossswood.  The quest line is "
           "important for telling fun and informative details about the lore of the "
           "Realm, and leading the adventurers to various zones throughout the region. "
           "In addition some of the rewards of the various quests in the chain have "
           "very powerful items."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*15*/ "\tR[HINT]:\tn \ty"
           "Have a great idea for features or changes to LuminariMUD?  Please "
           "use the IDEA command, IDEA SUBMIT <title of submission>, you will then "
           "enter our text editor, type out the details of the idea, and then "
           "type /s to save the submission."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*16*/ "\tR[HINT]:\tn \ty"
           "We have a lot of end-game content, including building a player treasure "
           "horde (HELP HOUSE), which allows you to create a corner of the world "
           "to call home and store your treasure.  You can add 'guests' to your home "
           "to allow your friends or other characters to enter."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*17*/ "\tR[HINT]:\tn \ty"
           "Reached level 30?  There are a lot of exciting end-game content here "
           "at LuminariMUD.  We have multiple planes of existence that can be accessed "
           "through various portals throughout the wilderness and magic.  Epic zones "
           "with extremely challenging group content with powerful magical items.  "
           "Hard to find epic quests litter the Realm as well, so explore!"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*18*/ "\tR[HINT]:\tn \ty"
           "LuminariMUD has an account system.  Through the account system you "
           "can manage multiple characters.  In addition you share 'account exp' "
           "between your characters, which can be spent on various account-wide "
           "benefits, such as unlocking advanced or epic races, and prestige classes. "
           "Type ACCOUNT to view your account details and ACCEXP to unlock various "
           "benefits spending account experience."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*19*/ "\tR[HINT]:\tn \ty"
           "Did you know that some weapons and armor have very special enchantments "
           "on them that can offer a variety of benefits?  Some of the more powerful "
           "gear in the Realm can cast their own spells, and act on their own.  "
           "Some will benefit your feats, stats, imbue you with affections while "
           "equipped.  Some gear responds to special command words that can be "
           "discovered either by examining the gear, through quest lines or other "
           "interesting ways.  Some gear respond to combat maneuvers, such as weapons "
           "that will help you trip, spikes on a shield when shield-punching, or even "
           "armor / shields that will cast a heal spell on you when they deflect a blow."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*20*/ "\tR[HINT]:\tn \ty"
           "LuminariMUD's crafting system allows you to CREATE, RESTRING (rename), "
           "RESIZE gear.  You will need a crafting kit and respective molds.  If you "
           "travel to Sanctus, there is a work area for buying molds.  If you add a "
           "crafting crystal while creating a new item, you will enchant it.  You "
           "can acquire crystals from treasure, DISENCHANTing magic items and you can "
           "even use AUGMENT to combine crystals to make them more powerful.  In addition "
           "you can do SUPPLYORDERs (basic crafting quests) in Sanctus for rewards "
           "including quest points. "
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*21*/ "\tR[HINT]:\tn \ty"
           "Reached the end-game?  Forming a CLAN can help you co-ordinate "
           "team efforts for some serious carnage.  Having a well co-ordinated "
           "team will allow you to take on the epic foes that require large teams "
           "to defeat.  In addition, clans can capture 'zones' to charge taxes on "
           "people that hunt in the zone.  The dynamic can result in clan-wars!  "
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*22*/ "\tR[HINT]:\tn \ty"
           "Overwhelmed by all the class and feat choices?  We started a community "
           "built thread on the FORUM.  We then transfer those submissions to "
           "help files under the heading HELP CLASS-BUILD.  The forum link is:  "
           "http://www.luminarimud.com/forums/topic/class-builds/"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*23*/ "\tR[HINT]:\tn \ty"
           "Help files are critical!  We try our best to anticipate all the subjects "
           "that are needed, but we rely heavily on contributions from players - "
           "with emphasis on new ones.  Please take the time to post it on the forum at "
           "http://www.luminarimud.com/forums/topic/help-files/ you can also "
           "post it as an IDEA in-game.  You can also help the staff workload by writing "
           "helpfiles via the forum!"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*24*/ "\tR[HINT]:\tn \ty"
           "LuminariMUD has a forum at: https://www.luminarimud.com/forums/ the public "
           "registration may be closed due to beloved spam-bots, but any staff can make "
           "you an account manually, just contact us via in-game mail, or email: "
           "Zusuk@@LuminariMUD.com or Ornir@@LuminariMUD.com"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*25*/ "\tR[HINT]:\tn \ty"
           "Voting keeps new players coming! (may require creating an account):\r\n"
           "http://www.topmudsites.com/vote-luminarimud.html \r\n"
           "http://www.mudconnect.com/cgi-bin/vote_rank.cgi?mud=LuminariMUD \r\n"
           "Also the MUD community on Reddit is a great way to promot us:\r\n"
           "https://www.reddit.com/r/MUD/ \r\n"
           " [use nohint or prefedit to deactivate this]\tn\r\n",
    /*26*/ "\tR[HINT]:\tn \ty"
           "Lore, story-telling, immersion...  Critical elements of a text based "
           "world.  View our background story here including an audio version!: "
           "https://www.luminarimud.com/lumina-voiced-stu-cook/"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*27*/ "\tR[HINT]:\tn \ty"
           "Come visit the LuminariMUD website: https://www.luminarimud.com/ for "
           "our forums, lore entries, related links, articles, updates.. the works!"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*28*/ "\tR[HINT]:\tn \ty"
           "Our Facebook page: https://www.facebook.com/LuminariMud/ \r\n"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*29*/ "\tR[HINT]:\tn \ty"
           "Archery is a great way to rain death from afar!  Archery automatically "
           "gets a bonus attack, and has a huge variety of feats to put her on par "
           "with melee attacks.  To initiate combat with a ranged weapon use the "
           "FIRE command.  To gather your ammo and make sure it does not get mixed "
           "up with other archers, just type COLLECT.  You can even toggle AUTOCOLLECT "
           "to make sure you automatically collect your ammo after each battle."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*30*/ "\tR[HINT]:\tn \ty"
           "Are you using our Mudlet GUI (http://www.luminarimud.com/forums/topic/official-luminari-gui/)?  "
           "If so, you may want to view our help file: HELP GUI-MAP to get an idea how to use "
           "the mapper properly!"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*31*/ "\tR[HINT]:\tn \ty"
           "Spot a typo?  Just use the command: TYPO SUBMIT <title> to enter our "
           "text editor.  From there you can type out what you found, then type /s to "
           "save your submission.  This creates a record for the staff to work off of!"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*32*/ "\tR[HINT]:\tn \ty"
           "Crafting is a critical and important part of the game, we have help files "
           "such as HELP CRAFTING - but we also have a great guide you can "
           "view on our forums: http://www.luminarimud.com/forums/topic/crafting-101-2018/"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*33*/ "\tR[HINT]:\tn \ty"
           "You may notice MOBILEs with an (!) exclamation point in front of them.  If the "
           "(!) is yellow, then this is an AUTOQUEST quest master, and you can type 'quest list' to "
           "view his/her quests.  If the (!) is red, then this is a HLQUEST mobile and you "
           "can try asking the mobile 'hi' or other keywords based on clues whereas you may "
           "also then get directions for a special quest."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*34*/ "\tR[HINT]:\tn \ty"
           "Stores within the realm will allow you to buy pets, mounts and hire mercenaries.  "
           "HELP PET, HELP CHARMEE, HELP ORDER for more info.  "
           "Once hired, they are your obedient servants.  As such you will be able to order them "
           "to do as you wish.  They will also automatically assist you in combat to the best of their "
           "abilties (determined by their level, race and class).  If they are groupped, you will get "
           "full credit for their kills as well.  "
           "(group new, order followers group join <your name>)"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*35*/ "\tR[HINT]:\tn \ty"
           "Got coin to spare and ready to build up a source of steady reliable income?  Set up a shop "
           "to the rest of the players!  HELP PLAYER-SHOP"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*36*/ "\tR[HINT]:\tn \ty"
           "Starting cities have convenient locations to buy supplies, equipment and even house "
           "donation rooms.  In Mosswood, from the elder: north, east to the armor shop; directly "
           "east for the general shop; south, west for the donation pit.  There are plenty more shops "
           "to be found throughout the realms."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*37*/ "\tR[HINT]:\tn \ty"
           "Helpful commands for finding secrets/hidden aspects of a zone include: 'look around' - "
           "this will help identify oustanding visual descriptions in the room you are in, 2) 'push' "
           "or 'pull' - these are common keywords for interacting with objects in a room that "
           "activate secrets, 3) 'enter' - sometimes hidden portals/entrances are in weird places, "
           "you can find them with this command, 4) and finally bare in mind not all zones will map "
           "out onto a 2-dimensional map."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*38*/ "\tR[HINT]:\tn \ty"
           "Want to know your Actions from your Maneuvers ? Don 't worry combat in Luminari can be deep "
           "and complex, but that' s part of what makes it so much fun.Make sure you check out HELP ACTIONS, "
           "COMBAT and QUEUE to become a combat pro!"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*39*/ "\tR[HINT]:\tn \ty"
           "To help navigate the massively expansiave surface wilderness, there is a CARRIAGE system "
           "that can quickly and safely get you between major locations in the wilderness (help carriage).  "
           "There are also plenty of zone connections in the wilderness that you can not reach via carriages, "
           "but a strategy is to use the SURVEY (help survey) command to identify the locations and use the "
           "carriage system to try to get near to your destination then hike the rest."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*40*/ "\tR[HINT]:\tn \ty"
           "'ENCOUNTERS' is a system whereby parties and individuals exploring or roaming the wilderness "
           "will randomly encounter level-appropriate mobs to fight.  Encounter mob types are pulled from "
           "an ever-growing list that currently numbers over 100+. Encounters can be dealt with in a number "
           "of ways, such as defeating it in combat, escaping, intimidating, bluffing or using diplomacy, "
           "and even bribing in some cases.  In addition to combat encounters, we will soon be adding other "
           "types of encounters, such as treasure caches, wandering vendors that sell special items, "
           "mysterious entities that grant your character bonuses, and more.  (help encounters)"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*41*/ "\tR[HINT]:\tn \ty"
           "'HUNTS' is a system that allows for individuals or parties to search out special boss "
           "mobs in the wilderness of Lumia.  These hunt targets, when defeated, award the entire party "
           "quest points, gold and some nice experience.  In addition to the party-wide rewards, each "
           "hunt mob also drops hunt trophies of various types: some are enhanced crafting materials, "
           "others can be exchanged for rings, pendants or bracers that give special affects when worn "
           "(eg. infravision, detect invis, haste, etc.), as well as weapon oils that can be applied to "
           "a weapon to give it a permanent special affect (eg. flaming, defending, vorpal, vampiric, "
           "etc.).  (help hunts)"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*42*/ "\tR[HINT]:\tn \ty"
           "Insidious rumors have begun that Caltursar the Dead Walker, a necrophant that resides in the "
           "Skull Gorge has discovered the dark art of becoming a LICH.  Perhaps one could ASK him about "
           "'undeath' to reveal what sort of necormantic madness he has tapped into.  (note: the quest "
           "line to become a lich is extremely difficult and designed for elite groups to conquer)"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*43*/ "\tR[HINT]:\tn \ty"
           "We have a MISSION system that allows players to take on bounties on targets througout the "
           "realm based on your level and specified difficulty.  Missions produce general rewards "
           "including experience, gold, quest points and random treasure.  (help missions)"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*44*/ "\tR[HINT]:\tn \ty"
           "Curious what our rules of conduct are for the game?  Type POLICY to view the rules!  Generally "
           "speaking we are not strict at all about enforcing rules outside of harassing your fellow players "
           "(AKA the golden rule - do unto others as you would have done to yourself and do not do unto others "
           "that which you would not want done to you) and "
           "abusing bugs before reporting them (if you report a bug then abuse it, we are generally not strict "
           "about that since it puts pressure on the developres to fix critical bugs quickly)."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*45*/ "\tR[HINT]:\tn \ty"
           "Although our normal POLICY does not allow multi-playing, due to a need for thorough game testing "
           "and limited player base, we -DO- CURRENTLY -PERMIT- multi-playing up to 4 characters at the same time.  "
           "You can also create as many storage-characters as you like to dump all your treasure and crafting "
           "resources.  "
           "We only ask that you please take the time and effort to report bugs/issues you find to us in return!"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*46*/ "\tR[HINT]:\tn \ty"
           "The henchmen, mercenaries, mounts and PETs that are spread throughout the realms not only can be "
           "hired with gold, but some of them can be acquired through mini quest lines.  A henchment guild has "
           "been opened in Ashenport's Jade Jug Inn to help adventurers."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*47*/ "\tR[HINT]:\tn \ty"
           "Per our POLICY entry, we do not normally allow BOTS.  The exception is automating tasks but being present "
           "(at keys - the MUDding app should be visible and attended to the player).  An example would be creating an "
           "automated hunting route for experience, etc where your scripts (in MUDLET for example) would do all the work "
           "for you.  As long as your MUDLET window is visible and you are at your computer, this is completely acceptable.  "
           "Further, currently and until a notice of a POLICY change, you are allowed to fully (even unattended) BOT AUTOCRAFTing.  "
           "This policy is in effect to help test the crafting system and can be changed at any time."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*48*/ "\tR[HINT]:\tn \ty"
           "SUPPLYORDERs / AUTOCRAFTing is the safest and easiest way to increase your crafting skills!"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*49*/ "\tR[HINT]:\tn \ty"
           "You can use the ARMORLIST command to view all the armor types in the realms and then ARMORINFO <name of armor type> "
           "to view details of each armor type.  The same goes for weapons via the WEAPONLIST and WEAPONINFO commands."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*50*/ "\tR[HINT]:\tn \ty"
           "More than a dozen zones form the outter planes of existence!  Visit our website to view the connections between the "
           "planes...  https://luminarimud.com/planes-of-existence/"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*51*/ "\tR[HINT]:\tn \ty"
           "Approximately 20 zones form the Underworld or Underdark!  Visit our website to view a rough map..."
           "  https://luminarimud.com/the-underdark/"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*52*/ "\tR[HINT]:\tn \ty"
           "There are 'dot' 'dash' and 'all' commands to help you manipulate targets.  Examples include:  "
           "To get all the 'bread' items from all the bags in your inventory you'd type 'take all.bread all.bag' | "
           "To loot the 2nd corpse on the ground you'd type 'take all 2.corpse' | "
           "To attack the blue dragon instead of the white dragon you'd type 'kill blue-dragon'"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*53*/ "\tR[HINT]:\tn \ty"
           "Want to see a full list of all implement spells and skills in the game?  Type respectively 'masterlist spells' or 'masterlist skills'"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*54*/ "\tR[HINT]:\tn \ty"
           "There is a shop that offers a service that will elighten your mind as to the basic "
           "enchantments found on all of your worn equipment.  It is one west of 'recall.'  The "
           "cost varies by the skill of the requester, beginning at 100 gold coins for a complete "
           "novice and up to 3000 gold coins for the most skilled of heroes.  To receive this "
           "enlightenment, simply use (type) the word: \tReqstats\ty"
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*55*/ "\tR[HINT]:\tn \ty"
           "There is a player-owned shop in Sanctus, where you can buy very powerful end-game gear."
           "From the Center of Sanctus, head 3 south, 1 east, then 1 south.  "
           "You will find 'A Shopper's Paradise' owned by Melaw and ran by Onat."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*56*/ "\tR[HINT]:\tn \ty"
           "There is a player-owned shop in Sanctus, where you can buy some different gear.  "
           "From the Center of Sanctus, head 3 south, 1 east, then 1 north.  You will find "
           "'Weaver and Fagn's' owned by Ellyanor and ran by Kyrt."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*57*/ "\tR[HINT]:\tn \ty"
           "There is a player-owned shop in Sanctus, where you can buy some different gear."
           "From the Center of Sanctus, head 3 south, 1 west, then 1 north.  You will find "
           "'The Kobold's Den' owned by Thimblethorp and ran by Ickthak."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",
    /*58*/ "\tR[HINT]:\tn \ty"
           "There is a player-owned shop in Ashenport, where you can buy very powerful end-game gear."
           "From Inside the Northern Gates of Ashenport, head 1 south, 1 west, then 1 north.  You will "
           "find 'Brondo's Bar and Grill' owned by Brondo and ran by The Towering Woman."
           "  [use nohint or prefedit to deactivate this]\tn\r\n",

};

static const size_t NUM_HINTS = sizeof(hints) / sizeof(hints[0]);

void show_hints(void)
{
  int roll = dice(1, NUM_HINTS) - 1;
  struct char_data *ch = NULL, *next_char = NULL;

  for (ch = character_list; ch; ch = next_char)
  {
    next_char = ch->next;

    if (IS_NPC(ch) || !ch->desc)
      continue;

    if (PRF_FLAGGED(ch, PRF_NOHINT))
      continue;

    /* player is in a menu */
    if (PLR_FLAGGED(ch, PLR_WRITING))
      continue;

    send_to_char(ch, hints[roll]);
  }
}

/* moved to do_gen_tog */
/*
ACMDU(do_nohints) {
  if (!PRF_FLAGGED(ch, PRF_NOHINTS)) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_NOHINTS);
    send_to_char(ch, "You will no longer see game hints.\r\n");
  } else {
    send_to_char(ch, "You will now see game hints every so often.\r\n");
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_NOHINTS);
  }
}
 */

/********** end hint system ********/

/* todo list system, from ParagonMUD */
void display_todo(struct char_data *ch, struct char_data *vict)
{
  int i;
  struct txt_block *tmp;

  for (tmp = GET_TODO(vict), i = 1; tmp; tmp = tmp->next, i++)
    send_to_char(ch, "%d) %s\r\n%s", i, tmp->text, (tmp->next ? "\r\n" : ""));
}

/* todo list system, from ParagonMUD, ported by Zusuk */
ACMD(do_todo)
{
  struct txt_block *tmp;

  skip_spaces_c(&argument);

  if (!*argument)
  {
    if (!GET_TODO(ch))
      send_to_char(ch, "You have nothing to do!\r\n");
    else
      display_todo(ch, ch);
    return;
  }
  if (!isdigit(*argument))
  {

    while (*argument == '~')
      argument++;

    if (!(tmp = GET_TODO(ch)))
    {
      CREATE(GET_TODO(ch), struct txt_block, 1);
      GET_TODO(ch)->text = strdup(argument);
    }
    else
    {
      while (tmp->next)
        tmp = tmp->next;
      CREATE(tmp->next, struct txt_block, 1);
      tmp->next->text = strdup(argument);
    }
    send_to_char(ch, "Great, another thing to do!\r\n");

    save_char(ch, 0);
    Crash_crashsave(ch);
  }
  else
  {
    int num, i, success = 0;
    sscanf(argument, "%d", &num);

    if (num == 1 && GET_TODO(ch) && (success = 1))
      GET_TODO(ch) = GET_TODO(ch)->next;
    else
      for (tmp = GET_TODO(ch), i = 1; tmp && tmp->next; tmp = tmp->next, i++)
        if (i + 1 == num)
        {
          tmp->next = tmp->next->next;
          success = 1;
          break;
        }
    if (success)
      send_to_char(ch, "Phew!  One less thing to do!\r\n");
    else
      send_to_char(ch, "No such item exists in your todo list!\r\n");

    save_char(ch, 0);
    Crash_crashsave(ch);
  }
}

ACMD(do_dice)
{
  char Gbuf1[MAX_STRING_LENGTH];
  char Gbuf2[MAX_STRING_LENGTH];
  int rolls, size, result;

  if (!*argument)
  {
    send_to_char(ch, "You need to specify the dice size!\r\n  ex: 'dice 1 8'  - one roll with an eight sided die.\n");
    return;
  }

  two_arguments(argument, Gbuf1, sizeof(Gbuf1), Gbuf2, sizeof(Gbuf2));

  if (is_number(Gbuf1))
  {
    rolls = atoi(Gbuf1);
    if (rolls < 1 || rolls > 10000)
    {
      send_to_char(ch, "Sorry bub, the first parameter is out of range.\r\n");
      return;
    }
  }
  else
  {
    send_to_char(ch, "The first parameter needs to be a number. Try again.\r\n");
    return;
  }

  if (!*Gbuf2)
  {
    send_to_char(ch, "You need to specify the dice size as the second parameter.\n");
    return;
  }

  one_argument(Gbuf2, Gbuf1, sizeof(Gbuf1));

  if (is_number(Gbuf1))
  {
    size = atoi(Gbuf1);
    if (size < 1 || size > 10000)
    {
      send_to_char(ch, "Sorry bub, the second number is out of range.\n");
      return;
    }
  }
  else
  {
    send_to_char(ch, "The second parameter needs to be a number. Try again.\n");
    return;
  }

  result = dice(rolls, size);

  sprintf(Gbuf1, "You roll a %d sided dice %d times, the total result is: \tB%d\tn\r\n", size, rolls, result);
  send_to_char(ch, Gbuf1);

  sprintf(Gbuf1, "A %d sided dice is rolled by %s %d times, the total result is: \tB%d\tn\r\n", size, GET_NAME(ch), rolls, result);
  send_to_room(ch->in_room, Gbuf1);

  return;
}

ACMD(do_summon)
{
  struct char_data *tch = NULL;
  bool found = false;

  for (tch = character_list; tch; tch = tch->next)
  {
    if (tch == ch)
      continue;
    if (!IS_NPC(tch))
      continue;
    if (!AFF_FLAGGED(tch, AFF_CHARM))
      continue;
    if (tch->master != ch)
      continue;
    if (IN_ROOM(tch) == NOWHERE)
      continue;
    if (IN_ROOM(tch) == IN_ROOM(ch))
      continue;
    act("$n disappears in a flash of light.", FALSE, tch, 0, 0, TO_ROOM);
    char_from_room(tch);
    char_to_room(tch, IN_ROOM(ch));
    act("$n appears in a flash of light.", FALSE, tch, 0, 0, TO_ROOM);
    found = true;
  }

  if (!found)
  {
    send_to_char(ch, "You do not have any charmies that require summoning.\r\n");
  }
}

/* some cleanup */
#undef NUM_HINTS
#undef MODE_CLASSLIST_NORMAL
#undef MODE_CLASSLIST_RESPEC
#undef MULTICAP
#undef WILDSHAPE_AFFECTS
#undef TOG_OFF
#undef TOG_ON

ACMDU(do_revoke)
{
  int spellnum = 0;

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Please specify which affect you'd like to revoke. (See the affects command for the affect name)\r\n");
    return;
  }

  spellnum = find_skill_num(argument);

  if (can_spell_be_revoked(spellnum))
  {
    send_to_char(ch, "You revoke the affect '%s' from your person.\r\n", spell_info[spellnum].name);
    affect_from_char(ch, spellnum);
  }
  else
  {
    send_to_char(ch, "Either that is not a valid affect name or that affect cannot be revoked.\r\n");
  }
}

ACMDU(do_holyweapon)
{

  int i = 0;

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Please specify a weapon type you'd like to use with your %sholy weapon spell.\r\n", subcmd ? "un" : "");
    send_to_char(ch, "Your current %sholy weapon type is '%s'.\r\n", subcmd ? "un" : "", weapon_list[GET_HOLY_WEAPON_TYPE(ch)].name);
    return;
  }

  for (i = 0; i < NUM_WEAPON_TYPES; i++)
  {
    if (is_abbrev(argument, weapon_list[i].name))
      break;
  }

  if (i >= NUM_WEAPON_TYPES)
  {
    send_to_char(ch, "That is not a valid weapon type.\r\n");
    return;
  }

  /* going to check bogus weapon types */
  switch (i)
  {
  case WEAPON_TYPE_UNARMED:
  case WEAPON_TYPE_UNDEFINED:
    send_to_char(ch, "Sorry, that is not a valid weapon type.\r\n");
    return;
  }

  GET_HOLY_WEAPON_TYPE(ch) = i;
  send_to_char(ch, "You have set your %sholy weapon type to %s.\r\n", subcmd ? "un" : "", weapon_list[GET_HOLY_WEAPON_TYPE(ch)].name);
  save_char(ch, 0);
}

int total_fiendish_boon_levels(struct char_data *ch)
{
  int num = CLASS_LEVEL(ch, CLASS_BLACKGUARD) / 4;
  if (CLASS_LEVEL(ch, CLASS_BLACKGUARD) >= 30)
    num++;
  return num;
}

int active_fiendish_boon_levels(struct char_data *ch)
{
  int i = 0, num = 0;

  for (i = 0; i < NUM_FIENDISH_BOONS; i++)
  {

    if (FIENDISH_BOON_ACTIVE(ch, i))
      num += fiendish_boon_slots[i];
  }
  return num;
}

ACMDU(do_fiendishboon)
{
  if (!HAS_FEAT(ch, FEAT_FIENDISH_BOON))
  {
    send_to_char(ch, "You do not benefit from fiendish boons.\r\n");
    return;
  }

  int i = 0, j = 0;
  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Fiendish Boons Available:\r\n");
    for (j = 0; j < 80; j++)
      send_to_char(ch, "-");
    send_to_char(ch, "\r\n");
    for (i = 1; i < NUM_FIENDISH_BOONS; i++)
    {
      if (CLASS_LEVEL(ch, CLASS_BLACKGUARD) < fiendish_boon_levels[i])
        continue;
      send_to_char(ch, "-%d slots- [", fiendish_boon_slots[i]);
      if (FIENDISH_BOON_ACTIVE(ch, i))
      {
        send_to_char(ch, "\tg%-8s\tn", "ACTIVE");
      }
      else
      {
        send_to_char(ch, "\tr%-8s\tn", "INACTIVE");
      }
      send_to_char(ch, "] %-15s : %s\r\n", fiendish_boons[i], fiendish_boon_descriptions[i]);
    }
    for (j = 0; j < 80; j++)
      send_to_char(ch, "-");
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Type 'fiendishboon (boon type) to toggle on and off.\r\n");
    send_to_char(ch, "Total slots available: %d, Total slots used: %d.\r\n", total_fiendish_boon_levels(ch), active_fiendish_boon_levels(ch));
    return;
  }

  for (i = 1; i < NUM_FIENDISH_BOONS; i++)
  {
    if (is_abbrev(argument, fiendish_boons[i]))
      break;
  }

  if (i < 1 || i >= NUM_FIENDISH_BOONS)
  {
    send_to_char(ch, "That is not a valid fiendish boon. Please type 'fiendishboons' to see a list.\r\n");
    return;
  }

  if (FIENDISH_BOON_ACTIVE(ch, i))
  {
    REMOVE_FIENDISH_BOON(ch, i);
    send_to_char(ch, "You deactivate your '%s' fiendish boon.\r\n", fiendish_boons[i]);
    return;
  }

  if ((active_fiendish_boon_levels(ch) + fiendish_boon_slots[i]) > total_fiendish_boon_levels(ch))
  {
    send_to_char(ch, "You do not have enough fiendish boon slots available.  Type 'fiendishboons' for a breakdown.\r\n");
    return;
  }

  SET_FIENDISH_BOON(ch, i);
  send_to_char(ch, "You have activated your '%s' fiendish boon.\r\n", fiendish_boons[i]);
}

/* undefines */
#undef DEBUG_MODE

/*EOF*/
