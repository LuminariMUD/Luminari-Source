-- Vessel System Phase 18: DurisMUD study step S1
-- (docs/ongoing-projects/vessels-ships.md). Mirrors the runtime DDL in
-- vessel_prototype_ensure_schema() (src/vessels/vessels_edit.c),
-- vessel_piracy_ensure_schema() (src/vessels/vessels_piracy.c), and
-- vessel_ownership_ensure_schema() (src/vessels/vessels_ownership.c), so it
-- also applies to a database that Phases 04, 06, and 07 have not reached.

-- Shipyard listing, and the level needed to take a hull out of port.
-- A min_level of 0 means the hull class minimum.
CREATE TABLE IF NOT EXISTS ship_prototypes (
  prototype_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(127) NOT NULL,
  vessel_class INT NOT NULL DEFAULT 2,
  max_speed INT NOT NULL DEFAULT 10,
  armor INT NOT NULL DEFAULT 10,
  for_sale TINYINT(1) NOT NULL DEFAULT 0,
  min_level INT NOT NULL DEFAULT 0,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

ALTER TABLE ship_prototypes
ADD COLUMN IF NOT EXISTS for_sale TINYINT(1) NOT NULL DEFAULT 0 AFTER armor,
ADD COLUMN IF NOT EXISTS min_level INT NOT NULL DEFAULT 0 AFTER for_sale;

-- Bounties decay from the last offense. Existing bounties start the clock
-- when this column is added.
CREATE TABLE IF NOT EXISTS vessel_bounties (
  player_name VARCHAR(64) PRIMARY KEY,
  bounty INT NOT NULL DEFAULT 0,
  marque_until INT NOT NULL DEFAULT 0,
  last_offense_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

ALTER TABLE vessel_bounties
ADD COLUMN IF NOT EXISTS last_offense_at TIMESTAMP NOT NULL
DEFAULT CURRENT_TIMESTAMP AFTER marque_until;

-- Crew are a one-time hire: clear wage debt left by the retired payroll. The
-- server still keeps the Phase 06 column.
ALTER TABLE ship_interiors
ADD COLUMN IF NOT EXISTS wages_owed INT NOT NULL DEFAULT 0;

UPDATE ship_interiors SET wages_owed = 0
WHERE wages_owed <> 0;
