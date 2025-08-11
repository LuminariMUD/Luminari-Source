-- =====================================================================
-- Luminari MUD Dynamic Descriptions & Wilderness Resource System
-- Complete Database Deployment Script
-- =====================================================================
-- This script creates all necessary database tables and structures for:
-- 1. Dynamic wilderness descriptions with weather integration
-- 2. Complete wilderness resource system with depletion tracking
-- 3. Ecological interdependencies and cascade effects
-- 4. Player conservation tracking
-- 5. Regional effects system
-- =====================================================================

-- Set database (update this to match your actual database name)
USE luminari_mudprod;

-- Enable foreign key checks
SET FOREIGN_KEY_CHECKS = 1;

-- =====================================================================
-- PART 1: CORE RESOURCE SYSTEM TABLES
-- =====================================================================

-- Resource type constants (reference table)
CREATE TABLE IF NOT EXISTS resource_types (
    resource_id INT PRIMARY KEY,
    resource_name VARCHAR(50) NOT NULL UNIQUE,
    resource_description TEXT,
    base_rarity DECIMAL(3,2) DEFAULT 1.00,
    regeneration_rate DECIMAL(4,3) DEFAULT 0.001,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Insert resource type definitions
INSERT IGNORE INTO resource_types (resource_id, resource_name, resource_description, base_rarity, regeneration_rate) VALUES
(0, 'vegetation', 'General plant life, grasses, shrubs', 1.00, 0.005),
(1, 'minerals', 'Ores, metals, precious stones', 0.60, 0.001),
(2, 'water', 'Fresh water sources', 1.20, 0.008),
(3, 'herbs', 'Medicinal or magical plants', 0.70, 0.003),
(4, 'game', 'Huntable animals and wildlife', 0.80, 0.004),
(5, 'wood', 'Harvestable timber and lumber', 0.90, 0.002),
(6, 'stone', 'Building materials and quarried rock', 0.85, 0.001),
(7, 'crystal', 'Rare magical components and gems', 0.30, 0.0005),
(8, 'clay', 'Crafting materials and pottery clay', 0.75, 0.003),
(9, 'salt', 'Preservation materials and salt deposits', 0.65, 0.002);

-- =====================================================================
-- PART 2: RESOURCE DEPLETION TRACKING SYSTEM
-- =====================================================================

-- Main resource depletion tracking table
-- Stores depletion levels for each resource type at each coordinate location
CREATE TABLE IF NOT EXISTS resource_depletion (
    id INT AUTO_INCREMENT PRIMARY KEY,
    zone_vnum INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    resource_type INT NOT NULL,
    depletion_level FLOAT DEFAULT 1.0,  -- 1.0 = fully available, 0.0 = completely depleted
    total_harvested INT DEFAULT 0,
    last_harvest TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    cascade_effects TEXT DEFAULT NULL,  -- JSON data for cascade effect tracking
    regeneration_rate DECIMAL(5,4) DEFAULT 0.0010,  -- Custom regeneration rate
    last_regeneration TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_location (zone_vnum, x_coord, y_coord),
    INDEX idx_resource (resource_type),
    INDEX idx_depletion (depletion_level),
    INDEX idx_last_harvest (last_harvest),
    UNIQUE KEY unique_location_resource (zone_vnum, x_coord, y_coord, resource_type),
    FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE CASCADE
);

-- Player conservation score tracking
-- Stores each player's environmental stewardship performance
CREATE TABLE IF NOT EXISTS player_conservation (
    id INT AUTO_INCREMENT PRIMARY KEY,
    player_id BIGINT NOT NULL,
    conservation_score FLOAT DEFAULT 0.5,  -- 0.0 = poor, 1.0 = excellent
    total_harvests INT DEFAULT 0,
    sustainable_harvests INT DEFAULT 0,
    unsustainable_harvests INT DEFAULT 0,
    ecosystem_damage DECIMAL(8,3) DEFAULT 0.0,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_player_id (player_id),
    INDEX idx_conservation_score (conservation_score),
    UNIQUE KEY unique_player (player_id)
);

-- Global resource statistics
-- Tracks server-wide resource usage patterns
CREATE TABLE IF NOT EXISTS resource_statistics (
    id INT AUTO_INCREMENT PRIMARY KEY,
    resource_type INT NOT NULL,
    total_harvested BIGINT DEFAULT 0,
    total_depleted_locations INT DEFAULT 0,
    average_depletion_level FLOAT DEFAULT 1.0,
    peak_depletion_level FLOAT DEFAULT 1.0,
    critical_locations INT DEFAULT 0,  -- Locations below 0.2 depletion
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    UNIQUE KEY unique_resource (resource_type),
    FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE CASCADE
);

-- Resource regeneration events log
-- Tracks when and how resources regenerate at coordinate locations
CREATE TABLE IF NOT EXISTS resource_regeneration_log (
    id INT AUTO_INCREMENT PRIMARY KEY,
    zone_vnum INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    resource_type INT NOT NULL,
    old_depletion_level FLOAT NOT NULL,
    new_depletion_level FLOAT NOT NULL,
    regeneration_amount FLOAT NOT NULL,
    regeneration_type ENUM('natural', 'seasonal', 'magical', 'admin', 'cascade') DEFAULT 'natural',
    regeneration_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_location_regen (zone_vnum, x_coord, y_coord),
    INDEX idx_time_regen (regeneration_time),
    INDEX idx_resource_type (resource_type),
    FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE CASCADE
);

-- =====================================================================
-- PART 3: ECOLOGICAL INTERDEPENDENCIES & CASCADE EFFECTS
-- =====================================================================

-- Table to define relationships between resources
CREATE TABLE IF NOT EXISTS resource_relationships (
    id INT PRIMARY KEY AUTO_INCREMENT,
    source_resource INT NOT NULL COMMENT 'Resource being harvested',
    target_resource INT NOT NULL COMMENT 'Resource being affected',
    effect_type ENUM('depletion', 'enhancement', 'threshold') NOT NULL COMMENT 'Type of effect',
    effect_magnitude DECIMAL(5,3) NOT NULL COMMENT 'Strength of effect (-1.0 to +1.0)',
    threshold_min DECIMAL(4,3) DEFAULT NULL COMMENT 'Minimum threshold for effect',
    threshold_max DECIMAL(4,3) DEFAULT NULL COMMENT 'Maximum threshold for effect',
    description VARCHAR(255) COMMENT 'Human readable description',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_source_resource (source_resource),
    INDEX idx_target_resource (target_resource),
    UNIQUE KEY unique_relationship (source_resource, target_resource, effect_type),
    FOREIGN KEY (source_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE,
    FOREIGN KEY (target_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE
);

-- Insert core ecological relationships
INSERT IGNORE INTO resource_relationships 
(source_resource, target_resource, effect_type, effect_magnitude, description) VALUES

-- Vegetation affects herbs (symbiotic)
(0, 3, 'depletion', -0.030, 'Vegetation harvesting damages herb root systems'),
(0, 4, 'depletion', -0.020, 'Vegetation removal disrupts game habitats'),
(0, 8, 'enhancement', 0.010, 'Plant removal exposes clay deposits'),

-- Herbs affect vegetation (symbiotic)
(3, 0, 'depletion', -0.020, 'Herb harvesting damages vegetation root networks'),
(3, 4, 'depletion', -0.010, 'Herb harvesting reduces game food sources'),

-- Minerals vs crystals (competitive destruction)
(1, 7, 'depletion', -0.080, 'Heavy mining operations destroy crystal formations'),
(1, 2, 'depletion', -0.030, 'Mining disrupts groundwater systems'),
(1, 6, 'enhancement', 0.020, 'Mining exposes stone deposits'),

-- Crystals vs minerals (precision mining interference)
(7, 1, 'depletion', -0.040, 'Crystal extraction affects mineral ore veins'),
(7, 6, 'depletion', -0.020, 'Precision crystal mining weakens stone integrity'),

-- Wood affects entire ecosystem
(5, 0, 'depletion', -0.050, 'Tree removal changes canopy and sunlight patterns'),
(5, 3, 'depletion', -0.040, 'Deforestation disrupts herb microclimates'),
(5, 4, 'depletion', -0.060, 'Tree removal destroys game habitats'),
(5, 2, 'depletion', -0.020, 'Forest loss reduces water retention'),

-- Game affects vegetation (grazing balance)
(4, 0, 'enhancement', 0.030, 'Reduced game population decreases grazing pressure'),
(4, 3, 'enhancement', 0.020, 'Less wildlife reduces herb trampling'),

-- Water supports biological resources
(2, 8, 'enhancement', 0.040, 'Water harvesting exposes lakebed clay deposits'),
(2, 0, 'enhancement', 0.020, 'Irrigation effect from water use'),
(2, 3, 'enhancement', 0.030, 'Medicinal plants benefit from water irrigation'),

-- Stone quarrying geological effects
(6, 1, 'depletion', -0.030, 'Quarrying operations disrupt mineral ore seams'),
(6, 7, 'depletion', -0.050, 'Stone quarrying vibrations shatter crystal formations'),
(6, 8, 'enhancement', 0.030, 'Quarrying exposes sediment layers containing clay'),

-- Salt harvesting environmental effects
(9, 2, 'depletion', -0.060, 'Salt harvesting depletes brine pools and water sources'),
(9, 0, 'depletion', -0.080, 'Salt extraction causes soil salinization'),
(9, 8, 'enhancement', 0.020, 'Salt flat harvesting exposes clay deposits'),

-- Clay harvesting water/soil effects
(8, 2, 'depletion', -0.025, 'Clay extraction diverts water from natural systems'),
(8, 0, 'depletion', -0.015, 'Clay harvesting disturbs vegetation root systems');

-- Table for tracking ecosystem health states
CREATE TABLE IF NOT EXISTS ecosystem_health (
    id INT PRIMARY KEY AUTO_INCREMENT,
    zone_vnum INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    health_state ENUM('pristine', 'healthy', 'stressed', 'degraded', 'collapsed') NOT NULL DEFAULT 'healthy',
    health_score DECIMAL(4,3) NOT NULL DEFAULT 1.000 COMMENT 'Overall health score 0.0-1.0',
    biodiversity_index DECIMAL(4,3) DEFAULT 1.000,
    stability_index DECIMAL(4,3) DEFAULT 1.000,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE KEY unique_location (zone_vnum, x_coord, y_coord),
    INDEX idx_health_state (health_state),
    INDEX idx_health_score (health_score),
    INDEX idx_zone (zone_vnum)
);

-- Table for storing cascade effect history (for debugging and analysis)
CREATE TABLE IF NOT EXISTS cascade_effects_log (
    id INT PRIMARY KEY AUTO_INCREMENT,
    zone_vnum INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    source_resource INT NOT NULL,
    target_resource INT NOT NULL,
    effect_magnitude DECIMAL(5,3) NOT NULL,
    player_id BIGINT DEFAULT NULL,
    harvest_amount INT DEFAULT 0,
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_location_time (zone_vnum, x_coord, y_coord, logged_at),
    INDEX idx_resources (source_resource, target_resource),
    INDEX idx_player (player_id),
    FOREIGN KEY (source_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE,
    FOREIGN KEY (target_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE
);

-- =====================================================================
-- PART 4: PLAYER-LOCATION CONSERVATION TRACKING
-- =====================================================================

-- Detailed player conservation tracking per location
CREATE TABLE IF NOT EXISTS player_location_conservation (
    id INT PRIMARY KEY AUTO_INCREMENT,
    player_id BIGINT NOT NULL,
    zone_vnum INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    conservation_score DECIMAL(4,3) DEFAULT 0.5 COMMENT 'Player conservation score 0.0-1.0',
    total_harvests INT DEFAULT 0,
    sustainable_harvests INT DEFAULT 0,
    ecosystem_damage DECIMAL(6,3) DEFAULT 0.0 COMMENT 'Cumulative ecosystem damage caused',
    last_harvest TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_player_location (player_id, zone_vnum, x_coord, y_coord),
    INDEX idx_conservation_score (conservation_score),
    INDEX idx_location (zone_vnum, x_coord, y_coord),
    UNIQUE KEY unique_player_location (player_id, zone_vnum, x_coord, y_coord)
);

-- =====================================================================
-- PART 5: REGIONAL EFFECTS SYSTEM
-- =====================================================================

-- Main effects table - defines what effects are available
CREATE TABLE IF NOT EXISTS region_effects (
    effect_id INT AUTO_INCREMENT PRIMARY KEY,
    effect_name VARCHAR(100) NOT NULL UNIQUE,
    effect_type ENUM('resource', 'combat', 'movement', 'weather', 'magic', 'description', 'other') NOT NULL,
    effect_description TEXT,
    effect_data JSON,  -- Flexible data storage for effect parameters
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    INDEX idx_effect_type (effect_type),
    INDEX idx_effect_name (effect_name),
    INDEX idx_active (is_active)
);

-- Region-Effect assignment table - links regions to their effects
CREATE TABLE IF NOT EXISTS region_effect_assignments (
    assignment_id INT AUTO_INCREMENT PRIMARY KEY,
    region_vnum INT NOT NULL,
    effect_id INT NOT NULL,
    intensity DECIMAL(3,2) DEFAULT 1.00,  -- How strong this effect is (multiplier)
    is_active BOOLEAN DEFAULT TRUE,
    assigned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NULL,  -- NULL = permanent, otherwise temporary effect
    
    FOREIGN KEY (effect_id) REFERENCES region_effects(effect_id) ON DELETE CASCADE,
    INDEX idx_region_vnum (region_vnum),
    INDEX idx_effect_id (effect_id),
    INDEX idx_active (is_active),
    INDEX idx_expires (expires_at),
    UNIQUE KEY unique_region_effect (region_vnum, effect_id)
);

-- =====================================================================
-- PART 6: WEATHER AND DESCRIPTION SYSTEM
-- =====================================================================

-- Weather pattern cache for wilderness areas (optional optimization)
CREATE TABLE IF NOT EXISTS weather_cache (
    id INT AUTO_INCREMENT PRIMARY KEY,
    zone_vnum INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    weather_value INT NOT NULL,  -- 0-255 wilderness weather value
    weather_type ENUM('clear', 'cloudy', 'rainy', 'stormy', 'lightning') NOT NULL,
    cached_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL,
    
    UNIQUE KEY unique_location (zone_vnum, x_coord, y_coord),
    INDEX idx_weather_type (weather_type),
    INDEX idx_expires (expires_at)
);

-- Dynamic description preferences (per-room customization)
CREATE TABLE IF NOT EXISTS room_description_settings (
    id INT AUTO_INCREMENT PRIMARY KEY,
    room_vnum INT NOT NULL UNIQUE,
    use_dynamic_descriptions BOOLEAN DEFAULT TRUE,
    description_detail_level TINYINT DEFAULT 3,  -- 1-5 scale
    weather_effects_enabled BOOLEAN DEFAULT TRUE,
    resource_awareness_enabled BOOLEAN DEFAULT TRUE,
    time_of_day_effects_enabled BOOLEAN DEFAULT TRUE,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    INDEX idx_room_vnum (room_vnum),
    INDEX idx_dynamic_enabled (use_dynamic_descriptions)
);

-- =====================================================================
-- PART 7: MATERIAL SUBTYPES SYSTEM
-- =====================================================================

-- Material categories (herbs, metals, etc.)
CREATE TABLE IF NOT EXISTS material_categories (
    category_id INT PRIMARY KEY,
    category_name VARCHAR(50) NOT NULL UNIQUE,
    category_description TEXT,
    resource_type INT NOT NULL,  -- Links to resource_types table
    
    FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE CASCADE,
    INDEX idx_resource_type (resource_type)
);

-- Specific material subtypes within categories
CREATE TABLE IF NOT EXISTS material_subtypes (
    subtype_id INT AUTO_INCREMENT PRIMARY KEY,
    category_id INT NOT NULL,
    subtype_name VARCHAR(50) NOT NULL,
    subtype_description TEXT,
    base_rarity DECIMAL(4,3) DEFAULT 1.000,
    terrain_preference SET('forest', 'plains', 'mountain', 'desert', 'swamp', 'water') DEFAULT '',
    seasonal_availability SET('spring', 'summer', 'autumn', 'winter') DEFAULT 'spring,summer,autumn,winter',
    
    FOREIGN KEY (category_id) REFERENCES material_categories(category_id) ON DELETE CASCADE,
    INDEX idx_category (category_id),
    INDEX idx_subtype_name (subtype_name),
    UNIQUE KEY unique_category_subtype (category_id, subtype_name)
);

-- Quality levels for materials
CREATE TABLE IF NOT EXISTS material_qualities (
    quality_id INT PRIMARY KEY,
    quality_name VARCHAR(20) NOT NULL UNIQUE,
    quality_description TEXT,
    rarity_multiplier DECIMAL(4,3) DEFAULT 1.000,
    value_multiplier DECIMAL(4,3) DEFAULT 1.000
);

-- Insert material quality levels
INSERT IGNORE INTO material_qualities (quality_id, quality_name, quality_description, rarity_multiplier, value_multiplier) VALUES
(1, 'poor', 'Low quality materials with basic properties', 2.000, 0.500),
(2, 'common', 'Standard quality materials found regularly', 1.000, 1.000),
(3, 'uncommon', 'Above average materials with enhanced properties', 0.600, 1.500),
(4, 'rare', 'High quality materials with exceptional properties', 0.300, 3.000),
(5, 'legendary', 'Extraordinary materials with unique properties', 0.100, 8.000);

-- =====================================================================
-- PART 8: PERFORMANCE AND ANALYTICS VIEWS
-- =====================================================================

-- View for easy ecosystem analysis
CREATE VIEW IF NOT EXISTS ecosystem_analysis AS
SELECT 
    eh.zone_vnum,
    eh.x_coord,
    eh.y_coord,
    eh.health_state,
    eh.health_score,
    COUNT(rd.resource_type) as tracked_resources,
    AVG(rd.depletion_level) as avg_depletion,
    MIN(rd.depletion_level) as min_depletion,
    MAX(rd.depletion_level) as max_depletion,
    SUM(CASE WHEN rd.depletion_level < 0.2 THEN 1 ELSE 0 END) as critical_resources,
    eh.last_updated
FROM ecosystem_health eh
LEFT JOIN resource_depletion rd ON 
    eh.zone_vnum = rd.zone_vnum AND 
    eh.x_coord = rd.x_coord AND 
    eh.y_coord = rd.y_coord
GROUP BY eh.zone_vnum, eh.x_coord, eh.y_coord, eh.health_state, eh.health_score, eh.last_updated;

-- View for resource availability summary
CREATE VIEW IF NOT EXISTS resource_availability_summary AS
SELECT 
    rt.resource_name,
    COUNT(rd.id) as locations_tracked,
    AVG(rd.depletion_level) as avg_availability,
    MIN(rd.depletion_level) as min_availability,
    MAX(rd.depletion_level) as max_availability,
    SUM(CASE WHEN rd.depletion_level < 0.2 THEN 1 ELSE 0 END) as critical_locations,
    SUM(CASE WHEN rd.depletion_level > 0.8 THEN 1 ELSE 0 END) as abundant_locations,
    MAX(rd.last_harvest) as last_global_harvest
FROM resource_types rt
LEFT JOIN resource_depletion rd ON rt.resource_id = rd.resource_type
GROUP BY rt.resource_id, rt.resource_name
ORDER BY rt.resource_id;

-- View for player conservation ranking
CREATE VIEW IF NOT EXISTS player_conservation_ranking AS
SELECT 
    pc.player_id,
    pc.conservation_score,
    pc.total_harvests,
    pc.sustainable_harvests,
    pc.unsustainable_harvests,
    ROUND((pc.sustainable_harvests * 100.0 / NULLIF(pc.total_harvests, 0)), 2) as sustainability_percentage,
    pc.ecosystem_damage,
    pc.last_updated,
    ROW_NUMBER() OVER (ORDER BY pc.conservation_score DESC) as conservation_rank
FROM player_conservation pc
WHERE pc.total_harvests > 0
ORDER BY pc.conservation_score DESC;

-- =====================================================================
-- PART 9: SAMPLE DATA AND REGION EFFECTS
-- =====================================================================

-- Insert some example resource effects for regions
INSERT IGNORE INTO region_effects (effect_name, effect_type, effect_description, effect_data) VALUES
('Forest Abundance', 'resource', 'Increases wood and vegetation resources in forested areas', 
 JSON_OBJECT('resource_modifiers', JSON_OBJECT('wood', JSON_OBJECT('multiplier', 1.5, 'bonus', 5), 'vegetation', JSON_OBJECT('multiplier', 1.2, 'bonus', 2)))),

('Mineral Rich', 'resource', 'Increases stone and metal resources', 
 JSON_OBJECT('resource_modifiers', JSON_OBJECT('stone', JSON_OBJECT('multiplier', 1.4, 'bonus', 3), 'minerals', JSON_OBJECT('multiplier', 1.6, 'bonus', 4)))),

('Magical Essence', 'resource', 'Enhances magical resource gathering', 
 JSON_OBJECT('resource_modifiers', JSON_OBJECT('crystal', JSON_OBJECT('multiplier', 2.0, 'bonus', 10), 'herbs', JSON_OBJECT('multiplier', 1.3, 'bonus', 3)))),

('Pristine Wilderness', 'description', 'Enhanced descriptions for untouched natural areas', 
 JSON_OBJECT('description_modifiers', JSON_OBJECT('detail_level', 5, 'nature_emphasis', true, 'wildlife_presence', 1.5))),

('Storm Front', 'weather', 'Increases frequency of storms and severe weather', 
 JSON_OBJECT('weather_modifiers', JSON_OBJECT('storm_chance', 1.8, 'lightning_chance', 2.0))),

('Desert Oasis', 'resource', 'Rare water sources in arid regions', 
 JSON_OBJECT('resource_modifiers', JSON_OBJECT('water', JSON_OBJECT('multiplier', 3.0, 'bonus', 15), 'herbs', JSON_OBJECT('multiplier', 0.8, 'bonus', -2))));

-- Insert material categories for detailed resource subtypes
INSERT IGNORE INTO material_categories (category_id, category_name, category_description, resource_type) VALUES
(1, 'Common Herbs', 'Basic medicinal and culinary plants', 3),
(2, 'Rare Herbs', 'Powerful magical and alchemical plants', 3),
(3, 'Common Metals', 'Basic metals for crafting and construction', 1),
(4, 'Precious Metals', 'Valuable metals for fine crafting', 1),
(5, 'Gemstones', 'Crystalline materials and precious stones', 7),
(6, 'Hardwoods', 'Dense, durable timber varieties', 5),
(7, 'Softwoods', 'Lightweight, easily worked timber', 5),
(8, 'Building Stone', 'Construction-grade stone materials', 6);

-- =====================================================================
-- PART 10: INDEXES AND PERFORMANCE OPTIMIZATION
-- =====================================================================

-- Additional performance indexes
CREATE INDEX IF NOT EXISTS idx_depletion_zone_resource ON resource_depletion (zone_vnum, resource_type);
CREATE INDEX IF NOT EXISTS idx_depletion_critical ON resource_depletion (depletion_level) WHERE depletion_level < 0.2;
CREATE INDEX IF NOT EXISTS idx_conservation_excellent ON player_conservation (conservation_score) WHERE conservation_score > 0.8;
CREATE INDEX IF NOT EXISTS idx_recent_harvest ON resource_depletion (last_harvest) WHERE last_harvest > DATE_SUB(NOW(), INTERVAL 24 HOUR);

-- =====================================================================
-- PART 11: CLEANUP AND MAINTENANCE PROCEDURES
-- =====================================================================

-- Create cleanup procedure for old logs (optional - can be run manually)
DELIMITER //
CREATE PROCEDURE IF NOT EXISTS CleanupOldLogs()
BEGIN
    -- Clean up regeneration logs older than 30 days
    DELETE FROM resource_regeneration_log 
    WHERE regeneration_time < DATE_SUB(NOW(), INTERVAL 30 DAY);
    
    -- Clean up cascade effect logs older than 7 days  
    DELETE FROM cascade_effects_log 
    WHERE logged_at < DATE_SUB(NOW(), INTERVAL 7 DAY);
    
    -- Clean up expired weather cache
    DELETE FROM weather_cache 
    WHERE expires_at < NOW();
    
    -- Update resource statistics
    INSERT INTO resource_statistics (resource_type, total_harvested, total_depleted_locations, average_depletion_level, critical_locations)
    SELECT 
        rd.resource_type,
        SUM(rd.total_harvested),
        COUNT(DISTINCT CONCAT(rd.zone_vnum, '_', rd.x_coord, '_', rd.y_coord)),
        AVG(rd.depletion_level),
        SUM(CASE WHEN rd.depletion_level < 0.2 THEN 1 ELSE 0 END)
    FROM resource_depletion rd
    GROUP BY rd.resource_type
    ON DUPLICATE KEY UPDATE
        total_harvested = VALUES(total_harvested),
        total_depleted_locations = VALUES(total_depleted_locations),
        average_depletion_level = VALUES(average_depletion_level),
        critical_locations = VALUES(critical_locations),
        last_updated = NOW();
END //
DELIMITER ;

-- =====================================================================
-- DEPLOYMENT COMPLETE
-- =====================================================================

-- Update statistics for better query performance
ANALYZE TABLE resource_depletion, player_conservation, ecosystem_health, resource_relationships;

-- Display deployment summary
SELECT 'Dynamic Descriptions & Wilderness Resource System Deployment Complete' as Status,
       NOW() as 'Deployed At',
       (SELECT COUNT(*) FROM resource_types) as 'Resource Types',
       (SELECT COUNT(*) FROM resource_relationships) as 'Ecological Relationships',
       (SELECT COUNT(*) FROM region_effects) as 'Regional Effects',
       (SELECT COUNT(*) FROM material_categories) as 'Material Categories',
       (SELECT COUNT(*) FROM material_qualities) as 'Quality Levels';

-- Show created tables
SHOW TABLES LIKE '%resource%';
SHOW TABLES LIKE '%conservation%';
SHOW TABLES LIKE '%ecosystem%';
SHOW TABLES LIKE '%region%';
SHOW TABLES LIKE '%weather%';
SHOW TABLES LIKE '%material%';

-- Basic verification query
SELECT 'System Ready' as Status, 
       'All database structures created successfully' as Message,
       'Use admin commands: settime, setweather for testing' as 'Testing Commands';
