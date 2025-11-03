# Duplicate talents.c Compilation Error Fix

## Date: October 15, 2025

## Issue
Compilation failed with multiple definition errors for all functions in `talents.c`:

```
/usr/bin/ld: src/talents.o: in function `init_talents':
/home/krynn/code/src/talents.c:42: multiple definition of `init_talents'
/usr/bin/ld: src/talents.o: in function `get_talent_rank':
/home/krynn/code/src/talents.c:76: multiple definition of `get_talent_rank'
[... and many more similar errors ...]
collect2: error: ld returned 1 exit status
```

## Root Cause
The file `src/talents.c` was listed **twice** in `Makefile.am`:
- Once at line 95
- Again at line 194

This caused the build system to compile `talents.c` twice, creating duplicate object files that the linker couldn't resolve.

## Solution
Removed the duplicate entry from `Makefile.am`:

### File: `Makefile.am`

**Before:**
```makefile
# Line 95
src/talents.c \

# Line 194
src/talents.c \  ← DUPLICATE!
```

**After:**
```makefile
# Line 95
src/talents.c \

# Line 194 - removed duplicate
```

Regenerated Makefile with `automake` command.

## Files Modified
- `Makefile.am` - Removed duplicate `src/talents.c` entry at line 194

## Compilation Status
✅ **Successfully compiled** with no errors or warnings

## How This Happened
Likely the file was added twice during development:
1. Initially added in the correct alphabetical position (around line 95)
2. Later accidentally added again in a different section (around line 194)

## Prevention
When adding new source files to `Makefile.am`:
1. Check if the file already exists in the list
2. Use grep to search: `grep "filename.c" Makefile.am`
3. Keep source files in alphabetical order within their section

---
**Status:** Fixed  
**Impact:** Build system integrity restored
