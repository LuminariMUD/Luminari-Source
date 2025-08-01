#!/bin/bash
#
# LuminariMUD Master Startup Script
# Copyright (c) 2025 LuminariMUD
#
# Description:
#   Main startup script for LuminariMUD server and associated services.
#   This script initializes the environment and launches all required daemons.
#
# Usage:
#   ./luminari.sh
#
# Services Started:
#   1. Flash policy daemon (port 843) for web-based MUD clients
#   2. HTML5 WebSocket policy daemon
#   3. MUD monitoring daemon (checkmud.sh)
#
# Exit Codes:
#   0 - Success
#   1 - Directory not found
#   2 - Required file not found
#
#############################################################################

# Enable strict error handling
set -euo pipefail

# Configuration
readonly SCRIPT_NAME="$(basename "$0")"
readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Allow environment variables to override default paths
readonly HMUD_DIR="${HMUD_DIR:-/home/luminari/public_html/hmud}"
readonly FMUD_DIR="${FMUD_DIR:-/home/luminari/public_html/FMud}"
readonly MUD_DIR="${MUD_DIR:-${SCRIPT_DIR}}"
readonly FLASH_POLICY_PORT=843
readonly FLASH_POLICY_FILE="${FLASH_POLICY_FILE:-${SCRIPT_DIR}/flashpolicy.xml}"

# Logging function
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] ${SCRIPT_NAME}: $*"
}

# Error handling function
die() {
    log "ERROR: $*" >&2
    exit 1
}

# Check if directory exists and is accessible
check_directory() {
    local dir="$1"
    local service="$2"
    
    if [[ ! -d "$dir" ]]; then
        log "WARNING: ${service} directory not found: ${dir} (skipping)"
        return 1
    fi
    
    if [[ ! -r "$dir" ]] || [[ ! -x "$dir" ]]; then
        log "WARNING: ${service} directory not accessible: ${dir} (skipping)"
        return 1
    fi
    
    return 0
}

# Start a background service
start_service() {
    local service_name="$1"
    local service_cmd="$2"
    
    log "Starting ${service_name}..."
    
    # Check if command exists before trying to run it
    if command -v "${service_cmd%% *}" >/dev/null 2>&1; then
        nohup ${service_cmd} >/dev/null 2>&1 &
        log "${service_name} started with PID $!"
    else
        log "WARNING: ${service_name} command not found: ${service_cmd%% *}"
    fi
}

#############################################################################
# Main Script
#############################################################################

log "Starting LuminariMUD services..."

# Set core dump size to unlimited for debugging
ulimit -c unlimited
log "Core dump size set to unlimited"

# Start HTML5 WebSocket policy daemon
if check_directory "$HMUD_DIR" "HTML5 MUD"; then
    cd "$HMUD_DIR" || die "Failed to change to HMUD directory"
    
    if [[ -x "./policyd" ]]; then
        start_service "HTML5 WebSocket policy daemon" "./policyd"
    else
        log "WARNING: policyd not found or not executable in ${HMUD_DIR}"
    fi
fi

# Start Flash policy daemon (for legacy Flash clients)
if check_directory "$FMUD_DIR" "Flash MUD"; then
    cd "$FMUD_DIR" || die "Failed to change to FMUD directory"
    
    if [[ -x "./flashpolicyd.py" ]]; then
        # Check if policy file exists
        if [[ ! -f "$FLASH_POLICY_FILE" ]]; then
            log "WARNING: Flash policy file not found: ${FLASH_POLICY_FILE}"
            log "Flash policy daemon may not function correctly"
        fi
        
        start_service "Flash policy daemon" "./flashpolicyd.py --file=${FLASH_POLICY_FILE} --port=${FLASH_POLICY_PORT}"
    else
        log "WARNING: flashpolicyd.py not found or not executable in ${FMUD_DIR}"
    fi
fi

# Start MUD monitoring daemon
if check_directory "$MUD_DIR" "MUD"; then
    cd "$MUD_DIR" || die "Failed to change to MUD directory"
    
    if [[ -x "./checkmud.sh" ]]; then
        start_service "MUD monitoring daemon" "./checkmud.sh"
    else
        die "checkmud.sh not found or not executable in ${MUD_DIR}"
    fi
else
    die "MUD directory not found: ${MUD_DIR}"
fi

log "All services started successfully"
log "To check service status, use: ps aux | grep -E '(policyd|flashpolicyd|checkmud)'"

exit 0