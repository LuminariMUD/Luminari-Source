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
struct descriptor_data;

/* A contract lasts this many seconds of wall-clock time. */
#define CRAFT_TRAINING_DURATION ((time_t)6 * 60 * 60)
/* A contract starts only below this rank. */
#define CRAFT_TRAINING_RANK_CEILING 20
/* The fee is this many gold coins times (rank + 1) squared. */
#define CRAFT_TRAINING_FEE_BASE 400

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

/** Settle the contract of the character just selected from account menu slot. While the contract
 * runs, explain it, redisplay the menu, and return false. A finished contract grants its
 * experience exactly once: the grant and the cleared record reach the player file in one save. */
bool craft_training_admit_selection(struct descriptor_data *d, int slot, time_t now);
/** Handle "recall <number> [confirm]" at the account menu, ending a contract early without a
 * refund or a grant. */
void craft_training_recall(struct descriptor_data *d, const char *argument);
/** Refuse the main menu's enter-game choice for a character that left play to train, telling the
 * player to go back to the account menu. */
bool craft_training_refuse_entry(struct descriptor_data *d);

#endif /* LUMINARI_CRAFT_CRAFT_TRAINING_H */
