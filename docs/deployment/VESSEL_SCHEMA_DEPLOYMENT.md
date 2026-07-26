# Vessel Schema - Production Deployment

Concrete DBA procedure for deploying vessel system database schema to
production: backup, transfer, install, verify, and roll back.

Originally written for the Phase 2 schema; the procedure applies unchanged to
every vessel phase. Substitute the phase you are deploying.

> **Preferred path**: the server auto-creates and auto-migrates all vessel
> tables at boot, so a normal deploy needs no manual SQL at all. Use this
> procedure when you want the schema in place before the server starts, when
> auto-creation is undesirable, or when you need a rehearsed rollback.

## Available schema components

All scripts live in `sql/components/`. Each phase has schema, rollback, and
(from Phase 4 on) verification scripts:

| Phase | Scripts | Adds |
|-------|---------|------|
| 2 | `vessels_phase2_schema.sql`, `vessels_phase2_rollback.sql`, `verify_vessels_schema.sql` | Interiors, docking, room templates, cargo manifest, crew roster |
| 4 | `vessels_phase4_schema.sql`, `vessels_phase4_rollback.sql`, `verify_vessels_phase4.sql` | `ship_prototypes` (vedit) |
| 6 | `vessels_phase6_schema.sql`, `vessels_phase6_rollback.sql`, `verify_vessels_phase6.sql` | Ownership, upgrades, insurance, wage columns |
| 7 | `vessels_phase7_schema.sql`, `vessels_phase7_rollback.sql`, `verify_vessels_phase7.sql` | Commodities, port supply, freight contracts, bounties |
| 8 | `vessels_phase8_schema.sql`, `vessels_phase8_rollback.sql`, `verify_vessels_phase8.sql` | Region-keyed encounter tables |
| help | `help_vessel_entries.sql` | 26 vessel/vehicle help entries into `help_entries`/`help_keywords` (idempotent; no rollback script needed - re-running updates in place) |

Deploy phases in ascending order - later phases extend tables that earlier
phases create.

## ⚠️ PRE-DEPLOYMENT CHECKLIST

### Required Files to Transfer to Production
Copy the scripts for the phase you are deploying, e.g. for Phase 7:
1. `sql/components/vessels_phase7_schema.sql` - Installation script
2. `sql/components/vessels_phase7_rollback.sql` - Emergency rollback
3. `sql/components/verify_vessels_phase7.sql` - Verification script

### Prerequisites
- [ ] Schedule maintenance window (5-10 minutes expected)
- [ ] Notify players of brief downtime if needed
- [ ] Have production database credentials ready
- [ ] Ensure you have root/admin access to MySQL

---

## 📋 STEP-BY-STEP DEPLOYMENT

### Step 1: Create Production Backup (CRITICAL)
```bash
# SSH into production server
ssh your_production_server

# Create timestamped backup
mysqldump -u root -p luminari_mudprod > /backup/luminari_backup_$(date +%Y%m%d_%H%M%S).sql

# Verify backup was created
ls -lh /backup/luminari_backup_*.sql
```

### Step 2: Transfer Files to Production
```bash
# From your local machine, upload the SQL files
scp sql/components/vessels_phase2_schema.sql user@production:/tmp/
scp sql/components/vessels_phase2_rollback.sql user@production:/tmp/
scp sql/components/verify_vessels_schema.sql user@production:/tmp/
```

### Step 3: Deploy the Schema
```bash
# On production server
cd /tmp

# Install the schema
mysql -u root -p luminari_mudprod < vessels_phase2_schema.sql

# Check for any errors (should be none)
echo $?  # Should return 0
```

### Step 4: Verify Installation
```bash
# Run verification script
mysql -u root -p luminari_mudprod < verify_vessels_schema.sql

# Quick manual checks
mysql -u root -p luminari_mudprod -e "
  SELECT COUNT(*) as table_count FROM information_schema.TABLES
  WHERE TABLE_SCHEMA = 'luminari_mudprod' AND TABLE_NAME LIKE 'ship_%';

  SELECT COUNT(*) as template_count FROM ship_room_templates;

  SHOW PROCEDURE STATUS WHERE Db = 'luminari_mudprod'
  AND Name IN ('cleanup_orphaned_dockings', 'get_active_dockings');"
```

**Expected Results:**
- table_count: 5
- template_count: 19
- 2 procedures listed

### Step 5: Test Basic Functionality
```bash
# Test stored procedure (harmless query)
mysql -u root -p luminari_mudprod -e "CALL get_active_dockings(1);"

# Should return empty result set (no error)
```

### Step 6: Restart MUD Server (if needed)
```bash
# Only if the MUD needs to reload database schema
# Check your specific restart procedure
sudo systemctl restart luminari  # or your specific command
```

---

## 🔄 ROLLBACK PROCEDURE (If Issues Occur)

### Emergency Rollback Steps
```bash
# If ANY issues occur, rollback immediately:
mysql -u root -p luminari_mudprod < vessels_phase2_rollback.sql

# Verify rollback
mysql -u root -p luminari_mudprod -e "SHOW TABLES LIKE 'ship_%';"
# Should return: Empty set

# Restore from backup if needed
mysql -u root -p luminari_mudprod < /backup/luminari_backup_YYYYMMDD_HHMMSS.sql
```

---

## ✅ POST-DEPLOYMENT VERIFICATION

### 1. Check MUD Server Logs
```bash
# Monitor for any database errors
tail -f /path/to/mud/logs/error.log

# Look for vessel-related issues
grep -i "vessel\|ship" /path/to/mud/logs/error.log
```

### 2. In-Game Testing
Log into the MUD and test:
- Basic vessel commands still work
- No error messages when interacting with ships
- Existing vessels are not affected

### 3. Database Health Check
```bash
mysql -u root -p luminari_mudprod -e "
  SELECT TABLE_NAME, TABLE_ROWS, DATA_LENGTH/1024 as size_kb
  FROM information_schema.TABLES
  WHERE TABLE_SCHEMA = 'luminari_mudprod'
  AND TABLE_NAME LIKE 'ship_%';"
```

---

## 📊 MONITORING COMMANDS

### Check Active Dockings
```sql
mysql -u root -p luminari_mudprod -e "
  SELECT * FROM ship_docking WHERE dock_status = 'active';"
```

### Check Ship Interiors (once data exists)
```sql
mysql -u root -p luminari_mudprod -e "
  SELECT ship_id, vessel_name, num_rooms, created_at
  FROM ship_interiors LIMIT 10;"
```

### Schedule Weekly Cleanup (optional)
```bash
# Add to crontab for weekly cleanup
crontab -e

# Add this line (runs Sundays at 3 AM):
0 3 * * 0 mysql -u root -pYOURPASSWORD luminari_mudprod -e "CALL cleanup_orphaned_dockings();"
```

---

## 🚨 TROUBLESHOOTING

### Issue: "Table already exists"
**Solution:** Tables already installed. Check with verification script.

### Issue: "Access denied for procedure"
**Solution:** Grant execute permissions:
```sql
GRANT EXECUTE ON luminari_mudprod.* TO 'luminari_mud'@'localhost';
FLUSH PRIVILEGES;
```

### Issue: "Foreign key constraint fails"
**Solution:** This shouldn't happen on fresh install. If it does, use rollback.

### Issue: MUD server can't access new tables
**Solution:**
1. Check MUD user permissions
2. Restart MUD server
3. Verify connection string in MUD config

---

## 📞 SUPPORT CHECKLIST

If you need to report issues, gather:
1. Error messages from MySQL
2. MUD server error logs
3. Output of verification script
4. Time of deployment attempt

---

## 🎯 QUICK DEPLOYMENT (Copy-Paste Commands)

```bash
# For experienced admins - all commands in sequence:

# 1. Backup
mysqldump -u root -p luminari_mudprod > /backup/luminari_$(date +%Y%m%d_%H%M%S).sql

# 2. Deploy
mysql -u root -p luminari_mudprod < vessels_phase2_schema.sql

# 3. Verify
mysql -u root -p luminari_mudprod -e "
  SELECT 'Tables:' as Check_Type, COUNT(*) as Result
  FROM information_schema.TABLES
  WHERE TABLE_SCHEMA = 'luminari_mudprod' AND TABLE_NAME LIKE 'ship_%'
  UNION ALL
  SELECT 'Templates:' as Check_Type, COUNT(*) as Result
  FROM ship_room_templates
  UNION ALL
  SELECT 'Procedures:' as Check_Type, COUNT(*) as Result
  FROM information_schema.ROUTINES
  WHERE ROUTINE_SCHEMA = 'luminari_mudprod'
  AND ROUTINE_NAME IN ('cleanup_orphaned_dockings', 'get_active_dockings');"

# Expected output:
# Check_Type | Result
# Tables     | 5
# Templates  | 19  
# Procedures | 2
```

---

**Deployment Time Estimate:** 5-10 minutes
**Risk Level:** Low (rollback available)
**Impact:** No existing data affected

*Last Updated: January 2025*
