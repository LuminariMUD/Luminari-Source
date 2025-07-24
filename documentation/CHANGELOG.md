# CHANGELOG

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

