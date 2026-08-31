# Active World

The active-world manager schedules concrete autonomous NPC work. It does not
schedule an NPC merely because that NPC is loaded, and it never walks the global
character list during normal dispatch.

## Core Contract

LuminariMUD has one global timing wheel. Active-world entries are typed work in
that scheduler, not a separate scheduler per mobile class.

- Entity existence is never sufficient for admission.
- Every scheduled NPC reports at least one explicit work reason.
- Losing the final reason cancels the NPC deadline immediately.
- A callback dispatches only registered due work, not every possible AI branch.
- Extraction and generation change invalidate every owned deadline.
- Player proximity is not an admission rule.

An owner may coalesce several due reasons behind one earliest-deadline handle.
That is an agenda of known work, not a periodic check for unknown work. An empty
agenda has no event.

## Work Reasons

Recurring work is reserved for behavior that is actually periodic:

- wander decisions for eligible non-sentinel NPCs;
- configured patrol steps;
- active hunt steps with a valid target;
- special procedures that declare mobile-activity support;
- configured mobile echoes;
- scavenging while eligible room objects remain;
- recovery while a consumed mobile resource is below maximum.

Combat rounds belong to the encounter manager. Aggression and memory recognition
are reactions to character entry. Guards, helpers, and listeners react to combat
state changes. Scavengers wake when room objects change. Sentinel posture
restoration exists only while posture differs. Gated creatures use their exact
expiry deadline. These are one-shot or state-owned events, not permanent mobile
heartbeats.

Off-screen simulation remains authoritative. A wanderer continues wandering, a
patrol continues its route, a hunter continues hunting, and NPC wars continue
through encounter events whether or not a player is present.

## Local Reactions

Typed domain facts update only affected owners. A movement fact may inspect the
destination room and bounded adjacent rooms; a combat-state fact may inspect the
participants, their room, and nearby listeners. Normal gameplay dispatch never
traverses `character_list`, `object_list`, or every room to discover work.

NPC movement wakes the moving NPC's own relevant arrival behavior. It does not
wake every other NPC in the room: ordinary NPCs are not aggression, memory,
pre-buff, or archery targets. Player and pet arrivals wake eligible observers
in the destination and bounded adjacent rooms. Combat facts independently wake
guards, helpers, assisting mobs, and listeners, so NPC wars remain active
without a player present.

Scheduled-mobile and domain-entity registries are external, generation-keyed
hash tables. Callbacks resolve immutable handles in expected constant time
before touching character memory. Extraction removes the registry entry and
cancels all events indexed to the exact owner generation.

## Production Correction

The original Phase 7 implementation assigned one recurring event to nearly
every AI-enabled NPC. A copied production world contained about 61,000 NPCs and
about 60,500 mobile owners. Even after queue ownership was coalesced, the server
processed about 20 million callbacks in eleven minutes and used about 97% of one
CPU core. That implementation was a distributed version of the old mob loop and
is rejected by specification version 1.29.

Production acceptance now requires event counts and callback cost to track
explicit pending behavior, with no scheduler backlog, no overdue work, and lower
cost than the compatibility loop on the same copied world.

The corrected copied-world run retained about 39,000 real agendas, including
about 37,000 off-screen wanderers, while settling at 2.6% of one core. The
timing wheel reported zero ready events, zero overdue pulses, and zero late
callbacks. About 193,000 agenda callbacks averaged roughly 4 microseconds.

## Diagnostics

`eventdebug` reports owner counts by work reason, scheduled owners, one-shot
reactions, callbacks by reason, admission rejections, and registry mismatches.
Output defaults to 80 columns and never exceeds 120 columns.

## Rollback

`LUMINARI_ACTIVE_WORLD=legacy` restores `mobile_activity_pulse()` for a complete
boot. Scheduled and legacy paths are exclusive. The rollback remains until the
Phase 11 stable-release gate permits physical removal.
