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
    *)
      fail "usage: $0 [--commands <game-command> ... | --dialog <input-line> ... | --copyover-check [<pre-copyover-command> ... --] <post-copyover-command> ... | --help-check <keyword> ... | --vessel-help-check]"
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
  else
    [[ $# -gt 0 ]] || fail "$mode mode requires at least one input line"
  fi

  for game_command in "$@"; do
    [[ -n "$game_command" && "$game_command" != *$'\n'* && "$game_command" != *$'\r'* ]] ||
      fail "game commands must be nonempty, single-line arguments"
  done
fi

for command_name in expect nc ss awk grep systemctl systemd-run; do
  need_command "$command_name"
done

[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"
[[ -x "$repo_root/bin/circle" ]] || fail "bin/circle is missing; build and install first"

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

  for ((attempt = 0; attempt < 600; attempt++)); do
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
proc run_game_command {command} {
  global command_index smoke_character

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

  set cleaned [clean_command_output $output $command $marker]
  puts "\n>>> $command"
  if {$cleaned ne ""} {
    puts $cleaned
  } else {
    puts "(no game output)"
  }
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

set timeout 45
match_max 200000
log_user 0
set mode [lindex $argv 0]
set game_commands [lrange $argv 1 end]

if {![info exists env(MUD_SMOKE_ACCOUNT)] ||
    ![info exists env(MUD_SMOKE_ACCOUNT_PASSWORD)] ||
    ![info exists env(MUD_SMOKE_CHARACTER)]} {
  fail "credential environment is unavailable to expect"
}
set smoke_character $env(MUD_SMOKE_CHARACTER)

spawn -noecho nc 127.0.0.1 $env(MUD_SMOKE_PORT)

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
    $mode eq "help-check"} {
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
    foreach help_keyword $game_commands {
      run_help_check $help_keyword
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
else
  printf 'PASS: %s entered the world, left the character, and logged out of the account (%ss).\n' \
    "$smoke_character" "$elapsed_seconds"
fi
