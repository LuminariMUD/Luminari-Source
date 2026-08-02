-- Vessel System Phase 08: encounter tables keyed to wilderness regions.
-- Mirrors the auto-creation in src/vessels/vessels_hazards.c
-- (vessel_hazard_ensure_schema).
--
-- IMPORTANT: region_vnum must reference a wilderness region of type
-- REGION_ENCOUNTER (region_type = 2) authored with the existing region
-- tooling. The vessel system deliberately owns no geography of its own -
-- see docs/product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md, "Wilderness Contract".
--
-- Column notes:
--   mob_vnum     - creature to spawn (0 = message-only atmosphere)
--   min/max_depth- depth band in wilderness elevation units below the
--                  waterline (0/0 = any depth)
--   vessel_class - restrict to one hull class, or -1 for all
--                  (0=Raft 1=Boat 2=Ship 3=Warship 4=Airship
--                   5=Submarine 6=Transport 7=Magical)
--   chance       - percent chance per encounter check

CREATE TABLE IF NOT EXISTS vessel_encounters (
  encounter_id INT AUTO_INCREMENT PRIMARY KEY,
  region_vnum INT NOT NULL,
  name VARCHAR(127) NOT NULL,
  mob_vnum INT NOT NULL DEFAULT 0,
  min_depth INT NOT NULL DEFAULT 0,
  max_depth INT NOT NULL DEFAULT 0,
  vessel_class INT NOT NULL DEFAULT -1,
  chance INT NOT NULL DEFAULT 10,
  warn_message VARCHAR(255) NOT NULL DEFAULT '',
  arrive_message VARCHAR(255) NOT NULL DEFAULT '',
  INDEX idx_region (region_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Example rows (commented out: substitute your own REGION_ENCOUNTER vnums
-- and mob vnums before enabling).
--
-- INSERT INTO vessel_encounters
--   (region_vnum, name, mob_vnum, min_depth, max_depth, vessel_class,
--    chance, warn_message, arrive_message)
-- VALUES
--   (1001, 'a sea serpent', 5001, 40, 0, -1, 15,
--    'The lookout cries out - something long and dark moves below!',
--    'A SEA SERPENT breaches alongside, jaws wide!'),
--   (1001, 'a floating derelict', 0, 0, 0, -1, 20,
--    'The lookout spots wreckage ahead.',
--    'A dismasted derelict drifts past, silent and empty.'),
--   (1002, 'a deep trench leviathan', 5002, 120, 0, 5, 25,
--    'Sonar returns something vast in the trench below.',
--    'Something immense moves in the dark water beyond the hull.'),
--   (1003, 'a roc', 5003, 0, 0, 4, 20,
--    'A shadow crosses the sun.',
--    'A ROC stoops on the airship, talons out!');
