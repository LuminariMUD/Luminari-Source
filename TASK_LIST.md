# Task List for LuminariMUD Code Fixes

## Issue #1: "feat info dragon mount" Command Not Working

### Problem Description
The command `feat info dragon mount` returns "Could not find that feat" even though:
- The feat "dragon mount" exists and is visible in `feats all`
- Similar commands like `feat info dragon link` work correctly
- The feat is properly defined in the code as FEAT_DRAGON_BOND with name "dragon mount"

### Root Cause Analysis
The `find_feat_num` function in `feats.c` has a word-matching logic bug:

1. **The Bug:**
   - The function uses word-by-word matching to allow partial matches
   - When all words in the search term are matched, it checks `ok && !*first2`
   - This means "all search words consumed", but doesn't verify all feat name words were consumed
   - Example: Searching "dragon" matches both "dragon mount" and "dragon link" incorrectly
   - The search should only match if BOTH the search term AND feat name are fully consumed

2. **Code Location:** `feats.c`, function `find_feat_num` (line 8347)

### Proposed Fix

#### Simple One-Line Fix (Recommended)
The bug can be fixed by ensuring both the search term AND the feat name are fully consumed. Change line 8347 in `find_feat_num` from:

```c
if (ok && !*first2)
```

to:

```c
if (ok && !*first2 && !*first)
```

This ensures that:
- All words in the search term have been matched (`!*first2`)
- All words in the feat name have been matched (`!*first`)
- Prevents partial matches like "dragon" matching "dragon mount"

#### Affected Functions
The same bug exists in multiple search functions throughout the codebase. All need the identical fix:

1. `find_feat_num()` in feats.c (line 8347)
2. `find_evolution_num()` in evolutions.c (line 1251)
3. `find_skill_num()` in spell_parser.c (line 170)
4. `find_ability_num()` in spell_parser.c (line 237)
5. `find_discovery_num()` in alchemy.c (line 889)
6. `find_grand_discovery_num()` in alchemy.c (line 1022)

### Testing Requirements

1. **Test Cases for feat info:**
   - `feat info dragon mount` → Should find FEAT_DRAGON_BOND
   - `feat info dragon link` → Should find FEAT_DRAGON_LINK  
   - `feat info dragon` → Should NOT match multi-word feats (bug fix prevents this)
   - `feat info drag mount` → Should find dragon mount (partial match)
   - `feat info dragon mou` → Should find dragon mount (abbreviated)
   - `feat info power attack` → Should still work for non-conflicting feats

2. **Test Similar Commands:**
   - `evo info` - Test evolution searches
   - `skill info` - Test skill searches  
   - `abil info` - Test ability searches
   - Discovery searches (if command exists)

3. **Edge Cases:**
   - Searching for partial names should still work when unambiguous
   - Full exact matches should work
   - Single-word searches should only match single-word items

### Temporary Workarounds (Until Fix is Implemented)

Users can work around this issue by:

1. **Use more specific abbreviations:**
   - `feat info dragon mou` (abbreviated second word)
   - `feat info drago mount` (abbreviated first word)

2. **Use quotes if supported:**
   - `feat info "dragon mount"`

3. **Find the feat number:**
   - Use `feats all` to find the feat
   - Note its position/number if displayed
   - Use the number instead of the name

### Implementation Notes

- The fix is a simple one-line change to the matching logic
- No performance impact - same algorithm, just corrected logic
- Maintains backward compatibility for valid searches
- Prevents incorrect partial matches that were bugs, not features
- The same fix pattern applies to all 6 affected functions

### Related Files
- `feats.c` - Contains `find_feat_num` function
- `evolutions.c` - Contains `find_evolution_num` function
- `spell_parser.c` - Contains `find_skill_num` and `find_ability_num` functions
- `alchemy.c` - Contains `find_discovery_num` and `find_grand_discovery_num` functions

### Priority
High - This affects usability of multiple core game features (feats, evolutions, skills, abilities, discoveries)

### Additional Notes
- The bug allows searches like "dragon" to incorrectly match "dragon mount" or "dragon link"
- After the fix, searching "dragon" will only match items named exactly "dragon"
- To search for dragon-related items, users must provide more specific terms like "dragon m" or "drag mount"