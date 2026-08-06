# Mudlet MSDP JSON Decoder Investigation

Date: 2026-08-05

Status: Server remediation complete; LuminariGUI remediation in progress; Mudlet finding remains external

## Purpose

This document records the investigation of three Mudlet errors observed while the current
LuminariGUI package was connected to LuminariMUD:

```text
[  LUA  ] - object: <JSON decoder error:> function:<json_to_value>
            <Lua error:InvalidJSONInput: lexical error: invalid character inside string.
                                                   "        cTrue Neutral        n"
                                 (right here) ------^

[  LUA  ] - object: <JSON decoder error:> function:<json_to_value>
            <Lua error:InvalidJSONInput: lexical error: invalid character inside string.
                                                   "        Wthe Vault of Ages        n"
                                 (right here) ------^
```

The Vault error appeared twice. The displayed spaces are literal horizontal-tab bytes expanded
by the error display. They are not ordinary runs of spaces.

The diagnostic phase was read-only. It inspected the development checkout, the public
LuminariGUI source, the package currently served by the documented stable download URL, the
current public Mudlet source, and the MSDP protocol references. It did not connect to or alter
production.

## Remediation Scope

Implementation began on 2026-08-05 in the development checkout. The active scope is limited
to LuminariMUD server behavior and server-owned tests and documentation:

- MJD-001: remove internal color markup from the three affected scalar values;
- MJD-004: make the negotiated MSDP-over-GMCP path emit valid JSON for every supported MSDP
  value shape, or explicitly decline unsupported values; and
- MJD-005: add regression coverage for the native and GMCP compatibility contracts.

MJD-002 (Mudlet core) remains a documented external finding. MJD-003 (LuminariGUI
subscriptions) was originally excluded from the server-only remediation, but package-owned
follow-up began on 2026-08-06 in the LuminariGUI repository. It remains separate from server
closure and does not require a production connection or production change.

## Snapshot and Provenance

The conclusions below were verified against these snapshots:

| Component | Inspected snapshot |
|-----------|--------------------|
| LuminariMUD source | `47163fc7ac43f875925b34d049f1778c67c2c86e` |
| Checkout environment | `APP_ENV=development` |
| Hosted GUI package | `LuminariGUI` version `2.0.4.037` |
| Hosted package size | 7,045,345 bytes |
| Hosted package SHA-256 | `ce41c2fd4011d10c9c41da9865bbf91d51fe660a2a1aa9d43971b8ca270adc84` |
| Public LuminariGUI source | `ab433970d3124880b8090b88961e4055c0c2d495` |
| Public Mudlet development source | `e91b8596fd1b862f7e4d9a0079a0b1bc086a40fd` |

The hosted package passed `unzip -t`, and its `config.lua` identified it as version
`2.0.4.037`. Its Lua/XML contents match the relevant public LuminariGUI source paths cited
below.

## Executive Summary

At the inspected snapshot, the primary defect was at the LuminariMUD structured-data boundary.
Three native MSDP scalar values were sent with LuminariMUD's internal tab-based color markup
still embedded:

| Wire value | MSDP variable | Error count in the report |
|------------|---------------|---------------------------|
| `<TAB>cTrue Neutral<TAB>n` | `ALIGNMENT` | One |
| `<TAB>Wthe Vault of Ages<TAB>n` | `AREA_NAME` | One |
| `<TAB>Wthe Vault of Ages<TAB>n` | `ROOM_NAME` | One |

The count and text therefore map exactly to one failed value for each of the three variables.
The two Vault errors are not evidence of one unexplained duplicate decoder call: the room and
its zone deliberately have the same colored display name.

The custom LuminariGUI Lua is not the decoder that throws the error. The package requests all
three variables with native `sendMSDP("REPORT", ...)`. Mudlet core receives the native MSDP
frames, converts each scalar into temporary JSON by adding double quotes, and passes that JSON
to `json_to_value`. Mudlet escapes a literal double quote but does not escape a literal tab.
The resulting temporary JSON contains an unescaped control byte inside a JSON string and YAJL
rejects it.

Attribution depends on what is meant by "the GUI":

- LuminariMUD is the root source of the unusable application values and is the correct primary
  repair location.
- LuminariGUI exposes the defect by subscribing to three scalar variables that its current
  visible UI does not consume. This is a secondary subscription and compatibility issue, not
  the malformed-data producer.
- Mudlet core has an incomplete native-MSDP-to-JSON conversion. If "GUI" includes the Mudlet
  client runtime, the incident crosses both the server and client sides.

The current visible room display and mapper should continue to work. They consume the separate
`ROOM` table, and LuminariMUD already strips color codes from that table before sending it. The
three rejected scalar fields remain unset or stale in Mudlet.

## Deterministic Failure Chain

```text
world/alignment display source
  -> LuminariMUD represents color as TAB + selector
  -> ALIGNMENT, AREA_NAME, and ROOM_NAME retain those bytes
  -> LuminariGUI sends native MSDP REPORT requests for all three
  -> LuminariMUD sends complete native MSDP scalar frames verbatim
  -> Mudlet msdp2Lua() wraps each scalar in double quotes
  -> the literal TAB is copied into the temporary JSON string
  -> json_to_value / YAJL rejects the unescaped control byte
  -> Mudlet logs "invalid character inside string"
  -> the corresponding msdp table value is not updated
```

This failure occurs before any LuminariGUI event handler can sanitize or display the value.
Lua package code cannot catch the malformed scalar after decoding because no decoded value is
produced.

## Source Evidence

### 1. Alignment is a colored terminal string

`src/comm.c:5082` sends `ALIGNMENT` directly from
`get_align_by_num(GET_ALIGNMENT(ch))`:

```c
MSDPSetString(d, eMSDP_ALIGNMENT, get_align_by_num(GET_ALIGNMENT(ch)));
```

`src/utils.c:4839-4860` defines the returned strings. True Neutral is:

```c
return "\tcTrue Neutral\tn";
```

Both `\t` sequences in that C literal compile to byte `0x09`. The following `c` and `n` bytes
are LuminariMUD color selectors. They are meaningful to the normal terminal-output color
processor, but they are not displayable alignment data for an out-of-band client.

The nearby `TITLE`, `POSITION`, `RACE`, `CLASS`, `OPPONENT_NAME`, and `TANK_NAME` paths first
copy their display strings into writable buffers and call `strip_colors()`. `ALIGNMENT` omits
that established boundary treatment.

Changing `get_align_by_num()` globally would be the wrong repair because its terminal-output
callers expect colored text. The structured-data path should use a plain alignment name or
strip a local copy.

### 2. The room and zone names are intentionally colored world data

`lib/world/artifacts/1699.wld:2` contains the room name:

```text
@Wthe Vault of Ages@n~
```

`lib/world/artifacts/1699.zon:3` contains the zone display name:

```text
@Wthe Vault of Ages@n~
```

The world files themselves contain ordinary `@` bytes, not tabs. At boot:

- `fread_string()` calls `parse_at()` for room strings at `src/db.c:6003-6007`.
- `load_zones()` calls `parse_at(Z.name)` at `src/db.c:4188-4192`.
- `parse_at()` changes each single `@` into a literal tab so the normal output color system can
  process the following selector.

The runtime values are consequently:

```text
0x09 W t h e ... s 0x09 n
```

`src/comm.c:5199-5201` passes both runtime pointers directly to `MSDPSetString()`:

```c
MSDPSetString(d, eMSDP_AREA_NAME, zone_table[GET_ROOM_ZONE(IN_ROOM(ch))].name);
MSDPSetString(d, eMSDP_ROOM_NAME, world[IN_ROOM(ch)].name);
```

No writable copy is made and `strip_colors()` is not called. The world data is correct for its
normal terminal purpose; removing colors from the `.wld` or `.zon` records would hide the
boundary bug rather than repair it.

### 3. The complete ROOM table already follows the correct policy

`update_msdp_room()` builds the structured `ROOM` table from the same room and zone names. At
`src/comm.c:5038-5039`, it does this before storage:

```c
strip_colors(buf2);
MSDPSetTable(ch->desc, eMSDP_ROOM, buf2);
```

This explains why the mapper and room panel can continue working while the scalar `ROOM_NAME`
and `AREA_NAME` updates fail. It also demonstrates an existing server policy: ordinary OOB
room data is intended to be plain text, not internal terminal markup.

### 4. MSDP storage and native serialization preserve the tabs

`ValidateMSDPValue()` at `src/net/protocol.c:567-603` validates string length and configured
minimum/maximum length. It does not inspect or normalize control bytes. This lets the literal
tabs enter the per-descriptor MSDP variable store unchanged.

`MSDPSend()` at `src/net/protocol.c:1582-1655` then interpolates string values directly into a
native MSDP frame:

```c
Written = snprintf(MSDPBuffer, sizeof(MSDPBuffer),
                   "%c%c%c%c%s%c%s%c%c", IAC, SB, TELOPT_MSDP,
                   MSDP_VAR, VariableNameTable[aMSDP].pName, MSDP_VAL,
                   pProtocol->pVariables[aMSDP]->pValueString, IAC, SE);
```

Native MSDP does not use JSON quoting. A tab is not one of MSDP's reserved framing bytes, so
the frame remains structurally complete. The problem is the semantic leakage of a server-only
color convention and Mudlet's later conversion limitation, not a truncated Telnet frame.

The general validator has a related hardening gap: it also does not reject the actual MSDP
delimiter bytes from scalar string values. That is not involved in this incident, but a future
generic serializer should distinguish permitted scalar content from table/array framing.

### 5. LuminariGUI requests all three failing variables

The source-of-truth fragment
`theGUI/src/scripts/gui/40_msdp_protocol.xml:5-16` in the LuminariGUI repository includes:

```lua
GUI.MSDP_REPORT_VARS = {
  "CHARACTER_NAME", "RACE", "CLASS", "ALIGNMENT", "LEVEL",
  -- ...
  "ROOM", "AREA_NAME", "ROOM_EXITS", "ROOM_NAME", "ROOM_VNUM",
  "WORLD_TIME",
}
```

`GUI.requestMSDPReports()` iterates this list and calls:

```lua
sendMSDP("REPORT", var)
```

The package does not define or invoke `json_to_value`. That function belongs to Mudlet core.

The three subscriptions are not currently necessary for visible package output:

- `GUI.updatePlayer()` reads `msdp.ALIGNMENT` only into debug instrumentation. Its rendered
  player HTML displays name, level, class, race, attributes, armor class, and gold, but not
  alignment.
- `GUI.updateRoom()` reads `msdp.ROOM_NAME` and `msdp.AREA_NAME` only into its debug record. Its
  rendered room HTML uses `msdp.ROOM.NAME` and `msdp.ROOM.AREA`.
- The mapper also consumes the structured `ROOM` table.

Removing the unused scalar subscriptions would be a valid short-term package mitigation, but
it would not make the server values correct for other clients or manual REPORT requests.

### 6. Mudlet converts native MSDP through JSON without escaping tabs

In the inspected Mudlet source, `ctelnet.cpp:3555-3573` recognizes a native MSDP Telnet
subnegotiation and passes its payload to `TLuaInterpreter::msdp2Lua()`.

`TLuaInterpreter.cpp:4016-4132` constructs JSON text from MSDP delimiters. For a scalar value it
adds double quotes. It has explicit cases for a backslash and double quote, but all other bytes,
including a horizontal tab, take the default path and are copied verbatim.

The constructed input therefore becomes conceptually:

```json
"<literal 0x09>cTrue Neutral<literal 0x09>n"
```

`setMSDPTable()` then calls the shared `parseJSON()` path at
`TLuaInterpreter.cpp:3761-3874`, which invokes the Lua global `json_to_value`. YAJL correctly
rejects an unescaped byte below `0x20` inside a JSON string.

This implementation detail explains two otherwise confusing facts:

1. A native MSDP failure is labeled as a JSON decoder error.
2. The invalid input shown in the error has surrounding double quotes even though native MSDP
   sends scalar strings without quote bytes.

The quote evidence makes native MSDP the diagnosed transport for the reported frames. In the
GMCP receive path, Mudlet passes the server's JSON data to the decoder rather than adding these
scalar quotes through `msdp2Lua()`.

Mudlet's conversion limitation is broader than horizontal tabs. Native MSDP permits control
bytes other than its reserved delimiters, while JSON requires all control bytes below `0x20`
inside strings to be escaped. Mudlet special-cases BEL and ESC before `msdp2Lua()`, but the
inspected path does not comprehensively JSON-escape the remaining permitted control bytes.

### 7. The protocol references confirm an integration mismatch

The MSDP protocol definition states that variable/value content cannot contain NUL, the six
MSDP delimiter bytes, or IAC. A horizontal tab is not prohibited. The same reference lists
Mudlet as having limited MSDP support because it does not allow control codes over MSDP.

The practical contract for LuminariMUD's Mudlet GUI must therefore be stricter than the
wire-level MSDP allowance: generic text fields should be plain client-facing text without
LuminariMUD color controls.

References:

- <https://tintin.mudhalla.net/protocols/msdp/>
- <https://wiki.mudlet.org/w/Manual:Supported_Protocols>
- Mudlet source at the inspected commit:
  <https://github.com/Mudlet/Mudlet/blob/e91b8596fd1b862f7e4d9a0079a0b1bc086a40fd/src/TLuaInterpreter.cpp>
- LuminariGUI source at the inspected commit:
  <https://github.com/LuminariMUD/LuminariGUI/blob/ab433970d3124880b8090b88961e4055c0c2d495/theGUI/src/scripts/gui/40_msdp_protocol.xml>

## Findings Register

| ID | Severity | Status | Finding |
|----|----------|--------|---------|
| MJD-001 | High | Fixed; verified | LuminariMUD sent internal tab color markup in `ALIGNMENT`, `AREA_NAME`, and `ROOM_NAME` scalar values. |
| MJD-002 | Medium | External; diagnosed | Mudlet's native MSDP conversion does not JSON-escape a literal tab before calling `json_to_value`. |
| MJD-003 | Low | Fixed; final verification in progress | LuminariGUI subscribed to all three failing scalars although its visible UI did not consume them. |
| MJD-004 | High | Fixed; verified | LuminariMUD's MSDP-over-GMCP fallback used a nonstandard package shape, emitted invalid JSON, and could not receive standard JSON commands. |
| MJD-005 | Medium | Complete; verified | Production scalar, wire-format, malformed-input, memory-tool, fuzz, alternate-build, and installation coverage passes. |

### MJD-001: Internal color markup leaks into scalar OOB values

This is the direct server-side cause and the primary repair target. The three values should be
plain text at the OOB boundary. The correct change should preserve colored terminal strings and
world data while sanitizing a local protocol copy.

Expected values are:

```text
ALIGNMENT = True Neutral
AREA_NAME = the Vault of Ages
ROOM_NAME = the Vault of Ages
```

The exact capitalization should follow the existing source text after color removal.

### MJD-002: Mudlet's native MSDP conversion has incomplete JSON escaping

Mudlet is allowed to expose native MSDP in Lua through any internal representation, but once it
chooses JSON as an intermediate form it must escape every JSON control character. The current
path cannot safely represent all byte sequences that native MSDP permits.

An upstream Mudlet repair would make the client more robust, but it would not be sufficient as
the LuminariMUD repair. Even if Mudlet accepted the tabs, values such as `\tcTrue Neutral\tn`
would still contain LuminariMUD-specific color selectors that have no useful meaning in the
GUI's structured-data model.

### MJD-003: The GUI report set is broader than its current consumers

The report list describes itself as the set of variables the GUI needs, but the inspected
rendering paths do not display these three scalars. `ROOM` already supplies clean nested room
and area names. This unnecessarily activates the exact server/client incompatibility on every
fresh REPORT cycle.

Removing unused reports can reduce errors and traffic. It should be treated as cleanup or a
temporary mitigation, not as closure of MJD-001 or MJD-002. If alignment is intended for a
future player display, the clean server contract should be established first.

On 2026-08-06, the LuminariGUI `master` branch at `7698e3b` (package version `2.0.4.044`) was
re-audited before implementation. `ALIGNMENT`, `AREA_NAME`, and `ROOM_NAME` are still present
in `GUI.MSDP_REPORT_VARS`. The latter two are read only into `GUI.updateRoom()` debug state.
`ALIGNMENT` is read only into `GUI.updatePlayer()` debug state and has an event-table entry that
refreshes a panel whose rendered HTML does not use alignment. The structured `ROOM` report
continues to supply the visible room panel and mapper. The package remediation will remove the
three unused reports, their debug-only reads, and the now-unowned alignment event entry, then
add regression coverage for the intended subscription contract.

### MJD-004: The GMCP fallback has a separate serialization defect

This is not the transport path identified in the reported log, but it was found while tracing
the shared sender.

Before remediation, when native MSDP was unavailable and GMCP was active, `MSDPSend()` used:

```c
"MSDP.%s %s"
```

for string variables. `MSDPSendPair()` and `MSDPSendList()` used the same raw interpolation
pattern. The defects were:

- The standard mapping uses the case-sensitive `MSDP` package with one JSON object, such as
  `MSDP {"HEALTH": 10}`. The old `MSDP.HEALTH 10` package/payload shape was not the
  documented MSDP-over-GMCP mapping.
- A scalar such as `LuminariMUD` was emitted without JSON quotes.
- Quotes, backslashes, tabs, newlines, and other JSON-sensitive bytes were not escaped.
- Stored MSDP tables and arrays contain binary MSDP delimiter bytes, not JSON object/array
  syntax.
- Only simple numeric payloads were naturally valid JSON in the old generic path.

The MSDP-over-GMCP reference explicitly notes that JSON cannot carry raw control codes and that
the conversion layer must handle the difference:

- <https://mudstandards.org/gmcp/msdp/>

The repair therefore required a real serializer, not only `strip_colors()` at three call sites.

## Implementation Progress

### 2026-08-06: LuminariGUI subscription narrowing implemented

- LuminariGUI commit `59d267e` removes `ALIGNMENT`, `AREA_NAME`, and `ROOM_NAME` from
  `GUI.MSDP_REPORT_VARS` while retaining the structured `ROOM` report used by the room panel
  and mapper.
- Removed the debug-only `msdp.ALIGNMENT` read and its obsolete player-refresh event entry.
  Room debug snapshots now take `NAME` and `AREA` from the same structured `ROOM` value used by
  the visible UI.
- Added a lifecycle regression that rejects any reintroduction of the three scalar reports,
  requires `ROOM`, checks uniqueness and exact REPORT dispatch, and verifies that package
  consumers and event ownership no longer reference the unused scalar fields.
- Updated the package changelog, Mudlet smoke procedure, and resource-ownership baseline. Follow-up
  documentation commit `5daf2cc` corrects the protocol reference's `ALIGNMENT` type, replaces its
  scalar `ROOM_NAME` consumer example with structured `ROOM.NAME`, documents the package omission,
  and shows the native Mudlet `sendMSDP("REPORT", variable)` subscription call.
- Built package version `2.0.4.045`. Build validation and generated-output drift checks pass; all
  8 supported test suites pass, including 37/37 lifecycle regressions and 82/82 Lua syntax checks;
  package validation passes; and the resource analyzer reports 36 owned and zero unowned runtime
  handlers plus 21 owned and zero unowned timer sites. GitHub checks remain in progress for the
  final pushed commit.

### 2026-08-05: Plain scalar boundary repair implemented

- Added one bounded production helper in `src/comm.c` that copies a scalar source, strips MUD
  color markup from the copy, and stores the plain result through `MSDPSetString()`.
- Routed `ALIGNMENT`, `AREA_NAME`, and `ROOM_NAME` through that helper. The canonical alignment,
  zone, and room strings remain unchanged for terminal output.
- Added `unittests/CuTest/test_msdp_production.c` to both supported production test manifests.
  Its fixtures use the real True Neutral source string and colored Vault-style room/area names,
  assert the three stored values are plain, assert the sources are not modified, and assert an
  unchanged clean value is not marked dirty again.
- Source formatting and `git diff --check` pass. The warning-free optimized build succeeds,
  the production-linked CuTest suite passes 413/413, and `make install` installs `bin/circle`
  and leaves no root-level `circle` artifact.
- While tracing MJD-004, verified that the fallback also uses the wrong GMCP package shape; the
  repair must emit `MSDP` followed by a JSON object, not `MSDP.<variable>` followed by a raw
  value.

### 2026-08-05: Standards-compliant MSDP-over-GMCP conversion implemented

- Added a bounded MSDP-to-JSON serializer in `src/net/msdp_json.c`. It emits the exact
  case-sensitive `MSDP` GMCP package with one JSON object, quotes scalar strings, preserves
  integer variables as JSON numbers, and recursively converts MSDP tables and arrays to JSON
  objects and arrays.
- The serializer validates UTF-8 for GMCP, escapes quotes, backslashes, JSON short-control
  characters, and remaining legal control bytes, rejects reserved MSDP marker bytes in scalar
  text, rejects malformed or over-deep marker structures, and performs its capacity check after
  escaping. A failed send queues no partial frame and leaves a reported variable dirty for
  retry.
- `MSDPSendPair()` now preserves its value as a JSON string, while `MSDPSendList()` produces an
  actual JSON array and ignores leading, repeated, and trailing space separators. The native
  list path now follows the same tokenization and no longer creates empty elements from repeated
  spaces.
- Replaced the legacy inbound GMCP `@NAME value` parser with strict UTF-8 JSON parsing for
  standard messages such as `MSDP {"REPORT":["HEALTH","TITLE"]}`. Only the case-sensitive
  `MSDP` package is routed to the existing MSDP command executor; malformed or unsupported JSON
  is rejected before any command in the object is applied. Lexical validation also rejects
  escaped NUL in either a JSON member name or value before `json-c` can normalize it.
- Normalized `AFFECTS` to store table content through `MSDPSetTable()` instead of embedding an
  outer table in `MSDPSetString()`. Its update path now supports either native MSDP or the GMCP
  fallback, matching the other structured producers.
- Added the new source and header to both build manifests and to the focused protocol and fuzz
  harnesses. The focused harness now passes 29/29 tests, covering strict scalar JSON round trips,
  all escape classes, UTF-8, JSON numeric typing, nested objects and arrays, GUI array defaults,
  pair/list typing, native list framing, malformed marker rejection, invalid UTF-8, post-escape
  overflow, standard inbound REPORT arrays, atomic rejection, and escaped NUL in member names and
  values. A ten-second ASan/UBSan fuzz run completed without a finding, and a warning-free
  optimized server build linked the new serializer successfully.
- Updated the canonical protocol and variable references, their legacy mirror, API comments,
  performance notes, and the changelog with the plain-text and JSON contracts. The in-game help
  corpus has no existing MSDP or GMCP entry to update; the maintained contract is developer- and
  integration-facing documentation.

### 2026-08-05: Final verification and server closure

- The production-linked CuTest suite passes 413/413 after rebuilding the server and test binary
  with GNU C23, `-Wall`, and `-Wextra`.
- The authoritative `make test-all` path passes the 413 production tests, 170 world-tool tests,
  29 focused protocol tests, documentation validation, both character-rename checks, and the
  final installation step.
- The focused 29-test suite passes under Valgrind with zero errors, zero bytes in use at exit,
  and 4,259 allocations matched by 4,259 frees.
- A final ten-second ASan/UBSan protocol fuzz run completed without a finding.
- A CMake Release build compiles and links `src/net/msdp_json.c`. It exposed object-bound
  diagnostics in the shared TTYPE allocator; client and version copies now use the actual
  65-byte working-buffer capacity, and focused tests cover both Mudlet and DecafMUD splitting.
  The final CMake build emits no protocol-source diagnostic.
- `make install` restored the authoritative Autotools build in `bin/circle`, verified its
  `libjson-c` and MariaDB linkage, and removed the root-level `circle` artifact.
- Formatting hooks, trailing-whitespace checks, merge-marker checks, ASCII documentation checks,
  and `git diff --check` pass.

### MJD-005: Regression coverage and verification complete

Before remediation, the protocol parser harness and fuzz target exercised malformed input and
called `MSDPSetString()`, but no test asserted that production text variables were free of
internal color markup or that a GMCP fallback payload was parseable JSON.

The production suite now covers the three color-bearing scalar sources and unchanged-value
dirty tracking. The focused harness covers native framing, strict JSON escaping, UTF-8, nested
tables and arrays, malformed marker rejection, post-escape size limits, and inbound
MSDP-over-GMCP commands. The production, focused, fuzz, Valgrind, CMake, documentation, and
installation gates all pass.

## User-Visible Impact

Before server remediation, the inspected LuminariGUI package experienced these effects:

- Mudlet logs one decoder error for alignment and one each for the scalar area and room name
  when those values are reported.
- `msdp.ALIGNMENT`, `msdp.AREA_NAME`, and `msdp.ROOM_NAME` are not updated by the rejected
  payloads. Existing values can remain nil or stale.
- Mudlet still raises protocol events after its parse attempt in the inspected source, so a
  package handler may run while the corresponding table value is missing.
- The visible player panel currently does not render alignment.
- The visible room panel and mapper use the separately sanitized `msdp.ROOM` table, so they
  should remain functional.
- Errors could recur after a fresh REPORT batch, profile reset, reconnect, alignment change, or
  movement into another colored room/zone value.

This incident does not show evidence of memory corruption, descriptor overflow, partial Telnet
framing, malformed world files, or a Lua exception thrown by LuminariGUI code.

The repaired server now stores the three scalars as plain text and therefore no longer supplies
the literal tabs that triggered these reported decoder errors. GMCP-only clients receive the
same logical values as strict JSON. No live Mudlet or LuminariGUI session was used during
verification, so the independent generic Mudlet escaping limitation and the GUI's broad report
list remain external findings rather than claims of client-side closure.

## Distinction From the Bardic MSDP Overflow Incident

`bardic-performance-msdp-overflow-audit.md` documents another Mudlet JSON error with a different
server cause. That incident involved a descriptor queue overflow truncating an `AFFECTS` frame,
inserting `**OVERFLOW**`, and allowing later prompt bytes into an unterminated structured value.

This incident instead has:

- complete short scalar frames;
- intact opening and closing value boundaries;
- no `**OVERFLOW**` marker;
- no truncated `AFFECTS` fields;
- no prompt ANSI bytes; and
- one literal tab at each internal color-code boundary.

The two investigations should not be merged into one repair. The bardic framing fix cannot
remove color markup from otherwise complete `ALIGNMENT`, `AREA_NAME`, or `ROOM_NAME` values.

## Recommended Repair Order

The list below preserves the original repair order; each heading now records its disposition.

### 1. Repair the server's three production values (complete)

At the protocol boundary:

1. Produce alignment from a plain-name source or strip a writable copy.
2. Copy the current zone name, strip colors, then set `AREA_NAME`.
3. Copy the current room name, strip colors, then set `ROOM_NAME`.
4. Preserve the colored canonical values for terminal output.
5. Do not edit the artifact world files merely to silence this client path.

This is the smallest repair that addresses the reported incident for every native MSDP client.

### 2. Add production-linked regression coverage (complete)

Tests should prove:

- True Neutral is stored and emitted as `True Neutral` with no byte `0x09`.
- A fixture room and zone with `\tW...\tn` runtime names emit plain scalar names.
- The structured `ROOM` table remains valid and plain.
- Native MSDP frames retain correct `IAC SB ... IAC SE` boundaries.
- Report/update behavior does not regress when a clean value is unchanged.

The root production-linked CuTest suite is the appropriate place for production room/alignment
behavior. The focused parser harness can cover byte-level conversion and framing helpers.

### 3. Narrow the GUI subscription set (implemented; verification in progress)

In the LuminariGUI source-of-truth fragment, remove `AREA_NAME` and `ROOM_NAME` if no current
consumer needs them beyond debug logging. Evaluate `ALIGNMENT` similarly until it is actually
rendered. Keep `ROOM`, which supplies the mapper and room panel.

This reduces unnecessary reports and provides an immediate client-package mitigation, but it
must not replace the server correction.

### 4. Replace the GMCP fallback interpolation with serialization (complete)

Define one explicit conversion from the stored MSDP value model to JSON:

- quote and escape scalar strings;
- emit numbers as JSON numbers;
- convert MSDP arrays to JSON arrays;
- convert MSDP tables to JSON objects;
- handle nested structures;
- reject malformed internal marker sequences; and
- calculate output bounds after escaping rather than before it.

Add independent tests for quotes, backslashes, all JSON control escapes, UTF-8 text, nested
tables, arrays, and boundary lengths.

### 5. Report the general Mudlet escaping gap upstream (external follow-up)

A focused upstream reproducer can feed native MSDP with a scalar horizontal tab and assert that
Mudlet stores the intended Lua string without logging a JSON error. Broader cases should cover
every native MSDP byte that is legal on the wire but requires escaping in JSON.

The server repair should not wait for an upstream Mudlet release.

## Acceptance Criteria and Closure

All server-owned criteria are complete:

- Native MSDP `ALIGNMENT`, `AREA_NAME`, and `ROOM_NAME` contain no internal color controls.
- Production fixtures prove expected plain values while preserving the canonical colored
  sources used by terminal output.
- Production-linked tests cover color-bearing alignment, room, and zone sources.
- The complete native value model, including nested tables and arrays, converts to parseable
  strict JSON over GMCP or fails atomically for malformed and oversized values.
- Protocol, API, variable, performance, changelog, and investigation documentation describes
  the plain-text OOB and JSON fallback contracts.
- The authoritative test and install gates pass, with no root-level `circle` artifact.

The following observations require changes or live acceptance work in repositories explicitly
excluded from this server remediation. They do not hold server closure open:

- Confirming the absence of `json_to_value` errors in a live Mudlet profile and inspecting its
  `msdp` Lua table.
- Exercising the LuminariGUI room panel and mapper against a deployed server build.
- Narrowing the LuminariGUI REPORT list to fields the package intentionally consumes.
- Repairing and reporting Mudlet's general native-MSDP control-byte escaping gap upstream.

## Investigation Boundaries

No code, world data, package source, configuration, credentials, or production system was
modified during the diagnostic phase. Remediation changed only the development checkout's
server code, server-owned tests, build manifests, and documentation. It did not modify Mudlet,
LuminariGUI, credentials, world data, or production. The only network reads were the public
hosted GUI package, public source repositories, and public protocol documentation.
