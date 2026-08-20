#!/bin/bash

# Enhanced copyover diagnostic script
# This script monitors and diagnoses copyover issues

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=========================================="
echo "ENHANCED COPYOVER DIAGNOSTIC"
echo "=========================================="
echo ""

# Check if the game is running.  The project PID file is the only authority:
# a process-name probe can select an unrelated MUD on a shared host.
MUD_PID_FILE="$PROJECT_ROOT/.mud.pid"
mud_pid=""
if [ -r "$MUD_PID_FILE" ]; then
    IFS= read -r mud_pid < "$MUD_PID_FILE" || true
fi
if [ -n "$mud_pid" ] && [ -d "/proc/$mud_pid" ]; then
    mud_exe="$(readlink -f -- "/proc/$mud_pid/exe" 2>/dev/null || true)"
    case "$mud_exe" in
        "$PROJECT_ROOT/bin/"*)
            echo "[OK] Game process is running"
            echo "PID: $mud_pid"
            echo "Executable: $mud_exe"
            ;;
        *)
            echo "[WARNING] $MUD_PID_FILE names PID $mud_pid, which is not running"
            echo "          an executable from this checkout: ${mud_exe:-unreadable}"
            ;;
    esac
else
    echo "[WARNING] Game process not found (no usable $MUD_PID_FILE)"
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
if [ -f "$PROJECT_ROOT/bin/luminari" ]; then
    echo "  [FOUND] $PROJECT_ROOT/bin/luminari"
    ls -la "$PROJECT_ROOT/bin/luminari"
    if [ -x "$PROJECT_ROOT/bin/luminari" ]; then
        echo "  [OK] Binary is executable"
    else
        echo "  [ERROR] Binary is NOT executable"
    fi
else
    echo "  [NOT FOUND] $PROJECT_ROOT/bin/luminari"
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
