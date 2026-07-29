-- Vessel System Phase 09 verification.

SELECT COUNT(*) AS required_tables_present
  FROM information_schema.TABLES
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME IN ('ship_runtime_state', 'ship_schedules');

SELECT COUNT(*) AS runtime_columns_present
  FROM information_schema.COLUMNS
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'ship_runtime_state'
   AND COLUMN_NAME IN (
     'ship_id', 'location_vnum', 'x', 'y', 'z',
     'farmor', 'finternal', 'last_attacker', 'room_types', 'slot_data',
     'autopilot_state', 'current_route_id', 'current_waypoint_index'
   );

-- Dynamic interiors without a runtime snapshot cannot be reconstructed.
SELECT si.ship_id, si.vessel_name
  FROM ship_interiors si
  LEFT JOIN ship_runtime_state runtime ON runtime.ship_id = si.ship_id
 WHERE si.ship_id >= 2
   AND runtime.ship_id IS NULL;

-- Runtime rows must always have their parent interior row.
SELECT runtime.ship_id
  FROM ship_runtime_state runtime
  LEFT JOIN ship_interiors si ON si.ship_id = runtime.ship_id
 WHERE si.ship_id IS NULL;

-- These values would be rejected or clamped by the runtime loader.
SELECT ship_id, autopilot_state, current_waypoint_index
  FROM ship_runtime_state
 WHERE autopilot_state < 0
    OR autopilot_state > 4
    OR current_waypoint_index < 0;

SELECT COUNT(*) AS runtime_instances FROM ship_runtime_state;
SELECT COUNT(*) AS vessel_schedules FROM ship_schedules;
