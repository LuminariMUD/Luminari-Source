# Duris Player Races Plan

Status: in progress on branch `2-add-the-17-duris-player-races` (work item #2); see
[Progress](#progress). Every decision is settled (2026-09-26), and only the descend forms' quests
are left for later world work. Traced against master `2c5859388` and the Duris checkout at
`/home/aiwithapex/projects/duris` (race data read at `e1357a30a`, unchanged through `d86a642ad`).
It replaces `DURIS_RACE_SPECIFICATIONS.md`, `DURIS_RACE_FEAT_PROPOSAL.md`, and
`DURIS_RACIAL_IMPORTS.md`; read them with `git show 2c5859388:docs/ongoing-projects/<name>`.

This plan adds the 17 Duris player races that LuminariMUD does not cover: 12 creation races and 5
quest-only descend forms. Their racial innates are already imported as feats that no race holds
(#159, #181, #182, #183; see
[GAME_MECHANICS_SYSTEMS.md](../systems/GAME_MECHANICS_SYSTEMS.md#racial-innate-feats-and-spell-like-abilities)),
so each race is mostly `add_race()` data and `feat_race_assignment()` lines.
[ADDING_NEW_RACE_GUIDE.md](../guides/ADDING_NEW_RACE_GUIDE.md) is the procedure for the generic
work (registry, creation, unlock, hard lock, conversion, help, persistence, tests); this plan holds
the race data, the code the guide does not cover, and the order of work. The other 20 Duris player
races map to existing races (for example Barbarian to Wemic, both Illithids to Half-Illithid).

## Progress

- Step 1 done. Beyond the plan: an epic damage reduction DR entry has spell 0, which the player
  file's DR dedupe always skips, so it was never saved. `update_feat_damage_reduction()`
  (`src/character/class.c`) now builds it from the feat's real ranks, and `load_char()` calls it,
  so it is rebuilt on every load. `TestSep2026InnateFeatsAreRegisteredAsInnates` pins the last
  feat; a new feat moves it. The renamed Wemic, QUADRUPED-BODY, and SQL help are edited in the
  files but not yet applied to the development help database; step 4 applies all help together.
- Step 2 done. The race blocks follow Yuan-Ti in `assign_races()` under a "Duris races" comment.
  Registry `type` is the display name ("Thri-Kreen", "Kuo Toa"), `name` the token. Beyond the
  plan: `process_race_level_feats()` reads `GET_REAL_RACE()`, so a disguise or wild shape cannot
  trade away a later-level grant (item 2 of the login repair plan); Centaur and Minotaur are
  furry, Minotaur and Kobold horned, Kobold and Kuo Toa scaled, and Thri-Kreen and Kuo Toa
  hairless for description choices; protocol v1 race summaries are 140 characters
  (`WEB_ONBOARDING_V1_RACE_SUMMARY_CHARS`), because v1 lists every race on one page and the full
  45 overflowed the 15000-byte payload at 220. Tests: the `TestDurisCreationRaces*` and
  `TestRaceLevelFeatsFollowTheRealRace` cases in `test_race_equivalence.c`, the media key and
  wire budget cases in `test_web_onboarding.c`.
- Step 3 done. The form blocks follow Vampire in `assign_races()`. The conversion table is
  `descend_form_conversions[]` in `src/quest/quest.c`, whose `descend_form_preflight()` runs
  inside the race-reward preflight and whose `convert_to_descend_form()` is the switch's default
  case. QEDIT accepts `-1` or any `race_is_transformation_only()` race. Tests:
  `TestDescendForm*` in `test_race_equivalence.c` (the conversion tests load the class table),
  the forged-form case in the web wire budget test, and `scripts/world/tests/test_semantics.py`.
  No quest uses a form yet.
- Step 4 done on development. `help_duris_races_entries.sql` holds the 17 topics and the new
  EPIC-RACES body. Minotaur updates the retired Dragonlance `minotaur` entry in place, which
  already owned the MINOTAUR and RACE-MINOTAUR keywords, so nothing is deleted. The development
  help database took the rows HEDIT-style (the four updated bodies archived in `help_versions`
  with `changed_by` `duris-races`), and `help.hlp` was rendered from it with the help-sync
  catalog (`take_snapshot().file_matches` is true). Production help is not touched; it needs the
  help-sync skill and the owner's authorization after merge.

## Rules behind the data

- Sources. Duris `lib/duris.properties` gives stat factors, casting multipliers
  (`spellcast.pulse.racial.<Race>`), shrug (`innate.shrug.<Race>`), and spell power
  (`stats.pow.<Race>`); `set_char_size()` in `src/account/nanny.c` gives sizes; `class_table[]` and
  `restricted_class_rows[]` in `src/core/constant.c` give alignments; `src/classes/innates.c` gives
  innates and their levels. Race descriptions and help can be adapted from the race entries in
  Duris `lib/information/help_index`, without its racewar sides.
- Abilities. The raw line: every 10 percent of a Duris stat factor above or below 100 is one point,
  and DEX is the average of Duris Dex and Agi. The proposed line fits the tier: bonuses only on
  stats Duris raises, in Duris order, within composition rule 1 (ability points at most half the
  budget) and the +10 cap (Fae); penalties only on stats Duris lowers. Drawbacks are credited
  first, penalties take whatever room composition rule 3 leaves, and a penalty with no room earns
  no credit.
- Size. Duris Huge becomes Large; a Huge player size was declined (#163).
- Alignment. A race allows every alignment that any of its Duris classes allows: "any" stays any,
  "evil or neutral" becomes not good (LN, TN, CN, LE, NE, CE), and "evil" becomes evil only. Kuo
  Toa, Orog, Harpy, and Storm Giant have `class_table[]` rows that allow no class, so their
  `restricted_class_rows[]` stand-ins are used. Stormkin is the exception: its stand-in copies
  Ogre's evil-only row, but Duris gives Storm Giant no racewar side, so Stormkin allows any.
- Feat levels. A gated Duris innate is granted at half its Duris level, rounded up (Duris mortals
  reach 56). A feat borrowed from a covered race takes its earliest Duris owner's level. Every other
  feat is granted at level 1.
- Casting speed. One rank of `FEAT_FAST_CASTING` or `FEAT_SLOW_CASTING` per 10 percent of the Duris
  multiplier, rounded to the nearest rank: Wight 1.9 (9 slow), Orog and Stormkin 1.6 (6), Death
  Knight 1.5 (5), Revenant 1.3 (3), Shadow Beast and Kuo Toa 1.1 (1), Firbolg 1.055 (1); Phantom and
  Harpy 0.7 (3 fast), Kobold 0.895 (1). The other six are within 5 percent of 1.0 and get none.
- Spell resistance and power. Duris shrug 20 to 25 maps to `FEAT_HALF_DROW_SPELL_RESISTANCE`
  (Githzerai and Phantom 20, Githyanki and Drider 25). One rank of `FEAT_ENHANCED_SPELL_DAMAGE` goes
  to Duris Pow 125 or more (Githyanki 130, Phantom 125, not Githzerai 120).
- Language. The nearest `SKILL_LANG_*`; the gith use Common (there is no Gith language); the
  descend forms get none, like Lich and Vampire.
- Declined, so in no race here: the Duris experience factor, racial hit points as race data, and
  troll regeneration ranks (#163); melee multipliers and attack round speed (#164); a race class
  deny list and a shared descend routine (#166). `FEAT_DAYBLIND` goes to no race (Duris comments it
  out on all six of these races that list it, and most already carry sun vulnerability).
  `FEAT_EXTRA_ARMS`, a separate feat that adds one arm per rank, goes to no race either:
  Thri-Kreen gets its four arms from `FEAT_FOUR_ARMS` (two arms plus two), and the feats add up, so
  Extra Arms on top would give it five or six. Duris dayvision, summon totem, and project image are
  unimplemented in Duris itself.

## Race data

### Registry

IDs follow [guide section 2](../guides/ADDING_NEW_RACE_GUIDE.md#2-resolve-the-numeric-id-before-implementation):
the creation races append after Yuan-Ti (151), raising `NUM_EXTENDED_RACES` to 164 and
`NUM_CREATION_RACES` to 45; the descend forms take IDs 55-59, reserved for quest-only races. The
retired `LEGACY_RACE_MINOTAUR` (37) is not reused. `NUM_EXTENDED_PC_RACES` (47) has no consumer,
so IDs 55-59 above it need no change. Every race allows male and female. XP is the `level_exp()`
multiplier that every existing race of the tier has (code item 6). Epic quest means Lich's hard
lock with a nominal cost of 999999999. Stormkin is a custom Large race, roughly half storm giant,
that takes the Duris Storm Giant's converted stats and feats.

| Race | ID | Token, abbrev | Tier: cost, LA, XP | Size | Family | Alignment | Language |
| -- | -- | -- | -- | -- | -- | -- | -- |
| Centaur | 152 | centaur, Cent | Advanced: 1000, +2, x2 | Large | Monstrous humanoid | any | Elven |
| Githzerai | 153 | githzerai, Gthz | Advanced: 1000, +2, x2 | Medium | Humanoid | any | Common |
| Firbolg | 154 | firbolg, Fbol | Advanced: 1000, +2, x2 | Large | Giant | any | Giant |
| Githyanki | 155 | githyanki, Gthy | Advanced: 1000, +2, x2 | Medium | Humanoid | not good | Common |
| Kobold | 156 | kobold, Kobo | Normal: 0, +0, x1 | Small | Humanoid | not good | Kobold |
| Drider | 157 | drider, Drdr | Advanced: 1000, +2, x2 | Large | Aberration | evil only | Undercommon |
| Thri-Kreen | 158 | thrikreen, TKrn | Epic: 50000, +10, x7 | Medium | Monstrous humanoid | any | Common |
| Minotaur | 159 | minotaur, Mino | Advanced: 1000, +2, x2 | Large | Monstrous humanoid | any | Giant |
| Kuo Toa | 160 | kuotoa, KToa | Normal: 0, +0, x1 | Medium | Monstrous humanoid | evil only | Undercommon |
| Orog | 161 | orog, Orog | Advanced: 1000, +2, x2 | Medium | Humanoid | not good | Orcish |
| Harpy | 162 | harpy, Hrpy | Advanced: 1000, +2, x2 | Small | Monstrous humanoid | any | Common |
| Stormkin | 163 | stormkin, Stmk | Advanced: 1000, +2, x2 | Large | Giant | any | Giant |
| Death Knight | 55 | deathknight, DKni | Epic quest: locked, +10, x10 | Large | Undead | evil only | - |
| Wight | 56 | wight, Wght | Epic quest: locked, +10, x10 | Large | Undead | evil only | - |
| Revenant | 57 | revenant, Rvnt | Epic quest: locked, +10, x10 | Large | Undead | evil only | - |
| Shadow Beast | 58 | shadowbeast, SBst | Epic quest: locked, +10, x10 | Medium | Undead | evil only | - |
| Phantom | 59 | phantom, Phnt | Epic quest: locked, +10, x10 | Medium | Undead | evil only | - |

### Stats and race points

RP is ability points + size + traits - refund, scored with the
[pricing table](../guides/PLAYER_RACES_REFERENCE.md#race-point-rp-pricing-table) against the
[tier bands](../guides/PLAYER_RACES_REFERENCE.md#tier-budgets). Ability points are the bonuses minus
the credited penalties. Traits are the beneficial feats in the feat table; the refund is its
drawbacks and lost slots, capped by composition rule 3 (1.75 Normal, 3.5 Advanced, 7 Epic, 12.5 Epic
quest). Order is STR/CON/INT/WIS/DEX/CHA.

| Race | Duris raw | Proposed | Ability | Size | Traits | Refund | RP (band) |
| -- | -- | -- | -- | -- | -- | -- | -- |
| Centaur | +4/+6/-2/0/-1/-1 | +3/+5/-1/0/0/0 | 7 | 1 | 6 | 2 | 12 (12-16) |
| Githzerai | 0/0/+2/+1/0/-1 | 0/0/+4/+3/0/-1 | 6 | 0 | 6.5 | 0 | 12.5 (12-16) |
| Firbolg | +13/+10/-3/-2/-3/-2 | +5/+4/-1/0/-1/0 | 7 | 1 | 6 | 1.25 | 12.75 (12-16) |
| Githyanki | 0/0/+2/0/0/-3 | 0/0/+4/0/0/-1 | 3 | 0 | 9.5 | 0 | 12.5 (12-16) |
| Kobold | -1/-1/+3/+1/+2/0 | -1/0/+2/0/+2/0 | 3 | 0 | 4 | 0 | 7 (5-9) |
| Drider | -1/+2/0/-1/+1/-3 | 0/+5/0/0/+3/-1 | 7 | 1 | 8 | 2 | 14 (12-16) |
| Thri-Kreen | +2/+1/-4/-4/+3/-3 | 0/0/-2/-2/+4/-2 | 4, penalties uncredited | 0 | 23 | 7 | 20 (20-28) |
| Minotaur | +7/+7/-2/-2/-2/-3 | +3/+4/0/0/0/0 | 7 | 1 | 7 | 3 | 12 (12-16) |
| Kuo Toa | +4/+4/-2/-2/+2/-1 | +1/+2/0/0/0/0 | 3 | 0 | 4 | 1.75 | 5.25 (5-9) |
| Orog | +6/+7/-6/-2/+2/-5 | +3/+4/-2/0/0/0 | 7, INT uncredited | 0 | 9 | 3.5 | 12.5 (12-16) |
| Harpy | -2/+1/+2/+1/+2/0 | -2/+1/+2/+1/+2/0 | 4 | 0 | 11.5 | 0 | 15.5 (12-16) |
| Stormkin | +5/+5/-2/-2/-3/-4 | +5/+4/0/0/-2/0 | 7 | 1 | 6 | 1.25 | 12.75 (12-16) |
| Death Knight | +2/+2/0/-1/-1/-3 | +8/+6/-2/+6/-2/0 | 16 | 1 | 28 | 4.25 | 40.75 (40-60) |
| Wight | +4/+6/-4/-4/-2/-5 | +10/+10/-2/0/0/-2 | 16 | 1 | 27.5 | 3.25 | 41.25 (40-60) |
| Revenant | +5/+7/-3/-3/0/-5 | +7/+7/-2/0/+6/-2 | 16 | 1 | 28.5 | 2.75 | 42.75 (40-60) |
| Shadow Beast | +2/+1/-3/-3/+3/-3 | 0/+7/+3/-2/+10/-2 | 16 | 0 | 26.5 | 2.25 | 40.25 (40-60) |
| Phantom | -1/0/0/0/+4/0 | -2/+5/+10/0/+3/0 | 16 | 0 | 37 | 2 | 51 (40-60) |

Harpy keeps its raw line. The descend forms follow a set emphasis instead of their Duris
direction, each at 16 ability points: Death Knight STR, CON, and WIS; Wight STR and CON; Revenant
STR, CON, and DEX evenly; Shadow Beast DEX, CON, and some INT; Phantom INT, moderate CON, and some
DEX. Every Advanced, Epic, and Epic quest race has a level-scaling trait (a weapon mastery, spell
resistance, Hardy, thick hide, or Four Arms).

Traits not in the pricing table are priced by its closest row: stampede 2 (as bull charge);
bodyslam, groundfighting, battle frenzy, racial flurry, sacrilegious power (50 percent resistance
to one type), psionic resistance, and poison bite 1 each; doorbash, forest sight, seadog, miner,
barter, calming, quick thinking, water breathing, and undead fealty 0.5 each; innate haste 1.5
(short self-buff); magical reduction 2 (broader than energy resistance 5 to four types); enhanced
spell damage 2 (as magical heritage); thick hide 4 (15 percent of physical damage is about DR 4/-
at mid levels and grows with it); epic damage reduction 3 per rank (DR 3/-); fast healing 3 per rank
(regeneration); toughness 2 (as hardy). Spell-like abilities price by their daily uses in
`get_daily_uses()`.

### Body

Attack types are the `set_race_attack_types()` flags used for wild shape and disguise. Lost slots
are `set_race_wear_restriction()` rows, except that Centaur and Drider lose legs and feet through
the tauric frame feat (code item 1).

| Race | Attack types | Lost slots |
| -- | -- | -- |
| Centaur | hit, punch, trample, charge | legs, feet (tauric frame) |
| Githzerai | hit, punch | - |
| Firbolg | hit, punch, smash | - |
| Githyanki | hit, slash, punch | - |
| Kobold | hit, bite, claw | - |
| Drider | bite, pierce, claw | legs, feet (tauric frame) |
| Thri-Kreen | bite, claw, slash | body, feet, both fingers, both ears |
| Minotaur | hit, gore, charge | head |
| Kuo Toa | hit, bite, pierce | - |
| Orog | hit, punch, smash | - |
| Harpy | claw, rake, peck | - |
| Stormkin | hit, punch, smash, crush | - |
| Death Knight | hit, slash, smash | - |
| Wight | hit, claw, thrash | - |
| Revenant | hit, claw, punch | - |
| Shadow Beast | claw, bite, rake | - |
| Phantom | hit, thrash | - |

### Feats

Every grant, as `FEAT_*` names without the prefix. A number after a feat is its grant level
(otherwise 1), and "xN" is N `feat_race_assignment()` lines. The chassis is the descend forms'
shared set (see [Descend forms](#descend-forms)).

| Race | Feats |
| -- | -- |
| Centaur | QUADRUPED_BODY, TAURIC_FRAME, DOORBASH, STAMPEDE 11, GREATSWORD_MASTERY 16 |
| Githzerai | ULTRAVISION, HALF_DROW_SPELL_RESISTANCE, SLA_PLANE_SHIFT, QUICK_THINKING, SLA_LEVITATE 6, RRAKKMA 11 |
| Firbolg | BODYSLAM, DOORBASH, FOREST_SIGHT, MAGIC_VULNERABILITY, SLOW_CASTING x1, OUTDOOR_STEALTH 6, HATRED 11, HAMMER_MASTERY 16 |
| Githyanki | ULTRAVISION, HALF_DROW_SPELL_RESISTANCE, SLA_PLANE_SHIFT, ENHANCED_SPELL_DAMAGE, SLA_PSIONIC_BLAST, SLA_LEVITATE 6, LONGSWORD_MASTERY 6 |
| Kobold | ULTRAVISION, UNDERDARK_STEALTH, CALMING, BARTER, FAST_CASTING x1, MINER 26 |
| Drider | ULTRAVISION, HALF_DROW_SPELL_RESISTANCE, QUADRUPED_BODY, TAURIC_FRAME, SLA_WEB, GROUNDFIGHTING 11, SLA_FIREBALL 11, SLA_MASS_DISPEL 27 |
| Thri-Kreen | ULTRAVISION, FOUR_ARMS, PSIONIC_RESISTANCE, VULNERABLE_TO_COLD, POISON_BITE 6, LEAP 11 |
| Minotaur | ULTRAVISION, DOORBASH, BLOODLUST, BULL_CHARGE 6, AXE_MASTERY 6, SLA_SCARE 6, KENDER_FEARLESSNESS 21 |
| Kuo Toa | ULTRAVISION, KEEN_SENSES, SWAMP_STEALTH, SEADOG, SUN_VULNERABILITY, SLOW_CASTING x1, WATER_BREATHING 8, SLA_LIGHTNING_BOLT 15 |
| Orog | ULTRAVISION, HARDY, ARMOR_SKIN x1, MAGICAL_REDUCTION, SUN_VULNERABILITY, SLOW_CASTING x6, SUMMON_HORDE 6, SUMMON_WARG 8, WARCALLERS_FURY 11 |
| Harpy | ULTRAVISION, WINGS, KEEN_SENSES, HARDY, FAST_CASTING x3, SLA_FARSEE 11, HASTE 16 |
| Stormkin | INFRAVISION, DOORBASH, SLOW_CASTING x6, SLA_LIGHTNING_BOLT 10, THICK_HIDE 11 |
| Death Knight | chassis, GREATSWORD_MASTERY, ULTRAVISION, TIEFLING_HELLISH_RESISTANCE, UNDEAD_FEALTY, SUN_VULNERABILITY, SLOW_CASTING x5, SLA_FIRE_STORM 13, SLA_FIRE_SHIELD 16, SACRILEGIOUS_POWER 23 |
| Wight | chassis, COLD_IMMUNITY, ULTRAVISION, BODYSLAM, DOORBASH, WEAKNESS_TO_FIRE, SLOW_CASTING x9, SLA_FROST_BREATH 6, SLA_STONESKIN 13 |
| Revenant | chassis, ULTRAVISION, TROLL_REGENERATION, BODYSLAM, DOORBASH, WEAKNESS_TO_FIRE, SLOW_CASTING x3, BATTLE_FRENZY 8, SLA_SHADOW_JUMP 13 |
| Shadow Beast | chassis, ULTRAVISION, UNDERDARK_STEALTH, SLA_STRENGTH, SLA_ENLARGE, WEAKNESS_TO_FIRE, SLOW_CASTING x1, RACIAL_FLURRY 18 |
| Phantom | chassis, ULTRAVISION, HALF_DROW_SPELL_RESISTANCE, SLA_PLANE_SHIFT, ENHANCED_SPELL_DAMAGE, EYELESS, WEAKNESS_TO_FIRE, FAST_CASTING x3, VAMPIRE_GASEOUS_FORM 10, WINGS 11, SPELL_ABSORB 11 |

Not from Duris, added to reach the band: Kuo Toa KEEN_SENSES; Orog HARDY and ARMOR_SKIN x1; Harpy
HARDY; Thri-Kreen PSIONIC_RESISTANCE; the chassis; Death Knight GREATSWORD_MASTERY (Duris
anti-paladins fight with two-handers); Wight COLD_IMMUNITY (to fit its frost breath). Githyanki
drops the Duris sun vulnerability, a racewar device rather than gith lore.

## Descend forms

Duris sold its descend forms against a level reset and outcast status, which LuminariMUD does not
model, and made them strong with melee multipliers (declined in #164), so their own kits fall far
short of the Epic quest band. Each form gets the same chassis of existing feats, Lich's defensive
set plus Vampire's regeneration and toughness, 21.5 RP: ARMOR_SKIN x5 (5), VITAL (0.5), HARDY (2),
TOUGHNESS (2), DAMAGE_REDUCTION x3 (DR 9/-, 9), and FAST_HEALING x1 (3).

Acquisition follows Lich and Vampire: a hard lock, then a standard `.qst` race reward that converts
the character, respecs them, sets experience to 0 and alignment to -1000, and announces the change.
Each form requires level 30 and levels in one of its classes, and the forms stay standalone: no
form converts into another.

| Form | Duris classes | Class levels required | Respec to |
| -- | -- | -- | -- |
| Death Knight | Warrior, Mercenary; anti-paladin kit | Blackguard or Warrior | Blackguard |
| Wight | Warrior; mercenary kit | Warrior | Warrior |
| Revenant | Warrior, Mercenary | Warrior or Rogue | Warrior |
| Shadow Beast | Warrior, Mercenary, Thief; assassin kit | Rogue or Assassin | Rogue |
| Phantom | Psionicist, Conjurer, Reaver, Summoner | Wizard, Summoner, or Psionicist | Wizard |

The Duris classes come from its help class lists (the same as `class_table[]`) and newbie kits
(`src/account/newbie_kit_plan.c`). Duris's old `do_old_descend()` (`src/cmd/actoth.c`) let
necromancers of level 35 or more become Wights (as Warriors) or Revenants (as Mercenaries), level
50 Revenants Shadow Beasts (Assassin), and level 50 Vampires Phantoms (Conjurer). Its anti-paladin
branch is commented out, so Death Knight never had a descend path, though its lore (fallen
paladins casting hellfire) and kit make it the anti-paladin form. Duris's current `do_descend()`
makes only Liches and is disabled. Mercenary is Duris's warrior-rogue hybrid and Conjurer its
summoning mage; Blackguard, Psionicist, and Summoner are base classes here, Assassin a prestige
class.

## Thri-Kreen

Thri-Kreen is an Epic race bought with account experience, not an Epic quest race. These answer
the open decisions in the
[race point pricing notes](../guides/PLAYER_RACES_REFERENCE.md#race-point-rp-pricing-table):

1. Price. Epic at 50000 by design decision, although composition rule 5 puts its 20 RP at 30000.
   Four Arms is 15 RP and, unique to Thri-Kreen, exempt from the single-trait cap (7.2 at Epic).
   What it adds is in [EXTRA_LIMB_MECHANICS.md](EXTRA_LIMB_MECHANICS.md); the unarmed monk third
   hand that comes with the arm count is not priced into it.
2. Psionic defence. `FEAT_PSIONIC_RESISTANCE` (code item 2), 1 RP, granted to Trelux and
   Thri-Kreen, the way the imports turned the Trelux cold and leap checks into feats.
3. Venom. Keep POISON_BITE at level 6; its extra procs from the second pair are part of the Four
   Arms price.
4. Riding. No restriction. The lost slots (6) and cold vulnerability (1) already fill the Epic
   refund cap of 7, so a riding ban would cost play value without changing the score.
5. Abilities. Only DEX gets a bonus (+4); STR and CON get none. The penalties are trimmed to -2
   each, because the refund cap leaves no room to credit them.

## Code beyond the guide

Each item is needed by the races above; file and function names were traced on `2c5859388`.

01. Tauric frame. `FEAT_LEONINE_FRAME` (1267) says "leonine" in its name and help text, and its
    refusal in `character_wear_slot_restriction()` (`src/character/race.c`) says "four-legged",
    which fits neither the horse-bodied Centaur nor the eight-legged Drider. Rename it to
    `FEAT_TAURIC_FRAME`, "tauric frame", with the same ID and neutral text, and update the two test
    files that name it, the Wemic help (`help_race_wemic_entries.sql`, its verifier, which checks
    for "Leonine Frame", and `help.hlp`), and the QUADRUPED BODY see-also line. Wemic's own
    `set_race_wear_restriction()` rows stay.
02. Psionic resistance. The Trelux 20 percent `DAM_MENTAL` reduction in
    `compute_damtype_reduction()` (`src/combat/fight.c`) is one of its per-type exoskeleton checks.
    Add `FEAT_PSIONIC_RESISTANCE` as 1322 (`FEAT_LAST_FEAT` 1323, `NUM_FEATS` 1324, then
    `python3 scripts/world/wtool.py constants sync --write`), grant it to Trelux and Thri-Kreen at
    level 1, and make that case read the race check or the feat. Race feats are saved only when
    first granted (`process_race_level_feats()` runs from `advance_level()`), so saved Trelux
    characters never receive the new grant and keep the race check.
03. Cold immunity. `FEAT_COLD_IMMUNITY` has no reader: Lich's immunity is an `IS_LICH()` check in
    `compute_damtype_reduction()`. Make the cold test
    `IS_LICH(ch) || HAS_FEAT(ch, FEAT_COLD_IMMUNITY)` so Wight's grant works and saved Liches are
    unchanged.
04. Race-granted damage reduction. `FEAT_DAMAGE_REDUCTION` (epic damage reduction) creates its DR
    entry only when taken through study (`src/character/study.c`); the flat read in
    `compute_damage_reduction_full()` is commented out. Move the study block into a helper and call
    it from `process_race_level_feats()` after it raises that feat, so the chassis's three grants
    give DR 9/-. `init_start_char()` frees the DR list, and the respec's level-one grants rebuild
    it.
05. Hardy and Vital hit points. Both feats are text only; the hit points come from
    `race_starting_hp_bonus()` and `race_hp_bonus_per_level()` in `src/character/race.c`. Add Orog
    and Harpy at +1 per level (as Wemic, which also holds Hardy) and the five forms at +10 once and
    +1 per level, as priced. Lich and Vampire's 4 per level is not in the race point table and is
    not copied.
06. XP. Add each race to its tier's case in `level_exp()` (`src/character/class.c`): the nine
    Advanced races x2, Thri-Kreen x7, and the forms x10; Kobold and Kuo Toa get none.
07. Family predicates. `IS_MONSTROUS_HUMANOID()`, `IS_GIANT()`, `IS_ABERRATION()`, `IS_UNDEAD()`,
    and the PC exclusion list in `IS_HUMANOID()` (`src/core/utils.h`) name each non-humanoid PC
    race, so the registry family alone changes nothing. Add Centaur, Thri-Kreen, Minotaur, Kuo Toa,
    and Harpy; Firbolg and Stormkin; Drider; and the forms (one `IS_DESCEND_FORM()` over 55-59), all
    by `GET_REAL_RACE()` as `IS_WEMIC()` does, and add all thirteen to the humanoid exclusion.
08. Hard lock. Replace the Lich and Vampire tests in `race_is_creation_eligible()`
    (`src/character/race.c`) and `has_unlocked_race()` (`src/player/account.c`) with one
    transformation-only predicate that covers Lich, Vampire, and the forms.
09. Respec argument. `respec_engine()` (`src/act/act.other.c`) dereferences `arg` for every race but
    Lich and Vampire, and the quest path passes `NULL`; make that check NULL-safe.
10. Conversion. In `complete_quest()` (`src/quest/quest.c`), one table (form, required classes,
    respec class, announcement) drives the added preflight checks (a level in a listed class; real
    race not already transformation-only) and the conversion for the five forms, which saves after
    the final XP and alignment. Lich and Vampire keep their cases. Accept 55-59 in `qedit`
    (`src/olc/qedit.c`, prompt and value check), `scripts/world/wtool_lib/constants.py`,
    `scripts/world/wtool_lib/semantics.py` with `scripts/world/tests/test_semantics.py`, and
    `docs/world_game-data/QUEST_FILE_FORMAT.md`.
11. Parser. Append each token after the existing chain in `parse_race_long()`, plus the display name
    with a space or hyphen (`thri-kreen`, `kuo toa`, `death knight`, `shadow beast`), because web
    onboarding sends the registry `type` as its `wireValue`. Appending keeps every existing prefix:
    `sto` stays Stout Halfling, `shad` Shade, `dr` Drow, `wi` Wild Elf, and `o` Half-Ogre.

## Steps

Each step ends green on `make -j$(nproc)`, `make test`, and `make install`.

1. Shared mechanics: code items 1-4, 8 (Lich and Vampire only), and 9, tested in
   `test_racial_innate_feats.c` and `test_race_equivalence.c`: a feat holder takes 20 percent less
   mental damage and a race-checked Trelux still does; a COLD_IMMUNITY holder and a Lich are cold
   immune; three race grants of DAMAGE_REDUCTION give one DR 9 entry; and
   `respec_engine(ch, class, NULL, TRUE)` is safe.
2. Creation races (152-163): constants and bounds in `src/core/structs.h`, `assign_races()` blocks
   from the tables, code items 5-7 and 11 for them, a `CON_QRACE` help case each, and `race/<slug>`
   keys in `web_onboarding_race_media_key()` (art in the web client repository, fallback until
   then). Extend `test_race_equivalence.c` (IDs and counts, every registry field, parser round trip,
   feat ranks and levels, hit points, XP, family predicates, lost slots, a class and alignment that
   can finish creation) and `test_web_onboarding.c` (media keys, catalog, wire budget). With them
   the account needs 22 of its 50 unlock slots for every locked race.
3. Descend forms (55-59): registry blocks and code items 5-8, 10, and 11 for them, with tests that a
   forged unlock is refused and the forms stay out of both catalogs; that a failed preflight
   changes nothing; that success sets race, class, XP 0, alignment -1000, size, level-one feats, and
   DR, and survives save and reload; and that a second conversion is refused. Account cards use the
   fallback image, as Lich and Vampire do. The quests themselves come later, one per form, modeled
   on the Vampire line (quest 34721, zone 347); until they exist no character can become a form.
4. Help: one `RACE-<SLUG>` topic per race in one component,
   `sql/components/help_duris_races_entries.sql` with `verify_help_duris_races_entries.sql`
   (manifest `apply` and `skip`, `Makefile.am` `EXTRA_DIST`), mirrored in `lib/text/help/help.hlp`;
   the PSIONIC RESISTANCE entry in `help_other_racial_innate_entries.sql` and `help.hlp`; Thri-Kreen
   added to EPIC-RACES. Production help goes through the help-sync skill.
5. Docs: add the races, feats, calibration rows, new trait prices, and descend acquisition to
   `PLAYER_RACES_REFERENCE.md`, replace its Thri-Kreen open decisions with the answers above, and
   correct its claim that the tiers differ only by unlock cost (`level_exp()` multiplies XP by
   tier). Record Four Arms on Thri-Kreen in `EXTRA_LIMB_MECHANICS.md` and code items 2-4 in
   `GAME_MECHANICS_SYSTEMS.md`. Then delete this plan and its index entries.
6. Live check on port 4100 (`MUD_PORT=4100 ./scripts/autorun/autorun.sh`): buy and create a locked
   race through the terminal and the web, create a Kobold from a premade build, level through a
   later grant, and reconnect each character. A form's live conversion is checked when its quest
   exists.

Done when the guide's final review checklist holds for all 17 races, apart from the descend forms'
live conversion, which waits for their quests.

## Simplifications

- One help component for the 17 topics instead of 17 pairs, as the racial innate import did.
- One conversion table for the five forms instead of five copies of the Lich case.
- The existing test suites take the new cases, so no test file and no build manifest change.
- Race checks stay beside the new feat checks (items 2 and 3) instead of migrating saved
  characters.
