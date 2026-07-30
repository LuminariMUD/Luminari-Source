-- Vessel System Phase 12 verification.

SELECT COUNT(*) AS passenger_fare_column_present
  FROM information_schema.COLUMNS
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'ship_schedules'
   AND COLUMN_NAME = 'passenger_fare';

SELECT ship_id, passenger_fare
  FROM ship_schedules
 WHERE passenger_fare < 0
    OR passenger_fare > 100000;

SELECT COUNT(*) AS fare_collecting_schedules
  FROM ship_schedules
 WHERE passenger_fare > 0;
