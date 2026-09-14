# Strict C23 toolchain: progress and remaining work

Tracking issue: #86. Branch: `strict-c23-toolchain`. Written 2026-09-14 on the
development host (16 cores, WSL2, Ubuntu 24.04).

Goal: a documented two-compiler contract (minimum and current GCC and Clang),
strict builds that are errors on every pull request, a ratchet for the known
warning debt, and feature detection that strict flags cannot influence.

## Completed

### Compilers and CI

- Compiler policy: GCC 13 and Clang 18 minimum (the `ubuntu-latest` runner
  image), GCC 16.2 and Clang 22.1.8 current (the `gcc:16.2` container image
  and apt.llvm.org). Documented with an update cadence in
  `docs/guides/SETUP_AND_BUILD_GUIDE.md`.
- The strict CMake job in `test.yml` is a matrix of all four compilers times
  Debug and Release, blocking, with `-Werror` on the baseline tier. The
  Autotools job configures with `--enable-werror`.
- `scripts/ci/check_compiler.sh` runs in every compiling job. It reads the
  preprocessor's predefined macros to prove the family and version, so
  installing Clang can never silently produce a GCC build. Configure and CMake
  both print the effective warning flags.
- `.github/actions/setup-build` works as root inside a compiler container.
- The local runner (`scripts/ci/local/run.py`) expands matrix `include`
  entries the way GitHub does, supports `container:` jobs through a second
  image (`scripts/ci/local/Dockerfile.gcc-16.2`), and installs Clang 22 in its
  main image.

### Warning tiers

- One flag list in `scripts/deployment/production_profile.sh`, probed per
  compiler and consumed by both `configure.ac` (`--enable-warning-tier`) and
  `CMakeLists.txt` (`LUMINARI_WARNING_TIER`). `DEVELOPER_MODE` is gone.
- Baseline tier: `-Wall -Wextra -Wstrict-prototypes -Wold-style-definition
  -Wpointer-arith -Wformat-security -Wvla` plus GCC's `-Wtrampolines
  -Walloc-size -Wbidi-chars=any -Wcalloc-transposed-args
  -Wflex-array-member-not-at-end -Wunterminated-string-initialization`. Clean
  on all four compilers; `-Werror` is refused with any other tier.
- Migration tier: conversions, shadowing, switch coverage, missing prototypes,
  `-Wformat=2`, allocation, duplicated conditions and branches, logical
  operators, fallthrough, `-Wwrite-strings`. Held by
  `scripts/ci/check_warning_budget.py` against `scripts/ci/warning_budget_gcc-16.txt`
  and `scripts/ci/warning_budget_clang-22.txt`; growth in any class fails the
  new `warning-budget` job. Counting is by distinct site with make output sync,
  which was required to make Clang's numbers deterministic.
- Analysis tier: GCC `-fanalyzer` and Clang's opinionated extras, plus an
  ISO C23 `-Wpedantic` extension report, in the weekly, non-blocking
  `.github/workflows/toolchain-analysis.yml`.

### Feature detection

- `-Werror` is stripped from the caller's CFLAGS for every configure and
  CMake probe and restored afterwards. The `struct in_addr` and `socklen_t`
  probes no longer emit diagnostics. Three unused `-Werror` probes were
  deleted.
- `scripts/ci/check_configure_probes.sh` configures both build systems with
  and without strict flags and fails when `src/conf.h` differs. It caught the
  `socklen_t` fallback and the `AC_CHECK_FUNCS` false negatives.

### Source fixes surfaced by the new compilers and flags

- NULL guards on all 119 special-procedure identify checks: the `SPECIAL`
  wrapper passes a NULL argument on pulse calls, so the unguarded `strcmp` was
  a latent crash that GCC 13 and 16 each proved at different inlining depths.
- An out-of-bounds read in the epic weapon specialization prerequisite: a
  FEAT number was used as a combat-feat array index (GCC 16.2 at -O3).
- A NULL name reaching `strchr` in the shop purchase message when the buyer
  carries nothing (GCC 16.2 at -O3).
- Four variable-length arrays removed (the repository already forbade them).
- An unused loop counter, a `strncpy` truncation, an uninitialized const
  pointer argument and a non-literal format call in tests.
- Both build systems now request an ELF build ID at link time, and the
  production profile probes for the CET property note on the linked image
  instead of trusting flag acceptance. A toolchain built from source without
  `--enable-cet` (the official gcc images) accepts `-fcf-protection` but
  cannot mark the binary; the profile now reports that honestly.

### Verification done locally

- Strict full builds: GCC 13, GCC 16, Clang 18, Clang 22, all zero errors.
- `make test` on the strict Autotools build: 1483 tests pass.
- The local CI matrix (`scripts/ci/local/run.py`) on the final commit.

## Budget snapshot

| Compiler | Distinct sites | Classes |
|----------|----------------|---------|
| GCC 16.2 | 11363 | 24 |
| Clang 22.1.8 | 22878 | 23 |

Largest classes: sign conversion, value conversion, missing prototypes,
`-Wformat=` signedness (GCC), switch default, jump-misses-init,
double promotion, discarded qualifiers.

## Burn-down progress

Sites after each landed step, measured with the CI budget job's exact CMake
command inside the pinned images (`luminari-ci:local-gcc-16.2` and
`luminari-ci:local-fast` for Clang 22.1.8) on a snapshot of the working tree
with the example config headers. The first local run reproduced both committed
budget files exactly; with ccache a full budget build takes about two minutes
per compiler.

| Step | Change | GCC 16.2 | Clang 22.1.8 |
|------|--------|----------|--------------|
| start | committed budgets | 11363 | 22878 |
| 0 | `--list` and `--by-token`; `-Wswitch-default` dropped | 10706 | 22221 |
| 1.1 | `IS_SET_AR` casts the element before the mask | 10706 | 10578 |
| 2.1 | generated `test_prototypes.h` | 9223 | 9095 |

Also fixed on the way: the budget check counted only `file:line:col: error:`
lines, so a build that stopped on a missing header (`fatal error:`), a linker
failure, or a make `***` line still reported a trustworthy count. The CI step
pipes the build through `tee` without `pipefail`, so the check is the only
gate that sees such a failure; it now counts all four forms.

## Remaining work

1. GitHub-side confirmation. Container jobs, the apt.llvm.org install step,
   and `actions/cache` inside the `gcc:16.2` container cannot be replicated
   locally. Open the pull request and watch the first run; the compiler check
   step is the first thing that would fail if the runner's toolchain differs.
2. Dispatch `toolchain-analysis.yml` once by hand to confirm its wall time
   fits the job timeout. Locally the GCC `-fanalyzer` build of the whole tree
   took well over half an hour on three cores.
3. Triage the analysis findings. The first local GCC 16.2 analyzer run
   reported 66 `malloc-leak`, 39 `null-dereference`, 27
   `possible-null-argument`, 10 `out-of-bounds`, and 4 `use-after-free`
   sites. These are candidate bugs, not noise, and deserve their own issue.
4. Burn down the migration budget following the plan in the next section.
5. Cadence. Bump the current versions in `test.yml`,
   `scripts/ci/local/Dockerfile*`, and the setup guide within a month of each
   GCC or LLVM point release; raise the minimum when the runner image drops a
   compiler. Regenerate both budget files whenever a pinned compiler changes,
   since counts are compiler-specific.
6. The gcc toolchain PPA on the development host ships a GCC 16 trunk
   snapshot, not 16.2. Use the `luminari-ci:local-gcc-16.2` image for anything
   that must match CI.

## Burn-down plan for the migration budget

Measured on the budget logs behind the two baseline files. The ordering is by
sites cleared per hour of work: root-cause edits in headers first, generated
or scripted edits second, hand edits last. Every step ends the same way: build
the migration tier with both pinned compilers, run
`check_warning_budget.py --update` for each, commit the lowered budget files
with the fix, and move any class that reached zero on both compilers from the
migration list to the baseline list in `production_profile.sh`.

### Where the sites actually are

| Lever | Sites it clears | Evidence |
|-------|-----------------|----------|
| `IS_SET_AR` in `src/utils.h` | about 11000 Clang `sign-conversion` | the `&` of an `int` array element with the `unsigned` `Q_BIT` mask converts the element; `IS_NPC` alone is 3084 sites, `GET_NAME` 1248, `AFF_FLAGGED` 939, the `*_FLAGGED` family and every colour macro (they expand to `PRF_FLAGGED`) the rest |
| `sh_int` and `byte` fields in `struct affected_type` and friends | about 1450 GCC `conversion`, about 1000 Clang `implicit-int-conversion` | 975 sites are `int` to `sh_int`, 281 `int` to `byte`, 195 `int` to `sbyte`; `src/magic/magic.c` alone has 509 |
| generated test prototypes | 1485 of 2123 `missing-prototypes` | every `Test*` function in `unittests/CuTest/` |
| GCC fix-it patch for `-Wformat` | 964 GCC `format=` | all are `%d` with an unsigned or vnum argument; GCC emits fix-its for these |
| four files for `jump-misses-init` | 579 | `magic.c` 303, `players.c` 135, `study.c` 66, `act.item.c` 43 |
| `float` locals in `src/wilderness/` | most of 554 `double-promotion` and 303 `float-conversion` | three wilderness files hold over 200 sites |
| duplicate `extern` lines | 567 `redundant-decls` | 27 redeclare `conn`, 18 `world`, 17 `mysql_available` |

### Step 0: tooling (half a day)

- Add `--list CLASS` to `scripts/ci/check_warning_budget.py` that prints the
  distinct sites of one class grouped by file, and `--by-token CLASS` that
  groups them by the identifier at the diagnostic column. Both were needed to
  produce the table above and are needed again after each step.
- Decide two policy questions before touching code, because they change the
  target by 700 sites:
  - Drop `-Wswitch-default` (657 sites, one message: "switch missing default
    case"). `-Wswitch` in the baseline already reports unhandled enum values,
    and `-Wswitch-enum` stays. Adding an empty `default: break;` to 657
    switches adds nothing to correctness.
  - Keep `-Wjump-misses-init`. It is C++-compatibility wording but every site
    is a `case` label jumping over an initialized declaration, which the
    style guide already forbids (declarations at the top of blocks).

### Step 1: header and type roots (one day, about 14000 sites)

1. `IS_SET_AR`: cast the array element to `unsigned int` before the mask, or
   store flag arrays as `unsigned int` if the ASCII loaders and savers agree.
   The cast is the one-line version; measure after it. Expect Clang
   `sign-conversion` to fall from 15028 to under 3000.
2. Widen `spell`, `duration`, `modifier`, `specific` in `struct affected_type`
   and the `byte` and `sbyte` fields that `magic.c`, `db.c`, `players.c`, and
   `objsave.c` assign from `int`. Player and object files are ASCII, so no
   on-disk layout changes; check the MySQL column widths for the same fields.
   Expect GCC `conversion` to fall by about 1450 and Clang
   `implicit-int-conversion` by about 1000.
3. `size_t` to `int` (Clang `shorten-64-to-32`, 602; GCC `conversion` 222):
   the results of `strlen`, `sizeof`, and `snprintf` stored in `int`. Change
   the local to `size_t` where it only feeds another size, cast where it feeds
   an `int` API. Scripted with the site list; review by file.

### Step 2: generated and scripted edits (one day, about 3500 sites)

1. Make `unittests/CuTest/make-tests.sh` also write
   `unittests/CuTest/test_prototypes.h` and include it from `CuTest.h`. Both
   build systems already run the generator before compiling. Clears 1485.
2. Build once with `-fdiagnostics-generate-patch` on GCC 16.2 with the
   migration tier and apply only the `-Wformat` hunks. Clears 964 in one
   commit; review the hunks that pick `%u` for a `vnum` and use `PRI_IDX`
   there instead.
3. Delete the flagged `extern` lines for `redundant-decls`: a script that
   removes a flagged line when it is a single-line declaration ending in `;`
   and the same symbol is declared in an included header. Clears 567.
4. The remaining 638 `missing-prototypes` in `src/`: for each flagged
   function, if no other file references the name, prepend `static`;
   otherwise add the prototype to the header that matches the source file.
   Scripted; the `static` half is safe by construction, the header half needs
   a compile to confirm.
5. Tests: `-Wwrite-strings` and `cast-qual` (591 test sites) are string
   literals assigned to `char *` and casts that strip `const`. Change the
   test locals to `const char *`; where a production API takes `char *` for a
   value it never writes, constify that parameter instead of casting.
6. Clang `implicit-fallthrough` (33): insert `[[fallthrough]];` where the
   existing comment says so. GCC already accepts the comments.

### Step 3: file-focused hand work (two to three days, about 2500 sites)

1. `jump-misses-init` (579): wrap each offending `case` body in braces or
   hoist the declaration. Four files; `magic.c` is half of it.
2. `double-promotion` and `float-conversion` (857 GCC, about 1000 Clang):
   change `float` to `double` in the wilderness and resource files, and give
   the float-typed struct fields explicit casts at the assignment. Performance
   is irrelevant on this path.
3. `shadow` (274): locals named `room_vnum`, `background`, `weapon_type`,
   `region_vnum` shadow typedefs and globals. Rename per function.
4. `-Wwrite-strings` in `src/` (188): the 23 in `bsd-snprintf.c` are
   `findLine` and friends taking `char *`; constify the parameters.
5. Small classes, one sitting: `null-dereference` 60, `switch-enum` 45,
   `nested-externs` 44, `logical-op` 31, `format-nonliteral` 25,
   `float-equal` 22, `duplicated-branches` 21, `cast-align` 8, `undef` 8,
   `alloca` 4, `duplicated-cond` 3, `alloc-zero` 1. The `null-dereference`
   sites are candidate bugs; the rest are style and fold into whatever is
   nearby.

### Step 4: the sign-conversion tail (decision point)

After step 1 the remaining `sign-conversion` sites are the ones the type
system genuinely disagrees about: `int` counters indexed into `size_t`, `int`
arguments to the unsigned `vnum` types, and `enum` status values compared to
`int`. Measure with `--by-token`. If fewer than 3000 remain, fix them in the
same file-focused way as step 3 and promote the flag. If more remain, the
honest choice is to move `-Wsign-conversion` to the analysis tier and keep
`-Wconversion` in migration; the issue asked for the family to be tracked,
not for every `int` index to become `size_t`.

### Expected trajectory

| After | GCC 16.2 sites | Clang 22.1.8 sites |
|-------|----------------|--------------------|
| today | 11363 | 22878 |
| step 1 | about 9500 | about 8500 |
| step 2 | about 5800 | about 5300 |
| step 3 | about 2800 | about 3000 |
| step 4 | 0 or the sign-conversion tail | same |

Each step is its own pull request so the budget files shrink in reviewable
increments and a regression in one class is visible in the diff of one file.

When items 1 and 2 are confirmed, this document's enduring content is already
in the setup, CMake, and testing guides; file items 3 and 4 as issues and
delete this file.
