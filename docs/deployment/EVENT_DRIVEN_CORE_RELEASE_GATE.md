# Event-Driven Core Release Gate

This runbook retains the production evidence, backup and restore requirements
for a proposed retirement of archival PubSub schema/data. No database deletion
has been authorized by the native-event migration.

The legacy queue, heartbeat, rollback branches and old save writer were
physically removed with maintainer authorization; see the
[full-world acceptance report](../testing/EVENT_CORE_FULL_WORLD_ACCEPTANCE_2026_09_05.md).
Both libevent and select remain supported I/O drivers for the same native
scheduler. Old event-save readers remain migration inputs. Those intentional
compatibility surfaces are outside the archival SQL proposal below.
Remaining SQL retention work is tracked in
[issue #112](https://github.com/LuminariMUD/Luminari-Source/issues/112).

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

The observed release must run the native scheduler. Record the selected
supported I/O driver (libevent or select), the
effective environment and startup log without recording secrets. The startup
evidence must include:

```text
Event runtime initialized: scheduler.
Runtime services: scheduled by named cadence.
I/O driver initialized: libevent (...).
Active-world mobile scheduling: demand driven (...).
```

Owner subsystems must report scheduled or managed modes. Native initialization
failure aborts startup; there is no supported legacy-heartbeat selector or
fallback. A release replacement during the observation period requires a new
source/deployment record and maintainer acceptance of the replacement evidence.
Do not assume an older executable can read newly written saves.

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
logs for the whole period. Record deployment interruptions, recovery actions and
any need to replace the release. Deadline-lateness and long-term RSS acceptance
remain separate from functional success; see
[issue #111](https://github.com/LuminariMUD/Luminari-Source/issues/111).

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
- an explicit statement that no active reader or recovery procedure needs the
  proposed archival SQL objects;
- the complete PubSub object and row inventory;
- the approved retention decision;
- a foreign-key-safe object removal order;
- the tested restore procedure and rehearsal evidence;
- explicit maintainer and production-database approval.

Only after the concrete SQL object list, retention decision and restore evidence
are approved may that database deletion proceed. Rehearse the exact proposed
slice in isolation and verify boot, gameplay, persistence and copyover against
the resulting schema. Runtime scheduler removal is already complete and is not
waiting on this data-retention decision.

## 7. Sign-off

| Decision | Reviewer | UTC timestamp | Evidence link |
|----------|----------|---------------|---------------|
| Stable release completed |  |  |  |
| No archival SQL reader/recovery dependency |  |  |  |
| PubSub backup verified |  |  |  |
| PubSub restore rehearsed |  |  |  |
| Retention decision approved |  |  |  |
| Exact SQL object deletion approved |  |  |  |

Any blank row leaves the irreversible gate closed.

## 8. Assigned-batch retention review (2026-09-06)

Disposition for this repair branch: retain archival PubSub SQL and all old
save readers. No retirement migration is included. This is the reversible
retention choice for #112; it is not approval to erase production history.

A read-only inventory of the local development schema `luminari` at
2026-09-06T16:10:40Z found these ten InnoDB tables, each with an exact count of
zero rows:

- `pubsub_topics`, `pubsub_player_settings`, `pubsub_subscriptions`
- `pubsub_messages`, `pubsub_message_metadata`, `pubsub_message_fields`
- `pubsub_messages_v3`, `pubsub_message_metadata_v3`, `pubsub_message_fields_v3`
- `pubsub_message_tags_v3`

No matching views, routines, triggers or scheduled database events were found.
Nine foreign-key relationships remain among these tables; the message tables
also reference themselves through `parent_message_id`. No foreign-key reference
from another table family appeared in this local inventory.

Source review found no PubSub runtime calls in `src`. The master schema and
archival V3 component still define the historical objects. The character-rename
schema test deliberately inserts archival rows and verifies they are unchanged;
`scripts/events/test_pubsub_retirement.sh` deliberately verifies schema
retention. Those test dependencies must be revised only as part of an approved
retirement slice. They are not live gameplay readers. Legacy event-save parsing
is a separate migration input and remains necessary.

The local result says nothing about production row counts or historical
installations. Production inventory, stable-release sign-off, approved archive
storage/retention, a verified full backup and restore rehearsal remain required
before proposing a drop. No production export, restore rehearsal or deletion
was performed by this review. The sign-off table above remains open.
