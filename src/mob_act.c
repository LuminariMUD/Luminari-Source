/**************************************************************************
 *  File: mob_act.c                                   Part of LuminariMUD *
 *  Usage: Main mobile activity loop and coordination                     *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "graph.h"
#include "fight.h"
#include "spec_procs.h"
#include "mud_event.h" /* for eSTUNNED */
#include "modify.h"
#include "shop.h"
#include "quest.h"      /* so you can identify questmaster mobiles */
#include "dg_scripts.h" /* so you can identify script mobiles */
#include "evolutions.h"
#include "psionics.h"
#include "mob_act.h"

/* External function prototypes */
void npc_offensive_spells(struct char_data *ch);
void npc_racial_behave(struct char_data *ch);
bool mob_knows_assigned_spells(struct char_data *ch);

void mobile_activity(void)
{
  struct char_data *ch = NULL, *next_ch = NULL, *vict = NULL, *tmp_char = NULL;
  struct obj_data *obj = NULL, *best_obj = NULL;
  int door = 0, found = FALSE, max = 0, where = -1;
  struct char_data *room_people = NULL;  /* Cache for room occupants */
  SPECIAL_DECL(*spec_func);               /* Cache for spec proc function */
  int mob_rnum = 0;                       /* Cache for mob rnum */

  for (ch = character_list; ch; ch = next_ch)
  {
    next_ch = ch->next;

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

    if (AFF_FLAGGED(ch, AFF_STUN) || AFF_FLAGGED(ch, AFF_PARALYZED) || AFF_FLAGGED(ch, AFF_DAZED) ||
        char_has_mud_event(ch, eSTUNNED) || AFF_FLAGGED(ch, AFF_NAUSEATED))
    {
      send_to_char(ch, "You are unable to move!\r\n");
      continue;
    }

    /* Examine call for special procedure */
    /* not the AWAKE() type of checks are inside the spec_procs */
    if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials)
    {
      mob_rnum = GET_MOB_RNUM(ch);  /* Cache the rnum lookup */
      spec_func = mob_index[mob_rnum].func;
      
      if (spec_func == NULL)
      {
        log("MOB ERROR: Mobile '%s' (vnum #%d) has the SPEC flag set but no special procedure assigned.",
            GET_NAME(ch), GET_MOB_VNUM(ch));
        log("MOB FIX: Either remove the SPEC flag from this mob in medit, OR assign a special procedure in the code (spec_assign.c).");
        log("MOB FIX: Common spec procs: shop_keeper, guild_guard, snake, cityguard, receptionist, cryogenicist, postmaster, bank.");
        log("MOB NOTE: The SPEC flag has been automatically removed to prevent further errors. Use 'medit %d' and check 'mob flags'.",
            GET_MOB_VNUM(ch));
        REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
      }
      else
      {
        char actbuf[MAX_INPUT_LENGTH] = "";
        if ((spec_func)(ch, ch, 0, actbuf))
          continue; /* go to next char */
      }
    }

    /* can't do any of the following if not at least AWAKE() and not casting */
    if (!AWAKE(ch) || IS_CASTING(ch))
      continue;

    /* If the mob has no specproc, do the default actions */

    // entry point for npc race and class behaviour in combat -zusuk
    if (GET_LEVEL(ch) > NEWBIE_LEVEL)
    {
      if (FIGHTING(ch))
      {
        
        if (dice(1, 4) == 1)
          npc_racial_behave(ch);
        else if (dice(1, 4) == 2)
          npc_ability_behave(ch);
        else if (dice(1, 4) == 3 && mob_knows_assigned_spells(ch))
          npc_assigned_spells(ch);
        else if (IS_NPC_CASTER(ch))
          npc_offensive_spells(ch);
        else
          npc_class_behave(ch);
        continue;
      }
#if defined(CAMPAIGN_DL)
      else if (!rand_number(0, 15) && MOB_FLAGGED(ch, MOB_BUFF_OUTSIDE_COMBAT) && IS_NPC_CASTER(ch))
      {
        /* not in combat - reduced from 12.5% to 6.25% chance */
        npc_spellup(ch);
      }
      else if (!rand_number(0, 15) && MOB_FLAGGED(ch, MOB_BUFF_OUTSIDE_COMBAT) && IS_PSIONIC(ch))
      {
        /* not in combat - reduced from 12.5% to 6.25% chance */
        npc_psionic_powerup(ch);
      }
#else
      else if (!rand_number(0, 15) && IS_NPC_CASTER(ch))
      {
        /* not in combat - reduced from 12.5% to 6.25% chance */
        npc_spellup(ch);
      }
      else if (!rand_number(0, 15) && IS_PSIONIC(ch))
      {
        /* not in combat - reduced from 12.5% to 6.25% chance */
        npc_psionic_powerup(ch);
      }
#endif
      else if (!rand_number(0, 8) && !IS_NPC_CASTER(ch))
      {
        /* not in combat, non-caster */
        ; // this is where we'd put mob AI to use hide skill, etc
      }
    }

    /* send out mobile echos to room or zone */
    mobile_echos(ch);

    /* Scavenger (picking up objects) */
    if (MOB_FLAGGED(ch, MOB_SCAVENGER) && !rand_number(0, 10))
    {
      struct obj_data *room_objs = world[IN_ROOM(ch)].contents;
      if (room_objs)  /* Only proceed if there are objects */
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
    if (!MOB_FLAGGED(ch, MOB_HELPER) && (!AFF_FLAGGED(ch, AFF_BLIND) ||
                                         !AFF_FLAGGED(ch, AFF_CHARM)))
    {
      found = FALSE;
      room_people = world[IN_ROOM(ch)].people;  /* Cache room people list */
      int room_is_singlefile = ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE);  /* Cache specific room flag */
      int mob_is_wimpy = MOB_FLAGGED(ch, MOB_WIMPY);
      int mob_is_encounter = MOB_FLAGGED(ch, MOB_ENCOUNTER);
      int mob_level = GET_LEVEL(ch);
      
      for (vict = room_people; vict && !found; vict = vict->next_in_room)
      {
        int can_see_vict;  /* Cache visibility check */

        if (IS_NPC(vict) && !IS_PET(vict))
          continue;

        if (IS_PET(vict) && IS_NPC(vict->master))
          continue;

        can_see_vict = CAN_SEE(ch, vict);  /* Cache this expensive check */
        if (!can_see_vict || (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOHASSLE)))
          continue;

        if (mob_is_wimpy && AWAKE(vict))
          continue;

        if (room_is_singlefile &&
            (ch->next_in_room != vict && vict->next_in_room != ch))
          continue;

        if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) ||
            (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
            (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
            (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict))
        )
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
    room_people = world[IN_ROOM(ch)].people;  /* Re-use cached list if still valid */
    for (vict = room_people; vict && !found; vict = vict->next_in_room)
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
    if ((MOB_FLAGGED(ch, MOB_HELPER) || MOB_FLAGGED(ch, MOB_GUARD)) && (!AFF_FLAGGED(ch, AFF_BLIND) ||
                                                                        !AFF_FLAGGED(ch, AFF_CHARM)))
    {
      found = FALSE;
      room_people = world[IN_ROOM(ch)].people;  /* Re-use cached list */
      int mob_is_guard = MOB_FLAGGED(ch, MOB_GUARD);
      int mob_is_helper = MOB_FLAGGED(ch, MOB_HELPER);
      
      for (vict = room_people; vict && !found; vict = vict->next_in_room)
      {
        if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
          continue;
        if (GROUP(vict) && GROUP(vict) == GROUP(ch))
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

    /* Mob Movement */

    /* follow set path for mobile (like patrols) */
    if (move_on_path(ch))
      continue;

    /* hunt a victim, if applicable */
    if (MOB_FLAGGED(ch, MOB_HUNTER))
      hunt_victim(ch);

    /* (mob-listen) is mob interested in fights nearby*/
    if (MOB_FLAGGED(ch, MOB_LISTEN) && !ch->master)
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
              return;
            continue;
          }
        }
      }
    }

    /* random movement */
    if (!rand_number(0, 2)) // customize frequency
      if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
          ((door = rand_number(0, 18)) < DIR_COUNT) && CAN_GO(ch, door) &&
          !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB) &&
          !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
          (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
           (world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone)))
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
    if (GET_POS(ch) != GET_DEFAULT_POS(ch) && MOB_FLAGGED(ch, MOB_SENTINEL) &&
        GET_MOB_LOADROOM(ch) == IN_ROOM(ch))
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
}
