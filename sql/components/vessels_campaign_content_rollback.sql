-- Disable and remove the initial Luminari vessel campaign package.
--
-- Stop the MUD and purge the active Vailand Ironwind Trader hull through the
-- operator command before applying this file. An active definition is kept
-- disabled rather than leaving a reconstructed hull with missing dependencies.

UPDATE vessel_npc_merchants
   SET enabled = 0
 WHERE name = 'Vailand Ironwind Trader';

DELETE FROM vessel_npc_merchants
 WHERE name = 'Vailand Ironwind Trader'
   AND (active_ship_id IS NULL OR active_ship_id = 0);

DELETE FROM ship_prototypes
 WHERE name = 'Vailand Merchant Cog'
   AND NOT EXISTS (
     SELECT 1 FROM vessel_npc_merchants
      WHERE name = 'Vailand Ironwind Trader'
   )
   AND NOT EXISTS (
     SELECT 1 FROM ship_runtime_state AS runtime
      WHERE runtime.prototype_id = ship_prototypes.prototype_id
   );

DELETE FROM ship_routes
 WHERE name = 'Vailand Iron Passage'
   AND NOT EXISTS (
     SELECT 1 FROM vessel_npc_merchants
      WHERE name = 'Vailand Ironwind Trader'
   )
   AND NOT EXISTS (
     SELECT 1 FROM ship_schedules AS schedule
      WHERE schedule.route_id = ship_routes.route_id
   );

DELETE FROM ship_waypoints
 WHERE name IN (
   'vailand_north_port',
   'vailand_northing',
   'vailand_outer_passage',
   'blackwake_anchorage',
   'vailand_southing',
   'vailand_central_approach',
   'vailand_central_port'
 )
   AND NOT EXISTS (
     SELECT 1 FROM vessel_npc_merchants
      WHERE name = 'Vailand Ironwind Trader'
   )
   AND NOT EXISTS (
     SELECT 1 FROM ship_route_waypoints AS link
      WHERE link.waypoint_id = ship_waypoints.waypoint_id
   );

DELETE FROM port_commodities
 WHERE port_vnum IN (1000360, 1000362)
   AND commodity_id = (
     SELECT commodity_id FROM trade_commodities WHERE name = 'iron'
   )
   AND ((port_vnum = 1000360 AND supply = 320)
     OR (port_vnum = 1000362 AND supply = 80))
   AND NOT EXISTS (
     SELECT 1 FROM vessel_npc_merchants
      WHERE name = 'Vailand Ironwind Trader'
   );

DELETE FROM vessel_region_law
 WHERE region_vnum IN (1000013, 1000014, 1000015, 1000016)
   AND NOT EXISTS (
     SELECT 1 FROM vessel_npc_merchants
      WHERE name = 'Vailand Ironwind Trader'
   );

DELETE FROM region_data
 WHERE ((vnum = 1000013 AND name = 'North Vailand Territorial Waters')
     OR (vnum = 1000014 AND name = 'Central Vailand Territorial Waters')
     OR (vnum = 1000015 AND name = 'Vailand Passage')
     OR (vnum = 1000016 AND name = 'Blackwake Anchorage'))
   AND NOT EXISTS (
     SELECT 1 FROM vessel_npc_merchants
      WHERE name = 'Vailand Ironwind Trader'
   );
