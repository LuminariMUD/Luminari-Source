#!/bin/bash
# Copyover Watchdog Script
# This script monitors for copyover failures and helps diagnose issues

COPYOVER_FILE="lib/misc/copyover.dat"
STATE_FILE="lib/misc/copyover_last_state.txt"
DIAG_LOG="../log/copyover_diagnostic.log"
PID_FILE="lib/misc/circle.pid"

# Function to check if MUD is running
is_mud_running() {
    if [ -f "$PID_FILE" ]; then
        PID=$(cat "$PID_FILE")
        if kill -0 "$PID" 2>/dev/null; then
            return 0
        fi
    fi
    return 1
}

# Function to analyze copyover state
analyze_copyover_state() {
    echo "=========================================="
    echo "COPYOVER STATE ANALYSIS"
    echo "=========================================="
    echo "Timestamp: $(date)"
    echo ""
    
    # Check if copyover file exists
    if [ -f "$COPYOVER_FILE" ]; then
        echo "WARNING: Copyover file still exists!"
        echo "  File size: $(stat -c%s "$COPYOVER_FILE" 2>/dev/null || stat -f%z "$COPYOVER_FILE" 2>/dev/null) bytes"
        echo "  Modified: $(stat -c%y "$COPYOVER_FILE" 2>/dev/null || stat -f "%Sm" "$COPYOVER_FILE" 2>/dev/null)"
        echo "  This indicates copyover may have failed during execl()"
        echo ""
    else
        echo "Copyover file: Not found (normal after successful copyover)"
        echo ""
    fi
    
    # Check last state file
    if [ -f "$STATE_FILE" ]; then
        echo "Last copyover state:"
        cat "$STATE_FILE" | sed 's/^/  /'
        echo ""
    else
        echo "No state file found"
        echo ""
    fi
    
    # Check if MUD is running
    if is_mud_running; then
        echo "MUD Status: RUNNING (PID: $(cat "$PID_FILE"))"
    else
        echo "MUD Status: NOT RUNNING"
    fi
    echo ""
    
    # Check system resources
    echo "System Resources:"
    echo "  Memory: $(free -h 2>/dev/null | grep Mem: | awk '{print "Used: " $3 " / Total: " $2}' || echo "N/A")"
    echo "  Load Average: $(uptime | awk -F'load average:' '{print $2}')"
    echo "  Disk Space (current dir): $(df -h . | tail -1 | awk '{print "Used: " $3 " / Total: " $2 " (" $5 " used)"}')"
    echo ""
    
    # Check file descriptor limits
    echo "File Descriptor Limits:"
    if [ -f "/proc/sys/fs/file-nr" ]; then
        USED_FDS=$(cat /proc/sys/fs/file-nr | awk '{print $1}')
        MAX_FDS=$(cat /proc/sys/fs/file-nr | awk '{print $3}')
        echo "  System-wide: $USED_FDS / $MAX_FDS"
    fi
    ulimit -n | awk '{print "  Process limit: " $1}'
    echo ""
    
    # Recent errors from diagnostic log
    if [ -f "$DIAG_LOG" ]; then
        echo "Recent diagnostic entries:"
        tail -20 "$DIAG_LOG" | grep -E "(FAILED|ERROR|SYSERR)" | tail -5 | sed 's/^/  /'
        echo ""
    fi
}

# Function to clean up after failed copyover
cleanup_failed_copyover() {
    echo "Cleaning up after failed copyover..."
    
    if [ -f "$COPYOVER_FILE" ]; then
        echo "  Backing up copyover file to copyover.dat.failed"
        cp "$COPYOVER_FILE" "${COPYOVER_FILE}.failed"
        rm -f "$COPYOVER_FILE"
        echo "  Removed stale copyover file"
    fi
    
    if [ -f "$STATE_FILE" ]; then
        echo "  Backing up state file to copyover_last_state.txt.bak"
        cp "$STATE_FILE" "${STATE_FILE}.bak"
    fi
    
    echo "Cleanup complete"
}

# Main script
case "$1" in
    check)
        analyze_copyover_state
        ;;
    cleanup)
        cleanup_failed_copyover
        ;;
    monitor)
        # Continuous monitoring mode
        echo "Starting copyover monitor (press Ctrl+C to stop)..."
        while true; do
            if [ -f "$COPYOVER_FILE" ]; then
                # Check if file is older than 60 seconds
                if [ $(($(date +%s) - $(stat -c%Y "$COPYOVER_FILE" 2>/dev/null || stat -f%m "$COPYOVER_FILE" 2>/dev/null))) -gt 60 ]; then
                    echo "$(date): WARNING - Stale copyover file detected!"
                    analyze_copyover_state
                fi
            fi
            sleep 10
        done
        ;;
    *)
        echo "Usage: $0 {check|cleanup|monitor}"
        echo ""
        echo "  check   - Analyze current copyover state"
        echo "  cleanup - Clean up after failed copyover"
        echo "  monitor - Continuously monitor for copyover issues"
        exit 1
        ;;
esac