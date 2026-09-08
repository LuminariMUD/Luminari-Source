# Screen-reader setup and optional MSP sound implementation plan

Status: planned; implementation and runtime acceptance have not started.
Created: 2026-09-08.
Issue: [#137](https://github.com/LuminariMUD/Luminari-Source/issues/137).
Source investigation: `5d95d355822c91e7c0a1deaf6fc38f1adbadd26a`.

## Outcome and scope

Offer a plain-language screen-reader choice early in character creation. An
opted-in character receives room descriptions without automatic ASCII maps and
without repeated gameplay prompts from the first room onward. Persist the choice
and provide an accessible way to change it later. Add independently optional,
persistent sound controls and demonstrate working MSP audio with a test cue and
one useful gameplay cue.

This plan implements #137. Coordinate with #127 for automatic screen-reader
negotiation and account-wide preferences, and #126 for linear SKORE output.
Neither is a prerequisite for this work. A complete presentation rewrite, a
general audio framework, music/ambience libraries, and production deployment are
outside this implementation scope.

### Plan ablation

Reuse player preference bits, existing checked saves and creation recovery,
map-free room descriptions, and `SoundSend()`. Do not add a second preference
store, account schema migration, parallel renderer, or new protocol stack.
Keep real-client playback and screen-reader acceptance: protocol support and
static source inspection do not establish usable audio or accessible output.

## Verified starting points

| Area | Existing behavior and implementation touchpoints |
| --- | --- |
| Creation | `src/interpreter.c`: name confirmation proceeds to sex/race/class; `init_char()` runs after alignment; `CON_SETPREFS` applies recommended settings. |
| Defaults | `src/db.c`: `init_char()` enables automap and HP/movement/action prompt fields. Recommended preferences enable additional prompt fields. |
| Recovery | `src/character/character_creation.c` and `.h`: core creation navigation, checked stage saves, restart, and durable creation resume. |
| Web onboarding | `src/net/onboarding.c`: terminal states map to structured screens; preserve older-client fallback. |
| Preferences | `src/structs.h`, `src/constants.c`, `src/players.c`, and `src/olc/prefedit.c`: numbered flags, names, `Pref` persistence, and editor copies. |
| Maps | `src/act.informative.c` and `src/asciimap.c`: automap gates; wilderness with automap disabled already uses generated descriptions. |
| Prompts | `src/act.other.c`: `do_display()` and `is_prompt_empty()`; `src/comm.c`: `make_prompt()` clears the final PC prompt when empty. |
| Status text | `src/act.informative.c`: `hp`, `moves`, `tnl`, and `survey` implementations; registrations in `src/interpreter.c`. |
| Sound | `src/net/protocol.c`: MSP negotiation, `eMSDP_SOUND`, `SoundSend()`, and MSDP/GMCP `PLAY_SOUND` routing; no gameplay C callers found in the investigation. |
| Sound UI | PREFEDIT option R mutates descriptor capability `bMSP`, not the separate sound-enable gate or a durable player setting. |
| Help | `lib/text/help/help.hlp`: SCREEN-READER recommends automap off and `prompt none`; update the database entry too. |

`prompt none` already suppresses the final PC prompt. Its clearing list and
`is_prompt_empty()` both omit gold/time flags. Fix that consistency without
misreporting the existing command as unable to suppress output.

## Behavioral decisions

### Screen-reader preference

Use a persistent character preference, provisionally `PRF_SCREEN_READER`, as an
effective output override. While enabled, automatic maps and gameplay prompts
are suppressed regardless of underlying map/prompt settings. Preserve those
underlying settings so `screenreader off` restores their current configured
behavior without a snapshot or a wholesale reset. `screenreader on`, `off`, and
`status` are explicit, repeatable operations; no argument displays status and
usage. Explain that map/prompt changes made while the mode is on take effect
when it is turned off.

Do not change brief mode, channels, combat-roll verbosity, colors, or unrelated
preferences implicitly. Preserve full descriptions and useful navigation text.
Manual map requests remain explicit. This mode is a focused output preference,
not a claim that every existing menu is screen-reader accessible.

The creation question should read along these lines:

> Would you like screen-reader-friendly output? (yes/no)
> This hides automatic ASCII maps and repeated gameplay prompts. Room text
> remains available. You can change this later with screenreader on or off.

Present the question after name confirmation and before sex/race/class menus.
Use plain text and explicit words for answers and errors. Do not ask players to
disclose a disability. Answering no preserves ordinary presentation; it does not
enable sound. Existing characters default to mode off and are not silently changed.

### Creation persistence boundary

Before the initial character save, the existing name/race/class flow is not a
durable resumable character. Hold the answer on the in-progress character and
explicitly preserve it across `init_char()`. A disconnect before that first save
restarts the existing flow and asks the question again; do not introduce partial
player records solely for this preference.

The initial checked save after alignment must include the preference. Thereafter,
reconnect/resume preserves it through recommended preferences and roleplay stages.
Save failures must not claim durable success. Explicit Start over discards the
old choice with the old character. Back navigation must retain the choice and
provide a route to revise it. Keep existing persisted creation-stage numbers
stable; this early question does not require inserting a new durable stage.

### Sound preference and transport

Use an independent persistent opt-in flag, provisionally `PRF_SOUND`. Add
`sound on`, `off`, `status`, and `test`; no argument reports status and usage.
Status distinguishes saved consent, negotiated client capability, and usable
transport. Sound defaults off for new and existing characters. `sound test`
does not enable sound implicitly. Unsupported clients receive an explanatory
text response to that command, not raw audio triggers.

Treat negotiation as capability, never as consent. Make saved player opt-out
authoritative even if a client changes `eMSDP_SOUND`. Reconcile descriptor state
on login, reconnect, copyover, and preference changes. Convert PREFEDIT's MSP
entry into a clearly named player sound control with separate capability status;
it must not let players fabricate negotiated capability.

Retain the existing sound helper but require both consent and a verified route.
Do not assume general GMCP/MSDP support implies `PLAY_SOUND` support. Inspect
client documentation and actual traffic before retaining that route. Use MSP
for clients that negotiate MSP when an alternative audio route is unverified;
emit no audio when no usable route exists. Check `Write()`/`ProtocolOutput()`
interaction so MSP reaches the wire as `!!SOUND(...)`, not internal color syntax.
Do not add a new media protocol as a shortcut.

## Sequenced implementation checklist

### 1. Establish shared preference behavior

- [ ] Recheck the relevant call paths against the implementation branch and
  record any drift from the investigation above.
- [ ] Allocate unused preference bits without renumbering existing flags; update
  `NUM_PRF_FLAGS`, `preference_bits`, and any enumerating/editor tables. Confirm
  capacity and old-file defaults through the existing `Pref` load/save path.
- [ ] Implement a small shared effective-automap predicate and gameplay-prompt
  suppression check in existing appropriate files. Apply the map predicate to
  both display selection and the wilderness text fallback, avoiding a blank
  description when the underlying automap bit is still on.
- [ ] Register screen-reader and sound commands in `src/interpreter.c`, declare
  them in `src/interpreter.h`, and implement them in existing command files.
  Use checked persistence with truthful failure feedback and rollback where
  required by the current save contract. Guard NPCs and missing descriptors.
- [ ] Ensure PREFEDIT's copied flags and save operation retain both new settings
  and cannot override mode behavior accidentally.

Checkpoint: old characters load unchanged; both preferences round-trip; changing
one does not change the other or unrelated preferences.

### 2. Integrate creation and structured onboarding

- [ ] Add the early terminal connection state using an unused identifier; update
  state names and every relevant dispatch, connection-state check, and creation
  navigation path discovered by reference search.
- [ ] Accept explicit yes/no answers, repeat the question on invalid input, and
  preserve the answer across initialization and recommended preferences.
- [ ] Integrate Back and Start over behavior and the durable boundary described
  above with `character_creation.c`; retain checked save/account-link recovery.
- [ ] Add the structured choice screen and action validation to
  `src/net/onboarding.c`. Follow its versioning contract if the new screen needs
  a protocol-version change; preserve readable terminal fallback for older clients.
- [ ] Deliver a short plain-text introduction before first room output, mentioning
  `help screen-reader`, `hp`, `moves`, `tnl`, `survey`, and the two mode commands.
  Do not repeatedly print it on every reconnect.

Checkpoint: yes/no paths work in terminal and structured onboarding; the initial
saved character contains the choice, and resumed creation does not undo it.

### 3. Complete map and prompt integration

- [ ] Trace all automatic map callers, including room look and movement output.
  Apply effective mode behavior while leaving explicitly requested maps available.
- [ ] Preserve normal-room descriptions, wilderness generated descriptions,
  exits, and existing visibility restrictions. Do not reveal hidden navigation
  information or alter in-game blindness mechanics.
- [ ] Suppress the final gameplay prompt in screen-reader mode, including combat,
  wait, and status additions. Preserve pager/editor/creation instructions and
  protocol-level prompt delimiters needed by clients.
- [ ] Make `prompt none` and prompt-emptiness handling consistent for all prompt
  field flags, including gold/time; verify gold-only and time-only prompts still
  work outside screen-reader mode.

Checkpoint: first-room and movement output remain informative without automatic
art, and mode off restores the player's underlying map/prompt behavior.

### 4. Connect sound controls to a working audio path

- [ ] Correct `SoundSend()` gating and transport selection as specified above;
  inspect explicit in-band sound paths so they cannot bypass player opt-out.
- [ ] Keep capability reporting accurate and prevent negotiation from overriding
  the saved preference. Verify MSP refusal and reconnect/copyover behavior.
- [ ] Add a short sound-test cue and one gameplay cue. Prefer a successful
  player-operated door opening: first trace its authoritative success branch,
  send only to the acting player, and preserve the existing text. Do not attach
  the cue to failed attempts or every movement tick.
- [ ] Inspect existing media assets and provenance, including
  `docs/media-gen/elevenlabs-onboarding-sfx-catalog.json`, before selecting files.
  Reuse assets only if redistribution rights and playback format are established.
  Record filename, source/license, attribution requirements, and distribution.
- [ ] Package the two required cues with documented client installation or a
  verified existing public asset location. Do not invent a download URL or
  publish a new hosting service as part of a code change.
- [ ] Use bounded, known cue names and preserve helper input limits. Missing or
  unsupported assets must not interrupt gameplay; document that server emission
  does not prove playback and give a sound-test troubleshooting procedure.

Checkpoint: captured wire output is correct, and a real MSP client plays both
cues when enabled and remains silent after opt-out and reconnect.

### 5. Documentation and help

- [ ] Update SCREEN-READER help with creation behavior, mode commands, canonical
  `moves` spelling, `tnl`, manual map behavior, and reversible mode semantics.
- [ ] Add sound command/help content covering opt-in, supported transport/client
  evidence, asset installation, test, mute, and missing-file troubleshooting.
- [ ] Update matching entries in the development help database and flat helpfile;
  follow existing additive SQL/help conventions and verify content parity. Use
  the help-sync skill only if cross-environment synchronization is requested.
- [ ] Update `docs/systems/PROTOCOL_SYSTEMS.md` and
  `docs/systems/WEB_ONBOARDING_SYSTEM.md` for the final implemented contracts.
- [ ] Record final validation evidence and remaining usability findings in this
  plan or a linked focused acceptance report. Keep #126/#127 work separate.

## Verification matrix

| Area | Required cases and expected result |
| --- | --- |
| Creation | Yes, no, invalid input, Back, Start over, recommended preferences yes/no; choice remains effective before first room. |
| Recovery | Disconnect before first save re-prompts on restarted flow; after save resumes with preference; save/account-link failure retains existing recovery guarantees. |
| Persistence | Old player files, save/reload, login, reconnect, copyover, and PREFEDIT save; consent and mode remain correct. |
| Rendering | Normal room and wilderness, look and movement, underlying automap on/off, manual map, brief/visibility combinations; useful text retained. |
| Prompt | Idle, combat, wait, status markers, gold-only/time-only, `prompt none`, mode on/off; gameplay suppression and ordinary output both correct. |
| Input UI | Creation, pager, editor, plain terminal, and supported/older structured clients; instructions and valid input remain usable. |
| Audio | Consent on/off crossed with supported/refused/unknown capability; MSP wire bytes, verified alternate route, invalid/oversized trigger, mute/reconnect, missing file. |
| Isolation | Two players with opposite preferences; no preference or sound delivery leaks between descriptors. |

Extend `unittests/CuTest/test_web_onboarding.c` for creation and recovery and
`unittests/CuTest/test_protocol_parser.c` for protocol behavior. Add production-linked
preference/rendering regressions where existing fixtures permit; tests must call
production code, not mirrored implementations. If a new test/source file is
needed, register it in both `Makefile.am` and `CMakeLists.txt`, including applicable
CuTest test-file lists. Avoid adding production-only abstractions for test access.

Run `make test` followed by `make install`, then the focused protocol-parser
harness as appropriate. CuTest has no per-function filter. Run focused isolated
database checks for new save/help behavior. Do not start a full burn-in for this
feature by default. Any development server smoke test uses `autorun.sh` and a
verified development `APP_ENV`; do not modify protected local headers or credentials.

Record a real screen-reader walkthrough with exact client, screen-reader, and
version details: create a character, enter a room, move in town and wilderness,
query status, enter combat, use a pager/editor, reconnect, and change modes.
Record a real MSP playback test for both cues and mute persistence. If the
required client or tester is unavailable, mark that acceptance item pending;
automated tests do not substitute for it. Invite voluntary player feedback via
the existing bug/idea workflow without sending unsolicited external messages.

## Completion and release boundary

Complete #137 only when the scoped code, development help parity, automated
checks, and real-client acceptance above have evidence. Audio transport and
asset distribution must both work; negotiation alone is insufficient. Update
this document with implemented command names and any deviations from the plan.

Existing players retain ordinary output and sound off until they opt in.
Deployment is a separate action. Before any release, record the tested revision,
help changes, asset location, and rollback procedure. Appended preference bits
must remain compatible with existing stored data; do not reuse their identifiers
for different behavior in a rollback or follow-up.
