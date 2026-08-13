/**
 * @file spec/spec_rol_darkhold.c
 * Source-profiled Darkhold adapters for the Realms of Luminari conversion.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "magic/domains_schools.h"
#include "magic/spells.h"
#include "spec/spec_combat.h"
#include "spec/spec_context.h"
#include "spec/spec_dispatch.h"
#include "spec/spec_rol_darkhold.h"

#define ROL_DARKHOLD_SUMMONED_MOBILE_VNUM 2094500
#define ROL_DARKHOLD_PASSAGE_ROOM_VNUM 2094666
#define ROL_DARKHOLD_NORTH_GEM_ROOM_VNUM 2094668
#define ROL_DARKHOLD_NORTH_GEM_DESTINATION_VNUM 2094674
#define ROL_DARKHOLD_SOUTH_GEM_ROOM_VNUM 2094667
#define ROL_DARKHOLD_SOUTH_GEM_DESTINATION_VNUM 2094673
#define ROL_DARKHOLD_SHADOW_DRAGON_ROOM_VNUM 2094675
#define ROL_DARKHOLD_SHADOW_FIEND_VNUM 2094505
#define ROL_DARKHOLD_SHADOW_DRAGON_VNUM 2094506

struct rol_darkhold_object_profile_data
{
  int object_vnum;
  enum rol_darkhold_object_kind kind;
  int room_vnum;
  int destination_vnum;
};

static const struct rol_darkhold_object_profile_data rol_darkhold_object_profiles[] = {
    {2094501, ROL_DARKHOLD_OBJECT_SUMMON_SKULL, -1, ROL_DARKHOLD_SUMMONED_MOBILE_VNUM},
    {2094502, ROL_DARKHOLD_OBJECT_SUMMON_SKULL, -1, ROL_DARKHOLD_SUMMONED_MOBILE_VNUM},
    {2094503, ROL_DARKHOLD_OBJECT_SUMMON_SKULL, -1, ROL_DARKHOLD_SUMMONED_MOBILE_VNUM},
    {2094504, ROL_DARKHOLD_OBJECT_PASSAGE_SKULL, ROL_DARKHOLD_PASSAGE_ROOM_VNUM, -1},
    {2094505, ROL_DARKHOLD_OBJECT_SUMMON_SKULL, -1, ROL_DARKHOLD_SUMMONED_MOBILE_VNUM},
    {2094506, ROL_DARKHOLD_OBJECT_SUMMON_SKULL, -1, ROL_DARKHOLD_SUMMONED_MOBILE_VNUM},
    {2094507, ROL_DARKHOLD_OBJECT_SUMMON_SKULL, -1, ROL_DARKHOLD_SUMMONED_MOBILE_VNUM},
    {2094508, ROL_DARKHOLD_OBJECT_SOUTH_GEM, ROL_DARKHOLD_SOUTH_GEM_ROOM_VNUM,
     ROL_DARKHOLD_SOUTH_GEM_DESTINATION_VNUM},
    {2094509, ROL_DARKHOLD_OBJECT_NORTH_GEM, ROL_DARKHOLD_NORTH_GEM_ROOM_VNUM,
     ROL_DARKHOLD_NORTH_GEM_DESTINATION_VNUM},
    {2094510, ROL_DARKHOLD_OBJECT_SOUTH_GEM, ROL_DARKHOLD_SOUTH_GEM_ROOM_VNUM,
     ROL_DARKHOLD_SOUTH_GEM_DESTINATION_VNUM},
    {2094511, ROL_DARKHOLD_OBJECT_NORTH_GEM, ROL_DARKHOLD_NORTH_GEM_ROOM_VNUM,
     ROL_DARKHOLD_NORTH_GEM_DESTINATION_VNUM},
};

static const struct rol_darkhold_object_profile_data *
rol_darkhold_object_profile_for(int object_vnum)
{
  size_t index;

  for (index = 0;
       index < sizeof(rol_darkhold_object_profiles) / sizeof(rol_darkhold_object_profiles[0]);
       index++)
    if (rol_darkhold_object_profiles[index].object_vnum == object_vnum)
      return &rol_darkhold_object_profiles[index];
  return NULL;
}

size_t rol_darkhold_object_profile_count(void)
{
  return sizeof(rol_darkhold_object_profiles) / sizeof(rol_darkhold_object_profiles[0]);
}

bool rol_darkhold_object_profile(int object_vnum, enum rol_darkhold_object_kind *kind,
                                 int *room_vnum, int *destination_vnum)
{
  const struct rol_darkhold_object_profile_data *profile =
      rol_darkhold_object_profile_for(object_vnum);

  if (profile == NULL)
    return false;
  if (kind != NULL)
    *kind = profile->kind;
  if (room_vnum != NULL)
    *room_vnum = profile->room_vnum;
  if (destination_vnum != NULL)
    *destination_vnum = profile->destination_vnum;
  return true;
}

static bool rol_darkhold_command_is(int cmd, const char *name)
{
  return cmd > 0 && complete_cmd_info != NULL && complete_cmd_info[cmd].command != NULL &&
         strcmp(complete_cmd_info[cmd].command, name) == 0;
}

static bool rol_darkhold_object_name_matches(const struct obj_data *obj, const char *argument)
{
  char name[MAX_INPUT_LENGTH];

  if (obj == NULL || obj->name == NULL || argument == NULL)
    return false;
  one_argument(argument, name, sizeof(name));
  return *name != '\0' && isname(name, obj->name);
}

static int rol_darkhold_summon_skull(struct char_data *ch, struct obj_data *obj,
                                     const char *argument)
{
  struct char_data *summoned;
  mob_rnum rnum;

  if (ch == NULL || obj == NULL || !AWAKE(ch) || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      !rol_darkhold_object_name_matches(obj, argument))
    return FALSE;
  if ((rnum = real_mobile(ROL_DARKHOLD_SUMMONED_MOBILE_VNUM)) == NOBODY ||
      (summoned = read_mobile(rnum, REAL)) == NULL)
  {
    log("SYSERR: RoL Darkhold skull cannot load mobile %d", ROL_DARKHOLD_SUMMONED_MOBILE_VNUM);
    return FALSE;
  }

  char_to_room(summoned, IN_ROOM(ch));
  act("You hear a deep, loud note.", TRUE, ch, obj, NULL, TO_CHAR);
  act("You hear a deep, loud note.", TRUE, ch, obj, NULL, TO_ROOM);
  act("A swirling mist coalesces into $n.", TRUE, summoned, NULL, NULL, TO_ROOM);
  return TRUE;
}

static int rol_darkhold_passage_skull(struct char_data *ch, struct obj_data *obj,
                                      const char *argument)
{
  struct char_data *dummy = NULL;
  struct obj_data *selected = NULL;
  struct room_direction_data *exit;
  room_rnum room;

  if (ch == NULL || obj == NULL || argument == NULL ||
      !generic_find(argument, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &dummy,
                    &selected) ||
      selected != obj)
    return FALSE;
  if (!VALID_ROOM_RNUM((room = real_room(ROL_DARKHOLD_PASSAGE_ROOM_VNUM))) ||
      (exit = world[room].dir_option[NORTH]) == NULL)
  {
    send_to_char(ch, "This item is broken. Talk to an admin!\r\n");
    return TRUE;
  }
  if (!EXIT_FLAGGED(exit, EX_BLOCKED))
  {
    send_to_char(ch, "Nothing happens.\r\n");
    return TRUE;
  }
  if (!VALID_ROOM_RNUM(IN_ROOM(obj)))
    return FALSE;

  send_to_char(ch, "You hear a high, crystalline note.\r\n");
  act("You hear a high, crystalline note.", FALSE, ch, NULL, NULL, TO_ROOM);
  REMOVE_BIT(exit->exit_info, EX_BLOCKED);
  send_to_room(room, "The north wall moves aside, revealing a passageway.\r\n");
  return TRUE;
}

static int rol_darkhold_gem(struct char_data *ch, struct obj_data *obj,
                            const struct rol_darkhold_object_profile_data *profile,
                            const char *argument)
{
  struct room_direction_data *exit;
  room_rnum destination;
  room_rnum room;
  int direction;

  if (ch == NULL || obj == NULL || profile == NULL ||
      !rol_darkhold_object_name_matches(obj, argument))
    return FALSE;
  room = real_room(profile->room_vnum);
  if (!VALID_ROOM_RNUM(room) || IN_ROOM(ch) != room)
    return FALSE;
  destination = real_room(profile->destination_vnum);
  direction = profile->kind == ROL_DARKHOLD_OBJECT_NORTH_GEM ? NORTH : SOUTH;
  if (!VALID_ROOM_RNUM(destination) || (exit = world[room].dir_option[direction]) == NULL)
  {
    send_to_char(ch, "This item is broken. Talk to an admin!\r\n");
    return TRUE;
  }

  act("As you try to drop $p, a flashing light brightens the room.", TRUE, ch, obj, NULL, TO_CHAR);
  act("As $n tries to drop $p, a flashing light brightens the room.", TRUE, ch, obj, NULL, TO_ROOM);
  if (exit->to_room != destination)
    exit->to_room = destination;
  return TRUE;
}

int rol_darkhold_object(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  return FALSE;
}

int rol_darkhold_object_typed(struct spec_event_context *context)
{
  const struct rol_darkhold_object_profile_data *profile;
  struct char_data *ch;
  struct obj_data *obj;

  if (context == NULL || context->owner_type != SPEC_OWNER_OBJECT ||
      context->event != SPEC_EVENT_COMMAND || context->owner == NULL)
    return FALSE;
  obj = context->owner;
  ch = context->actor;
  profile = rol_darkhold_object_profile_for(GET_OBJ_VNUM(obj));
  if (ch == NULL || profile == NULL)
    return FALSE;

  if (profile->kind == ROL_DARKHOLD_OBJECT_SUMMON_SKULL)
  {
    if (!rol_darkhold_command_is(context->command, "push"))
      return FALSE;
    return rol_darkhold_summon_skull(ch, obj, context->argument);
  }
  if (profile->kind == ROL_DARKHOLD_OBJECT_PASSAGE_SKULL)
  {
    if (!rol_darkhold_command_is(context->command, "push"))
      return FALSE;
    return rol_darkhold_passage_skull(ch, obj, context->argument);
  }
  if (!rol_darkhold_command_is(context->command, "drop"))
    return FALSE;
  return rol_darkhold_gem(ch, obj, profile, context->argument);
}

bool rol_darkhold_monster_profile(int mobile_vnum, bool *shadow_fiend, bool *shadow_dragon)
{
  bool fiend = mobile_vnum == ROL_DARKHOLD_SHADOW_FIEND_VNUM;
  bool dragon = mobile_vnum == ROL_DARKHOLD_SHADOW_DRAGON_VNUM;

  if (!fiend && !dragon)
    return false;
  if (shadow_fiend != NULL)
    *shadow_fiend = fiend;
  if (shadow_dragon != NULL)
    *shadow_dragon = dragon;
  return true;
}

int rol_darkhold_mobile_death(struct spec_event_context *context, struct char_data *ch)
{
  struct room_direction_data *exit;
  room_rnum room;

  if (context == NULL || ch == NULL || GET_MOB_VNUM(ch) != ROL_DARKHOLD_SHADOW_DRAGON_VNUM)
    return FALSE;
  room = real_room(ROL_DARKHOLD_SHADOW_DRAGON_ROOM_VNUM);
  if (!VALID_ROOM_RNUM(room) || (exit = world[room].dir_option[NORTH]) == NULL)
  {
    log("SYSERR: RoL Darkhold shadow dragon cannot find north exit in room %d",
        ROL_DARKHOLD_SHADOW_DRAGON_ROOM_VNUM);
    return FALSE;
  }

  act("As $n breathes for the last time, you sense something happen nearby.", FALSE, ch, NULL, NULL,
      TO_ROOM);
  REMOVE_BIT(exit->exit_info, EX_HIDDEN);
  REMOVE_BIT(exit->exit_info, EX_LOCKED);
  return TRUE;
}

bool rol_darkhold_shadow_fiend_steal_roll_fires(int roll)
{
  return roll == 0;
}

int rol_darkhold_shadow_fiend_cooldown_seconds(bool darkness)
{
  return darkness ? SECS_PER_MUD_DAY : PULSE_VIOLENCE * 4;
}

static bool rol_darkhold_cooldown_ready(time_t ready_at, time_t now)
{
  return ready_at <= 0 || ready_at <= now;
}

static bool rol_darkhold_shadow_fiend_darkness(struct char_data *ch, time_t now)
{
  if (!rol_darkhold_cooldown_ready(ch->mob_specials.rol_darkhold_darkness_ready_at, now) ||
      room_is_dark(IN_ROOM(ch)))
    return false;

  act("$n mutters a magical word...", FALSE, ch, NULL, NULL, TO_ROOM);
  act("You mutter a magical word...", FALSE, ch, NULL, NULL, TO_CHAR);
  call_magic(ch, NULL, NULL, SPELL_DARKNESS, 0, GET_LEVEL(ch), CAST_INNATE);
  ch->mob_specials.rol_darkhold_darkness_ready_at =
      now + rol_darkhold_shadow_fiend_cooldown_seconds(true);
  return true;
}

static bool rol_darkhold_shadow_fiend_steal(struct spec_event_context *context,
                                            struct char_data *ch, struct char_data *victim,
                                            time_t now)
{
  struct spec_damage_result result;
  int amount;

  if (GET_LEVEL(victim) >= LVL_IMMORT ||
      !rol_darkhold_cooldown_ready(ch->mob_specials.rol_darkhold_steal_ready_at, now) ||
      !rol_darkhold_shadow_fiend_steal_roll_fires(rand_number(0, 5)))
    return false;

  ch->mob_specials.rol_darkhold_steal_ready_at =
      now + rol_darkhold_shadow_fiend_cooldown_seconds(false);
  act("$n grins as it mutters a magical word...", FALSE, ch, NULL, victim, TO_NOTVICT);
  act("You mutter a magical word...", FALSE, ch, NULL, victim, TO_CHAR);
  act("$n mutters a magical word...", FALSE, ch, NULL, victim, TO_VICT);

  if (savingthrow(ch, victim, SAVING_WILL, 5, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
  {
    act("Suddenly $N shouts in pain, but $E manages to resist.", FALSE, ch, NULL, victim,
        TO_NOTVICT);
    act("Suddenly $N shouts in pain, but $E manages to resist.", FALSE, ch, NULL, victim, TO_CHAR);
    act("You feel someone trying to strip your mind away, but you resist.", FALSE, ch, NULL, victim,
        TO_VICT);
    return true;
  }

  act("Suddenly $N shouts in pain and flails about in agony.", FALSE, ch, NULL, victim, TO_NOTVICT);
  act("Suddenly $N shouts in pain and flails about in agony.", FALSE, ch, NULL, victim, TO_CHAR);
  act("You feel parts of your mind stripped away...", FALSE, ch, NULL, victim, TO_VICT);
  amount = dice(45, 10);
  result = spec_damage_current_target(ch, victim, amount, -1, DAM_RESERVED_DBC, FALSE);
  if (!AFF_FLAGGED(ch, AFF_BLACKMANTLE))
    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + amount);
  if (result.status == SPEC_DAMAGE_TARGET_INVALIDATED)
    context->invalidation |= SPEC_INVALIDATE_TARGET;
  return true;
}

int rol_darkhold_mobile_hit(struct spec_event_context *context, struct char_data *ch)
{
  struct char_data *victim;
  bool fired = false;
  time_t now;

  if (context == NULL || ch == NULL || GET_MOB_VNUM(ch) != ROL_DARKHOLD_SHADOW_FIEND_VNUM ||
      spec_context_validate_combat_target(ch, context->target, false) != SPEC_CONTEXT_VALID)
    return FALSE;
  victim = context->target;
  now = time(NULL);
  fired = rol_darkhold_shadow_fiend_darkness(ch, now);
  if (rol_darkhold_shadow_fiend_steal(context, ch, victim, now))
    fired = true;
  return fired ? TRUE : FALSE;
}

bool rol_darkhold_warhammer_roll_fires(int roll)
{
  return roll == 0;
}

int rol_darkhold_bastard_modifier(bool npc, int roll)
{
  if (npc)
    return roll >= 1 && roll <= 4 ? 3 + roll : 0;
  return roll >= 2 && roll <= 5 ? roll : 0;
}

static bool rol_darkhold_primary_weapon_slot(int slot)
{
  return slot == WEAR_WIELD_1 || slot == WEAR_WIELD_2H;
}

static int rol_darkhold_warhammer_hit(struct spec_event_context *context, struct char_data *ch,
                                      struct obj_data *obj, struct char_data *victim, int slot)
{
  struct spec_damage_result result;

  if (!rol_darkhold_primary_weapon_slot(slot) ||
      !rol_darkhold_warhammer_roll_fires(rand_number(0, 20)))
    return FALSE;
  act("With a mighty swing, your $p flies from your hands and smashes into $N. The warhammer "
      "explodes into shards of ice before appearing back in your hands.",
      FALSE, ch, obj, victim, TO_CHAR);
  act("With a mighty swing, $n's $p flies from $s hands and smashes into $N. The warhammer "
      "explodes into shards of ice before appearing back in $n's hands.",
      FALSE, ch, obj, victim, TO_NOTVICT);
  act("$n's $p smashes painfully into you and explodes into shards of freezing ice before "
      "appearing back in $n's hands.",
      FALSE, ch, obj, victim, TO_VICT);
  result = spec_damage_current_target(ch, victim, dice(30, 10), -1, DAM_RESERVED_DBC, FALSE);
  if (result.status == SPEC_DAMAGE_TARGET_INVALIDATED)
    context->invalidation |= SPEC_INVALIDATE_TARGET;
  return FALSE;
}

static int rol_darkhold_bastard_hit(struct char_data *ch, struct obj_data *obj,
                                    struct char_data *victim, int slot, bool critical)
{
  struct affected_type affect;
  int modifier;

  if (!critical || !rol_darkhold_primary_weapon_slot(slot))
    return FALSE;
  if (affected_by_spell(victim, SPELL_RAINBOW_PATTERN))
  {
    act("The sparkling lights around $N continue fading in and out of existence, further "
        "confusing $M!",
        TRUE, ch, NULL, victim, TO_CHAR);
    act("The sparkling lights around $N continue fading in and out of existence, further "
        "confusing $M!",
        TRUE, ch, NULL, victim, TO_NOTVICT);
    return FALSE;
  }

  act("As your $p arcs through the air, a thousand sparkling lights race toward $N, who "
      "stumbles as they fade in and out of existence.",
      FALSE, ch, obj, victim, TO_CHAR);
  act("As $n's $p arcs through the air, a thousand sparkling lights race toward $N, who "
      "stumbles as they fade in and out of existence.",
      FALSE, ch, obj, victim, TO_NOTVICT);
  act("As $n's $p arcs through the air, a thousand sparkling lights race toward your head and "
      "make it hard to focus.",
      FALSE, ch, obj, victim, TO_VICT);

  modifier = IS_NPC(victim) ? rol_darkhold_bastard_modifier(true, rand_number(1, 4))
                            : rol_darkhold_bastard_modifier(false, rand_number(2, 5));
  new_affect(&affect);
  affect.spell = SPELL_RAINBOW_PATTERN;
  affect.duration = 0;
  affect.modifier = -modifier;
  affect.location = APPLY_HITROLL;
  affect_to_char(victim, &affect);
  affect.location = APPLY_DAMROLL;
  affect_to_char(victim, &affect);
  return FALSE;
}

int rol_darkhold_weapon_hit(struct spec_event_context *context, struct char_data *ch,
                            struct obj_data *obj, struct char_data *victim, int slot,
                            bool warhammer)
{
  if (context == NULL || ch == NULL || obj == NULL || victim == NULL)
    return FALSE;
  if (warhammer)
    return rol_darkhold_warhammer_hit(context, ch, obj, victim, slot);
  return rol_darkhold_bastard_hit(ch, obj, victim, slot, context->critical);
}
