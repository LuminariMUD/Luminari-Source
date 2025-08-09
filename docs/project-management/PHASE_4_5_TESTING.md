# Phase 4.5 Testing Instructions

## Quick Test of Material System

The Phase 4.5 material subtype system is now implemented and ready for testing.

### Testing Steps

1. **Start the MUD**
   ```bash
   cd /home/jamie/Luminari-Source
   ./circle 4000
   ```

2. **Create a test character** (if needed)
   - Connect and create a character
   - Grant yourself staff privileges if testing admin commands

3. **Test Material Storage Commands**

   **Add some materials (admin command):**
   ```
   # Add legendary arcanite crystal
   materialadmin add YourName crystal 0 5 10
   
   # Add rare mithril ore  
   materialadmin add YourName minerals 0 4 5
   
   # Add common marjoram herb
   materialadmin add YourName herbs 0 2 20
   
   # Add uncommon ironwood
   materialadmin add YourName wood 0 3 8
   ```

   **View stored materials:**
   ```
   materials
   ```

   **Expected Output:**
   ```
   Your Wilderness Material Storage:
   =====================================
     10 x legendary arcanite
      5 x rare mithril
     20 x common marjoram  
      8 x uncommon ironwood

   Storage: 4/100 slots used
   ```

### Material Categories and IDs for Testing

**Categories:**
- herbs = RESOURCE_HERBS  
- crystal = RESOURCE_CRYSTAL
- minerals = RESOURCE_MINERALS (ores)
- wood = RESOURCE_WOOD
- vegetation = RESOURCE_VEGETATION
- stone = RESOURCE_STONE
- game = RESOURCE_GAME

**Subtypes (0-7 for each category):**
- **Herbs:** 0=marjoram, 1=kingfoil, 2=starlily, 3=wolfsbane, etc.
- **Crystals:** 0=arcanite, 1=nethermote, 2=sunstone, 3=voidshards, etc.
- **Ores:** 0=mithril, 1=adamantine, 2=cold iron, 3=star steel, etc.
- **Wood:** 0=ironwood, 1=silverbirch, 2=shadowbark, 3=brightoak, etc.

**Quality Levels:** 1=poor, 2=common, 3=uncommon, 4=rare, 5=legendary

### Validation Tests

1. **Storage Limits:** Try adding materials until storage is full (100 slots)
2. **Invalid Data:** Try invalid categories, subtypes, or quality levels
3. **Quality Display:** Verify quality names display correctly
4. **Material Names:** Confirm specific materials show proper names

### Error Cases to Test

```bash
# Invalid category
materialadmin add YourName invalid 0 3 5

# Invalid subtype (should be 0-7)  
materialadmin add YourName herbs 99 3 5

# Invalid quality (should be 1-5)
materialadmin add YourName herbs 0 99 5

# Invalid quantity
materialadmin add YourName herbs 0 3 0
```

### Next Phase Readiness

Once Phase 4.5 testing is complete, the system is ready for:
- Phase 5 harvesting command implementation
- Region-specific material bonuses
- Skill-based harvesting success rates
- Integration with existing crafting system

---
**Ready for Phase 5 Implementation:** ✅ Material foundation complete
