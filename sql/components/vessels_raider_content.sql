-- Vessel System 3.0: NPC raider content (DurisMUD study step S6,
-- docs/adr/0003-durismud-naval-model.md, 3.3.8). Requires Phase 22.
--
-- Six raider hulls after Duris's clipper, ketch, caravel, corvette,
-- destroyer, and frigate (speeds times 0.3, beam armor), none for sale, and
-- the tiers each sails. The raiders' captains, crews, chests, and keys are
-- zone 700 mobiles 70020-70027 and objects 70020-70021
-- (lib/world/vessel_raiders).

START TRANSACTION;

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor, armor_scale)
SELECT
  'Corsair Clipper',
  2,
  26,
  36,
  1
WHERE NOT EXISTS (
  SELECT 1 FROM ship_prototypes
  WHERE name = 'Corsair Clipper'
);
UPDATE ship_prototypes
SET
  vessel_class = 2,
  max_speed = 26,
  armor = 36,
  armor_scale = 1,
  for_sale = 0,
  min_level = 0
WHERE name = 'Corsair Clipper';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor, armor_scale)
SELECT
  'Corsair Ketch',
  2,
  23,
  50,
  1
WHERE NOT EXISTS (
  SELECT 1 FROM ship_prototypes
  WHERE name = 'Corsair Ketch'
);
UPDATE ship_prototypes
SET
  vessel_class = 2,
  max_speed = 23,
  armor = 50,
  armor_scale = 1,
  for_sale = 0,
  min_level = 0
WHERE name = 'Corsair Ketch';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor, armor_scale)
SELECT
  'Corsair Caravel',
  2,
  20,
  66,
  1
WHERE NOT EXISTS (
  SELECT 1 FROM ship_prototypes
  WHERE name = 'Corsair Caravel'
);
UPDATE ship_prototypes
SET
  vessel_class = 2,
  max_speed = 20,
  armor = 66,
  armor_scale = 1,
  for_sale = 0,
  min_level = 0
WHERE name = 'Corsair Caravel';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor, armor_scale)
SELECT
  'Corsair Corvette',
  3,
  22,
  63,
  1
WHERE NOT EXISTS (
  SELECT 1 FROM ship_prototypes
  WHERE name = 'Corsair Corvette'
);
UPDATE ship_prototypes
SET
  vessel_class = 3,
  max_speed = 22,
  armor = 63,
  armor_scale = 1,
  for_sale = 0,
  min_level = 0
WHERE name = 'Corsair Corvette';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor, armor_scale)
SELECT
  'Corsair Destroyer',
  3,
  19,
  84,
  1
WHERE NOT EXISTS (
  SELECT 1 FROM ship_prototypes
  WHERE name = 'Corsair Destroyer'
);
UPDATE ship_prototypes
SET
  vessel_class = 3,
  max_speed = 19,
  armor = 84,
  armor_scale = 1,
  for_sale = 0,
  min_level = 0
WHERE name = 'Corsair Destroyer';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor, armor_scale)
SELECT
  'Corsair Frigate',
  3,
  17,
  109,
  1
WHERE NOT EXISTS (
  SELECT 1 FROM ship_prototypes
  WHERE name = 'Corsair Frigate'
);
UPDATE ship_prototypes
SET
  vessel_class = 3,
  max_speed = 17,
  armor = 109,
  armor_scale = 1,
  for_sale = 0,
  min_level = 0
WHERE name = 'Corsair Frigate';

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

INSERT INTO vessel_raider_tiers (tier, prototype_id)
SELECT
  raider.tier,
  prototype.prototype_id
FROM ship_prototypes AS prototype
INNER JOIN (
  SELECT
    0 AS tier,
    'Corsair Clipper' AS name
  UNION ALL
  SELECT
    0,
    'Corsair Ketch'
  UNION ALL
  SELECT
    0,
    'Corsair Caravel'
  UNION ALL
  SELECT
    1,
    'Corsair Ketch'
  UNION ALL
  SELECT
    1,
    'Corsair Caravel'
  UNION ALL
  SELECT
    1,
    'Corsair Corvette'
  UNION ALL
  SELECT
    2,
    'Corsair Corvette'
  UNION ALL
  SELECT
    2,
    'Corsair Destroyer'
  UNION ALL
  SELECT
    3,
    'Corsair Destroyer'
  UNION ALL
  SELECT
    3,
    'Corsair Frigate'
) AS raider ON prototype.name = raider.name;

COMMIT;
