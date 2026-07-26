-- Vessel System Phase 08 verification.

SELECT COUNT(*) AS table_present
  FROM information_schema.TABLES
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'vessel_encounters';

SELECT COUNT(*) AS encounter_definitions FROM vessel_encounters;

-- Every encounter must key to a real REGION_ENCOUNTER region.
-- Any rows returned here are misconfigured (dangling or wrong-type region).
SELECT ve.encounter_id, ve.region_vnum, ve.name
  FROM vessel_encounters ve
  LEFT JOIN region_data rd ON rd.vnum = ve.region_vnum
 WHERE rd.vnum IS NULL OR rd.region_type <> 2;

-- Chance values outside 1..100 would never fire or always fire
SELECT encounter_id, name, chance
  FROM vessel_encounters
 WHERE chance < 1 OR chance > 100;
