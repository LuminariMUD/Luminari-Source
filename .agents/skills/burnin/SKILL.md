---
name: burnin
description: >-
  Fully qualify a LuminariMUD development checkout with clean builds, production-linked
  regressions, isolated MariaDB tests, memory checks, and a staff login smoke test.
  Repair findings and leave the healthy MUD running. Use for an explicit full burn-in
  or stability pass, not routine focused validation.
---

# Burn in LuminariMUD

Complete a clean build, regression, database, and live-runtime pass after the final repair.
Leave this checkout's healthy development MUD running unless the user asks otherwise.
Updating this skill does not authorize executing it or resuming a paused burn-in.

## Establish the checkout and scope

- Read `AGENTS.md`, `README.md`, `Makefile.am`, the configured root `Makefile`, and
  `docs/guides/TESTING_GUIDE.md`. For process and log handling, use
  `docs/guides/TROUBLESHOOTING_AND_MAINTENANCE.md` and
  `docs/runbooks/incident-response.md`. Recheck actual targets and helpers when the tree changes.
- Inspect Git status and preserve unrelated work. Read `APP_ENV` from `lib/.env` without dumping
  the file. This workflow requires `APP_ENV=development`. The marker alone does not prove that
  the database or listeners belong to development; verify their actual targets too.
- Follow the repository prohibition on production code changes. Production access in `lib/.env`
  does not authorize stopping production, migrating its database, or publishing local repairs.
  Do not change an environment marker to pass a development guard.
- Preserve `src/campaign.h`, `src/mud_options.h`, `src/vnums.h`, `lib/mysql_config`, and `lib/.env`.
  Use their tracked examples only as permitted by `AGENTS.md`. Keep player files, world files,
  runtime databases, existing releases, and crash evidence intact.
- Keep logs, test runtimes, and separate build trees in a unique ignored `.burnin-runtime*`
  directory or an identified temporary directory. Record commands, statuses, and process handles.
  Never include real passwords, connection secrets, or private player records in the report.
- A burn-in request authorizes the local stop/build/test/start cycle and narrow repairs within
  its scope. Reuse existing authorization; ask only for a real unresolved target, destructive
  change, content conflict, or action beyond that scope. Do not commit, merge, or publish unless
  requested. On pause, stop this run's test clients and monitors, close its staff login cleanly,
  and record unfinished gates. Preserve evidence and the user's requested MUD state; do not
  resume automatically.

## Stop only the instance under test

Use `./scripts/autorun/autorun.sh status` and inspect the actual listener, process executable,
working directory, supervisor, and `.autorun.state`. A stale state file alone is not proof of a
running server. The local game port defaults to 4101 and the health listener to loopback 8182;
check `MUD_PORT`, `lib/etc/config`, and `TERRAIN_API_PORT` for overrides.

For an autorun-owned development instance:

```bash
./scripts/autorun/autorun.sh stop
```

Wait for the verified supervisor, watchdog if present, server child, and its listeners to stop.
If a direct debugging process or another supervisor owns this checkout, identify that exact
process and stop it gracefully through its existing control path. Never stop another checkout
or use broad process-name kills. A conflicting listener requires resolving ownership.
Use autorun for the subsequent development start, not `luminari.service`.

## Handle Luminari's database contract

The server gets its database connection from `lib/mysql_config`; `DB_*` entries in `lib/.env`
are not a substitute for tracing `src/mysql.c`. Inspect the configured runtime read-only before
repairing it, keeping credentials out of command arguments and output.

Luminari initializes missing tables and runs embedded, versioned migrations during startup:

- `src/db_startup_init.c`: `startup_database_init()` and `initialize_missing_tables()`;
- `src/db_init.c`: `run_database_migrations()`, `run_pet_persistence_migrations()`, and
  `schema_migrations` bookkeeping;
- `src/db_init_data.c`: required player-table and pet-persistence verification.

Trace these current requirements and any affected subsystem's checked-in SQL. Recorded migration
versions are evidence of application, not proof that the current columns, indexes, and engines
still satisfy the code. Read the relevant `sql/components/verify_*.sql` before executing it and
inspect returned checks, not just SQL exit status.

Normal local startup includes database initialization and idempotent migration writes; even a
`-c` syntax boot is not read-only. Qualify pending changes on an isolated fixture or disposable
copy before using the configured development runtime. Apply only justified, authorized repairs
with appropriate backups and verification. Already authorized local startup does not need a
duplicate confirmation merely because it performs its normal initialization.

`sql/master_schema.sql` bootstraps isolated/fresh databases. Do not import it over the runtime
database or require literal equality with every optional or retired schema in that file.
There is no separate migration-runner or universal runtime-compatibility command to invoke
before building. Runtime compatibility must be proven by the current source contracts and boot.

Keep help repairs aligned in both the database and `lib/text/help/help.hlp`. Trace failures in
help-content verifiers to the current command behavior and authored content before replacing
entries or keyword ownership. Review deletions, renames, and genuine conflicts explicitly.
Use the help-sync skill for an explicitly requested cross-environment synchronization; an
ordinary burn-in does not authorize publishing help to production.

## Build, test, and repair

Read [references/validation.md](references/validation.md) for the exact fixture setup and
validation commands. A full pass covers:

1. The pinned pre-commit formatting hook and a fresh Autotools server/utility build.
2. Root `make test-all`, with isolated database persistence and help integration enabled,
   followed by the supported alternate I/O driver check.
3. A separate CMake build, ASan/UBSan production tests, bounded protocol fuzzing, and Valgrind.
4. Installation of the normal Autotools release and proof that no root `luminari` artifact remains.

Inspect complete logs even on exit zero. Negative regression fixtures intentionally produce
some `SYSERR` and warning messages; trace them to the test that expects them. Repair unexpected
compiler warnings, test failures, memory errors, crashes, hangs, and credible defects. Keep fixes
narrow, add a regression for changed behavior, and follow the source style and build-manifest
rules in `AGENTS.md`.

After a repair, run its focused regression, then repeat the clean build and full validation pass
against the final source. Recheck runtime compatibility when the repair changes that contract.
Do not count earlier green results for a different source or executable as the final pass.
Run independent fixtures concurrently only when they cannot race shared build artifacts or data.

Identify every skip and missing prerequisite. Restore legitimate fixtures or tool dependencies
where possible; do not weaken tests, fabricate historical conversion plans, or point tests at
the game database to obtain a pass. If a required gate remains unavailable, finish the safe
available work and report the burn-in as incomplete with its exact coverage gap.

## Live burn-in and handoff

1. Record byte/inode or timestamp boundaries for root `syslog`, regular files under `log/`, and
   existing crash evidence under `dumps/`. Follow rotation and newly created files throughout
   startup, login, and the soak. Use the actual configured log locations if overridden.
   Autorun copies and renames `syslog` during rotation; a new file can contain old messages.
   Check the timestamps inside those messages against the observation boundary before treating
   them as new diagnostics.
2. Start the installed normal release through autorun, using the verified development port:

   ```bash
   ./scripts/autorun/autorun.sh
   ./scripts/operations/healthcheck.sh --wait
   ./scripts/autorun/autorun.sh status
   ```

   Set `MUD_PORT` and the matching `LUMINARI_HEALTH_URL` when the verified configuration requires
   overrides. Prove the process belongs to this checkout and runs the tested immutable release
   under `bin/releases/<ELF-build-ID>/`, including the expected Git/build identity. Readiness
   must report both the MUD and required MariaDB as healthy. Verify that autorun survives the
   launching terminal/tool session. Local Ollama, I3, and Discord connectivity is not required
   unless those integrations are the task's subject.
3. Inspect `scripts/development/dev_kohdee_login_smoke.sh` for the current account-menu,
   command-capture, and complete character/account logout sequence.
   It reads `GAME_MASTER_ACCOUNT` and `GAME_MASTER_ACCOUNT_PASSWORD`, supports `DEV_MUD_ACCOUNT`,
   `DEV_MUD_ACCOUNT_PASSWORD`, and `DEV_MUD_CHARACTER`, and currently defaults to character
   `Kohdee`. Confirm that the selected existing staff character is idle before selecting it;
   do not take over an active session, create a character, or elevate one to pass the test.

   Use a credential-safe Telnet/Expect session against the verified autorun listener, with
   prompt/response boundaries and secrets excluded from transcripts. The helper's `--commands`
   mode sends in-game `say` completion markers; use it only when that messaging is explicitly
   authorized. It reads the port from `lib/etc/config` and can start a separate user service
   when no listener exists. Invoke it only after autorun is healthy and the ports match;
   its fallback is not the burn-in startup path.
4. Randomize a selection of safe inspection commands across player and staff systems. Trace
   registrations and handlers in `src/interpreter.c` and the owning subsystem first. Candidates
   include `score`, `inventory`, `equipment`, `time`, `weather`, `who`, `activity`, `show stats`,
   `eventdebug`, and read-only `perfmon` output. Record exact arguments and verify sensible
   responses; helper success alone does not prove command correctness. Normal
   login/logout persistence is expected. Avoid combat, travel, spawning, administration, or
   changing characters, world state, and configuration during this inspection smoke test.
5. If repairs affect scheduling, persistence cadence, copyover, or the game loop, follow
   `docs/testing/EVENT_DRIVEN_CORE_ACCEPTANCE.md` for the additional runtime acceptance, including
   a real descriptor-survival copyover where required. This is a separate targeted gate, not
   permission to invoke the helper's mutating vessel scenarios in an ordinary smoke test.
6. Monitor through at least 60 seconds after clean logout, longer when a changed timer needs it.
   Recheck readiness, listener ownership, release identity, and supervisor stability. Investigate
   new crashes, persistence errors, restarts, and unexpected output. If a fix is needed, stop
   this instance, repair it, repeat validation, and repeat the live pass.

Leave the healthy MUD under autorun for the user. Stop only disposable database services and
other temporary processes created for testing; preserve useful logs and identified backups.
If validation cannot finish, state whether a healthy development MUD was restored and exactly
what remains. Honour a user's pause or explicit request to keep it stopped.

Report fixes, exact validation commands/results and any skips, evidence locations and observation
interval, staff character and smoke commands without credentials, and the final port, process,
release, and health state. A build, health response, or partial suite alone is not a full burn-in.
