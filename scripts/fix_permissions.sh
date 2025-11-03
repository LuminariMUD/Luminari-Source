#!/bin/bash

# LuminariMUD Permission Fix Script
# This script fixes common permission issues for MUD servers

echo "================================================"
echo "LuminariMUD Permission Fix Script"
echo "================================================"
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check if running with sufficient privileges
if [ "$EUID" -ne 0 ] && [ -z "$1" ]; then 
    echo -e "${YELLOW}Warning:${NC} Not running as root. Some fixes may require sudo."
    echo "Usage: $0 [base_directory]"
    echo "Or run with sudo for full permission fixes"
    echo ""
fi

# Get base directory
if [ -n "$1" ]; then
    BASE_DIR="$1"
else
    BASE_DIR=$(dirname $(pwd))
fi

echo "Base directory: $BASE_DIR"
echo ""

# Ask for confirmation
read -p "This will fix permissions in $BASE_DIR. Continue? (y/N) " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborted."
    exit 1
fi

echo ""
echo "Fixing permissions..."
echo ""

# Fix main directories
echo "=== Fixing Directory Permissions ==="
for dir in "$BASE_DIR" "$BASE_DIR/bin" "$BASE_DIR/lib" "$BASE_DIR/log"; do
    if [ -d "$dir" ]; then
        chmod 755 "$dir" 2>/dev/null && echo -e "${GREEN}[FIXED]${NC} $dir" || echo -e "${RED}[FAILED]${NC} $dir"
    fi
done

# Fix world directories
for dir in "$BASE_DIR/lib/world" "$BASE_DIR/lib/world/mob" "$BASE_DIR/lib/world/obj" \
           "$BASE_DIR/lib/world/wld" "$BASE_DIR/lib/world/zon" "$BASE_DIR/lib/world/shp" \
           "$BASE_DIR/lib/world/trg" "$BASE_DIR/lib/world/qst"; do
    if [ -d "$dir" ]; then
        chmod 755 "$dir" 2>/dev/null && echo -e "${GREEN}[FIXED]${NC} $dir" || echo -e "${RED}[FAILED]${NC} $dir"
    fi
done

# Fix player directories (need write access)
for dir in "$BASE_DIR/lib/plrfiles" "$BASE_DIR/lib/plrmail" "$BASE_DIR/lib/plrobjs" \
           "$BASE_DIR/lib/plrvars" "$BASE_DIR/lib/plralias" "$BASE_DIR/lib/house"; do
    if [ -d "$dir" ]; then
        chmod 755 "$dir" 2>/dev/null && echo -e "${GREEN}[FIXED]${NC} $dir" || echo -e "${RED}[FAILED]${NC} $dir"
    fi
done

# Fix text and misc directories
for dir in "$BASE_DIR/lib/text" "$BASE_DIR/lib/text/help" "$BASE_DIR/lib/misc" "$BASE_DIR/lib/etc"; do
    if [ -d "$dir" ]; then
        chmod 755 "$dir" 2>/dev/null && echo -e "${GREEN}[FIXED]${NC} $dir" || echo -e "${RED}[FAILED]${NC} $dir"
    fi
done

echo ""
echo "=== Fixing File Permissions ==="

# Fix executables
if [ -f "$BASE_DIR/bin/circle" ]; then
    chmod 755 "$BASE_DIR/bin/circle" 2>/dev/null && echo -e "${GREEN}[FIXED]${NC} Main executable" || echo -e "${RED}[FAILED]${NC} Main executable"
fi

if [ -f "$BASE_DIR/bin/autowiz" ]; then
    chmod 755 "$BASE_DIR/bin/autowiz" 2>/dev/null && echo -e "${GREEN}[FIXED]${NC} Autowiz utility" || echo -e "${RED}[FAILED]${NC} Autowiz utility"
fi

if [ -f "$BASE_DIR/autorun" ]; then
    chmod 755 "$BASE_DIR/autorun" 2>/dev/null && echo -e "${GREEN}[FIXED]${NC} Autorun script" || echo -e "${RED}[FAILED]${NC} Autorun script"
fi

# Fix data files
echo ""
echo "=== Fixing Data File Permissions ==="

# Fix all files in world directories
find "$BASE_DIR/lib/world" -type f -exec chmod 644 {} \; 2>/dev/null && \
    echo -e "${GREEN}[FIXED]${NC} World data files" || echo -e "${RED}[FAILED]${NC} World data files"

# Fix text files
find "$BASE_DIR/lib/text" -type f -exec chmod 644 {} \; 2>/dev/null && \
    echo -e "${GREEN}[FIXED]${NC} Text files" || echo -e "${RED}[FAILED]${NC} Text files"

# Fix misc files
find "$BASE_DIR/lib/misc" -type f -exec chmod 644 {} \; 2>/dev/null && \
    echo -e "${GREEN}[FIXED]${NC} Misc files" || echo -e "${RED}[FAILED]${NC} Misc files"

# Create missing directories
echo ""
echo "=== Creating Missing Directories ==="
for dir in "$BASE_DIR/log" "$BASE_DIR/lib/plrfiles" "$BASE_DIR/lib/plrmail" \
           "$BASE_DIR/lib/plrobjs" "$BASE_DIR/lib/plrvars" "$BASE_DIR/lib/plralias"; do
    if [ ! -d "$dir" ]; then
        mkdir -p "$dir" 2>/dev/null && chmod 755 "$dir" && \
            echo -e "${GREEN}[CREATED]${NC} $dir" || echo -e "${RED}[FAILED]${NC} $dir"
    fi
done

# SELinux fixes for CentOS
if command -v getenforce &> /dev/null; then
    if [ "$(getenforce)" = "Enforcing" ]; then
        echo ""
        echo "=== SELinux Configuration ==="
        echo -e "${YELLOW}[INFO]${NC} SELinux is enforcing. You may need to:"
        echo "  1. Set context: chcon -R -t httpd_sys_content_t $BASE_DIR"
        echo "  2. Or disable for testing: setenforce 0"
        echo "  3. Or create proper SELinux policy for your MUD"
    fi
fi

echo ""
echo "================================================"
echo "Permission fixes complete!"
echo ""
echo "Next steps:"
echo "1. Run ./check_permissions.sh to verify"
echo "2. Ensure the MUD user owns the files:"
echo "   sudo chown -R muduser:mudgroup $BASE_DIR"
echo "3. Test by starting your MUD server"
echo "================================================"