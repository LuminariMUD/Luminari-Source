# High-Level Quest File Format (`.hlq`)

This guide documents high-level quest records loaded from `lib/world/hlq`.
Unlike `.qst`, an HLQ record has no independent quest VNUM. Its lookup key is
the host mobile VNUM, and one host block contains ASK, GIVE, and ROOM entries.

The authoritative implementation is split across:

- `src/quest/hlquest.h` for entry and command types;
- `src/quest/hlquest.c` for `boot_the_quests()` and runtime execution;
- `src/olc/hlqedit.c` for `hlqedit_parse()` and the command-code table; and
- `src/olc/hlqedit.c` for the canonical `hlqedit_save_to_disk()` writer.

## Files, Indexes, and Packages

Normal boot reads `lib/world/hlq/index`; mini-mud boot reads
`lib/world/hlq/index.mini`. Each significant index line names one `.hlq`
package, and `$` terminates the index. Package files normally use the zone
number, for example `100.hlq`.

The `#<host-mobile-vnum>` header attaches all following entries to that mobile
until another host header or the file terminator. The host must exist. HLQEDIT
saves the block in the package that owns the host VNUM. Duplicate host blocks
remain distinct to the validator because the server would attach entries from
all of them.

## Host and Entry Grammar

The canonical writer emits combinations of these forms:

```text
#<host-mobile-vnum>
A[!]
<keywords>~
<reply>~

Q[!]
<reply>~
I <command-code> <value> <location>
O <command-code> <value> <location>
S

R[!]
<room-vnum>
<reply>~
I <command-code> <value> <location>
O <command-code> <value> <location>
S
```

The package ends with `$~`; `$` is also loader-compatible. Blank lines and
lines beginning with `*` are ignored where the shared line reader accepts
them. Reply and keyword fields are tilde-terminated strings and may span
physical lines.

The optional `!` suffix means approved. The loader treats any suffix after the
entry marker as approved, but only `!` is canonical. Another suffix is a
warning because it may hide a typo. An entry without a suffix is a legitimate
unapproved builder state.

ASK entries have keywords and a reply, with no command chain and no `S`.
GIVE entries have a reply followed by zero or more commands and a required
`S`. ROOM entries add a room VNUM before the reply and also require the command
chain terminator.

Every command line has exactly four tokens:

```text
<direction> <command-code> <value> <location>
```

`I` means input and `O` means output. The `S` terminator is a single token,
not the cast-spell command code; cast spell appears as `O S value location`.

## Entry Types Reference

This table is checked against `src/quest/hlquest.h` by
`wtool docs --check`.

| Index | Macro | Marker | Meaning |
|-------|-------|--------|---------|
| 0 | QUEST_ASK | `A` | Match spoken keywords and return a reply. |
| 1 | QUEST_GIVE | `Q` | Consume matching inputs, then execute outputs. |
| 2 | QUEST_ROOM | `R` | Execute when the host/player interaction occurs in the named room. |

## Command Types Reference

The code ordering is checked against the enum in `src/quest/hlquest.h` and the
persisted code table in `src/olc/hlqedit.c`.

| Index | Macro | Code | Meaning | Runtime legality |
|-------|-------|------|---------|------------------|
| 0 | QUEST_COMMAND_COINS | `C` | Coins | GIVE input or output. |
| 1 | QUEST_COMMAND_ITEM | `I` | Item | GIVE input or output. |
| 2 | QUEST_COMMAND_LOAD_OBJECT_INROOM | `O` | Load object in room | Output only. |
| 3 | QUEST_COMMAND_LOAD_MOB_INROOM | `M` | Load mobile in room | Output only. |
| 4 | QUEST_COMMAND_ATTACK_QUESTOR | `A` | Attack questor | Output only; parameters unused. |
| 5 | QUEST_COMMAND_DISAPPEAR | `D` | Disappear | Output only; parameters unused. |
| 6 | QUEST_COMMAND_TEACH_SPELL | `T` | Teach spell or skill | Output only. |
| 7 | QUEST_COMMAND_OPEN_DOOR | `X` | Open door | Output only. |
| 8 | QUEST_COMMAND_FOLLOW | `F` | Follow questor | Output only; parameters unused. |
| 9 | QUEST_COMMAND_KIT | `K` | Change kit or perform lich transition | Output only. |
| 10 | QUEST_COMMAND_CHURCH | `U` | Set church | Output only. |
| 11 | QUEST_COMMAND_CAST_SPELL | `S` | Cast spell | Output only. |
| 12 | QUEST_COMMAND_QUEST_POINTS | `P` | Award or deduct quest points | Output only. |
| 13 | QUEST_COMMAND_EXPERIENCE | `E` | Award experience | Output only. |

Only COINS and ITEM input commands on GIVE entries are consumed by the
runtime. Other input commands, and all ROOM input commands, are ignored and
are errors even though the loader can retain them. Every defined command may
appear as output on GIVE or ROOM entries.

## Physical and Runtime Ordering

Ordering is behaviorally significant. The loader prepends each entry to the
host's list. It also prepends input commands within a GIVE/ROOM entry, while it
appends output commands.

For this physical file order:

```text
A
first~
First reply.~
A
second~
Second reply.~
Q
Give reply.~
I C 10 0
I I 10001 0
O C 20 0
O I 10002 0
S
```

the effective runtime order is:

- entries: GIVE, second ASK, first ASK;
- GIVE inputs: item 10001, then 10 coins; and
- GIVE outputs: 20 coins, then item 10002.

`wtool show hlquest <host-vnum>` reports both physical and runtime ordinals.
Do not reorder a file merely to make the display ascending: confirm intended
first-match and execution behavior in the runtime view.

## Command Values and Locations

| Code | `value` contract | `location` contract |
|------|------------------|---------------------|
| `C` | Non-negative coins through `MAX_GOLD` (2140000000). | Unused; canonical value is `0`. |
| `I` | Existing object VNUM. | Unused; canonical value is `0`. |
| `O` | Existing object VNUM. | Existing room VNUM, or `0` for the current room. |
| `M` | Existing mobile VNUM. | Existing room VNUM, or `0` for the current room. |
| `A` | Unused; canonical value is `0`. | Unused; canonical value is `0`. |
| `D` | Unused; canonical value is `0`. | Unused; canonical value is `0`. |
| `T` | Runtime-safe spell/skill `1..527` (`SPELL_RESERVED_DBC + 1` through `NUM_SPELLS - 1`). | Unused; canonical value is `0`. |
| `X` | Direction `0..5` in the current source-derived direction table. | Existing room VNUM with an exit in that direction. |
| `F` | Unused; canonical value is `0`. | Unused; canonical value is `0`. |
| `K` | Target class `0..37`, or `9999` for the lich transition. | Prerequisite class `0..37`, or `9999` for the lich transition. |
| `U` | Church `0..12`. | Unused; canonical value is `0`. |
| `S` | Runtime-safe spell/skill `1..527`. | Unused; canonical value is `0`. |
| `P` | Signed quest-point delta `-100000000..100000000`. | Unused; canonical value is `0`. |
| `E` | Non-negative experience through `2140000000`. | Unused; canonical value is `0`. |

The numeric values shown above are the current generated manifest values.
`wtool` reads their source-derived limits and direction count; run
`constants sync --check` after source changes. HLQEDIT currently accepts spell
values up to a wider editor constant, but runtime execution indexes the
smaller `NUM_SPELLS` table. The validator uses the runtime-safe range as the
error boundary.

Coin values above `MAX_GOLD`, negative coins, unsafe spell or class indexes,
invalid directions, missing door exits, invalid churches, out-of-range
quest-point deltas or experience awards, and negative load locations are errors.
Unused nonzero parameters are warnings. Quest-point awards saturate the player's
balance at `0..100000000`; experience awards use the normal quest-mode experience
path. GIVE completion consumes the exact configured coin amount and counts
repeated ITEM inputs as separate required copies.

## Typed References

The validator resolves and exposes these graph edges:

- host block to its mobile;
- ROOM entry to its room;
- ITEM input/output and load-object commands to objects;
- load-mobile commands to mobiles;
- nonzero object/mobile load destinations to rooms; and
- open-door commands to their target rooms.

Missing records and wrong record types are errors with host, physical entry,
and command context. Spell, class, church, direction, coin, and the `9999`
lich sentinel are scalar domains rather than invented world-record types.

## Loader Hazards

Hand-edited malformed HLQ content can trigger behavior that is more severe
than a normal rejected record:

- a missing host mobile can be dereferenced during boot;
- GIVE or ROOM before a host can dereference a null host;
- invalid ROOM numeric input can reuse stale parser state;
- an unknown command code can leave an uninitialized command type;
- an invalid input/output direction is discarded after loading;
- an unknown top-level marker truncates the rest of the file; and
- missing `S` or command-chain EOF leaves parsing in an unsafe state.

The validator reports these without trying to execute the loader or repair
the file.

## Builder Validation Loop

After saving with `hlqedit`, validate the owning package, then inspect the
host block and both directions of its graph:

```sh
python3 scripts/world/wtool.py validate --zone 100
python3 scripts/world/wtool.py show hlquest 10001
python3 scripts/world/wtool.py refs hlquest 10001
```

Use `hlq` as a supported short alias. Duplicate host blocks are shown as
separate matches rather than merged. For a staged file:

```sh
python3 scripts/world/wtool.py validate --paths staged/100.hlq
```

The validator is read-only and does not create a missing `index.mini`, add a
package to an index, or rewrite ordering. See
`docs/utilities/WORLD_VALIDATOR_CLI.md` for modes, exit status, JSON, and the
complete finding-code inventory.

## Publication Policy

This technical flat-file reference is maintained as Markdown and enforced by
`wtool docs --check`. It is intentionally not added to
`scripts/development/generate-web-guides.sh`; that script's curated HTML set
continues to cover OEDIT and the room/mobile flag guides.
