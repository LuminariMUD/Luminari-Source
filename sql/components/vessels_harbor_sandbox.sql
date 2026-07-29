-- Development-only shared vessel harbor fixture.
-- Idempotently adds or normalizes reserved fixture rows and leaves all other
-- builder-authored rows unchanged.

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Harbor Sandbox Raft', 0, 5, 5
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Harbor Sandbox Raft'
 );

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Harbor Sandbox Ferry', 2, 10, 20
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Harbor Sandbox Ferry'
 );

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Harbor Sandbox Airship', 4, 25, 15
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Harbor Sandbox Airship'
 );

INSERT INTO ship_waypoints (name, x, y, z, tolerance, wait_time, flags)
SELECT 'harbor_west_dock', -66, 92, 0, 0.5, 15, 0
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_waypoints WHERE name = 'harbor_west_dock'
 );

INSERT INTO ship_waypoints (name, x, y, z, tolerance, wait_time, flags)
SELECT 'harbor_east_dock', -62, 82, 0, 0.5, 15, 0
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_waypoints WHERE name = 'harbor_east_dock'
 );

INSERT INTO ship_waypoints (name, x, y, z, tolerance, wait_time, flags)
SELECT 'harbor_channel_turn', -64, 82, 0, 0.5, 0, 0
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_waypoints WHERE name = 'harbor_channel_turn'
 );

INSERT INTO ship_routes (name, loop_route, active)
SELECT 'harbor_ferry_loop', 1, 1
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_routes WHERE name = 'harbor_ferry_loop'
 );

SET @harbor_route_id = (
  SELECT MIN(route_id) FROM ship_routes WHERE name = 'harbor_ferry_loop'
);
SET @harbor_west_id = (
  SELECT MIN(waypoint_id) FROM ship_waypoints WHERE name = 'harbor_west_dock'
);
SET @harbor_east_id = (
  SELECT MIN(waypoint_id) FROM ship_waypoints WHERE name = 'harbor_east_dock'
);
SET @harbor_channel_id = (
  SELECT MIN(waypoint_id) FROM ship_waypoints WHERE name = 'harbor_channel_turn'
);

INSERT INTO ship_route_waypoints (route_id, waypoint_id, sequence_num)
SELECT @harbor_route_id, @harbor_west_id, 0
 WHERE NOT EXISTS (
   SELECT 1
     FROM ship_route_waypoints
    WHERE route_id = @harbor_route_id
      AND sequence_num = 0
 );

INSERT INTO ship_route_waypoints (route_id, waypoint_id, sequence_num)
VALUES (@harbor_route_id, @harbor_channel_id, 1)
ON DUPLICATE KEY UPDATE waypoint_id = VALUES(waypoint_id);

INSERT INTO ship_route_waypoints (route_id, waypoint_id, sequence_num)
VALUES (@harbor_route_id, @harbor_east_id, 2)
ON DUPLICATE KEY UPDATE waypoint_id = VALUES(waypoint_id);

INSERT INTO ship_route_waypoints (route_id, waypoint_id, sequence_num)
VALUES (@harbor_route_id, @harbor_channel_id, 3)
ON DUPLICATE KEY UPDATE waypoint_id = VALUES(waypoint_id);

INSERT IGNORE INTO ship_room_template_triggers
  (room_type, vessel_type, trigger_vnum)
VALUES
  ('bridge', 0, 70001),
  ('cargo_main', 0, 70002);
