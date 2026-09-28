-- Vessel System Phase 18 verification.

SELECT COUNT(*) AS prototype_sale_level_columns_present
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME = 'ship_prototypes'
  AND COLUMN_NAME IN ('for_sale', 'min_level')
  AND IS_NULLABLE = 'NO';

SELECT
  prototype_id,
  name,
  for_sale,
  min_level
FROM ship_prototypes
WHERE for_sale = 1
ORDER BY prototype_id;

SELECT
  prototype_id,
  name,
  min_level
FROM ship_prototypes
WHERE min_level < 0 OR min_level > 30;

SELECT COUNT(*) AS bounty_decay_column_present
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME = 'vessel_bounties'
  AND COLUMN_NAME = 'last_offense_at'
  AND IS_NULLABLE = 'NO';

SELECT COUNT(*) AS hulls_with_wage_debt
FROM ship_interiors
WHERE wages_owed <> 0;
