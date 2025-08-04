#!/bin/sh
#
# Simple Focused Debugging Script for Ancient MUD Codebase
# Catches the big problems without killing your system
#
mkdir -p log

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

echo "🎯 Running focused valgrind - 5 minute timeout to catch the big leaks..."

# 5-minute timeout prevents memory explosion
# Focus on definite leaks and errors that matter
timeout 5m valgrind \
    --leak-check=full \
    --show-leak-kinds=definite \
    --track-origins=yes \
    --show-reachable=no \
    --error-limit=yes \
    --num-callers=20 \
    --malloc-fill=0xAB \
    --free-fill=0xCD \
    --log-file=log/valgrind_focused_${TIMESTAMP}.log \
    bin/circle -q 4100

echo ""
echo "Analysis complete! Found the big problems in 5 minutes."
echo "Valgrind report: log/valgrind_focused_${TIMESTAMP}.log"
echo ""
echo "Focus on the DEFINITE leaks first - ignore 'possibly lost' for now ☕"