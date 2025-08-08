# LuminariMUD Setup and Build Guide

## Overview

This comprehensive guide covers the complete process of setting up, building, configuring, and running LuminariMUD from source code. It includes system requirements, dependency installation, compilation, database setup, and initial configuration.

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
- **Build System**: CMake 3.12+ (recommended) or Autotools
- **Database**: MySQL 8.0+ or MariaDB 10.3+

## Dependencies

### Core Dependencies
- **build-essential** - GCC compiler and build tools
- **cmake** - Cross-platform build system (3.12+ required for CMake builds)
- **mysql-server** - MySQL database server (5.0+ or MariaDB 10.3+)
- **libmysqlclient-dev** - MySQL client development libraries
- **libgd-dev** - GD graphics library for map generation
- **libcrypt-dev** - Cryptographic functions library
- **libcurl-dev** - URL transfer library
- **libssl-dev** - SSL/TLS library
- **git** - Version control system
- **autoconf** - GNU Autoconf for build configuration (for Autotools builds)
- **automake** - GNU Automake for Makefile generation (for Autotools builds)
- **libtool** - Generic library support script (optional)
- **valgrind** - Memory debugging tool (recommended)

### Ubuntu/Debian Installation (including WSL2)

#### Required Libraries (from Makefile.am line 170)
The project links against these libraries: `-lcrypt -lgd -lm -lmysqlclient -lcurl -lssl -lcrypto -lpthread`

```bash
# Update package list
sudo apt-get update

# Install REQUIRED dependencies (these match the libraries in Makefile.am)
sudo apt-get install -y build-essential libcrypt-dev libgd-dev libmysqlclient-dev \
                        libcurl4-openssl-dev libssl-dev mysql-server git make

# For CMake builds (recommended)
sudo apt-get install -y cmake

# For Autotools builds (traditional)
sudo apt-get install -y autoconf automake libtool

# Optional but recommended development tools
sudo apt-get install -y gdb valgrind doxygen graphviz cppcheck clang-format
```

**WSL2 Specific Note**: All the above packages work perfectly on WSL2 Ubuntu. The libraries map as follows:
- `-lcrypt` → `libcrypt-dev`
- `-lgd` → `libgd-dev` (for map generation)
- `-lm` → included in `build-essential` (math library)
- `-lmysqlclient` → `libmysqlclient-dev`
- `-lcurl` → `libcurl4-openssl-dev`
- `-lssl -lcrypto` → `libssl-dev`
- `-lpthread` → included in standard libc

### CentOS/RHEL/Fedora Installation
```bash
# For CentOS 7/RHEL 7
sudo yum install -y gcc make mysql-server mysql-devel gd-devel \
                    libcrypt-devel git autoconf automake libtool

# For CentOS 8+/RHEL 8+/Fedora
sudo dnf install -y gcc make mysql-server mysql-devel gd-devel \
                    libcrypt-devel git autoconf automake libtool

# Install additional development tools (optional)
sudo dnf install -y gdb valgrind doxygen graphviz
```

## Source Code Setup

### 1. Clone Repository
```bash
# Clone the main repository
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source

# Verify repository structure
ls -la
```

### 2. Configure Required Headers
```bash
# Copy and configure required header files (one-time setup)
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h

# Edit configuration headers as needed
# These files contain game-specific settings and must not be in version control
vi src/campaign.h      # Campaign settings (default, DragonLance, Forgotten Realms)
vi src/mud_options.h   # MUD feature toggles and options
vi src/vnums.h         # Virtual number assignments for zones/objects
```

### 3. Build the MUD

#### Option A: Build with CMake (Recommended)
```bash
# Configure the build
cmake -S . -B build/

# Build the project (using all available cores)
cmake --build build/ -j$(nproc)

# The binary will be created at: bin/circle
ls -la bin/circle
```

#### Option B: Build with Autotools (Traditional)
```bash
# If configure script doesn't exist or you modified configure.ac, regenerate it:
autoreconf -fvi

# Make configure script executable (if needed)
chmod +x configure

# Run configure script to generate Makefile
./configure

# Build the project
make -j$(nproc)

# The binary will be created at: bin/circle
ls -la bin/circle
```

### 4. Create Required Directories
```bash
# Create binary directory
mkdir -p bin

# Create log directory
mkdir -p log

# Verify directory structure
ls -la
```

## Database Setup

### 1. MySQL Server Configuration
```bash
# Start MySQL service
sudo systemctl start mysql
sudo systemctl enable mysql

# Secure MySQL installation (recommended)
sudo mysql_secure_installation
```

### 2. Create LuminariMUD Database
```bash
# Connect to MySQL as root
mysql -u root -p

# Create database and user
CREATE DATABASE luminari CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'luminari'@'localhost' IDENTIFIED BY 'your_secure_password';
GRANT ALL PRIVILEGES ON luminari.* TO 'luminari'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

### 3. Test Database Connection
```bash
# Test connection with new user
mysql -u luminari -p luminari

# Verify database exists
SHOW DATABASES;
EXIT;
```

## Configuration Files

### 1. Copy Example Configuration Files
```bash
# Copy example configuration files
cp campaign.example.h campaign.h
cp mud_options.example.h mud_options.h
cp vnums.example.h vnums.h

# Verify files were copied
ls -la *.h | grep -E "(campaign|mud_options|vnums)\.h"
```

### 2. Configure Database Connection
The database connection is configured in `lib/mysql_config` file:

```bash
# Create or edit the MySQL configuration file
cat > lib/mysql_config << EOF
mysql_host = localhost
mysql_database = luminari
mysql_username = luminari
mysql_password = your_secure_password
EOF

# Set appropriate permissions
chmod 600 lib/mysql_config
```

### 3. Configure Server Options
Edit `mud_options.h` for server-specific settings:

```c
/* Server Configuration */
#define DFLT_PORT 4000          /* Default port */
#define MAX_PLAYERS 300         /* Maximum concurrent players */
#define DFLT_DIR "lib"          /* Data directory */

/* Feature Toggles */
#define CIRCLE_UNIX             /* Unix-specific features */
#define CIRCLE_MYSQL            /* Enable MySQL support */
```

### 4. Configure Virtual Numbers
Edit `vnums.h` to set virtual number ranges:

```c
/* Virtual Number Ranges */
#define ROOM_VNUM_START 1       /* Starting room vnum */
#define OBJ_VNUM_START 1        /* Starting object vnum */
#define MOB_VNUM_START 1        /* Starting mobile vnum */
```

## Building the Server

### Build System Overview
LuminariMUD uses GNU Autotools (autoconf/automake) for its build system:
- **Source files**: `configure.ac`, `Makefile.am`
- **Generated files**: `configure`, `Makefile.in`, `Makefile`
- **Build artifacts**: Object files (`.o`), executables

### Cleaning Options
```bash
# Clean object files and executables only
make clean

# Clean autotools auxiliary files (keeps Makefile & config.h for daily builds)
make scrub

# Full clean - removes everything (requires autoreconf to rebuild)
make distclean
```

### 1. Clean Build
```bash
# Clean any previous build artifacts
make clean

# Generate dependencies
make depend
```

### 2. Compile Main Server
```bash
# Build the main server executable
make

# Or build everything including utilities
make all
```

### 3. Build Utilities (Optional)
```bash
# Build utility programs
make utils

# Verify utilities were built
ls -la bin/
```

### 4. Verify Build Success
```bash
# Check that main executable exists
ls -la bin/circle

# Check file permissions
chmod +x bin/circle

# Verify executable works
bin/circle --help
```

## Initial Server Setup

### 1. World Data Preparation
```bash
# Ensure world data directory exists
ls -la lib/

# Check for required world files
ls -la lib/world/
ls -la lib/text/
```

### 2. Create Initial Administrator
The first character created will automatically be granted administrator privileges.

### 3. Test Server Startup
```bash
# Test server startup (syntax check mode)
bin/circle -s

# If syntax check passes, start server normally
bin/circle
```

## Running the Server

### 1. Using the Autorun Script (Recommended)
The `autorun` script handles automatic restarts and log rotation:

```bash
# Start server with autorun script
./autorun

# Run autorun in background
nohup ./autorun &

# The autorun script will:
# - Automatically restart the server if it crashes
# - Rotate logs in the log/ directory
# - Handle shutdown commands from within the game
```

### 2. Direct Server Startup
```bash
# Start server on default port (4000)
bin/circle

# Start server on specific port
bin/circle -p 4000

# Start server with specific configuration
bin/circle -f campaign.h -o log/syslog

# Run server directly in background
nohup bin/circle > log/server.log 2>&1 &
```

### 3. Server Management
```bash
# Check if server is running
ps aux | grep circle
netstat -tulpn | grep :4000

# View server logs
tail -f log/syslog

# Stop the autorun script
touch .killscript

# Pause server restarts
touch pause
```

## Testing the Installation

### 1. Connect to Server
```bash
# Connect via telnet
telnet localhost 4000

# Or use netcat
nc localhost 4000
```

### 2. Create Test Character
1. Connect to the server
2. Create a new character
3. Verify character creation works
4. Test basic commands

### 3. Verify Database Integration
```bash
# Check database for player data
mysql -u luminari -p luminari
SELECT * FROM player_data LIMIT 5;
EXIT;
```

## Troubleshooting

### Common Build Issues

#### Missing Dependencies
```bash
# Error: mysql.h not found
sudo apt-get install libmysqlclient-dev

# Error: gd.h not found
sudo apt-get install libgd-dev
```

#### Compilation Errors
```bash
# Clean and rebuild
make clean
make depend
make

# Check for specific errors in output
make 2>&1 | grep -i error
```

#### Missing configure Script
```bash
# If configure script is missing or outdated
autoreconf -fvi

# If autoreconf fails with missing macros
aclocal
autoheader
automake --add-missing
autoconf
```

#### Rebuilding from Scratch
```bash
# Complete rebuild process
make distclean              # Remove everything
autoreconf -fiv             # Regenerate all autotools files
./configure                 # Create Makefile and config.h
make all                    # Build everything
make install                # Install binaries

# Quick cleanup (keeps build capability)
make scrub                  # Removes autotools auxiliary files only
```

#### Permission Issues
```bash
# Fix executable permissions
chmod +x configure
chmod +x autorun
chmod +x bin/circle

# Fix directory permissions
chmod 755 lib/
```

### Runtime Issues

#### Database Connection Problems
1. Verify MySQL service is running
2. Check database credentials in `lib/mysql_config`
3. Test database connection manually
4. Check MySQL error logs
5. Verify mysql_config file permissions (should be 600)

#### Port Binding Issues
```bash
# Check if port is already in use
netstat -tulpn | grep :4000

# Kill existing process if needed
pkill -f circle
```

#### File Permission Issues
```bash
# Fix world file permissions
chmod -R 644 lib/world/
chmod 755 lib/world/

# Fix log file permissions
touch log/syslog
chmod 666 log/syslog
```

## Next Steps

After successful installation:

1. **Read Documentation**: Review the [Technical Documentation Master Index](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)
2. **Configure Game Settings**: Customize game mechanics in configuration files
3. **Set Up World Data**: Import or create world content
4. **Configure Scripting**: Set up DG Scripts for dynamic content
5. **Establish Backup Procedures**: Implement regular database and file backups
6. **Monitor Performance**: Set up logging and monitoring systems

## Additional Resources

- **[Developer Guide](DEVELOPER_GUIDE_AND_API.md)**: For code development
- **[Database Integration](../systems/DATABASE_INTEGRATION.md)**: Detailed database setup
- **[Troubleshooting Guide](TROUBLESHOOTING_AND_MAINTENANCE.md)**: Common issues and solutions
- **[Testing Guide](TESTING_GUIDE.md)**: Quality assurance procedures

---

*This guide covers the essential setup process. For advanced configuration and customization, refer to the specific system documentation.*
