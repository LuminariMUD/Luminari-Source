-- Vessel System 3.0: Blackwake data- and DG-driven derelict content.
--
-- The provisioner rejects name, object VNUM, and trigger VNUM collisions
-- before applying this idempotent package. The generated bridge, quarters,
-- and cargo rooms receive guarded DG triggers on the next server boot.

SET @blackwake_derelict_name = 'Blackwake Derelict';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT @blackwake_derelict_name, 2, 6, 15
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = @blackwake_derelict_name
 );

UPDATE ship_prototypes
   SET vessel_class = 2,
       max_speed = 6,
       armor = 15
 WHERE name = @blackwake_derelict_name;

INSERT IGNORE INTO ship_room_template_triggers
  (room_type, vessel_type, trigger_vnum)
VALUES
  ('bridge', 0, 70010),
  ('quarters_crew', 0, 70011),
  ('cargo_main', 0, 70012);
