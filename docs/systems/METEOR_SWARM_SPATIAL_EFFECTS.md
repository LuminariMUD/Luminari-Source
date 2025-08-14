# Meteor Swarm Spatial Effects Implementation

## Overview

We've implemented a spectacular multi-phase spatial audio/visual system for the **Meteor Swarm** spell that showcases the power of the triple strategy pattern architecture. Players throughout the wilderness will experience realistic visual and audio effects based on their distance from the caster.

## Implementation Details

### Phase-Based Effects System

The meteor swarm creates **4 distinct phases** of spatial effects:

#### **Phase 1: Distant Visual (15 room range)**
- **Function**: `spatial_visual_test_meteor_approach()`
- **Effect**: Meteors appearing high in the sky (elevation 500)
- **Message**: "brilliant streaks of crimson and gold fire tearing across the starlit heavens, ancient celestial stones awakening from their cosmic slumber"
- **Range**: 15 rooms - visible from a considerable distance

#### **Phase 2: Approach Audio (12 room range)**  
- **Function**: `spatial_audio_test_sound_effect()`
- **Effect**: Low frequency rumbling as meteors approach
- **Message**: "an ominous crescendo of ethereal whistling and deep atmospheric rumbling as the very air trembles before celestial wrath"
- **Frequency**: `AUDIO_FREQ_LOW` (travels farther)
- **Range**: 12 rooms

#### **Phase 3: Descent Visual (8 room range)**
- **Function**: `spatial_visual_test_meteor_descent()`
- **Effect**: Meteors descending closer to target (elevation 200)
- **Message**: "colossal blazing meteorites plummeting through the atmosphere with trails of molten starfire, their surfaces crackling with primordial flame"
- **Range**: 8 rooms - closer visual as they descend

#### **Phase 4: Impact Effects (after damage)**
- **Visual**: `spatial_test_meteor_impact()` (10 room range)
- **Audio**: `spatial_audio_test_sound_effect()` (20 room range)
- **Effects**: 
  - Visual: "cataclysmic eruptions of azure and crimson flame as celestial hammers shatter the earth, sending waves of molten rock and starfire skyward"
  - Audio: "earth-shaking detonations that rival the roar of mountain avalanches as cosmic forces unleash their devastating fury upon the mortal realm"

## Player Experience Examples

### **Observer at 500 units from caster:**
```
A mage casts meteor swarm...

You see blazing meteors streaking through the distant sky, clearly visible.

You hear a deep rumbling and whistling as meteors tear through the air.

You see massive meteors blazing overhead, descending rapidly toward their target.

[DAMAGE APPLIED TO TARGETS]

You see massive fiery explosions as meteors crash into the ground, brilliant and devastating.

You hear thunderous crashes and explosions as meteors impact the earth.
```

### **Observer at 2000 units from caster:**
```
A mage casts meteor swarm...

You glimpse blazing meteors streaking through the distant sky on the horizon.

You hear the faint rumbling and whistling of something massive passing overhead.

[DAMAGE APPLIED - out of range for close visual]

You hear distant thunderous crashes and explosions echoing across the landscape.
```

### **Observer at 4000 units from caster:**
```
A mage casts meteor swarm...

[Too far for any visual effects]

[DAMAGE APPLIED - completely out of range]

You hear very faint distant rumbling, almost like thunder on the horizon.
```

## Code Integration Points

### **Magic System Integration**
- **File**: `src/magic.c`
- **Location**: `mag_areas()` function for SPELL_METEOR_SWARM
- **Pre-damage**: Visual approach and audio whistling
- **Post-damage**: Impact visual and audio effects

### **Spatial System Integration**
- **Visual**: `src/systems/spatial/spatial_visual.c`
- **Audio**: `src/systems/spatial/spatial_audio.c`
- **Headers**: Updated declarations in `spatial_visual.h` and `spatial_audio.h`

### **New Functions Added**

#### Visual Functions:
```c
int spatial_visual_test_meteor_approach(int meteor_x, int meteor_y, const char *meteor_desc, int visual_range);
int spatial_visual_test_meteor_descent(int meteor_x, int meteor_y, const char *meteor_desc, int visual_range);
int spatial_test_meteor_impact(int impact_x, int impact_y, const char *impact_desc, int range);
```

#### Audio Functions:
```c
int spatial_audio_test_sound_effect(int source_x, int source_y, int source_z, 
                                   const char *sound_desc, int frequency, int range);
```

## Technical Features Demonstrated

### **Triple Strategy Pattern in Action**
1. **Stimulus Strategy**: Different intensities for each phase
2. **Line of Sight Strategy**: Terrain blocking works for all phases
3. **Modifier Strategy**: Weather/time effects apply to all phases

### **Environmental Integration**
- **Wilderness Detection**: Only triggers in wilderness zones
- **3D Positioning**: Uses X_LOC(), Y_LOC(), and elevation data
- **Distance Calculation**: Realistic 3D distance calculations
- **Range Scaling**: Different ranges for different effect types

### **Audio Frequency System**
- **Low Frequency**: Deep rumbling travels farther (approach & impact)
- **Terrain Propagation**: Sound travels around obstacles
- **Volume Scaling**: Intensity decreases with distance

### **Visual Line of Sight**
- **Elevation Effects**: High meteors visible farther
- **Terrain Occlusion**: Mountains can block distant meteor sightings
- **Weather Effects**: Fog/rain reduce visibility of meteors

## Future Enhancement Opportunities

### **Additional Spell Integration**
- **Ice Storm**: Howling winds and visual ice shards
- **Fire Storm**: Roaring flames and intense heat visual effects
- **Chain Lightning**: Crackling audio and brilliant light flashes
- **Earthquake**: Ground rumbling audio and visual terrain shaking

### **Spell-Specific Enhancements**
- **Meteor Count**: Scale effects based on caster level
- **Target Direction**: Show meteors approaching from specific directions
- **Damage Correlation**: Larger explosions for higher damage rolls
- **Terrain Interaction**: Different impact effects on different terrain

### **Advanced Features**
- **Sequential Impacts**: Multiple meteors hitting at different times
- **Crater Effects**: Visual evidence of impact locations
- **Debris Field**: Secondary effects from meteor fragments
- **Atmospheric Entry**: Sonic boom effects for very large meteors

## Testing Commands

Once implemented, test with:
```
cast 'meteor swarm'
```

In wilderness areas, all nearby players will experience the full sequence of spatial effects!

## Performance Considerations

- **Range Optimization**: Uses efficient distance checks before processing
- **Wilderness Only**: Effects only trigger in wilderness zones to limit scope
- **Memory Management**: All contexts properly allocated and freed
- **Logging**: Comprehensive logging for debugging and performance monitoring

## Conclusion

This implementation demonstrates the power and flexibility of the spatial systems architecture. The meteor swarm spell now provides a truly immersive, cinematic experience that enhances gameplay while showcasing realistic physics-based audio and visual propagation.

The multi-phase approach creates anticipation and drama, while the triple strategy pattern ensures consistent behavior with environmental effects like weather, terrain, and line-of-sight obstruction.
