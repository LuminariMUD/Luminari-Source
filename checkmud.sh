#!/bin/bash
#
# LuminariMUD Process Monitor Script
# Copyright (c) 2025 LuminariMUD
#
# Description:
#   Monitors the LuminariMUD server process and automatically restarts it
#   if it's not running. This script is designed to be run as a daemon
#   by luminari.sh.
#
# Usage:
#   ./checkmud.sh
#
# Features:
#   - Checks if MUD process is running on configured port
#   - Archives core dumps with timestamps
#   - Preserves syslogs with date stamps
#   - Cleans up stale processes before restart
#   - Launches autorun script if MUD is down
#
# Exit Codes:
#   0 - Success (MUD is running or was successfully restarted)
#   1 - Configuration error
#   2 - Failed to change directory
#   3 - Failed to start autorun script
#
#############################################################################

# Enable strict error handling
set -euo pipefail

# Configuration
readonly SCRIPT_NAME="$(basename "$0")"
readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly MUD_DIR="${MUD_DIR:-${SCRIPT_DIR}}"
readonly MUD_BINARY="circle"
readonly MUD_PORT="4100"
readonly AUTORUN_SCRIPT="autorun.sh"
readonly DUMPS_DIR="dumps"
readonly CORE_FILE="lib/core"

# Date format patterns
readonly DATE_FORMAT_SYSLOG="%m-%d-%Y"
readonly DATE_FORMAT_DUMP="%m-%d-%Y-%H-%M"

# Logging function with timestamp
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] ${SCRIPT_NAME}: $*"
}

# Error handling function
die() {
    log "ERROR: $*" >&2
    exit "${2:-1}"
}

# Check if the MUD process is running
is_mud_running() {
    local process
    
    # Look for the MUD process running on the specified port
    # Using ps with grep pipeline to find the process
    # grep -v grep excludes the grep command itself from results
    process=$(ps auxwww | grep "${MUD_BINARY}" | grep " ${MUD_PORT}" | grep -v grep | awk '{print $11}')
    
    # Return success (0) if process found, failure (1) if not
    [[ -n "$process" ]]
}

# Archive core dump file with timestamp
archive_core_dump() {
    if [[ -f "$CORE_FILE" ]]; then
        local dump_name="core.$(date "+${DATE_FORMAT_DUMP}")"
        local dump_path="${DUMPS_DIR}/${dump_name}"
        
        # Create dumps directory if it doesn't exist
        mkdir -p "$DUMPS_DIR"
        
        log "Archiving core dump to ${dump_path}"
        if mv "$CORE_FILE" "$dump_path" 2>/dev/null; then
            log "Core dump archived successfully"
        else
            log "WARNING: Failed to archive core dump"
        fi
    fi
}

# Archive current syslog with date stamp
archive_syslog() {
    if [[ -f "syslog" ]]; then
        local syslog_name="syslog.$(date "+${DATE_FORMAT_SYSLOG}")"
        
        log "Archiving syslog to ${syslog_name}"
        if cp "syslog" "$syslog_name" 2>/dev/null; then
            log "Syslog archived successfully"
        else
            log "WARNING: Failed to archive syslog"
        fi
    fi
}

# Kill any lingering processes that might interfere with restart
cleanup_processes() {
    log "Cleaning up any lingering processes..."
    
    # Kill sleep processes (from autorun delays)
    if pgrep -x "sleep" >/dev/null 2>&1; then
        killall sleep 2>/dev/null || true
        log "Killed lingering sleep processes"
    fi
    
    # Kill any existing autorun scripts
    if pgrep -f "$AUTORUN_SCRIPT" >/dev/null 2>&1; then
        killall "$AUTORUN_SCRIPT" 2>/dev/null || true
        log "Killed existing autorun processes"
    fi
}

# Start the MUD server via autorun script
start_mud_server() {
    log "Starting MUD server via ${AUTORUN_SCRIPT}..."
    
    # Check if autorun script exists and is executable
    if [[ ! -x "./${AUTORUN_SCRIPT}" ]]; then
        die "Autorun script not found or not executable: ${AUTORUN_SCRIPT}" 3
    fi
    
    # Start the autorun script in background
    "./${AUTORUN_SCRIPT}" &
    local pid=$!
    
    log "MUD server started with autorun PID: ${pid}"
    
    # Give it a moment to start
    sleep 2
    
    # Verify it's still running
    if kill -0 "$pid" 2>/dev/null; then
        log "Autorun script confirmed running"
    else
        die "Autorun script failed to stay running" 3
    fi
}

#############################################################################
# Main Script
#############################################################################

# Change to MUD directory
log "Checking MUD status in ${MUD_DIR}"
cd "$MUD_DIR" || die "Failed to change to MUD directory: ${MUD_DIR}" 2

# Check if MUD is running
if is_mud_running; then
    log "MUD server is already running on port ${MUD_PORT}"
    echo "Server is already up"
    exit 0
fi

# MUD is not running - prepare for restart
log "MUD server is NOT running on port ${MUD_PORT} - initiating restart sequence"

# Set core dump size to unlimited for debugging
ulimit -c unlimited
log "Core dump size set to unlimited"

# Clean up before restart
cleanup_processes

# Archive any existing core dump
archive_core_dump

# Archive current syslog
archive_syslog

# Start the MUD server
start_mud_server

log "MUD restart sequence completed successfully"
exit 0