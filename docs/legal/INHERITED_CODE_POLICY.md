# Inherited Code Licensing and Provenance Policy

Status: repository maintenance policy

This document defines how LuminariMUD maintainers handle inherited code and
licensing records. It is not legal advice and does not reinterpret any license.
The root [LICENSE](../../LICENSE) remains the authoritative project notice.

## Current Repository Decision

LuminariMUD does not claim that the whole repository is available under the
Unlicense, LGPL, or any other single license. The Unlicense dedication in the
root notice applies only to custom LuminariMUD code. Code and content inherited
from tbaMUD, CircleMUD, DikuMUD, independent patch authors, and other sources
retain the terms and notices applicable to those contributions.

For repository maintenance, a later license change by an ancestor project is
evidence to review, not automatic authorization to relicense this tree. The
local tree includes separately authored layers and later LuminariMUD changes
whose rights must be evaluated independently.

Maintainers must preserve existing copyright, authorship, credit, and license
notices. They must not describe the complete repository as LGPL, released under
the Unlicense, public domain, or otherwise uniformly licensed without satisfying
the relicensing gate below.

## Preserved Records

The repository keeps these records as part of its provenance trail:

- [CIRCLEMUD_DIKUMUD_LICENSE.txt](CIRCLEMUD_DIKUMUD_LICENSE.txt) is a verbatim
  reference copy of `doc/license.txt` from upstream tbaMUD tag `v3.68`, commit
  `ef17530ed8717ca99e8c923224c6b24181eb622a`. Its SHA-256 is
  `848534e3f7b18a5b2c7d48170b65c3e8ad8b0de287db57f1016a443e93853231`.
- `lib/text/credits` retains the in-game contributor, patch-author, CircleMUD,
  and DikuMUD credits and license material.
- `lib/text/greetings` retains creator attribution in the login sequence.
- `lib/text/help/help.hlp` retains the `CIRCLEMUD`, `TBAMUD`, and `LICENSES`
  entries used by the in-game help system.
- Source-file headers, Git history, and changelogs retain more granular
  authorship and origin evidence.

The reference license copy is a preserved record. It is not a declaration that
every file has only those terms, nor does it replace component-specific notices.

## Contribution and Import Rules

Any new third-party code, content, or asset must have a provenance record before
merge. The change must identify:

1. the source URL or repository and exact revision;
2. the author or rightsholder information supplied by the source;
3. the exact license or permission statement;
4. whether the imported material was modified; and
5. every notice, attribution, source-offer, or redistribution obligation that
   must accompany it.

The contributor and reviewer must confirm that the proposed use is compatible
with this repository's existing obligations. If the source or permission is
unclear, the material must not be merged. Existing notices must not be shortened
or removed merely because a file has since been heavily modified.

## Whole-Tree Relicensing Gate

The repository must not claim a whole-tree relicense unless all of these steps
are complete:

1. Inventory every affected component and independently authored layer,
   including tbaMUD additions and incorporated patch sets such as DG Scripts,
   Oasis OLC, and ASCII player files.
2. Record the origin, authorship, current terms, and modification history for
   each component.
3. Obtain and archive a license grant or permission covering the target license
   from every required rightsholder, or document another counsel-reviewed basis
   for each component.
4. Resolve material with missing or incompatible permission through a process
   reviewed for that purpose; silence or an unreachable author is not recorded
   as permission.
5. Obtain maintainer approval and appropriate legal review for the completed
   rights matrix.
6. Update the root license, preserved notices, credits, help entries,
   contribution rules, and distribution artifacts together.

Relicensing an identified, independently owned component may be considered on
its own evidence. It does not change the terms of unrelated or inherited code.

## Review Checklist

Any pull request that changes licensing, credits, provenance, or third-party
material must answer these questions:

- What exact files or components are affected?
- Who supplied each affected component, and under what recorded terms?
- Are all existing notices preserved?
- Is written permission or another documented basis included where needed?
- Do the root notice, legal records, in-game credits/help, and shipped artifacts
  remain consistent?

If any answer is unknown, the licensing change remains blocked until the record
is complete.
