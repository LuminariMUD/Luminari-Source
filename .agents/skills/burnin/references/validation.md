# Luminari burn-in validation

Run commands from the repository root with the development MUD stopped. Inspect current
`Makefile.am`, `CMakeLists.txt`, `.github/workflows/test.yml`, and
`docs/guides/TESTING_GUIDE.md` when deciding what a gate covers. Do not use fixed historical test
counts as acceptance criteria.

## Isolated database and boot fixture

The root production-linked suite has opt-in database cases and a real syntax-boot case. Prepare
both; otherwise a green suite can omit database coverage or boot the configured game data.

Provision a uniquely named disposable MariaDB instance/database with test-only credentials.
Use an isolated local daemon or Docker as available; Docker is not itself a required gate.
Never repurpose the configured game database. The character-rename schema script and help-sync
integration tests independently create temporary local MariaDB datadirs, so those gates also
need `mariadb-install-db` and `mariadbd`; help integration needs Python `pymysql`.

Set these variables in the test command environment, not in protected configuration files:

| Variable | Required value |
|----------|----------------|
| `LUMINARI_TEST_MYSQL_ENABLE` | `1` |
| `LUMINARI_TEST_MYSQL_HOST` | The verified disposable loopback endpoint |
| `LUMINARI_TEST_MYSQL_USER` | A disposable test user |
| `LUMINARI_TEST_MYSQL_PASSWORD` | Its test-only password |
| `LUMINARI_TEST_MYSQL_DATABASE` | Its unique database name containing `test` or `ci` |
| `LUMINARI_TEST_MYSQL_PORT` | The fixture's TCP port, subject to the boot constraint below |
| `LUMINARI_HELP_SYNC_INTEGRATION` | `1` |

The server connection pool currently passes port zero to `mysql_real_connect()` and its
`mysql_config` parser has no `mysql_port` key. `prepare_test_runtime.sh` uses the test-port
variable for SQL provisioning but does not write it into the game configuration. Therefore a
random Docker port mapping alone cannot qualify the syntax boot. Use a verified free loopback
address at the client's default TCP port, normally 3306, or another explicitly verified
isolation arrangement. For example, a fixture on an available `127.0.0.2:3306` can coexist with
runtime MariaDB on `127.0.0.1:3306`; first check for wildcard listeners and actual ownership.
Do not stop the existing runtime database to make room for a test fixture.

Create a unique runtime directory and export the boot overrides in the same test environment:

```bash
burnin_dir=$(mktemp -d "$PWD/.burnin-runtime-XXXXXXXX")
burnin_relative=${burnin_dir#"$PWD/"}
export LUMINARI_TEST_DATA_DIR="$burnin_dir/lib"
export LUMINARI_TEST_CONFIG_FILE="$burnin_relative/lib/etc/config"
export LUMINARI_TEST_ROOT="$PWD"
export LUMINARI_TEST_SPEC_WORLD_ROOT="$PWD/unittests/CuTest/fixtures/spec_world_inventory"
export LUMINARI_HELP_SYNC_INTEGRATION=1
unset LUMINARI_TEST_SKIP_SYNTAX_BOOT
scripts/ci/prepare_test_runtime.sh "$LUMINARI_TEST_DATA_DIR"
```

Supply the six database variables before running this block. The preparer imports
`sql/master_schema.sql`, seeds the encounter fixture, and copies the maintained minimal world
and artifacts into the isolated directory. It rejects protected `lib/` paths and non-test DB
targets. Never remove those guards.

`LUMINARI_TEST_CONFIG_FILE` must be a safe relative path from the root command's working
directory, with no absolute path or `..` traversal. Argument/configuration parsing happens
before the later `-d` directory switch; `-d` alone does not select the fixture configuration.
The data-directory override can be absolute. Preserve this distinction in direct syntax boots.

## Clean build and authoritative regression gate

Use the compiler and dependencies selected by this checkout's configuration. LuminariMUD is the
only supported game identity; do not construct a build matrix for retired campaign variants.
GNU C23 and libevent are required; diagnose missing headers/libraries through the actual build
configuration and `pkg-config`, not another session's temporary paths. If configuration is absent,
run `autoreconf -fvi` and `./configure` without overwriting existing local headers.

Run each command only after its prerequisites succeed. If combining commands in a Bash script,
use `set -euo pipefail` so a failed build cannot be followed by tests against a stale executable.

```bash
pre-commit run clang-format --all-files --show-diff-on-failure
make clean
make -C unittests/CuTest clean
make -j"$(nproc)" all cutest
LUMINARI_IO_DRIVER=libevent make test-all
```

The formatting hook uses the version and exclusions in `.pre-commit-config.yaml` and can modify
files; inspect and retain justified formatting repairs, then repeat the gate. `make clean`
covers the main server, root tests, and maintained `util/` programs. The separate harness
needs its own clean. Do not use `scrub` as a build clean: it removes logs and runtime evidence.

`make test-all` runs root CuTest and registered shell regressions, world tools/documentation,
the focused source-linked protocol parser harness, process-memory tooling, character-rename static
checks and its isolated MariaDB schema test, then `make install`. Setting
`LUMINARI_HELP_SYNC_INTEGRATION=1` enables the isolated help DB cases discovered by `make test`;
`make test-help-sync-integration` can rerun that gate explicitly. CuTest has no function filter
and there is no `test_runner` executable.

Always run `make install` after a root `make test` or an interrupted/failed `make test-all` once
the normal server build is valid and work is authorized to continue. Installation preserves an
immutable release and debug sidecar under `bin/releases/`, updates `bin/luminari`, and removes
root `luminari`. Do not manually delete or replace releases to satisfy artifact checks.

The install hook also runs `scripts/provision_artifacts.sh`, which adds missing artifact records
and index entries under `lib/world/` and `lib/text/help/`. Existing OLC-edited records remain
authoritative. Include these additions in the runtime change record; installation is not limited
to copying the executable.

Run the entire production suite with the other supported I/O driver, keeping the same isolated
fixture and real syntax boot enabled:

```bash
LUMINARI_IO_DRIVER=select ./cutest
test -L bin/luminari
test -x bin/luminari
test ! -e luminari
./bin/luminari --build-info
```

The native scheduler is the supported timing backend. Do not invent a legacy timing-backend
matrix from obsolete examples. Review expected negative-test diagnostics against their owning
tests. Inspect skips explicitly, including those for ignored RoL corpus/conversion artifacts;
missing external data is a coverage gap, not a reason to synthesize passing historical inputs.

## CMake, sanitizers, and memory checks

Use a fresh separate CMake directory so instrumentation cannot contaminate the normal Autotools
release. Reuse the isolated test environment above. This build exercises the supported CMake
path and all maintained utilities as well as the production-linked tests:

```bash
cmake -S . -B "$burnin_dir/cmake-asan" -DBUILD_TESTS=ON -DBUILD_UTILS=ON \
  -DCMAKE_C_FLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build "$burnin_dir/cmake-asan" -j"$(nproc)"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
LUMINARI_TEST_SKIP_SYNTAX_BOOT=1 \
  ctest --test-dir "$burnin_dir/cmake-asan" --output-on-failure
make -C unittests/CuTest protocol-fuzz FUZZ_SECONDS=30
LUMINARI_TEST_SKIP_SYNTAX_BOOT=1 valgrind \
  --leak-check=full --show-leak-kinds=definite --errors-for-leak-kinds=definite \
  --track-origins=yes --error-exitcode=1 ./cutest
make -C unittests/CuTest valgrind-protocol
```

The syntax-boot skip is restricted to the specialized instrumentation runs, matching CI;
the normal suite must already have proven that boot. Do not install the instrumented release
over the normal binary. Targeted utility or subsystem regressions may need their own sanitizer
invocation; building an instrumented utility does not prove its behavior was exercised.
Retain sanitizer and Valgrind diagnostics, including leak categories; do not add suppressions
or weaken checks simply to make them green.

After repairs, repeat the fresh-build and validation sequence. Preserve full logs and each
command's exit status. Keep test environment variables scoped to test commands; the final
autorun must use this checkout's normal development data and tested normal release.
