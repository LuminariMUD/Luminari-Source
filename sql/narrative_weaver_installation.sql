-- =====================================================================
-- Narrative Weaver System Installation Script
-- =====================================================================
-- This script installs the complete narrative weaver system for dynamic
-- region descriptions. It creates all necessary tables, indexes, views,
-- and sample data for the AI-generated region hints system.
--
-- Prerequisites:
-- - region_data table must exist (part of the wilderness system)
-- - MySQL 5.7+ for JSON support
--
-- Usage: mysql -u root luminari_mudprod < narrative_weaver_installation.sql
-- =====================================================================

USE luminari_mudprod;

-- Check if region_data table exists (prerequisite)
SELECT COUNT(*) as region_data_exists FROM information_schema.tables 
WHERE table_schema = 'luminari_mudprod' AND table_name = 'region_data';

-- =====================================================================
-- PART 1: CORE NARRATIVE WEAVER TABLES
-- =====================================================================

-- Main table for AI-generated region hints
CREATE TABLE IF NOT EXISTS region_hints (
    id INT AUTO_INCREMENT PRIMARY KEY,
    region_vnum INT NOT NULL,
    hint_category ENUM(
        'atmosphere',        -- General atmospheric descriptions
        'fauna',            -- Animal life descriptions
        'flora',            -- Plant life descriptions  
        'geography',        -- Landform and geological features
        'weather_influence', -- How weather affects this region
        'resources',        -- Resource availability hints
        'landmarks',        -- Notable landmarks or features
        'sounds',           -- Ambient sounds
        'scents',           -- Ambient smells
        'seasonal_changes', -- How the region changes with seasons
        'time_of_day',      -- Day/night variations
        'mystical'          -- Magical or mystical elements
    ) NOT NULL,
    hint_text TEXT NOT NULL,
    priority TINYINT DEFAULT 5,  -- 1-10, higher = more likely to be used
    seasonal_weight JSON DEFAULT NULL,  -- {"spring": 1.0, "summer": 1.2, "autumn": 0.8, "winter": 0.5}
    weather_conditions SET('clear', 'cloudy', 'rainy', 'stormy', 'lightning') DEFAULT 'clear,cloudy,rainy,stormy,lightning',
    time_of_day_weight JSON DEFAULT NULL,  -- {"dawn": 1.0, "day": 1.0, "dusk": 1.2, "night": 0.8}
    resource_triggers JSON DEFAULT NULL,  -- {"vegetation": ">0.7", "water": "<0.3"} - when to use this hint
    ai_agent_id VARCHAR(100) DEFAULT NULL,  -- Which AI agent created this hint
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    
    FOREIGN KEY (region_vnum) REFERENCES region_data(vnum) ON DELETE CASCADE,
    INDEX idx_region_category (region_vnum, hint_category),
    INDEX idx_priority (priority),
    INDEX idx_active (is_active),
    INDEX idx_created (created_at)
);

-- Track which hints are actually used in descriptions for analytics
CREATE TABLE IF NOT EXISTS hint_usage_log (
    id INT AUTO_INCREMENT PRIMARY KEY,
    hint_id INT NOT NULL,
    room_vnum INT NOT NULL,
    player_id INT DEFAULT NULL,  -- If tracking per-player
    used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    weather_condition VARCHAR(20),
    season VARCHAR(10),
    time_of_day VARCHAR(10),
    resource_state JSON DEFAULT NULL,  -- Snapshot of resource levels when used
    
    FOREIGN KEY (hint_id) REFERENCES region_hints(id) ON DELETE CASCADE,
    INDEX idx_hint_usage (hint_id, used_at),
    INDEX idx_room_usage (room_vnum, used_at)
);

-- AI-generated personality profiles for regions (overall character/theme)
CREATE TABLE IF NOT EXISTS region_profiles (
    region_vnum INT PRIMARY KEY,
    overall_theme TEXT,           -- "Ancient mystical forest with elven influences"
    dominant_mood VARCHAR(100),   -- "Serene, mysterious, slightly melancholic"
    key_characteristics JSON,     -- ["ancient_trees", "elven_ruins", "magical_springs"]
    description_style ENUM('poetic', 'practical', 'mysterious', 'dramatic', 'pastoral') DEFAULT 'poetic',
    complexity_level TINYINT DEFAULT 3,  -- 1-5, how detailed descriptions should be
    ai_agent_id VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    FOREIGN KEY (region_vnum) REFERENCES region_data(vnum) ON DELETE CASCADE
);

-- AI-generated description templates that can be filled with dynamic data
CREATE TABLE IF NOT EXISTS description_templates (
    id INT AUTO_INCREMENT PRIMARY KEY,
    region_vnum INT NOT NULL,
    template_type ENUM('intro', 'weather_overlay', 'resource_state', 'time_transition') NOT NULL,
    template_text TEXT NOT NULL,  -- "The {adjective} {terrain} stretches {direction}, where {resource_hint}"
    placeholder_schema JSON,      -- {"adjective": "atmospheric_word", "terrain": "sector_type"}
    conditions JSON DEFAULT NULL, -- When this template should be used
    ai_agent_id VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    
    FOREIGN KEY (region_vnum) REFERENCES region_data(vnum) ON DELETE CASCADE,
    INDEX idx_region_type (region_vnum, template_type),
    INDEX idx_active_templates (is_active)
);

-- =====================================================================
-- PART 2: PERFORMANCE OPTIMIZATION VIEWS
-- =====================================================================

-- View for quick lookup of active hints by region and category
CREATE VIEW IF NOT EXISTS active_region_hints AS
SELECT 
    rh.id,
    rh.region_vnum,
    rh.hint_category,
    rh.hint_text,
    rh.priority,
    rh.weather_conditions,
    rh.seasonal_weight,
    rh.time_of_day_weight,
    rh.resource_triggers,
    rp.description_style,
    rp.complexity_level
FROM region_hints rh
LEFT JOIN region_profiles rp ON rh.region_vnum = rp.region_vnum
WHERE rh.is_active = TRUE
ORDER BY rh.region_vnum, rh.priority DESC;

-- View for hint usage analytics
CREATE VIEW IF NOT EXISTS hint_analytics AS
SELECT 
    rh.region_vnum,
    rh.hint_category,
    COUNT(hul.id) as usage_count,
    AVG(rh.priority) as avg_priority,
    MAX(hul.used_at) as last_used,
    COUNT(DISTINCT hul.room_vnum) as unique_rooms
FROM region_hints rh
LEFT JOIN hint_usage_log hul ON rh.id = hul.hint_id
WHERE rh.is_active = TRUE
GROUP BY rh.region_vnum, rh.hint_category;

-- =====================================================================
-- PART 3: SAMPLE DATA FOR TESTING AND DEMONSTRATION
-- =====================================================================

-- Insert sample region hints for testing (using region vnum 1001 as example)
-- Note: These will only insert if region 1001 exists in region_data
INSERT IGNORE INTO region_hints (region_vnum, hint_category, hint_text, priority, weather_conditions) 
SELECT * FROM (
    SELECT 1001 as region_vnum, 'atmosphere' as hint_category, 'Ancient oak trees tower overhead, their gnarled branches creating a natural cathedral of green.' as hint_text, 8 as priority, 'clear,cloudy' as weather_conditions
    UNION SELECT 1001, 'atmosphere', 'Mist clings to the forest floor, giving the woodland an ethereal, dreamlike quality.', 6, 'cloudy,rainy'
    UNION SELECT 1001, 'fauna', 'Squirrels chatter in the canopy while deer paths wind between the massive tree trunks.', 7, 'clear,cloudy'
    UNION SELECT 1001, 'flora', 'Thick carpets of moss cover fallen logs, and delicate wildflowers bloom in dappled clearings.', 7, 'clear,cloudy,rainy'
    UNION SELECT 1001, 'sounds', 'The gentle rustle of leaves mingles with distant bird calls and the soft trickle of hidden streams.', 6, 'clear,cloudy'
    UNION SELECT 1001, 'scents', 'The air carries the rich scent of earth and growing things, tinged with the sweetness of wild honeysuckle.', 5, 'clear,cloudy'
) AS sample_hints
WHERE EXISTS (SELECT 1 FROM region_data WHERE vnum = 1001);

-- Insert sample region profile for testing
INSERT IGNORE INTO region_profiles (region_vnum, overall_theme, dominant_mood, key_characteristics, description_style) 
SELECT * FROM (
    SELECT 1001 as region_vnum, 'Ancient mystical forest with primordial energy' as overall_theme, 'Serene yet alive with ancient power' as dominant_mood, '["towering_oaks", "hidden_clearings", "woodland_creatures", "mossy_paths"]' as key_characteristics, 'poetic' as description_style
) AS sample_profile
WHERE EXISTS (SELECT 1 FROM region_data WHERE vnum = 1001);

-- =====================================================================
-- PART 4: INSTALLATION VERIFICATION
-- =====================================================================

-- Verify installation was successful
SELECT 'Narrative Weaver Installation Complete!' as status;

-- Show table counts
SELECT 
    'region_hints' as table_name, 
    COUNT(*) as record_count 
FROM region_hints
UNION
SELECT 
    'hint_usage_log' as table_name, 
    COUNT(*) as record_count 
FROM hint_usage_log
UNION
SELECT 
    'region_profiles' as table_name, 
    COUNT(*) as record_count 
FROM region_profiles
UNION
SELECT 
    'description_templates' as table_name, 
    COUNT(*) as record_count 
FROM description_templates;

-- Show view status
SELECT 'Views created successfully' as status
WHERE EXISTS (
    SELECT 1 FROM information_schema.views 
    WHERE table_schema = 'luminari_mudprod' 
    AND table_name IN ('active_region_hints', 'hint_analytics')
);

-- Show sample data status (if region 1001 exists)
SELECT 
    CASE 
        WHEN EXISTS (SELECT 1 FROM region_data WHERE vnum = 1001) 
        THEN CONCAT('Sample data installed for region 1001: ', COUNT(*), ' hints created')
        ELSE 'No sample data installed (region 1001 not found in region_data)'
    END as sample_data_status
FROM region_hints 
WHERE region_vnum = 1001;

SELECT '=== Narrative Weaver Installation Summary ===' as summary;
