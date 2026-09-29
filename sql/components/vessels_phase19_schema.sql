-- Vessel System Phase 19: DurisMUD study step S3, the damage model
-- (docs/ongoing-projects/vessels-ships.md 3.3.10). Mirrors the runtime DDL in
-- vessel_prototype_ensure_schema() (src/vessels/vessels_edit.c) and
-- vessel_persistence_ensure_schema() (src/vessels/vessels_db.c), so it also
-- applies to a database that Phases 04 and 18 have not reached; it requires
-- Phases 09 and 10. Apply it before the vessel content packages, which write
-- S3-scale prototype armor.

-- Shipyard prototypes. armor_scale 0 marks armor on the pre-S3 scale.
CREATE TABLE IF NOT EXISTS ship_prototypes (
  prototype_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(127) NOT NULL,
  vessel_class INT NOT NULL DEFAULT 2,
  max_speed INT NOT NULL DEFAULT 10,
  armor INT NOT NULL DEFAULT 10,
  for_sale TINYINT(1) NOT NULL DEFAULT 0,
  min_level INT NOT NULL DEFAULT 0,
  armor_scale TINYINT(1) NOT NULL DEFAULT 1,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

-- Existing rows take 0 and are rescaled once, by the class beam armor over
-- the class's old default armor, to at most 229.
ALTER TABLE ship_prototypes
ADD COLUMN IF NOT EXISTS for_sale TINYINT(1) NOT NULL DEFAULT 0 AFTER armor,
ADD COLUMN IF NOT EXISTS min_level INT NOT NULL DEFAULT 0 AFTER for_sale,
ADD COLUMN IF NOT EXISTS armor_scale TINYINT(1) NOT NULL DEFAULT 0
AFTER min_level;

UPDATE ship_prototypes AS prototype
INNER JOIN (
  SELECT
    0 AS vessel_class,
    2 AS legacy_armor,
    3 AS beam_armor
  UNION ALL
  SELECT
    1,
    5,
    8
  UNION ALL
  SELECT
    2,
    20,
    66
  UNION ALL
  SELECT
    3,
    40,
    109
  UNION ALL
  SELECT
    4,
    15,
    63
  UNION ALL
  SELECT
    5,
    25,
    84
  UNION ALL
  SELECT
    6,
    20,
    110
  UNION ALL
  SELECT
    7,
    20,
    153
) AS scale
  ON
      scale.vessel_class
      = IF(prototype.vessel_class BETWEEN 0 AND 7, prototype.vessel_class, 2)
SET
  prototype.armor = LEAST(
    229,
    (
      GREATEST(0, prototype.armor) * scale.beam_armor
      + scale.legacy_armor DIV 2
    ) DIV scale.legacy_armor
  ),
  prototype.armor_scale = 1
WHERE prototype.armor_scale = 0;

ALTER TABLE ship_prototypes
ALTER COLUMN armor_scale SET DEFAULT 1;

-- Live hull snapshots. condition_model 0 marks a hull saved under the pre-S3
-- condition model; the server converts it once at load, keeping each arc's,
-- the sails', and the rudder's damage fraction, and writes 1 when it saves.
-- sink_ticks is a sinking hull's remaining sink timer (0 afloat), restored at
-- load so a restart does not restart the countdown.
ALTER TABLE ship_runtime_state
ADD COLUMN IF NOT EXISTS condition_model TINYINT UNSIGNED NOT NULL DEFAULT 0
AFTER maxslots,
ADD COLUMN IF NOT EXISTS sink_ticks SMALLINT UNSIGNED NOT NULL DEFAULT 0
AFTER condition_model;

-- Weapon damage: 1 or more disables a weapon until repaired, 100 destroys it.
ALTER TABLE ship_weapons
ADD COLUMN IF NOT EXISTS weapon_damage TINYINT UNSIGNED NOT NULL DEFAULT 0
AFTER reload_timer;
