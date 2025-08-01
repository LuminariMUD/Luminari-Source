# Fresh Install Problems - LuminariMUD

This document lists all issues encountered when attempting a fresh install and run of LuminariMUD from a clean repository pull.

## 1. Configuration Files Not Created by Default
**Issue**: The build requires several configuration files that don't exist in a fresh clone.
**Files needed**:
- `src/campaign.h` (must copy from `src/campaign.example.h`)
- `src/mud_options.h` (must copy from `src/mud_options.example.h`)
- `src/vnums.h` (must copy from `src/vnums.example.h`)
- `lib/mysql_config` (must copy from `lib/mysql_config_example`)

**Impact**: Build fails without these files.

## 2. MySQL Configuration Template Has Placeholder Values
**Issue**: The `mysql_config_example` file contains placeholder values that need to be replaced:
```
mysql_host = <host here, example: localhost>
mysql_database = <name of database here, example: MyMUD_DB>
mysql_username = <username here, example: mymud_user>
mysql_password = <password here, example: a$3koorPw34d>
```

**Impact**: Game fails to parse MySQL configuration with error "SYSERR: Malformed line in MySQL configuration"

## 3. Missing Text Files
**Issue**: The game expects various text files that don't exist in the repository:
- `text/news`
- `text/credits`
- `text/motd`
- `text/imotd`
- `text/help/help`
- `text/help/ihelp`
- `text/info`
- `text/wizlist`
- `text/immlist`
- `text/policies`
- `text/handbook`
- `text/background`
- `text/greetings`

**Impact**: Multiple SYSERR messages on startup, though game continues to load.

## 4. Missing etc/config File
**Issue**: Game looks for `etc/config` file which doesn't exist.
**Error**: "No etc/config file, using defaults: No such file or directory"
**Impact**: Game uses default configuration values.

## 5. Autorun Script Has Windows Line Endings
**Issue**: The `autorun.sh` script has Windows-style line endings (CRLF) causing bash errors.
**Error**: Multiple "command not found" errors for `$'\r'`
**Impact**: Cannot use the autorun script on Linux/WSL without converting line endings.
**Solution**: Run `dos2unix` on all script files:
```bash
# Install dos2unix if not already installed
sudo apt-get install dos2unix  # On Debian/Ubuntu
# or
sudo yum install dos2unix       # On RHEL/CentOS

# Convert all shell scripts
dos2unix autorun.sh
dos2unix autorun
dos2unix *.sh
dos2unix **/*.sh
```

## 6. No Clear Documentation on Running the Game
**Issue**: It's not immediately clear how to properly start the game after building.
**Correct command**: `bin/circle -q 30148 -d lib`
**Common mistakes**:
- Trying to run from `bin/` directory
- Not specifying the data directory
- Not knowing the default port

## 7. No Database Setup Instructions in Quick Start
**Issue**: The game requires a MySQL database but there are no quick instructions for:
- Creating the database
- Running schema migrations
- Setting up initial data

## 8. Build Output Directory
**Issue**: The build creates the executable in `bin/circle` but this isn't immediately obvious.
**Note**: The `bin/` directory already exists in the repo, so it's not clear that's where the output goes.

## 9. Missing World Files
**Issue**: The game expects world data files that don't exist in the repository:
- `world/zon/index` - Zone index file
- `world/wld/*` - World/room files
- `world/mob/*` - Mobile (NPC) files
- `world/obj/*` - Object files
- `world/shp/*` - Shop files

**Error**: "SYSERR: opening index file 'world/zon/index': No such file or directory"
**Impact**: Game cannot start without at least minimal world data.

## 10. MySQL Graceful Degradation Working
**Success**: After implementing MySQL bypass code, the game now:
- Shows warnings instead of exiting when MySQL config is missing
- Continues loading without MySQL support
- Displays: "WARNING: Running without MySQL support - some features will be disabled."

**Code Changes Made**:
1. Modified `src/mysql.c`:
   - Added `bool mysql_available = FALSE;` global flag
   - Changed `connect_to_mysql()` to not exit on failure
   - Changed all `log("SYSERR:...")` and `exit(1)` to `log("WARNING:...")` and `return`
   - Set `mysql_available = TRUE` only on successful connection

2. Modified `src/mysql.h`:
   - Added `extern bool mysql_available;` declaration

3. Modified `src/mysql.c` functions:
   - `mysql_query_safe()` - Added check for `mysql_available`
   - `load_regions()` - Added early return if MySQL not available

4. Modified `src/account.c`:
   - `load_account()` - Added check to return -1 if MySQL not available

**Note**: This is a positive development - the game can now start without MySQL configuration.

## 11. Creating Minimal World Files
**Issue**: World data files are gitignored and not included in repository
**Solution Attempted**: Created minimal world files to allow startup:

### Files Created:
1. `lib/world/zon/index` - Contains: `0.zon` and `$`
2. `lib/world/zon/0.zon` - Minimal zone file:
   ```
   #0
   The Void~
   0 3099 30 0 0 0
   O 0 3001 3001 1        (load object 3001 to room 3001)
   S
   $
   ```
3. `lib/world/wld/index` - Contains: `0.wld` and `$`
4. `lib/world/wld/0.wld` - Minimal room file:
   ```
   #3001
   The Void~
   You are floating in The Void.
   ~
   30 d 0 0 0 0
   S
   #0
   $
   ```
5. `lib/world/mob/index` - Empty (just `$`)
6. `lib/world/obj/index` - Contains: `0.obj` and `$`
7. `lib/world/obj/0.obj` - Minimal object file:
   ```
   #3001
   a sign~
   a sign~
   A sign floats here.~
   ~
   12 0 0 0 0 a 0 0 0 0 0 0 0
   0 0 0 0
   1 0 0
   #0
   $
   ```
8. `lib/world/shp/index` - Empty (just `$`)
9. `lib/world/trg/index` - Empty (just `$`)

### Text Files Created:
- `lib/text/credits` - Basic credits
- `lib/text/news` - Welcome message
- `lib/text/motd` - Message of the day
- Empty files: `imotd`, `help/help`, `help/ihelp`, `info`, `wizlist`, `immlist`, `policies`, `handbook`, `background`, `greetings`

**Current Status**: With these minimal files, the MUD progresses further in boot process but encounters zone format errors.

## 12. Running the MUD - Command Sequence
**Commands executed**:
1. `cp src/campaign.example.h src/campaign.h`
2. `cp src/mud_options.example.h src/mud_options.h` 
3. `cp src/vnums.example.h src/vnums.h`
4. `cp lib/mysql_config_example lib/mysql_config` (later removed for testing)
5. `make clean && make` - Build successful
6. `bin/circle -q 30148 -d lib` - To run the MUD

**Current Error Output**:
```
Aug  1 19:24:45 :: SYSERR: Format error in numeric constant line of world/zon/0.zon, attempting to fix.
Aug  1 19:24:45 :: Loading triggers and generating index.
Aug  1 19:24:45 :: Loading rooms.
Aug  1 19:24:45 ::    2 rooms, 464 bytes.
Aug  1 19:24:45 :: SYSERR: Room 3001 is outside of any zone.
```

**Progress**: The MUD is now attempting to load world files and getting further in the boot process.

## Summary
A fresh install requires significant manual setup that isn't documented in a clear "Quick Start" guide. The main barriers are:
1. Configuration files that must be manually created
2. MySQL setup and configuration (now optional with code changes)
3. Missing text files
4. Missing world data files
5. Unclear startup process

## Recommendations
1. Add a `QUICKSTART.md` with step-by-step instructions
2. Provide a setup script that creates required config files with sensible defaults
3. Include minimal required text files or create them automatically
4. Add database setup instructions or migration scripts
5. Fix line endings in shell scripts or add instructions to run `dos2unix` on all scripts
6. Make MySQL configuration optional for initial testing (use file-based storage as fallback)
7. Add a note in README about running `dos2unix` on all shell scripts after cloning on Windows/WSL