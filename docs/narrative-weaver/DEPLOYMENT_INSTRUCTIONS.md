# Narrative Weaver - Production Deployment Instructions

**Last Updated**: August 23, 2025  
**Status**: Ready for Production Deployment  
**System**: Narrative Weaver Dynamic Description Enhancement System

---

## ⚠️ PRE-DEPLOYMENT CHECKLIST

### Required Files to Transfer to Production
Copy these files to your production server:
1. `sql/narrative_weaver_reference_schema.sql` - Core database schema
2. `sql/mosswood_sample_data.sql` - Sample data for region 1000004
3. `sql/narrative_weaver_installation.sql` - Complete installation orchestrator
4. `sql/fix_narrative_weaver_schema_inconsistencies.sql` - Schema fixes (if needed)

### Prerequisites
- [ ] Schedule maintenance window (10-15 minutes expected)
- [ ] Notify players of brief downtime if needed  
- [ ] Have production database credentials ready
- [ ] Ensure you have root/admin access to MySQL
- [ ] Confirm database name (typically `luminari_mudprod`)
- [ ] Verify narrative weaver source code is deployed to `/src/systems/narrative_weaver/`

---

## 📋 STEP-BY-STEP DEPLOYMENT

### Step 1: Create Production Backup (CRITICAL)
```bash
# SSH into production server
ssh your_production_server

# Create timestamped backup
mysqldump -u root -p luminari_mudprod > /backup/narrative_weaver_backup_$(date +%Y%m%d_%H%M%S).sql

# Verify backup was created and size looks reasonable
ls -lh /backup/narrative_weaver_backup_*.sql
```

### Step 2: Transfer Files to Production
```bash
# From your local machine (in Luminari-Source directory), upload the SQL files
scp sql/narrative_weaver_reference_schema.sql user@production:/tmp/
scp sql/mosswood_sample_data.sql user@production:/tmp/
scp sql/narrative_weaver_installation.sql user@production:/tmp/
scp sql/fix_narrative_weaver_schema_inconsistencies.sql user@production:/tmp/

# Verify files transferred correctly
ssh user@production "ls -la /tmp/narrative_weaver_* /tmp/mosswood_*"
```

### Step 3: Deploy the Database Schema

#### Option A: Complete Installation (Recommended)
```bash
# On production server
cd /tmp

# Run the complete installation script (includes schema + sample data)
mysql -u root -p luminari_mudprod < narrative_weaver_installation.sql

# Check for any errors (should be none)
echo $?  # Should return 0
```

#### Option B: Manual Step-by-Step Installation
```bash
# On production server
cd /tmp

# 1. Install core schema
mysql -u root -p luminari_mudprod < narrative_weaver_reference_schema.sql

# 2. Install sample data
mysql -u root -p luminari_mudprod < mosswood_sample_data.sql

# 3. Apply any schema fixes (if upgrading existing installation)
mysql -u root -p luminari_mudprod < fix_narrative_weaver_schema_inconsistencies.sql
```

### Step 4: Verify Installation
```bash
# Connect to MySQL and verify the installation
mysql -u root -p luminari_mudprod

# Run verification queries:
```

```sql
-- Check enhanced region_data table
SELECT COUNT(*) as enhanced_columns 
FROM information_schema.COLUMNS 
WHERE table_schema = 'luminari_mudprod' 
AND table_name = 'region_data' 
AND column_name IN ('region_description', 'description_style', 'ai_agent_source');
-- Should return: 3

-- Check narrative weaver tables
SELECT COUNT(*) as narrative_tables
FROM information_schema.TABLES 
WHERE table_schema = 'luminari_mudprod' 
AND table_name IN ('region_hints', 'region_profiles', 'hint_usage_log', 'region_description_cache');
-- Should return: 4

-- Check narrative weaver views
SELECT COUNT(*) as narrative_views
FROM information_schema.VIEWS 
WHERE table_schema = 'luminari_mudprod' 
AND table_name IN ('active_region_hints', 'hint_analytics');
-- Should return: 2

-- Verify sample data for Mosswood
SELECT COUNT(*) as mosswood_hints FROM region_hints WHERE region_vnum = 1000004;
-- Should return: ~19 hints

-- Verify region profile
SELECT region_vnum, overall_theme, description_style FROM region_profiles WHERE region_vnum = 1000004;
-- Should return: 1000004, "Ancient mystical forest...", "mysterious"

-- Test views work correctly
SELECT COUNT(*) FROM active_region_hints WHERE region_vnum = 1000004;
-- Should return: ~19 active hints

-- Exit MySQL
EXIT;
```

### Step 5: Compile and Test the Game
```bash
# Navigate to game directory
cd /path/to/luminari-source

# Use autotools
autoreconf -ivf
./configure

# Compile the game with narrative weaver support
make clean
make

# Check compilation was successful
echo $?  # Should return 0

# Check if binary was created
ls -la bin/circle
```

### Step 6: Test In-Game (Optional but Recommended)
```bash
# Start the game in test mode (if you have a test port)
./bin/circle 4000 > /tmp/test_narrative_weaver.log 2>&1 &

# Connect and test
telnet localhost 4000

# In-game commands to test, in the Mosswood:
# look           (check if enhanced descriptions appear)
# repeat several times to see variation
```

---

## 🔧 POST-DEPLOYMENT VERIFICATION

### Expected Results After Successful Installation:

1. **Database Structure:**
   - `region_data` table enhanced with 11 new description columns
   - 4 new narrative weaver tables created
   - 2 new views for narrative queries
   - Complete Mosswood sample data (region 1000004)

2. **Functional Tests:**
   - Players visiting Mosswood (region 1000004) should see enhanced, varied descriptions
   - Descriptions should change slightly on repeated visits
   - No error messages in game logs
   - System should gracefully handle regions without narrative data

3. **Performance Indicators:**
   - Description generation should be fast (<100ms)
   - No memory leaks during extended gameplay
   - Database queries should be efficient

---

## 🚨 ROLLBACK PROCEDURE (If Needed)

### If Something Goes Wrong:
```bash
# 1. Stop the game server
killall circle

# 2. Restore from backup
mysql -u root -p luminari_mudprod < /backup/narrative_weaver_backup_YYYYMMDD_HHMMSS.sql

# 3. Restart with previous version
# (Restore previous binary if needed)
```

### Manual Cleanup (Alternative):
```sql
-- Connect to MySQL
mysql -u root -p luminari_mudprod

-- Remove narrative weaver tables
DROP VIEW IF EXISTS hint_analytics;
DROP VIEW IF EXISTS active_region_hints;
DROP TABLE IF EXISTS region_description_cache;
DROP TABLE IF EXISTS hint_usage_log;
DROP TABLE IF EXISTS region_profiles;
DROP TABLE IF EXISTS region_hints;

-- Remove enhanced columns from region_data
ALTER TABLE region_data 
DROP COLUMN IF EXISTS region_description,
DROP COLUMN IF EXISTS description_version,
DROP COLUMN IF EXISTS ai_agent_source,
DROP COLUMN IF EXISTS last_description_update,
DROP COLUMN IF EXISTS description_style,
DROP COLUMN IF EXISTS description_length,
DROP COLUMN IF EXISTS has_historical_context,
DROP COLUMN IF EXISTS has_resource_info,
DROP COLUMN IF EXISTS has_wildlife_info,
DROP COLUMN IF EXISTS has_geological_info,
DROP COLUMN IF EXISTS has_cultural_info,
DROP COLUMN IF EXISTS description_quality_score,
DROP COLUMN IF EXISTS requires_review,
DROP COLUMN IF EXISTS is_approved;
```

---

## 📊 MONITORING & MAINTENANCE

### Log Files to Monitor:
- `/log/syslog` - General game errors
- `/log/errors` - Specific error messages
- `/tmp/test_narrative_weaver.log` - Test output

### Performance Monitoring:
```sql
-- Check narrative weaver usage stats
SELECT 
    region_vnum,
    hint_category,
    COUNT(*) as usage_count,
    MAX(used_at) as last_used
FROM hint_usage_log 
GROUP BY region_vnum, hint_category 
ORDER BY usage_count DESC;

-- Check cache performance
SELECT 
    COUNT(*) as total_cached_descriptions,
    AVG(TIMESTAMPDIFF(MINUTE, created_at, NOW())) as avg_age_minutes
FROM region_description_cache;
```

### Regular Maintenance:
```sql
-- Clean old cache entries (run weekly)
DELETE FROM region_description_cache 
WHERE created_at < DATE_SUB(NOW(), INTERVAL 7 DAY);

-- Clean old usage logs (run monthly)
DELETE FROM hint_usage_log 
WHERE used_at < DATE_SUB(NOW(), INTERVAL 30 DAY);
```

---

## 🎯 SUCCESS CRITERIA

### Deployment is Successful When:
- [ ] All SQL scripts execute without errors
- [ ] All verification queries return expected results
- [ ] Game compiles successfully with narrative weaver integration
- [ ] Players see enhanced descriptions in region 1000004 (Mosswood)
- [ ] No error messages in game logs related to narrative weaver
- [ ] Database performance remains stable

### Next Steps After Deployment:
1. **Monitor system performance** for 24-48 hours
2. **Gather player feedback** on description quality
3. **Add content for additional regions** using Mosswood as template
4. **Implement admin commands** for system management

---

## 📞 SUPPORT & TROUBLESHOOTING

### Common Issues:

**"Table 'region_data' doesn't exist"**
- Ensure you're running against the correct database
- Check database name in scripts matches your setup

**"Column 'region_description' already exists"**
- You may have already installed some components
- Use individual scripts instead of the complete installer

**"No enhanced descriptions appearing"**
- Verify narrative weaver code is compiled in
- Check that region 1000004 has sample data
- Enable debug logging to trace function calls

**Performance Issues:**
- Check MySQL slow query log
- Verify database indexes were created
- Monitor memory usage during gameplay

### Getting Help:
- Check game logs in `/log/` directory
- Review SQL installation output for errors
- Test with region 1000004 (Mosswood) which has guaranteed sample data
- Verify database connectivity and permissions

---

**⚠️ Important:** This deployment installs a sophisticated narrative enhancement system. The core functionality is complete but requires periodic content updates and monitoring for optimal player experience.
