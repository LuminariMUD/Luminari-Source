#!/bin/bash
#
# LuminariMUD Autorun Watchdog
# This script monitors the main autorun.sh and restarts it if it fails
# This provides an additional layer of resilience
#
# Usage: ./autorun-watchdog.sh [start|stop|status]
#

set -u  # Exit on undefined variables

# Configuration
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
AUTORUN_SCRIPT="${SCRIPT_DIR}/autorun.sh"
CHECK_INTERVAL=60  # Check every 60 seconds
LOG_FILE="${SCRIPT_DIR}/log/watchdog.log"
STATE_FILE="${SCRIPT_DIR}/.autorun.state"
WATCHDOG_PID_FILE="${SCRIPT_DIR}/.watchdog.pid"
MAX_RESTART_ATTEMPTS=10
RESTART_COOLDOWN=300  # 5 minutes between restart attempts

# Logging function
log_msg() {
    local level="$1"
    shift
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] watchdog [$level]: $*" | tee -a "$LOG_FILE"
}

# Check if autorun is healthy
check_autorun_health() {
    # Check if state file exists and is recent
    if [[ ! -f "$STATE_FILE" ]]; then
        log_msg "WARN" "State file not found"
        return 1
    fi
    
    # Check if state file is stale (older than 5 minutes)
    local last_update=$(grep "LAST_UPDATE=" "$STATE_FILE" 2>/dev/null | cut -d= -f2)
    if [[ -z "$last_update" ]]; then
        log_msg "WARN" "Cannot read last update time"
        return 1
    fi
    
    local current_time=$(date +%s)
    local time_diff=$((current_time - last_update))
    
    if [[ $time_diff -gt 300 ]]; then
        log_msg "WARN" "State file is stale (${time_diff}s old)"
        return 1
    fi
    
    # Check if PID in state file is actually running
    local pid=$(grep "PID=" "$STATE_FILE" 2>/dev/null | cut -d= -f2)
    if [[ -z "$pid" ]] || ! kill -0 "$pid" 2>/dev/null; then
        log_msg "WARN" "Autorun PID $pid is not running"
        return 1
    fi
    
    # Check if MUD port is actually listening
    local port=$(grep "MUD_PORT=" "$STATE_FILE" 2>/dev/null | cut -d= -f2)
    if [[ -n "$port" ]]; then
        if command -v ss >/dev/null 2>&1; then
            if ! ss -tlnp 2>/dev/null | grep -q ":${port} "; then
                log_msg "INFO" "MUD not listening on port $port (may be restarting)"
                # This is not a failure - MUD might be restarting
            fi
        fi
    fi
    
    return 0
}

# Start autorun if not running
start_autorun() {
    log_msg "INFO" "Starting autorun..."
    
    if [[ ! -x "$AUTORUN_SCRIPT" ]]; then
        log_msg "ERROR" "Autorun script not found or not executable: $AUTORUN_SCRIPT"
        return 1
    fi
    
    # Start autorun in background
    "$AUTORUN_SCRIPT" &
    local pid=$!
    
    log_msg "INFO" "Autorun started with PID $pid"
    
    # Wait a moment for it to initialize
    sleep 5
    
    # Verify it's still running
    if kill -0 "$pid" 2>/dev/null; then
        log_msg "INFO" "Autorun successfully started"
        return 0
    else
        log_msg "ERROR" "Autorun failed to start"
        return 1
    fi
}

# Main watchdog loop
watchdog_loop() {
    local restart_attempts=0
    local last_restart_time=0
    
    log_msg "INFO" "Watchdog starting (PID: $$)"
    echo $$ > "$WATCHDOG_PID_FILE"
    
    while true; do
        # Check if we should stop (either .killwatchdog or .killscript)
        if [[ -f "${SCRIPT_DIR}/.killwatchdog" ]] || [[ -f "${SCRIPT_DIR}/.killscript" ]]; then
            if [[ -f "${SCRIPT_DIR}/.killscript" ]]; then
                log_msg "INFO" ".killscript detected - stopping watchdog and autorun"
            else
                log_msg "INFO" "Kill signal detected - stopping watchdog"
            fi
            rm -f "${SCRIPT_DIR}/.killwatchdog"
            rm -f "$WATCHDOG_PID_FILE"
            exit 0
        fi
        
        # Check autorun health
        if ! check_autorun_health; then
            log_msg "ERROR" "Autorun health check failed"
            
            # Check cooldown period
            local current_time=$(date +%s)
            local time_since_restart=$((current_time - last_restart_time))
            
            if [[ $time_since_restart -lt $RESTART_COOLDOWN ]]; then
                log_msg "INFO" "Waiting for cooldown period (${time_since_restart}/${RESTART_COOLDOWN}s)"
            else
                # Check restart attempts
                if [[ $restart_attempts -ge $MAX_RESTART_ATTEMPTS ]]; then
                    log_msg "ERROR" "Maximum restart attempts reached ($MAX_RESTART_ATTEMPTS)"
                    log_msg "ERROR" "Manual intervention required"
                    # Send alert if possible
                    echo "CRITICAL: Autorun watchdog failed after $MAX_RESTART_ATTEMPTS attempts" | \
                        mail -s "LuminariMUD Watchdog Failure" admin@example.com 2>/dev/null || true
                else
                    log_msg "INFO" "Attempting to restart autorun (attempt $((restart_attempts + 1)))"
                    
                    if start_autorun; then
                        log_msg "INFO" "Autorun restarted successfully"
                        restart_attempts=0
                    else
                        log_msg "ERROR" "Failed to restart autorun"
                        restart_attempts=$((restart_attempts + 1))
                    fi
                    
                    last_restart_time=$current_time
                fi
            fi
        else
            # Reset restart attempts on successful health check
            if [[ $restart_attempts -gt 0 ]]; then
                log_msg "INFO" "Autorun recovered - resetting restart counter"
                restart_attempts=0
            fi
        fi
        
        # Wait before next check
        sleep $CHECK_INTERVAL
    done
}

# Stop watchdog
stop_watchdog() {
    log_msg "INFO" "Stopping watchdog..."
    
    if [[ -f "$WATCHDOG_PID_FILE" ]]; then
        local pid=$(cat "$WATCHDOG_PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid"
            log_msg "INFO" "Watchdog stopped (PID: $pid)"
        fi
        rm -f "$WATCHDOG_PID_FILE"
    fi
    
    touch "${SCRIPT_DIR}/.killwatchdog"
}

# Show status
show_status() {
    echo "==================================="
    echo "LuminariMUD Watchdog Status"
    echo "==================================="
    
    if [[ -f "$WATCHDOG_PID_FILE" ]]; then
        local pid=$(cat "$WATCHDOG_PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            echo "Watchdog: RUNNING (PID: $pid)"
        else
            echo "Watchdog: NOT RUNNING (stale PID file)"
        fi
    else
        echo "Watchdog: NOT RUNNING"
    fi
    
    if check_autorun_health; then
        echo "Autorun: HEALTHY"
    else
        echo "Autorun: UNHEALTHY or NOT RUNNING"
    fi
    
    if [[ -f "$STATE_FILE" ]]; then
        echo ""
        echo "Autorun State:"
        cat "$STATE_FILE" | sed 's/^/  /'
    fi
    
    echo "==================================="
}

# Main script
case "${1:-start}" in
    start)
        if [[ -f "$WATCHDOG_PID_FILE" ]]; then
            pid=$(cat "$WATCHDOG_PID_FILE")
            if kill -0 "$pid" 2>/dev/null; then
                echo "Watchdog already running (PID: $pid)"
                exit 1
            fi
        fi
        
        # Start in background
        nohup "$0" loop > /dev/null 2>&1 &
        echo "Watchdog started"
        ;;
    
    loop)
        # Internal command for the actual loop
        watchdog_loop
        ;;
    
    stop)
        stop_watchdog
        ;;
    
    status)
        show_status
        ;;
    
    restart)
        stop_watchdog
        sleep 2
        "$0" start
        ;;
    
    *)
        echo "Usage: $0 {start|stop|status|restart}"
        echo ""
        echo "The watchdog monitors autorun.sh and restarts it if it fails"
        echo "This provides an additional layer of resilience for the MUD"
        exit 1
        ;;
esac