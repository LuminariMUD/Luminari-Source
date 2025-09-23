# LuminariMUD Getting Started Guide

## Overview

This guide covers setting up and running LuminariMUD from source code.

### Prerequisites
- Linux/Unix system (Ubuntu, Debian, CentOS, WSL, macOS)
- Git installed
- Internet connection for dependency installation

## Installation

### Automated Setup
The recommended approach for most users:

```bash
# Clone the repository
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source

# Run setup script
./scripts/setup.sh

# Start the server
./bin/circle -d lib
```

Connect to `localhost:4000` with any MUD client.

### Setup with Options
For more control over the deployment:

```bash
# Clone repository
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source

# Generate build system
autoreconf -fvi

# Run deployment with options
./scripts/deploy.sh --skip-db --init-world

# Start the server
./bin/circle -d lib
```

### Manual Setup
See the [Full Deployment Guide](deployment/DEPLOYMENT_GUIDE.md) for detailed manual setup instructions.

## Connecting to Your MUD

### MUD Clients
You can connect using any MUD client:

**Popular Clients:**
- **Mudlet** - Feature-rich, cross-platform (recommended)
- **MUSHclient** - Windows, powerful scripting
- **TinTin++** - Terminal-based, Unix/Linux
- **SimpleMU** - Windows, beginner-friendly
- **Blowtorch** - Android mobile client
- **MUDRammer** - iOS mobile client

**Basic Connection:**
- Host: `localhost` or `127.0.0.1`
- Port: `4000` (default)

### First Login
1. Create your first character (automatically gets admin privileges)
2. Choose name, password, race, and class
3. Complete character creation
4. You'll start in the default starting room

## Essential Commands

### Basic Commands
```
look / l           - Examine your surroundings
north/south/etc    - Movement (or n/s/e/w/u/d)
get <item>         - Pick up an item
drop <item>        - Drop an item
inventory / i      - Check your inventory
equipment / eq     - View equipped items
who                - List online players
say <message>      - Talk to others in the room
tell <player> <msg>- Send private message
help <topic>       - Access help system
```

### Combat Commands
```
kill <target>      - Attack a mobile/player
flee               - Escape from combat
rescue <player>    - Rescue someone from combat
bash               - Attempt to knock down opponent
cast '<spell>'     - Cast a spell
```

### Character Commands
```
score              - View character statistics
affects / aff      - List active affects
practice           - See/improve skills
train              - Spend training points
levelup            - Advance when ready
save               - Force save character
quit               - Exit the game
```

## Builder Commands (Admin)

As the first created character, you have access to builder commands:

### Online Creation (OLC)
```
redit              - Room editor
oedit              - Object editor
medit              - Mobile (NPC) editor
zedit              - Zone editor
sedit              - Shop editor
trigedit           - Trigger editor
```

### Zone Management
```
zreset             - Reset your current zone
saveall            - Save all OLC changes
show zones         - List all zones
goto <room#>       - Teleport to room
```

### Admin Commands
```
advance <player> <level> - Set player level
wizhelp            - List immortal commands
shutdown           - Shutdown server
copyover           - Reboot without disconnecting players
```

## Building Your World

### Creating Your First Zone

1. **Create a Zone:**
```
zedit new 100        - Create zone 100
zedit 100 name <Zone Name>
zedit 100 top 199    - Set room range (100-199)
zedit save
```

2. **Create Rooms:**
```
redit 100            - Create/edit room 100
redit name <Room Name>
redit desc           - Enter room description
redit exit north 101 - Link north to room 101
redit save
```

3. **Create Objects:**
```
oedit 100            - Create/edit object 100
oedit name sword
oedit short a steel sword
oedit long A sharp steel sword lies here.
oedit save
```

4. **Create Mobiles (NPCs):**
```
medit 100            - Create/edit mobile 100
medit name guard
medit short a city guard
medit long A guard watches the area carefully.
medit level 10
medit save
```

5. **Save Everything:**
```
saveall              - Save all OLC work
```

## Configuration

### Main Configuration Files
- `src/campaign.h` - Core game settings
- `src/mud_options.h` - Server options
- `src/vnums.h` - Virtual number assignments
- `lib/etc/config` - Runtime configuration

### Changing Port
Edit startup command:
```bash
./bin/circle -q 5000 -d lib  # Run on port 5000
```

### Database Setup (Optional)
See [Deployment Guide](deployment/DEPLOYMENT_GUIDE.md#database-configuration-optional) for MySQL/MariaDB setup.

## Troubleshooting

### Common Issues

**Can't connect:**
- Verify server is running: `ps aux | grep circle`
- Check firewall settings
- Try `telnet localhost 4000` to test

**Build errors:**
- Ensure all dependencies installed
- Run `autoreconf -fvi` if Makefile missing
- Check `log/syslog` for errors

**Missing files:**
- Run `./scripts/setup.sh` to create all required files
- Check symlinks exist: `ls -la world text etc`

## Getting Help

- **In-game help:** Type `help <topic>`
- **Immortal help:** Type `wizhelp` (as admin)
- **Documentation:** Browse the `docs/` directory
- **Logs:** Check `log/syslog` for errors
- **GitHub:** [Report issues](https://github.com/LuminariMUD/Luminari-Source/issues)

## Next Steps

1. **Explore the codebase:** Review source files in `src/`
2. **Read documentation:** 
   - [Developer Guide](guides/DEVELOPER_GUIDE_AND_API.md) - Code development
   - [Builder Manual](world_game-data/builder_manual.md) - World building
3. **Start building:** Create your own zones and content
4. **Join the community:** Connect with other MUD developers

---

*For complete deployment details, see the [Deployment Guide](deployment/DEPLOYMENT_GUIDE.md)*