#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)
started_at=$SECONDS

fail()
{
  printf 'dev test-character creation: %s\n' "$*" >&2
  exit 1
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
  fail "usage: $0 <character-name> OR $0 <local-test-account> <character-name>"
fi

if [[ $# -eq 1 ]]; then
  test_account=
  test_character=$1
else
  test_account=$1
  test_character=$2
fi

[[ "$test_character" =~ ^[[:alpha:]][[:alpha:]-]{1,29}$ ]] ||
  fail "character name must contain only letters or hyphens"
[[ -r "$repo_root/lib/.env" ]] || fail "cannot read lib/.env"

for command_name in expect nc awk; do
  command -v "$command_name" >/dev/null 2>&1 ||
    fail "required command not found: $command_name"
done

set +x
# shellcheck disable=SC1091
. "$repo_root/lib/.env"

[[ "${APP_ENV:-}" == "development" ]] ||
  fail "refusing to run because APP_ENV is not development"
[[ -n "${GAME_MASTER_ACCOUNT:-}" ]] ||
  fail "GAME_MASTER_ACCOUNT is not set"
[[ -n "${GAME_MASTER_ACCOUNT_PASSWORD:-}" ]] ||
  fail "GAME_MASTER_ACCOUNT_PASSWORD is not set"

if [[ -z "$test_account" ]]; then
  test_account=$GAME_MASTER_ACCOUNT
fi
test_password=${DEV_MUD_ACCOUNT_PASSWORD:-$GAME_MASTER_ACCOUNT_PASSWORD}

[[ "$test_account" =~ ^[[:alpha:]][[:alpha:]-]{1,29}$ ]] ||
  fail "local test account must contain only letters or hyphens"
export -n GAME_MASTER_ACCOUNT GAME_MASTER_ACCOUNT_PASSWORD
export -n DEV_MUD_ACCOUNT_PASSWORD

# Reuse the established preflight and startup path. Suppress its normal Kohdee
# smoke output because the creation result below is the relevant evidence.
"$script_dir/dev_kohdee_login_smoke.sh" >/dev/null

mud_port=$(awk -F= '
  /^[[:space:]]*DFLT_PORT[[:space:]]*=/ {
    value = $2
    gsub(/[[:space:]]/, "", value)
    print value
    exit
  }
' "$repo_root/lib/etc/config")

[[ "$mud_port" =~ ^[0-9]+$ ]] || fail "could not read DFLT_PORT from lib/etc/config"

MUD_CREATE_ACCOUNT="$test_account" \
MUD_CREATE_CHARACTER="$test_character" \
MUD_CREATE_PASSWORD="$test_password" \
MUD_CREATE_PORT="$mud_port" \
  expect -f /dev/stdin <<'EXPECT'
proc fail {message} {
  puts stderr "dev test-character creation: $message"
  exit 1
}

set timeout 45
match_max 200000
log_user 0

foreach required {
  MUD_CREATE_ACCOUNT MUD_CREATE_CHARACTER MUD_CREATE_PASSWORD MUD_CREATE_PORT
} {
  if {![info exists env($required)]} {
    fail "required environment is unavailable"
  }
}

set account_name $env(MUD_CREATE_ACCOUNT)
set character_name $env(MUD_CREATE_CHARACTER)

spawn -noecho nc 127.0.0.1 $env(MUD_CREATE_PORT)

expect {
  -re {What is your account name} {}
  timeout { fail "account prompt timeout" }
  eof { fail "connection closed at account prompt" }
}

send -- "$account_name\r"
expect {
  -re {Password:[[:space:]]*} {
    send -- "$env(MUD_CREATE_PASSWORD)\r"
    expect {
      -re {Your choice[[:space:]]*:} { set account_menu $expect_out(buffer) }
      -re {(Wrong|Incorrect|Invalid)[^\r\n]*password} {
        fail "account password was rejected"
      }
      timeout { fail "existing-account login timeout" }
      eof { fail "connection closed during existing-account login" }
    }
  }
  -re {Did I get that right} {
    send -- "y\r"
    expect {
      -re {Give me a Password:[[:space:]]*} {}
      timeout { fail "new-account password prompt timeout" }
    }

    send -- "$env(MUD_CREATE_PASSWORD)\r"
    expect {
      -re {Please retype password:[[:space:]]*} {}
      timeout { fail "password confirmation prompt timeout" }
    }

    send -- "$env(MUD_CREATE_PASSWORD)\r"
    expect {
      -re {Your choice[[:space:]]*:} { set account_menu $expect_out(buffer) }
      timeout { fail "new account menu timeout" }
    }
  }
  timeout { fail "account login or confirmation timeout" }
  eof { fail "connection closed during account login" }
}

set clean_menu $account_menu
regsub -all {\x1b\[[0-9;?]*[A-Za-z]} $clean_menu {} clean_menu
regsub -all {\t.} $clean_menu {} clean_menu
foreach menu_line [split $clean_menu "\n"] {
  set columns [split $menu_line "|"]
  if {[llength $columns] >= 2 &&
      [string equal -nocase [string trim [lindex $columns 1]] $character_name]} {
    fail "character $character_name already exists on account $account_name"
  }
}

send -- "c\r"
expect {
  -re {What will your new character be called[?][[:space:]]*:} {}
  timeout { fail "character-name prompt timeout" }
}

send -- "$character_name\r"
expect {
  -re {Did I get that right} {}
  timeout { fail "character-name confirmation timeout" }
}

send -- "y\r"
expect {
  -re {What is your sex} {}
  timeout { fail "sex prompt timeout" }
}

send -- "m\r"
expect {
  -re {Race Selection} {}
  timeout { fail "race prompt timeout" }
}

send -- "human\r"
expect {
  -re {Do you want to select this race[?]} {}
  timeout { fail "race confirmation timeout" }
}

send -- "y\r"
expect {
  -re {Class Selection} {}
  timeout { fail "class prompt timeout" }
}

send -- "warrior\r"
expect {
  -re {Do you want to select this class[?]} {}
  timeout { fail "class confirmation timeout" }
}

send -- "y\r"
expect {
  -re {Enter your choice [(]premade or custom[)]} {}
  timeout { fail "build prompt timeout" }
}

send -- "premade\r"
expect {
  -re {True Neutral} {}
  timeout { fail "alignment prompt timeout" }
}

after 200
send -- "4\r"
expect {
  -re {recommend preferences flags enabled[?]} {}
  timeout { fail "preferences prompt timeout" }
}

send -- "no\r"
expect {
  -re {Please Enter Your Choice} {}
  timeout { fail "roleplay prompt timeout" }
}

send -- "3\r"
expect {
  -re {PRESS RETURN} {}
  timeout { fail "MOTD prompt timeout" }
}

send -- "\r"
expect {
  -re {Make your choice:[[:space:]]*} {}
  timeout { fail "character menu timeout" }
}

send -- "1\r"
expect {
  -re {Welcome to Luminari} {}
  -re {May your visit here be} {}
  timeout { fail "world entry timeout" }
}

after 250
send -- "quit\r"
expect {
  -re {Goodbye, friend} {}
  -re {Reason:[[:space:]]*} {
    send -- "\r"
    exp_continue
  }
  timeout { fail "world logout timeout" }
}

expect {
  -re {Make your choice:[[:space:]]*} {}
  timeout { fail "post-quit character menu timeout" }
}

send -- "0\r"
expect {
  -re {Your choice[[:space:]]*:} {}
  timeout { fail "account menu return timeout" }
}

send -- "Q\r"
expect {
  -re {Quitting[.]} {}
  timeout { fail "account logout timeout" }
}

after 200
close
catch wait
exit 0
EXPECT

elapsed_seconds=$((SECONDS - started_at))
printf 'PASS: created, entered, and cleanly logged out local test character %s on account %s (%ss).\n' \
  "$test_character" "$test_account" "$elapsed_seconds"
