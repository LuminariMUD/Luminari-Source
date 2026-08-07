# Project scripts

Run commands from the project root unless a script says otherwise. Scripts resolve the
project root from their own location where they need repository files.

## Subject directories

| Directory | Purpose |
|-----------|---------|
| `autorun/` | Server supervisor, watchdog, and supervision regression test |
| `character-rename/` | Static and MariaDB-backed character rename tests |
| `copyover/` | Copyover monitoring and diagnostics |
| `debugging/` | GDB and Valgrind helpers |
| `deployment/` | Deployment, setup, and binary installation |
| `development/` | Local test-character and login helpers |
| `mariadb/` | Local MariaDB startup and sudoers setup |
| `operations/` | HTTP readiness probe and its regression test |
| `permissions/` | Permission inspection and repair |
| `process-memory/` | Process memory sampler and regression test |
| `vessels/` | Vessel provisioning, acceptance tests, soak tests, and benchmarks |
| `world/` | World and zone population utilities |

General one-off entry points remain directly under `scripts/`: clean builds, backups,
artifact provisioning, and single-subsystem tests.

The server exposes readiness on the loopback Terrain API listener. Run
`scripts/operations/healthcheck.sh` for a single check or add `--wait` during
service startup. Override the default `http://127.0.0.1:8182/health` URL with
`LUMINARI_HEALTH_URL` when the listener uses a non-default port.

## Compatibility paths

The project-root `autorun.sh` and `autorun-watchdog.sh` names are compatibility
symlinks. Existing systemd units and operator commands can keep using them during a
deployment transition. New configuration and documentation use the canonical files
under `scripts/autorun/`.

The `scripts/deploy.sh`, `scripts/setup.sh`, and `scripts/move_bin.sh` names are also
compatibility symlinks. Their canonical implementations live under
`scripts/deployment/`.

Autotools owns `configure`, `compile`, `config.status`, `depcomp`, `install-sh`, and
`missing` in the project root. They must stay there because generated build files refer
to those paths. An ignored `configure~` backup may also exist in a configured working
tree; it is not a project utility and can be removed separately if it is no longer
needed.
