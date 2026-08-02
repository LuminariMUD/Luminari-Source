-- Read-only verification for wilderness frontier vessel content.

SELECT vnum, zone_vnum, name, region_type, region_props,
       ST_AsText(region_polygon) AS region_polygon
  FROM region_data
 WHERE vnum BETWEEN 7100101 AND 7100103
 ORDER BY vnum;

SELECT path.vnum, path.zone_vnum, path.path_type, path.name,
       path.path_props, ST_NumPoints(path.path_linestring) AS covered_cells,
       ST_AsText(path.path_linestring) AS path_linestring,
       IF(path_index.vnum IS NULL, 'MISSING', 'READY') AS spatial_index
  FROM path_data AS path
  LEFT JOIN path_index ON path_index.vnum = path.vnum
 WHERE path.vnum = 7100104;

SELECT prototype_id, name, vessel_class, max_speed, armor
  FROM ship_prototypes
 WHERE name IN (
   'Sablebranch Raft',
   'Sablebranch Riverboat',
   'Starfall Bathyscaphe',
   'Aetherwind Courier'
 )
 ORDER BY vessel_class;
