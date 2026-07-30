-- Vessel System Phase 11 verification.

SELECT COUNT(*) AS required_tables_present
  FROM information_schema.TABLES
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'ship_room_template_triggers';

SELECT room_type, vessel_type, trigger_vnum
  FROM ship_room_template_triggers
 WHERE room_type = ''
    OR vessel_type < 0
    OR trigger_vnum <= 0;

SELECT room_type, vessel_type, COUNT(*) AS trigger_count
  FROM ship_room_template_triggers
 GROUP BY room_type, vessel_type
HAVING COUNT(*) > 8;
