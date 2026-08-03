#!/bin/sh
#
# Simple Focused Debugging Script for Ancient MUD Codebase
# Catches the big problems without killing your system
#
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
LOG_DIR="$PROJECT_ROOT/log"

mkdir -p "$LOG_DIR"

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

echo "🎯 Running focused valgrind - 20 minute timeout to catch the big leaks..."

# 20-minute timeout to allow full game loading in valgrind
# Focus on definite leaks and errors that matter
timeout 20m valgrind \
    --leak-check=full \
    --show-leak-kinds=definite \
    --track-origins=yes \
    --show-reachable=no \
    --error-limit=yes \
    --num-callers=20 \
    --malloc-fill=0xAB \
    --free-fill=0xCD \
    --log-file="$LOG_DIR/valgrind_focused_${TIMESTAMP}.log" \
    "$PROJECT_ROOT/bin/circle" -q 4100

echo ""
echo "Analysis complete! Found the big problems in up to 20 minutes."
echo "Valgrind report: $LOG_DIR/valgrind_focused_${TIMESTAMP}.log"
echo ""
echo "Focus on the DEFINITE leaks first - ignore 'possibly lost' for now ☕"
