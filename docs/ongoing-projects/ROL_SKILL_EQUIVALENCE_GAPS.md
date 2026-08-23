# RoL Skills Without a Close LuminariMUD Equivalent

Status: re-verified against both source trees 2026-08-23. All five player-facing
gaps have since been implemented as class-neutral, learnable LuminariMUD feats;
see "Ported gaps" below.

This is the companion to `ROL_SPELL_EQUIVALENCE_GAPS.md`, alongside
`ROL_RACE_EQUIVALENCE_GAPS.md`. That document covers
Realms of Luminari's `SPELL_CREATE()` registry and its psionic `SKILL_CREATE()`
registrations. This document covers every unique remaining, non-psionic
`SKILL_CREATE()` registration.

A close equivalent must preserve the skill's primary gameplay purpose and broad
effect shape. LuminariMUD expresses many RoL skills as feats, abilities, or
plain commands rather than as `skillo()` entries, so all four namespaces count
as equivalents.

- RoL `SKILL_CREATE()` calls: 173 (153 live, 20 never compiled)
- Unique RoL registrations reviewed: 172 (152 live, 20 never compiled)
- Psionic discipline registrations (covered in the spell document): 40
- Non-psionic registrations reviewed here: 132 (119 live, 13 never compiled)
- Live functional registrations with a close LuminariMUD equivalent: 107
- Live registrations without a close equivalent: 5
- Live registry aliases or inert entries: 3
- Live internal state markers, not player skills: 4

## Extraction correctness

`sparser.c` cannot be read with a plain `grep`. Registrations are excluded from
the build in two ways, and both must be filtered out:

- 20 `SKILL_CREATE()` calls sit inside `#if 0` blocks.
- `SKILL_CREATE("feign death", SKILL_FEIGN_DEATH, ...)` is `//` commented out.
  RoL still has a live `SPELL_FEIGN_DEATH`, which is listed as a gap in the
  spell document; do not count it twice.
- `SKILL_SHADOW` is registered twice in live code. This produces 153 live calls
  but only 152 unique live skill constants.

Exact names were not accepted automatically. For example, LuminariMUD's
`chant` command prepares divine spells and its `unbind` command is staff-only;
neither command is the equivalent used for the same-name RoL skill.

## Player-facing or functional gaps

These five were the only live registrations without a close equivalent. All five
are now implemented as feats in `src/rol_feats.c`; the RoL behavior column below
records what was missing, and the ported-gaps table that follows records what
LuminariMUD now provides.


| RoL ID | RoL skill | RoL constant | Notes |
|-------:|-----------|--------------|-------|
| 102 | shadow | `SKILL_SHADOW` | Covertly follow a target between rooms, contested against the target's own shadow skill (`actnoff.c:5502`). LuminariMUD has no follow-unseen mechanic. Its `shadow *` names (shadow jump, shadow walk, one with shadow, shadow master) are all Shadowdancer teleport and concealment effects, not tailing. |
| 169 | calm | `SKILL_CALM` | The `chant calm` effect attempts to stop every fight in the room (`actunused.c:395`). LuminariMUD has no generally available skill, feat, class ability, command, or spell that pacifies a room. The Horn of Henekar has an artifact-only pacify invocation, which is not a player-system equivalent. |
| 252 | establish camp | `SKILL_CAMP` | Set up camp and rent out in the wilderness (`actnoff.c:119`). LuminariMUD has `rest` but no camp-as-rent-point mechanic and no `camp` command. A hint in `src/act.other.c` says players can pitch a camp, but no handler or command-table entry implements it. |
| 375 | garrote | `SKILL_GARROTE` | Strangling attack from concealment (`actoff.c:5987`). LuminariMUD has no garrote or strangle anywhere in `src/`; `sap` and `backstab` are separate mechanics that do not model strangulation. |
| 506 | accompany | `SKILL_ACCOMPANY` | A second grouped bard joins the lead bard's song, adds skill to its quality, and can take over if the lead fails (`newbard.c:159`, `newbard.c:2205`). LuminariMUD supports independent simultaneous performers and lets one bard maintain two songs through Master of Motifs, but has no join, quality-boost, or handoff mechanic between two bards. |

## Ported gaps

Each gap was re-expressed as a feat rather than as a `skillo()` percentage skill,
using d20 checks, ability scores, and the existing feat, daily-use, and
performance machinery. Behavior lives in `src/rol_feats.c`.

| RoL skill | LuminariMUD feat | Command | Mechanic | Availability |
|-----------|------------------|---------|----------|------------|
| shadow | `FEAT_SHADOW` | `shadow <target>` | Contested stealth against the mark's perception to take up the trail, re-rolled every time the mark leaves the room. Requires sneaking. Hooked into `perform_move_full()`. | Learnable by any class with 5 ranks of stealth |
| calm | `FEAT_CALM` | `calm` | Will save against 10 + half level + charisma bonus stops each fight in the room and clears NPC memory. Mind-affecting immunity ignores it. Limited daily uses through `eROL_CALM`. | Learnable by any class with charisma 13 |
| establish camp | `FEAT_ESTABLISH_CAMP` | `camp` | Survival check against a terrain and weather difficulty. The camp affect halves again the recovery of anyone in the group settled into it, through `hit_gain()` and `move_gain()`, and sets the campers' load room. | Learnable by any class with 3 ranks of survival |
| garrote | `FEAT_GARROTE` | `garrote <target>` | Attack from a hide-then-sneak posture against a target that cannot see you, needing a free hand. On a failed fortitude save against 10 + half level + dexterity bonus the target is silenced and staggered. Creatures that do not breathe are immune. | Learnable by any class with 8 ranks of stealth and BAB 4 |
| accompany | `FEAT_ACCOMPANY` | `accompany <performer>` | A grouped performer backs the lead's performance, adding a capped effectiveness bonus from perform and instrument, and takes the song over when the lead's verse fails or stutters. | Learnable by any class with 5 ranks of perform |

Supporting changes: `SKILL_SHADOW`, `SKILL_CALM`, `SKILL_CAMP`, `SKILL_GARROTE`
and `SKILL_ACCOMPANY` were added as affect and damage identifiers and registered
with `skillo()`; `SHADOWING()` and `ACCOMPANYING()` were added to
`char_special_data` with the same lifecycle cleanup as `GUARDING()`. Help is in
`lib/text/help/help.hlp` and `sql/components/help_rol_feat_entries.sql`, and
coverage is in `unittests/CuTest/test_rol_feats.c`. None of the five feats is
granted by or registered as a bonus feat for any class.

## Implementation checkpoint

The class-neutral conversion was validated on 2026-08-23. The production and
CuTest binaries compile without warnings, the full fixture-backed `make test`
run passes all 849 CuTest cases, and `make install` installs the tested server
and removes the root-level binary. The class-neutrality test verifies that all
five feats are learnable through the normal feat menu and absent from every
class feat-assignment list.

This fresh development worktree has no `lib/mysql_config` and no deployed world
files. The validation therefore used the checked-in special-procedure world
fixture and skipped only the database-dependent syntax boot. An unmodified
`make test` reaches all 849 cases but reports those two missing-runtime-data
failures; neither is related to the RoL feats. No production database was
accessed or modified.

## Internal state markers, not player skills

These occupy skill slots but are engine bookkeeping. They are listed for
registry completeness and need no port.

| RoL ID | RoL registration | RoL constant | Purpose |
|-------:|------------------|--------------|---------|
| 251 | constitution hits | `SKILL_CON_BONUS` | Stores accumulated CON hit-point bonus |
| 253 | fumbling weapon | `SKILL_DISARM_FUMBLING_WEAP` | Disarm state flag |
| 254 | dropped weapon | `SKILL_DISARM_DROPPED_WEAP` | Disarm state flag |
| 383 | engaged in pkill | `SKILL_PKILL_TIMEOUT` | PK engagement timer |

All four are registered with `SKILL_TYPE_NONE`. So is `establish camp`
(`SKILL_CAMP`, 252), but that one is a real player skill and is listed as a gap
above, so the `SKILL_TYPE_NONE` group holds five registrations in total.

## Registry aliases and inert registrations

These three live entries are not ordinary implemented skills. They need no
skill port, but they must not be counted as functional skill equivalents.

| RoL ID | RoL registration | RoL constant | Purpose |
|-------:|------------------|--------------|---------|
| 106 | surprise | `SKILL_SURPRISE` | Assigned to Ranger and Dire Raider in `sparser.c`, but the constant is never read outside the registry and header. The registration has no gameplay effect. |
| 197 | summon mount | `SKILL_SUMMON_MOUNT` | Has no `SKILL_ADD()` and the constant is never read. RoL's real features use `SPELL_SUMMON_MOUNT` and `INNATE_SUMMON_MOUNT`; LuminariMUD has summon-mount spell and class mechanics. |
| 398 | summon horde | `SKILL_SUMMON_HORDE` | Has no `SKILL_ADD()` and the constant is never read. RoL's live `horde` command checks `INNATE_SUMMON_HORDE`; LuminariMUD's summon-creature systems cover the broad summon role. |

`SKILL_INSTANT_KILL` also has no active `SKILL_ADD()`, but unlike the entries
above its handler is live and is checked after successful backstabs. It remains
in the functional comparison because legacy or staff-assigned skill values can
activate it.

## Skill registrations never compiled

These 13 non-psionic registrations exist only inside `#if 0` blocks in
`sparser.c`. The seven never-compiled psionic registrations are listed in the
spell document instead.

| RoL registration | RoL constant | Note |
|------------------|--------------|------|
| spores of soothing | `SPORE_SOOTHE` | Myconid class skills; block is headed `/* Myconid Skills */` |
| spores of hypnosis | `SPORE_HYPNOTIZE` | |
| spores of pacifying | `SPORE_PACIFY` | |
| spores of distress | `SPORE_DISTRESS` | |
| spores of pain | `SPORE_PAIN` | |
| spores of vigor | `SPORE_VIGOR` | |
| spores of melding | `SPORE_MELDING` | |
| spellcast fire | `SKILL_CAST_FIRE` | |
| spellcast cold | `SKILL_CAST_COLD` | |
| specialize fire | `SKILL_SPEC_FIRE` | |
| specialize cold | `SKILL_SPEC_COLD` | |
| specialize divination | `SKILL_SPEC_DIVINATION` | |
| mix | `SKILL_MIX` | `do_mix()` exists and is wired into `interp.c`, but the skill itself is never registered |

None of the seven `SPORE_*` constants has a `#define` anywhere in the RoL tree.
They exist only as identifiers inside the disabled block.

## System-model differences, not per-skill gaps

Four groups of RoL skills have no one-to-one LuminariMUD counterpart because
the two codebases model the same concept with different machinery. These are
not gaps to fill skill-by-skill; porting them means deciding whether to adopt
RoL's model at all.

**Weapon and combat progression (14 live skills).** RoL tracks a percentage per
weapon class: `1h bludgeon`, `1h misc`, `1h piercing`, `1h slashing`, `2h bludgeon`,
`2h misc`, `2h slashing`, `archery`, `range weapons`, `range specialist`,
`dual wield`, `unarmed damage`, plus `offense` and `defense` as generic
to-hit and AC skills. LuminariMUD uses d20 proficiency tiers
(`minimal`/`basic`/`advanced`/`master`/`exotic weapon prof`) plus feats
(`weapon focus`, `point blank shot`, `rapid shot`, `manyshot`,
`two weapon fighting`, `improved unarmed strike`), with to-hit and AC derived
from BAB and armor rather than from a skill.

**Spell school percentages (26 live skills).** RoL has 13 live
`spellcast <school>` registrations, 11 live `specialize <school>`
registrations, plus `clerical spell knowledge` and
`sorcerous spell knowledge`. LuminariMUD has no per-school proficiency
percentage; school affinity is expressed through the `spell focus` and
`greater spell focus` feats, class spell lists, caster level, and spellcasting
ability checks. The five fire, cold, and divination variants that are not live
appear in the never-compiled table above.

**Bardic performance (11 covered skills, 1 gap).** RoL splits bard ability into
song categories (`baneful songs`, `healing songs`, `magical songs`, `melee
songs`), six instrument proficiencies (`drums`, `flute`, `harp`, `horn`,
`lyre`, `mandolin`), `virtuoso`, and `accompany`. LuminariMUD replaces the
first eleven with one Perform ability, 13 performance feats, instrument
objects, and performance perks. Instrument subtype is mechanically relevant:
using the ideal instrument changes performance effectiveness, while item
values can reduce difficulty. LuminariMUD does not track a separate proficiency
percentage for each instrument. `Accompany` was the one functional gap and is now
the `FEAT_ACCOMPANY` feat.

**Chant (6 live registrations: 5 covered, 1 gap).** RoL's `chant` command and
its five selectable effects (`calm`, `regeneration`, `heroism`, `soul strike`,
and `death grip`) are separate skill constants implemented through
`do_chant()` in `actunused.c`. LuminariMUD covers heroism and regeneration
directly, while its divine area damage and monk attack mechanics cover the
broad roles of soul strike and death grip. `Calm` was a gap and is now the
`FEAT_CALM` feat. Despite the filename, `actunused.c` is compiled (`Makefile:67`) and `do_chant`,
`do_dragon_punch`, and `do_self_preservation` are wired into `interp.c`.

## Near-miss cases ruled covered

The table records representative non-exact cases among the covered functional
registrations. Exact name matches are omitted.

| RoL skill | LuminariMUD equivalent |
|-----------|------------------------|
| hide, sneak | `hide` and `sneak` commands; stealth ability; hide in plain sight |
| listen | `listen` command; perception ability |
| steal | `steal` command; sleight of hand ability |
| pick lock | disable device ability |
| detect trap, disarm trap, trap | trapfinding, trap sense, trapmaking; disable device |
| assassinate | death attack (assassin) |
| instant kill | death attack / quivering palm / arrow of death |
| circle | `circle` command (`src/combat/act.offensive.c:10655`) |
| escape | `flee`; nimble escape |
| unbind | free movement; slip the bonds |
| safe fall | Acrobatics and stability resist knockdown; slow fall covers actual falling damage |
| switch opponents | attacking a new target during combat switches opponents, with an Acrobatics check for attacks of opportunity (`src/combat/act.offensive.c:4845`) |
| springleap | spring leap |
| swimming | athletics ability |
| lore | monster lore; knowledge abilities |
| awareness | perception ability; uncanny dodge |
| berserk | rage |
| blindfighting | blind fighting |
| martial arts, unarmed damage | improved unarmed strike; monk unarmed progression |
| dragon punch | stunning fist |
| shieldblock | shield prof; shield specialist |
| shieldpunch | shield punch; improved shield punch |
| missile snare | deflect arrows |
| disarm | improved disarm; greater disarm |
| double attack | iterative attacks from BAB |
| dual wield | two weapon fighting line |
| bandage | `bandage` command; medicine ability |
| self preservation | wholeness of body; fast healing; lay on hands |
| meditate | `meditate` command |
| mount | `mount` command; mounted combat |
| summon totem | already ported as converted spec behavior in `src/spec/spec_rol_totem.c` |
| chant, heroism, regeneration, soul strike, death grip | spell preparation uses a different `chant` command, but the individual buff and damage roles exist through spells, bardic performance, channel energy, and monk attacks |

`src/spec/spec_rol_*.c` in LuminariMUD holds converted RoL area, mob, and
object behavior. `spec_rol_totem.c` is the only one of those files that
supplies an equivalent for a RoL player skill; the rest are zone content and
are not equivalence sources for this audit.

## Source authority

- RoL skill registration: `RealmsOfLuminari/src/sparser.c`, `SKILL_CREATE()`
- RoL skill IDs: `RealmsOfLuminari/src/spells.h`. Note that RoL `SKILL_*` and
  `SPELL_*` constants share and overlap the same numeric range (for example
  `SPELL_CASTER_WATER_EMBODIMENT` and `SKILL_ENHANCE_AGI` are both 292), so
  skill IDs must not be cross-referenced against the spell IDs in the spell
  document.
- RoL skill behavior: `actoff.c`, `actnoff.c`, `actunused.c`, `bard.c`,
  `newbard.c`, `missile.c`, `traps.c`, and the `interp.c` command table
- LuminariMUD skills: `src/magic/spell_parser.c`, `skillo()`
- LuminariMUD feats: `src/character/feats.c`, `feato()`
- LuminariMUD abilities: `src/constants.c`, `ability_names[]`
- LuminariMUD commands: `src/interpreter.c`, `cmd_info[]`
- LuminariMUD bard mechanics: `src/bardic_performance.c` and bard perks in
  `src/character/perks.c`
- Converted RoL content: `src/spec/spec_rol_*.c`

## Verification method

1. `sparser.c` was preprocessed using RoL's checked-in `config.h`, preserving
   active `NEW_BARD` code while removing every `#if 0` branch.
2. Comments were removed before parsing calls. This excludes commented
   `SKILL_FEIGN_DEATH` and exposes the duplicate live `SKILL_SHADOW` call.
3. Each live non-psionic constant was traced outside its registration and
   header to distinguish implemented behavior, bookkeeping, registry aliases,
   and inert entries.
4. Non-exact matches were checked against LuminariMUD's spell, psionic, skill,
   feat, ability, command, bardic-performance, perk, and converted-RoL
   implementations.
