-- Canonical help-content schema for new installations.
-- Existing installations are upgraded by the collision-safe migrations in src/db_init.c.

CREATE TABLE IF NOT EXISTS help_entries (
  id INT AUTO_INCREMENT PRIMARY KEY,
  tag VARCHAR(50) NOT NULL,
  alternate_keywords TEXT DEFAULT NULL,
  category VARCHAR(50) NOT NULL DEFAULT 'general',
  entry LONGTEXT NOT NULL,
  min_level INT NOT NULL DEFAULT 0,
  max_level INT NOT NULL DEFAULT 1000,
  auto_generated BOOLEAN NOT NULL DEFAULT FALSE,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
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

CREATE TABLE IF NOT EXISTS help_related_topics (
  source_tag VARCHAR(50) NOT NULL,
  related_tag VARCHAR(50) NOT NULL,
  relevance_score DECIMAL(12,6) NOT NULL DEFAULT 1.0,
  PRIMARY KEY (source_tag, related_tag),
  INDEX idx_source (source_tag),
  INDEX idx_related (related_tag)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS help_versions (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  tag VARCHAR(50) NOT NULL,
  alternate_keywords TEXT DEFAULT NULL,
  entry LONGTEXT,
  min_level INT DEFAULT 0,
  max_level INT DEFAULT 1000,
  category VARCHAR(50) DEFAULT 'general',
  auto_generated BOOLEAN DEFAULT FALSE,
  changed_by VARCHAR(50),
  change_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  change_type ENUM('CREATE', 'UPDATE', 'DELETE') DEFAULT 'UPDATE',
  sync_plan_id VARCHAR(80) DEFAULT NULL,
  INDEX idx_tag_date (tag, change_date),
  INDEX idx_sync_plan_id (sync_plan_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS help_search_history (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  search_term VARCHAR(200) NOT NULL,
  searcher_name VARCHAR(50),
  searcher_level INT,
  results_count INT DEFAULT 0,
  search_type ENUM('keyword', 'fulltext', 'soundex') DEFAULT 'keyword',
  search_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_search_term (search_term),
  INDEX idx_search_date (search_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
