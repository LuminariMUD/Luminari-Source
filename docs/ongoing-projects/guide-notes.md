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
