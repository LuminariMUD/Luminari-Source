# Discord Bridge Setup Guide

## MUD-Side Setup Complete!

The Discord bridge has been fully integrated into LuminariMUD. Here's what's already configured:

### What's Already Done:
- **TCP Server**: Automatically starts on port 8181 when MUD starts
- **Command Integration**: `discord` command available for admins (LVL_IMPL)
- **Channel Hooks**: Gossip, auction, and gratz channels automatically bridge
- **Game Loop Integration**: Discord processing happens every game tick
- **Automatic Startup**: Bridge initializes when MUD starts

## Quick Start Guide

### 1. Start the MUD
```bash
./autorun.sh
```
The Discord bridge TCP server automatically starts on port 8181.

### 2. Verify Discord Bridge Status (In-Game)
As an admin (LVL_IMPL), type:
```
discord status
```

You should see:
- Server Socket: Active
- Client Connection: Disconnected (until bot connects)
- Configured channels list

### 3. Optional: Manual Control
The bridge starts automatically, but admins can control it:
```
discord stop    # Stop the bridge
discord start   # Start the bridge
discord status  # Check status
```

## Discord Bot Setup

### Official Discord Bridge Bot

**GitHub Repository**: https://github.com/LuminariMUD/discord-mud-chat

The official LuminariMUD Discord bridge bot is ready to use! It provides all required features:
- TCP client connection to MUD port 8181
- JSON message protocol implementation
- Channel mapping configuration
- Automatic reconnection on disconnect
- Environment-based configuration
- Docker support

### Quick Setup with Official Bot:

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/LuminariMUD/discord-mud-chat.git
   cd discord-mud-chat
   ```

2. **Install Dependencies**:
   ```bash
   npm install
   ```

3. **Configure the Bot**:
   ```bash
   cp .env.example .env
   # Edit .env with your settings:
   # - Discord bot token
   # - MUD host/port (default: localhost:8181)
   # - Discord channel IDs
   ```

4. **Create Discord Application**:
   - Go to https://discord.com/developers
   - Create new application and bot
   - Copy bot token to .env file
   - Invite bot to your server with message permissions

5. **Run the Bot**:
   ```bash
   npm start
   # Or with PM2 for production:
   pm2 start ecosystem.config.js
   # Or with Docker:
   docker-compose up -d
   ```

For detailed setup instructions, see the repository README at https://github.com/LuminariMUD/discord-mud-chat

## Configuration

### Default Channel Mappings (Already Configured):
| MUD Channel | Discord Channel | Status |
|-------------|-----------------|---------|
| gossip      | gossip         | Enabled |
| auction     | auction        | Enabled |
| gratz       | gratz          | Enabled |

### Security Features (Already Active):
- **Rate Limiting**: 10 messages per second per channel
- **Connection Limit**: Only 1 Discord bot connection allowed
- **Message Length**: Max 65535 characters
- **Idle Timeout**: 5 minutes (connection drops if idle)
- **Input Sanitization**: Special characters filtered

### Optional Authentication:
To enable authentication, edit `src/discord_bridge.c`:
```c
/* Line 201 - Set a secret token */
strcpy(discord_bridge->auth_token, "your-secret-token-here");
```
Then recompile and restart MUD. Bot must send auth token as first message.

## Testing the Bridge

### From MUD Side:
1. Start MUD
2. Check `discord status` shows Server Socket: Active
3. Watch for "Discord bridge connected from..." in logs when bot connects
4. Send a message on gossip channel: `gossip Hello Discord!`
5. Message should appear in Discord

### From Discord Side:
1. Type in mapped Discord channel
2. Messages appear in MUD with channel identification and colors:
   - Gossip: `[Discord-gossip] Username: Message` (in yellow)
   - Auction: `[Discord-auction] Username: Message` (in magenta)
   - Gratz: `[Discord-gratz] Username: Message` (in green)
3. All players with channel enabled see it

## Troubleshooting

### Discord Bot Can't Connect:
- Check firewall allows port 8181
- Verify MUD is running: `ps aux | grep circle`
- Check server listens: `netstat -an | grep 8181`
- Try telnet test: `telnet localhost 8181`

### Messages Not Bridging:
- Use `discord status` to check connection
- Verify channels are enabled in status output
- Check player has channel on (not set to NOGOSS, etc.)
- Look for errors in MUD syslog

### Connection Drops:
- Bot must send data within 5 minutes or timeout occurs
- Implement heartbeat/keepalive in bot
- Bot should auto-reconnect on disconnect

## Network Requirements

### Firewall Rules:
If MUD and bot on different machines, allow TCP port 8181:
```bash
# Ubuntu/Debian
sudo ufw allow 8181/tcp

# Or iptables
sudo iptables -A INPUT -p tcp --dport 8181 -j ACCEPT
```

### Connection Info:
- **Protocol**: TCP
- **Port**: 8181
- **Format**: JSON with newline delimiter
- **Binding**: 0.0.0.0 (accepts any interface)

## Monitoring

### Log Files:
- MUD syslog shows Discord bridge events
- Look for:
  - "Discord bridge connected from..."
  - "Discord bridge disconnected"
  - JSON parsing errors
  - Rate limit messages

### Performance:
- Minimal CPU usage (non-blocking I/O)
- ~1MB RAM for bridge buffers
- Processes dozens of messages per second

## Advanced Configuration

### Adding More Channels:
Edit `src/discord_bridge.c` function `load_discord_config()`:
```c
add_discord_channel("newchannel", "discord-channel", SCMD_NEWCHANNEL, 1);
```

### Changing Port:
Edit `src/discord_bridge.h`:
```c
#define DISCORD_BRIDGE_PORT 8181  /* Change to desired port */
```

### Adjusting Rate Limits:
Edit `src/discord_bridge.h`:
```c
#define DISCORD_RATE_LIMIT_MESSAGES 10  /* Messages per window */
#define DISCORD_RATE_LIMIT_WINDOW 1     /* Window in seconds */
```

Remember to recompile after any code changes:
```bash
make -j20
```

## Summary

**The MUD side is fully ready!** You just need to:
1. Create and configure a Discord bot
2. Run the bot to connect to port 8181
3. Start chatting between MUD and Discord!

The bridge handles everything else automatically.

## Enhanced Features (v1.2.0)

### Channel-Specific Visual Design:
- **Channel Identification**: Messages show exact Discord channel source
- **Color Consistency**: Each channel uses its corresponding MUD color
- **Visual Organization**: Easy to distinguish between different channel types

### Message Format Examples:
```
[Discord-gossip] JohnDoe: Hey everyone!        (yellow text)
[Discord-auction] JaneDoe: Selling magic sword (magenta text)
[Discord-gratz] BobSmith: Congrats on the win! (green text)
```

This provides clear visual separation and maintains consistency with native MUD channel colors.