-- Pin the GDB help entry to the canonical server executable.
--
-- The help system is database-first, so the file copy in
-- lib/text/help/help.hlp is mirrored here. This component states the required
-- end state rather than a one-way migration: any bin/ executable path in the
-- GDB entry is normalized to bin/luminari, so rerunning it is a no-op once the
-- entry is canonical.

START TRANSACTION;

UPDATE help_entries
   SET entry = REGEXP_REPLACE(entry, 'gdb bin/[[:alnum:]_.-]+', 'gdb bin/luminari')
 WHERE tag = 'GDB'
   AND entry REGEXP 'gdb bin/[[:alnum:]_.-]+'
   AND entry NOT LIKE '%gdb bin/luminari%';

COMMIT;

-- Verification (expect exactly one row, and one canonical reference):
--   SELECT COUNT(*) FROM help_entries WHERE tag = 'GDB';
--   SELECT COUNT(*) FROM help_entries
--    WHERE tag = 'GDB' AND entry LIKE '%gdb bin/luminari%';
