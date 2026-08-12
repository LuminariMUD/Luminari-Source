/**
 * @file spec/spec_rol_totem.c
 * Converted Realms of Luminari shaman-totem behavior.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "comm.h"
#include "db.h"
#include "dgscript/dg_scripts.h"
#include "handler.h"
#include "interpreter.h"
#include "olc/oasis.h"
#include "spec_rol_totem.h"

#define ROL_TOTEM_MIN_CLERIC_LEVEL 21
#define ROL_TOTEM_WEEK_DAYS 7
#define ROL_TOTEM_WEEKLY_USES 3
#define ROL_TOTEM_END_VNUM (-1)

struct rol_totem_definition
{
  int target_vnum;
  int choice;
  bool evil;
  const char *death_message;
};

static const struct rol_totem_definition rol_totems[] = {
    {2000716, 1, false, "$n quickly fades away to the sound of a long mournful howl..."},
    {2000717, 2, false, "$n quickly fades away to the sound of a low growl..."},
    {2000718, 3, false, "$n quickly fades away to the sound of a long squeal..."},
    {2000719, 4, false, "$n quickly fades away to the sound of fading hoof beats..."},
    {2000720, 5, false, "$n quickly fades away to the sound of a high-pitched screech..."},
    {2000721, 6, false, "$n quickly fades away to the sound of a fading caw..."},
    {2000722, 7, false, "$n quickly fades away to the sound of a fading roar..."},
    {2000723, 8, false, "$n quickly fades away to the sound of a fading roar..."},
    {2000724, 9, false, "$n quickly fades away to the sound of fading hoof beats..."},
    {2000725, 10, false, "$n quickly fades away to the sound of a soft hiss..."},
    {2000732, 17, true, "$n quickly fades away to the sound of a long eerie howl..."},
    {2000733, 18, true, "$n quickly fades away to the sound of a loud squawk..."},
    {2000734, 19, true, "$n quickly fades away to the sound of a low gurgle..."},
    {2000735, 20, true, "$n quickly fades away to the sound of a soft hiss..."},
    {2000736, 21, true, "$n quickly fades away to the sound of sharp clacking..."},
    {2000737, 22, true, "$n quickly fades away to the sound of loud cackling..."},
    {2000738, 23, true, "$n quickly fades away to the sound of loud barking..."},
    {2000739, 24, true, "$n quickly fades away to the sound of a loud squish..."},
    {2000740, 25, true, "$n quickly fades away to the sound of a low hiss..."},
    {2000741, 26, true, "$n quickly fades away to the sound of high-pitched squeaking..."},
    {2000742, 27, true, "$n quickly fades away to the sound of a fading caw..."},
    {ROL_TOTEM_END_VNUM, 0, false, NULL},
};

static const struct rol_totem_definition *rol_totem_by_vnum(int target_vnum)
{
  const struct rol_totem_definition *totem;

  for (totem = rol_totems; totem->target_vnum != ROL_TOTEM_END_VNUM; totem++)
    if (totem->target_vnum == target_vnum)
      return totem;

  return NULL;
}

static const struct rol_totem_definition *rol_totem_by_choice(int choice)
{
  const struct rol_totem_definition *totem;

  for (totem = rol_totems; totem->target_vnum != ROL_TOTEM_END_VNUM; totem++)
    if (totem->choice == choice)
      return totem;

  return NULL;
}

int rol_shaman_totem_choice(int target_vnum)
{
  const struct rol_totem_definition *totem = rol_totem_by_vnum(target_vnum);

  return totem != NULL ? totem->choice : 0;
}

int rol_shaman_totem_vnum(int choice)
{
  const struct rol_totem_definition *totem = rol_totem_by_choice(choice);

  return totem != NULL ? totem->target_vnum : ROL_TOTEM_END_VNUM;
}

bool rol_shaman_totem_race_allowed(int target_vnum, int race)
{
  const struct rol_totem_definition *totem = rol_totem_by_vnum(target_vnum);

  return totem != NULL && totem->evil == rol_race_is_evil(race);
}

int rol_shaman_totem_success_chance(struct char_data *ch)
{
  int cleric_level;
  int chance;

  if (ch == NULL || ch->player_specials == NULL)
    return 0;

  cleric_level = CLASS_LEVEL(ch, CLASS_CLERIC);
  if (cleric_level < ROL_TOTEM_MIN_CLERIC_LEVEL)
    return 0;

  /* RoL's trainable shaman skill has no direct target equivalent.  Cleric
   * advancement and Wisdom retain an improving, fallible summon check. */
  chance = 50 + ((cleric_level - ROL_TOTEM_MIN_CLERIC_LEVEL) * 5) + (GET_WIS_BONUS(ch) * 2);
  return MIN(100, MAX(25, chance));
}

bool rol_shaman_totem_consume_weekly_use(struct char_data *ch, time_t now)
{
  int current_day;

  if (ch == NULL || now < 0)
    return false;

  current_day = (int)(now / SECS_PER_MUD_DAY);
  if (GET_ROL_TOTEM_WINDOW(ch) <= current_day)
  {
    GET_ROL_TOTEM_USES(ch) = 0;
    GET_ROL_TOTEM_WINDOW(ch) = current_day + ROL_TOTEM_WEEK_DAYS;
  }

  if (GET_ROL_TOTEM_USES(ch) >= ROL_TOTEM_WEEKLY_USES)
    return false;

  GET_ROL_TOTEM_USES(ch)++;
  return true;
}

const char *rol_totem_spirit_death_message(int target_vnum)
{
  const struct rol_totem_definition *totem = rol_totem_by_vnum(target_vnum);

  return totem != NULL ? totem->death_message : NULL;
}

static bool rol_totem_is_held(const struct obj_data *obj, const struct char_data *ch)
{
  if (obj == NULL || obj->worn_by != ch)
    return false;

  switch (obj->worn_on)
  {
  case WEAR_WIELD_1:
  case WEAR_HOLD_1:
  case WEAR_WIELD_OFFHAND:
  case WEAR_HOLD_2:
  case WEAR_WIELD_2H:
  case WEAR_HOLD_2H:
    return true;
  default:
    return false;
  }
}

static void rol_totem_summon(struct char_data *ch, const struct rol_totem_definition *totem)
{
  struct char_data *mob;
  int cleric_level;

  if ((mob = read_mobile(totem->target_vnum, VIRTUAL)) == NULL)
  {
    log("SYSERR: RoL shaman totem mobile %d is unavailable", totem->target_vnum);
    send_to_char(ch, "Your spirit cannot answer. Please tell a staff member.\r\n");
    return;
  }

  cleric_level = CLASS_LEVEL(ch, CLASS_CLERIC);
  GET_LEVEL(mob) = MIN(40, MAX(1, cleric_level - 10));
  SET_BIT_AR(MOB_FLAGS(mob), MOB_ROL_HAS_WA);
  SET_BIT_AR(MOB_FLAGS(mob), MOB_ROL_TOTEM_SPIRIT);
  autoroll_mob(mob, true, true);
  GET_REAL_MAX_HIT(mob) += MAX(1, GET_REAL_MAX_HIT(mob) / 4);
  GET_HIT(mob) = GET_REAL_MAX_HIT(mob);
  GET_EXP(mob) = 0;
  GET_GOLD(mob) = 0;
  SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
  {
    X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
    Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
  }

  char_to_room(mob, IN_ROOM(ch));
  load_mtrigger(mob);
  add_follower(mob, ch);
  send_to_char(ch, "You feel the presence of an otherworldly being enter the room.\r\n");
  act("$n coalesces before you.", TRUE, mob, NULL, NULL, TO_ROOM);
}

int rol_shaman_totem(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;
  const struct rol_totem_definition *totem;
  int chance;

  if (cmd == 0 && argument != NULL && !str_cmp(argument, "identify"))
  {
    send_to_char(ch, "Use while wielding or holding: bonds a Cleric to one spirit totem. At Cleric "
                     "21, it can attempt to summon that spirit up to three times per seven MUD "
                     "days.\r\n");
    return TRUE;
  }

  if (ch == NULL || obj == NULL || !CMD_IS("use") || !rol_totem_is_held(obj, ch))
    return FALSE;

  if ((totem = rol_totem_by_vnum(GET_OBJ_VNUM(obj))) == NULL)
    return FALSE;

  if (IS_NPC(ch) || CLASS_LEVEL(ch, CLASS_CLERIC) <= 0)
  {
    send_to_char(ch, "You lack the Cleric training needed to bond with this totem.\r\n");
    return TRUE;
  }

  if (!rol_shaman_totem_race_allowed(totem->target_vnum, GET_RACE(ch)))
  {
    send_to_char(ch,
                 "You feel hatred emanating from the totem; perhaps you should put it down.\r\n");
    return TRUE;
  }

  if (GET_OBJ_BOUND_ID(obj) != (int)NOBODY && GET_OBJ_BOUND_ID(obj) != GET_IDNUM(ch))
  {
    send_to_char(ch, "This totem belongs to someone else.\r\n");
    return TRUE;
  }

  if (GET_ROL_TOTEM_CHOICE(ch) == 0)
  {
    if (GET_OBJ_BOUND_ID(obj) != (int)NOBODY)
    {
      send_to_char(ch, "You have no spirit bond, but this totem is already bonded.\r\n");
      return TRUE;
    }
    GET_ROL_TOTEM_CHOICE(ch) = totem->choice;
    GET_OBJ_BOUND_ID(obj) = GET_IDNUM(ch);
    send_to_char(ch, "A strong feeling of acceptance and bonding overcomes you.\r\n"
                     "You may now summon your spirit companion.\r\n");
    return TRUE;
  }

  if (GET_ROL_TOTEM_CHOICE(ch) != totem->choice)
  {
    send_to_char(ch, "The strange totem does not respond to your call.\r\n");
    return TRUE;
  }
  if (GET_OBJ_BOUND_ID(obj) == (int)NOBODY)
  {
    send_to_char(ch, "This is not the totem that shares your bond.\r\n");
    return TRUE;
  }
  if (CLASS_LEVEL(ch, CLASS_CLERIC) < ROL_TOTEM_MIN_CLERIC_LEVEL)
  {
    send_to_char(ch, "You lack the power to make the totem respond to your will.\r\n");
    return TRUE;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "A mysterious force blocks your conjuring!\r\n");
    return TRUE;
  }
  if (!can_add_follower_by_flag(ch, MOB_ROL_TOTEM_SPIRIT))
  {
    send_to_char(ch, "You cannot control another spirit totem!\r\n");
    return TRUE;
  }

  send_to_char(ch, "You pray to your spirit totem for aid.\r\n");
  if (!rol_shaman_totem_consume_weekly_use(ch, time(NULL)))
  {
    send_to_char(ch, "You may only summon three totems every seven MUD days!\r\n");
    return TRUE;
  }

  chance = rol_shaman_totem_success_chance(ch);
  if (rand_number(1, 100) > chance)
  {
    send_to_char(ch, "Your spirit totem is unimpressed with your efforts.\r\n");
    return TRUE;
  }

  rol_totem_summon(ch, totem);
  return TRUE;
}
