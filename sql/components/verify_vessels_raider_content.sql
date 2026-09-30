-- Read-only verification for the NPC raider content: six raider
-- prototypes, none for sale, sailing ten tier rows.

SELECT
  prototype.prototype_id,
  prototype.name,
  prototype.vessel_class,
  prototype.max_speed,
  prototype.armor,
  prototype.for_sale,
  GROUP_CONCAT(raider.tier ORDER BY raider.tier) AS tiers
FROM ship_prototypes AS prototype
LEFT JOIN vessel_raider_tiers AS raider
  ON prototype.prototype_id = raider.prototype_id
WHERE
  prototype.name IN (
    'Corsair Clipper',
    'Corsair Ketch',
    'Corsair Caravel',
    'Corsair Corvette',
    'Corsair Destroyer',
    'Corsair Frigate'
  )
GROUP BY
  prototype.prototype_id,
  prototype.name,
  prototype.vessel_class,
  prototype.max_speed,
  prototype.armor,
  prototype.for_sale
ORDER BY prototype.prototype_id;

SELECT COUNT(*) AS raider_tier_rows
FROM vessel_raider_tiers;
