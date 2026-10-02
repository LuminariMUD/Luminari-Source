# Vessel guide notes (S9)

S9's play record, from which S10 writes the player guide. Each chapter lists its screenshots
(files in `guide-screenshots/`), each with the character, the exact commands typed, what it
shows, and what had to be true first; then the prices, timings, refusals and tips met on the
way. The plan and the defects found are in
[vessels-ships.md](vessels-ships.md#phase-9-s9-progress).

Setup for every chapter: the branch's build on a fresh reload of the development dump with the
harbor and Vailand content provisioned, played in Luminari Web (1600 x 1000 viewport, Comfort
layout). Vesselmate is a level 1 mortal; Brinewick, the second captain, a new level 1 mortal on
its own account. Staging by the staff character Kohdee is noted where a chapter needed it and
never appears in a player's screenshot.

## 1. Finding a ship

Staging: none for the screenshots; Vesselmate was level 1 in the tutorial (room 14100). Before
chapter 2 Kohdee advanced him to level 16, set his gold to 60,000, gave him a lantern (it was
night) and transferred him to the Testing Dock.

- `01-finding-help-vessels.png` (Vesselmate, `help vessels`): page 1 of 6 of the navigation and
  boarding entry; the keyword list shows every command the entry covers. Help longer than a page
  opens the pager: Return for the next page, `q` to quit, a number to jump.
- `02-finding-shipbrowse.png` (Vesselmate, `shipbrowse`): the shipwright's catalog. `shipbrowse`
  works anywhere; buying needs a dock.

The catalog on the development world (prices in gold, Lvl is the level needed to take her out of
port, not to buy her):

| ID | Class | Speed | Armor | Price | Lvl | Name |
| -: | -- | -: | -: | -: | -: | -- |
| 18 | Raft | 10 | 8 | 333 | 1 | Sablebranch Raft |
| 19 | Boat | 10 | 13 | 594 | 1 | Sablebranch Riverboat |
| 20 | Submarine | 10 | 84 | 57,500 | 23 | Starfall Bathyscaphe |
| 21 | Airship | 25 | 63 | 74,455 | 24 | Aetherwind Courier |
| 26 | Ship | 12 | 66 | 7,200 | 16 | Starfall Survey Ship |
| 27 | Warship | 15 | 95 | 41,293 | 22 | Starfall Bastion |
| 28 | Transport | 8 | 110 | 21,200 | 21 | Sablebranch Grand Freighter |
| 29 | Magical Vessel | 15 | 153 | 146,571 | 25 | Liminal Wayfarer |

Help met on the way: `help vessels` (navigation and boarding), `help shipbrowse` (ownership:
browse, buy, trade-in, christen, customize, deed, permits, crew, summon, hull levels, loss and
insurance; 4 pages). Hull levels from that help: 1 for rafts and boats, 16 ships, 21 transports,
22 warships, 23 submarines, 24 airships, 25 magical hulls, unless the design sets its own.
Anyone may own a hull or ride aboard one. Ownership limit: three hulls per captain (wrecks and
hulls under summons count).

Tips: `nohint` turns off the in-game hints that otherwise interleave with ship output. At night
the wilderness is dark; hold a light (`hold lantern`) to see the dock and the sea map.

Defects found here and fixed: `help ships` opened a 2014 ferry entry instead of the vessel help
(pending, see the progress log); the catalog's Class column overflowed on "Magical Vessel".

## 2. Buying and knowing her

Staging: as above; Vesselmate at the Testing Dock (-66, 92), the west stop of the harbor ferry,
with 60,000 gold.

- `03-buying-dock.png` (Vesselmate, `look` at the Testing Dock): a `[Sea Port]` room on the
  wilderness map, with other hulls moored. A dock is any room where `shipbuy` works.
- `04-buying-shipbuy.png` (Vesselmate, `shipbuy 26`): "The shipwrights hand over Starfall Survey
  Ship, moored here. You pay 7200 gold coins. Fair winds, captain - board her and christen her
  with 'shipchristen <name>'." The new hull is moored in the room, berthed, with her class
  armament (a ship: one Medium Ballista on the bow).
- `05-buying-christen.png` (Vesselmate, aboard: `shipchristen Sea Wren`, `shipcustomize paint a deep green hull with a white stripe`, `shipcustomize figurehead a carved wren with outspread wings`, `shipcustomize show`): "By her owner's word, this vessel is christened Sea Wren!" The
  first christening is free; later ones cost a tenth of her value. Both commands work only
  aboard.
- `06-buying-ship-rooms.png` (Vesselmate, `ship_rooms` in the Crew Quarters): a ship has four
  rooms: Sea Wren's Bridge, Crew Quarters (the entrance, where `board` puts you), Main Cargo Hold,
  Main Deck. The minimap shows the layout.
- `07-buying-shipstatus.png` (Vesselmate, `shipstatus` on the bridge, Ship tab open): position,
  terrain Seaport, heading 0, speed 0 / 6 (her 12 halved in port), moorings Berthed, armor per
  side (fore 53, port 66, starboard 66, rear 33) over structure (26/33/33/16), sails 110, rudder
  20, crew stamina 500, repair stores 200, slot 0 Medium Ballista (fore) 50/50 rounds. The Ship
  tab shows the same live, plus the contacts in sight (here the hulls moored at the dock, at range
  0.0).
- `08-buying-shipcrew.png` (Vesselmate, `shipcrew`): owner, renown 0, no pilot, no permits, the
  four crew positions (sailmaster, gunner, bosun, quartermaster) unfilled.

Commands: `board <keyword>` takes any word of the hull's name (`board survey` before the
christening, `board wren` after); `board sea` was refused because "Sea Wren" was not her name
yet. `disembark` on a berthed hull: "You step off the vessel onto the dock." From the dock, `look`
shows her paint and figurehead: "Sea Wren is moored here, painted a deep green hull with a white
stripe and bearing a carved wren with outspread wings as a figurehead." (Tip: the paint text
follows "painted", so a color phrase reads best.)

Refusals met: `shipchristen` and `shipcustomize` on the dock: "You must be aboard your ship to
christen her." / "...to customize her."

Defects found here and fixed: the purchase showed builders' details (fleet slot, room numbers);
every hull glowed and hummed; the bridge kept the old name after christening; the generated
interior's exits contradicted each other (the minimap drew three "you are here" markers); the
purchase line told a new owner to christen her without saying to board first.

## 3. Crew, weapons and refits

Staging: none (Vesselmate's own gold). All of these need the hull berthed (`shiphire` needs her in
port); `shipdismiss` works anywhere aboard. One help entry covers them all: `help shiphire` (also
`help shipweapon`, `help shipupgrade`, `help ship-crew`, `help ship-refit`; 4 pages).

- `09-crew-shipcrew.png` (Vesselmate, `shiphire` then `shiphire <position> green` four times,
  `shipcrew`): the four hands hired green, each with its experience and the threshold for able
  (sailmaster 200 of 800, gunner 250 of 1000, bosun 220 of 900, quartermaster 200 of 800).
- `10-crew-shipupgrade.png` (Vesselmate, `shipupgrade plating`, `shipupgrade rigging`,
  `shipupgrade`): the refit list with two installed.
- `11-crew-shipstatus-armed.png` (Vesselmate, `shipstatus`, Ship tab): after the refit: armor
  up a fifth (fore 63, port 79, starboard 79, rear 39), crew stamina 900 (500 plus 100 per hand),
  "Shipwrights: 2210 seconds of work left", and the weapons: slot 0 Medium Ballista (fore), slots
  1 and 2 Large Ballista (port, starboard), slot 3 Ram.

Crew (`shiphire` with no arguments lists the one-time prices; crew draw no wages):

| Position | Does | Green | Able (renown) | Veteran (renown) |
| -- | -- | -: | -: | -: |
| sailmaster | speed handling under way | 1,600 | 6,000 (540) | 15,000 (1,350) |
| gunner | gunnery accuracy | 2,400 | 8,000 (700) | 18,000 (1,640) |
| bosun | faster repairs | 2,000 | 7,000 (640) | 16,000 (1,480) |
| quartermaster | more cargo capacity | 1,200 | 4,500 (540) | 11,000 (1,350) |

- `shiphire sailmaster able` on a new hull: "No able sailmaster will sign on with a hull of less
  than 540 renown, and Sea Wren has 0. Hire a green hand and let the sea promote them."
- `shiphire sailmaster green`: "You sign on a green sailmaster for 1600 gold." and, to everyone
  aboard, "A green sailmaster reports aboard Sea Wren."
- `shipdismiss quartermaster`: "The quartermaster gathers their kit and goes ashore. You dismiss
  the quartermaster." No refund; rehiring costs the full price again.

Weapons (`shipweapon list` on a Ship; prices in gold, weight, rounds, range in rooms, damage,
reload, arcs; capital weapons need the renown shown or a veteran gunner, one per hull):

| # | Weapon | Gold | Wt | Ammo | Range | Damage | Reload | Arcs |
| -: | -- | -: | -: | -: | -- | -- | -- | -- |
| 1 | Small Ballista | 100 | 3 | 60 | 0-8 | 2-4 | 17s | all |
| 2 | Medium Ballista | 200 | 6 | 50 | 0-10 | 4-6 | 17s | all |
| 3 | Large Ballista | 1000 | 10 | 30 | 0-12 | 6-9 | 17s | all |
| 4 | Small Catapult | 1000 | 10 | 30 | 4-15 | 4x 2-3 | 17s | ends |
| 5 | Medium Catapult | 1600 | 13 | 20 | 5-20 | 5x 2-4 | 17s | ends |
| 6 | Large Catapult | 2400 | 17 | 12 | 6-25 | 6x 2-5 | 17s | ends |
| 7 | Heavy Ballista | 2000 | 15 | 6 | 0-4 | 15-22 | 17s | beams |
| 8 | Light Beamcannon | 8000 | 7 | 40 | 0-20 | 16 to 4 | 25s | all, capital 1600 |
| 9 | Heavy Beamcannon | 10000 | 9 | 40 | 0-23 | 22 to 5 | 25s | all, capital 1800 (not on a Ship) |
| 10 | Mind Blast Cannon | 8000 | 5 | 50 | 0-20 | stun | 25s | all, capital 1700 |
| 11 | Fragmentation Cannon | 10000 | 7 | 20 | 0-16 | 5x 4-6 | 25s | ends, capital 1900 |
| 12 | Long Tom Catapult | 10000 | 9 | 6 | 12-32 | 8x 3-6 | 25s | ends, capital 2000 (not on a Ship) |

The list ends with each arc's weapons and weight against its mounts and cap (a Ship: fore 1
mount, 17 weight; port and starboard 3 mounts, 26; rear 1 mount, 17) and the fit-out weight (6 of
100 new).

- `shipweapon buy 3 starboard`: "The shipwrights mount a Large Ballista on the starboard arc of
  Sea Wren in slot 1 for 1000 gold, loaded with 30 rounds. She cannot sail for 750 seconds while
  they work." Work adds up: the second Large Ballista made it 1497 seconds, the ram 2072. The
  shipwrights take 75 seconds per point of weight; plan refits well before a voyage.
- `shipweapon sell 4`: "The shipwrights take the Small Ballista out of slot 4 and pay you 90
  gold." (90% back; 10% if damaged). Selling does not give back the work time it added.
- `shipweapon swap 1 2`: "Slots 1 and 2 trade places."
- `shipequip list`: Ram 400 gold (2 per point of hull weight), weight 8; Neutral Colors free and
  weightless. `shipequip buy ram`, `shipequip buy colors`, `shipequip sell colors` ("The
  shipwrights remove the Neutral Colors and pay you 0 gold.").
- `shipupgrade`: plating (+20% armor on all sides), rigging (+10% maximum speed), hold (+25% cargo
  capacity), reinforcement (+20% structure), each 1,600 gold on a Ship (a fifth of the class
  price), each once. Refits add no shipwright time.
- `shiprearm all` with full racks: "There is nothing to rearm." (2 gold a round, 75 seconds per
  weapon; played again after the fight in chapter 8.)

Money spent in chapters 2-3: hull 7,200; crew 8,400 (quartermaster twice); two Large Ballistas
2,000; ram 400; small ballista 100 (90 back); plating and rigging 3,200.

## 6. Passage on public ships

Played while the shipwrights worked on chapter 3's refit (about 37 minutes). Staging: for the
Vailand merchant, Kohdee moved Vesselmate to Vailand Central Port and, after a server restart,
back aboard her.

The harbor ferry (Harbor Sandbox Ferry, contact ID AF) runs a loop every MUD hour: the Testing
Dock (-66, 92, the west stop) to a channel turn (-64, 82) and the Harbor Sandbox East Dock (-62,
82), waiting 15 seconds at each dock, then 30 seconds casting off; a crossing takes about a
minute at her speed 8. The fare is 10 gold a boarding. (The development world runs three
"Harbor Sandbox Merchant" hulls on the same loop and fare; they are leftovers of the test
content.)

- `12-passage-ferry-berthed.png` (Vesselmate, `look` at the Testing Dock): the ferry and the
  merchants moored beside the Sea Wren.
- `13-passage-lookout.png` (Vesselmate, `board ferry`, `south` to the bridge, `lookout` under
  way): position, heading, weather and visibility, the water column, the eight compass sectors
  to the horizon, and the vessels in sight nearest first with their contact IDs, condition,
  range, bearing and height difference; a customized hull's paint and figurehead show under her
  line. (Retake pending: this capture predates the contact-ID fix and shows slot numbers.)
- `14-passage-ferry-status.png` (Vesselmate, `shipstatus` at the east dock, Ship tab): a
  passenger can read the status: "Moorings: Casting off (25 seconds)" after the stop.
- `15-passage-east-dock.png` (Vesselmate, `north`, `disembark` at the east dock): "You step off
  the vessel onto the dock." into the Harbor Sandbox East Dock.
- `16-passage-vailand-port.png` (Vesselmate, `look` at Vailand Central Port): the Vailand
  Ironwind Trader moored; she carries passengers free.
- `17-passage-merchant-status.png` (Vesselmate aboard the trader, `shipstatus`, Ship tab): under
  way on her route at speed 9 of 9.
- `18-passage-merchant-lookout.png` (Vesselmate, `lookout`): open water in the Vailand Passage;
  "At sea: A ship is running near full speed under overcast skies. The broad Vailand Passage
  draws a dark blue road between the island coasts."

Messages: boarding a ferry: "The purser collects 10 gold for passage aboard Harbor Sandbox
Ferry." then "You board the ship."; without the gold: "Passage aboard <ship> costs <fare> gold;
you have <gold>." Crossing into the harbor waters: "The charts mark our crossing into Harbor
Sandbox Free Seas (free seas), under Free Captains' Compact authority." The pilot: "The harbor
ferrymaster studies the route from the helm." Departure: "The crew begins casting off." then
"The first officer reports Harbor Sandbox Ferry ready to get under way." `showschedule` aboard
shows the next departure (MUD hour), the fare, and that a pilot is assigned.

Refusals: `disembark` under way: "You can't disembark while the vessel is moving!" and "Wait
until she stops." (the helm is told to bring her to a stop). A passenger cannot steer or stop a
public hull: her NPC pilot holds the helm.

The Vailand Ironwind Trader runs the Vailand Iron Passage: Vailand North Port (-599, 455) and
Vailand Central Port (-467, 204), 30 seconds at each, past the Blackwake anchorage; at speed 9 the
passage between the ports is roughly 20 minutes.

Tips: stay aboard through a stop and the ferry carries you round the loop again; step off while
she is berthed or casting off. Passengers may walk to the bridge to watch from the lookout.

## 4. Sailing

Staging: none for play. (A server restart for a fix left the Sea Wren at sea at (-78, 63) in
shoal water, where she had sailed unattended on her last orders; Kohdee put Vesselmate back
aboard and the chapter sails her home.) All helm commands work only on the bridge, by the owner
or a permitted helmsman.

- `20-sailing-tactical.png` (Vesselmate, `tactical` at (-78, 63)): the 21-by-21 chart around
  her: `@` the ship, `.` shoal, `:` beach, `^` land, `o` and `O` the 5- and 10-room range rings,
  `+` region edges; position, heading, weather and visibility, hull; then the legend, the charted
  regions, and the contact roster with IDs, state, range, bearing and height.
- `21-sailing-contacts-seastate.png` (Vesselmate, `contacts`, `seastate`): contacts nearest first
  with ID, range, bearing, compass direction and the arc of her hull each lies off; `seastate`:
  water (Water (Swim)), depth ("shallow - watch your draft"), weather (fair), visibility (55
  units, lookout posted), hull, and the waters' law ("Unnamed open waters (standard maritime
  law)").
- `22-sailing-lookout.png` (Vesselmate, `lookout` berthed at the Testing Dock): the lookout view
  with the same contact IDs and ranges as `contacts`.
- `23-sailing-dock-alongside.png` (Vesselmate, `dock T1` in port): "Docking complete with Test
  Vessel." `shipstatus` reads "Moorings: Made fast alongside another vessel". A gangway joins the
  two: from the Sea Wren's Crew Quarters, north leads to the Test Vessel's bridge. `undock`:
  "Undocking complete. You have successfully undocked from Test Vessel."
- `24-sailing-anchored.png` (Vesselmate, `shipstatus` at anchor one room off the dock, Ship tab).
- `25-sailing-dockfees.png` (Vesselmate, `dockfees` after berthing): "Sea Wren owes 25 gold for
  its berth at Testing Dock. This is a public-port charge; the payment leaves the economy. Use
  'dockfees pay' to settle the balance before departure."

The voyage, in order:

01. `undock` (berthed): "The crew begins casting off." 30 seconds later: "The first officer
    reports Sea Wren ready to get under way." Orders given meanwhile are refused: "Sea Wren is
    still casting off (27 seconds)."
02. `heading 23`: "Heading set to 23 degrees (NE)." She comes about at her turn rate; stopped she
    warps round a degree a tick (about 2 degrees a second). `shipstatus` shows "Heading: 151
    degrees (coming about to 23)" meanwhile.
03. `speed 8` in port: "Under present conditions she can make only 6." (port halves her). At sea
    with a green sailmaster and rigging she makes 8; `speed 6`: "Half speed. Speed set to 6."
    Speed 6 covers a room about every 7.5 seconds: 31 rooms took four minutes.
04. Ordering speed while her bow points at shoal or land: "The ship cannot navigate this terrain!
    It requires deep water to sail. The crew brings her up short." A ship needs deep water; stop
    (`speed 0`), come about, then order speed.
05. On the way: the narrative ("The ship's timbers work with the sea. It holds a steady pace.
    Clear light runs cleanly to the horizon.") and the waters she enters ("The charts mark our
    crossing into Harbor Sandbox Territorial Waters (territorial waters), under Harbor Admiralty
    authority.").
06. `speed 0` near the port: "All stop! The crew takes in sail." A hull that comes to rest in a
    port berths: "Lines go ashore; Sea Wren is made fast at the berth." and "The harbor master
    records a 25-gold berthing fee. Use 'dockfees pay' before departure."
07. `dockfees pay`: "You settle 25 gold in dock fees. Sea Wren may now depart." She cannot leave
    until it is paid; each berthing in a port is charged once.
08. `setsail south` (under way, speed 6 or less): "The vessel maneuvers south. Current position:
    (-66, 91, 0)"; she stops on the new heading. The crew needs 5 seconds between maneuvers.
    `setsail north` into the port berths her (and the fee is assessed again).
09. `anchor` (stopped, off a berth): "Sea Wren drops anchor." She takes no speed order or
    maneuver until `undock`: "The crew begins weighing anchor." (13 seconds).
10. `shiptalk All hands, we sail for the Testing Dock.`: "[Captain's channel - Sea Wren]
    Vesselmate: All hands, we sail for the Testing Dock." heard in every room aboard.

Tips: a hull keeps sailing on her last orders; bring her to rest before leaving the bridge. Check
`seastate` for depth before running inshore. Pay dock fees before ordering `undock`.

Defects found here and fixed: the chart lost its land cells and left the rest of the output
orange in Luminari Web (the client read the `^` land glyph as a color code); orders given while
casting off were told to order `undock`; lookout's ranges disagreed with contacts' for a hull
stopped part-way through a room; dock fees named the port by room number.

## 5. Routes, autopilot and schedules

Staging: an NPC to pilot her. Any NPC standing on the bridge can be assigned (a player brings a
follower or hireling); Kohdee loaded the harbor ferrymaster (mob 70001) onto the bridge for
`assignpilot` and purged it afterwards. Everything else is the owner's own work, on the bridge.

- `26-routes-autopilot-on.png` (Vesselmate, `setroute wren_run`, `autopilot on`, `autopilot status`): "Route 'wren_run' assigned to autopilot (2 waypoints)." "Autopilot engaged on route
  'wren_run'." The status: Traveling, Waypoint 1 of 2, the current target and its distance,
  position, movement steps, arrivals and completions.
- `27-routes-schedule.png` (Vesselmate, `setschedule wren_run 2`, `showschedule`): "Schedule
  set: Route 'wren_run' every 2 MUD hours. Next departure: MUD hour 10. Passenger fare: free."
  and the schedule card (Status: Active; "Pilot: None (silent departures)").
- `28-routes-pilot-arrives.png` (Vesselmate, `assignpilot ferrymaster` with the route
  `wren_home_run` set): the pilot casts off and sails her home, announcing each waypoint: "The
  harbor ferrymaster announces, 'Arriving at wren_home!'"; she berths, the fee is assessed, and
  `autopilot status` reads "State: Route Complete", Route Completions 1.

Commands, in the order played:

- `setwaypoint wren_home` (berthed): "Waypoint 'wren_home' created at position (-66.0, 92.0,
  0.0)." A waypoint is the ship's present position; names are letters, numbers, `_` and `-`.
- `listwaypoints`: every waypoint in the world, with ID and coordinates (17 here, including the
  ferry's `harbor_west_dock`, `harbor_channel_turn` and `harbor_east_dock`); any captain may use
  any of them in a route.
- `createroute wren_run`: "Route 'wren_run' created (ID: 6)."; `addtoroute wren_run harbor_channel_turn`: "Waypoint 'harbor_channel_turn' added to route 'wren_run' at position
  1." Up to 20 waypoints a route.
- `listroutes`: ID, name, waypoints, loop and active, for every route (the ferry's
  `harbor_ferry_loop` and the `Vailand Iron Passage` among them).
- `setroute`, `autopilot on`: the autopilot casts off from a berth (30 seconds), sails at full
  speed when no speed is ordered (steerage speed while coming about), and stops at the last
  waypoint of a one-way route; she berths if it is a port. `autopilot pause` holds her,
  `autopilot on` resumes, `autopilot off`: "Autopilot disengaged."
- The autopilot will not leave a port with dock fees unpaid: "Autopilot pauses: the harbor
  requires 25 gold in dock fees before departure." Pay with `dockfees pay` and `autopilot on`.
- `setwaypoint scratch_mark` / `delwaypoint scratch_mark` ("Waypoint 'scratch_mark' deleted."),
  `createroute scratch_run` / `delroute scratch_run` ("Route 'scratch_run' deleted.").
- `setschedule <route> <hours> [fare]`: departures every 1-24 MUD hours (a MUD hour is about 75
  seconds); a fare applies only to unowned public hulls. `clearschedule`: "Vessel schedule has
  been cleared." `showschedule` with none: "This vessel has no schedule configured."
- `assignpilot <npc>`: "You assign the harbor ferrymaster as the vessel's pilot." To all aboard:
  "The harbor ferrymaster has been assigned as the vessel's pilot." With a route set the pilot
  engages at once: "The harbor ferrymaster takes the helm and engages autopilot."
  `unassignpilot`: "You relieve the harbor ferrymaster of pilot duties."
- Engaging the autopilot, assigning a pilot and setting a schedule need the hull's level (16 for
  a ship).

The run: east dock (-62, 82) to the channel turn (-64, 82) and home to the Testing Dock (-66,
92), 14 rooms in about 90 seconds after casting off.

Defects found here and fixed: a one-way route ending in a port paused "before departure"
instead of completing (the unpaid berthing fee was checked before the arrival); the pilot's lines
began in lower case and each autopilot announcement left a blank line; ADDTOROUTE's help example
numbered positions from 0.

## 7. Trade and freight

Staging: none for play; one database update expired the Testing Dock's freight board after a
fix so it would regenerate (boards otherwise refresh hourly). All of these work aboard, moored at
a port.

- `29-trade-market.png` (Vesselmate, `market` at the Testing Dock): each commodity's weight per
  unit, buying and selling price, and local supply (scarce, steady, glutted); contraband marked;
  the hold's use ("Hold: 0 of 13200 lbs used.").
- `30-trade-contracts.png` (Vesselmate, `contracts` at the Harbor Sandbox East Dock): the freight
  board: ID, cargo, quantity, the bond the shipper asks, the payout, and the destination.
- `31-trade-manifest.png` (Vesselmate, `cargomanifest` after loading): each lot with units and
  weight, and the hold's use.
- `32-trade-deliver-sell.png` (Vesselmate, at the east dock: `contractdeliver 12`, `cargosell grain all`, `cargosell cloth all`): "Freight delivered. The consignee pays 370 gold." and the
  sales. (Retake pending: this run predates the freight bond.)
- `33-trade-delivered.png` (Vesselmate, `contractdeliver 18` at the Testing Dock): the delivery
  that repaid the 350-gold bond with 370.

The market at the Testing Dock (gold): timber 6/5, grain 10/8 (scarce), salt 16/13, cloth 33/28
(scarce), iron 25/21, wine 40/34, spice 88/74, silk 95/80, gemstones 330/280 (scarce); forbidden
tomes, rare poisons and dragon eggs not sold here (sell 248, 274, 405). The east dock stocks
forbidden tomes (190 to buy).

Played:

- `cargobuy grain 50`: "You load 50 units of grain for 562 gold (11 average each)." Big lots
  push the price as they go; the average is reported. `cargobuy cloth 40`: 1,410 gold.
- At the east dock `cargosell grain all`: 344 gold (6 each); `cargosell cloth all`: 967 (24
  each). The two harbor docks price alike, so buying in one and selling in the other lost money:
  profit needs ports with different supply, which means longer voyages.
- `contractaccept 18` (wine, 10 units, to the Testing Dock): "Contract 18 accepted: you post a
  350-gold bond, 10 units are loaded, and 370 gold is paid on delivery to Testing Dock." Without
  the gold: "The shipper asks a 140-gold bond for that freight; you have 100."
- `contracts` lists your active contracts under the board ("Your active contracts: 14 salt 75
  5100 deliver to Selerish Slateharbor Sea Port").
- `contractabandon 14`: "You abandon contract 14. The freight your bond paid for remains in your
  hold." The job goes back on the board.
- `contractdeliver 18` at the destination: "Freight delivered. The consignee pays 370 gold."
  "Dockhands unload 10 units of freight from Sea Wren."
- Sale modifiers: under neutral colors "The merchants pay a tenth less to a hull under neutral
  colors." (10 salt: 50 gold before, 45 under colors). A seadog captain gets a tenth more, a
  warship four tenths less. Colors cannot be struck while cargo is aboard: "Her neutral colors
  stay up while she has cargo aboard."
- The board's long hauls (100 units to Koorvik, North Vailand, Southwest Quechian or Central
  Vailand, payouts 10,000-18,600 against bonds of 600-3,000) are where freight pays; the two harbor
  docks are 10 rooms apart and pay a 20-gold premium.

Defects found here and fixed: the board offered "10 grain to an unknown port" (a market once read
aboard a hull had left rows for an interior room); trade and freight keyed the port by the hull
object's room even when she lay in port by her coordinates; accepting and abandoning a contract
handed out its freight for nothing, again and again (the freight now takes a bond); "fit Sea Wren
with a Neutral Colors".
