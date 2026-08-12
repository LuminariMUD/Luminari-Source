/**
 * @file spec/spec_rol_conversion.c
 * Shared adapters for active Realms of Luminari special procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "character/evolutions.h"
#include "combat/fight.h"
#include "comm.h"
#include "db.h"
#include "dgscript/dg_scripts.h"
#include "handler.h"
#include "interpreter.h"
#include "magic/domains_schools.h"
#include "magic/spells.h"
#include "mob/mob_utils.h"
#include "mud_event.h"
#include "mudlim.h"
#include "spec_combat.h"
#include "spec_context.h"
#include "spec_rol_conversion.h"
#include "spec_rol_totem.h"

#include <limits.h>

#define ROL_GATE_MAX_SUMMONS 5
#define ROL_GUILD_CLASS(class_id) (1ULL << (class_id))
#define ROL_GUILD_RACE(race_id) (1ULL << (race_id))
#define ROL_MAJOR_BEHOLDER_EYES 10
#define ROL_MAJOR_BEHOLDER_COOLDOWN_BITS 2
#define ROL_MAJOR_BEHOLDER_COOLDOWN_MASK 3U
#define ROL_MAJOR_BEHOLDER_COOLDOWN_ROUNDS 3

struct rol_guild_guard_rule
{
  int room_vnum;
  int direction;
  unsigned long long class_mask;
  unsigned long long race_mask;
  bool protects;
};

/* Only rooms reached by active converted guild_guard bindings are retained.
 * Target VNUMs are the source room VNUMs under the Phase 4 +2,000,000 offset. */
static const struct rol_guild_guard_rule rol_guild_guard_rules[] = {
    {2004128, NORTH, 0, 0, false},
    {2008014, SOUTH, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2008044, EAST, 0, 0, false},
    {2008046, EAST, 0, 0, false},
    {2008053, WEST, 0, 0, false},
    {2008070, WEST, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2008087, EAST, 0, ROL_GUILD_RACE(RACE_ELF) | ROL_GUILD_RACE(RACE_HALF_ELF), false},
    {2008113, SOUTH, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2008137, SOUTH, ROL_GUILD_CLASS(CLASS_DRUID), 0, true},
    {2008200, WEST, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2008305, EAST, ROL_GUILD_CLASS(CLASS_RANGER), 0, true},
    {2008311, SOUTH, ROL_GUILD_CLASS(CLASS_NECROMANCER), 0, true},
    {2008318, NORTH, ROL_GUILD_CLASS(CLASS_BARD), 0, true},
    {2011603, WEST, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2011633, WEST, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2011685, EAST, 0, 0, false},
    {2011812, UP, 0, 0, false},
    {2015314, NORTH, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2015333, NORTH, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2015506, NORTH, ROL_GUILD_CLASS(CLASS_BERSERKER), 0, true},
    {2015660, SOUTH, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2016007, WEST, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2016056, NORTH, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2016145, EAST, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2016192, NORTH, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2016283, SOUTH, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2016383, SOUTH, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2016392, SOUTH, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2016408, NORTH, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2019950, SOUTH, 0, 0, false},
    {2019951, SOUTH, 0, 0, false},
    {2019954, SOUTH, 0, 0, false},
    {2025001, NORTH, 0, 0, false},
    {2025201, NORTH, 0, 0, false},
    {2034367, SOUTH, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2034406, WEST, ROL_GUILD_CLASS(CLASS_ASSASSIN) | ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2034406, EAST, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2050624, WEST, 0, 0, true},
    {2066028, SOUTH, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2066065, WEST, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2066078, SOUTH, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2066084, NORTH, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2066088, EAST, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2090847, SOUTH, 0, 0, true},
    {2090849, EAST, 0, 0, true},
};

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
  else if (MOB_FLAGGED(ch, MOB_ROL_TOTEM_SPIRIT))
  {
    message = rol_totem_spirit_death_message(GET_MOB_VNUM(ch));
    if (message == NULL)
      message = "$n quickly fades back into the spirit world...";
  }

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

int rol_magic_pool(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;
  room_rnum destination;
  char name[MAX_INPUT_LENGTH];
  int damage_amount;

  if (ch == NULL || obj == NULL || argument == NULL || !cmd || !CMD_IS("enter"))
    return FALSE;

  one_argument(argument, name, sizeof(name));
  if (!*name || obj->name == NULL || !isname(name, obj->name))
    return FALSE;

  destination = real_room(GET_OBJ_VAL(obj, 0));
  if (!VALID_ROOM_RNUM(destination))
  {
    send_to_char(ch, "The pool leads nowhere. Please tell a staff member.\r\n");
    log("SYSERR: RoL magic pool object %d has invalid destination %d", GET_OBJ_VNUM(obj),
        GET_OBJ_VAL(obj, 0));
    return TRUE;
  }

  act("As you step into $p, there is a blinding flash of light!", FALSE, ch, obj, NULL, TO_CHAR);
  send_to_char(ch, "You are ripped through a dark and star-filled void; pain sears through\r\n"
                   "your body. When you open your eyes, you are elsewhere...\r\n");
  act("$n wades into $p.", FALSE, ch, obj, NULL, TO_ROOM);

  damage_amount = MAX(0, GET_OBJ_VAL(obj, 1));
  if (GET_LEVEL(ch) < LVL_IMMORT)
    GET_HIT(ch) = MAX(0, GET_HIT(ch) - damage_amount);

  act("$n slowly fades out of existence.", FALSE, ch, NULL, NULL, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, destination);
  act("$n slowly fades into existence.", FALSE, ch, NULL, NULL, TO_ROOM);
  return TRUE;
}

static room_rnum rol_random_room_in_zone(zone_rnum zone)
{
  room_rnum room;
  int room_count = 0;
  int selected;

  if (zone == NOWHERE || zone > top_of_zone_table)
    return NOWHERE;

  for (room = 0; room <= top_of_world; room++)
    if (world[room].zone == zone)
      room_count++;

  if (room_count == 0)
    return NOWHERE;

  selected = rand_number(0, room_count - 1);
  for (room = 0; room <= top_of_world; room++)
  {
    if (world[room].zone != zone)
      continue;
    if (selected-- == 0)
      return room;
  }

  return NOWHERE;
}

int rol_auto_distributor(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct room_data *room = me;
  room_rnum destination;
  zone_rnum zone;

  UNUSED(cmd);
  UNUSED(argument);

  if (ch == NULL || room == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;
  if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  zone = world[IN_ROOM(ch)].zone;
  destination = rol_random_room_in_zone(zone);
  if (!VALID_ROOM_RNUM(destination))
  {
    send_to_char(ch, "The distributing magic fails. Please tell a staff member.\r\n");
    log("SYSERR: RoL auto distributor room %d has no valid destination in zone %d", room->number,
        zone);
    return TRUE;
  }

  act("$n slowly fades out of existence.", FALSE, ch, NULL, NULL, TO_ROOM);
  char_from_room(ch);
  if (ZONE_FLAGGED(world[destination].zone, ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[destination].coords[0];
    Y_LOC(ch) = world[destination].coords[1];
  }
  char_to_room(ch, destination);
  act("$n enters.", FALSE, ch, NULL, NULL, TO_ROOM);
  return TRUE;
}

static bool rol_guild_guard_has_class(const struct char_data *ch, unsigned long long class_mask)
{
  int class_id;

  if (ch == NULL || class_mask == 0)
    return false;

  if (IS_NPC(ch))
    return GET_CLASS(ch) >= 0 && GET_CLASS(ch) < 64 &&
           (class_mask & ROL_GUILD_CLASS(GET_CLASS(ch))) != 0;

  for (class_id = 0; class_id < MAX_CLASSES && class_id < 64; class_id++)
    if ((class_mask & ROL_GUILD_CLASS(class_id)) != 0 && CLASS_LEVEL(ch, class_id) > 0)
      return true;

  return false;
}

bool rol_guild_guard_allows(int room_vnum, int direction, const struct char_data *ch)
{
  const struct rol_guild_guard_rule *rule;
  size_t rule_index;

  for (rule_index = 0;
       rule_index < sizeof(rol_guild_guard_rules) / sizeof(rol_guild_guard_rules[0]); rule_index++)
  {
    rule = &rol_guild_guard_rules[rule_index];
    if (rule->room_vnum != room_vnum || rule->direction != direction)
      continue;

    if (rule->class_mask != 0)
      return rol_guild_guard_has_class(ch, rule->class_mask);
    if (rule->race_mask != 0)
      return ch != NULL && GET_RACE(ch) >= 0 && GET_RACE(ch) < 64 &&
             (rule->race_mask & ROL_GUILD_RACE(GET_RACE(ch))) != 0;
    return false;
  }

  return true;
}

bool rol_guild_guard_protects(int room_vnum)
{
  size_t rule_index;

  for (rule_index = 0;
       rule_index < sizeof(rol_guild_guard_rules) / sizeof(rol_guild_guard_rules[0]); rule_index++)
    if (rol_guild_guard_rules[rule_index].room_vnum == room_vnum &&
        rol_guild_guard_rules[rule_index].protects)
      return true;

  return false;
}

static room_rnum rol_guild_guard_teleport_destination(struct char_data *victim)
{
  room_rnum room;
  room_rnum selected = NOWHERE;
  zone_rnum zone;
  int eligible = 0;

  if (victim == NULL || !VALID_ROOM_RNUM(IN_ROOM(victim)))
    return NOWHERE;

  zone = world[IN_ROOM(victim)].zone;
  for (room = 0; room <= top_of_world; room++)
  {
    if (room == IN_ROOM(victim) || world[room].zone != zone ||
        !valid_mortal_tele_dest(victim, room, true))
      continue;

    eligible++;
    if (rand_number(1, eligible) == 1)
      selected = room;
  }

  return selected;
}

static void rol_guild_guard_stop_victim_combat(struct char_data *victim)
{
  struct char_data *fighter;
  struct char_data *next;

  if (victim == NULL)
    return;

  if (FIGHTING(victim) != NULL)
    stop_fighting(victim);

  for (fighter = combat_list; fighter != NULL; fighter = next)
  {
    next = fighter->next_fighting;
    if (FIGHTING(fighter) == victim)
      stop_fighting(fighter);
  }
}

static int rol_guild_guard_protection(struct char_data *guard, struct char_data *victim)
{
  room_rnum destination;
  long loss;

  if (guard == NULL || victim == NULL || IS_NPC(victim) ||
      spec_context_validate_combat_target(guard, victim, true) != SPEC_CONTEXT_VALID)
    return FALSE;

  act("$n says, 'Begone from here, outlaw! None may attack guild guardians!'", FALSE, guard, NULL,
      victim, TO_ROOM);
  act("$n presses a small metal pin on $s chest, which flares with brilliant blue light!", FALSE,
      guard, NULL, victim, TO_ROOM);
  send_to_char(victim, "A wrenching pain drains your life force away!\r\n");

  loss = MIN((long)GET_LEVEL(victim) * 5000L, MAX(0L, GET_EXP(victim) - 2L));
  GET_EXP(victim) -= loss;

  call_magic(guard, victim, NULL, SPELL_DISPEL_MAGIC, 0, 60, CAST_INNATE);
  call_magic(guard, victim, NULL, SPELL_CURSE, 0, 60, CAST_INNATE);
  if (!affected_by_spell(victim, SPELL_POISON))
    call_magic(guard, victim, NULL, SPELL_POISON, 0, 120, CAST_INNATE);
  call_magic(guard, victim, NULL, SPELL_BLINDNESS, 0, 60, CAST_INNATE);
  call_magic(guard, victim, NULL, SPELL_SLOW, 0, 60, CAST_INNATE);

  if (GET_POS(victim) <= POS_DEAD || !VALID_ROOM_RNUM(IN_ROOM(victim)))
    return TRUE;

  GET_HIT(victim) = 1;
  update_pos(victim);
  destination = rol_guild_guard_teleport_destination(victim);
  rol_guild_guard_stop_victim_combat(victim);

  if (!VALID_ROOM_RNUM(destination))
    return TRUE;

  act("$n slowly fades out of existence.", FALSE, victim, NULL, NULL, TO_ROOM);
  char_from_room(victim);
  if (ZONE_FLAGGED(world[destination].zone, ZONE_WILDERNESS))
  {
    X_LOC(victim) = world[destination].coords[0];
    Y_LOC(victim) = world[destination].coords[1];
  }
  char_to_room(victim, destination);
  act("$n slowly fades into existence.", FALSE, victim, NULL, NULL, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
  return TRUE;
}

int rol_guild_guard(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *guard = me;
  int current_room_vnum;
  int direction;

  UNUSED(argument);

  if (guard == NULL || !IS_NPC(guard) || !VALID_ROOM_RNUM(IN_ROOM(guard)) ||
      GET_MOB_LOADROOM(guard) != IN_ROOM(guard))
    return FALSE;

  current_room_vnum = GET_ROOM_VNUM(IN_ROOM(guard));
  if (cmd == 0)
  {
    if (rol_guild_guard_protects(current_room_vnum) && FIGHTING(guard) != NULL)
      return rol_guild_guard_protection(guard, FIGHTING(guard));
    return FALSE;
  }

  if (ch == NULL || complete_cmd_info == NULL || !IS_MOVE(cmd))
    return FALSE;
  if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_GUARD))
    return FALSE;

  direction = complete_cmd_info[cmd].subcmd;
  if (rol_guild_guard_allows(current_room_vnum, direction, ch))
    return FALSE;

  act("$n humiliates you, and blocks your way.", FALSE, guard, NULL, ch, TO_VICT);
  act("$n humiliates $N, and blocks $S way.", FALSE, guard, NULL, ch, TO_NOTVICT);
  return TRUE;
}

int rol_major_beholder_eye_spell(int eye)
{
  static const int eye_spells[ROL_MAJOR_BEHOLDER_EYES] = {
      SPELL_FIREBALL,
      SPELL_ACID_ARROW,
      SPELL_SLOW,
      SPELL_RAY_OF_ENFEEBLEMENT,
      PSIONIC_WITHER,
      SPELL_DISPEL_MAGIC,
      SPELL_PRISMATIC_SPRAY,
      SPELL_HOLD_MONSTER,
      SPELL_HARM,
      SPELL_FINGER_OF_DEATH,
  };

  if (eye < 0 || eye >= ROL_MAJOR_BEHOLDER_EYES)
    return -1;
  return eye_spells[eye];
}

int rol_major_beholder_eye_cooldown(int state, int eye)
{
  unsigned int shift;

  if (eye < 0 || eye >= ROL_MAJOR_BEHOLDER_EYES)
    return -1;
  shift = (unsigned int)eye * ROL_MAJOR_BEHOLDER_COOLDOWN_BITS;
  return (int)(((unsigned int)state >> shift) & ROL_MAJOR_BEHOLDER_COOLDOWN_MASK);
}

static int rol_major_beholder_set_cooldown(int state, int eye, int rounds)
{
  unsigned int encoded;
  unsigned int shift;

  if (eye < 0 || eye >= ROL_MAJOR_BEHOLDER_EYES)
    return state;

  shift = (unsigned int)eye * ROL_MAJOR_BEHOLDER_COOLDOWN_BITS;
  encoded = (unsigned int)state & ~(ROL_MAJOR_BEHOLDER_COOLDOWN_MASK << shift);
  encoded |= ((unsigned int)MIN(ROL_MAJOR_BEHOLDER_COOLDOWN_ROUNDS, MAX(0, rounds)) << shift);
  return (int)encoded;
}

int rol_major_beholder_advance_cooldowns(int state, unsigned int fired_eye_mask)
{
  int cooldown;
  int eye;

  for (eye = 0; eye < ROL_MAJOR_BEHOLDER_EYES; eye++)
  {
    cooldown = rol_major_beholder_eye_cooldown(state, eye);
    if ((fired_eye_mask & (1U << eye)) != 0)
      cooldown = ROL_MAJOR_BEHOLDER_COOLDOWN_ROUNDS;
    else if (cooldown > 0)
      cooldown--;
    state = rol_major_beholder_set_cooldown(state, eye, cooldown);
  }
  return state;
}

static struct char_data *rol_major_beholder_target(struct char_data *ch)
{
  struct char_data *target;
  int target_count = 0;

  target = npc_find_target(ch, &target_count);
  if (target == NULL)
    target = FIGHTING(ch);

  if (target != NULL && IS_PET(target) && target->master != NULL &&
      IN_ROOM(target->master) == IN_ROOM(target))
    target = target->master;

  if (spec_context_validate_combat_target(ch, target, false) != SPEC_CONTEXT_VALID)
    return NULL;
  return target;
}

static bool rol_major_beholder_mass_dispel(struct char_data *ch)
{
  struct char_data *target;
  struct char_data *next;
  bool cast = false;

  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (target == ch || (IS_NPC(target) && !IS_PET(target)))
      continue;
    call_magic(ch, target, NULL, SPELL_DISPEL_MAGIC, 0, GET_LEVEL(ch), CAST_INNATE);
    cast = true;
  }
  return cast;
}

static bool rol_major_beholder_cast_eye(struct char_data *ch, struct char_data *target, int eye)
{
  static const char *ordinals[ROL_MAJOR_BEHOLDER_EYES] = {
      "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth",
  };
  char message[MAX_INPUT_LENGTH];

  snprintf(message, sizeof(message), "$n fixes $s %s eyestalk upon $N!", ordinals[eye]);
  act(message, FALSE, ch, NULL, target, TO_NOTVICT);
  snprintf(message, sizeof(message), "$n fixes $s %s eyestalk upon you!", ordinals[eye]);
  act(message, FALSE, ch, NULL, target, TO_VICT);

  if (eye == 5)
    return rol_major_beholder_mass_dispel(ch);

  call_magic(ch, target, NULL, rol_major_beholder_eye_spell(eye), 0, GET_LEVEL(ch), CAST_INNATE);
  if (eye == 3 && spec_context_validate_combat_target(ch, target, false) == SPEC_CONTEXT_VALID)
    call_magic(ch, target, NULL, SPELL_FEEBLEMIND, 0, GET_LEVEL(ch), CAST_INNATE);
  return true;
}

int rol_major_beholder(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *target;
  bool fired = false;
  int eye;
  int state;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || !IS_NPC(ch) || cmd || FIGHTING(ch) == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  state = rol_major_beholder_advance_cooldowns(ch->mob_specials.proc_fired, 0);
  for (eye = 0; eye < ROL_MAJOR_BEHOLDER_EYES; eye++)
  {
    if (rol_major_beholder_eye_cooldown(state, eye) != 0 || rand_number(0, 2) != 0)
      continue;
    if ((target = rol_major_beholder_target(ch)) == NULL)
      break;
    if (rol_major_beholder_cast_eye(ch, target, eye))
    {
      state = rol_major_beholder_set_cooldown(state, eye, ROL_MAJOR_BEHOLDER_COOLDOWN_ROUNDS);
      fired = true;
    }
  }

  ch->mob_specials.proc_fired = state;
  return fired;
}

bool rol_lich_energy_drain_together(const struct char_data *candidate,
                                    const struct char_data *primary)
{
  if (candidate == NULL || primary == NULL)
    return false;

  if (candidate == primary || candidate->master == primary || primary->master == candidate)
    return true;
  if (candidate->master != NULL && candidate->master == primary->master)
    return true;
  if (GROUP(candidate) != NULL && GROUP(candidate) == GROUP(primary))
    return true;
  if (candidate->master != NULL && GROUP(candidate->master) != NULL &&
      GROUP(candidate->master) == GROUP(primary))
    return true;
  if (primary->master != NULL && GROUP(primary->master) != NULL &&
      GROUP(candidate) == GROUP(primary->master))
    return true;

  return false;
}

int rol_lich_energy_drain_victim_hit(int current_hit, bool death_warded)
{
  if (current_hit <= 0)
    return current_hit;

  return death_warded ? 0 : -5;
}

int rol_lich_energy_drain_healer_hit(int current_hit, int drained_hit, bool blackmantled)
{
  if (blackmantled || drained_hit <= 0)
    return current_hit;
  if (current_hit > INT_MAX - drained_hit)
    return INT_MAX;

  return current_hit + drained_hit;
}

long rol_lich_energy_drain_stun_duration(long remaining)
{
  long duration = PULSE_VIOLENCE * 2;

  if (remaining <= 0)
    return duration;
  if (remaining > LONG_MAX - duration)
    return LONG_MAX;

  return remaining + duration;
}

static void rol_lich_energy_drain_stun(struct char_data *victim)
{
  struct mud_event_data *stun_event;
  long duration;
  long remaining;

  if (!can_stun(victim))
    return;

  stun_event = char_has_mud_event(victim, eSTUNNED);
  if (stun_event == NULL)
  {
    attach_mud_event(new_mud_event(eSTUNNED, victim, NULL), rol_lich_energy_drain_stun_duration(0));
    return;
  }

  remaining = stun_event->pEvent != NULL ? event_time(stun_event->pEvent) : 0;
  duration = rol_lich_energy_drain_stun_duration(remaining);
  change_event_duration(victim, eSTUNNED, duration);
}

int rol_lich_energy_drain(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *primary;
  struct char_data *victim;
  int drained_hit;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || !IS_NPC(ch) || cmd || IS_CASTING(ch) || (primary = FIGHTING(ch)) == NULL ||
      !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = victim->next_in_room)
  {
    if (GET_HIT(victim) <= 0 ||
        (victim != primary && !rol_lich_energy_drain_together(victim, primary)) ||
        rand_number(0, 4) != 0)
      continue;

    act("\tWYou reach out and suck the life force away from $N!\tn", TRUE, ch, NULL, victim,
        TO_CHAR);
    act("$n \trturns and gazes at you wickedly, and you freeze in place.\tn\r\n"
        "$n \tWreaches out with a skeletal hand and touches you!\tn\r\n"
        "\tWYou scream as your life force flows away from you.\tn",
        FALSE, ch, NULL, victim, TO_VICT);
    act("$n \trturns and gazes at $N, who freezes in place.\tn\r\n"
        "$n \tWreaches out and sucks the life force from $N!\tn",
        TRUE, ch, NULL, victim, TO_NOTVICT);

    drained_hit = GET_HIT(victim);
    GET_HIT(ch) = rol_lich_energy_drain_healer_hit(GET_HIT(ch), drained_hit,
                                                   AFF_FLAGGED(ch, AFF_BLACKMANTLE));
    GET_HIT(victim) =
        rol_lich_energy_drain_victim_hit(drained_hit, AFF_FLAGGED(victim, AFF_DEATH_WARD));
    update_pos(victim);

    rol_lich_energy_drain_stun(victim);
    break;
  }

  /* The source callback deliberately allows the ordinary NPC action to continue. */
  return FALSE;
}

static struct obj_data *rol_bandit_owned_wagon(struct char_data *ch)
{
  struct obj_data *obj;

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return NULL;

  for (obj = world[IN_ROOM(ch)].contents; obj != NULL; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_WAGON && GET_OBJ_VAL(obj, 3) == GET_IDNUM(ch))
      return obj;

  return NULL;
}

int rol_bandit_cargo_value(struct char_data *ch)
{
  struct obj_data *obj;
  struct obj_data *wagon;
  long long total = 0;

  if (ch == NULL)
    return 0;

  for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_RESOURCE)
      total += GET_OBJ_COST(obj);

  wagon = rol_bandit_owned_wagon(ch);
  if (wagon != NULL)
    for (obj = wagon->contains; obj != NULL; obj = obj->next_content)
      total += GET_OBJ_COST(obj);

  return (int)MIN((long long)INT_MAX, MAX(0LL, total));
}

int rol_bandit_fee_gold(int target_vnum, int cargo_value, int alignment, int carried_gold)
{
  long long base_platinum;

  base_platinum = MAX(0, cargo_value) / 1000;
  if (base_platinum == 0)
    return ROL_BANDIT_DEMAND_PASS;

  switch (target_vnum)
  {
  case 2099501:
    return 50;
  case 2099502:
    return (int)MIN((long long)INT_MAX, (base_platinum / 3) * 10);
  case 2099503:
    return (int)MIN((long long)INT_MAX, (base_platinum / 2) * 10);
  case 2099504:
    return (int)MIN((long long)INT_MAX, base_platinum * 10);
  case 2099505:
    return carried_gold > 0 ? carried_gold : ROL_BANDIT_DEMAND_TAKE_WAGON;
  case 2099506:
    if (alignment >= 350)
      return 100;
    if (alignment <= -350)
      return ROL_BANDIT_DEMAND_ATTACK;
    return carried_gold > 0 ? carried_gold : 100;
  case 2099507:
    return ROL_BANDIT_DEMAND_ATTACK;
  default:
    return ROL_BANDIT_DEMAND_PASS;
  }
}

static bool rol_bandit_is_alone(struct char_data *bandit)
{
  if (bandit == NULL || !VALID_ROOM_RNUM(IN_ROOM(bandit)))
    return true;

  return world[IN_ROOM(bandit)].people == bandit && bandit->next_in_room == NULL;
}

static void rol_bandit_vanish(struct char_data *bandit)
{
  rol_purge_gated_inventory(bandit);
  extract_char(bandit);
}

static void rol_bandit_attack(struct char_data *bandit, struct char_data *victim,
                              const char *message)
{
  if (message != NULL)
    do_say(bandit, message, 0, 0);
  hit(bandit, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
}

static bool rol_bandit_take_wagon(struct char_data *bandit, struct char_data *victim)
{
  struct obj_data *wagon;

  wagon = rol_bandit_owned_wagon(victim);
  if (wagon == NULL)
    return false;

  act("$n grabs your wagon.", FALSE, bandit, NULL, victim, TO_VICT);
  act("$n grabs $N's wagon.", TRUE, bandit, NULL, victim, TO_NOTVICT);
  extract_obj(wagon);
  return true;
}

static void rol_bandit_announce_demand(struct char_data *bandit, int target_vnum, int fee_gold)
{
  char message[MAX_INPUT_LENGTH];

  switch (target_vnum)
  {
  case 2099501:
    do_say(bandit, "You have to pay to pass. The toll is 50 gold coins.", 0, 0);
    break;
  case 2099502:
    do_say(bandit, "You had better pay, or your head will fall from your neck!", 0, 0);
    snprintf(message, sizeof(message), "The price for your life is %d gold coins.", fee_gold);
    do_say(bandit, message, 0, 0);
    break;
  case 2099503:
    do_say(bandit, "Have you ever experienced a blade in your belly?", 0, 0);
    snprintf(message, sizeof(message), "If you do not want to, pay me %d gold coins.", fee_gold);
    do_say(bandit, message, 0, 0);
    break;
  case 2099504:
    do_say(bandit, "Life is so dangerous today!", 0, 0);
    snprintf(message, sizeof(message),
             "For example, you will die if you do not hand me %d gold coins.", fee_gold);
    do_say(bandit, message, 0, 0);
    break;
  case 2099505:
    do_say(bandit, "It is a hard life being a merchant!", 0, 0);
    do_say(bandit, "But it is an even worse life being a bandit.", 0, 0);
    do_say(bandit, "Give me all your gold coins and leave your wagon to me.", 0, 0);
    break;
  case 2099506:
    if (fee_gold == 100)
    {
      do_say(bandit, "Poor people need your money more than you do.", 0, 0);
      do_say(bandit, "Pay a 100 gold toll and you will be free.", 0, 0);
    }
    else
    {
      do_say(bandit, "I really dislike people who refuse to take a side.", 0, 0);
      snprintf(message, sizeof(message), "A donation of %d gold coins could redeem you.", fee_gold);
      do_say(bandit, message, 0, 0);
    }
    break;
  }
}

static bool rol_bandit_blocks_command(int cmd)
{
  if (cmd <= 0 || complete_cmd_info == NULL)
    return false;

  return IS_MOVE(cmd) || CMD_IS("flee") || CMD_IS("get");
}

int rol_bandit(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *bandit = me;
  long long paid;
  int before_gold;
  int cargo_value;
  int fee_gold;
  int target_vnum;
  time_t now;

  if (bandit == NULL && cmd == 0)
    bandit = ch;
  if (bandit == NULL || !IS_NPC(bandit))
    return FALSE;

  now = time(NULL);
  if (bandit->mob_specials.rol_bandit_expire_at == 0)
    bandit->mob_specials.rol_bandit_expire_at = now + (10 * SECS_PER_MUD_HOUR);

  if (cmd == 0)
  {
    if (bandit->mob_specials.rol_bandit_expire_at > 0 &&
        now >= bandit->mob_specials.rol_bandit_expire_at)
    {
      bandit->mob_specials.rol_bandit_expire_at = (time_t)-1;
      if (rol_bandit_is_alone(bandit))
        rol_bandit_vanish(bandit);
      return TRUE;
    }
    return FALSE;
  }

  if (ch == NULL || IS_NPC(ch) || !AWAKE(bandit) || FIGHTING(bandit) != NULL ||
      complete_cmd_info == NULL)
    return FALSE;

  if (bandit->mob_specials.rol_bandit_victim_id == GET_IDNUM(ch))
  {
    if (CMD_IS("camp") || CMD_IS("leavecart"))
    {
      rol_bandit_attack(bandit, ch, "Are you trying to swindle me?");
      return TRUE;
    }

    if (CMD_IS("give"))
    {
      before_gold = GET_GOLD(bandit);
      do_give(ch, argument, cmd, 0);
      paid = (long long)GET_GOLD(bandit) - before_gold;
      if (paid < bandit->mob_specials.rol_bandit_fee_gold)
      {
        rol_bandit_attack(bandit, ch, "You are REALLY foolish. Die!");
        return TRUE;
      }

      if (GET_MOB_VNUM(bandit) == 2099505 && !rol_bandit_take_wagon(bandit, ch))
      {
        rol_bandit_attack(bandit, ch, "You promised me a wagon. Die!");
        return TRUE;
      }

      if (GET_MOB_VNUM(bandit) == 2099506)
      {
        do_say(bandit, "That was very nice of you.", 0, 0);
        act("$n bows deeply, then disappears.", FALSE, bandit, NULL, ch, TO_ROOM);
      }
      else
      {
        do_say(bandit, "That was wise of you.", 0, 0);
        act("$n quickly disappears.", FALSE, bandit, NULL, ch, TO_ROOM);
      }
      rol_bandit_vanish(bandit);
      return TRUE;
    }

    if (!rol_bandit_blocks_command(cmd))
      return FALSE;

    if (rand_number(1, 5) == 5)
      rol_bandit_attack(bandit, ch, "I am tired of you. Die!");
    else
    {
      act("$n stops you.", FALSE, bandit, NULL, ch, TO_VICT);
      act("$n stops $N.", TRUE, bandit, NULL, ch, TO_NOTVICT);
    }
    return TRUE;
  }

  if (bandit->mob_specials.rol_bandit_victim_id != 0 || !rol_bandit_blocks_command(cmd))
    return FALSE;

  target_vnum = GET_MOB_VNUM(bandit);
  cargo_value = rol_bandit_cargo_value(ch);
  fee_gold = rol_bandit_fee_gold(target_vnum, cargo_value, GET_ALIGNMENT(ch), GET_GOLD(ch));
  if (fee_gold == ROL_BANDIT_DEMAND_PASS)
    return FALSE;

  act("$n stops you.", FALSE, bandit, NULL, ch, TO_VICT);
  act("$n stops $N.", TRUE, bandit, NULL, ch, TO_NOTVICT);
  bandit->mob_specials.rol_bandit_victim_id = GET_IDNUM(ch);
  bandit->mob_specials.rol_bandit_fee_gold = MAX(0, fee_gold);

  if (fee_gold == ROL_BANDIT_DEMAND_ATTACK)
  {
    rol_bandit_attack(bandit, ch,
                      target_vnum == 2099506 ? "Evil is a malady, and I am the cure." : NULL);
    return TRUE;
  }

  if (fee_gold == ROL_BANDIT_DEMAND_TAKE_WAGON)
  {
    do_say(bandit, "You are terribly poor. I will take your wagon instead.", 0, 0);
    if (!rol_bandit_take_wagon(bandit, ch))
    {
      rol_bandit_attack(bandit, ch, "No wagon either? Die!");
      return TRUE;
    }
    act("$n quickly disappears.", FALSE, bandit, NULL, ch, TO_ROOM);
    rol_bandit_vanish(bandit);
    return TRUE;
  }

  rol_bandit_announce_demand(bandit, target_vnum, fee_gold);
  return TRUE;
}

int rol_shadow_giant_spook_damage(bool save_succeeded)
{
  int amount = dice(25, 8);

  return save_succeeded ? amount / 2 : amount;
}

bool rol_shadow_giant_spook_immune(struct char_data *target)
{
  if (target == NULL)
    return true;

  if (IS_UNDEAD(target) || IS_DRAGON(target))
    return true;

  return IS_NPC(target) &&
         (MOB_FLAGGED(target, MOB_ROL_DEMON) || MOB_FLAGGED(target, MOB_ROL_DEVIL) ||
          MOB_FLAGGED(target, MOB_ROL_ANGEL) || HAS_SUBRACE(target, SUBRACE_ANGEL));
}

bool rol_shadow_giant_stun_succeeds(int level, int chance_roll, int penalty_roll)
{
  return chance_roll < (level * 2) - penalty_roll;
}

static void rol_shadow_giant_spook(struct char_data *ch, struct char_data *target)
{
  bool saved;
  int amount;

  if (rol_shadow_giant_spook_immune(target))
  {
    act("$N laughs as you attempt to spook $M.", TRUE, ch, NULL, target, TO_CHAR);
    return;
  }

  saved = savingthrow(ch, target, SAVING_WILL, 0, CAST_INNATE, 30, ILLUSION);
  amount = rol_shadow_giant_spook_damage(saved);
  damage(ch, target, amount, -1, DAM_MENTAL, FALSE);

  if (GET_POS(target) <= POS_DEAD || !can_stun(target) || char_has_mud_event(target, eSTUNNED) ||
      !rol_shadow_giant_stun_succeeds(GET_LEVEL(ch), rand_number(1, 100), rand_number(1, 5)))
    return;

  attach_mud_event(new_mud_event(eSTUNNED, target, NULL), PULSE_VIOLENCE * rand_number(1, 3));
}

int rol_shadow_giant(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *target;
  struct char_data *next;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || !IS_NPC(ch) || cmd || FIGHTING(ch) == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      rand_number(0, 20) != 0)
    return FALSE;

  act("You pull your face off and scare the bejezus out of $N.", FALSE, ch, NULL, FIGHTING(ch),
      TO_CHAR);
  act("The Shadow Giant reaches up and pulls his face off.", FALSE, ch, NULL, FIGHTING(ch),
      TO_ROOM);

  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (IS_NPC(target) && !IS_PET(target))
      continue;
    rol_shadow_giant_spook(ch, target);
  }

  return FALSE;
}

bool rol_update_mobile_home_after_move(struct char_data *ch, int source_room, int destination_room)
{
  if (ch == NULL || !IS_NPC(ch) || !VALID_ROOM_RNUM(source_room) ||
      !VALID_ROOM_RNUM(destination_room) || !ROOM_FLAGGED(source_room, ROOM_ROL_HOME_RESET))
    return false;

  GET_MOB_LOADROOM(ch) = destination_room;
  return true;
}
