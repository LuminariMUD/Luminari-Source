-- Vessel System Phase 21 verification.

SELECT COUNT(*) AS phase21_columns_present
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME = 'ship_crew_roster'
  AND IS_NULLABLE = 'NO'
  AND COLUMN_NAME IN ('experience');
