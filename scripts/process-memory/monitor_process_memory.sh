#!/usr/bin/env bash
#
# LuminariMUD Process Memory Monitor & Telemetry Collector
#
# Monitors memory growth (RSS, Anonymous RSS, Heap, and Data segment)
# over long time horizons to detect memory leaks, retention, and growth spikes.
# Supports background daemon supervision, copyover tracking, real-time alerts,
# single-shot sampling, and time-series diagnostic report generation.
#

set -euo pipefail

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

SAMPLER_SCRIPT="${SCRIPT_DIR}/sample_process_memory_details.sh"
PID_FILE="${PROJECT_ROOT}/.memory-monitor.pid"
MUD_PID_FILE="${PROJECT_ROOT}/.mud.pid"
LOG_FILE="${PROJECT_ROOT}/log/memory-monitor.log"
DEFAULT_OUTPUT="${PROJECT_ROOT}/log/process-memory-timeseries.tsv"
DEFAULT_INTERVAL=30
DEFAULT_ALERT_THRESHOLD=1024  # KiB/min

fail()
{
  printf 'memory monitor: %s\n' "$*" >&2
  exit 1
}

log_msg()
{
  local level="$1"
  shift
  local timestamp
  timestamp="$(date '+%Y-%m-%d %H:%M:%S')"
  mkdir -p "$(dirname "$LOG_FILE")"
  printf '[%s] memory-monitor [%s]: %s\n' "$timestamp" "$level" "$*" | tee -a "$LOG_FILE"
}

usage()
{
  cat <<'EOF' >&2
Usage:
  ./scripts/process-memory/monitor_process_memory.sh start [options]
  ./scripts/process-memory/monitor_process_memory.sh stop
  ./scripts/process-memory/monitor_process_memory.sh status
  ./scripts/process-memory/monitor_process_memory.sh sample [--pid <pid>] [--label <label>]
  ./scripts/process-memory/monitor_process_memory.sh daemon [options]
  ./scripts/process-memory/monitor_process_memory.sh report [--input <file>] [--threshold <kib_min>]
  ./scripts/process-memory/monitor_process_memory.sh analyze <file.tsv>

Options:
  --interval <seconds>         Sampling interval (default: 30s)
  --output <file>              Output TSV file (default: log/process-memory-timeseries.tsv)
  --pid <pid>                  Pin target PID (default: follow .mud.pid across copyovers)
  --alert-threshold <kib_min>  Growth rate alert threshold in KiB/min (default: 1024)
  --label <label>              Sample label (default: live)
  --input <file>               Input TSV file for report
  -h, --help                   Show this help message
EOF
  exit 1
}

# Confirm a PID is running an executable from this checkout's bin tree. A bare
# process-name probe can select an unrelated MUD on a shared host, so ownership
# is proved from /proc rather than assumed from the executable's name.
pid_owned_by_checkout()
{
  local pid="$1"
  local process_exe=""

  process_exe="$(readlink -f -- "/proc/$pid/exe" 2>/dev/null || true)"
  [[ -n "$process_exe" ]] || return 1
  [[ "$process_exe" == "${PROJECT_ROOT}/bin/"* ]]
}

# Resolve the MUD PID for this checkout. The project PID file is the only
# discovery source; anything it cannot prove is treated as no target at all.
find_mud_pid()
{
  local explicit_pid="${1:-}"
  local candidate_pid=""

  if [[ -n "$explicit_pid" ]]; then
    if [[ -d "/proc/$explicit_pid" ]]; then
      printf '%s\n' "$explicit_pid"
      return 0
    else
      return 1
    fi
  fi

  [[ -r "$MUD_PID_FILE" ]] || return 1
  IFS= read -r candidate_pid < "$MUD_PID_FILE" || true
  [[ "$candidate_pid" =~ ^[1-9][0-9]*$ ]] || return 1
  [[ -d "/proc/$candidate_pid" ]] || return 1
  pid_owned_by_checkout "$candidate_pid" || return 1

  printf '%s\n' "$candidate_pid"
  return 0
}

read_pid_file()
{
  local pid_file="$1"
  local pid=""

  if [[ ! -r "$pid_file" ]]; then
    return 1
  fi

  IFS= read -r pid < "$pid_file" || true
  if [[ ! "$pid" =~ ^[1-9][0-9]*$ ]]; then
    return 1
  fi

  if kill -0 "$pid" 2>/dev/null; then
    printf '%s\n' "$pid"
    return 0
  fi

  return 1
}

take_sample()
{
  local target_pid="$1"
  local sample_label="$2"
  local status_file="/proc/$target_pid/status"
  local smaps_file="/proc/$target_pid/smaps"
  local epoch

  epoch="$(date +%s)"

  if [[ ! -r "$status_file" || ! -r "$smaps_file" ]]; then
    return 1
  fi

  if [[ -x "$SAMPLER_SCRIPT" ]]; then
    "$SAMPLER_SCRIPT" __parse "$status_file" "$smaps_file" "$epoch" "$target_pid" "$sample_label"
  else
    # Built-in parsing fallback
    awk -v status_file="$status_file" -v smaps_file="$smaps_file" \
        -v epoch="$epoch" -v pid="$target_pid" -v label="$sample_label" '
      FILENAME == status_file {
        if ($1 == "VmSize:") vm_size = $2
        else if ($1 == "VmRSS:") vm_rss = $2
        else if ($1 == "RssAnon:") rss_anon = $2
        else if ($1 == "RssFile:") rss_file = $2
        else if ($1 == "RssShmem:") rss_shmem = $2
        else if ($1 == "VmData:") vm_data = $2
        else if ($1 == "VmSwap:") vm_swap = $2
        next
      }
      FILENAME == smaps_file {
        if ($0 ~ /^[[:xdigit:]]+-[[:xdigit:]]+[[:space:]]/) {
          in_heap = ($NF == "[heap]")
          if (in_heap) heap_seen = 1
          next
        }
        if (in_heap && $1 == "Size:") heap_size = $2
        else if (in_heap && $1 == "Rss:") heap_rss = $2
        else if (in_heap && $1 == "Private_Dirty:") heap_dirty = $2
      }
      END {
        if (vm_size == "" || vm_rss == "" || rss_anon == "" ||
            rss_file == "" || rss_shmem == "" || vm_data == "" ||
            vm_swap == "" || !heap_seen || heap_size == "" ||
            heap_rss == "" || heap_dirty == "") {
          exit 2
        }
        printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
               epoch, label, pid, vm_size, vm_rss, rss_anon, rss_file,
               rss_shmem, vm_data, vm_swap, heap_size, heap_rss, heap_dirty
      }
    ' "$status_file" "$smaps_file"
  fi
}

cmd_sample()
{
  local explicit_pid=""
  local label="live"

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --pid)
        explicit_pid="$2"
        shift 2
        ;;
      --label)
        label="$2"
        shift 2
        ;;
      *)
        usage
        ;;
    esac
  done

  local target_pid
  if ! target_pid="$(find_mud_pid "$explicit_pid")"; then
    fail "target process not found or not readable"
  fi

  take_sample "$target_pid" "$label"
}

cmd_daemon()
{
  local interval="$DEFAULT_INTERVAL"
  local output_file="$DEFAULT_OUTPUT"
  local explicit_pid=""
  local alert_threshold="$DEFAULT_ALERT_THRESHOLD"
  local label="sample"

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --interval)
        interval="$2"
        shift 2
        ;;
      --output)
        output_file="$2"
        shift 2
        ;;
      --pid)
        explicit_pid="$2"
        shift 2
        ;;
      --alert-threshold)
        alert_threshold="$2"
        shift 2
        ;;
      *)
        usage
        ;;
    esac
  done

  [[ "$interval" =~ ^[1-9][0-9]*$ ]] || fail "interval must be a positive integer"
  [[ "$alert_threshold" =~ ^[0-9]+$ ]] || fail "alert threshold must be a positive number"

  mkdir -p "$(dirname "$output_file")"

  # Print header if file is empty or does not exist
  if [[ ! -s "$output_file" ]]; then
    if [[ -x "$SAMPLER_SCRIPT" ]]; then
      "$SAMPLER_SCRIPT" --header > "$output_file"
    else
      printf 'epoch\tlabel\tpid\tvm_size_kib\tvm_rss_kib\trss_anon_kib\trss_file_kib\trss_shmem_kib\tvm_data_kib\tvm_swap_kib\theap_size_kib\theap_rss_kib\theap_private_dirty_kib\n' > "$output_file"
    fi
  fi

  log_msg "INFO" "Starting memory monitoring daemon (interval=${interval}s, alert_threshold=${alert_threshold} KiB/min, output=${output_file})"

  local current_pid=""
  local sample_count=0
  local prev_epoch=0
  local prev_anon=0
  local first_epoch=0
  local first_anon=0
  local last_alert_time=0

  while true; do
    if [[ -z "$current_pid" ]] || ! kill -0 "$current_pid" 2>/dev/null; then
      local new_pid
      if new_pid="$(find_mud_pid "$explicit_pid")"; then
        if [[ -n "$current_pid" && "$current_pid" != "$new_pid" ]]; then
          log_msg "NOTICE" "Copyover detected: monitored PID transitioned from $current_pid to $new_pid"
        else
          log_msg "INFO" "Discovered running MUD process with PID $new_pid"
        fi
        current_pid="$new_pid"
        first_epoch=0
        first_anon=0
      else
        log_msg "WARN" "MUD process not running or unreadable; waiting for process..."
        sleep "$interval"
        continue
      fi
    fi

    local row
    if row="$(take_sample "$current_pid" "$label")"; then
      printf '%s\n' "$row" >> "$output_file"
      sample_count=$((sample_count + 1))

      local epoch anon
      epoch="$(awk -F '\t' '{ print $1 }' <<< "$row")"
      anon="$(awk -F '\t' '{ print $6 }' <<< "$row")"

      if (( first_epoch == 0 )); then
        first_epoch="$epoch"
        first_anon="$anon"
      fi

      # Calculate rolling growth rate if window >= 60 seconds
      if (( epoch - first_epoch >= 60 )); then
        local elapsed_min growth_kib rate_kib_min
        elapsed_min=$(( (epoch - first_epoch) / 60 ))
        growth_kib=$(( anon - first_anon ))
        if (( elapsed_min > 0 )); then
          rate_kib_min=$(( growth_kib / elapsed_min ))
          if (( rate_kib_min >= alert_threshold )) && (( epoch - last_alert_time >= 300 )); then
            last_alert_time="$epoch"
            log_msg "ALERT" "High memory growth detected! Anon RSS increased by ${growth_kib} KiB over ${elapsed_min} min (${rate_kib_min} KiB/min). PID: ${current_pid}, Current Anon RSS: ${anon} KiB"
          fi
        fi
      fi

      prev_epoch="$epoch"
      prev_anon="$anon"
    else
      log_msg "WARN" "Failed to collect memory sample for PID $current_pid"
    fi

    sleep "$interval"
  done
}

cmd_start()
{
  local interval="$DEFAULT_INTERVAL"
  local output_file="$DEFAULT_OUTPUT"
  local explicit_pid=""
  local alert_threshold="$DEFAULT_ALERT_THRESHOLD"

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --interval)
        interval="$2"
        shift 2
        ;;
      --output)
        output_file="$2"
        shift 2
        ;;
      --pid)
        explicit_pid="$2"
        shift 2
        ;;
      --alert-threshold)
        alert_threshold="$2"
        shift 2
        ;;
      *)
        usage
        ;;
    esac
  done

  local existing_pid
  if existing_pid="$(read_pid_file "$PID_FILE")"; then
    fail "memory monitor is already running with PID $existing_pid"
  fi

  local target_pid
  if ! target_pid="$(find_mud_pid "$explicit_pid")"; then
    fail "target MUD process not found"
  fi

  mkdir -p "$(dirname "$PID_FILE")" "$(dirname "$LOG_FILE")" "$(dirname "$output_file")"

  local -a daemon_args=(
    daemon
    --interval "$interval"
    --output "$output_file"
    --alert-threshold "$alert_threshold"
  )
  if [[ -n "$explicit_pid" ]]; then
    daemon_args+=(--pid "$explicit_pid")
  fi

  # Launch daemon in background. Auto-discovery stays unpinned so the daemon
  # can find a replacement process after copyover or restart.
  nohup "$SCRIPT_PATH" "${daemon_args[@]}" >> "$LOG_FILE" 2>&1 &
  local daemon_pid=$!

  printf '%s\n' "$daemon_pid" > "$PID_FILE"
  log_msg "INFO" "Memory monitor started with daemon PID $daemon_pid targeting MUD PID $target_pid"
  printf 'Memory monitor started (Daemon PID: %s, Monitored PID: %s, Output: %s)\n' \
    "$daemon_pid" "$target_pid" "$output_file"
}

cmd_stop()
{
  local daemon_pid
  if ! daemon_pid="$(read_pid_file "$PID_FILE")"; then
    rm -f "$PID_FILE"
    printf 'Memory monitor is not running.\n'
    return 0
  fi

  log_msg "INFO" "Stopping memory monitor daemon PID $daemon_pid"
  kill "$daemon_pid" 2>/dev/null || true
  sleep 1
  if kill -0 "$daemon_pid" 2>/dev/null; then
    kill -9 "$daemon_pid" 2>/dev/null || true
  fi

  rm -f "$PID_FILE"
  printf 'Memory monitor stopped (PID %s).\n' "$daemon_pid"
}

cmd_status()
{
  local daemon_pid
  if daemon_pid="$(read_pid_file "$PID_FILE")"; then
    printf 'Status: RUNNING\n'
    printf 'Daemon PID: %s\n' "$daemon_pid"
    if [[ -f "$LOG_FILE" ]]; then
      printf 'Log file: %s\n' "$LOG_FILE"
    fi
    if [[ -f "$DEFAULT_OUTPUT" ]]; then
      local count
      count="$(wc -l < "$DEFAULT_OUTPUT" 2>/dev/null || echo 0)"
      printf 'Output file: %s (%d rows)\n' "$DEFAULT_OUTPUT" "$count"
      if (( count > 1 )); then
        printf 'Latest sample:\n'
        tail -n 1 "$DEFAULT_OUTPUT"
      fi
    fi
  else
    printf 'Status: STOPPED\n'
    if [[ -f "$DEFAULT_OUTPUT" ]]; then
      local count
      count="$(wc -l < "$DEFAULT_OUTPUT" 2>/dev/null || echo 0)"
      printf 'Previous output file exists: %s (%d rows)\n' "$DEFAULT_OUTPUT" "$count"
    fi
  fi
}

cmd_report()
{
  local input_file="$DEFAULT_OUTPUT"
  local alert_threshold="$DEFAULT_ALERT_THRESHOLD"

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --input)
        input_file="$2"
        shift 2
        ;;
      --threshold)
        alert_threshold="$2"
        shift 2
        ;;
      *)
        if [[ -f "$1" ]]; then
          input_file="$1"
          shift
        else
          usage
        fi
        ;;
    esac
  done

  [[ -r "$input_file" ]] || fail "input file is not readable: $input_file"

  awk -F '\t' -v threshold="$alert_threshold" '
    function format_kib(kib) {
      if (kib >= 1048576) return sprintf("%.2f GiB (%d KiB)", kib / 1048576, kib)
      if (kib >= 1024) return sprintf("%.2f MiB (%d KiB)", kib / 1024, kib)
      return sprintf("%d KiB", kib)
    }
    function format_duration(seconds) {
      h = int(seconds / 3600)
      m = int((seconds % 3600) / 60)
      s = seconds % 60
      return sprintf("%02dh %02dm %02ds", h, m, s)
    }
    NR == 1 { next } # skip header
    {
      epoch = $1; label = $2; pid = $3;
      vmsize = $4; rss = $5; anon = $6; file = $7; shmem = $8; data = $9; swap = $10;
      heap_size = $11; heap_rss = $12; heap_dirty = $13;

      if (count == 0) {
        start_epoch = epoch
        start_rss = rss; start_anon = anon; start_heap = heap_rss; start_vmsize = vmsize
        min_rss = rss; max_rss = rss
        min_anon = anon; max_anon = anon
        min_heap = heap_rss; max_heap = heap_rss
        first_pid = pid
      }

      if (pid != prev_pid && prev_pid != "") {
        copyovers++
      }
      prev_pid = pid

      if (rss < min_rss) min_rss = rss
      if (rss > max_rss) max_rss = rss
      if (anon < min_anon) min_anon = anon
      if (anon > max_anon) max_anon = anon
      if (heap_rss < min_heap) min_heap = heap_rss
      if (heap_rss > max_heap) max_heap = heap_rss

      # Track max single-interval growth
      if (count > 0 && epoch > prev_epoch) {
        interval_min = (epoch - prev_epoch) / 60.0
        if (interval_min > 0) {
          interval_growth = (anon - prev_anon) / interval_min
          if (interval_growth > max_interval_growth) {
            max_interval_growth = interval_growth
            max_interval_epoch = epoch
          }
        }
      }

      prev_epoch = epoch
      prev_anon = anon
      last_rss = rss
      last_anon = anon
      last_heap = heap_rss
      last_vmsize = vmsize
      last_pid = pid
      count++
    }
    END {
      if (count == 0) {
        print "memory monitor report: no samples found in input file"
        exit 1
      }
      duration_sec = prev_epoch - start_epoch
      duration_min = duration_sec / 60.0

      net_rss = last_rss - start_rss
      net_anon = last_anon - start_anon
      net_heap = last_heap - start_heap

      rss_rate = duration_min > 0 ? (net_rss / duration_min) : 0
      anon_rate = duration_min > 0 ? (net_anon / duration_min) : 0
      heap_rate = duration_min > 0 ? (net_heap / duration_min) : 0

      print "=============================================================================="
      print "                   LUMINARI MUD PROCESS MEMORY ANALYSIS"
      print "=============================================================================="
      printf "Time Horizon:\n"
      printf "  Sample Count:             %d samples\n", count
      printf "  Observation Duration:     %s (%d seconds)\n", format_duration(duration_sec), duration_sec
      printf "  Monitored PID(s):         %s (Copyover Transitions: %d)\n\n", (first_pid == last_pid ? first_pid : first_pid " -> " last_pid), copyovers

      printf "Memory Footprint Evolution:\n"
      printf "  Resident Set (VmRSS):     Initial: %-18s Current: %-18s Peak: %s\n", format_kib(start_rss), format_kib(last_rss), format_kib(max_rss)
      printf "  Anonymous RSS (RssAnon):  Initial: %-18s Current: %-18s Peak: %s\n", format_kib(start_anon), format_kib(last_anon), format_kib(max_anon)
      printf "  Heap RSS:                 Initial: %-18s Current: %-18s Peak: %s\n", format_kib(start_heap), format_kib(last_heap), format_kib(max_heap)
      printf "  Virtual Size (VmSize):    Initial: %-18s Current: %s\n\n", format_kib(start_vmsize), format_kib(last_vmsize)

      printf "Net Memory Change:\n"
      printf "  RSS Change:               %+d KiB (%+.2f MiB)\n", net_rss, net_rss / 1024.0
      printf "  Anonymous RSS Change:     %+d KiB (%+.2f MiB)\n", net_anon, net_anon / 1024.0
      printf "  Heap RSS Change:          %+d KiB (%+.2f MiB)\n\n", net_heap, net_heap / 1024.0

      printf "Sustained Growth Rates:\n"
      printf "  RSS Growth Rate:          %+.2f KiB/min (%+.2f MiB/hour)\n", rss_rate, (rss_rate * 60.0) / 1024.0
      printf "  Anonymous RSS Rate:       %+.2f KiB/min (%+.2f MiB/hour)\n", anon_rate, (anon_rate * 60.0) / 1024.0
      printf "  Heap RSS Rate:            %+.2f KiB/min (%+.2f MiB/hour)\n", heap_rate, (heap_rate * 60.0) / 1024.0
      if (max_interval_growth > 0) {
        printf "  Peak Burst Rate:          %+.2f KiB/min (at epoch %d)\n", max_interval_growth, max_interval_epoch
      }
      printf "\n"

      printf "Health Assessment:\n"
      if (duration_sec < 180) {
        printf "  Status: [COLLECTING] Window under 3 minutes; continue monitoring for trend.\n"
      } else if (anon_rate >= threshold || rss_rate >= threshold) {
        printf "  Status: [CRITICAL] High sustained memory growth (>= %d KiB/min). Memory leak likely!\n", threshold
      } else if (anon_rate >= 200 || rss_rate >= 200) {
        printf "  Status: [WARNING] Elevated growth rate (>= 200 KiB/min). Monitor closely.\n"
      } else if (anon_rate >= 20 || rss_rate >= 50) {
        printf "  Status: [MODERATE] Normal active gameplay memory growth.\n"
      } else {
        printf "  Status: [HEALTHY / STABLE] Memory plateaued or negligible growth (< 20 KiB/min).\n"
      }
      print "=============================================================================="
    }
  ' "$input_file"
}

case "${1:-}" in
  start)
    shift
    cmd_start "$@"
    ;;
  stop)
    cmd_stop
    ;;
  status)
    cmd_status
    ;;
  sample)
    shift
    cmd_sample "$@"
    ;;
  daemon)
    shift
    cmd_daemon "$@"
    ;;
  report)
    shift
    cmd_report "$@"
    ;;
  analyze)
    shift
    cmd_report "$@"
    ;;
  -h|--help)
    usage
    ;;
  *)
    usage
    ;;
esac
