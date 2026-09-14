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
4. Burn down the migration budget. Each fix lowers a class; run
   `check_warning_budget.py --update` with both pinned compilers afterwards.
   When a class reaches zero on both, move its flag from the migration list
   to the baseline list in `production_profile.sh`.
5. Cadence. Bump the current versions in `test.yml`,
   `scripts/ci/local/Dockerfile*`, and the setup guide within a month of each
   GCC or LLVM point release; raise the minimum when the runner image drops a
   compiler. Regenerate both budget files whenever a pinned compiler changes,
   since counts are compiler-specific.
6. The gcc toolchain PPA on the development host ships a GCC 16 trunk
   snapshot, not 16.2. Use the `luminari-ci:local-gcc-16.2` image for anything
   that must match CI.

When items 1 and 2 are confirmed, this document's enduring content is already
in the setup, CMake, and testing guides; file items 3 and 4 as issues and
delete this file.
