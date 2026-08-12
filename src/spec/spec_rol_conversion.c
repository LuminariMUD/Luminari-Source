/**
 * @file spec/spec_rol_conversion.c
 * Shared adapters for active Realms of Luminari special procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "comm.h"
#include "handler.h"
#include "magic/spells.h"
#include "mudlim.h"
#include "spec_context.h"
#include "spec_rol_conversion.h"

bool rol_corpse_devourer_can_consume(const struct obj_data *obj)
{
  if (obj == NULL)
    return false;

  if (GET_OBJ_TYPE(obj) == ITEM_FOOD)
    return true;

  return IS_CORPSE(obj) && GET_OBJ_VAL(obj, 4) == 0;
}

int rol_poison_bite_roll_ceiling(int level)
{
  return MAX(0, 61 - level);
}

int rol_corpse_devourer(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj;
  struct obj_data *contained;
  struct obj_data *next;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || cmd || !AWAKE(ch) || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  for (obj = world[IN_ROOM(ch)].contents; obj != NULL; obj = obj->next_content)
  {
    if (!rol_corpse_devourer_can_consume(obj))
      continue;

    if (IS_CORPSE(obj))
    {
      for (contained = obj->contains; contained != NULL; contained = next)
      {
        next = contained->next_content;
        obj_from_obj(contained);
        obj_to_room(contained, IN_ROOM(ch));
      }
    }

    act("$n savagely devours $o.", FALSE, ch, obj, NULL, TO_ROOM);
    extract_obj(obj);
    return TRUE;
  }

  return FALSE;
}

int rol_poison_bite(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || cmd || (victim = FIGHTING(ch)) == NULL)
    return FALSE;

  if (spec_context_validate_combat_target(ch, victim, true) != SPEC_CONTEXT_VALID)
    return FALSE;

  if (rand_number(0, rol_poison_bite_roll_ceiling(GET_LEVEL(ch))) != 0)
    return FALSE;

  act("$n bites $N!", TRUE, ch, NULL, victim, TO_NOTVICT);
  act("$n bites you!", TRUE, ch, NULL, victim, TO_VICT);
  call_magic(ch, victim, NULL, SPELL_POISON, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
  return TRUE;
}

static void rol_thief_steal(struct char_data *ch, struct char_data *victim)
{
  int gold;

  if (IS_NPC(victim) || GET_LEVEL(victim) >= LVL_IMMORT || ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
    return;

  if (AWAKE(victim) && rand_number(0, GET_LEVEL(ch)) == 0)
  {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, NULL, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, NULL, victim, TO_NOTVICT);
    return;
  }

  gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
  if (gold > 0)
  {
    increase_gold(ch, gold);
    decrease_gold(victim, gold);
  }
}

int rol_thief(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || cmd || GET_POS(ch) != POS_STANDING || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = victim->next_in_room)
    if (!IS_NPC(victim) && GET_LEVEL(victim) < LVL_IMMORT)
      rol_thief_steal(ch, victim);

  return TRUE;
}
