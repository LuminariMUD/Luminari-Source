-- Vessel System Phase 21 rollback.
-- Removes crew experience. Pre-S5 code keeps each hand's tier.

ALTER TABLE ship_crew_roster
DROP COLUMN IF EXISTS experience;
