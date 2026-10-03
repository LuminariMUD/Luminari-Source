-- Vessel System Phase 24 verification.

SELECT COUNT(*) AS phase24_columns_present
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND COLUMN_NAME = 'creator_id'
  AND IS_NULLABLE = 'NO'
  AND COLUMN_DEFAULT = '0'
  AND TABLE_NAME IN ('ship_waypoints', 'ship_routes');
