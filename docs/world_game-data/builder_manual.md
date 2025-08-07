# Luminari MUD Builder's Manual

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started with Building](#getting-started-with-building)
3. [Online Creation (OLC) System](#online-creation-olc-system)
4. [World Building Fundamentals](#world-building-fundamentals)
5. [Room Creation and Design](#room-creation-and-design)
6. [Mobile (NPC) Creation](#mobile-npc-creation)
7. [Object Creation](#object-creation)
8. [Zone Management](#zone-management)
9. [Advanced Building Techniques](#advanced-building-techniques)
10. [Scripting and Triggers](#scripting-and-triggers)
11. [Testing and Debugging](#testing-and-debugging)
12. [Best Practices](#best-practices)

---

## Introduction

### Your Role as a World Architect

As a builder in Luminari MUD, you are a world architect responsible for creating immersive, balanced, and engaging areas that players will explore and enjoy. Building is both an art and a craft that requires creativity, attention to detail, and understanding of game balance.

**Key Responsibilities:**
- Creating compelling areas with coherent themes
- Designing balanced encounters and rewards
- Writing engaging descriptions and dialogue
- Implementing interactive elements and puzzles
- Testing and refining your creations

### Philosophy of World Building

Building an area is like writing a book - it needs plot, descriptive detail, and memorable characters and places. Your areas should contribute to the overall theme and atmosphere of the MUD while providing appropriate challenges and rewards for the intended player level.

**Core Principles:**
- **Immersion:** Create believable, detailed environments
- **Balance:** Ensure appropriate difficulty and rewards
- **Originality:** Develop unique themes and concepts
- **Quality:** Polish every detail before release
- **Player Experience:** Always consider the player's perspective

### Game Balance Considerations

Game balance is crucial for maintaining a fair and enjoyable experience. Each area should be designed with specific player levels and group sizes in mind. The monsters, objects, and rewards should match the intended difficulty level.

**Balance Guidelines:**
- Match monster difficulty to intended player level
- Provide appropriate rewards for effort required
- Avoid creating "must-have" items that unbalance the game
- Consider the area's impact on the overall game economy
- Test thoroughly with players of the target level

---

## Getting Started with Building

### Prerequisites

**Required Knowledge:**
- Basic understanding of MUD gameplay
- Familiarity with the game world and theme
- Understanding of player levels and progression
- Basic knowledge of file formats (if editing manually)

**Recommended Skills:**
- Creative writing ability
- Attention to detail
- Basic understanding of game design
- Patience for testing and iteration

### Building Permissions

**OLC Access Levels:**
- **Builder (Level 31+):** Basic building commands
- **Zone Editor:** Can modify assigned zones
- **Senior Builder:** Advanced building features
- **Implementor:** Full building access

**Getting Building Access:**
1. Demonstrate building interest and ability
2. Submit area proposals or sample work
3. Receive zone assignment from administrators
4. Learn OLC commands and procedures

### Understanding the World Structure

**File Organization:**
```
lib/world/
├── wld/    # Room files (.wld)
├── mob/    # Mobile files (.mob)
├── obj/    # Object files (.obj)
├── shp/    # Shop files (.shp)
├── trg/    # Trigger files (.trg)
└── zon/    # Zone files (.zon)
```

**Zone Numbering:**
- Each zone has a unique number range
- Rooms, mobs, and objects use zone-based numbering
- Standard zones use 100-number ranges (e.g., 3000-3099)
- Special zones may have different ranges

### Area Planning Process

**1. Concept Development:**
- Define the area's theme and purpose
- Determine target player level and group size
- Create a basic storyline or concept
- Research inspiration sources (literature, mythology, etc.)

**2. Design Phase:**
- Sketch area maps on paper first
- Plan room connections and layout
- Design key NPCs and their roles
- Plan special objects and rewards
- Consider quest opportunities

**3. Implementation:**
- Use OLC to create rooms, mobs, and objects
- Write detailed descriptions
- Implement special features and triggers
- Test basic functionality

**4. Testing and Refinement:**
- Test with players of appropriate level
- Adjust balance based on feedback
- Fix bugs and improve descriptions
- Polish details and add finishing touches

---

## Online Creation (OLC) System

### OLC Overview

The Online Creation (OLC) system allows builders to create and modify world content while the MUD is running. This powerful system provides menu-driven interfaces for creating rooms, mobiles, objects, shops, and zones.

**OLC Advantages:**
- Real-time creation and testing
- Menu-driven interface reduces errors
- Immediate feedback and validation
- No need to restart the server
- Built-in help and guidance

### Basic OLC Commands

**Main OLC Commands:**
- `redit <room_vnum>` - Edit rooms
- `medit <mob_vnum>` - Edit mobiles (NPCs)
- `oedit <obj_vnum>` - Edit objects
- `zedit <zone_num>` - Edit zones
- `sedit <shop_vnum>` - Edit shops
- `tedit <trigger_vnum>` - Edit triggers

**Navigation Commands:**
- `show` - Display current item being edited
- `save` - Save changes to disk
- `abort` - Cancel changes and exit
- `?` or `help` - Show available commands

### Room Editor (REDIT)

**Starting Room Editing:**
```
redit <room_vnum>    # Edit existing room
redit new            # Create new room in current zone
```

**Room Properties:**
- **Name:** Short title displayed in room
- **Description:** Detailed room description
- **Room Flags:** Special room properties
- **Sector Type:** Terrain type (affects movement)
- **Exits:** Connections to other rooms
- **Extra Descriptions:** Additional detail descriptions

**Common REDIT Commands:**
```
name <room_name>           # Set room name
desc                       # Edit room description
flags                      # Set room flags
sector <sector_type>       # Set terrain type
dig <direction> <vnum>     # Create exit to room
rdelete <direction>        # Remove exit
extra <keyword>            # Add extra description
```

**Room Flags:**
- `DARK` - Room is always dark
- `NO_MOB` - Mobiles cannot enter
- `INDOORS` - Room is inside
- `PEACEFUL` - No fighting allowed
- `SOUNDPROOF` - Sounds don't carry
- `NO_TRACK` - Cannot be tracked to
- `NO_MAGIC` - Magic doesn't work
- `TUNNEL` - Only one person at a time

**Sector Types:**
- `INSIDE` - Indoor rooms
- `CITY` - Urban areas
- `FIELD` - Open fields
- `FOREST` - Wooded areas
- `HILLS` - Hilly terrain
- `MOUNTAIN` - Mountainous areas
- `WATER_SWIM` - Swimmable water
- `WATER_NOSWIM` - Deep water
- `UNDERWATER` - Underwater areas
- `FLYING` - Air/flying areas

### Mobile Editor (MEDIT)

**Starting Mobile Editing:**
```
medit <mob_vnum>     # Edit existing mobile
medit new            # Create new mobile in current zone
```

**Mobile Properties:**
- **Keywords:** Words used to target the mobile
- **Short Description:** Name shown in room
- **Long Description:** Description when mobile is in room
- **Detailed Description:** Description when examined
- **Level:** Mobile's level and difficulty
- **Stats:** Hitpoints, armor class, damage, etc.
- **Flags:** Special mobile properties
- **Affects:** Permanent spell effects

**Common MEDIT Commands:**
```
keywords <keyword_list>    # Set targeting keywords
shortdesc <description>    # Set short description
longdesc                   # Edit long description
detaileddesc              # Edit detailed description
level <level>             # Set mobile level
hitpoints <hp>            # Set hit points
ac <armor_class>          # Set armor class
damage <num>d<size>+<add> # Set damage dice
flags                     # Set mobile flags
affects                   # Set spell affects
```

**Mobile Flags:**
- `SPEC` - Has special procedure
- `SENTINEL` - Doesn't move from room
- `SCAVENGER` - Picks up objects
- `ISNPC` - Is an NPC (automatically set)
- `AWARE` - Cannot be backstabbed
- `AGGRESSIVE` - Attacks players on sight
- `STAY_ZONE` - Won't leave its zone
- `WIMPY` - Flees when injured
- `MEMORY` - Remembers attackers

### Object Editor (OEDIT)

**Starting Object Editing:**
```
oedit <obj_vnum>     # Edit existing object
oedit new            # Create new object in current zone
```

**Object Properties:**
- **Keywords:** Words used to target the object
- **Short Description:** Name shown in inventory
- **Long Description:** Description when on ground
- **Action Description:** Message when object is used
- **Type:** Object type (weapon, armor, container, etc.)
- **Values:** Type-specific properties
- **Weight:** Object weight
- **Cost:** Object value
- **Rent:** Daily rent cost

**Common OEDIT Commands:**
```
keywords <keyword_list>    # Set targeting keywords
shortdesc <description>    # Set short description
longdesc                   # Edit long description
actiondesc                 # Edit action description
type <object_type>         # Set object type
values                     # Edit type-specific values
weight <weight>            # Set object weight
cost <cost>                # Set object value
rent <rent>                # Set rent cost
affects                    # Set stat affects
extra <keyword>            # Add extra description
```

**Object Types:**
- `LIGHT` - Light sources
- `SCROLL` - Spell scrolls
- `WAND` - Magic wands
- `STAFF` - Magic staves
- `WEAPON` - Weapons
- `ARMOR` - Armor and clothing
- `POTION` - Potions
- `CONTAINER` - Containers
- `FOOD` - Food items
- `DRINK` - Drink containers
- `KEY` - Keys
- `BOAT` - Boats for water travel

### Zone Editor (ZEDIT)

**Starting Zone Editing:**
```
zedit <zone_num>     # Edit existing zone
zedit new            # Create new zone
```

**Zone Properties:**
- **Name:** Zone name and description
- **Lifespan:** How long zone resets last
- **Reset Mode:** When zone resets occur
- **Commands:** Zone reset commands

**Zone Reset Commands:**
- `M` - Load mobile into room
- `O` - Load object into room
- `G` - Give object to mobile
- `E` - Equip object on mobile
- `P` - Put object in container
- `D` - Set door state
- `R` - Remove object from room

**Reset Modes:**
- `0` - Never reset
- `1` - Reset when no players in zone
- `2` - Always reset

---

## World Building Fundamentals

### Writing Effective Descriptions

**Room Descriptions:**
Room descriptions are the foundation of player immersion. They should paint a vivid picture while being concise enough to read quickly.

**Good Description Principles:**
- Use sensory details (sight, sound, smell, touch)
- Establish mood and atmosphere
- Provide navigation hints
- Include interactive elements
- Maintain consistent tone

**Example - Poor Description:**
```
You are in a room. It is dark. There are exits north and south.
```

**Example - Good Description:**
```
You stand in a dimly lit chamber carved from rough stone. The air is thick
with the scent of damp earth and something else - something metallic that
makes your skin crawl. Shadows dance along the walls in the flickering light
of a single torch mounted near the northern archway. To the south, a narrow
passage disappears into complete darkness.
```

**Mobile Descriptions:**
Mobile descriptions should convey personality, threat level, and role in the area.

**Description Components:**
- **Keywords:** How players target the mobile
- **Short Description:** Name in room and combat
- **Long Description:** What players see when mobile is present
- **Detailed Description:** What players see when examining

**Example Mobile Descriptions:**
```
Keywords: guard captain warrior
Short: a stern guard captain
Long: A stern guard captain stands here, watching for trouble.
Detailed: This weathered veteran bears the scars of countless battles.
His steel armor is well-maintained but shows signs of heavy use. Cold
blue eyes survey the area with professional alertness, and his hand
rests casually on the pommel of his sword.
```

### Creating Atmosphere and Theme

**Establishing Theme:**
Every area should have a clear, consistent theme that guides all design decisions.

**Common Themes:**
- **Dungeon Crawl:** Classic underground adventure
- **Urban Adventure:** City-based intrigue and exploration
- **Wilderness:** Natural environments and survival
- **Horror:** Dark, frightening atmospheres
- **Mystery:** Puzzles and investigation
- **Political:** Court intrigue and diplomacy

**Atmospheric Elements:**
- **Lighting:** Bright, dim, dark, or magical illumination
- **Weather:** Rain, snow, fog, or clear skies
- **Sounds:** Background noises and ambient effects
- **Smells:** Distinctive scents that enhance immersion
- **Temperature:** Hot, cold, or comfortable environments

### Planning Area Layout

**Map Design Principles:**
- **Logical Geography:** Areas should make geographical sense
- **Clear Navigation:** Players shouldn't get hopelessly lost
- **Interesting Paths:** Multiple routes and hidden areas
- **Appropriate Size:** Match area size to content and purpose
- **Strategic Placement:** Consider connections to other areas

**Common Layout Patterns:**
- **Linear:** Straight path with optional side areas
- **Hub and Spoke:** Central area with branches
- **Maze:** Complex interconnected passages
- **Layered:** Multiple levels (underground, ground, upper)
- **Open World:** Large area with multiple entry/exit points

### Balancing Difficulty and Rewards

**Level-Appropriate Content:**
- **Newbie Areas (1-10):** Simple combat, basic equipment
- **Low Level (11-20):** Moderate challenges, useful gear
- **Mid Level (21-30):** Complex encounters, good rewards
- **High Level (31+):** Difficult content, rare items

**Reward Guidelines:**
- **Experience Points:** Match effort required
- **Equipment:** Appropriate for level and slot
- **Gold:** Reasonable amounts for economy
- **Special Items:** Unique but balanced rewards

**Challenge Scaling:**
- **Solo Content:** Designed for single players
- **Small Group:** 2-3 players working together
- **Full Group:** 4-6 players with diverse roles
- **Raid Content:** Large groups with coordination

---

## Room Creation and Design

### Room Naming Conventions

**Effective Room Names:**
- Keep names concise (under 60 characters)
- Use descriptive but not overly detailed names
- Maintain consistent style within the area
- Avoid generic names like "A Room" or "Corridor"

**Examples:**
- "The Throne Room of King Aldric"
- "A Winding Forest Path"
- "The Alchemist's Laboratory"
- "Atop the Ancient Watchtower"

### Exit Design and Door Descriptions

**Exit Types:**
- **Standard Exits:** North, south, east, west, up, down
- **Special Exits:** Northeast, northwest, southeast, southwest
- **Custom Exits:** Enter, climb, swim, etc.

**Door Properties:**
- **Closeable:** Can be opened and closed
- **Locked:** Requires key or picking
- **Pickproof:** Cannot be picked
- **Hidden:** Not visible in room description

**Door Descriptions:**
Describe exits to enhance immersion and provide navigation hints.

**Example Exit Descriptions:**
```
North: A grand archway leads into the castle's main hall.
South: A narrow staircase spirals downward into darkness.
East: Heavy oak doors stand slightly ajar, revealing a library beyond.
```

### Extra Descriptions

Extra descriptions allow players to examine specific elements mentioned in room descriptions, adding depth and interactivity.

**Creating Extra Descriptions:**
```
extra <keywords>
```

**Example Extra Descriptions:**
- `extra torch flame fire` - Describe the torch in detail
- `extra shadows darkness` - Describe mysterious shadows
- `extra carving inscription` - Describe wall carvings
- `extra altar stone` - Describe a stone altar

### Special Room Features

**Room Flags for Special Effects:**
- **DARK:** Room requires light source
- **NO_MOB:** Prevents mobile entry
- **PEACEFUL:** Prevents combat
- **SOUNDPROOF:** Blocks sound transmission
- **NO_MAGIC:** Disables magic use
- **GODROOM:** Immortal-only access

**Environmental Hazards:**
- **Damage Rooms:** Cause periodic damage
- **Movement Restrictions:** Require special items or abilities
- **Teleport Rooms:** Transport players elsewhere
- **Death Traps:** Instant death for unwary players

### Interactive Elements

**Searchable Objects:**
Create hidden items or passages that players can discover through examination or searching.

**Puzzle Elements:**
- **Riddles:** Text-based puzzles with specific answers
- **Mechanical Puzzles:** Require manipulation of objects
- **Sequence Puzzles:** Require actions in specific order
- **Key Puzzles:** Require finding and using specific items

**Quest Integration:**
- **Quest Givers:** NPCs who provide missions
- **Quest Objects:** Items needed for quests
- **Quest Locations:** Specific rooms for quest events
- **Quest Rewards:** Special items or access granted

---

## Mobile (NPC) Creation

### Mobile Types and Roles

**Functional Roles:**
- **Shopkeepers:** Buy and sell items
- **Guards:** Provide security and law enforcement
- **Quest Givers:** Provide missions and information
- **Trainers:** Teach skills or spells
- **Informants:** Provide hints and lore

**Combat Roles:**
- **Tanks:** High hit points, low damage
- **Damage Dealers:** High damage, moderate hit points
- **Support:** Healing or buffing abilities
- **Specialists:** Unique abilities or resistances

### Mobile Statistics

**Core Statistics:**
- **Level:** Determines overall power level
- **Hit Points:** Amount of damage mobile can take
- **Armor Class:** Difficulty to hit in combat
- **Damage:** Amount of damage dealt in combat
- **Experience:** Points awarded when defeated

**Calculation Guidelines:**
```
Hit Points = Level * 8 + 2d(Level * 2)
Armor Class = 10 - (Level / 4)
Damage = Level/4 d6 + Level/8
Experience = Level * Level * 75
```

### Mobile Behavior and AI

**Movement Patterns:**
- **SENTINEL:** Stays in assigned room
- **STAY_ZONE:** Won't leave zone boundaries
- **TRACK:** Follows and hunts players
- **WANDER:** Moves randomly through area

**Combat Behavior:**
- **AGGRESSIVE:** Attacks players on sight
- **WIMPY:** Flees when badly injured
- **MEMORY:** Remembers and hunts attackers
- **HELPER:** Assists other mobiles in combat

**Special Abilities:**
- **SPEC:** Has special procedure (custom code)
- **SPELLCASTER:** Can cast spells
- **BREATH:** Has breath weapon attacks
- **REGENERATE:** Heals over time

### Mobile Equipment and Inventory

**Equipment Slots:**
- **Weapons:** Primary and secondary weapons
- **Armor:** Various armor pieces
- **Accessories:** Rings, amulets, etc.
- **Held Items:** Shields, lights, etc.

**Inventory Management:**
- **Starting Equipment:** Items mobile begins with
- **Carried Items:** Objects in mobile's inventory
- **Money:** Gold pieces carried
- **Keys:** Special access items

### Creating Memorable NPCs

**Personality Development:**
- **Background:** History and motivations
- **Speech Patterns:** Unique dialogue style
- **Quirks:** Memorable characteristics
- **Relationships:** Connections to other NPCs

**Dialogue Writing:**
- Keep responses concise but flavorful
- Use consistent voice and vocabulary
- Provide useful information or atmosphere
- Include personality in every interaction

**Example NPC Dialogue:**
```
A grizzled old miner says, "Aye, I've been diggin' these tunnels for nigh
on forty years. Seen things down here that'd make yer hair turn white, I
have. But the gold... the gold keeps callin' me back."
```
---

## Object Creation

### Object Categories and Types

**Equipment Objects:**
- **Weapons:** Swords, axes, bows, staves, etc.
- **Armor:** Helmets, body armor, shields, boots, etc.
- **Accessories:** Rings, amulets, belts, cloaks, etc.
- **Tools:** Lockpicks, torches, rope, etc.

**Consumable Objects:**
- **Potions:** Healing, stat enhancement, special effects
- **Food:** Hunger satisfaction, stat bonuses
- **Scrolls:** Single-use spells
- **Components:** Spell components, crafting materials

**Utility Objects:**
- **Containers:** Bags, chests, boxes
- **Keys:** Access to locked areas
- **Boats:** Water transportation
- **Lights:** Torches, lanterns, magical lights

### Object Statistics and Properties

**Basic Properties:**
- **Weight:** Affects carrying capacity
- **Value:** Base cost and rent price
- **Durability:** How long object lasts
- **Level Restrictions:** Minimum level to use

**Weapon Properties:**
- **Damage Dice:** Amount of damage dealt
- **Weapon Type:** Slash, pierce, blunt
- **Hit Bonus:** Accuracy modifier
- **Damage Bonus:** Extra damage modifier

**Armor Properties:**
- **Armor Class:** Protection provided
- **Armor Type:** Light, medium, heavy
- **Coverage:** Body parts protected
- **Special Resistances:** Magic, fire, etc.

### Creating Balanced Equipment

**Level-Appropriate Stats:**
```
Level 1-10:   AC 8-6,  +0 to +2 bonuses
Level 11-20:  AC 5-3,  +1 to +3 bonuses
Level 21-30:  AC 2-0,  +2 to +4 bonuses
Level 31+:    AC -1+,  +3 to +5 bonuses
```

**Weapon Damage Guidelines:**
```
Level 1-10:   1d6 to 2d4 damage
Level 11-20:  2d4 to 2d6 damage
Level 21-30:  2d6 to 3d6 damage
Level 31+:    3d6+ damage
```

### Special Object Features

**Magical Properties:**
- **Stat Bonuses:** Strength, dexterity, constitution, etc.
- **Skill Bonuses:** Improve specific abilities
- **Resistances:** Protection from damage types
- **Special Abilities:** Unique powers or effects

**Cursed Objects:**
- **Negative Effects:** Stat penalties or harmful effects
- **Removal Difficulty:** Hard to remove once equipped
- **Hidden Curses:** Effects not immediately apparent
- **Curse Removal:** Require special methods to remove

**Container Objects:**
- **Capacity:** How much can be stored
- **Weight Reduction:** Magical weight reduction
- **Access Restrictions:** Who can open/use
- **Special Properties:** Preservation, sorting, etc.

---

## Zone Management

### Zone Reset System

**Understanding Resets:**
Zone resets repopulate areas with mobiles and objects, ensuring consistent gameplay experience.

**Reset Timing:**
- **Lifespan:** Minutes between reset attempts
- **Reset Mode:** Conditions that trigger resets
- **Player Presence:** How players affect resets

**Reset Commands:**
```
M <mobile_vnum> <max_exist> <room_vnum>     # Load mobile
O <object_vnum> <max_exist> <room_vnum>     # Load object
G <object_vnum> <max_exist>                 # Give to mobile
E <object_vnum> <max_exist> <wear_position> # Equip on mobile
P <object_vnum> <max_exist> <container>     # Put in container
D <room_vnum> <exit> <state>                # Set door state
R <room_vnum> <object_vnum>                 # Remove object
```

### Zone Connectivity

**Connecting to Other Zones:**
- Plan logical connections to existing areas
- Consider travel time and difficulty
- Maintain world geography consistency
- Provide multiple access routes when appropriate

**Transportation Methods:**
- **Walking:** Standard movement between rooms
- **Boats:** Water travel requirements
- **Teleportation:** Magical transportation
- **Special Movement:** Climbing, swimming, flying

### Zone Documentation

**Required Documentation:**
- **Zone Description:** Theme, level range, purpose
- **Map:** Layout and room connections
- **NPC List:** All mobiles with descriptions and stats
- **Object List:** All items with properties
- **Quest Information:** Any quests or special features

---

## Advanced Building Techniques

### Creating Immersive Environments

**Environmental Storytelling:**
Use room descriptions, object placement, and NPC behavior to tell stories without explicit exposition.

**Techniques:**
- **Visual Clues:** Describe evidence of past events
- **Atmospheric Details:** Use weather, lighting, sounds
- **Interactive Elements:** Let players discover story through exploration
- **Consistent Details:** Maintain story coherence throughout area

### Dynamic Content

**Variable Descriptions:**
Create descriptions that change based on conditions:
- Time of day variations
- Weather-dependent descriptions
- Player action consequences
- Seasonal changes

**Conditional Elements:**
- **Hidden Areas:** Revealed by specific actions
- **Changing Layouts:** Rooms that transform
- **Progressive Difficulty:** Areas that become harder over time
- **Player Impact:** Areas affected by player actions

### Multi-Level Design

**Vertical Spaces:**
- **Underground Levels:** Caves, dungeons, sewers
- **Ground Level:** Main area activities
- **Upper Levels:** Towers, treetops, flying areas
- **Connections:** Stairs, elevators, magical transport

**Layered Complexity:**
- **Surface Story:** Obvious plot and activities
- **Hidden Depths:** Secrets requiring investigation
- **Multiple Perspectives:** Different viewpoints on events
- **Interconnected Elements:** Connections between different areas

---

## Scripting and Triggers

The DG (DikuMUD Scripting) system allows builders to create interactive, dynamic content through event-driven scripts. Scripts can be attached to mobiles (NPCs), objects, and rooms to respond to player actions and create complex behaviors.

**For complete DG scripting documentation, including:**
- Detailed trigger types and examples
- Script command reference
- Advanced scripting techniques
- Performance optimization
- Debugging and testing procedures

**See:** [DG Scripting System Documentation](../systems/SCRIPTING_SYSTEM_DG.md)

### Quick Reference

**Basic Script Types:**
- **Mobile Scripts:** NPC behaviors and interactions
- **Object Scripts:** Item-specific functionality
- **Room Scripts:** Environmental effects and puzzles

**Common Triggers:**
- **Command:** Respond to specific player commands
- **Speech:** React to spoken words
- **Enter/Leave:** Activate when players move
- **Get/Drop/Wear:** Object interaction events
- **Fight:** Combat-related triggers

---

## Testing and Debugging

### Testing Methodology

**Systematic Testing:**
1. **Basic Functionality:** Ensure all rooms, exits, and objects work
2. **Balance Testing:** Verify appropriate difficulty and rewards
3. **Player Experience:** Test from player perspective
4. **Edge Cases:** Test unusual situations and combinations

**Testing Tools:**
- **Goto Command:** Quick navigation for testing
- **Stat Command:** Check mobile and object properties
- **Set Command:** Modify values for testing
- **Log Analysis:** Review system logs for errors

### Common Issues and Solutions

**Navigation Problems:**
- **Missing Exits:** Rooms with no way out
- **Broken Connections:** Exits leading to wrong rooms
- **Confusing Layout:** Players getting lost easily
- **Inconsistent Directions:** North/south mismatches

**Balance Issues:**
- **Overpowered Rewards:** Items too good for level
- **Underpowered Challenges:** Too easy for intended level
- **Economic Impact:** Items affecting game economy
- **Progression Problems:** Difficulty spikes or valleys

**Technical Problems:**
- **Script Errors:** Triggers not working properly
- **Reset Issues:** Zone not resetting correctly
- **Performance Problems:** Area causing lag
- **Compatibility Issues:** Conflicts with other areas

### Quality Assurance

**Pre-Release Checklist:**
- [ ] All rooms have proper descriptions
- [ ] All exits work correctly
- [ ] All mobiles have appropriate stats
- [ ] All objects are properly balanced
- [ ] Zone resets work correctly
- [ ] Scripts function as intended
- [ ] Area fits world theme and balance
- [ ] Documentation is complete

---

## Best Practices

### Writing Guidelines

**Description Writing:**
- Use active voice when possible
- Vary sentence structure and length
- Include sensory details beyond sight
- Maintain consistent tone and style
- Proofread for grammar and spelling

**Dialogue Writing:**
- Give each NPC a distinct voice
- Use appropriate vocabulary for character
- Keep responses concise but meaningful
- Include personality in every interaction
- Avoid modern slang unless appropriate

### Design Principles

**Player-Centered Design:**
- Always consider the player experience
- Provide clear navigation and objectives
- Balance challenge with reward
- Include variety in encounters and puzzles
- Test with actual players regularly

**Consistency Standards:**
- Maintain theme throughout area
- Use consistent naming conventions
- Follow established world lore
- Match difficulty to intended level
- Coordinate with other builders

### Collaboration Guidelines

**Working with Other Builders:**
- Communicate about connected areas
- Share resources and ideas
- Maintain consistent world standards
- Respect others' creative vision
- Provide constructive feedback

**Working with Administrators:**
- Follow established guidelines and policies
- Submit work for review before release
- Accept feedback gracefully
- Document your work thoroughly
- Report bugs and issues promptly

### Maintenance and Updates

**Ongoing Responsibilities:**
- Monitor player feedback
- Fix reported bugs promptly
- Update content as needed
- Maintain balance with game changes
- Keep documentation current

**Version Control:**
- Keep backups of your work
- Document changes and updates
- Test thoroughly after modifications
- Coordinate updates with administrators
- Maintain change logs

---

## Conclusion

Building for Luminari MUD is both an art and a craft that requires creativity, technical skill, and attention to detail. The tools and techniques described in this manual provide the foundation for creating engaging, balanced, and memorable areas that will enhance the game experience for all players.

Remember that building is an iterative process - your first areas may not be perfect, but with practice, feedback, and dedication, you'll develop the skills to create truly exceptional content. The MUD community benefits from builders who are passionate about creating immersive worlds and committed to maintaining high standards of quality.

**Key Takeaways:**
- Plan thoroughly before implementing
- Focus on player experience and immersion
- Maintain balance and consistency
- Test extensively and gather feedback
- Collaborate effectively with others
- Continue learning and improving

**Resources for Continued Learning:**
- Study existing areas for inspiration and techniques
- Participate in builder discussions and forums
- Experiment with advanced scripting features
- Seek feedback from players and other builders
- Stay updated on new tools and features

---

## Additional Resources

### Documentation References
- **Administrator's Guide:** Server management and policies
- **Developer's Guide:** Code modification and programming
- **[DG Scripting System](../systems/SCRIPTING_SYSTEM_DG.md):** Complete DG Scripts documentation
- **File Format Specifications:** Technical file format details

### Community Resources
- **Builder Forums:** Discussion and collaboration
- **Area Sharing:** Exchange of building resources
- **Tutorial Collections:** Step-by-step building guides
- **Inspiration Galleries:** Showcase of exceptional areas

### Tools and Utilities
- **OLC Help System:** Built-in help and guidance
- **Area Editors:** External tools for area creation
- **Map Generators:** Tools for creating area layouts
- **Balance Calculators:** Tools for stat balancing

---

*This manual is based on the original tbaMUD building documentation, updated and expanded for Luminari MUD. For the most current information and updates, consult the official documentation and community resources.*

*Last updated: July 2025*