-- Vessel System Phase 04 verification.
-- Expect: 1 row (table exists) and the listed columns.

SELECT COUNT(*) AS table_present
  FROM information_schema.TABLES
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'ship_prototypes';

SELECT COLUMN_NAME, DATA_TYPE
  FROM information_schema.COLUMNS
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'ship_prototypes'
 ORDER BY ORDINAL_POSITION;
