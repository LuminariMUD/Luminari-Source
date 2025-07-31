-- AI Service Database Migration Script for LuminariMUD
-- This script creates the necessary tables for AI service functionality
-- 
-- Author: Zusuk
-- Date: 2025-01-27

-- AI Configuration Table
CREATE TABLE IF NOT EXISTS ai_config (
  id INT PRIMARY KEY AUTO_INCREMENT,
  config_key VARCHAR(50) UNIQUE NOT NULL,
  config_value TEXT,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_config_key (config_key)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- AI Request Logging Table
CREATE TABLE IF NOT EXISTS ai_requests (
  id INT PRIMARY KEY AUTO_INCREMENT,
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

-- AI Response Cache Table (for database-backed caching)
CREATE TABLE IF NOT EXISTS ai_cache (
  cache_key VARCHAR(191) PRIMARY KEY,
  response TEXT,
  expires_at TIMESTAMP,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_expires (expires_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- NPC AI Personality Configuration
CREATE TABLE IF NOT EXISTS ai_npc_personalities (
  mob_vnum INT PRIMARY KEY,
  personality TEXT COMMENT 'JSON object with personality traits, background, speech patterns',
  enabled BOOLEAN DEFAULT TRUE,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_enabled (enabled)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Insert default configuration values
INSERT INTO ai_config (config_key, config_value) VALUES 
  ('enabled', 'false'),
  ('model', 'gpt-4.1-mini'),
  ('max_tokens', '500'),
  ('temperature', '0.7'),
  ('requests_per_minute', '60'),
  ('requests_per_hour', '1000'),
  ('cache_expire_seconds', '3600'),
  ('content_filter_enabled', 'true'),
  ('api_key', '')  -- Store encrypted API key here if not using file
ON DUPLICATE KEY UPDATE config_value = VALUES(config_value);

-- Create stored procedure for cache cleanup
DELIMITER $$

DROP PROCEDURE IF EXISTS cleanup_ai_cache$$
CREATE PROCEDURE cleanup_ai_cache()
BEGIN
  DELETE FROM ai_cache WHERE expires_at < NOW();
END$$

DROP PROCEDURE IF EXISTS get_ai_usage_stats$$
CREATE PROCEDURE get_ai_usage_stats(IN days INT)
BEGIN
  SELECT 
    DATE(created_at) as usage_date,
    request_type,
    COUNT(*) as request_count,
    AVG(response_time_ms) as avg_response_time,
    SUM(tokens_used) as total_tokens
  FROM ai_requests
  WHERE created_at >= DATE_SUB(NOW(), INTERVAL days DAY)
  GROUP BY DATE(created_at), request_type
  ORDER BY usage_date DESC, request_type;
END$$

DELIMITER ;

-- Create event to run cache cleanup daily
CREATE EVENT IF NOT EXISTS ai_cache_cleanup_event
ON SCHEDULE EVERY 1 DAY
DO CALL cleanup_ai_cache();

-- Example personality for an NPC (commented out by default)
-- INSERT INTO ai_npc_personalities (mob_vnum, personality) VALUES 
-- (3001, '{"name": "Grumpy Dwarf Merchant", "traits": ["gruff", "business-minded", "secretly kind"], "background": "Former miner turned merchant after injury", "speech_patterns": ["uses mining metaphors", "complains about prices", "calls everyone laddie or lassie"]}');

-- Grant permissions (adjust username as needed)
-- GRANT SELECT, INSERT, UPDATE, DELETE ON ai_config TO 'luminari_user'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON ai_requests TO 'luminari_user'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON ai_cache TO 'luminari_user'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON ai_npc_personalities TO 'luminari_user'@'localhost';
-- GRANT EXECUTE ON PROCEDURE cleanup_ai_cache TO 'luminari_user'@'localhost';
-- GRANT EXECUTE ON PROCEDURE get_ai_usage_stats TO 'luminari_user'@'localhost';