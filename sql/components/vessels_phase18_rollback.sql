-- Vessel System Phase 18 rollback.
-- Removes shipyard listings, hull levels, and the bounty decay clock. Cleared
-- wage debt and decayed or paid bounties are not restored.

ALTER TABLE vessel_bounties
DROP COLUMN IF EXISTS last_offense_at;

ALTER TABLE ship_prototypes
DROP COLUMN IF EXISTS min_level,
DROP COLUMN IF EXISTS for_sale;
