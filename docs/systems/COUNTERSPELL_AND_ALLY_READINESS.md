# Counterspell and designated-ally readiness

Design decision for assigned issue #106, 2026-09-06. Implementation baseline:
`9b086f39f` on `fix/open-issue-repairs`. These are implementation requirements,
not a claim that the new commands or tests already exist.

## Existing contracts and simplification

`src/ready_action.c` already owns the standard-action reservation, subscriptions,
six-second out-of-combat expiry, next-semantic-turn expiry, and native queued
execution. Extend that owner with explicit reaction and trigger kinds. Do not add
a reaction manager, a second action allowance, a descriptor scan, or a timer for
each possible adversary. One character still has one readied action.

`src/activity_manager.c` publishes CastingStarted only after timed casting has
been admitted and survived ActivityTransitioned observers. It removes a completed
activity before calling its completion handler. Consequently a reaction cannot
cancel a spell whose activity has already completed, even if spell resolution is
still on the C stack. Preserve that boundary.

`src/combat/fight.c:resolve_hit` has early returns for invalid attacks, dispatches
queued special actions, invokes DG fight triggers, and eventually resolves the
ordinary strike. Its HIT_MISS result alone cannot distinguish an attempted miss
from a rejected command. Do not infer an attack fact from this return value or
from CharacterDamaged.

## Counterspell rules

1. Use `ready counterspell <caster> on casting`. Resolve a visible local caster
   at admission and retain its generation handle. Require an awake, otherwise
   eligible spellcaster, attention/hands/speech capability, and a standard action.
   Install the subscriptions and expiry before spending that action. Failure to
   admit costs neither an action nor a spell resource. An admitted reservation
   is not refunded on cancellation or expiry, matching readied attacks.
2. Trigger only on a new committed timed cast by that exact caster. Readiness
   established after CastingStarted cannot attach to the already-running cast.
   Instant casts, item activation, passive effects and psionic manifestation have
   no counterspell window under this rule. Do not add a hidden delay to them.
3. Observe the spell through structured casting data. Require local visibility
   and a perceptible component: an audible verbal component or a visible somatic
   component, accounting for silence, still/silent metamagic and soundproof rooms.
   A spell with neither observable component cannot be identified this way.
4. Make one identification roll for this reservation and cast ID. Initially use
   the existing game threshold: `compute_ability(ABILITY_SPELLCRAFT) + d20 > 20`,
   as in `say_spell` in spell_parser.c. This is a local game rule, not a claim
   about tabletop rules. Store the result in the reaction; repeated events cannot
   reroll it. Descriptive prose and its independent display roll are not evidence
   that a counterspell identified the cast. Failed identification ends this
   reservation without spending a spell resource.
5. The initial counter is the identical spell, using its unmodified spell ID.
   Pay an ordinary available preparation or spontaneous resource through the
   spell-preparation system; countering does not cast that spell's normal effect.
   Do not introduce opposite-spell tables or automatically enable the currently
   unimplemented Improved Counterspell feat. Dispel-as-counter and school-based
   counters need separate rules and tests before being advertised.
6. Resource selection needs a non-mutating eligibility query and a single
   authoritative commit. `spell_prep_gen_extract` is not a query: it can consume
   moon bonus uses before examining ordinary preparations. Share its established
   resource policy rather than copying a second multiclass/bonus-slot algorithm.
   A probe must not consume slots, update preparation queues, or emit messages.
   At-will effects do not supply an unlimited counterspell resource.
7. Queue execution through the existing native ready event, one pulse later.
   Revalidate owner, readiness incarnation, caster generation, room, perception,
   capability, resource and exact active cast ID. Consume no resource if the
   target cast has ended, been cancelled, or been replaced. Once these checks
   pass, consume exactly one resource and cancel that exact casting activity.
   This uses the reserved standard action; it starts no second primary activity
   and spends no additional action or reaction allowance.
8. Resource commit and exact-cast cancellation must have no intervening callback
   that can replace the target cast. If resource handling has callbacks, first
   refactor a bounded commit operation or use an explicit exact-cast claim; a
   check followed by a generic cancel is insufficient. Add a COUNTERED end reason
   and preserve the existing interrupted-cast resource policy for the victim.
   Observer callbacks run only after the target activity has detached.
9. Competing reactions use native dispatch order. The first successful counter
   ends that cast; later reactions discover that the ID is gone and spend no
   spell resource. An earlier readied strike can instead interrupt it through
   normal concentration rules. Never undo a completed cast or replay a reaction
   to compensate for dispatch lateness. Tests must cover equal deadlines in both
   insertion orders and a delayed dispatch containing both due events.

The existing `mode counterspell` AFF_COUNTERSPELL toggle currently has no spell
consumer. Keep persisted bit numbering stable, but retire its misleading active
mode behavior and direct users to the explicit ready command. It must not become
an automatic free counter on every cast. Update flat and database help together.

## Designated-ally defense rules

Use `ready attack on ally <ally> attacked`: bind one visible local ally at
admission, then bind the attacker when an eligible attack fact arrives. Require
the ally to be a current group member or an owned follower and recheck that
relationship at trigger time. Self-protection is not this trigger. Preserve the
same standard-action cost, one normal melee strike, expiry and cancellation
rules as other readied attacks. Do not dispatch the attack queue or grant a full
attack routine.

The trigger means retaliation against a witnessed committed attempt. It does not
redirect damage, grant cover, or retroactively prevent the triggering strike.
That strike resolves first; the queued reaction executes afterward if its owner
and attacker remain valid and normal melee eligibility still holds. A miss,
concealment failure, immunity or zero-damage result still counts as an attempt.
A rejected peaceful-room/PvP/range check or lack of ammunition does not.

Add one typed completed-attempt fact for this concrete consumer. Its payload
contains a monotonic attempt ID, attacker and intended defender generation
handles, origin room, attack kind, and outcome (hit, miss, prevented, or aborted
after commitment). Actor/target IDs are historical identity; a stale entity must
not be dereferenced. Delivery is scoped to the intended defender, not broadcast
to all NPCs or descriptors. An attack remains observable if the defender dies;
death or movement of the defended ally cancels future untriggered watches, but
must not erase a retaliation already bound to a live attacker.

Place the authoritative commitment marker after legality and required ammunition
checks and after pre-operation DG decisions, immediately before the attempt's
combat consequences. Capture handles before invoking callbacks, re-resolve after
callbacks, and publish exactly once as the attempt unwinds. Nested ripostes or
extra strikes have distinct attempt IDs. A delegated special-action command is
not itself another ordinary attack: its actual strike path publishes its facts.
Audit ranged and melee branches rather than putting publication around `hit()`
and assuming all returns mean the same thing.

The defender subscription tests the designated identity and the protector's
actual local visibility of the attacker. It atomically claims the first eligible
attempt and queues one reaction; further facts cannot retarget it. Cancel the
reservation before calling `combat_readied_attack`, preventing self-recursion.
Normal legality, single-file, charm, death and weapon restrictions still apply.
Protectors do not acquire awareness from prose or from unseen remote attacks.

## Ownership and verification requirements

Execution payloads must identify the particular readiness incarnation as well as
the character generation. The existing expiry callback checks its event ID;
execution must likewise reject callbacks belonging to a replaced reservation.
Keep actor, watched ally and bound attacker lifetimes distinct. Native scheduling
or subscription failure must leave no owner, subscription or timer leak.

Extend production-linked tests in `unittests/CuTest/test_activity_manager.c` and
the relevant domain-event tests. Required evidence before issue completion:

- Real prepared and spontaneous casting: identify, debit once, counter exact
  cast, suppress its effect and report the cancellation; failed identification,
  unavailable resources, moon bonus selection and silent/still observability.
- Instant casts stay instant and uninterruptible; a later cast with the same
  spell ID is not mistaken for the watched cast.
- Two counterers, competing readied strike, equal deadlines in both insertion
  orders, late dispatch, expiry and encounter entry/exit preserve one reservation.
- Move, hide, change visibility/range, disconnect, extract and replace participants
  before trigger and before queued execution; no stale pointer or resource debit.
- Real hit, miss and prevented attacks trigger exactly once; rejected attacks and
  queued-action delegation do not create false or duplicate attempts. Include
  nested strikes, a killed ally, remote ranged attacks and DG extraction.
- Admission failures cost no action/resource; execution failures do not refund
  the already-reserved action; later unrelated actions are not cancelled.
- Help agrees in the flat file and development database, resource and registry
  counts return to baseline, full tests pass and `make install` follows them.

This design resolves the rule choices in #106. Runtime implementation and the
listed acceptance tests remain outstanding; the issue is not complete.
