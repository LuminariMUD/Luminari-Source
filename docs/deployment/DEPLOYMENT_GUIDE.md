# LuminariMUD Deployment Guide

## Overview

LuminariMUD is a comprehensive MUD codebase implementing Pathfinder/D&D 3.5 mechanics. This guide covers the complete deployment process from source code to running server.

**Important**: This is a substantial deployment process for a complex C-based MUD server. Expect the initial setup to take 15-30 minutes depending on your system and experience level.

---

## System Requirements

### Minimum Requirements
- **Operating System**: Linux (Ubuntu 18.04+, CentOS 7+, Debian 9+) or Unix-like system
- **Memory**: 512MB RAM (2GB+ recommended for production)
- **Storage**: 1GB+ free disk space
- **Network**: TCP/IP networking capability
- **Compiler**: GCC 4.8+ with ANSI C90/C89 support

### Recommended Requirements
- **Operating System**: Ubuntu 20.04+ LTS or CentOS 8+
- **Memory**: 4GB+ RAM for development, 2GB+ for production
- **Storage**: 5GB+ free disk space
- **Compiler**: GCC 9.0+ or Clang 10.0+
- **Build System**: GNU Autotools (automake, autoconf)
- **Database**: MariaDB 10.3+ (optional but recommended)

---

## Dependencies Installation

### Ubuntu/Debian (including WSL2)

```bash
# Update package list
sudo apt-get update

# Install REQUIRED build dependencies
sudo apt-get install -y build-essential git make autoconf automake

# Optional but recommended dependencies
sudo apt-get install -y libcrypt-dev libgd-dev libmariadb-dev \
                        libcurl4-openssl-dev libssl-dev mariadb-server \
                        pkg-config libjson-c-dev

# For debugging (recommended)
sudo apt-get install -y gdb valgrind

# If encountering line ending issues
sudo apt-get install -y dos2unix
```

### CentOS/RHEL/Fedora

```bash
# For CentOS 7/RHEL 7
sudo yum install -y gcc make git autoconf automake

# For CentOS 8+/RHEL 8+/Fedora
sudo dnf install -y gcc make git autoconf automake

# Optional but recommended
sudo dnf install -y mariadb-server mariadb-devel gd-devel \
                    libcrypt-devel libtool json-c-devel
```

---

## Deployment Process

### Method 1: Automated Setup Script

The setup script handles all necessary steps automatically:

```bash
# Clone the repository
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source

# Run the setup script (handles autoreconf, configure, make, make install)
./scripts/setup.sh

# Start the server
./bin/circle -d lib
```

The setup script performs:
- Generates the build system (autoreconf + configure)
- Copies required configuration files
- Builds the entire codebase
- Installs binaries to bin/
- Creates required symlinks
- Sets up world files
- Creates necessary directories

### Method 2: Deploy Script with Options

For more control over the deployment process:

```bash
# Clone the repository
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source

# Generate build system first
autoreconf -fvi

# Run deployment with options
./scripts/deploy.sh --skip-db --init-world

# Start the server
./bin/circle -d lib
```

Deploy script options:
| Option | Description |
|--------|-------------|
| `--skip-db` | Skip database setup (run without MySQL) |
| `--skip-deps` | Skip dependency installation |
| `--init-world` | Initialize minimal world data |
| `--dev` | Development build with debug symbols |
| `--prod` | Production optimized build |
| `-h, --help` | Show help message |

### Method 3: Manual Deployment

For complete control over each step:

#### 1. Clone Repository
```bash
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source
```

#### 2. Generate Build System
```bash
# Generate configure script and Makefiles
autoreconf -fvi
```

#### 3. Copy Configuration Files
```bash
# Required for compilation
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h
```

#### 4. Configure and Build
```bash
# Configure the build
./configure

# Clean any previous builds
make clean

# Build using all available cores
make -j$(nproc)

# Install binaries to bin/
make install
```

#### 5. Create Required Symlinks
```bash
# The MUD expects these in the root directory
ln -sf lib/world world
ln -sf lib/text text
ln -sf lib/etc etc
```

#### 6. Set Up World Files
```bash
# Create world directories
mkdir -p lib/world/{zon,wld,mob,obj,shp,trg,qst,hlq}

# Copy minimal world files
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

#### 7. Create Text Files
```bash
# Create directories
mkdir -p lib/text/help lib/etc

# Create required text files
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
echo "# LuminariMUD Configuration" > lib/etc/config
```

#### 8. Create Required Directories
```bash
mkdir -p lib/plrfiles/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
mkdir -p lib/plrobjs/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
mkdir -p lib/house
mkdir -p lib/mudmail
mkdir -p log
```

#### 9. Start the Server
```bash
./bin/circle -d lib
```

---

## Database Configuration (Optional)

MySQL/MariaDB provides persistent storage for player data. The game runs without it, but some features will be disabled.

### Setting Up MySQL/MariaDB

#### 1. Install and Start Database
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

### Using the Autorun Script (Recommended for Production)
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

### Using Screen/Tmux (Recommended for Remote Servers)
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

### Common Issues

#### Build Fails - Missing Configuration Files
```bash
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h
```

#### Build Fails - No Makefile
```bash
# Generate build system first
autoreconf -fvi
./configure
```

#### Binary Not in bin/ Directory
```bash
# Must run make install after building
make install
```

#### Windows Line Endings (CRLF) Errors
```bash
# Fix line endings
sudo apt-get install dos2unix
dos2unix configure autorun
find . -name "*.sh" -exec dos2unix {} \;
```

#### MUD Won't Start - Missing Files
```bash
# Create required symlinks
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
- The game runs without MySQL - warnings can be ignored

---

## Post-Deployment

After successful deployment:

1. **Connect to the MUD**: Use any MUD client to connect to `localhost:4000`
2. **Create Admin Character**: The first character created gets admin privileges
3. **Review Documentation**: 
   - [Getting Started](../GETTING_STARTED.md) - Player and builder basics
   - [Developer Guide](../guides/DEVELOPER_GUIDE_AND_API.md) - For code development
   - [Database Guide](DATABASE_DEPLOYMENT_GUIDE.md) - Database details
4. **Start Building**: Use OLC (Online Creation) commands to build your world

---

## Known Issues

### Minor Issues (Non-blocking)
1. **Configure script cosmetic error**: `cat: ./src/conf.h.in: No such file or directory` - harmless, doesn't affect build
2. **MySQL config template**: Uses placeholder values - customize as needed
3. **Start room warnings**: "Immort/Frozen start room does not exist" - cosmetic warnings on startup

These issues do not prevent successful deployment.

---

## Support

- **GitHub Issues**: [Report bugs or request features](https://github.com/LuminariMUD/Luminari-Source/issues)
- **Documentation**: Check the `docs/` directory for detailed guides
- **Logs**: Check `log/syslog` for error messages

---

*Last updated: September 2025*
*Deployment status: WORKING*