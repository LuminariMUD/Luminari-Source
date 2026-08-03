# Production In-Game Typos Backlog

This is a read-only, source-filtered snapshot of the production
`typo submit <header>` queue, retrieved and traced on 2026-08-03. The initial
production file contained 87 submissions: 67 unresolved, 18 in progress, and
two resolved. Only reports whose player-facing text or behavior maps to source
code are retained here.

The 68 excluded reports cover `lib/` or world content, including room, object,
mob, quest, DG Script, help, background, and combat-message data, as well as one
website report. One repeated flee report is consolidated, so the 19 retained
source records become 18 distinct entries: 16 unresolved and two in progress
according to their production flags.

Production changed concurrently during this read-only audit, shrinking from 87
records to 35. This document preserves the code-backed records from the initial
snapshot so that concurrent queue cleanup cannot discard them. Record numbers
below refer to that initial snapshot. Reporter metadata uses `L` for character
level and `R` for room VNUM.

The nine source defects retained from that snapshot were corrected in the
development checkout on 2026-08-03. The score report was also rendered on the
development server and is no longer reproducible. Production status labels
below remain historical until the corrected build is deployed and the
production queue is reviewed; this local work did not change production.

## Fixed in the Current Development Checkout

- **Craft-resize progress can inherit an item's color** - The resize progress
  line now resets color before "You have approximately ... seconds left to go,"
  preventing the preceding item's light-grey color from bleeding into it. Source:
  `src/craft/craft.c:2853-2856`. Reporter: Janalon (L28, R101772, 2021-01-13;
  #004). Snapshot production status: unresolved.
- **Arrow Swarm cooldown names Death Arrow** - When Arrow Swarm is unavailable,
  its cooldown message now names "another swarm of arrows." Source:
  `src/combat/act.offensive.c:5631-5638`. Reporter: Casseia (L26, R103009,
  2021-03-10; #022). Snapshot production status: in progress.
- **`uncommune` emits two spaces after an unmodified spell name** - The format
  strings now prefix only present metamagic labels with spaces, leaving exactly
  one space before `from` when there are no labels. The preparation-queue and
  collection variants were both corrected. Source:
  `src/magic/spell_prep.c:5057-5087`. Reporter: Raewo (L2, R145202, 2021-04-14;
  #025). Snapshot production status: unresolved.
- **Smite Evil feat information says good-aligned targets** - The generated feat
  description now names evil-aligned targets. This text is generated from
  source, not a help file. Source: `src/character/feats.c:3739-3743`. Reporter:
  Horkesh (L8, R103000, 2022-02-14; #029). Snapshot production status:
  unresolved.
- **Guard messages misspell `successfully`** - Both successful guard messages
  now use `successfully`. Source: `src/combat/fight.c:247-251`. The same
  submission mentions a piercing-death message, but that instance is in
  `lib/misc/messages` and remains intentionally excluded. Reporter: Metvagen
  (L16, R1925, 2022-08-25; #039). Snapshot production status: in progress.
- **Cure Critical is displayed as Cure Critic** - The public spell registrations
  now use `cure critical` and `mass cure critical`. The clan-cleric service
  label, local fallback help, and generated web references were made
  consistent. The old spell abbreviations still resolve through normal prefix
  matching. Source: `src/magic/spell_parser.c:5065`,
  `src/magic/spell_parser.c:5179`, and `src/spec_procs.c:4706`. Reporter:
  Murdoch (L30, R23820, 2022-11-14; #049). Snapshot production status:
  unresolved.
- **The feat-selection hint has a broken sentence transition** - The hint says,
  "Have fun reading up on feats using the FEATS command with no argument. It
  will show ..." Source: `src/act.other.c:9042-9047`. Reporter: Amorea (L1,
  R14124, 2023-03-16; #067). Snapshot production status: unresolved.
- **The donation/Junk hint concatenates `itemthat`** - Adjacent string literals
  now include the missing separating space. Source:
  `src/act.other.c:9135-9138`. Reporter: Mddljeu (L1, R14108, 2025-07-05;
  #081). Snapshot production status: unresolved.
- **The `blood` command says `food on the blood`** - A character without Blood
  Drain is now told, "You don't have the ability to feed on the blood of
  others." Source: `src/combat/act.offensive.c:14406-14409`. Reporter: Falwel
  (L3, R145270, 2025-12-01; #083). Snapshot production status: unresolved.

## Verified in Development Rendering

- **Score class row may be misaligned** - The report says the `Class : <Levels>`
  row does not align with neighboring fields. A development login rendered the
  current `score` output and showed `Class` beginning in the same column as
  `Title`; the report is no longer reproducible and no source change was
  warranted. Source: `src/act.informative.c:4416-4472`. Reporter: Adelais (L1,
  R14125, 2021-02-02; #019). Snapshot production status: unresolved.

## Already Correct Before This Fix Pass

These reports were still unresolved in the initial production snapshot, but
the current checkout already contains the reported correction. They should be
verified against a current build and then closed rather than reimplemented.

- **Wizard circle gained at class level 9** - The report says a ninth-level
  Wizard gained fourth-circle spells. Current assignments grant fourth-circle
  spells at class level 7 and fifth-circle spells at class level 9. Source:
  `src/character/class.c:4143-4149`. Reporter: Bladepattern (L23, R23823,
  2021-01-24; #013).
- **Surprise Accuracy command spelling** - The report gives
  `supriseaccuracy`; the command and sort key are now both
  `surpriseaccuracy`. Source: `src/interpreter.c:4328-4331`. Reporter:
  Magnimoth (L27, R23805, 2021-02-25; #021).
- **Flee message misspelled `attempts`** - Two reports showed mobs that
  `attemps to flee`. Both generic flee paths now say `attempts`. Source:
  `src/combat/fight.c:277` and `src/combat/fight.c:339`. Reporters: Metvagen
  (L11, R103176, 2022-08-23; #037) and Talendor (L10, R26915, 2023-01-23;
  #054).
- **Blackguard title misspelled Adept** - The reported `Adaept Blackguard`
  title is now `Adept Blackguard`. Source: `src/character/class.c:6082-6086`.
  Reporter: Metvagen (L12, R103383, 2022-08-24; #038).
- **Body of Iron described `think iron`** - All current Body of Iron appearance
  strings say `thick iron`. Source: `src/act.informative.c:1013-1016`,
  `src/act.informative.c:1079-1082`, and `src/act.informative.c:1249-1252`.
  Reporter: Mnemosyne (L27, R23807, 2022-08-30; #042).
- **Dragon Bite said flesh was `redned`** - The three current combat messages
  use `rended`. Source: `src/combat/act.offensive.c:10215-10217`. Reporter:
  Dasvel (L30, R196004, 2022-09-08; #043).
- **Frightened Mercy said it removes `feat` status** - The current mercy
  description says it removes fear status. Source: `src/constants.c:332-343`.
  Reporter: Gicker (L34, R1204, 2022-09-25; #044).
- **Deadly Aim feat name was misspelled** - The current registered feat name is
  `deadly aim`, and its usage text uses `deadlyaim`. Source:
  `src/character/feats.c:1263-1267`. Reporter: Thiurs (L3, R145374,
  2023-01-27; #057).
