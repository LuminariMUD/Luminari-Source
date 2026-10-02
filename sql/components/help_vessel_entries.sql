-- Vessel and transport help entries -> help_entries / help_keywords
--
-- The help system runs in dual mode (src/core/db.c): database help is authoritative
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
-- step with src/core/interpreter.c whenever that command surface changes.

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VESSELS', 'Vessel navigation and boarding:

BOARD [vessel]
  Board a vessel object in your room. If a bulletin board is present, BOARD
  continues to operate that board instead.

DISEMBARK
  Leave a stopped vessel. Berthed and docked ships return you safely to the
  dock or linked ship. Away from shore, you must be able to swim and will
  enter the water.

TACTICAL
  Show a 21-by-21 canonical wilderness chart with terrain, public region
  edges, range rings, weather visibility, and damage-aware contacts. The
  contact roster below the chart is the CONTACTS list, with the same IDs.

SHIPSTATUS
  Show position, terrain, elevation or depth, heading, speed, moorings, the
  armor and structure of all four sides, sails, rudder, crew stamina, repair
  stores, and every slot by number: each weapon with its arc and rounds left
  (ready, reloading, out of ammunition, disabled with its damage, or
  destroyed), and any equipment.
  Heading and speed show any order the hull is still answering, and speed
  shows the most she can make under present conditions. Holed sides, the
  time left before a sinking hull goes down, and struck colors appear when
  they apply, as do merchant registry and unpaid dock-fee details.

SHIPTALK <message>
  Speak over the captain\'s channel to awake, hearing occupants in every room
  aboard your current vessel. The channel identifies both the vessel and
  speaker; it cannot be used ashore or while silenced.

SPEED [0-30]
  Order a speed at an authorized helm; zero is all stop. The hull gathers or
  loses way every half second at her class rate, quicker with a sailmaster,
  and sails on her heading until ordered otherwise: at speed 10 she covers 10
  rooms every 45 seconds. She can make at most her design speed, scaled by
  the sailmaster, the weight of her fit-out and cargo, her remaining sail, and
  the terrain, weather, and any high-altitude lane; a seadog at the helm adds
  one. She checks every room she enters: land or unpaid dock fees stop her
  at its edge, and at battle stations so do harbors and shallows (see
  SHIPFIRE). A berthed or anchored hull takes no speed order until UNDOCK
  completes.

HEADING [0-360]
  Order an absolute compass heading at an authorized helm. North is 0, east
  90, south 180, and west 270. The hull comes about at her class turn rate,
  slower at low speed and with a damaged rudder; a smashed rudder cannot
  turn.

SETSAIL <direction>
  Maneuver one room north, south, east, west, or diagonally, or climb or dive
  ten units. Across the map a maneuver needs speed 6 or less and leaves the
  hull stopped on the new heading; maneuvering into a port berths her.
  Climbing and diving keep her under way. The crew needs 5 seconds between
  maneuvers. Surface hulls remain at elevation zero. Air-capable hulls cannot
  exceed their class ceiling, and submersible hulls can use negative depth
  only over water.

CONTACTS
  List the vessels within your visibility, nearest first, with the
  two-letter ID that SHIPFIRE accepts, and the arc of your hull each one
  lies off; the locked contact is marked. Fog closes the horizon and a
  posted lookout extends it. The nearest 20 contacts are shown. Ranges
  count one room for every 10 of altitude or depth.

DOCK [vessel]
  With no target, list vessels in docking range. With a name or fleet ID,
  create a gangway between two nearby vessels moving no faster than speed 2.
  Docking brings both vessels to a stop.

DOCKFEES [pay]
  Show the one-time berthing fee assessed when an owned vessel enters a port.
  The vessel cannot leave until an owner or permitted helmsman pays it. A
  clan that owns the port\'s zone receives the fee; public-port fees leave the
  economy. Unowned public and NPC hulls are exempt.

UNDOCK
  With a vessel alongside, remove the gangway. Otherwise cast off from a
  berth in 30 seconds or weigh anchor in 13. A hull that comes to rest in a
  port is berthed and holds there until her crew reports her ready. Casting
  off needs a whole sail, settled dock fees, the shipwrights finished with
  her, a legal fit-out (see SHIPWEAPON), and a captain of the hull\'s level
  (see SHIPBROWSE).

ANCHOR
  Drop anchor where the hull lies stopped on the surface away from a berth.
  An anchored hull holds position and takes no speed order or maneuver until
  UNDOCK weighs anchor. Anchoring disengages the autopilot.

LOOKOUT (legacy alias: LOOK_OUTSIDE)
  From an interior room with an outside view, scan canonical wilderness
  sectors in eight compass directions out to the weather- and lookout-limited
  horizon. Also show current elevation and water depth plus nearest-first
  visible vessels with their contact IDs (as CONTACTS shows them),
  condition, bearing, range, and relative altitude.

SHIP_ROOMS
  List the vessel interior and identify its bridge and entrance.

BOARD_HOSTILE <vessel>
  Launch a hostile transfer from one nearby vessel to another. Grappling
  lines hold only on a vessel making speed 3 or less, or on a beaten one
  (holed, immobile, colors struck, or abandoned), and a hull aloft only
  from within 10 of her altitude. The first
  opposed Boarding check secures grappling lines; if they hold, a second
  opposed Boarding check resolves the crossing. Ties favor the defending
  vessel. Its strongest conscious, PvP-consenting occupant supplies the
  defense skill, while speed, hull class, structure, sailmaster, and bosun
  modify the vessel side. A decisive crossing failure or natural 1 throws the
  attacker into the water for an Athletics swim check.

  Boarding is a trainable class ability for every class. It uses the better
  of Strength or Dexterity, applies the armor penalty, and replaces the old
  level-plus-Athletics boarding formula. Player-owned targets remain subject
  to the shared PvP consent rules before either contest occurs.

Navigation changes require the owner, a permitted helmsman, or the authorized
NPC pilot at the helm. A public hull with an NPC pilot, such as a ferry or a
merchant, answers only to her pilot: her passengers ride. See the individual
ownership and autopilot topics for longer-lived controls.

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
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'SHIPTALK');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'SPEED');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'HEADING');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'SETSAIL');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'CONTACTS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'DOCK');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'DOCKFEES');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'UNDOCK');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'ANCHOR');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'LOOK_OUTSIDE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'LOOKOUT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'SHIP_ROOMS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'BOARD_HOSTILE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELS', 'BOARDING');
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
  list    - list all prototypes (id, class, speed, armor, for sale, level,
            name)
  new     - create a prototype with class-flavored defaults
  show    - inspect one prototype, including its fixed per-class cargo
            capacity in pounds
  set     - change a field; fields: name, class (0-7), speed (1-30),
            armor (0-229, the beam armor; the class profile sets the
            bow, stern, and structure in proportion), forsale
            (yes/no: listed by SHIPBROWSE and sold by SHIPBUY; new
            prototypes are not for sale), minlevel (0-30: level needed
            to take the hull out of port; 0 uses the class minimum)
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

SHIPLOCK <contact> | SHIPLOCK off
  Lock the guns onto a contact, addressed by the two-letter ID that
  CONTACTS and TACTICAL show or by the start of its name (the nearest match
  wins); only vessels within your visibility qualify. Locking calls the
  crew to battle stations. The lock drops when the contact passes out of
  sight, enters a port, dives, or goes down. With no argument SHIPLOCK
  shows the current lock.

SHIPFIRE <slot | fore | port | rear | starboard> [<contact>]
  Fire one weapon slot (0-15, as SHIPSTATUS lists them), or every weapon on
  an arc that can fire, at the locked contact; naming a contact locks onto
  it first. A weapon fires only when its arc faces the target (arcs run
  from the bow: fore 320-40 degrees, starboard 40-140, rear 140-220, port
  220-320), the target lies inside its range band, and it is sound, loaded,
  and reloaded. Your hull cannot fire from a berth, at anchor, submerged,
  or while going down. Every volley costs a combat round and each shot a
  round of ammunition; each weapon then reloads for 17 seconds (25 and a
  half for the capital weapons), a little faster with a better gunner and
  slower with a tired crew (see SHIPHIRE).

  A shot hits on d20 + gunnery bonus against a DC set by the geometry: the
  range (much better inside three quarters of the weapon\'s reach), how
  fast the target crosses your line of fire, how fast the bearing swings
  (your own turning counts), and how fast the range opens or closes, which
  matters most to lofted catapult shots. A bigger target is easier to hit,
  and a hull aloft is half again as hard. The gunnery bonus is your
  gunner\'s +2/+4/+6 plus your Dexterity modifier (Intelligence for
  catapults and the Long Tom), at most +7. A natural 1 misses and a 20
  hits. Every shot shows its chance to hit. Ranges count one room for every
  10 of altitude or depth, so a hull far above or below is out of reach,
  and a submerged hull can be neither locked nor fired upon.

  The Mind Blast Cannon does no damage. A hit stuns the target\'s crew for 5
  seconds at the cannon\'s 20-room reach, rising to 20 seconds at point
  blank: a stunned crew cannot steer or maneuver (the hull carries on as
  she was), fire, reload, or repair. Inside 10 rooms everyone aboard also makes a
  Will save (DC 15) or falls prone for two rounds.

SHIPSIGHT [<slot>]
  For the locked contact, each weapon\'s DC and chance to hit, or why it
  cannot fire.

SHIPSCAN <contact>
  A close look at a contact within 20 rooms (22 with a posted lookout):
  armor and structure on each side, weapons with the destroyed ones
  marked, condition, and whether her owner is WANTED, HUNTED, or sails
  under a letter of marque.

SHIPRAM [off]
  Brace the crew to ram the locked contact; she needs speed 6 or more. She
  rams when the contact comes within a room, inside a 120-degree cone off
  her bow and at her altitude. The heavier and faster the two hulls, the
  harder the blow is to land; a stopped target is ten times easier, and a
  better sailmaster helps. Both hulls take crash damage. A ram (SHIPEQUIP)
  strikes first and halves the crash damage to its own bow. The heavier
  hull slews the lighter about and both slow to speed 3, and everyone
  aboard both makes a Reflex save (DC 15) or falls prone. She cannot ram
  again for 50 seconds (25 after a miss), and her guns are silent for 25
  seconds after a hit. A braced crew does not reload. Losing the lock or
  slowing to speed 3 stands it down, and SHIPRAM OFF stands it down at
  once. So does an order that no longer stands when she reaches the
  contact: whoever gave it must still be in the game and free to fight the
  hull she is then locked on.

  Battle stations last until 180 seconds after the lock clears, your last
  shot, or the last shot at you. At battle stations no harbor admits her,
  she keeps off water too shallow for her draft as well as land, and when
  either stops her she may run aground: the faster she was going the
  likelier, a better sailmaster helps, and a crew stunned by a mental blast
  cannot save her. Running aground lands several hits, the first on the
  bow.

  Only the ship\'s owner, the owner\'s helm permit holders, members of the
  owner\'s group while the owner is online, and staff may fire a ship\'s
  weapons. Unowned hulls fire only through their NPC crews.

  Harbors are neutral ground: a vessel in a port can neither fire nor be
  fired upon, by players or by NPC crews.

  Each hit strikes the sails (costing speed) or the side facing the shooter,
  scattered by the weapon\'s spread; catapults and the Long Tom throw several
  fragments. Armor absorbs first; a hit it holds stops there unless it is a
  critical (a natural 20, 19-20, or 18-20 as the weapon pierces armor,
  confirmed by a second roll), which carries half its damage into the
  structure. Damage past the armor strikes that side\'s structure
  and may damage a weapon mounted there. On a gutted side, hits can glance
  into another section and always damage a weapon. Stern structure hits
  foul the rudder (turning). A structural hit can knock everyone aboard off
  their feet: Reflex DC 15, or prone for two rounds.

  A side with neither armor nor structure left is holed. One holed side
  leaves her dead in the water (an airship aloft keeps half speed); two
  start her SINKING. A player\'s hull goes down in 75-150 seconds, an
  unowned hull in 17-25 minutes, time enough to board and plunder her. A
  sinking hull cannot move, fire, or be repaired. When she goes down,
  everyone aboard is thrown into the water, half of each cargo lot floats
  off as salvage crates, and the hull becomes wreckage; a player\'s ship
  lives on in the wreck registry (see SHIPSUMMON). A hull shot from one
  side only is holed there but cannot sink until a second side is holed,
  so maneuver to bring a fresh side under your guns.

  Ships with an assigned NPC pilot automatically return fire at their
  attacker with every weapon that bears, and NPC merchants under fire run
  from their attacker until their crew stands down.

  Raiders prey on player vessels bigger than a boat under way on open
  water: an ambush comes about once in 17 minutes of sailing, twice as
  often in a pirate cove, half as often in territorial waters, and sixty
  times more rarely under neutral colors (SHIPEQUIP), which no raider
  picks as prey. A ship or transport is ambushed at most once a voyage,
  until she next berths. The bigger and more renowned the hull, the
  stronger the raider: four tiers, from corsairs in light ships to dread
  sea-lords in heavy warships, each carrying more renown (SHIPRENOWN). A
  raider appears beyond sight off your bow and closes to fight: she turns
  her guns to bear, rams, runs when her ammunition is spent or she is
  holed, and boards a hull slowed to speed 3 (a warship that has stopped)
  through the boarding contest. Pirates who board carry off part of the
  hold and leave; hunters stay to fight. Kill a raider\'s captain on her
  bridge and she stops dead; the captain carries the key to her strongbox.
  A raider that has lost her prey sails off and is gone five minutes
  later, once nobody is watching. Raiders cannot be claimed.

SHIPSALVAGE
  From the helm of a stopped vessel, haul the salvage crates floating
  alongside into the hold, as much as she can carry. Crates float for about
  half an hour.

SHIPREPAIR [armor | structure | sails | rudder | weapons | all]
  At sea the crew mends what it can on its own, from repair stores equal to
  the hull\'s weight and refilled whenever she berths (SHIPSTATUS shows what
  is left). A holed or battered side is shored up only to a tenth of its
  structure and the sails and rudder to two fifths, more with a better
  bosun; damaged weapons come back in full, but armor is never mended at
  sea. The crew works fastest at anchor and slowly under way, and cannot
  replace rigging shot away while she is under way. Anyone aboard may lend
  a hand: SHIPREPAIR at sea is one Craft (woodworking) check, DC 15, that
  patches one point of the worst damage from the stores.

  At a shipyard, with the hull berthed, her owner buys the rest: armor and
  structure at 2 gold a point, sails and rudder at 4, a damaged weapon at
  2 gold a point of damage, and a destroyed one for half its price. Each
  order keeps her at the berth 75 seconds plus a second a point (75 for a
  weapon, 150 rebuilt). SHIPREPAIR alone shows the shipwrights\' quote.

CLAIMSHIP
  Capture a beaten ship: stand on its bridge with no other conscious
  character present and claim it. Only a prize can be taken: a hull with a
  holed side, one that cannot move, one whose colors are struck, or one
  abandoned at sea with nobody conscious aboard but you. A sinking hull
  cannot be claimed. Ownership transfers to you, unless you already own as
  many hulls as the ownership limit allows. Pairs with hostile boarding
  (\'board_hostile <vessel>\' from a nearby vessel).

STRIKECOLORS
  Yield: the owner or a helm permit holder of a stopped vessel strikes her
  colors, making her a prize that may be boarded, plundered, and claimed.
  The colors fly again when she gets under way or after ten minutes.

PvP: firing on, boarding, plundering, or claiming another player\'s ship all
require that you and the ship\'s owner both have PVP enabled (type \'pvp\'),
exactly as attacking them in person would. Unowned hulls and NPC vessels are
always fair game. A ship whose owner is offline is normally protected. Once a
mutually consenting fight begins, however, logging out does not immediately
shield the vessel from that same opponent: the opponent retains five real
minutes of attack grace. Other attackers remain blocked while the owner is
offline.

A ship fights only with its owner\'s consent. When a permit holder or group
member fires on another player\'s ship, the firing ship\'s owner must also be
online with PVP enabled, so the target can always answer in kind. As in
person, turning on a groupmate\'s ship costs you your place in the group.

See also: BOARD, DOCK, TACTICAL, SHIPSTATUS, AUTOPILOT, SHIPRENOWN', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIPFIRE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIPREPAIR');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'CLAIMSHIP');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIPSALVAGE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'STRIKECOLORS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIPLOCK');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIPSIGHT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIPSCAN');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'BATTLE-STATIONS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIP-COMBAT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'NAVAL-COMBAT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'SHIPRAM');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'RAMMING');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPFIRE', 'RAIDERS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHIPRENOWN', 'Renown, prize money, and ship damage control.

SHIPRENOWN
  List the ten hulls with the most renown, with their class and owner.

Renown belongs to a hull, not to her captain, and she keeps it through the
wreck registry, a trade-in, and a new name; SHIPCREW shows it. A player\'s
hull wins renown only by sinking another player\'s hull: the hull weight of
the ship sunk (a boat 25, a ship 200, a warship 285), shared equally by the
hull that sank her and her allies in sight, the hulls out of port whose
owners are in the game and grouped with her owner. The hull sunk loses that
much, though never below none, and her crew lose a further 1% of their
experience for every 30 of it. Sinking NPC vessels trains the crew but wins no
renown.

Renown lets able and veteran hands sign on (SHIPHIRE), capital weapons be
mounted (SHIPWEAPON), and smugglers deal with her (CONTRABAND). It also
draws stronger raiders (RAIDERS).

Prize money: whenever a player\'s hull sinks another vessel, the same hulls
share out what she was worth, each share paid to the hull\'s owner at once,
or at their next login, with a receipt by mail:
  salvage       - an eighth of her value, scaled by the armor and structure
                  she had left, plus half the price of each weapon not
                  destroyed
  renown bounty - 2.5 gold for each point of her renown when she has more
                  than 100, as raiders do
  bounty        - the whole bounty of her owner, if WANTED or HUNTED and
                  aboard when she went down; it is then cleared (BOUNTY)
A hull sunk by another of her owner\'s hulls earns nothing.

Ship damage control: an epic feat of up to five ranks. While you are aboard
a vessel you own, every blow to her hull or sails is cut by 8 percent at
the first rank and 4 more at each rank after, 24 percent at the fifth,
never below 1 point.

See also: SHIP COMBAT, SHIPHIRE, MARKET, BOUNTY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPRENOWN', 'SHIPRENOWN');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPRENOWN', 'RENOWN');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPRENOWN', 'SHIP-RENOWN');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPRENOWN', 'PRIZE-MONEY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHIPBROWSE', 'Ship ownership commands:

SHIPBROWSE
  View the shipwright\'s catalog: every hull design offered for sale, with its
  price and the level needed to take it out of port (Lvl).

SHIPBUY <id> [trade]
  Purchase a listed hull and take immediate delivery. Only works at a dock
  (dockable room). You become the owner. One captain may own at most three
  hulls at a time (staff set the limit with CEDIT), counting wrecks and
  hulls under summons; unowned public and NPC hulls do not count. With
  TRADE, trade in your hull berthed at this dock with an empty hold: 90%
  of her value comes off the price (any excess is paid to you; a wreck,
  whose insurance has paid for her, earns nothing), and the shipwrights
  rebuild her as the new hull under her name, with her crew, paint, and
  permits. The new hull comes with her class armament and
  takes aboard whatever of the old weapons and equipment she can legally
  carry; the shipwrights buy the rest (see SHIPWEAPON sell).

SHIPCHRISTEN <name>
  Aboard a ship you own, rename her (3-60 printable characters). The first
  christening of a new hull is free; an owner may christen the
  ship again later for a tenth of her value. The current name appears on
  her rooms and in port and fleet reports.

SHIPCUSTOMIZE [show]
SHIPCUSTOMIZE <paint|figurehead> <description|clear>
  Aboard, review or change your ship\'s optional exterior details.
  Descriptions run 3-80 printable characters and appear on the hull and in
  lookout reports.

SHIPDEED <player>
  Sign your ship over to another player. They must be in the same room as you
  and own fewer hulls than the ownership limit.

SHIPPERMIT <player> / SHIPREVOKE <player>
  Manage who may take the helm of your ship. Owned ships answer only to
  their owner and permitted helmsmen; everyone else can ride as a
  passenger but cannot steer, set speed, or dock. The named player need not be
  present. Up to 10 permits. Capturing a ship (see CLAIMSHIP) voids all
  previous permits.

SHIPCREW
  List the ship\'s owner, NPC pilot, helm permits, and hired crew positions,
  with each hand\'s tier, experience, and the threshold for the next tier.

SHIPSUMMON [<number | name>]
  At a shipyard, list the hulls you own with where they are, the fee, and
  how long each would take to reach you; with a number or the start of a
  name, send for one. She leaves at once with her hold emptied, anyone
  aboard put over the side, and nothing alongside, and makes port here
  after her passage: quicker for a faster hull, twice as long from the
  wreck registry, never more than 75 minutes. The fee is a tenth of a gold
  piece per point of hull weight. A hull at battle stations or going down
  will not answer. A summons survives a reboot.

Hull levels: taking a hull out of port, engaging its autopilot, or putting
an NPC pilot or schedule to work needs the hull\'s level: 1 for rafts and
boats, 16 for ships, 21 for transports, 22 for warships, 23 for submarines,
24 for airships, and 25 for magical hulls, unless the design sets its own.
Anyone may own a hull or ride aboard one.

Ownership survives reboots. When a player\'s hull goes down, her crew lose some
of their experience and she is rebuilt as the shipyard\'s cheapest boat, with
no weapons, equipment, or refits and, unless an NPC vessel sank a larger hull,
no sails. She keeps her name, crew, paint, permits, and what renown she has
left, and waits in the wreck registry until you summon her. Insurance is
automatic: the underwriters pay three quarters of a lost ship\'s, transport\'s,
or boat\'s value and half of any other\'s, nine tenths when an NPC vessel sank
her, and nothing for a raft or a rebuilt wreck. The settlement arrives at once
if you are in the game, else when you next enter it.

See also: SHIP COMBAT, CLAIMSHIP, VEDIT, BOARD, DOCK', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPBROWSE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPBUY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPCHRISTEN');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPCUSTOMIZE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPDEED');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPPERMIT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPREVOKE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPCREW');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIP-OWNERSHIP');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPBROWSE', 'SHIPSUMMON');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHIPHIRE', 'Crew, refits, and weapons for a ship owner. SHIPHIRE requires the ship to be
in port; SHIPUPGRADE, SHIPWEAPON, SHIPEQUIP, and SHIPREARM require her
berthed and not casting off. SHIPDISMISS can be used by the owner anywhere
aboard, including while underway.

SHIPHIRE <position> <tier>
  Take on crew. Positions and what they do:
    sailmaster     - better speed handling under way
    gunner         - improved gunnery accuracy in combat
    bosun          - faster repairs (see SHIPREPAIR)
    quartermaster  - more cargo capacity
  Quality tiers: green, able, veteran. Each hand costs a one-time hire
  price and draws no wages; better hands cost more. Type \'shiphire\' with no
  arguments for the prices and the renown each quality asks of a hull:
  able hands sign on only with a hull of 540 to 700 renown, veteran hands
  1,350 to 1,640 (SHIPRENOWN); green hands earn promotion at sea.

  Every hand learns from the work: the sailmaster from each room sailed
  (not in a raft or boat), the gunner from reloading and firing at a
  locked contact, the bosun from repairs, and the sailmaster, bosun, and
  quartermaster from cargo sold. All of them learn from a hull their ship
  sinks, far more from a player\'s. Enough experience promotes a hand to
  the next tier for free; SHIPCREW shows how far each has to go. When
  their ship is lost the crew give up a tenth or so of their experience,
  more when a player sinks a hull of renown, and a hand who falls below
  their tier\'s threshold drops a tier, though never below green.

  A crew has stamina: 500, and 100 more for each tier aboard. Working the
  helm, every shot and reload, and every repair spend it; it returns
  slowly at sea and four times as fast berthed or at anchor. A crew worked
  past empty is exhausted: it turns and gathers way more slowly, reloads
  and repairs more slowly, and shoots less straight, the more so the
  deeper the deficit. SHIPSTATUS shows it.

SHIPDISMISS <position>
  Let a crew member go. Their bonus leaves with them. This does not require a
  port.

SHIPUPGRADE [<refit>]
  With no argument, list available refits and prices. Refits:
    plating        - +20% armor on all sides
    rigging        - +10% maximum speed (at least 1, at most 30)
    hold           - +25% cargo capacity
    reinforcement  - +20% hull structure
  Each can be installed once and costs a fifth of the class price. The new
  plates or frames add their points to what she has left; a refit repairs
  nothing.

SHIPWEAPON [list]
  List the shipyard\'s twelve weapons with price, weight, rounds, range band
  in rooms, damage, reload, and the arcs each may mount on, then how full
  each of your arcs is. A hull mounts only the weapons her class allows,
  up to a number of weapons and a weight on each arc (fore, port, rear,
  starboard), and her whole fit-out, equipment included, may not weigh
  more than she can carry. Heavier fits cost speed (see SPEED).
SHIPWEAPON buy <weapon number or name> <fore|port|rear|starboard>
  Buy and mount a weapon, fully loaded. Catapults and the capital Long Tom
  and fragmentation cannon fit only fore or rear, the heavy ballista only
  on the beams. The beam, blast, and fragmentation cannons and the Long
  Tom are capital weapons: one per hull, mounted only on a hull of the
  renown the list shows (1,600 to 2,000) or with a veteran gunner aboard.
  The shipwrights need 75 seconds per point of weight, and she cannot sail
  until they finish.
SHIPWEAPON sell <slot>
  Sell a weapon back for 90% of its price, 10% if it is damaged.
SHIPWEAPON swap <slot> <slot>
  Exchange two slots, to put the weapons you fire most in easy numbers.

SHIPEQUIP [list | buy <ram|colors> | sell <ram|colors>]
  Fit one ram (2 gold per point of hull weight, and heavy) or neutral colors
  (free, and weightless); sold back at 90%. Rafts and boats carry no ram.
  Neutral colors stay up while there is cargo aboard. A ram strikes first
  and guards her bow when she rams (SHIPRAM); raiders seldom ambush a hull
  under neutral colors and never pick one as prey (SHIPFIRE), but the
  merchants pay a tenth less for her cargo (MARKET).

SHIPREARM [<slot> | all]
  Refill ammunition at 2 gold a round; the shipwrights need 75 seconds per
  weapon. A weapon with no rounds left cannot fire. A destroyed weapon is
  not rearmed.

Wear: hulls under way slowly lose armor and subsystem condition. Put in
for repairs before a fight, not after.

See also: SHIP OWNERSHIP, SHIP COMBAT, SHIPREPAIR', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPHIRE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPDISMISS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPUPGRADE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPWEAPON');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPEQUIP');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPHIRE', 'SHIPREARM');
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
  Load bulk goods into the hold; any word of the commodity\'s name will
  do. Limited by your cargo capacity, which depends on hull class, the hold
  refit, and your quartermaster. Large batches cross supply levels, so the
  reported per-unit value is an average.

CARGOSELL <commodity> [<quantity>|all]
  Sell goods from the hold at the local price. Ports buy below their
  asking price, so buying and selling in the same port always loses money.
  Each unit in a large batch receives the price at its resulting supply
  level rather than the first unit\'s quote. A seller with the seadog feat
  gets a tenth more, a hull under neutral colors a tenth less, and a
  warship four tenths less: the merchants take her cargo for stolen.

CARGOMANIFEST
  List the bulk goods aboard and how much of the hold they occupy.

How prices move: every port tracks its own supply of each good. Buying
drains local stock and pushes the price up; selling floods it and pushes
the price down. Supply drifts back toward normal over time, so a route you
work hard cools off and later recovers. Price swings are bounded, so no
route ever pays without limit - the profit is in finding the gradient and
surviving the voyage.

Contraband: forbidden tomes, rare poisons, and dragon eggs are each stocked
at only one port, and MARKET marks them. Only that port sells them, and
only to a hull of 150, 200, or 250 renown or with an able sailmaster and
quartermaster, never to a warship or a captain of perfect virtue. Every
other port pays their scarce price. Each lawful port (any outside a pirate
cove) searches a player\'s hull sailing in, and its customs seize each unit
of contraband it does not stock with a chance of 35 percent plus half a
percent for each unit of the lot, less a fifth of the square root of her
renown, and raised the emptier her hold: contraband hides best among a
full hold of honest cargo. The chance is never below 5 percent.

Cargo is part of the ship, not your inventory: it survives reboots, it
counts against capacity, and it can be lost with the ship.

See also: SHIP OWNERSHIP, SHIP CREW, SHIP COMBAT, LOADVEHICLE, SHIPRENOWN', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'MARKET');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'CARGOBUY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'CARGOSELL');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'CARGOMANIFEST');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'SHIP-TRADE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'CARGO');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'CONTRABAND');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'CUSTOMS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MARKET', 'SMUGGLING');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('CONTRACTS', 'Freight work: paid deliveries between ports. Steadier money than
speculative trading, and the pay is known before you sail.

CONTRACTS
  Read the freight board at the port you are moored at, and list your own
  active contracts wherever you took them. Each offer shows the cargo,
  quantity, bond, payout, and destination.

CONTRACTACCEPT <id>
  Take a job. You post the goods\' worth as a bond, and the freight is
  loaded into your hold immediately, so you need the gold and the capacity
  free before you accept. Payout is fixed at acceptance.

CONTRACTDELIVER <id>
  At the destination port, hand over the freight and collect: the payout
  repays the bond with a premium for the distance. The cargo must still be
  aboard - lose it to pirates or a sinking and there is nothing to deliver.

CONTRACTABANDON <id>
  Give up a job. It returns to the board for another captain. The freight
  your bond paid for stays in your hold as ordinary cargo.

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
    - She is beaten: holed or immobile, colors struck, or abandoned at sea
    - Your ship is alongside with hold space free

BOUNTY [<player>]
  Check the price on your own head, or on someone else\'s.
    500 gold or more  - WANTED: lawful ports refuse you service
    2000 gold or more - HUNTED: the navy hunts you on sight
  A bounty holds for a full day after your last offense, then shrinks by 5%
  of its size for each further day; after 21 quiet days it is gone. Every
  new offense restarts the clock. A WANTED or HUNTED captain who goes
  down aboard their own hull pays the whole bounty to those who sank her
  (SHIPRENOWN), and it is cleared.

BOUNTY PAY
  Ashore at a lawful port\'s admiralty office (any dock outside a pirate
  cove), clear your whole bounty for 125% of it. Even a WANTED captain may
  pay.

MARQUE
  Ashore at a port\'s admiralty office (any dock), buy a letter of
  marque. It makes your prizes lawful where an authority would otherwise
  post a bounty. Costs 2000 gold and lasts one day. The admiralty will not commission a
  captain who is already WANTED - settle your bounty first (BOUNTY PAY).

Regional law comes from builder-authored wilderness geography. Territorial
waters, free seas, and pirate coves can scale the standard 15-gold bounty per
unit of stolen cargo from zero to 500 percent. Unmapped waters retain the
standard rate. SEASTATE names the current waters, authority, and rate.

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
  Waters     - named geographic waters and their piracy law, when authored

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

Legal waters use the same REGION_GEOGRAPHIC polygons as the wilderness.
Territorial waters, free seas, and pirate coves may apply different piracy
bounty rates. A pirate-cove port does not refuse a WANTED captain. While
underway, the ship announces each named-water boundary once and stays quiet
until it enters a different region.

See also: SHIP COMBAT, SHIPREPAIR, SHIP CREW, TACTICAL, PLUNDER', 0, FALSE)
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
  heading, speed, hull structure, and owner. A hull in the wreck registry
  or under summons shows that in place of her position. Ends with two
  health figures:
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
  object, and generated interior rooms. This is a forced recovery operation:
  it evacuates occupants and loose objects to the exterior and releases loaded
  vehicles before reclaiming the interior. Confirm the target carefully. The
  protected legacy fixture slots 0 and 1 cannot be purged with this command.

SHIPLOAD
  Reserved legacy placeholder. It currently performs no loading operation.

The Vessel System option in CEDIT is a load-bearing kill switch: it blocks
player and builder vessel/vehicle commands and pauses every vessel tick while
leaving SHIPLIST, SHIPGOTO, SHIPFIX, SHIPPURGE, VEHICLEPURGE, and VESSELDEBUG
available for staff recovery. Its Vessel Hulls Per Owner option (1-10,
default 3) caps the hulls one player may own; owners above a lowered cap keep
their hulls but cannot acquire more.

See also: VEDIT, VMERCHANT, VESSELDEBUG, VEHICLE-ADMIN, SHIP-COMBAT, SEASTATE', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIPLIST');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIPGOTO');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIPFIX');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIPPURGE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIPLOAD');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHIPLIST', 'SHIP-ADMIN');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VMERCHANT', 'Usage: vmerchant [list|sync|sink <merchant-id> confirm]

Staff control for the durable NPC merchant fleet.

VMERCHANT or VMERCHANT LIST
  List every configured merchant definition, including its generation, active
  hull, lifecycle state, faction, prototype, route, pilot, cargo, loss count,
  and most recent reconciliation error.

VMERCHANT SYNC
  Reconcile definitions immediately. Missing due merchants are assembled from
  their configured public-hull prototype, real cargo, pilot, route, schedule,
  and spawn coordinates. Stale active-hull references enter the configured
  recovery delay before replacement.

VMERCHANT SINK <merchant-id> CONFIRM
  Development acceptance and staff recovery command. It destroys the active
  merchant hull and cargo through the ordinary sink path, attributes the loss
  to the invoking character, applies the normal faction and bounty
  consequences, and schedules the configured replacement. The CONFIRM word is
  required because this changes durable gameplay state.

Definitions live in vessel_npc_merchants. Consequences are durable, deduplicated
per merchant generation and event, and delivered exactly once when the
responsible player is online or next logs in.

See also: SHIPLIST, SHIPFIRE, PLUNDER, BOUNTY, SEASTATE', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VMERCHANT', 'VMERCHANT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VESSELDEBUG', 'Usage: vesseldebug status
       vesseldebug on <category|all>
       vesseldebug off [category]
       vesseldebug encounter
       vesseldebug ambient
       vesseldebug balance [duels]
       vesseldebug raider <0-3> [hunter]
       vtradecheck [trades]

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
enable a category are refused. ENCOUNTER remains available to staff in every
build; it advances only the cadence counter and then runs the normal region,
class, depth, chance, eligibility, and spawn path.

AMBIENT remains available in every build while staff are aboard a vessel. It
immediately sends that vessel the same class-, speed-, weather-, and depth-aware
ambient line used by the periodic narrative heartbeat. It changes no vessel
state and is intended for acceptance testing.

BALANCE remains available in every build. It runs a read-only mechanical
report combining a deterministic equal-warship duel sample (default 200,
at most 1,000 duels, sailed and fought through the production rules and held
to a 3-8 minute median, a 12 minute p95, nothing under 90 seconds, and at
most 2% drawn), the production 1,000-trade simulation,
crew/refit/insurance/dock cost anchors, and anonymized persisted usage
totals. It does not replace human beta feedback or authorize production
rollout.

For an explicit development diagnostic build,
compile with -DVESSEL_SYSTEM_DEBUG=1; every category still starts disabled and
must be enabled at runtime.

VESSELDEBUG OFF with no category disables the entire runtime mask.

RAIDER remains available in every build while staff are aboard a player\'s
vessel at sea. It launches a raider of the given tier, a pirate or with
HUNTER a hunter, against that vessel through the normal ambush path, as if
the ambush roll had come up (see RAIDERS).

VTRADECHECK [trades]
  Run the deterministic, non-mutating vessel-economy release gate. The default
  is 1000 simulated trades. The check exercises production batch pricing,
  supply bounds, finite arbitrage, adversarial reversal, and restocking without
  changing live ports, cargo, or character gold.

See also: SHIPLIST, SHIP-ADMIN, VMERCHANT, VEDIT', 31, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELDEBUG', 'VESSELDEBUG');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELDEBUG', 'VDEBUG');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELDEBUG', 'VTRADECHECK');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VESSELDEBUG', 'VESSEL-DEBUG');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('AUTOPILOT', 'Usage: autopilot [on|off|pause|status]

The autopilot command controls automated vessel navigation. When enabled,
your vessel will automatically follow the assigned route, moving from
waypoint to waypoint without manual intervention.

The autopilot steers and the hull sails under the same rules as a
helmsman\'s orders (see VESSELS). It casts off from a berth or weighs anchor
first, cruises at the ordered speed (full speed when none is ordered), slows
to steerage speed while it comes about, turns where she lies when the next
waypoint is astern, and slows to a stop at a waypoint where it waits.

Subcommands:
  on     - Enable autopilot and begin navigating the assigned route
  off    - Disable autopilot; the hull keeps her ordered speed and heading
  pause  - Hold the hull where she lies (can be resumed with \'on\')
  status - Display current autopilot state, route, and progress (default)

Before using autopilot, you must:
  1. Create waypoints with \'setwaypoint\'
  2. Create a route with \'createroute\'
  3. Add waypoints to the route with \'addtoroute\'
  4. Assign the route to autopilot with \'setroute\'

You must be at the helm (bridge) or be the ship\'s owner to control autopilot.
Engaging it also needs the hull\'s level (see SHIPBROWSE). Anyone aboard can
view autopilot status.

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
  Waypoint \'harbor_entrance\' added to route \'trade_run\' at position 1.

  > addtoroute trade_run open_sea
  Waypoint \'open_sea\' added to route \'trade_run\' at position 2.

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
VALUES ('SETSCHEDULE', 'Usage: setschedule <route> <interval> [passenger fare]

Sets an automatic departure schedule for the vessel you are commanding.
The vessel will automatically begin following the specified route at
regular intervals measured in MUD hours. An optional fare is collected
whenever a player boards an unowned public vessel on this schedule.

Arguments:
  route    - Name of an existing route (use \'listroutes\' to see available)
  interval - Number of MUD hours between departures (1-24)
  fare     - Optional boarding cost in gold (0-100000; omitted keeps the
             existing fare)

Examples:
  setschedule FerryRoute 2 10 - Depart every 2 hours and charge 10 gold
  setschedule PatrolRoute 6   - Depart every 6 hours, preserving its fare

Requirements:
  - You must be aboard a vessel with autopilot capability
  - You must be the captain or at the helm, and meet the hull\'s level
    (see SHIPBROWSE)
  - The route must exist and have waypoints defined

Notes:
  - One MUD hour equals approximately 75 real seconds
  - If a pilot is assigned, departures will be announced
  - The schedule and passenger fare persist across server restarts
  - Fares on privately owned vessels are inactive
  - NPC crew do not pay; staff using ordinary BOARD do pay
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
  - Public-vessel passenger fare
  - Schedule status (Active, Paused, or Disabled)
  - Pilot status (whether departures will be announced)

Example output:
  --- Vessel Schedule ---
  Route: FerryRoute
  Interval: Every 2 MUD hours
  Next Departure: MUD hour 14
  Current Time: MUD hour 12
  Passenger Fare: 10 gold per boarding
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
  The success message prints the new numeric vehicle ID; record it for purge
  operations.

VEHICLEPURGE <vehicle-id>
  Permanently remove an active vehicle and its database record. All passengers
  must dismount first. VEHICLEPURGE remains available while the vessel-system
  kill switch is off so staff can recover bad state.

Use VSTATUS to inspect a nearby vehicle\'s condition and state. VSTATUS does
not print its ID; obtain the ID from VEHICLECREATE\'s success message.

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
- You must be the captain of the vessel and meet the hull\'s level
  (see SHIPBROWSE)
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

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('VEVENT', 'Usage: vevent <action>

Public actions:
  vevent status
    Show the active event, elapsed time, course or fleet contacts, roster,
    placements, and scores.

  vevent join [red|blue]
    Enter the vessel you are piloting. Regatta entries must begin at the start
    coordinate. Skirmish entries must choose the red or blue fleet.

  vevent leaderboard [regatta|skirmish|ghost]
    Show the top ten captains by wins, points, and best regatta time. With no
    event type, show all three leaderboards.

Staff actions:
  vevent start regatta <finish-x> <finish-y>
  vevent start skirmish
  vevent start ghost <warship-prototype-id> [count 1-5]
  vevent enlist <ship-slot> <red|blue>
  vevent end | cancel | recover

Regattas award placement points when a vessel first enters the exact finish
coordinate. Fleet skirmishes score damage between opposing enlisted teams.
Ghost fleets create temporary public warships and score damage plus sink
bonuses. END records the leaderboard; CANCEL closes without recording it.

Only one event may be open at a time. Events close after one hour. Ghost hulls
are tracked durably and retired on END, CANCEL, or restart recovery.

See also: VESSELS, SHIPFIRE, SHIPLIST, CONTACTS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('VEVENT', 'VEVENT');

INSERT INTO help_entries (tag, alternate_keywords, entry, min_level, auto_generated)
VALUES ('boats', NULL, 'Passage on ferries and other public ships:

Public ships, the ferries and merchant hulls an NPC pilot sails on a
schedule, carry passengers between ports. Wait on the dock until she is
moored there, then BOARD her by a word of her name (BOARD FERRY). A
scheduled ferry may charge a fare as you board; the purser takes it from
the gold you carry.

Aboard, walk to the bridge to watch the passage with LOOKOUT, or see where
she is with SHIPSTATUS; SHOWSCHEDULE tells her next departure and fare. Her
pilot holds the helm: passengers cannot steer, stop, or reroute her. Stay
aboard through a stop and she carries you on round her route.

Step off with DISEMBARK while she is moored or casting off at a dock. No one
leaves a hull under way; away from shore, leaving a stopped hull puts you in
the water.

See also: VESSELS, SHOWSCHEDULE, VEHICLE-TRANSPORT', 1, FALSE)
ON DUPLICATE KEY UPDATE alternate_keywords = VALUES(alternate_keywords),
  entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('boats', 'BOATS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('boats', 'FERRY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('boats', 'FERRIES');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('boats', 'PASSAGE');

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

-- The passenger entry gave SHIPS to VESSELS and lost a misspelled keyword;
-- its old alternate keywords are cleared above.
DELETE FROM help_keywords
WHERE
  BINARY help_tag = 'boats'
  AND UPPER(keyword) IN ('SHIPS', 'TRANSPORTSS');

-- SHIPWAGES was retired when crew became a one-time hire.
DELETE FROM help_keywords
WHERE
  BINARY help_tag = 'SHIPHIRE'
  AND UPPER(keyword) = 'SHIPWAGES';

-- SHIPINSURE was retired when insurance became automatic.
DELETE FROM help_keywords
WHERE
  BINARY help_tag = 'SHIPHIRE'
  AND UPPER(keyword) = 'SHIPINSURE';
