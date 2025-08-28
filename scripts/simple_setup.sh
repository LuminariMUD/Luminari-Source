#!/bin/bash

################################################################################
# LuminariMUD Simple Setup Script
# 
# A minimal script that sets up LuminariMUD from a fresh clone.
# This script focuses on getting the MUD running as quickly as possible.
#
# Usage: ./scripts/simple_setup.sh
################################################################################

set -e  # Exit on error

# Color codes for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}LuminariMUD Simple Setup${NC}"
echo -e "${GREEN}========================================${NC}"

# Get to project root
cd "$(dirname "$0")/.."

# Step 1: Copy configuration files if they don't exist
echo -e "\n${GREEN}Step 1: Setting up configuration files...${NC}"
if [[ ! -f src/campaign.h ]]; then
    cp src/campaign.example.h src/campaign.h
    echo "  Created src/campaign.h"
fi
if [[ ! -f src/mud_options.h ]]; then
    cp src/mud_options.example.h src/mud_options.h
    echo "  Created src/mud_options.h"
fi
if [[ ! -f src/vnums.h ]]; then
    cp src/vnums.example.h src/vnums.h
    echo "  Created src/vnums.h"
fi

# Step 2: Build the game
echo -e "\n${GREEN}Step 2: Building the MUD...${NC}"
make clean 2>/dev/null || true
make -j$(nproc) || make || { echo -e "${RED}Build failed!${NC}"; exit 1; }
echo "  Build completed successfully!"

# Step 3: Create critical symlinks
echo -e "\n${GREEN}Step 3: Creating symlinks...${NC}"
if [[ ! -L world ]]; then
    ln -sf lib/world world
    echo "  Created symlink: world -> lib/world"
fi
if [[ ! -L text ]]; then
    ln -sf lib/text text
    echo "  Created symlink: text -> lib/text"
fi
if [[ ! -L etc ]]; then
    ln -sf lib/etc etc
    echo "  Created symlink: etc -> lib/etc"
fi

# Step 4: Copy world files
echo -e "\n${GREEN}Step 4: Setting up world files...${NC}"
for dir in zon wld mob obj shp trg qst hlq; do
    mkdir -p lib/world/${dir}
done

# Copy minimal world files with proper renaming
if [[ -d lib/world/minimal ]]; then
    # Zone files
    cp lib/world/minimal/index.zon lib/world/zon/index 2>/dev/null || true
    cp lib/world/minimal/*.zon lib/world/zon/ 2>/dev/null || true
    
    # World/room files
    cp lib/world/minimal/index.wld lib/world/wld/index 2>/dev/null || true
    cp lib/world/minimal/*.wld lib/world/wld/ 2>/dev/null || true
    
    # Mob files
    cp lib/world/minimal/index.mob lib/world/mob/index 2>/dev/null || true
    cp lib/world/minimal/*.mob lib/world/mob/ 2>/dev/null || true
    
    # Object files
    cp lib/world/minimal/index.obj lib/world/obj/index 2>/dev/null || true
    cp lib/world/minimal/*.obj lib/world/obj/ 2>/dev/null || true
    
    # Other index files
    cp lib/world/minimal/index.shp lib/world/shp/index 2>/dev/null || true
    cp lib/world/minimal/index.trg lib/world/trg/index 2>/dev/null || true
    cp lib/world/minimal/index.qst lib/world/qst/index 2>/dev/null || true
    
    echo "  Copied minimal world files"
else
    echo -e "${YELLOW}  Warning: No minimal world files found, creating empty indexes${NC}"
    echo '$' > lib/world/zon/index
    echo '$' > lib/world/wld/index
    echo '$' > lib/world/mob/index
    echo '$' > lib/world/obj/index
    echo '$' > lib/world/shp/index
    echo '$' > lib/world/trg/index
    echo '$' > lib/world/qst/index
fi

# HLQ index (always create)
echo '$' > lib/world/hlq/index

# Step 5: Create text files
echo -e "\n${GREEN}Step 5: Creating text files...${NC}"
mkdir -p lib/text/help lib/etc

# Create basic text files if they don't exist
[[ ! -f lib/text/news ]] && echo "Welcome to LuminariMUD!" > lib/text/news
[[ ! -f lib/text/credits ]] && echo "LuminariMUD Credits" > lib/text/credits
[[ ! -f lib/text/motd ]] && echo "Message of the Day" > lib/text/motd
[[ ! -f lib/text/imotd ]] && echo "Immortal MOTD" > lib/text/imotd
[[ ! -f lib/text/help/help ]] && echo "Help" > lib/text/help/help
[[ ! -f lib/text/help/ihelp ]] && echo "Immortal Help" > lib/text/help/ihelp
[[ ! -f lib/text/info ]] && echo "Info" > lib/text/info
[[ ! -f lib/text/wizlist ]] && echo "Wizard List" > lib/text/wizlist
[[ ! -f lib/text/immlist ]] && echo "Immortal List" > lib/text/immlist
[[ ! -f lib/text/policies ]] && echo "Policies" > lib/text/policies
[[ ! -f lib/text/handbook ]] && echo "Handbook" > lib/text/handbook
[[ ! -f lib/text/background ]] && echo "Background" > lib/text/background
[[ ! -f lib/text/greetings ]] && echo "Welcome!" > lib/text/greetings

# Create help index
echo '$' > lib/text/help/index

# Create minimal config if it doesn't exist
if [[ ! -f lib/etc/config ]]; then
    echo "# Minimal LuminariMUD config" > lib/etc/config
    echo "  Created minimal config file"
fi

# Step 6: Create required directories
echo -e "\n${GREEN}Step 6: Creating required directories...${NC}"
mkdir -p lib/plrfiles/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
mkdir -p lib/plrobjs/{A-E,F-J,K-O,P-T,U-Z,ZZZ}
mkdir -p lib/house
mkdir -p lib/mudmail
mkdir -p log

echo -e "\n${GREEN}========================================${NC}"
echo -e "${GREEN}Setup complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "To start the MUD, run:"
echo "  ./bin/circle -d lib"
echo ""
echo "Default port is 4000. Connect with:"
echo "  telnet localhost 4000"
echo ""
echo "For more options, see: ./bin/circle -h"