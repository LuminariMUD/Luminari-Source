-- Vessel System 3.0: initial Luminari campaign shipping content.
--
-- This package uses the existing North and Central Vailand wilderness
-- seaports. It is idempotent after the collision checks in
-- scripts/provision_vessel_campaign.sh have passed.

SET @vailand_north_region_vnum = 1000013;
SET @vailand_central_region_vnum = 1000014;
SET @vailand_passage_region_vnum = 1000015;
SET @blackwake_region_vnum = 1000016;

INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT @vailand_north_region_vnum, 10000,
       'North Vailand Territorial Waters', 1,
       ST_GeomFromText(
         'POLYGON((-612 440,-584 440,-584 470,-612 470,-612 440))'
       ),
       0, '', NULL
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data
    WHERE vnum = @vailand_north_region_vnum
       OR name = 'North Vailand Territorial Waters'
 );

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 1,
       region_polygon = ST_GeomFromText(
         'POLYGON((-612 440,-584 440,-584 470,-612 470,-612 440))'
       ),
       region_props = 0,
       region_reset_data = '',
       region_reset_time = NULL
 WHERE vnum = @vailand_north_region_vnum
   AND name = 'North Vailand Territorial Waters';

INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT @vailand_central_region_vnum, 10000,
       'Central Vailand Territorial Waters', 1,
       ST_GeomFromText(
         'POLYGON((-480 190,-452 190,-452 220,-480 220,-480 190))'
       ),
       0, '', NULL
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data
    WHERE vnum = @vailand_central_region_vnum
       OR name = 'Central Vailand Territorial Waters'
 );

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 1,
       region_polygon = ST_GeomFromText(
         'POLYGON((-480 190,-452 190,-452 220,-480 220,-480 190))'
       ),
       region_props = 0,
       region_reset_data = '',
       region_reset_time = NULL
 WHERE vnum = @vailand_central_region_vnum
   AND name = 'Central Vailand Territorial Waters';

INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT @vailand_passage_region_vnum, 10000, 'Vailand Passage', 1,
       ST_GeomFromText(
         'POLYGON((-581 464,-449 213,-485 195,-617 446,-581 464))'
       ),
       0, '', NULL
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data
    WHERE vnum = @vailand_passage_region_vnum
       OR name = 'Vailand Passage'
 );

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 1,
       region_polygon = ST_GeomFromText(
         'POLYGON((-581 464,-449 213,-485 195,-617 446,-581 464))'
       ),
       region_props = 0,
       region_reset_data = '',
       region_reset_time = NULL
 WHERE vnum = @vailand_passage_region_vnum
   AND name = 'Vailand Passage';

INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT @blackwake_region_vnum, 10000, 'Blackwake Anchorage', 1,
       ST_GeomFromText(
         'POLYGON((-542 318,-524 318,-524 342,-542 342,-542 318))'
       ),
       0, '', NULL
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data
    WHERE vnum = @blackwake_region_vnum
       OR name = 'Blackwake Anchorage'
 );

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 1,
       region_polygon = ST_GeomFromText(
         'POLYGON((-542 318,-524 318,-524 342,-542 342,-542 318))'
       ),
       region_props = 0,
       region_reset_data = '',
       region_reset_time = NULL
 WHERE vnum = @blackwake_region_vnum
   AND name = 'Blackwake Anchorage';

INSERT INTO vessel_region_law
  (region_vnum, waters_type, priority, bounty_percent, authority)
VALUES
  (@vailand_north_region_vnum, 1, 200, 150,
   'Vailand Harbor Compact'),
  (@vailand_central_region_vnum, 1, 200, 150,
   'Vailand Harbor Compact'),
  (@vailand_passage_region_vnum, 2, 100, 100,
   'Free Captains of Vailand'),
  (@blackwake_region_vnum, 3, 300, 0,
   'Blackwake Brotherhood')
ON DUPLICATE KEY UPDATE
  waters_type = VALUES(waters_type),
  priority = VALUES(priority),
  bounty_percent = VALUES(bounty_percent),
  authority = VALUES(authority);

SET @vailand_route_name = 'Vailand Iron Passage';
SET @vailand_prototype_name = 'Vailand Merchant Cog';
SET @vailand_merchant_name = 'Vailand Ironwind Trader';

INSERT INTO ship_waypoints
  (name, x, y, z, tolerance, wait_time, flags)
SELECT 'vailand_north_port', -599, 455, 0, 0.5, 30, 0
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_waypoints WHERE name = 'vailand_north_port'
 );
INSERT INTO ship_waypoints
  (name, x, y, z, tolerance, wait_time, flags)
SELECT 'vailand_northing', -573, 405, 0, 0.5, 0, 0
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_waypoints WHERE name = 'vailand_northing'
 );
INSERT INTO ship_waypoints
  (name, x, y, z, tolerance, wait_time, flags)
SELECT 'vailand_outer_passage', -546, 355, 0, 0.5, 0, 0
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_waypoints WHERE name = 'vailand_outer_passage'
 );
INSERT INTO ship_waypoints
  (name, x, y, z, tolerance, wait_time, flags)
SELECT 'blackwake_anchorage', -533, 330, 0, 0.5, 0, 0
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_waypoints WHERE name = 'blackwake_anchorage'
 );
INSERT INTO ship_waypoints
  (name, x, y, z, tolerance, wait_time, flags)
SELECT 'vailand_southing', -507, 279, 0, 0.5, 0, 0
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_waypoints WHERE name = 'vailand_southing'
 );
INSERT INTO ship_waypoints
  (name, x, y, z, tolerance, wait_time, flags)
SELECT 'vailand_central_approach', -480, 229, 0, 0.5, 0, 0
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_waypoints WHERE name = 'vailand_central_approach'
 );
INSERT INTO ship_waypoints
  (name, x, y, z, tolerance, wait_time, flags)
SELECT 'vailand_central_port', -467, 204, 0, 0.5, 30, 0
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_waypoints WHERE name = 'vailand_central_port'
 );

UPDATE ship_waypoints
   SET x = -599, y = 455, z = 0, tolerance = 0.5,
       wait_time = 30, flags = 0
 WHERE name = 'vailand_north_port';
UPDATE ship_waypoints
   SET x = -573, y = 405, z = 0, tolerance = 0.5,
       wait_time = 0, flags = 0
 WHERE name = 'vailand_northing';
UPDATE ship_waypoints
   SET x = -546, y = 355, z = 0, tolerance = 0.5,
       wait_time = 0, flags = 0
 WHERE name = 'vailand_outer_passage';
UPDATE ship_waypoints
   SET x = -533, y = 330, z = 0, tolerance = 0.5,
       wait_time = 0, flags = 0
 WHERE name = 'blackwake_anchorage';
UPDATE ship_waypoints
   SET x = -507, y = 279, z = 0, tolerance = 0.5,
       wait_time = 0, flags = 0
 WHERE name = 'vailand_southing';
UPDATE ship_waypoints
   SET x = -480, y = 229, z = 0, tolerance = 0.5,
       wait_time = 0, flags = 0
 WHERE name = 'vailand_central_approach';
UPDATE ship_waypoints
   SET x = -467, y = 204, z = 0, tolerance = 0.5,
       wait_time = 30, flags = 0
 WHERE name = 'vailand_central_port';

SET @vailand_north_port_id = (
  SELECT MIN(waypoint_id) FROM ship_waypoints
   WHERE name = 'vailand_north_port'
);
SET @vailand_northing_id = (
  SELECT MIN(waypoint_id) FROM ship_waypoints
   WHERE name = 'vailand_northing'
);
SET @vailand_outer_passage_id = (
  SELECT MIN(waypoint_id) FROM ship_waypoints
   WHERE name = 'vailand_outer_passage'
);
SET @blackwake_anchorage_id = (
  SELECT MIN(waypoint_id) FROM ship_waypoints
   WHERE name = 'blackwake_anchorage'
);
SET @vailand_southing_id = (
  SELECT MIN(waypoint_id) FROM ship_waypoints
   WHERE name = 'vailand_southing'
);
SET @vailand_central_approach_id = (
  SELECT MIN(waypoint_id) FROM ship_waypoints
   WHERE name = 'vailand_central_approach'
);
SET @vailand_central_port_id = (
  SELECT MIN(waypoint_id) FROM ship_waypoints
   WHERE name = 'vailand_central_port'
);

INSERT INTO ship_routes (name, loop_route, active)
SELECT @vailand_route_name, 1, 1
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_routes WHERE name = @vailand_route_name
 );
SET @vailand_route_id = (
  SELECT MIN(route_id) FROM ship_routes WHERE name = @vailand_route_name
);
UPDATE ship_routes
   SET loop_route = 1, active = 1
 WHERE route_id = @vailand_route_id;

DELETE FROM ship_route_waypoints WHERE route_id = @vailand_route_id;
INSERT INTO ship_route_waypoints
  (route_id, waypoint_id, sequence_num)
VALUES
  (@vailand_route_id, @vailand_north_port_id, 0),
  (@vailand_route_id, @vailand_northing_id, 1),
  (@vailand_route_id, @vailand_outer_passage_id, 2),
  (@vailand_route_id, @blackwake_anchorage_id, 3),
  (@vailand_route_id, @vailand_southing_id, 4),
  (@vailand_route_id, @vailand_central_approach_id, 5),
  (@vailand_route_id, @vailand_central_port_id, 6),
  (@vailand_route_id, @vailand_central_approach_id, 7),
  (@vailand_route_id, @vailand_southing_id, 8),
  (@vailand_route_id, @blackwake_anchorage_id, 9),
  (@vailand_route_id, @vailand_outer_passage_id, 10),
  (@vailand_route_id, @vailand_northing_id, 11);

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT @vailand_prototype_name, 2, 12, 30
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = @vailand_prototype_name
 );
SET @vailand_prototype_id = (
  SELECT MIN(prototype_id) FROM ship_prototypes
   WHERE name = @vailand_prototype_name
);
UPDATE ship_prototypes
   SET vessel_class = 2, max_speed = 12, armor = 30
 WHERE prototype_id = @vailand_prototype_id;

SET @iron_commodity_id = (
  SELECT MIN(commodity_id) FROM trade_commodities WHERE name = 'iron'
);

INSERT INTO port_commodities (port_vnum, commodity_id, supply)
VALUES
  (1000360, @iron_commodity_id, 320),
  (1000362, @iron_commodity_id, 80)
ON DUPLICATE KEY UPDATE supply = VALUES(supply);

INSERT INTO vessel_npc_merchants
  (name, faction_id, prototype_id, route_id, pilot_mob_vnum,
   spawn_x, spawn_y, spawn_z, cargo_commodity_id, cargo_quantity,
   schedule_interval_hours, respawn_delay_seconds, enabled)
SELECT @vailand_merchant_name, 1, @vailand_prototype_id,
       @vailand_route_id, 31810, -599, 455, 0,
       @iron_commodity_id, 40, 1, 3600, 1
 WHERE @vailand_prototype_id IS NOT NULL
   AND @vailand_route_id IS NOT NULL
   AND @iron_commodity_id IS NOT NULL
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
