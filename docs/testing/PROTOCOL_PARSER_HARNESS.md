# Protocol Parser Harness

This document describes the focused source-side protocol parser harness and
bounded libFuzzer target for Telnet, MSDP, GMCP, MSSP, MXP, Unicode, TTYPE,
NAWS, unsupported option, and structured-onboarding paths.

Last verified: 2026-08-04

The harness is a safety baseline for future protocol changes. It does not make
MCCP, GMCP modules, MXP browser UI, live `MINIMAP`, `QUEST_INFO`, `TITLE`,
saves, or `DAMAGE_BONUS` supported web-client features.

## Command

From the source checkout:

```sh
cd /home/aiwithapex/projects/Luminari-Source/unittests/CuTest
make protocol-parser
```

The target builds `test_protocol_parser.c`, `../../src/net/protocol.c`, and the
production web-onboarding capability handler, then runs
`./protocol_parser_tests`.

The focused target uses the same GNU C23 mode as the server and the other
CuTest targets.

Run the allocation checks and bounded ASan/UBSan fuzz pass with:

```sh
make -C unittests/CuTest valgrind-protocol
make -C unittests/CuTest protocol-fuzz FUZZ_SECONDS=15
```

The fuzz target copies `fuzz_corpus/` to a temporary directory, mutates only
that copy, and enables sanitizer halt-on-error behavior. It exercises whole,
split, and small-chunk input together with output parsing, MSDP setters, MXP,
MSP, copyover, color, Unicode, and invalid-argument paths.

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
| Doubled IAC | `TestProtocolParser_DoubledIacLiteral` | Validates `IAC IAC` becomes one literal 255 byte in command output. |
| Telnet NUL | `TestProtocolParser_NulPaddingIsIgnored` | Validates NUL transport padding does not terminate or enter command text. |
| Split IAC | `TestProtocolParser_SplitIacIsRetained` | Validates negotiation state is retained across calls and following command text remains intact. |
| Incomplete subnegotiation | `TestProtocolParser_IncompleteAndMalformedSubnegotiations` | Completes an MSDP frame in a later call and validates the retained payload is processed. |
| Malformed MSDP | `TestProtocolParser_IncompleteAndMalformedSubnegotiations` | Validates malformed `REPORT` without `VAL` does not mark variables for reporting. |
| Malformed GMCP | `TestProtocolParser_IncompleteAndMalformedSubnegotiations` | Validates malformed GMCP payload is ignored without output. |
| Truncated lookahead | `TestProtocolParser_TruncatedLookaheadSequences` | Validates partial IAC, negotiation, and MXP prefixes stay bounded. |
| TTYPE | `TestProtocolParser_TtypeAndNawsNegotiation` | Validates TTYPE request output, client ID storage, and xterm 256-color detection. |
| NAWS | `TestProtocolParser_TtypeAndNawsNegotiation` | Validates valid four-byte NAWS payload updates width and height. |
| Short payloads | `TestProtocolParser_ShortSubnegotiationsAreIgnored` | Validates short NAWS and empty CHARSET payloads are ignored without state changes. |
| Unsupported options | `TestProtocolParser_UnsupportedOptionNegotiation` | Validates unknown `WILL`/`DO` options produce deterministic `DONT`/`WONT` replies. |
| MSDP/GMCP coexistence | `TestProtocolParser_GmcpAndMsdpCanCoexist` | Validates GMCP negotiation does not clear an active MSDP state. |
| Web onboarding capability | `TestProtocolParser_WebOnboardingCapability` | Validates the reserved MSDP capability reaches the production handler and records the negotiated version. |
| Web onboarding action | `TestProtocolParser_WebOnboardingActionUsesReservedVariable` | Validates the reserved v2 action variable reaches the isolated production handler without becoming command output. |
| Initialization | `TestProtocolParser_CreateInitializesAllParserState` | Validates parser state and every MSDP allocation are zero-initialized. |
| Error returns | `TestProtocolParser_NullAndInvalidMsdpInputsAreSafe` | Validates public null and invalid inputs return the standardized error codes. |
| Graceful overflow | `TestProtocolParser_GracefulTruncationKeepsConnectionUsable` | Validates oversized commands, subnegotiations, MXP responses, and output are bounded while later input remains usable. |
| MSSP pair length | `TestProtocolParser_MsspPairLengthIsCheckedBeforeAppend` | Validates an overlong runtime MSSP value is rejected before it can modify the destination buffer. |
| Unicode fallback | `TestProtocolParser_UnicodeFallbackHasValidLifetime` | Validates ASCII fallback and UTF-8 substitution remain valid after output parsing. |
| MSDP oversized response | `TestProtocolParser_OversizedResponsePaths` | Validates maximum-length strings work and overlong strings, tables, arrays, and lists are rejected. |
| MXP oversized tag | `TestProtocolParser_OversizedResponsePaths` | Validates maximum-length tags are formatted and overlong tags are returned or rejected without copying. |
| Copyover string | `TestProtocolParser_OversizedResponsePaths` | Validates copyover protocol flags fit the bounded static buffer. |
| MSSP response | `TestProtocolParser_MsspResponseIsBounded` | Validates MSSP response framing is emitted and stays below `MAX_MSSP_BUFFER`. |
| MSDP reporting | `TestProtocolParser_SelectedMsdpVariablesCanBeReported` | Validates selected numeric and string values are emitted and cleared from dirty state. |
| Mudlet identity | `TestProtocolParser_MudletPackageUsesStableIdentity` | Validates the package URL and version identity contract. |

## Known Gaps

These are recorded as validation findings, not support claims:

- MCCP compression start/end and proxy decompression behavior are outside the
  current harness because source compression is still stubbed.
- GMCP module schema validation is outside the current harness because source
  module ownership and payload contracts are not yet defined.
- The fuzzer is intentionally time-bounded and uses only synthetic seeds; it is
  regression evidence, not an exhaustive proof over every byte sequence.

## Adding Cases

1. Add a synthetic fixture with `protocol_fixture_t` helpers.
2. Assert fixture construction did not overflow.
3. Use a fresh `protocol_harness_t` per case.
4. Assert exact output bytes, protocol state, or logged guard behavior.
5. Call `harness_destroy()` before returning.
6. Update this matrix and the Luminari Web protocol backlog if the support
   boundary changes.

Future parser behavior changes should replace affected expectations with an
explicit passing outcome and extend the synthetic fuzz corpus when a compact
regression seed is useful.
