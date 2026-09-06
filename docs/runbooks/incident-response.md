# LuminariMUD Incident Response Runbook

## Scope and Escalation

This runbook contains repository-backed diagnosis and containment steps. The
repository does not define an on-call roster, response-time SLA, maintenance
window, or private security-reporting contact. Operators must use the approved
organizational escalation path for the affected host and record those missing
ownership details outside this codebase.

Preserve evidence before rebuilding, replacing binaries, mutating world data,
or changing database state.

## First Response

1. Record detection time, symptoms, affected players/systems, current commit,
   and the active executable identity.
2. Check supervisor, process, liveness, and readiness separately.
3. Preserve logs, `.autorun.state`, crash archives, relevant world files, and a
   database backup before corrective writes.
4. Prefer the normal systemd or autorun control path. Avoid broad process kills,
   file deletion, or unrehearsed database rollback.

```bash
./scripts/autorun/autorun.sh status
sudo systemctl status luminari.service --no-pager
sudo journalctl -u luminari.service -n 200 --no-pager
curl -fsS http://127.0.0.1:8182/health/live
./scripts/operations/healthcheck.sh
```

Interpret the probes independently:

- Liveness failure means the initialized game loop is not serving the local
  listener.
- Liveness success with readiness failure means the loop is active but the
  required MariaDB connection is unhealthy.
- Both probes succeeding does not prove every gameplay subsystem is healthy;
  continue with symptom-specific checks.

## Server Crash or Failed Startup

### Evidence

Autorun writes current output to root `syslog`, rotates prior logs under
`log/`, and stores crash evidence under `dumps/`. Preserve the matching
`bin/releases/<ELF-build-ID>/` directory before installing another binary.

```bash
./scripts/autorun/autorun.sh status
find dumps -maxdepth 2 -type f -print
tail -200 syslog
```

### Recovery

Autorun normally restarts a crashed game. If the managed service did not
recover and an operator has confirmed there is no competing supervisor:

```bash
sudo systemctl restart luminari.service
./scripts/autorun/autorun.sh status
./scripts/operations/healthcheck.sh --wait
```

Analyze a core only with the executable recorded for the crashed process and
the `.debug` sidecar beside it that carries the same basename. Every release
holds `luminari` and `luminari.debug`. During a controlled maintenance window,
the maintained capture self-test is:

```bash
./scripts/debugging/verify_core_capture.sh --self-test
```

Exit 0 verifies capture; exit 2 means the host remains unverified. Follow the
[crash recovery guide](../guides/TROUBLESHOOTING_AND_MAINTENANCE.md#logs-and-crash-evidence)
for detailed analysis.

## Database Readiness Failure

### Diagnosis

```bash
curl -fsS http://127.0.0.1:8182/health/live
./scripts/operations/healthcheck.sh
sudo systemctl status mariadb --no-pager
sudo journalctl -u mariadb -n 200 --no-pager
```

If liveness succeeds and readiness returns 503, focus on MariaDB availability,
the local `lib/mysql_config`, permissions, capacity, and database logs. Do not
paste credentials into tickets, chat, or command output.

### Recovery

After the database fault is corrected, require readiness before reopening or
declaring recovery:

```bash
./scripts/operations/healthcheck.sh --wait
```

Restart the MUD only when the application did not reconnect or the approved
maintenance plan requires it.

## Port Conflict

Check both the game port (default 4100) and loopback health port (default 8182):

```bash
sudo lsof -i :4100
sudo lsof -i :8182
./scripts/autorun/autorun.sh status
```

Resolve the owner before stopping anything. Use
`sudo systemctl stop luminari.service` for the managed service or
`./scripts/autorun/autorun.sh stop` for an unmanaged supervisor. A direct local
server can use another final positional game port; the health listener uses
`TERRAIN_API_PORT` and requires a matching `LUMINARI_HEALTH_URL` for probes.

## High Memory or CPU

Record the process identity and sample the live process before restart:

```bash
./scripts/autorun/autorun.sh status
ps -o pid,ppid,etime,%cpu,%mem,rss,vsz,cmd -C luminari
./scripts/process-memory/sample_process_memory_details.sh --header
```

Use a controlled maintenance restart for containment. Reproduce suspected
memory defects in development with the maintained sanitizer or Valgrind gates;
do not attach an intrusive debugger to production without approval.
The sampler accepts `--sample <pid> <label>` after the process identity has
been verified.

## World or Content Boot Failure

Preserve the failing file and log output. Validate a copied or development
world tree with the read-only world tool before changing production content:

```bash
python scripts/world/wtool.py --help
```

Do not rename, delete, or overwrite a production world file merely to bypass a
parser failure. Restore only from an identified backup and validate references
before restart.

## Vessel Incident

For vessel command failure, persistence mismatch, room pressure, or tick
latency, preserve syslog, application identity, `shiplist` output, and a
database backup. Confirm the checkout marker without printing credentials:

```bash
rg '^APP_ENV=' lib/.env
```

Use the cedit vessel option for containment when the behavior is understood;
do not purge ships, reuse fleet slots, edit ownership rows, or apply a schema
rollback to hide symptoms. Recovery and validation are defined in the
[vessel testing guide](../testing/VESSEL_SYSTEM_TESTING.md),
[vessel schema deployment guide](../deployment/VESSEL_SCHEMA_DEPLOYMENT.md),
and [vessel requirements](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md).

## Post-Incident Record

Record:

- detection, containment, recovery, and verification times;
- symptoms and confirmed scope;
- active commit and immutable executable identity;
- preserved logs, crash artifacts, world files, and database backup identity;
- commands and changes performed;
- confirmed cause versus unresolved hypotheses;
- follow-up owner, test, documentation, and rollout action.

Create a tracked issue for confirmed code or operational defects without
including credentials, player data, raw production database content, or other
sensitive evidence.

## References

- [Deployment guide](../deployment/DEPLOYMENT_GUIDE.md)
- [Health API contract](../api/README_api.md)
- [Environment boundaries](../deployment/environments.md)
- [Troubleshooting and maintenance](../guides/TROUBLESHOOTING_AND_MAINTENANCE.md)

Last updated: 2026-08-07
