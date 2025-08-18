-- =====================================================================
-- AI-Generated Region Hints System for Dynamic Descriptions
-- =====================================================================
-- This schema stores AI-generated descriptive hints for geographic regions
-- that will be used by the dynamic description engine to create immersive,
-- location-specific descriptions.

USE luminari_mudprod;

-- =====================================================================
-- PART 1: AI REGION DESCRIPTIVE HINTS
-- =====================================================================

-- Main table for AI-generated region hints
CREATE TABLE IF NOT EXISTS ai_region_hints (
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

-- =====================================================================
-- PART 2: HINT USAGE TRACKING AND ANALYTICS
-- =====================================================================

-- Track which hints are actually used in descriptions for analytics
CREATE TABLE IF NOT EXISTS ai_hint_usage_log (
    id INT AUTO_INCREMENT PRIMARY KEY,
    hint_id INT NOT NULL,
    room_vnum INT NOT NULL,
    player_id INT DEFAULT NULL,  -- If tracking per-player
    used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    weather_condition VARCHAR(20),
    season VARCHAR(10),
    time_of_day VARCHAR(10),
    resource_state JSON DEFAULT NULL,  -- Snapshot of resource levels when used
    
    FOREIGN KEY (hint_id) REFERENCES ai_region_hints(id) ON DELETE CASCADE,
    INDEX idx_hint_usage (hint_id, used_at),
    INDEX idx_room_usage (room_vnum, used_at)
);

-- =====================================================================
-- PART 3: REGION PERSONALITY PROFILES
-- =====================================================================

-- AI-generated personality profiles for regions (overall character/theme)
CREATE TABLE IF NOT EXISTS ai_region_profiles (
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

-- =====================================================================
-- PART 4: DYNAMIC DESCRIPTION TEMPLATES
-- =====================================================================

-- AI-generated description templates that can be filled with dynamic data
CREATE TABLE IF NOT EXISTS ai_description_templates (
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
-- PART 5: SAMPLE DATA FOR TESTING
-- =====================================================================

-- Sample region hints for testing (assuming region vnum 1001 exists)
INSERT IGNORE INTO ai_region_hints (region_vnum, hint_category, hint_text, priority, weather_conditions) VALUES
(1001, 'atmosphere', 'Ancient oak trees tower overhead, their gnarled branches creating a natural cathedral of green.', 8, 'clear,cloudy'),
(1001, 'atmosphere', 'Mist clings to the forest floor, giving the woodland an ethereal, dreamlike quality.', 6, 'cloudy,rainy'),
(1001, 'fauna', 'Squirrels chatter in the canopy while deer paths wind between the massive tree trunks.', 7, 'clear,cloudy'),
(1001, 'flora', 'Thick carpets of moss cover fallen logs, and delicate wildflowers bloom in dappled clearings.', 7, 'clear,cloudy,rainy'),
(1001, 'sounds', 'The gentle rustle of leaves mingles with distant bird calls and the soft trickle of hidden streams.', 6, 'clear,cloudy'),
(1001, 'scents', 'The air carries the rich scent of earth and growing things, tinged with the sweetness of wild honeysuckle.', 5, 'clear,cloudy');

-- Sample region profile
INSERT IGNORE INTO ai_region_profiles (region_vnum, overall_theme, dominant_mood, key_characteristics, description_style) VALUES
(1001, 'Ancient mystical forest with primordial energy', 'Serene yet alive with ancient power', '["towering_oaks", "hidden_clearings", "woodland_creatures", "mossy_paths"]', 'poetic');

-- =====================================================================
-- PART 6: PERFORMANCE OPTIMIZATION VIEWS
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
FROM ai_region_hints rh
LEFT JOIN ai_region_profiles rp ON rh.region_vnum = rp.region_vnum
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
FROM ai_region_hints rh
LEFT JOIN ai_hint_usage_log hul ON rh.id = hul.hint_id
WHERE rh.is_active = TRUE
GROUP BY rh.region_vnum, rh.hint_category;
