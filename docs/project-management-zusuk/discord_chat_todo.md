# Discord Chat Bridge - Current Status and Issues

## Problem Summary
The Discord bridge is not initializing after MUD server restarts or copyovers. The main symptom is that the `discord` command returns "Discord bridge is not initialized" even though the initialization code is being called.

## Current Situation (As of August 14, 2025)

### What's Working:
1. Discord bridge compiles successfully
2. Color codes are properly converted (CCYEL, CCNRM macros)
3. Port 8181 is configured in Plesk firewall
4. Socket options (SO_REUSEADDR, SO_REUSEPORT) are supported on the system
5. Shutdown before copyover is implemented

### Main Issue:
After copyover or server restart, the Discord bridge fails to initialize with "Address already in use" error, even though:
- Port 8181 shows as in use by the MUD process (expected)
- SO_REUSEPORT is set (should allow rebinding)
- Retry logic is implemented (3 attempts with 1-second delays)

### Recent Changes Made:

1. **Enhanced Debug Logging** (COMPLETED):
   - Added comprehensive DEBUG, INFO, WARNING, and ERROR level logging
   - Socket operations now log detailed state information
   - Bind attempts show retry counts and errno values
   - State transitions are tracked and logged
   - Added helpful hints for troubleshooting (e.g., netstat command)

2. **Socket Binding Improvements** (COMPLETED):
   - Added SO_REUSEPORT option for Linux (allows multiple processes to bind)
   - Implemented retry logic (3 attempts with 1-second delays)
   - Better error handling with saved errno values
   - Shutdown before copyover in act.wizard.c

3. **Code Fixes Applied**:
   - Fixed INVALID_SOCKET definition
   - Fixed one_argument function call signature
   - Fixed is_discord_bridge_active return statement
   - Fixed ACMD macro usage in header file
   - Added discord_bridge.h include in interpreter.c
   - Fixed color code processing (using CCYEL/CCNRM macros)

## Next Steps to Test:

1. **Recompile the MUD**:
   ```bash
   cd /home/luminari
   make -j20
   ```

2. **Monitor the logs during startup**:
   ```bash
   tail -f /var/log/syslog | grep -i discord
   ```

3. **Test the Discord bridge**:
   - In game: type `discord` to check status
   - Check if port 8181 is listening: `netstat -tulpn | grep 8181`

4. **Test copyover**:
   - Do a copyover in game
   - Watch the logs for shutdown/restart messages
   - Check if Discord bridge initializes properly

## Expected Log Output with New Debug Code:

### Successful Initialization:
```
DEBUG: init_discord_bridge() called
DEBUG: Discord bridge structure allocated at 0x...
DEBUG: Auth token set (length=0)
DEBUG: Loading Discord configuration...
DEBUG: Adding default Discord channels...
INFO: Discord bridge configuration loaded with 3 channels
  Channel 1: MUD='gossip' Discord='gossip' SCMD=... Enabled=1
  Channel 2: MUD='auction' Discord='auction' SCMD=... Enabled=1
  Channel 3: MUD='gratz' Discord='gratz' SCMD=... Enabled=1
INFO: Starting Discord bridge server on port 8181...
DEBUG: start_discord_server() called with port 8181
DEBUG: Socket created successfully, fd=...
DEBUG: SO_REUSEADDR set successfully
DEBUG: SO_REUSEPORT set successfully
DEBUG: Socket set to non-blocking mode
DEBUG: Attempting to bind socket ... to 0.0.0.0:8181
DEBUG: Bind attempt 1 of 3
SUCCESS: Socket bound to port 8181 on attempt 1
DEBUG: Setting socket to listen mode...
SUCCESS: Socket listening on port 8181
DEBUG: Discord bridge state set to LISTENING (1)
SUCCESS: Discord bridge server started on port 8181
INFO: Discord bridge ready - server_socket=..., state=1
```

### Failed Initialization (What we're currently seeing):
```
DEBUG: init_discord_bridge() called
...
DEBUG: Attempting to bind socket ... to 0.0.0.0:8181
DEBUG: Bind attempt 1 of 3
DEBUG: Bind failed with errno=98 (Address already in use)
WARNING: Port 8181 is in use (EADDRINUSE), will retry in 1 second(s)... (2 retries left)
DEBUG: Bind attempt 2 of 3
DEBUG: Bind failed with errno=98 (Address already in use)
WARNING: Port 8181 is in use (EADDRINUSE), will retry in 1 second(s)... (1 retries left)
DEBUG: Bind attempt 3 of 3
DEBUG: Bind failed with errno=98 (Address already in use)
ERROR: Discord bridge bind failed after all retries: Address already in use (errno=98)
ERROR: Port 8181 remains in use after 3 attempts
HINT: Check if another process is using port 8181 with: netstat -tulpn | grep 8181
ERROR: Failed to start Discord bridge server on port 8181
DEBUG: Discord bridge structure freed, ptr set to NULL
```

## Potential Root Causes to Investigate:

1. **Socket not properly closed before exec()**: The shutdown_discord_bridge() might not be called early enough before copyover
2. **File descriptor inheritance**: The socket might be inherited by the new process after exec()
3. **SO_REUSEPORT behavior**: May not work as expected when the same process tries to rebind
4. **Timing issue**: The old socket might not be fully released by the kernel

## Alternative Solutions to Consider:

1. **Use a different port after copyover** (not ideal but would work)
2. **Add FD_CLOEXEC flag** to ensure socket is closed on exec()
3. **Delay Discord bridge initialization** after copyover (give kernel time to release port)
4. **Use Unix domain sockets** instead of TCP (avoids port binding issues)

## Files Modified:
- `/src/discord_bridge.c` - Core implementation with enhanced logging
- `/src/discord_bridge.h` - Header file with proper declarations
- `/src/act.wizard.c` - Added shutdown before copyover
- `/src/interpreter.c` - Added discord_bridge.h include
- `/src/comm.c` - Calls init_discord_bridge() during startup

## Related Documentation:
- `/docs/systems/DISCORD_CHAT.md` - System documentation
- `/docs/deployment/DISCORD_BRIDGE_SETUP.md` - Setup guide
- GitHub Repository: https://github.com/LuminariMUD/discord-mud-chat