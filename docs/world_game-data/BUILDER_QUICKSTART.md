# Builder Quickstart: One Zone From Empty to Bootable

This walks a single zone from nothing to a server that boots with it. It is
deliberately small - three rooms, one mobile, one object - so you can see every
file that has to exist and every place a vnum has to agree with another vnum.

Read it once through before typing anything. The order matters: the world
loader is positional and unforgiving, and most first-zone failures are a file
that exists but is not indexed, or a vnum that appears in one file and not
another.

## Two Ways to Build

**In-game with OLC is the recommended path.** `zedit`, `redit`, `medit`, and
`oedit` write correct files by construction and cannot produce the malformed
records that abort a boot. If you have builder access and a running server,
skip to [The OLC Path](#the-olc-path).

**Hand-editing** is covered because you will need to read these files even if
you never write them, and because bulk or scripted work sometimes calls for it.
If you hand-edit, do it against a copy and boot-test before you go near a live
world.

## Pick a VNUM Range

Zones own a contiguous vnum range and by convention a zone numbered `N` owns
vnums starting at `N * 100`. Rooms, mobiles, objects, shops, and triggers each
have their own vnum space, so room 3000 and mobile 3000 are unrelated.

Before choosing, find out what is taken:

```
zlist               List every zone
rlist 3000 3099     List rooms already in that range
mlist 3000 3099     Mobiles
olist 3000 3099     Objects
```

Do this first. A vnum collision is not detected at boot - the later definition
silently wins.

For this walkthrough the zone is **30**, owning vnums **3000-3099**.

## The Files

A zone needs an entry in each index file for every type it uses. Files that
exist but are not listed in the index are simply never read - this is the most
common "my zone did not load and there is no error" cause.

```
lib/world/wld/30.wld     rooms          indexed in lib/world/wld/index
lib/world/mob/30.mob     mobiles        indexed in lib/world/mob/index
lib/world/obj/30.obj     objects        indexed in lib/world/obj/index
lib/world/zon/30.zon     zone + resets  indexed in lib/world/zon/index
```

Shops (`shp/`), triggers (`trg/`), quests (`qst/`), and high-level quests
(`hlq/`) work the same way and are optional. A `.qst` record is keyed by its
quest VNUM. An `.hlq` block is keyed by the mobile VNUM that hosts its entries.

An index file is one filename per line, terminated by a line containing `$`:

```
0.wld
30.wld
$
```

There is a second index per directory, `index.mini`, used when the server is
started in mini-mud mode. If you want your zone in mini-mud runs, add it there
too; otherwise leave `index.mini` alone.

## Step 1: The Zone File

`lib/world/zon/30.zon`:

```
#30
Your Name~
The Example Zone~
3000 3099 30 2 0 0 0 0 1 5 1
S
$
```

Line by line: zone vnum, builder namelist, zone name, then the numeric header -
vnums 3000 through 3099, reset every 30 minutes, reset mode 2 (always), no zone
flags, levels 1-5, weather shown. `S` ends the reset command list (empty for
now) and `$` ends the file.

The numeric header's field count is significant: the parser accepts exactly 4,
10, 11, or 14 fields and silently discards the extras from any other count. See
the [Zone File Format Reference](ZONE_FILE_FORMAT.md) for the full field list
and the traps around it.

## Step 2: The Rooms

`lib/world/wld/30.wld`:

```
#3000
The Entrance Hall~
   A short hall of grey stone runs north, its walls bare except for the
soot of old torches.
~
30 0 0 0 0 0
D0
The hall continues north.~
~
0 -1 3001
S
#3001
The Inner Chamber~
   The hall opens into a round chamber with a low ceiling.
~
30 0 0 0 0 0
D2
The way back south.~
~
0 -1 3000
S
$
```

Per record: vnum, name, description (terminated by a lone `~`), then the
numeric line - **zone number, then four room-flag bitvector fields, then the
sector type**. Each `D<n>` block is an exit: description, keyword list, then
door flags, key vnum, destination vnum. `S` ends the record.

Directions are `D0` north, `D1` east, `D2` south, `D3` west, `D4` up, `D5`
down.

Room flags and sector types are documented in
[ROOM_FLAGS.md](ROOM_FLAGS.md).

Two things that bite:

- **Exits are one-way.** Room 3000's north exit to 3001 does not create 3001's
  south exit back. Write both, as above.
- **An exit to a room that does not exist is silently nulled.** `renum_world()`
  rewrites it to `NOWHERE` and logs nothing. If a door leads nowhere after a
  clean boot, the destination vnum is wrong.

## Step 3: A Mobile

`lib/world/mob/30.mob`:

```
#3000
guard watchman~
a watchman~
A bored watchman leans against the wall here.
~
He wears the plain tabard of the town watch.
~
0 0 0 0 0 0 0 0 0 E
5 20 10 5d8+20 1d6+2
100 100 1
8 8 1
BareHandAttack: 0
E
$
```

The `E` at the end of the flag line marks this as the enhanced mobile format;
the trailing `E` closes the record. Mobile action flags are documented in
[MOB_FLAGS.md](MOB_FLAGS.md).

## Step 4: An Object

`lib/world/obj/30.obj`:

```
#3000
torch light~
a burning torch~
A burning torch lies here.~
~
1 0 1 0
0 0 24 0 0 0 0 0 0 0 0 0 0 0 0 0
1 10 5 0
$~
```

The first numeric line is item type, extra flags, wear flags, and a fourth
field. The second is the **value vector**, and it must contain **exactly 4 or
exactly 16 integers** - any other count is a fatal boot error. Sixteen is
correct for anything new, because the enhancement bonus lives in `value[4]` and
is unreachable in the 4-value form.

Here the type is `1` (Light), wear flags allow taking and holding, and
`value[2]` is 24 hours of burn time.

The full per-type value layout is in the
[OEDIT Guide](OEDIT_GUIDE.md#object-value-reference).

## Step 5: Populate the Zone

Now go back to `30.zon` and add reset commands between the header and the `S`:

```
#30
Your Name~
The Example Zone~
3000 3099 30 2 0 0 0 0 1 5 1
M 0 3000 1 3000
E 1 3000 1 14
S
$
```

`M 0 3000 1 3000` loads mobile 3000 into room 3000, with an if-flag of `0`
(always run) and a maximum of one in the world.

`E 1 3000 1 14` equips the mobile that was just loaded with object 3000 in wear
position 14 (`Hold`). Its if-flag is `1` - **run only if the previous command
ran.** This is what stops the torch spawning on a watchman that never loaded.

Reset commands must start in column 0. An indented command is counted
differently by the parser's two passes and produces
`SYSERR: Zone command count mismatch`.

## Validate Before Booting

From the repository root, validate the complete zone package after every save:

```sh
python3 scripts/world/wtool.py validate --zone 30
```

The compatibility wrapper is shorter and works even when called from another
current directory:

```sh
lib/world/validate-zone.sh 30
```

Fix every error, then review the warnings. Use strict mode when the zone is
expected to have no warnings, or JSON when another tool will consume the
result:

```sh
python3 scripts/world/wtool.py --json validate --zone 30 > /tmp/zone-30.json
lib/world/validate-zone.sh 30 --strict
```

The validator reports all independently recoverable parser, reference, and
topology findings in one run. It is read-only and needs neither MariaDB nor a
`circle` build.

## Step 6: Index and Boot

Add `30.wld`, `30.mob`, `30.obj`, and `30.zon` to their respective `index`
files, above the `$` line. Then boot:

```
./bin/circle -d lib
```

Watch the log. Every world-data problem is reported as `SYSERR` and, for the
parsers, immediately calls `exit(1)` - so you get one error per run. Fix it,
boot again, repeat.

Common first-boot errors and what they mean:

| Message | Cause |
|---------|-------|
| `Format error in second numeric line (expecting 4 or 16 args, got N)` | Object value line has the wrong integer count |
| `Zone command count mismatch` | A reset command is indented, or `S` is not alone on its line |
| `Format error in numeric constant line` | The zone header's field count matched none of 14/11/10/4 |
| `Expecting numeric line of room #N` | A description is missing its closing `~` |
| `ZONE ERROR: ... not found` | A reset command names a vnum that does not exist |

Note the difference between the last one and the rest: `ZONE ERROR` lines are
warnings and the boot continues. The others abort.

## The OLC Path

The same zone, built in-game:

```
zedit new 30 3000 3099    Create zone 30 owning vnums 3000-3099
redit 3000                Create and edit the first room
dig north 3001            From inside 3000: create room 3001 and link both ways
medit 3000                Create the watchman
oedit 3000                Create the torch
zedit 30                  Add the M and E reset commands
```

Each editor saves with `Q` then `Y`. OLC writes the flat files and the index
entries for you, so there is no separate indexing step. Return to the same
`validate --zone 30` step after each OLC save.

The two quest editors use different models:

```text
qedit 3001       Edit a numbered quest record saved in qst/30.qst
hlqedit 3000     Edit ASK/GIVE/ROOM entries attached to mobile 3000
```

After QEDIT saves, inspect the quest and every typed dependency:

```sh
python3 scripts/world/wtool.py validate --zone 30
python3 scripts/world/wtool.py show quest 3001
python3 scripts/world/wtool.py refs quest 3001
```

After HLQEDIT saves, use the host mobile VNUM. The display includes both file
order and effective runtime order:

```sh
python3 scripts/world/wtool.py validate --zone 30
python3 scripts/world/wtool.py show hlquest 3000
python3 scripts/world/wtool.py refs hlquest 3000
```

Validation is still followed by a development boot and playtest. In
particular, exercise quest acceptance/completion and the intended HLQ
conversation, give, room, input, and output paths.

`dig` is worth learning early. Its full syntax is:

```
dig <direction> <room>    Create the exit, creating the room if needed
dig <direction> -1        Delete the exit
```

It handles both directions of the link, which removes the most common
hand-editing mistake.

Full editor and listing command reference:
[OLC System](../systems/OLC_ONLINE_CREATION_SYSTEM.md).

When a finding names an unexpected reference, inspect the normalized record
and both sides of its typed reference graph:

```sh
python3 scripts/world/wtool.py show room 3000
python3 scripts/world/wtool.py refs room 3000
```

## Where to Go Next

- [Builder Manual](builder_manual.md) - the long-form guide
- [Zone File Format Reference](ZONE_FILE_FORMAT.md) - every reset command
- [Shop File Format Reference](SHOP_FILE_FORMAT.md) - adding a shopkeeper
- [Quest File Format Reference](QUEST_FILE_FORMAT.md) - QST grammar, types,
  flags, rewards, and chains
- [High-Level Quest File Format Reference](HLQUEST_FILE_FORMAT.md) - HLQ
  entries, commands, legality, and runtime order
- [OEDIT Guide](OEDIT_GUIDE.md) - item types, wear slots, value vectors
- [ROOM_FLAGS.md](ROOM_FLAGS.md) and [MOB_FLAGS.md](MOB_FLAGS.md)
- [World Validator CLI](../utilities/WORLD_VALIDATOR_CLI.md) - all commands,
  finding behavior, and exit statuses
- [DG Scripting](../systems/SCRIPTING_SYSTEM_DG.md) - making things react
