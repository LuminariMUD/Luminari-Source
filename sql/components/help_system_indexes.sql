-- Help System Performance Optimization Indexes
-- Created: August 12, 2025
-- Purpose: Improve query performance for help system operations

-- Check if indexes already exist before creating them
-- This prevents errors when running the script multiple times

-- Index on help_entries.tag for faster lookups (may already exist)
ALTER TABLE help_entries ADD INDEX IF NOT EXISTS idx_help_entries_tag (tag);

-- Index on help_entries.min_level for permission filtering (may already exist)
ALTER TABLE help_entries ADD INDEX IF NOT EXISTS idx_help_entries_min_level (min_level);

-- Index on help_keywords.help_tag for JOIN operations (may already exist)
ALTER TABLE help_keywords ADD INDEX IF NOT EXISTS idx_help_keywords_help_tag (help_tag);

-- Index on help_keywords.keyword for search operations (may already exist)
ALTER TABLE help_keywords ADD INDEX IF NOT EXISTS idx_help_keywords_keyword (keyword);

-- Composite index for common JOIN pattern (help_tag + keyword)
-- This significantly speeds up queries that join on help_tag and filter by keyword
ALTER TABLE help_keywords ADD INDEX IF NOT EXISTS idx_help_keywords_composite (help_tag, keyword);

-- Add FULLTEXT index for future full-text search capabilities
-- This enables MATCH...AGAINST queries for better search functionality
-- Note: This requires MyISAM or InnoDB with MySQL 5.6+
ALTER TABLE help_entries ADD FULLTEXT INDEX IF NOT EXISTS idx_help_entries_fulltext (entry);

-- Display index information after creation
SHOW INDEXES FROM help_entries;
SHOW INDEXES FROM help_keywords;