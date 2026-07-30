-- Vessel System Phase 13 verification.

SELECT COUNT(*) AS vessel_region_law_table_present
  FROM information_schema.TABLES
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'vessel_region_law';

SELECT COUNT(*) AS vessel_region_law_columns_present
  FROM information_schema.COLUMNS
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'vessel_region_law'
   AND COLUMN_NAME IN (
     'region_vnum',
     'waters_type',
     'priority',
     'bounty_percent',
     'authority'
   );

SELECT region_vnum, waters_type, bounty_percent
  FROM vessel_region_law
 WHERE waters_type NOT BETWEEN 1 AND 3
    OR bounty_percent NOT BETWEEN 0 AND 500;

SELECT law.region_vnum, region.name, region.region_type
  FROM vessel_region_law AS law
  LEFT JOIN region_data AS region ON region.vnum = law.region_vnum
 WHERE region.vnum IS NULL
    OR region.region_type <> 1;

SELECT law.region_vnum, region.name, law.waters_type, law.priority,
       law.bounty_percent, law.authority
  FROM vessel_region_law AS law
  JOIN region_data AS region ON region.vnum = law.region_vnum
 ORDER BY law.priority DESC, law.region_vnum;
