-- Vessel System Phase 06 verification.
-- Expect 4 rows: owner, upgrades, insured_for, wages_owed.

SELECT COLUMN_NAME, DATA_TYPE
  FROM information_schema.COLUMNS
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'ship_interiors'
   AND COLUMN_NAME IN ('owner', 'upgrades', 'insured_for', 'wages_owed')
 ORDER BY COLUMN_NAME;

-- Ownership and crew census
SELECT COUNT(*) AS owned_ships FROM ship_interiors WHERE owner <> '';
SELECT COUNT(*) AS helm_permits FROM ship_crew_roster
 WHERE crew_role = 'captain' AND npc_vnum = -1;
SELECT COUNT(*) AS hired_crew FROM ship_crew_roster WHERE npc_vnum <= -100;
