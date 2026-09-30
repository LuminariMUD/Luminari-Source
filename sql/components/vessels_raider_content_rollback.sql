-- Roll back the NPC raider content.
-- A raider prototype with a persistent runtime keeps its prototype row and
-- its tier rows: the next restart retires her through those tier rows. Run
-- this rollback again after that restart to remove what it kept.

DELETE FROM vessel_raider_tiers
WHERE
  prototype_id IN (
    SELECT prototype_id FROM ship_prototypes
    WHERE name IN (
      'Corsair Clipper',
      'Corsair Ketch',
      'Corsair Caravel',
      'Corsair Corvette',
      'Corsair Destroyer',
      'Corsair Frigate'
    )
  )
  AND NOT EXISTS (
    SELECT 1
    FROM ship_runtime_state AS runtime
    WHERE runtime.prototype_id = vessel_raider_tiers.prototype_id
  );

DELETE FROM ship_prototypes
WHERE
  name IN (
    'Corsair Clipper',
    'Corsair Ketch',
    'Corsair Caravel',
    'Corsair Corvette',
    'Corsair Destroyer',
    'Corsair Frigate'
  )
  AND NOT EXISTS (
    SELECT 1
    FROM ship_runtime_state AS runtime
    WHERE runtime.prototype_id = ship_prototypes.prototype_id
  );
