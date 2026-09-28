-- Vessel System Phase 18: DurisMUD study step S1
-- (docs/ongoing-projects/vessels-ships.md). Mirrors the runtime DDL in
-- vessel_prototype_ensure_schema() (src/vessels/vessels_edit.c) and
-- vessel_piracy_ensure_schema() (src/vessels/vessels_piracy.c).

-- Shipyard listing, and the level needed to take a hull out of port.
-- A min_level of 0 means the hull class minimum.
ALTER TABLE ship_prototypes
ADD COLUMN IF NOT EXISTS for_sale TINYINT(1) NOT NULL DEFAULT 0 AFTER armor,
ADD COLUMN IF NOT EXISTS min_level INT NOT NULL DEFAULT 0 AFTER for_sale;

-- Bounties decay from the last offense. Existing bounties start the clock
-- when this column is added.
ALTER TABLE vessel_bounties
ADD COLUMN IF NOT EXISTS last_offense_at TIMESTAMP NOT NULL
DEFAULT CURRENT_TIMESTAMP AFTER marque_until;

-- Crew are a one-time hire: clear wage debt left by the retired payroll.
UPDATE ship_interiors SET wages_owed = 0
WHERE wages_owed <> 0;
