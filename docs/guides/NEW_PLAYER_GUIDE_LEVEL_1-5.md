# New Player Guide - Levels 1-5
## Welcome to LuminariMUD!

This guide will help you navigate your first steps in LuminariMUD as a new character. The game can seem overwhelming at first, but this guide breaks down everything you need to know for levels 1 through 5.

---

## Table of Contents
1. [Getting Started](#getting-started)
2. [Understanding Your Character](#understanding-your-character)
3. [Basic Commands](#basic-commands)
4. [Combat Basics](#combat-basics)
5. [Improving Your Character](#improving-your-character)
6. [Starting Equipment](#starting-equipment)
7. [Finding Your Way](#finding-your-way)
8. [Quests and Missions](#quests-and-missions)
9. [Survival Tips](#survival-tips)
10. [Getting Help](#getting-help)

---

## Getting Started

### Character Creation
When you first connect to LuminariMUD, you'll create a new character by choosing:
- **Race**: Your character's species (Human, Elf, Dwarf, etc.)
- **Class**: Your character's profession (Warrior, Wizard, Rogue, Cleric, etc.)
- **Ability Scores**: Six core statistics that define your character

### Starting Location
You'll begin your adventure in the **Arrival Platform** (room 3000), which leads to the **Hall of Beginnings** (room 3001). This is a safe area where you can familiarize yourself with the game.

### Newbie Protection
As a new player (levels 1-6), you have several protections:
- **No Experience Loss**: You won't lose experience points when you die (levels 1-6)
- **No Gold Loss**: You won't lose gold when using the `unstuck` command (levels 1-5)
- **Zone Level Warnings**: The game will warn you if you try to enter areas too dangerous for your level

---

## Understanding Your Character

### Ability Scores
Your character has six core abilities that affect everything you do:

#### **Strength (STR)**
- Affects: Melee damage, carrying capacity, some skills
- Important for: Warriors, Rangers, Monks, Barbarians
- Bonus Formula: (Score - 10) / 2 (rounded down)

#### **Dexterity (DEX)**
- Affects: Armor Class (defense), ranged attacks, initiative, reflex saves
- Important for: Rogues, Rangers, Monks
- Bonus Formula: (Score - 10) / 2 (rounded down)

#### **Constitution (CON)**
- Affects: Hit points (health), fortitude saves
- Important for: Everyone, especially melee fighters
- Bonus Formula: (Score - 10) / 2 (rounded down)

#### **Intelligence (INT)**
- Affects: Wizard spells, skill points per level, some skills
- Important for: Wizards, Rogues
- Bonus Formula: (Score - 10) / 2 (rounded down)

#### **Wisdom (WIS)**
- Affects: Cleric/Druid spells, will saves, perception
- Important for: Clerics, Druids, Monks, Rangers
- Bonus Formula: (Score - 10) / 2 (rounded down)

#### **Charisma (CHA)**
- Affects: Bard/Sorcerer spells, some social skills
- Important for: Bards, Sorcerers, Paladins
- Bonus Formula: (Score - 10) / 2 (rounded down)

### Character Statistics

#### Hit Points (HP)
Your life force. When this reaches 0, you die. Check with the `score` command.
- Formula per level: Class base + Constitution bonus
- At level 1: You get maximum HP for your class

#### Movement Points (MV)
Used for walking, running, and performing actions. Regenerates over time.
- Running out of movement makes you exhausted
- Rest to recover movement faster

#### Spell Points (PSP) / Mana
For spellcasters, used to cast spells. Regenerates over time.
- Clerics, Wizards, Druids use PSP for spells
- Some classes have unlimited spell slots instead

#### Armor Class (AC)
Your defense against attacks. LOWER is better in this system.
- Base AC: 10
- Modified by: Dexterity bonus, armor, shields, magical items
- Target AC: Below 0 is excellent for low levels

### Experience Points (XP)
You gain experience by:
- Defeating enemies in combat
- Completing quests
- Exploring new areas
- Participating in group activities

**Level 1 to 2**: 1,000 XP
**Level 2 to 3**: 3,000 XP
**Level 3 to 4**: 6,000 XP
**Level 4 to 5**: 10,000 XP
**Level 5 to 6**: 15,000 XP

---

## Basic Commands

### Movement
```
north / n          - Move north
south / s          - Move south
east / e           - Move east
west / w           - Move west
up / u             - Move upward
down / d           - Move downward
```

### Examining Your Environment
```
look / l           - Look at your surroundings
look <target>      - Examine a specific person, object, or direction
look in <container>- Look inside a container
examine <item>     - Get detailed information about an item
```

### Inventory Management
```
inventory / i      - View items you're carrying
equipment / eq     - View items you're wearing/wielding
get <item>         - Pick up an item from the ground
get all            - Pick up all items from the ground
get all.<item>     - Pick up all of a specific item
drop <item>        - Drop an item
drop all           - Drop all items (be careful!)
wear <item>        - Equip an item
wield <weapon>     - Equip a weapon in your hand
remove <item>      - Remove a worn item
```

### Communication
```
say <message>      - Speak to everyone in the room
tell <player> <msg>- Send a private message
reply <message>    - Reply to last tell
gossip <message>   - Talk on the gossip channel (public)
chat <message>     - Chat channel (alias for gossip)
who                - List online players
```

### Information
```
score              - View your character statistics
affects / aff      - List active effects on you
time               - Check game time
weather            - Check weather conditions
help <topic>       - Access help on any topic
commands           - List available commands
```

### Resting and Recovery
```
rest               - Sit down and recover faster
sleep              - Sleep to recover even faster
wake               - Wake from sleep
stand              - Stand up
sit                - Sit down (furniture if available)
```

---

## Combat Basics

### Starting Combat
```
kill <target>      - Attack an enemy
hit <target>       - Same as kill
```

### During Combat
```
flee               - Attempt to escape (uses movement)
wimpy <hp>         - Auto-flee when HP drops below amount
wimpy off          - Disable auto-flee
```

### Combat Statistics

#### Attack Roll
- Roll: 1d20 + your attack bonus
- Must beat enemy's Armor Class (AC)
- Natural 20 = automatic hit (critical threat)
- Natural 1 = automatic miss

#### Damage Roll
- Based on your weapon
- Add strength bonus (for melee) or dexterity bonus (for ranged)
- Magical bonuses from enchanted weapons

#### Critical Hits
- Threat Range: Usually 20 (19-20 for some weapons)
- Confirmation Roll: Another attack roll to confirm
- Critical Multiplier: Usually x2 or x3 damage

### Combat Tips for Levels 1-5
1. **Don't fight alone**: Group up with other players when possible
2. **Know when to flee**: Set wimpy to 20-30% of max HP
3. **Use appropriate weapons**: Match your class (swords for fighters, bows for rangers, etc.)
4. **Heal between fights**: Rest to recover HP before engaging again
5. **Check enemy levels**: `consider <enemy>` to assess difficulty

### Death and Dying
If your HP reaches 0:
- **Level 1-6**: You lose NO experience points
- You'll respawn at the starting area
- Your equipment should remain on you (check `equipment`)
- Some temporary effects may be lost

---

## Improving Your Character

### Leveling Up
When you gain enough experience:
```
levelup            - Advance to the next level (when ready)
```

**What happens when you level:**
- Your HP, Movement, and possibly Spell Points increase
- You may gain new abilities or improve existing ones
- Every 4 levels (4, 8, 12, etc.) you can increase an ability score by 1
- You gain skill points to improve skills

### Skills
Skills are trained abilities that improve with practice and training.

#### Checking Skills
```
practice           - View all your skills and their ranks
skills             - Same as practice
```

#### Training Skills
Find a **Guild Trainer** (special NPC) and use:
```
practice <skill>   - Spend training points to improve a skill
```

**Common Guild Trainers:**
- Mosswood Village (starting area): Vnum 145387
- Sanctus: Vnum 196
- Various other locations

#### Skill Points per Level
- Base: Varies by class (Rogues get the most, Fighters get fewer)
- Bonus: +1 per point of Intelligence bonus
- At Level 1: You get 4x your normal skill points

#### Important Early Skills
**For Everyone:**
- **Perception**: Notice hidden things, avoid traps
- **Climb**: Climb in mountainous terrain
- **Swim**: Cross water without drowning

**For Warriors:**
- **Discipline**: Resist fear and mental effects
- **Intimidate**: Demoralize enemies

**For Rogues:**
- **Stealth**: Hide and move silently
- **Disable Device**: Disarm traps and pick locks
- **Sleight of Hand**: Pickpocket and palm objects

**For Spellcasters:**
- **Spellcraft**: Identify spells and magical effects
- **Knowledge (Arcana/Religion)**: Understand magic

### Feats
Feats are special abilities you can learn. You gain feats at:
- Level 1 (starting feats based on class and race)
- Every 3 levels (3, 6, 9, 12, etc.)
- Bonus feats from class features

**Common Early Feats:**
- **Weapon Focus**: +1 to hit with chosen weapon type
- **Dodge**: +1 AC
- **Toughness**: +3 HP per character level
- **Improved Initiative**: +4 initiative rolls
- **Power Attack**: Trade accuracy for damage

---

## Starting Equipment

### What You Begin With
When you create a character, you receive:
- **Basic starting equipment appropriate for your class**
- **Rations** (food - 5 pieces)
- **Waterskin** (water container)
- **Torches** (light sources - 3 torches)
- **Crafting tools** (if the crafting system is enabled)

### Class-Specific Gear
Different classes start with different equipment:

**Warriors/Fighters:**
- Weapon (usually a sword or axe)
- Basic armor
- Possibly a shield

**Wizards/Sorcerers:**
- Robes (light armor)
- Staff or dagger
- Spell components

**Clerics:**
- Mace or flail
- Medium armor
- Holy symbol
- Possibly a shield

**Rogues:**
- Light armor
- Short sword or daggers
- Thieves' tools

**Rangers:**
- Bow and arrows (in quiver)
- Light or medium armor
- Melee weapon

### Managing Your Equipment
```
equipment          - See what you're wearing/wielding
inventory          - See what's in your pack
wear <item>        - Equip armor or accessories
wield <weapon>     - Equip a weapon
remove <item>      - Take off equipped items
```

### Equipment Slots
Your character has multiple equipment slots:
- **Head**: Helmets, hats
- **Body**: Armor, robes, shirts
- **Arms**: Arm guards, sleeves
- **Hands**: Gloves, gauntlets
- **Legs**: Leg armor, pants
- **Feet**: Boots, shoes
- **Waist**: Belts
- **Neck**: Amulets, necklaces
- **Fingers**: Rings (2 slots)
- **Wield**: Primary weapon
- **Hold/Shield**: Secondary weapon or shield
- **Back**: Cloaks, capes

### Finding Better Equipment
1. **Loot from enemies**: Defeated foes may drop items
2. **Quest rewards**: Complete quests for equipment
3. **Shops**: Buy from merchants (see `list` in shops)
4. **Crafting**: Make your own (advanced)
5. **Treasure chests**: Find in dungeons and hidden areas

### Money (Gold Coins)
```
gold               - Check how much gold you have
drop <amount> coins- Drop coins on the ground
get <amount> coins - Pick up coins from the ground
```

**Earning Gold:**
- Looting enemies (automatic when they die)
- Completing quests
- Selling items to shops (`sell <item>`)
- Finding treasure

**Spending Wisely:**
- **Levels 1-5**: Focus on basic supplies (food, water, healing potions)
- **Save for**: Better armor and weapons
- **Avoid**: Expensive items you'll outgrow quickly

---

## Finding Your Way

### The Starting Area - Mosswood Village
Your adventure begins in or near **Mosswood Village**, a newbie-friendly area designed for levels 1-5.

**Key Locations:**
- **Guild Trainers**: Practice your skills here
- **Shops**: Buy basic supplies
- **Quest Givers**: NPCs offering quests (look for them!)

### Reading Room Descriptions
When you `look`, you'll see:
```
[Zone Name] - Room Title
<Room description with details about the environment>

Obvious exits: north, east, south
<List of characters and objects in the room>
```

**Exits:**
- Listed at the bottom of room descriptions
- `[door]` means there's a door (may need to open)
- `(closed)` means the door is closed
- No exit listed means you can't go that way

### Navigation Tips
```
map                - Display ASCII map (if available)
where              - Show other players' locations
recall             - Return to a safe recall point (if available)
```

### Wilderness Coordinates
Some areas use a coordinate system:
- Coordinates displayed as (X, Y)
- Use `map` to see your position
- Wilderness areas are larger and use grid movement

### Getting Unstuck
If you get lost or trapped:
```
unstuck <code>     - Teleport to starting area
                   (No cost for levels 1-5!)
```
The game will give you a confirmation code. Type `unstuck <code>` to confirm.

### Zone Levels
Areas have recommended level ranges:
- **The game warns you** if you enter a zone too high for your level
- **Mosswood Village**: Levels 1-5 (perfect for newbies!)
- **Sanctus**: Levels 1-10
- **Other starter zones**: Check `help zones` for list

---

## Quests and Missions

### Finding Quests
Look for **Questmasters** - NPCs who offer quests (marked with special descriptions).

#### Quest Commands
```
quest list         - View available quests from current questmaster
quest join <#>     - Accept a quest from the list
quest progress     - Check your current quest progress
quest progress <#> - Check specific quest slot progress
quest leave <#>    - Abandon a quest (careful!)
quest history      - View completed quests
```

### Quest Types

#### **Object Find (AQ_OBJ_FIND)**
- Task: Find and retrieve a specific object
- Reward: Shown when you complete it
- Tip: Check the quest progress to see what object you need

#### **Room Find (AQ_ROOM_FIND)**
- Task: Reach a specific location
- Reward: Given when you enter the room
- Tip: Explore systematically to find it

#### **Mob Find (AQ_MOB_FIND)**
- Task: Locate a specific NPC
- Reward: Given when you find them
- Tip: Ask other players or explore nearby areas

#### **Mob Kill (AQ_MOB_KILL)**
- Task: Defeat a specific enemy
- Reward: Experience, gold, possibly items
- Tip: Make sure you're high enough level!

#### **Mob Save (AQ_MOB_SAVE)**
- Task: Protect an NPC (usually from enemies)
- Reward: Gratitude and quest rewards
- Tip: Stay near the NPC and defend them

#### **Object Return (AQ_OBJ_RETURN)**
- Task: Bring an object to a specific NPC
- Reward: Quest completion reward
- Tip: Get the object first, then find the NPC

#### **Room Clear (AQ_ROOM_CLEAR)**
- Task: Defeat all enemies in a specific room
- Reward: Usually good XP and gold
- Tip: Make sure to kill ALL mobs in the room

### Quest Rewards
Quests may award:
- **Experience Points**: Help you level up
- **Gold Coins**: Currency for purchases
- **Items**: Equipment, consumables, or quest-specific rewards
- **Quest Points (QP)**: Special currency (advanced feature)
- **Reputation**: Standing with factions (advanced)

### Quest Tips for Levels 1-5
1. **Start with easy quests**: Look for level 1-5 recommended quests
2. **Read carefully**: Quest descriptions tell you what to do
3. **Check progress often**: Use `quest progress` to track objectives
4. **Don't abandon quests lightly**: You may have to wait to re-accept
5. **Group up**: Some quests are easier with friends

### Auto-Crafting Quests
Special quests where you craft items for NPCs:
```
supplyorder request  - Request a crafting quest
supplyorder complete - Complete and turn in crafted items
```
**Rewards (Levels 1-5):**
- 50 gold
- 100 experience points
- Sometimes quest points

---

## Survival Tips

### Health Management

#### Healing Methods
1. **Natural Regeneration**
   - Rest: Faster HP recovery
   - Sleep: Even faster HP recovery
   - Stand/Fight: Slowest recovery

2. **Healing Spells**
   - Clerics: `cast 'cure light wounds'`
   - Paladins: `lay on hands` ability
   - Potions: Buy from shops or find as loot

3. **Healing Items**
   - Healing potions
   - Healing kits (if available)
   - Food and water (restore some HP over time)

#### When to Heal
- **After every combat**: Top off your HP
- **Before dangerous areas**: Go in at full health
- **When HP drops below 50%**: Don't risk it

### Food and Water
Your character needs to eat and drink:
```
eat <food>         - Consume food
drink <container>  - Drink from waterskin or fountain
fill <container>   - Fill waterskin at a fountain
```

**Signs of hunger/thirst:**
- Messages appear periodically
- Ignoring them leads to stat penalties
- Eventually, you'll take damage

**Starting Food/Drink:**
- You begin with 5 rations
- You have a waterskin
- Shops sell more food and water

### Light Sources
Some areas are dark and require light:
```
light <torch>      - Light a torch
hold <light>       - Hold a light source
extinguish <light> - Put out a light source
```

### Avoiding Death
1. **Set wimpy**: `wimpy 30` (flee at 30 HP)
2. **Check enemy strength**: `consider <enemy>` before fighting
3. **Don't explore alone**: Group up for dangerous areas
4. **Keep healing items**: Always have potions or spells ready
5. **Know your limits**: If it's too hard, come back later

### Safe Zones
Some rooms are marked as **PEACEFUL**:
- No combat allowed
- Safe place to rest
- Often found in cities and temples

---

## Getting Help

### In-Game Help System
```
help               - Access main help
help <topic>       - Get help on specific topic
help index         - List all help topics
help commands      - List all commands
commands           - See available commands
```

### Useful Help Topics
```
help newbie        - New player information
help movement      - How to move around
help combat        - Combat basics
help commands      - Command list
help <class>       - Help for your specific class
help <skill>       - Help for specific skills
help <spell>       - Help for specific spells
```

### Asking Other Players
```
gossip <question>  - Ask on public channel
newbie <question>  - Newbie channel (if available)
who                - See who's online
tell <player> <msg>- Ask someone directly
```

### Immortals and Staff
Staff members (Immortals) can help with:
- Bug reports: `bug <description>`
- Ideas: `idea <description>`
- Typos: `typo <description>`
- Direct help: `tell <immortal name> <message>` (if they're available)

### When You're Stuck
1. **Read room descriptions carefully**: Clues are often in the text
2. **Try all exits**: Sometimes there are hidden exits
3. **Use 'look' on objects**: Examining things reveals information
4. **Ask for help**: Other players are usually friendly
5. **Use unstuck**: Last resort teleport to safety (no penalty for low levels!)

---

## Class-Specific Tips

### Warriors / Fighters
**Strengths:** High HP, good armor, excellent melee damage
**Weaknesses:** No magic, limited ranged options

**Early Tips:**
- Invest in Strength and Constitution
- Learn Power Attack feat for extra damage
- Use shields for better defense
- Practice Discipline skill for saves

**Combat Style:**
- Get in melee range and hit hard
- Use `bash` to knock enemies down
- Tank for groups (take the hits)

### Wizards
**Strengths:** Powerful spells, area effects, utility
**Weaknesses:** Low HP, weak in melee, limited spells per day

**Early Tips:**
- Maximize Intelligence
- Keep distance from enemies
- Memorize your spells: `memorize '<spell name>'`
- Rest to recover spell slots

**Combat Style:**
- Cast offensive spells from range
- Use Magic Missile (never misses)
- Save big spells for tough fights
- Stay behind the fighters

**Spell Management:**
```
memorize '<spell>' - Prepare a spell for casting
rest               - Recover spell slots
cast '<spell>' <target> - Cast a prepared spell
```

### Clerics
**Strengths:** Healing, support magic, decent combat
**Weaknesses:** Moderate HP, not as much damage as fighters

**Early Tips:**
- Balance Wisdom and Constitution
- Learn healing spells first
- Memorize Cure Light Wounds multiple times
- Use Turn Undead against undead enemies

**Combat Style:**
- Support the group with heals and buffs
- Use mace/flail in melee when needed
- Turn Undead is very effective
- Heal between fights

**Spell Management:**
```
memorize '<spell>' - Prepare a spell
cast 'cure light wounds' <target> - Heal someone
cast 'bless' <target> - Buff an ally
turn undead        - Special ability vs undead
```

### Rogues
**Strengths:** Skills, sneak attacks, versatility
**Weaknesses:** Lower HP, needs positioning

**Early Tips:**
- Maximize Dexterity
- Invest heavily in Stealth and Disable Device
- Use light armor (better for sneaking)
- Flank enemies for sneak attack damage

**Combat Style:**
- Use hide and sneak
- Backstab from hiding
- Flank enemies with allies
- Use ranged weapons when needed

**Special Abilities:**
```
hide               - Attempt to hide
sneak              - Move while hidden
backstab <target>  - Sneak attack from hiding
pick <lock/door>   - Pick locks
disable <trap>     - Disarm traps
```

### Rangers
**Strengths:** Archery, tracking, dual wielding, animal companions
**Weaknesses:** Not as tough as fighters, fewer spells than full casters

**Early Tips:**
- Balance Dexterity and Wisdom
- Choose your favored enemy wisely
- Practice with bow and arrow
- Get an animal companion when available

**Combat Style:**
- Use bow from range (arrows in quiver)
- Switch to dual wielding in melee
- Use spells for utility and buffs
- Track enemies through wilderness

**Special Commands:**
```
track <target>     - Track a creature
shoot <target>     - Fire your bow
dual wield         - Fight with two weapons
call companion     - Summon animal companion
```

### Monks
**Strengths:** Unarmed combat, high AC, special abilities, ki powers
**Weaknesses:** Best unarmored (hard to gear), need high stats

**Early Tips:**
- Maximize Dexterity and Wisdom
- Don't wear armor (lowers AC!)
- Use unarmed strikes
- Learn Flurry of Blows

**Combat Style:**
- Fight unarmed for best damage
- Use Flurry of Blows for multiple attacks
- Stunning Fist to disable enemies
- Very mobile in combat

**Special Abilities:**
```
flurry             - Multiple unarmed attacks
stunning fist      - Attempt to stun enemy
ki strike          - Use ki for special attacks
```

### Bards
**Strengths:** Support magic, skills, bardic music, versatility
**Weaknesses:** Not specialized, jack-of-all-trades

**Early Tips:**
- Balance Charisma and Dexterity
- Learn a variety of skills
- Use bardic music to buff allies
- Keep your instrument equipped

**Combat Style:**
- Start combat with bardic music
- Support with spells and crossbow
- High skill versatility
- Good for exploration

**Special Abilities:**
```
perform <song>     - Use bardic music
cast '<spell>'     - Cast bard spells
```

---

## Progression Checklist

### Level 1 Goals
- [ ] Complete character creation
- [ ] Learn basic movement commands
- [ ] Practice combat on weak enemies
- [ ] Visit the guild trainer
- [ ] Accept your first quest
- [ ] Reach level 2

### Level 2 Goals
- [ ] Spend your skill points wisely
- [ ] Join a group if possible
- [ ] Complete 2-3 quests
- [ ] Upgrade one piece of equipment
- [ ] Reach level 3

### Level 3 Goals
- [ ] Gain your first feat (every 3 levels)
- [ ] Explore beyond the starting area
- [ ] Learn your class abilities
- [ ] Start saving gold for better gear
- [ ] Reach level 4

### Level 4 Goals
- [ ] Increase an ability score by 1
- [ ] Complete 5+ quests
- [ ] Find or buy improved weapons/armor
- [ ] Try group content
- [ ] Reach level 5

### Level 5 Goals
- [ ] Master your basic abilities
- [ ] Have full equipment set
- [ ] Complete major quest chains
- [ ] Save gold for future purchases
- [ ] Prepare for level 6 (newbie protection ends!)

---

## Quick Reference Card

### Essential Commands
```
Movement:     n, s, e, w, u, d
Look:         look, look <target>
Inventory:    inventory, equipment, get, drop, wear, wield, remove
Combat:       kill, flee, wimpy
Information:  score, affects, who, time
Help:         help, help <topic>
Communication: say, tell, gossip, reply
Recovery:     rest, sleep, stand, wake
Quest:        quest list, quest join, quest progress
```

### Survival Quick Tips
1. Set wimpy: `wimpy 30`
2. Rest after fights
3. Keep food and water
4. Check enemy difficulty: `consider <enemy>`
5. Use unstuck if truly stuck (levels 1-5 have no penalty)

### Level Up Quick Steps
1. Gain required experience
2. Type `levelup`
3. Spend skill points with trainer: `practice <skill>`
4. Choose feat (if applicable level)
5. Increase ability score (levels 4, 8, 12, etc.)

---

## Conclusion

LuminariMUD is a rich and complex world with many systems to explore. This guide covers the essentials for your first 5 levels. As you grow more experienced, you'll discover:

- **Advanced combat tactics**
- **Crafting systems**
- **Clan/guild systems**
- **Epic quests and storylines**
- **Pk (player vs. player) content**
- **High-level dungeons and raids**
- **Prestige classes and multiclassing**

**Remember:**
- **Ask questions** - Other players and staff are here to help
- **Explore safely** - Check zone levels before entering
- **Have fun** - This is a game, enjoy the adventure!
- **Be patient** - Learning takes time, but it's worth it

**Welcome to LuminariMUD, adventurer. Your journey begins now!**

---

## Additional Resources

### In-Game
- `help newbie` - Newbie help topics
- `help <class>` - Class-specific information
- `help <topic>` - Specific help on any topic
- `gossip` - Ask the community

### Staff Contacta
- `bug <description>` - Report bugs
- `idea <description>` - Submit ideas
- `typo <description>` - Report typos
- `tell <immortal>` - Contact staff (if available)

### Key Starting Locations
- **Mosswood Village** (145387) - Level 1-5 starting area
- **Sanctus** (Vnum 196) - Guild trainer location
- **Arrival Platform** (3000) - Initial starting room
- **Hall of Beginnings** (3001) - Newbie hub

---

*Document Version: 1.0*  
*Last Updated: November 11, 2025*  
*For: LuminariMUD - Levels 1-5 Players*
