# Known Issues

Intentional exceptions and legacy technical debt tracked in repository documentation.

## Formatting (clang-format) - RESOLVED

### Codebase Format Status

**Status**: RESOLVED (2025-12-30)

**Original Claim**: "~118,000+ formatting violations in legacy code"

**Actual Status After Audit**:
```
Git-tracked source files:     375
Files with format violations:   0
Total diff lines:               0
```

**Finding**: The codebase is already fully formatted according to `.clang-format`. The original claim was either:
1. Fixed incrementally over time through pre-commit hooks
2. The `.clang-format` was designed to match existing style precisely

**Remaining Items** (NOT in git, local only):
- `campaign.h`, `mud_options.h`, `vnums.h` - local config copies have minor differences
- These are copied from `.example.h` templates and are user-specific

**Current Enforcement**:
- Pre-commit hooks validate all new/changed code
- CI quality.yml checks formatting on PRs to master/main

## Linting (clang-tidy)

### Disabled Checks

The following checks are intentionally disabled in `.clang-tidy`:

- `bugprone-macro-parentheses` - MUD codebase uses macros extensively in expected patterns
- `bugprone-reserved-identifier` - Legacy naming conventions
- `bugprone-easily-swappable-parameters` - Common in game dev APIs
- `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling` - Uses snprintf, acceptable
- `clang-analyzer-security.insecureAPI.strcpy` - Legacy code, gradually migrating

## Build (CMake cutest target) - RESOLVED

### Legacy Integration Tests

**Status**: RESOLVED (2025-12-30)

**Original Issue**: CMake `cutest` target failed to link because legacy integration tests (`test.interpreter.c`, `test_bounds_checking.c`, `test.helpers.c`) referenced main library functions (`three_arguments`, `get_wearoff`, `one_argument`, etc.) without proper linking.

**Resolution Applied**:
- Disabled CMake `BUILD_TESTS` option by default (set to OFF)
- Removed broken cutest target from CMake build
- Added documentation pointing users to the working Makefile tests
- CI workflow (`test.yml`) already uses Makefile tests, which remain fully functional

**Current State**:
- CMake builds main binary `bin/luminari` successfully
- Production-linked unit tests run via `make test`
- The focused protocol parser harness runs via
  `make -C unittests/CuTest protocol-parser`
- Legacy standalone vessel and vehicle mirror sources have been removed

## Production Health Activation

The loopback health endpoint and rendered systemd readiness probe passed against an isolated local
MariaDB runtime on 2026-08-07. This development checkout must not install or restart the production
service. After the change reaches an approved production release, install or refresh the canonical
systemd unit, restart the service, and verify readiness with
`scripts/operations/healthcheck.sh`. Remove this entry after that production probe passes.

## World Data Loading (`src/db.c`)

Found while auditing the world-building documentation on 2026-08-04. All four
are latent source bugs, not documentation errors. They are described from the
builder's side in
[ZONE_FILE_FORMAT.md](world_game-data/ZONE_FILE_FORMAT.md).

### Zone reset `case 'I'` and `case 'R'` are unreachable

`load_zones()` dispatches with
`if (strchr("MOGEPDTVJL", ZCMD.command) == NULL)`, sending anything not in that
string to a generic three-argument branch. `I` and `R` are absent from the
string, so their `case` labels in the switch below are dead code.

For `R` this is harmless - the generic branch parses exactly the three
arguments the `case` would have. For `I` it is not: the `case` expects two
arguments while the generic branch requires three, so a two-argument `I` line
fails to parse and aborts the boot.

**Fix**: add `I` and `R` to the dispatch string, or delete the unreachable
cases and document the three-argument form as intended.

### Zone reset command `L` is non-functional

`load_zones()` parses only `arg1` and `arg2` for `L`, but `reset_zone()` reads
`ZCMD.arg3` as the container to fill. `arg3` is never assigned, so it is always
zero. Separately, the call that would place the treasure
(`load_treasure_in_obj()`) is commented out and marked `Unfinished`. `L` either
does nothing or logs `ZONE ERROR: target obj not found`.

**Fix**: finish or remove the command. As shipped it is a trap for builders.

### Zone header field counts degrade silently

`load_zones()` tries the numeric header at exactly 14, then 11, then 10, then 4
fields. A header with 12 or 13 fields fails the 14-field scan and succeeds at
11, silently discarding `region`, `faction`, and `city`. A header with 5 to 9
fields degrades to the 4-field form, losing zone flags and the level range.
Neither case logs anything.

**Fix**: log a warning when the matched field count is lower than the field
count actually present on the line.

### Zone command prescan disagrees with the parse loop

The counting pass tests `buf[0]` directly:

```c
if ((strchr("MOPGERDTVJIL", buf[0]) && buf[1] == ' ') || (buf[0] == 'S' && buf[1] == '\0'))
```

The parse loop calls `skip_spaces()` first. An indented reset command is
therefore parsed but not counted, and the boot dies with
`SYSERR: Zone command count mismatch` - a message that does not point at the
offending line.

**Fix**: apply `skip_spaces()` in the prescan, or report the first line whose
form the prescan rejected.

### Dangling room exits are nulled without a log line

`renum_zone_table()` logs `ZONE ERROR` for object and mobile vnums it cannot
resolve. `renum_world()` does not: an exit whose `to_room` does not exist is
quietly rewritten to `NOWHERE`. A door that leads nowhere after a clean boot
produces no diagnostic at all.

**Fix**: log the room vnum, direction, and unresolved destination.

---

*Last updated: 2026-08-07*
*Maintained by repository documentation audits*
