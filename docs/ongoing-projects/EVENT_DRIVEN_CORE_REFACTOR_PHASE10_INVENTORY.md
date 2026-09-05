# Phase 10 Activity and Busy-State Inventory

**Status:** Accepted implementation inventory on 2026-08-31

This inventory separates intentional, progress-owning activities from action
recovery and imposed control effects. The activity manager is not a generic
replacement for every delay. A migration is appropriate only when gameplay has
an actor, a typed target, owned progress, and meaningful interruption policy.

## Migrated first slice

| Command | Prior model | Phase 10 model | Rollback |
|---|---|---|---|
| `camp` | Immediate Survival result and camp benefits | Character-owned three-step primary activity targeting a room | `LUMINARI_CAMP_ACTIVITY=legacy` |

The managed command preserves the existing Survival roll, action cost, group
shelter, recovery bonus, and load-room result. It adds explicit progress,
movement cancellation, damage delay, combat pause/resume, target recheck, and
player status/control.

### Deferred gameplay decision: Survival and Nature

Phase 10 deliberately does not rename or split the underlying ability. The
camp implementation continues to call `ABILITY_SURVIVAL`, exactly as the
immediate command did. In the current ability table that identifier occupies
slot 29, whose player-facing name is `Nature`; `ABILITY_NATURE` aliases the same
slot. Repository history shows that Survival was intentionally renamed to
Nature in 2017, while later help and mechanics continued to use both terms.
Resolving whether the game should expose Survival, Nature, or separate skills
has persistence, class-skill, lore, tracking, foraging, movement, perk, and help
implications and therefore requires a separate maintainer decision.

## Existing progress-owning work

| Source | Current ownership signal | Disposition |
|---|---|---|
| Legacy crafting | `eCRAFTING`, `GET_CRAFTING_OBJ`, command busy allowlist | High-priority later activity migration; define recipe/object target and rollback first |
| Brewing | `eBREWING`, serialized brew state, command busy allowlist | High-priority later activity migration; preserve vessel and ingredient ownership |
| Device creation | `eDEVICE_CREATION`, `eDEVICE_PROGRESS`, device state | High-priority later activity migration with object target and resumable progress review |
| Device repair | `eDEVICE_REPAIR`, device state | High-priority later activity migration with extraction and ownership tests |
| Spell casting/preparation | Casting/preparation events and separate command admission | Candidate only after a dedicated magic policy review; not silently folded into primary activities |

The old crafting/device/brewing allowlist remains only for those unmigrated
systems. Managed activities use central command metadata and admission instead.

## `WAIT_STATE` classification

The source contains 26 direct `WAIT_STATE` applications, plus the core wait
counter and DG-script variable access. They divide as follows:

| Class | Representative sources | Decision |
|---|---|---|
| Atomic command recovery | search, door/lock attempts, hostile boarding, ship weapons/repair, wagon looting | Retain as throttling unless a separate gameplay review turns the command into progressive work |
| Imposed control | bomb/artifact/spell knockdown, disarm lag, zone special effects | Retain; these are status consequences, not voluntary activities |
| Exhaustion | resurrection caster and target recovery | Retain; semantic action/status policy, not progress ownership |
| Compatibility plumbing | input wait decrement and DG `waitstate` variable | Retain through Phase 10; Phase 11 owns compatibility-heartbeat removal |

Lockpicking, searching, and ship repair are reasonable reviewed candidates, but
their current behavior is atomic with a recovery delay. Converting them without
design approval would change gameplay rather than merely migrate timing.

## Explicit non-activities

- Standard, move, swift, immediate, and reaction budgets are semantic combat
  resources. Their cooldown events do not represent activities.
- Stun, daze, knockdown, silence, stagger, and similar effects are actor state.
- Persistent cooldowns and daily-use counters remain typed effects or durable
  event records.
- Autonomous NPC, room, object, vessel, and zone deadlines are world simulation
  owners, not character intent.

## Replacement rule

Each later migration must add a reviewed capability/trait/interruption row,
typed target generation, explicit progress ownership, completion-once and stale
target tests, player-readable `activity` output, and an independent rollback
selector. Once all old busy systems have migrated, their command allowlists and
timer-specific flags can be deleted rather than mirrored inside the manager.
