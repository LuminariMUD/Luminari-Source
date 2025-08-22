-- Narrative Weaver Complete Installation Script
-- Fresh installation for new databases
-- This script orchestrates the complete setup of the narrative weaver system

-- ============================================================================
-- INSTALLATION INSTRUCTIONS
-- ============================================================================
-- Run the following scripts in order:
-- 1. mysql -u root luminari_mudprod < narrative_weaver_reference_schema.sql
-- 2. mysql -u root luminari_mudprod < mosswood_sample_data.sql
-- 3. mysql -u root luminari_mudprod < narrative_weaver_installation.sql
--
-- Or run this script which includes everything:

-- ============================================================================
-- PART 1: Include reference schema
-- ============================================================================
SOURCE narrative_weaver_reference_schema.sql;

-- ============================================================================
-- PART 2: Include sample data  
-- ============================================================================
SOURCE mosswood_sample_data.sql;

-- ============================================================================
-- PART 3: Final verification and summary
-- ============================================================================

SELECT '============================================================================' as separator;
SELECT 'NARRATIVE WEAVER INSTALLATION COMPLETE' as title;
SELECT '============================================================================' as separator;

-- Table structure verification
SELECT 'SCHEMA VERIFICATION:' as section;
SELECT COUNT(*) as region_data_enhanced_columns 
FROM information_schema.COLUMNS 
WHERE table_schema = DATABASE() 
AND table_name = 'region_data' 
AND column_name IN ('region_description', 'description_style', 'ai_agent_source');

SELECT COUNT(*) as narrative_tables_created
FROM information_schema.TABLES 
WHERE table_schema = DATABASE() 
AND table_name IN ('region_hints', 'region_profiles', 'hint_usage_log', 'region_description_cache');

SELECT COUNT(*) as narrative_views_created
FROM information_schema.VIEWS 
WHERE table_schema = DATABASE() 
AND table_name IN ('active_region_hints', 'hint_analytics');

-- Sample data verification
SELECT 'SAMPLE DATA VERIFICATION:' as section;
SELECT COUNT(*) as mosswood_hints FROM region_hints WHERE region_vnum = 1000004;
SELECT COUNT(*) as mosswood_profiles FROM region_profiles WHERE region_vnum = 1000004;
SELECT COUNT(*) as mosswood_region_data FROM region_data WHERE vnum = 1000004 AND region_description IS NOT NULL;

-- System status summary
SELECT 'SYSTEM STATUS:' as section;
SELECT 
    COUNT(DISTINCT region_vnum) as total_regions_with_hints,
    COUNT(*) as total_active_hints
FROM active_region_hints;

SELECT 
    COUNT(*) as total_region_profiles,
    COUNT(DISTINCT description_style) as unique_description_styles
FROM region_profiles;

SELECT 'Installation completed successfully! The narrative weaver system is ready for use.' as final_status;
