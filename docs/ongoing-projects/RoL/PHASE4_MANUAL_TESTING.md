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

### Phase 6 Bloodstone undead death procedure

- The composition-safe `bs_undead_die` behavior is implemented for converted Bloodstone
  mobiles 2007119, 2007162, 2007167, and 2007197. The package remains outside the five-package
  staged pilot, so test this only in a later dependency-complete Bloodstone stage.
- Kill each converted mobile. It must turn into black vapor and seep into the ground, must not
  also show Luminari's generic undead-crumble message, and must leave no corpse under the target's
  native undead policy.
- Use `stat mobile` before combat to confirm `RoL-Black-Vapor-Death`. Mobiles 2007119, 2007162,
  and 2007167 must retain their independent named Bloodstone procedure beside this flag.

### Phase 6 Bloodstone critter procedure

- `RoL Bloodstone Critter` is implemented for converted Bloodstone mobiles 2007110-2007112 and
  2007141. The package remains outside the five-package staged pilot, so test this only in a later
  dependency-complete Bloodstone stage.
- Use `stat mobile` to confirm each prototype has `MOB_SPEC` and the named procedure. While it is
  awake and idle, observe repeated activity pulses; the mobile should occasionally use the current
  `snarl` or `growl` social at the source two-in-81 cadence.
- Put the mobile to sleep or start combat and continue observing activity pulses. It must not run
  this ambient social behavior until it is both awake and idle again.

### Phase 6 converted item blockers

- `RoL Item Blocker` is implemented for converted ATD objects 2000891-2000896, which block north,
  east, south, west, up, and down respectively. The `misc_code` dependency remains outside the
  five-package staged pilot, so test this only in a later dependency-complete stage.
- Load one blocker in a room with an aggressive NPC. A mortal player and a player-controlled pet
  must be refused when moving in the configured direction; movement in other directions must
  proceed normally. A non-pet NPC and non-morphed staff character are exempt.
- Put a locked door in the configured direction and try `unlock <keyword> <direction>`. The
  blocker must refuse the attempt. An unlock aimed at another direction, a hidden or blocked
  exit, or a room with no aggressive NPC must retain normal target behavior.

### Phase 6 converted designated followers

- `RoL Designated Follower` is implemented for converted Icecrag mobiles 2097009,
  2097018-2097019, and 2097036-2097037. Their designated leaders are 2097012, 2097020,
  and 2097035 respectively. Icecrag remains outside the five-package staged pilot, so
  test this only in a later dependency-complete stage.
- Load a follower without its designated leader and observe activity pulses; it must
  remain unassigned. Load the matching leader in the same room and verify that an awake
  follower attaches, follows the leader through movement, and assists when the leader
  fights. A sleeping follower must wait until it wakes before attaching.

### Phase 6 converted fixed bodyguards

- `RoL Fixed Bodyguard` is implemented for converted Icecrag mobiles 2097040-2097042.
  Their assigned protected mobiles are 2097023, 2097029, and 2097008 respectively.
- Load each pair in the same room and attack the protected mobile. On an activity pulse,
  an awake bodyguard should attempt the target rescue mechanic. A sleeping bodyguard, a
  mismatched protected mobile, or an assigned mobile without an attacker must remain idle.

### Phase 6 converted floating pools

- `RoL Floating Pool` is implemented for converted Ethereal objects 2022706,
  2022707, 2022710, and 2022711. Ethereal remains outside the five-package staged
  pilot, so test this only in a later dependency-complete stage.
- Confirm each pool has `ITEM_AUTOPROC` and is left in a room. Over repeated object
  pulses, it should have a 12 percent chance per pulse to display departure and arrival
  messages and move through one random open north, east, south, west, up, or down exit.
- Close, hide, or block an exit, or mark its destination `ROOM_NOMOB`; the pool must
  exclude that direction. It must remain in place when no eligible exit exists and
  must receive no more than one movement roll per object pulse.

### Phase 6 converted Bloodstone portals

- `RoL Bloodstone Portal` is implemented for converted objects 2007147-2007149
  and 2022491. Their converted destinations are 2007250, 2007250, 2007109, and
  2022569 respectively. Bloodstone remains outside the five-package staged pilot,
  so test these only in a later dependency-complete stage.
- With an awake mortal, `enter portal` should select the exact portal object, move
  to its converted destination, remove 1-20 hit points and 1-30 movement points,
  floor movement at zero, and show passage and weakened messages. A staff character
  should move without either stress loss.
- Confirm an invalid or target-forbidden destination consumes the matching command
  without moving. At the low-hit boundary, a loss leaving exactly -10 hit points
  survives; a loss leaving less than -10 invokes the normal target death path.

### Phase 6 converted portal doors

- `RoL Portal Door` is implemented for converted objects 2000751-2000753 and 2000883.
  Their converted destination value is remapped from source room 3001 to 2003001 where
  authored; object 2000753 retains its source zero destination and must report as broken.
- `look in <portal>` should preview a loaded destination without moving. `enter <portal>`
  should require source level 20 for a player, preserve arena-boundary equivalence, and
  reject source-evil races when value 3 is zero or source-good races when it is nonzero.
  Target teleport-admission safety must reject private, death, staff-only, closed, or
  otherwise forbidden destinations.
- A permitted character selecting the exact object should see departure and arrival
  messages and move to the converted destination. Unrelated objects and commands must
  retain normal behavior.

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

### Phase 6 shadow-giant procedure

- `RoL Shadow Giant` is implemented and production-tested for all eight active source
  bindings, but the `abandon` package is outside the five staged pilot packages.
- In a later dependency-complete `abandon` stage, fight any of converted mobiles
  2090855, 2090856, 2090858, 2090862-2090865, or 2090869. Over repeated activity
  pulses, the face-removal message should appear at the source 1-in-21 rate. Every
  player and charmed pet in the room should take 25d8 mental damage, halved by a level
  30 Will save, and may be stunned for one to three rounds.
- Repeat with an undead or dragon character and with a converted angel, demon, or devil
  pet. Those targets should laugh off the effect and take no damage or stun. Ordinary
  non-pet NPCs in the room are never targeted.

### Phase 6 converted ship procedures

- The five `RoL Ship` procedures are implemented and production-tested for all 57 active
  bindings across seven ships, but those packages are outside the five-package staged
  pilot. Do not attach the procedures by hand: the adapter deliberately recognizes only
  the converted hulls, interiors, navigators, and routes.
- In a later dependency-complete ship stage, use `enter <hull>` on converted hull objects
  2005731, 2011100, 2011300, 2034249, 2090391, 2046610, and 2098451. Confirm boarding
  enters the associated fixed interior, respects capacity, and rejects duplicate or
  unavailable hulls.
- From an exit or lookout room, use `look out`. At a dock, use `disembark`; while sailing,
  a mortal must be refused. From a panel room, inspect `look panel` and exercise `order
  speed`, `order sail <direction> [repeat]`, `order fire`, `order ram`, and `order board`
  where the world state supplies a valid target.
- A route navigator blocks player-issued `order` commands. With the correct navigator in
  its control room, observe the original 2.5-second route cadence and the departure and
  arrival announcements. The Chionthar, Gloom, Mirar, Captain's Fancy, and Spirit Raven
  routes depart every four game hours; Realms Master and Silver Lady depart at game hour
  6 and every twelve hours thereafter.
- Attack the Realms Master or Silver Lady navigator and confirm its converted crew family
  is called to hunt the attacker. Other ship navigators protect orders but have no source
  crew-call family.

### Phase 6 converted guild guards

- `RoL Guild Guard` is implemented for all 60 active source bindings, but the affected
  packages are outside the five-package staged pilot. Do not assign it to an unrelated
  mobile: its class, race, direction, and protection rules recognize converted rooms only.
- In a later dependency-complete stage, approach a guarded entrance with an ineligible
  character and confirm the guard humiliates the character and blocks the authored
  direction. Repeat with the required class. Multiclass membership is accepted; the
  Leuthilspar elf entrance at room 2008087 accepts elves and half-elves.
- Move or transfer the guard away from its original load room and repeat. It must provide
  no gate or protection behavior until returned home. Immortals and NPC guards pass.
- At a protected entrance, attack the guard with a mortal character. The guard should
  drain up to 5,000 XP per character level without reducing XP below 2, apply the source
  dispel, curse, poison, blindness, and slow effects, reduce the attacker to 1 hit point,
  stop the combat, and relocate the attacker to another eligible room in the same zone.
- Exercise the six converted Bloodstone gates at rooms 2007669 north, 2007817 down,
  2007837 west, 2007844 east, 2007864 west, and 2007880 west. Their accepted target
  classes are Warrior/Blackguard, Cleric, Assassin/Rogue, Wizard/Sorcerer, Rogue, and
  Necromancer respectively.
- Exercise the seven converted Waterdeep gates at rooms 2002951 north, 2003055 south,
  2003067 north, 2003283 east, 2005510 east, 2005520 south, and 2005570 east. Accepted
  target classes are Assassin/Rogue, Warrior/Berserker/Blackguard, Cleric, Rogue,
  Warrior, Monk, and Wizard. A character rejected at 2002951 must be knocked to a
  sitting position.
  Accepted characters must reach the exact converted destination even when the entrance
  is closed. The guards must also retain their generated idle and fighting flavor.

### Phase 6 converted class-family guild rooms

- Forty-nine converted rooms use `RoL Mage Guild Room`, `RoL Thief Guild Room`, `RoL Warrior
  Guild Room`, `RoL Cleric Guild Room`, or `RoL Bard Guild Room`. These procedures retain
  the source family gate while delegating accepted commands to Luminari's current training
  service.
- In converted rooms 2005583, 2020963, 2034494, or 2034495, try `practice`, `train`, and
  `boosts` with a character that has no mage-family levels. Each command should be refused.
  Repeat with a Wizard, Sorcerer, Summoner, Warlock, or Necromancer level; the ordinary
  current guild response should appear.
- Repeat the same wrong-family and accepted-family checks in thief rooms 2020956 and
  2094948, warrior rooms 2020958 and 2094953, and cleric rooms 2020965 and 2094933.
  Multiclass characters pass when any class belongs to the room's family. NPC commands and
  unrelated commands retain the ordinary guild procedure behavior.
- Repeat in Bard rooms 2020957 and 2094961. A Bard level is required; source Battlechanter
  maps to the target Bard class.
- Verify the 37 exact-class source guild bindings through the same target multiclass families.
  Mage-family rooms are 2010691, 2035348, 2035362, 2046057, 2046082, and 2091723;
  thief-family rooms are 2035331, 2035341, 2081044, 2081274, 2091647, and 2091648;
  cleric-family rooms are 2010698, 2021869, 2035302, 2046062, 2046069, 2046071,
  2091707, 2091714, and 2091730; and warrior-family rooms are 2010696, 2010719,
  2021800, 2035319, 2046067, 2046073, 2046081, 2046248, 2046259, 2046298,
  2091373, 2091462, 2091695, 2091702, 2091717, and 2091726.
- The mapping preserves the source role after the target class-model migration: Conjurer,
  Elementalist, and Necromancer use the mage family; Thief and Assassin use the thief family;
  Cleric, Druid, and Shaman use the cleric family; and Warrior, Antipaladin, Mercenary, Monk,
  Paladin, and Ranger use the warrior family. Confirm an unrelated family is refused and each
  listed target family is accepted. No room procedure flag is required.

### Phase 6 converted Waterdeep guild rooms

- Twelve converted rooms use `RoL Waterdeep Guild Room`. They are outside the five-package
  staged pilot, so test them only after building a dependency-complete Waterdeep stage.
- Verify the exact-class rooms with both an ineligible and an eligible mortal: Paladin 2005505,
  Warrior 2005512 (the target mapping for source Mercenary), Monk 2005524, Bard 2005537,
  Ranger 2005544, Druid 2005568, and Rogue 2003289 and 2002956.
- Verify mage-family admission in rooms 2005581 and 2003044, cleric-family admission in room
  2003073, and warrior-family admission in room 2003061. A multiclass character passes when any
  target class matches the gate.
- In every room, an ineligible mortal's `practice`, `train`, and `boosts` command must be refused.
  An eligible mortal must receive the current target guild response. Unrelated commands and NPC
  commands retain the ordinary guild procedure behavior.

### Phase 6 converted major beholders

- `RoL Major Beholder` is implemented for all eight active source bindings. The affected
  packages remain outside the five-package staged pilot; do not assign this converter-owned
  procedure to unrelated mobiles.
- In a later dependency-complete stage, fight converted mobile 2043310, 2052365, 2080013,
  2080014, 2080018-2080020, or 2081029. Confirm the mobile has `MOB_SPEC` and can fire
  several independently selected eye rays in one combat turn.
- Across repeated turns, confirm a fired eye remains unavailable for the next two turns and
  becomes eligible again on the third. Ready eyes have independent one-in-three checks.
- Exercise the ten target-native effects: fireball, acid arrow, slow, ray of enfeeblement plus
  feeblemind, wither, room-wide dispel against players and charmed pets, prismatic spray, hold
  monster, harm, and finger of death. When its selected opponent is a charmed pet whose master
  is present, confirm the eye rays can redirect to that master.
- The source engine's all-unused-eyes weapon-critical burst is intentionally absent because the
  target special-procedure gateway provides combat-turn events, not source weapon-critical
  callbacks.

### Phase 6 converted lich energy drain

- `RoL Lich Energy Drain` is implemented for all six active source bindings. The affected
  packages remain outside the five-package staged pilot; do not assign this converter-owned
  procedure to unrelated mobiles.
- In a later dependency-complete stage, fight converted mobile 2001098, 2001104, 2009040,
  2019701, 2070603, or 2083253. Confirm it has `MOB_SPEC`, and that no drain occurs while the
  lich is casting.
- Group a second character or pet with the lich's current opponent. Across activity pulses and
  combat turns, confirm each eligible current opponent or party member receives an independent
  one-in-five check in room-list order and that the first successful target is drained.
- On a successful drain, an unwarded target must lose its current hit points plus five, the lich
  must gain the target's former current hit points even above its normal maximum, and the target
  must gain two combat rounds of cumulative stun. Blackmantle on the lich suppresses only the
  healing.
- Repeat with Death Ward on the target. It maps the source protection-from-undead spell: the
  victim stops at zero hit points instead of minus five, while the life transfer and stun remain.
  A non-party bystander must never be selected.

### Phase 6 converted undead drain family

- `RoL Undead Drain` is implemented for converted mobiles 2001256-2001262. These
  mobiles are outside the five-package staged pilot, so test them only in a later
  dependency-complete stage and do not assign the converter-owned procedure elsewhere.
- Confirm all seven mobiles have `MOB_SPEC` and the named procedure. Fight each with a
  mortal over repeated combat turns: 2001256 and 2001258 use a one-in-16 check; the
  other five use one-in-21. The target combat-turn cadence is the explicit adaptation
  for the source NPC-hit and NPC-critical callbacks, which have no target registry event.
- After a failed Will save, confirm 2001256 applies -1 armor and -5 Dexterity; 2001257
  applies -5 Strength and -1 Will; 2001258 applies -2 armor and -10 Dexterity; 2001259
  applies -2 armor, -15 Dexterity, and two ticks of slow; 2001260 applies -10 Strength
  and -1 Will; 2001261 applies -3 armor, -15 Dexterity, -15 Strength, and slow; and
  2001262 applies -10 Strength, -1 Will, and -1 Fortitude. Profile affects last two to
  three ticks except the fixed two-tick ghast slow.
- While one melee-profile effect is active, another melee-profile mobile must not add
  its drain; spell-profile effects behave the same within their separate shared group.
  One melee and one spell profile may coexist. Undead victims and targets protected by
  Death Ward must remain immune.

### Phase 6 converted trade bandits

- `RoL Trade Bandit` is implemented for all seven active source bindings. The affected
  trade package remains outside the five-package staged pilot; do not assign this
  converter-owned procedure to unrelated mobiles.
- In a later dependency-complete stage, place converted mobiles 2099501-2099507 in a
  room with `MOB_SPEC`. Carry converted `ITEM_RESOURCE` cargo, or own a room wagon whose
  value 3 is your character ID and which contains cargo. Confirm movement, `flee`, and
  `get` trigger the toll while other commands and players without at least 1,000 cargo
  cost pass normally.
- Confirm 2099501 demands 50 gold; 2099502 and 2099503 demand one third and one half of
  cargo value respectively; 2099504 demands full cargo value; and source platinum maps
  to ten target gold. Fractions are truncated before the currency conversion, matching
  the source.
- Confirm 2099505 demands all carried gold plus the owned wagon. A player with no gold
  loses the wagon immediately; a player who pays but has no owned wagon is attacked.
  Mobile 2099506 demands 100 gold from good characters, all carried gold (or 100 when
  broke) from neutral characters, and attacks evil characters. Mobile 2099507 attacks
  immediately.
- After a demand, repeated attempts to move, flee, or get are blocked, with a one-in-five
  chance that the bandit attacks. Another player is not captured by that bandit's current
  demand. Use `give <amount> gold <bandit>`; underpayment transfers the offered gold and
  starts combat, while sufficient payment makes the bandit purge its possessions and
  disappear. Variant 2099505 also takes the wagon.
- Leave a non-fighting bandit alone for ten MUD hours and confirm it disappears. If
  another character is present at that one-shot deadline, it remains and does not retry
  the cleanup, matching the source event contract.

### Phase 6 converted shaman totems

- `RoL Shaman Totem` is implemented for all 21 active object bindings, with matching
  corpse-free spirit death behavior on all 21 mobile families. These packages remain outside
  the five-package staged pilot; do not attach the procedure or compatibility flag by hand.
- With any Cleric, wield or hold a converted totem and use `use totem`; the first valid use
  permanently bonds the character and that object but does not summon. Below Cleric level 21,
  later uses must refuse the summon. A different totem or a copy of the bonded totem must refuse.
- Good spirit totems 2000716-2000725 accept non-evil source races and reject evil source races.
  Evil spirit totems 2000732-2000742 do the reverse. The check uses converted source race, not
  the character's current alignment.
- On later valid uses, confirm the prayer consumes one of three attempts even when the summon
  check fails. A successful spirit is ten Cleric levels lower, bounded to levels 1-40, gains a
  25 percent HP increase, follows and assists its owner, and prevents a second spirit summon.
- Confirm a peaceful room blocks summoning without consuming an attempt. After three attempts,
  additional uses refuse until seven MUD days after the first attempt window.
- Kill each available spirit family and confirm its animal-specific fade message appears and no
  corpse is created. The spirit mobile must retain its other converted behavior beside the
  `RoL-Totem-Spirit` compatibility flag.
- Converted Outpost mobile 2020971 owns `RoL Totem Restorer` and must retain
  `MOB_SPEC`. With a level-21-or-higher Cleric who has a saved spirit bond but no
  totem, give the restorer 10,000 target gold and say `spiritworld`. Confirm the
  exact good or evil totem selected by the saved bond appears in the Cleric's inventory, is bound
  to that character, and the restorer disappears.
- Repeat with a non-Cleric, a Cleric below level 21, no saved spirit bond, 9,999 gold, and an
  invalid saved choice. Each attempt must refuse without consuming the restorer. If the mapped
  totem prototype is unavailable, the restorer must remain and the player must receive a staff
  escalation message.

### Phase 6 converted lich rite

- Converted mobiles 2000009 and 2046990 own `RoL Lich Rite` and retain `MOB_SPEC`. Give either
  keeper converted objects 2089471 and 2046999, using one carried and one worn offering to cover
  both source locations. With an ungrouped level-30 Necromancer, enter the exact lowercase phrase
  `say immortality` and confirm the full source rite narrative appears.
- Confirm both offerings and the keeper disappear only after both offerings validate. The player
  must become the target Lich race, rebuild as a Wizard through the normal respec path, have zero
  experience and -1000 alignment, retain the target Lich size, and save the resulting state.
- Repeat while one offering is absent. Also repeat while grouped, following another character, or
  leading a follower. Every attempt must refuse without consuming either available offering or the
  keeper. An uppercase `say Immortality` must not trigger the source case-sensitive phrase.
- Repeat with a non-Necromancer and confirm the keeper attacks. Repeat below level 30 and confirm
  the keeper refuses without attacking or consuming anything. Do not perform this irreversible
  walkthrough on a persistent character that should retain its current build.

### Phase 6 converted Waterdeep town crier

- Converted northern Waterdeep mobile 2003008 owns `RoL Scheduled Mobile` and retains
  `MOB_SPEC`. Observe it while standing through repeated activity pulses. Confirm its source
  2d42 table produces both single and ordered multi-line room actions, speech, and zone-wide
  shouts, while rolls 43-84 remain silent.
- Observe the hour changes around 3-5 and 9-10. Confirm the Moonshae warning fires once at hour 3,
  its shared ship gate resets at hour 4, and the Calimport warning fires once at hour 10 after the
  hour-9 reset. Confirm the shop-opening warning fires once at hour 5. The source shared shop gate
  normally suppresses the hour-18 closing warning until its hour-19 reset; a crier first loaded at
  hour 18 may issue that warning once.
- Trigger ambient outcome 39 and confirm the two welcome shouts are followed by the housewife
  response only for connected players in outdoor rooms of the crier's current zone. Attack the
  crier and confirm each activity pulse retains the source help shout and outdoor city-cheering
  response in addition to the ordinary ambient and scheduled processing.

### Phase 6 converted Sister Knights

- `RoL Sister Knight` is implemented for all five active source bindings in the
  Moonshae package. The affected mobiles are 2026218-2026222; each must retain
  `MOB_SPEC`. Do not assign this converter-owned procedure to unrelated mobiles.
- Place at least two of the converted Sisters in different connected rooms in the same
  zone, no more than 100 rooms apart. Attack one. On its next activity pulse or combat
  turn, confirm it shouts `Come, my sisters` across the zone and each awake, idle Sister
  begins pursuing the attacker.
- Confirm a Sister already fighting, already hunting, charmed, unable to damage the
  attacker under shopkeeper protection, beyond 100 rooms, unreachable, or in another
  zone does not answer. The attacked Sister and its attacker must not enlist themselves.
- Let the original fight continue and confirm it does not shout repeatedly. End combat,
  allow an activity pulse to reset the encounter guard, and attack again; one new alert
  should occur.
- Repeat while the caller is in a soundproof room, silenced, paralyzed, asleep, or
  casting. No shout or pursuit should begin until the suppressing condition is gone.

### Phase 6 converted alert callers

- Build dependency-complete stages for callers 2019920, 2019921, 2024440, 2025406,
  2025409, 2059810, 2059830, 2062401, 2062402, 2062405, and 2062406. Confirm the nine
  callers without an existing direct
  procedure use `RoL Alert Caller`; 2024440 retains `breath_weapon_lightning`, and
  2025406 retains `breath_weapon_fire` while both still alert.
- Attack each caller and confirm one zone-wide, source-specific shout. Eligible helpers
  are 2019830/2019850/2019880 for either Demogorgon caller, 2024410/2024415/2024420/2024450
  for Yancbin, 2025402/2025404/2025405/2025408 for Imix, 2025410/2025405/2025404 for the
  Imix pet, 2059812/2059815/2059814 for Drisinil, and 2059832/2059833/2059834 for Tukra.
  Elemental Tower helpers are 2062421/2062444/2062433 for Xzix,
  2062422/2062442/2062434 for Drgun, 2062420/2062443/2062432 for Limj, and
  2062423/2062441/2062335 for Duyrn.
- Confirm helpers must be awake, idle, uncharmed, in the same zone, reachable within 100
  rooms, and able to damage the attacker. End combat and allow an activity pulse before
  attacking again; the caller should shout once in the new fight.
- Repeat with the caller in a soundproof room, silenced, paralyzed, asleep, or casting.
  No alert or pursuit should begin until the suppressing condition is gone.

### Phase 6 converted Yggdrasil branches

- Fight mobile 2062800, 2062801, 2062802, 2062803, or 2062804 with `MOB_SPEC` set and
  `RoL Yggdrasil Branch` selected. Across repeated turns, confirm roughly half the turns
  attempt no entangle and that attempts can select the current opponent or a more
  vulnerable player in the opponent's group.
- A successful Reflex save at the source -10 modifier reports an escape and applies no
  effect. A failed save applies entangle, prevents a second simultaneous branch effect,
  and releases after four to twelve combat rounds.
- On timed release, confirm the entangle is removed, the release message appears, and the
  target's current movement points are halved using integer truncation.

### Phase 6 converted command sentinels and Foggy Woods warnings

- Build dependency-complete stages containing the four mobile-owned passage sentinels,
  both room-owned command wards, and the three Foggy Woods warning rooms. Confirm the
  six native bindings use `RoL Command Sentinel`; mobile owners also require `MOB_SPEC`.
- In target room 2001483, repeatedly attempt west past stone golem 2001438 as a mortal.
  Roughly 80 percent of attempts should be blocked. Staff should always pass.
- In target room 2010320, attempt south past Splitshield guard 2010301 as a Half-Orc
  and as another race. The Half-Orc should pass and the other mortal should be blocked.
  In room 2010302, shady man 2010302 should block south only above level 20. Staff pass
  both guards.
- In target room 2081596, attempt south past Ancient One 2081508 with a source-good race
  at levels 10 and 11, then with a source-evil race above level 10. Only the level-11
  source-good character should be blocked; staff should pass.
- In cage room 2000001, confirm a mortal may use SAY (including the apostrophe alias),
  PETITION, PROJECT, and HELP, while another ordinary command is blocked. Staff commands
  should remain unrestricted.
- In glyph room 2046990, confirm a character with any Necromancer class level can move
  down after the tingle message. A non-Necromancer should be knocked back and remain in
  the room at no less than one hit point. The source behavior deals one hit point
  normally but 25 while Minor Globe or Globe of Invulnerability is active.
- Enter target rooms 2090107, 2090112, and 2090114. Each should deliver the same complete
  Foggy Woods barbarian warning through one shared entry trigger; unrelated rooms should
  not deliver it.

### Phase 6 converted toll and ticket keepers

- Build dependency-complete stages for the nine converted keeper mobiles. Confirm all
  use `RoL Toll Keeper` with `MOB_SPEC`; the duplicate Bloodstone bouncer assignment
  still produces one prototype behavior.
- At Bloodstone tax knight 2007210 in room 2007680, NORTH is blocked until the player
  gives 20 target gold, then moves the player to 2007681. A non-gold GIVE is rejected
  before transfer. At bouncer 2007335 in room 2007431, 10 gold permits SOUTH to 2007432.
  At Ghore keeper 2011542 in room 2011666, 500 gold permits UP to 2011667. Underpayments
  remain with the keeper as in the source; NPCs pass the guarded direction directly.
- Give 5 gold to bridge troll 2001919 in room 2001863 and troll 2014202 in room 2014237.
  The actor should be thrown sitting to one of 2001862/2001864 or 2014236/2014238,
  respectively, according to source room-list ordering. An underpayment is retained and
  refused; a GIVE that transfers no gold is silently consumed.
- At ticket takers 2011106/2011306, ENTER the matching ship 2011100/2011300 while carrying
  ticket 2005341, or after giving it to the taker. At 2098357/2098358, use ticket 2000046
  to ENTER ship 2098451. One ticket is destroyed and boarding continues. The matching
  ship is blocked without a ticket, while ENTER for an unrelated room object is ignored.

### Phase 6 converted travel portals

- Build dependency-complete stages containing converted objects 2000882, 2003088,
  2005515-2005516, 2008112-2008113, 2021500-2021501, and 2041941. Confirm all nine use
  `RoL Travel Portal`; object values 0-3 are remapped only where the source handler uses
  them as destination rooms.
- At dimensional fold 2000882, `look in fold` should preview room 2003001 without moving.
  `enter fold` should move there only when the destination passes target teleport admission
  and has the same arena state as the origin. An unrelated entered object is ignored.
- Enter Waterdeep portals 2005515 and 2005516. They should move to rooms 2003044 and
  2005581. Their active value-one damage is zero; temporarily test a staged copy with
  positive value one to confirm mortal hit points stop at zero and staff take no damage.
- Enter fountain 2003088 as a character with a Wizard class level and as a character
  without one. Only the Wizard, which represents the converted source Illusionist, should
  move to room 2005582.
- Enter elfgates 2008112 and 2008113 as a target Elf at levels 19 and 20, then as another
  race at level 20. Only the level-20 Elf should pass. Repeated entries should use the four
  remapped destination slots; the active objects currently repeat 2012805 or 2008001 in
  all four slots.
- Carry mushroom spores 2021500 or 2021501 and `use spores`. A Cleric should move to
  2021660 or 2021550 and consume the spores. A non-Cleric should remain in place, consume
  the spores, and be stunned for about 60 seconds. Zero charges, an invalid destination,
  or an arena-boundary mismatch should produce no movement or consumption.
- `enter circle` through Blip portal 2041941 from inventory, equipment, and a room. Each
  successful trip should move to 2041914 and give badge 2041900. The active infinite
  charge must not decrease or destroy the portal; an unrelated entered object is ignored.

### Phase 6 reconciled artifact identities

- Confirm the conversion identity map uses the existing artifacts rather than creating
  offset duplicates: Trorxek 1043 -> 169901, Amaukekel 1044 -> 169902, Fade 1042 ->
  169903, Henekar 1046 -> 169904, Doombringer 1050 -> 169905, both Kelrarin variants
  1007/1009 -> 169906, Kelrom 1048 -> 169907, Gesen 5343 -> 169908, Tiamat's Stinger
  1008 -> 169909, and Avernus 19730 -> 169910.
- Run `artifact list`, then `artifact info <item>` for each mapped artifact. The canonical
  target system should report its ownership rule, class oath where applicable, passive
  powers, called phrases, active ability, and signature combat behavior. No converted
  `2001007`-style artifact prototype or second named SpecProc should be staged.
- Exercise representative source contracts through the modern system: say a listed
  Trorxek, Amaukekel, Fade, Henekar, or Doombringer phrase while holding or wearing the
  item; use `soulstrike`, `divineward`, or `doomblast` when the matching artifact grants
  it; and fight eligible NPCs with Fade, Doombringer, Kelrarin, Kelrom, Gesen, Tiamat's
  Stinger, and Avernus. Use `artifact info` for the exact target odds and eligibility.
- When source Raven earring 1045 is staged as 2001045 in Phase 7, confirm it has no
  special procedure. Its source `NeverLooseItem` callback exposed unrestricted debug
  commands and is intentionally excluded; saying its teleport, currency, stat, death,
  healing, resurrection, invisibility, or unlock words must have no special effect.

### Phase 6 converted banana and isolated god toys

- In a dependency-complete stage, confirm fruit 2001235 and peel 2001234 both persist
  `RoL Banana`. Carry fruit 2001235 while hungry and use `eat banana`. The fruit should
  disappear, its value-zero food amount should increase hunger saturation, one combat
  round of command delay should apply, and peel 2001234 should appear in the room.
- Leave the peel undisturbed. Its target decay flag and eight 75-second object ticks
  should remove it after ten real minutes. Eating while already above 20 hunger
  saturation should be blocked without consuming the fruit or creating a peel.
- Walk a mortal repeatedly across a room containing peel 2001234. Intelligence rolls
  above four should avoid it. Failed Intelligence followed by Dexterity 1 should stop
  movement, remove up to 15 hit points without going below one, end non-aggressive
  combat, and apply four to six MUD ticks of sleep. Dexterity 2-5 should stop movement,
  deal that roll in bounded damage, sit the character, and apply one combat-round wait.
  Dexterity 6-10 should stop movement with a one-round wait; higher rolls should allow
  movement after the recovery message.
- Repeat with a staff character, a mounted mortal, a flying mortal, and a levitating
  mortal. Each should ignore the peel. Sleeping characters should not trigger it.
- Confirm converted god-toy objects 2000005, 2000006, 2000008, 2000009, 2000013,
  2000017, 2000021-2000025, 2000028, 2000030, 2000044-2000045, 2000050, and 2001025
  have no special procedure. Their source teleport, remote-room relinking, shutdown,
  reset, forced-death, character-deletion, and zone-wide destruction commands must have
  no special effect.

### Phase 6 converted death-event profiles

- Kill converted mobiles 2000202, 2000902, 2000903, 2000905-2000909, 2001250-2001253,
  and 2003050-2003053 in dependency-complete stages. Each must show its tentacle,
  treant, phantom-steed, dark-shade, mephit, or elemental source-family death message
  and create no corpse.
- Exercise the 20 expanded profiles in dependency-complete stages:

  - Balor 196030 explodes, blinds darkvision or infravision users in a dark room, deals
    150 damage through elemental protection or 250 otherwise, and leaves no corpse.
  - Shadow demon 2000200 adds darkness when it melts into the shadows. Unseen servant
    2000499 returns gold, equipment, and inventory to its master, or drops them in the
    room if it has no master. Neither leaves a corpse.
  - Stone creature 2001433 moves carried and equipped objects into stone pile 2001438
    and retains its source ordinary-corpse path. Spore balls 2012022-2012023 poison
    other occupants unless the room is peaceful and also retain ordinary corpses.
  - Halruaa transmuters 2053268 and 2053269 replace themselves with 2053269 and
    2053270, transfer equipment and inventory, apply the target-equivalent permanent
    buff package, and leave no corpse. Form 2053270 drops eye object 2053254 and keeps
    the ordinary corpse. Forms 2053268-2053269 must retarget a visible cleric secondary
    attacker when more than one attacker is present.
  - Fleshdoll 2053362 and Menden mobiles 2088812-2088815 emit their authored death
    messages and keep ordinary corpses.
  - Pure bloods 2090812, 2090819, 2090837, and 2090866 replace themselves with
    2090914-2090917 respectively, transferring equipment, inventory, and mobile memory
    without leaving a corpse. Ice malice 2097003 likewise replaces itself with 2097056,
    transfers equipment and inventory, leaves no corpse, and retains cleric retargeting.
  - Black pudding 2092613 leaves no corpse and does not split. This deliberately
    preserves the bound source branch's malformed real-mobile comparison rather than
    inventing the intended split behavior.
  - Darkhold fire, air, water, and earth elementals 2094501-2094504 emit their authored
    crumbling messages and drop objects 2094508-2094511 respectively: ruby, diamond,
    aquamarine, and golden nugget. Each retains the ordinary corpse path.
- Confirm these VNUM-owned profiles coexist with any other converted direct or automatic
  behavior on the same mobile and do not require a new mobile flag or second persisted
  SpecProc. An unrelated mobile must retain the ordinary target death/corpse path.

### Phase 6 converted Seelie faerie profiles

- Build a dependency-complete Seelie Court stage containing mobiles 2062701-2062708,
  2062710-2062717, and 2062721-2062722. Confirm all 18 use `RoL Monster Combat` with
  `MOB_SPEC`; mobile 2062709 is not part of this family.
- Confirm the exact capability combinations shown by `medit`: 2062701, 2062703-2062705,
  2062707, and 2062712-2062713 have prism, faerie fire, and search; 2062706, 2062711,
  and 2062717 have prism and search; 2062708, 2062714, and 2062721 have prism and faerie
  fire; 2062702, 2062710, and 2062715-2062716 have search only; 2062722 has faerie fire only.
- Fight a prism-capable faerie and observe multiple activity pulses. About one pulse in three
  should form a rainbow. Each eligible PC or pet receives independently one or two distinct
  colored beams: red, orange, and yellow deal 420, 280, and 140 illusion damage before a Will
  save halves the result; blue, indigo, green, violet, and azure invoke hold monster,
  feeblemind, poison, dispel magic, and blindness through target-native spell safety.
- Stun or knock down a prism-capable faerie while it remains in combat. Its special procedure
  must still run; on an eligible prism pulse it has the authored 22 percent chance to recover to
  standing and fire, otherwise it reports that it cannot gather its strength. Unrelated disabled
  mobiles must remain blocked by the ordinary activity gate.
- Fight a faerie-fire-capable mobile. About one eligible pulse in six should outline every valid
  PC or pet target in purplish flames for duration three, apply target AC modifier -2, and remove
  invisibility or hiding. The caster must then respect the dedicated three-MUD-day recovery
  event before using faerie fire again.
- Hide a mortal or pet as the first eligible room target near a search-capable mobile. Search
  must reveal it, interrupt casting, move it to reclining, and apply a safe three-round stun;
  illusionist 2062707 uses six rounds. The source event is consumed after the first eligible
  target, even when that target is not hidden.

### Phase 6 converted Hive manscorpion venom profiles

- Build a dependency-complete Hive stage containing light-venom mobiles 2043703, 2043728,
  2043744, 2043746, and 2043761; medium-venom mobiles 2043702, 2043745, 2043759, and
  2043780; heavy-venom mobiles 2043756, 2043758, 2043768-2043770, and 2043778; and king
  2043767. Confirm all 16 use `RoL Monster Combat` with `MOB_SPEC` and identify with the
  matching venom profile.
- Repeatedly land successful attacks with each tier. Light, medium, heavy, and king profiles
  should trigger about once per 31, 7, 11, and 25 qualifying hits respectively. On a trigger,
  confirm the venom chooses a random mortal PC in the room rather than always choosing the
  character struck by the attack. Pets and NPCs must not be selected.
- For light, medium, and heavy venom, confirm target-native poison immunity prevents the affect
  and a successful Fortitude save dissolves it. A failed save applies -2 Constitution for six,
  four, or two ticks respectively. A target already carrying manscorpion venom must not receive
  a second copy or have its existing duration replaced.
- Without RoL slow poison, allow king venom to trigger and confirm the selected mortal dies
  immediately through the source lethal branch. With RoL slow poison active, confirm the king
  instead follows the ordinary poison-immunity and Fortitude-save path and applies the same
  nonstacking -2 Constitution affect for one tick on a failed save.
- When king venom kills its selected target, including when that target was the character
  struck, confirm the remaining hit sequence stops cleanly: no later critical-hit, artifact, or
  weapon riders may access the extracted character. Unrelated mobiles must receive no
  successful-hit callback.

### Phase 6 converted successful-hit area profiles

- Build dependency-complete stages containing Dobluth mobiles 2021786 and 2021820, Hive
  sandstorm beast 2043705, and Greycloak mobiles 2096631, 2096670, and 2096672. Confirm all six
  use `RoL Monster Combat` with `MOB_SPEC` and identify with the matching successful-hit profile.
- Repeatedly land successful attacks with Dobluth bladestorm 2021786. About one qualifying hit
  in five should animate the room's primary, two-handed, and offhand weapons. The one aggregate
  payload is the sum of each weapon's maximum damage and applies independently to every safe
  area target; a successful Reflex save halves only that target's slashing damage. With no such
  weapons in the room, the proc must produce no effect.
- Test Dobluth banshee 2021820 over repeated successful hits. About one hit in four should deal
  150 sound damage to each safe area target, halved by a Will save. Survivors that fail a
  separate Fortitude save should be safely stunned for two to four rounds.
- Test Hive sandstorm beast 2043705 over repeated successful hits. About one hit in 16 should
  deal 10d10 earth damage without a save to each safe area target. Each eligible target has an
  independent 50 percent chance to become blind for three ticks; blindness immunity or another
  native blindness restriction must prevent the affect without preventing the damage.
- Test Greycloak banshee 2096631 and Urgutha Forka 2096670. About one hit in six from the
  banshee should deal 200 plus or minus 10 sound damage, halved by a Will save. A soundproof room
  or silenced banshee must suppress the wail. About one hit in 11 from Urgutha should deal 300
  plus or minus 10 poison damage, also halved by a Will save; target poison defenses apply
  through the native damage path.
- Fight Aralesh Tandar 2096672 and observe about one execution in 11 qualifying hits. The
  blazing-eye branch must kill its current opponent. When that opponent is a PC-owned pet and
  the owner remains in the same room, it must also kill the owner; an owner outside the room is
  not affected. If either victim is the character struck by the outer hit, confirm no later
  critical, artifact, or weapon rider accesses the extracted character.
- For every area profile, place grouped allies, unrelated NPCs, PCs, and pets in the room and
  confirm target-native area safety chooses only eligible opponents. Unprofiled mobiles must
  receive no successful-hit behavior.

### Phase 6 converted Hive Skriaxit sandstorm profiles

- Build a dependency-complete Hive stage containing converted Skriaxits 2043741 and 2043742.
  Confirm both use `RoL Monster Combat` with `MOB_SPEC` and identify with the three-round
  scheduled sandstorm profile. Unprofiled mobiles must receive no such scheduled behavior.
- Observe at least six mobile activity pulses while a Skriaxit is idle, then while it is
  fighting. A violent sandstorm must appear exactly every third pulse in both states. Repeat
  while the Skriaxit is stunned or otherwise disabled; its source timer behavior must continue
  even though ordinary disabled mobiles do not run activity procedures.
- Put eligible mortal players or PC-owned pets in the Skriaxit's room and in populated rooms
  through open north, east, south, west, up, and down exits. One firing must reach all of those
  rooms. Close an exit and confirm the room beyond is excluded; a room reachable only through a
  second step or a non-orthogonal connection must also be excluded.
- Put the Skriaxit in a peaceful room and confirm a due firing is suppressed. Put targets in a
  peaceful adjacent room and confirm that room receives no effect. Native area safety must also
  exclude protected allies and other invalid targets.
- Compare a mortal player, a PC-owned pet, an unrelated NPC, an incorporeal target, and air and
  earth elementals. Only the eligible mortal and pet may receive the dispel attempt; the others
  retain the source immunity or other-NPC exclusion.
- Give an eligible target several ordinary spell affects. Without spell resistance, each affect
  receives a level-48 target-native Will save in list order; the storm removes at most the first
  affect that fails and emits its normal wear-off message. Successful spell resistance prevents
  the whole dispel attempt. Repeated firings may remove later affects independently.
- Record hit points before and after several firings, including with both Skriaxits present. Hit
  points must not change. The bound source room loop resets its counted Skriaxits before
  evaluating `3 * num`, so zero direct damage is required source fidelity rather than a missing
  target damage implementation.

These profiles are production-tested and reconciled, but the current five-zone pilot does not
contain their Hive package. Exercise them only after a Phase 7 stage supplies the converted
mobiles, rooms, exits, and spell-bearing targets; do not hand-edit them into the pilot.

### Phase 6 converted planar death, burst, and Balor weapon profiles

- Build dependency-complete planar stages containing Manes 2000214; Balors 2000207 and
  2093204; Vrocks 2000221, 2093209, and 2093210; Spinagons 2000233, 2032632, 2032645,
  2032646, and 2033020; and Balor weapons 2093227 and 2093228. Confirm the mobiles use
  `RoL Monster Combat`, the weapons use `RoL Weapon Proc`, and unrelated identities do not
  receive these profiles.
- Kill Manes 2000214 with eligible mortal players, unrelated NPCs, demons, allies, and protected
  targets in the room. It must leave its ordinary corpse and emit an acidic-vapor burst. Safe
  non-demon area targets that fail a Fortitude save take 4d6 acid damage; demon NPCs and targets
  rejected by native area safety take none.
- Observe Balors 2000207 and 2093204 through mobile activity. Each must retain permanent
  elemental protection and fill an empty primary slot with sword 2093228 and an empty offhand
  slot with whip 2093227 when those prototypes are staged. Existing weapons are not replaced.
  On death, abyss-forged weapons dissolve first and the ordinary NPC corpse is suppressed.
- Give either Balor weapon to a non-demon NPC or player, both carried and worn. Its automatic
  pulse must make it erupt and vanish safely. A demon NPC may retain it; a demon pet may carry
  or wear it but must not trigger its combat proc.
- Strike an eligible opponent with whip 2093227. Every qualifying successful hit deals an
  additional 8d6 force damage with no save, preserving the source's unavoidable magical-fire
  intent through the target's non-elemental damage path.
- Score repeated critical hits with sword 2093228. Each qualifying critical deals 20d10
  negative-energy damage. About one in five criticals must produce a native-safe room burst
  against eligible PCs and player-owned pets. Other criticals apply four Constitution and four
  Strength penalties lasting 2, 4, 6, and 8 ticks before damage; while the marker remains,
  later direct criticals deal damage without stacking another penalty package.
- Reduce each Vrock below or equal to 15 percent health and land successful hits. If the Vrock
  is neither silenced nor in a soundproof room, its screech checks mortal PCs against raw
  Constitution and stuns failures for one combat round. It cannot screech again for one MUD day.
  Its separate spore branch fires on about five of six eligible hits, deals 10d2 poison damage
  without a save to its current opponent, and cools down for three combat rounds. A screech and
  spore cloud may occur on the same hit because their cooldowns are independent.
- Land repeated hits with each Spinagon. About five of six ready hits launch 2-5 spikes at safe
  area targets, then start a three-round cooldown. Each target receives a Reflex save with a +4
  bonus from elemental protection; a failed save takes the source code's actual 20d2 fire
  damage. Do not substitute the contradictory source comment's 2d20 roll.
- Observe Chasmes 2000210 and 2093203 through repeated activity pulses. No buzz-induced sleep
  should occur: the active source callback tests the Chasme owner's demon race rather than the
  victim, so its sleep branch cannot run for either automatically demon-bound prototype.

These profiles are automated and reconciled but are absent from the current five-zone pilot.
Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit them
into the pilot.

### Phase 6 converted planar capture, charm, and Vrock dance profiles

- Build dependency-complete planar stages containing Glabrezus 2000212 and 2093205, Mariliths
  2000215 and 2093206, Succubi 2000220 and 2093202, and at least five instances drawn from
  Vrocks 2000221, 2093209, and 2093210. Confirm each uses `RoL Monster Combat`; unrelated
  identities must not receive these profiles.
- Land repeated qualifying hits with each Glabrezu and Marilith. About one in 11 attempts must
  test the current opponent's raw Dexterity against the source strict-less-than half-stat rule.
  A failed evasion then compares a 1-85 roll to raw Constitution: a higher roll kills the target,
  while a surviving target becomes a charmed follower held by pincers or tail.
- While held, try movement, combat, inventory, and other commands. They must be blocked. The
  source whitelist remains available: score, tell, shout, look, help, who, weather, save, quit,
  time, toggle, ooc, commands, attributes, and petition. When the captor is no longer fighting,
  its next activity pulse must begin ordinary combat with the captive. If it is already fighting
  or the captive is gone, the one-captive state clears safely.
- Put eligible male mortal players with each Succubus and observe idle activity. Charm attempts
  occur on about one in four eligible scans. Mind blank, no-charm equipment, native charm
  immunity, spell resistance, or the target-native Will save at the source -2 modifier must
  prevent control. A failed defense makes the player a charmed follower and schedules the
  lethal kiss one to four MUD hours later.
- Put a higher-level male Blackguard in the room. An unbound Succubus must recognize the target
  equivalent of the source Antipaladin and become his charmed follower instead. A bound
  Succubus must not create a follow loop with her master.
- Confirm Succubus captives receive the same command whitelist and hazy-thought restriction.
  When the kiss deadline arrives out of combat, the first captive dies. A deadline reached
  during combat moves one MUD hour later; additional captives are handled one at a time.
- Start five eligible fighting Vrocks in one room and land a hit. The whole eligible cohort must
  enter the dance, suppress ordinary Vrock burst handling while dancing, and advance through
  two escalating chants and an explosion at intervals of one violence pulse. Disabled dancers
  must still advance because the source used timer events.
- Remove a dancer from combat before a stage. With fewer than five active dancers, the dance
  must abort and clear every remaining cohort member. Kill or move the timer leader and confirm
  another dancer takes over. This intentionally repairs the source's wrong-variable defect,
  which otherwise leaves peer Vrocks permanently marked as dancing.
- Let the dance complete. Roll one shared 20d10 lightning amount for eligible mortal PCs in the
  room; each target receives a target-native Reflex save for half and native area safety excludes
  protected targets. Every dancer then enters a one-MUD-day cooldown and may resume its ordinary
  screech and spore paths but cannot start another dance until the cooldown expires.

These profiles are automated and reconciled but are absent from the current five-zone pilot.
Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit them
into the pilot.

### Phase 6 converted Avernus devil combat profiles

- Build a dependency-complete Avernus stage containing dragon 2032622 and helper 2036180;
  Barbazu 2032629, 2032640-2032644, 2033000-2033001, 2033004, 2033008-2033009,
  2033011, and 2033021-2033022; Gelugons 2033015-2033016; Barbazu glaives 2032602 and
  2033001; and Gelugon spear 2033012. Confirm the dragon composes `RoL Guild Guard` with
  its alert profile, the Barbazu and Gelugons use `RoL Monster Combat`, and the three weapons
  use `RoL Weapon Proc`. Unrelated identities must not receive these profiles.
- Have dragon 2032622 land a successful hit. It must send the Tiamat defense call to its zone
  once per fight and make helper 2036180 hunt the current opponent when within 30 rooms. A
  helper outside that source distance must not answer. End combat and allow an activity pulse,
  then confirm a later fight can trigger a new call.
- Strike each Barbazu repeatedly. About one in 20 received hits must apply a five-tick rage that
  adds its current hitroll and damroll again and immediately adds half its maximum hit points.
  The dedicated rage marker must prevent stacking until it expires.
- Score critical hits on mortal PCs with each Barbazu glaive wielded by a non-pet NPC. Every
  critical attaches an independent blood-loss event; stacked wounds each remove 40 hit points
  every three violence pulses and stop at -9 rather than killing the PC. A wound remains
  attached while the victim is at or below -5 and resumes after healing. Immortals take no
  blood loss. A critical against an NPC applies the source's immediate 100-point wound instead.
- Have Gelugons 2033015 and 2033016 land repeated hits. The active source code fires the tail
  freeze about one in seven times despite its one-in-ten comment. Native paralysis immunity or
  a successful Fortitude save prevents it; a failure applies one to two ticks of paralysis.
- Keep eligible mortal Wizards, Clerics, and Bards with Meritos 2033015 during combat. About one
  in four successful hits must select the first unsilenced eligible caster in the room. Native
  resistance or a Will save with the source +5 modifier prevents the bolt; a failure applies
  four ticks of silence. An empty eligible set must remain safe.
- Attempt `disarm` in Hanariel's room as a mortal. Hanariel 2033016 must consume the command,
  trip the actor to sitting, and impose a three-round wait. Staff commands remain exempt.
- Land repeated hits with Gelugon spear 2033012. About one in three hits against a target not
  already slowed must test a native Fortitude save; failure applies slow for 2d4 ticks. On
  automatic pulses, a non-pet devil NPC or staff owner may retain the spear. An invalid mortal,
  pet, or non-devil owner is burned for 5-50 fire damage and the weapon's damage dice become
  1d1, preserving the source bad-owner penalty.

These profiles are automated and reconciled but are absent from the current five-zone pilot.
Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit them
into the pilot.

### Phase 6 converted Avernus lifecycle profiles

- Build a dependency-complete Avernus stage containing mobiles 2032623, 2032641, 2032643,
  2032654, 2032659, 2032660, 2033000, 2033003, 2033005, 2033008, 2033014, 2033020,
  2033021, 2033026, and 2033027; objects 2032631, 2033011, 2033021, and 2033025; and
  garden rooms 2032672-2032687. Confirm the mobiles compose `RoL Monster Combat`, the four
  objects use `RoL Avernus Object`, and room 2032672 uses `RoL Avernus Garden`. Unrelated
  identities must not receive these profiles. Object 2033006 must have no native binding because
  its source callback never parses an event and cannot register its unload handler.
- Kill man 2032623. Kri'ik 2032624 must replace him in the same room and inherit his carried and
  equipped possessions. Kill Erinyes 2033003 and confirm the apparent chamber melts into its
  ruined description; after the Erinyes lifecycle is restored, its next activity must restore
  the saved authored description without retaining a dangling string.
- Observe patrols 2032641, 2032643, 2033000, 2033008, and 2033021 while idle and uncharmed. The
  two ring and two citadel patrols must follow their authored forward or reverse routes, recover
  toward their load room when displaced, and stop for an eligible citadel intruder. The prison
  patrol must keep its door and direction state independently per mobile and close and lock the
  authored doors behind it. Fighting or pet patrols must not move.
- Reveal hidden Rogues 2032654, 2032659, and 2033020, then leave them idle; they must use the
  native hide command again. Lead prisoner 2032660 away from its load room and back to Coidon
  2032606; its leader must receive object 2032649 and the prisoner must depart only after the
  meeting. Missing reward data must log an error without dereferencing a null object.
- Place good and evil PCs with deva 2033005 and observe repeated activity pulses. Its two-in-three
  echo attempt must deliver the alignment-appropriate source message only to each PC. Neutral
  characters and NPCs receive no echo.
- Damage eligible non-pet NPCs beside black altar 2033026. About one in three activity pulses
  must heal each by up to 1,000 hit points and remove blindness. The altar's death must suppress
  an ordinary corpse.
- Exercise Bel 2033014 in room 2033073. He must maintain long regeneration and bless effects,
  destroy pets in his room, close and lock the south door after combat begins, and consume a
  mortal `shieldpunch` attempt with one round of wait. A lethal blow while guard 2033019 or
  2033020 survives must sacrifice a guard, replace Bel at full health with his possessions, and
  transfer the remaining guards to the replacement.
- Hold rod 2032631 and shout the exact phrase `Kri'ik` in room 2092338. It must create Kri'ik,
  green orb 2032633, and flying Cornugon follower 2032661, start Kri'ik against the caller, and
  destroy the rod. A mortal holding it elsewhere must see it return home. An unheld rod, an NPC,
  or a different phrase must not trigger the bargain.
- Give Bel's flaming sword 2033011 to Bel, staff, a mortal, a pet, and a non-pet devil. While Bel
  exists, it must return to him and kill a mortal or pet holder; without Bel, an unauthorized
  owner must take 5-50 fire damage and the sword must degrade to 1d1. About one in six valid hits
  must emit the 50d8+1d50 fire corona, apply a native Reflex save for half, respect area safety,
  and ignite surviving eligible targets for three ticks.
- Score repeated hits with dancing dagger 2033021 or 2033025 while wielded by a level-46-or-higher
  character. About one in ten hits must hide the object and create helper 2033027 with weapon
  2033033, the owner's level and damage bonus, and the current opponent. The helper must ignore
  mortal `order`, join its owner's fight, and have a one-in-five chance each activity pulse to
  return the exact originating dagger. Lost, separated, or idle helpers must also return it;
  simultaneous dagger identities must not satisfy each other's ownership check. Staff saying
  `darkness to light` while carrying either dagger must invoke continual light.
- In rooms 2032672-2032687, confirm the one authored room scheduler scans the whole garden without
  a world-wide special-procedure scan. Mortal combatants that fail the level-30 Will save at -3
  must calm unless raging. Idle PCs and pets that fail must sleep for five ticks unless an escape
  pool 2032613-2032616 is present or native no-sleep protection applies. Staff remain exempt.

These profiles are automated and reconciled but are absent from the current five-zone pilot.
Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit them
into the pilot.

### Phase 6 converted Darkhold special profiles

- Build a dependency-complete Darkhold stage containing musical skulls and passage gems
  2094501-2094511, bastard sword 2094566, warhammer 2094571, shadow fiend 2094505, and shadow
  dragon 2094506. Confirm the skulls and gems use `RoL Darkhold Object`, both weapons use `RoL
  Weapon Proc`, and both mobiles use `RoL Monster Combat` with `MOB_SPEC`. Unrelated identities
  must not receive these profiles.
- While awake, push summon skulls 2094501-2094503 and 2094505-2094507 by name from inventory,
  equipment, and the ground. Each successful push must create mobile 2094500 in the actor's room,
  emit the note and coalescing-mist messages, and retain the skull. Sleeping actors and unrelated
  object names must not trigger the summon.
- Put passage skull 2094504 on the ground and push it by name. It must remove `EX_BLOCKED` from
  the north exit of room 2094666 and announce the revealed passage there. A repeated push must
  say that nothing happens, and a carried skull must not open the passage.
- Attempt to drop ruby/aquamarine objects 2094508 or 2094510 in room 2094667. The action must be
  intercepted, retain the object, and route the south exit to 2094673. Repeat with gold/diamond
  objects 2094509 or 2094511 in room 2094668 and confirm the north exit routes to 2094674. The
  objects must drop normally outside their exact source rooms.
- Kill shadow dragon 2094506 and confirm the north exit of room 2094675 becomes visible and
  unlocked. On successful hits by shadow fiend 2094505 in a lit room, darkness may fire at most
  once per MUD day. Its independent one-in-six mind steal may fire at most once per four violence
  pulses, exempts staff, permits the source-pressure Will save, deals 45d10 source-untyped damage
  on failure, and heals the fiend by the raw roll unless blackmantled.
- Land repeated primary and two-handed hits with warhammer 2094571. About one in 21 must deal
  30d10 source-untyped damage and preserve the ice-hammer messages; offhand hits must not proc.
  Critical primary or two-handed hits with bastard sword 2094566 must add paired zero-duration
  `SPELL_RAINBOW_PATTERN` hit/damage penalties of 4-7 for NPC victims or 2-5 for PCs. Repeated
  criticals while marked must only emit the continuing-lights message and must not stack.

These profiles are automated and reconciled but are absent from the current five-zone pilot.
Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit them
into the pilot.

### Phase 6 converted drow-equipment decay profiles

- Build a dependency-complete Undermountain stage containing objects 2092080-2092082, 2092096,
  2093081-2093085, 2093087, and 2093150-2093155. Confirm each uses `RoL Drow Equipment` and
  `ITEM_AUTOPROC`. Unrelated objects, including the disabled source assignment corresponding to
  2093111, must not receive the procedure.
- Place representative weapons and armor in target Underdark sectors 19-24 and cave sector 29.
  Wait longer than one MUD hour and confirm no value, weight, dice, armor, affect, flag, message,
  or extraction change occurs. Move an object to a surface sector and issue a command that reaches
  its object procedure; its stopped event must restart without creating a duplicate event.
- On the surface, confirm the first event fires after one MUD hour and later events retain the
  source +/-4-pulse jitter translated from four pulses per second to the target pulse rate. Moving
  the object back underground before an event fires must stop rescheduling until a later surface
  command restarts it.
- Compare identical direct, nested, and direct-sunlight objects. A direct object outside sunlight
  must use modulus 6, a nested object modulus 8, and direct sunlight the source-clamped modulus 1.
  The authored daybreak and daytime `OR` predicates are intentionally always true at every hour;
  do not normalize them to conventional time ranges.
- After each decay, confirm the object gains `ITEM_NOSELL`, cost and weight follow source integer
  division, weapon dice collapse to one die before their face count falls, armor `value[0]` falls,
  and only the first two affects mutate. Preserve the source signed-integer behavior for negative
  modifiers and leave later affects unchanged.
- Continue through exhaustion. A weapon or armor that reaches zero must survive that event and be
  extracted on the following eligible event, matching the source threshold check. Directly owned
  objects emit the crumble and terminal-decay messages; ground objects and objects inside another
  object decay silently. Extraction must detach the running object event without a duplicate
  cancellation or use-after-free diagnostic.
- Confirm the source maintenance reset is not exposed: it required a source level above 50, while
  the target staff range ends at level 34. Ordinary staff commands must only exercise the normal
  event-restart path.

These profiles are automated and reconciled but are absent from the current five-zone pilot.
Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit them
into the pilot.

### Phase 6 converted Undermountain ambient profiles

- Build a dependency-complete Undermountain stage containing mobiles 2093012, 2093021-2093023,
  2093202, 2093211, 2093225, and 2093304. Confirm each retains `MOB_SPEC` and uses
  `RoL Source Periodic`; unrelated identities must not receive these profiles.
- Observe every profile while awake and idle. Each uses the source zero-to-100 roll, so only its
  authored low-number cases produce output. Sleeping or fighting mobiles must remain silent.
- Confirm the Mad Mage, Juris, Deriah, Talugen, and Sha'Tar profiles preserve their exact speech
  and room actions. Talugen's fourth case must produce the target social text `$n frowns.` rather
  than a literal command or missing action.
- Confirm the succubus, imp, and Bhara'Tir preserve their exact room messages, pronoun tokens,
  visibility, and case ordering. No profile may substitute a generic ambient message.
- Inspect troglodytes 2093404-2093408 and 2093426. They must not gain a stench procedure from
  this source binding: both source event registration and behavior are compile-disabled. Test
  any independent race or mobile behavior separately.

These profiles and inert dispositions are automated and reconciled but are absent from the
current five-zone pilot. Exercise them only in a disposable Phase 7 dependency-complete stage;
do not hand-edit them into the pilot.

### Phase 6 converted source death effects

- Build dependency-complete Trahern, Dobluth, and Undermountain stages containing mobiles
  2020221, 2020267, 2021783, 2092062, 2093017, 2093018, 2093020, and 2093301. Confirm the
  composable death profiles do not consume or replace an independently persisted mobile
  procedure.
- Kill weevils 2020221 and 2020267 with multiple eligible victims present. The source message
  must appear, an ordinary corpse must remain, and one shared `25d2` roll must damage the room.
  Fire-protected victims halve the current shared amount, including the source's cumulative
  halving for later victims in room order. Peaceful rooms and target-native area safety must
  prevent damage.
- Kill Lady Aleanrahel 2021783 while she carries and wears disposable objects. Her ordinary
  corpse must be suppressed; Dobluth banshee 2021820 must appear in the same room and inherit
  both carried and equipped objects in their original slots.
- Kill Helmed Horror 2092062 and Butcher Knife 2093017. Their source death messages must appear,
  ordinary corpses must be suppressed, and exactly one mapped reward must load: helmet 2092091
  for the horror and knife 2093048 for the butcher.
- Kill gargoyle 2093018 and crystal golem 2093020. Each must emit its exact granite or crystal
  shatter message and suppress the ordinary corpse without creating a reward object.
- Kill level-50 white pudding 2093301. The ordinary corpse must be suppressed and exactly two
  smaller white puddings 2093330 must appear in the death room. The source assignment activates
  only this first split generation; the smaller identities must not acquire an invented death
  callback.

These death effects are production-tested and reconciled but are absent from the current
five-zone pilot. Exercise them only in disposable Phase 7 dependency-complete stages; do not
hand-edit their mobiles or reward dependencies into the pilot.

### Phase 6 converted Griffon's Nest non-Berserker aggression

- Build a dependency-complete Griffon's Nest stage containing converted guards 2010661,
  2010744, 2010745, 2010749, 2010750, and 2010754-2010763. Confirm every identity retains
  `MOB_SPEC` and uses `RoL Monster Combat`; unrelated mobiles must not receive the profile.
- With an idle guard, issue an ordinary command as a visible mortal with no Berserker levels.
  The guard must bellow its battle cry and immediately attack, but the incoming command must
  continue because the source callback returns false after reacting.
- Repeat as a character with at least one Berserker level, including a multiclass character.
  The guard must not react. Staff at or above immortal level and NPC command actors are also
  exempt. An invisible or otherwise unseen mortal must not trigger the guard.
- Leave several eligible and excluded characters in the room and allow the mobile activity
  pulse to run. The guard must select the first visible eligible mortal in room-list order;
  excluded characters must be skipped without changing the ordering rule.
- While the guard is already fighting and has `MOB_MEMORY`, let another visible eligible mortal
  issue a command or be selected by the activity scan. The guard must remember that mortal
  without switching opponents or repeating the battle cry. A Berserker, staff member, NPC, or
  unseen character must not enter memory.

These profiles are production-tested and reconciled but are absent from the current five-zone
pilot. Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit
them into the pilot.

### Phase 6 Dusk Road paralysis gaze and Undermountain venom tails

- Build a dependency-complete stage containing Dusk Road basilisks 2089793 and 2089794,
  Undermountain manscorpion 2093061, and wyverns 2093219 and 2094563. Confirm each identity
  retains `MOB_SPEC` and uses `RoL Monster Combat`; unrelated mobiles must not receive a profile.
- Land successful hits with level-40 basilisk 2089793 and level-50 basilisk 2089794. Their gaze
  checks must occur about one in four and one in two successful hits respectively. The scan must
  skip the basilisk, staff, already-held victims, and ineligible NPCs; it must preserve room-list
  order, require the victim to see the basilisk, continue after a successful Fortitude save, and
  paralyze the first failed eligible target for ten rounds. Confirm the source-equivalent +1 and
  +2 victim save modifiers for the two identities.
- Land noncritical and critical hits with manscorpion 2093061. A noncritical hit must not invoke
  the tail. On a critical hit, paralysis-immune or successful-save victims must survive without
  paralysis; a failed Fortitude save must paralyze the victim for a random two to twelve rounds.
- Repeat with wyverns 2093219 and 2094563. Noncritical hits must not invoke the tail, while a
  failed Fortitude save after a critical hit is fatal. Paralysis immunity and a successful save
  must prevent death, and fatal target invalidation must prevent any later hit rider from using
  the extracted victim.

These profiles are production-tested and reconciled but are absent from the current five-zone
pilot. Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit
them into the pilot.

### Phase 6 Undermountain drow conclave guards

- Build a dependency-complete Undermountain stage containing guard mobiles 2093101, 2093102,
  2093108-2093112 and conclave rooms 2093146-2093156, including barracks 2093153. Confirm bound
  identities 2093102 and 2093108-2093112 retain `MOB_SPEC` and use `RoL Monster Combat`;
  unbound detection-only guard 2093101 and unrelated mobiles must not receive the profile.
- Place a visible mortal below immortal level with an idle bound guard. The first qualifying
  activity pulse must say `I have sounded the alarm! There is no escape!` exactly once. Repeat
  with staff, NPCs, invisible mortals, and fighting guards; none may trigger the idle alarm.
- After the alarm, inspect all 11 conclave rooms. Guard identities 2093101, 2093102, 2093109,
  2093110, and 2093112 must gain detect invisibility wherever they are in that range. Mobiles
  2093108, 2093111, and unrelated identities must not gain it from this procedure.
- In barracks room-list order, place three 2093102 guards followed later by a 2093109 sergeant.
  The guards must path toward rooms 2093146, 2093147, and 2093147 respectively, and the sergeant
  toward 2093155. Interleave unrelated mobiles and verify they stay untouched. Also put an
  unrelated mobile on the global character list outside the barracks and verify the source's
  wrong-list defect is not reproduced.
- Observe a bound guard while fighting over repeated activity pulses. Rolls one through six of
  20 must produce the six exact Lloth/combat lines; other rolls must be silent. Successful hits
  add no separate effect because the registered source callback has no NPC-hit implementation.
- Reset the zone after the first alarm. The alarm must remain suppressed because its authored
  lifetime is global for the process boot, not per zone. Restart the disposable server and
  confirm a new qualifying intruder can sound it once again.

These profiles are production-tested and reconciled but are absent from the current five-zone
pilot. Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit
them into the pilot.

### Phase 6 converted Scornubel profiles

- Build a dependency-complete Scornubel stage containing mobiles 2006001, 2006002, 2006006,
  2006029, 2006051, 2006058, 2006061, 2006064, 2006067, 2006072, 2006106, 2006109,
  2006111, 2006113, 2006132, 2006140, and 2006141, plus fiery mace 2006084. Confirm the
  mobiles carry `MOB_SPEC`; all but Parchimil 2006061 use `RoL Source Periodic`, Parchimil uses
  `RoL Guild Guard`, and the mace uses `RoL Weapon Proc`. Unrelated identities must not receive
  these profiles.
- Observe each mobile while awake, idle, and not fighting. Guardsman, merchants, Lady Rhessajan,
  the clerk, commoners, Parchimil, the loud peddler, mercenary, angry man, butler, Chansrin,
  Karlyn, and maid must reproduce their source speech, socials, room acts, hidden acts, and
  multi-action outcomes. Profiles must use their authored inclusive zero-to-15, zero-to-20, or
  zero-to-50 roll range rather than a normalized chance. Sleeping or fighting mobiles must not
  emit an outcome.
- Exercise Parchimil's configured guild passage as an eligible and ineligible mortal, then
  observe repeated idle pulses. Passage authorization and rejection must remain unchanged, and
  the same mobile must also perform Parchimil's periodic source behavior; no second persisted
  procedure or duplicate activity dispatch may be present.
- Land repeated primary, two-handed, and offhand hits with fiery mace 2006084. About one in 36
  hits must emit the source fire-corona messages and deal exactly 100 points through the source
  untyped compatibility channel. Other equipment slots must never proc, a defeated target must
  safely invalidate the hit context, and the ordinary weapon hit must remain consumed only by
  the normal combat path.

These profiles are automated and reconciled but are absent from the current five-zone pilot.
Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit them
into the pilot.

### Phase 6 converted Zhentil periodic profiles

- Build a dependency-complete Zhentil Keep stage containing minstrel 2081021, little girl
  2081054, terrified merchant 2081059, visiting dignitary 2081066, Scornubian trader 2081067,
  and ugly prostitute 2081068. Confirm all six use `RoL Source Periodic` with `MOB_SPEC`, while
  gate guard 2081074 remains unconverted pending its complete command, gate-state, and event
  lifecycle. Unrelated identities must not receive these profiles.
- Observe the six converted mobiles while awake, idle, and not fighting. The minstrel must use
  its inclusive zero-to-20 table; the dignitary, trader, prostitute, and terrified merchant use
  zero-to-15; and the little girl uses zero-to-10. Sleeping or fighting representatives must
  remain quiet.
- For the little girl, about one in 11 pulses must perform the source wiggle. For the terrified
  merchant, about one in 16 pulses must shudder. These are generated from source zero-roll
  conditionals, not normalized or hand-authored probabilities.
- Confirm the trader's zero outcome first emits the hidden notebook room act and then scratches,
  preserving source order and visibility. The prostitute's roll 2 must emit her source room act
  and then moan; the minstrel's rolls 0-4 and the dignitary's rolls 0-1 must preserve their
  authored acts and socials. Nonmatching rolls produce no output.

These profiles are automated and reconciled but are absent from the current five-zone pilot.
Exercise them only in a disposable Phase 7 dependency-complete stage; do not hand-edit them
into the pilot.

### Phase 6 converted Waterdeep ambient citizens

- Build dependency-complete stages containing the first 34 converted ambient citizens:
  2002812-2002813, 2002815-2002816, 2002827, 2002829, 2002835-2002836,
  2003006-2003007, 2003009-2003012, 2003014, 2003018, 2003030,
  2003064-2003066, 2003090,
  2003201, 2003203-2003205, 2003210, 2003232, 2003236, 2003242-2003243,
  2004830, 2005310, 2005321, and 2066037. Confirm each uses
  `RoL Waterdeep Ambient` with `MOB_SPEC`.
- Add the 22 converted tailor, shopper, assassin, brigand, fisherman, sailor, seaman,
  naval worker, seabird, commoner, and guard profiles: 2002825, 2002830, 2002832-2002834,
  2003035, 2003038, 2003059, 2003070, 2003234-2003235, 2003240, 2005300, 2005302-2005303,
  2005305, 2005307-2005308, 2005316-2005318, and 2005320. Confirm these use the same
  named procedure and bring the active profile set to 56 mobiles across 44 families.
- While each mobile is standing, observe repeated activity pulses. Wanderer, drunk,
  homeless, cat, merchant, farmer, baker, mage, cleric, artillery, warrior, mercenary,
  casino, and youth profiles should emit their source-authored speech and room actions;
  most use a two-d5 roll, mobile 2003204 uses two-d7, and 2003205 uses two-d6.
- Confirm multi-message rolls remain ordered. In particular, a roll of 2 for casino
  player 2003205 says the raise and then studies the cards, preserving the source
  switch fall-through. Nonmatching rolls produce no output.
- Move merchant 2005310 between target harbor room 2005400 and another room. Its
  ambient dialog should run only in 2005400. Sitting or sleeping ambient citizens and
  unrelated mobiles must not emit these profiles.
- Start fights involving converted guards 2003035, 2003059, and 2003070. Their ambient
  guard dialog must stop during combat and resume after combat, while other profiles
  retain their source position-driven behavior.

### Phase 6 generated source-periodic profiles

- Build dependency-complete source-profile stages. Confirm the following 122 mobiles use
  `RoL Source Periodic` with `MOB_SPEC`: Bloodstone 2007100-2007109, 2007113-2007129,
  2007142-2007144, 2007147, 2007152-2007154, 2007156, 2007160-2007162,
  2007164-2007167, 2007170, 2007172-2007180,
  2007189, 2007191-2007195, 2007199-2007203, 2007205-2007206, 2007209, 2007220,
  2007221, 2007308, 2007311, 2007314, 2007317, and 2007321-2007326; Fun 2001230;
  Mobile 2003069; Realm 2014048; Icecrag 2097000-2097002,
  2097005, 2097007-2097008, 2097011, 2097014, 2097016, 2097021, 2097023, and
  2097028, 2097033; Menden 2088806; Tower of Sorcery 2015901; Waterdeep 2003212;
  Lavatubes 2012000, 2012002, and 2012003; and Scornubel 2006001, 2006002, 2006006,
  2006029, 2006051, 2006058, 2006064, 2006067, 2006072, 2006106, 2006109, 2006111,
  2006113, 2006132, 2006140, and 2006141; and Zhentil Keep 2081021, 2081054, 2081059,
  2081066, 2081067, and 2081068.
- Observe repeated activity pulses while representative profiles are idle. Their source speech,
  social room text, and direct room actions should appear at the original random cadence.
  Multi-action and fall-through outcomes must retain their source order. Nonmatching rolls must
  remain quiet.
- Confirm the added Lavatubes profiles retain their source dice distributions: snowbeast 2012000
  rolls three-d6, and spiny creatures 2012002-2012003 roll three-d2. Their behavior must stop in
  combat. Waterdeep guard 2003212 rolls two-d4 and emits its profile only while sleeping. Bulette
  2015901 retains its zero-to-40 random range without an awake or combat gate.
- Put ordinary representatives to sleep and confirm their periodic behavior stops. Fun mobile
  2001230, jester 2003069, and cricket 2014048 deliberately retain source profiles without an awake
  gate. Start combat and confirm ordinary profiles stop; Fun mobile 2001230, jester 2003069, and
  Menden magus 2088806 deliberately retain their source combat behavior. Unrelated mobiles must
  not receive any of these identity-keyed profiles.

### Phase 6 generated state-aware Waterdeep profiles

- Build a dependency-complete Waterdeep stage and confirm these 27 mobiles use
  `RoL Stateful Periodic` with `MOB_SPEC`: 2002823, 2003020-2003023, 2003039, 2003206,
  2005315,
  2005503-2005504, 2005507-2005508, 2005510, 2005513-2005521, 2005525, 2005530,
  2005533-2005534, 2005538, and 2005540.
- Confirm seven additional Waterdeep guild guards use `RoL Guild Guard` with `MOB_SPEC`
  while drawing from the same generated state-profile table: 2002824, 2003025-2003027,
  2005505, 2005511, and 2005535.
- While awake and standing but not fighting, observe representative profiles across repeated
  activity pulses. They should use their source idle speech and room actions. Most roll two-d5;
  Selune dancer 2005519 rolls two-d7. Sitting and sleeping representatives remain quiet.
- Start combat with representative ordinary profiles. Each should switch to its one-d4
  fighting table instead of emitting idle text. This intentionally makes the explicitly authored
  combat branch usable where the source tested standing position first. Guildmaster 2003020 has
  no fighting table and must remain quiet in combat.
- Start combat with casino owner 2003206. It must independently roll its two-d5 fighting
  table and then its two-d5 standing table on the same activity pulse, preserving the source
  callback's two separate conditions rather than selecting only one table.
- For commoner 2003039, confirm idle outcomes include the ordered two-line missing-child speech,
  while fighting outcomes include calls for help and other combat reactions. Unrelated mobiles
  must not receive any of these identity-keyed profiles.
- Confirm converted rogue 2005509 has no named procedure from `rogue_one`: its source callback
  registered only for `NPC_HIT` but returned whenever that event supplied its victim, so adapting
  the unreachable body would invent behavior.

### Phase 6 remaining named guild guards and utility objects

- Confirm converted guild guards 2003024, 2005500, 2005524, 2005528, 2005531, and 2005537 use
  `RoL Guild Guard` with `MOB_SPEC`. Exercise their Wizard/Sorcerer/Summoner, Paladin, Bard,
  Ranger, Druid, and Wizard gates at rooms 2003038, 2005500, 2005534, 2005540, 2005560, and
  2005572 respectively. Rejected characters must remain outside; an admitted matching class must
  move through the configured exit. Attacking the protected guard must trigger retaliation.
- Observe guard 2005500 while awake, idle, and at its load room. Its source two-d5 Paladin greeting
  table must remain active. Moving it, putting it to sleep, or starting combat must suppress that
  table. The other five named guards must not gain unreachable source periodic behavior.
- Confirm objects 2000876, 2007151, 2046991, 2088825, and 2090004 use `RoL Utility Object`.
  Eating the selected carried goodberry while hungry must preserve native eating and cure light
  wounds. `get child`, `take child`, and `drag child` against the altar child must be blocked and
  cause one to nine hit points of source damage.
- Carry necromancer child 2046991 through repeated object pulses and confirm only source messages
  appear; soundproof rooms and silence suppress them. Hold or wield figurine 2088825, then
  `flex <figurine>` and confirm its remapped mobile appears charmed, follows the user, and consumes
  the figurine. Leave monocle 2090004 in rooms 2090124-2090142 and confirm it moves within that
  range only while the zone age is zero.
- Confirm objects converted from source handlers `blackPlagueCure` and `craine_serpent` have no
  named procedure. Their direct callbacks register no runtime events in the assessed source.

### Phase 6 converted Waterdeep peacekeepers

- Build dependency-complete Waterdeep stages and confirm tavern bouncers 2005523 and
  2005541-2005543, casino bouncer 2003207, and off-duty militia guard 2003229 use
  `RoL Waterdeep Peacekeeper` with `MOB_SPEC`.
- Move each tavern bouncer away from its assigned post without charming it. On an activity
  pulse it must fade out and return to its profile home: 2005523 to 2005532, 2005541 to
  2005531, 2005542 to 2005530, and 2005543 to 2005533. A charmed tavern bouncer must not
  return or eject anyone.
- At each bouncer post, start a visible fight in which the lower-alignment participant is
  fighting a neutral or good NPC. The bouncer must stop the selected offender and everyone
  attacking that offender, drag both characters along the source route, leave the offender
  sitting in room 2003258, and return to its post. With multiple eligible fights, the visible
  participant with the lowest alignment must be selected.
- Move casino bouncer 2003207 away from its load room and confirm it returns on an activity
  pulse. Start an eligible fight at its post and confirm the offender is removed from combat,
  thrown into room 2003254, and left sitting while the bouncer remains at its post.
- Observe off-duty guard 2003229 while standing. It must retain the source two-d6 drunken
  ambient table. Start an eligible fight while the room is not peaceful; the guard must speak,
  join against the selected aggressor, and stop scanning once already fighting. Unrelated
  mobiles must not receive any peacekeeper behavior.

### Phase 6 converted monster-combat procedures

- Confirm these 45 mobiles use `RoL Monster Combat` and retain `MOB_SPEC`: 150772,
  196007, 196027, 196040, 196076, 2000325-2000328, 2001407, 2001437, 2004070,
  2004480, 2004530, 2005023, 2012005, 2012006, 2012024-2012026, 2014026,
  2014601, 2015113, 2020378, 2034833, 2041900, 2043358, 2045116, 2045146,
  2045182, 2051246, 2051334, 2053264-2053266, 2062401, 2062402, 2062405,
  2062406, 2081706, 2081746, 2081747, 2083224, 2092608, and 2097061. Identify
  each and confirm its profile description.
- Exercise both plant profiles, the four lycans, spider 2005023, banshee 2034833, and
  the four pit fiends. Confirm their source-profiled poison, tearing, venom, sonic, and
  bite cadences. Werefox 2000326 and were-tiger-profile mobiles 2000325, 2000327, and
  2000328 must display their source fade messages and leave no corpse.
- Fight four-arm mobile 2045116 and tentacled mobile 2045146. The first receives one
  nonrecursive extra swing per combat turn and a rare Fortitude shockwave; the second
  produces its rarer Reflex shockwave. Failed saves sit and briefly stun eligible room
  targets. Rot Bringer 2045182 must summon helper 2045193 exactly once after falling
  below 40 percent hit points.
- Exercise Ashentoris 2020378, winged deva 2051246, and prismatic elementals
  2053264-2053266. Confirm the life-drain/lava, healing-lightning/earthquake, and
  level-specific prismatic profiles. Small elemental 2053264 must follow its master,
  join the master's fight, and vanish without a corpse when abandoned. Critical
  elemental 2053265 uses the documented one-in-20 combat-turn approximation because
  the target registry has no source NPC-critical event.
- Fight Elemental Tower bosses 2062401, 2062402, 2062405, and 2062406. Each must retain
  its one-time source alert and helper pursuit while using the single persisted monster
  procedure. Confirm the fire room storm, earth rockfall and knockdown, air single-target
  whirlwind and forced movement, and water tidal damage and silence. Active casting
  suppresses the elemental attack but does not create a second persisted SpecProc.
- Exercise kobold priest 2001437 in room 2001482 and elsewhere. Its north, east, and south
  force walls and west pit route must match the source room behavior; outside that room its
  four-in-five directional wall and five-pulse imp summoning cadence must remain active.
  Piercers 2004070, 2004530, and 2092608 must initialize at one hit point and +100 hit roll,
  make one hidden ambush with the source awareness save, and then remove `MOB_SPEC`.
- Fight purple worm 2004480, phalanx 2012005, skeletons 2012006 and 2012024, and the
  Xexos/Agthrodos pair 2012025-2012026. Confirm swallow healing and possession drops,
  phalanx ceiling blocking/retreat/crystal drop, one-in-20 skeleton trips and three-generation
  splitting, and equipment-preserving two-way transformation.
- Exercise tree spirit 2014026, Dranum 2015113, swallow profiles 196007, 196076, 2041900,
  and 2097061, movanic deva 2043358, and Canthus 2051334. Confirm root restraint and helper
  summons, life drains, lethal and nonlethal possession transfer, holy healing/wind, pack
  summons, and four-round elemental breath cadence.
- Fight Thrym 196027 and Utgard-Loki 196040. Confirm the one-in-two ice restraint and
  one-in-three room fear profiles. Fight each pit fiend and confirm its bite and independently
  composed tail save/death/restraint behavior share the single persisted procedure.

### Phase 6 converted weapon procedures

- In dependency-complete stages, confirm these 50 objects use `RoL Weapon Proc`:
  2001005, 2001010, 2001057, 2004505, 2008000, 2009054, 2013307-2013308, 2014023, 2014837,
  2015116, 2019886, 2019900, 2019912, 2019933, 2020075, 2024405, 2025018,
  2025030, 2026014, 2026233, 2026248, 2034840, 2038025, 2038095, 2040135, 2043741,
  2053243, 2053250, 2053259, 2053263, 2053266, 2053271, 2053289-2053292, 2080034, 2080038,
  2080547, 2083235, 2083238, 2089462, 2091305, 2095776, 2095851, 2095876,
  2095878, 2097117, and 2098330. Identify each object and confirm its
  identity-specific special-effect description appears.
- Fight with hammer 2004505 until its chain lightning fires and confirm the primary
  target is struck before eligible visible enemies. Exercise glimmering sword 2014837,
  Nightbringer 2026014, Kirin horn 2034840, fire-giant sword 2080547, barbed sword
  2091305, and black-flame longsword 2095876 until each produces its documented random
  hit effect. Normal weapon damage must still occur.
- Force or repeatedly produce critical hits with icy dagger 2013307, slender elven
  longsword 2020075, acid longsword 2089462, rippling-flame longsword 2095776, and
  jeweled fang 2095851. Confirm the cold, elven-wound, acid, fire/healing, and piercing
  profiles fire only on critical hits. Fire Elementals and Efreeti must be healed by
  2095776 instead of damaged; incorporeal targets must ignore 2020075.
- Critical hits with shadow dagger 2040135 must add damage based on the exact completed
  hit payload. Sneak attacks have a separate one-in-four shadow flurry that can stun the
  victim and armor the wielder. Ordinary noncritical, non-sneak hits must not use either
  branch.
- Give Windsong 2038025 or 2038095 to a non-Ranger and land a hit. The weapon must reduce
  a healthy wielder to one hit point, unequip, and fall into the room without being
  destroyed. A Ranger keeps the weapon and can trigger its extra-swing flurry; Elf,
  High-Elf, and Half-Elf profiles receive their mapped race weighting.
- Exercise Gith swords 2019886 and 2019900 against eligible sub-level-51 targets. Confirm
  severing and charged-vorpal outcomes use target-safe damage, a summoned reclaimer is
  hostile to a player wielder, and the charged blade destroys itself on its tenth
  activation. Exercise Valhalla scepter 2019912 in primary and offhand slots and confirm
  its extra swings do not recursively trigger another scepter proc.
- Wield moonblade 2095878 outside at night and confirm its random star flare applies
  faerie fire. While it is equipped, `say labelas` must apply barkskin to the wielder and
  grouped room companions, then refuse another invocation until its 168-MUD-hour
  cooldown expires.
- Exercise crimson dagger 2098330 in primary and offhand slots. Confirm its random
  noncritical branch drains Strength or agility through the mapped target effects, while
  a primary-slot critical can blind and add crimson damage.
- Give Mielikki scimitar 2019933 to a Ranger or Druid and confirm its random creeping-doom
  strike. Give it to an ineligible class and land a hit; it must reduce a healthy wielder
  to one hit point, unequip, and fall into the room. Exercise Flamberge 2025030 against an
  ordinary target and against a Fire Elemental or Efreeti; the first takes fire damage and
  the latter two heal.
- Exercise Orb 2009054 with an arcane caster, Bard, and noncaster. Confirm the documented
  class-weighted cold-burst cadence. Arcane criticals can add cold shield when it is absent;
  Bards and noncasters must not receive that critical shield.
- Exercise Doombringer 2025018 and dirk 2097117 in valid weapon slots. Their five and one
  extra swings respectively must not recursively trigger the same weapon procedure. While
  using Tahlshara 2001010, start below standing position and confirm the next hit restores a
  fighting stance; its random main proc can sit/stun the target and heal the wielder.
- Exercise Rockcrusher 2080034 or 2080038 on a grounded corporeal non-dragon and confirm its
  localized earthquake can sit/stun the target. Repeat against a dragon, incorporeal or
  flying target, and in water, flying, underwater, ocean, river, or no-ground terrain; no
  earthquake effect should apply.
- Exercise Cymric weapons 2026233 and 2026248 for their harm beam, Torment 2015116 for
  poison and blindness against a non-dragon, Pahluruk root 2013308 for a timed entangle,
  Frulghiem 2001005 for clenched fist, and sphere weapons 2014023 and 2024405 for two
  sequential lightning bolts. A lethal first bolt must not attempt the second bolt.
- Exercise Halruaan staves 2053259, 2053263, and 2053266 for flameheart damage and their
  target-native illusion/enchantment debuffs. Magebane objects 2053289-2053292 must affect
  only NPC arcane casters and can interrupt an active cast; dwarven hammer 2053243 produces
  its freezing-cold burst.
- Give dark-aura sword 2083238 to an evil wielder and gleaming sword 2083235 to a good
  wielder. Confirm their negative/blind/weakening and faerie-fire/blind progressions.
  Ineligible alignments must not trigger them. Unrelated objects must not receive any
  identity-keyed weapon behavior.
- Exercise elemental staff 2053250 against an Elemental and a non-Elemental. Only the
  Elemental may receive its 20d10 disruption proc. While fighting, `say summon prismatic
  helper` must load charmed helper 2053264, engage the current opponent, and start a
  72-MUD-hour cooldown; the same phrase outside combat or during cooldown must not summon.
- Put a corpse and a non-corpse in the room while wielding necrostaff 2053271. `say preserve
  corpse of <name>` must add 1,000 decay ticks only to the named corpse and start a
  72-MUD-hour cooldown. Its random hit proc must deal 100 negative damage and heal the
  wielder by 25 without exceeding maximum hit points.
- Force critical hits with dread gythka 2043741. Eligible targets can receive one tick of
  slow or two ticks of paralysis; dragons and converted demons/devils are immune. Ordinary
  hits retain the one-in-24 40d10 poison payload with a target-native Fortitude half save.
- Give holy weapon 2008000 to evil, neutral, good non-Paladin, and Paladin wielders. Evil
  players must be blasted and lose the weapon, neutral wielders must silently lose it, and
  a worthy good room recipient takes precedence over room relocation. Good non-Paladins
  use it normally. Paladins receive the one-in-21 combat dispel/smite and one-in-eight
  periodic stoneskin, armor, bless, or heal sequence.
- Fight a dragon and a non-dragon with Kor battleaxe 2001057. Its periodic modifiers must
  switch between +8 hit/+6 damage and +4 hit/+3 damage. Criticals add one nonrecursive
  reverse swing; the independent Tempus heal and 12d10 damage procs retain their source
  cadence. When that damage kills the target, converted head object 2001058 must appear
  in the room with the victim's name.

### Phase 6 converted Lavatubes procedures

- Observe snow vulture 2012001 while it is idle and while it is fighting. Only the idle
  state may produce its squeak, flap, or corpse-devouring outcomes.
- Hold crystal spike 2012000 and cast until its authored charge count reaches zero. Each
  cast must consume one charge; the zero-charge cast must show the fade messages and
  safely extract the held spike.
- Hold skeleton key 2012025 and try to unlock a locked container or door whose proper key
  is absent. Pickproof targets must always break the skeleton key. Other targets use the
  converted Dexterity-based break chance; success unlocks both sides of a paired door.
  Carrying the proper key must leave the command to native unlock handling.
- In rooms 2012158 and 2012159, descend through an open trapdoor while switch object
  2012027 is in the cellar. The move must complete, both exits must close, and the paired
  exits must become blocked. Pulling the lever below must clear both blocked flags.
- Reset the blocked pair with automaton 2012027 alone in the cellar; its activity must
  clear both blocked flags. Repeat with another character present and confirm that the
  automatic unblock does not occur until the automaton is alone.

These procedures are production-tested and reconciled, but the current five-zone pilot
does not contain their Lavatubes package. Exercise them only after a Phase 7 staged batch
supplies the converted records; do not hand-edit them into the pilot.

### Phase 6 converted Tarrasque encounter

- In a dependency-complete Tarrasque stage, confirm mobile 2002601 and objects 2002604 and
  2002610 all use `RoL Tarrasque Encounter`. The mobile must retain `MOB_SPEC`; stomach-acid
  object 2002610 must retain `ITEM_AUTOPROC`.
- Damage the Tarrasque below maximum hit points and observe its activity pulses. A healing
  pulse adds 25 hit points using the source behavior, including a possible final overshoot.
- Fight with a disposable player and pet. A pet engaged as the current target must be bitten
  in half and heal the Tarrasque by 300. Player combat must exercise the ordered one-in-19
  swallow, one-in-31 tail-fling, and one-in-16 room tail-sweep branches; an earlier branch
  suppresses later branches for that turn.
- A surviving swallowed player must stop fighting and casting, move to stomach room 2002661,
  take the 10d15 impact, and heal the Tarrasque by 200. Stomach acid must pulse for 10d10
  acid damage against mortal players and pets and independently have a one-in-three chance
  to interrupt casting and spell preparation. Target-native acid resistance and protection
  reduce this typed damage.
- A tail-flung player must stop fighting and casting, move to a valid random mortal teleport
  destination, become reclining, and receive a two-round stun when eligible. A tail sweep
  must damage every mortal player and pet in the room with 20d12 bludgeoning damage; the
  converted Reflex save at source modifier -2 halves the damage.
- Kill the Tarrasque in a disposable stage. The death event must create special corpse
  2002604 in the death room, suppress the ordinary NPC corpse, place exactly one weighted
  loot object (2002605, 2002606, 2002607, or 2002608) in room 2002661, and place portal
  2002609 there with the death room as its normal destination. Confirm the earthquake and
  crash message occur.
- From the death room, `enter corpse` or another valid corpse alias must move a player into
  room 2002661. NPCs, unrelated objects, and unrelated `enter` arguments must not use the
  encounter path.

This encounter is production-tested and reconciled, but the current five-zone pilot does
not contain its source package or dependencies. Exercise it only after a Phase 7 stage
supplies the converted room, mobile, corpse, loot, acid, and portal records.

### Phase 6 planar demon base behavior

- In a dependency-complete planar stage, inspect source mobiles 205-221, 234, 93202-93206,
  93209, and 93210 at their converted identities. All 25 must have `RoL-Abyss-Forged` without
  consuming the persistent SpecProc slot.
- Confirm direct `standardDemon` source bindings 92079, 93202-93206, 93209, and 93210 are race X
  and receive `RoL-Demon` through automatic race conversion. They must not gain a second
  persistent procedure, and any independently converted direct procedure must remain intact.
- Equip a marked disposable mobile with test weapons in the primary and off-hand slots, then
  kill it. Both weapons must show the dissolution message and neither may enter the corpse.
  Repeat with a two-handed weapon and confirm the same result.
- After the remaining Balor death handler is reconciled, repeat with marked source mobile 207
  or 93204. Its wielded weapons must dissolve before the typed handler suppresses or replaces
  ordinary corpse creation.
- Kill an otherwise comparable demon without `RoL-Abyss-Forged`. Its ordinary wielded weapons
  must follow the native corpse path, proving the dissolution applies only to the authored
  source subset.

This behavior is production-tested and reconciled, but the current five-zone pilot contains no
planar demon package. Exercise it only after a Phase 7 stage supplies the converted prototypes;
do not add the compatibility flag to unrelated demons by hand.

### Phase 6 planar mobile initializers

- In a dependency-complete planar stage, inspect converted Bar-lgura source mobile 208. It must
  have `RoL-Has-Th` and the permanent hide affect in addition to its automatic demon state.
- Inspect converted Cambion source mobiles 209 and 92079. Both must have `RoL-Has-Th` and the
  permanent sneak affect. Mobile 92079 must also retain every other independently converted
  callback requirement; no composable flag or affect may disappear because of binding order.
- Attempt to charm converted Lemure 229 and Nupperibo 230. Both must use target-native
  `MOB_NOCHARM` behavior and must not consume the persistent SpecProc slot.
- Confirm converted Dretch 211 lacks the wimpy action. Its source callback only removes that
  already absent property, so no replacement procedure should be attached.
- Confirm Alu-fiend 205 and Rutterkin 219 gain no procedure for `demon_aluFiendRegen` or
  `demon_rutterkin`. The first behavior is disabled in active source code and the second changes
  no state; automatic demon behavior and independently converted callbacks must remain intact.

These initializer dispositions are conversion-tested and reconciled, but the current five-zone
pilot contains no planar package. Exercise them only after a Phase 7 stage supplies the converted
prototypes; do not hand-edit flags or affects into the pilot.

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

### Phase 6 Undermountain Death's Head lifecycle

- In a dependency-complete Undermountain stage, confirm mobiles 2093013-2093016 use
  `RoL Death's Head` with `MOB_SPEC`, and seed object 2093044 uses the same owner-aware
  procedure with its object auto-procedure flag.
- Load sapling 2093013, young tree 2093015, and mature tree 2093016 separately. Their initial
  head counts must fall in ranges 1-5, 6-10, and 11-16. Put a larger tree in the same room as
  a sapling or young tree; the smaller tree must die out on its next activity pulse.
- Leave a sapling or young tree with zero or one ordinary corpse for more than 21 activity
  pulses; it must remain unchanged and restart its growth count. Repeat with two corpses; it
  must replace itself with the next tree stage and suppress its ordinary corpse.
- Observe a mature tree for 51 activity pulses. Its source-bug-compatible regrowth must set the
  head count to exactly 11. Every 11 pulses, each current head has an independent one-in-three
  fruit-drop chance, but the tree must retain at least one head. Its one-in-ten cry attempt must
  send the opposite-direction line through the first valid exit in source direction order.
- Fight each tree repeatedly. Every head has an independent one-in-21 bite chance per hit event;
  a bite adds seed 2093044 to the current victim, including NPC or staff victims, and does not
  deduplicate existing seeds. Killing any tree must suppress its ordinary corpse. The mature
  tree must not drop wood because the active source compares a mobile index with an object index
  and its wood branch is unreachable.
- Fight fruit mobile 2093014 with a mortal player. Its one-in-11 bite chance must implant seed
  2093044; staff and NPC victims are exempt. Killing the fruit suppresses its ordinary corpse.
  In a tree-free room, a wandering fruit must ignore an unseeded corpse, but a corpse already
  containing one seed causes the fruit to add a second seed and extract itself.
- Carry seed 2093044 in a mortal. Its first growth event occurs after the source-equivalent
  60 seconds and later events recur every 3.5-4.5 seconds. Growth increments object value zero,
  deals one-to-two damage initially and then two-to-growth damage, and emits the wince message.
  Staff receive the message without damage.
- Move the seed into an ordinary corpse in a populated room above internal room index zero. With
  no tree present, the next object pulse or seed event must create sapling 2093013. The successful
  sprout deliberately leaves the seed in the corpse; the next pulse detects the existing tree
  and removes it. A seed placed beside an existing tree must be removed without creating another.

This lifecycle is production-tested and reconciled, but the current five-zone pilot contains no
Undermountain Death's Head package. Exercise it only after a Phase 7 stage supplies all five
bound prototypes and their rooms; do not hand-edit the pilot.

### Phase 6 remaining hit-only weapons

- In dependency-complete stages, confirm objects 196000, 2020208, 2020271, 2021759, 2093035,
  2093086, and 2093156 persist `RoL Weapon Proc` and object extra-flag bit 44. `stat object` must
  show the identity-specific special-effect description rather than a generic or missing proc.
- Wield Frostbite 196000 in a primary weapon slot and make repeated hits. Its one-in-22 proc must
  deal `30d10` cold damage. Move it to an offhand-only slot and confirm that the burst stops.
- Wield crystal sword 2020208 and obsidian sword 2020271 in either a primary or offhand slot.
  Their one-in-33 procs must deal `(level / 5)d10` source-untyped damage. At source hours 6-17,
  the crystal sword must also cast Scorching Ray; at hours 0-5 and 18-23 it must not. The
  obsidian sword uses the inverse boundary: at night it applies one round of -50 maximum movement
  and -4 AC, while source hours 6-17 apply neither penalty.
- Wield broadsword of dancing shadows 2021759 in a primary slot. Its one-in-26 proc must roll
  `37d9` negative damage. Protection from evil halves the roll; a converted demon or devil halves
  it again; and a successful Will save halves the remainder again. Fire Shield applies the
  source save modifier. Verify the reductions in that order with an exact test fixture if live
  randomness makes the combined case impractical. The proc must not fire from an offhand slot.
- Critical-hit test both snake whips, 2093086 and 2093156. Each must cast level-40 poison on the
  struck target. Critical-hit test searing rod 2093035; it must cast level-35 Burning Hands.
  Ordinary noncritical hits from these three identities must not fire their critical effect.
- Kill a target with each direct-damage identity and confirm no later hit rider dereferences the
  extracted target. Regenerate a staged object and confirm the persisted procedure and required
  auto-procedure flag are emitted together.

These weapons are production-tested and reconciled, but the current five-zone pilot does not
contain all seven prototypes. Exercise each only after a Phase 7 stage supplies its owning
package. This checkpoint resolves hit and critical callbacks only: do not treat it as approval to
rewrite Frostbite's separate Jotun passive apply slots.

### Phase 6 Bhaal and Seelie hit weapons

- In dependency-complete stages, confirm Bhaal warrior weapon 2063747, Bhaal rogue weapon
  2063794, and Seelie bard's glaive 2062750 persist `RoL Weapon Proc` and object extra-flag bit 44.
  `stat object` must show `Bhaal's Torment` behavior on the two Bhaal identities and the
  Dexterity-weighted blinding-light behavior on the glaive.
- Wield either Bhaal weapon in a primary or offhand slot and strike a target without Fire Shield
  or Cold Shield. No Torment rider must fire. Add either shield and verify that every positive
  triggering hit repeats its encoded damage, or half that damage after a successful translated
  save. The rogue blade uses flare wording and the warrior blade uses flash wording; cold and
  fire shields retain distinct messages.
- Repeat the shielded strike while the wielder has elemental protection, then Globe of
  Invulnerability. Both defenses must suppress the Torment rider. Remove the defenses and confirm
  the rider resumes. A zero-damage hit and a Bhaal weapon outside the primary/offhand slots must
  not fire it.
- Give glaive 2062750 to a level-25 wielder and confirm that repeated hits never proc. At level 26
  or higher, its exact chance is `(current Dexterity + 1) / 2001` for a nonnegative Dexterity.
  Use a controlled roll fixture where practical: rolls zero through Dexterity fire, and the next
  roll does not.
- On the first qualifying glaive proc against a sighted target, confirm two rounds of blindness
  with no damage rider. While the target is already blind or has Blindness, the next qualifying
  proc must instead roll `10d10` source-untyped damage, halved after a successful translated save.
  Primary and offhand slots are valid; other slots are not.
- Kill a target with each direct-damage branch and confirm no later hit rider dereferences the
  extracted target. Regenerate each staged object and confirm the persisted procedure and required
  automatic-procedure flag are emitted together.

These weapons are production-tested and reconciled, but the current five-zone pilot does not
contain all three prototypes. Exercise them only after a Phase 7 stage supplies their owning
packages. The master bard's glaive, Spider venom pouch, and Jotun skull remain pending because
their source callbacks can suppress or replace base-hit damage before the current target gateway.

### Phase 6 Undermountain Astral-forged and Torin weapons

- In a dependency-complete stage, confirm objects 2093191, 2093195, 2093446, and 2093447 persist
  `RoL Weapon Proc` and object extra-flag bit 44. Source rooms whose sector was 23 must map to the
  target Planes sector and also persist `RoL-Astral`; neighboring non-Astral rooms in a mixed
  source zone must not gain that marker.
- Put Astral-forged object 2093191 or 2093195 on the ground, in an ordinary container, in a
  character's inventory, and in a worn slot. On each automatic pulse, affect slots zero and one
  must become +3 hitroll and +3 damroll outside an exact `RoL-Astral` room and +6/+6 inside one.
  Moving a worn weapon across the boundary must immediately refresh the wearer's derived values.
- Identify Torin object 2093446 and 2093447. Both must report that a Warrior or Cleric Mountain
  Dwarf or Duergar may use the item and that its combat critical is Chain Lightning. This is the
  intended source disclosure even though only object 2093447 has the separate active critical
  callback.
- Give either Torin object to a qualifying player, an immortal, or no owner. Its next pulse must
  restore prototype values one through three, wear flags, extra flags, weight, cost, character
  affect bits, and object applies without overwriting value zero. Repeat while it is nested in a
  carried container to verify recursive owner discovery.
- Give either Torin object to a nonqualifying mortal player or to a pet whose mortal master does
  not qualify. Each pulse with an active descriptor must show the intense-light burn and deal
  5-50 source-untyped damage; the early restriction path must not restore altered prototype state.
  A qualifying master must allow a pet-held object to use the normal restoration path.
- Critical-hit a target with object 2093447 in a primary or offhand slot. It must emit the
  lightning-stream message and cast level-40 Chain Lightning. A noncritical hit must not cast it,
  and lethal spell resolution must invalidate the extracted target safely. Object 2093446 must
  never cast Chain Lightning because it has only the shared general callback.

These weapons and their room metadata are production-tested and reconciled, but the current
five-zone pilot does not contain their Undermountain package. Exercise them only after a Phase 7
stage supplies the four prototypes and the exact converted room set; do not mark an entire mixed
zone Astral by hand.

### Phase 6 Undermountain Vortex Knights

- In a dependency-complete Undermountain stage, load Silver Knight 2093003 with silver portal
  2093006 and destination room 2093097; Golden Knight 2093004 with golden portal 2093007 and
  destination 2093098; and Platinum Knight 2093005 with platinum portal 2093008 and destination
  2093099. The three mobiles compose their death profiles without consuming a persistent
  special-procedure slot.
- Put two instances of one Knight identity in the same room and kill one. The ordinary corpse
  must be suppressed, but no portal may appear while the same-prototype peer remains. A different
  Knight identity in the room must not block the portal.
- Kill the last Silver, Golden, or Platinum Knight of its prototype in the room. It must suppress
  the ordinary corpse, create exactly its mapped portal in the death room, and emit the matching
  dissolve-and-coalesce message. Preserve the source spelling: Silver and Golden say
  `coallesces`, while Platinum says `coellesces`.
- Inspect each created portal. It must have the target decay flag and timer one, enter its mapped
  destination normally, and disappear on the next target mud-hour object update. A missing portal
  prototype must log an explicit error while the authored corpse suppression still occurs.

These deaths are production-tested and reconciled, but the current five-zone pilot contains no
Vortex Knight package. Exercise them only after a Phase 7 stage supplies all three Knights, all
three portals, and all three destination rooms; do not hand-create only one side of a portal.

### Phase 6 Trahern combat handlers

- In a dependency-complete Trahern stage, confirm Gakarak 2020217, Kazgoroth 2020234, and Slothen
  2020248 persist `RoL Monster Combat` and `ACT_SPEC`. Kazgoroth also requires destination room
  2020237; an unavailable destination must log an explicit error without moving or damaging the
  target.
- Fight Gakarak with multiple standing occupants in the room. On roughly one-third of successful
  hits, the root-quake message must appear. Each occupant whose `1..101` roll exceeds half current
  Dexterity must sit and receive one violence pulse of wait; the attacker, already sitting or
  lower occupants, and targets that pass the threshold must remain in place.
- Fight Kazgoroth until the toss fires on a successful hit. Its current opponent must move to room
  2020237, take typed `10d10` bludgeoning damage, leave combat on both sides, recline, and receive
  a three-round stun if it survives. Verify bludgeoning defenses apply and a lethal toss does not
  access an extracted target or create a combat event between different rooms.
- Fight Slothen with an ordinary player opponent, grouped allies, uninvolved NPCs, and a hostile
  eligible bystander present. On a proc, the opponent must take typed `10d15` acid. The room burst
  must spare the attacker, group-safe targets, and unrelated protected NPCs; eligible targets that
  fail the source-equivalent Fortitude save take typed `20d15` acid. Acid resistance must reduce
  both damage paths, and lethal primary or area damage must invalidate targets safely.

These handlers are production-tested and reconciled, but the current five-zone pilot contains no
Trahern package. Exercise them only after a Phase 7 stage supplies all three mobiles and the exact
Kazgoroth destination; do not substitute an arbitrary room for 2020237.

### Phase 6 Trahern Erinyes charm lifecycle

- In a dependency-complete Trahern stage, confirm Erinyes 2020246 persists one merged `RoL
  Monster Combat` procedure and `ACT_SPEC` despite its two source callbacks. It must not initiate
  ordinary aggressive combat; test the charm while it is awake and not already fighting.
- Place eligible mortal PCs of each sex in the room. Each is eligible, unlike the separate planar
  Succubus identity, which remains sex-selective. On roughly one-fourth of activity pulses, the
  Erinyes must send its telepathic invitation and make the source-equivalent Will save. Verify the
  source `-1` modifier is represented as target `+1`, and that spell resistance, mind blank,
  no-charm equipment, charm immunity, existing charm, NPC status, and immortality prevent charm.
- After a failed save, verify the PC follows the Erinyes with charm state and sees the source
  success message. The room must see the matching helplessness message. A successful save must
  show the source resistance line without creating follower or deadline state.
- While charmed, verify only `score`, `tell`, `shout`, `look`, `help`, `who`, `weather`, `save`,
  `quit`, `time`, `toggle`, `ooc`, `commands`, `attributes`, and `petition` pass. Every other
  command must be stopped with both source restraint lines.
- Advance the deadline through its one-to-four-MUD-hour range. If the Erinyes is fighting or a new
  charm attempt occurs when the deadline is due, execution must defer one MUD hour. Otherwise it
  must approach the first charmed follower, emit the murderous-grin and heart-ripping messages,
  kill that follower, and schedule the next follower one hour later rather than killing all at
  once.
- Recheck planar Succubus charm saves. Their source `-2` modifier must now be target `+2`; this is
  the correct translation between source lower-is-better and target higher-is-better save APIs.

This lifecycle is production-tested and reconciled, but the current five-zone pilot contains no
Trahern Erinyes. Exercise it only after a Phase 7 stage supplies mobile 2020246 and its surrounding
package; do not substitute the distinct planar Succubus identity.

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
  binding count. Of 1,813 discovered candidates, 92 are source-preprocessor exclusions;
  of the 1,721 active direct bindings, 1,598 are resolved and 123 remain. Of 795 distinct
  direct source handlers, 690 are resolved and 105 remain. Of 848 `ACT_SPEC` records, 819
  are resolved and 29 remain. The automatic race procedures
  are complete and the Hulburg subset is exposed above. The current five-zone pilot
  still has no selected source example from the newly shared guild, janitor, pet-shop,
  receptionist, corpse-devourer, poison-bite, thief, breath, or conjured-death families;
  it also has no selected `home_reset` room, `magic_pool` or `floating_pool` object,
  or `autoDistributor` room. Those additions remain automated evidence rather than
  manual-test claims for this bundle.
- Package-wide conversion, repair, balance review, and acceptance bundles are Phase 7.
- Development-world application and final operational documentation are Phase 8.

## Reporting a Manual Finding

Record the package, target zone, room/mobile/object/quest VNUM, command or action,
expected source behavior, observed target behavior, and relevant server log lines. Note
whether the issue reproduces after a forced zone reset. Do not repair staged files by
hand; fixes belong in the deterministic converter, runtime adapter, or explicit action
ledger so the bundle remains reproducible.
