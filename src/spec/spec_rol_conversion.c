/**
 * @file spec/spec_rol_conversion.c
 * Shared adapters for active Realms of Luminari special procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "combat/fight.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "magic/spells.h"
#include "mudlim.h"
#include "spec_combat.h"
#include "spec_context.h"
#include "spec_rol_conversion.h"

#define ROL_GATE_MAX_SUMMONS 5

struct rol_gate_recipe
{
  const char *alias;
  int family_flag;
  int chance;
  int minimum;
  int maximum;
  int cooldown_seconds;
  const char *summons[6];
};

/* These recipes preserve the source aliases, attempt cooldowns, success ranges,
 * and summon families. Recipes whose source branches could summon several
 * independent groups use one bounded mixed group in the target. */
static const struct rol_gate_recipe rol_gate_recipes[] = {
    {"babau", MOB_ROL_DEMON, 40, 1, 3, SECS_PER_MUD_DAY, {"babau", "cambion", NULL}},
    {"balor",
     MOB_ROL_DEMON,
     100,
     1,
     4,
     SECS_PER_MUD_HOUR / 2,
     {"balor", "glabrezu", "hezrou", "marilith", "nalfeshnee", "vrock"}},
    {"bar-lgura", MOB_ROL_DEMON, 36, 1, 3, SECS_PER_MUD_DAY, {"bar-lgura", NULL}},
    {"chasme",
     MOB_ROL_DEMON,
     40,
     1,
     4,
     SECS_PER_MUD_HOUR * 2,
     {"manes", "cambion", "chasme", NULL}},
    {"dretch", MOB_ROL_DEMON, 50, 1, 3, SECS_PER_MUD_DAY, {"dretch", NULL}},
    {"glabrezu", MOB_ROL_DEMON, 50, 1, 1, SECS_PER_MUD_DAY, {"babau", "chasme", "nabassu", NULL}},
    {"hezrou",
     MOB_ROL_DEMON,
     35,
     1,
     4,
     SECS_PER_MUD_HOUR,
     {"balor", "glabrezu", "hezrou", "marilith", "nalfeshnee", "vrock"}},
    {"marilith",
     MOB_ROL_DEMON,
     36,
     1,
     4,
     SECS_PER_MUD_HOUR / 2,
     {"babau", "chasme", "nabassu", "cambion", "dretch", NULL}},
    {"molydeus",
     MOB_ROL_DEMON,
     36,
     1,
     3,
     SECS_PER_MUD_HOUR / 2,
     {"molydeus", "chasme", "babau", NULL}},
    {"nabassu",
     MOB_ROL_DEMON,
     46,
     1,
     4,
     SECS_PER_MUD_HOUR * 2,
     {"nabassu", "cambion", "manes", NULL}},
    {"nalfeshnee", MOB_ROL_DEMON, 50, 1, 3, SECS_PER_MUD_HOUR * 2, {"vrock", "babau", NULL}},
    {"rutterkin",
     MOB_ROL_DEMON,
     50,
     1,
     3,
     SECS_PER_MUD_DAY,
     {"dretch", "manes", "rutterkin", NULL}},
    {"succubus", MOB_ROL_DEMON, 40, 1, 1, SECS_PER_MUD_HOUR * 2, {"balor", NULL}},
    {"incubus", MOB_ROL_DEMON, 40, 1, 1, SECS_PER_MUD_HOUR * 2, {"balor", NULL}},
    {"vrock", MOB_ROL_DEMON, 50, 1, 4, SECS_PER_MUD_DAY, {"nalfeshnee", "manes", NULL}},
    {"abishai", MOB_ROL_DEVIL, 45, 1, 3, SECS_PER_MUD_DAY, {"abishai", "lemure", NULL}},
    {"amnizu", MOB_ROL_DEVIL, 40, 1, 3, SECS_PER_MUD_DAY, {"abishai", "erinyes", NULL}},
    {"barbazu", MOB_ROL_DEVIL, 43, 1, 3, SECS_PER_MUD_DAY, {"abishai", "barbazu", NULL}},
    {"cornugon",
     MOB_ROL_DEVIL,
     60,
     1,
     5,
     SECS_PER_MUD_DAY,
     {"barbazu", "abishai", "cornugon", NULL}},
    {"erinyes", MOB_ROL_DEVIL, 43, 1, 4, SECS_PER_MUD_DAY, {"spinagon", "barbazu", NULL}},
    {"gelugon", MOB_ROL_DEVIL, 60, 1, 3, SECS_PER_MUD_DAY, {"barbazu", "abishai", NULL}},
    {"hamatula", MOB_ROL_DEVIL, 43, 1, 3, SECS_PER_MUD_DAY, {"abishai", "hamatula", NULL}},
    {"osyluth", MOB_ROL_DEVIL, 43, 1, 4, SECS_PER_MUD_DAY, {"nupperibo", "osyluth", NULL}},
    {"fiend",
     MOB_ROL_DEVIL,
     100,
     1,
     2,
     SECS_PER_MUD_HOUR / 2,
     {"amnizu", "cornugon", "gelugon", "abishai", "barbazu", NULL}},
    {"spinagon", MOB_ROL_DEVIL, 36, 1, 3, SECS_PER_MUD_DAY, {"spinagon", NULL}},
    {NULL, 0, 0, 0, 0, 0, {NULL}},
};

static const struct rol_gate_recipe *rol_gate_recipe_for(const struct char_data *ch)
{
  const struct rol_gate_recipe *recipe;

  if (ch == NULL || !IS_NPC(ch) || GET_NAME(ch) == NULL || isname("nogate", GET_NAME(ch)))
    return NULL;

  for (recipe = rol_gate_recipes; recipe->alias != NULL; recipe++)
    if (MOB_FLAGGED(ch, recipe->family_flag) && isname(recipe->alias, GET_NAME(ch)))
      return recipe;

  return NULL;
}

static mob_rnum rol_gate_template(const char *alias, int family_flag)
{
  mob_rnum rnum;
  struct char_data *prototype;

  for (rnum = 0; rnum <= top_of_mobt; rnum++)
  {
    prototype = mob_proto + rnum;
    if (MOB_FLAGGED(prototype, family_flag) && GET_NAME(prototype) != NULL &&
        isname("nogate", GET_NAME(prototype)) && isname(alias, GET_NAME(prototype)))
      return rnum;
  }

  return NOBODY;
}

static void rol_purge_gated_inventory(struct char_data *ch)
{
  struct obj_data *obj;
  int wear;

  while (ch->carrying != NULL)
  {
    obj = ch->carrying;
    obj_from_char(obj);
    extract_obj(obj);
  }
  for (wear = 0; wear < NUM_WEARS; wear++)
    if (GET_EQ(ch, wear) != NULL)
      extract_obj(unequip_char(ch, wear));
}

static void rol_gate_one(struct char_data *ch, const char *alias, int family_flag)
{
  struct char_data *summoned;
  mob_rnum rnum;

  if ((rnum = rol_gate_template(alias, family_flag)) == NOBODY)
  {
    log("SYSERR: RoL gate template '%s' is unavailable for mobile %d", alias, GET_MOB_VNUM(ch));
    return;
  }
  if ((summoned = read_mobile(rnum, REAL)) == NULL)
    return;

  char_to_room(summoned, IN_ROOM(ch));
  summoned->mob_specials.rol_gated_creature = true;
  summoned->mob_specials.rol_gate_expire_at = time(NULL) + (4 * SECS_PER_MUD_HOUR);
  act("With an arcane motion, $n gates in $N!", FALSE, ch, NULL, summoned, TO_ROOM);

  if (!isname("rutterkin", GET_NAME(summoned)))
  {
    if (GROUP(ch) == NULL)
      create_group(ch);
    add_follower(summoned, ch);
    if (GROUP(ch) != NULL && GROUP(summoned) == NULL)
      join_group(summoned, GROUP(ch));
  }

  if (FIGHTING(ch) != NULL && FIGHTING(summoned) == NULL)
    set_fighting(summoned, FIGHTING(ch));
}

static void rol_attempt_planar_gate(struct char_data *ch)
{
  const struct rol_gate_recipe *recipe;
  const char *alias;
  int chance;
  int count;
  int option_count;
  int index;
  time_t now;

  if (ch == NULL || ch->mob_specials.rol_gated_creature ||
      (ch->master != NULL && !IS_NPC(ch->master)))
    return;
  if (rand_number(0, 5) != 0 || (recipe = rol_gate_recipe_for(ch)) == NULL)
    return;

  now = time(NULL);
  if (ch->mob_specials.rol_gate_cooldown_until > now)
    return;
  ch->mob_specials.rol_gate_cooldown_until = now + recipe->cooldown_seconds;

  chance = recipe->chance;
  if (ch->master != NULL)
    chance /= 2;
  if (rand_number(0, 99) >= chance)
    return;

  for (option_count = 0; recipe->summons[option_count] != NULL; option_count++)
    ;
  count = rand_number(recipe->minimum, recipe->maximum);
  count = MIN(count, ROL_GATE_MAX_SUMMONS);
  for (index = 0; index < count; index++)
  {
    alias = recipe->summons[rand_number(0, option_count - 1)];
    rol_gate_one(ch, alias, recipe->family_flag);
  }
}

static obj_rnum rol_umberhulk_claws_template(void)
{
  obj_rnum rnum;

  for (rnum = 0; rnum <= top_of_objt; rnum++)
    if (obj_proto[rnum].name != NULL && strcmp(obj_proto[rnum].name, "claws") == 0)
      return rnum;
  return NOTHING;
}

static void rol_equip_umberhulk_claws(struct char_data *ch)
{
  struct obj_data *claws;
  obj_rnum rnum;

  if (GET_EQ(ch, WEAR_WIELD_1) != NULL || (rnum = rol_umberhulk_claws_template()) == NOTHING)
    return;
  if ((claws = read_object(rnum, REAL)) == NULL)
    return;
  equip_char(ch, claws, WEAR_WIELD_1);
}

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

int rol_umberhulk_proc_chance(int level)
{
  return MIN(100, MAX(0, (level * 17) / 10));
}

int rol_planar_gate_cooldown_seconds(const struct char_data *ch)
{
  const struct rol_gate_recipe *recipe = rol_gate_recipe_for(ch);

  return recipe != NULL ? recipe->cooldown_seconds : 0;
}

bool rol_automatic_race_activity(struct char_data *ch)
{
  if (ch == NULL || !IS_NPC(ch))
    return false;

  if (ch->mob_specials.rol_gated_creature && ch->mob_specials.rol_gate_expire_at > 0 &&
      ch->mob_specials.rol_gate_expire_at <= time(NULL))
  {
    act("$n disappears in a cloud of acrid black smoke.", FALSE, ch, NULL, NULL, TO_ROOM);
    rol_purge_gated_inventory(ch);
    extract_char(ch);
    return true;
  }

  if (MOB_FLAGGED(ch, MOB_ROL_UMBERHULK))
    rol_equip_umberhulk_claws(ch);

  return false;
}

void rol_automatic_race_combat_turn(struct char_data *ch)
{
  struct char_data *victim;
  int effect;

  if (ch == NULL || !IS_NPC(ch) || (victim = FIGHTING(ch)) == NULL)
    return;

  if (MOB_FLAGGED(ch, MOB_ROL_DEMON) || MOB_FLAGGED(ch, MOB_ROL_DEVIL))
    rol_attempt_planar_gate(ch);

  if (!MOB_FLAGGED(ch, MOB_ROL_UMBERHULK) ||
      rand_number(0, 100) > rol_umberhulk_proc_chance(GET_LEVEL(ch)))
    return;

  effect = rand_number(0, 8);
  if (effect < 2 && !IS_PET(victim))
  {
    act("$n focuses $s many eyes on $N, clouding $S thoughts!", TRUE, ch, NULL, victim, TO_NOTVICT);
    act("$n focuses $s many eyes on you, clouding your thoughts!", TRUE, ch, NULL, victim, TO_VICT);
    call_magic(ch, victim, NULL, SPELL_CONFUSION, 0, GET_LEVEL(ch), CAST_INNATE);
  }
  else
  {
    act("$n snaps at $N with crushing mandibles!", TRUE, ch, NULL, victim, TO_NOTVICT);
    hit(ch, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
  }
}

bool rol_handle_conjured_death(struct char_data *ch)
{
  const char *message = NULL;

  if (ch == NULL || !IS_NPC(ch))
    return false;

  if (MOB_FLAGGED(ch, MOB_ROL_FADE_FAMILIAR))
    message = "$n slowly fades away into the netherworld...";
  else if (MOB_FLAGGED(ch, MOB_ROL_FADE_MOUNT))
    message = "$n vanishes in a puff of white smoke!";
  else if (MOB_FLAGGED(ch, MOB_ROL_FADE_MONSTER))
    message = "$n disappears in a flash of bright light!";

  if (message == NULL)
    return false;

  act(message, FALSE, ch, NULL, NULL, TO_ROOM);
  return true;
}

static bool rol_breath_ready(struct char_data *ch)
{
  if (ch == NULL || !IS_NPC(ch) || FIGHTING(ch) == NULL)
    return false;

  ch->mob_specials.proc_fired = (ch->mob_specials.proc_fired + 1) % 4;
  return ch->mob_specials.proc_fired == 0;
}

static int rol_breath_weapon(struct char_data *ch, int spell)
{
  if (!rol_breath_ready(ch))
    return FALSE;

  call_magic(ch, NULL, NULL, spell, 0, GET_LEVEL(ch), CAST_INNATE);
  return FALSE;
}

static int rol_breath_attack(struct char_data *ch, int damage_type, const char *self_message,
                             const char *victim_message, const char *room_message)
{
  struct char_data *victim;
  struct spec_damage_result result;
  int dice_count;

  if (!rol_breath_ready(ch) || (victim = FIGHTING(ch)) == NULL)
    return FALSE;

  act(self_message, FALSE, ch, NULL, victim, TO_CHAR);
  act(victim_message, FALSE, ch, NULL, victim, TO_VICT);
  act(room_message, FALSE, ch, NULL, victim, TO_NOTVICT);
  dice_count = MAX(1, GET_LEVEL(ch) / 2);
  result = spec_damage_current_target(ch, victim, dice(dice_count, 6), -1, damage_type, FALSE);
  return result.status == SPEC_DAMAGE_TARGET_INVALIDATED;
}

int rol_breath_weapon_fire(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_weapon(ch, SPELL_FIRE_BREATHE);
}

int rol_breath_weapon_cold(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_weapon(ch, SPELL_FROST_BREATHE);
}

int rol_breath_weapon_acid(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_weapon(ch, SPELL_ACID_BREATHE);
}

int rol_breath_weapon_gas(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_weapon(ch, SPELL_GAS_BREATHE);
}

int rol_breath_weapon_lightning(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_weapon(ch, SPELL_LIGHTNING_BREATHE);
}

int rol_breath_attack_acid(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_attack(ch, DAM_ACID, "You spray \tLacid\tn at $N!",
                           "$n sprays \tLacid\tn at you!", "$n sprays \tLacid\tn at $N!");
}

int rol_breath_attack_lightning(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_attack(ch, DAM_ELECTRIC, "You breathe \tBlightning\tn at $N!",
                           "$n breathes \tBlightning\tn at you!",
                           "$n breathes \tBlightning\tn at $N!");
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

bool rol_update_mobile_home_after_move(struct char_data *ch, int source_room, int destination_room)
{
  if (ch == NULL || !IS_NPC(ch) || !VALID_ROOM_RNUM(source_room) ||
      !VALID_ROOM_RNUM(destination_room) || !ROOM_FLAGGED(source_room, ROOM_ROL_HOME_RESET))
    return false;

  GET_MOB_LOADROOM(ch) = destination_room;
  return true;
}
