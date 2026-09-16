# Craft trainers: plan for issue #196

Tracking issue: #196. Branch: `feat/196-craft-trainers`. Written 2026-09-16 from a trace of
`master` at `9c5a0223f89ceda8b15a7bb97c2110f64188f50e`; line numbers refer to that revision.
World-data facts come from the development world copy, which git does not track.

Status: in progress. On 2026-09-16 the owner accepted every decision at the end as final, so no
open question blocks implementation. Placing the trainer in the production world stays with the
owner's world-data release (step 5).

## Progress

Update this list with every commit, so a new session can resume from it.

- [x] Step 1: craft experience and ranks, plus the talent storage fix from Finding 12.
- [x] Step 2: contract rules and record.
- [ ] Step 3: SpecProc and command.
- [ ] Step 4: lock, settlement, recall, and menu row.
- [ ] Step 5: placement.
- [ ] Step 6: help and documentation.
- [ ] Step 7: verification.

Working notes for this worktree (`../Luminari-Source-issue-196`):

- `lib/.env` has `APP_ENV=development`; world data and the `lib/world/*/index` files are already
  copied, so the full suite boots the world.
- Build with `make -j16 luminari cutest` (about 40 seconds after a `structs.h` change). Run the suite
  the way `make run-cutest` does:
  `CUTEST_FILTER= LUMINARI_TEST_ROOT="$PWD" LUMINARI_TEST_SPEC_WORLD_ROOT="$PWD/unittests/CuTest/fixtures/spec_world_inventory" ./cutest`
  (1,516 tests after step 2, about 5 seconds). Use `CUTEST_FILTER=Test_craft_` for this feature.
- The pre-push hook runs `make`; run `make install` afterwards so no root-level `luminari` binary
  is left behind.

## Outcome

A player pays a Craft Trainer NPC to advance one of the 12 active craft or harvest skills. The
character leaves play with its belongings saved and cannot re-enter before the contract ends. The
first time the character is selected for play after the end time, it receives a fixed share of one
rank's experience, exactly once. Recalling early forfeits the fee and the experience.

All acceptance criteria in the issue stand. The trace found a live defect that the required
multi-rank fix would make worse (Finding 1), so step 1 fixes it before any trainer code lands.

## Findings from the trace

The issue's description of the code checks out, including the brewing bypass
(`src/craft/brew.c:180`, `:205`, `:369`) and the single rank check in `gain_craft_exp()`
(`src/craft/crafting_new.c:5175`). The findings below are new or change the proposal.

01. **Respec erases craft ranks but keeps their experience and talent points.**
    `init_start_char()` (`src/character/class.c:2696`) runs on every `do_start()`: the free respec
    (`respec_engine()`, `src/act/act.other.c:3322`, level 2 and up), the study reset
    (`src/character/study.c:3231`), and staff demotion (`src/act/act.wizard.c:2323`). Its loop at
    `:2837` zeroes abilities 1 to `NUM_ABILITIES` (51), which includes the craft (34-46) and harvest
    (48-51) ranks, while `ability_exp[]` and `talent_points` survive. The comment at `:2833` and
    `HELP RESPEC` (`lib/text/help/help.hlp:28394`) both say crafting skills are kept. After a
    respec, each `gain_craft_exp()` call restores one rank and awards another talent point, so
    talent points can be farmed today. A gain that awards every rank reached would pay all of them
    at once, and a respecced character would break "a contract never raises the skill more than one
    rank".

02. **`load_char()` is the wrong place to settle.** `show_account_menu()` loads every account
    character twice (`src/player/account.c:1271`, `:1302`), the web lobby loads each card
    (`src/net/onboarding.c:2265`), and `finger`, clan, and staff file commands load characters for
    inspection. A grant there would either be lost or be saved from a display path. The only load
    that leads to play is the account-menu selection (`src/core/interpreter.c:8096`).

03. **The way out of play is `perform_player_quit()`, not the receptionist.** `free_rent` defaults
    to `YES` (`src/config/config.c:121`), and with free rent `gen_receptionist()` only replies "Rent
    is free here. Just quit" (`src/obj/objsave.c:1845`). `perform_player_quit()`
    (`src/act/act.other.c:5658`) saves pets, dismisses followers, ends timed quests, saves belongings
    with `Crash_rentsave(ch, 0)` only when rent is free, sets the load room to the current room, and
    extracts. `Crash_rentsave()` (`src/obj/objsave.c:1513`) removes the objects it saves, so a
    second call writes an empty object file.

04. **Starting a contract leaves the player at the character menu.** `extract_char_final()`
    (`src/core/handler.c:3177`) saves the character (`:3389`), clears its events (`:3399`), and
    moves the descriptor to `CON_MENU` (`:3260`) with the same in-memory character. Menu option 1
    (`src/core/interpreter.c:9980`) calls `enter_player_game()` (`:10004`) without passing the
    account menu, and the forced short-description setup (`src/character/char_descs.c:1015`) starts
    from that option. A check at the account menu alone would lock nothing.

05. **Copyover can restore a character that has just started a contract.** The game loop runs
    every descriptor's command (`src/core/comm.c:1665`) before `extract_pending_chars()` (`:1745`).
    A `copyover` later in the same pass still sees the trainee as playing (the writer keeps only
    playing descriptors, `src/act/act.wizard.c:5615`), and `copyover_recover()` returns them to play
    through `enter_player_game()` (`src/core/comm.c:714`). No other path enters play:
    `perform_dupe_check()` (`src/core/interpreter.c:7452`) only reattaches to a body still in
    `character_list`, which the pending extraction removes in the same pass, and `CON_PASSWORD` and
    `CON_GET_NAME` never enter play for an existing character.

06. **Only a fresh load may be saved from the menus.** `CON_RMOTD` already saves a character
    loaded at the account menu (`src/core/interpreter.c:9701`). The post-extraction copy in
    `CON_MENU` has no events, and option 0 deliberately does not save it (`:9976`). Settlement and
    recall therefore run at the account menu, on a fresh load.

07. **Gold lives only in player files.** `Gold:` and `Bank:` are player-file tags
    (`src/player/players.c:2930`); `player_data` has no gold column (`sql/master_schema.sql:31`).
    Any later retuning against production data means a read-only pass over production player
    files, not a SQL query.

08. **World content is not in git.** `lib/world/{mob,wld,zon,...}` are gitignored and edited per
    site through OLC; authored additions ship as `data/<bundle>/` records with install instructions
    (`data/harvest-tools/README.md`). In the development world, Sanctus III (zone 3) has a crafting
    district: passage 368, the supply-order office 370 (Ambah, mob 305, with neither the
    quartermaster flag nor a SpecProc), materials vendor 369, mold shops 371, 376, and 377, and
    Crafting Benches 372, 373, and 375. Mob vnum 373 is unused, and room 373 is a dead end.

09. **The web lobby uses the same handler.** The browser sends the terminal's wire values to
    `nanny()`, so the lock needs no protocol work. The lobby card (`build_account_characters()`,
    `src/net/onboarding.c:2240`) has no status field; adding one is a paired gateway change
    (`docs/systems/WEB_ONBOARDING_SYSTEM.md`, change rule 09). Protocol v2 is compile-time
    default-off.

10. **Several lists count SpecProcs.** `unittests/CuTest/test_spec_registry_validation.c` (`:189`
    and the table ending at `:485`), `test_spec_registry_persistence.c` (`:550`),
    `test_spec_owner_aware_olc.c` (mobile names from `:28`), and
    `scripts/world/tests/test_constants.py` (`:211`). `docs/guides/OLC_SpecProcs.md` still says 54
    mobile, 34 object, and 17 room entries; the tests list 61, 41, and 20.

11. **Most admission rules already exist.** Legacy craft and brew events block other commands
    (`src/core/interpreter.c:6883`), `primary_activity_command_admit()` (`:6921`) runs before
    SpecProcs (`:6992`), and a standing-position command refuses fighting positions. The trainer
    adds only `primary_activity_snapshot()` (`src/events/activity_manager.h:184`) and an explicit
    `FIGHTING()` check.

12. **Harvest talents could not hold a rank.** Found while implementing step 1. Talent ids run to
    94 (`TALENT_MAX` 95, `src/character/talents.h`), but `talent_ranks` held 64 slots
    (`src/core/structs.h:6747`), `GET_TALENT_RANK()` and `SET_TALENT()` ignored ids above 63, and
    `Tlrk` saved and loaded 64 entries. `learn_talent()` still charged points and gold and wrote
    past the array for ids 64 to 94: every mining, hunting, forestry, and gathering talent and the
    mote synergy talents. Those talents never applied, the insightful bonus that decision 3
    extends to training grants could not reach the four harvest tracks, and staff `talent set`
    reported success while changing nothing. `talents.h` also defined an unused `MAX_TALENTS`
    (256). Rank storage now has `MAX_TALENTS` (128) slots defined in `structs.h`, `talents.c`
    asserts that `TALENT_MAX` fits, and `Tlrk` writes one entry per talent id (at most 385
    characters, inside the 512-byte `READ_SIZE` line) and reads up to 128, so older 64-entry saves
    still load. Points and gold already spent on these talents before the fix are not refunded.

## Design

### Tunables

All constants live in `src/craft/craft_training.h`; none is a runtime setting. These values are
final (decision 1).

| Tunable | Value | Reason |
| -- | -- | -- |
| Duration | 24 hours of wall-clock time | Issue proposal |
| Grant | Half of the next rank's requirement: 500 x (rank + 1) | A meaningful share of one rank |
| Rank ceiling | A contract starts only below rank 20 | Leaves the top of each track to active play and bounds what gold alone buys |
| Fee | 100 x (rank + 1)^2 gold, from gold on hand | Gold per experience point rises with rank |
| Insightful talent | Applies, through `gain_craft_exp()` | No second grant path |
| Cooldown | None beyond one contract per character | The ceiling already bounds a permanently training alt |

| Rank | Next rank needs | Grant | Grant with +25% | Fee | Gold per point |
| -: | -: | -: | -: | -: | -: |
| 0 | 1,000 | 500 | 625 | 100 | 0.2 |
| 4 | 5,000 | 2,500 | 3,125 | 2,500 | 1.0 |
| 9 | 10,000 | 5,000 | 6,250 | 10,000 | 2.0 |
| 14 | 15,000 | 7,500 | 9,375 | 22,500 | 3.0 |
| 19 | 20,000 | 10,000 | 12,500 | 40,000 | 4.0 |

What these values guarantee:

- **At most one rank per contract.** The largest grant, with the maximum insightful bonus, is 62.5
  percent of the next rank's requirement. Experience starts below the next threshold, and the gap
  to the threshold after it is larger still, so while the rank matches the experience (step 1
  makes that hold) a grant crosses at most one threshold. A table test keeps this true for every
  rank below the ceiling, so no runtime clamp is needed.
- **Well below active play.** At rank 9 a character can attempt items up to level 19 (the check
  refuses when 20 + rank is below 10 + item level), and one 60-second create of such an item grants
  950 experience, so the rank-9 grant equals about five creates. At rank 19 it equals about seven. A
  day away buys minutes of materials-limited crafting.
- **Bounded spending.** Training from rank 0 to the ceiling takes at most 40 daily contracts and
  574,000 gold; from rank 10 it takes at most 20 contracts and 497,000 gold. Then the trainer
  refuses.

### Contract record

- Three fields in `struct crafting_data_info` (`src/core/structs.h:5885`), beside the
  supply-order timestamps: `training_ability` (0 means no contract), `training_exp`, and
  `training_end` (`time_t`).
- Player-file tag `CrTr: <ability> <experience> <end epoch>`, written only while a contract exists,
  like `SuCD` (`src/player/players.c:3318`). Loading accepts an ability from
  `START_CRAFT_ABILITIES` to `END_HARVEST_ABILITIES` with positive experience and end time, so a
  paid contract survives a later eligibility change; anything else logs `SYSERR` and is ignored.
- The fee is not stored. The `mudlog` line at the start records it for staff.

### Trainer and command

- Command entry `apprentice` with `do_not_here` and a standing minimum position, right after
  `applies` (`src/core/interpreter.c:399`), the same pattern as `rent` (`:3947`). Away from a
  trainer it answers "Sorry, but you cannot do that here!". Abbreviations resolve in table order, so
  `ap` and `app` still mean `applies` and `appr` is the shortest form.
- `SPECIAL(craft_trainer)` in the new `src/craft/craft_training.c`, registered as `Craft Trainer`:
  mobile owner, command events, world binding only, builder visible, category `Crafting`. It goes
  in canonical order after `Buy Weapons`, before `Cryogenicist` (`src/spec/spec_registry.c:350`).
- `apprentice` lists the 12 tracks with rank, grant, fee, and the reason any track is unavailable.
  `apprentice <skill>` quotes and changes nothing; the quote says that leaving works like `quit`
  (followers dismissed, timed quests end) and gives the confirm syntax.
  `apprentice <skill> confirm` starts the contract.
- Skill names come from `ability_names[]` (`src/core/constants.c:3176`), matched with `is_abbrev()`
  against the tracks that `crafting_skill_type()` marks as craft or harvest.
- Admission order: a player with a descriptor; a trainer that is awake and can see the player (the
  receptionist's checks); an eligible skill; a rank below the ceiling; no recorded contract
  (defensive); not `FIGHTING()`; no primary activity; gold on hand that covers the fee.

### Starting a contract

1. Deduct the fee with `award_gold(ch, -fee)`, as `learn_talent()` does
   (`src/character/talents.c:654`), and set the record with `end = time(0) + duration`.
2. Call `save_char_checked(ch, 0)`. On failure, refund, clear the record, tell the player, log
   `SYSERR`, and stop.
3. Send the trainer and room messages, then `mudlog` the skill, rank, grant, fee, and end time.
4. `if (!CONFIG_FREE_RENT) Crash_rentsave(ch, 0);` then `perform_player_quit(ch)`. The quit sets
   the load room to the trainer's room and saves belongings itself when rent is free, so objects
   are saved exactly once either way.

### Entry lock

- `CON_MENU` option 1 (`src/core/interpreter.c:9980`): the first statement refuses while
  `training_ability` is set and tells the player to return to the account menu with 0. This also
  covers the forced description path.
- `copyover_recover()` (`src/core/comm.c`): a new `else if` between the lost-character branch and
  the entry branch (`:709`) writes a one-line explanation with `write_to_descriptor()`, as the
  lost-character branch does (`:706`), and calls `close_socket()`, so `enter_player_game()` (`:714`)
  never runs. Only the same-pass window of Finding 5 reaches it.
- Neither check settles or saves.

### Settlement

`craft_training_admit_selection(d, now)` runs in `CON_ACCOUNT_MENU` after the wizlock check
(`src/core/interpreter.c:8110`) and before `AddRecentPlayer()` (`:8120`):

- No contract: continue.
- Before the end time: print the skill, the time left, and the recall syntax, redisplay the account
  menu, and stop.
- At or after the end time: call `gain_craft_exp(ch, training_exp, training_ability, TRUE)`, clear
  the record, call `save_char_checked(ch, 0)` once, print a completion line, and continue to the
  MOTD.

Exactly once: the grant and the cleared record reach the file in the same write. If the write
fails, the in-memory result stands and the `CON_RMOTD` save retries. If nothing is ever written, the
file still holds the unsettled contract and the old experience, so the next selection grants it for
the first time.

### Recall

At the account menu, `recall <number>` shows the contract and warns that the fee and experience are
forfeit. `recall <number> confirm` loads the character fresh, clears the record, saves with
`save_char_checked()`, and redisplays the menu. It is a new `R` case ahead of the numeric default in
the menu switch (`src/core/interpreter.c:8049`). Nothing is refunded.

### Account menu row

`show_account_menu()` already loads each character (`src/player/account.c:1302`). While a contract
exists, the class column shows `training, 13h 20m left` or `training finished`. One pure formatter,
`craft_training_status()`, produces that text and the refusal messages.

## Implementation sequence

Each step builds and passes the full suite. Step 1 can ship on its own; steps 2 to 6 ship together,
because a trainer without the lock and settlement would take fees and never pay out.

### Step 1: craft experience and ranks (prerequisite)

Done. As built:

- `craft_skill_rank_for_exp(ch, exp)` in `src/craft/crafting_new.c`, declared in
  `crafting_new.h`.
- Brewing's `craft_skill_to_ability()` is gone: its only real mapping was brewing to alchemy, and
  `do_brew()` now reads `ABILITY_CRAFT_ALCHEMY` directly. The failure branches keep no experience
  message of their own, because `gain_craft_exp()` prints the amount.
- `reset_training_points()` (`src/character/study.c`) also clears abilities 1 to 51, but nothing
  calls it (its only call is commented out), so it was left alone.
- The talent storage fix from Finding 12.
- `unittests/CuTest/test_craft_training.c` holds six tests, and each of the five behavior tests was
  checked to fail against the old code: `Test_craft_rank_for_exp_counts_cumulative_thresholds`,
  `Test_craft_gain_crossing_three_thresholds_raises_every_rank`,
  `Test_craft_brewing_failures_raise_alchemy_with_insight` (seeds `circle_srandom()` so both the
  natural-1 and the ordinary failure branch run), `Test_craft_respec_keeps_craft_and_harvest_ranks`
  (through the real `do_start()`), `Test_craft_load_restores_rank_its_experience_earned`, and
  `Test_craft_harvest_talent_ranks_apply_and_persist`. The file has an isolated player-directory
  helper (`craft_player_files_enter()` and `craft_player_files_leave()`) for later steps.

Plan as written:

- `craft_skill_rank_for_exp()` beside `craft_skill_level_exp()` (`src/craft/crafting_new.c:6497`):
  the highest rank whose cumulative requirement is met, capped at `UCHAR_MAX` because
  `SET_ABILITY()` stores a `ubyte` (`src/core/utils.h:1615`).
- `gain_craft_exp()` (`:5135`): after adding experience, raise the rank to
  `craft_skill_rank_for_exp()` when that is higher, send one message from the old to the new rank,
  and call `gain_talent_point(ch, new_rank - old_rank)`.
- `src/craft/brew.c:180`, `:205`, `:369`: call
  `gain_craft_exp(ch, amount, ABILITY_CRAFT_ALCHEMY, TRUE)` and drop brewing's own amount messages;
  keep the critical-success line without the number.
- `init_start_char()` (`src/character/class.c:2837`): reset abilities 1 to `END_GENERAL_ABILITIES`
  only.
- `load_char()` (`src/player/players.c`, next to the ability normalization at `:2146` and before the
  immortal override at `:2181`): raise any craft or harvest rank that is below the rank its
  experience earned. No talent points: they were paid when the rank was first reached (decision 6).
- Tests in the new `unittests/CuTest/test_craft_training.c`, added to `cutest_SOURCES` and
  `cutest_test_files` in `Makefile.am` and to `CUTEST_TEST_SOURCES` in `CMakeLists.txt`:
  - rank lookup at 999, 1,000, 2,999, and 3,000 experience, and the cap;
  - one gain that crosses three thresholds raises three ranks and awards three talent points;
  - `event_brewing()` (`src/craft/brew.c:61`) with a synthetic payload whose DC cannot be met,
    starting one point below a threshold, raises alchemy one rank and adds an insightful bonus on
    either failure branch;
  - `do_start()` keeps craft and harvest ranks, their experience, and talent points, and clears
    general skills;
  - a player file whose `AbXP` is above its `Ablt` rank loads at the earned rank with unchanged
    talent points, in an isolated player directory (copy `enter_player_fixture()` and
    `leave_player_fixture()` from `unittests/CuTest/test_gameplay_e2e.c:98`).

### Step 2: contract rules and record

Done. As built:

- `src/craft/craft_training.h` declares `craft_training_track_eligible()`,
  `craft_training_grant()`, `craft_training_fee()`, and `craft_training_status()`, with the
  tunables `CRAFT_TRAINING_DURATION`, `CRAFT_TRAINING_RANK_CEILING`, and `CRAFT_TRAINING_FEE_BASE`.
  The grant and fee take only the rank. There is no separate seconds-left function: the status
  formatter is its only consumer, and it rounds minutes up so a running contract never reads
  "0m left".
- `training_ability`, `training_exp`, and `training_end` sit after the supply-order timestamps in
  `struct crafting_data_info`. `CrTr` is written just before `CrSR` in `save_char_checked()` and
  parsed with the other `Cr` tags in `load_char()`.
- Tests: `Test_craft_training_contract_survives_save_and_load` (also proves that clearing the
  record removes the line), `Test_craft_training_ignores_malformed_contracts`,
  `Test_craft_training_grant_never_crosses_two_ranks` (every trainable track and rank below the
  ceiling, with every talent at its highest rank, through `gain_craft_exp()`),
  `Test_craft_training_fee_rises_with_rank` (also checks the 574,000 gold total), and
  `Test_craft_training_status_counts_down_to_finished`.

Plan as written:

- `src/craft/craft_training.h` (self-contained, tunables as macros) and
  `src/craft/craft_training.c`, added to `Makefile.am` and `CMakeLists.txt`; run
  `python3 scripts/ci/check_build_parity.py`.
- Pure functions: track eligibility, the grant for a rank (half of
  `craft_skill_level_exp(ch, rank + 1) - craft_skill_level_exp(ch, rank)`), the fee for a rank,
  seconds left, and the status formatter.
- The record fields and the `CrTr` save and load.
- Tests: a contract survives `save_char_checked()` and `load_char()`; there is no `CrTr` line
  without a contract; malformed lines are ignored; for every rank below the ceiling, the grant
  times 1.25 stays below the next rank's requirement; the fee rises with rank.

### Step 3: SpecProc and command

- The registry entry, the command entry, `SPECIAL(craft_trainer)`, and the list updates from
  Finding 10.
- Tests, with a trainer bound through `find_spec_func_by_name("Craft Trainer")` and commands sent
  through `command_interpreter()`:
  - listing and quoting change neither gold nor state;
  - refusals: an ineligible skill (bowmaking), a rank at the ceiling, too little gold, fighting, an
    active primary activity, and a sleeping trainer;
  - confirm, in the isolated player directory: the fee is deducted, the contract is in the
    reloaded file, the load room is the trainer's room, `PLR_NOTDEADYET` is set, and a carried
    object is in the saved object file; repeat with `CONFIG_FREE_RENT` off to prove belongings are
    saved once.

### Step 4: lock, settlement, recall, and menu row

- The changes described under Design.
- Tests:
  - `nanny()` in `CON_MENU` with input `1` for a contracted character keeps the state at
    `CON_MENU` and the character out of `character_list`;
  - selection before the end time refuses, saves nothing, and grants nothing;
  - selection after the end time grants exactly the contract (plus the insightful bonus when
    ranked), raises the rank by at most one, and clears the record in the reloaded file; freeing,
    reloading, and selecting again grants nothing;
  - recall clears the record in the file and leaves experience and gold unchanged;
  - copyover, through the real `copyover_recover()`: in the isolated player directory, save a
    contracted character, open a `socketpair()`, and write `copyover.dat` with a boot time, one
    line in the writer's format (`<fd> <pref> <name> <host> 80/24`, `src/act/act.wizard.c:5752`),
    and `-1`. After the call, the peer socket holds the explanation and not "Copyover recovery
    complete", the character never joined `character_list`, `descriptor_list` is unchanged, and
    `copyover.dat` is gone. Save and restore `boot_time`. Write the file exactly: a missing file or
    an unreadable first line makes `copyover_recover()` exit the process (`src/core/comm.c:616`,
    `:629`). The refusal path never reaches `enter_player_game()`, so the test needs no world or
    database;
  - the account menu row through the existing `LUMINARI_TEST_MYSQL_*` harness
    (`unittests/CuTest/test_database_persistence.c:85`) with a temporary `player_data` table,
    because `show_account_menu()` queries it.

### Step 5: placement

- `data/craft-trainers/3.mob`: mob 373, a master artisan with `SpecProc: Craft Trainer` and the
  flags of the district's service NPCs (sentinel, uncharmable, unsummonable, unkillable, does not
  fight).
- `data/craft-trainers/README.md`: confirm vnum 373 is free, merge the record into
  `lib/world/mob/3.mob` in vnum order, and add `M 0 373 1 373 100 (the master artisan)` to
  `lib/world/zon/3.zon`; or build it in game with `medit 373` (Z, SpecProc) and `zedit`. Like
  `data/harvest-tools`, the bundle has no installer script and no build-list entry.
- `python3 scripts/world/wtool.py validate --paths data/craft-trainers/3.mob --strict`.
- Install it in the development world, start `MUD_PORT=4100 ./scripts/autorun/autorun.sh`, walk to
  Sanctus room 373, and check the list, quote, start, the lock at both menus, the account menu row,
  and recall. To see settlement live, move the `CrTr` end time into the past in the development
  player file while the character is offline.
- Production placement belongs to the owner's world-data release and is not part of this branch.

### Step 6: help and documentation

- `lib/text/help/help.hlp`: an `APPRENTICE CRAFT-TRAINER CRAFT-TRAINING` entry (commands, rules,
  lock, account-menu recall) and a `See also` from `CRAFT-SCORE`. `HELP RESPEC` already promises
  that crafting skills are kept; step 1 makes that true.
- `sql/components/help_craft_training_entries.sql` in the `help_pet_entries.sql` pattern, an
  `apply` line in `sql/components/ci_schema_manifest.txt`, and the file in the `Makefile.am` SQL
  list. Apply it to the development database and confirm the entry matches `help.hlp`.
- Builder help: a Craft Trainer sentence in `sql/components/help_specproc_entries.sql` and the
  matching `help.hlp` entry.
- `docs/guides/OLC_SpecProcs.md`: a Craft Trainer paragraph, and editor counts of 62 mobile, 41
  object, and 20 room entries.
- `docs/systems/CRAFT_ACTIVITY_LIFECYCLE.md`: training contracts use wall-clock time like supply
  offers and settle at selection.
- `docs/systems/SAVE_SYSTEMS_BREAKDOWN.md`: the `CrTr` tag.

### Step 7: verification

```sh
make -j"$(nproc)" test && make install
python3 scripts/ci/check_build_parity.py
make test-world-tools
pre-commit run --files <changed files>
```

Run the database-backed tests against the development database, then repeat the step 5 live check
on the final build.

## Ablation record

Removed or simplified:

- Receptionist mechanics: free rent short-circuits them, and `perform_player_quit()` already
  handles pets, followers, quests, belongings, and the load room (Finding 3).
- Settling on any load: `load_char()` serves displays and inspections (Finding 2), so settlement
  happens only at account-menu selection.
- A runtime one-rank clamp: the tunables rule it out, and a table test keeps it that way.
- A new connection state for recall: a typed account-menu command with `confirm`, like the
  trainer's own command.
- A stored fee, a staff command, and a runtime configuration surface: none serves a stated
  requirement.
- An installer script for placement: the change is one record and one reset line.
- A web protocol change: see decision 4.

Added, each for a traced failure:

- Keeping craft ranks through respec and restoring lagging ranks on load (Finding 1). Without them
  the required multi-rank gain pays duplicate talent points at once, and the one-rank bound fails
  for respecced characters.
- The copyover check (Finding 5), a real same-pass path into play.
- Talent rank storage for every talent id (Finding 12). Without it decision 3 fails for the four
  harvest tracks, and `learn_talent()` keeps charging for talents that write out of bounds.
- `Crash_rentsave()` when rent is not free (Finding 3). Otherwise a site that charges rent leaves the
  trainee's belongings on the floor of the trainer's room.

Kept from the issue: a named SpecProc instead of a mob flag, a wall-clock end time settled lazily,
no timers or scans, one contract per character, and the out-of-scope list.

## Decisions

The owner accepted these defaults as final on 2026-09-16.

1. **Tunables:** the table under Design (24 hours, half of the next rank's requirement, contracts
   only below rank 20, a fee of 100 x (rank + 1)^2 gold). They are constants in one header, so
   retuning later changes one file.
2. **Cooldown or weekly cap:** none. The rank ceiling bounds an always-training alt.
3. **Insightful bonus on trainer experience:** applies.
4. **Web lobby:** no protocol change. The lock still holds for web players, but the structured lobby
   cannot say why: selecting a training character just redraws the lobby, and the time left and
   the recall command appear only in the classic terminal, which the player has to open. Showing
   them in the lobby would take a `trainingSecondsLeft` card field and the paired gateway change.
5. **Placement:** one trainer, in Sanctus room 373. Trainer tiers and specialties stay out of
   scope.
6. **Talent points when restoring lagging ranks on load:** none. A character whose rank lags only
   because one gain crossed two thresholds loses the one talent point its next gain would have
   paid; paying instead would reward respecced characters again for ranks they were already paid
   for.

## Out of scope

As in the issue: languages, general skills, feats, ability scores, trainer tiers or specialties, an
in-game busy state, and runtime timers or scans.
