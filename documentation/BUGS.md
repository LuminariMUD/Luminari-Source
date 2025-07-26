# LuminariMUD Bug List

This file contains confirmed bugs that need to be fixed. Suggestions and typos are tracked separately.

## Game Mechanics Bugs

### Magic Stone Spell
- **Description**: Magic stone gives explosion message followed by random codes and numbers
- **Notes**: Previously working, now only works on PC targets
- **Reported by**: Andross (Level 7)

### Sending Password Too Early
- **Description**: Sending password in same packet as username causes echo loop (DO ECHO, WILL ECHO, DONT ECHO, WONT ECHO)
- **Reported by**: Yarea (Level 4)

### Clairvoyance Fall Damage
- **Description**: Casting clairvoyance on eternal staircase causes fall damage
- **Reported by**: Hibbidy (Level 26)

### Damage Reduction Display
- **Description**: Feats show 21 DR but combat rolls only show 18
- **Reported by**: Melow (Level 29)

### Post Death Bash
- **Description**: Mob can bash player immediately after mob's death
- **Reported by**: Chentu (Level 15)

### Psionic Duration
- **Description**: Sharpened edge shows 40 minutes instead of 50 (10 mins/level at level 5)
- **Reported by**: Murdoch (Level 19)

### Total Defense Aggro Loss
- **Description**: Tank in total defense can lose aggro even after backstab initiation
- **Reported by**: Murdoch (Level 30)

### Paladin Channel Bug
- **Description**: Paladin channel grants smite to entire room including enemies
- **Reported by**: Darthok (Level 25)

### Sorcerer Spell Uses
- **Description**: Shows "6 of 5 selected" for 3rd circle spells after charisma increase
- **Reported by**: Darthok (Level 28)

### Lich Touch Range
- **Description**: Cannot reach self in cramped rooms
- **Reported by**: Diel (Level 30)

### Backstab/Circle on Paralyzed
- **Description**: Backstab/circle doesn't work on paralyzed mobs
- **Reported by**: Diel (Level 30)

### Teamwork Feat Bug
- **Description**: Taking teamwork feat for flanking prevents rogue from gaining levels
- **Reported by**: Kittles (Level 10)

### Paralysis Lock
- **Description**: When paralyzed, cannot interact with game at all, including quit command
- **Reported by**: Lydia (Level 21)

### Linkdead Item Transfer
- **Description**: Items given to linkdead players may not save
- **Reported by**: Zzridt (Level 14)

### Encounters in Peace Rooms
- **Description**: Encounters spawning in waypoint/portal peace rooms may cause issues
- **Reported by**: Zusuk (Level 34)

### Vampire Quest Final Step
- **Description**: Vampire quest not working on final step
- **Reported by**: Zusuk (Level 34)

### Group Heal Blindness
- **Description**: Group heal doesn't cure blindness despite heal working
- **Reported by**: Arithon (Level 30)

### Druid 9th Circle Summons
- **Description**: Elemental swarm and Shambler only summon level 7 creatures
- **Reported by**: Arithon (Level 30)

### Sleeves of Liquid Fire HP Bug
- **Description**: +72 HP enhancement overrides higher HP enhancement items
- **Reported by**: Arithon (Level 30)

### Cexchange Stealth Break
- **Description**: Cexchange command breaks sneak and hide
- **Reported by**: Zyloch (Level 30)

### Berserker BAB
- **Description**: Level 30 Berserker shows BAB 20 instead of expected value
- **Reported by**: Kharadmon (Level 30)

### Tethyr Bandit Castle Trap
- **Description**: !tele peace room with guard blocking exit creates inescapable trap
- **Reported by**: Arithon (Level 30)

### Monk/Weaponmaster Critical
- **Description**: Threat range 16 doesn't crit on 18 roll
- **Reported by**: Arithon (Level 30)

### See the Unseen Duration
- **Description**: Help says 24 hours, actual duration is 1 round
- **Reported by**: Yisan (Level 1)

### Dark Ones Own Luck
- **Description**: Grants total CHA value instead of CHA bonus to saves
- **Reported by**: Yisan (Level 14), Aelin (Level 7)

### Dark Ones Own Luck Bonus Type
- **Description**: Provides Morale bonus instead of Luck bonus
- **Reported by**: Aelin (Level 7)

### Walkto Door Opening
- **Description**: Walkto command doesn't open closed doors
- **Reported by**: Neurrone (Level 3)

### Dark Foresight
- **Description**: Doesn't show affect or appear in resist/damagereduction commands
- **Reported by**: Yisan (Level 15)

### Eldritch Blast Casting
- **Description**: Returns "You are not even a caster!" error
- **Reported by**: Kavari (Level 9)

### Warlock Charm
- **Description**: No feedback or error messages when used
- **Reported by**: Kavari (Level 9)

### Walk Unseen Display
- **Description**: Doesn't show in affects list
- **Reported by**: Kavari (Level 9), Ogoun (Level 24)

### Gold Auto-loot
- **Description**: Gold not picked up when mob has no corpse or killed by cloudkill
- **Reported by**: Neurrone (Level 18)

### Devour Magic
- **Description**: No reaction when cast on buffing mobs
- **Reported by**: Ogoun (Level 24)

### Shade Stealth
- **Description**: Not acting as class skill for warlock shade
- **Reported by**: Moriens (Level 1)

### Quest Point Cloth Gear
- **Description**: +6 enhancement only gives +1 AC bonus
- **Reported by**: Ogoun (Level 30)

### Shifter Shapes AC
- **Description**: Armor class bonus not applied, attribute enhancement doesn't stack
- **Reported by**: Ogoun (Level 30)

### Push Command
- **Description**: Unable to push objects to manipulate them
- **Reported by**: Ogoun (Level 30)

### Warlock Darkness Duration
- **Description**: Lasts 15 seconds instead of 1 min/level
- **Reported by**: Aelin (Level 6)

### Beguiling Influence
- **Description**: Only affects Intimidate, not Bluff/Diplomacy
- **Reported by**: Aelin (Level 7)

### Multiple Skill Bonuses
- **Description**: Spells providing multiple skill bonuses only apply last one
- **Reported by**: Aelin (Level 7)

### Psionic Blast Stun
- **Description**: Shows stun message but mob attacks immediately
- **Reported by**: Ogoun (Level 30)

### Summoner Conjure
- **Description**: Returns "can't retain more spells" with open slots
- **Reported by**: Auset (Level 3)

### Large Sized Evolution
- **Description**: Currently doesn't function
- **Reported by**: Elhaym (Level 5)

### Stunning Critical
- **Description**: Shows stun message but mob attacks immediately
- **Reported by**: Ogoun (Level 30)

### Eidolon Bond
- **Description**: Command doesn't work even with eidolon in room
- **Reported by**: Auset (Level 16)

### Eidolon Attributes
- **Description**: Attribute evolutions and form boosts don't apply
- **Reported by**: Auset (Level 16)

### Alchemist Concoct
- **Description**: Returns "Try changing professions" error
- **Reported by**: Ogoun (Level 11)

### Eidolon Healing
- **Description**: Purified Calling heals 40 instead of (level*10)+20
- **Reported by**: Ogoun (Level 30)

### Eidolon Persistence
- **Description**: Logging out breaks summoner-eidolon connection
- **Reported by**: Ogoun (Level 30)

### Score Load Negative
- **Description**: Load can go massively negative with weightless bags
- **Reported by**: Ogoun (Level 30)

### Compact Toggle
- **Description**: Toggling compact has no effect
- **Reported by**: Wolves (Level 33)

### Ghost Wolf Unlimited
- **Description**: No limit to number of ghost wolves summoned
- **Reported by**: Kavari (Level 12)

### Invocation Limit
- **Description**: Can select more invocations than allowed
- **Reported by**: Ogoun (Level 30)

### Temote Command
- **Description**: Command doesn't work even with correct syntax
- **Reported by**: ErrnieElvin (Level 1)

### Hideous Blow Toggle
- **Description**: Cannot turn off hideous blow or cast blast after activation
- **Reported by**: Almund (Level 3)

### Multiclass Gain
- **Description**: 'gain cleric' advanced warrior instead
- **Reported by**: Zylese (Level 3)

### Crafting XP Mismatch
- **Description**: Shows 12 XP gain but TNL drops by 30
- **Reported by**: Zylese (Level 6)

### Page Continue
- **Description**: Return key repeats message instead of continuing
- **Reported by**: Zylese (Level 6)

### Charm Monster Encounter
- **Description**: Charmed mob attacks itself, can't leave encounter
- **Reported by**: Bragollach (Level 10)

### Death Effects HP
- **Description**: Death effects deal 1499 damage instead of killing
- **Reported by**: Iliri (Level 19)

### Prismatic Spray Blind
- **Description**: Blindness persists after death and resurrection
- **Reported by**: Mallyrn (Level 18)

### Inventory Destruction
- **Description**: Rapid copper purchases with full inventory destroys items
- **Reported by**: Bragollach (Level 27)

### Finger of Death
- **Description**: Can be magic resisted despite description saying no
- **Reported by**: Mallyrn (Level 21)

### Bandit Castle Trap
- **Description**: Cannot leave or teleport from bandit castle area
- **Reported by**: Bragollach (Level 27)

### Hunt Bodyparts
- **Description**: Barghest hunt sometimes gives banshee parts
- **Reported by**: Iliri (Level 28)

### Litany of Righteousness
- **Description**: Causes dazzle to paladin user
- **Reported by**: Gwyndaryn (Level 11)

### Supply Order New
- **Description**: Always says wait until tomorrow for new characters
- **Reported by**: Galeron (Level 10)

### Eidolon Acid Message
- **Description**: Shows "acid from <NULL>" in combat
- **Reported by**: Galeron (Level 12)

### Carriage Crash
- **Description**: Using carriage causes game crash/disconnect
- **Reported by**: Taurus (Level 2), Fierend (Level 4)

### Newbie Quest 3-4
- **Description**: Killing skirker in quest 3 breaks quest 4
- **Reported by**: Asterix (Level 1)

### Harvesting Depleted
- **Description**: Can harvest from depleted nodes
- **Reported by**: Alost (Level 1)

### Character List Empty
- **Description**: Character list empty on login but gear persists
- **Reported by**: Alost (Level 1)

### Quest History Wrap
- **Description**: Line wrap issues with cut-off descriptions
- **Reported by**: Fernidand (Level 2)

### Greport Crash
- **Description**: Ordering followers to greport crashes client
- **Reported by**: Lynx (Level 10)

### Cabinet Access
- **Description**: Cabinet in alchemist room can't be unlocked/opened
- **Reported by**: Eaion (Level 3)

### Phantom Steed Conflict
- **Description**: Phantom steed cast fails if you already have animal companion
- **Reported by**: Krillin (Level 5)

### Player Shop Issues
- **Description**: Cannot id or buy items in player shops (list works)
- **Reported by**: Ellyanor (Level 30)

### Flight Movement Cost
- **Description**: Flying over terrain consumes same moves as walking
- **Reported by**: Salinavi (Level 6)

### Call Companion While Wildshaped
- **Description**: Unable to call animal companion while wildshaped
- **Reported by**: Salinavi (Level 6)

### Shadow Shield AC Stacking
- **Description**: Shadow Shield +5 AC stacks when recast before expiration
- **Reported by**: Thimblethorp (Level 29)

### Portal to Immortals
- **Description**: Mortals can portal/gate to immortals unless NoHassle flag set
- **Reported by**: Danavan (Level 32)

### Locate Object Crash
- **Description**: Locate Object spell crashes the mud
- **Reported by**: Ellyanor (Level 30)

### Enchant Weapon Overpowered
- **Description**: Adds +4 untyped hit and damroll
- **Reported by**: Ornir (Level 34)

### Monk Size and Damage
- **Description**: Medium monk unarmed damage doesn't increase when enlarged
- **Reported by**: Nirri (Level 19)

### Acid Fog No Aggro
- **Description**: Can cast acid fog and leave room without causing aggro
- **Reported by**: Thimblethorp (Level 30)

### Firebrand/Cone of Cold Bugs
- **Description**: Level 5 spells do level 3 damage (90-110) and have 9 second cast time
- **Reported by**: Thimblethorp (Level 30)

### Monk Blinding Speed Display
- **Description**: Blinding speed in effect but doesn't show in affects
- **Reported by**: Nirri (Level 30)

### Iron Skin Absorption
- **Description**: Only absorbs 36 damage per attack instead of full damage up to 450 total
- **Reported by**: Nirri (Level 30)

### Scorching Ray No Roll
- **Description**: No attack roll, automatically hits
- **Reported by**: Thimblethorp (Level 30)

### Waves of Exhaustion
- **Description**: Says it causes Fatigue instead of Exhausted effect
- **Reported by**: Thimblethorp (Level 30)

### AC Persistence Bug
- **Description**: AC bonuses from spells not dropping when spell expires
- **Reported by**: Zusuk (Level 34)

### Epic Stat Feats
- **Description**: Greater Intelligence/Constitution don't grant HP or skill points
- **Reported by**: DeepfriedPotato (Level 29)

### Two Weapon Fighting Prerequisites
- **Description**: Can select improved/greater without normal feat first
- **Reported by**: Katlian (Level 7)

### Monk DR Non-Stackable
- **Description**: Monk's inherent 3 DR doesn't stack with other sources
- **Reported by**: Nirri (Level 30)

### Acid Arrow Save Bug
- **Description**: Still takes effect and does damage after successful save
- **Reported by**: Warrel (Level 4)

### Rage Death XP
- **Description**: No experience gained when NPC dies from rage fading
- **Reported by**: Kogan (Level 31)

### Timestop Casting
- **Description**: Decreases casting time but doesn't remove action cost limits
- **Reported by**: Thimblethorp (Level 30)

### XP Scaling Reversed
- **Description**: XP increases instead of decreases as player levels up
- **Reported by**: Durakh (Level 28)

### Horse Random Attack
- **Description**: Paladin mount randomly attacks rider despite high ride skill
- **Reported by**: Durakh (Level 29)

### Blur Staff Target
- **Description**: Blur on staff won't affect other players/pets in area
- **Reported by**: Warrel (Level 7)

### Grant-Feat Items
- **Description**: Items granting feats (like Strong Against Poison) don't apply bonus
- **Reported by**: Thimblethorp (Level 30)

### Canny Defense Not Working
- **Description**: Duelist's Canny Defense doesn't add Int bonus to AC
- **Reported by**: Talithra (Level 20)

### Appraise Overflow
- **Description**: High appraise skill causes negative or zero prices
- **Reported by**: Thimblethorp (Level 30)

### Resize Message Bug
- **Description**: Shows same size for both "from" and "to" in resize message
- **Reported by**: Meach (Level 10)

### GMCP/Xterm Settings
- **Description**: GMCP and Xterm 256 settings don't persist after reconnect
- **Reported by**: Edau (Level 4)

### Cabinet Key Bug
- **Description**: Cabinet shows locked when key held, drops key to see "closed" message
- **Reported by**: Lywevia (Level 4)

### Inventory Loss Bug
- **Description**: Items lost on login due to crash-save loading issue
- **Reported by**: Zenon (Level 14)

### Wild Shape Cooldown
- **Description**: Forces cooldown even when no animal shape entered
- **Reported by**: Zolon (Level 6)

### Druid Commune Display
- **Description**: Shows nonsensical "Available: 5 0th" at level 21
- **Reported by**: Zolon (Level 21)

### Trelux Stat Points
- **Description**: Shows negative stat points (-6948323) when leveling
- **Reported by**: Faldorn (Level 1), Jhin (Level 1)

### Power Critical Display
- **Description**: Shows wrong weapon type in feat selection message
- **Reported by**: Ghaloaire (Level 6)

### Combat Initiation Delay
- **Description**: Don't enter combat until taking damage, parries don't trigger combat
- **Reported by**: Ghaloaire (Level 6)

### Barkskin Infusion Duration
- **Description**: Only lasts 1 round when cast via infusion
- **Reported by**: Artephius (Level 4)

### Bombs After Respec
- **Description**: Bombs remain prepared after respec to level 1
- **Reported by**: Artephius (Level 7)

### Held Items Conflict
- **Description**: Can equip two-handed weapon after holding item with no error
- **Reported by**: Artephius (Level 7)

### Wild Shape Syntax
- **Description**: Uses wild shape charge even with wrong syntax
- **Reported by**: Alerehin (Level 4)

### Handle Animal Companion
- **Description**: Using handle animal on companion turns it hostile
- **Reported by**: Alerehin (Level 4)

### Dwarf Weapon Proficiency
- **Description**: Dwarf doesn't gain dwarven weapon proficiency feat
- **Reported by**: Daghan (Level 1)

### True Mutagen Targeting
- **Description**: Always gives STR bonus regardless of argument (dex/con)
- **Reported by**: Bersain (Level 30)

### Ultravision Range
- **Description**: Can't see one room away despite having ultravision
- **Reported by**: Krok (Level 8)

### Rapid Fire Range
- **Description**: No speed increase at far shot range
- **Reported by**: Iliothro (Level 4)

### Autocollect Familiar
- **Description**: Doesn't collect when familiar gets kill
- **Reported by**: Iliothro (Level 9)

### Sacred Fist Levels
- **Description**: Sacred Fist levels don't count toward cleric for Battlerage
- **Reported by**: Iliothro (Level 10)

### Autocollect vs Autosac
- **Description**: Sacrifice attempts before collect causing error
- **Reported by**: Iliothro (Level 6)

### Restring Size Change
- **Description**: Restringing weapon changes size and damage dice
- **Reported by**: Draca (Level 30)

### Posthumous Grapple
- **Description**: Mobs can grapple after death
- **Reported by**: Draca (Level 30)

### Kill Attack Loss
- **Description**: Lose remaining attacks when killing mob but enemies continue attacking
- **Reported by**: Draca (Level 30)

### Protected Mob Memory
- **Description**: Protected mob that remembers you attacks then disengages
- **Reported by**: Khell (Level 30)

### Displacement vs Blur
- **Description**: Displacement doesn't override blur when cast after
- **Reported by**: Khell (Level 30)

### Mob Springleap
- **Description**: Mobs can springleap from standing position
- **Reported by**: Draca (Level 30)

### Spellbattle Duration
- **Description**: Lasts 4 minutes instead of 6 as per help file
- **Reported by**: Jukaw (Level 8)

### Initiative Asymmetry
- **Description**: Mobs always attack first when pursuing fleeing player
- **Reported by**: Draca (Level 30)

### Spellbattle HP Bug
- **Description**: HP bonus incorrect, can reduce max HP to 5
- **Reported by**: Jukaw (Level 15)

### Imbuearrow Restrictions
- **Description**: Cannot imbue arrow with greater dispelling
- **Reported by**: Draca (Level 30)

### XP Cap Message
- **Description**: Says must gain level but ExpTNL is positive
- **Reported by**: Aiden (Level 6)

### Mission Cooldown
- **Description**: Shows 9.5 minute cooldown instead of 5 as per help
- **Reported by**: Kordon (Level 22)

### Impromptu Sneak Attack
- **Description**: Not showing on cooldowns
- **Reported by**: Ombra (Level 8)

## Quest Bugs

### Orc Blacksmith Weapon
- **Notes**: Tested and working properly - NOT A BUG
- **Reported by**: Maloc (Level 8)

### Flower Tiara Quest
- **Description**: Killing faeries doesn't update quest counter
- **Reported by**: Badase (Level 30)

### Tutorial Confusion
- **Description**: Many rooms/NPCs do nothing
- **Reported by**: Whil (Level 1)

### Tutorial Missing Items
- **Description**: Quests mention backpack, crafting kit, rations not given
- **Reported by**: Whil (Level 2)

### Tower Door Keyword
- **Description**: Door has keyword but doesn't work
- **Reported by**: Badase (Level 30)

### Rum Runners Quest
- **Description**: No quest information displayed
- **Reported by**: Talendor (Level 25), Neurrone (Level 23), Galeron (Level 20)

### First Step is the Hilt
- **Description**: Loops lumberjack portion, doesn't create hilt
- **Reported by**: Duirren (Level 7)

### Stohana Quest Trigger
- **Description**: Quest "Built On The Doorstep of the Darklings" triggers on wrong NPC (Old Man instead of Stohana)
- **Reported by**: Ylar (Level 7), Lueldora (Level 8)

### Dollhouse Entrance
- **Description**: Charmees not brought along when entering dollhouse
- **Reported by**: Salinavi (Level 11)

### Dollhouse Cups
- **Description**: Unclear which cup is "left" or "right" (should use numbers)
- **Reported by**: Coll (Level 14)

### Napalm Syntax
- **Description**: Napalm from Camille quest shows "nap <target>" but doesn't work
- **Reported by**: Dakota (Level 30)

### Memlin Stops Self
- **Description**: Memlin explaining "Navi" room says "stop" and stops following
- **Reported by**: Durakh (Level 30), Jukaw (Level 14)

### Dollhouse Journal Lost
- **Description**: Journal can be lost/eaten by Ridley before ready
- **Reported by**: Tenegre (Level 14)

### Ruth Journal Bug
- **Description**: Ruth in dollhouse doesn't write in journal but thinks she did
- **Reported by**: Draca (Level 15)

## Visual/Display Bugs

### Axe of Calamity Proc
- **Description**: Only 11 damage on failed save
- **Reported by**: Badase (Level 30)

### Hood of Swirling Clouds
- **Description**: Identifies as full plate helm instead of hood
- **Reported by**: Ertai (Level 28)

### EXP Display Bug
- **Description**: Shows 2500 exp then 0 exp in same kill
- **Reported by**: Gor (Level 12)

### Fiery Bracelet Stats
- **Description**: Level 1 item from HUGE Fire Elemental has "-5 to !Unused!" affection
- **Reported by**: Thimblethorp (Level 29)

### Gray Cloth Keywords
- **Description**: Gray piece of cloth from Grand Knight has no keywords
- **Reported by**: Ellyanor (Level 30)

### Wartorn Gold Drops
- **Description**: NPCs killing each other drop gold piles everywhere
- **Reported by**: Thimblethorp (Level 30)

### Comet's Tail Target
- **Description**: Shows as whip on floor but doesn't target as whip
- **Reported by**: Tiu (Level 9)

### Enemy Condition Typo
- **Description**: Status drops from Perfect to "Yexcellent" (extra Y)
- **Reported by**: Hadlar (Level 7)

### Crystal Dwarf Stats
- **Description**: Help file shows +8 con, race info shows +4 con
- **Reported by**: Shadorn (Level 1)

### Monk Feat Display
- **Description**: Power critical and improved critical for monk weapons show as blank lines
- **Reported by**: Fei (Level 15)

### Eagle Familiar Attack
- **Description**: Enemy "ducks under its fist" message for eagle
- **Reported by**: Iliothro (Level 10)

### Mob Keywords
- **Description**: "A lizard man" can't target with "lizard" or "man"
- **Reported by**: Iliothro (Level 6)

### Weapon Focus Monk
- **Description**: Not showing in feats list but giving bonus
- **Reported by**: Iliothro (Level 3)

### Memlin Sharp Rocks
- **Description**: Slip damage in sharp rock room even with levitate
- **Reported by**: Draca (Level 30)

### Quest Point Items Cursed
- **Description**: Can't drop/remove Shadow Stealer, War's Blood
- **Reported by**: Draca (Level 30)

### Raven Feather Eyepatch
- **Description**: Says can be worn on eyes but can't wear it
- **Reported by**: Khell (Level 30)

### Gauntlets of Magma
- **Description**: Doesn't proc and protect-elements doesn't work
- **Reported by**: Khell (Level 30)

### Yagh vs Yahg
- **Description**: Quest wants "Yagh" but mob is "Yahg"
- **Reported by**: Snips (Level 2), Pathos (Level 31)

### Missing Wear-off Messages
- **Description**: Multiple missing wear-off messages (174, 198, 206, 470, 548, 592, 81)
- **Reported by**: Various players

### Spell Circles Available Display
- **Description**: Shows "Available: 1 0th" at level 25-26 with nothing to apply
- **Reported by**: Thimblethorp (Level 29)

### Old Farmhand Race
- **Description**: Shows as farmhand but is actually dragon with dragon attacks
- **Reported by**: Dakota (Level 27)

### Dark Fiery Orb
- **Description**: Shows "Report this item to coder to add ITEM_type"
- **Reported by**: Salinavi (Level 11)

### Tomb Room Display
- **Description**: Has '&n' in room display
- **Reported by**: Ellyanor (Level 30)

### Prospector's Eyepiece
- **Description**: Wrong equip location (eye slot)
- **Reported by**: Ellyanor (Level 30)

### Elite Temple Guard
- **Description**: Swords are actually heavy maces with 2d12 damage
- **Reported by**: Meach (Level 7)

### Room 3074 Description
- **Description**: Room has no description
- **Reported by**: Drayse (Level 5)

### XP From Spider vs Mist
- **Description**: Level 16 spider gives more XP than level 17 mist
- **Reported by**: Jukaw (Level 17)