#!/bin/bash

# Check if binary exists
if [ ! -f "../bin/circle" ]; then
    echo "ERROR: ../bin/circle not found. Run 'make' first."
    exit 1
fi

# Set default port if not provided
PORT=${1:-4100}

echo "Starting LuminariMUD in GDB debugger..."
echo ""
echo "Useful GDB commands:"
echo "  run -q $PORT    - Start the game on port $PORT"
echo "  bt              - Show backtrace after crash"
echo "  bt full         - Show detailed backtrace with locals"
echo "  info locals     - Show local variables"
echo "  info args       - Show function arguments"
echo "  up/down         - Navigate stack frames"
echo "  print varname   - Print variable value"
echo "  list            - Show source code around current line"
echo "  continue        - Continue execution"
echo "  step            - Step into function"
echo "  next            - Step over function"
echo "  finish          - Step out of current function"
echo "  watch variable  - Break when variable changes"
echo "  info threads    - Show all threads (if using threads)"
echo "  quit            - Exit GDB"
echo ""
echo "MUD-specific commands:"
echo "  mud_status      - Show MUD status info"
echo "  show_char <ptr> - Display character info"
echo "  show_obj <ptr>  - Display object info"
echo "  show_room <num> - Display room info"
echo ""
echo "Log file: log/gdb_debug.log"
echo ""
echo "Starting GDB..."

# Create .gdbinit for this session with MUD-specific settings
cat > .gdbinit_mud << 'EOF'
# MUD-specific GDB settings
define mud_status
    printf "=== MUD Status ===\n"
    if (circle_shutdown)
        printf "Shutdown flag: %d\n", circle_shutdown
    end
    if (pulse)
        printf "Current pulse: %ld\n", pulse
    end
    printf "==================\n"
end

define show_char
    if $argc == 0
        printf "Usage: show_char <char_pointer>\n"
    else
        set $ch = (struct char_data *)$arg0
        if $ch
            printf "Character: %s\n", $ch->player.name ? $ch->player.name : "NULL"
            printf "Level: %d\n", $ch->player.level
            printf "Room: %d\n", $ch->in_room
            printf "HP: %d/%d\n", $ch->points.hit, $ch->points.max_hit
            printf "Gold: %d\n", $ch->points.gold
            if $ch->mob_specials.func
                printf "Special func: YES\n"
            else
                printf "Special func: NO\n"
            end
        else
            printf "NULL character pointer\n"
        end
    end
end

define show_obj
    if $argc == 0
        printf "Usage: show_obj <obj_pointer>\n"
    else
        set $obj = (struct obj_data *)$arg0
        if $obj
            printf "Object: %s\n", $obj->name ? $obj->name : "NULL"
            printf "Short: %s\n", $obj->short_description ? $obj->short_description : "NULL"
            printf "Type: %d\n", $obj->obj_flags.type_flag
            printf "Weight: %d\n", $obj->obj_flags.weight
            printf "Cost: %d\n", $obj->obj_flags.cost
            printf "In room: %d\n", $obj->in_room
        else
            printf "NULL object pointer\n"
        end
    end
end

define show_room
    if $argc == 0
        printf "Usage: show_room <room_num>\n"
    else
        set $rnum = (int)$arg0
        if $rnum >= 0 && $rnum < top_of_world
            set $room = &world[$rnum]
            printf "Room #%d: %s\n", $room->number, $room->name ? $room->name : "NULL"
            printf "Description: %.60s...\n", $room->description ? $room->description : "NULL"
            printf "Zone: %d\n", $room->zone
            printf "Flags: %lld\n", $room->room_flags
            printf "Sector: %d\n", $room->sector_type
        else
            printf "Invalid room number (must be 0-%d)\n", top_of_world-1
        end
    end
end

# Convenience aliases
define bt_all
    thread apply all bt
end

define locals_all
    info locals
    info args
end

# Help command
define mudhelp
    printf "MUD-specific GDB commands:\n"
    printf "  mud_status      - Show shutdown flag and pulse counter\n"
    printf "  show_char <ptr> - Display character details\n"
    printf "  show_obj <ptr>  - Display object details\n"
    printf "  show_room <num> - Display room details\n"
    printf "  bt_all          - Backtrace all threads\n"
    printf "  locals_all      - Show locals and arguments\n"
    printf "\nExample usage:\n"
    printf "  show_char ch\n"
    printf "  show_obj obj\n"
    printf "  show_room 3001\n"
end

printf "MUD debugging extensions loaded. Type 'mudhelp' for commands.\n"
EOF

# Create a more comprehensive GDB command file
cat > .gdbcommands << 'EOF'
set pagination off
set print pretty on
set print array on
set print array-indexes on
set logging enabled on
set logging file log/gdb_debug.log
set logging overwrite on
set logging debugredirect on
handle SIGPIPE nostop noprint pass
handle SIGUSR1 nostop noprint pass
handle SIGUSR2 nostop noprint pass
break core_dump_real
break abort
break __assert_fail
break exit if $_exitcode != 0
set confirm off
set debuginfod enabled off
echo \n=== GDB Starting LuminariMUD ===\n
echo Loading MUD-specific debugging commands...\n
source .gdbinit_mud
echo \n
EOF

# Add port argument to run command
echo "run -q $PORT" >> .gdbcommands

# Run GDB with command file
gdb -x .gdbcommands ../bin/circle

# Cleanup
rm -f .gdbinit_mud .gdbcommands