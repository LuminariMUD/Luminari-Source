-- Read-only verification for the initial Luminari vessel campaign package.

SELECT 'campaign_regions' AS check_name, COUNT(*) AS actual, 4 AS expected
  FROM region_data AS region
  JOIN vessel_region_law AS law ON law.region_vnum = region.vnum
 WHERE region.vnum IN (1000013, 1000014, 1000015, 1000016)
   AND region.zone_vnum = 10000
   AND region.region_type = 1
   AND region.region_polygon IS NOT NULL;

SELECT 'campaign_route_links' AS check_name, COUNT(*) AS actual, 12 AS expected
  FROM ship_routes AS route
  JOIN ship_route_waypoints AS link ON link.route_id = route.route_id
 WHERE route.name = 'Vailand Iron Passage'
   AND route.loop_route = 1
   AND route.active = 1;

SELECT 'campaign_route_sequence' AS check_name,
       COALESCE(GROUP_CONCAT(waypoint.name ORDER BY link.sequence_num
                            SEPARATOR ','), '') AS actual,
       CONCAT(
         'vailand_north_port,vailand_northing,vailand_outer_passage,',
         'blackwake_anchorage,vailand_southing,vailand_central_approach,',
         'vailand_central_port,vailand_central_approach,vailand_southing,',
         'blackwake_anchorage,vailand_outer_passage,vailand_northing'
       ) AS expected
  FROM ship_routes AS route
  JOIN ship_route_waypoints AS link ON link.route_id = route.route_id
  JOIN ship_waypoints AS waypoint ON waypoint.waypoint_id = link.waypoint_id
 WHERE route.name = 'Vailand Iron Passage';

SELECT 'campaign_merchant' AS check_name, COUNT(*) AS actual, 1 AS expected
  FROM vessel_npc_merchants AS merchant
  JOIN ship_prototypes AS prototype
    ON prototype.prototype_id = merchant.prototype_id
  JOIN ship_routes AS route ON route.route_id = merchant.route_id
  JOIN trade_commodities AS commodity
    ON commodity.commodity_id = merchant.cargo_commodity_id
 WHERE merchant.name = 'Vailand Ironwind Trader'
   AND merchant.faction_id = 1
   AND merchant.pilot_mob_vnum = 31810
   AND merchant.spawn_x = -599
   AND merchant.spawn_y = 455
   AND merchant.spawn_z = 0
   AND merchant.cargo_quantity = 40
   AND merchant.schedule_interval_hours = 1
   AND merchant.respawn_delay_seconds = 3600
   AND merchant.enabled = 1
   AND prototype.name = 'Vailand Merchant Cog'
   AND prototype.vessel_class = 2
   AND prototype.max_speed = 12
   AND prototype.armor = 30
   AND route.name = 'Vailand Iron Passage'
   AND commodity.name = 'iron';

SELECT 'campaign_market_gradient' AS check_name, COUNT(*) AS actual,
       2 AS expected
  FROM port_commodities AS market
  JOIN trade_commodities AS commodity
    ON commodity.commodity_id = market.commodity_id
 WHERE commodity.name = 'iron'
   AND ((market.port_vnum = 1000360 AND market.supply = 320)
     OR (market.port_vnum = 1000362 AND market.supply = 80));
