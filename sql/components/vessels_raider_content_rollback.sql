-- Roll back the NPC raider content.
-- Raider hulls still at sea retire at the next restart; a prototype with a
-- persistent runtime is retained until then.

DELETE FROM vessel_raider_tiers
WHERE prototype_id IN (
  SELECT prototype_id FROM ship_prototypes
  WHERE name IN (
    'Corsair Clipper',
    'Corsair Ketch',
    'Corsair Caravel',
    'Corsair Corvette',
    'Corsair Destroyer',
    'Corsair Frigate'
  )
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
