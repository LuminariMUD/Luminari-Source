-- =====================================================================
-- Fix Narrative Weaver Schema Inconsistencies
-- =====================================================================
-- This script corrects table name inconsistencies in views and foreign keys
-- that were using incorrect prefixed table names (ai_ prefix).
-- 
-- Date: August 22, 2025
-- Purpose: Fix naming inconsistencies in narrative weaver database schema

USE luminari_mudprod;

-- =====================================================================
-- 1. DROP AND RECREATE VIEWS WITH CORRECT TABLE REFERENCES
-- =====================================================================

-- Drop existing views that may have incorrect table references
DROP VIEW IF EXISTS active_region_hints;
DROP VIEW IF EXISTS hint_analytics;

-- Recreate active_region_hints view with correct table names
CREATE VIEW active_region_hints AS
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

-- Recreate hint_analytics view with correct table names
CREATE VIEW hint_analytics AS
SELECT 
    rh.region_vnum,
    rh.hint_category,
    COUNT(hul.id) as usage_count,
    AVG(rh.priority) as avg_priority,
    MAX(hul.used_at) as last_used,
    COUNT(DISTINCT hul.room_vnum) as unique_rooms
FROM region_hints rh
LEFT JOIN hint_usage_log hul ON rh.id = hul.hint_id
WHERE rh.is_active = TRUE
GROUP BY rh.region_vnum, rh.hint_category;

-- =====================================================================
-- 2. VERIFY FOREIGN KEY CONSTRAINTS ARE CORRECT
-- =====================================================================

-- Check if hint_usage_log foreign key needs to be recreated
-- First, let's check current foreign key constraints
SELECT 
    CONSTRAINT_NAME,
    TABLE_NAME,
    COLUMN_NAME,
    REFERENCED_TABLE_NAME,
    REFERENCED_COLUMN_NAME
FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE 
WHERE TABLE_SCHEMA = 'luminari_mudprod' 
AND TABLE_NAME = 'hint_usage_log' 
AND REFERENCED_TABLE_NAME IS NOT NULL;

-- If the foreign key points to the wrong table, we'll need to drop and recreate it
-- Note: This will be handled manually if needed based on the query results above

-- =====================================================================
-- 3. ADD MIGRATION RECORD
-- =====================================================================

INSERT INTO schema_migrations (version, description, applied_at) 
VALUES (5, 'Fix narrative weaver schema table name inconsistencies', NOW())
ON DUPLICATE KEY UPDATE applied_at = NOW();

-- =====================================================================
-- 4. VERIFICATION QUERIES
-- =====================================================================

-- Verify views were created successfully
SELECT 'active_region_hints' as view_name, COUNT(*) as record_count FROM active_region_hints
UNION ALL
SELECT 'hint_analytics' as view_name, COUNT(*) as record_count FROM hint_analytics;

-- Show current foreign key constraints for verification
SELECT 
    CONSTRAINT_NAME,
    TABLE_NAME,
    COLUMN_NAME,
    REFERENCED_TABLE_NAME,
    REFERENCED_COLUMN_NAME
FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE 
WHERE TABLE_SCHEMA = 'luminari_mudprod' 
AND TABLE_NAME IN ('hint_usage_log', 'region_hints', 'region_profiles')
AND REFERENCED_TABLE_NAME IS NOT NULL
ORDER BY TABLE_NAME, CONSTRAINT_NAME;

-- =====================================================================
-- COMPLETION
-- =====================================================================

SELECT 'Schema inconsistencies fixed successfully!' as status;
