-- VESSELS PHASE 2 SCHEMA VERIFICATION SCRIPT
-- Run this after installing vessels_phase2_schema.sql to verify everything is correct
-- Usage: mysql -u root -p luminari_mudprod < verify_vessels_schema.sql

-- Check that all required tables exist
SELECT '=== CHECKING TABLES ===' as Status;

SELECT 
    CASE 
        WHEN COUNT(*) = 5 THEN 'PASS: All 5 tables exist'
        ELSE CONCAT('FAIL: Only ', COUNT(*), ' of 5 tables exist')
    END as Table_Check
FROM information_schema.TABLES 
WHERE TABLE_SCHEMA = DATABASE() 
AND TABLE_NAME IN ('ship_interiors', 'ship_docking', 'ship_room_templates', 
                   'ship_cargo_manifest', 'ship_crew_roster');

-- List all vessel tables
SELECT TABLE_NAME, TABLE_ROWS, AUTO_INCREMENT, CREATE_TIME
FROM information_schema.TABLES 
WHERE TABLE_SCHEMA = DATABASE() 
AND TABLE_NAME LIKE 'ship_%'
ORDER BY TABLE_NAME;

-- Check ship_interiors structure
SELECT '=== CHECKING ship_interiors STRUCTURE ===' as Status;
SELECT COLUMN_NAME, DATA_TYPE, IS_NULLABLE, COLUMN_KEY, COLUMN_DEFAULT
FROM information_schema.COLUMNS
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ship_interiors'
ORDER BY ORDINAL_POSITION;

-- Check indexes on ship_interiors
SELECT '=== CHECKING ship_interiors INDEXES ===' as Status;
SELECT INDEX_NAME, COLUMN_NAME, NON_UNIQUE
FROM information_schema.STATISTICS
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ship_interiors'
ORDER BY INDEX_NAME, SEQ_IN_INDEX;

-- Check ship_docking structure
SELECT '=== CHECKING ship_docking STRUCTURE ===' as Status;
SELECT COLUMN_NAME, DATA_TYPE, IS_NULLABLE, COLUMN_KEY, COLUMN_DEFAULT
FROM information_schema.COLUMNS
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ship_docking'
ORDER BY ORDINAL_POSITION;

-- Check unique constraint on ship_docking
SELECT '=== CHECKING ship_docking CONSTRAINTS ===' as Status;
SELECT CONSTRAINT_NAME, CONSTRAINT_TYPE
FROM information_schema.TABLE_CONSTRAINTS
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ship_docking';

-- Check foreign keys
SELECT '=== CHECKING FOREIGN KEYS ===' as Status;
SELECT 
    TABLE_NAME,
    COLUMN_NAME,
    CONSTRAINT_NAME,
    REFERENCED_TABLE_NAME,
    REFERENCED_COLUMN_NAME
FROM information_schema.KEY_COLUMN_USAGE
WHERE TABLE_SCHEMA = DATABASE()
AND REFERENCED_TABLE_NAME IS NOT NULL
AND TABLE_NAME LIKE 'ship_%';

-- Check stored procedures
SELECT '=== CHECKING STORED PROCEDURES ===' as Status;
SELECT 
    ROUTINE_NAME,
    ROUTINE_TYPE,
    CREATED,
    LAST_ALTERED
FROM information_schema.ROUTINES
WHERE ROUTINE_SCHEMA = DATABASE()
AND ROUTINE_NAME IN ('cleanup_orphaned_dockings', 'get_active_dockings');

-- Verify room templates were inserted
SELECT '=== CHECKING ROOM TEMPLATES DATA ===' as Status;
SELECT 
    CASE 
        WHEN COUNT(*) >= 19 THEN CONCAT('PASS: ', COUNT(*), ' room templates loaded')
        ELSE CONCAT('FAIL: Only ', COUNT(*), ' room templates (expected 19)')
    END as Template_Check
FROM ship_room_templates;

-- Show room template summary
SELECT room_type, COUNT(*) as count, MIN(min_vessel_size) as min_size
FROM ship_room_templates
GROUP BY room_type
ORDER BY room_type;

-- Test stored procedures (non-destructive)
SELECT '=== TESTING STORED PROCEDURES ===' as Status;

-- Test get_active_dockings with non-existent ship (should return empty)
CALL get_active_dockings(99999);

-- Summary
SELECT '=== VERIFICATION SUMMARY ===' as Status;
SELECT 
    (SELECT COUNT(*) FROM information_schema.TABLES 
     WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME LIKE 'ship_%') as Tables_Created,
    (SELECT COUNT(*) FROM ship_room_templates) as Room_Templates_Loaded,
    (SELECT COUNT(*) FROM information_schema.ROUTINES 
     WHERE ROUTINE_SCHEMA = DATABASE() 
     AND ROUTINE_NAME IN ('cleanup_orphaned_dockings', 'get_active_dockings')) as Stored_Procedures_Created;