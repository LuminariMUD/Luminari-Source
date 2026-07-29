# Vessel Schema Deployment

**Last updated:** July 29, 2026

This runbook covers controlled vessel-schema installation, verification,
rollback rehearsal, and staged application rollout. The server can create and
migrate the current tables at boot, but an explicit database procedure is
required when operators need the schema in place before startup or need
auditable rollback evidence.

The vessel system stores player property. Treat migration and rollback as
high-impact operations even though the schema scripts are designed to be
repeatable.

## Source of Truth

- SQL components:
  [`sql/components/`](../../sql/components/)
- Current schema and behavior:
  [VESSEL_SYSTEM.md](../systems/VESSEL_SYSTEM.md#database-schema)
- Release requirements:
  [PRD.md](../PRD.md#release-acceptance)
- Remaining preflight work:
  [VESSELS_TODO.md](../project-management-zusuk/vessels/VESSELS_TODO.md)

## Available Components

Apply schema phases in ascending order. Later phases extend or depend on earlier
tables.

| Phase | Install | Verify | Rollback | Purpose |
|---|---|---|---|---|
| 2 | `vessels_phase2_schema.sql` | `verify_vessels_schema.sql` | `vessels_phase2_rollback.sql` | Interiors, docking, room templates, cargo, and crew |
| 4 | `vessels_phase4_schema.sql` | `verify_vessels_phase4.sql` | `vessels_phase4_rollback.sql` | Builder ship prototypes |
| 6 | `vessels_phase6_schema.sql` | `verify_vessels_phase6.sql` | `vessels_phase6_rollback.sql` | Ownership, upgrades, insurance, wages, permits, and hired crew |
| 7 | `vessels_phase7_schema.sql` | `verify_vessels_phase7.sql` | `vessels_phase7_rollback.sql` | Commodities, port supply, freight, bulk cargo, and bounties |
| 8 | `vessels_phase8_schema.sql` | `verify_vessels_phase8.sql` | `vessels_phase8_rollback.sql` | Region-keyed vessel encounters |
| 9 | `vessels_phase9_schema.sql` | `verify_vessels_phase9.sql` | `vessels_phase9_rollback.sql` | Live hull, condition, room, weapon-slot, autopilot, and schedule snapshots |
| Help | `help_vessel_entries.sql` | `verify_help_vessel_entries.sql` plus in-game sweep | Restore backup | 31 authoritative vessel and vehicle help entries covering 74 command keywords |

`test_vessels_integrity.sql` inserts and removes fixed test identifiers. Run it
only on an isolated rehearsal database where ship id 99999 is known to be free,
not against a live production database.

The planned `ship_weapons` persistence component does not exist yet and is not
part of this procedure.

## Pre-Deployment Gate

Before changing a shared or production-like database:

- [ ] Confirm the checkout and database are the intended environment. This
      repository's `lib/.env` is local configuration and must not be modified.
- [ ] Rehearse the exact install and rollback on a recent production snapshot.
- [ ] Schedule the maintenance window and name the operator with rollback
      authority.
- [ ] Record the application commit, schema inventory, row counts, and current
      vessel ownership/cargo census.
- [ ] Create a consistent database backup that includes routines and triggers,
      then prove it can be read and restored into an isolated database.
- [ ] Confirm every install, verify, and rollback file comes from the same
      reviewed source revision.
- [ ] Stop vessel writes before migration. Set the cedit vessel option to
      `Off`, confirm a gated command reports that the system is disabled, and
      confirm `shiplist` remains available for recovery. The flag gates vessel
      command dispatch and both heartbeat tick groups.
- [ ] Keep the previous application binary and configuration available.
- [ ] Confirm `vdebug status` reports `compiled out` in the candidate build.
      Debug support is available only in an explicit development build compiled
      with `-DVESSEL_SYSTEM_DEBUG=1`.

Do not expose credentials in shell history or command output. Use the approved
MySQL client configuration for the target environment.

## Rehearsal Procedure

Use an isolated clone of a recent production backup.

1. Restore the snapshot into the rehearsal database.
2. Start the matching pre-migration application and capture a baseline:
   - Owned ships and owner names.
   - Interior, cargo, crew, route, schedule, and encounter row counts.
   - Representative ship records selected for post-migration comparison.
3. Stop application writes.
4. Apply each schema component in ascending order.
5. Run every matching verification script.
6. Apply `help_vessel_entries.sql`, run
   `verify_help_vessel_entries.sql`, and complete the in-game command-keyword
   sweep.
7. Start the candidate application and run the manual vessel regression.
8. Exercise reboot and copyover with ships under way, in combat, and carrying
   cargo.
9. Stop writes, execute the rollback plan, restore the previous application,
   and compare the recovered data with the baseline.
10. Record commands, durations, results, and any manual intervention in the
    deployment record.

The rehearsal passes only when both forward migration and rollback preserve all
property that the release contract promises to preserve.

## Install Procedure

The examples below deliberately omit credentials. Set task-specific values for
the approved target:

```bash
vessel_db="approved_database_name"
vessel_client_config="/secure/path/to/mysql-client.cnf"
```

Create and validate a pre-change backup:

```bash
mysqldump --defaults-extra-file="$vessel_client_config" \
  --single-transaction --routines --triggers "$vessel_db" \
  > /approved/backup/location/vessels-before.sql

test -s /approved/backup/location/vessels-before.sql
```

With application writes stopped, apply the reviewed components:

```bash
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase2_schema.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase4_schema.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase6_schema.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase7_schema.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase8_schema.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase9_schema.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/help_vessel_entries.sql
```

Stop immediately on any nonzero exit status. Do not continue into a later phase
to see whether it repairs an earlier failure.

## Verification

Run every phase-specific verifier:

```bash
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/verify_vessels_schema.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/verify_vessels_phase4.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/verify_vessels_phase6.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/verify_vessels_phase7.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/verify_vessels_phase8.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/verify_vessels_phase9.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/verify_help_vessel_entries.sql
```

Also verify:

- Expected columns, indexes, procedures, and seeded commodities.
- No out-of-band port supply or invalid encounter-region references.
- Pre-existing ship, owner, cargo, crew, route, and schedule counts.
- Representative records against the pre-deployment snapshot.
- All 74 vessel and vehicle command-keyword searches in the running game,
  requiring database `Help Tag` results rather than file fallback.
- Database errors and slow queries during the manual regression.

Do not use a single `ship_%` table count as the release verdict. Later phases
also create `trade_commodities`, `port_commodities`, `freight_contracts`,
`vessel_bounties`, and `vessel_encounters`.

## Application Validation and Staged Rollout

After schema verification:

1. Start the candidate server on development or the designated staff stage.
2. Complete
   [VESSEL_SYSTEM_TESTING.md](../testing/VESSEL_SYSTEM_TESTING.md).
3. Verify reboot and copyover recovery.
4. Confirm the complete 500-ship benchmark and 72-hour soak evidence.
5. Observe logs, game-loop latency, room-pool pressure, database errors,
   schedules, cargo, ownership, and encounter volume.
6. Expand access from staff to the beta cohort, then to all players only after
   each stage remains healthy.

At every stage, retain the tested cedit path to stop vessel commands and ticks.
The intentionally ungated `shiplist`, `shipgoto`, `shipfix`, `shippurge`,
`vehiclepurge`, and debug-status commands remain available for diagnosis and
recovery. Also retain the previous application and database state.

## Rollback

Prefer restoring the complete pre-deployment backup with the previous
application version. This is the only rollback that also reliably restores
help rows changed by the idempotent help migration.

Before rollback:

1. Stop application writes.
2. Capture an incident backup for diagnosis.
3. Record all vessel changes made after deployment so the recovery decision is
   explicit.
4. Confirm the selected backup and previous binary belong to the same release.

The phase rollback scripts are destructive. If they are used instead of a full
restore, run them in reverse dependency order:

```bash
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase9_rollback.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase8_rollback.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase7_rollback.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase6_rollback.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase4_rollback.sql
mysql --defaults-extra-file="$vessel_client_config" "$vessel_db" \
  < sql/components/vessels_phase2_rollback.sql
```

These scripts delete encounters, economy data, ownership state, prototypes,
interiors, cargo, and crew. Never run them merely to retry an install. After
rollback or restore, run the previous version's verification, compare the
baseline census, start the previous application, and exercise representative
ships before reopening access.

## Deployment Record

Keep the following with the release record:

- Application revisions before and after.
- Database server/version and schema inventory.
- Backup location, checksum, restore-test result, and retention owner.
- Install and verification output for every component.
- Baseline and post-change vessel-property census.
- Manual regression, copyover, benchmark, and soak evidence.
- Stage start/end times, observed metrics, incidents, and decisions.
- Rollback outcome or the explicit decision not to roll back.
