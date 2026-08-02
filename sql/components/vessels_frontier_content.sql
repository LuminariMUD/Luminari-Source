-- Vessel System 3.0: wilderness frontier content.
--
-- Region types 5 through 7 are interpreted by vessel code while retaining
-- region_data as the canonical geometry. The trench threshold is natural
-- water-column depth; altitude thresholds are vessel Z coordinates. The river
-- is a normal path_data feature whose path_props converts covered cells to
-- SECT_RIVER (36). The existing path_data trigger digitalizes its sparse
-- vertices into contiguous wilderness cells with bresenham_line().

START TRANSACTION;

SET @starfall_trench_vnum = 7100101;
SET @aetherwind_skyway_vnum = 7100102;
SET @shardspire_island_vnum = 7100103;
SET @sablebranch_river_vnum = 7100104;

INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT @starfall_trench_vnum, 10000, 'Starfall Trench', 5,
       ST_GeomFromText(
         'POLYGON((896 221,904 221,904 229,896 229,896 221))'
       ),
       96, '', '1970-01-01 00:00:00'
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data
    WHERE vnum = @starfall_trench_vnum
       OR name = 'Starfall Trench'
 );

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 5,
       region_polygon = ST_GeomFromText(
         'POLYGON((896 221,904 221,904 229,896 229,896 221))'
       ),
       region_props = 96,
       region_reset_data = '',
       region_reset_time = '1970-01-01 00:00:00'
 WHERE vnum = @starfall_trench_vnum
   AND name = 'Starfall Trench';

INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT @aetherwind_skyway_vnum, 10000, 'Aetherwind Skyway', 6,
       ST_GeomFromText(
         'POLYGON((430 -2,475 -2,475 2,430 2,430 -2))'
       ),
       100, '', '1970-01-01 00:00:00'
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data
    WHERE vnum = @aetherwind_skyway_vnum
       OR name = 'Aetherwind Skyway'
 );

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 6,
       region_polygon = ST_GeomFromText(
         'POLYGON((430 -2,475 -2,475 2,430 2,430 -2))'
       ),
       region_props = 100,
       region_reset_data = '',
       region_reset_time = '1970-01-01 00:00:00'
 WHERE vnum = @aetherwind_skyway_vnum
   AND name = 'Aetherwind Skyway';

INSERT INTO region_data
  (vnum, zone_vnum, name, region_type, region_polygon, region_props,
   region_reset_data, region_reset_time)
SELECT @shardspire_island_vnum, 10000, 'Shardspire Sky Island', 7,
       ST_GeomFromText(
         'POLYGON((468 -2,474 -2,474 2,468 2,468 -2))'
       ),
       200, '', '1970-01-01 00:00:00'
 WHERE NOT EXISTS (
   SELECT 1 FROM region_data
    WHERE vnum = @shardspire_island_vnum
       OR name = 'Shardspire Sky Island'
 );

UPDATE region_data
   SET zone_vnum = 10000,
       region_type = 7,
       region_polygon = ST_GeomFromText(
         'POLYGON((468 -2,474 -2,474 2,468 2,468 -2))'
       ),
       region_props = 200,
       region_reset_data = '',
       region_reset_time = '1970-01-01 00:00:00'
 WHERE vnum = @shardspire_island_vnum
   AND name = 'Shardspire Sky Island';

INSERT INTO path_data
  (vnum, zone_vnum, path_type, name, path_props, path_linestring)
SELECT @sablebranch_river_vnum, 10000, 5, 'Sablebranch River', 36,
       ST_GeomFromText(
         'LINESTRING(-819 480,-780 480,-780 519)'
       )
 WHERE NOT EXISTS (
   SELECT 1 FROM path_data
    WHERE vnum = @sablebranch_river_vnum
       OR name = 'Sablebranch River'
 );

UPDATE path_data
   SET zone_vnum = 10000,
       path_type = 5,
       path_props = 36,
       path_linestring = ST_GeomFromText(
         'LINESTRING(-819 480,-780 480,-780 519)'
       )
 WHERE vnum = @sablebranch_river_vnum
   AND name = 'Sablebranch River';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Sablebranch Raft', 0, 10, 5
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Sablebranch Raft'
 );
UPDATE ship_prototypes
   SET vessel_class = 0, max_speed = 10, armor = 5
 WHERE name = 'Sablebranch Raft';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Sablebranch Riverboat', 1, 10, 8
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Sablebranch Riverboat'
 );
UPDATE ship_prototypes
   SET vessel_class = 1, max_speed = 10, armor = 8
 WHERE name = 'Sablebranch Riverboat';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Starfall Survey Ship', 2, 12, 20
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Starfall Survey Ship'
 );
UPDATE ship_prototypes
   SET vessel_class = 2, max_speed = 12, armor = 20
 WHERE name = 'Starfall Survey Ship';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Starfall Bastion', 3, 15, 35
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Starfall Bastion'
 );
UPDATE ship_prototypes
   SET vessel_class = 3, max_speed = 15, armor = 35
 WHERE name = 'Starfall Bastion';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Aetherwind Courier', 4, 25, 15
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Aetherwind Courier'
 );
UPDATE ship_prototypes
   SET vessel_class = 4, max_speed = 25, armor = 15
 WHERE name = 'Aetherwind Courier';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Starfall Bathyscaphe', 5, 10, 25
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Starfall Bathyscaphe'
 );
UPDATE ship_prototypes
   SET vessel_class = 5, max_speed = 10, armor = 25
 WHERE name = 'Starfall Bathyscaphe';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Sablebranch Grand Freighter', 6, 8, 20
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Sablebranch Grand Freighter'
 );
UPDATE ship_prototypes
   SET vessel_class = 6, max_speed = 8, armor = 20
 WHERE name = 'Sablebranch Grand Freighter';

INSERT INTO ship_prototypes (name, vessel_class, max_speed, armor)
SELECT 'Liminal Wayfarer', 7, 15, 20
 WHERE NOT EXISTS (
   SELECT 1 FROM ship_prototypes WHERE name = 'Liminal Wayfarer'
 );
UPDATE ship_prototypes
   SET vessel_class = 7, max_speed = 15, armor = 20
 WHERE name = 'Liminal Wayfarer';

COMMIT;
