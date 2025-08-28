# LuminariMUD Quick Start Guide

## 🚀 5-Minute Setup

Get LuminariMUD running from a fresh clone in under 5 minutes!

### Prerequisites
- Linux/Unix system (Ubuntu, Debian, CentOS, WSL, macOS)
- Git installed
- Internet connection for dependency installation

## Installation Options

### Option 1: Simplest Setup (Recommended for Beginners)
Perfect for getting started quickly:

```bash
# Clone and run
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source
./scripts/simple_setup.sh

# Start the MUD
./bin/circle -d lib
```

**That's it!** Connect to `localhost:4000` with any MUD client.

### Option 2: Automated Setup (No Database)
More options, still fast:

```bash
# Clone and run
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source
./scripts/deploy.sh --quick --skip-db --init-world

# Start the MUD
./bin/circle -d lib
```

### Option 3: Standard Setup (With Database)
For full features including persistent data:

```bash
# Clone repository
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source

# Run interactive setup
./scripts/deploy.sh --init-world

# Follow the prompts to:
# - Install dependencies
# - Configure database
# - Build the MUD
# - Initialize world data

# Start the server
./start_mud.sh
```

### Option 4: Developer Setup
For contributors and developers:

```bash
# Clone and setup with debug build
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source
./scripts/deploy.sh --dev --init-world

# Start with debugging
./debug_game.sh
```

## Deployment Script Options

The `deploy.sh` script supports these options:

| Option | Description |
|--------|-------------|
| `--quick` | Skip all prompts, use defaults |
| `--skip-db` | Skip database setup (run without MySQL) |
| `--skip-deps` | Skip dependency installation |
| `--init-world` | Initialize minimal world data |
| `--dev` | Development build with debug symbols |
| `--prod` | Production optimized build |
| `-h, --help` | Show help message |

### Examples:
```bash
# Minimal setup for testing
./scripts/deploy.sh --quick --skip-db --init-world

# Production server setup
./scripts/deploy.sh --prod --init-world

# Development environment
./scripts/deploy.sh --dev --skip-db --init-world
```

## Manual Build (Advanced Users)

If you prefer manual control:

```bash
# 1. Copy configuration files
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h

# 2. Build with autotools
autoreconf -fvi
./configure
make -j$(nproc)
make install

# 3. Initialize world (optional but recommended)
./scripts/deploy.sh --skip-deps --skip-db --init-world

# 4. Run
bin/circle -d lib
```

## Connecting to Your MUD

### Default Connection Info
- **Host:** localhost (or your server IP)
- **Port:** 4000
- **Protocol:** Telnet

### Recommended MUD Clients

#### Windows
- [Mudlet](https://www.mudlet.org/) - Feature-rich, cross-platform
- [MUSHclient](http://www.gammon.com.au/mushclient/) - Powerful scripting
- [zMUD/cMUD](https://www.zuggsoft.com/) - Commercial, feature-packed

#### macOS
- [Mudlet](https://www.mudlet.org/) - Best cross-platform option
- [Atlantis](https://www.riverdark.net/atlantis/) - Native macOS client

#### Linux
- [Mudlet](https://www.mudlet.org/) - Full-featured
- [TinTin++](https://tintin.sourceforge.io/) - Terminal-based
- `telnet localhost 4000` - Basic connection

#### Web/Mobile
- [Blowtorch](https://blowtorch.app/) - Android
- [MudRammer](https://mudrammer.com/) - iOS

### First Connection
1. Launch your MUD client
2. Create new connection:
   - Host: `localhost` (or server IP)
   - Port: `4000`
3. Connect and you'll see the welcome screen
4. Type `new` to create a character
5. Follow the character creation prompts

## Common Issues & Solutions

### Build Fails
```bash
# Ensure deploy script was run first
./scripts/deploy.sh --skip-db --init-world
```

### MUD Won't Start
```bash
# Check for missing world data
./scripts/deploy.sh --skip-deps --skip-db --init-world
```

### MySQL Errors
```bash
# Run without database
./scripts/deploy.sh --quick --skip-db --init-world
```

### Script Not Found
```bash
# Deploy script moved to scripts/ directory
./scripts/deploy.sh  # NOT ./deploy.sh
```

## Next Steps

### For Players
1. Create your first character
2. Type `help newbie` for beginner tips
3. Explore the starting area
4. Join the community Discord

### For Builders
1. Create an immortal character
2. Read `docs/world_game-data/builder_manual.md`
3. Learn OLC commands with `help olc`
4. Start building your zones

### For Developers
1. Read `docs/guides/DEVELOPER_GUIDE_AND_API.md`
2. Set up your IDE (VS Code recommended)
3. Review coding standards
4. Check open issues on GitHub

## Getting Help

### Documentation
- Main docs: `docs/` directory
- Setup guide: `docs/deployment/SETUP_AND_BUILD_GUIDE.md`
- Troubleshooting: `docs/guides/TROUBLESHOOTING_AND_MAINTENANCE.md`
- Developer guide: `docs/guides/DEVELOPER_GUIDE_AND_API.md`

### Community
- GitHub Issues: [Report bugs or request features](https://github.com/LuminariMUD/Luminari-Source/issues)
- Discord: Join our community server (link in main README)
- Forums: Visit the official forums

### Quick Commands
```bash
# View all documentation
ls -la docs/

# Get help with deployment
./scripts/deploy.sh --help

# Check system status
ps aux | grep circle

# View logs
tail -f log/syslog
```

## Success Checklist

✅ Repository cloned  
✅ Dependencies installed  
✅ MUD compiled successfully  
✅ World data initialized  
✅ Server started  
✅ Connected with client  
✅ Character created  
✅ Ready to play!

---

**Welcome to LuminariMUD!** 🎮

*Enjoy your adventures in the world of Luminari!*