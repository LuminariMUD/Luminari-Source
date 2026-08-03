#!/usr/bin/env bash

set -euo pipefail

fail()
{
  printf 'process memory detail sampler: %s\n' "$*" >&2
  exit 1
}

usage()
{
  printf '%s\n' \
    "Usage:" \
    "  ./scripts/process-memory/sample_process_memory_details.sh --header" \
    "  ./scripts/process-memory/sample_process_memory_details.sh --sample <pid> <label>" \
    "  ./scripts/process-memory/sample_process_memory_details.sh --validate <samples.tsv>" >&2
  exit 1
}

print_header()
{
  printf 'epoch\tlabel\tpid\tvm_size_kib\tvm_rss_kib\trss_anon_kib\t'
  printf 'rss_file_kib\trss_shmem_kib\tvm_data_kib\tvm_swap_kib\t'
  printf 'heap_size_kib\theap_rss_kib\theap_private_dirty_kib\n'
}

parse_snapshot()
{
  local status_file=$1
  local smaps_file=$2
  local epoch=$3
  local pid=$4
  local label=$5

  [[ -r "$status_file" ]] || fail "status file is not readable: $status_file"
  [[ -r "$smaps_file" ]] || fail "smaps file is not readable: $smaps_file"
  [[ "$epoch" =~ ^[1-9][0-9]*$ ]] || fail "epoch must be a positive integer"
  [[ "$pid" =~ ^[1-9][0-9]*$ ]] || fail "PID must be a positive integer"
  [[ "$label" =~ ^[A-Za-z0-9._-]+$ ]] ||
    fail "label must contain only letters, digits, dot, underscore, or hyphen"

  awk -v status_file="$status_file" -v smaps_file="$smaps_file" \
      -v epoch="$epoch" -v pid="$pid" -v label="$label" '
    function reject(message) {
      print "process memory detail sampler: " message > "/dev/stderr"
      invalid = 1
      exit 2
    }
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
      if (invalid) exit 2
      if (vm_size == "" || vm_rss == "" || rss_anon == "" ||
          rss_file == "" || rss_shmem == "" || vm_data == "" ||
          vm_swap == "") {
        reject("status snapshot is missing a required metric")
      }
      if (!heap_seen || heap_size == "" || heap_rss == "" ||
          heap_dirty == "") {
        reject("smaps snapshot is missing the heap metrics")
      }
      if (heap_rss > heap_size || heap_dirty > heap_rss ||
          vm_rss > vm_size) {
        reject("snapshot metric relationships are invalid")
      }
      printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
             epoch, label, pid, vm_size, vm_rss, rss_anon, rss_file,
             rss_shmem, vm_data, vm_swap, heap_size, heap_rss, heap_dirty
    }
  ' "$status_file" "$smaps_file"
}

validate_series()
{
  local input_file=$1

  [[ -r "$input_file" ]] || fail "sample file is not readable: $input_file"
  awk -F '\t' '
    function reject(message) {
      print "process memory detail sampler: invalid series: " message > "/dev/stderr"
      invalid = 1
      exit 2
    }
    function is_uint(value) {
      return value ~ /^[0-9]+$/
    }
    NR == 1 {
      expected[1] = "epoch"
      expected[2] = "label"
      expected[3] = "pid"
      expected[4] = "vm_size_kib"
      expected[5] = "vm_rss_kib"
      expected[6] = "rss_anon_kib"
      expected[7] = "rss_file_kib"
      expected[8] = "rss_shmem_kib"
      expected[9] = "vm_data_kib"
      expected[10] = "vm_swap_kib"
      expected[11] = "heap_size_kib"
      expected[12] = "heap_rss_kib"
      expected[13] = "heap_private_dirty_kib"
      if (NF != 13) reject("header field count")
      for (field = 1; field <= 13; field++) {
        if ($field != expected[field]) reject("header field " field)
      }
      next
    }
    {
      if (NF != 13 || !is_uint($1) || $1 == 0 ||
          $2 !~ /^[A-Za-z0-9._-]+$/ || !is_uint($3) || $3 == 0) {
        reject("sample identity")
      }
      for (field = 4; field <= 13; field++) {
        if (!is_uint($field)) reject("nonnumeric metric")
      }
      if ($5 > $4 || $12 > $11 || $13 > $12) {
        reject("metric relationships")
      }
      sample_count++
      if (sample_count == 1) {
        series_pid = $3
      } else {
        if ($1 <= previous_epoch) reject("epochs are not strictly increasing")
        if ($3 != series_pid) reject("PID changed within the series")
      }
      previous_epoch = $1
    }
    END {
      if (invalid) exit 2
      if (NR < 2 || sample_count == 0) {
        print "process memory detail sampler: invalid series: no samples" > "/dev/stderr"
        exit 2
      }
    }
  ' "$input_file"
}

case "${1:-}" in
  --header)
    (($# == 1)) || usage
    print_header
    ;;
  --sample)
    (($# == 3)) || usage
    parse_snapshot "/proc/$2/status" "/proc/$2/smaps" "$(date +%s)" "$2" "$3"
    ;;
  --validate)
    (($# == 2)) || usage
    validate_series "$2"
    ;;
  __parse)
    (($# == 6)) || usage
    parse_snapshot "$2" "$3" "$4" "$5" "$6"
    ;;
  *)
    usage
    ;;
esac
