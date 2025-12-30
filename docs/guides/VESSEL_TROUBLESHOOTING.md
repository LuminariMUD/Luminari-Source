# Vessel System Troubleshooting Guide

**Version**: 2.4840
**Last Updated**: 2025-12-30

---

## Common Issues and Solutions

### 1. Vessel Not Moving

**Symptoms**: Ship appears stuck, heading/speed commands have no effect

**Possible Causes**:
- Ship is docked
- Terrain is impassable for vessel type
- Speed is set to 0
- Autopilot is in PAUSED state

**Solutions**:
```
1. Check docking status: showschedule or shipstatus
2. Verify undocked: undock (if docked)
3. Set speed: speed 10
4. Check autopilot: autopilot status
5. Resume if paused: autopilot resume
```

---

### 2. Cannot Board Vessel

**Symptoms**: "You can't board that" or similar error

**Possible Causes**:
- Not at a dockable room
- Vessel is full
- Vessel is damaged
- No boarding point configured

**Solutions**:
```
1. Move to a dockable room (room with DOCKABLE flag)
2. Check vessel capacity: shipstatus
3. Repair damaged vessel if needed
4. Verify entrance_room is set in vessel data
```

---

### 3. Interior Room Navigation Issues

**Symptoms**: Cannot move between ship rooms, "You can't go that way"

**Possible Causes**:
- Connections not generated
- Room exits not configured
- Hatch is locked/sealed

**Solutions**:
```
1. Verify connections exist: ship_rooms
2. Check for locked hatches
3. Regenerate connections if ship was just loaded
```

---

### 4. Autopilot Not Following Route

**Symptoms**: Ship stays in WAITING state, doesn't move to next waypoint

**Possible Causes**:
- Route has no waypoints
- Current waypoint unreachable
- Wait time not expired
- Terrain impassable

**Solutions**:
```
1. Check route: listroutes
2. Verify waypoints: listwaypoints
3. Check current waypoint: autopilot status
4. Verify terrain: can vessel type reach waypoint coordinates?
```

---

### 5. Vehicle Loading Failures

**Symptoms**: "Cannot load vehicle onto vessel"

**Possible Causes**:
- Vessel lacks cargo capacity
- Vehicle too heavy
- Vessel not docked
- Wrong room type

**Solutions**:
```
1. Check vessel cargo capacity
2. Verify vehicle weight vs remaining capacity
3. Ensure vessel is docked or at loading point
4. Load from cargo hold room type
```

---

### 6. Coordinate Desync

**Symptoms**: Ship coordinates don't match expected location

**Possible Causes**:
- Manual teleportation
- Database corruption
- Missing wilderness room allocation

**Solutions**:
```
1. Reload ship: greyhawk_shipload (admin)
2. Force coordinate update: update_ship_wilderness_position()
3. Check wilderness room allocation in db
```

---

### 7. Memory Issues

**Symptoms**: Server slowdown with many vessels, memory warnings

**Possible Causes**:
- Exceeding 500 vessel limit
- Waypoint/route cache bloat
- Memory leaks (should not occur - Valgrind verified)

**Solutions**:
```
1. Monitor vessel count: keep under 500
2. Clear unused routes: delwaypoint, route_destroy
3. Run with Valgrind to verify no leaks
```

---

### 8. NPC Pilot Not Announcing

**Symptoms**: Pilot assigned but no waypoint announcements

**Possible Causes**:
- Pilot NPC not in ship interior
- Pilot VNUM mismatch
- NPC was killed/despawned

**Solutions**:
```
1. Verify pilot presence in ship
2. Check pilot_mob_vnum in autopilot_data
3. Reassign pilot: assignpilot <npc>
```

---

### 9. Schedule Not Triggering

**Symptoms**: Scheduled departure doesn't occur

**Possible Causes**:
- Schedule disabled
- Interval too long
- MUD hour calculation error
- Route not active

**Solutions**:
```
1. Check schedule: showschedule
2. Verify enabled flag: SCHEDULE_FLAG_ENABLED
3. Verify route is active
4. Check next_departure vs current MUD hour
```

---

### 10. Vehicle Terrain Blocked

**Symptoms**: "Cannot traverse that terrain"

**Possible Causes**:
- Vehicle type lacks terrain capability
- Sector type not in VTERRAIN flags

**Solutions**:
```
1. Check vehicle terrain flags: vstatus
2. Verify sector_to_vterrain() mapping
3. Use appropriate vehicle for terrain:
   - CART: roads, plains only
   - MOUNT: roads, plains, forest, hills
   - CARRIAGE: roads only
```

---

## Diagnostic Commands

### Admin Diagnostics

```
shipstatus        - Detailed ship status
autopilot status  - Autopilot state and route info
showschedule      - Schedule timing info
listwaypoints     - All configured waypoints
listroutes        - All configured routes
```

### Debug Functions

```c
/* Log vessel state */
log("VESSEL: ship %d at (%f,%f,%f) heading %d speed %d",
    ship->shipnum, ship->x, ship->y, ship->z,
    ship->heading, ship->speed);

/* Log autopilot state */
log("AUTOPILOT: state=%d waypoint=%d wait=%d",
    ship->autopilot->state,
    ship->autopilot->current_waypoint_index,
    ship->autopilot->wait_remaining);
```

---

## Performance Verification

### Running Tests

```bash
cd unittests/CuTest
make all
./vessel_tests           # 93 core tests
./vessel_stress_test     # 100/250/500 vessel stress
```

### Memory Check

```bash
valgrind --leak-check=full --suppressions=cutest.supp ./vessel_tests
```

Expected output: "All heap blocks were freed -- no leaks are possible"

---

## Related Documentation

- [VESSEL_SYSTEM.md](../VESSEL_SYSTEM.md) - Full system documentation
- [VESSEL_BENCHMARKS.md](../VESSEL_BENCHMARKS.md) - Performance data
- [TROUBLESHOOTING_AND_MAINTENANCE.md](TROUBLESHOOTING_AND_MAINTENANCE.md) - General troubleshooting

---

*Generated as part of Phase 03, Session 06 - Final Testing and Documentation*
