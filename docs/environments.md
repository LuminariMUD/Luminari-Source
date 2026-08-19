# LuminariMUD Environments

This repository distinguishes development and production by local operator
configuration and deployment procedure. It does not define a repository-managed
staging host, staging URL, staging database, or `--staging` deployment mode.

## Environment Boundaries

| Environment | Verified repository contract |
|-------------|------------------------------|
| Development | Local checkout; `lib/.env` declares `APP_ENV=development`; direct or autorun startup against non-production data |
| Production | Self-managed systemd service using `luminari.service`; changes require an approved release and operator action |

`APP_ENV` is a safety marker used by development and vessel scripts. It is not
a substitute for runtime database or server configuration. Scripts that require
development state fail closed when the marker is absent or not `development`.

## Local Configuration Files

| Local file | Tracked example | Purpose |
|------------|-----------------|---------|
| `src/campaign.h` | `src/campaign.example.h` | Local Luminari compile-time settings |
| `src/mud_options.h` | `src/mud_options.example.h` | Compile-time game options |
| `src/vnums.h` | `src/vnums.example.h` | Symbolic virtual-number configuration |
| `lib/mysql_config` | `lib/mysql_config_example` | Required MariaDB/MySQL connection |
| `lib/.env` | `lib/.env_example` | Environment marker and local script configuration |

These real files are ignored and may contain credentials or host-specific
values. Never overwrite an existing copy from its example and never commit its
contents.

## Development

Use the automated development build on a fresh checkout:

```bash
./scripts/deployment/deploy.sh --dev
```

For a configured checkout, use the standard gate:

```bash
make clean
make -j"$(nproc)"
make test
make install
./bin/circle -d lib
```

Development may use isolated minimal world data and a local test database. CI
creates its own guarded runtime and refuses protected repository `lib/`,
non-loopback database hosts, and database names without a test or CI marker.

## Production

The canonical production topology is a self-managed systemd unit supervising
`scripts/autorun/autorun.sh`. It requires MariaDB and uses a post-start local
readiness probe.

```bash
./scripts/deployment/deploy.sh --install-systemd --restart-service
./scripts/autorun/autorun.sh status
./scripts/operations/healthcheck.sh
```

Run those commands only on the approved host. Development checkouts do not
install, restart, or probe production.

## Runtime Variables

| Variable | Default | Consumer |
|----------|---------|----------|
| `MUD_PORT` | `4100` | Autorun game port |
| `MUD_FLAGS` | `-q` | Autorun server flags |
| `TERRAIN_API_PORT` | `8182` | Loopback Terrain and health listener |
| `LUMINARI_HEALTH_URL` | `http://127.0.0.1:8182/health` | Readiness script |
| `LUMINARI_HEALTH_REQUEST_TIMEOUT_SECONDS` | `3` | Per-request timeout |
| `LUMINARI_HEALTH_TIMEOUT_SECONDS` | `90` | Total readiness wait |
| `LUMINARI_HEALTH_INTERVAL_SECONDS` | `2` | Readiness retry interval |

When `TERRAIN_API_PORT` changes, set `LUMINARI_HEALTH_URL` to the matching
loopback URL. Valid listener ports are 1025 through 65535.

## Data and Security

- MariaDB/MySQL is required in every runnable environment.
- Development tests use synthetic or isolated data; they must never target a
  production database or the protected runtime tree.
- Production player data, world data, credentials, and service state require
  an approved operational change and backup/rollback plan.
- The health listener is loopback-only. Do not expose it through a public
  proxy without a separately reviewed authentication and transport design.
- Phase 00 special-procedure diagnostics contain static binding names, source
  locations, owner types, and prototype VNUMs. Keep them bounded, single-line,
  and free of player or account values as later gateway context is added.

See [Deployment and CI/CD](deployment.md), the detailed
[deployment guide](deployment/DEPLOYMENT_GUIDE.md), and the
[incident runbook](runbooks/incident-response.md). The targeted Phase 00
security and privacy result is recorded in the
[validation matrix](testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md#security-and-privacy-assessment).
