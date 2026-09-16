/**
 * @file craft/craft_training.c
 * Paid craft trainers: contract rules and status text.
 */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"

#include "craft/craft_training.h"
#include "craft/crafting_new.h"
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
