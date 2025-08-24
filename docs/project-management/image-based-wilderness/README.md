# Image-Based Wilderness System

## Overview

This directory contains the complete implementation### Integration Impact

### Affected Systems
- **Base Wilderness Generation**: Core terrain calculation functions (elevation, moisture, temperature, sectors)
- **World Coordinate System**: Dynamic coordinate range based on image dimensions
- **Coordinate Validation**: Hard-coded boundary checks need dynamic validation (5-7 locations)
- **Map Visualization**: Terrain display and mapping (base terrain from image)

### Minimally Affected Systems (Auto-Compatible with Dynamic Sizing)
- **Resource System**: Automatically scales with updated WILD_X_SIZE/WILD_Y_SIZE
- **Perlin Noise Scaling**: All existing scaling functions work unchanged
- **Map Generation**: Knowledge base and wilderness maps auto-scale
- **Mathematical Calculations**: Most algorithms remain unchangedr adding image-based wilderness generation as an alternative to the current Perlin noise system in LuminariMUD.

## System Goals

The image-based wilderness system allows world designers to create **base terrain** using image files where:
- **Colors define terrain sectors** (ocean, forest, mountain, etc.)
- **Elevation is estimated from sector type** (mountains=high, ocean=low, etc.)
- **Moisture is estimated from sector type** (swamp=wet, desert=dry, etc.)
- **Temperature is estimated from sector type + Y-coordinate latitude** (jungle=hot, tundra=cold, with latitude modifier)

**IMPORTANT SCOPE**: Only base terrain generation is affected. Weather, resources, and other noise layers continue using Perlin noise and automatically scale with updated `WILD_X_SIZE`/`WILD_Y_SIZE` constants.

## Key Features

- **Selective Layer Replacement**: Only base terrain uses image data (elevation, moisture, temperature, sectors)
- **Perlin Noise Preserved**: Weather, resources, and other noise layers continue using Perlin noise
- **Conditional Compilation**: Coexists with Perlin noise via `#ifdef USE_IMAGE_WILDERNESS`
- **Automatic Scaling**: Perlin noise layers automatically use updated size constants (no code changes needed)
- **Campaign Integration**: Different images per campaign setting
- **Fallback System**: Graceful degradation to Perlin noise if image unavailable
- **Performance Optimized**: Pixel caching and efficient coordinate mapping
- **Admin Tools**: Commands for image management and debugging

## Documentation Files

### [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)
Complete technical implementation plan including:
- Current system analysis
- Detailed phase-by-phase implementation
- Code examples and data structures
- Integration points and testing strategy
- Performance considerations and risk analysis

### [COLOR_GUIDE.md](COLOR_GUIDE.md)  
Comprehensive guide for creating wilderness images:
- RGB color values for each terrain type
- Image creation guidelines and tools
- File format requirements
- Testing and validation procedures
- Common issues and solutions

### [TECHNICAL_ARCHITECTURE.md](TECHNICAL_ARCHITECTURE.md)
Deep technical architecture documentation:
- System architecture diagrams
- Data flow specifications
- Core API function definitions
- Algorithm implementations
- Memory management and optimization

### [DEVELOPMENT_CHECKLIST.md](DEVELOPMENT_CHECKLIST.md)
Complete development and testing checklist:
- Phase-by-phase task breakdown
- Testing requirements and validation
- Quality assurance procedures
- Deployment checklist
- Risk mitigation strategies

## Quick Start Summary

### For Developers

1. **Enable the Feature**: Add `#define USE_IMAGE_WILDERNESS` to `src/campaign.h`
2. **Build Dependencies**: Install libpng development libraries
3. **Create Image**: Use the color guide to create `lib/world/wilderness_map.png`
4. **Implement Core**: Follow the implementation plan phases
5. **Test**: Use the development checklist for validation

### For World Designers

1. **Choose Colors**: Use exact RGB values from the color guide
2. **Design Geography**: Consider realistic terrain distribution
3. **Test Image**: Use image editing tools to verify color accuracy
4. **Deploy**: Place image at `lib/world/wilderness_map.png`
5. **Validate**: Use in-game admin commands to verify terrain generation

## Configuration Options

```c
/* Campaign-specific configuration */
#ifdef USE_IMAGE_WILDERNESS
  #define IMAGE_WILDERNESS_ENABLED 1
  #define WILDERNESS_IMAGE_PATH "lib/world/wilderness_map.png"
  #define COLOR_MATCH_THRESHOLD 10
#endif

/* Campaign-specific images */
#if defined(CAMPAIGN_DL)
  #define WILDERNESS_IMAGE_PATH "lib/world/krynn_wilderness.png"
#elif defined(CAMPAIGN_FR)  
  #define WILDERNESS_IMAGE_PATH "lib/world/faerun_wilderness.png"
#endif
```

## Integration Impact

### Affected Systems
- **Base Wilderness Generation**: Core terrain calculation functions (elevation, moisture, temperature, sectors)
- **Resource System**: Uses base terrain from image + resource-specific Perlin noise layers
- **Region System**: Overlays on wilderness terrain (no changes needed)
- **Navigation**: Movement through wilderness areas (no changes needed)
- **Map Visualization**: Terrain display and mapping (base terrain from image)

### Unaffected Systems
- **Weather System**: Always continues using Perlin noise (get_weather function never modified)
- **Resource Noise Layers**: All resource-specific noise layers continue using Perlin noise
- **Character Data**: No impact on existing saves
- **Database**: No schema changes required
- **Client Protocol**: No changes to player experience
- **World Building**: Static rooms and zones unchanged

## Performance Impact

### Memory Usage
- **Image Storage**: Depends on image size (width × height × 3 bytes RGB)
  - 1024×1024 image: ~3MB
  - 2048×2048 image: ~12MB
- **Pixel Cache**: ~32KB for LRU cache
- **Total Impact**: <15MB additional memory usage for large images

### World Size Considerations

#### **Phase 1 (Current): Compile-Time Image Sizing**
- **Developer sets #define statements** to match image dimensions before compilation
- **640×480 image**: Developer updates `WILD_X_SIZE 640`, `WILD_Y_SIZE 480` 
- **1024×1024 image**: Developer updates `WILD_X_SIZE 1024`, `WILD_Y_SIZE 1024`
- **Formula**: World range = [-(size/2), (size/2)-1] for each dimension
- **Benefit**: Minimal system impact, existing scaling functions work unchanged

#### **Phase 2 (Future): Dynamic Runtime Sizing**
- **Automatic image size detection** at runtime without recompilation
- **Dynamic coordinate system** that adapts to any image size automatically
- **Unified wilderness sizing** for both image-based and Perlin noise systems
- **Benefit**: Complete flexibility without developer intervention or recompilation

### Speed Comparison
- **Image Loading**: One-time 5-second cost at startup
- **Terrain Generation**: Comparable to Perlin noise
- **Pixel Access**: <10 microseconds with caching
- **Overall Impact**: <10% performance change

## Future Enhancements

### **Phase 1 Improvements (Near-term)**
- **Multiple Images**: Seasonal terrain variations
- **Advanced Mapping**: Color gradient support  
- **Campaign Integration**: Setting-specific image selection
- **Admin Tools**: Enhanced debugging and validation commands

### **Phase 2 Features (Long-term)**
- **Dynamic Runtime Sizing**: Automatic coordinate system scaling based on image dimensions
- **Unified Wilderness Architecture**: Consistent sizing system for both image and Perlin noise modes
- **Hot-Swappable Images**: Runtime image loading without server restart
- **Hybrid System**: Combine image sectors with Perlin noise details
- **Edit Tools**: In-game terrain editing capabilities

### Campaign Extensions
- **Themed Palettes**: Campaign-specific color schemes
- **Cultural Terrain**: Setting-appropriate terrain types
- **Magical Areas**: Special terrain for fantasy elements
- **Historical Maps**: Time-period specific landscapes

## Development Status

### **Phase 1: Compile-Time Image Sizing**
| Task | Status | Priority |
|------|--------|----------|
| Planning & Architecture | ✅ Complete | High |
| Image Loading Infrastructure | 🔄 Ready to Start | High |
| Core Image-Based Functions | ⏳ Pending | High |
| Coordinate Validation Updates | ⏳ Pending | Medium |
| Testing & Validation | ⏳ Pending | High |
| Documentation | 🔄 In Progress | Medium |

### **Phase 2: Dynamic Runtime Sizing** 
| Task | Status | Priority |
|------|--------|----------|
| Wilderness Architecture Refactor | ⏳ Future | Low |
| Dynamic Coordinate System | ⏳ Future | Low |
| Unified Sizing System | ⏳ Future | Low |
| Runtime Image Loading | ⏳ Future | Low |

## Getting Started

1. **Read the Implementation Plan** - Understand the technical approach
2. **Review the Color Guide** - Learn how to create terrain images  
3. **Study the Architecture** - Understand system integration points
4. **Follow the Checklist** - Use for systematic development
5. **Create Test Images** - Start with small test cases
6. **Implement Phase by Phase** - Build incrementally with testing

This system provides a powerful alternative to Perlin noise while maintaining full backwards compatibility and system stability.
