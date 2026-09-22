# clang-tidy findings (issue #218)

Working plan for issue #218 on branch `fix/218-clang-tidy-findings` (worktree
`../Luminari-Source-issue-218`). Goal: remove the most findings per hour of work. Every batch
lowers `scripts/ci/clang_tidy_baseline.txt`. Delete this file when the issue closes.

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

1. Configuration. Done, not yet committed: `.clang-tidy` carries D1-D3, the four
   `NOLINTNEXTLINE(bugprone-assignment-in-if-condition)` comments on `CREATE` calls are gone (the
   gate rejects a NOLINT naming a disabled check), and the baseline records 2,366.
2. Macros, one edit per macro:
   - `RECREATE` (`core/utils.h`) and `APPEND_TO_BUF` (`olc/hedit.c`): realloc into a temporary
     and keep the abort on failure (-40 realloc, -1 pointer conversion).
   - `TEST_OBJS` (three copies in `obj/objsave.c`): compare the `strcmp()` result with 0 (-12).
   - `USEC_PER_PULSE` (`core/perfmon.c`): a floating-point form for its nine floating uses (-9).
3. Defect review. The issue's groups 1 and 2 minus what step 2 cleared (191), plus
   `performance-no-int-to-ptr` (4), `portability-avoid-pragma-once` (3), and the 35 narrowing
   leftovers (int to char is where `char c = getc()` truncation hides). Fix real defects and
   give each one on a testable path a regression test; silence a confirmed false positive with
   `/* NOLINTNEXTLINE(check) -- reason */`. Known leads:
   - `comm.c:493-497`: copy both `getenv()` strings before `setenv()` (real, latent on glibc).
   - `unix.Malloc`: use after free at `clan.c:4376`, `ibt.c:151`, `movement_tracks.c:363/366`;
     double free at `db.c:7160`; six leaks.
   - `uninitialized.Assign` (6): `SET_BIT_AR` on arrays that were never zeroed.
   - False-positive shapes: `rand() % count` over non-empty literal tables in
     `narrative_weaver.c` (9), eight deliberate out-of-range enum casts in tests, and `calloc`
     sized through `CREATE` (10; changing its zero check raised a gcc
     `-Walloc-size-larger-than` warning in #213).
   - `roleplay.c` (25 missing commas): read each flagged pair, then one
     `NOLINTBEGIN`/`NOLINTEND(bugprone-suspicious-missing-comma)` pair around the four sentence
     tables (lines 889-1479).
   - `bugprone-suspicious-string-compare` (10 left): `clang-tidy --fix` with only that check
     applies its ` != 0` fix-its (verified); `act.social.c:157` by hand.
   - Lanes: three agents on disjoint directories (act, core, olc / obj, combat, magic, spec /
     the rest), each checking its files with per-file clang-tidy only. The lead builds, tests,
     and records. Write the step 4 codemods while the lanes run.
4. Codemods. Python scripts in `tmp/218/` (gitignored) read a fresh `clang-tidy-report.json`
   and edit at the reported line and column; clang-format then lays out the result. One commit
   per codemod; after a rebase, drop the commit and run the codemod again.
   - `bugprone-switch-missing-default-case` (620): add `default:` and `break;` before the
     closing brace, first adding `break;` to a last case that does not end in `break`,
     `return`, `continue`, or `goto` (`-Wimplicit-fallthrough` in the build catches a miss).
     The check skips enum switches, so `-Wswitch` protection is unchanged.
   - `bugprone-implicit-widening-of-multiplication-result` (455, 95 in tests): insert `(T)` at
     the reported column, `T` being the destination type the message names (`long`, `size_t`,
     `time_t`, ...). Before an operand it widens the multiplication; before a macro that
     expands to a product (`PULSE_VIOLENCE`) it makes the conversion explicit.
   - `bugprone-multi-level-implicit-pointer-conversion` (143): insert the cast the message
     names, `(void *)` for `free()` of a `T **` and `(T **)` for the reverse. `-Wcast-qual`
     flags a cast that drops a qualifier.
   - Leave `src/net/protocol.c` (12 findings) alone unless
     `python3 scripts/ci/mutation_test.py --module src/net/protocol.c` still passes: its score
     is 0.01 above its floor.
5. PR 1 ("Part of #218"). Rebase onto `origin/master`; on a baseline conflict take master's
   file and run `--update` again. Run the coverage check below and
   `run.py --job quality-clang-tidy --jobs 1 --cpus 16` (some findings appear only with the
   container's glibc macros), then the full local matrix once (`run.py --jobs 4 --cpus 4`,
   about 13 min). After pushing, compare CodeQL alerts with master: moved code re-opened
   dismissed alerts in #213.
6. Review lanes, four agents on disjoint directories as in step 3.
   - `bugprone-branch-clone` (329): merge the case labels of the 127 "switch has N consecutive
     identical branches" (a script can propose the merges; review each, since the check exists
     to catch copy-paste slips). The 200 repeated arms in `if`/`else if` chains: combine the
     conditions, or suppress where separate arms document separate rules.
   - `clang-analyzer-deadcode.DeadStores` (227): delete the store, or keep the call and drop
     the variable. A value that should have been read is a defect: fix it and test it.
7. String-to-number parsing (297): 275 `sscanf`, 8 `fscanf`, 14 `atoi`/`atol` in `util/`. 203
   formats are integer-only (173 plain `%d`, some with literal separators such as `"%d-%d"`;
   30 with length modifiers), 6 add floats, 71 mix in `%s`, `%c`, or `%[`. 172 sit in critical
   files (players.c 100, db.c 41, objsave.c 21). Add one strict helper beside `parse_int()` in
   `src/core/utils.c` for the integer-only subset (whitespace, literal characters, `%d`, `%ld`,
   `%u`), parsing each field with `strtol` and treating overflow as a failed parse. Convert the
   integer-only sites to it and the mixed ones by hand; `util/` calls `strtol` directly (it is
   linked separately). Well-formed input must parse as before. Test the helper in
   `test_bounds_checking.c` beside `Test_parse_number_helpers`.
8. PR 2 ("Closes #218"), as in step 5.

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
   of writing tests for untested parsers. Defect fixes keep their regression tests.
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

Step 1 is applied in the working tree and verified (full gate with `--update`: 2,366;
`check_baseline_ratchet.py --base origin/master`: no baseline grew; pre-commit hooks pass). It is
not committed. Next: commit step 1, then step 2.

## Progress log

- 2026-09-22: full run at `ac248dd16` reproduced 5,215 findings; plan written.
- 2026-09-22: D1-D3 approved; step 1 applied (5,215 -> 2,366).
