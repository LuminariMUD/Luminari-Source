#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
runner="$script_dir/run_vessel_ferry_soak.sh"

fail()
{
  printf 'vessel ferry preflight test: %s\n' "$*" >&2
  exit 1
}

[[ -x "$runner" ]] || fail "ferry runner is not executable: $runner"

# shellcheck disable=SC1090
. "$runner"
local_mud_error=""

test_root=$(mktemp -d "${TMPDIR:-/tmp}/vessel-ferry-preflight-test.XXXXXX")
trap 'rm -rf -- "$test_root"' EXIT

active_marker="$test_root/active"
server_log="$test_root/server.log"
bootstrap_output="$test_root/bootstrap.log"
helper_count="$test_root/helper-count"
helper="$test_root/login-helper"
failing_helper="$test_root/failing-login-helper"

export MOCK_ACTIVE_MARKER="$active_marker"
export MOCK_SERVER_LOG="$server_log"
export MOCK_HELPER_COUNT="$helper_count"

# The variables below are intentionally expanded by the generated helper.
# shellcheck disable=SC2016
printf '%s\n' \
  '#!/usr/bin/env bash' \
  'set -euo pipefail' \
  'printf "called\n" >>"$MOCK_HELPER_COUNT"' \
  ': >"$MOCK_SERVER_LOG"' \
  ': >"$MOCK_ACTIVE_MARKER"' \
  'printf "PASS: mock Kohdee login started development.\n"' >"$helper"
printf '%s\n' \
  '#!/usr/bin/env bash' \
  'exit 1' >"$failing_helper"
chmod +x "$helper" "$failing_helper"

systemctl()
{
  if [[ "$*" == "--user is-active --quiet test-mud.service" ]]; then
    [[ -f "$active_marker" ]]
    return
  fi
  return 1
}

timeout()
{
  local limit=$1

  [[ "$limit" == 120 ]] || return 1
  shift
  "$@"
}

ensure_local_mud_available "test-mud.service" "$server_log" \
  "$helper" "$bootstrap_output" ||
  fail "stopped development fixture did not bootstrap: $local_mud_error"
[[ -f "$active_marker" && -f "$server_log" ]] ||
  fail "bootstrap did not create the service and log fixtures"
[[ "$(wc -l <"$helper_count")" == 1 ]] ||
  fail "stopped development fixture did not invoke the login helper exactly once"
grep -Fq "PASS: mock Kohdee login started development." "$bootstrap_output" ||
  fail "bootstrap output was not preserved"

ensure_local_mud_available "test-mud.service" "$server_log" \
  "$failing_helper" "$bootstrap_output" ||
  fail "ready development fixture was rejected: $local_mud_error"
[[ "$(wc -l <"$helper_count")" == 1 ]] ||
  fail "ready development fixture unnecessarily invoked the login helper"

rm -f "$server_log"
if ensure_local_mud_available "test-mud.service" "$server_log" \
  "$helper" "$bootstrap_output"; then
  fail "active service without its log unexpectedly passed"
fi
[[ "$local_mud_error" == \
  "test-mud.service is active but its MUD log is unavailable" ]] ||
  fail "active service without a log reported the wrong error"

rm -f "$active_marker"
if ensure_local_mud_available "test-mud.service" "$server_log" \
  "$failing_helper" "$bootstrap_output"; then
  fail "failing stopped-service bootstrap unexpectedly passed"
fi
[[ "$local_mud_error" == \
  "the Kohdee login helper could not start the local MUD" ]] ||
  fail "failing stopped-service bootstrap reported the wrong error"

printf 'PASS: vessel ferry preflight bootstraps a stopped MUD and rejects invalid states.\n'
