# MySQL Connection Stability Fix

## Problem
"Lost connection to MySQL server during query" errors occurring due to improper connection checking.

## Solution Implemented
1. Created `ensure_mysql_connection()` helper function in `src/mysql.c`
2. Added `MYSQL_PING_CONN(x)` macro in `src/mysql.h`
3. Set connection timeouts (60s connect, 300s read/write)

## Migration Instructions

### Replace all bare `mysql_ping()` calls:

**OLD:**
```c
mysql_ping(x);
```

**NEW:**
```c
if (!MYSQL_PING_CONN(x)) {
    log("SYSERR: %s: Database connection failed", __func__);
    return;  // or handle error appropriately
}
```

### Files Requiring Updates (27+ remaining):

**Priority 1 - Player Data:**
- `src/players.c` (lines 4156, 4283)
- `src/db.c` (lines 7441, 7478)

**Priority 2 - Mail System:**
- `src/new_mail.c` (lines 166, 256-258, 358-359, 462, 557-559)

**Priority 3 - Game Systems:**
- `src/templates.c` (lines 46, 228, 414, 465, 587, 769, 961)
- `src/craft.c` (lines 3562, 3598)
- `src/act.wizard.c` (line 10788)

**Priority 4 - Wilderness/Paths:**
- `src/mysql.c` (lines 2070, 2127, 2256, 2503, 2525, 2562, 2630)

**Priority 5 - Anything missed!**

## Testing
After updates, monitor logs for:
- "WARNING: <function>: MySQL connection lost" messages
- "INFO: <function>: Successfully reconnected" confirmations
- Absence of "Lost connection to MySQL server" errors

## Notes
- The macro automatically includes function name for debugging
- Connection timeouts prevent long query failures
- MYSQL_OPT_RECONNECT already enabled for auto-reconnection