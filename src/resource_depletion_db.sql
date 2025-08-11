-- Phase 6 Resource Depletion Database Schema
-- LuminariMUD Resource Conservation System

-- Table to track resource depletion by coordinates (not room_vnum for wilderness)
CREATE TABLE IF NOT EXISTS resource_depletion (
    id INT AUTO_INCREMENT PRIMARY KEY,
    zone_vnum INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    resource_type TINYINT NOT NULL,
    depletion_level FLOAT NOT NULL DEFAULT 1.0,
    last_harvest TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    total_harvested INT DEFAULT 0,
    regeneration_rate FLOAT DEFAULT 0.01,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_location (zone_vnum, x_coord, y_coord),
    INDEX idx_resource (resource_type),
    UNIQUE KEY unique_location_resource (zone_vnum, x_coord, y_coord, resource_type)
);

-- Table to track player conservation scores (per player, not per resource)
CREATE TABLE IF NOT EXISTS player_conservation (
    id INT AUTO_INCREMENT PRIMARY KEY,
    player_id BIGINT NOT NULL,
    conservation_score FLOAT NOT NULL DEFAULT 0.5,
    total_harvests INT DEFAULT 0,
    sustainable_harvests INT DEFAULT 0,
    unsustainable_harvests INT DEFAULT 0,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY unique_player (player_id)
);

-- Table to track global resource statistics
CREATE TABLE IF NOT EXISTS resource_statistics (
    id INT AUTO_INCREMENT PRIMARY KEY,
    resource_type TINYINT NOT NULL,
    total_locations INT DEFAULT 0,
    total_harvested_global BIGINT DEFAULT 0,
    total_depleted_locations INT DEFAULT 0,
    average_depletion_level FLOAT DEFAULT 1.0,
    peak_depletion_level FLOAT DEFAULT 1.0,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY unique_resource (resource_type)
);

-- Insert default resource statistics with proper depletion rates
INSERT IGNORE INTO resource_statistics (resource_type, average_depletion_level, peak_depletion_level) VALUES
(0, 1.0, 1.0),   -- RESOURCE_VEGETATION (fast growing, fast depletion)
(1, 1.0, 1.0),   -- RESOURCE_MINERALS (slow regeneration, slow depletion)
(2, 1.0, 1.0),   -- RESOURCE_WATER (seasonal regeneration)
(3, 1.0, 1.0),   -- RESOURCE_HERBS (moderate regeneration)
(4, 1.0, 1.0),   -- RESOURCE_GAME (mobile, complex regeneration)
(5, 1.0, 1.0),   -- RESOURCE_WOOD (slow regeneration, large harvests)
(6, 1.0, 1.0),   -- RESOURCE_STONE (very slow regeneration)
(7, 1.0, 1.0),   -- RESOURCE_CRYSTAL (rare, very slow regeneration)
(8, 1.0, 1.0),   -- RESOURCE_CLAY (moderate regeneration, weather dependent)
(9, 1.0, 1.0);   -- RESOURCE_SALT (slow regeneration, location dependent)
