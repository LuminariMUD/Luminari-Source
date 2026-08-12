-- Builder help for registry-backed special procedures.
--
-- The help system is database-first. This idempotent migration is the
-- reviewable source for the builder-facing SpecProc topic.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('spec-proc', 'SPECIAL PROCEDURES (SPECPROCS)

Special procedures provide coded behavior for rooms, mobiles, and objects.
In medit, oedit, or redit, choose Z) SpecProc to view procedures that are safe
for that prototype type. The menu explains each procedure\'s events and any
required flags or placement. Enter a menu number to select it or 0 to clear it,
then save normally. The selected name is stored in the world file and restored
at boot.

The menu lists canonical names only. Explicit aliases remain load-compatible,
but selecting an entry writes its canonical name. Selecting a procedure does
not add runtime prerequisites such as MOB_SPEC, ITEM_AUTOPROC, equipped,
carried, or combat state; review the entry description and configure those
requirements separately.

World loading preserves the exact authored request even when a name is unknown
or incompatible and installs no callback. Merely opening and saving OLC keeps
that request. Select a menu entry to replace it with a canonical name, or enter
0 to clear the authored procedure and omit it on the next zone save.

At boot, SPEC_BIND lines show ordered world, parser-hook, legacy-assignment,
shop, and quest contributions. SPEC_BIND_FINAL shows the authored request,
chosen callback, source, and collision count. In -s mode only sources that
actually run are reported; -s is not a global callback-disable switch.

Immortal staff can inspect the same recorded post-boot chain on a live server
with SPECBIND <mob|obj|room> <vnum>. The command reports the effective callback,
every ordered contribution and outcome, source locations, collision count,
saved shop or quest secondary, and the chosen source. SPECBIND is read-only and
does not change the prototype or rebuild history after a later OLC edit.
This history is diagnostic, not a persisted multiple-procedure dispatch chain.
Each prototype still stores zero or one authored procedure name; shop and quest
secondaries are reconstructed compatibility state.

A moving room cannot also have a named room SpecProc. Both features own the
same callback slot, so redit refuses that selection and zone saving or boot
rejects a room containing both forms of data.

Guild is the mobile-owned training procedure. RoL Guild Room provides the
same current training service for converted room-owned guild bindings and is
available only in redit. Pet Shop is room-owned, Postmaster is mobile-owned,
and Bank is available for compatible mobile and object prototypes.

Use trigedit when a script is sufficient. Ask a coder when the needed behavior
is not present in the SpecProc menu. Shops, quests, pet shops, and boards have
additional setup requirements beyond choosing a callback.

See also: SPECBIND, OLC, MEDIT, OEDIT, REDIT, TRIGEDIT, PETSHOP, BOARDS', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

-- Retire the two stale file-imported entries from keyword search. The help
-- query displays only its first database match, so duplicate mappings would
-- make the maintained result nondeterministic.
DELETE FROM help_keywords
WHERE UPPER(keyword) IN (
  '<SPEC>', 'SPEC', 'SPEC-PROC', 'SPECIAL-PROCEDURE', 'SPECIALS', 'SPECBIND', 'SPECPROC'
);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPEC');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPEC-PROC');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPECIAL-PROCEDURE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPECIALS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPECBIND');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('spec-proc', 'SPECPROC');

COMMIT;
