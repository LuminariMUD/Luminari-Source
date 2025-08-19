# Narrative Weaver Enhancement TODO

## Current Status (Phase 1 Complete)

### ✅ Completed Features
- **Phase 1 Contextual Enhancement**: Safe enhancement of base descriptions with regional hints
- **Resource-Aware Terrain**: Terrain types now properly affect resource calculations
- **Core Hint Categories**: Flora, Atmosphere, Mystical, Geography/Landmarks implemented
- **Safe Fallback Architecture**: System preserves existing functionality for non-hint areas
- **Regional Integration**: Proper wilderness zone region lookups working
- **String Formatting**: Clean description integration without corruption

### 🎯 Currently Active
- **Hint Categories Used**: FLORA, ATMOSPHERE, MYSTICAL, GEOGRAPHY, LANDMARKS, SOUNDS (partial)
- **Enhancement Method**: Contextual parameter injection with intelligent text replacement
- **Coverage**: The Mosswood region (19 test hints) with abundant forest resource integration

## Phase 2: Immediate Wins (High Impact, Low Effort)

### 🚀 Priority 1: Seasonal Context Integration ✅ COMPLETED
- [x] **Add HINT_SEASONAL_CHANGES support**
  - ✅ Modified `extract_hint_context()` to process seasonal hints
  - ✅ Added seasonal modifiers to atmospheric descriptions
  - ✅ Implemented seasonal enhancement logic with examples: "Snow-laden branches" vs "Budding spring growth"
  - ✅ Full integration with existing The Mosswood data
  - **Implementation**: Complete seasonal context integration with `get_season_category()`, expanded `hint_context` structure, and contextual enhancement
  - **Files**: `narrative_weaver.c`, `narrative_weaver.h`

### 🚀 Priority 2: Time-of-Day Lighting Variations ✅ COMPLETED
- [x] **Add HINT_TIME_OF_DAY support**
  - ✅ Enhanced lighting context based on current game time (morning/afternoon/evening/night)
  - ✅ Replaced generic lighting with time-appropriate variants
  - ✅ Added time-specific atmospheric enhancements
  - ✅ Examples: "Dawn light" vs "Moonlight" vs "Harsh midday sun" vs "Twilight glow"
  - **Implementation**: Full time-of-day context integration with temporal lighting and atmosphere
  - **Files**: `narrative_weaver.c`, `narrative_weaver.h`

### 🚀 Priority 3: Resource-Based Flavor Text ✅ COMPLETED
- [x] **Add HINT_RESOURCES integration**
  - ✅ Integrated resource abundance to select appropriate hints
  - ✅ Added resource-based quality descriptions (abundant wood → "Rich timber stands" vs scarce wood → "Sparse groves")
  - ✅ Enhanced vegetation references based on actual resource levels at location
  - ✅ Added resource abundance atmospheric enhancements
  - **Implementation**: Full resource system integration with `get_base_resource_value()` calls and resource-driven contextual enhancement
  - **Files**: `narrative_weaver.c`, `narrative_weaver.h`

### 🚀 Priority 4: Enhanced Weather Integration
- [ ] **Improve HINT_WEATHER_INFLUENCE usage**
  - Move weather hints from template weaver to contextual enhancement
  - Better integration with current wilderness weather conditions
  - **Files**: `narrative_weaver.c`, coordinate with `get_wilderness_weather_condition()`

## Phase 3: Advanced Feature Integration (Medium Effort)

### 🔧 Sophisticated Filtering System
- [ ] **Implement advanced hint filtering**
  - Integrate `hint_matches_conditions()` function
  - Use weather/time/seasonal weighting functions
  - Dynamic hint relevance scoring
  - **Files**: `narrative_weaver.c`, `region_hints.c` integration

### 🔧 Multi-Condition Hint Selection
- [ ] **Priority-based hint combination**
  - Weight hints by multiple factors (weather + time + season + resources)
  - Avoid conflicting hints in same description
  - Better randomization with location consistency
  - **Files**: `narrative_weaver.c`, enhance `extract_hint_context()`

### 🔧 Enhanced Scent and Sound Integration
- [ ] **Improve HINT_SOUNDS and HINT_SCENTS usage**
  - Currently only used in template weaver
  - Add to contextual enhancement system
  - Context-aware sensory details
  - **Files**: `narrative_weaver.c`, expand contextual parameters

## Phase 4: Long-term Advanced Features (High Effort)

### 🌟 Dynamic Regional Systems
- [ ] **Multiple Region Integration**
  - Handle overlapping regions with different priorities
  - Blend hints from multiple sources
  - Region transition effects
  - **Files**: Multiple, significant architecture changes

### 🌟 Advanced Resource Correlation
- [ ] **Resource-State Dependent Mystical Elements**
  - High magic resources → more mystical hints
  - Depleted areas → different atmospheric hints
  - Dynamic resource-based region character
  - **Files**: `narrative_weaver.c`, `resource_system.c` integration

### 🌟 Temporal Description Variations
- [ ] **Long-term Environmental Changes**
  - Season-based hint availability
  - Regional hint evolution over time
  - Weather pattern influence on hint selection
  - **Files**: Multiple, database schema changes possible

### 🌟 Player Impact Integration
- [ ] **Character-Specific Enhancements**
  - Different hints based on character skills/background
  - Perception-based detail variations
  - Class-specific environmental awareness
  - **Files**: `narrative_weaver.c`, character system integration

## System Architecture Improvements

### 🛠️ Code Quality
- [ ] **Remove unused template weaver functions**
  - Clean up `weave_unified_description()` if not used
  - Consolidate hint loading functions
  - Reduce code duplication

### 🛠️ Performance Optimization
- [ ] **Hint caching improvements**
  - Cache processed contextual hints by location
  - Reduce database queries for repeated locations
  - **Files**: `narrative_weaver.c`, caching system

### 🛠️ Database Schema
- [ ] **Hint categorization enhancements**
  - Add hint effectiveness ratings
  - Track hint usage statistics
  - Support for conditional hint combinations
  - **Files**: SQL schema updates, database functions

## Testing and Validation

### 🧪 Quality Assurance
- [ ] **Expand hint coverage**
  - Create hints for additional regions
  - Test with different terrain types
  - Validate seasonal/weather/time combinations

### 🧪 Performance Testing
- [ ] **Load testing**
  - Test with many active regions
  - Validate cache performance
  - Memory usage optimization

### 🧪 Player Experience
- [ ] **In-game validation**
  - Player feedback on description quality
  - Immersion impact assessment
  - Repetition analysis

## Implementation Notes

### Development Priorities
1. **Immediate Wins first** - High impact features that reuse existing infrastructure
2. **Maintain safety** - All enhancements must preserve fallback functionality
3. **Incremental testing** - Test each enhancement individually
4. **Performance awareness** - Monitor resource usage as features are added

### Files Most Likely to Change
- `src/systems/narrative_weaver/narrative_weaver.c` - Primary implementation
- `src/systems/narrative_weaver/narrative_weaver.h` - Structure definitions
- `src/desc_engine.c` - Integration point
- `src/resource_system.c` - Resource correlation features

### Database Considerations
- Current schema supports all planned Phase 2 features
- Phase 3+ may require schema enhancements
- Hint data format is flexible for new categories

---

**Last Updated**: August 19, 2025  
**Current Phase**: Phase 1 Complete, Phase 2 Planning  
**Next Action**: Implement seasonal context integration
