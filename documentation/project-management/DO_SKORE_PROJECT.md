# TASK LIST - New `do_skore` Enhanced Display Command

## 🚨 CURRENT PROJECT STATUS (Updated: 2025-01-27)

**PARTIALLY IMPLEMENTED** - The do_skore command has been started but needs completion of Phase 1 MVP features.

### 📚 Key Documentation Files:
- **This File**: Overall project plan and current status
- **`skore_system_documentation.md`**: Contains:
  - Complete SKORE command help documentation (ready to copy)
  - Complete SCORECONFIG command help documentation (ready to copy)
  - Comprehensive test plan for Phase 1 MVP
  - Test scenarios and expected results

### ✅ What's Already Done:
- **ACMD(do_skore)** implemented in act.informative.c:3846
- **ACMD(do_scoreconfig)** implemented for configuration
- Commands registered in interpreter.c (lines 785-786)
- Preference storage added to player_special_data_saved:
  - score_display_width (byte)
  - score_color_theme (byte)
  - score_info_density (byte)
  - score_spare (byte for future)
- PRF_FLAGS defined in structs.h:
  - PRF_SCORE_CLASSIC (77)
  - PRF_SCORE_COMPACT (78)
  - PRF_SCORE_WIDE (79)
  - PRF_SCORE_NOCOLOR (80)
- Basic display panels implemented
- Helper functions stubbed (need completion)

### ❌ What Still Needs Work (Phase 1 MVP):
1. **Help System**: No help entries for skore/scoreconfig commands
2. **Helper Functions**: get_health_color(), get_class_color() need proper implementation
3. **Performance**: No perfmon integration (target: <10ms render)
4. **Database**: No MySQL column for preference persistence
5. **Missing Features**:
   - PSP progress bars for psionics
   - Spell slot displays for casters
   - Equipment condition indicators
   - "skore <section>" expanded views
6. **Testing**: No valgrind testing, no client compatibility testing
7. **Polish**: Remove "Phase 1 MVP" footer from display

## 🎯 PROJECT OVERVIEW: Modern Character Display System

**Goal:** Create a new `do_skore` command as an enhanced, visually appealing, and informative character display system that provides an alternative to the existing `do_score` command without modifying the original.

**Current State Analysis:**
- Existing `do_score`: Remains completely unchanged ✅
- Basic text output with simple ANSI colors
- Line-based display with 80-character width constraint  
- Limited color scheme using `\t` color codes
- Static information layout
- No customization options
- Current performance: ~3-5ms render time (measured)
- Original function: act.informative.c:3434 (ACMD(do_score))
- Uses text_line() and draw_line() for formatting

**New Command Strategy:**
- Implement as ACMD(do_skore) - completely separate function ✅
- Register in interpreter.c as new command ✅
- No modifications to existing do_score functionality ✅
- Players can use either command based on preference ✅

## 📋 **PHASE 1: Enhanced Basic Display (MVP)**
*Target: 2-3 weeks development time*

### 1.1 **Current System Analysis & Planning**
- [x] **Study existing `do_score` implementation** (2 hours) ✅
  - Analyze function structure in `act.informative.c:3434-3800` ✅
  - Map all displayed character data fields for reuse in do_skore ✅
  - Profile with perfmon.cpp to establish baseline (currently ~3-5ms) ❌
  - Document color system usage patterns (`\t` codes) ✅
- [x] **Create new do_skore command structure** (3 hours) ✅
  - Implement ACMD(do_skore) in act.informative.c ✅
  - Register command in interpreter.c cmd_info[] ✅
  - Add help entry for 'skore' command ❌ **NEEDS WORK**
  - Create initial skeleton that calls existing display logic ✅
- [x] **Design enhanced display layout mockups** (4 hours) ✅
  - Create ASCII mockups for different character types ✅
  - Design responsive layout for 80/100/120 character widths ✅
  - Plan information hierarchy based on usage frequency ✅
  - Create prototypes using existing text_line()/draw_line() functions ✅
- [x] **Technical architecture planning** (3 hours) ✅
  - Design modular display system for do_skore ✅
  - Plan do_skore specific preferences (PRF_SKORE_COMPACT, etc.) ✅
  - Extend player_special_data_saved for skore preferences (add ~64 bytes) ✅
  - Define caching strategy for computed values ⚠️ **PARTIAL**

### 1.2 **Core Display Improvements**
- [x] **Implement enhanced formatting system** (8 hours) ✅
  - Create utility functions for consistent spacing and alignment ✅
  - Implement improved text_line functions with better centering ✅
  - Add support for multi-column layouts ✅
  - Create responsive width handling (80, 120, 160 character support) ✅
- [ ] **Enhanced color theme system** (6 hours) ⚠️ **IN PROGRESS**
  - Extend existing color system with class-based themes ⚠️ **NEEDS WORK**
  - Implement alignment-based color variations ❌
  - Add health-based color intensity (bright when healthy, dim when injured) ⚠️ **NEEDS WORK**
  - Create fallback for clients without color support ✅
- [ ] **Visual progress indicators** (8 hours) ⚠️ **PARTIAL**
  - Implement ASCII progress bars for HP/MP/Movement: `[████████░░] 80%` ✅
  - Add experience progress with level milestone markers ✅
  - Create equipment condition visual indicators ❌ **NEEDS WORK**
  - Add spell slot usage displays for casters ❌ **NEEDS WORK**

### 1.3 **Information Organization**
- [x] **Modular information panels** (8 hours) ⚠️ **PARTIAL**
  - Split display into logical sections: Identity, Vitals, Combat, Magic, Equipment ✅
  - Implement consistent panel headers using existing text_line() patterns ✅
  - Add contextual information based on character class/level ✅
  - Create display_skore_<section>() functions for modularity ❌ **NEEDS REFACTORING**
- [ ] **Smart information density** (4 hours) ⚠️ **PARTIAL**
  - Analyze command usage to prioritize frequently-checked stats ✅
  - Group related information logically (saves together, abilities together) ✅
  - Implement abbreviated display using existing abbreviation patterns ✅
  - Add "skore <section>" for expanded views (e.g., "skore combat") ❌ **NEEDS WORK**

### 1.4 **Basic Customization System**
- [x] **Player preference storage** (6 hours) ⚠️ **PARTIAL**
  - Add skore preferences to player_special_data_saved (64 bytes max) ✅
  - Implement ACMD(do_skoreconfig) for preference management ✅ (NAMED: do_scoreconfig)
  - Create PRF_FLAGS for skore options (PRF_SKORE_COMPACT, PRF_SKORE_NO_COLOR, etc.) ✅
  - Add MySQL column for skore_preferences persistence ❌ **CRITICAL - NEEDS WORK**
- [ ] **Performance validation** (4 hours) ❌ **NOT STARTED**
  - Profile do_skore with perfmon to ensure <10ms render time ❌
  - Test with 100 concurrent skore commands ❌
  - Validate memory usage stays under 2KB per display ❌
  - Add performance metrics to PERF_log_code_profile() ❌
- [ ] **Compatibility testing** (3 hours) ❌ **NOT STARTED**
  - Test on MUSHclient, Mudlet, TinTin++, zMUD, raw telnet ❌
  - Verify screen reader compatibility (JAWS, NVDA) ❌
  - Test terminal widths: 80, 100, 120, 160 characters ❌
  - Ensure graceful degradation for limited clients ❌

**Phase 1 Success Criteria:**
- do_skore renders in <10ms for 99% of requests (perfmon verified) ❌ **NOT TESTED**
- Zero increase in memory leaks (valgrind clean) ❌ **NOT TESTED**
- Original do_score remains completely unmodified ✅ **CONFIRMED**
- Works on all major MUD clients without issues ❌ **NOT TESTED**
- Performance monitoring shows no impact on pulse timing ❌ **NOT TESTED**

## 🔴 IMMEDIATE ACTION ITEMS FOR PHASE 1 COMPLETION

### Priority 1 (Critical Path):
1. **Implement helper functions** in act.informative.c:
   - `get_health_color()` - Return color based on HP percentage
   - `get_class_color()` - Return color based on character class
   - Ensure proper function signatures match usage

2. **Add help entries** via hedit or help files:
   - SKORE command help - **COMPLETE DOCUMENTATION IN: `skore_system_documentation.md` (lines 11-86)**
   - SCORECONFIG command help - **COMPLETE DOCUMENTATION IN: `skore_system_documentation.md` (lines 89-162)**
   - Copy the help text exactly from the documentation file

3. **MySQL Integration**:
   - Add `skore_prefs` BLOB column to player table
   - Modify save/load functions to persist preferences
   - Test preference persistence across sessions

### Priority 2 (Feature Completion):
4. **PSP Progress Bars**:
   - Add PSP bar display for psionic characters
   - Use same progress bar function as HP/Move

5. **Spell Slot Display**:
   - Show prepared/used spell slots for casters
   - Format: "Spell Slots: [1st: 3/4] [2nd: 2/2]"

6. **Performance Integration**:
   - Add perfmon timing around do_skore function
   - Log render times to validate <10ms target

### Priority 3 (Quality & Testing):
7. **Remove MVP Footer**:
   - Remove development footer from production display
   - Clean up any debug output

8. **Testing**:
   - Run valgrind memory leak tests
   - Test on major MUD clients
   - Verify all widths work correctly

---

## ⚠️ **CRITICAL IMPLEMENTATION NOTES**

### **Performance Constraints**
- Current do_score: ~3-5ms render time
- Target: <10ms for enhanced display (2x overhead acceptable)
- Must not impact pulse timing (100ms per pulse)
- Cache computed values for 1 pulse duration minimum

### **Memory Constraints**  
- Player data extension: 64 bytes maximum
- Display buffer: 8KB maximum (current: ~4KB)
- No dynamic allocation in render path
- Use stack allocation for temporary buffers

### **C90/C89 Compliance REQUIRED**
```c
/* CORRECT - Variables declared at block start */
ACMD(do_skore) {
  char buf[MAX_STRING_LENGTH];
  int i, width;
  struct char_data *vict;
  
  /* code here */
}

/* WRONG - C99 style, will NOT compile */
for (int i = 0; i < MAX_CLASSES; i++) { /* FAILS */
```

### **Safety Patterns**
- ALWAYS check IS_NPC() before PRF_FLAGGED()
- Validate pointers before dereferencing
- Use existing macros (GET_NAME, GET_LEVEL, etc.)
- Follow existing error handling patterns

---

## 📊 **PHASE 2: Visual Polish & Enhancement**
*Target: 2-3 weeks development time*

### 2.1 **Advanced Visual Elements**
- [ ] **Enhanced ASCII decorative elements** (8 hours)
  - Create class-specific decorative borders (simple, not animated)
  - Add race-specific visual indicators
  - Implement seasonal/special event themes
  - Create visual separators and accent elements
- [ ] **Improved color schemes** (6 hours)
  - Expand color palette with RGB support where available
  - Create high-contrast accessibility themes
  - Add colorblind-friendly alternatives
  - Implement dynamic color intensity based on character state

### 2.2 **Information Enhancement**
- [ ] **Contextual information display** (10 hours)
  - Show different information based on current activity (combat, exploration, social)
  - Highlight relevant stats based on character class and level
  - Add situational warnings and notifications
  - Implement smart information filtering
- [ ] **Enhanced status integration** (8 hours)
  - Display current spell effects with duration indicators
  - Show temporary buffs/debuffs with clear formatting
  - Add equipment condition and durability warnings
  - Integrate environmental status (underwater, flying, etc.)

### 2.3 **Advanced Customization**
- [ ] **Expanded preference system** (8 hours)
  - Add layout customization options for do_skore
  - Implement information density controls
  - Create preset themes for different play styles
  - Add import/export of skore configurations

**Phase 2 Success Criteria:**
- Enhanced visual appeal with class/race-specific elements
- Contextual information display working correctly
- Advanced customization options functional
- Performance maintained under increased complexity
- Accessibility features implemented and tested

---

## 🎭 **PHASE 3: Dynamic Content & Interactivity**
*Target: 3-4 weeks development time*

### 3.1 **Interactive Elements**
- [ ] **Expandable information sections** (12 hours)
  - Implement `skore detail <section>` commands for expanded views
  - Create detailed breakdowns for combat stats, skills, equipment
  - Add cross-referenced information linking
  - Implement historical progression tracking
- [ ] **Quick action integration** (10 hours)
  - Add equipment comparison shortcuts from score display
  - Integrate spell preparation reminders for casters
  - Create skill training progress indicators
  - Add quest objective tracking integration

### 3.2 **Advanced Status Display**
- [ ] **Real-time status integration** (8 hours)
  - Display current spell effects with precise duration timers
  - Show temporary ability modifications with sources
  - Add equipment durability warnings with repair suggestions
  - Integrate group/party status information
- [ ] **Achievement and progression showcase** (10 hours)
  - Display recent accomplishments and milestones
  - Show progress toward major character goals
  - Highlight titles, honors, and special recognitions
  - Create rare/legendary item callouts

### 3.3 **Social Integration**
- [ ] **Social elements display** (8 hours)
  - Integrate guild/clan information and status
  - Show friend/enemy status indicators
  - Display mentorship relationships
  - Add PvP rankings and honors where applicable

**Phase 3 Success Criteria:**
- Interactive elements respond quickly and intuitively
- Social integration enhances multiplayer experience
- Achievement system motivates continued engagement
- Advanced features don't overwhelm new players

---

## 🔧 **PHASE 4: Performance & Accessibility**
*Target: 2 weeks development time*

### 4.1 **Performance Optimization**
- [ ] **Caching and optimization** (8 hours)
  - Implement display component caching for faster rendering
  - Optimize data access patterns to reduce database queries
  - Create configurable detail levels for different performance needs
  - Add bandwidth-conscious options for slow connections
- [ ] **Scalability improvements** (6 hours)
  - Test performance with large numbers of concurrent users
  - Optimize memory usage for complex character displays
  - Implement lazy loading for optional display sections
  - Add performance monitoring and metrics

### 4.2 **Accessibility & Compatibility**
- [ ] **Accessibility features** (10 hours)
  - Create screen reader friendly text-only formats
  - Implement high contrast mode options
  - Add large text alternatives for vision-impaired users
  - Create colorblind-friendly palette alternatives
- [ ] **Client compatibility** (8 hours)
  - Test across major MUD clients (MUSHclient, Mudlet, TinTin++, etc.)
  - Ensure graceful degradation for basic telnet clients
  - Create mobile-friendly compact display versions
  - Add support for different character encodings

**Phase 4 Success Criteria:**
- Score display renders in <25ms for 99% of requests
- 100% compatibility with major screen readers
- Graceful degradation across all client types
- Accessibility compliance with relevant standards

---

## 📊 **PROJECT SUCCESS METRICS**

### **Quantitative Metrics**
- **Performance**: do_skore renders in <10ms for 99% of requests (perfmon verified)
- **Memory**: No increase in memory usage beyond 64 bytes per player
- **Stability**: Zero crashes or memory leaks introduced (valgrind clean)
- **Compatibility**: 100% functionality across all tested MUD clients
- **Backward Compatibility**: do_score remains completely unchanged

### **Qualitative Metrics**
- **Code Quality**: Follows all existing patterns and conventions
- **Maintainability**: Modular design allows easy addition of new sections
- **Documentation**: Complete function documentation and help files
- **Testing**: Unit tests for all new utility functions

---

## 🎨 **TECHNICAL SPECIFICATIONS**

### **Display Constraints**
- **Width Support**: 80, 100, 120, 160 character terminal widths
- **Color System**: Existing `\t` color codes only (no RGB initially)
- **Performance**: <10ms render time (2x current), <2KB stack usage
- **Compatibility**: Must work with screen readers and basic telnet

### **Data Requirements**
- **Storage**: Extend player_special_data_saved by exactly 64 bytes
- **Caching**: 1-pulse cache for expensive calculations (100ms)
- **Database**: Add skore_prefs BLOB column to player table
- **File Format**: Binary compatible with existing player files

### **Architecture Principles**
- **C90 Compliance**: No C99/C11 features, variables at block start
- **No Dynamic Allocation**: Stack buffers only in render path
- **Existing Patterns**: Use text_line(), draw_line(), ACMD() macro
- **Error Handling**: Follow existing log() and mudlog() patterns
- **Command Separation**: do_score and do_skore are completely independent

---

## 🛡️ **RISK MITIGATION STRATEGIES**

### **Technical Risks**
1. **Performance Degradation**
   - Mitigation: Continuous perfmon profiling, 10ms hard limit
   - Fallback: Players can always use original do_score
   
2. **Memory Corruption**
   - Mitigation: Valgrind testing after each phase
   - Prevention: No dynamic allocation, bounded string operations
   
3. **Client Compatibility Issues**
   - Mitigation: Test matrix for all major clients
   - Fallback: Original do_score always available

### **Implementation Risks**
1. **C99 Compilation Errors**
   - Prevention: Code review checklist for C90 compliance
   - Tool: Compile with -ansi -pedantic flags during development
   
2. **NPC Data Access Crashes**
   - Prevention: IS_NPC() checks before all PRF_FLAGGED calls
   - Pattern: Copy existing safety patterns from codebase

3. **Database Migration Failures**
   - Mitigation: Incremental schema changes
   - Fallback: Default values for missing preference data

---

## 🚀 **IMPLEMENTATION ROADMAP**

### **Phase 1 (Weeks 1-2): MVP Foundation**
Focus on creating do_skore as separate command with core enhancements.
- Deliverable: New do_skore command with enhanced display
- Success: <10ms render, do_score remains untouched

### **Phase 2 (Weeks 3-4): Visual Enhancement**
Add visual polish to do_skore while maintaining performance.
- Deliverable: Class-specific themes and color improvements
- Success: No performance regression, positive user feedback

### **Phase 3 (Weeks 5-6): Advanced Features**
Implement modular sections and customization for do_skore.
- Deliverable: Expandable sections, skore preference system
- Success: All features configurable, preferences persist

### **Phase 4 (Week 7): Polish & Optimization**
Final optimization and compatibility testing of do_skore.
- Deliverable: Production-ready do_skore command
- Success: Meets all performance and compatibility metrics

### **Post-Launch: Monitoring & Iteration**
- Monitor perfmon metrics for do_skore performance
- Collect player feedback on preference between commands
- Iterate on most-requested features for do_skore
- Consider eventual deprecation of do_score if adoption is high

This approach creates a new enhanced command while preserving the original, allowing players to choose their preferred display method.

---

## 📝 QUICK START GUIDE FOR NEW DEVELOPERS

### Where to Find the Code:
- **Main Implementation**: `act.informative.c:3846` - ACMD(do_skore)
- **Config Command**: `act.informative.c:4160` - ACMD(do_scoreconfig)
- **Command Registration**: `interpreter.c:785-786`
- **Preferences**: `structs.h:5135-5138` (player_special_data_saved)
- **PRF_FLAGS**: `structs.h:1353-1356`
- **Macros**: `utils.h:1302-1304` (GET_SCORE_*)

### Key Functions to Implement:
```c
/* In act.informative.c - Add these before do_skore */

static const char *get_health_color(struct char_data *ch, int current, int max) {
  int percentage = (max > 0) ? (current * 100 / max) : 0;
  
  if (percentage >= 75) return "\tg";      /* Green - Healthy */
  else if (percentage >= 50) return "\ty"; /* Yellow - Wounded */
  else if (percentage >= 25) return "\tO"; /* Orange - Badly wounded */
  else return "\tr";                       /* Red - Critical */
}

static const char *get_class_color(struct char_data *ch) {
  switch (GET_CLASS(ch)) {
    case CLASS_WIZARD:
    case CLASS_SORCERER:
    case CLASS_BARD:
      return "\tb"; /* Blue - Arcane */
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_PALADIN:
    case CLASS_RANGER:
      return "\tg"; /* Green - Divine */
    case CLASS_WARRIOR:
    case CLASS_BERSERKER:
    case CLASS_MONK:
      return "\tr"; /* Red - Martial */
    case CLASS_ROGUE:
      return "\tm"; /* Magenta - Skill */
    default:
      return "\tc"; /* Cyan - Other */
  }
}
```

### Testing Your Changes:
1. Compile: `make clean && make`
2. Test commands: `skore`, `scoreconfig`
3. Check memory: `valgrind --leak-check=full ../bin/circle`
4. Test all configurations and character types
5. **Run the complete test plan from `skore_system_documentation.md` (lines 165-432)**

### Related Documentation:
- **Help Text**: See `skore_system_documentation.md` for complete help documentation
- **Test Plan**: See `skore_system_documentation.md` for comprehensive testing procedures
- **Test Scenarios**: Includes 8 test categories with detailed steps and expected results

