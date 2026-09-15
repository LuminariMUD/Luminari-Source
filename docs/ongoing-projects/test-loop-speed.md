# Test loop speed: measurements and plan

Measured 2026-09-13 in the `Luminari-Source-issue-169` worktree. Host: 16 cores,
47 GB RAM, WSL2 on ext4, gcc 13 with the default `-g -O2`. GitHub numbers come from
`test.yml` run 34765646351 on this branch. Tracking issue: #178.

Goal: keep every check the suite makes today, and make the loop fast at all three
levels: one edit on the host, `make test-all` on the host, and the full CI matrix
(locally in containers, and on GitHub).

## Bottom line

1. One test dominates the host suite. `Test_syntax_check_encounter_world_boots_and_cleans_up_once`
   takes 30.9 s of the 33.2 s `cutest` run; the other 1447 tests take 1.5 s combined.
   The child boots the full development world, and 30 s of that is the mob loader calling
   `affect_total()` once per E-spec line (397,085 calls) instead of once per mob (27,092).
   Moving that call to once per mob cuts the boot to 4.5 s and `cutest` to 6.0 s, with
   byte-identical prototypes. This is a production boot-time fix, not a test-only shortcut.
2. The next largest costs are fixed sleeps in two shell tests (autorun supervision 21.7 s,
   process-memory monitor 9.5 s, and the latter runs twice in `make test-all`).
3. On GitHub, `make test` and `make test-all` run without `-j`, so the two slowest jobs spend
   over six minutes compiling `cutest` (and the server) on one core.
4. The CI matrix compiles the server about 30 times per PR; roughly a quarter of those builds
   verify nothing that another job does not already verify.
5. The 25-minute local matrix replication is dominated by cold compiles in fresh containers.
   A prebuilt image plus a shared ccache directory removes almost all of that.

## Measurements

Host loop (this worktree, after `make clean && make -j16`):

| Step | Time |
|------|------|
| Cold `make -j16 cutest` (400 objects, `-O2`) | 14.2 s wall, 3 min CPU |
| Cold `make -j16` (server) | 13.2 s wall |
| Edit one `.c`, `make -j16 cutest` | about 3 s (draft measurement) |
| `./cutest`, 1448 tests | 33.2 s |
| - of which the syntax-check boot test | 30.9 s |
| - all other 1447 tests | 1.5 s |
| `./luminari -c -q -d lib` (full world, 27,092 mobs) | 31.1 s wall, 30.7 s user |
| Same with `affect_total()` once per mob | 4.5 s wall |
| `./cutest` with that change, all 1448 pass | 6.0 s |
| `make -j16 test` (prerequisites in parallel, unchanged sources), passes | 55 s |

Script sub-targets of `make test` and `make test-all`, run one at a time on the host:

| Target | Time | Note |
|--------|------|------|
| `test-autorun-supervision` | 21.7 s | 7.6 s is the hardcoded 5 s fastboot sleep in `autorun.sh`; the rest is 1 s watchdog/state intervals and a `sleep 1.2` |
| `test-world-tools` | 10.1 s | 6.7 s Python unit tests, 2.1 s doc checks (test-all only) |
| `test-vessel-tooling` | 9.0 s | all of it is `test_monitor_process_memory.sh` (fixed sleeps 2.5 + 1.5 + 2.5 s) |
| `test-process-memory` | 8.8 s | the same two scripts again (test-all only) |
| `test-character-rename-schema` | 2.6 s | starts a disposable `mariadbd` |
| `test-sql-interpolation` | 1.8 s | |
| the other 15 targets | under 1.3 s each | |

Boot profile (14 gdb samples during "Loading mobs"): 13 samples in `affect_total` /
`calculate_best_mod`, called from `interpret_espec` (`src/core/db.c:3423`). Every espec keyword
line ends with `affect_total(mob_proto + i)`; `parse_simple_mob` calls it once more per mob.

GitHub, one push (24 jobs, wall about 17 min):

| Job | Wall | What dominates |
|-----|------|----------------|
| Production-linked tests (libevent / select) | 8m35s / 7m35s | 377 s single-core build of server + cutest inside `make test-all`, then about 90 s of tests |
| Clean archive, both build systems | 6m05s | four `-j4` builds plus two full test runs |
| Behavioral tests | 6m22s | 73 s `make -j4`, then 207 s single-core cutest build inside `make test` |
| Production profile x5 | 4m15s to 6m29s | build + (for autotools) single-core cutest build + tests, five times |
| CMake gcc / clang | 4m29s / 3m47s | 149 s build, 66 s serial ctest |
| Coverage, sanitizers, memory check | 2m47s to 4m22s | one instrumented build each |
| CodeQL | 5m32s | its own build; cannot run locally |
| world-tools, parity, hygiene, format, lint, compile-check, integration | under 2 min each | |

Server compiles per PR today: cmake 4, clean-archive 4, behavioral 2, unit-tests 4,
production-profile 10, sanitizers 1, memory 1, coverage 1, compile-check 1, CodeQL 1,
server-startup 1 = 30.

Other facts that shape the plan:

- `LUMINARI_IO_DRIVER` is read at run time (`src/net/reactor.c`); the libevent and select jobs
  build identical binaries.
- `quality.yml` `compile-check` builds with `-Wall -Wextra` and never fails; `AM_CFLAGS`
  already carries those flags and the CMake `ci-*` presets build with `-Werror`.
- `quality.yml` `lint` runs clang-tidy on three files behind `|| true` and `head -100`.
  It cannot fail.
- `unittests/CuTest/test_mob_autoroll.c` is the only test that drives `interpret_espec`
  directly, and it asserts `GET_REAL_*` (base) values, which the change below does not touch.
- 97 files under `src/` test `LUMINARI_CUTEST`, including `handler.h`, `db.h`, `comm.h`,
  and `act.h`. Sharing objects between `luminari` and `cutest` stays off the table.
- The full world is not tracked (44 files under `lib/world/` in git), so CI boots the
  13-mob minimal world. The host `make test` is the only full-world boot check; it stays.
- This worktree's dependency files were empty (`# dummy`, 657 of 661) because `configure`
  reran after the objects were built. `make clean && make -j16` repaired them today.

## Plan

Ordered by payoff per line changed. Phases 1 to 4 are small, independent commits and can
land as one PR. Phases 5 and 6 touch the workflows and are a second PR.

### Phase 1: compute mob totals once per prototype (server)

Change `src/core/db.c`: remove the `affect_total(mob_proto + i)` at the end of `interpret_espec`
and the one at the end of `parse_simple_mob`; call it once at the end of `parse_mobile`,
after the S/E section and before the trigger loop. Result: 27,092 calls instead of about
424,000. Full-world syntax boot drops from 31 s to 4.5 s, the host `cutest` from 33 s to
6 s, and the live server boots and copyovers about 26 s faster.

Safety, verified today: `interpret_espec` never reads a derived stat (no `GET_STR`,
`GET_AC`, `GET_HITROLL`, and so on on a right-hand side), so no keyword depends on a prior
`affect_total`. Dumping `aff_abils`, `real_abils`, and `points` for all 27,092 prototypes
after boot gives byte-identical output with and without the change (3,955,432 bytes each).
The boot log is identical apart from the build id and connection thread ids. All 1448
tests pass.

Proof for the PR: repeat the prototype dump comparison against the development world and
record the two sizes and `cmp` result in the PR body; `make -j16 test-all` green.

### Phase 2: run the test targets in parallel and remove the duplicate

`Makefile.am`:

- `make -j16 test` already passes with all prerequisites running concurrently (verified).
  Document `make -j$(nproc) test` as the normal invocation. Optionally make the `cutest`
  run a prerequisite target (`run-cutest`) so it overlaps the shell tests instead of
  running after them.
- Turn `test-all` into a prerequisite list (`test test-world-tools test-protocol
  test-character-rename-static test-character-rename-schema`) with `$(MAKE) install` as
  the only recipe line, so `-j` applies to it too.
- Drop `test-process-memory` from `test-all`; `test-vessel-tooling` (inside `test`)
  already runs both scripts. Keep the standalone target.

`CMakePresets.json`: add `"execution": {"jobs": N}` to the `base` test preset (or pass
`ctest -j` in the workflows) so the same scripts run concurrently under ctest.

Workflows: pass `-j"$(nproc)"` to every `make test` and `make test-all` step (`build`,
`unit-tests`, `production-profile` autotools entries). This alone removes about five
minutes from each of the two slowest GitHub jobs and two to three minutes from the others.

### Phase 3: replace fixed sleeps in the two slow shell tests

`scripts/autorun/autorun.sh` and `scripts/autorun/test_autorun_supervision.sh`:

- Read the fastboot delay from `AUTORUN_FASTBOOT_DELAY` (default 5) and export it as 0 in
  the planned-reboot scenario. Saves 5 s.
- The watchdog and state intervals are already environment-driven and go straight to
  `sleep`, which accepts fractions. Set `WATCHDOG_CHECK_INTERVAL=0.2` and
  `AUTORUN_STATE_INTERVAL=0.2` in the scenarios instead of 1. Replace the `sleep 1.2`
  survival check with a poll on the watchdog log.
- Target: 21.7 s to about 5 s. The scenarios and assertions do not change.

`scripts/process-memory/test_monitor_process_memory.sh`: replace the three fixed sleeps with
`wait_for` loops on the TSV row they are waiting for (0.1 s poll, same overall timeout).
The monitor's `--interval` stays an integer, so the first sample arrives within 1 s.
Target: 9.5 s to about 3 s.

After Phases 1 to 3, `make -j16 test` on the host is bounded by `cutest` at about 6 s plus
the longest script at about 5 s, so roughly 10 s; `make -j16 test-all` about 15 s.

### Phase 4: make slow and single tests visible in cutest

`unittests/CuTest/make-tests.sh` (generated `AllTests.c`, used by both build systems):

- Honour `CUTEST_FILTER=<substring>`: register only tests whose name contains it. One test
  file's tests then run in well under a second after a 3 s incremental build.

`unittests/CuTest/CuTest.c`: time each test in `CuTestRun` and, after the summary, list
any test slower than 1 s. No flag, no threshold assertion (timing assertions flake in CI);
this is what would have made the 31 s boot visible months ago.

### Phase 5: build hygiene

- `AGENTS.md` and `docs/guides/SETUP_AND_BUILD_GUIDE.md`: the normal build is
  `make -j$(nproc)`; `make clean` is for configure or `Makefile.am` changes or a suspect
  tree. With working dependency files a header edit rebuilds exactly what includes it.
- Install `ccache` on the host (`sudo apt install ccache`, needs the user), put
  `/usr/lib/ccache` first on `PATH` (a `CC="ccache gcc"` configure breaks the shell test
  gates), and mount the cache directory into the local CI containers. Objects
  of an unchanged tree then cost about 5 ms each instead of 0.3 s.
- Optional: a dev configure with `CFLAGS="-g -O0"` (or the CMake `dev` preset). One-file
  compiles drop from 2.8 s to 0.75 s. Keep `-O2` for CI. Not required; the incremental
  build is already about 3 s.

### Phase 6: consolidate the CI matrix (same coverage, 23 builds instead of 30)

Each item names what it removes and why nothing is lost.

1. Merge `build` (Behavioral tests) into `unit-tests` libevent. `make test-all` is a
   superset of `make test`; move the clean-working-tree and `make dist` hygiene steps into
   the libevent job. Removes 2 builds and one 6-minute job.
2. Collapse the `select` matrix entry into the same job: after `make -j test-all`, run
   `LUMINARI_IO_DRIVER=select ./cutest` (6 s) instead of rebuilding and rerunning every
   shell test under an env var they never read. Removes 2 builds and a 7.5-minute job.
3. Fold the integration `server-startup` smoke test into that job as a final step: it needs
   only the installed binary and the prepared runtime, both already present. Keeps the
   real-port boot, health endpoint, and graceful-shutdown checks. Removes 1 build.
4. `production-profile`: keep all five hardened server builds and
   `verify_hardened_binary.sh` on each (that is the job's purpose). Run the test suite under
   hardening in two of them, one per build system and compiler family (autotools gcc-14,
   cmake clang); the other three build and verify only (`BUILD_TESTS=OFF` for cmake, no
   `make test` for autotools). Removes 3 cutest builds and three test runs.
5. Delete `quality.yml` `compile-check`. `-Wall -Wextra` is already in `AM_CFLAGS` and the
   CMake `ci-gcc`/`ci-clang` jobs fail on warnings. Removes 1 build.
6. `quality.yml` `lint`: either delete it or make it real: generate
   `compile_commands.json` with a CMake configure (no build), run clang-tidy with the
   tracked `.clang-tidy` on the C files changed in the PR, and fail on findings. Recommended:
   make it real; it is the only static-analysis gate that runs per PR and currently checks
   nothing.
7. `clean-archive`: keep both halves (the archive-completeness proof is the point) but run
   them as two matrix entries (`--skip-cmake`, `--skip-autotools`) so the 6-minute job
   becomes two 3-minute jobs in parallel.
8. Add ccache to the GitHub jobs through one composite action
   (`.github/actions/setup-build`: apt packages, config headers, ccache with
   `actions/cache` keyed on job name, compiler, and flags). Re-pushes to a PR then rebuild
   only changed objects. This also removes the seven copies of the "Install dependencies"
   and "Copy config headers" step pairs.
9. Keep `sanitizers`, `memory-check`, `coverage`, `CodeQL`, `world-tools`, `build-parity`,
   `hygiene`, `format-check`, `database-migration`, and `world-validation` as they are;
   each is a distinct instrumented build or a cheap non-build check.

Expected GitHub wall after Phases 2 and 6: about 5 min instead of 17, bounded by CodeQL
(5.5 min, GitHub-only) and the clean-archive halves.

### Phase 7: local matrix replication

The user-side runner (containers per job) keeps working as it does; change its inputs:

- Build one local image from the union of the workflow apt lists (a `Dockerfile` under
  `scripts/ci/local/`), instead of `apt-get` in every job.
- Mount a shared ccache directory into every container with `CCACHE_BASEDIR` set to the
  workspace so archive-tree builds hit the checkout-tree cache.
- Run three or four jobs concurrently; the host has 16 cores and each job builds at
  `-j4` on GitHub anyway.
- Run the matrix once, on the final commit. Iterate on the host with `make -j16 test-all`.

Estimate: 23 builds with about seven distinct flag sets means seven cold builds and
sixteen cache hits, so roughly 5 to 8 minutes wall for the whole matrix, most of it the
instrumented jobs (valgrind, ASan, coverage).

## Expected results

| Loop | Today | After Phases 1 to 4 | After all phases |
|------|-------|---------------------|------------------|
| Edit one file, rerun one test file | 3 s build + 33 s cutest | 3 s + under 1 s (`CUTEST_FILTER`) | same |
| `make test` on the host | about 70 s serial | about 10 s (`-j16`) | same |
| `make test-all` on the host | about 1.5 min | about 15 s | same |
| Live server boot / copyover | 31 s in the mob loader | 4.5 s | same |
| GitHub `test.yml` wall | about 17 min | about 10 min | about 5 min |
| Local container matrix | about 25 min | about 20 min | about 5 to 8 min |

"After" numbers for the matrix and GitHub are estimates from the per-step timings above;
the host numbers were measured with the changes applied in this worktree and then reverted.

## Ablation of the first draft

- Dropped "profile the child-boot tests" as a later step: it was the first step. The other
  three fork-based test files cost under 0.1 s combined.
- Dropped switching the host loop to the minimal CI world to avoid the 31 s boot. After
  Phase 1 the full-world boot costs 4.5 s and is the only place the real world data is
  booted, so it stays.
- Dropped a prebuilt image for GitHub: `apt-get` there is 15 to 25 s per job. A prebuilt
  image matters only for the local container replication (Phase 7).
- Demoted `-O0` to optional: the incremental build is already 3 s; it is not on the
  critical path once Phase 1 lands.
- Dropped a boot-time assertion in the syntax-check test; the slow-test report in Phase 4
  gives the same visibility without a flaky threshold.
- Kept "no shared objects between luminari and cutest" as a non-goal; 97 files differ.
- Did not add a new test-runner script or wrapper; every change lands in existing targets,
  scripts, and workflows.

## Verification per phase

1. Prototype dump comparison (`aff_abils`, `real_abils`, `points`, all mobs) identical;
   `make -j16 test-all` green; boot log identical apart from ids.
2. `make -j16 test-all` and `ctest -j16 --preset dev` green three times in a row (looks for
   ordering or port collisions between concurrent scripts).
3. Each rewritten script passes ten times in a loop; `bash -x` with `PS4='+ $EPOCHREALTIME '`
   shows no gap over 1 s except the disposable `mariadbd` start.
4. `CUTEST_FILTER=Test_mob_autoroll ./cutest` runs only those tests; a deliberately slow
   test appears in the slow list.
5. `touch src/core/structs.h && make -n cutest | grep -c ' -c '` schedules about 330 compiles.
6. Every remaining job green on GitHub; the merged libevent job log shows the clean-tree,
   dist, select-driver cutest, and startup-smoke steps with their success markers.
7. Full local matrix once on the final commit, timed.

## Documentation to update with the code

- `AGENTS.md`: build commands (`make -j`, `make clean` policy), `make -j test` guidance.
- `docs/guides/TESTING_GUIDE.md`: parallel targets, `CUTEST_FILTER`, slow-test report,
  the CI job map after Phase 6, ccache for local matrix runs.
- `docs/guides/SETUP_AND_BUILD_GUIDE.md`: ccache and the optional `-O0` configure.
- Help files: none; no player-facing behavior changes.

## Implementation evidence (issue #178)

All seven phases are implemented together. The no-op clang-tidy job was deleted rather
than introducing a new lint policy. All existing effective checks remain, including the
installed-binary assertions, clean-tree/dist checks, both I/O drivers, and real-port boot,
health, and graceful shutdown. Test execution is parallel in Make and the CI CTest commands.

Validation used the development worktree and its complete world through an Ubuntu 24.04
container toolchain; host development packages were unavailable and installing them required
sudo credentials. The isolated test database and runtime did not use local credentials.
The runtime enabled diagonal exits to match the existing development world.

- Baseline: 1,448 tests passed in 35.747 s. Updated: the same 1,448 passed in 5.985 s.
- All 27,092 mobile prototypes had byte-identical `aff_abils`, `real_abils`, and `points`:
  both dumps were 3,847,064 bytes; `cmp` succeeded. Dumps were taken immediately after
  `index_boot(DB_BOOT_MOB)` from baseline and updated production executables using GDB.
- Three final warm `make -j16 test-all` runs passed in 12.602, 11.460, and 11.615 s.
- Three `ctest -j16 --preset dev` runs passed all 28 entries in 10.81, 10.93, and 10.82 s.
- Both polling scripts passed ten consecutive runs. Supervision also passed ten runs
  with TCP port 4100 occupied, leaving the listener alive. Its fake executables use an
  isolated socket probe so they coexist with a development MUD. The container requires `--init` so
  detached supervisors are reaped, just as they are on a normal host.
- `CUTEST_FILTER=Test_mob_autoroll ./cutest` passed exactly four tests. The runner regression
  checks unset, empty, matching, and unmatched filters and reports a deliberately slow
  failing test after its summary.
- `make -n -W src/core/structs.h cutest` scheduled 390 affected compiles, confirming that header
  dependency tracking is active without changing the header's contents or timestamp.
- A cold-build compiler lock reproduced concurrent calculator compilation (exit 2).
  Making the calculator a shared prerequisite of `check` and world tools eliminated
  the race: the guarded build passed with exactly one calculator compilation.
- The installed server passed the port-4100 startup, health, and graceful-shutdown smoke
  test through autorun. Build parity, workflow syntax, and archive-runtime regressions passed.

The original build-count estimate had an arithmetic error: applying its stated removals
removes nine of thirty server/test executable builds, leaving 21, including CodeQL. All five
hardened server builds remain; only the three explicitly redundant hardened CuTest builds
are omitted. Instrumented builds and coverage floors are unchanged.

The local image and runner live in `scripts/ci/local/`. They use committed source snapshots,
three containers with four cores each by default, isolated databases, and a shared ccache.
Archive builds normalize debug paths to allow cache reuse across temporary directories.
The final-commit matrix writes a timed `summary.json` and per-job logs; the PR records its
result together with the GitHub checks. Matrix wall-time estimates above are not assertions.
