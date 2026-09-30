-- Vessel System Phase 22 verification.

SELECT COUNT(*) AS phase22_columns_present
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME = 'vessel_raider_tiers'
  AND IS_NULLABLE = 'NO'
  AND COLUMN_NAME IN ('tier', 'prototype_id');

SELECT
  tier,
  prototype_id
FROM vessel_raider_tiers
WHERE
  tier NOT BETWEEN 0 AND 3
  OR prototype_id NOT IN (SELECT prototype_id FROM ship_prototypes);
