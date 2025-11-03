# SKORE Phase 2-4 Coding Tasks

**Phase 1 COMPLETE** - See CHANGELOG.md for details
**Phase 2.1, 2.2, 2.3, 2.4 & 2.5 COMPLETE** - See CHANGELOG.md for details

## Technical Requirements
- C90 compliance (no C99)
- <10ms render time
- 64 byte max data extension
- Check IS_NPC() before PRF_FLAGGED()

---

### 2.4 Context Detection ✅ COMPLETE
- [ ] Add context override to scoreconfig (future enhancement)

### 2.5 Active Effects Display ✅ COMPLETE
- [ ] Group by spell schools/types (future enhancement)


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

