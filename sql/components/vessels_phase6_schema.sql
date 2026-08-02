-- Vessel System Phase 06: ownership, crew, upgrades, insurance.
-- Mirrors the auto-migration in src/vessels/vessels_ownership.c
-- (vessel_ownership_ensure_schema). Helm permits and hired crew reuse
-- ship_crew_roster rows: crew_role='captain' with npc_vnum=-1 for player
-- helm permits, npc_vnum <= -100 for hired crew positions (loyalty_rating
-- carries the quality tier).

ALTER TABLE ship_interiors
  ADD COLUMN IF NOT EXISTS owner VARCHAR(64) NOT NULL DEFAULT '',
  ADD COLUMN IF NOT EXISTS upgrades INT NOT NULL DEFAULT 0,
  ADD COLUMN IF NOT EXISTS insured_for INT NOT NULL DEFAULT 0,
  ADD COLUMN IF NOT EXISTS wages_owed INT NOT NULL DEFAULT 0;
