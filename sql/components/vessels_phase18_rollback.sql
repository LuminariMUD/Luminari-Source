-- Vessel System Phase 18 rollback.
-- Removes shipyard listings and hull levels. Cleared wage debt is not restored.

ALTER TABLE ship_prototypes
DROP COLUMN IF EXISTS min_level,
DROP COLUMN IF EXISTS for_sale;
