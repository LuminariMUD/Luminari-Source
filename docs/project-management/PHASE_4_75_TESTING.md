# Phase 4.75 Testing Instructions

## Campaign-Aware Wilderness Materials Integration Testing

Phase 4.75 integrates the wilderness material system with the newcrafting system, making materials campaign-specific while maintaining backward compatibility.

## Pre-Testing Setup

1. **Verify Campaign Configuration**
   ```bash
   # Check which campaign is active
   grep -E "^#define CAMPAIGN_" src/campaign.h
   ```

2. **Compile the System**
   ```bash
   cd /home/jamie/Luminari-Source
   make clean && make
   ```

3. **Start the MUD**
   ```bash
   ./circle 4000
   ```

## Testing Phase 1: Campaign-Specific Material Names

### Test 1.1: Material Name Display
```
# Add materials and verify campaign-appropriate names appear
materialadmin add YourName crystal 0 5 10
materialadmin add YourName herbs 0 2 5
materialadmin add YourName wood 0 3 8
materials
```

**Expected Results:**
- **CAMPAIGN_DL**: Should show DragonLance-themed names (dragonmetal ore, vallenwood leaf, etc.)
- **CAMPAIGN_FR**: Should show Forgotten Realms names (mithril ore, silverleaf, etc.)
- **Default**: Should show standard LuminariMUD names (arcanite, marjoram, ironwood)

### Test 1.2: Cross-Campaign Consistency
```
# Test all material categories for consistent theming
materialadmin add YourName crystal 1 4 5    # Different crystal type
materialadmin add YourName minerals 2 5 3   # Legendary ore
materialadmin add YourName vegetation 0 2 10 # Common vegetation
materialadmin add YourName game 1 3 4       # Uncommon game
materials
```

## Testing Phase 2: Wilderness-to-Crafting Bridge

### Test 2.1: Material Group Mapping
```
# Add wilderness materials
materialadmin add YourName crystal 0 5 10  # Should map to hard metals
materialadmin add YourName herbs 0 2 15    # Should map to cloth
materialadmin add YourName wood 0 4 8      # Should map to wood group
materialadmin add YourName game 0 3 5      # Should map to animal hides

# Check crafting material availability
craft materials
```

**Expected Results:**
- Wilderness materials should appear in appropriate crafting groups
- Quality should affect metal categorization (rare+ → hard metals, common → soft metals)

### Test 2.2: Material Conversion
```
# Test conversion commands
materialadmin convert YourName crystal 0 5 5  # Convert 5 legendary crystals
materialadmin convert YourName herbs 0 2 10   # Convert 10 common herbs
craft materials   # Verify converted materials appear
```

### Test 2.3: Bridge Integration
```
# Test bridge command
materialadmin bridge YourName all  # Convert all wilderness to crafting
craft materials                    # Check all materials are now available
materials                          # Wilderness storage should be affected
```

## Testing Phase 3: Enhanced Admin Commands

### Test 3.1: Extended Materialadmin
```
# Test all new subcommands
materialadmin                           # Show help with new options
materialadmin list YourName crafting    # Show crafting-compatible materials
materialadmin convert YourName crystal 0 5 10  # Conversion command
materialadmin bridge YourName all       # Bridge all materials
```

### Test 3.2: Error Handling
```
# Test invalid conversion attempts
materialadmin convert YourName invalid 0 5 10   # Invalid category
materialadmin convert YourName crystal 99 5 10  # Invalid subtype
materialadmin bridge NonexistentPlayer all      # Invalid player
```

## Testing Phase 4: Crafting System Integration

### Test 4.1: Enhanced Craft Materials Command
```
# Start a crafting project
craft itemtype weapon
craft specifictype sword
craft variant standard
craft materials   # Should show both traditional and wilderness materials
```

### Test 4.2: Recipe Compatibility
```
# Test if wilderness materials work in existing recipes
craft materials add cloth 10     # Traditional material
craft materials add herbs 5      # Wilderness material (if converted to cloth group)
craft create                     # Should work with mixed materials
```

### Test 4.3: Wilderness Recipes (if implemented)
```
# Test new wilderness-specific recipes
craft recipes                    # Check for wilderness recipes
craft itemtype potion           # Try wilderness-specific items
craft materials                 # Verify wilderness materials are usable
```

## Testing Phase 5: Campaign-Specific Features

### Test 5.1: Campaign Compilation
```bash
# Test all three campaign configurations compile successfully

# Test Default LuminariMUD
echo "// Default campaign" > src/campaign.h
make clean && make

# Test DragonLance (if available)
echo "#define CAMPAIGN_DL" > src/campaign.h  
make clean && make

# Test Forgotten Realms (if available)
echo "#define CAMPAIGN_FR" > src/campaign.h
make clean && make
```

### Test 5.2: Campaign-Specific Material Themes
For each campaign configuration:
```
# Add same materials and verify theme-appropriate names
materialadmin add YourName crystal 0 5 10
materialadmin add YourName herbs 0 2 5  
materialadmin add YourName wood 0 3 8
materials
```

**Verify:**
- DragonLance: Krynn-themed materials
- Forgotten Realms: Faerun-themed materials  
- Default: Generic fantasy materials

## Testing Phase 6: Backward Compatibility

### Test 6.1: Existing Crafting Still Works
```
# Test traditional crafting workflow
craft itemtype weapon
craft specifictype sword  
craft variant standard
craft materials add hardmetals 10
craft materials add cloth 5
craft create
```

### Test 6.2: Legacy Material Commands
```
# Verify old crafting commands still function
craft materials show
craft materials add softmetals 5
craft materials remove cloth 2
```

### Test 6.3: Mixed Material Usage
```
# Test using both wilderness and traditional materials
materialadmin add YourName crystal 0 5 10  # Wilderness
craft materials add hardmetals 5           # Traditional
craft materials                            # Both should appear
```

## Error Cases to Test

### Invalid Campaign Configurations
```bash
# Test invalid campaign defines
echo "#define INVALID_CAMPAIGN" > src/campaign.h
make  # Should either ignore or error gracefully
```

### Material Conversion Errors
```
materialadmin convert YourName crystal 0 5 100  # More than player has
materialadmin bridge EmptyPlayer all           # Player with no materials
```

### Crafting Integration Errors
```
craft materials add invalidgroup 10  # Invalid material group
craft convert invalidmaterial 5      # Invalid conversion
```

## Performance Testing

### Large Material Storage
```
# Test system performance with many materials
for i in {0..7}; do
  for j in {1..5}; do
    materialadmin add YourName crystal $i $j 10
  done
done
materials  # Should handle 40 different materials efficiently
```

### Mass Conversion
```
# Add many wilderness materials
materialadmin add YourName herbs 0 2 50
materialadmin add YourName crystal 0 5 30
materialadmin add YourName wood 0 4 25

# Convert all at once
materialadmin bridge YourName all
```

## Success Criteria

### ✅ Phase 4.75 Complete When:

1. **Campaign Integration**: All three campaigns compile and show appropriate material names
2. **Bridge Functionality**: Wilderness materials successfully convert to crafting materials
3. **Admin Tools**: Enhanced materialadmin commands work correctly
4. **Crafting Integration**: Wilderness materials work in existing crafting system
5. **Backward Compatibility**: All existing crafting functionality preserved
6. **Error Handling**: Invalid inputs handled gracefully
7. **Performance**: System handles large material inventories efficiently

### Expected Outputs by Campaign:

**Default LuminariMUD:**
```
Your Wilderness Material Storage:
=====================================
  10 x legendary arcanite
   5 x common marjoram
   8 x rare ironwood
```

**CAMPAIGN_DL (DragonLance):**
```
Your Wilderness Material Storage:
=====================================
  10 x legendary dragonmetal ore  
   5 x common vallenwood leaf
   8 x rare silverwood
```

**CAMPAIGN_FR (Forgotten Realms):**
```
Your Wilderness Material Storage:
=====================================
  10 x legendary mithril ore
   5 x common silverleaf
   8 x rare shadowtop
```

## Integration Points

### Commands to Test:
- `materialadmin` (enhanced with new subcommands)
- `materials` (unchanged, but shows campaign names)
- `craft materials` (enhanced to show wilderness materials)
- `craft convert` (new conversion command)

### Files Modified:
- `src/resource_system.c` - Campaign-aware material names
- `src/act.wizard.c` - Enhanced materialadmin
- `src/crafting_new.c` - Wilderness integration
- `src/wilderness_crafting_bridge.c` - Bridge system

---
**Phase 4.75 Testing validates the seamless integration of wilderness materials with the existing crafting system while maintaining campaign-specific theming and backward compatibility.**
