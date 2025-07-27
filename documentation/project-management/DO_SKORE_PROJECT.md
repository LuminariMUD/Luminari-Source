# SKORE Phase 2-4 Coding Tasks

**Phase 1 COMPLETE** - See CHANGELOG.md for details
**Phase 2.1, 2.2, 2.3, 2.4 & 2.5 COMPLETE** - See CHANGELOG.md for details

## Technical Requirements
- C90 compliance (no C99)
- <10ms render time
- 64 byte max data extension
- Check IS_NPC() before PRF_FLAGGED()

---

## PHASE 2: Visual Polish

### 2.1 Class Borders ✅ COMPLETE
- [x] Create `get_class_border_style()` in act.informative.c
- [x] Add border arrays: warrior_border[], mage_border[], etc.
- [x] Modify do_skore header section to use borders
- [x] Add PRF_SCORE_BORDERS flag to structs.h
- [x] Add `scoreconfig borders on/off` to do_scoreconfig

### 2.2 Race Symbols ✅ COMPLETE
- [x] Add `get_race_symbol()` function returning char*
- [x] Define race_symbols[] array in act.informative.c
- [x] Insert symbol in identity panel display
- [x] Add PRF_SCORE_RACE_SYMBOLS flag
- [x] Add `scoreconfig symbols on/off` to do_scoreconfig

### 2.3 New Color Themes ✅ COMPLETE
- [x] Add SCORE_THEME_HIGHCONTRAST (3) to enums
- [x] Add SCORE_THEME_DARK (4) to enums
- [x] Add SCORE_THEME_COLORBLIND (5) to enums
- [x] Implement theme color mappings in get_*_color() functions
- [x] Extend scoreconfig theme command validation

### 2.4 Context Detection ✅ COMPLETE
- [x] Add `get_display_context()` returning enum (CONTEXT_COMBAT, etc)
- [x] Check FIGHTING(ch) for combat mode
- [x] Reorder sections based on context in do_skore
- [ ] Add context override to scoreconfig (future enhancement)

### 2.5 Active Effects Display ✅ COMPLETE
- [x] Create `display_active_effects()` function
- [x] Loop ch->affected list
- [ ] Group by spell schools/types (future enhancement)
- [x] Calculate ticks to time conversion
- [x] Use progress bar for duration display
- [x] Add to magic section or new section

### 2.6 Layout Templates
- [ ] Add score_layout_template to player_special_data_saved
- [ ] Define LAYOUT_COMBAT, LAYOUT_ROLEPLAY, LAYOUT_EXPLORER, LAYOUT_CASTER
- [ ] Create section_order[] arrays for each template
- [ ] Add `scoreconfig template <name>` command
- [ ] Modify do_skore to respect section ordering

### 2.7 Section Ordering
- [ ] Add score_section_order[8] to player_special_data_saved
- [ ] Define SECTION_IDENTITY, SECTION_VITALS, etc enums
- [ ] Implement `scoreconfig order <section> <position>`
- [ ] Validate no duplicate positions
- [ ] Apply ordering in do_skore main display loop

---

## PHASE 3: Extended Features

### 3.1 Detailed Views
- [ ] Parse "skore detail <section>" in do_skore
- [ ] Create display_detailed_combat() function
- [ ] Create display_detailed_magic() function  
- [ ] Create display_detailed_stats() function
- [ ] Add skill breakdowns to detailed views

### 3.2 Group Display
- [ ] Add group member summary to do_skore
- [ ] Show group HP/Move percentages
- [ ] Display group buff indicators
- [ ] Add PRF_SCORE_SHOW_GROUP flag

### 3.3 Achievement Display
- [ ] Add achievement counters to character
- [ ] Create achievement summary panel
- [ ] Track titles earned count
- [ ] Display in wealth section

### 3.4 Clan/Guild Integration
- [ ] Check GET_CLAN(ch) for membership
- [ ] Display clan name and rank
- [ ] Show clan online count
- [ ] Add to identity or social section

---

## PHASE 4: Optimization

### 4.1 Performance
- [ ] Add score_cache struct to char_data
- [ ] Cache computed values for 1 pulse (100ms)
- [ ] Add cache validation timestamp
- [ ] Implement lazy section loading
- [ ] Add PRF_SCORE_NOCACHE for debugging

### 4.2 Screen Reader Mode
- [ ] Add PRF_SCORE_SCREENREADER flag
- [ ] Strip all color codes when enabled
- [ ] Use plain text progress indicators: [HP: 50/100]
- [ ] Add section headers as text: "=== COMBAT ==="
- [ ] Remove ASCII art when enabled

---

## Implementation Locations
- Main code: act.informative.c
- Preferences: structs.h (player_special_data_saved)
- Flags: structs.h (PRF_FLAGS)
- Registration: interpreter.c

