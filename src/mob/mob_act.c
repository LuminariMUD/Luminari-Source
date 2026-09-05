/**************************************************************************
 *  File: mob_act.c                                   Part of LuminariMUD *
 *  Usage: Mobile agenda behavior execution           *
 *  Rewritten by Zusuk                                                    *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "spec/spec_dispatch.h"
#include "spec/spec_registry.h"
#include "spec/spec_rol_conversion.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "magic/spells.h"
#include "constants.h"
#include "act.h"
#include "graph.h"
#include "combat/assign_wpn_armor.h"
#include "combat/fight.h"
#include "combat/projectiles.h"
#include "mud_event.h" /* for eSTUNNED */
#include "modify.h"
#include "obj/shop.h"
#include "quest/quest.h"         /* so you can identify questmaster mobiles */
#include "dgscript/dg_scripts.h" /* so you can identify script mobiles */
#include "character/evolutions.h"
#include "magic/psionics.h"
#include "mob_act.h"
#include "mob_spellslots.h"
#include "mob_known_spells.h"
#include "mob_spells.h"
#include "vessels/vessels.h"

/* External function prototypes */
void npc_offensive_spells(struct char_data *ch);
void npc_racial_behave(struct char_data *ch);
bool mob_knows_assigned_spells(struct char_data *ch);


static bool mobile_activity_owner_eligible(const struct char_data *ch)
{
  return ch != NULL && IS_MOB(ch) && world != NULL && IN_ROOM(ch) != NOWHERE &&
         IN_ROOM(ch) <= top_of_world && !MOB_FLAGGED(ch, MOB_NOTDEADYET) &&
         !MOB_FLAGGED(ch, MOB_NO_AI);
}

static bool mobile_has_activity_spec(const struct char_data *ch)
{
  SPECIAL_DECL(*handler);
  const struct spec_definition *definition;
  mob_rnum rnum;

  if (ch == NULL || !MOB_FLAGGED(ch, MOB_SPEC) || no_specials)
    return false;
  rnum = GET_MOB_RNUM(ch);
  if (rnum > top_of_mobt)
    return false;
  handler = mob_index[rnum].func;
  if (handler == NULL)
    return false;
  definition = spec_registry_find_by_handler(handler);
  return definition == NULL ||
         spec_definition_supports_event(definition, SPEC_OWNER_MOBILE, SPEC_EVENT_MOBILE_ACTIVITY);
}

static bool mobile_has_scavenge_work(struct char_data *ch)
{
  struct obj_data *obj;

  if (ch == NULL || !MOB_FLAGGED(ch, MOB_SCAVENGER) || IN_ROOM(ch) == NOWHERE ||
      IN_ROOM(ch) > top_of_world)
    return false;
  for (obj = world[IN_ROOM(ch)].contents; obj != NULL; obj = obj->next_content)
    if (GET_OBJ_COST(obj) > 1 && CAN_GET_OBJ(ch, obj))
      return true;
  return false;
}

static bool mobile_resource_recovery_blocked(const struct char_data *ch)
{
  return FIGHTING(ch) != NULL || !AWAKE(ch) || IS_CASTING(ch) || AFF_FLAGGED(ch, AFF_STUN) ||
         AFF_FLAGGED(ch, AFF_PARALYZED) || AFF_FLAGGED(ch, AFF_DAZED) ||
         char_has_mud_event((struct char_data *)ch, eSTUNNED) || AFF_FLAGGED(ch, AFF_NAUSEATED);
}

static bool mobile_has_resource_recovery_work(const struct char_data *ch)
{
  return mob_spell_slots_need_recovery(ch) || known_spell_slots_need_recovery(ch);
}

mobile_work_mask mobile_activity_room_reaction_reasons(const struct char_data *ch)
{
  if (!mobile_activity_owner_eligible(ch) || FIGHTING(ch))
    return MOBILE_WORK_NONE;
  if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) || MOB_FLAGGED(ch, MOB_ROL_AGGR_RACE_EVIL) ||
      MOB_FLAGGED(ch, MOB_ROL_AGGR_RACE_GOOD) || MOB_FLAGGED(ch, MOB_AGGR_EVIL) ||
      MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) || MOB_FLAGGED(ch, MOB_AGGR_GOOD) || MEMORY(ch) != NULL ||
      MOB_FLAGGED(ch, MOB_ROL_ARCHER) || IS_NPC_CASTER(ch) || IS_PSIONIC(ch) ||
      mob_has_known_spells((struct char_data *)ch))
    return MOBILE_WORK_ROOM_REACTION;
  return MOBILE_WORK_NONE;
}

mobile_work_mask mobile_activity_combat_reaction_reasons(const struct char_data *ch)
{
  if (!mobile_activity_owner_eligible(ch) || FIGHTING(ch))
    return MOBILE_WORK_NONE;
  if (MOB_FLAGGED(ch, MOB_HELPER) || MOB_FLAGGED(ch, MOB_GUARD) ||
      MOB_FLAGGED(ch, MOB_MOB_ASSIST) || MOB_FLAGGED(ch, MOB_LISTEN))
    return MOBILE_WORK_COMBAT_REACTION;
  return MOBILE_WORK_NONE;
}

mobile_work_mask mobile_activity_recurring_reasons(struct char_data *ch)
{
  mobile_work_mask reasons = MOBILE_WORK_NONE;

  if (!mobile_activity_owner_eligible(ch))
    return reasons;
  if (mobile_has_resource_recovery_work(ch))
    reasons |= MOBILE_WORK_RESOURCE_RECOVERY;
  if (FIGHTING(ch))
    return reasons;
  if (mobile_has_activity_spec(ch))
    reasons |= MOBILE_WORK_SPEC_ACTIVITY;
  if (ECHO_COUNT(ch) > 0 && ECHO_ENTRIES(ch) != NULL)
    reasons |= MOBILE_WORK_ECHO;
  if (mobile_has_scavenge_work(ch))
    reasons |= MOBILE_WORK_SCAVENGE;
  if (PATH_SIZE(ch) > 0)
    reasons |= MOBILE_WORK_PATROL;
  if (MOB_FLAGGED(ch, MOB_HUNTER) && HUNTING(ch) != NULL)
    reasons |= MOBILE_WORK_HUNT;
  if (!MOB_FLAGGED(ch, MOB_SENTINEL) && GET_POS(ch) == POS_STANDING && ch->master == NULL &&
      !vessel_npc_is_on_pilot_duty(ch))
    reasons |= MOBILE_WORK_WANDER;
  if (MOB_FLAGGED(ch, MOB_SENTINEL) && GET_MOB_LOADROOM(ch) == IN_ROOM(ch) &&
      GET_POS(ch) != GET_DEFAULT_POS(ch))
    reasons |= MOBILE_WORK_POSTURE;
  return reasons;
}

long mobile_activity_next_resource_recovery_delay(const struct char_data *ch)
{
  time_t now;
  time_t deadline = 0;
  time_t candidate;
  time_t seconds;

  if (!mobile_activity_owner_eligible(ch) || !mobile_has_resource_recovery_work(ch))
    return 0L;
  now = time(0);
  candidate = mob_spell_slot_recovery_deadline(ch);
  if (candidate > 0)
    deadline = candidate;
  candidate = known_spell_slot_recovery_deadline(ch);
  if (candidate > 0 && (deadline == 0 || candidate < deadline))
    deadline = candidate;
  if (deadline <= now)
    return mobile_resource_recovery_blocked(ch) ? (long)PULSE_MOBILE : 1L;
  seconds = deadline - now;
  if (seconds > LONG_MAX / PASSES_PER_SEC)
    return LONG_MAX;
  return (long)seconds * PASSES_PER_SEC;
}

long mobile_activity_next_wander_delay(void)
{
  long rounds = 1L;

  while (rounds < 64L && rand_number(0, 2) != 0)
    rounds++;
  return rounds * (long)PULSE_MOBILE;
}


static struct char_data *run_mobile_activity(struct char_data *start, size_t node_limit,
                                             size_t *nodes_visited_out,
                                             mobile_work_mask requested_work)
{
  struct char_data *ch = NULL, *next_ch = NULL, *vict = NULL, *tmp_char = NULL;
  struct obj_data *obj = NULL, *best_obj = NULL;
  int door = 0, found = FALSE, max = 0, where = -1;
  struct char_data *room_people = NULL; /* Cache for room occupants */
  SPECIAL_DECL(*spec_func);             /* Cache for spec proc function */
  int mob_rnum = 0;                     /* Cache for mob rnum */
  bool disabled = false;
  size_t nodes_visited = 0;

  for (ch = start; ch && nodes_visited < node_limit; ch = next_ch)
  {
    next_ch = ch->next;
    nodes_visited++;

    /* Defensive check - verify character is still valid */
    if (!ch || ch->in_room == NOWHERE)
      continue;

    /* CRITICAL: Skip characters marked for extraction */
    if (MOB_FLAGGED(ch, MOB_NOTDEADYET))
      continue;

    if (IN_ROOM(ch) > top_of_world)
      continue;

    if (!IS_MOB(ch))
      continue;

    if (MOB_FLAGGED(ch, MOB_NO_AI))
      continue;

    disabled = AFF_FLAGGED(ch, AFF_STUN) || AFF_FLAGGED(ch, AFF_PARALYZED) ||
               AFF_FLAGGED(ch, AFF_DAZED) || char_has_mud_event(ch, eSTUNNED) ||
               AFF_FLAGGED(ch, AFF_NAUSEATED);

    /* Examine call for special procedure */
    /* not the AWAKE() type of checks are inside the spec_procs */
    if ((requested_work & MOBILE_WORK_SPEC_ACTIVITY) && MOB_FLAGGED(ch, MOB_SPEC) && !no_specials)
    {
      mob_rnum = GET_MOB_RNUM(ch); /* Cache the rnum lookup */
      spec_func = mob_index[mob_rnum].func;

      if (spec_func == NULL)
      {
        log("MOB ERROR: Mobile '%s' (vnum #%d) has the SPEC flag set but no special procedure "
            "assigned.",
            GET_NAME(ch), GET_MOB_VNUM(ch));
        log("MOB FIX: Either remove the SPEC flag from this mob in medit, OR add it to the "
            "appropriate src/spec assignment inventory.");
        log("MOB FIX: Common spec procs: shop_keeper, guild_guard, snake, cityguard, receptionist, "
            "cryogenicist, postmaster, bank.");
        log("MOB NOTE: The SPEC flag has been automatically removed to prevent further errors. Use "
            "'medit %d' and check 'mob flags'.",
            GET_MOB_VNUM(ch));
        REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
      }
      else
      {
        if ((!disabled || (spec_func == rol_monster_combat &&
                           (rol_seelie_faerie_runs_while_disabled(GET_MOB_VNUM(ch)) ||
                            rol_skriaxit_sandstorm_profile(GET_MOB_VNUM(ch), NULL, NULL) ||
                            rol_planar_vrock_dance_profile(GET_MOB_VNUM(ch))))) &&
            spec_gateway_mobile_activity(ch, spec_func))
          continue; /* go to next char */
      }
    }

    if (disabled)
    {
      send_to_char(ch, "You are unable to move!\r\n");
      continue;
    }

    /* can't do any of the following if not at least AWAKE() and not casting */
    if (!AWAKE(ch) || IS_CASTING(ch))
      continue;

    if (requested_work & MOBILE_WORK_RESOURCE_RECOVERY)
    {
      regenerate_mob_spell_slot(ch);
      regenerate_known_spell_slot(ch);
    }

    /* If the mob has no specproc, do the default actions */

    // entry point for npc race and class behaviour in combat -zusuk
    if (GET_LEVEL(ch) > NEWBIE_LEVEL)
    {
      if ((requested_work & MOBILE_WORK_ROOM_REACTION) && !FIGHTING(ch))
      {
        if (!rand_number(0, 15) && (IS_NPC_CASTER(ch) || mob_has_known_spells(ch)))
        {
          /* Wizards and sorcerers use specialized long-duration pre-buffs. */
          if (npc_room_has_player(ch))
          {
            if (GET_CLASS(ch) == CLASS_WIZARD || GET_CLASS(ch) == CLASS_SORCERER)
              wizard_cast_prebuff(ch);
            else
              npc_spellup(ch);
          }
        }
        else if (!rand_number(0, 15) && IS_PSIONIC(ch))
        {
          if (npc_room_has_player(ch))
            npc_psionic_powerup(ch);
        }
      }
    }

    if (requested_work & MOBILE_WORK_ECHO)
      mobile_echos(ch);

    /* Scavenger (picking up objects) */
    if ((requested_work & MOBILE_WORK_SCAVENGE) && MOB_FLAGGED(ch, MOB_SCAVENGER) &&
        !rand_number(0, 10))
    {
      struct obj_data *room_objs = world[IN_ROOM(ch)].contents;
      if (room_objs) /* Only proceed if there are objects */
      {
        max = 1;
        best_obj = NULL;
        for (obj = room_objs; obj; obj = obj->next_content)
          if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max)
          {
            best_obj = obj;
            max = GET_OBJ_COST(obj);
          }
        if (best_obj != NULL)
        {
          obj_from_room(best_obj);
          obj_to_char(best_obj, ch);
          act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);

          if ((where = find_eq_pos(ch, best_obj, 0)) > 0)
            perform_wear(ch, best_obj, where);

          continue;
        }
      }
    }

    /* Aggressive Mobs */
    if ((requested_work & MOBILE_WORK_ROOM_REACTION) && !MOB_FLAGGED(ch, MOB_HELPER) &&
        (!AFF_FLAGGED(ch, AFF_BLIND) || !AFF_FLAGGED(ch, AFF_CHARM)))
    {
      found = FALSE;
      room_people = world[IN_ROOM(ch)].people; /* Cache room people list */
      int room_is_singlefile =
          ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE); /* Cache specific room flag */
      int mob_is_wimpy = MOB_FLAGGED(ch, MOB_WIMPY);
      int mob_is_encounter = MOB_FLAGGED(ch, MOB_ENCOUNTER);
      int mob_level = GET_LEVEL(ch);

      for (vict = room_people; vict && !found; vict = vict->next_in_room)
      {
        int can_see_vict; /* Cache visibility check */

        if (IS_NPC(vict) && !IS_PET(vict))
          continue;

        if (IS_PET(vict) && IS_NPC(vict->master))
          continue;

        can_see_vict = CAN_SEE(ch, vict); /* Cache this expensive check */
        if (!can_see_vict || (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOHASSLE)))
          continue;

        if (mob_is_wimpy && AWAKE(vict))
          continue;

        if (room_is_singlefile && (ch->next_in_room != vict && vict->next_in_room != ch))
          continue;

        if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) ||
            (MOB_FLAGGED(ch, MOB_ROL_AGGR_RACE_EVIL) && rol_race_is_evil(GET_RACE(vict))) ||
            (MOB_FLAGGED(ch, MOB_ROL_AGGR_RACE_GOOD) && rol_race_is_good(GET_RACE(vict))) ||
            (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
            (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
            (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict)))
        {
          if (IS_ANIMAL(ch) && HAS_FEAT(vict, FEAT_SOUL_OF_THE_FEY))
          {
            continue;
          }
          if (IS_UNDEAD(ch) && HAS_FEAT(vict, FEAT_ONE_OF_US))
          {
            continue;
          }
          if (HAS_FEAT(ch, FEAT_COWARDLY) && dice(1, 4) < 4)
            continue;
          if (mob_is_encounter && ((mob_level - GET_LEVEL(vict)) < 2))
          {
            // We don't want abandoned random encounters killing people they weren't meant for
            hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
            found = TRUE;
            /* CRITICAL: mob may have been extracted during combat */
            if (!ch || ch->in_room == NOWHERE)
              break;
          }
          else if (!mob_is_encounter)
          {
            // all other aggro mobs
            hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
            found = TRUE;
            /* CRITICAL: mob may have been extracted during combat */
            if (!ch || ch->in_room == NOWHERE)
              break;
          }
        }
      }
    }

    /* Mob Memory */
    found = FALSE;
    /* loop through room, check if each person is in memory */
    room_people = world[IN_ROOM(ch)].people;
    for (vict = (requested_work & MOBILE_WORK_ROOM_REACTION) ? room_people : NULL; vict && !found;
         vict = vict->next_in_room)
    {
      /* this function cross-references memory-list with vict */
      if (!is_in_memory(ch, vict))
        continue;

      /* bingo! */
      found = TRUE;
      if (!FIGHTING(ch))
      {
        act("'!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        /* CRITICAL: mob may have been extracted during combat */
        if (!ch || ch->in_room == NOWHERE)
          continue;
      }
    }

    /* NOTE old charmee rebellion - Deprecated by current system
     * use to be here */

    /* Helper Mobs */
    if ((requested_work & MOBILE_WORK_COMBAT_REACTION) &&
        (MOB_FLAGGED(ch, MOB_HELPER) || MOB_FLAGGED(ch, MOB_GUARD)) &&
        !AFF2_FLAGGED(ch, AFF2_ROL_DOCILE) &&
        (!AFF_FLAGGED(ch, AFF_BLIND) || !AFF_FLAGGED(ch, AFF_CHARM)))
    {
      found = FALSE;
      room_people = world[IN_ROOM(ch)].people; /* Re-use cached list */
      int mob_is_guard = MOB_FLAGGED(ch, MOB_GUARD);
      int mob_is_helper = MOB_FLAGGED(ch, MOB_HELPER);

      for (vict = room_people; vict && !found; vict = vict->next_in_room)
      {
        if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
          continue;
        /* Skip if both are in the same group AND at least one is a pet */
        if (GROUP(vict) && GROUP(vict) == GROUP(ch) && (IS_PET(ch) || IS_PET(vict)))
          continue;
        /* Skip group/follower assistance if neither has MOB_MOB_ASSIST flag */
        if ((GROUP(vict) && GROUP(vict) == GROUP(ch)) || (ch->master == vict || vict->master == ch))
        {
          if (!MOB_FLAGGED(ch, MOB_MOB_ASSIST) && !MOB_FLAGGED(vict, MOB_MOB_ASSIST))
            continue;
        }
        /* Skip if in master/follower relationship AND at least one is a pet */
        if ((ch->master == vict || vict->master == ch) && (IS_PET(ch) || IS_PET(vict)))
          continue;
        if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
          continue;
        if (mob_is_guard && !mob_is_helper && !MOB_FLAGGED(vict, MOB_CITIZEN))
          continue;
        if (mob_is_guard && (mob_is_helper || MOB_FLAGGED(vict, MOB_CITIZEN)))
          if (ch->mission_owner && vict->mission_owner && ch->mission_owner != vict->mission_owner)
            continue;
        if (mob_is_helper && IS_ANIMAL(vict))
          continue;

        act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
        hit(ch, FIGHTING(vict), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        found = TRUE;
        /* CRITICAL: mob may have been extracted during combat */
        if (!ch || ch->in_room == NOWHERE)
          break;
      }
      if (found)
        continue;
    }

    /* Mob-to-Mob Assistance (for grouped/following mobs with MOB_MOB_ASSIST flag) */
    if ((requested_work & MOBILE_WORK_COMBAT_REACTION) && MOB_FLAGGED(ch, MOB_MOB_ASSIST) &&
        (!AFF_FLAGGED(ch, AFF_BLIND) && !AFF_FLAGGED(ch, AFF_CHARM)))
    {
      found = FALSE;
      room_people = world[IN_ROOM(ch)].people; /* Re-use cached list */

      for (vict = room_people; vict && !found; vict = vict->next_in_room)
      {
        if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
          continue;
        /* Only assist if in group or follower relationship */
        if (!((GROUP(vict) && GROUP(vict) == GROUP(ch)) ||
              (ch->master == vict || vict->master == ch)))
          continue;
        /* Skip if either is a pet */
        if (IS_PET(ch) || IS_PET(vict))
          continue;
        /* Skip if attacking another NPC or attacking us */
        if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
          continue;
        /* Must have MOB_MOB_ASSIST flag */
        if (!MOB_FLAGGED(vict, MOB_MOB_ASSIST))
          continue;

        act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
        hit(ch, FIGHTING(vict), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        found = TRUE;
        /* CRITICAL: mob may have been extracted during combat */
        if (!ch || ch->in_room == NOWHERE)
          break;
      }
      if (found)
        continue;
    }

    /* Mob Movement */

    /* follow set path for mobile (like patrols) */
    if ((requested_work & MOBILE_WORK_PATROL) && move_on_path(ch))
      continue;

    /* hunt a victim, if applicable */
    if ((requested_work & MOBILE_WORK_HUNT) && MOB_FLAGGED(ch, MOB_HUNTER) && HUNTING(ch) != NULL)
      hunt_victim(ch);

    /* RoL archers fire one room away when their converted equipment provides
     * a usable ranged weapon and ammunition. */
    if ((requested_work & MOBILE_WORK_ROOM_REACTION) && MOB_FLAGGED(ch, MOB_ROL_ARCHER) &&
        !FIGHTING(ch) && !ch->master)
    {
      /* Crossbows and slings carry a loaded-ammo counter that hit() decrements
       * per shot, and the mob AI has no reload step of its own. Without this a
       * converted crossbow archer fires once per load and then falls silent. */
      auto_reload_weapon(ch, TRUE);
    }

    if ((requested_work & MOBILE_WORK_ROOM_REACTION) && MOB_FLAGGED(ch, MOB_ROL_ARCHER) &&
        !FIGHTING(ch) && !ch->master && can_fire_ammo(ch, TRUE))
    {
      found = FALSE;
      for (door = 0; door < DIR_COUNT && !found; door++)
      {
        if (!CAN_GO(ch, door) || ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_PEACEFUL))
          continue;
        for (vict = world[EXIT(ch, door)->to_room].people; vict; vict = vict->next_in_room)
        {
          if ((IS_NPC(vict) && !IS_PET(vict)) || !CAN_SEE(ch, vict) ||
              (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOHASSLE)))
            continue;
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_RANGED);
          found = TRUE;
          break;
        }
      }
      if (found)
        continue;
    }

    /* Converted archers with a throwable anchor, but no usable launcher,
     * use the same one-adjacent-room targeting contract as launcher archers. */
    if ((requested_work & MOBILE_WORK_ROOM_REACTION) && MOB_FLAGGED(ch, MOB_ROL_ARCHER) &&
        !FIGHTING(ch) && !ch->master && !can_fire_ammo(ch, TRUE) &&
        (obj = find_equipped_throwable(ch, &where)) != NULL &&
        set_thrown_projectile_mode(ch, GET_OBJ_VNUM(obj), where))
    {
      found = FALSE;
      for (door = 0; door < DIR_COUNT && !found; door++)
      {
        if (!CAN_GO(ch, door) || ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_PEACEFUL))
          continue;
        for (vict = world[EXIT(ch, door)->to_room].people; vict; vict = vict->next_in_room)
        {
          if ((IS_NPC(vict) && !IS_PET(vict)) || !CAN_SEE(ch, vict) ||
              (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOHASSLE)))
            continue;
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_THROWN);
          found = TRUE;
          break;
        }
      }
      clear_projectile_mode(ch);
      if (found)
        continue;
    }

    /* (mob-listen) is mob interested in fights nearby*/
    if ((requested_work & MOBILE_WORK_COMBAT_REACTION) && MOB_FLAGGED(ch, MOB_LISTEN) &&
        !ch->master)
    {
      for (door = 0; door < DIR_COUNT; door++)
      {
        if (!CAN_GO(ch, door))
          continue;
        for (vict = world[EXIT(ch, door)->to_room].people; vict; vict = vict->next_in_room)
        {
          if (FIGHTING(vict) && !rand_number(0, 3) && !ROOM_FLAGGED(vict->in_room, ROOM_NOTRACK))
          {
            perform_move(ch, door, 1);
            /* CRITICAL: mob may have been extracted during move */
            if (!ch || ch->in_room == NOWHERE)
            {
              if (nodes_visited_out != NULL)
                *nodes_visited_out = nodes_visited;
              return next_ch;
            }
            continue;
          }
        }
      }
    }

    /* random movement */
    if ((requested_work & MOBILE_WORK_WANDER) && !vessel_npc_is_on_pilot_duty(ch))
      if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
          ((door = rand_number(0, 18)) < DIR_COUNT) && CAN_GO(ch, door) &&
          !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB) &&
          !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
          (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
           (world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone)) &&
          (!MOB_FLAGGED(ch, MOB_ROL_STAY_SECTOR) ||
           world[EXIT(ch, door)->to_room].sector_type == world[IN_ROOM(ch)].sector_type))
      {
        /* If the mob is charmed, do not move the mob. */
        if (ch->master == NULL)
        {
          perform_move(ch, door, 1);
          /* CRITICAL: mob may have been extracted during move */
          if (!ch || ch->in_room == NOWHERE)
            continue;
        }
      }

    /* helping group members use to be here, now its in
     * perform_violence() in fight.c */

    /* a function to move mobile back to its loadroom (if sentinel) */
    /*    if (!HUNTING(ch) && !MEMORY(ch) && !ch->master &&
            MOB_FLAGGED(ch, MOB_SENTINEL) && !IS_PET(ch) &&
            GET_MOB_LOADROOM(ch) != IN_ROOM(ch))
      hunt_loadroom(ch);
*/
    // /* pets return to their master */
    // if (GET_POS(ch) == POS_STANDING && IS_PET(ch) && IN_ROOM(ch->master) != IN_ROOM(ch) && !HUNTING(ch))
    // {
    //   HUNTING(ch) = ch->master;
    //   hunt_victim(ch);
    // }

    /* return mobile to preferred (default) position if necessary */
    if ((requested_work & MOBILE_WORK_POSTURE) && GET_POS(ch) != GET_DEFAULT_POS(ch) &&
        MOB_FLAGGED(ch, MOB_SENTINEL) && GET_MOB_LOADROOM(ch) == IN_ROOM(ch))
    {
      if (GET_DEFAULT_POS(ch) == POS_SITTING)
      {
        do_sit(ch, NULL, 0, 0);
      }
      else if (GET_DEFAULT_POS(ch) == POS_RECLINING)
      {
        do_recline(ch, NULL, 0, 0);
      }
      else if (GET_DEFAULT_POS(ch) == POS_RESTING)
      {
        do_rest(ch, NULL, 0, 0);
      }
      else if (GET_DEFAULT_POS(ch) == POS_STANDING)
      {
        do_stand(ch, NULL, 0, 0);
      }
      else if (GET_DEFAULT_POS(ch) == POS_SLEEPING)
      {
        int go_to_sleep = FALSE;
        do_rest(ch, NULL, 0, 0);

        // only go back to sleep if no PCs in the room, and percentage
        if (rand_number(1, 100) <= 10)
        {
          go_to_sleep = TRUE;
          for (tmp_char = world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
          {
            if (!IS_NPC(tmp_char) && CAN_SEE(ch, tmp_char))
            {
              // don't go to sleep
              go_to_sleep = FALSE;
              break;
            }
          }
        }

        if (go_to_sleep == TRUE)
          do_sleep(ch, NULL, 0, 0);
      }
    }

    /* Add new mobile actions here */

  } /* end for() */

  if (nodes_visited_out != NULL)
    *nodes_visited_out = nodes_visited;
  return ch;
}


void mobile_activity_run_scheduled(struct char_data *ch, mobile_work_mask reasons)
{
  if (ch != NULL && reasons != MOBILE_WORK_NONE)
    run_mobile_activity(ch, 1U, NULL, reasons);
}

void mobile_activity_reset(void)
{
}

void mobile_activity_forget_character(struct char_data *ch)
{
  (void)ch;
}
