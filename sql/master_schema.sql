-- ============================================================================
-- LuminariMUD Master Database Schema
-- Sources: src/db_init.c, src/systems/pubsub/pubsub_db.c
-- This schema captures the runtime expectations baked into the codebase so
-- freshly cloned environments can provision MySQL/MariaDB consistently.
-- ============================================================================

SET NAMES utf8mb4;
SET time_zone = '+00:00';
SET FOREIGN_KEY_CHECKS = 0;

-- --------------------------------------------------------------------------
-- Core Account & Character Data
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS account_data (
  id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(50) NOT NULL,
  password VARCHAR(64) NOT NULL,
  email VARCHAR(255),
  experience INT DEFAULT 0,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_login TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  status ENUM('active', 'suspended', 'banned') DEFAULT 'active',
  UNIQUE KEY name (name),
  INDEX idx_name (name),
  INDEX idx_email (email),
  INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS player_data (
  id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(20) NOT NULL,
  password VARCHAR(32) NOT NULL,
  email VARCHAR(100),
  level INT DEFAULT 1,
  experience BIGINT DEFAULT 0,
  class INT DEFAULT 0,
  race INT DEFAULT 0,
  alignment INT DEFAULT 0,
  last_online TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  created TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  total_sessions INT DEFAULT 0,
  bad_pws INT DEFAULT 0,
  obj_save_header VARCHAR(255) DEFAULT '',
  UNIQUE KEY name (name),
  INDEX idx_name (name),
  INDEX idx_level (level),
  INDEX idx_last_online (last_online)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS unlocked_races (
  id INT AUTO_INCREMENT PRIMARY KEY,
  account_id INT NOT NULL,
  race_id INT NOT NULL,
  unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY unique_account_race (account_id, race_id),
  INDEX idx_unlocked_races_account (account_id),
  CONSTRAINT fk_unlocked_races_account
    FOREIGN KEY (account_id) REFERENCES account_data(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS unlocked_classes (
  id INT AUTO_INCREMENT PRIMARY KEY,
  account_id INT NOT NULL,
  class_id INT NOT NULL,
  unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY unique_account_class (account_id, class_id),
  INDEX idx_unlocked_classes_account (account_id),
  CONSTRAINT fk_unlocked_classes_account
    FOREIGN KEY (account_id) REFERENCES account_data(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS player_save_objs (
  id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(20) NOT NULL,
  serialized_obj LONGTEXT NOT NULL,
  creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_name (name),
  INDEX idx_creation_date (creation_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS player_save_objs_sheathed (
  id INT AUTO_INCREMENT PRIMARY KEY,
  sheath_obj_id BIGINT NOT NULL,
  sheathed_position INT NOT NULL,
  owner_name VARCHAR(20) NOT NULL,
  serialized_obj LONGTEXT NOT NULL,
  creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_player_save_objs_sheathed_owner (owner_name),
  INDEX idx_player_save_objs_sheathed_sheath (sheath_obj_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS pet_save_objs (
  id INT AUTO_INCREMENT PRIMARY KEY,
  pet_idnum BIGINT NOT NULL,
  owner_name VARCHAR(20) NOT NULL,
  serialized_obj LONGTEXT NOT NULL,
  creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_pet_save_objs_pet (pet_idnum),
  INDEX idx_pet_save_objs_owner (owner_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --------------------------------------------------------------------------
-- Object Database (in-game object exports)
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS object_database_items (
  id INT AUTO_INCREMENT PRIMARY KEY,
  idnum INT NOT NULL,
  name VARCHAR(255) NOT NULL,
  short_description TEXT,
  long_description TEXT,
  item_type INT DEFAULT 0,
  wear_flags BIGINT DEFAULT 0,
  extra_flags BIGINT DEFAULT 0,
  weight FLOAT DEFAULT 0.0,
  cost INT DEFAULT 0,
  rent_per_day INT DEFAULT 0,
  level_restriction INT DEFAULT 1,
  size INT DEFAULT 0,
  material INT DEFAULT 0,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY idnum (idnum),
  INDEX idx_idnum (idnum),
  INDEX idx_item_type (item_type),
  INDEX idx_level_restriction (level_restriction)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS object_database_wear_slots (
  id INT AUTO_INCREMENT PRIMARY KEY,
  object_idnum INT NOT NULL,
  wear_slot INT NOT NULL,
  UNIQUE KEY unique_object_slot (object_idnum, wear_slot),
  INDEX idx_wear_slot (wear_slot),
  CONSTRAINT fk_object_wear_object
    FOREIGN KEY (object_idnum) REFERENCES object_database_items(idnum) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS object_database_bonuses (
  id INT AUTO_INCREMENT PRIMARY KEY,
  object_idnum INT NOT NULL,
  bonus_location INT NOT NULL,
  bonus_value INT NOT NULL,
  INDEX idx_object_idnum (object_idnum),
  INDEX idx_bonus_location (bonus_location),
  CONSTRAINT fk_object_bonus_object
    FOREIGN KEY (object_idnum) REFERENCES object_database_items(idnum) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --------------------------------------------------------------------------
-- Wilderness Resource System
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS resource_types (
  resource_id INT PRIMARY KEY,
  resource_name VARCHAR(50) NOT NULL,
  resource_description TEXT,
  base_rarity DECIMAL(3,2) DEFAULT 1.00,
  regeneration_rate DECIMAL(4,3) DEFAULT 0.001,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY resource_name (resource_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS resource_depletion (
  id INT AUTO_INCREMENT PRIMARY KEY,
  zone_vnum INT NOT NULL,
  x_coord INT NOT NULL,
  y_coord INT NOT NULL,
  resource_type INT NOT NULL,
  depletion_level FLOAT DEFAULT 1.0,
  total_harvested INT DEFAULT 0,
  last_harvest TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  cascade_effects TEXT DEFAULT NULL,
  regeneration_rate DECIMAL(5,4) DEFAULT 0.0010,
  last_regeneration TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY unique_location_resource (zone_vnum, x_coord, y_coord, resource_type),
  INDEX idx_location (zone_vnum, x_coord, y_coord),
  INDEX idx_resource (resource_type),
  INDEX idx_depletion (depletion_level),
  INDEX idx_last_harvest (last_harvest),
  CONSTRAINT fk_resource_depletion_type
    FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS player_conservation (
  id INT AUTO_INCREMENT PRIMARY KEY,
  player_id BIGINT NOT NULL,
  conservation_score FLOAT DEFAULT 0.5,
  total_harvests INT DEFAULT 0,
  sustainable_harvests INT DEFAULT 0,
  unsustainable_harvests INT DEFAULT 0,
  ecosystem_damage DECIMAL(8,3) DEFAULT 0.0,
  last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY unique_player (player_id),
  INDEX idx_player_id (player_id),
  INDEX idx_conservation_score (conservation_score)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS resource_statistics (
  id INT AUTO_INCREMENT PRIMARY KEY,
  resource_type INT NOT NULL,
  total_harvested BIGINT DEFAULT 0,
  total_depleted_locations INT DEFAULT 0,
  average_depletion_level FLOAT DEFAULT 1.0,
  peak_depletion_level FLOAT DEFAULT 1.0,
  critical_locations INT DEFAULT 0,
  last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY unique_resource (resource_type),
  CONSTRAINT fk_resource_statistics_type
    FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

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
  weather_factor FLOAT DEFAULT 1.0,
  seasonal_factor FLOAT DEFAULT 1.0,
  INDEX idx_location_regen (zone_vnum, x_coord, y_coord),
  INDEX idx_time_regen (regeneration_time),
  INDEX idx_resource_type (resource_type),
  CONSTRAINT fk_resource_regen_type
    FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS resource_relationships (
  id INT AUTO_INCREMENT PRIMARY KEY,
  source_resource INT NOT NULL,
  target_resource INT NOT NULL,
  effect_type ENUM('depletion', 'enhancement', 'threshold') NOT NULL,
  effect_magnitude DECIMAL(5,3) NOT NULL,
  threshold_min DECIMAL(4,3) DEFAULT NULL,
  threshold_max DECIMAL(4,3) DEFAULT NULL,
  description VARCHAR(255),
  is_active BOOLEAN DEFAULT TRUE,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_source_resource (source_resource),
  INDEX idx_target_resource (target_resource),
  INDEX idx_effect_type (effect_type),
  INDEX idx_is_active (is_active),
  CONSTRAINT fk_resource_relationship_source
    FOREIGN KEY (source_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE,
  CONSTRAINT fk_resource_relationship_target
    FOREIGN KEY (target_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS ecosystem_health (
  id INT AUTO_INCREMENT PRIMARY KEY,
  zone_vnum INT NOT NULL,
  x_coord INT NOT NULL,
  y_coord INT NOT NULL,
  health_score DECIMAL(4,3) DEFAULT 1.000,
  health_state ENUM('pristine', 'healthy', 'degraded', 'critical', 'devastated') DEFAULT 'healthy',
  biodiversity_index DECIMAL(4,3) DEFAULT 1.000,
  pollution_level DECIMAL(4,3) DEFAULT 0.000,
  last_assessment TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY unique_location (zone_vnum, x_coord, y_coord),
  INDEX idx_health_state (health_state),
  INDEX idx_health_score (health_score),
  INDEX idx_zone (zone_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS cascade_effects_log (
  id INT AUTO_INCREMENT PRIMARY KEY,
  zone_vnum INT NOT NULL,
  x_coord INT NOT NULL,
  y_coord INT NOT NULL,
  source_resource INT NOT NULL,
  target_resource INT NOT NULL,
  effect_magnitude DECIMAL(5,3) NOT NULL,
  player_id BIGINT DEFAULT NULL,
  harvest_amount INT DEFAULT 0,
  logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_cascade_location_time (zone_vnum, x_coord, y_coord, logged_at),
  INDEX idx_cascade_resources (source_resource, target_resource),
  INDEX idx_cascade_player (player_id),
  CONSTRAINT fk_cascade_source_resource
    FOREIGN KEY (source_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE,
  CONSTRAINT fk_cascade_target_resource
    FOREIGN KEY (target_resource) REFERENCES resource_types(resource_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS player_location_conservation (
  id INT AUTO_INCREMENT PRIMARY KEY,
  player_id BIGINT NOT NULL,
  zone_vnum INT NOT NULL,
  x_coord INT NOT NULL,
  y_coord INT NOT NULL,
  visits INT DEFAULT 0,
  total_harvested INT DEFAULT 0,
  sustainable_actions INT DEFAULT 0,
  destructive_actions INT DEFAULT 0,
  last_visit TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  conservation_rating DECIMAL(4,3) DEFAULT 0.500,
  UNIQUE KEY unique_player_location (player_id, zone_vnum, x_coord, y_coord),
  INDEX idx_player_id (player_id),
  INDEX idx_location (zone_vnum, x_coord, y_coord),
  INDEX idx_conservation_rating (conservation_rating)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --------------------------------------------------------------------------
-- Environmental Effects, Materials, and Room Overrides
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS region_effects (
  effect_id INT AUTO_INCREMENT PRIMARY KEY,
  effect_name VARCHAR(100) NOT NULL,
  effect_type ENUM('weather', 'magical', 'seasonal', 'geological', 'biological') NOT NULL,
  effect_description TEXT,
  effect_data JSON DEFAULT NULL,
  intensity_min DECIMAL(3,2) DEFAULT 0.10,
  intensity_max DECIMAL(3,2) DEFAULT 1.00,
  duration_hours INT DEFAULT 24,
  is_active BOOLEAN DEFAULT TRUE,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_region_effect_type (effect_type),
  INDEX idx_region_effect_active (is_active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS region_effect_assignments (
  id INT AUTO_INCREMENT PRIMARY KEY,
  region_vnum INT NOT NULL,
  effect_id INT NOT NULL,
  intensity DECIMAL(3,2) DEFAULT 1.00,
  start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  end_time TIMESTAMP NULL,
  is_permanent BOOLEAN DEFAULT FALSE,
  assigned_by VARCHAR(50) DEFAULT 'system',
  INDEX idx_region_vnum (region_vnum),
  INDEX idx_effect_id (effect_id),
  INDEX idx_active_assignments (region_vnum, end_time),
  CONSTRAINT fk_region_effect_assignment_effect
    FOREIGN KEY (effect_id) REFERENCES region_effects(effect_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS weather_cache (
  id INT AUTO_INCREMENT PRIMARY KEY,
  zone_vnum INT NOT NULL,
  x_coord INT NOT NULL,
  y_coord INT NOT NULL,
  weather_type INT NOT NULL,
  temperature INT DEFAULT 20,
  humidity DECIMAL(3,2) DEFAULT 0.50,
  wind_speed INT DEFAULT 5,
  cached_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  expires_at TIMESTAMP DEFAULT (CURRENT_TIMESTAMP + INTERVAL 1 HOUR),
  UNIQUE KEY unique_location_weather (zone_vnum, x_coord, y_coord),
  INDEX idx_expires_at (expires_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS room_description_settings (
  id INT AUTO_INCREMENT PRIMARY KEY,
  zone_vnum INT NOT NULL,
  x_coord INT NOT NULL,
  y_coord INT NOT NULL,
  enable_dynamic_descriptions BOOLEAN DEFAULT TRUE,
  enable_weather_effects BOOLEAN DEFAULT TRUE,
  enable_resource_descriptions BOOLEAN DEFAULT TRUE,
  enable_seasonal_changes BOOLEAN DEFAULT TRUE,
  custom_description_override TEXT DEFAULT NULL,
  last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY unique_location_settings (zone_vnum, x_coord, y_coord),
  INDEX idx_room_description_settings_zone (zone_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS material_categories (
  category_id INT PRIMARY KEY,
  category_name VARCHAR(50) NOT NULL,
  category_description TEXT,
  resource_type INT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY category_name (category_name),
  CONSTRAINT fk_material_category_resource
    FOREIGN KEY (resource_type) REFERENCES resource_types(resource_id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS material_subtypes (
  subtype_id INT AUTO_INCREMENT PRIMARY KEY,
  category_id INT NOT NULL,
  subtype_name VARCHAR(50) NOT NULL,
  subtype_description TEXT,
  rarity_modifier DECIMAL(3,2) DEFAULT 1.00,
  value_modifier DECIMAL(3,2) DEFAULT 1.00,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY unique_category_subtype (category_id, subtype_name),
  INDEX idx_material_subtypes_category (category_id),
  CONSTRAINT fk_material_subtype_category
    FOREIGN KEY (category_id) REFERENCES material_categories(category_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS material_qualities (
  quality_id INT PRIMARY KEY,
  quality_name VARCHAR(30) NOT NULL,
  quality_description TEXT,
  rarity_multiplier DECIMAL(4,2) DEFAULT 1.00,
  value_multiplier DECIMAL(4,2) DEFAULT 1.00,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY quality_name (quality_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --------------------------------------------------------------------------
-- Region Geometry & Spatial Indexes
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS region_data (
  vnum INT PRIMARY KEY,
  zone_vnum INT NOT NULL,
  name VARCHAR(50),
  region_type INT NOT NULL,
  region_polygon POLYGON,
  region_props INT,
  region_reset_data VARCHAR(255) NOT NULL,
  region_reset_time DATETIME,
  region_description LONGTEXT DEFAULT NULL,
  description_version INT DEFAULT 1,
  ai_agent_source VARCHAR(100) DEFAULT NULL,
  last_description_update TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  description_style ENUM('poetic', 'practical', 'mysterious', 'dramatic', 'pastoral') DEFAULT 'poetic',
  description_length ENUM('brief', 'moderate', 'detailed', 'extensive') DEFAULT 'moderate',
  has_historical_context BOOLEAN DEFAULT FALSE,
  has_resource_info BOOLEAN DEFAULT FALSE,
  has_wildlife_info BOOLEAN DEFAULT FALSE,
  has_geological_info BOOLEAN DEFAULT FALSE,
  has_cultural_info BOOLEAN DEFAULT FALSE,
  description_quality_score DECIMAL(3,2) DEFAULT NULL,
  requires_review BOOLEAN DEFAULT FALSE,
  is_approved BOOLEAN DEFAULT FALSE,
  INDEX idx_vnum (vnum),
  INDEX idx_zone_vnum (zone_vnum),
  INDEX idx_region_type (region_type),
  INDEX idx_region_has_description (region_description(100)),
  INDEX idx_region_description_approved (is_approved, requires_review),
  INDEX idx_region_description_quality (description_quality_score),
  INDEX idx_region_ai_source (ai_agent_source)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS path_data (
  vnum INT PRIMARY KEY,
  zone_vnum INT NOT NULL DEFAULT 10000,
  path_type INT NOT NULL,
  name VARCHAR(50) NOT NULL,
  path_props INT,
  path_linestring LINESTRING,
  INDEX idx_vnum (vnum),
  INDEX idx_zone_vnum (zone_vnum),
  INDEX idx_path_type (path_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS region_index (
  vnum INT PRIMARY KEY,
  zone_vnum INT NOT NULL,
  region_polygon POLYGON NOT NULL,
  SPATIAL INDEX region_polygon (region_polygon)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS path_index (
  vnum INT PRIMARY KEY,
  zone_vnum INT NOT NULL,
  path_linestring LINESTRING NOT NULL,
  SPATIAL INDEX path_linestring (path_linestring)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --------------------------------------------------------------------------
-- Region Hinting & Narrative Systems
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS region_hints (
  id INT AUTO_INCREMENT PRIMARY KEY,
  region_vnum INT NOT NULL,
  hint_category ENUM(
    'atmosphere', 'fauna', 'flora', 'geography', 'weather_influence',
    'resources', 'landmarks', 'sounds', 'scents', 'seasonal_changes',
    'time_of_day', 'mystical'
  ) NOT NULL,
  hint_text TEXT NOT NULL,
  priority TINYINT DEFAULT 5,
  seasonal_weight JSON DEFAULT NULL,
  weather_conditions SET('clear', 'cloudy', 'rainy', 'stormy', 'lightning')
    DEFAULT 'clear,cloudy,rainy,stormy,lightning',
  time_of_day_weight JSON DEFAULT NULL,
  resource_triggers JSON DEFAULT NULL,
  agent_id VARCHAR(100) DEFAULT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  is_active BOOLEAN DEFAULT TRUE,
  INDEX idx_region_category (region_vnum, hint_category),
  INDEX idx_priority (priority),
  INDEX idx_active (is_active),
  INDEX idx_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS region_profiles (
  region_vnum INT PRIMARY KEY,
  overall_theme TEXT,
  dominant_mood VARCHAR(100),
  key_characteristics JSON,
  description_style ENUM('poetic', 'practical', 'mysterious', 'dramatic', 'pastoral') DEFAULT 'poetic',
  complexity_level TINYINT DEFAULT 3,
  agent_id VARCHAR(100),
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS hint_usage_log (
  id INT AUTO_INCREMENT PRIMARY KEY,
  hint_id INT NOT NULL,
  room_vnum INT NOT NULL,
  player_id INT DEFAULT NULL,
  used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  weather_condition VARCHAR(20),
  season VARCHAR(10),
  time_of_day VARCHAR(10),
  resource_state JSON DEFAULT NULL,
  INDEX idx_hint_usage_hint (hint_id, used_at),
  INDEX idx_hint_usage_room (room_vnum, used_at),
  CONSTRAINT fk_hint_usage_hint
    FOREIGN KEY (hint_id) REFERENCES region_hints(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS description_templates (
  id INT AUTO_INCREMENT PRIMARY KEY,
  region_vnum INT NOT NULL,
  template_type ENUM('intro', 'weather_overlay', 'resource_state', 'time_transition') NOT NULL,
  template_text TEXT NOT NULL,
  placeholder_schema JSON,
  conditions JSON DEFAULT NULL,
  agent_id VARCHAR(100),
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  is_active BOOLEAN DEFAULT TRUE,
  INDEX idx_region_type (region_vnum, template_type),
  INDEX idx_active_templates (is_active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --------------------------------------------------------------------------
-- AI Service Configuration
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS ai_config (
  id INT AUTO_INCREMENT PRIMARY KEY,
  config_key VARCHAR(50) NOT NULL,
  config_value TEXT,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY config_key (config_key),
  INDEX idx_config_key (config_key)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS ai_requests (
  id INT AUTO_INCREMENT PRIMARY KEY,
  request_type ENUM('npc_dialogue', 'room_desc', 'quest_gen', 'moderation', 'test') NOT NULL,
  prompt TEXT,
  response TEXT,
  tokens_used INT,
  response_time_ms INT,
  player_id INT,
  npc_vnum INT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_created (created_at),
  INDEX idx_player (player_id),
  INDEX idx_request_type (request_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS ai_cache (
  cache_key VARCHAR(191) PRIMARY KEY,
  response TEXT,
  expires_at TIMESTAMP,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_expires (expires_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS ai_npc_personalities (
  mob_vnum INT PRIMARY KEY,
  personality TEXT,
  enabled BOOLEAN DEFAULT TRUE,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_enabled (enabled)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --------------------------------------------------------------------------
-- Crafting & Housing Systems
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS supply_orders_available (
  idnum INT AUTO_INCREMENT PRIMARY KEY,
  player_name VARCHAR(20) NOT NULL,
  supply_orders_available INT DEFAULT 0,
  last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY unique_player_name (player_name),
  INDEX idx_player_name (player_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS house_data (
  id INT AUTO_INCREMENT PRIMARY KEY,
  vnum INT NOT NULL,
  serialized_obj LONGTEXT,
  creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_accessed TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY vnum (vnum),
  INDEX idx_vnum (vnum),
  INDEX idx_last_accessed (last_accessed)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --------------------------------------------------------------------------
-- Schema Migrations & Help System
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS schema_migrations (
  version INT NOT NULL PRIMARY KEY COMMENT 'Schema version number',
  description VARCHAR(255) NOT NULL COMMENT 'Description of migration',
  applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When migration was applied',
  INDEX idx_version (version)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Tracks database schema migrations';

CREATE TABLE IF NOT EXISTS help_versions (
  id INT AUTO_INCREMENT PRIMARY KEY,
  tag VARCHAR(50) NOT NULL,
  entry TEXT,
  min_level INT DEFAULT 0,
  changed_by VARCHAR(50),
  change_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  change_type ENUM('CREATE', 'UPDATE', 'DELETE') DEFAULT 'UPDATE',
  INDEX idx_tag_date (tag, change_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Version history for help entries';

CREATE TABLE IF NOT EXISTS help_search_history (
  id INT AUTO_INCREMENT PRIMARY KEY,
  search_term VARCHAR(200) NOT NULL,
  searcher_name VARCHAR(50),
  searcher_level INT,
  results_count INT DEFAULT 0,
  search_type ENUM('keyword', 'fulltext', 'soundex') DEFAULT 'keyword',
  search_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_search_term (search_term),
  INDEX idx_search_date (search_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Tracks help searches for analytics';

CREATE TABLE IF NOT EXISTS help_related_topics (
  source_tag VARCHAR(50) NOT NULL,
  related_tag VARCHAR(50) NOT NULL,
  relevance_score FLOAT DEFAULT 1.0,
  PRIMARY KEY (source_tag, related_tag),
  INDEX idx_source (source_tag),
  INDEX idx_related (related_tag)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Links between related help topics';

CREATE TABLE IF NOT EXISTS help_entries (
  id INT AUTO_INCREMENT PRIMARY KEY,
  tag VARCHAR(50) NOT NULL,
  category VARCHAR(50) DEFAULT 'general' COMMENT 'Help category for browsing',
  entry LONGTEXT NOT NULL,
  min_level INT DEFAULT 0,
  max_level INT DEFAULT 1000,
  auto_generated BOOLEAN DEFAULT FALSE COMMENT 'TRUE if auto-generated, FALSE if manual',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY tag (tag),
  INDEX idx_help_tag (tag),
  INDEX idx_category (category),
  INDEX idx_min_level (min_level),
  INDEX idx_auto_generated (auto_generated),
  FULLTEXT KEY idx_help_entries_fulltext (entry)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS help_keywords (
  id INT AUTO_INCREMENT PRIMARY KEY,
  help_tag VARCHAR(50) NOT NULL,
  keyword VARCHAR(100) NOT NULL,
  INDEX idx_keyword (keyword),
  INDEX idx_help_tag (help_tag),
  INDEX idx_help_keywords_composite (help_tag, keyword),
  UNIQUE KEY unique_tag_keyword (help_tag, keyword),
  CONSTRAINT fk_help_keywords_entry
    FOREIGN KEY (help_tag) REFERENCES help_entries(tag) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --------------------------------------------------------------------------
-- Vessel Persistence System
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS ship_interiors (
  ship_id VARCHAR(8) NOT NULL PRIMARY KEY,
  vessel_type INT NOT NULL DEFAULT 0,
  vessel_name VARCHAR(100),
  num_rooms INT NOT NULL DEFAULT 1,
  max_rooms INT NOT NULL DEFAULT 20,
  room_vnums TEXT,
  bridge_room INT DEFAULT 0,
  entrance_room INT DEFAULT 0,
  cargo_room1 INT DEFAULT 0,
  cargo_room2 INT DEFAULT 0,
  cargo_room3 INT DEFAULT 0,
  cargo_room4 INT DEFAULT 0,
  cargo_room5 INT DEFAULT 0,
  room_data LONGBLOB,
  connection_data LONGBLOB,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_modified TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_vessel_type (vessel_type),
  INDEX idx_vessel_name (vessel_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS ship_docking (
  dock_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  ship1_id VARCHAR(8) NOT NULL,
  ship2_id VARCHAR(8) NOT NULL,
  dock_room1 INT NOT NULL,
  dock_room2 INT NOT NULL,
  dock_type ENUM('standard','combat','emergency','forced') DEFAULT 'standard',
  dock_status ENUM('active','completed','aborted') DEFAULT 'active',
  dock_x INT DEFAULT 0,
  dock_y INT DEFAULT 0,
  dock_z INT DEFAULT 0,
  dock_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  undock_time TIMESTAMP NULL DEFAULT NULL,
  INDEX idx_ship1 (ship1_id),
  INDEX idx_ship2 (ship2_id),
  INDEX idx_active (dock_status),
  INDEX idx_dock_time (dock_time),
  UNIQUE KEY unique_active_dock (ship1_id, ship2_id, dock_status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS ship_room_templates (
  template_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  room_type VARCHAR(50) NOT NULL,
  vessel_type INT DEFAULT 0,
  name_format VARCHAR(200),
  description_text TEXT,
  room_flags INT DEFAULT 0,
  sector_type INT DEFAULT 0,
  min_vessel_size INT DEFAULT 0,
  max_vessel_size INT DEFAULT 99,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_room_type (room_type),
  INDEX idx_vessel_type (vessel_type),
  UNIQUE KEY unique_room_type (room_type, vessel_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS ship_cargo_manifest (
  manifest_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  ship_id VARCHAR(8) NOT NULL,
  cargo_room INT NOT NULL,
  item_vnum INT NOT NULL,
  item_name VARCHAR(100),
  item_count INT DEFAULT 1,
  item_weight INT DEFAULT 0,
  loaded_by VARCHAR(50),
  loaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_ship (ship_id),
  INDEX idx_room (cargo_room),
  INDEX idx_item (item_vnum),
  CONSTRAINT fk_ship_cargo_ship
    FOREIGN KEY (ship_id) REFERENCES ship_interiors(ship_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS ship_crew_roster (
  roster_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  ship_id VARCHAR(8) NOT NULL,
  npc_vnum INT NOT NULL,
  npc_name VARCHAR(100),
  crew_role ENUM('captain','pilot','gunner','engineer','medic','marine','crew') DEFAULT 'crew',
  assigned_room INT DEFAULT 0,
  duty_station INT DEFAULT 0,
  loyalty_rating INT DEFAULT 50,
  status ENUM('active','injured','awol','dead') DEFAULT 'active',
  hired_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_ship (ship_id),
  INDEX idx_role (crew_role),
  INDEX idx_status (status),
  CONSTRAINT fk_ship_crew_ship
    FOREIGN KEY (ship_id) REFERENCES ship_interiors(ship_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --------------------------------------------------------------------------
-- PubSub Messaging System
-- --------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS pubsub_topics (
  topic_id INT AUTO_INCREMENT PRIMARY KEY,
  topic_name VARCHAR(128) NOT NULL,
  description TEXT,
  created_by VARCHAR(80),
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  is_active BOOLEAN DEFAULT TRUE,
  subscriber_count INT DEFAULT 0,
  message_count INT DEFAULT 0,
  UNIQUE KEY topic_name (topic_name),
  INDEX idx_active (is_active),
  INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS pubsub_subscriptions (
  subscription_id INT AUTO_INCREMENT PRIMARY KEY,
  topic_id INT NOT NULL,
  player_name VARCHAR(80) NOT NULL,
  subscribed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  is_active BOOLEAN DEFAULT TRUE,
  UNIQUE KEY unique_subscription (topic_id, player_name),
  INDEX idx_player (player_name),
  INDEX idx_active (is_active),
  CONSTRAINT fk_pubsub_subscription_topic
    FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS pubsub_messages (
  message_id INT AUTO_INCREMENT PRIMARY KEY,
  topic_id INT NOT NULL,
  sender_name VARCHAR(80) NOT NULL,
  sender_id BIGINT,
  message_type INT NOT NULL DEFAULT 0,
  message_category INT NOT NULL DEFAULT 1,
  priority INT NOT NULL DEFAULT 2,
  content TEXT,
  content_type VARCHAR(64) DEFAULT 'text/plain',
  content_encoding VARCHAR(32) DEFAULT 'utf-8',
  content_version INT DEFAULT 1,
  spatial_data TEXT,
  legacy_metadata TEXT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  expires_at TIMESTAMP NULL,
  last_modified_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  parent_message_id INT NULL,
  thread_id INT NULL,
  sequence_number INT DEFAULT 0,
  delivery_attempts INT DEFAULT 0,
  successful_deliveries INT DEFAULT 0,
  failed_deliveries INT DEFAULT 0,
  is_processed BOOLEAN DEFAULT FALSE,
  processed_at TIMESTAMP NULL,
  reference_count INT DEFAULT 1,
  INDEX idx_topic_created (topic_id, created_at),
  INDEX idx_type_category (message_type, message_category),
  INDEX idx_thread (thread_id, sequence_number),
  INDEX idx_parent (parent_message_id),
  INDEX idx_processed (is_processed),
  INDEX idx_expires (expires_at),
  CONSTRAINT fk_pubsub_message_topic
    FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE,
  CONSTRAINT fk_pubsub_message_parent
    FOREIGN KEY (parent_message_id) REFERENCES pubsub_messages(message_id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS pubsub_message_metadata (
  metadata_id INT AUTO_INCREMENT PRIMARY KEY,
  message_id INT NOT NULL,
  sender_real_name VARCHAR(100),
  sender_title VARCHAR(255),
  sender_level INT,
  sender_class VARCHAR(50),
  sender_race VARCHAR(50),
  origin_room INT,
  origin_zone INT,
  origin_area_name VARCHAR(255),
  origin_x INT DEFAULT 0,
  origin_y INT DEFAULT 0,
  origin_z INT DEFAULT 0,
  context_type VARCHAR(100),
  trigger_event VARCHAR(100),
  related_object VARCHAR(100),
  related_object_id INT,
  handler_chain TEXT,
  processing_time_ms BIGINT DEFAULT 0,
  processing_notes TEXT,
  INDEX idx_message_id (message_id),
  INDEX idx_origin_location (origin_x, origin_y, origin_z),
  CONSTRAINT fk_pubsub_metadata_message
    FOREIGN KEY (message_id) REFERENCES pubsub_messages(message_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS pubsub_message_fields (
  field_id INT AUTO_INCREMENT PRIMARY KEY,
  message_id INT NOT NULL,
  field_name VARCHAR(128) NOT NULL,
  field_value TEXT,
  field_type VARCHAR(32) DEFAULT 'string',
  field_order INT DEFAULT 0,
  INDEX idx_message_field (message_id, field_name),
  INDEX idx_field_type (field_type),
  CONSTRAINT fk_pubsub_fields_message
    FOREIGN KEY (message_id) REFERENCES pubsub_messages(message_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS pubsub_messages_v3 (
  message_id INT AUTO_INCREMENT PRIMARY KEY,
  topic_id INT NOT NULL,
  sender_name VARCHAR(80) NOT NULL,
  sender_id BIGINT,
  message_type INT NOT NULL DEFAULT 0,
  message_category INT NOT NULL DEFAULT 1,
  priority INT NOT NULL DEFAULT 2,
  content TEXT,
  content_type VARCHAR(64) DEFAULT 'text/plain',
  content_encoding VARCHAR(32) DEFAULT 'utf-8',
  content_version INT DEFAULT 1,
  spatial_data TEXT,
  legacy_metadata TEXT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  expires_at TIMESTAMP NULL,
  last_modified_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  parent_message_id INT NULL,
  thread_id INT NULL,
  sequence_number INT DEFAULT 0,
  delivery_attempts INT DEFAULT 0,
  successful_deliveries INT DEFAULT 0,
  failed_deliveries INT DEFAULT 0,
  is_processed BOOLEAN DEFAULT FALSE,
  processed_at TIMESTAMP NULL,
  reference_count INT DEFAULT 1,
  INDEX idx_topic_created (topic_id, created_at),
  INDEX idx_type_category (message_type, message_category),
  INDEX idx_thread (thread_id, sequence_number),
  INDEX idx_parent (parent_message_id),
  INDEX idx_processed (is_processed),
  INDEX idx_expires (expires_at),
  CONSTRAINT fk_pubsub_v3_topic
    FOREIGN KEY (topic_id) REFERENCES pubsub_topics(topic_id) ON DELETE CASCADE,
  CONSTRAINT fk_pubsub_v3_parent
    FOREIGN KEY (parent_message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS pubsub_message_metadata_v3 (
  metadata_id INT AUTO_INCREMENT PRIMARY KEY,
  message_id INT NOT NULL,
  sender_real_name VARCHAR(80),
  sender_title VARCHAR(255),
  sender_level INT,
  sender_class VARCHAR(64),
  sender_race VARCHAR(64),
  origin_room INT,
  origin_zone INT,
  origin_area_name VARCHAR(128),
  origin_x INT,
  origin_y INT,
  origin_z INT,
  context_type VARCHAR(64),
  trigger_event VARCHAR(128),
  related_object VARCHAR(128),
  related_object_id INT,
  handler_chain TEXT,
  processing_time_ms INT DEFAULT 0,
  processing_notes TEXT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY unique_message_metadata (message_id),
  INDEX idx_sender (sender_real_name),
  INDEX idx_origin (origin_room, origin_zone),
  INDEX idx_context (context_type),
  CONSTRAINT fk_pubsub_v3_metadata_message
    FOREIGN KEY (message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS pubsub_message_fields_v3 (
  field_id INT AUTO_INCREMENT PRIMARY KEY,
  message_id INT NOT NULL,
  field_name VARCHAR(128) NOT NULL,
  field_type INT NOT NULL,
  string_value TEXT,
  integer_value BIGINT,
  float_value DOUBLE,
  boolean_value BOOLEAN,
  timestamp_value TIMESTAMP NULL,
  json_value JSON,
  player_ref_value BIGINT,
  location_ref_room INT,
  location_ref_zone INT,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_message_field (message_id, field_name),
  INDEX idx_field_type (field_type),
  INDEX idx_string_value (string_value(255)),
  INDEX idx_integer_value (integer_value),
  CONSTRAINT fk_pubsub_v3_fields_message
    FOREIGN KEY (message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS pubsub_message_tags_v3 (
  tag_id INT AUTO_INCREMENT PRIMARY KEY,
  message_id INT NOT NULL,
  tag_category VARCHAR(64) NOT NULL,
  tag_name VARCHAR(128) NOT NULL,
  tag_value VARCHAR(255),
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_message_tag (message_id, tag_name),
  INDEX idx_category_tag (tag_category, tag_name),
  INDEX idx_tag_value (tag_value),
  CONSTRAINT fk_pubsub_v3_tags_message
    FOREIGN KEY (message_id) REFERENCES pubsub_messages_v3(message_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

SET FOREIGN_KEY_CHECKS = 1;

-- --------------------------------------------------------------------------
-- Views
-- --------------------------------------------------------------------------

CREATE OR REPLACE VIEW active_region_hints AS
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

CREATE OR REPLACE VIEW hint_analytics AS
SELECT
  rh.region_vnum,
  rh.hint_category,
  COUNT(hul.id) AS usage_count,
  AVG(rh.priority) AS avg_priority,
  MAX(hul.used_at) AS last_used,
  COUNT(DISTINCT hul.room_vnum) AS unique_rooms
FROM region_hints rh
LEFT JOIN hint_usage_log hul ON rh.id = hul.hint_id
WHERE rh.is_active = TRUE
GROUP BY rh.region_vnum, rh.hint_category;

-- --------------------------------------------------------------------------
-- Stored Procedures, Functions, and Triggers
-- --------------------------------------------------------------------------

DROP PROCEDURE IF EXISTS bresenham_segment;
DELIMITER $$
CREATE PROCEDURE bresenham_segment(
  IN x1 INT, IN y1 INT, IN x2 INT, IN y2 INT,
  OUT points_wkt TEXT
)
BEGIN
  DECLARE dx INT;
  DECLARE dy INT;
  DECLARE sx INT;
  DECLARE sy INT;
  DECLARE err INT;
  DECLARE e2 INT;
  DECLARE x INT;
  DECLARE y INT;
  DECLARE first_point BOOLEAN DEFAULT TRUE;
  DECLARE continue_loop BOOLEAN DEFAULT TRUE;

  SET points_wkt = '';
  SET x = x1;
  SET y = y1;
  SET dx = ABS(x2 - x1);
  SET dy = ABS(y2 - y1);
  SET sx = IF(x1 < x2, 1, -1);
  SET sy = IF(y1 < y2, 1, -1);
  SET err = dx - dy;

  WHILE continue_loop DO
    IF first_point THEN
      SET points_wkt = CONCAT(x, ' ', y);
      SET first_point = FALSE;
    ELSE
      SET points_wkt = CONCAT(points_wkt, ',', x, ' ', y);
    END IF;

    IF x = x2 AND y = y2 THEN
      SET continue_loop = FALSE;
    ELSE
      SET e2 = 2 * err;
      IF e2 > -dy THEN
        SET err = err - dy;
        SET x = x + sx;
      END IF;
      IF e2 < dx THEN
        SET err = err + dx;
        SET y = y + sy;
      END IF;
    END IF;
  END WHILE;
END$$
DELIMITER ;

DROP FUNCTION IF EXISTS bresenham_line;
DELIMITER $$
CREATE FUNCTION bresenham_line(input_linestring LINESTRING)
RETURNS LINESTRING
READS SQL DATA
DETERMINISTIC
BEGIN
  DECLARE point_count INT;
  DECLARE i INT DEFAULT 1;
  DECLARE x1 INT;
  DECLARE y1 INT;
  DECLARE x2 INT;
  DECLARE y2 INT;
  DECLARE result_wkt TEXT DEFAULT 'LINESTRING(';
  DECLARE first_point BOOLEAN DEFAULT TRUE;

  SET point_count = ST_NumPoints(input_linestring);
  IF point_count < 2 THEN
    RETURN input_linestring;
  END IF;

  WHILE i < point_count DO
    SET x1 = CAST(ST_X(ST_PointN(input_linestring, i)) AS SIGNED);
    SET y1 = CAST(ST_Y(ST_PointN(input_linestring, i)) AS SIGNED);
    SET x2 = CAST(ST_X(ST_PointN(input_linestring, i + 1)) AS SIGNED);
    SET y2 = CAST(ST_Y(ST_PointN(input_linestring, i + 1)) AS SIGNED);

    CALL bresenham_segment(x1, y1, x2, y2, @segment_points);

    IF i = 1 THEN
      SET result_wkt = CONCAT(result_wkt, @segment_points);
    ELSE
      SET @segment_points = SUBSTRING(@segment_points, LOCATE(',', @segment_points) + 1);
      SET result_wkt = CONCAT(result_wkt, ',', @segment_points);
    END IF;

    SET i = i + 1;
  END WHILE;

  SET result_wkt = CONCAT(result_wkt, ')');
  RETURN ST_GeomFromText(result_wkt);
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS bi_digitalize_linestring;
DELIMITER $$
CREATE TRIGGER bi_digitalize_linestring
BEFORE INSERT ON path_data
FOR EACH ROW
BEGIN
  IF NEW.path_linestring IS NOT NULL THEN
    SET NEW.path_linestring = bresenham_line(NEW.path_linestring);
  END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS bu_digitalize_linestring;
DELIMITER $$
CREATE TRIGGER bu_digitalize_linestring
BEFORE UPDATE ON path_data
FOR EACH ROW
BEGIN
  IF NEW.path_linestring IS NOT NULL THEN
    SET NEW.path_linestring = bresenham_line(NEW.path_linestring);
  END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS ai_maintain_path_index;
DELIMITER $$
CREATE TRIGGER ai_maintain_path_index
AFTER INSERT ON path_data
FOR EACH ROW
BEGIN
  INSERT INTO path_index (vnum, zone_vnum, path_linestring)
  VALUES (NEW.vnum, NEW.zone_vnum, NEW.path_linestring);
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS au_maintain_path_index;
DELIMITER $$
CREATE TRIGGER au_maintain_path_index
AFTER UPDATE ON path_data
FOR EACH ROW
BEGIN
  UPDATE path_index
  SET zone_vnum = NEW.zone_vnum,
      path_linestring = NEW.path_linestring
  WHERE vnum = NEW.vnum;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS ad_maintain_path_index;
DELIMITER $$
CREATE TRIGGER ad_maintain_path_index
AFTER DELETE ON path_data
FOR EACH ROW
BEGIN
  DELETE FROM path_index WHERE vnum = OLD.vnum;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS AI_MAINTAIN_REGION_INDEX;
DELIMITER $$
CREATE TRIGGER AI_MAINTAIN_REGION_INDEX
AFTER INSERT ON region_data
FOR EACH ROW
BEGIN
  INSERT INTO region_index (vnum, zone_vnum, region_polygon)
  VALUES (NEW.vnum, NEW.zone_vnum, NEW.region_polygon);
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS AU_MAINTAIN_REGION_INDEX;
DELIMITER $$
CREATE TRIGGER AU_MAINTAIN_REGION_INDEX
AFTER UPDATE ON region_data
FOR EACH ROW
BEGIN
  UPDATE region_index
  SET zone_vnum = NEW.zone_vnum,
      region_polygon = NEW.region_polygon
  WHERE vnum = NEW.vnum;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS AD_MAINTAIN_REGION_INDEX;
DELIMITER $$
CREATE TRIGGER AD_MAINTAIN_REGION_INDEX
AFTER DELETE ON region_data
FOR EACH ROW
BEGIN
  DELETE FROM region_index WHERE vnum = OLD.vnum;
END$$
DELIMITER ;
