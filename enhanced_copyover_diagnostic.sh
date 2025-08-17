#!/bin/bash

# Enhanced copyover diagnostic script
# This script monitors and diagnoses copyover issues

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
if [ -f "/home/luminari/Luminari-Source/lib/copyover.dat" ]; then
    echo "  [FOUND] /home/luminari/Luminari-Source/lib/copyover.dat"
    ls -la /home/luminari/Luminari-Source/lib/copyover.dat
else
    echo "  [NOT FOUND] /home/luminari/Luminari-Source/lib/copyover.dat"
fi

if [ -f "/home/luminari/Luminari-Source/copyover.dat" ]; then
    echo "  [FOUND] /home/luminari/Luminari-Source/copyover.dat"
    ls -la /home/luminari/Luminari-Source/copyover.dat
else
    echo "  [NOT FOUND] /home/luminari/Luminari-Source/copyover.dat"
fi

# Check for temp files
echo ""
echo "Checking for temporary copyover files:"
if [ -f "/home/luminari/Luminari-Source/lib/copyover.dat.tmp" ]; then
    echo "  [FOUND] /home/luminari/Luminari-Source/lib/copyover.dat.tmp"
    ls -la /home/luminari/Luminari-Source/lib/copyover.dat.tmp
else
    echo "  [NOT FOUND] /home/luminari/Luminari-Source/lib/copyover.dat.tmp"
fi

# Check binary location and permissions
echo ""
echo "Checking game binary:"
if [ -f "/home/luminari/Luminari-Source/bin/circle" ]; then
    echo "  [FOUND] /home/luminari/Luminari-Source/bin/circle"
    ls -la /home/luminari/Luminari-Source/bin/circle
    if [ -x "/home/luminari/Luminari-Source/bin/circle" ]; then
        echo "  [OK] Binary is executable"
    else
        echo "  [ERROR] Binary is NOT executable"
    fi
else
    echo "  [NOT FOUND] /home/luminari/Luminari-Source/bin/circle"
fi

# Check directory permissions
echo ""
echo "Checking directory permissions:"
echo "  lib directory:"
ls -ld /home/luminari/Luminari-Source/lib/
echo "  parent directory:"
ls -ld /home/luminari/Luminari-Source/

# Check recent diagnostic logs
echo ""
echo "Recent diagnostic logs:"
echo "----------------------"
if [ -f "/home/luminari/Luminari-Source/log/copyover_diagnostic.log" ]; then
    echo "Last 20 lines of diagnostic log:"
    tail -20 /home/luminari/Luminari-Source/log/copyover_diagnostic.log
fi

echo ""
echo "Last state file:"
if [ -f "/home/luminari/Luminari-Source/log/copyover_last_state.txt" ]; then
    cat /home/luminari/Luminari-Source/log/copyover_last_state.txt
fi

echo ""
echo "=========================================="
echo "DIAGNOSTIC COMPLETE"
echo "=========================================="