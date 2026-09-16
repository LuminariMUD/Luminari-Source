# Oversized translation units: metrics, ranking, and the first decomposition wave

This report answers two questions for LuminariMUD's C sources: which translation units are
genuinely expensive to change, and what the first wave of decomposition did about the top of that
list. It exists because a line count on its own is a poor proxy for risk, and the tree contains
both kinds of long file: hand-maintained code with mixed responsibilities, and large tables that
are long because the data is long.

Tracking issue: [#96](https://github.com/LuminariMUD/Luminari-Source/issues/96).

## Reproducing the numbers

```bash
scripts/development/module_metrics.py            # top 40, ranked
scripts/development/module_metrics.py --top 0    # every translation unit
```

The tool reads the working tree and `git log`, so it needs no build. It takes about a minute on a
full checkout.

## What is measured, and why

A long file is only a problem when length combines with something else. The tool keeps the
contributing factors separate rather than collapsing them into a size budget.

| Metric | Meaning | Why it matters |
| -- | -- | -- |
| `lines` | Physical lines | The headline number, and the least informative one |
| `code` | Lines that are neither blank nor comment | Comments and string bodies are excluded so prose does not inflate the total |
| `Table%` | Share of code inside a file-scope initialiser | Separates a long table from long logic |
| `churn` | Commits touching the file in the last 18 months | A file nobody edits costs nothing to leave alone |
| `fixes` | Commits in that window whose subject reads as a bug or security fix | Defect density taken from real history rather than assigned by hand |
| `fan_in` | Files including this file's companion header | How far a change to its interface can reach |
| `fan_out` | Distinct headers this file pulls in | How much of the tree can force it to rebuild |
| `globals` | File-scope definitions with external linkage | The symbol surface other modules can bind to |

Churn follows renames, so the recent move to `src/<subsystem>/` does not read as "every file was
rewritten last month". Each metric is normalised against the maximum across the scanned set, so
the score answers "how extreme is this file for this tree" rather than comparing against an
absolute budget. Table data is excluded from the size term, so a large table cannot push a file up
the ranking on its own.

The tool deliberately does not measure compile time. Doing so would mean invoking the compiler
several hundred times with build-specific flags, which makes the tool slow and environment
dependent. Compile cost is measured directly for the files a wave actually touches; see
[Measured effect](#measured-effect).

## Table-heavy exceptions

These files are long because their content is long. They are not decomposition candidates, and
splitting them mechanically would make them harder to read, not easier. The correct treatment is
to keep the data in one place and keep algorithms out of it.

| File | Lines | Table% | What the table is |
| -- | -: | -: | -- |
| `src/core/interpreter.c` | 10534 | 65% | `cmd_info[]`, the master command table |
| `src/core/constants.c` | 5014 | 95% | Name and flag tables shared across the server |
| `src/spec/spec_registry.c` | 2764 | 70% | Special-procedure registry entries |
| `src/events/mud_event_list.c` | 545 | 97% | Event type descriptors |
| `src/character/character_creation_content.c` | 988 | 83% | Creation menu copy |
| `src/mob/random_names.c` | 502 | 100% | Name fragments |

`src/character/perk_definitions.c`, created by this wave, belongs in this category but the tool
does not label it. `Table%` detects brace initialisers, and the perk tables are written as runs of
field assignments into `perk_list[]` instead, so the metric reads 0% for a file that is 10,065
lines of pure data. That is a known limit of the heuristic rather than a judgement about the file;
treat it as a table-heavy exception when reading the ranking.

Note that `src/core/interpreter.c` combines a large table with the highest churn in the tree. The
table is not the problem there; the parser and dispatch logic sharing the file with it is a
candidate for a later wave.

## Ranking before this wave

Generated at `a71165e2`, the commit this work branched from. This is the ranking that selected the
first wave's targets.

| # | File | Lines | Code | Table% | Churn | Fixes | Fan-in | Fan-out | Globals | Score |
| -- | -- | -: | -: | -: | -: | -: | -: | -: | -: | -: |
| 1 | `src/core/db.c` | 9074 | 7307 | 0% | 284 | 68 | 325 | 95 | 108 | 71.3 |
| 2 | `src/core/utils.c` | 12900 | 10166 | 0% | 201 | 48 | 403 | 41 | 377 | 65.5 |
| 3 | `src/combat/fight.c` | 18153 | 14286 | 0% | 301 | 45 | 114 | 61 | 87 | 63.3 |
| 4 | `src/core/interpreter.c` _(table)_ | 10534 | 9616 | 65% | 339 | 55 | 267 | 89 | 34 | 62.3 |
| 5 | `src/character/perks.c` | 23614 | 16302 | 0% | 212 | 16 | 50 | 21 | 797 | 55.7 |
| 6 | `src/core/comm.c` | 6409 | 4952 | 2% | 179 | 31 | 316 | 71 | 68 | 48.3 |
| 7 | `src/magic/magic.c` | 15860 | 13300 | 1% | 210 | 44 | 0 | 42 | 50 | 48.2 |
| 8 | `src/act/act.wizard.c` | 13575 | 11030 | 2% | 197 | 41 | 0 | 70 | 18 | 46.2 |
| 9 | `src/act/act.informative.c` | 11597 | 9349 | 2% | 222 | 48 | 0 | 56 | 23 | 45.8 |
| 10 | `src/act/act.other.c` | 14192 | 11055 | 1% | 161 | 30 | 0 | 58 | 25 | 40.0 |
| 11 | `src/combat/act.offensive.c` | 16776 | 13124 | 0% | 169 | 16 | 0 | 34 | 48 | 37.6 |
| 12 | `src/core/handler.c` | 4335 | 3319 | 0% | 133 | 31 | 240 | 44 | 79 | 37.0 |
| 13 | `src/craft/crafting_new.c` | 11293 | 9504 | 0% | 116 | 30 | 24 | 36 | 151 | 35.2 |
| 14 | `src/player/players.c` | 8171 | 7002 | 0% | 180 | 32 | 0 | 41 | 47 | 34.7 |
| 15 | `src/core/constants.c` _(table)_ | 5014 | 4226 | 95% | 213 | 32 | 159 | 16 | 199 | 33.3 |

Two entries stand out beyond their rank. `src/core/utils.c` exports 377 symbols and is included
by 403 files: it is the widest interface in the tree, and its score understates it because its
churn is spread thinly. `src/character/perks.c` exported **797** symbols, almost all of them
one-line `has_*` and `get_*` accessors that were never marked static.

## Why these three seams first

The first wave took the highest-ranked candidates where a boundary already existed in the code and
could be moved without touching behaviour. Ranks 1, 2, 4 and 6 (`db.c`, `utils.c`,
`interpreter.c`, `comm.c`) were deliberately left alone: their fan-in is 240 to 403 files, so any
interface change there needs its own wave and its own review.

| Seam | From | To | Kind |
| -- | -- | -- | -- |
| Perk tables vs perk engine | `src/character/perks.c` (rank 5) | `src/character/perk_definitions.{c,h}` | Generated data vs algorithms |
| Combat presentation vs resolution | `src/combat/fight.c` (rank 3) | `src/combat/combat_messages.{c,h}` | Resolution vs presentation |
| The staff `set` command family | `src/act/act.wizard.c` (rank 8) | `src/act/act.wizard.set.c` | Command family, parsing vs execution |

### Perk tables

`perks.c` held fifteen `define_*_perks()` functions totalling 9,994 lines. They contain no
algorithm: each is a run of field assignments into `perk_list[]`. They are now a table file, and
`perks.c` keeps the engine: purchase rules, rank lookups, stage advancement, and the `perk`
command.

`perk_definitions.h` is the seam. It carries the fifteen declarations and the `perk_list[]`
`extern`, and documents the rules the tables must follow: write only into slots `init_perks()` has
reset, store `strdup()` copies in the three string fields because `destroy_perks()` frees them,
never free what is already in a slot, and run only from `init_perks()`, which owns the ordering.
The thirteen `define_*` declarations left `perks.h`, which no longer advertises them to gameplay
code. One of them, `define_druid_perks()`, was declared but never defined anywhere; it was
dropped rather than carried across.

### Combat presentation

`fight.c` mixed the text a room reads with the arithmetic that decides a fight. The messaging
cluster - `replace_string()`, `dam_message()`, `skill_message_with_projectile()` and the
`skill_message()` wrapper, 668 contiguous lines - is now its own module. Resolution calls
presentation and never the reverse, so retuning combat text cannot change a fight.

`skill_message()` had no caller outside `fight.c`, so its declaration left the widely included
`fight.h` and now lives only in the internal `combat_messages.h`. That is a net reduction in the
public surface, not a lateral move. `replace_string()` stays file-scope; only the two entry points
`fight.c` actually calls are declared.

One subtlety is documented rather than changed: the skill-message path reports defensive reactions
to the special-procedure gateway, because a parry or dodge becomes observable at the moment it is
described. `combat_messages.h` records that this is a notification and does not feed back into the
resolved outcome.

`get_wielded()` gained a real declaration in `fight.h`. It already had external linkage but no
prototype outside `fight.c`, so the definition was unchecked against its callers; it is now
declared once and checked.

### The staff `set` family

`do_set`, `perform_set`, `find_set_field`, `show_set_help` and the 117-entry `set_fields[]` table
formed a self-contained 1,154-line block. Moving it needed no new header at all: `do_set` is
reached through the command table and the existing test hook is already declared in `act/act.h`.

`set_fields[]` had external linkage with no external readers, so the move made it `static`,
deleting a global outright. Every other helper in the family is file-scope.

## Measured effect

Compile cost was measured with the project's own flags, taking the best of seven round-robin runs
so that all files saw the same machine conditions. The figure is CPU time, which is far less
sensitive to competing load than wall clock.

| Editing this responsibility | Rebuilds | Before | After | Change |
| -- | -- | -: | -: | -: |
| A class's perk table | `perk_definitions.c` | 3.99s | 1.53s | -62% |
| The perk engine | `perks.c` | 3.99s | 1.99s | -50% |
| Combat text | `combat_messages.c` | 3.00s | 0.14s | -95% |
| Combat resolution | `fight.c` | 3.00s | 2.84s | -5% |
| The `set` command | `act.wizard.set.c` | 2.63s | 0.27s | -90% |
| Other staff commands | `act.wizard.c` | 2.63s | 2.34s | -11% |

Header fan-out for the extracted responsibility, which is deterministic and load independent:

| Responsibility | Headers before | Headers after | Change |
| -- | -: | -: | -: |
| Perk tables | 53 | 30 | -43% |
| Combat text | 98 | 33 | -66% |
| The `set` command | 106 | 65 | -39% |

Full-build cost is essentially unchanged: the same code is compiled, just distributed differently
(`perks.c` + `perk_definitions.c` = 3.52s against 3.99s before, the other two pairs flat). The
gain is in incremental work and in blast radius. Editing combat wording no longer recompiles
17,000 lines of combat mathematics, and a change to any of the 65 headers that the `set` family
does not use no longer rebuilds it.

Symbol surface also moved in the right direction: `perks.c` dropped from 797 exported symbols to
783 as the table functions left, and `set_fields[]` stopped being a global.

## Defects these tests surfaced

Characterisation tests written against the perk tables found seven pre-existing content defects.
None were introduced by this wave and none are fixed by it: correcting any of them changes what
players can see or buy, which is a content decision that belongs in its own change with its own
review. They are recorded as ratchets in `unittests/CuTest/test_perk_definitions.c`, so the tables
cannot get worse and can be fixed freely, and are tracked in
[#194](https://github.com/LuminariMUD/Luminari-Source/issues/194).

- **Permanently unbuyable perk.** `PERK_WIZARD_EXTENDED_SPELL_3` requires
  `PERK_WIZARD_EXTENDED_SPELL_2`, which has an id in `structs.h` but is never defined by any tree.
- **Three unbuyable cleric capstones.** `PERK_CLERIC_DOMAIN_FOCUS_3`,
  `PERK_CLERIC_DIVINE_SPELL_POWER_3` and `PERK_CLERIC_GREATER_TURNING` each require rank 5 of a
  prerequisite whose `max_rank` is 2 or 3.
- **Three uncategorised barbarian perks.** `PERK_BARBARIAN_RAGE_ENHANCEMENT`,
  `PERK_BARBARIAN_EXTENDED_RAGE_1` and `PERK_BARBARIAN_TOUGHNESS` never set `perk_category`, so
  they keep `PERK_CATEGORY_UNDEFINED` and appear under no tree.

Writing those up also turned over a larger defect the tests themselves cannot catch:
`perk_category_names[]` has drifted out of sync with the `PERK_CATEGORY_*` constants, so 320 of the
562 defined perks display the wrong tree name and roughly 250 render as `Unknown Category`. It is
tracked in [#195](https://github.com/LuminariMUD/Luminari-Source/issues/195), and it needs fixing
before the barbarian perks above can be given a category.

All eight were fixed together afterwards, and the ratchets became strict assertions. Tracing the
fixes corrected two of the descriptions above:

- `can_purchase_perk()` skips a prerequisite it cannot resolve, so Extended Spell III could be
  bought with no prerequisite at all, not never.
- `list_perks_for_class()` stopped at the same mid-enum bound as the name lookup, so the paladin,
  bard, alchemist, psionicist, blackguard and inquisitor trees were missing from `perk` entirely.
  The table, the lookup and the loop now share `NUM_PERK_CATEGORIES`, and a static assertion
  checks the table's length.

The design sheets in `docs/systems/perks/` settled the content choices. Extended Spell II is defined
as `WIZARD_PERKS.md` describes it, and each cleric tier 3 perk requires its tier 2 prerequisite at
that perk's maximum rank. The barbarian perks are back in `PERK_CATEGORY_BERSERKER`, where they sat
until commit `4617c772a` rolled `perks.c` back to an older copy.

## Defects review found in the moved combat code

Once `combat_messages.h` became the contract callers read instead of `fight.c`, review of PR #193
found five places where the code moved verbatim did not do what that header said, or had
preconditions nothing stated. All five predate this wave. Unlike the perk content above they are
code defects, and they were fixed in the same PR, in a commit separate from the pure move so that
the move stays mechanically verifiable.

- **Object leak.** `skill_message_with_projectile()` loaded a stand-in claw object for every Trelux
  combat message and never extracted it. Each one stayed in `object_list` and in the
  `obj_index[].number` count for the life of the boot. Every path out now goes through one exit
  that extracts it.
- **Unbounded attack-type index.** `dam_message()` indexes `attack_hit_text[]` by `w_type` with no
  bound. The only guard was the `IS_WEAPON()` check at its one caller, and that macro was private
  to `fight.c`. It now lives in `structs.h`, `dam_message()` refuses an out-of-range type with a
  `SYSERR`, and `replace_string()` bounds both copy loops against its buffer.
- **Silent killing blow.** The header said `dam_message()` always produces output, but it renders
  nothing for a dead victim. `damage_with_projectile()` fell back to it for a killing blow with no
  authored message, so that blow went undescribed with nothing logged. The header now says so, and
  that path logs a `SYSERR` naming the attack type. Every weapon type has a message block today,
  so this is a guard, not a visible change.
- **Armour check on non-armour.** The glance-off-armour miss indexed `armor_list[]` with `value[1]`
  of whatever was worn on the body. It now uses the guarded `GET_ARMOR_TYPE_PROF()`. This is the
  one fix players can see: four crests that mobs in zone 20202 wear on the body, and Graye's Staff
  (#31020), are wands or staves whose charge count happened to select an armour row. Misses against
  their wearers could read "glances off" the crest; they now get the ordinary miss line.
- **Test fixture teardown.** `test_combat_messages.c` asserted before tearing its fixture down, and
  CuTest `longjmp()`s out of a failed assertion. A single failure would have left `world` pointing
  into a dead stack frame for every later test. Each test now tears down before it asserts.

## Where the tests live

| Module | Tests |
| -- | -- |
| `perk_definitions.c` | `unittests/CuTest/test_perk_definitions.c` - table completeness, prerequisite resolution, string ownership across re-init, sentinel handling for unclaimed slots |
| `combat_messages.c` | `unittests/CuTest/test_combat_messages.c` - the fallback protocol between `skill_message()` and `dam_message()`, weapon token substitution, damage-fraction tiering, the attack-type bound, silence for a dead victim, and release of the Trelux stand-in object |
| `act.wizard.set.c` | `unittests/CuTest/test_wizard_set.c` - field resolution, sentinel termination, the hardcoded `SET_NAME_FIELD` index, and the staff level gate on every row |

`unittests/CuTest/test_spec_combat_secondary.c` reads combat source by region to assert that
defensive reactions route through the gateway. Its anchors were retargeted from `fight.c` to
`combat_messages.c` along with the code, so the assertions still bind to the statements they were
written for.

## Suggested next wave

Ordered by what the ranking supports, not by size.

1. **`src/core/utils.c` (rank 2).** 377 exported symbols across 403 includers. The highest-value
   work is not a split but an audit: establish which symbols have external callers and make the
   rest static. That shrinks the interface without moving a line of logic.
2. **`src/character/perks.c` (rank 5, still 783 globals).** The hundreds of one-line `has_*` and
   `get_*` accessors are per-class and mechanical. Grouping them by class alongside a static
   default would cut the symbol surface sharply.
3. **`src/core/interpreter.c` (rank 4).** Separate the command table from the parser and dispatch
   logic that currently shares the file with it. Highest churn in the tree, so the payoff per
   change is large, but its 267 includers mean the interface must not move.
4. **`src/act/act.wizard.c` (rank 8).** Three further self-contained families are visible in the
   same shape as the `set` extraction: copyover (`validate_copyover_environment`,
   `perform_do_copyover`, `do_copyover`), the test-character provisioning pair (`do_settestchar`,
   `do_settestkit`, roughly 760 contiguous lines), and the `last`/login-log family.

Rank 1, `src/core/db.c`, is the highest-scoring file in the tree and the one to approach last: 325
includers, 95 headers of its own, and the world loader at its centre.
