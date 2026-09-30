-- Vessels study S7 contraband (docs/ongoing-projects/vessels-ships.md,
-- Phase 7; study 3.3.9): three contraband goods, each stocked at one sea port.
-- Needs Phase 23. A port without a stock row for a good will not sell it and
-- pays its scarce price for it, and a lawful one's customs seize it. Builders
-- may stock the goods at other ports with more rows.

INSERT INTO trade_commodities
(name, base_price, unit_weight, contraband_renown)
VALUES
('forbidden tomes', 190, 4, 150),
('rare poisons', 210, 1, 200),
('dragon eggs', 310, 10, 250)
ON DUPLICATE KEY UPDATE
base_price = VALUES (base_price),
unit_weight = VALUES (unit_weight),
contraband_renown = VALUES (contraband_renown);

-- Selerish Slateharbor, Southwest Quechian, and Koorvik sea ports.
INSERT IGNORE INTO port_commodities (port_vnum, commodity_id, supply)
SELECT
  stock.port_vnum,
  commodity.commodity_id,
  100
FROM (
  SELECT
    1000337 AS port_vnum,
    'forbidden tomes' AS name
  UNION ALL
  SELECT
    1000351,
    'rare poisons'
  UNION ALL
  SELECT
    1000278,
    'dragon eggs'
) AS stock
INNER JOIN trade_commodities AS commodity ON commodity.name = stock.name;
