#!/bin/bash
cd "$(dirname "$0")"
echo "Starting LuminariMUD under GDB on port 4100..."
echo "Commands:"
echo "  run -q 4100  - Start the game"
echo "  bt           - Show backtrace after crash"
echo "  quit         - Exit GDB"
echo ""
gdb ../bin/circle