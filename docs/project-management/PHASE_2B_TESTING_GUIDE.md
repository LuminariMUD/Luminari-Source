# Phase 2B Spatial Audio Testing Guide

**System:** LuminariMUD PubSub Spatial Audio Integration  
**Phase:** 2B Testing Procedures  
**Date:** August 12, 2025  
**Version:** Production Ready  

---

## Quick Start Testing

### 🚀 **Immediate Test Commands**

1. **Start the MUD server:**
   ```bash
   cd /home/jamie/Luminari-Source
   ./bin/circle 4000
   ```

2. **Login as an immortal character** (required for testing commands)

3. **Navigate to wilderness** - The spatial audio system requires wilderness areas

4. **Run basic spatial audio test:**
   ```
   pubsubqueue spatial
   ```

---

## Detailed Testing Procedures

### 🎯 **Test Environment Setup**

#### Prerequisites
- ✅ Immortal-level character (LVL_IMMORT or higher)
- ✅ Wilderness zone access
- ✅ Multiple characters for multi-player testing (optional)

#### Initial System Check
```
1. pubsub                    # Verify PubSub system is active
2. pubsubqueue status        # Check queue system status
3. where                     # Confirm your location
4. coordinate                # Get your wilderness coordinates (if available)
```

### 🌍 **Wilderness Location Testing**

#### Step 1: Verify Wilderness Access
```
# Move to a wilderness area
goto <wilderness_room_vnum>

# Or use wilderness travel
wilderness

# Verify you're in wilderness
pubsubqueue spatial
```
**Expected Result:** If in wilderness: "Wilderness spatial audio test sent."  
**If Error:** "You must be in the wilderness to test spatial audio."

#### Step 2: Check Coordinates
The system uses wilderness coordinates. Verify your position:
```
# If coordinate command exists:
coordinate

# Or check with immortal commands:
stat room
```

### 🔊 **Basic Spatial Audio Tests**

#### Test 1: Single Source Audio
```
pubsubqueue spatial
```
**Expected Output:**
- Source: "Wilderness spatial audio test sent."
- All nearby players: "A mysterious sound echoes powerfully across the wilderness." (if close)
- Or distance-modified version with direction information

#### Test 2: Distance Variation Testing
```
# Test from different distances:
1. Have another character move various distances away
2. Run: pubsubqueue spatial
3. Observer should see different volume levels and directions
```

**Expected Distance Effects:**
- **0-5 units:** "echoes powerfully across the wilderness" (Red text)
- **5-15 units:** "clearly hear nearby" (Yellow text)  
- **15-25 units:** "carried on the wind" (Cyan text)
- **25-35 units:** "faintly in the distance" (Blue text)
- **35-45 units:** "barely make out from far away" (Blue text)
- **45+ units:** "think you hear something" (Black text) or no message

#### Test 3: Directional Audio
```
# Have observer move to different cardinal positions:
# North, South, East, West, Northeast, etc.
pubsubqueue spatial
```
**Expected Directional Output:**
- "The sound comes from the north."
- "The sound comes from the southeast."
- etc.

### 🏔️ **Terrain-Based Audio Testing**

#### Test 4: Terrain Modifier Effects
Move to different terrain types and test:

```
# Forest areas (trees absorb sound)
pubsubqueue spatial
# Expected: Reduced volume, "muffled" descriptions

# Mountain/Hill areas (echo enhancement)  
pubsubqueue spatial
# Expected: Enhanced volume, clearer audio

# Water areas (sound carries)
pubsubqueue spatial  
# Expected: Sound carries further

# Plains (normal propagation)
pubsubqueue spatial
# Expected: Standard distance effects
```

#### Test 5: Elevation Effects
```
# Test uphill/downhill audio:
1. Position characters at different elevations
2. Run spatial tests
3. Observe volume differences

# Uphill: Sound should be slightly reduced
# Downhill: Sound should carry better
# Major elevation barriers: Sound may be blocked/muffled
```

### 🎵 **Multi-Source Audio Testing**

#### Test 6: Multiple Simultaneous Sources
```
# With multiple immortal characters:
1. Character A: pubsubqueue spatial
2. Character B: pubsubqueue spatial  (within 30 seconds)
3. Character C: pubsubqueue spatial  (within 30 seconds)

# Observers should see mixed audio:
"You hear a mixture of sounds: mysterious sound, mysterious sound, and mysterious sound."
```

#### Test 7: Audio Source Expiration
```
1. Run: pubsubqueue spatial
2. Wait 30+ seconds
3. Run: pubsubqueue spatial again
4. Should not mix with expired source
```

### 📊 **System Performance Testing**

#### Test 8: Queue Integration
```
# Check queue statistics:
pubsubqueue status

# Look for:
- Messages processed count
- Active handlers
- Queue processing status
- Any error messages
```

#### Test 9: Memory Management
```
# Create multiple audio sources rapidly:
1. Run "pubsubqueue spatial" 10+ times quickly
2. Check system stability
3. Wait for expiration cleanup
4. Verify no memory leaks (check system resources)
```

#### Test 10: Handler Registration
```
# Verify spatial handlers are registered:
pubsubstat

# Should show handlers including:
- wilderness_spatial_audio
- audio_mixing
```

---

## Advanced Testing Scenarios

### 🎭 **Realistic Usage Scenarios**

#### Scenario 1: Combat Audio
```
# Simulate combat sounds in wilderness:
1. Position multiple characters around wilderness area
2. Have characters at different distances and elevations
3. Run spatial tests to simulate weapon clashes, spells, etc.
4. Verify realistic audio propagation
```

#### Scenario 2: Environmental Audio
```
# Test environmental sounds:
1. Different weather conditions (if available)
2. Day/night cycles (if affecting audio)
3. Various terrain combinations
4. Large groups of players
```

#### Scenario 3: Edge Cases
```
# Test boundary conditions:
1. Maximum distance (50+ units apart)
2. Same coordinates (0 distance)
3. Underground areas (if accessible)
4. Zone boundaries
5. Very high elevation differences
```

### 🔧 **Integration Testing**

#### Test 11: Database Integration
```
# Verify no database conflicts:
1. Run normal PubSub operations
2. Run spatial audio tests
3. Check for database errors in logs
4. Verify message persistence (if enabled)
```

#### Test 12: Command Interface
```
# Test all command variations:
pubsubqueue
pubsubqueue status
pubsubqueue test
pubsubqueue spatial
pubsubqueue process
pubsubqueue start
pubsubqueue stop

# Verify help text:
pubsubqueue invalid_command
```

---

## Troubleshooting Guide

### ❌ **Common Issues**

#### Issue: "You must be in the wilderness to test spatial audio"
**Solution:** 
- Move to a wilderness zone (ZONE_WILDERNESS flag)
- Use `goto` command to wilderness room
- Check with `stat room` to verify zone flags

#### Issue: "Failed to send wilderness spatial audio test"
**Possible Causes:**
- PubSub system not initialized
- Database connection issues
- Memory allocation problems
- Handler not registered

**Debug Steps:**
```
1. pubsub                    # Check system status
2. pubsubqueue status        # Check queue status  
3. Check error logs for details
```

#### Issue: No audio received by other players
**Check:**
- Players within 50-unit range
- Players in same wilderness zone
- Queue processing is running (`pubsubqueue start`)
- No network/connection issues

#### Issue: Audio not showing terrain effects
**Verify:**
- Using actual terrain sectors (not indoor rooms)
- Elevation data available for coordinates
- Terrain detection working (`stat room` shows sector type)

### 🔍 **Debug Information**

#### Log File Locations
```
# Check these logs for issues:
log/errors          # General error messages
log/syslog.*        # System logs
log/security        # Security-related issues
```

#### Debug Commands
```
# Get detailed system information:
pubsubstat           # PubSub system statistics
pubsubqueue status   # Queue system details
stat room            # Room/zone information
coordinate           # Position information (if available)
```

---

## Performance Validation

### 📈 **Performance Benchmarks**

#### Load Testing
```
# Test with multiple players:
1. 10+ characters in wilderness area
2. Run simultaneous spatial audio tests
3. Monitor system performance
4. Check for delays or errors
```

#### Memory Usage
```
# Monitor memory during testing:
1. Create maximum audio sources (5 per player)
2. Let them expire naturally (30 seconds)
3. Verify cleanup occurs
4. Check for memory leaks
```

#### Response Time
```
# Measure response times:
1. Time from "pubsubqueue spatial" to audio delivery
2. Should be < 100ms for good performance
3. Check for delays with large numbers of players
```

---

## Test Result Documentation

### ✅ **Expected Test Results**

#### Successful Test Checklist
- [ ] Basic spatial audio command works
- [ ] Distance-based volume variation
- [ ] Directional information accurate
- [ ] Terrain effects apply correctly
- [ ] Elevation effects function
- [ ] Multi-source audio mixing works
- [ ] 30-second expiration cleanup
- [ ] No memory leaks
- [ ] Queue integration stable
- [ ] No database errors
- [ ] Performance acceptable (< 100ms response)

#### Sample Successful Output
```
> pubsubqueue spatial
Wilderness spatial audio test sent.

[Other players see:]
A mysterious sound echoes powerfully across the wilderness.
[or]
You hear a mysterious sound clearly nearby. The sound comes from the north.
[or]  
You hear a mysterious sound carried on the wind. The sound comes from the southeast.
```

### 📝 **Test Log Template**

```
Test Date: ___________
Tester: ___________
Version: Phase 2B

Basic Functionality:
[ ] Spatial audio command - Pass/Fail
[ ] Distance effects - Pass/Fail  
[ ] Direction detection - Pass/Fail
[ ] Terrain modifiers - Pass/Fail

Advanced Features:
[ ] Multi-source mixing - Pass/Fail
[ ] Elevation effects - Pass/Fail
[ ] Source expiration - Pass/Fail
[ ] Queue integration - Pass/Fail

Performance:
[ ] Response time < 100ms - Pass/Fail
[ ] Memory usage stable - Pass/Fail
[ ] No error messages - Pass/Fail

Notes:
_________________________________
```

---

## Production Deployment Testing

### 🚀 **Pre-Production Checklist**

Before deploying to live server:

- [ ] All unit tests pass
- [ ] Performance benchmarks met
- [ ] No memory leaks detected
- [ ] Database integration stable
- [ ] Multiple player testing completed
- [ ] Error handling validated
- [ ] Backward compatibility confirmed
- [ ] Documentation complete

### 🔄 **Ongoing Monitoring**

After deployment:

```
# Regular checks:
1. Monitor error logs daily
2. Check queue processing status
3. Verify spatial audio reports from players
4. Monitor system performance metrics
5. Watch for memory usage growth
```

---

## Support Information

### 📞 **Getting Help**

For issues during testing:

1. **Check Error Logs:** Review system logs for specific error messages
2. **Verify Prerequisites:** Ensure wilderness access and immortal privileges  
3. **System Status:** Use `pubsubqueue status` to check system health
4. **Documentation:** Reference the Phase 2B completion report for technical details

### 🐛 **Bug Reporting**

If you find issues:

1. **Document the exact steps** to reproduce
2. **Include error messages** from logs
3. **Note system state** (coordinates, zone, terrain type)
4. **Specify expected vs actual behavior**
5. **Include performance information** if relevant

---

*Testing Guide for LuminariMUD Phase 2B Spatial Audio System*  
*Comprehensive testing procedures for production deployment*  
*Updated: August 12, 2025*
