# Narrative Weaver Enhancement TODO

**Last Updated**: August 19, 2025  
**Mission**: Transform procedurally generated wilderness descriptions into unique regional experiences by weaving AI-generated hints and atmospheric elements into the base content.

## System Overview

**Content Flow**:
1. **Procedural Generation** → Creates dynamic wilderness descriptions based on game state
2. **Narrative Weaver** → Enhances descriptions with regional hints for unique atmospheric experiences  
3. **Player Experience** → Regions become memorable, distinctive locations with contextual atmosphere

**Current Status**: Basic hint layering implemented, extensive AI-generated metadata available but underutilized.

---

## 🚀 Phase 1: Utilize Existing AI-Generated Metadata (High Impact, Low Effort)

### **Priority 1: Advanced Hint Filtering with AI Weights**
- [ ] **Implement JSON seasonal_weight utilization**
  - **Current**: Basic hint loading with no seasonal consideration
  - **Available**: AI-calculated seasonal coefficients per hint (spring: 0.9, summer: 1.0, autumn: 0.8, winter: 0.7)
  - **Implementation**: Parse JSON weights in hint selection logic
  - **Impact**: Contextually appropriate seasonal atmosphere
  - **Files**: `narrative_weaver.c`

- [ ] **Implement JSON time_of_day_weight utilization**  
  - **Current**: Basic time category string
  - **Available**: AI-calculated time coefficients (dawn: 1.0, midday: 0.7, night: 0.9, etc.)
  - **Implementation**: Apply time-based weighting to hint relevance scoring
  - **Impact**: Time-appropriate atmospheric enhancement
  - **Files**: `narrative_weaver.c`

### **Priority 2: Complete Missing Hint Category Processing**
- [ ] **Add HINT_SEASONAL_CHANGES processing**
  - **Current**: Parsed but not used in `weave_unified_description()` or `simple_hint_layering()`
  - **Implementation**: Add to hint processing switch statements
  - **Impact**: Seasonal contextual enhancement
  - **Files**: `narrative_weaver.c`

- [ ] **Add HINT_TIME_OF_DAY processing**
  - **Current**: Parsed but not used in hint weaving functions
  - **Implementation**: Add time-based atmospheric enhancement
  - **Impact**: Day/night cycle integration
  - **Files**: `narrative_weaver.c`

- [ ] **Add HINT_RESOURCES processing**
  - **Current**: Parsed but not processed in weaving functions
  - **Implementation**: Integrate with procedural resource state
  - **Impact**: Resource-aware regional atmosphere
  - **Files**: `narrative_weaver.c`

### **Priority 3: AI Quality and Approval Integration**
- [ ] **Use AI quality scores for hint selection**
  - **Current**: No quality awareness in hint selection
  - **Available**: AI quality scores (0.00-5.00), approval status, review flags
  - **Implementation**: Prefer high-quality approved hints in selection algorithm
  - **Impact**: Ensure only best AI content reaches players
  - **Files**: `narrative_weaver.c`

- [ ] **Integrate AI-generated regional profiles**
  - **Current**: No regional personality awareness
  - **Available**: AI-generated dominant_mood, key_characteristics, complexity_level
  - **Implementation**: Use profile data to guide hint selection style
  - **Impact**: Maintain AI-determined regional character consistency
  - **Files**: `narrative_weaver.c` (already has `load_region_profile()` extern)

---

## 🌟 Phase 2: Enhanced Regional Experience (Medium Effort, High Impact)

### **Contextual Intelligence Expansion**
- [ ] **Multi-condition hint filtering**
  - **Goal**: Combine weather + time + season + resource state for hint selection
  - **Implementation**: Create sophisticated relevance scoring algorithm
  - **Impact**: Highly contextual, dynamic regional atmosphere

- [ ] **Regional transition effects**
  - **Goal**: Smooth atmospheric changes when entering/leaving regions
  - **Implementation**: Gradient hint application at region boundaries
  - **Impact**: Natural feeling regional boundaries

- [ ] **Weather system integration enhancement**
  - **Goal**: Better integration with wilderness weather conditions
  - **Implementation**: Improve HINT_WEATHER_INFLUENCE usage beyond basic string matching
  - **Impact**: Dynamic weather-responsive regional character

### **Content Richness**
- [ ] **Resource-state dependent regional character**
  - **Goal**: Regions feel different based on resource abundance/depletion
  - **Implementation**: Resource-based hint weighting and selection
  - **Impact**: Living, changing regional atmosphere

- [ ] **Multiple region support**
  - **Goal**: Handle overlapping regions with different priorities
  - **Implementation**: Region priority system and hint blending
  - **Impact**: Complex, layered regional experiences

---

## 🛠️ Phase 3: System Improvements (Low Priority, Technical Debt)

### **Code Quality**
- [ ] **Consolidate hint loading functions**
  - Remove duplication between different hint loading approaches
  - Standardize on efficient database query patterns

- [ ] **Improve error handling and fallbacks**
  - Ensure graceful degradation when regional data unavailable
  - Better logging for hint selection debugging

### **Performance Optimization**
- [ ] **Implement hint caching**
  - Cache processed contextual hints by location and conditions
  - Reduce database queries for repeated location visits
  - Clear cache when regional data updates

- [ ] **Optimize database queries**
  - Combine multiple hint queries where possible
  - Index optimization for common query patterns

### **Builder Tools**
- [ ] **Regional hint management interface**
  - Tools for viewing/editing regional hints
  - Quality score visualization
  - Regional coverage analysis

---

## 🧪 Testing and Quality Assurance

### **Immediate Testing Needs**
- [ ] **Test with Mosswood region (vnum 1000004)**
  - Validate AI-generated hint integration
  - Test seasonal/time weighting
  - Verify quality score utilization

- [ ] **Create additional test regions**
  - Different terrain types
  - Various weather conditions
  - Multiple hint coverage scenarios

### **Long-term Validation**
- [ ] **Player experience assessment**
  - Gather feedback on regional atmosphere
  - Analyze description repetition
  - Measure immersion impact

- [ ] **Performance monitoring**
  - Database query performance
  - Memory usage during hint processing
  - Cache effectiveness

---

## 💡 Future Enhancements (Post-Core Implementation)

### **Advanced Features**
- [ ] **Character-specific regional perception**
  - Different hints based on character skills/background
  - Perception-based detail variations
  - Class-specific environmental awareness

- [ ] **Temporal regional evolution**
  - Long-term environmental changes
  - Season-based hint availability
  - Regional hint evolution over time

- [ ] **Player impact on regional character**
  - Track player actions affecting regional atmosphere
  - Dynamic hint generation based on player activity
  - Regional memory of significant events

---

## Implementation Guidelines

### **Development Approach**
1. **Start with existing infrastructure** - Utilize available AI-generated metadata first
2. **Incremental enhancement** - Add one feature at a time with thorough testing
3. **Maintain fallback safety** - Ensure system gracefully handles missing data
4. **Performance awareness** - Monitor resource usage as features are added

### **Files Most Likely to Change**
- `src/systems/narrative_weaver/narrative_weaver.c` - Primary implementation
- `src/systems/narrative_weaver/narrative_weaver.h` - Structure definitions  
- Database queries and hint processing logic

### **Success Metrics**
- **Functionality**: All hint categories properly processed
- **Quality**: AI metadata effectively utilized for atmospheric enhancement
- **Performance**: No significant impact on room description generation speed
- **Experience**: Regional areas feel distinctive and atmospheric to players

**Next Action**: Implement JSON seasonal/time weight parsing in hint selection logic
