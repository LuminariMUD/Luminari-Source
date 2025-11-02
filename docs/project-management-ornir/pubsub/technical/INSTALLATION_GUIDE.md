# PubSub System Installation Guide

**System:** LuminariMUD Publish/Subscribe Messaging System  
**Version:** Phase 2B Complete - Spatial Audio Integration  
**Target:** Production and Development Environments  
**Last Updated:** August 13, 2025

---

## 🎯 **Installation Overview**

The LuminariMUD PubSub system is fully integrated into the main codebase and requires no additional dependencies beyond the standard LuminariMUD requirements. This guide covers installation, configuration, and initial testing.

---

## ✅ **Prerequisites**

### System Requirements
- **LuminariMUD:** Latest development branch
- **MySQL/MariaDB:** 5.7+ (existing LuminariMUD database)
- **Compiler:** GCC with C90 support
- **Build Tools:** Autotools (autoconf, automake)

### Required LuminariMUD Features
- ✅ **Wilderness System** - Required for spatial audio
- ✅ **MySQL Integration** - For persistent storage
- ✅ **Event System** - For heartbeat integration

---

## 🚀 **Installation Steps**

### Step 1: Source Code Integration

The PubSub system is already integrated into the LuminariMUD source tree:

```bash
# Verify PubSub source files exist
ls -la src/pubsub*
# Should show:
# pubsub.c / pubsub.h           - Core system
# pubsub_commands.c / .h        - Administrative commands  
# pubsub_db.c / .h              - Database operations
# pubsub_handlers.c / .h        - Message handlers
# pubsub_queue.c / .h           - Queue management
# pubsub_spatial.c / .h         - Spatial audio (Phase 2B)
```

### Step 2: Database Schema Setup

The PubSub system automatically creates required database tables on first startup:

```sql
-- Tables created automatically:
-- pubsub_topics          - Topic definitions
-- pubsub_subscriptions   - Player subscriptions  
-- pubsub_messages        - Message persistence (optional)
```

**No manual database setup required** - tables are created automatically when the system initializes.

### Step 3: Build Configuration

The build system is already configured. Verify PubSub files are included:

```bash
# Check Makefile.am includes PubSub sources
grep -n "pubsub" Makefile.am

# Should show PubSub source files in circle_SOURCES
```

### Step 4: Compilation

```bash
# Standard LuminariMUD build process
./autorun.sh          # Generate build configuration (if needed)
make clean             # Clean previous builds
make -j20              # Compile with parallel jobs
make install           # Install to bin/ directory
```

Expected output:
```
✅ All PubSub modules compile without errors
✅ No warnings from PubSub source files  
✅ circle binary includes PubSub functionality
```

---

## ⚙️ **Configuration**

### Default Configuration

The PubSub system uses sensible defaults and requires minimal configuration:

```c
// Default settings (in pubsub.h)
#define PUBSUB_VERSION                  1
#define PUBSUB_QUEUE_MAX_SIZE           1000
#define PUBSUB_QUEUE_BATCH_SIZE         20
#define SPATIAL_AUDIO_MAX_DISTANCE      50
#define MAX_CONCURRENT_AUDIO_SOURCES    5
```

### Runtime Configuration

System behavior can be controlled via administrative commands:

```bash
# In-game commands (require immortal access)
pubsub status          # Check system status
pubsub enable          # Enable PubSub system
pubsub disable         # Disable PubSub system

pubsubqueue status     # Check queue status
pubsubqueue start      # Start queue processing
pubsubqueue stop       # Stop queue processing
```

### Database Configuration

Uses existing LuminariMUD MySQL configuration:
- **Connection:** Uses global `conn` variable
- **Database:** Same database as main MUD
- **Tables:** Auto-created with `pubsub_` prefix
- **Cleanup:** Automatic table maintenance

---

## 🧪 **Initial Testing**

### Step 1: System Initialization Test

```bash
# Start the MUD server
./bin/circle 4000

# Check logs for PubSub initialization
tail -f log/syslog
# Look for: "PUBSUB INFO: Initializing PubSub system v1..."
```

Expected log entries:
```
PUBSUB INFO: Initializing PubSub system v1...
PUBSUB INFO: Database initialization complete
PUBSUB INFO: Message queue system initialized
PUBSUB INFO: Handler registration complete
PUBSUB INFO: PubSub system ready
```

### Step 2: Basic Functionality Test

```bash
# Login as an immortal character
# Test basic system status
> pubsub status

# Expected output:
# PubSub System Status: ENABLED
# Version: 1
# Topics: X active
# Handlers: Y registered
```

### Step 3: Queue System Test

```bash
# Test queue system
> pubsubqueue status

# Expected output:
# Message Queue Status:
# Queue Processing: ENABLED
# Total Queued: 0 messages
# Critical: 0, Urgent: 0, High: 0, Normal: 0, Low: 0
```

### Step 4: Spatial Audio Test (Phase 2B)

```bash
# Move to a wilderness area
> goto <wilderness_room_vnum>

# Test spatial audio
> pubsubqueue spatial

# Expected output:
# Wilderness spatial audio test sent.

# Other players in wilderness should see:
# "You hear a mysterious sound carried on the wind."
```

---

## 🔧 **Advanced Configuration**

### Customizing Spatial Audio Range

Edit `src/pubsub_spatial.c` to modify default ranges:

```c
// Modify these constants for different audio ranges
#define SPATIAL_AUDIO_MAX_DISTANCE      50    // Change max hearing distance
#define SPATIAL_AUDIO_CLOSE_THRESHOLD   5     // Modify close range
#define SPATIAL_AUDIO_MID_THRESHOLD     15    // Modify medium range
```

### Adjusting Queue Processing

Edit `src/pubsub.c` to modify automatic processing:

```c
// In pubsub_process_message_queue()
if (pulse_count < 3) {  // Change frequency (3 = every 0.75 seconds)
    return;
}
```

### Database Optimization

For high-traffic servers, consider:

```sql
-- Add database indexes for performance
CREATE INDEX idx_pubsub_topics_active ON pubsub_topics (is_active);
CREATE INDEX idx_pubsub_subscriptions_player ON pubsub_subscriptions (player_name);
```

---

## 🚨 **Troubleshooting**

### Common Issues

#### Issue: "PubSub system failed to initialize"
**Symptoms:** Error in startup logs  
**Causes:**
- Database connection failure
- Missing MySQL tables permissions
- Memory allocation problems

**Solutions:**
```bash
# Check database connection
mysql -u<username> -p<password> <database>

# Verify permissions
SHOW GRANTS FOR '<muduser>'@'<host>';

# Check available memory
free -h
```

#### Issue: "Huh?!?" when using pubsub commands
**Symptoms:** Commands not recognized  
**Cause:** Command not registered in interpreter

**Solution:**
```bash
# Verify command registration in src/interpreter.c
grep -n "pubsub" src/interpreter.c

# Should show:
# {"pubsub", "pubsub", POS_DEAD, do_pubsub, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
# {"pubsubqueue", "pubsubq", POS_DEAD, do_pubsubqueue, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
```

#### Issue: Spatial audio not working
**Symptoms:** No audio messages received  
**Causes:**
- Not in wilderness zone
- Queue processing disabled
- Too far from audio source

**Debug Steps:**
```bash
# Check if in wilderness
> stat room
# Look for: Zone flags: WILDERNESS

# Check queue processing
> pubsubqueue status
# Verify: Queue Processing: ENABLED

# Check distance
> coordinate  # Get your position
# Compare with audio source coordinates
```

#### Issue: Memory leaks or crashes
**Symptoms:** Server instability  
**Cause:** Reference counting bugs

**Debug Steps:**
```bash
# Use valgrind for memory debugging
valgrind --leak-check=full ./bin/circle 4000

# Check for proper cleanup
# Monitor memory usage during high activity
```

### Log Analysis

Key log entries to monitor:

```bash
# Success indicators
grep "PUBSUB INFO" log/syslog  # System status
grep "Auto-processed" log/syslog  # Queue processing

# Error indicators  
grep "PUBSUB ERROR" log/syslog  # System errors
grep "Failed to" log/syslog  # Specific failures

# Performance monitoring
grep "Processed.*messages" log/syslog  # Processing stats
```

---

## 📊 **Performance Monitoring**

### System Statistics

```bash
# In-game monitoring commands
> pubsub stats          # Detailed system statistics
> pubsubqueue status    # Queue performance metrics

# Log-based monitoring
tail -f log/syslog | grep "PUBSUB"
```

### Key Metrics to Monitor

- **Queue Size:** Should remain low (< 100 messages)
- **Processing Time:** Should be < 1ms per message
- **Memory Usage:** Should be stable (no steady growth)
- **Error Rate:** Should be minimal (< 1% failures)

### Performance Alerts

Set up monitoring for:
- Queue size > 500 messages (potential backup)
- Processing time > 10ms (performance issue)
- Memory usage growth > 1MB/hour (memory leak)
- Error rate > 5% (system problems)

---

## 🔄 **Maintenance**

### Regular Maintenance Tasks

#### Daily
- Monitor error logs for PubSub errors
- Check queue processing statistics
- Verify system responsiveness

#### Weekly  
- Analyze performance metrics
- Review memory usage patterns
- Check database table sizes

#### Monthly
- Clean up old message records (if using persistence)
- Analyze usage patterns for optimization
- Review system capacity requirements

### Database Maintenance

```sql
-- Clean up old messages (if persistence enabled)
DELETE FROM pubsub_messages 
WHERE expires_at < NOW() - INTERVAL 7 DAY;

-- Optimize tables
OPTIMIZE TABLE pubsub_topics, pubsub_subscriptions, pubsub_messages;

-- Check table sizes
SELECT 
    table_name, 
    ROUND(((data_length + index_length) / 1024 / 1024), 2) AS "Size (MB)"
FROM information_schema.tables 
WHERE table_name LIKE 'pubsub_%';
```

---

## 🚀 **Production Deployment**

### Pre-Deployment Checklist

- [ ] All tests pass (see testing guide)
- [ ] No compilation errors or warnings
- [ ] Database connectivity verified
- [ ] Spatial audio tested in wilderness
- [ ] Queue processing working automatically
- [ ] Memory usage stable
- [ ] Performance metrics acceptable

### Deployment Process

```bash
# 1. Build and test in staging environment
make clean && make -j20 && make install
# Run full test suite

# 2. Backup current production
cp bin/circle bin/circle.backup

# 3. Deploy new binary
make install

# 4. Start with monitoring
./bin/circle 4000 &
tail -f log/syslog | grep "PUBSUB"

# 5. Verify functionality
# Test pubsub commands and spatial audio
```

### Rollback Plan

```bash
# If issues occur during deployment:
killall circle
cp bin/circle.backup bin/circle
./bin/circle 4000 &
```

---

## 📞 **Support**

### Getting Help

1. **Check Logs:** Review `log/syslog` for error messages
2. **Test Commands:** Use `pubsub status` and `pubsubqueue status`
3. **Documentation:** Consult technical documentation
4. **Debug Mode:** Enable debug logging for detailed information

### Reporting Issues

Include in bug reports:
1. **Error Messages:** Exact error text from logs
2. **System State:** Output of status commands
3. **Reproduction Steps:** How to recreate the issue
4. **Environment:** Server specifications and load
5. **Timing:** When the issue occurred

---

*Installation and configuration guide for LuminariMUD PubSub System*  
*For technical details, see: [TECHNICAL_OVERVIEW.md](TECHNICAL_OVERVIEW.md)*  
*For testing procedures, see: [../testing/PHASE_2B_TESTING_GUIDE.md](../testing/PHASE_2B_TESTING_GUIDE.md)*
