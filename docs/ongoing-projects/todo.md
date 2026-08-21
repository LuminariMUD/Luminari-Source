# Copyover Code Issues Observed on 2026-08-19

## Scope

This report contains code or runtime issues observed while monitoring the
production copyover started at 16:50:22 UTC. WORLD data loading completed
without a WORLD/lib diagnostic.

## Status

All three items were verified against the source and addressed. Items 1 and 3
were real defects and are fixed. Item 2 was a diagnosability gap rather than a
defect; the requested profiling was added and no behavior was changed.

## 1. Copyover logs the previous executable's ELF build ID

Severity: Medium. This makes the startup identity line unreliable after a
copyover and can misidentify the binary involved in a later crash or incident.

Observed evidence:

- The pre-copyover process was release
  `70bb9124e49380410784c680da21574692d7e965`.
- `log/copyover_diagnostic.log` records that the 16:50:22 copyover executed the
  server in release `94e743c79dab9e17a38c77e5123a2e0342c0b3b4`.
- The new release manifest and `readelf -n` both identify the running binary as
  `94e743c79dab9e17a38c77e5123a2e0342c0b3b4` at git commit
  `52094f8f66342a0ee3c7413c74291e024803eb60`.
- `/proc/2487/exe` resolves to the new `94e743...` release, but the inherited
  process environment still contains
  `LUMINARI_ELF_BUILD_ID=70bb9124e49380410784c680da21574692d7e965`.
- The post-copyover startup line consequently reports the new git commit with
  the old ELF build ID:

  ```text
  Build identity: git_commit=52094f8f66342a0ee3c7413c74291e024803eb60 git_dirty=1 elf_build_id=70bb9124e49380410784c680da21574692d7e965
  ```

Code trace:

- `scripts/autorun/autorun.sh:1210` sets `LUMINARI_ELF_BUILD_ID` when autorun
  initially launches the MUD.
- `src/act.wizard.c:7125` uses `execl()` for copyover, which preserves the old
  environment.
- `src/comm.c:456-461` trusts `LUMINARI_ELF_BUILD_ID` when it writes the startup
  identity line.

Suggested correction:

Refresh the environment variable from the resolved copyover executable before
`execl()`, or derive and validate the running executable's build ID during
startup. Add a copyover test that starts release A, activates release B, and
asserts that release B logs its own ELF build ID.

Resolution (fixed):

- Added `src/elf_build_id.c` / `src/elf_build_id.h`. `get_self_elf_build_id()`
  walks the running image with `dl_iterate_phdr()` and reads the
  `NT_GNU_BUILD_ID` note from the main executable's `PT_NOTE` segment. The
  module is a separate translation unit so that `_GNU_SOURCE` does not leak
  into `comm.c`.
- `src/comm.c` now prefers the derived value, falls back to
  `LUMINARI_ELF_BUILD_ID` only when the note cannot be read, refreshes the
  environment variable so later execs inherit the correct value, and logs an
  informational line when the inherited value did not match.
- `unittests/CuTest/test_elf_build_id.c` asserts the value is readable,
  lowercase hex, and unchanged when `LUMINARI_ELF_BUILD_ID` is set to a
  different string. The derived value was also confirmed to match
  `readelf -n` on the same binary.

## 2. Login recovery caused main-loop latency warnings

Severity: Low to Medium. The observed pauses were short, but they blocked the
single-threaded command-processing loop beyond its 100 ms pulse budget.

Observed evidence:

- At 16:48:38 UTC, crash-save recovery produced a 417,855 usec pulse
  (417.86 percent of budget). `Process Commands` consumed 413,574 usec.
- At 16:52:17 UTC, after the copyover, another crash-save recovery produced a
  207,317 usec pulse (207.32 percent). `Process Commands` consumed 206,163
  usec. The visible `save.character` sample accounted for 23,270 usec, leaving
  most of the command time unattributed by the detailed profiler sections.
- The MUD caught up successfully after both pauses, with no dropped catch-up
  work reported.

Code trace:

- `src/comm.c:1200-1210` emits the pulse-latency warning.
- `src/comm.c:1329` begins the broad `Process Commands` profiling section.
- `src/players.c:2299` profiles character saving, but the remaining recovery
  work is not broken down enough to identify the dominant operation from this
  log alone.

Suggested follow-up:

Add profiling around crash/rent object loading and the login recovery command
path, then reproduce with both a small and a large saved inventory. Optimize
only after the dominant operation is identified.

Resolution (profiling added, no behavior change):

- `Crash_load()` in `src/obj/objsave.c` is now profiled as
  `load.crash_objects`.
- `enter_player_game()` in `src/interpreter.c` is profiled as
  `login.enter_game`, with inner sections `login.load_pets` and
  `login.init_classes`.

This was an attribution gap, not a defect: the pauses were real work inside
`Process Commands` that the profiler did not break down. The next latency
warning during login recovery will name the dominant operation. No
optimization was attempted, per the report's own guidance.

## 3. Mobile 110600 receives two different legacy special procedures

Severity: Low. Boot is deterministic and remains operational, but one legacy
assignment is silently made ineffective and the intended behavior is unclear
without reading assignment order.

Observed evidence:

- The effective-binding audit records `shadowdragon` as selected for mobile
  110600, followed by `dracolich_mob` overriding it.
- The final effective handler is `dracolich_mob` with one collision.
- `lib/world/mob/1106.mob` identifies mobile 110600 as Aurgloroasa, a former
  shadow dragon that became a dracolich, so either behavior may have been
  deliberately associated with the prototype. The world record itself does
  not author a named special procedure.

Code trace:

- `src/spec/spec_assign_mobiles.c:537` assigns `shadowdragon` to VNUM 110600.
- `src/spec/spec_assign_mobiles.c:745` later assigns `dracolich_mob` to the
  same VNUM, replacing the first callback.

Suggested follow-up:

Confirm whether Aurgloroasa needs dracolich behavior only or a deliberate
combination. Remove the obsolete assignment if only one handler is intended;
otherwise implement explicit composition rather than relying on overwrite
order.

Resolution (fixed, runtime behavior unchanged):

- Removed the unreachable `ASSIGNMOB(110600, shadowdragon)` and replaced it
  with a comment recording why. `dracolich_mob` was already the effective
  handler, so the live behavior of Aurgloroasa is identical; only the silent
  collision is gone.
- The `shadowdragon` callback itself was left in place. It is now unassigned
  and available if a deliberate composition or a different shadow dragon is
  wanted later; that is a design decision, not a code defect.
- `unittests/CuTest/test_spec_effective_binding.c` pins the total assignment
  token count, which moved from 784 to 783.
