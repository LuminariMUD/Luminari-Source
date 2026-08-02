-- Vessel System Phase 07: bulk cargo and port trading.
-- Mirrors the auto-creation in src/vessels/vessels_trade.c
-- (vessel_trade_ensure_schema), which also seeds the default commodity set
-- when trade_commodities is empty. Bulk cargo lots reuse
-- ship_cargo_manifest rows with cargo_room = 0 and item_vnum = commodity_id.

CREATE TABLE IF NOT EXISTS trade_commodities (
  commodity_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(63) NOT NULL UNIQUE,
  base_price INT NOT NULL DEFAULT 10,
  unit_weight INT NOT NULL DEFAULT 10
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS port_commodities (
  port_vnum INT NOT NULL,
  commodity_id INT NOT NULL,
  supply INT NOT NULL DEFAULT 100,
  PRIMARY KEY (port_vnum, commodity_id),
  INDEX idx_port (port_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS freight_contracts (
  contract_id INT AUTO_INCREMENT PRIMARY KEY,
  origin_vnum INT NOT NULL,
  destination_vnum INT NOT NULL,
  commodity_id INT NOT NULL,
  quantity INT NOT NULL,
  payout INT NOT NULL,
  status INT NOT NULL DEFAULT 0,
  taken_by VARCHAR(64) NOT NULL DEFAULT '',
  offered_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_origin (origin_vnum),
  INDEX idx_status (status),
  INDEX idx_taken (taken_by)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS vessel_bounties (
  player_name VARCHAR(64) PRIMARY KEY,
  bounty INT NOT NULL DEFAULT 0,
  marque_until INT NOT NULL DEFAULT 0,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Default goods (skip if trade_commodities already has rows)
INSERT IGNORE INTO trade_commodities (name, base_price, unit_weight) VALUES
  ('grain', 8, 20),
  ('salt', 14, 15),
  ('timber', 6, 40),
  ('iron', 30, 50),
  ('cloth', 25, 8),
  ('wine', 35, 25),
  ('spice', 90, 4),
  ('silk', 120, 3),
  ('gemstones', 250, 2);
