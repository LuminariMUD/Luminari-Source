-- Vessel System Phase 23 verification.

SELECT COUNT(*) AS phase23_columns_present
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND IS_NULLABLE = 'NO'
  AND (
    (TABLE_NAME = 'ship_runtime_state' AND COLUMN_NAME = 'renown')
    OR (TABLE_NAME = 'trade_commodities' AND COLUMN_NAME = 'contraband_renown')
  );

SELECT
  ship_id,
  renown
FROM ship_runtime_state
WHERE renown < 0;
