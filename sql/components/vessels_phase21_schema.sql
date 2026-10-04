-- Vessel System Phase 21: DurisMUD study step S5, crew, repair and loss
-- (docs/adr/0003-durismud-naval-model.md, 3.3.5-3.3.7 and 3.3.10). Mirrors the
-- runtime DDL in vessel_persistence_ensure_schema() (src/vessels/vessels_db.c)
-- and, for the premium refund, vessel_ownership_ensure_schema()
-- (src/vessels/vessels_ownership.c), so it also applies to a database that
-- Phase 06 has not reached.

-- Each hired crew position's experience, in Duris skill points. Crew hired
-- before S5 read 0 and start at the floor of their tier.
ALTER TABLE ship_crew_roster
ADD COLUMN IF NOT EXISTS experience DOUBLE NOT NULL DEFAULT 0
AFTER loyalty_rating;

-- A stowed hull is out of the world: in the wreck registry, or on her way to
-- the shipyard that summoned her until summon_due (Unix time; 0 = not
-- summoned). wreck_hull marks a hull rebuilt from a lost one, which carries
-- no insurance.
ALTER TABLE ship_runtime_state
ADD COLUMN IF NOT EXISTS stowed TINYINT UNSIGNED NOT NULL DEFAULT 0
AFTER sink_ticks,
ADD COLUMN IF NOT EXISTS wreck_hull TINYINT UNSIGNED NOT NULL DEFAULT 0
AFTER stowed,
ADD COLUMN IF NOT EXISTS summon_due BIGINT NOT NULL DEFAULT 0
AFTER wreck_hull;

-- Insurance is automatic. The owner and the retired policy are Phase 06
-- columns on ship_interiors.
ALTER TABLE ship_interiors
ADD COLUMN IF NOT EXISTS owner VARCHAR(64) NOT NULL DEFAULT '',
ADD COLUMN IF NOT EXISTS insured_for INT NOT NULL DEFAULT 0;

-- The premium of every policy bought before, a fifth of its value and at
-- least 1 gold, is refunded as a claim the settlement path delivers at the
-- owner's next login; clearing the policy in the same transaction makes the
-- refund run once. The server does the same at boot.
START TRANSACTION;

INSERT INTO vessel_insurance_claims (ship_id, owner, ship_name, amount)
SELECT
  ship_id,
  owner,
  vessel_name,
  GREATEST(1, insured_for DIV 5)
FROM ship_interiors
WHERE insured_for > 0 AND owner <> '';

UPDATE ship_interiors SET insured_for = 0
WHERE insured_for > 0;

COMMIT;
