# Board Command Registration Fix

## Date: October 15, 2025

## Issue
The "read board" command was not recognizing boards in rooms because the MySQL board command handlers (`do_read_board`, `do_write_board`, `do_remove_board`) were not properly registered in the command interpreter table.

## Root Cause
The command table in `src/interpreter.c` was still pointing to the old command handlers:
- `"read"` → `do_look` (with SCMD_READ subcmd)
- `"write"` → `do_write`
- `"remove"` → `do_remove`

These old handlers don't have the MySQL board integration logic, so they couldn't detect or interact with board objects in the room.

## Solution
Updated the command table in `src/interpreter.c` to use the MySQL board-aware handlers:

### Changes Made

**1. READ command (line ~761):**
```c
// BEFORE:
{"read", "rea", POS_RECLINING, do_look, 0, SCMD_READ, FALSE, ACTION_NONE, {0, 0}, NULL},

// AFTER:
{"read", "rea", POS_RECLINING, do_read_board, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
```

**2. REMOVE command (line ~773):**
```c
// BEFORE:
{"remove", "rem", POS_RESTING, do_remove, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},

// AFTER:
{"remove", "rem", POS_RESTING, do_remove_board, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
```

**3. WRITE command (line ~1039):**
```c
// BEFORE:
{"write", "write", POS_STANDING, do_write, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},

// AFTER:
{"write", "write", POS_STANDING, do_write_board, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
```

## How It Works Now

The MySQL board command handlers implement a fallback pattern:

### do_read_board()
1. Checks if there's a board object in the room (or inventory/worn)
2. If YES and argument is "board" → Shows board list
3. If YES and argument is a number → Shows that post
4. If NO board found → Falls back to `do_look()` (original read behavior)

### do_write_board()
1. Checks if there's a board object in the room
2. If YES → Starts board post editor
3. If NO → Falls back to `do_write()` (original write behavior for paper/etc)

### do_remove_board()
1. Checks if there's a board object in the room
2. If YES and argument is a number → Deletes that board post
3. If NO or argument isn't a number → Falls back to `do_remove()` (original remove equipment)

## Benefits

1. **Seamless Integration**: Commands work for both boards AND their original purposes
2. **No Breaking Changes**: Old functionality (reading books, writing on paper, removing equipment) still works
3. **Automatic Detection**: Players don't need special commands - just "read board" when a board is present

## Commands Now Available

When in a room with a board object:
- `read board` - List all posts on the board
- `read <number>` - Read a specific post
- `write` - Create a new post
- `remove <number>` - Delete a post (if you have permission)
- `board` or `note` - Alternative way to list posts
- `board help` - Show help information
- `breply <number>` - Reply to a post with quoted text

When NOT in a room with a board:
- `read <object>` - Works normally (reads books, signs, etc.)
- `write <object>` - Works normally (writes on paper, etc.)
- `remove <equipment>` - Works normally (removes worn gear)

## Testing Recommendations

1. **With board present:**
   - `read board` - Should show board list
   - `read 1` - Should show post #1
   - `write` - Should start board post editor
   - `remove 1` - Should attempt to delete post #1

2. **Without board present:**
   - `read book` - Should work normally
   - `write paper` - Should work normally  
   - `remove helmet` - Should work normally

3. **Edge cases:**
   - Board in inventory - Should still work
   - Board worn as equipment - Should still work
   - Multiple board objects - Uses first one found

## Files Modified
- `src/interpreter.c` - Updated command table entries for read, write, and remove

## Compilation Status
✅ **Successfully compiled** with no errors or new warnings

## Author
GitHub Copilot Assistant
