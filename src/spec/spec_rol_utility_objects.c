/**
 * @file spec/spec_rol_utility_objects.c
 * Converted Realms of Luminari utility-object special procedures.
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
#include "magic/spells.h"
#include "mud_event.h"
#include "spec_combat.h"
#include "spec_context.h"
#include "spec_cooldown.h"
#include "spec_dispatch.h"
#include "spec_rol_utility_objects.h"
#include "point_update_periodic.h"

#define ROL_GOODBERRY_VNUM 2000876
#define ROL_LOOT_BLOCKER_VNUM 2000897
#define ROL_PLAGUE_RESERVOIR_VNUM 2003088
#define ROL_PLAGUE_RESERVOIR_ROOM_VNUM 2003001
#define ROL_PLAGUE_MINIMUM_LEVEL 15
#define ROL_LOOT_SWEEP_SECONDS 120
#define ROL_LOOT_DECAY_TICKS 1
#define ROL_BLOODSTONE_CHILD_VNUM 2007151
#define ROL_NECROMANCER_CHILD_VNUM 2046991
#define ROL_MENDEN_FIGURINE_VNUM 2088825
#define ROL_RUBY_MONOCLE_VNUM 2090004
#define ROL_RUBY_MONOCLE_ROOM_MIN 2090124
#define ROL_RUBY_MONOCLE_ROOM_MAX 2090142
#define ROL_MAGIUS_STAFF_VNUM 2000047
#define ROL_BASILISK_SNAKE_VNUM 2000412
#define ROL_LATHANDER_DISC_VNUM 2019932
#define ROL_DRAGONCULT_ROBES_VNUM 2010672
#define ROL_CRESCENT_MOON_VNUM 2019988
#define ROL_EARTHMOTHER_STAFF_VNUM 2026260
#define ROL_BASILISK_LEGGINGS_VNUM 2043723
#define ROL_BASILISK_SNAKES_VNUM 2044019
#define ROL_BLUEPLUME_VNUM 2051110
#define ROL_WRITHING_ASH_VNUM 2051207
#define ROL_HASTE_SLEEVES_VNUM 2057236
#define ROL_SMOKE_STUN_SHIELD_VNUM 2057003
#define ROL_LLYMS_ALTAR_VNUM 2088830
#define ROL_LLYMS_SERVANT_ONE_VNUM 2088814
#define ROL_LLYMS_SERVANT_TWO_VNUM 2088815
#define ROL_LLYMS_REWARD_ONE_VNUM 2088831
#define ROL_LLYMS_REWARD_TWO_VNUM 2088832
#define ROL_LLYMS_REWARD_THREE_VNUM 2088833
#define ROL_BLACK_ORCHID_VNUM 2093243
#define ROL_BLACK_ORCHID_DECAY_HOURS 72
#define ROL_SPIDERHAUNT_MAGGOTS_VNUM 2080205
#define ROL_SPIDERHAUNT_CYRICS_ALTAR_VNUM 2080213
#define ROL_SPIDERHAUNT_MAGGOTS_DELAY (20 * PULSE_VIOLENCE)
#define ROL_ACHERON_PLATFORM_PORTAL_VNUM 2050000
#define ROL_ACHERON_ENTRANCE_PORTAL_VNUM 2050100
#define ROL_ACHERON_ROAMING_PORTAL_MIN_VNUM 2050101
#define ROL_ACHERON_ROAMING_PORTAL_MAX_VNUM 2050104
#define ROL_PROXIMITY_EXPLOSION_VNUM 2046008
#define ROL_HYSSK_SKELETON_VNUM 2035164
#define ROL_HYSSK_ILLUSION_DESTINATION_VNUM 2034494
#define ROL_UM_TATTERED_CLOAK_VNUM 2093175
#define ROL_UM_QOGEK_STAFF_VNUM 2093179

enum rol_utility_called_effect
{
  ROL_UTILITY_MAGIUS_STAFF = 0,
  ROL_UTILITY_DRAGONCULT_ROBES,
  ROL_UTILITY_CRESCENT_MOON,
  ROL_UTILITY_EARTHMOTHER_STAFF,
  ROL_UTILITY_BASILISK_LEGGINGS,
  ROL_UTILITY_BASILISK_SNAKES,
  ROL_UTILITY_BLUEPLUME,
  ROL_UTILITY_WRITHING_ASH,
  ROL_UTILITY_HASTE_SLEEVES
};

struct rol_utility_called_profile
{
  int object_vnum;
  enum rol_utility_called_effect effect;
  const char *phrase;
  int cooldown_hours;
  const char *description;
};

static bool rol_utility_say_phrase(const struct spec_event_context *context, const char *phrase);

/* Keep this table sorted by converted object VNUM for binary lookup. */
static const struct rol_utility_called_profile rol_utility_called_profiles[] = {
    {ROL_MAGIUS_STAFF_VNUM, ROL_UTILITY_MAGIUS_STAFF, "shirak", 0,
     "Say 'shirak' to light the staff or 'dulak' to darken it."},
    {ROL_DRAGONCULT_ROBES_VNUM, ROL_UTILITY_DRAGONCULT_ROBES, "draconian protection", 72,
     "Say 'draconian protection' while worn for elemental resistance every three MUD days."},
    {ROL_CRESCENT_MOON_VNUM, ROL_UTILITY_CRESCENT_MOON, "Crescent Moon", 0,
     "Say 'Crescent Moon' while worn for invisibility once per combat round."},
    {ROL_EARTHMOTHER_STAFF_VNUM, ROL_UTILITY_EARTHMOTHER_STAFF, "aid me earthmother", 72,
     "Say 'aid me earthmother' while worn for random elemental aid every three MUD days."},
    {ROL_BASILISK_LEGGINGS_VNUM, ROL_UTILITY_BASILISK_LEGGINGS, "petrify", 48,
     "Say 'petrify' while worn for stoneskin every two MUD days."},
    {ROL_BASILISK_SNAKES_VNUM, ROL_UTILITY_BASILISK_SNAKES, "snakes", 48,
     "Say 'snakes' while worn to summon a charmed snake every two MUD days."},
    {ROL_BLUEPLUME_VNUM, ROL_UTILITY_BLUEPLUME, "Tyr Grant Me Might", 24,
     "Say 'Tyr Grant Me Might' while worn for Tyr's favor once per MUD day."},
    {ROL_WRITHING_ASH_VNUM, ROL_UTILITY_WRITHING_ASH, "Ashentoris Aid Me", 24,
     "Say 'Ashentoris Aid Me' while worn and fighting for Ashentoris' aid once per MUD day."},
    {ROL_HASTE_SLEEVES_VNUM, ROL_UTILITY_HASTE_SLEEVES, "accelerate", 48,
     "Say 'accelerate' while worn for haste every two MUD days."},
};

static const struct rol_utility_called_profile *rol_utility_called_profile_for(int object_vnum)
{
  size_t high = sizeof(rol_utility_called_profiles) / sizeof(rol_utility_called_profiles[0]);
  size_t low = 0;
  size_t middle;

  while (low < high)
  {
    middle = low + (high - low) / 2;
    if (rol_utility_called_profiles[middle].object_vnum < object_vnum)
      low = middle + 1;
    else
      high = middle;
  }
  if (low < sizeof(rol_utility_called_profiles) / sizeof(rol_utility_called_profiles[0]) &&
      rol_utility_called_profiles[low].object_vnum == object_vnum)
    return &rol_utility_called_profiles[low];
  return NULL;
}

size_t rol_utility_called_profile_count(void)
{
  return sizeof(rol_utility_called_profiles) / sizeof(rol_utility_called_profiles[0]);
}

bool rol_utility_called_profile(int object_vnum, const char **phrase, int *cooldown_hours,
                                const char **description)
{
  const struct rol_utility_called_profile *profile = rol_utility_called_profile_for(object_vnum);

  if (profile == NULL)
    return false;
  if (phrase != NULL)
    *phrase = profile->phrase;
  if (cooldown_hours != NULL)
    *cooldown_hours = profile->cooldown_hours;
  if (description != NULL)
    *description = profile->description;
  return true;
}

static const char *rol_utility_description_for(int object_vnum)
{
  const struct rol_utility_called_profile *profile = rol_utility_called_profile_for(object_vnum);

  if (profile != NULL)
    return profile->description;
  switch (object_vnum)
  {
  case ROL_LATHANDER_DISC_VNUM:
    return "Rub the disc to become young again; the disc is consumed and leaves you unconscious.";
  case ROL_SMOKE_STUN_SHIELD_VNUM:
    return "May discharge electricity on a shield block or shield punch.";
  case ROL_LLYMS_ALTAR_VNUM:
    return "Offer a valuable held treasure to the altar for Llym's favor.";
  case ROL_LOOT_BLOCKER_VNUM:
    return "Aggressive creatures protect room containers and non-player corpses; unclaimed "
           "non-player corpses decay promptly.";
  case ROL_PLAGUE_RESERVOIR_VNUM:
    return "Drinking from or filling a container at this reservoir can infect experienced "
           "mortals with disease.";
  case ROL_SPIDERHAUNT_MAGGOTS_VNUM:
    return "Eating while carrying these maggots may leave a delayed squirming sensation.";
  case ROL_SPIDERHAUNT_CYRICS_ALTAR_VNUM:
    return "Sit before the altar and WORSHIP to receive Cyric's limited blessing.";
  default:
    return NULL;
  }
}

static const char *rol_utility_command_name(int cmd)
{
  if (cmd <= 0 || complete_cmd_info == NULL)
    return NULL;
  return complete_cmd_info[cmd].command;
}

static bool rol_utility_command_is(int cmd, const char *name)
{
  const char *command;

  command = rol_utility_command_name(cmd);
  return command != NULL && name != NULL && !strcmp(command, name);
}

bool rol_utility_spiderhaunt_maggots_trigger(const char *argument)
{
  return argument != NULL && strcmp(argument, "maggots") != 0;
}

bool rol_utility_spiderhaunt_altar_trigger(const char *command, int position)
{
  return command != NULL && !strcmp(command, "worship") && position == POS_SITTING;
}

bool rol_utility_acheron_roaming_room_allowed(int room_vnum)
{
  return room_vnum >= 2050100 && room_vnum <= 2050184 &&
         (room_vnum < 2050143 || room_vnum > 2050149);
}

bool rol_utility_acheron_platform_room_allowed(int current_vnum, int destination_vnum)
{
  return destination_vnum >= 2050000 && destination_vnum <= 2050099 &&
         abs(destination_vnum - current_vnum) > 3;
}

MUD_EVENT_CALLBACK(event_rol_spiderhaunt_maggots)
{
  struct mud_event_data *event_data;
  struct char_data *ch;

  event_data = event_obj;
  if (event_data == NULL || event_data->pStruct == NULL)
    return 0;
  ch = event_data->pStruct;
  if (!VALID_ROOM_RNUM(IN_ROOM(ch)))
    return 0;

  if (AWAKE(ch))
    send_to_char(ch, "You feel something squirm and wiggle inside your belly.\r\n");
  else
    send_to_char(ch, "You feel a vaguely disturbing sensation in your stomach.\r\n");
  return 0;
}

static int rol_utility_spiderhaunt_maggots(struct char_data *ch, struct obj_data *obj, int cmd,
                                           const char *argument)
{
  if (!AWAKE(ch) || !rol_utility_command_is(cmd, "eat") ||
      !rol_utility_spiderhaunt_maggots_trigger(argument) || obj->carried_by != ch || IS_NPC(ch) ||
      GET_COND(ch, HUNGER) > 20 || char_has_mud_event(ch, eROL_SPIDERHAUNT_MAGGOTS) != NULL)
    return FALSE;

  NEW_EVENT(eROL_SPIDERHAUNT_MAGGOTS, ch, NULL, ROL_SPIDERHAUNT_MAGGOTS_DELAY);
  return FALSE;
}

static int rol_utility_spiderhaunt_altar(struct char_data *ch, struct obj_data *obj, int cmd)
{
  if (!AWAKE(ch) || GET_OBJ_VAL(obj, 0) <= 0 ||
      !rol_utility_spiderhaunt_altar_trigger(rol_utility_command_name(cmd), GET_POS(ch)))
    return FALSE;

  send_to_char(ch, "As you kneel, a crimson beam erupts from the altar and strikes you. You feel a "
                   "stronger sense of purpose as evil starts looking more appealing.\r\n");
  act("As $n settles onto $s knees, a crimson beam erupts from the altar and envelops $m in a "
      "red aura.",
      TRUE, ch, NULL, NULL, TO_ROOM);
  GET_ALIGNMENT(ch) = MAX(-1000, GET_ALIGNMENT(ch) - 1);
  (void)call_magic(ch, ch, NULL, SPELL_HEAL, 0, 50, CAST_INNATE);
  GET_OBJ_VAL(obj, 0)--;
  return TRUE;
}

static bool rol_utility_acheron_portal_vnum(int vnum)
{
  return vnum == ROL_ACHERON_PLATFORM_PORTAL_VNUM || vnum == ROL_ACHERON_ENTRANCE_PORTAL_VNUM ||
         (vnum >= ROL_ACHERON_ROAMING_PORTAL_MIN_VNUM &&
          vnum <= ROL_ACHERON_ROAMING_PORTAL_MAX_VNUM);
}

static int rol_utility_acheron_enter(struct spec_event_context *context, struct char_data *ch,
                                     struct obj_data *obj)
{
  room_rnum destination;
  char keyword[MAX_INPUT_LENGTH];

  if (!rol_utility_command_is(context->command, "enter") || IS_NPC(ch) ||
      context->argument == NULL || IN_ROOM(obj) != IN_ROOM(ch) || GET_LEVEL(ch) < 15)
    return FALSE;

  one_argument(context->argument, keyword, sizeof(keyword));
  if (*keyword == '\0' || !isname(keyword, obj->name))
    return FALSE;

  destination = real_room(GET_OBJ_VAL(obj, 0));
  if (!VALID_ROOM_RNUM(destination))
  {
    log("SYSERR: RoL Acheron portal %d has unavailable destination %d", GET_OBJ_VNUM(obj),
        GET_OBJ_VAL(obj, 0));
    send_to_char(ch, "The portal twists away from this world. Please tell a staff member.\r\n");
    return TRUE;
  }

  act("$n steps into $p and vanishes.", TRUE, ch, obj, NULL, TO_ROOM);
  send_to_char(ch, "You step into the portal and the planes wrench around you.\r\n");
  char_from_room(ch);
  char_to_room(ch, destination);
  GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - rand_number(1, 10));
  act("$n steps out of a tearing planar portal.", TRUE, ch, NULL, NULL, TO_ROOM);
  look_at_room(ch, 0);
  return TRUE;
}

static int rol_utility_acheron_relocate(struct obj_data *obj)
{
  room_rnum destination;
  int candidate;
  int current_vnum;
  int attempts;

  if (!VALID_ROOM_RNUM(IN_ROOM(obj)) || GET_OBJ_SPECTIMER(obj, 0) > 0)
    return FALSE;

  current_vnum = GET_ROOM_VNUM(IN_ROOM(obj));
  for (attempts = 0; attempts < 100; attempts++)
  {
    if (GET_OBJ_VNUM(obj) == ROL_ACHERON_PLATFORM_PORTAL_VNUM)
    {
      candidate = rand_number(2050000, 2050099);
      if (!rol_utility_acheron_platform_room_allowed(current_vnum, candidate))
        continue;
    }
    else
    {
      candidate = rand_number(2050100, 2050184);
      if (!rol_utility_acheron_roaming_room_allowed(candidate))
        continue;
    }
    destination = real_room(candidate);
    if (!VALID_ROOM_RNUM(destination))
      continue;

    send_to_room(IN_ROOM(obj), "A planar portal shimmers and slips sideways out of reality.\r\n");
    obj_from_room(obj);
    obj_to_room(obj, destination);
    send_to_room(destination, "A planar portal shimmers into existence.\r\n");
    point_update_object_spec_timer_set(
        obj, 0, GET_OBJ_VNUM(obj) == ROL_ACHERON_PLATFORM_PORTAL_VNUM ? 3 : 1);
    return TRUE;
  }

  log("SYSERR: RoL Acheron portal %d found no available relocation room", GET_OBJ_VNUM(obj));
  point_update_object_spec_timer_set(obj, 0, 1);
  return FALSE;
}

static int rol_utility_proximity_explosion(struct spec_event_context *context, struct obj_data *obj)
{
  struct char_data *owner;

  owner = context->actor;
  if (owner == NULL || !IS_NPC(owner) || obj->worn_by != owner || owner->master == NULL ||
      IN_ROOM(owner) == IN_ROOM(owner->master))
    return FALSE;

  act("$p crackles with magical energy before exploding and consuming $n!", FALSE, owner, obj, NULL,
      TO_ROOM);
  die(owner, owner);
  context->invalidation |= SPEC_INVALIDATE_ACTOR;
  return TRUE;
}

static int rol_utility_hyssk_skeleton(struct char_data *ch, struct obj_data *obj, int cmd,
                                      const char *argument)
{
  room_rnum destination;
  char keyword[MAX_INPUT_LENGTH];

  if (!rol_utility_command_is(cmd, "bow") || IS_NPC(ch) || !IS_WIZARD(ch) || argument == NULL ||
      IN_ROOM(obj) != IN_ROOM(ch))
    return FALSE;
  one_argument(argument, keyword, sizeof(keyword));
  if (*keyword == '\0' || !isname(keyword, obj->name))
    return FALSE;
  destination = real_room(ROL_HYSSK_ILLUSION_DESTINATION_VNUM);
  if (!VALID_ROOM_RNUM(destination))
  {
    log("SYSERR: RoL Hyssk skeleton cannot find destination %d",
        ROL_HYSSK_ILLUSION_DESTINATION_VNUM);
    return TRUE;
  }

  act("$n bows before $p and disappears!", TRUE, ch, obj, NULL, TO_ROOM);
  send_to_char(ch, "The disguised illusionist allows you to pass in a burst of magic.\r\n");
  char_from_room(ch);
  char_to_room(ch, destination);
  act("$n slowly fades into view.", TRUE, ch, NULL, NULL, TO_ROOM);
  look_at_room(ch, 0);
  return TRUE;
}

static int rol_utility_tattered_cloak(struct spec_event_context *context, struct char_data *ch,
                                      struct obj_data *obj)
{
  if (context->event == SPEC_EVENT_OBJECT_AUTOMATIC)
  {
    if (ch != NULL && obj->worn_by == ch)
      GET_OBJ_VAL(obj, 3) = MIN(144, GET_OBJ_VAL(obj, 3) + 1);
    else
      GET_OBJ_VAL(obj, 3) = 0;
    return FALSE;
  }
  if (ch == NULL || !rol_utility_say_phrase(context, "thief's haven") || obj->worn_by != ch)
    return FALSE;
  if (GET_OBJ_VAL(obj, 3) < 144)
  {
    send_to_char(ch, "Your cloak has not recovered yet.\r\n");
    return TRUE;
  }
  act("You mutter 'thief's haven' to your $p.", FALSE, ch, obj, NULL, TO_CHAR);
  act("$n mutters something to $s $p.", FALSE, ch, obj, NULL, TO_ROOM);
  (void)call_magic(ch, ch, NULL, SPELL_INVISIBLE, 0, 35, CAST_INNATE);
  (void)call_magic(ch, ch, NULL, SPELL_DETECT_INVIS, 0, 35, CAST_INNATE);
  GET_OBJ_VAL(obj, 3) = 0;
  return TRUE;
}

static int rol_utility_qogek_staff(struct spec_event_context *context, struct char_data *ch,
                                   struct obj_data *obj)
{
  struct obj_data *corpse;
  char command[MAX_INPUT_LENGTH];
  char target[MAX_INPUT_LENGTH];

  if (ch == NULL || obj->worn_by != ch || !rol_utility_command_is(context->command, "say") ||
      context->argument == NULL)
    return FALSE;
  two_arguments(context->argument, command, sizeof(command), target, sizeof(target));
  if (!strcmp(command, "blood"))
  {
    if (!IS_NECROMANCER(ch) || GET_OBJ_VAL(obj, 0) <= 0)
    {
      send_to_char(ch, IS_NECROMANCER(ch) ? "Your staff has run out of blood-rain charges.\r\n"
                                          : "The staff's magic is beyond your understanding.\r\n");
      return TRUE;
    }
    act("Your $p erupts in a violent torrent of caustic blood!", FALSE, ch, obj, NULL, TO_CHAR);
    act("$n's $p erupts in a violent torrent of caustic blood!", FALSE, ch, obj, NULL, TO_ROOM);
    (void)call_magic(ch, ch, NULL, SPELL_CAUSTIC_BLOOD, 0, 50, CAST_INNATE);
    GET_OBJ_VAL(obj, 0)--;
    return TRUE;
  }
  if (strcmp(command, "animate") || *target == '\0')
    return FALSE;
  if (!IS_NECROMANCER(ch) || GET_OBJ_VAL(obj, 1) <= 0)
  {
    send_to_char(ch, IS_NECROMANCER(ch) ? "Your staff has run out of animation charges.\r\n"
                                        : "The staff's magic is beyond your understanding.\r\n");
    return TRUE;
  }
  corpse = get_obj_in_list_vis(ch, target, NULL, world[IN_ROOM(ch)].contents);
  if (corpse == NULL || !IS_CORPSE(corpse))
  {
    send_to_char(ch, "No such corpse lies here.\r\n");
    return TRUE;
  }
  act("You touch $p to $P, feeding the corpse a jolt of unholy energy.", FALSE, ch, obj, corpse,
      TO_CHAR);
  (void)call_magic(ch, NULL, corpse, SPELL_EMBALM, 0, 50, CAST_INNATE);
  (void)call_magic(ch, NULL, corpse, SPELL_ANIMATE_DEAD, 0, 50, CAST_INNATE);
  GET_OBJ_VAL(obj, 1)--;
  return TRUE;
}

int rol_utility_loot_sweep_interval_seconds(void)
{
  return ROL_LOOT_SWEEP_SECONDS;
}

int rol_utility_orchid_decay_hours(void)
{
  return ROL_BLACK_ORCHID_DECAY_HOURS;
}

bool rol_utility_loot_blockable_container(const struct obj_data *obj)
{
  if (obj == NULL)
    return false;
  if (IS_CORPSE(obj))
    return GET_OBJ_VAL(obj, 4) == 0;
  return GET_OBJ_TYPE(obj) == ITEM_CONTAINER;
}

bool rol_utility_plague_eligible(struct char_data *ch, const struct obj_data *obj)
{
  if (ch == NULL || obj == NULL || IS_NPC(ch) || GET_LEVEL(ch) < ROL_PLAGUE_MINIMUM_LEVEL ||
      !VALID_ROOM_RNUM(IN_ROOM(ch)) || IN_ROOM(obj) != IN_ROOM(ch) ||
      GET_ROOM_VNUM(IN_ROOM(ch)) != ROL_PLAGUE_RESERVOIR_ROOM_VNUM)
    return false;

  return !AFF_FLAGGED(ch, AFF_DISEASE) && !affected_by_spell(ch, SPELL_CONTAGION);
}

static int rol_utility_plague_reservoir(struct char_data *ch, struct obj_data *obj, int cmd)
{
  if (!rol_utility_command_is(cmd, "drink") && !rol_utility_command_is(cmd, "fill"))
    return FALSE;
  if (!rol_utility_plague_eligible(ch, obj))
    return FALSE;

  (void)call_magic(ch, ch, NULL, SPELL_CONTAGION, 0, MAX(GET_LEVEL(ch), 15), CAST_INNATE);
  return FALSE;
}

static int rol_utility_loot_sweep(struct obj_data *obj)
{
  struct obj_data *corpse;
  time_t now;

  if (obj == NULL || !VALID_ROOM_RNUM(IN_ROOM(obj)))
    return FALSE;

  now = time(NULL);
  if (obj->rol_loot_sweep_at == 0)
  {
    obj->rol_loot_sweep_at = now + ROL_LOOT_SWEEP_SECONDS;
    return FALSE;
  }
  if (obj->rol_loot_sweep_at > now)
    return FALSE;
  obj->rol_loot_sweep_at = now + ROL_LOOT_SWEEP_SECONDS;

  for (corpse = world[IN_ROOM(obj)].contents; corpse != NULL; corpse = corpse->next_content)
  {
    if (!IS_CORPSE(corpse) || GET_OBJ_VAL(corpse, 4) != 0 || OBJ_FLAGGED(corpse, ITEM_MAGIC))
      continue;

    if (GET_OBJ_TIMER(corpse) <= 0 || GET_OBJ_TIMER(corpse) > ROL_LOOT_DECAY_TICKS)
      GET_OBJ_TIMER(corpse) = ROL_LOOT_DECAY_TICKS;
    SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_MAGIC);
    point_update_object_sync(corpse);
  }

  return TRUE;
}

static int rol_utility_orchid_decay(struct obj_data *obj)
{
  if (obj == NULL || OBJ_FLAGGED(obj, ITEM_DECAY))
    return FALSE;

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_DECAY);
  GET_OBJ_TIMER(obj) = ROL_BLACK_ORCHID_DECAY_HOURS;
  point_update_object_sync(obj);
  return FALSE;
}

static struct char_data *rol_utility_loot_aggressor(room_rnum room)
{
  struct char_data *candidate;

  if (!VALID_ROOM_RNUM(room))
    return NULL;
  for (candidate = world[room].people; candidate != NULL; candidate = candidate->next_in_room)
    if (IS_NPC(candidate) && MOB_FLAGGED(candidate, MOB_AGGRESSIVE))
      return candidate;
  return NULL;
}

static int rol_utility_loot_block(struct char_data *ch, struct obj_data *obj, int cmd,
                                  const char *argument)
{
  struct char_data *aggressor;
  struct obj_data *container;
  char first[MAX_INPUT_LENGTH];
  char second[MAX_INPUT_LENGTH];

  if (ch == NULL || obj == NULL || argument == NULL ||
      (!rol_utility_command_is(cmd, "get") && !rol_utility_command_is(cmd, "take") &&
       !rol_utility_command_is(cmd, "drag")) ||
      (IS_NPC(ch) && !IS_PET(ch)) || !VALID_ROOM_RNUM(IN_ROOM(ch)) || IN_ROOM(obj) != IN_ROOM(ch))
    return FALSE;

  aggressor = rol_utility_loot_aggressor(IN_ROOM(ch));
  if (aggressor == NULL)
    return FALSE;

  two_arguments(argument, first, sizeof(first), second, sizeof(second));
  if (first[0] == '\0' || second[0] == '\0')
    return FALSE;

  if (rol_utility_command_is(cmd, "drag"))
    container = get_obj_in_list_vis(ch, first, NULL, world[IN_ROOM(ch)].contents);
  else
  {
    if (get_obj_in_list_vis(ch, second, NULL, ch->carrying) != NULL)
      return FALSE;
    container = get_obj_in_list_vis(ch, second, NULL, world[IN_ROOM(ch)].contents);
  }
  if (!rol_utility_loot_blockable_container(container) ||
      (rol_utility_command_is(cmd, "drag") && !IS_CORPSE(container)))
    return FALSE;

  act("$n stands protectively over $p, blocking you!", TRUE, aggressor, container, ch, TO_VICT);
  return TRUE;
}

static int rol_utility_held_slot(const struct char_data *ch, const struct obj_data *obj)
{
  static const int slots[] = {WEAR_WIELD_1, WEAR_HOLD_1,   WEAR_WIELD_OFFHAND,
                              WEAR_HOLD_2,  WEAR_WIELD_2H, WEAR_HOLD_2H};
  size_t index;

  if (ch == NULL || obj == NULL || obj->worn_by != ch)
    return -1;

  for (index = 0; index < sizeof(slots) / sizeof(slots[0]); index++)
    if (GET_EQ(ch, slots[index]) == obj)
      return slots[index];

  return -1;
}

static int rol_utility_treasure_slot(const struct char_data *ch, const char *keyword)
{
  static const int slots[] = {WEAR_HOLD_1, WEAR_HOLD_2, WEAR_HOLD_2H};
  size_t index;

  if (ch == NULL || keyword == NULL)
    return -1;
  for (index = 0; index < sizeof(slots) / sizeof(slots[0]); index++)
    if (GET_EQ(ch, slots[index]) != NULL &&
        GET_OBJ_TYPE(GET_EQ(ch, slots[index])) == ITEM_TREASURE &&
        isname(keyword, GET_EQ(ch, slots[index])->name))
      return slots[index];
  return -1;
}

bool rol_utility_sacrifice_keyword(const char *argument)
{
  char keyword[MAX_INPUT_LENGTH];

  if (argument == NULL)
    return false;

  one_argument(argument, keyword, sizeof(keyword));
  return !strcasecmp(keyword, "child") || !strcasecmp(keyword, "all");
}

bool rol_utility_sacrifice_command_name(const char *command)
{
  return command != NULL &&
         (!strcmp(command, "get") || !strcmp(command, "take") || !strcmp(command, "drag"));
}

const char *rol_utility_necro_child_message(int roll)
{
  static const char *const messages[] = {
      "The necromancer's child screams at the top of his lungs!\r\n",
      "The necromancer's child throws a fit!\r\n",
      "The necromancer's child shrieks in anger!\r\n",
      "The necromancer's child struggles and screams!\r\n",
      "The necromancer's child kicks and spits in fury!\r\n",
      "The necromancer's child glares at you evilly!\r\n",
      "The necromancer's child tries to bite you!\r\n",
  };

  if (roll < 0 || roll >= (int)(sizeof(messages) / sizeof(messages[0])))
    return NULL;
  return messages[roll];
}

bool rol_utility_monocle_room(int room_vnum)
{
  return room_vnum >= ROL_RUBY_MONOCLE_ROOM_MIN && room_vnum <= ROL_RUBY_MONOCLE_ROOM_MAX;
}

static int rol_utility_child_sacrifice(struct spec_event_context *context, struct char_data *ch,
                                       struct obj_data *obj)
{
  int damage_amount;

  if (!rol_utility_sacrifice_command_name(rol_utility_command_name(context->command)) ||
      !AWAKE(ch) || !VALID_ROOM_RNUM(IN_ROOM(obj)) || IN_ROOM(ch) != IN_ROOM(obj) ||
      !rol_utility_sacrifice_keyword(context->argument))
    return FALSE;

  act("You attempt to pick up $p...\r\n"
      "Suddenly, a wave of revulsion rushes through you!\r\n"
      "You drop $p, leaving your hands reddened and sore.",
      FALSE, ch, obj, NULL, TO_CHAR);
  act("As $n attempts to pick up $p, $e shudders and suddenly drops it.", TRUE, ch, obj, NULL,
      TO_ROOM);

  damage_amount = rand_number(1, 9);
  if (GET_HIT(ch) - damage_amount < -10)
  {
    act("Your wounds prove too much for you!", FALSE, ch, NULL, NULL, TO_CHAR);
    act("$n's wounds prove too much for $m!", TRUE, ch, NULL, NULL, TO_ROOM);
    GET_HIT(ch) = -11;
    update_pos(ch);
    die(ch, ch);
    context->invalidation |= SPEC_INVALIDATE_ACTOR;
  }
  else
  {
    combat_apply_raw_damage(ch, NULL, damage_amount, DAM_RESERVED_DBC, INT_MIN);
    update_pos(ch);
  }

  return TRUE;
}

static int rol_utility_goodberry(struct char_data *ch, struct obj_data *obj, int cmd,
                                 const char *argument)
{
  struct char_data *dummy_character;
  struct obj_data *selected;

  if (!rol_utility_command_is(cmd, "eat") || !AWAKE(ch) || obj->carried_by != ch ||
      GET_COND(ch, HUNGER) > 20 || argument == NULL ||
      !generic_find(argument, FIND_OBJ_INV, ch, &dummy_character, &selected) || selected != obj)
    return FALSE;

  /* The source schedules this cure one tick after the native eat. Selecting
   * the first matching carried object prevents duplicate healing while the
   * target's native eat command consumes the same berry. */
  call_magic(ch, ch, NULL, SPELL_CURE_LIGHT, 0, 50, CAST_INNATE);
  return FALSE;
}

static int rol_utility_menden_figurine(struct spec_event_context *context, struct char_data *ch,
                                       struct obj_data *obj)
{
  struct char_data *summoned;
  struct obj_data *removed;
  char name[MAX_INPUT_LENGTH];
  int slot;

  if (!rol_utility_command_is(context->command, "flex") || !AWAKE(ch) ||
      (slot = rol_utility_held_slot(ch, obj)) < 0 || context->argument == NULL)
    return FALSE;

  one_argument(context->argument, name, sizeof(name));
  if (*name == '\0' || !isname(name, obj->name))
    return FALSE;

  summoned = read_mobile(GET_OBJ_VAL(obj, 0), VIRTUAL);
  if (summoned == NULL)
  {
    log("SYSERR: RoL Menden figurine %d cannot load mobile %d", GET_OBJ_VNUM(obj),
        GET_OBJ_VAL(obj, 0));
    send_to_char(ch, "The figurine fails to awaken. Please tell a staff member.\r\n");
    return TRUE;
  }

  char_to_room(summoned, IN_ROOM(ch));
  load_mtrigger(summoned);
  SET_BIT_AR(AFF_FLAGS(summoned), AFF_CHARM);

  removed = unequip_char(ch, slot);
  if (removed != obj)
  {
    log("SYSERR: RoL Menden figurine changed held slot during command dispatch");
    if (removed != NULL)
      equip_char(ch, removed, slot);
    extract_char(summoned);
    return TRUE;
  }

  act("You flex $p several times, until it finally snaps!", TRUE, ch, obj, NULL, TO_CHAR);
  act("$n flexes $p several times, until it finally snaps!", TRUE, ch, obj, NULL, TO_ROOM);
  act("From the $o come swirling vapors, which solidify to form $n.", TRUE, summoned, obj, NULL,
      TO_ROOM);
  act("The $o rapidly disintegrates to powder, only to be borne away by a sudden wind.", TRUE,
      summoned, obj, NULL, TO_ROOM);
  add_follower(summoned, ch);
  extract_obj(removed);
  context->invalidation |= SPEC_INVALIDATE_OWNER;
  return TRUE;
}

static int rol_utility_necro_child(struct char_data *ch, struct obj_data *obj)
{
  const char *message;

  if (ch == NULL || obj->carried_by != ch || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) || AFF_FLAGGED(ch, AFF_SILENCED))
    return FALSE;

  message = rol_utility_necro_child_message(rand_number(0, 10));
  if (message != NULL)
    send_to_char(ch, "%s", message);
  return FALSE;
}

static int rol_utility_ruby_monocle(struct obj_data *obj)
{
  room_rnum destination;
  room_rnum room;
  zone_rnum zone;
  int destination_vnum;

  room = IN_ROOM(obj);
  if (!VALID_ROOM_RNUM(room) || !rol_utility_monocle_room(GET_ROOM_VNUM(room)))
    return FALSE;

  zone = world[room].zone;
  if (zone > top_of_zone_table || zone_table[zone].age != 0)
    return FALSE;

  destination_vnum = rand_number(ROL_RUBY_MONOCLE_ROOM_MIN, ROL_RUBY_MONOCLE_ROOM_MAX);
  destination = real_room(destination_vnum);
  if (!VALID_ROOM_RNUM(destination))
  {
    log("SYSERR: RoL ruby monocle selected unavailable room %d", destination_vnum);
    return FALSE;
  }

  obj_from_room(obj);
  obj_to_room(obj, destination);
  return TRUE;
}

static bool rol_utility_say_phrase(const struct spec_event_context *context, const char *phrase)
{
  const char *argument;

  if (context == NULL || phrase == NULL || context->command <= 0 || context->argument == NULL ||
      (!rol_utility_command_is(context->command, "say") &&
       !rol_utility_command_is(context->command, "'")))
    return false;

  argument = context->argument;
  skip_spaces_c(&argument);
  return !strcmp(argument, phrase);
}

static int rol_utility_magius_staff(struct spec_event_context *context, struct char_data *ch,
                                    struct obj_data *obj)
{
  const char *argument;

  if (context->argument == NULL || context->command <= 0 ||
      (!rol_utility_command_is(context->command, "say") &&
       !rol_utility_command_is(context->command, "'")))
    return FALSE;

  argument = context->argument;
  skip_spaces_c(&argument);
  if (!strcmp(argument, "shirak"))
  {
    if (OBJ_FLAGGED(obj, ITEM_GLOW))
    {
      send_to_char(ch, "Your Staff of Magius is already lit.\r\n");
      return FALSE;
    }
    SET_OBJ_FLAG(obj, ITEM_GLOW);
    act("As you say 'shirak' to $p, the crystal atop it flares to life.", FALSE, ch, obj, NULL,
        TO_CHAR);
    act("As $n murmurs to $p, the crystal atop it flares to life.", FALSE, ch, obj, NULL, TO_ROOM);
    return TRUE;
  }
  if (strcmp(argument, "dulak"))
    return FALSE;
  if (!OBJ_FLAGGED(obj, ITEM_GLOW))
  {
    send_to_char(ch, "Your Staff of Magius is already darkened.\r\n");
    return FALSE;
  }
  REMOVE_OBJ_FLAG(obj, ITEM_GLOW);
  act("As you say 'dulak' to $p, the crystal atop it goes dark.", FALSE, ch, obj, NULL, TO_CHAR);
  act("As $n murmurs to $p, the crystal atop it goes dark.", FALSE, ch, obj, NULL, TO_ROOM);
  return TRUE;
}

static int rol_utility_summon_basilisk_snake(struct char_data *ch, struct obj_data *obj)
{
  struct char_data *summoned;

  summoned = read_mobile(ROL_BASILISK_SNAKE_VNUM, VIRTUAL);
  if (summoned == NULL)
  {
    log("SYSERR: RoL basilisk snakes object %d cannot load mobile %d", GET_OBJ_VNUM(obj),
        ROL_BASILISK_SNAKE_VNUM);
    send_to_char(ch, "The snakes fail to awaken. Please tell a staff member.\r\n");
    return TRUE;
  }

  act("The larger snake on $p crawls free and grows to answer your call.", FALSE, ch, obj, NULL,
      TO_CHAR);
  act("The larger snake on $n's $p crawls free and grows to answer $s call.", FALSE, ch, obj, NULL,
      TO_ROOM);
  char_to_room(summoned, IN_ROOM(ch));
  SET_BIT_AR(AFF_FLAGS(summoned), AFF_CHARM);
  add_follower(summoned, ch);
  (void)spec_object_cooldown_commit(obj, 0, 48);

  /* Complete ownership before load triggers, which may extract the summon. */
  load_mtrigger(summoned);
  return TRUE;
}

static void rol_utility_earthmother_aid(struct char_data *ch)
{
  static const int spells[] = {SPELL_ICE_STORM,  SPELL_CHAIN_LIGHTNING, SPELL_WHIRLWIND,
                               SPELL_EARTHQUAKE, SPELL_FIRE_STORM,      SPELL_SUNBEAM,
                               SPELL_ICE_STORM};
  int spell = spells[rand_number(0, (int)(sizeof(spells) / sizeof(spells[0])) - 1)];

  (void)call_magic(ch, ch, NULL, spell, 0, 35, CAST_INNATE);
}

static int rol_utility_lathander_disc(struct spec_event_context *context, struct char_data *ch,
                                      struct obj_data *obj)
{
  char keyword[MAX_INPUT_LENGTH];

  if (!rol_utility_command_is(context->command, "rub") || IS_NPC(ch) || context->argument == NULL)
    return FALSE;
  one_argument(context->argument, keyword, sizeof(keyword));
  if (*keyword == '\0' || !isname(keyword, obj->name))
    return FALSE;

  act("Your $p glows and dissolves into a white cloud that tears through your body.", FALSE, ch,
      obj, NULL, TO_CHAR);
  act("$n's $p glows and dissolves as a white cloud surrounds $m.", FALSE, ch, obj, NULL, TO_ROOM);
  ch->player.time.birth = time(NULL);
  GET_POS(ch) = POS_SLEEPING;
  if (!char_has_mud_event(ch, eSTUNNED))
    attach_mud_event(new_mud_event(eSTUNNED, ch, NULL), PULSE_VIOLENCE * rand_number(5, 8));
  SET_WAIT(ch, PULSE_VIOLENCE);
  extract_obj(obj);
  context->invalidation |= SPEC_INVALIDATE_OWNER;
  return TRUE;
}

static void rol_utility_llyms_summon(struct char_data *ch, int mobile_vnum)
{
  struct char_data *summoned;

  summoned = read_mobile(mobile_vnum, VIRTUAL);
  if (summoned == NULL)
  {
    log("SYSERR: RoL Llym altar cannot load mobile %d", mobile_vnum);
    send_to_char(ch, "Llym's promised servant fails to appear. Please tell a staff member.\r\n");
    return;
  }
  char_to_room(summoned, IN_ROOM(ch));
  SET_BIT_AR(AFF_FLAGS(summoned), AFF_CHARM);
  add_follower(summoned, ch);
  act("$n appears in a flash of blue light!", TRUE, summoned, NULL, NULL, TO_ROOM);
  load_mtrigger(summoned);
}

static void rol_utility_llyms_reward(struct char_data *ch, int object_vnum)
{
  struct obj_data *reward;

  reward = read_object(object_vnum, VIRTUAL);
  if (reward == NULL)
  {
    log("SYSERR: RoL Llym altar cannot load object %d", object_vnum);
    send_to_char(ch, "Llym's promised reward fails to appear. Please tell a staff member.\r\n");
    return;
  }
  obj_to_room(reward, IN_ROOM(ch));
  act("$p appears before you in a flash of blue light!", TRUE, ch, reward, NULL, TO_CHAR);
  act("$p appears in a flash of blue light!", TRUE, ch, reward, NULL, TO_ROOM);
  load_otrigger(reward);
}

static int rol_utility_llyms_altar(struct spec_event_context *context, struct char_data *ch,
                                   struct obj_data *obj)
{
  struct obj_data *treasure;
  char altar_name[MAX_INPUT_LENGTH];
  char treasure_name[MAX_INPUT_LENGTH];
  int bonus;
  int slot;

  if (!rol_utility_command_is(context->command, "offer") || !AWAKE(ch) ||
      context->argument == NULL || !VALID_ROOM_RNUM(IN_ROOM(obj)) || IN_ROOM(obj) != IN_ROOM(ch))
    return FALSE;
  two_arguments(context->argument, treasure_name, sizeof(treasure_name), altar_name,
                sizeof(altar_name));
  if (*treasure_name == '\0' || *altar_name == '\0' || !isname(altar_name, obj->name) ||
      (slot = rol_utility_treasure_slot(ch, treasure_name)) < 0)
    return FALSE;
  treasure = GET_EQ(ch, slot);

  act("You offer $p to $P.", TRUE, ch, treasure, obj, TO_CHAR);
  act("$n offers $p to $P.", TRUE, ch, treasure, obj, TO_ROOM);
  if (GET_OBJ_COST(treasure) < 10000)
  {
    act("$P rumbles briefly, rejecting the inadequate offering.", TRUE, ch, treasure, obj, TO_CHAR);
    return TRUE;
  }

  treasure = unequip_char(ch, slot);
  if (treasure == NULL)
    return TRUE;
  extract_obj(treasure);
  if (!affected_by_spell(ch, SPELL_BLESS))
    (void)call_magic(ch, ch, NULL, SPELL_BLESS, 0, 50, CAST_INNATE);
  else
  {
    GET_GOLD(ch) += rand_number(1, 100);
    send_to_char(ch, "Your purse suddenly feels heavier!\r\n");
  }

  bonus = rand_number(1, 50);
  switch (bonus)
  {
  case 1:
    rol_utility_llyms_summon(ch, ROL_LLYMS_SERVANT_ONE_VNUM);
    break;
  case 2:
    rol_utility_llyms_summon(ch, ROL_LLYMS_SERVANT_TWO_VNUM);
    break;
  case 3:
    rol_utility_llyms_reward(ch, ROL_LLYMS_REWARD_ONE_VNUM);
    break;
  case 4:
    rol_utility_llyms_reward(ch, ROL_LLYMS_REWARD_TWO_VNUM);
    break;
  case 5:
    rol_utility_llyms_reward(ch, ROL_LLYMS_REWARD_THREE_VNUM);
    break;
  case 6:
    if (!affected_by_spell(ch, SPELL_AID))
      (void)call_magic(ch, ch, NULL, SPELL_AID, 0, 60, CAST_INNATE);
    break;
  default:
    break;
  }
  return TRUE;
}

static int rol_utility_smoke_shield(struct spec_event_context *context, struct char_data *ch,
                                    struct obj_data *obj)
{
  struct char_data *victim = FIGHTING(ch);
  struct spec_damage_result result;
  int amount;

  if (victim == NULL || GET_EQ(ch, WEAR_SHIELD) != obj ||
      spec_context_validate_combat_target(ch, victim, true) != SPEC_CONTEXT_VALID)
    return FALSE;
  context->target = victim;
  if (context->event == SPEC_EVENT_COMBAT_MANEUVER && context->argument != NULL &&
      !strcmp(context->argument, "shieldpunch") && rand_number(0, 9) == 0)
  {
    act("A crackling bolt leaps from your $p and violently jolts $N!", FALSE, ch, obj, victim,
        TO_CHAR);
    if (can_stun(victim) && !char_has_mud_event(victim, eSTUNNED))
      attach_mud_event(new_mud_event(eSTUNNED, victim, NULL), PULSE_VIOLENCE * 2);
    SET_WAIT(ch, PULSE_VIOLENCE * 2);
    return TRUE;
  }
  if (context->event != SPEC_EVENT_DEFENSE_REACTION || context->argument == NULL ||
      strcmp(context->argument, "shieldblock") || rand_number(0, 9) != 0)
    return FALSE;

  act("Your $p discharges a violent arc of electricity into $N!", FALSE, ch, obj, victim, TO_CHAR);
  amount = affected_by_spell(victim, SPELL_RESIST_ENERGY) ? dice(3, 10) : dice(6, 10);
  if (GET_HIT(victim) < amount)
    return FALSE;
  result = spec_damage_current_target(ch, victim, amount, -1, DAM_ELECTRIC, FALSE);
  if (result.status == SPEC_DAMAGE_TARGET_INVALIDATED)
    context->invalidation |= SPEC_INVALIDATE_TARGET;
  return TRUE;
}

static int rol_utility_called_effect(struct spec_event_context *context,
                                     const struct rol_utility_called_profile *profile,
                                     struct char_data *ch, struct obj_data *obj)
{
  struct char_data *victim = NULL;
  struct spec_object_cooldown_state cooldown;
  int result;

  if (profile->effect == ROL_UTILITY_MAGIUS_STAFF)
    return rol_utility_magius_staff(context, ch, obj);
  if (!rol_utility_say_phrase(context, profile->phrase) ||
      spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID)
    return FALSE;

  if (profile->effect == ROL_UTILITY_HASTE_SLEEVES && AFF_FLAGGED(ch, AFF_HASTE))
  {
    send_to_char(ch, "You are already moving too fast.\r\n");
    return FALSE;
  }
  if (profile->effect == ROL_UTILITY_WRITHING_ASH &&
      ((victim = FIGHTING(ch)) == NULL ||
       spec_context_validate_combat_target(ch, victim, true) != SPEC_CONTEXT_VALID))
    return FALSE;

  cooldown = spec_object_cooldown_read(obj, 0);
  if (cooldown.status != SPEC_OBJECT_COOLDOWN_READY)
  {
    act("The magic in $p does not answer your call.", FALSE, ch, obj, NULL, TO_CHAR);
    return TRUE;
  }

  switch (profile->effect)
  {
  case ROL_UTILITY_BASILISK_LEGGINGS:
    act("The scales of $p glow red as your skin hardens like stone.", FALSE, ch, obj, NULL,
        TO_CHAR);
    (void)call_magic(ch, ch, NULL, SPELL_STONESKIN, 0, 35, CAST_INNATE);
    break;
  case ROL_UTILITY_BASILISK_SNAKES:
    return rol_utility_summon_basilisk_snake(ch, obj);
  case ROL_UTILITY_DRAGONCULT_ROBES:
    act("Five spectral dragons rise from $p and surround you with elemental protection.", FALSE, ch,
        obj, NULL, TO_CHAR);
    (void)call_magic(ch, ch, NULL, SPELL_RESIST_ENERGY, 0, 35, CAST_INNATE);
    break;
  case ROL_UTILITY_CRESCENT_MOON:
    act("Moonlight spills from your $p as shadows wrap around you.", FALSE, ch, obj, NULL, TO_CHAR);
    (void)call_magic(ch, ch, NULL, SPELL_INVISIBLE, 0, 51, CAST_INNATE);
    SET_WAIT(ch, PULSE_VIOLENCE);
    break;
  case ROL_UTILITY_HASTE_SLEEVES:
    act("Your $p vibrates with power and every motion accelerates.", FALSE, ch, obj, NULL, TO_CHAR);
    (void)call_magic(ch, ch, NULL, SPELL_HASTE, 0, 35, CAST_INNATE);
    break;
  case ROL_UTILITY_EARTHMOTHER_STAFF:
    act("You raise $p and invoke the Earthmother's elemental aid!", FALSE, ch, obj, NULL, TO_CHAR);
    rol_utility_earthmother_aid(ch);
    break;
  case ROL_UTILITY_BLUEPLUME:
    act("Sky-blue light erupts from $p as Tyr grants you might.", FALSE, ch, obj, NULL, TO_CHAR);
    (void)call_magic(ch, ch, NULL, SPELL_STRENGTH, 0, 35, CAST_INNATE);
    (void)call_magic(ch, ch, NULL, SPELL_ARMOR, 0, 35, CAST_INNATE);
    if (FIGHTING(ch) == NULL)
      (void)call_magic(ch, ch, NULL, SPELL_BLESS, 0, 35, CAST_INNATE);
    break;
  case ROL_UTILITY_WRITHING_ASH:
    context->target = victim;
    act("Black energy streams from your $p and strikes $N in the chest!", FALSE, ch, obj, victim,
        TO_CHAR);
    result = call_magic(ch, victim, NULL, SPELL_ENERGY_DRAIN, 0, 35, CAST_INNATE);
    if (result < 0)
      context->invalidation |= SPEC_INVALIDATE_TARGET;
    if (!affected_by_spell(ch, SPELL_SHIELD) && !affected_by_spell(ch, SPELL_MAGE_ARMOR) &&
        !affected_by_spell(ch, SPELL_ARMOR))
      (void)call_magic(ch, ch, NULL, SPELL_SHIELD, 0, 40, CAST_INNATE);
    break;
  case ROL_UTILITY_MAGIUS_STAFF:
    return FALSE;
  }

  (void)spec_object_cooldown_commit(obj, 0, profile->cooldown_hours);
  return TRUE;
}

int rol_utility_object(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  /* Typed dispatch supplies the exact owner, event, and invalidation contract. */
  return FALSE;
}

int rol_utility_object_typed(struct spec_event_context *context)
{
  const struct rol_utility_called_profile *profile;
  struct char_data *ch;
  struct obj_data *obj;

  if (context == NULL || context->owner_type != SPEC_OWNER_OBJECT || context->owner == NULL)
    return FALSE;

  obj = context->owner;
  if (!VALID_OBJ_RNUM(obj))
    return FALSE;

  profile = rol_utility_called_profile_for(GET_OBJ_VNUM(obj));
  if (context->event == SPEC_EVENT_ITEM_IDENTIFY)
  {
    const char *description = rol_utility_description_for(GET_OBJ_VNUM(obj));

    if (description == NULL && GET_OBJ_VNUM(obj) == ROL_UM_TATTERED_CLOAK_VNUM)
      description = "Say 'thief's haven' after 144 worn pulses for invisibility and detection.";
    if (description == NULL && GET_OBJ_VNUM(obj) == ROL_UM_QOGEK_STAFF_VNUM)
      description = "Say 'blood' or 'animate <corpse>' to spend the corresponding charge.";

    if (description == NULL || context->actor == NULL)
      return FALSE;
    send_to_char(context->actor, "Special Effects: %s\r\n", description);
    return TRUE;
  }

  if ((context->event == SPEC_EVENT_DEFENSE_REACTION ||
       context->event == SPEC_EVENT_COMBAT_MANEUVER) &&
      GET_OBJ_VNUM(obj) == ROL_SMOKE_STUN_SHIELD_VNUM && context->actor != NULL)
    return rol_utility_smoke_shield(context, context->actor, obj);

  if (context->event == SPEC_EVENT_OBJECT_AUTOMATIC)
  {
    if (rol_utility_acheron_portal_vnum(GET_OBJ_VNUM(obj)) &&
        GET_OBJ_VNUM(obj) != ROL_ACHERON_ENTRANCE_PORTAL_VNUM)
      return rol_utility_acheron_relocate(obj);
    if (GET_OBJ_VNUM(obj) == ROL_PROXIMITY_EXPLOSION_VNUM)
      return rol_utility_proximity_explosion(context, obj);
    if (GET_OBJ_VNUM(obj) == ROL_UM_TATTERED_CLOAK_VNUM)
      return rol_utility_tattered_cloak(context, context->actor, obj);
    switch (GET_OBJ_VNUM(obj))
    {
    case ROL_LOOT_BLOCKER_VNUM:
      return rol_utility_loot_sweep(obj);
    case ROL_NECROMANCER_CHILD_VNUM:
      return rol_utility_necro_child(context->actor, obj);
    case ROL_RUBY_MONOCLE_VNUM:
      return rol_utility_ruby_monocle(obj);
    case ROL_BLACK_ORCHID_VNUM:
      return rol_utility_orchid_decay(obj);
    default:
      return FALSE;
    }
  }

  if (context->event != SPEC_EVENT_COMMAND || context->actor == NULL)
    return FALSE;

  ch = context->actor;
  if (rol_utility_acheron_portal_vnum(GET_OBJ_VNUM(obj)))
    return rol_utility_acheron_enter(context, ch, obj);
  if (GET_OBJ_VNUM(obj) == ROL_HYSSK_SKELETON_VNUM)
    return rol_utility_hyssk_skeleton(ch, obj, context->command, context->argument);
  if (GET_OBJ_VNUM(obj) == ROL_UM_TATTERED_CLOAK_VNUM)
    return rol_utility_tattered_cloak(context, ch, obj);
  if (GET_OBJ_VNUM(obj) == ROL_UM_QOGEK_STAFF_VNUM)
    return rol_utility_qogek_staff(context, ch, obj);
  if (profile != NULL)
    return rol_utility_called_effect(context, profile, ch, obj);
  switch (GET_OBJ_VNUM(obj))
  {
  case ROL_GOODBERRY_VNUM:
    return rol_utility_goodberry(ch, obj, context->command, context->argument);
  case ROL_LOOT_BLOCKER_VNUM:
    return rol_utility_loot_block(ch, obj, context->command, context->argument);
  case ROL_PLAGUE_RESERVOIR_VNUM:
    return rol_utility_plague_reservoir(ch, obj, context->command);
  case ROL_BLOODSTONE_CHILD_VNUM:
    return rol_utility_child_sacrifice(context, ch, obj);
  case ROL_MENDEN_FIGURINE_VNUM:
    return rol_utility_menden_figurine(context, ch, obj);
  case ROL_LATHANDER_DISC_VNUM:
    return rol_utility_lathander_disc(context, ch, obj);
  case ROL_LLYMS_ALTAR_VNUM:
    return rol_utility_llyms_altar(context, ch, obj);
  case ROL_SPIDERHAUNT_MAGGOTS_VNUM:
    return rol_utility_spiderhaunt_maggots(ch, obj, context->command, context->argument);
  case ROL_SPIDERHAUNT_CYRICS_ALTAR_VNUM:
    return rol_utility_spiderhaunt_altar(ch, obj, context->command);
  default:
    return FALSE;
  }
}
