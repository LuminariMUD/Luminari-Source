-- Read-only verification for the Blackwake derelict content package.

SELECT 'blackwake_derelict_prototype' AS check_name,
       COUNT(*) AS actual, 1 AS expected
  FROM ship_prototypes
 WHERE name = 'Blackwake Derelict'
   AND vessel_class = 2
   AND max_speed = 6
   AND armor = 15;

SELECT 'blackwake_derelict_triggers' AS check_name,
       COUNT(*) AS actual, 3 AS expected
  FROM ship_room_template_triggers
 WHERE vessel_type = 0
   AND (room_type, trigger_vnum) IN (
     ('bridge', 70010),
     ('quarters_crew', 70011),
     ('cargo_main', 70012)
   );

SELECT 'blackwake_derelict_runtime' AS check_name,
       COUNT(*) AS actual, '0_or_1' AS expected
  FROM ship_runtime_state AS runtime
  JOIN ship_interiors AS interior ON interior.ship_id = runtime.ship_id
  JOIN ship_prototypes AS prototype
    ON prototype.prototype_id = runtime.prototype_id
 WHERE prototype.name = 'Blackwake Derelict'
   AND interior.vessel_name = 'Blackwake Derelict'
   AND interior.owner = '';
