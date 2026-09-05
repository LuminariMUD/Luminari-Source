# Event-Driven Core Release Gate

This runbook gathers the external evidence required before removing the legacy
event queue, compatibility heartbeat, `select()` driver, rollback branches,
legacy persistence writer, or deprecated PubSub schema. It does not authorize
those removals.

## 1. Gate ownership

The maintainer must define the stable-release period before deployment and sign
the final record after it ends. Development CI, a local live server, or elapsed
time on an unmerged branch does not start the period.

Record these identities:

| Field | Required value |
|-------|----------------|
| Merge commit on `master` | Full SHA |
| Release tag | `v*.*.*` tag containing the merge |
| GitHub release | Release URL |
| Production deployment | Deployment identifier and timestamp |
| Build identity | Output of `luminari --build-info` |
| Stable-period start/end | UTC timestamps and maintainer-approved duration |
| Operators | Names of deployer and reviewers |

The containing tag, release, and deployment must all resolve to the same source
identity. Save the successful Build & Test, Security, and release workflow URLs
with the record.

## 2. Required production modes

The observed release must run the scheduler/libevent defaults. Record the
effective environment and startup log without recording secrets. The startup
evidence must include:

```text
Event backend initialized: scheduler.
Runtime services: scheduled by named cadence.
I/O driver initialized: libevent (...).
Active-world mobile scheduling: demand driven (...).
```

Owner subsystems must report scheduled or managed modes rather than
`legacy heartbeat`. A warning that restores the legacy heartbeat, an explicit legacy
selector, or an operator rollback ends the candidate period. Record the event,
reason, and corrective release; a new stable period starts only after the
maintainer accepts the replacement deployment.

## 3. Health evidence

Capture evidence at deployment, at the maintainer-approved sampling cadence,
after copyover, and at the end of the period:

```sh
scripts/operations/healthcheck.sh
```

In the MUD, capture these paginated immortal reports at the configured display
width:

```text
eventdebug
eventdebug queue 20
eventdebug types 30
eventdebug domain
eventdebug player <online-player> 10
eventdebug mob <visible-mobile> 10
eventdebug object <visible-object> 10
eventdebug room here 10
eventdebug scripts mob <visible-mobile> 10
perfmon entities
```

The acceptance record must explain every nonzero ready backlog, overdue
deadline, late callback, failed event, registry mismatch, stale-owner outcome,
or admission rejection. A transient value is not automatically a failure, but
an unexplained or growing value is not release evidence.

Also record successful checks for:

- normal login, character creation, movement, combat, and logout;
- autonomous off-screen movement, patrols, hunts, scripts, and NPC wars;
- local and spatial domain-event sights and sounds;
- ability recovery across logout and copyover;
- NPC class-slot and known-spell recovery;
- wilderness materialization and coordinate-owned trails;
- copyover with connected players and active timed work; and
- graceful shutdown, restart, and signal handling.

Keep timestamped server, event-debug, health, copyover, and process-resource
logs for the whole period. Record whether any fallback selector was requested,
whether any automatic fallback occurred, and whether operators believed a
rollback was necessary even if they did not perform one.

## 4. PubSub inventory and backup

PubSub runtime code is retired, but production data is not authorized for
deletion. Use a protected MariaDB client defaults file; never place credentials
on the command line or in the evidence bundle.

Before proposing a migration, inventory all matching tables, views, routines,
triggers, and events. Historical installations may contain objects absent from
the current schema files, so discovery must use `information_schema` rather
than a hard-coded list.

```sql
SELECT TABLE_NAME, TABLE_TYPE, ENGINE, TABLE_ROWS
FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME LIKE 'pubsub\_%' ESCAPE '\\'
ORDER BY TABLE_TYPE, TABLE_NAME;

SELECT TABLE_NAME
FROM information_schema.VIEWS
WHERE TABLE_SCHEMA = DATABASE()
  AND (TABLE_NAME LIKE '%pubsub%'
       OR VIEW_DEFINITION LIKE '%pubsub\_%')
ORDER BY TABLE_NAME;

SELECT ROUTINE_NAME, ROUTINE_TYPE
FROM information_schema.ROUTINES
WHERE ROUTINE_SCHEMA = DATABASE()
  AND (ROUTINE_NAME LIKE '%PubSub%'
       OR ROUTINE_NAME IN ('CreateMessageV3', 'AddMessageTag',
                           'MigrateLegacyToV3')
       OR ROUTINE_DEFINITION LIKE '%pubsub\_%')
ORDER BY ROUTINE_TYPE, ROUTINE_NAME;

SELECT TRIGGER_NAME, EVENT_OBJECT_TABLE
FROM information_schema.TRIGGERS
WHERE TRIGGER_SCHEMA = DATABASE()
  AND (TRIGGER_NAME LIKE '%pubsub%'
       OR EVENT_OBJECT_TABLE LIKE 'pubsub\_%' ESCAPE '\\'
       OR ACTION_STATEMENT LIKE '%pubsub\_%')
ORDER BY TRIGGER_NAME;

SELECT EVENT_NAME, STATUS
FROM information_schema.EVENTS
WHERE EVENT_SCHEMA = DATABASE()
  AND (EVENT_NAME LIKE '%pubsub%'
       OR EVENT_DEFINITION LIKE '%pubsub\_%')
ORDER BY EVENT_NAME;
```

Take a full consistent backup before any retirement migration so dependencies
outside the discovered name family cannot be missed:

```sh
backup_tmp=$(mktemp "${backup_path}.partial.XXXXXX") || exit 1
trap 'rm -f "$backup_tmp"' 0 HUP INT TERM
if ! mysqldump --defaults-extra-file="$db_client_config" \
  --single-transaction --routines --triggers --events --hex-blob \
  "$db_name" > "$backup_tmp"
then
  exit 1
fi
if ! mv "$backup_tmp" "$backup_path"
then
  exit 1
fi
trap - 0 HUP INT TERM
sha256sum "$backup_path" > "$backup_path.sha256"
sha256sum --check "$backup_path.sha256"
```

Store the encrypted backup in the approved location and record retention,
access control, size, checksum, source database identity, and UTC timestamp.

## 5. Restore rehearsal

Restore only into a disposable isolated database. Never rehearse against the
production schema.

1. Create an empty disposable database with the production character set and
   collation.
2. Import the protected full backup.
3. Re-run the object inventory and exact row counts for every discovered
   PubSub table on both source and restored databases.
4. Compare `SHOW CREATE` output for every discovered table, view, routine, and
   trigger. Compare normalized `SHOW CREATE EVENT` output for every discovered
   event and compare its `EVENT_DEFINITION`, `EVENT_TYPE`, `EXECUTE_AT`,
   `INTERVAL_VALUE`, `INTERVAL_FIELD`, `STARTS`, `ENDS`, `STATUS`,
   `ON_COMPLETION`, and `TIME_ZONE` fields from `information_schema.EVENTS`.
5. Verify each view, routine, trigger, and event `DEFINER` exists in the
   rehearsal environment or document an explicitly reviewed definer-rewrite
   policy. Do not silently strip security identities from a production dump.
6. Run foreign-key checks and representative read queries.
7. Drop only the disposable rehearsal database after evidence is retained.

The rehearsal record must include commands, tool versions, start/end times,
checksums, object and row-count comparisons, warnings, and reviewer sign-off.
A dump that was not successfully restored is not rollback evidence.

## 6. Retirement proposal

After the release period and restore rehearsal, submit a separately reviewed
proposal containing:

- the exact release and production evidence above;
- an explicit statement that no rollback occurred or remained necessary;
- the complete PubSub object and row inventory;
- the approved retention decision;
- a foreign-key-safe object removal order;
- the tested restore procedure and rehearsal evidence;
- durable-event file inventory or conversion evidence before removing legacy
  read support; and
- explicit maintainer and production-database approval.

Only after every item is approved may the post-release sequence in
`EVENT_DRIVEN_CORE_REFACTOR_PHASE11_REMOVAL_INVENTORY.md` begin. Each deletion
slice still requires the complete local, CI, database, copied-world, live-MUD,
copyover, sanitizer, Valgrind, and soak acceptance gates.

## 7. Sign-off

| Decision | Reviewer | UTC timestamp | Evidence link |
|----------|----------|---------------|---------------|
| Stable release completed |  |  |  |
| No rollback dependency |  |  |  |
| PubSub backup verified |  |  |  |
| PubSub restore rehearsed |  |  |  |
| Retention decision approved |  |  |  |
| Physical removal approved |  |  |  |

Any blank row leaves the irreversible gate closed.
