/**************************************************************************
 *  File: rewards.c                                    Part of LuminariMUD *
 *  Usage: Reward API for experience, currency, and progression balances.  *
 *                                                                         *
 *  Gameplay changes to experience, quest points, account experience,     *
 *  gold, bank gold, and the staff-awarded progression pools pass through  *
 *  these functions.  Only record construction writes those fields        *
 *  directly: player files (players.c), world data and the first player   *
 *  (db.c), and account loading and descriptor sync (account.c).         *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include <limits.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h" /* save_account */
#include "screen.h"
#include "mudlim.h"
#include "rewards.h"
#include "magic/spells.h"
#include "character/class.h"
#include "character/perks.h"

/* Whether this character has the balance named by type. */
static bool award_type_applies(struct char_data *ch, int type)
{
  switch (type)
  {
  case AWARD_EXPERIENCE:
  case AWARD_GOLD:
    return TRUE;
  case AWARD_ACCOUNT_EXPERIENCE:
    return !IS_NPC(ch) && ch->desc != NULL && ch->desc->account != NULL;
  case AWARD_CLASS_FEATS:
  case AWARD_EPIC_CLASS_FEATS:
    return !IS_NPC(ch) && ch->player_specials != NULL && GET_CLASS(ch) >= 0 &&
           GET_CLASS(ch) < NUM_CLASSES;
  case AWARD_BANK_GOLD: /* stored in points, not player_specials */
    return !IS_NPC(ch);
  case AWARD_QUEST_POINTS:
  case AWARD_SKILL_POINTS:
  case AWARD_FEATS:
  case AWARD_EPIC_FEATS:
  case AWARD_ABILITY_BOOSTS:
    return !IS_NPC(ch) && ch->player_specials != NULL;
  default:
    return FALSE;
  }
}

/* The largest value a balance may hold: a configured limit or its storage range. */
static long award_limit(int type)
{
  switch (type)
  {
  case AWARD_EXPERIENCE:
    return LONG_MAX;
  case AWARD_QUEST_POINTS:
    return MAX_QUEST_POINTS;
  case AWARD_ACCOUNT_EXPERIENCE:
    return MAX_ACCOUNT_EXPERIENCE;
  case AWARD_GOLD:
    return MAX_GOLD;
  case AWARD_BANK_GOLD:
    return MAX_BANK;
  case AWARD_SKILL_POINTS:
    return INT_MAX;
  case AWARD_FEATS:
  case AWARD_CLASS_FEATS:
  case AWARD_EPIC_FEATS:
  case AWARD_EPIC_CLASS_FEATS:
    return SCHAR_MAX;
  case AWARD_ABILITY_BOOSTS:
    return UCHAR_MAX;
  default:
    return 0;
  }
}

static long award_balance(struct char_data *ch, int type)
{
  switch (type)
  {
  case AWARD_EXPERIENCE:
    return GET_EXP(ch);
  case AWARD_QUEST_POINTS:
    return GET_QUESTPOINTS(ch);
  case AWARD_ACCOUNT_EXPERIENCE:
    return ch->desc->account->experience;
  case AWARD_GOLD:
    return GET_GOLD(ch);
  case AWARD_BANK_GOLD:
    return GET_BANK_GOLD(ch);
  case AWARD_SKILL_POINTS:
    return GET_TRAINS(ch);
  case AWARD_FEATS:
    return GET_FEAT_POINTS(ch);
  case AWARD_CLASS_FEATS:
    return GET_CLASS_FEATS(ch, GET_CLASS(ch));
  case AWARD_EPIC_FEATS:
    return GET_EPIC_FEAT_POINTS(ch);
  case AWARD_EPIC_CLASS_FEATS:
    return GET_EPIC_CLASS_FEATS(ch, GET_CLASS(ch));
  case AWARD_ABILITY_BOOSTS:
    return GET_BOOSTS(ch);
  default:
    return 0;
  }
}

/* The only writer of award balances; value is already within the limit. */
static void award_store(struct char_data *ch, int type, long value)
{
  switch (type)
  {
  case AWARD_EXPERIENCE:
    GET_EXP(ch) = value;
    break;
  case AWARD_QUEST_POINTS:
    GET_QUESTPOINTS(ch) = (int)value;
    break;
  case AWARD_ACCOUNT_EXPERIENCE:
    ch->desc->account->experience = (int)value;
    /* Persist to DB and update other descriptors that share this account */
    save_account(ch->desc->account);
    break;
  case AWARD_GOLD:
    GET_GOLD(ch) = (int)value;
    break;
  case AWARD_BANK_GOLD:
    GET_BANK_GOLD(ch) = (int)value;
    break;
  case AWARD_SKILL_POINTS:
    GET_TRAINS(ch) = (int)value;
    break;
  case AWARD_FEATS:
    GET_FEAT_POINTS(ch) = (byte)value;
    break;
  case AWARD_CLASS_FEATS:
    GET_CLASS_FEATS(ch, GET_CLASS(ch)) = (byte)value;
    break;
  case AWARD_EPIC_FEATS:
    GET_EPIC_FEAT_POINTS(ch) = (byte)value;
    break;
  case AWARD_EPIC_CLASS_FEATS:
    GET_EPIC_CLASS_FEATS(ch, GET_CLASS(ch)) = (byte)value;
    break;
  case AWARD_ABILITY_BOOSTS:
    GET_BOOSTS(ch) = (ubyte)value;
    break;
  default:
    break;
  }
}

long award_points(struct char_data *ch, int type, long amount)
{
  long current, limit, updated;

  if (ch == NULL || !award_type_applies(ch, type))
    return 0;

  current = award_balance(ch, type);
  limit = award_limit(type);
  updated = current;

  if (amount > 0 && current < limit)
  {
    if (current >= 0 && amount > limit - current)
      updated = limit;
    else
      updated = current + amount;
    if (updated > limit)
      updated = limit;
  }
  else if (amount < 0 && current > 0)
    updated = (amount < -current) ? 0 : current + amount;

  if (updated != current)
    award_store(ch, type, updated);

  if (amount > 0 && updated >= limit)
  {
    if (type == AWARD_GOLD)
      send_to_char(ch,
                   "%sYou have reached the maximum gold!\r\n%sYou must spend it or bank it before "
                   "you can gain any more.\r\n",
                   QBRED, QNRM);
    else if (type == AWARD_BANK_GOLD)
      send_to_char(
          ch,
          "%sYou have reached the maximum bank balance!\r\n%sYou cannot put more into your "
          "account unless you withdraw some first.\r\n",
          QBRED, QNRM);
  }

  return updated - current;
}

long award_set_points(struct char_data *ch, int type, long value)
{
  long current, updated;

  if (ch == NULL || !award_type_applies(ch, type))
    return 0;

  current = award_balance(ch, type);
  updated = value;
  if (updated < 0)
    updated = 0;
  else if (updated > award_limit(type))
    updated = award_limit(type);

  if (updated != current)
    award_store(ch, type, updated);

  return updated - current;
}

long award_capacity(struct char_data *ch, int type)
{
  long current, limit;

  if (ch == NULL || !award_type_applies(ch, type))
    return 0;

  current = award_balance(ch, type);
  limit = award_limit(type);
  if (current >= limit)
    return 0;
  return current > 0 ? limit - current : limit;
}

int award_gold(struct char_data *ch, int amount)
{
  return (int)award_points(ch, AWARD_GOLD, amount);
}

int award_bank_gold(struct char_data *ch, int amount)
{
  return (int)award_points(ch, AWARD_BANK_GOLD, amount);
}

int award_quest_points(struct char_data *ch, int amount)
{
  return (int)award_points(ch, AWARD_QUEST_POINTS, amount);
}

int award_account_experience(struct char_data *ch, int amount)
{
  return (int)award_points(ch, AWARD_ACCOUNT_EXPERIENCE, amount);
}

/* Earned-experience tuning for award_experience(). */
#define NEWBIE_EXP 150
#define MIN_NUM_MOBS_TO_KILL_5 9
#define MIN_NUM_MOBS_TO_KILL_10 24
#define MIN_NUM_MOBS_TO_KILL_15 59
#define MIN_NUM_MOBS_TO_KILL_20 120
#define MIN_NUM_MOBS_TO_KILL_25 185
int award_experience(struct char_data *ch, int gain, int mode)
{
  long int xp_to_lvl = 0;
  long int xp_to_lvl_cap = 0;
  long int gain_cap = 0;
  long lost = 0;
  int account_gain = 0;

  if (ch == NULL)
    return 0;

  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
    return 0;

  /* discourage people from killing their pets at the end of the day */
  if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master)
  {
    return 0;
  }

  if (IS_NPC(ch))
    return (int)award_points(ch, AWARD_EXPERIENCE, gain / 2);

  xp_to_lvl_cap = level_exp(ch, GET_LEVEL(ch) + 2);

  if (gain > 0)
  {
    if (GET_EXP(ch) > xp_to_lvl_cap && gain > 0 && GET_LEVEL(ch) < 30)
    {
      send_to_char(ch, "Your experience has been capped.  You must gain a level before you can "
                       "begin earning experience again.\r\n");
      return 0;
    }

    // leadership bonus
    gain = (int)((double)gain * ((double)leadership_exp_multiplier(ch) / (double)(100)));
    /* newbie bonus */
    if (GET_LEVEL(ch) <= NEWBIE_LEVEL)
      gain += (int)((double)gain * ((double)NEWBIE_EXP / (double)(100)));

    if (HAS_FEAT(ch, FEAT_ADAPTABILITY))
      gain += (int)((double)gain * .05);

    if (HAS_FEAT(ch, FEAT_BG_HERMIT) && get_party_size_same_room(ch) == 1)
      gain += (int)((double)gain * .05);

    /* flat rate for now! (halfed the rate for testing purposes) */
    if (rand_number(0, 1) && ch && ch->desc && ch->desc->account)
    {
      if (gain >= 3000)
      {
        account_gain = award_account_experience(ch, (gain / 3000));
        if (!ch->char_specials.post_combat_messages)
        {
          if (account_gain >= 4)
          {
            send_to_char(ch, "You gain %d account experience points!\r\n", account_gain);
          }
        }
        else
        {
          ch->char_specials.post_combat_account_exp = account_gain;
        }
      }
    }

    /* some limited xp cap conditions */
    switch (mode)
    {
      /* quest, script xp not limited here */
    case AWARD_EXP_MODE_QUEST:
    case AWARD_EXP_MODE_SCRIPT:
    case AWARD_EXP_MODE_DEATH: /* should be negative and not get here! */
      break;
    case AWARD_EXP_MODE_EDRAIN:
    case AWARD_EXP_MODE_CRAFT:
    case AWARD_EXP_MODE_DAMAGE:
    case AWARD_EXP_MODE_DUMP:
      /* further cap these */
      xp_to_lvl = level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch));
      if (GET_LEVEL(ch) < 6)
      {
        gain_cap = gain; /* no cap */
      }
      else if (GET_LEVEL(ch) < 11)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_5 * 4);
      }
      else if (GET_LEVEL(ch) < 16)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_10 * 4);
      }
      else if (GET_LEVEL(ch) < 21)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_15 * 4);
      }
      else if (GET_LEVEL(ch) < 26)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_20 * 4);
      }
      else
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_25 * 4);
      }
      gain = (int)long_min(gain_cap, gain);
      break;
    case AWARD_EXP_MODE_GROUP:
    case AWARD_EXP_MODE_SOLO:
    case AWARD_EXP_MODE_DEFAULT:
    case AWARD_EXP_MODE_TRAP:
    default:
      xp_to_lvl = level_exp(ch, GET_LEVEL(ch) + 1) - level_exp(ch, GET_LEVEL(ch));

      /* The no cap for below level 6 was causing serious power levelling issues. -- Gicker May 28, 2020
      if (GET_LEVEL(ch) < 6)
      {
        gain_cap = gain; // no cap
      }
      else
      */

      if (GET_LEVEL(ch) < 11)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_5);
      }
      else if (GET_LEVEL(ch) < 16)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_10);
      }
      else if (GET_LEVEL(ch) < 21)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_15);
      }
      else if (GET_LEVEL(ch) < 26)
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_20);
      }
      else
      {
        gain_cap = xp_to_lvl / (MIN_NUM_MOBS_TO_KILL_25);
      }
      gain = (int)long_min(gain_cap, gain);
      break;
    }

    /* happy hour bonus, purposely applied after above caps */
    if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
      gain += (int)((double)gain * ((double)HAPPY_EXP / (double)(100)));

    /* put an absolute cap on the max gain per kill */
    gain = MIN(CONFIG_MAX_EXP_GAIN, gain);

    /* new gain xp cap -zusuk */
    gain = (int)award_points(ch, AWARD_EXPERIENCE, gain);

    /* Check for stage advancement (Stage-based XP tracking - Step 3) */
    if (!IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT)
    {
      int perk_points_awarded = 0;
      check_stage_advancement(ch, &perk_points_awarded);
    }
  }
  else if (gain < 0)
  {
    gain = MAX(-CONFIG_MAX_EXP_LOSS, gain); /* Cap max exp lost per death */

    /* end game characters get hit much harder */
    if (GET_LEVEL(ch) >= 30)
    {
      gain -= 300000;
    }

    /* bam - hit 'em!  Return the loss actually taken; resurrection restores it. */
    lost = award_points(ch, AWARD_EXPERIENCE, gain);
    send_to_char(ch, "You lose %ld experience points!\r\n", -lost);
    gain = (int)lost;
  }

  if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
    run_autowiz();

  if (GET_LEVEL(ch) < LVL_IMMORT - CONFIG_NO_MORT_TO_IMMORT &&
      GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
    send_to_char(ch, "\tDYou have gained enough xp to advance, type 'gain' to level.\tn\r\n");

  if (mode == AWARD_EXP_MODE_GROUP || mode == AWARD_EXP_MODE_SOLO)
    ch->char_specials.post_combat_exp = gain;

  return gain;
}

int award_experience_uncapped(struct char_data *ch, int gain, bool is_ress)
{
  int is_altered = FALSE;
  int num_levels = 0;

  if (ch == NULL)
    return 0;

  if (!is_ress)
  {
    if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
      gain += (int)((double)gain * ((double)HAPPY_EXP / (double)(100)));
  }

  gain = (int)award_points(ch, AWARD_EXPERIENCE, gain);

  if (!is_ress)
  {
    if (!IS_NPC(ch))
    {
      while (GET_LEVEL(ch) < LVL_IMPL && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
      {
        GET_LEVEL(ch) += 1;
        if (CLASS_LEVEL(ch, GET_CLASS(ch)) < (LVL_STAFF - 1))
          CLASS_LEVEL(ch, GET_CLASS(ch))
        ++;
        num_levels++;
        /* our function for leveling up, takes in class that is being advanced */
        advance_level(ch, GET_CLASS(ch));
        is_altered = TRUE;
      }

      if (is_altered)
      {
        mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced %d level%s to level %d.",
               GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
        if (num_levels == 1)
          send_to_char(ch, "You rise a level!\r\n");
        else
          send_to_char(ch, "You rise %d levels!\r\n", num_levels);
        set_title(ch, NULL);
      }
    }
  }

  if (!is_ress)
  {
    if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
      run_autowiz();
  }

  if (GET_LEVEL(ch) < LVL_IMMORT - CONFIG_NO_MORT_TO_IMMORT &&
      GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1))
    send_to_char(ch, "\tDYou have gained enough xp to advance, type 'gain' to level.\tn\r\n");

  return gain;
}
