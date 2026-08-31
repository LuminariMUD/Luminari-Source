/**
 * @file spec/spec_rol_deaths_head.c
 * Converted Undermountain Death's Head lifecycle.
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
#include "mud_event.h"
#include "spec/spec_dispatch.h"
#include "spec/spec_rol_deaths_head.h"

#define ROL_DEATHS_HEAD_SAPLING_VNUM 2093013
#define ROL_DEATHS_HEAD_FRUIT_VNUM 2093014
#define ROL_DEATHS_HEAD_YOUNG_VNUM 2093015
#define ROL_DEATHS_HEAD_MATURE_VNUM 2093016
#define ROL_DEATHS_HEAD_SEED_VNUM 2093044

#define ROL_DEATHS_HEAD_SOURCE_PASSES_PER_SEC 4
#define ROL_DEATHS_HEAD_INITIAL_SEED_DELAY 240
#define ROL_DEATHS_HEAD_SEED_DELAY_MIN 14
#define ROL_DEATHS_HEAD_SEED_DELAY_MAX 18
#define ROL_DEATHS_HEAD_GERMINATE_THRESHOLD 50
#define ROL_DEATHS_HEAD_DROPOFF_THRESHOLD 10
#define ROL_DEATHS_HEAD_GROWTH_THRESHOLD 20

struct rol_deaths_head_mobile_profile
{
  int vnum;
  enum rol_deaths_head_kind kind;
};

static const struct rol_deaths_head_mobile_profile rol_deaths_head_mobile_profiles[] = {
    {ROL_DEATHS_HEAD_SAPLING_VNUM, ROL_DEATHS_HEAD_SAPLING},
    {ROL_DEATHS_HEAD_FRUIT_VNUM, ROL_DEATHS_HEAD_FRUIT},
    {ROL_DEATHS_HEAD_YOUNG_VNUM, ROL_DEATHS_HEAD_YOUNG},
    {ROL_DEATHS_HEAD_MATURE_VNUM, ROL_DEATHS_HEAD_MATURE},
};

size_t rol_deaths_head_mobile_profile_count(void)
{
  return sizeof(rol_deaths_head_mobile_profiles) / sizeof(rol_deaths_head_mobile_profiles[0]);
}

bool rol_deaths_head_mobile_profile(int mobile_vnum, enum rol_deaths_head_kind *kind)
{
  size_t index;

  if (kind != NULL)
    *kind = ROL_DEATHS_HEAD_NONE;
  for (index = 0; index < rol_deaths_head_mobile_profile_count(); index++)
  {
    if (rol_deaths_head_mobile_profiles[index].vnum != mobile_vnum)
      continue;
    if (kind != NULL)
      *kind = rol_deaths_head_mobile_profiles[index].kind;
    return true;
  }
  return false;
}

bool rol_deaths_head_seed_profile(int object_vnum)
{
  return object_vnum == ROL_DEATHS_HEAD_SEED_VNUM;
}

int rol_deaths_head_initial_head_min(enum rol_deaths_head_kind kind)
{
  switch (kind)
  {
  case ROL_DEATHS_HEAD_SAPLING:
    return 1;
  case ROL_DEATHS_HEAD_YOUNG:
    return 6;
  case ROL_DEATHS_HEAD_MATURE:
    return 11;
  default:
    return 0;
  }
}

int rol_deaths_head_initial_head_max(enum rol_deaths_head_kind kind)
{
  switch (kind)
  {
  case ROL_DEATHS_HEAD_SAPLING:
    return 5;
  case ROL_DEATHS_HEAD_YOUNG:
    return 10;
  case ROL_DEATHS_HEAD_MATURE:
    return 16;
  default:
    return 0;
  }
}

int rol_deaths_head_mature_regrowth_count(int current_heads, int random_heads)
{
  int source_value;

  source_value = current_heads + random_heads;
  return MIN(MAX(11, source_value), 11);
}

bool rol_deaths_head_larger_tree(enum rol_deaths_head_kind current,
                                 enum rol_deaths_head_kind candidate)
{
  if (current == ROL_DEATHS_HEAD_SAPLING)
    return candidate == ROL_DEATHS_HEAD_YOUNG || candidate == ROL_DEATHS_HEAD_MATURE;
  if (current == ROL_DEATHS_HEAD_YOUNG)
    return candidate == ROL_DEATHS_HEAD_MATURE;
  return false;
}

bool rol_deaths_head_mature_wood_drop_enabled(void)
{
  /* The source compares the mature mobile rnum to object 93016's rnum. */
  return false;
}

long rol_deaths_head_source_delay_pulses(int source_pulses)
{
  source_pulses = MAX(1, source_pulses);
  return MAX(1L, (long)source_pulses * PASSES_PER_SEC / ROL_DEATHS_HEAD_SOURCE_PASSES_PER_SEC);
}

int rol_deaths_head_seed_damage_min(int growth)
{
  growth = MAX(1, growth);
  return MIN(growth, 2);
}

int rol_deaths_head_seed_damage_max(int growth)
{
  growth = MAX(1, growth);
  return MAX(growth, 2);
}

static bool rol_deaths_head_is_tree(enum rol_deaths_head_kind kind)
{
  return kind == ROL_DEATHS_HEAD_SAPLING || kind == ROL_DEATHS_HEAD_YOUNG ||
         kind == ROL_DEATHS_HEAD_MATURE;
}

static void rol_deaths_head_initialize_tree(struct char_data *tree, enum rol_deaths_head_kind kind)
{
  int minimum;
  int maximum;

  if (tree == NULL || tree->mob_specials.rol_deaths_head_initialized ||
      !rol_deaths_head_is_tree(kind))
    return;

  minimum = rol_deaths_head_initial_head_min(kind);
  maximum = rol_deaths_head_initial_head_max(kind);
  tree->mob_specials.rol_deaths_head_count = rand_number(minimum, maximum);
  tree->mob_specials.rol_deaths_head_cycle = 0;
  tree->mob_specials.rol_deaths_head_drop = 0;
  tree->mob_specials.rol_deaths_head_initialized = true;
}

static bool rol_deaths_head_source_corpse(const struct obj_data *obj)
{
  /* Source corpse value[2] is the victim level for both PCs and NPCs. */
  return obj != NULL && IS_CORPSE(obj);
}

static struct char_data *rol_deaths_head_object_owner(struct obj_data *obj)
{
  for (; obj != NULL; obj = obj->in_obj)
  {
    if (obj->worn_by != NULL)
      return obj->worn_by;
    if (obj->carried_by != NULL)
      return obj->carried_by;
  }
  return NULL;
}

static bool rol_deaths_head_room_has_tree(room_rnum room)
{
  struct char_data *candidate;
  enum rol_deaths_head_kind kind;

  if (!VALID_ROOM_RNUM(room))
    return false;
  for (candidate = world[room].people; candidate != NULL; candidate = candidate->next_in_room)
    if (IS_NPC(candidate) && rol_deaths_head_mobile_profile(GET_MOB_VNUM(candidate), &kind) &&
        rol_deaths_head_is_tree(kind))
      return true;
  return false;
}

static struct char_data *rol_deaths_head_find_larger_tree(struct char_data *tree,
                                                          enum rol_deaths_head_kind kind)
{
  struct char_data *candidate;
  enum rol_deaths_head_kind candidate_kind;

  if (tree == NULL || !VALID_ROOM_RNUM(IN_ROOM(tree)))
    return NULL;
  for (candidate = world[IN_ROOM(tree)].people; candidate != NULL;
       candidate = candidate->next_in_room)
  {
    if (candidate == tree || !IS_NPC(candidate) ||
        !rol_deaths_head_mobile_profile(GET_MOB_VNUM(candidate), &candidate_kind))
      continue;
    if (rol_deaths_head_larger_tree(kind, candidate_kind))
      return candidate;
  }
  return NULL;
}

static int rol_deaths_head_count_corpses(room_rnum room)
{
  struct obj_data *corpse;
  int count = 0;

  if (!VALID_ROOM_RNUM(room))
    return 0;
  for (corpse = world[room].contents; corpse != NULL; corpse = corpse->next_content)
    if (rol_deaths_head_source_corpse(corpse))
      count++;
  return count;
}

static int rol_deaths_head_next_tree_vnum(enum rol_deaths_head_kind kind)
{
  if (kind == ROL_DEATHS_HEAD_SAPLING)
    return ROL_DEATHS_HEAD_YOUNG_VNUM;
  if (kind == ROL_DEATHS_HEAD_YOUNG)
    return ROL_DEATHS_HEAD_MATURE_VNUM;
  return 0;
}

static int rol_deaths_head_immature_activity(struct spec_event_context *context,
                                             struct char_data *tree, enum rol_deaths_head_kind kind)
{
  struct char_data *replacement;
  int next_vnum;

  tree->mob_specials.rol_deaths_head_cycle++;
  if (tree->mob_specials.rol_deaths_head_cycle <= ROL_DEATHS_HEAD_GROWTH_THRESHOLD)
    return TRUE;

  if (rol_deaths_head_count_corpses(IN_ROOM(tree)) > 1 &&
      (next_vnum = rol_deaths_head_next_tree_vnum(kind)) > 0 &&
      (replacement = read_mobile(next_vnum, VIRTUAL)) != NULL)
  {
    act("$n appears to have grown a little!", FALSE, tree, NULL, NULL, TO_ROOM);
    char_to_room(replacement, IN_ROOM(tree));
    extract_char(tree);
    context->invalidation |= SPEC_INVALIDATE_OWNER | SPEC_INVALIDATE_ACTOR;
    return TRUE;
  }

  tree->mob_specials.rol_deaths_head_cycle = 0;
  return TRUE;
}

static void rol_deaths_head_drop_fruit(struct char_data *tree)
{
  struct char_data *fruit;
  int original_heads;
  int remaining_heads;
  int index;

  original_heads = tree->mob_specials.rol_deaths_head_count;
  remaining_heads = original_heads;
  for (index = 0; index < original_heads; index++)
  {
    if (rand_number(0, 2) != 0 || remaining_heads <= 1)
      continue;
    fruit = read_mobile(ROL_DEATHS_HEAD_FRUIT_VNUM, VIRTUAL);
    if (fruit == NULL || !VALID_ROOM_RNUM(IN_ROOM(tree)))
      break;
    char_to_room(fruit, IN_ROOM(tree));
    act("$N falls from $n.", FALSE, tree, NULL, fruit, TO_ROOM);
    remaining_heads--;
  }
  tree->mob_specials.rol_deaths_head_count = remaining_heads;
}

static int rol_deaths_head_cry(struct char_data *tree)
{
  static const char *const messages[] = {
      "A faint cry for help can be heard from the south.\r\n",
      "A faint cry for help can be heard from the west.\r\n",
      "A faint cry for help can be heard from the north.\r\n",
      "A faint cry for help can be heard from the east.\r\n",
      "A faint cry for help can be heard from below.\r\n",
      "A faint cry for help can be heard from above.\r\n",
  };
  int direction;

  if (rand_number(0, 9) != 0)
    return FALSE;

  act("\tRA faint cry for help can be heard from one of the heads of\tn $n\tR.\tn", FALSE, tree,
      NULL, NULL, TO_ROOM);
  act("\tROne of your heads cries out for help, in an attempt to lure a victim.\tn", FALSE, tree,
      NULL, NULL, TO_CHAR);
  for (direction = NORTH; direction <= DOWN; direction++)
  {
    if (EXIT(tree, direction) == NULL || !VALID_ROOM_RNUM(EXIT(tree, direction)->to_room))
      continue;
    send_to_room(EXIT(tree, direction)->to_room, "%s", messages[direction]);
    return TRUE;
  }
  return FALSE;
}

static int rol_deaths_head_mature_activity(struct char_data *tree)
{
  tree->mob_specials.rol_deaths_head_cycle++;
  if (tree->mob_specials.rol_deaths_head_cycle > ROL_DEATHS_HEAD_GERMINATE_THRESHOLD)
  {
    tree->mob_specials.rol_deaths_head_count = rol_deaths_head_mature_regrowth_count(
        tree->mob_specials.rol_deaths_head_count, rand_number(10, 15));
    act("Several new heads begin to grow on $n.", FALSE, tree, NULL, NULL, TO_ROOM);
    tree->mob_specials.rol_deaths_head_cycle = 0;
  }

  tree->mob_specials.rol_deaths_head_drop++;
  if (tree->mob_specials.rol_deaths_head_drop > ROL_DEATHS_HEAD_DROPOFF_THRESHOLD)
  {
    rol_deaths_head_drop_fruit(tree);
    tree->mob_specials.rol_deaths_head_drop = 0;
  }
  return rol_deaths_head_cry(tree);
}

static int rol_deaths_head_tree_activity(struct spec_event_context *context, struct char_data *tree,
                                         enum rol_deaths_head_kind kind)
{
  struct char_data *larger_tree;

  larger_tree = rol_deaths_head_find_larger_tree(tree, kind);
  if (larger_tree != NULL)
  {
    act("$n dies out as $N siphons away all of its nutrients.", FALSE, tree, NULL, larger_tree,
        TO_ROOM);
    extract_char(tree);
    context->invalidation |= SPEC_INVALIDATE_OWNER | SPEC_INVALIDATE_ACTOR;
    return TRUE;
  }

  if (kind == ROL_DEATHS_HEAD_SAPLING || kind == ROL_DEATHS_HEAD_YOUNG)
    return rol_deaths_head_immature_activity(context, tree, kind);
  return rol_deaths_head_mature_activity(tree);
}

static bool rol_deaths_head_corpse_contains_seed(struct obj_data *corpse)
{
  struct obj_data *candidate;

  if (corpse == NULL)
    return false;
  for (candidate = corpse->contains; candidate != NULL; candidate = candidate->next_content)
    if (rol_deaths_head_seed_profile(GET_OBJ_VNUM(candidate)))
      return true;
  return false;
}

static int rol_deaths_head_fruit_activity(struct spec_event_context *context,
                                          struct char_data *fruit)
{
  struct obj_data *corpse;
  struct obj_data *seed;
  room_rnum room;

  if (fruit == NULL || (room = IN_ROOM(fruit)) == NOWHERE || !VALID_ROOM_RNUM(room) ||
      rol_deaths_head_room_has_tree(room))
    return FALSE;

  for (corpse = world[room].contents; corpse != NULL; corpse = corpse->next_content)
  {
    if (!rol_deaths_head_source_corpse(corpse) || !rol_deaths_head_corpse_contains_seed(corpse))
      continue;
    seed = read_object(ROL_DEATHS_HEAD_SEED_VNUM, VIRTUAL);
    if (seed == NULL)
      return FALSE;
    obj_to_obj(seed, corpse);
    act("$n \tRburies itself deep inside a corpse.\tn", FALSE, fruit, NULL, NULL, TO_ROOM);
    extract_char(fruit);
    context->invalidation |= SPEC_INVALIDATE_OWNER | SPEC_INVALIDATE_ACTOR;
    return TRUE;
  }
  return FALSE;
}

static int rol_deaths_head_insert_seed(struct char_data *attacker, struct char_data *victim,
                                       bool tree_attack)
{
  struct obj_data *seed;

  if (attacker == NULL || victim == NULL || !VALID_ROOM_RNUM(IN_ROOM(attacker)))
    return FALSE;

  if (tree_attack)
  {
    act("A head from $n bites $N viciously!", FALSE, attacker, NULL, victim, TO_NOTVICT);
    act("A head from $n bites you viciously! The pain quickly subsides.", FALSE, attacker, NULL,
        victim, TO_VICT);
    act("One of your heads bites $n viciously!", FALSE, attacker, NULL, victim, TO_CHAR);
  }
  else
  {
    act("$n bites $N viciously!", FALSE, attacker, NULL, victim, TO_NOTVICT);
    act("$n bites you viciously! The pain quickly subsides.", FALSE, attacker, NULL, victim,
        TO_VICT);
    act("You bite $n viciously!", FALSE, attacker, NULL, victim, TO_CHAR);
  }

  seed = read_object(ROL_DEATHS_HEAD_SEED_VNUM, VIRTUAL);
  if (seed != NULL)
    obj_to_char(seed, victim);
  return TRUE;
}

static int rol_deaths_head_tree_hit(struct char_data *tree)
{
  struct char_data *victim;
  int index;

  if (tree == NULL || !AWAKE(tree) || (victim = FIGHTING(tree)) == NULL)
    return FALSE;
  for (index = 0; index < tree->mob_specials.rol_deaths_head_count; index++)
    if (rand_number(0, 20) == 0)
      return rol_deaths_head_insert_seed(tree, victim, true);
  return FALSE;
}

static int rol_deaths_head_fruit_hit(struct char_data *fruit)
{
  struct char_data *victim;

  if (fruit == NULL || !AWAKE(fruit) || (victim = FIGHTING(fruit)) == NULL || IS_NPC(victim) ||
      GET_LEVEL(victim) >= LVL_IMMORT || rand_number(0, 10) != 0)
    return FALSE;
  return rol_deaths_head_insert_seed(fruit, victim, false);
}

static void rol_deaths_head_schedule_seed(struct obj_data *seed)
{
  if (seed == NULL || obj_has_mud_event(seed, eROL_DEATHS_HEAD_SEED) != NULL)
    return;
  attach_mud_event(new_mud_event(eROL_DEATHS_HEAD_SEED, seed, NULL),
                   rol_deaths_head_source_delay_pulses(ROL_DEATHS_HEAD_INITIAL_SEED_DELAY));
}

static bool rol_deaths_head_seed_can_germinate(struct obj_data *seed, room_rnum *room)
{
  struct obj_data *corpse;
  room_rnum seed_room;

  if (room != NULL)
    *room = NOWHERE;
  if (seed == NULL || rol_deaths_head_object_owner(seed) != NULL ||
      (corpse = seed->in_obj) == NULL || !rol_deaths_head_source_corpse(corpse) ||
      (seed_room = obj_room(seed)) == NOWHERE || !VALID_ROOM_RNUM(seed_room) ||
      world[seed_room].people == NULL)
    return false;
  if (room != NULL)
    *room = seed_room;
  return true;
}

static bool rol_deaths_head_extract_running_seed(struct mud_event_data *event,
                                                 struct obj_data *seed)
{
  if (event == NULL || seed == NULL)
    return false;
  mud_event_detach_owner(event);
  extract_obj(seed);
  return true;
}

static bool rol_deaths_head_try_germinate(struct mud_event_data *event, struct obj_data *seed,
                                          struct spec_event_context *context)
{
  struct char_data *tree;
  room_rnum room;

  if (!rol_deaths_head_seed_can_germinate(seed, &room))
    return false;

  if (rol_deaths_head_room_has_tree(room) || room <= 0 ||
      (tree = read_mobile(ROL_DEATHS_HEAD_SAPLING_VNUM, VIRTUAL)) == NULL)
  {
    if (event != NULL)
      (void)rol_deaths_head_extract_running_seed(event, seed);
    else
      extract_obj(seed);
    if (context != NULL)
      context->invalidation |= SPEC_INVALIDATE_OWNER;
    return true;
  }

  char_to_room(tree, room);
  send_to_room(room, "\tRA tree sapling has sprouted up here near the corpse.\tn\r\n");
  return true;
}

EVENTFUNC(event_rol_deaths_head_seed)
{
  struct mud_event_data *event = event_obj;
  struct obj_data *seed;
  struct char_data *owner;
  int growth;
  int damage;

  if (event == NULL || (seed = event->pStruct) == NULL ||
      !rol_deaths_head_seed_profile(GET_OBJ_VNUM(seed)))
    return 0;

  if (rol_deaths_head_try_germinate(event, seed, NULL))
    return 0;

  GET_OBJ_VAL(seed, 0)++;
  growth = GET_OBJ_VAL(seed, 0);
  owner = rol_deaths_head_object_owner(seed);
  if (owner == NULL || GET_POS(owner) == POS_DEAD)
    return 0;

  if (IS_NPC(owner) || GET_LEVEL(owner) < LVL_IMMORT)
  {
    damage = rand_number(rol_deaths_head_seed_damage_min(growth),
                         rol_deaths_head_seed_damage_max(growth));
    GET_HIT(owner) -= damage;
  }
  act("You wince in pain as $p grows inside of you.", FALSE, owner, seed, NULL, TO_CHAR);
  return rol_deaths_head_source_delay_pulses(
      rand_number(ROL_DEATHS_HEAD_SEED_DELAY_MIN, ROL_DEATHS_HEAD_SEED_DELAY_MAX));
}

int rol_deaths_head(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  return FALSE;
}

int rol_deaths_head_typed(struct spec_event_context *context)
{
  struct char_data *mobile;
  struct obj_data *seed;
  enum rol_deaths_head_kind kind;

  if (context == NULL || context->owner == NULL)
    return FALSE;

  if (context->owner_type == SPEC_OWNER_OBJECT)
  {
    seed = context->owner;
    if (context->event != SPEC_EVENT_OBJECT_AUTO_PULSE ||
        !rol_deaths_head_seed_profile(GET_OBJ_VNUM(seed)))
      return FALSE;
    if (!seed->rol_deaths_head_seed_initialized)
    {
      seed->rol_deaths_head_seed_initialized = true;
      GET_OBJ_VAL(seed, 0) = 0;
      rol_deaths_head_schedule_seed(seed);
    }
    (void)rol_deaths_head_try_germinate(NULL, seed, context);
    return FALSE;
  }

  if (context->owner_type != SPEC_OWNER_MOBILE)
    return FALSE;
  mobile = context->owner;
  if (!rol_deaths_head_mobile_profile(GET_MOB_VNUM(mobile), &kind))
    return FALSE;
  if (rol_deaths_head_is_tree(kind))
    rol_deaths_head_initialize_tree(mobile, kind);

  switch (context->event)
  {
  case SPEC_EVENT_MOBILE_ACTIVITY:
    if (kind == ROL_DEATHS_HEAD_FRUIT)
      return rol_deaths_head_fruit_activity(context, mobile);
    return rol_deaths_head_tree_activity(context, mobile, kind);
  case SPEC_EVENT_MOBILE_DEATH:
    /* Both source callbacks suppress their ordinary corpse. The source's
     * mature-tree wood condition is unreachable in its actual loaded indexes. */
    return TRUE;
  case SPEC_EVENT_MOBILE_HIT:
    if (kind == ROL_DEATHS_HEAD_FRUIT)
      return rol_deaths_head_fruit_hit(mobile);
    return rol_deaths_head_tree_hit(mobile);
  default:
    return FALSE;
  }
}
