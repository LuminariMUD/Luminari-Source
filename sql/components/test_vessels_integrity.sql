-- VESSELS PHASE 2 DATA INTEGRITY TEST
-- Tests insert, update, delete, and foreign key constraints
-- This is safe to run - it cleans up after itself

-- Test 1: Insert a test ship interior
INSERT INTO ship_interiors (ship_id, vessel_type, vessel_name, num_rooms, room_vnums, bridge_room)
VALUES (99999, 1, 'Test Vessel', 5, '100001,100002,100003,100004,100005', 100001);

SELECT 'Test 1: Ship interior inserted' as Status;

-- Test 2: Insert cargo manifest with foreign key
INSERT INTO ship_cargo_manifest (ship_id, cargo_room, item_vnum, item_name, item_count, item_weight, loaded_by)
VALUES (99999, 100003, 3001, 'Test Cargo', 10, 50, 'TestUser');

SELECT 'Test 2: Cargo manifest with FK inserted' as Status;

-- Test 3: Insert crew roster with foreign key
INSERT INTO ship_crew_roster (ship_id, npc_vnum, npc_name, crew_role, assigned_room)
VALUES (99999, 5001, 'Test Sailor', 'crew', 100002);

SELECT 'Test 3: Crew roster with FK inserted' as Status;

-- Test 4: Test docking record
INSERT INTO ship_docking (ship1_id, ship2_id, dock_room1, dock_room2, dock_type, dock_x, dock_y)
VALUES (99999, 99998, 100001, 200001, 'standard', 100, 100);

SELECT 'Test 4: Docking record inserted' as Status;

-- Test 5: Update ship interior (test ON UPDATE timestamp)
UPDATE ship_interiors SET num_rooms = 6 WHERE ship_id = 99999;

SELECT 'Test 5: Ship interior updated' as Status;

-- Test 6: Verify cascade delete
SELECT COUNT(*) as cargo_before FROM ship_cargo_manifest WHERE ship_id = 99999;
SELECT COUNT(*) as crew_before FROM ship_crew_roster WHERE ship_id = 99999;

DELETE FROM ship_interiors WHERE ship_id = 99999;

SELECT COUNT(*) as cargo_after FROM ship_cargo_manifest WHERE ship_id = 99999;
SELECT COUNT(*) as crew_after FROM ship_crew_roster WHERE ship_id = 99999;

SELECT 'Test 6: CASCADE DELETE verified (should show 1,1,0,0)' as Status;

-- Test 7: Clean up docking record
DELETE FROM ship_docking WHERE ship1_id = 99999 OR ship2_id = 99999;

SELECT 'Test 7: Cleanup complete' as Status;

-- Test 8: Test stored procedure
CALL get_active_dockings(1);
SELECT 'Test 8: Stored procedure executed' as Status;

-- Final verification - ensure no test data remains
SELECT 
    (SELECT COUNT(*) FROM ship_interiors WHERE ship_id = 99999) as ship_remains,
    (SELECT COUNT(*) FROM ship_cargo_manifest WHERE ship_id = 99999) as cargo_remains,
    (SELECT COUNT(*) FROM ship_crew_roster WHERE ship_id = 99999) as crew_remains,
    (SELECT COUNT(*) FROM ship_docking WHERE ship1_id = 99999 OR ship2_id = 99999) as dock_remains;

SELECT 'All tests complete - database should be clean (all zeros above)' as Final_Status;