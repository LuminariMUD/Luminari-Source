# Duris Race Specifications

Status: proposal only, written 2026-09-24; nothing here is implemented.

This completes the [race specification](../guides/ADDING_NEW_RACE_GUIDE.md#1-write-the-race-specification-first)
for the 17 [Duris races not imported](DURIS_RACIAL_IMPORTS.md#duris-races-not-imported). Their
tiers and feats are in [DURIS_RACE_FEAT_PROPOSAL.md](DURIS_RACE_FEAT_PROPOSAL.md). This document adds
everything else a race needs: ID, names, size, family, alignment, language, ability modifiers,
attack types, lost slots, feat levels, the race point (RP) score against the
[tier bands](../guides/PLAYER_RACES_REFERENCE.md#tier-budgets), the feats each race needs to
reach its band, how the descend forms are acquired, and answers to the open Thri-Kreen decisions.

Duris values were read at `e1357a30a` (`/home/aiwithapex/projects/duris`): stat factors from
`lib/duris.properties`, sizes from `set_char_size()` in `src/account/nanny.c`, alignments from
`class_table[]` and `restricted_class_rows[]` in `src/core/constant.c`, and innate levels from
`src/classes/innates.c`. The stat factors match the retired
[race conversion study](https://github.com/LuminariMUD/Luminari-Source/blob/dba4ca2de4afdbe47fc0d1f6831a1a1f75db1e7e/docs/ongoing-projects/DURIS_RACE_CONVERSION.md),
so its raw conversions stand.

## Rules used

- Ability modifiers. The raw line is the study's rule: every 10 percent of a Duris stat factor
  above or below 100 is one point, and DEX is the average of Duris Dex and Agi. The proposed line
  then fits the tier. Except for the descend forms (see the table notes), bonuses go only on
  stats Duris raises, keep their Duris order, and stay within composition rule 1 (ability points at most half the budget). The modifier cap is +10, the
  existing maximum (Fae). Penalties go only on stats Duris lowers. Drawbacks are credited first,
  and penalties take whatever room composition rule 3 leaves. A penalty with no room earns no
  credit.
- Size. Duris Huge becomes Large, because a Huge player size was declined in #163.
- Alignment. Each race allows every Duris alignment that any of its classes allows: "any" stays
  any, "evil or neutral" becomes not good (LN, TN, CN, LE, NE, CE), and "evil" becomes evil only.
  Kuo Toa, Orog, Harpy, and Storm Giant have `class_table[]` rows that allow no class, so their
  Duris `restricted_class_rows[]` stand-ins are used instead. The one exception is Stormkin. The
  Duris Storm Giant stand-in copies Ogre's evil-only row, but Duris gives the race no racewar
  side, so Stormkin allows any alignment.
- Feat levels. A gated Duris innate is granted at half its Duris level, rounded up (Duris
  mortals reach level 56). A feat borrowed from a covered race takes its earliest Duris owner's
  level. Every other feat is granted at level 1.
- Language. Each race gets the nearest `SKILL_LANG_*`. The gith use Common because there is no
  Gith language. Descend forms get none, like Lich and Vampire.

## Registry data

IDs follow [guide section 2](../guides/ADDING_NEW_RACE_GUIDE.md#2-resolve-the-numeric-id-before-implementation):
the creation races append after Yuan-Ti (151), which raises `NUM_EXTENDED_RACES` to 164 and
`NUM_CREATION_RACES` to 45. The descend forms take the five IDs 55-59 reserved for quest-only
races. The retired `LEGACY_RACE_MINOTAUR` (37) is not reused. Every race allows male and female.

Stormkin is a custom race, roughly half storm giant, that replaces the Duris Storm Giant and is
Large. Its stats and feats are the Duris Storm Giant's, converted like every other race here.

| Race | ID | Token, abbrev | Tier (unlock cost, LA) | Size | Family | Alignment | Language |
| -- | -- | -- | -- | -- | -- | -- | -- |
| Centaur | 152 | centaur, Cent | Advanced (1000, +2) | Large | Monstrous humanoid | any | Elven |
| Githzerai | 153 | githzerai, Gthz | Advanced (1000, +2) | Medium | Humanoid | any | Common |
| Firbolg | 154 | firbolg, Fbol | Advanced (1000, +2) | Large | Giant | any | Giant |
| Githyanki | 155 | githyanki, Gthy | Advanced (1000, +2) | Medium | Humanoid | not good | Common |
| Kobold | 156 | kobold, Kobo | Normal (0, +0) | Small | Humanoid | not good | Kobold |
| Drider | 157 | drider, Drdr | Advanced (1000, +2) | Large | Aberration | evil only | Undercommon |
| Thri-Kreen | 158 | thrikreen, TKrn | Epic (50000, +10) | Medium | Monstrous humanoid | any | Common |
| Minotaur | 159 | minotaur, Mino | Advanced (1000, +2) | Large | Monstrous humanoid | any | Giant |
| Kuo Toa | 160 | kuotoa, KToa | Normal (0, +0) | Medium | Monstrous humanoid | evil only | Undercommon |
| Orog | 161 | orog, Orog | Advanced (1000, +2) | Medium | Humanoid | not good | Orcish |
| Harpy | 162 | harpy, Hrpy | Advanced (1000, +2) | Small | Monstrous humanoid | any | Common |
| Stormkin | 163 | stormkin, Stmk | Advanced (1000, +2) | Large | Giant | any | Giant |
| Death Knight | 55 | deathknight, DKni | Epic quest (locked, +10) | Large | Undead | evil only | - |
| Wight | 56 | wight, Wght | Epic quest (locked, +10) | Large | Undead | evil only | - |
| Revenant | 57 | revenant, Rvnt | Epic quest (locked, +10) | Large | Undead | evil only | - |
| Shadow Beast | 58 | shadowbeast, SBst | Epic quest (locked, +10) | Medium | Undead | evil only | - |
| Phantom | 59 | phantom, Phnt | Epic quest (locked, +10) | Medium | Undead | evil only | - |

## Ability modifiers

Order is STR/CON/INT/WIS/DEX/CHA. Ability points are the bonuses minus the credited penalties.

| Race | Duris raw | Proposed | Ability points |
| -- | -- | -- | -- |
| Centaur | +4/+6/-2/0/-1/-1 | +3/+5/-1/0/0/0 | 7 |
| Githzerai | 0/0/+2/+1/0/-1 | 0/0/+4/+3/0/-1 | 6 |
| Firbolg | +13/+10/-3/-2/-3/-2 | +5/+4/-1/0/-1/0 | 7 |
| Githyanki | 0/0/+2/0/0/-3 | 0/0/+4/0/0/-1 | 3 |
| Kobold | -1/-1/+3/+1/+2/0 | -1/0/+2/0/+2/0 | 3 |
| Drider | -1/+2/0/-1/+1/-3 | 0/+5/0/0/+3/-1 | 7 |
| Thri-Kreen | +2/+1/-4/-4/+3/-3 | 0/0/-2/-2/+4/-2 | 4 (penalties uncredited) |
| Minotaur | +7/+7/-2/-2/-2/-3 | +3/+4/0/0/0/0 | 7 |
| Kuo Toa | +4/+4/-2/-2/+2/-1 | +1/+2/0/0/0/0 | 3 |
| Orog | +6/+7/-6/-2/+2/-5 | +3/+4/-2/0/0/0 | 7 (INT uncredited) |
| Harpy | -2/+1/+2/+1/+2/0 | -2/+1/+2/+1/+2/0 | 4 |
| Stormkin | +5/+5/-2/-2/-3/-4 | +5/+4/0/0/-2/0 | 7 |
| Death Knight | +2/+2/0/-1/-1/-3 | +8/+6/-2/+6/-2/0 | 16 |
| Wight | +4/+6/-4/-4/-2/-5 | +10/+10/-2/0/0/-2 | 16 |
| Revenant | +5/+7/-3/-3/0/-5 | +7/+7/-2/0/+6/-2 | 16 |
| Shadow Beast | +2/+1/-3/-3/+3/-3 | 0/+7/+3/-2/+10/-2 | 16 |
| Phantom | -1/0/0/0/+4/0 | -2/+5/+10/0/+3/0 | 16 |

The descend forms follow a set design emphasis instead of their Duris direction: Death Knight
STR, CON, and WIS; Wight STR and CON; Revenant STR, CON, and DEX evenly; Shadow Beast DEX, CON,
and some INT; Phantom INT, moderate CON, and some DEX. Each keeps 16 ability points, so its race
point score is unchanged. Harpy keeps its raw line unchanged.

## Body and feat levels

Attack types are the `set_race_attack_types()` flags used for wild shape and disguise. Lost slots
are `set_race_wear_restriction()` rows; Centaur and Drider lose legs and feet through the
leonine frame feat instead. Feats not listed here are granted at level 1.

| Race | Attack types | Lost slots | Feats granted after level 1 |
| -- | -- | -- | -- |
| Centaur | hit, punch, trample, charge | - | stampede 11, greatsword mastery 16 |
| Githzerai | hit, punch | - | levitate 6, rrakkma 11 |
| Firbolg | hit, punch, smash | - | outdoor stealth 6, hatred 11, hammer mastery 16 |
| Githyanki | hit, slash, punch | - | levitate 6, longsword mastery 6 |
| Kobold | hit, bite, claw | - | miner 26 |
| Drider | bite, pierce, claw | - | groundfighting 11, fireball 11, mass dispel 27 |
| Thri-Kreen | bite, claw, slash | body, feet, both fingers, both ears | poison bite 6, leap 11 |
| Minotaur | hit, gore, charge | head | bull charge 6, axe mastery 6, scare 6, fearlessness 21 |
| Kuo Toa | hit, bite, pierce | - | water breathing 8, lightning bolt 15 |
| Orog | hit, punch, smash | - | summon horde 6, summon warg 8, warcaller's fury 11 |
| Harpy | claw, rake, peck | - | farsee 11, innate haste 16 |
| Stormkin | hit, punch, smash, crush | - | lightning bolt 10, thick hide 11 |
| Death Knight | hit, slash, smash | - | fire storm 13, fire shield 16, sacrilegious power 23 |
| Wight | hit, claw, thrash | - | frost breath 6, stoneskin 13 |
| Revenant | hit, claw, punch | - | battle frenzy 8, shadow jump 13 |
| Shadow Beast | claw, bite, rake | - | racial flurry 18 |
| Phantom | hit, thrash | - | gaseous form 10, wings 11, spell absorb 11 |

## Race point scores

RP is ability points + size + traits - credited drawbacks. Traits are the proposal's feats plus the
additions in the last column; the refund column is capped by composition rule 3 (1.75 Normal,
3.5 Advanced, 7 Epic, 12.5 Epic quest). Every Advanced, Epic, and Epic quest race has a
level-scaling trait (a weapon mastery, spell resistance, Hardy, a percentage damage reduction, or
four arms).

| Race | Band | Ability | Size | Traits | Refund | RP | Feats added to reach the band |
| -- | -- | -- | -- | -- | -- | -- | -- |
| Centaur | 12-16 | 7 | 1 | 6 | 2 | 12 | - |
| Githzerai | 12-16 | 6 | 0 | 6.5 | 0 | 12.5 | - |
| Firbolg | 12-16 | 7 | 1 | 6 | 1.25 | 12.75 | - |
| Githyanki | 12-16 | 3 | 0 | 9.5 | 0 | 12.5 | - |
| Kobold | 5-9 | 3 | 0 | 4 | 0 | 7 | - |
| Drider | 12-16 | 7 | 1 | 8 | 2 | 14 | - |
| Thri-Kreen | 20-28 | 4 | 0 | 23 | 7 | 20 | psionic resistance (see [Thri-Kreen](#thri-kreen)) |
| Minotaur | 12-16 | 7 | 1 | 7 | 3 | 12 | - |
| Kuo Toa | 5-9 | 3 | 0 | 4 | 1.75 | 5.25 | keen senses |
| Orog | 12-16 | 7 | 0 | 9 | 3.5 | 12.5 | hardy, armor skin x1 |
| Harpy | 12-16 | 4 | 0 | 11.5 | 0 | 15.5 | hardy |
| Stormkin | 12-16 | 7 | 1 | 6 | 1.25 | 12.75 | - |
| Death Knight | 40-60 | 16 | 1 | 28 | 4.25 | 40.75 | descend chassis, greatsword mastery |
| Wight | 40-60 | 16 | 1 | 27.5 | 3.25 | 41.25 | descend chassis, cold immunity |
| Revenant | 40-60 | 16 | 1 | 28.5 | 2.75 | 42.75 | descend chassis |
| Shadow Beast | 40-60 | 16 | 0 | 26.5 | 2.25 | 40.25 | descend chassis |
| Phantom | 40-60 | 16 | 0 | 37 | 2 | 51 | descend chassis |

Prices not in the [pricing table](../guides/PLAYER_RACES_REFERENCE.md#race-point-rp-pricing-table),
set by its closest row: stampede 2 (as bull charge); bodyslam, groundfighting, battle frenzy,
racial flurry, sacrilegious power (50 percent resistance to one type), psionic resistance, and
poison bite 1 each; doorbash, forest sight, seadog, miner, barter, calming, quick thinking, water
breathing, and undead fealty 0.5 each; innate haste 1.5 (short self-buff); magical reduction 2
(broader than energy resistance 5 to four types); enhanced spell damage 2 (as magical heritage);
thick hide 4 (15 percent of physical damage is about DR 4/- at mid levels and grows with it);
epic damage reduction 3 per rank (DR 3/-); fast healing 3 per rank (regeneration); toughness 2 (as
hardy). Four arms is 15, set in the
[reference](../guides/PLAYER_RACES_REFERENCE.md#race-point-rp-pricing-table).

## Descend forms

Duris sold its descend forms against a level reset and outcast status, which LuminariMUD does not
model, so their own kits fall well short of the Epic quest band. Each form gets the same chassis
of existing feats, built from Lich's defensive set plus Vampire's regeneration and toughness. All
are race grants and stack where marked:

| Chassis feat | RP |
| -- | -- |
| `FEAT_ARMOR_SKIN` x5 (stacking) | 5 |
| `FEAT_VITAL` | 0.5 |
| `FEAT_HARDY` | 2 |
| `FEAT_TOUGHNESS` | 2 |
| `FEAT_DAMAGE_REDUCTION` x3 (stacking, DR 9/-) | 9 |
| `FEAT_FAST_HEALING` x1 | 3 |
| Total | 21.5 |

Death Knight also gets `FEAT_GREATSWORD_MASTERY`, because Duris anti-paladins fight with
two-handers, and Wight gets `FEAT_COLD_IMMUNITY` to fit its frost breath.

Acquisition follows Lich and Vampire: a hard lock, then a quest reward race that converts the
character, respecs them, sets experience to 0 and alignment to -1000, and announces the change.
Each form requires level 30 and levels in one of its classes.

The classes come from Duris's help class lists (the same as `class_table[]`) and its newbie kits
(`src/account/newbie_kit_plan.c`). Duris's old `do_old_descend()` (`src/cmd/actoth.c`) turned
necromancers of level 35 or more into Wights (Warrior) and Revenants (Mercenary). Level 50
Revenants became Shadow Beasts (Assassin), and level 50 Vampires became Phantoms (Conjurer). Its
anti-paladin branch is commented out and has no conversion case, so Death Knight never got a
descend path, although its lore (fallen paladins casting hellfire) and its anti-paladin kit make it
the anti-paladin form. Duris's current `do_descend()` makes only Liches and is disabled. We keep
each form standalone rather than chaining them.

| Form | Duris classes | LuminariMUD class levels required | Respec to |
| -- | -- | -- | -- |
| Death Knight | Warrior, Mercenary; anti-paladin kit | Blackguard or Warrior | Blackguard |
| Wight | Warrior; mercenary kit | Warrior | Warrior |
| Revenant | Warrior, Mercenary | Warrior or Rogue | Warrior |
| Shadow Beast | Warrior, Mercenary, Thief; assassin kit | Rogue or Assassin | Rogue |
| Phantom | Psionicist, Conjurer, Reaver, Summoner | Wizard, Summoner, or Psionicist | Wizard |

Mercenary is Duris's warrior-rogue hybrid, and Conjurer is its summoning mage. Blackguard,
Psionicist, and Summoner are base classes here, and Assassin is a prestige class.

## Thri-Kreen

Thri-Kreen is an Epic race bought with account experience, not an Epic quest race. It scores 20
RP, the bottom of the Epic band. It costs 50000 by design decision, although composition rule 5
puts a race at or below the Epic target of 24 at 30000. The Epic refund cap of 7 credits both its
lost slots (6) and its cold vulnerability (1). These are answers to the open decisions in the
[race point pricing notes](../guides/PLAYER_RACES_REFERENCE.md#race-point-rp-pricing-table):

1. Price. Four arms is 15 RP. It is unique to Thri-Kreen, so it is exempt from the single-trait
   cap (7.2 at Epic). What it adds is in [EXTRA_LIMB_MECHANICS.md](EXTRA_LIMB_MECHANICS.md); the
   unarmed monk third hand that came with the arm count is not priced into it.
2. Psionic defence. Trelux already takes 20 percent less `DAM_MENTAL` damage through a race check
   in `compute_damtype_reduction()` (`src/combat/fight.c`). Make that check a feat, granted to
   Trelux and Thri-Kreen, at 1 RP. This follows how the imports turned the Trelux cold and leap
   checks into feats.
3. Venom. Keep `FEAT_POISON_BITE` at level 6. Its extra procs from the second pair are part of the
   four arms price.
4. Cannot ride. Leave it out. The lost slots and cold vulnerability already fill the Epic refund
   cap of 7, so a riding ban would cost the race play value without changing its score.
5. Ability modifiers. Only DEX gets a bonus (+4); STR and CON get none. The penalties are trimmed
   to -2 each, because the refund cap leaves no room to credit them.

## Code needed beyond the guide

The [race guide](../guides/ADDING_NEW_RACE_GUIDE.md) covers the registry, creation, unlock, help,
persistence, and test work. These race-specific changes come on top of it:

- The mental damage feat in Thri-Kreen decision 2.
- A `QST_RACE` conversion case for each descend form in `src/quest/quest.c`, plus its hard lock in
  `has_unlocked_race()` and `race_is_creation_eligible()`. Also review `NUM_EXTENDED_PC_RACES`
  (47), because IDs 55-59 sit above it.
- Leonine frame's neutral name, from the proposal's [notes](DURIS_RACE_FEAT_PROPOSAL.md#notes).
