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
BUILD_TYPE="development"
SKIP_DEPS=false
SKIP_DB=false
QUICK_MODE=false
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
    
    cd "$SCRIPT_DIR"
    
    # Setup campaign.h
    if [[ ! -f src/campaign.h ]]; then
        print_msg "$GREEN" "Creating campaign.h from template..."
        cp src/campaign.example.h src/campaign.h
        
        if [[ "$QUICK_MODE" == false ]]; then
            print_msg "$YELLOW" "Select campaign setting:"
            echo "  1) LuminariMUD (default)"
            echo "  2) DragonLance (Chronicles of Krynn)"
            echo "  3) Forgotten Realms (Faerun)"
            read -p "Choice [1-3]: " campaign_choice
            
            case $campaign_choice in
                2)
                    sed -i 's|/\* #define CAMPAIGN_DL \*/|#define CAMPAIGN_DL|' src/campaign.h
                    print_msg "$GREEN" "DragonLance campaign selected"
                    ;;
                3)
                    sed -i 's|/\* #define CAMPAIGN_FR \*/|#define CAMPAIGN_FR|' src/campaign.h
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
    if [[ ! -f src/mud_options.h ]]; then
        print_msg "$GREEN" "Creating mud_options.h from template..."
        cp src/mud_options.example.h src/mud_options.h
    else
        print_msg "$YELLOW" "mud_options.h already exists, skipping..."
    fi
    
    # Setup vnums.h
    if [[ ! -f src/vnums.h ]]; then
        print_msg "$GREEN" "Creating vnums.h from template..."
        cp src/vnums.example.h src/vnums.h
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
    cat > lib/mysql_config <<EOF
# Auto-generated MySQL configuration for LuminariMUD
mysql_host = $DB_HOST
mysql_database = $DB_NAME
mysql_username = $DB_USER
mysql_password = $DB_PASS
EOF
    chmod 600 lib/mysql_config
    
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
    if [[ -f sql/pubsub_v3_schema.sql ]]; then
        print_msg "$GREEN" "Loading pubsub schema..."
        mysql -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" < sql/pubsub_v3_schema.sql
    fi
    
    # Clean up temp file
    rm -f /tmp/luminari_db_setup.sql
    
    print_msg "$GREEN" "Database setup complete!"
}

# Function to build the project
build_project() {
    print_header "Building LuminariMUD"
    
    cd "$SCRIPT_DIR"
    
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
        
        # Generate configure if needed
        if [[ ! -f configure ]]; then
            autoreconf -fvi
        fi
        
        # Configure
        if [[ "$BUILD_TYPE" == "production" ]]; then
            ./configure --enable-optimizations
        else
            ./configure --enable-debug
        fi
        
        # Build
        make -j$(nproc)
        
    else
        print_msg "$RED" "No build system found!"
        exit 1
    fi
    
    print_msg "$GREEN" "Build complete!"
}

# Function to setup environment
setup_environment() {
    print_header "Setting Up Environment"
    
    # Create necessary directories
    mkdir -p lib/plrfiles/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
    mkdir -p lib/plrobjs/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
    mkdir -p lib/house
    mkdir -p lib/mudmail
    mkdir -p log
    
    # Set permissions
    chmod -R 755 lib/
    chmod -R 755 log/
    
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
WorkingDirectory=$SCRIPT_DIR
ExecStart=$SCRIPT_DIR/bin/circle
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

# Function to create startup script
create_startup_script() {
    print_header "Creating Startup Script"
    
    cat > start_mud.sh <<'EOF'
#!/bin/bash
# LuminariMUD Startup Script

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check if already running
if pgrep -f "bin/circle" > /dev/null; then
    echo "LuminariMUD is already running!"
    exit 1
fi

echo "Starting LuminariMUD..."

# Start with autorun for automatic restarts
if [[ -f autorun.sh ]]; then
    ./autorun.sh &
else
    bin/circle &
fi

echo "LuminariMUD started! Connect to port 4000"
EOF
    
    chmod +x start_mud.sh
    print_msg "$GREEN" "Startup script created: start_mud.sh"
}

# Function to show final instructions
show_final_instructions() {
    print_header "Deployment Complete!"
    
    print_msg "$GREEN" "LuminariMUD has been successfully deployed!"
    echo
    print_msg "$YELLOW" "Next steps:"
    echo "  1. Start the server: ./start_mud.sh"
    echo "  2. Connect with a MUD client to: localhost:$MUD_PORT"
    echo "  3. Create your first immortal character"
    echo
    print_msg "$YELLOW" "Important files:"
    echo "  - Configuration: src/campaign.h, src/mud_options.h"
    echo "  - Database config: lib/mysql_config"
    echo "  - Logs: log/"
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
    create_startup_script
    show_final_instructions
}

# Run main function
main