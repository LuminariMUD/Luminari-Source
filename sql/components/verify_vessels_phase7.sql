-- Vessel System Phase 07 verification.
-- Expect: 4 tables, 9 seeded commodities (or more if builders added any).

SELECT COUNT(*) AS tables_present
  FROM information_schema.TABLES
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME IN ('trade_commodities', 'port_commodities', 'freight_contracts', 'vessel_bounties');

SELECT COUNT(*) AS commodity_count FROM trade_commodities;

-- Supply must stay inside the band the pricing code clamps to (10..400)
SELECT COUNT(*) AS out_of_band_supply
  FROM port_commodities
 WHERE supply < 10 OR supply > 400;

-- Contract board health: open offers, jobs in flight, completed runs
SELECT status, COUNT(*) AS contracts FROM freight_contracts GROUP BY status;

-- Outstanding bounties and active privateer commissions
SELECT COUNT(*) AS wanted_pirates FROM vessel_bounties WHERE bounty >= 500;
SELECT COUNT(*) AS active_marques FROM vessel_bounties
 WHERE marque_until > UNIX_TIMESTAMP();

-- Busiest ports by tracked goods
SELECT port_vnum, COUNT(*) AS tracked_goods
  FROM port_commodities
 GROUP BY port_vnum
 ORDER BY tracked_goods DESC
 LIMIT 10;
