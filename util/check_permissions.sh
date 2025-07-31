#!/bin/bash

# LuminariMUD Permission Checker Script
# This script checks all necessary permissions for running a MUD server

echo "================================================"
echo "LuminariMUD Directory Permissions Checker"
echo "================================================"
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to check directory permissions
check_dir() {
    local dir=$1
    local expected_perms=$2
    local description=$3
    
    if [ -d "$dir" ]; then
        actual_perms=$(stat -c "%a" "$dir" 2>/dev/null)
        owner=$(stat -c "%U:%G" "$dir" 2>/dev/null)
        
        if [ "$actual_perms" = "$expected_perms" ]; then
            echo -e "${GREEN}[OK]${NC} $dir - $description"
            echo "     Permissions: $actual_perms (${owner})"
        else
            echo -e "${YELLOW}[WARN]${NC} $dir - $description"
            echo "     Expected: $expected_perms, Actual: $actual_perms (${owner})"
        fi
    else
        echo -e "${RED}[MISSING]${NC} $dir - $description"
    fi
}

# Function to check file permissions
check_file() {
    local file=$1
    local expected_perms=$2
    local description=$3
    
    if [ -f "$file" ]; then
        actual_perms=$(stat -c "%a" "$file" 2>/dev/null)
        owner=$(stat -c "%U:%G" "$file" 2>/dev/null)
        
        if [ "$actual_perms" = "$expected_perms" ]; then
            echo -e "${GREEN}[OK]${NC} $file - $description"
            echo "     Permissions: $actual_perms (${owner})"
        else
            echo -e "${YELLOW}[WARN]${NC} $file - $description"
            echo "     Expected: $expected_perms, Actual: $actual_perms (${owner})"
        fi
    else
        echo -e "${RED}[MISSING]${NC} $file - $description"
    fi
}

# Get the base directory (assuming script is run from source directory)
BASE_DIR=$(dirname $(pwd))

echo "Checking base directory: $BASE_DIR"
echo ""

# Check main directories
echo "=== Main Directories ==="
check_dir "$BASE_DIR" "755" "Base MUD directory"
check_dir "$BASE_DIR/bin" "755" "Binary directory"
check_dir "$BASE_DIR/lib" "755" "Library directory"
check_dir "$BASE_DIR/log" "755" "Log directory"
echo ""

# Check world directories
echo "=== World Data Directories ==="
check_dir "$BASE_DIR/lib/world" "755" "World directory"
check_dir "$BASE_DIR/lib/world/mob" "755" "Mobile directory"
check_dir "$BASE_DIR/lib/world/obj" "755" "Object directory"
check_dir "$BASE_DIR/lib/world/wld" "755" "Room directory"
check_dir "$BASE_DIR/lib/world/zon" "755" "Zone directory"
check_dir "$BASE_DIR/lib/world/shp" "755" "Shop directory"
check_dir "$BASE_DIR/lib/world/trg" "755" "Trigger directory"
check_dir "$BASE_DIR/lib/world/qst" "755" "Quest directory"
echo ""

# Check player data directories (need write access)
echo "=== Player Data Directories (Write Required) ==="
check_dir "$BASE_DIR/lib/plrfiles" "755" "Player files directory"
check_dir "$BASE_DIR/lib/plrmail" "755" "Player mail directory"
check_dir "$BASE_DIR/lib/plrobjs" "755" "Player objects directory"
check_dir "$BASE_DIR/lib/plrvars" "755" "Player variables directory"
check_dir "$BASE_DIR/lib/plralias" "755" "Player aliases directory"
check_dir "$BASE_DIR/lib/house" "755" "Player houses directory"
echo ""

# Check text directories
echo "=== Text/Help Directories ==="
check_dir "$BASE_DIR/lib/text" "755" "Text directory"
check_dir "$BASE_DIR/lib/text/help" "755" "Help files directory"
echo ""

# Check misc directories
echo "=== Miscellaneous Directories ==="
check_dir "$BASE_DIR/lib/misc" "755" "Miscellaneous directory"
check_dir "$BASE_DIR/lib/etc" "755" "Configuration directory"
echo ""

# Check executables
echo "=== Executable Files ==="
check_file "$BASE_DIR/bin/circle" "755" "Main game executable"
check_file "$BASE_DIR/bin/autowiz" "755" "Autowiz utility"
echo ""

# Check important configuration files
echo "=== Configuration Files ==="
check_file "$BASE_DIR/lib/etc/players" "644" "Player index file"
check_file "$BASE_DIR/lib/misc/messages" "644" "Combat messages"
check_file "$BASE_DIR/lib/misc/socials" "644" "Social commands"
check_file "$BASE_DIR/lib/misc/xnames" "644" "Banned names"
echo ""

# Check log files (need write access)
echo "=== Log Files (Write Required) ==="
if [ -d "$BASE_DIR/log" ]; then
    # Check if MUD user can write to log directory
    if [ -w "$BASE_DIR/log" ]; then
        echo -e "${GREEN}[OK]${NC} Log directory is writable"
    else
        echo -e "${RED}[ERROR]${NC} Log directory is NOT writable"
    fi
fi
echo ""

# Special checks
echo "=== Special Permission Checks ==="

# Check if syslog exists and is writable
if [ -f "$BASE_DIR/log/syslog" ]; then
    if [ -w "$BASE_DIR/log/syslog" ]; then
        echo -e "${GREEN}[OK]${NC} syslog is writable"
    else
        echo -e "${YELLOW}[WARN]${NC} syslog exists but is NOT writable"
    fi
else
    echo -e "${YELLOW}[INFO]${NC} syslog does not exist (will be created on startup)"
fi

# Check autorun script if it exists
if [ -f "$BASE_DIR/autorun" ]; then
    check_file "$BASE_DIR/autorun" "755" "Autorun script"
fi

echo ""
echo "=== Recommended Fixes ==="
echo ""
echo "To fix directory permissions:"
echo "  chmod 755 /path/to/directory"
echo ""
echo "To fix file permissions:"
echo "  chmod 644 /path/to/file     (for data files)"
echo "  chmod 755 /path/to/file     (for executables)"
echo ""
echo "To fix ownership (run as root):"
echo "  chown -R muduser:mudgroup /path/to/mud"
echo ""
echo "For player data directories that need write access:"
echo "  Ensure the user running the MUD has write permissions"
echo ""

# Additional SELinux check for CentOS
echo "=== SELinux Status (CentOS) ==="
if command -v getenforce &> /dev/null; then
    selinux_status=$(getenforce)
    if [ "$selinux_status" = "Enforcing" ]; then
        echo -e "${YELLOW}[WARN]${NC} SELinux is enforcing - may need additional configuration"
        echo "     You may need to set appropriate SELinux contexts or create policies"
    else
        echo -e "${GREEN}[OK]${NC} SELinux status: $selinux_status"
    fi
else
    echo "SELinux not found (not a concern)"
fi

echo ""
echo "================================================"
echo "Permission check complete!"
echo "================================================"