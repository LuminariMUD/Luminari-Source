# CHANGELOG

## [2025-07-23] - Bug Fix: Multi-word Search Functions

### Summary
Fixed a critical bug in multiple search functions that prevented proper matching of multi-word names. The bug affected commands like `feat info dragon mount` which would incorrectly fail to find valid entries.

### Problem Description
The search functions were using word-by-word matching but only verified that all search words were consumed, not that all target name words were also consumed. This caused searches like "dragon" to incorrectly match both "dragon mount" and "dragon link", leading to failed searches when they should have succeeded.

### Files Modified

#### 1. `feats.c`
- **Function**: `find_feat_num()` 
- **Line**: 8347
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`
- **Note**: This fix was already present in the codebase

#### 2. `evolutions.c`
- **Function**: `find_evolution_num()`
- **Line**: 1540 (originally reported as 1251)
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`

#### 3. `spell_parser.c`
- **Function**: `find_skill_num()`
- **Line**: 409 (originally reported as 170)
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`
- **Function**: `find_ability_num()`
- **Line**: 442 (originally reported as 237)
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`

#### 4. `alchemy.c`
- **Function**: `find_discovery_num()`
- **Line**: 3185 (originally reported as 889)
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`
- **Function**: `find_grand_discovery_num()`
- **Line**: 3272 (originally reported as 1022)
- **Change**: Modified condition from `if (ok && !*first2)` to `if (ok && !*first2 && !*first)`

### Technical Details
The fix ensures that both the search term AND the target name are fully consumed before declaring a match. The additional `&& !*first` check prevents partial matches where the search term is a subset of the target name.

### Testing Notes
After this fix:
- `feat info dragon mount` will correctly find FEAT_DRAGON_BOND
- `feat info dragon link` will correctly find FEAT_DRAGON_LINK
- `feat info dragon` will NOT match multi-word feats (this is the intended behavior)
- Partial matches like `feat info drag mount` will still work correctly
- The same logic applies to evolution, skill, ability, and discovery searches

### Build Setup Notes
For developers setting up local builds:

1. **Required Header Files**: The following files must be created locally (they are in .gitignore):
   - `campaign.h` - Campaign-specific configuration
   - `vnums.h` - Copy from `vnums.example.h`
   - `mud_options.h` - Copy from `mud_options.example.h`

2. **Creating Required Files**:
   ```bash
   # Create campaign.h with minimal content
   echo '/* Campaign-specific configuration */' > campaign.h
   
   # Copy example files
   cp vnums.example.h vnums.h
   cp mud_options.example.h mud_options.h
   ```

3. **Compilation**: All modified files compile successfully with the standard project flags.

### Related Issues
- Original issue: "feat info dragon mount" command not working
- Root cause: Word-matching algorithm bug in search functions
- Impact: Affected feat, evolution, skill, ability, and discovery information commands

### Backwards Compatibility
This fix maintains backwards compatibility. All previously working searches will continue to work. The fix only prevents incorrect matches that were bugs, not features.