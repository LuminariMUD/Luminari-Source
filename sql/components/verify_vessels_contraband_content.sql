-- Vessels study S7 contraband verification: three contraband goods, each
-- stocked at its sea port.

SELECT COUNT(*) AS contraband_goods
FROM trade_commodities
WHERE
  (name = 'forbidden tomes' AND contraband_renown = 150)
  OR (name = 'rare poisons' AND contraband_renown = 200)
  OR (name = 'dragon eggs' AND contraband_renown = 250);

SELECT COUNT(*) AS contraband_stock
FROM port_commodities AS stock
INNER JOIN trade_commodities AS commodity
  ON stock.commodity_id = commodity.commodity_id
WHERE
  (stock.port_vnum = 1000337 AND commodity.name = 'forbidden tomes')
  OR (stock.port_vnum = 1000351 AND commodity.name = 'rare poisons')
  OR (stock.port_vnum = 1000278 AND commodity.name = 'dragon eggs');
