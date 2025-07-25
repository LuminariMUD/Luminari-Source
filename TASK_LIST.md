# LuminariMUD Development Task List

This document tracks ongoing development tasks, bug fixes, and improvements for the LuminariMUD project. Tasks are organized by priority and category to help contributors identify areas where help is needed.

---

## CODER TASKS

---
FIX THIS FIRST & NOW!!!!!!!!!!!:

## Core Dump Analysis Summary for LuminariMUD

### The Crash
- **Signal**: SIGABRT (Signal 6) - The program called `abort()` due to a fatal error
- **Location**: Inside `free()` in the C library, called from `free_tokens()` at mysql.c:243
- **Root Cause**: Memory corruption - attempting to free invalid/garbage memory pointers

### Call Stack (Bottom to Top)
1. MUD boots up (`main()` → `init_game()` → `boot_db()`)
2. House system initialization (`House_boot()`)
3. Loading house #24828 (`House_load()`)
4. Parsing objects from database (`objsave_parse_objects_db()`)
5. Tokenizing object data with `tokenize()` function
6. Attempting to free tokens with `free_tokens()` - **CRASH HERE**

### The Bug

The `tokenize()` function in mysql.c has a critical bug in how it NULL-terminates the token array:

```c
// Current buggy code (mysql.c lines 215-226):
while (1)
{
    if (count >= capacity)
        result = realloc(result, (capacity *= 2) * sizeof(*result));
    
    result[count++] = tok ? strdup(tok) : tok;  // <-- Adds NULL but increments count
    
    if (!tok)
        break;
    
    tok = strtok(NULL, delim);
}
```

**The Problem**: 
- When `strtok` returns NULL (no more tokens), the code stores NULL at `result[count]` and increments `count`
- Then it breaks from the loop
- The array now has NULL at position `count-1`, but position `count` and beyond contain uninitialized garbage memory

**Why It Crashes**:
- `free_tokens()` expects a NULL-terminated array
- It loops through with `for (it = tokens; *it; ++it)` looking for NULL
- Because the NULL is not at the end of the allocated data, it continues past NULL into garbage memory
- When it tries to `free()` these garbage pointers (like `0x24055380 " R\005$"`), the program crashes

### Evidence from GDB
- `lines` array after tokenize: Valid pointer addresses followed by garbage
- Token values show corrupted data: `" U\005$"`, `"\200R\005$"`, etc. instead of valid strings
- The serialized object data itself was valid (started with `"#3183\nLoc : -1\nFlag:..."`)

### The Fix

Replace the tokenize loop with proper NULL termination:

```c
while (tok)
{
    if (count >= capacity)
        result = realloc(result, (capacity *= 2) * sizeof(*result));
    
    result[count++] = strdup(tok);
    tok = strtok(NULL, delim);
}

// Ensure space for NULL terminator and add it
if (count >= capacity)
    result = realloc(result, (capacity + 1) * sizeof(*result));
result[count] = NULL;
```

This ensures the array is properly NULL-terminated without garbage values after the NULL.

### Additional Notes
- This bug only manifests when loading house data (not player data)
- The crash happens during boot when loading house #24828
- The bug is deterministic - it will crash every time at the same place until fixed

---


### Memory Leaks and Issues (From Valgrind Analysis - July 24, 2025)

Log file for reference if needed: valgrind_20250724_221634.md

#### Critical Memory Leaks (460KB total definitely lost)
| ☐ | Location | Issue | Size | Priority |
|---|----------|-------|------|----------|
| ☑ | hlquest.c:768, 769 | clear_hlquest() strdup without free | 113KB total | CRITICAL |
| ☑ | hlquest.c:925 | boot_the_quests() calloc without free | 15.7KB | HIGH |
| ✅ | mysql.c:211, 218 | tokenize() malloc/realloc leaks | 318KB total | CRITICAL |
| ☐ | db.c:4021 | read_object() object creation leaks | ~7KB | MEDIUM |
| ☐ | db.c:4004 | read_object() larger object leaks | 2.4KB | MEDIUM |
| ☐ | dg_variables.c:65 | Script variable memory not freed | Multiple small | LOW |
| ☐ | handler.c:134 | isname() strdup temporary strings not freed | Multiple 6-7 bytes | MEDIUM |
| ☐ | spell_parser.c:3055 | spello() spell name strings not freed | 38-39 bytes each | MEDIUM |
| ☐ | dg_scripts.c:2990,2996 | script_driver() temp data during triggers | Various | MEDIUM |
| ☐ | db.c:4311,4385 | Object strings during zone resets not freed | Various | MEDIUM |

#### Uninitialized Values
| ☐ | Location | Issue | Errors | Priority |
|---|----------|-------|--------|----------|
| ☐ | db.c:4937, 4939 | fread_clean_string() uninitialized stack vars | 60 errors | MEDIUM |

#### Use-After-Free
| ☐ | Location | Issue | Details | Priority |
|---|----------|-------|---------|----------|
| ☐ | lists.c/mud_event.c | Accessing freed event memory | 8 bytes inside freed block | HIGH |
| ✅ | comm.c:2951 | close_socket() accessing freed descriptor | Segfault during cleanup | CRITICAL |

**Details**:
- **hlquest.c**: Major leaks in quest system - clear_hlquest() allocates strings with strdup (lines 768, 769) but never frees them. Called from boot_the_quests() at lines 901 and 914. Total: 9,840 + 10,496 + 44,970 + 47,952 = 113,258 bytes lost.
- **mysql.c**: tokenize() function has severe memory leaks from malloc (line 211) and realloc (line 218) operations. Affects load_paths() and other database operations. Total: ~318KB lost.
- **db.c:4937**: fread_clean_string() has uninitialized stack variables causing 60 conditional jump errors. Affects IBT file loading.
- **handler.c:134**: isname() creates temporary strings with strdup during name parsing that are never freed. Common in movement/equipment commands.
- **spell_parser.c:3055**: spello() allocates spell name strings that persist for the entire runtime without cleanup.
- **dg_scripts.c**: script_driver() at lines 2990/2996 leaks temporary data during trigger execution, especially through load_mtrigger and reset_wtrigger.
- **Zone resets**: Objects loaded during zone resets (db.c:4311,4385) allocate strings that aren't tracked for cleanup.
- **Use-after-free**: Event system accessing memory 8 bytes inside a 24-byte block that was freed by free_mud_event().
- **close_socket**: Critical use-after-free at comm.c:2951 accessing descriptor data after freeing, causing segfaults.

Log file for reference if needed: valgrind_20250724_221634.md

---

## 🏗️ BUILDER TASKS (Fixable In-Game with OLC)

### Missing Triggers (12 total)

| ☐ | Trigger | Type | Affected Entity | Zone |
|---|---------|------|-----------------|------|
| ☐ | #2315 | Room | (room:-1) | Unknown |
| ☐ | #2314 | Mob | the Dark Knight | Unknown |
| ☐ | #2316 | Mob | the Dark Knight | Unknown |
| ☐ | #2310 | Mob | a giant mother spider | Unknown |
| ☐ | #2313 | Mob | a bat-like creature | Unknown |
| ☐ | #2308 | Obj | a jet black pearl | Unknown |
| ☐ | #2311 | Obj | a large stone chest | Unknown |
| ☐ | #2317 | Obj | Helm of Brilliance | Unknown |

### Zone File Corrections

| ☐ | Zone | Issue | Fix Required |
|---|------|-------|--------------|
| ☐ | #158 | Invalid object vnum 15802 in 'O' command (line 8) | Update to valid vnum |
| ☐ | #1481 | Invalid equipment position 148181 for High Priest of Grummsh | Use valid pos (0-21) |

### Quest Assignments (19 quests need questmasters)

| ☐ | Quest Vnum | Priority | Notes |
|---|------------|----------|-------|
| ☐ | #0 | HIGH | Quest 0 usually important |
| ☐ | #2011 | MEDIUM | |
| ☐ | #20309 | MEDIUM | |
| ☐ | #20315-20325 | LOW | Series of 11 quests |
| ☐ | #102412-102415 | LOW | Series of 4 quests |
| ☐ | #128102 | LOW | |

### Mob Script Fixes

| ☐ | Mob | Vnum | Issue |
|---|-----|------|-------|
| ☐ | Brother Spire | #200103 | Calling non-existing mob function |
| ☐ | Jakur the tanner | #125913 | Calling non-existing mob function |
| ☐ | Adoril | #21605 | Calling non-existing mob function |

### Missing Objects

| ☐ | Object Vnum | References | Action |
|---|-------------|------------|---------|
| ☐ | #19216 | 2 references during boot | Create object or remove refs |
| ☐ | #40252 | award_misc_magic_item() | Create object or fix award function |

---
