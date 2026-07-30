#!/usr/bin/env bash

set -euo pipefail

fail()
{
  printf 'vessel memory analyzer: %s\n' "$*" >&2
  exit 1
}

usage()
{
  cat >&2 <<'USAGE'
Usage:
  ./scripts/analyze_vessel_memory_samples.sh [options] <process-samples.tsv>

Options:
  --warmup-seconds <seconds>  Exclude this many seconds from the start.
  --block-seconds <seconds>   Summarize consecutive blocks (default: 3600).
  --windows <seconds,...>     Report trailing regression windows.
  --format <text|kv>          Human-readable or machine-readable output.
  --help                      Show this help.

The input may be the legacy headerless ferry series or the headered scale
series. Both contain epoch, PID, RSS KiB, VSZ KiB, threads, and file
descriptors in that order. This command reports trends; it does not apply a
bounded-growth pass/fail threshold.
USAGE
}

require_value()
{
  local option=$1
  local remaining=$2

  ((remaining >= 2)) || fail "$option requires a value"
}

warmup_seconds=0
block_seconds=3600
windows_csv="1800,3600,7200,21600,43200,86400"
output_format=text
input_file=""

while (($# > 0)); do
  case "$1" in
    --warmup-seconds)
      require_value "$1" "$#"
      warmup_seconds=$2
      shift 2
      ;;
    --block-seconds)
      require_value "$1" "$#"
      block_seconds=$2
      shift 2
      ;;
    --windows)
      require_value "$1" "$#"
      windows_csv=$2
      shift 2
      ;;
    --format)
      require_value "$1" "$#"
      output_format=$2
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    --*)
      fail "unknown option: $1"
      ;;
    *)
      [[ -z "$input_file" ]] || fail "only one input file may be analyzed"
      input_file=$1
      shift
      ;;
  esac
done

[[ -n "$input_file" ]] || {
  usage
  exit 1
}
[[ -r "$input_file" ]] || fail "input is not readable: $input_file"
[[ "$warmup_seconds" =~ ^[0-9]+$ ]] ||
  fail "--warmup-seconds must be a non-negative integer"
[[ "$block_seconds" =~ ^[1-9][0-9]*$ ]] ||
  fail "--block-seconds must be a positive integer"
[[ "$windows_csv" =~ ^[1-9][0-9]*(,[1-9][0-9]*)*$ ]] ||
  fail "--windows must be a comma-separated list of positive integers"
[[ "$output_format" == text || "$output_format" == kv ]] ||
  fail "--format must be text or kv"

LC_ALL=C awk \
  -v source_file="$input_file" \
  -v warmup_seconds="$warmup_seconds" \
  -v block_seconds="$block_seconds" \
  -v windows_csv="$windows_csv" \
  -v output_format="$output_format" '
function input_fail(message)
{
  printf "vessel memory analyzer: %s at input line %d\n", message, FNR \
    > "/dev/stderr"
  input_failed = 1
  exit 2
}

function is_unsigned_integer(value)
{
  return value ~ /^[0-9]+$/
}

function calculate_range(first_index, last_index,
                         sample_index, count, x, denominator,
                         sum_x, sum_x_squared,
                         sum_rss, sum_vsz, sum_x_rss, sum_x_vsz)
{
  calc_start_epoch = epochs[first_index]
  calc_end_epoch = epochs[last_index]
  calc_duration_seconds = calc_end_epoch - calc_start_epoch
  calc_sample_count = last_index - first_index + 1
  calc_rss_initial = rss_values[first_index]
  calc_rss_final = rss_values[last_index]
  calc_rss_minimum = rss_values[first_index]
  calc_rss_maximum = rss_values[first_index]
  calc_vsz_initial = vsz_values[first_index]
  calc_vsz_final = vsz_values[last_index]
  calc_vsz_minimum = vsz_values[first_index]
  calc_vsz_maximum = vsz_values[first_index]
  calc_threads_minimum = thread_values[first_index]
  calc_threads_maximum = thread_values[first_index]
  calc_descriptors_minimum = descriptor_values[first_index]
  calc_descriptors_maximum = descriptor_values[first_index]
  sum_x = 0
  sum_x_squared = 0
  sum_rss = 0
  sum_vsz = 0
  sum_x_rss = 0
  sum_x_vsz = 0

  for (sample_index = first_index; sample_index <= last_index; sample_index++) {
    x = (epochs[sample_index] - calc_start_epoch) / 3600.0
    sum_x += x
    sum_x_squared += x * x
    sum_rss += rss_values[sample_index]
    sum_vsz += vsz_values[sample_index]
    sum_x_rss += x * rss_values[sample_index]
    sum_x_vsz += x * vsz_values[sample_index]
    if (rss_values[sample_index] < calc_rss_minimum) {
      calc_rss_minimum = rss_values[sample_index]
    }
    if (rss_values[sample_index] > calc_rss_maximum) {
      calc_rss_maximum = rss_values[sample_index]
    }
    if (vsz_values[sample_index] < calc_vsz_minimum) {
      calc_vsz_minimum = vsz_values[sample_index]
    }
    if (vsz_values[sample_index] > calc_vsz_maximum) {
      calc_vsz_maximum = vsz_values[sample_index]
    }
    if (thread_values[sample_index] < calc_threads_minimum) {
      calc_threads_minimum = thread_values[sample_index]
    }
    if (thread_values[sample_index] > calc_threads_maximum) {
      calc_threads_maximum = thread_values[sample_index]
    }
    if (descriptor_values[sample_index] < calc_descriptors_minimum) {
      calc_descriptors_minimum = descriptor_values[sample_index]
    }
    if (descriptor_values[sample_index] > calc_descriptors_maximum) {
      calc_descriptors_maximum = descriptor_values[sample_index]
    }
  }

  count = calc_sample_count
  calc_rss_mean = sum_rss / count
  calc_vsz_mean = sum_vsz / count
  calc_rss_delta = calc_rss_final - calc_rss_initial
  calc_vsz_delta = calc_vsz_final - calc_vsz_initial
  denominator = count * sum_x_squared - sum_x * sum_x
  calc_slope_available = (count >= 2 && denominator > 0)
  if (calc_slope_available) {
    calc_rss_slope = \
      (count * sum_x_rss - sum_x * sum_rss) / denominator
    calc_vsz_slope = \
      (count * sum_x_vsz - sum_x * sum_vsz) / denominator
    calc_rss_slope_percent = calc_rss_mean == 0 ? 0 :
      (calc_rss_slope / calc_rss_mean) * 100.0
    calc_vsz_slope_percent = calc_vsz_mean == 0 ? 0 :
      (calc_vsz_slope / calc_vsz_mean) * 100.0
  }
}

function emit_kv_range(prefix)
{
  printf "%s_start_epoch=%.0f\n", prefix, calc_start_epoch
  printf "%s_end_epoch=%.0f\n", prefix, calc_end_epoch
  printf "%s_duration_seconds=%.0f\n", prefix, calc_duration_seconds
  printf "%s_samples=%d\n", prefix, calc_sample_count
  printf "%s_rss_initial_kib=%.0f\n", prefix, calc_rss_initial
  printf "%s_rss_mean_kib=%.6f\n", prefix, calc_rss_mean
  printf "%s_rss_minimum_kib=%.0f\n", prefix, calc_rss_minimum
  printf "%s_rss_maximum_kib=%.0f\n", prefix, calc_rss_maximum
  printf "%s_rss_final_kib=%.0f\n", prefix, calc_rss_final
  printf "%s_rss_delta_kib=%.0f\n", prefix, calc_rss_delta
  printf "%s_vsz_initial_kib=%.0f\n", prefix, calc_vsz_initial
  printf "%s_vsz_mean_kib=%.6f\n", prefix, calc_vsz_mean
  printf "%s_vsz_minimum_kib=%.0f\n", prefix, calc_vsz_minimum
  printf "%s_vsz_maximum_kib=%.0f\n", prefix, calc_vsz_maximum
  printf "%s_vsz_final_kib=%.0f\n", prefix, calc_vsz_final
  printf "%s_vsz_delta_kib=%.0f\n", prefix, calc_vsz_delta
  printf "%s_threads_minimum=%.0f\n", prefix, calc_threads_minimum
  printf "%s_threads_maximum=%.0f\n", prefix, calc_threads_maximum
  printf "%s_file_descriptors_minimum=%.0f\n", prefix, \
    calc_descriptors_minimum
  printf "%s_file_descriptors_maximum=%.0f\n", prefix, \
    calc_descriptors_maximum
  if (calc_slope_available) {
    printf "%s_rss_slope_kib_per_hour=%.6f\n", prefix, calc_rss_slope
    printf "%s_rss_slope_percent_per_hour=%.9f\n", prefix, \
      calc_rss_slope_percent
    printf "%s_vsz_slope_kib_per_hour=%.6f\n", prefix, calc_vsz_slope
    printf "%s_vsz_slope_percent_per_hour=%.9f\n", prefix, \
      calc_vsz_slope_percent
  } else {
    printf "%s_rss_slope_kib_per_hour=unavailable\n", prefix
    printf "%s_rss_slope_percent_per_hour=unavailable\n", prefix
    printf "%s_vsz_slope_kib_per_hour=unavailable\n", prefix
    printf "%s_vsz_slope_percent_per_hour=unavailable\n", prefix
  }
}

function emit_text_range(label)
{
  printf "%s: %d samples, %.0f seconds, epochs %.0f..%.0f\n", \
    label, calc_sample_count, calc_duration_seconds, calc_start_epoch, \
    calc_end_epoch
  printf "  RSS initial/mean/min/max/final/delta KiB: " \
    "%.0f/%.3f/%.0f/%.0f/%.0f/%+.0f\n", \
    calc_rss_initial, calc_rss_mean, calc_rss_minimum, calc_rss_maximum, \
    calc_rss_final, calc_rss_delta
  printf "  VSZ initial/mean/min/max/final/delta KiB: " \
    "%.0f/%.3f/%.0f/%.0f/%.0f/%+.0f\n", \
    calc_vsz_initial, calc_vsz_mean, calc_vsz_minimum, calc_vsz_maximum, \
    calc_vsz_final, calc_vsz_delta
  printf "  Threads min/max: %.0f/%.0f; file descriptors min/max: %.0f/%.0f\n", \
    calc_threads_minimum, calc_threads_maximum, \
    calc_descriptors_minimum, calc_descriptors_maximum
  if (calc_slope_available) {
    printf "  Linear RSS slope: %+.3f KiB/hour (%+.6f%%/hour)\n", \
      calc_rss_slope, calc_rss_slope_percent
    printf "  Linear VSZ slope: %+.3f KiB/hour (%+.6f%%/hour)\n", \
      calc_vsz_slope, calc_vsz_slope_percent
  } else {
    print "  Linear slopes: unavailable (fewer than two distinct samples)"
  }
}

BEGIN {
  input_header = "absent"
  window_count = split(windows_csv, requested_windows, ",")
}

{
  sub(/\r$/, "")
  if ($0 ~ /^[[:space:]]*$/) {
    next
  }

  if (!saw_nonblank_line) {
    saw_nonblank_line = 1
    if ($1 == "epoch") {
      if (NF != 6 ||
          $2 != "pid" ||
          $3 != "rss_kib" ||
          $4 != "vsz_kib" ||
          $5 != "threads" ||
          $6 != "file_descriptors") {
        input_fail("unrecognized process-series header")
      }
      input_header = "present"
      next
    }
  }

  if (NF != 6) {
    input_fail("expected exactly six fields")
  }
  if (!is_unsigned_integer($1) || $1 <= 0) {
    input_fail("epoch must be a positive integer")
  }
  if (!is_unsigned_integer($2) || $2 <= 0) {
    input_fail("PID must be a positive integer")
  }
  if (!is_unsigned_integer($3) || $3 <= 0) {
    input_fail("RSS must be a positive integer")
  }
  if (!is_unsigned_integer($4) || $4 <= 0) {
    input_fail("VSZ must be a positive integer")
  }
  if (!is_unsigned_integer($5) || $5 <= 0) {
    input_fail("thread count must be a positive integer")
  }
  if (!is_unsigned_integer($6)) {
    input_fail("file-descriptor count must be a non-negative integer")
  }

  sample_count++
  epochs[sample_count] = $1 + 0
  pids[sample_count] = $2 + 0
  rss_values[sample_count] = $3 + 0
  vsz_values[sample_count] = $4 + 0
  thread_values[sample_count] = $5 + 0
  descriptor_values[sample_count] = $6 + 0
  if (sample_count == 1) {
    constant_pid = pids[sample_count]
  } else {
    if (epochs[sample_count] <= epochs[sample_count - 1]) {
      input_fail("epochs must be strictly increasing")
    }
    if (pids[sample_count] != constant_pid) {
      input_fail("PID changed within the series")
    }
  }
}

END {
  if (input_failed) {
    exit 2
  }
  if (sample_count < 2) {
    print "vessel memory analyzer: at least two data samples are required" \
      > "/dev/stderr"
    exit 2
  }

  analysis_cutoff_epoch = epochs[1] + warmup_seconds
  analysis_first_index = 0
  for (sample_index = 1; sample_index <= sample_count; sample_index++) {
    if (epochs[sample_index] >= analysis_cutoff_epoch) {
      analysis_first_index = sample_index
      break
    }
  }
  if (analysis_first_index == 0 ||
      sample_count - analysis_first_index + 1 < 2) {
    print "vessel memory analyzer: warmup leaves fewer than two samples" \
      > "/dev/stderr"
    exit 2
  }

  if (output_format == "kv") {
    print "format_version=1"
    print "result=REPORT_ONLY"
    printf "source_file=%s\n", source_file
    printf "input_header=%s\n", input_header
    printf "pid=%.0f\n", constant_pid
    printf "warmup_seconds=%.0f\n", warmup_seconds
    printf "block_seconds=%.0f\n", block_seconds
    printf "requested_windows_seconds=%s\n", windows_csv
    calculate_range(1, sample_count)
    emit_kv_range("all")
    calculate_range(analysis_first_index, sample_count)
    emit_kv_range("analysis")
  } else {
    print "Vessel process-memory analysis"
    print "Result: REPORT_ONLY (no bounded-growth threshold is applied)"
    printf "Source: %s\n", source_file
    printf "Input header: %s; PID: %.0f\n", input_header, constant_pid
    printf "Warmup excluded: %.0f seconds\n\n", warmup_seconds
    calculate_range(1, sample_count)
    emit_text_range("All samples")
    print ""
    calculate_range(analysis_first_index, sample_count)
    emit_text_range("Post-warmup samples")
  }

  block_count = 0
  block_first_index = analysis_first_index
  block_number = 0
  for (sample_index = analysis_first_index;
       sample_index <= sample_count;
       sample_index++) {
    current_block = int((epochs[sample_index] - epochs[analysis_first_index]) / block_seconds)
    if (current_block != block_number) {
      block_last_index = sample_index - 1
      block_first[block_count] = block_first_index
      block_last[block_count] = block_last_index
      block_label[block_count] = block_number
      block_count++
      block_first_index = sample_index
      block_number = current_block
    }
  }
  block_first[block_count] = block_first_index
  block_last[block_count] = sample_count
  block_label[block_count] = block_number
  block_count++

  if (output_format == "kv") {
    printf "block_count=%d\n", block_count
  } else {
    printf "\nConsecutive %.0f-second blocks anchored at epoch %.0f:\n", \
      block_seconds, epochs[analysis_first_index]
  }
  for (block_index = 0; block_index < block_count; block_index++) {
    calculate_range(block_first[block_index], block_last[block_index])
    if (output_format == "kv") {
      printf "block_%d_index=%d\n", block_index, block_label[block_index]
      emit_kv_range("block_" block_index)
    } else {
      emit_text_range("Block " block_label[block_index])
    }
  }

  if (output_format == "text") {
    print "\nTrailing regression windows:"
  }
  analysis_duration = epochs[sample_count] - epochs[analysis_first_index]
  for (window_index = 1; window_index <= window_count; window_index++) {
    requested_window = requested_windows[window_index] + 0
    window_prefix = "window_" requested_window
    if (analysis_duration < requested_window) {
      if (output_format == "kv") {
        printf "%s_status=insufficient_duration\n", window_prefix
      } else {
        printf "  %.0f seconds: unavailable (post-warmup duration is %.0f)\n", \
          requested_window, analysis_duration
      }
      continue
    }

    window_cutoff = epochs[sample_count] - requested_window
    window_first_index = analysis_first_index
    for (sample_index = analysis_first_index;
         sample_index <= sample_count;
         sample_index++) {
      if (epochs[sample_index] >= window_cutoff) {
        window_first_index = sample_index
        break
      }
    }
    if (sample_count - window_first_index + 1 < 2) {
      if (output_format == "kv") {
        printf "%s_status=insufficient_samples\n", window_prefix
      } else {
        printf "  %.0f seconds: unavailable (fewer than two samples)\n", \
          requested_window
      }
      continue
    }

    calculate_range(window_first_index, sample_count)
    if (output_format == "kv") {
      printf "%s_status=available\n", window_prefix
      printf "%s_requested_seconds=%.0f\n", window_prefix, requested_window
      emit_kv_range(window_prefix)
    } else {
      emit_text_range(sprintf("Trailing %.0f seconds", requested_window))
    }
  }
}
' "$input_file"
