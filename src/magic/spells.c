/**************************************************************************
 *  File: spells.c                                     Part of LuminariMUD *
 *  Usage: Implementation of "manual spells."                              *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "string.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "character/perks.h"
#include "interpreter.h"
#include "dgscript/dg_scripts.h"
#include "act.h"
#include "combat/fight.h"
#include "mud_event.h"
#include "obj/house.h" /* for house_can_enter() */
#include "screen.h"    /* for QNRM, etc */
#include "craft/craft.h"
#include "mudlim.h"
#include "obj/item.h"
#include "obj/treasure.h"
#include "domains_schools.h"
#include "olc/oasis.h"
#include "olc/genzon.h" /* for real_zone_by_thing */
#include "psionics.h"
#include "combat/assign_wpn_armor.h"
#include "actions.h" /* for use_ACTION() */
#include "vessels/transport.h"
#include "character/evolutions.h"
#include "character/feats.h"
#include "vessels/routing.h"
#include "movement/movement_validation.h"
#include "character_periodic.h"
#include "domain_event_world.h"

/************************************************************/
/*  Functions, Events, etc needed to perform manual spells  */
/************************************************************/

bool save_char_pets(struct char_data *ch);

/* Reference
#define SPELL_WALL_OF_FORCE             147
#define SPELL_WALL_OF_FIRE              282
#define SPELL_WALL_OF_THORNS            283
#define SPELL_WALL_OF_FOG               92
#define SPELL_PRISMATIC_SPHERE          211
#define PSIONIC_WALL_OF_ECTOPLASM 1562

| stops movement? | spellnum | long name | short name | keywords | duration |
   duration = 0 is default: 1 + level / 10
 */

bool is_wall_spell(int spellnum)
{
  switch (spellnum)
  {
  case SPELL_WALL_OF_FORCE:
  case SPELL_WALL_OF_FIRE:
  case SPELL_WALL_OF_THORNS:
  case SPELL_WALL_OF_FOG:
  case SPELL_PRISMATIC_SPHERE:
  case WARLOCK_WALL_OF_PERILOUS_FLAME:
  case PSIONIC_WALL_OF_ECTOPLASM:
    break;
  default:
    /* this isn't a wall spell! */
    return FALSE;
  }
  return TRUE;
}

struct wall_information wallinfo[NUM_WALL_TYPES] = {
    /* WALL_TYPE_FORCE 0 */
    {TRUE, SPELL_WALL_OF_FORCE, "\tRA wall of force stands towards the %s.\tn",
     "\tRa wall of force\tn", "wall force", 1},
    /* WALL_TYPE_FIRE 1 */
    {FALSE, SPELL_WALL_OF_FIRE, "\trA wall of f\tRi\trre stands towards the %s.\tn",
     "\tra wall of fire\tn", "wall fire", 0},
    /* WALL_TYPE_THORNS 2 */
    {FALSE, SPELL_WALL_OF_THORNS, "\tGA wall of thorns stands towards the %s.\tn",
     "\tGa wall of thorns\tn", "wall thorns", 0},
    /* WALL_TYPE_FOG 3 */
    {FALSE, SPELL_WALL_OF_FOG, "\tCA foggy cloud forms a wall towards the %s.\tn",
     "\tCa wall of fog\tn", "wall fog", 0},
    /* WALL_TYPE_PRISM 4 */
    {FALSE, SPELL_PRISMATIC_SPHERE, "\tnA \tRp\tYr\tBi\tMs\tWm\tn forms a wall towards the %s.\tn",
     "\tna wall of \tRp\tYr\tBi\tMs\tWm\tn", "wall prism", 0},
    /* WALL_TYPE_ECTOPLASM 5 */
    {TRUE, PSIONIC_WALL_OF_ECTOPLASM, "\tnA sphere of ectoplasm forms a wall towards the %s.\tn",
     "\tna wall of ectoplasm", "wall ectoplasm", 10},
    /* WALL_TYPE_FIRE 6 */
    {FALSE, WARLOCK_WALL_OF_PERILOUS_FLAME,
     "\trA wall of perilous f\tRi\trre stands towards the %s.\tn", "\tra wall of perilous fire\tn",
     "wall perilous fire", 0},
};

static bool wall_type_valid(const struct obj_data *wall)
{
  int type;

  if (wall == NULL || GET_OBJ_TYPE(wall) != ITEM_WALL)
    return false;
  type = GET_OBJ_VAL(wall, WALL_TYPE);
  return type >= 0 && type < NUM_WALL_TYPES;
}

static bool wall_affects_character(struct obj_data *wall, struct char_data *victim,
                                   struct char_data **creator)
{
  int spellnum;

  *creator = find_char(GET_OBJ_VAL(wall, WALL_IDNUM));
  if (*creator == NULL)
    return true;
  spellnum = wallinfo[GET_OBJ_VAL(wall, WALL_TYPE)].spell_num;
  return aoeOK(*creator, victim, spellnum);
}

static struct obj_data *blocking_wall_in_room(struct char_data *victim, room_rnum room, int dir)
{
  struct obj_data *wall;
  struct char_data *creator;

  if (victim == NULL || world == NULL || room == NOWHERE || room > top_of_world || dir < 0 ||
      dir >= NUM_OF_DIRS)
    return NULL;
  for (wall = world[room].contents; wall != NULL; wall = wall->next_content)
    if (wall_type_valid(wall) && GET_OBJ_VAL(wall, WALL_DIR) == dir &&
        wallinfo[GET_OBJ_VAL(wall, WALL_TYPE)].stops_movement &&
        wall_affects_character(wall, victim, &creator))
      return wall;
  return NULL;
}

/* Passability is a pre-move decision. Inspect both authored sides of the edge
 * before changing rooms so a destination wall never requires a rollback. */
bool wall_blocks_movement(struct char_data *victim, room_rnum from_room, room_rnum to_room, int dir)
{
  struct obj_data *wall;

  wall = blocking_wall_in_room(victim, from_room, dir);
  if (wall == NULL)
    wall = blocking_wall_in_room(victim, to_room, rev_dir[dir]);
  if (wall == NULL)
    return false;
  act("You bump into $p.", FALSE, victim, wall, NULL, TO_CHAR);
  act("$n bumps into $p.", FALSE, victim, wall, NULL, TO_ROOM);
  return true;
}

#ifdef LUMINARI_CUTEST
static wall_crossing_test_callback wall_test_callback;
static void *wall_test_context;

void wall_crossing_set_test_callback(wall_crossing_test_callback callback, void *context)
{
  wall_test_callback = callback;
  wall_test_context = context;
}
#endif

static bool apply_wall_crossings_in_room(struct char_data *victim, room_rnum room, int dir)
{
  struct domain_entity_handle victim_identity;
  struct domain_entity_handle wall_identity;
  struct domain_entity_handle next_identity;
  struct obj_data *wall;
  struct obj_data *next;
  struct char_data *creator;
  int level;
  int spellnum;

  if (victim == NULL || world == NULL || room == NOWHERE || room > top_of_world || dir < 0 ||
      dir >= NUM_OF_DIRS)
    return false;
  victim_identity = domain_event_character_handle(victim);
  for (wall = world[room].contents; wall != NULL; wall = next)
  {
    next = wall->next_content;
    next_identity =
        next != NULL ? domain_event_object_handle(next) : (struct domain_entity_handle){0};
    if (!wall_type_valid(wall) || GET_OBJ_VAL(wall, WALL_DIR) != dir ||
        !wall_affects_character(wall, victim, &creator))
      continue;
    wall_identity = domain_event_object_handle(wall);
    if (!domain_entity_handle_is_valid(wall_identity))
      continue;
#ifdef LUMINARI_CUTEST
    if (wall_test_callback != NULL)
    {
      if (!wall_test_callback(wall_identity, victim, wall_test_context))
        return false;
    }
    else
#endif
    {
      level = creator != NULL ? GET_LEVEL(creator) : GET_OBJ_VAL(wall, WALL_LEVEL);
      spellnum = wallinfo[GET_OBJ_VAL(wall, WALL_TYPE)].spell_num;
      if ((CONFIG_PK_ALLOWED || (IS_NPC(victim) && !IS_PET(victim))) &&
          mag_damage(level + dice(2, 6), creator != NULL ? creator : victim, victim, NULL, spellnum,
                     0, SAVING_FORT, CAST_WALL) < 0)
        return false;
    }
    victim = domain_event_world_resolve_character(victim_identity);
    if (victim == NULL)
      return false;
    next = domain_entity_handle_is_valid(next_identity)
               ? domain_event_world_resolve_object(next_identity)
               : NULL;
    if (next != NULL && next->in_room != room)
      next = NULL;
  }
  return true;
}

/* Damaging crossings are post-commit notifications. A wall object is one
 * generation-safe source, and one CharacterMoved fact visits it at most once. */
void apply_wall_crossing(struct char_data *victim, room_rnum from_room, room_rnum to_room, int dir)
{
  struct domain_entity_handle victim_identity;

  if (victim == NULL || from_room == to_room || dir < 0 || dir >= NUM_OF_DIRS)
    return;
  victim_identity = domain_event_character_handle(victim);
  if (!apply_wall_crossings_in_room(victim, from_room, dir))
    return;
  victim = domain_event_world_resolve_character(victim_identity);
  if (victim != NULL)
    (void)apply_wall_crossings_in_room(victim, to_room, rev_dir[dir]);
}

/* this function will load the wall object, assign the appropriate values
   to it and do a little basic dummy checking */
void create_wall(struct char_data *ch, int room, int dir, int type, int level)
{
  struct obj_data *wall = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  for (wall = world[room].contents; wall; wall = wall->next_content)
  {
    if (GET_OBJ_TYPE(wall) == ITEM_WALL && GET_OBJ_VAL(wall, WALL_DIR) == dir)
    {
      send_to_char(ch, "There is already a wall in that direction.\r\n");
      return;
    }
  }

  if (!CAN_GO(ch, dir))
  {
    send_to_char(ch, "There is no open exit in that direction where you can put a wall in.\r\n");
    return;
  }

  wall = read_object_reason(WALL_ITEM, VIRTUAL, PERF_ENTITY_SPELL_SUMMON);

  if (!wall)
  { /* make sure we have the object */
    send_to_char(ch, "Please Report Wall Bug To Staff\r\n");
    return;
  }

  GET_OBJ_TYPE(wall) = ITEM_WALL;                             /* set type */
  wall->name = strdup(wallinfo[type].keyword);                /* dump the keywords */
  wall->short_description = strdup(wallinfo[type].shortname); /* short descrip */

  /* create an item description */
  snprintf(buf, sizeof(buf), wallinfo[type].longname, dirs[dir]);
  wall->description = strdup(CAP(buf));

  /* either use a default time of 1 + level/10 or set duration */
  switch (type)
  {
  case WALL_TYPE_FORCE:
    GET_OBJ_TIMER(wall) = level;
    break;

  default:
    if (wallinfo[type].duration == 0)
      GET_OBJ_TIMER(wall) = 1 + level / 10;
    else
      GET_OBJ_TIMER(wall) = wallinfo[type].duration;

    break;
  }

  /* make sure the wall fades eventually */
  if (!OBJ_FLAGGED(wall, ITEM_DECAY))
    TOGGLE_BIT_AR(GET_OBJ_EXTRA(wall), ITEM_DECAY);

  /* set the correct type, direction blocking, level and identifier */
  GET_OBJ_VAL(wall, WALL_TYPE) = type;
  GET_OBJ_VAL(wall, WALL_DIR) = dir;
  GET_OBJ_VAL(wall, WALL_LEVEL) = level; /* in case we can't find wall creator */
  GET_OBJ_VAL(wall, WALL_IDNUM) = GET_IDNUM(ch);

  /* all done!  drop the object in the room and let it wreak havoc! */
  obj_to_room(wall, room);
  snprintf(buf, sizeof(buf), "%s appears to the %s.\r\n", wallinfo[type].shortname, dirs[dir]);
  send_to_room(room, "%s", buf);
}

/* this function takes a real number for a room and returns:
   FALSE - mortals shouldn't be able to teleport to this destination
   TRUE - mortals CAN teleport to this destination
 * accepts NULL ch data
 * dim_lock means this test is checking for the dimensional lock affection
 */
int valid_mortal_tele_dest(struct char_data *ch, room_rnum dest, bool dim_lock)
{
  if (dest == NOWHERE)
    return FALSE;

  if (ch && !room_level_allows_entry(ch, dest, false))
    return FALSE;

  /* if dim_lock is TRUE, we are checking for dim_lock, requires ch data */
  if (ch && IS_AFFECTED(ch, AFF_DIM_LOCK) && dim_lock)
    return FALSE;

  /* this function needs a vnum, not rnum */
  if (ch && !House_can_enter(ch, GET_ROOM_VNUM(dest)))
    return FALSE;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(dest), ZONE_NOTELEPORT))
    return FALSE;

  if (ROOM_FLAGGED(dest, ROOM_PRIVATE))
    return FALSE;

  if (ROOM_FLAGGED(dest, ROOM_DEATH))
    return FALSE;

  if (ROOM_FLAGGED(dest, ROOM_STAFFROOM))
    return FALSE;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(dest), ZONE_CLOSED))
    return FALSE;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(dest), ZONE_NOASTRAL))
    return FALSE;

  if (ROOM_FLAGGED(dest, ROOM_NOTELEPORT))
    return FALSE;

  // passed all tests!
  return TRUE;
}

/* Used by the locate object spell to check the alias list on objects */
int isname_obj(char *search, char *list)
{
  char *found_in_list; /* But could be something like 'ring' in 'shimmering.' */
  char searchname[128];
  char namelist[MAX_STRING_LENGTH] = {'\0'};
  int found_pos = -1;
  int found_name = 0; /* found the name we're looking for */
  int match = 1;
  int i;

  /* Force to lowercase for string comparisons */
  snprintf(searchname, sizeof(searchname), "%s", search);
  for (i = 0; searchname[i]; i++)
    searchname[i] = LOWER(searchname[i]);

  snprintf(namelist, sizeof(namelist), "%s", list);
  for (i = 0; namelist[i]; i++)
    namelist[i] = LOWER(namelist[i]);

  /* see if searchname exists any place within namelist */
  found_in_list = strstr(namelist, searchname);
  if (!found_in_list)
  {
    return 0;
  }

  /* Found the name in the list, now see if it's a valid hit. The following
   * avoids substrings (like ring in shimmering) is it at beginning of
   * namelist? */
  for (i = 0; searchname[i]; i++)
    if (searchname[i] != namelist[i])
      match = 0;

  if (match) /* It was found at the start of the namelist string. */
    found_name = 1;
  else
  { /* It is embedded inside namelist. Is it preceded by a space? */
    found_pos = found_in_list - namelist;
    if (namelist[found_pos - 1] == ' ')
      found_name = 1;
  }

  if (found_name)
    return 1;
  else
    return 0;
}

/* the main engine of charm spell, and similar */
void effect_charm(struct char_data *ch, struct char_data *victim, int spellnum, int casttype,
                  int level)
{
  struct affected_type af;
  int bonus = 0;

  /* resistance bonuses, etc */
  if (!IS_NPC(victim) && (GET_RACE(victim) == RACE_ELF || // elven enchantment resistance
                          GET_RACE(victim) == RACE_H_ELF))
    /* added check for IS_NPC because RACE_TYPE_HUMAN == RACE_ELF and
     * RACE_TYPE_ABERRATION == RACE_H_ELF */
    bonus += 2;
  if (!IS_NPC(victim) && HAS_FEAT(victim, FEAT_STILL_MIND))
    bonus += 2;

  if (victim == ch)
  {
    send_to_char(ch, "You cannot charm or dominate yourself.\r\n");
    return;
  }
  else if (char_has_object_flag(victim, ITEM_ROL_NO_CHARM))
  {
    send_to_char(ch, "Your victim is protected from this enchantment by carried equipment!\r\n");
    if (IS_NPC(victim))
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
  {
    send_to_char(ch, "Your victim doesn't seem vulnerable to this "
                     "enchantments!\r\n");
    if (IS_NPC(victim))
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }
  else if (IS_AFFECTED(victim, AFF_MIND_BLANK))
  {
    send_to_char(ch, "Your victim is protected from this "
                     "enchantment!\r\n");
    if (IS_NPC(victim))
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }
  else if (spellnum != SPELL_COMMAND_UNDEAD && is_immune_charm(ch, victim, FALSE))
  {
    send_to_char(ch, "Your victim is protected from this "
                     "enchantment!\r\n");
    if (IS_NPC(victim))
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }
  else if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "You can't have any followers of your own!\r\n");

  else if (AFF_FLAGGED(victim, AFF_CHARM))
    send_to_char(ch, "Your victim is already charmed.\r\n");

  else if (spellnum == SPELL_CHARM &&
           (CASTER_LEVEL(ch) < GET_LEVEL(victim) || GET_LEVEL(victim) >= 8))
    send_to_char(ch, "Your victim is too powerful.\r\n");

  // else if (check_npc_followers(ch, NPC_MODE_SPARE, 0) <= 0)
  else if (IS_NPC(victim) && !can_add_follower(ch, GET_MOB_VNUM(victim)))
    send_to_char(ch, "You can not manage more followers!\r\n");

  else if ((spellnum == SPELL_DOMINATE_PERSON || spellnum == SPELL_MASS_DOMINATION ||
            spellnum == WARLOCK_CHARM) &&
           CASTER_LEVEL(ch) < GET_LEVEL(victim))
    send_to_char(ch, "Your victim is too powerful.\r\n");

  else if (spellnum == ABILITY_VAMPIRIC_DOMINATION && level < GET_LEVEL(victim))
    send_to_char(ch, "Your victim is too powerful.\r\n");

  /* player charming another player - no legal reason for this */
  else if (!CONFIG_PK_ALLOWED && !IS_NPC(victim))
    send_to_char(ch, "You fail - shouldn't be doing it anyway.\r\n");

  else if (circle_follow(victim, ch))
    send_to_char(ch, "Sorry, following in circles is not allowed.\r\n");

  else if (mag_resistance(ch, victim, 0))
  {
    send_to_char(ch, "You failed to penetrate the spell resistance!");
    if (IS_NPC(victim))
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }
  else if (savingthrow(ch, victim, SAVING_WILL, bonus, casttype, level, ENCHANTMENT))
  {
    send_to_char(ch, "Your victim resists!\r\n");
    if (IS_NPC(victim))
      hit(victim, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }
  else
  {
    /* slippery mind gives a second save */
    if (!IS_NPC(victim) && HAS_FEAT(victim, FEAT_SLIPPERY_MIND) && spell_info[spellnum].violent)
    {
      send_to_char(victim, "\tW*Slippery Mind*\tn  ");
      if (savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, ENCHANTMENT))
      {
        return;
      }
    }

    if (victim->master)
      stop_follower(victim);

    if (FIGHTING(ch) == victim)
      stop_fighting(ch);

    stop_fighting(victim);
    add_follower(victim, ch);

    new_affect(&af);
    af.spell = spellnum;
    af.duration = 100;
    if (GET_CHA_BONUS(ch))
      af.duration += GET_CHA_BONUS(ch) * 4;
    if (IS_NPC(victim))
      af.duration *= 10;
    SET_BIT_AR(af.bitvector, AFF_CHARM);
    affect_to_char(victim, &af);

    act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
    save_char_pets(ch);
    //    if (IS_NPC(victim))
    //      REMOVE_BIT_AR(MOB_FLAGS(victim), MOB_SPEC);
  }
  // should never get here
}

/* for dispel magic and greater dispelling */

/* a hack job so far, gets rid of the first x affections */

/* TODO:  add strength/etc to affection struct, that'd help a lot especially
   here */
static struct affected_type *first_dispellable_affect(struct char_data *ch)
{
  struct affected_type *af;

  if (ch == NULL)
    return NULL;

  for (af = ch->affected; af != NULL; af = af->next)
    if (!rol_elemental_embodiment_affect_is_transient(af->spell))
      return af;

  return NULL;
}

void perform_dispel(struct char_data *ch, struct char_data *vict, struct obj_data *obj,
                    int spellnum)
{
  struct affected_type *af;
  int i = 0, attempt = 0, challenge = 0, num_dispels = 0, msg = FALSE;
  bool wildshape = false;

  // no target == room
  if (!vict && !obj)
  {
    if (IS_SET_AR(ROOM_FLAGS(IN_ROOM(ch)), (ROOM_FOG)))
    {
      // if (SECT(ch->in_room) != SECT_CLOUDS && SECT(ch->in_room) != SECT_SHADOWPLANE) {
      REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), (ROOM_FOG));
      send_to_room(IN_ROOM(ch), "\tWThe fog dissipates into thin air!\tn\r\n");
      //} else {
      // send_to_room("Your magic is useless against these clouds!\r\n", ch->in_room);
      //}
    }
    return;
  }

  if (obj)
  {
    attempt = d20(ch) + CASTER_LEVEL(ch);
    challenge = d20(vict) + GET_OBJ_LEVEL(obj);

    if (GET_OBJ_TYPE(obj) == ITEM_WALL)
    {
      if (attempt >= challenge)
      {
        act("You dispel $p, which fades away.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n dispels $p, which fades away.", FALSE, ch, obj, 0, TO_ROOM);
        extract_obj(obj);
      }
      else
      {
        act("You fail to dispel $p!", FALSE, ch, obj, NULL, TO_CHAR);
        act("$n fails to dispel $p!", FALSE, ch, obj, NULL, TO_ROOM);
      }
    }
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "You dispel all your own magic!\r\n");
    act("$n dispels all $s magic!", FALSE, ch, 0, 0, TO_ROOM);
    while ((af = first_dispellable_affect(ch)) != NULL)
    {
      if (get_wearoff(af->spell))
        send_to_char(ch, "%s\r\n", get_wearoff(af->spell));
      affect_remove(ch, af);
    }
    if (AFF_FLAGGED(ch, AFF_WILD_SHAPE))
      wildshape = true;
    for (i = 0; i < AF_ARRAY_MAX; i++)
      AFF_FLAGS(ch)[i] = 0;
    if (wildshape)
      SET_BIT_AR(AFF_FLAGS(ch), AFF_WILD_SHAPE);
    if (ch->affected != NULL)
      affect_total(ch);
    return;
  }
  else
  {
    attempt = d20(ch) + CASTER_LEVEL(ch);
    challenge = d20(vict) + CASTER_LEVEL(vict);

    if (spellnum == SPELL_GREATER_DISPELLING || spellnum == WARLOCK_VORACIOUS_DISPELLING ||
        spellnum == WARLOCK_DEVOUR_MAGIC)
    {
      num_dispels = dice(2, 2);
      for (i = 0; i < num_dispels; i++)
      {
        if (attempt >= challenge)
        { // successful
          if ((af = first_dispellable_affect(vict)) != NULL)
          {
            if (af->spell == WARLOCK_RETRIBUTIVE_INVISIBILITY)
            {
              // this spell explodes.
              mag_areas(GET_WARLOCK_LEVEL(vict), vict, NULL, WARLOCK_RETRIBUTIVE_INVISIBILITY, 0,
                        SAVING_FORT, CAST_INNATE);
            }
            msg = TRUE;
            affect_remove(vict, af);
            if (spellnum == WARLOCK_VORACIOUS_DISPELLING)
            {
              damage(ch, vict, CASTER_LEVEL(ch) / 2, WARLOCK_VORACIOUS_DISPELLING, DAM_FORCE, 0);
            }
            else if (spellnum == WARLOCK_DEVOUR_MAGIC)
            {
              mag_affects(GET_WARLOCK_LEVEL(ch), ch, ch, NULL, WARLOCK_DEVOUR_MAGIC, -1,
                          CAST_INNATE, 0);
            }
          }
        }
        attempt = d20(ch) + CASTER_LEVEL(ch);
        challenge = d20(vict) + CASTER_LEVEL(vict);
      }
      if (msg)
      {
        send_to_char(ch, "You successfully dispel some magic!\r\n");
        act("$n dispels some of $N's magic!", FALSE, ch, 0, vict, TO_ROOM);
      }
      else
      {
        send_to_char(ch, "You fail your dispel magic attempt!\r\n");
        act("$n fails to dispel some of $N's magic!", FALSE, ch, 0, vict, TO_ROOM);
      }
      return;
    }

    if (spellnum == SPELL_DISPEL_MAGIC)
    {
      if (attempt >= challenge)
      { // successful
        send_to_char(ch, "You successfuly dispel some magic!\r\n");
        act("$n dispels some of $N's magic!", FALSE, ch, 0, vict, TO_ROOM);
        if ((af = first_dispellable_affect(vict)) != NULL)
          affect_remove(vict, af);
      }
      else
      { // failed
        send_to_char(ch, "You fail your dispel magic attempt!\r\n");
        act("$n fails to dispel some of $N's magic!", FALSE, ch, 0, vict, TO_ROOM);
      }
    }
  }
}

MUD_EVENT_CALLBACK(event_ice_storm)
{
  struct char_data *ch;
  struct mud_event_data *pMudEvent;

  if (event_obj == NULL)
    return 0;

  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  if (ch == NULL)
    return 0;

  call_magic(ch, NULL, NULL, SPELL_ICE_STORM, 0, CASTER_LEVEL(ch), CAST_SPELL);
  return 0;
}

MUD_EVENT_CALLBACK(event_chain_lightning)
{
  struct char_data *ch;
  struct mud_event_data *pMudEvent;

  if (event_obj == NULL)
    return 0;

  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  if (ch == NULL)
    return 0;

  call_magic(ch, NULL, NULL, SPELL_CHAIN_LIGHTNING, 0, CASTER_LEVEL(ch), CAST_SPELL);
  return 0;
}

/* The "return" of the event function is the time until the event is called
 * again. If we return 0, then the event is freed and removed from the list, but
 * any other numerical response will be the delay until the next call */
MUD_EVENT_CALLBACK(event_acid_arrow)
{
  struct char_data *ch, *victim = NULL;
  struct mud_event_data *pMudEvent;
  int casttype = CAST_SPELL;
  int level = 0;

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;

  ch = (struct char_data *)pMudEvent->pStruct;

  if (ch && FIGHTING(ch)) // assign victim, if none escape
    victim = FIGHTING(ch);
  else
    return 0;

  if (ch == NULL || victim == NULL)
    return 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return 0;
  }

  send_to_char(ch, "You send out an arrow of acid towards your opponent!\r\n");
  act("$n sends out an arrow of acid!", FALSE, ch, 0, 0, TO_ROOM);

  if (mag_resistance(ch, victim, 0))
    return 0;

  /* how about wands and everything else?? */
  level = CASTER_LEVEL(ch);

  if (level < 1)
    level = 15; /* so lame */

  if (savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, EVOCATION))
    damage(ch, victim, (dice(3, 6) / 2), SPELL_ACID_ARROW, DAM_ACID, FALSE);
  else
    damage(ch, victim, dice(3, 6), SPELL_ACID_ARROW, DAM_ACID, FALSE);

  update_pos(victim);

  return 0;
}


/* The "return" of the event function is the time until the event is called
 * again. If we return 0, then the event is freed and removed from the list, but
 * any other numerical response will be the delay until the next call */
MUD_EVENT_CALLBACK(event_aqueous_orb)
{
  struct char_data *ch, *victim = NULL;
  struct mud_event_data *pMudEvent;
  int casttype = CAST_SPELL;
  int level = 0;
  bool is_fire = false;

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;

  ch = (struct char_data *)pMudEvent->pStruct;

  if (ch && FIGHTING(ch)) // assign victim, if none escape
    victim = FIGHTING(ch);
  else
    return 0;

  if (ch == NULL || victim == NULL)
    return 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return 0;
  }

  if (mag_resistance(ch, victim, 0))
    return 0;

  if (affected_by_spell(victim, SPELL_FIRE_SHIELD) || AFF_FLAGGED(victim, AFF_FSHIELD))
  {
    act("The aqueous orb quenches your fire shield.", FALSE, victim, 0, 0, TO_CHAR);
    act("The aqueous orb quenches $n's fire shield.", FALSE, victim, 0, 0, TO_ROOM);
    affect_from_char(victim, SPELL_FIRE_SHIELD);
    REMOVE_BIT_AR(AFF_FLAGS(victim), AFF_FSHIELD);
  }

  if (affected_by_spell(victim, SPELL_CONTINUAL_FLAME))
  {
    act("The aqueous orb quenches your continual flame.", FALSE, victim, 0, 0, TO_CHAR);
    act("The aqueous orb quenches $n's continual flame.", FALSE, victim, 0, 0, TO_ROOM);
    affect_from_char(victim, SPELL_CONTINUAL_FLAME);
  }

  if (affected_by_spell(victim, SPELL_SUN_METAL))
  {
    act("The aqueous orb quenches your sun metal enhancement.", FALSE, victim, 0, 0, TO_CHAR);
    act("The aqueous orb quenches $n's sun metal enhancement.", FALSE, victim, 0, 0, TO_ROOM);
    affect_from_char(victim, SPELL_SUN_METAL);
  }

  if (affected_by_spell(victim, SPELL_FIRE_OF_ENTANGLEMENT))
  {
    act("The aqueous orb quenches your fires of entanglement.", FALSE, victim, 0, 0, TO_CHAR);
    act("The aqueous orb quenches $n's fires of entanglement.", FALSE, victim, 0, 0, TO_ROOM);
    affect_from_char(victim, SPELL_FIRE_OF_ENTANGLEMENT);
  }

  if (affected_by_spell(victim, BOMB_AFFECT_FIRE_BRAND))
  {
    act("The aqueous orb quenches your fire brand bomb effect.", FALSE, victim, 0, 0, TO_CHAR);
    act("The aqueous orb quenches $n's fire brand bomb effect.", FALSE, victim, 0, 0, TO_ROOM);
    affect_from_char(victim, BOMB_AFFECT_FIRE_BRAND);
  }

  if (INCENDIARY(victim) > 0)
  {
    act("The aqueous orb quenches your incendiary cloud.", FALSE, victim, 0, 0, TO_CHAR);
    act("The aqueous orb quenches $n's incendiary cloud.", FALSE, victim, 0, 0, TO_ROOM);
    INCENDIARY(victim) = 0;
  }

  if (GET_SIZE(victim) > SIZE_LARGE)
  {
    act("The aqueous orb rolls through you, leaving you unaffected.", FALSE, victim, 0, 0, TO_CHAR);
    act("The aqueous orb rolls through $n, leaving $m unaffected.", FALSE, victim, 0, 0, TO_ROOM);
    return 0;
  }

  if (!IS_LIVING(victim))
  {
    act("Without the need to breathe, being engulfed in the aqueous orb does not harm you at all.",
        FALSE, victim, 0, 0, TO_CHAR);
    act("Without the need to breathe, being engulfed in the aqueous orb does not harm $n at all.",
        FALSE, victim, 0, 0, TO_ROOM);
    return 0;
  }

  if (AFF_FLAGGED(victim, AFF_WATER_BREATH))
  {
    act("With the ability to breathe water, being engulfed in the aqueous orb does not harm you at "
        "all.",
        FALSE, victim, 0, 0, TO_CHAR);
    act("With the ability to breathe water, being engulfed in the aqueous orb does not harm $n at "
        "all.",
        FALSE, victim, 0, 0, TO_ROOM);
    return 0;
  }

  is_fire =
      (GET_SUBRACE(victim, 0) == SUBRACE_FIRE || GET_SUBRACE(victim, 1) == SUBRACE_FIRE ||
       GET_SUBRACE(victim, 2) == SUBRACE_FIRE || GET_RACE(victim) == RACE_SMALL_FIRE_ELEMENTAL ||
       GET_RACE(victim) == RACE_MEDIUM_FIRE_ELEMENTAL ||
       GET_RACE(victim) == RACE_LARGE_FIRE_ELEMENTAL ||
       GET_RACE(victim) == RACE_HUGE_FIRE_ELEMENTAL ||
       GET_RACE(victim) == RACE_GARGANTUAN_FIRE_ELEMENTAL ||
       GET_RACE(victim) == RACE_COLOSSAL_FIRE_ELEMENTAL);

  /* how about wands and everything else?? */
  level = CASTER_LEVEL(ch);

  if (level < 1)
    level = 15; /* so lame */

  if (savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, CONJURATION))
    damage(ch, victim, is_fire ? dice(2, 6) : 0, SPELL_AQUEOUS_ORB, DAM_WATER, FALSE);
  else
    damage(ch, victim, is_fire ? dice(4, 6) : dice(2, 6), SPELL_AQUEOUS_ORB, DAM_WATER, FALSE);

  update_pos(victim);

  return 0;
}

/* The "return" of the event function is the time until the event is called
 * again. If we return 0, then the event is freed and removed from the list, but
 * any other numerical response will be the delay until the next call */
MUD_EVENT_CALLBACK(event_implode)
{
  struct char_data *ch, *victim = NULL;
  struct mud_event_data *pMudEvent;
  int casttype = CAST_SPELL;
  int level = 0;

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;
  if (ch && FIGHTING(ch)) // assign victim, if none escape
    victim = FIGHTING(ch);
  else
    return 0;

  if (ch == NULL || victim == NULL)
    return 0;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return 0;
  }

  if (mag_resistance(ch, victim, 0))
    return 0;

  /* how about wands and everything else?? */
  level = CASTER_LEVEL(ch);
  if (level < 1)
    level = 15; /* so lame */

  if (savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, DIVINATION))
    damage(ch, victim, (dice(CASTER_LEVEL(ch), 6) / 2), SPELL_IMPLODE, DAM_PUNCTURE, FALSE);
  else
    damage(ch, victim, dice(CASTER_LEVEL(ch), 6), SPELL_IMPLODE, DAM_PUNCTURE, FALSE);

  update_pos(victim);
  return 0;
}

/************************************************************/
/*  ASPELL defines                                          */
/************************************************************/

ASPELL(spell_acid_arrow)
{
  int x = 0, num_arrows = 1;

  if (ch == NULL || victim == NULL)
    return;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  num_arrows += CASTER_LEVEL(ch) / 3;

  for (x = 0; x < num_arrows; x++)
  {
    NEW_EVENT(eACIDARROW, ch, NULL, ((x * 6) * PASSES_PER_SEC));
  }
}

ASPELL(spell_control_summoned_creature)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char outbuf[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;
  struct char_data *master = NULL;

  one_argument(cast_arg3, arg1, sizeof(arg1));

  if (!*arg1)
  {
    send_to_char(ch, "You have to specify a summoned creature to control.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "No one by that name here.\r\n");
    return;
  }

  if (!IS_NPC(vict))
  {
    send_to_char(
        ch,
        "You can only used this on mobs that are summoned through various summoning spells.\r\n");
    return;
  }

  if (!AFF_FLAGGED(vict, AFF_CHARM) || !vict->master)
  {
    send_to_char(
        ch,
        "You can only used this on mobs that are summoned through various summoning spells.\r\n");
    return;
  }

  master = vict->master;

  if (!isSummonMob(GET_MOB_VNUM(vict)))
  {
    send_to_char(
        ch,
        "You can only used this on mobs that are summoned through various summoning spells.\r\n");
    return;
  }

  if (mag_resistance(ch, vict, 0) ||
      savingthrow(ch, vict, SAVING_WILL, 0, CAST_SPELL, CASTER_LEVEL(ch), ENCHANTMENT))
  {
    act("Your attempt to wrest control over $N fails.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n's attempt to wrest control over You fails.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n's attempt to wrest control over $N fails.", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }

  if (skill_roll(ch, ABILITY_SPELLCRAFT) < skill_roll(vict, ABILITY_SPELLCRAFT))
  {
    act("Your attempt to wrest control over $N's minion fails.", FALSE, ch, 0, master, TO_CHAR);
    act("$n's attempt to wrest control over Your minon fails.", FALSE, ch, 0, master, TO_CHAR);
    act("$n's attempt to wrest control over $N's minion fails.", FALSE, ch, 0, master, TO_NOTVICT);
    return;
  }

  snprintf(outbuf, sizeof(outbuf), "You wrest control of %s from $N.", GET_NAME(vict));
  act(outbuf, FALSE, ch, 0, master, TO_CHAR);
  snprintf(outbuf, sizeof(outbuf), "$n wrests control of %s from You.", GET_NAME(vict));
  act(outbuf, FALSE, ch, 0, master, TO_VICT);
  snprintf(outbuf, sizeof(outbuf), "$n wrests control of %s from $N.", GET_NAME(vict));
  act(outbuf, FALSE, ch, 0, master, TO_NOTVICT);

  if (GROUP(vict))
    leave_group(ch);
  stop_follower(vict);

  save_char_pets(master);

  add_follower(vict, ch);
  if (!GROUP(vict) && GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
    join_group(vict, GROUP(ch));

  save_char_pets(ch);
}

ASPELL(spell_siphon_might)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  char out[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *enemy = NULL, *recipient = NULL;
  int strength = 0;
  struct affected_type af, af2;

  two_arguments(cast_arg3, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "You need to specify the recipient of the siphoned might and whose might "
                     "you'd like to siphon.\r\n");
    return;
  }
  if (!*arg2)
  {
    if (FIGHTING(ch))
    {
      enemy = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "You need to specify the recipient of the siphoned might and whose might "
                       "you'd like to siphon.\r\n");
      return;
    }
  }

  if (!(recipient = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "There's no recipient here by that description.\r\n");
    return;
  }

  if (affected_by_spell(recipient, SPELL_SIPHON_MIGHT))
  {
    if (ch == recipient)
    {
      send_to_char(ch, "You're already under the effect of siphon might.\r\n");
    }
    else
    {
      act("$N is already under the effect of siphon might.", FALSE, ch, 0, recipient, TO_CHAR);
    }
    return;
  }

  if (!enemy)
  {
    if (!(enemy = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "There's no enemy here by that description.\r\n");
      return;
    }
  }

  if (affected_by_spell(recipient, SPELL_SIPHON_MIGHT))
  {
    act("$N is already under the effect of siphon might.", FALSE, ch, 0, enemy, TO_CHAR);
    return;
  }

  if (mag_resistance(ch, enemy, 0))
  {
    act("$N has resisted your siphoning.", FALSE, ch, 0, enemy, TO_CHAR);
    act("You have resisted $n's siphoning.", FALSE, ch, 0, enemy, TO_VICT);
    act("$N has resisted $n's siphoning.", FALSE, ch, 0, enemy, TO_NOTVICT);
    return;
  }

  strength = dice(1, 6) + MIN(5, CASTER_LEVEL(ch) / 2);

  if (!savingthrow(ch, enemy, SAVING_FORT, 0, SPELL_SIPHON_MIGHT, CASTER_LEVEL(ch), NECROMANCY))
  {
    act("$N has partially resisted your siphoning.", FALSE, ch, 0, enemy, TO_CHAR);
    act("You have partially resisted $n's siphoning.", FALSE, ch, 0, enemy, TO_VICT);
    act("You have partially resisted $n's siphoning.", FALSE, ch, 0, enemy, TO_NOTVICT);
    strength /= 2;
  }

  new_affect(&af);
  af.spell = SPELL_SIPHON_MIGHT;
  af.modifier = -strength;
  af.duration = CASTER_LEVEL(ch);
  af.location = APPLY_STR;
  affect_to_char(enemy, &af);

  new_affect(&af2);
  af2.spell = SPELL_SIPHON_MIGHT;
  af2.modifier = strength;
  af2.duration = CASTER_LEVEL(ch);
  af2.location = APPLY_STR;
  affect_to_char(recipient, &af2);

  if (ch == recipient)
  {
    act("You have siphoned might from $N into yourself.", FALSE, ch, 0, enemy, TO_CHAR);
    act("$n has siphoned might from You into $mself.", FALSE, ch, 0, enemy, TO_VICT);
    act("$n has siphoned might from $N into $mself.", FALSE, ch, 0, enemy, TO_CHAR);
  }
  else
  {
    snprintf(out, sizeof(out), "You have siphoned might from $N into %s", GET_NAME(recipient));
    act(out, FALSE, ch, 0, enemy, TO_CHAR);
    snprintf(out, sizeof(out), "$n has siphoned might from You into %s", GET_NAME(recipient));
    act(out, FALSE, ch, 0, enemy, TO_VICT);
    snprintf(out, sizeof(out), "$n has siphoned might from $N into %s", GET_NAME(recipient));
    act(out, FALSE, ch, 0, enemy, TO_NOTVICT);
  }
}

ASPELL(spell_human_potential)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  char which_stat[50] = {'\0'};
  char stat_buf[200] = {'\0'};
  struct affected_type af;
  struct char_data *vict = NULL;

  two_arguments(cast_arg3, arg1, sizeof(arg1), arg2, sizeof(arg2));

  new_affect(&af);

  if (!*arg1)
  {
    send_to_char(ch, "You need to specify who you wish to cast this upon.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "No one by that name here.\r\n");
    return;
  }

  if (affected_by_spell(vict, SPELL_HUMAN_POTENTIAL))
  {
    act("$N has already had their potential raised.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "Please specify an ability score: strength, constituion, dexterity, "
                     "intelligence, wisdom, charisma.\r\n");
    return;
  }

  if (is_abbrev(arg2, "strength"))
  {
    af.location = APPLY_STR;
    snprintf(which_stat, sizeof(which_stat), "stronger");
  }
  else if (is_abbrev(arg2, "constitution"))
  {
    af.location = APPLY_CON;
    snprintf(which_stat, sizeof(which_stat), "more hardy");
  }
  else if (is_abbrev(arg2, "dexterity"))
  {
    af.location = APPLY_DEX;
    snprintf(which_stat, sizeof(which_stat), "more agile");
  }
  else if (is_abbrev(arg2, "intelligence"))
  {
    af.location = APPLY_INT;
    snprintf(which_stat, sizeof(which_stat), "smarter");
  }
  else if (is_abbrev(arg2, "wisdom"))
  {
    af.location = APPLY_WIS;
    snprintf(which_stat, sizeof(which_stat), "wiser");
  }
  else if (is_abbrev(arg2, "charisma"))
  {
    af.location = APPLY_CHA;
    snprintf(which_stat, sizeof(which_stat), "more likeable");
  }
  else
  {
    send_to_char(ch, "Please specify an ability score: strength, constituion, dexterity, "
                     "intelligence, wisdom, charisma.2\r\n");
    return;
  }

  af.spell = SPELL_HUMAN_POTENTIAL;
  af.modifier = 2;
  af.duration = 10 * CASTER_LEVEL(ch);
  af.bonus_type = BONUS_TYPE_RACIAL;

  affect_to_char(vict, &af);

  snprintf(stat_buf, sizeof(stat_buf), "You feel %s.", which_stat);
  act(stat_buf, FALSE, ch, 0, vict, TO_VICT);
  snprintf(stat_buf, sizeof(stat_buf), "$N looks %s.", which_stat);
  act(stat_buf, FALSE, ch, 0, vict, TO_ROOM);
}

ASPELL(spell_mass_human_potential)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char which_stat[50] = {'\0'};
  char stat_buf[200] = {'\0'};
  struct affected_type af;
  struct char_data *vict = NULL;
  int apply_loc = APPLY_NONE;

  one_argument(cast_arg3, arg1, sizeof(arg1));

  if (!*arg1)
  {
    send_to_char(ch, "Please specify an ability score: strength, constituion, dexterity, "
                     "intelligence, wisdom, charisma.\r\n");
    return;
  }

  if (is_abbrev(arg1, "strength"))
  {
    apply_loc = APPLY_STR;
    snprintf(which_stat, sizeof(which_stat), "stronger");
  }
  else if (is_abbrev(arg1, "constitution"))
  {
    apply_loc = APPLY_CON;
    snprintf(which_stat, sizeof(which_stat), "more hardy");
  }
  else if (is_abbrev(arg1, "dexterity"))
  {
    apply_loc = APPLY_DEX;
    snprintf(which_stat, sizeof(which_stat), "more agile");
  }
  else if (is_abbrev(arg1, "intelligence"))
  {
    apply_loc = APPLY_INT;
    snprintf(which_stat, sizeof(which_stat), "smarter");
  }
  else if (is_abbrev(arg1, "wisdom"))
  {
    apply_loc = APPLY_WIS;
    snprintf(which_stat, sizeof(which_stat), "wiser");
  }
  else if (is_abbrev(arg1, "charisma"))
  {
    apply_loc = APPLY_CHA;
    snprintf(which_stat, sizeof(which_stat), "more likeable");
  }
  else
  {
    send_to_char(ch, "Please specify an ability score: strength, constituion, dexterity, "
                     "intelligence, wisdom, charisma.2\r\n");
    return;
  }

  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
  {
    if (GROUP(ch) != GROUP(vict) && ch != vict)
      continue;

    if (affected_by_spell(vict, SPELL_HUMAN_POTENTIAL))
    {
      act("$N has already had their potential raised.", FALSE, ch, 0, vict, TO_CHAR);
      continue;
    }

    new_affect(&af);

    af.location = apply_loc;
    af.spell = SPELL_HUMAN_POTENTIAL;
    af.modifier = 2;
    af.duration = 10 * level;
    af.bonus_type = BONUS_TYPE_RACIAL;

    affect_to_char(vict, &af);

    snprintf(stat_buf, sizeof(stat_buf), "You feel %s.", which_stat);
    act(stat_buf, FALSE, ch, 0, vict, TO_VICT);
    snprintf(stat_buf, sizeof(stat_buf), "$N looks %s.", which_stat);
    act(stat_buf, FALSE, ch, 0, vict, TO_ROOM);
  }
}

ASPELL(spell_aqueous_orb)
{
  int x = 0, num_rounds = 1;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *wall = NULL;

  one_argument(cast_arg2, arg, sizeof(arg));

  // if there's a wall of fire, quench it.
  if (*arg)
  {
    send_to_char(ch, "You send out a large ball of rolling water!\r\n");
    act("$n sends out a large ball of rolling water!", FALSE, ch, 0, 0, TO_ROOM);

    for (wall = world[victim->in_room].contents; wall; wall = wall->next_content)
    {
      if (GET_OBJ_TYPE(wall) == ITEM_WALL &&
          (GET_OBJ_VAL(wall, WALL_TYPE) == WALL_TYPE_FIRE ||
           GET_OBJ_VAL(wall, WALL_TYPE) == WALL_TYPE_PERILOUS_FIRE))
      {
        send_to_char(ch, "You quench a wall of fire!\r\n");
        act("$n quenches a wall of fire!", FALSE, ch, 0, 0, TO_ROOM);
        obj_from_room(wall);
      }
    }
  }

  // Now inflict a DoT

  if (ch == NULL || victim == NULL)
    return;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  num_rounds += CASTER_LEVEL(ch);

  for (x = 0; x < num_rounds; x++)
  {
    NEW_EVENT(eAQUEOUSORB, ch, NULL, ((x * 6) * PASSES_PER_SEC));
  }
}

ASPELL(spell_banish)
{
  struct follow_type *k;

  if (!ch || !victim)
    return;

  /* go through target's list of followers */
  for (k = victim->followers; k; k = k->next)
  {
    /* follower in same room? */
    if (IN_ROOM(victim) == IN_ROOM(k->follower))
    {
      /* actually a follower? */
      if (AFF_FLAGGED(k->follower, AFF_CHARM))
      {
        /* might have to downgrade this */
        if (IS_NPC(k->follower))
        {
          /* great, attempt to banish */
          act("$n banishes $N!", FALSE, ch, 0, k->follower, TO_ROOM);
          send_to_char(ch, "You banish %s!\r\n", GET_NAME(k->follower));
          extract_char(k->follower);

          /* 50% chance to keep on banishing away */
          if (!rand_number(0, 1))
            return;
        }
      }
    }
  }
}

ASPELL(spell_charm) // enchantment
{
  if (victim == NULL || ch == NULL)
    return;

  effect_charm(ch, victim, SPELL_CHARM, casttype, level);
}

ASPELL(spell_charm_monster) // enchantment
{
  if (victim == NULL || ch == NULL)
    return;

  effect_charm(ch, victim, SPELL_CHARM_MONSTER, casttype, level);
}

ASPELL(spell_charm_animal) // enchantment
{
  if (victim == NULL || ch == NULL)
    return;

  if (IS_NPC(victim) && GET_RACE(victim) == RACE_TYPE_ANIMAL)
  {
    effect_charm(ch, victim, SPELL_CHARM_ANIMAL, casttype, level);
  }
  else
  {
    send_to_char(ch, "This spell can only be used on animals.");
  }
}

ASPELL(spell_clairvoyance)
{
  room_rnum location;

  if (ch == NULL || victim == NULL)
    return;

  if (AFF_FLAGGED(victim, AFF_NON_DETECTION))
  {
    send_to_char(ch, "Your victim is affected by non-detection.\r\n");
    return;
  }

  if (GET_LEVEL(victim) >= LVL_IMMORT)
  {
    send_to_char(ch, "You can't spy on staff members.\r\n");
    return;
  }

  if ((location = IN_ROOM(victim)) == NOWHERE)
  {
    send_to_char(ch, "Your spell fails.\r\n");
    return;
  }

  /* Use look_at_room_number to view the target room without physically moving there */
  send_to_char(ch, "You close your eyes and your vision shifts...\r\n\r\n");
  look_at_room_number(ch, 0, location);
  send_to_char(ch, "\r\nYour vision returns to normal.\r\n");
}

ASPELL(spell_cloudkill)
{
  int num_of_clouds = 0;

  if (INCENDIARY(ch) || DOOM(ch) || TENACIOUS_PLAGUE(ch))
  {
    send_to_char(ch, "You already have a cloud following you!\r\n");
    return;
  }
  send_to_char(ch, "You summon forth a cloud of death!\r\n");
  act("$n summons forth a cloud of death!", FALSE, ch, 0, 0, TO_ROOM);

  if (!IS_NPC(ch) && HAS_DOMAIN(ch, DOMAIN_DESTRUCTION))
  {
    num_of_clouds = DIVINE_LEVEL(ch) / 5;
  }

  CLOUDKILL(ch) = MAX((CASTER_LEVEL(ch) / 5), num_of_clouds);
}

ASPELL(spell_control_plants)
{
  if (victim == NULL || ch == NULL)
    return;

  if (IS_NPC(victim) && GET_RACE(victim) == RACE_TYPE_PLANT)
  {
    effect_charm(ch, victim, SPELL_CONTROL_PLANTS, casttype, level);
  }
  else
  {
    send_to_char(ch, "This spell can only be used on plants.");
  }
}

/* i decided to wait for room events for this one */
ASPELL(spell_control_weather)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  if (IS_NPC(ch) || !ch->desc)
    return;

  one_argument(cast_arg2, arg, sizeof(arg));

  if (is_abbrev(arg, "worsen"))
  {
  }
  else if (is_abbrev(arg, "improve"))
  {
  }
  else
  {
    send_to_char(ch, "You need to cast this spell with an argument of either, "
                     "'worsen' or 'improve' in order for it to be a success!\r\n");
    return;
  }
}

ASPELL(spell_create_water)
{
  int water;

  if (ch == NULL || obj == NULL)
    return;

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
  {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0))
    {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
    }
    else
    {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0)
      {
        if (GET_OBJ_VAL(obj, 1) >= 0)
          name_from_drinkcon(obj);
        GET_OBJ_VAL(obj, 2) = LIQ_WATER;
        GET_OBJ_VAL(obj, 1) += water;
        name_to_drinkcon(obj, LIQ_WATER);
        weight_change_object(obj, water);
        act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}

ASPELL(spell_creeping_doom)
{
  if (CLOUDKILL(ch) || INCENDIARY(ch))
  {
    send_to_char(ch, "You already have a cloud following you!\r\n");
    return;
  }

  send_to_char(ch, "You summon forth a mass of centipede swarms!\r\n");
  act("$n summons forth a mass of centipede swarms!", FALSE, ch, 0, 0, TO_ROOM);

  DOOM(ch) = MAX(1, (DIVINE_LEVEL(ch) + GET_CALL_EIDOLON_LEVEL(ch)));
}

ASPELL(spell_detect_poison)
{
  if (victim)
  {
    if (victim == ch)
    {
      if (AFF_FLAGGED(victim, AFF_POISON))
        send_to_char(ch, "You can sense poison in your blood.\r\n");
      else
        send_to_char(ch, "You feel healthy.\r\n");
    }
    else
    {
      if (AFF_FLAGGED(victim, AFF_POISON))
        act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
      else
        act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
    }
  }

  if (obj)
  {
    switch (GET_OBJ_TYPE(obj))
    {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
        act("You sense that $p has been contaminated.", FALSE, ch, obj, 0, TO_CHAR);
      else
        act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0, TO_CHAR);
      break;
    default:
      send_to_char(ch, "You sense that it should not be consumed.\r\n");
    }
  }
}

ASPELL(spell_arcane_mark)
{
  const char *mark = NULL;
  char truncated[MAX_ARCANE_MARK_LENGTH + 1];

  if (!obj)
  {
    send_to_char(ch, "You need to focus this spell on an object to mark it.\r\n");
    return;
  }

  /* Check if object already has an arcane mark */
  if (GET_OBJ_ARCANE_MARK(obj))
  {
    send_to_char(ch, "That object already bears an arcane mark reading \"%s\"\tn.\r\n",
                 GET_OBJ_ARCANE_MARK(obj));
    return;
  }

  mark = GET_ARCANE_MARK(ch);
  if (!mark || !*mark || !strcmp(mark, "(null)") || !strcmp(mark, "null"))
  {
    send_to_char(ch, "You have not set an arcane mark signature. Use 'arcanemark <signature>' to "
                     "set it.\r\n");
    send_to_char(ch, "The ARCANEMARK command defines the signature; this spell applies it to an "
                     "inventory object.\r\n");
    send_to_char(ch, "Your signature can be up to %d characters long, including color codes.\r\n",
                 MAX_ARCANE_MARK_LENGTH);
    send_to_char(ch, "Please keep your arcane mark in-character and tasteful.\r\n");
    send_to_char(ch, "You can change it later with 'arcanemark <signature>' or remove it with "
                     "'arcanemark clear'.\r\n");
    return;
  }

  /* Enforce hard cap on stored marks */
  if (strlen(mark) > MAX_ARCANE_MARK_LENGTH)
  {
    strlcpy(truncated, mark, sizeof(truncated));
    mark = truncated;
  }

  GET_OBJ_ARCANE_MARK(obj) = strdup(mark);

  act("You inscribe your arcane mark upon $p.", FALSE, ch, obj, 0, TO_CHAR);
  send_to_char(ch,
               "The mark reads \"%s\"\tn. Use LOOK <object> or EXAMINE <object> to read it "
               "again.\r\n",
               mark);
  act("$n inscribes a faint sigil onto $p.", TRUE, ch, obj, 0, TO_ROOM);
}

/* This spell allows travel to any Luminari carriage destination. */
ASPELL(spell_overland_flight)
{
  if (IN_ROOM(ch) == NOWHERE)
    return;

  char *zone = cast_arg3 + 1;

  if (zone == NULL || !*zone || strlen(zone) < 3)
  {
    send_to_char(ch,
                 "Please specify the area you'd like to fly to.  Type flightlist for a list.\r\n");
    return;
  }

  start_flight_to_destination_luminari(ch, zone);
}

ASPELL(spell_dismissal)
{
  struct follow_type *k;

  if (!ch || !victim)
    return;

  /* go through target's list of followers */
  for (k = victim->followers; k; k = k->next)
  {
    /* follower in same room? */
    if (IN_ROOM(victim) == IN_ROOM(k->follower))
    {
      /* actually a follower? */
      if (AFF_FLAGGED(k->follower, AFF_CHARM))
      {
        /* has proper subrace to be dismissed? */
        if (IS_NPC(k->follower) && HAS_SUBRACE(k->follower, SUBRACE_EXTRAPLANAR))
        {
          /* great, attempt to dismiss and exit, just one victim */
          act("$n dismisses $N!", FALSE, ch, 0, k->follower, TO_ROOM);
          send_to_char(ch, "You dismiss %s!\r\n", GET_NAME(k->follower));
          extract_char(k->follower);
          return;
        }
      }
    }
  }
}

ASPELL(spell_dispel_magic) // divination
{
  if (ch == NULL)
    return;
  if (victim == NULL)
    victim = ch;

  perform_dispel(ch, victim, obj, SPELL_DISPEL_MAGIC);
}

ASPELL(spell_dominate_person) // enchantment
{
  if (victim == NULL || ch == NULL)
    return;

  effect_charm(ch, victim, SPELL_DOMINATE_PERSON, casttype, level);
}

/* Cannot use this spell on an equipped object or it will mess up the wielding
 * character's hit/dam totals. */
ASPELL(spell_enchant_item) // enchantment
{
  int i;
  int bonus = 0;
  int eligible = true;

  if (ch == NULL || obj == NULL)
    return;

  /* Either already enchanted or not a weapon. */
  if ((GET_OBJ_TYPE(obj) != ITEM_WEAPON && GET_OBJ_TYPE(obj) != ITEM_ARMOR) ||
      OBJ_FLAGGED(obj, ITEM_MAGIC))
    eligible = false;

  /* Make sure no other affections. */
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj->affected[i].location != APPLY_NONE)
    {
      eligible = false;
      break;
    }

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) == obj)
    {
      eligible = false;
      break;
    }

  if (GET_OBJ_VAL(obj, 4) > 0)
    eligible = false;

  if (!eligible)
  {
    send_to_char(ch,
                 "Your spell failed.\r\n"
                 "This spell will only work on nonmagical weapons and armor that offer no stat "
                 "modifications.\r\n"
                 "It will also fail on any worn equipment.  The item must be in your inventory");
    return;
  }

  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  bonus = MAX(1, (int)(CASTER_LEVEL(ch) / 7));

  // enhancement bonus
  GET_OBJ_VAL(obj, 4) = bonus;

  switch (bonus)
  {
  case 1:
    GET_OBJ_LEVEL(obj) = 1;
    break;
  case 2:
    GET_OBJ_LEVEL(obj) = 8;
    break;
  case 3:
    GET_OBJ_LEVEL(obj) = 16;
    break;
  case 4:
    GET_OBJ_LEVEL(obj) = 25;
    break;
  default:
    GET_OBJ_LEVEL(obj) = 31;
    break;
  }

  act("$p glows \tYyellow\tn.", FALSE, ch, obj, 0, TO_CHAR);
}

ASPELL(spell_greater_dispelling) // abjuration
{
  if (ch == NULL)
    return;
  if (victim == NULL)
    victim = ch;

  perform_dispel(ch, victim, obj, SPELL_GREATER_DISPELLING);
}

ASPELL(spell_group_summon)
{
  struct char_data *tch = NULL;

  if (ch == NULL)
    return;

  if (!GROUP(ch))
    return;

  if (!valid_mortal_tele_dest(ch, IN_ROOM(ch), TRUE))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  simple_list(NULL);
  while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
  {
    if (ch == tch)
      continue;

    if (MOB_FLAGGED(tch, MOB_NOSUMMON) || char_has_worn_object_flag(tch, ITEM_ROL_NO_SUMMON))
      continue;

    if (IN_ROOM(tch) == IN_ROOM(ch))
      continue;

    if (!valid_mortal_tele_dest(tch, IN_ROOM(tch), TRUE))
      continue;

    act("$n disappears suddenly.", TRUE, tch, 0, 0, TO_ROOM);

    char_from_room(tch);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
    {
      X_LOC(tch) = world[IN_ROOM(ch)].coords[0];
      Y_LOC(tch) = world[IN_ROOM(ch)].coords[1];
    }

    char_to_room_cause(tch, IN_ROOM(ch), ch, DOMAIN_RELOCATION_TELEPORT, -1);

    act("$n arrives suddenly.", TRUE, tch, 0, 0, TO_ROOM);
    act("$n has summoned you!", FALSE, ch, 0, tch, TO_VICT);
    look_at_room(tch, 0);
    entry_memory_mtrigger(tch);
    greet_mtrigger(tch, -1);
    greet_memory_mtrigger(tch);
  }
}

ASPELL(spell_identify) // divination
{
  if (obj)
  {
    if (OBJ_FLAGGED(obj, ITEM_ROL_NO_IDENTIFY) && GET_LEVEL(ch) < LVL_IMMORT)
    {
      send_to_char(ch, "Your senses boggle; you are unable to identify that item.\r\n");
      return;
    }
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_IDENTIFIED);
    do_stat_object(ch, obj, ITEM_STAT_MODE_IDENTIFY_SPELL);
  }
  else if (victim)
  {
    /* victim */
    lore_id_vict(ch, victim);
  }
}

ASPELL(spell_implode)
{
  int x = 0;

  if (ch == NULL || victim == NULL)
    return;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  send_to_char(ch, "You cause %s to implode!\r\n", GET_NAME(victim));
  act("$n causes $N to implode!", FALSE, ch, 0, victim, TO_NOTVICT);
  act("$n causes you to implode!", FALSE, ch, 0, victim, TO_VICT);

  for (x = 0; x < (CASTER_LEVEL(ch) / 3); x++)
  {
    NEW_EVENT(eIMPLODE, ch, NULL, ((x * 6) * PASSES_PER_SEC));
  }
}

ASPELL(spell_incendiary_cloud)
{
  if (CLOUDKILL(ch) || DOOM(ch) || TENACIOUS_PLAGUE(ch))
  {
    send_to_char(ch, "You already have a cloud following you!\r\n");
    return;
  }

  send_to_char(ch, "You summon forth an incendiary cloud!\r\n");
  act("$n summons forth an incendiary cloud!", FALSE, ch, 0, 0, TO_ROOM);

  INCENDIARY(ch) = MAX(1, CASTER_LEVEL(ch) / 4);
}

ASPELL(spell_locate_creature)
{
  struct char_data *i;
  int found = 0, num = 0;

  if (ch == NULL)
    return;
  if (victim == NULL)
    return;
  if (victim == ch)
  {
    send_to_char(ch, "You were once lost, but now you are found!\r\n");
    return;
  }

  char vname[MAX_NAME_LENGTH], iname[MAX_NAME_LENGTH];

  snprintf(vname, MAX_NAME_LENGTH, "%s", GET_NAME(victim));

  send_to_char(ch, "%s\r\n", QNRM);
  for (i = character_list; i; i = i->next)
  {
    snprintf(iname, MAX_NAME_LENGTH, "%s", GET_NAME(i));
    if (is_abbrev(vname, iname) && CAN_SEE(ch, i) && IN_ROOM(i) != NOWHERE)
    {
      found = 1;
      send_to_char(ch, "%3d. %-25s%s - %-25s%s", ++num, GET_NAME(i), QNRM, world[IN_ROOM(i)].name,
                   QNRM);
      send_to_char(ch, "%s\r\n", QNRM);
    }
  }

  if (!found)
    send_to_char(ch, "Couldn't find any such creature.\r\n");
}

ASPELL(spell_locate_object)
{
  struct obj_data *i = NULL;
  int j = 0, bonus_stat = 0;
  bool found = FALSE;

  if (!obj)
  {
    send_to_char(ch, "You sense nothing.\r\n");
    return;
  }

  /*  added a global var to catch 2nd arg. */
  // char name[MAX_INPUT_LENGTH] = {'\0'};
  // snprintf(name, sizeof(name), "%s", cast_arg2);

  /* # items to show = caster-level + highest mental stat bonus */
  bonus_stat = GET_INT_BONUS(ch);

  if (GET_WIS_BONUS(ch) > bonus_stat)
    bonus_stat = GET_WIS_BONUS(ch);

  if (GET_CHA_BONUS(ch) > bonus_stat)
    bonus_stat = GET_CHA_BONUS(ch);

  j = CASTER_LEVEL(ch) + bonus_stat;
  /* got j.. */

  /* loop through object list */
  for (i = object_list; i && (j > 0); i = i->next)
  {
    /* found something, bingo! */
    if (CAN_SEE_OBJ(ch, i) && isname(cast_arg2, i->name))
    {
      found = TRUE;

      send_to_char(ch, "%c%s", UPPER(*i->short_description), i->short_description + 1);

      if (i->carried_by)
        send_to_char(ch, " is being carried by %s.\r\n", PERS(i->carried_by, ch));
      else if (IN_ROOM(i) != NOWHERE)
        send_to_char(ch, " is in %s.\r\n", world[IN_ROOM(i)].name);
      else if (i->in_obj)
        send_to_char(ch, " is in %s.\r\n", i->in_obj->short_description);
      else if (i->worn_by)
        send_to_char(ch, " is being worn by %s.\r\n", PERS(i->worn_by, ch));
      else
        send_to_char(ch, "'s location is uncertain.\r\n");

      j--;
    }
  }

  if (!found)
    send_to_char(ch, "Couldn't find any such thing.\r\n");

  return;
}

/* Callback for mass domination AoE effect */
static int mass_domination_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  struct mass_dom_data
  {
    int casttype;
    int level;
  } *dom_data = (struct mass_dom_data *)data;

  effect_charm(ch, tch, SPELL_MASS_DOMINATION, dom_data->casttype, dom_data->level);
  return 1;
}

ASPELL(spell_mass_domination) // enchantment
{
  struct mass_dom_data
  {
    int casttype;
    int level;
  } dom_data;

  if (ch == NULL)
    return;

  dom_data.casttype = casttype;
  dom_data.level = level;

  aoe_effect(ch, -1, mass_domination_callback, &dom_data);
}

ASPELL(spell_plane_shift)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  room_rnum to_room = NOWHERE;

  if (ch == NULL)
    return;

  if (!valid_mortal_tele_dest(ch, IN_ROOM(ch), TRUE))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  one_argument(cast_arg2, arg, sizeof(arg));

  if (is_abbrev(arg, "astral"))
  {
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE))
    {
      send_to_char(ch, "You are already on the astral plane!\r\n");
      return;
    }

    do
    {
      to_room = rand_number(0, top_of_world);
    } while (!ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ASTRAL_PLANE));
  }
  else if (is_abbrev(arg, "ethereal"))
  {
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE))
    {
      send_to_char(ch, "You are already on the ethereal plane!\r\n");
      return;
    }

    do
    {
      to_room = rand_number(0, top_of_world);
    } while (!ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ETH_PLANE));
  }
  else if (is_abbrev(arg, "elemental"))
  {
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL))
    {
      send_to_char(ch, "You are already on the elemental plane!\r\n");
      return;
    }

    do
    {
      to_room = rand_number(0, top_of_world);
    } while (!ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ELEMENTAL));
  }
  else if (is_abbrev(arg, "prime"))
  {
    if (!ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE) &&
        !ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE) &&
        !ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL))
    {
      send_to_char(ch, "You need to be off the prime plane to gate to it!\r\n");
      return;
    }

    do
    {
      to_room = rand_number(0, top_of_world);
    } while ((ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ELEMENTAL) ||
              ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ETH_PLANE) ||
              ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ASTRAL_PLANE)));
  }
  else
  {
    send_to_char(ch, "Not a valid target (astral, ethereal, elemental, prime)");
    return;
  }

  if (!valid_mortal_tele_dest(ch, to_room, TRUE))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  send_to_char(ch, "You slowly fade out of existence...\r\n");
  act("$n slowly fades out of existence and is gone.", FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[to_room].coords[0];
    Y_LOC(ch) = world[to_room].coords[1];
  }
  char_to_room_cause(ch, to_room, ch, DOMAIN_RELOCATION_TELEPORT, -1);

  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "You slowly fade back into existence...\r\n");
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

#define GENIE_DJINNI 1
#define GENIE_EFREETI 2
#define GENIE_MARID 3
#define GENIE_SHAITAN 4

ASPELL(spell_geniekind)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int geniekind = 0;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (!can_add_follower_by_flag(ch, MOB_GENIEKIND))
  {
    send_to_char(ch, "You already have a geniekind follower.\r\n");
    return;
  }

  one_argument(cast_arg3, arg, sizeof(arg));

  if (is_abbrev(arg, "djinni"))
  {
    geniekind = GENIE_DJINNI;
  }
  else if (is_abbrev(arg, "efreeti"))
  {
    geniekind = GENIE_EFREETI;
  }
  else if (is_abbrev(arg, "marid"))
  {
    geniekind = GENIE_MARID;
  }
  else if (is_abbrev(arg, "shaitan"))
  {
    geniekind = GENIE_SHAITAN;
  }
  else
  {
    geniekind = dice(1, 4);
  }

  // clear all geniekind affects so we aren't stacking multiple kinds.
  affect_from_char(ch, SPELL_DJINNI_KIND);
  affect_from_char(ch, SPELL_EFREETI_KIND);
  affect_from_char(ch, SPELL_MARID_KIND);
  affect_from_char(ch, SPELL_SHAITAN_KIND);

  switch (geniekind)
  {
  case GENIE_DJINNI:
    mag_affects(CASTER_LEVEL(ch), ch, ch, 0, SPELL_DJINNI_KIND, 0, CAST_SPELL, 0);
    mag_summons(CASTER_LEVEL(ch), ch, 0, SPELL_DJINNI_KIND, 0, CAST_SPELL);
    break;
  case GENIE_EFREETI:
    mag_affects(CASTER_LEVEL(ch), ch, ch, 0, SPELL_EFREETI_KIND, 0, CAST_SPELL, 0);
    mag_summons(CASTER_LEVEL(ch), ch, 0, SPELL_EFREETI_KIND, 0, CAST_SPELL);
    break;
  case GENIE_MARID:
    mag_affects(CASTER_LEVEL(ch), ch, ch, 0, SPELL_MARID_KIND, 0, CAST_SPELL, 0);
    mag_summons(CASTER_LEVEL(ch), ch, 0, SPELL_MARID_KIND, 0, CAST_SPELL);
    break;
  case GENIE_SHAITAN:
    mag_affects(CASTER_LEVEL(ch), ch, ch, 0, SPELL_SHAITAN_KIND, 0, CAST_SPELL, 0);
    mag_summons(CASTER_LEVEL(ch), ch, 0, SPELL_SHAITAN_KIND, 0, CAST_SPELL);
    break;
  default:
    send_to_char(ch, "You were unable to summon any kind of genie.\r\n");
    break;
  }
}

ASPELL(spell_polymorph)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (IS_WILDSHAPED(ch))
  {
    send_to_char(ch, "You cannot polymorph while wildshaped.\r\n");
    return;
  }

  one_argument(cast_arg2, arg, sizeof(arg));

  /* act.other.c, part of druid wildshape engine, the value "1" notifies the
       the function that this is the polymorph spells */
  wildshape_engine(ch, arg, 1);
}

ASPELL(spell_prismatic_sphere)
{
  struct char_data *mob;

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (!(mob = read_mobile_reason(PRISMATIC_SPHERE, VIRTUAL, PERF_ENTITY_SPELL_SUMMON)))
  {
    send_to_char(ch, "You don't quite remember how to create that.\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
  {
    X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
    Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
  }
  char_to_room_cause(mob, IN_ROOM(ch), ch, DOMAIN_RELOCATION_SPAWN, -1);

  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  act("$n conjures $N!", FALSE, ch, 0, mob, TO_ROOM);
  send_to_char(ch, "You conjure a prismatic sphere!\r\n");

  load_mtrigger(mob);
}

ASPELL(spell_recall)
{
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT) ||
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NORECALL))
  {
    send_to_char(ch, "Something in the area is hampering your magic!\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room_cause(victim, r_mortal_start_room, ch, DOMAIN_RELOCATION_TELEPORT, -1);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

ASPELL(spell_luskan_recall)
{
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT) ||
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NORECALL))
  {
    send_to_char(ch, "Something in the area is hampering your magic!\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room_cause(victim, real_room(3088), ch, DOMAIN_RELOCATION_TELEPORT, -1);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

ASPELL(spell_triboar_recall)
{
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT) ||
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NORECALL))
  {
    send_to_char(ch, "Something in the area is hampering your magic!\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room_cause(victim, real_room(7000), ch, DOMAIN_RELOCATION_TELEPORT, -1);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

ASPELL(spell_silverymoon_recall)
{
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT) ||
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NORECALL))
  {
    send_to_char(ch, "Something in the area is hampering your magic!\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room_cause(victim, real_room(6118), ch, DOMAIN_RELOCATION_TELEPORT, -1);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

ASPELL(spell_mirabar_recall)
{
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT) ||
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NORECALL))
  {
    send_to_char(ch, "Something in the area is hampering your magic!\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room_cause(victim, real_room(4923), ch, DOMAIN_RELOCATION_TELEPORT, -1);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}


ASPELL(spell_palanthas_recall)
{
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT) ||
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NORECALL))
  {
    send_to_char(ch, "Something in the area is hampering your magic!\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room_cause(victim, real_room(2200), ch, DOMAIN_RELOCATION_TELEPORT, -1);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}


ASPELL(spell_sanction_recall)
{
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT) ||
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NORECALL))
  {
    send_to_char(ch, "Something in the area is hampering your magic!\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room_cause(victim, real_room(6530), ch, DOMAIN_RELOCATION_TELEPORT, -1);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}


ASPELL(spell_solace_recall)
{
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOTELEPORT) ||
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_NORECALL))
  {
    send_to_char(ch, "Something in the area is hampering your magic!\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(victim)), ZONE_NOASTRAL))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room_cause(victim, real_room(1317), ch, DOMAIN_RELOCATION_TELEPORT, -1);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

ASPELL(spell_refuge) // illusion (also divine)
{
  struct char_data *tch, *next_tch;
  struct affected_type af;

  if (ch == NULL)
    return;

  act("As $n makes a strange arcane gesture, a golden light descends\r\n"
      "from the heavens!\r\n",
      FALSE, ch, 0, 0, TO_ROOM);
  send_to_room(IN_ROOM(ch), "The room is a refuge!\r\n");

  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
  {
    next_tch = tch->next_in_room;

    /* this is to possible victims */
    if (tch && aoeOK(ch, tch, -1))
    {
      if (FIGHTING(tch))
      {
        stop_fighting(tch);
        resetCastingData(tch);
      }
      if (IS_NPC(tch))
        clearMemory(tch);

      /* this should be allies */
    }
    else if (tch)
    {
      send_to_char(tch, "You are now refuged.\r\n");
      if (FIGHTING(tch))
        stop_fighting(tch);

      if (!AFF_FLAGGED(tch, AFF_SNEAK))
      {
        SET_BIT_AR(AFF_FLAGS(tch), AFF_SNEAK);
      }
      if (!AFF_FLAGGED(tch, AFF_HIDE))
      {
        SET_BIT_AR(AFF_FLAGS(tch), AFF_HIDE);
      }

      new_affect(&af);
      af.spell = SPELL_REFUGE;
      af.duration = 6;
      SET_BIT_AR(af.bitvector, AFF_REFUGE);
      affect_to_char(tch, &af);
    }
  }
}

ASPELL(spell_salvation) // divination
{
  room_vnum load_broom;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE))
  {
    send_to_char(ch, "You can't use salvation on the astral plane.\r\n");
    return;
  }
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE))
  {
    send_to_char(ch, "You can't use salvation on the ethereal plane.\r\n");
    return;
  }
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL))
  {
    send_to_char(ch, "You can't use salvation on the elemental plane.\r\n");
    return;
  }

  if (!PLR_FLAGGED(ch, PLR_SALVATION) || !GET_SALVATION_NAME(ch) ||
      GET_SALVATION_ROOM(ch) == NOWHERE)
  {
    if (!valid_mortal_tele_dest(ch, real_room(world[ch->in_room].number), TRUE))
    {
      send_to_char(ch, "You can't use salvation here.\r\n");
      return;
    }

    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
    {
      send_to_char(ch, "You can't use salvation in the wilderness.\r\n");
      return;
    }

    SET_BIT_AR(PLR_FLAGS(ch), PLR_SALVATION);
    load_broom = world[ch->in_room].number;
    if (GET_SALVATION_NAME(ch) != NULL)
      GET_SALVATION_NAME(ch) = NULL;
    GET_SALVATION_NAME(ch) = strdup(world[ch->in_room].name);
    GET_SALVATION_ROOM(ch) = load_broom;
    send_to_char(ch, "Your salvation is set to this room.\r\n");
    return;
  }
  else
  {
    if (!valid_mortal_tele_dest(ch, real_room(world[ch->in_room].number), TRUE))
    {
      send_to_char(ch, "You can't use salvation here.\r\n");
      return;
    }

    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_SALVATION);
    if (GET_SALVATION_NAME(ch) != NULL)
      GET_SALVATION_NAME(ch) = NULL;
    load_broom = GET_SALVATION_ROOM(ch);
    load_broom = real_room(load_broom);
    act("$n disappears in a flash of white light", FALSE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room_cause(ch, load_broom, ch, DOMAIN_RELOCATION_TELEPORT, -1);
    send_to_char(ch, "As the flash of light disappears you can see the room.\r\n\r\n");
    act("$n appears in a flash of white light", FALSE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);
    return;
  }
}

/* The "return" of the event function is the time until the event is called
 * again. If we return 0, then the event is freed and removed from the list, but
 * any other numerical response will be the delay until the next call */
MUD_EVENT_CALLBACK(event_moonbeam)
{
  struct char_data *ch, *victim = NULL;
  struct mud_event_data *pMudEvent;
  int casttype = CAST_SPELL;
  int level = 0;

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;
  if (ch && FIGHTING(ch)) // assign victim, if none escape
    victim = FIGHTING(ch);
  else
    return 0;

  if (ch == NULL || victim == NULL)
    return 0;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return 0;
  }

  if (mag_resistance(ch, victim, 0))
    return 0;

  /* how about wands and everything else?? */
  level = CASTER_LEVEL(ch);
  if (level < 1)
    level = 15;

  if (savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, EVOCATION))
    damage(ch, victim, dice(2, 10), SPELL_MOONBEAM, DAM_LIGHT, FALSE);
  else
    damage(ch, victim, dice(1, 10), SPELL_MOONBEAM, DAM_LIGHT, FALSE);

  update_pos(victim);
  return 0;
}

ASPELL(spell_moonbeam)
{
  int x = 0;

  if (ch == NULL || victim == NULL)
    return;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  send_to_char(ch, "You call forth a searing beam of moonlight upon your opponent!\r\n");
  act("$n calls forth a searing beam of moonlight!", FALSE, ch, 0, 0, TO_ROOM);

  for (x = 0; x < 5; x++)
  {
    NEW_EVENT(eMOONBEAM, ch, NULL, ((x * 6) * PASSES_PER_SEC));
  }
}

ASPELL(spell_spellstaff)
{
  char spellname[MAX_STRING_LENGTH] = {'\0'};
  struct obj_data *staff = NULL;
  int spellnum = 0;

  // cast_arg2 should be the spellname
  one_argument(cast_arg2, spellname, sizeof(spellname));

  if (!*spellname)
  {
    send_to_char(ch, "You must specify which spell you want to enchant the staff with.\r\n");
    return;
  }

  // find staff in caster's hands
  if (GET_EQ(ch, WEAR_HOLD_1) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD_1)) == ITEM_STAFF)
    staff = GET_EQ(ch, WEAR_HOLD_1);
  else if (GET_EQ(ch, WEAR_HOLD_2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD_2)) == ITEM_STAFF)
    staff = GET_EQ(ch, WEAR_HOLD_2);
  else if (GET_EQ(ch, WEAR_HOLD_2H) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD_2H)) == ITEM_STAFF)
    staff = GET_EQ(ch, WEAR_HOLD_2H);

  //  for (staff = ch->carrying; staff; staff = staff->next_content) {
  //    if (GET_OBJ_TYPE(staff) == ITEM_STAFF) {
  //      // found one!
  //      break;
  //    }
  //  }

  if (staff)
  {
    if (GET_OBJ_VAL(staff, 2) > 0)
    {
      send_to_char(ch, "That staff is already enchanted with a spell.\r\n");
      return;
    }
    else
    {
      // determine the spellname to enchant with
      if (is_abbrev(spellname, "barkskin"))
        spellnum = SPELL_BARKSKIN;
      else if (is_abbrev(spellname, "cure light wounds"))
        spellnum = SPELL_CURE_LIGHT;
      else if (is_abbrev(spellname, "endurance"))
        spellnum = SPELL_ENDURANCE;
      else if (is_abbrev(spellname, "flame strike"))
        spellnum = SPELL_FLAME_STRIKE;
      else if (is_abbrev(spellname, "strength"))
        spellnum = SPELL_STRENGTH;

      if (spellnum != 0)
      {
        GET_OBJ_VAL(staff, 0) = GET_LEVEL(ch); // new staff only cast at caster's level
        GET_OBJ_VAL(staff, 2) = 1;             // only good for 1 charge
        GET_OBJ_VAL(staff, 3) = spellnum;
        send_to_char(ch, "You enchant %s with the %s spell.\r\n", staff->short_description,
                     spell_info[spellnum].name);
        act("$n concentrates on enhancing the power of $p.", FALSE, ch, staff, 0, TO_ROOM);
      }
      else
      {
        send_to_char(ch, "You are unable to store that spell in the staff.\r\n");
      }
    }
  }
  else
  {
    send_to_char(ch, "You are not holding a staff.\r\n");
  }
}

ASPELL(spell_summon_instrument)
{
  struct obj_data *instrument = NULL;
  int instrument_type = -1;
  int i = 0;
  int j = 0;
  char instrument_name[MAX_INPUT_LENGTH] = {'\0'};
  char instrument_lower[MAX_INPUT_LENGTH] = {'\0'};
  char long_buf[MAX_INPUT_LENGTH + 64] = {'\0'};
  char short_buf[MAX_INPUT_LENGTH + 16] = {'\0'};

  /* Parse the instrument name from cast_arg2 */
  one_argument(cast_arg2, instrument_name, sizeof(instrument_name));

  if (!*instrument_name)
  {
    send_to_char(ch, "You must specify which instrument to summon.\r\n");
    send_to_char(ch, "Available instruments: ");
    for (i = 0; i < MAX_INSTRUMENTS; i++)
    {
      char available_lower[MAX_INPUT_LENGTH] = {'\0'};
      snprintf(available_lower, sizeof(available_lower), "%s", instrument_names[i]);
      for (j = 0; available_lower[j]; j++)
        available_lower[j] = LOWER(available_lower[j]);
      send_to_char(ch, "%s%s", available_lower, (i < MAX_INSTRUMENTS - 1) ? ", " : ".\r\n");
    }
    return;
  }

  /* Find matching instrument */
  for (i = 0; i < MAX_INSTRUMENTS; i++)
  {
    if (is_abbrev(instrument_name, instrument_names[i]))
    {
      instrument_type = i;
      break;
    }
  }

  if (instrument_type == -1)
  {
    send_to_char(ch, "Unknown instrument. Available instruments: ");
    for (i = 0; i < MAX_INSTRUMENTS; i++)
    {
      char available_lower[MAX_INPUT_LENGTH] = {'\0'};
      snprintf(available_lower, sizeof(available_lower), "%s", instrument_names[i]);
      for (j = 0; available_lower[j]; j++)
        available_lower[j] = LOWER(available_lower[j]);
      send_to_char(ch, "%s%s", available_lower, (i < MAX_INSTRUMENTS - 1) ? ", " : ".\r\n");
    }
    return;
  }

  /* Lowercase representation for the summoned instrument */
  snprintf(instrument_lower, sizeof(instrument_lower), "%s", instrument_names[instrument_type]);
  for (i = 0; instrument_lower[i]; i++)
    instrument_lower[i] = LOWER(instrument_lower[i]);

  /* Create the instrument object */
  instrument = create_obj();

  /* Set basic object type and values */
  GET_OBJ_TYPE(instrument) = ITEM_INSTRUMENT;
  GET_OBJ_VAL(instrument, INSTRUMENT_VALUE_TYPE) = instrument_type;
  GET_OBJ_VAL(instrument, INSTRUMENT_VALUE_DIFFICULTY_REDUCTION) = 0;
  GET_OBJ_VAL(instrument, INSTRUMENT_VALUE_EFFECTIVENESS) = 0;
  GET_OBJ_VAL(instrument, INSTRUMENT_VALUE_BREAKABILITY) = 10; /* 10 in 11,111 per verse. */

  /* Set object properties */
  GET_OBJ_COST(instrument) = 0;
  GET_OBJ_WEIGHT(instrument) = 1;
  GET_OBJ_RENT(instrument) = 0;

  /* Set keywords, short, and long descriptions */
  instrument->name = strdup(instrument_lower);

  snprintf(short_buf, sizeof(short_buf), "a summoned %s", instrument_lower);
  instrument->short_description = strdup(short_buf);

  snprintf(long_buf, sizeof(long_buf), "A summoned %s lies here, waiting to make music.",
           instrument_lower);
  instrument->description = strdup(long_buf);

  /* Set object flags: NORENT and NOSELL */
  SET_BIT_AR(GET_OBJ_WEAR(instrument), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(instrument), ITEM_WEAR_INSTRUMENT);
  SET_BIT_AR(GET_OBJ_EXTRA(instrument), ITEM_NORENT);
  SET_BIT_AR(GET_OBJ_EXTRA(instrument), ITEM_NOSELL);
  GET_OBJ_BOUND_ID(instrument) = NOBODY;

  /* Give object to caster */
  obj_to_char(instrument, ch);

  /* Send messages */
  send_to_char(ch, "A summoned %s appears in your possession!\r\n", instrument_lower);
  act("$n summons a $o!", FALSE, ch, instrument, 0, TO_ROOM);
}

ASPELL(spell_storm_of_vengeance)
{
  struct mud_event_data *pMudEvent = NULL;

  if (ch == NULL)
    return;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if ((pMudEvent = char_has_mud_event(ch, eICE_STORM)))
  {
    send_to_char(ch, "You already have a storm of vengeance!\r\n");
    return;
  }

  if ((pMudEvent = char_has_mud_event(ch, eCHAIN_LIGHTNING)))
  {
    send_to_char(ch, "You already have a storm of vengeance!\r\n");
    return;
  }

  send_to_char(ch, "You summon a storm of vengeance!\r\n");
  act("$n summons a storm of vengeance!", FALSE, ch, 0, 0, TO_ROOM);

  NEW_EVENT(eICE_STORM, ch, NULL, (6 * PASSES_PER_SEC));
  NEW_EVENT(eCHAIN_LIGHTNING, ch, NULL, (12 * PASSES_PER_SEC));
}

ASPELL(warlock_charm)
{
  if (victim == NULL || ch == NULL)
    return;

  effect_charm(ch, victim, WARLOCK_CHARM, casttype, level);
}

ASPELL(voracious_dispelling)
{
  if (ch == NULL)
    return;
  if (victim == NULL)
    victim = ch;

  perform_dispel(ch, victim, obj, WARLOCK_VORACIOUS_DISPELLING);
}

ASPELL(tenacious_plague)
{
  if (CLOUDKILL(ch) || INCENDIARY(ch) || DOOM(ch))
  {
    send_to_char(ch, "You already have a cloud following you!\r\n");
    return;
  }

  send_to_char(ch, "You summon forth a mass of biting and stinging insects!\r\n");
  act("$n summons forth a mass of biting and stinging insects!", FALSE, ch, 0, 0, TO_ROOM);

  TENACIOUS_PLAGUE(ch) = 3;
}

ASPELL(wall_of_perilous_flame)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int dir = -1;

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  one_argument(cast_arg2, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "You must specify a direction to conjure your wall at.\r\n");
    return;
  }

  dir = search_block(arg, dirs, FALSE);
  if (dir >= 0)
  {
    create_wall(ch, ch->in_room, dir, WALL_TYPE_PERILOUS_FIRE, GET_WARLOCK_LEVEL(ch));
  }
  else
    send_to_char(ch, "You must specify a direction to conjure your wall at.\r\n");
}

ASPELL(eldritch_blast)
{
  if (ch == NULL || victim == NULL)
    return;

  int attack_result = 0;
  int effective_level = spell_info[WARLOCK_ELDRITCH_BLAST].effective_level;
  if (GET_ELDRITCH_SHAPE(ch) != -1)
    effective_level = MAX(effective_level, spell_info[GET_ELDRITCH_SHAPE(ch)].effective_level);
  if (GET_ELDRITCH_ESSENCE(ch) != -1)
    effective_level = MAX(effective_level, spell_info[GET_ELDRITCH_ESSENCE(ch)].effective_level);

  PREREQ_CAN_FIGHT();
  PREREQ_IN_POSITION(POS_SITTING, "You must be on your feet to cast this.\r\n");
  PREREQ_NOT_PEACEFUL_ROOM();

  /* dynamic memory allocation required */
  struct list_data *target_list = NULL;
  struct char_data *tch = NULL, *next_tch = NULL;

  if (GET_ELDRITCH_SHAPE(ch) == WARLOCK_ELDRITCH_CHAIN ||
      GET_ELDRITCH_SHAPE(ch) == WARLOCK_ELDRITCH_DOOM)
  {
    /* we need to build a list of possible targets */
    target_list = create_list();
    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;
      if (!aoeOK(ch, tch, WARLOCK_ELDRITCH_BLAST))
        continue;
      else if (GET_ELDRITCH_SHAPE(ch) == WARLOCK_ELDRITCH_CHAIN &&
               target_list->iSize >= (size_t)(GET_WARLOCK_LEVEL(ch) / 5))
        continue;
      add_to_list(tch, target_list);
    }
  }

  // Check to see if the spell should miss
  if (GET_ELDRITCH_SHAPE(ch) == WARLOCK_ELDRITCH_DOOM)
  {
    act("$n sends out an explosive blast of eldritch energy.", FALSE, ch, 0, tch, TO_ROOM);
    act("You release an explosive blast of eldritch energy into the area.", FALSE, ch, 0, tch,
        TO_CHAR);
    add_to_list(victim, target_list);
    while (target_list->iSize > 0)
    {
      // Remove a target from the list.
      tch = random_from_list(target_list);
      remove_from_list(tch, target_list);
      // Do the spell effects.
      mag_damage(effective_level, ch, tch, NULL, WARLOCK_ELDRITCH_DOOM, 0, SAVING_REFL,
                 CAST_INNATE);
      mag_affects(effective_level, ch, tch, NULL, WARLOCK_ELDRITCH_DOOM, -1, CAST_INNATE, 0);
      act("You're hit with a wave of eldritch energy from $n.", FALSE, ch, 0, tch, TO_VICT);
    }
  }
  else if (GET_ELDRITCH_SHAPE(ch) == WARLOCK_ELDRITCH_CONE)
  {
    act("$n sends out a cone of eldritch energy.", FALSE, ch, 0, tch, TO_NOTVICT);
    act("You release a cone of eldritch energy into the area.", FALSE, ch, 0, tch, TO_CHAR);
    mag_damage(effective_level, ch, victim, NULL, WARLOCK_ELDRITCH_CONE, 0, SAVING_REFL,
               CAST_INNATE);
    mag_affects(effective_level, ch, victim, NULL, WARLOCK_ELDRITCH_CONE, -1, CAST_INNATE, 0);
  }
  else if (!(attack_result =
                 attack_roll_with_critical(ch, victim, ATTACK_TYPE_ELDRITCH_BLAST, TRUE, 0, 20)))
  {
    act("You send a blast of energy towards $E, but $E avoids it.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n sends out a blast of energy towards you, but you avoid it.", FALSE, ch, 0, victim,
        TO_VICT);
    act("$n sends out a blast of energy towards $E, but $E avoids it.", FALSE, ch, 0, victim,
        TO_NOTVICT);
    free_list(target_list);
    return;
  }
  else if (GET_ELDRITCH_SHAPE(ch) != WARLOCK_HIDEOUS_BLOW)
  {
    const bool is_critical = attack_result == 999;
    if (is_critical)
    {
      mag_damage(effective_level, ch, victim, NULL, WARLOCK_CRITICAL_ELDRITCH_BLAST, 0, -1,
                 CAST_INNATE);
      mag_affects(effective_level, ch, victim, NULL, WARLOCK_ELDRITCH_BLAST, -1, CAST_INNATE, 0);
    }
    else
    {
      mag_damage(effective_level, ch, victim, NULL, WARLOCK_ELDRITCH_BLAST, 0, -1, CAST_INNATE);
      mag_affects(effective_level, ch, victim, NULL, WARLOCK_ELDRITCH_BLAST, -1, CAST_INNATE, 0);
    }
    if (GET_ELDRITCH_SHAPE(ch) == WARLOCK_ELDRITCH_CHAIN)
    {
      while (target_list->iSize > 0)
      {
        // Remove a target from the list.
        tch = random_from_list(target_list);
        remove_from_list(tch, target_list);
        // Do the spell effects
        mag_damage(effective_level, ch, tch, NULL, WARLOCK_ELDRITCH_CHAIN, 0, -1, CAST_INNATE);
        mag_affects(effective_level, ch, tch, NULL, WARLOCK_ELDRITCH_CHAIN, -1, CAST_INNATE, 0);
        act("An eldritch arc of energy jumps to you from $N's blast.", FALSE, ch, 0, tch, TO_VICT);
        act("An eldritch arc of energy jumps to $E from $N's blast.", FALSE, ch, 0, tch,
            TO_NOTVICT);
        act("An eldritch arc of energy jumps to $E from your blast.", FALSE, ch, 0, tch, TO_CHAR);
      }
    }
  }
  else if (GET_ELDRITCH_SHAPE(ch) == WARLOCK_HIDEOUS_BLOW)
  {
    const bool is_critical = attack_result == 999;
    if (is_critical)
    {
      mag_damage(effective_level, ch, victim, NULL, WARLOCK_CRITICAL_ELDRITCH_BLAST, 0, -1,
                 CAST_INNATE);
      mag_affects(effective_level, ch, victim, NULL, WARLOCK_ELDRITCH_BLAST, -1, CAST_INNATE, 0);
    }
    else
    {
      mag_damage(effective_level, ch, victim, NULL, WARLOCK_ELDRITCH_BLAST, 0, -1, CAST_INNATE);
      mag_affects(effective_level, ch, victim, NULL, WARLOCK_ELDRITCH_BLAST, -1, CAST_INNATE, 0);
    }
  }
  if (target_list != NULL)
    free_list(target_list);
}

ASPELL(spell_summon)
{
  bool is_mission_mob(struct char_data * ch, struct char_data * mob);

  if (ch == NULL || victim == NULL)
    return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3))
  {
    send_to_char(ch, "(level) %s", SUMMON_FAIL);
    return;
  }

  if (!valid_mortal_tele_dest(victim, IN_ROOM(ch), TRUE))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  if (!valid_mortal_tele_dest(ch, IN_ROOM(victim), TRUE))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOSUMMON) || ROOM_FLAGGED(IN_ROOM(victim), ROOM_NOSUMMON))
  {
    send_to_char(ch, "(no-summon room) %s", SUMMON_FAIL);
    return;
  }

  if (!CONFIG_PK_ALLOWED)
  {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE))
    {
      act("As the words escape your lips and $N travels\r\n"
          "through time and space towards you, you realize that $E is\r\n"
          "aggressive and might harm you, so you wisely send $M back.",
          FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) && !PLR_FLAGGED(victim, PLR_KILLER))
    {
      send_to_char(victim,
                   "%s just tried to summon you to: %s.\r\n"
                   "%s failed because you have summon protection on.\r\n"
                   "Type NOSUMMON to allow other players to summon you.\r\n",
                   GET_NAME(ch), world[IN_ROOM(ch)].name,
                   (ch->player.sex == SEX_MALE) ? "He" : "She");

      send_to_char(ch, "You failed because %s has summon protection on.\r\n", GET_NAME(victim));
      mudlog(BRF, LVL_IMMORT, TRUE, "%s failed summoning %s to %s.", GET_NAME(ch), GET_NAME(victim),
             world[IN_ROOM(ch)].name);
      return;
    }
  }

  if (MOB_FLAGGED(victim, MOB_NOSUMMON) || char_has_worn_object_flag(victim, ITEM_ROL_NO_SUMMON))
  {
    send_to_char(ch, "Your victim seems unsummonable.");
    return;
  }

  if (AFF_FLAGGED(victim, AFF_NOTELEPORT))
  {
    send_to_char(ch, "Your traget seems to be affected by teleport protection!\r\n");
    return;
  }

  if (IS_POWERFUL_BEING(victim))
  {
    send_to_char(ch, "Summon failed!  The target is a powerful being and easily dismisses your "
                     "annoying magic!\r\n");
    return;
  }

  if (IS_NPC(victim) && !is_mission_mob(ch, victim))
  {
    send_to_char(ch, "That target belongs to another player.\r\n");
    return;
  }

  if (mag_resistance(ch, victim, 0))
    return;
  if (IS_NPC(victim) && savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, CONJURATION))
  {
    send_to_char(ch, "%s", SUMMON_FAIL);
    return;
  }

  act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

  char_from_room(victim);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
  {
    X_LOC(victim) = world[IN_ROOM(ch)].coords[0];
    Y_LOC(victim) = world[IN_ROOM(ch)].coords[1];
  }
  char_to_room_cause(victim, IN_ROOM(ch), ch, DOMAIN_RELOCATION_TELEPORT, -1);

  act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

ASPELL(spell_gird_allies)
{
  struct char_data *pet = NULL;
  struct affected_type af;

  if (IN_ROOM(ch) == NOWHERE)
    return;

  send_to_char(ch, "You weave a protective shell around your conjured allies.\r\n");

  for (pet = world[IN_ROOM(ch)].people; pet; pet = pet->next_in_room)
  {
    if (IS_PET(pet) && GROUP(pet->master) == GROUP(ch))
    {
      act("You have been protected by $n's might.", FALSE, ch, 0, pet, TO_VICT);
      act("$n has been protected by $N's might.", FALSE, pet, 0, ch, TO_ROOM);
      new_affect(&af);
      af.spell = SPELL_GIRD_ALLIES;
      af.duration = 10 * (level / 2);
      af.location = APPLY_AC_NEW;
      af.modifier = 1 + (level / 6);
      af.bonus_type = BONUS_TYPE_DEFLECTION;
      affect_join(pet, &af, FALSE, FALSE, FALSE, FALSE);
    }
  }
}

ASPELL(spell_teleport)
{
  room_rnum to_room = NOWHERE;

  if (ch == NULL)
    return;

  if (!victim)
  {
    victim = ch;
  }

  if (AFF_FLAGGED(victim, AFF_NOTELEPORT))
  {
    send_to_char(ch, "Your spell fails to target that victim!\r\n");
    return;
  }

  if (MOB_FLAGGED(victim, MOB_NOTELEPORT))
  {
    send_to_char(ch, "The teleportation magic while beginning to form, flashes brightly, then dies "
                     "suddenly!\r\n");
    return;
  }

  if (IS_POWERFUL_BEING(victim))
  {
    send_to_char(ch, "Teleport failed!  The target is a powerful being and easily dismisses your "
                     "magic from the other side!\r\n");
    return;
  }

  to_room = IN_ROOM(victim);

  if (!valid_mortal_tele_dest(ch, to_room, TRUE))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  if (!valid_mortal_tele_dest(ch, IN_ROOM(ch), TRUE))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  /* no teleporting on the outter planes */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE))
  {
    send_to_char(ch, "This magic won't help you travel on this plane!\r\n");
    return;
  }

  /* no teleporting off the prime plane to another */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ELEMENTAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ETH_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ASTRAL_PLANE))
  {
    send_to_char(ch, "Your target is beyond the reach of your magic!\r\n");
    return;
  }

  send_to_char(ch, "You slowly fade out of existence...\r\n");
  act("$n slowly fades out of existence and is gone.", FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[to_room].coords[0];
    Y_LOC(ch) = world[to_room].coords[1];
  }
  char_to_room_cause(ch, to_room, ch, DOMAIN_RELOCATION_TELEPORT, -1);

  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "You slowly fade back into existence...\r\n");
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

ASPELL(spell_shadow_jump)
{
  room_rnum to_room = NOWHERE;

  if (ch == NULL)
    return;

  if (!victim)
  {
    victim = ch;
  }

  if (AFF_FLAGGED(victim, AFF_NOTELEPORT))
  {
    send_to_char(ch, "Your spell fails to target that victim!\r\n");
    return;
  }

  to_room = IN_ROOM(victim);

  if (!valid_mortal_tele_dest(ch, to_room, TRUE))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  if (!valid_mortal_tele_dest(ch, IN_ROOM(ch), TRUE))
  {
    send_to_char(ch, "A bright flash prevents your spell from working!");
    return;
  }

  /* no teleporting on the outter planes */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE))
  {
    send_to_char(ch, "This magic won't help you travel on this plane!\r\n");
    return;
  }

  /* no teleporting off the prime plane to another */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ELEMENTAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ETH_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ASTRAL_PLANE))
  {
    send_to_char(ch, "Your target is beyond the reach of your magic!\r\n");
    return;
  }

  if ((OUTSIDE(victim) || OUTSIDE(ch)) && weather_info.sunlight != SUN_DARK)
  {
    send_to_char(ch, "It must be night time for you to shadow jump.\r\n");
    return;
  }

  if (IS_SHADOW_CONDITIONS(ch) && IS_SHADOW_CONDITIONS(victim))
  {
    send_to_char(ch,
                 "Either your current or target room is too bright to perform a shadow jump.\r\n");
    return;
  }

  send_to_char(ch, "You slowly fade into the shadows...\r\n");
  act("$n slowly fades into the shadows and is gone.", FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[to_room].coords[0];
    Y_LOC(ch) = world[to_room].coords[1];
  }
  char_to_room_cause(ch, to_room, ch, DOMAIN_RELOCATION_TELEPORT, -1);

  act("$n slowly fades in from the shadows.", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "You slowly fade back in from the shadows...\r\n");
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

ASPELL(psionic_psychoportation)
{
  room_rnum to_room = NOWHERE;

  if (ch == NULL)
    return;

  if (!victim)
  {
    victim = ch;
  }

  if (AFF_FLAGGED(victim, AFF_NOTELEPORT))
  {
    send_to_char(ch, "Your manifestation fails to target that victim!\r\n");
    return;
  }

  to_room = IN_ROOM(victim);

  if (!valid_mortal_tele_dest(ch, to_room, TRUE))
  {
    send_to_char(ch, "A bright flash prevents your manifestation from working!");
    return;
  }

  if (!valid_mortal_tele_dest(ch, IN_ROOM(ch), TRUE))
  {
    send_to_char(ch, "A bright flash prevents your manifestation from working!");
    return;
  }

  /* no teleporting on the outter planes */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE))
  {
    send_to_char(ch, "This power won't help you travel on this plane!\r\n");
    return;
  }

  /* no teleporting off the prime plane to another */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ELEMENTAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ETH_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_ASTRAL_PLANE))
  {
    send_to_char(ch, "Your target is beyond the reach of your power!\r\n");
    return;
  }

  send_to_char(ch, "You slowly fade out of existence...\r\n");
  act("$n slowly fades out of existence and is gone.", FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[to_room].coords[0];
    Y_LOC(ch) = world[to_room].coords[1];
  }
  char_to_room_cause(ch, to_room, ch, DOMAIN_RELOCATION_TELEPORT, -1);

  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "You slowly fade back into existence...\r\n");
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

/* Object value 3 on corpse is just a marker that its a corpse (2 for PC corpse)
   Object value 4 on corpse is the ID number of the player  */
ASPELL(spell_resurrect)
{
  struct char_data *ressed = NULL, *vict = NULL, *next_v = NULL;
  struct descriptor_data *d = NULL, *next_d = NULL;
  int exp = 0, gain = 0;

  if (ch == NULL || obj == NULL)
    return;

  if (obj->in_room == NOWHERE)
    return;

  /* If it is not a pcorpse, then out*/
  if (!IS_CORPSE(obj))
  {
    act("$p is not a player corpse.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  if (!GET_OBJ_VAL(obj, 4))
  {
    act("$p is not a player corpse.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

  /* looking for the player associated with the corpse */
  for (d = descriptor_list; d; d = next_d)
  {
    next_d = d->next;

    if (d && d->character && (STATE(d) == CON_PLAYING) &&
        GET_OBJ_VAL(obj, 4) == GET_IDNUM(d->character))
      ressed = d->character;
  }

  if (ressed == NULL)
  {
    send_to_char(ch, "That char is not online at the moment!\r\n");
    return;
  }

  /* we don't currently have a fail chance */
  /*
  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    if (number(0, 101) > GET_CON(ressed) &&
        number(0, 101) > GET_MOVE(ch) &&
        number(0, 101) > GET_LUCK(ressed))
    {
      send_to_char("You feel as if you are at two places at once!\r\n", ressed);
      send_to_char("Suddenly you return to normal and feel rather disappointed.\r\n", ressed);
      send_to_char("Oops...that did NOT go as planned.!\r\n", ch);
      // okies.. we failed ress, time to mark the corpse as unressable
      GET_OBJ_VAL(obj, 2) = 1;
      return;
    }
  }
  */

  /* At this point the character is resurrected (nothing stopping us) */
  act("You howl in pain as your body is ripped to shreds.", FALSE, ressed, obj, 0, TO_CHAR);
  act("$n howls in pain as his body is ripped to shreds!", FALSE, ressed, obj, 0, TO_ROOM);
  act("\tW$N\tn\tW's body seems to \tn\tcsh\tn\tCimm\tn\twer \tWsuddenly, then crumbles into "
      "\tn\tydust.\tn\n",
      TRUE, ressed, obj, ressed, TO_NOTVICT);
  act("\tWYour body seems to \tn\tcsh\tn\tCimm\tn\twer \tWsuddenly, then crumbles into "
      "\tn\tydust.\tn\n",
      TRUE, ch, obj, ressed, TO_VICT);

  /* here is the stored xp and 10% penalty on that */
  exp = -GET_OBJ_VAL(
      obj,
      5); /* this will be negative, so we are swapping it since we want to -gain- this xp back */
  if (GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(ressed) < LVL_IMMORT)
  {
    exp /= 10;
    exp *= 9;
  }

  /* Drop all stuffs on ground */
  /* we don't do this currently, corpses are empty and player should already have all his gear */
  /*
  dump_eq_to_room(ressed);
  */

  /* more unused code */
  /*
     for (tobj = ressed->carrying; tobj; tobj = next_obj) {
        next_obj = tobj->next_content;
        obj_to_room(tobj, ressed->in_room);
     }

     for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ressed, i))
           obj_to_room(unequip_char(ressed, i), ressed->in_room );
  */

  /* stop combat */
  /* stop vanishers combat */
  if (char_has_mud_event(ressed, eCOMBAT_ROUND))
  {
    event_cancel_specific(ressed, eCOMBAT_ROUND);
  }
  stop_fighting(ressed);

  /* stop all those who are fighting vanisher */
  for (vict = world[IN_ROOM(ressed)].people; vict; vict = next_v)
  {
    next_v = vict->next_in_room;

    if (vict && FIGHTING(vict) == ressed)
    {
      if (char_has_mud_event(vict, eCOMBAT_ROUND))
      {
        event_cancel_specific(vict, eCOMBAT_ROUND);
      }

      if (vict)
        stop_fighting(vict);
    }

    if (vict && IS_NPC(vict))
      clearMemory(vict);
  }

  /* relocate ress-target! */
  char_from_room(ressed);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(obj->in_room), ZONE_WILDERNESS))
  {
    X_LOC(ressed) = world[obj->in_room].coords[0];
    Y_LOC(ressed) = world[obj->in_room].coords[1];
  }
  char_to_room_cause(ressed, obj->in_room, ch, DOMAIN_RELOCATION_TELEPORT, -1);

  /* more unused code */
  /*
  for (tobj = obj->contains; tobj; tobj = next_obj)
  {
    next_obj = tobj->next_content;
    obj_from_obj(tobj);
    obj_to_char(tobj, ressed);
    get_check_money(ressed, tobj);
  }
  */

  /* extra "cost" for ress */
  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    GET_MOVE(ch) = 0; // exhausted
    WAIT_STATE(ch, 12 RL_SEC);
    USE_FULL_ROUND_ACTION(ch);
    USE_SWIFT_ACTION(ch);
  }

  if (GET_LEVEL(ressed) < LVL_IMMORT)
  {
    GET_MOVE(ressed) = 0; // exhausted
    WAIT_STATE(ressed, PULSE_VIOLENCE * 1);
    USE_FULL_ROUND_ACTION(ressed);
    USE_SWIFT_ACTION(ressed);
  }
  /* end cost */

  /* get XP back! */
  if (exp <= 0)
    exp = 1;
  gain = gain_exp_regardless(ressed, exp, TRUE);

  act("\twYou complete your chant, and stand humbled before the might of\n"
      "your \tn\tWdeity.\tn\tw Your vision swims as you see your deity's \tYdivine\n"
      "\tYhand \tn\twreaching down to touch $N and $S \tn\tLremains. \tn\tw$N\tn\tw's\n"
      "\tn\tcsoul \tn\twis guided out of its current vessel and gently deposited\n"
      "\twinto $S \tn\tLremains. \tn\twThe empty \tn\tLcarcass \tn\twcrumbles into dust\n"
      "\tn\twas your \tn\tWdeity \tn\twwithdraws their touch, leaving you exhausted.\tn\n",
      TRUE, ch, obj, ressed, TO_CHAR);

  act("\twYou feel a \tn\tWPresence \tn\twtouch you, its divine hand cupping itself\n"
      "\twaround your \tn\tcsoul \tn\twand drawing it forth from your current body. For\n"
      "\twone brief instant, you witness the enormity of the \tn\tLuni\tn\tCve\tn\tcrse\tn\tw "
      "before\n"
      "\twyour \tn\tcsoul \tn\twis gently deposited into your previous body, at the feet of $n.\tn",
      TRUE, ch, obj, ressed, TO_VICT);

  act("\tw$n\tn\tw completes $s chant, and stares \tn\tYrapturously \tn\twinto space.\n"
      "\tw$s body seems to \tn\tcsh\tn\tCimm\tn\twer \tn\twas a great Power enters the room, and\n"
      "\twa divine \tn\tYradiance \tn\twengulfs the body at $n's feet. After a\n"
      "\twbrief moment, the body \tn\tLtwitches \tn\twand convulses, before the eyes snap\n"
      "\twopen and $N takes a deep breath. The \tn\tYradiance \tn\twdissipates, leaving\n"
      "\tw$n standing disoriented and exhausted.\tn",
      TRUE, ch, obj, ressed, TO_NOTVICT);

  send_to_char(ressed, "You feel extremely tired after beeing resurrected!\r\n");
  act("$n has been resurrected by $N!", FALSE, ressed, obj, ch, TO_NOTVICT);
  act("You have resurrected $n!", FALSE, ressed, obj, ch, TO_VICT);

  /* remove corpse */
  extract_obj(obj);

  save_char(ressed, 0);
  look_at_room(ressed, 0);
  send_to_char(ressed, "You have regained %d exp back from the resurrection!\r\n", gain);
}

ASPELL(spell_transport_via_plants)
{
  obj_vnum obj_num = NOTHING;
  room_rnum to_room = NOWHERE;
  struct obj_data *dest_obj = NULL, *tmp_obj = NULL;

  if (ch == NULL)
    return;

  if (!obj)
  {
    send_to_char(ch, "Your target does not exist!\r\n");
    return;
  }
  else if (GET_OBJ_TYPE(obj) != ITEM_PLANT)
  {
    send_to_char(ch, "That is not a plant!\r\n");
    return;
  }
  else if (GET_OBJ_SIZE(obj) < SIZE_MEDIUM)
  {
    send_to_char(ch, "That plant is not large enough to transport you.\r\n");
    return;
  }
  obj_num = GET_OBJ_VNUM(obj);

  // find another of that plant in the world
  for (tmp_obj = object_list; tmp_obj; tmp_obj = tmp_obj->next)
  {
    if (tmp_obj == obj)
      continue;

    // we don't want to transport to a plant in someone's inventory
    if (GET_OBJ_VNUM(tmp_obj) == obj_num && !tmp_obj->carried_by)
    {
      dest_obj = tmp_obj;

      // 5% chance we will just stop at this obj
      if (!rand_number(0, 10))
        break;
    }
  }

  act("$n walks toward $p, and steps inside of it.", FALSE, ch, obj, 0, TO_ROOM);
  act("You walk toward $p, and step inside of it.", FALSE, ch, obj, 0, TO_CHAR);

  if (dest_obj != NULL)
  {
    to_room = dest_obj->in_room;
  }

  if (to_room == NOWHERE)
  {
    send_to_char(ch, "You are unable to find another exit, and are ejected from the plant.\r\n");
    act("$n comes tumbling out from inside of $p.", FALSE, ch, obj, 0, TO_ROOM);
    return;
  }
  else
  {
    if (!valid_mortal_tele_dest(ch, to_room, TRUE))
    {
      send_to_char(ch, "A bright flash prevents your spell from working!\r\n");
      act("$n comes tumbling out from inside of $p.", FALSE, ch, obj, 0, TO_ROOM);
      return;
    }

    // transport player to new location
    char_from_room(ch);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(to_room), ZONE_WILDERNESS))
    {
      X_LOC(ch) = world[to_room].coords[0];
      Y_LOC(ch) = world[to_room].coords[1];
    }
    char_to_room_cause(ch, to_room, ch, DOMAIN_RELOCATION_TELEPORT, -1);

    look_at_room(ch, 0);
    act("You find your destination, and step out through $p.", FALSE, ch, dest_obj, 0, TO_CHAR);
    act("$n steps out from inside of $p!", FALSE, ch, dest_obj, 0, TO_ROOM);
    // TODO: make this an event, so player enters into the plant, and sees a couple messages, then comes out the other side
  }
}

ASPELL(spell_wall_of_thorns)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int dir = -1;

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  one_argument(cast_arg2, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "You must specify a direction to conjure your wall at.\r\n");
    return;
  }

  dir = search_block(arg, dirs, FALSE);
  if (dir >= 0)
  {
    create_wall(ch, ch->in_room, dir, WALL_TYPE_THORNS, GET_LEVEL(ch));
  }
  else
    send_to_char(ch, "You must specify a direction to conjure your wall at.\r\n");
}

ASPELL(spell_wall_of_fire)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int dir = -1;

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  one_argument(cast_arg2, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "You must specify a direction to conjure your wall at.\r\n");
    return;
  }

  dir = search_block(arg, dirs, FALSE);
  if (dir >= 0)
  {
    create_wall(ch, ch->in_room, dir, WALL_TYPE_FIRE, GET_LEVEL(ch));
  }
  else
    send_to_char(ch, "You must specify a direction to conjure your wall at.\r\n");
}

ASPELL(spell_wall_of_force)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int dir = -1;

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  one_argument(cast_arg2, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "You must specify a direction to conjure your wall at.\r\n");
    return;
  }

  dir = search_block(arg, dirs, FALSE);
  if (dir >= 0)
  {
    create_wall(ch, ch->in_room, dir, WALL_TYPE_FORCE, GET_LEVEL(ch));
  }
  else
    send_to_char(ch, "You must specify a direction to conjure your wall at.\r\n");

  /* old wall of force */
  /*
  struct char_data *mob;
   *
  if (!(mob = read_mobile_reason(WALL_OF_FORCE, VIRTUAL, PERF_ENTITY_SPELL_SUMMON))) {
    send_to_char(ch, "You don't quite remember how to create that.\r\n");
    return;
  }

  char_to_room_cause(mob, IN_ROOM(ch), ch, DOMAIN_RELOCATION_SPAWN, -1);
  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;

  act("$n conjures $N!", FALSE, ch, 0, mob, TO_ROOM);
  send_to_char(ch, "You conjure a wall of force!\r\n");

  load_mtrigger(mob);
   */
}

ASPELL(psionic_wall_of_ectoplasm)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int dir = -1;

  if (AFF_FLAGGED(ch, AFF_CHARM))
    return;

  one_argument(cast_arg2, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "You must specify a direction to conjure your wall at.\r\n");
    return;
  }

  dir = search_block(arg, dirs, FALSE);
  if (dir >= 0)
  {
    create_wall(ch, ch->in_room, dir, WALL_TYPE_ECTOPLASM, GET_LEVEL(ch));
  }
  else
    send_to_char(ch, "You must specify a direction to conjure your wall at.\r\n");
}

ASPELL(spell_wizard_eye)
{
  struct char_data *eye = read_mobile_reason(WIZARD_EYE, VIRTUAL, PERF_ENTITY_SPELL_SUMMON);

  // dummy check
  if (!eye)
  {
    send_to_char(ch, "You don't quite remember how to create that.\r\n");
    return;
  }
  if (!ch || !ch->desc)
  {
    send_to_char(ch, "You don't quite remember how to create that.\r\n");
    return;
  }

  // first load the eye
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
  {
    X_LOC(eye) = world[IN_ROOM(ch)].coords[0];
    Y_LOC(eye) = world[IN_ROOM(ch)].coords[1];
  }
  char_to_room_cause(eye, IN_ROOM(ch), ch, DOMAIN_RELOCATION_SPAWN, -1);
  IS_CARRYING_W(eye) = 0;
  IS_CARRYING_N(eye) = 0;
  load_mtrigger(eye);

  // now take control
  send_to_char(ch, "You summon a wizard eye! (\tDType 'return' to return to your body\tn)\r\n");
  ch->desc->character = eye;
  ch->desc->original = ch;
  eye->desc = ch->desc;
  ch->desc = NULL;
  character_periodic_sync(ch);
  character_periodic_sync(eye);
}

ASPELL(psionic_concussive_onslaught)
{
  int x = 0;

  if (ch == NULL)
    return;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  send_to_char(ch, "You blast out wave after wave of concussive kinetic energy!\r\n");
  act("$n blasts out wave after wave of concussive kinetic energy!", FALSE, ch, 0, 0, TO_ROOM);

  // we need to correct the psp cost below, because the power only benefits from 2 augment points at a time.
  ch->player_specials->dam_co_holder_ndice = 3 + (GET_AUGMENT_PSP(ch) / 2);
  ch->player_specials->dam_co_holder_sdice = 6;
  ch->player_specials->save_co_holder_dc_bonus = GET_AUGMENT_PSP(ch) / 2;

  for (x = 0; x < GET_PSIONIC_LEVEL(ch); x++)
  {
    NEW_EVENT(eCONCUSSIVEONSLAUGHT, ch, NULL, ((x * 6) * PASSES_PER_SEC));
  }
  ch->player_specials->concussive_onslaught_duration = GET_PSIONIC_LEVEL(ch);
}

/* The "return" of the event function is the time until the event is called
 * again. If we return 0, then the event is freed and removed from the list, but
 * any other numerical response will be the delay until the next call */
MUD_EVENT_CALLBACK(event_concussive_onslaught)
{
  struct char_data *ch, *victim = NULL;
  struct mud_event_data *pMudEvent;
  int casttype = CAST_SPELL;
  int level = 0;

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  if (ch == NULL)
    return 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return 0;
  }

  level = GET_PSIONIC_LEVEL(ch);
  int ndice = ch->player_specials->dam_co_holder_ndice;
  int sdice = ch->player_specials->dam_co_holder_sdice;

  for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room)
  {
    if (!aoeOK(ch, victim, PSIONIC_CONCUSSIVE_ONSLAUGHT))
      continue;
    if (power_resistance(ch, victim, 0))
      continue;
    GET_DC_BONUS(ch) = ch->player_specials->save_co_holder_dc_bonus;
    if (savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, EVOCATION))
      damage(ch, victim, (dice(ndice, sdice) / 2), PSIONIC_CONCUSSIVE_ONSLAUGHT, DAM_FORCE, FALSE);
    else
      damage(ch, victim, dice(ndice, sdice), PSIONIC_CONCUSSIVE_ONSLAUGHT, DAM_FORCE, FALSE);
    update_pos(victim);
  }

  return 0;
}

MUD_EVENT_CALLBACK(event_power_leech)
{
  struct char_data *ch, *victim = NULL;
  struct mud_event_data *pMudEvent;
  int casttype = CAST_SPELL;
  int level = 0;

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  if (ch == NULL)
    return 0;

  if (ch && FIGHTING(ch)) // assign victim, if none escape
    victim = FIGHTING(ch);
  else
    return 0;
  if (GET_PSP(victim) > 0 && GET_PSP(ch) < GET_MAX_PSP(ch))
  {
    if (is_immune_mind_affecting(ch, victim, 0))
      return 0;
    if (power_resistance(ch, victim, get_psionic_piercing_will_bonus(ch)))
      return 0;
    if (savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NOSCHOOL))
      return 0;

    GET_PSP(victim) -= dice(1, 4);
    GET_PSP(victim) = MAX(0, GET_PSP(victim));
    GET_PSP(ch) = MIN(GET_MAX_PSP(ch), GET_PSP(ch) + 1);

    act("You drain some of $N's psychic power.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n drains some of YOUR psychic power.", FALSE, ch, 0, victim, TO_VICT);
    act("$n drains some of $N's psychic power.", FALSE, ch, 0, victim, TO_NOTVICT);
  }

  return 0;
}

#define ZOCMD zone_table[zrnum].cmd[subcmd]

// static void list_zone_commands_room(struct char_data *ch, room_vnum rvnum) {
ASPELL(spell_augury)
{
  if (IN_ROOM(ch) == NOWHERE)
    return;

  zone_rnum zrnum = real_zone_by_thing(world[IN_ROOM(ch)].number);
  room_rnum rrnum = IN_ROOM(ch), cmd_room = NOWHERE;
  int subcmd = 0, count = 0;

  if (zrnum == NOWHERE || rrnum == NOWHERE)
  {
    send_to_char(ch, "Your spell cannot divine anything about this area.\r\n");
    return;
  }

  get_char_colors(ch);

  send_to_char(ch, "Your spell reveals the following about this area:%s\r\n", yel);
  while (ZOCMD.command != 'S')
  {
    switch (ZOCMD.command)
    {
    case 'M':
    case 'O':
    case 'T':
    case 'V':
      cmd_room = ZOCMD.arg3;
      break;
    case 'D':
    case 'R':
      cmd_room = ZOCMD.arg1;
      break;
    default:
      break;
    }
    if (cmd_room == rrnum)
    {
      count++;
      /* start listing */
      switch (ZOCMD.command)
      {
      case 'I':
        send_to_char(ch, "%sMay have random treasure (%d%%)", ZOCMD.if_flag ? " then " : "",
                     ZOCMD.arg1);
        break;
      case 'L':
        send_to_char(ch, "%sMay have random treasure in %s [%s%d%s] (%d%%)",
                     ZOCMD.if_flag ? " then " : "", obj_proto[ZOCMD.arg1].short_description, cyn,
                     obj_index[ZOCMD.arg1].vnum, yel, ZOCMD.arg2);
        break;
      case 'M':
        send_to_char(ch, "%s%s may be found here.\r\n", ZOCMD.if_flag ? " then " : "",
                     mob_proto[ZOCMD.arg1].player.short_descr);
        break;
      case 'G':
        send_to_char(ch, "%sthey may possess %s [%s%d%s].\r\n", ZOCMD.if_flag ? " then " : "",
                     obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel);
        break;
      case 'O':
        send_to_char(ch, "%s%s may be found here. [%s%d%s]\r\n", ZOCMD.if_flag ? " then " : "",
                     obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel);
        break;
      case 'E':
        send_to_char(ch, "%s they may equip %s  [%s%d%s].\r\n", ZOCMD.if_flag ? " then " : "",
                     obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel);
        break;
      case 'P':
        send_to_char(ch, "%s%s [%s%d%s] may be inside %s.\r\n", ZOCMD.if_flag ? " then " : "",
                     obj_proto[ZOCMD.arg1].short_description, cyn, obj_index[ZOCMD.arg1].vnum, yel,
                     obj_proto[ZOCMD.arg3].short_description);
        break;
      default:
        break;
      }
    }
    subcmd++;
  }
  send_to_char(ch, "%s", nrm);
  if (!count)
    send_to_char(ch, "Your spell reveals nothing about this area.\r\n");
}

/* The "return" of the event function is the time until the event is called
 * again. If we return 0, then the event is freed and removed from the list, but
 * any other numerical response will be the delay until the next call */
MUD_EVENT_CALLBACK(event_spiritual_weapon)
{
  struct char_data *ch, *victim = NULL;
  struct mud_event_data *pMudEvent;
  int level = 0;

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  if (ch && FIGHTING(ch)) // assign victim, if none escape
    victim = FIGHTING(ch);
  else
    return 0;

  if (ch == NULL || victim == NULL)
    return 0;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return 0;
  }

  if (mag_resistance(ch, victim, 0))
    return 0;

  /* how about wands and everything else?? */
  level = DIVINE_LEVEL(ch);
  if (level < 1)
    level = 15; /* so lame */

  int roll = dice(1, 20);
  int threat = 20 - weapon_list[get_default_spell_weapon(ch)].range;
  bool is_crit = roll >= threat;
  int mult = weapon_list[get_default_spell_weapon(ch)].critMult;
  int attack_roll = MAX(roll, d20(ch)) + GET_BAB(ch) + GET_WIS_BONUS(ch);
  int ac = compute_armor_class(ch, victim, FALSE, MODE_ARMOR_CLASS_NORMAL);
  int dam = dice(weapon_list[get_default_spell_weapon(ch)].numDice,
                 weapon_list[get_default_spell_weapon(ch)].diceSize) +
            MIN(5, CASTER_LEVEL(ch));
  if (is_crit)
  {
    if (mult >= 2)
      dam += dice(weapon_list[get_default_spell_weapon(ch)].numDice,
                  weapon_list[get_default_spell_weapon(ch)].diceSize) +
             MIN(5, CASTER_LEVEL(ch));
    if (mult >= 3)
      dam += dice(weapon_list[get_default_spell_weapon(ch)].numDice,
                  weapon_list[get_default_spell_weapon(ch)].diceSize) +
             MIN(5, CASTER_LEVEL(ch));
  }

  if (attack_roll >= ac)
  {
    damage(ch, victim, dam, SPELL_SPIRITUAL_WEAPON, DAM_FORCE, FALSE);
  }
  else
  {
    damage(ch, victim, 0, SPELL_SPIRITUAL_WEAPON, DAM_FORCE, FALSE);
  }

  update_pos(victim);
  return 0;
}

ASPELL(spell_spiritual_weapon)
{
  struct mud_event_data *pMudEvent = NULL;
  char msg[200];
  int bab = 0;
  int i = 0;

  if (ch == NULL)
    return;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if ((pMudEvent = char_has_mud_event(ch, eSPIRITUALWEAPON)))
  {
    send_to_char(ch, "You already have a spiritual weapon!\r\n");
    return;
  }

  send_to_char(ch, "You summon forth a spiritual %s of force!\r\n",
               weapon_list[get_default_spell_weapon(ch)].name);
  snprintf(msg, sizeof(msg), "$n summons forth a spiritual %s of force!\r\n",
           weapon_list[get_default_spell_weapon(ch)].name);
  act(msg, TRUE, ch, 0, 0, TO_ROOM);
  level = MAX(1, DIVINE_LEVEL(ch));

  bab = BAB(ch);
  for (; bab > 0; bab -= 5)
  {
    for (i = level; i > 0; i--)
      NEW_EVENT(eSPIRITUALWEAPON, ch, NULL, ((i * 6) * PASSES_PER_SEC));
  }
}

/* The "return" of the event function is the time until the event is called
 * again. If we return 0, then the event is freed and removed from the list, but
 * any other numerical response will be the delay until the next call */
MUD_EVENT_CALLBACK(event_dancing_weapon)
{
  struct char_data *ch, *victim = NULL;
  struct mud_event_data *pMudEvent;
  int level = 0;

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;
  if (ch && FIGHTING(ch)) // assign victim, if none escape
    victim = FIGHTING(ch);
  else
    return 0;

  if (ch == NULL || victim == NULL)
    return 0;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return 0;
  }

  if (mag_resistance(ch, victim, 0))
    return 0;

  /* how about wands and everything else?? */
  level = CASTER_LEVEL(ch);
  if (level < 1)
    level = 15; /* so lame */

  int roll = dice(1, 20);
  int threat = 20 - weapon_list[get_default_spell_weapon(ch)].range;
  bool is_crit = roll >= threat;
  int mult = weapon_list[get_default_spell_weapon(ch)].critMult;
  int attack_roll = MAX(roll, d20(ch)) + GET_BAB(ch) + MAX(GET_INT_BONUS(ch), GET_CHA_BONUS(ch));
  int ac = compute_armor_class(ch, victim, FALSE, MODE_ARMOR_CLASS_NORMAL);
  int dam = dice(weapon_list[get_default_spell_weapon(ch)].numDice,
                 weapon_list[get_default_spell_weapon(ch)].diceSize) +
            MIN(5, CASTER_LEVEL(ch));
  if (is_crit)
  {
    if (mult >= 2)
      dam += dice(weapon_list[get_default_spell_weapon(ch)].numDice,
                  weapon_list[get_default_spell_weapon(ch)].diceSize) +
             MIN(5, CASTER_LEVEL(ch));
    if (mult >= 3)
      dam += dice(weapon_list[get_default_spell_weapon(ch)].numDice,
                  weapon_list[get_default_spell_weapon(ch)].diceSize) +
             MIN(5, CASTER_LEVEL(ch));
  }

  if (attack_roll >= ac)
  {
    damage(ch, victim, dam, SPELL_DANCING_WEAPON, DAM_FORCE, FALSE);
  }
  else
  {
    damage(ch, victim, 0, SPELL_DANCING_WEAPON, DAM_FORCE, FALSE);
  }

  update_pos(victim);
  return 0;
}

ASPELL(spell_dancing_weapon)
{
  struct mud_event_data *pMudEvent = NULL;
  char msg[200];
  int bab = 0, i = 0;

  if (ch == NULL)
    return;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if ((pMudEvent = char_has_mud_event(ch, eDANCINGWEAPON)))
  {
    send_to_char(ch, "You already have a dancing weapon!\r\n");
    return;
  }

  send_to_char(ch, "You summon forth a dancing %s of force!\r\n",
               weapon_list[get_default_spell_weapon(ch)].name);
  snprintf(msg, sizeof(msg), "$n summons forth a dancing %s of force!\r\n",
           weapon_list[get_default_spell_weapon(ch)].name);
  act(msg, TRUE, ch, 0, 0, TO_ROOM);
  level = MAX(1, ARCANE_LEVEL(ch));

  bab = BAB(ch);
  for (; bab > 0; bab -= 5)
  {
    for (i = level; i > 0; i--)
      NEW_EVENT(eDANCINGWEAPON, ch, NULL, ((i * 6) * PASSES_PER_SEC));
  }
}

/* The "return" of the event function is the time until the event is called
 * again. If we return 0, then the event is freed and removed from the list, but
 * any other numerical response will be the delay until the next call */
MUD_EVENT_CALLBACK(event_holy_javelin)
{
  struct char_data *ch, *victim = NULL;
  struct mud_event_data *pMudEvent;
  int level = 0;

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;
  if (ch && FIGHTING(ch)) // assign victim, if none escape
    victim = FIGHTING(ch);
  else
    return 0;

  if (ch == NULL || victim == NULL)
    return 0;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return 0;
  }

  /* how about wands and everything else?? */
  level = CASTER_LEVEL(ch);
  if (level < 1)
    level = 15; /* so lame */

  damage(ch, victim, dice(1, 6), SPELL_HOLY_JAVELIN, DAM_HOLY, FALSE);

  update_pos(victim);
  return 0;
}

ASPELL(spell_mass_identify)
{
  int i, k;
  int found = false;
  struct obj_data *item = NULL;
  char buf2[300], bitbuf[300];
  struct char_data *orig = ch;
  ch = victim;

  if (orig == victim)
  {
    act("You cast mass identify on yourself.", TRUE, orig, 0, 0, TO_CHAR);
  }
  else
  {
    act("You cast mass identify on $N.", TRUE, orig, 0, ch, TO_CHAR);
    act("$n casts mass identify on You.", TRUE, orig, 0, ch, TO_VICT);
  }

  send_to_char(ch, "You are using:\r\n");
  for (i = 0; i < NUM_WEARS; i++)
  {
    found = false;
    if (GET_EQ(ch, i))
    {
      item = GET_EQ(ch, i);
      if (OBJ_FLAGGED(item, ITEM_ROL_NO_IDENTIFY) && GET_LEVEL(orig) < LVL_IMMORT)
      {
        send_to_char(ch, "%-30s cannot be identified.\r\n", wear_where[i]);
        continue;
      }
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
      {
        send_to_char(ch, "%-30s", wear_where[i]);

        if (GET_OBJ_TYPE(item) == ITEM_WEAPON || GET_OBJ_TYPE(item) == ITEM_ARMOR)
          send_to_char(ch, " %s Enhancement: %d ",
                       GET_OBJ_TYPE(item) == ITEM_ARMOR
                           ? (CAN_WEAR(item, ITEM_WEAR_SHIELD) ? "Shield" : "Armor")
                           : "Weapon",
                       GET_ENHANCEMENT_BONUS(item));

        for (k = 0; k < MAX_OBJ_AFFECT; k++)
        {
          if ((item->affected[k].location != APPLY_NONE) && (item->affected[k].modifier != 0))
          {
            if (!found)
            {
              found = true;
            }
            sprinttype(item->affected[k].location, apply_types, bitbuf, sizeof(bitbuf));
            switch (item->affected[k].location)
            {
            case APPLY_FEAT:
              snprintf(buf2, sizeof(buf2), " (%s)", feat_list[item->affected[k].modifier].name);
              send_to_char(ch, " %s%s", bitbuf, buf2);
              break;
            default:
              buf2[0] = 0;
              send_to_char(ch, " %s%s %s%d (%s)", bitbuf, buf2,
                           (item->affected[k].modifier > 0) ? "+" : "", item->affected[k].modifier,
                           bonus_types[item->affected[k].bonus_type]);
              break;
            }
          }
        }
        send_to_char(ch, "\r\n");
      }
      else
      {
        send_to_char(ch, "%-30s", wear_where[i]);
        send_to_char(ch, "Something.\r\n");
      }
    }
    else
    {
      if (!GET_EQ(ch, i))
      {
        send_to_char(ch, "%-30s<empty>\r\n", wear_where[i]);
      }
    }
  }
}

ASPELL(spell_holy_javelin)
{
  int x = 0, num_times = 0;

  if (ch == NULL || victim == NULL)
    return;
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (!victim)
  {
    send_to_char(ch, "You need to specify a target for your holy javelin.\r\n");
    return;
  }

  send_to_char(ch, "You hurl a shimmering javelin of pure light!\r\n");
  act("$n hurls a shimmering javelin of pure light!", FALSE, ch, 0, 0, TO_ROOM);

  if (!IS_EVIL(victim))
  {
    act("The javelin hits $N and then dissipates in a spray of harmless yellow sparks.", FALSE, ch,
        0, victim, TO_CHAR);
    act("The javelin hits You and then dissipates in a spray of harmless yellow sparks.", FALSE, ch,
        0, victim, TO_VICT);
    act("The javelin hits $N and then dissipates in a spray of harmless yellow sparks.", FALSE, ch,
        0, victim, TO_NOTVICT);
    return;
  }

  if (attack_roll(ch, victim, ATTACK_TYPE_RANGED, TRUE, 1) < 0)
  {
    act("$N dodges the holy javelin.", FALSE, ch, 0, victim, TO_CHAR);
    act("You dodge the holy javelin.", FALSE, ch, 0, victim, TO_VICT);
    act("$N dodges the holy javelin.", FALSE, ch, 0, victim, TO_ROOM);
    return;
  }

  if (mag_resistance(ch, victim, 0))
  {
    act("$N resists the holy javelin.", FALSE, ch, 0, victim, TO_CHAR);
    act("You resist the holy javelin.", FALSE, ch, 0, victim, TO_VICT);
    act("$N resists the holy javelin.", FALSE, ch, 0, victim, TO_ROOM);
    return;
  }

  appear(victim, TRUE);

  num_times = MIN(5, 1 + (level / 4));

  for (x = 0; x < num_times; x++)
  {
    NEW_EVENT(eHOLYJAVELIN, ch, NULL, ((x * 6) * PASSES_PER_SEC));
  }
}

/* Unassigned spell implementations. */
#define NO_AFFECT_FLAG (-1)

static int spell_effect_duration(int level, int minimum, int divisor)
{
  if (divisor < 1)
    divisor = 1;

  return MAX(minimum, level / divisor);
}

static void apply_spell_effect(struct char_data *victim, int spellnum, int duration, int location,
                               int modifier, int affect_flag, int affect2_flag)
{
  struct affected_type af;

  if (victim == NULL)
    return;

  new_affect(&af);
  af.spell = spellnum;
  af.duration = duration < 0 ? -1 : MAX(1, duration);
  af.location = location;
  af.modifier = modifier;
  if (affect_flag > AFF_DONTUSE)
    SET_BIT_AR(af.bitvector, affect_flag);
  if (affect2_flag > AFF2_DONTUSE)
    SET_BIT_AR(af.bitvector2, affect2_flag);
  affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
}

static bool target_resists_spell(struct char_data *ch, struct char_data *victim, int level,
                                 int casttype, int save, int school)
{
  if (ch == NULL || victim == NULL)
    return TRUE;

  if (mag_resistance(ch, victim, 0) || savingthrow(ch, victim, save, 0, casttype, level, school))
  {
    act("$N resists your spell.", FALSE, ch, NULL, victim, TO_CHAR);
    act("You resist $n's spell.", FALSE, ch, NULL, victim, TO_VICT);
    return TRUE;
  }

  return FALSE;
}

static bool can_adjust_character_age(struct char_data *ch, struct char_data *victim)
{
  if (ch == NULL || victim == NULL || IS_NPC(victim))
  {
    if (ch != NULL)
      send_to_char(ch, "That spell can only alter a player character's age.\r\n");
    return FALSE;
  }

  if (ch != victim && !is_player_grouped(ch, victim))
  {
    send_to_char(ch, "%s must be grouped with you to accept that lasting change.\r\n",
                 GET_NAME(victim));
    return FALSE;
  }

  return TRUE;
}

static void adjust_character_age(struct char_data *victim, int years)
{
  time_t now;
  time_t delta;

  if (victim == NULL || IS_NPC(victim) || years == 0)
    return;

  now = time(NULL);
  delta = (time_t)years * SECS_PER_MUD_YEAR;
  victim->player.time.birth -= delta;
  if (years < 0 && victim->player.time.birth > now)
    victim->player.time.birth = now;
}

static void end_fights_with(struct char_data *victim)
{
  struct char_data *person;
  struct char_data *next_person;

  if (victim == NULL || IN_ROOM(victim) == NOWHERE)
    return;

  for (person = world[IN_ROOM(victim)].people; person; person = next_person)
  {
    next_person = person->next_in_room;
    if (FIGHTING(person) == victim)
      stop_fighting(person);
  }
  if (FIGHTING(victim) != NULL)
    stop_fighting(victim);
}

ASPELL(spell_farsee)
{
  if (ch == NULL)
    return;

  apply_spell_effect(ch, SPELL_FARSEE, MAX(4, level * 2), APPLY_NONE, 0, AFF_FARSEE,
                     NO_AFFECT_FLAG);
  send_to_char(ch, "Your vision sharpens and reaches far beyond the horizon.\r\n");
}

ASPELL(spell_rejuvenate_major)
{
  int years;

  if (!can_adjust_character_age(ch, victim))
    return;

  years = dice(1, 3);
  adjust_character_age(victim, -years);
  send_to_char(victim, "Warmth settles into your bones as %d year%s fall away.\r\n", years,
               years == 1 ? "" : "s");
  if (ch != victim)
    act("$N looks subtly younger.", FALSE, ch, NULL, victim, TO_CHAR);
}

ASPELL(spell_rejuvenate_minor)
{
  int years;

  if (ch == NULL || victim == NULL)
    return;
  if (ch != victim && !is_player_grouped(ch, victim))
  {
    send_to_char(ch, "That spell can only be shared with a group member.\r\n");
    return;
  }

  years = MAX(1, dice(2, MAX(1, level)) / 2);
  apply_spell_effect(victim, SPELL_REJUVENATE_MINOR, MAX(4, level), APPLY_AGE, -years,
                     NO_AFFECT_FLAG, NO_AFFECT_FLAG);
  send_to_char(victim, "You feel younger, though the change is only temporary.\r\n");
}

ASPELL(spell_age)
{
  int years;

  if (!can_adjust_character_age(ch, victim))
    return;

  years = dice(2, 8);
  adjust_character_age(victim, years);
  send_to_char(victim, "A sudden weight settles on you as you age %d years.\r\n", years);
  if (ch != victim)
    act("$N looks noticeably older.", FALSE, ch, NULL, victim, TO_CHAR);
}

ASPELL(spell_command_undead)
{
  if (ch == NULL || victim == NULL)
    return;
  if (!IS_NPC(victim) || !IS_UNDEAD(victim))
  {
    send_to_char(ch, "Only an undead creature can be commanded by this spell.\r\n");
    return;
  }
  if (GET_LEVEL(victim) > level)
  {
    act("$N is too powerful for you to command.", FALSE, ch, NULL, victim, TO_CHAR);
    return;
  }

  effect_charm(ch, victim, SPELL_COMMAND_UNDEAD, casttype, level);
}

ASPELL(spell_command_horde)
{
  struct char_data *target;
  struct char_data *next_target;
  int commanded;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  commanded = 0;
  for (target = world[IN_ROOM(ch)].people; target; target = next_target)
  {
    next_target = target->next_in_room;
    if (target == ch || !IS_NPC(target) || !IS_UNDEAD(target) || GET_LEVEL(target) > level ||
        !aoeOK(ch, target, SPELL_COMMAND_HORDE))
      continue;
    spell_command_undead(level, ch, target, obj, casttype);
    if (target->master == ch)
      commanded++;
  }

  if (commanded == 0)
    send_to_char(ch, "No undead in the area submit to your command.\r\n");
}

ASPELL(spell_slow_poison)
{
  if (victim == NULL)
    return;

  apply_spell_effect(victim, SPELL_SLOW_POISON, spell_effect_duration(level, 4, 2), APPLY_NONE, 0,
                     NO_AFFECT_FLAG, AFF2_ROL_SLOW_POISON);
  send_to_char(victim, "Your pulse steadies as poisons begin moving more slowly.\r\n");
}

ASPELL(spell_comprehend_languages)
{
  if (victim == NULL)
    return;

  apply_spell_effect(victim, SPELL_COMPREHEND_LANGUAGES, MAX(4, level * 2), APPLY_NONE, 0,
                     NO_AFFECT_FLAG, NO_AFFECT_FLAG);
  send_to_char(victim, "Unfamiliar languages begin to make sense to you.\r\n");
}

ASPELL(spell_fumble)
{
  int penalty;

  if (victim == NULL || target_resists_spell(ch, victim, level, casttype, SAVING_WILL, ENCHANTMENT))
    return;

  penalty = MIN(0, 1 - GET_REAL_DEX(victim));
  apply_spell_effect(victim, SPELL_FUMBLE, spell_effect_duration(level, 3, 4), APPLY_DEX, penalty,
                     NO_AFFECT_FLAG, NO_AFFECT_FLAG);
  act("Your hands become impossibly clumsy.", FALSE, ch, NULL, victim, TO_VICT);
  act("$N fumbles as precise movement deserts $M.", FALSE, ch, NULL, victim, TO_CHAR);
}

ASPELL(spell_stumble)
{
  int duration;

  if (victim == NULL || target_resists_spell(ch, victim, level, casttype, SAVING_WILL, ENCHANTMENT))
    return;

  duration = spell_effect_duration(level, 3, 4);
  apply_spell_effect(victim, SPELL_STUMBLE, duration, APPLY_AC_NEW, -4, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  apply_spell_effect(victim, SPELL_STUMBLE, duration, APPLY_SAVING_REFL, -4, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  apply_spell_effect(victim, SPELL_STUMBLE, duration, APPLY_INITIATIVE, -4, AFF_STAGGERED,
                     NO_AFFECT_FLAG);
  act("Your balance fails and every step becomes uncertain.", FALSE, ch, NULL, victim, TO_VICT);
  act("$N staggers as $S balance deserts $M.", FALSE, ch, NULL, victim, TO_CHAR);
}

ASPELL(spell_enervate)
{
  int penalty;

  if (victim == NULL || target_resists_spell(ch, victim, level, casttype, SAVING_FORT, NECROMANCY))
    return;

  penalty = MIN(0, 1 - GET_REAL_CON(victim));
  apply_spell_effect(victim, SPELL_ENERVATE, spell_effect_duration(level, 3, 4), APPLY_CON, penalty,
                     NO_AFFECT_FLAG, NO_AFFECT_FLAG);
  act("Your vitality drains away, leaving you frighteningly frail.", FALSE, ch, NULL, victim,
      TO_VICT);
  act("$N pales as $S vitality drains away.", FALSE, ch, NULL, victim, TO_CHAR);
}

ASPELL(spell_protect_undead)
{
  int duration;

  if (ch == NULL || victim == NULL)
    return;
  if (!IS_UNDEAD(victim))
  {
    send_to_char(ch, "That target has no undead essence to protect.\r\n");
    return;
  }

  duration = spell_effect_duration(level, 5, 2);
  apply_spell_effect(victim, SPELL_PROT_UNDEAD, duration, APPLY_AC_NEW, 4, AFF_WARDED,
                     NO_AFFECT_FLAG);
  apply_spell_effect(victim, SPELL_PROT_UNDEAD, duration, APPLY_SAVING_WILL, 2, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  send_to_char(victim, "A dark ward settles around your undead form.\r\n");
}

ASPELL(spell_protection_from_undead)
{
  int duration;

  if (victim == NULL)
    return;

  duration = spell_effect_duration(level, 5, 2);
  apply_spell_effect(victim, SPELL_PROT_FROM_UNDEAD, duration, APPLY_AC_NEW, 2, AFF_WARDED,
                     NO_AFFECT_FLAG);
  apply_spell_effect(victim, SPELL_PROT_FROM_UNDEAD, duration, APPLY_SAVING_WILL, 2, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  send_to_char(victim, "A pale ward rises between you and the undead.\r\n");
}

ASPELL(spell_ancestral_shield)
{
  struct char_data *target;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  for (target = world[IN_ROOM(ch)].people; target; target = target->next_in_room)
  {
    if (!is_player_grouped(ch, target))
      continue;
    apply_spell_effect(target, SPELL_ANCESTRAL_SHIELD, spell_effect_duration(level, 2, 10),
                       APPLY_NONE, 0, NO_AFFECT_FLAG, NO_AFFECT_FLAG);
    send_to_char(target, "Ancestral spirits gather into a shimmering shield around you.\r\n");
  }
}

ASPELL(spell_protection_from_animals)
{
  int duration;

  if (victim == NULL)
    return;

  duration = spell_effect_duration(level, 5, 2);
  apply_spell_effect(victim, SPELL_PROTECTION_FROM_ANIMALS, duration, APPLY_AC_NEW, 2, AFF_WARDED,
                     NO_AFFECT_FLAG);
  apply_spell_effect(victim, SPELL_PROTECTION_FROM_ANIMALS, duration, APPLY_SAVING_REFL, 2,
                     NO_AFFECT_FLAG, NO_AFFECT_FLAG);
  send_to_char(victim, "A primal ward rises between you and hostile beasts.\r\n");
}

ASPELL(spell_pass_without_trace)
{
  if (ch == NULL)
    return;

  apply_spell_effect(ch, SPELL_PASS_WITHOUT_TRACE, spell_effect_duration(level, 4, 2), APPLY_NONE,
                     0, AFF_NOTRACK, NO_AFFECT_FLAG);
  send_to_char(ch, "Your passage ceases to leave any trail.\r\n");
}

ASPELL(spell_greater_realm_of_protection)
{
  static const int resistance_applies[] = {
      APPLY_RES_FIRE,  APPLY_RES_COLD, APPLY_RES_AIR,
      APPLY_RES_EARTH, APPLY_RES_ACID, APPLY_RES_ELECTRIC,
  };
  size_t index;
  int duration;
  int resistance;

  if (victim == NULL)
    return;

  duration = spell_effect_duration(level, 5, 2);
  resistance = 10 + MIN(20, level / 2);
  for (index = 0; index < sizeof(resistance_applies) / sizeof(resistance_applies[0]); index++)
    apply_spell_effect(victim, SPELL_GREATER_REALM_OF_PROTECTION, duration,
                       resistance_applies[index], resistance, NO_AFFECT_FLAG, NO_AFFECT_FLAG);
  send_to_char(victim, "Layered wards shield you from every elemental realm.\r\n");
}

ASPELL(spell_feign_death)
{
  if (victim == NULL)
    return;

  end_fights_with(victim);
  apply_spell_effect(victim, SPELL_FEIGN_DEATH, spell_effect_duration(level, 2, 5), APPLY_NONE, 0,
                     AFF_REFUGE, NO_AFFECT_FLAG);
  send_to_char(victim, "Your breath stills and you take on the semblance of death.\r\n");
  act("$n goes utterly still, showing no sign of life.", FALSE, victim, NULL, NULL, TO_ROOM);
}

ASPELL(spell_tranquility)
{
  struct char_data *target;
  struct char_data *next_target;
  int duration;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  duration = spell_effect_duration(level, 2, 10);
  for (target = world[IN_ROOM(ch)].people; target; target = next_target)
  {
    next_target = target->next_in_room;
    if (target != ch && !is_player_grouped(ch, target) && !aoeOK(ch, target, SPELL_TRANQUILITY))
      continue;
    end_fights_with(target);
    apply_spell_effect(target, SPELL_TRANQUILITY, duration, APPLY_NONE, 0, NO_AFFECT_FLAG,
                       AFF2_ROL_DOCILE);
  }
  send_to_room(IN_ROOM(ch), "A profound tranquility settles over the area.\r\n");
}

ASPELL(spell_agility)
{
  int duration;

  if (victim == NULL)
    return;

  duration = MAX(4, level);
  apply_spell_effect(victim, SPELL_AGILITY, duration, APPLY_AC_NEW, 4, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  apply_spell_effect(victim, SPELL_AGILITY, duration, APPLY_SAVING_REFL, 4, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  apply_spell_effect(victim, SPELL_AGILITY, duration, APPLY_INITIATIVE, 4, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  send_to_char(victim, "Your balance and reactions become supernaturally agile.\r\n");
}

ASPELL(spell_natures_blessing)
{
  int duration;
  int hit_bonus;
  int save_bonus;

  if (ch == NULL)
    return;

  duration = MAX(5, level / 2);
  hit_bonus = level < 35 ? 2 : 3;
  save_bonus = level < 18 ? 3 : (level < 49 ? 4 : 5);
  apply_spell_effect(ch, SPELL_NATURES_BLESSING, duration, APPLY_HITROLL, hit_bonus, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  apply_spell_effect(ch, SPELL_NATURES_BLESSING, duration, APPLY_SAVING_FORT, save_bonus,
                     NO_AFFECT_FLAG, NO_AFFECT_FLAG);
  apply_spell_effect(ch, SPELL_NATURES_BLESSING, duration, APPLY_SAVING_REFL, save_bonus,
                     NO_AFFECT_FLAG, NO_AFFECT_FLAG);
  apply_spell_effect(ch, SPELL_NATURES_BLESSING, duration, APPLY_SAVING_WILL, save_bonus,
                     NO_AFFECT_FLAG, NO_AFFECT_FLAG);
  send_to_char(ch, "Nature's blessing wraps around you like a warm mantle.\r\n");
}

ASPELL(spell_song_of_travel)
{
  struct char_data *target;
  int duration;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  duration = spell_effect_duration(level, 4, 2);
  for (target = world[IN_ROOM(ch)].people; target; target = target->next_in_room)
  {
    if (!is_player_grouped(ch, target))
      continue;
    GET_MOVE(target) = MIN(GET_MAX_MOVE(target), GET_MOVE(target) + MAX(10, level * 2));
    apply_spell_effect(target, SPELL_SONG_OF_TRAVEL, duration, APPLY_NONE, 0, AFF_FLYING,
                       NO_AFFECT_FLAG);
    send_to_char(target,
                 "A traveling melody lightens your feet and lifts you from the ground.\r\n");
  }
}

ASPELL(spell_poltergeist)
{
  struct char_data *target;
  int eligible;
  int selected;
  int strike;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  send_to_room(IN_ROOM(ch),
               "Dark forms coalesce from shadow and begin hurling invisible force!\r\n");
  for (strike = 0; strike < 3; strike++)
  {
    eligible = 0;
    for (target = world[IN_ROOM(ch)].people; target; target = target->next_in_room)
      if (target != ch && !IS_INCORPOREAL(target) && aoeOK(ch, target, SPELL_POLTERGEIST))
        eligible++;

    if (eligible == 0)
      break;

    selected = rand_number(1, eligible);
    for (target = world[IN_ROOM(ch)].people; target; target = target->next_in_room)
    {
      if (target == ch || IS_INCORPOREAL(target) || !aoeOK(ch, target, SPELL_POLTERGEIST))
        continue;
      if (--selected == 0)
        break;
    }

    if (target != NULL)
      mag_damage(level, ch, target, NULL, SPELL_POLTERGEIST, 0, SAVING_REFL, casttype);
  }
}

struct minor_creation_definition
{
  const char *keyword;
  const char *short_description;
  const char *room_description;
  int item_type;
  int subtype;
  int material;
  int weight;
  int value;
};

static const struct minor_creation_definition minor_creation_options[] = {
    {"bag", "a plain cloth bag", "A plain cloth bag rests here.", ITEM_CONTAINER, 0,
     MATERIAL_COTTON, 2, 100},
    {"ration", "a simple trail ration", "A simple trail ration rests here.", ITEM_FOOD, 0,
     MATERIAL_ORGANIC, 1, 24},
    {"raft", "a small wooden raft", "A small wooden raft rests here.", ITEM_BOAT, 0, MATERIAL_WOOD,
     25, 0},
    {"robe", "a plain cotton robe", "A plain cotton robe lies here.", ITEM_ARMOR,
     SPEC_ARMOR_TYPE_CLOTHING, MATERIAL_COTTON, 2, 0},
    {"barrel", "a plain wooden barrel", "A plain wooden barrel stands here.", ITEM_CONTAINER, 0,
     MATERIAL_WOOD, 15, 250},
    {"club", "a plain wooden club", "A plain wooden club lies here.", ITEM_WEAPON, WEAPON_TYPE_CLUB,
     MATERIAL_WOOD, 3, 0},
    {"staff", "a plain wooden quarterstaff", "A plain wooden quarterstaff lies here.", ITEM_WEAPON,
     WEAPON_TYPE_QUARTERSTAFF, MATERIAL_WOOD, 4, 0},
    {"spear", "a plain wooden spear", "A plain wooden spear lies here.", ITEM_WEAPON,
     WEAPON_TYPE_SPEAR, MATERIAL_WOOD, 6, 0},
    {"dagger", "a plain steel dagger", "A plain steel dagger lies here.", ITEM_WEAPON,
     WEAPON_TYPE_DAGGER, MATERIAL_STEEL, 1, 0},
    {"shield", "a plain wooden buckler", "A plain wooden buckler lies here.", ITEM_ARMOR,
     SPEC_ARMOR_TYPE_BUCKLER, MATERIAL_WOOD, 5, 0},
    {"torch", "a plain wooden torch", "A plain wooden torch lies here.", ITEM_LIGHT, 0,
     MATERIAL_WOOD, 1, 24},
    {"box", "a plain wooden box", "A plain wooden box rests here.", ITEM_CONTAINER, 0,
     MATERIAL_WOOD, 5, 50},
    {"paper", "a blank sheet of paper", "A blank sheet of paper lies here.", ITEM_NOTE, 0,
     MATERIAL_PAPER, 1, 0},
    {"quill", "a plain writing quill", "A plain writing quill lies here.", ITEM_PEN, 0,
     MATERIAL_ORGANIC, 1, 0},
};

ASPELL(spell_minor_creation)
{
  const struct minor_creation_definition *definition;
  struct obj_data *created;
  char choice[MAX_INPUT_LENGTH];
  size_t index;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  one_argument(cast_arg3, choice, sizeof(choice));
  definition = NULL;
  for (index = 0; index < sizeof(minor_creation_options) / sizeof(minor_creation_options[0]);
       index++)
  {
    if (*choice && is_abbrev(choice, minor_creation_options[index].keyword))
    {
      definition = &minor_creation_options[index];
      break;
    }
  }

  if (definition == NULL)
  {
    send_to_char(ch, "Create what? Choose bag, ration, raft, robe, barrel, club, staff, spear, "
                     "dagger, shield, torch, box, paper, or quill.\r\n");
    return;
  }

  created = create_obj();
  if (definition->item_type == ITEM_WEAPON)
    set_weapon_object(created, definition->subtype);
  else if (definition->item_type == ITEM_ARMOR)
    set_armor_object(created, definition->subtype);
  else
    GET_OBJ_TYPE(created) = definition->item_type;

  GET_OBJ_COST(created) = 0;
  GET_OBJ_RENT(created) = 0;
  GET_OBJ_WEIGHT(created) = definition->weight;
  GET_OBJ_MATERIAL(created) = definition->material;
  GET_OBJ_SIZE(created) = GET_SIZE(ch);
  GET_OBJ_BOUND_ID(created) = NOBODY;
  created->name = strdup(definition->keyword);
  created->short_description = strdup(definition->short_description);
  created->description = strdup(definition->room_description);

  if (definition->item_type == ITEM_CONTAINER || definition->item_type == ITEM_FOOD)
    GET_OBJ_VAL(created, 0) = definition->value;
  else if (definition->item_type == ITEM_LIGHT)
    GET_OBJ_VAL(created, 2) = definition->value;
  else if (definition->item_type == ITEM_NOTE)
    created->action_description = strdup("");

  SET_BIT_AR(GET_OBJ_WEAR(created), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(created), ITEM_NORENT);
  SET_BIT_AR(GET_OBJ_EXTRA(created), ITEM_NOSELL);
  obj_to_room(created, IN_ROOM(ch));
  act("$p suddenly takes shape before you.", FALSE, ch, created, NULL, TO_CHAR);
  act("$p suddenly takes shape before $n.", FALSE, ch, created, NULL, TO_ROOM);
}

ASPELL(spell_ventriloquate)
{
  struct char_data *target_char;
  struct char_data *listener;
  struct obj_data *target_obj;
  const char *speech;
  const char *source_name;
  char target_name[MAX_INPUT_LENGTH];
  int found;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  speech = one_argument(cast_arg3, target_name, sizeof(target_name));
  while (*speech && isspace((unsigned char)*speech))
    speech++;
  if (!*target_name || !*speech)
  {
    send_to_char(ch, "Usage: cast 'ventriloquate' <target> <speech>\r\n");
    return;
  }

  target_char = NULL;
  target_obj = NULL;
  found = generic_find(target_name, FIND_CHAR_ROOM | FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP,
                       ch, &target_char, &target_obj);
  if (!found || (target_char == NULL && target_obj == NULL))
  {
    send_to_char(ch, "You cannot find anything nearby to throw your voice toward.\r\n");
    return;
  }
  if (target_char == ch)
  {
    send_to_char(ch, "Throwing your own voice back at yourself would accomplish nothing.\r\n");
    return;
  }

  source_name = target_char != NULL ? GET_NAME(target_char) : target_obj->short_description;
  if (source_name == NULL)
    source_name = "something nearby";

  send_to_char(ch, "You throw your voice toward %s: '%s'\r\n", source_name, speech);
  for (listener = world[IN_ROOM(ch)].people; listener; listener = listener->next_in_room)
  {
    if (listener == ch || AFF_FLAGGED(listener, AFF_DEAF))
      continue;
    if (listener != target_char &&
        savingthrow(ch, listener, SAVING_WILL, 0, casttype, level, ILLUSION))
      send_to_char(listener, "A telltale magical echo betrays the voice coming from %s.\r\n",
                   source_name);
    else
      send_to_char(listener, "%s says, '%s'\r\n", source_name, speech);
  }
}

ASPELL(spell_wraithform)
{
  if (ch == NULL)
    return;
  if (victim == NULL)
    victim = ch;
  if (affected_by_spell(victim, SPELL_WRAITHFORM))
  {
    if (victim == ch)
      send_to_char(ch, "You are already without a material form.\r\n");
    else
      send_to_char(ch, "%s is already without a material form.\r\n", GET_NAME(victim));
    return;
  }

  end_fights_with(victim);
  apply_spell_effect(victim, SPELL_WRAITHFORM, 10, APPLY_NONE, 0, AFF_IMMATERIAL, NO_AFFECT_FLAG);
  send_to_char(victim, "Your body fades until only a pale, immaterial outline remains.\r\n");
  act("$n fades into a pale, immaterial outline.", FALSE, victim, NULL, NULL, TO_ROOM);
}

ASPELL(spell_create_spring)
{
  struct obj_data *spring;
  int capacity;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;
  if (!OUTSIDE(ch))
  {
    send_to_char(ch, "A spring can only break through open ground outdoors.\r\n");
    return;
  }

  spring = create_obj();
  capacity = MAX(20, level * 20);
  GET_OBJ_TYPE(spring) = ITEM_FOUNTAIN;
  GET_OBJ_VAL(spring, 0) = capacity;
  GET_OBJ_VAL(spring, 1) = capacity;
  GET_OBJ_VAL(spring, 2) = LIQ_WATER;
  GET_OBJ_VAL(spring, 3) = 0;
  GET_OBJ_TIMER(spring) = MAX(2, level);
  GET_OBJ_COST(spring) = 0;
  GET_OBJ_RENT(spring) = 0;
  GET_OBJ_WEIGHT(spring) = 100;
  GET_OBJ_MATERIAL(spring) = MATERIAL_STONE;
  spring->name = strdup("spring water fountain");
  spring->short_description = strdup("a clear natural spring");
  spring->description = strdup("A clear natural spring bubbles up from the ground here.");
  SET_BIT_AR(GET_OBJ_EXTRA(spring), ITEM_DECAY);
  SET_BIT_AR(GET_OBJ_EXTRA(spring), ITEM_NORENT);
  SET_BIT_AR(GET_OBJ_EXTRA(spring), ITEM_NOSELL);
  obj_to_room(spring, IN_ROOM(ch));
  send_to_room(IN_ROOM(ch), "A clear spring bursts from the ground and begins to flow.\r\n");
}

static struct obj_data *create_moonwell_portal(room_vnum destination, int duration)
{
  struct obj_data *portal;

  portal = create_obj();
  GET_OBJ_TYPE(portal) = ITEM_PORTAL;
  GET_OBJ_VAL(portal, 0) = PORTAL_NORMAL;
  GET_OBJ_VAL(portal, 1) = destination;
  GET_OBJ_TIMER(portal) = duration;
  GET_OBJ_COST(portal) = 0;
  GET_OBJ_RENT(portal) = 0;
  GET_OBJ_WEIGHT(portal) = 0;
  GET_OBJ_MATERIAL(portal) = MATERIAL_ETHER;
  portal->name = strdup("moonwell portal pool");
  portal->short_description = strdup("a pool of swirling silver moonlight");
  portal->description = strdup("A pool of swirling silver moonlight shimmers here.");
  SET_BIT_AR(GET_OBJ_EXTRA(portal), ITEM_DECAY);
  SET_BIT_AR(GET_OBJ_EXTRA(portal), ITEM_NORENT);
  SET_BIT_AR(GET_OBJ_EXTRA(portal), ITEM_NOSELL);
  return portal;
}

ASPELL(spell_moonwell)
{
  struct obj_data *near_portal;
  struct obj_data *far_portal;
  room_rnum source_room;
  room_rnum destination_room;
  int duration;

  if (ch == NULL || victim == NULL || IN_ROOM(ch) == NOWHERE || IN_ROOM(victim) == NOWHERE)
    return;
  if (IS_NPC(victim))
  {
    send_to_char(ch, "A moonwell can only be anchored to another player.\r\n");
    return;
  }

  source_room = IN_ROOM(ch);
  destination_room = IN_ROOM(victim);
  if (source_room == destination_room)
  {
    send_to_char(ch, "The two ends of a moonwell must be in different places.\r\n");
    return;
  }
  if (AFF_FLAGGED(victim, AFF_NOTELEPORT) || AFF_FLAGGED(victim, AFF_DIM_LOCK) ||
      ROOM_FLAGGED(source_room, ROOM_NOSUMMON) || ROOM_FLAGGED(destination_room, ROOM_NOSUMMON) ||
      !valid_mortal_tele_dest(ch, source_room, TRUE) ||
      !valid_mortal_tele_dest(ch, destination_room, TRUE))
  {
    send_to_char(ch, "The moonwell cannot find a stable anchor between those locations.\r\n");
    return;
  }
  if (ZONE_FLAGGED(GET_ROOM_ZONE(source_room), ZONE_ELEMENTAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(source_room), ZONE_ETH_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(source_room), ZONE_ASTRAL_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(destination_room), ZONE_ELEMENTAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(destination_room), ZONE_ETH_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(destination_room), ZONE_ASTRAL_PLANE))
  {
    send_to_char(ch, "Moonwells can only join locations on the material plane.\r\n");
    return;
  }

  duration = MAX(2, level / 5);
  near_portal = create_moonwell_portal(GET_ROOM_VNUM(destination_room), duration);
  far_portal = create_moonwell_portal(GET_ROOM_VNUM(source_room), duration);
  obj_to_room(near_portal, source_room);
  obj_to_room(far_portal, destination_room);
  send_to_room(source_room,
               "Swirling silver mist gathers into a moonlit pool upon the ground.\r\n");
  send_to_room(destination_room,
               "Swirling silver mist gathers into a moonlit pool upon the ground.\r\n");
}

ASPELL(spell_blink)
{
  struct char_data *ally;
  room_rnum destination;
  int available_directions[NUM_OF_DIRS];
  int available_count;
  int direction;

  if (ch == NULL)
    return;
  if (victim == NULL)
    victim = ch;
  if (IN_ROOM(victim) == NOWHERE)
    return;
  if (victim != ch && !is_player_grouped(ch, victim))
  {
    send_to_char(ch, "You can only blink yourself or a member of your group.\r\n");
    return;
  }

  ally = NULL;
  for (ally = world[IN_ROOM(victim)].people; ally; ally = ally->next_in_room)
    if (ally != victim && is_player_grouped(victim, ally))
      break;

  if (FIGHTING(victim) != NULL && ally != NULL)
  {
    end_fights_with(victim);
    send_to_char(victim, "You vanish and reappear a few feet away, free of the melee.\r\n");
    act("$n vanishes and reappears a few feet away, free of the melee.", FALSE, victim, NULL, NULL,
        TO_ROOM);
    return;
  }

  available_count = 0;
  for (direction = 0; direction < DIR_COUNT; direction++)
  {
    if (EXIT(victim, direction) == NULL || EXIT(victim, direction)->to_room == NOWHERE ||
        IS_SET(EXIT(victim, direction)->exit_info, EX_CLOSED | EX_BLOCKED | EX_HIDDEN |
                                                       EX_HIDDEN_EASY | EX_HIDDEN_MEDIUM |
                                                       EX_HIDDEN_HARD) ||
        !valid_mortal_tele_dest(victim, EXIT(victim, direction)->to_room, TRUE))
      continue;
    available_directions[available_count++] = direction;
  }

  if (available_count == 0)
  {
    send_to_char(ch, "The spell cannot find a safe nearby place to re-form.\r\n");
    return;
  }

  direction = available_directions[rand_number(0, available_count - 1)];
  destination = EXIT(victim, direction)->to_room;
  end_fights_with(victim);
  act("$n blinks out of existence.", FALSE, victim, NULL, NULL, TO_ROOM);
  send_to_char(victim, "You blink out of existence.\r\n");
  char_from_room(victim);
  if (ZONE_FLAGGED(GET_ROOM_ZONE(destination), ZONE_WILDERNESS))
  {
    X_LOC(victim) = world[destination].coords[0];
    Y_LOC(victim) = world[destination].coords[1];
  }
  char_to_room_cause(victim, destination, ch, DOMAIN_RELOCATION_TELEPORT, -1);
  act("$n blinks into existence.", FALSE, victim, NULL, NULL, TO_ROOM);
  send_to_char(victim, "You blink back into existence nearby.\r\n");
  if (!IS_NPC(victim))
    look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
}

ASPELL(spell_dimension_shift)
{
  struct affected_type af;
  struct char_data *target;
  struct char_data *next_target;
  int duration;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;
  if ((AFF_FLAGGED(ch, AFF_DIM_LOCK) || AFF_FLAGGED(ch, AFF_NOTELEPORT)) &&
      !affected_by_spell(ch, SPELL_DIMENSION_SHIFT))
  {
    send_to_char(ch, "A dimensional lock prevents the shift.\r\n");
    return;
  }

  duration = MAX(2, level / 10);
  for (target = world[IN_ROOM(ch)].people; target; target = next_target)
  {
    next_target = target->next_in_room;
    if (target != ch && !is_player_grouped(ch, target))
      continue;
    if (target != ch &&
        (AFF_FLAGGED(target, AFF_DIM_LOCK) || AFF_FLAGGED(target, AFF_NOTELEPORT)) &&
        !affected_by_spell(target, SPELL_DIMENSION_SHIFT))
    {
      send_to_char(target, "A dimensional anchor prevents you from joining the shift.\r\n");
      continue;
    }

    end_fights_with(target);
    if (affected_by_spell(target, SPELL_DIMENSION_SHIFT))
      affect_from_char(target, SPELL_DIMENSION_SHIFT);
    new_affect(&af);
    af.spell = SPELL_DIMENSION_SHIFT;
    af.duration = duration;
    SET_BIT_AR(af.bitvector, AFF_REFUGE);
    SET_BIT_AR(af.bitvector, AFF_IMMATERIAL);
    SET_BIT_AR(af.bitvector, AFF_DIM_LOCK);
    affect_to_char(target, &af);
    send_to_char(target,
                 "Space folds around you, sheltering you within a pocket beyond the room.\r\n");
  }
  act("Space folds inward and shelters $n's group beyond ordinary reach.", FALSE, ch, NULL, NULL,
      TO_ROOM);
}

ASPELL(spell_spirit_walk)
{
  struct obj_data *corpse;
  room_rnum destination;

  if (ch == NULL || victim == NULL || IN_ROOM(ch) == NOWHERE || IS_NPC(victim))
    return;
  if (victim != ch && !is_player_grouped(ch, victim))
  {
    send_to_char(ch, "%s must be grouped with you before you can follow that spirit.\r\n",
                 GET_NAME(victim));
    return;
  }

  corpse = NULL;
  for (corpse = object_list; corpse; corpse = corpse->next)
    if (IS_CORPSE(corpse) && GET_OBJ_VAL(corpse, 4) == GET_IDNUM(victim) &&
        IN_ROOM(corpse) != NOWHERE)
      break;

  if (corpse == NULL)
  {
    send_to_char(ch, "You cannot find that spirit's corpse anywhere in the world.\r\n");
    return;
  }

  destination = IN_ROOM(corpse);
  if (destination == IN_ROOM(ch))
  {
    send_to_char(ch, "That spirit's corpse is already here.\r\n");
    return;
  }
  if (AFF_FLAGGED(ch, AFF_NOTELEPORT) || !valid_mortal_tele_dest(ch, IN_ROOM(ch), TRUE) ||
      !valid_mortal_tele_dest(ch, destination, TRUE))
  {
    send_to_char(ch, "The spirit path to that corpse is sealed.\r\n");
    return;
  }

  end_fights_with(ch);
  act("$n dissolves into pale spirit light.", FALSE, ch, NULL, NULL, TO_ROOM);
  send_to_char(ch, "You dissolve into spirit light and follow the path of the dead.\r\n");
  char_from_room(ch);
  if (ZONE_FLAGGED(GET_ROOM_ZONE(destination), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[destination].coords[0];
    Y_LOC(ch) = world[destination].coords[1];
  }
  char_to_room_cause(ch, destination, ch, DOMAIN_RELOCATION_TELEPORT, -1);
  act("Pale spirit light gathers and resolves into $n.", FALSE, ch, NULL, NULL, TO_ROOM);
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

static bool is_earth_elemental(struct char_data *target)
{
  if (target == NULL)
    return FALSE;
  if (HAS_SUBRACE(target, SUBRACE_EARTH) && GET_NPC_RACE(target) == RACE_TYPE_ELEMENTAL)
    return TRUE;

  switch (GET_RACE(target))
  {
  case RACE_SMALL_EARTH_ELEMENTAL:
  case RACE_MEDIUM_EARTH_ELEMENTAL:
  case RACE_EARTH_ELEMENTAL:
  case RACE_LARGE_EARTH_ELEMENTAL:
  case RACE_HUGE_EARTH_ELEMENTAL:
  case RACE_GARGANTUAN_EARTH_ELEMENTAL:
  case RACE_COLOSSAL_EARTH_ELEMENTAL:
    return TRUE;
  default:
    return FALSE;
  }
}

static struct char_data *find_terrain_spell_target(struct char_data *ch, char *target_name,
                                                   size_t target_name_size)
{
  one_argument(cast_arg3, target_name, target_name_size);
  if (!*target_name || is_abbrev(target_name, "room"))
    return NULL;
  return get_char_room_vis(ch, target_name, NULL);
}

ASPELL(spell_rock_to_mud)
{
  struct char_data *target;
  char target_name[MAX_INPUT_LENGTH];
  int amount;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  target = find_terrain_spell_target(ch, target_name, sizeof(target_name));
  if (*target_name && !is_abbrev(target_name, "room") && target == NULL)
  {
    send_to_char(ch, "You cannot find that target here.\r\n");
    return;
  }
  if (target != NULL && is_earth_elemental(target))
  {
    if (!aoeOK(ch, target, SPELL_ROCK_TO_MUD) || mag_resistance(ch, target, 0))
      return;
    amount = dice(MAX(1, level / 3), 6) + level;
    if (savingthrow(ch, target, SAVING_FORT, 0, casttype, level, TRANSMUTATION))
      amount /= 2;
    damage(ch, target, MAX(1, amount), SPELL_ROCK_TO_MUD, DAM_EARTH, FALSE);
    return;
  }

  mag_room(level, ch, NULL, SPELL_ROCK_TO_MUD, casttype);
}

ASPELL(spell_mud_to_rock)
{
  struct char_data *target;
  struct raff_node *raff;
  char target_name[MAX_INPUT_LENGTH];
  int amount;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  target = find_terrain_spell_target(ch, target_name, sizeof(target_name));
  if (*target_name && !is_abbrev(target_name, "room") && target == NULL)
  {
    send_to_char(ch, "You cannot find that target here.\r\n");
    return;
  }
  if (target != NULL && is_earth_elemental(target))
  {
    if (target != ch && !is_player_grouped(ch, target))
    {
      send_to_char(ch, "You can only reinforce an earth elemental allied with you.\r\n");
      return;
    }
    amount = MIN(GET_MAX_HIT(target) - GET_HIT(target), MAX(10, level * 4));
    if (amount <= 0)
    {
      send_to_char(ch, "%s needs no reinforcement.\r\n",
                   target == ch ? "Your elemental form" : GET_NAME(target));
      return;
    }
    GET_HIT(target) += amount;
    update_pos(target);
    send_to_char(target, "Your earthen form hardens, restoring %d hit points.\r\n", amount);
    if (target != ch)
      act("$N's earthen form hardens and repairs itself.", FALSE, ch, NULL, target, TO_CHAR);
    return;
  }

  for (raff = raff_list; raff; raff = raff->next)
  {
    if (raff->room != IN_ROOM(ch) || raff->spell != SPELL_ROCK_TO_MUD)
      continue;
    rem_room_aff(raff);
    return;
  }
  send_to_char(ch, "There is no magically softened ground here to harden.\r\n");
}

ASPELL(spell_phantom_heal)
{
  int amount;
  int half_health;

  if (ch == NULL || victim == NULL)
    return;
  if (victim != ch && !is_player_grouped(ch, victim))
  {
    send_to_char(ch, "You can only veil the wounds of a member of your group.\r\n");
    return;
  }
  if (affected_by_spell(victim, SPELL_PHANTOM_HEAL))
  {
    send_to_char(ch, "Those wounds are already concealed by phantom vitality.\r\n");
    return;
  }

  half_health = GET_MAX_HIT(victim) / 2;
  if (GET_HIT(victim) >= half_health)
  {
    send_to_char(ch, "That target is not wounded badly enough for the illusion to take hold.\r\n");
    return;
  }

  amount = MIN(half_health - GET_HIT(victim), MAX(10, level * 5));
  apply_spell_effect(victim, SPELL_PHANTOM_HEAL, 2, APPLY_SPECIAL, amount, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  GET_HIT(victim) += amount;
  update_pos(victim);
  send_to_char(victim,
               "Phantom vitality conceals your wounds and lends you %d temporary hit "
               "points.\r\n",
               amount);
  act("A healthy illusion settles over $n's wounds.", FALSE, victim, NULL, NULL, TO_ROOM);
}

ASPELL(spell_heal_undead)
{
  int amount;

  if (ch == NULL || victim == NULL)
    return;
  if (!IS_UNDEAD(victim))
  {
    if (victim == ch)
      send_to_char(ch, "You are not undead.\r\n");
    else
      send_to_char(ch, "%s is not undead.\r\n", GET_NAME(victim));
    return;
  }
  if (AFF_FLAGGED(victim, AFF_BLACKMANTLE))
  {
    send_to_char(ch, "The black mantle smothers your healing magic.\r\n");
    return;
  }

  if (!IS_NPC(ch) && !IS_NPC(victim) && IS_LICH(ch) && IS_LICH(victim))
    amount = 100;
  else
    amount = dice(4, MAX(1, level));

  amount = MIN(amount, MAX(0, GET_MAX_HIT(victim) - GET_HIT(victim)));
  if (amount <= 0)
  {
    if (victim == ch)
      send_to_char(ch, "You are already at full strength.\r\n");
    else
      send_to_char(ch, "%s is already at full strength.\r\n", GET_NAME(victim));
    return;
  }

  GET_HIT(victim) += amount;
  update_pos(victim);
  send_to_char(victim, "Dark power knits your undead form together, restoring %d hit points.\r\n",
               amount);
  if (victim != ch)
    send_to_char(ch, "You restore %d hit points to %s's undead form.\r\n", amount,
                 GET_NAME(victim));
}

ASPELL(spell_dark_wrath)
{
  int damage_bonus;
  int duration;
  int save_bonus;

  if (ch == NULL)
    return;
  if (FIGHTING(ch) != NULL)
  {
    send_to_char(ch, "You cannot invite dark wrath while already fighting.\r\n");
    return;
  }
  if (affected_by_spell(ch, SPELL_DARK_WRATH))
  {
    send_to_char(ch, "Dark wrath already empowers you.\r\n");
    return;
  }

  if (level < 46)
  {
    duration = rand_number(5, 8);
    damage_bonus = 1;
    save_bonus = 3;
  }
  else if (level < 50)
  {
    duration = rand_number(7, 10);
    damage_bonus = 2;
    save_bonus = 4;
  }
  else
  {
    duration = rand_number(9, 12);
    damage_bonus = 3;
    save_bonus = 5;
  }

  apply_spell_effect(ch, SPELL_DARK_WRATH, duration, APPLY_DAMROLL, damage_bonus, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  apply_spell_effect(ch, SPELL_DARK_WRATH, duration, APPLY_SAVING_FORT, save_bonus, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  apply_spell_effect(ch, SPELL_DARK_WRATH, duration, APPLY_SAVING_REFL, save_bonus, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  apply_spell_effect(ch, SPELL_DARK_WRATH, duration, APPLY_SAVING_WILL, save_bonus, NO_AFFECT_FLAG,
                     NO_AFFECT_FLAG);
  send_to_char(ch, "Your god smiles upon your destructive soul.\r\n");
}

ASPELL(spell_unholy_aura)
{
  int duration;

  if (ch == NULL)
    return;
  if (AFF_FLAGGED(ch, AFF_FSHIELD))
  {
    send_to_char(ch, "Flames already surround you.\r\n");
    return;
  }

  duration = MAX(1, 3 * get_spell_duration_bonus(ch) / 100);
  apply_spell_effect(ch, SPELL_UNHOLY_AURA, duration, APPLY_NONE, 0, AFF_FSHIELD, NO_AFFECT_FLAG);
  send_to_char(ch, "Unholy flames flare into an aura around you.\r\n");
  if (IN_ROOM(ch) != NOWHERE)
    act("Unholy flames flare into an aura around $n.", FALSE, ch, NULL, NULL, TO_ROOM);
}

void remove_spell_camouflage(struct char_data *ch)
{
  if (ch == NULL)
    return;

  if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
    affect_from_char(ch, SPELL_CAMOUFLAGE);
}

ASPELL(spell_camouflage)
{
  if (ch == NULL)
    return;
  if (AFF_FLAGGED(ch, AFF_HIDE))
  {
    send_to_char(ch, "You are already hidden from view.\r\n");
    return;
  }
  if (RIDING(ch) != NULL)
  {
    send_to_char(ch, "You cannot blend into the surroundings while mounted.\r\n");
    return;
  }

  end_fights_with(ch);
  apply_spell_effect(ch, SPELL_CAMOUFLAGE, -1, APPLY_NONE, 0, AFF_HIDE, NO_AFFECT_FLAG);
  send_to_char(ch, "Your appearance shifts until you blend into the surroundings.\r\n");
  if (IN_ROOM(ch) != NOWHERE)
    act("$n's appearance shifts and blends into the surroundings.", FALSE, ch, NULL, NULL, TO_ROOM);
}

static bool ice_layer_target_is_immune(struct char_data *victim)
{
  if (victim == NULL)
    return true;

  if (IS_INCORPOREAL(victim) || IS_DRAGON(victim) || HAS_SUBRACE(victim, SUBRACE_ANGEL))
    return true;

  return IS_NPC(victim) &&
         (MOB_FLAGGED(victim, MOB_ROL_DEMON) || MOB_FLAGGED(victim, MOB_ROL_DEVIL) ||
          MOB_FLAGGED(victim, MOB_ROL_ANGEL) || MOB_FLAGGED(victim, MOB_ROL_BEHOLDER));
}

#ifdef LUMINARI_CUTEST
bool test_ice_layer_target_is_immune(struct char_data *victim)
{
  return ice_layer_target_is_immune(victim);
}
#endif

ASPELL(spell_ice_layer)
{
  if (ch == NULL || victim == NULL)
    return;
  if (GET_POS(victim) <= POS_SITTING)
  {
    send_to_char(ch, "%s is already on the ground.\r\n", GET_NAME(victim));
    return;
  }
  if (ice_layer_target_is_immune(victim))
  {
    send_to_char(ch, "The ice melts harmlessly beneath %s.\r\n", GET_NAME(victim));
    return;
  }

  act("You conjure a sheet of slippery ice beneath $N!", FALSE, ch, NULL, victim, TO_CHAR);
  act("$n conjures a sheet of slippery ice beneath you!", FALSE, ch, NULL, victim, TO_VICT);
  act("$n conjures a sheet of slippery ice beneath $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
  if (savingthrow(ch, victim, SAVING_REFL, -1, casttype, level, EVOCATION))
  {
    act("$n slips, but manages to retain $s balance.", FALSE, victim, NULL, NULL, TO_ROOM);
    send_to_char(victim, "You slip, but manage to retain your balance.\r\n");
    return;
  }

  act("$n flails wildly and crashes to the ground!", FALSE, victim, NULL, NULL, TO_ROOM);
  send_to_char(victim, "You flail wildly and crash to the ground!\r\n");
  if (damage(ch, victim, dice(2, 10), SPELL_ICE_LAYER, DAM_BLUDGEON, FALSE) < 0)
    return;
  change_position(victim, POS_SITTING);
  WAIT_STATE(victim, PULSE_VIOLENCE);
}

static int call_lycanthrope_level(int caster_level)
{
  return MIN(40, MAX(1, caster_level - 10));
}

static int call_lycanthrope_charm_save_target(int charisma)
{
  return MIN(20, MAX(1, charisma - 2));
}

static mob_vnum random_call_lycanthrope_vnum(void)
{
  mob_rnum index;
  mob_vnum selected = NOBODY;
  int candidates = 0;

  if (mob_proto == NULL || mob_index == NULL)
    return NOBODY;

  for (index = 0; index <= top_of_mobt; index++)
  {
    if (!MOB_FLAGGED(&mob_proto[index], MOB_ROL_LYCANTHROPE_SUMMON))
      continue;

    candidates++;
    if (rand_number(1, candidates) == 1)
      selected = mob_index[index].vnum;
  }

  return selected;
}

MUD_EVENT_CALLBACK(event_rol_call_lycanthrope_charm)
{
  struct mud_event_data *event;
  struct char_data *master;
  struct char_data *mob;

  if (event_obj == NULL)
    return 0;

  event = (struct mud_event_data *)event_obj;
  mob = (struct char_data *)event->pStruct;
  if (mob == NULL || !IS_NPC(mob) || !MOB_FLAGGED(mob, MOB_ROL_LYCANTHROPE_SUMMON) ||
      !AFF_FLAGGED(mob, AFF_CHARM) || mob->master == NULL)
    return 0;

  master = mob->master;
  if (FIGHTING(mob) == NULL)
  {
    if (IN_ROOM(mob) != NOWHERE)
      act("$n slips back through a black door as the charm expires.", FALSE, mob, NULL, NULL,
          TO_ROOM);
    stop_follower(mob);
    extract_char(mob);
    return 0;
  }

  if (rand_number(1, 20) < call_lycanthrope_charm_save_target(GET_CHA(master)))
    return 30 * PASSES_PER_SEC;

  stop_follower(mob);
  act("$n breaks free of the charm and turns on you with a furious snarl!", FALSE, mob, NULL,
      master, TO_VICT);
  act("$n breaks free of the charm and turns on $N with a furious snarl!", FALSE, mob, NULL, master,
      TO_NOTVICT);
  end_fights_with(mob);
  if (IN_ROOM(mob) != NOWHERE && IN_ROOM(mob) == IN_ROOM(master))
    hit(mob, master, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

  return 0;
}

ASPELL(spell_call_lycanthrope)
{
  struct char_data *mob;
  mob_vnum mob_vnum;
  int hit_points;
  int mob_level;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;
  if (!can_add_follower_by_flag(ch, MOB_ROL_LYCANTHROPE_SUMMON))
  {
    send_to_char(ch, "You cannot control more than one lycanthrope at a time!\r\n");
    return;
  }

  mob_vnum = random_call_lycanthrope_vnum();
  if (mob_vnum == NOBODY ||
      (mob = read_mobile_reason(mob_vnum, VIRTUAL, PERF_ENTITY_SPELL_SUMMON)) == NULL)
  {
    log("SYSERR: spell_call_lycanthrope could not find a converted summon prototype");
    send_to_char(ch, "No lycanthrope answers your call. Please report this to staff.\r\n");
    return;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
  {
    X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
    Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
  }
  char_to_room_cause(mob, IN_ROOM(ch), ch, DOMAIN_RELOCATION_SPAWN, -1);
  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;
  GET_GOLD(mob) = 0;

  while (mob->affected != NULL)
    affect_remove(mob, mob->affected);

  mob_level = call_lycanthrope_level(GET_LEVEL(ch));
  GET_LEVEL(mob) = mob_level;
  autoroll_mob(mob, TRUE, TRUE);
  hit_points = MAX(1, dice(mob_level, 20) + GET_CON_BONUS(mob) * mob_level);
  GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob) = GET_HIT(mob) = hit_points;

  act("A black door opens in space and $N leaps through!", FALSE, ch, NULL, mob, TO_ROOM);
  act("A black door opens in space and $N leaps through!", FALSE, ch, NULL, mob, TO_CHAR);
  SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);
  load_mtrigger(mob);
  add_follower(mob, ch);
  if (!GROUP(mob) && GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
    join_group(mob, GROUP(ch));
  NEW_EVENT(eROL_CALL_LYCANTHROPE_CHARM, mob, NULL, 30 * PASSES_PER_SEC);
}

static bool tazriks_event_state(const char *state, room_vnum *room, int *strike)
{
  int parsed_room;
  int parsed_strike;
  char trailing;

  if (state == NULL || sscanf(state, "%d %d %c", &parsed_room, &parsed_strike, &trailing) != 2 ||
      parsed_room < 0 || parsed_strike < 0 || parsed_strike > 2)
    return false;

  if (room != NULL)
    *room = (room_vnum)parsed_room;
  if (strike != NULL)
    *strike = parsed_strike;
  return true;
}

static void tazriks_hound_disappears(room_rnum room)
{
  if (room != NOWHERE)
    send_to_room(room, "The hellhound disappears in a puff of acrid black smoke.\r\n");
}

MUD_EVENT_CALLBACK(event_rol_tazriks_frenzied_hound)
{
  struct mud_event_data *event;
  struct char_data *caster;
  struct char_data *target;
  room_vnum stored_vnum;
  room_rnum room;
  char state[64];
  char *next_state;
  int eligible = 0;
  int selected;
  int strike;

  if (event_obj == NULL)
    return 0;

  event = (struct mud_event_data *)event_obj;
  caster = (struct char_data *)event->pStruct;
  if (!tazriks_event_state(event->sVariables, &stored_vnum, &strike) ||
      (room = real_room(stored_vnum)) == NOWHERE)
    return 0;
  if (caster == NULL || IN_ROOM(caster) == NOWHERE)
  {
    tazriks_hound_disappears(room);
    return 0;
  }

  for (target = world[room].people; target; target = target->next_in_room)
    if (target != caster && !IS_INCORPOREAL(target) &&
        aoeOK(caster, target, SPELL_TAZRIKS_FRENZIED_HOUND))
      eligible++;

  if (eligible == 0)
  {
    tazriks_hound_disappears(room);
    return 0;
  }

  selected = rand_number(1, eligible);
  for (target = world[room].people; target; target = target->next_in_room)
  {
    if (target == caster || IS_INCORPOREAL(target) ||
        !aoeOK(caster, target, SPELL_TAZRIKS_FRENZIED_HOUND))
      continue;
    if (--selected == 0)
      break;
  }

  if (target == NULL)
  {
    tazriks_hound_disappears(room);
    return 0;
  }

  send_to_char(caster, "The frenzied hound lunges from the vortex at %s!\r\n", GET_NAME(target));
  act("A slavering hellhound lunges from the vortex and tears into you!", FALSE, target, NULL, NULL,
      TO_CHAR);
  act("A slavering hellhound lunges from the vortex and tears into $n!", FALSE, target, NULL, NULL,
      TO_ROOM);
  mag_damage(GET_LEVEL(caster), caster, target, NULL, SPELL_TAZRIKS_FRENZIED_HOUND, 0, -1,
             CAST_SPELL);

  if (strike >= 2)
  {
    tazriks_hound_disappears(room);
    return 0;
  }
  if (IN_ROOM(caster) == NOWHERE)
  {
    tazriks_hound_disappears(room);
    return 0;
  }

  snprintf(state, sizeof(state), "%d %d", world[IN_ROOM(caster)].number, strike + 1);
  next_state = strdup(state);
  if (next_state == NULL)
  {
    log("SYSERR: event_rol_tazriks_frenzied_hound could not update event state");
    tazriks_hound_disappears(room);
    return 0;
  }
  free(event->sVariables);
  event->sVariables = next_state;
  return PULSE_VIOLENCE;
}

ASPELL(spell_tazriks_frenzied_hound)
{
  char state[64];

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;

  send_to_room(IN_ROOM(ch),
               "A vortex to the Abyss opens in midair. From it springs a slavering hellhound!\r\n");
  snprintf(state, sizeof(state), "%d 0", world[IN_ROOM(ch)].number);
  NEW_EVENT(eROL_TAZRIKS_FRENZIED_HOUND, ch, state, PULSE_VIOLENCE);
}

#define ROL_ELEMENTAL_MAX_RESISTANCES 3
#define ROL_ELEMENTAL_MAX_FLAGS 3
#define ROL_ELEMENTAL_PROTECTION 50

struct rol_elemental_embodiment_profile
{
  int spellnum;
  int hp_factor;
  int armor_bonus;
  int size_percent;
  int resistance_count;
  int resistances[ROL_ELEMENTAL_MAX_RESISTANCES];
  int flag_count;
  int flags[ROL_ELEMENTAL_MAX_FLAGS];
  const char *victim_message;
  const char *room_message;
};

static bool get_rol_elemental_embodiment_profile(int spellnum,
                                                 struct rol_elemental_embodiment_profile *profile)
{
  if (profile == NULL)
    return false;

  memset(profile, 0, sizeof(*profile));
  profile->spellnum = spellnum;

  switch (spellnum)
  {
  case SPELL_ELEMENTAL_WATER_EMBODIMENT:
    profile->hp_factor = 5;
    profile->size_percent = 25;
    profile->resistance_count = 3;
    profile->resistances[0] = APPLY_RES_FIRE;
    profile->resistances[1] = APPLY_RES_POISON;
    profile->resistances[2] = APPLY_RES_ACID;
    profile->flag_count = 1;
    profile->flags[0] = AFF_WATER_BREATH;
    profile->victim_message =
        "Your body begins to flow and liquefy as you embody elemental water.\r\n";
    profile->room_message = "$n's body flows and liquefies into an embodiment of water.";
    return true;
  case SPELL_ELEMENTAL_FIRE_EMBODIMENT:
    profile->hp_factor = 7;
    profile->armor_bonus = 6;
    profile->size_percent = 35;
    profile->resistance_count = 2;
    profile->resistances[0] = APPLY_RES_POISON;
    profile->resistances[1] = APPLY_RES_FIRE;
    profile->flag_count = 3;
    profile->flags[0] = AFF_FSHIELD;
    profile->flags[1] = AFF_HASTE;
    profile->flags[2] = AFF_FLYING;
    profile->victim_message =
        "Your body begins to smolder and burn as you embody elemental fire.\r\n";
    profile->room_message = "$n's body smolders and burns into an embodiment of fire.";
    return true;
  case SPELL_ELEMENTAL_EARTH_EMBODIMENT:
    /* The live RoL handler uses the fire factor of seven; its declared earth factor is unused. */
    profile->hp_factor = 7;
    profile->size_percent = 50;
    profile->resistance_count = 2;
    profile->resistances[0] = APPLY_RES_POISON;
    profile->resistances[1] = APPLY_RES_COLD;
    profile->victim_message =
        "Your body begins to harden and solidify as you embody elemental earth.\r\n";
    profile->room_message = "$n's body hardens and solidifies into an embodiment of earth.";
    return true;
  case SPELL_ELEMENTAL_AIR_EMBODIMENT:
    profile->hp_factor = 3;
    profile->armor_bonus = 5;
    profile->size_percent = 15;
    profile->resistance_count = 2;
    profile->resistances[0] = APPLY_RES_POISON;
    profile->resistances[1] = APPLY_RES_ACID;
    profile->flag_count = 2;
    profile->flags[0] = AFF_HASTE;
    profile->flags[1] = AFF_FLYING;
    profile->victim_message =
        "Your body begins to swirl and waver as you embody elemental air.\r\n";
    profile->room_message = "$n's body swirls and wavers into an embodiment of air.";
    return true;
  default:
    return false;
  }
}

static bool rol_elemental_embodiment_spell(int spellnum)
{
  struct rol_elemental_embodiment_profile profile;

  return get_rol_elemental_embodiment_profile(spellnum, &profile);
}

bool rol_elemental_embodiment_affect_is_transient(int spellnum)
{
  return spellnum == AFFECT_ROL_ELEMENTAL_EMBODIMENT_MAINTAIN ||
         rol_elemental_embodiment_spell(spellnum);
}

bool rol_elemental_embodiment_active(struct char_data *ch)
{
  if (ch == NULL)
    return false;

  return affected_by_spell(ch, SPELL_ELEMENTAL_WATER_EMBODIMENT) ||
         affected_by_spell(ch, SPELL_ELEMENTAL_FIRE_EMBODIMENT) ||
         affected_by_spell(ch, SPELL_ELEMENTAL_EARTH_EMBODIMENT) ||
         affected_by_spell(ch, SPELL_ELEMENTAL_AIR_EMBODIMENT);
}

static bool rol_elemental_embodiment_same_side(struct char_data *ch, struct char_data *victim)
{
  if (ch == NULL || victim == NULL)
    return false;
  if (IS_NPC(ch) || IS_NPC(victim) || GET_LEVEL(ch) >= LVL_IMMORT || are_grouped(ch, victim))
    return true;

  return !((rol_race_is_good(GET_RACE(ch)) && rol_race_is_evil(GET_RACE(victim))) ||
           (rol_race_is_evil(GET_RACE(ch)) && rol_race_is_good(GET_RACE(victim))));
}

static int rol_elemental_embodiment_hp_bonus(struct char_data *ch, struct char_data *victim,
                                             int factor)
{
  int base;
  int bonus;
  int variance;

  base = MAX(0, MIN(GET_LEVEL(ch), GET_LEVEL(victim)) * factor);
  variance = base * 5 / 100;
  bonus = base + rand_number(-variance, variance);

  if (GET_MAX_HIT(victim) > 30000 || bonus > 30000 - GET_MAX_HIT(victim))
    return 0;

  return MAX(0, bonus);
}

static int rol_elemental_embodiment_size_bonus(int current, int percent)
{
  if (current <= 0 || percent <= 0)
    return 0;

  return MIN(UCHAR_MAX - current, current * percent / 100);
}

static void apply_rol_elemental_embodiment_affect(struct char_data *victim, int spellnum,
                                                  int duration, int location, int modifier,
                                                  int bonus_type, const int *flags, int flag_count,
                                                  long caster_id)
{
  struct affected_type af;
  int index;

  new_affect(&af);
  af.spell = spellnum;
  af.duration = duration;
  af.location = location;
  af.modifier = modifier;
  af.bonus_type = bonus_type;
  for (index = 0; index < flag_count; index++)
    SET_BIT_AR(af.bitvector, flags[index]);
  affect_to_char_source(victim, &af, caster_id);
}

static void apply_rol_elemental_embodiment(struct char_data *ch, struct char_data *victim,
                                           int spellnum)
{
  struct rol_elemental_embodiment_profile profile;
  struct affected_type maintenance;
  long caster_id;
  long victim_id;
  int duration;
  int height_bonus;
  int hp_bonus;
  int index;
  int weight_bonus;

  if (!get_rol_elemental_embodiment_profile(spellnum, &profile))
    return;
  if (ch == NULL || victim == NULL)
    return;
  if (IS_NPC(victim))
  {
    send_to_char(ch, "You cannot cast this spell on NPCs.\r\n");
    return;
  }
  if (!rol_elemental_embodiment_same_side(ch, victim))
  {
    send_to_char(ch, "That person is not worthy of your spell.\r\n");
    return;
  }
  if (affected_by_spell(ch, AFFECT_ROL_ELEMENTAL_EMBODIMENT_MAINTAIN))
  {
    send_to_char(ch, "You are already maintaining an elemental embodiment!\r\n");
    return;
  }
  if (rol_elemental_embodiment_active(victim) || GET_ELEMENTAL_EMBODIMENT_TIMER(victim) > 0)
  {
    send_to_char(ch, "That person is already embodying an elemental force!\r\n");
    return;
  }
  if (RIDING(victim) != NULL)
  {
    send_to_char(ch, "That person needs to dismount first!\r\n");
    return;
  }

  caster_id = char_script_id(ch);
  victim_id = char_script_id(victim);
  duration = MAX(1, 10 * get_spell_duration_bonus(ch) / 100);
  hp_bonus = rol_elemental_embodiment_hp_bonus(ch, victim, profile.hp_factor);
  height_bonus = rol_elemental_embodiment_size_bonus(GET_HEIGHT(victim), profile.size_percent);
  weight_bonus = rol_elemental_embodiment_size_bonus(GET_WEIGHT(victim), profile.size_percent);

  affect_batch_begin(victim);
  apply_rol_elemental_embodiment_affect(victim, spellnum, duration, APPLY_HIT, hp_bonus,
                                        BONUS_TYPE_UNIVERSAL, NULL, 0, caster_id);
  apply_rol_elemental_embodiment_affect(
      victim, spellnum, duration, profile.armor_bonus > 0 ? APPLY_AC_NEW : APPLY_NONE,
      profile.armor_bonus, BONUS_TYPE_NATURALARMOR, profile.flags, profile.flag_count, caster_id);
  for (index = 0; index < profile.resistance_count; index++)
    apply_rol_elemental_embodiment_affect(victim, spellnum, duration, profile.resistances[index],
                                          ROL_ELEMENTAL_PROTECTION, BONUS_TYPE_ENHANCEMENT, NULL, 0,
                                          caster_id);
  if (height_bonus > 0)
    apply_rol_elemental_embodiment_affect(victim, spellnum, duration, APPLY_CHAR_HEIGHT,
                                          height_bonus, BONUS_TYPE_UNIVERSAL, NULL, 0, caster_id);
  if (weight_bonus > 0)
    apply_rol_elemental_embodiment_affect(victim, spellnum, duration, APPLY_CHAR_WEIGHT,
                                          weight_bonus, BONUS_TYPE_UNIVERSAL, NULL, 0, caster_id);
  affect_batch_end(victim);

  GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + hp_bonus);

  new_affect(&maintenance);
  maintenance.spell = AFFECT_ROL_ELEMENTAL_EMBODIMENT_MAINTAIN;
  maintenance.duration = duration;
  maintenance.specific = spellnum;
  affect_to_char_source(ch, &maintenance, victim_id);

  send_to_char(victim, "%s", profile.victim_message);
  if (IN_ROOM(victim) != NOWHERE)
    act(profile.room_message, FALSE, victim, NULL, NULL, TO_ROOM);
}

ASPELL(spell_elemental_water_embodiment)
{
  apply_rol_elemental_embodiment(ch, victim, SPELL_ELEMENTAL_WATER_EMBODIMENT);
}

ASPELL(spell_elemental_fire_embodiment)
{
  apply_rol_elemental_embodiment(ch, victim, SPELL_ELEMENTAL_FIRE_EMBODIMENT);
}

ASPELL(spell_elemental_earth_embodiment)
{
  apply_rol_elemental_embodiment(ch, victim, SPELL_ELEMENTAL_EARTH_EMBODIMENT);
}

ASPELL(spell_elemental_air_embodiment)
{
  apply_rol_elemental_embodiment(ch, victim, SPELL_ELEMENTAL_AIR_EMBODIMENT);
}

static void remove_rol_elemental_embodiment_components(struct char_data *ch, int spellnum,
                                                       long source_id, int specific)
{
  struct affected_type *af;
  struct affected_type *next;

  if (ch == NULL)
    return;

  for (af = ch->affected; af != NULL; af = next)
  {
    next = af->next;
    if (af->spell != spellnum)
      continue;
    if (source_id > 0 && af->source_id != source_id)
      continue;
    if (specific > 0 && af->specific != specific)
      continue;
    affect_remove(ch, af);
  }
}

void remove_rol_elemental_embodiment_affect(struct char_data *ch, int spellnum)
{
  struct affected_type *af;
  struct char_data *counterpart;
  long counterpart_id = 0;
  long own_id;
  int maintained_spell = 0;
  int target_spell;

  if (ch == NULL || !rol_elemental_embodiment_affect_is_transient(spellnum))
    return;

  own_id = char_script_id(ch);
  for (af = ch->affected; af != NULL; af = af->next)
  {
    if (af->spell != spellnum)
      continue;
    counterpart_id = af->source_id;
    maintained_spell = af->specific;
    break;
  }

  remove_rol_elemental_embodiment_components(ch, spellnum, 0, 0);
  if (rol_elemental_embodiment_spell(spellnum))
  {
    counterpart = find_char(counterpart_id);
    remove_rol_elemental_embodiment_components(
        counterpart, AFFECT_ROL_ELEMENTAL_EMBODIMENT_MAINTAIN, own_id, spellnum);
    GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));
    return;
  }

  counterpart = find_char(counterpart_id);
  if (counterpart == NULL)
    return;
  if (rol_elemental_embodiment_spell(maintained_spell))
  {
    remove_rol_elemental_embodiment_components(counterpart, maintained_spell, own_id, 0);
  }
  else
  {
    for (target_spell = SPELL_ELEMENTAL_WATER_EMBODIMENT;
         target_spell <= SPELL_ELEMENTAL_AIR_EMBODIMENT; target_spell++)
      remove_rol_elemental_embodiment_components(counterpart, target_spell, own_id, 0);
  }
  GET_HIT(counterpart) = MIN(GET_HIT(counterpart), GET_MAX_HIT(counterpart));
}

void remove_all_rol_elemental_embodiments(struct char_data *ch)
{
  int spellnum;

  if (ch == NULL)
    return;

  remove_rol_elemental_embodiment_affect(ch, AFFECT_ROL_ELEMENTAL_EMBODIMENT_MAINTAIN);
  for (spellnum = SPELL_ELEMENTAL_WATER_EMBODIMENT; spellnum <= SPELL_ELEMENTAL_AIR_EMBODIMENT;
       spellnum++)
    remove_rol_elemental_embodiment_affect(ch, spellnum);
}

#ifdef LUMINARI_CUTEST
int test_call_lycanthrope_level(int caster_level)
{
  return call_lycanthrope_level(caster_level);
}

int test_call_lycanthrope_charm_save_target(int charisma)
{
  return call_lycanthrope_charm_save_target(charisma);
}

bool test_tazriks_event_state(const char *state, room_vnum *room, int *strike)
{
  return tazriks_event_state(state, room, strike);
}

bool test_rol_elemental_embodiment_same_side(struct char_data *ch, struct char_data *victim)
{
  return rol_elemental_embodiment_same_side(ch, victim);
}
#endif

int adjust_area_damage_for_spell_wards(struct char_data *victim, int damage)
{
  if (victim == NULL || damage <= 0)
    return MAX(0, damage);

  if (affected_by_spell(victim, SPELL_ANCESTRAL_SHIELD) ||
      affected_by_spell(victim, SPELL_NATURES_BLESSING))
    damage = damage * 3 / 4;

  return MAX(0, damage);
}

int adjust_damage_for_creature_wards(struct char_data *attacker, struct char_data *victim,
                                     int damage)
{
  if (attacker == NULL || victim == NULL || damage <= 0 || attacker == victim)
    return MAX(0, damage);

  if ((IS_UNDEAD(attacker) && affected_by_spell(victim, SPELL_PROT_FROM_UNDEAD)) ||
      (IS_ANIMAL(attacker) && affected_by_spell(victim, SPELL_PROTECTION_FROM_ANIMALS)))
    damage = damage * 3 / 4;

  return MAX(0, damage);
}

#undef NO_AFFECT_FLAG
#undef ROL_ELEMENTAL_MAX_RESISTANCES
#undef ROL_ELEMENTAL_MAX_FLAGS
#undef ROL_ELEMENTAL_PROTECTION

#undef ZOCMD

#undef WIZARD_EYE
#undef PRISMATIC_SPHERE
#undef SUMMON_FAIL

#undef WALL_ITEM
#undef WALL_TYPE
#undef WALL_DIR
#undef WALL_LEVEL
#undef WALL_IDNUM
