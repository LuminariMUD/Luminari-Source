/*
 * File:   grapple.h
 * Author: Zusuk
 *
 * Created: 05/21/2015
 */

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

#ifndef GRAPPLE_H
#define GRAPPLE_H

#include "utils.h" /* for the ACMD macro */

#ifdef __cplusplus
extern "C"
{
#endif

    /* functions */
    void grapple_cleanup(struct char_data *ch);
    void clear_grapple(struct char_data *ch, struct char_data *vict);

    /* Functions with subcommands */

    /* Functions without subcommands */
    ACMD(do_grapple);
    ACMD(do_struggle);
    ACMD(do_free_grapple);
    ACMD(do_bind);
    ACMD(do_pin);

/* Macros */
#define GRAPPLE_TARGET(ch) ((ch)->char_specials.grapple_target)
#define GRAPPLE_ATTACKER(ch) ((ch)->char_specials.grapple_attacker)
#define GRAPPLING(ch) (GRAPPLE_TARGET(ch))
#define GRAPPLED(ch) (GRAPPLE_ATTACKER(ch))

#ifdef __cplusplus
}
#endif

#endif /* GRAPPLE_H */
