-- Vessel System Phase 23: DurisMUD study step S7, rewards and economy
-- (docs/adr/0003-durismud-naval-model.md). Mirrors the runtime DDL
-- in vessel_persistence_ensure_schema() (src/vessels/vessels_db.c) and
-- vessel_trade_ensure_schema() (src/vessels/vessels_trade.c), so it also
-- applies to a database that Phase 07 has not reached.

-- A hull's renown, won sinking other players' hulls. Hulls saved before S7
-- read 0.
ALTER TABLE ship_runtime_state
ADD COLUMN IF NOT EXISTS renown INT NOT NULL DEFAULT 0;

-- The Phase 07 commodity table.
CREATE TABLE IF NOT EXISTS trade_commodities (
  commodity_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(63) NOT NULL UNIQUE,
  base_price INT NOT NULL DEFAULT 10,
  unit_weight INT NOT NULL DEFAULT 10
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

-- Above 0, a commodity is contraband and this is the renown a hull needs to
-- buy it. Only a port with a port_commodities row for it stocks it, and only
-- content creates those rows (vessels_contraband_content.sql).
ALTER TABLE trade_commodities
ADD COLUMN IF NOT EXISTS contraband_renown INT NOT NULL DEFAULT 0;
