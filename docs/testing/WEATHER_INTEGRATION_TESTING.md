# Weather Integration Testing Guide

## Overview
The dynamic wilderness description system now includes weather integration that provides weather-aware descriptions based on coordinate-specific weather patterns in wilderness areas and global weather in non-wilderness areas.

## New Admin Commands

### settime Command
**Usage:** `settime <hour> [day] [month]`

Sets the game time for testing time-of-day descriptions.

**Parameters:**
- `hour`: 0-23 (required)
- `day`: 1-35 (optional, defaults to current day)
- `month`: 0-16 (optional, defaults to current month)

**Examples:**
```
settime 14          # Set to 2 PM, keep current day/month
settime 22 15       # Set to 10 PM on day 15, keep current month
settime 6 1 0       # Set to 6 AM on day 1 of month 0
```

**Sunlight States:**
- Hours 5-6: Dawn (SUN_RISE)
- Hours 6-21: Daylight (SUN_LIGHT)
- Hours 21-22: Dusk (SUN_SET)
- Hours 22-5: Dark (SUN_DARK)

### setweather Command
**Usage:** `setweather <0-4>`

Sets weather for testing weather-aware descriptions.

**Weather Types:**
- `0` - Clear/Cloudless
- `1` - Cloudy
- `2` - Rainy
- `3` - Stormy
- `4` - Lightning

**Important Notes:**
- **Wilderness Areas**: Weather is coordinate-based using Perlin noise and cannot be manually changed. The command will show the current wilderness weather value and type.
- **Non-Wilderness Areas**: Uses global weather system that can be changed with this command.

**Examples:**
```
setweather 0    # Set clear weather (non-wilderness only)
setweather 2    # Set rainy weather (non-wilderness only)
setweather 4    # Set lightning storms (non-wilderness only)
```

## Weather System Details

### Wilderness Weather
- **Range**: 0-255 (generated using Perlin noise)
- **Clear**: 0-177
- **Cloudy**: 128-177 (mid-range values)
- **Rainy**: 178-199
- **Stormy**: 200-224
- **Lightning**: 225-255

### Weather-Aware Descriptions

The system now includes weather-specific description variations:

#### Atmospheric Descriptions
- **Clear**: "Clear skies stretch overhead", "The air is crisp and clean"
- **Cloudy**: "Clouds gather overhead", "A gray overcast dims the light"
- **Rainy**: "Rain falls steadily", "Droplets create small puddles"
- **Stormy**: "Wind whips through the area", "Heavy rain lashes the landscape"
- **Lightning**: "Thunder rumbles ominously", "Lightning illuminates the storm clouds"

#### Vegetation Descriptions
Weather affects how vegetation is described:
- **Rain/Storms**: Plants appear "rain-washed", leaves "drip with moisture"
- **Clear**: Vegetation appears "sun-dappled", "basking in sunlight"
- **Cloudy**: Plants appear "subdued in the gray light"

#### Geological Descriptions
Weather affects terrain descriptions:
- **Rain**: Stone surfaces "glisten with rain", rocks appear "darkened by moisture"
- **Storms**: Mountain peaks "disappear into storm clouds"
- **Clear**: Rocky areas "bask in clear sunlight"

## Testing Procedures

### Basic Weather Testing
1. Use `setweather` in a non-wilderness area to test different weather types
2. Use `look` to see how room descriptions change with weather
3. Move between different terrain types to see weather effects on various landscapes

### Wilderness Weather Testing
1. Travel to wilderness coordinates (check with `setweather` to see current values)
2. Move to different coordinates to experience varying weather patterns
3. Note how weather changes descriptions across different terrain types

### Time-of-Day Testing
1. Use `settime` to test different hours
2. Observe how lighting conditions affect descriptions
3. Test interaction between time-of-day and weather (e.g., storms at night vs. day)

### Combined Testing
1. Test weather + time combinations:
   - Stormy night scenes
   - Clear dawn descriptions
   - Rainy midday atmosphere
   - Lightning at dusk

## Expected Results

### Clear Weather Examples
- "Clear skies stretch overhead, providing excellent visibility across the landscape."
- "The bright sun casts sharp shadows among the rocks."

### Rainy Weather Examples
- "Rain falls steadily, creating small rivulets that wind between the stones."
- "The vegetation appears lush and rain-washed, leaves glistening with moisture."

### Storm Weather Examples
- "Wind whips through the area, bending trees and stirring up loose debris."
- "Heavy rain lashes the landscape, making visibility poor."

### Lightning Weather Examples
- "Thunder rumbles ominously overhead, echoing across the terrain."
- "Lightning illuminates the storm clouds, casting everything in stark relief."

## Performance Notes

- Weather calculations use coordinate-based Perlin noise in wilderness areas
- Weather descriptions are generated dynamically and cache-friendly
- The system integrates seamlessly with existing resource depletion and time-of-day systems

## Troubleshooting

If weather descriptions don't appear:
1. Verify you're using the correct terrain type (weather affects different terrains differently)
2. Check that ENABLE_DYNAMIC_RESOURCE_DESCRIPTIONS is enabled in campaign.h
3. Ensure you're testing in areas that support dynamic descriptions
4. Use `setweather` to verify current weather state

## Next Steps

Future enhancements may include:
- Seasonal weather variations
- Elevation-based weather effects
- Regional weather patterns
- Weather event notifications to players
