# Duris Race Feat Proposal

Status: proposal only, written 2026-09-23, casting ranks and Thri-Kreen tier revised 2026-09-24;
nothing here is implemented.

This proposal places the [imported racial innate feats](DURIS_RACIAL_IMPORTS.md#what-we-imported)
on the [Duris races not imported](DURIS_RACIAL_IMPORTS.md#duris-races-not-imported), so each race
would be `add_race()` data plus `feat_race_assignment()` lines, following
[ADDING_NEW_RACE_GUIDE.md](../guides/ADDING_NEW_RACE_GUIDE.md). Names are the in-game feat names
from the [DURIS_RACIAL_IMPORTS.md](DURIS_RACIAL_IMPORTS.md) tables, without the `innate` prefix on
the spell-like ones.

Of the 54 new feats (1268-1321), 29 already have a home here: Duris registers them on these races,
some only in commented-out lines. Of the other 25, 24 come only from Duris races LuminariMUD
already covers and `FEAT_EXTRA_ARMS` has no Duris source. Those, plus fearlessness and innate haste
([wired](DURIS_RACIAL_IMPORTS.md#existing-feats-wired-or-repurposed) but held by no race), are
placed by lore fit; two feats stay unassigned (see the [notes](#notes)). The tier column names the
[race point band](../guides/PLAYER_RACES_REFERENCE.md#tier-budgets) each race would be priced
against. Each race's stats, size, alignment, feat levels, race point score, and the feats it needs
to reach its band are in [DURIS_RACE_SPECIFICATIONS.md](DURIS_RACE_SPECIFICATIONS.md).

## Feats by race

| Duris race | Tier | Own Duris feats | From covered races (Duris owner) |
| -- | -- | -- | -- |
| Centaur | Advanced | quadruped body, leonine frame, doorbash, stampede, greatsword mastery | - |
| Githzerai | Advanced | plane shift, rrakkma | quick thinking (Halfling, Half-Elf) |
| Firbolg | Advanced | bodyslam, doorbash, forest sight, magic vulnerability, slow casting (1 rank) | outdoor stealth (Grey Elf, Wood Elf), hatred and hammer mastery (Mountain Dwarf) |
| Githyanki | Advanced | plane shift, enhanced spell damage; drop its sun vulnerability, a Duris racewar device rather than gith lore | longsword mastery (elves), psionic blast (Illithid) |
| Kobold | Normal | underdark stealth, miner and calming (both commented out), fast casting (1 rank) | barter (Halfling, commented out) |
| Drider | Advanced | quadruped body, leonine frame, groundfighting, web (commented out) | fireball and mass dispel (Drow Elf, its parent race, commented out) |
| Thri-Kreen | Epic | four arms, leap, vulnerable to cold | - |
| Minotaur | Advanced | doorbash, bull charge, bloodlust | axe mastery (Mountain Dwarf), scare (Ogre roar), fearlessness (Barbarian) |
| Death Knight | Epic quest | fire shield, fire storm, sun vulnerability, slow casting (5 ranks) | sacrilegious power (Vampire), undead fealty (Lich) |
| Wight | Epic quest | bodyslam, doorbash, stoneskin, weakness to fire, slow casting (9 ranks) | frost breath (Barbarian, commented out) |
| Revenant | Epic quest | bodyslam, doorbash, shadow jump, weakness to fire, slow casting (3 ranks) | battle frenzy (Duergar Dwarf) |
| Shadow Beast | Epic quest | underdark stealth, weakness to fire, slow casting (1 rank) | racial flurry (Halfling and Goblin, commented out) |
| Phantom | Epic quest | plane shift, weakness to fire, fast casting (3 ranks), enhanced spell damage | spell absorb and eyeless (Lich) |
| Kuo Toa | Normal | lightning bolt, water breathing, swamp stealth, sun vulnerability, slow casting (1 rank) | seadog (Human, Orc) |
| Orog | Advanced | warcaller's fury, sun vulnerability, slow casting (6 ranks) | summon horde and summon warg (Orc), magical reduction (Mountain and Duergar Dwarf) |
| Harpy | Advanced | fast casting (3 ranks) | farsee (Gnome), innate haste (Duergar Dwarf) |
| Stormkin (Duris Storm Giant) | Advanced | doorbash and lightning bolt (both commented out), slow casting (6 ranks) | thick hide (Troll) |

## Notes

- Unassigned. `FEAT_EXTRA_ARMS`: four arms covers Thri-Kreen, and the
  [race point reference](../guides/PLAYER_RACES_REFERENCE.md#race-point-rp-pricing-table) says not
  to grant both. `FEAT_DAYBLIND`: Duris commented it out on all six of these races that list it
  (Drider, Githyanki, Kuo Toa, Orog, Wight, Phantom), and it would stack a -4 drawback on races
  that mostly carry sun vulnerability already.
- Casting speed, one rank per 10 percent of the Duris multiplier
  (`spellcast.pulse.racial.<Race>`), rounded to the nearest rank; each rank is 10 percent of a
  spell's casting time
  ([GAME_MECHANICS_SYSTEMS.md](../systems/GAME_MECHANICS_SYSTEMS.md#racial-innate-feats-and-spell-like-abilities)).
  Slow casting:
  Wight 1.9 (9 ranks), Orog and Stormkin 1.6 (6), Death Knight 1.5 (5), Revenant 1.3 (3),
  Shadow Beast and Kuo Toa 1.1 (1), Firbolg 1.055 (1). Fast casting: Phantom and Harpy 0.7 (3),
  Kobold 0.895 (1). Githzerai and Githyanki (0.975), Drider (0.985), Thri-Kreen (1.0), Centaur
  (1.02), and Minotaur (1.04) get none. A fast cast that reaches zero ticks is not a quickened
  cast: in standard-action mode it still costs a standard and a move action, so fast casting does
  not raise how often a character casts there.
- Duris re-based the creation races' casting and combat pulses on 2026-09-14 (Duris `98a8ee469`).
  The eight creation races here now cast at 0.895 (Kobold) to 1.055 (Firbolg), which gives Kobold
  one fast rank, Firbolg one slow rank, and the other six none. The retired
  [race conversion study](https://github.com/LuminariMUD/Luminari-Source/blob/dba4ca2de4afdbe47fc0d1f6831a1a1f75db1e7e/docs/ongoing-projects/DURIS_RACE_CONVERSION.md)
  predates that commit, so its combat pulse and spellcast values, and the melee and casting scores
  built on them, are stale for those races.
- Spell resistance and spell power. Duris shrug 20 to 25 (`innate.shrug.<Race>`: Githzerai and
  Phantom 20, Githyanki and Drider 25) maps to `FEAT_HALF_DROW_SPELL_RESISTANCE`. One rank of
  `FEAT_ENHANCED_SPELL_DAMAGE` goes to the races with Duris Pow of 125 or more
  (`stats.pow.<Race>`: Githyanki 130, Phantom 125); Githzerai at 120 does not get it.
- Existing feats from
  [Duris innates already covered by existing feats](DURIS_RACIAL_IMPORTS.md#duris-innates-already-covered-by-existing-feats)
  come along: `FEAT_ULTRAVISION` on every race except Centaur and Firbolg (no vision innate) and
  Stormkin (`FEAT_INFRAVISION`, commented out in Duris); `FEAT_HALF_DROW_SPELL_RESISTANCE` as
  above; `FEAT_SLA_LEVITATE` on both gith; `FEAT_WINGS` on Harpy and Phantom; `FEAT_KEEN_SENSES`
  on Harpy; `FEAT_POISON_BITE` on Thri-Kreen; `FEAT_TROLL_REGENERATION` on Revenant;
  `FEAT_VAMPIRE_GASEOUS_FORM` on Phantom; `FEAT_TIEFLING_HELLISH_RESISTANCE` on Death Knight;
  `FEAT_SLA_STRENGTH` and `FEAT_SLA_ENLARGE` on Shadow Beast.
- Leonine frame's leg and foot refusal message is generic, but its name and help text say
  "leonine"; give it a neutral name before Centaur and Drider take it.
- The five descend forms stay far below the epic quest band even with these feats. Duris made
  them strong with melee multipliers, which #164 declined (see
  [Not imported](DURIS_RACIAL_IMPORTS.md#not-imported)), so they need native feats (armor skin
  stacks, damage reduction, hardy) the way Lich and Vampire have them; the
  [descend chassis](DURIS_RACE_SPECIFICATIONS.md#descend-forms) proposes them.
- Thri-Kreen is an Epic race bought with account experience, not an Epic quest race. The
  specification [answers](DURIS_RACE_SPECIFICATIONS.md#thri-kreen) the open four-arms decisions in
  the [race point pricing notes](../guides/PLAYER_RACES_REFERENCE.md#race-point-rp-pricing-table):
  price, psionic damage reduction, venom, riding, and ability adjustments.
