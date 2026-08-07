#!/bin/bash

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
readonly PROJECT_ROOT
readonly HEALTHCHECK="$PROJECT_ROOT/scripts/operations/healthcheck.sh"

test_root=$(mktemp -d "${TMPDIR:-/tmp}/luminari-healthcheck-test.XXXXXX")
trap 'rm -rf -- "$test_root"' EXIT
mkdir -p "$test_root/bin"

cat > "$test_root/bin/curl" <<'FAKE_CURL'
#!/bin/bash
case "${FAKE_CURL_RESPONSE:-healthy}" in
    healthy)
        echo '{"service":"luminari-mud","status":"healthy","database":"healthy","uptime_seconds":5}'
        ;;
    unhealthy)
        echo '{"service":"luminari-mud","status":"unhealthy","database":"unhealthy","uptime_seconds":5}'
        ;;
    malformed)
        echo '{"status":"healthy"}'
        ;;
    transport-error)
        exit 22
        ;;
    *)
        exit 2
        ;;
esac
FAKE_CURL
chmod +x "$test_root/bin/curl"

run_healthcheck()
{
    PATH="$test_root/bin:$PATH" \
        LUMINARI_HEALTH_URL=http://127.0.0.1:8182/health \
        "$HEALTHCHECK"
}

FAKE_CURL_RESPONSE=healthy run_healthcheck >/dev/null

if FAKE_CURL_RESPONSE=unhealthy run_healthcheck >/dev/null 2>&1; then
    echo "FAIL: unhealthy database payload was accepted" >&2
    exit 1
fi

if FAKE_CURL_RESPONSE=malformed run_healthcheck >/dev/null 2>&1; then
    echo "FAIL: malformed readiness payload was accepted" >&2
    exit 1
fi

if FAKE_CURL_RESPONSE=transport-error run_healthcheck >/dev/null 2>&1; then
    echo "FAIL: curl transport failure was accepted" >&2
    exit 1
fi

echo "healthcheck regression tests passed"
