# Quest File Format (`.qst`)

This guide documents the Luminari quest flat-file format loaded from
`lib/world/qst`. It describes the grammar accepted by the current server, the
canonical form written by QEDIT, and the checks performed by the read-only
world validator.

The authoritative implementation is split across:

- `src/quest/quest.h` for the record, type, flag, and string-size contracts;
- `src/quest/quest.c` for `parse_quest()` and runtime quest behavior;
- `src/olc/genqst.c` for the canonical `save_quests()` writer; and
- `src/olc/qedit.c` for `qedit_parse()` and editor bounds.

This is the Luminari format only. Historical campaign variants with an extra
kill-list string are not supported by this repository.

## Files, Indexes, and Packages

Normal boot reads `lib/world/qst/index`; mini-mud boot reads
`lib/world/qst/index.mini`. Each significant index line names one `.qst`
package, and `$` terminates the index. Package files normally use the zone
number, for example `100.qst`.

Quest identity comes from the `#<vnum>` record header. QEDIT saves a quest to
the package that owns its VNUM. Records must be in increasing VNUM order and a
VNUM must occur only once across the selected packages. An empty dataset is
legal when its selected index exists and terminates correctly.

## Record Grammar

The canonical writer emits:

```text
#<quest-vnum>
<name>~
<description>~
<accept-message>~
<completion-message>~
<quit-message>~
<type> <questmaster> <flags> <target> <previous> <next> <prerequisite-object>
<points> <quit-penalty> <min-level> <max-level> <time> <return-mobile> <quantity>
<gold> <experience> <reward-object> <race> <x> <y> <follower-mobile>
D
<diplomacy-dc> <intimidate-dc> <bluff-dc> <alternative-quest>
S
```

The package ends with `$~`. The loader also accepts `$` because it tests only
the first character of the terminator line.

Blank lines and lines beginning with `*` are ignored where the shared server
line reader accepts them. The five strings are tilde-terminated and may span
physical lines. A literal `~` ends the field.

The first and second numeric rows require exactly seven converted fields. The
reward row accepts either the current seven-field form shown above or this
legacy three-field form:

```text
<gold> <experience> <reward-object>
```

In the legacy form, race, coordinates, and follower reward retain their
loader defaults. `D` is optional and may appear before `S`; the current writer
always emits it. Multiple `D` blocks are unsafe because each later block
silently replaces the earlier dialogue values. Any other extension marker is
invalid and can leave the server parser on the same input indefinitely.

Use `-1` for absent optional VNUM fields and for the documented scalar
sentinels. Do not substitute `0`: it can be a real value or reference in
several fields.

## Field Reference

| Field | Meaning |
|-------|---------|
| `quest-vnum` | Unique quest identity and lookup number. |
| `name` | Short builder-facing quest name. |
| `description` | Player-facing quest description. |
| `accept-message` | Text displayed when the player joins. |
| `completion-message` | Text displayed on completion. |
| `quit-message` | Text displayed when the player abandons the quest. |
| `type` | Source-defined quest type, or `AQ_UNDEFINED` (`-1`) for an unavailable record. |
| `questmaster` | Mobile VNUM that offers the quest; `-1` means none. |
| `flags` | One ASCII bitvector token. This is not a four-token world bitvector. |
| `target` | Type-dependent object, room, mobile, scalar, or unused value. |
| `previous` | Quest that must already be complete; `-1` means none. |
| `next` | Quest installed after completion; `-1` means none. |
| `prerequisite-object` | Object the player must possess; `-1` means none. |
| `points` | Quest points awarded on completion. |
| `quit-penalty` | Quest points removed when the player quits. |
| `min-level`, `max-level` | Inclusive player-level range. |
| `time` | Time limit; `-1` means no limit. |
| `return-mobile` | Recipient for object-return and give-gold quests; otherwise normally `-1`. |
| `quantity` | Required count. |
| `gold`, `experience` | Scalar completion rewards. |
| `reward-object` | Object reward VNUM; `-1` means none. |
| `race` | Race-change reward; only the documented sentinel values are valid. |
| `x`, `y` | Persisted wilderness coordinates. |
| `follower-mobile` | Mobile reward/follower VNUM; `-1` means none. |
| dialogue DCs | Diplomacy, intimidate, and bluff thresholds; `-1` disables a check. |
| `alternative-quest` | Dialogue failure/alternative quest; `-1` means none. |

At boot, `assign_the_quests()` saves the questmaster mobile's current callback as one runtime-only
secondary before installing `questmaster`. Because shop assignment runs first, an overlapping mobile
uses `questmaster -> shop_keeper -> original callback`. The `.qst` file persists the questmaster VNUM,
not a special-procedure chain.

## Quest Types Reference

This table is checked against `src/quest/quest.h` and the display table in
`src/quest/quest.c` by `wtool docs --check`.

| Index | Macro | Display name | Target contract |
|-------|-------|--------------|-----------------|
| 0 | AQ_OBJ_FIND | Acquire Object | Object VNUM. |
| 1 | AQ_ROOM_FIND | Find Room | Room VNUM. |
| 2 | AQ_MOB_FIND | Find Mob | Mobile VNUM. |
| 3 | AQ_MOB_KILL | Kill Mob | Mobile VNUM. |
| 4 | AQ_MOB_SAVE | Save Mob | Mobile VNUM. |
| 5 | AQ_OBJ_RETURN | Return Object | Object VNUM; return mobile required. |
| 6 | AQ_ROOM_CLEAR | Clear Room | Room VNUM. |
| 7 | AQ_AUTOCRAFT | Complete a Supplyorder | Crafting event; no typed target. |
| 8 | AQ_CRAFT | Craft Item | Crafting event; no typed target. |
| 9 | AQ_CRAFT_RESIZE | ReSize Item | Crafting event; no typed target. |
| 10 | AQ_CRAFT_DIVIDE | Divide Item | Crafting event; no typed target. |
| 11 | AQ_CRAFT_MINE | Mine Crafting Mat | Crafting event; no typed target. |
| 12 | AQ_CRAFT_HUNT | Hunt for Crafting Mat | Crafting event; no typed target. |
| 13 | AQ_CRAFT_KNIT | Knit Crafting Mat | Crafting event; no typed target. |
| 14 | AQ_CRAFT_FOREST | Forest for Crafting Mat | Crafting event; no typed target. |
| 15 | AQ_CRAFT_DISENCHANT | Disenchant Item | Crafting event; no typed target. |
| 16 | AQ_CRAFT_AUGMENT | Augment Item | Crafting event; no typed target. |
| 17 | AQ_CRAFT_CONVERT | Convert Item | Crafting event; no typed target. |
| 18 | AQ_CRAFT_RESTRING | ReString Item | Crafting event; no typed target. |
| 19 | AQ_COMPLETE_MISSION | Complete a Mission | Mission-difficulty index. |
| 20 | AQ_HOUSE_FIND | Find a Player House | Current private house; no target. |
| 21 | AQ_WILD_FIND | Get to Wilderness Coordinates | Uses persisted `x` and `y`. |
| 22 | AQ_GIVE_GOLD | Give Gold | Non-negative gold threshold; return mobile required. |
| 23 | AQ_MOB_MULTI_KILL | Kill Multi Mob (comma-separated vnums) | Not persistable in the Luminari grammar. |
| 24 | AQ_DIALOGUE | Dialogue Quest | Mobile VNUM plus dialogue fields. |

`AQ_UNDEFINED` is `-1`, outside the indexed display table. It is an explicit
unavailable state and does not by itself produce a validator finding.

## Quest Flags Reference

The serialized token uses the normal ASCII bit notation, but only one token
is present on the numeric row.

| Bit | Macro | Display name | Meaning |
|-----|-------|--------------|---------|
| 0 | AQ_REPEATABLE | REPEATABLE | The quest may be completed repeatedly. |
| 1 | AQ_REPLACE_OBJ_REWARD | REPLACE-OBJ-REWARD | A lost object reward may be acquired again. |

List and inspect the live source-derived flag contract with:

```sh
python3 scripts/world/wtool.py flags list quest
python3 scripts/world/wtool.py flags decode quest <token>
python3 scripts/world/wtool.py flags encode quest AQ_REPEATABLE
```

## Scalar and String Bounds

QEDIT enforces or truncates to these source-backed limits. The validator
reports values that hand-edited files can place outside them.

| Field | Valid persisted value |
|-------|-----------------------|
| Name | At most 39 bytes (`MAX_QUEST_NAME - 1`). |
| Description | At most 74 bytes (`MAX_QUEST_DESC - 1`). |
| Accept, completion, quit messages | At most 4095 bytes each (`MAX_QUEST_MSG - 1`). |
| Quantity | `1..50`. |
| Completion points | `0..999999`. |
| Quit penalty | `0..999999`. |
| Minimum and maximum level | `0..LVL_IMPL`, currently `0..34`; minimum must not exceed maximum. |
| Time limit | `-1..100`. |
| Gold reward | `0..99999`. |
| Experience reward | `0..999999`. |
| Dialogue DC | `-1..100`; a dialogue quest needs at least one positive DC. |
| Mission difficulty target | `0..NUM_MISSION_DIFFICULTIES - 1`, currently `0..5`. |
| Race reward | `-1`, `RACE_LICH` (`45`), or `RACE_VAMPIRE` (`46`). |
| Give-gold target | Zero or greater. |

String limits count encoded bytes, matching storage behavior, rather than
displayed characters.

## Typed References and Topology

In addition to the type-specific target, the validator resolves the
questmaster, return mobile, prerequisite object, reward object, follower
mobile, previous quest, next quest, and dialogue alternative. Missing records
and wrong record types are errors with the exact field location.

`previous` gates whether a player can join. `next` can install a later quest
after completion. For a normal chain, each link should be reciprocal:

```text
quest 10001: previous=-1, next=10002
quest 10002: previous=10001, next=-1
```

A previous-link self-reference or cycle can make the chain impossible to
enter and is an error. A next-link self-reference or cycle is a warning
because completion history can eventually stop it, but it can reinstall an
earlier stage. Non-reciprocal links are warnings.

A dialogue alternative must not reference itself. The alternative quest must
name the dialogue quest as its `previous` quest, or players cannot join it as
the intended branch. Dialogue fields on non-dialogue quests are ignored or
produce an unusable alternative and are diagnosed.

## Known Format Limitation

`AQ_MOB_MULTI_KILL` uses an in-memory comma-separated mobile list, but the
Luminari loader and writer do not read or write that field. A `.qst` record of
this type cannot survive an edit/reboot cycle with its intended targets.
`wtool` therefore reports it as an error. Adding a disk field requires a
separate server-format change; the validator never invents or repairs one.

## Builder Validation Loop

After saving with `qedit`, run the smallest useful read-only check and then
inspect the record and its graph:

```sh
python3 scripts/world/wtool.py validate --zone 100
python3 scripts/world/wtool.py show quest 10001
python3 scripts/world/wtool.py refs quest 10001
```

Use `qst` as a supported short alias. For a staged file that is not under the
world root:

```sh
python3 scripts/world/wtool.py validate --paths staged/100.qst
```

The validator does not write, reformat, or add files to an index. See
`docs/utilities/WORLD_VALIDATOR_CLI.md` for exit status, JSON output, and the
complete finding-code inventory.

## Publication Policy

This technical flat-file reference is maintained as Markdown and enforced by
`wtool docs --check`. It is not routed through
`scripts/development/generate-web-guides.sh`, whose curated HTML set remains
the OEDIT and room/mobile flag guides.
