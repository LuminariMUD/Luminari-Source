-- Vessel System Phase 20 verification.

SELECT COUNT(*) AS phase20_columns_present
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME = 'ship_weapons'
  AND IS_NULLABLE = 'NO'
  AND COLUMN_NAME IN ('catalog_id', 'ammo');

SELECT COUNT(*) AS weapons_awaiting_conversion
FROM ship_weapons
WHERE slot_type = 1 AND catalog_id = 0;

SELECT
  ship_id,
  slot_index,
  slot_type,
  catalog_id
FROM ship_weapons
WHERE
  slot_type NOT IN (1, 2)
  OR (slot_type = 1 AND catalog_id > 12)
  OR (slot_type = 2 AND catalog_id NOT IN (1, 2));
