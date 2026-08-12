# Realms of Luminari Staged Pilot Manual Testing

- Status: Phases 4-5 complete; Phase 6 in progress
- Environment: disposable development runtime only
- Staged world: `lib/rol-conversion/runs/phase6-special-20260812-race-keep-stage/staging/world`
- Runtime contract: `lib/rol-conversion/runs/phase6-special-20260812-race-keep-stage/validation/pilot-runtime-contract.json`
- Live target writes: zero

## Safety Boundary

The five converted pilot packages are staged, not installed in `lib/world`. Do not
copy them over the normal development world or start them against the development or
production database for an exploratory test.

Prepare a disposable lib root with `scripts/ci/prepare_test_runtime.sh`. That script
requires an explicitly enabled, loopback-only MariaDB test database whose name contains
`test` or `ci`. After it succeeds, replace the disposable root's `world` directory with
a copy of the staged world above, run `bin/circle -c -d <isolated-lib-root>`, and then
start `bin/circle -d <isolated-lib-root>`. Use a staff test character that exists only in
that isolated database.

The preparer loads `sql/master_schema.sql` into the named database. Treat that database
as disposable. Never point its environment variables at an existing development or
production database.

## Pilot Packages and Entry VNUMs

| Package | Target zone | Rooms | Suggested entry VNUMs |
|---------|------------:|------:|-----------------------|
| `hulburg` | 1591 | 492 | 159100, 159342, 159349, 159353, 159354, 159430, 159487, 159553, 159564 |
| `swamp_two` | 20261 | 35 | 2026050, 2026084 |
| `theswamp` | 20409 | 99 | 2040901, 2040998 |
| `cemetery` | 20553 | 75 | 2055300, 2055311, 2055350, 2055373, 2055374 |
| `muspel` | 20586 | 459 | 2058600, 2058825, 2059053, 2059061, 2059062, 2059063, 2059064 |

The entry list contains one root for every disconnected physical-exit component. The
automated walkthrough already reached all 1,160 pilot rooms from these roots.

## Capabilities Available to Test

### Navigation and room presentation

- Move through every physical and vertical exit in the five packages.
- Inspect converted room names, descriptions, sectors, size approximations, extra
  descriptions, doors, door keywords, keys, hidden exits, and blocked exits.
- Confirm that unresolved exits were excluded rather than becoming accidental links.
- At Cemetery room 2055300, use `stat room` to confirm the level range is `15 and
  above`. A level 14 test character and a level 14 mount must be refused; a level 15
  character without an under-level mount may enter. Portal and teleport attempts obey
  the same range.
- At Swamp room 2040906 or 2040969, use `stat room` to confirm the underwater sector,
  then verify the target's underwater movement and breathing rules.
- Use `stat room` to inspect the inherited zone restrictions. Swamp Two room 2026050
  must be no-recall, no-teleport, and no-summon; Cemetery rooms must be no-teleport and
  no-summon; Muspel rooms must be no-recall, with additional local restrictions where
  the source supplies them.
- Compare normal recovery with healing rooms 2040934, 2040968, 2040969, 2040976,
  2040996, or 2055363. Each must carry the target regeneration flag.
- At Swamp Two room 2026051, rest or sit with a psionic character below maximum PSP.
  Its positive PSP tick gain must be twice the equivalent gain in an otherwise normal
  room and must not exceed maximum PSP.
- During active precipitation, confirm that flagged Muspel rooms 2059044-2059049 do
  not receive precipitation messages. Their daylight and room-light behavior must
  remain independent of the weather suppression.

### Resets and spawned content

- Force or wait for zone resets and inspect mobile and object population.
- Check mobile load limits, percentage loads, equipment, inventory, container contents,
  removals, followers, groups, mounts, door state changes, and calendar-conditioned
  resets.
- Exercise the converted `M`, `O`, `G`, `E`, `P`, `R`, `D`, `F`, `K`, `C`, and `X`
  reset families where the selected zones use them.
- In a reset chain with several inventory or equipment rows after one mobile load,
  confirm that a failed percentage or occupied equipment slot does not suppress later
  `E` or `G` rows for that same mobile.

### Shops

- Find the 14 appended shops in Hulburg and the converted shop in Muspel.
- Test listing, buying, selling, opening hours, keeper messages, accepted item types,
  price multipliers, and roaming-shop behavior.
- Muspel shop 2058829 is attached to its roaming keeper and deliberately has no fixed
  room. It produces objects 2058639, 2058641, 2058651-2058654, and 2058657, operates
  from hour 1 through 23, and uses normal buy/sell multipliers 1.25 and 0.5556.
- Compare otherwise equivalent human or elf and non-matching customers at shop 2058829.
  The human or elf must pay twice the post-GREED buy price and receive half the normal
  sell return. The omitted illithid identity has no target player-race counterpart, and
  the source `PZ` token was invalid in the source loader; neither should affect pricing.
- Shop 2058829 has no source `CASTING` disposition. Its awake keeper must refuse `cast`,
  `recite`, and `use` while permitting ordinary non-magic commands. Converted shops
  with `CASTING` permit those commands; native shops without an RoL policy bit retain
  their prior behavior.
- Confirm normal shop save/export does not remove a converted shop's adverse-price
  metadata. The optional `R <mask>~` record is not directly editable in `sedit`, but it
  must survive saving an otherwise edited shop.

### High-level quests

- Speak to converted quest-host mobiles and test ASK and GIVE entries.
- Confirm required coins and duplicate required items are consumed exactly.
- Test output commands for items, coins, attacks, disappear behavior, doors, kits,
  churches, and spell or skill teaching where the selected quest uses them.
- Phase 5 has added runtime support for configured experience rewards, signed quest-point
  changes, argument-free attacks, and all mapped source spell or skill rewards. Those
  additions are built, unit-tested, and included in the current restaged pilot.

### SOC actions and special procedures

- Trigger converted room, mobile, and object SOC actions through movement, speech,
  commands, combat, object interaction, and time or random pulses as applicable.
- Exercise the 46 retained native special bindings and the 45 adapted special bindings
  represented by 13 generated DG triggers.
- Verify command blocking, messages, movement, combat, object transfer, and pulse-driven
  behavior against the source intent.

### Converted data details

- Inspect converted mobile flags, affects, positions, races, classes, attacks, dice,
  money, and equipment positions.
- Inspect object types, wear flags, affects, applies, containers and keys, and mapped
  magic-item spells.
- Confirm high source magic-item spell levels are capped at target level 34 and that the
  source-only `mud to rock` spell is disabled rather than mis-mapped.
- Inspect high-number mobile and object affects in the restaged pilot. The current
  converter uses traced RoL identities rather than treating those source values as
  target numeric positions.

### RoL apply and affect compatibility

- Use `stat mobile` on Muspel mobiles 2058604, 2058608, 2058621, 2058706, 2058911,
  2059005, 2059012, 2059017, 2059027, or 2059028. Each must show the RoL slow-poison
  secondary affect. Apply poison and compare a positive damage tick with an otherwise
  equivalent unprotected test mobile; the protected result must be half, rounded down,
  with a minimum of one.
- Muspel mobiles 2058604, 2058911, 2059012, 2059027, and 2059028 also carry the
  converted meditation/rapid-preparation affect. Confirm it appears independently of
  slow poison in `stat mobile`.
- Use `stat object` on Cemetery objects 2055306, 2055311, or 2055312 and Theswamp
  objects 2040911, 2040915, 2040916, 2040926, or 2040927. Their source agility applies
  must be target Dexterity applies with the same signed modifier.
- Use `stat object` on representative maximum-stat conversions such as Hulburg object
  159175 or 159356 and Muspel objects 2058707, 2058751, 2058803, or 2058905. The
  converted apply must name the corresponding target attribute rather than expose an
  unknown source apply. Equip an obtainable example and confirm the displayed stat
  changes by the listed modifier.
- The docile secondary affect and bounded race-factor applies are built and
  production-tested, but the five pilot packages contain no converted `ADD` example
  suitable for manual exercise. Do not hand-edit the staged pilot to create one.

### RoL mobile-action compatibility

- Use `stat mobile` on Muspel mobile 2058809. It must show `RoL-Archer`, `Helper`, and
  `Listen`. With its ranged weapon and ammunition available, a valid player or pet in
  an adjacent non-peaceful room is a one-room ranged target; removing usable ammunition
  must prevent that shot.
- Use `stat mobile` on Muspel mobiles 2058702-2058705, 2058707-2058709, 2058711,
  2058717, 2058718, 2058721, or 2058722. Their source protector role must appear as
  `Helper` plus `Listen`; they should assist eligible allies in the same room and react
  to eligible combat in an adjacent room through the existing target behaviors.
- Inspect class-role flags independently of the mobile's primary class. Cemetery mobile
  2055315 must show `RoL-Cleric`; 2055317 must show `RoL-Mage`; 2055328 must show all
  five RoL class roles. Muspel 2059001 must show `RoL-Thief`. These flags participate in
  the matching target caster, psionic, rogue, or warrior role checks.
- Muspel mobile 2059008 and Theswamp mobile 2040928 must show
  `RoL-Aggro-Evil-Race`; Muspel mobile 2059013 and Swamp Two mobile 2026102 must show
  `RoL-Aggro-Good-Race`. In a non-peaceful room, compare a test character from the
  matching source-race alignment family with one from the opposite family and confirm
  only the matching family triggers this aggression rule.
- Muspel mobile 2058610 or 2058623 and Muspel mobile 2058906, 2058907, 2058952, or
  2059016 must show `RoL-Nice-Thief`. A failed theft from one of these awake mobiles
  must not by itself start its automatic retaliation path; unrelated aggression and
  scripted behavior remain independent.
- Stay-sector and delayed-hunter behavior are implemented and production-tested, but
  the five pilot packages contain no converted `ADD` example. Do not hand-edit the
  staged pilot to create one.

### Phase 6 automatic race procedures

- The refreshed staged world patches all six preserved Hulburg prototypes that use a
  source boot-time race procedure. `stat mobile` on 159118, 159204, 159246, 159309, or
  159341 must show `RoL-Umberhulk` and aggregate elemental protection. Mobile 159211
  must show `RoL-Demon`, infravision, and aggregate elemental protection.
- Fight one of the five flagged umber hulks using a disposable mortal character. Its
  level-scaled combat proc must periodically produce either the many-eyes confusion
  message/effect or the extra crushing-mandibles attack. This hook operates without a
  named `SpecProc` and therefore does not consume that persistent slot.
- The source claw object maps to target object 2001230, which is outside the current
  five-package stage. Automatic claw equipment therefore becomes manually testable in
  a Phase 7 batch containing that dependency; do not hand-create the object here.
- The Hulburg marilith exposes the demon flag and initialization affects, but its
  `nogate` summon templates are also outside this pilot. Test planar gating only in a
  later dependency-complete stage. The runtime and its recipe/cooldown helpers already
  have production-linked automated coverage.

### Phase 6 breath and conjured-death procedures

- Seven named breath procedures and three composition-safe conjured-death behaviors
  are implemented and production-tested. Their source consumers are outside the five
  staged pilot packages, so this bundle does not yet provide an honest manual example.
- A later dependency-complete stage should verify that `breath_attack_acid` and
  `breath_attack_lightning` strike only the current opponent every fourth combat turn,
  while the five `breath_weapon_*` procedures affect eligible room targets on the same
  cadence.
- That later stage should also kill a flagged familiar, mount, and summoned monster.
  Each must show its source-specific fade message, leave no corpse, and retain any
  independent named SpecProc or automatic-race behavior before death.

### Phase 6 converted transport procedures

- `RoL Magic Pool` and `RoL Auto Distributor` are implemented and
  production-tested, but their active source consumers are outside the five staged
  pilot packages.
- A later dependency-complete stage should enter a converted magic-pool object and
  confirm its fixed damage and remapped destination. On an auto-distributor boundary
  room, any command by a mortal must replace that command with a random transfer to a
  loaded room in the same zone; the same command by staff must proceed normally.
- Do not attach either procedure by hand to this staged pilot. Magic pools require
  authored damage and destination object values, and distributor rooms deliberately
  intercept every mortal command.

### RoL object-property compatibility

- Cast identify and use lore or greater lore on Theswamp objects 2040901, 2040903,
  2040907-2040909, 2040911-2040913, 2040915-2040917, 2040923, 2040924,
  2040926, 2040928, or 2040929. A mortal must receive the no-identify refusal; an
  immortal may inspect the item. Mass identify must report the worn item as unable to
  be identified without exposing its properties.
- Wield Theswamp object 2040916 or 2040930, Cemetery object 2055349 or 2055350, or a
  flagged Muspel weapon such as 2058657. Each must consume two hands regardless of
  size and must receive the target's two-handed combat treatment.
- Attempt to equip Cemetery objects 2055305 or 2055306 as a drow or duergar and as a
  source-good analogue such as a human or elf. The evil-race character must be refused;
  the good-race character must be allowed. Reverse the expectation for Cemetery objects
  2055312 or 2055313 and Muspel objects 2058662, 2058952, or 2058961.
- Equip Muspel whole-body armor 2058913 or 2058917. It must be refused while arm or leg
  gear is worn; after equipping it, new arm or leg gear must be refused.
- Equip Muspel helm 2058921, then have another character attempt summon and group
  summon. Both must leave the wearer in place. Removing the helm restores ordinary
  summon eligibility, subject to normal room and preference restrictions.
- Sleep protection, charm protection, and whole-head overlap support are built and
  production-tested, but none of the five current pilot packages contains an object
  with those flags. Exercise them only when a later staged package supplies a source
  example; do not hand-edit this pilot.

Phase 5 also provides converted object-trap behavior for movement, get and put, open,
and lock-pick events, including finite charges, area effects, status effects, damage,
and same-zone teleport. Staff can inspect the payload with `stat object`; players can
use `detecttrap <object>` and `disabletrap <object>`. None of the five Phase 4 pilot
packages contains an active source object trap, so test these commands only after a
later staged batch includes one; do not invent or hand-edit a trap into this pilot.

### RoL exit-trap compatibility

- Swamp Two room 2026051 contains the pilot's converted exit trap on the down exit. It
  is a reusable, area-effect falling-rock trap that deals 30-70 damage, uses source
  hardness 50 for its target DCs, and has a 75 percent chance to arm at boot.
- Use a mortal test character with Perception and Disable Device. Exit traps deliberately
  exempt NPCs and immortals, so use the staff character only to prepare the door state.
- Run `detecttrap` with no object argument in room 2026051. On a successful check, run
  `disabletrap`, again with no object argument, and confirm that opening or picking the
  down exit no longer triggers the trap for that boot.
- To test triggering on a fresh armed boot, force zone 20261 to reset, have staff unlock
  the down exit if needed, and have the mortal run `open down`. Alternatively, leave it
  locked and use `pick down`. A failed Reflex save must apply the 30-70 bludgeoning
  damage to mortal characters in the room; a successful save avoids the effect.
- The source trap is reusable: after an armed trigger, it remains present and can fire
  on a later open or pick attempt. This pilot's door reset does not carry the source
  rearm bit, so a successfully disabled pilot trap is restored by a fresh isolated boot,
  not by that zone reset. Full-corpus converted resets that do carry the bit clear
  detected, disabled, and triggered state and reroll the authored load percentage.
- If the 75 percent boot roll leaves the pilot trap unarmed, restart the disposable
  runtime instead of editing the staged world.

## What Is Not Ready Yet

- The pilot is not installed into the normal development world.
- The remaining 247 source packages have not completed conversion.
- All active symbolic families and shared Phase 5 capabilities have explicit
  dispositions. The active quest corpus uses fixed item rewards; no random item-reward
  range remains to implement.
- Flagged arena, no-precipitation, PSP-regeneration, and RoL-jail runtime support is
  built and unit-tested. The current five pilots contain no flagged arena or RoL-jail
  room, so those two behaviors cannot yet be exercised from this staged bundle.
- Phase 6 now has an exact inventory rather than treating `ACT_SPEC` as the direct
  binding count. Of 1,234 discovered candidates, 87 are source-preprocessor exclusions;
  of the 1,147 active direct bindings, 377 are resolved and 770 remain. Of 848
  `ACT_SPEC` records, 517 are resolved and 331 remain. The automatic race procedures
  are complete and the Hulburg subset is exposed above. The current five-zone pilot
  still has no selected source example from the newly shared guild, janitor, pet-shop,
  receptionist, corpse-devourer, poison-bite, thief, breath, or conjured-death families;
  it also has no selected `home_reset` room, `magic_pool` object, or `autoDistributor`
  room. Those additions remain automated evidence rather than manual-test claims for
  this bundle.
- Package-wide conversion, repair, balance review, and acceptance bundles are Phase 7.
- Development-world application and final operational documentation are Phase 8.

## Reporting a Manual Finding

Record the package, target zone, room/mobile/object/quest VNUM, command or action,
expected source behavior, observed target behavior, and relevant server log lines. Note
whether the issue reproduces after a forced zone reset. Do not repair staged files by
hand; fixes belong in the deterministic converter, runtime adapter, or explicit action
ledger so the bundle remains reproducible.
