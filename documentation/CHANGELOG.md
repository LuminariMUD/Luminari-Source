# CHANGELOG

## 2025-01-24

### Performance Optimizations

#### Performance Monitoring System Optimization
- **Implemented Configurable Monitoring Levels**: Reduced overhead by up to 95%
  - **Sampling Mode** (default): Monitors 1 in N pulses (90% overhead reduction)
  - **Basic Mode**: Only logs when performance exceeds 100% threshold
  - **Off Mode**: Completely disables monitoring for production
  - **Full Mode**: Original behavior for debugging
- **Dynamic Load-Based Monitoring**:
  - Automatically switches to full monitoring when server load > 150%
  - Returns to sampling mode when load drops below 130%
  - Prevents missing critical performance issues while minimizing overhead
- **Runtime Configuration**: New `perfconfig` command for administrators
  - No server restart required to adjust monitoring levels
  - Real-time status display and configuration
  - Adjustable sampling rates (e.g., 1/5, 1/20, 1/100 pulses)
- **Implementation Details**:
  - Created `perfmon_optimized.c` with optimized monitoring logic
  - Added `PERF_log_pulse_optimized()` function with level checking
  - Maintains backward compatibility with existing `perfmon` command
  - Weak symbol linkage for graceful fallback if not compiled
- **Files Added/Modified**:
  - New: `perfmon_config.h`, `perfmon_optimized.c`
  - Modified: `comm.c`, `perfmon.h`, `perfmon.cpp`, `act.h`, `interpreter.c`

## 2025-01-25

### Bug Fixes

#### Fixed Critical Player Data Structure Access Violations
- **Issue**: NPCs were attempting to access player-only data structures (player_specials), causing crashes
- **Affected Files**:
  - `treasure.c:1525,1702` - NPCs accessing PRF_USE_STORED_CONSUMABLES preference flag
  - `spec_procs.c:6315,6335,6354,6376,6395,6417,6436,6458` - NPCs accessing PRF_CONDENSED preference flag
  - `magic.c` - Multiple instances of NPCs accessing GET_PSIONIC_ENERGY_TYPE
- **Solution**: 
  - Added `!IS_NPC()` checks before all PRF_FLAGGED macro uses
  - Implemented safe energy type access: `IS_NPC(ch) ? DAM_MENTAL : GET_PSIONIC_ENERGY_TYPE(ch)`
  - NPCs now default to DAM_MENTAL damage type for psionic abilities
- **Impact**: Prevents server crashes when NPCs receive treasure, engage in combat with special weapons, or cast psionic spells

## 2025-01-27

### Performance Optimizations

#### do_save() Performance Improvements (257ms → ~50-75ms target)
- **Buffered I/O Implementation**: Replaced hundreds of individual fprintf() calls with a single buffered write operation
  - 64KB initial buffer with dynamic growth capability
  - All player data collected in memory before one disk write
  - Dramatically reduces system call overhead
- **String Operation Optimization**: 
  - Created `buffer_write_string_field()` helper function to consolidate repetitive string operations
  - Pre-allocated buffers to minimize memory allocations
  - Reduced redundant string copying operations
- **Performance Monitoring**: Added timing measurements to track save performance
  - Logs saves taking >50ms for ongoing monitoring
  - Previously: 257ms per save (down from 513ms)
  - Target: 50-75ms per save (75% reduction)

#### affect_update() Performance Improvements (30% CPU reduction target)
- **Enhanced NPC Processing**:
  - Skip NPCs without affects entirely (no processing needed)
  - Only update MSDP for players with active descriptors
  - Prevents unnecessary work for disconnected players
- **Safe Iteration**: Cache next pointer to handle character extraction during processing
- **Performance Metrics**:
  - Track affected characters vs total characters
  - Count total affects processed per update
  - Log metrics every 100 updates (10 minutes) for monitoring

### Bug Fixes

#### Fixed "degenerate board" Error
- **Issue**: Board initialization was missing the rnum field, causing find_board() to fail
- **Solution**: 
  - Explicitly initialized rnum to NOTHING in board_info array declarations
  - Fixed array initialization for both CAMPAIGN_DL (7 boards) and standard (1 board)
  - Added enhanced error logging to identify problematic boards
  - Added validation to ensure board array size matches NUM_OF_BOARDS
- **Impact**: Eliminates SYSERR log spam and allows bulletin boards to function properly

### Technical Details
- Added `<sys/time.h>` include to players.c for performance timing
- Fixed C99 compilation error in db.c (for loop variable declaration)
- Maintained full backwards compatibility with existing save files
- No changes to save file format or data structures
- All existing functionality preserved

