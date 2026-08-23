# Adding a New Player Class to LuminariMUD

Status: source-backed developer guide, verified 2026-08-23.

This guide is an integration checklist for base and prestige classes. It focuses on the places a
working class must reach without documenting every class-related subsystem in the game. NPC-only
class design is out of scope, but a new class ID still needs an explicit mobile/OLC policy.

## Completion contract

A class is ready when:

- its numeric ID is permanent, unused for another identity, and covered by every class bound;
- `load_class_list()` creates a complete registry entry and every positional table has the entry at
  the same index;
- progression, feats, perks, stat caps, commands, equipment, reset, and persistence match the
  design;
- every applicable entry path lists, parses, validates, and explains the class;
- caster mechanics work end to end, or every caster classifier rejects the class;
- help exists in both maintained stores; and
- production-linked tests, both builds, and the applicable player flows pass.

Additional requirements depend on the class:

| Class type | Required path |
|------------|---------------|
| Selectable base | Terminal/web creation, exact help, gear, custom build, and premade policy |
| Prestige | Rejected by creation/respec; available through qualified `gain` |
| Account-locked | `accexp class`, SQL reload, and consistent access checks |
| Caster/manifester | Complete casting model and state round trip |
| NPC-enabled | Mobile OLC, serialization, autoroll category, and relevant AI/content behavior |

## 1. Define the class before editing

Record the following decisions. They determine which later sections apply.

| Area | Decision |
|------|----------|
| Identity | `CLASS_*` symbol, permanent ID, registry name, aliases, and help/media slug |
| Availability | Base or prestige, `in_game`, account lock, unlock cost, and maximum class level |
| Progression | BAB, hit die, movement, trains, saves, and epic class-feat interval |
| Abilities | Class (`CA`), cross-class (`CC`), or unavailable (`NA`) |
| Features | Automatic/selectable feats, level grants, perks, and stat-cap category |
| Casting | None, prepared, spontaneous, extract, at-will, psionic, or prestige advancement |
| Restrictions | Race, alignment, prerequisites, and incompatible multiclass pairs |
| Creation | Starting equipment and premade support policy |
| Content | Commands, guilds, trainers, tutorials, quests, items, and mobile behavior |
| Compatibility | Existing pfiles, SQL rows, world data, saved objects, and rollback |

`prestige_class` and `locked_class` are independent. A base or prestige class may be locked; do not
use either flag as a substitute for the other.

Choose the nearest existing class separately for lifecycle, progression, casting, feats, perks,
and NPC behavior. Do not copy one class wholesale. Trace every use of the references and inspect
exhaustive class switches:

```bash
rg -n '\bCLASS_(WIZARD|SORCERER)\b' src unittests --glob '*.[ch]'
rg -uu -n '\bCLASS_(WIZARD|SORCERER)\b' lib/world
rg -n 'case[[:space:]]+CLASS_|switch[[:space:]]*\([^)]*(GET_CLASS|class)' \
  src --glob '*.[ch]'
```

Replace the example classes with the closest references for the new design. For each hit, decide
whether the new class needs the same branch, another branch, or the generic fallback.

## 2. Allocate a durable numeric ID

Class IDs are persisted integers, not reorderable enum positions. Never renumber or reuse a
released ID or compatibility alias.

The current source allocation is:

- real classes `0` through `35` (`CLASS_WIZARD` through `CLASS_ARTIFICER`);
- placeholders `36` and `37`;
- `NUM_CLASSES == MAX_CLASSES == 38`; and
- `NUM_CASTERS == 9`, which is a legacy spell-storage width, not the class count.

Do not assume the placeholders are free because their registry entries are disabled. This
development checkout contains persisted uses of IDs 36 and 37. Replacing either placeholder is a
data migration. Appending ID 38 is the likely source change, but it is safe only after auditing 38
in every target environment; an old invalid value can become valid when the bound grows.

Audit at least these stores:

- Player files: `Clas`, `CLvl`, `Cfpt`, `Ecfp`, `PPts`, `Perk`, `PreB`, `PCAr`, `PCDi`, and
  class-keyed spell sections.
- SQL: `unlocked_classes.class_id`, deployed `player_levelups.class_number`, and serialized
  player, pet, and house objects.
- World/content: mobile `Class:`, HLQ class kits/prerequisites, DG class values, and
  class-specific spell-circle object affects.
- Operations: imports, staged data, archives, and restorable backups.

Use `-uu` for ignored deployment data. Raw numeric matches are candidates; confirm the record
format before treating them as class IDs.

```bash
rg -n '^#define (CLASS_|NUM_CLASSES|MAX_CLASSES|NUM_CASTERS)' src/structs.h
rg -n '\b(NUM_CLASSES|MAX_CLASSES|NUM_CASTERS)\b' src unittests \
  Makefile.am CMakeLists.txt
rg -uu -n '^(Clas|PreB|PCAr|PCDi):[[:space:]]+(36|37|38)$' lib/plrfiles
rg -uu -n '^Class:[[:space:]]+(36|37|38)$' lib/world/mob --glob '*.mob'
rg -uu -n 'has_class\(|%actor\.class%|set .*class' lib/world/trg --glob '*.trg'
```

Run read-only queries against every target database:

```sql
SELECT class_id, COUNT(*)
FROM unlocked_classes
WHERE class_id IN (36, 37, 38)
GROUP BY class_id;

-- Run only where this legacy table exists.
SELECT class_number, COUNT(*)
FROM player_levelups
WHERE class_number IN (36, 37, 38)
GROUP BY class_number;
```

Never expose database credentials or player-identifying audit results.

When appending a class:

1. Add the new `CLASS_*` value in `src/structs.h` without shifting existing values.
2. Raise both `NUM_CLASSES` and `MAX_CLASSES`; the code uses both for class-keyed arrays and loops.
3. Inspect every bound use and fixed-size initializer.
4. Add range checks before indexing class tables from persisted or user-supplied numbers.

`char_player_data.chclass` and the preferred caster fields use the signed `byte` type, so the
current representation cannot safely hold an ID above 127. Lower limits also exist in bitmasks.
The legacy `who`/`users` class filters use `int` shifts and already cannot safely represent an
appended ID 38; fix or replace that representation before exposing the new class there. Converted
RoL guild masks use 64-bit shifts and need redesign before ID 64.

Do not increase `NUM_CASTERS` merely because the class casts. New work should use the class-keyed
spell structures unless a separate migration of the legacy fixed-column format is intended.

## 3. Add the core registry and tables

Update the exact numeric position in each applicable location:

| Location | Required change |
|----------|-----------------|
| `src/structs.h` | Permanent `CLASS_*` ID and class bounds |
| `src/constants.c` | Names, description, preparation, and consign entries |
| `src/character/class.c` | Complete `load_class_list()` registration |
| `src/net/onboarding.c` | Media key or intentional fallback |
| `src/magic/casting_visuals.c` | Casting style for a caster or explicit `NULL` for a noncaster |

The name and short-description tables include sentinels and are checked against
`NUM_CLASSES + 1`. The spell word tables are checked against `NUM_CLASSES`. Explicitly sized media
and casting-style arrays can silently zero-fill a missing final initializer, so add value tests for
the new index.

`load_class_list()` is the runtime authority. Keep `in_game = N` until the class is complete. A
minimal registry block has this shape:

```c
classo(CLASS_NEW_CLASS, "new class", "New", "\tcNew\tn", "?) \tcNew Class\tn",
       max_level, locked, prestige, bab, hit_die, psp, move, trains, in_game,
       unlock_cost, epic_feat_interval, "spell progression", "primary attributes",
       "Player-facing description.");

assign_class_saves(CLASS_NEW_CLASS, fort, reflex, will, poison, death);
assign_class_abils(CLASS_NEW_CLASS, /* copy the formal argument order */);
assign_class_titles(CLASS_NEW_CLASS, /* eleven title strings */);
```

Read the helper definitions before filling positional arguments:

- `assign_class_saves()` requires all five save categories.
- `assign_class_abils()` has unused parameters, hardcoded abilities, and aliased ability IDs. Test
  the resulting `CLSLIST_ABIL()` values instead of trusting a copied call.
- `assign_class_titles()` requires ten level bands plus the default title.
- `feat_assignment(..., level, ...)` grants a feat at that class level;
  `NOASSIGN_FEAT` makes it an eligible class-feat choice instead.
- `spell_assignment()` records spell access; it does not implement slots, study, or casting.
- `class_prereq_*()` entries define `gain` eligibility for prestige/restricted classes.

The `classo()` PSP field is not the current Psionicist advancement implementation, and the prestige
spell-progression string is display text only. Implement those mechanics in their actual
consumers.

Add canonical and intended alias forms to `parse_class_long()`. Registry-name matching also occurs
through `get_class_by_name()` and account-specific parsing, so a parser entry alone is insufficient,
especially for multiword names. Reject ambiguous abbreviations rather than silently resolving to
the first earlier class.

## 4. Wire progression and class mechanics

Trace each relevant integration point:

| Integration point | Required work |
|-------------------|---------------|
| `level_exp()` | Add the class; omission logs a `SYSERR` and returns `123456` |
| `advance_level()` | Verify HP, movement, trains, fixed BAB, feat points, and special branches |
| `init_class()` | Reconcile first-class and login-time access idempotently |
| `init_start_char()` / `do_start()` | Clear and rebuild class-owned state |
| `feat_assignment()` | Register and test automatic/selectable feats |
| `class_to_perk_class()` in `src/character/perks.c` | Route stage awards to usable perk trees |
| `compute_char_cap()` | Select an intentional stat-cap category |
| `newbieEquipment()` | Add every creation-selectable base class; use named VNUMs |
| Proficiency and item checks | Implement the designed equipment policy |
| `src/act.wizard.c` | Add the class-level set field and update its gameplay test mapping |

`GET_CLASS()` is the current or most recently gained class, not a permanent primary-class identity.
The durable multiclass composition is `CLASS_LEVEL(ch, class)`. Test both multiclass directions and
the level-20 fixed-BAB boundary.

`init_class()` is called on first acquisition and again for owned classes during login. One-time
rewards must not be granted there without a persisted guard. For every new resource, choice,
cooldown, companion, or event, cover its complete lifecycle: defaults, gain, use, rest/death,
respec/reset, event cancellation, save/load, and old-pfile defaults.

If the class adds commands, register the declaration, `cmd_info[]` entry in `src/interpreter.c`,
implementation, permissions, help, and behavior tests. Audit guilds, trainers, class predicates,
combat/equipment branches, score presentation, and the nearest reference-class content hits.

New feat IDs are permanent values in `src/structs.h`; register them with `feato()` in
`src/character/feats.c`. Spells and skill-spells share the number space in
`src/magic/spells.h`; register them in `mag_assign_spells()` and implement them in the owning
subsystem. Never recycle released IDs.

If work adds or removes a C source file, update both `Makefile.am` and `CMakeLists.txt`. Do not edit
`src/campaign.h`, `src/mud_options.h`, or `src/vnums.h`; change the corresponding example template
when a shared configuration symbol is required.

## 5. Wire every player entry path

### Base-class creation

Terminal creation in `nanny()` and web creation should use one bounded eligibility decision:

```text
valid class ID
and in_game
and not prestige
and unlocked when locked
and race/alignment compatible
and any explicit race whitelist satisfied
```

Apply it to the initial menu, direct `CON_QCLASS` input, help-decline redisplay, and the web catalog
and selection handler. Current terminal direct input and help redisplay do not both enforce
`in_game`; fix that gap before registering the new parser name. Add the exact `class-<slug>` help
dispatch and a `newbieEquipment()` case.

`valid_align_by_class()` controls starting alignment. A `class_prereq_align()` entry controls later
availability; restricted classes normally need both. `valid_class_race_alignment()` only checks
whether the race and class share a legal alignment, so a class-specific race whitelist needs its
own creation check.

### `gain` and prestige classes

`do_gain()` in `src/act.other.c` resolves the class, calls `class_is_available()`, enforces the
class cap and `MULTICAP`, handles explicit incompatible pairs, and advances the selected class.
Verify:

- first-class and permitted multiclass acquisition;
- both directions of each incompatible pair;
- class-cap and multiclass-cap boundaries; and
- every prerequisite failing individually and the fully qualified path.

Race prerequisite rows form one OR group, as do alignment rows. Other prerequisite rows are ANDed.
Keep `class prereqs` output consistent with the actual evaluator. A prestige class must be rejected
by creation and respec and acquired only through this path.

### Respec, names, and premade builds

`do_respec()` currently rejects `CLSLIST_LOCK()` while describing it as prestige. Correct
integration must reject `CLSLIST_PRESTIGE()` and let account ownership decide locked base-class
access. Respec lists must also omit prestige classes. Evaluate eligibility for the post-reset
level-1 character; do not let class levels, feats, or casting state that `do_start()` removes
authorize the target.

Creation/class commands use `parse_class_long()`, while `gain` and `respec` use
`get_class_by_name()`, and `accexp class` has another scan. Multiword names need intentional tests
on every applicable surface; current respec tokenization only passes the first word as the class.

Every selectable base class needs one of two complete policies:

1. implement its premade build through all supported levels, including stats, abilities, feats,
   spells, human adjustments, and clearing at the earlier of level 20 or the class cap; or
2. hide and reject premade on terminal creation, web creation, and respec before any state or
   progression points are cleared.

### Account unlock and web onboarding

For `locked_class = Y`, verify `accexp class` listing, intended name matching, cost, insufficient-XP
failure, full unlock storage, `save_account()`, and reconnect loading. Confirm creation, web,
`gain`, and respec agree before and after purchase. `unlock_cost` alone does not lock a class.

In `src/net/onboarding.c`, add the media key, use the same eligibility rule as terminal creation,
and test the catalog payload plus direct forged selections. Hiding a card is not authorization.
Provision matching client art or record the fallback as an explicit release choice.

## 6. Complete casting, if applicable

Start from the closest actual storage model:

| Model | Useful references |
|-------|-------------------|
| Prepared queues | Wizard, Cleric, Druid, Paladin, Ranger, Blackguard |
| Spontaneous slots | Sorcerer, Bard, Inquisitor, Summoner |
| Extracts | Alchemist |
| At-will known invocations | Warlock |
| PSP and powers | Psionicist |
| Parent-class advancement | Mystic Theurge, Eldritch Knight, Necromancer |

For a caster, trace all of the following rather than stopping at `spell_assignment()`:

- circle access in `compute_spells_circle()` and `get_class_highest_circle()`;
- slots, known limits, bonus attribute, and progression tables;
- preparation/recovery command registration, dispatch, and the matching four dictionary words;
- study choices and preferred caster selection in `src/character/study.c`;
- casting attribute, DC, resource checks, extraction/consumption, metamagic, and rest recovery in
  `src/magic/spell_parser.c` and `src/magic/spell_prep.c`;
- caster membership helpers and macros in `src/utils.c` and `src/utils.h`;
- caster-level and prestige-parent advancement, including `PCAr`/`PCDi` persistence;
- casting visuals, staff test-character setup, premade choices, crafting/items, and NPC behavior;
  and
- save/reload of queues, slots, collections, known spells, and preparation events.

Prepared, spontaneous, extract, at-will, and psionic classes do not share identical commands or
storage. Do not add a consign command where the model has nothing spell-specific to remove. For a
noncaster, keep dictionary entries empty, set casting style to `NULL`, and test that caster
classifiers reject it.

Test boundary circles, study, successful casting, the exact resource consumed, recovery, caster
level, multiclass/prestige advancement, and a full save/reload round trip.

## 7. Decide mobile, world-content, and help behavior

Adding an ID changes builder-visible data even for a PC-only class. Mobile OLC enumerates class
IDs, mobile files persist `Class: <id>`, DG can set classes by name, and HLQ kit conversions carry
class values. Choose one policy:

- implement mobile autoroll category, stats, spell/combat AI, OLC save/load, and related content;
  or
- filter unsupported selection/randomization and provide a safe generic fallback.

Validate all loaded IDs before `CLSLIST_*` access. The current mobile loader uses an inclusive
`NUM_CLASSES` clamp; change it to the last valid ID when extending the registry. Audit tutorials,
trainers, guilds, quest kits, and DG branches:

```bash
rg -uu -n 'has_class\(|%actor\.class%|GET_CLASS|Class:' \
  lib/world src/dgscript src/quest --glob '*.trg' --glob '*.hlq' --glob '*.[ch]'
```

Project policy requires class help in both maintained stores:

1. Add `sql/components/help_<slug>_entries.sql` and a read-only
   `verify_help_<slug>_entries.sql`, following `docs/systems/HELP_SYSTEM.md`.
2. Add both to `sql/components/ci_schema_manifest.txt` (`apply` and `skip`, respectively) and
   `Makefile.am` `EXTRA_DIST`.
3. Add the matching player-readable entry before the final `$~` in
   `lib/text/help/help.hlp`.
4. Apply the migration twice to an isolated test database, run the verifier, reload help in
   development, and test the exact `class-<slug>` topic.

Document real runtime behavior, restrictions, unlocks, commands, and features. Update other current
technical documentation only where the new class changes it; historical changelogs keep their
historical paths.

## 8. Verify persistence and rollback

Class identity is stored beyond `Clas` and `CLvl`. Pfiles also retain class feat points, perk
points/purchases, premade class, preferred arcane/divine class, class-keyed spell state, and some
class-specific craft data. Account unlocks and optional level history live in SQL. Mobiles, HLQs,
and class-specific spell-circle object affects can also persist the ID in world, flat saved-object,
or SQL serialized-object data.

Range-check class IDs before indexing on every new or touched load/display path. Test these round
trips as applicable:

1. level-1 custom character and starting equipment;
2. every feature boundary and unspent class resource;
3. multiclass into and away from the class;
4. respec away and back, including premade policy;
5. perks and account unlock purchase;
6. all caster state and preferred parent class; and
7. mobile, HLQ, craft, and class-specific object data.

Raising the bounds makes pfile writers emit new indexed rows even for characters who never select
the class. After a widened binary saves data, an older binary with smaller arrays is not a safe
rollback. Retain a compatibility rollback build that keeps the permanent ID, widened bounds,
positional entries, load-safe registry row, `level_exp()` handling, and persistence readers while
setting `in_game = N` and rejecting new selection. Shrinking bounds requires a verified reverse
migration across every restorable store.

Production deployment must follow the normal approved path: re-audit the target data, back up all
affected stores, deploy code that understands the ID before content can persist it, apply and
verify help SQL, deploy matching help/web assets, then run the applicable smoke tests. Never edit
the remote production source tree directly.

## 9. Test and build

Add focused production-linked CuTest coverage. A new `test_class_<slug>.c` must appear in:

- `cutest_SOURCES` and `cutest_test_files` in `Makefile.am`; and
- `CUTEST_TEST_SOURCES` in `CMakeLists.txt`.

Test functions must begin with `Test`; `make-tests.sh` regenerates `AllTests.c`. Cover the common
row and each applicable branch:

- Common: ID/bounds/tables, registry values, parsers, invalid IDs, XP, progression boundaries,
  feats, perks, stat caps, staff field, multiclassing, reset, and pfile round trip.
- Base: terminal/web list and direct selection, help, rejected restrictions, starting gear,
  custom build, respec, and premade policy.
- Prestige: creation/respec rejection, each prerequisite failure, successful `gain`, cap, and any
  parent-caster advancement.
- Locked: purchase failures/success, SQL reload, and consistent access on every entry path.
- Caster: circle/slot boundaries, study, cast/consume/recover, caster level, visuals, staff setup,
  and all spell-state round trips.
- NPC/content: OLC round trip, invalid-ID rejection, autoroll/AI policy, HLQ/DG behavior, and
  relevant tutorial paths.
- Help: idempotent SQL migration, verifier, exact runtime topic, and flat-file mirror.

Run the maintained local gates from the development checkout:

```bash
make clean
make -j"$(nproc)"
make test
make install
make test-all

cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

Run the MUD locally with `autorun.sh`, not `luminari.service`. Smoke the actual applicable flows,
quit and reload the character, and inspect logs for invalid-class, bounds, premade, perk, and spell
state errors. The class is complete only when the resulting behavior matches the specification,
not merely when the registry compiles.
