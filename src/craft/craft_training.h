/**
 * @file craft/craft_training.h
 * Paid craft trainers: a player pays to leave play and returns with craft or harvest experience.
 *
 * A contract is three fields in the player's crafting data (training_ability, training_exp,
 * training_end), saved as the CrTr player-file tag. See docs/ongoing-projects/craft-trainers.md.
 */

#ifndef LUMINARI_CRAFT_CRAFT_TRAINING_H
#define LUMINARI_CRAFT_CRAFT_TRAINING_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

struct char_data;

/* A contract lasts this many seconds of wall-clock time. */
#define CRAFT_TRAINING_DURATION (24 * 60 * 60)
/* A contract starts only below this rank. */
#define CRAFT_TRAINING_RANK_CEILING 20
/* The fee is this many gold coins times (rank + 1) squared. */
#define CRAFT_TRAINING_FEE_BASE 100

/** True for the craft and harvest abilities a trainer can train. */
bool craft_training_track_eligible(int ability);
/** Experience a contract started at rank grants: half of the next rank's requirement. */
int craft_training_grant(int rank);
/** Gold a contract started at rank costs. */
int craft_training_fee(int rank);
/** Write ch's contract status as of now, "training, 13h 20m left" or "training finished", and
 * return true; write an empty string and return false when ch has no contract. */
bool craft_training_status(const struct char_data *ch, time_t now, char *buffer, size_t size);

/** The Craft Trainer special procedure: the apprentice command lists, quotes, and starts
 * contracts. */
int craft_trainer(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_CRAFT_CRAFT_TRAINING_H */
