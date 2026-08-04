# Zone File (`.zon`) Format Reference

Zone files live in `lib/world/zon/` and are read by `load_zones()` in
`src/db.c`. A zone file defines one zone: its vnum range, its reset behavior,
and the list of reset commands that populate it with mobiles and objects.

Everything below is derived from `load_zones()` and `reset_zone()`. Where the
parser is stricter or looser than you would expect, that is called out - those
are the cases that cost people a boot.

**A parse error in a zone file is fatal.** The server logs one `SYSERR` and
calls `exit(1)`. It does not skip the zone and it does not report a second
problem, so fix errors one at a time.

## File Layout

```
#<zone vnum>
<builder namelist>~
<zone name>~
<numeric header line>
<reset command>
<reset command>
...
S
$
```

The `S` line terminates the command list. The `$` line terminates the file.
Both are required.

### Example

This is `lib/world/minimal/0.zon` in full:

```
#0
None.~
The Void~
0 3099 30 2
S
$
```

## The Numeric Header Line

`load_zones()` tries four field counts in order and takes the first that
matches exactly: **14, then 11, then 10, then 4.**

| Field | Name | Meaning |
|-------|------|---------|
| 1 | `bot` | Lowest vnum in the zone |
| 2 | `top` | Highest vnum in the zone |
| 3 | `lifespan` | Minutes between resets |
| 4 | `reset_mode` | See below |
| 5-8 | zone flags | Four bitvector fields, see [Zone Flags](#zone-flags) |
| 9 | `min_level` | Minimum recommended level (`-1` for none) |
| 10 | `max_level` | Maximum recommended level (`-1` for none) |
| 11 | `show_weather` | `1` to report weather in this zone, `0` to suppress |
| 12 | `region` | Region id |
| 13 | `faction` | Controlling faction id |
| 14 | `city` | Associated city id |

`bot` greater than `top` is a fatal error.

### The 12-and-13-field trap

Because the parser falls back on *exact* counts, a header with 12 or 13 fields
does not fail - it fails the 14-field scan, then **succeeds** at the 11-field
scan, and `region`, `faction`, and `city` are silently discarded. There is no
warning. If you are using any of those three fields, write all fourteen.

The same applies to counts between 4 and 10: a 5-to-9 field line silently
degrades to the 4-field form, losing flags and level range.

### `reset_mode`

| Value | Behavior |
|-------|----------|
| 0 | Never reset |
| 1 | Reset only when the zone is empty of players |
| 2 | Reset on schedule regardless of who is present |

Modes 1 and 2 both age at one point per minute and enqueue a reset once `age`
reaches `lifespan`. Mode 0 never ages, so it never resets.

### Zone Flags

Fields 5 through 8 are four separate bitvector fields, converted with
`asciiflag_conv()`. Each accepts either a decimal number or a letter string
where `a`-`z` are bits 0-25 and `A`-`Z` are bits 26-51. A token made entirely
of digits is read as decimal; anything else is read as letters. `0` is the
usual "no flags" value.

| Bit | Flag |
|-----|------|
| 0 | Closed |
| 1 | No-Immortal |
| 2 | Quest |
| 3 | Open for Players |
| 4 | No-Build |
| 5 | No-Teleport |
| 6 | Worldmap |
| 7 | Not-Claimable |
| 8 | Astral-Plane |
| 9 | Ethereal-Plane |
| 10 | Elemental-Plane |
| 11 | Wilderness |
| 12 | Random-Chests |
| 13 | Random-Traps |
| 14 | No-Automap |
| 15 | Hunts-Can-Load |
| 16 | Missions-Can-Load |
| 17 | Random-Encounters |

## Reset Commands

Each command is one line. The general shape is:

```
<char> <if-flag> <arg1> <arg2> [arg3] [arg4]
```

The **if-flag** is the first number after the command character. `0` means
"always execute this command"; `1` means "execute only if the previous command
executed". This is how you attach an object to a mobile that may not have
loaded.

**All vnums in reset commands are real numbers, not virtual numbers**, after
`renum_zone_table()` converts them at boot. You write vnums in the file; the
conversion is automatic. A vnum that does not resolve produces a
`ZONE ERROR` log line for objects, but see the warning at the end of this
document about rooms.

### Command Summary

| Char | Purpose | Args after the if-flag |
|------|---------|------------------------|
| `M` | Load a mobile into a room | mob vnum, max existing, room vnum, [load %] |
| `O` | Load an object into a room | obj vnum, max existing, room vnum, [load %] |
| `P` | Put an object inside another object | obj vnum, max existing, container vnum, [load %] |
| `G` | Give an object to the last mobile loaded | obj vnum, max existing, [load %] |
| `E` | Equip the last mobile loaded | obj vnum, max existing, wear position, [load %] |
| `D` | Set the state of a door | room vnum, direction, state |
| `R` | Remove an object from a room | room vnum, obj vnum |
| `T` | Attach a trigger | trigger target type, trigger vnum, target vnum |
| `V` | Assign a DG script variable | target type, context, var name, var value |
| `J` | Jump over the following lines | number of lines, [chance %] |
| `I` | Load random treasure onto a mobile | chance %, plus a third ignored number - see below |
| `L` | Load random treasure into a container | non-functional, see below |
| `S` | End of command list | none |
| `*` | Comment; the whole line is ignored | none |

### Load Percentage

`M`, `O`, and `P` take an optional fifth number: a 1-100 percentage chance the
command runs. `G` takes it as its fourth. If you omit it, or pass a negative
number, it defaults to **100** - the command always runs.

`max existing` caps how many of that prototype may exist world-wide before the
command is skipped. A `0` here means "load only at boot time".

### Door States

The `D` command's third argument selects a combination of closed, locked, and
hidden states with three difficulty tiers each:

| State | Effect |
|-------|--------|
| 0 | Open and unlocked |
| 1 | Closed |
| 2 | Closed and locked (easy) |
| 3 | Closed, locked (easy), hidden (easy) |
| 4 | Closed, locked (easy), hidden (medium) |
| 5 | Closed, locked (easy), hidden (hard) |
| 6 | Closed and hidden (easy) |
| 7 | Closed and hidden (medium) |
| 8 | Closed and hidden (hard) |
| 9 | Closed, locked (medium), hidden (easy) |
| 10 | Closed, locked (medium), hidden (medium) |
| 11 | Closed, locked (medium), hidden (hard) |
| 12 | Closed and locked (medium) |
| 13 | Closed, locked (hard), hidden (easy) |
| 14 | Closed, locked (hard), hidden (medium) |
| 15 | Closed, locked (hard), hidden (hard) |
| 16 | Closed and locked (hard) |

Directions are the standard `0` north, `1` east, `2` south, `3` west, `4` up,
`5` down. A `D` command naming a direction with no exit logs a `ZONE ERROR` and
is skipped - it does not abort the boot.

## Parser Gotchas

These are behaviors of `load_zones()` that are easy to trip over and produce
confusing failures.

### Reset commands must start in column 0

The parser reads the file twice. The first pass counts commands by testing
`buf[0]` directly:

```c
if ((strchr("MOPGERDTVJIL", buf[0]) && buf[1] == ' ') || (buf[0] == 'S' && buf[1] == '\0'))
```

The second pass calls `skip_spaces()` before reading the command character.
An **indented** reset command is therefore missed by the count but parsed by
the loop, and the two disagree:

```
SYSERR: Zone command count mismatch for <file>. Estimated: N, Actual: M
```

Never indent a reset command. The count also requires exactly one space after
the command character, and requires the terminating `S` to be alone on its line
with nothing after it.

### `I` takes three arguments, not two

`load_zones()` dispatches on `strchr("MOGEPDTVJL", command)`. Note that `I` and
`R` are **absent** from that string, so both fall through to the generic
three-argument branch before their `case` labels are ever reached. The
`case 'I'` and `case 'R'` blocks in the switch are unreachable.

For `R` this is harmless - the generic branch parses exactly the three
arguments `R` wants. For `I` it is not: the switch case expects two arguments,
but the generic branch demands three, so a two-argument `I` line fails to parse
and aborts the boot. Write `I` with three numbers.

This is a source-level bug, not a documentation quirk. It is recorded in
[known-issues.md](../known-issues.md).

### `L` does not work

The `L` command parses only two arguments after its if-flag, but `reset_zone()`
reads `arg3` as the container to fill - and `arg3` is never assigned, so it is
always zero. Even when the chance roll passes and a container is somehow found,
the call that would actually place the treasure is commented out and marked
`Unfinished`. `L` is dead weight: it will either do nothing or log
`ZONE ERROR: target obj not found`. Do not use it. Also recorded in
[known-issues.md](../known-issues.md).

### Unresolvable room references are silenced

`renum_zone_table()` logs a `ZONE ERROR` when an object or mobile vnum in a
reset command cannot be resolved. `renum_world()`, which resolves room exits,
does **not**: an exit pointing at a nonexistent room is quietly rewritten to
`NOWHERE`. A door that leads nowhere after a boot is usually this, and nothing
in the log will tell you.

## Validation and Lookup

Run the standalone validator from the repository root before booting a changed
zone. It catches the parser traps above, validates typed reset targets, and
detects room exits that the server would silently null:

```sh
python3 scripts/world/wtool.py validate --zone 30
lib/world/validate-zone.sh 30 --strict
```

Use typed lookup when a reset or exit finding crosses files:

```sh
python3 scripts/world/wtool.py show zone 30
python3 scripts/world/wtool.py refs room 3000
```

The wrapper finds the repository from its own path and preserves validator
exit status when called outside the repository. See the
[World Validator CLI](../utilities/WORLD_VALIDATOR_CLI.md) for JSON output,
exit statuses, and staging-world selection.

## Related

- [Room Flags Reference](ROOM_FLAGS.md)
- [Mobile Flags Reference](MOB_FLAGS.md)
- [OEDIT Guide](OEDIT_GUIDE.md) - includes the object value reference
- [OLC System](../systems/OLC_ONLINE_CREATION_SYSTEM.md) - `zedit` edits these
  files in-game, which is safer than hand-editing
