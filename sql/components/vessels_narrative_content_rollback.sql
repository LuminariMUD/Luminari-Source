-- Remove only the Vailand hints owned by vessels_narrative_content.sql.

DELETE FROM region_hints
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
