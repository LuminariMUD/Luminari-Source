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

# CRITICAL: Do NOT use set -e or set -o errexit anywhere in this script!
# The script MUST continue running even if commands fail
# Only use set -u to catch undefined variables
set -u

#############################################################################
# Configuration Section
#############################################################################

# Script identification
readonly SCRIPT_NAME="$(basename "$0")"
readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# CRITICAL: Change to script directory to ensure all relative paths work correctly
# BUT NEVER EXIT! The autorun must continue even if directory change fails
if ! cd "$SCRIPT_DIR"; then
    echo "ERROR: Failed to change to script directory: $SCRIPT_DIR" >&2
    echo "WARNING: Attempting to continue from current directory: $(pwd)" >&2
    # Try to use absolute paths instead
    SCRIPT_DIR="$(pwd)"
fi

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
readonly MAX_LOG_SIZE_MB="${MAX_LOG_SIZE_MB:-100}"  # Max size before rotation
readonly LOG_RETENTION_DAYS="${LOG_RETENTION_DAYS:-30}"  # Keep logs for 30 days

# Runtime control options (can be overridden by environment variables)
readonly IGNORE_DISK_SPACE="${IGNORE_DISK_SPACE:-true}"  # Default: keep running even with low disk space

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

# Error handling function - NEVER actually die!
die() {
    log_error "ERROR (but not dying): $@"
    # Don't exit! Log the error and return
    return "${2:-1}"
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
    # First check if something is actually listening on the MUD port
    # This avoids false positives from zombie processes or similar names
    if command -v ss >/dev/null 2>&1; then
        # Use ss to check for listening socket (most reliable)
        ss -tlnp 2>/dev/null | grep -q ":${MUD_PORT} "
        return $?
    elif command -v netstat >/dev/null 2>&1; then
        # Fallback to netstat if ss not available
        netstat -tlnp 2>/dev/null | grep -q ":${MUD_PORT} "
        return $?
    elif command -v lsof >/dev/null 2>&1; then
        # Another fallback using lsof
        lsof -i :${MUD_PORT} >/dev/null 2>&1
        return $?
    else
        # Last resort: check for process (original method)
        # More robust process detection using pgrep if available
        if command -v pgrep >/dev/null 2>&1; then
            pgrep -f "${BIN_DIR}/${MUD_BINARY}.*${MUD_PORT}" >/dev/null 2>&1
        else
            # Fallback to ps if pgrep not available
            local count
            count=$(ps auxwww | grep -E "${BIN_DIR}/${MUD_BINARY}.*${MUD_PORT}" | grep -v grep | wc -l)
            [[ $count -gt 0 ]]
        fi
    fi
}

# Get MUD process PID
get_mud_pid() {
    if command -v pgrep >/dev/null 2>&1; then
        pgrep -f "${BIN_DIR}/${MUD_BINARY}.*${MUD_PORT}" | head -1
    else
        ps auxwww | grep -E "${BIN_DIR}/${MUD_BINARY}.*${MUD_PORT}" | grep -v grep | awk '{print $2}' | head -1
    fi
}

#############################################################################
# Core Dump Management
#############################################################################

# Archive core dump file with timestamp and generate backtrace
archive_core_dump() {
    # Check multiple possible core file locations
    local core_file=""
    local possible_cores=("${LIB_DIR}/core" "core" "${BIN_DIR}/core")
    
    for cf in "${possible_cores[@]}"; do
        if [[ -f "$cf" ]]; then
            core_file="$cf"
            break
        fi
    done
    
    if [[ -z "$core_file" ]]; then
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
    
    # Clean up old logs
    cleanup_old_logs
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

# Clean up old log files
cleanup_old_logs() {
    if [[ -z "$LOG_RETENTION_DAYS" ]] || [[ "$LOG_RETENTION_DAYS" -le 0 ]]; then
        return
    fi
    
    log_info "Cleaning up logs older than $LOG_RETENTION_DAYS days"
    
    # Find and remove old log files
    if command -v find >/dev/null 2>&1; then
        find "$LOG_DIR" -name "syslog.*" -type f -mtime +$LOG_RETENTION_DAYS -delete 2>/dev/null || true
        find "$DUMPS_DIR" -name "core.*" -type f -mtime +$LOG_RETENTION_DAYS -delete 2>/dev/null || true
        find "$DUMPS_DIR" -name "backtrace.*" -type f -mtime +$LOG_RETENTION_DAYS -delete 2>/dev/null || true
    fi
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
    
    if ! cd "$current_dir"; then
        log_error "Failed to return to original directory: $current_dir"
        log_warn "Continuing from current directory: $(pwd)"
        # Don't exit - autorun must continue!
    fi
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
    
    if ! cd "$current_dir"; then
        log_error "Failed to return to original directory: $current_dir"
        log_warn "Continuing from current directory: $(pwd)"
        # Don't exit - autorun must continue!
    fi
}

# Stop auxiliary services
stop_auxiliary_services() {
    # Stop websocket policy daemon
    if [[ -f "${SCRIPT_DIR}/.websocket_policy.pid" ]]; then
        local pid
        pid=$(cat "${SCRIPT_DIR}/.websocket_policy.pid" 2>/dev/null || echo "")
        if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
            log_info "Stopping WebSocket policy daemon (PID: $pid)"
            kill "$pid" 2>/dev/null || true
        fi
        rm -f "${SCRIPT_DIR}/.websocket_policy.pid"
    fi
    
    # Stop flash policy daemon
    if [[ -f "${SCRIPT_DIR}/.flash_policy.pid" ]]; then
        local pid
        pid=$(cat "${SCRIPT_DIR}/.flash_policy.pid" 2>/dev/null || echo "")
        if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
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
    
    # Clean up any zombie MUD processes
    cleanup_zombie_processes
}

# Clean up zombie or defunct MUD processes
cleanup_zombie_processes() {
    # Find zombie/defunct MUD processes
    local zombies=$(ps aux | grep "${MUD_BINARY}" | grep "<defunct>" | awk '{print $2}')
    if [[ -n "$zombies" ]]; then
        log_warn "Found zombie MUD processes, cleaning up: $zombies"
        echo "$zombies" | xargs kill -9 2>/dev/null || true
    fi
    
    # Also check for stuck MUD processes that aren't listening
    if ! is_mud_running; then
        # No listening socket, but check for stuck processes
        local stuck_pids=$(pgrep -f "${BIN_DIR}/${MUD_BINARY}" 2>/dev/null || true)
        if [[ -n "$stuck_pids" ]]; then
            log_warn "Found stuck MUD processes not listening on port, cleaning up: $stuck_pids"
            echo "$stuck_pids" | xargs kill -TERM 2>/dev/null || true
            sleep 2
            # Force kill if still running
            for pid in $stuck_pids; do
                if kill -0 "$pid" 2>/dev/null; then
                    log_warn "Force killing stuck process: $pid"
                    kill -9 "$pid" 2>/dev/null || true
                fi
            done
        fi
    fi
}

# Verify MUD binary exists and is executable
verify_mud_binary() {
    local binary_path="${BIN_DIR}/${MUD_BINARY}"
    
    # Security check: ensure paths don't contain shell metacharacters
    if [[ "$binary_path" =~ [';|&<>$`'] ]]; then
        log_error "Invalid characters in binary path: $binary_path"
        return 2
    fi
    
    if [[ ! -f "$binary_path" ]]; then
        log_error "MUD binary not found: $binary_path"
        return 2
    fi
    
    if [[ ! -x "$binary_path" ]]; then
        log_error "MUD binary not executable: $binary_path"
        return 2
    fi
    
    # Verify it's a regular file (not symlink to dangerous location)
    if [[ ! -f "$binary_path" ]] || [[ -L "$binary_path" ]]; then
        local real_path=$(readlink -f "$binary_path" 2>/dev/null || echo "")
        if [[ -z "$real_path" ]] || [[ ! -f "$real_path" ]]; then
            log_error "MUD binary is not a regular file or broken symlink: $binary_path"
            return 2
        fi
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
    # CRITICAL: We must handle ALL possible failures gracefully
    set +e
    
    # Check if binary still exists before starting
    if [[ ! -x "${BIN_DIR}/${MUD_BINARY}" ]]; then
        log_error "MUD binary not found or not executable: ${BIN_DIR}/${MUD_BINARY}"
        log_info "Binary missing - waiting 60 seconds before retry"
        return 1
    fi
    
    # Start the MUD - this may crash, segfault, or exit in any way
    "${BIN_DIR}/${MUD_BINARY}" ${FLAGS} ${MUD_PORT} >> syslog 2>&1
    local exit_code=$?
    
    # NO MATTER WHAT HAPPENS, WE CONTINUE!
    # The script should continue running even if the MUD explodes spectacularly
    
    log_info "MUD server exited with code $exit_code"
    
    # Always return the exit code but NEVER let it stop the script
    return $exit_code || true
}

# Handle shutdown based on control files
handle_shutdown() {
    # Check for killscript
    if [[ -r .killscript ]]; then
        log_info "Killscript detected - shutting down autorun"
        rm -f .killscript
        cleanup  # Call cleanup explicitly
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
        pid=$(get_mud_pid)
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

# Signal handling - CRITICAL FOR RESILIENCE
# We must be very careful about which signals we handle and how

# Handle SIGTERM gracefully
handle_sigterm() {
    log_info "Received SIGTERM - ignoring to keep autorun alive"
    log_info "Use 'autorun.sh stop' or create .killscript to stop autorun"
    # DO NOT EXIT! The MUD crashed but autorun must continue
    return 0
}

# Handle SIGINT (Ctrl+C)
handle_sigint() {
    log_info "Received SIGINT - user interrupt"
    # Only respond to SIGINT in interactive mode
    if [[ -t 0 ]]; then
        log_info "Interactive mode - creating killscript"
        touch .killscript
        # Give the main loop a chance to exit cleanly
        sleep 1
        exit 0
    else
        log_info "Non-interactive mode - ignoring SIGINT"
    fi
    return 0
}

# Handle SIGHUP (terminal hangup)
handle_sighup() {
    log_info "Received SIGHUP - terminal disconnected, continuing in background"
    # Don't exit when SSH connection drops!
    return 0
}

# Set up signal handlers
# CRITICAL: These handlers keep autorun alive during various failure scenarios
trap handle_sigint SIGINT    # User interrupt (Ctrl+C)
trap handle_sighup HUP       # Terminal hangup (SSH disconnect)
trap handle_sigterm TERM     # Termination signal
trap '' PIPE                 # Ignore broken pipes
trap '' QUIT                 # Ignore quit signal

# Log signal configuration
log_info "Signal handlers configured - autorun is resilient to crashes"

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
        
        # Stop the watchdog first if it's running
        if [[ -f "${SCRIPT_DIR}/autorun-watchdog.sh" ]]; then
            log_info "Stopping watchdog..."
            "${SCRIPT_DIR}/autorun-watchdog.sh" stop 2>/dev/null || true
        fi
        
        # Find and kill any running autorun processes
        autorun_pids=$(pgrep -f "bash.*${SCRIPT_NAME}" 2>/dev/null | grep -v "$$" | grep -v grep || true)
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
            if [[ -n "$mud_pid" ]]; then
                log_info "Stopping MUD server (PID: $mud_pid)"
                kill -TERM "$mud_pid" 2>/dev/null || true
            fi
        fi
        
        exit 0
        ;;
    foreground|fg)
        # Run in foreground mode - continue to main loop
        log_info "Running in foreground mode"
        ;;
    help|--help|-h)
        echo "Usage: $SCRIPT_NAME [foreground|status|stop|help]"
        echo ""
        echo "By default, autorun starts in daemon mode (detached from terminal)"
        echo ""
        echo "Commands:"
        echo "  (no args)   - Start in daemon mode (default)"
        echo "  foreground  - Run in foreground (attached to terminal)"
        echo "  status      - Show current status"
        echo "  stop        - Stop the autorun and MUD server"
        echo "  help        - Show this help"
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
    ""|*)
        # DEFAULT BEHAVIOR: Daemonize when no arguments
        # Check if already running
        if is_mud_running; then
            log_error "MUD already running on port $MUD_PORT"
            exit 1
        fi
        
        # Proper daemonization with lock file to prevent multiple instances
        lockfile="${SCRIPT_DIR}/.autorun.lock"
        
        # Check for stale lock file - more robust handling
        if [[ -f "$lockfile" ]]; then
            old_pid=$(cat "$lockfile" 2>/dev/null || echo "")
            if [[ -z "$old_pid" ]]; then
                # Empty lock file - remove it
                log_warn "Removing empty lock file"
                rm -f "$lockfile"
            elif ! kill -0 "$old_pid" 2>/dev/null; then
                # Process doesn't exist - remove stale lock
                log_warn "Removing stale lock file (PID: $old_pid)"
                rm -f "$lockfile"
            elif ! pgrep -f "bash.*${SCRIPT_NAME}.*foreground" >/dev/null 2>&1; then
                # Lock file exists but no autorun process found
                log_warn "Lock file exists but no autorun process found - removing"
                rm -f "$lockfile"
            fi
        fi
        
        # Use flock for atomic lock acquisition
        exec 200>"$lockfile"
        if ! flock -n 200; then
            log_error "Another autorun instance is already running"
            exit 1
        fi
        
        # Fork to background with MAXIMUM resilience
        # Create a subshell that will NEVER die
        (
            # Detach from terminal completely
            exec </dev/null >/dev/null 2>&1
            
            # Ignore ALL signals that could kill us
            trap '' HUP TERM QUIT PIPE
            
            # Use nohup for extra protection
            nohup bash -c "
                # Ignore signals in the inner shell too
                trap '' HUP TERM QUIT PIPE
                # Change to new process group to avoid signal propagation
                set -m
                # Run autorun in foreground mode
                exec '$0' foreground
            " &
            
            # Store the background process PID
            DAEMON_PID=$!
            echo $DAEMON_PID > "${lockfile}.pid"
        ) &
        
        # Give it a moment to start
        sleep 1
        
        echo "LuminariMUD daemon started"
        echo "Use '$SCRIPT_NAME status' to check status"
        echo "Use '$SCRIPT_NAME stop' to stop"
        exit 0
        ;;
esac

# Initial setup
log_info "========================================" 
log_info "LuminariMUD Enhanced Autorun Starting"
log_info "Script: $SCRIPT_NAME"
log_info "PID: $$"
log_info "Date: $(date)"
log_info "Working Directory: $(pwd)"
log_info "========================================"

# Clean up any leftover control files from previous runs
if [[ -f .killscript ]]; then
    log_info "Removing leftover .killscript from previous run"
    rm -f .killscript
fi

# Clean up stale PID files from previous runs
cleanup_stale_pidfiles() {
    local pidfile old_pid
    for pidfile in .websocket_policy.pid .flash_policy.pid; do
        if [[ -f "$pidfile" ]]; then
            old_pid=$(cat "$pidfile" 2>/dev/null || echo "")
            if [[ -n "$old_pid" ]] && ! kill -0 "$old_pid" 2>/dev/null; then
                log_info "Removing stale PID file: $pidfile (PID: $old_pid)"
                rm -f "$pidfile"
            fi
        fi
    done
}
cleanup_stale_pidfiles

# Set core dump size to unlimited
ulimit -c unlimited
log_info "Core dump size set to unlimited"

# Verify the MUD binary exists (non-fatal if it fails)
if ! verify_mud_binary; then
    log_error "MUD binary verification failed - will retry in main loop"
    # Don't exit - maybe it will be fixed by the time we loop
fi

# Create required directories
mkdir -p "$LOG_DIR" "$DUMPS_DIR"

# Check disk space
check_disk_space() {
    # Skip disk space check if configured to ignore
    if [[ "$IGNORE_DISK_SPACE" == "true" ]]; then
        return 0
    fi
    
    local min_space_mb=1000  # Require at least 1GB free
    local available_mb
    
    if command -v df >/dev/null 2>&1; then
        available_mb=$(df -m . | awk 'NR==2 {print $4}')
        if [[ $available_mb -lt $min_space_mb ]]; then
            log_error "CRITICAL: Low disk space! Only ${available_mb}MB available (minimum: ${min_space_mb}MB)"
            log_info "Set IGNORE_DISK_SPACE=true to continue anyway"
            return 1
        fi
    fi
    return 0
}

# Initial disk space check (non-fatal)
if ! check_disk_space; then
    log_error "WARNING: Insufficient disk space detected"
    log_warn "Continuing anyway - MUD may experience issues"
    # Don't exit - let the MUD try to run
fi

# Check for improper shutdown
check_improper_shutdown

# Start auxiliary services
start_websocket_policy
start_flash_policy

# Start the watchdog if it exists and isn't already running
start_watchdog() {
    local watchdog_script="${SCRIPT_DIR}/autorun-watchdog.sh"
    local watchdog_pid_file="${SCRIPT_DIR}/.watchdog.pid"
    
    # Check if watchdog script exists
    if [[ ! -f "$watchdog_script" ]]; then
        log_info "Watchdog script not found - running without extra protection"
        return 0
    fi
    
    # Check if watchdog is already running
    if [[ -f "$watchdog_pid_file" ]]; then
        local wpid=$(cat "$watchdog_pid_file" 2>/dev/null || echo "")
        if [[ -n "$wpid" ]] && kill -0 "$wpid" 2>/dev/null; then
            log_info "Watchdog already running (PID: $wpid)"
            return 0
        fi
    fi
    
    # Start the watchdog
    log_info "Starting autorun watchdog for extra protection"
    if [[ -x "$watchdog_script" ]]; then
        "$watchdog_script" start >/dev/null 2>&1
        log_info "Watchdog started successfully"
    else
        chmod +x "$watchdog_script" 2>/dev/null || true
        "$watchdog_script" start >/dev/null 2>&1
        log_info "Watchdog started successfully"
    fi
}

# Only start watchdog in foreground mode (not during initial daemon fork)
if [[ "${1:-}" == "foreground" ]] || [[ "${1:-}" == "fg" ]]; then
    start_watchdog
fi

# Log autorun configuration for debugging
log_info "Autorun Configuration:"
log_info "  MUD_PORT=$MUD_PORT"
log_info "  MUD_BINARY=$MUD_BINARY"
log_info "  BIN_DIR=$BIN_DIR"
log_info "  FLAGS=$FLAGS"
log_info "  IGNORE_DISK_SPACE=$IGNORE_DISK_SPACE"
log_info "  Signal handlers: ACTIVE"
log_info "  EXIT trap: DISABLED (safe mode)"
log_info "  Watchdog: ENABLED (if available)"

# Production monitoring
CRASH_COUNT=0
CRASH_WINDOW_START=$(date +%s)
MAX_CRASHES_PER_HOUR=10
MAX_UPTIME_HOURS="${MAX_UPTIME_HOURS:-168}"  # Default: restart after 7 days

# Autorun health tracking
AUTORUN_START_TIME=$(date +%s)
AUTORUN_PID=$$
log_info "Autorun started with PID $AUTORUN_PID at $(date)"

# Write autorun state file for external monitoring
write_autorun_state() {
    local state_file="${SCRIPT_DIR}/.autorun.state"
    cat > "$state_file" <<EOF
PID=$AUTORUN_PID
START_TIME=$AUTORUN_START_TIME
LAST_UPDATE=$(date +%s)
STATUS=RUNNING
CRASH_COUNT=$CRASH_COUNT
MUD_PORT=$MUD_PORT
EOF
}
write_autorun_state

# Clean up function (only called when explicitly shutting down via .killscript)
cleanup() {
    log_info "Performing cleanup..."
    stop_auxiliary_services
    rm -f "${SCRIPT_DIR}/.autorun.lock" 2>/dev/null || true
}
# CRITICAL: Do NOT trap EXIT! This causes autorun to terminate on any error
# Only cleanup when we explicitly want to shut down
# trap cleanup EXIT  # REMOVED - This was causing autorun to terminate!

# Main loop - THIS MUST NEVER EXIT!
# Even if everything fails, keep trying
# CRITICAL: This loop is the heart of autorun - it must be indestructible
log_info "Entering main autorun loop - this will run forever until .killscript"
while true; do
    # Trap any errors in the loop itself and continue
    set +e  # Disable error exit for the entire loop
    
    # Safety check: ensure we're still in a valid state
    if [[ ! -d "$BIN_DIR" ]]; then
        log_error "CRITICAL: Binary directory missing: $BIN_DIR"
        log_info "Waiting 60 seconds before retry..."
        sleep 60
        continue
    fi
    # Clean up any zombie processes before checking if MUD is running
    cleanup_zombie_processes
    
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
    
    # Crash loop detection
    current_time=$(date +%s)
    time_since_window_start=$((current_time - CRASH_WINDOW_START))
    
    # Reset crash counter if we've been stable for an hour
    if [[ $time_since_window_start -gt 3600 ]]; then
        CRASH_COUNT=0
        CRASH_WINDOW_START=$current_time
    fi
    
    # Track MUD start time
    mud_start_time=$(date +%s)
    
    # Update state before starting MUD
    write_autorun_state
    
    # Start the MUD
    start_mud
    mud_exit_code=$?
    
    # Log detailed exit information
    echo "MUD EXIT: code=$mud_exit_code time=$(date) uptime=${mud_uptime}s" >> "${LOG_DIR}/mud_exits.log"
    
    # Calculate MUD uptime
    mud_end_time=$(date +%s)
    mud_uptime=$((mud_end_time - mud_start_time))
    mud_uptime_hours=$((mud_uptime / 3600))
    
    # Log exit status for monitoring
    log_info "MUD ran for $mud_uptime seconds ($mud_uptime_hours hours)"
    
    if [[ $mud_exit_code -eq 0 ]]; then
        log_info "MUD exited cleanly"
    elif [[ $mud_exit_code -eq 139 ]]; then
        log_error "MUD crashed with segmentation fault (SIGSEGV)"
        CRASH_COUNT=$((CRASH_COUNT + 1))
    elif [[ $mud_exit_code -eq 134 ]]; then
        log_error "MUD aborted (SIGABRT)"
        CRASH_COUNT=$((CRASH_COUNT + 1))
    else
        log_error "MUD exited with unexpected code: $mud_exit_code"
        CRASH_COUNT=$((CRASH_COUNT + 1))
    fi
    
    # Log crash count but NEVER stop restarting
    if [[ $CRASH_COUNT -ge $MAX_CRASHES_PER_HOUR ]]; then
        log_warn "MUD has crashed $CRASH_COUNT times in the last hour - continuing anyway!"
        # Optional: Send alert (uncomment and configure as needed)
        # echo "MUD crash loop detected on $(hostname)" | mail -s "MUD CRITICAL" admin@example.com
    fi
    
    # Archive any core dump
    archive_core_dump
    
    # Process logs
    proc_syslog
    
    # Periodic disk space check
    if ! check_disk_space; then
        log_error "Disk space critically low - pausing operations"
        sleep 300  # Wait 5 minutes before checking again
        continue
    fi
    
    # Handle shutdown/restart
    handle_shutdown
    
    # Brief pause before next iteration
    sleep 2 || true  # Even if sleep fails, continue!
    
    # FAILSAFE: If we somehow get here with an error, continue anyway
    true
    
    # Extra safety: Check if we should continue
    if [[ -r .killscript ]]; then
        log_info "Killscript detected in main loop - initiating shutdown"
        cleanup
        proc_syslog
        exit 0
    fi
done

# THIS SHOULD NEVER BE REACHED!
# If we somehow exit the loop, restart the entire script
log_error "CRITICAL: Main loop exited unexpectedly! Restarting entire autorun..."
# Save state for debugging
echo "Main loop exit at $(date) - PID $$" >> "${LOG_DIR}/autorun_crashes.log"
# Restart with absolute path to ensure we can find it
if [[ -x "${SCRIPT_DIR}/${SCRIPT_NAME}" ]]; then
    exec "${SCRIPT_DIR}/${SCRIPT_NAME}" "$@"
else
    # Last resort - try to find and restart ourselves
    log_error "FATAL: Cannot restart autorun - script not found!"
    # Still don't exit - sleep and hope someone fixes it
    while true; do
        log_error "Autorun broken but refusing to die - sleeping 300 seconds"
        sleep 300
        if [[ -x "${SCRIPT_DIR}/${SCRIPT_NAME}" ]]; then
            exec "${SCRIPT_DIR}/${SCRIPT_NAME}" "$@"
        fi
    done
fi