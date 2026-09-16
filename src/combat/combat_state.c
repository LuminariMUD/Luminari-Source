#include "combat/combat_state.h"

#include "combat/fight.h"
#include "core/db.h"
#include "events/domain_event_world.h"

/* Count the characters currently fighting this victim.
 * Walks the live character list; there is no separate fighting roster. */
size_t combat_state_count_attackers(const struct char_data *victim)
{
  const struct char_data *attacker;
  size_t count = 0U;

  if (victim == NULL)
    return 0U;
  for (attacker = character_list; attacker != NULL; attacker = attacker->next)
    if (FIGHTING(attacker) == victim)
      count++;
  return count;
}

/* Stop every character currently fighting this victim.
 * The successor is captured before stop_fighting() so the walk survives the
 * combat-state changes it triggers. */
void combat_state_stop_attackers(struct char_data *victim)
{
  struct char_data *attacker;
  struct char_data *next;

  if (victim == NULL)
    return;
  for (attacker = character_list; attacker != NULL; attacker = next)
  {
    next = attacker->next;
    if (FIGHTING(attacker) == victim)
      stop_fighting(attacker);
  }
}

/* A participant must still resolve, remain in its original room and be alive,
 * without pending extraction. Spell participants may start in different rooms. */
bool combat_state_character_context_valid(struct domain_entity_handle character_handle,
                                          room_rnum expected_room)
{
  struct char_data *ch = domain_event_world_resolve_character(character_handle);

  if (ch == NULL || expected_room == NOWHERE)
    return false;
  if (IN_ROOM(ch) != expected_room || GET_POS(ch) <= POS_DEAD)
    return false;
  if ((IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOTDEADYET)) ||
      (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_NOTDEADYET)))
    return false;
  return true;
}

/* An attack also requires both participants to remain in the same room. */
bool combat_state_attack_context_valid(struct domain_entity_handle attacker_handle,
                                       struct domain_entity_handle victim_handle,
                                       room_rnum expected_room)
{
  return combat_state_character_context_valid(attacker_handle, expected_room) &&
         combat_state_character_context_valid(victim_handle, expected_room);
}
