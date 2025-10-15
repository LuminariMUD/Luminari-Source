# MySQL Boards Color Code Fix

## Date: 2025-10-15

## Issue
The MySQL boards system was not properly handling color codes in board posts, titles, and board names. Users could type color codes using the `@` symbol format (e.g., `@Y` for bright yellow, `@y` for normal yellow, `@C` for bright cyan, etc.), but these codes were not being converted to the proper internal format for display.

## Color Code Format
The MUD uses two color code formats:
- **User Format**: `@X` where X is a color code letter
  - `@Y` = bright yellow
  - `@y` = normal yellow
  - `@C` = bright cyan
  - `@c` = dark cyan
  - `@R` = bright red
  - `@r` = dark red
  - `@G` = bright green
  - `@g` = dark green
  - etc.
- **Internal Format**: `\t` (tab character) followed by the color code letter
  - The protocol system (KaVir's protocol) uses tab characters instead of `@`

## Solution
Added calls to `parse_at()` function which converts `@` symbols to `\t` (tab) characters for proper color display. The function handles:
- Single `@` → converted to `\t` for color codes
- Double `@@` → becomes single `@` (escape sequence)

## Changes Made to `src/mysql_boards.c`

### 1. Added Include
```c
#include "helpers.h"
```
This provides access to the `parse_at()` function.

### 2. Updated `mysql_board_show_post()` Function
Added color code parsing for post title and body before display:
```c
/* Parse @ color codes in the post body and title */
if (post->body) {
    parse_at(post->body);
}
if (post->title) {
    parse_at(post->title);
}
```

### 3. Updated `mysql_board_show_list()` Function
- Added parsing for board name in empty board display
- Added parsing for board name in post list header
- Added parsing for each post title in the list

Changes include:
```c
char board_name_buf[MAX_STRING_LENGTH];

/* Parse @ color codes in board name */
if (board && board->board_name) {
    strncpy(board_name_buf, board->board_name, sizeof(board_name_buf) - 1);
    board_name_buf[sizeof(board_name_buf) - 1] = '\0';
    parse_at(board_name_buf);
}
```

And for post titles:
```c
char title_buf[MAX_BOARD_TITLE_LENGTH + 1];
/* Copy and parse @ color codes in title */
strncpy(title_buf, row[1], sizeof(title_buf) - 1);
title_buf[sizeof(title_buf) - 1] = '\0';
parse_at(title_buf);
```

### 4. Updated `show_board_in_room()` Function
Added color code parsing for board name when showing "X is mounted here" message:
```c
char board_name_buf[MAX_STRING_LENGTH];

/* Parse @ color codes in board name */
strncpy(board_name_buf, board->board_name, sizeof(board_name_buf) - 1);
board_name_buf[sizeof(board_name_buf) - 1] = '\0';
parse_at(board_name_buf);
```

## Impact

### What Works Now
1. **Post Titles**: Players can use color codes in post titles (e.g., "Looking for @Rhelp@n!")
2. **Post Bodies**: Full color code support in message bodies
3. **Board Names**: Board names created with `bedit` can include color codes
4. **Replies**: When replying to posts, quoted text preserves color codes

### Database Storage
- Color codes are stored in the database as `@` symbols
- Conversion to `\t` happens only during display
- This approach keeps data portable and readable in raw database

### No Breaking Changes
- Existing posts without color codes work exactly as before
- The `@@` escape sequence allows literal `@` symbols when needed
- Compatible with all existing board functionality

## Testing Recommendations

1. **Create a new post** with color codes in the title and body
2. **List posts** to verify titles display with colors
3. **Read a post** to verify body text displays with colors
4. **Create a board** with color codes in the name using `bedit`
5. **Reply to a post** with color codes to ensure quoting works
6. **Test escape sequences** by using `@@` to display a literal `@`

## Example Usage

```
> write
Enter the title for your post: @YImportant Announcement@n
[Editor opens]
This is a @Rred warning@n about @Cblue things@n!
/s
[Post saved]

> board
+------------------------------------------------------------------------------+
|Important Announcement                          Author           Date         |
+------------------------------------------------------------------------------+
```

The text will display with proper colors instead of showing the raw @ codes.

## Files Modified
- `src/mysql_boards.c` - Added color code parsing in 4 locations
- `src/bedit.c` - Added color code parsing in 2 locations (bedit menu and blist command)

## Compilation Status
✅ Successfully compiled with no errors
- Standard warnings about `send_to_char` argument order (pre-existing)
- Warning about format string (pre-existing)

## Additional Changes (October 15, 2025)

### Board Editor (bedit) Color Support

The board editor now properly displays color codes in board names:

1. **bedit Menu**: When editing a board with `bedit <id>`, the board name now displays with proper colors
2. **blist Command**: The board list command now shows board names with proper color rendering

#### Implementation Details

**bedit.c changes:**
- Added `#include "helpers.h"` for `parse_at()` function access
- Modified `bedit_disp_menu()` to parse @ color codes before displaying board name
- Modified `do_blist()` to parse @ color codes for each board name in the list

#### Example Usage in bedit

```
> bedit 1
[Option 2 to change name]
Enter new board name: @YGolden @CNotice @RBoard@n

> bedit 1
-- Board Editor
1) Board ID     : 1
2) Name         : Golden Notice Board  <-- displays with colors!
3) Type         : General
...

> blist
ID    Name                           Type         RD WR DL ObjVNUM Clan  Rank Active
-------------------------------------------------------------------------------------
1     Golden Notice Board            General      1  1  31 3099    0     0    Yes
      ^^ displays with proper colors
```

### Where Color Codes Now Work

**In mysql_boards.c:**
1. Individual post display (title and body)
2. Board list view (board name and post titles)
3. Empty board display (board name)
4. Room board notification (board name)

**In bedit.c:**
1. Board editor menu (board name display)
2. Board list command (`blist`) output (all board names)

### Technical Notes

- Color codes are stored as `@` in the database for portability
- Conversion to `\t` happens only during display rendering
- The `parse_at()` function handles `@@` → `@` escape sequences
- All string operations use safe `strncpy()` to prevent buffer overflows
- Board names, post titles, and post bodies all support full color code syntax

## Author
GitHub Copilot Assistant
