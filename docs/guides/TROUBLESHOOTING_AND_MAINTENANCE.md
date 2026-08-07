# LuminariMUD Troubleshooting and Maintenance

Start with read-only checks and preserve evidence before changing processes,
binaries, world files, or database state. Production actions require an
approved operator and rollback plan.

## Quick Diagnosis

```bash
./scripts/autorun/autorun.sh status
sudo systemctl status luminari.service --no-pager
curl -fsS http://127.0.0.1:8182/health/live
./scripts/operations/healthcheck.sh
```

- Liveness failure: the initialized game loop is not servicing the local
  listener.
- Liveness success and readiness failure: the game loop is active, but the
  required MariaDB connection is unhealthy.
- Both succeed: inspect the affected gameplay subsystem and logs; the probes
  do not exercise every feature.

For containment, recovery, and evidence rules, use the
[incident response runbook](../runbooks/incident-response.md).

## Build Problems

### Missing `configure` or `Makefile`

```bash
autoreconf -fvi
./configure
```

### Missing Local Headers

On a fresh clone only, create files that do not already exist:

```bash
test -e src/campaign.h || cp src/campaign.example.h src/campaign.h
test -e src/mud_options.h || cp src/mud_options.example.h src/mud_options.h
test -e src/vnums.h || cp src/vnums.example.h src/vnums.h
```

Never overwrite or commit those local headers.

### Clean Rebuild and Test

```bash
make clean
make -j"$(nproc)"
make test
make install
```

There is no `test_runner`. The root `make test` target builds the
production-linked CuTest executable and registered shell regressions. The
required install step activates `bin/circle` and removes the root-level build
artifact.

### CMake Test Target Missing

Tests are disabled by default in a fresh CMake tree:

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

## Startup Problems

### Installed Binary Missing or Stale

```bash
make install
test -x ./bin/circle
./bin/circle --build-info
test ! -e ./circle
```

### Database Connection Failure

MariaDB/MySQL is required:

```bash
sudo systemctl status mariadb --no-pager
./scripts/operations/healthcheck.sh
```

Check permissions and values in local `lib/mysql_config` without printing them
into logs, chat, or issue text. Use
[DATABASE_INITIALIZATION_GUIDE.md](DATABASE_INITIALIZATION_GUIDE.md) for schema
setup.

### Missing World or Text Data

The server requires the indexes and data under `lib/world/` and `lib/text/`.
For a fresh local environment, let the deployment script create the minimal
runtime:

```bash
./scripts/deployment/deploy.sh --auto --init-world
```

Do not run initialization over an existing custom or production world.

### Port Conflict

The checked-in default game port is 4100 and the loopback health port is 8182:

```bash
sudo lsof -i :4100
sudo lsof -i :8182
./scripts/autorun/autorun.sh status
```

Resolve ownership before stopping a process. Use systemd or autorun control,
not an unconditional `SIGKILL`. A direct development server accepts another
game port as its final positional argument:

```bash
./bin/circle -d lib 4200
```

Set `TERRAIN_API_PORT` and the matching `LUMINARI_HEALTH_URL` when changing the
health listener.

## Logs and Crash Evidence

Direct output and the current autorun log may appear in root `syslog`; rotated
logs are under `log/`. Autorun crash evidence is under `dumps/` and identifies
the matching immutable release.

```bash
tail -200 syslog
find dumps -maxdepth 2 -type f -print
./scripts/autorun/autorun.sh status
```

Do not rebuild or remove `bin/releases/<ELF-build-ID>/` before preserving the
executable and `circle.debug` used by the crashed process.

### GDB

Use the maintained development helper:

```bash
./scripts/debugging/debug_game.sh
```

For production core analysis, follow the exact executable identity and capture
procedure in the [incident runbook](../runbooks/incident-response.md).

### Valgrind and Sanitizers

The root suite has dedicated CI sanitizer and Valgrind jobs. The focused
protocol harness can be run locally with:

```bash
cd unittests/CuTest
make valgrind-protocol
```

Existing process-lifetime allocations can make a live server leak report differ
from the focused suite. Record the command, lifecycle, and suppression settings
with every result.

## World Data Diagnosis

Use the read-only world tool before modifying authored content:

```bash
python scripts/world/wtool.py --help
python scripts/world/wtool.py validate --help
```

Preserve the exact failing file and boot log. Do not rename or delete a
production file to bypass a parser error; restore only from an identified
backup after validating references.

## Routine Maintenance

- Keep MariaDB, filesystem, and log capacity visible to the approved operator.
- Run the repository test/install gate before deploying a code change.
- Preserve an identified database backup, world-data snapshot, active build
  identity, and rollback plan before a production update.
- Verify managed process identity with autorun status after restart.
- Require database-backed readiness before reopening service.
- Record proven remedies in this guide or the incident runbook; do not add
  invented schedules, contacts, or service-level promises.

## References

- [Setup and build](SETUP_AND_BUILD_GUIDE.md)
- [Deployment guide](../deployment/DEPLOYMENT_GUIDE.md)
- [Environment boundaries](../environments.md)
- [Health API contract](../api/README_api.md)
- [Testing guide](TESTING_GUIDE.md)

Last updated: 2026-08-07
