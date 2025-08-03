#!/bin/bash
echo "Starting LuminariMUD in GDB debugger..."
echo ""
echo "Useful GDB commands:"
echo "  run -q 4100     - Start the game on port 4100"
echo "  bt              - Show backtrace after crash"
echo "  info locals     - Show local variables"
echo "  info args       - Show function arguments"
echo "  up/down         - Navigate stack frames"
echo "  print varname   - Print variable value"
echo "  continue        - Continue execution"
echo "  quit            - Exit GDB"
echo ""
echo "Starting GDB..."

gdb -ex "set pagination off" \
    -ex "set print pretty on" \
    -ex "handle SIGPIPE nostop noprint pass" \
    -ex "break basic_mud_vlog" \
    -ex "run -q 4100" \
    ../bin/circle