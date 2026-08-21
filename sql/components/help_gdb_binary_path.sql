-- Pin the GDB help entry to the canonical server executable.
--
-- The help system is database-first, so the file copy in
-- lib/text/help/help.hlp is mirrored here. This component states the required
-- end state rather than a one-way migration: any bin/ executable path in the
-- GDB entry is normalized to bin/luminari, so rerunning it is a no-op once the
-- entry is canonical.

DROP PROCEDURE IF EXISTS assert_gdb_help_binary_path;

DELIMITER //
CREATE PROCEDURE assert_gdb_help_binary_path(IN require_canonical BOOLEAN)
BEGIN
  DECLARE canonical_count INT DEFAULT 0;
  DECLARE gdb_count INT DEFAULT 0;

  SELECT COUNT(*),
         COALESCE(SUM(entry LIKE '%gdb bin/luminari%'), 0)
    INTO gdb_count, canonical_count
    FROM help_entries
   WHERE tag = 'GDB';

  IF gdb_count <> 1 THEN
    SIGNAL SQLSTATE '45000'
      SET MESSAGE_TEXT = 'Expected exactly one GDB help entry';
  END IF;

  IF require_canonical AND canonical_count <> 1 THEN
    SIGNAL SQLSTATE '45000'
      SET MESSAGE_TEXT = 'GDB help entry does not use gdb bin/luminari';
  END IF;
END//
DELIMITER ;

START TRANSACTION;

CALL assert_gdb_help_binary_path(FALSE);

UPDATE help_entries
   SET entry = REGEXP_REPLACE(entry, 'gdb bin/[[:alnum:]_.-]+', 'gdb bin/luminari')
 WHERE tag = 'GDB'
   AND entry REGEXP 'gdb bin/[[:alnum:]_.-]+'
   AND entry NOT LIKE '%gdb bin/luminari%';

CALL assert_gdb_help_binary_path(TRUE);

COMMIT;

DROP PROCEDURE assert_gdb_help_binary_path;
