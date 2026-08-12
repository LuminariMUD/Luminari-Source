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
#include "spec_context.h"
#include "spec_cooldown.h"
#include "spec_dispatch.h"
#include "spec_rol_utility_objects.h"

#define ROL_GOODBERRY_VNUM 2000876
#define ROL_BLOODSTONE_CHILD_VNUM 2007151
#define ROL_NECROMANCER_CHILD_VNUM 2046991
#define ROL_MENDEN_FIGURINE_VNUM 2088825
#define ROL_RUBY_MONOCLE_VNUM 2090004
#define ROL_RUBY_MONOCLE_ROOM_MIN 2090124
#define ROL_RUBY_MONOCLE_ROOM_MAX 2090142
#define ROL_MAGIUS_STAFF_VNUM 2000047
#define ROL_BASILISK_SNAKE_VNUM 2000412
#define ROL_DRAGONCULT_ROBES_VNUM 2010672
#define ROL_EARTHMOTHER_STAFF_VNUM 2026260
#define ROL_BASILISK_LEGGINGS_VNUM 2043723
#define ROL_BASILISK_SNAKES_VNUM 2044019
#define ROL_BLUEPLUME_VNUM 2051110
#define ROL_WRITHING_ASH_VNUM 2051207
#define ROL_HASTE_SLEEVES_VNUM 2057236

enum rol_utility_called_effect
{
  ROL_UTILITY_MAGIUS_STAFF = 0,
  ROL_UTILITY_DRAGONCULT_ROBES,
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

/* Keep this table sorted by converted object VNUM for binary lookup. */
static const struct rol_utility_called_profile rol_utility_called_profiles[] = {
    {ROL_MAGIUS_STAFF_VNUM, ROL_UTILITY_MAGIUS_STAFF, "shirak", 0,
     "Say 'shirak' to light the staff or 'dulak' to darken it."},
    {ROL_DRAGONCULT_ROBES_VNUM, ROL_UTILITY_DRAGONCULT_ROBES, "draconian protection", 72,
     "Say 'draconian protection' while worn for elemental resistance every three MUD days."},
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
    GET_HIT(ch) -= damage_amount;
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
    if (profile == NULL || context->actor == NULL)
      return FALSE;
    send_to_char(context->actor, "Special Effects: %s\r\n", profile->description);
    return TRUE;
  }

  if (context->event == SPEC_EVENT_OBJECT_AUTO_PULSE)
  {
    switch (GET_OBJ_VNUM(obj))
    {
    case ROL_NECROMANCER_CHILD_VNUM:
      return rol_utility_necro_child(context->actor, obj);
    case ROL_RUBY_MONOCLE_VNUM:
      return rol_utility_ruby_monocle(obj);
    default:
      return FALSE;
    }
  }

  if (context->event != SPEC_EVENT_COMMAND || context->actor == NULL)
    return FALSE;

  ch = context->actor;
  if (profile != NULL)
    return rol_utility_called_effect(context, profile, ch, obj);
  switch (GET_OBJ_VNUM(obj))
  {
  case ROL_GOODBERRY_VNUM:
    return rol_utility_goodberry(ch, obj, context->command, context->argument);
  case ROL_BLOODSTONE_CHILD_VNUM:
    return rol_utility_child_sacrifice(context, ch, obj);
  case ROL_MENDEN_FIGURINE_VNUM:
    return rol_utility_menden_figurine(context, ch, obj);
  default:
    return FALSE;
  }
}
