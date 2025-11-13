/**************************************************************************
 *  File: act.offensive.c                              Part of LuminariMUD *
 *  Usage: Player-level commands of an offensive nature.                   *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include <zconf.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "act.h"
#include "fight.h"
#include "mud_event.h"
#include "constants.h"
#include "spec_procs.h"
#include "class.h"
#include "mudlim.h"
#include "actions.h"
#include "actionqueues.h"
#include "assign_wpn_armor.h"
#include "feats.h"
#include "missions.h"
#include "domains_schools.h"
#include "encounters.h"
#include "constants.h"
#include "spec_procs.h" /* for is_wearing() */
#include "evolutions.h"
#include "perks.h"

/* externs */
extern char cast_arg2[MAX_INPUT_LENGTH];

int roll_initiative(struct char_data *ch);
/* Returns true if the Avatar of the Elements granted a free ki use (25% chance)
 * Caller should still ensure the player had a ki point available before calling. */
static bool avatar_consumes_ki_free(struct char_data *ch)
{
  if (!ch || IS_NPC(ch))
    return FALSE;

  if (affected_by_spell(ch, PERK_MONK_AVATAR_OF_ELEMENTS))
  {
    if (rand_number(1, 100) <= 25)
    {
      send_to_char(ch, "Your Avatar of the Elements shimmers, conserving your ki!\r\n");
      return TRUE;
    }
  }

  return FALSE;
}

/* Helper: consume a ki point unless Avatar grants it for free */
static void maybe_consume_ki(struct char_data *ch, int feat_id)
{
  if (IS_NPC(ch))
    return;

  if (avatar_consumes_ki_free(ch))
    return; /* free */

  start_daily_use_cooldown(ch, feat_id);
}
void create_wall(struct char_data *ch, int room, int dir, int type, int level);

/* defines */
#define RAGE_AFFECTS 7
#define SACRED_FLAMES_AFFECTS 1
#define INNER_FIRE_AFFECTS 6
#define D_STANCE_AFFECTS 4

/**** Utility functions *******/

// Centralize the rage bonus calculation logic.
int get_rage_bonus(struct char_data *ch)
{
  int bonus;

  if (!ch)
    return 0;

  if (IS_NPC(ch) || IS_MORPHED(ch))
  {
    bonus = (GET_LEVEL(ch) / 3) + 3;
  }
  else
  {
    bonus = 4;
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_GREATER_RAGE))
      bonus += 2;
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_MIGHTY_RAGE))
      bonus += 3;
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_INDOMITABLE_RAGE))
      bonus += 3;
  }
  return bonus;
}

/* simple function to check whether ch has a piercing weapon, has 3 modes:
   wield == 1  --  primary one hand weapon
   wield == 2  --  off hand weapon
   wield == 3  --  two hand weapon
 */

/* NOTE - this is probably deprecated by the IS_PIERCE() macro */
bool has_piercing_weapon(struct char_data *ch, int wield)
{
  if (!ch)
    return FALSE;

  if (wield < 1 || wield > 3)
    return FALSE;

  if (wield == 1 && GET_EQ(ch, WEAR_WIELD_1) &&
      IS_PIERCE(GET_EQ(ch, WEAR_WIELD_1)))
  {
    return TRUE;
  }

  if (wield == 2 && GET_EQ(ch, WEAR_WIELD_OFFHAND) &&
      GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD_OFFHAND), 3) == TYPE_PIERCE - TYPE_HIT)
  {
    return TRUE;
  }

  if (wield == 3 && GET_EQ(ch, WEAR_WIELD_2H) &&
      GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD_2H), 3) == TYPE_PIERCE - TYPE_HIT)
  {
    return TRUE;
  }

  return FALSE;
}

/*  End utility */

/* death arrow engine: The death arrow is reliant on a successful RANGED attack */
void perform_deatharrow(struct char_data *ch)
{

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_ARROW_OF_DEATH);

  send_to_char(ch, "Your body glows with a \tDblack aura\tn as the power of \tDdeath\tn enters your ammo pouch!\r\n");
  act("$n's body begins to glow with a \tDblack aura\tn!", FALSE, ch, 0, 0, TO_ROOM);

  /* glowing with powahhh */
  struct affected_type af;
  new_affect(&af);
  af.spell = SKILL_DEATH_ARROW;
  af.duration = 24;
  affect_to_char(ch, &af);
}

/* quivering palm engine: The quivering palm is reliant on a successful UNARMED
 * attack (or an attack with a KI_STRIKE weapon) */
void perform_quiveringpalm(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_QUIVERING_PALM;
  af.duration = 24;

  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_QUIVERING_PALM);

  send_to_char(ch, "You beging to meditate and focus all your ki energy towards your palms...  \tBYour body begins to vibrate with massive amounts of \tYenergy\tB.\tn");
  act("$n's body begins to \tBvibrate\tn with massive amounts of \tYenergy\tn!", FALSE, ch, 0, 0, TO_ROOM);
}

/* stunningfist engine */

/* The stunning fist is reliant on a successful UNARMED attack (or an attack with a KI_STRIKE weapon) */
void perform_stunningfist(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_STUNNING_FIST;
  af.duration = 24;

  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "You focus your Ki energies and prepare a disabling unarmed attack.\r\n");
  act("$n's focuses $s Ki, preparing a disabling unarmed attack!", FALSE, ch, 0, 0, TO_ROOM);
}

void perform_crushingblow(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_CRUSHING_BLOW;
  af.duration = 24;

  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "You focus your Ki energies and prepare a devastating crushing blow.\r\n");
  act("$n focuses $s Ki, preparing a devastating crushing blow!", FALSE, ch, 0, 0, TO_ROOM);
}

void perform_shatteringstrike(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_SHATTERING_STRIKE;
  af.duration = 24;

  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "You focus your Ki to channel overwhelming force into your next strike!\r\n");
  act("$n channels devastating Ki energy, preparing a bone-shattering strike!", FALSE, ch, 0, 0, TO_ROOM);
}

void perform_vanishingtechnique(struct char_data *ch, int spell_num)
{
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  if (spell_num == SPELL_GREATER_INVIS)
  {
    send_to_char(ch, "You focus your Ki and completely fade from existence!\r\n");
    act("$n focuses $s Ki and completely vanishes from sight!", FALSE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    send_to_char(ch, "You focus your Ki and fade from sight!\r\n");
    act("$n focuses $s Ki and vanishes into shadow!", FALSE, ch, 0, 0, TO_ROOM);
  }
  
  call_magic(ch, ch, NULL, spell_num, 0, CLASS_LEVEL(ch, CLASS_MONK), CAST_INNATE);
}

void perform_shadowclone(struct char_data *ch)
{
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "You focus your Ki and create illusory duplicates of yourself!\r\n");
  act("$n focuses $s Ki and suddenly multiple images of $m appear!", FALSE, ch, 0, 0, TO_ROOM);
  
  call_magic(ch, ch, NULL, SPELL_MIRROR_IMAGE, 0, CLASS_LEVEL(ch, CLASS_MONK), CAST_INNATE);
}

void perform_smokebomb(struct char_data *ch)
{
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "You focus your Ki and release a cloud of obscuring darkness!\r\n");
  act("$n focuses $s Ki and explodes into a cloud of darkness!", FALSE, ch, 0, 0, TO_ROOM);
  
  call_magic(ch, ch, NULL, SPELL_DARKNESS, 0, CLASS_LEVEL(ch, CLASS_MONK), CAST_INNATE);
}

void perform_miststance(struct char_data *ch)
{
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "\tCYou focus your Ki and your body transforms into mist!\tn\r\n");
  act("\tC$n focuses $s Ki and $s body transforms into a cloud of mist!\tn", FALSE, ch, 0, 0, TO_ROOM);
  
  call_magic(ch, ch, NULL, SPELL_GASEOUS_FORM, 0, CLASS_LEVEL(ch, CLASS_MONK), CAST_INNATE);
}

void perform_icerabbit(struct char_data *ch, struct char_data *vict)
{
  int dam = 0;
  bool same_room = FALSE;

  if (!ch || !vict)
    return;

  /* Consume ki (25% chance free while Avatar active) */
  maybe_consume_ki(ch, FEAT_STUNNING_FIST);

  /* Calculate damage: 3d6 cold damage */
  dam = dice(3, 6);

  /* Avatar of the Elements adds +2d6 elemental damage */
  if (affected_by_spell(ch, PERK_MONK_AVATAR_OF_ELEMENTS))
  {
    int extra = dice(2, 6);
    dam += extra;
  }

  /* Check if in same room */
  same_room = (IN_ROOM(ch) == IN_ROOM(vict));

  if (same_room)
  {
    send_to_char(ch, "\tWYou summon a swarm of ice rabbits that leap toward %s!\tn\r\n", GET_NAME(vict));
    act("\tW$n summons a swarm of spectral ice rabbits that leap toward you!\tn", FALSE, ch, 0, vict, TO_VICT);
    act("\tW$n summons a swarm of spectral ice rabbits that leap toward $N!\tn", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  else
  {
    send_to_char(ch, "\tWYou summon a swarm of ice rabbits that bound away toward the distance!\tn\r\n");
    send_to_char(vict, "\tWA swarm of spectral ice rabbits suddenly appears and leaps at you!\tn\r\n");
    act("\tWA swarm of spectral ice rabbits suddenly appears and leaps at $n!\tn", FALSE, vict, 0, 0, TO_ROOM);
  }

  /* Apply cold damage */
  damage(ch, vict, dam, SKILL_SWARMING_ICE_RABBIT, DAM_COLD, FALSE);
}

void perform_shadowwalk(struct char_data *ch)
{
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "You channel your Ki through the shadows, gaining supernatural mobility!\r\n");
  act("$n channels $s Ki and blends with the shadows, moving with supernatural grace!", FALSE, ch, 0, 0, TO_ROOM);
  
  call_magic(ch, ch, NULL, SPELL_WATERWALK, 0, CLASS_LEVEL(ch, CLASS_MONK), CAST_INNATE);
  call_magic(ch, ch, NULL, SPELL_SPIDER_CLIMB, 0, CLASS_LEVEL(ch, CLASS_MONK), CAST_INNATE);
}

void perform_blinding_speed(struct char_data *ch)
{
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "You focus your Ki, moving with blinding speed!\r\n");
  act("$n focuses $s Ki and begins moving with blinding speed!", FALSE, ch, 0, 0, TO_ROOM);
  
  call_magic(ch, ch, NULL, SPELL_HASTE, 0, CLASS_LEVEL(ch, CLASS_MONK), CAST_INNATE);
}

void perform_voidstrike(struct char_data *ch)
{
  if (!IS_NPC(ch))
  {
    GET_VOID_STRIKE_COOLDOWN(ch) = time(0) + 60; /* 1 minute cooldown */
  }

  GET_VOID_STRIKE_TIMER(ch) = 1; /* Lasts 1 round - affects next attack */

  send_to_char(ch, "You channel your Ki into the void, preparing a devastating strike!\r\n");
  act("$n's fist crackles with dark energy as $e channels the void!", FALSE, ch, 0, 0, TO_ROOM);
}

/* Way of Four Elements - Tier 3 Abilities */
int flamesofphoenix_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  int dam, save_level;
  struct affected_type af;

  /* Calculate damage: 8d6 fire damage */
  dam = dice(8, 6);

  /* Avatar of the Elements adds +2d6 elemental damage */
  if (affected_by_spell(ch, PERK_MONK_AVATAR_OF_ELEMENTS))
  {
    int extra = dice(2, 6);
    dam += extra;
  }

  /* Calculate effective level for DC: monk level / 2
   * savingthrow will add 10 + this level + WIS bonus to get DC = 10 + WIS bonus + (monk level / 2) */
  save_level = MONK_TYPE(ch);

  /* Apply reflex save - half damage on success, set on fire on failure */
  if (savingthrow(ch, tch, SAVING_REFL, 0, CAST_INNATE, save_level, NOSCHOOL))
  {
    /* Successful save - half damage */
    dam /= 2;
    act("\tRYou twist away from the flames, reducing the damage!\tn", FALSE, ch, 0, tch, TO_VICT);
    act("\tR$N twists away from the flames!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
  }
  else
  {
    /* Failed save - set on fire for 2 rounds */
    new_affect(&af);
    af.spell = SKILL_FLAMES_OF_PHOENIX;
    af.duration = 2;
    af.location = APPLY_NONE;
    SET_BIT_AR(af.bitvector, AFF_ON_FIRE);

    affect_to_char(tch, &af);

    act("\tRYou are engulfed in flames!\tn", FALSE, ch, 0, tch, TO_VICT);
    act("\tR$N is engulfed in flames!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
  }

  /* Apply fire damage with resistance checks */
  damage(ch, tch, dam, SKILL_FLAMES_OF_PHOENIX, DAM_FIRE, FALSE);

  return TRUE;
}

void perform_flamesofphoenix(struct char_data *ch)
{
  int targets_hit;

  send_to_char(ch, "\tRYou summon the Flames of the Phoenix, unleashing an inferno!\tn\r\n");
  act("\tR$n channels $s ki into a brilliant phoenix of flame that explodes in every direction!\tn", FALSE, ch, 0, 0, TO_ROOM);

  /* Use the centralized AoE system */
  targets_hit = aoe_effect(ch, SKILL_FLAMES_OF_PHOENIX, flamesofphoenix_callback, NULL);

  /* Consume ki (25% chance free while Avatar active) */
  maybe_consume_ki(ch, FEAT_STUNNING_FIST);

  if (targets_hit == 0)
  {
    send_to_char(ch, "The phoenix flames dissipate with no enemies in range.\r\n");
  }
}

int waveofrollingearth_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  int dam, save_level;

  /* Skip flying or levitating targets - they're not touching the ground */
  if (AFF_FLAGGED(tch, AFF_FLYING) || AFF_FLAGGED(tch, AFF_LEVITATE))
  {
    act("\tyThe rolling earth passes harmlessly beneath $N!\tn", FALSE, ch, 0, tch, TO_CHAR);
    act("\tyThe rolling earth passes harmlessly beneath you!\tn", FALSE, ch, 0, tch, TO_VICT);
    act("\tyThe rolling earth passes harmlessly beneath $N!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
    return TRUE; /* Still counts as a target */
  }

  /* Calculate damage: 8d6 earth damage */
  dam = dice(8, 6);

  /* Avatar of the Elements adds +2d6 elemental damage */
  if (affected_by_spell(ch, PERK_MONK_AVATAR_OF_ELEMENTS))
  {
    int extra = dice(2, 6);
    dam += extra;
  }

  /* Calculate effective level for DC */
  save_level = MONK_TYPE(ch);

  /* Apply reflex save - if they fail, knock them prone */
  if (savingthrow(ch, tch, SAVING_REFL, 0, CAST_INNATE, save_level, NOSCHOOL))
  {
    /* Successful save - they keep their footing */
    act("\tyYou manage to keep your footing as the earth rolls beneath you!\tn", FALSE, ch, 0, tch, TO_VICT);
    act("\ty$N keeps $S footing despite the rolling earth!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
  }
  else
  {
    /* Failed save - knock them prone */
    if (GET_POS(tch) > POS_SITTING)
    {
      change_position(tch, POS_SITTING);
      act("\tyYou are knocked to the ground by the rolling earth!\tn", FALSE, ch, 0, tch, TO_VICT);
      act("\ty$N is knocked to the ground by the rolling earth!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
      USE_MOVE_ACTION(tch);
    }
  }

  /* Apply earth damage with resistance checks */
  damage(ch, tch, dam, SKILL_WAVE_OF_ROLLING_EARTH, DAM_EARTH, FALSE);

  return TRUE;
}

void perform_waveofrollingearth(struct char_data *ch)
{
  int targets_hit;

  send_to_char(ch, "\tyYou strike the ground, sending a wave of rolling earth outward!\tn\r\n");
  act("\ty$n strikes the ground with tremendous force, causing the earth to roll and shake!\tn", FALSE, ch, 0, 0, TO_ROOM);

  /* Use the centralized AoE system */
  targets_hit = aoe_effect(ch, SKILL_WAVE_OF_ROLLING_EARTH, waveofrollingearth_callback, NULL);

  /* Consume ki (25% chance free while Avatar active) */
  maybe_consume_ki(ch, FEAT_STUNNING_FIST);

  if (targets_hit == 0)
  {
    send_to_char(ch, "The wave of earth rolls outward but finds no enemies.\r\n");
  }
}

void perform_ridethewind(struct char_data *ch)
{
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "\tCYou channel the winds, gaining the ability to fly!\tn\r\n");
  act("\tC$n channels the winds and begins to float above the ground!\tn", FALSE, ch, 0, 0, TO_ROOM);
  
  call_magic(ch, ch, NULL, SPELL_FLY, 0, CLASS_LEVEL(ch, CLASS_MONK), CAST_INNATE);
}

void perform_eternalmountaindefense(struct char_data *ch)
{
  struct affected_type af;
  struct damage_reduction_type *new_dr;

  /* Check for conflicting effects */
  if (affected_by_spell(ch, SPELL_STONESKIN) ||
      affected_by_spell(ch, SPELL_IRONSKIN) ||
      affected_by_spell(ch, SPELL_EPIC_WARDING))
  {
    send_to_char(ch, "A magical ward is already in effect, preventing Eternal Mountain Defense.\r\n");
    return;
  }

  /* Check if already active */
  if (affected_by_spell(ch, SKILL_ETERNAL_MOUNTAIN_DEFENSE))
  {
    send_to_char(ch, "You are already protected by Eternal Mountain Defense.\r\n");
    return;
  }

  send_to_char(ch, "\tyYour body becomes as unyielding as the eternal mountain!\tn\r\n");
  act("\ty$n's form becomes rigid and unyielding, like an eternal mountain!\tn", FALSE, ch, 0, 0, TO_ROOM);

  /* Create the affect for tracking */
  new_affect(&af);
  af.spell = SKILL_ETERNAL_MOUNTAIN_DEFENSE;
  af.duration = 100; /* Duration in combat rounds */
  af.location = APPLY_DR;
  af.modifier = 0;
  affect_to_char(ch, &af);

  /* Create the damage reduction structure */
  CREATE(new_dr, struct damage_reduction_type, 1);

  new_dr->duration = 100;
  new_dr->bypass_cat[0] = DR_BYPASS_CAT_NONE; /* 5/- (nothing bypasses) */
  new_dr->bypass_val[0] = 0;

  new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
  new_dr->bypass_val[1] = 0;

  new_dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
  new_dr->bypass_val[2] = 0;

  new_dr->amount = 5; /* 5 points of DR */
  new_dr->max_damage = 100; /* Absorbs up to 100 HP total */
  new_dr->spell = SKILL_ETERNAL_MOUNTAIN_DEFENSE;
  new_dr->feat = FEAT_UNDEFINED;
  new_dr->next = GET_DR(ch);
  GET_DR(ch) = new_dr;

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);
}
/* Event handler for Fist of Four Thunders lightning strikes */
EVENTFUNC(event_fist_of_four_thunders)
{
  struct char_data *ch = NULL, *vict = NULL, *next_vict = NULL;
  struct mud_event_data *pMudEvent = NULL;
  int num_enemies = 0, target_num, dam, strikes_remaining;

  if (event_obj == NULL)
    return 0;

  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  /* Check if character is still valid and playing */
  if (!ch || !IS_PLAYING(ch->desc))
    return 0;

  /* Check how many strikes remain */
  strikes_remaining = atoi(pMudEvent->sVariables);
  if (strikes_remaining <= 0)
    return 0;

  /* Count enemies in the room - NPCs for players, players for NPCs */
  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
  {
    if (vict == ch || !CAN_SEE(ch, vict))
      continue;

    /* Skip allies: same group */
    if (GROUP(vict) && GROUP(ch) && GROUP(ch) == GROUP(vict))
      continue;

    /* Skip allies: charmed NPCs of the caster */
    if (IS_NPC(vict) && vict->master && AFF_FLAGGED(vict, AFF_CHARM) && vict->master == ch)
      continue;

    /* Skip allies: charmed NPCs of group members */
    if (IS_NPC(vict) && vict->master && AFF_FLAGGED(vict, AFF_CHARM) &&
        GROUP(vict->master) && GROUP(ch) && GROUP(ch) == GROUP(vict->master))
      continue;

    /* Skip allies: master (if caster is charmed) */
    if (ch->master && AFF_FLAGGED(ch, AFF_CHARM) && ch->master == vict)
      continue;

    /* Skip allies: group members of master (if caster is charmed) */
    if (ch->master && AFF_FLAGGED(ch, AFF_CHARM) &&
        GROUP(ch->master) && GROUP(vict) && GROUP(vict) == GROUP(ch->master))
      continue;

    /* Skip NPCs fighting other NPCs (unless charmed) */
    if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM) && IS_NPC(vict))
      continue;

    /* Now count valid enemies */
    if ((!IS_NPC(ch) && IS_NPC(vict)) || (IS_NPC(ch) && !IS_NPC(vict)))
      num_enemies++;
  }

  /* If there are enemies, pick a random one */
  if (num_enemies > 0)
  {
    target_num = rand_number(1, num_enemies);
    num_enemies = 0;
    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_vict)
    {
      next_vict = vict->next_in_room;

      if (vict == ch || !CAN_SEE(ch, vict))
        continue;

      /* Skip allies: same group */
      if (GROUP(vict) && GROUP(ch) && GROUP(ch) == GROUP(vict))
        continue;

      /* Skip allies: charmed NPCs of the caster */
      if (IS_NPC(vict) && vict->master && AFF_FLAGGED(vict, AFF_CHARM) && vict->master == ch)
        continue;

      /* Skip allies: charmed NPCs of group members */
      if (IS_NPC(vict) && vict->master && AFF_FLAGGED(vict, AFF_CHARM) &&
          GROUP(vict->master) && GROUP(ch) && GROUP(ch) == GROUP(vict->master))
        continue;

      /* Skip allies: master (if caster is charmed) */
      if (ch->master && AFF_FLAGGED(ch, AFF_CHARM) && ch->master == vict)
        continue;

      /* Skip allies: group members of master (if caster is charmed) */
      if (ch->master && AFF_FLAGGED(ch, AFF_CHARM) &&
          GROUP(ch->master) && GROUP(vict) && GROUP(vict) == GROUP(ch->master))
        continue;

      /* Skip NPCs fighting other NPCs (unless charmed) */
      if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM) && IS_NPC(vict))
        continue;

      /* Now check valid enemies and select target */
      if ((!IS_NPC(ch) && IS_NPC(vict)) || (IS_NPC(ch) && !IS_NPC(vict)))
      {
        num_enemies++;
        if (num_enemies == target_num)
        {
          /* Strike the target with lightning */
          dam = dice(3, 10);

          /* Avatar of the Elements adds +2d6 elemental damage */
          if (affected_by_spell(ch, PERK_MONK_AVATAR_OF_ELEMENTS))
          {
            int extra = dice(2, 6);
            dam += extra;
          }

          act("\tBA lightning bolt streaks from $n and strikes $N!\tn", FALSE, ch, 0, vict, TO_NOTVICT);
          act("\tBA lightning bolt streaks from $n and strikes YOU!\tn", FALSE, ch, 0, vict, TO_VICT);
          act("\tBYour residual thunder energy strikes $N with lightning!\tn", FALSE, ch, 0, vict, TO_CHAR);
          damage(ch, vict, dam, SKILL_FIST_OF_FOUR_THUNDERS, DAM_ELECTRIC, FALSE);
          break;
        }
      }
    }
  }

  /* Decrement strikes remaining */
  strikes_remaining--;
  if (strikes_remaining > 0)
  {
    /* Schedule next lightning strike */
    char buf[20];
    snprintf(buf, sizeof(buf), "%d", strikes_remaining);
    attach_mud_event(new_mud_event(eFIST_OF_FOUR_THUNDERS, ch, buf), PULSE_VIOLENCE);
  }

  return 0;
}

int fistoffourthunders_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  int dam;

  /* Calculate damage: 4d6 sound damage */
  dam = dice(4, 6);

  /* Avatar of the Elements adds +2d6 elemental damage */
  if (affected_by_spell(ch, PERK_MONK_AVATAR_OF_ELEMENTS))
  {
    int extra = dice(2, 6);
    dam += extra;
  }

  act("\tBYou are struck by a deafening thunderclap!\tn", FALSE, ch, 0, tch, TO_VICT);
  act("\tB$N is struck by a deafening thunderclap!\tn", FALSE, ch, 0, tch, TO_NOTVICT);

  /* Apply sound damage with resistance checks */
  damage(ch, tch, dam, SKILL_FIST_OF_FOUR_THUNDERS, DAM_SOUND, FALSE);

  return TRUE;
}

void perform_fistoffourthunders(struct char_data *ch)
{
  int targets_hit;

  send_to_char(ch, "\tBYou unleash the Fist of Four Thunders, sending shockwaves of sound and lightning!\tn\r\n");
  act("\tB$n's fist strikes with the fury of thunder, unleashing devastating shockwaves!\tn", FALSE, ch, 0, 0, TO_ROOM);

  /* Use the centralized AoE system for initial sound damage */
  targets_hit = aoe_effect(ch, SKILL_FIST_OF_FOUR_THUNDERS, fistoffourthunders_callback, NULL);

  /* Schedule the first lightning strike event (3 strikes total) */
  attach_mud_event(new_mud_event(eFIST_OF_FOUR_THUNDERS, ch, "3"), PULSE_VIOLENCE);

  /* Consume ki (25% chance free while Avatar active) */
  maybe_consume_ki(ch, FEAT_STUNNING_FIST);

  if (targets_hit == 0)
  {
    send_to_char(ch, "The thunderous shockwave dissipates with no enemies in range.\r\n");
  }
}

void perform_riverofhungryflame(struct char_data *ch, int dir)
{
  struct obj_data *wall = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  /* Check if there's already a wall in that direction */
  for (wall = world[IN_ROOM(ch)].contents; wall; wall = wall->next_content)
  {
    if (GET_OBJ_TYPE(wall) == ITEM_WALL && GET_OBJ_VAL(wall, WALL_DIR) == dir)
    {
      send_to_char(ch, "There is already a wall in that direction.\r\n");
      return;
    }
  }

  /* Check if there's an open exit in that direction */
  if (!CAN_GO(ch, dir))
  {
    send_to_char(ch, "There is no open exit in that direction where you can put a wall in.\r\n");
    return;
  }

  /* Consume ki (25% chance free while Avatar active) */
  maybe_consume_ki(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "\tRYou create a River of Hungry Flame, a wall of searing fire to the %s!\tn\r\n", dirs[dir]);
  snprintf(buf, sizeof(buf), "\tR$n creates a wall of intense flames to the %s that burns everything in its path!\tn", dirs[dir]);
  act(buf, FALSE, ch, 0, 0, TO_ROOM);
  
  /* Create wall using the same system as SPELL_WALL_OF_FIRE */
  create_wall(ch, IN_ROOM(ch), dir, WALL_TYPE_FIRE, MONK_TYPE(ch));
}

/* Way of Four Elements - Tier 4 Capstone Abilities */

/* Callback for Breath of Winter AoE effect */
int breathofwinter_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  int save_level, dam;
  struct affected_type af;

  /* Calculate effective level for DC: WIS bonus + (monk level / 2)
   * savingthrow will add 10 + this level + WIS bonus to get DC = 10 + WIS bonus + (monk level / 2) */
  save_level = GET_WIS_BONUS(ch) + (MONK_TYPE(ch) / 2);

  /* Calculate damage: 10d6 cold damage */
  dam = dice(10, 6);

  /* Avatar of the Elements adds +2d6 elemental damage */
  if (affected_by_spell(ch, PERK_MONK_AVATAR_OF_ELEMENTS))
  {
    int extra = dice(2, 6);
    dam += extra;
  }

  /* Apply cold damage */
  damage(ch, tch, dam, SKILL_BREATH_OF_WINTER, DAM_COLD, FALSE);

  /* Check for fortitude save - if failed, apply slow effect */
  if (!savingthrow(ch, tch, SAVING_FORT, 0, CAST_INNATE, save_level, NOSCHOOL))
  {
    /* Failed save - apply slow effect for 3 rounds */
    new_affect(&af);
    af.spell = SKILL_BREATH_OF_WINTER;
    af.duration = 3;
    SET_BIT_AR(af.bitvector, AFF_SLOW);
    affect_join(tch, &af, FALSE, FALSE, FALSE, FALSE);
    
    send_to_char(tch, "\tWYou are \tCslowed\tW by the freezing cold!\tn\r\n");
    act("\tW$N is \tCslowed\tW by the freezing cold!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
  }

  return 1; /* Target was affected */
}

void perform_breathofwinter(struct char_data *ch)
{
  /* Consume ki (25% chance free while Avatar active) */
  maybe_consume_ki(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "\tWYou unleash the Breath of Winter, a devastating blast of absolute cold!\tn\r\n");
  act("\tW$n breathes forth a blast of freezing cold that chills everything in the area!\tn", FALSE, ch, 0, 0, TO_ROOM);
  
  /* Use aoe_effect to hit all valid targets in the room */
  aoe_effect(ch, SKILL_BREATH_OF_WINTER, breathofwinter_callback, NULL);
}

void perform_elementalembodiment(struct char_data *ch, int element_type)
{
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  /* Set transformation timer and element type */
  GET_ELEMENTAL_EMBODIMENT_TIMER(ch) = 10; /* Lasts 1 minute (10 rounds) */
  GET_ELEMENTAL_EMBODIMENT_TYPE(ch) = element_type;

  switch (element_type)
  {
    case 1: /* Fire */
      send_to_char(ch, "\tRYou transform into a being of living flame!\tn\r\n");
      act("\tR$n transforms into a being of pure fire!\tn", FALSE, ch, 0, 0, TO_ROOM);
      call_magic(ch, ch, NULL, SPELL_FIRE_SHIELD, 0, CLASS_LEVEL(ch, CLASS_MONK), CAST_INNATE);
      break;
    case 2: /* Water */
      send_to_char(ch, "\tCYou transform into a being of flowing water!\tn\r\n");
      act("\tC$n transforms into a being of pure water!\tn", FALSE, ch, 0, 0, TO_ROOM);
      call_magic(ch, ch, NULL, SPELL_COLD_SHIELD, 0, CLASS_LEVEL(ch, CLASS_MONK), CAST_INNATE);
      break;
    case 3: /* Air */
      send_to_char(ch, "\tWYou transform into a being of pure air!\tn\r\n");
      act("\tW$n transforms into a being of swirling wind!\tn", FALSE, ch, 0, 0, TO_ROOM);
      break;
    case 4: /* Earth */
      send_to_char(ch, "\tyYou transform into a being of solid stone!\tn\r\n");
      act("\ty$n transforms into a being of living earth!\tn", FALSE, ch, 0, 0, TO_ROOM);
      break;
    default:
      send_to_char(ch, "You must choose an element: fire, water, air, or earth.\r\n");
      return;
  }
}

void perform_firesnake(struct char_data *ch)
{
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  GET_FIRESNAKE_TIMER(ch) = 10; /* Lasts 1 minute (10 rounds) */

  send_to_char(ch, "\tRYou channel your Ki, igniting flames along your strikes!\tn\r\n");
  act("\tR$n's attacks burst into flames as $e channels elemental fire!\tn", FALSE, ch, 0, 0, TO_ROOM);
}

/* rp_surprise_accuracy engine */

/* The surprise-accuracy is reliant on rage */
void perform_surpriseaccuracy(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_SURPRISE_ACCURACY;
  af.duration = 24;

  affect_to_char(ch, &af);

  attach_mud_event(new_mud_event(eSURPRISE_ACCURACY, ch, NULL), SECS_PER_MUD_DAY * 1);

  send_to_char(ch, "You focus your rage and prepare a surprise accurate attack.\r\n");
  act("$n's focuses $s rage, preparing a surprise accuracy attack!", FALSE, ch, 0, 0, TO_ROOM);
}

/* rp_come_and_get_me engine */

/* The come and get me is reliant on rage */
void perform_comeandgetme(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_COME_AND_GET_ME;
  af.duration = 2;

  affect_to_char(ch, &af);

  attach_mud_event(new_mud_event(eCOME_AND_GET_ME, ch, NULL), SECS_PER_MUD_DAY * 1);

  send_to_char(ch, "You focus your rage and prepare to SMASH.\r\n");
  act("$n's focuses $s rage, preparing to SMASH!", FALSE, ch, 0, 0, TO_ROOM);
}

/* rp_powerful_blow engine */

/* The powerful blow is reliant on rage */
void perform_powerfulblow(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_POWERFUL_BLOW;
  af.duration = 24;

  affect_to_char(ch, &af);

  attach_mud_event(new_mud_event(ePOWERFUL_BLOW, ch, NULL), SECS_PER_MUD_DAY * 1);

  send_to_char(ch, "You focus your rage and prepare a powerful blow.\r\n");
  act("$n's focuses $s rage, preparing a powerful blow!", FALSE, ch, 0, 0, TO_ROOM);
}

/* inner fire engine */
void perform_inner_fire(struct char_data *ch)
{
  struct affected_type af[INNER_FIRE_AFFECTS];
  int bonus = 0, duration = 0, i = 0;

  /* bonus of 4 */
  bonus = 4;

  /* a little bit over a minute */
  duration = GET_WIS_BONUS(ch) + CLASS_LEVEL(ch, CLASS_SACRED_FIST);

  send_to_char(ch, "You activate \tWinner \tRfire\tn!\r\n");
  act("$n activates \tWinner \tRfire\tn!", FALSE, ch, 0, 0, TO_ROOM);

  /* init affect array */
  for (i = 0; i < INNER_FIRE_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SKILL_INNER_FIRE;
    af[i].duration = duration;
    af[i].modifier = bonus;
    af[i].bonus_type = BONUS_TYPE_SACRED;
  }

  af[0].location = APPLY_AC_NEW;
  af[1].location = APPLY_SAVING_FORT;
  af[2].location = APPLY_SAVING_WILL;
  af[3].location = APPLY_SAVING_REFL;
  af[4].location = APPLY_SAVING_POISON;
  af[5].location = APPLY_SAVING_DEATH;

  for (i = 0; i < INNER_FIRE_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);
}

/* sacred flames engine */
void perform_sacred_flames(struct char_data *ch)
{
  struct affected_type af;
  int bonus = 0, duration = 0;

  /* The additional damage is equal to the sacred fist's class levels plus his wisdom modifier */
  bonus = GET_WIS_BONUS(ch) + CLASS_LEVEL(ch, CLASS_SACRED_FIST);

  /* a little bit over a minute */
  duration = 12;

  send_to_char(ch, "You activate \tWsacred \tRflames\tn!\r\n");
  act("$n activates \tWsacred \tRflames\tn!", FALSE, ch, 0, 0, TO_ROOM);

  new_affect(&af);

  af.spell = SKILL_SACRED_FLAMES;
  af.duration = duration;
  af.location = APPLY_DAMROLL;
  af.modifier = bonus;
  af.bonus_type = BONUS_TYPE_SACRED;

  affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
}

/* rage (berserk) engine */
void perform_rage(struct char_data *ch)
{
  struct affected_type af[RAGE_AFFECTS];
  int bonus = 0, duration = 0, i = 0;

  if (char_has_mud_event(ch, eRAGE))
  {
    send_to_char(ch, "You must wait longer before you can use this ability "
                     "again.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_FATIGUED))
  {
    send_to_char(ch, "You are are too fatigued to rage!\r\n");
    return;
  }

  if (affected_by_spell(ch, SKILL_RAGE))
  {
    send_to_char(ch, "You are already raging!\r\n");
    return;
  }

  bonus = get_rage_bonus(ch);

  duration = 6 + GET_CON_BONUS(ch) * 2;

  send_to_char(ch, "You go into a \tRR\trA\tRG\trE\tn!.\r\n");
  act("$n goes into a \tRR\trA\tRG\trE\tn!", FALSE, ch, 0, 0, TO_ROOM);

  /* init affect array */
  for (i = 0; i < RAGE_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SKILL_RAGE;
    af[i].duration = duration;
  }

  af[0].location = APPLY_STR;
  af[0].modifier = bonus;
  af[0].bonus_type = BONUS_TYPE_MORALE;

  af[1].location = APPLY_CON;
  af[1].modifier = bonus;
  af[1].bonus_type = BONUS_TYPE_MORALE;

  af[2].location = APPLY_SAVING_WILL;
  af[2].modifier = bonus;
  af[2].bonus_type = BONUS_TYPE_MORALE;

  // this is a penalty
  af[3].location = APPLY_AC_NEW;
  af[3].modifier = -2;

  af[4].location = APPLY_HIT;
  af[4].modifier = bonus * 2;
  af[4].bonus_type = BONUS_TYPE_MORALE;

  for (i = 0; i < RAGE_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  GET_HIT(ch) += bonus * 2;
  if (GET_HIT(ch) > GET_MAX_HIT(ch))
    GET_HIT(ch)
  --;

  /* Add another affect for heavy shrug. */
  //  if (HAS_FEAT(ch, FEAT_RP_HEAVY_SHRUG)) {
  //    struct affected_type heavy_shrug_af;
  //    struct damage_reduction_type *new_dr;
  //
  //    new_affect(&heavy_shrug_af);
  //    heavy_shrug_af.spell = SKILL_RAGE;
  //    heavy_shrug_af.duration = duration;
  //    heavy_shrug_af.location = APPLY_DR;
  //    heavy_shrug_af.modifier = 0;
  //
  //    CREATE(new_dr, struct damage_reduction_type, 1);
  //
  //    new_dr->bypass_cat[0] = DR_BYPASS_CAT_NONE;
  //    new_dr->bypass_val[0] = 0;
  //
  //    new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
  //    new_dr->bypass_val[1] = 0; /* Unused. */
  //
  //    new_dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
  //    new_dr->bypass_val[2] = 0; /* Unused. */
  //
  //    new_dr->amount     = 3;
  //    new_dr->max_damage = -1;
  //    new_dr->spell      = SKILL_RAGE;
  //    new_dr->feat       = FEAT_RP_HEAVY_SHRUG;
  //    new_dr->next       = GET_DR(ch);
  //    GET_DR(ch) = new_dr;
  //
  //    affect_join(ch, &heavy_shrug_af, FALSE, FALSE, FALSE, FALSE);
  //
  //  }

  attach_mud_event(new_mud_event(eRAGE, ch, NULL), (180 * PASSES_PER_SEC));

  USE_STANDARD_ACTION(ch);
}

/* rescue skill mechanic */
void perform_rescue(struct char_data *ch, struct char_data *vict)
{
  struct char_data *tmp_ch;

  if (vict == ch)
  {
    send_to_char(ch, "What about fleeing instead?\r\n");
    return;
  }

  if (FIGHTING(ch) == vict)
  {
    send_to_char(ch, "How can you rescue someone you are trying to kill?\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch &&
                                           (FIGHTING(tmp_ch) != vict);
       tmp_ch = tmp_ch->next_in_room)
    ;

  if ((FIGHTING(vict) != NULL) && (FIGHTING(ch) == FIGHTING(vict)) && (tmp_ch == NULL))
  {
    tmp_ch = FIGHTING(vict);
    if (FIGHTING(tmp_ch) == ch)
    {
      send_to_char(ch, "You have already rescued %s from %s.\r\n", GET_NAME(vict), GET_NAME(FIGHTING(ch)));
      return;
    }
  }

  if (!tmp_ch)
  {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  if (attack_roll(ch, vict, ATTACK_TYPE_PRIMARY, FALSE, 1) <= 0)
  {
    send_to_char(ch, "You fail the rescue!\r\n");
    return;
  }

  act("You place yourself between $N and $S opponent, rescuing $M!", FALSE, ch, 0, vict, TO_CHAR);
  act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(vict) == tmp_ch)
  {
    /* don't forget to remove the fight event! */
    if (char_has_mud_event(vict, eCOMBAT_ROUND))
    {
      event_cancel_specific(vict, eCOMBAT_ROUND);
    }
    stop_fighting(vict);
  }
  if (FIGHTING(tmp_ch))
  {
    /* don't forget to remove the fight event! */
    if (char_has_mud_event(tmp_ch, eCOMBAT_ROUND))
    {
      event_cancel_specific(tmp_ch, eCOMBAT_ROUND);
    }
    stop_fighting(tmp_ch);
  }
  if (FIGHTING(ch))
  {
    /* don't forget to remove the fight event! */
    if (char_has_mud_event(ch, eCOMBAT_ROUND))
    {
      event_cancel_specific(ch, eCOMBAT_ROUND);
    }
    stop_fighting(ch);
  }

  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);

  USE_FULL_ROUND_ACTION(ch);
}

/* charge mechanic */
#define CHARGE_AFFECTS 3

void perform_charge(struct char_data *ch, struct char_data *vict)
{
  struct affected_type af[CHARGE_AFFECTS];
  extern struct index_data *mob_index;
  int (*name)(struct char_data * ch, void *me, int cmd, const char *argument);
  int i = 0;

  if (AFF_FLAGGED(ch, AFF_CHARGING))
  {
    send_to_char(ch, "You are already charging!\r\n");
    return;
  }

  /* init affect array */
  for (i = 0; i < CHARGE_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SKILL_CHARGE;
    af[i].duration = 1;
  }

  SET_BIT_AR(af[0].bitvector, AFF_CHARGING);

  af[1].location = APPLY_HITROLL; /* bonus */
  af[1].modifier = 2;

  af[2].location = APPLY_AC_NEW; /* penalty */
  af[2].modifier = -2;

  for (i = 0; i < CHARGE_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  if (RIDING(ch))
  {
    act("You urge $N forward for a \tYcharge\tn!", FALSE, ch, NULL, RIDING(ch), TO_CHAR);
    act("$n urges you forward for a \tYcharge\tn!", FALSE, ch, NULL, RIDING(ch), TO_VICT);
    act("$n urges $N forward for a \tYcharge\tn!", FALSE, ch, NULL, RIDING(ch), TO_NOTVICT);
    name = mob_index[GET_MOB_RNUM(RIDING(ch))].func;
    if (name)
      (name)(ch, RIDING(ch), 0, "charge");
  }
  else
  {
    act("You run forward for a \tYcharge\tn!", FALSE, ch, NULL, NULL, TO_CHAR);
    act("$n runs forward for a \tYcharge\tn!", FALSE, ch, NULL, NULL, TO_NOTVICT);
  }

  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_RIDE_BY_ATTACK))
  {
    USE_MOVE_ACTION(ch);
  }
  else
  {
    USE_FULL_ROUND_ACTION(ch);
  }

  if (!FIGHTING(ch) && vict && vict != ch)
    hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

  if (vict && vict != ch)
  {
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, vict);
    if (GET_POS(vict) > POS_STUNNED && (FIGHTING(vict) == NULL))
    {
      set_fighting(vict, ch);
    }
  }
}
#undef CHARGE_AFFECTS

/* engine for knockdown, used in bash/trip/etc */
bool perform_knockdown(struct char_data *ch, struct char_data *vict, int skill, bool can_counter, bool display)
{
  int penalty = 0, attack_check = 0, defense_check = 0;
  bool success = FALSE, counter_success = FALSE, skilled_monk = FALSE;

  /* Safety check: prevent dead characters from performing knockdown */
  if (!ch || GET_POS(ch) <= POS_DEAD || DEAD(ch))
  {
    return FALSE;
  }

  /* Safety check: prevent knockdown on dead or invalid targets */
  if (!vict || GET_POS(vict) <= POS_DEAD || DEAD(vict))
  {
    return FALSE;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch && skill != SPELL_BANISHING_BLADE)
  {
    if (display)
      send_to_char(ch, "You simply can't reach that far.\r\n");
    return FALSE;
  }

  if (MOB_FLAGGED(vict, MOB_NOKILL))
  {
    if (display)
      if (skill != SPELL_BANISHING_BLADE)
        send_to_char(ch, "This mob is protected.\r\n");
    return FALSE;
  }

  if (has_perk(vict, PERK_FIGHTER_IMMOVABLE_OBJECT))
  {
    if (display)
      send_to_char(ch, "Your knockdown attempt is unsuccessful against %s!\r\n", show_pers(ch, vict));
    return FALSE;
  }

  if (!is_mission_mob(ch, vict))
  {
    if (display)
      if (skill != SPELL_BANISHING_BLADE)
        send_to_char(ch, "This mob cannot be attacked by you.\r\n");
    return FALSE;
  }

  /* PVP CHECK - prevent knockdown on players without mutual PVP consent */
  if (!IS_NPC(vict) || (IS_NPC(vict) && vict->master && !IS_NPC(vict->master)))
  {
    if (!pvp_ok(ch, vict, display && skill != SPELL_BANISHING_BLADE))
      return FALSE;
  }

  if ((GET_SIZE(ch) - GET_SIZE(vict)) >= 2)
  {
    if (display)
      if (skill != SPELL_BANISHING_BLADE)
        send_to_char(ch, "Your knockdown attempt is unsuccessful due to the target being too small!\r\n");
    return FALSE;
  }

  if ((GET_SIZE(vict) - GET_SIZE(ch)) >= 2)
  {
    if (display)
      if (skill != SPELL_BANISHING_BLADE)
        send_to_char(ch, "Your knockdown attempt is unsuccessful due to the target being too big!\r\n");
    return FALSE;
  }

  if (GET_POS(vict) == POS_SITTING)
  {
    if (display)
      if (skill != SPELL_BANISHING_BLADE)
        send_to_char(ch, "You can't knock down something that is already down!\r\n");
    return FALSE;
  }

  if (IS_INCORPOREAL(vict) && !is_using_ghost_touch_weapon(ch))
  {
    if (skill != SPELL_BANISHING_BLADE)
    {
      act("$n sprawls completely through $N as $e tries to attack $M, slamming into the ground!",
          FALSE, ch, NULL, vict, TO_NOTVICT);
      act("You sprawl completely through $N as you try to attack $M, slamming into the ground!",
          FALSE, ch, NULL, vict, TO_CHAR);
      act("$n sprawls completely through you as $e tries to attack you, slamming into the ground!",
          FALSE, ch, NULL, vict, TO_VICT);
      change_position(ch, POS_SITTING);
    }
    return FALSE;
  }

  if (HAS_SUBRACE(vict, SUBRACE_SWARM))
  {
    act("$n sprawls completely through $N as $e tries to attack $M, slamming into the ground!",
          FALSE, ch, NULL, vict, TO_NOTVICT);
    act("You sprawl completely through $N as you try to attack $M, slamming into the ground!",
        FALSE, ch, NULL, vict, TO_CHAR);
    act("$n sprawls completely through you as $e tries to attack you, slamming into the ground!",
        FALSE, ch, NULL, vict, TO_VICT);
    change_position(ch, POS_SITTING);
    return FALSE;
  }

  if (MOB_FLAGGED(vict, MOB_NOBASH))
  {
    if (display)
      if (skill != SPELL_BANISHING_BLADE)
        send_to_char(ch, "You realize you will probably not succeed:  ");
    penalty = -100;
  }

  if (is_wearing(vict, 132133) && can_counter)
  {
    if (skill != SPELL_BANISHING_BLADE)
    {
      send_to_char(ch, "You failed to knock over %s due to stability boots!  Incoming counter attack!!! ...\r\n", GET_NAME(vict));
      send_to_char(vict, "You stand your ground against %s, and with a snarl attempt a counterattack!\r\n",
                   GET_NAME(ch));
      act("$N via stability boots a knockdown attack from $n is resisted...  $N follows up with a counterattack!", FALSE, ch, 0, vict,
          TO_NOTVICT);

      perform_knockdown(vict, ch, SKILL_BASH, true, true);
    }
    return FALSE;
  }

  switch (skill)
  {
  case SKILL_SHIELD_CHARGE:
  case SKILL_BASH:
    attack_check = GET_STR_BONUS(ch);
    if (is_flying(vict))
    {
      send_to_char(ch, "Impossible, your target is flying!\r\n");
      return FALSE;
    }
    if (!HAS_FEAT(ch, FEAT_IMPROVED_TRIP))
      attack_of_opportunity(vict, ch, 0);
    break;
  case SKILL_BODYSLAM:
    attack_check = GET_WIS_BONUS(ch);
    if (is_flying(vict))
    {
      send_to_char(ch, "Impossible, your target is flying!\r\n");
      return FALSE;
    }
    if (!HAS_FEAT(ch, FEAT_IMPROVED_TRIP))
      attack_of_opportunity(vict, ch, 0);
    break;
  case SKILL_TRIP:
    attack_check = GET_DEX_BONUS(ch);
    // whips give a +5 bonus to the check
    if ((GET_EQ(ch, WEAR_WIELD_1) && GET_WEAPON_TYPE(GET_EQ(ch, WEAR_WIELD_1)) == WEAPON_TYPE_WHIP) ||
        (GET_EQ(ch, WEAR_WIELD_2H) && GET_WEAPON_TYPE(GET_EQ(ch, WEAR_WIELD_2H)) == WEAPON_TYPE_WHIP) ||
        (GET_EQ(ch, WEAR_WIELD_OFFHAND) && GET_WEAPON_TYPE(GET_EQ(ch, WEAR_WIELD_OFFHAND)) == WEAPON_TYPE_WHIP))
    {
      attack_check += 5;
    }
    /* Tactical Fighter perk: Improved Trip */
    if (has_perk(ch, PERK_FIGHTER_IMPROVED_TRIP))
      attack_check += 4;
    /* Monk perk: Sweeping Strike */
    if (has_perk(ch, PERK_MONK_SWEEPING_STRIKE))
      attack_check += 2;
    if (is_flying(vict))
    {
      send_to_char(ch, "Impossible, your target is flying!\r\n");
      return FALSE;
    }
    if (!HAS_FEAT(ch, FEAT_IMPROVED_TRIP) && !has_perk(ch, PERK_FIGHTER_IMPROVED_TRIP))
      attack_of_opportunity(vict, ch, 0);
    break;
  case EVOLUTION_WING_BUFFET_EFFECT:
    // one size smaller gives a -4 penalty
    if ((GET_SIZE(ch) - GET_SIZE(vict)) == 1)
      attack_check -= 4;
    // two sizes smaller gives a -2 penalty
    else if ((GET_SIZE(ch) - GET_SIZE(vict)) == 2)
      attack_check -= 2;
    // if target is not smaller at all, can't be done
    else if ((GET_SIZE(ch) - GET_SIZE(vict)) <= 0)
      return FALSE;
    // three or more sizes smaller is no penalty
    break;    
  case SPELL_BANISHING_BLADE:
    break;
  default:
    log("Invalid skill sent to perform knockdown!\r\n");
    return FALSE;
  }

  /* Perform the unarmed touch attack */
  if ((attack_roll(ch, vict, ATTACK_TYPE_UNARMED, TRUE, 1) + penalty) > 0)
  {
    /* Successful unarmed touch attacks. */

    if (skill != SPELL_BANISHING_BLADE)
    {
      attack_check += d20(ch);
    }
    else
    {
      /* Perform strength check. */
      // if the teamwork feat tandem trip is in effect, we'll take the highest of two d20 rolls.
      // Otherwise, just one d20.
      attack_check += MAX(d20(ch), has_teamwork_feat(ch, FEAT_TANDEM_TRIP) ? d20(ch) : 0);
      attack_check += d20(ch) + size_modifiers[GET_SIZE(ch)]; /* we added stat bonus above */
    }
    defense_check = d20(vict) + MAX(GET_STR_BONUS(vict), GET_DEX_BONUS(vict)) + size_modifiers[GET_SIZE(vict)];

    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_IMPROVED_TRIP) && skill != SPELL_BANISHING_BLADE)
    {
      /* You do not provoke an attack of opportunity when you attempt to trip an opponent while you are unarmed.
       * You also gain a +4 bonus on your Strength check to trip your opponent.
       *
       * If you trip an opponent in melee combat, you immediately get a melee attack against that opponent as if
       * you hadn't used your attack for the trip attempt.
       *
       * Normal:
       * Without this feat, you provoke an attack of opportunity when you attempt to trip an opponent while you
       * are unarmed. */
      attack_check += 4;
    }

    if (MONK_TYPE(ch) >= 10 && skill != SPELL_BANISHING_BLADE)
    {
      attack_check += 8;
      skilled_monk = TRUE;
    }

    if (GET_RACE(vict) == RACE_DWARF ||
        GET_RACE(vict) == RACE_CRYSTAL_DWARF ||
        GET_RACE(vict) == RACE_DUERGAR) /* dwarven stability */
      defense_check += 4;

    /*DEBUG*/ /*send_to_char(ch, "attack check: %d, defense_check: %d\r\n", attack_check, defense_check);*/
    if (attack_check >= defense_check)
    {
      if (attack_check == defense_check)
      {
        /* Check the bonuses. */
        if ((GET_STR_BONUS(ch) + (GET_SIZE(ch) - GET_SIZE(vict)) * 4) >= (MAX(GET_STR_BONUS(vict), GET_DEX_BONUS(vict))))
          success = TRUE;
        else
          success = FALSE;
      }
      else
      {
        success = TRUE;
      }
    }

    if (success == TRUE)
    {
      switch (skill)
      {
      case SKILL_SHIELD_CHARGE:
        /* Messages for shield charge */
        act("\tyYou knock $N to the ground!\tn", FALSE, ch, NULL, vict, TO_CHAR);
        act("\ty$n knocks you to the ground!\tn", FALSE, ch, NULL, vict, TO_VICT);
        act("\ty$n knocks $N to the ground!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
        break;
      case SKILL_BODYSLAM:
        /* Messages for body slam */
        act("\tyYou grab and SLAM $N, throwing $M to the ground!\tn", FALSE, ch, NULL, vict, TO_CHAR);
        act("\ty$n grabs and SLAMS you, throwing you to the ground!\tn", FALSE, ch, NULL, vict, TO_VICT);
        act("\ty$n grabs and SLAMS $N, throwing $M to the ground!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
        break;
      case SPELL_BANISHING_BLADE:
        /* Messages for body slam */
        act("\tyYour banishing blade sweeps under $N, throwing $M to the ground!\tn", FALSE, ch, NULL, vict, TO_CHAR);
        act("\ty$n's banishing blade sweeps under you, throwing you to the ground!\tn", FALSE, ch, NULL, vict, TO_VICT);
        act("\ty$n's banishing blade sweeps under $N, throwing $M to the ground!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
        break;
      default:
        /* (Default) Messages for successful trip */
        act("\tyYou grab and overpower $N, throwing $M to the ground!\tn", FALSE, ch, NULL, vict, TO_CHAR);
        act("\ty$n grabs and overpowers you, throwing you to the ground!\tn", FALSE, ch, NULL, vict, TO_VICT);
        act("\ty$n grabs and overpowers $N, throwing $M to the ground!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
        break;
      }
    }
    else
    { /* failed!! */
      /* Messages for shield charge */
      if (skill == SKILL_SHIELD_CHARGE)
      {
        /* just moved this to damage-messages */
      }
      else if (skill == SPELL_BANISHING_BLADE || skill == EVOLUTION_WING_BUFFET_EFFECT)
      {
        // no fail message
      }
      else
      {
        /* Messages for failed trip */
        if (GET_STR_BONUS(vict) >= GET_DEX_BONUS(vict))
        {
          act("\tyYou grab $N but $E tears out of your grasp!\tn", FALSE, ch, NULL, vict, TO_CHAR);
          act("\ty$n grabs you but you tear yourself from $s grasp!\tn", FALSE, ch, NULL, vict, TO_VICT);
          act("\ty$n grabs $N but $E tears out of $s grasp!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
        }
        else
        {
          act("\tyYou grab $N but $E deftly turns away from your attack!\tn", FALSE, ch, NULL, vict, TO_CHAR);
          act("\ty$n grabs you and you deftly turn away from $s attack!\tn", FALSE, ch, NULL, vict, TO_VICT);
          act("\ty$n grabs $N but $E deftly turns away from $s attack!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
        }
      }

      /* Victim gets a chance to countertrip */
      if (skill != SKILL_SHIELD_CHARGE && skill != SPELL_BANISHING_BLADE && GET_POS(vict) > POS_SITTING && can_counter)
      {
        attack_check = (d20(vict) + GET_STR_BONUS(vict) + (GET_SIZE(vict) - GET_SIZE(ch)) * 4);
        defense_check = (d20(ch) + MAX(GET_STR_BONUS(ch), GET_DEX_BONUS(ch)));

        if (GET_RACE(ch) == RACE_DWARF ||
            GET_RACE(ch) == RACE_DUERGAR ||
            GET_RACE(ch) == RACE_CRYSTAL_DWARF) /* Dwarves get a stability bonus. */
          defense_check += 4;
        /*DEBUG*/ /*send_to_char(ch, "counterattack check: %d, defense_check: %d\r\n", attack_check, defense_check);*/

        if (attack_check >= defense_check)
        {
          if (attack_check == defense_check)
          {
            /* Check the bonuses. */
            if ((GET_STR_BONUS(vict) + (GET_SIZE(vict) - GET_SIZE(ch)) * 4) >= (MAX(GET_STR_BONUS(ch), GET_DEX_BONUS(ch))))
              counter_success = TRUE;
            else
              counter_success = FALSE;
          }
          else
          {
            counter_success = TRUE;
          }
        }

        /* No tripping someone already on the ground. */
        if (GET_POS(ch) == POS_SITTING)
        {
          act("\tyYou can't counter-trip someone who is already down!\tn", FALSE, ch, NULL, vict, TO_VICT);
        }
        else if (counter_success == TRUE)
        {
          /* Messages for successful counter-trip */
          act("\tyTaking advantage of your failed attack, $N throws you to the ground!\tn", FALSE, ch, NULL, vict, TO_CHAR);
          act("\tyTaking advantage of $n's failed attack, you throw $m to the ground!\tn", FALSE, ch, NULL, vict, TO_VICT);
          act("\tyTaking advantage of $n's failed attack, $N throws $m to the ground!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
        }
        else
        {
          /* Messages for failed counter-trip */
          if (GET_STR_BONUS(ch) >= GET_DEX_BONUS(ch))
          {
            act("\tyYou resist $N's attempt to take advantage of your failed attack.\tn", FALSE, ch, NULL, vict, TO_CHAR);
            act("\ty$n resists your attempt to take advantage of $s failed attack.\tn", FALSE, ch, NULL, vict, TO_VICT);
            act("\ty$n resists $N's attempt to take advantage of $s failed attack.\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
          }
          else
          {
            act("\tyYou twist away from $N's attempt to take advantage of your failed attack.\tn", FALSE, ch, NULL, vict, TO_CHAR);
            act("\ty$n twists away from your attempt to take advantage of $s failed attack.\tn", FALSE, ch, NULL, vict, TO_VICT);
            act("\ty$n twists away from $N's attempt to take advantage of $s failed attack.\t\n", FALSE, ch, NULL, vict, TO_NOTVICT);
          }
        }
      }
    }

    /* FAILED attack roll */
  }
  else
  {
    /* Messages for a missed unarmed touch attack. */
    if (skill == SKILL_SHIELD_CHARGE)
    {
      /* just moved this to damage-messages */
    }
    if (skill == SPELL_BANISHING_BLADE)
    {
      // No miss message
    }
    else
    {
      act("\tyYou are unable to grab $N!\tn", FALSE, ch, NULL, vict, TO_CHAR);
      act("\ty$n tries to grab you, but you dodge easily away!\tn", FALSE, ch, NULL, vict, TO_VICT);
      act("\ty$n tries to grab $N, but $N dodges easily away!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
    }
  }

  /* further processing: set position, special feats, etc */
  if (!success)
  {
    if (counter_success)
    {
      change_position(ch, POS_SITTING);
    }
  }
  else
  { /* success! */
    change_position(vict, POS_SITTING);
    if ((skill == SKILL_TRIP) ||
        (skill == SKILL_BASH) ||
        (skill == SKILL_BODYSLAM) ||
        (skill == SKILL_SHIELD_CHARGE))
    {

      /* bodyslam with a skilled monk */
      if (skill == SKILL_BODYSLAM && skilled_monk)
      {
        damage(ch, vict, dice(MONK_TYPE(ch), 8) + GET_WIS_BONUS(ch), SKILL_BODYSLAM, DAM_FORCE, FALSE);
      }

      /* Successful trip, cheat for feats */
      if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_IMPROVED_TRIP))
      {
        /* You get a free swing on the tripped opponent. */
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    }
  }

  /* fire-shield, etc check */
  if (success && skill != SKILL_SHIELD_CHARGE)
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);

  /* make sure combat starts */
  if (vict != ch)
  {
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, vict);
    if (GET_POS(vict) > POS_STUNNED && (FIGHTING(vict) == NULL))
    {
      set_fighting(vict, ch);
    }
  }

  return success;
}

/* shieldpunch engine :
 * Perform the shield punch, check for proficiency and the required
 * equipment, also check for any enhancing feats. */
bool perform_shieldpunch(struct char_data *ch, struct char_data *vict)
{
  extern struct index_data *obj_index;
  int (*name)(struct char_data * ch, void *me, int cmd, const char *argument);
  struct obj_data *shield = GET_EQ(ch, WEAR_SHIELD);

  if (!shield)
  {
    send_to_char(ch, "You need a shield to do that.\r\n");
    return FALSE;
  }

  if (!vict)
  {
    send_to_char(ch, "Shieldpunch who?\r\n");
    return FALSE;
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return FALSE;
  }

  if (!CAN_SEE(ch, vict))
  {
    send_to_char(ch, "You don't see well enough to attempt that.\r\n");
    return FALSE;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE))
  {
    if (ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You simply can't reach that far.\r\n");
      return FALSE;
    }
  }

  if (!HAS_FEAT(ch, FEAT_IMPROVED_SHIELD_PUNCH))
  {
    /* Remove shield bonus from ac. */
    attach_mud_event(new_mud_event(eSHIELD_RECOVERY, ch, NULL), PULSE_VIOLENCE);
  }

  /*  Use an attack mechanic to determine success. */
  if (attack_roll(ch, vict, ATTACK_TYPE_OFFHAND, FALSE, 1) <= 0)
  {
    damage(ch, vict, 0, SKILL_SHIELD_PUNCH, DAM_FORCE, FALSE);
  }
  else
  {
    damage(ch, vict, dice(1, 6) + (GET_STR_BONUS(ch) / 2), SKILL_SHIELD_PUNCH, DAM_FORCE, FALSE);
    name = obj_index[GET_OBJ_RNUM(shield)].func;
    if (name)
      (name)(ch, shield, 0, "shieldpunch");

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);
  }

  return TRUE;
}

/* shieldcharge engine :
 * Perform the shieldcharge, check for proficiency and the required
 * equipment, also check for any enhancing feats. Perform a trip attack
 * on success
 *
 * Note - Charging gives +2 to your attack
 */
bool perform_shieldcharge(struct char_data *ch, struct char_data *vict)
{
  extern struct index_data *obj_index;
  int (*name)(struct char_data * ch, void *me, int cmd, const char *argument);
  struct obj_data *shield = GET_EQ(ch, WEAR_SHIELD);

  if (!shield)
  {
    send_to_char(ch, "You need a shield to do that.\r\n");
    return FALSE;
  }

  if (!vict)
  {
    send_to_char(ch, "Shieldcharge who?\r\n");
    return FALSE;
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return FALSE;
  }

  if (!CAN_SEE(ch, vict))
  {
    send_to_char(ch, "You don't see well enough to attempt that.\r\n");
    return FALSE;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE))
  {
    if (ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You simply can't reach that far.\r\n");
      return FALSE;
    }
  }

  /*  Use an attack mechanic to determine success.
   *  Add 2 for the charge bonus!  */
  if (attack_roll(ch, vict, ATTACK_TYPE_OFFHAND, FALSE, 1) + 2 <= 0)
  {
    damage(ch, vict, 0, SKILL_SHIELD_CHARGE, DAM_FORCE, FALSE);
  }
  else
  {
    damage(ch, vict, dice(1, 6) + GET_STR_BONUS(ch), SKILL_SHIELD_CHARGE, DAM_FORCE, FALSE);
    name = obj_index[GET_OBJ_RNUM(shield)].func;
    if (name)
      (name)(ch, shield, 0, "shieldcharge");

    perform_knockdown(ch, vict, SKILL_SHIELD_CHARGE, true, true);

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);
  }

  USE_STANDARD_ACTION(ch);
  USE_MOVE_ACTION(ch);

  return TRUE;
}

/* shieldslam engine :
 * Perform the shield slam, check for proficiency and the required
 * equipment, also check for any enhancing feats. */
bool perform_shieldslam(struct char_data *ch, struct char_data *vict)
{
  struct affected_type af;
  extern struct index_data *obj_index;
  int (*name)(struct char_data * ch, void *me, int cmd, const char *argument);
  struct obj_data *shield = GET_EQ(ch, WEAR_SHIELD);

  if (!shield)
  {
    send_to_char(ch, "You need a shield to do that.\r\n");
    return FALSE;
  }

  if (!vict)
  {
    send_to_char(ch, "Shieldslam who?\r\n");
    return FALSE;
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return FALSE;
  }

  if (!CAN_SEE(ch, vict))
  {
    send_to_char(ch, "You don't see well enough to attempt that.\r\n");
    return FALSE;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE))
  {
    if (ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You simply can't reach that far.\r\n");
      return FALSE;
    }
  }

  /*  Use an attack mechanic to determine success. */
  if (attack_roll(ch, vict, ATTACK_TYPE_OFFHAND, FALSE, 1) <= 0)
  {
    damage(ch, vict, 0, SKILL_SHIELD_SLAM, DAM_FORCE, FALSE);
  }
  else
  {
    damage(ch, vict, dice(1, 6) + (GET_STR_BONUS(ch) / 2), SKILL_SHIELD_SLAM, DAM_FORCE, FALSE);
    name = obj_index[GET_OBJ_RNUM(shield)].func;
    if (name)
      (name)(ch, shield, 0, "shieldslam");

    if (!savingthrow(ch, vict, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL) && can_daze(vict))
    {
      new_affect(&af);
      af.spell = SKILL_SHIELD_SLAM;
      SET_BIT_AR(af.bitvector, AFF_DAZED);
      GET_NODAZE_COOLDOWN(vict) = NODAZE_COOLDOWN_TIMER;
      af.duration = 1; /* One round */
      affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
      act("$N appears to be dazed by $n's blow!",
          FALSE, ch, NULL, vict, TO_NOTVICT);
      act("$N appears to be dazed by your blow!",
          FALSE, ch, NULL, vict, TO_CHAR);
      act("You are dazed by $n's blow!",
          FALSE, ch, NULL, vict, TO_VICT | TO_SLEEP);
    }

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);
  }

  USE_STANDARD_ACTION(ch);
  USE_MOVE_ACTION(ch);

  return TRUE;
}

/* engine for headbutt skill */
void perform_headbutt(struct char_data *ch, struct char_data *vict)
{
  struct affected_type af;

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE))
  {
    if (ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You simply can't reach that far.\r\n");
      return;
    }
  }

  if (GET_POS(ch) <= POS_SITTING)
  {
    send_to_char(ch, "You need to get on your feet to do a headbutt.\r\n");
    return;
  }

  if (GET_SIZE(ch) != GET_SIZE(vict))
  {
    send_to_char(ch, "Its too difficult to headbutt that target due to size!\r\n");
    return;
  }

  if (IS_INCORPOREAL(vict) && !is_using_ghost_touch_weapon(ch))
  {
    act("$n sprawls completely through $N as $e tries to attack $M!",
        FALSE, ch, NULL, vict, TO_NOTVICT);
    act("You sprawl completely through $N as you try to attack $N!",
        FALSE, ch, NULL, vict, TO_CHAR);
    act("$n sprawls completely through you as $e tries to attack!",
        FALSE, ch, NULL, vict, TO_VICT);
    change_position(ch, POS_SITTING);
    return;
  }

  if (attack_roll(ch, vict, ATTACK_TYPE_UNARMED, FALSE, 1) > 0)
  {
    damage(ch, vict, dice((HAS_FEAT(ch, FEAT_IMPROVED_UNARMED_STRIKE) ? 2 : 1), 8), SKILL_HEADBUTT, DAM_FORCE, FALSE);

    if (!rand_number(0, 4))
    {
      if (!paralysis_immunity(vict))
      {
        new_affect(&af);
        af.spell = SKILL_HEADBUTT;
        SET_BIT_AR(af.bitvector, AFF_PARALYZED);
        af.duration = 1;
        affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
        act("$n slams $s head into $N with \tRVICIOUS\tn force!",
            FALSE, ch, NULL, vict, TO_NOTVICT);
        act("You slam your head into $N with \tRVICIOUS\tn force!",
            FALSE, ch, NULL, vict, TO_CHAR);
        act("$n slams $s head into you with \tRVICIOUS\tn force!",
            FALSE, ch, NULL, vict, TO_VICT | TO_SLEEP);
      }
    }

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);
  }
  else
  {
    damage(ch, vict, 0, SKILL_HEADBUTT, DAM_FORCE, FALSE);
  }
}

void apply_paladin_mercies(struct char_data *ch, struct char_data *vict)
{
  if (!ch || !vict)
    return;

  struct affected_type *af = NULL, *af_next = NULL;
  struct affected_type af2;
  bool found = false;

  if (KNOWS_MERCY(ch, PALADIN_MERCY_DECEIVED))
  {
    if (char_has_mud_event(vict, eINTIMIDATED))
    {
      event_cancel_specific(ch, eINTIMIDATED);
      send_to_char(ch, "You are no longer intimidated.\r\n");
    }
    if (char_has_mud_event(vict, eTAUNTED))
    {
      event_cancel_specific(ch, eTAUNTED);
      send_to_char(ch, "You are no longer feeliong taunted.\r\n");
    }
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_FATIGUED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_FATIGUED, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_SHAKEN))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_SHAKEN, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_DAZED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_DAZED, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_ENFEEBLED))
  {
    for (af = vict->affected; af; af = af_next)
    {
      af_next = af->next;
      if ((af->location == APPLY_STR || af->location == APPLY_CON) && af->modifier < 0)
      {
        send_to_char(vict, "Affect '%s' has been healed!\r\n", spell_info[af->spell].name);
        send_to_char(ch, "%s's Affect '%s' has been healed!\r\n", GET_NAME(vict), spell_info[af->spell].name);
        affect_from_char(vict, af->spell);
      }
    }
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_STAGGERED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_STAGGERED, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_CONFUSED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_CONFUSED, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_CURSED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_CURSE, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_FRIGHTENED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_FEAR, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_INJURED))
  {
    new_affect(&af2);
    af2.spell = PALADIN_MERCY_INJURED_FAST_HEALING;
    af2.bonus_type = BONUS_TYPE_MORALE;
    af2.location = APPLY_SPECIAL;
    af2.modifier = 3;
    af2.duration = CLASS_LEVEL(ch, CLASS_PALADIN) / 2;
    if (ch != vict)
    {
      send_to_char(ch, "%s gains fast healing of 3 hp/round for %d rounds.\r\n", GET_NAME(vict), af2.duration);
      send_to_char(vict, "You gain fast healing of 3 hp/round for %d rounds.\r\n", af2.duration);
    }
    else
    {
      send_to_char(ch, "You gain fast healing of 3 hp/round for %d rounds.\r\n", af2.duration);
    }
    affect_to_char(ch, &af2);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_NAUSEATED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_NAUSEATED, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_POISONED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_POISON, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_BLINDED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_BLIND, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_DEAFENED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_DEAF, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_PARALYZED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_PARALYZED, true);
  }

  if (KNOWS_MERCY(ch, PALADIN_MERCY_STUNNED))
  {
    remove_any_spell_with_aff_flag(ch, vict, AFF_STUN, true);
  }

  // this goes last so that all of the above mercies take precedent and
  // don't have their usefuless reduced by the ensorcelled mercy
  if (KNOWS_MERCY(ch, PALADIN_MERCY_ENSORCELLED))
  {
    int spell_num = 0;
    for (af = vict->affected; af; af = af_next)
    {
      spell_num = af->spell;
      if (spell_num < 0 || spell_num >= TOP_SKILL_DEFINE) continue;
      af_next = af->next;
      if (spell_info[af->spell].violent && 
          dice(1, 2) == 1 && 
          !found)
      {
        found = true;
        send_to_char(vict, "Affect '%s' has been healed!\r\n", spell_info[af->spell].name);
        send_to_char(ch, "%s's Affect '%s' has been healed!\r\n", GET_NAME(vict), spell_info[af->spell].name);
        affect_from_char(vict, af->spell);
      }
    }
  }
}

/* engine for layonhands skill */
void perform_layonhands(struct char_data *ch, struct char_data *vict)
{
  int heal_amount = 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  if (HAS_FEAT(ch, FEAT_HOLY_CHAMPION)) 
  {
    // maximize the lay on hands as per feat description
    heal_amount = 20 + GET_CHA_BONUS(ch) + CLASS_LEVEL(ch, CLASS_PALADIN) * 7;
  }
  else {
    heal_amount = 20 + GET_CHA_BONUS(ch) + dice(CLASS_LEVEL(ch, CLASS_PALADIN), 6);
    if (HAS_FEAT(ch, FEAT_HOLY_WARRIOR)) 
    {
      // add plus one healing per die as per feat description
      heal_amount += CLASS_LEVEL(ch, CLASS_PALADIN);
    }
  }
  
  /* Paladin Sacred Defender perk: Healing Hands - +10% healing per rank */
  if (!IS_NPC(ch))
  {
    int healing_hands_bonus = get_paladin_healing_hands_bonus(ch);
    if (healing_hands_bonus > 0)
    {
      heal_amount = heal_amount * (100 + healing_hands_bonus) / 100;
    }
  }
  
  heal_amount = MIN(GET_MAX_HIT(vict) - GET_HIT(vict), heal_amount);

  send_to_char(ch, "Your hands flash \tWbright white\tn as you reach out...\r\n");
  if (ch == vict)
  {
    send_to_char(ch, "You heal yourself! [%d]\r\n", heal_amount);
    act("$n \tWheals\tn $mself!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  else
  {
    send_to_char(ch, "You heal %s! [%d]\r\n", GET_NAME(vict), heal_amount);
    act("You are \tWhealed\tn by $N!", FALSE, vict, 0, ch, TO_CHAR);
    act("$n \tWheals\tn $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_LAYHANDS);

  GET_HIT(vict) += heal_amount;
  apply_paladin_mercies(ch, vict);
  
  /* Paladin Sacred Defender perk: Cleansing Touch - remove one negative affect */
  if (!IS_NPC(ch) && has_paladin_cleansing_touch(ch))
  {
    struct affected_type *af = NULL, *af_next = NULL;
    bool removed = FALSE;
    
    /* Try to find and remove a negative affect */
    for (af = vict->affected; af && !removed; af = af_next)
    {
      af_next = af->next;
      
      /* Check for harmful affects (negative modifiers or harmful flags) */
      if (af->modifier < 0 || 
          IS_SET_AR(af->bitvector, AFF_BLIND) ||
          IS_SET_AR(af->bitvector, AFF_CURSE) ||
          IS_SET_AR(af->bitvector, AFF_POISON) ||
          IS_SET_AR(af->bitvector, AFF_DISEASE) ||
          IS_SET_AR(af->bitvector, AFF_PARALYZED) ||
          IS_SET_AR(af->bitvector, AFF_STUN) ||
          IS_SET_AR(af->bitvector, AFF_FEAR) ||
          IS_SET_AR(af->bitvector, AFF_CONFUSED) ||
          IS_SET_AR(af->bitvector, AFF_NAUSEATED) ||
          IS_SET_AR(af->bitvector, AFF_FATIGUED) ||
          IS_SET_AR(af->bitvector, AFF_DAZED))
      {
        send_to_char(vict, "\tWYour affliction '%s' has been cleansed!\tn\r\n", spell_info[af->spell].name);
        if (ch != vict)
          send_to_char(ch, "\tWYou cleanse %s's affliction '%s'!\tn\r\n", GET_NAME(vict), spell_info[af->spell].name);
        affect_from_char(vict, af->spell);
        removed = TRUE;
        break;
      }
    }
  }
  
  /* Paladin Sacred Defender perk: Merciful Touch - +20 current and max HP for 5 rounds */
  if (!IS_NPC(ch) && has_paladin_merciful_touch(ch) && !affected_by_spell(vict, SKILL_MERCIFUL_TOUCH))
  {
    struct affected_type af;
    new_affect(&af);
    af.spell = SKILL_MERCIFUL_TOUCH;
    af.duration = 5; /* 5 rounds */
    SET_BIT_AR(af.bitvector, AFF_REGEN); /* Regenerating health */
    af.location = APPLY_HIT;
    af.modifier = 20; /* +20 current HP */
    affect_to_char(vict, &af);
    
    /* Also increase max HP temporarily */
    af.location = APPLY_CON;
    af.modifier = 2; /* +2 CON = roughly +20 HP */
    affect_to_char(vict, &af);
    
    if (ch != vict)
      send_to_char(vict, "\tYYou feel divinely empowered!\tn\r\n");
    else
      send_to_char(ch, "\tYYou feel divinely empowered!\tn\r\n");
  }
  
  update_pos(vict);

  /* Paladin Sacred Defender perk: Cleansing Touch - can be used as swift action */
  if (!IS_NPC(ch) && has_paladin_cleansing_touch(ch))
  {
    if (ch != vict)
    {
      USE_SWIFT_ACTION(ch);
    }
    /* free action to use it on yourself */
  }
  else
  {
    if (ch != vict)
    {
      USE_STANDARD_ACTION(ch);
    }
    /* free action to use it on yourself */
  }
}

/* engine for sap skill */
#define TRELUX_CLAWS 800

void perform_sap(struct char_data *ch, struct char_data *vict)
{
  int dam = 0, found = FALSE;
  int prob = -6;
  struct affected_type af;
  struct obj_data *wielded = NULL;

  if (vict == ch)
  {
    send_to_char(ch, "How can you sneak up on yourself?\r\n");
    return;
  }

  if (ch->in_room != vict->in_room)
  {
    send_to_char(ch, "That would be a bit hard.\r\n");
    return;
  }

  PREREQ_NOT_PEACEFUL_ROOM();

  if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE))
  {
    if (ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You simply can't reach that far.\r\n");
      return;
    }
  }

  if (!AFF_FLAGGED(ch, AFF_HIDE) || !AFF_FLAGGED(ch, AFF_SNEAK))
  {
    send_to_char(ch, "You have to be sneaking and hiding (in that order) before you can sap "
                     "anyone.\r\n");
    return;
  }

  if (IS_INCORPOREAL(vict) && !is_using_ghost_touch_weapon(ch))
  {
    send_to_char(ch, "There is simply nothing solid there to sap.\r\n");
    return;
  }

  if (GET_SIZE(ch) < GET_SIZE(vict))
  {
    send_to_char(ch, "You can't reach that high.\r\n");
    return;
  }

  if (GET_SIZE(ch) - GET_SIZE(vict) >= 2)
  {
    send_to_char(ch, "The target is too small.\r\n");
    return;
  }

  /* 2h bludgeon */
  wielded = GET_EQ(ch, WEAR_WIELD_2H);
  if (wielded &&
      IS_SET(weapon_list[GET_WEAPON_TYPE(wielded)].damageTypes, DAMAGE_TYPE_BLUDGEONING))
  {
    prob += 4;
    found = TRUE;
  }

  /* 1h bludgeon primary */
  if (!found)
  {
    wielded = GET_EQ(ch, WEAR_WIELD_1);
    if (wielded &&
        IS_SET(weapon_list[GET_WEAPON_TYPE(wielded)].damageTypes, DAMAGE_TYPE_BLUDGEONING))
    {
      found = TRUE;
    }
  }

  /* 1h bludgeon off-hand */
  if (!found)
  {
    wielded = GET_EQ(ch, WEAR_WIELD_OFFHAND);
    if (wielded &&
        IS_SET(weapon_list[GET_WEAPON_TYPE(wielded)].damageTypes, DAMAGE_TYPE_BLUDGEONING))
    {
      found = TRUE;
    }
  }

  /* almost forgot about trelux!  yes their claws can be used for sapping */
  if (!found && GET_RACE(ch) == RACE_TRELUX)
  {
    prob += 8; /* negate the penalty, plus 2 bonus */
    wielded = read_object(TRELUX_CLAWS, VIRTUAL);
    found = TRUE;
  }

  if (!found || !wielded)
  {
    send_to_char(ch, "You need a bludgeon weapon to make this a success...\r\n");
    return;
  }

  if (!CAN_SEE(vict, ch))
    prob += 4;

  if (HAS_EVOLUTION(vict, EVOLUTION_UNDEAD_APPEARANCE))
    prob -= get_evolution_appearance_save_bonus(vict);

  if (attack_roll(ch, vict, ATTACK_TYPE_PRIMARY, FALSE, prob) > 0)
  {
    dam = 5 + dice(GET_LEVEL(ch), 2);
    damage(ch, vict, dam, SKILL_SAP, DAM_FORCE, FALSE);
    change_position(vict, POS_RECLINING);

    /* success!  fortitude save? */
    if (!savingthrow(ch, vict, SAVING_FORT, prob, CAST_INNATE, CLASS_LEVEL(ch, CLASS_ROGUE), NOSCHOOL))
    {
      if (!paralysis_immunity(vict))
      {
        new_affect(&af);
        af.spell = SKILL_SAP;
        SET_BIT_AR(af.bitvector, AFF_PARALYZED);
        af.duration = 1;
        affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);
        act("$n \tYsavagely\tn beats $N with $p!!",
            FALSE, ch, wielded, vict, TO_NOTVICT);
        act("You \tYsavagely\tn beat $N with $p!",
            FALSE, ch, wielded, vict, TO_CHAR);
        act("$n \tYsavagely\tn beats you with $p!!",
            FALSE, ch, wielded, vict, TO_VICT | TO_SLEEP);
      }
    }

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);
  }
  else
  {
    damage(ch, vict, 0, SKILL_SAP, DAM_FORCE, FALSE);
  }

  USE_STANDARD_ACTION(ch);
  USE_MOVE_ACTION(ch);
}
#undef TRELUX_CLAWS

/* main engine for dirt-kick mechanic */
bool perform_dirtkick(struct char_data *ch, struct char_data *vict)
{
  struct affected_type af;
  int dam = 0;

  if (!CAN_SEE(ch, vict))
  {
    send_to_char(ch, "You don't see well enough to attempt that.\r\n");
    return FALSE;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE))
  {
    if (ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You simply can't reach that far.\r\n");
      return FALSE;
    }
  }

  if (AFF_FLAGGED(ch, AFF_IMMATERIAL))
  {
    send_to_char(ch, "Its pretty hard to kick with immaterial legs.\r\n");
    return FALSE;
  }

  if (MOB_FLAGGED(vict, MOB_NOBLIND))
  {
    send_to_char(ch, "Your technique is ineffective...  ");
    damage(ch, vict, 0, SKILL_DIRT_KICK, 0, FALSE);
    return FALSE;
  }

  if (GET_SIZE(vict) - GET_SIZE(ch) > 1)
  {
    send_to_char(ch, "Your target is too large for this technique to be effective!\r\n");
    return FALSE;
  }

  if (GET_SIZE(ch) - GET_SIZE(vict) >= 2)
  {
    send_to_char(ch, "Your target is too small for this technique to be effective!\r\n");
    return FALSE;
  }

  if (attack_roll(ch, vict, ATTACK_TYPE_UNARMED, FALSE, 1) > 0)
  {
    dam = dice(1, GET_LEVEL(ch));
    damage(ch, vict, dam, SKILL_DIRT_KICK, 0, FALSE);

    if (!AFF_FLAGGED(vict, AFF_BLIND) && !savingthrow(ch, vict, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL) && can_blind(vict))
    {
      new_affect(&af);

      af.spell = SKILL_DIRT_KICK;
      SET_BIT_AR(af.bitvector, AFF_BLIND);
      af.modifier = -4;
      af.duration = dice(1, GET_LEVEL(ch) / 5);
      af.location = APPLY_HITROLL;

      affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);

      act("$n blinds $N with the debris!!", FALSE, ch, NULL, vict, TO_NOTVICT);
      act("You blind $N with debris!", FALSE, ch, NULL, vict, TO_CHAR);
      act("$n blinds you with debris!!", FALSE, ch, NULL, vict, TO_VICT | TO_SLEEP);
    }
  }
  else
    damage(ch, vict, 0, SKILL_DIRT_KICK, 0, FALSE);

  USE_STANDARD_ACTION(ch);

  return TRUE;
}

/* main engine for assist mechanic */
void perform_assist(struct char_data *ch, struct char_data *helpee)
{
  struct char_data *opponent = NULL;

  if (!ch)
    return;

  /* hit same opponent as person you are helping */
  if (FIGHTING(helpee))
    opponent = FIGHTING(helpee);
  else
    for (opponent = world[IN_ROOM(ch)].people;
         opponent && (FIGHTING(opponent) != helpee);
         opponent = opponent->next_in_room)
      ;

  if (!opponent)
    act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
  else if (!CAN_SEE(ch, opponent))
    act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
  /* prevent accidental pkill */
  else if (!IS_NPC(opponent) || (IS_NPC(opponent) && opponent->master && !IS_NPC(opponent->master)))
  {
    if (!pvp_ok(ch, opponent, true))
      return;
  }
  else if (!MOB_CAN_FIGHT(ch))
  {
    send_to_char(ch, "You can't fight!\r\n");
  }
  else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
           ch->next_in_room != opponent && opponent->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
  }
  else
  {
    send_to_char(ch, "You join the fight!\r\n");
    act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
    act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
    set_fighting(ch, opponent);

    // hit(ch, opponent, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }
}

/* the primary engine for springleap */
void perform_springleap(struct char_data *ch, struct char_data *vict)
{
  int dam = 0;

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (!CAN_SEE(ch, vict))
  {
    send_to_char(ch, "You don't see well enough to attempt that.\r\n");
    return;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE))
  {
    if (ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You simply can't reach that far.\r\n");
      return;
    }
  }

  if (IS_INCORPOREAL(vict) && !is_using_ghost_touch_weapon(ch))
  {
    act("$n sprawls completely through $N as $e tries to springleap $M!",
        FALSE, ch, NULL, vict, TO_NOTVICT);
    act("You sprawl completely through $N as you try a springleap attack!",
        FALSE, ch, NULL, vict, TO_CHAR);
    act("$n sprawls completely through you as $e attempts a springleap attack!",
        FALSE, ch, NULL, vict, TO_VICT);
    return;
  }

  if (attack_roll(ch, vict, ATTACK_TYPE_UNARMED, FALSE, 1) > 0)
  {
    dam = dice(6, (GET_LEVEL(ch) / 5) + 2);
    damage(ch, vict, dam, SKILL_SPRINGLEAP, DAM_FORCE, FALSE);

    /* ornir decided to disable this, so i changed the skill from full around action
     * to a move action -zusuk
        struct affected_type af;
        new_affect(&af);
        af.spell = SKILL_SPRINGLEAP;
        if (!rand_number(0, 5))
          SET_BIT_AR(af.bitvector, AFF_PARALYZED);
        else
          SET_BIT_AR(af.bitvector, AFF_STUN);
        af.duration = dice(1, 2);
        affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE); */

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);
  }
  else
  {
    damage(ch, vict, 0, SKILL_SPRINGLEAP, DAM_FORCE, FALSE);
  }

  change_position(ch, POS_STANDING);
  USE_MOVE_ACTION(ch);
}

/* smite engine */
void perform_smite(struct char_data *ch, int smite_type)
{
  struct affected_type af;

  new_affect(&af);

  switch (smite_type)
  {
  case SMITE_TYPE_EVIL:
    af.spell = SKILL_SMITE_EVIL;
    if (!IS_NPC(ch))
      start_daily_use_cooldown(ch, FEAT_SMITE_EVIL);
    break;
  case SMITE_TYPE_GOOD:
    af.spell = SKILL_SMITE_GOOD;
    if (!IS_NPC(ch))
      start_daily_use_cooldown(ch, FEAT_SMITE_GOOD);
    break;
  case SMITE_TYPE_DESTRUCTION:
    af.spell = SKILL_SMITE_DESTRUCTION;
    if (!IS_NPC(ch))
      start_daily_use_cooldown(ch, FEAT_DESTRUCTIVE_SMITE);
    break;
  }
  af.duration = 24;

  affect_to_char(ch, &af);

  send_to_char(ch, "You prepare to wreak vengeance upon your foe.\r\n");
}

/* the primary engine for backstab */
bool perform_backstab(struct char_data *ch, struct char_data *vict)
{
  int blow_landed = 0, prob = 0, successful = 0, has_piercing = 0, assassin_mod = 2;
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD_1);
  bool make_aware = FALSE, marked_target = FALSE;

  if (AFF_FLAGGED(ch, AFF_HIDE))
    prob += 1; // minor bonus for being hidden
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    prob += 1; // minor bonus for being sneaky

  if (HAS_FEAT(ch, FEAT_BACKSTAB))
    prob += 4;

  if (attack_roll(ch, vict, ATTACK_TYPE_PRIMARY, FALSE, prob) > 0)
  {
    blow_landed = 1;
  }
  else
  {
    blow_landed = 0;
  }

  if (wielded && IS_SET(weapon_list[GET_WEAPON_TYPE(wielded)].damageTypes, DAMAGE_TYPE_PIERCING))
    has_piercing = 1;

  if (GET_RACE(ch) == RACE_TRELUX && !IS_NPC(ch))
  {
    has_piercing = 1;
  }

  /* try for primary 1handed weapon */
  if (has_piercing)
  {
    if (AWAKE(vict) && !blow_landed)
    {
      damage(ch, vict, 0, SKILL_BACKSTAB, DAM_PUNCTURE, FALSE);
    }
    else
    {
      apply_assassin_backstab_bonuses(ch, vict);
      hit(ch, vict, SKILL_BACKSTAB, DAM_PUNCTURE, 0, FALSE);
      make_aware = TRUE;
      // hidden weapons feat grants an extra attack
      if (is_marked_target(ch, vict) && HAS_FEAT(ch, FEAT_HIDDEN_WEAPONS) && 
          (skill_roll(ch, ABILITY_SLEIGHT_OF_HAND) >= skill_roll(vict, ABILITY_PERCEPTION)))
      {
        marked_target = TRUE;
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      // reach attacks get extra attack when combat starts
      if (has_reach(ch) && has_piercing)
      {
        send_to_char(ch, "You gain an extra attack because of having long reach.\r\n");
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    }
    successful++;
  }

  update_pos(vict);
  has_piercing = 0;

  wielded = GET_EQ(ch, WEAR_WIELD_OFFHAND);
  if (wielded && IS_SET(weapon_list[GET_WEAPON_TYPE(wielded)].damageTypes, DAMAGE_TYPE_PIERCING))
    has_piercing = 1;

  if (GET_RACE(ch) == RACE_TRELUX && !IS_NPC(ch))
  {
    has_piercing = 1;
  }

  /* try for offhand */
  if (vict && GET_POS(vict) > POS_DEAD && has_piercing)
  {
    if (AWAKE(vict) && !blow_landed)
    {
      damage(ch, vict, 0, SKILL_BACKSTAB, DAM_PUNCTURE, TRUE);
    }
    else
    {
      apply_assassin_backstab_bonuses(ch, vict);
      hit(ch, vict, SKILL_BACKSTAB, DAM_PUNCTURE, 0, TRUE);
      // hidden weapons feat grants an extra attack
      if (marked_target && HAS_FEAT(ch, FEAT_HIDDEN_WEAPONS) && (skill_roll(ch, ABILITY_SLEIGHT_OF_HAND) >= skill_roll(vict, ABILITY_PERCEPTION)))
      {
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
      make_aware = TRUE;
    }
    successful++;
  }
  update_pos(vict);
  has_piercing = 0;

  wielded = GET_EQ(ch, WEAR_WIELD_2H);
  if (wielded && IS_SET(weapon_list[GET_WEAPON_TYPE(wielded)].damageTypes, DAMAGE_TYPE_PIERCING))
    has_piercing = 1;
  if (GET_RACE(ch) == RACE_TRELUX && !IS_NPC(ch))
  {
    has_piercing = 1;
  }
  /* try for 2 handed */
  if (vict && GET_POS(vict) > POS_DEAD && has_piercing)
  {
    if (AWAKE(vict) && !blow_landed)
    {
      damage(ch, vict, 0, SKILL_BACKSTAB, DAM_PUNCTURE, TRUE);
    }
    else
    {
      apply_assassin_backstab_bonuses(ch, vict);
      hit(ch, vict, SKILL_BACKSTAB, DAM_PUNCTURE, 0, TRUE);
      make_aware = TRUE;
      // hidden weapons feat grants an extra attack
      if (is_marked_target(ch, vict) && HAS_FEAT(ch, FEAT_HIDDEN_WEAPONS) && (skill_roll(ch, ABILITY_SLEIGHT_OF_HAND) >= skill_roll(vict, ABILITY_PERCEPTION)))
      {
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
      
      // reach attacks get extra attack when combat starts
      if (has_reach(ch) && has_piercing)
      {
        send_to_char(ch, "You gain an extra attack because of having long reach.\r\n");
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    }
    successful++;
  }

  if (successful)
  {
    if (make_aware && !AFF_FLAGGED(vict, AFF_AWARE))
    {
      struct affected_type aware_affect;

      new_affect(&aware_affect);
      aware_affect.spell = SKILL_BACKSTAB;
      aware_affect.duration = 20;
      SET_BIT_AR(aware_affect.bitvector, AFF_AWARE);
      affect_join(vict, &aware_affect, TRUE, FALSE, FALSE, FALSE);
    }

    if (is_marked_target(ch, vict))
    {
      assassin_mod += GET_LEVEL(ch) - GET_LEVEL(vict);
      if (HAS_FEAT(ch, FEAT_DEATH_ATTACK))
        assassin_mod += 2;
      if (has_perk(ch, PERK_ROGUE_DEATH_ATTACK))
        assassin_mod += 2;
      if (HAS_FEAT(ch, FEAT_TRUE_DEATH))
        assassin_mod += 3;
      if (HAS_FEAT(ch, FEAT_QUIET_DEATH))
        assassin_mod += 2;
      if (HAS_FEAT(ch, FEAT_SWIFT_DEATH))
        assassin_mod += 2;
      if (HAS_FEAT(ch, FEAT_ANGEL_OF_DEATH))
        assassin_mod += 4;

      if (HAS_EVOLUTION(vict, EVOLUTION_UNDEAD_APPEARANCE))
        assassin_mod -= get_evolution_appearance_save_bonus(vict);

      if (!AFF_FLAGGED(vict, AFF_PARALYZED) && !paralysis_immunity(vict) &&
          !savingthrow(ch, vict, SAVING_FORT, assassin_mod, CAST_INNATE, IS_ROGUE_TYPE(ch), NOSCHOOL))
      {
        struct affected_type death_attack;

        new_affect(&death_attack);
        death_attack.spell = SPELL_AFFECT_DEATH_ATTACK;
        death_attack.duration = 2;
        SET_BIT_AR(death_attack.bitvector, AFF_PARALYZED);
        affect_join(vict, &death_attack, TRUE, FALSE, FALSE, FALSE);
        act("$n paralyzes $N as $e performs a death attack against $M!",
            FALSE, ch, NULL, vict, TO_NOTVICT);
        act("You paralyze $N as you perform a death attack!",
            FALSE, ch, NULL, vict, TO_CHAR);
        act("$n paralyzes you as $e performs a death attack!",
            FALSE, ch, NULL, vict, TO_VICT);
      }
    }

    if (HAS_FEAT(ch, FEAT_SWIFT_DEATH))
    {
      USE_SWIFT_ACTION(ch);
    }
    else if (HAS_FEAT(ch, FEAT_BACKSTAB) <= 0)
    {
      USE_FULL_ROUND_ACTION(ch);
    }
    else
    {
      USE_MOVE_ACTION(ch);
    }

    if (!FIGHTING(ch))
      set_fighting(ch, vict);
    if (!FIGHTING(vict))
      set_fighting(vict, ch);

    return TRUE;
  }
  else
    send_to_char(ch, "You have no piercing weapon equipped.\r\n");

  return FALSE;
}

/* this is the event function for whirlwind */
EVENTFUNC(event_whirlwind)
{
  struct char_data *ch = NULL, *tch = NULL;
  struct mud_event_data *pMudEvent = NULL;
  struct list_data *room_list = NULL;
  int count = 0;

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;
  if (!IS_PLAYING(ch->desc))
    return 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE))
  {
    send_to_char(ch, "You simply can't reach in this area.\r\n");
    return 0;
  }

  /* When using a list, we have to make sure to allocate the list as it
   * uses dynamic memory */
  room_list = create_list();

  /* We search through the "next_in_room", and grab all NPCs and add them
   * to our list */
  if (!IN_ROOM(ch))
    return 0;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    if (IS_NPC(tch))
      add_to_list(tch, room_list);

  /* If our list is empty or has "0" entries, we free it from memory and
   * close off our event */
  if (room_list->iSize == 0)
  {
    free_list(room_list);
    send_to_char(ch, "There is no one in the room to whirlwind!\r\n");
    return 0;
  }

  if (GET_HIT(ch) < 1)
  {
    return 0;
  }

  /* We spit out some ugly colour, making use of the new colour options,
   * to let the player know they are performing their whirlwind strike */
  send_to_char(ch, "\t[f313]You deliver a vicious \t[f014]\t[b451]WHIRLWIND!!!\tn\r\n");

  /* Lets grab some a random NPC from the list, and hit() them up */
  for (count = dice(1, 4); count > 0; count--)
  {
    tch = random_from_list(room_list);
    hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }

  /* Now that our attack is done, let's free out list */
  if (room_list)
    free_list(room_list);

  /* The "return" of the event function is the time until the event is called
   * again. If we return 0, then the event is freed and removed from the list, but
   * any other numerical response will be the delay until the next call */
  if (20 < rand_number(1, 101))
  {
    send_to_char(ch, "You stop spinning.\r\n");
    return 0;
  }
  else
    return 4 * PASSES_PER_SEC;
}

/******* start offensive commands *******/

ACMDCHECK(can_turnundead)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_TURN_UNDEAD, "You do not possess divine favor!\r\n");
  ACMDCHECK_PERMFAIL_IF(CLASS_LEVEL(ch, CLASS_CLERIC) <= 0 && CLASS_LEVEL(ch, CLASS_PALADIN) <= 2, "You do not possess divine favor!\r\n");
  return CAN_CMD;
}

/* this is the engine for turn undead */
int perform_turnundead(struct char_data *ch, struct char_data *vict, int turn_level)
{

  /* this is redundant IF you came in the normal way, but its here in case we are coming in more directly */
  if (!vict || !IS_UNDEAD(vict))
  {
    send_to_char(ch, "You can only attempt to turn undead!\r\n");
    return 0;
  }

  /* powerful being mechanic here will override the turn_level */
  if (IS_POWERFUL_BEING(ch))
  {
    turn_level = GET_LEVEL(ch) + 1;
  }

  /* Apply turn undead perk bonuses */
  int turn_bonus = 0;
  int destroy_threshold = 0;
  
  /* Turn Undead Enhancement I & II: +DC bonus (affects effective level difference) */
  turn_bonus += get_cleric_turn_undead_enhancement_bonus(ch);
  
  /* Master of the Undead: Additional +5 DC bonus */
  turn_bonus += get_cleric_master_of_undead_dc_bonus(ch);
  
  /* Greater Turning: Affect undead +2 HD levels higher */
  int greater_turning_bonus = get_cleric_greater_turning_bonus(ch);
  
  /* Paladin Turn Undead Mastery: +HD bonus */
  if (CLASS_LEVEL(ch, CLASS_PALADIN) > 0)
  {
    int paladin_turn_hd_bonus = get_paladin_turn_undead_hd_bonus(ch);
    if (paladin_turn_hd_bonus > 0)
      greater_turning_bonus += paladin_turn_hd_bonus;
  }
  
  /* Destroy Undead: Get HD threshold for instant destruction */
  destroy_threshold = get_destroy_undead_threshold(ch);

  int turn_difference = (turn_level - GET_LEVEL(vict)) + turn_bonus + greater_turning_bonus;
  int turn_roll = d20(ch);
  int turn_result = 0;

  switch (turn_difference)
  {
  case -5:
  case -4:
    if (turn_roll >= 20)
      turn_result = 1;
    break;
  case -3:
    if (turn_roll >= 17)
      turn_result = 1;
    break;
  case -2:
    if (turn_roll >= 15)
      turn_result = 1;
    break;
  case -1:
    if (turn_roll >= 13)
      turn_result = 1;
    break;
  case 0:
    if (turn_roll >= 11)
      turn_result = 1;
    break;
  case 1:
    if (turn_roll >= 9)
      turn_result = 1;
    break;
  case 2:
    if (turn_roll >= 6)
      turn_result = 1;
    break;
  case 3:
    if (turn_roll >= 3)
      turn_result = 1;
    break;
  case 4:
  case 5:
    if (turn_roll >= 2)
      turn_result = 1;
    break;
  default:
    turn_result = 0;
    break;
  }

  if (turn_difference <= -6)
    turn_result = 0;
  else if (turn_difference >= 6)
    turn_result = 2;

  if (turn_result >= 1 && !IS_NPC(vict))
    turn_result = 3;
  
  /* Check for Destroy Undead perk: instantly destroy weak undead */
  if (turn_result >= 1 && destroy_threshold > 0)
  {
    int hd_difference = turn_level - GET_LEVEL(vict);
    if (hd_difference >= destroy_threshold)
    {
      turn_result = 2; /* Upgrade to destroy */
    }
  }

  /* messaging! */
  act("You raise your divine symbol toward $N declaring, 'BEGONE!'", FALSE, ch, 0, vict, TO_CHAR);
  act("$n raises $s divine symbol towards you declaring, 'BEGONE!'", FALSE, ch, 0, vict, TO_VICT);
  act("$n raises $s divine symbol toward $N declaring, 'BEGONE!'", FALSE, ch, 0, vict, TO_NOTVICT);

  switch (turn_result)
  {
  case 0: /* Undead resists turning */
    act("$N blasphemously mocks your faith!", FALSE, ch, 0, vict, TO_CHAR);
    act("You blasphemously mock $N and $S faith!", FALSE, vict, 0, ch, TO_CHAR);
    act("$n blasphemously mocks $N and $S faith!", FALSE, vict, 0, ch, TO_NOTVICT);
    if (!FIGHTING(vict))
      hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    break;
  case 1: /* Undead is turned */

    /* nice nasty bonus to powerful beings turn ability! */
    if (IS_POWERFUL_BEING(ch))
    {
      call_magic(ch, vict, 0, SPELL_GREATER_RUIN, 0, GET_LEVEL(ch), CAST_INNATE);
    }

    /* Master of the Undead: Control instead of turn */
    if (has_control_undead(ch) && IS_NPC(vict))
    {
      /* Check if can add follower */
      if (!can_add_follower_by_flag(ch, MOB_C_O_T_N))
      {
        act("You cannot control any more undead creatures!", FALSE, ch, 0, vict, TO_CHAR);
        act("The power of $N's faith overwhelms $n, who flees!", FALSE, vict, 0, ch, TO_NOTVICT);
        do_flee(vict, 0, 0, 0);
      }
      else
      {
        act("The power of your faith bends $N to your will!", FALSE, ch, 0, vict, TO_CHAR);
        act("The power of $N's faith bends you to $S will!", FALSE, vict, 0, ch, TO_CHAR);
        act("The power of $N's faith bends $n to $S will!", FALSE, vict, 0, ch, TO_NOTVICT);
        
        /* Add as follower and charm */
        if (vict->master)
          stop_follower(vict);
        add_follower(vict, ch);
        SET_BIT_AR(AFF_FLAGS(vict), AFF_CHARM);
        SET_BIT_AR(MOB_FLAGS(vict), MOB_C_O_T_N);
      }
    }
    else
    {
      act("The power of your faith overwhelms $N, who flees!", FALSE, ch, 0, vict, TO_CHAR);
      act("The power of $N's faith overwhelms you! You flee in terror!!!", FALSE, vict, 0, ch, TO_CHAR);
      act("The power of $N's faith overwhelms $n, who flees!", FALSE, vict, 0, ch, TO_NOTVICT);
      do_flee(vict, 0, 0, 0);
    }
    break;
  case 2: /* Undead is automatically destroyed */
    act("The mighty force of your faith blasts $N out of existence!", FALSE, ch, 0, vict, TO_CHAR);
    act("The mighty force of $N's faith blasts you out of existence!", FALSE, vict, 0, ch, TO_CHAR);
    act("The mighty force of $N's faith blasts $n out of existence!", FALSE, vict, 0, ch, TO_NOTVICT);
    
    /* Holy Avenger: Apply spell boost after destroying undead */
    if (has_paladin_holy_avenger(ch))
    {
      struct affected_type af;
      new_affect(&af);
      af.spell = SKILL_HOLY_AVENGER;
      af.duration = 1; /* 1 round */
      af.modifier = 4; /* +4 caster level stored in modifier */
      af.location = APPLY_SPECIAL;
      affect_to_char(ch, &af);
      
      send_to_char(ch, "\tWHoly power surges through you, enhancing your next spell!\tn\r\n");
    }
    
    dam_killed_vict(ch, vict);
    break;
  case 3:
    act("The mighty force of your faith blasts $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("The mighty force of $N's faith blasts you!", FALSE, vict, 0, ch, TO_CHAR);
    act("The mighty force of $N's faith blasts $n!", FALSE, vict, 0, ch, TO_NOTVICT);
    
    /* Base damage */
    int turn_damage = dice(GET_LEVEL(ch) / 2, 6);
    
    /* Paladin Turn Undead Mastery II: +damage bonus */
    if (CLASS_LEVEL(ch, CLASS_PALADIN) > 0)
    {
      int paladin_turn_damage_bonus = get_paladin_turn_undead_damage_bonus(ch);
      if (paladin_turn_damage_bonus > 0)
        turn_damage += dice(paladin_turn_damage_bonus, 6);
    }
    
    damage(ch, vict, turn_damage, SPELL_GREATER_RUIN, DAM_HOLY, FALSE);
    break;
  }

  return 1;
}

/* turn undead skill (clerics, paladins, etc) */
ACMD(do_turnundead)
{
  struct char_data *vict = NULL;
  int turn_level = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_turnundead);

  if (CLASS_LEVEL(ch, CLASS_PALADIN) > 2)
    turn_level += CLASS_LEVEL(ch, CLASS_PALADIN) - 2;
  turn_level += CLASS_LEVEL(ch, CLASS_CLERIC);

  if (turn_level <= 0)
  {
    // This should never happen because of the ACMDCHECK_PERMFAIL_IF() in can_turnundead, but leave it in place just to be safe.
    send_to_char(ch, "You do not possess divine favor!\r\n");
    return;
  }

  one_argument(argument, buf, sizeof(buf));
  if (!(vict = get_char_room_vis(ch, buf, NULL)))
  {
    send_to_char(ch, "Turn who?\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "How do you plan to turn yourself?\r\n");
    return;
  }

  if (!IS_UNDEAD(vict))
  {
    send_to_char(ch, "You can only attempt to turn undead!\r\n");
    return;
  }

  /* too powerful */
  if (GET_LEVEL(vict) >= LVL_IMMORT)
  {
    send_to_char(ch, "This undead is too powerful!\r\n");
    return;
  }

  /* this is the engine and messaging! */
  perform_turnundead(ch, vict, turn_level);

  /* Actions */
  USE_STANDARD_ACTION(ch);
  start_daily_use_cooldown(ch, FEAT_TURN_UNDEAD);
}

ACMDCHECK(can_channel_energy)
{
  /* Check if they have either the feat OR the paladin perk */
  if (!HAS_FEAT(ch, FEAT_CHANNEL_ENERGY) && !has_perk(ch, PERK_PALADIN_CHANNEL_ENERGY_1))
  {
    ACMDCHECK_TEMPFAIL_IF(true, "You do not possess divine favor!\r\n");
  }
  return CAN_CMD;
}

ACMDU(do_channelenergy)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_channel_energy);

  skip_spaces(&argument);
  int level = 0;
  bool has_feat = HAS_FEAT(ch, FEAT_CHANNEL_ENERGY);
  bool has_paladin_perk = has_perk(ch, PERK_PALADIN_CHANNEL_ENERGY_1);
  bool has_both = has_feat && has_paladin_perk;

  /* Only clerics (neutral alignment) need to choose energy type */
  if (has_feat && IS_NEUTRAL(ch) && ch->player_specials->saved.channel_energy_type == CHANNEL_ENERGY_TYPE_NONE)
  {
    if (!*argument)
    {
      send_to_char(ch, "As a neutral cleric you need to devote yourself to positive or negative energy.\r\n"
                       "This can only be changed with a respec. Ie. 'channel positive|negative'.\r\n");
      return;
    }
    if (is_abbrev(argument, "positive"))
    {
      ch->player_specials->saved.channel_energy_type = CHANNEL_ENERGY_TYPE_POSITIVE;
      send_to_char(ch, "You have devoted yourself to positive channeled energy.\r\n");
      return;
    }
    else if (is_abbrev(argument, "negative"))
    {
      ch->player_specials->saved.channel_energy_type = CHANNEL_ENERGY_TYPE_NEGATIVE;
      send_to_char(ch, "You have devoted yourself to negative channeled energy.\r\n");
      return;
    }
    else
    {
      send_to_char(ch, "As a neutral cleric you need to devote yourself to positive or negative energy.\r\n"
                       "This can only be changed with a respec. Ie. 'channel positive|negative'.\r\n");
      return;
    }
  }

  /* Check uses - when both feat and perk exist, check both cooldown pools */
  if (has_both)
  {
    /* Player has both - try feat first, then perk if feat is exhausted */
    int feat_uses = daily_uses_remaining(ch, FEAT_CHANNEL_ENERGY);
    struct mud_event_data *pMudEvent = NULL;
    int perk_uses = 0;
    int max_perk_uses = get_paladin_channel_energy_uses(ch); /* Returns 2 */
    
    if ((pMudEvent = char_has_mud_event(ch, ePALADIN_CHANNEL_ENERGY)))
    {
      if (pMudEvent->sVariables && sscanf(pMudEvent->sVariables, "uses:%d", &perk_uses) == 1)
      {
        /* perk_uses is how many used, calculate remaining */
        perk_uses = max_perk_uses - perk_uses;
      }
      else
      {
        perk_uses = max_perk_uses;
      }
    }
    else
    {
      perk_uses = max_perk_uses;
    }
    
    /* Check if both pools are exhausted */
    if (feat_uses <= 0 && perk_uses <= 0)
    {
      send_to_char(ch, "You must recover the divine energy required to channel energy.\r\n");
      return;
    }
  }
  else if (has_feat)
  {
    PREREQ_HAS_USES(FEAT_CHANNEL_ENERGY, "You must recover the divine energy required to channel energy.\r\n");
  }
  else if (has_paladin_perk)
  {
    /* Paladin perk only: Check manual use tracking - 2 uses per day */
    struct mud_event_data *pMudEvent = NULL;
    int uses = 0;
    int max_uses = get_paladin_channel_energy_uses(ch); /* Returns 2 */
    
    if ((pMudEvent = char_has_mud_event(ch, ePALADIN_CHANNEL_ENERGY)))
    {
      if (pMudEvent->sVariables && sscanf(pMudEvent->sVariables, "uses:%d", &uses) == 1)
      {
        if (uses >= max_uses)
        {
          send_to_char(ch, "You must recover the divine energy required to channel energy.\r\n");
          return;
        }
      }
    }
  }

  level = compute_channel_energy_level(ch);

  /* Paladins always channel positive energy (good aligned) */
  if (has_paladin_perk || IS_GOOD(ch) || ch->player_specials->saved.channel_energy_type == CHANNEL_ENERGY_TYPE_POSITIVE)
  {
    act("You channel positive energy.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n channels positive energy.", FALSE, ch, 0, 0, TO_ROOM);
    call_magic(ch, NULL, NULL, ABILITY_CHANNEL_POSITIVE_ENERGY, 0, level, CAST_INNATE);
  }
  else if (IS_EVIL(ch) || ch->player_specials->saved.channel_energy_type == CHANNEL_ENERGY_TYPE_NEGATIVE)
  {
    act("You channel negative energy.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n channels negative energy.", FALSE, ch, 0, 0, TO_ROOM);
    call_magic(ch, NULL, NULL, ABILITY_CHANNEL_NEGATIVE_ENERGY, 0, level, CAST_INNATE);
  }
  else
  {
    send_to_char(ch, "Error channeling energy. Please tell a staff member ERRCHANEN001.\r\n");
    return;
  }

  /* Actions */
  USE_STANDARD_ACTION(ch);
  
  /* Track uses appropriately - use feat pool first if available, then perk pool */
  if (has_both)
  {
    /* Try to use feat pool first */
    int feat_uses = daily_uses_remaining(ch, FEAT_CHANNEL_ENERGY);
    
    if (feat_uses > 0)
    {
      /* Use from feat pool */
      start_daily_use_cooldown(ch, FEAT_CHANNEL_ENERGY);
    }
    else
    {
      /* Feat pool exhausted, use perk pool */
      struct mud_event_data *pMudEvent = NULL;
      int uses = 0;
      char buf[128];
      
      if ((pMudEvent = char_has_mud_event(ch, ePALADIN_CHANNEL_ENERGY)))
      {
        /* Increment existing event */
        if (pMudEvent->sVariables && sscanf(pMudEvent->sVariables, "uses:%d", &uses) == 1)
        {
          uses++;
          free(pMudEvent->sVariables);
          snprintf(buf, sizeof(buf), "uses:%d", uses);
          pMudEvent->sVariables = strdup(buf);
        }
      }
      else
      {
        /* Create new event - resets every MUD day */
        attach_mud_event(new_mud_event(ePALADIN_CHANNEL_ENERGY, ch, "uses:1"), SECS_PER_MUD_DAY RL_SEC);
      }
    }
  }
  else if (has_feat)
  {
    start_daily_use_cooldown(ch, FEAT_CHANNEL_ENERGY);
  }
  else if (has_paladin_perk)
  {
    /* Manual cooldown tracking for paladin perk */
    struct mud_event_data *pMudEvent = NULL;
    int uses = 0;
    char buf[128];
    
    if ((pMudEvent = char_has_mud_event(ch, ePALADIN_CHANNEL_ENERGY)))
    {
      /* Increment existing event */
      if (pMudEvent->sVariables && sscanf(pMudEvent->sVariables, "uses:%d", &uses) == 1)
      {
        uses++;
        free(pMudEvent->sVariables);
        snprintf(buf, sizeof(buf), "uses:%d", uses);
        pMudEvent->sVariables = strdup(buf);
      }
    }
    else
    {
      /* Create new event - resets every MUD day */
      attach_mud_event(new_mud_event(ePALADIN_CHANNEL_ENERGY, ch, "uses:1"), SECS_PER_MUD_DAY RL_SEC);
    }
  }
}

/* Beacon of Hope - Divine Healer Tier 4 Capstone */
ACMDU(do_beaconofhope)
{
  struct char_data *tch = NULL, *next_tch = NULL;
  struct affected_type af;
  int healed_count = 0;
  
  PREREQ_CAN_FIGHT();
  
  /* Check for perk */
  if (!has_beacon_of_hope(ch))
  {
    send_to_char(ch, "You don't have the Beacon of Hope ability.\r\n");
    return;
  }
  
  /* Check cooldown - once per day */
  if (char_has_mud_event(ch, eBEACON_OF_HOPE))
  {
    send_to_char(ch, "You must wait before using Beacon of Hope again.\r\n");
    return;
  }
  
  /* Activate beacon */
  act("\tW$n becomes a \tYradiant beacon of hope\tW, divine light flooding the area!\tn", 
      FALSE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "\tWYou become a \tYradiant beacon of hope\tW, divine light flooding the area!\tn\r\n");
  
  /* Heal all allies in the room */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
  {
    next_tch = tch->next_in_room;
    
    /* Skip if not an ally */
    if (tch == ch || IS_NPC(tch))
      continue;
    
    if (!AFF_FLAGGED(ch, AFF_GROUP) || !AFF_FLAGGED(tch, AFF_GROUP))
      continue;
    
    /* Fully heal the ally */
    GET_HIT(tch) = GET_MAX_HIT(tch);
    GET_MOVE(tch) = GET_MAX_MOVE(tch);
    update_pos(tch);
    
    /* Grant +4 save bonus for 10 rounds */
    new_affect(&af);
    af.spell = PERK_CLERIC_BEACON_OF_HOPE;
    af.duration = 10;
    af.bonus_type = BONUS_TYPE_MORALE;
    af.location = APPLY_SAVING_FORT;
    af.modifier = 4;
    affect_to_char(tch, &af);
    
    af.location = APPLY_SAVING_REFL;
    affect_to_char(tch, &af);
    
    af.location = APPLY_SAVING_WILL;
    affect_to_char(tch, &af);
    
    send_to_char(tch, "\tWYou are \tYfully healed\tW and filled with \tYhope\tW! (+4 saves)\tn\r\n");
    healed_count++;
  }
  
  /* Heal self */
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  update_pos(ch);
  
  /* Grant save bonus to self */
  new_affect(&af);
  af.spell = PERK_CLERIC_BEACON_OF_HOPE;
  af.duration = 10;
  af.bonus_type = BONUS_TYPE_MORALE;
  af.location = APPLY_SAVING_FORT;
  af.modifier = 4;
  affect_to_char(ch, &af);
  
  af.location = APPLY_SAVING_REFL;
  affect_to_char(ch, &af);
  
  af.location = APPLY_SAVING_WILL;
  affect_to_char(ch, &af);
  
  send_to_char(ch, "\tWYou are \tYfully restored\tW and filled with \tYhope\tW! (+4 saves)\tn\r\n");
  
  if (healed_count > 0)
    send_to_char(ch, "\tWYou healed %d %s.\tn\r\n", healed_count, 
                 healed_count == 1 ? "ally" : "allies");
  
  /* Set daily cooldown - 2 hours */
  attach_mud_event(new_mud_event(eBEACON_OF_HOPE, ch, NULL), 2 * 60 * 60 * PASSES_PER_SEC);
  
  /* Actions */
  USE_STANDARD_ACTION(ch);
}

/* a function to clear rage and do other dirty work associated with that */
void clear_rage(struct char_data *ch)
{

  send_to_char(ch, "You calm down from your rage...\r\n");
  act("$n looks calmer now.", FALSE, ch, NULL, NULL, TO_ROOM);

  if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_TIRELESS_RAGE) &&
      !AFF_FLAGGED(ch, AFF_FATIGUED))
  {
    struct affected_type fatigued_af;

    send_to_char(ch, "You are left fatigued from the rage!\r\n");
    new_affect(&fatigued_af);
    fatigued_af.spell = SKILL_RAGE_FATIGUE;
    fatigued_af.duration = 10;
    SET_BIT_AR(fatigued_af.bitvector, AFF_FATIGUED);
    affect_join(ch, &fatigued_af, TRUE, FALSE, FALSE, FALSE);
  }

  /* clearing some rage powers */
  if (char_has_mud_event(ch, eSURPRISE_ACCURACY))
  {
    change_event_duration(ch, eSURPRISE_ACCURACY, 0);
  }
  if (char_has_mud_event(ch, ePOWERFUL_BLOW))
  {
    change_event_duration(ch, ePOWERFUL_BLOW, 0);
  }
  if (char_has_mud_event(ch, eCOME_AND_GET_ME))
  {
    change_event_duration(ch, eCOME_AND_GET_ME, 0);
  }
  
  /* Clear Indomitable Will auto-success flag when rage ends */
  if (affected_by_spell(ch, PERK_BERSERKER_INDOMITABLE_WILL))
  {
    affect_from_char(ch, PERK_BERSERKER_INDOMITABLE_WILL);
  }

  /* Remove whatever HP we granted.  This may kill the character. */
  /* PCs only.  Otherwise mobs will die and player won't get exp. */
  if (IS_NPC(ch))
  {
    GET_HIT(ch) = MAX(0, GET_HIT(ch) - (get_rage_bonus(ch) * 2));
  }
  else
  {
    GET_HIT(ch) -= get_rage_bonus(ch) * 2;
  }
  if (GET_HIT(ch) < 0)
  {
    send_to_char(ch, "Your rage no longer sustains you and you pass out!\r\n");
    act("$n passes out as $s rage no longer sustains $m.", FALSE, ch, NULL, NULL, TO_ROOM);
  }
}

/* a function to clear defensive stance and do other dirty work associated with that */
void clear_defensive_stance(struct char_data *ch)
{

  send_to_char(ch, "You feel your tension release as you relax your defensive stance...\r\n");

  if (!IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_FATIGUED))
  {
    struct affected_type fatigued_af;

    send_to_char(ch, "You are left fatigued from the efforts exerted during your defensive stance!\r\n");
    new_affect(&fatigued_af);
    fatigued_af.spell = SKILL_RAGE_FATIGUE;
    fatigued_af.duration = 10;
    SET_BIT_AR(fatigued_af.bitvector, AFF_FATIGUED);
    affect_join(ch, &fatigued_af, TRUE, FALSE, FALSE, FALSE);
  }

  /* clearing some defensive stance powers */
}

ACMDCHECK(can_defensive_stance)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_DEFENSIVE_STANCE, "You don't know how to use a defensive stance.\r\n");
  ACMDCHECK_TEMPFAIL_IF(AFF_FLAGGED(ch, AFF_FATIGUED), "You are are too fatigued to use defensive stance!\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_RAGE), "You can't enter a defensive stance while raging!\r\n");

  return CAN_CMD;
}

/* defensive stance skill stalwart defender primarily */
ACMD(do_defensive_stance)
{
  struct affected_type af, aftwo, afthree, affour, affive, afsix;
  int bonus = 0, duration = 0;

  PREREQ_CAN_FIGHT();

  if (affected_by_spell(ch, SKILL_DEFENSIVE_STANCE))
  {
    clear_defensive_stance(ch);
    affect_from_char(ch, SKILL_DEFENSIVE_STANCE);
    return;
  }

  PREREQ_CHECK(can_defensive_stance);
  PREREQ_HAS_USES(FEAT_DEFENSIVE_STANCE, "You must recover before you can use a defensive stance again.\r\n");

  /* bonus */
  bonus = 4;

  /* duration */
  duration = (6 + GET_CON_BONUS(ch)) + (CLASS_LEVEL(ch, CLASS_STALWART_DEFENDER) * 2);

  if (HAS_FEAT(ch, FEAT_BULWARK_OF_DEFENSE))
  {
    bonus = 6;
    duration *= 1.5;
  }

  send_to_char(ch, "\tcYou take on a \tWdefensive stance\tc!\tn\r\n");
  act("$n \tctakes on a \tWdefensive stance\tc!\tn", FALSE, ch, 0, 0, TO_ROOM);

  new_affect(&af);
  new_affect(&aftwo);
  new_affect(&afthree);
  new_affect(&affour);
  new_affect(&affive);
  new_affect(&afsix);

  af.spell = SKILL_DEFENSIVE_STANCE;
  af.duration = duration;
  af.location = APPLY_STR;
  af.modifier = bonus;
  af.bonus_type = BONUS_TYPE_CIRCUMSTANCE; /* stacks */

  aftwo.spell = SKILL_DEFENSIVE_STANCE;
  aftwo.duration = duration;
  aftwo.location = APPLY_CON;
  aftwo.modifier = bonus;
  aftwo.bonus_type = BONUS_TYPE_CIRCUMSTANCE; /* stacks */

  afthree.spell = SKILL_DEFENSIVE_STANCE;
  afthree.duration = duration;
  afthree.location = APPLY_SAVING_WILL;
  afthree.modifier = 2;
  afthree.bonus_type = BONUS_TYPE_CIRCUMSTANCE; /* stacks */

  affour.spell = SKILL_DEFENSIVE_STANCE;
  affour.duration = duration;
  affour.location = APPLY_AC_NEW;
  affour.modifier = 2;
  affour.bonus_type = BONUS_TYPE_CIRCUMSTANCE; /* stacks */

  affive.spell = SKILL_DEFENSIVE_STANCE;
  affive.duration = duration;
  affive.location = APPLY_SAVING_FORT;
  affive.modifier = 2;
  affive.bonus_type = BONUS_TYPE_CIRCUMSTANCE; /* stacks */

  afsix.spell = SKILL_DEFENSIVE_STANCE;
  afsix.duration = duration;
  afsix.location = APPLY_SAVING_REFL;
  afsix.modifier = 2;
  afsix.bonus_type = BONUS_TYPE_CIRCUMSTANCE; /* stacks */

  affect_to_char(ch, &af);
  affect_to_char(ch, &aftwo);
  affect_to_char(ch, &afthree);
  affect_to_char(ch, &affour);
  affect_to_char(ch, &affive);
  affect_to_char(ch, &afsix);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_DEFENSIVE_STANCE);

  if (!IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_STALWART_DEFENDER) < 1)
  {
    USE_STANDARD_ACTION(ch);
  }

  /* causing issues with balance? */
  GET_HIT(ch) += (bonus / 2) * GET_LEVEL(ch) + GET_CON_BONUS(ch);
}

ACMDCHECK(can_rage)
{
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_DEFENSIVE_STANCE), "You can't rage while using a defensive stance!\r\n");
  ACMDCHECK_TEMPFAIL_IF(AFF_FLAGGED(ch, AFF_FATIGUED), "You are are too fatigued to rage!\r\n");
  ACMDCHECK_PERMFAIL_IF(!IS_ANIMAL(ch) && !HAS_FEAT(ch, FEAT_RAGE), "You don't know how to rage.\r\n");
  return CAN_CMD;
}

/* rage skill (berserk) primarily for berserkers character class */
ACMD(do_rage)
{
  struct affected_type af[RAGE_AFFECTS];
  int bonus = 0, duration = 0, i = 0;

  PREREQ_CAN_FIGHT();

  /* If currently raging, all this does is stop. */
  if (affected_by_spell(ch, SKILL_RAGE))
  {
    clear_rage(ch);
    affect_from_char(ch, SKILL_RAGE);
    return;
  }

  PREREQ_CHECK(can_rage);

  if (!IS_NPC(ch))
  {
    PREREQ_HAS_USES(FEAT_RAGE, "You must recover before you can go into a rage.\r\n");
  }

  /* bonus */
  bonus = get_rage_bonus(ch);

  /* duration */
  duration = 12 + GET_CON_BONUS(ch) * 3;

  send_to_char(ch, "You go into a \tRR\trA\tRG\trE\tn!.\r\n");
  act("$n goes into a \tRR\trA\tRG\trE\tn!", FALSE, ch, 0, 0, TO_ROOM);

  /* init affect array */
  for (i = 0; i < RAGE_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SKILL_RAGE;
    af[i].duration = duration;
    af[i].bonus_type = BONUS_TYPE_MORALE;
  }

  af[0].location = APPLY_STR;
  af[0].modifier = bonus;

  af[1].location = APPLY_CON;
  af[1].modifier = bonus;

  af[2].location = APPLY_SAVING_WILL;
  af[2].modifier = bonus;
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_INDOMITABLE_WILL))
  {
    af[2].modifier += 4;
  }

  af[3].location = APPLY_AC_NEW;
  af[3].modifier = -2; /* penalty! */

  af[4].location = APPLY_HIT;
  af[4].modifier = bonus * 2;

  /* hit/damroll bonuses */
  bonus = 1;
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_INDOMITABLE_RAGE))
    bonus += 3;

  af[5].location = APPLY_HITROLL;
  af[5].modifier = bonus;

  af[5].location = APPLY_DAMROLL;
  af[5].modifier = bonus;
  /******/

  for (i = 0; i < RAGE_AFFECTS; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);

  GET_HIT(ch) += bonus * 2;
  if (GET_HIT(ch) > GET_MAX_HIT(ch))
    GET_HIT(ch) = GET_MAX_HIT(ch);

  save_char(ch, 0); /* this is redundant but doing it for dummy sakes */

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_RAGE);

  if (!IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_BERSERKER) < 1)
  {
    USE_STANDARD_ACTION(ch);
  }

  /* bonus hp from the rage */
  // GET_HIT(ch) += bonus * GET_LEVEL(ch) + GET_CON_BONUS(ch) + 1;
  save_char(ch, 0); /* this is redundant but doing it for dummy sakes */

  /* Blinding Rage perk - blind enemies when entering rage */
  if (has_berserker_blinding_rage(ch))
  {
    struct char_data *tch = NULL, *next_tch = NULL;
    
    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;
      
      /* Only affect enemies currently fighting the berserker */
      if (tch == ch || !IS_NPC(tch) || FIGHTING(tch) != ch)
        continue;
      
      /* Check if already blind */
      if (AFF_FLAGGED(tch, AFF_BLIND))
        continue;
      
      /* Make a Will save */
      int dc = 10 + GET_LEVEL(ch) + GET_CHA_BONUS(ch);
      if (!savingthrow(ch, tch, SAVING_WILL, dc, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      {
        struct affected_type af;
        int blind_duration = dice(1, 4); // 1d4 rounds
        
        new_affect(&af);
        af.spell = SKILL_RAGE;
        af.duration = blind_duration;
        SET_BIT_AR(af.bitvector, AFF_BLIND);
        affect_to_char(tch, &af);
        
        act("Your rage blinds $N with overwhelming fury!", FALSE, ch, 0, tch, TO_CHAR);
        act("$n's rage blinds you with overwhelming fury!", FALSE, ch, 0, tch, TO_VICT);
        act("$n's rage blinds $N with overwhelming fury!", FALSE, ch, 0, tch, TO_NOTVICT);
      }
    }
  }
  
  /* Stunning Blow perk - set flag for next attack to stun */
  if (has_berserker_stunning_blow(ch))
  {
    /* Set a flag that will be checked in hit() function */
    SET_BIT_AR(AFF_FLAGS(ch), AFF_NEXTATTACK_STUN);
    send_to_char(ch, "Your next attack will carry \tYoverwhelming force\tn!\r\n");
  }

  return;
}

/* Sprint - Berserker Primal Warrior ability */
ACMD(do_sprint)
{
  struct affected_type af;
  int duration = 5; // 5 rounds

  PREREQ_CAN_FIGHT();

  /* Check if already sprinting */
  if (affected_by_spell(ch, SKILL_SPRINT))
  {
    send_to_char(ch, "You are already sprinting!\r\n");
    return;
  }

  /* Check if they have the perk */
  if (!has_berserker_sprint(ch))
  {
    send_to_char(ch, "You don't know how to sprint!\r\n");
    return;
  }

  /* Check cooldown using the feat system - treat it like a daily use feat */
  if (!IS_NPC(ch))
  {
    PREREQ_HAS_USES(SKILL_SPRINT, "You must recover before you can sprint again.\r\n");
  }

  send_to_char(ch, "You break into a powerful \tYsprint\tn, your legs a blur of motion!\r\n");
  act("$n suddenly breaks into a powerful \tYsprint\tn, moving with incredible speed!", 
      FALSE, ch, 0, 0, TO_ROOM);

  /* Create the sprint affect */
  new_affect(&af);
  af.spell = SKILL_SPRINT;
  af.duration = duration;
  af.location = APPLY_NONE; // Movement speed is handled by movement_cost.c checking for sprint affect
  af.modifier = 0;
  af.bonus_type = BONUS_TYPE_INHERENT;

  affect_to_char(ch, &af);

  /* Start cooldown - 2 minutes in game */
  if (!IS_NPC(ch))
  {
    start_daily_use_cooldown(ch, SKILL_SPRINT);
  }

  USE_MOVE_ACTION(ch);

  return;
}

ACMD(do_reckless_abandon)
{
  struct affected_type af;
  int duration = 5; // 5 rounds

  PREREQ_CAN_FIGHT();

  /* Check if already using reckless abandon */
  if (affected_by_spell(ch, SKILL_RECKLESS_ABANDON))
  {
    send_to_char(ch, "You are already fighting with reckless abandon!\r\n");
    return;
  }

  /* Check if they have the perk */
  if (!has_berserker_reckless_abandon(ch))
  {
    send_to_char(ch, "You don't know how to fight with reckless abandon!\r\n");
    return;
  }

  /* Check cooldown - 5 minute cooldown */
  if (!IS_NPC(ch))
  {
    PREREQ_HAS_USES(SKILL_RECKLESS_ABANDON, "You must recover before you can use reckless abandon again.\r\n");
  }

  /* Must be raging to use */
  if (!affected_by_spell(ch, SKILL_RAGE))
  {
    send_to_char(ch, "You must be raging to use reckless abandon!\r\n");
    return;
  }

  send_to_char(ch, "You throw aside all defensive concerns and attack with \tRreckless abandon\tn!\r\n");
  act("$n's eyes blaze with fury as $e attacks with \tRreckless abandon\tn!", 
      FALSE, ch, 0, 0, TO_ROOM);

  /* +4 to hit bonus */
  new_affect(&af);
  af.spell = SKILL_RECKLESS_ABANDON;
  af.duration = duration;
  af.location = APPLY_HITROLL;
  af.modifier = 4;
  af.bonus_type = BONUS_TYPE_MORALE;
  affect_to_char(ch, &af);

  /* +8 damage bonus */
  new_affect(&af);
  af.spell = SKILL_RECKLESS_ABANDON;
  af.duration = duration;
  af.location = APPLY_DAMROLL;
  af.modifier = 8;
  af.bonus_type = BONUS_TYPE_MORALE;
  affect_to_char(ch, &af);

  /* -4 AC penalty */
  new_affect(&af);
  af.spell = SKILL_RECKLESS_ABANDON;
  af.duration = duration;
  af.location = APPLY_AC_NEW;
  af.modifier = -4;
  af.bonus_type = BONUS_TYPE_UNDEFINED; /* Penalties don't have a specific bonus type */
  affect_to_char(ch, &af);

  /* Start cooldown - 5 minutes */
  if (!IS_NPC(ch))
  {
    start_daily_use_cooldown(ch, SKILL_RECKLESS_ABANDON);
  }

  USE_SWIFT_ACTION(ch);

  return;
}

ACMD(do_warcry)
{
  struct affected_type af;
  struct char_data *tch = NULL;
  int duration = 5; // 5 rounds

  PREREQ_CAN_FIGHT();

  /* Check if they have the perk */
  if (!has_berserker_war_cry(ch))
  {
    send_to_char(ch, "You don't know how to use a war cry!\r\n");
    return;
  }

  /* Check cooldown - 5 minute cooldown */
  if (!IS_NPC(ch))
  {
    PREREQ_HAS_USES(SKILL_WAR_CRY, "You must recover before you can use war cry again.\r\n");
  }

  send_to_char(ch, "You unleash a mighty \tRWAR CRY\tn that echoes across the battlefield!\r\n");
  act("$n unleashes a mighty \tRWAR CRY\tn that echoes across the battlefield!", 
      FALSE, ch, 0, 0, TO_ROOM);

  /* Apply buff to all group members in the same room */
  if (GROUP(ch))
  {
    while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
    {
      if (IN_ROOM(tch) == IN_ROOM(ch))
      {
        /* +2 attack bonus */
        new_affect(&af);
        af.spell = SKILL_WAR_CRY_ALLY;
        af.duration = duration;
        af.location = APPLY_HITROLL;
        af.modifier = 2;
        af.bonus_type = BONUS_TYPE_MORALE;
        affect_to_char(tch, &af);

        /* +2 damage bonus */
        new_affect(&af);
        af.spell = SKILL_WAR_CRY_ALLY;
        af.duration = duration;
        af.location = APPLY_DAMROLL;
        af.modifier = 2;
        af.bonus_type = BONUS_TYPE_MORALE;
        affect_to_char(tch, &af);

        if (tch != ch)
        {
          send_to_char(tch, "You feel empowered by %s's \tGwar cry\tn!\r\n", GET_NAME(ch));
        }
      }
    }
  }
  else
  {
    /* Solo - buff self */
    new_affect(&af);
    af.spell = SKILL_WAR_CRY_ALLY;
    af.duration = duration;
    af.location = APPLY_HITROLL;
    af.modifier = 2;
    af.bonus_type = BONUS_TYPE_MORALE;
    affect_to_char(ch, &af);

    new_affect(&af);
    af.spell = SKILL_WAR_CRY_ALLY;
    af.duration = duration;
    af.location = APPLY_DAMROLL;
    af.modifier = 2;
    af.bonus_type = BONUS_TYPE_MORALE;
    affect_to_char(ch, &af);
  }

  /* Apply debuff to all enemies currently fighting you or your group */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch || !IS_NPC(tch))
      continue;

    /* Check if this enemy is fighting the berserker or any group member */
    bool is_fighting_group = FALSE;
    if (FIGHTING(tch) == ch)
    {
      is_fighting_group = TRUE;
    }
    else if (GROUP(ch))
    {
      struct char_data *gch = NULL;
      while ((gch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
      {
        if (FIGHTING(tch) == gch && IN_ROOM(gch) == IN_ROOM(ch))
        {
          is_fighting_group = TRUE;
          break;
        }
      }
    }

    if (is_fighting_group)
    {
      /* -2 attack penalty */
      new_affect(&af);
      af.spell = SKILL_WAR_CRY_ENEMY;
      af.duration = duration;
      af.location = APPLY_HITROLL;
      af.modifier = -2;
      af.bonus_type = BONUS_TYPE_UNDEFINED; /* Penalties don't have a specific bonus type */
      affect_to_char(tch, &af);

      /* -2 damage penalty */
      new_affect(&af);
      af.spell = SKILL_WAR_CRY_ENEMY;
      af.duration = duration;
      af.location = APPLY_DAMROLL;
      af.modifier = -2;
      af.bonus_type = BONUS_TYPE_UNDEFINED; /* Penalties don't have a specific bonus type */
      affect_to_char(tch, &af);

      act("$N recoils from your \tRwar cry\tn!", FALSE, ch, 0, tch, TO_CHAR);
      act("You recoil from $n's \tRwar cry\tn!", FALSE, ch, 0, tch, TO_VICT);
    }
  }

  /* Start cooldown - 5 minutes */
  if (!IS_NPC(ch))
  {
    start_daily_use_cooldown(ch, SKILL_WAR_CRY);
  }

  USE_STANDARD_ACTION(ch);

  return;
}

ACMD(do_earthshaker)
{
  struct char_data *tch = NULL, *next_tch = NULL;
  int dam_amount = 0;

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_PEACEFUL_ROOM();

  /* Check if they have the perk */
  if (!has_berserker_earthshaker(ch))
  {
    send_to_char(ch, "You don't know how to use earthshaker!\r\n");
    return;
  }

  /* Check cooldown - 30 second cooldown */
  if (!IS_NPC(ch))
  {
    PREREQ_HAS_USES(SKILL_EARTHSHAKER, "You must recover before you can use earthshaker again.\r\n");
  }

  /* Calculate damage based on STR modifier */
  dam_amount = GET_STR_BONUS(ch);
  if (dam_amount < 1)
    dam_amount = 1;

  send_to_char(ch, "You slam the ground with tremendous force, causing the earth to \tYSHAKE\tn!\r\n");
  act("$n slams the ground with tremendous force, causing the earth to \tYSHAKE\tn!", 
      FALSE, ch, 0, 0, TO_ROOM);

  /* Knock down all enemies currently fighting you or your group members */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
  {
    next_tch = tch->next_in_room;

    if (tch == ch || !IS_NPC(tch))
      continue;

    /* Check if this enemy is fighting the berserker or any group member */
    bool is_fighting_group = FALSE;
    if (FIGHTING(tch) == ch)
    {
      is_fighting_group = TRUE;
    }
    else if (GROUP(ch))
    {
      struct char_data *gch = NULL;
      while ((gch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
      {
        if (FIGHTING(tch) == gch && IN_ROOM(gch) == IN_ROOM(ch))
        {
          is_fighting_group = TRUE;
          break;
        }
      }
    }

    if (is_fighting_group)
    {
      /* Deal damage */
      if (dam_amount > 0)
      {
        damage(ch, tch, dam_amount, SKILL_EARTHSHAKER, DAM_FORCE, FALSE);
      }

      /* Knock prone (no save) - but check NOBASH */
      if (!IS_NPC(tch) || !MOB_FLAGGED(tch, MOB_NOBASH))
      {
        change_position(tch, POS_SITTING);
        act("You are knocked to the ground!", FALSE, ch, 0, tch, TO_VICT);
        act("$N is knocked to the ground!", FALSE, ch, 0, tch, TO_CHAR);
        act("$N is knocked to the ground!", FALSE, ch, 0, tch, TO_NOTVICT);
      }
      else
      {
        act("$N resists being knocked down!", FALSE, ch, 0, tch, TO_CHAR);
      }
    }
  }

  /* Start cooldown - 30 seconds */
  if (!IS_NPC(ch))
  {
    start_daily_use_cooldown(ch, SKILL_EARTHSHAKER);
  }

  USE_SWIFT_ACTION(ch);

  return;
}

/* hardy - berserker perk */
ACMD(do_hardy)
{
  struct affected_type af[2];
  int duration = 10; // 10 rounds

  PREREQ_CAN_FIGHT();

  if (!has_berserker_hardy(ch))
  {
    send_to_char(ch, "You do not have the Hardy perk.\r\n");
    return;
  }

  if (affected_by_spell(ch, SKILL_HARDY))
  {
    send_to_char(ch, "You are already hardy!\r\n");
    return;
  }

  send_to_char(ch, "You steel yourself, becoming \tRhardy\tn and resilient!\r\n");
  act("$n steels $mself, becoming more \tRhardy\tn and resilient!", FALSE, ch, 0, 0, TO_ROOM);

  /* init affect array */
  new_affect(&(af[0]));
  af[0].spell = SKILL_HARDY;
  af[0].duration = duration;
  af[0].bonus_type = BONUS_TYPE_MORALE;
  af[0].location = APPLY_CON;
  af[0].modifier = 2;

  new_affect(&(af[1]));
  af[1].spell = SKILL_HARDY;
  af[1].duration = duration;
  af[1].bonus_type = BONUS_TYPE_MORALE;
  af[1].location = APPLY_SAVING_FORT;
  af[1].modifier = 1;

  affect_join(ch, &af[0], FALSE, FALSE, FALSE, FALSE);
  affect_join(ch, &af[1], FALSE, FALSE, FALSE, FALSE);

  USE_STANDARD_ACTION(ch);

  return;
}

/* inner fire - sacred fist feat */
ACMD(do_innerfire)
{
  PREREQ_CAN_FIGHT();

  if (!HAS_FEAT(ch, FEAT_INNER_FIRE))
  {
    send_to_char(ch, "You do not know how to use inner fire...\r\n");
    return;
  }

  if (affected_by_spell(ch, SKILL_INNER_FIRE))
  {
    send_to_char(ch, "You are already using inner fire!\r\n");
    return;
  }

  if (!IS_NPC(ch))
  {
    PREREQ_HAS_USES(FEAT_INNER_FIRE, "You must recover before you can use inner fire again.\r\n");
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_INNER_FIRE);

  /*engine*/
  perform_inner_fire(ch);
}

/* sacred flames - sacred fist feat */
ACMD(do_sacredflames)
{
  PREREQ_CAN_FIGHT();

  if (!HAS_FEAT(ch, FEAT_SACRED_FLAMES))
  {
    send_to_char(ch, "You do not know how to use sacred flames...\r\n");
    return;
  }

  if (affected_by_spell(ch, SKILL_SACRED_FLAMES))
  {
    send_to_char(ch, "You are already using sacred flames!\r\n");
    return;
  }

  if (!IS_NPC(ch))
  {
    PREREQ_HAS_USES(FEAT_SACRED_FLAMES, "You must recover before you can use sacred flames again.\r\n");
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SACRED_FLAMES);

  /*engine*/
  perform_sacred_flames(ch);
}

ACMDCHECK(can_dragonborn_breath_weapon)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_DRAGONBORN_BREATH, "You do not have access to this ability.\r\n");
  return CAN_CMD;
}

/* Data structure for dragonborn breath weapon callback */
struct dragonborn_breath_data {
  int level;
  int dam_type;
};

/* Callback for dragonborn breath weapon AoE damage */
static int dragonborn_breath_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  struct dragonborn_breath_data *breath_data = (struct dragonborn_breath_data *)data;
  
  if (breath_data->level <= 15)
    damage(ch, tch, dice(breath_data->level, 6), SPELL_DRAGONBORN_ANCESTRY_BREATH, 
           breath_data->dam_type, FALSE);
  else
    damage(ch, tch, dice(breath_data->level, 14), SPELL_DRAGONBORN_ANCESTRY_BREATH, 
           breath_data->dam_type, FALSE);
  
  return 1;
}

ACMD(do_dragonborn_breath_weapon)
{
  struct dragonborn_breath_data breath_data;

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_dragonborn_breath_weapon);

  if (!IS_NPC(ch))
  {
    PREREQ_HAS_USES(FEAT_DRAGONBORN_BREATH, "You must recover before you can use your dragonborn ancestry breath weapon again.\r\n");
    send_to_char(ch, "You have %d uses remaining.\r\n", uses_remaining - 1);
  }

  PREREQ_NOT_PEACEFUL_ROOM();

  send_to_char(ch, "You exhale breathing out %s!\r\n", DRCHRT_ENERGY_TYPE(GET_DRAGONBORN_ANCESTRY(ch)));
  char to_room[200];
  sprintf(to_room, "$n exhales breathing %s!", DRCHRT_ENERGY_TYPE(GET_DRAGONBORN_ANCESTRY(ch)));
  act(to_room, FALSE, ch, 0, 0, TO_ROOM);

  breath_data.dam_type = draconic_heritage_energy_types[GET_DRAGONBORN_ANCESTRY(ch)];
  breath_data.level = GET_LEVEL(ch);

  aoe_effect(ch, SPELL_DRAGONBORN_ANCESTRY_BREATH, dragonborn_breath_callback, &breath_data);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_DRAGONBORN_BREATH);
  USE_STANDARD_ACTION(ch);
}

ACMD(do_assist)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *helpee = NULL;

  PREREQ_CAN_FIGHT();

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You're already fighting!  How can you assist someone else?\r\n");
    return;
  }
  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
    send_to_char(ch, "Whom do you wish to assist?\r\n");
  else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else
    perform_assist(ch, helpee);
}

ACMD(do_hit)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;
  int chInitiative = 0, victInitiative = 0, i = 0;
  struct char_data *mob = NULL;
  bool found = false;
  char mob_keys[200];

  PREREQ_CAN_FIGHT();

  /* temporary solution */
  if (is_using_ranged_weapon(ch, TRUE))
  {
    send_to_char(ch, "You can't use a ranged weapon in melee combat, use 'fire' "
                     "instead..\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_AUTOHIT))
    {
      send_to_char(ch, "Hit who?\r\n");
      return;
    }
    if (IS_NPC(ch) && (!ch->master  || !PRF_FLAGGED(ch->master, PRF_AUTOHIT)))
    {
      send_to_char(ch, "Hit who?\r\n");
      return;
    }
    else
    {
      // auto hit is enabled.  We're going to try to attack the first mob we can see in the room.
      if (IN_ROOM(ch) == NOWHERE)
      {
        return;
      }
      for (mob = world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room)
      {
        if (!IS_NPC(mob))
          continue;
        if (AFF_FLAGGED(mob, AFF_CHARM))
          continue;
        if (mob->master && mob->master == ch)
          continue;
        if (!CAN_SEE(ch, mob))
          continue;

        // ok we found one
        found = true;
        snprintf(mob_keys, sizeof(mob_keys), "%s", (mob)->player.name);
        for (i = 0; i < strlen(mob_keys); i++)
          if (mob_keys[i] == ' ')
            mob_keys[i] = '-';
        do_hit(ch, strdup(mob_keys), cmd, subcmd);
        
        // reach attacks get extra attack when combat starts
        if (has_reach(ch))
        {
          send_to_char(ch, "You gain an extra attack because of having long reach.\r\n");
          hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
        }
        
        return;
      }
      if (!found)
      {
        send_to_char(ch, "There are no eligible mobs here. Please specify your target instead.\r\n");
        return;
      }
    }
  }

  /* grab our target */
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "They don't seem to be here.\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "You hit yourself...OUCH!.\r\n");
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
    return;
  }

  if (FIGHTING(ch) == vict)
  {
    send_to_char(ch, "You are already fighting that one!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
  {
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  /* PVP CHECK - prevent attacking players without mutual PVP consent */
  if (!IS_NPC(vict) || (IS_NPC(vict) && vict->master && !IS_NPC(vict->master)))
  {
    if (!pvp_ok(ch, vict, true))
      return;
  }

  /* PKILL */
  if (!CONFIG_PK_ALLOWED && !IS_NPC(vict) && !IS_NPC(ch))
    check_killer(ch, vict);

  /* not yet engaged */
  if (!FIGHTING(ch) && !char_has_mud_event(ch, eCOMBAT_ROUND))
  {

    /* INITIATIVE */
    chInitiative = roll_initiative(ch);
    victInitiative = roll_initiative(vict);

    if (chInitiative >= victInitiative || GET_POS(vict) < POS_FIGHTING || !CAN_SEE(vict, ch))
    {

      /* ch is taking an action so loses the Flat-footed flag */
      if (AFF_FLAGGED(ch, AFF_FLAT_FOOTED)) REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLAT_FOOTED);

      hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE); /* ch first */

      if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_IMPROVED_INITIATIVE) && GET_POS(vict) > POS_DEAD)
      {
        send_to_char(vict, "\tYYour superior initiative grants another attack!\tn\r\n");
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }

      // reach attacks get extra attack when combat starts
      if (has_reach(ch))
      {
        send_to_char(ch, "You gain an extra attack because of having long reach.\r\n");
        hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    }
    else
    { /* ch lost initiative */

      /* this is just to avoid silly messages in peace rooms -zusuk */
      if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !ROOM_FLAGGED(IN_ROOM(vict), ROOM_PEACEFUL))
      {

        if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_CONDENSED))
        {
        }
        else
        {
          send_to_char(vict, "\tYYour superior initiative grants the first strike!\tn\r\n");
        }

        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_CONDENSED))
        {
        }
        else
        {
          send_to_char(ch,
                       "\tyYour opponents superior \tYinitiative\ty grants the first strike!\tn\r\n");
        }
      }

      /* vict is taking an action so loses the Flat-footed flag */
      if (AFF_FLAGGED(vict, AFF_FLAT_FOOTED))
        REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_FLAT_FOOTED);

      hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE); // victim is first
      update_pos(ch);

      if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_IMPROVED_INITIATIVE) &&
          GET_POS(ch) > POS_DEAD)
      {
        send_to_char(vict, "\tYYour superior initiative grants another attack!\tn\r\n");
        hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    } /* END INITIATIVE */
    /* END not-fighting scenario */
  }
  else
  { /* fighting, so trying to switch opponents */

    if (GET_POS(ch) <= POS_SITTING)
    {
      send_to_char(ch, "You are in no position to switch opponents!\r\n");
      return;
    }

    /* don't forget to remove the fight event! */
    if (char_has_mud_event(ch, eCOMBAT_ROUND))
    {
      event_cancel_specific(ch, eCOMBAT_ROUND);
    }

    send_to_char(ch, "You switch opponents!\r\n");
    act("$n switches opponents!", FALSE, ch, 0, vict, TO_ROOM);

    USE_STANDARD_ACTION(ch);
    stop_fighting(ch);
    hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

    /* everyone gets a free shot at you unless you make a acrobatics check 15 is DC */
    if (FIGHTING(ch) && FIGHTING(vict))
    {
      if (!skill_check(ch, ABILITY_ACROBATICS, 15))
        attacks_of_opportunity(ch, 0);
    }
  }
}

ACMD(do_kill)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_CAN_FIGHT();

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Kill who?\r\n");
  }
  else
  { /* we have an argument */
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "They aren't here.\r\n");
      return;
    }
    else if (ch == vict)
    {
      send_to_char(ch, "Your mother would be so sad.. :(\r\n");
      return;
    }
    else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
             ch->next_in_room != vict && vict->next_in_room != ch)
    {
      send_to_char(ch, "You simply can't reach that far.\r\n");
      return;
    }
    else if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_PEACEFUL) && GET_LEVEL(ch) < LVL_IMPL)
    {
      send_to_char(ch, "Targets room just has such a peaceful, easy feeling...\r\n");
      return;
    }
    else if (GET_LEVEL(ch) <= GET_LEVEL(vict) ||
             (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOHASSLE)))
    {
      do_hit(ch, argument, cmd, subcmd);
      return;
    }
    else if (GET_LEVEL(ch) < LVL_IMMORT || IS_NPC(ch) ||
             !PRF_FLAGGED(ch, PRF_NOHASSLE))
    {
      do_hit(ch, argument, cmd, subcmd);
      return;
    }
    else
    { /* should be higher level staff against someone with nohas off */

      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);

      raw_kill(vict, ch);

      return;
    }
  }
}

ACMDCHECK(can_backstab)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SNEAK_ATTACK, "You have no idea how to do that "
                                              "(you need at least 1 rank of the sneak attack feat to perform a "
                                              "backstab).\r\n");

  if (GET_RACE(ch) == RACE_TRELUX)
    ;
  else if (!GET_EQ(ch, WEAR_WIELD_1) && !GET_EQ(ch, WEAR_WIELD_OFFHAND) && !GET_EQ(ch, WEAR_WIELD_2H))
  {
    ACMD_ERRORMSG("You need to wield a weapon to make it a success.\r\n");
    return CANT_CMD_TEMP;
  }
  return CAN_CMD;
}

ACMD(do_backstab)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_backstab);

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You are too busy to attempt a backstab!\r\n");
    return;
  }

  one_argument(argument, buf, sizeof(buf));
  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "Backstab who?\r\n");
    return;
  }
  if (vict == ch)
  {
    send_to_char(ch, "How can you sneak up on yourself?\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  if (FIGHTING(vict))
  {
    send_to_char(ch, "You can't backstab a fighting person -- they're too alert!\r\n");
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict))
  {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    return;
  }
  if (AFF_FLAGGED(vict, AFF_AWARE) && AWAKE(vict))
  {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    return;
  }

  /* PVP CHECK - prevent backstabbing players without mutual PVP consent */
  if (!IS_NPC(vict) || (IS_NPC(vict) && vict->master && !IS_NPC(vict->master)))
  {
    if (!pvp_ok(ch, vict, true))
      return;
  }

  perform_backstab(ch, vict);
}

/* set this up for lots of redundancy checking due to really annoying crashs issue */
bool pet_order_check(struct char_data *ch, struct char_data *vict)
{

  if (ch && vict && IS_NPC(vict) && vict->master && vict->master == ch && AFF_FLAGGED(vict, AFF_CHARM) &&
      GET_HIT(vict) >= -9 && GET_POS(vict) > POS_MORTALLYW && IN_ROOM(ch) != NOWHERE && IN_ROOM(vict) != NOWHERE)
  {
    return TRUE;
  }

  return FALSE;
}

/* pet order command */
ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH] = {'\0'}, message[MAX_INPUT_LENGTH] = {'\0'};
  bool found = FALSE;
  struct char_data *vict = NULL, *next_vict = NULL;

  half_chop_c(argument, name, sizeof(name), message, sizeof(message));

  if (!*name || !*message)
    send_to_char(ch, "Order who to do what?\r\n");
  else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM)) && !is_abbrev(name, "followers"))
    send_to_char(ch, "That person isn't here.\r\n");
  else if (ch == vict)
    send_to_char(ch, "Why order yourself?\r\n");
  else
  {
    if (AFF_FLAGGED(ch, AFF_CHARM))
    {
      send_to_char(ch, "Your superior would not approve of you giving orders.\r\n");
      return;
    }
    if (vict && ch)
    {
      char buf[MAX_STRING_LENGTH] = {'\0'};

      snprintf(buf, sizeof(buf), "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
        act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else
      {
        send_to_char(ch, "%s", CONFIG_OK);
        command_interpreter(vict, message);
      }

      /* use a move action here -zusuk */
      USE_SWIFT_ACTION(ch);
    }

    else if (ch) /* This is order "followers" */
    {
      char buf[MAX_STRING_LENGTH] = {'\0'};
      struct list_data *room_list = NULL;

      snprintf(buf, sizeof(buf), "$n commands, '%s'.", message);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      /* When using a list, we have to make sure to allocate the list as it
       * uses dynamic memory */
      room_list = create_list();

      /* first build our list using a lot of silly checks due to crash issues */
      for (vict = world[IN_ROOM(ch)].people; vict; vict = next_vict)
      {
        next_vict = vict->next_in_room;

        if (pet_order_check(ch, vict))
        {
          add_to_list(vict, room_list);
        }
      }

      /* If our list is empty or has "0" entries, we free it from memory and
       * bail from this function */
      if (room_list->iSize == 0)
      {
        free_list(room_list);
        send_to_char(ch, "Nobody here is a loyal subject of yours!\r\n");
        return;
      }

      /* resetting the variable, really isn't actually necessary :) */
      vict = NULL;

      /* Beginner's Note: Reset simple_list iterator before use to prevent
       * cross-contamination from previous iterations. Without this reset,
       * if simple_list was used elsewhere and not completed, it would
       * continue from where it left off instead of starting fresh. */
      simple_list(NULL);
      
      /* SHOULD have a clean nice list, now lets loop through it with redundancy
         due to our silly crash issues from earlier */
      while ((vict = (struct char_data *)simple_list(room_list)) != NULL)
      {
        if (pet_order_check(ch, vict))
        {
          found = TRUE;
          /* here is what we came here to accomplish... */
          command_interpreter(vict, message);
        }
      }

      /* made it! */
      if (found)
      {
        USE_SWIFT_ACTION(ch);
        send_to_char(ch, "%s", CONFIG_OK);
      }
      else
      {
        /* it shouldn't be possible to get here, but regardless... */
        send_to_char(ch, "Nobody here is a loyal subject of yours!\r\n");
      }

      /* Now that our order is done, let's free out list */
      if (room_list)
        free_list(room_list);
    } /* end order all followers */

  } /* end order */

  /* all done! */
}

ACMD(do_flee)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int i;

  if (GET_POS(ch) < POS_FIGHTING || GET_HIT(ch) <= 0)
  {
    send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
    return;
  }

  if (in_encounter_room(ch))
  {
    if (!can_flee_speed(ch))
    {
      send_to_char(ch, "You are not fast enough to flee this encounter.\r\n");
      return;
    }
  }

  if (argument)
    one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    perform_flee(ch);
  }
  else if (*arg && !IS_NPC(ch) && !HAS_FEAT(ch, FEAT_SPRING_ATTACK) && !get_perk_rank(ch, PERK_FIGHTER_SPRING_ATTACK, CLASS_WARRIOR) && !HAS_FEAT(ch, FEAT_NIMBLE_ESCAPE))
  {
    send_to_char(ch, "You don't have the option to choose which way to flee, and flee randomly!\r\n");
    perform_flee(ch);
  }
  else
  { // there is an argument, check if its valid
    if (!HAS_FEAT(ch, FEAT_SPRING_ATTACK) && !get_perk_rank(ch, PERK_FIGHTER_SPRING_ATTACK, CLASS_WARRIOR))
    {
      send_to_char(ch, "You don't have the option to choose which way to flee!\r\n");
      return;
    }
    // actually direction?
    if ((i = search_block(arg, dirs, FALSE)) >= 0)
    {
      if (CAN_GO(ch, i))
      {
        if (do_simple_move(ch, i, 3))
        {
          send_to_char(ch, "You make a tactical retreat from battle!\r\n");
          act("$n makes a tactical retreat from the battle!",
              TRUE, ch, 0, 0, TO_ROOM);
          USE_MOVE_ACTION(ch);
        }
        else
        {
          send_to_char(ch, "You can't escape that direction!\r\n");
          return;
        }
      }
      else
      {
        send_to_char(ch, "You can't escape that direction!\r\n");
        return;
      }
    }
    else
    {
      send_to_char(ch, "That isn't a valid direction!\r\n");
      return;
    }
  }
}

ACMD(do_disengage)
{
  struct char_data *vict;

  if (!FIGHTING(ch))
  {
    send_to_char(ch, "You aren't even fighting anyone, calm down.\r\n");
    return;
  }
  else if (GET_POS(ch) < POS_STANDING)
  {
    send_to_char(ch, "Maybe you should get on your feet first.\r\n");
    return;
  }
  else
  {
    vict = FIGHTING(ch);
    if (FIGHTING(vict) == ch)
    {
      send_to_char(ch, "You are too busy fighting for your life!\r\n");
      return;
    }

    /* don't forget to remove the fight event! */
    if (char_has_mud_event(ch, eCOMBAT_ROUND))
    {
      event_cancel_specific(ch, eCOMBAT_ROUND);
    }

    USE_MOVE_ACTION(ch);
    stop_fighting(ch);
    send_to_char(ch, "You disengage from the fight.\r\n");
    act("$n disengages from the fight.", FALSE, ch, 0, 0, TO_ROOM);
  }
}

/* taunt engine */
int perform_taunt(struct char_data *ch, struct char_data *vict)
{
  int attempt = d20(ch), resist = 10;
  int success = 0;

  /* we started with base roll and defense in the variable declaration */
  if (!IS_NPC(ch))
  {
    if (affected_by_spell(ch, SPELL_HONEYED_TONGUE))
    {
      attempt = MAX(attempt, d20(ch));
    }
    attempt += compute_ability(ch, ABILITY_DIPLOMACY);
    if (HAS_FEAT(ch, FEAT_KENDER_TAUNT)) attempt += 4;
  }
  else
  {
    attempt += GET_LEVEL(ch);
  }
  attempt += GET_LEVEL(ch);
  if (!IS_NPC(vict))
    resist += compute_ability(vict, ABILITY_CONCENTRATION);
  else
    resist += GET_LEVEL(vict);

  /* Should last one round, plus one second for every point over the resist */
  if (attempt >= resist)
  {
    send_to_char(ch, "You taunt your opponent!\r\n");
    act("You are \tRtaunted\tn by $N!", FALSE, vict, 0, ch, TO_CHAR);
    act("$n \tWtaunts\tn $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    attach_mud_event(new_mud_event(eTAUNTED, vict, NULL), (attempt - resist + 6) * PASSES_PER_SEC);
    success = 1;
  }
  else
  {
    send_to_char(ch, "You fail to taunt your opponent!\r\n");
    act("$N fails to \tRtaunt\tn you...", FALSE, vict, 0, ch, TO_CHAR);
    act("$n fails to \tWtaunt\tn $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  if (!FIGHTING(vict))
    hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

  if (HAS_FEAT(ch, FEAT_KENDER_TAUNT))
  {
    USE_SWIFT_ACTION(ch);
  }
  else if (HAS_FEAT(ch, FEAT_IMPROVED_TAUNTING))
  {
    USE_MOVE_ACTION(ch);
  }
  else
  {
    USE_STANDARD_ACTION(ch);
  }

  return success;
}

ACMDCHECK(can_taunt)
{
  ACMDCHECK_PERMFAIL_IF(!GET_ABILITY(ch, ABILITY_DIPLOMACY), "You have no idea how (requires diplomacy).\r\n");
  return CAN_CMD;
}

ACMD(do_taunt)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();

  one_argument(argument, arg, sizeof(arg));

  PREREQ_NOT_PEACEFUL_ROOM();
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Taunt who?\r\n");
      return;
    }
  }
  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOKILL))
  {
    send_to_char(ch, "This mob is protected.\r\n");
    return;
  }
  if (!is_mission_mob(ch, vict))
  {
    send_to_char(ch, "This mob cannot be attacked by you.\r\n");
    return;
  }
  if (HAS_FEAT(vict, FEAT_COWARDLY))
  {
    act("$n is too cowardly to be goaded by your taunts.", FALSE, vict, 0, ch, TO_VICT);
    return;
  }
  if (char_has_mud_event(vict, eTAUNTED))
  {
    send_to_char(ch, "Your target is already taunted...\r\n");
    return;
  }

  perform_taunt(ch, vict);
}

int perform_intimidate(struct char_data *ch, struct char_data *vict)
{
  int success = 0;
  int attempt = d20(ch), resist = 10;

  /* we started with base roll and defense in the variable declaration */
  if (!IS_NPC(vict))
    attempt += compute_ability(ch, ABILITY_INTIMIDATE);
  else
    attempt += GET_LEVEL(vict);
  if (!IS_NPC(vict))
    resist += compute_ability(vict, ABILITY_CONCENTRATION);
  else
    resist += (GET_LEVEL(vict) / 2);

  if (HAS_FEAT(vict, FEAT_DEMORALIZE))
    resist += 2;

  if (HAS_FEAT(vict, FEAT_COWARDLY))
    resist -= 4;

  /* Should last one round, plus one second for every point over the resist */
  if (attempt >= resist)
  {
    send_to_char(ch, "You intimidate your opponent!\r\n");
    act("You are \tRintimidated\tn by $N!", FALSE, vict, 0, ch, TO_CHAR);
    act("$n \tWintimidates\tn $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    attach_mud_event(new_mud_event(eINTIMIDATED, vict, NULL), (attempt - resist + 6) * PASSES_PER_SEC);
    success = 1;
  }
  else
  {
    send_to_char(ch, "You fail to intimidate your opponent!\r\n");
    act("$N fails to \tRintimidate\tn you...", FALSE, vict, 0, ch, TO_CHAR);
    act("$n fails to \tWintimidate\tn $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  if (!FIGHTING(vict))
    hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

  if (HAS_FEAT(ch, FEAT_IMPROVED_INTIMIDATION) && HAS_FEAT(ch, FEAT_DEMORALIZE))
  {
    USE_SWIFT_ACTION(ch);
  }
  if (HAS_FEAT(ch, FEAT_IMPROVED_INTIMIDATION) || HAS_FEAT(ch, FEAT_DEMORALIZE))
  {
    USE_MOVE_ACTION(ch);
  }
  else
  {
    USE_STANDARD_ACTION(ch);
  }

  return success;
}

ACMDCHECK(can_arrowswarm)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SWARM_OF_ARROWS, "You don't know how to do this!\r\n");

  /* ranged attack requirement */
  ACMDCHECK_TEMPFAIL_IF(!can_fire_ammo(ch, TRUE),
                        "You have to be using a ranged weapon with ammo ready to "
                        "fire in your ammo pouch to do this!\r\n");
  return CAN_CMD;
}

ACMDCHECK(can_manyshot)
{
  /* Allow if player has the ranger perk or the Manyshot feat */
  if (!has_perk(ch, PERK_RANGER_MANYSHOT) && !HAS_FEAT(ch, FEAT_MANYSHOT))
  {
    ACMD_ERRORMSG("You don't know how to do this!\r\n");
    return CANT_CMD_PERM;
  }

  /* cooldown check */
  ACMDCHECK_TEMPFAIL_IF(char_has_mud_event(ch, eMANYSHOT), "You must wait longer before you can use manyshot again.\r\n");

  /* ranged attack requirement */
  ACMDCHECK_TEMPFAIL_IF(!can_fire_ammo(ch, TRUE),
                        "You have to be using a ranged weapon with ammo ready to "
                        "fire in your ammo pouch to do this!\r\n");

  return CAN_CMD;
}

ACMD(do_manyshot)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_manyshot);
  PREREQ_NOT_PEACEFUL_ROOM();

  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }

  if (!vict)
  {
    send_to_char(ch, "Use manyshot on who?\r\n");
    return;
  }

  /* Point Blank restriction: too close without Point Blank Shot feat */
  if (is_tanking(ch) && !IS_NPC(ch) && !HAS_FEAT(ch, FEAT_POINT_BLANK_SHOT))
  {
    send_to_char(ch, "You are too close to your foe to effectively use Manyshot without Point Blank Shot!\r\n");
    return;
  }

  /* TODO: Extended range validation
   * For future: allow specifying distant targets (e.g., in adjacent wilderness rooms) and
   * validate maximum weapon range bands; currently restricted to same-room target.
   */

  act("$n unleashes a rapid volley of arrows!", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "You unleash a rapid volley of arrows!\r\n");

  /* Fire three rapid shots */
  int shots = 0;
  for (shots = 0; shots < 3; shots++)
  {
    if (!can_fire_ammo(ch, TRUE))
      break;
    hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_RANGED);
  }

  /* Start 2-minute cooldown */
  attach_mud_event(new_mud_event(eMANYSHOT, ch, NULL), 120 * PASSES_PER_SEC);

  /* Consume a standard action */
  USE_STANDARD_ACTION(ch);
}

/* Callback for arrow swarm AoE */
static int arrowswarm_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  /* ammo check! */
  if (can_fire_ammo(ch, TRUE))
  {
    /* FIRE! */
    hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_RANGED);
    return 1;
  }
  return 0;
}

ACMD(do_arrowswarm)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_arrowswarm);
  PREREQ_NOT_PEACEFUL_ROOM();
  PREREQ_NOT_SINGLEFILE_ROOM();
  PREREQ_HAS_USES(FEAT_SWARM_OF_ARROWS, "You must recover before you can use another death arrow.\r\n");

  send_to_char(ch, "You open up a barrage of fire!\r\n");
  act("$n opens up a barrage of fire!", FALSE, ch, 0, 0, TO_ROOM);

  aoe_effect(ch, -1, arrowswarm_callback, NULL);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SWARM_OF_ARROWS);

  USE_STANDARD_ACTION(ch);
}

ACMDCHECK(can_intimidate)
{
  ACMDCHECK_PERMFAIL_IF(!GET_ABILITY(ch, ABILITY_INTIMIDATE), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_intimidate)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();
  PREREQ_NOT_PEACEFUL_ROOM();

  one_argument(argument, arg, sizeof(arg));

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Intimidate who?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOKILL))
  {
    send_to_char(ch, "This mob is protected.\r\n");
    return;
  }
  if (!is_mission_mob(ch, vict))
  {
    send_to_char(ch, "This mob cannot be attacked by you.\r\n");
    return;
  }
  if (char_has_mud_event(vict, eINTIMIDATED))
  {
    send_to_char(ch, "Your target is already intimidated...\r\n");
    return;
  }

  perform_intimidate(ch, vict);
}

ACMDCHECK(can_eldritch_blast)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_ELDRITCH_BLAST, "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_blast)
{
  struct char_data *vict = NULL, *tch = NULL;
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  room_rnum room = NOWHERE;
  int direction = -1, original_loc = NOWHERE;

  PREREQ_NOT_NPC();
  PREREQ_NOT_PEACEFUL_ROOM();
  PREREQ_CHECK(can_eldritch_blast);

  if (FIGHTING(ch) && GET_ELDRITCH_SHAPE(ch) == WARLOCK_ELDRITCH_SPEAR)
  {
    send_to_char(ch, "You are too busy fighting to try and fire right now!\r\n");
    return;
  }
  else if (GET_ELDRITCH_SHAPE(ch) == WARLOCK_HIDEOUS_BLOW)
  {
    send_to_char(ch, "Just hit them!\r\n");
    BLASTING(ch) = TRUE;
    return;
  }

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  /* no 2nd argument?  target room has to be same room */
  if (!*arg2)
  {
    room = IN_ROOM(ch);
  }
  else
  {
    if (!IS_NPC(ch) && GET_ELDRITCH_SHAPE(ch) != WARLOCK_ELDRITCH_SPEAR)
    {
      send_to_char(ch, "You need the 'eldritch spear' shape to shoot outside of your"
                       " immediate area!\r\n");
      return;
    }

    /* try to find target room */
    direction = search_block(arg2, dirs, FALSE);
    if (direction < 0)
    {
      send_to_char(ch, "That is not a direction!\r\n");
      return;
    }
    if (!CAN_GO(ch, direction))
    {
      send_to_char(ch, "You can't fire in that direction!\r\n");
      return;
    }
    room = EXIT(ch, direction)->to_room;
  }

  /* since we could possible no longer be in room, check if combat is ok
   in new room */
  if (ROOM_FLAGGED(room, ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  /* no arguments?  no go! */
  if (!*arg1)
  {
    send_to_char(ch, "You need to select a target!\r\n");
    return;
  }

  /* a location has been found. */
  original_loc = IN_ROOM(ch);
  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(room), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[room].coords[0];
    Y_LOC(ch) = world[room].coords[1];
  }

  char_to_room(ch, room);
  vict = get_char_room_vis(ch, arg1, NULL);

  /* check if the char is still there */
  if (IN_ROOM(ch) == room)
  {
    char_from_room(ch);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(original_loc), ZONE_WILDERNESS))
    {
      X_LOC(ch) = world[original_loc].coords[0];
      Y_LOC(ch) = world[original_loc].coords[1];
    }

    char_to_room(ch, original_loc);
  }

  if (!vict)
  {
    send_to_char(ch, "Fire at who?\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  /* if target is group member, we presume you meant to assist */
  if (GROUP(ch) && room == IN_ROOM(ch))
  {
    /* Beginner's Note: Reset simple_list iterator before use to prevent
     * cross-contamination from previous iterations. Without this reset,
     * if simple_list was used elsewhere and not completed, it would
     * continue from where it left off instead of starting fresh. */
    simple_list(NULL);
    
    while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) !=
           NULL)
    {
      if (IN_ROOM(tch) != IN_ROOM(vict))
        continue;
      if (vict == tch)
      {
        vict = FIGHTING(vict);
        break;
      }
    }
  }

  /* maybe its your pet?  so assist */
  if (vict && IS_PET(vict) && vict->master == ch && room == IN_ROOM(ch))
    vict = FIGHTING(vict);

  if (ch && vict && IN_ROOM(ch) != IN_ROOM(vict))
  {
    cast_spell(ch, vict, NULL, WARLOCK_ELDRITCH_BLAST, 0);
    /* don't forget to remove the fight event! */
    if (char_has_mud_event(ch, eCOMBAT_ROUND))
    {
      event_cancel_specific(ch, eCOMBAT_ROUND);
    }

    stop_fighting(ch);
    USE_STANDARD_ACTION(ch);
  }
  else
  {
    cast_spell(ch, vict, NULL, WARLOCK_ELDRITCH_BLAST, 0);
    BLASTING(ch) = TRUE;
  }
}

ACMDCHECK(can_dazzling_display)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_DAZZLING_DISPLAY, "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_dazzling_display)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_dazzling_display);

  if (!FIGHTING(ch))
  {
    send_to_char(ch, "You can only use this ability in combat.\r\n");
    return;
  }

  if (!is_action_available(ch, atSTANDARD, TRUE))
  {
    return;
  }

  act ("You make an intimidating display of force with your dazzling display of weapons.", false, ch, 0, ch, TO_CHAR);
  struct char_data *tch = NULL, *next_tch = NULL;
  const int skill_lvl = compute_ability(ch, ABILITY_INTIMIDATE);
  const int challenge = 10 + skill_lvl;
  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
  {
    next_tch = tch->next_in_room;
    if (AFF_FLAGGED(ch, AFF_DAZZLED))
      continue;
    if (!aoeOK(ch, tch, ABILITY_DAZZLING_DISPLAY))
      continue;
    if (is_immune_mind_affecting(ch, tch, 0))
      continue;
    int roll = compute_mag_saves(tch, SAVING_WILL, 0) + d20(tch) + GET_LEVEL(tch);
    if (roll > challenge)
      continue;
    struct affected_type af;
    new_affect(&af);
    af.spell = ABILITY_DAZZLING_DISPLAY;
    af.location = APPLY_SPECIAL;
    af.modifier = 0;
    af.duration = 1 + ((challenge - roll) / 5);
    SET_BIT_AR(af.bitvector, AFF_DAZZLED);
    affect_to_char(ch, &af);
    act("You are dazzled by $n's fearsome weapon display.", false, ch, 0, tch, TO_VICT);
    act("$N is dazzled by your fearsome weapon display.", false, ch, 0, tch, TO_CHAR);
    act("$N is dazzled by $n's fearsome weapon display.", false, ch, 0, tch, TO_NOTVICT);
  }

  USE_FULL_ROUND_ACTION(ch);
}

ACMDCHECK(can_tabaxi_claw_attack)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_TABAXI_CATS_CLAWS, "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_tabaxi_claw_attack)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_tabaxi_claw_attack);
  PREREQ_HAS_USES(FEAT_TABAXI_CATS_CLAWS, "You must wait to recover your tabaxi cats claw attacks.\r\n");

  if (!FIGHTING(ch))
  {
    send_to_char(ch, "You can only use this ability in combat.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    return;
  }

  send_to_char(ch, "Your bring to bear your long, feline claws on your opponent.\r\n");
  hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_SLICE, 0, ATTACK_TYPE_PRIMARY);
  hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_SLICE, 0, ATTACK_TYPE_PRIMARY);

  USE_SWIFT_ACTION(ch);
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_TABAXI_CATS_CLAWS);
}

ACMD(do_minotaur_gore)
{
  if (!HAS_FEAT(ch, FEAT_MINOTAUR_GORE))
  {
    send_to_char(ch, "You must be a minotaur to gore.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    return;
  }

  if (!FIGHTING(ch))
  {
    send_to_char(ch, "You can only gore when in combat.\r\n");
    return;
  }

  struct char_data *vict = FIGHTING(ch);

  act("You rush forward to gore $N", true, ch, 0, vict, TO_CHAR);
  act("$n rushes forward to gore $N.", TRUE, ch, 0, vict, TO_VICT);
  act("$n rushes forward to gore You.", TRUE, ch, 0, vict, TO_NOTVICT);

  if (combat_maneuver_check(ch, vict, COMBAT_MANEUVER_TYPE_GORE, 0) > 0)
  {
    damage(ch, vict, dice(1, 6) + GET_STR_BONUS(ch), SKILL_GORE, DAM_PUNCTURE, FALSE);

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_PUNCTURE);
  }
  else
    damage(ch, vict, 0, SKILL_GORE, DAM_PUNCTURE, FALSE);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_bite_attack)
{
  if (!has_bite_attack(ch))
  {
    send_to_char(ch, "You do not have a bite attack.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    return;
  }

  if (!FIGHTING(ch))
  {
    send_to_char(ch, "You can only bite when in combat.\r\n");
    return;
  }

  struct char_data *vict = FIGHTING(ch);

  act("You snap at $N with your vicious jaws.", true, ch, 0, vict, TO_CHAR);
  act("$n snaps at $N with $s vicious jaws.", TRUE, ch, 0, vict, TO_VICT);
  act("$n snaps at You with $s vicious jaws.", TRUE, ch, 0, vict, TO_NOTVICT);

  if (combat_maneuver_check(ch, vict, COMBAT_MANEUVER_TYPE_BITE, 0) > 0)
  {
    damage(ch, vict, dice(1, 4) + GET_STR_BONUS(ch), SKILL_BITE, DAM_PUNCTURE, FALSE);

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_PUNCTURE);
  }
  else
    damage(ch, vict, 0, SKILL_BITE, DAM_PUNCTURE, FALSE);

  USE_SWIFT_ACTION(ch);
}

/* do_frightful - Perform an AoE attack that terrifies the victims, causign them to flee.
 * Currently this is limited to dragons, but really it should be doable by any fear-inspiring
 * creature.  Don't tell me the tarrasque isn't scary! :)
 * This ability SHOULD be able to be resisted with a successful save.
 * The paladin ability AURA OF COURAGE should give a +4 bonus to all saves against fear,
 * usually will saves. */
ACMD(do_frightful)
{
  struct char_data *tch;
  int modifier = 0;

  /*  Only dragons can do this. */
  if (!IS_DRAGON(ch))
  {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  /* we have to put a restriction here for npc's, otherwise you can order
     a dragon to spam this ability -zusuk */
  if (char_has_mud_event(ch, eDRACBREATH))
  {
    send_to_char(ch, "You are too exhausted to do that!\r\n");
    act("$n tries to use a frightful presence attack, but is too exhausted!", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  PREREQ_NOT_PEACEFUL_ROOM();

  /* Check to see if the victim is affected by an AURA OF COURAGE */
  if (GROUP(ch) != NULL)
  {
    /* Beginner's Note: Reset simple_list iterator before use to prevent
     * cross-contamination from previous iterations. Without this reset,
     * if simple_list was used elsewhere and not completed, it would
     * continue from where it left off instead of starting fresh. */
    simple_list(NULL);
    
    while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      if (affected_by_aura_of_cowardice(tch))
      {
        modifier -= 4;
      }
      if (has_aura_of_courage(tch))
      {
        modifier += 4;
        /* Can only have one morale bonus. */
        break;
      }
    }
  }

  send_to_char(ch, "You ROAR!\r\n");
  act("$n lets out a mighty ROAR!", FALSE, ch, 0, 0, TO_ROOM);

  /* Data structure for frightful callback */
  struct frightful_data {
    int modifier;
    int level;
  };

  /* Callback for frightful AoE effect */
  int frightful_callback(struct char_data *ch, struct char_data *tch, void *data) {
    struct frightful_data *fright = (struct frightful_data *)data;
    
    if (is_immune_fear(ch, tch, TRUE))
      return 0;

    send_to_char(ch, "You roar at %s.\r\n", GET_NAME(tch));
    send_to_char(tch, "A mighty roar from %s is directed at you!\r\n", GET_NAME(ch));
    act("$n roars at $N!", FALSE, ch, 0, tch, TO_NOTVICT);

    /* Check the save. */
    if (has_aura_of_courage(tch) && !affected_by_aura_of_cowardice(tch))
    {
      send_to_char(tch, "You are unaffected!\r\n");
      return 0;
    }
    else if (savingthrow(ch, tch, SAVING_WILL, fright->modifier, CAST_INNATE, fright->level, NOSCHOOL))
    {
      /* Lucky you, you saved! */
      send_to_char(tch, "You stand your ground!\r\n");
      return 0;
    }
    else
    {
      /* Failed save, tough luck. */
      send_to_char(tch, "You PANIC!\r\n");
      perform_flee(tch);
      perform_flee(tch);
      return 1;
    }
  }

  struct frightful_data fright_data;
  fright_data.modifier = modifier;
  fright_data.level = GET_LEVEL(ch);

  aoe_effect(ch, -1, frightful_callback, &fright_data);

  /* 12 seconds = 2 rounds */
  attach_mud_event(new_mud_event(eDRACBREATH, ch, NULL), 12 * PASSES_PER_SEC);
}

ACMDCHECK(can_tailspikes)
{
  ACMDCHECK_PERMFAIL_IF(GET_RACE(ch) != RACE_MANTICORE && GET_DISGUISE_RACE(ch) != RACE_MANTICORE, "You have no idea how.\r\n");
  return CAN_CMD;
}

/* Callback for tailspikes AoE damage */
static int tailspikes_damage_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  damage(ch, tch, dice(3, 6) + 10, SPELL_GENERIC_AOE, DAM_PUNCTURE, FALSE);
  return 1;
}

ACMD(do_tailspikes)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_tailspikes);
  PREREQ_NOT_PEACEFUL_ROOM();

  if (!is_action_available(ch, atSWIFT, FALSE))
  {
    send_to_char(ch, "You have already used your swift action this round.\r\n");
    return;
  }

  act("You lift your tail and send a spray of tail spikes to all your foes.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n lifts $s tail and sends a spray of tail spikes to all $s foes.", FALSE, ch, 0, 0, TO_ROOM);

  aoe_effect(ch, SPELL_GENERIC_AOE, tailspikes_damage_callback, NULL);
  
  USE_SWIFT_ACTION(ch);
}

ACMDCHECK(can_dragonfear)
{
  ACMDCHECK_PERMFAIL_IF(!IS_DRAGON(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

/* Callback for dragonfear AoE effect */
static int dragonfear_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  struct affected_type af;
  int *cast_level = (int *)data;
  
  if (is_immune_fear(ch, tch, TRUE))
    return 0;
  if (is_immune_mind_affecting(ch, tch, TRUE))
    return 0;
  if (mag_resistance(ch, tch, 0))
    return 0;
  if (savingthrow(ch, tch, SAVING_WILL, affected_by_aura_of_cowardice(tch) ? -4 : 0, 
                  CAST_INNATE, *cast_level, ENCHANTMENT))
    return 0;

  /* success */
  act("You have been shaken by $n's might.", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been shaken by $n's might.", FALSE, ch, 0, tch, TO_ROOM);
  new_affect(&af);
  af.spell = SPELL_DRAGONFEAR;
  af.duration = dice(5, 6);
  SET_BIT_AR(af.bitvector, AFF_SHAKEN);
  affect_join(tch, &af, FALSE, FALSE, FALSE, FALSE);

  /* Failed save, tough luck. */
  send_to_char(tch, "You PANIC!\r\n");
  perform_flee(tch);
  perform_flee(tch);

  return 1;
}

/* the engine for dragon fear mechanic */
int perform_dragonfear(struct char_data *ch)
{
  int cast_level;

  if (!ch)
    return 0;

  act("You raise your head and let out a bone chilling roar.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n raises $s head and lets out a bone chilling roar", FALSE, ch, 0, 0, TO_ROOM);

  cast_level = CLASS_LEVEL(ch, CLASS_DRUID) + GET_SHIFTER_ABILITY_CAST_LEVEL(ch);

  return aoe_effect(ch, SPELL_DRAGONFEAR, dragonfear_callback, &cast_level);
}

/* this is another version of dragon fear (frightful above is another version) */
ACMD(do_dragonfear)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_dragonfear);
  PREREQ_NOT_PEACEFUL_ROOM();

  if (!is_action_available(ch, atSWIFT, FALSE))
  {
    send_to_char(ch, "You have already used your swift action this round.\r\n");
    return;
  }

  /* engine */
  perform_dragonfear(ch);

  USE_SWIFT_ACTION(ch);
}

ACMDCHECK(can_fear_aura)
{
  ACMDCHECK_PERMFAIL_IF(!AFF_FLAGGED(ch, AFF_FEAR_AURA), "You have no idea how.\r\n");
  return CAN_CMD;
}

/* Callback for fear aura AoE effect */
static int fear_aura_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  struct affected_type af;
  int *cast_level = (int *)data;
  
  if (is_immune_fear(ch, tch, TRUE))
    return 0;
  if (is_immune_mind_affecting(ch, tch, TRUE))
    return 0;
  if (mag_resistance(ch, tch, 0))
    return 0;
  if (savingthrow(ch, tch, SAVING_WILL, affected_by_aura_of_cowardice(tch) ? -4 : 0, 
                  CAST_INNATE, *cast_level, ENCHANTMENT))
    return 0;

  /* success */
  act("You have been shaken by $n's might.", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been shaken by $n's might.", FALSE, ch, 0, tch, TO_ROOM);
  new_affect(&af);
  af.spell = SPELL_FEAR;
  af.duration = dice(1, 4);
  SET_BIT_AR(af.bitvector, AFF_SHAKEN);
  affect_join(tch, &af, FALSE, FALSE, FALSE, FALSE);

  return 1;
}

/* the engine for dragon fear mechanic */
int perform_fear_aura(struct char_data *ch)
{
  int cast_level;

  if (!ch)
    return 0;

  act("\tCYou raise your head and let out a bone chilling roar.\tn", FALSE, ch, 0, 0, TO_CHAR);
  act("\tC$n raises $s head and lets out a bone chilling roar.\tn", FALSE, ch, 0, 0, TO_ROOM);

  cast_level = CLASS_LEVEL(ch, CLASS_DRUID) + GET_SHIFTER_ABILITY_CAST_LEVEL(ch);

  return aoe_effect(ch, SPELL_FEAR, fear_aura_callback, &cast_level);
}

/* this is another version of dragon fear (frightful above is another version) */
ACMD(do_fear_aura)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_fear_aura);
  PREREQ_NOT_PEACEFUL_ROOM();

  if (!is_action_available(ch, atSWIFT, FALSE))
  {
    send_to_char(ch, "You have already used your swift action this round.\r\n");
    return;
  }

  /* engine */
  perform_fear_aura(ch);

  USE_SWIFT_ACTION(ch);
}

ACMDCHECK(can_breathe)
{
  ACMDCHECK_PERMFAIL_IF(!IS_DRAGON(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_breathe)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_breathe);
  PREREQ_NOT_PEACEFUL_ROOM();

  /* we have to put a restriction here for npc's, otherwise you can order
     a dragon to spam this ability -zusuk */
  if (char_has_mud_event(ch, eDRACBREATH))
  {
    send_to_char(ch, "You are too exhausted to do that!\r\n");
    act("$n tries to use a breath attack, but is too exhausted!", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  int dam_type = DAM_FIRE;
  int spellnum = SPELL_FIRE_BREATHE;
  int cast_level = GET_LEVEL(ch);
  char buf[MEDIUM_STRING] = {'\0'};

  if (IS_MORPHED(ch))
  {
    cast_level = GET_SHIFTER_ABILITY_CAST_LEVEL(ch) + CLASS_LEVEL(ch, CLASS_DRUID);
    switch (GET_DISGUISE_RACE(ch))
    {
    case RACE_WHITE_DRAGON:
      dam_type = DAM_COLD;
      spellnum = SPELL_FROST_BREATHE;
      break;
    case RACE_BLACK_DRAGON:
      dam_type = DAM_ACID;
      spellnum = SPELL_ACID_BREATHE;
      break;
    case RACE_GREEN_DRAGON:
      dam_type = DAM_POISON;
      spellnum = SPELL_POISON_BREATHE;
      break;
    case RACE_BLUE_DRAGON:
      dam_type = DAM_ELECTRIC;
      spellnum = SPELL_LIGHTNING_BREATHE;
      break;
      // no need for red here, as defaults above are fire.
    }
  }

  send_to_char(ch, "You exhale breathing out %s!\r\n", damtypes[dam_type]);
  snprintf(buf, sizeof(buf), "$n exhales breathing %s!", damtypes[dam_type]);
  act(buf, FALSE, ch, 0, 0, TO_ROOM);

  int dam = dice(GET_LEVEL(ch), GET_LEVEL(ch) > 30 ? 14 : 6);
  if (IS_MORPHED(ch))
    dam = dice(cast_level, 6);

  /* Data structure for breath weapon callback */
  struct breath_weapon_data {
    int dam;
    int dam_type;
    int spellnum;
    bool is_morphed;
  };

  /* Callback for breath weapon AoE damage */
  int breath_weapon_callback(struct char_data *ch, struct char_data *tch, void *data) {
    struct breath_weapon_data *breath = (struct breath_weapon_data *)data;
    
    if (process_iron_golem_immunity(ch, tch, breath->dam_type, breath->dam))
      return 0;
    
    if (breath->is_morphed)
      damage(ch, tch, breath->dam, breath->spellnum, breath->dam_type, FALSE);
    else
      damage(ch, tch, breath->dam, SPELL_FIRE_BREATHE, DAM_FIRE, FALSE);
    
    return 1;
  }

  struct breath_weapon_data breath_data;
  breath_data.dam = dam;
  breath_data.dam_type = dam_type;
  breath_data.spellnum = spellnum;
  breath_data.is_morphed = IS_MORPHED(ch);

  aoe_effect(ch, SPELL_FIRE_BREATHE, breath_weapon_callback, &breath_data);

  USE_STANDARD_ACTION(ch);

  /* 12 seconds = 2 rounds */
  attach_mud_event(new_mud_event(eDRACBREATH, ch, NULL), 12 * PASSES_PER_SEC);
}

ACMDCHECK(can_poisonbreath)
{
  ACMDCHECK_PERMFAIL_IF(!IS_IRON_GOLEM(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_poisonbreath)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_poisonbreath);
  PREREQ_NOT_PEACEFUL_ROOM();

  act("You exhale, breathing out a cloud of poison!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n exhales, breathing out a cloud of poison!", FALSE, ch, 0, 0, TO_ROOM);

  call_magic(ch, NULL, NULL, SPELL_DEATHCLOUD, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  USE_STANDARD_ACTION(ch);
}

ACMDCHECK(can_pixieinvis)
{
  ACMDCHECK_PERMFAIL_IF(!IS_PIXIE(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_pixieinvis)
{
  PREREQ_CHECK(can_pixieinvis);

  act("You blink your eyes and vanish from sight!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n blinks $s eyes and vaishes from sight!", FALSE, ch, 0, 0, TO_ROOM);

  call_magic(ch, ch, NULL, SPELL_GREATER_INVIS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  USE_STANDARD_ACTION(ch);
}

ACMDCHECK(can_pixiedust)
{
  ACMDCHECK_PERMFAIL_IF(!IS_PIXIE(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_pixiedust)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_pixiedust);
  PREREQ_NOT_PEACEFUL_ROOM();

  if (!IS_NPC(ch))
  {
    if (PIXIE_DUST_USES(ch) <= 0)
    {
      if (PIXIE_DUST_TIMER(ch) <= 0)
      {
        PIXIE_DUST_TIMER(ch) = 0;
        PIXIE_DUST_USES(ch) = PIXIE_DUST_USES_PER_DAY(ch);
        send_to_char(ch, "Your pixie dust uses have been refreshed to %d.\r\n", PIXIE_DUST_USES_PER_DAY(ch));
      }
      else
      {
        send_to_char(ch, "You don't have any pixie dust uses left.\r\n");
        return;
      }
    }
  }

  struct char_data *vict = NULL;
  char arg1[MEDIUM_STRING] = {'\0'}, arg2[MEDIUM_STRING] = {'\0'};

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "You need to specify who you want to cast pixie dust upon.\r\n");
    return;
  }

  /* find the victim */
  vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM);

  if (!vict)
  {
    send_to_char(ch, "There is no one here by that description.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "You need to specify what effect you'd like your pixie dust to take:\r\n"
                     "pixiedust (target) (sleep|charm|confuse|dispel|entangle|shield)\r\n");
    return;
  }

  if (is_abbrev(arg2, "sleep") || is_abbrev(arg2, "charm") || is_abbrev(arg2, "confuse") || is_abbrev(arg2, "dispel") ||
      is_abbrev(arg2, "entangle") || is_abbrev(arg2, "shield"))
  {
    act("You cast a handful of pixie dust over $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n casts a handful of pixie dust over you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n casts a handful of pixie dust over $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    USE_STANDARD_ACTION(ch);
    if (!IS_NPC(ch))
    {
      if (PIXIE_DUST_TIMER(ch) <= 0)
        PIXIE_DUST_TIMER(ch) = 150;
      PIXIE_DUST_USES(ch)
      --;
    }
  }
  if (is_abbrev(arg2, "sleep"))
  {
    call_magic(ch, vict, NULL, SPELL_DEEP_SLUMBER, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "charm"))
  {
    call_magic(ch, vict, NULL, SPELL_DOMINATE_PERSON, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "confuse"))
  {
    call_magic(ch, vict, NULL, SPELL_CONFUSION, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "dispel"))
  {
    call_magic(ch, vict, NULL, SPELL_DISPEL_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "entangle"))
  {
    call_magic(ch, vict, NULL, SPELL_ENTANGLE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "shield"))
  {
    call_magic(ch, vict, NULL, SPELL_SHIELD, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else
  {
    send_to_char(ch, "You need to specify what effect you'd like your pixie dust to take:\r\n"
                     "pixiedust (target) (sleep|charm|confuse|dispel|entangle|shield)\r\n");
    return;
  }
  send_to_char(ch, "You have %d pixie dust uses left.\r\n", PIXIE_DUST_USES(ch));
}

void perform_red_dragon_magic(struct char_data *ch, const char *argument)
{
  struct char_data *vict = NULL;
  char arg1[MEDIUM_STRING] = {'\0'}, arg2[MEDIUM_STRING] = {'\0'};

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "You need to specify who you want to cast your dragon magic upon.\r\n");
    return;
  }

  /* find the victim */
  vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM);

  if (!vict)
  {
    send_to_char(ch, "There is no one here by that description.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "You need to specify what effect you'd like your dragon magic to take:\r\n"
                     "dragonmagic (target) (detect-magic|continual-flame|dispel-magic|invisibility|see-invis|magic-missile|shield|true-strike|endure-elements|haste)\r\n");
    return;
  }

  if (is_abbrev(arg2, "detect-magic") || is_abbrev(arg2, "continual-flame") || is_abbrev(arg2, "dispel-magic") || is_abbrev(arg2, "invisibility") ||
      is_abbrev(arg2, "see-invisibility") || is_abbrev(arg2, "magic-missile") || is_abbrev(arg2, "shield") || is_abbrev(arg2, "true-strike") ||
      is_abbrev(arg2, "endure-elements") || is_abbrev(arg2, "haste"))
  {
    act("You invoke your natural dragon magic on $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n invokes $s natural dragon magic on you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n invokes $s natural dragon magic on $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    USE_STANDARD_ACTION(ch);
    if (!IS_NPC(ch) && !is_abbrev(arg2, "detect-magic") && !is_abbrev(arg2, "continual-flame"))
    {
      if (DRAGON_MAGIC_TIMER(ch) <= 0)
        DRAGON_MAGIC_TIMER(ch) = 150;
      DRAGON_MAGIC_USES(ch)
      --;
    }
  }
  if (is_abbrev(arg2, "detect-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "continual-flame"))
  {
    call_magic(ch, vict, NULL, SPELL_CONTINUAL_FLAME, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "dispel-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DISPEL_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_INVISIBLE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "see-invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_INVIS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "magic-missile"))
  {
    call_magic(ch, vict, NULL, SPELL_MAGIC_MISSILE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "shield"))
  {
    call_magic(ch, vict, NULL, SPELL_SHIELD, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "true-strike"))
  {
    call_magic(ch, vict, NULL, SPELL_TRUE_STRIKE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "endure-elements"))
  {
    call_magic(ch, vict, NULL, SPELL_ENDURE_ELEMENTS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "haste"))
  {
    call_magic(ch, vict, NULL, SPELL_HASTE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else
  {
    send_to_char(ch, "You need to specify what effect you'd like your dragon magic to take:\r\n"
                     "dragonmagic (target) (detect-magic|continual-flame|dispel-magic|invisibility|see-invis|magic-missile|shield|true-strike|endure-elements|haste)\r\n");
  }
}

void perform_blue_dragon_magic(struct char_data *ch, const char *argument)
{
  struct char_data *vict = NULL;
  char arg1[MEDIUM_STRING] = {'\0'}, arg2[MEDIUM_STRING] = {'\0'};

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "You need to specify who you want to cast your dragon magic upon.\r\n");
    return;
  }

  /* find the victim */
  vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM);

  if (!vict)
  {
    send_to_char(ch, "There is no one here by that description.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "You need to specify what effect you'd like your dragon magic to take:\r\n"
                     "dragonmagic (target) (detect-magic|continual-flame|dispel-magic|invisibility|see-invis|mage-armor|shield|true-strike|endure-elements|lightning-bolt)\r\n");
    return;
  }

  if (is_abbrev(arg2, "detect-magic") || is_abbrev(arg2, "continual-flame") || is_abbrev(arg2, "dispel-magic") || is_abbrev(arg2, "invisibility") ||
      is_abbrev(arg2, "see-invisibility") || is_abbrev(arg2, "mage-armor") || is_abbrev(arg2, "shield") || is_abbrev(arg2, "true-strike") ||
      is_abbrev(arg2, "endure-elements") || is_abbrev(arg2, "lightning-bolt"))
  {
    act("You invoke your natural dragon magic on $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n invokes $s natural dragon magic on you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n invokes $s natural dragon magic on $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    USE_STANDARD_ACTION(ch);
    if (!IS_NPC(ch) && !is_abbrev(arg2, "detect-magic") && !is_abbrev(arg2, "continual-flame"))
    {
      if (DRAGON_MAGIC_TIMER(ch) <= 0)
        DRAGON_MAGIC_TIMER(ch) = 150;
      DRAGON_MAGIC_USES(ch)
      --;
    }
  }
  if (is_abbrev(arg2, "detect-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "continual-flame"))
  {
    call_magic(ch, vict, NULL, SPELL_CONTINUAL_FLAME, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "dispel-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DISPEL_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_INVISIBLE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "see-invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_INVIS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "mage-armor"))
  {
    call_magic(ch, vict, NULL, SPELL_MAGE_ARMOR, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "shield"))
  {
    call_magic(ch, vict, NULL, SPELL_SHIELD, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "true-strike"))
  {
    call_magic(ch, vict, NULL, SPELL_TRUE_STRIKE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "endure-elements"))
  {
    call_magic(ch, vict, NULL, SPELL_ENDURE_ELEMENTS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "lightning-bolt"))
  {
    call_magic(ch, vict, NULL, SPELL_LIGHTNING_BOLT, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else
  {
    send_to_char(ch, "You need to specify what effect you'd like your dragon magic to take:\r\n"
                     "dragonmagic (target) (detect-magic|continual-flame|dispel-magic|invisibility|see-invis|mage-armor|shield|true-strike|endure-elements|lightning-bolt)\r\n");
  }
}

void perform_green_dragon_magic(struct char_data *ch, const char *argument)
{
  struct char_data *vict = NULL;
  char arg1[MEDIUM_STRING] = {'\0'}, arg2[MEDIUM_STRING] = {'\0'};

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "You need to specify who you want to cast your dragon magic upon.\r\n");
    return;
  }

  /* find the victim */
  vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM);

  if (!vict)
  {
    send_to_char(ch, "There is no one here by that description.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "You need to specify what effect you'd like your dragon magic to take:\r\n"
                     "dragonmagic (target) (detect-magic|continual-flame|dispel-magic|invisibility|see-invis|mage-armor|shield|true-strike|endure-elements|mirror-image)\r\n");
    return;
  }

  if (is_abbrev(arg2, "detect-magic") || is_abbrev(arg2, "continual-flame") || is_abbrev(arg2, "dispel-magic") || is_abbrev(arg2, "invisibility") ||
      is_abbrev(arg2, "see-invisibility") || is_abbrev(arg2, "mage-armor") || is_abbrev(arg2, "shield") || is_abbrev(arg2, "true-strike") ||
      is_abbrev(arg2, "endure-elements") || is_abbrev(arg2, "mirror-image"))
  {
    act("You invoke your natural dragon magic on $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n invokes $s natural dragon magic on you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n invokes $s natural dragon magic on $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    USE_STANDARD_ACTION(ch);
    if (!IS_NPC(ch) && !is_abbrev(arg2, "detect-magic") && !is_abbrev(arg2, "continual-flame"))
    {
      if (DRAGON_MAGIC_TIMER(ch) <= 0)
        DRAGON_MAGIC_TIMER(ch) = 150;
      DRAGON_MAGIC_USES(ch)
      --;
    }
  }
  if (is_abbrev(arg2, "detect-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "continual-flame"))
  {
    call_magic(ch, vict, NULL, SPELL_CONTINUAL_FLAME, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "dispel-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DISPEL_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_INVISIBLE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "see-invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_INVIS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "mage-armor"))
  {
    call_magic(ch, vict, NULL, SPELL_MAGE_ARMOR, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "shield"))
  {
    call_magic(ch, vict, NULL, SPELL_SHIELD, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "true-strike"))
  {
    call_magic(ch, vict, NULL, SPELL_TRUE_STRIKE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "endure-elements"))
  {
    call_magic(ch, vict, NULL, SPELL_ENDURE_ELEMENTS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "mirror-image"))
  {
    call_magic(ch, vict, NULL, SPELL_MIRROR_IMAGE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else
  {
    send_to_char(ch, "You need to specify what effect you'd like your dragon magic to take:\r\n"
                     "dragonmagic (target) (detect-magic|continual-flame|dispel-magic|invisibility|see-invis|mage-armor|shield|true-strike|endure-elements|mirror-image)\r\n");
  }
}

void perform_black_dragon_magic(struct char_data *ch, const char *argument)
{
  struct char_data *vict = NULL;
  char arg1[MEDIUM_STRING] = {'\0'}, arg2[MEDIUM_STRING] = {'\0'};

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "You need to specify who you want to cast your dragon magic upon.\r\n");
    return;
  }

  /* find the victim */
  vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM);

  if (!vict)
  {
    send_to_char(ch, "There is no one here by that description.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "You need to specify what effect you'd like your dragon magic to take:\r\n"
                     "dragonmagic (target) (detect-magic|continual-flame|dispel-magic|invisibility|see-invis|stinking-cloud|shield|true-strike|endure-elements|slow)\r\n");
    return;
  }

  if (is_abbrev(arg2, "detect-magic") || is_abbrev(arg2, "continual-flame") || is_abbrev(arg2, "dispel-magic") || is_abbrev(arg2, "invisibility") ||
      is_abbrev(arg2, "see-invisibility") || is_abbrev(arg2, "stinking-cloud") || is_abbrev(arg2, "shield") || is_abbrev(arg2, "true-strike") ||
      is_abbrev(arg2, "endure-elements") || is_abbrev(arg2, "slow"))
  {
    act("You invoke your natural dragon magic on $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n invokes $s natural dragon magic on you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n invokes $s natural dragon magic on $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    USE_STANDARD_ACTION(ch);
    if (!IS_NPC(ch) && !is_abbrev(arg2, "detect-magic") && !is_abbrev(arg2, "continual-flame"))
    {
      if (DRAGON_MAGIC_TIMER(ch) <= 0)
        DRAGON_MAGIC_TIMER(ch) = 150;
      DRAGON_MAGIC_USES(ch)
      --;
    }
  }
  if (is_abbrev(arg2, "detect-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "continual-flame"))
  {
    call_magic(ch, vict, NULL, SPELL_CONTINUAL_FLAME, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "dispel-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DISPEL_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_INVISIBLE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "see-invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_INVIS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "stinking-cloud"))
  {
    call_magic(ch, vict, NULL, SPELL_STINKING_CLOUD, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "shield"))
  {
    call_magic(ch, vict, NULL, SPELL_SHIELD, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "true-strike"))
  {
    call_magic(ch, vict, NULL, SPELL_TRUE_STRIKE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "endure-elements"))
  {
    call_magic(ch, vict, NULL, SPELL_ENDURE_ELEMENTS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "slow"))
  {
    call_magic(ch, vict, NULL, SPELL_SLOW, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else
  {
    send_to_char(ch, "You need to specify what effect you'd like your dragon magic to take:\r\n"
                     "dragonmagic (target) (detect-magic|continual-flame|dispel-magic|invisibility|see-invis|stinking-cloud|shield|true-strike|endure-elements|slow)\r\n");
  }
}

void perform_white_dragon_magic(struct char_data *ch, const char *argument)
{
  struct char_data *vict = NULL;
  char arg1[MEDIUM_STRING] = {'\0'}, arg2[MEDIUM_STRING] = {'\0'};

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "You need to specify who you want to cast your dragon magic upon.\r\n");
    return;
  }

  /* find the victim */
  vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM);

  if (!vict)
  {
    send_to_char(ch, "There is no one here by that description.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "You need to specify what effect you'd like your dragon magic to take:\r\n"
                     "dragonmagic (target) (detect-magic|continual-flame|dispel-magic|invisibility|see-invis|chill-touch|shield|true-strike|endure-elements|ice-storm)\r\n");
    return;
  }

  if (is_abbrev(arg2, "detect-magic") || is_abbrev(arg2, "continual-flame") || is_abbrev(arg2, "dispel-magic") || is_abbrev(arg2, "invisibility") ||
      is_abbrev(arg2, "see-invisibility") || is_abbrev(arg2, "chill-touch") || is_abbrev(arg2, "shield") || is_abbrev(arg2, "true-strike") ||
      is_abbrev(arg2, "endure-elements") || is_abbrev(arg2, "ice-storm"))
  {
    act("You invoke your natural dragon magic on $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n invokes $s natural dragon magic on you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n invokes $s natural dragon magic on $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    USE_STANDARD_ACTION(ch);
    if (!IS_NPC(ch) && !is_abbrev(arg2, "detect-magic") && !is_abbrev(arg2, "continual-flame"))
    {
      if (DRAGON_MAGIC_TIMER(ch) <= 0)
        DRAGON_MAGIC_TIMER(ch) = 150;
      DRAGON_MAGIC_USES(ch)
      --;
    }
  }
  if (is_abbrev(arg2, "detect-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "continual-flame"))
  {
    call_magic(ch, vict, NULL, SPELL_CONTINUAL_FLAME, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "dispel-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DISPEL_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_INVISIBLE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "see-invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_INVIS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "chill-touch"))
  {
    call_magic(ch, vict, NULL, SPELL_CHILL_TOUCH, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "shield"))
  {
    call_magic(ch, vict, NULL, SPELL_SHIELD, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "true-strike"))
  {
    call_magic(ch, vict, NULL, SPELL_TRUE_STRIKE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "endure-elements"))
  {
    call_magic(ch, vict, NULL, SPELL_ENDURE_ELEMENTS, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "ice-storm"))
  {
    call_magic(ch, vict, NULL, SPELL_ICE_STORM, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else
  {
    send_to_char(ch, "You need to specify what effect you'd like your dragon magic to take:\r\n"
                     "dragonmagic (target) (detect-magic|continual-flame|dispel-magic|invisibility|see-invis|chill-touch|shield|true-strike|endure-elements|ice-storm)\r\n");
  }
}

ACMDCHECK(can_dragonmagic)
{
  ACMDCHECK_PERMFAIL_IF(!IS_DRAGON(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_dragonmagic)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_dragonmagic);
  PREREQ_NOT_PEACEFUL_ROOM();

  if (!IS_NPC(ch))
  {
    if (DRAGON_MAGIC_USES(ch) <= 0)
    {
      if (DRAGON_MAGIC_TIMER(ch) <= 0)
      {
        DRAGON_MAGIC_TIMER(ch) = 0;
        DRAGON_MAGIC_USES(ch) = DRAGON_MAGIC_USES_PER_DAY;
        send_to_char(ch, "Your dragon magic uses have been refreshed to %d.\r\n", DRAGON_MAGIC_USES_PER_DAY);
      }
      else
      {
        send_to_char(ch, "You don't have any dragon magic uses left.\r\n");
        return;
      }
    }
  }

  switch (GET_DISGUISE_RACE(ch))
  {
  case RACE_RED_DRAGON:
    perform_red_dragon_magic(ch, argument);
    break;
  case RACE_BLUE_DRAGON:
    perform_blue_dragon_magic(ch, argument);
    break;
  case RACE_GREEN_DRAGON:
    perform_green_dragon_magic(ch, argument);
    break;
  case RACE_BLACK_DRAGON:
    perform_black_dragon_magic(ch, argument);
    break;
  case RACE_WHITE_DRAGON:
    perform_white_dragon_magic(ch, argument);
    break;
  default:
    send_to_char(ch, "You are not of an eligible dragon type to be able to use dragon magic.\r\n");
    return;
  }
  send_to_char(ch, "You have %d dragon magic uses left.\r\n", DRAGON_MAGIC_USES(ch));
}

ACMDCHECK(can_efreetimagic)
{
  ACMDCHECK_PERMFAIL_IF(!IS_EFREETI(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_efreetimagic)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_efreetimagic);
  PREREQ_NOT_PEACEFUL_ROOM();

  if (!IS_NPC(ch))
  {
    if (EFREETI_MAGIC_USES(ch) <= 0)
    {
      if (EFREETI_MAGIC_TIMER(ch) <= 0)
      {
        EFREETI_MAGIC_TIMER(ch) = 0;
        EFREETI_MAGIC_USES(ch) = EFREETI_MAGIC_USES_PER_DAY;
        send_to_char(ch, "Your efreeti magic uses have been refreshed to %d.\r\n", EFREETI_MAGIC_USES_PER_DAY);
      }
      else
      {
        send_to_char(ch, "You don't have any efreeti magic uses left.\r\n");
        return;
      }
    }
  }

  struct char_data *vict = NULL;
  char arg1[MEDIUM_STRING] = {'\0'}, arg2[MEDIUM_STRING] = {'\0'};
  int i = 0;

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "You need to specify who you want to cast your efreeti magic upon.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "You need to specify what effect you'd like your efreeti magic to take:\r\n"
                     "efreetimagic (target) (detect-magic|produce-flame|scorching-ray|invisiblity|wall-of-fire|mirror-image|enlarge-person|reduce-person)\r\n");
    return;
  }

  if (is_abbrev(arg2, "wall-of-fire"))
  {
    for (i = 0; i < NUM_OF_DIRS; i++)
      if (is_abbrev(dirs[i], arg1))
        break;
    if (i >= NUM_OF_DIRS)
    {
      send_to_char(ch, "There's no exit in that direction.\r\n");
      return;
    }
    vict = ch;
  }
  else
  {
    /* find the victim */
    vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM);

    if (!vict)
    {
      send_to_char(ch, "There is no one here by that description.\r\n");
      return;
    }
  }

  if (is_abbrev(arg2, "detect-magic") || is_abbrev(arg2, "produce-flame") || is_abbrev(arg2, "scorching-ray") || is_abbrev(arg2, "invisibility") ||
      is_abbrev(arg2, "wall-of-fire") || is_abbrev(arg2, "mirror-image") || is_abbrev(arg2, "enlarge-person") || is_abbrev(arg2, "reduce-person"))
  {
    act("You invoke your natural efreeti magic on $N!", FALSE, ch, 0, vict, TO_CHAR);
    if (vict != ch)
      act("$n invokes $s natural efreeti magic on you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n invokes $s natural efreeti magic on $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    USE_STANDARD_ACTION(ch);
    if (!IS_NPC(ch) && !is_abbrev(arg2, "detect-magic") && !is_abbrev(arg2, "produce-flame") && !is_abbrev(arg2, "scorching-ray"))
    {
      if (EFREETI_MAGIC_TIMER(ch) <= 0)
        EFREETI_MAGIC_TIMER(ch) = 150;
      EFREETI_MAGIC_USES(ch)
      --;
    }
  }
  if (is_abbrev(arg2, "detect-magic"))
  {
    call_magic(ch, vict, NULL, SPELL_DETECT_MAGIC, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "produce-flame"))
  {
    call_magic(ch, vict, NULL, SPELL_PRODUCE_FLAME, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "scorching-ray"))
  {
    call_magic(ch, vict, NULL, SPELL_SCORCHING_RAY, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "invisibility"))
  {
    call_magic(ch, vict, NULL, SPELL_INVISIBLE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "wall-of-fire"))
  {
    strlcpy(cast_arg2, arg1, sizeof(cast_arg2));
    call_magic(ch, ch, NULL, SPELL_WALL_OF_FIRE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "mirror-image"))
  {
    call_magic(ch, ch, NULL, SPELL_MIRROR_IMAGE, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "enlarge-person"))
  {
    call_magic(ch, ch, NULL, SPELL_ENLARGE_PERSON, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else if (is_abbrev(arg2, "reduce-person"))
  {
    call_magic(ch, ch, NULL, SPELL_SHRINK_PERSON, 0, GET_SHIFTER_ABILITY_CAST_LEVEL(ch), CAST_INNATE);
  }
  else
  {
    send_to_char(ch, "You need to specify what effect you'd like your efreeti magic to take:\r\n"
                     "efreetimagic (target) (detect-magic|produce-flame|scorchin-ray|invisiblity|wall-of-fire|mirror-image|enlarge-person|reduce-person)\r\n");
    return;
  }
  send_to_char(ch, "You have %d efreeti magic uses left.\r\n", EFREETI_MAGIC_USES(ch));
}

ACMDCHECK(can_fey_magic)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SORCERER_BLOODLINE_FEY, "You do not have access to this ability.\r\n");
  return CAN_CMD;
}

#define FEY_MAGIC_NO_ARG "Please specify one of the following fey magic options:\r\n"                                       \
                         "laughing-touch  : sorcerer lvl 1  - causes target to lose their standard action for 1 round.\r\n" \
                         "fleeting-glance : sorcerer lvl 9  - casts greater invisibility.\r\n"                              \
                         "shadow-walk     : sorcerer lvl 20 - casts shadow walk.\r\n"                                       \
                         "\r\n"

ACMD(do_fey_magic)
{
  // laughing tough, fleeting glance, shadow walk - soul of the fey

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_fey_magic);
  PREREQ_NOT_PEACEFUL_ROOM();

  char arg1[100], arg2[100];

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "%s", FEY_MAGIC_NO_ARG);
    return;
  }

  if (is_abbrev(arg1, "laughing-tough"))
  {
    if (!IS_NPC(ch))
    {
      if (LAUGHING_TOUCH_USES(ch) <= 0)
      {
        if (LAUGHING_TOUCH_TIMER(ch) <= 0)
        {
          LAUGHING_TOUCH_TIMER(ch) = 0;
          LAUGHING_TOUCH_USES(ch) = LAUGHING_TOUCH_USES_PER_DAY(ch);
          send_to_char(ch, "Your laughing touch uses have been refreshed to %d.\r\n", LAUGHING_TOUCH_USES_PER_DAY(ch));
        }
        else
        {
          send_to_char(ch, "You don't have any laughing touch uses left.\r\n");
          return;
        }
      }
    }
    else
    {
      send_to_char(ch, "NPCs cannot use laughing touch.\r\n");
      return;
    }

    if (!*arg2)
    {
      send_to_char(ch, "You need to specify a target for your laughing touch.\r\n");
      return;
    }

    struct char_data *victim = get_char_room_vis(ch, arg2, NULL);

    if (!victim)
    {
      send_to_char(ch, "There is no one by that description here.\r\n");
      return;
    }

    if (is_player_grouped(ch, victim))
    {
      send_to_char(ch, "You probably shouldn't do that to a group member.\r\n");
      return;
    }

    act("You touch $N, who bursts into uncontrolable laughter.", false, ch, 0, victim, TO_CHAR);
    act("$n touches you causing you to burst into uncontrolable laughter.", false, ch, 0, victim, TO_VICT);
    act("$n touches $N, who bursts into uncontrolable laughter.", false, ch, 0, victim, TO_NOTVICT);

    call_magic(ch, ch, NULL, SPELL_HIDEOUS_LAUGHTER, 0, compute_arcane_level(ch), CAST_INNATE);

    USE_STANDARD_ACTION(victim);

    if (LAUGHING_TOUCH_TIMER(ch) <= 0)
      LAUGHING_TOUCH_TIMER(ch) = 150;
    LAUGHING_TOUCH_USES(ch)
    --;
    return; // end laughing touch
  }
  else if (is_abbrev(arg1, "fleeting-glance"))
  {
    if (!HAS_FEAT(ch, FEAT_FLEETING_GLANCE))
    {
      send_to_char(ch, "You need to have the sorcerer fey bloodline ability, 'fleeting glance', to use this ability.\r\n");
      return;
    }
    if (!IS_NPC(ch))
    {
      if (FLEETING_GLANCE_USES(ch) <= 0)
      {
        if (FLEETING_GLANCE_TIMER(ch) <= 0)
        {
          FLEETING_GLANCE_TIMER(ch) = 0;
          FLEETING_GLANCE_USES(ch) = FLEETING_GLANCE_USES_PER_DAY;
          send_to_char(ch, "Your fleeting glance uses have been refreshed to %d.\r\n", FLEETING_GLANCE_USES_PER_DAY);
        }
        else
        {
          send_to_char(ch, "You don't have any fleeting glance uses left.\r\n");
          return;
        }
      }
    }
    else
    {
      send_to_char(ch, "NPCs cannot use fleeting glance.\r\n");
      return;
    }

    act("You draw upon your innate fey magic.", true, ch, 0, 0, TO_CHAR);
    act("$n draws upon $s innate fey magic.", true, ch, 0, 0, TO_ROOM);

    call_magic(ch, ch, NULL, SPELL_GREATER_INVIS, 0, compute_arcane_level(ch), CAST_INNATE);

    if (FLEETING_GLANCE_TIMER(ch) <= 0)
      FLEETING_GLANCE_TIMER(ch) = 150;
    FLEETING_GLANCE_USES(ch)
    --;

    return; // end fleeting glance
  }
  else if (is_abbrev(arg1, "shadow-walk"))
  {
    if (!HAS_FEAT(ch, FEAT_SOUL_OF_THE_FEY))
    {
      send_to_char(ch, "You need the sorcerer fey bloodline ability, 'soul of the fey', to use fey magic shadow walk.\r\n");
      return;
    }
    if (!IS_NPC(ch))
    {
      if (FEY_SHADOW_WALK_USES(ch) <= 0)
      {
        if (FEY_SHADOW_WALK_TIMER(ch) <= 0)
        {
          FEY_SHADOW_WALK_TIMER(ch) = 0;
          FEY_SHADOW_WALK_USES(ch) = FEY_SHADOW_WALK_USES_PER_DAY;
          send_to_char(ch, "Your fey shadow walk uses have been refreshed to %d.\r\n", FEY_SHADOW_WALK_USES_PER_DAY);
        }
        else
        {
          send_to_char(ch, "You don't have any fey shadow walk uses left.\r\n");
          return;
        }
      }
    }
    else
    {
      send_to_char(ch, "NPCs cannot use fey shadow walk.\r\n");
      return;
    }

    act("You draw upon your innate fey magic.", true, ch, 0, 0, TO_CHAR);
    act("$n draws upon $s innate fey magic.", true, ch, 0, 0, TO_ROOM);

    call_magic(ch, ch, NULL, SPELL_SHADOW_WALK, 0, compute_arcane_level(ch), CAST_INNATE);

    if (FEY_SHADOW_WALK_TIMER(ch) <= 0)
      FEY_SHADOW_WALK_TIMER(ch) = 150;
    FEY_SHADOW_WALK_USES(ch)
    --;

    return; // end shadow walk
  }
  else
  {
    send_to_char(ch, "%s", FEY_MAGIC_NO_ARG);
    return;
  }
}

ACMDCHECK(can_grave_magic)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SORCERER_BLOODLINE_UNDEAD, "You do not have access to this ability.\r\n");
  return CAN_CMD;
}

#define GRAVE_MAGIC_NO_ARG "Please specify one of the following fey magic options:\r\n"                                         \
                           "touch            : sorcerer lvl 1  - causes target to become shaken.\r\n"                           \
                           "grasp            : sorcerer lvl 9  - deals AoE slashing damage.\r\n"                                \
                           "incorporeal-form : sorcerer lvl 15 - become incorporeal, reduced damage from physical attacks.\r\n" \
                           "\r\n"

ACMD(do_grave_magic)
{
  // laughing tough, fleeting glance, shadow walk - soul of the fey

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_grave_magic);
  PREREQ_NOT_PEACEFUL_ROOM();

  char arg1[100], arg2[100];

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "%s", GRAVE_MAGIC_NO_ARG);
    return;
  }

  if (is_abbrev(arg1, "touch"))
  {
    if (!IS_NPC(ch))
    {
      if (GRAVE_TOUCH_USES(ch) <= 0)
      {
        if (GRAVE_TOUCH_TIMER(ch) <= 0)
        {
          GRAVE_TOUCH_TIMER(ch) = 0;
          GRAVE_TOUCH_USES(ch) = GRAVE_TOUCH_USES_PER_DAY(ch);
          send_to_char(ch, "Your grave touch uses have been refreshed to %d.\r\n", GRAVE_TOUCH_USES_PER_DAY(ch));
        }
        else
        {
          send_to_char(ch, "You don't have any grave touch uses left.\r\n");
          return;
        }
      }
    }
    else
    {
      send_to_char(ch, "NPCs cannot use grave touch.\r\n");
      return;
    }

    struct char_data *victim = NULL;

    if (!*arg2)
    {
      if (FIGHTING(ch))
      {
        victim = FIGHTING(ch);
      }
      else
      {
        send_to_char(ch, "You need to specify a target for your grave touch.\r\n");
        return;
      }
    }
    else
    {
      victim = get_char_room_vis(ch, arg2, NULL);
    }

    if (!victim)
    {
      send_to_char(ch, "There is no one by that description here.\r\n");
      return;
    }

    if (is_player_grouped(ch, victim))
    {
      send_to_char(ch, "You probably shouldn't do that to a group member.\r\n");
      return;
    }

    if (is_immune_fear(ch, victim, FALSE))
    {
      act("You touch $N, who shakes off the fear affect immediately.", false, ch, 0, victim, TO_CHAR);
      act("$n touches you, but you shake off the fear affect immediately.", false, ch, 0, victim, TO_VICT);
      act("$n touches $N, who shakes off the fear affect immediately.", false, ch, 0, victim, TO_NOTVICT);
    }
    else
    {
      if (savingthrow(ch, victim, SAVING_FORT, affected_by_aura_of_cowardice(victim) ? -4 : 0, CASTING_TYPE_ARCANE, compute_arcane_level(ch), NECROMANCY))
      {
        act("You touch $N, who shakes off the fear affect immediately.", false, ch, 0, victim, TO_CHAR);
        act("$n touches you, but you shake off the fear affect immediately.", false, ch, 0, victim, TO_VICT);
        act("$n touches $N, who shakes off the fear affect immediately.", false, ch, 0, victim, TO_NOTVICT);
      }
      else
      {
        struct affected_type af;
        new_affect(&af);

        af.spell = SPELL_GRAVE_TOUCH;
        af.duration = MAX(2, CLASS_LEVEL(ch, CLASS_SORCERER) / 2);
        SET_BIT_AR(af.bitvector, AFF_SHAKEN);
        af.modifier = -1;
        af.location = APPLY_AC_NEW;

        affect_to_char(victim, &af);

        act("You touch $N, who immediately becomes quite frightened.", false, ch, 0, victim, TO_CHAR);
        act("$n touches you causing you to become quite frightened.", false, ch, 0, victim, TO_VICT);
        act("$n touches $N, who immediately becomes quite frightened.", false, ch, 0, victim, TO_NOTVICT);
      }
    }

    if (GRAVE_TOUCH_TIMER(ch) <= 0)
      GRAVE_TOUCH_TIMER(ch) = 150;
    GRAVE_TOUCH_USES(ch)
    --;
    return; // end GRAVE touch
  }
  else if (is_abbrev(arg1, "grasp"))
  {
    if (!HAS_FEAT(ch, FEAT_GRASP_OF_THE_DEAD))
    {
      send_to_char(ch, "You need the sorcerer undead bloodline ability, 'grasp of the dead', to use this ability.\r\n");
      return;
    }
    if (!IS_NPC(ch))
    {
      if (GRASP_OF_THE_DEAD_USES(ch) <= 0)
      {
        if (GRASP_OF_THE_DEAD_TIMER(ch) <= 0)
        {
          GRASP_OF_THE_DEAD_TIMER(ch) = 0;
          GRASP_OF_THE_DEAD_USES(ch) = GRASP_OF_THE_DEAD_USES_PER_DAY(ch);
          send_to_char(ch, "Your undead bloodline grasp of the dead uses have been refreshed to %d.\r\n", GRASP_OF_THE_DEAD_USES_PER_DAY(ch));
        }
        else
        {
          send_to_char(ch, "You don't have any undead bloodline grasp of the dead uses left.\r\n");
          return;
        }
      }
    }
    else
    {
      send_to_char(ch, "NPCs cannot use grasp of the dead.\r\n");
      return;
    }

    act("You draw upon your innate undead prowess.", true, ch, 0, 0, TO_CHAR);
    act("$n draws upon $s innate undead prowess.", true, ch, 0, 0, TO_ROOM);

    call_magic(ch, ch, NULL, SPELL_GRASP_OF_THE_DEAD, 0, compute_arcane_level(ch), CAST_INNATE);

    if (GRASP_OF_THE_DEAD_TIMER(ch) <= 0)
      GRASP_OF_THE_DEAD_TIMER(ch) = 150;
    GRASP_OF_THE_DEAD_USES(ch)
    --;

    return; // end grasp of the dead
  }
  else if (is_abbrev(arg1, "incorporeal-form"))
  {
    if (!HAS_FEAT(ch, FEAT_INCORPOREAL_FORM))
    {
      send_to_char(ch, "You need the sorcerer undead bloodline ability, 'incorporeal form', to use this ability.\r\n");
      return;
    }
    if (!IS_NPC(ch))
    {
      if (INCORPOREAL_FORM_USES(ch) <= 0)
      {
        if (INCORPOREAL_FORM_TIMER(ch) <= 0)
        {
          INCORPOREAL_FORM_TIMER(ch) = 0;
          INCORPOREAL_FORM_USES(ch) = INCORPOREAL_FORM_USES_PER_DAY(ch);
          send_to_char(ch, "Your undead bloodline incorporeal form uses have been refreshed to %d.\r\n", INCORPOREAL_FORM_USES_PER_DAY(ch));
        }
        else
        {
          send_to_char(ch, "You don't have any undead bloodline incorporeal form uses left.\r\n");
          return;
        }
      }
    }
    else
    {
      send_to_char(ch, "NPCs cannot use incorporeal form.\r\n");
      return;
    }

    act("You draw upon your innate undead prowess.", true, ch, 0, 0, TO_CHAR);
    act("$n draws upon $s innate undead prowess.", true, ch, 0, 0, TO_ROOM);

    struct affected_type af;
    new_affect(&af);

    af.spell = SPELL_INCORPOREAL_FORM;
    af.duration = 3;
    ;
    SET_BIT_AR(af.bitvector, AFF_IMMATERIAL);
    af.modifier = 1;
    af.location = APPLY_AC_NEW;

    affect_to_char(ch, &af);

    act("You suddenly become incorporeal, half in this plane, half in the astral plane.", false, ch, 0, 0, TO_CHAR);
    act("$n suddenly becomes incorporeal.", false, ch, 0, 0, TO_ROOM);

    if (INCORPOREAL_FORM_TIMER(ch) <= 0)
      INCORPOREAL_FORM_TIMER(ch) = 150;
    INCORPOREAL_FORM_USES(ch)
    --;

    return; // end incorporeal form
  }
  else
  {
    send_to_char(ch, "%s", GRAVE_MAGIC_NO_ARG);
    return;
  }
}

ACMDCHECK(can_sorcerer_breath_weapon)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_DRACONIC_HERITAGE_BREATHWEAPON, "You do not have access to this ability.\r\n");
  return CAN_CMD;
}

ACMD(do_sorcerer_breath_weapon)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_sorcerer_breath_weapon);

  if (!IS_NPC(ch))
  {
    PREREQ_HAS_USES(FEAT_DRACONIC_HERITAGE_BREATHWEAPON, "You must recover before you can use your draconic heritage breath weapon again.\r\n");
    send_to_char(ch, "You have %d uses remaining.\r\n", uses_remaining - 1);
  }

  PREREQ_NOT_PEACEFUL_ROOM();

  send_to_char(ch, "You exhale breathing out %s!\r\n", DRCHRT_ENERGY_TYPE(GET_BLOODLINE_SUBTYPE(ch)));
  char to_room[200];
  snprintf(to_room, sizeof(to_room), "$n exhales breathing %s!", DRCHRT_ENERGY_TYPE(GET_BLOODLINE_SUBTYPE(ch)));
  act(to_room, FALSE, ch, 0, 0, TO_ROOM);

  int damtype = draconic_heritage_energy_types[GET_BLOODLINE_SUBTYPE(ch)];
  int dam = 0;
  if (GET_LEVEL(ch) <= 15)
    dam = dice(GET_LEVEL(ch), 6);
  else
    dam = dice(GET_LEVEL(ch), 14);

  /* Data structure for sorcerer breath callback */
  struct sorcerer_breath_data {
    int dam;
    int damtype;
  };

  /* Callback for sorcerer breath weapon AoE damage */
  int sorcerer_breath_callback(struct char_data *ch, struct char_data *tch, void *data) {
    struct sorcerer_breath_data *breath = (struct sorcerer_breath_data *)data;
    
    if (process_iron_golem_immunity(ch, tch, breath->damtype, breath->dam))
      return 0;
    
    damage(ch, tch, breath->dam, SPELL_DRACONIC_BLOODLINE_BREATHWEAPON, breath->damtype, FALSE);
    return 1;
  }

  struct sorcerer_breath_data breath_data;
  breath_data.dam = dam;
  breath_data.damtype = damtype;

  aoe_effect(ch, SPELL_DRACONIC_BLOODLINE_BREATHWEAPON, sorcerer_breath_callback, &breath_data);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_DRACONIC_HERITAGE_BREATHWEAPON);
  USE_STANDARD_ACTION(ch);
}

ACMDCHECK(can_sorcerer_claw_attack)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_DRACONIC_HERITAGE_CLAWS, "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_DRHRT_CLAWS), "You are already in the process of making a draconic claws attack.\r\n");
  return CAN_CMD;
}

ACMD(do_sorcerer_claw_attack)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_sorcerer_claw_attack);
  PREREQ_HAS_USES(FEAT_DRACONIC_HERITAGE_CLAWS, "You must wait to recover your draconic heritage claw attacks.\r\n");

  if (!FIGHTING(ch))
  {
    send_to_char(ch, "You can only use this ability in combat.\r\n");
    return;
  }

  if (!is_action_available(ch, atSTANDARD, TRUE))
  {
    return;
  }

  struct affected_type af;
  int i = 0;

  new_affect(&af);

  af.spell = SKILL_DRHRT_CLAWS;
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_DRACONIC_HERITAGE_CLAWS);
  af.duration = 24;

  affect_to_char(ch, &af);

  send_to_char(ch, "Your hands morph into long draconic claws that you bring to bear on your opponent.\r\n");
  hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);

  for (i = 0; i < NUM_ATTACKS_BAB(ch) / 5 + 1; i++)
  {
    if (FIGHTING(ch))
    {
      hit(ch, FIGHTING(ch), TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
    }
  }

  affect_from_char(ch, SKILL_DRHRT_CLAWS);
  USE_STANDARD_ACTION(ch);
}

/* Callback for tailsweep AoE effect */
static int tailsweep_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  int *vict_count = (int *)data;
  int percent, prob;
  
  if (tch == ch)
    return 0;
  if (IS_INCORPOREAL(tch) && !is_using_ghost_touch_weapon(ch))
    return 0;

  /* stability boots */
  if (is_wearing(tch, 132133) && rand_number(0, 1))
  {
    send_to_char(ch, "You failed to knock over %s due to stability boots!\r\n", GET_NAME(tch));
    send_to_char(tch, "Via your stability boots, you hop over a tailsweep from %s.\r\n",
                 GET_NAME(ch));
    act("$N via stability boots dodges a tailsweep from $n.", FALSE, ch, 0, tch,
        TO_NOTVICT);
    return 0;
  }

  percent = rand_number(1, 101);
  prob = rand_number(75, 100);

  if (percent > prob)
  {
    send_to_char(ch, "You failed to knock over %s.\r\n", GET_NAME(tch));
    send_to_char(tch, "You were able to dodge a tailsweep from %s.\r\n",
                 GET_NAME(ch));
    act("$N dodges a tailsweep from $n.", FALSE, ch, 0, tch,
        TO_NOTVICT);
    return 0;
  }
  else
  {
    change_position(tch, POS_SITTING);

    send_to_char(ch, "You knock over %s.\r\n", GET_NAME(tch));
    send_to_char(tch, "You were knocked down by a tailsweep from %s.\r\n",
                 GET_NAME(ch));
    act("$N is knocked down by a tailsweep from $n.", FALSE, ch, 0, tch,
        TO_NOTVICT);

    /* fire-shield, etc check */
    damage_shield_check(ch, tch, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);

    (*vict_count)++;
  }

  if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL)) // ch -> tch
    set_fighting(ch, tch);
  if (GET_POS(tch) > POS_STUNNED && (FIGHTING(tch) == NULL))
  { // tch -> ch
    set_fighting(tch, ch);
    if (MOB_FLAGGED(tch, MOB_MEMORY) && !IS_NPC(ch))
      remember(tch, ch);
  }

  return 1;
}

/* main engine for tail sweep */
int perform_tailsweep(struct char_data *ch)
{
  int vict_count = 0;

  send_to_char(ch, "You lash out with your mighty tail!\r\n");
  act("$n lashes out with $s mighty tail!", FALSE, ch, 0, 0, TO_ROOM);

  // pass -2 as spellnum to handle tailsweep
  aoe_effect(ch, -2, tailsweep_callback, &vict_count);

  return vict_count;
}

void perform_stones_endurance(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = ABILITY_AFFECT_STONES_ENDURANCE;
  af.duration = 5;

  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STONES_ENDURANCE);

  act("You summon the fortitude and hardiness of the mountains!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n flexes and $s skin turns grey and hard as a mountainside.", FALSE, ch, 0, 0, TO_ROOM);
}

ACMDCHECK(can_stones_endurance)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_STONES_ENDURANCE, "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, ABILITY_AFFECT_STONES_ENDURANCE), "You have already triggered stone's endurance.\r\n");
  return CAN_CMD;
}

ACMDU(do_stones_endurance)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_stones_endurance);
  PREREQ_HAS_USES(FEAT_STONES_ENDURANCE, "You must recover before you can trigger stone's endurance again.\r\n");

  perform_stones_endurance(ch);
}

ACMDCHECK(can_tailsweep)
{
  ACMDCHECK_PERMFAIL_IF(!IS_DRAGON(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_tailsweep)
{

  PREREQ_CAN_FIGHT();

  PREREQ_CHECK(can_tailsweep);
  PREREQ_NOT_PEACEFUL_ROOM();
  PREREQ_NOT_SINGLEFILE_ROOM();

  /* we have to put a restriction here for npc's, otherwise you can order
     a dragon to spam this ability -zusuk */
  if (char_has_mud_event(ch, eDRACBREATH))
  {
    send_to_char(ch, "You are too exhausted to do that!\r\n");
    act("$n tries to use a tailsweep attack, but is too exhausted!", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  perform_tailsweep(ch);

  /* 12 seconds = 2 rounds */
  attach_mud_event(new_mud_event(eDRACBREATH, ch, NULL), 12 * PASSES_PER_SEC);
}

ACMD(do_bash)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_NOT_NPC();
  PREREQ_CAN_FIGHT();
  PREREQ_NOT_PEACEFUL_ROOM();

  one_argument(argument, arg, sizeof(arg));

  /* find the victim */
  vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);

  /* we have a disqualifier here due to action system */
  if (!FIGHTING(ch) && !vict)
  {
    send_to_char(ch, "Who do you want to bash?\r\n");
    return;
  }
  if (vict == ch)
  {
    send_to_char(ch, "You bash yourself.\r\n");
    return;
  }
  if (FIGHTING(ch) && !vict && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    vict = FIGHTING(ch);

  perform_knockdown(ch, vict, SKILL_BASH, true, true);
}

ACMD(do_trip)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (IS_NPC(ch) && GET_CLASS(ch) != CLASS_ROGUE)
  {
    send_to_char(ch, "But you don't know how!\r\n");
    return;
  }

  PREREQ_CAN_FIGHT();

  PREREQ_NOT_PEACEFUL_ROOM();

  one_argument(argument, arg, sizeof(arg));

  /* find the victim */
  vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);

  /* we have a disqualifier here due to action system */
  if (!FIGHTING(ch) && !vict)
  {
    send_to_char(ch, "Who do you want to trip?\r\n");
    return;
  }
  if (FIGHTING(ch) && !vict && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    vict = FIGHTING(ch);

  perform_knockdown(ch, vict, SKILL_TRIP, true, true);
}

ACMDCHECK(can_layonhands)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_LAYHANDS, "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_layonhands)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_CHECK(can_layonhands);

  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "Whom do you want to lay hands on?\r\n");
    return;
  }

  PREREQ_HAS_USES(FEAT_LAYHANDS, "You must recover the divine energy required to lay on hands.\r\n");

  perform_layonhands(ch, vict);
}

ACMDCHECK(can_spiritualweapon)
{
  ACMDCHECK_PREREQ_HASFEAT(PERK_CLERIC_SPIRITUAL_WEAPON, "You don't have the spiritual weapon perk.\r\n");
  return CAN_CMD;
}

ACMD(do_spiritualweapon)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_spiritualweapon);

  /* Check if perk is available */
  if (!has_spiritual_weapon(ch))
  {
    send_to_char(ch, "You don't have the spiritual weapon perk.\r\n");
    return;
  }

  /* Check cooldown - 5 minutes (50 ticks at 6 seconds each = 300 seconds) */
  if (GET_SPIRITUAL_WEAPON_COOLDOWN(ch) > 0)
  {
    int seconds_left = GET_SPIRITUAL_WEAPON_COOLDOWN(ch) * 6;
    int minutes = seconds_left / 60;
    int seconds = seconds_left % 60;
    send_to_char(ch, "You must wait %d minute%s and %d second%s before summoning another spiritual weapon.\r\n",
                 minutes, (minutes != 1 ? "s" : ""), seconds, (seconds != 1 ? "s" : ""));
    return;
  }

  /* Cast spiritual weapon spell */
  call_magic(ch, ch, 0, SPELL_SPIRITUAL_WEAPON, 0, GET_LEVEL(ch), CAST_INNATE);

  /* Set cooldown to 5 minutes (50 ticks) */
  GET_SPIRITUAL_WEAPON_COOLDOWN(ch) = 50;

  send_to_char(ch, "You channel divine energy to summon a spiritual weapon!\r\n");
}

ACMDCHECK(can_irresistablemagic)
{
  ACMDCHECK_PREREQ_HASFEAT(PERK_WIZARD_IRRESISTIBLE_MAGIC, "You don't have the Irresistible Magic perk.\r\n");
  return CAN_CMD;
}

ACMD(do_irresistablemagic)
{
  struct affected_type af;

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_irresistablemagic);

  /* Check if perk is available */
  if (!has_perk(ch, PERK_WIZARD_IRRESISTIBLE_MAGIC))
  {
    send_to_char(ch, "You don't have the Irresistible Magic perk.\r\n");
    return;
  }

  /* Check cooldown - 5 minutes (50 ticks at 6 seconds each = 300 seconds) */
  if (GET_IRRESISTIBLE_MAGIC_COOLDOWN(ch) > 0)
  {
    int seconds_left = GET_IRRESISTIBLE_MAGIC_COOLDOWN(ch) * 6;
    int minutes = seconds_left / 60;
    int seconds = seconds_left % 60;
    send_to_char(ch, "You must wait %d minute%s and %d second%s before using irresistible magic again.\r\n",
                 minutes, (minutes != 1 ? "s" : ""), seconds, (seconds != 1 ? "s" : ""));
    return;
  }

  /* Apply Irresistible Magic buff - next spell auto-succeeds */
  new_affect(&af);
  af.spell = PERK_WIZARD_IRRESISTIBLE_MAGIC;
  af.duration = 3; /* 3 rounds to cast the spell */
  af.location = APPLY_SPECIAL;
  af.modifier = 0;
  af.bonus_type = BONUS_TYPE_CIRCUMSTANCE;
  affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);

  /* Set cooldown to 5 minutes (50 ticks) */
  GET_IRRESISTIBLE_MAGIC_COOLDOWN(ch) = 50;

  send_to_char(ch, "\tMYou weave an unstoppable pattern of arcane energy!\tn\r\n"
                   "Your next spell will bypass all resistances and automatically succeed!\r\n");
  act("\tM$n weaves an unstoppable pattern of arcane energy!\tn", FALSE, ch, NULL, NULL, TO_ROOM);
}

ACMDCHECK(can_spellrecall)
{
  ACMDCHECK_PREREQ_HASFEAT(PERK_WIZARD_SPELL_RECALL, "You don't have the Spell Recall perk.\r\n");
  return CAN_CMD;
}

ACMD(do_spellrecall)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_spellrecall);

  /* Check if perk is available */
  if (!can_use_spell_recall(ch))
  {
    if (!has_perk(ch, PERK_WIZARD_SPELL_RECALL))
    {
      send_to_char(ch, "You don't have the Spell Recall perk.\r\n");
      return;
    }
    
    /* Must be on cooldown */
    int seconds_left = GET_SPELL_RECALL_COOLDOWN(ch) * 6;
    int hours = seconds_left / 3600;
    int minutes = (seconds_left % 3600) / 60;
    int seconds = seconds_left % 60;
    
    if (hours > 0)
      send_to_char(ch, "You must wait %d hour%s, %d minute%s, and %d second%s before using spell recall again.\r\n",
                   hours, (hours != 1 ? "s" : ""), minutes, (minutes != 1 ? "s" : ""), seconds, (seconds != 1 ? "s" : ""));
    else if (minutes > 0)
      send_to_char(ch, "You must wait %d minute%s and %d second%s before using spell recall again.\r\n",
                   minutes, (minutes != 1 ? "s" : ""), seconds, (seconds != 1 ? "s" : ""));
    else
      send_to_char(ch, "You must wait %d second%s before using spell recall again.\r\n",
                   seconds, (seconds != 1 ? "s" : ""));
    return;
  }

  /* Simplified implementation - just display message and set cooldown */
  /* The actual spell slot restoration will be implemented later with proper spell system integration */
  send_to_char(ch, "\tCYou focus your will and recall your arcane knowledge!\tn\r\n"
                   "Your magical reserves are temporarily restored.\r\n"
                   "\tY(This perk's full functionality will be implemented in a future update.)\tn\r\n");
  
  act("\tC$n focuses deeply, recalling arcane knowledge!\tn", FALSE, ch, NULL, NULL, TO_ROOM);
  
  /* Set daily cooldown - 24 hours = 10 ticks per minute * 60 minutes * 24 hours = 14400 ticks */
  GET_SPELL_RECALL_COOLDOWN(ch) = 14400;
}

ACMDCHECK(can_avatarofwar)
{
  ACMDCHECK_PREREQ_HASFEAT(PERK_CLERIC_AVATAR_OF_WAR, "You don't have the Avatar of War perk.\r\n");
  return CAN_CMD;
}

ACMD(do_avatarofwar)
{
  struct affected_type af;

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_avatarofwar);

  /* Check if perk is available */
  if (!has_perk(ch, PERK_CLERIC_AVATAR_OF_WAR))
  {
    send_to_char(ch, "You don't have the Avatar of War perk.\r\n");
    return;
  }

  /* Check prerequisite: Divine Favor III and Armor of Faith III */
  if (!has_perk(ch, PERK_CLERIC_DIVINE_FAVOR_3) || !has_perk(ch, PERK_CLERIC_ARMOR_OF_FAITH_3))
  {
    send_to_char(ch, "You need both Divine Favor III and Armor of Faith III to use this ability.\r\n");
    return;
  }

  /* Check daily use cooldown */
  PREREQ_HAS_USES(PERK_CLERIC_AVATAR_OF_WAR, "You have already used Avatar of War today.\r\n");

  /* Apply Avatar of War buff */
  new_affect(&af);
  af.spell = PERK_CLERIC_AVATAR_OF_WAR;
  af.duration = 10; /* 10 rounds */
  af.location = APPLY_SPECIAL;
  af.modifier = 0;
  af.bonus_type = BONUS_TYPE_MORALE;
  affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);

  send_to_char(ch, "\tWYou transform into an avatar of divine war!\tn\r\n"
                   "You gain \tY+3 to hit\tn, \tC+3 AC\tn, and \tR+10 to damage\tn for 10 rounds!\r\n");
  act("$n transforms into an avatar of divine war, radiating holy power!", FALSE, ch, NULL, NULL, TO_ROOM);

  /* Set daily cooldown */
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, PERK_CLERIC_AVATAR_OF_WAR);
}

ACMDCHECK(can_mastermind)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_MASTER_OF_THE_MIND, "How do you plan on doing that?\r\n");
  return CAN_CMD;
}

ACMD(do_mastermind)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_mastermind);
  PREREQ_HAS_USES(FEAT_MASTER_OF_THE_MIND, "You have expended all of your mastermind attempts.\r\n");

  struct affected_type af;

  new_affect(&af);
  af.spell = PSIONIC_ABILITY_MASTERMIND;
  af.duration = 10;
  af.location = APPLY_SPECIAL;
  affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);

  send_to_char(ch, "You have tapped into your mind mastery.  Your next beneficial power will affect\r\n"
                   "your whole group (in the same room as you).  Or, your next violent spell will\r\n"
                   "affect all non-group members in your room.\r\n");

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_MASTER_OF_THE_MIND);
}

ACMDCHECK(can_insectbeing)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_INSECTBEING, "How do you plan on doing that?\r\n");
  return CAN_CMD;
}

ACMD(do_insectbeing)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_insectbeing);
  PREREQ_HAS_USES(FEAT_INSECTBEING, "You are too exhausted to use insect being.\r\n");

  call_magic(ch, ch, 0, RACIAL_ABILITY_INSECTBEING, 0, GET_LEVEL(ch), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_INSECTBEING);
}

ACMDCHECK(can_crystalfist)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_CRYSTAL_FIST, "How do you plan on doing that?\r\n");
  return CAN_CMD;
}

ACMD(do_crystalfist)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_crystalfist);
  PREREQ_HAS_USES(FEAT_CRYSTAL_FIST, "You are too exhausted to use crystal fist.\r\n");

  // send_to_char(ch, "\tCLarge, razor sharp crystals sprout from your hands and arms!\tn\r\n");
  // act("\tCRazor sharp crystals sprout from $n's arms and hands!\tn", FALSE, ch, 0, 0, TO_NOTVICT);
  // attach_mud_event(new_mud_event(eCRYSTALFIST_AFF, ch, NULL), (3 * SECS_PER_MUD_HOUR));

  call_magic(ch, ch, 0, RACIAL_ABILITY_CRYSTAL_FIST, 0, GET_LEVEL(ch), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_CRYSTAL_FIST);
}

ACMDCHECK(can_crystalbody)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_CRYSTAL_BODY, "How do you plan on doing that?\r\n");
  return CAN_CMD;
}

ACMD(do_crystalbody)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_crystalbody);
  PREREQ_HAS_USES(FEAT_CRYSTAL_BODY, "You are too exhausted to harden your body.\r\n");

  // This ability is now handled in magic.c
  // send_to_char(ch, "\tCYour crystalline body becomes harder!\tn\r\n");
  // act("\tCYou watch as $n's crystalline body becomes harder!\tn", FALSE, ch, 0, 0, TO_NOTVICT);
  // attach_mud_event(new_mud_event(eCRYSTALBODY_AFF, ch, NULL), (3 * SECS_PER_MUD_HOUR));
  call_magic(ch, ch, 0, RACIAL_ABILITY_CRYSTAL_BODY, 0, GET_LEVEL(ch), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_CRYSTAL_BODY);
}

ACMDCHECK(can_reneweddefense)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_RENEWED_DEFENSE, "You have no idea how to do that!\r\n");
  ACMDCHECK_TEMPFAIL_IF(char_has_mud_event(ch, eRENEWEDDEFENSE), "You must wait longer before you can use this ability again.\r\n");
  ACMDCHECK_TEMPFAIL_IF(!affected_by_spell(ch, SKILL_DEFENSIVE_STANCE), "You need to be in a defensive stance to do that!\r\n");
  return CAN_CMD;
}

ACMD(do_reneweddefense)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_reneweddefense);

  if (FIGHTING(ch) && GET_POS(ch) < POS_FIGHTING)
  {
    send_to_char(ch, "You need to be in a better position in combat in order"
                     " to use this ability!\r\n");
    return;
  }

  send_to_char(ch, "Your body glows \tRred\tn as your wounds heal...\r\n");
  act("$n's body glows \tRred\tn as some wounds heal!", FALSE, ch, 0, NULL, TO_NOTVICT);
  attach_mud_event(new_mud_event(eRENEWEDDEFENSE, ch, NULL),
                   (2 * SECS_PER_MUD_DAY));
  GET_HIT(ch) += MIN((GET_MAX_HIT(ch) - GET_HIT(ch)),
                     (dice(CLASS_LEVEL(ch, CLASS_STALWART_DEFENDER) / 2 + 1, 8) + 10 + GET_CON_BONUS(ch)));
  update_pos(ch);

  /* Actions */
  USE_SWIFT_ACTION(ch);
}

ACMDCHECK(can_renewedvigor)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_RP_RENEWED_VIGOR, "You have no idea how to do that!\r\n");
  ACMDCHECK_TEMPFAIL_IF(char_has_mud_event(ch, eRENEWEDVIGOR), "You must wait longer before you can use this ability again.\r\n");
  ACMDCHECK_TEMPFAIL_IF(!affected_by_spell(ch, SKILL_RAGE), "You need to be raging to do that!\r\n");
  return CAN_CMD;
}

ACMD(do_renewedvigor)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_renewedvigor);

  if (FIGHTING(ch) && GET_POS(ch) < POS_FIGHTING)
  {
    send_to_char(ch, "You need to be in a better position in combat in order"
                     " to use this ability!\r\n");
    return;
  }

  send_to_char(ch, "Your body glows \tRred\tn as your wounds heal...\r\n");
  act("$n's body glows \tRred\tn as some wounds heal!", FALSE, ch, 0, NULL, TO_NOTVICT);
  attach_mud_event(new_mud_event(eRENEWEDVIGOR, ch, NULL),
                   (2 * SECS_PER_MUD_DAY));
  GET_HIT(ch) += dice(CLASS_LEVEL(ch, CLASS_BERSERKER) + 3, 8) +
                 10 + GET_CON_BONUS(ch) + GET_DEX_BONUS(ch) + GET_STR_BONUS(ch);
  update_pos(ch);

  /* Actions */
  USE_SWIFT_ACTION(ch);
}

ACMDCHECK(can_wholenessofbody)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_WHOLENESS_OF_BODY, "You have no idea how to do that!\r\n");
  ACMDCHECK_TEMPFAIL_IF(char_has_mud_event(ch, eWHOLENESSOFBODY), "You must wait longer before you can use this "
                                                                  "ability again.\r\n");
  return CAN_CMD;
}

ACMD(do_wholenessofbody)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_wholenessofbody);

  if (FIGHTING(ch) && GET_POS(ch) < POS_FIGHTING)
  {
    send_to_char(ch, "You need to be in a better position in combat in order"
                     " to use this ability!\r\n");
    return;
  }

  send_to_char(ch, "Your body glows \tWwhite\tn as your wounds heal...\r\n");
  act("$n's body glows \tWwhite\tn as some wounds heal!", FALSE, ch, 0, NULL, TO_NOTVICT);
  attach_mud_event(new_mud_event(eWHOLENESSOFBODY, ch, NULL),
                   (4 * SECS_PER_MUD_DAY));
  GET_HIT(ch) += 20 + (MONK_TYPE(ch) + (GET_WIS_BONUS(ch) * 2) * 3);
  update_pos(ch);

  /* Actions */
  USE_SWIFT_ACTION(ch);
}

ACMDCHECK(can_emptybody)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_EMPTY_BODY, "You have no idea how to do that.\r\n");
  return CAN_CMD;
}

ACMD(do_emptybody)
{
  struct affected_type af;

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_emptybody);

  if (char_has_mud_event(ch, eEMPTYBODY))
  {
    send_to_char(ch, "You must wait longer before you can use this "
                     "ability again.\r\n");
    return;
  }

  if (FIGHTING(ch) && GET_POS(ch) < POS_FIGHTING)
  {
    send_to_char(ch, "You need to be in a better position in combat in order"
                     " to use this ability!\r\n");
    return;
  }

  send_to_char(ch, "You focus your Ki energy, drawing energy from the Ethereal plane to transition your body to an 'empty' state.\r\n");
  act("$n briefly closes $s eyes in focus, and you watch as $s body phases partially out of the realm!", FALSE, ch, 0, NULL, TO_NOTVICT);

  attach_mud_event(new_mud_event(eEMPTYBODY, ch, NULL),
                   (2 * SECS_PER_MUD_DAY));

  new_affect(&af);

  af.spell = SPELL_DISPLACEMENT;
  af.duration = MONK_TYPE(ch) + 2;
  SET_BIT_AR(af.bitvector, AFF_DISPLACE);

  affect_to_char(ch, &af);

  /* Actions */
  USE_MOVE_ACTION(ch);
}

ACMDCHECK(can_trip)
{
  return CAN_CMD;
}

ACMDCHECK(can_bash)
{
  return CAN_CMD;
}

ACMDCHECK(can_kick)
{
  return CAN_CMD;
}

ACMDCHECK(can_grapple)
{
  return CAN_CMD;
}

ACMDCHECK(can_slam)
{
  return CAN_CMD;
}

ACMDCHECK(can_feint)
{
  return CAN_CMD;
}

ACMDCHECK(can_guard)
{
  return CAN_CMD;
}

ACMDCHECK(can_charge)
{
  return CAN_CMD;
}

ACMDCHECK(can_bodyslam)
{
  return CAN_CMD;
}

ACMDCHECK(can_disarm)
{
  return CAN_CMD;
}

ACMDCHECK(can_sunder)
{
  return CAN_CMD;
}

ACMDCHECK(can_rescue)
{
  return CAN_CMD;
}

ACMDCHECK(can_treatinjury)
{
  ACMDCHECK_TEMPFAIL_IF(char_has_mud_event(ch, eTREATINJURY), "You must wait longer before you can use this "
                                                              "ability again.\r\n");
  return CAN_CMD;
}

ACMD(do_treatinjury)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;

  PREREQ_NOT_NPC();
  one_argument(argument, arg, sizeof(arg));
  PREREQ_CHECK(can_treatinjury);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "Whom do you want to treat?\r\n");
    return;
  }

  if (FIGHTING(ch) && GET_POS(ch) < POS_FIGHTING)
  {
    send_to_char(ch, "You need to be in a better position in combat in order"
                     " to use this ability!\r\n");
    return;
  }

  send_to_char(ch, "You skillfully dress the wounds...\r\n");
  act("Your injuries are \tWtreated\tn by $N!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n \tWtreats\tn $N's injuries!", FALSE, ch, 0, vict, TO_NOTVICT);

  if (HAS_FEAT(ch, FEAT_FAST_HEALER))
    attach_mud_event(new_mud_event(eTREATINJURY, ch, NULL),
                     (10 * SECS_PER_MUD_HOUR));
  else
    attach_mud_event(new_mud_event(eTREATINJURY, ch, NULL),
                     (20 * SECS_PER_MUD_HOUR));

  /* first attempt to recover lost health */
  if (GET_MAX_HIT(vict) != GET_HIT(vict))
  {
    GET_HIT(vict) += MIN((GET_MAX_HIT(vict) - GET_HIT(vict)),
                         (20 + compute_ability(ch, ABILITY_HEAL) + GET_LEVEL(ch)));
    update_pos(vict);
  }

  /* attempt to cure poison */
  if (compute_ability(ch, ABILITY_HEAL) >= 22)
  {
    if (affected_by_spell(vict, SPELL_POISON))
      affect_from_char(vict, SPELL_POISON);
    if (AFF_FLAGGED(vict, AFF_POISON))
      REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_POISON);
  }

  /* attempt to cure disease */
  if (compute_ability(ch, ABILITY_HEAL) >= 33)
  {
    if (affected_by_spell(vict, SPELL_EYEBITE))
      affect_from_char(vict, SPELL_EYEBITE);
    if (AFF_FLAGGED(vict, AFF_DISEASE))
      REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_DISEASE);
  }

  if (affected_by_spell(vict, BOMB_AFFECT_BONESHARD))
  {
    affect_from_char(vict, BOMB_AFFECT_BONESHARD);
    if (ch == vict)
    {
      act("The bone shards in your flesh dissolve and your bleeding stops.", FALSE, ch, 0, vict, TO_CHAR);
    }
    else
    {
      act("The bone shards in your flesh dissolve and your bleeding stops.", FALSE, ch, 0, vict, TO_VICT);
      act("The bone shards in $N's flesh dissolve and $S bleeding stops.", FALSE, ch, 0, vict, TO_ROOM);
    }
  }

  /* Actions */
  USE_STANDARD_ACTION(ch);
}

ACMD(do_bandage)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;

  PREREQ_NOT_NPC();
  one_argument(argument, arg, sizeof(arg));

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "Whom do you want to bandage?\r\n");
    return;
  }

  if (FIGHTING(ch) && GET_POS(ch) < POS_FIGHTING)
  {
    send_to_char(ch, "You need to be in a better position in combat in order"
                     " to use this ability!\r\n");
    return;
  }

  sbyte attempted = FALSE;

  if (GET_HIT(vict) <= 0)
  {
    if (skill_check(ch, ABILITY_HEAL, 15))
    {
      send_to_char(ch, "You skillfully BANDAGE the wounds...\r\n");
      act("Your injuries are \tWbandaged\tn by $N!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n \tWbandages\tn $N's injuries!", FALSE, ch, 0, vict, TO_NOTVICT);
      GET_HIT(vict) = 1;
      update_pos(vict);
    }
    else
    {
      if (ch == vict)
      {
        act("You try, but fail to bandage your own wounds.", FALSE, ch, 0, vict, TO_CHAR);
        act("$n tries, but fails to bandage $s own wounds.", FALSE, ch, 0, vict, TO_ROOM);
      }
      else
      {
        act("You try, but fail to bandage $N's wounds.", FALSE, ch, 0, vict, TO_CHAR);
        act("$n tries, but fails to bandage your wounds.", FALSE, ch, 0, vict, TO_VICT);
        act("$n tries, but fails to bandage $N's wounds.", FALSE, ch, 0, vict, TO_NOTVICT);
      }
    }
    attempted = TRUE;
  }

  if (affected_by_spell(vict, BOMB_AFFECT_BONESHARD))
  {
    if (skill_check(ch, ABILITY_HEAL, 15))
    {
      affect_from_char(vict, BOMB_AFFECT_BONESHARD);
      if (ch == vict)
      {
        act("The bone shards in your flesh dissolve and your bleeding stops.", FALSE, ch, 0, vict, TO_CHAR);
      }
      else
      {
        act("The bone shards in your flesh dissolve and your bleeding stops.", FALSE, ch, 0, vict, TO_VICT);
        act("The bone shards in $N's flesh dissolve and $S bleeding stops.", FALSE, ch, 0, vict, TO_ROOM);
      }
      attempted = TRUE;
    }
  }

  if (!attempted)
  {
    if (ch == vict)
    {
      act("You bandage yourself to no effect.", FALSE, ch, 0, vict, TO_CHAR);
      act("$n bandages $mself to no effect.", FALSE, ch, 0, vict, TO_ROOM);
    }
    else
    {
      act("You bandage $N to no effect.", FALSE, ch, 0, vict, TO_CHAR);
      act("$n bandages you to no effect.", FALSE, ch, 0, vict, TO_VICT);
      act("$n bandages $N to no effect.", FALSE, ch, 0, vict, TO_NOTVICT);
    }
  }

  /* Actions */
  USE_STANDARD_ACTION(ch);
}

ACMD(do_rescue)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;

  if (IS_NPC(ch) && !IS_FIGHTER(ch))
  {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  PREREQ_CAN_FIGHT();

  one_argument(argument, arg, sizeof(arg));

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "Whom do you want to rescue?\r\n");
    return;
  }

  perform_rescue(ch, vict);
}

ACMDCHECK(can_whirlwind)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_WHIRLWIND_ATTACK, "You have no idea how.\r\n");
  return CAN_CMD;
}

/* whirlwind attack! */
ACMD(do_whirlwind)
{
  int num_attacks = 1;

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_whirlwind);
  PREREQ_IN_POSITION(POS_SITTING, "You must be on your feet to perform a whirlwind.\r\n");
  PREREQ_NOT_SINGLEFILE_ROOM();
  PREREQ_NOT_PEACEFUL_ROOM();

#define RETURN_NUM_ATTACKS 1
  num_attacks += perform_attacks(ch, RETURN_NUM_ATTACKS, 0);
#undef RETURN_NUM_ATTACKS

  send_to_char(ch, "In a whirlwind of motion you strike out at your foes!\r\n");
  act("$n in a whirlwind of motions lashes out at $s foes!", FALSE, ch, 0, 0, TO_ROOM);

  /* Callback for whirlwind AoE attack */
  int whirlwind_callback(struct char_data *ch, struct char_data *tch, void *data) {
    int *remaining_attacks = (int *)data;
    
    if (*remaining_attacks <= 0)
      return 0;
    
    hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
    (*remaining_attacks)--;
    
    return 1;
  }

  aoe_effect(ch, -1, whirlwind_callback, &num_attacks);

  USE_FULL_ROUND_ACTION(ch);
  return;

  /* OLD VERSION */
  /* First thing we do is check to make sure the character is not in the middle
   * of a whirl wind attack.
   *
   * "char_had_mud_event() will sift through the character's event list to see if
   * an event of type "eWHIRLWIND" currently exists. */
  /*
  if (char_has_mud_event(ch, eWHIRLWIND)) {
    send_to_char(ch, "You are already attempting that!\r\n");
    return;
  }

  send_to_char(ch, "You begin to spin rapidly in circles.\r\n");
  act("$n begins to rapidly spin in a circle!", FALSE, ch, 0, 0, TO_ROOM);
   */

  /* NEW_EVENT() will add a new mud event to the event list of the character.
   * This function below adds a new event of "eWHIRLWIND", to "ch", and passes "NULL" as
   * additional data. The event will be called in "3 * PASSES_PER_SEC" or 3 seconds */
  // NEW_EVENT(eWHIRLWIND, ch, NULL, 3 * PASSES_PER_SEC);
}

ACMDCHECK(can_deatharrow)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_ARROW_OF_DEATH, "You don't know how to do this!\r\n");

  /* ranged attack requirement */
  ACMDCHECK_TEMPFAIL_IF(!can_fire_ammo(ch, TRUE), "You have to be using a ranged weapon with ammo ready to "
                                                  "fire in your ammo pouch to do this!\r\n");
  return CAN_CMD;
}

ACMD(do_deatharrow)
{

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_deatharrow);
  PREREQ_HAS_USES(FEAT_ARROW_OF_DEATH, "You must recover before you can use another death arrow.\r\n");

  if (affected_by_spell(ch, SKILL_DEATH_ARROW))
  {
    send_to_char(ch, "You have already imbued one of your arrows with death!\r\n");
    return;
  }

  perform_deatharrow(ch);
}

ACMDCHECK(can_quiveringpalm)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_QUIVERING_PALM, "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_QUIVERING_PALM), "You have already focused your ki!\r\n");
  return CAN_CMD;
}

ACMD(do_quiveringpalm)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_quiveringpalm);
  PREREQ_HAS_USES(FEAT_QUIVERING_PALM, "You must recover before you can focus your ki in this way again.\r\n");

  perform_quiveringpalm(ch);
}

ACMDCHECK(can_stunningfist)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_STUNNING_FIST, "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_STUNNING_FIST), "You have already focused your ki!\r\n");
  return CAN_CMD;
}

ACMD(do_stunningfist)
{

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_stunningfist);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_stunningfist(ch);
}

ACMDCHECK(can_crushingblow)
{
  ACMDCHECK_PERMFAIL_IF(!has_perk(ch, PERK_MONK_CRUSHING_BLOW), "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_CRUSHING_BLOW), "You have already prepared a crushing blow!\r\n");
  return CAN_CMD;
}

ACMD(do_crushingblow)
{

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_crushingblow);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_crushingblow(ch);
}

ACMDCHECK(can_shatteringstrike)
{
  ACMDCHECK_PERMFAIL_IF(!has_perk(ch, PERK_MONK_SHATTERING_STRIKE), "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_SHATTERING_STRIKE), "You have already prepared a shattering strike!\r\n");
  return CAN_CMD;
}

ACMD(do_shatteringstrike)
{

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_shatteringstrike);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_shatteringstrike(ch);
}

ACMDCHECK(can_vanishingtechnique)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_vanishing_technique(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_vanishingtechnique)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int spell_num = SPELL_INVISIBLE;

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_vanishingtechnique);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  one_argument(argument, arg, sizeof(arg));

  if (*arg)
  {
    if (is_abbrev(arg, "greater"))
    {
      if (!has_monk_shadow_master(ch))
      {
        send_to_char(ch, "You need the Shadow Master perk to use greater invisibility.\r\n");
        return;
      }
      spell_num = SPELL_GREATER_INVIS;
    }
    else if (!is_abbrev(arg, "invisibility"))
    {
      send_to_char(ch, "Usage: vanishingtechnique [invisibility|greater]\r\n");
      return;
    }
  }

  perform_vanishingtechnique(ch, spell_num);
}

ACMDCHECK(can_shadowclone)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_shadow_clone(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_shadowclone)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_shadowclone);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_shadowclone(ch);
}

ACMDCHECK(can_smokebomb)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_smoke_bomb(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_smokebomb)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_smokebomb);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_smokebomb(ch);
}

ACMDCHECK(can_miststance)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_mist_stance(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_miststance)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_miststance);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_miststance(ch);
}

ACMDCHECK(can_icerabbit)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_swarming_ice_rabbit(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_icerabbit)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL, *tch = NULL;
  room_rnum room = NOWHERE;
  int direction = -1, original_loc = NOWHERE;

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_icerabbit);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You are too busy fighting to use this technique!\r\n");
    return;
  }

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  /* no 2nd argument? target room has to be same room */
  if (!*arg2)
  {
    room = IN_ROOM(ch);
  }
  else
  {
    /* try to find target room */
    direction = search_block(arg2, dirs, FALSE);
    if (direction < 0)
    {
      send_to_char(ch, "That is not a direction!\r\n");
      return;
    }
    if (!CAN_GO(ch, direction))
    {
      send_to_char(ch, "You can't send the ice rabbits in that direction!\r\n");
      return;
    }
    room = EXIT(ch, direction)->to_room;
  }

  /* check if combat is ok in target room */
  if (ROOM_FLAGGED(room, ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  /* no arguments? no go! */
  if (!*arg1)
  {
    send_to_char(ch, "You need to select a target!\r\n");
    return;
  }

  /* a location has been found - temporarily move to target room to find victim */
  original_loc = IN_ROOM(ch);
  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(room), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[room].coords[0];
    Y_LOC(ch) = world[room].coords[1];
  }

  char_to_room(ch, room);
  vict = get_char_room_vis(ch, arg1, NULL);

  /* move character back to original room */
  if (IN_ROOM(ch) == room)
  {
    char_from_room(ch);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(original_loc), ZONE_WILDERNESS))
    {
      X_LOC(ch) = world[original_loc].coords[0];
      Y_LOC(ch) = world[original_loc].coords[1];
    }

    char_to_room(ch, original_loc);
  }

  if (!vict)
  {
    send_to_char(ch, "Target who?\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "You can't target yourself!\r\n");
    return;
  }

  /* if target is group member in same room, we presume you meant to assist */
  if (GROUP(ch) && room == IN_ROOM(ch))
  {
    simple_list(NULL);
    
    while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
    {
      if (IN_ROOM(tch) != IN_ROOM(vict))
        continue;
      if (vict == tch)
      {
        vict = FIGHTING(vict);
        break;
      }
    }
  }

  /* maybe its your pet? so assist */
  if (vict && IS_PET(vict) && vict->master == ch && room == IN_ROOM(ch))
    vict = FIGHTING(vict);

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) && room == IN_ROOM(ch) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  perform_icerabbit(ch, vict);
  USE_STANDARD_ACTION(ch);
}

/* ============ Tier 3 Four Elements Abilities ============ */

ACMDCHECK(can_flamesofphoenix)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_flames_of_phoenix(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_flamesofphoenix)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_flamesofphoenix);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_flamesofphoenix(ch);
}

ACMDCHECK(can_waveofrollingearth)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_wave_of_rolling_earth(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_waveofrollingearth)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_waveofrollingearth);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_waveofrollingearth(ch);
}

ACMDCHECK(can_ridethewind)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_ride_the_wind(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_ridethewind)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_ridethewind);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_ridethewind(ch);
}

ACMDCHECK(can_eternalmountaindefense)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_eternal_mountain_defense(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_eternalmountaindefense)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_eternalmountaindefense);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_eternalmountaindefense(ch);
}

ACMDCHECK(can_fistoffourthunders)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_fist_of_four_thunders(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_fistoffourthunders)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_fistoffourthunders);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_fistoffourthunders(ch);
}

ACMDCHECK(can_riverofhungryflame)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_river_of_hungry_flame(ch), "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(daily_uses_remaining(ch, FEAT_STUNNING_FIST) == 0, "You are out of ki points.\r\n");
  return CAN_CMD;
}

ACMD(do_riverofhungryflame)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int dir = -1;

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_riverofhungryflame);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "You must specify a direction for the River of Hungry Flame.\r\n");
    return;
  }

  dir = search_block(arg, dirs, FALSE);
  if (dir >= 0)
  {
    perform_riverofhungryflame(ch, dir);
  }
  else
  {
    send_to_char(ch, "You must specify a valid direction (north, south, east, west, up, down).\r\n");
  }
}

/* ============ Tier 4 Four Elements Capstones ============ */

ACMDCHECK(can_breathofwinter)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_breath_of_winter(ch), "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(daily_uses_remaining(ch, FEAT_STUNNING_FIST) == 0, "You are out of ki points.\r\n");
  return CAN_CMD;
}

ACMD(do_breathofwinter)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_breathofwinter);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_breathofwinter(ch);
}

ACMDCHECK(can_elementalembodiment)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_elemental_embodiment(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_elementalembodiment)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int element_type = 0;

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_elementalembodiment);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Choose an element: fire, water, air, or earth.\r\n");
    return;
  }

  if (is_abbrev(arg, "fire"))
    element_type = 1;
  else if (is_abbrev(arg, "water"))
    element_type = 2;
  else if (is_abbrev(arg, "air"))
    element_type = 3;
  else if (is_abbrev(arg, "earth"))
    element_type = 4;
  else
  {
    send_to_char(ch, "Choose an element: fire, water, air, or earth.\r\n");
    return;
  }

  perform_elementalembodiment(ch, element_type);
}

ACMDCHECK(can_avatarofelements)
{
  ACMDCHECK_PREREQ_HASFEAT(PERK_MONK_AVATAR_OF_ELEMENTS, "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_avatarofelements)
{
  struct affected_type af;

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_avatarofelements);

  /* Check if perk is available */
  if (!has_perk(ch, PERK_MONK_AVATAR_OF_ELEMENTS))
  {
    send_to_char(ch, "You don't have the Avatar of the Elements perk.\r\n");
    return;
  }

  /* Check daily use cooldown */
  PREREQ_HAS_USES(PERK_MONK_AVATAR_OF_ELEMENTS, "You have already used Avatar of the Elements today.\r\n");

  /* Apply Avatar of the Elements buff */
  new_affect(&af);
  af.spell = PERK_MONK_AVATAR_OF_ELEMENTS;
  af.duration = 5; /* 5 rounds */
  af.location = APPLY_SPECIAL;
  af.modifier = 0;
  af.bonus_type = BONUS_TYPE_ENHANCEMENT;
  affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);

  send_to_char(ch, "\tCYou become one with the elements, channeling their raw power!\tn\r\n"
                   "Your elemental abilities gain \tR+2d6 damage\tn, you have a \tY25%% chance\tn to use ki abilities for free,\r\n"
                   "and you gain \tWimmunity\tn to \trfire\tn, \tbcold\tn, \tcair\tn, \tyelectric\tn, \tDearth\tn, and \tcwater\tn damage for 5 rounds!\r\n");
  act("$n becomes wreathed in elemental power, $s body shimmering with fire, ice, lightning, and earth!", FALSE, ch, NULL, NULL, TO_ROOM);

  /* Set daily cooldown */
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, PERK_MONK_AVATAR_OF_ELEMENTS);
}

ACMDCHECK(can_shadowwalk)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_shadow_step_iii(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_shadowwalk)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_shadowwalk);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_shadowwalk(ch);
}

ACMDCHECK(can_blinding_speed)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_blinding_speed(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_blinding_speed)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_blinding_speed);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_blinding_speed(ch);
}

ACMDCHECK(can_voidstrike)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_void_strike(ch), "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMDCHECK(can_firesnake)
{
  ACMDCHECK_PERMFAIL_IF(!has_monk_fangs_of_fire_snake(ch), "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(daily_uses_remaining(ch, FEAT_STUNNING_FIST) == 0, "You are out of ki points.\r\n");
  return CAN_CMD;
}

ACMD(do_voidstrike)
{
  time_t current_time = time(0);

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_voidstrike);

  /* Check cooldown */
  if (GET_VOID_STRIKE_COOLDOWN(ch) > current_time)
  {
    int seconds = (int)(GET_VOID_STRIKE_COOLDOWN(ch) - current_time);
    send_to_char(ch, "You must wait %d second%s before using void strike again.\r\n",
                 seconds, seconds == 1 ? "" : "s");
    return;
  }

  perform_voidstrike(ch);
}

ACMD(do_elementalmastery)
{
  time_t current_time = time(0);

  if (IS_NPC(ch))
  {
    send_to_char(ch, "Monsters cannot use elemental mastery.\r\n");
    return;
  }

  if (!has_druid_elemental_mastery(ch))
  {
    send_to_char(ch, "You need the Elemental Mastery perk to use this ability.\r\n");
    return;
  }

  /* Check cooldown (5 minutes = 300 seconds) */
  if (GET_ELEMENTAL_MASTERY_COOLDOWN(ch) > current_time)
  {
    int seconds = (int)(GET_ELEMENTAL_MASTERY_COOLDOWN(ch) - current_time);
    int minutes = seconds / 60;
    int remaining_seconds = seconds % 60;
    
    if (minutes > 0)
    {
      send_to_char(ch, "You must wait %d minute%s and %d second%s before using elemental mastery again.\r\n",
                   minutes, minutes == 1 ? "" : "s",
                   remaining_seconds, remaining_seconds == 1 ? "" : "s");
    }
    else
    {
      send_to_char(ch, "You must wait %d second%s before using elemental mastery again.\r\n",
                   seconds, seconds == 1 ? "" : "s");
    }
    return;
  }

  /* Check if already active */
  if (GET_ELEMENTAL_MASTERY_ACTIVE(ch))
  {
    send_to_char(ch, "Elemental mastery is already active and will trigger on your next elemental spell.\r\n");
    return;
  }

  /* Activate elemental mastery */
  GET_ELEMENTAL_MASTERY_ACTIVE(ch) = TRUE;
  send_to_char(ch, "\tCYou channel the raw power of the elements!\tn\r\n");
  send_to_char(ch, "\tCYour next elemental spell will deal maximum damage!\tn\r\n");
  act("$n channels the raw power of the elements!", TRUE, ch, 0, 0, TO_ROOM);
}

ACMD(do_firesnake)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_firesnake);

  perform_firesnake(ch);
}

/* Power Strike command - monk combat mode */
ACMD(do_powerstrike)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int value = 0;
  int max_ranks = 0;
  
  if (IS_NPC(ch))
  {
    send_to_char(ch, "Monsters cannot use power strike.\r\n");
    return;
  }
  
  if (!has_perk(ch, PERK_MONK_POWER_STRIKE))
  {
    send_to_char(ch, "You need the power strike perk to use this ability.\r\n");
    return;
  }
  
  one_argument(argument, arg, sizeof(arg));
  
  if (!*arg)
  {
    /* Show current status */
    if (GET_POWER_STRIKE(ch) > 0)
    {
      send_to_char(ch, "You are currently using power strike at level %d: -%d to hit, +%d to damage.\r\n",
                   GET_POWER_STRIKE(ch), GET_POWER_STRIKE(ch), GET_POWER_STRIKE(ch) * 2);
      send_to_char(ch, "Usage: powerstrike <0-%d> to adjust or turn off.\r\n", 
                   get_perk_rank(ch, PERK_MONK_POWER_STRIKE, CLASS_MONK));
    }
    else
    {
      max_ranks = get_perk_rank(ch, PERK_MONK_POWER_STRIKE, CLASS_MONK);
      send_to_char(ch, "Power strike is currently off.\r\n");
      send_to_char(ch, "Usage: powerstrike <1-%d> to activate (-1 to hit, +2 damage per rank).\r\n", max_ranks);
    }
    return;
  }
  
  if (!is_number(arg))
  {
    send_to_char(ch, "You must specify a number between 0 and %d.\r\n",
                 get_perk_rank(ch, PERK_MONK_POWER_STRIKE, CLASS_MONK));
    return;
  }
  
  value = atoi(arg);
  max_ranks = get_perk_rank(ch, PERK_MONK_POWER_STRIKE, CLASS_MONK);
  
  if (value < 0)
  {
    send_to_char(ch, "The minimum value is 0 (to turn off power strike).\r\n");
    return;
  }
  
  if (value > max_ranks)
  {
    send_to_char(ch, "You can only set power strike up to %d (your perk rank).\r\n", max_ranks);
    return;
  }
  
  GET_POWER_STRIKE(ch) = value;
  
  if (value == 0)
  {
    send_to_char(ch, "You stop using power strike.\r\n");
  }
  else
  {
    send_to_char(ch, "You focus your strikes for maximum impact!\r\n");
    send_to_char(ch, "Power strike level %d: -%d to hit, +%d to damage on unarmed and monk weapon attacks.\r\n",
                 value, value, value * 2);
  }
}

ACMDCHECK(can_surpriseaccuracy)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_RP_SURPRISE_ACCURACY, "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_SURPRISE_ACCURACY), "You have already focused your rage into accuracy!\r\n");
  ACMDCHECK_TEMPFAIL_IF(char_has_mud_event(ch, eSURPRISE_ACCURACY), "You are too exhausted to use surprise accuracy again!\r\n");
  ACMDCHECK_TEMPFAIL_IF(AFF_FLAGGED(ch, AFF_FATIGUED), "You are are too fatigued to use surprise accuracy!\r\n");
  ACMDCHECK_TEMPFAIL_IF(!affected_by_spell(ch, SKILL_RAGE), "You need to be raging to use surprise accuracy!\r\n");
  return CAN_CMD;
}

ACMD(do_surpriseaccuracy)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_surpriseaccuracy);

  perform_surpriseaccuracy(ch);
}

ACMDCHECK(can_comeandgetme)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_RP_COME_AND_GET_ME, "You have no idea how.\r\n");

  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_COME_AND_GET_ME),
                        "You have already focused your rage into 'come and get me'!\r\n");

  ACMDCHECK_TEMPFAIL_IF(char_has_mud_event(ch, eCOME_AND_GET_ME),
                        "You are too exhausted to use 'come and get me' again!\r\n");

  ACMDCHECK_TEMPFAIL_IF(AFF_FLAGGED(ch, AFF_FATIGUED),
                        "You are are too fatigued to use 'come and get me'!\r\n");

  ACMDCHECK_TEMPFAIL_IF(!affected_by_spell(ch, SKILL_RAGE),
                        "You need to be raging to use 'come and get me'!\r\n");

  return CAN_CMD;
}

ACMD(do_comeandgetme)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_comeandgetme);

  perform_comeandgetme(ch);
}

ACMDCHECK(can_powerfulblow)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_RP_POWERFUL_BLOW, "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_POWERFUL_BLOW),
                        "You have already focused your rage into a powerful blow!\r\n");
  ACMDCHECK_TEMPFAIL_IF(char_has_mud_event(ch, ePOWERFUL_BLOW),
                        "You are too exhausted to use powerful blow again!\r\n");
  ACMDCHECK_TEMPFAIL_IF(AFF_FLAGGED(ch, AFF_FATIGUED),
                        "You are too fatigued to use powerful blow again!\r\n");
  ACMDCHECK_TEMPFAIL_IF(!affected_by_spell(ch, SKILL_RAGE),
                        "You need to be raging to use powerful blow!\r\n");

  return CAN_CMD;
}

ACMD(do_powerfulblow)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_powerfulblow);

  perform_powerfulblow(ch);
}

ACMDCHECK(can_smitegood)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SMITE_GOOD, "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_smitegood)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_smitegood);
  PREREQ_HAS_USES(FEAT_SMITE_GOOD, "You must recover the divine energy required to smite good.\r\n");

  perform_smite(ch, SMITE_TYPE_GOOD);
}

ACMD(do_aura_of_vengeance)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_smitegood);
  PREREQ_HAS_USES(FEAT_SMITE_GOOD, "You must recover the divine energy required to enact an aura of vengeance.\r\n");

  struct char_data *tch = NULL;
  struct affected_type af;

  send_to_char(ch, "You enact an aura of vengeance upon your present allies.\r\n");
  act("$n enacts an aura of vengeance around $m and $s present allies.", TRUE, ch, 0, 0, TO_ROOM);

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (GROUP(ch) != GROUP(tch))
      continue;
    if (ch == tch)
      continue;
    new_affect(&af);
    af.spell = SKILL_SMITE_GOOD;
    af.duration = 24;
    affect_to_char(tch, &af);

    act("$n gives you the ability to smite good against your current target.", FALSE, ch, 0, tch, TO_VICT);
    act("You give $N the ability to smite good against your current target.", FALSE, ch, 0, tch, TO_CHAR);
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SMITE_GOOD);
}

ACMD(do_aura_of_justice)
{
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_smiteevil);
  PREREQ_HAS_USES(FEAT_SMITE_EVIL, "You must recover the divine energy required to enact an aura of justice.\r\n");

  struct char_data *tch = NULL;
  struct affected_type af;

  send_to_char(ch, "You enact an aura of justice upon your present allies.\r\n");
  act("$n enacts an aura of justice around $m and $s present allies.", TRUE, ch, 0, 0, TO_ROOM);

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (GROUP(ch) != GROUP(tch))
      continue;
    if (ch == tch)
      continue;
    new_affect(&af);
    af.spell = SKILL_SMITE_EVIL;
    af.duration = 24;
    affect_to_char(tch, &af);

    act("$n gives you the ability to smite evil against your current target.", FALSE, ch, 0, tch, TO_VICT);
    act("You give $N the ability to smite evil against your current target.", FALSE, ch, 0, tch, TO_CHAR);
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SMITE_EVIL);
}

ACMDCHECK(can_smiteevil)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SMITE_EVIL, "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_smiteevil)
{

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_smiteevil);
  PREREQ_HAS_USES(FEAT_SMITE_EVIL, "You must recover the divine energy required to smite evil.\r\n");

  perform_smite(ch, SMITE_TYPE_EVIL);
}

/* Paladin Faithful Strike - Knight of the Chalice Tier 1 */
ACMD(do_faithful_strike)
{
  struct affected_type af;
  int duration = 1; // 1 round
  int wis_bonus = GET_WIS_BONUS(ch);

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();

  /* Check if already using faithful strike */
  if (affected_by_spell(ch, SKILL_FAITHFUL_STRIKE))
  {
    send_to_char(ch, "You have already channeled divine power!\r\n");
    return;
  }

  /* Check if they have the perk */
  if (!has_paladin_faithful_strike(ch))
  {
    send_to_char(ch, "You don't know how to use faithful strike!\r\n");
    return;
  }

  /* Check cooldown - 1 minute cooldown */
  PREREQ_HAS_USES(SKILL_FAITHFUL_STRIKE, "You must recover before you can use faithful strike again.\r\n");

  send_to_char(ch, "You channel divine power into your next attack!\r\n");
  act("$n's weapon glows briefly with divine light!", 
      FALSE, ch, 0, 0, TO_ROOM);

  /* Add WIS bonus to next attack roll */
  new_affect(&af);
  af.spell = SKILL_FAITHFUL_STRIKE;
  af.duration = duration;
  af.location = APPLY_HITROLL;
  af.modifier = wis_bonus;
  af.bonus_type = BONUS_TYPE_SACRED;
  affect_to_char(ch, &af);

  /* Start cooldown */
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, SKILL_FAITHFUL_STRIKE);

  USE_SWIFT_ACTION(ch);
}

/* Paladin Holy Blade - Knight of the Chalice Tier 2 */
ACMD(do_holy_blade)
{
  struct affected_type af;
  int duration = 50; // 5 minutes = 50 rounds
  struct obj_data *wielded = NULL;

  PREREQ_NOT_NPC();

  /* Check if they have the perk */
  if (!has_paladin_holy_blade(ch))
  {
    send_to_char(ch, "You don't know how to enchant your weapon with holy power!\r\n");
    return;
  }

  /* Check cooldown - 10 minute cooldown */
  PREREQ_HAS_USES(SKILL_HOLY_BLADE, "You must recover before you can use holy blade again.\r\n");

  /* Must be wielding a weapon */
  wielded = GET_EQ(ch, WEAR_WIELD_1);
  if (!wielded)
    wielded = GET_EQ(ch, WEAR_WIELD_2H);
  if (!wielded)
  {
    send_to_char(ch, "You must be wielding a weapon to enchant it!\r\n");
    return;
  }

  /* Check if weapon is already affected */
  if (affected_by_spell(ch, SKILL_HOLY_BLADE))
  {
    send_to_char(ch, "Your weapon is already enchanted with holy power!\r\n");
    return;
  }

  send_to_char(ch, "You call upon divine power to enchant your weapon with holy might!\r\n");
  act("$n's $p glows with brilliant holy light!", 
      FALSE, ch, wielded, 0, TO_ROOM);

  /* Enhancement bonus (base +2, or +4 with Holy Sword perk) */
  int enhancement_bonus = 2;
  if (has_paladin_holy_sword(ch))
    enhancement_bonus = 4;

  /* Enhancement bonus to hit */
  new_affect(&af);
  af.spell = SKILL_HOLY_BLADE;
  af.duration = duration;
  af.location = APPLY_HITROLL;
  af.modifier = enhancement_bonus;
  af.bonus_type = BONUS_TYPE_ENHANCEMENT;
  affect_to_char(ch, &af);

  /* Enhancement bonus to damage */
  new_affect(&af);
  af.spell = SKILL_HOLY_BLADE;
  af.duration = duration;
  af.location = APPLY_DAMROLL;
  af.modifier = enhancement_bonus;
  af.bonus_type = BONUS_TYPE_ENHANCEMENT;
  affect_to_char(ch, &af);

  /* Start cooldown */
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, SKILL_HOLY_BLADE);
}

/* Paladin Divine Might - Knight of the Chalice Tier 3 */
ACMD(do_divine_might)
{
  struct affected_type af;
  int duration = 10; // 1 minute = 10 rounds
  int cha_bonus = GET_CHA_BONUS(ch);

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();

  /* Check if already using divine might */
  if (affected_by_spell(ch, SKILL_DIVINE_MIGHT))
  {
    send_to_char(ch, "You are already channeling divine might!\r\n");
    return;
  }

  /* Check if they have the perk */
  if (!has_paladin_divine_might(ch))
  {
    send_to_char(ch, "You don't know how to channel divine might!\r\n");
    return;
  }

  /* Check cooldown - 5 minute cooldown */
  PREREQ_HAS_USES(SKILL_DIVINE_MIGHT, "You must recover before you can use divine might again.\r\n");

  send_to_char(ch, "You channel divine power into your strikes, infusing them with righteous fury!\r\n");
  act("$n radiates with divine power as holy energy flows through $s weapon!", 
      FALSE, ch, 0, 0, TO_ROOM);

  /* Add CHA bonus to damage */
  new_affect(&af);
  af.spell = SKILL_DIVINE_MIGHT;
  af.duration = duration;
  af.location = APPLY_DAMROLL;
  af.modifier = cha_bonus;
  af.bonus_type = BONUS_TYPE_SACRED;
  affect_to_char(ch, &af);

  /* Start cooldown */
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, SKILL_DIVINE_MIGHT);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_defensive_strike)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;
  struct affected_type af;
  int duration = 5; // 5 rounds

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();

  /* Check if they have the perk */
  if (!has_paladin_defensive_strike(ch))
  {
    send_to_char(ch, "You don't know how to perform a defensive strike!\r\n");
    return;
  }

  /* Check cooldown */
  PREREQ_HAS_USES(SKILL_DEFENSIVE_STRIKE, "You must recover before you can use defensive strike again.\r\n");

  /* Find target */
  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
    else
    {
      send_to_char(ch, "Defensive strike who?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "You can't defensive strike yourself!\r\n");
    return;
  }

  PREREQ_NOT_PEACEFUL_ROOM();

  /* Make the attack */
  send_to_char(ch, "You strike out with a defensive stance, preparing to guard!\r\n");
  act("$n strikes at $N with a defensive stance!", FALSE, ch, 0, vict, TO_NOTVICT);
  act("$n strikes at you with a defensive stance!", FALSE, ch, 0, vict, TO_VICT);

  /* Perform one attack */
  hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

  /* Apply AC bonus regardless of hit */
  new_affect(&af);
  af.spell = SKILL_DEFENSIVE_STRIKE;
  af.duration = duration;
  af.location = APPLY_AC_NEW;
  af.modifier = 2;
  af.bonus_type = BONUS_TYPE_DODGE;
  affect_to_char(ch, &af);

  send_to_char(ch, "You assume a defensive posture, gaining +2 AC!\r\n");
  act("$n assumes a defensive posture!", FALSE, ch, 0, 0, TO_ROOM);

  /* Start cooldown */
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, SKILL_DEFENSIVE_STRIKE);

  USE_MOVE_ACTION(ch);
}

ACMD(do_bastion)
{
  struct affected_type af;
  int duration = 5; // 5 rounds

  PREREQ_NOT_NPC();

  /* Check if they have the perk */
  if (!has_paladin_bastion_of_defense(ch))
  {
    send_to_char(ch, "You don't know how to invoke bastion of defense!\r\n");
    return;
  }

  /* Check cooldown - 5 minute cooldown */
  PREREQ_HAS_USES(SKILL_BASTION, "You must recover before you can invoke bastion again.\r\n");

  /* Check if already active */
  if (affected_by_spell(ch, SKILL_BASTION))
  {
    send_to_char(ch, "You are already protected by bastion of defense!\r\n");
    return;
  }

  send_to_char(ch, "\tWYou invoke the bastion of defense, becoming an immovable defender!\tn\r\n");
  act("\tW$n glows with divine power, becoming an immovable bastion!\tn", FALSE, ch, 0, 0, TO_ROOM);

  /* Grant 20 temporary HP */
  new_affect(&af);
  af.spell = SKILL_BASTION;
  af.duration = duration;
  af.location = APPLY_HIT;
  af.modifier = 20; /* +20 temporary HP */
  affect_to_char(ch, &af);

  /* Grant +4 AC */
  new_affect(&af);
  af.spell = SKILL_BASTION;
  af.duration = duration;
  af.location = APPLY_AC_NEW;
  af.modifier = 4;
  af.bonus_type = BONUS_TYPE_SACRED;
  affect_to_char(ch, &af);

  /* Start cooldown */
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, SKILL_BASTION);

  USE_SWIFT_ACTION(ch);
}

/**
 * Radiant Aura - Paladin Divine Champion Tier 1 perk
 * Toggle ability that causes holy damage to undead in the room
 */
ACMD(do_radiantaura)
{
  PREREQ_NOT_NPC();

  /* Check if they have the perk */
  if (!has_paladin_radiant_aura(ch))
  {
    send_to_char(ch, "You don't know how to invoke a radiant aura!\r\n");
    return;
  }

  /* Toggle the aura on/off */
  if (affected_by_spell(ch, SKILL_RADIANT_AURA))
  {
    /* Turn off the aura */
    affect_from_char(ch, SKILL_RADIANT_AURA);
    send_to_char(ch, "\tYYour radiant aura fades as you release the divine light.\tn\r\n");
    act("\tY$n's radiant aura fades.\tn", FALSE, ch, 0, 0, TO_ROOM);
  }
  else
  {
    /* Turn on the aura */
    struct affected_type af;
    
    new_affect(&af);
    af.spell = SKILL_RADIANT_AURA;
    af.duration = -1; /* Permanent until toggled off */
    affect_to_char(ch, &af);
    
    send_to_char(ch, "\tYYou invoke your radiant aura, emanating holy light that burns the undead!\tn\r\n");
    act("\tY$n begins to emanate a radiant holy aura!\tn", FALSE, ch, 0, 0, TO_ROOM);
    
    /* Start the periodic damage event - triggers every 6 seconds (1 round) */
    if (!char_has_mud_event(ch, eRADIANT_AURA))
    {
      attach_mud_event(new_mud_event(eRADIANT_AURA, ch, NULL), 6 * PASSES_PER_SEC);
    }
  }
}

/**
 * Event handler for Radiant Aura periodic damage
 * Damages undead in the room every 6 seconds
 */
EVENTFUNC(event_radiant_aura)
{
  struct char_data *ch = NULL, *vict = NULL, *next_vict = NULL;
  struct mud_event_data *pMudEvent = NULL;
  int dam;
  bool pvp_enabled;

  if (event_obj == NULL)
    return 0;

  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  /* Check if character is still valid and playing */
  if (!ch || !IS_PLAYING(ch->desc))
    return 0;

  /* Check if aura is still active */
  if (!affected_by_spell(ch, SKILL_RADIANT_AURA))
    return 0;

  /* Check if paladin has PvP enabled */
  pvp_enabled = PRF_FLAGGED(ch, PRF_PVP);

  /* Damage all undead in the room */
  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (vict == ch || !CAN_SEE(ch, vict))
      continue;

    /* Must be undead */
    if (!IS_UNDEAD(vict))
      continue;

    /* Skip player undead and their charmies unless both have PVP enabled */
    if (!IS_NPC(vict))
    {
      /* Skip if paladin doesn't have PVP or victim doesn't have PVP */
      if (!pvp_enabled || !PRF_FLAGGED(vict, PRF_PVP))
        continue;
    }
    else if (vict->master && !IS_NPC(vict->master) && AFF_FLAGGED(vict, AFF_CHARM))
    {
      /* This is a charmed NPC - check if both paladin and master have PVP */
      if (!pvp_enabled || !PRF_FLAGGED(vict->master, PRF_PVP))
        continue;
    }

    /* Deal 3d6 holy damage */
    dam = dice(3, 6);
    
    act("\tY$n's radiant aura burns $N with holy fire!\tn", FALSE, ch, 0, vict, TO_NOTVICT);
    act("\tYYour radiant aura burns $N with holy fire!\tn", FALSE, ch, 0, vict, TO_CHAR);
    act("\tY$n's radiant aura burns you with holy fire!\tn", FALSE, ch, 0, vict, TO_VICT);
    
    damage(ch, vict, dam, SKILL_RADIANT_AURA, DAM_HOLY, FALSE);
  }

  /* Continue the event every 6 seconds */
  return 6 * PASSES_PER_SEC;
}

/* drow faerie fire engine */
void perform_faerie_fire(struct char_data *ch, struct char_data *vict)
{
  PREREQ_NOT_PEACEFUL_ROOM();

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  act("You briefly focus your innate magic and point at $N...",
      FALSE, ch, NULL, vict, TO_CHAR);
  act("$n focuses briefly then points at you...",
      FALSE, ch, NULL, vict, TO_VICT);
  act("$n focuses briefly then points at $N...",
      FALSE, ch, NULL, vict, TO_NOTVICT);

  // int call_magic(struct char_data *caster, struct char_data *cvict,
  // struct obj_data *ovict, int spellnum, int metamagic, int level, int casttype);
  call_magic(ch, vict, NULL, SPELL_FAERIE_FIRE, 0, GET_LEVEL(ch), CAST_SPELL);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SLA_FAERIE_FIRE);
}

/* dragonbite engine, just used for prisoner right now -zusuk */
int perform_dragonbite(struct char_data *ch, struct char_data *vict)
{
  int discipline_bonus = 0, diceOne = 0, diceTwo = 0;
  bool got_em = FALSE;

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return got_em;
  }

  /* maneuver bonus/penalty */
  if (!IS_NPC(ch) && compute_ability(ch, ABILITY_DISCIPLINE))
    discipline_bonus += compute_ability(ch, ABILITY_DISCIPLINE);
  if (!IS_NPC(vict) && compute_ability(vict, ABILITY_DISCIPLINE))
    discipline_bonus -= compute_ability(vict, ABILITY_DISCIPLINE);

  /* damage! */
  diceOne = GET_LEVEL(ch) + 4;
  diceTwo = GET_LEVEL(ch) + 4;

  if (combat_maneuver_check(ch, vict, COMBAT_MANEUVER_TYPE_KICK, 50) > 0)
  {
    /* damagee! */
    damage(ch, vict, dice(diceOne, diceTwo) + GET_STR(ch) + 4, SKILL_DRAGON_BITE, DAM_FORCE, FALSE);

    act("Your flesh is rended by a bite from $N!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e is rended by your bite at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n's flesh is rended by a bite from $N!", FALSE, vict, 0, ch, TO_NOTVICT);

    if (!savingthrow(ch, vict, SAVING_REFL, GET_STR_BONUS(vict), CAST_INNATE, GET_LEVEL(ch), NOSCHOOL) && rand_number(0, 2))
    {
      USE_FULL_ROUND_ACTION(vict);
      act("You are thrown off-balance by a bite from $N!", FALSE, vict, 0, ch, TO_CHAR);
      act("$e is thrown off-blance by your bite at $m!", FALSE, vict, 0, ch, TO_VICT);
      act("$n is thrown off-balance by a bite from $N!", FALSE, vict, 0, ch, TO_NOTVICT);
    }

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);

    got_em = TRUE;
  }
  else
  {
    damage(ch, vict, 0, SKILL_DRAGON_BITE, DAM_FORCE, FALSE);
    act("You dodge a vicious bite from $N!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e dodges to the side as you bite at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n dodges a vicious bite from $N!", FALSE, vict, 0, ch, TO_NOTVICT);
  }

  return got_em;
}

/* kick engine */
void perform_kick(struct char_data *ch, struct char_data *vict)
{
  int discipline_bonus = 0, diceOne = 0, diceTwo = 0;

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  PREREQ_NOT_PEACEFUL_ROOM();

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  if (IS_INCORPOREAL(vict) && !is_using_ghost_touch_weapon(ch))
  {
    act("$n sprawls completely through $N as $e tries to attack $M.", FALSE, ch, NULL, vict, TO_NOTVICT);
    act("You sprawl completely through $N as you try to attack!", FALSE, ch, NULL, vict, TO_CHAR);
    act("$n sprawls completely through you as $e tries to attack!", FALSE, ch, NULL, vict, TO_VICT);
    change_position(ch, POS_SITTING);
    return;
  }

  /* maneuver bonus/penalty */
  if (!IS_NPC(ch) && compute_ability(ch, ABILITY_DISCIPLINE))
    discipline_bonus += compute_ability(ch, ABILITY_DISCIPLINE);
  if (!IS_NPC(vict) && compute_ability(vict, ABILITY_DISCIPLINE))
    discipline_bonus -= compute_ability(vict, ABILITY_DISCIPLINE);

  /* monk damage? */
  compute_barehand_dam_dice(ch, &diceOne, &diceTwo);
  if (diceOne < 1)
    diceOne = 1;
  if (diceTwo < 2)
    diceTwo = 2;

  if (combat_maneuver_check(ch, vict, COMBAT_MANEUVER_TYPE_KICK, 0) > 0)
  {
    damage(ch, vict, dice(diceOne, diceTwo) + GET_STR_BONUS(ch), SKILL_KICK, DAM_FORCE, FALSE);
    if (!savingthrow(ch, vict, SAVING_REFL, GET_STR_BONUS(vict), CAST_INNATE, GET_LEVEL(ch), NOSCHOOL) && rand_number(0, 2))
    {
      if (vict->char_specials.recently_kicked == 0)
      {
        USE_MOVE_ACTION(vict);
        act("You are thrown off-balance by a kick from $N!", FALSE, vict, 0, ch, TO_CHAR);
        act("$e is thrown off-blance by your kick at $m!", FALSE, vict, 0, ch, TO_VICT);
        act("$n is thrown off-balance by a kick from $N!", FALSE, vict, 0, ch, TO_NOTVICT);
        vict->char_specials.recently_kicked = 5;
      }
    }

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);
  }
  else
    damage(ch, vict, 0, SKILL_KICK, DAM_FORCE, FALSE);
}

ACMDCHECK(can_impromptu)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_IMPROMPTU_SNEAK_ATTACK, "You don't know how to do this!\r\n");

  return CAN_CMD;
}

/* impromptu sneak attack engine */
void perform_impromptu(struct char_data *ch, struct char_data *vict)
{

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  PREREQ_NOT_PEACEFUL_ROOM();

  start_daily_use_cooldown(ch, FEAT_IMPROMPTU_SNEAK_ATTACK);

  /* As a free action once per day per rank of the feat, can attack as a sneak attack */
  send_to_char(ch, "IMPROMPTU:  ");
  hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 4, ATTACK_TYPE_PRIMARY_SNEAK);

  /* try for offhand */
  update_pos(vict);
  update_pos(ch);
  if (ch && vict && GET_EQ(ch, WEAR_WIELD_OFFHAND))
    hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 4, ATTACK_TYPE_OFFHAND_SNEAK);
}

/* seeker arrow engine */
void perform_seekerarrow(struct char_data *ch, struct char_data *vict)
{

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  PREREQ_NOT_PEACEFUL_ROOM();

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "Too cramped to safely fire an arrow here!\r\n");
    return;
  }

  start_daily_use_cooldown(ch, FEAT_SEEKER_ARROW);

  /* As a free action once per day per rank of the seeker arrow feat, the arcane
                   archer can fire an arrow that gets +20 to hit */
  send_to_char(ch, "Taking careful aim you fire:  ");
  hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 20, ATTACK_TYPE_RANGED);
}

ACMDCHECK(can_seekerarrow)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SEEKER_ARROW, "You don't know how to do this!\r\n");
  /* ranged attack requirement */
  ACMDCHECK_TEMPFAIL_IF(!can_fire_ammo(ch, TRUE), "You have to be using a ranged weapon with ammo ready to "
                                                  "fire in your ammo pouch to do this!\r\n");
  return CAN_CMD;
}

/* As a free action once per day per rank of the feat, perform a sneak attack with primary/offhand. */
ACMD(do_impromptu)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_CAN_FIGHT();

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_impromptu);
  PREREQ_HAS_USES(FEAT_IMPROMPTU_SNEAK_ATTACK, "You must recover the energy required to use another impromptu sneak attack.\r\n");

  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Launch a impromptu sneak attack at who?\r\n");
      return;
    }
  }

  perform_impromptu(ch, vict);
}

/* As a free action once per day per rank of the seeker arrow feat, the arcane
                   archer can fire an arrow that does not miss (+20 to hit). */
ACMD(do_seekerarrow)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_CAN_FIGHT();

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_seekerarrow);
  PREREQ_HAS_USES(FEAT_SEEKER_ARROW, "You must recover the arcane energy required to use another seeker arrow.\r\n");

  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Launch a seeker arrow at who?\r\n");
      return;
    }
  }

  perform_seekerarrow(ch, vict);
}

ACMDCHECK(can_faeriefire)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SLA_FAERIE_FIRE, "You don't know how to do this!\r\n");
  return CAN_CMD;
}

ACMD(do_faeriefire)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_faeriefire);
  PREREQ_HAS_USES(FEAT_SLA_FAERIE_FIRE, "You must recover before you can use faerie fire again.\r\n");

  one_argument(argument, arg, sizeof(arg));

  /* find the victim */
  vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);

  /* we have a disqualifier here due to action system */
  if (!FIGHTING(ch) && !vict)
  {
    send_to_char(ch, "Who do you want to faerie fire?\r\n");
    return;
  }
  if (vict == ch)
  {
    /* we allow this */
  }
  if (FIGHTING(ch) && !vict && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    vict = FIGHTING(ch);

  perform_faerie_fire(ch, vict);
}

ACMD(do_kick)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_CAN_FIGHT();

  PREREQ_NOT_NPC();

  one_argument(argument, arg, sizeof(arg));

  /* find the victim */
  vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);

  /* we have a disqualifier here due to action system */
  if (!FIGHTING(ch) && !vict)
  {
    send_to_char(ch, "Who do you want to kick?\r\n");
    return;
  }
  if (vict == ch)
  {
    send_to_char(ch, "You kick yourself.\r\n");
    return;
  }
  if (FIGHTING(ch) && !vict && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    vict = FIGHTING(ch);

  perform_kick(ch, vict);
}

ACMD(do_hitall)
{
  /* not used right now, whirlwind attack essentially replaces this */
  send_to_char(ch, "This skill has been removed, the whirlwind feat is meant to replace it.\r\n");
  return;

  int lag = 1;

  if (!MOB_CAN_FIGHT(ch))
    return;

  /* added this check because of abuse */
  if (IS_NPC(ch) || IS_FAMILIAR(ch) || IS_PET(ch))
  {
    return;
  }

  if ((IS_NPC(ch) || !HAS_FEAT(ch, FEAT_WHIRLWIND_ATTACK)) && (!IS_PET(ch) || IS_FAMILIAR(ch)))
  {
    send_to_char(ch, "But you do not know how to do that.\r\n");
    return;
  }

  PREREQ_NOT_PEACEFUL_ROOM();

  PREREQ_NOT_SINGLEFILE_ROOM();

  /* Data structure for hitall callback */
  struct hitall_data {
    int count;
    int lag;
  };

  /* Callback for hitall AoE attack */
  int hitall_callback(struct char_data *ch, struct char_data *tch, void *data) {
    struct hitall_data *hitall = (struct hitall_data *)data;
    
    if (!IS_NPC(tch) || IS_PET(tch))
      return 0;
    if (!CAN_SEE(ch, tch) && !IS_SET_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_MAGICDARK))
      return 0;

    hitall->count++;

    if (rand_number(0, 111) < 20 ||
        (IS_PET(ch) && rand_number(0, 101) > GET_LEVEL(ch)))
      hitall->lag++;

    hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, 0);
    return 1;
  }

  struct hitall_data hitall_data;
  hitall_data.count = 0;
  hitall_data.lag = 1;

  aoe_effect(ch, SKILL_HITALL, hitall_callback, &hitall_data);

  lag = 1 + hitall_data.lag / 3 + hitall_data.count / 4;
  if (lag > 4)
    lag = 4;

  USE_FULL_ROUND_ACTION(ch);
}

ACMDCHECK(can_circle)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SNEAK_ATTACK, "You have no idea how to do that (circle requires at least "
                                              "1 rank in sneak attack).\r\n");

  if (GET_RACE(ch) == RACE_TRELUX)
    ;
  else if (!GET_EQ(ch, WEAR_WIELD_1) && !GET_EQ(ch, WEAR_WIELD_OFFHAND) && !GET_EQ(ch, WEAR_WIELD_2H))
  {
    ACMD_ERRORMSG("You need to wield a weapon to make it a success.\r\n");
    return CANT_CMD_TEMP;
  }
  return CAN_CMD;
}

/* the ability to backstab a fighting opponent when not tanking  */
ACMD(do_circle)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_circle);

  if (!FIGHTING(ch))
  {
    send_to_char(ch, "You can only circle while in combat.\r\n");
    return;
  }

  if (is_tanking(ch))
  {
    send_to_char(ch, "You are too busy defending yourself to try that.\r\n");
    return;
  }

  one_argument(argument, buf, sizeof(buf));
  if (!*buf && FIGHTING(ch))
    vict = FIGHTING(ch);
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "Circle who?\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "Do not be ridiculous!\r\n");
    return;
  }

  if (!FIGHTING(vict))
  {
    send_to_char(ch, "You can only circle an opponent whom is in combat.\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict))
  {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    return;
  }
  if (AFF_FLAGGED(vict, AFF_AWARE) && AWAKE(vict))
  {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    return;
  }

  perform_backstab(ch, vict);
}

/*
Vhaerun:
    A rather useful type of bash for large humanoids to start a combat with.
 */
ACMD(do_bodyslam)
{
  struct char_data *vict;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();

  one_argument(argument, buf, sizeof(buf));

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You can't bodyslam while fighting!");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (!*buf)
  {
    send_to_char(ch, "Bodyslam who?\r\n");
    return;
  }

  if (!(vict = get_char_room_vis(ch, buf, NULL)))
  {
    send_to_char(ch, "Bodyslam who?\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

#if defined(CAMPAIGN_DL)
  if (GET_SIZE(vict) >= GET_SIZE(ch))
  {
    send_to_char(ch, "You can only bodyslam targets smaller than yourself.\r\n");
    return;
  }
#endif

  perform_knockdown(ch, vict, SKILL_BODYSLAM, true, true);
}

ACMDCHECK(can_headbutt)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_IMPROVED_UNARMED_STRIKE, "You have no idea how.\r\n");
  return CAN_CMD;
}

ACMD(do_headbutt)
{
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_headbutt);

  one_argument(argument, arg, sizeof(arg));

  PREREQ_NOT_PEACEFUL_ROOM();

  if (!*arg || !(vict = get_char_room_vis(ch, arg, NULL)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }

  if (!vict)
  {
    send_to_char(ch, "Headbutt who?\r\n");
    return;
  }

  perform_headbutt(ch, vict);
}

ACMDCHECK(can_sap)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SAP, "But you do not know how!\r\n");
  return CAN_CMD;
}

ACMD(do_sap)
{
  struct char_data *vict = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_sap);

  one_argument(argument, buf, sizeof(buf));

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You are too busy fighting to do this!\r\n");
    return;
  }

  if (!(vict = get_char_room_vis(ch, buf, NULL)))
  {
    send_to_char(ch, "Sap who?\r\n");
    return;
  }

  perform_sap(ch, vict);
}

ACMD(do_guard)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  if (IS_NPC(ch))
    return;

  if (affected_by_spell(ch, SKILL_RAGE))
  {
    send_to_char(ch, "All you want is BLOOD!\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_BLIND))
  {
    send_to_char(ch, "You can't see well enough to guard anyone!.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    if (GUARDING(ch))
      act("You are guarding $N", FALSE, ch, 0, ch->char_specials.guarding, TO_CHAR);
    else
      send_to_char(ch, "You are not guarding anyone.\r\n");
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg, NULL)))
  {
    send_to_char(ch, "Whom do you want to guard?\r\n");
    return;
  }

  if (IS_NPC(vict) && !IS_PET(vict))
  {
    send_to_char(ch, "Not even funny..\r\n");
    return;
  }

  if (GUARDING(ch))
  {
    act("$n stops guarding $N", FALSE, ch, 0, GUARDING(ch), TO_ROOM);
    act("You stop guarding $N", FALSE, ch, 0, GUARDING(ch), TO_CHAR);
  }

  GUARDING(ch) = vict;
  act("$n now guards $N", FALSE, ch, 0, vict, TO_ROOM);
  act("You now guard $N", FALSE, ch, 0, vict, TO_CHAR);
}

ACMDCHECK(can_dirtkick)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_DIRT_KICK, "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(AFF_FLAGGED(ch, AFF_IMMATERIAL), "You got no material feet to dirtkick with.\r\n");

  return CAN_CMD;
}

ACMD(do_dirtkick)
{
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_PEACEFUL_ROOM();
  PREREQ_CHECK(can_dirtkick);

  one_argument(argument, arg, sizeof(arg));

  if (!*arg || !(vict = get_char_room_vis(ch, arg, NULL)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }

  if (!vict)
  {
    send_to_char(ch, "Dirtkick who?\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  perform_dirtkick(ch, vict);
}

ACMDCHECK(can_springleap)
{
  ACMDCHECK_PERMFAIL_IF(!HAS_FEAT(ch, FEAT_SPRING_ATTACK) && !get_perk_rank(ch, PERK_FIGHTER_SPRING_ATTACK, CLASS_WARRIOR) && CLASS_LEVEL(ch, CLASS_MONK) < 5, "You have no idea how.\r\n");
  return CAN_CMD;
}

/*
 * Monk sit -> stand skill
 */
ACMD(do_springleap)
{
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  PREREQ_CAN_FIGHT();

  one_argument(argument, arg, sizeof(arg));

  PREREQ_NOT_NPC();
  PREREQ_CHECK(can_springleap);
  PREREQ_NOT_PEACEFUL_ROOM();

  /* character's position is a restriction/advantage of this skill */
  switch (GET_POS(ch))
  {
  case POS_RECLINING: /* fallthrough */ /* includes POS_CRAWLING */
  case POS_RESTING:                     /* fallthrough */
  case POS_SITTING:
    /* valid positions */
    break;
  default:
    send_to_char(ch, "You must be sitting or reclining to springleap.\r\n");
    return;
  }

  if (!*arg)
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }
  else
    vict = get_char_room_vis(ch, arg, NULL);

  if (!vict)
  {
    send_to_char(ch, "Springleap who?\r\n");
    return;
  }
  perform_springleap(ch, vict);
}

ACMDCHECK(can_shieldpunch)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_ARMOR_PROFICIENCY_SHIELD, "You are not proficient enough in the use of your shield to shieldpunch.\r\n");
  ACMDCHECK_TEMPFAIL_IF(!GET_EQ(ch, WEAR_SHIELD), "You need to wear a shield to be able to shieldpunch.\r\n");
  return CAN_CMD;
}

/* Shieldpunch :
 * Use your shield as a weapon, bashing out with it and doing a
 * small amount of damage.  The feat FEAT_IMPROVED_SHIELD_PUNCH allows
 * you to retain the AC of your shield when you perform a shield punch.
 */
ACMD(do_shieldpunch)
{
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();
  PREREQ_IN_POSITION(POS_SITTING, "You need to get on your feet to shieldpunch.\r\n");
  PREREQ_CHECK(can_shieldpunch);

  one_argument(argument, arg, sizeof(arg));

  PREREQ_NOT_PEACEFUL_ROOM();

  if (!*arg)
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }
  else
    vict = get_char_room_vis(ch, arg, NULL);

  perform_shieldpunch(ch, vict);
}

ACMDCHECK(can_shieldcharge)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SHIELD_CHARGE, "You are not proficient enough in the use of your shield to shieldcharge.\r\n");
  ACMDCHECK_TEMPFAIL_IF(!GET_EQ(ch, WEAR_SHIELD), "You need to wear a shield to be able to shieldcharge.\r\n");
  return CAN_CMD;
}

/* Shieldcharge :
 *
 * Use your shield as a weapon, bashing out with it and doing a
 * small amount of damage, also attempts to trip the opponent, if
 * possible.
 *
 * Requires FEAT_SHIELD_CHARGE
 */
ACMD(do_shieldcharge)
{
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();
  PREREQ_IN_POSITION(POS_SITTING, "You need to get on your feet to shieldcharge.\r\n");
  PREREQ_CHECK(can_shieldcharge);

  one_argument(argument, arg, sizeof(arg));

  PREREQ_NOT_PEACEFUL_ROOM();

  if (!*arg)
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }
  else
    vict = get_char_room_vis(ch, arg, NULL);

  perform_shieldcharge(ch, vict);
}

ACMDCHECK(can_shieldslam)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_ARMOR_PROFICIENCY_SHIELD, "You are not proficient enough in the use of your shield to shieldslam.\r\n");
  ACMDCHECK_PREREQ_HASFEAT(FEAT_SHIELD_SLAM, "You don't know how to do that.\r\n");
  ACMDCHECK_TEMPFAIL_IF(!GET_EQ(ch, WEAR_SHIELD), "You need to wear a shield to be able to shieldslam.\r\n");
  return CAN_CMD;
}

/* Shieldslam :
 *
 * Use your shield as a weapon, bashing out with it and doing a
 * small amount of damage, also daze the opponent on a failed fort save..
 *
 * Requires FEAT_SHIELD_SLAM
 *
 * (old comment)
 * Vhaerun:  A warrior skill to Stun !BASH mobs.
 */
ACMD(do_shieldslam)
{
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();
  PREREQ_IN_POSITION(POS_SITTING, "You need to get on your feet to shieldslam.\r\n");
  PREREQ_CHECK(can_shieldslam);

  one_argument(argument, arg, sizeof(arg));

  PREREQ_NOT_PEACEFUL_ROOM();

  if (!*arg)
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }
  else
    vict = get_char_room_vis(ch, arg, NULL);

  perform_shieldslam(ch, vict);
}

/* charging system for combat */
ACMD(do_charge)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'}, mob_keys[200];
  struct char_data *vict = NULL, *mob = NULL;
  bool found = false;
  int i = 0;

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_PEACEFUL_ROOM();
  PREREQ_IN_POSITION(POS_SITTING, "You need to stand to charge!\r\n");

  one_argument(argument, arg, sizeof(arg));

  // if (!*arg)
  // {
  //   if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
  //     vict = FIGHTING(ch);
  //   else
  //   {
  //     send_to_char(ch, "You are not in combat, nor have you specified a target to charge.\r\n");
  //     return;
  //   }
  // }
  if (!*arg)
  {
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_AUTOHIT))
    {
      if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
        vict = FIGHTING(ch);
      else
      {
        send_to_char(ch, "You are not in combat, nor have you specified a target to charge.\r\n");
        return;
      }
    }
    if (IS_NPC(ch) && (!ch->master  || !PRF_FLAGGED(ch->master, PRF_AUTOHIT)))
    {
      if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
        vict = FIGHTING(ch);
      else
      {
        send_to_char(ch, "You are not in combat, nor have you specified a target to charge.\r\n");
        return;
      }
    }
    else
    {
      // auto hit is enabled.  We're going to try to attack the first mob we can see in the room.
      if (IN_ROOM(ch) == NOWHERE)
      {
        return;
      }
      for (mob = world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room)
      {
        if (!IS_NPC(mob))
          continue;
        if (AFF_FLAGGED(mob, AFF_CHARM))
          continue;
        if (mob->master && mob->master == ch)
          continue;
        if (!CAN_SEE(ch, mob))
          continue;

        // ok we found one
        found = true;
        snprintf(mob_keys, sizeof(mob_keys), "%s", (mob)->player.name);
        for (i = 0; i < strlen(mob_keys); i++)
          if (mob_keys[i] == ' ')
            mob_keys[i] = '-';
        do_charge(ch, strdup(mob_keys), cmd, subcmd);
        
        return;
      }
      if (!found)
      {
        send_to_char(ch, "There are no eligible mobs here. Please specify your target instead.\r\n");
        return;
      }
    }
  }
  else
    vict = get_char_room_vis(ch, arg, NULL);

  if (!vict)
  {
    send_to_char(ch, "You are not in combat, nor have you specified a target to charge.\r\n");
    return;
  }

  perform_charge(ch, vict);
}

/* ranged-weapons, reload mechanic for slings, crossbows */

/* TODO:  improve this cheese :P  also combine autoreload mechanic with this */
ACMD(do_reload)
{
  struct obj_data *wielded = is_using_ranged_weapon(ch, FALSE);

  if (!wielded)
  {
    return;
  }

  if (!is_reloading_weapon(ch, wielded, FALSE))
  {
    return;
  }

  if (!has_ammo_in_pouch(ch, wielded, FALSE))
  {
    return;
  }

  /* passed all dummy checks, let's see if we have the action available we
   need to reload this weapon */

  switch (GET_OBJ_VAL(wielded, 0))
  {
  case WEAPON_TYPE_HEAVY_REP_XBOW:
  case WEAPON_TYPE_LIGHT_REP_XBOW:
  case WEAPON_TYPE_HEAVY_CROSSBOW:
    if (HAS_FEAT(ch, FEAT_RAPID_RELOAD))
    {
      if (is_action_available(ch, atMOVE, TRUE))
      {
        if (reload_weapon(ch, wielded, FALSE))
        {
          USE_MOVE_ACTION(ch); /* success! */
        }
        else
        {
          return;
        }
      }
      else
      {
        send_to_char(ch, "Reloading %s requires a move-action\r\n",
                     wielded->short_description);
        return;
      }
    }
    else if (is_action_available(ch, atSTANDARD, TRUE) &&
             is_action_available(ch, atMOVE, TRUE))
    {
      if (reload_weapon(ch, wielded, FALSE))
      {
        USE_FULL_ROUND_ACTION(ch); /* success! */
      }
      else
      {
        return;
      }
    }
    else
    {
      send_to_char(ch, "Reloading %s requires a full-round-action\r\n",
                   wielded->short_description);
      return;
    }

    break;
  case WEAPON_TYPE_HAND_CROSSBOW:
  case WEAPON_TYPE_LIGHT_CROSSBOW:
  case WEAPON_TYPE_SLING:
    if (HAS_FEAT(ch, FEAT_RAPID_RELOAD))
      reload_weapon(ch, wielded, FALSE);
    else if (is_action_available(ch, atMOVE, TRUE))
    {
      if (reload_weapon(ch, wielded, FALSE))
      {
        USE_MOVE_ACTION(ch); /* success! */
      }
      else
      {
        return;
      }
    }
    else
    {
      send_to_char(ch, "Reloading %s requires a move-action\r\n",
                   wielded->short_description);
      return;
    }

    break;
  default:
    send_to_char(ch, "%s does not require reloading!\r\n",
                 wielded->short_description);
    return;
  }

  send_to_char(ch, "You reload %s.\r\n", wielded->short_description);
  if (FIGHTING(ch))
    FIRING(ch) = TRUE;
  return;
}

/* ranged-weapons combat, archery
 * fire command, fires single arrow - checks can_fire_ammo()
 */
ACMD(do_fire)
{
  struct char_data *vict = NULL, *tch = NULL;
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  room_rnum room = NOWHERE;
  int direction = -1, original_loc = NOWHERE;

  PREREQ_NOT_NPC();

  PREREQ_NOT_PEACEFUL_ROOM();

  if (FIGHTING(ch) || FIRING(ch))
  {
    send_to_char(ch, "You are too busy fighting to try and fire right now!\r\n");
    return;
  }

  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  /* no 2nd argument?  target room has to be same room */
  if (!*arg2)
  {
    room = IN_ROOM(ch);
  }
  else
  {

    if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_FAR_SHOT))
    {
      send_to_char(ch, "You need the 'far shot' feat to shoot outside of your"
                       " immediate area!\r\n");
      return;
    }

    /* try to find target room */
    direction = search_block(arg2, dirs, FALSE);
    if (direction < 0)
    {
      send_to_char(ch, "That is not a direction!\r\n");
      return;
    }
    if (!CAN_GO(ch, direction))
    {
      send_to_char(ch, "You can't fire in that direction!\r\n");
      return;
    }
    room = EXIT(ch, direction)->to_room;
  }

  /* since we could possible no longer be in room, check if combat is ok
   in new room */
  if (ROOM_FLAGGED(room, ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  /* no arguments?  no go! */
  if (!*arg1)
  {
    send_to_char(ch, "You need to select a target!\r\n");
    return;
  }

  /* a location has been found. */
  original_loc = IN_ROOM(ch);
  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(room), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[room].coords[0];
    Y_LOC(ch) = world[room].coords[1];
  }

  char_to_room(ch, room);
  vict = get_char_room_vis(ch, arg1, NULL);

  /* check if the char is still there */
  if (IN_ROOM(ch) == room)
  {
    char_from_room(ch);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(original_loc), ZONE_WILDERNESS))
    {
      X_LOC(ch) = world[original_loc].coords[0];
      Y_LOC(ch) = world[original_loc].coords[1];
    }

    char_to_room(ch, original_loc);
  }

  if (!vict)
  {
    send_to_char(ch, "Fire at who?\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  /* if target is group member, we presume you meant to assist */
  if (GROUP(ch) && room == IN_ROOM(ch))
  {
    /* Beginner's Note: Reset simple_list iterator before use to prevent
     * cross-contamination from previous iterations. Without this reset,
     * if simple_list was used elsewhere and not completed, it would
     * continue from where it left off instead of starting fresh. */
    simple_list(NULL);
    
    while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) !=
           NULL)
    {
      if (IN_ROOM(tch) != IN_ROOM(vict))
        continue;
      if (vict == tch)
      {
        vict = FIGHTING(vict);
        break;
      }
    }
  }

  /* maybe its your pet?  so assist */
  if (vict && IS_PET(vict) && vict->master == ch && room == IN_ROOM(ch))
    vict = FIGHTING(vict);

  if (can_fire_ammo(ch, FALSE))
  {

    if (ch && vict && IN_ROOM(ch) != IN_ROOM(vict))
    {
      hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, 2); // 2 in last arg indicates ranged
      /* don't forget to remove the fight event! */
      if (char_has_mud_event(ch, eCOMBAT_ROUND))
      {
        event_cancel_specific(ch, eCOMBAT_ROUND);
      }

      stop_fighting(ch);
      USE_STANDARD_ACTION(ch);
    }
    else
    {
      if (FIGHTING(ch))
        USE_MOVE_ACTION(ch);
      hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, 2); // 2 in last arg indicates ranged
      FIRING(ch) = TRUE;
    }
  }
  else
  {
    /* arrived here?  can't fire, silent-mode from can-fire sent a message why */
  }
}

/* primarily useful for those with eldritch blast,
 * auto-assists players by auto-casting eldritch blast
 * when possible */
ACMD(do_assistblast)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL, *tch = NULL;

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();
  PREREQ_NOT_PEACEFUL_ROOM();

  one_argument(argument, arg, sizeof(arg));

  if (IN_ROOM(ch) == NOWHERE)
    return;

  if (FIGHTING(ch) && GET_ELDRITCH_SHAPE(ch) == WARLOCK_ELDRITCH_SPEAR)
  {
    send_to_char(ch, "You are too busy fighting!\r\n");
    return;
  }

  if (!*arg)
  {
    send_to_char(ch, "Eldritch blast who?\r\n");
    return;
  }

  if (!GROUP(ch))
  {
    send_to_char(ch, "But you are not a member of a group, this is an assist command for eldritch blast!\r\n");
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg, NULL)))
  {
    send_to_char(ch, "There is no one by that description here.\r\n");
    return;
  }

  if (IN_ROOM(vict) == NOWHERE)
    return;

  /* Beginner's Note: Reset simple_list iterator before use to prevent
   * cross-contamination from previous iterations. Without this reset,
   * if simple_list was used elsewhere and not completed, it would
   * continue from where it left off instead of starting fresh. */
  simple_list(NULL);
  
  while ((tch = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
  {
    if (IN_ROOM(tch) != IN_ROOM(vict))
      continue;
    if (vict == tch)
    {
      vict = FIGHTING(vict);
      break;
    }
  }

  if (!vict)
  {
    send_to_char(ch, "Eldritch blast at who?\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (GET_ELDRITCH_SHAPE(ch) != WARLOCK_HIDEOUS_BLOW)
  {
    cast_spell(ch, vict, NULL, WARLOCK_ELDRITCH_BLAST, 0);
  }
  else
  {
    send_to_char(ch, "Your strikes will now flow with eldritch energy.\r\n");
  }
  BLASTING(ch) = TRUE;
}

/* ranged-weapons combat, archery, a sort of ranged combat assist command
 * autofire command, fires single arrow - checks can_fire_ammo()
 * sets FIRING() */
ACMD(do_autofire)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL, *tch = NULL;

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_NPC();
  PREREQ_NOT_PEACEFUL_ROOM();

  one_argument(argument, arg, sizeof(arg));

  if (FIGHTING(ch) || FIRING(ch))
  {
    send_to_char(ch, "You are too busy fighting to try and fire right now!\r\n");
    return;
  }

  if (!*arg)
  {
    send_to_char(ch, "Fire at who?\r\n");
    return;
  }

  if (!GROUP(ch))
  {
    send_to_char(ch, "But you are not a member of a group, this is an assist command for archery!\r\n");
    return;
  }

  vict = get_char_room_vis(ch, arg, NULL);

  /* Beginner's Note: Reset simple_list iterator before use to prevent
   * cross-contamination from previous iterations. Without this reset,
   * if simple_list was used elsewhere and not completed, it would
   * continue from where it left off instead of starting fresh. */
  simple_list(NULL);
  
  while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) !=
         NULL)
  {
    if (IN_ROOM(tch) != IN_ROOM(vict))
      continue;
    if (vict == tch)
    {
      vict = FIGHTING(vict);
      break;
    }
  }

  if (!vict)
  {
    send_to_char(ch, "Fire at who?\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (can_fire_ammo(ch, FALSE))
  {
    hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, 2); // 2 in last arg indicates ranged
    FIRING(ch) = TRUE;
    USE_MOVE_ACTION(ch);
  }
  else
  {
    /* arrived here?  can't fire, message why was sent via silent-mode */
  }
}

/* function used to gather up all the ammo in the room/corpses-in-room */
int perform_collect(struct char_data *ch, bool silent)
{
  struct obj_data *ammo_pouch = GET_EQ(ch, WEAR_AMMO_POUCH);
  struct obj_data *obj = NULL;
  struct obj_data *nobj = NULL;
  struct obj_data *cobj = NULL;
  struct obj_data *next_obj = NULL;
  int ammo = 0;
  bool fit = TRUE;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (!ammo_pouch)
  {
    return 0;
  }

  for (obj = world[ch->in_room].contents; obj; obj = nobj)
  {
    nobj = obj->next_content;

    /*debug*/
    // act("$p", FALSE, ch, obj, 0, TO_CHAR);

    /* checking corpse for ammo first */
    if (IS_CORPSE(obj))
    {
      for (cobj = obj->contains; cobj; cobj = next_obj)
      {
        next_obj = cobj->next_content;
        if (GET_OBJ_TYPE(cobj) == ITEM_MISSILE &&
            MISSILE_ID(cobj) == GET_IDNUM(ch))
        {
          if (num_obj_in_obj(ammo_pouch) < GET_OBJ_VAL(ammo_pouch, 0))
          {
            obj_from_obj(cobj);
            obj_to_obj(cobj, ammo_pouch);
            ammo++;
            if (!silent)
              act("You get $p.", FALSE, ch, cobj, 0, TO_CHAR);
          }
          else
          {
            fit = FALSE;
            break;
          }
        }
      }
    }
    /* checking room for ammo */
    else if (GET_OBJ_TYPE(obj) == ITEM_MISSILE &&
             MISSILE_ID(obj) == GET_IDNUM(ch))
    {
      if (num_obj_in_obj(ammo_pouch) < GET_OBJ_VAL(ammo_pouch, 0))
      {
        obj_from_room(obj);
        obj_to_obj(obj, ammo_pouch);
        ammo++;
        if (!silent)
          act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
      }
      else
      {
        fit = FALSE;
        break;
      }
    }

  } /*for loop*/

  if (ammo && !silent)
  {
    snprintf(buf, sizeof(buf), "You collected ammo:  %d.\r\n", ammo);
    send_to_char(ch, "%s", buf);
    act("$n gathers $s ammunition.", FALSE, ch, 0, 0, TO_ROOM);
  }

  if (!fit && !silent)
    send_to_char(ch, "There are still some of your ammunition laying around that does not fit into your currently"
                     " equipped ammo pouch.\r\n");

  return ammo;
}

/* function used to gather up all the ammo in the room/corpses-in-room */
ACMD(do_collect)
{
  struct obj_data *ammo_pouch = GET_EQ(ch, WEAR_AMMO_POUCH);

  if (!ammo_pouch)
  {
    send_to_char(ch, "But you don't have an ammo pouch to collect to.\r\n");
    return;
  }

  perform_collect(ch, FALSE);
}

/*
Feinting is a standard action. To feint, make a Bluff skill check. The DC of
this check is equal to 10 + your opponent's base attack bonus + your opponent's
Wisdom modifier. If your opponent is trained in Sense Motive, the DC is instead
equal to 10 + your opponent's Sense Motive bonus, if higher. If successful, the
next melee attack you make against the target does not allow him to use his
Dexterity bonus to AC (if any). This attack must be made on or before your next
turn.

When feinting against a non-humanoid you take a –4 penalty. Against a creature
of animal Intelligence (1 or 2), you take a –8 penalty. Against a creature
lacking an Intelligence score, it's impossible. Feinting in combat does not
provoke attacks of opportunity.

Feinting as a Move Action:
With the Improved Feint feat, you can attempt a feint as a move action.
 */
int perform_feint(struct char_data *ch, struct char_data *vict)
{
  int bluff_skill_check = 0;
  int dc_bab_wisdom = 0;
  int dc_sense_motive = 0;
  int final_dc = 0;
  struct affected_type af;

  if (!ch || !vict)
    return -1;
  if (ch == vict)
  {
    send_to_char(ch, "You feint yourself mightily.\r\n");
    return -1;
  }
  if (!CAN_SEE(ch, vict))
  {
    send_to_char(ch, "You can't see well enough to attempt that.\r\n");
    return -1;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return -1;
  }

  /* calculate our final bluff skill check (feint attempt) */
  bluff_skill_check = d20(ch) + compute_ability(ch, ABILITY_BLUFF) + (HAS_FEAT(ch, FEAT_IMPROVED_FEINT) ? 4 : 0);
  if (IS_NPC(vict) && GET_NPC_RACE(vict) != RACE_TYPE_HUMANOID)
  {
    if (HAS_FEAT(ch, FEAT_IMPROVED_FEINT))
      bluff_skill_check -= 2;
    else
      bluff_skill_check -= 4;
  }

  if (GET_INT(vict) <= 2)
  {
    if (HAS_FEAT(ch, FEAT_IMPROVED_FEINT))
      bluff_skill_check -= 4;
    else
      bluff_skill_check -= 8;
  }

  /* calculate the defense (DC) */
  dc_bab_wisdom = 10 + BAB(vict) + GET_WIS_BONUS(vict);
  if (!IS_NPC(vict))
    dc_sense_motive = 10 + compute_ability(vict, ABILITY_SENSE_MOTIVE);
  final_dc = MAX(dc_bab_wisdom, dc_sense_motive);

  if (bluff_skill_check >= final_dc)
  { /* success */
    act("\tyYou feint, throwing $N off-balance!\tn", FALSE, ch, NULL, vict, TO_CHAR);
    act("\ty$n feints at you successfully, throwing you off balance!\tn", FALSE, ch, NULL, vict, TO_VICT);
    act("\ty$n successfully feints $N!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
    new_affect(&af);
    af.spell = SKILL_FEINT;
    af.duration = 10;
    SET_BIT_AR(af.bitvector, AFF_FEINTED);
    affect_to_char(vict, &af);
  }
  else
  { /* failure */
    act("\tyYour attempt to feint $N fails!\tn", FALSE, ch, NULL, vict, TO_CHAR);
    act("\ty$n attempt to feint you fails!\tn", FALSE, ch, NULL, vict, TO_VICT);
    act("\ty$n fails to feint $N!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
  }

  USE_SWIFT_ACTION(ch);

  if (vict != ch)
  {
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, vict);
    if (GET_POS(vict) > POS_STUNNED && (FIGHTING(vict) == NULL))
    {
      set_fighting(vict, ch);
    }
  }

  return 0;
}

ACMD(do_feint)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_NOT_NPC();
  PREREQ_NOT_PEACEFUL_ROOM();
  PREREQ_IN_POSITION(POS_SITTING, "You need to stand to feint!\r\n");

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }
  else
    vict = get_char_room_vis(ch, arg, NULL);

  if (!vict)
  {
    send_to_char(ch, "Feint who?\r\n");
    return;
  }
  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (affected_by_spell(vict, SKILL_FEINT))
  {
    send_to_char(ch, "Your opponent is already off balance!\r\n");
    return;
  }

  perform_feint(ch, vict);
}

/* disarm mechanic */
int perform_disarm(struct char_data *ch, struct char_data *vict, int mod)
{
  int pos;
  struct obj_data *wielded = NULL;

  if (!vict)
  {
    send_to_char(ch, "You can only try to disarm the opponent you are fighting.\r\n");
    return -1;
  }

  if (ch == vict)
  {
    send_to_char(ch, "You can just remove your weapon instead.\r\n");
    return -1;
  }

  if (!CAN_SEE(ch, vict))
  {
    send_to_char(ch, "You can't see well enough to attempt that.\r\n");
    return -1;
  }

  // Determine what we are going to disarm. Check for a 2H weapon first.
  if (GET_EQ(vict, WEAR_WIELD_2H))
  {
    wielded = GET_EQ(vict, WEAR_WIELD_2H);
    pos = WEAR_WIELD_2H;
  }
  else
  {
    // Check for a 1h weapon, primary hand.
    wielded = GET_EQ(vict, WEAR_WIELD_1);
    pos = WEAR_WIELD_1;
  }
  //  If neither of those was successful, check for a 1H weapon in the secondary hand.
  if (!wielded)
  {
    wielded = GET_EQ(vict, WEAR_WIELD_OFFHAND);
    pos = WEAR_WIELD_OFFHAND;
  }

  // If wielded is NULL, then the victim is weilding no weapon!
  if (!wielded)
  {
    act("But $N is not wielding anything.", FALSE, ch, 0, vict, TO_CHAR);
    return -1;
  }

  // Trigger AOO, save damage for modifying the CMD roll.
  if (!HAS_FEAT(ch, FEAT_IMPROVED_DISARM) && !has_perk(ch, PERK_FIGHTER_IMPROVED_DISARM))
    mod -= attack_of_opportunity(vict, ch, 0);

  // Check to see what we are wielding.
  if ((GET_EQ(ch, WEAR_WIELD_2H) == NULL) &&
      (GET_EQ(ch, WEAR_WIELD_1) == NULL) &&
      (GET_EQ(ch, WEAR_WIELD_OFFHAND) == NULL) &&
      (!HAS_FEAT(ch, FEAT_IMPROVED_UNARMED_STRIKE)))
  {
    // Trying an unarmed disarm, -4.
    mod -= 4;
  }

  // whips give a +5 bonus to the check
  if ((GET_EQ(ch, WEAR_WIELD_1) && GET_WEAPON_TYPE(GET_EQ(ch, WEAR_WIELD_1)) == WEAPON_TYPE_WHIP) ||
      (GET_EQ(ch, WEAR_WIELD_2H) && GET_WEAPON_TYPE(GET_EQ(ch, WEAR_WIELD_2H)) == WEAPON_TYPE_WHIP) ||
      (GET_EQ(ch, WEAR_WIELD_OFFHAND) && GET_WEAPON_TYPE(GET_EQ(ch, WEAR_WIELD_OFFHAND)) == WEAPON_TYPE_WHIP))
  {
    mod += 5;
  }
  
  /* Tactical Fighter perk: Improved Disarm */
  if (has_perk(ch, PERK_FIGHTER_IMPROVED_DISARM))
    mod += 4;

  int result = combat_maneuver_check(ch, vict, COMBAT_MANEUVER_TYPE_DISARM, mod);
  if (result > 0 && !HAS_FEAT(vict, FEAT_WEAPON_MASTERY))
  { /* success! */
    act("$n disarms $N of $S $p.",
        FALSE, ch, wielded, vict, TO_NOTVICT);
    act("You manage to knock $p out of $N's hands.",
        FALSE, ch, wielded, vict, TO_CHAR);
    act("$n disarms you, $p goes flying!",
        FALSE, ch, wielded, vict, TO_VICT | TO_SLEEP);
    if (HAS_FEAT(ch, FEAT_GREATER_DISARM))
      obj_to_room(unequip_char(vict, pos), vict->in_room);
    else
      obj_to_char(unequip_char(vict, pos), vict);

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);
  }
  else if (result <= -10)
  { /* critical failure */
    /* have to check if we have a weapon to lose */
    if (GET_EQ(ch, WEAR_WIELD_2H))
    {
      wielded = GET_EQ(ch, WEAR_WIELD_2H);
      pos = WEAR_WIELD_2H;
    }
    else
    { /* check 1h weapon, primary hand */
      wielded = GET_EQ(ch, WEAR_WIELD_1);
      pos = WEAR_WIELD_1;
    }
    /* If neither successful, check for a 1H weapon in the secondary hand. */
    if (!wielded)
    {
      wielded = GET_EQ(ch, WEAR_WIELD_OFFHAND);
      pos = WEAR_WIELD_OFFHAND;
    }
    if (!wielded)
    { /* not wielding */
      act("$n attempt to disarm $N, but fails terribly.",
          FALSE, ch, NULL, vict, TO_NOTVICT);
      act("You fail terribly in your attempt to disarm $N.",
          FALSE, ch, NULL, vict, TO_CHAR);
      act("$n attempt to disarm you, but fails terribly.",
          FALSE, ch, NULL, vict, TO_VICT | TO_SLEEP);
    }
    else
    {
      act("$n fails the disarm maneuver on $N, stumbles and drops $s $p.",
          FALSE, ch, wielded, vict, TO_NOTVICT);
      act("You drop $p in your attempt to disarm $N!",
          FALSE, ch, wielded, vict, TO_CHAR);
      act("$n fails the disarm maneuver on you, stumbles and drops $s $p.",
          FALSE, ch, wielded, vict, TO_VICT);
      obj_to_room(unequip_char(ch, pos), ch->in_room);
    }
  }
  else
  { /* failure */
    act("$n fails to disarm $N of $S $p.",
        FALSE, ch, wielded, vict, TO_NOTVICT);
    act("You fail to disarm $p out of $N's hands.",
        FALSE, ch, wielded, vict, TO_CHAR);
    act("$n fails to disarm you of your $p.",
        FALSE, ch, wielded, vict, TO_VICT);
  }

  return 0;
}

/* entry point for disarm combat maneuver */
ACMD(do_disarm)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int mod = 0;
  struct char_data *vict = NULL;

  PREREQ_NOT_NPC();
  PREREQ_NOT_PEACEFUL_ROOM();
  PREREQ_IN_POSITION(POS_SITTING, "You need to stand to disarm!\r\n");

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }
  else
    vict = get_char_room_vis(ch, arg, NULL);

  if (!vict)
  {
    send_to_char(ch, "Disarm who?\r\n");
    return;
  }
  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  perform_disarm(ch, vict, mod);
}

/* sunder mechanic - opposed attack rolls to break weapon/shield */
int perform_sunder(struct char_data *ch, struct char_data *vict, int mod)
{
  int pos;
  struct obj_data *target_item = NULL;
  int attacker_roll = 0, defender_roll = 0;
  int size_mod = 0;
  struct obj_data *attacker_weapon = NULL;

  if (!vict)
  {
    send_to_char(ch, "You can only try to sunder the weapon or shield of the opponent you are fighting.\r\n");
    return -1;
  }

  if (ch == vict)
  {
    send_to_char(ch, "You can't sunder your own equipment!\r\n");
    return -1;
  }

  if (!CAN_SEE(ch, vict))
  {
    send_to_char(ch, "You can't see well enough to attempt that.\r\n");
    return -1;
  }

  /* Determine what weapon/shield we're attacking */
  if (GET_EQ(vict, WEAR_SHIELD))
  {
    target_item = GET_EQ(vict, WEAR_SHIELD);
    pos = WEAR_SHIELD;
  }
  else if (GET_EQ(vict, WEAR_WIELD_2H))
  {
    target_item = GET_EQ(vict, WEAR_WIELD_2H);
    pos = WEAR_WIELD_2H;
  }
  else if (GET_EQ(vict, WEAR_WIELD_1))
  {
    target_item = GET_EQ(vict, WEAR_WIELD_1);
    pos = WEAR_WIELD_1;
  }
  else if (GET_EQ(vict, WEAR_WIELD_OFFHAND))
  {
    target_item = GET_EQ(vict, WEAR_WIELD_OFFHAND);
    pos = WEAR_WIELD_OFFHAND;
  }

  if (!target_item)
  {
    act("But $N is not wielding a weapon or shield.", FALSE, ch, 0, vict, TO_CHAR);
    return -1;
  }

  /* Check if attacker has a valid weapon (slashing or bludgeoning) */
  if (GET_EQ(ch, WEAR_WIELD_2H))
    attacker_weapon = GET_EQ(ch, WEAR_WIELD_2H);
  else if (GET_EQ(ch, WEAR_WIELD_1))
    attacker_weapon = GET_EQ(ch, WEAR_WIELD_1);

  if (!attacker_weapon)
  {
    send_to_char(ch, "You need to wield a slashing or bludgeoning weapon to sunder.\r\n");
    return -1;
  }

  /* Check weapon type - must be slashing or bludgeoning */
  if (!IS_BLADE(attacker_weapon) && !IS_BLUNT(attacker_weapon))
  {
    send_to_char(ch, "You need to wield a slashing or bludgeoning weapon to sunder.\r\n");
    return -1;
  }

  /* Trigger AoO unless they have Improved Sunder feat or perk */
  if (!HAS_FEAT(ch, FEAT_IMPROVED_SUNDER) && !has_perk(ch, PERK_FIGHTER_IMPROVED_SUNDER))
    mod -= attack_of_opportunity(vict, ch, 0);

  /* Make opposed attack rolls */
  attacker_roll = d20(ch);
  defender_roll = d20(vict);

  /* Add attack bonuses */
  attacker_roll += compute_attack_bonus(ch, vict, ATTACK_TYPE_PRIMARY);
  defender_roll += compute_attack_bonus(vict, ch, ATTACK_TYPE_PRIMARY);

  /* Two-handed weapon bonus: +4 for attacker */
  if (GET_EQ(ch, WEAR_WIELD_2H))
    attacker_roll += 4;
  
  /* Two-handed weapon bonus: +4 for defender */
  if (pos == WEAR_WIELD_2H)
    defender_roll += 4;

  /* Light weapon penalty: -4 */
  if (attacker_weapon && GET_OBJ_SIZE(attacker_weapon) < GET_SIZE(ch))
    attacker_roll -= 4;
  
  if (target_item && GET_OBJ_TYPE(target_item) == ITEM_WEAPON && 
      GET_OBJ_SIZE(target_item) < GET_SIZE(vict))
    defender_roll -= 4;

  /* Size difference bonus: +4 per size category */
  size_mod = (GET_SIZE(ch) - GET_SIZE(vict)) * 4;
  attacker_roll += size_mod;

  /* Add any modifiers passed in */
  attacker_roll += mod;

  /* Improved Sunder perk bonus */
  if (has_perk(ch, PERK_FIGHTER_IMPROVED_SUNDER))
    attacker_roll += 4;

  /* Compare rolls */
  if (attacker_roll > defender_roll)
  {
    /* Success! */
    act("$n strikes $N's $p, knocking it from $S grip!",
        FALSE, ch, target_item, vict, TO_NOTVICT);
    act("You strike $N's $p and knock it from $S hands!",
        FALSE, ch, target_item, vict, TO_CHAR);
    act("$n strikes your $p, knocking it from your grip!",
        FALSE, ch, target_item, vict, TO_VICT | TO_SLEEP);

    /* Unequip and move to inventory */
    obj_to_char(unequip_char(vict, pos), vict);

    /* Apply 4 second lag to victim */
    WAIT_STATE(vict, 4 * PASSES_PER_SEC);

    /* fire-shield, etc check */
    damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);

    return 1;
  }
  else
  {
    /* Failure */
    act("$n fails to sunder $N's $p.",
        FALSE, ch, target_item, vict, TO_NOTVICT);
    act("You fail to sunder $N's $p.",
        FALSE, ch, target_item, vict, TO_CHAR);
    act("$n fails to sunder your $p.",
        FALSE, ch, target_item, vict, TO_VICT);

    return 0;
  }
}

/* entry point for sunder combat maneuver */
ACMD(do_sunder)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int mod = 0;
  struct char_data *vict = NULL;

  PREREQ_NOT_NPC();
  PREREQ_NOT_PEACEFUL_ROOM();
  PREREQ_IN_POSITION(POS_SITTING, "You need to stand to sunder!\r\n");

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }
  else
    vict = get_char_room_vis(ch, arg, NULL);

  if (!vict)
  {
    send_to_char(ch, "Sunder whose equipment?\r\n");
    return;
  }
  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  perform_sunder(ch, vict, mod);
}

/* do_process_attack()
 *
 * This is the Luminari Attack engine - All attack actions go through this function.
 * When an attack is processed, it is sent to the attack queue and a hit command is generated
 * based on the character's combat status and any supplied arguments.
 *
 * This is a KEY part of the Luminari Action system.
 */
ACMD(do_process_attack)
{

  bool fail = FALSE;
  struct attack_action_data *attack = NULL;
  const char *fail_msg = "You have no idea how to";

  /* Check if ch can perform the attack... */
  switch (subcmd)
  {
  case AA_HEADBUTT:
    if (!HAS_FEAT(ch, FEAT_IMPROVED_UNARMED_STRIKE))
    {
      fail = TRUE;
    }
    break;
  }

  if (fail == TRUE)
  {
    send_to_char(ch, "%s %s.\r\n", fail_msg, complete_cmd_info[cmd].command);
    return;
  }

  CREATE(attack, struct attack_action_data, 1);
  attack->command = cmd;
  attack->attack_type = subcmd;
  attack->argument = strdup(argument);

  enqueue_attack(GET_ATTACK_QUEUE(ch), attack);

  if (FIGHTING(ch) || (!FIGHTING(ch) && (strcmp(argument, "") == 0)))
    send_to_char(ch, "Attack queued.\r\n");

  if (!FIGHTING(ch) && (strcmp(argument, "") != 0))
  {
    do_hit(ch, argument, cmd, subcmd);
  }
}

/* engine for the lich touch ability
   ch -> individual executing skill
   vict -> target of skill
   return TRUE if successful in executing
   return FALSE if attack failed to execute */
bool perform_lichtouch(struct char_data *ch, struct char_data *vict)
{

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");

    if (!IS_POWERFUL_BEING(ch))
      return FALSE;
  }

  /* compute amount of points heal vs damage */
  int amount = 10 + GET_INT_BONUS(ch) + dice(GET_LEVEL(ch), 4);
  int dc = 10 + GET_INT_BONUS(ch) + (GET_LEVEL(ch) / 2);
  int prob = 0;

  if (IS_POWERFUL_BEING(ch))
  {
    amount *= 2;
    amount += (GET_LEVEL(ch) - 30) * 50;
    dc += (GET_LEVEL(ch) - 30) * 4;
  }

  /* this skill will heal undead */
  if (IS_UNDEAD(vict) || IS_LICH(vict))
  {
    amount *= 2;

    if (ch == vict)
    {
      act("\tWYou focus necromantic power inward, and the surge of negative energy heals you.\tn", FALSE, ch, 0, vict, TO_CHAR);
      act("$n \tWglows with necromantic negative energy as wounds heal.\tn", FALSE, ch, 0, vict, TO_NOTVICT);
    }
    else
    {
      act("\tWYou reach out and touch $N with necromantic power, and the surge of negative energy heals $M.\tn", FALSE, ch, 0, vict, TO_CHAR);
      act("$n \tWreaches out and touches you with necromantic power, and the surge of negative energy heals you.\tn", FALSE, ch, 0, vict, TO_VICT);
      act("$n \tWreaches out and touches $N with necromantic power, and the surge of negative energy heals $M.\tn", FALSE, ch, 0, vict, TO_NOTVICT);
    }

    process_healing(ch, vict, RACIAL_LICH_TOUCH, amount, 0, 0);
    return TRUE;
  }

  /* victim should be damaged! */

  if (MOB_FLAGGED(vict, MOB_NOKILL))
  {
    send_to_char(ch, "This mob is protected.\r\n");
    return FALSE;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return FALSE;
  }

  if (!pvp_ok(ch, vict, true))
    return FALSE;

  act("\tLYou reach out and touch $N with \tRnegative energy\tL, and $E wilts before you.\tn", FALSE, ch, 0, vict, TO_CHAR);
  act("$n \tLreaches out and touches you with \tRnegative energy\tL, causing you to wilt before $m.\tn", FALSE, ch, 0, vict, TO_VICT);
  act("$n \tLreaches out and touches $N with \tRnegative energy\tL, causing $M to wilt!\tn", FALSE, ch, 0, vict, TO_NOTVICT);

  if (HAS_EVOLUTION(vict, EVOLUTION_UNDEAD_APPEARANCE))
    prob -= get_evolution_appearance_save_bonus(vict);

  /* paralysis - fortitude save */
  if (!savingthrow(ch, vict, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
  {
    if (!paralysis_immunity(vict))
    {
      struct affected_type af;

      new_affect(&af);

      af.spell = RACIAL_LICH_TOUCH;
      SET_BIT_AR(af.bitvector, AFF_PARALYZED);
      af.duration = 2;

      affect_join(vict, &af, TRUE, FALSE, FALSE, FALSE);

      act("$n \tWglows with \tLblack energy as $s touch \tWparalyzes $N!\tn",
          FALSE, ch, NULL, vict, TO_NOTVICT);
      act("You \tWglow with \tLblack energy as you \tWparalyze $N with your touch!\tn",
          FALSE, ch, NULL, vict, TO_CHAR);
      act("$n \tWglows with \tLblack energy as $s touch \tWparalyzes you!\tn",
          FALSE, ch, NULL, vict, TO_VICT | TO_SLEEP);
    }
  }

  damage(ch, vict, amount, RACIAL_LICH_TOUCH, DAM_NEGATIVE, FALSE);

  /* fire-shield, etc check */
  damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);

  return TRUE;
}

/* the potent touch of a lich */
ACMD(do_lichtouch)
{
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!IS_LICH(ch))
  {
    send_to_char(ch, "You do not have that ability!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_LICH_TOUCH)) == 0)
  {
    send_to_char(ch, "You must recover the profane energy required to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  if (!is_action_available(ch, atSTANDARD, TRUE))
  {
    send_to_char(ch, "Lichtouch requires a standard action available to use.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Lich touch who?\r\n");
      return;
    }
  }

  if (perform_lichtouch(ch, vict))
  {
    if (!IS_NPC(ch))
      start_daily_use_cooldown(ch, FEAT_LICH_TOUCH);

    USE_STANDARD_ACTION(ch);
  }
}

bool perform_lichfear(struct char_data *ch)
{

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE))
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return FALSE;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return FALSE;
  }

  act("You raise your countenance to cause fear!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n raises $s countenance to cause fear!", FALSE, ch, 0, 0, TO_ROOM);

  /* Callback for lich fear AoE effect */
  int lich_fear_callback(struct char_data *ch, struct char_data *tch, void *data) {
    struct affected_type af;
    int *cast_level = (int *)data;
    
    /* exits */
    if (is_immune_fear(ch, tch, TRUE))
      return 0;
    if (is_immune_mind_affecting(ch, tch, TRUE))
      return 0;
    if (savingthrow(ch, tch, SAVING_WILL, 0, CAST_INNATE, *cast_level, SCHOOL_NOSCHOOL))
      return 0;

    // success
    act("$n glows with black energy as $s coutenance causes FEAR to $N!",
        FALSE, ch, NULL, tch, TO_NOTVICT);
    act("You glow with black energy as you cause fear to $N with your countenance!",
        FALSE, ch, NULL, tch, TO_CHAR);
    act("$n glows with black energy as $s countenance causes you uncontrollable fear!",
        FALSE, ch, NULL, tch, TO_VICT | TO_SLEEP);
    new_affect(&af);
    af.spell = RACIAL_LICH_FEAR;
    af.duration = dice(1, 6);
    SET_BIT_AR(af.bitvector, AFF_FEAR);
    affect_join(tch, &af, FALSE, FALSE, FALSE, FALSE);
    do_flee(tch, 0, 0, 0);
    do_flee(tch, 0, 0, 0);
    
    return 1;
  }

  int cast_level = GET_LEVEL(ch);
  aoe_effect(ch, RACIAL_LICH_FEAR, lich_fear_callback, &cast_level);

  return TRUE;
}

ACMD(do_lichfear)
{
  int uses_remaining = 0;

  if (!IS_LICH(ch))
  {
    send_to_char(ch, "You do not have that ability!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_LICH_FEAR)) == 0)
  {
    send_to_char(ch, "You must recover the profane energy required to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  if (!is_action_available(ch, atSTANDARD, TRUE))
  {
    send_to_char(ch, "Lichfear requires a standard action available to use.\r\n");
    return;
  }

  if (perform_lichfear(ch))
  {
    if (!IS_NPC(ch))
      start_daily_use_cooldown(ch, FEAT_LICH_FEAR);

    USE_STANDARD_ACTION(ch);
  }
}

/* can 'curse' an opponent with your touch */
ACMD(do_touch_of_corruption)
{
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!HAS_REAL_FEAT(ch, FEAT_TOUCH_OF_CORRUPTION))
  {
    send_to_char(ch, "You do not have that ability!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_TOUCH_OF_CORRUPTION)) == 0)
  {
    send_to_char(ch, "You must recover the profane energy required to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "You cannot use this ability on yourself\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  int amount = 20 + GET_CHA_BONUS(ch) + dice(CLASS_LEVEL(ch, CLASS_BLACKGUARD), 6);

  if (IS_UNDEAD(vict))
  {
    char buf[200];
    snprintf(buf, sizeof(buf), "You reach out and touch $N with a withering finger, and the surge of negative energy heals him for %d hp.", amount);
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
    snprintf(buf, sizeof(buf), "$n reaches out and touches you with a withering finger, and the surge of negative energy heals you for %d hp.", amount);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    act("$n reaches out and touches $N with a withering finger, and the surge of negative energy heals $M.", FALSE, ch, 0, vict, TO_NOTVICT);
  }

  if (!pvp_ok(ch, vict, true))
    return;

  if (!attack_roll(ch, vict, ATTACK_TYPE_PRIMARY, TRUE, 0))
  {
    act("You reach out to touch $N with a withering finger, but $E avoids you.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n reaches out to touch you with a withering finger, but you avoid $m.", FALSE, ch, 0, vict, TO_VICT);
    act("$n reaches out to touch $N with a withering finger, but $E avoids it.", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }

  act("You reach out and touch $N with a withering finger, and $E wilts before you.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n reaches out and touches you with a withering finger, causing you to wilt before $m.", FALSE, ch, 0, vict, TO_VICT);
  act("$n reaches out and touches $N with a withering finger, causing him to wilt before you.", FALSE, ch, 0, vict, TO_NOTVICT);

  damage(ch, vict, amount, BLACKGUARD_TOUCH_OF_CORRUPTION, DAM_NEGATIVE, FALSE);

  if (*arg2)
  {
    apply_blackguard_cruelty(ch, vict, strdup(arg2));
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_TOUCH_OF_CORRUPTION);

  USE_STANDARD_ACTION(ch);
}


// Necromancer's ability to cause status affects to enemies
ACMD(do_touch_of_undeath)
{
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;
  int spellnum;

  if (!HAS_REAL_FEAT(ch, FEAT_TOUCH_OF_UNDEATH))
  {
    send_to_char(ch, "You do not have that ability!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_TOUCH_OF_UNDEATH)) == 0)
  {
    send_to_char(ch, "You must recover the profane energy required to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "You cannot use this ability on yourself\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "You must specify the type of undeath touch you want to use:\r\n"
                     "paralyze | weaken | degenerate | destroy | death\r\n");
    return;
  }
  else
  {
    if (is_abbrev(arg2, "paralyze"))
    {
      if (!HAS_REAL_FEAT(ch, FEAT_PARALYZING_TOUCH))
      {
        send_to_char(ch, "You don't have the paralyzing touch ability.\r\n");
        return;
      }
      spellnum = ABILITY_PARALYZING_TOUCH;
    }
    else if (is_abbrev(arg2, "weaken"))
    {
      if (!HAS_REAL_FEAT(ch, FEAT_WEAKENING_TOUCH))
      {
        send_to_char(ch, "You don't have the weakening touch ability.\r\n");
        return;
      }
      spellnum = ABILITY_WEAKENING_TOUCH;
    }
    else if (is_abbrev(arg2, "degenerate"))
    {
      if (!HAS_REAL_FEAT(ch, FEAT_DEGENERATIVE_TOUCH))
      {
        send_to_char(ch, "You don't have the degenerative touch ability.\r\n");
        return;
      }
      spellnum = ABILITY_DEGENERATIVE_TOUCH;
    }
    else if (is_abbrev(arg2, "destroy"))
    {
      if (!HAS_REAL_FEAT(ch, FEAT_DESTRUCTIVE_TOUCH))
      {
        send_to_char(ch, "You don't have the destructive touch ability.\r\n");
        return;
      }
      spellnum = ABILITY_DESTRUCTIVE_TOUCH;
    }
    else if (is_abbrev(arg2, "death"))
    {
      if (!HAS_REAL_FEAT(ch, FEAT_DEATHLESS_TOUCH))
      {
        send_to_char(ch, "You don't have the deathless touch ability.\r\n");
        return;
      }
      spellnum = ABILITY_DEATHLESS_TOUCH;
    }
    else
    { 
      send_to_char(ch, "You must specify a valid type of undeath touch that you want to use:\r\n"
                      "paralyze | weaken | degenerate | destroy | death\r\n");
      return; 
    }
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  if (IS_UNDEAD(vict))
  {
    act("Your touch of undeath has no effect on $N", FALSE, ch, 0, vict, TO_CHAR);
    act("$n's touch of undeath has no effect on You", FALSE, ch, 0, vict, TO_VICT);
    act("$n's touch of undeath has no effect on $N", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }

  if (!pvp_ok(ch, vict, true))
    return;

  if (!attack_roll(ch, vict, ATTACK_TYPE_PRIMARY, TRUE, 0))
  {
    act("You reach out to touch $N with your undead arm, but $E avoids you.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n reaches out to touch you with $s undead arm, but you avoid $m.", FALSE, ch, 0, vict, TO_VICT);
    act("$n reaches out to touch $N with $s undead arm, but $E avoids it.", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }

  act("You reach out and touch $N with your undead arm, and $E suffers visibly.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n reaches out and touches you with $s undead arm, causing $M to suffer visibly.", FALSE, ch, 0, vict, TO_VICT);
  act("$n reaches out and touches $N with $s undead arm, causing $M to syffer visibly.", FALSE, ch, 0, vict, TO_NOTVICT);
  call_magic(ch, vict, 0, spellnum, 0, compute_arcane_level(ch), CASTING_TYPE_ARCANE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_TOUCH_OF_CORRUPTION);

  USE_STANDARD_ACTION(ch);
}

void apply_blackguard_cruelty(struct char_data *ch, struct char_data *vict, char *cruelty)
{
  if (!ch || !vict)
    return;

  if (!*cruelty)
  {
    return;
  }

  int i = 0, duration = 0;
  int which_cruelty = BLACKGUARD_CRUELTY_NONE;
  int save_mod = 0;
  const char *to_vict = NULL, *to_room = NULL;

  for (i = 1; i < NUM_BLACKGUARD_CRUELTIES; i++)
  {
    if (is_abbrev(cruelty, blackguard_cruelties[i]))
    {
      which_cruelty = i;
      break;
    }
  }

  if (i >= NUM_BLACKGUARD_CRUELTIES || i < 1)
  {
    send_to_char(ch, "That is not a valid cruelty type.  See the cruelties command for a list.\r\n");
    return;
  }

  if (!KNOWS_CRUELTY(ch, i))
  {
    send_to_char(ch, "You do not know that cruelty.\r\n");
    return;
  }

  // check for immunities
  switch (which_cruelty)
  {
  case BLACKGUARD_CRUELTY_SHAKEN:
    to_vict = "You are -shaken- from the cruelty inflicted upon you by the corrupting touch!";
    to_room = "$n is -shaken- from the cruelty inflicted upon $M by the corrupting touch!";
    if (is_immune_fear(ch, vict, true))
      return;
    if (is_immune_mind_affecting(ch, vict, true))
      return;
    if (affected_by_aura_of_cowardice(vict))
      save_mod = -4;
    break;
  case BLACKGUARD_CRUELTY_FRIGHTENED:
    to_vict = "You are -frightened- from the cruelty inflicted upon you by the corrupting touch!";
    to_room = "$n is -frightened- from the cruelty inflicted upon $M by the corrupting touch!";
    if (is_immune_fear(ch, vict, true))
      return;
    if (is_immune_mind_affecting(ch, vict, true))
      return;
    if (affected_by_aura_of_cowardice(vict))
      save_mod = -4;
    break;
  case BLACKGUARD_CRUELTY_SICKENED:
    to_vict = "You are -sickened- from the cruelty inflicted upon you by the corrupting touch!";
    to_room = "$n is -sickened- from the cruelty inflicted upon $M by the corrupting touch!";
    if (!can_disease(vict))
    {
      act("$E is immune to disease!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    break;
  case BLACKGUARD_CRUELTY_DISEASED:
    to_vict = "You are -diseased- from the cruelty inflicted upon you by the corrupting touch!";
    to_room = "$n is -diseased- from the cruelty inflicted upon $M by the corrupting touch!";
    if (!can_disease(vict))
    {
      act("$E is immune to disease!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    break;
  case BLACKGUARD_CRUELTY_POISONED:
    to_vict = "You are -poisoned- from the cruelty inflicted upon you by the corrupting touch!";
    to_room = "$n is -poisoned- from the cruelty inflicted upon $M by the corrupting touch!";
    if (!can_poison(vict))
    {
      act("$E is immune to poison!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    break;
  case BLACKGUARD_CRUELTY_BLINDED:
    to_vict = "You are -blinded- from the cruelty inflicted upon you by the corrupting touch!";
    to_room = "$n is -blinded- from the cruelty inflicted upon $M by the corrupting touch!";
    if (!can_blind(vict))
    {
      act("$E cannot be blinded!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    break;
  case BLACKGUARD_CRUELTY_DEAFENED:
    to_vict = "You are -deafened- from the cruelty inflicted upon you by the corrupting touch!";
    to_room = "$n is -deafened- from the cruelty inflicted upon $M by the corrupting touch!";
    if (!can_deafen(vict))
    {
      act("$E cannot be deafened!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    break;
  case BLACKGUARD_CRUELTY_PARALYZED:
    if (AFF_FLAGGED(vict, AFF_FREE_MOVEMENT))
    {
      act("$E cannot be paralyzed!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    if (vict->char_specials.eldritch_blast_cooldowns[ELDRITCH_BLAST_COOLDOWN_BINDING_BLAST] > 0)
    {
      act("The target is on an immunity cooldown for paralysis already.", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    to_vict = "You are -paralyzed- from the cruelty inflicted upon you by the corrupting touch!";
    to_room = "$n is -paralyzed- from the cruelty inflicted upon $M by the corrupting touch!";
    break;
  case BLACKGUARD_CRUELTY_DAZED:
    if (AFF_FLAGGED(vict, AFF_FREE_MOVEMENT))
    {
      act("$E cannot be dazed!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    if (vict->char_specials.eldritch_blast_cooldowns[ELDRITCH_BLAST_COOLDOWN_NOXIOUS_BLAST] > 0)
    {
      act("The target is on an immunity cooldown for being dazed already.", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    to_vict = "You are -dazed- from the cruelty inflicted upon you by the corrupting touch!";
    to_room = "$n is -dazed- from the cruelty inflicted upon $M by the corrupting touch!";
    break;
  case BLACKGUARD_CRUELTY_STUNNED:
    if (!can_stun(vict))
    {
      act("$E cannot be stunned!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    if (vict->char_specials.eldritch_blast_cooldowns[ELDRITCH_BLAST_COOLDOWN_NOXIOUS_BLAST] > 0)
    {
      act("The target is on an immunity cooldown for being stunned already.", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    to_vict = "You are -stunned- from the cruelty inflicted upon you by the corrupting touch!";
    to_room = "$n is -stunned- from the cruelty inflicted upon $M by the corrupting touch!";
    break;
  }

  // figure out duration
  switch (which_cruelty)
  {
  case BLACKGUARD_CRUELTY_DAZED:
  case BLACKGUARD_CRUELTY_PARALYZED:
  case BLACKGUARD_CRUELTY_STUNNED:
    duration = 1;
    break;
  case BLACKGUARD_CRUELTY_STAGGERED:
  case BLACKGUARD_CRUELTY_FRIGHTENED:
    duration = CLASS_LEVEL(ch, CLASS_BLACKGUARD) / 2;
    break;
  case BLACKGUARD_CRUELTY_NAUSEATED:
    duration = CLASS_LEVEL(ch, CLASS_BLACKGUARD) / 3;
    break;
  default:
    duration = CLASS_LEVEL(ch, CLASS_BLACKGUARD);
    break;
  }

  if (HAS_FEAT(ch, FEAT_IMPROVED_CRUELTIES))
    save_mod -= 2;
  if (HAS_FEAT(ch, FEAT_ADVANCED_CRUELTIES))
    save_mod -= 2;
  if (HAS_FEAT(ch, FEAT_MASTER_CRUELTIES))
    save_mod -= 2;
  if (HAS_FEAT(ch, FEAT_EPIC_CRUELTIES))
    save_mod -= 2;

  if (savingthrow(ch, vict, SAVING_FORT, save_mod, CAST_CRUELTY, CLASS_LEVEL(ch, CLASS_BLACKGUARD), NOSCHOOL))
  {
    return;
  }

  struct affected_type af;

  new_affect(&af);

  af.spell = BLACKGUARD_CRUELTY_AFFECTS;
  SET_BIT_AR(af.bitvector, blackguard_cruelty_affect_types[which_cruelty]);
  af.duration = duration;

  affect_to_char(vict, &af);

  if (to_vict != NULL)
    act(to_vict, FALSE, vict, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, vict, 0, ch, TO_ROOM);
}

void throw_hedging_weapon(struct char_data *ch)
{
  if (!ch || !FIGHTING(ch))
    return;
  if (!affected_by_spell(ch, SPELL_HEDGING_WEAPONS))
    return;

  struct affected_type *af = ch->affected;
  bool remove = false;
  int roll = 0, defense = 0, dam = 0;

  for (af = ch->affected; af; af = af->next)
  {
    if (af->spell != SPELL_HEDGING_WEAPONS)
      continue;
    if (af->modifier < 1)
    {
      remove = true;
      break;
    }
    else
    {
      roll = d20(ch) + compute_attack_bonus(ch, FIGHTING(ch), ATTACK_TYPE_RANGED);
      defense = compute_armor_class(ch, FIGHTING(ch), TRUE, MODE_ARMOR_CLASS_NORMAL);
      if (roll >= defense)
      {
        dam = dice(2, 4) + GET_CHA_BONUS(ch);
      }
      else
      {
        dam = 0;
      }
      damage(ch, FIGHTING(ch), dam, SPELL_HEDGING_WEAPONS, DAM_FORCE, FALSE);
      af->modifier--;
      if (af->modifier < 1)
        remove = true;
      break;
    }
  }
  if (remove)
  {
    affect_from_char(ch, SPELL_HEDGING_WEAPONS);
  }
  save_char(ch, 0);
}

ACMD(do_mark)
{
  struct char_data *vict = NULL;
  char arg[100];

  one_argument(argument, arg, sizeof(arg));

  if (CLASS_LEVEL(ch, CLASS_ASSASSIN) < 1)
  {
    send_to_char(ch, "Only assassins know how to do that!\r\n");
    return;
  }

  if (!*arg)
  {
    send_to_char(ch, "Who would you like to mark for assassination?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "That person isn't here right now.\r\n");
    return;
  }

  act("You begin to mark $N for assassination.", false, ch, 0, vict, TO_CHAR);
  GET_MARK(ch) = vict;
  GET_MARK_ROUNDS(ch) = 0;
}

int max_judgements_active(struct char_data *ch)
{
  int num = 0;
  if (HAS_REAL_FEAT(ch, FEAT_JUDGEMENT))
    num++;
  if (HAS_REAL_FEAT(ch, FEAT_SECOND_JUDGEMENT))
    num++;
  if (HAS_REAL_FEAT(ch, FEAT_THIRD_JUDGEMENT))
    num++;
  if (HAS_REAL_FEAT(ch, FEAT_FOURTH_JUDGEMENT))
    num++;
  if (HAS_REAL_FEAT(ch, FEAT_FIFTH_JUDGEMENT))
    num++;

  return num;
}

int num_judgements_active(struct char_data *ch)
{
  int i = 0, num_active = 0;
  for (i = 1; i < NUM_INQ_JUDGEMENTS; i++)
    if (IS_JUDGEMENT_ACTIVE(ch, i))
      num_active++;

  return num_active;
}

ACMDU(do_judgement)
{

  if (!HAS_REAL_FEAT(ch, FEAT_JUDGEMENT))
  {
    send_to_char(ch, "You are not able to perform judgements.\r\n");
    return;
  }

  skip_spaces(&argument);

  char arg1[200], arg2[200];
  int uses_remaining = 0, i = 0, judgement = 0;
  struct char_data *vict = FIGHTING(ch);

  if (!*argument)
  {
    send_to_char(ch, "\r\n"
                     "Please select from the following judgement options:\r\n"
                     "judgement enact                   - performs a judgement on the currently targetted enemy in combat.\r\n"
                     "judgement toggle (judgement type) - will toggle on/off the specified judgement type.\r\n"
                     "judgement list                    - will display judgements, which are enabled and how many uses left.\r\n"
                     "\r\n");
    return;
  }

  half_chop(argument, arg1, arg2);

  // We're going to perform our judgement on our current combat target.
  if (is_abbrev(arg1, "enact"))
  {
    if (!FIGHTING(ch))
    {
      send_to_char(ch, "You have to be in combat to enact a judgement.\r\n");
      return;
    }

    if ((uses_remaining = daily_uses_remaining(ch, FEAT_JUDGEMENT)) == 0)
    {
      send_to_char(ch, "You must recover your energy required to use this ability again.\r\n");
      return;
    }

    if (uses_remaining < 0)
    {
      send_to_char(ch, "You are not experienced enough.\r\n");
      return;
    }

    GET_JUDGEMENT_TARGET(ch) = vict;

    act("You pronounce divine judgment upon $N.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n pronounces divine judgment upon YOU.", TRUE, ch, 0, vict, TO_VICT);
    act("$n pronounces divine judgment upon $N.", TRUE, ch, 0, vict, TO_NOTVICT);

    if (!IS_NPC(ch))
      start_daily_use_cooldown(ch, FEAT_JUDGEMENT);

    USE_SWIFT_ACTION(ch);
    return;
  }
  else if (is_abbrev(arg1, "list"))
  {
    send_to_char(ch, "    %-15s %s\r\n", "Judgement Name", "Description");
    for (i = 1; i < NUM_INQ_JUDGEMENTS; i++)
    {
      send_to_char(ch, "%s[[%s]\tn %-15s : %s\r \n",
                   IS_JUDGEMENT_ACTIVE(ch, i) ? "\tG" : "\tR",
                   IS_JUDGEMENT_ACTIVE(ch, i) ? "+" : "-",
                   inquisitor_judgements[i], inquisitor_judgement_descriptions[i]);
    }
    send_to_char(ch, "\r\n");
    send_to_char(ch, "Judgements Active: %d of %d max.\r\n", num_judgements_active(ch), max_judgements_active(ch));
    send_to_char(ch, "Uses left: %d\r\n", MAX(0, daily_uses_remaining(ch, FEAT_JUDGEMENT)));
    send_to_char(ch, "\r\n");
    return;
  }
  else if (is_abbrev(arg1, "toggle"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Please specify which judgment you'd like to toggle. For a list, type: judgement list.\r\n");
      return;
    }

    CAP(arg2);

    for (i = 1; i < NUM_INQ_JUDGEMENTS; i++)
    {
      if (is_abbrev(arg2, inquisitor_judgements[i]))
      {
        judgement = i;
        break;
      }
    }

    if (judgement == 0)
    {
      send_to_char(ch, "That is not a valid judgement.  For a list type: judgement list.\r\n");
      return;
    }

    if (IS_JUDGEMENT_ACTIVE(ch, judgement))
    {
      send_to_char(ch, "You turn \tRoff\tn the '%s' judgement effect.\r\n", inquisitor_judgements[judgement]);
      IS_JUDGEMENT_ACTIVE(ch, judgement) = 0;
      if (GET_SLAYER_JUDGEMENT(ch) == judgement)
      {
        send_to_char(ch, "Your slayer effect for '%s' has been removed as well.\r\n", inquisitor_judgements[judgement]);
        GET_SLAYER_JUDGEMENT(ch) = 0;
      }
    }
    else
    {
      if (num_judgements_active(ch) >= max_judgements_active(ch))
      {
        send_to_char(ch, "You already have your maximum active judgement effects.\r\n");
        return;
      }
      send_to_char(ch, "You turn \tGon\tn the '%s' judgement effect.\r\n", inquisitor_judgements[judgement]);
      IS_JUDGEMENT_ACTIVE(ch, judgement) = 1;
    }
  }
  else
  {
    send_to_char(ch, "\r\n"
                     "Please select from the following judgement options:\r\n"
                     "judgement enact                   - performs a judgement on the currently targetted enemy in combat.\r\n"
                     "judgement toggle (judgement type) - will toggle on/off the specified judgement type.\r\n"
                     "judgement list                    - will display judgements, which are enabled and how many uses left.\r\n"
                     "\r\n");
    return;
  }
}

void perform_bane(struct char_data *ch)
{
  struct affected_type af;
  char buf[200];

  new_affect(&af);
  af.spell = ABILITY_AFFECT_BANE_WEAPON;
  af.duration = 5;

  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_BANE);

  snprintf(buf, sizeof(buf), "You enhance your weapons with a bane effect against %s creatures.", race_family_types[GET_BANE_TARGET_TYPE(ch)]);
  act(buf, FALSE, ch, 0, 0, TO_CHAR);
  act("$n's weapons begin to glow yellow.", TRUE, ch, 0, 0, TO_ROOM);
}

ACMDCHECK(can_bane)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_BANE, "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, ABILITY_AFFECT_BANE_WEAPON),
                        "You have already enhanced your weapons with the bane effect! See score for more info.\r\n"
                        "You will need to wait for this bane effect to expire, or remove it using the 'revoke' command, to choose another bane enemy type.\r\n");
  return CAN_CMD;
}

ACMDU(do_bane)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_bane);
  PREREQ_HAS_USES(FEAT_BANE, "You must recover before you can enhance your weapons with the bane effect again.\r\n");

  int i = 0;

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Please specify one of the following racial types:\r\n");
    for (i = 1; i < NUM_RACE_TYPES; i++)
    {
      send_to_char(ch, "%s\r\n", race_family_types[i]);
    }
    return;
  }

  CAP(argument);

  for (i = 1; i < NUM_RACE_TYPES; i++)
  {
    if (is_abbrev(argument, race_family_types[i]))
      break;
  }

  if (i < 1 || i >= NUM_RACE_TYPES)
  {
    send_to_char(ch, "That is an invalid selection. Please select again.  Type 'bane' by itself for a list of options.\r\n");
    return;
  }

  GET_BANE_TARGET_TYPE(ch) = i;

  perform_bane(ch);
}

ACMDU(do_slayer)
{
  if (!HAS_REAL_FEAT(ch, FEAT_SLAYER))
  {
    send_to_char(ch, "You do not have the slayer feat.\r\n");
    return;
  }

  int i = 0;

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Please specify an active judgement.  A list can be seen by typing: judgement list.\r\n");
    return;
  }

  CAP(argument);

  for (i = 0; i < NUM_INQ_JUDGEMENTS; i++)
  {
    if (is_abbrev(argument, inquisitor_judgements[i]))
    {
      if (!IS_JUDGEMENT_ACTIVE(ch, i))
      {
        send_to_char(ch, "That judgement is not active.  Please select an active judgement.  A list can be seen by typing: judgement list.\r\n");
        return;
      }
      break;
    }
  }

  if (i < 1 || i >= NUM_INQ_JUDGEMENTS)
  {
    send_to_char(ch, "That is not a valid judgement type.  A list can be seen by typing: judgement list.\r\n");
    return;
  }

  GET_SLAYER_JUDGEMENT(ch) = i;
  send_to_char(ch, "You assign %s as your slayer enabled judgement.  "
                   "Your inquisitor level will be treated as 5 higher when determining bonus amount.\r\n",
               inquisitor_judgements[i]);
}

ACMDCHECK(can_true_judgement)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_TRUE_JUDGEMENT, "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, ABILITY_AFFECT_TRUE_JUDGEMENT), "You have already gathered your divine energy!\r\n");
  return CAN_CMD;
}

ACMD(do_true_judgement)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_true_judgement);
  PREREQ_HAS_USES(FEAT_TRUE_JUDGEMENT, "You must recover before you can gather your divine energy in this way again.\r\n");

  perform_true_judgement(ch);
}

void perform_true_judgement(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = ABILITY_AFFECT_TRUE_JUDGEMENT;
  af.duration = 24;

  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_TRUE_JUDGEMENT);

  act("You gather your divine energy into your next attack.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n begins gathering $s divine energy.", TRUE, ch, 0, 0, TO_ROOM);
}

ACMDCHECK(can_children_of_the_night)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_VAMPIRE_CHILDREN_OF_THE_NIGHT, "You have no idea how.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT), "You have already called your children of the night!\r\n");
  // if (check_npc_followers(ch, NPC_MODE_FLAG, MOB_C_O_T_N))
  if (!can_add_follower_by_flag(ch, MOB_C_O_T_N))
  {
    send_to_char(ch, "Wha?!  You have already called your children of the night!\r\n");
    return 0;
  }

  return CAN_CMD;
}

ACMD(do_children_of_the_night)
{
  PREREQ_CHECK(can_children_of_the_night);
  PREREQ_HAS_USES(FEAT_VAMPIRE_CHILDREN_OF_THE_NIGHT, "You must recover before you can call your children of the night.\r\n");

  if (!CAN_USE_VAMPIRE_ABILITY(ch))
  {
    send_to_char(ch, "You cannot use your vampiric abilities when in sunlight or moving water.\r\n");
    return;
  }

  perform_children_of_the_night(ch);
}

void perform_children_of_the_night(struct char_data *ch)
{

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_VAMPIRE_CHILDREN_OF_THE_NIGHT);

  act("You reach out into the wilds to pull forth your children of the night.", FALSE, ch, 0, 0, TO_CHAR);

  call_magic(ch, ch, 0, VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT, 0, GET_LEVEL(ch), CAST_INNATE);
}

ACMDCHECK(can_create_vampire_spawn)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_VAMPIRE_CREATE_SPAWN, "You have no idea how.\r\n");

  // if (check_npc_followers(ch, NPC_MODE_FLAG, MOB_VAMP_SPWN))
  if (!can_add_follower_by_flag(ch, MOB_VAMP_SPWN))
  {
    send_to_char(ch, "You have already created vampiric spawn.\r\n");
    return 0;
  }

  return CAN_CMD;
}

ACMDU(do_create_vampire_spawn)
{
  PREREQ_CHECK(can_create_vampire_spawn);

  if (!CAN_USE_VAMPIRE_ABILITY(ch))
  {
    send_to_char(ch, "You cannot use your vampiric abilities when in sunlight or moving water.\r\n");
    return;
  }

  struct obj_data *obj = NULL;

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "You must specify a corpse to convert into your vampiric spawn.\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, argument, 0, world[IN_ROOM(ch)].contents)))
  {
    send_to_char(ch, "You don't see any corpses by that description.\r\n");
    return;
  }

  if (!IS_CORPSE(obj))
  {
    send_to_char(ch, "That's not a corpse.\r\n");
    return;
  }

  if (!obj->drainKilled)
  {
    send_to_char(ch, "You can only create vampiric spawn from the corpse of a victim you killed by blood drain.\r\n");
    return;
  }

  act("You draw upon your vampiric strength and attempt to convert $p into vampiric spawn", FALSE, ch, obj, 0, TO_CHAR);

  call_magic(ch, ch, obj, ABILITY_CREATE_VAMPIRE_SPAWN, 0, GET_LEVEL(ch), CAST_INNATE);
}

ACMDCHECK(can_vampiric_gaseous_form)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_VAMPIRE_GASEOUS_FORM, "You have no idea how.\r\n");

  return CAN_CMD;
}

ACMD(do_vampiric_gaseous_form)
{
  PREREQ_CHECK(can_vampiric_gaseous_form);

  if (!CAN_USE_VAMPIRE_ABILITY(ch))
  {
    send_to_char(ch, "You cannot use your vampiric abilities when in sunlight or moving water.\r\n");
    return;
  }

  act("You draw upon your vampiric ablity and your body shifts into a gaseous state.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n's body assumes a gaseous state.", FALSE, ch, 0, 0, TO_ROOM);

  call_magic(ch, ch, 0, SPELL_GASEOUS_FORM, 0, GET_LEVEL(ch), CAST_INNATE);
}

ACMDCHECK(can_vampiric_shape_change)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_VAMPIRE_CHANGE_SHAPE, "You have no idea how.\r\n");

  return CAN_CMD;
}

ACMDU(do_vampiric_shape_change)
{
  PREREQ_CHECK(can_vampiric_shape_change);

  if (!CAN_USE_VAMPIRE_ABILITY(ch))
  {
    send_to_char(ch, "You cannot use your vampiric abilities when in sunlight or moving water.\r\n");
    return;
  }

  if (IS_WILDSHAPED(ch))
  {
    send_to_char(ch, "You cannot shift form while wildshaped.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "You must specify either 'wolf' or 'bat' form.\r\n");
    return;
  }

  act("You draw upon your vampiric ablity and shift into animal form.", FALSE, ch, 0, 0, TO_CHAR);

  /* act.other.c, part of druid wildshape engine, the value "2" notifies the
      the function that this is the vampiric change shape ability */
  wildshape_engine(ch, argument, 2);
}

ACMDCHECK(can_vampiric_dominate)
{
  ACMDCHECK_PREREQ_HASFEAT(FEAT_VAMPIRE_DOMINATE, "You have no idea how.\r\n");

  return CAN_CMD;
}

ACMDU(do_vampiric_dominate)
{
  PREREQ_CHECK(can_vampiric_dominate);

  if (!CAN_USE_VAMPIRE_ABILITY(ch))
  {
    send_to_char(ch, "You cannot use your vampiric abilities when in sunlight or moving water.\r\n");
    return;
  }

  struct char_data *vict;

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "You must specify who you would like to try and dominate.\r\n");
    return;
  }

  if (!(vict = get_char_room_vis(ch, argument, 0)))
  {
    send_to_char(ch, "There is no one in the room by that description.\r\n");
    return;
  }

  if (!IS_SENTIENT(vict))
  {
    send_to_char(ch, "You can only dominate sentient beings.\r\n");
    return;
  }

  act("You draw upon your vampiric ablity and try to dominate $N.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n gazes into your eyes and tries to dominate your will!", FALSE, ch, 0, vict, TO_VICT);
  act("$n gazes into $N's eyes and tries to dominate $S will!", FALSE, ch, 0, vict, TO_NOTVICT);

  effect_charm(ch, vict, ABILITY_VAMPIRIC_DOMINATION, CAST_INNATE, GET_LEVEL(ch));
}

/* slam engine */
void perform_slam(struct char_data *ch, struct char_data *vict)
{
  int discipline_bonus = 0, diceOne = 0, diceTwo = 0;
  struct affected_type af;

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  PREREQ_NOT_PEACEFUL_ROOM();

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  if (IS_INCORPOREAL(vict) && !is_using_ghost_touch_weapon(ch))
  {
    act("$n sprawls completely through $N as $e tries to slam $M.", FALSE, ch, NULL, vict, TO_NOTVICT);
    act("You sprawl completely through $N as you try to slam $M!", FALSE, ch, NULL, vict, TO_CHAR);
    act("$n sprawls completely through you as $e tries to slam you!", FALSE, ch, NULL, vict, TO_VICT);
    change_position(ch, POS_SITTING);
    return;
  }

  /* maneuver bonus/penalty */
  if (!IS_NPC(ch) && compute_ability(ch, ABILITY_DISCIPLINE))
    discipline_bonus += compute_ability(ch, ABILITY_DISCIPLINE);
  if (!IS_NPC(vict) && compute_ability(vict, ABILITY_DISCIPLINE))
    discipline_bonus -= compute_ability(vict, ABILITY_DISCIPLINE);

  /* saving throw dc - unused for slam */
  /* dc = GET_LEVEL(ch) / 2 + GET_STR_BONUS(ch); */

  /* monk damage? */
  compute_barehand_dam_dice(ch, &diceOne, &diceTwo);
  if (diceOne < 1)
    diceOne = 1;
  if (diceTwo < 2)
    diceTwo = 2;


  if (vict->char_specials.recently_slammed == 0)
  {
    if (combat_maneuver_check(ch, vict, COMBAT_MANEUVER_TYPE_SLAM, 0) > 0)
    {
      damage(ch, vict, dice(diceOne, diceTwo) + GET_STR_BONUS(ch), SKILL_SLAM, DAM_FORCE, FALSE);

      if (HAS_FEAT(ch, FEAT_VAMPIRE_ENERGY_DRAIN) && IS_LIVING(vict) && CAN_USE_VAMPIRE_ABILITY(ch))
      {
        if (daily_uses_remaining(ch, FEAT_VAMPIRE_ENERGY_DRAIN) > 0)
        {
          if (!savingthrow(ch, vict, SAVING_WILL, 0, CAST_INNATE, GET_LEVEL(ch), NECROMANCY))
          {
            new_affect(&af);
            af.spell = AFFECT_LEVEL_DRAIN;
            af.location = APPLY_SPECIAL;
            af.modifier = 1;
            af.duration = 10;
            affect_join(vict, &af, TRUE, FALSE, TRUE, FALSE);
            act("You drain some of $N's life force away.", FALSE, ch, 0, vict, TO_CHAR);
            act("$n drains some of your life force away.", FALSE, ch, 0, vict, TO_VICT);
            act("$n drains some of $N's life force away.", FALSE, ch, 0, vict, TO_NOTVICT);

            // set the enemy as drainkilled so they can be turned into a vampire spawn if they do end up being killed.
            vict->char_specials.drainKilled = true;
          }
          if (!IS_NPC(ch))
            start_daily_use_cooldown(ch, FEAT_VAMPIRE_ENERGY_DRAIN);
        }
      }

      /* fire-shield, etc check */
      damage_shield_check(ch, vict, ATTACK_TYPE_UNARMED, TRUE, DAM_FORCE);
    }
    else
      damage(ch, vict, 0, SKILL_SLAM, DAM_FORCE, FALSE);
    vict->char_specials.recently_slammed = 3;
  }
  else
  {
    send_to_char(ch, "They predicted your slam attempt and bypassed easily.\r\n");
  }
}

ACMD(do_slam)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  PREREQ_CAN_FIGHT();

  PREREQ_NOT_NPC();

  one_argument(argument, arg, sizeof(arg));

  /* find the victim */
  vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);

  /* we have a disqualifier here due to action system */
  if (!FIGHTING(ch) && !vict)
  {
    send_to_char(ch, "Who do you want to slam?\r\n");
    return;
  }
  if (vict == ch)
  {
    send_to_char(ch, "You slam yourself.\r\n");
    return;
  }
  if (FIGHTING(ch) && !vict && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    vict = FIGHTING(ch);

  perform_slam(ch, vict);
}

ACMD(do_blood_drain)
{

  struct char_data *vict = NULL;
  char arg[200];
  int uses_remaining;

  one_argument(argument, arg, sizeof(arg));

  if (!HAS_FEAT(ch, FEAT_VAMPIRE_BLOOD_DRAIN))
  {
    send_to_char(ch, "You don't have the ability to food on the blood of others.\r\n");
    return;
  }

  if (affected_by_spell(ch, ABILITY_BLOOD_DRAIN))
  {
    send_to_char(ch, "You are already feasting on your opponent's blood.\r\n");
    return;
  }

  if (FIGHTING(ch))
    vict = FIGHTING(ch);
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "There's no one there by that description.\r\n");
    return;
  }

  if (!can_blood_drain_target(ch, vict))
  {
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_VAMPIRE_BLOOD_DRAIN)) == 0)
  {
    send_to_char(ch, "You must recover the profane energy required to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  if (!is_action_available(ch, atSTANDARD, TRUE))
  {
    send_to_char(ch, "Blood drain requires a standard action available to use.\r\n");
    return;
  }

  act("You prepare to feast on the blood of $N!", FALSE, ch, 0, vict, TO_CHAR);

  if (!FIGHTING(ch))
  {
    hit(ch, vict, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
  }

  struct affected_type af;

  new_affect(&af);
  af.spell = ABILITY_BLOOD_DRAIN;
  af.duration = 5 + GET_LEVEL(ch) / 6;
  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_VAMPIRE_BLOOD_DRAIN);

  USE_STANDARD_ACTION(ch);
}

ACMD(do_quick_chant)
{

  int uses_remaining = 0;

  if (subcmd == SCMD_QUICK_CHANT && !HAS_FEAT(ch, FEAT_QUICK_CHANT))
  {
    send_to_char(ch, "You do not have the quick chant feat.\r\n");
    return;
  }
  else if (subcmd == SCMD_QUICK_MIND && !HAS_FEAT(ch, FEAT_QUICK_MIND))
  {
    send_to_char(ch, "You do not have the quick mind feat.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, (subcmd == SCMD_QUICK_CHANT) ? FEAT_QUICK_CHANT : FEAT_QUICK_MIND)) == 0)
  {
    send_to_char(ch, "You must recover the energy required to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  if (subcmd == SCMD_QUICK_CHANT)
  {
    if (ch->char_specials.quick_chant)
    {
      send_to_char(ch, "You are already benefitting from quick chant.\r\n");
      return;
    }
    send_to_char(ch, "Youn invoke your quick chant ability.  The next non-ritual spell you cast will only use a swift action.\r\n");
    ch->char_specials.quick_chant = true;
  }
  else
  {
    if (ch->char_specials.quick_mind)
    {
      send_to_char(ch, "You are already benefitting from quick mind.\r\n");
      return;
    }
    send_to_char(ch, "Youn invoke your quick mind ability.  The next non-ritual power your manifest will only use a swift action.\r\n");
    ch->char_specials.quick_mind = true;
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, (subcmd == SCMD_QUICK_CHANT) ? FEAT_QUICK_CHANT : FEAT_QUICK_MIND);
}

ACMD(do_planarsoul)
{
  if (!affected_by_spell(ch, SPELL_PLANAR_SOUL))
  {
    send_to_char(ch, "You must be under the affect of the planar soul spell to use it's surging effect.\r\n");
    return;
  }

  if (affected_by_spell(ch, AFFECT_PLANAR_SOUL_SURGE))
  {
    send_to_char(ch, "You are already under the effect of a planar soul surge enhancement.\r\n");
    return;
  }

  struct affected_type af[6];
  struct affected_type *aff = NULL;
  int i = 0, duration = 0;

  for (aff = ch->affected; aff; aff = aff->next)
  {
    if (aff->spell == SPELL_PLANAR_SOUL)
    {
      duration = aff->duration;
      break;
    }
  }

  affect_from_char(ch, SPELL_PLANAR_SOUL);

  duration /= 600;
  duration = MAX(duration, 1);

  for (i = 0; i < 6; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = AFFECT_PLANAR_SOUL_SURGE;
    af[i].bonus_type = BONUS_TYPE_SACRED;
    af[i].duration = duration;
  }

  af[0].location = APPLY_AC_NEW;
  af[0].modifier = 2;
  af[1].location = APPLY_STR;
  af[1].modifier = 4;
  af[2].location = APPLY_RES_ACID;
  af[2].modifier = 15;
  af[3].location = APPLY_RES_FIRE;
  af[3].modifier = 15;
  af[4].location = APPLY_FAST_HEALING;
  af[4].modifier = 2;
  af[5].location = APPLY_SKILL;
  af[5].modifier = 5;
  af[5].specific = ABILITY_INTIMIDATE;

  for (i = 0; i < 6; i++)
  {
    affect_to_char(ch, (&(af[i])));
  }

  act("Your planar soul surges with might!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n seems to surge with might!", FALSE, ch, 0, 0, TO_ROOM);
}


ACMD(do_grand_destiny)
{
  if (!affected_by_spell(ch, SPELL_GRAND_DESTINY))
  {
    send_to_char(ch, "You must be under the affect of the grand destiny spell to actualize it.\r\n");
    return;
  }

  if (affected_by_spell(ch, SPELL_EFFECT_GRAND_DESTINY))
  {
    send_to_char(ch, "You are already under the effect of a grand destiny actualization.\r\n");
    return;
  }

  struct affected_type af;

  affect_from_char(ch, SPELL_GRAND_DESTINY);

  new_affect(&af);
  af.spell = SPELL_EFFECT_GRAND_DESTINY;
  af.location = APPLY_SPECIAL;
  af.duration = 3;

  affect_to_char(ch, (&af));

  act("Your grand destiny has been actualized!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n's grand destiny has been actualized!", FALSE, ch, 0, 0, TO_ROOM);
}

ACMD(do_bullrush)
{
  if (!HAS_EVOLUTION(ch, EVOLUTION_PUSH))
  {
    send_to_char(ch, "You don't have the ability to bullrush.\r\n");
    return;
  }

  struct char_data *vict = NULL;
  char arg[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
  int dir = 0;

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!*arg)
  {
    send_to_char(ch, "You need to specify who you'd like to bullrush.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "There's no one there by that description.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "In what direction do you wish to bullrush your target?\r\n");
    return;
  }

  for (dir = 0; dir < NUM_OF_DIRS; dir++)
  {
    if (is_abbrev(dirs[dir], arg2))
      break;
  }

  if (dir >= NUM_OF_DIRS)
  {
    send_to_char(ch, "That is not a valid direction.\r\n");
    return;
  }

  if (GET_SIZE(vict) >= GET_SIZE(ch))
  {
    send_to_char(ch, "They are too big for you to bullrush.\r\n");
    return;
  }

  if (GET_PUSHED_TIMER(vict) > 0)
  {
    send_to_char(ch, "That subject is on a bullrush timer and is temporarily immune.\r\n");
    return;
  }

  GET_PUSHED_TIMER(vict) = 10;
  USE_STANDARD_ACTION(ch);

  if (!push_attempt(ch, vict, true))
  {
    act("You try to push $N, but $E won't budge an inch!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n tries to push You, but you refuse budge an inch!", FALSE, ch, 0, vict, TO_VICT);
    act("$n tries to push $N, but $E won't budge an inch!", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }

  snprintf(buf, sizeof(buf), "You shove $N hard, pushing $M to the %s.", dirs[dir]);
  act(buf, FALSE, ch, 0, vict, TO_CHAR);
  snprintf(buf, sizeof(buf), "$n shoves You hard, pushing You to the %s.", dirs[dir]);
  act(buf, FALSE, ch, 0, vict, TO_VICT);
  snprintf(buf, sizeof(buf), "$n shoves $N hard, pushing $N to the %s.", dirs[dir]);
  act(buf, FALSE, ch, 0, vict, TO_NOTVICT);

  perform_move_full(vict, dir, false, false);

}

ACMD(do_evoweb)
{
  char arg[200];
  struct char_data *vict = NULL;

  one_argument(argument, arg, sizeof (arg));

  if (!HAS_EVOLUTION(ch, EVOLUTION_WEB))
  {
    send_to_char(ch, "You do not have the web evolution.\r\n");
    return;
  }

  if (!*arg)
  {
    if (!FIGHTING(ch))
    {
      send_to_char(ch, "You need to specify whom you wish to web.");
      return;
    }
    vict = FIGHTING(ch);    
  }

  if (!vict && (!(vict = get_char_room_vis(ch, arg, NULL))))
  {
    send_to_char(ch, "There is no one here by that name.\r\n");
    return;
  }

  act("You raise your spinneret at $N, spitting forth a stream of webbing.", TRUE, ch, 0, vict, TO_CHAR);
  act("$n raises $s spinneret at You, spitting forth a stream of webbing.", TRUE, ch, 0, vict, TO_VICT);
  act("$n raises $s spinneret at $N, spitting forth a stream of webbing.", TRUE, ch, 0, vict, TO_NOTVICT);

  call_magic(ch, vict, 0, SPELL_WEB, 0, GET_CALL_EIDOLON_LEVEL(ch), CAST_INNATE);

  USE_SWIFT_ACTION(ch);

}


ACMD(do_evobreath)
{

  int cooldown = 60;

  PREREQ_CAN_FIGHT();
  PREREQ_NOT_PEACEFUL_ROOM();

  if (!HAS_EVOLUTION(ch, EVOLUTION_ACID_BREATH) && !HAS_EVOLUTION(ch, EVOLUTION_FIRE_BREATH) &&
      !HAS_EVOLUTION(ch, EVOLUTION_COLD_BREATH) && !HAS_EVOLUTION(ch, EVOLUTION_ELECTRIC_BREATH))
  {
    send_to_char(ch, "You don't know how to do that.\r\n");
    return;
  }

  if (char_has_mud_event(ch, eEVOBREATH))
  {
    send_to_char(ch, "You are too exhausted to do that!\r\n");
    return;
  }

  act("You prepare your breath weapon.", TRUE, ch, 0, 0, TO_CHAR);
  act("$n prepares a breath weapon.", TRUE, ch, 0, 0, TO_ROOM);

  process_evolution_breath_damage(ch);

  cooldown -= MAX(0, num_evo_breaths(ch) - 1) * 18;
    
  attach_mud_event(new_mud_event(eEVOBREATH, ch, NULL), cooldown * PASSES_PER_SEC);
  USE_STANDARD_ACTION(ch);
}

ACMD(do_vital_strike)
{
  if (!HAS_FEAT(ch, FEAT_VITAL_STRIKE))
  {
    send_to_char(ch, "You do not know how to perform a vital strike.\r\n");
    return;
  }

  if (VITAL_STRIKING(ch))
  {
    VITAL_STRIKING(ch) = FALSE;
    act("You cease concentrating your attacks into a single vital strike!", TRUE, ch, 0, 0, TO_CHAR);
  }
  else
  {
    USE_FULL_ROUND_ACTION(ch);
    VITAL_STRIKING(ch) = TRUE;
    act("You concentrate your attacks into a single vital strike!", TRUE, ch, 0, 0, TO_CHAR);
  }

}

ACMD(do_strength_of_honor)
{

  struct affected_type af;
  int abil_mod = 0;
  int uses_remaining = 0;
  
  if (!HAS_REAL_FEAT(ch, FEAT_STRENGTH_OF_HONOR)) {
    send_to_char(ch, "You do not have the strength of honor ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, ABILITY_STRENGTH_OF_HONOR))
  {
    send_to_char(ch, "You are already benefitting from strength of honor.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_STRENGTH_OF_HONOR)) == 0)
  {
    send_to_char(ch, "You need to recover your strength in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Strength of Honor requires a swift action available to use.\r\n");
    return;
  }
  
  if (HAS_FEAT(ch, FEAT_STRENGTH_OF_HONOR))
  {
  	abil_mod = 4;
  }
  
  if (HAS_FEAT(ch, FEAT_MIGHT_OF_HONOR))
  {
  	abil_mod = 6;
  }
  
  new_affect(&af);
  af.location = APPLY_STR;
  af.bonus_type = BONUS_TYPE_MORALE;
  af.spell = ABILITY_STRENGTH_OF_HONOR;
  af.duration = 10 * (1 + HAS_REAL_FEAT(ch, FEAT_MIGHT_OF_HONOR));
  af.modifier = abil_mod;

  affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  
  send_to_char(ch, "You lift your weapon in a knightly salute and recite your oath of honor.\r\n");
  act("$n lifts $s weapon in a knightly salute and powerfully recites $s oath of honor.", false, ch, 0, 0, TO_NOTVICT);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STRENGTH_OF_HONOR);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_crown_of_knighthood)
{

  struct affected_type af[6];
  int uses_remaining = 0, i = 0;
  
  if (!HAS_REAL_FEAT(ch, FEAT_CROWN_OF_KNIGHTHOOD)) {
    send_to_char(ch, "You do not have the crown of knighthood ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, ABILITY_CROWN_OF_KNIGHTHOOD))
  {
    send_to_char(ch, "You are already benefitting from crown of knighthood.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_CROWN_OF_KNIGHTHOOD)) == 0)
  {
    send_to_char(ch, "You need to recover your strength in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Crown of knighthood requires a swift action available to use.\r\n");
    return;
  }
  
  /* init affect array */
  for (i = 0; i < 6; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = ABILITY_CROWN_OF_KNIGHTHOOD;
    af[i].duration = 75;
    af[i].modifier = 4;
    af[i].bonus_type = BONUS_TYPE_MORALE;
  }

  af[0].location = APPLY_HITROLL;
  af[1].location = APPLY_DAMROLL;
  af[2].location = APPLY_SAVING_FORT;
  af[3].location = APPLY_SAVING_WILL;
  af[4].location = APPLY_SAVING_REFL;
  af[5].location = APPLY_HIT;
  af[5].modifier = 20;

  for (i = 0; i < 6; i++)
    affect_join(ch, af + i, FALSE, FALSE, FALSE, FALSE);
  
  send_to_char(ch, "You close your eyes and allow the oath and measure to fill your soul.\r\n");
  act("$n closes $s eyes and seems to grow in confidence.", false, ch, 0, 0, TO_NOTVICT);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_CROWN_OF_KNIGHTHOOD);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_soul_of_knighthood)
{

  int uses_remaining = 0;
  
  if (!HAS_REAL_FEAT(ch, FEAT_SOUL_OF_KNIGHTHOOD)) {
    send_to_char(ch, "You do not have the soul of knighthood ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, SPELL_HOLY_AURA))
  {
    send_to_char(ch, "You are already benefitting from soul of knighthood/holy aura.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_SOUL_OF_KNIGHTHOOD)) == 0)
  {
    send_to_char(ch, "You need to recover your strength in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Soul of knighthood requires a swift action available to use.\r\n");
    return;
  }
  
  send_to_char(ch, "You close your eyes and connect with the triumvirate's power.\r\n");
  act("$n closes $s eyes and seems to grow in confidence.", false, ch, 0, 0, TO_NOTVICT);

  call_magic(ch, ch, 0, SPELL_HOLY_AURA, 0, CASTER_LEVEL(ch), CAST_INNATE);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_SOUL_OF_KNIGHTHOOD);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_rallying_cry)
{

  int uses_remaining = 0;
  
  if (!HAS_REAL_FEAT(ch, FEAT_RALLYING_CRY))
  {
    send_to_char(ch, "You do not have the rallying cry ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, AFFECT_RALLYING_CRY))
  {
    send_to_char(ch, "You are already benefitting from rallying cry.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_RALLYING_CRY)) == 0)
  {
    send_to_char(ch, "You need to recover your strength in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Rallying cry requires a swift action available to use.\r\n");
    return;
  }
  
  send_to_char(ch, "You raise your arm and rally your allies!\r\n");
  act("$n raises $s arm and rallies $s allies.", false, ch, 0, 0, TO_ROOM);

  call_magic(ch, ch, 0, AFFECT_RALLYING_CRY, 0, CASTER_LEVEL(ch), CAST_INNATE);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_RALLYING_CRY);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_inspire_courage)
{

  int uses_remaining = 0;
  
  if (!HAS_REAL_FEAT(ch, FEAT_INSPIRE_COURAGE))
  {
    send_to_char(ch, "You do not have the inspire courage ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, AFFECT_INSPIRE_COURAGE) || affected_by_spell(ch, AFFECT_INSPIRE_GREATNESS))
  {
    send_to_char(ch, "You are already benefitting from inspire courage.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_INSPIRE_COURAGE)) == 0)
  {
    send_to_char(ch, "You need to recover your strength in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Inspire courage requires a swift action available to use.\r\n");
    return;
  }
  
  send_to_char(ch, "You shout words of encouragement, bolstering the courage of your allies.\r\n");
  act("$n shouts words of encouragement to $s allies.", false, ch, 0, 0, TO_ROOM);

  if (HAS_FEAT(ch, FEAT_INSPIRE_GREATNESS))
    call_magic(ch, ch, 0, AFFECT_INSPIRE_GREATNESS, 0, CASTER_LEVEL(ch), CAST_INNATE);
  else
    call_magic(ch, ch, 0, AFFECT_INSPIRE_COURAGE, 0, CASTER_LEVEL(ch), CAST_INNATE);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_INSPIRE_COURAGE);

  USE_SWIFT_ACTION(ch);
}


ACMD(do_wisdom_of_the_measure)
{

  int uses_remaining = 0;
  
  if (!HAS_REAL_FEAT(ch, FEAT_WISDOM_OF_THE_MEASURE))
  {
    send_to_char(ch, "You do not have the wisdom of the measure ability.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_WISDOM_OF_THE_MEASURE)) == 0)
  {
    send_to_char(ch, "You need to recover your strength in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Wisdom of the measure requires a swift action available to use.\r\n");
    return;
  }
  
  send_to_char(ch, "You tap into your knowledge of the measure, and the power of Paladine offers you special insight.\r\n");
  
  spell_augury(CASTER_LEVEL(ch), ch, ch, 0, CAST_INNATE);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_WISDOM_OF_THE_MEASURE);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_final_stand)
{

  int uses_remaining = 0;
  
  if (!HAS_REAL_FEAT(ch, FEAT_FINAL_STAND))
  {
    send_to_char(ch, "You do not have the final stand ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, AFFECT_FINAL_STAND))
  {
    send_to_char(ch, "You are already benefitting from final stand.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_FINAL_STAND)) == 0)
  {
    send_to_char(ch, "You need to recover your strength in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Final stand requires a swift action available to use.\r\n");
    return;
  }
  
  send_to_char(ch, "You guide your allies into a defensive formation.\r\n");
  act("$n guides $s allies into a defensive formation.", false, ch, 0, 0, TO_ROOM);
  
  call_magic(ch, ch, 0, AFFECT_FINAL_STAND, 0, CASTER_LEVEL(ch), CAST_INNATE);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_FINAL_STAND);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_knighthoods_flower)
{

  int uses_remaining = 0;
  
  if (!HAS_REAL_FEAT(ch, FEAT_KNIGHTHOODS_FLOWER))
  {
    send_to_char(ch, "You do not have the knighthood's flower ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, AFFECT_KNIGHTHOODS_FLOWER))
  {
    send_to_char(ch, "You are already benefitting from knighthood's flower.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_KNIGHTHOODS_FLOWER)) == 0)
  {
    send_to_char(ch, "You need to recover your strength in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Knighthood's flower requires a swift action available to use.\r\n");
    return;
  }
  
  send_to_char(ch, "Recalling the Oath and the Measure fills you with courage and purpose!\r\n");
  act("$n looks filled with courage and purpose!", false, ch, 0, 0, TO_ROOM);
  
  call_magic(ch, ch, 0, AFFECT_KNIGHTHOODS_FLOWER, 0, CASTER_LEVEL(ch), CAST_INNATE);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_KNIGHTHOODS_FLOWER);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_foretell)
{

  int uses_remaining = 0;
  
  if (!HAS_REAL_FEAT(ch, FEAT_COSMIC_UNDERSTANDING))
  {
    send_to_char(ch, "You do not have the cosmic understanding feat, which bestows this ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, AFFECT_FORETELL))
  {
    send_to_char(ch, "You are already benefitting from foretell.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_COSMIC_UNDERSTANDING)) == 0)
  {
    send_to_char(ch, "You need to recover your strength in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Foretell requires a swift action available to use.\r\n");
    return;
  }
  
  send_to_char(ch, "Tapping into the power of the Vision, you peek into the immediate future!\r\n"
                   "You will get a +5 bonus on your next 5 attack rolls, saving throws, or skill checks.\r\n");
  
  call_magic(ch, ch, 0, AFFECT_FORETELL, 0, CASTER_LEVEL(ch), CAST_INNATE);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_COSMIC_UNDERSTANDING);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_prescience)
{

  int uses_remaining = 0;
  
  if (!HAS_REAL_FEAT(ch, FEAT_COSMIC_UNDERSTANDING))
  {
    send_to_char(ch, "You do not have the cosmic understanding feat, which bestows this ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, AFFECT_PRESCIENCE))
  {
    send_to_char(ch, "You are already benefitting from foretell.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_COSMIC_UNDERSTANDING)) == 0)
  {
    send_to_char(ch, "You need to recover your strength in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Foretell requires a swift action available to use.\r\n");
    return;
  }
  
  send_to_char(ch, "Tapping into the power of the Vision, you peek into the immediate future!\r\n"
                   "You and your allies gain a +2 luck bonus to attacks rolls saves, skill checks and caster level checks. Enemies gain -2 to the same.\r\n");
  
  call_magic(ch, ch, 0, AFFECT_PRESCIENCE, 0, CASTER_LEVEL(ch), CAST_INNATE);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_COSMIC_UNDERSTANDING);

  USE_SWIFT_ACTION(ch);
}

ACMD(do_gloryscall)
{

  int uses_remaining = 0;
  
  if (!HAS_DRAGON_BOND_ABIL(ch, 7, DRAGON_BOND_KIN))
  {
    send_to_char(ch, "You do not have the glory's call ability.\r\n");
    return;
  }

  if (affected_by_spell(ch, AFFECT_GLORYS_CALL))
  {
    send_to_char(ch, "You are already benefitting from glory's call.\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_DRAGOON_POINTS)) == 0)
  {
    send_to_char(ch, "You need to recover dragoon points in order to use this ability again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You have no uses in this ability.\r\n");
    return;
  }

  if (!is_action_available(ch, atSWIFT, TRUE))
  {
    send_to_char(ch, "Glory's call requires a swift action available to use.\r\n");
    return;
  }
  
  send_to_char(ch, "You raise your voice and call your allies to glory!\r\n");
  
  call_magic(ch, ch, 0, AFFECT_GLORYS_CALL, 0, GET_LEVEL(ch), CAST_INNATE);
  
  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_DRAGOON_POINTS);

  USE_SWIFT_ACTION(ch);
}

/* Water Whip - Monk Four Elements perk ability */
/* Prepares your next unarmed attack to deal bonus water damage and potentially entangle */

void perform_waterwhip(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_WATER_WHIP;
  af.duration = 24; /* Lasts 24 rounds or until used */

  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "\tBYou focus your Ki and conjure water around your fists, preparing a water whip strike.\tn\r\n");
  act("\tB$n's fists begin to shimmer with flowing water!\tn", FALSE, ch, 0, 0, TO_ROOM);
}

ACMDCHECK(can_waterwhip)
{
  ACMDCHECK_PERMFAIL_IF(!has_perk(ch, PERK_MONK_WATER_WHIP), "You don't know how to use the Water Whip technique.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_WATER_WHIP), "You have already prepared a water whip strike!\r\n");
  return CAN_CMD;
}

ACMD(do_waterwhip)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_waterwhip);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_waterwhip(ch);
}

/* Gong of the Summit - Monk Four Elements perk ability */
/* Prepares your next unarmed attack to deal bonus sound damage and potentially deafen */

void perform_gongsummit(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_GONG_OF_SUMMIT;
  af.duration = 24; /* Lasts 24 rounds or until used */

  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "\tYYou focus your Ki and prepare to strike with the reverberating power of a mighty gong.\tn\r\n");
  act("\tY$n's body begins to hum with resonant energy!\tn", FALSE, ch, 0, 0, TO_ROOM);
}

ACMDCHECK(can_gongsummit)
{
  ACMDCHECK_PERMFAIL_IF(!has_perk(ch, PERK_MONK_GONG_OF_SUMMIT), "You don't know how to use the Gong of the Summit technique.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_GONG_OF_SUMMIT), "You have already prepared a gong of the summit strike!\r\n");
  return CAN_CMD;
}

ACMD(do_gongsummit)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_gongsummit);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_gongsummit(ch);
}

/* Fist of Unbroken Air - AoE force attack that damages and potentially knocks down enemies */

void perform_fistair(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_FIST_OF_UNBROKEN_AIR;
  af.duration = 24; /* Lasts 24 rounds or until used */

  affect_to_char(ch, &af);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  send_to_char(ch, "\tCYou channel your Ki into a building wave of unstoppable force.\tn\r\n");
  act("\tCThe air around $n begins to shimmer and crackle with power!\tn", FALSE, ch, 0, 0, TO_ROOM);
}

ACMDCHECK(can_fistair)
{
  ACMDCHECK_PERMFAIL_IF(!has_perk(ch, PERK_MONK_FIST_OF_UNBROKEN_AIR), "You don't know how to use the Fist of Unbroken Air technique.\r\n");
  ACMDCHECK_TEMPFAIL_IF(affected_by_spell(ch, SKILL_FIST_OF_UNBROKEN_AIR), "You have already prepared a fist of unbroken air strike!\r\n");
  return CAN_CMD;
}

ACMD(do_fistair)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_fistair);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_fistair(ch);
}

/* Flowing River - AoE water attack that damages and extinguishes fire effects */

/* Callback for flowing river AoE effect */
int flowingriver_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  int dam;

  /* Calculate damage: 2d6 water damage */
  dam = dice(2, 6);

  /* Apply water damage with resistance checks */
  damage(ch, tch, dam, SKILL_FLOWING_RIVER, DAM_WATER, FALSE);

  /* Extinguish fire effects on the target */
  if (affected_by_spell(tch, SPELL_FIRE_SHIELD))
  {
    affect_from_char(tch, SPELL_FIRE_SHIELD);
    send_to_char(tch, "\tBThe flowing river extinguishes your fire shield!\tn\r\n");
  }
  if (affected_by_spell(tch, SPELL_FLAME_BLADE))
  {
    affect_from_char(tch, SPELL_FLAME_BLADE);
    send_to_char(tch, "\tBThe flowing river extinguishes your flame blade!\tn\r\n");
  }
  if (affected_by_spell(tch, SPELL_FIREBRAND))
  {
    affect_from_char(tch, SPELL_FIREBRAND);
    send_to_char(tch, "\tBThe flowing river extinguishes your firebrand!\tn\r\n");
  }
  if (AFF_FLAGGED(tch, AFF_ON_FIRE))
  {
    REMOVE_BIT_AR(AFF_FLAGS(tch), AFF_ON_FIRE);
    send_to_char(tch, "\tBThe flowing river extinguishes the flames that are burning you!\tn\r\n");
  }

  return TRUE;
}

void perform_flowingriver(struct char_data *ch)
{
  int targets_hit;

  send_to_char(ch, "\tBYou unleash a powerful wave of rushing water that crashes through the area!\tn\r\n");
  act("\tB$n unleashes a surging wave of water that crashes through the area!\tn", FALSE, ch, 0, 0, TO_ROOM);

  /* Use the centralized AoE system */
  targets_hit = aoe_effect(ch, SKILL_FLOWING_RIVER, flowingriver_callback, NULL);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  if (targets_hit == 0)
  {
    send_to_char(ch, "The wave crashes harmlessly with no enemies in range.\r\n");
  }
  else
  {
    send_to_char(ch, "The flowing river strikes %d enem%s!\r\n", targets_hit, targets_hit == 1 ? "y" : "ies");
  }
}

ACMDCHECK(can_flowingriver)
{
  ACMDCHECK_PERMFAIL_IF(!has_perk(ch, PERK_MONK_FLOWING_RIVER), "You don't know how to use the Flowing River technique.\r\n");
  return CAN_CMD;
}

ACMD(do_flowingriver)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_flowingriver);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_flowingriver(ch);
}

/* Sweeping Cinder Strike - Cone AoE fire attack that damages and sets targets on fire */

/* Callback for sweeping cinder strike AoE effect */
int sweepingcinder_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  int dam, save_level;
  struct affected_type af;

  /* Calculate damage: 2d6 fire damage */
  dam = dice(2, 6);

  /* Apply fire damage with resistance checks */
  damage(ch, tch, dam, SKILL_SWEEPING_CINDER_STRIKE, DAM_FIRE, FALSE);

  /* Calculate effective level for DC: WIS bonus + (monk level / 2)
   * savingthrow will add 10 + this level to get DC = 10 + WIS bonus + (monk level / 2) */
  save_level = GET_WIS_BONUS(ch) + (MONK_TYPE(ch) / 2);

  /* Check reflex save - if they fail, set them on fire */
  if (!savingthrow(ch, tch, SAVING_REFL, 0, CAST_INNATE, save_level, NOSCHOOL))
  {
    /* Apply on fire effect for 3 rounds */
    new_affect(&af);
    af.spell = SKILL_SWEEPING_CINDER_STRIKE;
    af.duration = 3;
    af.location = APPLY_NONE;
    SET_BIT_AR(af.bitvector, AFF_ON_FIRE);

    affect_to_char(tch, &af);

    act("\trYou are set ablaze by the sweeping flames!\tn", FALSE, ch, 0, tch, TO_VICT);
    act("\tr$N is set ablaze by your sweeping flames!\tn", FALSE, ch, 0, tch, TO_CHAR);
    act("\tr$N is set ablaze by $n's sweeping flames!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
  }
  else
  {
    act("You dodge the worst of the flames!", FALSE, ch, 0, tch, TO_VICT);
  }

  return TRUE;
}

void perform_sweepingcinder(struct char_data *ch)
{
  int targets_hit;

  send_to_char(ch, "\trYou unleash a sweeping cone of blazing cinder and flame!\tn\r\n");
  act("\tr$n unleashes a sweeping cone of blazing cinder and flame!\tn", FALSE, ch, 0, 0, TO_ROOM);

  /* Use the centralized AoE system */
  targets_hit = aoe_effect(ch, SKILL_SWEEPING_CINDER_STRIKE, sweepingcinder_callback, NULL);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  if (targets_hit == 0)
  {
    send_to_char(ch, "The flames sweep harmlessly with no enemies in range.\r\n");
  }
  else
  {
    send_to_char(ch, "The sweeping flames strike %d enem%s!\r\n", targets_hit, targets_hit == 1 ? "y" : "ies");
  }
}

ACMDCHECK(can_sweepingcinder)
{
  ACMDCHECK_PERMFAIL_IF(!has_perk(ch, PERK_MONK_SWEEPING_CINDER_STRIKE), "You don't know how to use the Sweeping Cinder Strike technique.\r\n");
  return CAN_CMD;
}

ACMD(do_sweepingcinder)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_sweepingcinder);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_sweepingcinder(ch);
}

/* Rush of the Gale Spirits - Creates a gust of wind that knocks down flying enemies and pushes back others */

/* Callback for gale rush AoE effect */
int galerush_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  int save_level, dam;
  struct affected_type af;

  /* Calculate effective level for DC: WIS bonus + (monk level / 2) */
  save_level = GET_WIS_BONUS(ch) + (MONK_TYPE(ch) / 2);

  /* Calculate damage: 3d6 air damage */
  dam = dice(3, 6);

  /* Apply air damage with resistance checks */
  damage(ch, tch, dam, SKILL_RUSH_OF_GALE_SPIRITS, DAM_AIR, FALSE);

  /* Check if target is flying */
  if (AFF_FLAGGED(tch, AFF_FLYING))
  {
    /* Flying creatures - check reflex save to avoid being knocked down */
    if (!savingthrow(ch, tch, SAVING_REFL, 0, CAST_INNATE, save_level, NOSCHOOL))
    {
      /* Failed save - knock them down and prevent flying */
      if (GET_POS(tch) > POS_SITTING)
      {
        change_position(tch, POS_SITTING);
        send_to_char(tch, "\tCThe powerful gust of wind knocks you out of the air!\tn\r\n");
        act("\tC$N is knocked out of the air by the gust of wind!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
      }
      
      /* Remove flying and prevent it for 1 minute (10 rounds) */
      REMOVE_BIT_AR(AFF_FLAGS(tch), AFF_FLYING);
      
      new_affect(&af);
      af.spell = SKILL_RUSH_OF_GALE_SPIRITS;
      af.duration = 10; /* 1 minute = 10 rounds */
      af.location = APPLY_SPECIAL;
      af.modifier = 1; /* Flag to indicate grounded by gale */
      
      affect_to_char(tch, &af);
      
      send_to_char(tch, "\tCThe violent winds prevent you from taking flight!\tn\r\n");
    }
    else
    {
      send_to_char(tch, "\tCYou struggle against the wind and maintain your flight!\tn\r\n");
    }
  }
  else
  {
    /* Non-flying creatures - push them back with some effects */
    send_to_char(tch, "\tCYou are buffeted by powerful winds!\tn\r\n");
    act("\tC$N is buffeted by the powerful winds!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
    
    /* Check reflex save to avoid minor knockback effects */
    if (!savingthrow(ch, tch, SAVING_REFL, 0, CAST_INNATE, save_level, NOSCHOOL))
    {
      /* Failed save - lose balance momentarily */
      if (GET_POS(tch) == POS_FIGHTING && rand_number(1, 3) == 1)
      {
        change_position(tch, POS_SITTING);
        send_to_char(tch, "\tCThe force of the wind knocks you off balance!\tn\r\n");
        act("\tC$N is knocked off balance by the wind!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
      }
    }
  }

  return TRUE;
}

void perform_galerush(struct char_data *ch)
{
  int targets_hit;

  send_to_char(ch, "\tCYou summon the spirits of the gale, unleashing a powerful gust of wind!\tn\r\n");
  act("\tC$n summons a powerful gust of wind that tears through the area!\tn", FALSE, ch, 0, 0, TO_ROOM);

  /* Use the centralized AoE system */
  targets_hit = aoe_effect(ch, SKILL_RUSH_OF_GALE_SPIRITS, galerush_callback, NULL);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);

  if (targets_hit == 0)
  {
    send_to_char(ch, "The wind howls harmlessly with no enemies in range.\r\n");
  }
  else
  {
    send_to_char(ch, "The gust affects %d enem%s!\r\n", targets_hit, targets_hit == 1 ? "y" : "ies");
  }
}

ACMDCHECK(can_galerush)
{
  ACMDCHECK_PERMFAIL_IF(!has_perk(ch, PERK_MONK_RUSH_OF_GALE_SPIRITS), "You don't know how to use the Rush of the Gale Spirits technique.\r\n");
  return CAN_CMD;
}

ACMD(do_galerush)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_galerush);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  perform_galerush(ch);
}

/* Clench of the North Wind - Single-target attack that deals cold damage and encases in ice */

void perform_clenchofnorthwind(struct char_data *ch, struct char_data *vict)
{
  int dam, save_level;
  struct affected_type af;

  if (!ch || !vict)
    return;

  send_to_char(ch, "\tCYou focus your ki and strike %s with the Clench of the North Wind!\tn\r\n", GET_NAME(vict));
  act("\tC$n strikes you with a freezing cold attack!\tn", FALSE, ch, 0, vict, TO_VICT);
  act("\tC$n strikes $N with a blast of freezing cold!\tn", FALSE, ch, 0, vict, TO_NOTVICT);

  /* Calculate damage: 2d6 cold damage */
  dam = dice(2, 6);

  /* Apply cold damage with resistance checks */
  damage(ch, vict, dam, SKILL_CLENCH_OF_NORTH_WIND, DAM_COLD, FALSE);

  /* Calculate effective level for DC: 10 + (monk level / 2) + WIS bonus */
  save_level = 10 + (MONK_TYPE(ch) / 2) + GET_WIS_BONUS(ch);

  /* Check reflex save to avoid ice encasement */
  if (!savingthrow(ch, vict, SAVING_REFL, 0, CAST_INNATE, save_level, NOSCHOOL))
  {
    /* Failed save - encase in ice */
    send_to_char(vict, "\tCYou are encased in a thick layer of ice and cannot move!\tn\r\n");
    act("\tC$N is encased in a thick layer of ice!\tn", FALSE, ch, 0, vict, TO_NOTVICT);
    act("\tC$N is encased in a thick layer of ice!\tn", FALSE, ch, 0, vict, TO_CHAR);

    /* Apply ice encasement effect: paralyzed, immune to cold, DR 5/- for 2 rounds */
    new_affect(&af);
    af.spell = SKILL_CLENCH_OF_NORTH_WIND;
    af.duration = 2; /* 2 rounds */
    SET_BIT_AR(af.bitvector, AFF_ENCASED_IN_ICE);
    SET_BIT_AR(af.bitvector, AFF_PARALYZED);
    affect_to_char(vict, &af);
  }
  else
  {
    send_to_char(vict, "\tCYou narrowly avoid being encased in ice!\tn\r\n");
    act("\tC$N resists the ice encasement!\tn", FALSE, ch, 0, vict, TO_NOTVICT);
  }

  /* Set cooldown: 1 minute (60 seconds) */
  if (!IS_NPC(ch))
  {
    ch->player_specials->saved.clench_of_north_wind_cooldown = time(0) + 60;
  }
}

ACMDCHECK(can_clenchofnorthwind)
{
  ACMDCHECK_PERMFAIL_IF(!has_perk(ch, PERK_MONK_CLENCH_NORTH_WIND), "You don't know how to use the Clench of the North Wind technique.\r\n");
  ACMDCHECK_TEMPFAIL_IF(!IS_NPC(ch) && ch->player_specials->saved.clench_of_north_wind_cooldown > time(0), 
    "You must wait before you can use this technique again.\r\n");
  return CAN_CMD;
}

ACMD(do_clenchofnorthwind)
{
  PREREQ_CAN_FIGHT();
  PREREQ_CHECK(can_clenchofnorthwind);
  PREREQ_HAS_USES(FEAT_STUNNING_FIST, "You must recover before you can focus your ki in this way again.\r\n");

  /* Set the timer - next melee attack will trigger the clench effect */
  GET_CLENCH_NORTH_WIND_TIMER(ch) = 1; /* Lasts 1 round - affects next attack */
  
  /* Set cooldown */
  if (!IS_NPC(ch))
  {
    ch->player_specials->saved.clench_of_north_wind_cooldown = time(0) + 60;
  }
  
  /* Use a ki point */
  start_daily_use_cooldown(ch, FEAT_STUNNING_FIST);
  
  act("You focus your ki, preparing to unleash the \tCClench of the North Wind\tn on your next strike!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n's hands glow with \tCicy energy\tn!", FALSE, ch, 0, 0, TO_ROOM);
  
  /* Use a swift action */
  USE_SWIFT_ACTION(ch);
}



/* Mass Cure Wounds - Paladin Divine Champion Tier 4 ability
 * Heals all allies in room for 3d8 + CHA modifier
 * 2 uses per day */
ACMD(do_masscurewounds)
{
  struct char_data *tch;
  int healing = 0;
  int cha_mod = GET_CHA_BONUS(ch);
  char buf[128];
  
  PREREQ_CAN_FIGHT();
  
  if (!has_paladin_mass_cure_wounds(ch))
  {
    send_to_char(ch, "You do not have that ability.\r\n");
    return;
  }
  
  /* Check daily uses - 2 per day using mud event */
  struct mud_event_data *pMudEvent = NULL;
  int uses_today = 0;
  
  if ((pMudEvent = char_has_mud_event(ch, eMASS_CURE_WOUNDS)))
  {
    if (pMudEvent->sVariables && sscanf(pMudEvent->sVariables, "%d", &uses_today) == 1)
    {
      if (uses_today >= 2)
      {
        send_to_char(ch, "You must recover the divine energy required for mass cure wounds.\r\n");
        return;
      }
    }
  }
  
  act("You call upon divine energy to heal your allies!", FALSE, ch, 0, 0, TO_CHAR);
  act("$n calls upon divine energy to heal $s allies!", FALSE, ch, 0, 0, TO_ROOM);
  
  /* Heal all allies in the room */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch == ch || is_player_grouped(ch, tch))
    {
      healing = dice(3, 8) + cha_mod;
      
      if (healing > 0)
      {
        GET_HIT(tch) = MIN(GET_MAX_HIT(tch), GET_HIT(tch) + healing);
        
        if (tch == ch)
          send_to_char(tch, "You are healed for %d hit points.\r\n", healing);
        else
          send_to_char(tch, "%s heals you for %d hit points.\r\n", GET_NAME(ch), healing);
      }
    }
  }
  
  /* Use action */
  USE_STANDARD_ACTION(ch);
  
  /* Track uses - increment count */
  if (pMudEvent)
  {
    uses_today++;
    if (pMudEvent->sVariables)
      free(pMudEvent->sVariables);
    snprintf(buf, sizeof(buf), "%d", uses_today);
    pMudEvent->sVariables = strdup(buf);
  }
  else
  {
    /* Create new event that lasts 1 MUD day */
    attach_mud_event(new_mud_event(eMASS_CURE_WOUNDS, ch, "1"), SECS_PER_MUD_DAY * PASSES_PER_SEC);
  }
}
