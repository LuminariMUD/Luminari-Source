#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)
started_at=$SECONDS

fail()
{
  printf 'dev character login smoke: %s\n' "$*" >&2
  exit 1
}

need_command()
{
  command -v "$1" >/dev/null 2>&1 || fail "required command not found: $1"
}

port_is_listening()
{
  ss -H -ltn "sport = :$mud_port" 2>/dev/null | grep -q .
}

mode="smoke"
if [[ $# -gt 0 ]]; then
  generated_vessel_help=false
  case "$1" in
    --commands)
      mode="commands"
      ;;
    --dialog)
      mode="dialog"
      ;;
    --copyover-check)
      mode="copyover-check"
      ;;
    --help-check)
      mode="help-check"
      ;;
    --vessel-help-check)
      mode="help-check"
      generated_vessel_help=true
      ;;
    --vessel-builder-check)
      mode="vessel-builder-check"
      ;;
    --vessel-msdp-check)
      mode="vessel-msdp-check"
      ;;
    --vessel-channel-check)
      mode="vessel-channel-check"
      ;;
    --vessel-message-check)
      mode="vessel-message-check"
      ;;
    --vessel-crossing-check)
      mode="vessel-crossing-check"
      ;;
    *)
      fail "usage: $0 [--commands <game-command> ... | --dialog <input-line> ... | --copyover-check [<pre-copyover-command> ... --] <post-copyover-command> ... | --help-check <keyword> ... | --vessel-help-check | --vessel-builder-check | --vessel-msdp-check <ship-slot> | --vessel-channel-check <ship-slot> <crew-character> | --vessel-message-check <ship-slot> | --vessel-crossing-check <ship-slot>]"
      ;;
  esac
  shift

  if [[ "$generated_vessel_help" == true ]]; then
    [[ $# -eq 0 ]] || fail "--vessel-help-check does not accept additional arguments"
    mapfile -t vessel_help_keywords < <(
      {
        awk '
          /this must be last/ { exit }
          /^[[:space:]]+\{"[^"]+"/ {
            line = $0
            sub(/^[[:space:]]+\{"/, "", line)
            sub(/".*/, "", line)
            command = toupper(line)
          }
          /CMD_FEATURE_VESSEL/ { print command }
        ' "$repo_root/src/interpreter.c"

        # Recovery commands intentionally remain usable while the vessel
        # feature gate is off, so they do not carry CMD_FEATURE_VESSEL.
        printf '%s\n' \
          BOARD SHIPLIST SHIPGOTO SHIPFIX SHIPPURGE VEHICLEPURGE \
          VDEBUG VESSELDEBUG
      } | sort -u
    )
    ((${#vessel_help_keywords[@]} > 0)) ||
      fail "could not derive vessel commands from src/interpreter.c"
    set -- "${vessel_help_keywords[@]}"
  elif [[ "$mode" == "vessel-builder-check" ]]; then
    [[ $# -eq 0 ]] || fail "--vessel-builder-check does not accept additional arguments"
  elif [[ "$mode" == "vessel-msdp-check" ]]; then
    [[ $# -eq 1 && "$1" =~ ^[1-9][0-9]*$ && "$1" -le 500 ]] ||
      fail "--vessel-msdp-check requires one ship slot from 1 through 500"
  elif [[ "$mode" == "vessel-channel-check" ]]; then
    [[ $# -eq 2 && "$1" =~ ^[1-9][0-9]*$ && "$1" -le 500 ]] ||
      fail "--vessel-channel-check requires a ship slot from 1 through 500 and a crew character"
    [[ "$2" =~ ^[[:alpha:]][[:alpha:]-]{1,29}$ ]] ||
      fail "--vessel-channel-check crew character must be a valid character name"
  elif [[ "$mode" == "vessel-message-check" ]]; then
    [[ $# -eq 1 && "$1" =~ ^[1-9][0-9]*$ && "$1" -le 500 ]] ||
      fail "--vessel-message-check requires one ship slot from 1 through 500"
  elif [[ "$mode" == "vessel-crossing-check" ]]; then
    [[ $# -eq 1 && "$1" =~ ^[1-9][0-9]*$ && "$1" -le 500 ]] ||
      fail "--vessel-crossing-check requires one ship slot from 1 through 500"
  else
    [[ $# -gt 0 ]] || fail "$mode mode requires at least one input line"
  fi

  for game_command in "$@"; do
    [[ -n "$game_command" && "$game_command" != *$'\n'* && "$game_command" != *$'\r'* ]] ||
      fail "game commands must be nonempty, single-line arguments"
  done
fi

for command_name in expect nc ss awk flock grep systemctl systemd-run; do
  need_command "$command_name"
done

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -x "$repo_root/bin/circle" ]] || fail "bin/circle is missing; build and install first"

login_lock="${TMPDIR:-/tmp}/luminari-dev-character-login-${UID}.lock"
exec 9>"$login_lock"
flock -w 180 9 ||
  fail "timed out waiting for another local character session to finish"

set +x
# shellcheck disable=SC1091
. "$repo_root/lib/.env"

[[ "${APP_ENV:-}" == "development" ]] ||
  fail "refusing to run because APP_ENV is not development"

smoke_character="${DEV_MUD_CHARACTER:-Kohdee}"
smoke_account="${DEV_MUD_ACCOUNT:-${GAME_MASTER_ACCOUNT:-}}"
smoke_password="${DEV_MUD_ACCOUNT_PASSWORD:-${GAME_MASTER_ACCOUNT_PASSWORD:-}}"

[[ "$smoke_character" =~ ^[[:alpha:]][[:alpha:]-]{1,29}$ ]] ||
  fail "DEV_MUD_CHARACTER must be a valid character name"
[[ -n "$smoke_account" ]] ||
  fail "DEV_MUD_ACCOUNT or GAME_MASTER_ACCOUNT is not set"
[[ -n "$smoke_password" ]] ||
  fail "DEV_MUD_ACCOUNT_PASSWORD or GAME_MASTER_ACCOUNT_PASSWORD is not set"
if [[ "$mode" == "vessel-channel-check" &&
      "${2,,}" == "${smoke_character,,}" ]]; then
  fail "--vessel-channel-check requires two different character names"
fi
export -n GAME_MASTER_ACCOUNT GAME_MASTER_ACCOUNT_PASSWORD
export -n DEV_MUD_ACCOUNT DEV_MUD_ACCOUNT_PASSWORD DEV_MUD_CHARACTER

mud_port=$(awk -F= '
  /^[[:space:]]*DFLT_PORT[[:space:]]*=/ {
    value = $2
    gsub(/[[:space:]]/, "", value)
    print value
    exit
  }
' "$repo_root/lib/etc/config")

[[ "$mud_port" =~ ^[0-9]+$ ]] || fail "could not read DFLT_PORT from lib/etc/config"
((mud_port > 1024 && mud_port <= 65535)) || fail "invalid development port: $mud_port"

if port_is_listening; then
  listener=$(ss -H -ltnp "sport = :$mud_port" 2>/dev/null || true)
  [[ "$listener" == *circle* ]] ||
    fail "port $mud_port is occupied by something other than the MUD"
  printf 'Reusing the development MUD on port %s.\n' "$mud_port"
else
  if ! systemctl is-active --quiet mariadb 2>/dev/null &&
     ! systemctl is-active --quiet mysql 2>/dev/null; then
    fail "MariaDB/MySQL is not active"
  fi

  server_log="${TMPDIR:-/tmp}/luminari-dev-login-smoke.log"
  server_unit="luminari-dev-login-smoke.service"
  : >"$server_log"

  if systemctl --user is-active --quiet "$server_unit"; then
    fail "$server_unit is active but port $mud_port is not listening"
  fi

  systemctl --user reset-failed "$server_unit" 2>/dev/null || true
  systemd-run --user --quiet --collect \
    --unit="${server_unit%.service}" \
    --property="WorkingDirectory=$repo_root" \
    --property="StandardOutput=append:$server_log" \
    --property="StandardError=append:$server_log" \
    "$repo_root/bin/circle" -d "$repo_root/lib"

  server_ready=false

  for ((attempt = 0; attempt < 3000; attempt++)); do
    if ! systemctl --user is-active --quiet "$server_unit"; then
      tail -30 "$server_log" >&2 || true
      fail "the MUD exited during startup"
    fi

    if port_is_listening && grep -Fq "Entering game loop." "$server_log"; then
      server_ready=true
      break
    fi

    sleep 0.1
  done

  if [[ "$server_ready" != true ]]; then
    systemctl --user stop "$server_unit" 2>/dev/null || true
    tail -30 "$server_log" >&2 || true
    fail "timed out waiting for the MUD game loop"
  fi

  server_pid=$(systemctl --user show --property=MainPID --value "$server_unit")
  printf 'Started the development MUD: unit=%s pid=%s port=%s log=%s\n' \
    "$server_unit" "$server_pid" "$mud_port" "$server_log"
fi

MUD_SMOKE_ACCOUNT="$smoke_account" \
MUD_SMOKE_ACCOUNT_PASSWORD="$smoke_password" \
MUD_SMOKE_CHARACTER="$smoke_character" \
MUD_SMOKE_PORT="$mud_port" \
  expect -f /dev/stdin -- "$mode" "$@" <<'EXPECT'
proc fail {message} {
  puts stderr "dev character login smoke: $message"
  exit 1
}

proc send_msdp_report {variable} {
  set frame "[binary format H* fffa4501]REPORT[binary format H* 02]$variable[binary format H* fff0]"
  send -- $frame
}

proc extract_msdp_value {raw variable} {
  set prefix "[binary format H* fffa4501]$variable[binary format H* 02]"
  set suffix [binary format H* fff0]
  set frame_start [string first $prefix $raw]

  if {$frame_start < 0} {
    return [list 0 ""]
  }

  set value_start [expr {$frame_start + [string length $prefix]}]
  set frame_end [string first $suffix $raw $value_start]
  if {$frame_end < 0} {
    return [list 0 ""]
  }

  return [list 1 [string range $raw $value_start [expr {$frame_end - 1}]]]
}

proc require_msdp_value {raw variable expected context} {
  lassign [extract_msdp_value $raw $variable] found actual
  if {!$found} {
    fail "$context did not receive $variable"
  }
  if {$actual ne $expected} {
    fail "$context received $variable='$actual', expected '$expected'"
  }
}

proc require_msdp_number {raw variable context} {
  lassign [extract_msdp_value $raw $variable] found actual
  if {!$found} {
    fail "$context did not receive $variable"
  }
  if {![regexp {^-?[0-9]+$} $actual]} {
    fail "$context received nonnumeric $variable='$actual'"
  }
  return $actual
}

proc collect_msdp_frames {phase} {
  set marker "__MUD_SMOKE_MSDP_${phase}_DONE__"
  set output ""

  after 1500
  send -- "say $marker\r"
  expect {
    -re $marker { append output $expect_out(buffer) }
    timeout { fail "timed out while collecting $phase MSDP frames" }
    eof { fail "connection closed while collecting $phase MSDP frames" }
  }
  expect {
    -re $marker { append output $expect_out(buffer) }
    timeout { fail "timed out while completing $phase MSDP collection" }
    eof { fail "connection closed while completing $phase MSDP collection" }
  }

  return $output
}

proc clean_command_output {raw command marker} {
  global smoke_character

  regsub -all {\x1b\[[0-9;?]*[A-Za-z]} $raw {} raw
  regsub -all {\t.} $raw {} raw
  regsub -all {[^\x09\x0a\x0d\x20-\x7e]} $raw {} raw

  set cleaned_lines {}
  foreach line [split $raw "\n"] {
    set trimmed [string trim $line]
    if {$trimmed eq "" || $trimmed eq $command ||
        [string first "say $marker" $trimmed] >= 0 ||
        [string first $marker $trimmed] >= 0 ||
        [regexp {__MUD_SMOKE_COMMAND_[0-9]+_DONE__} $trimmed]} {
      continue
    }
    lappend cleaned_lines [string trimright $line]
  }
  return [string trim [join $cleaned_lines "\n"]]
}

set command_index 0
set last_game_command_raw ""
proc run_game_command {command} {
  global command_index last_game_command_raw smoke_character

  if {[regexp {^@wait ([0-9]+)$} $command ignored wait_seconds]} {
    if {$wait_seconds < 1 || $wait_seconds > 60} {
      fail "@wait must be between 1 and 60 seconds"
    }
    puts "\n>>> $command"
    puts "Paused the local test session for $wait_seconds second(s)."
    set wait_deadline [expr {[clock milliseconds] + ($wait_seconds * 1000)}]
    set prior_timeout $::timeout
    set ::timeout 1
    while {[clock milliseconds] < $wait_deadline} {
      expect {
        -re {.+} {}
        timeout {}
        eof { fail "connection closed during @wait" }
      }
    }
    set ::timeout 0
    expect {
      -re {.+} { exp_continue }
      timeout {}
      eof { fail "connection closed during @wait" }
    }
    set ::timeout $prior_timeout
    return
  }

  incr command_index
  set marker "__MUD_SMOKE_COMMAND_${command_index}_DONE__"
  set output ""

  send -- "$command\r"
  after 100
  send -- "say $marker\r"

  # nc displays the server's echo of the marker command first.
  expect {
    -re $marker { append output $expect_out(buffer) }
    timeout { fail "timed out while submitting game command $command_index" }
    eof { fail "connection closed while submitting game command $command_index" }
  }

  # The second marker is delivered by the in-game say command. It proves
  # the preceding command completed on the game loop.
  expect {
    -re $marker { append output $expect_out(buffer) }
    timeout { fail "timed out while completing game command $command_index" }
    eof { fail "connection closed while completing game command $command_index" }
  }

  set last_game_command_raw $output
  set cleaned [clean_command_output $output $command $marker]
  puts "\n>>> $command"
  if {$cleaned ne ""} {
    puts $cleaned
  } else {
    puts "(no game output)"
  }

  return $cleaned
}

proc run_vessel_msdp_check {ship_slot} {
  global last_game_command_raw

  set ship_variables {
    SHIP_NAME SHIP_X SHIP_Y SHIP_Z SHIP_HEADING SHIP_SPEED
    SHIP_HULL SHIP_HULL_MAX SHIP_STATUS
  }

  set output [run_game_command "shipgoto $ship_slot"]
  if {![regexp "Aboard (.+) \\(slot $ship_slot\\)\\." $output ignored ship_name]} {
    fail "could not read vessel name after shipgoto $ship_slot"
  }

  set output [run_game_command "shipstatus"]
  if {![regexp {Coordinates: \((-?[0-9]+), (-?[0-9]+)\)} $output ignored ship_x ship_y] ||
      ![regexp {Elevation/Depth: (-?[0-9]+)} $output ignored ship_z] ||
      ![regexp {Heading: (-?[0-9]+) degrees} $output ignored ship_heading] ||
      ![regexp {Speed: (-?[0-9]+) /} $output ignored ship_speed]} {
    fail "could not read complete vessel state from shipstatus"
  }

  foreach variable $ship_variables {
    send_msdp_report $variable
  }
  set aboard_raw [collect_msdp_frames "ABOARD"]

  require_msdp_value $aboard_raw SHIP_NAME $ship_name "aboard state"
  require_msdp_value $aboard_raw SHIP_X $ship_x "aboard state"
  require_msdp_value $aboard_raw SHIP_Y $ship_y "aboard state"
  require_msdp_value $aboard_raw SHIP_Z $ship_z "aboard state"
  require_msdp_value $aboard_raw SHIP_HEADING $ship_heading "aboard state"
  require_msdp_value $aboard_raw SHIP_SPEED $ship_speed "aboard state"

  set ship_hull [require_msdp_number $aboard_raw SHIP_HULL "aboard state"]
  set ship_hull_max [require_msdp_number $aboard_raw SHIP_HULL_MAX "aboard state"]
  if {$ship_hull < 0 || $ship_hull_max <= 0 || $ship_hull > $ship_hull_max} {
    fail "aboard state received invalid hull values $ship_hull/$ship_hull_max"
  }

  lassign [extract_msdp_value $aboard_raw SHIP_STATUS] status_found ship_status
  if {!$status_found ||
      [lsearch -exact {sound battered crippled sinking} $ship_status] < 0} {
    fail "aboard state received invalid SHIP_STATUS='$ship_status'"
  }

  run_game_command "goto 1000389"
  set ashore_raw [collect_msdp_frames "ASHORE"]
  set clear_raw "$last_game_command_raw$ashore_raw"
  require_msdp_value $clear_raw SHIP_NAME "" "ashore state"
  require_msdp_value $clear_raw SHIP_X 0 "ashore state"
  require_msdp_value $clear_raw SHIP_Y 0 "ashore state"
  require_msdp_value $clear_raw SHIP_Z 0 "ashore state"
  require_msdp_value $clear_raw SHIP_HEADING 0 "ashore state"
  require_msdp_value $clear_raw SHIP_SPEED 0 "ashore state"
  require_msdp_value $clear_raw SHIP_HULL 0 "ashore state"
  require_msdp_value $clear_raw SHIP_HULL_MAX 0 "ashore state"
  require_msdp_value $clear_raw SHIP_STATUS "" "ashore state"

  puts "\nPASS: native MSDP reported all nine vessel variables aboard slot $ship_slot."
  puts "PASS: native MSDP cleared all nine vessel variables after leaving the vessel."
}

proc require_game_output {output expected context} {
  if {[string first $expected $output] < 0} {
    fail "$context did not contain '$expected'"
  }
}

proc run_vessel_builder_check {} {
  set workflow_started_at [clock milliseconds]
  set prototype_name "Builder Timing Cutter [clock seconds]"

  set output [run_game_command "vedit"]
  require_game_output $output "vedit new <class> <name>" "vedit usage"
  require_game_output $output "vedit spawn <id>" "vedit usage"

  set output [run_game_command "goto -66 92"]
  require_game_output $output "Current Location  : (-66, 92)" "builder staging teleport"

  set output [run_game_command "vedit new 1 $prototype_name"]
  if {![regexp {Created Boat prototype ([0-9]+):} $output ignored prototype_id]} {
    fail "could not read the new vessel prototype id"
  }

  set output [run_game_command "vedit set $prototype_id speed 8"]
  require_game_output $output "Prototype $prototype_id updated: speed = 8." \
    "prototype speed update"

  set output [run_game_command "vedit show $prototype_id"]
  require_game_output $output "Prototype $prototype_id: $prototype_name" "prototype detail"
  require_game_output $output "Speed : 8" "prototype detail"

  set output [run_game_command "vedit spawn $prototype_id"]
  if {![regexp {as ship ([0-9]+):} $output ignored ship_slot]} {
    fail "could not read the spawned vessel slot"
  }

  set output [run_game_command "shipgoto $ship_slot"]
  require_game_output $output "Aboard $prototype_name (slot $ship_slot)." \
    "spawned vessel teleport"

  set output [run_game_command "shipstatus"]
  if {![regexp {Coordinates: \((-?[0-9]+), (-?[0-9]+)\)} $output ignored before_x before_y]} {
    fail "could not read the spawned vessel coordinates"
  }

  set output [run_game_command "speed 2"]
  require_game_output $output "Speed set to 2." "vessel speed command"
  run_game_command "setsail west"

  set output [run_game_command "shipstatus"]
  if {![regexp {Coordinates: \((-?[0-9]+), (-?[0-9]+)\)} $output ignored after_x after_y]} {
    fail "could not read the sailed vessel coordinates"
  }
  if {$after_x != ($before_x - 1) || $after_y != $before_y} {
    fail "spawned vessel did not sail west: before=($before_x,$before_y) after=($after_x,$after_y)"
  }

  set workflow_elapsed_ms [expr {[clock milliseconds] - $workflow_started_at}]
  run_game_command "speed 0"

  set output [run_game_command "shippurge $ship_slot"]
  require_game_output $output "Purged ship $ship_slot '$prototype_name'" "vessel cleanup"

  set output [run_game_command "vedit delete $prototype_id"]
  require_game_output $output "Prototype $prototype_id deleted." "prototype cleanup"

  set output [run_game_command "vedit list"]
  if {[string first $prototype_name $output] >= 0} {
    fail "temporary builder prototype remained after cleanup"
  }

  puts "\nPASS: builder created, tuned, spawned, and sailed a vessel in [format %.1f [expr {$workflow_elapsed_ms / 1000.0}]] seconds."
  puts "PASS: temporary ship $ship_slot and prototype $prototype_id were removed."
}

proc clean_dialog_output {raw commands marker} {
  global smoke_character

  regsub -all {\x1b\[[0-9;?]*[A-Za-z]} $raw {} raw
  regsub -all {\t.} $raw {} raw
  regsub -all {[^\x09\x0a\x0d\x20-\x7e]} $raw {} raw

  set cleaned_lines {}
  foreach line [split $raw "\n"] {
    set trimmed [string trim $line]
    if {$trimmed eq "" || [lsearch -exact $commands $trimmed] >= 0 ||
        [string first "say $marker" $trimmed] >= 0 ||
        [string first $marker $trimmed] >= 0} {
      continue
    }
    lappend cleaned_lines [string trimright $line]
  }
  return [string trim [join $cleaned_lines "\n"]]
}

proc run_game_dialog {commands} {
  global smoke_character

  set marker "__MUD_SMOKE_DIALOG_DONE__"
  set output ""

  foreach input_line $commands {
    send -- "$input_line\r"
    after 150
  }

  after 250
  send -- "say $marker\r"
  expect {
    -re $marker { append output $expect_out(buffer) }
    timeout { fail "dialog did not return to normal command mode" }
    eof { fail "connection closed while submitting dialog completion marker" }
  }
  expect {
    -re $marker { append output $expect_out(buffer) }
    timeout { fail "dialog completion marker was not delivered" }
    eof { fail "connection closed while completing dialog" }
  }

  set cleaned [clean_dialog_output $output $commands $marker]
  puts "\n>>> interactive dialog"
  if {$cleaned ne ""} {
    puts $cleaned
  } else {
    puts "(no game output)"
  }
}

proc run_copyover {} {
  set output ""

  send -- "copyover\r"
  expect {
    -re {Copyover recovery complete\.} { append output $expect_out(buffer) }
    -re {COPYOVER FAILED:[^\r\n]*} {
      fail "copyover failed: [string trim $expect_out(0,string)]"
    }
    timeout { fail "timed out waiting for copyover recovery" }
    eof { fail "connection closed during copyover" }
  }

  after 500
  set prior_timeout $::timeout
  set ::timeout 0
  expect {
    -re {.+} { exp_continue }
    timeout {}
  }
  set ::timeout $prior_timeout

  regsub -all {\x1b\[[0-9;?]*[A-Za-z]} $output {} output
  regsub -all {\t.} $output {} output
  regsub -all {[^\x09\x0a\x0d\x20-\x7e]} $output {} output

  puts "\n>>> copyover"
  puts "Copyover recovery complete."
}

proc run_help_check {keyword} {
  global command_index smoke_character

  incr command_index
  set command "help $keyword"
  set marker "__MUD_SMOKE_HELP_${command_index}_DONE__"
  set output ""

  send -- "$command\r"

  # A long entry enters the MUD pager. Sending q is harmless noise after a
  # short entry and exits a paged entry immediately, so one code path handles
  # both without waiting for a pager timeout.
  after 150
  send -- "q\r"
  after 150
  send -- "say $marker\r"

  expect {
    -re $marker { append output $expect_out(buffer) }
    timeout { fail "timed out while submitting help check $command_index ($keyword)" }
    eof { fail "connection closed while submitting help check $command_index ($keyword)" }
  }
  expect {
    -re $marker { append output $expect_out(buffer) }
    timeout { fail "timed out while completing help check $command_index ($keyword)" }
    eof { fail "connection closed while completing help check $command_index ($keyword)" }
  }

  set cleaned [clean_command_output $output $command $marker]
  if {[string first "There is no help on" $cleaned] >= 0} {
    fail "no in-game help entry for $keyword"
  }
  if {![regexp -line {Help Tag[[:space:]]*:[[:space:]]*([^\r\n]+)} $cleaned ignored tag]} {
    fail "help for $keyword did not come from the authoritative database"
  }

  puts "PASS help $keyword -> [string trim $tag]"
}

proc clean_socket_output {raw} {
  regsub -all {\x1b\[[0-9;?]*[A-Za-z]} $raw {} raw
  regsub -all {\t.} $raw {} raw
  regsub -all {[^\x09\x0a\x0d\x20-\x7e]} $raw {} raw
  return $raw
}

proc observe_game_output {seconds context} {
  set deadline [expr {[clock milliseconds] + ($seconds * 1000)}]
  set output ""
  set prior_timeout $::timeout
  set ::timeout 1

  while {[clock milliseconds] < $deadline} {
    expect {
      -re {.+} { append output $expect_out(buffer) }
      timeout {}
      eof { fail "$context connection closed during message observation" }
    }
  }

  set ::timeout 0
  expect {
    -re {.+} {
      append output $expect_out(buffer)
      exp_continue
    }
    timeout {}
    eof { fail "$context connection closed after message observation" }
  }
  set ::timeout $prior_timeout

  set cleaned [string trim [clean_socket_output $output]]
  puts "\n>>> observe vessel messages ($seconds seconds)"
  if {$cleaned ne ""} {
    puts $cleaned
  } else {
    puts "(no game output)"
  }
  return $cleaned
}

proc run_vessel_message_check {ship_slot} {
  global smoke_character

  set output [run_game_command "shipgoto $ship_slot"]
  if {![regexp "Aboard (.+) \\(slot $ship_slot\\)\\." $output ignored ship_name]} {
    fail "could not read vessel name after shipgoto $ship_slot"
  }

  run_game_command "perfmon reset"
  set observed [observe_game_output 8 $smoke_character]
  require_game_output $observed "The crew RETURNS FIRE at" \
    "$smoke_character vessel-message observation"

  set output [run_game_command "perfmon csv"]
  if {![regexp -line {^# vessel_messages_throttled=([1-9][0-9]*)[[:space:]]*$} \
        $output ignored throttled_count]} {
    fail "perfmon csv did not report a nonzero vessel message-throttling count"
  }

  run_game_command "goto 1000389"
  puts "\nPASS: $smoke_character observed live reciprocal fire aboard $ship_name."
  puts "PASS: the installed production tick suppressed $throttled_count repeated vessel messages."
}

proc named_water_crossing_pattern {} {
  return [join {
    {The charts mark our crossing into}
    {(Harbor Sandbox Territorial Waters|Harbor Sandbox Free Seas)}
    {\((territorial waters|free seas)\), under}
    {(Harbor Admiralty|Free Captains' Compact) authority\.}
  } " "]
}

proc named_water_crossing_from_output {raw} {
  set crossing_pattern [named_water_crossing_pattern]
  set crossing [regexp -inline -- $crossing_pattern $raw]

  if {[llength $crossing] != 4} {
    return {}
  }
  return [lrange $crossing 1 end]
}

proc wait_for_named_water_crossing {context initial_output} {
  set crossing_pattern [named_water_crossing_pattern]
  set crossing [named_water_crossing_from_output $initial_output]
  set matched 0
  set raw ""
  set prior_timeout $::timeout

  if {[llength $crossing] == 3} {
    puts "\n>>> observe named-water crossing"
    puts [string trim [clean_socket_output $initial_output]]
    return $crossing
  }

  set ::timeout 45

  expect {
    -re $crossing_pattern {
      set matched 1
      set raw $expect_out(buffer)
    }
    timeout {}
    eof { fail "$context connection closed while waiting for a named-water crossing" }
  }
  set ::timeout $prior_timeout

  if {!$matched} {
    return {}
  }

  puts "\n>>> observe named-water crossing"
  puts [string trim [clean_socket_output $raw]]
  return [named_water_crossing_from_output $raw]
}

proc run_vessel_crossing_check {ship_slot} {
  set failure ""
  set output [run_game_command "shipgoto $ship_slot"]
  if {![regexp "Aboard (.+) \\(slot $ship_slot\\)\\." $output ignored ship_name]} {
    fail "could not read vessel name after shipgoto $ship_slot"
  }

  set output [run_game_command "autopilot on"]
  set crossing [wait_for_named_water_crossing $ship_name $output]
  if {[llength $crossing] != 3} {
    set failure "no harbor named-water crossing arrived within 45 seconds"
  } else {
    lassign $crossing region waters_type authority
    if {$region eq "Harbor Sandbox Territorial Waters"} {
      set expected_type "territorial waters"
      set expected_authority "Harbor Admiralty"
      set expected_bounty 150
    } elseif {$region eq "Harbor Sandbox Free Seas"} {
      set expected_type "free seas"
      set expected_authority "Free Captains' Compact"
      set expected_bounty 100
    } else {
      set expected_type ""
      set expected_authority ""
      set expected_bounty 0
      set failure "the harbor route announced an unexpected region"
    }

    if {$failure eq "" &&
        ($waters_type ne $expected_type || $authority ne $expected_authority)} {
      set failure "the crossing announcement mixed incompatible region law metadata"
    }
    if {$failure eq ""} {
      set output [run_game_command "seastate"]
      set expected_waters \
          "Waters    : $region ($expected_type; $expected_authority; piracy bounty $expected_bounty%)"
      if {[string first $expected_waters $output] < 0} {
        set failure "seastate did not match the announced named-water region"
      }
    }
  }

  run_game_command "goto 1000389"
  if {$failure ne ""} {
    fail $failure
  }

  puts "\nPASS: $ship_name announced a canonical boundary crossing into $region."
  puts "PASS: seastate matched its water type, authority, and piracy bounty."
}

proc open_secondary_character {character} {
  global env

  spawn -noecho nc 127.0.0.1 $env(MUD_SMOKE_PORT)
  set secondary_session $spawn_id

  expect {
    -re {What is your account name} {}
    timeout { fail "timed out waiting for the secondary account prompt" }
    eof { fail "secondary connection closed before the account prompt" }
  }

  send -- "$env(MUD_SMOKE_ACCOUNT)\r"
  expect {
    -re {Password:[[:space:]]*} {}
    -re {Did I get that right} { fail "configured account was not found for the secondary session" }
    timeout { fail "timed out waiting for the secondary password prompt" }
    eof { fail "secondary connection closed before the password prompt" }
  }

  send -- "$env(MUD_SMOKE_ACCOUNT_PASSWORD)\r"
  expect {
    -re {Your choice[[:space:]]*:} { set account_menu $expect_out(buffer) }
    -re {(Wrong|Incorrect|Invalid)[^\r\n]*password} {
      fail "account password was rejected for the secondary session"
    }
    timeout { fail "timed out waiting for the secondary account menu" }
    eof { fail "secondary connection closed before the account menu" }
  }

  set clean_menu $account_menu
  regsub -all {\x1b\[[0-9;?]*[A-Za-z]} $clean_menu {} clean_menu
  regsub -all {\t.} $clean_menu {} clean_menu
  set character_slot ""
  set character_rows 0

  foreach menu_line [split $clean_menu "\n"] {
    set columns [split $menu_line "|"]
    if {[llength $columns] >= 2 &&
        [string equal -nocase [string trim [lindex $columns 1]] $character]} {
      if {[regexp {^[^0-9]*([0-9]+)[[:space:]]*$} [lindex $columns 0] ignored slot]} {
        incr character_rows
        set character_slot $slot
      }
    }
  }

  if {$character_rows != 1 || $character_slot eq ""} {
    fail "expected exactly one account-menu Name match for secondary character $character"
  }

  send -- "$character_slot\r"
  set entered_world 0
  expect {
    -re {PRESS RETURN} {}
    -re {Reconnecting\.} { set entered_world 1 }
    -re {This character has been deleted} {
      fail "$character is soft-deleted and cannot enter the secondary session"
    }
    timeout { fail "timed out while loading secondary character $character" }
    eof { fail "secondary connection closed while loading $character" }
  }

  if {!$entered_world} {
    send -- "\r"
    expect {
      -re {Make your choice:[[:space:]]*} {}
      timeout { fail "timed out waiting for $character's character menu" }
      eof { fail "secondary connection closed before $character's character menu" }
    }

    send -- "1\r"
    expect {
      -re {Welcome to Luminari} { set entered_world 1 }
      -re {May your visit here be} { set entered_world 1 }
      timeout { fail "timed out while entering the game world as $character" }
      eof { fail "secondary connection closed while entering the world as $character" }
    }
  }

  if {!$entered_world} {
    fail "$character did not enter the game world in the secondary session"
  }

  after 250
  set prior_timeout $::timeout
  set ::timeout 0
  expect {
    -re {.+} { exp_continue }
    timeout {}
    eof { fail "secondary connection closed after entering as $character" }
  }
  set ::timeout $prior_timeout

  return $secondary_session
}

proc read_session_marker {session marker context} {
  set spawn_id $session
  set output ""

  expect {
    -re $marker { append output $expect_out(buffer) }
    timeout { fail "$context did not receive its channel marker" }
    eof { fail "$context connection closed before receiving its channel marker" }
  }

  return [clean_socket_output $output]
}

proc require_session_marker_silent {session marker context} {
  set spawn_id $session
  set prior_timeout $::timeout
  set ::timeout 2

  expect {
    -re $marker { fail "$context received an aboard-only channel marker while ashore" }
    timeout {}
    eof { fail "$context connection closed during the ashore-isolation check" }
  }

  set ::timeout $prior_timeout
}

proc logout_character_session {session character} {
  set spawn_id $session

  send -- "quit\r"
  expect {
    -re {Goodbye, friend} {}
    -re {Reason:[[:space:]]*} {
      send -- "\r"
      expect {
        -re {Goodbye, friend} {}
        timeout { fail "quit feedback completed but $character did not leave the world" }
        eof { fail "$character's connection closed during character logout" }
      }
    }
    timeout { fail "timed out waiting for $character to leave the game world" }
    eof { fail "$character's connection closed during character logout" }
  }

  expect {
    -re {Make your choice:[[:space:]]*} {}
    timeout { fail "timed out waiting for $character's post-quit menu" }
    eof { fail "$character's connection closed before the post-quit menu" }
  }

  send -- "0\r"
  expect {
    -re {Your choice[[:space:]]*:} {}
    timeout { fail "timed out returning $character to the account menu" }
    eof { fail "$character's connection closed before the account menu" }
  }

  send -- "Q\r"
  expect {
    -re {Quitting\.} {}
    timeout { fail "timed out logging out $character's account session" }
    eof { fail "$character's connection closed before account logout confirmation" }
  }

  after 250
  close
  catch wait
}

proc run_vessel_channel_check {ship_slot crew_character} {
  global smoke_character

  set primary_session $::spawn_id
  set output [run_game_command "shipgoto $ship_slot"]
  if {![regexp "Aboard (.+) \\(slot $ship_slot\\)\\." $output ignored ship_name]} {
    fail "could not read vessel name after shipgoto $ship_slot"
  }

  set secondary_session [open_secondary_character $crew_character]
  set ::spawn_id $primary_session
  run_game_command "trans $crew_character"
  set output [run_game_command "north"]
  if {[string first "Alas, you cannot go that way" $output] >= 0} {
    fail "ship slot $ship_slot needs at least two connected interior rooms"
  }

  set marker_seed [clock milliseconds]
  set primary_marker "__VESSEL_CHANNEL_PRIMARY_${marker_seed}__"
  run_game_command "shiptalk $primary_marker"
  set secondary_output \
      [read_session_marker $secondary_session $primary_marker $crew_character]
  require_game_output $secondary_output "Captain's channel - $ship_name" \
    "$crew_character cross-room channel"
  require_game_output $secondary_output "$smoke_character: $primary_marker" \
    "$crew_character cross-room channel"

  set crew_marker "__VESSEL_CHANNEL_CREW_${marker_seed}__"
  set ::spawn_id $secondary_session
  run_game_command "shiptalk $crew_marker"
  set primary_output [read_session_marker $primary_session $crew_marker $smoke_character]
  require_game_output $primary_output "Captain's channel - $ship_name" \
    "$smoke_character cross-room channel"
  require_game_output $primary_output "$crew_character: $crew_marker" \
    "$smoke_character cross-room channel"

  set ::spawn_id $primary_session
  run_game_command "goto 1000389"
  set isolation_marker "__VESSEL_CHANNEL_ISOLATION_${marker_seed}__"
  set ::spawn_id $secondary_session
  run_game_command "shiptalk $isolation_marker"
  require_session_marker_silent $primary_session $isolation_marker $smoke_character

  set ::spawn_id $primary_session
  set output [run_game_command "shiptalk ashore-check-$marker_seed"]
  require_game_output $output "You must be aboard a vessel to use the captain's channel." \
    "$smoke_character ashore refusal"
  run_game_command "trans $crew_character"

  set ::spawn_id $secondary_session
  set output [run_game_command "shiptalk crew-ashore-check-$marker_seed"]
  require_game_output $output "You must be aboard a vessel to use the captain's channel." \
    "$crew_character ashore refusal"
  logout_character_session $secondary_session $crew_character

  set ::spawn_id $primary_session
  puts "\nPASS: $smoke_character and $crew_character exchanged identified captain-channel messages across separate rooms of $ship_name."
  puts "PASS: the aboard channel stayed isolated from $smoke_character ashore, and both characters received the ashore refusal."
}

set timeout 45
match_max 200000
log_user 0
set mode [lindex $argv 0]
set game_commands [lrange $argv 1 end]

if {$mode eq "__syntax-check"} {
  exit 0
}

if {![info exists env(MUD_SMOKE_ACCOUNT)] ||
    ![info exists env(MUD_SMOKE_ACCOUNT_PASSWORD)] ||
    ![info exists env(MUD_SMOKE_CHARACTER)]} {
  fail "credential environment is unavailable to expect"
}
set smoke_character $env(MUD_SMOKE_CHARACTER)

spawn -noecho nc 127.0.0.1 $env(MUD_SMOKE_PORT)
if {$mode eq "vessel-msdp-check"} {
  fconfigure $spawn_id -translation binary -encoding binary
  send -- [binary format H* fffd45]
}

expect {
  -re {What is your account name} {}
  timeout { fail "timed out waiting for the account prompt" }
  eof { fail "connection closed before the account prompt" }
}

send -- "$env(MUD_SMOKE_ACCOUNT)\r"
expect {
  -re {Password:[[:space:]]*} {}
  -re {Did I get that right} { fail "configured account was not found" }
  timeout { fail "timed out waiting for the password prompt" }
  eof { fail "connection closed before the password prompt" }
}

send -- "$env(MUD_SMOKE_ACCOUNT_PASSWORD)\r"
expect {
  -re {Your choice[[:space:]]*:} { set account_menu $expect_out(buffer) }
  -re {(Wrong|Incorrect|Invalid)[^\r\n]*password} { fail "account password was rejected" }
  timeout { fail "timed out waiting for the account menu" }
  eof { fail "connection closed before the account menu" }
}

set clean_menu $account_menu
regsub -all {\x1b\[[0-9;?]*[A-Za-z]} $clean_menu {} clean_menu
regsub -all {\t.} $clean_menu {} clean_menu
set character_slot ""
set character_rows 0

foreach menu_line [split $clean_menu "\n"] {
  set columns [split $menu_line "|"]
  if {[llength $columns] >= 2 &&
      [string equal -nocase [string trim [lindex $columns 1]] $smoke_character]} {
    if {[regexp {^[^0-9]*([0-9]+)[[:space:]]*$} [lindex $columns 0] ignored slot]} {
      incr character_rows
      set character_slot $slot
    }
  }
}

if {$character_rows != 1 || $character_slot eq ""} {
  fail "expected exactly one account-menu Name match for $smoke_character"
}

send -- "$character_slot\r"
set entered_world 0

expect {
  -re {PRESS RETURN} {}
  -re {Reconnecting\.} { set entered_world 1 }
  -re {This character has been deleted} {
    fail "$smoke_character is soft-deleted and cannot enter the game"
  }
  timeout { fail "timed out while loading $smoke_character" }
  eof { fail "connection closed while loading $smoke_character" }
}

if {!$entered_world} {
  send -- "\r"
  expect {
    -re {Make your choice:[[:space:]]*} {}
    timeout { fail "timed out waiting for $smoke_character's character menu" }
    eof { fail "connection closed before $smoke_character's character menu" }
  }

  send -- "1\r"
  expect {
    -re {Welcome to Luminari} { set entered_world 1 }
    -re {May your visit here be} { set entered_world 1 }
    timeout { fail "timed out while entering the game world as $smoke_character" }
    eof { fail "connection closed while entering the game world as $smoke_character" }
  }
}

if {!$entered_world} {
  fail "$smoke_character did not enter the game world"
}

after 250
if {$mode eq "commands" || $mode eq "dialog" || $mode eq "copyover-check" ||
    $mode eq "help-check" || $mode eq "vessel-builder-check" ||
    $mode eq "vessel-msdp-check" || $mode eq "vessel-channel-check" ||
    $mode eq "vessel-message-check" || $mode eq "vessel-crossing-check"} {
  # Discard the welcome/room display that can arrive just after world entry.
  set prior_timeout $timeout
  set timeout 0
  expect {
    -re {.+} { exp_continue }
    timeout {}
  }
  set timeout $prior_timeout

  if {$mode eq "commands"} {
    foreach game_command $game_commands {
      run_game_command $game_command
    }
  } elseif {$mode eq "dialog"} {
    run_game_dialog $game_commands
  } elseif {$mode eq "copyover-check"} {
    set separator [lsearch -exact $game_commands "--"]
    if {$separator >= 0} {
      set pre_copyover_commands [lrange $game_commands 0 [expr {$separator - 1}]]
      set post_copyover_commands [lrange $game_commands [expr {$separator + 1}] end]
      if {[llength $post_copyover_commands] == 0} {
        fail "copyover command separator requires at least one post-copyover command"
      }
    } else {
      set pre_copyover_commands {}
      set post_copyover_commands $game_commands
    }

    foreach game_command $pre_copyover_commands {
      run_game_command $game_command
    }
    run_copyover
    foreach game_command $post_copyover_commands {
      run_game_command $game_command
    }
  } else {
    if {$mode eq "help-check"} {
      foreach help_keyword $game_commands {
        run_help_check $help_keyword
      }
    } elseif {$mode eq "vessel-builder-check"} {
      run_vessel_builder_check
    } elseif {$mode eq "vessel-channel-check"} {
      run_vessel_channel_check [lindex $game_commands 0] [lindex $game_commands 1]
    } elseif {$mode eq "vessel-message-check"} {
      run_vessel_message_check [lindex $game_commands 0]
    } elseif {$mode eq "vessel-crossing-check"} {
      run_vessel_crossing_check [lindex $game_commands 0]
    } else {
      run_vessel_msdp_check [lindex $game_commands 0]
    }
  }
}

send -- "quit\r"
expect {
  -re {Goodbye, friend} {}
  -re {Reason:[[:space:]]*} {
    send -- "\r"
    expect {
      -re {Goodbye, friend} {}
      timeout { fail "quit feedback completed but character logout did not" }
      eof { fail "connection closed during character logout" }
    }
  }
  timeout { fail "timed out waiting for $smoke_character to leave the game world" }
  eof { fail "connection closed during character logout" }
}

expect {
  -re {Make your choice:[[:space:]]*} {}
  timeout { fail "timed out waiting for the post-quit character menu" }
  eof { fail "connection closed before the post-quit character menu" }
}

send -- "0\r"
expect {
  -re {Your choice[[:space:]]*:} {}
  timeout { fail "timed out returning to the account menu" }
  eof { fail "connection closed before returning to the account menu" }
}

send -- "Q\r"
expect {
  -re {Quitting\.} {}
  timeout { fail "timed out waiting for account logout" }
  eof { fail "connection closed before account logout confirmation" }
}

after 500
close
catch wait
exit 0
EXPECT

elapsed_seconds=$((SECONDS - started_at))
if [[ "$mode" == "commands" ]]; then
  printf 'PASS: %s ran %d game command(s) and logged out cleanly (%ss).\n' \
    "$smoke_character" "$#" "$elapsed_seconds"
elif [[ "$mode" == "dialog" ]]; then
  printf 'PASS: %s completed an interactive dialog and logged out cleanly (%ss).\n' \
    "$smoke_character" "$elapsed_seconds"
elif [[ "$mode" == "copyover-check" ]]; then
  printf 'PASS: %s survived copyover, completed the post-copyover checks, and logged out cleanly (%ss).\n' \
    "$smoke_character" "$elapsed_seconds"
elif [[ "$mode" == "help-check" ]]; then
  printf 'PASS: %s verified %d authoritative help keyword(s) and logged out cleanly (%ss).\n' \
    "$smoke_character" "$#" "$elapsed_seconds"
elif [[ "$mode" == "vessel-builder-check" ]]; then
  printf 'PASS: %s completed the vessel builder check and logged out cleanly (%ss total).\n' \
    "$smoke_character" "$elapsed_seconds"
elif [[ "$mode" == "vessel-msdp-check" ]]; then
  printf 'PASS: %s completed the native MSDP vessel-state check and logged out cleanly (%ss total).\n' \
    "$smoke_character" "$elapsed_seconds"
elif [[ "$mode" == "vessel-channel-check" ]]; then
  printf 'PASS: %s completed the same-account two-character vessel-channel check and logged out cleanly (%ss total).\n' \
    "$smoke_character" "$elapsed_seconds"
elif [[ "$mode" == "vessel-message-check" ]]; then
  printf 'PASS: %s completed the live vessel-message throttling check and logged out cleanly (%ss total).\n' \
    "$smoke_character" "$elapsed_seconds"
elif [[ "$mode" == "vessel-crossing-check" ]]; then
  printf 'PASS: %s completed the named-water crossing check and logged out cleanly (%ss total).\n' \
    "$smoke_character" "$elapsed_seconds"
else
  printf 'PASS: %s entered the world, left the character, and logged out of the account (%ss).\n' \
    "$smoke_character" "$elapsed_seconds"
fi
