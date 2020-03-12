/**************************************************************************
 *  File: grapple.c                                    Part of LuminariMUD *
 *  Usage: grapple combat maneuver mechanics and related functions         *
 *  Author: Zusuk                                                          *
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
#include "grapple.h"

/* As a standard action, you can attempt to grapple a foe, hindering his combat
 * options. If you do not have Improved Grapple, grab, or a similar ability,
 * attempting to grapple a foe provokes an attack of opportunity from the target
 * of your maneuver. Humanoid creatures without two free hands attempting to
 * grapple a foe take a –4 penalty on the combat maneuver roll. If successful,
 * both you and the target gain the grappled condition. If you successfully
 * grapple a creature that is not adjacent to you, move that creature to an
 * adjacent open space (if no space is available, your grapple fails). Although
 * both creatures have the grappled condition, you can, as the creature that
 * initiated the grapple, release the grapple as a free action, removing the
 * condition from both you and the target. If you do not release the grapple,
 * you must continue to make a check each round, as a standard action, to
 * maintain the hold. If your target does not break the grapple, you get a +5
 * circumstance bonus on grapple checks made against the same target in
 * subsequent rounds. Once you are grappling an opponent, a successful check
 * allows you to continue grappling the foe, and also allows you to perform one
 * of the following actions (as part of the standard action spent to maintain the
 * grapple).
 **Move
 * You can move both yourself and your target up to half your speed. At the end
 * of your movement, you can place your target in any square adjacent to you. If
 * you attempt to place your foe in a hazardous location, such as in a wall of
 * fire or over a pit, the target receives a free attempt to break your grapple
 * with a +4 bonus.
 **Damage
 * You can inflict damage to your target equal to your unarmed strike, a natural
 * attack, or an attack made with armor spikes or a light or one-handed weapon.
 * This damage can be either lethal or nonlethal.
 **Pin
 * You can give your opponent the pinned condition (see Conditions). Despite
 * pinning your opponent, you still only have the grappled condition, but you
 * lose your Dexterity bonus to AC.
 **Tie Up
 * If you have your target pinned, otherwise restrained, or unconscious, you can
 * use rope to tie him up. This works like a pin effect, but the DC to escape
 * the bonds is equal to 20 + your Combat Maneuver Bonus (instead of your CMD).
 * The ropes do not need to make a check every round to maintain the pin. If you
 * are grappling the target, you can attempt to tie him up in ropes, but doing
 * so requires a combat maneuver check at a –10 penalty. If the DC to escape from
 * these bindings is higher than 20 + the target's CMB, the target cannot escape
 * from the bonds, even with a natural 20 on the check.
 **If You Are Grappled
 * If you are grappled, you can attempt to break the grapple as a standard action
 * by making a combat maneuver check (DC equal to your opponent's CMD; this does
 * not provoke an attack of opportunity) or Escape Artist check (with a DC equal
 * to your opponent's CMD). If you succeed, you break the grapple and can act
 * normally. Alternatively, if you succeed, you can become the grappler, grappling
 * the other creature (meaning that the other creature cannot freely release the
 * grapple without making a combat maneuver check, while you can). Instead of
 * attempting to break or reverse the grapple, you can take any action that doesn’t
 * require two hands to perform, such as cast a spell or make an attack or full
 * attack with a light or one-handed weapon against any creature within your reach,
 * including the creature that is grappling you. See the grappled condition for
 * additional details. If you are pinned, your actions are very limited. See the
 * pinned condition in Conditions for additional details.
 **Multiple Creatures
 * Multiple creatures can attempt to grapple one target. The creature that first
 * initiates the grapple is the only one that makes a check, with a +2 bonus for
 * each creature that assists in the grapple (using the Aid Another action). Multiple
 * creatures can also assist another creature in breaking free from a grapple, with
 * each creature that assists (using the Aid Another action) granting a +2 bonus
 * on the grappled creature's combat maneuver check.
 */

/* how does the mechanic work?  simple:  both grappler and grapled get the
 grapple affection (and all the penalties of it), there is one who is in the
 dominant position that has more choices/power on how to continue, and the
 person in the disadvantage, whom has less choices*/

/* check and cleanup grapple */
bool valid_grapple_cond(struct char_data *ch)
{
  bool valid_conditions = TRUE;

  if (!ch)
    return FALSE;

  /* vict of grapple, must have grapple affection flag */
  if (GRAPPLE_ATTACKER(ch) && !AFF_FLAGGED(ch, AFF_GRAPPLED))
  {
    GRAPPLE_ATTACKER(ch) = NULL;
    valid_conditions = FALSE;
  }

  /* grappler, must have grapple affection flag */
  if (GRAPPLE_TARGET(ch) && !AFF_FLAGGED(ch, AFF_GRAPPLED))
  {
    GRAPPLE_TARGET(ch) = NULL;
    valid_conditions = FALSE;
  }

  /* grapple affection with no variable set */
  if (AFF_FLAGGED(ch, AFF_GRAPPLED) && !GRAPPLE_TARGET(ch) &&
      !GRAPPLE_ATTACKER(ch))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GRAPPLED);
    valid_conditions = FALSE;
  } /* uh oh, how about grapple affects such as web spell? TODO:FIX! */

  /* same room check */
  if (GRAPPLE_ATTACKER(ch) && IN_ROOM(ch) != IN_ROOM(GRAPPLE_ATTACKER(ch)))
  {
    if (AFF_FLAGGED(ch, AFF_GRAPPLED))
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GRAPPLED);
    GRAPPLE_ATTACKER(ch) = NULL;
    valid_conditions = FALSE;
  }

  /* same room check */
  if (GRAPPLE_TARGET(ch) && IN_ROOM(ch) != IN_ROOM(GRAPPLE_TARGET(ch)))
  {
    if (AFF_FLAGGED(ch, AFF_GRAPPLED))
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GRAPPLED);
    GRAPPLE_TARGET(ch) = NULL;
    valid_conditions = FALSE;
  }

  /* position check? */

  return valid_conditions;
}
/* disengage ch from grappling vict */
void clear_grapple(struct char_data *ch, struct char_data *vict)
{
  if (ch)
  {
    GRAPPLE_ATTACKER(ch) = NULL;
    if (AFF_FLAGGED(ch, AFF_GRAPPLED))
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GRAPPLED);
    if (AFF_FLAGGED(ch, AFF_PINNED))
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PINNED);
  }
  if (vict)
  {
    GRAPPLE_TARGET(vict) = NULL;
    if (AFF_FLAGGED(vict, AFF_GRAPPLED))
      REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_GRAPPLED);
    if (AFF_FLAGGED(vict, AFF_PINNED))
      REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_PINNED);
  }
}

/* called by pulse_luminari to make sure we don't have extraneous funky
   grappling situations */
void grapple_cleanup(struct char_data *ch)
{
  if (!ch)
    return;
  struct char_data *vict = GRAPPLE_ATTACKER(ch);
  //struct char_data *vict = GRAPPLE_TARGET(ch);
  if (!valid_grapple_cond(ch) && vict)
    /* cleanup! */
    clear_grapple(ch, vict);
}

/* set ch grappling vict, with ch in the dominant position */
void set_grapple(struct char_data *ch, struct char_data *vict)
{
  GRAPPLE_TARGET(ch) = vict;
  if (!AFF_FLAGGED(ch, AFF_GRAPPLED))
    SET_BIT_AR(AFF_FLAGS(ch), AFF_GRAPPLED);
  GRAPPLE_ATTACKER(vict) = ch;
  if (!AFF_FLAGGED(vict, AFF_GRAPPLED))
    SET_BIT_AR(AFF_FLAGS(vict), AFF_GRAPPLED);
}

/* set ch pinning vict, with ch in the dominant position */
/* pinning makes your dex bonus -5, and you are treated as if you have no
 dex bonus for the sake of sneak attacks etc */
void set_pin(struct char_data *ch, struct char_data *vict)
{
  if (!AFF_FLAGGED(vict, AFF_PINNED))
    SET_BIT_AR(AFF_FLAGS(vict), AFF_PINNED);
  change_position(vict, POS_RECLINING);
}

/* primary grapple and reversal entry point */
ACMD(do_grapple)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (IS_NPC(ch))
  {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!*arg)
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
  }
  else
    vict = get_char_room_vis(ch, arg, NULL);

  if (!vict)
  {
    send_to_char(ch, "Grapple who?\r\n");
    return;
  }
  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  /* try for reversale? */
  if (GRAPPLE_ATTACKER(ch) && GRAPPLE_ATTACKER(ch) == vict &&
      AFF_FLAGGED(ch, AFF_GRAPPLED))
  {
    /* cmb, escape artist: check which is better is done in compute_cmb() */
    if (combat_maneuver_check(ch, vict, COMBAT_MANEUVER_TYPE_REVERSAL, 0) > 0)
    {
      /* reversal! */
      if (AFF_FLAGGED(ch, AFF_PINNED))
      {
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PINNED);
        act("\tyYou break $N's pin on you!\tn", FALSE, ch, NULL, vict, TO_CHAR);
        act("\ty$n breaks your pin!\tn", FALSE, ch, NULL, vict, TO_VICT);
        act("\ty$n breaks $N's pin!!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
      }
      GRAPPLE_TARGET(ch) = vict;
      GRAPPLE_ATTACKER(ch) = NULL;
      GRAPPLE_TARGET(vict) = NULL;
      GRAPPLE_ATTACKER(vict) = ch;
      act("\tyYou release purposely tensed muscles to create a little "
          "space then deftly reverse the grapple, assuming dominant position "
          "over $N!\tn",
          FALSE, ch, NULL, vict, TO_CHAR);
      act("\ty$n creates a miniscule amount of space, enough to deftly reverse "
          "$s position over you!\tn",
          FALSE, ch, NULL, vict, TO_VICT);
      act("\ty$n creates a little space then deftly reverses the grapple, assuming "
          "dominant position over $N!\tn",
          FALSE, ch, NULL, vict, TO_NOTVICT);
    }
    else
    {
      /* failed reversal */
      act("\tyYou fail to reverse the grapple $N has on you!\tn", FALSE, ch, NULL, vict, TO_CHAR);
      act("\tyYou maneuver yourself perfectly to stop $n's attempt to reverse the grapple!\tn", FALSE, ch, NULL, vict, TO_VICT);
      act("\ty$N maneuvers perfectly to avoid a reversal attempt from $n!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
    }
    USE_STANDARD_ACTION(ch);
    return;
  }
  else if (GRAPPLE_TARGET(ch) || GRAPPLE_ATTACKER(ch))
  {
    send_to_char(ch, "You are already grappling with someone!!\r\n");
    return;
  }
  else
  { /* attempt to grapple */
    if (GET_POS(ch) <= POS_SITTING)
    {
      send_to_char(ch, "You need to stand to grapple!\r\n");
      return;
    }

    int grapple_penalty = 0;

    /* aoo damage becomes a penalty */
    if (!HAS_FEAT(ch, FEAT_IMPROVED_GRAPPLE))
    {
      grapple_penalty = attack_of_opportunity(vict, ch, 0);
    }

    if (combat_maneuver_check(ch, vict, COMBAT_MANEUVER_TYPE_INIT_GRAPPLE, -(grapple_penalty)) > 0)
    {
      /* success! */
      act("\tyAn opportune moment presents itself, you quickly lunge towards $N "
          "engaging with a successful grapple!!\tn",
          FALSE, ch, NULL, vict, TO_CHAR);
      act("\tyYou are caught off guard as $n lunges toward you engaging you in a "
          "grapple!\tn",
          FALSE, ch, NULL, vict, TO_VICT);
      act("\tyAn opportune moment presents itself, $n quickly lunge towards $N "
          "engaging with a successful grapple!!\tn",
          FALSE, ch, NULL, vict, TO_NOTVICT);
      set_grapple(ch, vict);
    }
    else
    {
      /* failure! */
      act("\tyYou fail to grapple $N!\tn", FALSE, ch, NULL, vict, TO_CHAR);
      act("\tyYou deftly avoid a grapple attempt from $n\tn", FALSE, ch, NULL, vict, TO_VICT);
      act("\ty$n fails to grapple $N!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
    }
    USE_STANDARD_ACTION(ch);

    if (vict != ch)
    { /* make sure combat starts */
      if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
        set_fighting(ch, vict);
      if (GET_POS(vict) > POS_STUNNED && (FIGHTING(vict) == NULL))
      {
        set_fighting(vict, ch);
      }
    }

    return;
  }
}

/* as a switf action, attempt to free oneself from grapple, can attempt once per
 * round, note: as soon as swift action is fixed, this should use a swift action
 * as opposed to the current setup */
ACMD(do_struggle)
{
  if (!GRAPPLE_ATTACKER(ch) || !AFF_FLAGGED(ch, AFF_GRAPPLED))
  {
    send_to_char(ch, "But you are not the victim of grapple!\r\n");
    return;
  }

  if (char_has_mud_event(ch, eSTRUGGLE))
  {
    send_to_char(ch, "You can only attempt to struggle (free action), once per "
                     "round.\r\n");
    return;
  }

  struct char_data *vict = GRAPPLE_ATTACKER(ch);

  if (combat_maneuver_check(ch, vict, COMBAT_MANEUVER_TYPE_REVERSAL, 0) > 0)
  {
    /* success, defender escapes! */
    act("\tyYou succeed in escaping the grapple from $N!\tn", FALSE, ch, NULL, vict, TO_CHAR);
    act("\ty$n succeeds in escaping from your grapple!\tn", FALSE, ch, NULL, vict, TO_VICT);
    act("\ty$n succeed in escaping the grapple from $N!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
    clear_grapple(ch, vict);
  }
  else
  {
    /* failed, continue grapple */
    act("\tyYou fail to escape the grapple from $N!\tn", FALSE, ch, NULL, vict, TO_CHAR);
    act("\ty$n fails to escape your grapple!\tn", FALSE, ch, NULL, vict, TO_VICT);
    act("\ty$n fails to escape the grapple from $N!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
  }
  /* gotta make sure we don't allow this more than once a around */
  attach_mud_event(new_mud_event(eSTRUGGLE, ch, NULL), (6 * PASSES_PER_SEC));
}

/* as a free action, release your grapple victim */
ACMD(do_free_grapple)
{
  if (!ch)
    return;
  struct char_data *vict = GRAPPLE_TARGET(ch);
  if (!vict)
    return;
  clear_grapple(vict, ch);
  act("\tyYou release $N from your grapple!\tn", FALSE, ch, NULL, vict, TO_CHAR);
  act("\ty$n releases you from the grapple!\tn", FALSE, ch, NULL, vict, TO_VICT);
  act("\ty$n releases $N from a grapple!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
}

/* as a standard action, try to pin grappled opponent */
ACMD(do_pin)
{
  if (IS_NPC(ch))
  {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  if (GRAPPLE_ATTACKER(ch))
  {
    send_to_char(ch, "You have to be in the dominant position of a grapple to attempt a pin!\r\n");
    return;
  }

  if (GRAPPLE_TARGET(ch) && AFF_FLAGGED(ch, AFF_GRAPPLED) && AFF_FLAGGED(GRAPPLE_TARGET(ch), AFF_GRAPPLED))
    ; /* good conditions */
  else
  {
    /* conditions are not met */
    send_to_char(ch, "You need to be in the dominant position of a grapple to attempt a pin!\r\n");
    return;
  }

  /* we're assuming we are now in a position to attempt a pin */
  int grapple_penalty = 4;
  struct char_data *vict = GRAPPLE_TARGET(ch);

  if (vict == ch)
    return; /*huh?*/

  /* aoo damage becomes a penalty */
  if (!HAS_FEAT(ch, FEAT_IMPROVED_GRAPPLE))
  {
    grapple_penalty += attack_of_opportunity(vict, ch, 0);
  }

  if (combat_maneuver_check(ch, vict, COMBAT_MANEUVER_TYPE_PIN, -(grapple_penalty)) > 0)
  {
    /* success! */
    act("\tyYou take $N to the ground with a pin maneuver!\tn", FALSE, ch, NULL, vict, TO_CHAR);
    act("\ty$n takes you to the ground with a pin maneuver!\tn", FALSE, ch, NULL, vict, TO_VICT);
    act("\ty$n takes $N to the ground with a pin maneuver!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
    set_pin(ch, vict);
  }
  else
  {
    /* failure! */
    act("\tyYou fail to pin $N!\tn", FALSE, ch, NULL, vict, TO_CHAR);
    act("\tyYou deftly avoid a pin attempt from $n\tn", FALSE, ch, NULL, vict, TO_VICT);
    act("\ty$n fails to pin $N!\tn", FALSE, ch, NULL, vict, TO_NOTVICT);
  }
  USE_STANDARD_ACTION(ch);

  if (vict != ch)
  { /* make sure combat starts */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, vict);
    if (GET_POS(vict) > POS_STUNNED && (FIGHTING(vict) == NULL))
    {
      set_fighting(vict, ch);
    }
  }

  return;
}

/* as a standard action, try to bind pinned opponent */
ACMD(do_bind)
{
  if (IS_NPC(ch))
  {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  send_to_char(ch, "Under construction.\r\n");
  return;
}

/*EOF*/
