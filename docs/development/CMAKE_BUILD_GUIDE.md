# LuminariMUD CMake Build Guide

## Overview

CMake is the supported secondary build. Autotools remains preferred for
incremental development, but both systems must compile the same production
and production-linked test sources with the same feature definitions, and CI
builds and tests CMake with GCC and Clang on every change.

LuminariMUD uses GNU C23. The build retains the established source formatting
and declaration style.

## Prerequisites

| Dependency | Minimum | Ubuntu package |
| -- | -- | -- |
| CMake | 3.21 | `cmake` |
| GCC or Clang with GNU C23 | GCC 13 / Clang 16 | `gcc` or `clang` |
| pkg-config | any | `pkg-config` |
| libevent core | 2.1.12 | `libevent-dev` |
| MariaDB Connector/C (or MySQL client) | 3.1 | `libmariadb-dev` |
| json-c | 0.13 | `libjson-c-dev` |
| GD | 2.2 | `libgd-dev` |
| libcurl | 7.68 | `libcurl4-openssl-dev` |
| OpenSSL | 1.1.1 | `libssl-dev` |
| libcrypt | any | `libcrypt-dev` |
| Python | 3.10 | `python3` |

Every library above is required. Autotools links all of them unconditionally,
so CMake resolves each through an imported target (`PkgConfig::LIBEVENT`,
`PkgConfig::JSONC`, `PkgConfig::GD`, `PkgConfig::MARIADB` or
`PkgConfig::MYSQLCLIENT`, `CURL::libcurl`, `OpenSSL::SSL`, `OpenSSL::Crypto`,
`Threads::Threads`) and fails configuration when one is missing. Libraries
installed outside the default search paths are found through
`PKG_CONFIG_PATH` or `CMAKE_PREFIX_PATH`. The one exception is libcrypt,
which ships neither a pkg-config file nor a CMake package: it is resolved
with `find_library(CRYPT_LIBRARY crypt REQUIRED)`, so `CRYPT_LIBRARY` is the
only library cache entry you may need to set by hand. `nsl`, `socket`, and
`dmalloc` use the same mechanism but are only probed on platforms (or with
options) that need them.

## Quick Start

```bash
# Configure required headers (one-time setup on a fresh clone)
cp src/config/campaign.example.h src/config/campaign.h
cp src/config/mud_options.example.h src/config/mud_options.h
cp src/config/vnums.example.h src/config/vnums.h

cmake --preset dev
cmake --build --preset dev -j"$(nproc)"
ctest --preset dev
cmake --install build/dev
```

`cmake --install` promotes `build/<preset>/bin/luminari` into an immutable
build-ID release and activates `bin/luminari`.

## Presets

`CMakePresets.json` defines the supported configurations. Every configure
preset writes to `build/<preset>`; each one except `analysis`, which only
exports a compilation database, has a matching `--build` preset and, where tests
apply, a `ctest` preset.

| Preset | Purpose |
| -- | -- |
| `dev` | Debug build with tests and utilities, system compiler |
| `dev-clang` | The same with `clang` |
| `ci-gcc` | RelWithDebInfo, baseline warning tier with `-Werror`, blocking in CI (Debug and Release, GCC 13 and 16.2) |
| `ci-clang` | The same with `clang` (Clang 18 and 22.1.8), blocking in CI |
| `analysis` | RelWithDebInfo with `clang`, tests and utilities; exports `compile_commands.json` for `scripts/ci/check_clang_tidy.py` without building |
| `sanitizers` | Debug with `-fsanitize=address,undefined` |
| `coverage` | Debug with `--coverage` for gcov/gcovr |
| `release-hardened` | Release with fortify, stack protector, PIE, RELRO, no tests |
| `cross-aarch64` | Release cross build through `cmake/toolchains/aarch64-linux-gnu.cmake` |

Personal variations belong in the gitignored `CMakeUserPresets.json`.

### Cross builds

The `cross-aarch64` preset uses the GNU cross compiler
(`gcc-aarch64-linux-gnu`) and a sysroot containing the aarch64 versions of the
libraries listed above:

```bash
cmake --preset cross-aarch64 -DLUMINARI_SYSROOT=/srv/sysroots/aarch64
cmake --build --preset cross-aarch64 -j"$(nproc)"
```

The toolchain file redirects pkg-config into the sysroot. Copy it to add
another target triple. No GitHub Actions job exercises this preset: the hosted
runners have no aarch64 sysroot, and a configure-only run against host
`.pc` files would prove nothing. Cross builds are verified by hand on a
machine that has the sysroot.

## Options

All options are declared in `CMakeLists.txt` and printed in the configuration
summary.

| Option | Default | Effect |
| -- | -- | -- |
| `BUILD_UTILS` | `ON` | Build the `util/` helper programs |
| `BUILD_TESTS` | `OFF` | Build `cutest` and register the CTest entries |
| `LUMINARI_WARNING_TIER` | `baseline` | `baseline`, `migration`, or `analysis`; see the compiler policy in the [setup and build guide](../guides/SETUP_AND_BUILD_GUIDE.md#compiler-policy-and-warning-tiers) |
| `MEMORY_DEBUG` | `OFF` | Define `MEMORY_DEBUG` for the in-tree allocation tracing |
| `DMALLOC` | `OFF` | Define `DMALLOC` and link the dmalloc allocator (required when set) |
| `STATIC_ANALYSIS` | `OFF` | Run clang-tidy on every compiled source and export compile commands; CI compares findings with a baseline instead (see [Static Analysis](../guides/SETUP_AND_BUILD_GUIDE.md#static-analysis)) |
| `LUMINARI_WERROR` | `OFF` | Add `-Werror`; accepted only with the `baseline` tier |
| `LUMINARI_COVERAGE` | `OFF` | Add `--coverage` to compile and link |
| `LUMINARI_HARDENING` | `OFF` | Add `_FORTIFY_SOURCE=3`, `-fstack-protector-strong`, `-fstack-clash-protection`, PIE, RELRO, and `-z now` |
| `LUMINARI_SANITIZERS` | empty | Comma-separated `-fsanitize=` list, for example `address,undefined` |

Options compose with any preset. The `migration` tier is deliberately not
enabled by a checked-in preset because `-Wconversion` alone reports thousands
of pre-existing warnings; turn it on for a focused pass, then lower the
budget file with `scripts/ci/check_warning_budget.py --update`:

```bash
cmake --preset dev -DLUMINARI_WARNING_TIER=migration
```

The warning flags come from `scripts/deployment/production_profile.sh`, the
same probe Autotools uses, so both builds print and apply one list.

Every flag, definition, include path, and link option is attached to the
`luminari_build` and `luminari_deps` interface targets, which the server,
`cutest`, and every utility link. `STATIC_ANALYSIS` likewise sets the
`C_CLANG_TIDY` property on each executable target instead of the
directory-wide `CMAKE_C_CLANG_TIDY` variable. The build never edits
`CMAKE_C_FLAGS` or global include directories, so `CMAKE_C_FLAGS` remains
free for caller additions:

```bash
cmake --preset dev -DCMAKE_C_FLAGS="-march=native"
```

## Feature definitions

Configuration writes the same platform macros `configure.ac` produces into
`conf.h` in the build directory, from `cmake/cmake_config.h.in`, beside the
generated `build_identity.h`. Autotools generates both headers in its build
root. CTest points the standalone scripts that preprocess sources at the build
directory through `CPPFLAGS`.

The two headers differ only in macros no source consumes: Autotools also
emits `PACKAGE_*`, `VERSION`, and `HAVE_EVENT2_EVENT_H`. Fallback `pid_t`,
`size_t`, `ssize_t`, and `socklen_t` definitions are written into `conf.h` by
both systems. The obsolete Autoconf `TIME_WITH_SYS_TIME` probe is defined by
neither; `sysdep.h` includes `<time.h>` unconditionally alongside
`<sys/time.h>` when that header exists.

## Manifest parity

`Makefile.am` and `CMakeLists.txt` list sources by hand. The check

```bash
python3 scripts/ci/check_build_parity.py
```

fails when `luminari_SOURCES`/`SRC_C_FILES`,
`cutest_test_files`/`CUTEST_TEST_SOURCES`, or the world-tool and help-sync
lists differ, contain duplicates, or name files that are missing or untracked.
Variable references are expanded before comparison (`$(luminari_SOURCES)`
inside `cutest_SOURCES`, `${SRC_FILES}` inside the CMake `cutest` target), so
the check also verifies that both production-linked suites compile exactly the
harness, every test file, and every production source, and that CMake compiles
the generated `AllTests.c` registry. Removing `$(luminari_SOURCES)` from
`cutest_SOURCES`, or `${SRC_FILES}` from `add_executable(cutest ...)`, fails
the check. It runs as the `build-parity` CTest entry, through
`make check-build-parity` (part of `make test`), and as a blocking GitHub
Actions job.

## Distribution check

```bash
make distcheck-archive
# or
scripts/ci/check_clean_archive.sh
```

exports `git archive HEAD` into a temporary directory and, for each build
system in turn, configures, builds, runs that system's full test entry point
(`make test`, then `ctest` for the CMake preset), and installs. It proves that
no untracked local file or generated artifact is required. The blocking
`Clean archive, both build systems` GitHub Actions job runs this script after
`scripts/ci/prepare_test_runtime.sh` has provisioned the isolated MariaDB
runtime; a plain checkout is not a substitute, because it still contains
`.git` and any local build output. The script is not part of `make test`
because it rebuilds the whole tree twice; run it before tagging a release or
whenever the manifests, install scripts, or generated headers change. Without
the MariaDB runtime, export `LUMINARI_TEST_SKIP_SYNTAX_BOOT=1` so the boot
test skips instead of failing on missing world data.

The two test entry points are not identical. Autotools `make test` and CTest
both run the production-linked CuTest suite, the manifest parity check, the
help-sync, SQL interpolation, and background-help checks, and the event,
autorun, versioned-install, binary-name, and healthcheck scripts. CTest
additionally runs the world-data tool tests, the `bsd-snprintf` fallback
test, and the vessel and process-memory tooling checks that Autotools
exposes through `make test-all` and `make test-vessel-tooling`.

## Build Targets

```bash
cmake --build build/dev --target luminari
cmake --build build/dev --target cutest
cmake --build build/dev --target test-world-tools
cmake --build build/dev --target test-help-sync
cmake --build build/dev --target clean
```

## IDE Integration

Any IDE that reads `CMakePresets.json` (VS Code with the CMake Tools
extension, CLion) lists the presets above directly. For clangd, configure the
`analysis` preset or enable `CMAKE_EXPORT_COMPILE_COMMANDS`.

## Troubleshooting

### Missing dependency

Configuration stops with the pkg-config module or package name that was not
found. Install the matching development package from the table above, or set
`PKG_CONFIG_PATH` to a prefix that contains its `.pc` file.

### Configuration headers missing

```
CMake Error: campaign.h not found!
```

Copy the three `.example.h` templates as shown in the quick start. Never
overwrite an existing local header. If configuration instead stops because a
local header must move to `src/config/`, move the existing headers:

```bash
mkdir -p src/config && mv -n src/{campaign,mud_options,vnums}.h src/config/
```

### Source style

GNU C23 accepts `//` comments, but project style continues to use `/* */`
comments. Existing code is not mechanically restyled.

## See Also

- [Setup and Build Guide](../guides/SETUP_AND_BUILD_GUIDE.md)
- [Testing Guide](../guides/TESTING_GUIDE.md)
- [Developer Guide](../guides/DEVELOPER_GUIDE_AND_API.md)
