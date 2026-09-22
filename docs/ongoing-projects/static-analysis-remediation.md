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
- [ ] 5. Unsafe string functions: every site to `snprintf`/`strlcpy`/`strlcat`; `rewind` to
  a checked `fseek`.
- [ ] 6. String-to-number conversions (design recorded here before starting).
- [ ] 7. `--update` baselines, local CI jobs for the touched paths, push, PR, confirm CodeQL
  on the PR shows no open alerts in changed code.

## Progress log

- 2026-09-22: branch at master aeb9f3dda; full clang-tidy run recorded above; plan written.
- 2026-09-22: steps 1-3 done and committed; step 4 done and pushed through `d5af939e7`
  (full CuTest suite 1723/1723); step 5 next.
