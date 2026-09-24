# Extra Limb Mechanics

Status: record of the code, verified 2026-09-24; nothing here proposes a change.

LuminariMUD has no limb-count system. Three separate mechanics, each hard-coded, give a character
more arms or hands, and none of them takes a number of limbs. How the two racial feats work in
full is in
[GAME_MECHANICS_SYSTEMS.md](../systems/GAME_MECHANICS_SYSTEMS.md#racial-innate-feats-and-spell-like-abilities);
their race point prices are in
[PLAYER_RACES_REFERENCE.md](../guides/PLAYER_RACES_REFERENCE.md#race-point-rp-pricing-table).

| Mechanic | Hands | Wear slots | Attacks | Scales with a count |
| -- | -- | -- | -- | -- |
| Extra arms (`FEAT_EXTRA_ARMS`, stackable) | none | none | one full-bonus melee attack per rank | yes, attacks only |
| Four arms (`FEAT_FOUR_ARMS`) | +2 | +7 | a mirrored second pair | no, exactly two pairs |
| Vestigial arm (`ALC_DISC_VESTIGIAL_ARM`) | +1 | none | none | no |

## Extra arms

`FEAT_EXTRA_ARMS` (1320) is the general "one more arm" trait. In `perform_attacks()`
(`src/combat/fight.c`), each rank adds one main-hand bonus attack at full base attack bonus. It
runs after the ranged attack routines, so bows and thrown weapons never gain it. It adds no hands
and no equipment slots. It costs 3 race points per rank, and no race grants it.

## Four arms

`FEAT_FOUR_ARMS` (1321) is the Thri-Kreen mechanic from issue #168. It is on or off; ranks do
nothing.

- Capability. `has_four_arms()` in `src/core/utils.c` reads the character's own feat, an
  `APPLY_FEAT` item worn in an ordinary slot, or a mob feat for NPCs and disguised wild shapes. An
  item in one of the four-arm slots cannot provide the arms.
- Hands. `hands_have()` in `src/obj/act.item.c` adds two hands.
- Slots. It adds seven wear positions after the old tail: `WEAR_WIELD_3` (44), `WEAR_WIELD_4`,
  `WEAR_WIELD_2H_2`, `WEAR_ARMS_2`, `WEAR_HANDS_2`, `WEAR_WRIST_R2`, and `WEAR_WRIST_L2` (50), with
  `NUM_WEARS` at 51. A four-armed character wears four weapons, two sleeves, two gloves, and four
  wrist items. Every other slot, rings and ears included, is unchanged.
- Weapon pairs. Each pair holds two one-handers or one two-hander, never both. The second pair
  takes melee weapons only (`second_pair_rejects_object()` in `src/core/utils.c`), so shields,
  held items, and ranged weapons stay on the first pair. `perform_wear_impl()` and `equip_char()`
  both enforce this.
- Attacks. `perform_second_pair_attacks()` in `src/combat/fight.c` runs after the first pair
  each round and mirrors its base, haste, and bonus attacks as `ATTACK_TYPE_THIRD` and
  `ATTACK_TYPE_FOURTH`. Each rolls at 50 percent, 75 with two-weapon fighting, and 100 with
  improved two-weapon fighting. The second pair uses its own two-weapon penalties and two-hander
  rules.
- Losing the arms. `four_arms_reconcile()` in `src/obj/act.item.c`, called from `affect_total()`,
  moves the seven slots into inventory and trims held items back to two hands.
- Saving. Saved object locations 45 to 51 restore into the new slots. `save_char()` defers
  reconciliation while it unequips and re-equips, so saving never moves gear.

### Four arms on a monk

Duris allows Thri-Kreen only the warrior class, but LuminariMUD has no race class limits (#166),
so a Thri-Kreen monk or sacred fist (`MONK_TYPE()`) is possible. Four arms gives it extra
attacks only when the second pair holds a weapon, and a weapon there ends bare-handed fighting:

| Loadout | Second pair attacks | Monk benefits |
| -- | -- | -- |
| All four hands empty | none: `perform_second_pair_attacks()` needs a weapon in the third hand | all kept |
| Monk-family weapons in the second pair | mirrors base, haste, and every bonus attack, flurry of blows included | flurry, monk gear, and monk-weapon perks kept; bare-hand bonuses lost |
| Other weapons in the second pair | as above, but without flurry | `monk_gear_ok()` fails, so flurry and monk gear benefits are lost |

- Bare hands. `is_bare_handed()` (`src/combat/assign_wpn_armor.c`) requires the second pair to be
  empty too. Losing it drops the monk glove damage bonus and the unarmed critical range.
- Monk weapons. `monk_gear_ok()` accepts monk-family weapons in either pair. A monk rolls the
  better of the weapon's dice and its unarmed dice with them, so the second pair hits with monk
  unarmed damage.
- Hit chance. The second pair rolls at 50 percent. Flurry does not count as two-weapon training,
  so it rises to 75 and 100 percent only with the two-weapon fighting feats. The fourth hand
  attacks only when the second pair holds two one-handers.
- First pair only. The monk glove bonus reads only `WEAR_HANDS`, so the second gloves add their
  stats and nothing more. The One With Wood and Stone armor bonus reads only a first-pair weapon.

Four arms costs 15 race points, and as a trait unique to Thri-Kreen it is exempt from the
single-trait cap. The price assumes an armed second pair; an unarmed monk gets only the slots. The [Duris race specifications](DURIS_RACE_SPECIFICATIONS.md#thri-kreen) place
it on Thri-Kreen.

## Vestigial arm

`ALC_DISC_VESTIGIAL_ARM` is an alchemist discovery, available at alchemist level 9. It adds one
hand in `hands_have()`, so the character can hold one more item with the existing slots, and
looking at the character shows the extra arm. It adds no slot and no attack.

## What a limb count would need

A race with six or more arms, or a variable number of arms, is new code rather than data:

- a third set of wear positions, with saved-object locations and `NUM_WEARS` raised again;
- new attack types past `ATTACK_TYPE_FOURTH`, and a third-pair pass in `perform_attacks()`;
- pair rules for the new weapons in `perform_wear_impl()`, `equip_char()`, and the two-hander and
  two-weapon checks;
- `four_arms_reconcile()` and the `save_char()` deferral generalized past the second pair;
- a hand count in `hands_have()` that comes from the limb count instead of the fixed +2.

[ARM_COUNT_PLAN.md](ARM_COUNT_PLAN.md) plans that system with equipment capped at four arms.
