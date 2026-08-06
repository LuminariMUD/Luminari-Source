# Necromancer Class End-to-End Audit

## Audit state

- Audit date: 2026-08-06
- Status: Implementation in progress
- Release verdict: Not ready
- Implementation baseline: commit
  `0262cf59ca37f250dafe278406289ca594fe620d`
- Implementation branch: `codex/necromancer-completion-20260806`
- Environment: development, as identified by `lib/.env`
- Runtime mutation: none

This document audits the Necromancer prestige class from eligibility and level
advancement through feat execution, summoned mobile creation, follower admission,
pet persistence calls, live help, and automated coverage. It is a working audit,
not an authoritative description of intended game behavior.

Existing unrelated working-tree changes were preserved and excluded from this
audit.

The initial audit used direct source tracing, the development help database, and
the live world mobile prototypes. No production system was accessed or changed.
The initial audit did not run the build or test suite because it made no source
change and the existing suite contained no Necromancer behavior coverage.

## Implementation progress

Last updated: 2026-08-06, isolated worktree and branch created from the development
master checkpoint. This table is updated with every implementation checkpoint.

| Finding | Status | Verification evidence |
|---------|--------|-----------------------|
| NEC-001 | Focused verification passed | Exactly one selected base class advances; seven progression/study tests pass. |
| NEC-002 | Focused verification passed | Safe kit cardinality and description ownership; ASan/UBSan clean. |
| NEC-003 | Focused verification passed | Valid attempts spend the correct daily use and swift action; dispatcher and queue enforcement are tested. |
| NEC-004 | Focused verification passed | Innate cast type, selected progression level, fallback, and `1d4+1` duration are tested. |
| NEC-005 | Focused verification passed | At-will summon casts bypass prepared and spontaneous resource extraction. |
| NEC-006 | Focused verification passed | Non-Necromancers allow one animated undead; Necromancers allow exactly two. |
| NEC-007 | Focused verification passed | Greater Animation applies its computed tier-scaled mobile level. |
| NEC-008 | Focused verification passed | Deathless Touch empowers and is consumed by one successful eligible summon. |
| NEC-009 | Focused verification passed | Every equipped armor material is aggregated independent of slot order. |
| NEC-010 | Focused verification passed | Shared affect and timed-event admission enforce stun immunity; direct producers are guarded. |
| NEC-011 | Focused verification passed | Animate Dead and Greater Animation share the holy-room rejection policy. |
| NEC-012 | Focused verification passed | Immediate and delayed casts use the one selected progression for summon tiers. |
| NEC-013 | Focused verification passed | Resistances apply to the cohort; mandatory Undead Appearance is free and the budget is nonnegative. |
| NEC-014 | Focused verification passed | Pending first-level choices now drive known-spell study before save. |
| NEC-015 | Open | Authoritative help update and database verification pending. |
| NEC-016 | In progress | Twenty-seven production-linked tests added; full-suite environment exception recorded below. |

### Checkpoint 1: selected spell progression and first-level study

- Necromancer bonus caster levels now advance only the selected preferred base
  class. If no preference is needed because exactly one supported base class is
  present on the chosen side, that class is inferred. Ambiguous multi-class
  selections remain unresolved until the player chooses a preferred class.
- Study now uses the pending level-up casting selection, so a first-level
  spontaneous Necromancer can choose newly earned known spells before saving.
  Study refuses to save an ambiguous Necromancer progression and directs the
  player to complete the casting-type and preferred-class choices.
- Paladin and Ranger are available in the divine preferred-class menu, matching
  the divine caster-level calculation already used by the game.
- Seven production-linked CuTest cases cover selected arcane and divine classes,
  sole-class inference, ambiguous multi-class rejection, and pending arcane and
  divine known-spell study. The warning-free test binary builds successfully.
- The local suite currently reports 446 runs, 445 passes, and one environment
  failure. The unrelated encounter-world syntax test cannot boot from this fresh
  worktree without ignored database configuration. Pointing it at the existing
  development data reaches that data but is incompatible with the fresh
  worktree's required example configuration headers. No credential or customized
  configuration file was copied or changed.

### Checkpoint 2: Bone Armor safety and aggregation

- Bone Armor now counts the complete crafting-kit contents list and accepts
  exactly one object. Empty and multi-object kits return before any object
  dereference or conversion work.
- Description replacement has one owner transition per string. The room
  description is freed once, eliminating the normal conversion double free.
- Spell-failure reduction now requires at least one equipped armor piece and
  keeps an aggregate all-bone result across body, head, legs, arms, and shield
  slots. A later bone slot can no longer erase an earlier non-bone result.
- Four additional production-linked tests cover zero, one, and two kit objects,
  replacement of allocated descriptions, mixed materials in the formerly
  order-sensitive direction, and the two-rank all-bone reduction. The normal
  suite reports 450 runs, 449 passes, and the same unrelated environment
  failure described above.
- An isolated ASan/UBSan build with leak detection passed the focused
  Necromancer suite: 11 runs, 11 passes, and no sanitizer finding. The
  instrumented full suite reached the known bootstrap assertion; its CuTest
  failure-message allocation is retained at exit, so it is not a clean
  leak-detection target in this fresh worktree.

### Checkpoint 3: Touch of Undeath execution contract

- `undeath` now has one swift-action contract at the command dispatcher, queued
  action executor, and direct handler. A queued swift command remains pending
  until a swift action is available, and a valid attack attempt consumes the
  swift action immediately before its attack roll.
- A valid attempt spends a Touch of Undeath daily use whether the touch attack
  hits or misses. Invalid room, target, undead-target, and player-killing checks
  return without spending either resource. The cooldown is attached to
  `FEAT_TOUCH_OF_UNDEATH`; Touch of Corruption is no longer charged.
- Touch effects use `CAST_INNATE` and the selected Necromancer progression's
  arcane or divine level. Legacy characters without an unambiguous selection
  fall back to their Necromancer class level instead of silently resolving at
  zero. Paralyzing Touch now lasts `1d4+1` rounds.
- Five additional production-linked tests cover daily-use breakpoints, selected
  arcane and divine levels, fallback and cast type, the full duration range,
  cooldown/action consumption, queue enforcement, and command registration. The
  normal suite reports 455 runs, 454 passes, and the same unrelated environment
  failure described above.
- The isolated ASan/UBSan build with leak detection passed all 16 focused
  Necromancer tests with no sanitizer finding.

### Checkpoint 4: animated-undead summon lifecycle

- At-will Necromancer summons are identified before the destructive preparation
  hook, so neither a prepared copy nor a spontaneous slot is consumed. They
  remain at-will spells with the normal spoken-cast action, concentration, and
  failure semantics; failed validation or the ten-percent summon roll consumes
  no spell resource and leaves the corpse in place.
- Both immediate and delayed casts now derive their effective level from the one
  selected Necromancer base progression. The summon routine uses that supplied
  level for mobile-tier selection instead of the composite all-class caster
  macro. Legacy characters without an unambiguous selection fall back to their
  Necromancer class level.
- Animated-undead admission now counts all matching charmed followers and tests
  `current < allowance`. Non-Necromancers may control one and Necromancers may
  control exactly two; the extra allowance no longer spills into other follower
  flags.
- Greater Animation applies its calculated tier-scaled mobile level instead of
  overwriting every result with level 18. It now shares Animate Dead's holy-room
  rejection policy.
- Deathless Touch contributes only to Animate Dead or Greater Animation and is
  cleared after one successful eligible summon. Invalid corpses, holy rooms,
  random failure, follower-cap rejection, and mobile-load failure retain it.
- A pet-persistence failure does not destroy an already-created follower or
  restore an already-consumed corpse. The existing structured error log remains,
  and the player now receives a warning to save again before disconnecting.
- Seven additional production-linked tests cover the complete contract above.
  The normal suite reports 462 runs, 461 passes, and the same unrelated
  environment failure described above. The isolated ASan/UBSan suite passes all
  23 focused Necromancer tests with leak detection enabled.

### Checkpoint 5: Tough as Bone and Undead Cohort

- Stun admission is now enforced in both shared representations. An `AFF_STUN`
  affect is rejected by `affect_to_char_source()` when `can_stun()` denies the
  victim, and an `eSTUNNED` event is rejected and freed before it enters the
  event queue. `can_stun()` also safely rejects a null target.
- The confirmed direct producers now check immunity before success messaging or
  event attachment: Stunning Critical, Stunning Fist, Pressure Point Strike,
  Singular Impact, and stun traps. The trace also found and repaired Berserker
  Stunning Blow. Existing spell, poison, and weapon-proc paths already checked
  `can_stun()`; the shared gates protect future and load-time callers.
- Fire, Cold, Acid, Electricity, and Sonic resistance evolutions now modify the
  cohort passed to `assign_eidolon_evolutions()`, never its owner.
- One automatically granted Undead Appearance rank is free for a Necromancer.
  The evolution menu and eligibility checks now use the same cost-aware point
  calculation, and invalid legacy overspending is displayed as zero rather than
  a negative point balance. A level 1 Necromancer has one usable cohort
  evolution point after the mandatory identity grant.
- Four additional production-linked tests exercise affect-based and event-based
  stun admission, all five cohort resistance targets, the mandatory evolution
  grant, menu availability, and a nonnegative overspent budget. The normal suite
  reports 466 runs, 465 passes, and the same unrelated environment failure
  described above. The isolated ASan/UBSan suite passes all 27 focused
  Necromancer tests with leak detection enabled.

## Executive verdict

The Necromancer class is registered, selectable when its prerequisites are met,
and all advertised class feats are assigned at a level. The original progression,
Bone Armor, Touch of Undeath, and animated-undead summon blockers now have focused
production-linked and sanitizer verification.

The release verdict remains not ready while authoritative help and final
build/install verification remain open. The original findings below are retained
as the audit record; the implementation-progress table and checkpoints are the
current status.

## Class registration and progression

The core class definition is present in `src/character/class.c:9164-9238`:

- Ten-level, locked prestige class, in game, with a 5,000 account-point unlock.
- Low base attack bonus, d6 hit die, two movement points, and four base skill
  trains per level.
- Good Will and death saves; bad Fortitude, Reflex, and poison saves.
- Prerequisites are Arcana 5, Religion 5, any fourth-circle spellcasting, and one
  of the six non-good alignments.
- The player is prompted to choose arcane or divine progression. The choice is
  saved in the `NecC` player-file field.

Automatic class-feat assignment is processed at the exact class level and the
feat array is persisted. The special level hooks also exist: +4 real Strength at
level 6, Weapon Focus: Polearms at level 5, Weapon Specialization: Polearms and a
bonus class-feat point at level 7, and Undead Appearance for the cohort at level 1.

### Level-by-level feature matrix

| Level | Granted behavior | Audit result |
|-------|------------------|--------------|
| 1 | Necromancer weapons; Undead Cohort | Weapons and cohort creation work; resistance ownership and the free mandatory identity grant pass focused checks. |
| 2 | Summon Undead | Focused lifecycle verification passed; see Checkpoint 4. |
| 3 | Ultravision | Implemented through the standard feat visibility check. |
| 4 | Light armor; Bone Armor rank 1 | Proficiency and the repaired conversion/failure contract have focused verification. |
| 5 | Deathless Vigor; Weapon Focus: Polearms | Implemented: +4 Fortitude and the weapon-family focus hook are present. |
| 6 | Undead Graft; Touch of Undeath; Paralyzing Touch | Strength and the repaired touch execution contract have focused verification. |
| 7 | Tough as Bone; Weakening Touch; Weapon Specialization; bonus class-feat point | Weapon, point, Touch, and centralized stun-immunity behavior pass focused checks. |
| 8 | Medium armor; Bone Armor rank 2; Degenerative Touch | Proficiency, Bone Armor, and Touch behavior pass focused checks. |
| 9 | Summon Greater Undead; Destructive Touch | Greater Animation scaling and Touch behavior pass focused checks. |
| 10 | Essence of Undeath; Deathless Touch | Most Essence checks are wired; one-shot summon empowerment passes focused checks. |

## Prioritized findings

| ID | Severity | Finding |
|----|----------|---------|
| NEC-001 | Blocker | The chosen spell progression is bypassed by class-specific bonus caster levels. |
| NEC-002 | Blocker | `bonearmor` can dereference an empty kit and double-free an ordinary armor description. |
| NEC-003 | Blocker | Touch of Undeath spends the wrong daily feat and has unenforced/conflicting action accounting. |
| NEC-004 | High | Touch of Undeath passes a prerequisite enum as a runtime cast type, always uses arcane level, and implements the paralysis duration incorrectly. |
| NEC-005 | High | The at-will summon feats can consume a prepared spell or spontaneous slot before summon validation. |
| NEC-006 | High | Necromancers cannot control the intended second animated undead because the follower comparison is inverted. |
| NEC-007 | High | Greater Animation calculates a scaled level and then overwrites every summoned mobile to level 18. |
| NEC-008 | High | Deathless Touch's promised next-summon enhancement is never consumed. |
| NEC-009 | High | Bone Armor spell-failure reduction depends only on the last iterated armor slot, not on all required pieces being bone. |
| NEC-010 | Medium | Tough as Bone's stun immunity is not enforced at every stun ingress. |
| NEC-011 | Medium | Animate Dead is blocked in holy rooms while Greater Animation is not. |
| NEC-012 | Medium | Summon tier selection uses a composite total caster level, not the spell level or one selected casting progression. |
| NEC-013 | Medium | Undead Cohort resistance evolutions modify the owner instead of the cohort. |
| NEC-014 | Medium | A first-level spontaneous Necromancer cannot select a newly earned known spell in the same study session. |
| NEC-015 | Medium | Live class and spell help is stale, incomplete, and ambiguous around the summon feats. |
| NEC-016 | High | No production-linked regression test exercises Necromancer progression, feats, summons, Bone Armor, Touch, or cohort behavior. |

## Finding details

### NEC-001: selected spell progression is not honored

The study flow correctly prompts for arcane or divine progression and persists the
choice (`src/character/study.c:1959-1981`, `3468-3485`, and `499`; `src/players.c:1432-1433`
and `2507-2508`). The aggregate `compute_arcane_level()` and
`compute_divine_level()` helpers also consult that choice.

The class-specific path does not. `compute_bonus_caster_level()` in
`src/utils.c:206-237` adds every Necromancer level to each of Wizard, Sorcerer,
Bard, and Summoner, and separately to each of Cleric, Druid, Ranger, Paladin, and
Inquisitor. It never reads `NECROMANCER_CAST_TYPE(ch)` or a preferred base class.
That helper feeds actual spell-circle, preparation, slot, and known-spell capacity
calculations in `src/magic/spell_prep.c`.

For example, a Wizard/Cleric/Necromancer advances both Wizard and Cleric casting
even after choosing only one side. A character with more than one arcane base
class advances every listed arcane class at once. This contradicts the registered
class contract of one selected progression per Necromancer level.

### NEC-002 and NEC-009: Bone Armor is unsafe and its bonus is unstable

`bonearmor()` in `src/craft/craft.c:1581-1684` contains three independent defects:

- The contents loop breaks after its first object, so `num_objs > 1` can never
  detect extra objects.
- An empty kit leaves `obj == NULL`, followed by `GET_OBJ_TYPE(obj)` at line 1608.
- Lines 1657-1660 free `obj->description` twice without clearing the pointer. A
  normal described armor item therefore reaches a double free.

Separately, `compute_gear_spell_failure()` in
`src/combat/assign_wpn_armor.c:1586-1620` resets `bonearmor` for every relevant
equipped item. The final value describes only the last iterated armor slot. Mixed
bone and non-bone equipment can gain or lose the reduction solely because of wear
slot order, despite the feat text requiring all armor pieces to be bone.

The level 4 and level 8 rank grants are present, and the reduction is ten percent
per real rank when the final boolean is true. The feature is still unsafe to use.

### NEC-003 and NEC-004: Touch of Undeath is not resource-safe

The command and all five touch variants are registered. The daily-use calculation
also returns one use at levels 6-7, two at 8-9, and three at level 10. The execution
path in `src/combat/act.offensive.c:12643-12798` breaks that wiring:

- A successful hit starts the cooldown for `FEAT_TOUCH_OF_CORRUPTION`, not
  `FEAT_TOUCH_OF_UNDEATH`. The `eTOUCHOFUNDEATH` event is therefore not attached
  and the Necromancer's displayed uses do not decrease.
- A missed touch returns before charging either a use or an action, permitting
  cost-free retries.
- The command table declares `undeath` as `ACTION_SWIFT`
  (`src/interpreter.c:4641-4649`), but the handler calls `USE_STANDARD_ACTION`.
  The dispatcher only gates standard and move requirements at
  `src/interpreter.c:6289-6293`, so the declared swift requirement is not enforced
  and the handler does not check standard availability before executing.
- `call_magic()` receives `CASTING_TYPE_ARCANE`, whose value 1 comes from the feat
  prerequisite namespace. Runtime magic interprets cast type 1 as `CAST_POTION`;
  the correct innate value is `CAST_INNATE` (5).
- The effect level is always `compute_arcane_level(ch)`. A divine-track
  Necromancer can therefore resolve with level zero or an unrelated low arcane
  level. Weakening, Degenerative, and Destructive Touch use that value as their
  duration, and saves/DC behavior also receives it.
- Paralyzing Touch promises `1d4+1` rounds, but the implementation calls
  `dice(1, 4 + 1)`, producing `1d5` instead.

The variants themselves are gated at the intended class levels and route to the
expected ability IDs. The shared execution contract must be corrected before any
variant can be considered complete.

### NEC-010: Tough as Bone was only a partial immunity

`can_disease()` and `can_stun()` both reject their effects when the victim has
Tough as Bone (`src/utils.c:7385-7386` and `7567-7568`). Standard spell paths that
call those helpers work.

Stun admission is not centralized. Confirmed bypasses directly set `AFF_STUN` or
attach `eSTUNNED` without calling `can_stun()`, including:

- Stunning Critical in `src/magic/magic.c:5726-5737`.
- Stunning Fist in `src/combat/fight.c:12657-12687`.
- Pressure Point Strike in `src/combat/fight.c:13216-13255`.
- Singular Impact in `src/character/perks.c:12084-12094`.
- Stun traps in `src/combat/traps_new.c:1018-1023`.

At the implementation baseline, `affect_to_char()` did not apply an immunity
guard, so these direct callers bypassed the feat. Checkpoint 5 records the shared
affect/event admission repair and direct-producer checks that close this finding.

## Summon Undead and Summon Greater Undead

### Registration and user-facing mapping

The feat names are not the spell names:

| Class feat | Class level | Runtime spell | Command syntax |
|------------|-------------|---------------|----------------|
| Summon Undead | 2 | Animate Dead | `cast 'animate dead' <corpse>` |
| Summon Greater Undead | 9 | Greater Animation | `cast 'greater animation' <corpse>` |

`isPaleMasterMagic()` maps these feats to the two spells, and `canCastAtWill()`
allows them without ordinary spell availability. Both spells are registered as
room-object-targeted necromancy summons. The old Pale Master naming remains in the
helper, but it maps the current Necromancer class feats.

### Confirmed successful pipeline

For a successful cast, the implementation does all of the following:

1. Requires a corpse object in the room.
2. Selects an existing mobile prototype from caster-level bands.
3. Applies a configured ten percent summon failure chance.
4. Creates the mobile, charms it, applies summon configuration and augmentation
   bonuses, and applies Necromancy Spell Focus bonuses.
5. Adds it as a follower and joins the leader's group when applicable.
6. Moves the corpse contents into the follower and extracts the corpse.
7. Calls `save_char_pets(ch)` and tells the player how to dismiss the follower.

All eight selected mobile prototypes exist in `lib/world/mob/0.mob`, and their
prototype flags include `MOB_ANIMATED_DEAD`. There is no summon-expiration event;
the live spell help describes the followers as permanent, so permanence is
internally consistent.

The persistence call is present, but that is not proof of durable or failure-safe
storage. Pet durability still depends on the separate
[pet-persistence investigation](production-crash-2026-08-05-pet-persistence.md),
and these summon callers do not report a failed pet save to the player.

### NEC-007: Actual selection and level behavior

The world prototype levels below were verified from the current mobile data.

| Spell | Composite caster level | Prototype | Prototype level | Actual summon level |
|-------|------------------------|-----------|-----------------|---------------------|
| Animate Dead | 0-9 | Zombie | 6 | 6 |
| Animate Dead | 10-19 | Ghoul | 11 | 11 |
| Animate Dead | 20-29 | Giant Skeleton | 21 | 21 |
| Animate Dead | 30+ | Mummy | 29 | 29 |
| Greater Animation | 0-19 | Ghost | 15 | 18 |
| Greater Animation | 20-24 | Spectre | 19 | 18 |
| Greater Animation | 25-29 | Banshee | 23 | 18 |
| Greater Animation | 30+ | Wight | 27 | 18 |

The Greater Animation branch computes a randomized `mob_level` for every tier at
`src/magic/magic.c:11838-11869`. The later shared summon switch groups Greater
Animation with Summon Creature VIII and unconditionally sets level 18 at lines
12270-12275. The calculated value is never used. Creature identity changes with
caster level, but its actual level does not scale.

### NEC-005: At-will resource extraction

`cast_spell()` calls `spell_prep_gen_extract()` before it tests
`canCastAtWill()` (`src/magic/spell_parser.c:2753-2758`). The extraction helper is
destructive: it removes/requeues a prepared spell or consumes spontaneous slot
capacity.

As a result, a Necromancer who also has Animate Dead or Greater Animation prepared
or available from a spontaneous class can lose that real resource while using the
at-will feat. Extraction occurs before the summon routine rejects a non-corpse,
checks a holy room, rolls the ten percent failure, or rejects the follower cap.

The at-will path does bypass armor spell failure, but otherwise uses the ordinary
spoken casting, concentration, and timing path. That conflicts with the feat text's
word `innate`. The implementation needs one explicit contract: either a genuinely
innate command/action, or an at-will spell whose remaining normal cast semantics
are documented.

### NEC-006: Follower cap

Both spells call `can_add_follower_by_flag(ch, MOB_ANIMATED_DEAD)`. The helper sets
the intended allowance to two for any Necromancer and one for everyone else, but
then returns false when `undead <= undead_allowed`
(`src/utils.c:1116-1140`). With one existing animated undead, a Necromancer is
therefore rejected while attempting the second. The effective limit is one for
both Necromancers and non-Necromancers.

The same defect was independently noted in the
[pet-system comparison](PET_SYSTEM_COMPARISON_LUMINARI_CHRONICLES_OF_KRYNN.md).
The repair should use one central follower-admission policy and test the boundary
before relying on this generic flag helper for other pet categories.

### NEC-008: Deathless Touch enhancement

Killing a victim with Deathless Touch sets
`ch->char_specials.deathless_touch = true` and promises that the next Animate Dead
or Greater Animation follower receives increased stats
(`src/combat/fight.c:6476-6482`). Both summon paths read the flag and apply the
bonus (`src/magic/magic.c:12433-12444`).

Those are the only reader and writer in the source tree. No successful summon
clears the flag. Once earned, every later animated undead receives the bonus for
the remainder of that in-memory character lifetime, not only the next one.

### NEC-011 and NEC-012: Holy-room and caster-level contracts

Animate Dead explicitly rejects holy rooms at `src/magic/magic.c:11801-11805`.
Greater Animation has no corresponding check. There is no documented reason for
the stronger spell to bypass the restriction.

Both summon tiers ignore the `level` passed by the casting path and select their
prototype through `CASTER_LEVEL(ch)`. For ordinary players that macro sums divine,
arcane, Warlock, Alchemist, and Artificer levels, with special adjustments and a
cap (`src/utils.h:974-980`). It is not the selected Necromancer progression and is
not a single base class's caster level. This may be a legacy balance choice, but it
needs an explicit decision because multiclass characters can reach summon tiers
much earlier than a per-track interpretation would permit.

## NEC-013: Undead Cohort

The level 1 feat is executable through either `call cohort` or `call eidolon`.
Both create the shared Eidolon prototype at combined Summoner plus Necromancer
level, capped at 30, apply evolutions, add the follower, call pet persistence, and
attach the standard Eidolon call cooldown. The automatic Undead Appearance
evolution is granted at the first Necromancer level.

The implementation-baseline trace found two gaps:

1. `assign_eidolon_evolutions()` checks the cohort's resistance evolutions but
   adds Fire, Cold, Acid, Electricity, and Sonic resistance to `ch`, the owner,
   rather than `mob`, the cohort (`src/character/evolutions.c:916-935`).
2. The evolution pool adds one point per Necromancer level, then charges the full
   two-point cost of the automatically granted Undead Appearance evolution
   (`src/character/study.c:5894-5913`). A level 1 Necromancer with no Summoner
   levels can therefore begin at negative one free point. The intended treatment
   of this mandatory class identity needed to be decided and documented.

Checkpoint 5 resolves both gaps. Resistance evolutions now affect the cohort.
One class-granted Undead Appearance rank is exempt from cost, leaving the level 1
Necromancer's one point usable; all evolution menus use the same cost-aware,
nonnegative calculation.

## Passive feat results

The following paths are materially present and behaved consistently under static
trace:

- Necromancer Weapons: Scythe proficiency plus the level 5 polearm focus and level
  7 polearm specialization hooks.
- Ultravision: standard feat visibility integration.
- Light and Medium Armor Proficiency: standard proficiency grants at levels 4 and
  8.
- Deathless Vigor: +4 Fortitude save.
- Undead Graft: +4 real Strength at level 6.
- Essence of Undeath: critical-hit, sneak-attack, poison damage/status, sleep,
  ability-drain, paralysis, and death-magic checks are present in the relevant
  helpers and combat/magic paths audited.

Essence of Undeath is substantially wired, but it still lacks regression tests for
every immunity ingress. Tough as Bone's stun half now has shared affect/event
admission gates and direct-producer regression coverage as recorded in Checkpoint
5.

## NEC-014: Study and persistence gaps

The Necromancer casting choice is stored only in the level-up work area until the
study session is saved. The known-spell menu checks the already persisted
`NECROMANCER_CAST_TYPE(ch)` instead of the pending work-area value
(`src/character/study.c:2915-2952`). A character taking the first Necromancer
level with a spontaneous preferred class cannot select a newly earned known spell
in that same study session. Saving and entering study again exposes the selected
track, so this is a delayed first-level workflow rather than lost persistence.

Class feats and the cast-type selection themselves have player-file save/load
paths. Summons and the cohort call the shared pet persistence layer. The open pet
persistence work remains an end-to-end dependency for logout/restart durability.

## NEC-015: Live help audit

The development database contains authoritative rows for `class-necromancer`,
`animate-dead`, and `greater-animation`. It does not contain dedicated help tags or
keywords for `summon-undead`, `summon-greater-undead`, `touch-of-undeath`,
`bone-armor`, `undead-cohort`, `deathless-touch`, or `essence-of-undeath`.

The current class help is stale:

- It says Medium Armor arrives at level 7; source grants it at level 8.
- It says two skill points plus Intelligence; class data grants four base trains
  plus Intelligence, with a minimum of one.
- It describes hit-die gain as 4-6; the level routine now grants the full d6.
- It omits the summon feats, a complete level table, command syntax, follower cap,
  failure chance, holy-room behavior, and Touch/Bone Armor limitations.

The Animate Dead and Greater Animation rows correctly state the spell syntax,
corpse target, permanent duration, and caster-level-based creature selection. They
do not mention the ten percent failure chance, follower cap, or their inconsistent
holy-room behavior.

A separate stale `animatedead` help row says
`cast 'animatedead' <target corpse>`, which is not the registered Animate Dead
spell name. There is also an unrelated `animatedead` command for a different daily
feat, making the naming collision especially confusing. The Necromancer feat text
also calls the stronger spell `greater conjuration` in one place, and Tough as Bone
contains the typo `immunse`.

## NEC-016: Automated coverage

The root CuTest sources, focused protocol harness, `Makefile.am`, and
`CMakeLists.txt` contain no Necromancer-specific test. The only search hit is a
class-name entry used by a generic staff-set gameplay test; it does not exercise
class progression or a feat.

Minimum release coverage should include:

1. All prerequisites, level 1-10 grants, special level hooks, save/load, and
   respec/relevel behavior.
2. Arcane and divine choice cases with multiple base casting classes, proving
   exactly one intended progression advances.
3. First-level prepared and spontaneous study flows, including known spells.
4. Animate Dead and Greater Animation with no prepared copy, with a prepared copy,
   and from spontaneous classes; no at-will cast may spend a spell resource.
5. Non-corpse, holy-room, ten-percent-failure, follower-cap, and persistence-failure
   summon paths, including whether a corpse or spell resource is consumed.
6. Every summon tier, prototype, final level, animated-undead flag, augment bonus,
   group membership, corpse-content transfer, dismiss, save, logout, and reload.
7. Zero, one, and two existing undead for Necromancers and non-Necromancers.
8. Deathless Touch enhancement applying to exactly one successful eligible summon.
9. Touch daily uses, hit/miss charging policy, declared action type, arcane/divine
   scaling, cast type, saves, duration, immunity, and all five variants.
10. Bone Armor with an empty kit, multiple objects, a normal described item, every
    armor slot, mixed material sets, both ranks, save/reload, and sanitizer builds.
11. Tough as Bone and Essence of Undeath against every direct status ingress,
    including combat events, perks, traps, spells, and poisons.
12. Cohort point accounting, every resistance evolution, call/dismiss/cooldown,
    persistence, and combined Summoner/Necromancer scaling.
13. Database-backed checks for class, feat, command, and spell help.

## Recommended remediation order

1. Fix and sanitizer-test Bone Armor before allowing the command to be used.
2. Define one selected base spellcasting progression and repair every consumer of
   bonus caster levels.
3. Repair Touch of Undeath's daily event, action contract, cast type, chosen-track
   level, and variant math.
4. Move the at-will check before destructive spell extraction and test every
   failed summon path.
5. Correct the undead follower boundary, Greater Animation level assignment,
   one-shot Deathless Touch clearing, and holy-room consistency.
6. Centralize stun immunity enforcement and fix cohort evolution target/point
   accounting.
7. Add production-linked regression tests, then update the authoritative database
   help and enduring class documentation to match the approved contracts.

The class should not be described as fully implemented until the blocker and high
findings are repaired, the ambiguous scaling/action contracts are decided, and the
minimum regression matrix passes against the production-linked game sources.
