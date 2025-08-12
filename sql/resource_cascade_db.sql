-- Phase 7: Ecological Resource Interdependencies Database Schema
-- This extends the existing resource_depletion system with cascade relationships

-- Add cascade tracking to existing resource_depletion table
ALTER TABLE resource_depletion ADD COLUMN cascade_effects TEXT DEFAULT NULL;

-- Table to define relationships between resources
CREATE TABLE resource_relationships (
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
    UNIQUE KEY unique_relationship (source_resource, target_resource, effect_type)
);

-- Insert core ecological relationships
INSERT INTO resource_relationships 
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
CREATE TABLE ecosystem_health (
    id INT PRIMARY KEY AUTO_INCREMENT,
    zone_vnum INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    health_state ENUM('pristine', 'healthy', 'stressed', 'degraded', 'collapsed') NOT NULL,
    health_score DECIMAL(4,3) NOT NULL COMMENT 'Overall health score 0.0-1.0',
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY unique_location (zone_vnum, x_coord, y_coord),
    INDEX idx_health_state (health_state),
    INDEX idx_health_score (health_score)
);

-- Table for tracking player conservation impact
CREATE TABLE player_conservation (
    id INT PRIMARY KEY AUTO_INCREMENT,
    player_id INT NOT NULL,
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
    INDEX idx_conservation_score (conservation_score)
);

-- Table for storing cascade effect history (for debugging and analysis)
CREATE TABLE cascade_effects_log (
    id INT PRIMARY KEY AUTO_INCREMENT,
    zone_vnum INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    source_resource INT NOT NULL,
    target_resource INT NOT NULL,
    effect_magnitude DECIMAL(5,3) NOT NULL,
    player_id INT DEFAULT NULL,
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_location_time (zone_vnum, x_coord, y_coord, logged_at),
    INDEX idx_resources (source_resource, target_resource)
);

-- View for easy ecosystem analysis
CREATE VIEW ecosystem_analysis AS
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
    eh.last_updated
FROM ecosystem_health eh
LEFT JOIN resource_depletion rd ON 
    eh.zone_vnum = rd.zone_vnum AND 
    eh.x_coord = rd.x_coord AND 
    eh.y_coord = rd.y_coord
GROUP BY eh.zone_vnum, eh.x_coord, eh.y_coord, eh.health_state, eh.health_score, eh.last_updated;

COMMIT;
