#include "combat/combat_state.h"

#include "combat/fight.h"
#include "db.h"
#include "domain_event_world.h"

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

bool combat_state_attack_context_valid(struct domain_entity_handle attacker_handle,
                                       struct domain_entity_handle victim_handle,
                                       room_rnum expected_room)
{
  struct char_data *attacker = domain_event_world_resolve_character(attacker_handle);
  struct char_data *victim = domain_event_world_resolve_character(victim_handle);

  if (attacker == NULL || victim == NULL || expected_room == NOWHERE)
    return false;
  if (IN_ROOM(attacker) != expected_room || IN_ROOM(victim) != expected_room)
    return false;
  if (GET_POS(attacker) <= POS_DEAD || GET_POS(victim) <= POS_DEAD)
    return false;
  if ((IS_NPC(attacker) && MOB_FLAGGED(attacker, MOB_NOTDEADYET)) ||
      (!IS_NPC(attacker) && PLR_FLAGGED(attacker, PLR_NOTDEADYET)) ||
      (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOTDEADYET)) ||
      (!IS_NPC(victim) && PLR_FLAGGED(victim, PLR_NOTDEADYET)))
    return false;
  return true;
}
