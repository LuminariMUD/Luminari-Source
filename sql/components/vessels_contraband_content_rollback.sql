-- Vessels study S7 contraband rollback: removes the three goods, their stock,
-- and any lot of them in a hold. Run before the Phase 23 rollback.

DELETE manifest
FROM ship_cargo_manifest AS manifest
INNER JOIN trade_commodities AS commodity
  ON manifest.cargo_room = 0 AND manifest.item_vnum = commodity.commodity_id
WHERE commodity.name IN ('forbidden tomes', 'rare poisons', 'dragon eggs');

DELETE stock
FROM port_commodities AS stock
INNER JOIN trade_commodities AS commodity
  ON stock.commodity_id = commodity.commodity_id
WHERE commodity.name IN ('forbidden tomes', 'rare poisons', 'dragon eggs');

DELETE FROM trade_commodities
WHERE name IN ('forbidden tomes', 'rare poisons', 'dragon eggs');
