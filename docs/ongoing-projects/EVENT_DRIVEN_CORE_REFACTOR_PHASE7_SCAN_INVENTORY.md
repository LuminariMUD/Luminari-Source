# Phase 7 Heartbeat and Global-Scan Inventory

**Audit date:** 2026-08-30
**Source boundary:** direct `heartbeat()` consumers and their primary owner
populations on `event-driven-core-refactor`

This is the required migration inventory. "Bounded registry" means normal
runtime work already visits eligible owners rather than the full source list;
it does not mean that its cadence has already moved off the heartbeat.

| Cadence | Consumer or group | Population/reason | Classification and disposition |
|---|---|---|---|
| Every pulse | compatibility event dispatch | Due timed events | Scheduler deadlines already drive the reactor; retain until Phase 11 removes the legacy queue. |
| Every pulse | pending character extraction | Pending extraction list | Active-work cleanup; retain as a bounded safety drain. |
| Every pulse | minute persistence step | Stable-ID task snapshot | Already budgeted to one operation per pulse; later move its next deadline into the scheduler. |
| 0.5 s | vessel autopilot, hunters, combat, events, wages, upkeep, trade, weather, encounters, MSDP | Loaded vessels and subsystem registries | Mixed active/global work; vessel-owned deadline conversion is a later Phase 7 slice. |
| 0.7 s | walk-to actions | Characters with walk-to state | Create an eligible-character registry and owner deadline. |
| 1 s | help reload poll | One filesystem control point | Genuine service/watchdog work; scheduled global service is acceptable. |
| 1 s | MSDP update | Connected descriptors | Active connection work; convert to descriptor deadlines or one connection registry event. |
| 1 s | travel, self-buff, crafting, supply slots | Characters/connected players with relevant state | Split into eligible owner registries and deadlines. |
| 1 s | I3 ingress and presence | Cross-thread queue/connected players | Queue wake already signals the reactor; presence is genuine coordinated service work. |
| 2.5 s | converted RoL ships | Converted ship list | Vessel-owner deadline candidate. |
| 3 s | zone reset | Zone reset queue | Due-work queue, not a world scan; migrate queue deadline without changing reset semantics. |
| 5 s | PSP regeneration | Characters eligible to regenerate | Character-owner deadline candidate. |
| 5 s | Luminari pulse | Mixed character/system work | Decompose by owner before migration; do not preserve as one opaque scheduled callback. |
| 6 s | NPC thinking | Previously every character | **Converted:** one distributed owner deadline per autonomous NPC; player absence does not suspend patrols, wandering, scripts, or NPC wars. Only out-of-world, extracting, and `MOB_NO_AI` owners are dormant. |
| 6 s | object auto-procs | `ITEM_AUTOPROC` registry | Already eligibility-bounded; object-owner scheduling is a next-slice candidate. |
| 6 s | character affects | Affected-character registry plus room-affect list and connected-PC MSDP refresh | Already eligibility-bounded; split character and room owners before scheduling. |
| 6 s | D20 round procedure | Encounter/combat state | Phase 8 owns encounter-level scheduling. |
| 6 s | damage/effects and player misc | Character/descriptor state | Split named effects from generic player maintenance, then schedule owners. |
| 11 s | bardic performance | Performing characters/groups | Owner-event candidate; preserve song audience semantics. |
| 13 s | DG random triggers | Mobile/object/room random-trigger registries | Already eligibility-bounded; schedule script owners with OLC/extraction cancellation. |
| 30 s | idle passwords | Non-playing descriptors | Descriptor registry/deadline candidate. |
| 30 s | auction/other thirty-second work | Auction and subsystem globals | Audit each member; auction deadline may be a genuine service event. |
| 60 s | shutdown/happy-hour checks, activated items | Service state and eligible activated objects | Keep service checks global; move item recharge to owners/registry. |
| 60 s | minute persistence admission | Connected players, dirty pets/artifacts/houses | Admission is global maintenance; execution is already budgeted active work. |
| 75 s mud hour | world clock/weather | World state | Genuine scheduled global coordination. |
| 75 s mud hour | DG time triggers | All scripted character/object/room owners | Add time-trigger eligible registries, then schedule owners or one bounded time boundary dispatch. |
| 75 s mud hour | point update | Characters, objects, conditions | High-value mixed scan; decompose regeneration, consumables, and lifecycle owners. |
| 75 s mud hour | timed quests, diplomacy, clans | Active players/quests/clans | Use stable registries and explicit deadlines. |
| 75 s mud hour | vessel schedules | Scheduled vessel departures | Vessel deadline queue candidate. |
| 75 s mud hour | trail cleanup | Existing trail records | Bounded record registry; genuine maintenance deadline. |
| 24 mud hours | clan investments | Clan investment rows | Genuine scheduled global economic event. |
| 5 min | usage record and hints | Descriptors/players plus metrics | Metrics are explicitly global; hints belong to connected player owners. |
| 30 min | world-time save | One world record | Genuine persistence event. |
| 2 h | hunt creation | Hunt/world policy | Genuine coordinated world event if implementation does not scan dormant owners. |
| 10 s | moving rooms | Configured moving-room list | Active configured world objects; move each route to a room/service owner deadline. |

Calls driven directly by socket readiness, command queues, signals, or explicit
cross-thread wakeups are not heartbeat scans and are outside this inventory.
Staff validation routines may deliberately traverse full lists; they are
diagnostic paths, not normal gameplay orchestration.

## Priority after NPC thinking

1. `ITEM_AUTOPROC` object owners and DG random-trigger owners, because their
   lifecycle registries already exist.
2. Affected character and room owners, after separating connected-PC MSDP
   refresh from duration expiry.
3. Walk-to, regeneration, bardic performance, hints, and similar explicit
   character state.
4. Vessel tick decomposition and the mixed `point_update()`/Luminari pulses.
