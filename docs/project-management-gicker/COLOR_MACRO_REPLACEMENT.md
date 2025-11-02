# Color Macro Replacement - MySQL Boards and bedit

## Date: October 15, 2025

## Summary
Replaced CircleMUD-style color macros (like `CCYEL(ch, C_SPR)`) with Luminari's `\t` color code format in the MySQL boards system and board editor.

## Color Code Mapping

The following CircleMUD color macros were replaced with Luminari color codes:

| CircleMUD Macro | Luminari Code | Description |
|----------------|---------------|-------------|
| `CCYEL(ch, C_SPR)` | `\tY` | Bright Yellow |
| `CCYEL(ch, C_NRM)` | `\tY` | Bright Yellow |
| `CCRED(ch, C_SPR)` | `\tR` | Bright Red |
| `CCGRN(ch, C_SPR)` | `\tG` | Bright Green |
| `CCCYN(ch, C_SPR)` | `\tC` | Bright Cyan |
| `CCWHT(ch, C_CMP)` | `\tW` | Bright White |
| `CCWHT(ch, C_SPR)` | `\tW` | Bright White |
| `CCBLU(ch, C_SPR)` | `\tB` | Bright Blue |
| `CCMAG(ch, C_SPR)` | `\tM` | Bright Magenta |
| `CCBLK(ch, C_SPR)` | `\tk` | Black |
| `CCNRM(ch, C_SPR)` | `\tn` | Normal/Reset |
| `CCNRM(ch, C_NRM)` | `\tn` | Normal/Reset |

## Files Modified

### src/mysql_boards.c
Replaced all CircleMUD color macros with `\t` codes in:
- `mysql_board_show_list()` - Board list display (empty board message, header, post rows, footer)
- `mysql_board_show_post()` - Individual post display
- `mysql_board_show_help()` - Help text display
- `show_board_in_room()` - Room board notification

### src/bedit.c
No changes needed - already uses OLC color variables (`grn`, `nrm`, `cyn`, `yel`, etc.) which are externally defined and work correctly.

## Technical Changes

### Before (CircleMUD style):
```c
sprintf(buf,
    "\r\n%s+------------------------------------------------------------------------------+%s\r\n"
    "%s|%-78.78s|%s\r\n",
    CCYEL(ch, C_SPR), CCNRM(ch, C_SPR),
    CCYEL(ch, C_SPR), board_name_buf, CCNRM(ch, C_SPR));
```

### After (Luminari style):
```c
sprintf(buf,
    "\r\n\tY+------------------------------------------------------------------------------+\tn\r\n"
    "\tY|%-78.78s|\tn\r\n",
    board_name_buf);
```

## Benefits

1. **Consistency**: Uses the same color code system as the rest of Luminari
2. **Simplicity**: Fewer function calls, more readable code
3. **Compatibility**: Works with the existing `parse_at()` system already in place
4. **Performance**: Slightly faster (no macro expansion/function calls for colors)
5. **Maintainability**: Easier to see what colors are being used at a glance

## Examples of Color Usage

### Board List Header
```
\tY+------------------------------------------------------------------------------+\tn
\tY|Board Name                                                                  |\tn
\tY+------------------------------------------------------------------------------+\tn
```
- Yellow (`\tY`) for borders
- Normal (`\tn`) to reset

### Post Display
```
\tC\tW[Post Title]\tn
\tY================================================================================\tn
\tCPost #123\tn                                                      \tCBy: PlayerName\tn
```
- Cyan (`\tC`) for labels
- Bright White (`\tW`) for title
- Yellow (`\tY`) for separators

### Empty Board Message
```
\tY|\tRThere are no posts on this board.\tY|\tn
\tY|\tGType 'board help' for board commands\tY|\tn
```
- Red (`\tR`) for warning/empty message
- Green (`\tG`) for help text

## Compilation Status
✅ **Successfully compiled** with no new errors
- Only pre-existing warnings about format strings and ACMD_DECL
- All color codes properly embedded in string literals

## Testing Recommendations

1. **View board list** - Check that borders and text display with correct colors
2. **Read a post** - Verify post title, header, and body render properly
3. **Empty board** - Confirm empty message displays in red
4. **Help text** - Ensure board help shows with proper cyan/yellow colors
5. **bedit menu** - Verify board editor still displays correctly with its OLC colors
6. **blist command** - Check board list for admins shows proper formatting

## Notes

- The `parse_at()` function continues to work as before, converting user-entered `@Y`, `@C`, etc. into `\t` codes
- Board names, post titles, and post bodies can still contain user-entered color codes
- The system now uses consistent color coding throughout the entire board interface
- No functional changes - only visual/color code format changes

## Author
GitHub Copilot Assistant
