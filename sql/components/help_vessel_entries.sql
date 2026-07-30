-- Vessel and transport help entries -> help_entries / help_keywords
--
-- The help system runs in dual mode (src/db.c): database help is authoritative
-- when a keyword exists. Runtime help files are ignored deployment data, and
-- standalone autopilot.hlp or schedule.hlp files are not indexed or maintained
-- sources. A deployment that provisions file fallback should generate one
-- consolidated help.hlp from authoritative content.
--
-- Equivalent to running 'hedit import' in-game, but reviewable and repeatable
-- without an interactive staff session. Idempotent: re-running updates existing
-- rows rather than failing on the UNIQUE tag constraint.
--
-- Covers every command registered for the vessel, vehicle, transport, autopilot,
-- and vessel staff surfaces. Keep the keyword audit at the end of this file in
-- step with src/interpreter.c whenever that command surface changes.

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VESSELS', 'Vessel navigation and boarding:

BOARD [vessel]
  Board a vessel object in your room. If a bulletin board is present, BOARD
  continues to operate that board instead.

DISEMBARK
  Leave a stopped vessel. Docked ships return you safely to the dock or linked
  ship. Away from shore, you must be able to swim and will enter the water.

TACTICAL
  Show the local terrain grid and nearby vessels.

SHIPSTATUS
  Show position, terrain, heading, speed, and armor on all four sides.

SPEED [0-max]
  Set desired speed at an authorized helm. Zero is all stop.

HEADING [0-360]
  Set an absolute compass heading at an authorized helm. North is 0, east 90,
  south 180, and west 270.

SETSAIL <direction>
  Set a cardinal, diagonal, up, or down heading and begin moving. The vessel
  must already have a positive speed. Surface hulls remain at elevation zero.
  Air-capable hulls cannot exceed their class ceiling, and submersible hulls
  can use negative depth only over water.

CONTACTS
  List other vessels within detection range, nearest first.

DOCK [vessel]
  With no target, list vessels in docking range. With a name or fleet ID,
  create a gangway between two stopped, nearby vessels.

DOCKFEES [pay]
  Show the one-time berthing fee assessed when an owned vessel enters a port.
  The vessel cannot leave until an owner or permitted helmsman pays it. A
  clan that owns the port\'s zone receives the fee; public-port fees leave the
  economy. Unowned public and NPC hulls are exempt.

UNDOCK
  Remove the gangway to the vessel currently alongside.

LOOK_OUTSIDE
  Show position, terrain, weather, and visible vessels from an interior room
  that has an outside view.

SHIP_ROOMS
  List the vessel interior and identify its bridge and entrance.

BOARD_HOSTILE <vessel>
  Attempt a hostile transfer from one nearby vessel to another. Player-owned
  targets remain subject to the shared PvP consent rules.

Navigation changes require the owner, a permitted helmsman, or the authorized
NPC pilot at the helm. See the individual ownership and autopilot topics for
longer-lived controls.

See also: AUTOPILOT, SHIP-COMBAT, SHIP-OWNERSHIP, SEASTATE, VEHICLES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'VESSELS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'VESSEL');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'SHIPS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'BOARD');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'DISEMBARK');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'TACTICAL');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'SHIPSTATUS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'SPEED');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'HEADING');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'SETSAIL');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'CONTACTS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'DOCK');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'DOCKFEES');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'UNDOCK');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'LOOK_OUTSIDE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'SHIP_ROOMS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'BOARD_HOSTILE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'NAVIGATION');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VEDIT', 'Usage: vedit list
       vedit new <class 0-7> <name>
       vedit show <id>
       vedit set <id> <field> <value>
       vedit delete <id>
       vedit spawn <id>
       vedit spawnpublic <id>

Staff command (builder level). The ship prototype editor: author vessel
prototypes in the database and spawn live, boardable ships from them
without touching world files or recompiling.

Subcommands:
  list    - list all prototypes (id, class, speed, armor, name)
  new     - create a prototype with class-flavored defaults
  show    - inspect one prototype, including its fixed per-class cargo
            capacity in pounds
  set     - change a field; fields: name, class (0-7), speed (1-30),
            armor (0-100, applied to all four sides at spawn)
  delete  - remove a prototype (existing spawned ships are unaffected)
  spawn   - instantiate a live ship in your current room: allocates a
            ship slot, generates the interior from the room templates,
            links the boardable object, assigns you as owner, and saves
            to the database
  spawnpublic - staff-only fixture path for an unclaimed public or NPC
            vessel; it has no player owner and accrues no owner dock fees

Classes: 0=Raft 1=Boat 2=Ship 3=Warship 4=Airship 5=Submarine
         6=Transport 7=Magical

Interior room names and descriptions come from the ship_room_templates
database table. DG attachments for generated room types come from
ship_room_template_triggers. Changes to either table take effect next boot.

See also: AUTOPILOT, SETWAYPOINT, CREATEROUTE', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VEDIT', 'VEDIT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHIPFIRE', 'Naval combat commands (usable from anywhere aboard your vessel):

SHIPFIRE <slot> <target>
  Fire the weapon in the given slot (0-9) at another ship, addressed by
  name or two-letter fleet ID. The shot must be in range of the weapon
  and the weapon\'s mounted side (fore/port/rear/starboard) must face the
  target. Attack roll: d20 + half level + gunnery crew bonus against a
  defense value based on the target\'s speed. Weapons reload over several
  seconds after firing.

  Damage strikes the target\'s facing side: armor absorbs first, then the
  section\'s internal structure. Structural hits degrade subsystems - bow
  hits tear rigging (speed), stern hits foul the rudder (turning). A hull
  with no structure left SINKS: everyone aboard is thrown into the water
  and the ship becomes salvageable wreckage.

  Ships with an assigned NPC pilot automatically return fire at their
  attacker with every weapon that bears.

SHIPREPAIR
  Slow at-sea repairs. The ship must be stationary. Each use patches a
  little armor, structure, rigging, and rudder. Dockside repair (faster,
  for gold) arrives with the shipyard system.

CLAIMSHIP
  Capture a ship: stand on its bridge with no other conscious character
  present and claim it. Ownership transfers to you. Pairs with hostile
  boarding (\'board_hostile <vessel>\' from a nearby vessel).

Running aground: deep-draft vessels that sail into water shallower than
their draft grind to a halt and take bow damage. Check your charts.

PvP: firing on, boarding, plundering, or claiming another player\'s ship all
require that you and the ship\'s owner both have PVP enabled (type \'pvp\'),
exactly as attacking them in person would. Unowned hulls and NPC vessels are
always fair game. A ship whose owner is not logged in cannot be attacked -
nobody is there to consent.

See also: BOARD, DOCK, TACTICAL, SHIPSTATUS, AUTOPILOT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIPFIRE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIPREPAIR');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'CLAIMSHIP');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIP-COMBAT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'NAVAL-COMBAT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHIPBROWSE', 'Ship ownership commands:

SHIPBROWSE
  View the shipwright\'s catalog: every hull design with its price.

SHIPBUY <id>
  Purchase a hull and take immediate delivery. Only works at a dock
  (dockable room). You become the owner.

SHIPCHRISTEN <name>
  Rename a ship you own (3-60 printable characters). Do it once, do it
  well - she\'ll carry the name into every port report.

SHIPDEED <player>
  Sign your ship over to another player. They must be aboard with you.

SHIPPERMIT <player> / SHIPREVOKE <player>
  Manage who may take the helm of your ship. Owned ships answer only to
  their owner and permitted helmsmen; everyone else can ride as a
  passenger but cannot steer, set speed, or dock. Up to 10 permits.
  Capturing a ship (see CLAIMSHIP) voids all previous permits.

SHIPCREW
  List the ship\'s owner, NPC pilot, and helm permits.

Ownership survives reboots. Losing your ship in combat is permanent -
sail accordingly, or don\'t sail what you can\'t afford to lose.

See also: SHIP COMBAT, CLAIMSHIP, VEDIT, BOARD, DOCK', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPBROWSE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPBUY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPCHRISTEN');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPDEED');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPPERMIT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPREVOKE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPCREW');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIP-OWNERSHIP');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHIPHIRE', 'Crew, refits, and insurance - all arranged while moored at a dock, by the
ship\'s owner.

SHIPHIRE <position> <tier>
  Take on crew. Positions and what they do:
    sailmaster     - better speed handling under way
    gunner         - improved gunnery accuracy in combat
    bosun          - faster repairs (see SHIPREPAIR)
    quartermaster  - more cargo capacity
  Quality tiers: green, able, veteran. Better hands cost more to sign and
  more per payday. Type \'shiphire\' with no arguments for current rates.

SHIPDISMISS <position>
  Let a crew member go. Their bonus leaves with them.

SHIPWAGES
  Review the payroll and settle back wages. Wages accrue on their own
  schedule; leave them unpaid too long and your best-paid hand walks off
  at the next opportunity.

SHIPUPGRADE [<refit>]
  With no argument, list available refits and prices. Refits:
    plating        - +50% armor on all sides
    rigging        - +5 maximum speed
    hold           - +25% cargo capacity
    reinforcement  - +50% hull structure
  Each can be installed once. Cost scales with the hull\'s class.

SHIPINSURE [<value>]
  With no argument, show current coverage. Otherwise insure the ship for
  the given payout, up to her market value; the premium is one fifth of
  the payout. If the ship sinks, the underwriters pay the owner.

Wear: hulls under way slowly lose armor and subsystem condition. Put in
for repairs before a fight, not after.

See also: SHIP OWNERSHIP, SHIP COMBAT, SHIPREPAIR', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPHIRE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPDISMISS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPWAGES');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPUPGRADE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPINSURE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIP-CREW');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIP-REFIT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('MARKET', 'Bulk trading. Buy cheap in one port, sell dear in another - the sailing in
between is where the risk lives.

MARKET
  While moored at a port, list every traded commodity: weight per unit,
  the price to buy, the price the port pays, and whether local stock is
  scarce, steady, or glutted. Scarce goods cost more and sell for more.

CARGOBUY <commodity> <quantity>
  Load bulk goods into the hold. Limited by your cargo capacity, which
  depends on hull class, the hold refit, and your quartermaster. Large
  batches cross supply levels, so the reported per-unit value is an average.

CARGOSELL <commodity> [<quantity>|all]
  Sell goods from the hold at the local price. Ports buy below their
  asking price, so buying and selling in the same port always loses money.
  Each unit in a large batch receives the price at its resulting supply
  level rather than the first unit\'s quote.

CARGOMANIFEST
  List the bulk goods aboard and how much of the hold they occupy.

How prices move: every port tracks its own supply of each good. Buying
drains local stock and pushes the price up; selling floods it and pushes
the price down. Supply drifts back toward normal over time, so a route you
work hard cools off and later recovers. Price swings are bounded, so no
route ever pays without limit - the profit is in finding the gradient and
surviving the voyage.

Cargo is part of the ship, not your inventory: it survives reboots, it
counts against capacity, and it can be lost with the ship.

See also: SHIP OWNERSHIP, SHIP CREW, SHIP COMBAT, LOADVEHICLE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'MARKET');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'CARGOBUY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'CARGOSELL');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'CARGOMANIFEST');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'SHIP-TRADE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'CARGO');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('CONTRACTS', 'Freight work: paid deliveries between ports. Steadier money than
speculative trading, and the pay is known before you sail.

CONTRACTS
  Read the freight board at the port you are moored at, and list your own
  active contracts wherever you took them. Each offer shows the cargo,
  quantity, payout, and destination.

CONTRACTACCEPT <id>
  Take a job. The freight is loaded into your hold immediately, so you
  need the capacity free before you accept. Payout is fixed at acceptance.

CONTRACTDELIVER <id>
  At the destination port, hand over the freight and collect. The cargo
  must still be aboard - lose it to pirates or a sinking and there is
  nothing to deliver.

CONTRACTABANDON <id>
  Give up a job. It returns to the board for another captain. The freight
  stays in your hold as ordinary cargo.

Payouts scale with the goods\' value and the distance of the run, so long
hauls of valuable cargo pay best - and those are exactly the runs pirates
watch. Boards refresh periodically; a job someone else takes is gone.

See also: SHIP TRADE, MARKET, CARGOMANIFEST, SHIP COMBAT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CONTRACTS', 'CONTRACTS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CONTRACTS', 'CONTRACTACCEPT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CONTRACTS', 'CONTRACTDELIVER');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CONTRACTS', 'CONTRACTABANDON');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CONTRACTS', 'FREIGHT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CONTRACTS', 'SHIP-FREIGHT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('PLUNDER', 'Taking what isn\'t yours, and what it costs you.

PLUNDER
  Standing on the bridge of a ship you have boarded and cleared, transfer
  her cargo into your own vessel\'s hold. Your ship must be alongside
  (docked or within docking range) and must have the capacity to carry
  what you take. Requirements:
    - You hold the bridge with nobody conscious to contest it
    - It is not your own ship
    - Your ship is alongside with hold space free

BOUNTY [<player>]
  Check the price on your own head, or on someone else\'s.
    500 gold or more  - WANTED: lawful ports refuse you service
    2000 gold or more - HUNTED: the navy hunts you on sight

MARQUE
  At a port\'s admiralty office (any dock), buy a letter of marque. It makes
  your prizes lawful: plundering under a marque earns no bounty. Costs
  2000 gold and lasts one day. The admiralty will not commission a captain
  who is already WANTED - settle your affairs first.

PvP: plundering another player\'s ship requires that you and its owner both
have PVP enabled (type \'pvp\'). Unowned and NPC hulls are always fair game.
See SHIP COMBAT.

Being WANTED shuts you out of every lawful port service: no market, no
freight board, no crew hall, no shipyard, no new hulls. Pirates who cannot
sell what they steal go hungry, which is why the marque exists.

See also: SHIP COMBAT, BOARD, CLAIMSHIP, SHIP TRADE, FREIGHT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PLUNDER', 'PLUNDER');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PLUNDER', 'BOUNTY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PLUNDER', 'MARQUE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PLUNDER', 'PIRACY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PLUNDER', 'SHIP-PIRACY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PLUNDER', 'LETTER-OF-MARQUE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SEASTATE', 'Usage: seastate

Reads the water, sky, and depth around your vessel:

  Water      - the sector you are floating in
  Depth      - how much water is under the keel; shallow water grounds
               deep-draft hulls
  Weather    - fair, fogbound, squally, storm, or gale
  Visibility - how far you can see; fog closes the horizon, a posted
               lookout opens it again
  Hull       - your damage state (sound, battered, crippled, sinking)

Weather is the same weather a walker on the coast experiences - the storm
you are fighting is a real storm in that part of the world, not a private
event. It matters:

  Squall - spray and discomfort
  Storm  - tears at the rigging, costing you sail (and speed)
  Gale   - savages the rigging, and without a sailmaster at the helm the
           hull itself takes damage

Submerged submarines are sheltered from surface weather entirely, but dive
too deep for the water beneath you and pressure will crush the hull.

Dangerous waters: some regions of the sea, sky, and depths hold things that
hunt ships. Seastate will tell you when you are in them. A good lookout
gives you warning before whatever it is arrives.

See also: SHIP COMBAT, SHIPREPAIR, SHIP CREW, TACTICAL', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SEASTATE', 'SEASTATE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SEASTATE', 'SEA-STATE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SEASTATE', 'WEATHER-AT-SEA');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SEASTATE', 'SHIP-HAZARDS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHIPLIST', 'Staff commands for operating the vessel system.

SHIPLIST [summary]
  Fleet overview: every active vessel with its slot, name, class, position,
  heading, speed, hull structure, and owner. Ends with two health figures:
    - fleet slots in use out of the maximum
    - wilderness dynamic room pool utilization

  SHIPLIST SUMMARY omits the per-vessel rows and prints only those health
  figures. Use it for large fleets so the totals fit in one socket output
  buffer.

  The room pool matters: it is shared with every traveller in the
  wilderness, not reserved for ships. If it approaches exhaustion the
  listing flags PRESSURE, and ship movement degrades to reusing the
  nearest room rather than claiming a fresh one.

SHIPGOTO <slot>
  Teleport aboard a vessel - its bridge if one exists, otherwise the water
  it floats in. Use the slot numbers from SHIPLIST.

SHIPFIX <slot>
  Restore a vessel to full condition: armor, hull structure, rigging, and
  rudder. For repairing damage caused by bugs rather than by enemies.

SHIPPURGE <slot>
  Permanently remove a dynamic vessel, its persisted records, boardable
  object, and generated interior rooms. Occupied vessels cannot be purged.
  The protected legacy fixture slots cannot be purged with this command.

SHIPLOAD
  Reserved legacy placeholder. It currently performs no loading operation.

The Vessel System option in CEDIT is a load-bearing kill switch: it blocks
player and builder vessel/vehicle commands and pauses every vessel tick while
leaving SHIPLIST, SHIPGOTO, SHIPFIX, SHIPPURGE, VEHICLEPURGE, and VESSELDEBUG
available for staff recovery.

See also: VEDIT, VESSELDEBUG, VEHICLE-ADMIN, SHIP-COMBAT, SEASTATE', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIPLIST');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIPGOTO');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIPFIX');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIPPURGE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIPLOAD');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIP-ADMIN');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VESSELDEBUG', 'Usage: vesseldebug status
       vesseldebug on <category|all>
       vesseldebug off [category]

Staff runtime control for focused vessel diagnostics. VDEBUG is an alias.

Categories:
  core          - general vessel control flow
  move          - vessel movement
  auto          - autopilot and schedules
  dock          - docking and hostile boarding
  db            - vessel persistence
  func          - function entry and exit tracing
  state         - vessel state changes
  vehicle       - vehicle lifecycle and commands
  vehicle_move  - high-frequency vehicle movement
  transport     - unified transport commands
  all           - every category

Production/default builds compile diagnostics out with VESSEL_SYSTEM_DEBUG=0.
In that build, status reports that logging is unavailable and attempts to
enable a category are refused. For an explicit development diagnostic build,
compile with -DVESSEL_SYSTEM_DEBUG=1; every category still starts disabled and
must be enabled at runtime.

VESSELDEBUG OFF with no category disables the entire runtime mask.

See also: SHIPLIST, SHIP-ADMIN, VEDIT', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELDEBUG', 'VESSELDEBUG');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELDEBUG', 'VDEBUG');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELDEBUG', 'VESSEL-DEBUG');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('AUTOPILOT', 'Usage: autopilot [on|off|pause|status]

The autopilot command controls automated vessel navigation. When enabled,
your vessel will automatically follow the assigned route, moving from
waypoint to waypoint without manual intervention.

Subcommands:
  on     - Enable autopilot and begin navigating the assigned route
  off    - Disable autopilot and stop navigation
  pause  - Temporarily pause navigation (can be resumed with \'on\')
  status - Display current autopilot state, route, and progress (default)

Before using autopilot, you must:
  1. Create waypoints with \'setwaypoint\'
  2. Create a route with \'createroute\'
  3. Add waypoints to the route with \'addtoroute\'
  4. Assign the route to autopilot with \'setroute\'

You must be at the helm (bridge) or be the ship\'s owner to control autopilot.
Anyone aboard can view autopilot status.

Example:
  > autopilot status    - Check current autopilot state
  > autopilot on        - Begin automated navigation
  > autopilot pause     - Pause at current position
  > autopilot off       - Stop autopilot completely

See also: SETWAYPOINT, LISTWAYPOINTS, DELWAYPOINT, CREATEROUTE, ADDTOROUTE,
          DELROUTE, LISTROUTES, SETROUTE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('AUTOPILOT', 'AUTOPILOT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SETWAYPOINT', 'Usage: setwaypoint <name>

Creates a new navigation waypoint at the vessel\'s current position.
Waypoints are used to define points along a route for autopilot navigation.

The waypoint name must:
  - Be 1-63 characters long
  - Contain only letters, numbers, underscores, and hyphens

You must be at the helm or be the ship\'s owner to create waypoints.

Example:
  > setwaypoint harbor_entrance
  Waypoint \'harbor_entrance\' created at position (150.0, 200.0, 0.0).

See also: LISTWAYPOINTS, DELWAYPOINT, AUTOPILOT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SETWAYPOINT', 'SETWAYPOINT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('LISTWAYPOINTS', 'Usage: listwaypoints

Displays a list of all navigation waypoints in the database.
Shows the waypoint ID, name, and coordinates (X, Y, Z) for each waypoint.

This command can be used by anyone aboard a vessel.

Example:
  > listwaypoints
  --- Waypoints ---
  ID   Name                          X          Y          Z
  ---- -------------------- ---------- ---------- ----------
  1    harbor_entrance           150.0      200.0        0.0
  2    open_sea                  500.0      500.0        0.0
  ----------------------------
  Total: 2 waypoints

See also: SETWAYPOINT, DELWAYPOINT, AUTOPILOT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('LISTWAYPOINTS', 'LISTWAYPOINTS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('DELWAYPOINT', 'Usage: delwaypoint <name>

Deletes a navigation waypoint by name.

WARNING: Deleting a waypoint that is part of an active route may cause
navigation issues. Ensure the waypoint is not currently in use.

You must be at the helm or be the ship\'s owner to delete waypoints.

Example:
  > delwaypoint old_dock
  Waypoint \'old_dock\' deleted.

See also: SETWAYPOINT, LISTWAYPOINTS, AUTOPILOT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('DELWAYPOINT', 'DELWAYPOINT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('CREATEROUTE', 'Usage: createroute <name>

Creates a new empty navigation route. Routes are ordered collections of
waypoints that define a path for autopilot navigation.

The route name must:
  - Be 1-63 characters long
  - Contain only letters, numbers, underscores, and hyphens

After creating a route, use \'addtoroute\' to add waypoints to it.

You must be at the helm or be the ship\'s owner to create routes.

Example:
  > createroute trade_run
  Route \'trade_run\' created (ID: 1).

See also: ADDTOROUTE, LISTROUTES, SETROUTE, AUTOPILOT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CREATEROUTE', 'CREATEROUTE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ADDTOROUTE', 'Usage: addtoroute <route> <waypoint>

Adds an existing waypoint to a route. Waypoints are added to the end
of the route in the order you add them.

Each route can contain up to 20 waypoints.

You must be at the helm or be the ship\'s owner to modify routes.

Example:
  > addtoroute trade_run harbor_entrance
  Waypoint \'harbor_entrance\' added to route \'trade_run\' at position 0.

  > addtoroute trade_run open_sea
  Waypoint \'open_sea\' added to route \'trade_run\' at position 1.

See also: CREATEROUTE, LISTROUTES, SETROUTE, SETWAYPOINT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('ADDTOROUTE', 'ADDTOROUTE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('DELROUTE', 'Usage: delroute <name>

Permanently deletes a navigation route and its ordered waypoint associations.
The named waypoints themselves are not deleted.

You must be aboard the vessel and authorized as its captain. Route deletion is
written to the database and the live route cache immediately.

Example:
  > delroute obsolete_run
  Route \'obsolete_run\' deleted.

See also: CREATEROUTE, ADDTOROUTE, LISTROUTES, SETROUTE, AUTOPILOT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('DELROUTE', 'DELROUTE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('LISTROUTES', 'Usage: listroutes

Displays a list of all navigation routes in the database.
Shows the route ID, name, number of waypoints, and loop/active status.

This command can be used by anyone aboard a vessel.

Example:
  > listroutes
  --- Routes ---
  ID   Name                  WPs  Loop   Active
  ---- -------------------- ----- ------ ------
  1    trade_run                5 No     Yes
  2    patrol_route             8 Yes    Yes
  ----------------------------
  Total: 2 routes

See also: CREATEROUTE, ADDTOROUTE, DELROUTE, SETROUTE, AUTOPILOT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('LISTROUTES', 'LISTROUTES');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SETROUTE', 'Usage: setroute <name>

Assigns a route to the vessel\'s autopilot system. The route must exist
and contain at least one waypoint.

After assigning a route, use \'autopilot on\' to begin navigation.

If autopilot is currently active, it will be stopped before the new
route is assigned.

You must be at the helm or be the ship\'s owner to assign routes.

Example:
  > setroute trade_run
  Route \'trade_run\' assigned to autopilot (5 waypoints).
  Use \'autopilot on\' to begin navigation.

See also: AUTOPILOT, CREATEROUTE, ADDTOROUTE, LISTROUTES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SETROUTE', 'SETROUTE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SETSCHEDULE', 'Usage: setschedule <route> <interval>

Sets an automatic departure schedule for the vessel you are commanding.
The vessel will automatically begin following the specified route at
regular intervals measured in MUD hours.

Arguments:
  route    - Name of an existing route (use \'listroutes\' to see available)
  interval - Number of MUD hours between departures (1-24)

Examples:
  setschedule FerryRoute 2    - Depart every 2 MUD hours
  setschedule PatrolRoute 6   - Depart every 6 MUD hours

Requirements:
  - You must be aboard a vessel with autopilot capability
  - You must be the captain or at the helm
  - The route must exist and have waypoints defined

Notes:
  - One MUD hour equals approximately 75 real seconds
  - If a pilot is assigned, departures will be announced
  - The schedule persists across server restarts
  - Use \'showschedule\' to see current schedule status
  - Use \'clearschedule\' to remove the schedule

See also: CLEARSCHEDULE, SHOWSCHEDULE, AUTOPILOT, LISTROUTES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SETSCHEDULE', 'SETSCHEDULE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('CLEARSCHEDULE', 'Usage: clearschedule

Removes the automatic departure schedule from the vessel you are commanding.
After clearing, the vessel will no longer depart automatically.

Requirements:
  - You must be aboard a vessel with a schedule configured
  - You must be the captain or at the helm

Example:
  clearschedule    - Removes the current schedule

Notes:
  - This does not stop a route already in progress
  - Use \'autopilot off\' to stop an active route

See also: SETSCHEDULE, SHOWSCHEDULE, AUTOPILOT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CLEARSCHEDULE', 'CLEARSCHEDULE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHOWSCHEDULE', 'Usage: showschedule

Displays the current departure schedule for the vessel you are aboard.

Output includes:
  - Route name and settings
  - Departure interval in MUD hours
  - Next scheduled departure time
  - Current MUD time for reference
  - Schedule status (Active, Paused, or Disabled)
  - Pilot status (whether departures will be announced)

Example output:
  --- Vessel Schedule ---
  Route: FerryRoute
  Interval: Every 2 MUD hours
  Next Departure: MUD hour 14
  Current Time: MUD hour 12
  Status: Active
  Pilot: Assigned (departures will be announced)

Requirements:
  - You must be aboard a vessel

Notes:
  - No special permissions required to view the schedule
  - Use \'setschedule\' to create or modify a schedule
  - Use \'clearschedule\' to remove a schedule

See also: SETSCHEDULE, CLEARSCHEDULE, AUTOPILOT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHOWSCHEDULE', 'SHOWSCHEDULE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VMOUNT', 'Usage: vmount [vehicle]

This command allows you to mount (board) a land vehicle that is in the same
room as you. Vehicles include carts, wagons, mounts (like horses), and
carriages. Once mounted, you can use the DRIVE command to move the vehicle
across the wilderness.

The optional selector accepts a visible name, vehicle type, or numeric vehicle
ID. With no selector, the first available vehicle in the room is used.

Requirements:
  - A vehicle must be present in your current room
  - The vehicle must be operational (not damaged)
  - The vehicle must have available passenger space

Example:
  > vmount
  You climb onto a sturdy wooden cart.

See also: VDISMOUNT, DRIVE, VSTATUS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VMOUNT', 'VMOUNT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VMOUNT', 'VEHICLES');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VMOUNT', 'LAND-VEHICLES');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VDISMOUNT', 'Usage: vdismount

This command allows you to dismount (exit) from a vehicle you are currently
riding. You will remain in the vehicle\'s current location after dismounting.

Requirements:
  - You must be mounted on a vehicle

Example:
  > vdismount
  You dismount from a sturdy wooden cart.

See also: VMOUNT, DRIVE, VSTATUS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VDISMOUNT', 'VDISMOUNT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('DRIVE', 'Usage: drive <direction>

This command moves your vehicle in the specified direction. You must be
mounted on a vehicle to use this command. The vehicle will travel across
the wilderness in the chosen direction.

Valid directions:
  - north (n), south (s), east (e), west (w)
  - northeast (ne), northwest (nw), southeast (se), southwest (sw)

The vehicle\'s ability to traverse terrain depends on its type. Carts and
wagons work well on roads and plains, while mounts can traverse more
difficult terrain like forests and hills.

Requirements:
  - You must be mounted on a vehicle
  - The vehicle must be operational
  - The destination terrain must be traversable

Example:
  > drive north
  You drive the cart north.
  Current position: (100, 201)

See also: VMOUNT, VDISMOUNT, VSTATUS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('DRIVE', 'DRIVE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VSTATUS', 'Usage: vstatus

This command displays detailed information about a vehicle. If you are
mounted on a vehicle, it shows that vehicle\'s status. Otherwise, it shows
information about any vehicle in your current room.

The status display includes:
  - Vehicle name and type
  - Current state (idle, moving, loaded, damaged)
  - Position coordinates in the wilderness
  - Current and base movement speed
  - Passenger capacity and current count
  - Cargo weight capacity and current load
  - Condition (durability) with percentage

Example:
  > vstatus

  === Vehicle Status ===

  Name: a sturdy wooden cart
  Type: cart
  State: loaded

  Position: (100, 200)
  Speed: 2 (base 2)

  Passengers: 1 / 2
  Cargo: 50 / 500 lbs

  Condition: 100 / 100 (100%)
  The cart is in good condition.

  You are currently riding this cart.

See also: VMOUNT, VDISMOUNT, DRIVE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VSTATUS', 'VSTATUS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VEHICLE-TRANSPORT', 'Vehicle and unified transport commands:

LOADVEHICLE [vehicle]
  From aboard a stopped vessel, load a named vehicle waiting beside the hull.
  The vehicle must be empty and the vessel must have enough vehicle capacity.

UNLOADVEHICLE [number]
  With no number, list vehicles carried by the vessel. Select a list number to
  unload it beside the stopped vessel when the terrain is suitable.

TENTER [target]
  Enter the transport present in the room. For a land vehicle this is the
  unified equivalent of VMOUNT; vessel boarding still directs you to BOARD.

TEXIT
  Exit the current transport. For a land vehicle this is the unified equivalent
  of VDISMOUNT; vessel passengers are directed to DISEMBARK.

TGO <direction>
  Move the current land vehicle in one of the eight horizontal directions.
  Vessel passengers are directed to HEADING and SPEED.

TSTATUS
  Show the current transport, or a transport present in the room. The output is
  specialized for either a land vehicle or vessel.

Loaded vehicle identity, state, and coordinates persist across server restarts.

See also: VMOUNT, VDISMOUNT, DRIVE, VSTATUS, VESSELS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('VEHICLE-TRANSPORT', 'VEHICLE-TRANSPORT');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('VEHICLE-TRANSPORT', 'LOADVEHICLE');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('VEHICLE-TRANSPORT', 'LOADVEH');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('VEHICLE-TRANSPORT', 'UNLOADVEHICLE');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('VEHICLE-TRANSPORT', 'UNLOADVEH');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('VEHICLE-TRANSPORT', 'TENTER');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('VEHICLE-TRANSPORT', 'TEXIT');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('VEHICLE-TRANSPORT', 'TGO');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('VEHICLE-TRANSPORT', 'TSTATUS');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('VEHICLE-TRANSPORT', 'TRANSPORT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VEHICLE-ADMIN', 'Staff land-vehicle commands:

VEHICLECREATE <cart|wagon|mount|carriage> [name]
  Create and persist a test vehicle in the current wilderness room. The
  character becomes its owner. Creation is rolled back if persistence fails.

VEHICLEPURGE <vehicle-id>
  Permanently remove an active vehicle and its database record. All passengers
  must dismount first. VEHICLEPURGE remains available while the vessel-system
  kill switch is off so staff can recover bad state.

Use VSTATUS to inspect a nearby vehicle and its numeric ID.

See also: VSTATUS, VEHICLE-TRANSPORT, SHIP-ADMIN, VESSELDEBUG', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VEHICLE-ADMIN', 'VEHICLE-ADMIN');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VEHICLE-ADMIN', 'VEHICLECREATE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VEHICLE-ADMIN', 'VEHICLEPURGE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ASSIGNPILOT', 'Usage: assignpilot <npc name>

Assigns an NPC in the helm room as the vessel\'s pilot. Once assigned,
the pilot will automatically operate the autopilot system when a route
is set, without requiring manual \'autopilot on\' commands.

Requirements:
- You must be the captain of the vessel
- The target must be an NPC (not a player)
- The NPC must be present in the helm/bridge room
- The vessel cannot already have a pilot assigned

Example:
  assignpilot helmsman

When a pilot is assigned:
- The vessel will automatically navigate any active route
- The pilot will announce waypoint arrivals to all aboard
- The pilot cannot leave the helm room while assigned

To remove a pilot, use the UNASSIGNPILOT command.

See also: UNASSIGNPILOT, AUTOPILOT, SETROUTE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('ASSIGNPILOT', 'ASSIGNPILOT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('UNASSIGNPILOT', 'Usage: unassignpilot

Removes the currently assigned NPC pilot from the vessel. This will
also stop the autopilot if it is currently running.

Requirements:
- You must be the captain of the vessel
- The vessel must have a pilot currently assigned

After removing the pilot:
- Autopilot will be disengaged if it was active
- Manual autopilot control will be required to navigate
- The NPC will remain in the helm room but no longer controls the ship

To assign a new pilot, use the ASSIGNPILOT command.

See also: ASSIGNPILOT, AUTOPILOT, SETROUTE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('UNASSIGNPILOT', 'UNASSIGNPILOT');

-- Retire obsolete one-command vessel entries imported from old file help.
-- search_help() displays only the first matching database row, so leaving these
-- mappings in place makes the authoritative result nondeterministic. Preserve
-- the unrelated character movement-speed topic under unambiguous keywords.
INSERT IGNORE INTO help_keywords (help_tag, keyword)
SELECT tag, 'MOVEMENT-SPEED'
FROM help_entries
WHERE BINARY tag = 'speed';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
SELECT tag, 'RACIAL-SPEED'
FROM help_entries
WHERE BINARY tag = 'speed';
DELETE FROM help_keywords
WHERE BINARY help_tag = 'speed'
  AND UPPER(keyword) = 'SPEED';

DELETE FROM help_keywords
WHERE BINARY help_tag IN (
  'board_hostile', 'disembark', 'dock', 'look_outside', 'ship_rooms', 'undock'
);
DELETE FROM help_entries
WHERE BINARY tag IN (
  'board_hostile', 'disembark', 'dock', 'look_outside', 'ship_rooms', 'undock'
);
