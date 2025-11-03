/* *************************************************************************
 *   File: resource_regeneration_api.md                Part of LuminariMUD *
 *  Usage: Developer API reference for resource regeneration system       *
 * Author: LuminariMUD Development Team                                    *
 ***************************************************************************
 * API documentation for integrating with the resource regeneration       *
 * system from other game components.                                      *
 ***************************************************************************/

# Resource Regeneration API Reference

## Overview

This document provides API-level documentation for developers working with the resource regeneration system. It covers function signatures, return values, error conditions, and integration patterns.

## Core API Functions

### Regeneration Rate Functions

#### `float get_resource_regeneration_rate(int resource_type)`

Returns the base regeneration rate per hour for a resource type.

**Parameters:**
- `resource_type` - Integer constant (RESOURCE_VEGETATION, RESOURCE_HERBS, etc.)

**Returns:**
- `float` - Base regeneration rate (0.0-1.0, representing percentage per hour)

**Example:**
```c
float herb_rate = get_resource_regeneration_rate(RESOURCE_HERBS);
// Returns: 0.08 (8% per hour)
```

---

#### `float get_modified_regeneration_rate(int resource_type, int x, int y)`

Returns regeneration rate with seasonal and weather modifiers applied.

**Parameters:**
- `resource_type` - Resource type constant
- `x`, `y` - Wilderness coordinates for weather calculation

**Returns:**
- `float` - Modified regeneration rate with all modifiers applied

**Example:**
```c
float modified_rate = get_modified_regeneration_rate(RESOURCE_HERBS, 100, 200);
// Returns: 0.144 (18% per hour in spring with rain)
```

---

### Time-Based Calculation

#### `float calculate_regeneration_amount(int resource_type, time_t last_harvest_time, int x, int y)`

Calculates total regeneration based on elapsed time since last harvest.

**Parameters:**
- `resource_type` - Resource type constant
- `last_harvest_time` - Unix timestamp of last harvest/interaction
- `x`, `y` - Coordinates for weather/seasonal modifiers

**Returns:**
- `float` - Total regeneration amount (0.0-1.0), capped at 1.0

**Example:**
```c
time_t last_time = time(NULL) - (3600 * 6); // 6 hours ago
float regen = calculate_regeneration_amount(RESOURCE_HERBS, last_time, 100, 200);
// Returns: ~1.0 (6 hours * 18% = 108%, capped at 100%)
```

---

### Lazy Regeneration Trigger

#### `void apply_lazy_regeneration(room_rnum room, int resource_type)`

Applies regeneration for a specific resource at a room location.

**Parameters:**
- `room` - Room number (must be wilderness room with coordinates)
- `resource_type` - Resource type to regenerate

**Returns:**
- `void` - Updates database directly

**Side Effects:**
- Queries database for current depletion data
- Calculates and applies regeneration
- Updates database with new depletion level and timestamp

**Example:**
```c
// Regenerate herbs when player enters room
apply_lazy_regeneration(ch->in_room, RESOURCE_HERBS);
```

---

## Modifier Functions (from resource_system.c)

### Seasonal Modifiers

#### `float get_seasonal_modifier(int resource_type)`

Returns seasonal multiplier based on current game month.

**Parameters:**
- `resource_type` - Resource type constant

**Returns:**
- `float` - Seasonal modifier (0.3-1.8 range)

**Global Dependencies:**
- Uses `time_info.month` global variable

**Example:**
```c
float seasonal = get_seasonal_modifier(RESOURCE_VEGETATION);
// Spring: returns 1.8, Winter: returns 0.3
```

---

### Weather Modifiers  

#### `float get_weather_modifier(int resource_type, int weather_value)`

Returns weather multiplier based on current weather conditions.

**Parameters:**
- `resource_type` - Resource type constant  
- `weather_value` - Weather value from `get_weather(x, y)`

**Returns:**
- `float` - Weather modifier (0.5-2.0 range)

**Example:**
```c
int weather = get_weather(x, y);
float weather_mod = get_weather_modifier(RESOURCE_WATER, weather);
// Storm: returns 2.0, Clear: returns 0.8
```

---

## Integration Patterns

### Movement Integration

The system is integrated with character movement in `handler.c`:

```c
void char_to_room(struct char_data *ch, room_rnum room) {
    // ... existing movement code ...
    
    /* Apply resource regeneration for wilderness rooms */
    if (ROOM_FLAGGED(room, ROOM_WILDERNESS)) {
        int resource_type;
        for (resource_type = 0; resource_type < NUM_RESOURCE_TYPES; resource_type++) {
            apply_lazy_regeneration(room, resource_type);
        }
    }
    
    // ... rest of movement code ...
}
```

### Harvesting Integration

When players harvest resources, the system triggers regeneration:

```c
// In harvest command handler
void do_harvest(struct char_data *ch, char *argument, int cmd, int subcmd) {
    // ... harvest logic ...
    
    /* Trigger regeneration check before harvesting */
    apply_lazy_regeneration(ch->in_room, resource_type);
    
    /* Get current availability after regeneration */
    float availability = get_resource_depletion_level(ch->in_room, resource_type);
    
    // ... continue with harvest ...
}
```

### Survey Command Integration

The survey command shows regeneration information:

```c
// Show regeneration rates in survey detailed
void show_resource_survey_detailed(struct char_data *ch, room_rnum room) {
    int x = world[room].coords[0];
    int y = world[room].coords[1];
    
    for (int i = 0; i < NUM_RESOURCE_TYPES; i++) {
        float base_rate = get_resource_regeneration_rate(i);
        float modified_rate = get_modified_regeneration_rate(i, x, y);
        float availability = get_resource_depletion_level(room, i);
        
        send_to_char(ch, "%s: %.1f%% available, %.1f%%/hour regen\r\n",
                     resource_names[i], availability * 100, modified_rate * 100);
    }
}
```

## Error Handling

### Database Errors

All database operations use safe wrappers:

```c
if (mysql_query_safe(conn, query)) {
    log("SYSERR: Error in regeneration query: %s", mysql_error(conn));
    return; // Fail gracefully
}
```

### Invalid Parameters

Functions validate inputs:

```c
float get_resource_regeneration_rate(int resource_type) {
    if (resource_type < 0 || resource_type >= NUM_RESOURCE_TYPES) {
        log("SYSERR: Invalid resource type: %d", resource_type);
        return 0.05; // Default rate
    }
    // ... normal processing ...
}
```

### Missing Database Connection

System falls back gracefully when MySQL unavailable:

```c
void apply_lazy_regeneration(room_rnum room, int resource_type) {
    /* If MySQL not available, skip */
    if (!mysql_available || !conn) {
        return; // Silent failure, system continues
    }
    // ... normal processing ...
}
```

## Performance Considerations

### Database Optimization

- Uses coordinate-based indexing
- Batch updates multiple resources per location
- Prepared statements for repeated queries

### Memory Management

- Minimal memory allocation during regeneration
- Reuses static buffers for query strings
- No persistent state between calls

### Computational Efficiency

- Lazy evaluation reduces CPU overhead
- Weather calculation cached per location
- Simple mathematical operations (no complex algorithms)

## Testing Hooks

### Debug Functions

```c
// Add to resource_depletion.c for testing
void debug_regeneration_calculation(int resource_type, int x, int y) {
    float base = get_resource_regeneration_rate(resource_type);
    float seasonal = get_seasonal_modifier(resource_type);
    float weather = get_weather_modifier(resource_type, get_weather(x, y));
    float final = base * seasonal * weather;
    
    log("DEBUG REGEN: Type=%d, Base=%.3f, Season=%.3f, Weather=%.3f, Final=%.3f",
        resource_type, base, seasonal, weather, final);
}
```

### Mock Weather Testing

```c
// Override weather for testing
float test_get_weather_modifier(int resource_type, int forced_weather) {
    return get_weather_modifier(resource_type, forced_weather);
}
```

## Constants and Definitions

### Resource Type Constants

```c
#define RESOURCE_VEGETATION   0
#define RESOURCE_MINERALS     1  
#define RESOURCE_WATER        2
#define RESOURCE_HERBS        3
#define RESOURCE_GAME         4
#define RESOURCE_WOOD         5
#define RESOURCE_STONE        6
#define RESOURCE_CRYSTAL      7
#define RESOURCE_CLAY         8
#define RESOURCE_SALT         9
#define NUM_RESOURCE_TYPES   10
```

### Weather Value Ranges

```c
#define WEATHER_CLEAR_MAX     177
#define WEATHER_LIGHT_RAIN    178
#define WEATHER_HEAVY_RAIN    200
#define WEATHER_STORM         225
```

### Season Definitions

```c
// Months 0-2, 12-16: Winter
// Months 3-5: Spring  
// Months 6-8: Summer
// Months 9-11: Autumn
```

## Thread Safety

The system is designed for single-threaded MUD architecture:
- No locks required within MUD process
- MySQL handles concurrent access to database
- Safe wrappers handle connection state

## Future API Extensions

### Planned Enhancements

```c
// Biome-specific modifiers (planned)
float get_biome_modifier(int resource_type, int biome_type);

// Conservation tracking (planned)  
float get_conservation_modifier(struct char_data *ch, int resource_type);

// Magical influence (planned)
float get_magical_modifier(room_rnum room, int resource_type);
```

---

*Last Updated: August 11, 2025*  
*API Version: 1.0*  
*Compatible with: LuminariMUD Phase 6*
