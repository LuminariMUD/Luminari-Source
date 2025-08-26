










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

## 5. Multiple Scripts Have Windows Line Endings
**Issue**: Multiple critical scripts have Windows-style line endings (CRLF) causing bash errors.
**Affected files**:
- `configure` - Build configuration script
- `Makefile.in` - Makefile template
- `autorun` - Server autorun script
- `autorun.sh` - Alternative autorun script
- `luminari.sh` - Server management script
- Various other shell scripts throughout the codebase

**Error**: Multiple "command not found" errors for `$'\r'`, syntax errors
**Example error from configure**:
```
configure: line 2: $'\r': command not found
configure: line 58: syntax error near unexpected token `$'do\r''
```

**Impact**: Cannot run configure, autorun, or other scripts on Linux/WSL without converting line endings.
**Solution**: Run `dos2unix` on all affected files:
```bash
# Install dos2unix if not already installed
sudo apt-get install dos2unix  # On Debian/Ubuntu
# or
sudo yum install dos2unix       # On RHEL/CentOS

# Convert critical files first
dos2unix configure Makefile.in autorun luminari.sh

# Convert all shell scripts
find . -name "*.sh" -type f -exec dos2unix {} \;

# Also check other potentially affected files
dos2unix autorun.* checkmud.sh vgrind*.sh
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

## 13. Zone File Format Error
**Issue**: Zone file format was incorrect. The zone loader expects a specific format with 3 lines:
1. Zone number (e.g., `#0`)
2. Builder names ending with `~` (e.g., `None.~`)
3. Zone name ending with `~` (e.g., `The Void~`)
4. Numeric line with at least 4 values: bottom_room top_room lifespan reset_mode

**Error**: "SYSERR: Format error in numeric constant line of world/zon/0.zon, attempting to fix."

**Solution**: Fixed zone file format from:
```
#0
The Void~
0 3099 30 0 0 0
```
To:
```
#0
None.~
The Void~
3000 3099 30 2
```

**Explanation**: 
- Added builder line (`None.~`)
- Changed numeric line to have exactly 4 values
- Changed bottom room from 0 to 3000 to include room 3001
- Changed reset mode from 0 to 2 (standard reset mode)

## 14. Room File Format Error
**Issue**: Room file had incorrect terminator causing "Room #0 is below zone" error
**Error**: "SYSERR: (parse_room) Room #0 is below zone 0 (bot=3000, top=3099)."
**Solution**: Removed the `#0` line from room file. Room files should end with `$` not `#0`.

## 15. Missing MySQL Check in load_paths()
**Issue**: The `load_paths()` function in mysql.c was causing segmentation fault when MySQL wasn't available
**Error**: Segmentation fault after "Loading paths. (MySQL)"
**Solution**: Added MySQL availability check at the beginning of `load_paths()`:
```c
if (!mysql_available) {
  log("INFO: Skipping path loading - MySQL not available.");
  return;
}
```

## 16. Creating Minimal Mob File
**Issue**: MUD requires at least one mob file to boot
**Error**: "SYSERR: boot error - 0 records counted in world/mob//index."
**Solution**: Created minimal mob file `lib/world/mob/0.mob` with a simple mob (#3001) and updated mob index

## 17. Object File Format Error
**Issue**: Object file had incorrect terminator like room file
**Error**: "SYSERR: fread_string: format error at or near object #0"
**Solution**: Removed the `#0` line from object file. Object files should end with `$` not `#0`.

## 18. Zone Reset Command Error
**Issue**: Zone file reset command references invalid vnums
**Error**: "SYSERR: zone file: Invalid vnum 1, cmd disabled"
**Details**: The zone reset command `O 0 3001 3001 1` has incorrect format - the last parameter (1) is being interpreted as a vnum

## 19. Missing Quest Index File
**Issue**: Quest system expects index file
**Error**: "SYSERR: opening index file 'world/qst/index': No such file or directory"
**Status**: Need to create `lib/world/qst/index` with just `$`

## Current Progress
**Latest test output shows**:
- MySQL bypass working correctly
- Zone, room, mob, and object files loading (with warnings)
- Game progressing much further in boot sequence
- Still encountering missing files but no longer crashing immediately

**Files successfully created and fixed**:
- Zone file with correct format
- Room file without #0 terminator
- Mob file (with some unrecognized keywords)
- Object file without #0 terminator
- Various text files in lib/text/

**Next steps needed**:
- Create quest index file
- Fix zone reset commands
- Address start room warnings
- Continue until full boot achieved

## 20. Configure Script Minor Error
**Issue**: The configure script displays an error at the very end: `cat: ./src/conf.h.in: No such file or directory`
**Details**: The configure script completes successfully and creates all necessary files, but has a final command that looks for conf.h.in in the wrong location (it's in the root directory, not src/)
**Impact**: Harmless - the configure script has already completed its work successfully. This is just a cosmetic issue.
**Status**: Can be ignored, does not affect build process

## Summary
A fresh install requires significant manual setup that isn't documented in a clear "Quick Start" guide. The main barriers are:
1. Configuration files that must be manually created
2. MySQL setup and configuration (now optional with code changes)
3. Missing text files
4. Missing world data files
5. Unclear startup process
6. Scripts with Windows line endings that need conversion

## Recommendations
1. Add a `QUICKSTART.md` with step-by-step instructions
2. Provide a setup script that creates required config files with sensible defaults
3. Include minimal required text files or create them automatically
4. Add database setup instructions or migration scripts
5. Fix line endings in shell scripts or add instructions to run `dos2unix` on all scripts
6. Make MySQL configuration optional for initial testing (use file-based storage as fallback)
7. Add a note in README about running `dos2unix` on all shell scripts after cloning on Windows/WSL

## DEPLOYMENT IMPROVEMENTS (2025-08-26)

### 🎯 KEY ACHIEVEMENT
**The game now builds successfully from a fresh clone!** All critical build issues have been resolved:
- Line endings fixed permanently
- Makefile.am source list bug fixed
- Autotools build process working perfectly
- Deploy script created for automated setup

### ✅ COMPLETED FIXES

1. **Fixed all Windows line endings** (Commit: bd62fbff)
   - Ran `dos2unix` on entire codebase (762 files converted)
   - Scripts now run correctly on Linux/WSL without errors
   - No more `$'\r'` command not found errors
   - **This issue is permanently resolved for all future clones**

2. **Fixed critical Makefile.am build issue** (Commit: e09e148f)
   - **Problem**: Comment lines in middle of source list broke line continuation
   - The comment after `i3_utils.c` caused all subsequent files (including `race.c` and `utils.c`) to be excluded from build
   - This caused "undefined reference" linking errors for `race_list`, `basic_mud_log`, etc.
   - **Solution**: Removed the problematic comment lines that interrupted the source list
   - Build now completes successfully with all source files properly included

3. **Created automated deployment script (`deploy.sh`)**
   - Automatically detects OS (Ubuntu/Debian/CentOS/Arch)
   - Installs all required dependencies based on OS
   - Handles configuration file creation from templates
   - Provides interactive and quick-setup modes
   - Includes database setup automation with secure password generation
   - Supports both development and production builds
   - Creates systemd service files for production deployment
   - Generates startup scripts for easy server management
   - Updated to properly use `make install` instead of manually moving binaries

4. **Autotools build confirmed working**
   - Successfully generates configure script with `autoreconf -fvi`
   - Creates src/conf.h properly during configure (in correct location)
   - Builds complete executable without errors
   - Properly installs to bin/circle with `make install`
   - No CMake configuration needed for basic build
   - Must run `chmod +x unittests/CuTest/make-tests.sh` for tests to work

### 🚀 QUICK START FOR NEW USERS

```bash
# Clone the repository
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source

# Option 1: Use the automated deployment script (RECOMMENDED)
./deploy.sh --quick --skip-db   # Quick setup without database

# Option 2: Manual build with autotools
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h
chmod +x unittests/CuTest/make-tests.sh  # Make test script executable
autoreconf -fvi
./configure
make -j$(nproc) all
make install

# Run the game (after creating minimal world files - see below)
bin/circle -d lib
```

**Note**: The executable is installed to `bin/circle` after `make install`

### Key Features of deploy.sh
- **OS Detection**: Automatically identifies Linux distribution and installs appropriate packages
- **Configuration Automation**: Copies and configures all required .h files from examples
- **Database Setup**: Creates database, user, and sets permissions automatically
- **Build System Support**: Works with both CMake and Autotools
- **Environment Setup**: Creates all required directories and sets proper permissions
- **Service Management**: Optional systemd service creation for production servers
- **Quick Mode**: `--quick` flag for zero-interaction deployment with sensible defaults

### Usage Examples
```bash
./deploy.sh              # Interactive setup with prompts
./deploy.sh --quick      # Quick setup with all defaults
./deploy.sh --dev        # Development mode with debug tools
./deploy.sh --prod       # Production optimized build
./deploy.sh --skip-deps  # Skip dependency installation
./deploy.sh --skip-db    # Skip database setup
```

### ⚠️ REMAINING ISSUES TO FIX

1. **Missing minimal world files** - Game needs at least basic world data to start
   - Need to create minimal world package or include in repo
   - Currently requires manual creation of zone/room/mob/obj files

2. **Build output location** (RESOLVED)
   - Autotools builds `circle` executable in root directory, then `make install` copies to `bin/`
   - CMake builds directly in `bin/` directory
   - Both systems ultimately put the executable in `bin/circle`

3. **Missing text files** - Game expects but can run without:
   - lib/text/news, credits, motd, imotd
   - lib/text/help/help, help/ihelp
   - lib/text/info, wizlist, immlist, policies, handbook, background, greetings

4. **MySQL configuration template has placeholders**
   - lib/mysql_config_example needs actual values, not `<host here>`
   - Should provide working defaults or better template

### 📋 TODO for Complete Automation
1. Create minimal world data package that works out of the box
2. Add `.env` file support for configuration
3. Create health check script to verify successful deployment
4. Add Docker/container support for one-command deployment
5. Include basic text files with default content
6. Fix MySQL config template to use actual default values
7. Add automated testing to CI/CD pipeline
8. Create migration scripts for database schema updates

### Next Steps for Testing
- Test deploy.sh on fresh Ubuntu/Debian system
- Test deploy.sh on fresh CentOS/RHEL system
- Verify world data initialization works
- Document any remaining edge cases