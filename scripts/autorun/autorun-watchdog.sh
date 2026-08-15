#!/bin/bash
#
# LuminariMUD Autorun Watchdog
# This script monitors the main autorun.sh and restarts it if it fails
# This provides an additional layer of resilience
#
# Usage: ./scripts/autorun/autorun-watchdog.sh [start|stop|status]
#

set -u  # Exit on undefined variables

# Configuration
SCRIPT_PATH="$(readlink -f -- "$0" 2>/dev/null || true)"
if [[ -z "$SCRIPT_PATH" ]]; then
    SCRIPT_PATH="$(cd "$(dirname "$0")" && pwd)/$(basename "$0")"
fi
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
PROJECT_ROOT="${LUMINARI_PROJECT_ROOT:-}"
if [[ -z "$PROJECT_ROOT" ]]; then
    if [[ -f "${SCRIPT_DIR}/../../configure.ac" ]]; then
        PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
    else
        PROJECT_ROOT="$SCRIPT_DIR"
    fi
fi
AUTORUN_SCRIPT="${SCRIPT_DIR}/autorun.sh"
WATCHDOG_SCRIPT="${SCRIPT_DIR}/autorun-watchdog.sh"
CHECK_INTERVAL="${WATCHDOG_CHECK_INTERVAL:-60}"
LOG_FILE="${PROJECT_ROOT}/log/watchdog.log"
STATE_FILE="${PROJECT_ROOT}/.autorun.state"
WATCHDOG_PID_FILE="${PROJECT_ROOT}/.watchdog.pid"
MAX_RESTART_ATTEMPTS=10
RESTART_COOLDOWN="${WATCHDOG_RESTART_COOLDOWN:-300}"
STARTUP_GRACE_PERIOD="${WATCHDOG_STARTUP_GRACE_PERIOD:-10}"
AUTORUN_STARTUP_TIMEOUT="${WATCHDOG_AUTORUN_STARTUP_TIMEOUT:-15}"
STATE_STALE_THRESHOLD="${WATCHDOG_STATE_STALE_THRESHOLD:-300}"

# Logging function
log_msg() {
    local level="$1"
    shift
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] watchdog [$level]: $*" | tee -a "$LOG_FILE"
}

# Read and validate a PID file before using it.
read_pid_file() {
    local pid_file="$1"
    local pid=""

    if [[ ! -r "$pid_file" ]]; then
        return 1
    fi

    IFS= read -r pid < "$pid_file" || true
    if [[ ! "$pid" =~ ^[1-9][0-9]*$ ]]; then
        return 1
    fi

    printf '%s\n' "$pid"
}

# Publish the watchdog PID atomically.
write_pid_file() {
    local pid_file="$1"
    local pid="$2"
    local pid_tmp

    pid_tmp=$(mktemp "${pid_file}.tmp.XXXXXX") || return 1
    if ! printf '%s\n' "$pid" > "$pid_tmp"; then
        rm -f -- "$pid_tmp"
        return 1
    fi

    if ! mv -f -- "$pid_tmp" "$pid_file"; then
        rm -f -- "$pid_tmp"
        return 1
    fi
}

# Confirm that a PID is this exact watchdog script running the loop command.
pid_is_watchdog() {
    local pid="$1"
    local candidate=""
    local expected_path
    local process_cwd
    local process_exe
    local -a command_line=()

    if [[ ! "$pid" =~ ^[1-9][0-9]*$ ]] ||
       [[ ! -r "/proc/${pid}/cmdline" ]]; then
        return 1
    fi

    expected_path=$(readlink -f -- "$WATCHDOG_SCRIPT" 2>/dev/null || true)
    process_exe=$(readlink -f -- "/proc/${pid}/exe" 2>/dev/null || true)
    process_cwd=$(readlink -f -- "/proc/${pid}/cwd" 2>/dev/null || true)
    mapfile -d '' -t command_line < "/proc/${pid}/cmdline" 2>/dev/null || return 1

    if [[ -z "$expected_path" ]] || [[ ${#command_line[@]} -lt 3 ]]; then
        return 1
    fi

    if [[ "${command_line[1]}" == /* ]]; then
        candidate=$(readlink -f -- "${command_line[1]}" 2>/dev/null || true)
    elif [[ "${command_line[1]}" == */* ]] && [[ -n "$process_cwd" ]]; then
        candidate=$(readlink -f -- "${process_cwd}/${command_line[1]}" 2>/dev/null || true)
    fi

    case "$(basename "$process_exe")" in
        bash|dash|sh)
            [[ "$candidate" == "$expected_path" ]] &&
                [[ "${command_line[2]}" == "loop" ]]
            ;;
        *)
            return 1
            ;;
    esac
}

# Confirm that a PID is this checkout's foreground autorun supervisor.
pid_is_autorun() {
    local pid="$1"
    local candidate=""
    local expected_path
    local process_cwd
    local process_exe
    local -a command_line=()

    if [[ ! "$pid" =~ ^[1-9][0-9]*$ ]] ||
       [[ ! -r "/proc/${pid}/cmdline" ]]; then
        return 1
    fi

    expected_path=$(readlink -f -- "$AUTORUN_SCRIPT" 2>/dev/null || true)
    process_exe=$(readlink -f -- "/proc/${pid}/exe" 2>/dev/null || true)
    process_cwd=$(readlink -f -- "/proc/${pid}/cwd" 2>/dev/null || true)
    mapfile -d '' -t command_line < "/proc/${pid}/cmdline" 2>/dev/null || return 1

    if [[ -z "$expected_path" ]] || [[ ${#command_line[@]} -lt 2 ]]; then
        return 1
    fi

    if [[ "$process_exe" == "$expected_path" ]]; then
        candidate="$process_exe"
        [[ "${command_line[1]:-}" == "foreground" ||
           "${command_line[1]:-}" == "fg" ]]
        return
    fi

    if [[ "${command_line[1]}" == /* ]]; then
        candidate=$(readlink -f -- "${command_line[1]}" 2>/dev/null || true)
    elif [[ "${command_line[1]}" == */* ]] && [[ -n "$process_cwd" ]]; then
        candidate=$(readlink -f -- "${process_cwd}/${command_line[1]}" 2>/dev/null || true)
    fi

    case "$(basename "$process_exe")" in
        bash|dash|sh)
            [[ "$candidate" == "$expected_path" ]] &&
                [[ "${command_line[2]:-}" == "foreground" ||
                   "${command_line[2]:-}" == "fg" ]]
            ;;
        *)
            return 1
            ;;
    esac
}

# Check the exact configured TCP port without depending on process-name output.
mud_port_is_listening() {
    local port="$1"

    if [[ ! "$port" =~ ^[1-9][0-9]*$ ]] ||
       ((port > 65535)) ||
       ! command -v ss >/dev/null 2>&1; then
        return 1
    fi

    ss -H -ltn "sport = :$port" 2>/dev/null | grep -q .
}

# Check if autorun is healthy
check_autorun_health() {
    local current_time
    local last_update
    local pid
    local port
    local time_diff

    # Check if state file exists and is recent
    if [[ ! -f "$STATE_FILE" ]]; then
        log_msg "WARN" "State file not found"
        return 1
    fi

    # Resolve process and listener evidence before considering state age. A
    # delayed state update must never cause a second supervisor to be launched.
    last_update=$(grep -m 1 "^LAST_UPDATE=" "$STATE_FILE" 2>/dev/null | cut -d= -f2)
    if [[ ! "$last_update" =~ ^[0-9]+$ ]]; then
        log_msg "WARN" "Cannot read last update time"
        return 1
    fi

    current_time=$(date +%s)
    time_diff=$((current_time - last_update))

    pid=$(grep -m 1 "^PID=" "$STATE_FILE" 2>/dev/null | cut -d= -f2)
    port=$(grep -m 1 "^MUD_PORT=" "$STATE_FILE" 2>/dev/null | cut -d= -f2)

    if [[ $time_diff -gt $STATE_STALE_THRESHOLD ]]; then
        if [[ "$pid" =~ ^[1-9][0-9]*$ ]] &&
           kill -0 "$pid" 2>/dev/null &&
           pid_is_autorun "$pid"; then
            log_msg "WARN" \
                "State is stale (${time_diff}s old), but verified autorun PID $pid is running"
            return 0
        fi
        if mud_port_is_listening "$port"; then
            log_msg "WARN" \
                "State is stale (${time_diff}s old), but MUD port $port is listening; suppressing restart"
            return 0
        fi
        log_msg "WARN" "State file is stale (${time_diff}s old)"
        return 1
    fi

    # Check both PID existence and exact command identity.
    if [[ ! "$pid" =~ ^[1-9][0-9]*$ ]] ||
       ! kill -0 "$pid" 2>/dev/null; then
        log_msg "WARN" "Autorun PID $pid is not running"
        return 1
    fi
    if ! pid_is_autorun "$pid"; then
        log_msg "WARN" "PID $pid is running, but is not this autorun supervisor"
        return 1
    fi

    # Check if MUD port is actually listening
    if [[ -n "$port" ]]; then
        if ! mud_port_is_listening "$port"; then
            log_msg "INFO" "MUD not listening on port $port (may be restarting)"
            # This is not a failure - MUD might be restarting
        fi
    fi

    return 0
}

# Start autorun if not running
start_autorun() {
    local elapsed
    local existing_pid=""
    local existing_port=""

    log_msg "INFO" "Starting autorun..."

    if [[ ! -x "$AUTORUN_SCRIPT" ]]; then
        log_msg "ERROR" "Autorun script not found or not executable: $AUTORUN_SCRIPT"
        return 1
    fi

    if [[ -r "$STATE_FILE" ]]; then
        existing_pid=$(grep -m 1 "^PID=" "$STATE_FILE" 2>/dev/null | cut -d= -f2)
        existing_port=$(grep -m 1 "^MUD_PORT=" "$STATE_FILE" 2>/dev/null | cut -d= -f2)
    fi
    if [[ "$existing_pid" =~ ^[1-9][0-9]*$ ]] &&
       kill -0 "$existing_pid" 2>/dev/null &&
       pid_is_autorun "$existing_pid"; then
        log_msg "WARN" "Verified autorun PID $existing_pid appeared before restart; not launching another"
        return 0
    fi
    if mud_port_is_listening "$existing_port"; then
        log_msg "WARN" "MUD port $existing_port is already listening; not launching another autorun"
        return 0
    fi

    # autorun daemonizes itself, so its launcher should exit after starting the
    # foreground supervisor.
    if ! "$AUTORUN_SCRIPT"; then
        log_msg "ERROR" "Autorun launcher failed"
        return 1
    fi

    elapsed=0
    while [[ $elapsed -lt $AUTORUN_STARTUP_TIMEOUT ]]; do
        sleep 1
        if check_autorun_health; then
            log_msg "INFO" "Autorun successfully started"
            return 0
        fi
        elapsed=$((elapsed + 1))
    done

    log_msg "ERROR" "Autorun failed to become healthy within ${AUTORUN_STARTUP_TIMEOUT}s"
    return 1
}

# Main watchdog loop
watchdog_loop() {
    local restart_attempts=0
    local last_restart_time=0

    log_msg "INFO" "Watchdog starting (PID: $$)"
    if ! write_pid_file "$WATCHDOG_PID_FILE" "$$"; then
        log_msg "ERROR" "Unable to publish watchdog PID"
        return 1
    fi

    if [[ $STARTUP_GRACE_PERIOD -gt 0 ]]; then
        log_msg "INFO" "Waiting ${STARTUP_GRACE_PERIOD}s startup grace period"
        sleep "$STARTUP_GRACE_PERIOD"
    fi

    while true; do
        # .killwatchdog is an immediate, watchdog-specific stop request.
        if [[ -f "${PROJECT_ROOT}/.killwatchdog" ]]; then
            log_msg "INFO" "Kill signal detected - stopping watchdog"
            rm -f "${PROJECT_ROOT}/.killwatchdog"
            rm -f "$WATCHDOG_PID_FILE"
            exit 0
        fi

        # The MUD creates .killscript while booting and removes it only after a
        # successful boot. Defer that marker while autorun is healthy so a slow
        # production boot does not disable the watchdog. Once autorun has
        # stopped, the marker is an intentional shutdown and must prevent a
        # watchdog restart.
        if [[ -f "${PROJECT_ROOT}/.killscript" ]]; then
            if check_autorun_health; then
                log_msg "INFO" \
                    ".killscript detected while autorun is healthy - deferring shutdown"
                sleep "$CHECK_INTERVAL"
                continue
            fi

            log_msg "INFO" \
                ".killscript detected after autorun stopped - stopping watchdog"
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
        sleep "$CHECK_INTERVAL"
    done
}

# Stop watchdog
stop_watchdog() {
    local pid

    log_msg "INFO" "Stopping watchdog..."
    touch "${PROJECT_ROOT}/.killwatchdog"

    if pid=$(read_pid_file "$WATCHDOG_PID_FILE"); then
        if kill -0 "$pid" 2>/dev/null && pid_is_watchdog "$pid"; then
            kill "$pid"
            log_msg "INFO" "Watchdog stopped (PID: $pid)"
            rm -f -- "$WATCHDOG_PID_FILE"
        elif kill -0 "$pid" 2>/dev/null; then
            log_msg "WARN" "Refusing to signal PID $pid: command is not this watchdog"
        else
            rm -f -- "$WATCHDOG_PID_FILE"
        fi
    fi
}

# Show status
show_status() {
    local pid

    echo "==================================="
    echo "LuminariMUD Watchdog Status"
    echo "==================================="

    if pid=$(read_pid_file "$WATCHDOG_PID_FILE" 2>/dev/null); then
        if kill -0 "$pid" 2>/dev/null && pid_is_watchdog "$pid"; then
            echo "Watchdog: RUNNING (PID: $pid)"
        elif kill -0 "$pid" 2>/dev/null; then
            echo "Watchdog: NOT RUNNING (PID belongs to another command)"
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
        if pid=$(read_pid_file "$WATCHDOG_PID_FILE" 2>/dev/null); then
            if kill -0 "$pid" 2>/dev/null && pid_is_watchdog "$pid"; then
                echo "Watchdog already running (PID: $pid)"
                exit 1
            fi
            rm -f -- "$WATCHDOG_PID_FILE"
        fi

        rm -f "${PROJECT_ROOT}/.killwatchdog"

        # Start in background
        nohup "$WATCHDOG_SCRIPT" loop > /dev/null 2>&1 &
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
