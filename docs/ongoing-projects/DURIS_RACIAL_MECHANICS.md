# Duris Racial Mechanics

Status: the innate-feat layer is complete and merged (PR #159, 2026-09-12).
This document now serves two purposes. The first half is the gap list: every
race-level mechanic Duris uses that LuminariMUD cannot yet express, so that
all 37 Duris player races could be defined here as data once the gaps are
closed. Moving the races themselves is not planned; having the mechanics in
place is. The second half is a concise record of the feat work already done.
Companion study: [DURIS_RACE_CONVERSION.md](DURIS_RACE_CONVERSION.md)
(stat conversion, race point scores, per-race trait pricing).
Duris source verified at `/home/aiwithapex/projects/duris`; our side traced in
`src/character/race.c`, `src/character/class.c`, `src/character/feats.c`,
`src/obj/act.item.c`, `src/combat/fight.c`, `src/magic/spell_parser.c`,
`src/limits.c`, and `src/quest/quest.c`.

## Part 1: mechanic gaps

Duris builds a race from the levers in the study's "How Duris builds a race"
table plus a handful of body and wield rules keyed to race constants. Each
lever below is marked as covered (only race data is needed), partial, or
missing (code is needed). The previous PR converted the innates; the gaps
that remain are the levers that are not innates.

### Levers already covered: race data only

| Duris lever | Our mechanism | Note |
|-------------|---------------|------|
| Stat factors, stat bonus | `ability_mods[]` in `add_race()` | Conversion rule in the study. Firbolg and Ogre exceed our modifier range and use the study's compressed lines |
| Size Small, Medium, Large | `size` in `add_race()` | Duris Huge (Wight) maps to Large; see the Huge note under partial |
| Shrug (spell resistance) | `FEAT_HALF_DROW_SPELL_RESISTANCE`, `FEAT_DROW_SPELL_RESISTANCE`, `FEAT_LICH_SPELL_RESIST` | Now feat-gated in `compute_spell_res()`; assign by shrug band |
| Racial saves | `FEAT_SPELL_HARDINESS`, `FEAT_STRONG_SPELL_HARDINESS`, the general save feats | Race-assignable with `feat_race_assignment()` |
| Every registered player-race innate | The feats in Part 2 | 48 new feats plus the wired and reused ones |
| Level-gated innates | `feat_race_assignment(race, feat, level, stacks)` | Gates rescaled by 30/56 (L11 to 6, L21 to 11, L31 to 17, L41 to 22, L46 to 25) |
| Sun vulnerability, Dayblind | `FEAT_SUN_VULNERABILITY`, `FEAT_DAYBLIND` | Regeneration stop, daylight damage, cover, blindness |
| Regeneration (Revenant 4, Troll 9) | `FEAT_TROLL_REGENERATION` | Fixed 3 per tick; see partial for the Troll rank |
| Lost equipment slots (Ogre, Minotaur head, Thri-Kreen body/foot/finger/ear, Drider and Centaur legs/feet) | `wear_slot_restrictions[]` per race, `FEAT_LEONINE_FRAME` | One rejection string per slot per race |
| Alignment and gender limits | `set_race_alignments()`, `set_race_genders()` | |
| Racial language | `racial_language` | |
| Availability groups (roster, legacy, lore-restricted) | `is_pc`, `unlock_cost`, `epic_adv`, `is_locked_race()` | Tiers replace Duris's racewar rosters |
| Dayvision, Summon Totem, Project Image | none | No implementation in Duris either |
| Racewar side, hometown, multiclass rules | none | Duris-wide systems, not race features; out of scope |

### Partial: mechanism exists but needs a small change

| Gap | Races | Current state | Work |
|-----|-------|---------------|------|
| Experience factor | Every race (Human 1.3, Lich and Illithid 0.1) | `race_data.level_adjustment` is stored by `add_race()` and read nowhere; `gain_exp()` in `src/limits.c` has no race term | Apply it in `gain_exp()` as a percent modifier, or decide that tiers are the only balance lever and delete the field. Duris uses this as its enforced level adjustment, so a race adopted at its Duris score needs it |
| Troll regeneration rank | Troll (10 per tick) | `FEAT_TROLL_REGENERATION` is `can_stack = FALSE` and grants a fixed 3 | Make it stackable at +3 per rank, or add a second feat |
| Racial hit points | Any new race with a Duris Con factor above ours | `race_starting_hp_bonus()` and `race_hp_bonus_per_level()` in `src/character/race.c` are switch statements on race constants | Move both into `race_data` fields set by `add_race()` so a new race is data, not a code edit |
| Huge player size | Wight, Storm Giant | `add_race()` accepts `SIZE_HUGE` but no PC race uses it; wield, wear, mount and room capacity rules are unverified for a Huge PC | Either keep the study's rule (Huge maps to Large) or audit `hands_needed_full()`, `character_wear_slot_restriction()` and `do_mount` for Huge before allowing it |
| Class list per race | Ogre, Firbolg, Storm Giant (two classes), every race with a Duris class list | `CLASS_PREREQ_RACE` in `src/character/class.c` is an allow-list on the class side: once a class carries any race prereq, only listed races qualify. Forbidding one race from a class would mean listing every other race on that class | Add a race-side deny list (`class_restrictions[NUM_CLASSES]` in `race_data`, checked next to the race prereq in `meets_class_prerequisite()`) |
| Spell power (Pow stat) | Illithid (200), Githyanki (130), casters generally | No Pow stat. `FEAT_ENHANCED_SPELL_DAMAGE` is a stackable class feat | Confirm it is race-assignable and consumed for all spell damage, or add a racial spell-power feat (damage and DC) |
| Undead descend forms | Lich, Vampire, Death Knight, Wight, Revenant, Shadow Beast, Phantom, Shade | Lich and vampire conversions are one-off paths in `src/quest/quest.c` and `src/quest/hlquest.c`; `race_is_creation_eligible()` hard-codes those two | Generalise: a `conversion_only` flag in `race_data` and one shared transformation routine (level reset optional) that the quest code calls with a race number |

### Missing: code is needed

| Gap | Races | Duris mechanic | Suggested implementation |
|-----|-------|----------------|--------------------------|
| Giant wield | Ogre, Firbolg, Minotaur, Storm Giant | `IS_GIANT()` in `src/core/utils.h` makes every weapon one-handed (`wield_item_size()` in `src/cmd/actobj.c`); the 1.5x two-handed dice multiplier still applies, so a shield or second weapon stacks on top | New feat (`FEAT_GIANT_WIELD`, innate, 3 RP) checked in `hands_needed_full()` in `src/obj/act.item.c`. Decide where the weapon sits: if it goes in `WEAR_WIELD_1`, the two-handed strength bonus in `compute_damage_bonus()` (`src/combat/fight.c`, keyed to `WEAR_WIELD_2H`) needs a feat-aware clause so the Duris "full damage" rule holds |
| Four arms | Thri-Kreen | `HAS_FOUR_HANDS()`, `THIRD_WEAPON` and `FOURTH_WEAPON` equipment slots, extra attacks per round in `src/combat/fight.c`, doubled wrist, sleeve and glove slots | The largest gap. Needs two new wear positions, equip and remove handling, save-file support, attack routine extra swings, and doubled slots. The cheaper stand-in is a stackable racial extra-attack feat plus no extra slots; the study prices the full mechanic at 8 RP, so the stand-in should be priced lower |
| Melee output multiplier | Death Knight, Wight, Storm Giant, Orog (x2.0 to x2.7), Ogre, Firbolg, Troll | `damage.totalOutput.racial` and `damage.damrollModifier.racial` multiply all melee damage | No percentage melee trait exists; flat feats (Crystal Fist +3) are the nearest. Add a scaling racial damage feat (+1 per N levels, stackable) in `compute_damage_bonus()`, capped by composition rule 2 (no trait over 30 percent of budget) |
| Attack round speed | Gnome 11.5, Ogre 19, Wight and giants slow, elves fast (Human 15) | `damage.pulse.racial` shortens or lengthens the combat round | Rounds are fixed at `PULSE_VIOLENCE`. Express fast races as a racial extra attack at a level gate and slow races as a small attack penalty, or accept the loss; the study already folds pulse into its melee factor |
| Casting speed | Illithid 0.025, Kobold fast, Ogre and Firbolg 1.5 to 1.6 | `spellcast.pulse.racial` multiplies casting time | Casting time is set in `src/magic/spell_parser.c` (near lines 3173 and 3239) with only class reductions (`FEAT_WIZ_CHANT`, sorcerer quick chant). Add a racial casting-time modifier feat, stackable, plus or minus one round per rank, consumed there |
| Charge with stun and reach | Minotaur | `do_charge` in `src/cmd/actoff.c` accepts a direction, reaches one room away through `get_char_ranged()`, and stuns | Our general `charge` has neither. Add a feat (`FEAT_BULL_CHARGE`) that `do_charge` checks for the stun and the adjacent-room target; the movement and single-file checks already exist |
| Bloodlust rage below half hit points | Minotaur | `minotaur_race_proc()` in `src/combat/fight.c` applies `TAG_MINOTAUR_RAGE`: uncontrolled attacks, no casting | Drawback feat (-2 RP) checked once per combat round in `perform_violence()`: below 50 percent hit points apply a rage affect that blocks `cast` and forces the attack |
| Bonus damage vs smaller foes | Ogre | Priced by the study; the Duris hook is the size comparison in `src/combat/fight.c` | Small feat in `compute_damage_bonus()`: +2 when the victim is at least one size smaller. Cheap; do it with the giant wield feat |
| Innate power loss on daylight and cover for Duris undead | Vampire, Lich, Wight, Phantom | Fire vulnerability x1.3 is registered for these forms but commented out in Duris | `FEAT_WEAKNESS_TO_FIRE` is now feat-gated at -50; a milder -30 rank would need the feat made stackable or a second constant. Optional |

### Order of work

1. Experience factor and racial hit points as data (partial rows): small,
   unblock every later race definition.
2. Giant wield, bonus damage vs smaller, and the scaling melee damage feat:
   together they make Ogre, Firbolg, Minotaur, Storm Giant, Orog and the undead
   warriors expressible.
3. Racial casting-time feat and the spell-power decision: Illithid, Kobold and
   the giants' slow casting.
4. Class deny list per race and the generalised conversion routine: the
   descend forms and the two-class giants.
5. Charge stun and bloodlust: Minotaur alone.
6. Four arms: Thri-Kreen alone, and the only gap that touches the save format.
   Decide between the full mechanic and the extra-attack stand-in before
   starting.

## Part 2: the innate-feat work, as built

Every Duris player-race innate that LuminariMUD did not already cover is now a
feat registered with `feato()` as `in_game = TRUE`, `can_learn = FALSE`,
`FEAT_TYPE_INNATE_ABILITY`, gated on `HAS_FEAT()` and never on `GET_RACE()`.
No race was granted any new feat. The only `assign_races()` changes are the
two behaviour-preserving lines (`FEAT_STABILITY` to `RACE_CRYSTAL_DWARF`,
`FEAT_BODYSLAM` to `RACE_HALF_TROLL`) that keep those races as they were once
their race checks became feat checks. Assigning feats to races is the
separate next step once the gaps above are closed.

### Already covered by existing feats (no work)

Infravision, Ultravision, Levitate, Faerie Fire, Darkness and Globe of
Darkness, Enlarge, Innate Strength, Underdark Invisibility, Fly, Protection
From Fire and Cold, Magic Resistance by shrug band, Regeneration, Innate Hide,
Disappear, Shade Movement, Perception, Gambler's Luck, Vampiric Touch, Bite,
Gaze, Phantasmal Form, Call Of The Grave, Charge, and Dayblind map to feats
that already existed (`FEAT_INFRAVISION`, `FEAT_SLA_*`, `FEAT_WINGS`,
`FEAT_TROLL_REGENERATION`, `FEAT_NATURALLY_STEALTHY`, `FEAT_ONE_WITH_SHADOW`,
`FEAT_KEEN_SENSES`, `FEAT_LUCKY`, the vampire feats, `FEAT_SUMMON_UNDEAD`,
`FEAT_LIGHT_BLINDNESS`, and so on).

### Wired: feat existed, mechanic was race-keyed (Phase 1)

`FEAT_WEAKNESS_TO_FIRE` (-50) and `FEAT_VULNERABLE_TO_COLD` (-20) in
`compute_damtype_reduction()`; `FEAT_LEAP` dodge; `FEAT_COMBAT_TRAINING_VS_GIANTS`
+4 AC vs larger attackers; `FEAT_LEONINE_FRAME` refuses legs and feet in
`character_wear_slot_restriction()`; `FEAT_LICH_SPELL_RESIST` in
`compute_spell_res()`; `FEAT_STABILITY` in both `perform_knockdown()` checks;
`FEAT_KENDER_FEARLESSNESS` renamed "fearlessness"; `FEAT_HASTE` repurposed as
the 1/day "innate haste" (`battlehaste`); `FEAT_BODYSLAM` gates the bodyslam
skill.

### New feats (48)

| Group | Feats |
|-------|-------|
| Passive defence and resistance | `FEAT_SUN_VULNERABILITY`, `FEAT_DAYBLIND`, `FEAT_EYELESS`, `FEAT_MAGIC_VULNERABILITY`, `FEAT_MAGICAL_REDUCTION`, `FEAT_THICK_HIDE`, `FEAT_SACRILEGIOUS_POWER`, `FEAT_SPELL_ABSORB`, `FEAT_QUICK_THINKING`, `FEAT_GROUNDFIGHTING`, `FEAT_QUADRUPED_BODY`, `FEAT_WATER_BREATHING`, `FEAT_UNDEAD_FEALTY`, `FEAT_ONE_OF_US`, `FEAT_SOUL_OF_THE_FEY` |
| Passive offence | `FEAT_AXE_MASTERY`, `FEAT_HAMMER_MASTERY`, `FEAT_LONGSWORD_MASTERY`, `FEAT_GREATSWORD_MASTERY`, `FEAT_HATRED`, `FEAT_BLOODHUNT`, `FEAT_BATTLE_FRENZY`, `FEAT_WARCALLERS_FURY`, `FEAT_AUTHORITATIVE`, `FEAT_RRAKKMA` |
| Terrain and utility | `FEAT_OUTDOOR_STEALTH`, `FEAT_SWAMP_STEALTH`, `FEAT_UNDERDARK_STEALTH`, `FEAT_FOREST_SIGHT`, `FEAT_SEADOG`, `FEAT_MINER`, `FEAT_BARTER`, `FEAT_CALMING` |
| Spell-like abilities (SLA table, `do_racial_sla`) | `FEAT_SLA_FARSEE` (`farsee`), `FEAT_SLA_STONESKIN` (`stoneskin`), `FEAT_SLA_LIGHTNING_BOLT` (`throwlightning`), `FEAT_SLA_FIRE_SHIELD` (`fireshield`), `FEAT_SLA_FIRE_STORM` (`firestorm`), `FEAT_SLA_SHADOW_JUMP` (`shadowdoor`), `FEAT_SLA_PLANE_SHIFT` (`planeshift`), `FEAT_SLA_PSIONIC_BLAST` (`mindblast`), `FEAT_SLA_SCARE` (`roar`), `FEAT_HASTE` (`battlehaste`), `FEAT_SLA_FIREBALL` (`fireball`), `FEAT_SLA_MASS_DISPEL` (`massdispel`), `FEAT_SLA_FROST_BREATH` (`frostbreath`), `FEAT_SLA_WEB` (`webwrap`), `FEAT_SUMMON_WARG` (`summonwarg`), `FEAT_SUMMON_HORDE` (`summonhorde`) |
| Bespoke commands | `FEAT_BODYSLAM` (`bodyslam`), `FEAT_DOORBASH` (`doorbash`), `FEAT_STAMPEDE` (`stampede`), `FEAT_RACIAL_FLURRY` (`onslaught`; `flurry` is shadowed by the monk row) |

### Shared infrastructure

- Constants 1268 to 1315 in `src/structs.h`; 18 events in `src/mud_event.h`
  and `src/mud_event_list.c` (17 persisted daily-use cooldowns, `eSTAMPEDE` a
  three-round countdown); `get_daily_uses()` cases in `src/utils.c`.
- One table-driven SLA handler in `src/act.other.c` (`racial_sla_table[]`,
  `racial_sla_lookup()`, `do_racial_sla`, `SCMD_RSLA_*` in
  `src/interpreter.h`). It spends its action and daily use only when the
  ability committed: a `call_magic()` fizzle returns early, teleports must
  move the caster, summons must add a follower, and mass dispel applies
  `perform_dispel()` directly with `aoeOK()`, `CAN_SEE()` and `pvp_ok()`
  filtering so it never starts fights.
- Helpers in `src/utils.c`: `suffers_sun_vulnerability()`, `is_dayblinded()`,
  `sun_cover_protects()`, `char_is_blinded()`, `count_grouped_in_room()`,
  `racial_warcallers_fury_bonus()`, `racial_rrakkma_allies()`,
  `racial_quick_thinking_chance()`, `undead_fealty_protects()`,
  `calming_applies()`; `racial_terrain_ability_bonus()` in
  `src/character/abilities.c`; `shop_haggle_score()` in `src/obj/shop.c`;
  `vessel_pilot_speed_bonus()` in `src/vessels/vessels.c`.
- Summons: `ABILITY_SUMMON_WARG` and `ABILITY_SUMMON_HORDE` are `MAG_SUMMONS`
  rows; `PET_RACIAL_WARG` 19502 and `PET_RACIAL_ORC_WARRIOR` 19503 in
  `lib/world/mob/195.mob` (tracked in `data/pet-lycanthropes/`), follower
  categories `FOLLOWER_WARG` (one) and `FOLLOWER_ORC_HORDE` (four, admitted as
  a batch). Caution: the `// N` comments in the summon message tables in
  `src/magic/magic.c` skip 18, so from the dire wolf on each comment is one
  higher than the real index; the warg and horde rows are real 36 and 37.
- Help: 49 entries in `sql/components/help_duris_racial_innate_entries.sql`
  (listed in `Makefile.am`, applied to the development database) and
  `lib/text/help/help.hlp`; race-point rows in
  `docs/guides/PLAYER_RACES_REFERENCE.md`.
- Tests: `unittests/CuTest/test_racial_innate_feats.c` (38 tests) in both
  build lists; `scripts/world/wtool_constants.json` regenerated because
  `NUM_FEATS` moved (repeat `python3 scripts/world/wtool.py constants sync
  --write` whenever it moves again).

### Deviations from the Duris originals

Verbs renamed for prefix collisions (`onslaught`, `battlehaste`,
`shadowdoor`, `throwlightning`); giant training keeps the existing +4 AC;
frost breath uses cone of cold; shadow jump takes a target; innate haste
refuses while hasted from any source; Doorkick merged into `FEAT_DOORBASH`;
Dayvision, Summon Totem and Project Image dropped (no implementation in
Duris); the swamp boat waiver, ship sale bonus and potion-spill resistance
side effects dropped. `spell_shadow_jump` had its shadow predicate inverted
and lacked teleport's unique-mob guards; both fixed in the review pass.

### Follow-ups

- Any checkout that has not run them needs the help SQL component applied and
  `python3 scripts/world/install_pet_constructs.py` run so the warg and orc
  warrior prototypes exist.
- A help-sync run when the entries should reach production.
- Assigning feats to races per the study, and the `innates` style summary,
  wait on Part 1.
- The `Clean archive, both build systems` CI job fails on master with two
  tests unrelated to this work (`test_syntax_check_boot.c:526`,
  `test_database_persistence.c:807`).
