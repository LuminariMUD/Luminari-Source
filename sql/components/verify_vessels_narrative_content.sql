-- Read-only verification for vessels_narrative_content.sql.

SELECT 'vessel_narrative_hints' AS check_name, COUNT(*) AS actual,
       8 AS expected
  FROM region_hints
 WHERE region_vnum IN (1000013, 1000014, 1000015, 1000016)
   AND agent_id = 'vessel_narrative_v1';

SELECT 'vessel_narrative_regions' AS check_name,
       COUNT(DISTINCT region_vnum) AS actual, 4 AS expected
  FROM region_hints
 WHERE region_vnum IN (1000013, 1000014, 1000015, 1000016)
   AND agent_id = 'vessel_narrative_v1'
   AND is_active = 1;

SELECT 'vessel_narrative_all_weather' AS check_name, COUNT(*) AS actual,
       4 AS expected
  FROM region_hints
 WHERE region_vnum IN (1000013, 1000014, 1000015, 1000016)
   AND agent_id = 'vessel_narrative_v1'
   AND priority = 10
   AND weather_conditions = 'clear,cloudy,rainy,stormy,lightning'
   AND is_active = 1;

SELECT 'vessel_narrative_bad_rows' AS check_name, COUNT(*) AS actual,
       0 AS expected
  FROM region_hints
 WHERE region_vnum IN (1000013, 1000014, 1000015, 1000016)
   AND agent_id = 'vessel_narrative_v1'
   AND (
     is_active <> 1
     OR priority NOT IN (10, 11)
     OR (priority = 10 AND hint_category NOT IN ('atmosphere', 'landmarks'))
     OR (priority = 11 AND hint_category <> 'weather_influence')
     OR (priority = 11 AND weather_conditions <> 'rainy,stormy,lightning')
   );
