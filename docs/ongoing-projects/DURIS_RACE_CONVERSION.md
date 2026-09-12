# Duris Race Conversion Study

Status: source-backed study, verified 2026-09-11 against the Duris checkout at
`/home/aiwithapex/projects/duris` (`src/core/defines.h`, `src/core/constant.c`,
`src/core/common.c`, `src/classes/innates.c`, `src/combat/dam_mods.c`,
`src/combat/fight.c`, `src/magic/affects.c`, `src/world/limits.c`,
`src/world/handler.c`, `src/world/db.c`, `src/account/nanny.c`,
`src/cmd/actobj.c`, `lib/duris.properties`, and
`help/duris_help_parsed.hlp`). Scoring uses the race point (RP) table in
[PLAYER_RACES_REFERENCE.md](../guides/PLAYER_RACES_REFERENCE.md), section
"Balance: race point budgets by tier".

Follow-up: [DURIS_RACIAL_MECHANICS.md](DURIS_RACIAL_MECHANICS.md)
plans the uncovered innates below as non-selectable feats.

This document converts every Duris player race to LuminariMUD's scale so the
team can decide which ones are worth adapting. It keeps each race's stat
profile proportionate to the Duris original, carries over every innate, and
scores the result with our RP table. Nothing here changes code or the
registry. It is an evaluation input, not a design commitment.

## What counts as a Duris player race

Duris defines 37 player races (`RACE_HUMAN` 1 through `RACE_TIEFLING` 37,
`RACE_PLAYER_MAX`). They fall into four availability groups, which are the
closest thing Duris has to our tiers:

| Duris group | Races | How acquired | Our nearest tier |
|-------------|-------|--------------|------------------|
| Creation roster, good side | Human, Barbarian, Grey Elf, Mountain Dwarf, Halfling, Gnome, Centaur, Githzerai, Firbolg | Character creation | Normal or Advanced |
| Creation roster, evil side | Drow, Duergar, Ogre, Troll, Orc, Githyanki, Goblin, Kobold, Drider | Character creation | Normal or Advanced |
| Creation roster, neutral (player picks a side) | Thri-Kreen, Minotaur, Tiefling | Character creation | Normal or Advanced |
| Descend forms | Lich, Vampire, Death Knight, Wight, Revenant, Shadow Beast, Phantom, Shade | `descend` from a level 50 character of a specific class; resets to level 1 and costs 2000 epic points | Epic quest |
| Legacy | Half-Elf, Wood Elf, Kuo-Toa, Orog | Only with `CREATION_ALL_RACES=TRUE` | Varies |
| Lore-restricted | Harpy, Illithid, Half-Illithid (Pillithid), Storm Giant | Only with `CREATION_ALL_RACES=TRUE`; three have no racewar side | Varies |

The roster comes from `playable_races[]` and `restricted_races[]` in
`src/core/constant.c`. The `descend` command in `src/cmd/actoth.c` currently
prints "This command has been disabled until further notice" after its checks,
so the descend forms are not reachable in the live Duris build either.

## How Duris builds a race

Duris has no ability modifiers. A race is a set of multipliers and innates,
almost all read from `lib/duris.properties` at boot:

| Duris lever | Where | What it does |
|-------------|-------|--------------|
| Stat factors | `stats.<stat>.<Race>` (loaded by `update_stat_data()` in `src/world/db.c`) | Percent multiplier on each of ten stats: Str, Dex, Agi, Con, Pow, Int, Wis, Cha, Kar, Luc. Human is 100 on all. Ogre Str is 230, Illithid Pow is 200 |
| Stat bonus | `stats.bonus.<Race>` (`add_racial_stat_bonus()` in `src/magic/affects.c`) | Flat add to every stat. Only Human has one (+5) |
| Damroll modifier | `damage.damrollModifier.racial.<Race>` | Multiplier on damroll. 0.45 (Illithid) to 1.5 (Ogre) |
| Total output | `damage.totalOutput.racial.<Race>` | Multiplier on total melee damage. 0.6 (Illithid) to 2.2 (Wight) |
| Combat pulse | `damage.pulse.racial.<Race>` | Attack round length. Human 15; lower is faster. Gnome 11.5, Ogre 19 |
| Spellcast pulse | `spellcast.pulse.racial.<Race>` | Casting time multiplier. Human 1.0; lower is faster. Illithid 0.025 |
| Experience factor | `exp.factor.<Race>` (`update_racial_exp_mods()` in `src/net/sparser.c`) | Multiplier on experience gained. Human 1.3, Orc 1.15, most evil races 0.8 to 0.85, Lich and Illithid 0.1. This is Duris's enforced level adjustment |
| Shrug | `innate.shrug.<Race>` (`get_innate_resistance()` in `src/classes/innates.c`) | Percent chance to ignore a spell, scaled by level up to 50 and never below 5. Lich 65, Illithid 50, elves 35 |
| Racial saves | `saves.<spell,breath,para,petri>.racial.<Race>` | Flat save bonuses for a handful of races |
| Innates | `ADD_RACIAL_INNATE()` calls in `src/classes/innates.c` | Passive and active abilities, many gated by level (Duris mortals go to 56) |
| Size | Player creation switch in `src/account/nanny.c` (`race_size()` in `src/core/common.c` is the mob table and disagrees for several player races) | Tiny to Gargantuan. Player races are Small, Medium, Large, or Huge |
| Giant wield | `IS_GIANT()` in `src/core/utils.h`, `wield_item_size()` in `src/cmd/actobj.c` | Ogre, Minotaur, Firbolg, and Storm Giant treat every weapon as one-handed. A two-handed weapon keeps its 1.5x dice multiplier (`damage.modifier.twohanded` in `src/combat/fight.c`) and leaves the other hand free for a shield or second weapon. This is keyed to race, not size: Wight is Huge and does not get it |
| Regeneration | `hit.regen.Troll` 9, `hit.regen.Revenant` 4 (`get_innate_regeneration()`) | Adds mult+1 hit points per tick standing, doubled resting, tripled sleeping |
| Sun vulnerability | `INNATE_VULN_SUN` (`src/world/limits.c`, `sun_damage_check()` in `src/world/handler.c`) | Zero hit point and movement regeneration in sunlight, plus 5 to 20 damage per check outdoors in daylight. Forests, swamps, and globe of darkness suppress it |
| Dayblind | `INNATE_DAYBLIND` | Cannot see in daylight. Help text lists it on many races, but the registration is commented out for all of them except Half-Illithid |

Racial spell damage type factors (`damage.spellTypeMod.*.racial.*`) are defined
only for Human and are all 1.0, so they are not a lever in practice.

## Conversion rules

The goal is a line our registry could hold, with the Duris proportions intact.

**Ability modifiers.** Every 10 percent of stat factor above or below 100 is
one point of modifier, rounded half away from zero. Our six stats map as:

| Ours | From Duris |
|------|------------|
| STR | Str |
| CON | Con |
| INT | Int |
| WIS | Wis |
| DEX | Average of Dex and Agi (Duris splits hand speed from body speed) |
| CHA | Cha |

Pow (spell power), Luc (luck), and Kar (karma) have no stat of ours and are
excluded from the conversion and the score entirely. They remain visible in
Table 1 for reference only.

**Melee multiplier.** Damroll modifier, total output, and combat pulse are
combined into one effective melee factor: `M = totalOutput x damroll x
(15 / pulse)`. Each 10 percent of M above 1.0 is priced at 0.75 RP (between
Crystal Fist's flat +3 for 2 RP and a scaling damage trait). Each 10 percent
below refunds 0.5 RP. Tiefling has no damroll or total output property in
Duris, so both default to 1.0.

**Casting speed.** `S = 1 / spellcastPulse`. Each 10 percent faster than human
is 0.5 RP, capped at +10 steps; each 10 percent slower refunds 0.25 RP, capped
at -5 steps. The cap matters only for Illithid, whose 0.025 pulse is effectively
instant casting and would otherwise score 195 RP.

**Shrug to spell resistance.** Shrug 50 or more is priced as SR 15 + level
(6 RP), 35 to 40 as SR 10 + level (4 RP), 20 to 25 as SR 5 + half level (2 RP),
and 5 as 0.5 RP.

**Size.** Duris Small stays Small (0 RP). Duris Large and Huge both become our
Large (1 RP); we have no Huge player size and size itself carries no extra
mechanics in Duris beyond size comparisons. The giant wield rule is priced
separately as a trait (3 RP: a two-handed weapon at full 1.5x dice in one hand
plus a shield or off-hand weapon) for the four races that have it.

**Innates.** Each is priced by the closest row of our trait table. Where the
Duris effect is numerically known (weapon masters, troll skin, regeneration,
sun damage) the price follows the number; where only help text describes it,
the price follows the analogous feat. Innates with no implementation in the
Duris source (Summon Totem, Project Image) are listed at 0.

**Drawbacks.** Sun vulnerability is -3 (worse than our Light Blindness at -2,
milder than Vampire Weaknesses at -4). Fire vulnerability x1.3 and cold
vulnerability x1.35 are -2. Equipment slot losses are -1 per slot, capped at
-6. Our 25 percent refund cap and 4 point ability penalty cap apply.

**Experience factor.** Not converted into RP. It is reported alongside the
score because it is Duris's own statement of where the race sits, and the two
should agree.

## Table 1: Duris source values

Stats are Str/Dex/Agi/Con/Pow/Int/Wis/Cha/Luc as percent of human. Size is
the player-creation size (S/M/L/H). Multipliers
are damroll / total output / combat pulse / spellcast pulse. Exp is the
experience factor; "none" means the property is absent and the code default of
1.0 applies.

| Race | Stats | Size | Multipliers | Exp | Shrug | Roster |
|------|-------|------|-------------|-----|-------|--------|
| Human | 100/100/100/100/100/100/100/100/100 | M | 1.0/1.1/15/1.0 | 1.3 | 0 | good |
| Barbarian | 155/90/90/165/70/70/95/75/90 | L | 1.15/1.25/18.25/1.3 | none | 0 | good |
| Grey Elf | 90/110/120/90/105/115/120/120/100 | M | 0.77/0.9/13.5/0.825 | 0.8 | 35 | good |
| Mountain Dwarf | 135/95/85/120/90/85/125/80/100 | M | 1.12/1.2/17/0.85 | 0.9 | 0 | good |
| Halfling | 95/130/125/95/80/100/115/110/120 | S | 0.85/1.0/12.5/0.7 | 0.9 | 0 | good |
| Gnome | 85/115/120/90/105/130/95/95/100 | S | 0.8/0.9/11.5/0.6 | 0.9 | 0 | good |
| Centaur | 140/90/90/155/65/80/100/90/90 | L | 1.18/1.2/15.5/1.2 | 0.9 | 0 | good |
| Githzerai | 100/100/100/100/120/115/110/90/100 | M | 0.975/0.95/15/0.9 | 0.9 | 20 | good |
| Firbolg | 230/75/75/200/65/75/80/80/90 | H | 1.45/1.7/15.5/1.5 | 0.85 | 0 | good |
| Drow | 90/110/130/90/110/120/115/110/100 | M | 0.75/0.9/13.5/0.825 | 0.8 | 35 | evil |
| Duergar | 130/90/90/135/85/75/130/70/100 | M | 1.11/1.2/17/0.85 | 0.85 | 0 | evil |
| Ogre | 230/75/75/200/60/70/80/50/90 | H | 1.5/1.8/19/1.6 | 0.85 | 0 | evil |
| Troll | 160/90/100/160/75/75/90/70/90 | L | 1.18/1.3/18.25/1.4 | 0.85 | 0 | evil |
| Orc | 120/100/95/125/100/90/90/85/100 | M | 1.03/1.2/15/1.0 | 1.15 | 0 | evil |
| Githyanki | 100/100/100/100/130/120/100/75/100 | M | 0.97/0.95/15/0.9 | 0.8 | 25 | evil |
| Goblin | 100/135/115/100/85/110/105/90/110 | S | 0.95/1.05/12/0.7 | 0.85 | 0 | evil |
| Kobold | 90/110/120/95/95/125/105/100/110 | S | 0.775/0.95/15/0.6 | 0.9 | 0 | evil |
| Drider | 95/110/110/115/85/100/90/70/100 | L | 1.175/0.95/15/0.95 | 0.9 | 25 | evil |
| Thri-Kreen | 115/125/130/105/70/65/65/75/90 | M | 1.0/0.85/17.5/1.0 | 0.8 | 0 | neutral |
| Minotaur | 165/80/80/170/65/85/85/70/85 | H | 1.2/1.05/18.5/1.4 | 0.7 | 0 | neutral |
| Tiefling | 110/110/110/100/100/120/100/120/110 | M | 1.0/1.0/14/0.9 | 0.9 | 20 | neutral |
| Shade | 85/140/125/100/120/140/100/100/115 | S | 0.7/0.75/15/0.7 | none | 0 | descend: thief/illusionist |
| Revenant | 145/90/110/169/85/70/75/50/100 | L | 1.4/1.65/16.4/1.3 | none | 0 | descend: mercenary |
| Lich | 70/100/115/70/150/145/100/90/100 | M | 0.65/0.8/12/0.55 | 0.1 | 65 | descend: necromancer |
| Vampire | 120/125/115/100/110/120/100/120/100 | M | 1.28/1.65/13/0.8 | 0.79 | 40 | descend: sorcerer/dreadlord |
| Death Knight | 120/95/95/120/105/100/95/70/100 | L | 1.475/1.8/14/1.5 | none | 0 | descend: anti-paladin |
| Shadow Beast | 120/130/130/106/100/70/70/70/120 | M | 1.25/1.25/12/1.1 | none | 0 | descend: assassin |
| Wight | 135/80/80/155/100/60/60/50/100 | H | 1.35/2.2/15.5/1.9 | none | 0 | descend: warrior |
| Phantom | 90/140/130/97/125/100/100/100/100 | M | 0.9/0.9/14/0.7 | none | 20 | descend: conjurer |
| Half-Elf | 105/110/110/102/125/115/110/115/100 | M | 1.03/1.05/14/0.9 | 1.0 | 20 | legacy |
| Wood Elf | 125/115/115/135/95/85/91/85/100 | M | 1.15/1.2/15/0.95 | 1.0 | 5 | legacy |
| Kuo-Toa | 135/115/115/140/95/85/85/90/100 | M | 1.25/1.2/15/1.1 | 0.8 | 0 | legacy |
| Orog | 160/120/120/170/50/40/80/50/75 | M | 1.2/2.0/15/1.6 | 0.85 | 0 | legacy |
| Harpy | 80/115/130/109/100/120/110/100/80 | S | 1.05/1.0/12/0.7 | none | 0 | lore-restricted |
| Illithid | 70/90/90/85/200/150/110/25/100 | M | 0.45/0.6/15.5/0.025 | 0.1 | 50 | lore-restricted |
| Half-Illithid (Pillithid) | 83/105/105/82/125/120/100/100/100 | M | 0.65/1.0/15.5/0.75 | 0.1 | 50 | lore-restricted |
| Storm Giant | 150/70/70/150/80/80/80/65/100 | H | 1.455/2.0/14.5/1.6 | none | 0 | lore-restricted |

## Table 2: converted line

Ability is STR/CON/INT/WIS/DEX/CHA on our scale. M is the effective melee
factor and S the casting speed factor, both relative to human at 1.00.

| Race | STR/CON/INT/WIS/DEX/CHA | M | S |
|------|-------------------------|---|---|
| Human | 0/0/0/0/0/0 | 1.10 | 1.00 |
| Barbarian | +6/+7/-3/-1/-1/-3 | 1.18 | 0.77 |
| Grey Elf | -1/-1/+2/+2/+2/+2 | 0.77 | 1.21 |
| Mountain Dwarf | +4/+2/-2/+3/-1/-2 | 1.19 | 1.18 |
| Halfling | -1/-1/0/+2/+3/+1 | 1.02 | 1.43 |
| Gnome | -2/-1/+3/-1/+2/-1 | 0.94 | 1.67 |
| Centaur | +4/+6/-2/0/-1/-1 | 1.37 | 0.83 |
| Githzerai | 0/0/+2/+1/0/-1 | 0.93 | 1.11 |
| Firbolg | +13/+10/-3/-2/-3/-2 | 2.39 | 0.67 |
| Drow | -1/-1/+2/+2/+2/+1 | 0.75 | 1.21 |
| Duergar | +3/+4/-3/+3/-1/-3 | 1.18 | 1.18 |
| Ogre | +13/+10/-3/-2/-3/-5 | 2.13 | 0.62 |
| Troll | +6/+6/-3/-1/-1/-3 | 1.26 | 0.71 |
| Orc | +2/+3/-1/-1/0/-2 | 1.24 | 1.00 |
| Githyanki | 0/0/+2/0/0/-3 | 0.92 | 1.11 |
| Goblin | 0/0/+1/+1/+3/-1 | 1.25 | 1.43 |
| Kobold | -1/-1/+3/+1/+2/0 | 0.74 | 1.67 |
| Drider | -1/+2/0/-1/+1/-3 | 1.12 | 1.05 |
| Thri-Kreen | +2/+1/-4/-4/+3/-3 | 0.73 | 1.00 |
| Minotaur | +7/+7/-2/-2/-2/-3 | 1.02 | 0.71 |
| Tiefling | +1/0/+2/0/+1/+2 | 1.07 | 1.11 |
| Shade | -2/0/+4/0/+3/0 | 0.52 | 1.43 |
| Revenant | +5/+7/-3/-3/0/-5 | 2.11 | 0.77 |
| Lich | -3/-3/+5/0/+1/-1 | 0.65 | 1.82 |
| Vampire | +2/0/+2/0/+2/+2 | 2.44 | 1.25 |
| Death Knight | +2/+2/0/-1/-1/-3 | 2.84 | 0.67 |
| Shadow Beast | +2/+1/-3/-3/+3/-3 | 1.95 | 0.91 |
| Wight | +4/+6/-4/-4/-2/-5 | 2.87 | 0.53 |
| Phantom | -1/0/0/0/+4/0 | 0.87 | 1.43 |
| Half-Elf | +1/0/+2/+1/+1/+2 | 1.16 | 1.11 |
| Wood Elf | +3/+4/-2/-1/+2/-2 | 1.38 | 1.05 |
| Kuo-Toa | +4/+4/-2/-2/+2/-1 | 1.50 | 0.91 |
| Orog | +6/+7/-6/-2/+2/-5 | 2.40 | 0.62 |
| Harpy | -2/+1/+2/+1/+2/0 | 1.31 | 1.43 |
| Illithid | -3/-2/+5/+1/-1/-8 | 0.26 | 40.00 |
| Half-Illithid (Pillithid) | -2/-2/+2/0/+1/0 | 0.63 | 1.33 |
| Storm Giant | +5/+5/-2/-2/-3/-4 | 3.01 | 0.62 |

## Table 3: race point score and suggested tier

Ability is after the 4 point penalty cap. Traits is the innate and drawback
total from the appendix. The tier column applies our bands (Normal 5 to 9,
Advanced 12 to 16, Epic 20 to 28, Epic quest 40 to 60); a slash means the
score falls in the gap between two bands.

| Race | Ability | Size | Melee | Cast | Traits | RP | Tier by score |
|------|---------|------|-------|------|--------|----|---------------|
| Human | 0 | 0 | 0.75 | 0 | 3.5 | 4.2 | Normal |
| Barbarian | 9 | 1 | 1.5 | -0.5 | 7 | 18 | Advanced/Epic |
| Grey Elf | 6 | 0 | -1 | 1 | 9 | 15 | Advanced |
| Mountain Dwarf | 5 | 0 | 1.5 | 1 | 10 | 17.5 | Advanced/Epic |
| Halfling | 4 | 0 | 0 | 2 | 3 | 9 | Normal |
| Gnome | 1 | 0 | -0.5 | 3.5 | 2 | 6 | Normal |
| Centaur | 6 | 1 | 3 | -0.5 | 3 | 12.5 | Advanced |
| Githzerai | 2 | 0 | -0.5 | 0.5 | 7.5 | 9.5 | Normal/Advanced |
| Firbolg | 19 | 1 | 10.5 | -0.75 | 4 | 33.8 | Epic/Quest |
| Drow | 5 | 0 | -1 | 1 | 7.5 | 12.5 | Advanced |
| Duergar | 6 | 0 | 1.5 | 1 | 6 | 14.5 | Advanced |
| Ogre | 19 | 1 | 8.25 | -1 | 1 | 28.2 | Epic |
| Troll | 8 | 1 | 2.25 | -0.75 | 13 | 23.5 | Epic |
| Orc | 1 | 0 | 1.5 | 0 | 4.5 | 7 | Normal |
| Githyanki | -1 | 0 | -0.5 | 0.5 | 3 | 2 | Normal |
| Goblin | 4 | 0 | 1.5 | 2 | 2 | 9.5 | Normal/Advanced |
| Kobold | 4 | 0 | -1.5 | 3.5 | 2 | 8 | Normal |
| Drider | -1 | 1 | 0.75 | 0.5 | 3.5 | 4.8 | Normal |
| Thri-Kreen | 2 | 0 | -1.5 | 0 | 7 | 7.5 | Normal |
| Minotaur | 10 | 1 | 0 | -0.75 | 5 | 15.2 | Advanced |
| Tiefling | 6 | 0 | 0.75 | 0.5 | 4.5 | 11.8 | Advanced |
| Shade | 5 | 0 | -2.5 | 2 | 0.5 | 5 | Normal |
| Revenant | 8 | 1 | 8.25 | -0.5 | 6.5 | 23.2 | Epic |
| Lich | 2 | 0 | -2 | 4 | 11 | 15 | Advanced |
| Vampire | 8 | 0 | 10.5 | 1.5 | 10 | 30 | Epic/Quest |
| Death Knight | 0 | 1 | 13.5 | -0.75 | 3 | 16.8 | Advanced/Epic |
| Shadow Beast | 2 | 0 | 7.5 | -0.25 | 2 | 11.2 | Normal/Advanced |
| Wight | 6 | 1 | 14.25 | -1.25 | 5.5 | 25.5 | Epic |
| Phantom | 3 | 0 | -0.5 | 2 | 10 | 14.5 | Advanced |
| Half-Elf | 7 | 0 | 1.5 | 0.5 | 7 | 16 | Advanced |
| Wood Elf | 5 | 0 | 3 | 0.5 | 5 | 13.5 | Advanced |
| Kuo-Toa | 6 | 0 | 3.75 | -0.25 | 0.5 | 10 | Normal/Advanced |
| Orog | 11 | 0 | 10.5 | -1 | 2.5 | 23 | Epic |
| Harpy | 4 | 0 | 2.25 | 2 | 5 | 13.2 | Advanced |
| Illithid | 2 | 0 | -3.5 | 5 | 9 | 12.5 | Advanced |
| Half-Illithid (Pillithid) | -1 | 0 | -2 | 1.5 | 5 | 3.5 | Normal |
| Storm Giant | 6 | 1 | 15 | -1 | 4.5 | 25.5 | Epic |

## Reading the results

- **The roster races cluster where Duris says they should.** Every good and
  evil creation race with an experience factor of 0.9 or higher scores Normal
  or low Advanced (Human, Gnome, Halfling, Orc, Kobold, Goblin, Githzerai,
  Centaur).
  Races Duris taxes at 0.8 to 0.85 score Advanced (Grey Elf, Drow, Duergar,
  Barbarian, Wood Elf, Tiefling). The two levers agree often enough that the
  Duris experience factor is a fair cross-check on our RP table.
- **Duris giants do not fit our stat scale.** Firbolg and Ogre convert to STR
  +13 and CON +10, three points past our highest existing modifier (Fae DEX
  +10). Their melee factor alone is worth 8 to 10 RP. Both score Epic on raw
  conversion despite being ordinary evil or good roster races in Duris, where
  their 1.5 to 1.6 spellcast pulse and two-class list are the real cost. Both,
  with Minotaur and Storm Giant, also wield two-handed weapons in one hand at
  full damage, so a shield or second weapon stacks on top of the multiplier.
  Adopt them only with compressed stats (see below).
- **Descend forms score Advanced to Epic, not Epic quest.** Lich 15,
  Revenant 23, Wight 25.5, Vampire 30. Our own Lich and Vampire score 44.5
  and 59.5. Duris undead
  are balanced against a level reset and outcast status, which our system does
  not model. If adopted as quest races they would need roughly double their
  Duris kit.
- **Melee multipliers dominate the undead warriors.** Death Knight, Wight,
  Storm Giant, and Orog draw 10 to 15 RP from total output alone. We have no
  equivalent of a 2.0x damage multiplier; the honest conversion is a large
  flat or scaling damage trait, which our composition rule 2 (no single trait
  over 30 percent of budget) would reject at Advanced.
- **Illithid's power is in a stat we do not score.** Its Pow 200 is
  excluded, so it lands at Advanced (12.5) on INT +5, shrug 50, and the
  capped casting speed. Duris prices it with a 0.1 experience factor, the
  same as Lich, which says the raw spell power is most of the race.
- **Thri-Kreen depends entirely on four-weapon wielding.** Stats, innates,
  and lost slots net to -0.5 RP before the arms; the four-weapon trait is
  priced at 8 and is the whole race. In our attack routine that trait would
  need its own design.
- **Half-Illithid scores below our Half-Illithid.** Duris's version is a
  frail caster with sun and daylight penalties (3.5 RP); ours is an epic race
  at 20.5. They share a name and nothing else.

## Adaptation notes

Races grouped by what they would take to adopt. Scores in parentheses are the
raw conversion; "compressed" means the stat line reduced to our tier ceiling
while keeping the Duris ordering of stats.

**Already covered by an existing race.** Human, Grey Elf (Moon or High Elf),
Mountain Dwarf, Halfling, Gnome, Half-Elf, Wood Elf (Wild Elf), Tiefling,
Drow, Duergar, Goblin, Shade, Lich, Vampire. The Duris versions differ in
flavour more than kit. Two ideas worth lifting: the dwarven Axe and Hammer
Master scaling weapon bonuses (a level-scaling trait our Mountain Dwarf lacks),
and Duergar Battle Rage as a per-day haste.

**Near matches with a different frame.** Troll versus our HalfTroll (23.5 vs
8.5): Duris Troll's 10 per tick regeneration and Troll Skin are what our
HalfTroll is missing to reach the Advanced band. Ogre versus our Half-Ogre
(28.2 vs 9.5): compress to STR +6, CON +4, Large, Ogre Roar, bonus damage vs
smaller, giant wield, and it lands at about 17. Thri-Kreen versus Trelux: Trelux already
has exoskeleton, leap, and pincers; the four-arm mechanic is the only new
idea. Centaur versus Wemic: Wemic already covers the quadruped body; Duris
adds Stampede and Doorkick.

**New races that convert cleanly to Normal or Advanced.**

| Race | Raw RP | Target tier | Adjustments to fit |
|------|--------|-------------|--------------------|
| Orc | 7 | Normal | None. STR +2, CON +3, minor penalties, Ultravision, Seadog, two summons. Summons would be 1/day SLAs |
| Kobold | 8 | Normal | None. Small, DEX +2, INT +3, Ultravision, stealth, fast casting |
| Githyanki | 2 | Normal, or Advanced with SR | Shrug 25 and unscored Pow 130 are the race. Drop sun vulnerability (it is a racewar device) and give it a psionic SLA and it is a clean Advanced race at about 8 to 10 |
| Githzerai | 9.5 | Normal or Advanced | Add one scaling trait (Rrakkma grows with grouped Githzerai; alone it does nothing) |
| Barbarian | 18 | Advanced | STR +6, CON +7 exceed rule 1 (ability over 50 percent of budget). Compress to +4/+4 and keep Bodyslam, Groundfighting, Dauntless, cold resistance |
| Drider | 4.8 | Advanced | Under-built in Duris (four innates, three of them shared). Needs its stats raised toward Drow's and Webwrap implemented to justify the Large body |
| Minotaur | 15.2 | Advanced | STR +7, CON +7 compress to +4/+4; Charge and giant wield are the identity. The bloodlust rage below half hp is a genuine drawback we could implement |
| Kuo-Toa | 10 | Normal or Advanced | Sun vulnerability and Throw Lightning; otherwise a sturdy aquatic fighter |
| Harpy | 13.2 | Advanced | Flight is 4 of 13 points. Comparable to a flying Tabaxi |

**New races that convert to Epic only after compression.**

| Race | Raw RP | Issue | Compressed line |
|------|--------|-------|-----------------|
| Firbolg | 33.8 | STR +13, CON +10, melee x2.4 | STR +8, CON +6, INT -2, CHA -2, Large, giant wield, Bodyslam, Doorbash, Forest Sight, Magic Vulnerability. About 23 |
| Ogre | 28.2 | Same as Firbolg, plus three lost slots | See near matches above; best folded into Half-Ogre |
| Storm Giant | 25.5 | No innates beyond giant wield; entire score is stats and melee x2.7 | Needs Throw Lightning and Doorbash restored (both are commented out in Duris) before it is a race rather than a stat block |
| Orog | 23 | Melee x2.0 and 40 spell save | STR +6, CON +7 compress to +4/+5; Warcaller's Fury as a party-scaling damage trait is the interesting piece |
| Illithid | 12.5 | Pow 200 unscored, instant casting, CHA 25 | As an Epic caster: INT +5, WIS +1, CHA -4 (capped), SR 15 + level, mind blast 3/day, levitate, planar shift. About 24 |

**Undead descend forms.** Death Knight (16.8), Shadow Beast (11.2), Phantom
(14.5), Revenant (23.2), Wight (25.5) are all built around one or two large
multipliers plus fire or sun vulnerability. The kits are thin by our
standards. If we want more transformation races beyond Lich and Vampire,
Death Knight (fire shield and firestorm on a fighter chassis) and Phantom
(flight, phasing, planar travel on a caster chassis) are the two with a
distinct identity; both would need their budgets roughly doubled to reach the
Epic quest band.

## Caveats

- Conversion factors are heuristics. Ten percent of a Duris stat is not
  exactly one d20 modifier point; the two systems apply stats through
  different tables (`str_app[]` in Duris, ability modifiers here). The
  mapping preserves ordering and proportion, which is what the comparison
  needs.
- Duris melee and casting multipliers have no clean counterpart in our
  engine. Their RP prices are the largest source of uncertainty in Table 3.
- Several Duris help entries describe innates that the source does not
  register (Dayblind on most races, Summon Totem, Project Image, Calming,
  Miner, Burrow, Gambler's Luck, Webwrap). Only registered innates are scored.
- Duris level range is 1 to 56 for mortals. Level-gated innates listed as
  "L21" or "L31" would need their gates rescaled to our 30-level range.
- Racewar side, hometown, multiclassing, and the descend level reset are not
  converted. They are Duris-wide systems, not race features.

## Appendix: per-race trait pricing

Each innate and drawback as priced for Table 3. Prices follow the trait table
in the player races reference; a level in parentheses is the Duris level at
which the innate becomes available.

### Human

| Trait | RP |
|-------|----|
| Seadog (ship handling) | 0.5 |
| +5 to every stat (stats.bonus) | 3 |
| Multiclass at 50 (not convertible) | 0 |

### Barbarian

| Trait | RP |
|-------|----|
| Bodyslam | 1 |
| Doorbash | 0.5 |
| Groundfighting (L21) | 1 |
| Protection From Cold (L31) | 1 |
| Dauntless: fear immunity (L41) | 1 |
| Cold damage -40 percent | 1 |
| Saves: spell +20, breath/para/petri +10 | 1.5 |

### Grey Elf

| Trait | RP |
|-------|----|
| Infravision | 0.5 |
| Magic resistance: shrug 35 | 4 |
| Longsword Master (L11): +lvl/8 hit, +lvl/12 dam | 3 |
| Forest Sight (L11) | 0.5 |
| Outdoor Sneak (L21) | 1 |

### Mountain Dwarf

| Trait | RP |
|-------|----|
| Infravision | 0.5 |
| Innate Strength (self buff) | 1 |
| Giant Avoidance: +10 percent dodge vs giants | 0.5 |
| Magical Reduction: -20 percent generic spell damage | 1 |
| Axe Master (L11) | 3 |
| Hammer Master (L31) | 3 |
| Hatred (L21): rage vs evil | 1 |

### Halfling

| Trait | RP |
|-------|----|
| Innate Hide | 1 |
| Quick Thinking: 15 percent auto-save vs INT/POW | 1.5 |
| Perception (L11): wider map view, track bonus | 0.5 |

### Gnome

| Trait | RP |
|-------|----|
| Infravision | 0.5 |
| Ultravision | 1 |
| Farsee (L21) | 0.5 |

### Centaur

| Trait | RP |
|-------|----|
| Horse Body: bash immunity vs smaller, mountable | 1.5 |
| Doorkick | 0.5 |
| Stampede (L21) | 1 |
| Two-Handed Sword Mastery (L31) | 1 |
| Saves: petri +25 | 1 |
| No leg or foot slots | -2 |

### Githzerai

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Dayvision | 0 |
| Magic resistance: shrug 20 | 2 |
| Levitate (L11) | 1 |
| Shift Prime / Shift Astral (L45) | 2 |
| Spirit of the Rrakkma (L21): +5 shrug, -10 AC per grouped Githzerai | 1.5 |

### Firbolg

| Trait | RP |
|-------|----|
| Giant wield: two-handed weapons in one hand at full damage | 3 |
| Bodyslam | 1 |
| Doorbash | 0.5 |
| Forest Sight | 0.5 |
| Magic Vulnerability: +10 percent spell damage | -1 |

### Drow

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Magic resistance: shrug 35 | 4 |
| Faerie Fire | 0.5 |
| Levitate (L11) | 1 |
| Globe Of Darkness (L26) | 1 |
| Longsword Master (L11) | 3 |
| Sun vulnerability: no regen, 5-20 damage per tick in sun | -3 |

### Duergar

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Magical Reduction | 1 |
| Innate Strength (L6) | 1 |
| Underdark Invisibility | 1 |
| Underdark Sneak (L31) | 1 |
| Enlarge (L26) | 1 |
| Battle Frenzy (L16): 5 percent extra attack vs humanoids | 1 |
| Battle Rage (L31): haste on a timer | 2 |
| Sun vulnerability | -3 |

### Ogre

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Giant wield: two-handed weapons in one hand at full damage | 3 |
| Bodyslam | 1 |
| Doorbash | 0.5 |
| Ogre Roar (L11): fear debuff | 1.5 |
| Bonus damage vs smaller foes | 1 |
| Magic Vulnerability | -1 |
| Sun vulnerability | -3 |
| Saves: para +15, breath -10, petri -10 | 0 |
| No arm, leg, or full-body slots | -3 |

### Troll

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Regeneration: 10 per tick standing, 20 resting, 30 sleeping | 10 |
| Troll Skin (L21): melee damage x0.85 | 3 |
| Bodyslam | 1 |
| Doorbash | 0.5 |
| Groundfighting (L21) | 1 |
| Swamp Sneak (L11) | 0.5 |
| Saves: para +15, breath/petri +10 | 1 |
| Fire vulnerability x1.3 | -2 |
| Sun vulnerability | -3 |

### Orc

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Dayvision | 0 |
| Seadog | 0.5 |
| Summon Horde (L11) | 1.5 |
| Summon Warg (L16) | 1.5 |
| Multiclass at 50 (not convertible) | 0 |

### Githyanki

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Magic resistance: shrug 25 | 2 |
| Levitate (L11) | 1 |
| Shift Prime / Shift Astral | 2 |
| Sun vulnerability | -3 |

### Goblin

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Disappear (L31): hide while others fight | 1 |
| Summon Totem (L26): no implementation found | 0 |

### Kobold

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Dayvision | 0 |
| Underdark Sneak | 1 |

### Drider

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Magic resistance: shrug 25 | 2 |
| Spider Body: bash immunity vs smaller | 1.5 |
| Groundfighting (L21) | 1 |
| No leg or foot slots | -2 |

### Thri-Kreen

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Dayvision | 0 |
| Four arms: wield four weapons, double wrist/sleeve/glove slots | 8 |
| Bite (L11): paralysing venom | 2 |
| Leap (L21) | 1 |
| Cold damage -30 percent | 1 |
| Cold vulnerability x1.35 plus paralysis save | -2 |
| No body, foot, finger, or ear slots | -4 |

### Minotaur

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Giant wield: two-handed weapons in one hand at full damage | 3 |
| Dayvision | 0 |
| Doorbash | 0.5 |
| Charge (L11): damage and stun up to one room away | 2 |
| Saves: spell/para/petri +15, breath +10 | 1.5 |
| Bloodlust rage below 50 percent hp: uncontrolled, no casting | -2 |
| No head slot | -1 |

### Tiefling

| Trait | RP |
|-------|----|
| Infravision | 0.5 |
| Ultravision | 1 |
| Magic resistance: shrug 20 | 2 |
| Protection From Fire | 1 |

### Shade

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Darkness | 0.5 |
| Shade Movement (L21): hide and sneak in darkness | 1 |
| Fire vulnerability x1.3 | -2 |

### Revenant

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Regeneration: 5 per tick standing, 10 resting, 15 sleeping | 5 |
| Bodyslam | 1 |
| Doorbash | 0.5 |
| Shadow Door (L25) | 1 |
| Fire vulnerability x1.3 | -2 |

### Lich

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Magic resistance: shrug 65 | 6 |
| Spell Absorb (L21) | 2 |
| Eyeless: blind immunity | 1 |
| Protection From Cold | 1 |
| Undead Fealty: lower undead do not aggro | 1 |
| Call Of The Grave: summon skeletons | 2 |
| Sun vulnerability | -3 |

### Vampire

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Magic resistance: shrug 40 | 4 |
| Vampiric Touch: permanent life drain on hits | 3 |
| Bite: drain | 2 |
| Sacrilegious Power (L46): holy damage x0.75, x0.5 at 51, x0.25 at 56 | 3 |
| Sun vulnerability | -3 |

### Death Knight

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Protection From Fire | 1 |
| Firestorm (L26) | 2 |
| Fireshield (L31) | 2 |
| Sun vulnerability | -3 |

### Shadow Beast

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Innate Strength | 1 |
| Underdark Sneak | 1 |
| Enlarge | 1 |
| Project Image: no implementation found | 0 |
| Fire vulnerability x1.3 | -2 |

### Wight

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Bodyslam | 1 |
| Doorbash | 0.5 |
| Stone Skin (L25, 300s timer) | 2 |
| Saves: spell +30, breath/para/petri +25 | 3 |
| Fire vulnerability x1.3 | -2 |

### Phantom

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Magic resistance: shrug 20 | 2 |
| Shift Prime / Shift Astral | 2 |
| Fly (L21) | 4 |
| Phantasmal Form (L20): invisible, passdoor | 3 |
| Fire vulnerability x1.3 | -2 |

### Half-Elf

| Trait | RP |
|-------|----|
| Infravision | 0.5 |
| Magic resistance: shrug 20 | 2 |
| Longsword Master (L11) | 3 |
| Quick Thinking (L30) | 1.5 |

### Wood Elf

| Trait | RP |
|-------|----|
| Magic resistance: shrug 5 | 0.5 |
| Longsword Master (L11) | 3 |
| Forest Sight (L11) | 0.5 |
| Outdoor Sneak (L11) | 1 |

### Kuo-Toa

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Waterbreath (L15) | 0.5 |
| Throw Lightning (L30) | 1.5 |
| Swamp Sneak | 0.5 |
| Sun vulnerability | -3 |

### Orog

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Warcaller's Fury (L21): +2 to 15 percent damage by group size | 2 |
| Saves: spell +40, petri +25, breath/para +10 | 2.5 |
| Sun vulnerability | -3 |

### Harpy

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Dayvision | 0 |
| Fly | 4 |
| Perception | 0.5 |
| Must rest to tupor | -0.5 |

### Illithid

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Magic resistance: shrug 50 | 6 |
| Levitate | 1 |
| Blast: paralysing psionic attack | 2 |
| Shift Prime / Shift Astral | 2 |
| Sun vulnerability | -3 |

### Half-Illithid (Pillithid)

| Trait | RP |
|-------|----|
| Ultravision | 1 |
| Magic resistance: shrug 50 | 6 |
| Levitate (L10) | 1 |
| Shift Prime / Shift Astral | 2 |
| Dayblind (the only race with it wired in code) | -2 |
| Sun vulnerability | -3 |

### Storm Giant

| Trait | RP |
|-------|----|
| No innates registered | 0 |
| Giant wield: two-handed weapons in one hand at full damage | 3 |
| Saves: para +15, spell/breath/petri +10 | 1.5 |
