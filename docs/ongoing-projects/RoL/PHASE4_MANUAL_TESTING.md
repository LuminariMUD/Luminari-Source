# Realms of Luminari Phase 4 Manual Testing

- Status: Phase 4 complete
- Environment: disposable development runtime only
- Staged world: `lib/rol-conversion/runs/phase5-object-applies-affects-20260812-pilot/staging/world`
- Runtime contract: `lib/rol-conversion/runs/phase5-object-applies-affects-20260812-pilot/validation/pilot-runtime-contract.json`
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

### Shops

- Find the 14 appended shops in Hulburg and the converted shop in Muspel.
- Test listing, buying, selling, opening hours, keeper messages, accepted item types,
  price multipliers, and roaming-shop behavior.

### High-level quests

- Speak to converted quest-host mobiles and test ASK and GIVE entries.
- Confirm required coins and duplicate required items are consumed exactly.
- Test output commands for items, coins, attacks, disappear behavior, doors, kits,
  churches, and spell or skill teaching where the selected quest uses them.
- Phase 5 has added runtime support for configured experience rewards, signed quest-point
  changes, argument-free attacks, and all mapped source spell or skill rewards. Those
  additions are built and unit-tested. The five-zone pilot has been restaged with the
  current converter, but a capability-complete full-corpus Phase 5 bundle has not yet
  been staged for broad manual testing.

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

## What Is Not Ready Yet

- The pilot is not installed into the normal development world.
- The remaining 247 source packages have not completed conversion.
- Mobile actions are the only remaining unmapped symbolic family in Phase 5. The active
  quest corpus uses fixed item rewards; no random item-reward range remains to
  implement.
- Flagged arena, no-precipitation, PSP-regeneration, and RoL-jail runtime support is
  built and unit-tested. The current five pilots contain no flagged arena or RoL-jail
  room, so those two behaviors cannot yet be exercised from this staged bundle.
- The remaining source special-procedure corpus is Phase 6 work.
- Package-wide conversion, repair, balance review, and acceptance bundles are Phase 7.
- Development-world application and final operational documentation are Phase 8.

## Reporting a Manual Finding

Record the package, target zone, room/mobile/object/quest VNUM, command or action,
expected source behavior, observed target behavior, and relevant server log lines. Note
whether the issue reproduces after a forced zone reset. Do not repair staged files by
hand; fixes belong in the deterministic converter, runtime adapter, or explicit action
ledger so the bundle remains reproducible.
