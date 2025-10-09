# Vessels Phase 2 - Production Deployment Guide

## Pre-Deployment Checklist

### Prerequisites
- [x] MySQL/MariaDB 5.7+ or compatible
- [x] Database user with CREATE TABLE, CREATE PROCEDURE privileges
- [x] Backup of production database completed
- [x] Maintenance window scheduled
- [x] Rollback script tested and ready

### Files Required
1. `vessels_phase2_schema.sql` - Main schema installation
2. `vessels_phase2_rollback.sql` - Emergency rollback script
3. `verify_vessels_schema.sql` - Verification script

## Deployment Steps

### 1. Backup Production Database
```bash
# Create full backup before deployment
mysqldump -u root -p luminari_mudprod > backup_$(date +%Y%m%d_%H%M%S).sql
```

### 2. Test Installation (if staging environment available)
```bash
# Test on staging first if possible
mysql -u root -p staging_db < vessels_phase2_schema.sql
mysql -u root -p staging_db < verify_vessels_schema.sql
```

### 3. Deploy to Production
```bash
# Install schema
mysql -u root -p luminari_mudprod < vessels_phase2_schema.sql

# Verify installation
mysql -u root -p luminari_mudprod < verify_vessels_schema.sql
```

### 4. Verify Deployment
Run these checks:
```sql
-- Check all 5 tables exist
SELECT COUNT(*) as table_count FROM information_schema.TABLES 
WHERE TABLE_SCHEMA = 'luminari_mudprod' AND TABLE_NAME LIKE 'ship_%';
-- Expected: 5

-- Check room templates loaded
SELECT COUNT(*) as template_count FROM ship_room_templates;
-- Expected: 19

-- Check stored procedures
SHOW PROCEDURE STATUS WHERE Db = 'luminari_mudprod' 
AND Name IN ('cleanup_orphaned_dockings', 'get_active_dockings');
-- Expected: 2 procedures
```

## What Gets Installed

### Tables Created (5 total)
1. **ship_interiors** - Main vessel room configuration storage
   - Primary key: ship_id
   - Indexes: vessel_type, vessel_name
   
2. **ship_docking** - Active and historical docking records
   - Auto-increment primary key
   - Unique constraint on active dockings
   - Indexes for fast lookups
   
3. **ship_room_templates** - 19 pre-configured room types
   - Bridge, quarters, cargo, engineering, etc.
   
4. **ship_cargo_manifest** - Cargo tracking
   - Foreign key to ship_interiors
   
5. **ship_crew_roster** - NPC crew assignments
   - Foreign key to ship_interiors

### Stored Procedures (2 total)
1. **cleanup_orphaned_dockings()** - Maintenance procedure
2. **get_active_dockings(ship_id)** - Query active dockings

## Rollback Procedure

If issues arise, rollback immediately:
```bash
# Execute rollback script
mysql -u root -p luminari_mudprod < vessels_phase2_rollback.sql

# Verify cleanup
mysql -u root -p luminari_mudprod -e "SHOW TABLES LIKE 'ship_%';"
# Expected: No results
```

## Post-Deployment

### 1. Monitor for Issues
- Check error logs for any MySQL errors
- Monitor MUD server logs for vessel-related errors
- Test basic vessel commands in-game

### 2. Schedule Regular Maintenance
```sql
-- Run weekly to clean up orphaned dockings
CALL cleanup_orphaned_dockings();
```

### 3. Performance Monitoring
```sql
-- Check table sizes periodically
SELECT TABLE_NAME, TABLE_ROWS, DATA_LENGTH/1024/1024 as size_mb
FROM information_schema.TABLES 
WHERE TABLE_SCHEMA = 'luminari_mudprod' AND TABLE_NAME LIKE 'ship_%';
```

## Troubleshooting

### Issue: Foreign key constraint errors
**Solution:** Tables have CASCADE DELETE - ensure ship_interiors records exist before adding related data

### Issue: Stored procedure not working
**Solution:** Check user has EXECUTE privilege:
```sql
GRANT EXECUTE ON luminari_mudprod.* TO 'luminari_mud'@'localhost';
```

### Issue: Performance degradation
**Solution:** Check indexes are being used:
```sql
EXPLAIN SELECT * FROM ship_docking WHERE ship1_id = 123;
```

## Integration Notes

The schema integrates with existing vessel system in:
- `src/vessels.c` - Core vessel management
- `src/vessels_rooms.c` - Room generation
- `src/vessels_docking.c` - Docking mechanics  
- `src/vessels_db.c` - Database persistence layer

The MUD server code will automatically use these tables when:
- Creating new vessels with interiors
- Saving/loading vessel configurations
- Managing ship-to-ship docking
- Tracking cargo and crew

## Contact for Issues

Report any deployment issues immediately to the development team.
Check `/docs/project-management-zusuk/VESSELS_PHASE2.md` for technical details.

---
*Last Updated: January 2025*
*Version: 1.0*