# LuminariMUD Deployment Guide

## 🔧 Remaining Issues to Address

### Minor Issues (Non-blocking)
1. **Configure script cosmetic error**: `cat: ./src/conf.h.in: No such file or directory` at the end of configure script (harmless, doesn't affect build)
2. **MySQL config template placeholders**: `lib/mysql_config_example` has placeholder values like `<host here>` instead of working defaults
3. **Start room warnings**: "Immort start room does not exist" and "Frozen start room does not exist" warnings on startup

### Future Improvements
1. **CI/CD testing**: Add automated testing to ensure deployment stays working
2. **Docker support**: Add container support for one-command deployment
3. **Health check script**: Create script to verify successful deployment
4. **Migration scripts**: Add database schema migration scripts

---

## 🚀 Quick Start

**Get LuminariMUD running in under 2 minutes!**

### Simplest Method (Recommended)
```bash
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source
./scripts/simple_setup.sh
./bin/circle -d lib
```

### Alternative Method (More Options)
```bash
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source
./scripts/deploy.sh --quick --skip-db --init-world
./bin/circle -d lib
```

Connect to `localhost:4000` with any MUD client and start playing!

---

## 📋 Table of Contents

- [System Requirements](#system-requirements)
- [Dependencies](#dependencies)
- [Deployment Options](#deployment-options)
- [Manual Setup](#manual-setup)
- [Database Configuration](#database-configuration)
- [Running the Server](#running-the-server)
- [Troubleshooting](#troubleshooting)
- [Historical Issues (Resolved)](#historical-issues-resolved)

---

## System Requirements

### Minimum Requirements
- **Operating System**: Linux (Ubuntu 18.04+, CentOS 7+, Debian 9+) or Unix-like system
- **Memory**: 512MB RAM (2GB+ recommended for production)
- **Storage**: 1GB+ free disk space
- **Network**: TCP/IP networking capability
- **Compiler**: GCC 4.8+ with ANSI C90/C89 support (NOT C99!)

### Recommended Requirements
- **Operating System**: Ubuntu 20.04+ LTS or CentOS 8+
- **Memory**: 4GB+ RAM for development, 2GB+ for production
- **Storage**: 5GB+ free disk space
- **Compiler**: GCC 9.0+ or Clang 10.0+
- **Build System**: Make (Autotools)
- **Database**: MariaDB 10.3+ (optional)

---

## Dependencies

### Ubuntu/Debian Installation (including WSL2)

```bash
# Update package list
sudo apt-get update

# Install REQUIRED dependencies
sudo apt-get install -y build-essential git make

# Optional but recommended
sudo apt-get install -y libcrypt-dev libgd-dev libmariadb-dev \
                        libcurl4-openssl-dev libssl-dev mariadb-server pkg-config

# For debugging (recommended)
sudo apt-get install -y gdb valgrind

# If you encounter Windows line endings (CRLF) issues
sudo apt-get install -y dos2unix
```

### CentOS/RHEL/Fedora Installation
```bash
# For CentOS 7/RHEL 7
sudo yum install -y gcc make git

# For CentOS 8+/RHEL 8+/Fedora
sudo dnf install -y gcc make git

# Optional but recommended
sudo dnf install -y mariadb-server mariadb-devel gd-devel \
                    libcrypt-devel autoconf automake libtool
```

---

## Deployment Options

### Option 1: Simple Setup Script (Fastest)
Best for beginners and quick testing:

```bash
./scripts/simple_setup.sh
```

This script automatically:
- Copies configuration files from examples
- Builds the game
- Creates required symlinks
- Sets up minimal world files
- Creates text files
- Zero user interaction required

### Option 2: Full Deployment Script
More control over the setup process:

```bash
# Quick setup without database
./scripts/deploy.sh --quick --skip-db --init-world

# Interactive setup with all features
./scripts/deploy.sh

# Development build with debug symbols
./scripts/deploy.sh --dev --skip-db --init-world

# Production optimized build
./scripts/deploy.sh --prod --init-world
```

#### Deployment Script Options
| Option | Description |
|--------|-------------|
| `--quick` | Skip all prompts, use defaults |
| `--skip-db` | Skip database setup (run without MySQL) |
| `--skip-deps` | Skip dependency installation |
| `--init-world` | Initialize minimal world data |
| `--dev` | Development build with debug symbols |
| `--prod` | Production optimized build |
| `-h, --help` | Show help message |

---

## Manual Setup

If you prefer manual control or the scripts don't work for your system:

### 1. Clone Repository
```bash
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source
```

### 2. Copy Configuration Files
```bash
# These files are required for compilation
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h
```

### 3. Fix Line Endings (If Needed)
If you cloned on Windows or see `$'\r'` errors:
```bash
# Install dos2unix if needed
sudo apt-get install dos2unix

# Convert all shell scripts
find . -name "*.sh" -type f -exec dos2unix {} \;
dos2unix configure autorun
```

### 4. Build the Game
```bash
# Clean any previous builds
make clean

# Build (using all available cores)
make -j$(nproc)

# Or just
make
```

### 5. Create Required Symlinks
The MUD expects certain directories in the root, not in lib/:
```bash
ln -sf lib/world world
ln -sf lib/text text
ln -sf lib/etc etc
```

### 6. Set Up World Files
```bash
# Create world directories
mkdir -p lib/world/{zon,wld,mob,obj,shp,trg,qst,hlq}

# Copy minimal world files if they exist
for dir in zon wld mob obj shp trg qst; do
    if [ -f lib/world/minimal/index.${dir} ]; then
        cp lib/world/minimal/index.${dir} lib/world/${dir}/index
    else
        echo '$' > lib/world/${dir}/index
    fi
    cp lib/world/minimal/*.${dir} lib/world/${dir}/ 2>/dev/null || true
done

# Create HLQ index
echo '$' > lib/world/hlq/index
```

### 7. Create Text Files
```bash
# Create directories
mkdir -p lib/text/help lib/etc

# Create basic text files
echo "Welcome to LuminariMUD!" > lib/text/news
echo "LuminariMUD Credits" > lib/text/credits
echo "Message of the Day" > lib/text/motd
echo "Immortal MOTD" > lib/text/imotd
echo "Help" > lib/text/help/help
echo "Immortal Help" > lib/text/help/ihelp
echo "Info" > lib/text/info
echo "Wizard List" > lib/text/wizlist
echo "Immortal List" > lib/text/immlist
echo "Policies" > lib/text/policies
echo "Handbook" > lib/text/handbook
echo "Background" > lib/text/background
echo "Welcome!" > lib/text/greetings

# Create help index
echo '$' > lib/text/help/index

# Create minimal config
echo "# Minimal config" > lib/etc/config
```

### 8. Create Required Directories
```bash
mkdir -p lib/plrfiles/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
mkdir -p lib/plrobjs/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
mkdir -p lib/house
mkdir -p lib/mudmail
mkdir -p log
```

### 9. Run the MUD
```bash
./bin/circle -d lib
```

---

## Database Configuration

MySQL/MariaDB is **optional**. The game runs fine without it, with some features disabled.

### Setting Up MySQL (Optional)

#### 1. Install and Start MySQL/MariaDB
```bash
# Ubuntu/Debian
sudo apt-get install mariadb-server
sudo systemctl start mariadb
sudo systemctl enable mariadb

# Secure the installation
sudo mysql_secure_installation
```

#### 2. Create Database and User
```bash
mysql -u root -p

CREATE DATABASE luminari CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'luminari'@'localhost' IDENTIFIED BY 'your_secure_password';
GRANT ALL PRIVILEGES ON luminari.* TO 'luminari'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

#### 3. Configure Connection
```bash
# Create configuration file
cat > lib/mysql_config << EOF
mysql_host = localhost
mysql_database = luminari
mysql_username = luminari
mysql_password = your_secure_password
EOF

# Set secure permissions
chmod 600 lib/mysql_config
```

---

## Running the Server

### Using the Autorun Script (Recommended)
```bash
# Start with auto-restart on crash
./autorun

# Run in background
nohup ./autorun &
```

### Direct Startup
```bash
# Start on default port (4000)
./bin/circle -d lib

# Start on specific port
./bin/circle -q 5000 -d lib

# Run in background
nohup ./bin/circle -d lib > log/server.log 2>&1 &
```

### Using Screen/Tmux (Best for Remote Servers)
```bash
# Using screen
screen -S luminari
./bin/circle -d lib
# Detach: Ctrl+A then D
# Reattach: screen -r luminari

# Using tmux
tmux new -s luminari
./bin/circle -d lib
# Detach: Ctrl+B then D
# Reattach: tmux attach -t luminari
```

### Server Management
```bash
# Check if running
ps aux | grep circle

# View logs
tail -f log/syslog

# Stop autorun script
touch .killscript

# Pause autorun temporarily
touch pause
```

---

## Troubleshooting

### Common Issues and Solutions

#### Build Fails
```bash
# Missing configuration files
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h

# Clean rebuild
make clean && make
```

#### "$'\r': command not found" Errors
```bash
# Fix Windows line endings
sudo apt-get install dos2unix
dos2unix configure autorun
find . -name "*.sh" -exec dos2unix {} \;
```

#### MUD Won't Start - Missing Files
```bash
# Run simple setup to create all required files
./scripts/simple_setup.sh

# Or manually create symlinks
ln -sf lib/world world
ln -sf lib/text text
ln -sf lib/etc etc
```

#### Port Already in Use
```bash
# Find what's using port 4000
sudo lsof -i :4000

# Kill the process
kill -9 [PID]

# Or use a different port
./bin/circle -q 5000 -d lib
```

#### MySQL Connection Fails
- Verify service is running: `sudo systemctl status mariadb`
- Check credentials in `lib/mysql_config`
- Test connection: `mysql -u luminari -p luminari`
- The game runs fine without MySQL - just ignore the warnings

---

## Historical Issues (Resolved)

These issues have been fixed but are documented for reference:

### ✅ Fixed in Deploy Scripts (August 2025)

1. **Deploy script path navigation** - Script now correctly navigates to project root
2. **World file copying bugs** - Fixed incorrect wildcard usage, proper index renaming
3. **Missing symlinks** - Automatically created for world/, text/, etc/
4. **Missing HLQ directory** - Now created automatically
5. **Text file initialization** - All required files created automatically

### ✅ Fixed in Codebase (August 2025)

1. **Windows line endings** - All scripts converted to Unix format (commit: bd62fbff)
2. **Makefile.am build issue** - Fixed source list interruption (commit: e09e148f)
3. **MySQL hard requirement** - Now optional with graceful degradation
4. **Missing example files** - All .example.h files now included

### ✅ Documentation Created

1. Created `scripts/simple_setup.sh` for zero-interaction deployment
2. Fixed `scripts/deploy.sh` with proper path handling
3. Created comprehensive quick start guides
4. Updated all documentation with working instructions

---

## Next Steps

After successful deployment:

1. **Connect to the MUD**: Use any MUD client to connect to `localhost:4000`
2. **Create Admin Character**: The first character created gets admin privileges
3. **Read Documentation**: 
   - [QUICKSTART Guide](../QUICKSTART.md) - Beginner's guide
   - [Developer Guide](../guides/DEVELOPER_GUIDE_AND_API.md) - For coding
   - [Database Guide](DATABASE_DEPLOYMENT_GUIDE.md) - Database details
4. **Start Building**: Use OLC commands to create your world

---

## Support

- **GitHub Issues**: [Report bugs or request features](https://github.com/LuminariMUD/Luminari-Source/issues)
- **Documentation**: Check the `docs/` directory for detailed guides
- **Logs**: Check `log/syslog` for error messages

---

*Last updated: August 28, 2025*
*Deployment from fresh clone: ✅ WORKING*