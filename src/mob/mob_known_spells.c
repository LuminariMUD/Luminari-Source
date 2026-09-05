/**************************************************************************
 *  File: mob_known_spells.c                          Part of LuminariMUD *
 *  Usage: Mobile known spell slot tracking system.                        *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "magic/spells.h"
#include "constants.h"
#include "mob_known_spells.h"
#include "active_world.h"

#define KNOWN_SPELL_SLOT_RECOVERY_SECONDS 60

/* Initialize known spell slots for a mob when it loads */
void init_known_spell_slots(struct char_data *ch)
{
  int i = 0;

  if (!ch || !IS_NPC(ch))
    return;

  /* Initialize all known spell slots to max (2 per spell) */
  for (i = 0; i < MAX_SPELLS; i++)
  {
    if (MOB_KNOWS_SPELL(ch, i))
    {
      ch->mob_specials.known_spell_slots[i] = 2; /* Max 2 slots per known spell */
    }
    else
    {
      ch->mob_specials.known_spell_slots[i] = 0;
    }
  }

  /* Initialize regeneration timer */
  ch->mob_specials.last_known_slot_regen = time(0);
}

/* Check if mob has an available slot for a known spell */
bool has_known_spell_slot(struct char_data *ch, int spellnum)
{
  if (!ch || !IS_NPC(ch))
    return false;

  if (spellnum < 0 || spellnum >= MAX_SPELLS)
    return false;

  if (!MOB_KNOWS_SPELL(ch, spellnum))
    return false;

  return (ch->mob_specials.known_spell_slots[spellnum] > 0);
}

/* Consume a known spell slot when mob casts a known spell */
void consume_known_spell_slot(struct char_data *ch, int spellnum)
{
  bool recovery_already_pending;

  if (!ch || !IS_NPC(ch))
    return;

  if (spellnum < 0 || spellnum >= MAX_SPELLS)
    return;

  if (!MOB_KNOWS_SPELL(ch, spellnum))
    return;

  if (ch->mob_specials.known_spell_slots[spellnum] > 0)
  {
    recovery_already_pending = known_spell_slots_need_recovery(ch);
    ch->mob_specials.known_spell_slots[spellnum]--;
    if (!recovery_already_pending)
      ch->mob_specials.last_known_slot_regen = time(0);
    active_world_sync_mobile(ch);
  }
}

bool known_spell_slots_need_recovery(const struct char_data *ch)
{
  int spellnum;

  if (ch == NULL || !IS_NPC(ch))
    return false;
  for (spellnum = 0; spellnum < MAX_SPELLS; spellnum++)
    if (MOB_KNOWS_SPELL(ch, spellnum) && ch->mob_specials.known_spell_slots[spellnum] < 2)
      return true;
  return false;
}

time_t known_spell_slot_recovery_deadline(const struct char_data *ch)
{
  if (!known_spell_slots_need_recovery(ch))
    return 0;
  return ch->mob_specials.last_known_slot_regen + KNOWN_SPELL_SLOT_RECOVERY_SECONDS;
}

/* Regenerate one random known spell slot every 60 seconds out of combat */
void regenerate_known_spell_slot(struct char_data *ch)
{
  time_t now = time(0);
  int regeneration_time = KNOWN_SPELL_SLOT_RECOVERY_SECONDS;
  int available_slots[MAX_SPELLS] = {0};
  int num_available = 0;
  int i = 0, random_index = 0;
  int spell_to_regen = -1;

  if (!ch || !IS_NPC(ch))
    return;

  /* Only regenerate out of combat */
  if (FIGHTING(ch))
    return;

  /* Check if enough time has passed since last regeneration */
  if ((now - ch->mob_specials.last_known_slot_regen) < regeneration_time)
    return;

  /* Update regeneration timestamp */
  ch->mob_specials.last_known_slot_regen = now;

  /* Build list of known spells that have slots below maximum (2) */
  for (i = 0; i < MAX_SPELLS; i++)
  {
    if (MOB_KNOWS_SPELL(ch, i) && ch->mob_specials.known_spell_slots[i] < 2)
    {
      available_slots[num_available] = i;
      num_available++;
    }
  }

  /* If we have available slots to regenerate, pick a random one */
  if (num_available > 0)
  {
    random_index = rand_number(0, num_available - 1);
    spell_to_regen = available_slots[random_index];

    if (spell_to_regen >= 0 && spell_to_regen < MAX_SPELLS)
    {
      ch->mob_specials.known_spell_slots[spell_to_regen]++;
    }
  }
}

/* Categorize a known spell based on its properties */
int categorize_known_spell(int spellnum)
{
  if (spellnum < 0 || spellnum >= NUM_SPELLS)
    return KNOWN_SPELL_CATEGORY_UTILITY;

  /* Offensive spells */
  if (spell_info[spellnum].routines & (MAG_DAMAGE))
    return KNOWN_SPELL_CATEGORY_OFFENSIVE;

  /* Healing and resurrection (uses MAG_POINTS for HP restoration) */
  if (spell_info[spellnum].routines & (MAG_POINTS))
    return KNOWN_SPELL_CATEGORY_HEAL;

  /* Summoning */
  if (spell_info[spellnum].routines & (MAG_SUMMONS))
    return KNOWN_SPELL_CATEGORY_SUMMON;

  /* Buffs and debuffs (affects) */
  if (spell_info[spellnum].routines & (MAG_AFFECTS))
    return KNOWN_SPELL_CATEGORY_BUFF;

  /* Everything else is utility */
  return KNOWN_SPELL_CATEGORY_UTILITY;
}

/* Check if mob has any known spells */
bool mob_has_known_spells(struct char_data *ch)
{
  int i = 0;

  if (!ch || !IS_NPC(ch))
    return false;

  /* Check if any spell is known */
  for (i = 0; i < MAX_SPELLS; i++)
  {
    if (MOB_KNOWS_SPELL(ch, i))
      return true;
  }

  return false;
}
