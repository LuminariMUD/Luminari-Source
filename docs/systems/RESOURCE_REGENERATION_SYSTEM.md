# Resource Regeneration System Documentation

## Overview

The Resource Regeneration System is a comprehensive environmental simulation that manages how natural resources replenish themselves over time in the wilderness. This system uses lazy evaluation, seasonal cycles, and dynamic weather patterns to create realistic resource availability fluctuations.

## Table of Contents

1. [System Architecture](#system-architecture)
2. [Core Components](#core-components)
3. [Regeneration Mechanics](#regeneration-mechanics)
4. [Seasonal Modifiers](#seasonal-modifiers)
5. [Weather Effects](#weather-effects)
6. [Database Schema](#database-schema)
7. [Implementation Details](#implementation-details)
8. [Configuration](#configuration)
9. [Testing and Debugging](#testing-and-debugging)
10. [Performance Considerations](#performance-considerations)

## System Architecture

### Lazy Evaluation Model

The system uses **lazy evaluation** instead of periodic updates:
- Resources regenerate only when players enter wilderness rooms
- Calculation based on time elapsed since last visit/harvest
- Reduces server load compared to continuous background processing
- Provides immediate feedback when players explore

### Integration Points

- **Movement System**: Triggered via `char_to_room()` in `handler.c`
- **Resource System**: Uses existing resource type definitions and seasonal modifiers
- **Weather System**: Integrates with wilderness weather generation
- **Database**: Coordinate-based persistence using MySQL

## Core Components

### Files Involved

| File | Purpose |
|------|---------|
| `src/resource_depletion.c` | Core regeneration logic and database operations |
| `src/resource_depletion.h` | Function prototypes and constants |
| `src/resource_system.c` | Seasonal and weather modifier functions |
| `src/handler.c` | Integration with character movement |
| `src/wilderness.c` | Weather data generation |

### Key Functions

#### Primary Regeneration Functions

```c
// Get base regeneration rate for resource type
float get_resource_regeneration_rate(int resource_type);

// Apply seasonal and weather modifiers
float get_modified_regeneration_rate(int resource_type, int x, int y);

// Calculate regeneration based on elapsed time
float calculate_regeneration_amount(int resource_type, time_t last_harvest_time, int x, int y);

// Apply regeneration when player enters room
void apply_lazy_regeneration(room_rnum room, int resource_type);
```

#### Modifier Functions (from resource_system.c)

```c
// Get seasonal multiplier based on game month
float get_seasonal_modifier(int resource_type);

// Get weather multiplier based on current conditions
float get_weather_modifier(int resource_type, int weather_value);
```

## Regeneration Mechanics

### Base Regeneration Rates

Each resource type has a different base regeneration rate per hour:

| Resource Type | Base Rate/Hour | Description |
|---------------|----------------|-------------|
| Vegetation | 12% | Fast-growing plants and foliage |
| Herbs | 8% | Medicinal and magical plants |
| Water | 20% | Surface water sources |
| Game | 6% | Wild animals for hunting |
| Wood | 2% | Trees and woody materials |
| Clay | 10% | Moldable earth materials |
| Stone | 0.5% | Building stone and rocks |
| Minerals | 0.1% | Metal ores and gems |
| Crystal | 0.05% | Magical crystals |
| Salt | 1% | Salt deposits and brine |

### Calculation Formula

```
Final Regeneration = Base Rate × Seasonal Modifier × Weather Modifier × Hours Elapsed
```

### Regeneration Caps

- Maximum regeneration per calculation: 100% (fully restored)
- Resources cannot exceed their natural maximum availability
- Negative regeneration is not possible (resources don't decay from time alone)

## Seasonal Modifiers

The system uses the game's built-in time system (`time_info.month`) with 17 months per year:

### Season Definitions

- **Winter**: Months 0-2, 12-16
- **Spring**: Months 3-5  
- **Summer**: Months 6-8
- **Autumn**: Months 9-11

### Resource-Specific Seasonal Effects

#### Vegetation & Herbs
- **Winter**: 30% rate (dormancy, freezing)
- **Spring**: 180% rate (explosive growth)
- **Summer**: 120% rate (optimal growing conditions)
- **Autumn**: 70% rate (preparing for dormancy)

#### Game Animals
- **Winter**: 50% rate (hibernation, migration)
- **Spring**: 130% rate (breeding season, young animals)
- **Summer**: 100% rate (normal activity)
- **Autumn**: 110% rate (fattening for winter)

#### Wood (Trees)
- **Winter**: 80% rate (slow growth)
- **Spring**: 120% rate (active growing season)
- **Summer/Autumn**: 100% rate (normal growth)

#### Water Sources
- Seasonal effects are overridden by weather patterns
- Spring tends to have higher precipitation
- Summer may have increased evaporation

## Weather Effects

### Weather Value Ranges

The wilderness weather system generates values 0-255:
- **0-177**: Clear weather
- **178-199**: Light rain/drizzle  
- **200-224**: Heavy rain
- **225+**: Thunderstorms

### Weather-Specific Modifiers

#### Clear Weather (0-177)
- **Water**: 80% rate (increased evaporation)
- **Game**: 120% rate (animals more active)
- **Other resources**: 100% rate (normal)

#### Light Rain (178-199)
- **Water/Clay**: 120% rate (gentle precipitation)
- **Vegetation/Herbs**: 110% rate (beneficial moisture)
- **Other resources**: 100% rate

#### Heavy Rain (200-224)
- **Water/Clay**: 150% rate (significant precipitation)
- **Vegetation/Herbs**: 130% rate (abundant water)
- **Game**: 80% rate (animals seek shelter)

#### Thunderstorms (225+)
- **Water/Clay**: 200% rate (intense precipitation)
- **Vegetation/Herbs**: 70% rate (storm damage)
- **Game**: 50% rate (animals hide from storms)

### Additional Weather Integration

The system also considers `weather_info.sky` for additional effects:

```c
switch (weather_info.sky) {
    case SKY_RAINING:
        // Boost water/clay, help vegetation
        break;
    case SKY_LIGHTNING:
        // Major water boost, stress vegetation
        break;
    case SKY_CLOUDLESS:
        // Increased evaporation for water
        break;
}
```

## Database Schema

### resource_depletion Table

```sql
CREATE TABLE resource_depletion (
    id INT PRIMARY KEY AUTO_INCREMENT,
    zone_vnum INT NOT NULL,
    x_coord INT NOT NULL,
    y_coord INT NOT NULL,
    resource_type INT NOT NULL,
    depletion_level DECIMAL(4,3) NOT NULL DEFAULT 1.000,
    last_harvest TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY unique_location_resource (zone_vnum, x_coord, y_coord, resource_type)
);
```

### Key Fields

- **zone_vnum**: Wilderness zone identifier
- **x_coord, y_coord**: Precise wilderness coordinates  
- **resource_type**: Integer representing resource type (0-9)
- **depletion_level**: Current availability (0.0 = depleted, 1.0 = fully available)
- **last_harvest**: Timestamp of last interaction (for regeneration calculation)

## Implementation Details

### Trigger Mechanism

Regeneration is triggered when a player enters a wilderness room:

```c
// In handler.c - char_to_room()
if (ROOM_FLAGGED(dest, ROOM_WILDERNESS)) {
    int resource_type;
    for (resource_type = 0; resource_type < NUM_RESOURCE_TYPES; resource_type++) {
        apply_lazy_regeneration(dest, resource_type);
    }
}
```

### Coordinate-Based Tracking

Unlike room-based systems, this uses precise coordinates:
- Supports dynamic wilderness generation
- Maintains consistency across room reloads
- Enables fine-grained location tracking

### Thread Safety

- Uses `mysql_query_safe()` and `mysql_store_result_safe()` wrappers
- Handles MySQL connection failures gracefully
- Falls back to mock data when database unavailable

## Configuration

### Adjusting Base Rates

Modify rates in `get_resource_regeneration_rate()`:

```c
case RESOURCE_HERBS:
    base_rate = 0.08;  // Change from 8% to desired rate
    break;
```

### Modifying Seasonal Effects

Edit `get_seasonal_modifier()` in `resource_system.c`:

```c
case RESOURCE_VEGETATION:
    switch (time_info.month) {
        case 0: case 1: case 2: 
            return 0.3;  // Adjust winter modifier
        // ... other seasons
    }
```

### Weather Sensitivity

Adjust weather effects in `get_weather_modifier()`:

```c
if (weather_value >= 225) {
    // Thunderstorm effects
    switch (resource_type) {
        case RESOURCE_WATER:
            weather_factor = 2.0;  // Adjust multiplier
            break;
    }
}
```

## Testing and Debugging

### Debug Commands

#### Show Resource Status
```
survey detailed  // Shows current resource levels and regeneration info
```

#### Admin Commands
```
regen status <zone> <x> <y>  // Show regeneration data for coordinates
regen force <zone> <x> <y>   // Force regeneration calculation
```

### Test Scenarios

1. **Time Progression**: Advance game time and verify seasonal changes
2. **Weather Variation**: Test different weather conditions in same location
3. **Coordinate Accuracy**: Verify same coordinates always return same data
4. **Edge Cases**: Test with extreme weather values and long time periods

### Logging

Enable debug logging by adding to `resource_depletion.c`:

```c
log("REGEN: %s at (%d,%d) - Base: %.3f, Season: %.3f, Weather: %.3f, Final: %.3f",
    resource_names[resource_type], x, y, base_rate, seasonal_mod, weather_mod, final_rate);
```

## Performance Considerations

### Optimization Strategies

1. **Lazy Evaluation**: Only calculates when needed
2. **Batch Updates**: Updates all resources for a location simultaneously
3. **Caching**: Room coordinates cached to avoid repeated lookups
4. **Selective Processing**: Only processes wilderness rooms

### Performance Monitoring

- Track database query execution times
- Monitor memory usage during bulk regeneration
- Profile coordinate lookup performance

### Scalability

- System scales linearly with player movement
- Database indexed on (zone_vnum, x_coord, y_coord)
- Minimal overhead for inactive wilderness areas

## Future Enhancements

### Potential Improvements

1. **Biome-Specific Modifiers**: Different effects for forests vs. deserts
2. **Magical Influence**: Areas with high magic affecting regeneration
3. **Player Conservation**: Individual player impact on regeneration rates
4. **Seasonal Events**: Special weather events affecting large areas
5. **Resource Interdependency**: Some resources affecting others' regeneration

### Backward Compatibility

- System gracefully handles missing database entries
- Falls back to default regeneration when modifiers unavailable
- Compatible with existing resource harvesting commands

## Troubleshooting

### Common Issues

1. **No Regeneration**: Check MySQL connection and database initialization
2. **Incorrect Rates**: Verify seasonal and weather modifier functions
3. **Coordinate Mismatch**: Ensure consistent coordinate calculation
4. **Performance Issues**: Check database indexing and query optimization

### Error Handling

The system includes comprehensive error handling:
- Database connection failures
- Invalid coordinate ranges  
- Malformed timestamps
- Resource type validation

All errors are logged with descriptive messages for debugging.

---

*Last Updated: August 11, 2025*
*System Version: Phase 6 - Step 4 Complete*
*Author: LuminariMUD Development Team*
