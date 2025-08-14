# Discord Chat Bridge System Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Installation & Setup](#installation--setup)
4. [Configuration](#configuration)
5. [Usage](#usage)
6. [Protocol Specification](#protocol-specification)
7. [Security](#security)
8. [Troubleshooting](#troubleshooting)
9. [API Reference](#api-reference)
10. [Development Guide](#development-guide)

## Overview

The Discord Chat Bridge system enables bidirectional real-time communication between LuminariMUD players and Discord users. This allows players to participate in MUD channels through Discord and vice versa, fostering community engagement across platforms.

### Key Features
- **Bidirectional Communication**: Messages flow seamlessly between MUD and Discord
- **Channel Mapping**: Configure which MUD channels bridge to Discord channels
- **Permission Aware**: Respects existing MUD channel permissions
- **Secure**: Input validation, connection limiting, and injection prevention
- **Performant**: Non-blocking I/O integrated with MUD's event loop
- **Resilient**: Automatic reconnection support

## Architecture

### System Components

```
+------------------+         TCP Socket         +------------------+
|                  |         Port 8181          |                  |
|   LuminariMUD    |<-------------------------->|  Discord Bot     |
|  (TCP Server)    |         JSON/TCP           |  (TCP Client)    |
|                  |                             |                  |
+------------------+                             +------------------+
        |                                                |
        |                                                |
        v                                                v
+------------------+                             +------------------+
|  MUD Players     |                             |  Discord Users   |
+------------------+                             +------------------+
```

### Data Flow

1. **MUD -> Discord**:
   - Player sends message on MUD channel (e.g., `gossip hello`)
   - MUD captures message in `do_gen_comm()` 
   - Routes through `route_mud_to_discord()`
   - Builds JSON message with channel, name, and content
   - Sends over TCP socket to Discord bot
   - Discord bot posts to appropriate Discord channel

2. **Discord -> MUD**:
   - Discord user posts in bridged channel
   - Discord bot captures message
   - Builds JSON with channel, username, and content
   - Sends over TCP socket to MUD
   - MUD parses JSON in `receive_from_discord()`
   - Routes through `route_discord_to_mud()`
   - Broadcasts to players on that MUD channel

## Installation & Setup

### Prerequisites
- LuminariMUD compiled with Discord bridge support
- Discord bot with appropriate permissions
- Network connectivity between MUD and Discord bot

### MUD-Side Setup

1. **Compile with Discord Bridge**:
   ```bash
   autoreconf -fiv
   ./configure
   make -j20
   ```

2. **Start the MUD**:
   ```bash
   ./autorun.sh
   ```

3. **Enable Discord Bridge** (as admin):
   ```
   discord start
   ```

4. **Verify Status**:
   ```
   discord status
   ```

### Discord Bot Setup

**Official Discord Bot Repository**: https://github.com/LuminariMUD/discord-mud-chat

The official LuminariMUD Discord bridge bot is available at the repository above. It provides:
1. TCP client that connects to MUD on port 8181
2. JSON message parsing and building
3. Discord channel mapping
4. Automatic reconnection support
5. Configuration via environment variables

#### Quick Setup with Official Bot:
```bash
# Clone the repository
git clone https://github.com/LuminariMUD/discord-mud-chat.git
cd discord-mud-chat

# Install dependencies
npm install

# Configure environment variables (see repository README)
cp .env.example .env
# Edit .env with your settings

# Run the bot
npm start
```

#### Manual Implementation Example (Python):
```python
import socket
import json

# Connect to MUD
mud_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
mud_socket.connect(('mud.example.com', 8181))

# Optional: Send authentication if required
auth_token = "your-secret-token"  # Load from config
if auth_token:
    auth_msg = {
        'channel': 'auth',
        'name': 'bot',
        'message': auth_token
    }
    mud_socket.send((json.dumps(auth_msg) + '\n').encode('utf-8'))

# Send regular message to MUD
message = {
    'channel': 'gossip',
    'name': 'DiscordUser',
    'message': 'Hello from Discord!'
}
mud_socket.send((json.dumps(message) + '\n').encode('utf-8'))

# Receive from MUD
data = mud_socket.recv(4096).decode('utf-8')
mud_message = json.loads(data.strip())
```

## Configuration

### System Limits

| Parameter | Value | Description |
|-----------|-------|-------------|
| MAX_INPUT_LENGTH | 65535 | Maximum message length in characters |
| Rate Limit | 10 msg/sec | Per-channel message rate limit |
| Connection Timeout | 300 seconds | Idle connection timeout |
| Reconnect Attempts | 5 | Maximum reconnection attempts |
| Reconnect Delay | 30 seconds | Delay between reconnection attempts |

### Default Channel Mappings

| MUD Channel | Discord Channel | Status  | Description |
|------------|----------------|---------|-------------|
| gossip     | gossip         | Enabled | General chat |
| auction    | auction        | Enabled | Trade channel |
| gratz      | gratz          | Enabled | Congratulations |

### Adding New Channels

To add a new channel mapping, modify the `load_discord_config()` function in `discord_bridge.c`:

```c
add_discord_channel("newchannel", "discord-channel", SCMD_NEWCHANNEL, 1);
```

### Rate Limiting Configuration

To configure rate limiting per channel, add to `discord_bridge.c`:

```c
// Rate limiting structure
struct rate_limit {
    int message_count;
    time_t window_start;
    int max_messages;     // 10 messages
    int window_seconds;   // per 1 second
};

// Check rate limit before sending
int check_rate_limit(struct rate_limit *rl) {
    time_t now = time(NULL);
    
    // Reset window if expired
    if (now - rl->window_start >= rl->window_seconds) {
        rl->message_count = 0;
        rl->window_start = now;
    }
    
    // Check if limit exceeded
    if (rl->message_count >= rl->max_messages) {
        return 0; // Rate limit exceeded
    }
    
    rl->message_count++;
    return 1; // OK to send
}
```

### Authentication Configuration

To enable authentication, modify `init_discord_bridge()` in `discord_bridge.c`:

```c
/* Set authentication token (empty string = no auth required) */
strcpy(discord_bridge->auth_token, "your-secret-token-here");
```

Or load from a configuration file:
```c
/* Load token from lib/discord_auth.txt */
FILE *fp = fopen("lib/discord_auth.txt", "r");
if (fp) {
    fscanf(fp, "%63s", discord_bridge->auth_token);
    fclose(fp);
}
```

### Runtime Configuration

Administrators can manage the Discord bridge using these commands:

- `discord start` - Start the Discord bridge server
- `discord stop` - Stop the Discord bridge server  
- `discord status` - View connection status and statistics

## Usage

### For MUD Players

Messages from Discord appear with a `[Discord]` prefix:
```
[Discord] JohnDoe: Hello from Discord!
```

Regular channel commands work normally:
```
gossip Hello Discord users!
auction Selling +5 sword of awesome
gratz Congratulations on level 50!
```

### For Discord Users

Simply type in the configured Discord channels. Messages appear in the MUD as:
```
[Discord] DiscordUser: Message content
```

### Channel Permissions

- MUD players must have the channel enabled (not set to NOGOSS, etc.)
- Discord messages respect MUD channel permissions
- Players can toggle channels with standard MUD commands

## Protocol Specification

### Message Format

All messages use JSON encoding with newline delimiters.

#### MUD to Discord
```json
{
    "channel": "gossip",
    "name": "PlayerName", 
    "message": "Message content",
    "emoted": 0
}
```

#### Discord to MUD
```json
{
    "channel": "gossip",
    "name": "DiscordUser",
    "message": "Message content"
}
```

### Field Specifications

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| channel | string | Yes | Channel identifier |
| name | string | Yes | Sender's name |
| message | string | Yes | Message content |
| emoted | integer | No | 1 for emotes, 0 for normal |

### Connection Protocol

1. Discord bot connects to MUD TCP server
2. (Optional) Send authentication if required
3. Connection remains persistent
4. Messages are newline-delimited JSON
5. Reconnection on disconnect (bot responsibility)

### Authentication Protocol (Optional)

If authentication is enabled (auth token set in MUD), the Discord bot must authenticate before sending messages:

1. **First Message** must be authentication:
   ```json
   {
       "channel": "auth",
       "name": "bot",
       "message": "your-secret-token-here"
   }
   ```

2. **Success**: Connection remains open, bot can send messages
3. **Failure**: Connection is immediately closed
4. **No Token Set**: Authentication not required (default)

## Security

### Implementation Security Features

1. **Connection Limiting**
   - Only one Discord bridge connection allowed
   - Additional connections are rejected

2. **Input Validation**
   - Empty fields rejected
   - Username sanitization (no @, !, # characters)
   - Message length limits (MAX_INPUT_LENGTH = 65535 characters)
   - Rate limiting (10 messages per second per channel)

3. **Injection Prevention**
   - Discord usernames prefixed with `[Discord]`
   - Special characters filtered
   - MUD commands cannot be executed via Discord

4. **Network Security**
   - Non-blocking sockets prevent DoS
   - Graceful error handling
   - Connection timeout management (5 minutes idle)
   - Automatic disconnection on timeout

5. **Authentication** (Optional):
   - Token-based authentication support
   - First message must authenticate if enabled
   - Unauthenticated connections rejected

### Recommended Security Practices

1. **Firewall Configuration**:
   ```bash
   # Allow only Discord bot IP
   iptables -A INPUT -p tcp --dport 8181 -s DISCORD_BOT_IP -j ACCEPT
   iptables -A INPUT -p tcp --dport 8181 -j DROP
   ```

2. **Monitor Logs**:
   - Check for connection attempts
   - Review error messages
   - Track message statistics

3. **Rate Limiting**:
   - Per-channel limit: 10 messages per second
   - Automatic throttling when limit exceeded
   - Configurable limits per channel

## Troubleshooting

### Common Issues and Solutions

#### Discord Bridge Won't Start

**Symptoms**: `discord start` fails or shows errors

**Solutions**:
1. Check if port 8181 is already in use:
   ```bash
   netstat -an | grep 8181
   ```
2. Verify Discord bridge compiled correctly:
   ```bash
   grep discord_bridge src/Makefile
   ```
3. Check system logs for errors

#### Discord Bot Can't Connect

**Symptoms**: Bot connection fails or times out

**Solutions**:
1. Verify MUD is listening:
   ```bash
   telnet mud.example.com 8181
   ```
2. Check firewall rules
3. Confirm correct IP/port in bot config
4. Use `discord status` to check server state

#### Messages Not Appearing in MUD

**Symptoms**: Discord messages don't show in MUD

**Possible Causes**:
1. Channel mapping incorrect
2. Channel disabled in config
3. JSON format error
4. Players have channel turned off

**Debug Steps**:
1. Check MUD logs for JSON parsing errors
2. Verify channel configuration with `discord status`
3. Test with a player who has all channels enabled
4. Monitor raw TCP traffic with tcpdump

#### Messages Not Reaching Discord

**Symptoms**: MUD messages don't appear in Discord

**Solutions**:
1. Verify Discord bot is connected: `discord status`
2. Check if message contains `[Discord]` (loop prevention)
3. Ensure channel is enabled in configuration
4. Review Discord bot logs

#### High Latency or Delays

**Symptoms**: Noticeable delay between sending and receiving

**Solutions**:
1. Check network latency between MUD and bot
2. Monitor server CPU/memory usage
3. Review message queue sizes
4. Consider implementing message batching

### Debug Commands

For administrators to diagnose issues:

```
discord status     - View current connection and statistics
```

#### Status Command Output

The `discord status` command displays:
- Server socket state (Active/Inactive)
- Client connection state (Connected/Disconnected)
- Authentication status (Disabled/Authenticated/Not Authenticated)
- Messages sent/received counters
- Messages dropped due to rate limiting
- Connection duration (if connected)
- Time since last activity (if connected)
- List of configured channels with their status

## API Reference

### Core Functions

#### `init_discord_bridge()`
Initializes the Discord bridge system, creates server socket, loads configuration.

#### `shutdown_discord_bridge()`
Cleanly shuts down the Discord bridge, closes all connections, saves configuration.

#### `send_to_discord(channel, name, message, emoted)`
Sends a message from MUD to Discord.

**Parameters**:
- `channel`: Target Discord channel name
- `name`: Sender's name
- `message`: Message content
- `emoted`: 1 for emote, 0 for normal

#### `route_discord_to_mud(channel, name, message)`
Routes an incoming Discord message to appropriate MUD channel.

#### `route_mud_to_discord(subcmd, ch, message, emoted)`
Routes a MUD channel message to Discord.

### Configuration Functions

#### `add_discord_channel(mud_channel, discord_name, scmd, enabled)`
Adds a new channel mapping configuration.

#### `find_discord_channel_by_scmd(scmd)`
Finds channel configuration by MUD subcmd.

### Utility Functions

#### `strip_mud_colors(text)`
Removes MUD color codes from text before sending to Discord.

#### `is_discord_bridge_active()`
Returns true if Discord bridge is connected and active.

## Development Guide

### Adding New Features

#### Adding a New Channel

1. Define the channel SCMD in `act.h`:
   ```c
   #define SCMD_NEWCHANNEL 10
   ```

2. Add to channel configuration in `load_discord_config()`:
   ```c
   add_discord_channel("newchannel", "discord-new", SCMD_NEWCHANNEL, 1);
   ```

3. Map the PRF flag in `route_discord_to_mud()`:
   ```c
   case SCMD_NEWCHANNEL:
       channel_flag = PRF_NONEWCHANNEL;
       break;
   ```

#### Authentication Implementation

Authentication is now fully implemented in the Discord bridge:

1. **Configuration**: Set `auth_token` in `discord_bridge_data`:
   ```c
   strcpy(discord_bridge->auth_token, "your-secret-token");
   // Empty string "" means no authentication required
   ```

2. **Bot Authentication**: Send as first message:
   ```python
   # Python example
   auth_msg = {
       "channel": "auth",
       "name": "bot",
       "message": "your-secret-token"
   }
   mud_socket.send((json.dumps(auth_msg) + '\n').encode('utf-8'))
   ```

3. **Verification**: The MUD automatically:
   - Checks first message for auth token
   - Sets authenticated flag on success
   - Closes connection on failure
   - Rejects all messages if not authenticated

#### Adding TLS/SSL Support

For encrypted communication:

1. Include OpenSSL headers
2. Wrap socket with SSL context
3. Use SSL_read/SSL_write instead of recv/send
4. Update connection handling

### Code Structure

```
src/
|-- discord_bridge.c     # Core implementation
|-- discord_bridge.h     # Header with definitions
|-- comm.c              # Integration with game loop
|-- act.comm.c          # Channel message hooks
+-- interpreter.c       # Command registration
```

### Testing Checklist

- [ ] TCP server starts on port 8181
- [ ] Accepts Discord bot connection
- [ ] Parses JSON messages correctly
- [ ] Routes to correct MUD channels
- [ ] Sends MUD messages to Discord
- [ ] Handles disconnections gracefully
- [ ] Reconnection works properly
- [ ] Channel permissions respected
- [ ] Message formatting correct
- [ ] No memory leaks
- [ ] Performance acceptable
- [ ] **Message Length Boundaries**:
  - [ ] Accepts messages up to 65535 characters
  - [ ] Rejects messages over 65535 characters gracefully
  - [ ] Handles empty messages correctly
  - [ ] Processes single character messages
- [ ] **Rate Limiting**:
  - [ ] Enforces 10 messages per second limit per channel
  - [ ] Throttles excess messages without dropping connection
  - [ ] Rate limit resets properly after time window
  - [ ] Independent rate limits for each channel

### Performance Considerations

- Non-blocking I/O prevents game loop blocking
- Buffer sizes optimized for typical message lengths
- Efficient JSON parsing without external libraries
- Minimal overhead in message routing

### Future Enhancements

1. **Rich Message Support**
   - Embed support for Discord
   - Markdown formatting
   - Attachments/images

2. **Command Bridge**
   - Execute MUD commands from Discord
   - Query game state
   - Administrative functions

3. **Presence Sync**
   - Show online players in Discord
   - Player status updates
   - Who list integration

4. **Advanced Security**
   - TLS encryption
   - OAuth authentication
   - Rate limiting per user

5. **Statistics & Monitoring**
   - Message throughput graphs
   - Latency monitoring
   - Error rate tracking

## Support

For issues or questions about the Discord bridge system:

1. Check this documentation
2. Review MUD logs for errors
3. Consult Discord bot logs
4. Contact MUD administrators

## Version History

- **1.1.0** (2025-01): Enhanced security and monitoring
  - Token-based authentication support
  - Per-channel rate limiting (10 msg/sec)
  - Connection timeout handling (5 minutes)
  - Enhanced status display with statistics
  - Message drop tracking for rate limits
- **1.0.0** (2025-01): Initial implementation
  - Basic bidirectional messaging
  - Channel mapping system
  - Security features
  - Integration with game loop

## License

This Discord bridge system is part of LuminariMUD and subject to its licensing terms.