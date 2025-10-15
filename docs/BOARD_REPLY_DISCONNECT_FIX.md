# Board Reply Disconnect Bug Fix

## Date: October 15, 2025

## Issue
When using the `breply` command to reply to a board post:
1. User enters reply title
2. User writes reply body
3. User types `/s` to save
4. **MUD hangs briefly and then disconnects the user**
5. Reply is not saved to the database
6. MUD does not crash, but connection is lost

## Root Cause Analysis

### The Problem
The `mysql_board_handle_reply_title()` function was setting the wrong connection state for the string editor:

**Incorrect Code:**
```c
STATE(d) = CON_BOARD_BODY;  // Wrong state!
```

### Why This Caused Disconnection

The string editor cleanup system in `modify.c` uses a callback mechanism based on connection states:

```c
static struct {
    int mode;
    void (*func)(struct descriptor_data *d, int action);
} cleanup_modes[] = {
    // ... other states ...
    {CON_BOARD_POST, board_post_string_cleanup},
    {CON_BOARD_POST_ABORT, board_post_string_cleanup},
    // CON_BOARD_BODY was NOT registered!
    {-1, NULL}
};
```

**What happened:**
1. User typed `/s` to save
2. String editor looked for cleanup handler for `CON_BOARD_BODY`
3. **No handler found** - string editor didn't know what to do
4. Resources weren't properly cleaned up
5. Connection state became inconsistent
6. Descriptor eventually got cleaned up, causing disconnect
7. Post was never saved because cleanup callback was never called

### Connection State Flow

#### Regular Post (Working):
```
CON_BOARD_TITLE
    ↓ (title entered)
CON_BOARD_POST ← Correct!
    ↓ (/s typed)
board_post_string_cleanup() called
    ↓
mysql_board_finish_post(d, 1) saves post
    ↓
CON_PLAYING
```

#### Reply Post (Was Broken):
```
CON_BOARD_TITLE
    ↓ (title entered)
mysql_board_handle_reply_title()
    ↓
CON_BOARD_BODY ← WRONG!
    ↓ (/s typed)
NO cleanup handler found!
    ↓
Connection becomes inconsistent
    ↓
DISCONNECT (post not saved)
```

#### Reply Post (Now Fixed):
```
CON_BOARD_TITLE
    ↓ (title entered)
mysql_board_handle_reply_title()
    ↓
CON_BOARD_POST ← CORRECT!
    ↓ (/s typed)
board_post_string_cleanup() called
    ↓
mysql_board_finish_post(d, 1) saves post
    ↓
CON_PLAYING
```

## Solution

Changed `mysql_board_handle_reply_title()` to use the correct state:

### File: `src/mysql_boards.c`

**Before (line ~1270):**
```c
STATE(d) = CON_BOARD_BODY;
```

**After:**
```c
STATE(d) = CON_BOARD_POST;
```

Also added initialization of `d->backstr = NULL;` for consistency with the regular post flow.

## Technical Details

### Connection States Used by Board System

- **CON_BOARD_TITLE (81)** - Getting the title for a post or reply
- **CON_BOARD_POST (83)** - Editing post body (has cleanup handler)
- **CON_BOARD_POST_ABORT (84)** - Post aborted (has cleanup handler)
- ~~**CON_BOARD_BODY (82)** - Intended for body editing but NO cleanup handler!~~

### Why CON_BOARD_BODY Existed

The `CON_BOARD_BODY` state was defined in `structs.h` but was never properly integrated:
- It has no cleanup handler in `modify.c`
- It was probably intended for a different implementation approach
- The working implementation uses `CON_BOARD_POST` instead

### Proper String Editor Integration

For any connection state that uses the string editor, you **must**:
1. Define the state in `structs.h`
2. Add a cleanup handler function
3. Register the handler in the `cleanup_modes[]` array in `modify.c`
4. Handle the state in `string_add()` STRINGADD_ABORT case

## Files Modified

1. **src/mysql_boards.c** (line ~1270)
   - Changed `STATE(d) = CON_BOARD_BODY;` to `STATE(d) = CON_BOARD_POST;`
   - Added `d->backstr = NULL;` initialization

## Testing Checklist

### Before Fix:
- [x] `breply <number>` → enter title → write body → `/s` → **DISCONNECT**
- [x] Reply not saved to database
- [x] MUD stays running (not a crash)

### After Fix:
- [ ] `breply <number>` → enter title → write body → `/s` → **SUCCESS**
- [ ] Reply saved to database correctly
- [ ] User stays connected
- [ ] Confirmation message displayed
- [ ] Reply appears on board with proper formatting
- [ ] Quoted text from original post included
- [ ] Title formatted as "Re: Post #X - [additional text]"

## Related Functions

### Primary Functions:
- `do_reply_board()` - Command handler for breply
- `mysql_board_start_reply_title()` - Initiates reply process
- `mysql_board_handle_reply_title()` - **Fixed function** - creates quoted body
- `mysql_board_finish_post()` - Saves post to database
- `board_post_string_cleanup()` - String editor cleanup callback

### Flow:
```
do_reply_board()
    ↓
mysql_board_start_reply_title()
    ↓ (sets CON_BOARD_TITLE)
[User enters additional title text]
    ↓
mysql_board_handle_reply_title()
    ↓ (quotes original, sets CON_BOARD_POST)
[User edits body, types /s]
    ↓
string_add() detects /s
    ↓
board_post_string_cleanup()
    ↓
mysql_board_finish_post(d, 1)
    ↓
mysql_board_create_post()
    ↓
Post saved to database!
```

## Security Implications
None - this was a bug that prevented functionality, not a security issue.

## Performance Implications
None - fix is a simple state constant change.

## Backward Compatibility
✅ Fully compatible - only fixes broken functionality.

## Future Considerations

### CON_BOARD_BODY State
The `CON_BOARD_BODY` state (82) is defined but unused. Consider:
1. **Remove it** - if not needed for future features
2. **Document it** - if intentionally reserved for future use
3. **Add handler** - if it will be used differently than CON_BOARD_POST

### Recommendation
Since `CON_BOARD_POST` works correctly for both regular posts and replies, and `CON_BOARD_BODY` has no implementation, it should probably be removed to avoid confusion.

## Lessons Learned

1. **Always register cleanup handlers** for connection states that use string editor
2. **Test all code paths** - regular posts worked but replies didn't
3. **Connection state debugging** is critical for MUD stability
4. **String editor integration** requires careful state management

## Compilation Status
✅ **Successfully compiled** with no errors or warnings

## Credits
- Bug Report: User testing
- Root Cause Analysis: GitHub Copilot Assistant  
- Fix Implementation: GitHub Copilot Assistant

---
**Last Updated:** October 15, 2025  
**Status:** Fixed  
**Priority:** High (caused data loss and disconnections)
