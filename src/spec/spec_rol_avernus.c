/**
 * @file spec/spec_rol_avernus.c
 * Typed runtime adapters for converted Realms of Luminari Avernus procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "combat/fight.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "dgscript/dg_scripts.h"
#include "graph.h"
#include "handler.h"
#include "interpreter.h"
#include "magic/domains_schools.h"
#include "magic/spells.h"
#include "movement/movement.h"
#include "spec/spec_context.h"
#include "spec/spec_dispatch.h"
#include "spec/spec_rol_avernus.h"

#define ROL_AVERNUS_KRIIK_VNUM 2032624
#define ROL_AVERNUS_ROD_HOME_VNUM 2092338
#define ROL_AVERNUS_ROD_VNUM 2032631
#define ROL_AVERNUS_GREEN_ORB_VNUM 2032633
#define ROL_AVERNUS_CORNUGON_VNUM 2032661
#define ROL_AVERNUS_COIDON_VNUM 2032606
#define ROL_AVERNUS_PRISONER_REWARD_VNUM 2032649
#define ROL_AVERNUS_BEL_VNUM 2033014
#define ROL_AVERNUS_BEL_GUARD_ONE_VNUM 2033019
#define ROL_AVERNUS_BEL_GUARD_TWO_VNUM 2033020
#define ROL_AVERNUS_DAGGER_MOB_VNUM 2033027
#define ROL_AVERNUS_DAGGER_HELPER_WEAPON_VNUM 2033033
#define ROL_AVERNUS_GARDEN_FIRST_VNUM 2032672
#define ROL_AVERNUS_GARDEN_LAST_VNUM 2032687
#define ROL_AVERNUS_ESCAPE_POOL_FIRST_VNUM 2032613
#define ROL_AVERNUS_ESCAPE_POOL_LAST_VNUM 2032616

#define ROL_AVERNUS_PRISON_FIRST_DOOR (1U << 0)
#define ROL_AVERNUS_PRISON_SECOND_DOOR (1U << 1)
#define ROL_AVERNUS_PRISON_REVERSE (1U << 2)

enum rol_avernus_mobile_effect
{
  ROL_AVERNUS_MAN = 0,
  ROL_AVERNUS_RING_PATROL,
  ROL_AVERNUS_RING_PATROL_REVERSE,
  ROL_AVERNUS_REHIDE,
  ROL_AVERNUS_PRISONER_RETURN,
  ROL_AVERNUS_PRISON_PATROL,
  ROL_AVERNUS_ERINYES_ILLUSION,
  ROL_AVERNUS_DEVA_ECHOES,
  ROL_AVERNUS_CITADEL_PATROL,
  ROL_AVERNUS_BEL,
  ROL_AVERNUS_CITADEL_PATROL_REVERSE,
  ROL_AVERNUS_BLACK_ALTAR,
  ROL_AVERNUS_DANCING_DAGGER
};

struct rol_avernus_mobile_profile
{
  int mobile_vnum;
  enum rol_avernus_mobile_effect effect;
  bool commands;
  bool activity;
  bool death;
};

enum rol_avernus_object_effect
{
  ROL_AVERNUS_OBJECT_ROD = 0,
  ROL_AVERNUS_OBJECT_BEL_SWORD,
  ROL_AVERNUS_OBJECT_DANCING_DAGGER
};

struct rol_avernus_object_profile
{
  int object_vnum;
  enum rol_avernus_object_effect effect;
  bool commands;
  bool pulse;
  bool weapon_hit;
};

static const struct rol_avernus_mobile_profile rol_avernus_mobile_profiles[] = {
    {2032623, ROL_AVERNUS_MAN, false, false, true},
    {2032641, ROL_AVERNUS_RING_PATROL, false, true, false},
    {2032643, ROL_AVERNUS_RING_PATROL_REVERSE, false, true, false},
    {2032654, ROL_AVERNUS_REHIDE, false, true, false},
    {2032659, ROL_AVERNUS_REHIDE, false, true, false},
    {2032660, ROL_AVERNUS_PRISONER_RETURN, false, true, false},
    {2033000, ROL_AVERNUS_PRISON_PATROL, false, true, false},
    {2033003, ROL_AVERNUS_ERINYES_ILLUSION, false, true, true},
    {2033005, ROL_AVERNUS_DEVA_ECHOES, false, true, false},
    {2033008, ROL_AVERNUS_CITADEL_PATROL, false, true, false},
    {2033014, ROL_AVERNUS_BEL, true, true, true},
    {2033020, ROL_AVERNUS_REHIDE, false, true, false},
    {2033021, ROL_AVERNUS_CITADEL_PATROL_REVERSE, false, true, false},
    {2033026, ROL_AVERNUS_BLACK_ALTAR, false, true, true},
    {2033027, ROL_AVERNUS_DANCING_DAGGER, true, true, true},
};

static const struct rol_avernus_object_profile rol_avernus_object_profiles[] = {
    {2032631, ROL_AVERNUS_OBJECT_ROD, true, true, false},
    {2033011, ROL_AVERNUS_OBJECT_BEL_SWORD, false, true, true},
    {2033021, ROL_AVERNUS_OBJECT_DANCING_DAGGER, true, true, true},
    {2033025, ROL_AVERNUS_OBJECT_DANCING_DAGGER, true, true, true},
};

static room_rnum rol_avernus_illusion_room = NOWHERE;
static char *rol_avernus_illusion_description = NULL;

static const struct rol_avernus_mobile_profile *rol_avernus_mobile_profile_for(int mobile_vnum)
{
  size_t index;

  for (index = 0;
       index < sizeof(rol_avernus_mobile_profiles) / sizeof(rol_avernus_mobile_profiles[0]);
       index++)
    if (rol_avernus_mobile_profiles[index].mobile_vnum == mobile_vnum)
      return &rol_avernus_mobile_profiles[index];
  return NULL;
}

static const struct rol_avernus_object_profile *rol_avernus_object_profile_for(int object_vnum)
{
  size_t index;

  for (index = 0;
       index < sizeof(rol_avernus_object_profiles) / sizeof(rol_avernus_object_profiles[0]);
       index++)
    if (rol_avernus_object_profiles[index].object_vnum == object_vnum)
      return &rol_avernus_object_profiles[index];
  return NULL;
}

size_t rol_avernus_mobile_profile_count(void)
{
  return sizeof(rol_avernus_mobile_profiles) / sizeof(rol_avernus_mobile_profiles[0]);
}

bool rol_avernus_mobile_profile(int mobile_vnum, bool *commands, bool *activity, bool *death)
{
  const struct rol_avernus_mobile_profile *profile = rol_avernus_mobile_profile_for(mobile_vnum);

  if (profile == NULL)
    return false;
  if (commands != NULL)
    *commands = profile->commands;
  if (activity != NULL)
    *activity = profile->activity;
  if (death != NULL)
    *death = profile->death;
  return true;
}

size_t rol_avernus_object_profile_count(void)
{
  return sizeof(rol_avernus_object_profiles) / sizeof(rol_avernus_object_profiles[0]);
}

bool rol_avernus_object_profile(int object_vnum, bool *commands, bool *pulse, bool *weapon_hit)
{
  const struct rol_avernus_object_profile *profile = rol_avernus_object_profile_for(object_vnum);

  if (profile == NULL)
    return false;
  if (commands != NULL)
    *commands = profile->commands;
  if (pulse != NULL)
    *pulse = profile->pulse;
  if (weapon_hit != NULL)
    *weapon_hit = profile->weapon_hit;
  return true;
}

bool rol_avernus_garden_room_vnum(int room_vnum)
{
  return room_vnum >= ROL_AVERNUS_GARDEN_FIRST_VNUM && room_vnum <= ROL_AVERNUS_GARDEN_LAST_VNUM;
}

static int rol_avernus_ring_direction(int room_vnum, bool reverse)
{
  int source_vnum = room_vnum - 2000000;

  if (!reverse)
  {
    switch (source_vnum)
    {
    case 32908:
    case 32909:
    case 32910:
    case 32911:
    case 32912:
    case 32913:
    case 32915:
    case 32948:
    case 32950:
    case 32951:
    case 32952:
    case 32953:
    case 32954:
    case 32955:
    case 32958:
    case 32959:
    case 32960:
    case 32962:
    case 32963:
    case 32984:
    case 32985:
    case 32987:
    case 32988:
    case 32989:
      return EAST;
    case 32914:
    case 32916:
    case 32917:
    case 32918:
    case 32919:
    case 32920:
    case 32921:
    case 32922:
    case 32924:
    case 32927:
    case 32979:
    case 32980:
    case 32981:
    case 32982:
    case 32983:
    case 32986:
      return NORTH;
    case 32923:
    case 32925:
    case 32926:
    case 32928:
    case 32929:
    case 32930:
    case 32931:
    case 32932:
    case 32933:
    case 32934:
    case 32935:
    case 32937:
    case 32938:
    case 32940:
    case 32969:
    case 32970:
    case 32971:
    case 32972:
    case 32973:
    case 32974:
    case 32975:
    case 32976:
    case 32977:
    case 32978:
      return WEST;
    case 32936:
    case 32939:
    case 32941:
    case 32942:
    case 32943:
    case 32944:
    case 32945:
    case 32946:
    case 32947:
    case 32949:
    case 32961:
    case 32964:
    case 32965:
    case 32966:
    case 32967:
    case 32968:
      return SOUTH;
    default:
      return -1;
    }
  }

  switch (source_vnum)
  {
  case 32908:
  case 32909:
  case 32910:
  case 32911:
  case 32912:
  case 32913:
  case 32914:
  case 32916:
  case 32949:
  case 32951:
  case 32952:
  case 32953:
  case 32954:
  case 32955:
  case 32958:
  case 32959:
  case 32960:
  case 32961:
  case 32963:
  case 32964:
  case 32985:
  case 32986:
  case 32988:
  case 32989:
    return WEST;
  case 32915:
  case 32917:
  case 32918:
  case 32919:
  case 32920:
  case 32921:
  case 32922:
  case 32923:
  case 32925:
  case 32928:
  case 32980:
  case 32981:
  case 32982:
  case 32983:
  case 32984:
  case 32987:
    return SOUTH;
  case 32924:
  case 32926:
  case 32927:
  case 32929:
  case 32930:
  case 32931:
  case 32932:
  case 32933:
  case 32934:
  case 32935:
  case 32936:
  case 32938:
  case 32939:
  case 32941:
  case 32970:
  case 32971:
  case 32972:
  case 32973:
  case 32974:
  case 32975:
  case 32976:
  case 32977:
  case 32978:
  case 32979:
    return EAST;
  case 32937:
  case 32940:
  case 32942:
  case 32943:
  case 32944:
  case 32945:
  case 32946:
  case 32947:
  case 32948:
  case 32950:
  case 32962:
  case 32965:
  case 32966:
  case 32967:
  case 32968:
  case 32969:
    return NORTH;
  default:
    return -1;
  }
}

static int rol_avernus_citadel_direction(int room_vnum, bool reverse)
{
  int source_vnum = room_vnum - 2000000;

  if (!reverse)
  {
    switch (source_vnum)
    {
    case 33028:
    case 33029:
    case 33030:
    case 33035:
    case 33037:
    case 33061:
    case 33068:
    case 33069:
      return NORTH;
    case 33022:
    case 33023:
    case 33024:
    case 33025:
    case 33027:
    case 33059:
    case 33060:
    case 33062:
    case 33063:
    case 33066:
    case 33067:
      return EAST;
    case 33054:
    case 33055:
    case 33056:
    case 33058:
    case 33064:
    case 33065:
    case 33070:
    case 33072:
      return SOUTH;
    case 33038:
    case 33039:
    case 33040:
    case 33042:
    case 33043:
    case 33044:
    case 33045:
    case 33047:
    case 33048:
    case 33049:
    case 33050:
    case 33053:
      return WEST;
    default:
      return -1;
    }
  }

  switch (source_vnum)
  {
  case 33022:
  case 33055:
  case 33056:
  case 33058:
  case 33059:
  case 33065:
  case 33066:
  case 33070:
    return NORTH;
  case 33039:
  case 33040:
  case 33042:
  case 33043:
  case 33044:
  case 33045:
  case 33047:
  case 33048:
  case 33049:
  case 33050:
  case 33053:
  case 33054:
    return EAST;
  case 33029:
  case 33030:
  case 33035:
  case 33037:
  case 33038:
  case 33062:
  case 33069:
  case 33071:
    return SOUTH;
  case 33023:
  case 33024:
  case 33025:
  case 33027:
  case 33028:
  case 33060:
  case 33061:
  case 33063:
  case 33064:
  case 33067:
  case 33068:
  case 33072:
    return WEST;
  default:
    return -1;
  }
}

int rol_avernus_patrol_direction(int mobile_vnum, int room_vnum)
{
  switch (mobile_vnum)
  {
  case 2032641:
    return rol_avernus_ring_direction(room_vnum, false);
  case 2032643:
    return rol_avernus_ring_direction(room_vnum, true);
  case 2033008:
    return rol_avernus_citadel_direction(room_vnum, false);
  case 2033021:
    return rol_avernus_citadel_direction(room_vnum, true);
  default:
    return -1;
  }
}

static bool rol_avernus_command_is(int cmd, const char *name)
{
  return cmd > 0 && name != NULL && complete_cmd_info != NULL &&
         complete_cmd_info[cmd].command != NULL && !strcmp(complete_cmd_info[cmd].command, name);
}

static void rol_avernus_transfer_inventory(struct char_data *from, struct char_data *to)
{
  struct obj_data *obj;
  int wear;

  if (from == NULL || to == NULL)
    return;
  while ((obj = from->carrying) != NULL)
  {
    obj_from_char(obj);
    obj_to_char(obj, to);
  }
  for (wear = 0; wear < NUM_WEARS; wear++)
  {
    if (GET_EQ(from, wear) == NULL)
      continue;
    obj = unequip_char(from, wear);
    if (obj != NULL && GET_EQ(to, wear) == NULL)
      equip_char(to, obj, wear);
    else if (obj != NULL)
      obj_to_char(obj, to);
  }
}

static void rol_avernus_stop_combat(struct char_data *victim)
{
  struct char_data *fighter;
  struct char_data *next;

  if (victim == NULL || !VALID_ROOM_RNUM(IN_ROOM(victim)))
    return;
  if (FIGHTING(victim) != NULL)
    stop_fighting(victim);
  for (fighter = world[IN_ROOM(victim)].people; fighter != NULL; fighter = next)
  {
    next = fighter->next_in_room;
    if (FIGHTING(fighter) == victim)
      stop_fighting(fighter);
  }
}

static bool rol_avernus_open_and_move(struct char_data *ch, int direction)
{
  struct room_direction_data *exit;
  struct room_direction_data *reverse_exit;
  room_rnum destination;

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)) || direction < 0 || direction >= NUM_OF_DIRS ||
      (exit = EXIT(ch, direction)) == NULL || !VALID_ROOM_RNUM(exit->to_room))
    return false;
  destination = exit->to_room;
  REMOVE_BIT(exit->exit_info, EX_LOCKED);
  REMOVE_BIT(exit->exit_info, EX_LOCKED_EASY);
  REMOVE_BIT(exit->exit_info, EX_LOCKED_MEDIUM);
  REMOVE_BIT(exit->exit_info, EX_LOCKED_HARD);
  REMOVE_BIT(exit->exit_info, EX_CLOSED);
  reverse_exit = world[destination].dir_option[rev_dir[direction]];
  if (reverse_exit != NULL && reverse_exit->to_room == IN_ROOM(ch))
  {
    REMOVE_BIT(reverse_exit->exit_info, EX_LOCKED);
    REMOVE_BIT(reverse_exit->exit_info, EX_LOCKED_EASY);
    REMOVE_BIT(reverse_exit->exit_info, EX_LOCKED_MEDIUM);
    REMOVE_BIT(reverse_exit->exit_info, EX_LOCKED_HARD);
    REMOVE_BIT(reverse_exit->exit_info, EX_CLOSED);
  }
  return perform_move(ch, direction, 1) != 0;
}

static void rol_avernus_close_and_lock(struct char_data *ch, int direction)
{
  struct room_direction_data *exit;
  struct room_direction_data *reverse_exit;
  room_rnum destination;

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)) || direction < 0 || direction >= NUM_OF_DIRS ||
      (exit = EXIT(ch, direction)) == NULL || !VALID_ROOM_RNUM(exit->to_room))
    return;
  destination = exit->to_room;
  SET_BIT(exit->exit_info, EX_ISDOOR | EX_CLOSED | EX_LOCKED);
  reverse_exit = world[destination].dir_option[rev_dir[direction]];
  if (reverse_exit != NULL && reverse_exit->to_room == IN_ROOM(ch))
    SET_BIT(reverse_exit->exit_info, EX_ISDOOR | EX_CLOSED | EX_LOCKED);
}

static bool rol_avernus_prison_patrol(struct char_data *ch)
{
  unsigned int state;
  int current_vnum;

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return false;
  state = (unsigned int)ch->mob_specials.proc_fired;
  current_vnum = GET_ROOM_VNUM(IN_ROOM(ch));

  if ((state & ROL_AVERNUS_PRISON_REVERSE) == 0)
  {
    switch (current_vnum)
    {
    case 2033001:
    case 2033004:
    case 2033007:
      if ((state & (ROL_AVERNUS_PRISON_FIRST_DOOR | ROL_AVERNUS_PRISON_SECOND_DOOR)) == 0)
        (void)rol_avernus_open_and_move(ch, EAST);
      else if ((state & ROL_AVERNUS_PRISON_FIRST_DOOR) != 0 &&
               (state & ROL_AVERNUS_PRISON_SECOND_DOOR) == 0)
      {
        rol_avernus_close_and_lock(ch, EAST);
        (void)rol_avernus_open_and_move(ch, WEST);
      }
      else
      {
        rol_avernus_close_and_lock(ch, WEST);
        (void)rol_avernus_open_and_move(ch, SOUTH);
        state &= ~(ROL_AVERNUS_PRISON_FIRST_DOOR | ROL_AVERNUS_PRISON_SECOND_DOOR);
        if (current_vnum == 2033001)
          state |= ROL_AVERNUS_PRISON_REVERSE;
      }
      ch->mob_specials.proc_fired = (int)state;
      return true;
    case 2033002:
    case 2033005:
    case 2033008:
      (void)rol_avernus_open_and_move(ch, EAST);
      ch->mob_specials.proc_fired = (int)(state | ROL_AVERNUS_PRISON_SECOND_DOOR);
      return true;
    case 2033003:
    case 2033006:
    case 2033009:
      (void)rol_avernus_open_and_move(ch, WEST);
      ch->mob_specials.proc_fired = (int)(state | ROL_AVERNUS_PRISON_FIRST_DOOR);
      return true;
    case 2033012:
    case 2033013:
    case 2033016:
    case 2033017:
      (void)rol_avernus_open_and_move(ch, EAST);
      return true;
    case 2033010:
    case 2033011:
    case 2033014:
    case 2033015:
      (void)rol_avernus_open_and_move(ch, SOUTH);
      return true;
    default:
      return false;
    }
  }

  switch (current_vnum)
  {
  case 2033000:
    (void)rol_avernus_open_and_move(ch, NORTH);
    rol_avernus_close_and_lock(ch, SOUTH);
    return true;
  case 2033001:
  case 2033004:
  case 2033007:
  case 2033010:
  case 2033013:
  case 2033014:
    (void)rol_avernus_open_and_move(ch, NORTH);
    return true;
  case 2033011:
  case 2033012:
  case 2033015:
    (void)rol_avernus_open_and_move(ch, WEST);
    return true;
  case 2033016:
    (void)rol_avernus_open_and_move(ch, WEST);
    ch->mob_specials.proc_fired = (int)(state & ~(unsigned int)ROL_AVERNUS_PRISON_REVERSE);
    return true;
  default:
    return false;
  }
}

static bool rol_avernus_patrol_observes_intruder(struct char_data *ch)
{
  struct char_data *target;

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return false;
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = target->next_in_room)
  {
    if (target == ch || (IS_NPC(target) && !IS_PET(target)) || GET_LEVEL(target) >= LVL_IMMORT)
      continue;
    if (AFF_FLAGGED(target, AFF_HIDE))
      do_search(ch, "", 0, 0);
    return true;
  }
  return false;
}

static void rol_avernus_patrol_recover(struct char_data *ch)
{
  room_rnum home;
  int direction;

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      !VALID_ROOM_RNUM((home = GET_MOB_LOADROOM(ch))) || home == IN_ROOM(ch))
    return;
  direction = find_first_step(IN_ROOM(ch), home);
  if (direction >= 0 && direction < NUM_OF_DIRS)
    (void)rol_avernus_open_and_move(ch, direction);
}

static int rol_avernus_patrol_activity(struct char_data *ch, enum rol_avernus_mobile_effect effect)
{
  bool moved = false;
  int direction;

  if (ch == NULL || FIGHTING(ch) != NULL || IS_PET(ch))
    return FALSE;
  if ((effect == ROL_AVERNUS_CITADEL_PATROL || effect == ROL_AVERNUS_CITADEL_PATROL_REVERSE) &&
      rol_avernus_patrol_observes_intruder(ch))
    return FALSE;

  if (effect == ROL_AVERNUS_PRISON_PATROL)
    moved = rol_avernus_prison_patrol(ch);
  else
  {
    direction = rol_avernus_patrol_direction(GET_MOB_VNUM(ch), GET_ROOM_VNUM(IN_ROOM(ch)));
    if (direction >= 0)
      moved = rol_avernus_open_and_move(ch, direction);
  }
  if (!moved)
    rol_avernus_patrol_recover(ch);
  return FALSE;
}

static int rol_avernus_man_death(struct char_data *ch)
{
  struct char_data *replacement;

  replacement = read_mobile(ROL_AVERNUS_KRIIK_VNUM, VIRTUAL);
  if (replacement == NULL)
  {
    log("SYSERR: RoL Avernus man cannot load replacement mobile %d", ROL_AVERNUS_KRIIK_VNUM);
    return FALSE;
  }
  char_to_room(replacement, IN_ROOM(ch));
  GET_MOB_LOADROOM(replacement) = IN_ROOM(ch);
  rol_avernus_transfer_inventory(ch, replacement);
  send_to_room(IN_ROOM(ch),
               "The man's body shudders and tears apart as black wings and long claws burst "
               "forth. Kri'ik stands revealed in a blaze of infernal fire.\r\n");
  return TRUE;
}

static void rol_avernus_erinyes_restore(struct char_data *ch)
{
  if (ch == NULL || rol_avernus_illusion_description == NULL ||
      rol_avernus_illusion_room != IN_ROOM(ch) || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return;
  free(world[IN_ROOM(ch)].description);
  world[IN_ROOM(ch)].description = rol_avernus_illusion_description;
  rol_avernus_illusion_description = NULL;
  rol_avernus_illusion_room = NOWHERE;
}

static int rol_avernus_erinyes_death(struct char_data *ch)
{
  static const char ruined_description[] =
      "   What was once a large and decorated chamber is now a dilapidated office. Fallen "
      "deeply into disrepair, it appears to have been abandoned for centuries. The stone "
      "walls crumble around weapon and claw scars, while tattered, burned tapestries barely "
      "cling to them. The smell of blood and decay makes every breath foul.\r\n";

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;
  free(rol_avernus_illusion_description);
  rol_avernus_illusion_description =
      strdup(world[IN_ROOM(ch)].description != NULL ? world[IN_ROOM(ch)].description : "");
  if (rol_avernus_illusion_description == NULL)
    return FALSE;
  rol_avernus_illusion_room = IN_ROOM(ch);
  send_to_room(IN_ROOM(ch), "The very room itself seems to quiver and melt.\r\n");
  free(world[IN_ROOM(ch)].description);
  world[IN_ROOM(ch)].description = strdup(ruined_description);
  if (world[IN_ROOM(ch)].description == NULL)
  {
    world[IN_ROOM(ch)].description = rol_avernus_illusion_description;
    rol_avernus_illusion_description = NULL;
    rol_avernus_illusion_room = NOWHERE;
  }
  return FALSE;
}

static int rol_avernus_deva_echoes(struct char_data *ch)
{
  struct char_data *target;
  int echo;

  if (rand_number(0, 2) == 0)
    return FALSE;
  echo = rand_number(0, 3);
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = target->next_in_room)
  {
    if (IS_NPC(target))
      continue;
    if (IS_GOOD(target))
    {
      switch (echo)
      {
      case 0:
        act("$n looks at you longingly. A soft voice in your head pleads, 'Help me.'", FALSE, ch,
            NULL, target, TO_VICT);
        break;
      case 1:
        act("$n looks up at you with a pleading gaze.", FALSE, ch, NULL, target, TO_VICT);
        break;
      case 2:
        act("$n looks at you and says, 'Free me.'", FALSE, ch, NULL, target, TO_VICT);
        break;
      default:
        act("$n says, 'Help me, please.'", FALSE, ch, NULL, target, TO_VICT);
        break;
      }
    }
    else if (IS_EVIL(target))
    {
      switch (echo)
      {
      case 0:
        act("$n spits in your face and tells you to send Bel in himself.", FALSE, ch, NULL, target,
            TO_VICT);
        break;
      case 1:
        act("$n spits blood at your feet and snarls that you will never break him.", FALSE, ch,
            NULL, target, TO_VICT);
        break;
      case 2:
        act("$n says, 'Back already? You will pay if I ever get free, evil scum.'", FALSE, ch, NULL,
            target, TO_VICT);
        break;
      default:
        act("$n lunges at you, but the chains stop him an inch away.", FALSE, ch, NULL, target,
            TO_VICT);
        break;
      }
    }
  }
  return FALSE;
}

static int rol_avernus_prisoner_return(struct char_data *ch)
{
  struct char_data *coidon;
  struct char_data *leader;
  struct obj_data *reward;

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      (VALID_ROOM_RNUM(GET_MOB_LOADROOM(ch)) && GET_MOB_LOADROOM(ch) == IN_ROOM(ch)) ||
      (leader = ch->master) == NULL)
    return FALSE;
  for (coidon = world[IN_ROOM(ch)].people; coidon != NULL; coidon = coidon->next_in_room)
    if (IS_NPC(coidon) && GET_MOB_VNUM(coidon) == ROL_AVERNUS_COIDON_VNUM)
      break;
  if (coidon == NULL)
    return FALSE;

  act("$n says, 'I appreciate all your help. Give this to Coidon with the assignment.'", FALSE, ch,
      NULL, leader, TO_VICT);
  reward = read_object(ROL_AVERNUS_PRISONER_REWARD_VNUM, VIRTUAL);
  if (reward != NULL)
  {
    obj_to_char(reward, leader);
    act("You receive $p from $n.", FALSE, ch, reward, leader, TO_VICT);
    act("$n gives $p to $N.", FALSE, ch, reward, leader, TO_NOTVICT);
    load_otrigger(reward);
  }
  else
    log("SYSERR: RoL Avernus prisoner cannot load reward object %d",
        ROL_AVERNUS_PRISONER_REWARD_VNUM);
  extract_char(ch);
  return TRUE;
}

static int rol_avernus_black_altar_activity(struct char_data *ch)
{
  struct char_data *target;

  if (rand_number(0, 2) != 0)
    return FALSE;
  send_to_room(IN_ROOM(ch), "The black altar pulses.\r\n");
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = target->next_in_room)
  {
    if (!IS_NPC(target) || IS_PET(target) || GET_HIT(target) >= GET_MAX_HIT(target))
      continue;
    if (AFF_FLAGGED(target, AFF_BLIND))
    {
      affect_from_char(target, SPELL_BLINDNESS);
      REMOVE_BIT_AR(AFF_FLAGS(target), AFF_BLIND);
      send_to_char(target, "Your vision returns!\r\n");
    }
    GET_HIT(target) = MIN(GET_MAX_HIT(target), GET_HIT(target) + 1000);
  }
  return FALSE;
}

static struct char_data *rol_avernus_dagger_owner(const struct char_data *dagger)
{
  struct char_data *owner;
  long owner_id;

  if (dagger == NULL || (owner_id = dagger->mob_specials.rol_dancing_dagger_owner_id) <= 0)
    return NULL;
  for (owner = character_list; owner != NULL; owner = owner->next)
    if (!IS_NPC(owner) && GET_IDNUM(owner) == owner_id && VALID_ROOM_RNUM(IN_ROOM(owner)))
      return owner;
  return NULL;
}

static int rol_avernus_dagger_return(struct char_data *dagger)
{
  struct char_data *owner;
  struct obj_data *obj;
  obj_vnum object_vnum;

  if (dagger == NULL || (owner = rol_avernus_dagger_owner(dagger)) == NULL ||
      dagger->mob_specials.rol_dancing_dagger_object_vnum <= 0)
    return FALSE;
  object_vnum = (obj_vnum)dagger->mob_specials.rol_dancing_dagger_object_vnum;
  for (obj = owner->carrying; obj != NULL; obj = obj->next_content)
  {
    if (GET_OBJ_VNUM(obj) != object_vnum)
      continue;
    if (OBJ_FLAGGED(obj, ITEM_HIDDEN))
    {
      REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_HIDDEN);
      act("$p flies back to you.", FALSE, owner, obj, NULL, TO_CHAR);
      act("$p flies back to $n.", FALSE, owner, obj, NULL, TO_ROOM);
    }
    return TRUE;
  }
  return FALSE;
}

static int rol_avernus_dagger_activity(struct char_data *dagger)
{
  struct char_data *master;

  if (dagger == NULL || MOB_FLAGGED(dagger, MOB_NOTDEADYET))
    return TRUE;
  master = dagger->master;
  if (master == NULL || !VALID_ROOM_RNUM(IN_ROOM(master)) || IN_ROOM(master) != IN_ROOM(dagger))
  {
    die(dagger, dagger);
    return TRUE;
  }
  if (FIGHTING(dagger) == NULL)
  {
    if (FIGHTING(master) != NULL && IN_ROOM(FIGHTING(master)) == IN_ROOM(dagger))
      set_fighting(dagger, FIGHTING(master));
    else
    {
      die(dagger, dagger);
      return TRUE;
    }
  }
  if (rand_number(0, 4) == 0)
  {
    die(dagger, dagger);
    return TRUE;
  }
  return FALSE;
}

static int rol_avernus_dagger_command(struct spec_event_context *context, struct char_data *dagger)
{
  if (context == NULL || dagger == NULL || context->actor == NULL || dagger->master == NULL ||
      context->actor != dagger->master || GET_LEVEL(dagger->master) >= LVL_IMMORT ||
      !rol_avernus_command_is(context->command, "order"))
    return FALSE;
  act("$n ignores your orders.", FALSE, dagger, NULL, dagger->master, TO_VICT);
  return TRUE;
}

static int rol_avernus_bel_command(struct spec_event_context *context, struct char_data *bel)
{
  struct char_data *actor;

  if (context == NULL || bel == NULL || (actor = context->actor) == NULL ||
      !rol_avernus_command_is(context->command, "shieldpunch"))
    return FALSE;
  act("As you align your shield, $n slams a wing into it and sends you reeling!", FALSE, bel, NULL,
      actor, TO_VICT);
  act("As $N begins to shieldpunch, $n slams a wing into the shield and sends $M reeling!", FALSE,
      bel, NULL, actor, TO_NOTVICT);
  act("You slam your wing into $N's shield and send $M reeling.", FALSE, bel, NULL, actor, TO_CHAR);
  WAIT_STATE(actor, PULSE_VIOLENCE);
  return TRUE;
}

static void rol_avernus_bel_ensure_affects(struct char_data *bel)
{
  struct affected_type affect;

  if (!affected_by_spell(bel, SPELL_REGENERATION))
  {
    new_affect(&affect);
    affect.spell = SPELL_REGENERATION;
    affect.duration = 99;
    SET_BIT_AR(affect.bitvector, AFF_REGEN);
    affect_to_char(bel, &affect);
  }
  if (!affected_by_spell(bel, SPELL_BLESS))
  {
    new_affect(&affect);
    affect.spell = SPELL_BLESS;
    affect.duration = 99;
    affect.location = APPLY_HITROLL;
    affect.modifier = (GET_HITROLL(bel) * 3) / 2;
    affect_to_char(bel, &affect);
  }
}

static int rol_avernus_bel_activity(struct char_data *bel)
{
  struct char_data *target;
  struct char_data *next;

  rol_avernus_bel_ensure_affects(bel);
  for (target = world[IN_ROOM(bel)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (!IS_PET(target))
      continue;
    if (target->master != NULL && IN_ROOM(target->master) == IN_ROOM(bel))
      act("$n glares at $N and demands to know how $E dares bring a summoned being here.", FALSE,
          bel, NULL, target->master, TO_ROOM);
    else
      act("$n glares at $N and demands to know who dares bring a summoned being here.", FALSE, bel,
          NULL, target, TO_ROOM);
    die(target, bel);
  }

  if (FIGHTING(bel) != NULL && GET_ROOM_VNUM(IN_ROOM(bel)) == 2033073 && EXIT(bel, SOUTH) != NULL &&
      (!EXIT_FLAGGED(EXIT(bel, SOUTH), EX_CLOSED) || !EXIT_FLAGGED(EXIT(bel, SOUTH), EX_LOCKED)))
  {
    act("$n utters an arcane phrase. With a wave of $s hand, the door slams shut and locks with "
        "a heavy thud.",
        FALSE, bel, NULL, NULL, TO_ROOM);
    rol_avernus_close_and_lock(bel, SOUTH);
    if (real_room(2033072) != NOWHERE)
      send_to_room(real_room(2033072), "The door slams shut and locks with a heavy thud.\r\n");
  }
  return FALSE;
}

static bool rol_avernus_bel_guard_vnum(int mobile_vnum)
{
  return mobile_vnum == ROL_AVERNUS_BEL_GUARD_ONE_VNUM ||
         mobile_vnum == ROL_AVERNUS_BEL_GUARD_TWO_VNUM;
}

static int rol_avernus_bel_death(struct char_data *bel)
{
  struct char_data *guard = NULL;
  struct char_data *new_bel;
  struct char_data *target;

  for (target = world[IN_ROOM(bel)].people; target != NULL; target = target->next_in_room)
    if (target != bel && IS_NPC(target) && rol_avernus_bel_guard_vnum(GET_MOB_VNUM(target)) &&
        !MOB_FLAGGED(target, MOB_NOTDEADYET))
      guard = target;
  if (guard == NULL)
    return FALSE;

  act("$n condemns $N for failing him. A green sphere strips the flesh from $S bones and "
      "returns its stolen strength to $n.",
      FALSE, bel, NULL, guard, TO_ROOM);
  die(guard, bel);
  new_bel = read_mobile(ROL_AVERNUS_BEL_VNUM, VIRTUAL);
  if (new_bel == NULL)
  {
    log("SYSERR: RoL Avernus Bel cannot load replacement mobile %d", ROL_AVERNUS_BEL_VNUM);
    return FALSE;
  }
  char_to_room(new_bel, IN_ROOM(bel));
  GET_MOB_LOADROOM(new_bel) = GET_MOB_LOADROOM(bel);
  rol_avernus_transfer_inventory(bel, new_bel);
  GET_HIT(new_bel) = GET_MAX_HIT(new_bel);
  for (target = world[IN_ROOM(new_bel)].people; target != NULL; target = target->next_in_room)
  {
    if (target == new_bel || !IS_NPC(target) || !rol_avernus_bel_guard_vnum(GET_MOB_VNUM(target)) ||
        MOB_FLAGGED(target, MOB_NOTDEADYET))
      continue;
    if (target->master != NULL)
      stop_follower(target);
    add_follower(target, new_bel);
  }
  return TRUE;
}

static int rol_avernus_mobile_activity(const struct rol_avernus_mobile_profile *profile,
                                       struct char_data *mobile)
{
  switch (profile->effect)
  {
  case ROL_AVERNUS_RING_PATROL:
  case ROL_AVERNUS_RING_PATROL_REVERSE:
  case ROL_AVERNUS_PRISON_PATROL:
  case ROL_AVERNUS_CITADEL_PATROL:
  case ROL_AVERNUS_CITADEL_PATROL_REVERSE:
    return rol_avernus_patrol_activity(mobile, profile->effect);
  case ROL_AVERNUS_REHIDE:
    if (FIGHTING(mobile) == NULL && !AFF_FLAGGED(mobile, AFF_HIDE) && IS_ROGUE(mobile))
      do_hide(mobile, "", 0, 0);
    return FALSE;
  case ROL_AVERNUS_PRISONER_RETURN:
    return rol_avernus_prisoner_return(mobile);
  case ROL_AVERNUS_ERINYES_ILLUSION:
    rol_avernus_erinyes_restore(mobile);
    return FALSE;
  case ROL_AVERNUS_DEVA_ECHOES:
    return rol_avernus_deva_echoes(mobile);
  case ROL_AVERNUS_BEL:
    return rol_avernus_bel_activity(mobile);
  case ROL_AVERNUS_BLACK_ALTAR:
    return rol_avernus_black_altar_activity(mobile);
  case ROL_AVERNUS_DANCING_DAGGER:
    return rol_avernus_dagger_activity(mobile);
  default:
    return FALSE;
  }
}

static int rol_avernus_mobile_death(const struct rol_avernus_mobile_profile *profile,
                                    struct char_data *mobile)
{
  switch (profile->effect)
  {
  case ROL_AVERNUS_MAN:
    return rol_avernus_man_death(mobile);
  case ROL_AVERNUS_ERINYES_ILLUSION:
    return rol_avernus_erinyes_death(mobile);
  case ROL_AVERNUS_BEL:
    return rol_avernus_bel_death(mobile);
  case ROL_AVERNUS_BLACK_ALTAR:
    act("$n shatters into several large chunks of stone.", TRUE, mobile, NULL, NULL, TO_ROOM);
    return TRUE;
  case ROL_AVERNUS_DANCING_DAGGER:
    act("Overwhelmed, $n returns to its rightful owner.", FALSE, mobile, NULL, NULL, TO_ROOM);
    return rol_avernus_dagger_return(mobile);
  default:
    return FALSE;
  }
}

int rol_avernus_mobile_event(struct spec_event_context *context, struct char_data *mobile)
{
  const struct rol_avernus_mobile_profile *profile;

  if (context == NULL || mobile == NULL || !IS_NPC(mobile) ||
      (profile = rol_avernus_mobile_profile_for(GET_MOB_VNUM(mobile))) == NULL)
    return FALSE;
  switch (context->event)
  {
  case SPEC_EVENT_COMMAND:
    if (profile->effect == ROL_AVERNUS_BEL)
      return rol_avernus_bel_command(context, mobile);
    if (profile->effect == ROL_AVERNUS_DANCING_DAGGER)
      return rol_avernus_dagger_command(context, mobile);
    return FALSE;
  case SPEC_EVENT_MOBILE_ACTIVITY:
    return rol_avernus_mobile_activity(profile, mobile);
  case SPEC_EVENT_MOBILE_DEATH:
    return rol_avernus_mobile_death(profile, mobile);
  default:
    return FALSE;
  }
}

static struct char_data *rol_avernus_object_owner(struct obj_data *obj)
{
  if (obj == NULL)
    return NULL;
  while (obj->in_obj != NULL)
    obj = obj->in_obj;
  if (obj->worn_by != NULL)
    return obj->worn_by;
  return obj->carried_by;
}

static void rol_avernus_detach_object(struct obj_data *obj)
{
  if (obj == NULL)
    return;
  if (obj->worn_by != NULL)
    (void)unequip_char(obj->worn_by, obj->worn_on);
  else if (obj->carried_by != NULL)
    obj_from_char(obj);
  else if (obj->in_obj != NULL)
    obj_from_obj(obj);
  else if (VALID_ROOM_RNUM(IN_ROOM(obj)))
    obj_from_room(obj);
}

static bool rol_avernus_rod_is_held(const struct char_data *ch, const struct obj_data *obj)
{
  return ch != NULL && obj != NULL && obj->worn_by == ch &&
         (obj->worn_on == WEAR_HOLD_1 || obj->worn_on == WEAR_HOLD_2 ||
          obj->worn_on == WEAR_HOLD_2H);
}

static int rol_avernus_rod_command(struct spec_event_context *context, struct char_data *ch,
                                   struct obj_data *obj)
{
  struct char_data *kriik;
  struct char_data *cornugon;
  struct obj_data *orb;
  room_rnum home;

  if (context == NULL || ch == NULL || obj == NULL || IS_NPC(ch) ||
      !rol_avernus_rod_is_held(ch, obj) || !rol_avernus_command_is(context->command, "shout"))
    return FALSE;
  home = real_room(ROL_AVERNUS_ROD_HOME_VNUM);
  if (GET_LEVEL(ch) < LVL_IMMORT && IN_ROOM(ch) != home)
  {
    send_to_char(ch, "The rod wrenches itself from your grasp and flies away!\r\n");
    act("$p glows, tears itself free of $n's grasp, and flies away!", FALSE, ch, obj, NULL,
        TO_ROOM);
    if (home != NOWHERE)
    {
      rol_avernus_detach_object(obj);
      obj_to_room(obj, home);
    }
    return FALSE;
  }
  if (context->argument == NULL || str_cmp(context->argument, "Kri'ik"))
    return FALSE;

  kriik = read_mobile(ROL_AVERNUS_KRIIK_VNUM, VIRTUAL);
  if (kriik == NULL)
  {
    log("SYSERR: RoL Avernus rod cannot load Kri'ik (%d) or Cornugon (%d)", ROL_AVERNUS_KRIIK_VNUM,
        ROL_AVERNUS_CORNUGON_VNUM);
    return TRUE;
  }
  cornugon = read_mobile(ROL_AVERNUS_CORNUGON_VNUM, VIRTUAL);
  if (cornugon == NULL)
  {
    log("SYSERR: RoL Avernus rod cannot load Kri'ik (%d) or Cornugon (%d)", ROL_AVERNUS_KRIIK_VNUM,
        ROL_AVERNUS_CORNUGON_VNUM);
    char_to_room(kriik, IN_ROOM(ch));
    extract_char(kriik);
    return TRUE;
  }
  act("As you shout 'Kri'ik', your consciousness tears free and races across the Astral Plane "
      "to Avernus. It snaps back as an infernal rift opens before you.",
      FALSE, ch, obj, NULL, TO_CHAR);
  act("As $n shouts 'Kri'ik', the sky blackens and an infernal rift opens nearby.", FALSE, ch, obj,
      NULL, TO_ROOM);
  char_to_room(kriik, IN_ROOM(ch));
  GET_MOB_LOADROOM(kriik) = IN_ROOM(ch);
  orb = read_object(ROL_AVERNUS_GREEN_ORB_VNUM, VIRTUAL);
  if (orb != NULL)
  {
    obj_to_char(orb, kriik);
    load_otrigger(orb);
  }
  char_to_room(cornugon, IN_ROOM(ch));
  GET_MOB_LOADROOM(cornugon) = IN_ROOM(ch);
  add_follower(cornugon, kriik);
  SET_BIT_AR(AFF_FLAGS(cornugon), AFF_FLYING);
  send_to_room(IN_ROOM(ch),
               "Kri'ik reveals his bargain's cruel deception, breaks the rod, and summons an "
               "infernal ally.\r\n");
  rol_avernus_detach_object(obj);
  extract_obj(obj);
  context->invalidation |= SPEC_INVALIDATE_OWNER;
  set_fighting(kriik, ch);
  load_mtrigger(kriik);
  load_mtrigger(cornugon);
  return TRUE;
}

static bool rol_avernus_has_dagger_follower(const struct char_data *owner,
                                            const struct obj_data *obj)
{
  struct follow_type *follower;
  int object_vnum;

  if (owner == NULL || obj == NULL)
    return false;
  object_vnum = GET_OBJ_VNUM(obj);
  for (follower = owner->followers; follower != NULL; follower = follower->next)
    if (follower->follower != NULL && IS_NPC(follower->follower) &&
        GET_MOB_VNUM(follower->follower) == ROL_AVERNUS_DAGGER_MOB_VNUM &&
        follower->follower->mob_specials.rol_dancing_dagger_object_vnum == object_vnum)
      return true;
  return false;
}

static int rol_avernus_restore_dancing_dagger(struct char_data *owner, struct obj_data *obj)
{
  if (!OBJ_FLAGGED(obj, ITEM_HIDDEN))
    return FALSE;
  if (owner == NULL)
  {
    if (obj->in_obj != NULL)
      REMOVE_OBJ_FLAG(obj, ITEM_HIDDEN);
    return FALSE;
  }
  if (!rol_avernus_has_dagger_follower(owner, obj))
  {
    REMOVE_OBJ_FLAG(obj, ITEM_HIDDEN);
    act("$p appears in your inventory.", FALSE, owner, obj, NULL, TO_CHAR);
  }
  return FALSE;
}

static int rol_avernus_dagger_object_command(struct spec_event_context *context,
                                             struct char_data *ch, struct obj_data *obj)
{
  if (context == NULL || ch == NULL || obj == NULL || GET_LEVEL(ch) < LVL_IMMORT ||
      context->argument == NULL ||
      (!rol_avernus_command_is(context->command, "say") &&
       !rol_avernus_command_is(context->command, "'")) ||
      str_cmp(context->argument, "darkness to light"))
    return FALSE;
  act("Light spills from $p and envelops the room.", FALSE, ch, obj, NULL, TO_ROOM);
  act("Light spills from $p and bathes the room.", FALSE, ch, obj, NULL, TO_CHAR);
  (void)call_magic(ch, ch, NULL, SPELL_CONTINUAL_LIGHT, 0, GET_LEVEL(ch), CAST_INNATE);
  return TRUE;
}

static int rol_avernus_dagger_object_identify(struct char_data *ch, struct obj_data *obj)
{
  int index;

  if (ch == NULL || obj == NULL)
    return FALSE;
  for (index = 0; index < 2; index++)
    send_to_char(ch, "Affects: %s by %+d\r\n", apply_types[obj->affected[index].location],
                 obj->affected[index].modifier);
  send_to_char(ch, "Special Effects: Dancing object; high-level wielders can release it to fight "
                   "independently.\r\n");
  return TRUE;
}

static int rol_avernus_dagger_object_hit(struct spec_event_context *context, struct char_data *ch,
                                         struct obj_data *obj, struct char_data *victim)
{
  struct char_data *dagger;
  struct obj_data *helper_weapon;
  int slot;

  if (context == NULL || ch == NULL || obj == NULL || victim == NULL ||
      spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID ||
      spec_context_validate_combat_target(ch, victim, false) != SPEC_CONTEXT_VALID ||
      GET_LEVEL(ch) < 46 || rand_number(0, 9) != 0)
    return FALSE;
  slot = obj->worn_on;
  if (slot != WEAR_WIELD_1 && slot != WEAR_WIELD_OFFHAND)
    return FALSE;
  dagger = read_mobile(ROL_AVERNUS_DAGGER_MOB_VNUM, VIRTUAL);
  if (dagger == NULL)
  {
    log("SYSERR: RoL dancing dagger cannot load helper mobile %d", ROL_AVERNUS_DAGGER_MOB_VNUM);
    return FALSE;
  }

  (void)unequip_char(ch, slot);
  obj_to_char(obj, ch);
  char_to_room(dagger, IN_ROOM(ch));
  GET_MOB_LOADROOM(dagger) = IN_ROOM(ch);
  GET_GOLD(dagger) = 0;
  GET_EXP(dagger) = 0;
  GET_LEVEL(dagger) = GET_LEVEL(ch);
  GET_DAMROLL(dagger) = GET_DAMROLL(ch);
  dagger->mob_specials.rol_dancing_dagger_owner_id = IS_NPC(ch) ? 0 : GET_IDNUM(ch);
  dagger->mob_specials.rol_dancing_dagger_object_vnum = GET_OBJ_VNUM(obj);
  SET_BIT_AR(AFF_FLAGS(dagger), AFF_CHARM);
  dagger->char_specials.is_charmie = true;
  SET_OBJ_FLAG(obj, ITEM_HIDDEN);

  helper_weapon = read_object(ROL_AVERNUS_DAGGER_HELPER_WEAPON_VNUM, VIRTUAL);
  if (helper_weapon != NULL)
  {
    equip_char(dagger, helper_weapon, WEAR_WIELD_1);
    load_otrigger(helper_weapon);
  }
  add_follower(dagger, ch);
  act("$p floats from your hands and begins attacking $N on its own.", FALSE, ch, obj, victim,
      TO_CHAR);
  act("$p floats from $n's hands and begins attacking $N on its own.", FALSE, ch, obj, victim,
      TO_NOTVICT);
  act("$p floats from $n's hands and begins attacking you on its own!", FALSE, ch, obj, victim,
      TO_VICT);
  set_fighting(dagger, victim);
  load_mtrigger(dagger);
  return FALSE;
}

static struct char_data *rol_avernus_find_bel(void)
{
  struct char_data *candidate;

  for (candidate = character_list; candidate != NULL; candidate = candidate->next)
    if (IS_NPC(candidate) && GET_MOB_VNUM(candidate) == ROL_AVERNUS_BEL_VNUM &&
        VALID_ROOM_RNUM(IN_ROOM(candidate)) && !MOB_FLAGGED(candidate, MOB_NOTDEADYET))
      return candidate;
  return NULL;
}

static int rol_avernus_bel_sword_restrict(struct spec_event_context *context,
                                          struct char_data *owner, struct obj_data *obj)
{
  if (owner == NULL || obj == NULL || GET_LEVEL(owner) >= LVL_IMMORT ||
      (IS_NPC(owner) && !IS_PET(owner) && MOB_FLAGGED(owner, MOB_ROL_DEVIL)))
    return FALSE;
  act("$p flashes with infernal heat and burns you severely.", FALSE, owner, obj, NULL, TO_CHAR);
  act("$n winces as infernal heat surrounds $m.", FALSE, owner, obj, NULL, TO_ROOM);
  GET_OBJ_VAL(obj, 1) = 1;
  GET_OBJ_VAL(obj, 2) = 1;
  if (damage(owner, owner, rand_number(5, 50), -1, DAM_FIRE, FALSE) < 0 && context != NULL)
    context->invalidation |= SPEC_INVALIDATE_ACTOR;
  return TRUE;
}

static int rol_avernus_enforce_bel_sword_owner(struct spec_event_context *context,
                                               struct char_data *owner, struct obj_data *obj)
{
  struct char_data *bel;
  bool punish;

  if (owner != NULL && IS_NPC(owner) && !IS_PET(owner) &&
      GET_MOB_VNUM(owner) == ROL_AVERNUS_BEL_VNUM)
    return FALSE;
  if (owner != NULL && GET_LEVEL(owner) >= LVL_IMMORT)
    return FALSE;
  bel = rol_avernus_find_bel();
  if (bel == NULL)
    return rol_avernus_bel_sword_restrict(context, owner, obj);

  punish = owner != NULL && (!IS_NPC(owner) || IS_PET(owner));
  rol_avernus_detach_object(obj);
  if (GET_EQ(bel, WEAR_WIELD_1) == NULL)
    equip_char(bel, obj, WEAR_WIELD_1);
  else
    obj_to_char(obj, bel);
  act("$p flies back into $n's hands.", FALSE, bel, obj, NULL, TO_ROOM);
  if (punish)
  {
    act("Infernal fire consumes you as $p tears itself from your grasp.", FALSE, owner, obj, NULL,
        TO_CHAR);
    act("$n is consumed by infernal fire as $p flies away.", FALSE, owner, obj, NULL, TO_ROOM);
    die(owner, bel);
    if (context != NULL)
      context->invalidation |= SPEC_INVALIDATE_ACTOR;
  }
  return TRUE;
}

static bool rol_avernus_bel_sword_target(struct char_data *ch, struct char_data *target)
{
  if (ch == NULL || target == NULL || target == ch || GET_LEVEL(target) >= LVL_IMMORT ||
      (IS_NPC(target) && !IS_PET(target)))
    return false;
  return aoeOK(ch, target, -1);
}

static int rol_avernus_bel_sword_hit(struct spec_event_context *context, struct char_data *ch,
                                     struct obj_data *obj)
{
  struct char_data *target;
  struct char_data *next;
  struct affected_type affect;
  int amount;

  if (ch == NULL || obj == NULL || obj->worn_by != ch || FIGHTING(ch) == NULL ||
      rand_number(0, 5) != 0)
    return FALSE;
  act("Your $p ignites and sends out a corona of fire!", FALSE, ch, obj, NULL, TO_CHAR);
  act("$n's $p glows brightly and sends out a corona of fire!", FALSE, ch, obj, NULL, TO_ROOM);
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (!rol_avernus_bel_sword_target(ch, target))
      continue;
    amount = dice(50, 8) + dice(1, 50);
    if (savingthrow(ch, target, SAVING_REFL, 0, CAST_WEAPON_SPELL, GET_LEVEL(ch), EVOCATION))
      amount /= 2;
    if (damage(ch, target, amount, SPELL_FIREBALL, DAM_FIRE, FALSE) < 0)
    {
      if (context != NULL && target == context->target)
        context->invalidation |= SPEC_INVALIDATE_TARGET;
      continue;
    }
    if (!AFF_FLAGGED(target, AFF_ON_FIRE))
    {
      new_affect(&affect);
      affect.spell = SPELL_FIREBALL;
      affect.duration = 3;
      SET_BIT_AR(affect.bitvector, AFF_ON_FIRE);
      affect_to_char(target, &affect);
    }
  }
  return FALSE;
}

int rol_avernus_object(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return FALSE;
}

int rol_avernus_object_typed(struct spec_event_context *context)
{
  const struct rol_avernus_object_profile *profile;
  struct char_data *ch;
  struct obj_data *obj;

  if (context == NULL || context->owner_type != SPEC_OWNER_OBJECT || context->owner == NULL)
    return FALSE;
  obj = context->owner;
  profile = rol_avernus_object_profile_for(GET_OBJ_VNUM(obj));
  if (profile == NULL)
    return FALSE;
  ch = context->actor;

  switch (context->event)
  {
  case SPEC_EVENT_COMMAND:
    if (profile->effect == ROL_AVERNUS_OBJECT_ROD)
      return rol_avernus_rod_command(context, ch, obj);
    if (profile->effect == ROL_AVERNUS_OBJECT_DANCING_DAGGER)
      return rol_avernus_dagger_object_command(context, ch, obj);
    return FALSE;
  case SPEC_EVENT_OBJECT_AUTOMATIC:
    ch = rol_avernus_object_owner(obj);
    if (profile->effect == ROL_AVERNUS_OBJECT_DANCING_DAGGER)
      return rol_avernus_restore_dancing_dagger(ch, obj);
    if (profile->effect == ROL_AVERNUS_OBJECT_BEL_SWORD)
      return rol_avernus_enforce_bel_sword_owner(context, ch, obj);
    return FALSE;
  case SPEC_EVENT_ITEM_IDENTIFY:
    if (profile->effect == ROL_AVERNUS_OBJECT_DANCING_DAGGER)
      return rol_avernus_dagger_object_identify(ch, obj);
    return FALSE;
  case SPEC_EVENT_WEAPON_HIT:
    if (ch == NULL || context->target == NULL)
      return FALSE;
    if (profile->effect == ROL_AVERNUS_OBJECT_DANCING_DAGGER)
      return rol_avernus_dagger_object_hit(context, ch, obj, context->target);
    if (profile->effect == ROL_AVERNUS_OBJECT_BEL_SWORD)
      return rol_avernus_bel_sword_hit(context, ch, obj);
    return FALSE;
  default:
    return FALSE;
  }
}

static bool rol_avernus_garden_has_escape_pool(room_rnum room)
{
  struct obj_data *obj;
  int object_vnum;

  for (obj = world[room].contents; obj != NULL; obj = obj->next_content)
  {
    object_vnum = GET_OBJ_VNUM(obj);
    if (object_vnum >= ROL_AVERNUS_ESCAPE_POOL_FIRST_VNUM &&
        object_vnum <= ROL_AVERNUS_ESCAPE_POOL_LAST_VNUM)
      return true;
  }
  return false;
}

static bool rol_avernus_garden_save(struct char_data *target)
{
  return savingthrow(NULL, target, SAVING_WILL, -3, CAST_INNATE, 30, ENCHANTMENT);
}

static void rol_avernus_garden_sleep(struct char_data *target)
{
  struct affected_type affect;

  if (target == NULL || AFF_FLAGGED(target, AFF_SLEEP) || MOB_FLAGGED(target, MOB_NOSLEEP) ||
      char_has_object_flag(target, ITEM_ROL_NO_SLEEP))
    return;
  new_affect(&affect);
  affect.spell = SPELL_SLEEP;
  affect.duration = 5;
  SET_BIT_AR(affect.bitvector, AFF_SLEEP);
  affect_join(target, &affect, FALSE, FALSE, FALSE, FALSE);
  if (GET_POS(target) > POS_SLEEPING)
    GET_POS(target) = POS_SLEEPING;
  rol_avernus_stop_combat(target);
  send_to_char(target, "The sense of peace and tranquility puts you to sleep.\r\n");
  act("$n's eyes close as $e falls into a deep sleep.", FALSE, target, NULL, NULL, TO_ROOM);
}

static void rol_avernus_garden_room_activity(room_rnum room)
{
  struct char_data *target;
  struct char_data *next;
  bool escape_pool;

  if (!VALID_ROOM_RNUM(room))
    return;
  escape_pool = rol_avernus_garden_has_escape_pool(room);
  for (target = world[room].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (GET_LEVEL(target) >= LVL_IMMORT)
      continue;
    if (FIGHTING(target) != NULL && !rol_avernus_garden_save(target))
    {
      if (affected_by_spell(target, SKILL_RAGE))
        continue;
      rol_avernus_stop_combat(target);
      send_to_char(target, "The surroundings bring a sense of calm upon you.\r\n");
    }
    else if (!escape_pool && (!IS_NPC(target) || IS_PET(target)) &&
             !rol_avernus_garden_save(target))
      rol_avernus_garden_sleep(target);
  }
}

int rol_avernus_garden(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return FALSE;
}

int rol_avernus_garden_typed(struct spec_event_context *context)
{
  struct room_data *garden;
  room_rnum room;
  int room_vnum;

  if (context == NULL || context->owner_type != SPEC_OWNER_ROOM ||
      context->event != SPEC_EVENT_ROOM_ACTIVITY || context->owner == NULL)
    return FALSE;
  garden = context->owner;
  if (garden->number != ROL_AVERNUS_GARDEN_FIRST_VNUM)
    return FALSE;
  for (room_vnum = ROL_AVERNUS_GARDEN_FIRST_VNUM; room_vnum <= ROL_AVERNUS_GARDEN_LAST_VNUM;
       room_vnum++)
  {
    room = real_room(room_vnum);
    if (room != NOWHERE)
      rol_avernus_garden_room_activity(room);
  }
  return FALSE;
}

void rol_avernus_process_garden_activity(void)
{
  room_rnum garden = real_room(ROL_AVERNUS_GARDEN_FIRST_VNUM);

  if (garden != NOWHERE && world[garden].func != NULL)
    spec_gateway_room_activity(&world[garden]);
}
