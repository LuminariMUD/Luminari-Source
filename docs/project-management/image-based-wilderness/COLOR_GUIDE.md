# Color Guide for Image-Based Wilderness

## Overview

This guide defines the RGB color values that map to specific terrain types in the image-based wilderness system. When creating wilderness images, use these exact colors to ensure proper terrain recognition.

## Standard Terrain Colors

### Water Terrain
| Color | RGB Values | Hex Code | Terrain Type | Description |
|-------|------------|----------|--------------|-------------|
| ![#0000FF](https://placehold.it/15/0000FF/000000?text=+) | (0, 0, 255) | #0000FF | SECT_OCEAN | Deep ocean water |
| ![#4080FF](https://placehold.it/15/4080FF/000000?text=+) | (64, 128, 255) | #4080FF | SECT_WATER_SWIM | Shallow swimming water |

### Coastal Terrain  
| Color | RGB Values | Hex Code | Terrain Type | Description |
|-------|------------|----------|--------------|-------------|
| ![#FFFF80](https://placehold.it/15/FFFF80/000000?text=+) | (255, 255, 128) | #FFFF80 | SECT_BEACH | Sandy beaches |
| ![#8B4513](https://placehold.it/15/8B4513/000000?text=+) | (139, 69, 19) | #8B4513 | SECT_MARSHLAND | Coastal marshes |

### Forest Terrain
| Color | RGB Values | Hex Code | Terrain Type | Description |
|-------|------------|----------|--------------|-------------|
| ![#228B22](https://placehold.it/15/228B22/000000?text=+) | (34, 139, 34) | #228B22 | SECT_FOREST | Temperate forests |
| ![#006400](https://placehold.it/15/006400/000000?text=+) | (0, 100, 0) | #006400 | SECT_JUNGLE | Tropical jungles |
| ![#3CB371](https://placehold.it/15/3CB371/000000?text=+) | (60, 179, 113) | #3CB371 | SECT_TAIGA | Northern coniferous forests |

### Elevated Terrain
| Color | RGB Values | Hex Code | Terrain Type | Description |
|-------|------------|----------|--------------|-------------|
| ![#A0522D](https://placehold.it/15/A0522D/000000?text=+) | (160, 82, 45) | #A0522D | SECT_HILLS | Rolling hills |
| ![#808080](https://placehold.it/15/808080/000000?text=+) | (128, 128, 128) | #808080 | SECT_MOUNTAIN | Mountain ranges |
| ![#404040](https://placehold.it/15/404040/000000?text=+) | (64, 64, 64) | #404040 | SECT_HIGH_MOUNTAIN | High peaks |

### Plains and Desert
| Color | RGB Values | Hex Code | Terrain Type | Description |
|-------|------------|----------|--------------|-------------|
| ![#FFFF00](https://placehold.it/15/FFFF00/000000?text=+) | (255, 255, 0) | #FFFF00 | SECT_FIELD | Grasslands and plains |
| ![#FFD700](https://placehold.it/15/FFD700/000000?text=+) | (255, 215, 0) | #FFD700 | SECT_DESERT | Arid desert |

### Wetlands and Cold Terrain
| Color | RGB Values | Hex Code | Terrain Type | Description |
|-------|------------|----------|--------------|-------------|
| ![#556B2F](https://placehold.it/15/556B2F/000000?text=+) | (85, 107, 47) | #556B2F | SECT_SWAMP | Swamplands |
| ![#B0C4DE](https://placehold.it/15/B0C4DE/000000?text=+) | (176, 196, 222) | #B0C4DE | SECT_TUNDRA | Frozen tundra |

### Special/Default
| Color | RGB Values | Hex Code | Terrain Type | Description |
|-------|------------|----------|--------------|-------------|
| ![#FFFFFF](https://placehold.it/15/FFFFFF/000000?text=+) | (255, 255, 255) | #FFFFFF | SECT_INSIDE | Undefined/error terrain |

## Image Creation Guidelines

### Resolution Requirements
- **Image size directly determines wilderness size**
- **Minimum**: 256x256 pixels = 256x256 wilderness grid
- **Recommended**: 1024x1024 pixels = 1024x1024 wilderness grid  
- **Maximum**: 2048x2048 pixels = 2048x2048 wilderness grid
- **Direct mapping**: World coordinate (x,y) = Image pixel (x,y)

### Color Precision
- Use exact RGB values listed above for best results
- The system uses Euclidean distance matching for color recognition
- Colors within ~10 RGB units may be recognized as intended terrain
- For precision, avoid anti-aliasing when painting terrain boundaries

### Design Principles

#### Geographic Realism
1. **Water Placement**: Oceans at edges, lakes and rivers inland
2. **Elevation Gradients**: Mountains don't appear next to oceans suddenly
3. **Climate Zones**: Deserts in inland areas, tundra in northern regions
4. **Forest Distribution**: Forests in temperate zones, jungles in warm areas

#### Terrain Value System
- **Elevation**: Determined by sector type (mountains=high, oceans=low)
- **Moisture**: Determined by sector type (swamps=wet, deserts=dry)
- **Temperature**: Base temperature from sector type, modified by latitude (Y-coordinate)

#### Coordinate System
- **Y-coordinate (Latitude)**: Affects temperature only (center=warm, edges=cold)
- **Image Colors**: Determine all other terrain properties via sector type

### Tools and Software

#### Recommended Image Editors
1. **GIMP** (Free) - Excellent palette control
2. **Photoshop** - Professional features
3. **Paint.NET** - Simple and effective
4. **Aseprite** - Pixel art focused

#### Palette Setup
Create a custom palette with the exact RGB values listed above:

```
GIMP Palette File
Name: Wilderness Terrain
#
  0   0 255 Deep Ocean
 64 128 255 Shallow Water
255 255 128 Beach
 34 139  34 Forest
  0 100   0 Jungle
160  82  45 Hills
128 128 128 Mountain
 64  64  64 High Mountain
255 255   0 Grassland
255 215   0 Desert
 85 107  47 Swamp
139  69  19 Marshland
176 196 222 Tundra
 60 179 113 Taiga
255 255 255 Undefined
```

## File Format Requirements

### Supported Formats
- **PNG** (Preferred) - Best compression and quality
- **BMP** (Fallback) - Simple format, larger files

### File Specifications
- **Color Depth**: 24-bit RGB (no alpha channel needed)
- **Compression**: PNG compression acceptable
- **File Size**: Under 50MB for performance

### File Location
Place wilderness image at: `lib/world/wilderness_map.png`

## Testing Your Image

### Color Validation
1. Open image in editor with eyedropper tool
2. Verify RGB values match the table above exactly
3. Check for unintended color variations from compression

### Game Testing
1. Enable image wilderness system in configuration
2. Use `reload wilderness` admin command
3. Generate test maps to verify terrain recognition
4. Check log files for color matching errors

## Common Issues

### Color Matching Problems
- **Anti-aliasing**: Disable when drawing terrain boundaries
- **Compression artifacts**: Use PNG instead of JPEG
- **Color space**: Ensure image is in sRGB color space

### Performance Issues
- **Large images**: Consider scaling down to 1024x1024
- **Memory usage**: Monitor server memory after image loading
- **Loading time**: Optimize image file size

### Design Issues
- **Unrealistic geography**: Ocean next to mountains
- **Climate mismatch**: Desert in northern regions
- **Scale problems**: Too many small terrain patches

## Advanced Techniques

### Sector-Based Terrain Values
Each terrain type has inherent elevation, moisture, and temperature values:

**Elevation Values (0-255 scale)**:
- Ocean: 60 (below waterline)
- Beach: 140 (just above waterline)  
- Plains: 160 (grassland level)
- Hills: 200 (elevated terrain)
- Mountains: 230 (high elevations)
- High Mountains: 250 (highest peaks)

**Moisture Values (0-255 scale)**:
- Desert: 30 (very dry)
- Plains: 120 (moderate)
- Forest: 180 (forest moisture)
- Jungle: 220 (tropical moisture)
- Swamp: 250 (very wet)
- Ocean: 255 (maximum moisture)

**Base Temperature Values (-30 to +35°C)**:
- Tundra: -15°C (frozen)
- High Mountains: -10°C (very cold peaks)
- Taiga: 5°C (cold forest)
- Mountains: 5°C (cold mountains)
- Forest: 18°C (forest shade)
- Plains: 20°C (temperate)
- Desert: 30°C (hot arid)
- Jungle: 28°C (hot tropical)

### Latitude Temperature Modifier
The Y-coordinate (latitude) modifies base temperature:
- Center of image: No temperature change (equator)
- Top/Bottom edges: Up to -15°C modifier (polar regions)
- Use this for realistic climate distribution

### Strategic Placement
- Place hot sectors (desert, jungle) near image center for realism
- Place cold sectors (tundra, high mountains) near image edges
- Use elevation progression: ocean → beach → plains → hills → mountains

## Campaign-Specific Palettes

### DragonLance (Krynn)
Consider using more muted earth tones:
- Replace bright greens with darker forest colors
- Use more brown/gray tones for realism

### Forgotten Realms (Faerun)
May include more diverse terrain:
- Additional forest subtypes
- Varied mountain colors
- Magical terrain indicators

### Custom Campaigns
Create your own palette by:
1. Copying the standard palette file
2. Modifying RGB values as needed
3. Updating terrain_colors[] array in code
4. Testing thoroughly before deployment

## Validation Tools

### Color Checker Script
```bash
# Check for non-standard colors in image
identify -verbose wilderness_map.png | grep -A 1000 "Histogram:"
```

### Image Analysis
Use the admin command `analyze wilderness` to see:
- Color distribution statistics
- Unrecognized color warnings
- Terrain type percentages

This color guide ensures consistent and predictable terrain generation from your wilderness images.
