# Protocol Parser Harness

This document describes the focused source-side protocol parser harness for
Telnet, MSDP, GMCP, TTYPE, NAWS, unsupported option, and bounded response-path
validation.

The harness is a safety baseline for future protocol changes. It does not make
MCCP, GMCP modules, MXP browser UI, live `MINIMAP`, `QUEST_INFO`, `TITLE`,
saves, or `DAMAGE_BONUS` supported web-client features.

## Command

From the source checkout:

```sh
cd /home/aiwithapex/projects/Luminari-Source/unittests/CuTest
make protocol-parser
```

The target builds `test_protocol_parser.c` and `../../src/protocol.c`, then
runs `./protocol_parser_tests`.

The focused target uses GNU99 flags because the current source headers and
`protocol.c` use C99-style comments, mixed declarations, and newer compiler
assumptions. Existing CuTest targets keep their legacy C89 flags unchanged.

## Scope

The harness calls `ProtocolInput()` and selected public protocol helpers through
minimal source-compatible doubles:

| Double | Purpose |
|--------|---------|
| `struct descriptor_data` fixture | Holds `pProtocol`, output pointer, and source parser state. |
| `ProtocolCreate()` / `ProtocolDestroy()` | Allocates and releases real `protocol_t` state for each case. |
| `write_to_output()` stub | Captures protocol writes without opening a socket. |
| `basic_mud_log()` stub | Records visible rejection or guard-path logging. |
| `config_info` stub | Keeps source config reads deterministic and default-disabled. |

No MUD boot, database, player object, live socket, or private account data is
required.

## Session 04 Decision Implications

The harness is necessary evidence for future parser or protocol work, but it is
not sufficient to claim Luminari Web support for MCCP or GMCP.

- MCCP remains rejected by Luminari Web while `CompressStart()` and
  `CompressEnd()` are stubs and the proxy has no decompression layer.
- GMCP remains deferred for Luminari Web until source-owned modules, versions,
  schemas, proxy parsing, client mappings, MSDP coexistence behavior, fixtures,
  and rollback are planned.
- Any future MCCP or GMCP source change should extend this harness with
  synthetic fixtures before changing production negotiation or payload behavior.

## Privacy Rules

Use only synthetic byte fixtures committed in the harness source.

Do not use:

- Player commands.
- Passwords, tokens, keys, cookies, or credentials.
- Private hosts, IPs, ports, account names, or character names.
- Terminal transcripts or live captures.
- Redacted captures that still preserve real session timing or command text.

If a future case needs real-world shape, write a new synthetic fixture from the
protocol grammar and document the grammar source. Do not paste live bytes.

## Case Matrix

| Area | Harness Case | Current Coverage |
|------|--------------|------------------|
| Split IAC | `TestProtocolParser_SplitIacCurrentGap` | Documents the current source gap: split `IAC` is not retained across calls and the following bytes can become command output. |
| Doubled IAC | `TestProtocolParser_DoubledIacLiteral` | Validates `IAC IAC` becomes one literal 255 byte in command output. |
| Incomplete subnegotiation | `TestProtocolParser_IncompleteAndMalformedSubnegotiations` | Validates incomplete subnegotiation does not crash and leaves parser state visible. |
| Malformed MSDP | `TestProtocolParser_IncompleteAndMalformedSubnegotiations` | Validates malformed `REPORT` without `VAL` does not mark variables for reporting. |
| Malformed GMCP | `TestProtocolParser_IncompleteAndMalformedSubnegotiations` | Validates malformed GMCP payload is ignored without output. |
| TTYPE | `TestProtocolParser_TtypeAndNawsNegotiation` | Validates TTYPE request output, client ID storage, and xterm 256-color detection. |
| NAWS | `TestProtocolParser_TtypeAndNawsNegotiation` | Validates valid four-byte NAWS payload updates width and height. |
| Unsupported options | `TestProtocolParser_UnsupportedOptionNegotiation` | Validates unknown `WILL`/`DO` options produce deterministic `DONT`/`WONT` replies. |
| MSDP oversized response | `TestProtocolParser_OversizedResponsePaths` | Validates oversized MSDP list output is rejected and logged instead of emitted. |
| MXP oversized tag | `TestProtocolParser_OversizedResponsePaths` | Validates overlong MXP tags are returned unchanged. |
| Copyover string | `TestProtocolParser_OversizedResponsePaths` | Validates copyover protocol flags fit the bounded static buffer. |
| MSSP response | `TestProtocolParser_MsspResponseIsBounded` | Validates MSSP response framing is emitted and stays below `MAX_MSSP_BUFFER`. |

## Known Gaps

These are recorded as validation findings, not support claims:

- Split `IAC` state is not retained across `ProtocolInput()` calls. The harness
  documents current behavior so future parser hardening can change the expected
  outcome deliberately.
- Incomplete subnegotiation payload bytes are not retained across calls even
  though `bIACMode` remains set. Future hardening should add persistent
  subnegotiation length tracking.
- Short NAWS payloads are not separately asserted as safe because the current
  source parser reads four bytes without checking `aSize`. Add a failing or
  fixed expectation when that parser boundary is hardened.
- Direct full-table MSSP and MXP stress coverage is bounded to emitted response
  size and public helper behavior. Deeper string/allocation hardening belongs in
  the follow-up source hardening work identified by the Phase 04 backlog.
- MCCP compression start/end and proxy decompression behavior are outside the
  current harness because source compression is still stubbed.
- GMCP module schema validation is outside the current harness because source
  module ownership and payload contracts are not yet defined.

## Adding Cases

1. Add a synthetic fixture with `protocol_fixture_t` helpers.
2. Assert fixture construction did not overflow.
3. Use a fresh `protocol_harness_t` per case.
4. Assert exact output bytes, protocol state, or logged guard behavior.
5. Call `harness_destroy()` before returning.
6. Update this matrix and the Luminari Web protocol backlog if the support
   boundary changes.

Future parser behavior changes should update the current-gap tests to assert
the hardened outcome. Do not remove a gap without replacing it with an explicit
passing expectation.
