---
name: help-sync
description: >-
  Build or operate LuminariMUD's two-way help-content synchronization between development and
  production while keeping MySQL and lib/text/help/help.hlp aligned. Use for cross-environment help
  drift, HEDIT reconciliation, sealed sync plans, guarded help publication, or sync rollback; do not
  use for ordinary single-entry editing or worldfile synchronization.
---

# Luminari Help Sync

Before any help synchronization or sync-system implementation action, read the
[helpfile synchronization design][design] completely. Treat it and the repository's `AGENTS.md` as
authoritative. Trace the current source, schema, scripts, and deployment configuration instead of
relying on stale line numbers or assumed commands.

[design]: ../../../docs/ongoing-projects/HELPFILE_SYNCHRONIZATION_DESIGN.md

## Select the operation

Classify the request as one or more of these modes:

- `implement`: build or change the deterministic synchronization engine and its tests;
- `audit`: compare the database and text projection in each environment without writing;
- `plan`: reconcile the last common baseline, development, and production into a sealed plan;
- `apply-dev`: apply and prove an exact sealed candidate on development;
- `apply-prod`: publish that already-proven candidate to production after a fresh drift check;
- `verify`: prove database, file projection, and candidate hashes agree; or
- `rollback`: restore a named run from its validated backups and verify the restored state.

Default an ambiguous operational request to read-only `audit` or `plan`. Do not infer permission to
apply, delete, roll back, restart, or mutate production from skill invocation alone.

## Establish the capability and environment

1. Locate the repository root and inspect the branch and worktree status. Preserve unrelated edits.
2. Read only `APP_ENV` from `lib/.env` without printing the file or any credential values. Local
   implementation and testing require `APP_ENV=development`.
3. Inspect `scripts/help-sync/help_sync.py`, its tests, and its command help before treating any
   operation as implemented. Do not assume a command from the design document exists.
4. For production access, discover connection variable names from the protected configuration, but
   never echo, log, copy, commit, or place secret values in arguments, plans, manifests, or reports.
5. Confirm that remote production identifies itself as production. Refuse a target whose identity or
   help-file path is ambiguous.

If the deterministic engine or the requested safety gate is absent:

- for an `implement` request, continue through the relevant design phases with tests and temporary
  development fixtures;
- for an operational request, stop before any data write and report the missing capability;
- never substitute improvised SQL, `scp`, `rsync`, direct file replacement, or interactive `helpgen`
  commands for the missing engine.

## Use the implemented command contract

Run `python3 scripts/help-sync/help_sync.py --help` and the selected subcommand's `--help` before an
operational action. The public workflow is:

```text
audit [--json]
baseline-init --source <development|production|file> --authorize-hash <hash> [...]
plan [--tombstones <file>] [--renames <file>] [--repair-integrity] [--repair-layers] [--save]
resolve <plan> --resolutions <file> [--save]
show <plan>
apply-dev <plan>
preview-prod <plan>
apply-prod <plan> --authorize-plan <plan-id> --authorize-preview <token>
verify <plan> --environment <development|production|both>
rollback <run-id> --environment <development|production> \
  --expected-current-hash <hash> [--authorize-run <run-id>]
```

The first common baseline is an explicit operator choice and a state-artifact write on both
endpoints; do not infer it from an audit. Production rollback additionally requires
`--authorize-run` to exactly equal the named run ID. Operational state is ignored below
`lib/text/help/.help-sync/`; do not relocate it into tracked files.

## Preserve the content model

Treat each environment as one logical editable catalog with two required representations:

- MySQL is the active authoring and lookup catalog;
- `lib/text/help/help.hlp` is a deterministic, tracked fallback projection.

The cross-environment representation must preserve every semantic content field and exclude local
IDs, bookkeeping timestamps, search analytics, and transplanted version-history rows. Do not use
`help.hlp` as the lossless transfer format. Do not treat its modification time, database timestamps,
or an environment preference as conflict resolution.

Reconcile current development and production against the last catalog successfully applied to both.
Allow automatic field-level merges only where the design explicitly permits them. Require
explicit resolution for same-field divergence, edit-versus-delete, removal-versus-change, and
renames. Missing data is not a deletion; only a sealed tombstone can delete content.

## Run read-only audit and planning

Audit and default planning must perform no database, help-file, cache, service, lock, or Git
mutation. Use the repository engine through its `audit` and `plan` interfaces. `plan --save` and
`resolve --save` are explicit writes only to ignored plan state; do not describe them as fully
read-only.

Require the resulting evidence to identify:

- the baseline, development, and production catalog hashes;
- database/file layer drift in each environment;
- additions, updates, explicit deletions, renames, and conflicts;
- excluded operational tables and fields;
- deterministic candidate and plan hashes; and
- every unresolved decision that prevents sealing or apply.

Inspect the machine-readable plan as well as its human summary. A plan is applyable only if it is
content-addressed, conflict-free, names its expected source hashes, and contains no implicit
deletion.

Unexplained database/file drift blocks sealing. `--repair-layers` is an explicit decision to
regenerate the drifted file from the reviewed database. If the file contains legitimate file-only
development work, stop and normalize that work into the development database before planning;
never use layer repair to overwrite it merely to clear the audit.

## Apply on development

Use only the exact sealed plan produced by the engine. Before writing, confirm the development
source hash still matches the plan and validate targeted database and file backups. Apply
content-table changes transactionally, generate `help.hlp` through a validated temporary file and
atomic rename, then use the supported cache invalidation path or the approved local `autorun.sh`
restart path.

Re-export and require semantic equality with the sealed candidate. Run the engine's integrity,
round-trip, idempotence, and representative lookup checks. A failed or partially verified
development apply is not eligible for production.

## Apply on production

Never modify production code. Production help-content mutation is allowed only when all of these are
true:

1. the exact candidate passed development apply and verification;
2. every conflict and required policy decision is resolved;
3. the user has been shown the sealed plan ID, hashes, row counts, deletions, and backup
   destination;
4. the user explicitly authorizes that exact production apply after seeing the preview;
5. the HEDIT write barrier is active;
6. a fresh production export still matches the plan's expected production hash; and
7. validated targeted database and `help.hlp` backups exist.

If any condition fails, release any acquired barrier safely and abort without applying a revised
candidate. Re-plan when production changed; never rebuild or silently reseal a candidate during
apply.

Run `preview-prod` immediately before apply and preserve its exact authorization token. Apply only
the sealed row delta with optimistic old-hash preconditions and one database
transaction. Install the generated file atomically, invalidate the runtime cache through the
supported mechanism, and verify database, file projection, runtime lookups, logs, and service
health. Record the new common baseline only after every verification passes, then release the HEDIT
barrier.

## Roll back and report

Rollback is a separate mutation requiring a named failed run, validated backups, and explicit
target authorization. Restore both layers, refresh the runtime, and prove the restored semantic
hash. Retain the failed run artifacts for diagnosis.

For every completed operation, report the mode, environments touched, plan ID and catalog hashes,
entry/relationship counts, conflicts and deletions, backup locations without secrets, verification
results, and whether the common baseline advanced. Never claim a sync succeeded from command exit
status alone.
