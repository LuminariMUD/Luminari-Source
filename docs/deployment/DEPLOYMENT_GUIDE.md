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
  libcurl4-openssl-dev libssl-dev libjson-c-dev gdb valgrind
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
Autotools, builds, and installs `bin/circle`.

Verified options from `./scripts/deployment/deploy.sh --help`:

| Option | Behavior |
|--------|----------|
| `--auto` | Use defaults without prompts |
| `--dev` | Development build with debugging tools |
| `--prod` | Optimized production build |
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
`bin/releases/<ELF-build-ID>/`, then atomically points `bin/circle` at the new
release. Existing releases remain available for crash analysis. Installation
refuses to replace a live legacy regular `bin/circle` during the first upgrade.

After testing, verify the installed identity and absence of a root artifact:

```bash
./bin/circle --build-info
test ! -e ./circle
```

## Direct Runtime

Start the server against the repository runtime tree:

```bash
./bin/circle -d lib
```

The checked-in runtime configuration defaults to game port 4100. Supply a
different port as the final positional argument when required:

```bash
./bin/circle -d lib 4200
```

Use direct startup for local development. For a supervised local process:

```bash
./scripts/autorun/autorun.sh
./scripts/autorun/autorun.sh status
./scripts/autorun/autorun.sh stop
```

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
test -x ./bin/circle
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

- [Deployment and CI/CD overview](../deployment.md)
- [Environment boundaries](../environments.md)
- [Database deployment](DATABASE_DEPLOYMENT_GUIDE.md)
- [Testing guide](../guides/TESTING_GUIDE.md)
- [Incident response](../runbooks/incident-response.md)

Last updated: 2026-08-07
