-- Vessel System Phase 19 rollback.
-- Returns prototype armor to the pre-S3 scale (rounded) and removes the
-- armor-scale flag, the hull condition model, the sink timer, and weapon
-- damage. Live hulls keep their S3 condition values; weapon damage and sink
-- timers are not restored.

-- A repeated rollback finds no flag; the placeholder flags no row as S3.
ALTER TABLE ship_prototypes
ADD COLUMN IF NOT EXISTS armor_scale TINYINT(1) NOT NULL DEFAULT 0;

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
  prototype.armor
  = (
    GREATEST(0, prototype.armor) * scale.legacy_armor
    + scale.beam_armor DIV 2
  ) DIV scale.beam_armor
WHERE prototype.armor_scale = 1;

ALTER TABLE ship_prototypes
DROP COLUMN IF EXISTS armor_scale;

ALTER TABLE ship_runtime_state
DROP COLUMN IF EXISTS condition_model,
DROP COLUMN IF EXISTS sink_ticks;

ALTER TABLE ship_weapons
DROP COLUMN IF EXISTS weapon_damage;
