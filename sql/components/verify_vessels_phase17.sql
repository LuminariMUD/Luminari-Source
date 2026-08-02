-- Vessel System Phase 17 verification.

SELECT COUNT(*) AS vessel_customization_columns_present
  FROM information_schema.COLUMNS
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'ship_interiors'
   AND COLUMN_NAME IN ('figurehead', 'paint_scheme')
   AND DATA_TYPE = 'varchar'
   AND CHARACTER_MAXIMUM_LENGTH = 80
   AND IS_NULLABLE = 'NO'
   AND COLUMN_DEFAULT = '';

SELECT ship_id, figurehead, paint_scheme
  FROM ship_interiors
 WHERE CHAR_LENGTH(figurehead) > 80
    OR CHAR_LENGTH(paint_scheme) > 80;
