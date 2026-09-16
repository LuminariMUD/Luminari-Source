/**
 * @file craft/craft_training.c
 * Paid craft trainers: contract rules, status text, and the trainer's apprentice command.
 */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"

#include "act/act.h"
#include "character/rewards.h"
#include "core/comm.h"
#include "core/constants.h"
#include "core/db.h"
#include "core/handler.h"
#include "core/helpers.h"
#include "core/interpreter.h"
#include "craft/craft_training.h"
#include "craft/crafting_new.h"
#include "events/activity_manager.h"
#include "magic/spells.h"

bool craft_training_track_eligible(int ability)
{
  if (ability < START_CRAFT_ABILITIES || ability > END_HARVEST_ABILITIES)
    return false;
  return crafting_skill_type(ability) != CRAFT_SKILL_TYPE_NONE;
}

int craft_training_grant(int rank)
{
  return (craft_skill_level_exp(NULL, rank + 1) - craft_skill_level_exp(NULL, rank)) / 2;
}

int craft_training_fee(int rank)
{
  return CRAFT_TRAINING_FEE_BASE * (rank + 1) * (rank + 1);
}

bool craft_training_status(const struct char_data *ch, time_t now, char *buffer, size_t size)
{
  long minutes;

  if (size > 0)
    *buffer = '\0';
  if (ch == NULL || ch->player_specials == NULL || !GET_CRAFT(ch).training_ability)
    return false;

  if (GET_CRAFT(ch).training_end <= now)
  {
    snprintf(buffer, size, "training finished");
    return true;
  }

  /* Round up, so a contract with seconds to go never reads as zero minutes left. */
  minutes = ((long)(GET_CRAFT(ch).training_end - now) + 59) / 60;
  if (minutes >= 60)
    snprintf(buffer, size, "training, %ldh %ldm left", minutes / 60, minutes % 60);
  else
    snprintf(buffer, size, "training, %ldm left", minutes);
  return true;
}

/* Match a craft or harvest skill name, preferring the tracks a trainer can train, so a prefix
 * shared with an untrainable skill still finds the trainable one. */
static int craft_training_find_track(const char *name)
{
  int ability;

  for (ability = START_CRAFT_ABILITIES; ability <= END_HARVEST_ABILITIES; ability++)
    if (craft_training_track_eligible(ability) && is_abbrev(name, ability_names[ability]))
      return ability;
  for (ability = START_CRAFT_ABILITIES; ability <= END_HARVEST_ABILITIES; ability++)
    if ((ability <= END_CRAFT_ABILITIES || ability >= START_HARVEST_ABILITIES) &&
        is_abbrev(name, ability_names[ability]))
      return ability;
  return ABILITY_UNDEFINED;
}

static void craft_training_list(struct char_data *ch, struct char_data *trainer)
{
  int ability, rank;

  act("$N can train these skills while you spend a day away from play:", FALSE, ch, 0, trainer,
      TO_CHAR);
  send_to_char(ch, "%-16s %4s %10s %8s\r\n", "Skill", "Rank", "Experience", "Fee");
  for (ability = START_CRAFT_ABILITIES; ability <= END_HARVEST_ABILITIES; ability++)
  {
    if (!craft_training_track_eligible(ability))
      continue;
    rank = GET_ABILITY(ch, ability);
    if (rank >= CRAFT_TRAINING_RANK_CEILING)
      send_to_char(ch, "%-16s %4d %10s %8s  (trained only below rank %d)\r\n",
                   ability_names[ability], rank, "-", "-", CRAFT_TRAINING_RANK_CEILING);
    else
      send_to_char(ch, "%-16s %4d %10d %8d%s\r\n", ability_names[ability], rank,
                   craft_training_grant(rank), craft_training_fee(rank),
                   GET_GOLD(ch) < craft_training_fee(rank) ? "  (not enough gold on hand)" : "");
  }
  send_to_char(ch, "Type 'apprentice <skill>' for a quote.\r\n");
}

/* Take the fee, record the contract, and send the player out of play. The contract reaches the
 * player file before anything else happens, so a failed save refunds and changes nothing. */
static void craft_training_start(struct char_data *ch, struct char_data *trainer, int ability,
                                 int rank)
{
  int fee = craft_training_fee(rank);

  award_gold(ch, -fee);
  GET_CRAFT(ch).training_ability = ability;
  GET_CRAFT(ch).training_exp = craft_training_grant(rank);
  GET_CRAFT(ch).training_end = time(0) + CRAFT_TRAINING_DURATION;
  if (!save_char_checked(ch, 0))
  {
    award_gold(ch, fee);
    GET_CRAFT(ch).training_ability = 0;
    GET_CRAFT(ch).training_exp = 0;
    GET_CRAFT(ch).training_end = 0;
    send_to_char(ch, "Your training could not be recorded, so your gold has been returned.\r\n");
    log("SYSERR: craft_trainer: could not save the training contract for %s.", GET_NAME(ch));
    return;
  }

  act("$N accepts your fee and leads you away to train.", FALSE, ch, 0, trainer, TO_CHAR);
  act("$N leads $n away to train.", TRUE, ch, 0, trainer, TO_ROOM);
  mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
         "%s began craft training: %s from rank %d for %d experience and %d gold, until %s.",
         GET_NAME(ch), ability_names[ability], rank, GET_CRAFT(ch).training_exp, fee,
         format_time_ymd_hms(GET_CRAFT(ch).training_end));

  /* perform_player_quit() saves belongings only when rent is free; save them exactly once. */
  if (!CONFIG_FREE_RENT)
    Crash_rentsave(ch, 0);
  perform_player_quit(ch);
}

int craft_trainer(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *trainer = (struct char_data *)me;
  struct primary_activity_snapshot activity;
  char skill[MAX_INPUT_LENGTH], confirm[MAX_INPUT_LENGTH];
  int ability, rank, fee;

  if (!cmd || !CMD_IS("apprentice") || IS_NPC(ch) || ch->desc == NULL)
    return FALSE;

  if (!AWAKE(trainer))
  {
    act("$N is unable to talk to you...", FALSE, ch, 0, trainer, TO_CHAR);
    return TRUE;
  }
  if (!CAN_SEE(trainer, ch))
  {
    act("$n says, 'I don't teach people I can't see!'", FALSE, trainer, 0, 0, TO_ROOM);
    return TRUE;
  }

  two_arguments(argument, skill, sizeof(skill), confirm, sizeof(confirm));
  if (!*skill)
  {
    craft_training_list(ch, trainer);
    return TRUE;
  }

  ability = craft_training_find_track(skill);
  if (ability == ABILITY_UNDEFINED)
  {
    send_to_char(ch, "There is no craft or harvest skill called '%s'.\r\n", skill);
    return TRUE;
  }
  if (!craft_training_track_eligible(ability))
  {
    send_to_char(ch, "The %s skill cannot be trained here.\r\n", ability_names[ability]);
    return TRUE;
  }
  rank = GET_ABILITY(ch, ability);
  if (rank >= CRAFT_TRAINING_RANK_CEILING)
  {
    send_to_char(ch, "Trainers teach %s only below rank %d, and yours is %d.\r\n",
                 ability_names[ability], CRAFT_TRAINING_RANK_CEILING, rank);
    return TRUE;
  }
  if (GET_CRAFT(ch).training_ability)
  {
    send_to_char(ch, "You already have a training contract.\r\n");
    return TRUE;
  }
  if (FIGHTING(ch))
  {
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
    return TRUE;
  }
  if (primary_activity_snapshot(ch, &activity))
  {
    send_to_char(ch, "You cannot leave to train while %s.\r\n", activity.display_name);
    return TRUE;
  }
  fee = craft_training_fee(rank);
  if (GET_GOLD(ch) < fee)
  {
    send_to_char(ch, "Training %s from rank %d costs %d gold coins, and you carry %d.\r\n",
                 ability_names[ability], rank, fee, GET_GOLD(ch));
    return TRUE;
  }

  if (strcmp(confirm, "confirm") != 0)
  {
    act("$N looks you over and names a price.", FALSE, ch, 0, trainer, TO_CHAR);
    send_to_char(ch,
                 "Training %s from rank %d costs %d gold coins. You would leave play for %d hours "
                 "and return with %d %s experience, plus any insightful talent bonus.\r\n"
                 "Leaving works like quit: your followers are dismissed and timed quests end. "
                 "Recalling early from the account menu forfeits the fee and the experience.\r\n"
                 "Type 'apprentice %s confirm' to begin.\r\n",
                 ability_names[ability], rank, fee, CRAFT_TRAINING_DURATION / 3600,
                 craft_training_grant(rank), ability_names[ability], ability_names[ability]);
    return TRUE;
  }

  craft_training_start(ch, trainer, ability, rank);
  return TRUE;
}
