#!/bin/bash
# run_phase01_tests.sh - Phase 01 Automation Layer Test Runner
# Part of LuminariMUD Vessel System Testing
#
# Runs all Phase 01 unit tests with optional Valgrind memory analysis
# and stress testing with configurable vessel count.
#
# Usage:
#   ./run_phase01_tests.sh           # Run all tests (no Valgrind)
#   ./run_phase01_tests.sh --valgrind # Run tests with Valgrind
#   ./run_phase01_tests.sh --stress N # Run stress test with N vessels

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_FILE="${SCRIPT_DIR}/phase01_results.log"
VALGRIND_OPTS="--leak-check=full --track-origins=yes --error-exitcode=1"

# Test binaries
TESTS=(
    "autopilot_tests:14:Autopilot Structs"
    "autopilot_pathfinding_tests:30:Pathfinding"
    "test_waypoint_cache:11:Waypoint Cache"
    "npc_pilot_tests:12:NPC Pilot"
    "schedule_tests:17:Schedule"
)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Parse arguments
USE_VALGRIND=0
STRESS_VESSELS=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --valgrind)
            USE_VALGRIND=1
            shift
            ;;
        --stress)
            STRESS_VESSELS="${2:-100}"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--valgrind] [--stress N]"
            exit 1
            ;;
    esac
done

# Functions
log_header() {
    echo ""
    echo "========================================="
    echo "$1"
    echo "========================================="
}

log_result() {
    local name=$1
    local result=$2
    local count=$3

    if [ "$result" -eq 0 ]; then
        echo -e "${GREEN}[PASS]${NC} $name ($count tests)"
        PASSED_TESTS=$((PASSED_TESTS + count))
    else
        echo -e "${RED}[FAIL]${NC} $name"
        FAILED_TESTS=$((FAILED_TESTS + count))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + count))
}

run_test() {
    local binary=$1
    local count=$2
    local name=$3

    echo ""
    echo "--- $name ($count tests) ---"

    if [ ! -x "${SCRIPT_DIR}/${binary}" ]; then
        echo -e "${RED}Error: ${binary} not found or not executable${NC}"
        return 1
    fi

    if [ "$USE_VALGRIND" -eq 1 ]; then
        if valgrind $VALGRIND_OPTS "${SCRIPT_DIR}/${binary}" 2>&1; then
            log_result "$name" 0 "$count"
            return 0
        else
            log_result "$name" 1 "$count"
            return 1
        fi
    else
        if "${SCRIPT_DIR}/${binary}"; then
            log_result "$name" 0 "$count"
            return 0
        else
            log_result "$name" 1 "$count"
            return 1
        fi
    fi
}

# Main execution
cd "$SCRIPT_DIR"

# Rebuild tests
log_header "Building Phase 01 Tests"
make -j4 2>&1

log_header "Phase 01 Automation Layer Tests"
echo "Date: $(date)"
echo "Valgrind: $([ "$USE_VALGRIND" -eq 1 ] && echo "Enabled" || echo "Disabled")"
echo ""

# Run all tests
for test_info in "${TESTS[@]}"; do
    IFS=':' read -r binary count name <<< "$test_info"
    run_test "$binary" "$count" "$name" || true
done

# Stress test
if [ "$STRESS_VESSELS" -gt 0 ]; then
    log_header "Stress Test ($STRESS_VESSELS vessels)"

    if [ ! -x "${SCRIPT_DIR}/vessel_stress_test" ]; then
        echo -e "${RED}Error: vessel_stress_test not found${NC}"
    else
        if [ "$USE_VALGRIND" -eq 1 ]; then
            valgrind $VALGRIND_OPTS "${SCRIPT_DIR}/vessel_stress_test" "$STRESS_VESSELS"
        else
            "${SCRIPT_DIR}/vessel_stress_test" "$STRESS_VESSELS"
        fi
    fi
fi

# Summary
log_header "Test Summary"
echo "Total Tests: $TOTAL_TESTS"
echo "Passed:      $PASSED_TESTS"
echo "Failed:      $FAILED_TESTS"
echo ""

if [ "$FAILED_TESTS" -eq 0 ]; then
    echo -e "${GREEN}All Phase 01 tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi
