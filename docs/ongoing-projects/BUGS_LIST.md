# Production In-Game Bug Backlog

This is a read-only snapshot of the production `bug submit <header>` queue,
retrieved on 2026-08-03. The production file contained 146 submissions: 136
unresolved, five in progress, and five resolved.

Obvious duplicate and continuation reports are consolidated below. This turns
the 146 source records into 129 distinct entries. Long logs, repeated text, and
non-actionable joke or crude filler were summarized, but no distinct actionable
report was discarded. Queue status is copied from production and is not a fresh
verification against the current source or world data.

Reporter metadata uses `L` for character level, `R` for room VNUM, and `#` for
the record's position in this snapshot. Queue positions can change when the
production list is edited.

## In Progress

- **Magic Stone produces corrupt output** - The spell prints its explosion
  message followed by random codes and numbers; staff notes say it may now work
  only against player targets. Reporter: Andross (L7, R6763, 2022-07-14; #001).
  This record is also flagged important.
- **Obscuring Mist stacks with Self Concealment** - `resist` reports 40 percent
  concealment when both effects are active. Staff left this pending a design
  decision because Obscuring Mist is a room effect. Reporter: Fei (L27,
  R145308, 2022-10-18; #005).
- **False Life appears one level early** - A level 4 sorcerer with the undead
  bloodline can access False Life although the help entry says level 5. Staff
  asked whether the character used a premade build. Reporter: Arcavius (L4,
  R145330, 2022-10-18; #006).
- **Dollhouse quest eyes have the wrong size** - The Blind Girl quest gives
  eyes that may need resizing before use. Staff considered adding DG Script
  size support or making the relevant slot ignore size. Reporter: Levud (L15,
  R11854, 2022-11-03; #009).
- **Dollhouse vortex mobs can be farmed at fixed respawn points** - Scripted
  deaths respawn mobs in designated rooms, making the behavior exploitable.
  Reporter: Zusuk (L34, R11885, 2022-11-10; #010). This record is also flagged
  important.

## Unresolved

### 2022

- **Early password input causes a TELNET echo loop** - Sending the password in
  the same packet as the username repeatedly negotiates `DO/WILL/DONT/WONT
  ECHO`. Reporter: Yarea (L4, R145348, 2022-08-10; #003).
- **Clairvoyance applies remote falling damage** - Looking at a character on
  the Eternal Staircase with Clairvoyance moved the caster through a falling
  sequence and dealt damage. Reporter: Hibbidy (L26, R145200, 2022-09-03;
  #004).
- **Displayed damage reduction exceeds applied reduction** - Feats displayed
  21 DR while combat rolls applied only 18. Reporter: Melow (L29, R145200,
  2022-10-23; #007).
- **Dead mobs can still bash** - A mob bashed the player immediately after the
  mob died. Reporter: Chentu (L15, R148112, 2022-10-27; #008).
- **Psionic durations are one level short** - Sharpened Edge documented as ten
  minutes per psionic level lasted 40 minutes for a level 5 psionicist.
  Reporter: Murdoch (L19, R14100, 2022-11-12; #012).
- **Total Defense tanks can immediately lose aggro** - A tank opened with
  Backstab while Total Defense was active, but the target switched to the first
  group member who auto-assisted. Reporter: Murdoch (L30, R23802, 2022-11-14;
  #013).
- **Cleric domain spells omit their circle** - The domain-spell display does
  not identify each spell's circle. Reporter: Zusuk (L34, R110409, 2022-11-14;
  #014).
- **Vampire and lich transformations change racial size** - The transformed
  character does not retain the original race size. Reporter: Zusuk (L34,
  R1208, 2022-11-14; #015).
- **Paladin Channel grants Smite to enemies** - Channel applies Smite to the
  whole room, including active opponents, although its messaging describes
  allies. Reporter: Darthok (L25, R24719, 2022-11-17; #016).
- **Sorcerer can exceed spells-known limit** - One character showed six of five
  third-circle spells selected after increasing Charisma at level 16.
  Reporter: Darthok (L28, R30923, 2022-11-18; #017).
- **Lich Touch cannot target self in cramped rooms** - Room size restrictions
  prevent self-use of Lich Touch. Reporter: Diel (L30, R125472, 2022-11-28;
  #018).
- **Circle and Backstab reject paralyzed targets** - The maneuvers do not work
  against a paralyzed mob. Reporter: Diel (L30, R129563, 2022-11-29; #019).
- **Teamwork feat can block level gain** - Taking the flanking teamwork feat
  left a rogue unable to gain because the game still requested spending feats
  from the previous level; an inquisitor did not reproduce it. Reporter:
  Kittles (L10, R40431, 2022-12-01; #020).
- **Flower Tiara quest kills do not count** - Killing the requested garden
  faeries leaves the objective at five of five remaining. Reporter: Badase
  (L30, R33009, 2022-12-12; #021).
- **Flower Tiara quest gives the wrong hand-in cue** - The petals must be given
  to the princess, but the quest giver does not make that destination clear.
  Reporter: Badase (L30, R33002, 2022-12-12; #022).
- **Song of Flight does not apply its affect** - The performance appears not to
  add the expected affect. Reporter: Zusuk (L34, R1206, 2022-12-15; #023).
- **Hood of Swirling Clouds identifies as heavy full-plate armor** - The hood
  unexpectedly identifies as a full-plate helm. Reporter: Ertai (L28, R23831,
  2022-12-17; #024).
- **Tutorial rooms and NPCs appear inert** - A new player found many tutorial
  rooms and NPCs that did not respond or progress anything. Reporter: Whil
  (L1, R14124, 2022-12-22; #025).
- **New characters can lack documented starter equipment** - Tutorial content
  refers to a backpack, crafting kit, and rations that were not received; a
  later player likewise had no crafting kit. Reporters: Whil (L2, R145203,
  2022-12-22; #026) and Clanorth (L3, R145355, 2026-01-11; #132).
- **Psionic Vigor has no help entry** - The spell cannot be found in help or
  from the study menu. Reporters: Ungol (L23, R40600, 2022-12-25; #027) and
  Darowin (L1, R14100, 2024-08-01; #097).
- **Paralysis blocks every command, including communication and quit** - A
  trap-paralyzed character could not chat, inspect affects, or leave the game.
  Reporter: Lydia (L21, R14100, 2023-01-04; #028).
- **Tower door cannot be opened with any apparent keyword** - A door in room
  132664 advertises a keyword but did not accept any keyword the reporter could
  identify. Reporter: Badase (L30, R132664, 2023-01-11; #029).
- **Axe of Calamit has negligible proc damage** - Its proc dealt 11 damage
  after the target failed the save. Reporter: Badase (L30, R1877, 2023-01-16;
  #030).
- **Items given to link-dead characters may not save** - The report warns that
  transferred items can be lost when the recipient is link-dead. Reporter:
  Zzridt (L14, R145307, 2023-01-26; #031).

### 2023

- **Rum Runners has no quest information** - Joining Ybic's Rum Runners quest
  gives no description, objective, or location. Reporters: Talendor (L25,
  R105096, 2023-01-26; #032), Neurrone (L23, R105096, 2023-03-26; #052), and
  Galeron (L20, R1004010, 2023-12-19; #095).
- **Hideous Blow damage and mode handling are broken** - The invocation prints
  an Eldritch Blast message without apparent blast or essence damage, and a
  later report could not disable the mode to resume normal blasts. Reporters:
  Astoret (L8, R145379, 2023-01-28; #033) and Almund (L3, R145357, 2023-08-12;
  #080).
- **Encounters can spawn in a waypoint peace room** - Random encounters can
  appear in the portal waypoint room, where combat restrictions may break
  encounter behavior. Reporter: Zusuk (L34, R1000115, 2023-02-03; #034).
- **Vampire quest final step no longer works** - The final step previously
  worked but no longer progresses. Reporter: Zusuk (L34, R369, 2023-02-08;
  #035).
- **Group Heal does not cure blindness** - Two casts failed to remove a group
  member's blindness, while the single-target Heal spell removed it. Reporter:
  Arithon (L30, R109560, 2023-02-10; #036).
- **Elemental Swarm and Shambler summon level 7 creatures** - The ninth-circle
  druid spells always create level 7 summons. Reporter: Arithon (L30, R1004004,
  2023-02-14; #037).
- **Sleeves of Liquid Fire override a larger HP enhancement** - Their +72
  Max-HP enhancement replaces a stronger +120 enhancement and lowers total HP.
  Reporter: Arithon (L30, R145200, 2023-02-16; #038).
- **`cexchange` breaks Sneak and Hide** - Currency exchange appears to remove
  both stealth states. Reporter: Zyloch (L30, R132262, 2023-02-19; #039).
- **Aasimar Healing Hands exceeds its daily limit** - Regeneration could be
  cast more than the documented three times per day. Reporter: Lothelye (L10,
  R103009, 2023-02-19; #040).
- **Level 30 Berserker has only 20 BAB** - The class display showed level 30
  Berserker with base attack bonus 20. Reporter: Kharadmon (L30, R14100,
  2023-02-22; #041).
- **Tethyr Bandit Castle can trap players** - A guard blocks the exit from a
  peace, no-teleport room and also prevents leaving after entry. Reporters:
  Arithon (L30, R143306, 2023-02-25; #042) and Bragollach (L27, R143306,
  2023-09-27; #090).
- **Monk/Weaponmaster expanded critical range is ignored** - Bare-hand threat
  range displayed 16, but an 18 did not crit while a natural 20 did. Reporter:
  Arithon (L30, R196004, 2023-02-26; #043).
- **See the Unseen lasts one round instead of 24 hours** - The applied duration
  is far shorter than its help entry. Reporter: Yisan (L1, R145390, 2023-03-12;
  #044).
- **Dark One's Own Luck adds the full Charisma score** - The invocation adds
  the complete Charisma value to saves instead of the ability modifier.
  Reporters: Yisan (L14, R6740, 2023-03-14; #045) and Aelin (L7, R145354,
  2023-04-16; #062).
- **`walkto` does not open closed doors** - Automated travel stopped at a
  closed door on the route to the library. Reporter: Neurrone (L3, R103046,
  2023-03-20; #046).
- **Dark Foresight appears to do nothing** - It adds no visible affect and
  changes neither `resist` nor `damagereduction`. Reporter: Yisan (L15, R6758,
  2023-03-22; #047).
- **Warlocks cannot cast Eldritch Blast through the cast command** - Casting
  the named ability returns "You are not even a caster" with or without a
  target. Reporter: Kavari (L9, R103320, 2023-03-22; #048).
- **Warlock Charm gives no effect or failure feedback** - Attempts on multiple
  mobs produced neither a charm nor an explanatory error. Reporter: Kavari
  (L9, R5913, 2023-03-22; #049).
- **Walk Unseen is absent from the affects display** - The invocation shows no
  visible affect and may not apply any effect. Reporters: Kavari (L9, R6778,
  2023-03-22; #050) and Ogoun (L24, R145202, 2023-04-07; #054).
- **Autoloot misses gold when no corpse is created** - Gold is not collected
  for corpse-less mobs or kills caused by Cloudkill. Reporter: Neurrone (L18,
  R40604, 2023-03-25; #051).
- **Kill output awards XP and then reports zero XP** - Killing a Mystic
  Darkling printed both a 2,500 XP award and a final zero-experience line.
  Reporter: Gor (L12, R40428, 2023-04-02; #053).
- **Devour Magic has no apparent effect** - Successful casts against a buffed
  Gnar Shaman produced no reaction across multiple attempts. Reporter: Ogoun
  (L24, R101772, 2023-04-07; #055).
- **Shade Stealth remains cross-class** - The Shade racial feat does not make
  Stealth a class skill. Reporters: Moriens (L1, R145202, 2023-04-09; #056) and
  Therius (L1, R14101, 2023-08-20; #084).
- **Bazaar cloth armor applies only +1 AC enhancement** - Cloth gear ordered
  with a +6 enhancement identifies appropriately but most pieces appear to
  grant only +1 AC. Reporter: Ogoun (L30, R145200, 2023-04-11; #057).
- **Shifter forms do not apply their AC bonus** - Shape AC is missing, while
  shape attribute enhancements replace rather than stack with item
  enhancements. Reporter: Ogoun (L30, R103000, 2023-04-11; #058).
- **`push` cannot manipulate scripted objects** - The command could not push a
  workbench used to access the Underdark. Reporter: Ogoun (L30, R40629,
  2023-04-14; #059).
- **Warlock Darkness lasts only about three rounds** - Help says 15 rounds and
  tabletop behavior is one minute per level, but the effect expired after
  roughly 18 seconds. Reporter: Aelin (L6, R145294, 2023-04-15; #060).
- **Multi-skill affects retain only the last skill** - Beguiling Influence
  buffs only Intimidate instead of Intimidate, Bluff, and Diplomacy;
  Otherworldly Whispers similarly retains only Spellcraft. Reporter: Aelin
  (L7, R145372/R145354, 2023-04-16; #061, #064).
- **Dark One's Own Luck uses the wrong bonus type** - It grants a morale bonus
  rather than a luck bonus. Reporter: Aelin (L7, R145354, 2023-04-16; #063).
- **Psionic Blast announces stun without stunning** - A target that fails its
  save attacks immediately after the stun message. Reporter: Ogoun (L30,
  R145206, 2023-04-19; #065).
- **Summoner Conjure cannot prepare an open spell slot** - Conjuring Mage Armor
  reports that no more spells of the circle can be retained despite all six
  slots being open. Reporter: Auset (L3, R145294, 2023-04-20; #066).
- **Large evolution does not function** - Selecting the eidolon Large evolution
  has no effect. Reporter: Elhaym (L5, R5938, 2023-04-20; #067).
- **Stunning Critical announces stun without stunning** - The target attacks on
  the line immediately following the stun message. Reporter: Ogoun (L30,
  R196012, 2023-04-20; #068).
- **Eidolon Bond rejects a present eidolon** - The command says the eidolon
  must be summoned and in the room even when it is visibly present. Reporter:
  Auset (L16, R103277, 2023-04-22; #069).
- **Eidolon form and attribute evolutions do not change stats** - Quadruped form
  and two Strength/Constitution evolutions left the eidolon's attributes
  unchanged. Reporter: Auset (L16, R103277, 2023-04-22; #070).
- **Alchemist Concoct rejects an Alchemist** - The command responds with "Try
  changing professions" for a character of that profession. Reporter: Ogoun
  (L11, R145268, 2023-04-25; #071).
- **Purified Calling heals only 40 HP** - At level 30 the eidolon ability heals
  40 instead of its documented `(caster level * 10) + 20`, or 320. Reporter:
  Ogoun (L30, R196004, 2023-04-25; #072).
- **Eidolon ownership breaks across logout** - A returning eidolon remains in
  the room but cannot be controlled; calling another can leave two eidolons.
  Reporter: Ogoun (L30, R196004, 2023-04-26; #073).
- **Score load can become highly negative** - Weightless-container behavior can
  drive the displayed carried load below zero. Reporter: Ogoun (L30, R145200,
  2023-04-26; #074).
- **Compact toggle has no effect** - Enabling or disabling compact mode does not
  change output. Reporter: Wolves (L33, R1204, 2023-04-26; #075).
- **Eidolons may lack their documented darkvision** - The expected 60-foot
  darkvision does not appear to be present. Reporter: Khell (L28, R145201,
  2023-05-12; #076).
- **Ghost Wolf has no summon-count limit** - A caster can create an apparently
  unlimited number of wolves. Reporter: Kavari (L12, R102526, 2023-05-13;
  #077).
- **Warlock invocation selection exceeds its limit** - More invocations can be
  selected than allowed, driving the remaining count negative. Reporter: Ogoun
  (L30, R14100, 2023-05-27; #078).
- **`temote` rejects documented syntax** - The help example, including attempts
  with names substituted for `#M`, returns "Huh?" Reporter: ErrnieElvin (L1,
  R14103, 2023-06-23; #079).
- **`gain <class>` can level the wrong class** - `gain cleric` at third level
  advanced Warrior instead. Reporter: Zylese (L3, R145373, 2023-08-13; #081).
- **Crafting XP messages disagree with actual gains** - A message promises 12
  XP while TNL drops by 30, and crafting a wooden shield can improve
  Leatherworking. Reporter: Zylese (L6, R370, 2023-08-19; #082).
- **Return key repeats pagination prompt** - On long output, Return repeats the
  prompt instead of advancing; entering the next page number is required.
  Reporter: Zylese (L6, R370, 2023-08-19; #083).
- **Charmed encounter mob attacks itself and blocks departure** - A charmed
  Ogrillion could not be left with `encounter depart`, attacked itself, and
  continued until death after `encounter bluff` allowed departure. Reporter:
  Bragollach (L10, R1004010, 2023-09-10; #085).
- **Death effects grant account XP without killing high-HP targets** - Recall
  Death and Psychic Crush can deal a capped 1,499 damage, award account XP, and
  leave severe or hunt targets alive. Reporter: Iliri (L19, R1004006,
  2023-09-20; #086).
- **Prismatic Spray blindness persists through death** - A lethal cast can
  leave the victim blind after resurrection. Reporter: Mallyrn (L18, R14100,
  2023-09-25; #087).
- **Rapid purchases with a full inventory can destroy inventory contents** -
  Repeatedly buying copper before an autocraft alias used a kit caused all
  non-material inventory, including bags and their contents, to disappear.
  Reporter: Bragollach (L27, R369, 2023-09-25; #088).
- **Finger of Death can be resisted despite its help entry** - The spell help
  says magic resistance does not apply, but targets can resist it. Reporter:
  Mallyrn (L21, R1004000, 2023-09-27; #089).
- **Hunts can return body parts from the wrong species** - A Barghest hunt can
  request or produce Banshee parts. Reporter: Iliri (L28, R1004004, 2023-10-02;
  #091).
- **Litany of Righteousness dazzles its Paladin user** - A good-aura caster
  receives dazzle messages, and apparently the effect, when using the litany.
  Reporter: Gwyndaryn (L11, R40606, 2023-10-04; #092).
- **First `supplyorder new` is blocked until tomorrow** - A character with no
  previous supply order receives the daily cooldown message. Reporter: Galeron
  (L10, R370, 2023-12-18; #093).
- **Eidolon acid attack prints `<NULL>`** - The combat line identifies the acid
  source as `<NULL>` instead of the eidolon. Reporter: Galeron (L12, R1966,
  2023-12-18; #094).

### 2024

- **Carriage travel disconnects or crashes the game** - Using the carriage to
  travel consistently caused a crash or disconnection. Reporters: Taurus (L2,
  R145387, 2024-04-10; #096) and Fierend (L4, R145387, 2024-09-07; #104).
- **Stats respec has no help entry** - `help stats respec` cannot be found.
  Reporter: Darowin (L5, R548, 2024-08-01; #098).
- **Newbie quest 4 target can be killed during quest 3** - Killing the Skirker
  early leaves the next quest unable to progress or respawn its target.
  Reporter: Asterix (L1, R14109, 2024-08-14; #099).
- **Rules-of-conduct hint misspells "developers"** - The final parenthetical
  line contains the misspelling `developres`. Reporter: Asterix (L1, R14123,
  2024-08-14; #100).
- **Depleted harvesting nodes still yield material** - A node described as
  depleted could still be mined successfully. Reporter: Alost (L1, R145207,
  2024-09-02; #101).
- **Character list can disappear while old equipment carries into a new
  character** - After login showed an empty character list, a newly created
  character retained gear bought and equipped by the missing character.
  Reporter: Alost (L1, R14101, 2024-09-04; #102).
- **Quest history descriptions are cut off** - Line wrapping truncates quest
  history text with no apparent way to expand or reformat it. Reporter:
  Fernidand (L2, R145207, 2024-09-05; #103).
- **First Step Is the Hilt loops the lumber hand-in** - Giving Great Oak wood
  twice repeats the lumberjack stage and produces neither the hilt nor further
  instructions. Reporter: Duirren (L7, R6777, 2024-09-07; #105).
- **Ordering followers to `greport` crashes the client** - Grouping and sitting
  followers works, but requesting their health/stamina report crashes the
  reporter's client. Reporter: Lynx (L10, R40405, 2024-09-11; #106).

### 2025

- **Newbie wizard alchemist cabinet cannot be opened** - The cabinet is visible
  and its key can be obtained, but `unlock cabinet` and `open cabinet` report no
  such object. Reporter: Eaion (L3, R5933, 2025-01-31; #107).
- **Crafting tutorial duplicates rings and copper** - The tutorial allowed two
  rings and awarded four surplus copper. Reporter: Mddljeu (L2, R14107,
  2025-07-05; #108).
- **East exit to Road to Mosswood Village is blocked** - A player could not
  leave east into the road. Reporter: Oxyo (L1, R145310, 2025-08-13; #109).
- **Rogues lack composite shortbow proficiency** - A rogue wielding a composite
  shortbow receives a not-proficient warning despite expected shortbow
  proficiency. Reporter: Kazne (L2, R145207, 2025-09-18; #110).
- **Ghost Wolf loses behavior and cannot be mounted** - The summon can stop
  following or attacking, and cannot be mounted despite the help entry and
  valid size difference. Dismissal and resummoning only help temporarily.
  Reporter: Syman (L7/L18, R145352/R148130, 2025-11-26/27; #111, #114).
- **Harvest rejects every node outside the wilderness** - Ore and other placed
  harvesting nodes in normal zones say harvesting is allowed only in the
  outworld. Reporters: Syman (L7, R145373, 2025-11-26; #112) and Gerok (L10,
  R406, 2026-07-30; #146).
- **Eidolon recovery prints Strength of Honor text** - When Call Eidolon becomes
  available, the recovery message names a Strength of Honor use instead.
  Reporter: Syman (L10, R6727, 2025-11-26; #113).
- **Underdark boulder remains after being pushed** - The boulder reports moving
  and the north secret can be found, but its room description remains
  unchanged. Reporter: Syman (L19, R102516, 2025-11-27; #115).
- **Blinking color state leaks from character creation** - Blinking text from
  character creation persisted across the character's exits, notes, hints,
  portal, and room displays, while another character on the account rendered
  normally. Reporter: Ity (L1, R14100, 2025-11-27; #116, #117).
- **Waterwalk and an inventory raft do not satisfy Trollcave pond entry** - The
  exit still says a boat is required. Reporter: Syman (L20, R6750, 2025-11-28;
  #118).
- **A spell cannot be aborted while it is being cast** - `abort` responds that
  the character is too busy casting. Reporter: Syman (L24, R23807, 2025-11-29;
  #119).
- **Enemy summons do not assist their summoner** - A creature summoned by a mob
  remains out of the summoner's fight. Reporter: Tsoli (L7, R6767, 2025-11-30;
  #120).
- **Neutral Channel Energy reports success without healing** - With healing the
  living selected, neither the caster nor a charmee receives HP. Reporter:
  Tsoli (L9, R145370, 2025-11-30; #121).
- **Damp Stone Passage exit is never hidden as described** - The quest says the
  upward exit must be searched for, but it is always visible and `search` does
  nothing. Reporter: Tsoli (L9, R5931, 2025-11-30; #122).
- **Built on the Doorstep of the Darklings cannot recognize prior progress** -
  Completing an objective before accepting the quest leaves the accepted quest
  unable to advance. Reporter: Tsoli (L10, R6705, 2025-11-30; #123).
- **Blackguard Call Mount is unusable** - The feat is displayed, but the command
  always says the character does not remember how to call the creature.
  Reporters: Falwel (L7, R103165, 2025-12-01; #124) and Uhmbrall (L8, R6758,
  2026-04-22; #138).
- **Graven camp well looks full but acts empty** - Examining the well shows
  liquid, while `drink well` says it is empty. Reporter: Falwel (L7, R6722,
  2025-12-02; #125).
- **Blackguard spell list resolves to the wrong class** - Before multiclassing,
  bare `spells` shows nothing; after adding Warlock, even `spells blackguard`
  shows Warlock spells. Reporter: Falwel (L9, R40400, 2025-12-03; #126).
- **Ravanda's spellcasting tutorial ends silently** - After asking the player
  to cast Shield, the tutorial gives no completion or next-step message.
  Reporter: Frondbrau (L1, R14104, 2025-12-05; #127).
- **Warlock `spells` shows only first-circle spells** - Learned higher-circle
  invocations or spells are omitted. Reporter: Zori (L5, R145293, 2025-12-10;
  #128).
- **High Wizard's Summoning Chamber moves only the pet** - Pulling the loose
  torch does nothing to the player, while the pet eventually disappears into
  the destination quest room. Reporter: Zori (L9, R5941, 2025-12-13; #129).
- **Boon Companion does not remove the Ranger level penalty** - Taking the feat
  left the companion five levels below the character and may have reduced its
  effective power. Reporter: Bijori (L9, R6758, 2025-12-13; #130).
- **Magic Fang spells fail on wild-shaped druids** - Magic Fang and Greater
  Magic Fang do not affect a druid transformed into a beast. Reporter: Luos
  (L9, R40431, 2025-12-14; #131).

### 2026

- **Within the Roots leaves a character alive at extreme negative HP** -
  Entering Spider Swamp room 1918 set HP to -99,668 without completing death,
  leaving the character unable to act. Reporter: Dakar (L17, R1918,
  2026-02-01; #133).
- **Death at Blindbreak Rest does not teleport the corpse** - An Orb of the
  Protector kill left the character dead in room 40412 for hours, with every
  command rejected. Reporter: Ryt (L10, R40412, 2026-02-02; #134).
- **Crisrion shop rejects `list`** - The shopkeeper does not recognize the list
  command. Reporter: Elumanate (L8, R103498, 2026-03-25; #135).
- **Weapon of Flame `pole` choice creates a quarterstaff** - A 2022 report was
  marked resolved after testing, but a 2026 player again interpreted `pole` as
  polearm and received a quarterstaff. Reporters: Maloc (L8, R6769,
  2022-07-29; #002) and Elumanate (L9, R6769, 2026-03-27; #136).
- **Eidolon Basic Magic cannot cast** - After purchasing Basic Magic for an
  eidolon, ordered casts report that it cannot use magic. Reporter: Grimthur
  (L10, R29002, 2026-04-14; #137).
- **Combat targets the first mob instead of the named mob** - In rooms with
  multiple mobs, attacks always select the top-listed mob regardless of the
  target argument. Reporter: Gerok (L5, R9615, 2026-07-25; #142).
- **`help arcana` opens Arcana Golem instead** - Exact help lookup selects the
  longer entry and makes the Arcana skill help difficult to reach. Reporter:
  Mjodvitnir (L16, R11809, 2026-07-26; #143).
- **Followers reconnect in inconsistent states** - After logout, a hired
  follower becomes an untargetable ghost and a spell follower is visible but
  uncontrollable; a druid companion continues normally. Reporter: Gerok (L9,
  R6746, 2026-07-28; #144).
- **Short page length cuts help text incorrectly** - With page length 22,
  `help vampire` breaks at the page boundary and displays truncated output.
  Reporter: Zusuk (L34, R1204, 2026-07-29; #145).

## Resolved in the Production Queue

- **Spider Swamp automap appears geometrically inconsistent** - Staff marked
  this resolved as intended behavior: builders are not required to map zones
  onto a two-dimensional field. Reporter: Murdoch (L18, R14100, 2022-11-11;
  #011).
- **Crafting Mold Shop item 2 looked like the wrong mold** - The suspected
  bracer-mold listing was checked and is now correct. Reporter: Daeren (L16,
  R377, 2026-05-25; #139).
- **Ashenport Bazaar sign listed wrong minimum levels** - The enhancement-tier
  sign did not match purchased item levels; staff corrected it to levels 1, 5,
  10, 15, 20, and 25. Reporter: Ralmont (L30, R103006, 2026-07-23; #140,
  #141).
