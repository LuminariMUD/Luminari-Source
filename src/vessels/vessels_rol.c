/**************************************************************************
 *  File: vessels/vessels_rol.c                       Part of LuminariMUD *
 *  Usage: Converted Realms of Luminari fixed-interior ship procedures.  *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "event_runtime.h"
#include "handler.h"
#include "interpreter.h"
#include "vessel_periodic.h"
#include "vessels_rol.h"

#define ROL_SHIP_NONE (-1)
#define ROL_SHIP_MAX_SPEED 30
#define ROL_SHIP_DIRECTIONS 6

struct rol_ship_definition
{
  obj_vnum hull_vnum;
  obj_vnum panel_vnum;
  room_vnum entrance_vnum;
  room_vnum first_interior_vnum;
  room_vnum last_interior_vnum;
  mob_vnum navigator_vnum;
  room_vnum control_vnum;
  room_vnum route_start_vnum;
  room_vnum route_destination_vnum;
  const char *outbound_path;
  const char *return_path;
  int sail_hour;
  int frequency;
};

struct rol_ship_state
{
  struct obj_data *hull;
  int max_hull;
  int max_speed;
  int damage;
  int capacity;
  int size;
  int velocity;
  int action_timer;
  int move_timer;
  int last_direction;
  int repeat;
  int docked_ship;
  bool route_sailing;
  const char *route_path;
  room_vnum route_destination;
  struct char_data *last_help_victim;
  struct event_runtime_handle periodic_event_handle;
  uint64_t periodic_generation;
};

static const struct rol_ship_definition rol_ship_definitions[] = {
    {2005731, 2005732, 2005999, 2005998, 2005999, 2005739, 2005998, 2005926, 2005934, "22222222.",
     "00000000.", 0, 4},
    {2011100, 2011104, 2011100, 2011100, 2011136, 2011101, 2011101, 2026200, 2005313,
     "000000300000011111111011011101101110110111011011101101110110111011011101100111011011"
     "1011011101101110110111011000001010000111111112.",
     "033333333222232322222332333233233323323332332333233233323323332332333233233323323332"
     "332333233233323323332332333333332222221222222.",
     6, 12},
    {2011300, 2011304, 2011300, 2011300, 2011336, 2011301, 2011301, 2005399, 2026200,
     "232323333333233233323323332332333233233323323332332333233233323323332332333233233323"
     "32333233233323323333333322222212221222.",
     "030003000000001111111111101101110110111011011101101110110111011011101101110110111011"
     "01110110111011011101101110110111101010.",
     6, 12},
    {2034249, 2034250, 2034774, 2034774, 2034775, 2034233, 2034775, 2034240, 2034256,
     "322112222221122.", "003300000033001.", 0, 4},
    {2090391, 2090390, 2090395, 2090395, 2090396, 2090390, 2090396, 2090306, 2090308, "00000000.",
     "22222222.", 0, 4},
    {2046610, 2046611, 2046611, 2046610, 2046612, 2046610, 2046612, 2020101, 2021496,
     "3333330003333303333333333.", "1111111111211111222111111.", 0, 4},
    {2098451, 2098450, 2098453, 2098450, 2098477, 2098452, 2098452, 2098425, 2014312,
     "3300000110033000000000000000000000000000000000000001.",
     "3222222222222222222222222222222222222221122332222211.", 0, 4},
};

static struct rol_ship_state
    rol_ship_states[sizeof(rol_ship_definitions) / sizeof(rol_ship_definitions[0])];
static bool rol_periodic_initialized;
static game_event_type_id_t rol_ship_event_type;
static uint64_t rol_next_generation = 1U;
static uint64_t rol_periodic_callback_count;

static struct game_event_result rol_ship_owner_event(const struct game_event_context *context);

static int rol_ship_count(void)
{
  return (int)(sizeof(rol_ship_definitions) / sizeof(rol_ship_definitions[0]));
}

static long rol_ship_boundary_delay(void)
{
  unsigned long cadence = (unsigned long)(PASSES_PER_SEC * 5 / 2);
  unsigned long remainder = pulse % cadence;

  return remainder == 0U ? (long)cadence : (long)(cadence - remainder);
}

static void borrowed_owner_cleanup(void *payload)
{
  (void)payload;
}

bool rol_ship_periodic_register_event_type(void)
{
  struct game_event_type_config config;
  const char *registered_name;
  enum game_scheduler_status status;

  if (!event_runtime_is_initialized())
    return false;
  registered_name = event_runtime_type_name(rol_ship_event_type);
  if (registered_name != NULL && !strcmp(registered_name, "vessel.rol.agenda"))
    return true;
  rol_ship_event_type = 0U;
  memset(&config, 0, sizeof(config));
  config.name = "vessel.rol.agenda";
  config.handler = rol_ship_owner_event;
  config.cleanup = borrowed_owner_cleanup;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = (size_t)rol_ship_count();
  config.max_events_per_owner = 1U;
  config.requires_owner = true;
  status = event_runtime_register_type(&config, &rol_ship_event_type);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: unable to register native event type 'vessel.rol.agenda' (status %d).", status);
    return false;
  }
  return true;
}

static struct game_event_result rol_ship_owner_event(const struct game_event_context *context)
{
  struct rol_ship_state *state = context != NULL ? context->payload : NULL;
  int ship_index;

  if (state == NULL || state < rol_ship_states || state >= rol_ship_states + rol_ship_count())
    return game_event_result_complete();
  ship_index = (int)(state - rol_ship_states);
  rol_periodic_callback_count++;
  if (!rol_periodic_initialized || !CONFIG_VESSEL_SYSTEM || !vessel_periodic_events_enabled() ||
      state->hull == NULL || IN_ROOM(state->hull) == NOWHERE || IN_ROOM(state->hull) > top_of_world)
  {
    if (context != NULL && state->periodic_event_handle.id == context->event_id)
      state->periodic_event_handle = EVENT_RUNTIME_HANDLE_NONE;
    return game_event_result_complete();
  }
  rol_ship_activity_one(ship_index);
  return game_event_result_reschedule_after(PASSES_PER_SEC * 5 / 2);
}

static void rol_ship_schedule(int ship_index)
{
  struct game_event_owner owner = game_event_owner_none();
  struct rol_ship_state *state;

  if (!rol_periodic_initialized || !CONFIG_VESSEL_SYSTEM || !vessel_periodic_events_enabled() ||
      ship_index < 0 || ship_index >= rol_ship_count())
    return;
  state = &rol_ship_states[ship_index];
  if (state->hull == NULL || !event_runtime_handle_is_none(state->periodic_event_handle) ||
      IN_ROOM(state->hull) == NOWHERE || IN_ROOM(state->hull) > top_of_world)
    return;
  if (state->periodic_generation == 0U)
  {
    if (rol_next_generation == 0U)
      return;
    state->periodic_generation = rol_next_generation;
    if (rol_next_generation == UINT64_MAX)
      rol_next_generation = 0U;
    else
      rol_next_generation++;
  }
  owner.kind = GAME_EVENT_OWNER_VESSEL;
  owner.runtime_id = 0x524f4c00U + (uint64_t)ship_index + 1U;
  owner.generation = state->periodic_generation;
  (void)event_runtime_schedule_owned_after(rol_ship_event_type, owner,
                                           (game_tick_t)rol_ship_boundary_delay(), state,
                                           &state->periodic_event_handle);
}

static void rol_ship_cancel(int ship_index)
{
  struct rol_ship_state *state;
  struct event_runtime_handle handle;

  if (ship_index < 0 || ship_index >= rol_ship_count())
    return;
  state = &rol_ship_states[ship_index];
  if (!event_runtime_handle_is_none(state->periodic_event_handle))
  {
    handle = state->periodic_event_handle;
    state->periodic_event_handle = EVENT_RUNTIME_HANDLE_NONE;
    (void)event_runtime_cancel(handle);
  }
  state->periodic_generation = 0U;
}

int rol_ship_definition_count(void)
{
  return rol_ship_count();
}

bool rol_ship_interior_contains(int ship_index, room_vnum room)
{
  const struct rol_ship_definition *definition;

  if (ship_index < 0 || ship_index >= rol_ship_count())
    return false;

  definition = &rol_ship_definitions[ship_index];
  return room >= definition->first_interior_vnum && room <= definition->last_interior_vnum;
}

int rol_ship_move_delay_for_speed(int speed)
{
  return MAX(0, ROL_SHIP_MAX_SPEED - MAX(0, MIN(ROL_SHIP_MAX_SPEED, speed)));
}

bool rol_ship_can_enter_sector(int sector, bool dockable)
{
  if (dockable)
    return true;

  return sector == SECT_OCEAN || sector == SECT_WATER_SWIM || sector == SECT_WATER_NOSWIM ||
         sector == SECT_UD_WATER || sector == SECT_UD_NOSWIM || sector == SECT_RIVER;
}

static void rol_ship_reset_state(int ship_index, struct obj_data *hull)
{
  struct rol_ship_state *state;

  state = &rol_ship_states[ship_index];
  memset(state, 0, sizeof(*state));
  state->hull = hull;
  state->docked_ship = ROL_SHIP_NONE;

  if (hull == NULL)
    return;

  state->max_hull = MAX(0, GET_OBJ_VAL(hull, 0));
  state->max_speed = MAX(0, MIN(ROL_SHIP_MAX_SPEED, GET_OBJ_VAL(hull, 1)));
  state->damage = MAX(0, GET_OBJ_VAL(hull, 2));
  state->capacity = MAX(0, GET_OBJ_VAL(hull, 3));
  state->size = state->max_hull >> 3;
}

static struct obj_data *rol_ship_find_hull(int ship_index)
{
  struct rol_ship_state *state;

  state = &rol_ship_states[ship_index];
  if (state->hull != NULL && IN_ROOM(state->hull) != NOWHERE &&
      IN_ROOM(state->hull) <= top_of_world)
    return state->hull;
  return NULL;
}

static int rol_ship_index_for_hull(struct obj_data *hull)
{
  struct obj_data *existing;
  int ship_index;

  if (hull == NULL)
    return ROL_SHIP_NONE;

  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
  {
    if (GET_OBJ_VNUM(hull) != rol_ship_definitions[ship_index].hull_vnum)
      continue;

    existing = rol_ship_find_hull(ship_index);
    if (existing != NULL && existing != hull)
      return ROL_SHIP_NONE;
    if (existing == NULL)
      rol_ship_reset_state(ship_index, hull);
    return ship_index;
  }

  return ROL_SHIP_NONE;
}

static int rol_ship_index_for_room(room_rnum room)
{
  room_vnum vnum;
  int ship_index;

  if (room == NOWHERE || room > top_of_world)
    return ROL_SHIP_NONE;

  vnum = GET_ROOM_VNUM(room);
  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
  {
    if (rol_ship_interior_contains(ship_index, vnum))
      return ship_index;
  }

  return ROL_SHIP_NONE;
}

static int rol_ship_index_for_navigator(struct char_data *mob)
{
  int ship_index;

  if (mob == NULL || !IS_NPC(mob))
    return ROL_SHIP_NONE;

  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
  {
    if (GET_MOB_VNUM(mob) == rol_ship_definitions[ship_index].navigator_vnum)
      return ship_index;
  }

  return ROL_SHIP_NONE;
}

static int rol_ship_character_count(int ship_index)
{
  struct char_data *person;
  room_rnum room;
  room_vnum vnum;
  int count;

  count = 0;
  for (vnum = rol_ship_definitions[ship_index].first_interior_vnum;
       vnum <= rol_ship_definitions[ship_index].last_interior_vnum; vnum++)
  {
    room = real_room(vnum);
    if (room == NOWHERE)
      continue;
    for (person = world[room].people; person != NULL; person = person->next_in_room)
      count++;
  }
  return count;
}

static void rol_ship_broadcast(int ship_index, const char *message)
{
  room_rnum room;
  room_vnum vnum;

  for (vnum = rol_ship_definitions[ship_index].first_interior_vnum;
       vnum <= rol_ship_definitions[ship_index].last_interior_vnum; vnum++)
  {
    room = real_room(vnum);
    if (room != NOWHERE)
      send_to_room(room, "%s\r\n", message);
  }
}

static bool rol_ship_is_valid(int ship_index)
{
  struct obj_data *hull;

  hull = rol_ship_find_hull(ship_index);
  return hull != NULL && IN_ROOM(hull) != NOWHERE && IN_ROOM(hull) <= top_of_world;
}

static bool rol_ship_is_docked(int ship_index)
{
  struct rol_ship_state *state;

  if (!rol_ship_is_valid(ship_index))
    return false;

  state = &rol_ship_states[ship_index];
  if (state->docked_ship != ROL_SHIP_NONE)
    return true;

  return world[IN_ROOM(state->hull)].sector_type != SECT_OCEAN &&
         world[IN_ROOM(state->hull)].sector_type != SECT_WATER_NOSWIM;
}

static bool rol_ship_move(int ship_index, int direction, struct char_data *operator)
{
  struct rol_ship_state *state;
  struct room_direction_data *exit;
  room_rnum destination;
  char message[MAX_STRING_LENGTH];

  if (!rol_ship_is_valid(ship_index) || direction < 0 || direction >= ROL_SHIP_DIRECTIONS)
    return false;

  state = &rol_ship_states[ship_index];
  exit = world[IN_ROOM(state->hull)].dir_option[direction];
  if (exit == NULL || exit->to_room == NOWHERE || exit->to_room > top_of_world)
  {
    if (operator!= NULL)
      send_to_char(operator, "You will drop off the face of the world if you sail there.\r\n");
    return false;
  }

  destination = exit->to_room;
  if (!rol_ship_can_enter_sector(world[destination].sector_type,
                                 ROOM_FLAGGED(destination, ROOM_DOCKABLE)))
  {
    if (operator!= NULL)
      send_to_char(operator, "The ship can only sail on water.\r\n");
    return false;
  }

  snprintf(message, sizeof(message), "%s sails %s.", GET_OBJ_SHORT(state->hull), dirs[direction]);
  CAP(message);
  send_to_room(IN_ROOM(state->hull), "%s\r\n", message);
  obj_from_room(state->hull);
  obj_to_room(state->hull, destination);
  snprintf(message, sizeof(message), "%s sails here from the %s.", GET_OBJ_SHORT(state->hull),
           dirs[rev_dir[direction]]);
  CAP(message);
  send_to_room(destination, "%s\r\n", message);
  snprintf(message, sizeof(message), "Your ship sails %s.", dirs[direction]);
  rol_ship_broadcast(ship_index, message);

  if (ROOM_FLAGGED(destination, ROOM_DOCKABLE))
    rol_ship_broadcast(ship_index, "Your ship docks here.");
  return true;
}

static struct obj_data *rol_ship_find_named_hull(struct char_data *ch, int ship_index,
                                                 const char *name, int *target_index)
{
  struct obj_data *obj;
  int candidate;

  if (target_index != NULL)
    *target_index = ROL_SHIP_NONE;
  if (name == NULL || *name == '\0' || !rol_ship_is_valid(ship_index))
    return NULL;

  for (obj = world[IN_ROOM(rol_ship_states[ship_index].hull)].contents; obj != NULL;
       obj = obj->next_content)
  {
    candidate = rol_ship_index_for_hull(obj);
    if (candidate == ROL_SHIP_NONE || !isname(name, obj->name) || !CAN_SEE_OBJ(ch, obj))
      continue;
    if (target_index != NULL)
      *target_index = candidate;
    return obj;
  }
  return NULL;
}

static void rol_ship_transfer_interior(int ship_index, room_rnum destination)
{
  struct char_data *person;
  struct char_data *next_person;
  struct obj_data *obj;
  struct obj_data *next_obj;
  room_rnum room;
  room_vnum vnum;

  for (vnum = rol_ship_definitions[ship_index].first_interior_vnum;
       vnum <= rol_ship_definitions[ship_index].last_interior_vnum; vnum++)
  {
    room = real_room(vnum);
    if (room == NOWHERE)
      continue;

    for (obj = world[room].contents; obj != NULL; obj = next_obj)
    {
      next_obj = obj->next_content;
      obj_from_room(obj);
      obj_to_room(obj, destination);
    }
    for (person = world[room].people; person != NULL; person = next_person)
    {
      next_person = person->next_in_room;
      char_from_room(person);
      char_to_room(person, destination);
    }
  }
}

static void rol_ship_sink(int ship_index)
{
  struct rol_ship_state *state;
  struct obj_data *hull;
  room_rnum destination;
  int other_ship;

  if (!rol_ship_is_valid(ship_index))
    return;

  state = &rol_ship_states[ship_index];
  hull = state->hull;
  destination = IN_ROOM(hull);
  send_to_room(destination, "%s sinks into the water.\r\n", GET_OBJ_SHORT(hull));
  rol_ship_broadcast(ship_index,
                     "Your ship sinks into the water! You are thrown into the surrounding water!");
  rol_ship_transfer_interior(ship_index, destination);

  other_ship = state->docked_ship;
  if (other_ship >= 0 && other_ship < rol_ship_count())
    rol_ship_states[other_ship].docked_ship = ROL_SHIP_NONE;
  rol_ship_reset_state(ship_index, NULL);
  extract_obj(hull);
}

SPECIAL(rol_ship)
{
  struct obj_data *hull;
  struct obj_data *target;
  room_rnum entrance;
  int ship_index;
  char name[MAX_INPUT_LENGTH];

  if (!cmd || !CMD_IS("enter") || ch == NULL || me == NULL)
    return false;

  hull = (struct obj_data *)me;
  one_argument(argument, name, sizeof(name));
  target = get_obj_in_list_vis(ch, name, NULL, world[IN_ROOM(ch)].contents);
  if (target != hull)
    return false;

  ship_index = rol_ship_index_for_hull(hull);
  if (ship_index == ROL_SHIP_NONE || !rol_ship_is_valid(ship_index))
  {
    send_to_char(ch, "That ship is not operable.\r\n");
    return true;
  }
  if (rol_ship_character_count(ship_index) >= rol_ship_states[ship_index].capacity)
  {
    send_to_char(ch, "Too many people are already aboard that ship!\r\n");
    return true;
  }

  entrance = real_room(rol_ship_definitions[ship_index].entrance_vnum);
  if (entrance == NOWHERE)
  {
    send_to_char(ch, "That ship's entrance is unavailable.\r\n");
    return true;
  }

  act("$n boards $p.", true, ch, hull, NULL, TO_ROOM);
  act("You board $p.", false, ch, hull, NULL, TO_CHAR);
  char_from_room(ch);
  char_to_room(ch, entrance);
  act("$n comes aboard the ship.", true, ch, NULL, NULL, TO_ROOM);
  look_at_room(ch, 0);
  return true;
}

static int rol_ship_control_look(struct char_data *ch, int ship_index, const char *argument)
{
  struct rol_ship_state *state;
  char name[MAX_INPUT_LENGTH];

  one_argument(argument, name, sizeof(name));
  if (!isname(name, "panel instrument instruments"))
    return false;
  if (!rol_ship_is_valid(ship_index))
  {
    send_to_char(ch, "This ship is not operable.\r\n");
    return true;
  }

  state = &rol_ship_states[ship_index];
  send_to_char(ch, "You examine %s's instruments and see:\r\n", GET_OBJ_SHORT(state->hull));
  send_to_char(ch, "  hull: (%d/%d), speed: (%d/%d), velocity: %d\r\n", GET_OBJ_VAL(state->hull, 0),
               state->max_hull, GET_OBJ_VAL(state->hull, 1), state->max_speed, state->velocity);
  send_to_char(ch, "  fire-power: %d, docked: %s, capacity: (%d/%d)\r\n", state->damage,
               rol_ship_is_docked(ship_index) ? "yes" : "no", rol_ship_character_count(ship_index),
               state->capacity);
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    send_to_char(
        ch, "  size: %d, rooms: %d, repeat: %d, timers: %d/%d, exterior: %d\r\n", state->size,
        rol_ship_definitions[ship_index].last_interior_vnum -
            rol_ship_definitions[ship_index].first_interior_vnum + 1,
        state->repeat, state->action_timer, state->move_timer, GET_ROOM_VNUM(IN_ROOM(state->hull)));
  }
  return true;
}

static int rol_ship_direction(const char *name)
{
  int direction;

  for (direction = 0; direction < ROL_SHIP_DIRECTIONS; direction++)
  {
    if (is_abbrev(name, dirs[direction]))
      return direction;
  }
  return ROL_SHIP_NONE;
}

static int rol_ship_control_sail(struct char_data *ch, int ship_index, const char *argument)
{
  struct rol_ship_state *state;
  char direction_name[MAX_INPUT_LENGTH];
  char repeat_name[MAX_INPUT_LENGTH];
  int direction;
  int repeat;

  argument = one_argument(argument, direction_name, sizeof(direction_name));
  one_argument(argument, repeat_name, sizeof(repeat_name));
  direction = rol_ship_direction(direction_name);
  if (direction == ROL_SHIP_NONE)
  {
    send_to_char(ch, "You must provide a direction.\r\n");
    return true;
  }

  state = &rol_ship_states[ship_index];
  state->repeat = 0;
  if (state->move_timer > 0)
  {
    send_to_char(ch, "The ship is slow to respond to your control.\r\n");
    return true;
  }
  if (state->docked_ship != ROL_SHIP_NONE)
  {
    rol_ship_states[state->docked_ship].docked_ship = ROL_SHIP_NONE;
    state->docked_ship = ROL_SHIP_NONE;
    rol_ship_broadcast(ship_index, "The ship casts off from the other vessel.");
  }

  send_to_char(ch, "You shout an order to sail %s!\r\n", dirs[direction]);
  act("$n shouts an order to sail!", false, ch, NULL, NULL, TO_ROOM);
  if (!rol_ship_move(ship_index, direction, ch))
    return true;

  if (state->velocity <= 0)
    state->velocity = MAX(1, state->max_speed >> 2);
  state->last_direction = direction;
  state->move_timer = rol_ship_move_delay_for_speed(state->velocity);
  repeat = is_number(repeat_name) ? MIN(50, atoi(repeat_name)) : 0;
  if (repeat > 1)
  {
    state->repeat = repeat - 1;
    send_to_char(ch, "Engaging auto-mode.\r\n");
  }
  return true;
}

static int rol_ship_control_speed(struct char_data *ch, int ship_index, const char *argument)
{
  struct rol_ship_state *state;
  char speed[MAX_INPUT_LENGTH];

  state = &rol_ship_states[ship_index];
  one_argument(argument, speed, sizeof(speed));
  if (rol_ship_is_docked(ship_index))
  {
    send_to_char(ch, "You cannot control the speed of a docked ship.\r\n");
    return true;
  }

  if (is_abbrev(speed, "fast"))
    state->velocity = state->max_speed;
  else if (is_abbrev(speed, "medium"))
    state->velocity = MAX(1, state->max_speed >> 1);
  else if (is_abbrev(speed, "slow"))
    state->velocity = MAX(1, state->max_speed >> 2);
  else if (is_abbrev(speed, "stop"))
    state->velocity = 0;
  else
  {
    send_to_char(ch, "Choose fast, medium, slow, or stop.\r\n");
    return true;
  }
  send_to_char(ch, "You order the ship's speed changed to %s.\r\n", speed);
  return true;
}

static int rol_ship_control_fire(struct char_data *ch, int ship_index, const char *argument)
{
  struct rol_ship_state *state;
  struct rol_ship_state *victim_state;
  struct obj_data *victim_hull;
  int victim_ship;
  char name[MAX_INPUT_LENGTH];

  state = &rol_ship_states[ship_index];
  one_argument(argument, name, sizeof(name));
  if (rol_ship_is_docked(ship_index))
  {
    send_to_char(ch, "You cannot fire from a docked ship.\r\n");
    return true;
  }
  if (state->action_timer > 0)
  {
    send_to_char(ch, "The ship is slow to respond to your control.\r\n");
    return true;
  }
  if (state->damage <= 0)
  {
    send_to_char(ch, "This ship has no working shipboard weapon.\r\n");
    return true;
  }

  victim_hull = rol_ship_find_named_hull(ch, ship_index, name, &victim_ship);
  if (victim_hull == NULL || victim_ship == ship_index)
  {
    send_to_char(ch, "There is no other operable ship by that name here.\r\n");
    return true;
  }

  victim_state = &rol_ship_states[victim_ship];
  GET_OBJ_VAL(victim_hull, 0) -= state->damage;
  rol_ship_broadcast(ship_index, "Your ship fires its weapons with a deafening boom!");
  rol_ship_broadcast(victim_ship, "Another ship fires on you! The hull shakes and creaks!");
  send_to_room(IN_ROOM(state->hull), "%s fires on %s.\r\n", GET_OBJ_SHORT(state->hull),
               GET_OBJ_SHORT(victim_hull));
  state->action_timer = rol_ship_move_delay_for_speed(state->max_speed);
  if (GET_OBJ_VAL(victim_state->hull, 0) <= 0)
    rol_ship_sink(victim_ship);
  return true;
}

static int rol_ship_control_ram(struct char_data *ch, int ship_index, const char *argument)
{
  struct rol_ship_state *state;
  struct rol_ship_state *victim_state;
  struct obj_data *victim_hull;
  int victim_ship;
  char name[MAX_INPUT_LENGTH];

  state = &rol_ship_states[ship_index];
  one_argument(argument, name, sizeof(name));
  if (rol_ship_is_docked(ship_index) || state->action_timer > 0)
  {
    send_to_char(ch, "The ship cannot ram another vessel right now.\r\n");
    return true;
  }
  victim_hull = rol_ship_find_named_hull(ch, ship_index, name, &victim_ship);
  if (victim_hull == NULL || victim_ship == ship_index)
  {
    send_to_char(ch, "What other operable ship do you want to ram?\r\n");
    return true;
  }

  victim_state = &rol_ship_states[victim_ship];
  GET_OBJ_VAL(victim_hull, 0) -= state->size;
  GET_OBJ_VAL(state->hull, 0) -= victim_state->size;
  rol_ship_broadcast(ship_index, "Your ship rams another vessel! The hull shakes violently!");
  rol_ship_broadcast(victim_ship, "Another ship rams you! The hull shakes violently!");
  state->action_timer = rol_ship_move_delay_for_speed(state->max_speed);
  if (GET_OBJ_VAL(victim_hull, 0) <= 0)
    rol_ship_sink(victim_ship);
  if (state->hull != NULL && GET_OBJ_VAL(state->hull, 0) <= 0)
    rol_ship_sink(ship_index);
  return true;
}

static int rol_ship_control_board(struct char_data *ch, int ship_index, const char *argument)
{
  struct rol_ship_state *state;
  struct rol_ship_state *victim_state;
  struct obj_data *victim_hull;
  int victim_ship;
  char name[MAX_INPUT_LENGTH];

  state = &rol_ship_states[ship_index];
  one_argument(argument, name, sizeof(name));
  if (rol_ship_is_docked(ship_index))
  {
    send_to_char(ch, "This ship is already docked.\r\n");
    return true;
  }
  victim_hull = rol_ship_find_named_hull(ch, ship_index, name, &victim_ship);
  if (victim_hull == NULL || victim_ship == ship_index || rol_ship_is_docked(victim_ship))
  {
    send_to_char(ch, "You cannot dock with that ship.\r\n");
    return true;
  }

  victim_state = &rol_ship_states[victim_ship];
  if (GET_OBJ_VAL(state->hull, 1) < GET_OBJ_VAL(victim_state->hull, 1))
  {
    send_to_char(ch, "The other ship is too fast to board.\r\n");
    return true;
  }
  state->repeat = 0;
  state->velocity = 0;
  state->docked_ship = victim_ship;
  victim_state->docked_ship = ship_index;
  state->action_timer = rol_ship_move_delay_for_speed(state->max_speed);
  rol_ship_broadcast(ship_index, "The two ships pull alongside and are secured together.");
  rol_ship_broadcast(victim_ship, "The two ships pull alongside and are secured together.");
  return true;
}

SPECIAL(rol_ship_control)
{
  const char *order_arguments;
  int ship_index;
  char order[MAX_INPUT_LENGTH];

  (void)me;
  if (!cmd || ch == NULL)
    return false;

  ship_index = rol_ship_index_for_room(IN_ROOM(ch));
  if (ship_index == ROL_SHIP_NONE)
    return false;
  if (CMD_IS("look"))
    return rol_ship_control_look(ch, ship_index, argument);
  if (!CMD_IS("order"))
    return false;
  if (!rol_ship_is_valid(ship_index))
  {
    send_to_char(ch, "This ship is not operable.\r\n");
    return true;
  }

  order_arguments = one_argument(argument, order, sizeof(order));
  if (is_abbrev(order, "sail"))
    return rol_ship_control_sail(ch, ship_index, order_arguments);
  if (is_abbrev(order, "speed"))
    return rol_ship_control_speed(ch, ship_index, order_arguments);
  if (is_abbrev(order, "fire"))
    return rol_ship_control_fire(ch, ship_index, order_arguments);
  if (is_abbrev(order, "ram"))
    return rol_ship_control_ram(ch, ship_index, order_arguments);
  if (is_abbrev(order, "board"))
    return rol_ship_control_board(ch, ship_index, order_arguments);
  return false;
}

static bool rol_ship_look_out(struct char_data *ch, int ship_index, const char *argument)
{
  char first[MAX_INPUT_LENGTH];

  one_argument(argument, first, sizeof(first));
  if (!is_abbrev(first, "out"))
    return false;
  if (!rol_ship_is_valid(ship_index))
  {
    send_to_char(ch, "This ship is not operable.\r\n");
    return true;
  }
  look_at_room_number(ch, 1, IN_ROOM(rol_ship_states[ship_index].hull));
  return true;
}

SPECIAL(rol_ship_lookout)
{
  int ship_index;

  (void)me;
  if (!cmd || ch == NULL || !CMD_IS("look"))
    return false;
  ship_index = rol_ship_index_for_room(IN_ROOM(ch));
  if (ship_index == ROL_SHIP_NONE)
    return false;
  return rol_ship_look_out(ch, ship_index, argument);
}

SPECIAL(rol_ship_exit)
{
  struct rol_ship_state *state;
  room_rnum destination;
  int ship_index;
  int other_ship;

  (void)me;
  if (!cmd || ch == NULL || (!CMD_IS("look") && !CMD_IS("disembark")))
    return false;
  ship_index = rol_ship_index_for_room(IN_ROOM(ch));
  if (ship_index == ROL_SHIP_NONE)
    return false;
  if (CMD_IS("look"))
    return rol_ship_look_out(ch, ship_index, argument);
  if (!rol_ship_is_valid(ship_index))
  {
    send_to_char(ch, "This ship is not operable.\r\n");
    return true;
  }
  if (!rol_ship_is_docked(ship_index) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "You should wait until the ship is docked before disembarking.\r\n");
    return true;
  }
  if (GET_POS(ch) < POS_STANDING)
  {
    send_to_char(ch, "You are in no position to disembark.\r\n");
    return true;
  }

  state = &rol_ship_states[ship_index];
  other_ship = state->docked_ship;
  if (other_ship != ROL_SHIP_NONE && rol_ship_is_valid(other_ship))
  {
    destination = real_room(rol_ship_definitions[other_ship].entrance_vnum);
    send_to_char(ch, "You cross over to the other ship.\r\n");
  }
  else
  {
    destination = IN_ROOM(state->hull);
    send_to_char(ch, "You disembark from the ship.\r\n");
  }
  if (destination == NOWHERE)
    return true;

  act("$n disembarks from the ship.", true, ch, NULL, NULL, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, destination);
  if (ZONE_FLAGGED(GET_ROOM_ZONE(destination), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[destination].coords[0];
    Y_LOC(ch) = world[destination].coords[1];
  }
  act("$n arrives from a ship.", true, ch, NULL, NULL, TO_ROOM);
  look_at_room(ch, 0);
  return true;
}

static bool rol_ship_has_navigator(int ship_index)
{
  struct char_data *mob;
  room_rnum control_room;

  control_room = real_room(rol_ship_definitions[ship_index].control_vnum);
  if (control_room == NOWHERE)
    return false;
  for (mob = world[control_room].people; mob != NULL; mob = mob->next_in_room)
  {
    if (IS_NPC(mob) && GET_MOB_VNUM(mob) == rol_ship_definitions[ship_index].navigator_vnum)
      return true;
  }
  return false;
}

static void rol_ship_route_tick(int ship_index)
{
  const struct rol_ship_definition *definition;
  struct rol_ship_state *state;
  room_vnum exterior_vnum;
  int direction;

  if (!rol_ship_is_valid(ship_index) || !rol_ship_has_navigator(ship_index))
    return;

  definition = &rol_ship_definitions[ship_index];
  state = &rol_ship_states[ship_index];
  exterior_vnum = GET_ROOM_VNUM(IN_ROOM(state->hull));
  if (!state->route_sailing &&
      (time_info.hours - definition->sail_hour) % definition->frequency == 0)
  {
    if (exterior_vnum == definition->route_start_vnum)
    {
      state->route_path = definition->outbound_path;
      state->route_destination = definition->route_destination_vnum;
      state->route_sailing = true;
      rol_ship_broadcast(ship_index, "Someone shouts, 'All right, we are ready to sail!'");
    }
    else if (exterior_vnum == definition->route_destination_vnum && definition->return_path != NULL)
    {
      state->route_path = definition->return_path;
      state->route_destination = definition->route_start_vnum;
      state->route_sailing = true;
      rol_ship_broadcast(ship_index, "Someone shouts, 'All right, we are ready to sail!'");
    }
  }
  if (!state->route_sailing || state->move_timer > 0 || state->route_path == NULL)
    return;

  if (*state->route_path == '.')
  {
    if (exterior_vnum == state->route_destination)
      rol_ship_broadcast(
          ship_index,
          "Someone shouts, 'We have arrived at our destination! Prepare to disembark!'");
    else
      rol_ship_broadcast(ship_index, "Someone shouts, 'Help us! We are lost!'");
    state->route_sailing = false;
    state->route_path = NULL;
    return;
  }

  direction = *state->route_path - '0';
  state->route_path++;
  if (direction < 0 || direction >= ROL_SHIP_DIRECTIONS)
  {
    state->route_sailing = false;
    state->route_path = NULL;
    log("SYSERR: RoL ship %d contains an invalid route direction.", definition->hull_vnum);
    return;
  }
  state->velocity = state->max_speed;
  if (rol_ship_move(ship_index, direction, NULL))
  {
    state->last_direction = direction;
    state->move_timer = rol_ship_move_delay_for_speed(state->velocity);
  }
  else
  {
    state->route_sailing = false;
    state->route_path = NULL;
    rol_ship_broadcast(ship_index, "Someone shouts, 'Help us! The route is blocked!'");
  }
}

void rol_ship_activity_one(int ship_index)
{
  struct rol_ship_state *state;

  if (ship_index < 0 || ship_index >= rol_ship_count())
    return;
  state = &rol_ship_states[ship_index];
  if (!rol_ship_is_valid(ship_index))
    return;

  rol_ship_route_tick(ship_index);
  if (state->action_timer > 0)
    state->action_timer--;
  if (state->move_timer > 0)
    state->move_timer--;
  if (state->repeat > 0 && state->move_timer == 0 && state->velocity > 0)
  {
    state->move_timer = rol_ship_move_delay_for_speed(state->velocity);
    if (!rol_ship_move(ship_index, state->last_direction, NULL))
      state->repeat = 0;
    else
      state->repeat--;
  }
}

void rol_ship_activity(void)
{
  int ship_index;

  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
    rol_ship_activity_one(ship_index);
}

void rol_ship_note_object_placed(struct obj_data *obj)
{
  int ship_index;

  if (obj == NULL)
    return;
  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
  {
    if (GET_OBJ_VNUM(obj) != rol_ship_definitions[ship_index].hull_vnum)
      continue;
    if (rol_ship_states[ship_index].hull == NULL)
      rol_ship_reset_state(ship_index, obj);
    if (rol_ship_states[ship_index].hull == obj)
      rol_ship_schedule(ship_index);
    return;
  }
}

void rol_ship_note_object_extracted(struct obj_data *obj)
{
  int ship_index;

  if (obj == NULL)
    return;
  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
  {
    if (rol_ship_states[ship_index].hull != obj)
      continue;
    rol_ship_cancel(ship_index);
    rol_ship_reset_state(ship_index, NULL);
    return;
  }
}

void rol_ship_periodic_init(void)
{
  int ship_index;

  rol_periodic_initialized = true;
  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
    rol_ship_schedule(ship_index);
}

void rol_ship_periodic_shutdown(void)
{
  int ship_index;

  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
    rol_ship_cancel(ship_index);
  rol_periodic_initialized = false;
}

size_t rol_ship_periodic_loaded_count(void)
{
  size_t count = 0U;
  int ship_index;

  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
  {
    if (rol_ship_states[ship_index].hull != NULL &&
        IN_ROOM(rol_ship_states[ship_index].hull) != NOWHERE &&
        IN_ROOM(rol_ship_states[ship_index].hull) <= top_of_world)
      count++;
  }
  return count;
}

size_t rol_ship_periodic_scheduled_count(void)
{
  size_t count = 0U;
  int ship_index;

  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
  {
    if (!event_runtime_handle_is_none(rol_ship_states[ship_index].periodic_event_handle))
      count++;
  }
  return count;
}

size_t rol_ship_periodic_validate(void)
{
  struct rol_ship_state *state;
  size_t mismatches = 0U;
  int ship_index;
  bool loaded;

  for (ship_index = 0; ship_index < rol_ship_count(); ship_index++)
  {
    state = &rol_ship_states[ship_index];
    loaded = state->hull != NULL && IN_ROOM(state->hull) != NOWHERE &&
             IN_ROOM(state->hull) <= top_of_world;
    if ((!event_runtime_handle_is_none(state->periodic_event_handle)) !=
        (CONFIG_VESSEL_SYSTEM && vessel_periodic_events_enabled() && loaded))
      mismatches++;
    if (!event_runtime_handle_is_none(state->periodic_event_handle) &&
        state->periodic_generation == 0U)
      mismatches++;
  }
  return mismatches;
}

uint64_t rol_ship_periodic_callbacks(void)
{
  return rol_periodic_callback_count;
}

static void rol_ship_call_helpers(struct char_data *navigator, int ship_index)
{
  static const mob_vnum realms_helpers[] = {2011102, 2011103, 2011104, 2011105, 2011107,
                                            2011108, 2011110, 2011112, 2011114, NOBODY};
  static const mob_vnum silver_helpers[] = {2011302, 2011303, 2011304, 2011305, 2011307,
                                            2011308, 2011310, 2011312, 2011314, NOBODY};
  const mob_vnum *helpers;
  struct char_data *helper;
  struct char_data *victim;
  int helper_index;

  if (ship_index != 1 && ship_index != 2)
    return;
  victim = FIGHTING(navigator);
  if (victim == NULL || rol_ship_states[ship_index].last_help_victim == victim)
    return;

  rol_ship_states[ship_index].last_help_victim = victim;
  act("$n shouts, 'Help me mates! We are under attack by $N!'", false, navigator, NULL, victim,
      TO_ROOM);
  helpers = ship_index == 1 ? realms_helpers : silver_helpers;
  for (helper_index = 0; helpers[helper_index] != NOBODY; helper_index++)
  {
    for (helper = character_list; helper != NULL; helper = helper->next)
    {
      if (!IS_NPC(helper) || GET_MOB_VNUM(helper) != helpers[helper_index] ||
          IN_ROOM(helper) == NOWHERE ||
          GET_ROOM_ZONE(IN_ROOM(helper)) != GET_ROOM_ZONE(IN_ROOM(navigator)))
        continue;
      if (helper != victim && FIGHTING(helper) == NULL)
        HUNTING(helper) = victim;
    }
  }
}

SPECIAL(rol_ship_navigator)
{
  struct char_data *navigator;
  int ship_index;

  if (me == NULL)
    return false;
  navigator = (struct char_data *)me;
  ship_index = rol_ship_index_for_navigator(navigator);
  if (ship_index == ROL_SHIP_NONE)
    return false;

  if (!cmd)
  {
    rol_ship_call_helpers(navigator, ship_index);
    return false;
  }
  if (ch == NULL || ch == navigator || !CMD_IS("order"))
    return false;

  act("$n growls at $N, 'Only I can order my ship to sail!'", false, navigator, NULL, ch,
      TO_NOTVICT);
  act("$n growls at you, 'Only I can order my ship to sail!'", false, navigator, NULL, ch, TO_VICT);
  (void)argument;
  return true;
}
