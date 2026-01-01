# Knight of Solamnia Implementation - Manual Steps Required

## Status

✅ **COMPLETED:**
- Updated `src/structs.h` - Class number definitions
- Updated `src/constants.c` - Class names array
- Updated `src/utils.h` - All macros and class checks
- Updated `src/utils.c` - Divine level and channel energy calculations
- Created `src/knight_conversion.c` - Character conversion system
- Created `src/knight_solamnia_class_def.txt` - New class definition ready to insert

⏳ **REQUIRES MANUAL EDITING:**
- `src/class.c` - Delete old classes, insert new combined class (size limitations prevent automated editing)
- `src/interpreter.c` - Add `do_knightconvert` command to command table
- Build system - Add `knight_conversion.c` to CMakeLists.txt/Makefile

## Step 1: Delete Old Knight Classes from class.c

Open `/home/krynn/code/src/class.c` in your editor and delete lines **7312 through 7710** (inclusive).

These lines contain the three old knight class definitions:
- CLASS_KNIGHT_OF_THE_CROWN (lines 7312-7494)
- CLASS_KNIGHT_OF_THE_SWORD (lines 7496-7636)
- CLASS_KNIGHT_OF_THE_ROSE (lines 7639-7710)

**Before deletion, line 7312 starts with:**
```c
  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_KNIGHT_OF_THE_CROWN,
```

**After deletion, line 7711 should now become adjacent to line 7311:**
```c
  class_prereq_feat(CLASS_STALWART_DEFENDER, FEAT_ARMOR_PROFICIENCY_LIGHT, 1);
  /****************************************************************************/


  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_KNIGHT_OF_THE_LILY,
```

## Step 2: Insert New Knight of Solamnia Class

After deleting the old classes, insert the new combined class definition at line 7312 (where the old Crown class was).

The complete new class definition is in:
**`/home/krynn/code/src/knight_solamnia_class_def.txt`**

Copy the ENTIRE contents of that file and paste it at line 7312 in class.c (right after the Stalwart Defender prerequisites and the `/**************/` separator line, before the Knight of the Lily class).

The result should look like:
```c
  class_prereq_feat(CLASS_STALWART_DEFENDER, FEAT_ARMOR_PROFICIENCY_LIGHT, 1);
  /****************************************************************************/

  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_KNIGHT_OF_SOLAMNIA,
  ... [NEW CLASS DEFINITION] ...
  class_prereq_align(CLASS_KNIGHT_OF_SOLAMNIA, LAWFUL_GOOD);
  /****************************************************************************/


  /****************************************************************************/
  /*     class-number               name      abrv   clr-abrv     menu-name*/
  classo(CLASS_KNIGHT_OF_THE_LILY,
```

## Step 3: Add Conversion Command to Command Table

Open `/home/krynn/code/src/interpreter.c` and find the command table. Add the knight conversion command:

```c
  ACMD(do_knightconvert);  /* Add to declarations at top of file */
```

Then in the command table array:
```c
  { "knightconvert", POS_DEAD, do_knightconvert, LVL_IMPL, 0, 0 },
```

## Step 4: Verify Divine Spell Progression

The Knight of Solamnia uses **spell progression advancement** (like Sacred Fist or Mystic Theurge), NOT its own spell slots.

**How it works:**
- Levels 1-5: No spell progression (pure martial Crown training)
- Levels 6-20: Each level adds +1 to your existing divine caster level
- Example: Cleric 5 / Knight of Solamnia 10 = 15th level divine caster
  * Knight level 1-5 = +0 to divine level
  * Knight level 6 = +1 to divine level
  * Knight level 7 = +2 to divine level
  * ... Knight level 20 = +15 to divine level

**Already completed:**
✅ Updated `/home/krynn/code/src/utils.c`:
  - `compute_divine_level()` - Added Knight of Solamnia with level -5 offset
  - `compute_channel_energy_level()` - Added Knight of Solamnia with level -5 offset

**No additional spell slot arrays needed!** The class advances your existing divine spellcasting class (Cleric, Druid, etc.).

## Step 5: Search for Remaining References

Search the entire codebase for references to the old classes:
```bash
cd /home/krynn/code
grep -r "CLASS_KNIGHT_OF_THE_CROWN" src/
grep -r "CLASS_KNIGHT_OF_THE_SWORD" src/
grep -r "CLASS_KNIGHT_OF_THE_ROSE" src/
grep -r "KNIGHT_OF_THE_CROWN" src/
grep -r "KNIGHT_OF_THE_SWORD" src/
grep -r "KNIGHT_OF_THE_ROSE" src/
```

Update any remaining references (in mob scripts, special abilities, dialogue, etc.) to use the new `CLASS_KNIGHT_OF_SOLAMNIA`.

## Step 6: Add Conversion Function to Build System

Open `/home/krynn/code/CMakeLists.txt` or `/home/krynn/code/Makefile.am` and add:
```
src/knight_conversion.c
```
to the source file list.

## Step 7: Test Compilation

```bash
cd /home/krynn/code
make clean
make
```

Fix any compilation errors that appear.

## Step 8: Test In-Game

1. Create a test character
2. Try to take levels in Knight of Solamnia
3. Verify prerequisites are checked correctly
4. Verify feats are granted at the right levels
5. Verify divine spellcasting starts at level 6
6. Test the `knightconvert` command on a character with old knight levels

## Additional Notes

### Divine Spell Progression

The Knight of Solamnia follows cleric spell progression starting at level 6:
- Level 6: 1st level spells
- Level 7: 1st level spells
- Level 8: 2nd level spells
- ... and so on

This is similar to how Paladin gets spells at level 4 instead of level 1.

### Character Conversion

Players with existing Crown/Sword/Rose levels will need to use the `knightconvert` admin command. Admins should:
1. Note the player's current Crown/Sword/Rose levels
2. Use: `knightconvert <playername> <crown> <sword> <rose>`
3. Have the player relog to see changes

### Feat Considerations

The new combined class grants different feats at different levels than the old three classes. Converted characters may end up with different abilities. This is intentional as part of the class consolidation.

## Files Modified

- ✅ `/home/krynn/code/src/structs.h` - Updated
- ✅ `/home/krynn/code/src/constants.c` - Updated
- ✅ `/home/krynn/code/src/utils.h` - Updated
- ✅ `/home/krynn/code/src/knight_conversion.c` - Created
- ✅ `/home/krynn/code/src/knight_solamnia_class_def.txt` - Created (for reference)
- ⏳ `/home/krynn/code/src/class.c` - **NEEDS MANUAL EDITING**
- ⏳ `/home/krynn/code/src/interpreter.c` - **NEEDS knightconvert command added**
- ⏳ Spell progression code - **NEEDS level 6 check added**
- ⏳ Build system - **NEEDS knight_conversion.c added to sources**

## Design Document

See `/home/krynn/code/docs/KNIGHTS_OF_SOLAMNIA_COMBINED.md` for the complete class design specification.
