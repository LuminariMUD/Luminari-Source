# Intermud3 System Documentation

## Overview

The Intermud3 (I3) system enables LuminariMUD to connect to the global Intermud3 network, allowing players to communicate across different MUDs. This implementation uses a client-gateway architecture where LuminariMUD connects to an I3 Gateway service that handles the complex I3 protocol.

## Architecture

```
[LuminariMUD Client] <--JSON-RPC--> [I3 Gateway] <--I3 Protocol--> [Global I3 Network]
```

### Components

1. **I3 Client** (`src/net/i3_client.c`)

   - Manages connection to the I3 Gateway
   - Handles message queuing and threading
   - Processes incoming/outgoing messages

2. **I3 Commands** (`src/net/i3_commands.c`)

   - Implements player-facing commands
   - Handles admin functions
   - Manages configuration

3. **I3 Gateway** (External service)

   - Runs on localhost:8081 by default
   - Handles I3 protocol complexity
   - Available at: https://github.com/LuminariMUD/Intermud3

## Installation & Configuration

### Prerequisites

- JSON-C library: `sudo apt-get install libjson-c-dev`
- pthread support (included with system)
- Running I3 Gateway service

### Configuration File

Location: `lib/i3_config`

```
# Intermud3 Configuration
gateway_host localhost
gateway_port 8081
api_key YOUR-API-KEY-HERE
mud_name LuminariMUD
default_channel intermud
enable_tell 1
enable_channels 1
enable_who 1
auto_reconnect 1
reconnect_delay 30
max_queue_size 1000
```

**Note:** The actual `i3_config` file is excluded from git. Use `i3_config.example` as a template.

### Starting the I3 Gateway

```bash
# Clone and start the gateway
git clone https://github.com/LuminariMUD/Intermud3
cd Intermud3
npm install
npm start
```

## Player Commands

| Command | Alias | Description | Example |
| -- | -- | -- | -- |
| `i3tell` | `i3t` | Send tell to player on another MUD | `i3tell Bob@TestMUD Hello!` |
| `i3chat` | `i3c` | Send message to I3 channel | `i3chat Hello everyone!` |
| `i3who` | `i3w` | List players on a MUD | `i3who TestMUD` |
| `i3finger` | `i3f` | Get info about a player | `i3finger Bob@TestMUD` |
| `i3locate` | `i3l` | Find player on network | `i3locate Bob` |
| `i3mudlist` | `i3m` | List all connected MUDs | `i3mudlist` |
| `i3channels` | `i3chan` | Manage channel subscriptions | `i3channels list` |
| `i3config` | `i3conf` | Configure I3 settings | `i3config` |

### Channel Management

```
i3channels list              # List available channels
i3channels join <channel>    # Join a channel
i3channels leave <channel>   # Leave a channel
```

### Personal Configuration

```
i3config                     # Show current settings
i3config tells               # Toggle I3 tells on/off
i3config channels            # Toggle I3 channels on/off
i3config who                 # Toggle I3 who queries on/off
```

## Admin Commands

**Command:** `i3admin` (Implementor only)

| Subcommand | Description |
| -- | -- |
| `i3admin status` | Show connection status and details |
| `i3admin stats` | Display message statistics |
| `i3admin reconnect` | Force reconnection to gateway |
| `i3admin reload` | Reload configuration file |
| `i3admin save` | Save current configuration |

## Technical Implementation

### File Structure

```
src/net/
i3_client.h      # Header with structures and declarations
i3_client.c      # Core client implementation
i3_commands.c    # Command handlers

lib/
i3_config        # Active configuration (git-ignored)
i3_config.example # Example configuration
```

### Integration Points

1. **Build System**

   - `Makefile.am`: Added to luminari_SOURCES
   - `CMakeLists.txt`: Added to SRC_C_FILES

2. **Command Registration**

   - `interpreter.c`: Commands registered in cmd_info[]
   - All commands available at POS_DEAD

3. **Game Loop Integration**

   - `comm.c`: Initialization in init_game()
   - `comm.c`: Event processing in heartbeat()
   - `comm.c`: Shutdown handling

### Threading Model

The I3 client runs in a separate thread to prevent blocking:

- Main thread: Game logic and player commands
- I3 thread: Network I/O and message processing
- Communication via thread-safe queues

### Message Flow

1. **Outgoing Messages:**

   ```
   Player Command -> Command Handler -> Queue Command -> I3 Thread -> Gateway
   ```

2. **Incoming Messages:**

   ```
   Gateway -> I3 Thread -> Queue Event -> Process Events -> Send to Players
   ```

### Memory Management

- Dynamic allocation for client structure
- Thread-safe queue management
- Proper cleanup on shutdown
- JSON object lifecycle management

## Protocol Details

### Communication with Gateway

- Protocol: JSON-RPC over TCP
- Port: 8081 (configurable)
- Authentication: API key-based (format: `API_KEY_LUMINARI:luminari-i3-gateway-2025`)
- Heartbeat: Every 30 seconds
- Handshake: Waits for gateway welcome message before authenticating

### Message Types

| Type | Direction | Description |
| -- | -- | -- |
| welcome | In | Gateway welcome message (triggers auth) |
| authenticate | Out | Initial authentication |
| authenticated | In | Authentication success response |
| tell | Both | Private messages |
| channel_send | Out | Channel messages |
| channel_message | In | Received channel messages |
| who_request | Out | Request player list |
| who_reply | In | Player list response |
| mudlist_request | Out | Request MUD list |
| mudlist_reply | In | MUD list response |

## Error Handling

### Connection Issues

- Automatic reconnection with exponential backoff
- Queue preservation during disconnects
- Graceful degradation when gateway unavailable

### Common Problems

| Issue | Solution |
| -- | -- |
| "I3 network unavailable" | Check gateway is running |
| "Failed to connect" | Verify gateway host/port |
| "Authentication failed" | Check API key in config (format: `API_KEY_LUMINARI:key`) |
| "Could not load I3 configuration" | Update `lib/i3_config` with valid API key |
| No authentication after connect | Fixed: Now waits for welcome message |
| Messages not received | Verify i3config settings |

## Performance Considerations

- **CPU Usage:** < 1% typical overhead
- **Memory:** ~5MB for client and queues
- **Network:** Minimal bandwidth usage
- **Threading:** Non-blocking I/O prevents lag

## Security

### Input Validation

- All user input sanitized
- Message length limits enforced
- Command injection prevention

### Configuration Security

- API key stored in git-ignored file
- No sensitive data in repository
- Player preference controls

### Network Security

- Local gateway connection (no external exposure)
- Gateway handles external I3 protocol
- Rate limiting at gateway level

## Monitoring

### Logs

- I3 events logged to MUD syslog
- Connection status in `i3admin status`
- Statistics via `i3admin stats`

### Health Checks

```bash
# Check connection status
i3admin status

# Monitor message flow
i3admin stats

# Test connectivity
i3tell TestUser@TestMUD ping
```

## Development Guidelines

### Adding New Commands

1. Define command in `i3_client.h`
2. Implement handler in `i3_commands.c`
3. Register in `interpreter.c`
4. Update this documentation

### Code Standards

- C89/ANSI C compliance required
- No C99 features (declarations at block start)
- Use UNUSED_VAR() for unused parameters
- Follow LuminariMUD coding conventions

### Testing Checklist

- [ ] Compiles without warnings
- [ ] Gateway connection established
- [ ] Tell command works both ways
- [ ] Channel messages work
- [ ] MUD list retrieval works
- [ ] Reconnection after disconnect
- [ ] Clean shutdown

## Troubleshooting

### Debug Logging

The I3 client includes comprehensive debug logging when issues occur:

- Connection attempts and results
- Authentication flow and API key usage
- All sent and received messages
- JSON parsing and processing

Check the MUD syslog for lines containing "I3: DEBUG" for detailed diagnostics.

### Gateway Not Responding

1. Check gateway is running: `ps aux | grep node`
2. Verify port is open: `netstat -an | grep 8081`
3. Check firewall settings
4. Review gateway logs

### Messages Not Delivering

1. Check `i3admin status` for connection
2. Verify recipient MUD is online: `i3mudlist`
3. Check player exists: `i3finger Player@MUD`
4. Verify i3config settings enabled

### Compilation Issues

1. Ensure json-c installed: `pkg-config --libs json-c`
2. Check pthread linking in Makefile
3. Verify all files in build system
4. Clean and rebuild: `make clean && make`

## Future Enhancements

### Planned Features

- [ ] Channel moderation tools
- [ ] Ignore list for players/MUDs
- [ ] Message history/replay
- [ ] Channel logging options
- [ ] Web interface integration

### Protocol Extensions

- [ ] File transfer support
- [ ] MUD information exchange
- [ ] Cross-MUD mail system
- [ ] Shared ban lists

## References

- I3 Gateway Repository: https://github.com/LuminariMUD/Intermud3
- I3 Protocol Specification: Available in gateway docs
- LPMuds I3 Network: Historical I3 documentation

## Maintenance

### Regular Tasks

- Monitor gateway health
- Update API keys periodically
- Review message statistics
- Check for gateway updates

### Upgrade Procedure

1. Stop MUD server
2. Update gateway if needed
3. Update client code
4. Test in development
5. Deploy to production

## Contact

For I3 network issues or gateway access, contact the MUD development team or check the community forums for current I3 network administrators.

---

*Last Updated: Implementation completed for LuminariMUD*
*System Version: 1.0*
*Compatible with: I3 Gateway 1.x*
