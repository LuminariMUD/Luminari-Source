# Static analysis remediation (issue #213)

Working reference for issue #213 on branch `fix/213-static-analysis-remediation`
(worktree `../Luminari-Source-issue-213`). A new session should be able to continue from this
file alone. Delete it when the issue closes.

## Scope

The issue's own action items, nothing wider:

1. Resolve the 11 open CodeQL alerts (6 `cpp/type-confusion`, 5 `cpp/path-injection`).
2. Resolve PR #214 (dependabot bump of `github/codeql-action` 4.38.0 -> 4.38.1).
3. clang-tidy: fix the high-priority analyzer checks `clang-analyzer-core.NullDereference`,
   `clang-analyzer-security.ArrayBound`, `clang-analyzer-core.NonNullParamChecker`.
4. clang-tidy: convert unsafe string functions (`bugprone-unsafe-functions`: `sprintf`,
   `strcpy`, `strcat`, `vsprintf`, `rewind`) to bounded forms.
5. clang-tidy: convert unchecked string-to-number conversions
   (`bugprone-unchecked-string-to-number-conversion`: `atoi`, `atol`, `sscanf`, ...).
6. Record every lower count with `--update` as each step lands.

Other checks in the baseline (`assignment-in-if-condition`, `switch-missing-default-case`,
`branch-clone`, ...) are out of scope.

## Tooling

```bash
# Once per worktree (clang-22 is the pinned 22.1.8; there is no plain `clang` on this host):
cmake --preset analysis -DCMAKE_C_COMPILER=clang-22
# Full run, about 170 s with 16 jobs; writes clang-tidy-report.json:
python3 scripts/ci/check_clang_tidy.py --build-dir build/analysis --report-dir <dir>
# Incremental against master (what CI runs on a PR):
python3 scripts/ci/check_clang_tidy.py --build-dir build/analysis --base origin/master
# Record lower counts (whole tree):
python3 scripts/ci/check_clang_tidy.py --build-dir build/analysis --update
# One file:
clang-tidy -p build/analysis --config-file=.clang-tidy --quiet src/<dir>/<file>.c
```

A verified false positive is silenced with `/* NOLINTNEXTLINE(<check>) -- <reason> */`; the gate
rejects suppressions without a check name and a reason.

Local CodeQL (verified steps 2-3 before pushing): the release bundle is unpacked in the session
scratchpad (`codeql-bundle-linux64.tar.gz` from `github/codeql-action` releases; the `gh codeql`
2.25.6 CLI cannot download current packs). Build a database from a `git archive` copy with the
`.example.h` headers copied into place and `./configure`, then:
`codeql database create <db> --language=cpp --command='make -j16'` and
`codeql database analyze <db> <pack>/Security/CWE/CWE-843/TypeConfusion.ql <pack>/Security/CWE/CWE-022/TaintedPath.ql --format=sarif-latest --output=<file>`.
Result on the step 2-3 tree: no type-confusion results, and path-injection only in `util/`
(alerts dismissed long ago, outside the issue).

CodeQL results for master: `gh api -H 'Accept: application/sarif+json' repos/LuminariMUD/Luminari-Source/code-scanning/analyses/<id>` (ids from
`code-scanning/analyses?ref=refs/heads/master`) returns the SARIF with code flows.

## Findings at start (master aeb9f3dda, 2026-09-22)

7,523 findings (baseline file records 7,684; 31 entries were already lower than recorded).

| Check | Count |
| -- | -- |
| clang-analyzer-core.NullDereference | 180 (56 in `character/study.c`; 90 in tests) |
| clang-analyzer-security.ArrayBound | 65 |
| clang-analyzer-core.NonNullParamChecker | 45 (41 in tests) |
| bugprone-unsafe-functions | 273 (46 in `movement/asciimap.c`, 42 in `dgscript/dg_variables.c`) |
| bugprone-unchecked-string-to-number-conversion | 2,008 (1,640 atoi, 275 sscanf) |

## CodeQL analysis

All 11 open alerts re-open earlier false-positive dismissals (#361-#365, #379-#384) whose
fingerprints changed when nearby code moved. Code fixes, not new dismissals, so they stay closed.

- `cpp/type-confusion` (comm.c `perform_act`, #889-#894): the query reports an explicit cast of
  `vict_obj` to `struct obj_data *` when a `char_data` allocation reaches `act()` and no
  `obj_data` allocation does (it cannot read the `$` format contract). Flagged sources:
  `act.wizard.set.c` `$N`/`$n` messages, `vendor.c:784`, `spec_rol_combat.c:2518`. Fix: one
  typed view per interpretation, taken by C's implicit `void *` conversion; the query only
  inspects explicit casts.
- `cpp/path-injection` (#886-#888, #896, #897): the query's only barriers are calls that
  return an arithmetic type and variables with an upper-bound comparison; it does not model
  boolean validators such as `is_safe_path_component()`. Fix: the validators rebuild the
  accepted path one checked character at a time, so the opened name is the validator's output,
  not the raw input. Sites: index file entries in `index_boot()` (db.c, two loops), the `-f`
  configuration file (`load_config`), and the log file (`setup_log`).

## Steps

- [x] 1. PR #214: merged (rebase) as `2bc76e731`; branch fast-forwarded onto it.
- [x] 2. CodeQL type confusion: typed `vict_obj` views in `perform_act`; flagged callers
  (`act.wizard.set.c` homeland messages, `vendor.c` pet purchase, `spec_rol_combat.c` imp
  summon) all pass a character with `$N`.
- [x] 3. CodeQL path injection: `build_safe_path()` (utils.c) replaces
  `is_safe_path_component()` and `is_safe_relative_path()`; used by both `index_boot()`
  loops, the `-f` configuration file, and `setup_log()` (log names may be absolute but not
  traverse). `Test_path_component_validation` covers it.
- [x] 4. NullDereference / ArrayBound / NonNullParamChecker: zero left tree-wide (commits
  `49ad4de40`, `94f3b3ae2`, `8c84e3965` for src, `dd77ba152` for tests, baseline `d5af939e7`).
  About 40 were real defects (listed in the commit messages). Recurring false positives,
  each fixed with a NOLINT and reason rather than contorted code: `buf[strcspn(...)]` after
  `fgets`, `isspace()` on an `unsigned char`, `buf[fread(...)]`, and heap arrays sized by
  `CREATE()` (its `number * sizeof <= 0` check lets the analyzer assume a zero-byte block;
  changing the macro surfaced a new gcc `-Walloc-size-larger-than` warning, so it stays).
  `MIN`/`MAX` are out-of-line functions in utils.c, so the analyzer cannot bound their
  results; clamp explicitly where an index depends on them. Test findings were CuTest
  asserts the analyzer could not see end the test: `CuFail_Line()` now carries
  `analyzer_noreturn` and the null/true asserts call it directly.
- [x] 5. Unsafe string functions: zero left (commits `529003390` for the helpers whose
  signatures changed, `2196fd8cd` for local conversions). New `rewind_stream()` in utils.c
  replaces `rewind()`; `sprintbitarray()`, `one_phrase()`, and `zedit_get_levels()` take
  buffer sizes. `clang_tidy_unsafe_sites.txt` is now empty, so any new unbounded call fails
  the gate.
- [ ] 6. String-to-number conversions: IN PROGRESS; the conversion is committed and pushed
  (see "Resume here" for what remains). Design: the issue's item says `atoi/atol -> strtol`, so every
  `atoi`, `atol`, `atoll`, and `atof` call in `src/` becomes a strtol-family helper;
  `sscanf`/`fscanf` numeric parsing (252 sites in src, multi-field world/DB loaders) is not
  converted, because it has no drop-in bounded form and the item does not name it.
  - New in `src/core/utils.c`/`.h` (next to `snprintf_append`): `parse_int()`,
    `parse_long()`, `parse_llong()`, `parse_double()`. Same reading as libc (leading
    space, sign, digits); NULL or no digits gives 0; out of range saturates
    (`parse_int` clamps to `INT_MIN`/`INT_MAX`) instead of undefined behaviour.
  - 1,702 calls in 113 `src/` files renamed mechanically (comment lines skipped).
  - `src/net/protocol.c` (2 sites) and `src/net/onboarding.c` (1) use `strtol()` directly:
    the protocol parser harness (`unittests/CuTest/Makefile`) links them without utils.c.
  - `src/vessels/vessels_autopilot.c` gained `#include "core/utils.h"`, placed after
    `<math.h>` because utils.h defines `log` as a macro.
  - Verified: `make` warning-free, full CuTest 1723/1723, protocol harness 31/31.
  - `Test_parse_number_helpers` (test_bounds_checking.c) covers the helpers and passes.
  - Not yet: the clang-tidy gate run and `--update`.
- [ ] 7. `--update` baselines, rebase onto current `origin/master` (it moved to `9a7ece8c2`
  and later), local CI jobs for the touched paths, push, open the PR (`Closes #213`),
  confirm CodeQL on the PR shows the 11 alerts fixed and nothing new in changed code, then
  delete this file in the final commit (move anything enduring to `docs/`).

## Resume here

State when the session stopped (2026-09-22): everything is committed and pushed, including
the step 6 conversion (helpers commit first, then the renames by directory). Not done for
step 6: the clang-tidy gate run and `--update`.

1. `git fetch` and confirm the worktree is clean at the pushed branch head.
2. Build and run the helper test:
   `make -j16 cutest && CUTEST_FILTER=Test_parse_number_helpers LUMINARI_TEST_ROOT=$PWD ./cutest`.
3. Format: `pre-commit run clang-format --files $(git diff --name-only -- '*.c' '*.h')`.
4. Gate: `python3 scripts/ci/check_clang_tidy.py --build-dir build/analysis --base HEAD`
   (about 150 s). Fix anything above the baseline, then
   `python3 scripts/ci/check_clang_tidy.py --build-dir build/analysis --update`
   (expected: `bugprone-unchecked-string-to-number-conversion` drops by about 1,700).
5. Full suite: `CUTEST_FILTER= LUMINARI_TEST_ROOT="$PWD" LUMINARI_TEST_SPEC_WORLD_ROOT="$PWD/unittests/CuTest/fixtures/spec_world_inventory" ./cutest`
   (1723+ tests, about a minute), and `make -C unittests/CuTest protocol-parser`.
6. Commit the baseline update and this file's progress entry. `git add` paths under `src/core` need
   `git add -u` or `-f` (a core-dump ignore pattern matches them).
7. Step 7 above. `make install` after every push (the pre-push hook leaves a root
   `luminari` binary).

Workflow notes learned here:

- Pre-commit hooks reformat staged C files and abort the commit; format first (step 3).
- To split one file's changes across commits, stage hunks with `git apply --cached` on a
  filtered `git diff` (no interactive `git add -p` in this environment); check the staged
  tree builds by applying `git diff --cached` to a `git archive HEAD` copy and running make.
- The local CodeQL bundle lived in the session scratchpad and is gone; re-download
  `codeql-bundle-linux64.tar.gz` from the latest `github/codeql-action` release to rerun
  the check described under Tooling.

## Progress log

- 2026-09-22: branch at master aeb9f3dda; full clang-tidy run recorded above; plan written.
- 2026-09-22: steps 1-3 done and committed; step 4 done and pushed through `d5af939e7`
  (full CuTest suite 1723/1723).
- 2026-09-22: step 5 done (baseline 7,223 -> 6,926); step 6 next.
- 2026-09-22: step 6 conversion committed and pushed; session stopped. Continue at
  "Resume here".
