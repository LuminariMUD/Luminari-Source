-- Mosswood Region Sample Data (Region 1000004)
-- Complete sample data extracted from production database
-- This script inserts comprehensive test data for the narrative weaver system

-- ============================================================================
-- PART 1: Insert region_data with comprehensive description
-- ============================================================================

INSERT INTO region_data (
    vnum, zone_vnum, name, region_type, region_props, region_reset_data,
    region_description, description_style, description_length,
    has_historical_context, has_resource_info, has_wildlife_info, 
    has_geological_info, has_cultural_info, description_quality_score,
    is_approved, ai_agent_source
) VALUES (
    1000004, 10000, 'The Mosswood', 1, 0, '',
    'THE MOSSWOOD COMPREHENSIVE DESCRIPTION

OVERVIEW:
The Mosswood is an ancient, mystical forest characterized by towering moss-draped trees and an otherworldly atmosphere. Spanning roughly 8 square miles, this primeval woodland feels untouched by time, where thick carpets of emerald moss coat every surface and create an almost supernatural silence. The forest canopy filters sunlight into ethereal green-tinted beams that dance through perpetual mist.

GEOGRAPHY & TERRAIN:
- Elevation: 150-280 feet above sea level with gentle rolling hills
- Terrain: Soft, spongy moss-covered ground with hidden depressions
- Water features: Numerous small springs and moss-lined pools
- Rocky features: Ancient granite boulders covered in thick moss layers
- Soil: Rich, dark humus beneath deep moss carpets
- Microclimates: Cool, humid pockets with temperature variations

VEGETATION & FLORA:
Primary Canopy: Ancient oak and maple trees (600-1000 years old), reaching 80-100 feet
Moss Species: Dominant luminescent moss varieties creating the signature green glow
Understory: Scattered mountain laurel, wild rhododendron, ancient ferns
Ground Cover: Continuous moss carpet (6-12 inches thick), occasional mushroom rings
Notable Flora: Rare bioluminescent fungi, ghost flowers that bloom only in deep shade
Special Features: Some moss patches glow faintly in darkness, especially after rain

WILDLIFE & FAUNA:
Large Mammals: Elusive deer with unusually silent movement, occasional bears
Small Mammals: Moss-colored rabbits, squirrels with adapted climbing for moss-covered trees
Birds: Quiet songbirds, occasional owls, woodpeckers adapted to moss-covered bark
Insects: Fireflies, unique beetles that feed on moss, silent moths
Mystical Creatures: Local legends speak of moss spirits and will-o-wisps
Behavioral Notes: Animals move unusually quietly due to soft moss surfaces

ATMOSPHERIC CHARACTERISTICS:
Sound: Profound natural silence, muffled footsteps, soft dripping, whispered wind
Scents: Rich earthy moss, clean humid air, subtle mushroom fragrances
Lighting: Filtered green-tinted light, soft ethereal quality, occasional light shafts
Weather Effects: Mist clings to moss, rain creates gentle trickling sounds
Mystical Elements: Occasional unexplained glowing patches, sense of ancient presence

SEASONAL VARIATIONS:
Spring: New moss growth brightens to vivid green, rare flowers bloom
Summer: Deep shade keeps temperatures cool, bioluminescent fungi most active
Autumn: Moss takes on golden-green hues, fallen leaves quickly absorbed
Winter: Snow creates striking contrast with green moss, ethereal winter beauty

RESOURCE AVAILABILITY:
- Medicinal moss varieties with healing properties
- Pure spring water from moss-filtered sources
- Rare fungi with alchemical properties
- Soft moss fibers suitable for bedding and insulation
- Clay deposits beneath moss layers in certain areas

CULTURAL & MYSTICAL ELEMENTS:
- Ancient druidic circles hidden beneath moss mounds
- Local folklore of moss guardians who protect the forest
- Sacred groves where moss glows more brightly
- Legends of travelers who became lost in time among the moss
- Whispered stories of healing springs hidden in the deepest parts',
    'mysterious', 'extensive',
    TRUE, TRUE, TRUE, TRUE, TRUE, 4.75,
    TRUE, 'sample_data_generator'
) ON DUPLICATE KEY UPDATE
    region_description = VALUES(region_description),
    description_version = description_version + 1,
    last_description_update = CURRENT_TIMESTAMP,
    ai_agent_source = VALUES(ai_agent_source);

-- ============================================================================
-- PART 2: Insert region profile
-- ============================================================================

INSERT INTO region_profiles (
    region_vnum, overall_theme, dominant_mood, key_characteristics,
    description_style, complexity_level, agent_id
) VALUES (
    1000004,
    'An ancient, moss-covered mystical forest where silence reigns and bioluminescent moss creates an otherworldly atmosphere. The Mosswood represents primeval nature untouched by civilization, where every surface is draped in thick emerald moss that muffles sound and filters light into ethereal green hues.',
    'mystical_tranquility',
    '{"atmosphere": ["ethereal", "timeless", "sacred", "peaceful"], "weather_effects": ["mist_enhancement", "sound_dampening", "humidity_dependent"], "primary_features": ["moss-covered_everything", "supernatural_silence", "green_filtered_light", "ancient_trees"], "mystical_elements": ["glowing_moss", "lost_time_legends", "druidic_circles", "moss_spirits"], "wildlife_behavior": ["unusually_quiet", "adapted_to_moss", "elusive_movement"]}',
    'mysterious',
    5,
    'sample_data_generator'
) ON DUPLICATE KEY UPDATE
    overall_theme = VALUES(overall_theme),
    dominant_mood = VALUES(dominant_mood),
    key_characteristics = VALUES(key_characteristics),
    updated_at = CURRENT_TIMESTAMP;

-- ============================================================================
-- PART 3: Insert region hints (19 total)
-- ============================================================================

-- Atmosphere hints (3 total)
INSERT INTO region_hints (region_vnum, hint_category, hint_text, priority, seasonal_weight, weather_conditions, time_of_day_weight, agent_id) VALUES
(1000004, 'atmosphere', 'The profound silence of the moss-covered forest creates an almost sacred atmosphere, where even your footsteps are muffled by the thick emerald carpet beneath your feet.', 8, '{"autumn": 0.8, "spring": 0.9, "summer": 1.0, "winter": 0.7}', 'clear,cloudy', '{"dawn": 1.0, "night": 0.9, "midday": 0.7, "evening": 1.0, "morning": 0.9, "afternoon": 0.8}', 'sample_data_generator'),
(1000004, 'atmosphere', 'An ethereal green glow emanates from the bioluminescent moss patches, creating an otherworldly ambiance that seems to pulse gently with the rhythm of the ancient forest.', 9, '{"autumn": 0.9, "spring": 0.8, "summer": 1.0, "winter": 0.6}', 'clear,cloudy,rainy', '{"dawn": 0.6, "night": 1.0, "midday": 0.3, "evening": 0.9, "morning": 0.4, "afternoon": 0.5}', 'sample_data_generator'),
(1000004, 'atmosphere', 'The mist that perpetually drifts between the moss-draped trees gives the forest a dreamlike quality, as if you have stepped into a realm outside of normal time.', 7, '{"autumn": 1.0, "spring": 1.0, "summer": 0.8, "winter": 0.9}', 'cloudy,rainy', '{"dawn": 1.0, "night": 0.7, "midday": 0.5, "evening": 0.8, "morning": 0.9, "afternoon": 0.6}', 'sample_data_generator');

-- Flora hints (3 total)
INSERT INTO region_hints (region_vnum, hint_category, hint_text, priority, seasonal_weight, weather_conditions, time_of_day_weight, agent_id) VALUES
(1000004, 'flora', 'Every tree trunk and branch is completely enveloped in thick, velvety moss that ranges from deep emerald to bright jade, creating a monochromatic yet richly textured landscape.', 8, '{"autumn": 0.8, "spring": 1.0, "summer": 0.9, "winter": 0.7}', 'clear,cloudy,rainy', '{"dawn": 0.8, "night": 0.5, "midday": 1.0, "evening": 0.7, "morning": 1.0, "afternoon": 0.9}', 'sample_data_generator'),
(1000004, 'flora', 'Delicate ghost flowers peek through the moss carpet, their translucent white petals seeming to glow with an inner light in the perpetual green twilight of the forest.', 6, '{"autumn": 0.4, "spring": 1.0, "summer": 0.7, "winter": 0.1}', 'clear,cloudy', '{"dawn": 0.8, "night": 0.8, "midday": 0.6, "evening": 1.0, "morning": 0.9, "afternoon": 0.7}', 'sample_data_generator'),
(1000004, 'flora', 'Clusters of luminescent fungi grow in perfect fairy rings among the moss, their soft blue-green glow providing natural waypoints through the otherwise uniform carpet of green.', 7, '{"autumn": 0.8, "spring": 0.6, "summer": 1.0, "winter": 0.3}', 'cloudy,rainy', '{"dawn": 0.5, "night": 1.0, "midday": 0.2, "evening": 0.9, "morning": 0.3, "afternoon": 0.4}', 'sample_data_generator');

-- Fauna hints (3 total)
INSERT INTO region_hints (region_vnum, hint_category, hint_text, priority, seasonal_weight, weather_conditions, time_of_day_weight, agent_id) VALUES
(1000004, 'fauna', 'A moss-colored rabbit freezes motionless against a tree trunk, its natural camouflage making it nearly invisible until it bounds away with surprising silence across the soft moss floor.', 6, '{"autumn": 0.9, "spring": 1.0, "summer": 0.8, "winter": 0.7}', 'clear,cloudy', '{"dawn": 1.0, "night": 0.4, "midday": 0.6, "evening": 0.8, "morning": 0.9, "afternoon": 0.7}', 'sample_data_generator'),
(1000004, 'fauna', 'High above, an owl calls softly, its voice strangely muffled by the moss-laden branches, while fireflies begin their nightly dance among the glowing fungi.', 5, '{"autumn": 0.8, "spring": 0.7, "summer": 1.0, "winter": 0.3}', 'clear,cloudy', '{"dawn": 0.3, "night": 1.0, "midday": 0.1, "evening": 0.9, "morning": 0.2, "afternoon": 0.2}', 'sample_data_generator'),
(1000004, 'fauna', 'The distinctive tap-tap-tapping of a woodpecker echoes through the forest, the bird specially adapted to find insects in the moss-covered bark of the ancient trees.', 4, '{"autumn": 0.6, "spring": 1.0, "summer": 0.8, "winter": 0.4}', 'clear,cloudy', '{"dawn": 0.8, "night": 0.1, "midday": 0.9, "evening": 0.4, "morning": 1.0, "afternoon": 0.7}', 'sample_data_generator');

-- Weather influence hints (3 total)
INSERT INTO region_hints (region_vnum, hint_category, hint_text, priority, seasonal_weight, weather_conditions, time_of_day_weight, agent_id) VALUES
(1000004, 'weather_influence', 'Gentle rain creates a symphony of soft trickling sounds as water cascades from moss to moss, each layer acting like a natural sponge to filter the precipitation.', 7, '{"autumn": 1.0, "spring": 1.0, "summer": 0.9, "winter": 0.6}', 'rainy', '{"dawn": 0.8, "night": 0.7, "midday": 1.0, "evening": 0.9, "morning": 1.0, "afternoon": 1.0}', 'sample_data_generator'),
(1000004, 'weather_influence', 'The misty air clings to the moss-covered surfaces, creating tiny water droplets that sparkle like jewels in the filtered green light of the forest.', 6, '{"autumn": 1.0, "spring": 1.0, "summer": 0.8, "winter": 0.9}', 'cloudy,rainy', '{"dawn": 1.0, "night": 0.8, "midday": 0.6, "evening": 0.8, "morning": 0.9, "afternoon": 0.7}', 'sample_data_generator'),
(1000004, 'weather_influence', 'On this clear day, rare shafts of golden sunlight penetrate the canopy, illuminating patches of moss that seem to glow like emerald fire against the perpetual green twilight.', 8, '{"autumn": 0.8, "spring": 0.9, "summer": 1.0, "winter": 0.7}', 'clear', '{"dawn": 0.7, "night": 0.2, "midday": 1.0, "evening": 0.6, "morning": 1.0, "afternoon": 0.9}', 'sample_data_generator');

-- Sound hints (2 total)
INSERT INTO region_hints (region_vnum, hint_category, hint_text, priority, seasonal_weight, weather_conditions, time_of_day_weight, agent_id) VALUES
(1000004, 'sounds', 'The forest maintains an almost supernatural silence, with only the softest whisper of wind through moss-laden branches and the distant, muffled drip of moisture.', 8, '{"autumn": 0.9, "spring": 0.9, "summer": 1.0, "winter": 1.0}', 'clear,cloudy', '{"dawn": 1.0, "night": 1.0, "midday": 1.0, "evening": 1.0, "morning": 0.9, "afternoon": 1.0}', 'sample_data_generator'),
(1000004, 'sounds', 'Your own breathing seems unnaturally loud in this hushed cathedral of moss and ancient wood, where even the smallest sound carries a reverent quality.', 6, '{"autumn": 0.9, "spring": 0.8, "summer": 1.0, "winter": 1.0}', 'clear,cloudy', '{"dawn": 1.0, "night": 1.0, "midday": 0.9, "evening": 1.0, "morning": 0.8, "afternoon": 0.8}', 'sample_data_generator');

-- Scent hints (1 total)
INSERT INTO region_hints (region_vnum, hint_category, hint_text, priority, seasonal_weight, weather_conditions, time_of_day_weight, agent_id) VALUES
(1000004, 'scents', 'The rich, earthy aroma of ancient moss mingles with the clean scent of filtered moisture and the subtle, mushroom-like fragrance of the luminescent fungi.', 7, '{"autumn": 1.0, "spring": 1.0, "summer": 0.9, "winter": 0.8}', 'cloudy,rainy', '{"dawn": 0.9, "night": 0.9, "midday": 0.8, "evening": 1.0, "morning": 1.0, "afternoon": 0.9}', 'sample_data_generator');

-- Seasonal change hints (2 total)
INSERT INTO region_hints (region_vnum, hint_category, hint_text, priority, seasonal_weight, weather_conditions, time_of_day_weight, agent_id) VALUES
(1000004, 'seasonal_changes', 'Fresh spring growth has brightened the moss to a vivid emerald green, while delicate new shoots push through the ancient carpet with determined vitality.', 6, '{"autumn": 0.1, "spring": 1.0, "summer": 0.1, "winter": 0.1}', 'clear,cloudy,rainy', '{"dawn": 0.8, "night": 0.4, "midday": 0.9, "evening": 0.6, "morning": 1.0, "afternoon": 0.8}', 'sample_data_generator'),
(1000004, 'seasonal_changes', 'The deep summer shade keeps the forest surprisingly cool, while the bioluminescent fungi reach their peak activity, creating an underwater-like luminescence.', 7, '{"autumn": 0.1, "spring": 0.1, "summer": 1.0, "winter": 0.1}', 'clear,cloudy', '{"dawn": 0.6, "night": 0.9, "midday": 1.0, "evening": 0.8, "morning": 0.7, "afternoon": 1.0}', 'sample_data_generator');

-- Time of day hints (1 total)
INSERT INTO region_hints (region_vnum, hint_category, hint_text, priority, seasonal_weight, weather_conditions, time_of_day_weight, agent_id) VALUES
(1000004, 'time_of_day', 'In the pre-dawn darkness, the moss seems to pulse with its own inner light, creating a natural constellation across the forest floor that guides your way.', 8, '{"autumn": 0.9, "spring": 0.8, "summer": 1.0, "winter": 0.7}', 'clear,cloudy', '{"dawn": 1.0, "night": 0.9, "midday": 0.1, "evening": 0.3, "morning": 0.2, "afternoon": 0.1}', 'sample_data_generator');

-- Mystical hints (1 total)
INSERT INTO region_hints (region_vnum, hint_category, hint_text, priority, seasonal_weight, weather_conditions, time_of_day_weight, agent_id) VALUES
(1000004, 'mystical', 'An ancient stone circle emerges from beneath a mound of particularly thick moss, its weathered surfaces inscribed with symbols that seem to shift in the ethereal light.', 9, '{"autumn": 1.0, "spring": 0.8, "summer": 0.9, "winter": 0.7}', 'clear,cloudy', '{"dawn": 0.9, "night": 0.8, "midday": 0.5, "evening": 1.0, "morning": 0.7, "afternoon": 0.6}', 'sample_data_generator');

-- ============================================================================
-- PART 4: Verification and summary
-- ============================================================================

SELECT 'Mosswood region (1000004) sample data inserted successfully!' as status;
SELECT COUNT(*) as hint_count FROM region_hints WHERE region_vnum = 1000004;
SELECT COUNT(*) as profile_count FROM region_profiles WHERE region_vnum = 1000004;
SELECT 
    vnum,
    name,
    description_style,
    description_length,
    description_quality_score,
    is_approved,
    ai_agent_source
FROM region_data 
WHERE vnum = 1000004;
