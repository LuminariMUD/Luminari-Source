# Screen-reader and MSP acceptance evidence

Date: 2026-09-08. Branch: `feature/screen-reader-msp`.
Implementation: `09d144587e1b55d2312f76c0ea470a10eeb5c727`.
Additional regressions and this report are committed together after that revision.
Issue: https://github.com/LuminariMUD/Luminari-Source/issues/137.

## Automated evidence

- `make test`: 1,269 production-linked tests pass; configured auxiliary checks
  pass. Eight optional database cases remain skipped. Followed by `make install`.
- Final `make -j8 cutest` and `./cutest`: 1,269 pass, including creation save-failure
  recovery retaining the screen-reader choice after recommended settings.
- `make -C unittests/CuTest protocol-parser`: 31 pass.
- Production-linked tests cover actual save/reload, absent legacy preference
  defaults, failed command and PREFEDIT saves with rollback, real normal/wilderness
  room rendering, gameplay prompt suppression with IAC GA retained, pager/editor
  instructions, and successful/failed/repeated/muted door cues.
- Onboarding tests cover invalid/yes/no choices, navigation, preservation of
  underlying preferences, v2 structured choice and v1 terminal fallback.
- Protocol tests exercise MSP wire conversion, missing capability, saved opt-out,
  legacy client-variable bypass prevention, and malformed/oversized/raw triggers.
- Exact SQL help migration ran twice against session-temporary schema copies;
  two texts and four aliases matched. Temporary tables omit only the FULLTEXT
  index, which temporary InnoDB tables do not support. Separately verified exact
  flat-file/development-database text parity. Production was not accessed.

Temporary execution logs: `/tmp/screen-reader-final-test.log`,
`/tmp/screen-reader-final-cutest.log`, `/tmp/screen-reader-protocol.log`.
These logs are supporting local artifacts, not a permanent CI archive.

## Live development evidence

Used `autorun.sh` on the former development port with `APP_ENV=development`.
Runtime implementation revision was `09d144587` (clean), ELF build ID
`c8265e1335b2d92237bf2d834acc3565cccc5653`.

- Created synthetic characters with screen-reader yes and recommended preferences
  both no and yes; entered the world and logged out cleanly. Reconnected with mode
  retained and useful `hp`, `moves`, and `tnl` text.
- Disconnected an unsaved draft at identity selection twice. Both new creation
  attempts asked the screen-reader question; no draft player file was created.
- Normal room description/exits remained readable without automatic maps.
  Wilderness under darkness retained its visibility restriction. With the test
  staff character's holylight enabled, forest narrative and survey were retained;
  explicit map produced art, mode off restored automatic art/prompts, and mode on
  restored narrative-only automatic output.
- Raw binary `nc` connection accepted MSP (DO option 90), enabled saved sound,
  survived copyover, reported capability and retained screen-reader mode.
  `sound test` emitted `!!SOUND(luminari-test.wav)`; after `sound off`, it emitted
  explanatory text and no trigger. This checks server bytes, not client audio.
- The existing creation helper now accepts `DEV_MUD_SCREEN_READER` and
  `DEV_MUD_RECOMMENDED_PREFS` (yes/no, default no). The configured legacy login
  fixture was absent from the live account menu. For these probes, a temporary
  helper copy skipped only that fixture preflight after verifying autorun was
  already listening; account authentication and creation were exercised normally.

## Final runtime limitation

After the recorded successful probes, the managed listener on the former development port disappeared
while another local process attempted server startup on 4100. A later optional
help smoke invoked the existing helper's automatic development service startup,
then failed to find the synthetic character in its account menu. This later
check is not a pass. The task-started development login service was no longer loaded when cleanup
attempted to stop it.
Concurrent changes now explicitly require game port 4100 only. Further server changes and synthetic-character cleanup
are paused pending coordination with the other local session. The synthetic
`Accessprobe` and `Accessrecs` files remain; `Accessprobe` was temporarily promoted
to level 31 for copyover testing. Restore or remove only these task fixtures after
coordination. No final installed/running revision match is claimed.

## Follow-up client validation and release

The user clarified that this task is complete when all code is in place; the
following client checks and historical fixture cleanup do not block code completion.
Implementation and regression commits `09d144587` and `fe53ec808` are published.
For follow-up validation, record client and screen-reader names and versions,
then exercise creation, town/wilderness movement, status queries, combat,
pager/editor, reconnect and mode changes. Record usability findings.

A real MSP-capable client must play both bundled cues, then remain silent after
mute and reconnect. Test missing-file behavior and document the exact client
installation path. No real screen-reader or audible client test was performed;
protocol tests and socket captures do not substitute for those checks. Opposite
preferences on two simultaneous real clients also remain a useful acceptance check.

Assets and installation instructions: `lib/sounds/README.md`. The two original
synthesized WAV files are distributed in `lib/sounds`; no external sound hosting
was deployed. The SQL help artifact is `sql/components/help_screen_reader_msp.sql`.

Deployment is separate. Before release, record the accepted revision and client
results and deploy matching binary, sound assets and help. Rollback uses the
previous binary and reviewed help restoration; the development pre-change help
backup is `/tmp/luminari-screen-reader-help-before.json`. Retain appended flag
identifiers 87/88 for their assigned meanings; do not reuse them for other features.
