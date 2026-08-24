# Helpfile Two-Way Synchronization Design

Status: implementation complete; bounded end-to-end orchestration passes isolated two-endpoint
tests, and the guarded live production workflow was verified on 2026-08-24

Analysis date: 2026-08-24

This document records the implemented architecture for safely reconciling help
content between local development and remote production. The engine and runtime
safeguards are present in the development checkout. They have not been deployed
to production, no production content has been changed, and the first shared
baseline has not been initialized.

## 1. Problem statement

Help content currently has four observable storage surfaces:

1. the development database;
2. the development `lib/text/help/help.hlp` file;
3. the production database; and
4. the production `lib/text/help/help.hlp` file.

Changes legitimately originate in both environments:

- Production staff create and edit entries through HEDIT.
- Development work adds or changes help alongside features, bug fixes, and
  documentation updates.

A one-way copy in either direction would therefore lose valid work. A two-way
last-writer-wins copy would also be unsafe because concurrent edits to the same
entry require review rather than a timestamp-based winner.

The required system is a content-aware, three-way reconciler with explicit
conflict handling, transactional application, deterministic file generation,
and a durable record of the last common state.

## 2. Current implementation findings

### 2.1 Runtime lookup uses both layers

The server loads `help.hlp` into the legacy in-memory help table during boot in
`src/db.c:4593`. Normal keyword lookup queries the database first and falls back
to that file-backed table when the database returns no match in
`src/help.c:348-363`.

The two layers are consequently not interchangeable:

- The database is the primary runtime authoring and lookup layer.
- `help.hlp` remains a boot-loaded fallback and a tracked recovery artifact.

The synchronization design must keep both current.

### 2.2 HEDIT writes the database, not the text file

`hedit_save_internally()` calls `hedit_save_to_disk()`, but the latter delegates
only to `hedit_save_to_db()` in `src/olc/hedit.c:508-518`. The save path validates
the entry, opens a transaction, archives the prior version when possible,
upserts the entry, and updates keywords.

An HEDIT save can therefore make the production database newer than the
production text file. The function name must not be treated as proof that the
file was regenerated.

### 2.3 Built-in import and export are useful but not lossless sync

The implementor-only `helpgen` command is registered in
`src/interpreter.c:2102`. It supports:

- `helpgen import preview|skip|merge|force`; and
- `helpgen export preview|backup|force` with optional filters.

These commands are useful migration and recovery primitives, but they are not a
two-way synchronization protocol:

- Export selects `tag`, `entry`, and `min_level`, then emits sorted keywords.
- The file format does not preserve all database metadata, including
  `max_level`, `category`, `auto_generated`, or related-topic relationships.
- Import `merge` creates suffixed duplicate tags; that is not the desired
  resolution for two people editing the same logical entry.
- Import modes compare the file with one database, not development and
  production against a last common baseline.

`help.hlp` should therefore be a deterministic projection and review artifact,
not the sole cross-environment transfer representation.

### 2.4 Content and operational tables have different ownership

The content model currently spans at least:

- `help_entries`;
- `help_keywords`; and
- `help_related_topics`, when populated.

The following data is environment-local and must not be copied as content:

- auto-increment database IDs;
- `help_search_history` analytics;
- timestamps used only as database bookkeeping; and
- existing `help_versions` history rows.

A synchronization apply should add its own audit/version record where
appropriate. It should not transplant one environment's history or analytics
into the other.

### 2.5 Development audit at implementation handoff

The final read-only development snapshot on 2026-08-24 found 2,151 logical
entries, no related-topic rows, and a write-ready 2026082408 schema. It also
found 83 pre-existing integrity issues: 80 orphan keyword rows and three entries
without keyword rows. The current `help.hlp` parses successfully but differs
from the deterministic database projection. These observations are inputs to a
future reviewed plan; this implementation did not repair them or alter help
content.

No production snapshot or mutation was performed during implementation.

## 3. Authority model

The four storage surfaces should not be treated as four equal peers. They are
two editable logical catalogs, each with two representations:

```text
Production HEDIT -> production DB ----+
                                       +-> three-way merge -> candidate catalog
Development work -> development DB ---+
          |                                      |
          +-> generated development help.hlp     +-> apply to dev, then prod
Production DB -> generated production help.hlp
```

Within each environment:

- The database is the active runtime catalog.
- `help.hlp` is a deterministic projection of the catalog fields supported by
  the legacy format.
- A file-only development change is an import candidate, not an independent
  permanent authority. It must be normalized into the development database and
  exported again.
- A database-only change causes the file to be regenerated.
- The tool must report unexplained DB/file drift rather than silently selecting
  the newest timestamp.

This preserves both required layers while reducing synchronization to two real
branches: development content and production content.

## 4. Canonical logical representation

Both databases and both text files should be parsed into one normalized model.
The recommended exchange and review format is deterministically ordered JSONL
or an equivalently lossless format.

Each logical entry should contain:

- normalized tag;
- entry body;
- sorted keyword set;
- minimum and maximum level;
- category;
- `auto_generated` state; and
- sorted related-topic relationships and relevance values.

Database IDs, creation timestamps, update timestamps, search analytics, and
version-history row IDs should not participate in the semantic content hash.

Each entry receives a hash over its normalized content. The complete catalog
receives a hash over the ordered entry and relationship hashes. A manifest also
records:

- format and schema version;
- exporter version;
- environment identity;
- source Git/build identity when available;
- entry and relationship counts;
- content hashes; and
- snapshot time for audit purposes only.

The recommended durable review artifacts are:

1. a lossless normalized catalog;
2. the deterministically rendered `help.hlp`; and
3. a compact manifest.

The implementation stores ignored operational artifacts below
`lib/text/help/.help-sync/` on each endpoint:

- `baseline/` stores the last catalog successfully applied to both endpoints;
- `plans/` stores explicitly saved, content-addressed plans;
- `proofs/` stores development apply and verification evidence; and
- `runs/` stores targeted logical backups and run results.

Plans include their lossless source and candidate catalogs. The tracked
`help.hlp` remains reviewable in Git, while secrets and operational artifacts do
not enter the repository.

## 5. Three-way reconciliation

Let:

- `B` be the last catalog successfully applied to both environments;
- `D` be the current normalized development catalog; and
- `P` be the current normalized production catalog.

The initial per-entry decision matrix is:

| Development | Production | Result |
|-------------|------------|--------|
| `D == B` | `P == B` | No change |
| `D != B` | `P == B` | Accept the development change |
| `D == B` | `P != B` | Accept the production change |
| `D == P`, both differ from `B` | Same change on both sides; accept once |
| `D != B`, `P != B`, and `D != P` | Attempt field-level merge or report conflict |

Field behavior should be explicit:

- Entry body, level bounds, category, and generation state are scalar fields.
  Different concurrent changes to the same scalar are conflicts.
- Keywords and related topics are sets. Independent additions can merge.
  Removing an item changed by the other side is a conflict.
- Changing separate fields can merge automatically when both changes are
  independently valid.
- Deleting on one side while editing on the other is always a conflict.
- A tag rename must be represented explicitly. It must not be inferred from an
  unrelated deletion and addition.
- Missing data is never interpreted automatically as permission to delete.
  Deletions require an explicit tombstone in the sealed plan.

Every unresolved conflict should show the base, development, and production
values. The system must not choose a winner based on timestamps or environment
priority.

## 6. Implemented synchronization workflow

### 6.1 Read-only snapshot and plan

1. Assert that the local checkout reports the development environment and that
   the configured remote identifies itself as production.
2. Export a transactionally consistent content snapshot from each database.
3. Snapshot and parse each `help.hlp`.
4. Compare DB and file projections within each environment and report layer
   drift.
5. Load the last common catalog `B`.
6. Produce the three-way merged candidate and a conflict report.
7. Emit a sealed, content-addressed plan containing every proposed addition,
   update, deletion, and layer repair.

This phase must be safe to run unattended because it performs no writes. A
scheduled audit may report drift, but it must not apply changes automatically.

### 6.2 Resolve and validate on development

1. Resolve every conflict explicitly and reseal the candidate.
2. Back up the affected development rows and current development `help.hlp`.
3. Apply the candidate to the development content tables in one transaction.
4. Render `help.hlp` to a temporary file, validate it, and rename it atomically.
5. Invalidate the runtime help cache through a supported interface or restart
   the development MUD if no safe invalidation path exists.
6. Re-export the database and require an exact semantic hash match with the
   candidate.
7. Run database integrity, parser round-trip, and representative in-game help
   lookup checks.

Development is the proving ground for the exact candidate later offered to
production. Production must not receive a newly rebuilt variant.

### 6.3 Guarded production apply

1. Present the sealed plan ID, candidate hash, counts, additions, changes,
   deletions, conflicts resolved, and proposed backup location.
2. Require explicit authorization for the production mutation.
3. Enter a narrow HEDIT write lock or maintenance barrier. The entire game need
   not stop if help writes and cache invalidation can be isolated safely.
4. Re-export production and require its current hash to match the hash used to
   create the plan. If it changed, abort and re-plan.
5. Create a targeted database backup and an exact copy of production
   `help.hlp`.
6. Apply only the sealed row-level delta in one transaction, using expected old
   hashes as optimistic-concurrency preconditions.
7. Render and atomically install production `help.hlp` from the resulting
   database state.
8. Invalidate the help cache or perform the approved controlled restart.
9. Re-export and require the database, file projection, and candidate hashes to
   agree.
10. Run health, log, and representative help-lookup checks.
11. Record the candidate as the new common baseline, then release the HEDIT
    lock.

If verification fails, restore the targeted database backup and text file,
invalidate the cache again, verify the restored hash, and retain the failed run
artifacts for diagnosis.

## 7. Safety invariants

The implementation should enforce all of the following:

- Planning is read-only and is the default operation.
- No production write occurs without an exact sealed plan and explicit
  authorization.
- No plan applies if production changed after its snapshot.
- No unresolved conflict applies.
- No implicit deletion, blanket table truncation, or timestamp-based
  last-writer-wins behavior is allowed.
- Database updates are transactional; file replacement is atomic.
- A second application of the same candidate is a no-op.
- Backups are created and validated before mutation.
- Credentials are read from existing protected configuration and never copied
  into logs or manifests.
- Content tables are allowlisted. Player data, search analytics, and unrelated
  database tables are out of scope.
- Each run retains enough evidence to explain exactly what was observed,
  approved, written, verified, or rolled back.

## 8. Tool and skill boundary

Synchronization mechanics live in one repository-owned deterministic tool:

```text
scripts/help-sync/help_sync.py audit
scripts/help-sync/help_sync.py baseline-init ...
scripts/help-sync/help_sync.py plan [--repair-integrity] [--repair-layers] [--save]
scripts/help-sync/help_sync.py resolve <plan> --resolutions <file> [--save]
scripts/help-sync/help_sync.py show <plan>
scripts/help-sync/help_sync.py apply-dev <plan>
scripts/help-sync/help_sync.py preview-prod <plan>
scripts/help-sync/help_sync.py apply-prod <plan> --authorize-plan <id> \
  --authorize-preview <token>
scripts/help-sync/help_sync.py sync --authorize-production [--repair-layers] \
  [--max-passes <1-10>]
scripts/help-sync/help_sync.py verify <plan> --environment both
scripts/help-sync/help_sync.py rollback <run-id> --environment <target> \
  --expected-current-hash <hash>
```

The repository-local `.agents/skills/help-sync` skill routes read-only audit and
planning, implementation work, guarded development and production apply,
verification, and rollback. It orchestrates this tool rather than embedding
SQL, SSH, merge, or file-writing logic in prose. The skill defaults ambiguous
operational requests to read-only work.

All modes must call the same implementation so their normalization, conflict,
and validation rules cannot diverge. A later split into separate reconcile and
apply skills is unnecessary unless real usage shows that the single entry point
causes routing or safety problems.

The `sync --authorize-production` command is the bounded autonomous composition of those same
primitives. It is valid only for an explicit user request to complete synchronization. It generates
and stores a fresh plan, automatically includes supported integrity repairs, applies and proves
development, emits the exact production preview, binds the fresh preview token internally,
publishes, reloads, advances the common baseline, and verifies both endpoints. It refuses unresolved
conflicts, deletions, and renames. It also holds the development HEDIT barrier across publication
and can make bounded follow-up passes if supported concurrent edits arrive. Database-to-file layer
repair remains explicit through `--repair-layers` after file-only work has been inspected and, when
legitimate, normalized into the development database.

## 9. Verification requirements

At minimum, automated tests should cover:

- deterministic export independent of row order and database IDs;
- lossless normalized database round-trip;
- deterministic `help.hlp` generation and parsing;
- production-only and development-only additions;
- identical concurrent changes;
- independent field and keyword additions;
- same-field conflicts;
- edit-versus-delete conflicts;
- explicit deletion and rename handling;
- stale-plan refusal after a new HEDIT save;
- transaction rollback on partial failure;
- atomic file replacement failure;
- cache invalidation or restart verification;
- exact post-apply re-export hashes;
- idempotent repeat application;
- service-owner remote execution without credentials in command arguments;
- bounded end-to-end orchestration and concurrent-development follow-up passes;
- environment and target-path refusal gates; and
- proof that analytics and unrelated tables are untouched.

Production behavior should first be exercised against isolated development
databases and temporary text roots. No test should point at production.

Implementation evidence recorded on 2026-08-24:

- `make test` passed, including all 868 production-linked CuTests and the two
  help-sync runtime guard tests.
- Python discovery passed 27 active unit tests; the eight MariaDB tests are
  intentionally skipped unless their explicit environment gate is set.
- The separately gated isolated MariaDB run passed all eight tests, covering
  apply, exact verification, idempotence, explicit rollback, automatic rollback
  after file failure, transaction rollback after database failure, stale-plan
  refusal after either a database or file edit, environment refusal,
  unrelated-table preservation, the exact embedded collision-safe migration
  sequence, and bounded autonomous reconciliation across isolated development
  and production endpoints.
- Both Autotools and CMake help-sync test targets passed, `ruff` passed, the
  normal C build completed without warnings, `make install` succeeded, and no
  root-level `luminari` artifact remained.

This is development and isolated-fixture evidence, not a claim of live
production deployment or validation.

## 10. Implementation status

All five implementation phases are present in the development checkout:

1. The versioned canonical catalog, database snapshot, legacy parser, renderer,
   and semantic hashes are implemented in `scripts/help-sync/catalog.py` and
   `scripts/help-sync/endpoint.py`.
2. Audit, durable baseline handling, field-aware three-way merge, explicit
   tombstones and renames, conflict resolution, and sealed plans are exposed by
   `scripts/help-sync/help_sync.py`.
3. Development apply uses targeted backups, a database transaction, atomic file
   replacement, runtime reload acknowledgment, exact verification, and an
   automatic compensating rollback on post-transaction failure.
4. Production apply requires a previously written development proof, a fresh
   production preview, exact plan and preview authorization values, a
   last-moment source check, the HEDIT barrier, the MySQL advisory lock, and the
   same backup/apply/verify/rollback implementation.
5. The repository-local skill describes both the guarded manual workflow and the explicitly
   authorized bounded `sync` workflow. Scheduled audit automation is optional and has not been
   enabled; unattended or scheduled production writes remain disabled.

The database contract is enforced by collision-safe migrations numbered
2026082401 through 2026082408. Startup now fails if those migrations cannot make
the help content and history tables write-ready. The canonical new-install
contract is also recorded in `sql/components/help_sync_schema.sql`, with a
read-only verifier in `sql/components/verify_help_sync_schema.sql`.

Production deployment, its required server restart/schema upgrade, a read-only
live audit, and explicit first-baseline selection remain operational rollout
steps. They are intentionally not inferred from implementation work.

## 11. Resolved implementation decisions

1. Lossless operational state lives below the ignored
   `lib/text/help/.help-sync/` directory on both endpoints.
2. HEDIT remains database-first. The tracked file is regenerated only through
   an explicit reconciliation apply; ordinary staff edits are discovered by
   the next audit.
3. A `.help_sync.lock` file blocks HEDIT and mutating HELPGEN paths, while a
   named MySQL advisory lock closes the database race during apply.
4. Runtime invalidation uses a tokenized request/acknowledgment handshake polled
   by the game heartbeat. Apply succeeds only after the exact candidate token is
   acknowledged.
5. `auto_generated` is semantic content and is synchronized by default.
6. Sync-authored history uses `changed_by = 'help-sync'` and records the sealed
   plan ID in `help_versions.sync_plan_id`; existing history is never copied.
7. Renames are explicit plan metadata applied as a deletion plus addition. No
   new persistent identity was added beyond the current tag key.

## 12. Deployment and first-use runbook

Install the Python dependency from `scripts/help-sync/requirements.txt` on both
endpoints. Deploy the same revision to development and production, then restart
each through its approved environment-specific path so migration version
2026082408 and the runtime reload handler are active. Do not use
`luminari.service` for local development.

From the development checkout:

```bash
python3 scripts/help-sync/help_sync.py audit
python3 scripts/help-sync/help_sync.py baseline-init \
  --source development --authorize-hash <audited-catalog-hash>
python3 scripts/help-sync/help_sync.py plan --repair-integrity --repair-layers --save
python3 scripts/help-sync/help_sync.py show <plan-id>
python3 scripts/help-sync/help_sync.py apply-dev <plan-id>
python3 scripts/help-sync/help_sync.py preview-prod <plan-id>
python3 scripts/help-sync/help_sync.py apply-prod <plan-id> \
  --authorize-plan <plan-id> --authorize-preview <fresh-preview-token>
python3 scripts/help-sync/help_sync.py verify <plan-id> --environment both
```

After the first common baseline exists, an explicit user request for a complete, non-destructive
sync can use the bounded route instead:

```bash
python3 scripts/help-sync/help_sync.py sync --authorize-production [--repair-layers]
```

This command does not authorize tombstones, renames, or conflict resolution. Those remain explicit
review decisions. The production flag authorizes only the exact zero-deletion plans and fresh
preview tokens created during that single invocation; bounded re-planning handles supported edits
that arrive during the run.

The first baseline source is an operator decision. Select `production` instead
of `development` when production is the approved common starting catalog, or
use `file` with `--catalog-file` for a separately reviewed canonical catalog.
The authorization hash must exactly match the selected catalog. Baseline
initialization writes artifacts only; it does not reconcile either database or
file.

Use `--tombstones` and `--renames` only with reviewed JSON metadata. A plan with
conflicts exits nonzero and cannot apply; resolve it with a conflict-ID keyed
JSON file and the `resolve` command. The `--repair-integrity` switch explicitly
seals only the repairs reported by the snapshots. It prevents the current
legacy orphan and missing-keyword issues from being silently discarded.

Database/file drift also blocks sealing by default. `--repair-layers`
explicitly authorizes regenerating each drifted `help.hlp` from its reviewed
database catalog. If an audit reveals legitimate file-only development work,
first normalize that entry into the development database through the ordinary
reviewed help-editing workflow; do not use `--repair-layers` until the database
contains it. File absence is never treated as an implicit content deletion.

For rollback, use the run ID printed by apply and the exact current catalog hash:

```bash
python3 scripts/help-sync/help_sync.py rollback <run-id> \
  --environment development --expected-current-hash <hash>
python3 scripts/help-sync/help_sync.py rollback <run-id> \
  --environment production --expected-current-hash <hash> \
  --authorize-run <run-id>
```

Always inspect current `--help` output before an operational run. A failed
freshness, schema, environment, barrier, backup, reload, or verification check
aborts the apply; failures after database commit trigger the targeted
compensating rollback automatically.
