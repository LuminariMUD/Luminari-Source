-- Vessel System Phase 21 rollback.
-- Removes crew experience and the stowed-hull state. Pre-S5 code keeps each
-- hand's tier. Refunded premiums stay refunded, and pre-S5 code treats the
-- hulls as uninsured. Remove stowed hulls with shippurge before rolling back:
-- older code would place them back in the world where they were saved.

ALTER TABLE ship_crew_roster
DROP COLUMN IF EXISTS experience;

ALTER TABLE ship_runtime_state
DROP COLUMN IF EXISTS stowed,
DROP COLUMN IF EXISTS wreck_hull,
DROP COLUMN IF EXISTS summon_due;
