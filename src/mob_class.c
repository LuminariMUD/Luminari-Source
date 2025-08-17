/**************************************************************************
 *  File: mob_class.c                                 Part of LuminariMUD *
 *  Usage: Mobile class-specific behavior functions                       *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

/* Debug mount behavior - set to 1 to enable debug messages, 0 to disable */
#define DEBUG_MOUNT_BEHAVIOR 0

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "fight.h"
#include "evolutions.h"  /* for EVOLUTION_UNDEAD_APPEARANCE */
#include "mob_utils.h"
#include "mob_race.h"
#include "mob_psionic.h"
#include "mob_spells.h"
#include "mob_class.h"

void npc_ability_behave(struct char_data *ch)
{
  if (dice(1, 2) == 1)
    npc_racial_behave(ch);
  else
  {
    if (GET_CLASS(ch) == CLASS_PSIONICIST)
      npc_offensive_powers(ch);
    else
      npc_offensive_spells(ch);
  }
  return;

  // we need to code the abilities before we go ahead with this
  struct char_data *vict = NULL;
  int num_targets = 0;

  if (!can_continue(ch, TRUE))
    return;

  /* retrieve random valid target and number of targets */
  if (!(vict = npc_find_target(ch, &num_targets)))
    return;
}

// monk behaviour, behave based on level

void npc_monk_behave(struct char_data *ch, struct char_data *vict,
                     int engaged)
{
  /* list of skills to use:
   1) switch opponents
   2) springleap
   3) stunning fist
   4) quivering palm
   */

  /* Combat-only behaviors */
  if (!vict)
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  switch (rand_number(1, 6))
  {
  case 1:
    perform_stunningfist(ch);
    break;
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
    perform_springleap(ch, vict);
    break;
  case 7: /* hahah just kidding */
    perform_quiveringpalm(ch);
    break;
  default:
    break;
  }
}
// rogue behaviour, behave based on level

void npc_rogue_behave(struct char_data *ch, struct char_data *vict,
                      int engaged)
{
  /* list of skills to use:
   1) trip
   2) dirt kick
   3) sap  //todo
   4) backstab / circle
   */
  
  /* Set up sneak attack feat - can do this outside combat */
  if (GET_LEVEL(ch) >= 2 && !HAS_FEAT(ch, FEAT_SNEAK_ATTACK))
  {
    MOB_SET_FEAT(ch, FEAT_SNEAK_ATTACK, (GET_LEVEL(ch)) / 2);
  }

  /* Combat-only behaviors */
  if (!vict)
    return;

  /* almost finished victims, they will stop using these skills -zusuk */
  if (GET_HIT(vict) <= 5)
    return;

  if (!can_continue(ch, TRUE))
    return;

  switch (rand_number(1, 2))
  {
  case 1:
    if (perform_knockdown(ch, vict, SKILL_TRIP, true, true))
      break;
    /* fallthrough */
  case 2:
    if (perform_dirtkick(ch, vict))
    {
      send_to_char(ch, "Succeeded dirtkick\r\n");
      break;
    }
    else
      send_to_char(ch, "Failed dirtkick\r\n");
    /* fallthrough */
  default:
    if (perform_backstab(ch, vict))
      break;
    break;
  }
}
// bard behaviour, behave based on level

void npc_bard_behave(struct char_data *ch, struct char_data *vict,
                     int engaged)
{
  /* list of skills to use:
   1) trip
   2) dirt kick
   3) perform
   4) kick
   */
  /* try to throw up song - works in or out of combat */
  perform_perform(ch);

  /* Combat-only behaviors */
  if (vict)
  {
    if (!can_continue(ch, TRUE))
      return;

    switch (rand_number(1, 3))
    {
    case 1:
      perform_knockdown(ch, vict, SKILL_TRIP, true, true);
      break;
    case 2:
      perform_dirtkick(ch, vict);
      break;
    case 3:
      perform_kick(ch, vict);
      break;
    default:
      break;
    }
  }
}
// warrior behaviour, behave based on circle

void npc_warrior_behave(struct char_data *ch, struct char_data *vict,
                        int engaged)
{
  /* list of skills to use:
   1) rescue
   2) bash
   3) shieldpunch
   4) switch opponents
   */

  /* Combat-only behaviors */
  if (!vict)
    return;

  /* first rescue friends/master */
  if (npc_rescue(ch))
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  switch (rand_number(1, 2))
  {
  case 1:
    if (perform_knockdown(ch, vict, SKILL_BASH, true, true))
      break;
  case 2:
    if (perform_shieldpunch(ch, vict))
      break;
  default:
    break;
  }
}
// ranger behaviour, behave based on level

void npc_ranger_behave(struct char_data *ch, struct char_data *vict,
                       int engaged)
{

  /* list of skills to use:
   1) rescue
   2) switch opponents
   3) call companion
   4) kick
   */

  /* attempt to call companion if appropriate */
  if (npc_should_call_companion(ch, MOB_C_ANIMAL))
    perform_call(ch, MOB_C_ANIMAL, GET_LEVEL(ch));

  /* Combat-only behaviors */
  if (vict)
  {
    /* next rescue friends/master */
    if (npc_rescue(ch))
      return;

    if (!can_continue(ch, TRUE))
      return;

    /* switch opponents attempt */
    if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
      return;

    perform_kick(ch, vict);
  }
}

/* Paladin NPC behavior function
 * 
 * Mount Calling Logic:
 * 1. First checks if NPC should call a mount (npc_should_call_companion)
 *    - Verifies NPC is a Paladin (class 8)
 *    - Checks if mount already exists as follower
 *    - Has 50% chance in combat, 10% out of combat
 * 2. If mount should be called, performs the call (perform_call)
 * 3. Then checks if mount is in room and attempts to mount it
 * 
 * Skills used:
 * - Call mount (celestial mount)
 * - Rescue allies
 * - Lay on hands (self-healing when low)
 * - Smite evil (against evil opponents)
 * - Turn undead (against undead)
 * 
 * DEBUG: Set DEBUG_MOUNT_BEHAVIOR to 1 at top of file to enable debug logs
 */
void npc_paladin_behave(struct char_data *ch, struct char_data *vict,
                        int engaged)
{
  float percent = ((float)GET_HIT(ch) / (float)GET_MAX_HIT(ch)) * 100.0;

  /* list of skills to use:
   1) call mount
   2) rescue
   3) lay on hands
   4) smite evil
   5) switch opponents
   6) turn undead
   */

#if DEBUG_MOUNT_BEHAVIOR
  /* Debug info about NPC attempting mount behavior */
  mudlog(NRM, LVL_IMMORT, TRUE, 
         "DEBUG: npc_paladin_behave called for %s (Paladin, Level: %d, Room: %d)",
         GET_NAME(ch), 
         GET_LEVEL(ch), 
         IN_ROOM(ch));
#endif

  /* attempt to call mount if appropriate */
  if (npc_should_call_companion(ch, MOB_C_MOUNT))
  {
#if DEBUG_MOUNT_BEHAVIOR
    mudlog(NRM, LVL_IMMORT, TRUE, 
           "DEBUG: %s should call mount - calling perform_call()",
           GET_NAME(ch));
#endif
    perform_call(ch, MOB_C_MOUNT, GET_LEVEL(ch));
  }
#if DEBUG_MOUNT_BEHAVIOR
  else
  {
    mudlog(NRM, LVL_IMMORT, TRUE, 
           "DEBUG: %s should NOT call mount (companion check failed)",
           GET_NAME(ch));
  }
#endif
  
  /* attempt to mount if we have a mount and not currently riding */
  if (!RIDING(ch))
  {
    struct char_data *mount = NULL;
    struct follow_type *f = NULL;
    
#if DEBUG_MOUNT_BEHAVIOR
    if (GET_CLASS(ch) == CLASS_PALADIN)
      mudlog(NRM, LVL_IMMORT, TRUE, 
             "DEBUG: %s is not riding, checking for available mount...",
             GET_NAME(ch));
    
    /* Debug: List all followers */
    if (ch->followers)
    {
      mudlog(NRM, LVL_IMMORT, TRUE, 
             "DEBUG: %s has followers, checking each:",
             GET_NAME(ch));
    }
    else
    {
      mudlog(NRM, LVL_IMMORT, TRUE, 
             "DEBUG: %s has NO followers!",
             GET_NAME(ch));
    }
#endif
    
    /* look for our mount in the room */
    for (f = ch->followers; f; f = f->next)
    {
#if DEBUG_MOUNT_BEHAVIOR
      mudlog(NRM, LVL_IMMORT, TRUE, 
             "DEBUG: Checking follower %s - Room: %d (ch room: %d), NPC: %s, MOB_C_MOUNT: %s, AFF_CHARM: %s, RIDDEN: %s",
             GET_NAME(f->follower),
             IN_ROOM(f->follower),
             IN_ROOM(ch),
             IS_NPC(f->follower) ? "Yes" : "No",
             MOB_FLAGGED(f->follower, MOB_C_MOUNT) ? "Yes" : "No",
             AFF_FLAGGED(f->follower, AFF_CHARM) ? "Yes" : "No",
             RIDDEN_BY(f->follower) ? "Yes" : "No");
#endif
      
      if (IN_ROOM(f->follower) == IN_ROOM(ch) && 
          IS_NPC(f->follower) && 
          MOB_FLAGGED(f->follower, MOB_C_MOUNT) &&
          AFF_FLAGGED(f->follower, AFF_CHARM) &&
          !RIDDEN_BY(f->follower))
      {
        mount = f->follower;
#if DEBUG_MOUNT_BEHAVIOR
        mudlog(NRM, LVL_IMMORT, TRUE, 
               "DEBUG: Found suitable mount: %s!",
               GET_NAME(mount));
#endif
        break;
      }
    }
    
    /* if we found our mount, try to mount it */
    if (mount)
    {
#if DEBUG_MOUNT_BEHAVIOR
      mudlog(NRM, LVL_IMMORT, TRUE, 
             "DEBUG: %s attempting to mount %s",
             GET_NAME(ch), GET_NAME(mount));
#endif
      /* NPCs mount directly without the command processor */
      mount_char(ch, mount);
      act("$n mounts $N.", TRUE, ch, 0, mount, TO_ROOM);
#if DEBUG_MOUNT_BEHAVIOR
      mudlog(NRM, LVL_IMMORT, TRUE, 
             "DEBUG: %s successfully mounted %s",
             GET_NAME(ch), GET_NAME(mount));
#endif
      return;
    }
#if DEBUG_MOUNT_BEHAVIOR
    else
    {
      mudlog(NRM, LVL_IMMORT, TRUE, 
             "DEBUG: %s could not find a suitable mount in room",
             GET_NAME(ch));
    }
#endif
  }
#if DEBUG_MOUNT_BEHAVIOR
  else
  {
    mudlog(NRM, LVL_IMMORT, TRUE, 
           "DEBUG: %s is already riding %s",
           GET_NAME(ch), 
           RIDING(ch) ? GET_NAME(RIDING(ch)) : "something");
  }
#endif

  /* Combat-only behaviors */
  if (vict)
  {
    /* first rescue friends/master */
    if (npc_rescue(ch))
      return;

    if (!can_continue(ch, TRUE))
      return;

    /* switch opponents attempt */
    if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
      return;

    if (IS_EVIL(vict))
      perform_smite(ch, SMITE_TYPE_EVIL);

    if (IS_UNDEAD(vict) && GET_LEVEL(ch) > 2)
      perform_turnundead(ch, vict, (GET_LEVEL(ch) - 2));

    if (percent <= 25.0)
      perform_layonhands(ch, ch);
  }
}

/* Blackguard NPC behavior function
 * 
 * Mount Calling Logic:
 * 1. First checks if NPC should call a mount (npc_should_call_companion)
 *    - Verifies NPC is a Blackguard (class 24)
 *    - Checks if mount already exists as follower
 *    - Has 50% chance in combat, 10% out of combat
 * 2. If mount should be called, performs the call (perform_call)
 * 3. Then checks if mount is in room and attempts to mount it
 * 
 * Skills used:
 * - Call mount (fiendish mount)
 * - Rescue allies (if evil aligned)
 * - Smite good (against good opponents)
 * - Command undead (control undead instead of turn)
 * - Touch of corruption (harm touch)
 * 
 * DEBUG: Set DEBUG_MOUNT_BEHAVIOR to 1 at top of file to enable debug logs
 */
void npc_blackguard_behave(struct char_data *ch, struct char_data *vict,
                           int engaged)
{
  float percent = ((float)GET_HIT(ch) / (float)GET_MAX_HIT(ch)) * 100.0;
  struct char_data *mount = NULL;
  struct follow_type *f = NULL;

  /* list of skills to use:
   1) call fiendish mount
   2) rescue
   3) touch of corruption
   4) smite good
   5) switch opponents
   6) command undead
   */

  /* attempt to call mount if appropriate */
  if (npc_should_call_companion(ch, MOB_C_MOUNT))
    perform_call(ch, MOB_C_MOUNT, GET_LEVEL(ch));
  
  /* attempt to mount if we have a mount and not currently riding */
  if (!RIDING(ch))
  {
    /* look for our mount in the room */
    for (f = ch->followers; f; f = f->next)
    {
      if (IN_ROOM(f->follower) == IN_ROOM(ch) && 
          IS_NPC(f->follower) && 
          MOB_FLAGGED(f->follower, MOB_C_MOUNT) &&
          AFF_FLAGGED(f->follower, AFF_CHARM) &&
          !RIDDEN_BY(f->follower))
      {
        mount = f->follower;
        break;
      }
    }
    
    /* if we found our mount, try to mount it */
    if (mount)
    {
      /* NPCs mount directly without the command processor */
      mount_char(ch, mount);
      act("$n mounts $N.", TRUE, ch, 0, mount, TO_ROOM);
      return;
    }
  }

  /* Combat-only behaviors */
  if (vict)
  {
    /* first rescue evil allies/master */
    if (IS_EVIL(ch) && npc_rescue(ch))
      return;

    if (!can_continue(ch, TRUE))
      return;

    /* switch opponents attempt */
    if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
      return;

    /* Blackguard specific combat abilities */
    if (IS_GOOD(vict))
      perform_smite(ch, SMITE_TYPE_GOOD);

    /* Command undead instead of turn - blackguards control undead */
    /* TODO: Implement command undead for blackguards */
    
    /* Touch of corruption - harm touch ability */
    /* TODO: Implement touch of corruption */

    /* Use lay on hands as harm touch on enemies when low on health */
    if (percent <= 25.0)
    {
      /* Blackguards could use negative energy on themselves or harm enemies */
      /* For now, they'll just be more aggressive when low */
    }
  }
}

// dragonrider behaviour
void npc_dragonrider_behave(struct char_data *ch, struct char_data *vict,
                            int engaged)
{
  /* list of skills to use:
   1) call dragon mount
   2) mount dragon if not mounted
   3) use dragon breath weapon
   4) switch opponents
   5) rescue
   */

  /* attempt to call dragon mount if appropriate */
  if (npc_should_call_companion(ch, MOB_C_DRAGON))
    perform_call(ch, MOB_C_DRAGON, GET_LEVEL(ch));
  
  /* attempt to mount if we have a dragon mount and not currently riding */
  if (!RIDING(ch))
  {
    struct char_data *mount = NULL;
    struct follow_type *f = NULL;
    
    /* look for our dragon mount in the room */
    for (f = ch->followers; f; f = f->next)
    {
      if (IN_ROOM(f->follower) == IN_ROOM(ch) && 
          IS_NPC(f->follower) && 
          is_dragon_rider_mount(f->follower) &&
          AFF_FLAGGED(f->follower, AFF_CHARM) &&
          !RIDDEN_BY(f->follower))
      {
        mount = f->follower;
        break;
      }
    }
    
    /* if we found our dragon mount, try to mount it */
    if (mount)
    {
      /* NPCs mount directly without the command processor */
      mount_char(ch, mount);
      act("$n mounts $N.", TRUE, ch, 0, mount, TO_ROOM);
      return;
    }
  }

  /* Combat-only behaviors */
  if (vict)
  {
    /* first rescue friends/master */
    if (npc_rescue(ch))
      return;

    if (!can_continue(ch, TRUE))
      return;

    /* switch opponents attempt */
    if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
      return;

    /* TODO: Add dragon breath weapon usage when mounted */
    /* TODO: Add dragoon point spell usage */
  }
}

// berserk behaviour, behave based on level

void npc_berserker_behave(struct char_data *ch, struct char_data *vict,
                          int engaged)
{
  /* list of skills to use:
   1) rescue
   2) berserk
   3) headbutt
   4) switch opponents
   */

  /* Combat-only behaviors */
  if (!vict)
    return;

  /* first rescue friends/master */
  if (npc_rescue(ch))
    return;

  if (!can_continue(ch, TRUE))
    return;

  /* switch opponents attempt */
  if (!rand_number(0, 2) && npc_switch_opponents(ch, vict))
    return;

  perform_rage(ch);

  perform_headbutt(ch, vict);
}

/* this is our non-caster's entry point in combat AI
 all semi-casters such as ranger/paladin will go through here */
void npc_class_behave(struct char_data *ch)
{
  struct char_data *vict = NULL;
  int num_targets = 0;

#if DEBUG_MOUNT_BEHAVIOR
  /* Debug: Log only PALADIN NPCs entering this function */
  if (GET_CLASS(ch) == CLASS_PALADIN)
  {
    mudlog(NRM, LVL_IMMORT, TRUE, 
           "DEBUG: npc_class_behave ENTRY - %s (PALADIN, Room: %d)",
           GET_NAME(ch), 
           IN_ROOM(ch));
  }
#endif

  if (MOB_FLAGGED(ch, MOB_NOCLASS))
  {
#if DEBUG_MOUNT_BEHAVIOR
    if (GET_CLASS(ch) == CLASS_PALADIN)
    {
      mudlog(NRM, LVL_IMMORT, TRUE, 
             "DEBUG: %s (PALADIN) has MOB_NOCLASS flag - EXITING",
             GET_NAME(ch));
    }
#endif
    return;
  }

  /* retrieve random valid target and number of targets if in combat */
  if (FIGHTING(ch))
  {
    if (!(vict = npc_find_target(ch, &num_targets)))
      return;
  }
  /* Out of combat - just pass NULL for vict, behaviors will handle it */

  switch (GET_CLASS(ch))
  {
  case CLASS_BARD:
    npc_bard_behave(ch, vict, num_targets);
    break;
  case CLASS_BERSERKER:
    npc_berserker_behave(ch, vict, num_targets);
    break;
  case CLASS_PALADIN:
    npc_paladin_behave(ch, vict, num_targets);
    break;
  case CLASS_BLACKGUARD:
    npc_blackguard_behave(ch, vict, num_targets);
    break;
  case CLASS_DRAGONRIDER:
    npc_dragonrider_behave(ch, vict, num_targets);
    break;
  case CLASS_RANGER:
    npc_ranger_behave(ch, vict, num_targets);
    break;
  case CLASS_ROGUE:
    npc_rogue_behave(ch, vict, num_targets);
    break;
  case CLASS_MONK:
    npc_monk_behave(ch, vict, num_targets);
    break;
  case CLASS_WARRIOR:
  case CLASS_WEAPON_MASTER: /*todo!*/
  default:
    npc_warrior_behave(ch, vict, num_targets);
    break;
  }
}