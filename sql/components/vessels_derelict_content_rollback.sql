-- Roll back Blackwake derelict content definitions.
-- A prototype with a persistent runtime is deliberately retained.

DELETE FROM ship_room_template_triggers
 WHERE vessel_type = 0
   AND (room_type, trigger_vnum) IN (
     ('bridge', 70010),
     ('quarters_crew', 70011),
     ('cargo_main', 70012)
   );

DELETE FROM ship_prototypes
 WHERE name = 'Blackwake Derelict'
   AND NOT EXISTS (
     SELECT 1
       FROM ship_runtime_state AS runtime
      WHERE runtime.prototype_id = ship_prototypes.prototype_id
   );
