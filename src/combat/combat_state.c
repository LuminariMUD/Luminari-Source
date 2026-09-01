#include "combat/combat_state.h"

#include "combat/fight.h"
#include "db.h"

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
