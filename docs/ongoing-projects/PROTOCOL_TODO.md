# Protocol System TODO - Security and Robustness

Working notes for `src/net/protocol.c` (3836 lines) and `src/net/protocol.h`.

**Re-verified against source on 2026-08-03.** The previous revision of this
file was written against an older tree and had drifted badly: it listed three
CRITICAL remote-code-execution buffer overflows that are all fixed, and every
line number in it was wrong. Anyone following it would have been sent to
re-fix working code. What follows is only what the current source actually
shows. Line numbers below are from the 2026-08-03 tree and will drift again -
confirm before acting.

## Already addressed - do not re-open

Recorded so a future audit does not re-file them.

| Item | Evidence |
|------|----------|
| Bounds checking in `ProtocolInput` IAC/IAC parsing | `protocol.c:664` - `apData[Index] == IAC && Index + 1 < aSize && apData[Index + 1] == IAC` |
| Bounds checking in IAC/SE parsing | `protocol.c:675` - same `Index + 1 < aSize` guard |
| Bounds checking in MXP escape parsing | `protocol.c:687` - `Index + 3 < aSize` guards the whole `\033[<digit>z` lookahead |
| Null validation at the `ProtocolInput` entry point | `protocol.c:638-644` - rejects null `apData`/`apOut` and non-positive `aSize`, with a defined fallback path when `pProtocol` is null |
| Per-descriptor protocol buffers | `protocol.h:746` - `char CmdBuf[MAX_PROTOCOL_BUFFER + 1]` on the descriptor |
| Buffer overflow detection | Present throughout; still drops the connection rather than degrading - see item 4 below |
| Guarded `strcat` usage | All 13 call sites are length-checked before the call: `protocol.c:3017-3031` and the surrounding MSDP list builders, `protocol.c:3655-3680` for MSSP. Each failure path calls `ReportBug`. |

The `strcat` entry is the important correction. The old file called these
"unsafe string operations - buffer overflow attacks, HIGH PRIORITY". They are
bounded. Converting them to `strncat` is a readability change, not a fix, and
should not be scheduled as security work.

## Open items

### 1. Unbounded `sprintf` into `MSSPPair[128]`

`protocol.c:3361` declares `char MSSPPair[128]`. `protocol.c:3651` fills it
with `sprintf(MSSPPair, "%c%s%c%s", MSSP_VAR, MSSPTable[i].pName, MSSP_VAL,
...)` where the value is either a table constant or the return of an MSSP
callback. The overflow check at `protocol.c:3656` runs *after* the write, so
it bounds `MSSPBuffer` but not `MSSPPair`.

Any MSSP value longer than ~120 bytes overruns a stack buffer. Several values
come from server configuration rather than compile-time constants, so the
input is staff-settable rather than attacker-settable - which is why this is
first but not critical.

Fix: `snprintf` with `sizeof(MSSPPair)`, and check the truncation return.

### 2. Bare `sprintf` audit - 11 remaining sites

`protocol.c` uses `snprintf` 23 times and `sprintf` 11 times. The formats are
all fixed strings, so these are not format-string vulnerabilities - the risk
is destination sizing only. Sites, with the destination each writes to:

| Line | Destination | Note |
|------|-------------|------|
| 1209 | `static char Buffer[64]` (1203) | two ints, safe by construction |
| 1758, 1776, 1809 | `char MXPBuffer[1024]` | writes caller-supplied `apTag`; needs a length audit of the callers |
| 1838 | heap `pBuffer` | sized from the trigger string |
| 3339, 3346 | `static char Buffer[64]` | int and uptime, safe |
| 3644, 3651, 3672 | `MSSPBuffer` / `MSSPPair` | 3651 is item 1 above |

Convert all to `snprintf` for uniformity; the only one carrying real risk is
3651, with 1758/1776/1809 needing the caller audit before being called safe.

### 3. `malloc` without zero-initialization - 8 sites

`protocol.c:328, 357, 367, 591, 1661, 1707, 3184, 3823` use `malloc`; only one
`calloc` appears in the file. Each site currently relies on manual field
initialization. Converting to `calloc` removes a class of uninitialized-read
bug and is mechanical.

`AllocString` (`protocol.c:3815-3836`) additionally computes `int Size =
strlen(apString)` with no maximum. It is called with internal constants today,
so it is not currently reachable with hostile input; bound it anyway if it
ever takes network data.

### 4. Graceful buffer-overflow handling

Overflow is detected but handled by dropping the connection. Degrading -
truncating the affected message and continuing - is friendlier and removes a
cheap disconnect vector. Behavioral change; needs a decision before work.

### 5. Standardized error return codes

Return conventions are inconsistent across the file: some functions return
`-1`, others `0`/`1`, others `TRUE`/`FALSE`. No standard error enum exists.
This is a maintainability item, not a security one.

### 6. Comprehensive null validation

Entry-point validation exists in the main parse path (see the table above) but
is not applied consistently across all exported functions. Worth a sweep; low
risk given current call sites are all internal.

## Testing

The focused harness already exists and is the right place for regression
coverage on anything changed here:

```bash
cd unittests/CuTest
make protocol-parser
make valgrind-protocol
```

Not yet covered: fuzzing of the parse entry points, and MSSP value-length
cases for item 1.

## Priority

1. Item 1 - the one real memory-safety defect remaining.
2. Item 2's MXP caller audit (1758/1776/1809).
3. Item 3 - mechanical, low risk.
4. Items 4-6 - quality and consistency; schedule when the file is open anyway.

Nothing here is remote-code-execution class. The previous revision's
"immediate action required" assessment no longer reflects the source.
