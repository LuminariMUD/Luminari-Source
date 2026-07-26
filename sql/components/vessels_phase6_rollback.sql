-- Vessel System Phase 06 rollback.
-- WARNING: destroys ownership, upgrades, insurance, wage debt, helm
-- permits, and hired crew.

ALTER TABLE ship_interiors
  DROP COLUMN IF EXISTS owner,
  DROP COLUMN IF EXISTS upgrades,
  DROP COLUMN IF EXISTS insured_for,
  DROP COLUMN IF EXISTS wages_owed;

DELETE FROM ship_crew_roster WHERE crew_role = 'captain' AND npc_vnum = -1;
DELETE FROM ship_crew_roster WHERE npc_vnum <= -100;
