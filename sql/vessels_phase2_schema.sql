-- UNIFIED VESSEL SYSTEM - PHASE 2 DATABASE SCHEMA
-- Project: VESSELS-PHASE2-2025
-- Created: January 2025
-- Purpose: Persistent storage for multi-room vessel configurations

-- =====================================================
-- SHIP INTERIORS TABLE
-- Stores the room configuration for each vessel
-- =====================================================
CREATE TABLE IF NOT EXISTS ship_interiors (
    ship_id INT NOT NULL PRIMARY KEY,
    vessel_type INT NOT NULL DEFAULT 0,
    vessel_name VARCHAR(100),
    num_rooms INT NOT NULL DEFAULT 1,
    max_rooms INT NOT NULL DEFAULT 20,
    
    -- Room vnums (serialized array of up to 20 rooms)
    room_vnums TEXT,
    
    -- Special room designations
    bridge_room INT DEFAULT 0,
    entrance_room INT DEFAULT 0,
    
    -- Cargo rooms (up to 5)
    cargo_room1 INT DEFAULT 0,
    cargo_room2 INT DEFAULT 0,
    cargo_room3 INT DEFAULT 0,
    cargo_room4 INT DEFAULT 0,
    cargo_room5 INT DEFAULT 0,
    
    -- Room configuration data (JSON or serialized format)
    room_data BLOB,
    connection_data BLOB,
    
    -- Metadata
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_modified TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    -- Indexes
    INDEX idx_vessel_type (vessel_type),
    INDEX idx_vessel_name (vessel_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
-- SHIP DOCKING RECORDS TABLE
-- Tracks current and historical docking connections
-- =====================================================
CREATE TABLE IF NOT EXISTS ship_docking (
    dock_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    ship1_id INT NOT NULL,
    ship2_id INT NOT NULL,
    
    -- Docking points
    dock_room1 INT NOT NULL,
    dock_room2 INT NOT NULL,
    
    -- Docking metadata
    dock_type ENUM('standard', 'combat', 'emergency', 'forced') DEFAULT 'standard',
    dock_status ENUM('active', 'completed', 'aborted') DEFAULT 'active',
    
    -- Location where docking occurred
    dock_x INT DEFAULT 0,
    dock_y INT DEFAULT 0,
    dock_z INT DEFAULT 0,
    
    -- Timestamps
    dock_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    undock_time TIMESTAMP NULL DEFAULT NULL,
    
    -- Indexes for fast lookup
    INDEX idx_ship1 (ship1_id),
    INDEX idx_ship2 (ship2_id),
    INDEX idx_active (dock_status),
    INDEX idx_dock_time (dock_time),
    
    -- Prevent duplicate active dockings
    UNIQUE KEY unique_active_dock (ship1_id, ship2_id, dock_status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
-- SHIP ROOM TEMPLATES TABLE
-- Predefined room templates for ship generation
-- =====================================================
CREATE TABLE IF NOT EXISTS ship_room_templates (
    template_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    room_type VARCHAR(50) NOT NULL,
    vessel_type INT DEFAULT 0,
    
    -- Room properties
    name_format VARCHAR(200),
    description_text TEXT,
    room_flags INT DEFAULT 0,
    sector_type INT DEFAULT 0,
    
    -- Requirements
    min_vessel_size INT DEFAULT 0,
    max_vessel_size INT DEFAULT 99,
    
    -- Metadata
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    -- Indexes
    INDEX idx_room_type (room_type),
    INDEX idx_vessel_type (vessel_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
-- SHIP CARGO MANIFEST TABLE
-- Tracks cargo items in ship cargo holds
-- =====================================================
CREATE TABLE IF NOT EXISTS ship_cargo_manifest (
    manifest_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    ship_id INT NOT NULL,
    cargo_room INT NOT NULL,
    
    -- Item information
    item_vnum INT NOT NULL,
    item_name VARCHAR(100),
    item_count INT DEFAULT 1,
    item_weight INT DEFAULT 0,
    
    -- Loading information
    loaded_by VARCHAR(50),
    loaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    -- Indexes
    INDEX idx_ship (ship_id),
    INDEX idx_room (cargo_room),
    INDEX idx_item (item_vnum),
    
    -- Foreign key to ship_interiors
    FOREIGN KEY (ship_id) REFERENCES ship_interiors(ship_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
-- SHIP CREW ROSTER TABLE
-- Tracks NPC crew assignments
-- =====================================================
CREATE TABLE IF NOT EXISTS ship_crew_roster (
    roster_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    ship_id INT NOT NULL,
    
    -- NPC information
    npc_vnum INT NOT NULL,
    npc_name VARCHAR(100),
    crew_role ENUM('captain', 'pilot', 'gunner', 'engineer', 'medic', 'marine', 'crew') DEFAULT 'crew',
    
    -- Assignment details
    assigned_room INT DEFAULT 0,
    duty_station INT DEFAULT 0,
    loyalty_rating INT DEFAULT 50,
    
    -- Status
    status ENUM('active', 'injured', 'awol', 'dead') DEFAULT 'active',
    
    -- Timestamps
    hired_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    
    -- Indexes
    INDEX idx_ship (ship_id),
    INDEX idx_role (crew_role),
    INDEX idx_status (status),
    
    -- Foreign key to ship_interiors
    FOREIGN KEY (ship_id) REFERENCES ship_interiors(ship_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================================
-- DEFAULT ROOM TEMPLATES DATA
-- Insert standard room templates for vessel generation
-- =====================================================
INSERT INTO ship_room_templates (room_type, name_format, description_text, room_flags, sector_type, min_vessel_size) VALUES
-- Bridge/Control rooms
('bridge', 'The %s''s Bridge', 'The command center of the vessel, filled with navigation equipment and control panels. Large windows provide a panoramic view of the surroundings.', 262144, 0, 0),
('helm', 'The %s''s Helm', 'The pilot''s station, featuring the ship''s wheel and primary navigation controls.', 262144, 0, 0),

-- Crew areas
('quarters_captain', 'Captain''s Quarters', 'A spacious cabin befitting the ship''s commander, with a large bunk, desk, and personal storage.', 262144, 0, 3),
('quarters_crew', 'Crew Quarters', 'Rows of bunks line the walls of this cramped but functional sleeping area.', 262144, 0, 1),
('quarters_officer', 'Officers'' Quarters', 'Modest private cabins for the ship''s officers, each with a bunk and small desk.', 262144, 0, 5),

-- Cargo and storage
('cargo_main', 'Main Cargo Hold', 'A cavernous space filled with crates, barrels, and secured cargo. The air smells of tar and sea salt.', 262144, 0, 2),
('cargo_secure', 'Secure Cargo Hold', 'A reinforced compartment with heavy locks, used for valuable or dangerous cargo.', 262144, 0, 5),

-- Ship systems
('engineering', 'Engineering', 'The heart of the ship''s mechanical systems, filled with pipes, gauges, and machinery.', 262144, 0, 3),
('weapons', 'Weapons Deck', 'Cannons and ballistae line this reinforced deck, ready for naval combat.', 262144, 0, 5),
('armory', 'Ship''s Armory', 'Racks of weapons and armor line the walls, secured behind iron bars.', 262144, 0, 7),

-- Common areas
('mess_hall', 'Mess Hall', 'Long tables and benches fill this communal dining area. The lingering smell of the last meal hangs in the air.', 262144, 0, 3),
('galley', 'Ship''s Galley', 'The ship''s kitchen, equipped with a large stove and food preparation areas.', 262144, 0, 2),
('infirmary', 'Ship''s Infirmary', 'A small medical bay with cots and basic healing supplies.', 262144, 0, 4),

-- Connectivity
('corridor', 'Ship''s Corridor', 'A narrow passageway connecting different sections of the vessel.', 262144, 0, 0),
('deck_main', 'Main Deck', 'The open deck of the ship, exposed to the elements. Rigging and masts tower overhead.', 262144, 0, 0),
('deck_lower', 'Lower Deck', 'Below the main deck, this area provides access to the ship''s interior compartments.', 262144, 0, 1),

-- Special rooms
('airlock', 'Airlock', 'A sealed chamber used for boarding and emergency exits.', 262144, 0, 10),
('observation', 'Observation Deck', 'An elevated platform providing excellent views in all directions.', 262144, 0, 5),
('brig', 'Ship''s Brig', 'Iron-barred cells for holding prisoners or unruly crew members.', 262144, 0, 6)
ON DUPLICATE KEY UPDATE description_text = VALUES(description_text);

-- =====================================================
-- STORED PROCEDURES
-- =====================================================

-- Procedure to clean up orphaned docking records
DELIMITER $$
CREATE PROCEDURE IF NOT EXISTS cleanup_orphaned_dockings()
BEGIN
    UPDATE ship_docking 
    SET dock_status = 'aborted', 
        undock_time = NOW()
    WHERE dock_status = 'active' 
    AND dock_time < DATE_SUB(NOW(), INTERVAL 24 HOUR);
END$$
DELIMITER ;

-- Procedure to get active dockings for a ship
DELIMITER $$
CREATE PROCEDURE IF NOT EXISTS get_active_dockings(IN p_ship_id INT)
BEGIN
    SELECT * FROM ship_docking 
    WHERE (ship1_id = p_ship_id OR ship2_id = p_ship_id)
    AND dock_status = 'active'
    ORDER BY dock_time DESC;
END$$
DELIMITER ;

-- =====================================================
-- MIGRATION NOTES
-- =====================================================
-- Run this script to create the vessel persistence tables
-- No existing data migration required for Phase 2
-- 
-- To apply this schema:
-- mysql -u root -p luminari < vessels_phase2_schema.sql