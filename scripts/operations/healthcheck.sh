#!/bin/bash

set -u

readonly HEALTH_URL="${LUMINARI_HEALTH_URL:-http://127.0.0.1:8182/health}"
readonly REQUEST_TIMEOUT="${LUMINARI_HEALTH_REQUEST_TIMEOUT_SECONDS:-3}"
readonly WAIT_TIMEOUT="${LUMINARI_HEALTH_TIMEOUT_SECONDS:-90}"
readonly WAIT_INTERVAL="${LUMINARI_HEALTH_INTERVAL_SECONDS:-2}"

wait_for_health=false

usage()
{
    echo "Usage: $0 [--wait]"
    echo
    echo "Checks the LuminariMUD readiness endpoint and required MariaDB status."
    echo "Set LUMINARI_HEALTH_URL to override the loopback endpoint."
}

fail()
{
    echo "healthcheck: $*" >&2
    exit 1
}

validate_positive_integer()
{
    local name="$1"
    local value="$2"

    [[ "$value" =~ ^[1-9][0-9]*$ ]] || fail "$name must be a positive integer"
}

check_once()
{
    local response

    if ! response=$(curl --silent --show-error --fail \
        --max-time "$REQUEST_TIMEOUT" "$HEALTH_URL"); then
        return 1
    fi

    if [[ "$response" != *'"service":"luminari-mud"'* ]] ||
       [[ "$response" != *'"status":"healthy"'* ]] ||
       [[ "$response" != *'"database":"healthy"'* ]]; then
        echo "healthcheck: endpoint returned an invalid readiness payload" >&2
        return 1
    fi

    echo "$response"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --wait)
            wait_for_health=true
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage >&2
            fail "unknown option: $1"
            ;;
    esac
    shift
done

command -v curl >/dev/null 2>&1 || fail "curl is required"
validate_positive_integer LUMINARI_HEALTH_REQUEST_TIMEOUT_SECONDS "$REQUEST_TIMEOUT"
validate_positive_integer LUMINARI_HEALTH_TIMEOUT_SECONDS "$WAIT_TIMEOUT"
validate_positive_integer LUMINARI_HEALTH_INTERVAL_SECONDS "$WAIT_INTERVAL"

if [[ "$wait_for_health" != true ]]; then
    check_once || fail "readiness check failed for $HEALTH_URL"
    exit 0
fi

deadline=$((SECONDS + WAIT_TIMEOUT))
while ((SECONDS < deadline)); do
    if check_once; then
        exit 0
    fi
    sleep "$WAIT_INTERVAL"
done

fail "readiness did not pass within ${WAIT_TIMEOUT}s for $HEALTH_URL"
