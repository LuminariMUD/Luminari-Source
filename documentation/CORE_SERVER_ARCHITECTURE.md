# LuminariMUD Core Server Architecture

## Overview

LuminariMUD uses a single-threaded, event-driven server architecture based on the classic CircleMUD/tbaMUD design. The server handles all client connections, game logic, and world simulation in a single main loop that executes approximately 10 times per second (every 0.1 seconds).

## Main Server Components

### 1. Main Entry Point (`main()` function)

The server startup sequence follows this pattern:

```c
int main(int argc, char **argv)
{
    // 1. Parse command line arguments
    // 2. Load configuration
    // 3. Setup logging
    // 4. Change to data directory
    // 5. Initialize game or run syntax check
    // 6. Cleanup and shutdown
}
```

**Key Command Line Options:**
- `-f <file>` - Specify configuration file
- `-o <file>` - Specify log file
- `-d <dir>` - Set data directory
- `-C<socket>` - Copyover recovery mode
- `-s` - Syntax check mode only
- `<port>` - Port number to listen on

### 2. Game Initialization (`init_game()`)

The initialization sequence is critical for proper server startup:

```c
static void init_game(ush_int local_port)
{
    // 1. Create kill script protection
    touch(KILLSCRIPT_FILE);
    
    // 2. Initialize random number generator
    circle_srandom(time(0));
    
    // 3. Determine player limits
    max_players = get_max_players();
    
    // 4. Setup network socket (unless copyover)
    if (!fCopyOver)
        mother_desc = init_socket(local_port);
    
    // 5. Initialize event system
    event_init();
    
    // 6. Setup character lookup hash table
    init_lookup_table();
    
    // 7. Load world data
    boot_db();
    
    // 8. Setup signal handlers
    signal_setup();
    
    // 9. Handle copyover recovery if needed
    if (fCopyOver)
        copyover_recover();
    
    // 10. Enter main game loop
    game_loop(mother_desc);
    
    // 11. Shutdown sequence
    // - Save all player data
    // - Close all sockets
    // - Save world state
    // - Clean shutdown or reboot
}
```

### 3. Main Game Loop (`game_loop()`)

The heart of the server is the main game loop, which handles all server operations:

```c
void game_loop(socket_t local_mother_desc)
{
    fd_set input_set, output_set, exc_set, null_set;
    struct timeval timeout, last_time, opt_time;
    
    while (!circle_shutdown) {
        // 1. Handle no-connection sleep state
        // 2. Setup file descriptor sets
        // 3. Calculate timing for next iteration
        // 4. Sleep until next pulse
        // 5. Poll for network activity (select())
        // 6. Accept new connections
        // 7. Handle exceptions and disconnections
        // 8. Process input from all descriptors
        // 9. Execute commands and game logic
        // 10. Process output to all descriptors
        // 11. Run heartbeat functions
        // 12. Performance monitoring
    }
}
```

**Timing System:**
- **Pulse Rate:** 10 pulses per second (0.1 second intervals)
- **Optimal Time:** `OPT_USEC` microseconds per pulse
- **Sleep Mechanism:** Precise timing using `select()` with timeout

## Network Architecture

### Socket Management

The server uses BSD sockets for network communication:

**Mother Socket (`mother_desc`):**
- Listens for new incoming connections
- Bound to specified port (default varies by configuration)
- Non-blocking mode for connection acceptance

**Descriptor Management:**
- Each client connection gets a `descriptor_data` structure
- Linked list of all active descriptors (`descriptor_list`)
- File descriptor sets for `select()` system call

### Connection Handling

```c
// New connection acceptance
if (FD_ISSET(local_mother_desc, &input_set))
    new_descriptor(local_mother_desc);

// Input processing
for (d = descriptor_list; d; d = next_d) {
    if (FD_ISSET(d->descriptor, &input_set)) {
        if (process_input(d) < 0)
            close_socket(d);
    }
}

// Output processing  
for (d = descriptor_list; d; d = next_d) {
    if (*(d->output) && FD_ISSET(d->descriptor, &output_set)) {
        if (process_output(d) < 0)
            close_socket(d);
    }
}
```

### Input/Output System

**Input Processing:**
- Raw socket data read into buffers
- Line-based command parsing
- Alias expansion and command queuing
- State-based processing (login, playing, editing, etc.)

**Output Processing:**
- Buffered output system with queues
- Color code processing
- Protocol handling (telnet, MSDP, etc.)
- Flow control and connection management

## Heartbeat System

The heartbeat function runs various game subsystems at different intervals:

```c
void heartbeat(int heart_pulse)
{
    // Every pulse (0.1 seconds)
    event_process();                    // Event system
    
    // Every 0.5 seconds  
    if (!(heart_pulse % PULSE_DG_SCRIPT))
        script_trigger_check();         // DG Scripts
    
    // Every second
    if (!(heart_pulse % PASSES_PER_SEC)) {
        msdp_update();                  // Protocol updates
        travel_tickdown();              // Movement timers
        craft_update();                 // Crafting system
        // ... other per-second updates
    }
    
    // Every 5 seconds
    if (!(heart_pulse % PULSE_MOBILE))
        mobile_activity();              // NPC actions
    
    // Every 30 seconds  
    if (!(heart_pulse % PULSE_ZONE))
        zone_update();                  // Zone resets
    
    // Every MUD hour (75 seconds default)
    if (!(heart_pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
        weather_and_time(1);            // Weather/time
        point_update();                 // HP/MP regeneration
        check_timed_quests();           // Quest timers
    }
    
    // Every minute
    if (!(heart_pulse % PULSE_AUTOSAVE)) {
        Crash_save_all();               // Player saves
        House_save_all();               // House saves
    }
}
```

## Signal Handling

The server handles various UNIX signals for administrative control:

```c
static void signal_setup(void)
{
    signal(SIGUSR1, reread_wizlists);   // Reload wizard lists
    signal(SIGUSR2, checkpointing);     // Force save checkpoint  
    signal(SIGINT, hupsig);             // Graceful shutdown
    signal(SIGTERM, hupsig);            // Graceful shutdown
    signal(SIGPIPE, SIG_IGN);           // Ignore broken pipes
    signal(SIGALRM, SIG_IGN);           // Ignore alarm signals
    signal(SIGCHLD, reap);              // Clean up child processes
}
```

**Signal Functions:**
- `reread_wizlists()` - Reload administrator lists
- `checkpointing()` - Force save all player data
- `hupsig()` - Initiate graceful shutdown
- `reap()` - Clean up zombie child processes

## World Loading (`boot_db()` and `boot_world()`)

The world loading process is complex and follows a specific order:

```c
void boot_world(void)
{
    // 1. Database connection
    connect_to_mysql();
    
    // 2. Core world data
    index_boot(DB_BOOT_ZON);    // Zones
    index_boot(DB_BOOT_TRG);    // Triggers  
    index_boot(DB_BOOT_WLD);    // Rooms
    
    // 3. Extended world features
    load_regions();             // Geographic regions
    load_paths();               // Travel paths
    renum_world();              // Renumber rooms
    
    // 4. Entities
    index_boot(DB_BOOT_MOB);    // NPCs/Monsters
    index_boot(DB_BOOT_OBJ);    // Objects/Items
    
    // 5. Game systems
    load_class_list();          // Character classes
    assign_feats();             // Feat system
    load_deities();             // Religion system
    
    // 6. Finalization
    renum_zone_table();         // Final zone numbering
    boot_social_messages();     // Social commands
    load_help();                // Help system
}
```

## Performance Monitoring

The server includes built-in performance profiling through the `perfmon.cpp` system:

```c
// Performance tracking macros
PERF_PROF_ENTER(pr_main_loop_, "Main Loop");
// ... code to profile ...
PERF_PROF_EXIT(pr_main_loop_);

// Key performance areas monitored:
// - Main loop execution time
// - Input/output processing
// - Event system processing
// - Script execution
// - Database operations
// - Heartbeat functions
```

### Performance Monitoring Architecture

**Hierarchical Data Storage:**
- Pulse-level data (individual game loop iterations)
- Second-level aggregation (from pulse data)
- Minute-level aggregation (from second data)
- Hour-level aggregation (from minute data)
- Circular buffers prevent memory growth while maintaining history

**Dual Statistics Tracking:**
- **Per-pulse statistics**: Reset each game loop iteration for current performance
- **Cumulative statistics**: Accumulated since server startup for long-term trends

**Threshold Monitoring:**
- Configurable performance thresholds (10%, 30%, 50%, 70%, 90%, 100%, 250%, 500%, 1000%, 2500%)
- Automatic violation counting and reporting
- Performance percentage calculation based on allocated time slice

### API Functions

```c
// Pulse performance logging
void PERF_log_pulse(double val);

// Report generation
size_t PERF_repr(char *out_buf, size_t n);
size_t PERF_prof_repr_pulse(char *out_buf, size_t n);
size_t PERF_prof_repr_total(char *out_buf, size_t n);
size_t PERF_prof_repr_sect(char *out_buf, size_t n, const char *id);

// Section management (typically used via macros)
void PERF_prof_sect_init(struct PERF_prof_sect **ptr, const char *id);
void PERF_prof_sect_enter(struct PERF_prof_sect *ptr);
void PERF_prof_sect_exit(struct PERF_prof_sect *ptr);
void PERF_prof_reset(void);
```

### Code Quality Improvements (2025)

The performance monitoring system underwent comprehensive refactoring to address:

**Memory Management:**
- Fixed memory leaks in section creation
- Added proper RAII compliance with destructors
- Implemented safe resource cleanup

**Buffer Safety:**
- Added comprehensive bounds checking to prevent overflows
- Safe string operations with proper size validation
- Null termination guarantees

**Performance Optimizations:**
- 15-20% improvement in report generation
- Optimized string operations and loop conditions
- Enhanced circular buffer efficiency

**Testing:**
- Comprehensive unit test suite (`test_perfmon.cpp`)
- Memory leak verification
- Buffer safety validation
- Performance regression testing

### Future Performance Monitoring Enhancements

**Recommended Improvements:**
1. **Thread Safety** - Add mutex protection for multi-threaded environments
2. **Runtime Configuration** - Make buffer sizes and thresholds configurable
3. **Debug Logging** - Optional detailed logging for troubleshooting
4. **Export Formats** - JSON/XML export for external monitoring tools
5. **Real-time Alerts** - Hooks for performance threshold notifications
6. **Historical Analysis** - Long-term trend analysis and reporting

## Memory Management

**Buffer Pools:**
- Pre-allocated buffer pools for common operations
- Reduces malloc/free overhead
- Configurable buffer sizes

**Descriptor Management:**
- Linked list of active connections
- Automatic cleanup on disconnect
- Memory leak prevention

**World Data:**
- Static allocation for world structures
- Reference counting for shared data
- Cleanup functions for shutdown

## Error Handling and Logging

**Logging System:**
- Configurable log levels
- Timestamped entries
- Separate error streams
- Rotation and archival support

**Error Recovery:**
- Graceful handling of network errors
- Automatic cleanup of corrupted connections
- Core dump generation for debugging
- Restart mechanisms for critical failures

## Copyover System

LuminariMUD supports "copyover" - restarting the server without disconnecting players:

1. **Preparation:** Save all critical state
2. **Socket Transfer:** Pass socket descriptors to new process
3. **Restart:** Execute new server binary
4. **Recovery:** Restore player connections and state
5. **Continuation:** Resume normal operation

This allows for seamless updates and maintenance without player disruption.

## Configuration System

The server uses a flexible configuration system:

- **Config Files:** Text-based configuration files
- **Runtime Changes:** Some settings can be changed while running
- **Default Values:** Sensible defaults for all settings
- **Validation:** Input validation and error checking

## Thread Safety

**Important Note:** LuminariMUD is single-threaded by design. All operations must be thread-safe or avoided entirely. This simplifies development but requires careful attention to:

- Non-blocking I/O operations
- Efficient algorithms to prevent lag
- Proper resource management
- Event-driven programming patterns

The single-threaded design ensures data consistency and simplifies debugging while maintaining excellent performance for typical MUD workloads.
