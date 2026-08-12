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

- Twelve converted rooms use `RoL Mage Guild Room`, `RoL Thief Guild Room`, `RoL Warrior
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
- Confirm these VNUM-owned profiles coexist with any other converted direct or automatic
  behavior on the same mobile and do not require a new mobile flag or second persisted
  SpecProc. An unrelated mobile must retain the ordinary target death/corpse path.

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

- Build dependency-complete source-profile stages. Confirm the following 100 mobiles use
  `RoL Source Periodic` with `MOB_SPEC`: Bloodstone 2007100-2007109, 2007113-2007129,
  2007142-2007144, 2007147, 2007152-2007154, 2007156, 2007160-2007162,
  2007164-2007167, 2007170, 2007172-2007180,
  2007189, 2007191-2007195, 2007199-2007203, 2007205-2007206, 2007209, 2007220,
  2007221, 2007308, 2007311, 2007314, 2007317, and 2007321-2007326; Fun 2001230;
  Mobile 2003069; Realm 2014048; Icecrag 2097000-2097002,
  2097005, 2097007-2097008, 2097011, 2097014, 2097016, 2097021, 2097023, and
  2097028, 2097033; Menden 2088806; Tower of Sorcery 2015901; Waterdeep 2003212;
  and Lavatubes 2012000, 2012002, and 2012003.
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

- Build a dependency-complete Waterdeep stage and confirm these 26 mobiles use
  `RoL Stateful Periodic` with `MOB_SPEC`: 2002823, 2003020-2003023, 2003039, 2005315,
  2005503-2005504, 2005507-2005508, 2005510, 2005513-2005521, 2005525, 2005530,
  2005533-2005534, 2005538, and 2005540.
- Confirm seven additional Waterdeep guild guards use `RoL Guild Guard` with `MOB_SPEC`
  while drawing from the same generated state-profile table: 2002824, 2003025-2003027,
  2005505, 2005511, and 2005535.
- While awake and standing but not fighting, observe representative profiles across repeated
  activity pulses. They should use their source idle speech and room actions. Most roll two-d5;
  Selune dancer 2005519 rolls two-d7. Sitting and sleeping representatives remain quiet.
- Start combat with representative mobiles. Each should switch to its one-d4 fighting table
  instead of emitting idle text. This intentionally makes the explicitly authored combat branch
  usable where the source tested standing position first. Guildmaster 2003020 has no fighting
  table and must remain quiet in combat.
- For commoner 2003039, confirm idle outcomes include the ordered two-line missing-child speech,
  while fighting outcomes include calls for help and other combat reactions. Unrelated mobiles
  must not receive any of these identity-keyed profiles.
- Confirm converted rogue 2005509 has no named procedure from `rogue_one`: its source callback
  registered only for `NPC_HIT` but returned whenever that event supplied its victim, so adapting
  the unreachable body would invent behavior.

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
  of the 1,147 active direct bindings, 594 are resolved and 553 remain. Of 562 distinct
  source handlers, 99 are resolved and 463 remain. Of 848 `ACT_SPEC` records, 568 are
  resolved and 280 remain. The automatic race procedures
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
