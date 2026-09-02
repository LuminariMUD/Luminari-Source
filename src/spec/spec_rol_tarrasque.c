/**
 * @file spec/spec_rol_tarrasque.c
 * Converted Realms of Luminari Tarrasque encounter procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "combat/fight.h"
#include "comm.h"
#include "db.h"
#include "dgscript/dg_scripts.h"
#include "handler.h"
#include "interpreter.h"
#include "magic/domains_schools.h"
#include "magic/spell_prep.h"
#include "magic/spells.h"
#include "mud_event.h"
#include "spec_context.h"
#include "spec_dispatch.h"
#include "spec_rol_tarrasque.h"

#define ROL_TARRASQUE_MOBILE_VNUM 2002601
#define ROL_TARRASQUE_CORPSE_VNUM 2002604
#define ROL_TARRASQUE_HORN_VNUM 2002605
#define ROL_TARRASQUE_GOGGLES_VNUM 2002606
#define ROL_TARRASQUE_SHOVEL_VNUM 2002607
#define ROL_TARRASQUE_CROWN_VNUM 2002608
#define ROL_TARRASQUE_EXIT_PORTAL_VNUM 2002609
#define ROL_TARRASQUE_STOMACH_ACID_VNUM 2002610
#define ROL_TARRASQUE_STOMACH_ROOM_VNUM 2002661

#define ROL_TARRASQUE_DEATH_LEVEL 59
#define ROL_TARRASQUE_STUN_ROUNDS 2
#define ROL_TARRASQUE_TELEPORT_ATTEMPTS 100

static bool rol_tarrasque_command_is(int cmd, const char *name)
{
  return cmd > 0 && name != NULL && complete_cmd_info != NULL &&
         !strcmp(complete_cmd_info[cmd].command, name);
}

int rol_tarrasque_loot_vnum_for_roll(int roll)
{
  if (roll >= 1 && roll <= 6)
    return ROL_TARRASQUE_HORN_VNUM;
  if (roll >= 7 && roll <= 12)
    return ROL_TARRASQUE_GOGGLES_VNUM;
  if (roll >= 13 && roll <= 18)
    return ROL_TARRASQUE_SHOVEL_VNUM;
  if (roll >= 19 && roll <= 20)
    return ROL_TARRASQUE_CROWN_VNUM;
  return NOTHING;
}

bool rol_tarrasque_corpse_keyword(const char *argument, const char *aliases)
{
  char keyword[MAX_INPUT_LENGTH];

  if (argument == NULL || aliases == NULL)
    return false;

  one_argument(argument, keyword, sizeof(keyword));
  return *keyword != '\0' && isname(keyword, aliases);
}

static bool rol_tarrasque_target_is_mortal(const struct char_data *target)
{
  return target != NULL && (!IS_NPC(target) || IS_PET(target)) && GET_LEVEL(target) < LVL_IMMORT;
}

static void rol_tarrasque_stop_attackers(struct char_data *victim)
{
  struct char_data *attacker;
  struct char_data *next;

  if (victim == NULL)
    return;

  if (FIGHTING(victim) != NULL)
    stop_fighting(victim);

  for (attacker = combat_list; attacker != NULL; attacker = next)
  {
    next = attacker->next_fighting;
    if (FIGHTING(attacker) == victim)
      stop_fighting(attacker);
  }
}

static void rol_tarrasque_sudden_death(struct char_data *victim, struct char_data *killer)
{
  if (victim == NULL || killer == NULL)
    return;

  GET_HIT(victim) = -11;
  update_pos(victim);
  die(victim, killer);
}

static int rol_tarrasque_apply_stomach_acid(struct obj_data *acid)
{
  struct char_data *target;
  struct char_data *next;
  room_rnum room;
  int result;

  if (acid == NULL || !VALID_OBJ_RNUM(acid) || (room = IN_ROOM(acid)) == NOWHERE ||
      !VALID_ROOM_RNUM(room))
    return FALSE;

  for (target = world[room].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (!rol_tarrasque_target_is_mortal(target))
      continue;

    act("$n's flesh burns in the Tarrasque's caustic stomach acid!", FALSE, target, NULL, NULL,
        TO_ROOM);
    act("Your flesh burns in the Tarrasque's caustic stomach acid!", FALSE, target, NULL, NULL,
        TO_CHAR);
    result = damage(target, target, dice(10, 10), -1, DAM_ACID, FALSE);
    if (result < 0 || GET_POS(target) <= POS_DEAD || IN_ROOM(target) != room)
      continue;

    if (IS_CASTING(target) && rand_number(0, 2) == 0)
    {
      act("$n cringes from the pain and loses $s spell.", FALSE, target, NULL, NULL, TO_ROOM);
      act("You cringe from the pain and lose your spell.", FALSE, target, NULL, NULL, TO_CHAR);
      resetCastingData(target);
    }
    if (!IS_NPC(target) && char_has_mud_event(target, ePREPARATION) != NULL &&
        rand_number(0, 2) == 0)
    {
      act("$n cringes from the pain and loses $s train of thought.", FALSE, target, NULL, NULL,
          TO_ROOM);
      act("You cringe from the pain and lose your train of thought.", FALSE, target, NULL, NULL,
          TO_CHAR);
      stop_all_preparations(target);
    }
  }

  return FALSE;
}

static int rol_tarrasque_activity(struct char_data *mob)
{
  if (mob == NULL || !IS_MOB(mob) || GET_MOB_VNUM(mob) != ROL_TARRASQUE_MOBILE_VNUM ||
      GET_HIT(mob) >= GET_MAX_HIT(mob))
    return FALSE;

  GET_HIT(mob) += 25;
  return TRUE;
}

static int rol_tarrasque_pet_bite(struct char_data *mob, struct char_data *victim)
{
  act("The Tarrasque's gigantic maw chomps you in half!", FALSE, mob, NULL, victim, TO_VICT);
  act("$N is chomped in half by the Tarrasque's gigantic maw!", FALSE, mob, NULL, victim,
      TO_NOTVICT);
  rol_tarrasque_sudden_death(victim, mob);
  GET_HIT(mob) += 300;
  return TRUE;
}

static int rol_tarrasque_swallow(struct char_data *mob, struct char_data *victim)
{
  room_rnum stomach_room;
  int amount;

  amount = dice(10, 15);
  if (GET_HIT(victim) < amount)
  {
    act("The Tarrasque's gigantic maw chomps you in half!", FALSE, mob, NULL, victim, TO_VICT);
    act("$N is chomped in half by the Tarrasque's gigantic maw!", FALSE, mob, NULL, victim,
        TO_NOTVICT);
    rol_tarrasque_sudden_death(victim, mob);
    return TRUE;
  }

  stomach_room = real_room(ROL_TARRASQUE_STOMACH_ROOM_VNUM);
  if (!VALID_ROOM_RNUM(stomach_room))
  {
    log("SYSERR: RoL Tarrasque stomach room %d is unavailable", ROL_TARRASQUE_STOMACH_ROOM_VNUM);
    return TRUE;
  }

  act("$n swallows $N whole!", FALSE, mob, NULL, victim, TO_NOTVICT);
  act("The Tarrasque's enormous maw closes over you, and everything goes dark.", FALSE, mob, NULL,
      victim, TO_VICT);
  rol_tarrasque_stop_attackers(victim);
  resetCastingData(victim);
  char_from_room(victim);
  char_to_room(victim, stomach_room);
  (void)damage(mob, victim, amount, -1, DAM_BLUDGEON, FALSE);
  GET_HIT(mob) += 200;
  return TRUE;
}

static room_rnum rol_tarrasque_random_teleport_room(struct char_data *victim)
{
  room_rnum destination;
  int attempt;

  if (victim == NULL)
    return NOWHERE;

  for (attempt = 0; attempt < ROL_TARRASQUE_TELEPORT_ATTEMPTS; attempt++)
  {
    destination = rand_number(0, top_of_world);
    if (destination != IN_ROOM(victim) && valid_mortal_tele_dest(victim, destination, TRUE))
      return destination;
  }

  return NOWHERE;
}

static int rol_tarrasque_tail_fling(struct char_data *mob, struct char_data *victim)
{
  room_rnum destination;

  destination = rol_tarrasque_random_teleport_room(victim);
  if (!VALID_ROOM_RNUM(destination))
  {
    log("SYSERR: RoL Tarrasque could not find a safe tail-fling destination");
    return TRUE;
  }

  act("The Tarrasque's mighty tail smashes into your chest and sends you flying!", FALSE, mob, NULL,
      victim, TO_VICT);
  act("The Tarrasque's mighty tail smashes into $N and sends $M flying!", FALSE, mob, NULL, victim,
      TO_NOTVICT);
  rol_tarrasque_stop_attackers(victim);
  resetCastingData(victim);
  char_from_room(victim);
  if (ZONE_FLAGGED(GET_ROOM_ZONE(destination), ZONE_WILDERNESS))
  {
    X_LOC(victim) = world[destination].coords[0];
    Y_LOC(victim) = world[destination].coords[1];
  }
  char_to_room(victim, destination);
  GET_POS(victim) = POS_RECLINING;
  if (can_stun(victim) && char_has_mud_event(victim, eSTUNNED) == NULL)
    attach_mud_event(new_mud_event(eSTUNNED, victim, NULL),
                     PULSE_VIOLENCE * ROL_TARRASQUE_STUN_ROUNDS);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
  return TRUE;
}

static int rol_tarrasque_tail_sweep(struct char_data *mob)
{
  struct char_data *target;
  struct char_data *next;
  int amount;

  for (target = world[IN_ROOM(mob)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (!rol_tarrasque_target_is_mortal(target))
      continue;

    amount = dice(20, 12);
    if (savingthrow(mob, target, SAVING_REFL, -2, CAST_INNATE, ROL_TARRASQUE_DEATH_LEVEL, NOSCHOOL))
      amount /= 2;

    act("The Tarrasque's gigantic tail smashes into your chest!", FALSE, mob, NULL, target,
        TO_VICT);
    act("The Tarrasque's gigantic tail smashes into $N!", FALSE, mob, NULL, target, TO_NOTVICT);
    if (GET_HIT(target) < amount)
      rol_tarrasque_sudden_death(target, mob);
    else
      (void)damage(mob, target, amount, -1, DAM_BLUDGEON, FALSE);
  }

  return TRUE;
}

static int rol_tarrasque_combat_turn(struct char_data *mob)
{
  struct char_data *victim;

  if (mob == NULL || !IS_MOB(mob) || GET_MOB_VNUM(mob) != ROL_TARRASQUE_MOBILE_VNUM ||
      (victim = FIGHTING(mob)) == NULL ||
      spec_context_validate_combat_target(mob, victim, true) != SPEC_CONTEXT_VALID)
    return FALSE;

  if (IS_PET(victim))
    return rol_tarrasque_pet_bite(mob, victim);
  if (rand_number(0, 18) == 0)
    return rol_tarrasque_swallow(mob, victim);
  if (rand_number(0, 30) == 0)
    return rol_tarrasque_tail_fling(mob, victim);
  if (rand_number(0, 15) == 0)
    return rol_tarrasque_tail_sweep(mob);
  return FALSE;
}

static void rol_tarrasque_extract_death_objects(struct obj_data *corpse, struct obj_data *loot,
                                                struct obj_data *portal)
{
  if (corpse != NULL)
    extract_obj(corpse);
  if (loot != NULL)
    extract_obj(loot);
  if (portal != NULL)
    extract_obj(portal);
}

static int rol_tarrasque_death(struct char_data *mob)
{
  struct obj_data *corpse;
  struct obj_data *loot;
  struct obj_data *portal;
  room_rnum death_room;
  room_rnum stomach_room;
  int loot_vnum;

  if (mob == NULL || !IS_MOB(mob) || GET_MOB_VNUM(mob) != ROL_TARRASQUE_MOBILE_VNUM ||
      !VALID_ROOM_RNUM((death_room = IN_ROOM(mob))) ||
      !VALID_ROOM_RNUM((stomach_room = real_room(ROL_TARRASQUE_STOMACH_ROOM_VNUM))))
    return FALSE;

  loot_vnum = rol_tarrasque_loot_vnum_for_roll(rand_number(1, 20));
  corpse = read_object(ROL_TARRASQUE_CORPSE_VNUM, VIRTUAL);
  loot = read_object(loot_vnum, VIRTUAL);
  portal = read_object(ROL_TARRASQUE_EXIT_PORTAL_VNUM, VIRTUAL);
  if (corpse == NULL || loot == NULL || portal == NULL)
  {
    log("SYSERR: RoL Tarrasque death could not load corpse %d, loot %d, or portal %d",
        ROL_TARRASQUE_CORPSE_VNUM, loot_vnum, ROL_TARRASQUE_EXIT_PORTAL_VNUM);
    rol_tarrasque_extract_death_objects(corpse, loot, portal);
    return FALSE;
  }

  GET_OBJ_TYPE(portal) = ITEM_PORTAL;
  GET_OBJ_VAL(portal, 0) = PORTAL_NORMAL;
  GET_OBJ_VAL(portal, 1) = GET_ROOM_VNUM(death_room);
  GET_OBJ_VAL(portal, 2) = GET_ROOM_VNUM(death_room);

  act("With a HUGE crash, the legendary Tarrasque falls to the ground!", FALSE, mob, NULL, NULL,
      TO_ROOM);
  obj_to_room(corpse, death_room);
  obj_to_room(loot, stomach_room);
  obj_to_room(portal, stomach_room);
  (void)call_magic(mob, NULL, NULL, SPELL_EARTHQUAKE, 0, ROL_TARRASQUE_DEATH_LEVEL, CAST_INNATE);
  return TRUE;
}

static int rol_tarrasque_corpse_enter(struct spec_event_context *context, struct obj_data *corpse)
{
  struct char_data *actor;
  room_rnum stomach_room;

  if (context == NULL || corpse == NULL || (actor = context->actor) == NULL || IS_NPC(actor) ||
      !rol_tarrasque_command_is(context->command, "enter") ||
      !rol_tarrasque_corpse_keyword(context->argument, corpse->name) ||
      !VALID_ROOM_RNUM(IN_ROOM(actor)) || IN_ROOM(corpse) != IN_ROOM(actor) ||
      !VALID_ROOM_RNUM((stomach_room = real_room(ROL_TARRASQUE_STOMACH_ROOM_VNUM))))
    return FALSE;

  act("$n crawls into the Tarrasque's icky, disgusting corpse.", TRUE, actor, corpse, NULL,
      TO_ROOM);
  act("You crawl through disgusting goo and enter the inside of the Tarrasque!", FALSE, actor,
      corpse, NULL, TO_CHAR);
  char_from_room(actor);
  char_to_room(actor, stomach_room);
  act("$n slowly crawls in from outside.", TRUE, actor, NULL, NULL, TO_ROOM);
  return TRUE;
}

int rol_tarrasque(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  /* Typed dispatch supplies the exact owner and event. */
  return FALSE;
}

int rol_tarrasque_typed(struct spec_event_context *context)
{
  struct char_data *mob;
  struct obj_data *obj;

  if (context == NULL)
    return FALSE;

  if (context->owner_type == SPEC_OWNER_MOBILE)
  {
    mob = context->owner;
    switch (context->event)
    {
    case SPEC_EVENT_MOBILE_ACTIVITY:
      return rol_tarrasque_activity(mob);
    case SPEC_EVENT_MOBILE_COMBAT_TURN:
      return rol_tarrasque_combat_turn(mob);
    case SPEC_EVENT_MOBILE_DEATH:
      return rol_tarrasque_death(mob);
    default:
      return FALSE;
    }
  }

  if (context->owner_type != SPEC_OWNER_OBJECT || (obj = context->owner) == NULL ||
      !VALID_OBJ_RNUM(obj))
    return FALSE;

  switch (GET_OBJ_VNUM(obj))
  {
  case ROL_TARRASQUE_STOMACH_ACID_VNUM:
    if (context->event == SPEC_EVENT_OBJECT_AUTOMATIC)
      return rol_tarrasque_apply_stomach_acid(obj);
    break;
  case ROL_TARRASQUE_CORPSE_VNUM:
    if (context->event == SPEC_EVENT_COMMAND)
      return rol_tarrasque_corpse_enter(context, obj);
    break;
  default:
    break;
  }

  return FALSE;
}
