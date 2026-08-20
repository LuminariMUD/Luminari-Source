#!/bin/bash
# Clean and rebuild with progress bar, logging output to build/build.log

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT" || exit 1

# Colors for pretty output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}═══════════════════════════════════════════════════${NC}"
echo -e "${BLUE}       LuminariMUD Clean Build with Progress        ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════${NC}"

# Check if build directory exists and is configured
if [ ! -f build/CMakeCache.txt ]; then
    echo -e "${YELLOW}▶ First time setup - configuring CMake...${NC}"
    mkdir -p build
    cmake -S . -B build/ > build/cmake_config.log 2>&1 &
    pid=$!
    while kill -0 $pid 2>/dev/null; do
        echo -n "."
        sleep 2
    done
    wait $pid
    if [ $? -ne 0 ]; then
        echo ""
        echo "❌ Configuration failed! Check build/cmake_config.log"
        exit 1
    fi
    echo ""
    echo -e "${GREEN}✓ Configuration complete${NC}"
fi

# Clean previous build
echo -e "${YELLOW}▶ Cleaning previous build...${NC}"
cmake --build build/ --target clean > /dev/null 2>&1
echo -e "${GREEN}✓ Clean complete${NC}"

# Build with progress tracking
echo -e "${GREEN}▶ Building... (output → build/build.log)${NC}"
echo ""

# Use cmake's built-in progress and redirect verbose output
# Detect if running in WSL and be more conservative with resources
CORES=$(nproc)
if grep -qi microsoft /proc/version; then
    # WSL detected - use 1/3 of cores for better system responsiveness
    JOBS=$((CORES / 3))
    echo -e "${YELLOW}WSL detected - using conservative parallelism${NC}"
else
    # Native Linux - use half the cores
    JOBS=$((CORES / 2))
fi
# Ensure at least 2 jobs
if [ $JOBS -lt 2 ]; then
    JOBS=2
fi
echo -e "${BLUE}Building with $JOBS parallel jobs (out of $CORES cores)${NC}"
cmake --build build/ -j$JOBS > build/build_raw.log 2>&1 &
BUILD_PID=$!

# Monitor the build progress
hit_100=false
while kill -0 $BUILD_PID 2>/dev/null; do
    if [ -f build/build_raw.log ]; then
        # Get latest percentage from build log
        percent=$(grep -o '\[[ ]*[0-9]\+%\]' build/build_raw.log | tail -1 | grep -o '[0-9]\+')
        if [ ! -z "$percent" ]; then
            # Create progress bar
            filled=$((percent / 2))
            empty=$((50 - filled))
            bar=$(printf '█%.0s' $(seq 1 $filled))
            spaces=$(printf '░%.0s' $(seq 1 $empty))
            printf "\r${GREEN}Progress: [${bar}${spaces}] ${percent}%%${NC}"

            # When we hit 100%, show the message
            if [ "$percent" = "100" ] && [ "$hit_100" = "false" ]; then
                hit_100=true
                printf "\n${YELLOW}💪 Moment, doing some push ups...${NC}\n"
            fi
        fi
    fi
    sleep 0.5
done

# Wait for build to complete and get exit status
wait $BUILD_PID
BUILD_RESULT=$?

# Clear progress line
printf "\r${GREEN}Progress: [%-50s] 100%%${NC}\n" "██████████████████████████████████████████████████"

# Move raw log to final location
mv build/build_raw.log build/build.log 2>/dev/null

# Check for errors/warnings in log
if grep -q "error:" build/build.log; then
    echo -e "\033[0;31m❌ Compilation errors found in build/build.log${NC}"
elif grep -q "warning:" build/build.log; then
    echo -e "${YELLOW}⚠ Build completed with warnings (see build/build.log)${NC}"
fi

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════${NC}"
if [ -f bin/luminari ]; then
    echo -e "${GREEN}✅ Build successful!${NC}"
    echo -e "${GREEN}   Binary: bin/luminari${NC}"
    echo -e "${GREEN}   Full log: build/build.log${NC}"
else
    echo -e "\033[0;31m❌ Build may have failed. Check build/build.log${NC}"
fi
echo -e "${BLUE}═══════════════════════════════════════════════════${NC}"
