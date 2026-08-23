# Adding a New Playable Race to LuminariMUD

Status: source-backed developer guide, verified 2026-08-23.

This guide covers the complete path for adding a selectable player race to
LuminariMUD. It covers numeric identity, registry data, character creation,
racial mechanics, account unlocks, web onboarding, help content, persistence,
tests, deployment, and rollback.

This is not the procedure for adding only an NPC race family or a wildshape
form. NPCs use `RACE_TYPE_*` values in world files, while playable characters
use concrete `RACE_*` IDs registered in `race_list[]`. Quest-only race
transformations such as Lich and Vampire are also a separate workflow.

## Definition of done

A new race is complete only when all of the following are true:

- Its numeric ID is unique, stable, representable in character storage, and
  covered by every relevant registry bound.
- `assign_races()` creates a complete `race_list[]` entry.
- Terminal and web character creation both list, accept, explain, and validate
  it.
- Its unlock cost, alignments, classes, size, statistics, language, feats, and
  any custom mechanics behave as designed.
- Its help topic exists in both the database migration and
  `lib/text/help/help.hlp`.
- A newly created character survives save, disconnect, reload, and level-up
  without changing race or losing racial features.
- Automated tests, the full build, and the manual creation smoke test pass.

The active data flow is:

```text
src/structs.h numeric ID and bounds
                  |
                  v
src/character/race.c assign_races() ---> race_list[]
                  |                         |
                  |                         +--> stats, size, language, feats
                  |                         +--> race/info commands
                  |                         +--> account unlock rules
                  |                         +--> terminal creation
                  |                         +--> web onboarding
                  |                         +--> save/load numeric identity
                  v
        help, tests, deployment, rollback
```

## 1. Write the race specification first

Record these decisions before editing code. They determine which downstream
systems must change.

| Decision | Required detail |
|----------|-----------------|
| Identity | Canonical `RACE_*` constant and permanent numeric ID |
| Names | Parser token, display name, colored name, four-character abbreviation |
| Classification | Existing `RACE_TYPE_*` family and `SIZE_*` value |
| Availability | Playable or quest-only; normal, advanced, or epic |
| Unlock | Account-XP cost; zero means available without an unlock |
| Genders | Neuter, male, and female availability |
| Statistics | STR, CON, INT, WIS, DEX, and CHA modifiers, in that exact order |
| Alignments | LG, NG, CG, LN, TN, CN, LE, NE, and CE availability |
| Attacks | Unarmed attack types available without a weapon |
| Language | One explicit `SKILL_LANG_*` racial language |
| Features | Existing feats, new feats, level granted, and stacking behavior |
| Special rules | XP multiplier, per-level bonuses, anatomy, immunities, or choices |
| Presentation | `race-<slug>` help tag and `race/<slug>` web media key |
| Compatibility | Existing characters, unlock rows, quests, and world references |

Do not use `race_list[].level_adjustment` as proof that an XP penalty is
implemented. It is currently presentation data. Actual advanced and epic race
XP multipliers are hardcoded in `level()` in `src/character/class.c`.

## 2. Resolve the numeric ID before implementation

Race IDs are durable data, not reorderable enum positions. The player file
writes the number as `Race: <id>` in `src/players.c`; account unlocks store the
number as `unlocked_races.race_id`; other SQL and world data can also retain
numeric race references. Never renumber or reuse a released ID.

### Current allocation constraints

As of this guide's verification date, `src/structs.h` has these boundaries:

- Selectable IDs are the dense range 0 through 27, with `NUM_RACES` set to 28.
- `RACE_H_OGRE` is already assigned ID 28 but is not implemented as a
  selectable race.
- IDs 29 through 54 are reserved legacy IDs. Lich 45 and Vampire 46 are
  explicit quest-only exceptions inside that area.
- IDs 55 through 59 are reserved by comment for future quest-only races.
- Extended NPC/form IDs begin at 60 and currently continue through 148.
- `NUM_EXTENDED_RACES` is 149, the array bound rather than a playable count.
- `char_player_data.race` is a signed `byte`, so values above 127 cannot be
  represented safely without changing that field and auditing every reader,
  writer, serializer, and protocol consumer.

There is therefore no generic safe "next playable race ID" in the current
layout.

### The two valid implementation paths

1. **Implement Half Ogre using its existing ID 28.** This is the only existing
   named slot immediately after the dense player range. Bringing it into the
   creation range requires raising `NUM_RACES` to 29 and updating every fixed
   table, loop, test, and boundary that uses `NUM_RACES`. It does not authorize
   using ID 29 for the following race.

2. **Add any other new playable race.** Make a separate, reviewed registry
   decision first. The change must either establish a safe non-dense playable
   registry or widen and migrate race storage before allocating a new ID. Do
   not consume a legacy, quest-only, or NPC/form ID merely because it appears
   unused in one source file.

An ID-layout change is a persistence migration and should be reviewed as such.
The proposal must state the old and new bounds, the chosen stable ID, how
terminal and web enumeration will find non-dense playable IDs, and how old
player files and account unlock rows remain valid.

Use these searches during the allocation review:

```bash
rg -n '^#define (RACE_|NUM_RACES|NUM_EXTENDED|LEGACY_RACE)' src/structs.h
rg -n '\b(NUM_RACES|NUM_EXTENDED_PC_RACES|NUM_EXTENDED_RACES)\b' src unittests
rg -n '\b(Race:|race_id|race_reward)\b' src sql docs lib/world
rg -n 'GET_(REAL_)?RACE|race_list\[' src --glob '*.[ch]'
```

`MAX_UNLOCKED_RACES` is the number of unlock slots on an account. It is not a
maximum race ID and should not be changed just because the chosen ID is larger.

### Bounds that require special attention

If the ID or any race bound changes, inspect every search result, not just the
following common consumers:

| Source | Why it matters |
|--------|----------------|
| `src/interpreter.c` | Terminal creation menus, parsing result, and race help dispatch |
| `src/db.c` | `init_char()` rejects a selected race outside `NUM_RACES` |
| `src/account.c` | Account-XP race listing and purchase loops use `NUM_RACES` |
| `src/net/onboarding.c` | Web catalog bounds and a position-indexed media-key table |
| `src/utils.c` | `get_race_by_name()` searches only `NUM_RACES` |
| `src/character/race.c` | Random basic-race selection and extended registry allocation |
| `src/constants.c` | `racial_spells[NUM_RACES][3]` has positional entries |
| `src/character/feats.c` | Legacy race-feat display uses `NUM_EXTENDED_PC_RACES` |
| `src/movement/movement_tracks.c` | PC race-name lookup is bounded by `NUM_RACES` |
| `src/olc/medit.c` | One random PC-race display path uses `NUM_RACES` |
| `unittests/CuTest/test_web_onboarding.c` | Media bounds and every-playable-race assertions |

Also inspect `get_random_basic_pc_race()` before changing `NUM_RACES`. Its
current `dice(1, NUM_RACES + 1)` followed by decrement can sample index
`NUM_RACES`; expansion work must add a regression test and correct the
selection boundary rather than copying it.

## 3. Add the registry constant and bounds

After the ID plan is approved, add the permanent `RACE_NEW` constant in the
concrete-race block in `src/structs.h`. Adjust only the bounds required by the
chosen registry design.

Do not:

- renumber existing `RACE_*` or `LEGACY_RACE_*` values;
- assume `NUM_RACES`, `NUM_EXTENDED_PC_RACES`, and `NUM_EXTENDED_RACES` mean the
  same thing;
- place a concrete playable race in the `RACE_TYPE_*` namespace;
- change `src/campaign.h`, `src/mud_options.h`, or `src/vnums.h` as part of the
  race addition.

Most playable races reuse an existing family such as
`RACE_TYPE_HUMANOID`, `RACE_TYPE_GIANT`, `RACE_TYPE_FEY`, or
`RACE_TYPE_OUTSIDER`. Adding a new family is a larger change: it requires a new
`RACE_TYPE_*`, a new `NUM_RACE_TYPES` bound, and corresponding entries in the
`morph_to_*`, `race_family_abbrevs`, `race_family_short`,
`race_family_types`, and `race_family_types_plural` tables in
`src/constants.c`, plus OLC and favored-enemy validation.

## 4. Register the race in `assign_races()`

`assign_races()` in `src/character/race.c` is the authoritative runtime
registry. Place the new block with the other player races and follow the
current helper API. This skeleton shows all core fields:

```c
add_race(RACE_NEW, "new race", "New Race", "\tCNew Race\tn", "NewR", "\tCNewR\tn",
         RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);

set_race_details(
    RACE_NEW,
    "The character-creation and race-info description of the new race.",
    "Your body changes until your form becomes that of a New Race.",
    "$n's body changes until $s form becomes that of a New Race.");

set_race_genders(RACE_NEW, N, Y, Y);
set_race_abilities(RACE_NEW, 0, 0, 0, 0, 0, 0);
set_race_alignments(RACE_NEW, Y, Y, Y, Y, Y, Y, Y, Y, Y);

set_race_attack_types(RACE_NEW,
                      /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                      Y, N, N, N, N, N, N, N, N, N, N, N,
                      /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                      N, Y, N, N, N, N, N, N, N, N, N, N);

feat_race_assignment(RACE_NEW, FEAT_EXAMPLE, 1, N);
race_list[RACE_NEW].racial_language = SKILL_LANG_COMMON;
```

The argument contracts are easy to misread:

- `add_race()`: ID, parser-style name, display name, colored display name,
  abbreviation, colored abbreviation, family, size, playable flag, level
  adjustment, unlock cost, and normal/advanced/epic classification.
- `set_race_genders()`: neuter, male, female.
- `set_race_abilities()`: STR, CON, INT, WIS, DEX, CHA. This is not the order
  used by every other ability array in the codebase.
- `set_race_alignments()`: LG, NG, CG, LN, TN, CN, LE, NE, CE.
- `set_race_attack_types()`: exactly 24 booleans in the order shown above.
- `feat_race_assignment()`: race, feat, character level granted, stacks.

Set every field explicitly even when the value is zero or Common. The
description is used by `race info` and web onboarding. At least one gender and
one alignment must be allowed. A race whose alignments have no intersection
with an in-game base class cannot finish character creation.

`affect_assignment()` currently builds `affassign_list`, but no runtime code
consumes that list. Do not use it to implement a racial effect. Use a registered
feat or add an explicitly tested consumer first.

## 5. Implement racial mechanics

### Ability modifiers and size

`set_race_abilities()` feeds both custom-study statistics and premade builds.
`do_start()` in `src/character/class.c` sets the character's real size from
`race_list[GET_RACE(ch)].size`. Test both a custom build and at least one
premade build; they apply the same registry values through different code.

`TOTAL_STAT_POINTS()` in `src/utils.h` has special budgets for Human and Half
Elf. Add a race there only when its design intentionally changes the point-buy
budget; ordinary racial modifiers do not require a new case.

### Racial feats

Prefer existing feats when they express the mechanic. Register grants through
`feat_race_assignment()` in `assign_races()`. `process_race_level_feats()` in
`src/character/class.c` applies those grants from `advance_level()` when the
character reaches the configured level.

If a racial feature needs a new feat, implement the feat end to end:

1. Add its stable `FEAT_*` constant in `src/character/feats.h`.
2. Register its metadata with `feato()` in `assign_feats()` in
   `src/character/feats.c`.
3. Implement the behavior in the subsystem that owns the mechanic.
4. Add focused CuTest coverage for the actual effect, not only the constant.
5. Add player help in both database SQL and `lib/text/help/help.hlp`.

If the mechanic needs a new C source file, add that file to both `Makefile.am`
and `CMakeLists.txt`.

The old `level_feats[][LEVEL_FEATS]` table in `src/character/class.c` is marked
deprecated. It still drives `racefix` and the "Obtained by Races" section of
`feat info`; it is not the authoritative grant path. For a genuinely new race
with no old characters, do not treat this table as a substitute for
`feat_race_assignment()`. If compatibility or feat-display requirements call
for an entry there, keep it synchronized deliberately and test both paths.

### XP and per-level bonuses

The `level_adjustment` and `epic_adv` registry fields do not fully implement
progression. Review both `do_start()` and `advance_level()` in
`src/character/class.c` for one-time and per-level racial bonuses. Review
`level()` in the same file if the race has an XP multiplier. Add explicit cases
and tests only for mechanics in the approved race specification.

### Family predicates, anatomy, and special choices

Do not assume that `race_list[RACE_NEW].family` automatically affects all
family mechanics. Several `IS_*` predicates in `src/utils.h` special-case PCs;
for example, non-morphed PCs are generally treated as humanoid, while Undead
and Construct PCs require explicit handling. Test the new race against every
family-sensitive mechanic in its design.

Update these optional surfaces when applicable:

- `is_furry()`, `has_horns()`, `has_scales()`, and `race_has_no_hair()` in
  `src/character/race.c` for character-description choices.
- Race convenience predicates in `src/utils.h` when existing mechanics need
  an exact race or ancestry grouping.
- `has_racial_abils_unchosen()` and the study flow when the race must choose an
  ancestry, cantrip, resistance, or similar option. The choice also needs a
  persisted field, save/load coverage, web presentation, and restart rules.
- `racial_spells[NUM_RACES][3]` in `src/constants.c` only after tracing a live
  consumer. The current table has no reader and must not be mistaken for a
  working spell grant.
- `invalid_race()` and object flags only if the design adds race-specific item
  restrictions. A new anti-race flag also affects object persistence, OEDIT,
  flag tables, documentation, and world-data tests.

Age and homeland are currently general character-roleplay choices, not fields
on `race_data`. Do not add race-specific age or hometown tables unless the
feature specification requires a broader creation-system change.

## 6. Wire terminal character creation

The terminal creation flow is in `nanny()` in `src/interpreter.c`.

1. Add the canonical name and any intentional aliases to `parse_race_long()`
   in `src/character/race.c`. Registry names are not parsed automatically.
2. Make sure the creation enumeration includes the race and still filters on
   `race_list[i].is_pc` and account unlock state.
3. Add a `CON_QRACE` switch case that calls
   `perform_help(d, "race-new-race")`.
4. Confirm that `CON_QRACE_HELP` can return to the menu and that the selected
   race reaches class selection.
5. Confirm `init_char()` in `src/db.c` accepts the ID instead of resetting it
   to `RACE_UNDEFINED`.

`parse_race()` is the old single-character mapping used by legacy race
bitvectors. It is not the main creation parser. Do not add an arbitrary letter
there unless a traced consumer requires it and the bitvector can represent the
new race.

The base-class menu calls `valid_class_race_alignment()`. Despite its name,
that function only tests whether the class and race share at least one legal
alignment. There is no general per-race base-class allowlist. If the new race
must prohibit or require classes, implement one shared validation rule and use
it from terminal creation, web onboarding, and later class changes; changing
only the menu is not enforcement.

## 7. Wire account unlocks

`unlock_cost` controls the standard account-XP path:

- `0`: the race is not locked.
- Greater than `0`: `is_locked_race()` reports locked, and the account must
  contain the race ID in `account->races[]` / `unlocked_races`.

For a locked race, verify all of these paths in `src/account.c`:

- `accexp race` lists it with the correct cost;
- `accexp race <name>` finds and purchases it;
- `save_account()` writes the numeric ID to `unlocked_races`;
- a new session reloads the unlock;
- terminal and web creation both hide or disable it before purchase and allow
  it afterward.

The account array uses `0` as its empty-slot sentinel. This is harmless for the
always-unlocked Human ID 0, but it is another reason not to change existing
numeric identities.

Do not use the quest race-reward field as an account unlock. `complete_quest()`
currently implements only direct Lich and Vampire transformations, and the
world validator documents those as the only legal race rewards. A new
quest-based transformation requires its own specification, conversion logic,
respec behavior, validator change, file-format documentation, and tests.

## 8. Wire web onboarding

The web catalog is server-owned in `src/net/onboarding.c`; a client must not
duplicate race eligibility rules.

1. Add a stable `race/<slug>` entry to `race_media_keys` or to the replacement
   keyed lookup introduced by a non-dense registry change.
2. Keep the key independent of colored display text and spelling aliases.
3. Ensure `race_is_selectable()` can address the new ID.
4. Extend `TestWebOnboardingMediaKeysAreStableAndBounded()` in
   `unittests/CuTest/test_web_onboarding.c`.
5. Run `TestPopulatedRaceCatalogStaysWithinTheOnboardingWireBudget`; adding a
   race can change catalog pagination and payload size.
6. Provision the matching image or fallback behavior in the web-client asset
   repository when the UI expects race art.

The current media table is indexed directly by IDs from 0 through
`NUM_RACES - 1`. A non-dense player ID cannot be added safely by merely making
that array larger: the creation, bounds, fallback, and every-playable-race tests
must be redesigned around the approved registry model.

## 9. Add player help in both maintained stores

Character creation calls an exact `race-<slug>` topic. Add that keyword in both
places required by the project:

1. **Database authority:** add an idempotent
   `sql/components/help_race_<slug>_entries.sql` and a read-only
   `verify_help_race_<slug>_entries.sql`.
2. **Flat-file mirror:** add the same player-facing topic to
   `lib/text/help/help.hlp` before the final `$~` marker.

The flat-file entry follows this shape:

```text
NEW-RACE RACE-NEW-RACE

New Race

Player-facing description, statistics, features, restrictions, unlock cost,
and any special choices or commands.
#0
```

For the SQL pair:

- use `INSERT ... ON DUPLICATE KEY UPDATE` for `help_entries`;
- assign deterministic keywords in `help_keywords`;
- verify the entry, `min_level`, required text, required keywords, and keyword
  ownership;
- classify the migration as `apply` and the verifier as `skip` in
  `sql/components/ci_schema_manifest.txt`;
- add both SQL files to `Makefile.am` `EXTRA_DIST`;
- apply the migration twice in an isolated development test to prove
  idempotency, then run the verifier;
- restart or reload help and test the exact `race-<slug>` lookup in game.

Follow `docs/systems/HELP_SYSTEM.md` for the maintained SQL workflow. Never
change production help directly while developing the race.

Also update relevant player/developer documentation when the new race changes
the advertised playable-race list, unlock behavior, or creation rules.

## 10. Protect persistence and compatibility

The authoritative player record is the ASCII player file written by
`save_char()`. Its `Race:` field is numeric. MariaDB also has an integer
`player_data.race` column, but current code does not make that column the
authoritative character load path. Account race unlocks are authoritative SQL
rows in `unlocked_races`.

Before release:

1. Inventory the candidate ID across player files, account unlock rows, SQL
   components, quests, and source constants in a development copy of the data.
2. Prove the runtime field can represent the ID without truncation or sign
   conversion.
3. Save and reload a character of the new race through the normal player-file
   path.
4. Save and reload an account that unlocked the race.
5. Verify staff inspection, account menus, score displays, and I3/MSDP output
   do not index outside `race_list[]`.

If the race ID changes during development, migrate every development character
and unlock row before testing again. After public release, do not change the
ID.

If the race must be withdrawn after characters exist, the safe rollback is to
stop new selection while preserving the constant, registry entry, parser, and
load compatibility for existing characters. Do not delete the ID or assign it
to a different race.

## 11. Add automated tests

At minimum, add assertions for:

- ID uniqueness, representability, and correct bounds;
- complete `race_list[RACE_NEW]` registration after `assign_races()`;
- canonical name and alias parsing through `parse_race_long()`;
- playable and locked/unlocked eligibility;
- at least one valid class/alignment path;
- racial ability modifiers, size, language, and feat assignments;
- level-one and later-level feat grants, when applicable;
- XP or per-level bonuses, when applicable;
- family, anatomy, immunity, item, or custom-choice mechanics in the design;
- a non-fallback web media key and presence in the web race catalog;
- catalog pagination and wire-budget safety;
- player and account persistence round trips;
- database help migration idempotency and verifier success.

Prefer a focused `unittests/CuTest/test_race.c` suite when the coverage does not
fit an existing production-linked suite. When adding that source file, update
all three test manifests required by the repository:

- `cutest_SOURCES` in `Makefile.am`;
- `cutest_test_files` in `Makefile.am`;
- `CUTEST_TEST_SOURCES` in `CMakeLists.txt`.

Extend `unittests/CuTest/test_web_onboarding.c` for the presentation contract.
CuTest test functions must begin with `Test`; `make-tests.sh` regenerates
`AllTests.c`.

## 12. Build and run the end-to-end smoke test

Run the production-linked suite and install the tested binary:

```bash
make clean
make -j$(nproc)
make test
make install
```

If `make test` leaves a root-level `luminari` artifact, `make install` removes
it while installing `bin/luminari`.

On local development, start the game with `autorun.sh`, not
`luminari.service`. Complete this manual matrix against a development database:

1. Boot without race-table, feat-table, help, or bounds errors.
2. Confirm `race list`, `race info <name>`, and `race feats <name>`.
3. Confirm the exact `help race-<slug>` topic comes from the database.
4. Create the race through the terminal flow using both a custom and a premade
   build.
5. Create it through web onboarding and inspect label, description, facts,
   media key, pagination, and confirmation.
6. For a locked race, verify a fresh account cannot select it, purchase it with
   account XP, reconnect, and then select it.
7. Exercise every allowed gender, at least one allowed alignment, and at least
   one valid base class.
8. Confirm initial statistics, real size, racial language, racial feats, and
   any special choice.
9. Gain enough levels to exercise every non-level-one racial grant and any XP
   multiplier or per-level bonus.
10. Quit, reconnect, and verify the numeric race, unlock, statistics, feats,
    language, choices, and size are unchanged.
11. Inspect the character through account, score, staff-stat, MSDP/web, and I3
    surfaces that are enabled in the development environment.

Ollama, I3, and Discord are not expected to work on local development unless
they are specifically in scope. Their absence is not a race-test failure, but
code-level bounds and serialization tests still must pass.

## 13. Deployment and rollback

The race registry is built at boot, so a code deployment requires a controlled
restart. Use the normal deployment procedure after all local and staging gates
pass.

Recommended order:

1. Back up player files and the account/help database tables.
2. Apply and verify the help SQL component in the target non-production stage.
3. Deploy the tested code and matching flat help file.
4. Restart and inspect the boot log.
5. Repeat the creation, unlock, save/reload, and help smoke checks.
6. Only then schedule the production rollout through the normal approval path.

Rollback before anyone creates the race can remove its selection and code.
Rollback after persisted characters or unlocks exist must retain the numeric ID
and enough registry/parser behavior to load those records safely. Disable new
selection with a compatibility entry; do not renumber, reuse, or erase the ID.

## Source map

| Area | Authority |
|------|-----------|
| Concrete race IDs and bounds | `src/structs.h` |
| Runtime race structure | `struct race_data` in `src/structs.h` |
| Registry and parser | `src/character/race.c`, `assign_races()`, `parse_race_long()` |
| Registry declarations | `src/character/race.h` |
| Terminal creation | `src/interpreter.c`, `CON_QRACE` and `CON_QRACE_HELP` |
| Creation initialization | `src/db.c`, `init_char()` |
| Stats, size, grants, XP | `src/character/class.c`, `premadebuilds.c`, and `study.c` |
| Feat registry and mechanics | `src/character/feats.h`, `src/character/feats.c`, owning subsystem |
| Account unlocks | `src/account.c`, `unlocked_races` |
| Web catalog and media | `src/net/onboarding.c` |
| Character persistence | `src/players.c`, `Race:` |
| Help architecture | `docs/systems/HELP_SYSTEM.md` |
| Flat help mirror | `lib/text/help/help.hlp` |
| SQL help authority | `sql/components/help_race_<slug>_entries.sql` |
| Web tests | `unittests/CuTest/test_web_onboarding.c` |
| Build and test procedure | `docs/guides/TESTING_GUIDE.md` |

## Final review checklist

- [ ] Race specification is approved.
- [ ] Numeric ID and bound changes have a persistence-safety review.
- [ ] No existing or reserved ID was renumbered or reused.
- [ ] Runtime storage can represent the selected ID.
- [ ] `assign_races()` registers every field explicitly.
- [ ] Parser aliases and exact terminal help dispatch are wired.
- [ ] Terminal and web creation use the same eligibility rules.
- [ ] Unlock purchase and reload work, or unlock cost is intentionally zero.
- [ ] At least one class/alignment combination can finish creation.
- [ ] Stats, size, language, feats, and special mechanics are tested.
- [ ] XP and per-level behavior are implemented rather than merely displayed.
- [ ] Family predicates and appearance gates match the design.
- [ ] Web media and catalog wire-budget tests pass.
- [ ] Database help migration and verifier are complete.
- [ ] Matching `lib/text/help/help.hlp` content is complete.
- [ ] Player and account save/reload tests pass.
- [ ] `make test` and `make install` pass.
- [ ] Local terminal and web smoke tests pass.
- [ ] Deployment and post-persistence rollback plans preserve the numeric ID.
