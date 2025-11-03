SQL Schema Generation Plan
==========================

Goal
----
Produce an authoritative SQL schema for LuminariMUD that matches the expectations embedded in code, scripts, and configuration. The plan below captures everything needed before drafting SQL files.

Preparation
-----------
1. Confirm local environment can build the codebase and run tooling without sudo.
2. Ensure `rg`, `ctags`, and `jq` (optional) are available for static analysis helpers.
3. Back up any existing database instances so exploratory commands cannot damage data.

Phase 1: Inventory Data Touchpoints
-----------------------------------
1. Scan source for SQL API usage:
   - `rg -n "mysql_" src sql` to locate MySQL C API calls.
   - `rg -n "\"SELECT" src sql` and similar for other SQL verbs.
   - Note wrapper headers (e.g., `mysql.h`, `db.h`) and any custom abstraction layers.
2. Review `sql/components/` for partial schemas, migration fragments, or helper scripts.
3. Inspect shell/perl/python scripts under `scripts/` that might initialize tables.
4. Enumerate configuration files in `lib/etc/` that reference table names or credentials.

Phase 2: Map Logical Modules to Tables
--------------------------------------
1. For each subsystem (player accounts, clans, quests, mail, etc.), list structs and data persisted to SQL.
2. Trace struct fields to serialized columns by following `snprintf`/`sql_escape` paths.
3. Record inferred table names, column names, and value domains in a working spreadsheet or markdown table.
4. Identify implicit relationships (foreign keys) from code-level joins or ID cross references.

Phase 3: Derive Column Definitions
----------------------------------
1. For every recorded column:
   - Determine type/size from struct definitions (`int`, `char[NAME_LENGTH]`, etc.).
   - Note default values, nullability, and enum semantics based on conditional logic.
2. Catalog indices or unique constraints implied by query patterns (`WHERE name = ? LIMIT 1`, etc.).
3. Capture auto-increment expectations from insert logic (e.g., manual `LAST_INSERT_ID()` usage).

Phase 4: Assemble Draft Schema
------------------------------
1. Organize tables by dependency (core entities first, junction tables afterward).
2. Author `CREATE TABLE` statements in a staging markdown or SQL file under `sql/`.
3. Include explicit `PRIMARY KEY`, `FOREIGN KEY`, and index declarations consistent with findings.
4. Add comments referencing source files and line numbers supporting each design choice.

Phase 5: Validate Against Runtime
---------------------------------
1. Stand up a disposable MariaDB/MySQL instance (container or local) without sudo if possible; otherwise request assistance.
2. Apply the draft schema and run game initialization routines that touch the database.
3. Exercise representative workflows (character creation, clan management, etc.) to confirm no missing columns.
4. Capture logs for any SQL errors or warnings and fold fixes back into the draft schema.

Phase 6: Finalize Deliverables
------------------------------
1. Produce `sql/schema.sql` (or another agreed filename) containing ordered DDL statements.
2. Create accompanying documentation summarizing tables, columns, and relationships (markdown preferred).
3. Add automated sanity check: a script that loads the schema into an ephemeral database and exits cleanly.
4. Submit schema and docs for review, including a test log showing validation steps executed.

Tracking and Sign-off
---------------------
- Maintain a checklist mapping every SQL interaction to a schema element to ensure full coverage.
- Keep notes on assumptions or unresolved questions for stakeholder review before finalizing the schema.
