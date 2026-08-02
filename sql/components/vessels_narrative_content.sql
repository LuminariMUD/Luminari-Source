-- Vessel dynamic narrative content for the four canonical Vailand waters.
--
-- Apply after vessels_campaign_content.sql has created the geographic
-- regions. Rows are owned by the vessel_narrative_v1 agent id so verification
-- and rollback never need to match or alter unrelated regional prose.

INSERT INTO region_hints
  (region_vnum, hint_category, hint_text, priority, weather_conditions,
   agent_id, is_active)
SELECT 1000013, 'atmosphere',
       'Cold northern currents comb long silver paths through North Vailand waters.',
       10, 'clear,cloudy,rainy,stormy,lightning',
       'vessel_narrative_v1', 1
 WHERE NOT EXISTS (
   SELECT 1 FROM region_hints
    WHERE region_vnum = 1000013
      AND agent_id = 'vessel_narrative_v1'
      AND hint_text =
          'Cold northern currents comb long silver paths through North Vailand waters.'
 );

INSERT INTO region_hints
  (region_vnum, hint_category, hint_text, priority, weather_conditions,
   agent_id, is_active)
SELECT 1000013, 'weather_influence',
       'Rain and hard wind close around the guarded approaches of North Vailand.',
       11, 'rainy,stormy,lightning', 'vessel_narrative_v1', 1
 WHERE NOT EXISTS (
   SELECT 1 FROM region_hints
    WHERE region_vnum = 1000013
      AND agent_id = 'vessel_narrative_v1'
      AND hint_text =
          'Rain and hard wind close around the guarded approaches of North Vailand.'
 );

INSERT INTO region_hints
  (region_vnum, hint_category, hint_text, priority, weather_conditions,
   agent_id, is_active)
SELECT 1000014, 'atmosphere',
       'Low green headlands frame the trade lanes of Central Vailand waters.',
       10, 'clear,cloudy,rainy,stormy,lightning',
       'vessel_narrative_v1', 1
 WHERE NOT EXISTS (
   SELECT 1 FROM region_hints
    WHERE region_vnum = 1000014
      AND agent_id = 'vessel_narrative_v1'
      AND hint_text =
          'Low green headlands frame the trade lanes of Central Vailand waters.'
 );

INSERT INTO region_hints
  (region_vnum, hint_category, hint_text, priority, weather_conditions,
   agent_id, is_active)
SELECT 1000014, 'weather_influence',
       'Driven rain blurs the headlands and shipping lanes of Central Vailand.',
       11, 'rainy,stormy,lightning', 'vessel_narrative_v1', 1
 WHERE NOT EXISTS (
   SELECT 1 FROM region_hints
    WHERE region_vnum = 1000014
      AND agent_id = 'vessel_narrative_v1'
      AND hint_text =
          'Driven rain blurs the headlands and shipping lanes of Central Vailand.'
 );

INSERT INTO region_hints
  (region_vnum, hint_category, hint_text, priority, weather_conditions,
   agent_id, is_active)
SELECT 1000015, 'atmosphere',
       'The broad Vailand Passage draws a dark blue road between the island coasts.',
       10, 'clear,cloudy,rainy,stormy,lightning',
       'vessel_narrative_v1', 1
 WHERE NOT EXISTS (
   SELECT 1 FROM region_hints
    WHERE region_vnum = 1000015
      AND agent_id = 'vessel_narrative_v1'
      AND hint_text =
          'The broad Vailand Passage draws a dark blue road between the island coasts.'
 );

INSERT INTO region_hints
  (region_vnum, hint_category, hint_text, priority, weather_conditions,
   agent_id, is_active)
SELECT 1000015, 'weather_influence',
       'Storm-driven swells run the length of Vailand Passage in marching ranks.',
       11, 'rainy,stormy,lightning', 'vessel_narrative_v1', 1
 WHERE NOT EXISTS (
   SELECT 1 FROM region_hints
    WHERE region_vnum = 1000015
      AND agent_id = 'vessel_narrative_v1'
      AND hint_text =
          'Storm-driven swells run the length of Vailand Passage in marching ranks.'
 );

INSERT INTO region_hints
  (region_vnum, hint_category, hint_text, priority, weather_conditions,
   agent_id, is_active)
SELECT 1000016, 'landmarks',
       'Tar-dark pilings and crowded masts mark Blackwake Anchorage ahead.',
       10, 'clear,cloudy,rainy,stormy,lightning',
       'vessel_narrative_v1', 1
 WHERE NOT EXISTS (
   SELECT 1 FROM region_hints
    WHERE region_vnum = 1000016
      AND agent_id = 'vessel_narrative_v1'
      AND hint_text =
          'Tar-dark pilings and crowded masts mark Blackwake Anchorage ahead.'
 );

INSERT INTO region_hints
  (region_vnum, hint_category, hint_text, priority, weather_conditions,
   agent_id, is_active)
SELECT 1000016, 'weather_influence',
       'Rain veils Blackwake Anchorage while bells carry across the crowded water.',
       11, 'rainy,stormy,lightning', 'vessel_narrative_v1', 1
 WHERE NOT EXISTS (
   SELECT 1 FROM region_hints
    WHERE region_vnum = 1000016
      AND agent_id = 'vessel_narrative_v1'
      AND hint_text =
          'Rain veils Blackwake Anchorage while bells carry across the crowded water.'
 );

UPDATE region_hints
   SET priority = CASE
         WHEN hint_text IN (
           'Cold northern currents comb long silver paths through North Vailand waters.',
           'Low green headlands frame the trade lanes of Central Vailand waters.',
           'The broad Vailand Passage draws a dark blue road between the island coasts.',
           'Tar-dark pilings and crowded masts mark Blackwake Anchorage ahead.'
         ) THEN 10
         ELSE 11
       END,
       hint_category = CASE
         WHEN hint_text =
              'Tar-dark pilings and crowded masts mark Blackwake Anchorage ahead.'
           THEN 'landmarks'
         WHEN hint_text IN (
           'Cold northern currents comb long silver paths through North Vailand waters.',
           'Low green headlands frame the trade lanes of Central Vailand waters.',
           'The broad Vailand Passage draws a dark blue road between the island coasts.'
         ) THEN 'atmosphere'
         ELSE 'weather_influence'
       END,
       weather_conditions = CASE
         WHEN hint_text IN (
           'Cold northern currents comb long silver paths through North Vailand waters.',
           'Low green headlands frame the trade lanes of Central Vailand waters.',
           'The broad Vailand Passage draws a dark blue road between the island coasts.',
           'Tar-dark pilings and crowded masts mark Blackwake Anchorage ahead.'
         ) THEN 'clear,cloudy,rainy,stormy,lightning'
         ELSE 'rainy,stormy,lightning'
       END,
       is_active = 1
 WHERE region_vnum IN (1000013, 1000014, 1000015, 1000016)
   AND agent_id = 'vessel_narrative_v1'
   AND hint_text IN (
     'Cold northern currents comb long silver paths through North Vailand waters.',
     'Rain and hard wind close around the guarded approaches of North Vailand.',
     'Low green headlands frame the trade lanes of Central Vailand waters.',
     'Driven rain blurs the headlands and shipping lanes of Central Vailand.',
     'The broad Vailand Passage draws a dark blue road between the island coasts.',
     'Storm-driven swells run the length of Vailand Passage in marching ranks.',
     'Tar-dark pilings and crowded masts mark Blackwake Anchorage ahead.',
     'Rain veils Blackwake Anchorage while bells carry across the crowded water.'
   );
