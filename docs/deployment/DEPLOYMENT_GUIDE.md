# LuminariMUD Deployment Guide

## Scope

This guide covers a fresh local installation, an existing development build,
and the repository-managed systemd service. MariaDB/MySQL and world data are
required; the server will not run without them.

Production changes require an approved release and operator action. A
development checkout must not modify or restart production.

## Requirements

- Linux or a Linux-compatible environment such as Ubuntu under WSL2
- GCC 13+ or Clang 18+ with GNU C23 support
- GNU Autotools; CMake 3.21+ is supported as a secondary build
- MariaDB/MySQL server and development headers
- crypt, GD, curl, OpenSSL, pthread, and json-c development libraries
- `curl` on a managed host for the systemd readiness probe

Ubuntu, Debian, and WSL2 packages:

```bash
sudo apt-get update
sudo apt-get install -y build-essential git make autoconf automake libtool \
  cmake pkg-config mariadb-server libmariadb-dev libcrypt-dev libgd-dev \
  libcurl4-openssl-dev libssl-dev libjson-c-dev zlib1g-dev mariadb-client \
  curl pandoc gdb valgrind
```

## Fresh Install

The preferred path is the repository deployment script:

```bash
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source
./scripts/deployment/deploy.sh
```

It installs missing dependencies, copies only missing local configuration from
tracked examples, provisions MariaDB, initializes minimal world data, configures
Autotools, builds, and installs `bin/luminari`.

Verified options from `./scripts/deployment/deploy.sh --help`:

| Option | Behavior |
|--------|----------|
| `--auto` | Use defaults without prompts |
| `--dev` | Development build with debugging tools |
| `--prod` | Production profile: optimized, hardened, and verified build |
| `--skip-deps` | Skip dependency installation |
| `--skip-db` | Skip database setup; the server still requires a configured database |
| `--init-world` | Initialize minimal world data; enabled by default |
| `--no-init-world` | Preserve an existing custom world instead of initializing one |
| `--install-systemd` | Install/update the canonical unit and reload systemd |
| `--restart-service` | Restart after unit installation; requires `--install-systemd` |

The script writes generated database credentials to `lib/mysql_config` with
mode 600. Treat its terminal output and that file as sensitive.

## Existing Development Checkout

Autotools is preferred for incremental work:

```bash
make clean
make -j"$(nproc)"
make test
make install
```

If generated build files are missing:

```bash
autoreconf -fvi
./configure
make -j"$(nproc)"
make test
make install
```

The [setup and build guide](../guides/SETUP_AND_BUILD_GUIDE.md) documents fresh
manual configuration and the CMake path.

## Production Build Profile

`deploy.sh --prod` configures the production profile. Both build systems apply
the same contract, produced by `scripts/deployment/production_profile.sh`,
which feature-detects every flag against the selected compiler and reports
anything the toolchain rejects instead of dropping it silently:

```bash
./configure --enable-production
cmake -S . -B build -DLUMINARI_PRODUCTION=ON
```

`configure.ac` makes unknown configure options fatal, so a misspelled or
removed profile can never fall back to the default flags; pass
`--enable-option-checking=warn` only to override that deliberately. With CMake
the profile owns the optimization policy, so leave `CMAKE_BUILD_TYPE` unset;
configuring both is an error.

| Policy | Setting |
|--------|---------|
| Optimization | `-O2` |
| Debug symbols | `-g`; `make install` splits them into `bin/releases/<build-id>/luminari.debug` |
| Assertions | Enabled (no `NDEBUG`); a core file beats running on corrupt state |
| Build ID | Required by the versioned installer and the crash workflow |
| LTO and PGO | Never default; explicit options below |

Hardening set (each flag is requested explicitly and accepted only after a
compile-and-link probe; GCC 14's `-fhardened` bundle is not used because
distributions that already define `_FORTIFY_SOURCE` in the compiler spec make
it reject the very flags it bundles):

- `_FORTIFY_SOURCE=3` with optimization
- PIE (`-fPIE -pie`)
- full RELRO with immediate binding (`-z relro -z now`)
- strong stack protector
- stack-clash protection
- control-flow protection (`-fcf-protection=full`, x86 toolchains)
- non-executable stack (`-z noexecstack`)

Supported GCC and Clang toolchains produce equivalent binaries; configure
prints the enabled set and warns for each unsupported item. On architectures
without `-fcf-protection` that item is reported as unsupported and left out.

The profile also defines `LUMINARI_PRODUCTION_PROFILE`, which makes
`src/constants.c` embed a marker in a `.luminari.profile` ELF section. That
marker is what lets the verifier below distinguish the repository profile from
a distribution whose compiler defaults happen to include the same hardening.

### Verifying the artifact

Every production build must pass the ELF check, which only needs `readelf`:

```bash
./scripts/deployment/verify_hardened_binary.sh bin/luminari
```

It requires the profile marker section, PIE, a non-executable stack, RELRO
with BIND_NOW, stack-protector and fortified libc references, a build ID,
control-flow protection notes on x86-64, and the absence of RPATH/RUNPATH and
text relocations. A binary built with the compiler's default flags fails on the
missing marker even on Ubuntu, whose defaults already supply the other
properties; `test-production-profile` asserts exactly that. `deploy.sh --prod`
runs the check after installation and fails the deployment on any missing
property. The `production-profile` CI matrix (Autotools with GCC, GCC 14, and
Clang; CMake with GCC and Clang) builds the profile, verifies the linked server
and the test binary, runs the full production-linked suite against that exact
hardened artifact, and verifies the installed `bin/luminari`. The Autotools
cells also assert that the retired `--enable-optimizations` option is rejected.
The release workflow builds the same profile and verifies it before publishing.

### Crash symbolization

The installed release keeps full symbols next to the executable. Load them
explicitly when the build ID directory is not in gdb's search path:

```bash
gdb -ex "symbol-file bin/releases/<build-id>/luminari.debug" \
    bin/releases/<build-id>/luminari core
```

`bin/luminari --build-info` and `readelf -n` print the build ID that names the
release directory for any core file or autorun crash archive.

### Explicit LTO and PGO profiles

Link-time optimization and profile-guided optimization are opt-in, and their
effect must be measured against the benchmarks below before they are ever
made default:

```bash
./configure --enable-production --enable-lto
./configure --enable-production --with-pgo-generate=/path/to/profiles
./configure --enable-production --with-pgo-use=/path/to/profiles
cmake -S . -B build -DLUMINARI_PRODUCTION=ON -DLUMINARI_LTO=ON
cmake -S . -B build -DLUMINARI_PRODUCTION=ON -DLUMINARI_PGO_GENERATE=/path/to/profiles
```

GCC uses `-flto=auto`; Clang uses ThinLTO. With Clang, merge the generated
`.profraw` files with `llvm-profdata merge` and pass the resulting
`.profdata` file to `--with-pgo-use` or `LUMINARI_PGO_USE`. A requested LTO or
PGO profile the compiler cannot honor is a configuration error, not a warning.

### Benchmarks

Two stable benchmarks record the cost of a flag change:

- The hot-parser microbenchmark isolates the protocol layer from world,
  database, and scheduler effects. Build it with the flags under test and
  compare the median:

  ```bash
  make -C unittests/CuTest protocol-bench \
      PROTOCOL_BENCH_CFLAGS="-Wall -Wextra -std=gnu23 <profile CFLAGS>" \
      PROTOCOL_BENCH_LDFLAGS="-lm -ljson-c <profile LDFLAGS>"
  ```

- The live game-loop benchmark is the
  [event-core performance gate](EVENT_DRIVEN_CORE_RELEASE_GATE.md), which
  measures scheduler lateness and command round-trip latency on the installed
  artifact. It needs a dedicated database snapshot and an idle host, so CI
  does not run it; it has not yet been recorded under the production profile,
  and that measurement is a precondition for making LTO or PGO default.

Baseline recorded 2026-09-11 on an idle WSL2 host (Ubuntu 24.04, GCC 13.3,
glibc 2.39, x86-64) with `PROTOCOL_BENCH_ITERATIONS=2000`, median of five
rounds over the three corpus inputs:

| Profile | Median per iteration |
|---------|----------------------|
| Default `-O2 -g` | 34.2 us |
| Production (hardened) | 34.6 us |
| Production + LTO | 33.9 us |

Round-to-round spread was about 1 us, so the hardening cost and the LTO gain
are both inside measurement noise on this parser workload. Removing any single
hardening flag did not move the median. This is parser-only evidence: LTO
stays opt-in until the live gate above shows a benefit on the hardened server.
Numbers taken while other builds are running are not comparable; the same
machine measured a 2x slowdown under load.

## Configuration Boundaries

The following real files are local and protected:

- `src/campaign.h`, `src/mud_options.h`, and `src/vnums.h`
- `lib/mysql_config` and `lib/.env`

Their tracked examples are `src/*.example.h`, `lib/mysql_config_example`, and
`lib/.env_example`. Copy an example only on a fresh clone when the real file is
absent. Never commit credentials or replace an existing local configuration.

For manual database creation and schema initialization, use the
[database initialization guide](../guides/DATABASE_INITIALIZATION_GUIDE.md).

## Immutable Installation

`make install` stores the executable and matching debug file under
`bin/releases/<ELF-build-ID>/`, then atomically points `bin/luminari` at the new
release. Existing releases remain available for crash analysis. Autorun records
the active executable and build identity for that analysis. Installation refuses
to replace a live legacy regular `bin/luminari` during the first upgrade.

After testing, verify the installed identity and absence of a root artifact:

```bash
./bin/luminari --build-info
test ! -e ./luminari
```

## Direct Runtime

Start the server against the repository runtime tree:

```bash
./bin/luminari -d lib
```

The checked-in local runtime configuration defaults to game port 4100. The
production systemd unit explicitly supplies port 4100. A final positional port
overrides the default, but reserve that port in the shared inventory before
using it.

Use direct startup for local development. For a supervised local process:

```bash
./scripts/autorun/autorun.sh
./scripts/autorun/autorun.sh status
./scripts/autorun/autorun.sh stop
```

## CI/CD Pipeline

| Workflow | Trigger | Contract |
|----------|---------|----------|
| Code Quality | Push or pull request affecting C sources/config | Formatting, targeted static analysis, warning build |
| Build & Test | Relevant pushes and pull requests | World tools, production tests, hardened production-profile matrix, sanitizers, Valgrind, coverage, and related gates |
| Security | Relevant pushes/PRs, manual, twice monthly | Gitleaks, CodeQL, and PR-only dependency review |
| Integration | Relevant pull requests or manual dispatch | MariaDB schema checks, isolated world/runtime validation, network and health smoke tests |
| GitHub Pages | Documentation push to `master` or manual dispatch | Publishes the `docs/` tree |
| Release | Tag matching `v*.*.*` | Builds and verifies the hardened production profile, then creates GitHub release notes |

The release workflow creates GitHub release metadata; it does not update a
running host. Production deployment remains an explicit operator action.

## Managed systemd Service

The canonical `luminari.service` starts autorun, tracks the supervisor PID, and
runs a bounded readiness check after startup. On an approved host:

```bash
./scripts/deployment/deploy.sh --install-systemd --restart-service
./scripts/autorun/autorun.sh status
./scripts/operations/healthcheck.sh
```

The deployment command succeeds only when systemd and `.autorun.state` agree
with the active immutable executable identity. If systemd is inactive while an
unmanaged autorun is already active, stop that instance before installing the
managed unit.

Common service commands:

```bash
sudo systemctl status luminari.service --no-pager
sudo journalctl -u luminari.service -n 200 --no-pager
sudo systemctl restart luminari.service
```

As of the Phase 00 transition on 2026-08-07, the endpoint, probe, and rendered
unit passed against an isolated local MariaDB runtime. Installing the released
unit, restarting the production service, and probing production readiness still
require an approved operator action; local success is not a production
activation claim.

## Readiness and Liveness

The loopback Terrain API listener defaults to port 8182:

```bash
./scripts/operations/healthcheck.sh
curl -fsS http://127.0.0.1:8182/health/live
```

`/health` and `/health/ready` require both the initialized game loop and
MariaDB. `/health/live` does not query MariaDB. When the listener port changes,
set both `TERRAIN_API_PORT` and the matching `LUMINARI_HEALTH_URL`. See the
[health API contract](../api/README_api.md).

## Rollback Boundary

The repository does not implement one general application rollback command.
Before a production change, preserve the current immutable release identity,
database backup, world data, and service state. Use the approved host procedure
to reactivate a prior release, and use only subsystem rollback scripts whose
deployment guides specify their order and validation.

Never improvise a rollback by deleting `bin/releases/`, overwriting a running
executable, or applying database scripts without their component runbook.

## Troubleshooting

### Build Configuration Missing

```bash
autoreconf -fvi
./configure
```

### Installed Binary Missing

```bash
make install
test -x ./bin/luminari
```

### Database Unavailable

```bash
sudo systemctl status mariadb --no-pager
./scripts/operations/healthcheck.sh
```

Review `lib/mysql_config` without copying its values into logs or issue text.

### Port Already in Use

```bash
sudo lsof -i :4100
./scripts/autorun/autorun.sh status
```

Stop the owning service or supervisor through its normal control command; do
not send an unconditional `SIGKILL` to an unresolved PID.

### Crash or Failed Startup

Follow the [incident response runbook](../runbooks/incident-response.md) and
[troubleshooting guide](../guides/TROUBLESHOOTING_AND_MAINTENANCE.md). Preserve
the crash archive and its matching immutable executable before rebuilding.

## Related Documentation

- [Event-driven core release gate](EVENT_DRIVEN_CORE_RELEASE_GATE.md)
- [Phase 00 security and privacy assessment](../testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md#security-and-privacy-assessment)
- [Environment boundaries](environments.md)
- [Database deployment](DATABASE_DEPLOYMENT_GUIDE.md)
- [Testing guide](../guides/TESTING_GUIDE.md)
- [Incident response](../runbooks/incident-response.md)

Last updated: 2026-09-11
