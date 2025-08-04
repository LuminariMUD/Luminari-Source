#!/bin/bash
#
# LuminariMUD Enhanced Autorun Script
# Originally based on CircleMUD 3.0 autorun script
# Contributions by Fred Merkel, Stuart Lamble, and Jeremy Elson
# Enhanced with features from luminari.sh and checkmud.sh
# Copyright (c) 1996 The Trustees of The Johns Hopkins University
# Copyright (c) 2025 LuminariMUD
# All Rights Reserved
# See license.doc for more information
#
#############################################################################
#
# This script runs LuminariMUD continuously, automatically rebooting if it
# crashes. It also manages auxiliary services like websocket policy daemons.
#
# Control files:
#   .fastboot   - Makes the script wait only 5 seconds between reboots
#   .killscript - Makes the script terminate (stop rebooting the MUD)
#   pause       - Pauses rebooting until the file is removed
#
# Commands from within the MUD:
#   shutdown reboot - Quick reboot (creates .fastboot)
#   shutdown die    - Stop the MUD and autorun (creates .killscript)
#   shutdown pause  - Pause autorun (creates pause file)
#
#############################################################################

# Enable strict error handling for debugging
set -euo pipefail

#############################################################################
# Configuration Section
#############################################################################

# Script identification
readonly SCRIPT_NAME="$(basename "$0")"
readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# MUD Configuration (can be overridden by environment variables)
readonly MUD_PORT="${MUD_PORT:-4100}"
readonly MUD_BINARY="${MUD_BINARY:-circle}"
readonly BIN_DIR="${BIN_DIR:-bin}"
readonly LIB_DIR="${LIB_DIR:-lib}"
readonly LOG_DIR="${LOG_DIR:-log}"
readonly DUMPS_DIR="${DUMPS_DIR:-dumps}"

# MUD flags (see admin.txt for description of all flags)
readonly FLAGS="${MUD_FLAGS:--q}"

# Auxiliary services configuration
readonly ENABLE_WEBSOCKET="${ENABLE_WEBSOCKET:-false}"
readonly ENABLE_FLASH="${ENABLE_FLASH:-false}"
readonly HMUD_DIR="${HMUD_DIR:-/home/luminari/public_html/hmud}"
readonly FMUD_DIR="${FMUD_DIR:-/home/luminari/public_html/FMud}"
readonly FLASH_POLICY_PORT="${FLASH_POLICY_PORT:-843}"
readonly FLASH_POLICY_FILE="${FLASH_POLICY_FILE:-${SCRIPT_DIR}/flashpolicy.xml}"

# Logging configuration
readonly BACKLOGS="${BACKLOGS:-6}"
readonly LEN_CRASHLOG="${LEN_CRASHLOG:-30}"

# Date format patterns
readonly DATE_FORMAT_LOG="%Y-%m-%d %H:%M:%S"
readonly DATE_FORMAT_SYSLOG="%Y%m%d"
readonly DATE_FORMAT_DUMP="%Y%m%d-%H%M%S"

# Log files configuration
# Format: filename:maxlines:pattern
readonly LOGFILES='
delete:0:self-delete
delete:0:PCLEAN
dts:0:death trap
rip:0:killed
restarts:0:Running
levels:0:advanced
rentgone:0:equipment lost
usage:5000:usage
newplayers:0:new player
errors:5000:SYSERR
godcmds:0:(GC)
badpws:0:Bad PW
olc:5000:OLC
help:0:get help on
trigger:5000:trigger
security:0:SECURITY
performance:5000:PERFORMANCE
'

#############################################################################
# Utility Functions
#############################################################################

# Enhanced logging function with timestamps
log() {
    local level="${1:-INFO}"
    shift
    local message="[$(date "+${DATE_FORMAT_LOG}")] ${SCRIPT_NAME} [${level}]: $*"
    echo "$message"
    
    # Also append to syslog if it exists
    if [[ -w syslog ]]; then
        echo "$message" >> syslog
    fi
}

# Convenience logging functions
log_info() { log "INFO" "$@"; }
log_warn() { log "WARN" "$@"; }
log_error() { log "ERROR" "$@"; }

# Error handling function
die() {
    log_error "$@"
    exit "${2:-1}"
}

# Check if a directory exists and is accessible
check_directory() {
    local dir="$1"
    local service="$2"
    
    if [[ ! -d "$dir" ]]; then
        log_warn "${service} directory not found: ${dir}"
        return 1
    fi
    
    if [[ ! -r "$dir" ]] || [[ ! -x "$dir" ]]; then
        log_warn "${service} directory not accessible: ${dir}"
        return 1
    fi
    
    return 0
}

# Check if the MUD process is running
is_mud_running() {
    # More robust process detection
    local count
    count=$(ps auxwww | grep -E "${BIN_DIR}/${MUD_BINARY}.*${MUD_PORT}" | grep -v grep | wc -l)
    [[ $count -gt 0 ]]
}

# Get MUD process PID
get_mud_pid() {
    ps auxwww | grep -E "${BIN_DIR}/${MUD_BINARY}.*${MUD_PORT}" | grep -v grep | awk '{print $2}' | head -1
}

#############################################################################
# Core Dump Management
#############################################################################

# Archive core dump file with timestamp and generate backtrace
archive_core_dump() {
    local core_file="${LIB_DIR}/core"
    
    if [[ ! -f "$core_file" ]]; then
        return 0
    fi
    
    # Create dumps directory if needed
    mkdir -p "$DUMPS_DIR"
    
    local dump_name="core.${HOSTNAME}.$(date "+${DATE_FORMAT_DUMP}")"
    local dump_path="${DUMPS_DIR}/${dump_name}"
    
    log_info "Archiving core dump to ${dump_path}"
    
    if mv "$core_file" "$dump_path" 2>/dev/null; then
        log_info "Core dump archived successfully"
        
        # Generate backtrace if gdb is available
        if command -v gdb >/dev/null 2>&1; then
            local bt_file="${DUMPS_DIR}/backtrace.${dump_name}.txt"
            log_info "Generating backtrace to ${bt_file}"
            
            # Create comprehensive GDB commands
            cat > gdb.tmp <<EOF
echo === BACKTRACE ===\n
bt full
echo \n=== REGISTERS ===\n
info registers
echo \n=== THREADS ===\n
info threads
echo \n=== MEMORY MAPPINGS ===\n
info proc mappings
echo \n=== SHARED LIBRARIES ===\n
info sharedlibrary
quit
EOF
            
            gdb "${BIN_DIR}/${MUD_BINARY}" "$dump_path" -batch -command gdb.tmp > "$bt_file" 2>&1
            rm -f gdb.tmp
            
            # Add system information to backtrace
            {
                echo "=== SYSTEM INFORMATION ==="
                echo "Date: $(date)"
                echo "Hostname: $(hostname)"
                echo "Uptime: $(uptime)"
                echo "Memory: $(free -h 2>/dev/null || true)"
                echo ""
                cat "$bt_file"
            } > "${bt_file}.tmp" && mv "${bt_file}.tmp" "$bt_file"
            
            log_info "Backtrace generated successfully"
        else
            log_warn "gdb not found - cannot generate backtrace"
        fi
    else
        log_error "Failed to archive core dump"
    fi
}

#############################################################################
# Log Processing Functions
#############################################################################

# Process and rotate syslog files
proc_syslog() {
    # Return if there's no syslog
    if [[ ! -s syslog ]]; then
        return
    fi
    
    log_info "Processing syslog files"
    
    # Create log directory if it doesn't exist
    mkdir -p "$LOG_DIR"
    
    # Create the crashlog if configured
    if [[ -n "$LEN_CRASHLOG" ]] && [[ "$LEN_CRASHLOG" -gt 0 ]]; then
        tail -n "$LEN_CRASHLOG" syslog > syslog.CRASH
        log_info "Created crash log with last $LEN_CRASHLOG lines"
    fi
    
    # Process specialty log files
    local OLD_IFS=$IFS
    IFS=$'\n'
    for rec in $LOGFILES; do
        # Skip empty lines
        [[ -z "$rec" ]] && continue
        
        local name="${LOG_DIR}/$(echo "$rec" | cut -f 1 -d:)"
        local len=$(echo "$rec" | cut -f 2 -d:)
        local pattern=$(echo "$rec" | cut -f 3- -d:)
        
        # Create parent directory if needed
        mkdir -p "$(dirname "$name")"
        
        # Extract matching lines
        grep -F "$pattern" syslog >> "$name" 2>/dev/null || true
        
        # Truncate to maximum length if specified
        if [[ "$len" -gt 0 ]] && [[ -f "$name" ]]; then
            local temp=$(mktemp "${LOG_DIR}/.tmp.XXXXXX")
            tail -n "$len" "$name" > "$temp"
            mv -f "$temp" "$name"
        fi
    done
    IFS=$OLD_IFS
    
    # Rotate main syslog files
    rotate_syslogs
}

# Rotate syslog files with proper numbering
rotate_syslogs() {
    local newlog=1
    
    # Find the next available log number
    if [[ -f "${LOG_DIR}/syslog.${BACKLOGS}" ]]; then
        newlog=$((BACKLOGS + 1))
    else
        while [[ -f "${LOG_DIR}/syslog.${newlog}" ]]; do
            newlog=$((newlog + 1))
        done
    fi
    
    # Rotate existing logs
    local y=2
    while [[ $y -lt $newlog ]]; do
        local x=$((y - 1))
        if [[ -f "${LOG_DIR}/syslog.${y}" ]]; then
            mv -f "${LOG_DIR}/syslog.${y}" "${LOG_DIR}/syslog.${x}"
        fi
        y=$((y + 1))
    done
    
    # Archive current syslog with date stamp
    local dated_syslog="${LOG_DIR}/syslog.$(date "+${DATE_FORMAT_SYSLOG}")"
    cp syslog "$dated_syslog" 2>/dev/null || true
    
    # Move current syslog to numbered position
    mv -f syslog "${LOG_DIR}/syslog.${newlog}"
    
    log_info "Syslog rotated to ${LOG_DIR}/syslog.${newlog}"
}

#############################################################################
# Auxiliary Services Management
#############################################################################

# Start websocket policy daemon
start_websocket_policy() {
    if [[ "$ENABLE_WEBSOCKET" != "true" ]]; then
        return 0
    fi
    
    if ! check_directory "$HMUD_DIR" "HTML5 WebSocket"; then
        return 1
    fi
    
    local current_dir="$PWD"
    cd "$HMUD_DIR" || return 1
    
    if [[ -x "./policyd" ]]; then
        log_info "Starting HTML5 WebSocket policy daemon"
        nohup ./policyd > /dev/null 2>&1 &
        local pid=$!
        log_info "WebSocket policy daemon started with PID $pid"
        echo "$pid" > "${SCRIPT_DIR}/.websocket_policy.pid"
    else
        log_warn "WebSocket policyd not found or not executable"
    fi
    
    cd "$current_dir"
}

# Start flash policy daemon
start_flash_policy() {
    if [[ "$ENABLE_FLASH" != "true" ]]; then
        return 0
    fi
    
    if ! check_directory "$FMUD_DIR" "Flash Policy"; then
        return 1
    fi
    
    local current_dir="$PWD"
    cd "$FMUD_DIR" || return 1
    
    if [[ -x "./flashpolicyd.py" ]]; then
        if [[ ! -f "$FLASH_POLICY_FILE" ]]; then
            log_warn "Flash policy file not found: ${FLASH_POLICY_FILE}"
        fi
        
        log_info "Starting Flash policy daemon on port $FLASH_POLICY_PORT"
        nohup ./flashpolicyd.py --file="${FLASH_POLICY_FILE}" --port="${FLASH_POLICY_PORT}" > /dev/null 2>&1 &
        local pid=$!
        log_info "Flash policy daemon started with PID $pid"
        echo "$pid" > "${SCRIPT_DIR}/.flash_policy.pid"
    else
        log_warn "Flash policyd not found or not executable"
    fi
    
    cd "$current_dir"
}

# Stop auxiliary services
stop_auxiliary_services() {
    # Stop websocket policy daemon
    if [[ -f "${SCRIPT_DIR}/.websocket_policy.pid" ]]; then
        local pid=$(cat "${SCRIPT_DIR}/.websocket_policy.pid")
        if kill -0 "$pid" 2>/dev/null; then
            log_info "Stopping WebSocket policy daemon (PID: $pid)"
            kill "$pid" 2>/dev/null || true
        fi
        rm -f "${SCRIPT_DIR}/.websocket_policy.pid"
    fi
    
    # Stop flash policy daemon
    if [[ -f "${SCRIPT_DIR}/.flash_policy.pid" ]]; then
        local pid=$(cat "${SCRIPT_DIR}/.flash_policy.pid")
        if kill -0 "$pid" 2>/dev/null; then
            log_info "Stopping Flash policy daemon (PID: $pid)"
            kill "$pid" 2>/dev/null || true
        fi
        rm -f "${SCRIPT_DIR}/.flash_policy.pid"
    fi
}

#############################################################################
# Process Management
#############################################################################

# Clean up any lingering processes
cleanup_processes() {
    log_info "Cleaning up lingering processes"
    
    # Don't kill all sleep processes - only those related to autorun
    local autorun_pids=$(pgrep -f "${SCRIPT_NAME}.*sleep" 2>/dev/null || true)
    if [[ -n "$autorun_pids" ]]; then
        log_info "Killing autorun-related sleep processes"
        echo "$autorun_pids" | xargs kill 2>/dev/null || true
    fi
}

# Verify MUD binary exists and is executable
verify_mud_binary() {
    local binary_path="${BIN_DIR}/${MUD_BINARY}"
    
    if [[ ! -f "$binary_path" ]]; then
        die "MUD binary not found: $binary_path" 2
    fi
    
    if [[ ! -x "$binary_path" ]]; then
        die "MUD binary not executable: $binary_path" 2
    fi
    
    return 0
}

#############################################################################
# Startup Check
#############################################################################

# Check if we're recovering from an improper shutdown
check_improper_shutdown() {
    if [[ -s syslog ]]; then
        log_warn "Improper shutdown detected - found existing syslog"
        echo "Improper shutdown of autorun detected, rotating syslogs before startup." >> syslog
        proc_syslog
    fi
}

#############################################################################
# Main Control Functions
#############################################################################

# Start the MUD server
start_mud() {
    log_info "Starting MUD server on port $MUD_PORT"
    log_info "Command: ${BIN_DIR}/${MUD_BINARY} ${FLAGS} ${MUD_PORT}"
    
    # Start the MUD and capture its exit code
    "${BIN_DIR}/${MUD_BINARY}" ${FLAGS} ${MUD_PORT} >> syslog 2>&1
    local exit_code=$?
    
    log_info "MUD server exited with code $exit_code"
    
    return $exit_code
}

# Handle shutdown based on control files
handle_shutdown() {
    # Check for killscript
    if [[ -r .killscript ]]; then
        log_info "Killscript detected - shutting down autorun"
        rm -f .killscript
        stop_auxiliary_services
        proc_syslog
        log_info "Autorun terminated gracefully"
        exit 0
    fi
    
    # Check for fastboot
    local wait_time=60
    if [[ -r .fastboot ]]; then
        log_info "Fastboot mode - restarting in 5 seconds"
        rm -f .fastboot
        wait_time=5
    else
        log_info "Normal restart - waiting $wait_time seconds"
    fi
    
    sleep $wait_time
    
    # Handle pause mode
    while [[ -r pause ]]; do
        log_info "Pause mode active - waiting..."
        sleep 60
    done
}

# Display status information
show_status() {
    echo "========================================"
    echo "LuminariMUD Autorun Status"
    echo "========================================"
    echo "Script: $SCRIPT_NAME"
    echo "MUD Port: $MUD_PORT"
    echo "MUD Binary: ${BIN_DIR}/${MUD_BINARY}"
    
    if is_mud_running; then
        local pid=$(get_mud_pid)
        echo "MUD Status: RUNNING (PID: $pid)"
    else
        echo "MUD Status: NOT RUNNING"
    fi
    
    echo "WebSocket Policy: $ENABLE_WEBSOCKET"
    echo "Flash Policy: $ENABLE_FLASH"
    echo "========================================"
}

#############################################################################
# Signal Handlers
#############################################################################

# Handle SIGTERM gracefully
handle_sigterm() {
    log_info "Received SIGTERM - initiating graceful shutdown"
    touch .killscript
    exit 0
}

# Handle SIGINT (Ctrl+C)
handle_sigint() {
    log_info "Received SIGINT - initiating graceful shutdown"
    touch .killscript
    exit 0
}

# Set up signal handlers
trap handle_sigterm SIGTERM
trap handle_sigint SIGINT

#############################################################################
# Main Script
#############################################################################

# Parse command line arguments
case "${1:-}" in
    status)
        show_status
        exit 0
        ;;
    stop)
        log_info "Stopping autorun"
        touch .killscript
        
        # Find and kill any running autorun processes
        autorun_pids=$(pgrep -f "bash.*${SCRIPT_NAME}" | grep -v "$$" | grep -v grep)
        if [[ -n "$autorun_pids" ]]; then
            log_info "Found running autorun process(es): $autorun_pids"
            echo "$autorun_pids" | xargs kill 2>/dev/null || true
            log_info "Sent termination signal to autorun process(es)"
        else
            log_info "No running autorun process found"
        fi
        
        # Also try to stop the MUD server gracefully
        if is_mud_running; then
            mud_pid=$(get_mud_pid)
            log_info "Stopping MUD server (PID: $mud_pid)"
            kill -TERM "$mud_pid" 2>/dev/null || true
        fi
        
        exit 0
        ;;
    help|--help|-h)
        echo "Usage: $SCRIPT_NAME [status|stop|help]"
        echo ""
        echo "Control files:"
        echo "  .fastboot   - Quick restart (5 seconds)"
        echo "  .killscript - Stop autorun"
        echo "  pause       - Pause autorun"
        echo ""
        echo "Environment variables:"
        echo "  MUD_PORT    - Port number (default: 4100)"
        echo "  MUD_FLAGS   - Server flags (default: -q)"
        echo "  ENABLE_WEBSOCKET - Enable websocket policy (default: false)"
        echo "  ENABLE_FLASH     - Enable flash policy (default: false)"
        exit 0
        ;;
esac

# Initial setup
log_info "========================================" 
log_info "LuminariMUD Enhanced Autorun Starting"
log_info "========================================"

# Set core dump size to unlimited
ulimit -c unlimited
log_info "Core dump size set to unlimited"

# Verify the MUD binary exists
verify_mud_binary

# Create required directories
mkdir -p "$LOG_DIR" "$DUMPS_DIR"

# Check for improper shutdown
check_improper_shutdown

# Start auxiliary services
start_websocket_policy
start_flash_policy

# Main loop
while true; do
    # Check if MUD is already running
    if is_mud_running; then
        log_warn "MUD already running on port $MUD_PORT - waiting..."
        sleep 60
        continue
    fi
    
    # Start syslog for this run
    log_info "Starting new MUD session at $(date)"
    echo "autorun starting game $(date)" > syslog
    echo "running ${BIN_DIR}/${MUD_BINARY} ${FLAGS} ${MUD_PORT}" >> syslog
    
    # Start the MUD
    start_mud
    
    # Archive any core dump
    archive_core_dump
    
    # Process logs
    proc_syslog
    
    # Handle shutdown/restart
    handle_shutdown
    
    # Brief pause before next iteration
    sleep 2
done