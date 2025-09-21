#!/bin/bash

################################################################################
# LuminariMUD Setup Script
# 
# Automated deployment script that builds and configures LuminariMUD from source.
# This script handles all necessary steps for a complete deployment.
#
# Usage: ./scripts/setup.sh
################################################################################

set -e  # Exit on error

# Color codes for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}LuminariMUD Deployment${NC}"
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

# Check if we need to run autoreconf first
if [[ ! -f Makefile ]]; then
    echo "  Generating build system..."
    autoreconf -fvi || { echo -e "${RED}Failed to generate build system!${NC}"; exit 1; }
    ./configure || { echo -e "${RED}Configure failed!${NC}"; exit 1; }
fi

make clean 2>/dev/null || true
make -j$(nproc) || make || { echo -e "${RED}Build failed!${NC}"; exit 1; }
make install || { echo -e "${RED}Install failed!${NC}"; exit 1; }
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

# Copy minimal world files and create proper indexes
if [[ -d lib/world/minimal ]]; then
    # Zone files
    if [[ -f lib/world/minimal/0.zon ]]; then
        cp lib/world/minimal/*.zon lib/world/zon/ 2>/dev/null || true
        echo "0.zon" > lib/world/zon/index
        echo '$' >> lib/world/zon/index
    else
        echo '$' > lib/world/zon/index
    fi
    
    # World/room files
    if [[ -f lib/world/minimal/0.wld ]]; then
        cp lib/world/minimal/*.wld lib/world/wld/ 2>/dev/null || true
        echo "0.wld" > lib/world/wld/index
        echo '$' >> lib/world/wld/index
    else
        echo '$' > lib/world/wld/index
    fi
    
    # Mob files
    if [[ -f lib/world/minimal/0.mob ]]; then
        cp lib/world/minimal/*.mob lib/world/mob/ 2>/dev/null || true
        echo "0.mob" > lib/world/mob/index
        echo '$' >> lib/world/mob/index
    else
        echo '$' > lib/world/mob/index
    fi
    
    # Object files - create a minimal one if it doesn't exist
    if [[ -f lib/world/minimal/0.obj ]]; then
        cp lib/world/minimal/*.obj lib/world/obj/ 2>/dev/null || true
        echo "0.obj" > lib/world/obj/index
        echo '$' >> lib/world/obj/index
    else
        # Create a minimal object file
        cat > lib/world/obj/0.obj << 'EOF'
#1
bread~
a loaf of bread~
A loaf of bread is here.~
~
11 0 0 0 0 a 0 0 0 0 0 0 0
0 0 0 0
1 100 10 0 0
$
EOF
        echo "0.obj" > lib/world/obj/index
        echo '$' >> lib/world/obj/index
    fi
    
    # Shop files
    if [[ -f lib/world/minimal/0.shp ]]; then
        cp lib/world/minimal/*.shp lib/world/shp/ 2>/dev/null || true
        ls lib/world/shp/*.shp 2>/dev/null | xargs -n1 basename > lib/world/shp/index 2>/dev/null
        echo '$' >> lib/world/shp/index
    else
        echo '$' > lib/world/shp/index
    fi
    
    # Trigger files
    if [[ -f lib/world/minimal/0.trg ]]; then
        cp lib/world/minimal/*.trg lib/world/trg/ 2>/dev/null || true
        ls lib/world/trg/*.trg 2>/dev/null | xargs -n1 basename > lib/world/trg/index 2>/dev/null
        echo '$' >> lib/world/trg/index
    else
        echo '$' > lib/world/trg/index
    fi
    
    # Quest files
    if [[ -f lib/world/minimal/0.qst ]]; then
        cp lib/world/minimal/*.qst lib/world/qst/ 2>/dev/null || true
        ls lib/world/qst/*.qst 2>/dev/null | xargs -n1 basename > lib/world/qst/index 2>/dev/null
        echo '$' >> lib/world/qst/index
    else
        echo '$' > lib/world/qst/index
    fi
    
    echo "  Set up minimal world files"
else
    echo -e "${YELLOW}  Warning: No minimal world directory found${NC}"
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