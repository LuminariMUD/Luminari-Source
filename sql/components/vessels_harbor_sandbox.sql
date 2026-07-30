-- Development-only shared vessel harbor fixture.
-- Idempotently adds or normalizes reserved fixture rows and leaves all other
-- builder-authored rows unchanged.

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Harbor Sandbox Raft', 0, 5, 5
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Harbor Sandbox Raft'
 );

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Harbor Sandbox Hunted Raft', 0, 5, 100
 WHERE NOT EXISTS (
   SELECT 1
     FROM ship_prototypes
    WHERE name = 'Harbor Sandbox Hunted Raft'
 );

UPDATE ship_prototypes
   SET vessel_class = 0,
       max_speed = 5,
       armor = 100
 WHERE name = 'Harbor Sandbox Hunted Raft';

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

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Harbor Admiralty Hunter', 3, 8, 30
 WHERE NOT EXISTS (
   SELECT 1
     FROM ship_prototypes
    WHERE name = 'Harbor Admiralty Hunter'
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
SET @harbor_ferry_prototype_id = (
  SELECT MIN(prototype_id)
    FROM ship_prototypes
   WHERE name = 'Harbor Sandbox Ferry'
);
SET @harbor_hunter_prototype_id = (
  SELECT MIN(prototype_id)
    FROM ship_prototypes
   WHERE name = 'Harbor Admiralty Hunter'
);
SET @harbor_spice_id = (
  SELECT MIN(commodity_id)
    FROM trade_commodities
   WHERE name = 'spice'
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

INSERT INTO vessel_npc_merchants
  (name, faction_id, prototype_id, route_id, pilot_mob_vnum,
   spawn_x, spawn_y, spawn_z, cargo_commodity_id, cargo_quantity,
   schedule_interval_hours, respawn_delay_seconds, enabled)
SELECT 'Harbor Sandbox Merchant', 1, @harbor_ferry_prototype_id,
       @harbor_route_id, 70001, -66, 92, 0, @harbor_spice_id, 25, 1, 5, 1
 WHERE @harbor_ferry_prototype_id IS NOT NULL
   AND @harbor_route_id IS NOT NULL
   AND @harbor_spice_id IS NOT NULL
ON DUPLICATE KEY UPDATE
  faction_id = VALUES(faction_id),
  prototype_id = VALUES(prototype_id),
  route_id = VALUES(route_id),
  pilot_mob_vnum = VALUES(pilot_mob_vnum),
  spawn_x = VALUES(spawn_x),
  spawn_y = VALUES(spawn_y),
  spawn_z = VALUES(spawn_z),
  cargo_commodity_id = VALUES(cargo_commodity_id),
  cargo_quantity = VALUES(cargo_quantity),
  schedule_interval_hours = VALUES(schedule_interval_hours),
  respawn_delay_seconds = VALUES(respawn_delay_seconds),
  enabled = VALUES(enabled);

INSERT IGNORE INTO ship_room_template_triggers
  (room_type, vessel_type, trigger_vnum)
VALUES
  ('bridge', 0, 70001),
  ('cargo_main', 0, 70002);

-- Canonical wilderness geography for legal-water testing. The reserved
-- region VNUMs are inserted only when neither the VNUM nor fixture name is
-- already present; the provisioner rejects any collision.
INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT 7000001, 10000, 'Harbor Sandbox Territorial Waters', 1,
       ST_GeomFromText(
         'POLYGON((-70 78,-60 78,-60 96,-70 96,-70 78))'
       ),
       0, '', NULL
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data WHERE vnum = 7000001
 )
   AND NOT EXISTS (
     SELECT 1
       FROM region_data
      WHERE name = 'Harbor Sandbox Territorial Waters'
   );

INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT 7000002, 10000, 'Harbor Sandbox Free Seas', 1,
       ST_GeomFromText(
         'POLYGON((-66 79,-60 79,-60 87,-66 87,-66 79))'
       ),
       0, '', NULL
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data WHERE vnum = 7000002
 )
   AND NOT EXISTS (
     SELECT 1 FROM region_data WHERE name = 'Harbor Sandbox Free Seas'
   );

INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT 7000003, 10000, 'Harbor Sandbox Pirate Cove', 1,
       ST_GeomFromText(
         'POLYGON((-60 88,-56 88,-56 94,-60 94,-60 88))'
       ),
       0, '', NULL
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data WHERE vnum = 7000003
 )
   AND NOT EXISTS (
     SELECT 1 FROM region_data WHERE name = 'Harbor Sandbox Pirate Cove'
   );

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 1,
       region_polygon = ST_GeomFromText(
         'POLYGON((-70 78,-60 78,-60 96,-70 96,-70 78))'
       ),
       region_props = 0
 WHERE vnum = 7000001
   AND name = 'Harbor Sandbox Territorial Waters';

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 1,
       region_polygon = ST_GeomFromText(
         'POLYGON((-66 79,-60 79,-60 87,-66 87,-66 79))'
       ),
       region_props = 0
 WHERE vnum = 7000002
   AND name = 'Harbor Sandbox Free Seas';

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 1,
       region_polygon = ST_GeomFromText(
         'POLYGON((-60 88,-56 88,-56 94,-60 94,-60 88))'
       ),
       region_props = 0
 WHERE vnum = 7000003
   AND name = 'Harbor Sandbox Pirate Cove';

-- The encounter polygon uses the canonical REGION_ENCOUNTER layer and may
-- overlap geographic law polygons by design.
INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT 7000004, 10000, 'Harbor Sandbox Bounty Patrol', 2,
       ST_GeomFromText(
         'POLYGON((-70 78,-60 78,-60 96,-70 96,-70 78))'
       ),
       0, '', NULL
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data WHERE vnum = 7000004
 )
   AND NOT EXISTS (
     SELECT 1
       FROM region_data
      WHERE name = 'Harbor Sandbox Bounty Patrol'
   );

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 2,
       region_polygon = ST_GeomFromText(
         'POLYGON((-70 78,-60 78,-60 96,-70 96,-70 78))'
       ),
       region_props = 0
 WHERE vnum = 7000004
   AND name = 'Harbor Sandbox Bounty Patrol';

INSERT INTO vessel_encounters
  (region_vnum, name, mob_vnum, min_depth, max_depth, vessel_class,
   chance, warn_message, arrive_message)
SELECT 7000004, 'Harbor Admiralty hunter patrol', 0, 0, 0, 0, 100,
       'A navy pennant rises on the horizon - the Admiralty has found you!',
       'A Harbor Admiralty warship bears down with its ballistae run out!'
 WHERE @harbor_hunter_prototype_id IS NOT NULL
   AND NOT EXISTS (
     SELECT 1
       FROM vessel_encounters
      WHERE region_vnum = 7000004
        AND name = 'Harbor Admiralty hunter patrol'
   );

UPDATE vessel_encounters
   SET mob_vnum = 0,
       min_depth = 0,
       max_depth = 0,
       vessel_class = 0,
       chance = 100,
       warn_message =
         'A navy pennant rises on the horizon - the Admiralty has found you!',
       arrive_message =
         'A Harbor Admiralty warship bears down with its ballistae run out!'
 WHERE region_vnum = 7000004
   AND name = 'Harbor Admiralty hunter patrol';

SET @harbor_hunter_encounter_id = (
  SELECT MIN(encounter_id)
    FROM vessel_encounters
   WHERE region_vnum = 7000004
     AND name = 'Harbor Admiralty hunter patrol'
);

INSERT INTO vessel_hunter_encounters
  (encounter_id, prototype_id, pilot_mob_vnum, min_bounty,
   pursuit_speed, hunt_duration_seconds, target_grace_seconds,
   cooldown_seconds, enabled)
SELECT @harbor_hunter_encounter_id, @harbor_hunter_prototype_id, 70002,
       2000, 5, 300, 15, 30, 1
 WHERE @harbor_hunter_encounter_id IS NOT NULL
   AND @harbor_hunter_prototype_id IS NOT NULL
ON DUPLICATE KEY UPDATE
  prototype_id = VALUES(prototype_id),
  pilot_mob_vnum = VALUES(pilot_mob_vnum),
  min_bounty = VALUES(min_bounty),
  pursuit_speed = VALUES(pursuit_speed),
  hunt_duration_seconds = VALUES(hunt_duration_seconds),
  target_grace_seconds = VALUES(target_grace_seconds),
  cooldown_seconds = VALUES(cooldown_seconds),
  enabled = VALUES(enabled);

INSERT INTO vessel_region_law
  (region_vnum, waters_type, priority, bounty_percent, authority)
SELECT vnum, 1, 100, 150, 'Harbor Admiralty'
  FROM region_data
 WHERE vnum = 7000001
   AND name = 'Harbor Sandbox Territorial Waters'
ON DUPLICATE KEY UPDATE
  waters_type = VALUES(waters_type),
  priority = VALUES(priority),
  bounty_percent = VALUES(bounty_percent),
  authority = VALUES(authority);

INSERT INTO vessel_region_law
  (region_vnum, waters_type, priority, bounty_percent, authority)
SELECT vnum, 2, 150, 100, 'Free Captains'' Compact'
  FROM region_data
 WHERE vnum = 7000002
   AND name = 'Harbor Sandbox Free Seas'
ON DUPLICATE KEY UPDATE
  waters_type = VALUES(waters_type),
  priority = VALUES(priority),
  bounty_percent = VALUES(bounty_percent),
  authority = VALUES(authority);

INSERT INTO vessel_region_law
  (region_vnum, waters_type, priority, bounty_percent, authority)
SELECT vnum, 3, 200, 0, 'Cove Brotherhood'
  FROM region_data
 WHERE vnum = 7000003
   AND name = 'Harbor Sandbox Pirate Cove'
ON DUPLICATE KEY UPDATE
  waters_type = VALUES(waters_type),
  priority = VALUES(priority),
  bounty_percent = VALUES(bounty_percent),
  authority = VALUES(authority);

UPDATE ship_schedules AS schedule
  JOIN ship_routes AS route ON route.route_id = schedule.route_id
   SET schedule.passenger_fare = 10
 WHERE route.name = 'harbor_ferry_loop';
