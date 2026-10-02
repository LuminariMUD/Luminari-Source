# Vessel guide notes (S9)

S9's play record, from which S10 wrote the
[Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md). Each chapter lists its screenshots
(files now in `docs/guides/vessel-guide/`, some cropped or redacted by S10; 21 and 72, copies
of 20 and 71, and 42, which shows a refused shot rather than the group forfeit, were removed),
each with the character, the exact commands typed, what it shows, and what had to be true first;
then the prices, timings, refusals and tips met on the way. The notes stay as S9 recorded them: where S10
found a note contradicted by the code, the guide states the code's rule. The plan and the
defects found are in [the history](vessels-ships-history.md#phase-9-s9-progress).

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
- `13-passage-lookout.png` (retaken after the contact-ID fix, from the Sea Wren under way at
  speed 2 off the harbor; a ferry passenger's `lookout` reads the same): "LOOKOUT VIEW FROM Sea
  Wren", her paint and figurehead, position, heading, weather and visibility, the water column,
  the eight compass sectors to the horizon, and the vessels in sight nearest first with their
  contact IDs, condition, range, bearing and height difference ("[AP] Kestrel sound 0.6u E (86
  deg), dz +0"), then "2 more contacts are visible; use TACTICAL for the full roster."
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
- `32-trade-deliver-sell.png` (retaken with the freight bond: Vesselmate accepted contract 24 at
  the Testing Dock - "Contract 24 accepted: you post a 350-gold bond, 10 units are loaded, and
  370 gold is paid on delivery to Harbor Sandbox East Dock." - sailed `wren_run` to the east
  dock, `dockfees pay`, `contractdeliver 24`, `cargosell grain all`): "Freight delivered. The
  consignee pays 370 gold." "Dockhands unload 10 units of freight from Sea Wren." "You sell 50
  units of grain for 272 gold (5 average each)."
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

## 8. Gunnery against a raider

Staging: with the Sea Wren under way in open water south-west of the harbor ((-82, 79), "Unnamed
open waters"), Kohdee went aboard, ran `vesseldebug raider 0` ("Raider 15 [AP] Corsair Clipper
comes for Sea Wren.") and left. A Corsair Ketch [AO] joined the attack on her own: a raider
ambush. Vesselmate fought with a green gunner, a Medium Ballista fore, Large Ballistas port and
starboard, and a ram.

- `34-gunnery-tactical.png` (Vesselmate, `tactical` after `shiplock AP`, Ship tab): the raiders
  on the chart and in the roster.
- `35-gunnery-fire.png` (Vesselmate, `shipfire port AO` and `shipfire fore AP`): "The guns lock
  onto [AO] Corsair Ketch. The port Large Ballista FIRES at Corsair Ketch! Chance to hit: 95%
  Direct hit on Corsair Ketch! You hit [AO] Corsair Ketch for 6 points on the stern!"
- `36-gunnery-scan.png` (Vesselmate, `shipscan AO`): "[AO] Corsair Ketch, a Ship, heading 11 at
  speed 0, 7.6 rooms off. Armor/structure: fore 0/40 0/20, port 18/50 25/25, starboard 24/50
  18/25, rear 0/25 0/12. Condition: sinking, holed on 2 sides. Weapons: fore Small Catapult, port
  Small Ballista, rear Small Ballista, starboard Small Ballista."

The fight, in order (about ten minutes):

1. The Clipper appeared 40 rooms off the bow ("AP Corsair Clipper 40.1 u 259 deg W fore" in
   `contacts`). `shiplock AP`: "The guns lock onto [AP] Corsair Clipper. The crew scrambles to
   battle stations!"
2. `shipsight` lists each weapon's chance or why it cannot fire: "Corsair Clipper is outside the
   Medium Ballista's 0-10 room band (20.8)." / "The port Large Ballista cannot bear - Corsair
   Clipper lies off your fore arc." (`shipsight` takes a weapon slot, not a contact.)
3. Both raiders closed fast and rammed: "[AP] Corsair Clipper attempts to ram you! Timbers crunch
   and crack as [AP] Corsair Clipper crashes into your ship! The sails are hit for 3 points! The
   port side is hit for 4 points!" A ram slews the lighter hull about: the Sea Wren's heading went
   from 230 to 71. Then grapples: "WARNING: Corsair Ketch throws grappling lines across!" "The
   crew beats off Corsair Ketch's boarders!" Raider fire: "Raider fire from Corsair Ketch splashes
   wide!" or "The bow is hit for 4 points!"
4. Hits knock the crew down: "You can't do that while reclining..." until you `stand`.
5. `shipfire <arc> <contact>` locks and fires every weapon on the arc that bears; "No weapon on
   the starboard arc can fire at Corsair Ketch now. See 'shipsight'." when none does. Each weapon
   reloads in 17 seconds: "The port Large Ballista is reloaded and ready." Each shot spends a
   round (the port ballista went from 30 to 25 rounds). Hits land on the side facing you; a hit
   through the armor can damage a weapon mounted there: "You damage the rear Small Ballista
   aboard Corsair Ketch!"
6. `shipram` (speed 6 or more, locked): "The crew braces to ram [AO] Corsair Ketch!" In shallow
   water at battle stations she would not go on: "The crew keeps her from running aground." "The
   crew stands down from ramming: she has lost way."
7. The raiders' rams holed their own bows, and our stern hits holed the Ketch's rear: two holed
   sides set her sinking (an unowned hull takes 17-25 minutes to go down: time to board and
   plunder). The Clipper, holed at the bow only, lay dead in the water.
8. `shiplock off`: "The guns come off their target; the crew stands down in 180 seconds."

Our damage: armor fore 29/63, port 65/79, starboard 53/79, rear 31/39 (structure untouched),
sails 89/110, rudder 18/20.

Tips: at battle stations a hull keeps off water too shallow for her draft and no harbor admits
her; fight in deep water. Keep the target on a beam: the Large Ballistas on port and starboard
did the work. Stand up after every knock-down before firing.

Defects found here and fixed: a holed raider dead in the water read "sound" (condition now ranks
any holed hull crippled).

## 9. Damage, repair and salvage (at sea)

Played during and after chapter 8's fight. The shipyard repair and salvage are at the end of
this chapter's notes once played.

- Hits land on the side facing the shooter (`The bow is hit for 4 points!`), on the sails (`The sails are hit for 3 points!`), and through the armor into structure and weapons. A side with
  neither armor nor structure is holed; one holed side leaves her dead in the water, two set her
  sinking. `shipscan` and `tactical` show it: "Condition: sinking, holed on 2 sides", `X` on the
  chart. A raider holed at the bow by her own ram lay dead in the water ("crippled").
- `shiprepair` at sea with only armor and light sail damage: "Nothing aboard needs a patch the
  stores can make at sea; armor, and the rest of her, are made good only at a shipyard." The
  crew patches structure (to a tenth), sails and rudder (to two fifths) and damaged weapons from
  the repair stores (200 on a ship); armor only at a shipyard.
- A sunk raider leaves "The shattered wreckage of Corsair Ketch floats here." in the water room.

## 10. Boarding and taking a prize

Staging: Kohdee spawned an unowned Starfall Survey Ship with nobody aboard beside the Sea Wren
(`vedit spawnpublic 26`), and launched a second tier-0 raider; for the plunder he removed the
raider's four deckhands after Vesselmate had died to them twice, and restored him once.

- `37-prize-claimship.png` (Vesselmate on the abandoned hull's bridge, `claimship`): "Vesselmate
  seizes control of Starfall Survey Ship! You take the helm - Starfall Survey Ship is yours
  now." `shipcrew` lists him as owner; he christened her Gull Prize (free while she bears her
  design's name).
- `38-prize-strongbox.png` (Vesselmate in the raider's Main Cargo Hold, `unlock chest`, `open chest`, `look in chest`, `get all chest`): "*Click*" ... "You get a big pile of gold coins from
  an iron-bound strongbox. There were 1048 coins."

`board_hostile <vessel>`, from your own deck to a hull nearby (the raider was 1.5 rooms off):

- The grapple contest, then the crossing contest, each your Boarding plus d20 against the best
  defender's Boarding plus d20 plus the hull's modifier; ties go to the defender. "Grapple
  contest: Boarding 4 + d20 19 = 23; the corsair captain Boarding 9 + d20 11 + vessel modifier
  (+0) = 20. SUCCESS" "Your grappling lines bite home; you commit to the crossing!" then
  "Crossing contest: ... FAILURE" "The defenders drive you back and the grappling lines are
  cut!"
- A bad crossing throws you in: "The grappling line snaps taut and throws you into the water!"
  "Swimming: Athletics Skill (4) + d20 roll (12) = Total (16) vs. DC (15)" "You manage to stay
  afloat." You come up beside your own hull; `board <hull>` from the water climbs back aboard.
- An empty hull still resists (her motion): with nobody aboard, "Starfall Survey Ship Boarding 0
  - d20 17". From the water you can simply `board` a hull moored in the same room.
- Boarding is a class ability trained in the `study` menu (0 Skills, then type `boarding` once
  per rank; `quit`, `q`, `y` to save): Vesselmate went from 4 to 23 with his unspent trains.
- Once across you fight the crew. A tier-0 Corsair Ketch carries a level-12 corsair captain and
  four level-8 deckhands, some of them casters ("A corsair deckhand directs a ray of negative
  energy at you"); Vesselmate (level 16, his armor still in his pack the first time) died twice
  boarding her: "You are dead! Sorry..." and woke at the tutorial beach. Wear your armor (`wear all`, `wield sword`) and bring friends.
- `kill captain` on the raider's bridge: "The corsair captain is dead! R.I.P." His corpse holds
  90 gold and "a brass strongbox key"; the strongbox is lashed in the hold.
- `plunder` on the bridge with nobody conscious left: the raider's hold was empty ("Corsair
  Ketch's hold is empty: there is nothing to take." after the fix).
- `claimship` on a sinking raider: "Corsair Ketch is going down - there is nothing left to
  claim." Raiders can never be claimed; an abandoned hull at sea can.

D6 prize rules met: a hull is a prize when holed, unable to move, under struck colors, or
abandoned at sea with nobody conscious aboard; hostile boarding needs her at speed 3 or less or
beaten; a sinking hull cannot be claimed.

Defects found here and fixed: a boarder thrown into the sea came up at the harbor, 30 rooms
away (people put off a hull used their stale coordinates; the same bug landed a ferry passenger
at the wrong dock in chapter 6); a failed boarding of an empty hull spoke of defenders; plunder's
empty-handed message did not say whether the prize's hold was empty or her own was full; the
raider's key answered to "strongbox", so `unlock strongbox` found the key in the boarder's pack.

## 11. Two captains

Staging: Kohdee advanced Brinewick to level 16 and set his gold to 60,000 at the Testing Dock;
after server restarts he returned both captains to their bridges. Brinewick bought a Starfall
Survey Ship and christened her Kestrel.

- `40-captains-permit-deed.png` (Vesselmate, `shippermit brinewick` ... `shiprevoke brinewick`,
  `shipdeed brinewick`): "Brinewick is now cleared to take the helm of Sea Wren." / "...no
  longer cleared for the helm of Sea Wren." / "You sign over Sea Wren to Brinewick." To all
  aboard: "Sea Wren is under new ownership: Brinewick." Brinewick deeded her straight back.
- `41-captains-colors-struck.png` (Vesselmate, `shipscan AP` after Brinewick's `strikecolors`):
  "Condition: sound, colors struck". Brinewick saw: "Brinewick strikes Kestrel's colors: she
  yields. They fly again when she gets under way or in ten minutes."
- `42-captains-group-forfeit.png` (Vesselmate, `shiplock kestrel`, `shipfire port kestrel` while
  grouped with Brinewick): "[Group] Vesselmate has left the group."
- `43-captains-sinking.png` (Brinewick, `shipstatus` aboard the Kestrel, Ship tab): "Holed:
  starboard side and stern. SINKING: she goes down in about 77 seconds."
- `44-captains-renown.png` (Vesselmate, after she sank): "Sea Wren wins 200 renown for sinking
  Kestrel." "The harbor office delivers 462 gold from 1 vessel settlement. Check your mail for
  the receipt."
- `45-loss-sunk.png` (Brinewick): "The hull gives way - Kestrel is SINKING!" ... "You are thrown
  into the water as the ship goes down!" "A salvage crate of grain (50 units) bobs among the
  waves here." "The wreckage of Kestrel settles into the water, breaking apart." "The harbor
  office delivers 5400 gold from 1 vessel settlement." (Insurance: three quarters of her 7,200.)
- `46-salvage.png` (Vesselmate, `setsail` to the wreck, `shipsalvage`): "The crew hauls 50 units
  of floating salvage into the hold." Half of each cargo lot floats off; crates float about half
  an hour.

Played in order:

1. Permits: with Vesselmate's permit Brinewick set the Sea Wren's heading from her bridge
   ("Heading set to 90 degrees (E)."; aboard: "Brinewick adjusts the vessel's heading."); revoked,
   "You must be at an authorized helm to set heading."
2. `shipdeed <player>` needs both captains in the same room and the receiver under the ownership
   cap.
3. `pvp` (both): "PvP flag enabled. You are now eligible for player vs. player combat." The flag
   cannot be turned off again for a while ("You must wait 4 more minutes before you can disable
   your PvP flag.").
4. Grouped (`group leave`, `group join vesselmate`), then apart: firing on a groupmate's ship
   costs the shooter the group, as attacking them in person does.
5. The fight: hits land on the side facing the shooter; a stern full of hits fouls her rudder so
   she cannot turn; one holed side cannot sink her, so the Sea Wren maneuvered (`setsail northwest` twice) to bring the Kestrel's starboard under her port battery. Two holed sides:
   "Condition: sinking, holed on 2 sides"; a player's hull goes down in 75-150 seconds.
6. Two hulls on the same spot (range 0.0) cannot fire at each other unless a weapon faces north:
   at range 0 the bearing defaults to north. Keep a room or two apart.
7. `shiprenown`: "The most renowned hulls: 1. Sea Wren Ship Vesselmate 200 renown". Renown (the
   sunk hull's weight: a ship 200) is shared with allies in sight: hulls out of port whose owners
   are online and grouped with the sinker's owner (not played: it needs a third captain).
8. `bounty`: "You carry no price." (sinking a consenting captain's ship is not piracy; plunder of
   a player's cargo is). `bounty brinewick`: "Brinewick carries no price." `marque` at sea:
   "Letters of marque are issued at a port's admiralty office." (bought in chapter 12.)

Defects found here and fixed: grouped captains could fire on each other's ships and stay grouped;
`bounty` told a clean captain "You carries no price."

## 12. Contraband and customs

Staging: Kohdee gave Vesselmate a fresh lantern (the first had burned out, and in the dark he
could not see his hull on the dock to board her). The Sea Wren had 200 renown from chapter 11.

- `47-contraband-buy.png` (Vesselmate at the Harbor Sandbox East Dock, `cargobuy forbidden 10`,
  `cargomanifest`): "You load 10 units of forbidden tomes for 1938 gold (193 average each)." with
  50 grain from the salvage in the hold.
- `48-contraband-customs.png` (Vesselmate, berthing at the Testing Dock): "The port authorities
  come aboard Sea Wren in search of contraband." "Customs confiscate 10 of 10 units of forbidden
  tomes!"

Notes:

- `market` marks contraband: the east dock lists "forbidden tomes 4 190 161 steady (contraband)"
  (it stocks them); everywhere else they read "- 248 none (contraband)": not for sale, but every
  other port pays the scarce price.
- Each contraband good is stocked at one port and sold there only to a hull of 150 (tomes), 200
  (poisons) or 250 (eggs) renown, or with an able sailmaster and quartermaster; never to a
  warship or a captain of perfect virtue.
- Every lawful port searches a player's hull sailing in. Each unit not stocked there is seized
  with a chance of 35% plus half a percent a unit, less a fifth of the square root of her renown,
  higher the emptier her hold, never under 5%: ten tomes among 50 grain in a 13,200-pound hold
  were all taken. Fill the hold with honest cargo first.
- `cargobuy` takes any word of the goods' name (`cargobuy tomes 10` after the fix; `forbidden`
  before).
- `marque` and `bounty pay` are done ashore on a dock: "You pay 2000 gold. The admiralty
  commissions you as a privateer - prizes taken now are lawful." Aboard a berthed hull they were
  refused with no hint to step ashore (fixed).

Defects found here and fixed: `cargobuy tomes` found nothing; the admiralty refusals did not say
the office is ashore.

## 13. Loss and recovery

The Kestrel sank in chapter 11; this chapter follows Brinewick.

- `45-loss-sunk.png` (chapter 11) carries the sinking and the insurance: "The harbor office
  delivers 5400 gold from 1 vessel settlement." A player's sunk hull is insured for three quarters
  of her value, paid by mail.
- `49-loss-shipsummon-list.png` (Brinewick at the Testing Dock, `shipsummon`): " 1. Kestrel
  (Boat): in the wreck registry; 2 gold, about 63 minutes." `shipsummon 1`: "You pay 2 gold.
  Word goes out to Kestrel; she should make port here in about 63 minutes." A wreck keeps her
  name and fleet slot but is rebuilt as the cheapest boat the shipyard sells, stripped
  (decision D3).
- `50-loss-ownership-cap.png` (Vesselmate, owning the Sea Wren and the Gull Prize, buys a
  Sablebranch Raft, then tries another): "You already own 3 hulls, the most one captain may
  hold."
- `51-loss-rename-fee.png` (Vesselmate aboard the raft): the first christening is free ("By her
  owner's word, this vessel is christened Skiff!"); renaming her costs a tenth of her price:
  "You pay the registry 33 gold." "...christened Wren Skiff!"
- `64-loss-trade-in.png` (Brinewick at the Testing Dock after the Kestrel made port, `shipbuy 26 trade`): "The shipwrights take Kestrel in trade for 0 gold and rebuild her as a Ship. You pay
  7200 gold." A wreck is worth nothing in trade; a sound hull is credited 90% of her value. She
  keeps her name, slot, crew and display ID.
- The Kestrel made port about an hour after the order; her owner, at sea, heard nothing (fixed:
  the harbor now sends word to an owner who is elsewhere).

## 14. Other hulls and vehicles

Staging: Kohdee spawned each frontier hull where it can travel (`vedit spawn <id>` with Kohdee
standing there), granted Vesselmate a helm permit from aboard (`shippermit vesselmate`: a staff
spawn belongs to the staff member), and transferred him to the bridge. Kohdee created the
cart with `vehiclecreate cart River Cart` on the plains at (-810, 478).

- `52-hulls-riverboat.png` (Vesselmate on the Sablebranch Riverboat at (-809, 480), Ship tab):
  "Terrain: River". `setsail east` along the river; `setsail north` onto the bank: "Your boat
  cannot go there! She keeps to rivers and shallow coastal water." (fixed: it read "cannot handle
  these conditions! It's designed for coastal waters only.")
- `53-hulls-airship-skyway.png` (the Aetherwind Courier at (467, 0)): ten `setsail up` (five
  seconds apart) to altitude 100: "The vessel climbs to 100." "The high currents of Aetherwind
  Skyway lend speed to the vessel." `seastate`: "Sky lane : Aetherwind Skyway (active above
  100)".
- `54-hulls-sky-island.png` (ten more climbs to 200, `setsail east` twice to (469, 0)):
  `seastate`: "Sky island: Shardspire Sky Island (reachable above 200)". `shipstatus` shows
  "Elevation/Depth: 200".
- `55-hulls-submarine.png` (the Starfall Bathyscaphe at (900, 225), Ship tab): three `setsail down`: "The vessel descends to -30." "Elevation/Depth: -30"; `seastate` "Depth : deep". She
  sails east submerged and climbs back to 0.
- `56-hulls-magical.png` (the Liminal Wayfarer at (-810, 479)): on the plains ("Terrain:
  Plains"), `setsail north` onto the river, `setsail down` under it to -10, back up, south onto
  the plains and `setsail up` to 10 over them.
- `57-vehicles-look.png` (Vesselmate beside the cart): `look` lists "River Cart, a cart, stands
  here." (fixed: vehicles were not listed at all).
- `58-vehicles-unload.png` (aboard the Wayfarer): hovering at 10, "Liminal Wayfarer must be at
  the surface to unload vehicles."; `setsail down` to 0, "You unload River Cart from Liminal
  Wayfarer." (fixed: she loaded and unloaded the cart while hovering).
- `59-hulls-freighter.png` (aboard the Sablebranch Grand Freighter at the Testing Dock):
  "Speed: 0 / 5", "Hold: 0 of 40000 lbs used."

Vehicle play, in order: `vstatus` beside the cart (type, speed, "Passengers: 0 / 2", "Cargo: 0 /
500 lbs", condition); `vmount` ("You climb onto River Cart."); `drive west`/`east` ("You drive
the cart west." "Current position: (-811, 478)"); `vdismount`; the unified `tstatus`, `tenter cart`, `tgo north`, `texit` (fixed: `tgo` moved the cart and left the rider behind);
`loadvehicle cart` from aboard the stopped Wayfarer ("You load River Cart onto Liminal
Wayfarer."), `unloadvehicle` (lists "1. River Cart [cart]"), `unloadvehicle 1` on the river:
"The terrain here is not suitable for River Cart.", on the plains it rolls off.

Notes:

- The level gate holds departures only (`undock` from a berth, `autopilot on`, `assignpilot`,
  `setschedule`): a level-16 permit holder sails a level-24 airship already afloat.
- The Testing Dock is a pier in open water: a cart created there cannot be driven anywhere.
- A cart drives on roads and plains, not water; a hull carries a vehicle only if her hold takes
  its weight (a raft cannot take a cart).

Defects found here and fixed: the riverboat's off-water refusal; vehicles missing from `look`;
`tgo` leaving the rider behind; vehicles loading and unloading aloft or submerged.

## 15. The living world

Staging: Kohdee started each event (`vevent start ...` from a helm or surface water), placed
Vesselmate aboard the Blackwake derelict (slot 8, (-533, 330)), set Brinewick's bounty to 2000
in the database to make him HUNTED (lifting it for a moment so he could buy a raft), and forced
the encounter check (`vesseldebug encounter`).

- `60-world-regatta-status.png` (Vesselmate, `vevent status` after both captains' `vevent join`): "Vessel Event #1: regatta", "Course: (-66,92) -> (-62,82)", both entrants.
  Kohdee opened it from the freighter's helm: `vevent start regatta -62 82`; the start is the
  helm's own position, and entries must begin there.
- `61-world-regatta-finish.png` (Brinewick): "FINISHED #1 in 137s" (the Wren Skiff, on
  Vesselmate's helm permit) and "FINISHED #2 in 151s" (the Sea Wren under autopilot). The finish
  is the exact coordinate; the broadcast reads "[Vessel Event] Vesselmate finished regatta #1 in
  place 2 (151s)."
- `62-world-leaderboard.png` (`vevent leaderboard regatta` after Kohdee's `vevent end`):
  "1. Brinewick entries 1 wins 1 points 100 best 137s".
- `63-world-ghost-score.png` (Vesselmate fighting "Ghost Fleet Wraith 2-1", raised with
  `vevent start ghost 33 1`): "Event score: +8 damage (147 total)." The wraith fights back hard;
  the Sea Wren broke off with her bow armor gone.
- `65-world-skirmish-status.png` (`vevent start skirmish`, Kohdee's `vevent enlist 16 red`,
  `vevent join red` and `vevent join blue`): three entrants and "Team score: red 0, blue 0";
  each hit then scores ("Event score: +6 damage (6 total).").
- `66-world-derelict-search.png` (Vesselmate on the derelict's bridge, `search`): "Beneath the
  collapsed chart table, your hand closes around an ash-stained captain's log." (fixed: only
  the made-up word `searchashlog` worked, and nothing hinted at it).
- `67-world-derelict-salvage.png`: `readashlog` names the next step (SEARCHASHCHART in the crew
  quarters), `studyashchart` the last (RECOVERASHSALVAGE in the cargo hold): "you pry open a
  concealed panel and recover a corroded bronze gear."
- `68-world-wanted-refused.png` (Brinewick, HUNTED, `shipbuy 18` at the Testing Dock): "The
  harbourmaster knows your face - 2000 gold is posted for you here. No lawful business will be
  done with you in this port."
- `69-world-hunter.png` (Brinewick under way on a raft, after the forced check): "A Harbor
  Admiralty warship bears down with its ballistae run out!" The hunter shot the raft's rigging
  away within a minute ("Sablebranch Raft is immobile and cannot maneuver.").
- `70-world-bounty-paid.png` (Brinewick, swum ashore to the dock, `bounty pay`): "You pay 2500
  gold. The admiralty strikes the 2000 gold bounty from its rolls." The hunter is retired at the
  next pardon check (ten seconds).

Notes:

- Only one event runs at a time; END records the leaderboard, CANCEL does not.
- `bounty` while HUNTED: "You: 2000 gold on your head - HUNTED by the navy." "A lawful port's
  admiralty clears it for 2500 gold ('bounty pay')."
- A forced encounter needs a moving hull inside an encounter region. The sandbox's only row is
  the Admiralty hunter patrol (region 7000004 around the harbor, rafts only, no creature), so an
  ordinary creature encounter could not be shown here; builders author those per region.
- `board <word>` takes the first hull whose name has that word: with the Wren Skiff and the Sea
  Wren at one dock, `board wren` chose the skiff; `board sea` the Sea Wren.

Defects found here and fixed: a hull under way in harbor was told gunfire is forbidden "from a
berth"; `shiplock wraith` could not find "Ghost Fleet Wraith 2-1"; a summoned hull's owner was
not told she had made port; the derelict's first clue needed an undiscoverable word; a new raft
could come with a mess hall.

## 16. Staff tools

Played by Kohdee from the Staff Board Room (room 1204), with the hulls left by chapters 1-15.
These screenshots are the only ones from a staff session.

- `71-staff-vedit.png` (`vedit` with no argument): the usage list - `vedit list`, `new <class> <name>`, `show <id>`, `set <id> <field> <value>` (name, class, speed, armor, forsale,
  minlevel), `delete`, `spawn` (a hull owned by the staff member, here), `spawnpublic` (an
  unclaimed public hull) - and the class numbers 0-7. `vedit show 26`: "Prototype 26: Starfall
  Survey Ship", class, speed 12, armor 66 (beam; the class profile sets the rest), cargo 12000
  lbs, "listed in the shipyard", "Level : 16 to take her out of port (class minimum)".
- `72-staff-vmerchant.png` (`vmerchant`): the two NPC merchants with generation, fleet slot,
  state, faction, prototype, route, pilot, cargo and losses ("Harbor Sandbox Merchant",
  "Vailand Ironwind Trader").
- `73-staff-shiplist.png` (`shiplist`): every active hull with slot, name, class, position,
  heading, speed, hull and owner, then "22 of 500 active fleet slots in use." and the wilderness
  room pool. A stowed hull reads "summoned" in the position column.
- `74-staff-shipfix.png` (`shipfix 13`): "Sea Wren (slot 13) restored to full condition."
  (armor, structure, rigging, rudder; not ammunition). Aboard, her crew read "A divine hand mends
  every timber and line."
- `75-staff-shippurge.png` (`shippurge 17` ... `21`, the hulls staged for chapter 14): "Purged
  ship 17 'Sablebranch Riverboat': reclaimed 2 rooms and released 0 vehicles."
- `76-staff-vesseldebug-balance.png` (`vesseldebug balance`): per class, the hull price, one
  refit, the insurance payout and the dock fee at speed 10 and armor 10 (raft 367 / 40 / 0 / 5
  ... magical 100067 / 28800 / 50033 / 75), the persisted sample (owned hulls, fees, completed
  freight, showcase entries), and "Human beta and player fun sign-off: REQUIRED".
- `77-staff-cedit-vessels.png` (`cedit`, then `E`, Extra Game Play Options): "J) Vessel
  System : On" (the kill switch) and "L) Vessel Hulls Per Owner : 3" (1-10). Left with `Q` and
  `n`, so nothing was changed: "Game configuration not saved to memory."

Also played:

- `vesseldebug` and `vesseldebug status`: "Vessel debug support: compiled out (production-safe
  default)." (the debug categories are a build option; `balance`, `ambient` and `encounter` work
  in a normal build).
- `vesseldebug ambient` aboard the Sea Wren: one at-sea ambience line ("The ship's timbers work
  with the sea. It lies still. Clear light runs cleanly to the horizon.") and "Forced this
  vessel's contextual ambient message."
- `vesseldebug encounter` (chapter 15): "Forced the next normal vessel encounter check."
- `shipgoto <slot>` puts the staff member on the hull's bridge; `transfer <player>` then brings
  a player there.
- `vevent start regatta -62 82` from the Sea Wren's helm, then `vevent cancel`: "[Vessel Event]
  regatta event #4 ended: cancelled by staff" "Vessel event cancelled without leaderboard
  changes." `vevent recover`: "Vessel event recovery pass completed; review the system log."
  `vevent end` (chapter 15) records the leaderboard.
- `vehiclecreate` and `vehiclepurge` (chapter 14): "Created vehicle #2: River Cart [cart] at
  (-810, 478)." "Purged vehicle #1."
- `boardfind` is the bulletin-board finder (it lists MySQL message boards, not hulls, and listed
  none: the staff room's HELP board is an old-style board). It and `boardcheck` belong to the
  board system, not the vessel system.

## 17. Client data

Luminari Web's inspector has a Ship tab fed by the server's vessel MSDP variables; most
chapters show it beside the terminal (see the "Ship tab" shots: 07, 11, 14, 17, 24, 34, 43, 52,
55).

- `79-client-ship-tab-aboard.png` (Vesselmate on the Sea Wren's bridge at sea): "[AN] Sea Wren",
  condition, locked target, position (x, y, z), heading, speed; bars for hull (108/108), sails,
  rudder and crew stamina; armor and structure by arc; and each weapon with slot, arc, rounds
  and state ("0 ROUNDS out of ammunition" for the emptied port ballista).
- `34-gunnery-tactical.png` (chapter 8) shows the tab in a fight: the locked contact and the
  contact list.
- `78-client-ship-tab-ashore.png` (Brinewick on the Testing Dock): "Not aboard a vessel. Ship
  data appears while your character is aboard a vessel."

The 21 vessel variables (`docs/systems/MSDP_VARIABLES.md`), for players scripting their own
client: `SHIP_NAME`, `SHIP_ID` (the two-letter contact ID), `SHIP_X`, `SHIP_Y`, `SHIP_Z`,
`SHIP_HEADING`, `SHIP_SPEED`, `SHIP_HULL` and `SHIP_HULL_MAX` (internal structure summed over the
four arcs), `SHIP_STATUS` (sound, battered, crippled or sinking), `SHIP_ARMOR` and
`SHIP_INTERNAL` (tables by arc, current and maximum), `SHIP_SAIL`, `SHIP_SAIL_MAX`,
`SHIP_RUDDER`, `SHIP_RUDDER_MAX`, `SHIP_STAMINA`, `SHIP_STAMINA_MAX`, `SHIP_TARGET` (the locked
contact's ID, or empty), `SHIP_WEAPONS` (slot, name, arc, ammunition, ready, damage) and
`SHIP_CONTACTS` (nearest contacts: ID, name, range, bearing, arc). A client must report them
(`REPORT`); they clear when the character leaves the hull. A client that speaks GMCP but not
MSDP gets the same values as JSON in the `MSDP` GMCP package.

The client's "MSDP Vars" menu renames the variables it requests; it does not show their values.
