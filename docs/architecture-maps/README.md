# LuminariMUD Architecture Atlas

Ten interactive, source-verified maps of the LuminariMUD server. Each map is a
self-contained HTML page compiled by [Archify](https://github.com/tt-a1i/archify)
from a typed JSON source in [`sources/`](sources/). Nothing is fetched from the
network when a page opens.

Start with [`index.html`](index.html). Once this folder is on `master`, GitHub
Pages serves the gallery at
<https://luminarimud.github.io/Luminari-Source/architecture-maps/>.

## Maps

| Map | Type | Question it answers | Scope |
| --- | --- | --- | --- |
| [Server Runtime](server-runtime.html) | Architecture | What runs inside `bin/luminari`, and where does state live? | 12 components, 13 relationships, 30 source references |
| [Event-Driven Core](event-core.html) | Architecture | How do the reactor, scheduler, and domain events share the main thread? | 10 components, 11 relationships, 24 source references |
| [Event Core Delta](event-core-delta.html) | Architecture delta | What did the ADR 0002 refactor remove and add? | 14 added, 8 removed, 4 changed facts |
| [Player Command Round Trip](command-round-trip.html) | Sequence | What happens between a typed command and the next prompt? | 8 participants, 14 messages |
| [Server Startup and World Boot](boot-sequence.html) | Sequence | In what order does the server boot, and how does it exit or reboot? | 8 participants, 14 messages |
| [Copyover Hot Reboot](copyover.html) | Sequence | How does the server replace its own binary without dropping players? | 6 participants, 13 messages |
| [Player Connection Lifecycle](connection-lifecycle.html) | Lifecycle | Which connection states does a player pass through? | 8 states, 9 transitions |
| [Scheduled Game Event Lifecycle](scheduled-event.html) | Lifecycle | What happens to a timed event from scheduling to cleanup? | 8 states, 9 transitions |
| [Save and Reload Data Flow](save-and-reload.html) | Data flow | Where are world and player state written, and how are they read back? | 12 nodes, 11 flows |
| [Change to Merge Workflow](change-to-merge.html) | Workflow | Which local gates and hooks does a change pass before CI? | 8 steps, 9 edges |

Every map except the delta also has two to five guided views: named chapters
that highlight one path through the diagram.

## Reading a map

- Press `?` for the built-in guide and `/` to find a node by name or ID.
- Pick a numbered guided view above the diagram, or press `P` to play them in order.
- Focus a node and choose Upstream or Downstream to trace its authored reach.
- Press `R` to probe the route between two nodes and `L` to compare semantic roles.
- Press `F` to present, `S` to cycle visual styles, and `T` to switch light and dark.
- Press `E` to export PNG, JPEG, WebP, dual-theme SVG, WebM, or a 1200x630 share card.
- On the architecture maps and the delta, a node marked `SRC n` opens the cited
  files and line ranges on GitHub, pinned to the commit the map was checked against.
- The delta page has Before, Delta, and After tabs plus a Review mode that steps
  through each authored change.

## How the maps were built and checked

Every node and relationship comes from reading the code, not from file names.
The maps follow traced call paths such as `game_loop()` in `src/core/comm.c`,
`command_interpreter()` and `nanny()` in `src/core/interpreter.c`, `boot_db()`
in `src/core/db.c`, the timing wheel in `src/events/game_scheduler.c`, and
`perform_do_copyover()` in `src/act/act.wizard.c`.

- The two architecture maps cite 54 file and line references pinned to
  `0bc67f2c05351e9761d5b4f5c25d66abbd7ea362` (`master`, 2026-09-15).
- The delta base cites 11 references pinned to
  `fbe9366fbde8180b49a211cbcfdf2b2b531a15bd` (2026-08-29), the commit just
  before the native scheduler was added. Its head is the event-core map.
- Archify re-verified every cited path, blob, and line range against local Git
  history for that revision before it rendered each architecture page.

Each page then passed three separate gates:

1. `archify validate --quality showcase`: schema, layout, routing, label
   clearance, and desktop readability. Every map reports 9 of 9 artifact checks
   with 0 errors and 0 warnings. The delta reports 28 of 28 compare checks, with
   both snapshots passing showcase composition.
2. `archify deliver`: renders a frozen copy of the source and replaces the page
   only after every check passes. The receipts below are the hashes it reported.
3. `archify visual-check`: headless Chromium measures page containment and
   projected text size at 1440x900, 1600x1000, 1920x1080, and 2048x1320, and
   captures light and dark screenshots.

Those screenshots were also inspected in both themes for crossing lines, masked
labels, clipped text, and balance.

| Map | Validation | Browser evidence |
| --- | --- | --- |
| Server Runtime | 9/9 showcase | pass |
| Event-Driven Core | 9/9 showcase | pass |
| Event Core Delta | 28/28 compare | fail: the delta page is taller than the viewport |
| Player Command Round Trip | 9/9 showcase | pass |
| Server Startup and World Boot | 9/9 showcase | pass |
| Copyover Hot Reboot | 9/9 showcase | pass |
| Player Connection Lifecycle | 9/9 showcase | pass |
| Scheduled Game Event Lifecycle | 9/9 showcase | pass |
| Save and Reload Data Flow | 9/9 showcase | pass |
| Change to Merge Workflow | 9/9 showcase | pass |

The delta's only browser finding is vertical overflow. Archify's own reference
delta, `examples/checkout-platform-delta.html`, produces the same measurement,
so the tall page comes from the compare viewer rather than from these snapshots.

### Receipts

Run either block through `sha256sum -c` from this folder to confirm that the
committed files are the exact bytes that passed delivery.

Sources:

```text
65ac067e9605af84682b04d208548d8aeb2e2e684cc61d4510eba31d41a79973  sources/server-runtime.architecture.json
1fface64338778879f33223dd8497d90458352d5161f394a7f80aac1ea824723  sources/event-core.architecture.json
2ed75682b98eb87aaa9f137fe853904a0533ee58fc5105426f6206a57dcb4baa  sources/event-core-delta.base.architecture.json
3d66109d8b0e2e225260fc84d641403becc92281d8b66b2745e309ffc4bcf3ca  sources/event-core-delta.head.architecture.json
12f47f4fa090ff044acbb024f090c8afd69dd2b752d939f2249ab9a4891c8b37  sources/command-round-trip.sequence.json
7b1849b4b8b57ce86c29c57dd190ada3c456f6e10561c28b0d51cf9682559379  sources/boot-sequence.sequence.json
278008c617def20c7d327a0734a17528ead8c7e3f431691a48b20babe190221f  sources/copyover.sequence.json
18d6505b483efcdfa552b18a997441e63eab980adf22c6abf8f23d6e5e0e7c2b  sources/connection-lifecycle.lifecycle.json
2683b7148ed253f42319e7e9b9b56c1e1424a5d61d7efb42ffaad7390865f181  sources/scheduled-event.lifecycle.json
c669899e67c23bc2724722a5e14579e6bcb1d49255c64951018e6db3d87e2bdd  sources/save-and-reload.dataflow.json
8094fa5120a9902c3c5489edfcad9ab30c65d0206e4ca59271dfd305314fc843  sources/change-to-merge.workflow.json
```

Pages:

```text
2dbb8351c1222238d363b6c16e3843c35f1dfaaac1b1e3150f5c9ef9e1c99c0e  server-runtime.html
1ea3b209e0158a5e88c582f863cfd01de5d7ce768e4187de34dddf8d417f0b5f  event-core.html
b67e856cf026300e0b452981e6cf27da07f5b0b0a0358771cce3854a21e80692  event-core-delta.html
3a344cfd3349fda580a5705c862875e2deccc57d953d872f991b7ae3e409b90d  command-round-trip.html
b8b4f212a41b925307bb48f5e37bd0ead85cdfe15ba6742082a75b023f8e3305  boot-sequence.html
b3fb54269069480b9fc161c8de12aaeb1f3b6a5d034e4ed18361cd92defa5672  copyover.html
8348df7553c916e5587c62ceb0dbbde144e0fd56f2312c16e00bd6e82bf67fe1  connection-lifecycle.html
473016e139c481976cbf3cb251298a3b634aafbbc041e5630d50ec5d55955680  scheduled-event.html
e1263b3a74196d3f27101607097d0cb99a2b28581b80ae5cdee0182f69df022a  save-and-reload.html
55c49a0d24484082c37002160fce930cff93d1a6cfd65cfb009caa7534d205c5  change-to-merge.html
```

[`event-core-delta.receipt.json`](event-core-delta.receipt.json) is the compare
receipt. It lists every added, removed, changed, and evidence-changed fact
together with the hashes of both snapshots.

## Regenerating a map

The maps were built with Archify `2.17.0-dev.1`. Archify needs only Node.js 18
or newer.

```bash
git clone https://github.com/tt-a1i/archify /tmp/archify
cd /tmp/archify/archify
REPO=/path/to/Luminari-Source
MAPS="$REPO/docs/architecture-maps"

# Architecture pages need --repo-root so their source references can be verified.
node bin/archify.mjs validate architecture "$MAPS/sources/server-runtime.architecture.json" \
  --repo-root "$REPO" --quality showcase --json
node bin/archify.mjs deliver architecture "$MAPS/sources/server-runtime.architecture.json" \
  "$MAPS/server-runtime.html" --repo-root "$REPO" --quality showcase --json

# Sequence, lifecycle, data-flow, and workflow pages do not take --repo-root.
node bin/archify.mjs deliver sequence "$MAPS/sources/command-round-trip.sequence.json" \
  "$MAPS/command-round-trip.html" --quality showcase --json

# The delta compares two architecture snapshots and writes its receipt.
node bin/archify.mjs compare architecture \
  "$MAPS/sources/event-core-delta.base.architecture.json" \
  "$MAPS/sources/event-core-delta.head.architecture.json" \
  "$MAPS/event-core-delta.html" --receipt "$MAPS/event-core-delta.receipt.json" \
  --repo-root "$REPO" --quality showcase --json

# Browser evidence. Run it on a copy outside the repository: it writes
# screenshots and a JSON receipt beside the page.
ARCHIFY_CHROME=/path/to/chrome node bin/archify.mjs visual-check /tmp/server-runtime.html --json
```

Notes for editing a source:

- Keep authored text ASCII. Archify's own viewer glyphs inside the generated
  pages are UTF-8, which the hygiene check allows for HTML.
- Source references are verified at the pinned revision, not the working tree,
  so existing links keep working after the code moves. To refresh a map, re-read
  the code, update the paths and lines, and bump `meta.repository.revision`.
- Showcase validation fails when node text would project below 6px in a 960px
  reader. Keep architecture view boxes under about 1,380px wide and sequence
  and lifecycle view boxes near 1,040px wide. Keep the width at least 1.55
  times the height so the page fits a 1440x900 screen.
- Snap-packaged Chromium cannot read files under `/tmp`. Point `ARCHIFY_CHROME`
  at a regular Chrome build or at Playwright's Chromium instead.
- Every page embeds Archify's complete viewer, about 800 KB each and 2.2 MB for
  the delta. `.pre-commit-config.yaml` exempts `docs/architecture-maps/*.html`
  from the 500 KB added-file limit, and from the trailing-whitespace and
  end-of-file fixers so the committed pages keep the exact bytes in the receipts.

## Limits

- These are authored maps of the main paths, checked against the code. They are
  not generated from runtime traces, and they are not exhaustive call graphs.
- Upstream and Downstream reach, route probes, and the delta report authored
  relationships only. They do not measure runtime impact or risk.
- The sequence, lifecycle, data-flow, and workflow pages cite their evidence in
  their cards; only architecture pages carry clickable `SRC` references.
