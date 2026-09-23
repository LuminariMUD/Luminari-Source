# clang-tidy findings (issue #218)

Working plan for issue #218: steps 1-5 on branch `fix/218-clang-tidy-findings` (PR #226), steps
6-8 on `fix/218-clang-tidy-remaining`, stacked on it (worktree `../Luminari-Source-issue-218`).
Goal: remove the most findings per hour of work. Every batch lowers
`scripts/ci/clang_tidy_baseline.txt`. Delete this file when the issue closes.

## Measured starting point

A full run in this worktree at master `ac248dd16` (clang-tidy 22.1.8, 16 jobs, 152 s)
reproduces the issue: 5,215 findings. It was joined with master's CI coverage report (Build &
Test run 35772434600) to see what each fix costs under the coverage gate.

| Measurement | Consequence |
| -- | -- |
| 1,186 findings (23%) are spelled inside macros; 747 of them were assignment-in-if (`CREATE` alone 554) and went with D1. Others: `RECREATE` 23, `APPEND_TO_BUF` 18, `TEST_OBJS` 12, `USEC_PER_PULSE` 9. | One edit clears every expansion. Macro bodies are not executable lines, so these edits cost no changed-line coverage. |
| The build already enforces what two checks look for: `-Wall` includes `-Wparentheses`, which rejects an unparenthesized assignment in a condition, and `AM_CFLAGS` sets `-Wno-sign-conversion` on purpose. | Basis for decisions D1 and D2. |
| 284 of the 319 `bugprone-narrowing-conversions` are same-width, mostly the `unsigned int` vnum/rnum typedefs stored in `int`. With `WarnOnEquivalentBitWidth: false`, 35 remain (measured). | D2. |
| `bugprone-signed-char-misuse.CharTypdefsToIgnore: 'sbyte;byte'` silences none of the 51 (measured: the sites read struct members). | Fixed per site in step 3. |
| 242 findings are in `unittests/` and `util/`, which the coverage gate does not measure. | No coverage cost. |
| Changed-line floors cover only the 25 files of the 8 critical subsystems in `scripts/ci/coverage_policy.json`. 575 non-macro findings sit there (db.c 141, players.c 136, dg_scripts.c 67, objsave.c 46, genzon.c 40, comm.c 30), 255 of them on covered lines. The repository floor has about 8,800 uncovered lines of margin (32.37% against 31.36%). | Only edits in those 25 files need the coverage check below. |
| #216 plans edits to five files holding 149 of the findings (`crafting_new.c` 104). | Codemods are re-run after a rebase instead of resolving conflicts by hand. |

## Decisions (approved 2026-09-22)

Applied in step 1, each with the scope, reason, owner, and expiry entry `.clang-tidy` requires.

1. D1: disable `bugprone-assignment-in-if-condition` (-2,522). `-Wparentheses` already demands
   the explicit `if ((x = f()))` form, and the check cannot be told to accept it.
2. D2: `bugprone-narrowing-conversions.WarnOnEquivalentBitWidth: false` (-284): the same-width
   class the build allows with `-Wno-sign-conversion`.
3. D3: disable `clang-analyzer-optin.performance.Padding` (-43): reordering `char_data` and
   positionally initialized tables risks silent misinitialization for no measurable gain.

## Steps

| Step | Clears | Left | Effort |
| -- | -: | -: | -- |
| 1. Configuration (D1-D3) | 2,849 | 2,366 | 0.5 h |
| 2. Macros | 62 | 2,304 | 0.5 h |
| 3. Defect review: issue groups 1 and 2, narrowing leftovers | 233 | 2,071 | 3 h (1.5 h with 3 lanes) |
| 4. Codemods: switch defaults, widening casts, pointer casts | 1,218 | 853 | 3 h |
| 5. PR 1 |  |  | 1 h |
| 6. Review lanes: branch clones, dead stores | 556 | 297 | 4 h (2 h with 4 lanes) |
| 7. `sscanf` family | 297 | 0 | 5-6 h |
| 8. PR 2 |  |  | 1 h |

84% of the findings are gone after steps 1-5, about 6.5 hours with lanes.

1. Configuration. Done: `.clang-tidy` carries D1-D3, the four
   `NOLINTNEXTLINE(bugprone-assignment-in-if-condition)` comments on `CREATE` calls are gone (the
   gate rejects a NOLINT naming a disabled check), and the baseline records 2,366.
2. Macros. Done (2,304 left), one edit per macro:
   - `RECREATE` (`core/utils.h`, also used by `APPEND_TO_BUF` in `olc/hedit.c`): realloc into
     a temporary and keep the abort on failure (-40 realloc, -1 pointer conversion).
   - `TEST_OBJS` (three copies in `obj/objsave.c`): compare the `strcmp()` result with 0 (-12).
   - `USEC_PER_PULSE` (`core/perfmon.c`): its nine floating uses take the new
     `USEC_PER_PULSE_F` (-9); the values are unchanged at 10 pulses a second.
3. Defect review. Done (2,071 left). Three agents on disjoint directories, the lead on
   `protocol.c` (mutation score 32.47%, floor 32.07). Of the 233 sites, 191 were fixed and 42
   are suppressed with a named NOLINT and reason; in the critical files a suppression was used
   only on uncovered lines that are not defects. Real defects fixed: an NPC's short
   description freed twice in `free_char()`; uninitialized affects (bleeding, crippling
   strike, pressure point, blinding shield, poison touch, dog charm, Menzoberranzan chokers,
   which also stacked hitroll and could not be removed); bomb commands checking `ACTION_*`
   bitmasks where `is_action_available()` takes `atSTANDARD`/`atMOVE`/`atSWIFT`; an Inferno
   Bomb `$t` message reading an integer as a string; a direction passed to `act()` as an
   object pointer in `do_drive`; `USE_FULL_ROUND_ACTION` and `USE_MOVE_ACTION` running
   unbraced (silent trap detection and immortals' apply-poison spent actions); score width
   160 stored in a signed byte and never applied; `getenv()` strings used after `setenv()`;
   zero-byte allocations (board configs, drink names); leaks in `isname_tok()`,
   `load_clans()`, `zmalloc()`, and touch of corruption; `errno` lost across `close()` in
   `fopen_restricted()`; a device-creation spell count without a range check. Reviewing
   next to the flagged sites also found trailing-space overruns in `do_homelands()`,
   `handle_region_help()`, and `handle_background_help()` and a heap overflow in
   `transform_voice_to_observational()`; all four are fixed. Ten regression tests (CuTest
   1,744/1,744).
   Found but not changed (outside this issue, filed as #227): every toggleable perk ID is 256
   or more, so
   the perk toggles ignore them (Defensive Stance and Immovable Object never apply, alchemist
   mutagen/catalyst toggles do not stick); `score_display_width` should be `ubyte`; the
   chokers' affect tag equals `SPELL_IRON_GUTS`; shutdown-only leaks in `ibt.c`,
   `mysql_boards.c`, and `free_clan_list()`; an unused octave cache in `perlin.c`.
4. Codemods. Done (877 left). Scripts in `tmp/218/codemods/` (gitignored) read a fresh
   `clang-tidy-report.json` and edit at the reported line and column; clang-format lays out
   the result. Regenerate the report between codemods: each one shifts lines or columns.
   - `bugprone-switch-missing-default-case`: 619 `default: break;` (a `break;` first where
     the last case could fall through; `account.c` skipped, authentication's floor has no
     room). An empty last case joins the default instead, since `case X: break; default: break;` is a new `bugprone-branch-clone` finding (15 sites), and the turn undead switch's
     last case is braced for `-Wjump-misses-init`.
   - `bugprone-implicit-widening-of-multiplication-result`: 455 casts to the destination type
     before the left operand, then a second pass of 34 for products spelled inside macros
     (`SECS_PER_MUD_DAY`) that the first pass exposed. Statement macros take the cast inside
     their body (`NODE_ADVANCE` in test_gameplay_e2e.c).
   - `bugprone-multi-level-implicit-pointer-conversion`: 143 casts; the three
     `CuAssertPtrEquals` sites cast their arguments, since the conversion is in the macro.
   - `tmp/218/codemods/coverage_trim.py` backed out the 24 unexecuted codemod hunks (26
     findings) in olc, persistence, sql, and the world/DG/config parsers, leaving each at least
     ten points above its changed-line floor. `--update` never raises a count, so the baseline
     was restored from the step 3 commit and recorded again.
5. PR 1. Done: #226 ("Part of #218"), rebased onto master `e33ed0d6f`. Master's player-file
   test rewrite had left two dead stores in `test_gameplay_e2e.c` above its baseline; they are
   removed on the branch. The first matrix run caught two storm cases that the label merge had
   let fall through into the new default (clang's `-Wimplicit-fallthrough`; gcc is silent), now
   fixed, and every unit then compiled warning-free under clang-22 with the CMake warning tier.
   All 33 `run.py` jobs pass on `beb3b3622` (1,406 s), all GitHub checks pass, and CodeQL
   reports no alerts on the pull request ref (master has none open).
6. Review lanes. Done (324 left), four agents on disjoint directories as in step 3.
   `bugprone-branch-clone` (326) and `clang-analyzer-deadcode.DeadStores` (227) are cleared in
   125 files: identical switch arms share one body, adjacent identical `if` arms share one
   condition, unread stores are gone. About a hundred clones stay behind a named NOLINT where
   the arms are separate rules that coincide (treasure.c level tiers, per-class values, ordered
   first-match ladders). Seven sites were defects and are fixed: `get_number()` ("2x.sword"
   picked the second sword), `do_activate()` and `use_wand()` (object spells cast with no
   object), `get_spell_circle()` (three-quarter and half caster mobs read low spells as circle 5
   or 3), `list_spells()` (paladin and ranger lists printed empty circles), `savingthrow_full()`
   (Indomitable Will overwrote the save), and bestow power in `mag_points()` (returned before its
   messages); three regression tests. Four more found beside the sites are fixed with tests in a
   separate commit: Bomb Mastery raised the thrower's saved alchemist level by 5 per spell bomb,
   `show_mote_bonuses()` read `ability_names[j]` with `j` unset, the class feat point loaders
   looped forever on a truncated player file, and `mclanwar`/`mclanally` reread their first
   argument as the status. The rest of what the lanes noticed is filed as #228.
7. String-to-number parsing. Done (27 left). Instead of an integer-only helper,
   `src/core/strict_scan.c` adds `strict_sscanf()` and `strict_fscanf()`: the same formats and
   results as scanf (every directive the tree uses, one pushback character for streams, the
   format attribute), with each number converted by `strto*` and one outside its type treated
   as a matching failure. A differential run against glibc over every format in the tree (79,695
   generated and mutated inputs, string and stream) matched except on overflow. The 272 calls in
   `src/` and `unittests/` are renamed; `util/` links the module for four tools and uses `strtol`
   for its `atoi`/`atol`. `Test_strict_scan_matches_scanf_and_stops_at_overflow` compares both
   entry points with libc.
8. PR 2 ("Closes #218"). The 27 findings left were the codemod sites step 4 held back for
   coverage: they are applied again (17 defaults, 6 pointer casts, 4 widening casts), and the
   baseline file is now empty, so any finding fails the gate. `do_accexp`'s knight alias switch
   admitted only the classes its cases named, so its default could never run; it is one
   condition per class instead. Ten tests execute the changed lines the coverage gate prices
   (accexp knight alias, exits prompt, protocol report, copyover recovery, script editor, zone
   index insertion, tokenize, random region point against temporary region tables, zone
   command forms, `wait until`). Priced against #226, every critical subsystem clears its
   changed-line floor by at least five points.

## Rules for every batch

1. `pre-commit run clang-format --files <changed files>` before committing; the hook aborts a
   commit that it reformats. Never commit while a background build or test reads the tree.
2. `make -j16` with no new warnings, then `make install`.
3. Build and run the full CuTest suite (command under Tooling).
4. Full gate with `--update`; commit the code and the baseline together.
5. Edits in the 25 critical files: run
   `python3 scripts/ci/local/run.py --job test-coverage --jobs 1 --cpus 16 --results <dir>`
   (about 2 min), then
   `python3 scripts/ci/check_coverage.py --report <dir>/test-coverage/coverage.xml --base origin/master`,
   which prices the whole branch as the pull request will. Keep each subsystem's changed-line
   coverage at least one point above its floor, since GitHub's runner measured a line or two
   differently on #203. If a subsystem falls short, take the mechanical edits on its uncovered
   lines back out (codemods take an exclusion list) and record their findings again, instead
   of writing tests for untested parsers. Defect fixes keep their regression tests. Step 8 is
   the exception: it leaves no findings, so it tests the lines instead. `run.py` tests
   committed `HEAD` (`git archive`), so commit before running it.
6. Add tests to existing test files, which avoids manifest changes. Stage `src/core` paths with
   `git add -u` or `-f`.

## Tooling

```bash
# Done once in this worktree; there is no plain `clang` on this host.
cmake --preset analysis -DCMAKE_C_COMPILER=clang-22
# Full gate (152 s); its JSON report feeds the codemods.
python3 scripts/ci/check_clang_tidy.py --build-dir build/analysis --report-dir tmp/218/tidy-full
python3 scripts/ci/check_clang_tidy.py --build-dir build/analysis --update
# One check over all 427 units in about 6 s, for codemod iteration. It may count a few more
# than the gate (for example bsd-snprintf.c, compiled under another target's command).
python3 -c "import json; print('\n'.join(json.load(open('tmp/218/tidy-full/clang-tidy-report.json'))['translation_units']))" |
  xargs -P16 -I{} clang-tidy -p build/analysis --quiet --checks='-*,CHECK' {} 2>/dev/null |
  grep 'warning:.*\[CHECK\]' | sort -u | wc -l
# Full suite.
make -j16 cutest && CUTEST_FILTER= LUMINARI_TEST_ROOT="$PWD" \
  LUMINARI_TEST_SPEC_WORLD_ROOT="$PWD/unittests/CuTest/fixtures/spec_world_inventory" ./cutest
```

`tmp/218/` holds the latest full report and log (`tidy-full/`, after step 1), the original run
at `ac248dd16` (`tidy-ac248dd16/`), master's `coverage.xml` with a per-line index
(`cov-master/lines.json`), and `load.py`, which joins findings with macro notes, coverage, and
subsystems. Rerun the full gate before any codemod: line numbers go stale with every edit.

## Ablation

Removed while planning: a coverage predictor (the real coverage job takes 2 minutes), the local
matrix after every batch (once per PR), and new tests for mechanical edits on untested lines
(those edits come back out instead). Kept two pull requests rather than one, so the defect
fixes land early and the branch is exposed to #216 and other parallel work for less time.

## Resume here

All steps are done: #226 carries steps 1-5, and the pull request from
`fix/218-clang-tidy-remaining` carries steps 6-8 and closes #218; the tree has no findings.
When #226 merges, rebase the second branch onto master
(`git rebase --onto origin/master origin/fix/218-clang-tidy-findings`) and retarget it. Found
defects that change gameplay are #227 and #228.

## Progress log

- 2026-09-22: full run at `ac248dd16` reproduced 5,215 findings; plan written.
- 2026-09-22: D1-D3 approved; step 1 committed (5,215 -> 2,366).
- 2026-09-22: step 2 committed (2,304); build warning-free, CuTest 1,734/1,734.
- 2026-09-23: step 3 committed (2,071); build warning-free, CuTest 1,744/1,744, protocol
  parser harness 32/32, new tests clean under valgrind.
- 2026-09-23: step 4 committed (877); build warning-free, CuTest 1,744/1,744, coverage policy
  passes locally.
- 2026-09-23: rebased onto master `e33ed0d6f`; PR #226 opened; local matrix 33/33, GitHub checks
  and CodeQL clean.
- 2026-09-23: step 6 committed (324); seven defects fixed. Four more defects fixed beside its
  sites; the rest filed as #228.
- 2026-09-23: step 7 committed (27); `strict_sscanf()`/`strict_fscanf()` with a differential
  check against glibc.
- 2026-09-23: held-back codemod sites applied (0 findings, empty baseline); ten coverage tests,
  CuTest 1,763/1,763 with the test database, coverage policy passes against #226.
