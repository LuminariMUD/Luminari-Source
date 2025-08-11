-- Phase 6: Resource Depletion System Database Schema
-- This file creates the tables needed for persistent resource depletion tracking

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
    INDEX idx_location (zone_vnum, x_coord, y_coord),
    INDEX idx_resource (resource_type),
    UNIQUE KEY unique_location_resource (zone_vnum, x_coord, y_coord, resource_type)
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
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
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
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY unique_resource (resource_type)
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
    regeneration_type ENUM('natural', 'seasonal', 'magical', 'admin') DEFAULT 'natural',
    regeneration_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_location_regen (zone_vnum, x_coord, y_coord),
    INDEX idx_time_regen (regeneration_time)
);
