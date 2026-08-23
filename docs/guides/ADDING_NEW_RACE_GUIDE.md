# Adding a New Player Race to LuminariMUD

Status: source-backed developer guide, verified 2026-08-23.

This guide covers the complete path for adding a player race to LuminariMUD.
Most sections describe a race selected during character creation. It also
covers the additional hard-lock and conversion work required for a
transformation-only race such as Lich. Numeric identity, registry data, racial
mechanics, help content, persistence, tests, deployment, and rollback apply to
both paths; account unlocks and terminal/web onboarding apply only to
creation-selectable races.

This is not the procedure for adding only an NPC race family or a wildshape
form. NPCs use `RACE_TYPE_*` values in world files, while player races use
concrete `RACE_*` IDs registered in `race_list[]`.

## Definition of done

A new race is complete only when all of the following are true:

- Its numeric ID is not shared with a different persistent identity, is stable
  and representable in character storage, and is covered by every relevant
  registry bound. Intentional synonym constants may share the same identity.
- `assign_races()` creates a complete `race_list[]` entry.
- A creation-selectable race is listed, accepted, explained, and validated by
  both terminal and web creation.
- A transformation-only race is absent from account-XP purchase and both
  creation paths, even if an unlock row is injected, and has one explicitly
  tested conversion owner.
- Its unlock cost, alignments, classes, size, statistics, language, feats, and
  any custom mechanics behave as designed.
- Its help topic exists in both the database migration and
  `lib/text/help/help.hlp`.
- A newly created or transformed character survives save, disconnect, reload,
  respec where permitted, and level-up without changing race or losing racial
  features.
- Automated tests, the full build, and the applicable creation or conversion
  smoke test pass.

The active data flow is:

```text
src/structs.h numeric ID and bounds
                  |
                  v
src/character/race.c assign_races() ---> race_list[]
                  |                         |
                  |                         +--> stats, size, language, feats
                  |                         +--> race/info commands
                  |                         +--> standard account unlock
                  |                         |       +--> terminal creation
                  |                         |       +--> web onboarding
                  |                         +--> hard lock + conversion owner
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
| Selection mode | Creation-selectable or transformation-only |
| Power tier | Normal, advanced, or epic presentation category |
| Access gate | Free, account-XP unlock, or explicit hard lock plus conversion |
| Genders | Recorded intent, plus enforcement work if any sex is disallowed |
| Statistics | STR, CON, INT, WIS, DEX, and CHA modifiers, in that exact order |
| Alignments | LG, NG, CG, LN, TN, CN, LE, NE, and CE availability |
| Attacks | Wildshape/disguise attack types; normal-form natural attacks are separate |
| Language | One explicit `SKILL_LANG_*` racial language |
| Features | Existing feats, new feats, level granted, and stacking behavior |
| Special rules | XP multiplier, per-level bonuses, anatomy, immunities, or choices |
| Presentation | `race-<slug>` help tag; `race/<slug>` creation media if selectable |
| Compatibility | Existing characters, unlock rows, quests, and world references |
| Transformation | Trigger, prerequisites, consumed state, respec, size, XP, alignment, and save |

Do not use `race_list[].level_adjustment` as proof that an XP penalty is
implemented. It is currently presentation data. Actual advanced and epic race
XP multipliers are hardcoded in `level_exp()` in `src/character/class.c`.

The registry has three independent controls which must not be inferred from
one another:

- `epic_adv` labels `race info` output and excludes non-normal races from the
  random-basic-race helper. It does not set XP, cost, or acquisition rules.
- `level_adjustment` is shown as a web-onboarding fact. The terminal `race info`
  output does not currently show it, and it does not alter XP.
- `unlock_cost` drives the ordinary account-XP lock. It does not create a
  quest-only hard lock.

## Worked source traces: advanced, epic, and hard-locked epic

These existing races demonstrate the three paths. The values are independent
implementation choices, not formulas derived from the tier label:

- Half-Troll is ID 3 with `IS_ADVANCE`, adjustment 2, and cost 1,000. It uses
  standard account unlock and terminal/web creation, plus an explicit 2x case
  in `level_exp()`.
- Crystal Dwarf is ID 4 with `IS_EPIC_R`, adjustment 10, and cost 30,000. It
  uses the same standard acquisition path, plus an explicit 7x XP multiplier
  and custom HP cases.
- Lich is ID 45 with `IS_EPIC_R`, adjustment 10, and cost 999,999,999. Account
  policy hard-denies it, conversion grants it, and explicit code supplies 10x
  XP, custom HP, Undead handling, and respec behavior.

### Advanced trace: Half-Troll

Half-Troll is inside the dense creation range, has `is_pc` true, and is parsed
by `parse_race_long()`. Before purchase, the terminal and web creation catalogs
omit it. `accexp race` finds it inside the `NUM_RACES` loop, stores ID 3 in
`unlocked_races`, and creation accepts it after reload. Its `IS_ADVANCE` value
only supplies the label; the 1,000 account-XP cost, adjustment 2 presentation,
and 2x `level_exp()` multiplier are three separate registrations.

Its level-one feats are linked by `feat_race_assignment()` and granted when
`do_start()` calls `advance_level()`, which in turn calls
`process_race_level_feats()`. Its racial mechanics still live in their owning
subsystems; the registry entries alone do not implement regeneration,
vulnerability, resistance, or vision behavior.

### Epic trace: Crystal Dwarf

Crystal Dwarf follows the same ordinary account-unlock and character-creation
path as Half-Troll even though it is labeled epic. The differences are explicit:
cost 30,000, adjustment 10, a 7x case in `level_exp()`, a larger feat set, and
race-specific HP handling in `init_start_char()`, `advance_level()`, and
`calculate_max_hp()`.

This is the key epic-race lesson: setting `IS_EPIC_R` and adjustment 10 does not
produce any of those mechanics. Derived-stat recomputation must also be traced;
an HP adjustment made only while leveling can be replaced the next time
`calculate_max_hp()` rebuilds the value.

### Hard-locked epic trace: Lich

Lich is a player race but not a creation-selectable race. It deliberately sits
outside `NUM_RACES` at ID 45 while remaining inside `race_list[]`, and
`assign_races()` sets `is_pc` true so player race displays and mechanics can use
it. That flag does not mean "selectable at creation."

The current lock has several layers:

- terminal and web catalogs enumerate only `0..NUM_RACES - 1`;
- `init_char()` rejects a new-character race outside that range;
- `has_unlocked_race()` always returns false for Lich and Vampire, even if the
  account array or database contains the ID;
- account purchase also enumerates only `0..NUM_RACES - 1`;
- the nominal cost is 999,999,999 while account XP is capped at 100,000,000.

The large cost is defense in depth, not the hard-lock implementation. A new
transformation-only race needs an explicit policy denial and tests with a
forged unlock row. It must not rely on price or on being omitted from a menu.

The in-game informational `race list` is different from a creation catalog. It
iterates `NUM_EXTENDED_RACES`, includes every `is_pc` entry, and currently
shows Lich and Vampire with the locked marker. That visibility is expected and
does not make either race creation-selectable.

Lich acquisition is handled by conversion code, discussed in section 8. The
conversion sets the real race before calling `respec_engine()`, allowing the
level-one Lich feats and racial initialization to run during `do_start()`.
`respec_engine()` explicitly preserves pre-transformation size only for Lich
and Vampire. A new transformation race does not inherit that behavior.

Lich is useful source evidence, not a complete template. Its three conversion
implementations have drifted prerequisites, only the RoL rite saves again after
the final XP/alignment changes, and none has an explicit already-Lich guard.
Its flat help has a `LICH` redirect rather than an exact `RACE-LICH` entry, and
there is no race-specific Lich SQL help component. The requirements later in
this guide are the standard for new work, not a claim that every legacy Lich
surface already meets them. Existing tests cover the RoL requirement preflight
and Lich/Vampire size preservation, but not the full hard lock, successful
conversion post-state, consumption, or reload transaction.

## 2. Resolve the numeric ID before implementation

Race IDs are durable data, not reorderable enum positions. The player file
writes the number as `Race: <id>` in `src/players.c`; account unlocks store the
number as `unlocked_races.race_id`; other SQL and world data can also retain
numeric race references. Never renumber or reuse a released ID.

### Current allocation constraints

As of this guide's verification date, `src/structs.h` has these boundaries:

- Selectable IDs are the dense range 0 through 27, with `NUM_RACES` set to 28.
- IDs 26 and 27 are registered as Goblin and Hobgoblin. The stale
  `RACE_DEEP_GNOME` and `RACE_ORC` defines collide with those IDs; they are not
  spare slots or valid distinct persistent identities.
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

### Creation-selectable implementation paths

1. **Implement Half Ogre using its existing ID 28.** This is the only existing
   named slot immediately after the dense player range. Bringing it into the
   creation range requires raising `NUM_RACES` to 29 and updating every fixed
   table, loop, test, and boundary that uses `NUM_RACES`. It does not authorize
   using ID 29 for the following race.

2. **Add any other new creation-selectable race.** Make a separate, reviewed
   registry decision first. The change must either establish a safe non-dense
   playable registry or widen and migrate race storage before allocating a new
   ID. Do not consume a legacy, quest-only, or NPC/form ID merely because it
   appears unused in one source file.

### Transformation-only allocation

IDs 55 through 59 are reserved only by a source comment for future quest-only
races. Using one still requires an explicit registry and persistence review.
Do not raise `NUM_RACES`, because doing so would make every intervening ID part
of dense creation iteration. Decide whether `NUM_EXTENDED_PC_RACES` should be
replaced with keyed PC iteration rather than enlarged across legacy holes, and
audit every consumer of the chosen ID. The hard-lock and conversion work in
sections 7 and 8 is required in addition to ordinary registry mechanics.

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
- place a concrete player race in the `RACE_TYPE_*` namespace;
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
- `set_race_genders()`: neuter, male, female. These values are currently stored
  but not enforced by character creation.
- `set_race_abilities()`: STR, CON, INT, WIS, DEX, CHA. This is not the order
  used by every other ability array in the codebase.
- `set_race_alignments()`: LG, NG, CG, LN, TN, CN, LE, NE, CE.
- `set_race_attack_types()`: exactly 24 booleans in the order shown above. The
  live consumer is `wildshape_weapon_type()` for a disguise or wildshape, not
  ordinary normal-form player attacks.
- `feat_race_assignment()`: race, feat, character level granted, and a stored
  stacking-intent flag.

Set every field explicitly even when the value is zero or Common. The
description is used by `race info` and, for selectable races, web onboarding.
For a selectable race, at least one alignment must be allowed. A race whose
alignments have no intersection with an in-game base class cannot finish
character creation.

Several registry fields are metadata or have narrower consumers than their
names suggest:

- `is_pc` includes transformation-only Lich and Vampire; it is not a creation
  eligibility decision.
- `genders[]` has no runtime reader. Terminal and web creation offer male and
  female before race selection without consulting it. A real restriction needs
  shared creation validation and tests.
- `attack_types[]` selects attacks for concrete disguise/wildshape forms. Add
  normal-form natural attacks through the combat/feat path instead.
- `morph_to_char` and `morph_to_room` are stored by `set_race_details()`, but
  current transformation output uses the family-level `morph_to_*` tables in
  `src/constants.c`. Do not assume the per-race strings will be emitted.
- `race_feat_assign.stacks` is displayed by `race feats`, but
  `process_race_level_feats()` currently increments every matching assignment
  without consulting it. Lich obtains Armor Skin +5 from five separate
  assignments, not from enforcement of this flag. Test the resulting rank.

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

1. Add its stable `FEAT_*` constant in the feat-ID block in `src/structs.h`.
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
`racefix` only grants a missing feat, so duplicated rows do not restore
multiple ranks such as Lich's five Armor Skin assignments.

### XP and per-level bonuses

The `level_adjustment` and `epic_adv` registry fields do not implement
progression. Review `init_start_char()`, `do_start()`, and `advance_level()` in
`src/character/class.c` for one-time and per-level racial bonuses. Review
`calculate_max_hp()` in `src/utils.c` and equivalent derived-stat rebuilders so
the bonus survives recomputation. Review `level_exp()` in
`src/character/class.c` if the race has an XP multiplier. Add explicit cases
and tests only for mechanics in the approved race specification.

For a transformation-only race that resets the character, set the real race
before `respec_engine()` invokes `do_start()` or the old race's level-one
features will be applied. Treat this as a destructive character rebuild, not a
class-only reset. Through `init_start_char()`, it leaves groups, removes
equipped gear, clears followers and affects, and resets class levels, skills,
feats, abilities, languages, domains, class/racial choices, spell state,
favored enemies, perks, and temporary forms. It also clears the premade build
and background and sets `HAS_SET_STATS_STUDY` false. Read the function in full,
decide which state the conversion must preserve or re-establish, and save only
after that post-conversion state is complete.

Existing Lich/Vampire callers pass `NULL` as the respec argument. That is safe
only because `respec_engine()` skips its `*arg` access for those two races. A
new transformation race that copies the call can dereference `NULL`. Make the
argument handling NULL-safe, define whether premade builds are allowed, and
test the new race instead of relying on the existing exceptions.

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

## 6. Wire terminal character creation for a selectable race

This section does not apply to a transformation-only race. Its parser name may
remain useful to `race info`, but creation must reject it as described in
section 8.

The terminal creation flow is in `nanny()` in `src/interpreter.c`. The menu and
the submitted-name handler are separate policy surfaces: the menu enumerates
`0..NUM_RACES - 1` and checks `is_pc` plus the account lock, while the current
`CON_QRACE` handler parses any name known to `parse_race_long()` and checks only
the lock. It does not independently reject an ID outside `NUM_RACES` or an
entry whose `is_pc` is false. Do not treat absence from the printed menu as an
access control.

`race_is_available()` is not currently a creation-eligibility predicate. It
accepts any ID below `NUM_EXTENDED_RACES` and is used to mark entries in the
informational race list. Any shared creation predicate must add the approved
selection-mode and creation-bound policy rather than reusing that helper alone.

1. Add the canonical name and any intentional aliases to `parse_race_long()`
   in `src/character/race.c`. Registry names are not parsed automatically.
   This is an ordered `is_abbrev()` chain, so test that the new prefixes do not
   steal an existing race's input and that the registry `type` sent as the web
   `wireValue` parses back to the intended ID.
2. Make sure both enumeration and submitted-name validation require the
   approved creation range, `race_list[i].is_pc`, and account unlock state.
   Prefer one shared predicate for terminal and web onboarding.
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

## 7. Wire the intended access gate

### Standard account-XP unlock

`unlock_cost` controls the standard account-XP path:

- `0`: the race is not locked.
- Greater than `0`: `is_locked_race()` reports locked, and the account must
  contain the race ID in `account->races[]` / `unlocked_races`.
- Less than `0`: current code also treats the race as unlocked. This is not a
  supported configuration; use zero for a free race.

For a creation-selectable locked race, verify all of these paths in
`src/account.c`:

- `accexp race` lists it with the correct cost;
- `accexp race <name>` finds and purchases it;
- `save_account()` writes the numeric ID to `unlocked_races`;
- a new session reloads the unlock;
- terminal and web creation both omit it before purchase and allow it
  afterward.

The purchase parser does not call `parse_race_long()`. It performs a separate,
ordered abbreviation scan over `race_list[i].type`, so test short-name
collisions in both parsers.

`accexp race` also calls every purchasable race "advanced" in its messages;
that wording is not evidence that `epic_adv` is `IS_ADVANCE`. Ordinary account
unlock works the same way for an advanced Half-Troll and an epic Crystal Dwarf.

The account array uses `0` as its empty-slot sentinel. This is harmless for the
always-unlocked Human ID 0, but it is another reason not to change existing
numeric identities.

### Transformation-only hard lock

Do not put a transformation-only race through the standard purchase path.
`unlock_cost` alone cannot enforce this: a large price can become affordable
after an economy change, and an unlock row can be inserted independently of a
purchase. Under the current terminal handler, retain a positive nominal cost
so the lock check runs, but make the explicit hard-denial policy authoritative.

The current Lich/Vampire exception is an explicit denial in
`has_unlocked_race()`. Their IDs are also outside `NUM_RACES`, so account
purchase and both creation catalogs cannot enumerate them. Preserve all of
these defenses for a new hard-locked race:

- a centralized policy denial that ignores a forged account unlock;
- no account-XP listing or purchase match;
- no terminal or web catalog entry;
- submitted terminal and web values rejected before changing `GET_REAL_RACE`;
- `init_char()` unable to bless the ID as a new-character selection; and
- a separately authorized conversion path.

Add negative tests that inject the race ID into `account->races[]` and, where
practical, an `unlocked_races` row. The character must still be unable to start
as that race.

## 8. Wire a transformation-only acquisition

Lich is not owned by one generic race mechanism. The current source contains
three conversion paths with different prerequisites:

- The RoL special procedure `rol_lich_rite()` in
  `src/spec/spec_rol_conversion.c` requires a PC with at least one Necromancer
  level, total level exactly `LVL_IMMORT - 1`, no group/master/followers, and
  two offerings held or carried by the keeper. It accepts `say` or the
  apostrophe alias with the exact lowercase argument `immortality`. Only after
  preflight does it consume both offerings and the keeper, respec to Wizard,
  reset XP/alignment, and save the final state.
- The standard `.qst` race reward in `complete_quest()` accepts level 30 or
  higher with no group/master/followers, then handles only Lich or Vampire in
  its conversion switch. QEDIT and the world validator permit only `-1`, Lich,
  or Vampire. Gold, XP, and object rewards run before conversion; the
  conversion then discards the XP reward by setting total XP to zero, while
  the other rewards remain. A follower reward runs afterward.
- The legacy high-level quest uses `QUEST_COMMAND_KIT` with the local
  `LICH_QUEST` value 9999. It performs a level-30-or-higher Lich conversion and
  Wizard respec in `src/quest/hlquest.c`.

These are separate implementations, not aliases for one shared policy.
Their level and class requirements already differ. Do not copy all three for a
new race. Choose one acquisition owner in the feature specification and, when
touching more than one route, factor shared eligibility and conversion logic
so the paths cannot drift.

An irreversible conversion should follow this order:

1. Preflight the actor, supported target race, current race/repeat policy,
   exact level/class requirements, group/master/follower state, required quest
   state, and every consumable.
2. Return without consuming rewards, offerings, mobs, or quest state if any
   check fails.
3. Set `GET_REAL_RACE(ch)` before `respec_engine()` so `do_start()` applies the
   new race's level-one setup.
4. Invoke the approved base-class respec and account for everything it clears:
   equipment state, affects, premade build, background, temporary forms,
   languages, choices, class levels, feats, abilities, perks, and spell state.
5. Apply the final XP, alignment, size, homeland, choices, cooldown, messaging,
   and audit log required by the specification.
6. Save after all final fields are set, then verify disconnect/reload and
   protect against a repeated conversion.

Step 6 is deliberate. `respec_engine()` saves internally, but the standard and
legacy Lich callers set XP and alignment after that save. The RoL rite performs
an additional `save_char()` after its final changes; new code should likewise
persist the completed transaction explicitly.

That final save closes the normal success path but does not make the sequence
crash-atomic: the respec has already persisted an intermediate character. If
the specification requires crash recovery, refactor the respec save behind an
explicit defer-save option or record enough durable conversion state to resume
or roll back safely.

If the chosen owner is a standard quest race reward, update all of the
following together:

- the preflight and conversion switch in `src/quest/quest.c`;
- accepted values and prompts in `src/olc/qedit.c`;
- extracted constants in `scripts/world/wtool_lib/constants.py`;
- semantic validation and tests in `scripts/world/wtool_lib/semantics.py` and
  `scripts/world/tests/test_semantics.py`; and
- the race-reward contract in
  `docs/world_game-data/QUEST_FILE_FORMAT.md`.

Also decide whether gold, XP, object, and follower rewards may coexist with the
conversion. Test their exact ordering rather than assuming a normal quest XP
reward survives a reset-to-zero transformation.

If the owner is a registered special procedure, add its registry entry and
test its irreversible preflight and trigger semantics. Bind it to the intended
world entities and update `docs/guides/OLC_SpecProcs.md`. Do not copy the RoL
rite's locally hardcoded offering numbers: use existing symbolic VNUMs, or add
new configuration symbols to `src/vnums.example.h`; never edit the local
`src/vnums.h` while implementing the feature. If legacy high-level quest
compatibility is intentionally extended, update
`docs/world_game-data/HLQUEST_FILE_FORMAT.md` and its validation too. Do not
introduce another magic sentinel unless that legacy format is explicitly the
approved owner.

## 9. Wire web onboarding for a selectable race

This section does not apply to a transformation-only race, which must have no
web creation media entry or catalog choice. The web catalog is server-owned in
`src/net/onboarding.c`; a client must not duplicate race eligibility rules.

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

The current `race_is_selectable()` separately implements the same bounds,
`is_pc`, and account-lock policy expected by terminal creation; it does not
call a shared terminal predicate. Keep both paths synchronized or introduce a
shared eligibility function and test it through both transports.

The current media table is indexed directly by IDs from 0 through
`NUM_RACES - 1`. A non-dense player ID cannot be added safely by merely making
that array larger: the creation, bounds, fallback, and every-playable-race tests
must be redesigned around the approved registry model.

`web_onboarding_race_media_key()` is also used when existing account characters
are displayed. Because it is bounded by `NUM_RACES`, a current Lich or Vampire
receives the fallback race image. If a transformation-only race needs distinct
account-card art, separate display-media lookup from creation eligibility; do
not add the race to the creation catalog merely to obtain an image.

## 10. Add player help in both maintained stores

A selectable race's terminal creation case calls an exact `race-<slug>` topic.
A transformation-only race does not need a `CON_QRACE` help case, but it still
needs a discoverable `race-<slug>` topic explaining prerequisites, irreversible
effects, and how it is acquired. Add the keyword and content in both places
required by the project:

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
the advertised player-race list, unlock behavior, conversion, or creation
rules.

## 11. Protect persistence and compatibility

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
3. Save and reload a created or transformed character through the normal
   player-file path.
4. For a standard locked race, save and reload an account that purchased the
   unlock. For a transformation-only race, prove that an injected unlock still
   cannot authorize creation.
5. After a conversion, verify the final race, class, XP, alignment, real size,
   features, and any consumed state after disconnect/reload, not merely before
   the internal respec save.
6. Verify staff inspection, account menus, score displays, and I3/MSDP output
   do not index outside `race_list[]`.

If the race ID changes during development, migrate every development character
and unlock row before testing again. After public release, do not change the
ID.

If the race must be withdrawn after characters exist, the safe rollback is to
stop new selection while preserving the constant, registry entry, parser, and
load compatibility for existing characters. Do not delete the ID or assign it
to a different race.

## 12. Add automated tests

At minimum, add these assertions for either path:

- ID uniqueness, representability, and correct bounds;
- complete `race_list[RACE_NEW]` registration after `assign_races()`;
- canonical name and alias parsing through `parse_race_long()`;
- racial ability modifiers, size, language, and feat assignments;
- level-one and later-level feat grants, including the resulting rank when a
  feat is assigned more than once;
- the exact XP multiplier and derived-stat rebuild behavior, when applicable;
- independence of the normal/advanced/epic label, displayed adjustment,
  unlock cost, and implemented progression;
- family, anatomy, immunity, item, or custom-choice mechanics in the design;
- player persistence round trips;
- database help migration idempotency and verifier success.

For a creation-selectable race, also assert:

- terminal menu and direct-name submission use the same range, `is_pc`, and
  lock policy;
- locked and unlocked eligibility, account purchase, and account persistence;
- at least one valid class/alignment path;
- a non-fallback web media key and presence in the web race catalog; and
- catalog pagination and wire-budget safety.

For a transformation-only race, also assert:

- it is absent from account purchase and terminal/web creation catalogs;
- direct terminal/web submission is rejected before changing the character;
- a forged in-memory unlock and a forged SQL unlock do not bypass the hard
  lock;
- every failed conversion preflight leaves offerings, mobs, quest state, race,
  class, XP, and alignment unchanged;
- the respec argument cannot cause a NULL dereference and the intended gear,
  affect, language, background, choice, and premade-build policy is enforced;
- the authorized trigger produces the exact race, class, XP, alignment, size,
  level-one feats, and audit behavior; and
- existing-character web/account presentation uses the intended media or a
  deliberately accepted fallback without exposing a creation choice; and
- repeated conversion is rejected and the completed state survives reload.

Prefer a focused `unittests/CuTest/test_race.c` suite when the coverage does not
fit an existing production-linked suite. When adding that source file, update
all three test manifests required by the repository:

- `cutest_SOURCES` in `Makefile.am`;
- `cutest_test_files` in `Makefile.am`;
- `CUTEST_TEST_SOURCES` in `CMakeLists.txt`.

Extend `unittests/CuTest/test_web_onboarding.c` for the presentation contract.
CuTest test functions must begin with `Test`; `make-tests.sh` regenerates
`AllTests.c`.

## 13. Build and run the end-to-end smoke test

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
`luminari.service`. Complete the applicable acquisition matrix below and this
common runtime matrix against a development database:

1. Boot without race-table, feat-table, help, or bounds errors.
2. Confirm `race list`, `race info <name>`, and `race feats <name>`.
3. Confirm the exact `help race-<slug>` topic comes from the database.
4. Confirm initial statistics, real size, racial language, racial feats, and
   any special choice.
5. Gain enough levels to exercise every non-level-one racial grant and any XP
   multiplier or per-level bonus.
6. Quit, reconnect, and verify the numeric race, statistics, feats, language,
   choices, and size are unchanged.
7. Inspect the character through account, score, staff-stat, MSDP/web, and I3
    surfaces that are enabled in the development environment.

For a creation-selectable race:

1. Create it through the terminal flow using both a custom and a premade build.
2. Create it through web onboarding and inspect label, description, facts,
   media key, pagination, and confirmation.
3. For a standard locked race, verify a fresh account cannot select it,
   purchase it with account XP, reconnect, and then select it.
4. Exercise every supported sex, at least one allowed alignment, and at least
   one valid base class. If the design restricts sex, verify the new shared
   enforcement rather than relying on `race_data.genders[]`.
5. Confirm the account unlock also survives reconnect.

For a transformation-only race:

1. Confirm it is absent from account purchase and both creation catalogs.
2. Attempt direct submission in both protocols with no unlock, an in-memory
   forged unlock, and a development-database forged unlock; every attempt must
   fail without changing race.
3. Exercise every failed conversion prerequisite and confirm no offering,
   keeper, quest state, or character state is consumed.
4. Complete the authorized conversion and verify its exact class, XP,
   alignment, size, features, messages, and log.
5. Reconnect and verify the final post-conversion state, then confirm the
   conversion cannot be repeated.

Ollama, I3, and Discord are not expected to work on local development unless
they are specifically in scope. Their absence is not a race-test failure, but
code-level bounds and serialization tests still must pass.

## 14. Deployment and rollback

The race registry is built at boot, so a code deployment requires a controlled
restart. Use the normal deployment procedure after all local and staging gates
pass.

Recommended order:

1. Back up player files and the account/help database tables.
2. Apply and verify the help SQL component in the target non-production stage.
3. Deploy the tested code and matching flat help file.
4. Restart and inspect the boot log.
5. Repeat the applicable creation or conversion, access-gate, save/reload, and
   help smoke checks.
6. Only then schedule the production rollout through the normal approval path.

Rollback before anyone acquires the race can remove its acquisition and code.
Rollback after persisted characters or unlocks exist must retain the numeric
ID and enough registry/parser behavior to load those records safely. Disable
new creation, purchase, and conversion with a compatibility entry; do not
renumber, reuse, or erase the ID.

## Source map

| Area | Authority |
|------|-----------|
| Concrete race IDs and bounds | `src/structs.h` |
| Runtime race structure | `struct race_data` in `src/structs.h` |
| Registry and parser | `src/character/race.c`, `assign_races()`, `parse_race_long()` |
| Registry declarations | `src/character/race.h` |
| Terminal creation | `src/interpreter.c`, `CON_QRACE` and `CON_QRACE_HELP` |
| Creation initialization | `src/db.c`, `init_char()` |
| Stats, grants, and XP | `src/character/class.c`, `src/utils.c` |
| Ability build paths | `src/character/premadebuilds.c`, `src/character/study.c` |
| Feat IDs and registry | `src/structs.h`, `src/character/feats.c` |
| Feat mechanics | The subsystem that owns the feature |
| Account unlock and hard-lock policy | `src/account.c`, `unlocked_races` |
| Web catalog and media | `src/net/onboarding.c` |
| Respec behavior | `respec_engine()` in `src/act.other.c` |
| Dedicated Lich rite | `src/spec/spec_rol_conversion.c`, `src/spec/spec_registry.c` |
| Special-procedure builder contract | `docs/guides/OLC_SpecProcs.md` |
| Lich rite mechanics tests | `unittests/CuTest/test_spec_mechanics.c` |
| Standard quest conversion | `src/quest/quest.c`, `src/olc/qedit.c` |
| Legacy Lich conversion | `src/quest/hlquest.c` |
| Quest race-reward validation | `scripts/world/wtool_lib/semantics.py` |
| Quest validator tests | `scripts/world/tests/test_semantics.py` |
| Quest race-reward format | `docs/world_game-data/QUEST_FILE_FORMAT.md` |
| Legacy quest format | `docs/world_game-data/HLQUEST_FILE_FORMAT.md` |
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
- [ ] Tier label, displayed adjustment, unlock gate, and XP multiplier were
      reviewed as independent behavior.
- [ ] Parser aliases and exact player help are wired.
- [ ] A selectable race uses the same tested eligibility policy for terminal
      and web creation.
- [ ] A standard unlock purchase and reload work, or the selectable race is
      intentionally free.
- [ ] A selectable race has at least one class/alignment combination that can
      finish creation.
- [ ] A transformation-only race is denied by account purchase, both creation
      catalogs, direct submission, and forged unlocks.
- [ ] A transformation-only race has one approved conversion owner with
      no-consumption preflight, repeat protection, and a save after final state.
- [ ] Stats, size, language, feats, and special mechanics are tested.
- [ ] Duplicate racial feat assignments produce the intended rank.
- [ ] XP and per-level behavior are implemented and survive derived-stat
      recomputation rather than merely being displayed.
- [ ] Family predicates and appearance gates match the design.
- [ ] Web media and catalog wire-budget tests pass for a selectable race; a
      transformation-only race has no creation media/catalog entry.
- [ ] Existing-character web media behavior is defined for a
      transformation-only race without weakening its creation hard lock.
- [ ] Database help migration and verifier are complete.
- [ ] Matching `lib/text/help/help.hlp` content is complete.
- [ ] Player and applicable account save/reload tests pass, including final
      post-conversion state.
- [ ] `make test` and `make install` pass.
- [ ] Applicable local creation or conversion smoke tests pass.
- [ ] Deployment and post-persistence rollback plans preserve the numeric ID.
