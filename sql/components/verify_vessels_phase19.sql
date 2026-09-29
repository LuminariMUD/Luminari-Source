-- Vessel System Phase 19 verification.

SELECT COUNT(*) AS phase19_columns_present
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND IS_NULLABLE = 'NO'
  AND (
    (TABLE_NAME = 'ship_prototypes' AND COLUMN_NAME = 'armor_scale')
    OR (TABLE_NAME = 'ship_runtime_state' AND COLUMN_NAME = 'condition_model')
    OR (TABLE_NAME = 'ship_runtime_state' AND COLUMN_NAME = 'sink_ticks')
    OR (TABLE_NAME = 'ship_weapons' AND COLUMN_NAME = 'weapon_damage')
  );

SELECT COLUMN_DEFAULT AS new_prototype_armor_scale
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME = 'ship_prototypes'
  AND COLUMN_NAME = 'armor_scale';

SELECT COUNT(*) AS prototypes_not_rescaled
FROM ship_prototypes
WHERE armor_scale <> 1;

SELECT
  prototype_id,
  name,
  vessel_class,
  armor
FROM ship_prototypes
WHERE armor < 0 OR armor > 229;

SELECT COUNT(*) AS hulls_awaiting_conversion
FROM ship_runtime_state
WHERE condition_model = 0;

SELECT
  ship_id,
  slot_index,
  weapon_damage
FROM ship_weapons
WHERE weapon_damage > 100;
