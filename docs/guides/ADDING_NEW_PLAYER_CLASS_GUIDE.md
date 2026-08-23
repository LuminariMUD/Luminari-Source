# Adding a New Player Class to LuminariMUD

Status: source-backed developer guide, verified 2026-08-23.

This guide covers the complete path for adding a selectable player class to
LuminariMUD: numeric identity, registry data, level progression, feats and
spellcasting, character creation, the `gain` multiclass path, account unlocks,
web onboarding, help content, persistence, tests, deployment, and rollback.

It applies to base classes (taken at level 1, unlimited or capped levels) and
to prestige classes (entered later through prerequisites). It does not cover
NPC-only class behavior, which is driven by `GET_CLASS()` on mobiles plus
`MOB_ROL_HAS_*` flags, nor mob spellcasting profiles.

Read `docs/guides/ADDING_NEW_RACE_GUIDE.md` alongside this document. The two
systems share the creation flow, unlock model, help workflow, and test
manifests, and a new class frequently needs new feats.

## Definition of done

A new class is complete only when all of the following are true:

- Its numeric ID is unique, stable, and inside every registry bound.
- `load_class_list()` produces a complete `class_list[]` entry.
- `class_names[]` and `class_short_descriptions[]` in `src/constants.c` have
  matching entries, and the `CHECK_TABLE_SIZE()` assertions still hold.
- `level_exp()` returns real experience for the class rather than the
  `123456` error value.
- Terminal creation, `gain`, `class list/info/feats/prereqs`, and web
  onboarding all list, accept, explain, and validate it.
- Its BAB, hit dice, saves, trains, class abilities, titles, feats, prestige
  prerequisites, and any spellcasting behave as designed.
- Its help topic exists in both the database migration and
  `lib/text/help/help.hlp`.
- A character survives save, disconnect, reload, level-up, and multiclassing
  without losing class levels or class features.
- Automated tests, the full build, and the manual smoke matrix pass.

The active data flow is:

```text
src/structs.h CLASS_* id and NUM_CLASSES / MAX_CLASSES bounds
                  |
                  v
src/character/class.c load_class_list() ---> class_list[]
                  |                             |
                  |                             +--> classo() core stats
                  |                             +--> saves, class abilities, titles
                  |                             +--> feat_assignment() grants
                  |                             +--> spell_assignment() spell levels
                  |                             +--> class_prereq_*() prestige gates
                  |
                  +--> src/constants.c name and short-description tables
                  +--> advance_level(), init_class(), level_exp()
                  +--> creation (interpreter.c), gain (act.other.c), study.c
                  +--> account unlocks, web onboarding, persistence
                  v
        help, tests, deployment, rollback
```

## 1. Write the class specification first

Record these decisions before editing code. They determine which downstream
systems must change.

| Decision | Required detail |
|----------|-----------------|
| Identity | Canonical `CLASS_*` constant and permanent numeric ID |
| Names | Registry name, abbreviation, colored abbreviation, menu name |
| Kind | Base or prestige; maximum class levels (`-1` for unlimited) |
| Availability | `in_game` flag, locked flag, account-XP unlock cost |
| Progression | BAB (`H`/`M`/`L`), hit dice, PSP gain, move gain, trains per level |
| Saves | Fortitude, reflex, will, poison, death as good (`G`) or bad (`B`) |
| Abilities | Class / cross-class / not-available for all 27 ability entries |
| Titles | Ten level-banded titles plus a default |
| Feats | Automatic grants by level, and eligible class-feat choices |
| Epic | `epic_feat_progression` interval and any epic-only grants |
| Casting | None, prepared, spontaneous, innate, or psionic; attribute; circles |
| Prestige gates | Attribute, class level, feat, ability, BAB, race, align, casting |
| Alignment | Any alignment restriction enforced by `valid_align_by_class()` |
| Presentation | `class-<slug>` help tag and `class/<slug>` web media key |
| Compatibility | Existing characters, multiclass interactions, unlock rows |

Decide casting early. A non-caster class touches roughly a dozen files; a
caster class additionally touches `src/magic/spell_prep.c`,
`src/magic/spell_parser.c`, `src/character/study.c`, `src/constants.c` slot
tables, and several `src/utils.h` macros.

## 2. Resolve the numeric ID before implementation

Class IDs are durable data, not reorderable enum positions:

- `src/players.c:2512` writes the primary class as `Clas: <id>`.
- `src/players.c:3560` writes the `CLvl:` block, whose lines are
  `<class-id> <level>` pairs read back by `load_class_level()`
  (`src/players.c:5037`).
- Account unlocks store the number as `unlocked_classes.class_id`
  (`src/account.c:821`, `src/db_init.c:203`).
- Per-class runtime arrays are indexed by ID: `class_level[MAX_CLASSES]`,
  `spec_abil[MAX_CLASSES]`, `class_feat_points[NUM_CLASSES]`,
  `preparation_queue[NUM_CLASSES]`, `known_spells[NUM_CLASSES]`, and
  `perk_points[NUM_CLASSES]` in `src/structs.h`.

Never renumber or reuse a released ID.

### Current allocation state

As of this guide's verification date, `src/structs.h:470-530` defines:

- `CLASS_UNDEFINED` as `-1` and real classes densely from `0`
  (`CLASS_WIZARD`) through `35` (`CLASS_ARTIFICER`).
- `CLASS_PLACEHOLDER_1` as `36` and `CLASS_PLACEHOLDER_2` as `37`.
- `NUM_CLASSES` and `MAX_CLASSES` both as `38`.
- `NUM_CASTERS` as `9`, a separate legacy bound used by the older
  `prep_queue`/`collection` arrays. It is not the class count.

### The two valid implementation paths

1. **Consume a placeholder slot.** `CLASS_PLACEHOLDER_1` and
   `CLASS_PLACEHOLDER_2` are registered as locked, not-in-game stubs at
   `src/character/class.c:9152`. Renaming one of them to the new class is the
   cheapest path because no bound changes. Verify first that no character or
   account row has ever stored `36` or `37`; the stubs are `in_game = N`, so
   in a normal database no rows exist.

2. **Append a new ID and raise the bounds.** Add `CLASS_NEW 38`, raise
   `NUM_CLASSES` and `MAX_CLASSES` to `39`, and then update every fixed table,
   loop, and test that uses those bounds. This is the correct path once both
   placeholders are consumed.

Raising `MAX_CLASSES` widens `char_player_data`/`player_special_data` arrays
and therefore changes in-memory layout. It does not change the ASCII player
file format, because both `Clas:` and `CLvl:` are written as decimal numbers,
but it does change every consumer that iterates the arrays. Treat it as a
reviewed change, not an incidental one.

Use these searches during the allocation review:

```bash
grep -n '^#define \(CLASS_\|NUM_CLASSES\|MAX_CLASSES\|NUM_CASTERS\)' src/structs.h
grep -rn '\b\(NUM_CLASSES\|MAX_CLASSES\|NUM_CASTERS\)\b' src unittests
grep -rn 'class_id\|CLvl\|Clas:' src sql docs
grep -rn 'class_list\[\|CLSLIST_' src --include='*.[ch]'
```

`MAX_UNLOCKED_CLASSES` is the number of unlock slots on an account. It is not a
maximum class ID and should not be changed just because a new class exists.

### Bounds that require special attention

If the ID or any class bound changes, inspect every search result. The heaviest
consumers are:

| Source | Why it matters |
|--------|----------------|
| `src/magic/spell_prep.c` | 28 bound-sensitive sites: queues, collections, known spells |
| `src/character/perks.c` | Per-class perk arrays and `associated_class` |
| `src/character/class.c` | Registry init loop, `init_spell_levels()`, displays |
| `src/act.informative.c` | Score, `whoami`, and class-level display loops |
| `src/players.c` | Class-level save/load loops |
| `src/obj/act.item.c` | Item and consumable class checks |
| `src/net/onboarding.c` | `class_media_keys[NUM_CLASSES]` and catalog bounds |
| `src/account.c` | Account-XP class listing and purchase loops |
| `src/act.other.c` | `do_gain` class enumeration and multiclass cap |
| `src/db.c`, `src/constants.c` | Boot-time tables and `CHECK_TABLE_SIZE()` |
| `src/mob/mob_spellslots.c` | NPC caster slot derivation |
| `src/olc/medit.c`, `src/olc/hlqedit.c` | Builder class pickers |

`CHECK_TABLE_SIZE()` (`src/constants.c:32`) is a compile-time assertion. If you
raise `NUM_CLASSES` without adding the matching `class_names[]` and
`class_short_descriptions[]` entries, the build fails immediately. That is the
intended safety net; do not silence it.

## 3. Add the registry constant and bounds

Add the permanent constant to the class block in `src/structs.h`, next to the
existing definitions:

```c
#define CLASS_NEW_CLASS 36 /* replaces CLASS_PLACEHOLDER_1 */
```

Do not:

- renumber existing `CLASS_*` values or their compatibility aliases
  (`CLASS_WEAPONMASTER`, `CLASS_PSION`, `CLASS_PALE_MASTER`, and similar);
- assume `NUM_CLASSES`, `MAX_CLASSES`, and `NUM_CASTERS` are interchangeable;
- change `src/campaign.h`, `src/mud_options.h`, or `src/vnums.h`.

Then add the display strings in `src/constants.c`:

- `class_names[]` at `src/constants.c:512`, at the exact ID position;
- `class_short_descriptions[]` at `src/constants.c:4456`, same position.

Both tables end with `"\n"` and are size-checked against `NUM_CLASSES + 1`.

## 4. Register the class in `load_class_list()`

`load_class_list()` at `src/character/class.c:3858` is the authoritative runtime
registry. It first calls `init_class_list()` for every ID, which installs safe
defaults (`"unusedclass"`, low BAB, `in_game = N`, all saves bad, all abilities
not-available). Then each class is declared in a fixed order that the file
documents: `classo`, saves, abilities, titles, free feats, class feats, spell
assignments, prerequisites.

```c
/*     class-number     name         abrv  clr-abrv    menu-name */
classo(CLASS_NEW_CLASS, "new class", "New", "\tcNew\tn", "v) \tcNew Class\tn",
       /* max-lvl lock? prestige? BAB HD psp move trains in-game? unlkCost efeatp */
       -1, N, N, M, 8, 0, 2, 4, Y, 0, 3,
       /* prestige spell progression */ "none",
       /* primary attributes */ "Dexterity, Constitution for survivability",
       /* description */
       "The character-creation and 'class info' description of the class.");

/* fortitude, reflex, will, poison, death */
assign_class_saves(CLASS_NEW_CLASS, G, B, B, B, B);

assign_class_abils(CLASS_NEW_CLASS,
                   /* acrobatics, stealth, perception, heal, intimidate, concentration, spellcraft */
                   CA, CC, CA, CC, CA, CC, CC,
                   /* appraise, discipline, total_defense, lore, ride, climb, sleight_of_hand, bluff */
                   CC, CA, CA, CA, CA, CA, CC, CC,
                   /* diplomacy, disable_device, disguise, escape_artist, handle_animal, sense_motive */
                   CC, CC, CC, CC, CA, CC,
                   /* survival, swim, use_magic_device, perform */
                   CA, CA, CC, CC);

assign_class_titles(CLASS_NEW_CLASS,
                    "",                    /* <= 4  */
                    "the Initiate",        /* <= 9  */
                    "the Adept",           /* <= 14 */
                    "the Veteran",         /* <= 19 */
                    "the Master",          /* <= 24 */
                    "the Grandmaster",     /* <= 29 */
                    "the Paragon",         /* <= 30 */
                    "the Immortal Exemplar", /* <= LVL_IMMORT */
                    "the Avatar",          /* <= LVL_STAFF */
                    "the God",             /* <= LVL_GRSTAFF */
                    "the New Class"        /* default */
);

/*              class num       feat              cfeat lvl stack */
feat_assignment(CLASS_NEW_CLASS, FEAT_SIMPLE_WEAPON_PROFICIENCY, Y, 1, N);
feat_assignment(CLASS_NEW_CLASS, FEAT_ARMOR_PROFICIENCY_LIGHT,   Y, 1, N);
/* eligible class-feat choices use NOASSIGN_FEAT */
feat_assignment(CLASS_NEW_CLASS, FEAT_DODGE, Y, NOASSIGN_FEAT, N);
```

The argument contracts are easy to misread:

- `classo()` (`src/character/class.c:325`): class number, name, abbreviation,
  colored abbreviation, menu name, `max_level`, `locked_class`,
  `prestige_class`, `base_attack_bonus`, `hit_dice`, `psp_gain`, `move_gain`,
  `trains_gain`, `in_game`, `unlock_cost`, `epic_feat_progression`, prestige
  spell progression string, primary attribute string, description.
- `assign_class_saves()` (`:378`): fortitude, reflex, will, poison, death.
- `assign_class_abils()` (`:390`): 26 positional values. Several parameters
  (`lore`, `survival`, `swim`) are marked unused and the function hardcodes
  `CA` for Arcana, Religion, History, Boarding, Survival, and Linguistics. Do
  not assume a value you pass for those is honoured; trace the function before
  relying on it.
- `assign_class_titles()` (`:358`): eleven strings in the banded order shown.
- `feat_assignment()` (`:312`): class, feat, `is_classfeat`, level received
  (`NOASSIGN_FEAT` = -1 means "eligible choice, not granted"), stacks.

The local single-letter macros are defined at the top of `class.c` and
`#undef`-ed at the bottom: `G`/`B` for good/bad saves, `Y`/`N`, `H`/`M`/`L` for
BAB, and `NA`/`CC`/`CA` for not-available, cross-class, and class ability.

Set every field explicitly. `in_game = N` keeps the class out of every menu
while it is being built, which is the correct state for work in progress; the
placeholder stubs use it today.

## 5. Wire experience, level-up, and initialization

### `level_exp()` is a required edit

`level_exp()` at `src/character/class.c:3738` switches on the character's class.
Every playable class is listed in one shared fall-through arm; the `default`
arm logs `SYSERR: Reached invalid class in class.c level()!` and returns
`123456`. A class missing from that switch cannot level normally. Add the new
`case CLASS_NEW_CLASS:` to the existing fall-through group unless the design
calls for a different curve.

Race XP multipliers are applied after the class switch in the same function.

### `advance_level()`

`advance_level()` at `src/character/class.c:3356` applies per-level gains. It
reads `CLSLIST_HPS`, `CLSLIST_MVS`, and `CLSLIST_TRAINS` from the registry, so
ordinary progression needs no code change. Add an explicit block only for a
mechanic the registry cannot express, such as the Psionicist PSP formula or the
Wizard bonus class feat every five levels. Both patterns are in that function
and are the models to copy.

Epic class feats derive from `CLSLIST_EFEATP(class)`; set the interval in
`classo()` rather than hardcoding it.

### `init_class()` and `init_start_char()`

`init_class()` at `src/character/class.c:2650` runs the first time a character
takes a level in the class. `newbieEquipment()` at `:2418` and the class switch
near `:2564` decide starting gear for level-one characters. Add a `case` there
only when a base class needs distinct starting equipment; prestige classes do
not.

`process_class_level_feats()` (`:3000`) and
`process_conditional_class_level_feats()` (`:3113`) apply registry feat grants
during level-up. The legacy `level_feats[][LEVEL_FEATS]` table at `:2066` is
deprecated as a grant path; it still feeds `feat info` display and staff fix
commands. Use `feat_assignment()` for grants, and only add a `level_feats`
entry when the display or a compatibility path demands it.

### Alignment restrictions

`valid_align_by_class()` at `src/character/class.c:1353` is the alignment gate
used by creation and by `valid_class_race_alignment()`
(`src/character/race.c:3223`). The final fall-through arm lists every class with
no alignment restriction. A class absent from the switch entirely falls to the
function's default behavior, so add the class to the unrestricted group
explicitly even when it has no restriction.

## 6. Wire class feats and mechanics

Prefer existing feats when they express the mechanic. If a class feature needs
a new feat, implement it end to end:

1. Add its stable `FEAT_*` constant in `src/character/feats.h`.
2. Register its metadata with `feato()` inside `assign_feats()` in
   `src/character/feats.c`.
3. Add class-level prerequisites with `feat_prereq_class_level()` where the
   feat should only be reachable from the new class.
4. Implement the behavior in the subsystem that owns the mechanic
   (`src/combat/fight.c`, `src/act.other.c`, `src/magic/*`, and so on).
5. Add focused CuTest coverage for the effect, not only for the constant.
6. Add player help in both the SQL component and `lib/text/help/help.hlp`.

If the mechanic needs a new C source file, add it to **both** `Makefile.am` and
`CMakeLists.txt`.

Class-identity predicates live in `src/utils.h` around lines 2211-2275
(`IS_WIZARD`, `IS_BARD`, `IS_ARTIFICER`, and so on). Add an `IS_NEW_CLASS(ch)`
macro when other systems need to test for the class, and audit the aggregate
predicates that must include it: `IS_CASTER` (`src/utils.h:2261`),
`IS_SPELLCASTER_CLASS` (`:1017`), `IS_FIGHTER`, and the BAB helper at `:1674`.
These aggregates are hand-maintained lists; nothing adds a class to them
automatically.

Per-class perks are registered in `src/character/perks.c` through
`perk->associated_class`. Perk trees are optional and can follow the initial
release.

## 7. Wire spellcasting, if the class casts

Skip this section for a non-caster class. For a caster, budget as much time
here as for everything else combined.

1. **Spell access.** Add `spell_assignment(CLASS_NEW_CLASS, SPELL_X, circle)`
   calls in the registry block. `init_spell_levels()`
   (`src/character/class.c:3707`) walks `spellassign_list` at boot and calls
   `spell_level()` for each entry; skills above `MAX_SPELLS` are defaulted to
   level 1 for all classes first.

2. **Circle progression.** Add a case to `get_class_highest_circle()`
   (`src/magic/spell_prep.c:2632`) and to `compute_spells_circle()` (`:2404`).

3. **Slots or known spells.** Add a case to `compute_slots_by_circle()`
   (`:2947`), pointing at a slot table in `src/constants.c` (see
   `wizard_slots` at `:3211`, `sorcerer_known` at `:3489`, `warlock_known` at
   `:3566`) and at the correct `spell_bonus[GET_<ATTR>(ch)][circle]` entry.

4. **Preparation model.** Prepared, spontaneous, innate, and psionic casters
   diverge in `known_spells_add()` (`:1081`),
   `count_known_spells_by_circle()` (`:1516`), `validate_spell_for_class()`
   (`:2022`), `start_prep_event()` (`:2814`), `stop_prep_event()` (`:2853`),
   and `spell_prep_gen_extract()` (`:3226`). Every one of those is a hand-
   maintained per-class switch or predicate.

5. **Caster level.** Update `compute_arcane_level()` and
   `compute_divine_level()` in `src/utils.c` (near lines 214-340), and the
   `CASTER_LEVEL` macro in `src/utils.h:985` if the class contributes its own
   term the way Warlock, Alchemist, and Artificer do.

6. **Casting entry points.** `src/magic/spell_parser.c` maps casting classes
   (see `:107`, `:699`, `:876`, `:2773`); `src/magic/casting_visuals.c` selects
   casting messages.

7. **Study menus.** `src/character/study.c` implements the per-class spell
   selection and known-spell screens. Spontaneous casters need an explicit menu
   path there; the Warlock and Sorcerer blocks are the models.

8. **Preparation dictionaries.** `spell_prep_dict[NUM_CLASSES][4]`
   (`src/constants.c:4254`) and `spell_consign_dict[NUM_CLASSES][4]`
   (`:4302`) are positional and size-checked. Add entries at the right index
   even for non-casters, or the build fails.

Prestige casting classes that advance an existing class instead of casting on
their own set `prestige_spell_progression` in `classo()` and add themselves to
`BONUS_CASTER_LEVEL` handling. Necromancer, Mystic Theurge, and Eldritch Knight
are the working examples; `src/utils.c:214-235` shows how a prestige class picks
the class it advances.

## 8. Wire prestige-class prerequisites

Prestige classes are gated by a prerequisite list built in the registry block.
The helpers are at `src/character/class.c:84-278`:

```c
class_prereq_attribute(CLASS_NEW_CLASS, AB_DEX, 13);
class_prereq_bab(CLASS_NEW_CLASS, 6);
class_prereq_ability(CLASS_NEW_CLASS, ABILITY_STEALTH, 8);
class_prereq_feat(CLASS_NEW_CLASS, FEAT_DODGE, 1);
class_prereq_cfeat(CLASS_NEW_CLASS, FEAT_WEAPON_FOCUS, CFEAT_SPECIAL_NONE);
class_prereq_class_level(CLASS_NEW_CLASS, CLASS_ROGUE, 5);
class_prereq_spellcasting(CLASS_NEW_CLASS, CASTING_TYPE_ARCANE, PREP_TYPE_ANY, 4);
class_prereq_race(CLASS_NEW_CLASS, RACE_ELF);
class_prereq_align(CLASS_NEW_CLASS, LAWFUL_GOOD);
class_prereq_weapon_proficiency(CLASS_NEW_CLASS);
```

`meets_class_prerequisite()` (`:518`) evaluates them, `class_is_available()`
(`:804`) applies the aggregate rule, and `display_class_prereqs()` (`:700`)
renders them for players. Note the aggregate semantics at `:868-885`: alignment
and race prerequisites are treated as "any one of" groups, while all other
prerequisite types must all be satisfied. Design the list accordingly.

A prestige class must set `prestige_class = Y` and normally `locked_class = Y`
in `classo()` so that creation rejects it at level 1
(`src/interpreter.c:8145-8153`).

## 9. Wire terminal character creation and `gain`

Base classes are selected during `nanny()` in `src/interpreter.c`:

1. The class menu enumeration at `src/interpreter.c:8025` filters on
   `CLSLIST_INGAME()`, `CLSLIST_LOCK()` with `has_unlocked_class()`, and
   `valid_class_race_alignment()`.
2. `CON_QCLASS` parses input through `parse_class_long()`
   (`src/character/class.c:1557`). Registry names are **not** parsed
   automatically: add an `is_abbrev(arg, "new class")` entry there, plus any
   intentional aliases. Note that the parser converts spaces to dashes before
   matching, and `do_gain` converts them back, so multi-word names must be
   handled consistently.
3. `CON_QCLASS` then dispatches class help through a switch at
   `src/interpreter.c:8166`; add `case CLASS_NEW_CLASS: perform_help(d, "class-new-class"); break;`.
4. `CON_QCLASS_HELP` (`src/character/character_creation.c:120`) must be able to
   return to the menu.

`parse_class()` (`src/character/class.c:1484`) is the legacy single-character
mapping used by `find_class_bitvector()` for `who`/`users` filters. It cannot
represent more than 32 classes and most modern classes are absent from it. Add a
letter only if a traced consumer needs one and a free letter exists.

`get_class_by_name()` (`src/utils.c:4900`) does abbreviation matching over
`CLSLIST_NAME()` and needs no edit, but a new name that is an abbreviation
prefix of an existing one will shadow it. Check for collisions.

Levels after the first are taken with `gain` (`do_gain`,
`src/act.other.c:3254`). It enumerates classes with `display_all_classes()`,
resolves the name with `get_class_by_name()`, re-checks
`class_is_available()`, enforces the `MULTICAP` multiclass limit and
`CLSLIST_MAXLVL()`, and refuses to advance while unspent points remain. Any
class-pair exclusion (the Cleric/Inquisitor rule at `:3369` is the existing
example) must be added there.

The player-facing `class` command (`do_class`, `src/character/class.c:1284`)
provides `list`, `info`, `feats`, `prereqs`, and staff `staff` views. All of
them read the registry, so a correctly registered class appears automatically.

## 10. Wire account unlocks

`unlock_cost` and `locked_class` control the account-XP path:

- `locked_class = N`: available to anyone; `has_unlocked_class()` returns TRUE.
- `locked_class = Y` with `unlock_cost > 0`: the account must contain the ID in
  `account->classes[]`, backed by the `unlocked_classes` SQL table.

Verify all of these in `src/account.c`:

- `accexp class` lists it with the correct cost (`:434`, `:445`);
- `accexp class <name>` finds and purchases it;
- `save_account()` writes the ID to `unlocked_classes` (`:1218`);
- a new session reloads the unlock (`:821`);
- terminal and web creation both hide or disable it before purchase.

The account array uses `0` as its empty-slot sentinel, which is one more reason
never to reassign ID `0` (`CLASS_WIZARD`).

## 11. Wire web onboarding

The web catalog is server-owned in `src/net/onboarding.c`; the client must not
duplicate class eligibility rules.

1. Add a stable `class/<slug>` entry to `class_media_keys[NUM_CLASSES]`
   (`src/net/onboarding.c:361`) at the exact ID position. Prestige and
   not-in-game classes use `NULL`, which falls back to `class/fallback`
   (`:1886`).
2. Confirm `class_is_selectable()` (`:2073`) admits the class for the intended
   audience; it applies the same registry, unlock, and alignment rules as the
   terminal flow.
3. Extend the media-key assertions in
   `unittests/CuTest/test_web_onboarding.c:487-495`.
4. Run the class-catalog pagination and wire-budget tests; adding a class
   changes page counts and payload size.
5. Provision the matching art or fallback in the web-client asset repository.

## 12. Add player help in both maintained stores

Character creation calls an exact `class-<slug>` topic. Add that keyword in both
places required by the project:

1. **Database authority:** add an idempotent
   `sql/components/help_class_<slug>_entries.sql` and a read-only
   `verify_help_class_<slug>_entries.sql`. Use
   `sql/components/help_necromancer_entries.sql` as the working model; it
   covers a class, its feats, and its commands in one transactional migration.
2. **Flat-file mirror:** add the same topic to `lib/text/help/help.hlp` before
   the final `$~` marker, following the existing `CLASS-WIZARD` entry
   (`lib/text/help/help.hlp:4413`).

The flat-file entry follows this shape:

```text
NEW-CLASS CLASS-NEW-CLASS

New Class

Player-facing summary, hit die, BAB, saves, skill trains, class features by
level, spellcasting if any, prerequisites, unlock cost, and restrictions.
#0
```

For the SQL pair:

- use `INSERT ... ON DUPLICATE KEY UPDATE` for `help_entries`;
- assign deterministic keywords in `help_keywords`;
- verify the entry, `min_level`, required text, and keyword ownership;
- classify the migration as `apply` and the verifier as `skip` in
  `sql/components/ci_schema_manifest.txt`;
- add both SQL files to `EXTRA_DIST` in `Makefile.am`;
- apply the migration twice in an isolated development database to prove
  idempotency, then run the verifier;
- restart or reload help and test the exact `help class-<slug>` lookup in game.

Follow `docs/systems/HELP_SYSTEM.md` for the maintained SQL workflow. Never
change production help while developing the class.

Also update player-facing and developer documentation when the class changes the
advertised class list, unlock behavior, or multiclass rules, and add the new
guide entry to `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md` if you add
supporting documents.

## 13. Protect persistence and compatibility

The authoritative player record is the ASCII player file written by
`save_char()`:

- `Clas: <id>` is the primary class (`src/players.c:2512`).
- The `CLvl:` block holds one `<class-id> <level>` line per class with levels,
  terminated by `-1` (`src/players.c:3560`, `:5037`).
- Class-scoped spell data is written per class by
  `save_prep_queue_by_class()`, `save_innate_magic_by_class()`,
  `save_collection_by_class()`, and `save_known_spells_by_class()`
  (`src/magic/spell_prep.c:398-506`).

Before release:

1. Confirm no existing player file or `unlocked_classes` row uses the chosen ID.
2. Create a character of the new class, level it, quit, and reload; confirm
   class levels, feats, trains, and spell data survive.
3. Multiclass into and out of the class and reload again.
4. Save and reload an account that unlocked the class.
5. Verify staff inspection, `score`, account menus, and MSDP/web output do not
   index outside `class_list[]`.

If the class ID changes during development, migrate every development character
and unlock row before testing again. After public release, do not change the ID.

## 14. Add automated tests

At minimum, add assertions for:

- ID uniqueness, bounds, and `CHECK_TABLE_SIZE()` consistency for
  `class_names[]`, `class_short_descriptions[]`, `spell_prep_dict`, and
  `spell_consign_dict`;
- complete `class_list[CLASS_NEW_CLASS]` registration after
  `load_class_list()`, including saves, abilities, titles, and `in_game`;
- canonical name and alias parsing through `parse_class_long()` and
  `get_class_by_name()`;
- `level_exp()` returning a real curve rather than the `123456` error value at
  several levels;
- feat grants at each configured level, and class-feat eligibility;
- prestige prerequisites: a character who qualifies and one who does not;
- alignment validity through `valid_align_by_class()`;
- for casters: highest circle, slots or known spells per level, and a
  prepare/cast/extract round trip;
- multiclass interaction with `MULTICAP` and `CLSLIST_MAXLVL()`;
- a non-fallback web media key and presence in the web class catalog;
- player and account persistence round trips;
- database help migration idempotency and verifier success.

Prefer a focused `unittests/CuTest/test_class_<slug>.c` suite when the coverage
does not fit an existing production-linked suite. When adding that source file,
update all three test manifests:

- `cutest_SOURCES` in `Makefile.am`;
- `cutest_test_files` in `Makefile.am`;
- `CUTEST_TEST_SOURCES` in `CMakeLists.txt`.

Extend `unittests/CuTest/test_web_onboarding.c` for the presentation contract.
CuTest test functions must begin with `Test`; `make-tests.sh` regenerates
`AllTests.c`.

## 15. Build and run the end-to-end smoke test

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

1. Boot without class-table, feat-table, help, or bounds errors, and with no
   `SYSERR` lines mentioning class or level.
2. Confirm `class list`, `class info <name>`, `class feats <name>`, and
   `class prereqs <name>`.
3. Confirm the exact `help class-<slug>` topic comes from the database.
4. For a base class: create a character through the terminal flow with both a
   custom and a premade build, and through web onboarding.
5. For a prestige class: build a character that fails the prerequisites,
   confirm it is refused with a readable reason, then satisfy them and enter.
6. For a locked class: verify a fresh account cannot select it, purchase it
   with account XP, reconnect, and then select it.
7. Confirm starting hit points, moves, PSP, trains, saves, BAB, class
   abilities, and title.
8. `gain` several levels; confirm feat grants, class feat points, epic feat
   progression, and any per-level bonus fire at the configured levels.
9. For casters: study or prepare spells, cast at each accessible circle, rest,
   and confirm recovery.
10. Multiclass in both directions and confirm `MULTICAP`, class-level caps, and
    any exclusion rules.
11. Quit, reconnect, and verify class levels, feats, spells, trains, and title
    are unchanged.
12. Inspect the character through account, `score`, staff-stat, and MSDP/web
    surfaces enabled in the development environment.

Ollama, I3, and Discord are not expected to work on local development unless
they are specifically in scope. Their absence is not a class-test failure, but
code-level bounds and serialization tests still must pass.

## 16. Deployment and rollback

The class registry is built at boot, so a code deployment requires a controlled
restart.

Recommended order:

1. Back up player files and the account/help database tables.
2. Apply and verify the help SQL component in the target non-production stage.
3. Deploy the tested code and matching flat help file.
4. Restart and inspect the boot log.
5. Repeat the creation, `gain`, unlock, save/reload, and help smoke checks.
6. Only then schedule the production rollout through the normal approval path.

Rollback before anyone takes a level in the class can remove its selection and
code. Rollback after persisted class levels or unlocks exist must retain the
numeric ID and enough registry behavior to load those records safely: set
`in_game = N` to stop new selection while leaving the constant, the
`class_list[]` entry, the name tables, the parser, and `level_exp()` intact. Do
not renumber, reuse, or erase the ID.

## Known traps

- `level_exp()` silently returns `123456` and logs a `SYSERR` for a class not
  listed in its switch. This is the single most common omission.
- `prac_params[][NUM_CLASSES]` is declared `extern` in
  `src/character/class.h:147` but has no definition anywhere in the tree. Do
  not add entries to it; it is dead.
- `assign_class_abils()` ignores several of its named parameters and hardcodes
  `CA` for Arcana, Religion, History, Boarding, Survival, and Linguistics.
- `NUM_CASTERS` is `9` and indexes the legacy `prep_queue`/`collection` arrays.
  It is unrelated to how many classes cast in the current system.
- `IS_CASTER`, `IS_SPELLCASTER_CLASS`, and `IS_FIGHTER` are hand-maintained
  macro lists in `src/utils.h`. A registry entry does not update them.
- Alignment and race prerequisites are "any one of" groups inside
  `class_is_available()`; every other prerequisite type is required.
- `class_media_keys[]`, `class_names[]`, `class_short_descriptions[]`,
  `spell_prep_dict`, and `spell_consign_dict` are positional. A missing entry
  either fails `CHECK_TABLE_SIZE()` at compile time or silently shifts every
  later class.
- `parse_class()` and `find_class_bitvector()` cannot represent more than 32
  classes. Do not treat them as a general class parser.

## Source map

| Area | Authority |
|------|-----------|
| Class IDs and bounds | `src/structs.h:470-530` |
| Runtime class structure | `struct class_table` in `src/character/class.h` |
| Registry | `src/character/class.c`, `load_class_list()` |
| Registry helpers | `classo()`, `assign_class_saves()`, `assign_class_abils()`, `assign_class_titles()`, `feat_assignment()`, `spell_assignment()`, `class_prereq_*()` |
| Registry accessors | `CLSLIST_*` macros, `src/utils.h:2172-2204` |
| Name and description tables | `src/constants.c:512`, `src/constants.c:4456` |
| Experience and level-up | `level_exp()`, `advance_level()`, `init_class()` in `src/character/class.c` |
| Alignment gate | `valid_align_by_class()`, `valid_class_race_alignment()` |
| Prestige gates | `meets_class_prerequisite()`, `class_is_available()` |
| Terminal creation | `src/interpreter.c`, `CON_QCLASS` and `CON_QCLASS_HELP` |
| Level gain and multiclass | `do_gain()`, `src/act.other.c:3254` |
| Player commands | `do_class()`, `src/character/class.c:1284` |
| Feats | `src/character/feats.h`, `src/character/feats.c` |
| Spellcasting | `src/magic/spell_prep.c`, `src/magic/spell_parser.c`, `src/character/study.c` |
| Slot tables | `src/constants.c` (`wizard_slots`, `sorcerer_known`, `warlock_known`, and peers) |
| Account unlocks | `src/account.c`, `unlocked_classes` |
| Web catalog and media | `src/net/onboarding.c` |
| Character persistence | `src/players.c`, `Clas:` and `CLvl:` |
| Help architecture | `docs/systems/HELP_SYSTEM.md` |
| Flat help mirror | `lib/text/help/help.hlp` |
| SQL help authority | `sql/components/help_class_<slug>_entries.sql` |
| Web tests | `unittests/CuTest/test_web_onboarding.c` |
| Build and test procedure | `docs/guides/TESTING_GUIDE.md` |
| Related workflow | `docs/guides/ADDING_NEW_RACE_GUIDE.md` |

## Final review checklist

- [ ] Class specification is approved.
- [ ] Numeric ID is a free placeholder or an approved bound increase.
- [ ] No existing `CLASS_*` value or alias was renumbered.
- [ ] `class_names[]` and `class_short_descriptions[]` entries added at the
      correct index and `CHECK_TABLE_SIZE()` passes.
- [ ] `classo()`, saves, abilities, and titles are fully specified.
- [ ] Feat grants and class-feat eligibility are registered.
- [ ] `level_exp()` includes the class.
- [ ] `valid_align_by_class()` includes the class.
- [ ] Prestige prerequisites are complete and display correctly.
- [ ] `parse_class_long()` parses the canonical name and intended aliases.
- [ ] Terminal creation help dispatch calls the exact `class-<slug>` topic.
- [ ] `gain` lists, validates, and advances the class within caps.
- [ ] Spellcasting is wired through every per-class switch, or the class is a
      confirmed non-caster.
- [ ] `IS_CASTER`/`IS_SPELLCASTER_CLASS`/`IS_FIGHTER` aggregates are correct.
- [ ] Unlock purchase and reload work, or the class is intentionally unlocked.
- [ ] Web media key, catalog eligibility, and wire-budget tests pass.
- [ ] Database help migration and verifier are complete.
- [ ] Matching `lib/text/help/help.hlp` content is complete.
- [ ] Player and account save/reload tests pass, including multiclass.
- [ ] `make test` and `make install` pass.
- [ ] Local terminal and web smoke matrix passes.
- [ ] Deployment and post-persistence rollback plans preserve the numeric ID.
