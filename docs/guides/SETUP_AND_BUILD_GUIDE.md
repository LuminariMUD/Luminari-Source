# LuminariMUD Setup and Build Guide

## Supported Setup

LuminariMUD builds on Linux and Linux-compatible environments such as Ubuntu
under WSL2. The server requires MariaDB/MySQL and a compiler with the
repository's GNU C23 feature support.

The preferred fresh-install path is:

```bash
./scripts/deployment/deploy.sh
```

That command installs supported dependencies, creates missing local
configuration from examples, provisions MariaDB, initializes minimal world
data, configures Autotools, builds, and installs `bin/luminari`. Inspect exact
flags with `./scripts/deployment/deploy.sh --help`.

## Ubuntu, Debian, and WSL2 Dependencies

```bash
sudo apt-get update
sudo apt-get install -y build-essential git make autoconf automake libtool \
  cmake pkg-config mariadb-server libmariadb-dev libcrypt-dev libgd-dev \
  libevent-dev libcurl4-openssl-dev libssl-dev libjson-c-dev zlib1g-dev \
  mariadb-client curl pandoc gdb valgrind
```

Minimum supported dependency versions are listed in the
[CMake build guide](../development/CMAKE_BUILD_GUIDE.md); both build systems
require the same libraries.

## Existing Configured Checkout

Autotools is preferred for incremental development:

```bash
make -j"$(nproc)"
make -j"$(nproc)" test
make install
```

Incremental builds retain dependency files and rebuild affected source/header users.
Use `make clean` after changing compiler flags or build configuration, or when dependency
files are stale; it is not part of the normal edit/test loop.

Optional compiler caching and faster unoptimized development builds:

```bash
sudo apt install ccache
export PATH=/usr/lib/ccache:$PATH
./configure
# Optional: use this instead when optimizing edit/compile latency.
./configure CFLAGS="-g -O0"
```

Put the `PATH` export in your shell profile so every build and test shell sees it. Do not
configure with `CC="ccache gcc"`: the test gates run `$CC` as a single program name, so
`make test` fails with `ccache gcc: command not found`. The ccache masquerade directory
keeps `CC=gcc` and is what CI uses. Clean once when switching an existing build's compiler
or flags. CI retains its optimized
and instrumented profiles. For CMake, add `-DCMAKE_C_COMPILER_LAUNCHER=ccache` to configure;
`cmake --preset dev` already selects a debug build.

If the generated build files are absent:

```bash
autoreconf -fvi
./configure
make -j"$(nproc)"
make -j"$(nproc)" test
make install
```

`make test` may leave a root-level test build of `luminari`; the required
`make install` step activates the versioned binary at `bin/luminari` and removes
that root artifact.

## Fresh Manual Configuration

Only when the real local files do not exist, copy the tracked examples:

```bash
test -e src/config/campaign.h || cp src/config/campaign.example.h src/config/campaign.h
test -e src/config/mud_options.h || cp src/config/mud_options.example.h src/config/mud_options.h
test -e src/config/vnums.h || cp src/config/vnums.example.h src/config/vnums.h
test -e lib/mysql_config || install -m 600 lib/mysql_config_example lib/mysql_config
test -e lib/.env || install -m 600 lib/.env_example lib/.env
```

Edit local configuration without committing it. Never overwrite an existing
`src/config/campaign.h`, `src/config/mud_options.h`, `src/config/vnums.h`, `lib/mysql_config`, or
`lib/.env`. Database initialization details are in the
[deployment guide](../deployment/DEPLOYMENT_GUIDE.md) and
[database initialization guide](DATABASE_INITIALIZATION_GUIDE.md).

A checkout created before the local headers moved to `src/config/` moves them
once. Until it does, configure, CMake, `deploy.sh`, and `setup.sh` stop and
print this command:

```bash
mkdir -p src/config && mv -n src/{campaign,mud_options,vnums}.h src/config/
```

World and text data must exist under `lib/`. Use the deployment script for a
fresh minimal world rather than assembling the required indexes manually.

## Production Profile

The default configuration is the development build. Production deployments
use the optimized and hardened profile, which both build systems derive from
`scripts/deployment/production_profile.sh`:

```bash
./configure --enable-production
make -j"$(nproc)"
./scripts/deployment/verify_hardened_binary.sh ./luminari
make -j"$(nproc)" test
make install
```

Unknown configure options are fatal, so a misspelled profile cannot fall back
to the default flags. `--enable-lto`, `--with-pgo-generate=DIR`, and
`--with-pgo-use=PATH` are explicit, measured additions. The
[deployment guide](../deployment/DEPLOYMENT_GUIDE.md#production-build-profile)
records the flag policy, verification, and crash-symbolization workflow.

## Compiler Policy and Warning Tiers

The build is GNU C23 on GCC or Clang. Two compiler generations are supported
and both are exercised by blocking CI jobs on every pull request:

| Role | GCC | Clang | Where CI gets it |
| -- | -- | -- | -- |
| Minimum | 13 | 18 | the `ubuntu-latest` runner image |
| Current | 16.2 | 22.1.8 | the `gcc:16.2` container image; apt.llvm.org |

GCC 13 only knows the pre-publication `-std=gnu2x` spelling; configure and
CMake accept it after the C23 keyword probe passes. Update cadence: the
current versions in `.github/workflows/test.yml` move to each new GCC and
LLVM point release within a month of it shipping, and the minimum moves when
the runner image's distribution drops a compiler. Every job that compiles
runs `scripts/ci/check_compiler.sh`, which reads the preprocessor's
predefined macros to prove the family and version before building, so
installing Clang can never silently produce a GCC build. The configure
summary and the CMake status output print the effective warning flags.

GNU extensions are an explicit choice (`-std=gnu23`, `CMAKE_C_EXTENSIONS ON`).
The scheduled `toolchain-analysis.yml` workflow compiles the tree as ISO C23
with `-Wpedantic` and reports how much of it depends on extensions; the
report is informational.

Warnings come in three cumulative tiers. The flag lists live in
`scripts/deployment/production_profile.sh`, which both build systems call so
the two builds apply the same set; each flag is probed, and a compiler that
lacks one reports it and continues.

| Tier | Contents | Enforcement |
| -- | -- | -- |
| `baseline` | `-Wall -Wextra` plus prototype hygiene, format security, `-Wvla`, and the GCC allocation-size and flexible-array checks | errors on every pull request (`--enable-werror`, `LUMINARI_WERROR=ON`) |
| `migration` | conversions, shadowing, switch coverage, missing prototypes, `-Wformat=2`, allocation, duplicated conditions and branches, logical-operator mistakes, fallthrough, `-Wwrite-strings` | a per-compiler budget that may only shrink |
| `analysis` | GCC `-fanalyzer`; Clang's opinionated extras | scheduled, informational |

Select a tier with `./configure --enable-warning-tier=migration` or
`cmake -DLUMINARI_WARNING_TIER=migration`. `-Werror` is refused with any
tier but `baseline`; the migration tier has thousands of pre-existing
instances and is held by `scripts/ci/check_warning_budget.py` instead. The
CI job builds with the pinned current compilers and compares the count of
distinct warning sites per class with `scripts/ci/warning_budget_gcc-16.txt`
and `scripts/ci/warning_budget_clang-22.txt`. Growth in any class, or a new
class, fails the job. Counting is by distinct site, and the build uses
make's per-target output sync so parallel diagnostics cannot interleave and
change the count. After fixing warnings, lower the budget:

```bash
cmake -S . -B build/budget -DCMAKE_C_COMPILER=clang-22 \
  -DLUMINARI_WARNING_TIER=migration -DBUILD_TESTS=ON
cmake --build build/budget -j"$(nproc)" -- --output-sync=target 2>&1 | tee build/budget/build.log
scripts/ci/check_warning_budget.py --compiler clang-22 --log build/budget/build.log --update
```

A class whose budget reaches zero on both compilers is promoted to the
baseline tier. A warning that must be suppressed is suppressed at the site
(`__attribute__` or a pragma) or, for a diagnostic that is wrong for this
code base, in the profile script next to a comment giving the reason; the
repository-wide tier lists are never weakened to accommodate one site.

Feature detection is independent of the warning policy: no configure or
CMake probe uses `-Werror` unless the probe itself requires it, and
`scripts/ci/check_configure_probes.sh` configures both build systems with
strict flags and plainly and fails if the generated `conf.h` differs.

## CMake

CMake is the supported secondary build. Use the checked-in presets, which
enable tests and write to `build/<preset>`:

```bash
cmake --preset dev
cmake --build --preset dev -j"$(nproc)"
ctest -j"$(nproc)" --preset dev
cmake --install build/dev
```

`dev-clang`, `ci-gcc`, `ci-clang`, `sanitizers`, `coverage`,
`release-hardened`, and `cross-aarch64` cover the other supported workflows.
Options such as `LUMINARI_WARNING_TIER`, `LUMINARI_WERROR`, and
`LUMINARI_SANITIZERS` are documented in the [CMake build guide](../development/CMAKE_BUILD_GUIDE.md).

Both build systems must list the same sources. `make check-build-parity` (also
run by `make test`, CTest, and CI) fails when `Makefile.am` and
`CMakeLists.txt` drift, including the production-source membership of both
`cutest` targets. `make distcheck-archive` configures, builds, tests, and
installs an untouched `git archive HEAD` through both systems; CI runs it as
a blocking job, and it is not part of `make test` because it rebuilds the
tree twice.

The production profile replaces `CMAKE_BUILD_TYPE`:

```bash
cmake -S . -B build -DLUMINARI_PRODUCTION=ON
```

`LUMINARI_LTO`, `LUMINARI_PGO_GENERATE`, and `LUMINARI_PGO_USE` mirror the
Autotools options.

## Run and Verify

```bash
./bin/luminari -d lib
```

The checked-in runtime configuration defaults to the reserved local game port
4100\. While the server runs, verify the loopback health listener from another
terminal:

```bash
./scripts/operations/healthcheck.sh
```

See [development.md](../development/README_development.md) for daily commands,
[TESTING_GUIDE.md](TESTING_GUIDE.md) for all test surfaces, and
[incident-response.md](../runbooks/incident-response.md) for operational
diagnosis.

## Source Tree Hygiene

The repository tracks source and data only. `scripts/ci/check_source_hygiene.py`
enforces three rules over every tracked file, reports every violation, and exits
non-zero when any is found:

- **No build products.** ELF, PE, Mach-O, and ar files are rejected by magic
  bytes; object, library, coverage, profiler, and core-dump files by name; and
  configure, automake, and CMake outputs by name. Images, audio, and fonts are
  the only binary files allowed.
- **Well-formed text.** Every non-media file must be valid UTF-8 with no NUL
  bytes and no carriage returns. This check is independent of the ASCII rule so
  a malformed byte or CRLF file cannot hide behind it.
- **ASCII documentation.** Every `*.md` file, plus `*.txt` under `docs/`, must
  be plain ASCII. Use `->`, `-`, straight quotes, `[OK]`/`[X]`, and ASCII box
  drawing (`.-|+'`) instead of typographic punctuation, emoji, or Unicode boxes.
  Documentation that has to describe a Unicode glyph names its code point
  (`U+2588 full block`) rather than embedding it.

Exceptions to the ASCII rule live in `ASCII_EXCEPTIONS` inside the script with
the reason each one exists. The list is empty: legal text under `docs/legal/`
and every current document are already ASCII. HTML under `docs/` is generated
web output that declares its own charset, so it is held to the UTF-8 and LF
rules only. Non-ASCII in C sources is limited to deliberate in-game glyphs
(map symbols, box borders) and is outside the documentation rule.

Run the checks locally:

```bash
python3 scripts/ci/check_source_hygiene.py              # tracked files
python3 scripts/ci/check_source_hygiene.py --root DIR   # an unpacked make dist tree
pre-commit run source-hygiene --all-files
```

CI runs the tracked-file check on every push (`.github/workflows/hygiene.yml`).
The behavioral test job additionally proves that configure, build, test,
install, and clean leave `git status` empty and that `make dist` produces a
tarball that passes the same scan. `.editorconfig` and `.gitattributes` carry
the matching editor and Git settings (UTF-8, LF, 2-space C indentation, text
world files, binary media).
