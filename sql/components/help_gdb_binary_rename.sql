-- Point the GDB help entry at the renamed server executable.
--
-- The help system is database-first, so the file copy in
-- lib/text/help/help.hlp is mirrored here. This component is idempotent: it
-- rewrites only the obsolete path and leaves the rest of the entry alone, so
-- rerunning it after the rename is a no-op.
--
-- Companion change: scripts/deployment/install_versioned_binary.sh installs
-- bin/luminari and keeps bin/circle as a Phase A compatibility symlink.

START TRANSACTION;

UPDATE help_entries
   SET entry = REPLACE(entry, 'gdb bin/circle', 'gdb bin/luminari')
 WHERE tag = 'GDB'
   AND entry LIKE '%gdb bin/circle%';

COMMIT;

-- Verification (expect exactly one row, and zero obsolete references):
--   SELECT COUNT(*) FROM help_entries WHERE tag = 'GDB';
--   SELECT COUNT(*) FROM help_entries
--    WHERE tag = 'GDB' AND entry LIKE '%bin/circle%';
