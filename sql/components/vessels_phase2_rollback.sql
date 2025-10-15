-- VESSELS PHASE 2 ROLLBACK SCRIPT
-- Emergency rollback script to remove Phase 2 vessel system from database
-- WARNING: This will DELETE all vessel interior data!
-- 
-- Usage: mysql -u root -p luminari_mudprod < vessels_phase2_rollback.sql

-- Disable foreign key checks temporarily
SET FOREIGN_KEY_CHECKS = 0;

-- Drop stored procedures
DROP PROCEDURE IF EXISTS cleanup_orphaned_dockings;
DROP PROCEDURE IF EXISTS get_active_dockings;

-- Drop tables in reverse order of dependencies
DROP TABLE IF EXISTS ship_crew_roster;
DROP TABLE IF EXISTS ship_cargo_manifest;
DROP TABLE IF EXISTS ship_room_templates;
DROP TABLE IF EXISTS ship_docking;
DROP TABLE IF EXISTS ship_interiors;

-- Re-enable foreign key checks
SET FOREIGN_KEY_CHECKS = 1;

-- Verify cleanup
SELECT CONCAT('Rollback complete. Remaining ship_ tables: ', COUNT(*)) as Status
FROM information_schema.TABLES 
WHERE TABLE_SCHEMA = DATABASE() 
AND TABLE_NAME LIKE 'ship_%';