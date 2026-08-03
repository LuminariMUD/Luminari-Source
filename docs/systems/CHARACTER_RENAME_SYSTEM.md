# End-to-End Character Rename Fix

## Purpose

This document is the developer handoff for fixing character renames throughout
LuminariMUD. The fix must be implemented and tested in a development
environment. It must not be a `Bartof`/`Hartof` special case.

The required outcome is one authoritative rename operation that keeps the
player index, player file, account system, object saves, other name-keyed
database records, auxiliary files, and live caches consistent.

This is an implementation specification, not a statement that the source has
already been fixed. The audit was checked against the current source and the
deployed schema on 2026-07-23. The developer must implement the service and
pass the tests and acceptance criteria below in a disposable environment
before the rename command can be considered safe.

## Production incident and current status

The incident was triggered with:

```text
set file bartof name hartof
```

The command renamed the player-file identity and player-index entry to
`Hartof`, but the account system still obtained `Bartof` from
`player_data.name`. The account menu then tried to load a player file for
`Bartof`, which no longer existed.

Additional inconsistencies found during the incident:

- `player_data.name` was `Bartof`, linked to account ID 641.
- The actual database account spelling was `Claudmax` (the initial report said
  `Claudemax`).
- `lib/plrfiles/F-J/hartof.plr` existed and contained `Name: Hartof`.
- The offline save performed by `set file` had removed the player file's
  `Acct:` line.
- The in-memory/on-disk player index contained `hartof`.
- All 475 `player_save_objs` rows were still keyed as `Bartof`.
- The legacy object fallback file was still
  `lib/plrobjs/A-E/bartof.objs`.

Production was manually repaired on 2026-07-23:

- `player_data.name`: `Bartof` -> `Hartof`, preserving `account_id = 641`.
- 475 `player_save_objs.name` rows: `Bartof` -> `Hartof`.
- Restored `Acct: Claudmax` in the `Hartof` player file.
- Moved the legacy object fallback file to
  `lib/plrobjs/F-J/hartof.objs`, preserving its contents byte-for-byte.
- Verified the account row resolves to `Hartof`, its object-save header is
  present, all 475 inventory rows resolve under `Hartof`, and the audited live
  data stores have zero stale `Bartof` ownership rows.

The production rollback snapshot is outside the repository at:

```text
/home/luminari/manual-patch-backups/20260723T1928Z-bartof-to-hartof
```

No source-code change was made as part of the production repair.

A second hand-patch on 2026-07-23 renamed `DollhouseTestChar` to `Ralmont`.
That repair independently demonstrated that the broader rename map is
necessary:

- one canonical `player_data` row was renamed without changing account ID 641;
- 118 `player_save_objs` rows were re-keyed;
- three `loot_chests` rows were re-keyed;
- one `pet_data` row was re-keyed;
- the player file retained/restored `Acct: Claudmax`;
- the `.objs` fallback file followed the new sharded path; and
- a post-patch audit found zero remaining `DollhouseTestChar` rows in every
  deployed active-name key listed in this document, including objects, pets,
  eidolons, mail, quests, crafting, loot, and pubsub.

Its rollback snapshot is:

```text
/home/luminari/manual-patch-backups/20260723T1951Z-dollhousetestchar-to-ralmont
```

This was also a data-only repair; it did not fix `change_player_name()`.

## Root cause in the current code

### 1. `change_player_name()` only updates part of the identity

[`change_player_name()`](../../src/act.wizard.c#L7851) currently:

1. checks whether the new name can be loaded;
2. changes `player_table[i].name`;
3. changes `GET_PC_NAME(vict)`;
4. invokes `mv` for the `.plr` file through `system()`;
5. saves the player index; and
6. returns success.

It does not rename any database ownership keys, account character entries,
object files, script-variable files, or other character-name data.

The account list is loaded independently with:

```sql
SELECT name FROM player_data WHERE account_id = ?
```

See
[`load_account_characters()`](../../src/account.c#L773). Consequently,
`player_data.name` and the player-file/index name must always agree.

### 2. The account menu trusts the stale database name

[`show_account_menu()`](../../src/account.c#L1108) iterates the names loaded from
`player_data`, then calls `load_char()` for each name. Selecting a menu entry
also calls `load_char()` with the account's stored name in
[`interpreter.c`](../../src/interpreter.c#L2761).

After the partial rename, these paths asked for `Bartof` while only
`hartof.plr` existed.

### 3. Offline saves can drop the durable account identity

The player-file loader reads `Acct:` into `GET_ACCOUNT_NAME(ch)` in
[`players.c`](../../src/players.c#L715).

The save path writes `Acct:` only when all of the following are present:

```c
ch->desc && ch->desc->account && ch->desc->account->name
```

See [`save_char()`](../../src/players.c#L2148). A character loaded by
`set file ...` has no descriptor, so saving the renamed offline character
silently removes `Acct:` even though `GET_ACCOUNT_NAME(ch)` was loaded.

That omission also breaks the account auto-relink path in
[`interpreter.c`](../../src/interpreter.c#L2862), which intentionally uses the
player-file account name when restoring an unlinked character.

### 4. Object persistence is keyed by character name

With `OBJSAVE_DB`, object loading first obtains `obj_save_header` from
`player_data` and then loads inventory from `player_save_objs` using
`GET_NAME(ch)`. See [`Crash_load_objs()`](../../src/obj/objsave.c#L2744) and
[`objsave_parse_objects_db()`](../../src/obj/objsave.c#L2211).

The following stores are name-keyed:

- `player_data.name`
- `player_save_objs.name`
- `player_save_objs_sheathed.owner_name`
- `pet_data.owner_name`
- `pet_save_objs.owner_name`
- `player_eidolons.owner`
- the `.objs` fallback path generated for `CRASH_FILE`

Renaming only `player_data` would restore the account menu but could make the
character enter with no inventory, sheathed items, pets, pet equipment, or
eidolon descriptions.

The object and pet payloads themselves must not be regenerated as part of a
rename. `player_data.obj_save_header`, object row IDs/order, pet IDs, sheath
relationships, and every `serialized_obj` value must survive unchanged. Bound
object ownership is serialized as the immutable numeric `GET_OBJ_BOUND_ID`
(`Bind:`), so it must not be text-replaced with a character name.

[`save_char_pets()`](../../src/players.c#L5227) is specifically unsafe as a migration
mechanism: it deletes all `pet_save_objs` and `pet_data` rows for the owner and
then rebuilds them from currently loaded followers. Rename the ownership
columns in place instead.

### 5. Other subsystems also use character names as keys

The current source and production schema contain more character-name keys,
including mail state, quest state, loot timers, crafting orders, level
history, and pubsub settings/subscriptions. These must be deliberately
classified and migrated.

Two easy-to-miss active keys are:

- `player_eidolons.owner`, used by `save_eidolon_descs()` and
  `set_eidolon_descs()` in [`players.c`](../../src/players.c#L5527); and
- both `player_mail.sender` and `player_mail.receiver`, used by inbox/outbox
  listing, read access, delete access, and mail alerts in
  [`new_mail.c`](../../src/comms/new_mail.c#L308).

The main mail sender is not merely a historical label: sent-mail retrieval is
keyed by the current `GET_NAME(ch)`. Both columns therefore have to migrate,
along with `player_mail_read.player_name` and
`player_mail_deleted.player_name`.

The introduction feature is a file-based fan-out relationship. Every player's
`Intr:` block stores names of other characters, and
[`knows_character()`](../../src/introduce.c#L28) compares those strings to
`GET_NAME(vict)`. The deployed configuration currently has
`use_introduction_system = 0`, but the data format and runtime feature still
exist. A rename must have an explicit policy for these references before that
feature is enabled.

### 6. Live caches can remain stale

Each descriptor can own a separate `account_data` instance. Updating one
descriptor's `character_names[]` does not update every descriptor already
logged into the same account.

Pubsub also hashes and caches subscriptions by player name. See
[`pubsub_invalidate_player_cache()`](../../src/pubsub/pubsub_db.c#L440).
Both the old-name and new-name cache entries must be invalidated after a
successful rename.

### 7. Current file and error handling is not reliable enough

The existing implementation builds:

```c
snprintf(buf, sizeof(buf), "mv %s %s", old_pfile, new_pfile);
system(buf);
```

Problems:

- It unnecessarily invokes a shell.
- It only treats `system() == -1` as failure; a launched `mv` that exits
  nonzero is not handled as a rename failure.
- It changes in-memory state before confirming the file move succeeded.
- `save_player_index()` and `save_char()` return `void`, write directly to
  their final files, and cannot report a durable-save failure to the rename
  operation.
- If the player ID is not found, the current loop can leave
  `i == top_of_p_table + 1` and then dereference `player_table[i]`.
- The temporary `char_data` allocated while checking the new name is not freed
  on the normal "new name does not exist" path.

## Required invariants

The rename must report success only if all applicable invariants are true:

1. The immutable player-file/index ID (`GET_IDNUM`) is unchanged.
2. The account ID is unchanged.
3. The new name is valid and unused, using case-insensitive collision checks
   consistent with login and the database collation.
4. `player_table`, `player_data`, the `.plr` filename, and the `Name:` value in
   the `.plr` file all identify the new name.
5. The `.plr` file retains the correct `Acct:` value.
6. All current ownership/state rows use the new name.
7. No old-name ownership/state rows remain in any supported active table.
8. Object, pet, sheath, and eidolon payloads and immutable IDs are unchanged;
   only their name-based lookup keys move.
9. Auxiliary files are available at paths derived from the new name.
10. Introduction-list references follow the configured feature policy.
11. Existing descriptors and other live name caches no longer retain an
    authoritative old-name key.
12. A server restart/copyover preserves the result.
13. Any failure leaves the old identity fully usable; partial success must not
    be presented to the administrator.

## Recommended implementation

### A. Add one rename service

Move orchestration out of `act.wizard.c` into a focused service, for example:

```text
src/player_rename.c
src/player_rename.h
```

Suggested public API:

```c
enum player_rename_status
{
  PLAYER_RENAME_OK = 0,
  PLAYER_RENAME_INVALID_NAME,
  PLAYER_RENAME_NAME_EXISTS,
  PLAYER_RENAME_PLAYER_NOT_FOUND,
  PLAYER_RENAME_DATABASE_UNAVAILABLE,
  PLAYER_RENAME_DATABASE_ERROR,
  PLAYER_RENAME_FILE_COLLISION,
  PLAYER_RENAME_FILE_ERROR,
  PLAYER_RENAME_SAVE_ERROR,
  PLAYER_RENAME_POSTCONDITION_FAILED
};

struct player_rename_report
{
  long player_id;
  int account_id;
  unsigned int database_rows_changed;
  unsigned int files_moved;
  enum player_rename_status status;
};

enum player_rename_status rename_player_everywhere(
    struct char_data *actor,
    struct char_data *victim,
    const char *requested_name,
    struct player_rename_report *report);
```

Both `set <online-player> name ...` and `set file <offline-player> name ...`
must use this same service.

Do not add separate ad hoc SQL to the `set file` branch. That would leave the
online rename path inconsistent.

### B. Canonicalize and preflight before changing anything

Capture immutable and old state first:

```c
old_display_name = GET_NAME(victim);
player_id = GET_IDNUM(victim); /* pfile/player-index identity */
player_index_position = ...;
account_id = ...; /* from the locked player_data row */
```

Preflight requirements:

- Apply `_parse_name()`, `valid_name()`, `fill_word()`, `reserved_word()`, and
  length checks consistently with character creation.
- Reject or explicitly treat `strcasecmp(old_name, new_name) == 0` as a no-op.
  Filenames and the player index are lowercase, so a case-only "rename" is not
  a normal path rename.
- Check the in-memory world, player index, player file, and `player_data` for a
  case-insensitive new-name collision.
- Find the player index by immutable ID and use a bounded/not-found check
  before indexing the array.
- Require a healthy database connection. A rename must fail closed when MySQL
  is unavailable because the account system is database-backed.
- Lock and verify exactly one `player_data` row for the old name. Preserve any
  database row ID if that deployment has one.
- Generate every old/new path with `get_filename()` rather than building paths
  manually.
- Require the source `.plr` file to exist and the destination `.plr` file not
  to exist.
- For optional auxiliary files, allow a missing source, but reject an existing
  destination.
- Check every active current-state table for destination-name rows. A valid
  new character name can still have orphaned/stale rows in a non-unique table;
  do not silently merge those rows with the character being renamed.

The old and new names should be escaped with
`mysql_real_escape_string()`/`mysql_escape_string_alloc()` for every query.
Prepared statements would be preferable if introduced consistently.

### C. Enumerate auxiliary files

Use `get_filename()` for each supported mode:

| Mode | Data | Required behavior |
| --- | --- | --- |
| `PLR_FILE` | Main ASCII player file | Source required; rename and rewrite identity |
| `CRASH_FILE` | Legacy/fallback inventory | Rename if present |
| `SCRIPT_VARS_FILE` | Player DG script variables | Rename if present |
| `ETEXT_FILE` | Player text data, if enabled | Rename if present |

The first letter can move the file between shard directories, such as
`A-E/bartof.*` -> `F-J/hartof.*`. This case must be covered by tests.

Use the C/POSIX `rename()` function, not `system("mv ...")`. Treat errors other
than an absent optional source as fatal and include `strerror(errno)` in the
staff log.

### D. Migrate database keys in one transaction

Start an InnoDB transaction and lock the canonical player row:

```sql
START TRANSACTION;

SELECT id, account_id
FROM player_data
WHERE LOWER(name) = LOWER(?)
FOR UPDATE;
```

Verify:

- exactly one old-name row was returned;
- the new name does not exist; and
- the account ID is retained.

Do not assume an auto-increment `player_data.id` equals `GET_IDNUM(victim)`.
The current insertion paths can create `player_data` rows with only
`name`/`account_id`, and production schemas have differed from
`sql/master_schema.sql`. `GET_IDNUM` is currently authoritative in the
player file/index. If a deployment has a separate database row ID, capture
and preserve it without conflating the two identities.

Update `player_data.name` and every applicable current-state ownership table
in the same transaction. Every SQL error must cause `ROLLBACK`.

Confirm every table in the transactional rename map uses InnoDB (or another
transactional engine). A `START TRANSACTION` does not make updates to a MyISAM
legacy table rollback-safe. Migrate nontransactional current-state tables
before relying on atomic rename behavior.

The 2026-07-23 deployed-schema audit found the object, pet, eidolon, loot,
quest, crafting, and pubsub current-state tables on InnoDB. It also found all
three active mail tables on MyISAM:

```text
player_mail
player_mail_deleted
player_mail_read
```

Converting these three tables to InnoDB in a reviewed schema migration is a
prerequisite for the preferred atomic implementation. Back up and validate
them before conversion. If conversion cannot happen, the rename service needs
explicit table locking, exact row snapshots, checked compensating writes, and
failure-injection tests for the MyISAM portion; a normal SQL `ROLLBACK` is not
sufficient.

The active/core rename map is:

| Table | Character-name column | Notes |
| --- | --- | --- |
| `player_data` | `name` | Canonical account-menu and object-header row; exactly one row must change |
| `player_save_objs` | `name` | Inventory/equipment DB rows |
| `player_save_objs_sheathed` | `owner_name` | Sheathed object rows |
| `pet_data` | `owner_name` | Saved pet/companion records |
| `pet_save_objs` | `owner_name` | Saved pet inventory |
| `player_eidolons` | `owner` | Eidolon descriptions loaded by current owner name |
| `loot_chests` | `character_name` | Per-character loot state |
| `player_mail` | `sender` | Required for the renamed character's sent-mail listing and access |
| `player_mail` | `receiver` | Required for inbox listing, access, and alerts; do not alter the special `All` receiver |
| `player_mail_deleted` | `player_name` | Per-character deleted-mail state |
| `player_mail_read` | `player_name` | Per-character read-mail state |
| `player_quest_info` | `character_name` | Per-character quest state |
| `player_quest_progress` | `character_name` | Per-character quest progress |
| `player_supply_orders` | `player_name` | Crafting order state, where this table exists |
| `supply_orders_available` | `player_name` | Crafting order availability |
| `pubsub_player_settings` | `player_name` | Per-character pubsub limits/settings |
| `pubsub_subscriptions` | `player_name` | Per-character subscriptions |
| `player_levelups` | `character_name` | Newer level-history path, where present |
| `player_levels` | `char_name` | Legacy level-history path, where present |

The production schema also contained these legacy/reporting objects:

| Table | Candidate column | Expected decision |
| --- | --- | --- |
| `player_data2` | `name` | It had zero rows and no current source reference on 2026-07-23. Treat it as obsolete unless a deployment owner proves it is a live mirror; remove it in a separate migration or include it explicitly if reactivated. |
| `level_30_characters` | `name` | It is a view over `player_data`, not a writable table. Do not update it; verify that it reflects the new name after `player_data` changes. |

Do not blindly rename every column called `name`. Tables such as
`account_data`, `path_data`, `quest_lines`, `region_data`, and
`pubsub_topics` use `name` for a different entity.

Likewise, explicitly decide whether historical display snapshots should keep
the name used at event time. Examples include pubsub message
`sender_name`/metadata, `mysql_board_posts.author`,
`ship_cargo_manifest.loaded_by`, and creator fields. These deployed fields are
not current ownership/retrieval keys and have a numeric or separate owning
identity where applicable. The recommended policy is to retain them as
historical snapshots. If product requirements choose display-name rewriting
instead, implement it as a separately named policy—not as a broad text
replacement.

Because deployments may not all have the legacy/optional tables, either:

- put each optional update behind a schema/feature check such as
  `table_exists()` plus a column check; or
- normalize the schema with a migration so the rename service has one
  guaranteed table set.

Do not ignore an update error for a table that exists. Roll back the whole
database transaction.

Use `mysql_affected_rows()` for observability, but do not require at least one
row in optional tables. Require exactly one canonical `player_data` row.
Capture pre-update counts for each old-name key and verify the same counts are
owned by the new name before commit. A destination-name collision should fail,
not be folded into those counts.

There is also schema drift to resolve or explicitly accommodate:

- [`src/db_init.c`](../../src/db_init.c#L147) creates `player_data.account_id`.
- The current
  [`sql/master_schema.sql`](../../sql/master_schema.sql#L31) definition does not
  include `account_id`.
- The deployed `player_mail`, `player_mail_read`, `player_mail_deleted`, and
  `player_eidolons` tables are not defined in the checked-in master schema.
  Add reviewed schema/migration definitions, including the required InnoDB
  engines and useful old/new-name indexes, rather than relying on undocumented
  production-only tables.
- Production inspection during this incident showed `player_data.name` as the
  primary key, while the checked-in master schema models a separate
  auto-increment primary key plus a unique name.

The developer should establish which schema is canonical, update the checked-in
schema/migrations accordingly, and run rename tests against the same shape used
in deployment. The rename service must not silently assume the master schema
matches every existing database.

### E. Preserve object, pet, and eidolon payloads exactly

The rename is a key migration, not a resave. Capture these preconditions and
compare them after the rename and again after restart/copyover:

| Store | Values that may change | Values that must not change |
| --- | --- | --- |
| `player_data` | `name` | row identity, `account_id`, and `obj_save_header` bytes |
| `player_save_objs` | `name` | row count, `idnum`, `creation_date`, and `serialized_obj` bytes |
| `player_save_objs_sheathed` | `owner_name` | row count, row ID, `sheath_obj_id`, `sheathed_position`, and `serialized_obj` bytes |
| `pet_data` | `owner_name` | row count, `pet_data_id`, vnum/stats, and user-authored pet descriptions |
| `pet_save_objs` | `owner_name` | row count, row ID, `pet_idnum`, `creation_date`, and `serialized_obj` bytes |
| `player_eidolons` | `owner` | row count, `idnum`, `short_desc`, and `long_desc` |
| legacy `.objs` | pathname only | complete file bytes and metadata as required by the file policy |

Specific implementation rules:

- Move the `.objs` fallback whenever it exists, even when
  `player_data.obj_save_header` is present. When that header is absent, the
  fallback file is the active inventory source.
- Do not call `Crash_save()`, `save_char_pets()`, or another normal gameplay
  resave to synthesize migrated rows. Those paths can delete/reorder rows or
  serialize only what is currently loaded.
- Preserve the `pet_data.pet_data_id` to `pet_save_objs.pet_idnum`
  relationship. Updating only `pet_data.owner_name` strands pet inventory;
  deleting/reinserting the pet can assign a new ID and do the same.
- Do not search/replace the old character name inside `serialized_obj` or free
  text. Object bindings use immutable numeric `GET_OBJ_BOUND_ID`, and custom
  object/pet/eidolon descriptions may legitimately contain the same text.
- Saved clones (`MOB_CLONE`, currently vnum 10) are a special derived-display
  case. `load_char_pets()` assigns the current owner name and then overwrites
  it with saved `pet_name`/`pet_sdesc`. Fix that ordering so a clone's derived
  name and short description are generated from the current owner after
  loading, while ordinary user-authored pet text remains byte-for-byte
  unchanged. Add a rename/restart test for a saved clone.
- Validate per-row identity and payload equality in the integration fixture,
  not just aggregate counts. Equal counts would not detect a changed object
  serialization or a broken pet/sheath join.

Houses (`house_control.owner`), persistent vehicles (`vehicle_data.owner_id`),
player-corpse recovery (`GET_OBJ_VAL(corpse, 4)`), mission ownership
(`mission_owner`), clan leadership/membership, board read/visit state, and
legacy filesystem mudmail use immutable numeric player IDs or non-name clan
IDs. They require no persisted rename update. Tests should still prove those
systems work after the ID-preserving rename.

### F. Handle names stored in other players' files

The current `Intr:` player-file block stores character names in every player
who knows that character. Renaming only the target's file leaves all of those
references stale. Although production currently configures the introduction
system off, the source can enable it at runtime, so this cannot remain an
undocumented hole.

Choose and test one supported design:

1. Preferably migrate introduction storage to immutable player IDs and resolve
   the display name through the player index. Include a one-time conversion
   for existing name entries and preserve unknown entries safely for review.
2. As an interim design, locate every exact case-insensitive old-name `Intr:`
   entry, atomically rewrite it to the canonical new name, refresh the
   `intro_list[]` arrays of online characters, and include all touched player
   files in the rename's rollback journal.

Do not run an unrestricted text replacement across player files; only parse
and modify `Intr:` records. Do not silently skip this step merely because the
current production toggle is off; stale entries would become active if the
feature were enabled later. The only alternative is to remove/deprecate the
introduction feature and its persisted data in a separate, explicit change.

### G. Make player-file and index saves report failures

For a trustworthy all-or-nothing operation, the rename service must know
whether the player file and index were saved successfully.

Recommended changes:

- Change `save_char()` (or add a checked variant) to return success/failure.
- Write the new player file to a same-directory temporary file.
- `fflush()`, check `ferror()`, optionally `fsync(fileno(...))`, close it, and
  atomically `rename()` it over the destination.
- Change `save_player_index()` (or add a checked variant) to use the same
  temporary-file pattern and return success/failure.
- Preserve file mode/ownership as appropriate.
- Separate the player-file serialization step from `save_account()` side
  effects, or add a rename-safe save mode. The current `save_char()` can call
  `save_account()` for online characters while the rename is still in
  progress; atomic file persistence must not implicitly mutate stale account
  arrays.

At minimum, the rename operation must retain recoverable copies of the old
player file and player index until the database commit and postcondition
checks succeed.

### H. Preserve `Acct:` for descriptorless saves

Fix the player-file save logic independently of rename orchestration:

```c
const char *account_name = NULL;

if (ch->desc && ch->desc->account && ch->desc->account->name)
  account_name = ch->desc->account->name;
else if (GET_ACCOUNT_NAME(ch) && *GET_ACCOUNT_NAME(ch))
  account_name = GET_ACCOUNT_NAME(ch);

if (account_name)
  BUFFER_WRITE("Acct: %s\n", account_name);
```

When the database row has a valid account link, the rename service should
also make sure `GET_ACCOUNT_NAME(victim)` agrees with the owning
`account_data.name` before saving.

Avoid leaking the existing `GET_ACCOUNT_NAME` allocation when replacing it.

This change needs a direct regression test:

1. load an account-owned character with no descriptor;
2. save it;
3. reload it; and
4. assert that `GET_ACCOUNT_NAME()` still equals the original account.

### I. Use compensating rollback across SQL and files

MySQL and the filesystem cannot share one atomic transaction. Implement
explicit compensation:

1. Complete all validation and collision checks.
2. Start the SQL transaction and lock the canonical player row.
3. Create recoverable same-filesystem snapshots/staging files for the target
   player file, player index, and every introduction-bearing player file in
   the selected plan.
4. Rename/stage auxiliary files with `rename()`, recording each successful
   operation.
5. Apply SQL updates without committing.
6. Update the in-memory victim and player-index entry.
7. Save the new target player file, player index, and any parsed introduction
   updates with checked, atomic writers.
8. Verify pre-commit postconditions that are visible in the transaction.
9. Commit SQL.
10. Refresh caches and remove temporary rollback snapshots.

If a failure occurs before commit:

- issue `ROLLBACK`;
- restore each moved file in reverse order;
- restore the old player file, player index, and introduction-bearing files
  from the snapshots;
- restore `GET_PC_NAME(victim)` and `player_table[index].name`; and
- return an error without printing the normal success message.

If `COMMIT` itself fails, perform the same file/in-memory compensation and
report a high-severity staff error. Keep rollback snapshots when recovery is
not proven.

The MUD's command loop is single-threaded, which limits in-process
interleaving, but the SQL row lock still protects against an external
database repair or another server process using the same database.

### J. Refresh all in-memory state after commit

After a successful commit:

- Iterate `descriptor_list`.
- For every `d->account` with the renamed character's `account_id`, call
  `load_account_characters(d->account)`.
- Ensure an online victim's `GET_ACCOUNT_NAME()` still names the owning
  account.
- Invalidate pubsub caches for both `old_name` and `new_name`.
- Update any protocol/MSDP character-name value if the target is online.
- Refresh exact generated-name references still present in the live world,
  even when the rename target was loaded through `set file`:
  - `trail_data.name` entries that identify the target;
  - loaded `MOB_CLONE` followers derived from the target's name;
  - live player corpses whose numeric corpse owner is the target ID; and
  - live mission mobs whose `mission_owner` is the target ID.
- Refresh loaded `intro_list[]` data according to the introduction design in
  section F.
- Do not call `save_account()` with a stale `character_names[]` as a substitute
  for reloading it.

Refreshing every matching descriptor avoids requiring already-connected
account-menu users to reconnect.

Trails, corpse keywords/descriptions, and mission-mob owner text are generated
transient displays rather than persisted ownership keys; their authoritative
identity is already numeric where one exists. Update only records positively
identified as belonging to the renamed player. Never replace matching text in
arbitrary object, pet, mob, board, mail-body, or pubsub content.

### K. Improve administrator-facing results

The command should print success only after commit and postcondition checks.
A useful result is:

```text
Character Bartof (id 5063) renamed to Hartof.
Account link preserved; 475 object rows and 1 auxiliary file migrated.
```

On failure, report the failed stage without exposing database credentials or
private account data:

```text
Rename failed while migrating player_save_objs; all changes were rolled back.
See the server log for details.
```

Write one structured staff log entry containing:

- actor name/ID;
- old and new character names;
- immutable player ID;
- account ID, or `NULL` for a legacy unlinked character;
- per-table affected-row counts;
- moved-file count;
- final status; and
- rollback status on failure.

## Suggested implementation sequence

1. Reconcile the checked-in schema with deployment and migrate the three
   active mail tables from MyISAM to InnoDB.
2. Fix `save_char()` so descriptorless saves preserve `GET_ACCOUNT_NAME()`.
3. Add checked/atomic player-file and player-index writers.
4. Add file-path planning and `rename()`/rollback helpers for all four file
   modes.
5. Add a database rename helper with one transaction and the explicit rename
   map.
6. Implement the introduction-list ID migration or checked fan-out described
   in section F.
7. Add cache and generated-live-name refresh helpers.
8. Implement `rename_player_everywhere()`.
9. Reduce `change_player_name()` to permissions/UX plus a call to the service.
10. Add automated unit and integration coverage.
11. Run the manual end-to-end scenario in a disposable dev database and data
   directory.
12. Test a restart/copyover before considering the fix complete.

## Automated test plan

### Unit-level tests

- Valid name succeeds.
- Invalid, reserved, too-short, and too-long names fail without mutation.
- Case-insensitive collision in the player index fails.
- Case-insensitive collision in `player_data` fails.
- Case-only rename follows the documented reject/no-op behavior.
- Missing player-index ID returns an error without an out-of-bounds access.
- The temporary `char_data` used for collision checking is always freed.
- Path generation works within one shard and across shards.
- Missing optional `.objs`, `.mem`, or `.text` files is accepted.
- Existing auxiliary destination is treated as a collision.
- A non-`ENOENT` `rename()` error causes compensation.
- Descriptorless save preserves `Acct:`.
- SQL names are escaped correctly.
- Startup/preflight rejects a nontransactional active table unless the tested
  compensation mode is explicitly enabled.
- The canonical `player_data` update must affect exactly one row.
- An optional table changing zero rows is valid.
- The special mail receiver `All` is never rewritten.
- Introduction references migrate as parsed records or immutable IDs,
  regardless of the current feature toggle.
- A forced SQL error rolls back all previous SQL updates.
- A forced file/index save error rolls back SQL, files, and in-memory names.

Run memory/error tooling where available to catch the current collision-check
leak and rollback leaks.

### Integration fixture

Create a disposable account and character with:

- an account link;
- a player file containing `Name:` and `Acct:`;
- a player-index entry;
- DB inventory plus a nonempty `obj_save_header`;
- sheathed equipment;
- an `.objs` fallback file;
- script variables;
- a saved pet and pet inventory;
- a saved clone;
- eidolon descriptions;
- sent and received `player_mail`, including read/deleted state;
- quest/loot/crafting state;
- pubsub settings/subscriptions;
- level history where those tables are enabled;
- another offline player whose `Intr:` block contains the old name;
- an online character whose introduction list contains the old name;
- a clan membership/leadership record, house, vehicle, bound object, board
  read/visit record, and legacy mudmail entry tied to immutable/non-name IDs;
- live generated references: a trail, player corpse, and mission mob.

Before the rename, snapshot row identities, relationship keys, timestamps, and
raw payload bytes for object, sheath, pet, pet-object, and eidolon rows, plus a
hash of the fallback `.objs` file.

Use a rename whose first letter crosses a shard boundary, for example:

```text
Bartof -> Hartof
```

Exercise both:

```text
set file bartof name hartof
```

and the online-player rename form.

### Success assertions

After the command:

- the old name cannot be loaded;
- the new name can be loaded;
- account login lists the new name exactly once;
- selecting the account-menu entry enters the correct character;
- immutable player ID and account ID are unchanged;
- password/account authentication behavior is unchanged;
- the player file has `Name: Hartof` and the original `Acct:` value;
- the player index has one lowercase `hartof` entry and no `bartof` entry;
- `obj_save_header`, DB inventory IDs/order/timestamps, and every serialized
  object payload are unchanged;
- the fallback `.objs` hash is unchanged;
- sheath parent/slot relationships, pet IDs, pet-item joins, pet stats, and
  eidolon descriptions are unchanged and load under the new name;
- an ordinary pet's authored descriptions remain unchanged;
- a saved clone derives `Hartof` after load instead of restoring `Bartof`;
- sent and received mail, plus read/deleted state, remains accessible;
- mail, quest, loot, crafting, pubsub, and level-history state remains
  accessible;
- `level_30_characters`, when applicable, reflects the new name through its
  view definition without a direct update;
- introduction recognition still works under the selected design and no
  parsed `Intr:` entry retains `Bartof`;
- clan membership/leadership, the house, vehicle, bound object, board state,
  legacy mudmail, corpse recovery, and mission ownership still work via the
  unchanged numeric/non-name IDs;
- no current-state ownership row remains under `Bartof`;
- no new duplicate row was created;
- active descriptors on the same account see `Hartof` without reconnecting;
- pubsub operations use the new name; and
- restart/copyover retains every assertion.

### Failure-injection assertions

Inject failure at each stage:

- DB unavailable before start;
- player row missing;
- new DB name collision;
- destination file collision;
- auxiliary file rename failure;
- failure on the first/middle/final table update;
- failure during each mail-table update or its compensation path;
- failure while rewriting one of multiple introduction-bearing player files;
- player-file write failure;
- player-index write failure;
- SQL commit failure; and
- postcondition failure.

After every injected failure, assert the old character remains completely
usable through direct login and its account menu, and assert that no
new-name-owned row/file is left behind.

### Online-state tests

- Rename an online character.
- Keep a second character from the same account online in another descriptor.
- Keep a third descriptor sitting at the same account menu.
- Confirm all account objects refresh and no descriptor retains `old_name`.
- Confirm loaded introduction lists, clone names, trails, owned corpse display,
  and mission-mob generated owner text no longer retain `old_name`.
- Save/quit both characters and reconnect.

### Legacy/unlinked tests

- Rename a character with `account_id IS NULL`.
- Confirm no account link is invented.
- Confirm direct login, inventory, auxiliary files, and restart behavior still
  work.
- Confirm clan membership/leadership, houses, vehicles, bound items, board
  state, legacy filesystem mudmail, player corpses, and mission ownership
  remain attached by immutable/non-name IDs.

## Useful schema-audit query

Run this in each disposable dev/staging schema to find candidate name keys.
Review the results manually; do not turn this into a blind runtime update.

```sql
SELECT
    c.table_name,
    t.table_type,
    t.engine,
    c.column_name,
    c.column_key,
    c.data_type
FROM information_schema.columns AS c
JOIN information_schema.tables AS t
  ON t.table_schema = c.table_schema
 AND t.table_name = c.table_name
WHERE c.table_schema = DATABASE()
  AND (
      LOWER(c.column_name) IN (
          'name',
          'character_name',
          'player_name',
          'owner_name',
          'char_name',
          'sender_name',
          'owner',
          'sender',
          'receiver',
          'recipient',
          'author',
          'created_by',
          'loaded_by'
      )
      OR LOWER(c.column_name) LIKE '%character%name%'
      OR LOWER(c.column_name) LIKE '%player%name%'
      OR LOWER(c.column_name) LIKE '%owner%name%'
  )
ORDER BY c.table_name, c.ordinal_position;
```

Compare the result against the explicit rename map and classify every
candidate as:

- current ownership/state: migrate;
- historical display snapshot: intentionally keep or intentionally migrate;
- unrelated entity name: do not touch; or
- obsolete table: remove through a separate schema migration; and
- derived view: do not update, but verify its result.

Including table type and engine is mandatory: it is what exposes derived views
such as `level_30_characters` and nontransactional tables such as the deployed
mail tables. The earlier suffix-only query missed `player_eidolons.owner` and
`player_mail.sender`/`receiver`.

## Post-rename verification queries

Use escaped/bound parameters in tooling. The literals below are examples for
the original incident:

```sql
SELECT id, name, account_id, obj_save_header
FROM player_data
WHERE LOWER(name) IN (LOWER('Bartof'), LOWER('Hartof'));

SELECT a.id, a.name AS account_name, p.name AS character_name
FROM account_data AS a
JOIN player_data AS p ON p.account_id = a.id
WHERE p.name = 'Hartof';

SELECT name, COUNT(*)
FROM player_save_objs
WHERE LOWER(name) IN (LOWER('Bartof'), LOWER('Hartof'))
GROUP BY name;

SELECT owner, COUNT(*)
FROM player_eidolons
WHERE LOWER(owner) IN (LOWER('Bartof'), LOWER('Hartof'))
GROUP BY owner;

SELECT sender, receiver, COUNT(*)
FROM player_mail
WHERE LOWER(sender) IN (LOWER('Bartof'), LOWER('Hartof'))
   OR LOWER(receiver) IN (LOWER('Bartof'), LOWER('Hartof'))
GROUP BY sender, receiver;

SELECT owner_name, pet_data_id
FROM pet_data
WHERE LOWER(owner_name) IN (LOWER('Bartof'), LOWER('Hartof'));

SELECT owner_name, pet_idnum, COUNT(*)
FROM pet_save_objs
WHERE LOWER(owner_name) IN (LOWER('Bartof'), LOWER('Hartof'))
GROUP BY owner_name, pet_idnum;
```

The integration test should generate equivalent zero-stale-row checks for
every table in the active rename map.

Filesystem verification should assert:

```text
new .plr exists
old .plr does not exist
new optional auxiliary file exists iff the old one existed
old optional auxiliary file does not exist after success
player-file Name matches new name
player-file Acct matches the unchanged owning account
player-index entry matches new lowercase name
```

## Acceptance criteria

The work is complete only when all of the following are demonstrated in a
development/staging environment:

- One shared service handles online and offline character renames.
- No shell command is used to move player data.
- The canonical DB row and all supported name-keyed ownership records migrate
  transactionally.
- The active mail tables are transactional, or an equally tested
  nontransactional compensation design is in place.
- Player and index writes are checked and recoverable.
- Descriptorless saves retain `Acct:`.
- Auxiliary files follow the new name, including cross-shard paths.
- Object headers, row identities, serialized payloads, sheath relationships,
  pet identities/inventory, eidolon descriptions, and fallback-file bytes are
  proven unchanged apart from their owning name/path keys.
- Sent/received mail and mail read/deleted state work under the new name.
- Introduction references remain valid independent of the feature's current
  on/off setting.
- Account and pubsub caches refresh after commit.
- Generated live references (clone, trail, owned corpse, and mission mob) are
  refreshed without rewriting arbitrary user-authored text.
- Numeric/non-name-ID systems (clans, houses, vehicles, object bindings, board
  state, legacy mudmail, corpse recovery, and mission ownership) are proven
  unaffected.
- Failure injection proves compensation rather than partial rename.
- Account-menu selection and every persisted subsystem in the active rename
  map work immediately after rename.
- Restart/copyover retains the rename.
- Automated tests cover success, collisions, cross-shard moves, online state,
  unlinked legacy characters, and rollback.
- The implementation contains no incident-specific names, IDs, or row counts.

## Longer-term recommendation

Character names are mutable presentation data and should not be ownership
keys. The durable design is to key character-owned tables by one canonical
immutable character ID, with foreign keys and appropriate delete behavior.
Before that migration, reconcile the player-file/index `GET_IDNUM` identity
with any deployment's `player_data.id`; do not assume today's auto-increment
database row ID already has the same value or semantics.

A later schema migration should replace columns such as
`player_save_objs.name`, `pet_data.owner_name`, `player_eidolons.owner`,
`player_mail.sender`/`receiver`, and `pubsub_subscriptions.player_name` with a
canonical `player_id`. Introduction lists should store the same immutable ID.
Historical display names can remain as snapshots where useful, but lookups and
ownership should use the immutable ID.

That migration would make a character rename primarily:

- one update to `player_data.name`;
- a player-file/index rename;
- cache refresh; and
- no fan-out across unrelated tables.

Until that migration exists, the centralized rename service and explicit
rename map are required to prevent another partial rename.
