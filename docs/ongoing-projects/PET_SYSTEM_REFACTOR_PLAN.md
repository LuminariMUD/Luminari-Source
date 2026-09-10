```text
###############################################################################
# !!!!!!!!!!!!!!!!!!!!! PHASE 1 SCOPE LOCK !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! #
#                                                                             #
# ULTIMATE GOAL: DELIVER GAME FEATURE PARITY WITH BOTH REFERENCE REPOSITORIES.  #
#                                                                             #
# /home/aiwithapex/projects/RealmsOfLuminari                                     #
# Baseline: 4f945b0e0c0485178d1dbf531d82db91ef1060be                             #
#                                                                             #
# /home/aiwithapex/projects/duris                                              #
# Baseline: c27968e8ffa34f7baa38e0f27d653debb7b8cad4                             #
#                                                                             #
# KEEP THIS BLOCK FIRST. DO NOT DELETE, SHORTEN, MOVE, OR REPLACE REFERENCES.   #
# Preserve it through rewrites, compaction, handoffs, and completed-item cuts.  #
# Change these references or the parity objective only on user instruction.   #
# DELIVERY FIRST. NO PHASE 2. NO SPECULATIVE REDESIGN. NO ENDLESS GREEN RETESTS. #
###############################################################################
```

# Pet System Refactor - Phase 1 Game Feature Parity Delivery

## Delivery contract

The deadline has passed. Finish usable native pet gameplay equivalent to BOTH repositories
above, preserving Luminari class rules and balance. Matching names or internal APIs do not
establish parity. A player must be able to acquire, control where intended, use, recover or
release the pet through an actual supported game path. Existing native equivalents count;
foreign architecture and exact foreign balance numbers are not requirements.

The pinned commits keep the comparison finite. Inspect reference code at those commits;
do not silently chase newer reference changes. Reference repositories are read-only inputs.
Never remove a missing required capability merely to shorten the plan or claim completion.

Only work that closes a concrete reference gameplay gap, a remaining acceptance criterion,
or a correctness failure blocking those workflows belongs in Phase 1. Durability, offline
expiry, restart/restore redesign, and other deferred work stay in
[Phase 2](PET_SYSTEM_REFACTOR_PHASE_2_PLAN.md). No new framework, harness, tracking document,
foreign-internals port, cosmetic refactor, or unrelated cleanup belongs on this delivery path.

## Shortest execution path

1. **Resolve the known crash first.** Use the preserved recovery evidence below to investigate
   the purchase/transfer SIGSEGV. Inspect Petaccept's actual state before reuse. Establish the
   cause, repair the concrete failure, and verify the affected purchase/transfer path. Do not
   turn this blocker into a general stability project.
2. **Close one workflow at a time, starting with purchases.** Finish the interrupted purchases,
   then work down the remaining table, grouping related role bonuses, ownership, mounts, and
   lifecycle checks into the same native session. For each workflow, trace its relevant pinned
   reference capability and native equivalent first; reuse existing evidence; implement only
   the missing behavior; verify it and update its existing documentation/help in that batch.
   Do not defer reference comparison until after implementation or start a separate global audit.
3. **Deliver.** When the remaining workflows and both reference comparisons are accounted for,
   run the final delivery gate below and close Phase 1. Do not add another design or testing phase.

Before each implementation batch, apply
[ablation](../../.agents/skills/ablation/SKILL.md) briefly: name the missing player behavior or concrete
failure, the smallest existing path to change, and the proof needed.
Omit any proposed work that cannot answer which requirement or concrete correctness risk it
addresses. This is a brief check, not a new report or approval ceremony.

Reuse completed evidence below. A green test or successful live workflow stays closed unless a
relevant change or observed failure invalidates it. Batch shared acceptance; do not exercise
every role through every identical infrastructure path. Keep role-specific mechanics covered.
Use the narrowest relevant existing regressions during repair and the required integration
checks for coherent code batches; do not repeatedly rerun green suites without cause.

Remove completed work from the remaining table, retaining evidence in the existing test
artifacts and system/help documentation. Preserve the scope-lock block and recovery evidence.
Do not replace open items with new speculative tasks. Report concrete closures, remaining
blockers, and the next delivery action; tests passing alone does not mean Phase 1 is delivered.

## Reference anchors for each workflow

These are source entry points, not substitutes for tracing callers, class access, and commands
at the pinned commits. Both references must be reconciled against the remaining gameplay scope.

| Reference | Start tracing here | Required comparison |
| --- | --- | --- |
| `/home/aiwithapex/projects/RealmsOfLuminari` | `src/specs.mobile.c`, `src/actnoff.c` (`do_claim`), `src/actcomm.c` (`do_order`), `src/follow.c` | Keeper services, claiming/recovery, ownership versus following, and usable orders. |
| `/home/aiwithapex/projects/RealmsOfLuminari` | `src/elementalist.c`, `src/mount.c`; trace necromancy acquisition from command/spell registration | Elementalist choices, necromancy, and mount gameplay. |
| `/home/aiwithapex/projects/duris` | `src/classes/necromancy.c` | Undead forms and control costs, opt-in raising, hosts, and construct gameplay. |
| `/home/aiwithapex/projects/duris` | `src/classes/sillusionist.c`, `src/combat/fight.c` (`PLR3_PET_DAMAGE`) | Owned non-orderable illusions and optional owner pet-combat feedback. |

For each compared capability, record the reference source/symbol, usable native path, and
supporting existing test or live evidence in the existing system documentation or test artifact.
If missing, keep it open in the matching workflow below and implement its native equivalent.
Do not create a second checklist. A disabled sketch, direct handler test, or matching name is
not evidence of player access.

For every role and added choice, finish acquisition/source and cost, control cost, equipment
rules, duration, cooldown, and dismissal in the existing system/help documentation. Reuse
already correct entries. Check both help stores for changed player behavior.

## Remaining gameplay acceptance

No Phase 1 gameplay workflow remains open. Bonded/class roles, role bonuses, undead/divine
choices, opt-in raising and hosts, constructs, other acquisition routes, ownership/combat,
mounts, and lifecycle/equipment are reconciled and verified at their required scope in
[final acceptance](../testing/pet-phase1-final-acceptance-2026-09-09.txt). Purchase acceptance
is recorded separately in
[purchase acceptance](../testing/pet-phase1-purchases-2026-09-09.txt). The quest reward uses
the production-linked `complete_quest` path because this world contains no authored follower
reward quest; inventing unrelated quest content solely for acceptance is outside the scope lock.


## Final delivery gate

- [x] Finish relevant production-linked regressions for changed behavior and uncovered acceptance
  failures. Use isolated MariaDB for database bodies; keep live acceptance on game port 4100.
- [x] Run the required checks for the final coherent change batch, including
  `LUMINARI_TEST_MYSQL_ENABLE=1 make -j$(nproc) test` against the isolated test database,
  followed by `make install`. Resolve compiler warnings, inspect hook changes, and complete
  applicable pre-commit checks. Never leave a root-level `luminari` binary.
- [x] Update changed player behavior in both the development help database and
  `lib/text/help/help.hlp`, with matching tracked SQL where applicable. Verify changed topics
  and aliases in game; review conflicting/duplicate aliases explicitly before changing them.
- [x] Commit reviewable, verified Phase 1 changes. Close Phase 1 only when every workflow and
  reference capability above has direct evidence at its required scope; leave any unverified
  item explicitly open. Do not pull Phase 2 work into this deadline pass.

## Preserved recovery evidence: work stopped on 2026-09-09

Phase 1 was incomplete when this historical recovery context was written. It is not a new
execution gate. At that handoff, autorun had terminated at 06:39:44 IDT and port 4100 had no
listener; that state and the instruction not to restart were specific to the stopped handoff.

### First issue at handoff: crash during native purchase acceptance

- Investigate the SIGSEGV at 06:34:22 IDT (03:34:22 UTC), before continuing purchase acceptance.
  Installed build: `bin/releases/092eae044ae3fcaa08bf07eea9070e66f21286a7/luminari`.
  This build includes the changes subsequently committed as `2888dd2b2`; its embedded identity
  reports the preceding commit with a dirty tree because installation preceded the commit.
- Setup: staff Kohdee and test character Petaccept at Fredrick's guild shop, room/shop/keeper
  103489. Petaccept bought two producing-stock rogue tokens (104300), creating General followers.
  CHA 30 permits multiple General followers; the second successful purchase was not a capacity
  bug. The first script incorrectly expected denial, then disconnected both sessions.
- A second script reconnected both characters, ran staff `goto 103489`, then `trans Petaccept`
  to the same room. Transfer output appeared, the command marker timed out, and the server
  crashed. The following planned CHA change and follower cleanup were never reached.
  Same-room transfer with purchased followers is a reproduction lead, not an established cause.
- Petaccept may retain two purchased rogues and the spent gold. Inspect actual saved/live state
  before cleanup or further purchases; do not assume exact IDs or balances. No quest-point,
  account-experience, or shop configuration changes were made during this investigation.
- Local evidence: `/tmp/pet-phase1-purchase-gold-live.log`,
  `/tmp/pet-phase1-purchase-gold-final-live.log`, and
  `log/last_error_2026-09-09T03:34:22.459Z.json`. These are local scratch/runtime files, not
  committed artifacts. No backtrace was captured; the WSL kernel core pipe provided no
  retrievable core. Same-user GDB attachment failed, while debugging a child process worked.
- A temporary diagnostic launch used `LD_PRELOAD=/tmp/pet-debug-attach.so` (source:
  `/tmp/pet-debug-attach.c`, enabling `PR_SET_PTRACER_ANY`). It was stopped before reproduction
  or debugger attachment. No persistent debugger configuration was changed. The later exit 143
  at 06:39:43 was the requested stop, not the original SIGSEGV. A future normal launch must use
  `MUD_PORT=4100 ./scripts/autorun/autorun.sh` without that temporary preload.

Resolution on 2026-09-09: the same-room transfer was only where the delayed failure surfaced.
GDB showed `character_periodic` traversing a freed temporary login character after unrelated
mobile movement. `free_char()` detached periodic ownership early, but affect removal could
re-register a still descriptor-owned character before final release. The final release boundary
now detaches it again, and the existing regression frees an affected character to cover the
path. Link loss, reconnect, pet commands, and continued server health passed on port 4100.

The interrupted purchase acceptance is complete for gold, quest points, and account experience.
It also found and repaired an account-experience persistence error by routing the debit through
the existing persistent account helper. Full currency, stock, denial, paid-token recovery,
ownership, duplicate prevention, live cleanup, and database evidence are recorded in
[purchase acceptance](../testing/pet-phase1-purchases-2026-09-09.txt). Purchases are therefore
removed from the remaining table.

### Evidence and practical continuation notes

Completed native summon-choice acceptance is recorded in the existing artifacts below; keep
that removed workflow closed and use these records rather than repeating it:

- [Ordered spells](../testing/pet-phase1-ordered-spells-2026-09-09.txt): planar healer known-spell
  orders, target rejection, slot exhaustion, action costs, and owner feedback (`f6e3de357`).
- [Genies](../testing/pet-phase1-genies-2026-09-09.txt): guardian and all four native genie choices,
  ordered abilities, denial, dismissal, and test-character spell setup repair (`959323ddd`).
- [Elementals and plants](../testing/pet-phase1-elemental-plant-2026-09-09.txt): Summoner and Druid
  choices, swarm and shambler batches, combat/dismissal, and preflight denial retaining prepared
  spells (`2888dd2b2`). The latest coherent code batch passed all 1354 tests and `make install`,
  with no compiler warnings, before the subsequent live purchase crash. Passing tests do not
  establish resolution of that crash.

Earlier completed changes include keeper snapshot-cache invalidation (`3e324193d`), all authored
Dragon Rider mount choices (`2d4473066`), and native Call Lycanthrope item access (`93e05c335`).
The remaining role progression, respec, lifecycle, source, construct, undead, and purchase
requirements were closed in the final evidence linked above. Temporary live currency, crafting,
source-object, and follower setup was bounded and restored or removed after verification.

Use the existing `scripts/development/dev_kohdee_login_smoke.sh` for native sessions; it reads
credentials quietly. Timed casts need explicit waits. Named pet commands require visibility
(air elementals need `detect invisibility`), and dismissal must target each pet by name or ID;
`dismiss followers` is unsupported. Purchased rogues use General capacity, not the class
Mercenary slot. Verify the intended capacity before asserting purchase denial.

The final coherent run used the disposable isolated test database on 127.0.0.1:33379 and passed
all 1354 tests, followed by `make install`. Never substitute the live database for isolated
regression tests.
