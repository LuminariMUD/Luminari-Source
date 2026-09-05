/**
 * @file spec/spec_rol_pilot.c
 * Bounded adapters for active special procedures in the RoL Phase 4 pilot.
 *
 * These handlers preserve behavior that needs engine combat, item placement,
 * cooldown, or spell hooks. VNUM-dependent behavior is compiled to DG scripts
 * by the world conversion pipeline instead of being embedded here.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "character/evolutions.h"
#include "comm.h"
#include "combat/fight.h"
#include "handler.h"
#include "interpreter.h"
#include "magic/spells.h"
#include "mudlim.h"
#include "spec/spec_combat.h"
#include "spec/spec_context.h"
#include "spec/spec_rol_pilot.h"
#include "point_update_periodic.h"

static bool rol_identify(struct char_data *ch, int cmd, const char *argument,
                         const char *description)
{
  if (ch == NULL || cmd != 0 || argument == NULL || str_cmp(argument, "identify"))
    return false;

  send_to_char(ch, "%s\r\n", description);
  return true;
}

static bool rol_phrase_command(int cmd, const char *argument, const char *phrase)
{
  if (cmd <= 0 || argument == NULL || phrase == NULL)
    return false;
  if (!CMD_IS("say") && !CMD_IS("'"))
    return false;

  skip_spaces_c(&argument);
  return !str_cmp(argument, phrase);
}

static bool rol_worn_combat_object(struct char_data *ch, struct obj_data *obj,
                                   struct char_data **victim)
{
  struct char_data *target;

  if (spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID)
    return false;

  target = FIGHTING(ch);
  if (spec_context_validate_combat_target(ch, target, true) != SPEC_CONTEXT_VALID)
    return false;

  if (victim != NULL)
    *victim = target;
  return true;
}

static bool rol_is_critical(const char *argument)
{
  return argument != NULL && !str_cmp(argument, "critical");
}

static void rol_cast_on_target(struct char_data *ch, struct char_data *victim, int spell)
{
  int level;

  if (ch == NULL || victim == NULL)
    return;

  level = MIN(30, MAX(1, GET_LEVEL(ch)));
  call_magic(ch, victim, NULL, spell, 0, level, CAST_WEAPON_SPELL);
}

static bool rol_damage_target(struct char_data *ch, struct char_data *victim, int amount,
                              int damage_type)
{
  struct spec_damage_result result;

  result = spec_damage_current_target(ch, victim, amount, -1, damage_type, FALSE);
  return result.status == SPEC_DAMAGE_APPLIED || result.status == SPEC_DAMAGE_TARGET_INVALIDATED;
}

int rol_breath_attack_fire(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  int dice_count;

  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  if (ch == NULL || !IS_NPC(ch) || (victim = FIGHTING(ch)) == NULL)
    return FALSE;

  ch->mob_specials.proc_fired = (ch->mob_specials.proc_fired + 1) % 4;
  if (ch->mob_specials.proc_fired != 0)
    return FALSE;

  act("You breathe \tRfire\tn on $N!", FALSE, ch, NULL, victim, TO_CHAR);
  act("$n breathes \tRfire\tn at you!", FALSE, ch, NULL, victim, TO_VICT);
  act("$n breathes \tRfire\tn on $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
  dice_count = MAX(1, GET_LEVEL(ch) / 2);
  return rol_damage_target(ch, victim, dice(dice_count, 6), DAM_FIRE);
}

static int rol_beholder_attack(struct char_data *ch, bool major)
{
  static const int minor_spells[] = {
      SPELL_BLINDNESS,    SPELL_CHARM_MONSTER,  SPELL_CONE_OF_COLD,    SPELL_FIREBALL,
      SPELL_HARM,         SPELL_LIGHTNING_BOLT, SPELL_CHAIN_LIGHTNING, SPELL_ENFEEBLEMENT,
      SPELL_DISPEL_MAGIC, SPELL_HOLD_MONSTER,
  };
  static const int major_spells[] = {
      SPELL_FIREBALL,        SPELL_ACID_ARROW,   SPELL_SLOW,
      SPELL_ENFEEBLEMENT,    SPELL_HARM,         SPELL_DISPEL_MAGIC,
      SPELL_PRISMATIC_SPRAY, SPELL_HOLD_MONSTER, SPELL_CHAIN_LIGHTNING,
      SPELL_THUNDERCLAP,
  };
  const int *spells;
  struct char_data *victim;
  size_t spell_count;
  size_t spell_index;

  if (ch == NULL || !IS_NPC(ch) || (victim = FIGHTING(ch)) == NULL || rand_number(0, 2) != 0)
    return FALSE;

  spells = major ? major_spells : minor_spells;
  spell_count = major ? sizeof(major_spells) / sizeof(major_spells[0])
                      : sizeof(minor_spells) / sizeof(minor_spells[0]);
  spell_index = (size_t)rand_number(0, (int)spell_count - 1);

  act("One of $n's eyestalks fixes its gaze upon $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
  act("One of $n's eyestalks fixes its gaze upon you!", FALSE, ch, NULL, victim, TO_VICT);
  rol_cast_on_target(ch, victim, spells[spell_index]);
  return TRUE;
}

int rol_hulburg_beholder_major(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_beholder_attack(ch, true);
}

int rol_hulburg_beholder_minor(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_beholder_attack(ch, false);
}

int rol_money_changer(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *mob = me;

  if (ch == NULL || mob == NULL || cmd <= 0 || !AWAKE(mob) || FIGHTING(mob) != NULL)
    return FALSE;
  if (!CMD_IS("exchange") && !CMD_IS("list"))
    return FALSE;

  if (CMD_IS("list"))
  {
    send_to_char(ch, "This money changer uses Luminari's currency exchange rates.\r\n");
    do_cexchange(ch, "", cmd, 0);
    return TRUE;
  }

  do_cexchange(ch, argument, cmd, 0);
  return TRUE;
}

static int rol_plant_attack(struct char_data *ch, int spell, const char *message)
{
  struct char_data *next;
  struct char_data *victim;
  bool fired = false;

  if (ch == NULL || !IS_NPC(ch) || IN_ROOM(ch) == NOWHERE)
    return FALSE;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (victim == ch || IS_NPC(victim) || GET_LEVEL(victim) >= LVL_IMMORT || rand_number(0, 2) != 0)
      continue;
    if (spell == SPELL_BLINDNESS && !can_blind(victim))
      continue;
    if (spell == SPELL_HOLD_MONSTER && paralysis_immunity(victim))
      continue;

    if (!fired)
      act(message, TRUE, ch, NULL, NULL, TO_ROOM);
    rol_cast_on_target(ch, victim, spell);
    fired = true;
  }

  return fired;
}

int rol_plant_attacks_blindness(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_plant_attack(ch, SPELL_BLINDNESS,
                          "The bright flowers along $n's vines puff out a dense cloud of pollen!");
}

int rol_plant_attacks_paralysis(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_plant_attack(ch, SPELL_HOLD_MONSTER,
                          "$n's vines lash outward and wrap around nearby intruders!");
}

int rol_cemetery_disruption(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  struct obj_data *obj = me;
  int amount;

  if (rol_identify(ch, cmd, argument, "Proc: Disruption damage against undead."))
    return TRUE;
  if (!rol_worn_combat_object(ch, obj, &victim) || !IS_UNDEAD(victim) || rand_number(0, 24) != 0)
    return FALSE;

  amount = dice(IS_LICH(victim) ? 3 : 10, 10);
  act("Your $p slams into $N, causing $S form to waver!", FALSE, ch, obj, victim, TO_CHAR);
  act("$n's $p slams into you, causing your form to waver!", FALSE, ch, obj, victim, TO_VICT);
  return rol_damage_target(ch, victim, amount, DAM_HOLY);
}

int rol_cemetery_skeletal_hand(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;

  if (rol_identify(ch, cmd, argument,
                   "Say 'kelemvor's guard' while wearing it: protection from evil, once per week."))
    return TRUE;
  if (!rol_phrase_command(cmd, argument, "kelemvor's guard") ||
      spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID)
    return FALSE;

  if (GET_OBJ_SPECTIMER(obj, 0) > 0)
  {
    send_to_char(ch, "Kelemvor does not answer your call.\r\n");
    return TRUE;
  }

  act("A dark nimbus spreads from $p and surrounds you.", FALSE, ch, obj, NULL, TO_CHAR);
  act("A dark nimbus spreads from $n's $p and surrounds $m.", FALSE, ch, obj, NULL, TO_ROOM);
  call_magic(ch, ch, NULL, SPELL_PROT_FROM_EVIL, 0, MIN(30, GET_LEVEL(ch)), CAST_WEAPON_SPELL);
  point_update_object_spec_timer_set(obj, 0, 168);
  return TRUE;
}

static int rol_cemetery_mob_blade(struct char_data *ch, struct obj_data *obj, bool gleaming)
{
  struct char_data *next;
  struct char_data *target;
  struct char_data *victim;
  int amount;

  if (!IS_NPC(ch) || !rol_worn_combat_object(ch, obj, &victim) || rand_number(0, 15) != 0)
    return FALSE;

  if (rand_number(0, 2) != 0)
  {
    act(gleaming ? "Your gleaming blade explodes with painful light at $N!"
                 : "Your black blade sends a painful wave of shadows at $N!",
        FALSE, ch, obj, victim, TO_CHAR);
    return rol_damage_target(ch, victim, dice(10, 10), gleaming ? DAM_HOLY : DAM_NEGATIVE);
  }

  act(gleaming ? "$n's gleaming blade explodes with brilliant light!"
               : "$n's black blade sends out a swirling cloud of shadows!",
      FALSE, ch, obj, victim, TO_ROOM);
  amount = dice(8, 10);
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (target == ch || (IS_NPC(target) && !IS_PET(target)))
      continue;
    damage(ch, target, amount, -1, gleaming ? DAM_HOLY : DAM_NEGATIVE, FALSE);
  }
  return TRUE;
}

int rol_cemetery_black_blade(struct char_data *ch, void *me, int cmd, const char *argument)
{
  if (rol_identify(ch, cmd, argument, "NPC weapon proc: shadow wave or shadow burst."))
    return TRUE;
  return rol_cemetery_mob_blade(ch, me, false);
}

int rol_cemetery_gleaming_blade(struct char_data *ch, void *me, int cmd, const char *argument)
{
  if (rol_identify(ch, cmd, argument, "NPC weapon proc: light beam or light burst."))
    return TRUE;
  return rol_cemetery_mob_blade(ch, me, true);
}

int rol_cemetery_cloak_meteors(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  struct obj_data *obj = me;

  if (rol_identify(ch, cmd, argument, "Say 'starfall' in combat: meteor burst, once per week."))
    return TRUE;
  if (!rol_phrase_command(cmd, argument, "starfall") || !rol_worn_combat_object(ch, obj, &victim))
    return FALSE;

  if (GET_OBJ_SPECTIMER(obj, 0) > 0)
  {
    send_to_char(ch, "The stars on the cloak remain dark.\r\n");
    return TRUE;
  }

  act("The stars on your $p darken, then streak toward $N!", FALSE, ch, obj, victim, TO_CHAR);
  act("The stars on $n's $p streak toward you!", FALSE, ch, obj, victim, TO_VICT);
  point_update_object_spec_timer_set(obj, 0, 168);
  rol_damage_target(ch, victim, dice(MAX(1, MIN(30, GET_LEVEL(ch))), 10), DAM_FIRE);
  return TRUE;
}

int rol_cemetery_lightsaber(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  struct obj_data *obj = me;

  if (rol_identify(ch, cmd, argument, "Proc: energy beam; critical hits may blind or distract."))
    return TRUE;
  if (!rol_worn_combat_object(ch, obj, &victim))
    return FALSE;

  if (rol_is_critical(argument))
  {
    if (rand_number(0, 9) == 0)
      rol_cast_on_target(ch, victim, SPELL_RAINBOW_PATTERN);
    if (rand_number(0, 9) == 0)
      rol_cast_on_target(ch, victim, SPELL_BLINDNESS);
    return FALSE;
  }

  if (rand_number(0, 29) != 0)
    return FALSE;
  act("A brilliant beam leaps from your $p toward $N!", FALSE, ch, obj, victim, TO_CHAR);
  return rol_damage_target(ch, victim, dice(MAX(1, GET_LEVEL(ch) / 2), 10), DAM_HOLY);
}

static int rol_tanthorian_blade(struct char_data *ch, struct obj_data *obj, const char *argument,
                                bool flaming)
{
  struct char_data *victim;

  if (!rol_is_critical(argument) || !rol_worn_combat_object(ch, obj, &victim))
    return FALSE;

  act(flaming ? "Deep blue flames leap from your $p into $N."
              : "The blade of your $p glows deep blue as it strikes $N.",
      FALSE, ch, obj, victim, TO_CHAR);
  act(flaming ? "$n's $p sends deep blue flames into you."
              : "$n's $p glows deep blue as it strikes you.",
      FALSE, ch, obj, victim, TO_VICT);
  return FALSE;
}

int rol_longsword_tanthorian(struct char_data *ch, void *me, int cmd, const char *argument)
{
  if (rol_identify(ch, cmd, argument, "Critical effect: deep-blue blade flare."))
    return TRUE;
  return rol_tanthorian_blade(ch, me, argument, false);
}

int rol_flaming_tanthorian(struct char_data *ch, void *me, int cmd, const char *argument)
{
  if (rol_identify(ch, cmd, argument, "Critical effect: deep-blue flame flare."))
    return TRUE;
  return rol_tanthorian_blade(ch, me, argument, true);
}

int rol_obj_drain(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;
  int amount;

  UNUSED(cmd);
  UNUSED(argument);

  if (ch == NULL || obj == NULL || obj->worn_by != ch || GET_OBJ_SPECTIMER(obj, 0) > 0)
    return FALSE;

  amount = dice(30, 10);
  send_to_char(ch, "\tLThe life is slowly drawn out of you.\tn\r\n");
  GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - dice(6, 3));
  point_update_object_spec_timer_set(obj, 0, 24);
  damage(ch, ch, amount, -1, DAM_NEGATIVE, FALSE);
  return TRUE;
}

int rol_murlynds_spoon(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;

  if (rol_identify(ch, cmd, argument,
                   "Say 'murlynd's feast' while holding it: satiation, once per day."))
    return TRUE;
  if (!rol_phrase_command(cmd, argument, "murlynd's feast") ||
      spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID || FIGHTING(ch) != NULL)
    return FALSE;

  if (GET_OBJ_SPECTIMER(obj, 0) > 0)
  {
    send_to_char(ch, "The spoon hums briefly, then stops.\r\n");
    return TRUE;
  }

  act("A grand feast appears around your $p, then rushes into you as pure nourishment.", FALSE, ch,
      obj, NULL, TO_CHAR);
  act("A grand feast appears around $n's $p, then vanishes.", FALSE, ch, obj, NULL, TO_ROOM);
  gain_condition(ch, HUNGER, 24);
  gain_condition(ch, THIRST, 24);
  point_update_object_spec_timer_set(obj, 0, 24);
  return TRUE;
}

int rol_muspel_bec_de_corbin(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  struct obj_data *obj = me;

  if (rol_identify(ch, cmd, argument, "Proc: reverse strike with the bec de corbin's spike."))
    return TRUE;
  if (!rol_worn_combat_object(ch, obj, &victim) || rand_number(0, 25) != 0)
    return FALSE;

  act("You pull your $p back sharply, stabbing $N with its spike.", FALSE, ch, obj, victim,
      TO_CHAR);
  return rol_damage_target(ch, victim, dice(5, 3), DAM_PUNCTURE);
}

int rol_muspel_dragon_lance(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  struct obj_data *obj = me;

  if (rol_identify(ch, cmd, argument,
                   "Critical effect: mounted electrical strike against dragons."))
    return TRUE;
  if (!rol_is_critical(argument) || !rol_worn_combat_object(ch, obj, &victim) ||
      RIDING(ch) == NULL || !IS_DRAGON(victim))
    return FALSE;

  act("You drive your $p into $N as electricity bursts from its tip!", FALSE, ch, obj, victim,
      TO_CHAR);
  return rol_damage_target(ch, victim, dice(15, 15), DAM_ELECTRIC);
}

int rol_muspel_spider_dagger(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  struct obj_data *obj = me;

  if (rol_identify(ch, cmd, argument, "Critical effect: spider venom."))
    return TRUE;
  if (!rol_is_critical(argument) || !rol_worn_combat_object(ch, obj, &victim) ||
      rand_number(0, 1) != 0)
    return FALSE;

  act("The runes on your $p writhe; spectral spiders bite $N!", FALSE, ch, obj, victim, TO_CHAR);
  rol_cast_on_target(ch, victim, SPELL_POISON);
  return TRUE;
}

int rol_muspel_crystal_scimitar(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  struct obj_data *obj = me;
  int drain;

  if (rol_identify(ch, cmd, argument, "Proc: drains power from psionic targets."))
    return TRUE;
  if (!rol_worn_combat_object(ch, obj, &victim) || !IS_PSIONIC(victim))
    return FALSE;

  drain = MIN(GET_PSP(victim), dice(1, 15));
  if (drain <= 0)
    return FALSE;
  GET_PSP(victim) -= drain;
  act("Your $p passes through $N and drains $S mental energy.", FALSE, ch, obj, victim, TO_CHAR);
  return TRUE;
}

int rol_muspel_recurve_bow(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  struct obj_data *obj = me;

  if (rol_identify(ch, cmd, argument, "Proc: sonic feedback damage."))
    return TRUE;
  if (!rol_worn_combat_object(ch, obj, &victim) || rand_number(0, 20) != 0)
    return FALSE;

  act("A sonic pulse from your $p slams into $N!", FALSE, ch, obj, victim, TO_CHAR);
  return rol_damage_target(ch, victim, rand_number(100, 200), DAM_SOUND);
}

int rol_muspel_duergar_battlehammer(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  struct obj_data *obj = me;

  if (rol_identify(ch, cmd, argument, "Critical effect: weakening dark energy."))
    return TRUE;
  if (!rol_is_critical(argument) || !rol_worn_combat_object(ch, obj, &victim))
    return FALSE;

  act("A dark wave flies from your $p and weakens $N.", FALSE, ch, obj, victim, TO_CHAR);
  rol_cast_on_target(ch, victim, SPELL_ENFEEBLEMENT);
  GET_MOVE(victim) = MAX(0, GET_MOVE(victim) - 40);
  return TRUE;
}

int rol_muspel_dagger_whispers(struct char_data *ch, void *me, int cmd, const char *argument)
{
  static const char *const whispers[] = {
      "Your $p whispers, 'They are watching us.'",
      "Your $p whispers, 'Kill them... kill them all.'",
      "Your $p whispers something you cannot quite understand.",
      "Your $p laughs softly in your hand.",
  };
  struct char_data *victim;
  struct obj_data *obj = me;
  int index;

  if (rol_identify(ch, cmd, argument, "Proc: the sentient dagger whispers during combat."))
    return TRUE;
  if (!rol_worn_combat_object(ch, obj, &victim) || rand_number(0, 19) != 0)
    return FALSE;

  UNUSED(victim);
  index = rand_number(0, (int)(sizeof(whispers) / sizeof(whispers[0])) - 1);
  act(whispers[index], FALSE, ch, obj, NULL, TO_CHAR);
  return TRUE;
}

int rol_thorn_shield(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;
  struct obj_data *obj = me;

  if (rol_identify(ch, cmd, argument, "Defense proc: thorns damage an attacker on shield block."))
    return TRUE;
  if (cmd != 0 || argument == NULL || str_cmp(argument, "shieldblock") ||
      !rol_worn_combat_object(ch, obj, &victim) || rand_number(0, 1) != 0)
    return FALSE;

  act("Blood sprays from $N as your $p's thorns catch $M!", FALSE, ch, obj, victim, TO_CHAR);
  return rol_damage_target(ch, victim, GET_LEVEL(ch) + dice(1, 15), DAM_PUNCTURE);
}
