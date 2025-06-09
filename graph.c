/**************************************************************************
 *  File: graph.c                                      Part of LuminariMUD *
 *  Usage: Various graph algorithms.                                       *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "act.h" /* for the do_say command */
#include "constants.h"
#include "graph.h"
#include "fight.h"
#include "spec_procs.h"
#include "mud_event.h"
#include "actions.h"
#include "wilderness.h"
#include "shop.h" /* shopkeepers hunting?! */
#include "evolutions.h"

/* local functions */
static int VALID_EDGE(room_rnum x, int y);
static void bfs_enqueue(room_rnum room, int dir);
static void bfs_dequeue(void);
static void bfs_clear_queue(void);
struct bfs_queue_struct
{
  room_rnum room;
  char dir;
  struct bfs_queue_struct *next;
};

/* had to rename queue_head because it already exists in dg_event.h */
static struct bfs_queue_struct *queue_head_2 = 0, *queue_tail = 0;

/* Utility macros */
#define MARK(room) (SET_BIT_AR(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define UNMARK(room) (REMOVE_BIT_AR(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define IS_MARKED(room) (ROOM_FLAGGED(room, ROOM_BFS_MARK))
#define TOROOM(x, y) (world[(x)].dir_option[(y)]->to_room)
#define IS_CLOSED(x, y) (EXIT_FLAGGED(world[(x)].dir_option[(y)], EX_CLOSED))

static int VALID_EDGE(room_rnum x, int y)
{
  if (world[x].dir_option[y] == NULL || TOROOM(x, y) == NOWHERE)
    return 0;
  if (CONFIG_TRACK_T_DOORS == FALSE && IS_CLOSED(x, y))
    return 0;
  if (ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK) || IS_MARKED(TOROOM(x, y)))
    return 0;

  return 1;
}

static void bfs_enqueue(room_rnum room, int dir)
{
  struct bfs_queue_struct *curr;

  CREATE(curr, struct bfs_queue_struct, 1);
  curr->room = room;
  curr->dir = dir;
  curr->next = 0;

  if (queue_tail)
  {
    queue_tail->next = curr;
    queue_tail = curr;
  }
  else
    queue_head_2 = queue_tail = curr;
}

static void bfs_dequeue(void)
{
  struct bfs_queue_struct *curr;

  curr = queue_head_2;

  if (!(queue_head_2 = queue_head_2->next))
    queue_tail = 0;
  free(curr);
}

static void bfs_clear_queue(void)
{
  while (queue_head_2)
    bfs_dequeue();
}

/* find_first_step: given a source room and a target room, find the first step
 * on the shortest path from the source to the target. Intended usage: in
 * mobile_activity, give a mob a dir to go if they're tracking another mob or a
 * PC.  Or, a 'track' skill for PCs. */
int find_first_step(room_rnum src, room_rnum target)
{
  int curr_dir;
  room_rnum curr_room;

  if (src == NOWHERE || target == NOWHERE || src > top_of_world || target > top_of_world)
  {
    log("SYSERR: Illegal value %d or %d passed to find_first_step. (%s)", src, target, __FILE__);
    return (BFS_ERROR);
  }

#if !defined(CAMPAIGN_DL)
  if (GET_ROOM_ZONE(src) != GET_ROOM_ZONE(target))
  {
    /* i turned off the log because at spots we are purposely trying to find a target that may not be in the same zone (forced) -zusuk */
    /* log("INFO: Attempt to path across zones, vnum %d (%d) to vnum %d (%d).", world[src].number, src, world[target].number, target); */
    return (BFS_NO_PATH);
  }
#endif

  if (src == target)
    return (BFS_ALREADY_THERE);

  /* clear marks first, some OLC systems will save the mark. */
  for (curr_room = 0; curr_room <= top_of_world; curr_room++)
    UNMARK(curr_room);

  MARK(src);

  /* first, enqueue the first steps, saving which direction we're going. */
  for (curr_dir = 0; curr_dir < DIR_COUNT; curr_dir++)
    if (VALID_EDGE(src, curr_dir))
    {
      MARK(TOROOM(src, curr_dir));
      bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
    }

  /* now, do the classic BFS. */
  while (queue_head_2)
  {
    if (queue_head_2->room == target)
    {
      curr_dir = queue_head_2->dir;
      bfs_clear_queue();
      return (curr_dir);
    }
    else
    {
      for (curr_dir = 0; curr_dir < DIR_COUNT; curr_dir++)
        if (VALID_EDGE(queue_head_2->room, curr_dir))
        {
          MARK(TOROOM(queue_head_2->room, curr_dir));
          bfs_enqueue(TOROOM(queue_head_2->room, curr_dir), queue_head_2->dir);
        }
      bfs_dequeue();
    }
  }

  return (BFS_NO_PATH);
}

/* Functions and Commands which use the above functions. */

/* our pimritive version of track, to be upgraded by Ornir at some point
   (that work can be found commented out in act.informative.c do_track) */
ACMD(do_track)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  char dirchar[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;
  int dir, track_dc = 0;
  int ch_in_wild = FALSE, vict_in_wild = FALSE, moves = 0;

  /* The character must have the track skill. */
  if (!HAS_FEAT(ch, FEAT_NATURAL_TRACKER) && !HAS_FEAT(ch, FEAT_TRACK) && !HAS_EVOLUTION(ch, EVOLUTION_SCENT))
  {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Whom are you trying to track?\r\n");
    if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    {
      snprintf(buf, sizeof(buf), " %s Who do you want me to track?\r\n", GET_NAME(ch->master));
      do_tell(ch, buf, 0, 0);
    }
    return;
  }

  /* The person can't see the victim. */
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
  {
    send_to_char(ch, "No one is around by that name.\r\n");
    if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    {
      snprintf(buf, sizeof(buf), " %s I can't find tracks for anyone named %s\r\n", GET_NAME(ch->master), GET_NAME(vict));
      do_tell(ch, buf, 0, 0);
    }
    return;
  }

  /* We can't track the victim. */
  if (AFF_FLAGGED(vict, AFF_NOTRACK) && GET_LEVEL(ch) < LVL_IMPL)
  {
    send_to_char(ch, "You sense they left no trail...\r\n");
    if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    {
      snprintf(buf, sizeof(buf), " %s I sense no trail to %s.\r\n", GET_NAME(ch->master), GET_NAME(vict));
      do_tell(ch, buf, 0, 0);
    }
    return;
  }

  if (IS_SET_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_FOG) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "The fog makes it impossible to attempt to track anything from here.");
    if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    {
      snprintf(buf, sizeof(buf), " %s The fog makes it impossible to attempt to track anything from here.\r\n", GET_NAME(ch->master));
      do_tell(ch, buf, 0, 0);
    }
    return;
  }

  moves = dice(5, 5);
  if (HAS_FEAT(ch, FEAT_SWIFT_TRACKER))
    moves = 0;

  if (GET_MOVE(ch) < moves)
  {
    send_to_char(ch, "You are too exhausted!");
    if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    {
      snprintf(buf, sizeof(buf), " %s I am too exhausted.\r\n", GET_NAME(ch->master));
      do_tell(ch, buf, 0, 0);
    }
    return;
  }

  /* skill check */
  if (IS_NPC(vict))
  {
    track_dc = GET_LEVEL(vict) + 10;
  }
  else
    track_dc = 10 + compute_ability(vict, ABILITY_SURVIVAL);

  if (HAS_EVOLUTION(ch, EVOLUTION_KEEN_SCENT))
    track_dc -= 5;

  /* skill check continue */
  if (GET_LEVEL(ch) >= LVL_IMPL)
    ;
  else if (!skill_check(ch, ABILITY_SURVIVAL, track_dc))
  {
    if (!HAS_FEAT(ch, FEAT_SWIFT_TRACKER))
      USE_MOVE_ACTION(ch);
    int tries = 10;
    /* Find a random direction. :) */
    do
    {
      dir = rand_number(0, DIR_COUNT - 1);
    } while (!CAN_GO(ch, dir) && --tries);
    send_to_char(ch, "You sense a trail %s from here!\r\n", dirs[dir]);
    if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    {
      snprintf(buf, sizeof(buf), " %s I sense a trail to %s %s of here.\r\n", GET_NAME(ch->master), GET_NAME(vict), dirs[dir]);
      do_tell(ch, buf, 0, 0);
    }
      
    return;
  }

  /* They passed the skill check. */

  ch_in_wild = IS_WILDERNESS_VNUM(GET_ROOM_VNUM(IN_ROOM(ch)));
  vict_in_wild = IS_WILDERNESS_VNUM(GET_ROOM_VNUM(IN_ROOM(vict)));

  /* handle wilderness */
  if (ch_in_wild && vict_in_wild)
  {
    int ch_x_location = X_LOC(ch);
    int ch_y_location = Y_LOC(ch);
    int vict_x_location = X_LOC(vict);
    int vict_y_location = Y_LOC(vict);

    if (vict_y_location == ch_y_location && vict_x_location == ch_x_location)
    {
      send_to_char(ch, "You are already in the same room!");
      return;
    }

    send_to_char(ch, "You sense a trail ");

    /* y corresponds to north/south (duh) */
    if (vict_y_location > ch_y_location) /* north! */
    {
      send_to_char(ch, "north");
      snprintf(dirchar, sizeof(dirchar), "north");
    }
    else if (vict_y_location < ch_y_location) /* south! */
    {
      send_to_char(ch, "south");
      snprintf(dirchar, sizeof(dirchar), "south");
    }
  
    /* x corresponds to east/west (duh) */
    if (vict_x_location > ch_x_location) /* east! */
    {
      send_to_char(ch, "east");
      snprintf(dirchar, sizeof(dirchar), "east");
    }

    else if (vict_x_location < ch_x_location) /* west! */
    {
      send_to_char(ch, "west");
      snprintf(dirchar, sizeof(dirchar), "west");
    }

    send_to_char(ch, " from here!\r\n");

    if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    {
      snprintf(buf, sizeof(buf), " %s I sense a trail to %s %s of here.\r\n", GET_NAME(ch->master), GET_NAME(vict), dirchar);
      do_tell(ch, buf, 0, 0);
    }
  }

  /* handle inside of a zone (stock) */
  else if (!ch_in_wild && !vict_in_wild)
  {
    dir = find_first_step(IN_ROOM(ch), IN_ROOM(vict));
    switch (dir)
    {
    case BFS_ERROR:
      send_to_char(ch, "Hmm.. something seems to be wrong.\r\n");
      break;
    case BFS_ALREADY_THERE:
      send_to_char(ch, "You're already in the same room!!\r\n");
      if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
      {
        snprintf(buf, sizeof(buf), " %s We're already in the same room as %s.\r\n", GET_NAME(ch->master), GET_NAME(vict));
        do_tell(ch, buf, 0, 0);
      }
      break;
    case BFS_NO_PATH:
      send_to_char(ch, "You can't sense a trail to %s from here.\r\n", HMHR(vict));
      if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
      {
        snprintf(buf, sizeof(buf), " %s I can't sense a trail to %s.\r\n", GET_NAME(ch->master), GET_NAME(vict));
        do_tell(ch, buf, 0, 0);
      }
      break;
    default: /* Success! */
      send_to_char(ch, "You sense a trail %s from here!\r\n", dirs[dir]);
      if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
      {
        snprintf(buf, sizeof(buf), " %s I sense a trail to %s %s of here.\r\n", GET_NAME(ch->master), GET_NAME(vict), dirs[dir]);
        do_tell(ch, buf, 0, 0);
      }
      break;
    }
  }

  /* one person in wild, one is not, we don't handle currently */
  else
  {
    send_to_char(ch, "The trail has gone cold.\r\n");
    if (IS_NPC(ch) && ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    {
      snprintf(buf, sizeof(buf), " %s The trail to %s has gone cold.\r\n", GET_NAME(ch->master), GET_NAME(vict));
      do_tell(ch, buf, 0, 0);
    }
  }
}

void hunt_victim(struct char_data *ch)
{
  int dir;
  byte found;
  bool mem_found = FALSE;
  struct char_data *tmp, *vict;
  memory_rec *names = NULL;
  int ch_in_wild = FALSE, vict_in_wild = FALSE;

  if (!ch || FIGHTING(ch))
    return;

  if (MOB_FLAGGED(ch, MOB_NOKILL))
  {
    send_to_char(ch, "You are a protected mob, it doesn't make sense for you to hunt!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM))
  {
    send_to_char(ch, "You can't hunt anything while you're under someone else's control.\r\n");
    return;
  }

  /* if ch has memory, try finding a new hunting victim */
  if (!HUNTING(ch))
  {
    if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch))
    {
      mem_found = FALSE;
      for (mem_found = FALSE, tmp = character_list; tmp && !mem_found;
           tmp = tmp->next)
      {
        if (IS_NPC(tmp) || !CAN_SEE(ch, tmp) || PRF_FLAGGED(tmp, PRF_NOHASSLE))
          continue;

        for (names = MEMORY(ch); names && !mem_found; names = names->next)
        {
          if (names->id != GET_IDNUM(tmp))
            continue;

          mem_found = TRUE;
          HUNTING(ch) = tmp;
          act("'bwargh!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
          break;
        }
      }
    }
    else
      return;
  }

  /* make sure the char still exists */
  for (found = FALSE, tmp = character_list; tmp && !found; tmp = tmp->next)
    if (HUNTING(ch) == tmp)
      found = TRUE;

  if (!found)
  {
    char actbuf[MAX_INPUT_LENGTH] = "???";

    do_say(ch, actbuf, 0, 0);
    HUNTING(ch) = NULL;
    return;
  }

  /* easier reference */
  vict = HUNTING(ch);

  if (!ok_damage_shopkeeper(vict, ch))
  {
    send_to_char(ch, "You are a shopkeeper (that can't be damaged), it doesn't make sense for you to hunt!\r\n");
    return;
  }

  if (ch->master && vict == ch->master && AFF_FLAGGED(ch, AFF_CHARM))
  {
    HUNTING(ch) = NULL;
    return;
  }

  /*
  if (!is_mission_mob(vict, ch))
  {
    send_to_char(ch, "You are a mission mob, it doesn't make sense for you to hunt!\r\n");
    return;
  }
  */

  ch_in_wild = IS_WILDERNESS_VNUM(GET_ROOM_VNUM(IN_ROOM(ch)));
  vict_in_wild = IS_WILDERNESS_VNUM(GET_ROOM_VNUM(IN_ROOM(vict)));

  /* handle wilderness */
  if (ch_in_wild && vict_in_wild)
  {
    int ch_x_location = X_LOC(ch);
    int ch_y_location = Y_LOC(ch);
    int vict_x_location = X_LOC(vict);
    int vict_y_location = Y_LOC(vict);

    if (vict_y_location == ch_y_location && vict_x_location == ch_x_location)
    {
      /* found victim! */
      act("'!!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return;
    }

    /* y corresponds to north/south (duh) */
    if (vict_y_location > ch_y_location) /* north! */
      perform_move(ch, NORTH, 1);
    else if (vict_y_location < ch_y_location) /* south! */
      perform_move(ch, SOUTH, 1);
    /* x corresponds to east/west (duh) */
    /* note we can make two moves */
    if (vict_x_location > ch_x_location) /* east! */
      perform_move(ch, EAST, 1);
    else if (vict_x_location < ch_x_location) /* west! */
      perform_move(ch, WEST, 1);

    if (IN_ROOM(ch) == IN_ROOM(vict))
    {
      /* found victim! */
      act("'!!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return;
    }
  }
  /* handle inside of a zone (stock) */
  else if (!ch_in_wild && !vict_in_wild)
  {
    if ((dir = find_first_step(IN_ROOM(ch), IN_ROOM(vict))) < 0)
    {
      // char buf[MAX_INPUT_LENGTH] = {'\0'};

      // snprintf(buf, sizeof(buf), "!?!");
      // do_say(ch, buf, 0, 0);
      HUNTING(ch) = NULL;
    }
    else
    {
      perform_move(ch, dir, 1);
      if (IN_ROOM(ch) == IN_ROOM(vict) && !IS_PET(ch) && !FIGHTING(ch))
      {
        act("'!!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    }
  }
  else
  {
    send_to_char(ch, "The trail has gone cold.\r\n");
    /*todo: handle transition between zones/wilderness*/
  }
}

/* this function will cause ch to attempt to find its loadroom */
void hunt_loadroom(struct char_data *ch)
{
  int dir;

  if (!ch || FIGHTING(ch) || GET_POS(ch) != POS_STANDING)
    return;

  if (GET_MOB_LOADROOM(ch) == NOWHERE)
    return;

  if (GET_ROOM_VNUM(ch->in_room) == GET_ROOM_VNUM(GET_MOB_LOADROOM(ch)))
    return;

  if ((dir = find_first_step(ch->in_room, GET_MOB_LOADROOM(ch))) < 0)
    return;

  perform_move(ch, dir, 1);
}
