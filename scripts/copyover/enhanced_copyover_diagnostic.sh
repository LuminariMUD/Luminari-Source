#!/bin/bash

# Enhanced copyover diagnostic script
# This script monitors and diagnoses copyover issues

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=========================================="
echo "ENHANCED COPYOVER DIAGNOSTIC"
echo "=========================================="
echo ""

# Check if the game is running
if pgrep -f "circle" > /dev/null; then
    echo "[OK] Game process is running"
    echo "PID: $(pgrep -f circle)"
else
    echo "[WARNING] Game process not found"
fi

echo ""
echo "Checking file system state..."
echo "------------------------------"

# Check copyover.dat in both locations
echo "Checking for copyover.dat files:"
if [ -f "$PROJECT_ROOT/lib/copyover.dat" ]; then
    echo "  [FOUND] $PROJECT_ROOT/lib/copyover.dat"
    ls -la "$PROJECT_ROOT/lib/copyover.dat"
else
    echo "  [NOT FOUND] $PROJECT_ROOT/lib/copyover.dat"
fi

if [ -f "$PROJECT_ROOT/copyover.dat" ]; then
    echo "  [FOUND] $PROJECT_ROOT/copyover.dat"
    ls -la "$PROJECT_ROOT/copyover.dat"
else
    echo "  [NOT FOUND] $PROJECT_ROOT/copyover.dat"
fi

# Check for temp files
echo ""
echo "Checking for temporary copyover files:"
if [ -f "$PROJECT_ROOT/lib/copyover.dat.tmp" ]; then
    echo "  [FOUND] $PROJECT_ROOT/lib/copyover.dat.tmp"
    ls -la "$PROJECT_ROOT/lib/copyover.dat.tmp"
else
    echo "  [NOT FOUND] $PROJECT_ROOT/lib/copyover.dat.tmp"
fi

# Check binary location and permissions
echo ""
echo "Checking game binary:"
if [ -f "$PROJECT_ROOT/bin/circle" ]; then
    echo "  [FOUND] $PROJECT_ROOT/bin/circle"
    ls -la "$PROJECT_ROOT/bin/circle"
    if [ -x "$PROJECT_ROOT/bin/circle" ]; then
        echo "  [OK] Binary is executable"
    else
        echo "  [ERROR] Binary is NOT executable"
    fi
else
    echo "  [NOT FOUND] $PROJECT_ROOT/bin/circle"
fi

# Check directory permissions
echo ""
echo "Checking directory permissions:"
echo "  lib directory:"
ls -ld "$PROJECT_ROOT/lib/"
echo "  parent directory:"
ls -ld "$PROJECT_ROOT/"

# Check recent diagnostic logs
echo ""
echo "Recent diagnostic logs:"
echo "----------------------"
if [ -f "$PROJECT_ROOT/log/copyover_diagnostic.log" ]; then
    echo "Last 20 lines of diagnostic log:"
    tail -20 "$PROJECT_ROOT/log/copyover_diagnostic.log"
fi

echo ""
echo "Last state file:"
if [ -f "$PROJECT_ROOT/log/copyover_last_state.txt" ]; then
    cat "$PROJECT_ROOT/log/copyover_last_state.txt"
fi

echo ""
echo "=========================================="
echo "DIAGNOSTIC COMPLETE"
echo "=========================================="
