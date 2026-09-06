# Development Guide and Onboarding

This page is the concise command and source map for day-to-day development.
Detailed APIs and subsystem behavior live in the
[developer reference](../guides/DEVELOPER_GUIDE_AND_API.md).

## Required Tools

- Linux, a Linux-compatible Unix environment, or Ubuntu under WSL2
- GCC 13+ or Clang 18+ with GNU C23 support
- Autoconf, Automake, Make, and the repository's configured Autotools files
- MariaDB/MySQL server and client development headers
- crypt, GD, curl, OpenSSL, pthread, and json-c development libraries
- CMake 3.21+ for the supported secondary build
- GDB and Valgrind for debugging and memory checks

## Onboarding

This checklist takes a fresh development checkout from clone to a tested local
server. Use the required tools above; the automated setup installs supported
Ubuntu/Debian dependencies. The [setup and build guide](../guides/SETUP_AND_BUILD_GUIDE.md)
covers exact packages and manual setup. For player and builder orientation, use
[Getting Started](../GETTING_STARTED.md).

### Setup

1. Clone and enter the repository:

   ```bash
   git clone https://github.com/LuminariMUD/Luminari-Source.git
   cd Luminari-Source
   ```

2. Run the one-command development setup:

   ```bash
   ./scripts/deployment/deploy.sh --dev
   ```

   The script prepares missing local configuration, MariaDB, minimal world
   data, the Autotools build, and the installed executable. It does not make
   local credential files safe to commit; `lib/mysql_config` and `lib/.env`
   remain protected local files.

3. Verify the repository gate:

   ```bash
   make test
   make install
   ```

4. Start the server and verify readiness:

   ```bash
   ./bin/luminari -d lib
   ./scripts/operations/healthcheck.sh
   ```

   Run the health check from a second terminal while the server is active.
   The local game and health ports are 4101 and 8182, respectively. Production
   explicitly uses game port 4100 through `luminari.service`.

### Read Before Editing

- [Contributing rules](../../CONTRIBUTING.md)
- [Development commands and repository map](#common-commands)
- [Architecture](../systems/ARCHITECTURE.md)
- [Testing guide](../guides/TESTING_GUIDE.md)
- [Technical documentation index](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)

Local configuration headers and credential files are intentionally ignored.
Never overwrite an existing local configuration with its example template.

## Common Commands

| Command | Purpose |
|---------|---------|
| `make clean && make -j"$(nproc)"` | Rebuild the configured Autotools tree |
| `make test` | Run production-linked CuTest and registered shell regressions |
| `make install` | Activate the tested immutable release as `bin/luminari` |
| `./bin/luminari -d lib` | Run the server with repository runtime data |
| `./scripts/debugging/debug_game.sh` | Start the maintained GDB helper |
| `./scripts/operations/healthcheck.sh` | Check database-backed local readiness |
| `python scripts/world/wtool.py --help` | Inspect the read-only world-data tool surface |

If `configure` or `Makefile` is missing, initialize Autotools first:

```bash
autoreconf -fvi
./configure
```

## CMake Validation

Tests are opt-in for a fresh CMake tree:

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
cmake --install build
```

## Test Surfaces

The root `make test` target is authoritative for behavior linked against all
game sources. There is no `test_runner` binary and CuTest has no
per-test-function filter. The focused protocol parser harness is available at:

```bash
cd unittests/CuTest
make protocol-parser
make test-all
```

See [TESTING_GUIDE.md](../guides/TESTING_GUIDE.md) for the complete suite, schema,
world-tool, sanitizer, Valgrind, and subsystem commands.

## Source Map

- `src/comm.c`, `src/interpreter.c`, `src/db.c`, `src/handler.c`, and
  `src/utils.c` form the server core.
- Feature directories under `src/` are one level deep. Put a file where its
  primary responsibility belongs; do not introduce second-level source trees.
- Spells and skills share the number space and live under `src/magic/`.
- Combat behavior lives under `src/combat/`; movement commands live under
  `src/movement/`; OLC lives under `src/olc/`.
- Headers inside a feature directory use path-qualified includes from outside
  that directory.
- Every source addition or removal updates both `Makefile.am` and
  `CMakeLists.txt`.

## Local Configuration

On a fresh clone only, copy missing examples to their local paths:

```bash
test -e src/campaign.h || cp src/campaign.example.h src/campaign.h
test -e src/mud_options.h || cp src/mud_options.example.h src/mud_options.h
test -e src/vnums.h || cp src/vnums.example.h src/vnums.h
test -e lib/mysql_config || install -m 600 lib/mysql_config_example lib/mysql_config
```

Never overwrite or commit those local files. Edit the example only when the
shared template contract changes.

## Code and Documentation Style

Use 2-space indentation, Allman braces, declarations at block starts,
`lower_snake_case` identifiers, `UPPER_SNAKE_CASE` constants, and bounded
string functions. Do not mechanically restyle legacy code. Documentation and
helpfiles must be ASCII, UTF-8, and LF.

## Further Development References

- [CMake build guide](CMAKE_BUILD_GUIDE.md)
- [Data structures and memory](DATA_STRUCTURES_AND_MEMORY.md)
- [Engineering conventions](CONVENTIONS.md)
- [Design and maintenance considerations](CONSIDERATIONS.md)
