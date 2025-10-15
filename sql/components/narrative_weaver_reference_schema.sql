-- Narrative Weaver Reference Schema
-- Core table structures, indexes, and views for the narrative weaver system
-- This script creates the foundational infrastructure without sample data

-- ============================================================================
-- PART 1: Enhance region_data table with description and metadata columns
-- ============================================================================

-- Add comprehensive description fields to region_data table
ALTER TABLE region_data 
ADD COLUMN IF NOT EXISTS region_description LONGTEXT DEFAULT NULL COMMENT 'Comprehensive description for AI context',
ADD COLUMN IF NOT EXISTS description_version INT DEFAULT 1 COMMENT 'Version tracking for description updates',
ADD COLUMN IF NOT EXISTS ai_agent_source VARCHAR(100) DEFAULT NULL COMMENT 'Which AI agent created/updated description',
ADD COLUMN IF NOT EXISTS last_description_update TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last modification timestamp',
ADD COLUMN IF NOT EXISTS description_style ENUM('poetic', 'practical', 'mysterious', 'dramatic', 'pastoral') DEFAULT 'poetic' COMMENT 'Writing style for consistency',
ADD COLUMN IF NOT EXISTS description_length ENUM('brief', 'moderate', 'detailed', 'extensive') DEFAULT 'moderate' COMMENT 'Target description length',
ADD COLUMN IF NOT EXISTS has_historical_context BOOLEAN DEFAULT FALSE COMMENT 'Contains historical information',
ADD COLUMN IF NOT EXISTS has_resource_info BOOLEAN DEFAULT FALSE COMMENT 'Contains resource availability info',
ADD COLUMN IF NOT EXISTS has_wildlife_info BOOLEAN DEFAULT FALSE COMMENT 'Contains wildlife descriptions',
ADD COLUMN IF NOT EXISTS has_geological_info BOOLEAN DEFAULT FALSE COMMENT 'Contains geological details',
ADD COLUMN IF NOT EXISTS has_cultural_info BOOLEAN DEFAULT FALSE COMMENT 'Contains cultural/mystical elements',
ADD COLUMN IF NOT EXISTS description_quality_score DECIMAL(3,2) DEFAULT NULL COMMENT 'Quality rating 0.00-5.00',
ADD COLUMN IF NOT EXISTS requires_review BOOLEAN DEFAULT FALSE COMMENT 'Flagged for human review',
ADD COLUMN IF NOT EXISTS is_approved BOOLEAN DEFAULT FALSE COMMENT 'Approved by staff';

-- Add indexes for AI queries and performance
CREATE INDEX IF NOT EXISTS idx_region_has_description ON region_data (region_description(100));
CREATE INDEX IF NOT EXISTS idx_region_description_approved ON region_data (is_approved, requires_review);
CREATE INDEX IF NOT EXISTS idx_region_description_quality ON region_data (description_quality_score);
CREATE INDEX IF NOT EXISTS idx_region_ai_source ON region_data (ai_agent_source);
CREATE INDEX IF NOT EXISTS idx_region_style_length ON region_data (description_style, description_length);

-- ============================================================================
-- PART 2: Create dedicated narrative weaver tables
-- ============================================================================

-- Create region_hints table
CREATE TABLE IF NOT EXISTS region_hints (
    id INT AUTO_INCREMENT PRIMARY KEY,
    region_vnum INT NOT NULL,
    hint_category ENUM('atmosphere','fauna','flora','geography','weather_influence','resources','landmarks','sounds','scents','seasonal_changes','time_of_day','mystical') NOT NULL,
    hint_text TEXT NOT NULL,
    priority TINYINT DEFAULT 5,
    seasonal_weight JSON,
    weather_conditions SET('clear','cloudy','rainy','stormy','lightning') DEFAULT 'clear,cloudy,rainy,stormy,lightning',
    time_of_day_weight JSON,
    resource_triggers JSON,
    agent_id VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    INDEX idx_region_vnum (region_vnum),
    INDEX idx_hint_category (hint_category),
    INDEX idx_priority (priority),
    INDEX idx_is_active (is_active),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (region_vnum) REFERENCES region_data(vnum) ON DELETE CASCADE
);

-- Create region_profiles table
CREATE TABLE IF NOT EXISTS region_profiles (
    region_vnum INT PRIMARY KEY,
    overall_theme TEXT NOT NULL,
    dominant_mood VARCHAR(100) NOT NULL,
    key_characteristics JSON,
    description_style ENUM('poetic', 'practical', 'mysterious', 'dramatic', 'pastoral') DEFAULT 'mysterious',
    complexity_level TINYINT DEFAULT 5,
    agent_id VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (region_vnum) REFERENCES region_data(vnum) ON DELETE CASCADE
);

-- Create hint_usage_log table
CREATE TABLE IF NOT EXISTS hint_usage_log (
    id INT AUTO_INCREMENT PRIMARY KEY,
    region_vnum INT NOT NULL,
    hint_id INT NOT NULL,
    used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    context VARCHAR(255),
    INDEX idx_region_vnum (region_vnum),
    INDEX idx_hint_id (hint_id),
    INDEX idx_used_at (used_at),
    FOREIGN KEY (region_vnum) REFERENCES region_data(vnum) ON DELETE CASCADE,
    FOREIGN KEY (hint_id) REFERENCES region_hints(id) ON DELETE CASCADE
);

-- Create region_description_cache table
CREATE TABLE IF NOT EXISTS region_description_cache (
    id INT AUTO_INCREMENT PRIMARY KEY,
    region_vnum INT NOT NULL,
    description_hash VARCHAR(64) NOT NULL,
    cached_description TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NULL,
    hit_count INT DEFAULT 0,
    INDEX idx_region_vnum (region_vnum),
    INDEX idx_hash (description_hash),
    INDEX idx_expires (expires_at),
    FOREIGN KEY (region_vnum) REFERENCES region_data(vnum) ON DELETE CASCADE
);

-- ============================================================================
-- PART 3: Create views for narrative weaver queries
-- ============================================================================

-- Create active_region_hints view
CREATE OR REPLACE VIEW active_region_hints AS
SELECT 
    rh.id,
    rh.region_vnum,
    rh.hint_category,
    rh.hint_text,
    rh.priority,
    rh.seasonal_weight,
    rh.weather_conditions,
    rh.time_of_day_weight,
    rh.resource_triggers,
    rh.agent_id,
    rh.created_at,
    rh.updated_at
FROM region_hints rh
WHERE rh.is_active = TRUE;

-- Create hint_analytics view
CREATE OR REPLACE VIEW hint_analytics AS
SELECT 
    rh.id as hint_id,
    rh.region_vnum,
    rh.hint_category,
    rh.hint_text,
    rh.priority,
    COUNT(hul.id) as usage_count,
    MAX(hul.used_at) as last_used,
    rh.created_at,
    rh.updated_at
FROM region_hints rh
LEFT JOIN hint_usage_log hul ON rh.id = hul.hint_id
WHERE rh.is_active = TRUE
GROUP BY rh.id, rh.region_vnum, rh.hint_category, rh.hint_text, rh.priority, rh.created_at, rh.updated_at;

-- ============================================================================
-- PART 4: Installation verification
-- ============================================================================

SELECT 'Narrative Weaver reference schema installed successfully!' as status;
SELECT 'Tables created: region_hints, region_profiles, hint_usage_log, region_description_cache' as tables_created;
SELECT 'Views created: active_region_hints, hint_analytics' as views_created;
SELECT 'region_data enhanced with description and metadata columns' as region_data_status;
