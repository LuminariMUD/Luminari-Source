-- Region Effects System
-- A flexible system for assigning various types of effects to regions
-- This allows regions to have multiple effects assigned via foreign keys

USE luminari_mudprod;

-- Main effects table - defines what effects are available
CREATE TABLE IF NOT EXISTS region_effects (
    effect_id INT AUTO_INCREMENT PRIMARY KEY,
    effect_name VARCHAR(100) NOT NULL UNIQUE,
    effect_type ENUM('resource', 'combat', 'movement', 'weather', 'magic', 'other') NOT NULL,
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

-- Insert some example resource effects
INSERT INTO region_effects (effect_name, effect_type, effect_description, effect_data) VALUES
('Forest Abundance', 'resource', 'Increases wood and food resources in forested areas', 
 JSON_OBJECT('resource_modifiers', JSON_OBJECT('wood', JSON_OBJECT('multiplier', 1.5, 'bonus', 5), 'food', JSON_OBJECT('multiplier', 1.2, 'bonus', 2)))),

('Mineral Rich', 'resource', 'Increases stone and metal resources', 
 JSON_OBJECT('resource_modifiers', JSON_OBJECT('stone', JSON_OBJECT('multiplier', 1.4, 'bonus', 3), 'metal', JSON_OBJECT('multiplier', 1.6, 'bonus', 4)))),

('Magical Essence', 'resource', 'Enhances magical resource gathering', 
 JSON_OBJECT('resource_modifiers', JSON_OBJECT('magic_essence', JSON_OBJECT('multiplier', 2.0, 'bonus', 10), 'gems', JSON_OBJECT('multiplier', 1.3, 'bonus', 3)))),

('Barren Wasteland', 'resource', 'Reduces most resource availability', 
 JSON_OBJECT('resource_modifiers', JSON_OBJECT('wood', JSON_OBJECT('multiplier', 0.3, 'bonus', -5), 'food', JSON_OBJECT('multiplier', 0.2, 'bonus', -8), 'water', JSON_OBJECT('multiplier', 0.5, 'bonus', -3)))),

('Crystal Caves', 'resource', 'Rich in gems and crystals', 
 JSON_OBJECT('resource_modifiers', JSON_OBJECT('gems', JSON_OBJECT('multiplier', 2.5, 'bonus', 8), 'crystal', JSON_OBJECT('multiplier', 3.0, 'bonus', 12)))),

('Fertile Plains', 'resource', 'Excellent for food and water', 
 JSON_OBJECT('resource_modifiers', JSON_OBJECT('food', JSON_OBJECT('multiplier', 1.8, 'bonus', 6), 'water', JSON_OBJECT('multiplier', 1.4, 'bonus', 4))));

-- Example future effect types (not implemented yet, but shows extensibility)
INSERT INTO region_effects (effect_name, effect_type, effect_description, effect_data, is_active) VALUES
('Haunted Aura', 'combat', 'Undead creatures gain bonuses in this region', 
 JSON_OBJECT('combat_modifiers', JSON_OBJECT('undead_bonus', 25, 'fear_chance', 15)), FALSE),

('Difficult Terrain', 'movement', 'Movement costs are increased', 
 JSON_OBJECT('movement_modifiers', JSON_OBJECT('cost_multiplier', 1.5, 'fatigue_increase', 10)), FALSE),

('Mystical Storms', 'weather', 'Magical weather effects occur more frequently', 
 JSON_OBJECT('weather_modifiers', JSON_OBJECT('magic_storm_chance', 30, 'mana_regen_bonus', 5)), FALSE);
