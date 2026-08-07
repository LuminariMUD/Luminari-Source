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
SCRIPT_PATH="$(readlink -f -- "$0" 2>/dev/null || true)"
if [[ -z "$SCRIPT_PATH" ]]; then
    SCRIPT_PATH="$(cd "$(dirname "$0")" && pwd)/$(basename "$0")"
fi
readonly SCRIPT_PATH
readonly SCRIPT_NAME="$(basename "$SCRIPT_PATH")"
readonly SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

# A standalone copy keeps its historical behavior; the repository copy lives
# under scripts/autorun and operates on the project root.
PROJECT_ROOT="${LUMINARI_PROJECT_ROOT:-}"
if [[ -z "$PROJECT_ROOT" ]]; then
    if [[ -f "${SCRIPT_DIR}/../../configure.ac" ]]; then
        PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
    else
        PROJECT_ROOT="$SCRIPT_DIR"
    fi
fi

# CRITICAL: Change to the project root so all relative runtime paths work.
# BUT NEVER EXIT! The autorun must continue even if the directory change fails.
if ! cd "$PROJECT_ROOT"; then
    echo "ERROR: Failed to change to project root: $PROJECT_ROOT" >&2
    echo "WARNING: Attempting to continue from current directory: $(pwd)" >&2
    PROJECT_ROOT="$(pwd)"
fi
readonly PROJECT_ROOT

# MUD Configuration (can be overridden by environment variables)
readonly MUD_PORT="${MUD_PORT:-4100}"
readonly MUD_BINARY="${MUD_BINARY:-circle}"
readonly BIN_DIR="${BIN_DIR:-bin}"
readonly LIB_DIR="${LIB_DIR:-lib}"
readonly LOG_DIR="${LOG_DIR:-log}"
readonly DUMPS_DIR="${DUMPS_DIR:-dumps}"

# MUD command-line flags; quick boot skips the rent object-limit scan.
readonly FLAGS="${MUD_FLAGS:--q}"

# Auxiliary services configuration
readonly ENABLE_WEBSOCKET="${ENABLE_WEBSOCKET:-false}"
readonly ENABLE_FLASH="${ENABLE_FLASH:-false}"
readonly HMUD_DIR="${HMUD_DIR:-/home/luminari/public_html/hmud}"
readonly FMUD_DIR="${FMUD_DIR:-/home/luminari/public_html/FMud}"
readonly FLASH_POLICY_PORT="${FLASH_POLICY_PORT:-843}"
readonly FLASH_POLICY_FILE="${FLASH_POLICY_FILE:-${PROJECT_ROOT}/flashpolicy.xml}"

# Logging configuration
readonly BACKLOGS="${BACKLOGS:-6}"
readonly LEN_CRASHLOG="${LEN_CRASHLOG:-30}"
readonly MAX_LOG_SIZE_MB="${MAX_LOG_SIZE_MB:-100}"  # Max size before rotation
readonly LOG_RETENTION_DAYS="${LOG_RETENTION_DAYS:-30}"  # Keep logs for 30 days

# Runtime control options (can be overridden by environment variables)
readonly IGNORE_DISK_SPACE="${IGNORE_DISK_SPACE:-true}"  # Default: keep running even with low disk space
readonly STATE_UPDATE_INTERVAL="${AUTORUN_STATE_INTERVAL:-60}"
readonly AUTORUN_PID_FILE="${PROJECT_ROOT}/.autorun.lock.pid"
readonly MUD_PID_FILE="${PROJECT_ROOT}/.mud.pid"
readonly MUD_IDENTITY_FILE="${PROJECT_ROOT}/.mud.identity"

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

# Escape a string for inclusion in a JSON string value without depending on
# optional runtime tools such as jq or Python.
json_escape() {
    local char
    local code
    local escaped=""
    local index
    local LC_ALL=C
    local value="${1-}"

    for ((index = 0; index < ${#value}; index++)); do
        char="${value:index:1}"
        case "$char" in
            '"') escaped+='\"' ;;
            \\) escaped="${escaped}\\\\" ;;
            $'\b') escaped+='\b' ;;
            $'\f') escaped+='\f' ;;
            $'\n') escaped+='\n' ;;
            $'\r') escaped+='\r' ;;
            $'\t') escaped+='\t' ;;
            *)
                printf -v code '%d' "'$char"
                if ((code < 32)); then
                    printf -v char '\\u%04x' "$code"
                fi
                escaped+="$char"
                ;;
        esac
    done

    printf '%s' "$escaped"
}

# Publish a machine-readable crash record atomically. The file contains only
# process and immutable build identity; configuration and credentials are
# deliberately excluded.
write_last_error() {
    local backtrace="${LAST_CORE_BACKTRACE:-}"
    local core_dump="${LAST_CORE_DUMP:-}"
    local crash_count="${5:-0}"
    local error_type="${1:-MudProcessExit}"
    local exit_code="${3:-0}"
    local last_error_file
    local last_error_tmp
    local message="${2:-MUD process exited unexpectedly}"
    local pid_json=null
    local stack="No backtrace was captured"
    local timestamp
    local uptime_seconds="${4:-0}"

    if [[ ! "$exit_code" =~ ^[0-9]+$ ]]; then
        exit_code=0
    fi
    if [[ ! "$uptime_seconds" =~ ^[0-9]+$ ]]; then
        uptime_seconds=0
    fi
    if [[ ! "$crash_count" =~ ^[0-9]+$ ]]; then
        crash_count=0
    fi
    if [[ "${LAST_MUD_PID:-}" =~ ^[1-9][0-9]*$ ]]; then
        pid_json="$LAST_MUD_PID"
    fi
    if [[ -n "$backtrace" ]]; then
        stack="$backtrace"
    elif [[ -n "$core_dump" ]]; then
        stack="Core captured at ${core_dump}; no text backtrace is available"
    fi

    timestamp=$(date -u '+%Y-%m-%dT%H:%M:%S.%3NZ')
    if ! mkdir -p "$LOG_DIR"; then
        log_error "Unable to create structured error log directory: $LOG_DIR"
        return 1
    fi
    last_error_file="${LOG_DIR}/last_error_${timestamp}.json"
    last_error_tmp=$(mktemp "${LOG_DIR}/.last_error.XXXXXX") || {
        log_error "Unable to create temporary structured error record"
        return 1
    }

    if ! cat > "$last_error_tmp" <<EOF
{
  "timestamp": "$(json_escape "$timestamp")",
  "level": "error",
  "msg": "$(json_escape "$message")",
  "error": {
    "type": "$(json_escape "$error_type")",
    "message": "$(json_escape "$message")",
    "stack": "$(json_escape "$stack")"
  },
  "context": {
    "pid": $pid_json,
    "exit_code": $exit_code,
    "uptime_seconds": $uptime_seconds,
    "crash_count": $crash_count,
    "executable": "$(json_escape "${LAST_MUD_EXECUTABLE:-unknown}")",
    "git_commit": "$(json_escape "${LAST_MUD_GIT_COMMIT:-unknown}")",
    "git_dirty": "$(json_escape "${LAST_MUD_GIT_DIRTY:-unknown}")",
    "elf_build_id": "$(json_escape "${LAST_MUD_BUILD_ID:-unavailable}")",
    "sha256": "$(json_escape "${LAST_MUD_SHA256:-unavailable}")",
    "core_dump": "$(json_escape "$core_dump")",
    "backtrace": "$(json_escape "$backtrace")"
  }
}
EOF
    then
        log_error "Unable to write structured error record"
        rm -f -- "$last_error_tmp"
        return 1
    fi

    chmod 600 "$last_error_tmp" 2>/dev/null || true
    if ! mv -f -- "$last_error_tmp" "$last_error_file"; then
        log_error "Unable to publish structured error record: $last_error_file"
        rm -f -- "$last_error_tmp"
        return 1
    fi

    log_info "Structured error context written to $last_error_file"
}

# Error handling function - NEVER actually die!
die() {
    log_error "ERROR (but not dying): $*"
    # Don't exit! Log the error and return
    return "${2:-1}"
}

# Read and validate a PID file without treating arbitrary contents as a PID.
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

# Publish PID files atomically so readers never observe partial contents.
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

# Read one field from a generated identity or release manifest without
# evaluating its contents as shell input.
read_identity_value() {
    local identity_file="$1"
    local identity_key="$2"

    if [[ ! "$identity_key" =~ ^[A-Z0-9_]+$ ]] ||
       [[ ! -r "$identity_file" ]]; then
        return 1
    fi

    awk -F= -v key="$identity_key" '
        $1 == key {
            print substr($0, index($0, "=") + 1)
            exit
        }
    ' "$identity_file"
}

# Resolve and verify the executable, release manifest, and matching symbols.
# Legacy binaries without a release manifest remain launchable with unknown
# Git identity so an existing installation can be migrated deliberately.
resolve_mud_binary_identity() {
    local binary_path="$1"
    local debug_build_id=""
    local manifest=""
    local manifest_build_id=""
    local manifest_sha256=""
    local real_path=""

    RESOLVED_MUD_EXECUTABLE=""
    RESOLVED_MUD_BUILD_ID="unavailable"
    RESOLVED_MUD_GIT_COMMIT="unknown"
    RESOLVED_MUD_GIT_DIRTY="unknown"
    RESOLVED_MUD_SHA256="unavailable"

    real_path=$(readlink -f -- "$binary_path" 2>/dev/null || true)
    if [[ -z "$real_path" ]] || [[ ! -f "$real_path" ]] || [[ ! -x "$real_path" ]]; then
        return 1
    fi
    RESOLVED_MUD_EXECUTABLE="$real_path"

    if command -v sha256sum >/dev/null 2>&1; then
        RESOLVED_MUD_SHA256=$(sha256sum "$real_path" 2>/dev/null |
            awk '{print $1; exit}')
        if [[ ! "$RESOLVED_MUD_SHA256" =~ ^[0-9a-f]{64}$ ]]; then
            RESOLVED_MUD_SHA256="unavailable"
        fi
    fi

    if command -v readelf >/dev/null 2>&1; then
        RESOLVED_MUD_BUILD_ID=$(readelf -nW "$real_path" 2>/dev/null |
            awk '/Build ID:/ {print tolower($NF); exit}')
        if [[ ! "$RESOLVED_MUD_BUILD_ID" =~ ^[0-9a-f]{16,}$ ]]; then
            RESOLVED_MUD_BUILD_ID="unavailable"
        fi
    fi

    manifest="$(dirname "$real_path")/manifest"
    if [[ ! -r "$manifest" ]]; then
        if [[ "$real_path" == */releases/*/circle ]]; then
            return 2
        fi
        return 0
    fi

    manifest_build_id=$(read_identity_value "$manifest" ELF_BUILD_ID)
    manifest_sha256=$(read_identity_value "$manifest" SHA256)
    RESOLVED_MUD_GIT_COMMIT=$(read_identity_value "$manifest" GIT_COMMIT)
    RESOLVED_MUD_GIT_DIRTY=$(read_identity_value "$manifest" GIT_DIRTY)
    if [[ "$manifest_build_id" != "$RESOLVED_MUD_BUILD_ID" ]] ||
       [[ "$manifest_sha256" != "$RESOLVED_MUD_SHA256" ]] ||
       [[ "$RESOLVED_MUD_GIT_COMMIT" != unknown &&
          ! "$RESOLVED_MUD_GIT_COMMIT" =~ ^[0-9a-f]{40}$ ]] ||
       [[ "$RESOLVED_MUD_GIT_DIRTY" != 0 && "$RESOLVED_MUD_GIT_DIRTY" != 1 ]]; then
        return 2
    fi

    if [[ ! -r "$(dirname "$real_path")/circle.debug" ]]; then
        return 2
    fi
    debug_build_id=$(readelf -nW "$(dirname "$real_path")/circle.debug" 2>/dev/null |
        awk '/Build ID:/ {print tolower($NF); exit}')
    if [[ "$debug_build_id" != "$RESOLVED_MUD_BUILD_ID" ]]; then
        return 2
    fi

    return 0
}

# Publish the exact launched executable identity atomically. The file survives
# process exit until crash collection has used it.
write_mud_identity() {
    local identity_tmp
    local mud_pid="$1"

    identity_tmp=$(mktemp "${MUD_IDENTITY_FILE}.tmp.XXXXXX") || return 1
    if ! cat > "$identity_tmp" <<EOF
PID=$mud_pid
EXECUTABLE=$LAST_MUD_EXECUTABLE
GIT_COMMIT=$LAST_MUD_GIT_COMMIT
GIT_DIRTY=$LAST_MUD_GIT_DIRTY
ELF_BUILD_ID=$LAST_MUD_BUILD_ID
SHA256=$LAST_MUD_SHA256
EOF
    then
        rm -f -- "$identity_tmp"
        return 1
    fi

    if ! mv -f -- "$identity_tmp" "$MUD_IDENTITY_FILE"; then
        rm -f -- "$identity_tmp"
        return 1
    fi
}

# Return the executable recorded for a specific MUD PID. Refuse stale or
# malformed identity rather than following the current launch alias.
get_recorded_mud_executable() {
    local expected_pid="$1"
    local identity_pid
    local recorded_executable

    identity_pid=$(read_identity_value "$MUD_IDENTITY_FILE" PID 2>/dev/null || true)
    recorded_executable=$(read_identity_value "$MUD_IDENTITY_FILE" EXECUTABLE 2>/dev/null || true)
    if [[ "$identity_pid" != "$expected_pid" ]] ||
       [[ -z "$recorded_executable" ]] ||
       [[ "$recorded_executable" != /* ]] ||
       [[ ! -f "$recorded_executable" ]]; then
        return 1
    fi

    printf '%s\n' "$recorded_executable"
}

# Verify both the PID and its exact executable/script before sending a signal.
# For interpreted scripts, /proc records the interpreter as argv[0] and the
# script path as argv[1].
pid_matches_command() {
    local pid="$1"
    local expected_command="$2"
    local expected_argument="${3:-}"
    local candidate=""
    local expected_path=""
    local matched_index=-1
    local process_cwd=""
    local process_exe=""
    local -a command_line=()

    if [[ ! "$pid" =~ ^[1-9][0-9]*$ ]] ||
       [[ ! -r "/proc/${pid}/cmdline" ]]; then
        return 1
    fi

    expected_path=$(readlink -f -- "$expected_command" 2>/dev/null || true)
    if [[ -z "$expected_path" ]]; then
        return 1
    fi

    mapfile -d '' -t command_line < "/proc/${pid}/cmdline" 2>/dev/null || return 1
    if [[ ${#command_line[@]} -eq 0 ]]; then
        return 1
    fi

    process_exe=$(readlink -f -- "/proc/${pid}/exe" 2>/dev/null || true)
    if [[ "$process_exe" == "$expected_path" ]]; then
        matched_index=0
    else
        process_cwd=$(readlink -f -- "/proc/${pid}/cwd" 2>/dev/null || true)

        if [[ "${command_line[0]}" == /* ]]; then
            candidate=$(readlink -f -- "${command_line[0]}" 2>/dev/null || true)
        elif [[ "${command_line[0]}" == */* ]] && [[ -n "$process_cwd" ]]; then
            candidate=$(readlink -f -- "${process_cwd}/${command_line[0]}" 2>/dev/null || true)
        fi
        if [[ "$candidate" == "$expected_path" ]]; then
            matched_index=0
        fi

        if [[ $matched_index -lt 0 ]] && [[ ${#command_line[@]} -gt 1 ]]; then
            candidate=""
            if [[ "${command_line[1]}" == /* ]]; then
                candidate=$(readlink -f -- "${command_line[1]}" 2>/dev/null || true)
            elif [[ "${command_line[1]}" == */* ]] && [[ -n "$process_cwd" ]]; then
                candidate=$(readlink -f -- "${process_cwd}/${command_line[1]}" 2>/dev/null || true)
            fi

            case "$(basename "$process_exe")" in
                bash|dash|sh|python|python[0-9]*|perl|ruby)
                    if [[ "$candidate" == "$expected_path" ]]; then
                        matched_index=1
                    fi
                    ;;
            esac
        fi
    fi

    if [[ $matched_index -lt 0 ]]; then
        return 1
    fi

    if [[ -n "$expected_argument" ]] &&
       [[ "${command_line[$((matched_index + 1))]:-}" != "$expected_argument" ]]; then
        return 1
    fi

    return 0
}

# Check for an exact command-line argument without using global process search.
pid_has_argument() {
    local pid="$1"
    local expected_argument="$2"
    local argument
    local -a command_line=()

    if [[ ! "$pid" =~ ^[1-9][0-9]*$ ]] ||
       [[ ! -r "/proc/${pid}/cmdline" ]]; then
        return 1
    fi

    mapfile -d '' -t command_line < "/proc/${pid}/cmdline" 2>/dev/null || return 1
    for argument in "${command_line[@]:1}"; do
        if [[ "$argument" == "$expected_argument" ]]; then
            return 0
        fi
    done

    return 1
}

# Resolve this checkout's supervisor from its PID file or atomic state file.
# The state fallback supports upgrading a running supervisor that predates the
# PID-file fix.
get_verified_autorun_pid() {
    local pid=""
    local state_file="${PROJECT_ROOT}/.autorun.state"

    pid=$(read_pid_file "$AUTORUN_PID_FILE" 2>/dev/null || true)
    if [[ -n "$pid" ]] &&
       kill -0 "$pid" 2>/dev/null &&
       pid_matches_command "$pid" "${SCRIPT_DIR}/${SCRIPT_NAME}" "foreground"; then
        printf '%s\n' "$pid"
        return 0
    fi

    if [[ -r "$state_file" ]]; then
        pid=$(awk -F= '$1 == "PID" {print $2; exit}' "$state_file")
    fi
    if [[ "$pid" =~ ^[1-9][0-9]*$ ]] &&
       kill -0 "$pid" 2>/dev/null &&
       pid_matches_command "$pid" "${SCRIPT_DIR}/${SCRIPT_NAME}" "foreground"; then
        printf '%s\n' "$pid"
        return 0
    fi

    return 1
}

# Resolve a legacy MUD without a PID file only through its verified supervisor's
# direct children. Refuse ambiguity rather than risking another MUD on the host.
get_verified_mud_child_pid() {
    local child
    local children_file
    local expected_command
    local found_pid=""
    local mud_command="${BIN_DIR}/${MUD_BINARY}"
    local supervisor_pid
    local -a children=()

    if ! supervisor_pid=$(get_verified_autorun_pid); then
        return 1
    fi

    children_file="/proc/${supervisor_pid}/task/${supervisor_pid}/children"
    if [[ ! -r "$children_file" ]]; then
        return 1
    fi
    if [[ "$mud_command" != /* ]]; then
        mud_command="${PROJECT_ROOT}/${mud_command}"
    fi

    read -r -a children < "$children_file" || true
    for child in "${children[@]}"; do
        expected_command=$(get_recorded_mud_executable "$child" 2>/dev/null || true)
        if [[ -z "$expected_command" ]]; then
            expected_command="$mud_command"
        fi
        if kill -0 "$child" 2>/dev/null &&
           pid_matches_command "$child" "$expected_command" "" &&
           pid_has_argument "$child" "$MUD_PORT"; then
            if [[ -n "$found_pid" ]]; then
                return 2
            fi
            found_pid="$child"
        fi
    done

    if [[ -z "$found_pid" ]]; then
        return 1
    fi

    printf '%s\n' "$found_pid"
}

# Signal an already-resolved process only after exact command verification.
signal_verified_process() {
    local pid="$1"
    local expected_command="$2"
    local expected_argument="$3"
    local process_label="$4"

    if ! kill -0 "$pid" 2>/dev/null; then
        log_info "${process_label} PID $pid is no longer running"
        return 1
    fi

    if ! pid_matches_command "$pid" "$expected_command" "$expected_argument"; then
        log_warn "Refusing to signal ${process_label} PID $pid: command does not match"
        return 2
    fi

    log_info "Stopping ${process_label} (PID: $pid)"
    kill -TERM "$pid" 2>/dev/null
}

# Signal only a process proven to belong to this checkout.
signal_pid_file_process() {
    local pid_file="$1"
    local expected_command="$2"
    local expected_argument="$3"
    local process_label="$4"
    local pid

    if ! pid=$(read_pid_file "$pid_file"); then
        log_info "No recorded ${process_label} process found"
        return 1
    fi

    if ! kill -0 "$pid" 2>/dev/null; then
        log_info "Removing stale ${process_label} PID file (PID: $pid)"
        rm -f -- "$pid_file"
        return 1
    fi

    signal_verified_process \
        "$pid" \
        "$expected_command" \
        "$expected_argument" \
        "$process_label"
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
        # Last resort: trust only the PID file after exact command verification.
        get_mud_pid >/dev/null
    fi
}

# Get MUD process PID
get_mud_pid() {
    local mud_command="${BIN_DIR}/${MUD_BINARY}"
    local pid

    if [[ "$mud_command" != /* ]]; then
        mud_command="${PROJECT_ROOT}/${mud_command}"
    fi

    if ! pid=$(read_pid_file "$MUD_PID_FILE"); then
        get_verified_mud_child_pid
        return $?
    fi

    mud_command=$(get_recorded_mud_executable "$pid" 2>/dev/null || printf '%s\n' "$mud_command")

    if kill -0 "$pid" 2>/dev/null &&
       pid_matches_command "$pid" "$mud_command" "" &&
       pid_has_argument "$pid" "$MUD_PORT"; then
        printf '%s\n' "$pid"
        return 0
    fi

    get_verified_mud_child_pid
}

#############################################################################
# Core Dump Management
#############################################################################

# Archive a local or systemd-managed core and analyze it with the exact
# immutable executable that produced it.
archive_core_dump() {
    local bt_file
    local cf
    local core_file=""
    local core_pattern
    local crash_exit_code="${1:-0}"
    local dump_name
    local dump_path
    local executable_sha256
    local gdb_commands
    local identity_file
    local process_start_time="${2:-0}"
    local system_info_tmp
    local -a possible_cores=()

    LAST_CORE_DUMP=""
    LAST_CORE_BACKTRACE=""

    shopt -s nullglob
    possible_cores=(
        "${LIB_DIR}"/core
        "${LIB_DIR}"/core.*
        core
        core.*
        "${BIN_DIR}"/core
        "${BIN_DIR}"/core.*
    )
    shopt -u nullglob

    if [[ ! "$process_start_time" =~ ^[0-9]+$ ]]; then
        process_start_time=0
    fi
    for cf in "${possible_cores[@]:-}"; do
        if [[ -f "$cf" ]] &&
           [[ $(stat -c '%Y' "$cf" 2>/dev/null || printf '0') -ge $process_start_time ]] &&
           { [[ -z "$core_file" ]] || [[ "$cf" -nt "$core_file" ]]; }; then
            core_file="$cf"
        fi
    done

    mkdir -p "$DUMPS_DIR"
    dump_name="core.${HOSTNAME}.$(date "+${DATE_FORMAT_DUMP}").pid-${LAST_MUD_PID:-unknown}.build-${LAST_MUD_BUILD_ID:-unknown}"
    dump_path="${DUMPS_DIR}/${dump_name}"

    if [[ -z "$core_file" ]] &&
       [[ "$LAST_MUD_PID" =~ ^[1-9][0-9]*$ ]] &&
       command -v coredumpctl >/dev/null 2>&1; then
        if coredumpctl dump "$LAST_MUD_PID" --output="$dump_path" >/dev/null 2>&1; then
            core_file="$dump_path"
            log_info "Retrieved core for PID $LAST_MUD_PID from systemd-coredump"
        else
            rm -f -- "$dump_path"
        fi
    fi

    if [[ -z "$core_file" ]]; then
        if [[ "$crash_exit_code" -ne 0 ]]; then
            core_pattern=$(cat /proc/sys/kernel/core_pattern 2>/dev/null || printf 'unavailable')
            log_warn "No retrievable core for PID ${LAST_MUD_PID:-unknown}; kernel core pattern: $core_pattern"
        fi
        return 0
    fi

    log_info "Archiving core dump to ${dump_path}"

    if [[ "$core_file" == "$dump_path" ]] || mv "$core_file" "$dump_path" 2>/dev/null; then
        log_info "Core dump archived successfully"
        LAST_CORE_DUMP="$dump_path"

        identity_file="${DUMPS_DIR}/identity.${dump_name}.txt"
        {
            printf 'PID=%s\n' "${LAST_MUD_PID:-unknown}"
            printf 'EXECUTABLE=%s\n' "${LAST_MUD_EXECUTABLE:-unknown}"
            printf 'GIT_COMMIT=%s\n' "${LAST_MUD_GIT_COMMIT:-unknown}"
            printf 'GIT_DIRTY=%s\n' "${LAST_MUD_GIT_DIRTY:-unknown}"
            printf 'ELF_BUILD_ID=%s\n' "${LAST_MUD_BUILD_ID:-unknown}"
            printf 'SHA256=%s\n' "${LAST_MUD_SHA256:-unknown}"
            printf 'EXIT_CODE=%s\n' "$crash_exit_code"
        } > "$identity_file"

        if [[ -z "${LAST_MUD_EXECUTABLE:-}" ]] || [[ ! -f "$LAST_MUD_EXECUTABLE" ]]; then
            log_error "Exact MUD executable is unavailable; preserving core without a backtrace"
            return 0
        fi
        executable_sha256=$(sha256sum "$LAST_MUD_EXECUTABLE" 2>/dev/null |
            awk '{print $1; exit}')
        if [[ "$LAST_MUD_SHA256" != unavailable ]] &&
           [[ "$executable_sha256" != "$LAST_MUD_SHA256" ]]; then
            log_error "Exact MUD executable failed its recorded SHA-256; preserving core without GDB"
            return 0
        fi

        if command -v gdb >/dev/null 2>&1; then
            bt_file="${DUMPS_DIR}/backtrace.${dump_name}.txt"
            log_info "Generating backtrace to ${bt_file}"

            gdb_commands=$(mktemp "${DUMPS_DIR}/.gdb-commands.XXXXXX")
            cat > "$gdb_commands" <<EOF
echo === ALL THREAD BACKTRACES ===\n
thread apply all bt full
echo \n=== CURRENT THREAD REGISTERS ===\n
info registers
echo \n=== THREADS ===\n
info threads
echo \n=== MEMORY MAPPINGS ===\n
info proc mappings
echo \n=== SHARED LIBRARIES ===\n
info sharedlibrary
quit
EOF

            gdb "$LAST_MUD_EXECUTABLE" "$dump_path" -batch -command "$gdb_commands" \
                > "$bt_file" 2>&1
            rm -f -- "$gdb_commands"

            system_info_tmp=$(mktemp "${bt_file}.tmp.XXXXXX")
            {
                echo "=== SYSTEM INFORMATION ==="
                echo "Date: $(date)"
                echo "Hostname: $(hostname)"
                echo "Uptime: $(uptime)"
                echo "Memory: $(free -h 2>/dev/null || true)"
                echo ""
                cat "$bt_file"
            } > "$system_info_tmp" && mv -f -- "$system_info_tmp" "$bt_file"

            log_info "Backtrace generated successfully"
            LAST_CORE_BACKTRACE="$bt_file"
        else
            log_warn "gdb not found - cannot generate backtrace"
        fi
    else
        log_error "Failed to archive core dump"
    fi
}

log_core_capture_configuration() {
    local core_limit
    local core_pattern

    core_limit=$(ulimit -Sc 2>/dev/null || printf 'unavailable')
    core_pattern=$(cat /proc/sys/kernel/core_pattern 2>/dev/null || printf 'unavailable')
    log_info "Core capture configuration: soft_limit=$core_limit pattern=$core_pattern"
    if [[ "$core_pattern" == \|* ]] &&
       ! command -v coredumpctl >/dev/null 2>&1; then
        log_warn "Kernel cores are owned by a pipe handler and cannot be retrieved by autorun; run scripts/debugging/verify_core_capture.sh --self-test on this host"
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
        find "$LOG_DIR" -name "syslog.*" -type f -mtime +"$LOG_RETENTION_DAYS" -delete 2>/dev/null || true
        find "$LOG_DIR" -name "last_error_*.json" -type f \
            -mtime +"$LOG_RETENTION_DAYS" -delete 2>/dev/null || true
        find "$DUMPS_DIR" -name "core.*" -type f -mtime +"$LOG_RETENTION_DAYS" -delete 2>/dev/null || true
        find "$DUMPS_DIR" -name "backtrace.*" -type f \
            -mtime +"$LOG_RETENTION_DAYS" -delete 2>/dev/null || true
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
        if ! write_pid_file "${PROJECT_ROOT}/.websocket_policy.pid" "$pid"; then
            log_error "Unable to publish WebSocket policy daemon PID"
        fi
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
        if ! write_pid_file "${PROJECT_ROOT}/.flash_policy.pid" "$pid"; then
            log_error "Unable to publish Flash policy daemon PID"
        fi
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
    if [[ -f "${PROJECT_ROOT}/.websocket_policy.pid" ]]; then
        signal_pid_file_process \
            "${PROJECT_ROOT}/.websocket_policy.pid" \
            "${HMUD_DIR}/policyd" \
            "" \
            "WebSocket policy daemon" || true
        rm -f "${PROJECT_ROOT}/.websocket_policy.pid"
    fi

    # Stop flash policy daemon
    if [[ -f "${PROJECT_ROOT}/.flash_policy.pid" ]]; then
        signal_pid_file_process \
            "${PROJECT_ROOT}/.flash_policy.pid" \
            "${FMUD_DIR}/flashpolicyd.py" \
            "" \
            "Flash policy daemon" || true
        rm -f "${PROJECT_ROOT}/.flash_policy.pid"
    fi
}

#############################################################################
# Process Management
#############################################################################

# Clean up zombie or defunct MUD processes
cleanup_zombie_processes() {
    local mud_command="${BIN_DIR}/${MUD_BINARY}"
    local pid
    local process_state

    if [[ "$mud_command" != /* ]]; then
        mud_command="${PROJECT_ROOT}/${mud_command}"
    fi

    if ! pid=$(read_pid_file "$MUD_PID_FILE"); then
        return 0
    fi

    mud_command=$(get_recorded_mud_executable "$pid" 2>/dev/null || printf '%s\n' "$mud_command")

    if ! kill -0 "$pid" 2>/dev/null; then
        rm -f -- "$MUD_PID_FILE"
        return 0
    fi

    if ! pid_matches_command "$pid" "$mud_command" ""; then
        log_warn "Ignoring MUD PID $pid: command does not match this checkout"
        return 0
    fi

    process_state=$(ps -o stat= -p "$pid" 2>/dev/null || true)
    if [[ "$process_state" == Z* ]]; then
        log_warn "Recorded MUD PID $pid is defunct and must be reaped by its parent"
        return 0
    fi

    # A recorded MUD that is no longer listening may be stuck. Only signal the
    # exact process recorded by this checkout.
    if ! is_mud_running; then
        log_warn "Recorded MUD PID $pid is not listening; requesting shutdown"
        kill -TERM "$pid" 2>/dev/null || true
    fi
}

# Verify MUD binary exists and is executable
verify_mud_binary() {
    local binary_path="${BIN_DIR}/${MUD_BINARY}"
    local identity_status

    # Security check: ensure paths don't contain shell metacharacters
    if [[ "$binary_path" =~ [';|&<>$`'] ]]; then
        log_error "Invalid characters in binary path: $binary_path"
        return 2
    fi

    resolve_mud_binary_identity "$binary_path"
    identity_status=$?
    if [[ $identity_status -eq 1 ]]; then
        log_error "MUD binary not found or not executable: $binary_path"
        return 2
    elif [[ $identity_status -ne 0 ]]; then
        log_error "MUD release identity or matching debug symbols failed verification: $binary_path"
        return 2
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
    local exit_code
    local heartbeat_pid=""
    local identity_status
    local mud_pid
    local -a mud_flags=()

    log_info "Starting MUD server on port $MUD_PORT"
    log_info "Command: ${BIN_DIR}/${MUD_BINARY} ${FLAGS} ${MUD_PORT}"

    # Start the MUD and capture its exit code
    # CRITICAL: We must handle ALL possible failures gracefully
    set +e

    resolve_mud_binary_identity "${BIN_DIR}/${MUD_BINARY}"
    identity_status=$?
    if [[ $identity_status -ne 0 ]]; then
        log_error "MUD binary identity verification failed: ${BIN_DIR}/${MUD_BINARY}"
        log_info "Binary missing - waiting 60 seconds before retry"
        return 1
    fi

    LAST_MUD_EXECUTABLE="$RESOLVED_MUD_EXECUTABLE"
    LAST_MUD_BUILD_ID="$RESOLVED_MUD_BUILD_ID"
    LAST_MUD_GIT_COMMIT="$RESOLVED_MUD_GIT_COMMIT"
    LAST_MUD_GIT_DIRTY="$RESOLVED_MUD_GIT_DIRTY"
    LAST_MUD_SHA256="$RESOLVED_MUD_SHA256"
    log_info "Resolved MUD executable: $LAST_MUD_EXECUTABLE"
    log_info "Release identity: git_commit=$LAST_MUD_GIT_COMMIT dirty=$LAST_MUD_GIT_DIRTY build_id=$LAST_MUD_BUILD_ID sha256=$LAST_MUD_SHA256"

    # Run the MUD in the foreground as before, but do not let it inherit the
    # autorun lock descriptor.
    read -r -a mud_flags <<< "$FLAGS"
    LUMINARI_ELF_BUILD_ID="$LAST_MUD_BUILD_ID" \
        "$LAST_MUD_EXECUTABLE" "${mud_flags[@]}" "$MUD_PORT" 200>&- >> syslog 2>&1 &
    mud_pid=$!
    LAST_MUD_PID="$mud_pid"
    ACTIVE_MUD_PID="$mud_pid"
    if ! write_pid_file "$MUD_PID_FILE" "$mud_pid" ||
       ! write_mud_identity "$mud_pid"; then
        log_error "Unable to publish MUD PID and release identity"
        kill -TERM "$mud_pid" 2>/dev/null || true
        wait "$mud_pid" 2>/dev/null || true
        exit_code=1
    else
        write_autorun_state
        # Fork the heartbeat only after active identity is populated so its
        # subshell reports the exact running release while the parent waits.
        (
            trap 'exit 0' INT TERM
            while kill -0 "$AUTORUN_PID" 2>/dev/null; do
                sleep "$STATE_UPDATE_INTERVAL"
                if kill -0 "$AUTORUN_PID" 2>/dev/null; then
                    write_autorun_state
                fi
            done
        ) 200>&- &
        heartbeat_pid=$!
        wait "$mud_pid"
        exit_code=$?
    fi

    if [[ "$(read_pid_file "$MUD_PID_FILE" 2>/dev/null || true)" == "$mud_pid" ]]; then
        rm -f -- "$MUD_PID_FILE"
    fi
    # The MUD has exited, so stop and reap its state heartbeat.
    if [[ "$heartbeat_pid" =~ ^[1-9][0-9]*$ ]]; then
        kill "$heartbeat_pid" 2>/dev/null || true
        wait "$heartbeat_pid" 2>/dev/null || true
    fi
    ACTIVE_MUD_PID=""
    write_autorun_state

    # NO MATTER WHAT HAPPENS, WE CONTINUE!
    # The script should continue running even if the MUD explodes spectacularly

    log_info "MUD server exited with code $exit_code"

    # Return the exit code to the main loop for crash accounting.
    return "$exit_code"
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
        write_autorun_state
        sleep 60
    done
}

# Display status information
show_status() {
    local active_build_id="unknown"
    local active_commit="unknown"
    local active_dirty="unknown"
    local active_executable="unknown"
    local active_sha256="unknown"
    local identity_match="unknown"
    local installed_build_id="unknown"
    local installed_commit="unknown"
    local installed_executable="unknown"
    local pid=""

    echo "========================================"
    echo "LuminariMUD Autorun Status"
    echo "========================================"
    echo "Script: $SCRIPT_NAME"
    echo "MUD Port: $MUD_PORT"
    echo "MUD Binary: ${BIN_DIR}/${MUD_BINARY}"

    pid=$(get_mud_pid 2>/dev/null || true)
    if [[ -n "$pid" ]]; then
        if is_mud_running; then
            echo "MUD Status: RUNNING (PID: $pid)"
        else
            echo "MUD Status: STARTING OR RESTARTING (PID: $pid)"
        fi
        active_executable=$(read_identity_value "$MUD_IDENTITY_FILE" EXECUTABLE 2>/dev/null ||
            printf 'unknown')
        active_commit=$(read_identity_value "$MUD_IDENTITY_FILE" GIT_COMMIT 2>/dev/null ||
            printf 'unknown')
        active_dirty=$(read_identity_value "$MUD_IDENTITY_FILE" GIT_DIRTY 2>/dev/null ||
            printf 'unknown')
        active_build_id=$(read_identity_value "$MUD_IDENTITY_FILE" ELF_BUILD_ID 2>/dev/null ||
            printf 'unknown')
        active_sha256=$(read_identity_value "$MUD_IDENTITY_FILE" SHA256 2>/dev/null ||
            printf 'unknown')
        echo "Active Executable: $active_executable"
        echo "Active Git Commit: $active_commit (dirty=$active_dirty)"
        echo "Active ELF Build ID: $active_build_id"
        echo "Active SHA-256: $active_sha256"
    elif is_mud_running; then
        echo "MUD Status: PORT LISTENING (managed PID unavailable)"
    else
        echo "MUD Status: NOT RUNNING"
    fi

    if resolve_mud_binary_identity "${BIN_DIR}/${MUD_BINARY}"; then
        installed_executable="$RESOLVED_MUD_EXECUTABLE"
        installed_commit="$RESOLVED_MUD_GIT_COMMIT"
        installed_build_id="$RESOLVED_MUD_BUILD_ID"
        echo "Installed Executable: $installed_executable"
        echo "Installed Git Commit: $installed_commit (dirty=$RESOLVED_MUD_GIT_DIRTY)"
        echo "Installed ELF Build ID: $installed_build_id"
        echo "Installed SHA-256: $RESOLVED_MUD_SHA256"
        if [[ -n "$pid" ]]; then
            if [[ "$active_executable" == "$installed_executable" ]] &&
               [[ "$active_build_id" == "$installed_build_id" ]]; then
                identity_match="yes"
            else
                identity_match="no - restart required"
            fi
            echo "Active Matches Installed: $identity_match"
        fi
    else
        echo "Installed Identity: INVALID OR UNAVAILABLE"
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
        autorun_pid=""
        mud_signal_sent=false
        mud_signal_status=1
        mud_pid=""
        mud_command="${BIN_DIR}/${MUD_BINARY}"
        if [[ "$mud_command" != /* ]]; then
            mud_command="${PROJECT_ROOT}/${mud_command}"
        fi
        mud_pid=$(read_pid_file "$MUD_PID_FILE" 2>/dev/null || true)
        if [[ -n "$mud_pid" ]]; then
            mud_command=$(get_recorded_mud_executable "$mud_pid" 2>/dev/null ||
                printf '%s\n' "$mud_command")
        fi

        log_info "Stopping autorun"
        touch .killscript

        # Stop the watchdog first if it's running
        if [[ -f "${SCRIPT_DIR}/autorun-watchdog.sh" ]]; then
            log_info "Stopping watchdog..."
            "${SCRIPT_DIR}/autorun-watchdog.sh" stop 2>/dev/null || true
        fi

        # Stop the recorded MUD first so the foreground supervisor's wait
        # returns naturally and it can consume .killscript.
        if [[ -e "$MUD_PID_FILE" ]]; then
            signal_pid_file_process \
                "$MUD_PID_FILE" \
                "$mud_command" \
                "" \
                "MUD server"
            mud_signal_status=$?
        fi

        if [[ $mud_signal_status -eq 1 ]]; then
            if mud_pid=$(get_verified_mud_child_pid); then
                log_info "Using verified supervisor child for legacy MUD shutdown"
                mud_command=$(get_recorded_mud_executable "$mud_pid" 2>/dev/null ||
                    printf '%s\n' "$mud_command")
                signal_verified_process "$mud_pid" "$mud_command" "" "MUD server"
                mud_signal_status=$?
            else
                mud_resolution_status=$?
                if [[ $mud_resolution_status -eq 2 ]]; then
                    log_warn "Refusing MUD shutdown: multiple verified children found"
                fi
            fi
        fi

        if [[ $mud_signal_status -eq 0 ]]; then
            mud_signal_sent=true
        fi

        if [[ "$mud_signal_sent" != true ]]; then
            # With no managed MUD to wake the supervisor, interrupt its current
            # sleep so it can consume .killscript.
            autorun_signal_status=1
            if [[ -e "$AUTORUN_PID_FILE" ]]; then
                signal_pid_file_process \
                    "$AUTORUN_PID_FILE" \
                    "${SCRIPT_DIR}/${SCRIPT_NAME}" \
                    "foreground" \
                    "autorun supervisor"
                autorun_signal_status=$?
            fi

            if [[ $autorun_signal_status -eq 1 ]] &&
               autorun_pid=$(get_verified_autorun_pid); then
                log_info "Using verified state PID for legacy supervisor shutdown"
                signal_verified_process \
                    "$autorun_pid" \
                    "${SCRIPT_DIR}/${SCRIPT_NAME}" \
                    "foreground" \
                    "autorun supervisor" || true
            fi
        else
            log_info "Autorun supervisor will stop after the MUD exits"
        fi

        if is_mud_running && [[ "$mud_signal_sent" != true ]]; then
            log_warn "MUD port is still active without a verified PID"
            log_warn "Refusing broad process matching"
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
        echo "  AUTORUN_STATE_INTERVAL - State heartbeat seconds (default: 60)"
        exit 0
        ;;
    ""|*)
        daemon_pid=""

        # DEFAULT BEHAVIOR: Daemonize when no arguments
        # Proper daemonization with lock file to prevent multiple instances
        lockfile="${PROJECT_ROOT}/.autorun.lock"

        # The lock file is a persistent inode. It may be empty while actively
        # locked, so never unlink it based on its contents.
        exec 200>"$lockfile"
        if ! flock -n 200; then
            log_error "Another autorun instance is already running"
            exit 1
        fi
        rm -f "$AUTORUN_PID_FILE"

        # Keep the lock in a dedicated wrapper. The foreground supervisor starts
        # with descriptor 200 closed so none of its children can inherit the lock.
        (
            # Detach from terminal completely
            exec </dev/null >/dev/null 2>&1

            # Keep the lock wrapper alive until the supervisor exits
            trap '' HUP QUIT PIPE
            trap ':' TERM

            nohup "${SCRIPT_DIR}/${SCRIPT_NAME}" foreground 200>&- &

            DAEMON_PID=$!
            if ! write_pid_file "$AUTORUN_PID_FILE" "$DAEMON_PID"; then
                kill -TERM "$DAEMON_PID" 2>/dev/null || true
                wait "$DAEMON_PID" 2>/dev/null || true
                exit 1
            fi
            wait "$DAEMON_PID"
        ) &

        # Wait briefly for the supervisor to publish its PID.
        for ((startup_attempt = 0; startup_attempt < 50; startup_attempt++)); do
            daemon_pid=$(read_pid_file "$AUTORUN_PID_FILE" 2>/dev/null || true)
            if [[ -n "$daemon_pid" ]] && kill -0 "$daemon_pid" 2>/dev/null; then
                break
            fi
            sleep 0.1
        done

        if [[ -z "$daemon_pid" ]] || ! kill -0 "$daemon_pid" 2>/dev/null; then
            log_error "LuminariMUD daemon failed to start"
            exit 1
        fi

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
    local mud_command="${BIN_DIR}/${MUD_BINARY}"
    local pidfile old_pid

    for pidfile in .websocket_policy.pid .flash_policy.pid; do
        if [[ -f "$pidfile" ]]; then
            old_pid=$(read_pid_file "$pidfile" 2>/dev/null || true)
            if [[ -n "$old_pid" ]] && ! kill -0 "$old_pid" 2>/dev/null; then
                log_info "Removing stale PID file: $pidfile (PID: $old_pid)"
                rm -f "$pidfile"
            fi
        fi
    done

    if [[ "$mud_command" != /* ]]; then
        mud_command="${PROJECT_ROOT}/${mud_command}"
    fi
    if [[ -f "$MUD_PID_FILE" ]]; then
        old_pid=$(read_pid_file "$MUD_PID_FILE" 2>/dev/null || true)
        if [[ -n "$old_pid" ]]; then
            mud_command=$(get_recorded_mud_executable "$old_pid" 2>/dev/null ||
                printf '%s\n' "$mud_command")
        fi
        if [[ -z "$old_pid" ]] ||
           ! kill -0 "$old_pid" 2>/dev/null ||
           ! pid_matches_command "$old_pid" "$mud_command" ""; then
            log_info "Removing stale MUD PID file"
            rm -f -- "$MUD_PID_FILE"
        fi
    fi
}
cleanup_stale_pidfiles

# Set core dump size to unlimited
ulimit -c unlimited
log_info "Core dump size set to unlimited"
log_core_capture_configuration

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
    local watchdog_pid_file="${PROJECT_ROOT}/.watchdog.pid"
    local wpid

    # Check if watchdog script exists
    if [[ ! -f "$watchdog_script" ]]; then
        log_info "Watchdog script not found - running without extra protection"
        return 0
    fi

    # Check if watchdog is already running
    if [[ -f "$watchdog_pid_file" ]]; then
        wpid=$(read_pid_file "$watchdog_pid_file" 2>/dev/null || true)
        if [[ -n "$wpid" ]] &&
           kill -0 "$wpid" 2>/dev/null &&
           pid_matches_command "$wpid" "$watchdog_script" "loop"; then
            log_info "Watchdog already running (PID: $wpid)"
            return 0
        fi
        rm -f -- "$watchdog_pid_file"
    fi

    # Start the watchdog
    log_info "Starting autorun watchdog for extra protection"
    if [[ ! -x "$watchdog_script" ]]; then
        chmod +x "$watchdog_script" 2>/dev/null || true
    fi

    if "$watchdog_script" start >/dev/null 2>&1; then
        log_info "Watchdog started successfully"
    else
        log_error "Watchdog failed to start"
        return 1
    fi
}

# Production monitoring
CRASH_COUNT=0
CRASH_WINDOW_START=$(date +%s)
MAX_CRASHES_PER_HOUR=10
MAX_UPTIME_HOURS="${MAX_UPTIME_HOURS:-168}"  # Default: restart after 7 days
ACTIVE_MUD_PID=""
LAST_MUD_PID=""
LAST_MUD_EXECUTABLE=""
LAST_MUD_GIT_COMMIT="unknown"
LAST_MUD_GIT_DIRTY="unknown"
LAST_MUD_BUILD_ID="unavailable"
LAST_MUD_SHA256="unavailable"
LAST_CORE_DUMP=""
LAST_CORE_BACKTRACE=""

# Autorun health tracking
AUTORUN_START_TIME=$(date +%s)
AUTORUN_PID=$$
log_info "Autorun started with PID $AUTORUN_PID at $(date)"

# Write autorun state file for external monitoring
write_autorun_state() {
    local active_build_id=""
    local active_commit=""
    local active_dirty=""
    local active_executable=""
    local active_identity_match="not-running"
    local active_sha256=""
    local installed_build_id="unavailable"
    local installed_commit="unknown"
    local installed_dirty="unknown"
    local installed_executable="unavailable"
    local installed_sha256="unavailable"
    local state_file="${PROJECT_ROOT}/.autorun.state"
    local state_tmp

    if resolve_mud_binary_identity "${BIN_DIR}/${MUD_BINARY}"; then
        installed_build_id="$RESOLVED_MUD_BUILD_ID"
        installed_commit="$RESOLVED_MUD_GIT_COMMIT"
        installed_dirty="$RESOLVED_MUD_GIT_DIRTY"
        installed_executable="$RESOLVED_MUD_EXECUTABLE"
        installed_sha256="$RESOLVED_MUD_SHA256"
    fi

    if [[ "$ACTIVE_MUD_PID" =~ ^[1-9][0-9]*$ ]]; then
        active_executable="$LAST_MUD_EXECUTABLE"
        active_commit="$LAST_MUD_GIT_COMMIT"
        active_dirty="$LAST_MUD_GIT_DIRTY"
        active_build_id="$LAST_MUD_BUILD_ID"
        active_sha256="$LAST_MUD_SHA256"
        if [[ "$active_executable" == "$installed_executable" ]] &&
           [[ "$active_build_id" == "$installed_build_id" ]]; then
            active_identity_match="yes"
        else
            active_identity_match="restart-required"
        fi
    fi

    state_tmp=$(mktemp "${state_file}.tmp.XXXXXX") || {
        log_error "Unable to create temporary autorun state file"
        return 1
    }

    if ! cat > "$state_tmp" <<EOF
PID=$AUTORUN_PID
START_TIME=$AUTORUN_START_TIME
LAST_UPDATE=$(date +%s)
STATUS=RUNNING
CRASH_COUNT=$CRASH_COUNT
MUD_PORT=$MUD_PORT
MUD_PID=$ACTIVE_MUD_PID
MUD_EXECUTABLE=$active_executable
MUD_GIT_COMMIT=$active_commit
MUD_GIT_DIRTY=$active_dirty
MUD_ELF_BUILD_ID=$active_build_id
MUD_SHA256=$active_sha256
INSTALLED_EXECUTABLE=$installed_executable
INSTALLED_GIT_COMMIT=$installed_commit
INSTALLED_GIT_DIRTY=$installed_dirty
INSTALLED_ELF_BUILD_ID=$installed_build_id
INSTALLED_SHA256=$installed_sha256
MUD_IDENTITY_MATCH=$active_identity_match
EOF
    then
        log_error "Unable to write autorun state"
        rm -f "$state_tmp"
        return 1
    fi

    if ! mv -f -- "$state_tmp" "$state_file"; then
        log_error "Unable to publish autorun state"
        rm -f "$state_tmp"
        return 1
    fi
}
write_autorun_state

# Only start watchdog after publishing the initial state. This prevents the
# watchdog from treating a supervisor that is still starting as missing.
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
log_info "  STATE_UPDATE_INTERVAL=$STATE_UPDATE_INTERVAL"
log_info "  Signal handlers: ACTIVE"
log_info "  EXIT trap: DISABLED (safe mode)"
log_info "  Watchdog: ENABLED (if available)"

# Clean up function (only called when explicitly shutting down via .killscript)
cleanup() {
    log_info "Performing cleanup..."
    stop_auxiliary_services
    rm -f "$MUD_PID_FILE" "$AUTORUN_PID_FILE" 2>/dev/null || true
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

    write_autorun_state

    if [[ -r .killscript ]]; then
        log_info "Killscript detected at top of main loop"
        cleanup
        proc_syslog
        exit 0
    fi

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

    # Calculate MUD uptime
    mud_end_time=$(date +%s)
    mud_uptime=$((mud_end_time - mud_start_time))
    mud_uptime_hours=$((mud_uptime / 3600))

    # Log detailed exit information
    echo "MUD EXIT: code=$mud_exit_code time=$(date) uptime=${mud_uptime}s" >> "${LOG_DIR}/mud_exits.log"

    # Log exit status for monitoring
    log_info "MUD ran for $mud_uptime seconds ($mud_uptime_hours hours)"

    mud_error_type=""
    mud_error_message=""
    if [[ $mud_exit_code -eq 0 ]]; then
        log_info "MUD exited cleanly"
    elif [[ $mud_exit_code -eq 139 ]]; then
        mud_error_type="SegmentationFault"
        mud_error_message="MUD crashed with segmentation fault (SIGSEGV)"
        log_error "$mud_error_message"
        CRASH_COUNT=$((CRASH_COUNT + 1))
    elif [[ $mud_exit_code -eq 134 ]]; then
        mud_error_type="AbortSignal"
        mud_error_message="MUD aborted (SIGABRT)"
        log_error "$mud_error_message"
        CRASH_COUNT=$((CRASH_COUNT + 1))
    else
        mud_error_type="MudProcessExit"
        mud_error_message="MUD exited with unexpected code: $mud_exit_code"
        log_error "$mud_error_message"
        CRASH_COUNT=$((CRASH_COUNT + 1))
    fi

    # Log crash count but NEVER stop restarting
    if [[ $CRASH_COUNT -ge $MAX_CRASHES_PER_HOUR ]]; then
        log_warn "MUD has crashed $CRASH_COUNT times in the last hour - continuing anyway!"
        # Optional: Send alert (uncomment and configure as needed)
        # echo "MUD crash loop detected on $(hostname)" | mail -s "MUD CRITICAL" admin@example.com
    fi

    # Archive any core dump
    archive_core_dump "$mud_exit_code" "$mud_start_time"

    if [[ $mud_exit_code -ne 0 ]]; then
        write_last_error "$mud_error_type" "$mud_error_message" \
            "$mud_exit_code" "$mud_uptime" "$CRASH_COUNT" || true
    fi

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
