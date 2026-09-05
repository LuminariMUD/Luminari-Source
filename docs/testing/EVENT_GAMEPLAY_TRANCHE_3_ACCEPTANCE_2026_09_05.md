# Tactical readied attacks: tranche 3 acceptance

Date: 2026-09-05
Branch: `refactor/fight-combat-safety`

## Delivered behavior

`ready attack <caster> on casting` binds a visible local caster. Players never
supply a cast ID. A typed CastingStarted fact queues one native attack against
the committed cast ID, before the timed cast can complete. Damage invokes the
existing concentration check. A miss or successful check preserves casting.
Entry and door attacks use the same reservation and single-strike path.

## Requirement evidence

| Requirement | Implementation and verification |
| --- | --- |
| Typed start and stable identities | `domain_casting_started` carries caster, target, room and activity ID. Production casting tests verify one start per accepted cast and distinct IDs after recasting. |
| One action paid at arming | All three attack triggers use `USE_STANDARD_ACTION`; execution bypasses command costs. Tests verify unavailable action after arming and no extension of cooldown at execution. |
| Expire at next turn | Encounter turn entry calls `ready_action_on_semantic_turn` before budget recovery and dispatch. A test arms outside combat, joins three seconds later and verifies readiness survives the old wall deadline but expires before the next semantic turn's phase callback. |
| Bounded outside-combat readiness | One native owner expiry deadline; combat-state facts cancel/restore it. Tests verify six-second expiry for entry, door and casting. |
| One normal strike | `resolve_hit` bypasses the queued special-attack dispatch for a readied attack. Real combat tests observe one damage fact and verify a queued kick remains untouched. No opening initiative or full-attack routine is called. |
| Hit, miss and concentration | Real `cast_spell`, scheduler, hit and damage paths cover failed concentration, successful concentration and a miss. Cancelled casts cannot complete later. |
| Ordering | An overdue-dispatch test advances both reaction and cast deadlines into the same scheduler pass; the strike cancels the cast before its due callback resolves. Native scheduler ordering is deadline then insertion sequence. |
| Revalidation | Single-strike admission checks position, activity, incapacity, visibility, melee eligibility, same-room and single-file reach. Normal hit retains PvP, peaceful-room checks and defenses. A real combat test loses visibility after arming and confirms no attack damage. |
| Lifetime cleanup | Scoped owner subscriptions cancel timers on owner extraction/shutdown. Tests cover cancellation, owner movement/death/extraction, target movement/extraction, next-turn and wall expiry, replaced casts, door close/replacement/reset and native callback cleanup. |
| All attack triggers share accounting | Production entry and door events release the same reserved single strike as casting; their real damage interrupts a timed cast. |
| No extra scheduler | Only `action.ready.execute` and `action.ready.expire` native types are used. Scoped facts and the encounter turn hook drive state changes; there is no armed-character polling loop. Architecture checks pass. |
| Player documentation | READY/READIED-ACTION flat-file help and the existing development `ready-action` database entry match. Existing keyword aliases remain, and the previous database body is stored in `help_versions`. |

## Validation results

- `make -j8 test`: passed, including all 1,123 production-linked CuTests and
  repository architecture/tooling checks (15 additional tests over tranche 2).
- `make install`: passed; current server installed and root `luminari` removed.
- CMake Debug build: passed; all 19 CTest targets passed.
- Valgrind full production-linked suite, including child processes: passed with
  zero reported errors and no definite or indirect leaks. No suppressions used.
- Modified C/header files formatted with clang-format 18; no compiler warnings
  or diff whitespace errors reported by final validation.

Local logs: `/tmp/tranche3-final-make-test.log`,
`/tmp/tranche3-final-install.log`, `/tmp/tranche3-cmake-build.log`,
`/tmp/tranche3-ctest.log`, `/tmp/tranche3-valgrind-tests.log`, and
`/tmp/tranche3-valgrind.*.log`.

## Scope and limits

This tranche supports a normal melee or unarmed strike. It does not add full
counterspelling, ranged readied attacks, instant-spell interruption or
ally-attacked triggers. Noncombat readiness supports explicit say, emote, look,
rest, stand, sit, open and close commands; arbitrary combat commands and aliases
are rejected rather than given an unreserved execution path.

Entry targets bind on arrival. Their departures are watched by a room-scoped
subscription installed at arming, because the bus rejects subscription changes
during dispatch. Death or extraction within the one-pulse execution window is
rejected by resolving the stable target handle at execution. Casting and door
targets are known at arming and have subject-scoped terminal subscriptions.

No live server restart or production database write was performed. Gameplay
verification uses the production-linked test executable and real event/combat
paths, rather than a manual player-session smoke test.
