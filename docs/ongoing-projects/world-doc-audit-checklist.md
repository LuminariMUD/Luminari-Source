# World-Building Documentation Audit Checklist

Status: **complete**. Findings were gathered against the source tree on
2026-08-04 and worked the same day. Every box below is closed. The mechanical
verification in section 5 re-runs clean.

Companion document: [world-validator-cli-plan.md](world-validator-cli-plan.md).
Section 6 records how this work was sequenced against that plan, and what
changed about the plan's premise as a result.

## 1. Documents in scope

| Document | Verdict |
|---|---|
| [ROOM_FLAGS.md](../world_game-data/ROOM_FLAGS.md) | Fixed. All 42 bits documented; every citation regenerated. |
| [MOB_FLAGS.md](../world_game-data/MOB_FLAGS.md) | Fixed. All 105 bits documented; count corrected from 101. |
| [OEDIT_GUIDE.md](../world_game-data/OEDIT_GUIDE.md) | Rewritten in parts. Now covers all item types, wear slots, extra flags, and the value vectors. |
| [builder_manual.md](../world_game-data/builder_manual.md) | Fixed. `tedit`/`trigedit` confusion, `zedit new` syntax, reset command list. |
| [gear_guide.md](../world_game-data/gear_guide.md) | Fixed. `APPLY_MANA` corrected to `APPLY_PSP`; armor value section rewritten. |
| [wilderness_system.md](../world_game-data/wilderness_system.md) | Fixed. Configuration constants re-derived; resource map symbols corrected; `C` line format added. |
| [CRAFTING_SYSTEM_NOTES.md](../world_game-data/CRAFTING_SYSTEM_NOTES.md) | Renamed from `crafting_notes_old.md`; skill numbers corrected; status banner added. |
| [STARTER_AREA.md](../world/STARTER_AREA.md) | Fixed. Tracked-bundle location corrected. |
| [OLC_ONLINE_CREATION_SYSTEM.md](../systems/OLC_ONLINE_CREATION_SYSTEM.md) | Fixed. Command reference added; fabricated code samples flagged. |
| [OLC_SpecProcs.md](../guides/OLC_SpecProcs.md) | ASCII cleanup only. Content was already accurate. |
| `docs/web/guides/*.html` | Now generated. See item 2.1. |

New documents written to close section 4:

| Document | Purpose |
|---|---|
| [ZONE_FILE_FORMAT.md](../world_game-data/ZONE_FILE_FORMAT.md) | `.zon` header fields, all reset commands, door states, parser gotchas |
| [SHOP_FILE_FORMAT.md](../world_game-data/SHOP_FILE_FORMAT.md) | `.shp` field-by-field reference |
| [BUILDER_QUICKSTART.md](../world_game-data/BUILDER_QUICKSTART.md) | One zone from empty to bootable |

## 2. Cross-cutting items

### 2.1 The HTML mirror is a duplicate with no generator - **[done]**

- [x] Model decided: **generate from Markdown.** `README_WEB.md` already
      documented the pandoc invocation; what was missing was a committed script.
- [x] `scripts/development/generate-web-guides.sh` written. Regenerates all
      three pages; `--check` exits non-zero and names stale pages without
      writing anything.
- [x] Recorded in `docs/web/README_WEB.md`, which now states plainly that the
      HTML is build output and must not be hand-edited, and lists the
      source-to-output mapping.
- [x] All three pages regenerated from the corrected Markdown.

### 2.2 Non-ASCII bytes - **[done]**

- [x] All five files cleaned: arrows to `->`, smart quotes to straight quotes,
      en/em dashes to `-`, degree signs spelled out, box-drawing characters in
      the `builder_manual.md` directory tree replaced with `|--` / `` `-- ``.
- [x] The `wilderness_system.md` resource-density table used block characters
      and emoji while claiming to describe "ASCII maps". It was not an encoding
      problem: **the symbols were wrong.** Replaced with the real ones from
      `get_resource_map_symbol_with_coords()` - `#`, `*`, `+`, `.`, `,`, space -
      along with the real thresholds, and a note that the symbol and color
      ladders use different cutoffs.
- [x] All files in scope now report `ASCII text`; no CR bytes anywhere.

### 2.3 Source-path references - **[done]**

- [x] `vessels_src.c` did not exist. `ROOM_VEHICLE`'s references now point at
      `src/vessels/vessels_rooms.c` with the real functions, plus a note that
      the docking and movement code never reads the flag.
- [x] **The line numbers had drifted wholesale.** Of 103 `file.c:line`
      citations checked across both flag docs, **93 did not land anywhere near
      the flag they claimed to document.** All 104 citations were regenerated as
      path-qualified file plus enclosing function name - a form that does not
      rot. Zero bare line numbers remain in either document.
- [x] Stale `src/structs.h` line ranges in both overviews replaced with
      references to the define block by name.
- [x] No doc in the set references a phantom source file.

### 2.4 Dead campaign references - **[done]**

- [x] The `#ifdef CAMPAIGN_FR` sample at `wilderness_system.md:329` removed.
- [x] Swept the rest of the set for `CAMPAIGN_`, DragonLance, and Forgotten
      Realms framed as a build-time variant. That was the only occurrence.

## 3. Per-document items

### 3.1 ROOM_FLAGS.md - **[done]**

- [x] Bit 2 `No-Mob` added, with its real behavior: it stops mobiles choosing
      the room as a wander destination, and does *not* prevent placement by
      zone reset, summon, or `transfer`.
- [x] All 42 `(Index: N)` values verified against `src/structs.h`. Zero
      mismatches.
- [x] Bit 15 `ROOM_BFS_MARK` confirmed described as a system flag, not invented.
- [x] `vessels_src.c` path fixed.
- [x] Quick reference table gained an **OLC Display Name** column. The docs used
      constant names throughout while the in-game menus show display names, so
      searching the doc for `No-Mob` previously found nothing.

### 3.2 MOB_FLAGS.md - **[done]**

- [x] All four missing flags added with real code references: **101
      `Custom-Mob-Stats`**, **102 `No-Block-Bypass`**, **103 `Golem`**,
      **104 `No-Teleport`**.
- [x] The overview claimed "101 mobile flags (indices 0-100)". Corrected to 105
      (0-104, `NUM_MOB_FLAGS`).
- [x] All 105 `(Index: N)` values verified. Zero mismatches.
- [x] Bit 20 `!DEAD!` and bit 96 `UNUSED-96` confirmed already marked unusable.
- [x] Warning added about `MOB_*` defines that are not flags. It names the
      collision explicitly: `MOB_BLOCK_E` is bit 46 and `MOB_DIRE_SPIDER` is
      vnum 46, and nothing in either name distinguishes them. Points readers at
      `action_bits[]` as the authoritative list.
- [x] Quick reference table gained the same display-name column.

### 3.3 OEDIT_GUIDE.md - **[done]**

- [x] Both missing extra flags added: **114 `Costs-Account-Experience`**
      (`ITEM_ACCOUNT_EXP`) and **115 `Can-Be-Reforged`** (`ITEM_REFORGEABLE`).
      Total corrected from 114 to 116.
- [x] **Full item type reference added** - all 58 types with constant names and
      per-type notes. Was 24 of 58.
- [x] **Full wear slot reference added** - all 34 bits. Was 4 of 34. Includes
      the nine crafting-tool slots and an explanation of why `(Takeable)` is not
      a slot.
- [x] **Object value reference written.** Per-type layout of `value[0]` through
      `value[4]` for every type that uses them, which existed nowhere before.
- [x] Documented two things the source makes easy to get wrong:
      - The editor's `ValueN` label is one higher than the `value[N-1]` slot it
        writes.
      - `parse_object()` accepts **exactly 4 or exactly 16** integers on the
        value line and aborts the boot on anything else. Because enhancement
        bonuses live in `value[4]`, the 4-value form cannot express them - a
        silent, common failure.
- [x] Documented that the extra-flag and wear-flag menus prompt for `bit + 1`
      while the tables list the bit. The `#` column previously read as the
      number to type, and it is not.
- [x] Coverage verified mechanically: 58/58 item types, 34/34 wear bits,
      116/116 extra flags all appear in the document.

### 3.4 OLC_ONLINE_CREATION_SYSTEM.md - **[done]**

- [x] Command reference section added near the top, covering every editor
      (`redit`, `oedit`, `medit`, `zedit`, `sedit`, `trigedit`, `qedit`,
      `hedit`, `aedit`, `iedit`, `bedit`) and every listing command (`rlist`,
      `mlist`, `olist`, `slist`, `tlist`, `qlist`, `zlist`, `blist`,
      `pathlist`), with the shared argument forms.
- [x] Recorded that `plist` is not part of the family - it lists players.
- [x] `hedit` and `aedit` cross-referenced rather than duplicated.
- [x] **Found while working: the `ZCMD_*` constants the document presented as
      the zone command set do not exist.** Replaced with the real
      `struct reset_com` and a table of the actual command characters.
- [x] **21 identifiers in the document's code samples do not exist under
      `src/`**, including `olc_save_to_disk()`, `validate_room_data()`,
      `create_new_room()`, and the `massroomset` command described at the end,
      which is not registered in `cmd_info[]`. A banner now states that the
      listings are illustrative and points at the real implementation.

### 3.5 STARTER_AREA.md - **[done]**

- [x] Zone range claim re-confirmed correct.
- [x] Location corrected. The checklist's own note was also wrong: the bundle is
      **flat** at `lib/world/minimal/` (`0.wld`, `0.zon`, ...), not
      `lib/world/minimal/zon/0.zon`. Documented the layout, that `setup.sh`
      copies it into the ignored live directories, and why the live files are
      not in version control.
- [x] En dash fixed.

### 3.6 builder_manual.md, gear_guide.md, wilderness_system.md - **[done]**

- [x] `builder_manual.md`:
      - **`tedit` was listed as the trigger editor.** It edits the server's
        static text files (`motd`, `news`, `policies`). Corrected, with
        `trigedit` and `qedit` added.
      - `zedit new` documented as taking no arguments; it requires
        `<zone number> <bottom-room> <upper-room>`.
      - Reset command list was missing `T`, `V`, `J`, `I`, `L`, and `S`, and
        omitted the if-flag and load-percentage arguments entirely. All added.
      - Listing commands and the real in-editor save flow added.
      - Every backticked command token checked against `cmd_info[]`: clean.
- [x] `gear_guide.md`:
      - `APPLY_MANA` does not exist; the constant is `APPLY_PSP` (12).
        Corrected. All other apply constants verified correct.
      - The armor value section was vague and hedged ("or swapped with Value
        2"). Rewritten against `oedit.c` and `utils.h`, and cross-linked to the
        new value reference.
- [x] `wilderness_system.md`:
      - **The configuration section was largely fabricated.** Of 27 documented
        constants, 16 did not exist, and 6 of the 11 that did had wrong values
        (`PUBSUB_VERSION` 3 vs 1, `PUBSUB_QUEUE_BATCH_SIZE` 10 vs 50,
        `SUBSCRIPTION_CACHE_SIZE` 256 vs 1024,
        `PUBSUB_MAX_TOPIC_NAME_LENGTH` 64 vs 255,
        `PUBSUB_MAX_HANDLER_NAME_LENGTH` 32 vs 64,
        `PUBSUB_PRIORITY_NORMAL` 5 vs 2). All three blocks replaced with the
        real headers, plus a note that `NUM_RESOURCE_TYPES` and the subtype
        counts are enum terminators rather than macros.
      - The `.wld` `C` block was undocumented. Added, with a worked example and
        the note that it is a two-line construct read by a bare `sscanf`, so a
        malformed second line does not error.
      - Coordinate range and all wilderness vnum constants verified correct.
      - Illustrative-code banner added for the four named functions that do not
        exist.

### 3.7 crafting_notes_old.md - **[done]**

- [x] Fate decided: **kept and renamed**, not retired. There is no other
      crafting document to fold it into, and the resize chart and material
      mapping have no other home. Moved to `CRAFTING_SYSTEM_NOTES.md` with
      `git mv`, so the `_old` trap is gone.
- [x] **Every skill number in it was wrong** - it listed 471-485, from before
      skills moved to the `START_SKILLS` 2000 base. All fifteen corrected to
      2071-2085 and reorganized into tables.
- [x] Status banner added marking it design notes rather than a system
      reference, and naming what it does not cover.
- [x] All eight crafting commands verified present in `cmd_info[]`.
- [x] Master index entry updated.

## 4. Missing documents - **[done]**

- [x] **[ZONE_FILE_FORMAT.md](../world_game-data/ZONE_FILE_FORMAT.md)** - the
      numeric header, all four accepted field counts, zone flags, every reset
      command with its arity, all 17 door states, and the parser gotchas.
- [x] **Object value-vector reference** - written into `OEDIT_GUIDE.md` rather
      than as a separate file, since it is inseparable from the item types.
- [x] **[SHOP_FILE_FORMAT.md](../world_game-data/SHOP_FILE_FORMAT.md)** -
      field-by-field, including the `v3.0` tag that silently changes how the
      lists are parsed.
- [x] **[BUILDER_QUICKSTART.md](../world_game-data/BUILDER_QUICKSTART.md)** -
      one zone from empty to bootable, both by hand and via OLC, with a table
      of first-boot error messages and their causes.

## 5. Verification procedure

All re-run clean on 2026-08-04. Scripts used are throwaway; the checks are:

- **Flag coverage and index correctness** - extract `room_bits[]` and
  `action_bits[]` from `src/constants.c`, extract every `(Index: N)` heading
  from the doc, diff both directions, and confirm each documented index matches
  the `#define`. Handle ranged headings (`MOB_BLOCK_N through MOB_BLOCK_D
  (Index: 45-54)`). Result: 42/42 and 105/105, zero mismatches, every display
  name present.
- **Path validity** - for every `*.c` / `*.h` citation, confirm the basename
  exists under `src/`. Result: clean. Two apparent hits are regex artifacts
  (`index.hlq` truncated, and the `*edit.c` glob).
- **Identifier validity** - for every identifier named in a doc's code blocks,
  confirm it exists somewhere under `src/`. This is what surfaced the fabricated
  OLC functions and wilderness constants. Result: clean except the four
  wilderness functions now covered by an explicit banner.
- **Command validity** - for every command named in a doc, confirm a matching
  string literal in `src/interpreter.c`. Result: clean.
- **Encoding** - `file` reports `ASCII text` for every doc in scope; no CR
  bytes. Result: clean.
- **HTML sync** - `./scripts/development/generate-web-guides.sh --check`.
  Result: all guide pages current.

## 6. Sequencing against the validator plan

The original plan was to defer the five missing flag entries until
`wtool` Phase 4 could generate them. **They were done by hand instead**, for a
reason that changes the case for that phase:

The flag *tables* were never the problem. All 147 documented indices were
correct before this pass; only five entries were absent. What was actually
broken was the surrounding prose - 93 of 103 line-number citations pointed at
the wrong place, and the OLC and wilderness documents contained substantial
invented content. **No generator would have caught any of that.**

Worth carrying into the `wtool` work:

- Phase 4's flag-table generation remains worth doing, but as
  drift-*prevention*, not as a fix for present damage. The value is lower than
  the plan implies.
- The generator pattern that *did* pay off here is the one now living in
  `scripts/development/generate-web-guides.sh`: a `--check` mode that fails
  loudly when a derived artifact falls out of sync. `wtool` should have the
  same, and it is cheap.
- The checks in section 5 above are the ones that found real defects. They are
  a better specification for `wtool`'s doc-linting surface than the flag-table
  generation the plan currently leads with. In particular, "does every
  identifier named in a code block exist in the tree" found more real errors
  than everything else combined.
- Several source bugs surfaced while writing the format references. They are
  filed in [known-issues.md](../known-issues.md) and described from the
  builder's side in `ZONE_FILE_FORMAT.md`: the unreachable `case 'I'` and
  `case 'R'` in `load_zones()`, the entirely non-functional `L` command, the
  silent zone-header field-count degradation, the column-0 prescan desync, and
  `renum_world()` nulling dangling exits without a log line. `wtool` should
  detect all five in world data.

## 7. Definition of done

- [x] All section 2 and 3 checkboxes closed. Nothing deferred.
- [x] Section 4 documents written and indexed in
      [TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md).
- [x] Section 5 verification re-run clean.
- [x] Findings that turned out to be source bugs rather than doc bugs filed in
      [known-issues.md](../known-issues.md).
- [x] Outcome recorded in [CHANGELOG.md](../CHANGELOG.md).

This working file can be deleted once the outcome above has been reviewed. It
is retained for now as the record of what was checked and what was found.
