# World Validator Quest-System Expansion Plan

Status: in progress (Session 3 - HLQ first-class parsing and selection).

Planning baseline: 2026-08-04 at commit `42378034` on the development
environment.

## Implementation Progress

Last updated: 2026-08-04.

- Active session: Session 3 - HLQ first-class parsing and selection.
- Baseline source commit: `4237803456838f3b955462cee2e64acd95d0a04e`.
- Environment gate: `APP_ENV=development` confirmed; no production work is
  authorized or planned.
- Protected files: `lib/.env`, `lib/mysql_config`, and both live quest-data
  trees are ignored and remain read-only.
- Baseline live-data hashes (path-and-content aggregate): QST
  `9d80ee4d90c360c10d5c4b38eb939516b7928a2fb5cef76a61c8511393ce0655`;
  HLQ `7f1647e1d55c3404c348a3cb967cc6722bb764fcae518fb256e55d1a058b7bfe`.
- Baseline inventory: 182 QST files, 162 normal QST index entries, one mini
  QST entry, 320 HLQ files, 283 normal HLQ index entries, no HLQ mini index,
  and indexed `1068.hlq` is zero bytes.
- Verification completed: local and remote baseline commits match after
  `git fetch origin master`; the planning snapshot counts were reproduced.
- Session 1 implementation: generalized source-table extraction, added
  source-derived QST types/flags, HLQ entry/command contracts, mission
  difficulties, validator limits, per-set flag chunk metadata, one-token quest
  flag commands, tracked QST/HLQ packages, and canonical/legacy samples.
- Finding-code inventory: `QST001` and `HLQ001` begin new format ranges;
  reference findings continue at `REF032`; semantic findings continue at
  `SEM023`. Existing highest allocated codes are `REF031` and `SEM022`.
- Session 1 verification: 33 focused contract/CLI tests passed; both Make and
  CMake `test-world-tools` targets passed all 113 tests plus the wrapper; the
  constants and documentation checks passed; QST and HLQ live-data hashes are
  unchanged.
- Published checkpoints: planning baseline commit `9b41691e`; Session 1
  implementation commit `e3052d35`; Session 2 implementation commit
  `da08510c`; Session 3 parser checkpoint pending at this update.
- Session 2 implementation: added the typed QST model and field spans, a
  byte-safe five-string/three-row/dialogue parser, deterministic recovery,
  QST index and selection support, package/order checks, explicit-path
  support, and normal/mini/zone/path tests.
- Session 2 live audit: all 182 ignored QST files parsed completely in 0.09
  seconds with 18,420 KiB peak RSS. An initial cross-package order false
  positive was fixed; the final aggregate contains one real `QST040` duplicate
  finding and no parser incompleteness. No live content was committed.
- Session 2 verification: 18 focused QST tests passed; both Make and CMake
  `test-world-tools` targets passed all 130 tests plus the wrapper; constants
  and documentation checks passed; QST and HLQ live-data hashes are unchanged.
- Session 3 parser implementation: added nested host, entry, and command
  models; source-derived entry/command decoding; physical and effective
  runtime ordinals; byte-safe ASK/GIVE/ROOM parsing; and deterministic
  recovery that does not reproduce stale numeric or uninitialized command
  state.
- Session 3 parser verification: 16 focused HLQ tests passed. A hash-guarded
  read-only pass parsed 320 development files, 1,665 host blocks, 3,654
  entries, and 2,387 commands in 0.14 seconds with 18,680 KiB peak RSS. The
  aggregate had one zero-byte/missing-terminator finding and four real
  post-terminator-content findings; no validator defect was found and the
  audit hash was unchanged.
- Next implementation step: connect `.hlq` to indexes and every selection
  mode, add packaging tests, then rerun the complete gates.

This plan extends the read-only `wtool` system documented in
[`docs/utilities/WORLD_VALIDATOR_CLI.md`](../utilities/WORLD_VALIDATOR_CLI.md)
from six covered flat-file datasets to eight covered flat-file datasets:

- existing: `.zon`, `.wld`, `.mob`, `.obj`, `.shp`, and `.trg`;
- new: `.qst` from `lib/world/qst`; and
- new: `.hlq` from `lib/world/hlq`.

The target is parity with the existing formats, not merely syntax parsing.
Quest files are complete only when they participate in every applicable
index, parser, model, reference, semantic, lookup, reporting, documentation,
fixture, build, CI, and operational-validation path.

This is a validator and lookup expansion. It does not authorize an emitter,
automatic repair, or any write to builder-owned world data.

## 1. Outcome and Definition of Parity

At completion, these commands must treat `.qst` and `.hlq` as first-class
world data:

```sh
python3 scripts/world/wtool.py validate --all
python3 scripts/world/wtool.py validate --mini
python3 scripts/world/wtool.py validate --zone 100
python3 scripts/world/wtool.py validate --paths staged/100.qst staged/100.hlq

python3 scripts/world/wtool.py show quest 10001
python3 scripts/world/wtool.py refs quest 10001
python3 scripts/world/wtool.py show hlquest 10000
python3 scripts/world/wtool.py refs hlquest 10000

python3 scripts/world/wtool.py flags list quest
python3 scripts/world/wtool.py constants list quest-types
python3 scripts/world/wtool.py constants list hlquest-entry-types
python3 scripts/world/wtool.py constants list hlquest-commands
```

For `hlquest`, the lookup number is the host mobile VNUM. The HLQ format has
no independent quest VNUM; one host block contains an ordered list of ASK,
GIVE, and ROOM entries.

Parity means all applicable cells below are complete.

| Capability | Required quest-system result |
|------------|------------------------------|
| Indexed discovery | Read `qst/{index,index.mini}` and `hlq/{index,index.mini}` with the same path, suffix, order, duplicate, missing-file, terminator, and unindexed-file checks used by existing datasets. |
| Byte-safe parsing | Parse with the shared source cursor, physical source spans, UTF-8 plus `surrogateescape`, C-width integer checks, and deterministic recovery. |
| Typed model | Add `QuestRecord` and host-keyed `HlQuestRecord` models, including nested HLQ entries and commands without losing physical or runtime order. |
| Structural validation | Diagnose every boot-fatal, crash-prone, silently truncated, ignored, or noncanonical construct identified from the current loaders and OLC writers. |
| Packaging and ordering | Check duplicate identities, package ownership, canonical order, selected-zone inclusion, and normal/mini index behavior. |
| Typed references | Validate and expose quest links to mobiles, objects, rooms, and other quests; expose HLQ host, entry, and command references. |
| Semantic checks | Apply source-derived type, flag, range, reward, command-legality, chain, and dialogue rules. |
| Lookup | Support human and JSON `show` and bidirectional `refs`, including incoming quest references on existing world records. |
| Flags and constants | Extract quest types, quest flags, HLQ entry types, HLQ command codes, and required limits from bounded C source regions. |
| Reporting | Preserve stable finding codes, deterministic ordering, shared exit rules, parse completeness, and versioned JSON. |
| Documentation | Add permanent file-format guides, update builder and CLI guides, and enforce the source-backed tables with `docs --check`. |
| Tests and delivery | Cover canonical and legacy syntax, malformed input, all reference roles, normal/mini/zone/path modes, Make and CMake gates, CTest, and GitHub Actions. |
| Operational safety | Run a hash-guarded audit of the ignored development data and a development boot/playtest without modifying world files. |

## 2. Source-Verified Current State

### 2.1 Current validator boundary

The current implementation explicitly enumerates only six extensions and six
record types in:

- `scripts/world/wtool_lib/indexes.py:DATA_EXTENSIONS`;
- `scripts/world/wtool_lib/models.py:WorldRecord` and `WorldData`;
- `scripts/world/wtool_lib/world.py:_load_files()`, indexed loading, selected
  packages, and explicit-path collection;
- `scripts/world/wtool_lib/lookup.py:RECORD_TYPE_ALIASES` and
  `CLI_RECORD_TYPES`;
- `scripts/world/wtool_lib/cli.py` command choices;
- `Makefile.am:world_tool_sources` and `world_tool_support_files`;
- `CMakeLists.txt:WORLD_TOOL_SOURCES` and `WORLD_TOOL_SUPPORT_FILES`; and
- the tracked `scripts/world/tests/fixtures/phase2/complete` world.

Consequently, current `validate`, `show`, and `refs` calls neither parse nor
report anything from `lib/world/qst` or `lib/world/hlq`.

### 2.2 QST source contract

The authoritative Luminari QST grammar is split across:

- `src/db.h:QST_PREFIX` and `src/db.c:index_boot()`;
- `src/quest/quest.h:struct aq_data`, the `AQ_*` types, and quest flags;
- `src/quest/quest.c:parse_quest()`, `assign_the_quests()`, and the runtime
  consumers; and
- `src/olc/genqst.c:save_quests()` plus `src/olc/qedit.c:qedit_parse()`.

One record has this shape:

```text
#<quest-vnum>
<name>~
<description>~
<accept-message>~
<completion-message>~
<quit-message>~
<type> <questmaster-mob> <one-flag-token> <target> <previous> <next> <prerequisite-object>
<points> <quit-penalty> <min-level> <max-level> <time> <return-mobile> <quantity>
<gold> <experience> <reward-object> [<race> <x> <y> <follower-mobile>]
[D
<diplomacy-dc> <intimidate-dc> <bluff-dc> <alternative-quest>]
S
```

The file ends with a line whose first character is `$`; the current writer
emits `$~`.

Important compatibility behavior:

- The Luminari branch reads five tilde strings. The sixth kill-list string is
  compiled only for `CAMPAIGN_DL`, which this repository no longer supports.
- Numeric rows one and two require seven conversions.
- The reward row intentionally accepts the legacy three-field form and the
  current seven-field form.
- The `D` block is optional to the loader and always emitted by the current
  writer. Repeated `D` blocks overwrite earlier values in memory.
- An unknown extension marker is not handled by `parse_quest()` and can leave
  the server looping on the same input. It is an error, not an extension point.
- The parser resolves the questmaster while loading and replaces an unresolved
  raw VNUM with `NOBODY`. `wtool` must preserve the raw token so the real defect
  remains diagnosable.
- Quest flags use one `sprintascii()` token, not the four serialized chunks
  used by room, mobile, and object bitvectors.

The type-sensitive target contract is:

| Types | Target interpretation | Additional typed fields |
|-------|-----------------------|-------------------------|
| `AQ_OBJ_FIND`, `AQ_OBJ_RETURN` | Object VNUM | `AQ_OBJ_RETURN` also requires the return-mobile field. |
| `AQ_ROOM_FIND`, `AQ_ROOM_CLEAR` | Room VNUM | None. |
| `AQ_MOB_FIND`, `AQ_MOB_KILL`, `AQ_MOB_SAVE`, `AQ_DIALOGUE` | Mobile VNUM | Dialogue also uses its three DCs and optional alternative quest. |
| `AQ_AUTOCRAFT` through `AQ_CRAFT_RESTRING` | No typed VNUM target | Completion is driven by the matching crafting event. |
| `AQ_COMPLETE_MISSION` | Mission difficulty index | Validate against `NUM_MISSION_DIFFICULTIES`. |
| `AQ_HOUSE_FIND` | No target | Completion uses the current private house. |
| `AQ_WILD_FIND` | Wilderness coordinates | Validate coordinate presence, but do not query MariaDB wilderness data. |
| `AQ_GIVE_GOLD` | Gold threshold | Requires the return-mobile field. |
| `AQ_MOB_MULTI_KILL` | Comma-separated mobile list | The Luminari disk grammar does not persist that list; see the explicit limitation below. |

Every quest also has independent optional references to a prerequisite object,
reward object, follower mobile, previous quest, next quest, and dialogue
alternative quest.

Source-backed OLC bounds include:

- quantity `1..50`;
- completion points and quit penalty `0..999999`;
- minimum and maximum level `0..LVL_IMPL`, with minimum not above maximum;
- time `-1..100`;
- gold reward `0..99999` and experience reward `0..999999`;
- dialogue DCs `-1..100`;
- mission difficulty `0..NUM_MISSION_DIFFICULTIES - 1`;
- race reward `-1`, `RACE_LICH`, or `RACE_VAMPIRE`; and
- `MAX_QUEST_NAME`, `MAX_QUEST_DESC`, and `MAX_QUEST_MSG` string bounds.

### 2.3 HLQ source contract

The authoritative high-level quest grammar is split across:

- `src/db.h:HLQST_PREFIX` and `src/db.c:index_boot()`;
- `src/quest/hlquest.h:enum quest_type`, `enum quest_command_type`, and the
  nested structs;
- `src/quest/hlquest.c:boot_the_quests()` and command execution; and
- `src/olc/hlqedit.c:hlqedit_command`, `hlqedit_save_to_disk()`, and editor
  bounds.

The canonical writer emits host blocks in this shape:

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

`A` is ASK, `Q` is GIVE, and `R` is ROOM. A canonical `!` suffix marks an
approved entry. The loader treats any suffix as approved, so other suffixes
must be diagnosed as noncanonical rather than silently normalized. ASK has no
command-chain terminator; GIVE and ROOM require `S`. The file ends with a line
whose first character is `$`; the current writer emits `$~`.

Persisted command codes are indexed by
`src/olc/hlqedit.c:hlqedit_command`, currently `CIOMADTXFKUS`:

| Code | Meaning | Legal direction and validation |
|------|---------|--------------------------------|
| `C` | Coins | Input or output; editor range `0..100000`; runtime output clamps to `MAX_GOLD`. |
| `I` | Item | Input or output; value is an object VNUM. |
| `O` | Load object in room | Output only; value is object VNUM, location is room VNUM or `0` for current room. |
| `M` | Load mobile in room | Output only; value is mobile VNUM, location is room VNUM or `0` for current room. |
| `A` | Attack questor | Output only; no meaningful parameters. |
| `D` | Disappear | Output only; no meaningful parameters. |
| `T` | Teach spell or skill | Output only; runtime-safe range is above `SPELL_RESERVED_DBC` and below `NUM_SPELLS`. |
| `X` | Open door | Output only; location is a room VNUM and value is a direction. |
| `F` | Follow questor | Output only; no meaningful parameters. |
| `K` | Change kit or lich transition | Output only; class values use `NUM_CLASSES`, with the persisted `9999` lich sentinel handled explicitly. |
| `U` | Set church | Output only; value is `0..NUM_CHURCHES - 1`. |
| `S` | Cast spell | Output only; runtime-safe range is above `SPELL_RESERVED_DBC` and below `NUM_SPELLS`. |

The editor currently accepts spell values below `TOP_SKILL_DEFINE`, while the
runtime indexes `spell_info` only below `NUM_SPELLS`. The validator must use
the narrower runtime-safe range as the error boundary and may separately note
the editor disagreement.

Other loader hazards that the validator must expose are:

- a missing host mobile is dereferenced without a validity check;
- GIVE or ROOM before a host can dereference a null host;
- ROOM numeric parsing is unchecked and can reuse stale state;
- an unknown command code can leave an uninitialized command type;
- an invalid input/output direction is logged and discarded;
- an unknown top-level marker truncates the remainder of the file; and
- command-chain EOF or missing `S` can leave parsing in an unsafe state.

The loader prepends host entries and input commands but appends output
commands. The model must retain physical order and expose effective runtime
order; otherwise `show hlquest` could present a different first-match ASK or
GIVE behavior than the game.

### 2.4 Development-world planning snapshot

The builder-owned files are ignored by Git. A read-only snapshot on
2026-08-04 found:

| Dataset | Files on disk | Normal index entries | Mini index |
|---------|---------------|----------------------|------------|
| QST | 182 `.qst` files | 162 | Present; contains only `0.qst`. |
| HLQ | 320 `.hlq` files | 283 | Missing. |

`lib/world/hlq/1068.hlq` is listed by the normal index and is currently zero
bytes. This is expected to become a real validation error. It is not a reason
to weaken the parser or modify the file as part of implementation.

The live snapshot also contains both legacy and current QST reward rows and
both `$` and `$~` quest-file terminators. Tests must preserve intentional
loader compatibility instead of treating current OLC output as the only boot
grammar.

These counts are planning evidence, not acceptance baselines. Recompute them
with the finished parser because ignored builder data can change independently
of this repository.

## 3. Scope and Safety Boundary

### 3.1 In scope

- `.qst`, `.hlq`, `index`, and `index.mini` discovery and parsing;
- QST and HLQ structural, packaging, ordering, reference, semantic, and
  topology findings;
- source-derived quest types, quest flags, HLQ entry types, HLQ command codes,
  mission difficulties, and the limits needed by validation;
- typed `show` and bidirectional `refs` support;
- human and JSON output through the existing reporting contract;
- tracked positive and negative fixtures;
- Make, CMake, CTest, and GitHub Actions integration;
- permanent builder, format, utility, testing, and OLC documentation; and
- read-only development-world audit plus development boot/playtest.

### 3.2 Out of scope

- changes to the C loaders, OLC writers, quest gameplay, or quest rewards;
- the MariaDB-backed questline system and other database quest content;
- file-based help entries under `lib/text/help`, which remain outside the
  current world-validator format boundary;
- Dragonlance or Forgotten Realms campaign file variants;
- wilderness database validation beyond static coordinate shape;
- execution or simulation of quest scripts, combat, skill checks, rewards, or
  OLC sessions;
- authoring, emitting, repairing, reformatting, or reindexing world files;
- creating the missing live `hlq/index.mini`; and
- cleaning the live backlog of unindexed or invalid quest data.

`AQ_MOB_MULTI_KILL` deserves an explicit boundary. The Luminari runtime uses
`kill_list`, but the Luminari loader and writer do not read or persist that
field. A Luminari `.qst` record of this type cannot be represented safely
end-to-end. The validator must emit a high-confidence error explaining that
source mismatch. Adding a new on-disk field or changing runtime behavior is a
separate server feature, not part of this plan.

### 3.3 Read-only guarantees

- `validate`, `show`, `refs`, `flags`, and `docs --check` remain read-only.
- `constants sync --write` remains the only intentional write command and may
  update only `scripts/world/wtool_constants.json`.
- Tests use tracked fixtures or temporary directories, never live world data.
- Operational audits hash `lib/world/qst` and `lib/world/hlq` before and after
  every run and fail the audit if a byte changes.
- No production host or production code is modified. Boot and playtest occur
  only after confirming `APP_ENV=development` in `lib/.env`.
- Ignored live data and its index files are never added to Git by this work.

## 4. Design

### 4.1 Modules and typed model

Add parser modules consistent with the existing layout:

- `scripts/world/wtool_lib/quests.py` for `.qst`;
- `scripts/world/wtool_lib/hlquests.py` for `.hlq`;
- `scripts/world/tests/test_quests.py`; and
- `scripts/world/tests/test_hlquests.py`.

Extend `models.py` with:

- `QuestRecord`, including raw and normalized scalar fields, source spans for
  reference-bearing fields, reward-row width, optional dialogue data, package,
  and completeness;
- `HlQuestRecord`, keyed by `host_mobile_vnum` through its required `vnum`
  property or field;
- `HlQuestEntryRecord`, with entry type, approval marker, keywords, reply,
  optional room, physical ordinal, and effective runtime ordinal; and
- `HlQuestCommandRecord`, with input/output direction, persisted code,
  normalized command type, value, location, physical ordinal, runtime ordinal,
  and source span.

Do not flatten every HLQ entry into a fake VNUM. Nested entries need a stable
composite identity in JSON and findings, such as host mobile plus one-based
physical entry and command ordinals. Human diagnostics should use the same
identity.

Add `quests` and `hlquests` collections to `WorldData`, include both in
`WorldRecord`, and make record-type rendering explicit enough that `HlQuestRecord`
is always serialized as `hlquest`.

### 4.2 Parser compatibility and recovery

Both parsers reuse `SourceFile`, `SourceCursor`, `read_significant()`, tilde
string handling, integer parsing, `ParseResult`, and common finding helpers.
They must not implement a second interpretation of comments or blank lines.

Each parser distinguishes:

1. accepted loader grammar;
2. accepted but legacy or noncanonical grammar;
3. dangerous loader acceptance or silent data loss; and
4. malformed input for which recovery is no longer trustworthy.

Recovery rules must be deterministic:

- after a damaged QST record, resume only at a credible `#<integer>` header or
  file terminator at column zero;
- after a damaged HLQ entry or chain, resume at a credible host header, entry
  marker, or file terminator, while marking the affected host incomplete;
- never manufacture references from stale numeric state; and
- report a primary error rather than cascades caused by fields that were not
  parsed.

### 4.3 Index and selection behavior

Extend `DATA_EXTENSIONS` to include `qst` and `hlq`. Keep
`REQUIRED_FULL_DATASETS` unchanged: the server permits zero QST and HLQ
records, but still requires readable index files.

| Selector | Required behavior |
|----------|-------------------|
| `--all` | Parse both normal indexes, report unindexed conventional files, and load both datasets into the complete graph. |
| `--mini` | Require and parse both `index.mini` files. A missing HLQ mini index is `IDX009`, matching server boot failure; do not silently skip it. Do not report normal-index omissions in mini mode. |
| `--zone N` | Load the normal indexed reference world, add unindexed canonical `N.qst` and `N.hlq` files when present, and restrict record findings to selected packages while retaining cross-zone reference resolution. |
| `--paths` | Recognize `.qst` and `.hlq` in explicit files and directories, use only the supplied reference universe, and update the no-supported-files diagnostic. |

QST package ownership is derived from the quest VNUM and zone ranges. HLQ
package ownership is derived from the host mobile VNUM and zone ranges. A
record in a different numeric package from the canonical OLC destination is
diagnosed without moving it.

### 4.4 Source-derived constants and flags

Generalize the bounded manifest extractor where needed so a table can name its
own source file instead of assuming all display arrays live in
`src/constants.c`. Add checked manifest entries for:

- `src/quest/quest.h:AQ_UNDEFINED..NUM_AQ_TYPES` paired with
  `src/quest/quest.c:quest_types`;
- `src/quest/quest.h:AQ_REPEATABLE..NUM_AQ_FLAGS` paired with
  `src/quest/quest.c:aq_flags`;
- `src/quest/hlquest.h:enum quest_type`;
- `src/quest/hlquest.h:enum quest_command_type` paired by index with
  `src/olc/hlqedit.c:hlqedit_command`;
- `src/quest/missions.h:NUM_MISSION_DIFFICULTIES` and, if displayed by the
  CLI/docs, `src/quest/missions.c:mission_difficulty`;
- `NUM_CHURCHES`, `TOP_SKILL_DEFINE`, `SPELL_RESERVED_DBC`, `MAX_GOLD`,
  `MAX_QUEST_NAME`, `MAX_QUEST_DESC`, `MAX_QUEST_MSG`, `NUM_AQ_FLAGS`, and
  `NUM_AQ_TYPES`; and
- `RACE_UNDEFINED`, `RACE_LICH`, and `RACE_VAMPIRE`.

Existing manifest entries already provide directions, `NUM_CLASSES`,
`NUM_SPELLS`, and `LVL_IMPL`; reuse them rather than duplicating values.

Refactor flag encoding around per-set serialized chunk counts. Existing sets
must remain byte-for-byte compatible and continue emitting four tokens. The
new `quest` set accepts and emits exactly one token. Contract tests must prove
that adding the one-token codec does not change existing flag JSON or human
output.

### 4.5 Typed reference graph

Add these edges to the shared graph.

QST edges:

- quest -> mobile: questmaster;
- quest -> object, room, or mobile: type-sensitive target;
- quest -> mobile: return recipient for object-return and give-gold quests;
- quest -> object: prerequisite and object reward;
- quest -> mobile: follower reward;
- quest -> quest: previous, next, and dialogue alternative; and
- incoming edges on every referenced existing record.

HLQ edges:

- hlquest -> mobile: attached host;
- hlquest -> room: ROOM entry;
- hlquest -> object: input item, output item, and load-object commands;
- hlquest -> mobile: load-mobile command;
- hlquest -> room: nonzero load destination and open-door location; and
- incoming edges on the host and all referenced existing records.

Spell, class, church, mission-difficulty, race, coordinate, and gold values are
source-derived scalar domains, not world records. Validate them semantically
without inventing graph record types.

### 4.6 Structural and semantic policy

Use the existing severity meanings. In particular:

| Condition | Default treatment |
|-----------|-------------------|
| Boot failure, possible crash/undefined access, parser loop, inaccessible duplicate, missing required reference, or runtime array over-index | Error. |
| Server accepts but silently drops, substitutes, truncates, clamps, or cannot persist the intended data | Error when intent is lost; warning when the value remains safely usable. |
| Legacy three-field QST reward row or omitted historical `D` block | Accepted and represented; warning only if the project chooses a migration signal after measuring live noise. |
| Noncanonical HLQ approval suffix | Warning; preserve the fact that the loader treats it as approved. |
| `AQ_UNDEFINED` quest | Info or no finding; it is an explicit unavailable state, not malformed syntax. |
| Unapproved HLQ entry | No finding; approval is a legitimate builder workflow state. |
| Unindexed conventional file in normal mode | Existing `IDX008` warning. |
| Missing QST/HLQ index, including mini mode | Existing `IDX009` error. |
| Empty QST/HLQ dataset with a valid index | Allowed. |

Do not finalize warning policy from the local corpus alone. First write parser
tests for source behavior, run a count-only audit, and promote only findings
that identify actionable risk rather than normal historical syntax.

QST semantic checks include:

- valid type and known flag bits;
- required questmaster and type-dependent targets;
- all optional typed references when not at their documented sentinel;
- numeric and string bounds from QEDIT;
- nonnegative rewards and sensible quantity, level, and time relationships;
- dialogue DCs and alternative-quest applicability;
- duplicate VNUMs and canonical package/order behavior;
- previous/next self-links, cycles, and non-reciprocal chain links, with
  severity based on demonstrated runtime impact;
- missing or inconsistent alternative-dialogue topology; and
- the unpersistable Luminari multi-kill type.

HLQ semantic checks include:

- valid host and entry marker;
- valid approval suffix;
- required strings, room field, and chain terminator by entry type;
- command code, arity, and legal input/output direction;
- typed value and location references;
- runtime-safe spell, direction, class, church, coin, and lich-sentinel bounds;
- input commands restricted to the COINS and ITEM cases the runtime consumes;
- parameter-free output commands checked for suspicious persisted junk only
  after canonical writer fixtures establish what the current editor emits;
- duplicate host blocks and package/zone ownership; and
- preserved physical versus effective runtime ordering.

Finding prefixes remain consistent with the existing contract:

- `QST` and `HLQ` for format-specific structure;
- `REF` for typed reference failures; and
- `SEM` for semantic or topology findings.

Allocate the next unused numeric codes only after an automated code inventory.
Add every new code and severity to the permanent CLI documentation and lock it
with human/JSON reporting tests.

### 4.7 CLI and JSON compatibility

Add record aliases:

- `quest` and `qst` -> `quest`;
- `hlquest` and `hlq` -> `hlquest`.

Keep the documented CLI choices concise (`quest` and `hlquest`) while aliases
remain available through normalization where argparse permits them. `show
hlquest <host-vnum>` may return multiple source blocks if malformed data
duplicates a host; it must not merge those blocks and hide the duplicate.

Default version decision:

- bump `TOOL_VERSION` from `0.1.0` to `0.2.0` for the new public record types;
- retain `JSON_SCHEMA_VERSION = 1` only if existing envelopes and field types
  remain unchanged and new quest output is additive; and
- bump the JSON schema if implementation requires changing an existing field
  type, not merely because validation now discovers two more datasets.

Golden JSON tests must prove deterministic output and document the final
decision.

## 5. Test Strategy

### 5.1 QST parser matrix

Positive fixtures cover:

- one and multiple records;
- all five tilde strings, multiline messages, comments, and blank lines;
- alphabetic and numeric one-token flags;
- legacy three-field and current seven-field reward rows;
- absent and present `D` blocks;
- `$` and `$~` file terminators;
- all 25 source-derived quest types;
- every optional reward and chain reference; and
- canonical `save_quests()` output.

Negative fixtures cover:

- bad or missing headers and terminators;
- unterminated strings;
- short, overlong, nonnumeric, and overflowing numeric rows;
- invalid flag characters and out-of-range bits;
- malformed, repeated, and unknown extension blocks;
- missing `S` and deterministic resynchronization;
- duplicate/out-of-order VNUMs and wrong package placement;
- missing questmaster, target, reward, prerequisite, follower, and chain
  records;
- every numeric bound and relationship; and
- the Luminari multi-kill persistence defect.

### 5.2 HLQ parser matrix

Positive fixtures cover:

- multiple host blocks;
- approved and unapproved ASK, GIVE, and ROOM entries;
- multiline keywords and replies;
- every command code in every legal direction;
- zero/current-room and explicit-room locations;
- physical and effective entry/input/output order;
- the `9999` lich sentinel; and
- canonical `hlqedit_save_to_disk()` output.

Negative fixtures cover:

- entries before a host and a missing host prototype;
- malformed host and ROOM numeric lines;
- unknown top-level markers and approval suffixes;
- missing strings, command fields, `S`, and file terminators;
- invalid input/output direction and unknown command codes;
- commands in directions the runtime ignores;
- missing object, mobile, and room references;
- invalid spell, direction, class, church, and coin values;
- duplicate host blocks and wrong package placement;
- zero-byte indexed files; and
- deterministic recovery without stale host, room, value, or location reuse.

### 5.3 Integrated fixture and mode matrix

Extend `scripts/world/tests/fixtures/phase2/complete` with `qst/` and `hlq/`
directories. Each receives a normal `index`, `index.mini`, and a compact
cross-linked file. Keep the complete fixture clean in both modes and make its
quest records refer to fixture rooms, mobiles, objects, and quests.

Add focused temporary or negative fixtures for failures rather than making the
complete world noisy. Tests must cover:

- `validate --all`, `--mini`, `--zone`, and `--paths`;
- unindexed selected `N.qst` and `N.hlq` files;
- missing mini indexes;
- the `validate-zone.sh` compatibility wrapper now including both quest
  packages;
- `show` and `refs` human output;
- `show` and `refs` JSON output;
- incoming quest references on mobile, object, and room records;
- flags list/decode/encode for one quest token and unchanged four-token sets;
- constants list and constants drift;
- sorted findings and parse completeness; and
- no input-file mutation, including on parse failures.

## 6. Documentation Deliverables

Create permanent, source-backed guides:

- `docs/world_game-data/QUEST_FILE_FORMAT.md`;
- `docs/world_game-data/HLQUEST_FILE_FORMAT.md`.

Update:

- `docs/utilities/WORLD_VALIDATOR_CLI.md` with eight-format scope, examples,
  record aliases, constants, flags, finding prefixes, limitations, and CI
  coverage;
- `docs/world_game-data/BUILDER_QUICKSTART.md` with the quest validation loop;
- `docs/world_game-data/builder_manual.md` with QEDIT and HLQEDIT workflows and
  links to both format guides;
- `docs/systems/OLC_ONLINE_CREATION_SYSTEM.md`, including correction of the
  current misleading `CON_HLQEDIT` description;
- `docs/guides/TESTING_GUIDE.md` with quest fixture and operational test
  coverage;
- `docs/utilities/README.md` and
  `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md`; and
- `docs/CHANGELOG.md` at implementation closeout.

Extend `scripts/world/wtool_lib/docs_check.py:WORLD_DOCUMENTS` and its bounded
table specs so the permanent docs are checked for:

- every QST type and QST flag;
- all three HLQ entry types;
- all twelve persisted HLQ command codes in enum/string order;
- relevant source paths and function names;
- registered `qedit` and `hlqedit` commands;
- ASCII, UTF-8, and LF requirements; and
- generated HTML only if the builder-guide generator is intentionally
  expanded to publish these two guides.

Do not add generated HTML by accident. Decide during implementation whether
the new format guides belong in `generate-web-guides.sh`; if not, document
that they are Markdown-only and keep the existing generated-guide set
unchanged.

## 7. Delivery Sessions

The work is split into five independently verifiable sessions. Each session
targets one reviewable 2-4 hour change set and leaves the repository with its
own focused gate passing.

### Session 1 - Contracts, constants, and fixtures

Goal: establish source-derived contracts and test scaffolding without changing
validation behavior prematurely.

- [x] T001 [S0101] Reconfirm `APP_ENV=development`, clean working tree, baseline commit, and protected-file rules.
- [x] T002 [S0102] Hash the ignored QST/HLQ trees and record count-only normal/mini/index/file baselines without adding them to Git.
- [x] T003 [S0103] Inventory all existing finding codes and reserve noncolliding `QST`, `HLQ`, `REF`, and `SEM` ranges.
- [x] T004 [S0104] Write parser contract tests from `parse_quest()`, `save_quests()`, `boot_the_quests()`, and `hlqedit_save_to_disk()` examples.
- [x] T005 [S0105] Generalize manifest table specifications to support source tables outside `src/constants.c`.
- [x] T006 [S0106] Extract QST types and display names with count/order drift checks.
- [x] T007 [S0107] Extract QST flags and aliases with count/order drift checks.
- [x] T008 [S0108] Extract HLQ entry enums and command enum/code pairing with length/order/uniqueness checks.
- [x] T009 [S0109] Extract mission difficulty and all missing QST/HLQ scalar limits from bounded source regions.
- [x] T010 [S0110] Refactor flag metadata for per-set chunk counts while preserving existing four-token behavior.
- [x] T011 [S0111] Add one-token quest flag list/decode/encode support and invalid-token tests.
- [x] T012 [S0112] Add clean QST and HLQ packages plus normal/mini indexes to the complete tracked fixture.
- [x] T013 [S0113] Add canonical-writer and legacy positive samples without copying builder-owned live content.
- [x] T014 [S0114] Add new sources, tests, fixtures, and source-contract support files to `Makefile.am` and `CMakeLists.txt` in lockstep.
- [x] T015 [S0115] Regenerate and review `wtool_constants.json`; confirm no unrelated manifest entries changed.
- [x] T016 [S0116] Run constants, flag, fixture-inventory, Make, and CMake focused gates; verify QST/HLQ hashes are unchanged.

Session gate:

```sh
python3 scripts/world/wtool.py constants sync --check
PYTHONPATH=scripts/world python3 -m unittest \
  scripts.world.tests.test_constants scripts.world.tests.test_cli \
  scripts.world.tests.test_quests scripts.world.tests.test_hlquests -v
make check-world-docs
```

### Session 2 - QST first-class parsing and selection

Goal: load `.qst` safely in every validation mode and produce complete
structural findings.

- [x] T017 [S0201] Add `QuestRecord` and field-level span structures to `models.py`.
- [x] T018 [S0202] Create `quests.py` on the shared source cursor and parse record/file terminators.
- [x] T019 [S0203] Parse the five Luminari tilde strings with correct length and recovery behavior.
- [x] T020 [S0204] Parse numeric row one, preserve raw sentinels/VNUMs, and decode exactly one flag token.
- [x] T021 [S0205] Parse numeric row two with signed C-integer bounds and field spans.
- [x] T022 [S0206] Parse both three-field and seven-field reward rows without inventing omitted defaults.
- [x] T023 [S0207] Parse optional/repeated `D` blocks and `S`, diagnosing overwrite and unknown-marker hazards.
- [x] T024 [S0208] Implement credible-header recovery and per-record/file completeness.
- [x] T025 [S0209] Diagnose duplicate/inaccessible VNUMs, canonical ordering, and QST package ownership.
- [x] T026 [S0210] Add `qst` to index discovery, selected packages, explicit paths, and supported-file messages.
- [x] T027 [S0211] Add QST paths and records to `_load_files()`, `WorldData`, and normal/mini/zone/path results.
- [x] T028 [S0212] Add canonical, legacy, malformed, overflow, flag, extension, and recovery unit tests.
- [x] T029 [S0213] Test empty QST datasets, missing indexes, unindexed files, and missing listed files.
- [x] T030 [S0214] Test selected unindexed `N.qst` merge behavior and isolated-path reference universes.
- [x] T031 [S0215] Add human/JSON parser findings and completeness golden tests.
- [x] T032 [S0216] Run the QST parser over a temporary copy/list of the live corpus, triage parser defects separately from data findings, and retain only aggregate evidence.
- [x] T033 [S0217] Run focused and full world-tool gates and confirm all input hashes remain unchanged.

Session gate:

```sh
PYTHONPATH=scripts/world python3 -m unittest scripts.world.tests.test_quests \
  scripts.world.tests.test_indexes scripts.world.tests.test_reporting -v
make test-world-tools
```

### Session 3 - HLQ first-class parsing and selection

Goal: load `.hlq` safely in every validation mode while preserving its nested
and order-sensitive behavior.

- [x] T034 [S0301] Add host, entry, and command HLQ models with composite identities and source spans.
- [x] T035 [S0302] Create `hlquests.py` on the shared source cursor and parse host/file terminators.
- [x] T036 [S0303] Parse ASK entries, approval markers, keyword strings, and reply strings.
- [x] T037 [S0304] Parse GIVE entries, replies, command chains, and required `S` terminators.
- [x] T038 [S0305] Parse ROOM entries, exact room numerics, replies, chains, and terminators.
- [x] T039 [S0306] Parse all twelve source-derived command codes and preserve input/output direction, value, and location.
- [x] T040 [S0307] Model physical and effective runtime order for entries, input commands, and output commands.
- [x] T041 [S0308] Diagnose missing hosts, unsafe host lookup, stale ROOM state, invalid directions/codes, and top-level truncation.
- [x] T042 [S0309] Implement deterministic entry/host recovery without stale values or cascading references.
- [ ] T043 [S0310] Diagnose duplicate host blocks, canonical host order, and host/package ownership.
- [ ] T044 [S0311] Add `hlq` to index discovery, selected packages, explicit paths, and supported-file messages.
- [ ] T045 [S0312] Add HLQ paths and records to `_load_files()`, `WorldData`, and normal/mini/zone/path results.
- [ ] T046 [S0313] Make missing `hlq/index.mini` a normal `IDX009` path and test it explicitly.
- [ ] T047 [S0314] Add canonical ASK/GIVE/ROOM and every-command positive tests.
- [ ] T048 [S0315] Add malformed host, marker, numeric, string, command, chain, EOF, and recovery tests.
- [ ] T049 [S0316] Add order-preservation and zero-byte indexed-file tests.
- [ ] T050 [S0317] Test selected unindexed `N.hlq` merge behavior and isolated-path reference universes.
- [ ] T051 [S0318] Run the HLQ parser over a temporary copy/list of the live corpus and separate validator defects from builder-data findings.
- [ ] T052 [S0319] Run focused and full world-tool gates and confirm all input hashes remain unchanged.

Session gate:

```sh
python3 -m unittest scripts.world.tests.test_hlquests \
  scripts.world.tests.test_indexes scripts.world.tests.test_reporting -v
make test-world-tools
```

### Session 4 - References, semantics, lookup, and reporting

Goal: make parsed quest data as useful and safe as the existing six typed
formats.

- [ ] T053 [S0401] Build typed QST and host-keyed HLQ maps alongside existing room/mobile/object maps.
- [ ] T054 [S0402] Add duplicate and selected-package filtering behavior to the full graph pass.
- [ ] T055 [S0403] Add questmaster, target, return-mobile, prerequisite, reward, follower, chain, and dialogue QST edges.
- [ ] T056 [S0404] Add HLQ host, entry-room, item, load-mobile, load-object, destination-room, and open-door edges.
- [ ] T057 [S0405] Validate every missing QST reference with field-level locations and related records where available.
- [ ] T058 [S0406] Validate every missing HLQ reference with host/entry/command composite context.
- [ ] T059 [S0407] Add QST type, flag, string, quantity, level, time, point, penalty, and reward semantic bounds.
- [ ] T060 [S0408] Add QST mission, wilderness-coordinate, race, dialogue-DC, and multi-kill checks.
- [ ] T061 [S0409] Add QST previous/next/alternative self-link, cycle, reciprocity, and dialogue topology checks with evidence-based severities.
- [ ] T062 [S0410] Add HLQ entry-shape, approval-marker, and legal input/output command checks.
- [ ] T063 [S0411] Add HLQ coin, spell, direction, class, church, lich-sentinel, and parameter checks against runtime-safe bounds.
- [ ] T064 [S0412] Add `quest`/`qst` and `hlquest`/`hlq` type normalization and CLI choices.
- [ ] T065 [S0413] Implement human and JSON `show quest` and `show hlquest`, including physical/runtime ordering.
- [ ] T066 [S0414] Implement bidirectional human and JSON `refs` for both types and incoming edges on existing records.
- [ ] T067 [S0415] Bump tool version, make the schema decision, and add unchanged-six-format compatibility goldens.
- [ ] T068 [S0416] Add graph tests for every edge role, missing target, duplicate, selected-zone filter, and reverse lookup.
- [ ] T069 [S0417] Add semantic boundary tests at below/min/max/above values and sentinel cases.
- [ ] T070 [S0418] Measure deterministic full-fixture time/memory and guard against accidental quadratic quest-chain/reference scans.
- [ ] T071 [S0419] Run the complete Python, constants, docs, wrapper, Make, CMake, and CTest gates.

Session gate:

```sh
make test-world-tools
cmake --build build --target test-world-tools
ctest --test-dir build --output-on-failure -R '^world-tool'
```

### Session 5 - Documentation, CI, operational audit, and closeout

Goal: make the feature discoverable, enforced, and proven against development
data without changing that data.

- [ ] T072 [S0501] Write `QUEST_FILE_FORMAT.md` from the verified Luminari loader/writer contract.
- [ ] T073 [S0502] Write `HLQUEST_FILE_FORMAT.md`, including command legality and physical/runtime order.
- [ ] T074 [S0503] Expand `WORLD_VALIDATOR_CLI.md` from six to eight formats and document all new commands, findings, and limitations.
- [ ] T075 [S0504] Update Builder Quickstart and the builder manual with QEDIT/HLQEDIT validation loops.
- [ ] T076 [S0505] Correct and expand the OLC system documentation for both quest editors.
- [ ] T077 [S0506] Update the testing guide, utilities README, and technical documentation master index.
- [ ] T078 [S0507] Add both guides and their source-backed type/flag/command tables to `docs --check`.
- [ ] T079 [S0508] Decide and implement or explicitly decline generated HTML routing for the new guides.
- [ ] T080 [S0509] Finalize Makefile/CMake support-file lists and verify they remain exactly synchronized.
- [ ] T081 [S0510] Verify GitHub Actions path filters and world-tool steps cover all new sources and docs.
- [ ] T082 [S0511] Run `make test-world-tools`, the equivalent CMake target, and all focused CTest world-tool entries from clean outputs.
- [ ] T083 [S0512] Run the repository-required `make test` followed by `make install`; confirm no root-level `circle` artifact remains.
- [ ] T084 [S0513] Hash-guard and run `validate --all`, `--mini`, representative `--zone`, `show`, and `refs` against development QST/HLQ data.
- [ ] T085 [S0514] Classify the live findings into validator defects, server-source hazards, and builder-owned data issues; fix only validator defects in scope.
- [ ] T086 [S0515] Record aggregate counts, elapsed time, peak memory, and before/after hash evidence without committing live data or sensitive paths.
- [ ] T087 [S0516] Boot the development server and playtest one QST and one HLQ flow after validator gates pass; record what static validation cannot prove.
- [ ] T088 [S0517] Update `docs/CHANGELOG.md`, final CLI examples, version output, and acceptance evidence.
- [ ] T089 [S0518] Remove this completed working plan and its ongoing-project index row only after all durable content and evidence have moved to permanent docs.

Session gate:

```sh
python3 scripts/world/wtool.py constants sync --check
python3 scripts/world/wtool.py docs --check
make test-world-tools
make test
make install
```

## 8. Acceptance Criteria

The project is complete only when all of the following are true:

- [ ] `DATA_EXTENSIONS` and every indexed/selected/explicit loading path include `qst` and `hlq`.
- [ ] Normal and mini validation require both quest indexes, while allowing valid empty quest datasets.
- [ ] `--zone` and the compatibility wrapper include selected `.qst` and `.hlq` packages.
- [ ] Both parsers match current Luminari loader grammar and current OLC writer output, including documented legacy forms.
- [ ] Structural corruption produces stable source-located findings and deterministic recovery/completeness.
- [ ] QST questmaster, targets, rewards, prerequisites, chains, dialogue, and type-specific scalar domains are validated.
- [ ] HLQ hosts, entries, commands, command direction, typed parameters, scalar bounds, and runtime order are validated.
- [ ] `show` and `refs` support `quest` and host-keyed `hlquest` in human and JSON output.
- [ ] Existing records show incoming quest-system references.
- [ ] Quest flags use one serialized token without changing the four-token behavior of existing sets.
- [ ] Quest types, flags, HLQ entry types, HLQ command codes, and required limits are source-derived and drift-checked.
- [ ] The complete tracked fixture has eight clean datasets with normal and mini indexes.
- [ ] Positive, negative, boundary, recovery, graph, lookup, reporting, CLI, and no-mutation tests pass.
- [ ] `Makefile.am` and `CMakeLists.txt` contain synchronized source, test, fixture, and support-file lists.
- [ ] Make, CMake, CTest, constants, documentation, wrapper, and GitHub Actions gates pass.
- [ ] Permanent QST and HLQ format guides exist and their bounded tables pass `docs --check`.
- [ ] `WORLD_VALIDATOR_CLI.md` accurately states eight-format coverage and remaining limitations.
- [ ] A hash-guarded development-world audit completes without changing a byte in either quest tree.
- [ ] Every observed live finding is classified; no known validator defect is left hidden as a data issue.
- [ ] Development boot and representative QST/HLQ playtests complete after static validation.
- [ ] No ignored live quest file, credential file, protected configuration header, or production system is modified.

## 9. Known Risks and Decisions Already Made

| Risk | Plan response |
|------|---------------|
| Live `hlq/index.mini` is absent. | Report the same missing-index error the server boot would encounter. Supply a valid tracked fixture index; do not create or repair the live one. |
| Live quest data is ignored and can change during implementation. | Treat current counts as a snapshot, use fixtures for gates, and hash/recount the live tree during final audit. |
| QST and HLQ loaders contain unsafe or destructive behaviors. | Preserve raw tokens and diagnose the hazard without reproducing unsafe dereferences, stale state, or loops. Server fixes require separate authorization. |
| QST flags are one token while current flag helpers assume four. | Make chunk count a per-set contract and lock all existing output with regression tests. |
| HLQ has no standalone quest VNUM. | Use the host mobile VNUM as the typed lookup key and nested composite ordinals for entries/commands. |
| HLQ load order reverses entries and input commands. | Preserve and expose both physical and effective runtime order; never silently sort nested content. |
| QEDIT accepts multi-kill in memory but Luminari cannot persist it. | Emit an error for persisted Luminari multi-kill records and document the source mismatch; do not invent a sixth string. |
| OLC spell bounds exceed runtime-safe HLQ bounds. | Treat the runtime `spell_info` bound as authoritative for safety and document the editor disagreement. |
| Quest chain checks can create noisy style findings. | Separate missing/dangerous edges from reciprocity/style advice and set severity only after source and live-corpus triage. |
| Live data will likely produce many reference findings. | Keep CI on clean tracked fixtures. Report live builder-owned findings separately; do not suppress errors or weaken rules to get a green full-world run. |
| Full graph work can become quadratic. | Index every typed target once, use adjacency maps for chain analysis, and measure full-world time/memory before closeout. |
| Adding parsers without docs/CI would create false confidence. | Parity and acceptance criteria require lookup, constants, docs drift, both build systems, CI, and operational evidence. |

## 10. Definition of Safe End-to-End Use After Completion

This expansion will make `wtool` a substantially stronger static oracle for
world content that includes both quest systems. It will be able to prove that
candidate flat files are structurally loadable, correctly indexed, internally
consistent, and connected to the typed world graph according to the current C
source.

It will not, by itself, prove gameplay balance, dialogue quality, reward
appropriateness, MariaDB-backed state, successful skill rolls, or every runtime
side effect. The safe builder loop remains:

1. author or stage content outside production;
2. run `validate` and resolve every error;
3. inspect normalized records with `show` and dependencies with `refs`;
4. review warnings and use `--strict` when a clean warning budget is required;
5. boot the development server; and
6. playtest QST acceptance/completion and HLQ ASK/GIVE/ROOM behavior.

An emitter that writes world files remains a separately reviewed project with
atomic staging, validation-before-success, backups, and an unmistakable opt-in
for any write beneath `lib/world/`.
