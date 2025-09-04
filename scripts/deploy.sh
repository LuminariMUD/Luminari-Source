#!/bin/bash

################################################################################
# LuminariMUD Automated Deployment Script
# 
# This script automates the deployment process for LuminariMUD, making it
# easy for anyone to set up and run their own server.
#
# Usage: ./deploy.sh [options]
#   -h, --help        Show help message
#   -q, --quick       Quick setup with defaults
#   -d, --dev         Development mode (includes debug tools)
#   -p, --prod        Production mode (optimized build)
#   --skip-deps       Skip dependency installation
#   --skip-db         Skip database setup
################################################################################

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration variables
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_TYPE="development"
SKIP_DEPS=false
SKIP_DB=false
QUICK_MODE=false
INIT_WORLD=false
MUD_PORT=4000
DB_HOST="localhost"
DB_NAME="luminari"
DB_USER="luminari"

# Function to print colored messages
print_msg() {
    local color=$1
    local msg=$2
    echo -e "${color}${msg}${NC}"
}

# Function to print header
print_header() {
    echo
    print_msg "$BLUE" "=================================================================================="
    print_msg "$BLUE" "$1"
    print_msg "$BLUE" "=================================================================================="
    echo
}

# Function to check if running as root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        print_msg "$YELLOW" "Warning: Running as root is not recommended!"
        read -p "Continue anyway? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
}

# Function to detect OS
detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        OS=$ID
        VER=$VERSION_ID
    elif type lsb_release >/dev/null 2>&1; then
        OS=$(lsb_release -si | tr '[:upper:]' '[:lower:]')
        VER=$(lsb_release -sr)
    else
        print_msg "$RED" "Cannot detect OS. Please install manually."
        exit 1
    fi
    
    print_msg "$GREEN" "Detected OS: $OS $VER"
}

# Function to install dependencies
install_dependencies() {
    print_header "Installing Dependencies"
    
    if [[ "$SKIP_DEPS" == true ]]; then
        print_msg "$YELLOW" "Skipping dependency installation..."
        return
    fi
    
    case $OS in
        ubuntu|debian)
            print_msg "$GREEN" "Installing packages for Ubuntu/Debian..."
            sudo apt-get update
            sudo apt-get install -y \
                build-essential cmake autoconf automake libtool pkg-config \
                libcrypt-dev libgd-dev libmariadb-dev libcurl4-openssl-dev \
                libssl-dev mariadb-server git make
            
            if [[ "$BUILD_TYPE" == "development" ]]; then
                sudo apt-get install -y gdb valgrind
            fi
            ;;
        
        centos|rhel|fedora)
            print_msg "$GREEN" "Installing packages for CentOS/RHEL/Fedora..."
            sudo yum install -y \
                gcc gcc-c++ make cmake autoconf automake libtool \
                mariadb mariadb-devel mariadb-server \
                gd-devel openssl-devel libcurl-devel git
            
            if [[ "$BUILD_TYPE" == "development" ]]; then
                sudo yum install -y gdb valgrind
            fi
            ;;
        
        arch|manjaro)
            print_msg "$GREEN" "Installing packages for Arch Linux..."
            sudo pacman -Sy --noconfirm \
                base-devel cmake mariadb libmariadbclient \
                gd openssl curl git
            
            if [[ "$BUILD_TYPE" == "development" ]]; then
                sudo pacman -Sy --noconfirm gdb valgrind
            fi
            ;;
        
        *)
            print_msg "$YELLOW" "Unknown OS: $OS"
            print_msg "$YELLOW" "Please install dependencies manually:"
            echo "  - GCC/build tools"
            echo "  - CMake (3.12+)"
            echo "  - MariaDB server and client libraries"
            echo "  - GD, OpenSSL, cURL development libraries"
            ;;
    esac
}

# Function to setup configuration files
setup_config_files() {
    print_header "Setting Up Configuration Files"
    
    # Setup campaign.h
    if [[ ! -f "$PROJECT_ROOT/src/campaign.h" ]]; then
        print_msg "$GREEN" "Creating campaign.h from template..."
        cp "$PROJECT_ROOT"/src/campaign.example.h "$PROJECT_ROOT"/src/campaign.h
        
        if [[ "$QUICK_MODE" == false ]]; then
            print_msg "$YELLOW" "Select campaign setting:"
            echo "  1) LuminariMUD (default)"
            echo "  2) DragonLance (Chronicles of Krynn)"
            echo "  3) Forgotten Realms (Faerun)"
            read -p "Choice [1-3]: " campaign_choice
            
            case $campaign_choice in
                2)
                    sed -i 's|/\* #define CAMPAIGN_DL \*/|#define CAMPAIGN_DL|' "$PROJECT_ROOT"/src/campaign.h
                    print_msg "$GREEN" "DragonLance campaign selected"
                    ;;
                3)
                    sed -i 's|/\* #define CAMPAIGN_FR \*/|#define CAMPAIGN_FR|' "$PROJECT_ROOT"/src/campaign.h
                    print_msg "$GREEN" "Forgotten Realms campaign selected"
                    ;;
                *)
                    print_msg "$GREEN" "Using default LuminariMUD campaign"
                    ;;
            esac
        fi
    else
        print_msg "$YELLOW" "campaign.h already exists, skipping..."
    fi
    
    # Setup mud_options.h
    if [[ ! -f "$PROJECT_ROOT/src/mud_options.h" ]]; then
        print_msg "$GREEN" "Creating mud_options.h from template..."
        cp "$PROJECT_ROOT"/src/mud_options.example.h "$PROJECT_ROOT"/src/mud_options.h
    else
        print_msg "$YELLOW" "mud_options.h already exists, skipping..."
    fi
    
    # Setup vnums.h
    if [[ ! -f "$PROJECT_ROOT/src/vnums.h" ]]; then
        print_msg "$GREEN" "Creating vnums.h from template..."
        cp "$PROJECT_ROOT"/src/vnums.example.h "$PROJECT_ROOT"/src/vnums.h
    else
        print_msg "$YELLOW" "vnums.h already exists, skipping..."
    fi
}

# Function to setup database
setup_database() {
    print_header "Setting Up Database"
    
    if [[ "$SKIP_DB" == true ]]; then
        print_msg "$YELLOW" "Skipping database setup..."
        return
    fi
    
    # Start MariaDB service
    print_msg "$GREEN" "Starting MariaDB service..."
    if command -v systemctl &> /dev/null; then
        sudo systemctl start mariadb || sudo systemctl start mysql
        sudo systemctl enable mariadb || sudo systemctl enable mysql
    else
        sudo service mysql start || sudo service mariadb start
    fi
    
    # Get database credentials
    if [[ "$QUICK_MODE" == false ]]; then
        read -p "Database host [$DB_HOST]: " input_host
        DB_HOST=${input_host:-$DB_HOST}
        
        read -p "Database name [$DB_NAME]: " input_name
        DB_NAME=${input_name:-$DB_NAME}
        
        read -p "Database user [$DB_USER]: " input_user
        DB_USER=${input_user:-$DB_USER}
        
        read -s -p "Database password (hidden): " DB_PASS
        echo
        
        if [[ -z "$DB_PASS" ]]; then
            # Generate random password if none provided
            DB_PASS=$(openssl rand -base64 12)
            print_msg "$YELLOW" "Generated password: $DB_PASS"
            print_msg "$YELLOW" "Please save this password!"
        fi
    else
        # Quick mode - generate random password
        DB_PASS=$(openssl rand -base64 12)
        print_msg "$GREEN" "Using default database settings:"
        echo "  Host: $DB_HOST"
        echo "  Database: $DB_NAME"
        echo "  User: $DB_USER"
        print_msg "$YELLOW" "Generated password: $DB_PASS"
        print_msg "$YELLOW" "IMPORTANT: Save this password!"
    fi
    
    # Create MySQL config file
    print_msg "$GREEN" "Creating MySQL configuration file..."
    cat > "$PROJECT_ROOT"/lib/mysql_config <<EOF
# Auto-generated MySQL configuration for LuminariMUD
mysql_host = $DB_HOST
mysql_database = $DB_NAME
mysql_username = $DB_USER
mysql_password = $DB_PASS
EOF
    chmod 600 "$PROJECT_ROOT"/lib/mysql_config
    
    # Setup database
    print_msg "$GREEN" "Setting up database..."
    
    # Create database setup SQL
    cat > /tmp/luminari_db_setup.sql <<EOF
-- Create database if not exists
CREATE DATABASE IF NOT EXISTS $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- Create user if not exists (MySQL 5.7+/MariaDB 10.2+)
CREATE USER IF NOT EXISTS '$DB_USER'@'$DB_HOST' IDENTIFIED BY '$DB_PASS';

-- Grant privileges
GRANT ALL PRIVILEGES ON $DB_NAME.* TO '$DB_USER'@'$DB_HOST';
FLUSH PRIVILEGES;

USE $DB_NAME;
EOF
    
    # Execute database setup
    print_msg "$YELLOW" "Please enter your MySQL/MariaDB root password:"
    mysql -u root -p < /tmp/luminari_db_setup.sql
    
    # Run schema files if they exist
    if [[ -f "$PROJECT_ROOT/sql/pubsub_v3_schema.sql" ]]; then
        print_msg "$GREEN" "Loading pubsub schema..."
        mysql -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" < "$PROJECT_ROOT"/sql/pubsub_v3_schema.sql
    fi
    
    # Clean up temp file
    rm -f /tmp/luminari_db_setup.sql
    
    print_msg "$GREEN" "Database setup complete!"
}

# Function to build the project
build_project() {
    print_header "Building LuminariMUD"
    
    # Change to project root directory
    cd "$PROJECT_ROOT"
    
    # Detect build system
    if [[ -f CMakeLists.txt ]]; then
        print_msg "$GREEN" "Building with CMake..."
        
        # Clean old build
        rm -rf build
        mkdir -p build
        
        # Configure
        if [[ "$BUILD_TYPE" == "production" ]]; then
            cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Release
        else
            cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Debug
        fi
        
        # Build
        cmake --build build/ -j$(nproc)
        
    elif [[ -f configure.ac ]]; then
        print_msg "$GREEN" "Building with Autotools..."
        
        # Clean any previous build attempts
        if [[ -f Makefile ]]; then
            make distclean 2>/dev/null || true
        fi
        
        # Generate configure script
        print_msg "$GREEN" "Generating configure script..."
        autoreconf -fvi
        
        # Make sure make-tests.sh is executable
        if [[ -f unittests/CuTest/make-tests.sh ]]; then
            chmod +x unittests/CuTest/make-tests.sh
        fi
        
        # Configure
        print_msg "$GREEN" "Running configure..."
        if [[ "$BUILD_TYPE" == "production" ]]; then
            ./configure --enable-optimizations
        else
            ./configure
        fi
        
        # Build
        print_msg "$GREEN" "Building (this may take a few minutes)..."
        make -j$(nproc) all
        
        # Install
        print_msg "$GREEN" "Installing..."
        make install
        
        # Check if build succeeded
        if [[ ! -f "$PROJECT_ROOT/bin/circle" ]]; then
            print_msg "$RED" "Build failed - bin/circle executable not created"
            print_msg "$YELLOW" "Try running 'make' manually to see detailed errors"
            exit 1
        fi
        
        print_msg "$GREEN" "Build and install complete: bin/circle"
        
    else
        print_msg "$RED" "No build system found!"
        exit 1
    fi
    
    print_msg "$GREEN" "Build complete!"
}

# Function to initialize minimal world data
initialize_world_data() {
    print_msg "$GREEN" "Initializing minimal world data..."
    
    # Check if world directories exist and are empty
    if [[ ! -d "$PROJECT_ROOT/lib/world/zon" ]] || [[ -z "$(ls -A "$PROJECT_ROOT/lib/world/zon" 2>/dev/null)" ]]; then
        print_msg "$YELLOW" "Setting up minimal world files..."
        
        # Create world directories
        mkdir -p "$PROJECT_ROOT"/lib/world/{zon,wld,mob,obj,shp,trg,qst,hlq}
        
        # Copy minimal world files (properly renamed)
        if [[ -d "$PROJECT_ROOT/lib/world/minimal" ]]; then
            # Copy zone files - rename index.zon to index
            cp "$PROJECT_ROOT"/lib/world/minimal/index.zon "$PROJECT_ROOT"/lib/world/zon/index 2>/dev/null || true
            cp "$PROJECT_ROOT"/lib/world/minimal/*.zon "$PROJECT_ROOT"/lib/world/zon/ 2>/dev/null || true
            
            # Copy world/room files - rename index.wld to index
            cp "$PROJECT_ROOT"/lib/world/minimal/index.wld "$PROJECT_ROOT"/lib/world/wld/index 2>/dev/null || true
            cp "$PROJECT_ROOT"/lib/world/minimal/*.wld "$PROJECT_ROOT"/lib/world/wld/ 2>/dev/null || true
            
            # Copy mob files - rename index.mob to index
            cp "$PROJECT_ROOT"/lib/world/minimal/index.mob "$PROJECT_ROOT"/lib/world/mob/index 2>/dev/null || true
            cp "$PROJECT_ROOT"/lib/world/minimal/*.mob "$PROJECT_ROOT"/lib/world/mob/ 2>/dev/null || true
            
            # Copy object files - rename index.obj to index
            cp "$PROJECT_ROOT"/lib/world/minimal/index.obj "$PROJECT_ROOT"/lib/world/obj/index 2>/dev/null || true
            cp "$PROJECT_ROOT"/lib/world/minimal/*.obj "$PROJECT_ROOT"/lib/world/obj/ 2>/dev/null || true
            
            # Copy other index files - rename to just 'index'
            cp "$PROJECT_ROOT"/lib/world/minimal/index.shp "$PROJECT_ROOT"/lib/world/shp/index 2>/dev/null || true
            cp "$PROJECT_ROOT"/lib/world/minimal/index.trg "$PROJECT_ROOT"/lib/world/trg/index 2>/dev/null || true
            cp "$PROJECT_ROOT"/lib/world/minimal/index.qst "$PROJECT_ROOT"/lib/world/qst/index 2>/dev/null || true
            
            # Create HLQ index (Homeland Quests)
            echo '$' > "$PROJECT_ROOT"/lib/world/hlq/index
            
            print_msg "$GREEN" "Minimal world data initialized!"
        else
            print_msg "$YELLOW" "Warning: Minimal world data not found in $PROJECT_ROOT/lib/world/minimal/"
            print_msg "$YELLOW" "Creating empty index files..."
            echo '$' > "$PROJECT_ROOT"/lib/world/zon/index
            echo '$' > "$PROJECT_ROOT"/lib/world/wld/index
            echo '$' > "$PROJECT_ROOT"/lib/world/mob/index
            echo '$' > "$PROJECT_ROOT"/lib/world/obj/index
            echo '$' > "$PROJECT_ROOT"/lib/world/shp/index
            echo '$' > "$PROJECT_ROOT"/lib/world/trg/index
            echo '$' > "$PROJECT_ROOT"/lib/world/qst/index
            echo '$' > "$PROJECT_ROOT"/lib/world/hlq/index
        fi
    else
        print_msg "$GREEN" "World data already exists, skipping initialization."
    fi
}

# Function to create default text files
create_text_files() {
    print_msg "$GREEN" "Creating default text files..."
    
    mkdir -p "$PROJECT_ROOT"/lib/text/help
    mkdir -p "$PROJECT_ROOT"/lib/etc
    
    # Create news file
    if [[ ! -f "$PROJECT_ROOT/lib/text/news" ]]; then
        cat > "$PROJECT_ROOT"/lib/text/news <<'EOF'
&RWelcome to LuminariMUD!&n

This is a fresh installation of LuminariMUD. You can customize this
message by editing lib/text/news.

&YLatest Updates:&n
- Successfully deployed from GitHub
- Minimal world initialized
- Ready for building and customization

For help getting started, type 'help newbie' once logged in.
EOF
    fi
    
    # Create credits file
    if [[ ! -f "$PROJECT_ROOT/lib/text/credits" ]]; then
        cat > "$PROJECT_ROOT"/lib/text/credits <<'EOF'
&WLuminariMUD Credits&n

LuminariMUD is based on CircleMUD 3.0, created by Jeremy Elson.

&YLuminariMUD Development Team:&n
- Ornir (Lead Developer)
- Zusuk (Lead Developer)
- Gicker (Developer)
- Apfro (Developer)
- Ashyel (Developer)
- Lumi (Wilderness Development)
- Nashak (Developer)

Special thanks to all contributors and players who help make
LuminariMUD a great place to play!

CircleMUD was based on DikuMUD, created by:
Sebastian Hammer, Michael Seifert, Hans Henrik St{rfeldt,
Tom Madsen, and Katja Nyboe.
EOF
    fi
    
    # Create motd file
    if [[ ! -f "$PROJECT_ROOT/lib/text/motd" ]]; then
        cat > "$PROJECT_ROOT"/lib/text/motd <<'EOF'
&W*** Message of the Day ***&n

Welcome to LuminariMUD!

Please follow the rules and be respectful to other players.
Type 'help rules' for more information.

Have fun and enjoy your adventures!

&R[Report bugs and issues on GitHub]&n
EOF
    fi
    
    # Create imotd file
    if [[ ! -f "$PROJECT_ROOT/lib/text/imotd" ]]; then
        cat > "$PROJECT_ROOT"/lib/text/imotd <<'EOF'
&Y*** Immortal Message of the Day ***&n

Welcome, Immortal!

Please remember to be fair and helpful to players.
Check 'help immortal' for immortal commands.

Current development priorities:
- World building
- Bug fixes
- Player experience improvements
EOF
    fi
    
    # Create greetings file
    if [[ ! -f "$PROJECT_ROOT/lib/text/greetings" ]]; then
        cat > "$PROJECT_ROOT"/lib/text/greetings <<'EOF'

&W            Welcome to LuminariMUD!&n
            
    Based on CircleMUD 3.0 and DikuMUD
    
Enter your character name or 'new' to create a character:
EOF
    fi
    
    # Create basic help file
    if [[ ! -f "$PROJECT_ROOT/lib/text/help/help" ]]; then
        cat > "$PROJECT_ROOT"/lib/text/help/help <<'EOF'
Welcome to the LuminariMUD help system!

For a list of commands, type: commands
For newbie help, type: help newbie
For a list of all help topics, type: help index

You can get help on any command or topic by typing:
help <topic>
EOF
    fi
    
    # Create immortal help file  
    if [[ ! -f "$PROJECT_ROOT/lib/text/help/ihelp" ]]; then
        cat > "$PROJECT_ROOT"/lib/text/help/ihelp <<'EOF'
Immortal Help System

For a list of immortal commands, type: wizhelp
For building commands, type: help building
For OLC help, type: help olc

Common immortal commands:
- goto <room>    - Teleport to a room
- where          - List all players and locations  
- users          - Show connection information
- reboot         - Reboot the MUD
- shutdown       - Shutdown the MUD
EOF
    fi
    
    # Create info file
    if [[ ! -f "$PROJECT_ROOT/lib/text/info" ]]; then
        echo "LuminariMUD - A CircleMUD based MUD" > "$PROJECT_ROOT"/lib/text/info
    fi
    
    # Create wizlist file
    if [[ ! -f "$PROJECT_ROOT/lib/text/wizlist" ]]; then
        echo "Wizard List - See 'who' for online staff" > "$PROJECT_ROOT"/lib/text/wizlist
    fi
    
    # Create immlist file
    if [[ ! -f "$PROJECT_ROOT/lib/text/immlist" ]]; then
        echo "Immortal List - See 'who' for online staff" > "$PROJECT_ROOT"/lib/text/immlist
    fi
    
    # Create policies file
    if [[ ! -f "$PROJECT_ROOT/lib/text/policies" ]]; then
        cat > "$PROJECT_ROOT"/lib/text/policies <<'EOF'
LuminariMUD Policies

1. Be respectful to all players and staff
2. No cheating or exploiting bugs
3. No harassment or offensive behavior
4. Keep content appropriate for all ages
5. Report bugs when you find them
6. Have fun!

Violations may result in warnings, suspensions, or bans.
EOF
    fi
    
    # Create handbook file
    if [[ ! -f "$PROJECT_ROOT/lib/text/handbook" ]]; then
        echo "Player Handbook - Type 'help newbie' for getting started" > "$PROJECT_ROOT"/lib/text/handbook
    fi
    
    # Create background file
    if [[ ! -f "$PROJECT_ROOT/lib/text/background" ]]; then
        cat > "$PROJECT_ROOT"/lib/text/background <<'EOF'
The World of Luminari

A realm of magic and adventure awaits...

[This is a placeholder. Customize this with your world's lore]
EOF
    fi
    
    # Create etc/config with defaults
    if [[ ! -f "$PROJECT_ROOT/lib/etc/config" ]]; then
        cat > "$PROJECT_ROOT"/lib/etc/config <<'EOF'
# LuminariMUD Default Configuration
# This file contains default game configuration settings

# Game Settings
max_playing = 300
max_filesize = 50000
max_bad_pws = 3
siteok_everyone = 1
nameserver_is_slow = 0

# Port Settings
# (Set via command line with -q flag)

# Gameplay Settings  
pk_allowed = 1
pt_allowed = 1
level_can_shout = 1
holler_move_cost = 20
tunnel_size = 2
max_exp_gain = 100000
max_exp_loss = 500000
max_npc_corpse_time = 5
max_pc_corpse_time = 10
idle_void = 8
idle_rent_time = 48
idle_max_level = 50
dts_are_dumps = 1
load_into_inventory = 0
track_through_doors = 1
immort_level_ok = 0

# Rent/Economy Settings
free_rent = 0
max_obj_save = 30
min_rent_cost = 100
auto_save = 1
autosave_time = 5
crash_file_timeout = 10
rent_file_timeout = 30
EOF
    fi
    
    print_msg "$GREEN" "Default text files created!"
}

# Function to setup environment
setup_environment() {
    print_header "Setting Up Environment"
    
    # Create necessary directories
    mkdir -p "$PROJECT_ROOT"/lib/plrfiles/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
    mkdir -p "$PROJECT_ROOT"/lib/plrobjs/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
    mkdir -p "$PROJECT_ROOT"/lib/house
    mkdir -p "$PROJECT_ROOT"/lib/mudmail
    mkdir -p "$PROJECT_ROOT"/lib/etc
    mkdir -p "$PROJECT_ROOT"/log
    
    # Initialize world data if requested
    if [[ "$INIT_WORLD" == true ]]; then
        initialize_world_data
    fi
    
    # Create text files
    create_text_files
    
    # Create critical symlinks if they don't exist
    # The MUD expects files in the root directory, not in lib/
    if [[ ! -L "$PROJECT_ROOT/world" ]]; then
        ln -sf lib/world "$PROJECT_ROOT"/world
        print_msg "$GREEN" "Created symlink: world -> lib/world"
    fi
    if [[ ! -L "$PROJECT_ROOT/text" ]]; then
        ln -sf lib/text "$PROJECT_ROOT"/text
        print_msg "$GREEN" "Created symlink: text -> lib/text"
    fi
    if [[ ! -L "$PROJECT_ROOT/etc" ]]; then
        ln -sf lib/etc "$PROJECT_ROOT"/etc
        print_msg "$GREEN" "Created symlink: etc -> lib/etc"
    fi
    
    # Set permissions
    chmod -R 755 "$PROJECT_ROOT"/lib/
    chmod -R 755 "$PROJECT_ROOT"/log/
    
    # Create systemd service file (optional)
    if [[ "$QUICK_MODE" == false ]]; then
        read -p "Create systemd service file? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            create_systemd_service
        fi
    fi
    
    print_msg "$GREEN" "Environment setup complete!"
}

# Function to create systemd service
create_systemd_service() {
    print_msg "$GREEN" "Creating systemd service..."
    
    cat > /tmp/luminari.service <<EOF
[Unit]
Description=LuminariMUD Server
After=network.target mariadb.service

[Service]
Type=simple
User=$USER
WorkingDirectory=$PROJECT_ROOT
ExecStart=$PROJECT_ROOT/bin/circle
ExecReload=/bin/kill -HUP \$MAINPID
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
    
    sudo cp /tmp/luminari.service /etc/systemd/system/
    sudo systemctl daemon-reload
    
    print_msg "$GREEN" "Systemd service created!"
    print_msg "$YELLOW" "To start: sudo systemctl start luminari"
    print_msg "$YELLOW" "To enable on boot: sudo systemctl enable luminari"
}

# Function to verify autorun script
verify_autorun_script() {
    print_header "Verifying Autorun Script"
    
    if [[ -f "$PROJECT_ROOT/autorun.sh" ]]; then
        print_msg "$GREEN" "Autorun script found: autorun.sh"
        if [[ ! -x "$PROJECT_ROOT/autorun.sh" ]]; then
            print_msg "$YELLOW" "Making autorun.sh executable..."
            chmod +x "$PROJECT_ROOT/autorun.sh"
        fi
        print_msg "$GREEN" "Autorun script is ready to use"
    else
        print_msg "$YELLOW" "Warning: autorun.sh not found in project root"
        print_msg "$YELLOW" "You can start the MUD directly with: bin/circle"
    fi
}

# Function to show final instructions
show_final_instructions() {
    print_header "Deployment Complete!"
    
    print_msg "$GREEN" "LuminariMUD has been successfully deployed!"
    echo
    print_msg "$YELLOW" "Next steps:"
    echo "  1. Start the server: ./autorun.sh"
    echo "  2. Connect with a MUD client to: localhost:$MUD_PORT"
    echo "  3. Create your first immortal character"
    echo
    print_msg "$YELLOW" "Important files:"
    echo "  - Configuration: $PROJECT_ROOT/src/campaign.h, $PROJECT_ROOT/src/mud_options.h"
    echo "  - Database config: lib/mysql_config"
    echo "  - Logs: log/"
    echo "  - Autorun commands:"
    echo "      ./autorun.sh          - Start in background (daemon mode)"
    echo "      ./autorun.sh status   - Check server status"
    echo "      ./autorun.sh stop     - Stop the server"
    echo
    
    if [[ -n "$DB_PASS" ]]; then
        print_msg "$RED" "IMPORTANT: Save your database password: $DB_PASS"
    fi
}

# Function to show help
show_help() {
    cat <<EOF
LuminariMUD Automated Deployment Script

Usage: $0 [options]

Options:
    -h, --help        Show this help message
    -q, --quick       Quick setup with defaults (no prompts)
    -d, --dev         Development mode (includes debug tools)
    -p, --prod        Production mode (optimized build)
    --skip-deps       Skip dependency installation
    --skip-db         Skip database setup
    --init-world      Initialize minimal world data files
    
Examples:
    $0                # Interactive setup
    $0 --quick        # Quick setup with defaults
    $0 --dev          # Development setup with debug tools
    $0 --prod         # Production optimized build
    
For more information, see: docs/guides/SETUP_AND_BUILD_GUIDE.md
EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -q|--quick)
            QUICK_MODE=true
            shift
            ;;
        -d|--dev)
            BUILD_TYPE="development"
            shift
            ;;
        -p|--prod)
            BUILD_TYPE="production"
            shift
            ;;
        --skip-deps)
            SKIP_DEPS=true
            shift
            ;;
        --skip-db)
            SKIP_DB=true
            shift
            ;;
        --init-world)
            INIT_WORLD=true
            shift
            ;;
        *)
            print_msg "$RED" "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Main execution
main() {
    print_header "LuminariMUD Automated Deployment"
    
    check_root
    detect_os
    install_dependencies
    setup_config_files
    setup_database
    build_project
    setup_environment
    verify_autorun_script
    show_final_instructions
}

# Run main function
main