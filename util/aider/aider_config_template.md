# Aider Configuration for LuminariMUD World Editing
*Customized for LuminariMUD's Pathfinder/D&D 3.5 implementation*

## Step 1: Create LuminariMUD-Specific Configuration

### Update your .aider.conf.yml

```bash
cat > .aider.conf.yml << 'EOF'
# Aider Configuration File for LuminariMUD
# https://aider.chat/docs/config.html

# OpenRouter API Configuration
openai-api-base: https://openrouter.ai/api/v1
api-key: openrouter=sk-or-v1-YOUR_KEY_HERE

# Model Selection (use free models for cost efficiency)
model: openrouter/deepseek/deepseek-chat:free

# LuminariMUD world editing specific settings
map-tokens: 4096
auto-commits: false
git: false
gitignore: false
dirty-commits: false
attribute-author: false

# Enable streaming for better responsiveness
stream: true
pretty: true
EOF
```

## Step 2: Create .aiderignore File

```bash
cat > .aiderignore << 'EOF'
# Ignore compiled binaries and object files
bin/
*.o
*.a
circle
autorun
*.so
*.dylib

# Ignore player files and runtime data
lib/plrfiles/
lib/plrobjs/
lib/plrvars/
lib/plralias/
lib/plrtext/
lib/house/
lib/plrmail/
lib/mudmail/
lib/etc/

# Ignore logs and crash files
log/
syslog*
lib/misc/crash_*
lib/misc/*.backup
*.log

# Ignore core dumps and debug files
core
core.*
vgcore.*
*.core
gdb.txt

# Ignore backup files
*.bak
*.backup
lib/world/backups/
bak/

# IMPORTANT: Don't let aider touch git
.git/
.gitignore

# Ignore configure-generated files
Makefile
config.status
config.cache
src/conf.h

# Ignore sensitive configuration
lib/mysql_config
.env
campaign.h
vnums.h
mud_options.h
EOF
```

## Step 3: Create LuminariMUD Context File

```bash
cat > .aider.luminari.context.md << 'EOF'
# LuminariMUD World Structure

## CRITICAL RULES
- NEVER use git commands or commits on world files
- World files are edited directly and loaded on reboot/copyover
- ALWAYS backup files before editing (use backup-zone.sh script)
- Changes go live when MUD reboots or zones are reloaded
- Preserve existing VNUM assignments (breaking them corrupts player data)

## Directory Layout
- `lib/world/wld/` - Room/World files (.wld) - Room descriptions and exits
- `lib/world/mob/` - Mobile/NPC files (.mob) - NPCs with Pathfinder stats
- `lib/world/obj/` - Object files (.obj) - Items, weapons, armor, etc.
- `lib/world/shp/` - Shop files (.shp) - NPC merchant definitions
- `lib/world/zon/` - Zone reset files (.zon) - Zone commands and resets
- `lib/world/trg/` - DG Script trigger files (.trg) - Scripted behaviors
- `lib/world/qst/` - Quest files (.qst) - Quest definitions
- `lib/world/hlq/` - Help quest files (.hlq) - Extended quest data
- `lib/text/` - Help files and world text
- `lib/misc/` - Messages, socials, and other data

## File Format Rules
1. Zone numbering: Standard zones 0-999, special zones 1000+
2. Room VNUMs: Typically [zone# * 100] to [(zone# * 100) + 99]
3. All VNUMs must be unique within their type (room/mob/obj)
4. Use ~ to terminate strings in all file formats
5. Use $ to end file sections where required
6. Format MUST match existing patterns exactly (spacing matters!)

## LuminariMUD-Specific Features
- Full Pathfinder/D&D 3.5 stats (Str, Dex, Con, Int, Wis, Cha)
- Saving throws (Fort, Refl, Will, Poison, Death)
- Race and SubRace system (SubRace1, SubRace2, SubRace3)
- Class system with multi-classing support
- Feats system
- Size categories (Fine to Colossal)
- Climate zones (affects gameplay)
- Treasure system with random generation
- Crafting materials and systems

## World File (.wld) Format
#VNUM
Room Name~
   Multi-line room description. Should be immersive and detailed.
Include details about atmosphere, visible features, and exits.
~
zone_number room_flags1 room_flags2 room_flags3 room_flags4 sector_type
D0-5 (North/East/South/West/Up/Down)
Exit description when looking that direction~
Keywords for door (if any)~
door_flag key_vnum target_room_vnum
E (Extra descriptions)
keywords~
Description when examining keywords~
C (Coordinates - optional)
x y
S (End of room)

## Mobile File (.mob) Format
#VNUM
keyword list~
short description (a goblin warrior)~
long description (in room)
~
detailed description (when examined)
~
mob_flags1 mob_flags2 mob_flags3 mob_flags4 aff_flags1 aff_flags2 aff_flags3 aff_flags4 alignment E
level hitroll armor hp psp move hit_dice damage_dice damroll
gold experience
position default_position sex
Str: value
Dex: value  
Con: value
Int: value
Wis: value
Cha: value
Size: value
Class: class_number
Race: race_number
BareHandAttack: attack_type
SavingFort: value
SavingRefl: value
SavingWill: value
SavingPoison: value
SavingDeath: value
SubRace 1: value
SubRace 2: value
SubRace 3: value
Walkin: text
Walkout: text
T trigger_vnum (trigger assignments)
E (End of mobile)

## Object File (.obj) Format
#VNUM
keywords~
short description (a steel sword)~
long description (on ground)~
action description (special)~
type_flag extra_flags wear_flags material_type
value0 value1 value2 value3
weight cost rent min_level timer

Lettered sections (zero or more, in any order):
- A (Apply): location modifier [bonus_type] [specific]
- B (Spellbook Slot): spell_id pages
- C (Special ability): ability level activation value0 value1 value2 value3 [command_word]
- E (Extra description):
  - keywords~
  - description~
- G (Proficiency): proficiency_number
- H (Material): material_number
- I (Size): size_number
- J (Mob recipient): mobile_vnum
- K (Activated spells): spell_id level percent procs_in_combat

Note: Objects do not use a specific end-of-record marker; the next `#VNUM` or `$` ends the record.

## Zone File (.zon) Format
#ZONE_NUM
Builder Name~
Zone Name~
bot top lifespan reset_mode zone_flags1 zone_flags2 zone_flags3 zone_flags4 min_level max_level [show_weather] [region] [faction] [city]
* Reset commands:
M 0 mob_vnum max_exist room_vnum max_in_room (Load mobile)
E 1 obj_vnum max_exist wear_position max_in_world (Equip on previous mob)
G 1 obj_vnum max_exist max_in_world (Give to previous mob)
O 0 obj_vnum max_exist room_vnum max_in_room (Load object in room)
P 1 obj_vnum max_exist container_vnum max_in_container (Put in container)
D 0 room_vnum direction door_state (Set door state)
R 0 room_vnum obj_vnum (Remove object from room)
S (End of zone commands)

## Testing Practices
- Use ./bin/circle -c -q to syntax check
- Test on development port (e.g., 4001) before live
- Use 'stat' commands in-game to verify changes
- Check 'syslog' for loading errors
- Use 'zreset' command to test zone resets

## Balance Guidelines
- Level 1-5: Basic equipment, 1d4-1d6 damage
- Level 6-10: +1 enchantments, 1d8 damage, minor magic
- Level 11-15: +2 enchantments, 1d10 damage, moderate magic
- Level 16-20: +3 enchantments, 2d6 damage, significant magic
- Level 21-25: +4 enchantments, 2d8 damage, powerful magic
- Level 26-30: +5 enchantments, epic items, legendary effects

## Common VNUMs to Know
- Zone 0: Void and Immortal areas
- Zone 1: Sanctus (main city)
- Zone 30: The Temple area
- Zone 100+: Various adventure zones
- Zone 600: Mosswood
- Zone 10000+: Special/instanced content
EOF
```

## Step 4: Create LuminariMUD Prompts File

```bash
cat > .aider.luminari.prompts.md << 'EOF'
# LuminariMUD World Building Instructions

When editing LuminariMUD world files, follow these guidelines:

## Writing Style
- Medieval fantasy with Pathfinder/D&D flavor
- Rich, immersive descriptions using all senses
- Avoid modern references unless thematically appropriate
- Maintain consistent tone within zones
- Use proper grammar and British spelling when appropriate

## Room Descriptions
1. **First Line**: Set the scene immediately
2. **Atmosphere**: Include lighting, temperature, sounds, smells
3. **Details**: Describe 2-3 notable features players might examine
4. **Exits**: Naturally incorporate visible exits into description
5. **Length**: Aim for 3-6 lines, more for important rooms
6. **Format**: Indent continuation lines with 2 spaces

Example:
```
   The ancient stone walls of this chamber bear the scars of countless
battles, with deep gouges and burn marks telling tales of violence.
Flickering torchlight casts dancing shadows across scattered bones and
rusted weapons. A musty smell of decay hangs heavy in the air, broken
only by a faint draft from the darkened passage leading north.
```

## NPC/Mobile Creation
1. **Keywords**: Include all reasonable variations (guard soldier warrior)
2. **Short Desc**: What players see in room (a grizzled guard)
3. **Long Desc**: Full sentence with period (A grizzled guard stands watch here.)
4. **Detailed**: 3-5 lines when examined, personality and appearance
5. **Stats**: Appropriate for zone level and NPC role
6. **Equipment**: Logical for NPC type and level

## Object Design
1. **Keywords**: All reasonable terms (sword blade weapon steel)
2. **Weight**: Realistic (longsword = 40, dagger = 10)
3. **Value**: Appropriate for level and rarity
4. **Material**: Matching description (steel, wood, cloth, etc.)
5. **Stats**: Balanced for intended level range
6. **Descriptions**: Both ground and inventory views

## Zone Resets
1. **Mobile Loading**: M command before E/G commands
2. **Max Existing**: Usually 1 for unique NPCs, higher for common
3. **Equipment**: E for worn items, G for inventory
4. **Object Loading**: O for room objects, P for containers
5. **Door States**: D commands at end of reset list
6. **Reset Timer**: 15-30 minutes typical, longer for bosses

## D&D/Pathfinder Stat Guidelines

### NPC Statistics by Level
- **Level 1-5**: HP 10-50, AC 10-15, Damage 1d6
- **Level 6-10**: HP 50-100, AC 15-20, Damage 1d8+2
- **Level 11-15**: HP 100-200, AC 20-25, Damage 2d6+4
- **Level 16-20**: HP 200-400, AC 25-30, Damage 2d8+6
- **Level 21-25**: HP 400-600, AC 30-35, Damage 3d6+8
- **Level 26-30**: HP 600-1000, AC 35-40, Damage 3d8+10

### Ability Scores
- Commoner: 8-11 in all stats
- Soldier: Str 14-16, Con 12-14
- Mage: Int 16-18, Wis 14-16
- Boss: Primary stat 18-20+

### Saving Throws by Level
- Low Level (1-10): Fort/Refl/Will 0-5
- Mid Level (11-20): Fort/Refl/Will 5-10
- High Level (21-30): Fort/Refl/Will 10-15

## DG Script Triggers
1. **Trigger Types**: Use appropriate type (mob/obj/room)
2. **Variables**: Check existing scripts for variable naming
3. **Performance**: Avoid heavy loops or constant checks
4. **Testing**: Test all paths through conditional logic

## Quest Design
1. **Clear Objectives**: State goals explicitly
2. **Rewards**: Scale with difficulty and level
3. **Dialogue**: Character-appropriate speech patterns
4. **Multiple Steps**: Break complex quests into stages
5. **Item Requirements**: Ensure items are obtainable

## Common Mistakes to Avoid
- L Changing existing VNUMs (breaks saves)
- L Missing ~ terminators (causes parse errors)
- L Inconsistent spacing (format matters!)
- L Unbalanced stats (test similar content)
- L Missing reset commands (NPCs won't load)
- L Wrong sector types (affects movement)
- L Circular exits (unless intentional)

## File Validation Checklist
- [ ] All strings terminated with ~
- [ ] VNUMs are unique and in sequence
- [ ] Stats appropriate for level
- [ ] Reset commands in correct order
- [ ] Exits connect properly
- [ ] Keywords comprehensive
- [ ] Descriptions immersive
- [ ] Format matches examples exactly
EOF
```

## Step 5: Working with World Files

### Start Aider for World Editing
```bash
# Navigate to MUD directory
cd /mnt/c/Projects/Luminari-Source

# Activate virtual environment
source aider-env/bin/activate

# Start with NO GIT for world file editing
aider --no-git --read .aider.luminari.context.md --read .aider.luminari.prompts.md \
      lib/world/wld/30.wld lib/world/mob/30.mob lib/world/obj/30.obj lib/world/zon/30.zon

# Or use the simpler command after aliasing (see Step 7)
luminari-zone 30
```

### Common World Editing Commands
```bash
# In aider prompt:

# Add entire zone for editing
> /add lib/world/wld/100.wld lib/world/mob/100.mob lib/world/obj/100.obj lib/world/zon/100.zon

# Add scripts and shops
> /add lib/world/trg/100.trg lib/world/shp/100.shp

# Read source for reference (read-only)
> /read src/structs.h src/spells.h src/class.c

# Example editing requests:
> Create a new tavern in room 10050 called "The Wandering Dragon"
> Add a level 15 bartender NPC with appropriate Pathfinder stats
> Create a shop for the bartender selling healing potions and food
> Add quest dialogue to the bartender about local rumors
> Create connecting rooms 10051-10053 for tavern private rooms
```

## Step 6: Backup and Testing Workflow

### Create Backup Scripts
```bash
# Create backup script
cat > backup-zone.sh << 'EOF'
#!/bin/bash
# Backup a specific zone
if [ -z "$1" ]; then
    echo "Usage: $0 <zone_number>"
    exit 1
fi

ZONE=$1
DATE=$(date +%Y%m%d-%H%M)
BACKUP_DIR="lib/world/backups/${DATE}"

mkdir -p "${BACKUP_DIR}"

# Backup all file types for this zone
for ext in wld mob obj zon trg shp qst hlq; do
    if [ -f "lib/world/${ext}/${ZONE}.${ext}" ]; then
        cp "lib/world/${ext}/${ZONE}.${ext}" "${BACKUP_DIR}/${ZONE}.${ext}.bak"
        echo "Backed up ${ZONE}.${ext}"
    fi
done

echo "Zone ${ZONE} backed up to ${BACKUP_DIR}"
EOF

chmod +x backup-zone.sh
```

### Safe Editing Workflow
```bash
# 1. Backup the zone
./backup-zone.sh 100

# 2. Edit with Aider (NO GIT)
aider --no-git lib/world/wld/100.wld lib/world/mob/100.mob

# 3. Syntax check
./bin/circle -c -q

# 4. Check for errors
tail -n 50 syslog

# 5. Test on dev port if available
# ./bin/circle 4001 &

# 6. Connect and test
# telnet localhost 4001

# 7. If bad, restore backup
# ./restore-zone.sh 100 $(date +%Y%m%d-%H%M)
```

## Step 7: Useful Aliases for LuminariMUD

```bash
# Add to ~/.bashrc
cat >> ~/.bashrc << 'EOF'

# LuminariMUD World Building Aliases
export LUMINARI_DIR="/mnt/c/Projects/Luminari-Source"

# Quick navigation
alias cdmud='cd $LUMINARI_DIR'
alias cdworld='cd $LUMINARI_DIR/lib/world'

# Aider shortcuts
alias luminari-aider='cd $LUMINARI_DIR && source aider-env/bin/activate && aider --no-git --read .aider.luminari.context.md --read .aider.luminari.prompts.md'
alias luminari-zone='luminari-aider lib/world/wld/$1.wld lib/world/mob/$1.mob lib/world/obj/$1.obj lib/world/zon/$1.zon'

# Backup commands
alias backup-zone='$LUMINARI_DIR/backup-zone.sh'
alias backup-world='tar -czf $LUMINARI_DIR/lib/world/backups/world-$(date +%Y%m%d-%H%M).tar.gz -C $LUMINARI_DIR lib/world/'

# Quick checks
alias check-mud='$LUMINARI_DIR/bin/circle -c -q'
alias check-log='tail -n 100 $LUMINARI_DIR/syslog'
alias check-errors='grep -i error $LUMINARI_DIR/syslog | tail -n 20'

# Zone management
alias list-zones='ls -la $LUMINARI_DIR/lib/world/zon/*.zon | cut -d/ -f8 | cut -d. -f1 | sort -n'
alias zone-info='grep -H "^#" $LUMINARI_DIR/lib/world/zon/$1.zon | head -3'
EOF

source ~/.bashrc
```

## Step 8: Templates for Common Tasks

### Creating a New Shop
```
> Create a potion shop for room 10050. The shopkeeper should be a 
> level 15 alchemist with Int 16, Wis 14. Sell healing potions (light, 
> moderate, serious), mana potions, and antidotes. Set appropriate 
> prices for level 15 area. Include personality in descriptions.
```

### Adding a Quest NPC
```
> Add a quest giver NPC to room 10051. Level 20 retired adventurer with
> scars and stories. Should have dialogue about three different quests:
> 1. Retrieve stolen artifact (level 18)
> 2. Clear undead from cemetery (level 20)
> 3. Investigate disappearances (level 22)
> Include Pathfinder-appropriate stats and equipment.
```

### Creating a Dungeon Sequence
```
> Create rooms 10060-10065 as a goblin warren. Include:
> - Entrance with guards (10060)
> - Main tunnel with traps (10061)
> - Side chamber with shaman (10062)
> - Treasury with locked chest (10063)
> - Chief's chamber with boss mob (10064)
> - Secret escape tunnel (10065)
> All appropriate for level 8-10 party.
```

### Balancing Items
```
> Review all weapons in zone 100 and adjust their stats for level 15-20 
> range. Ensure damage dice, bonuses, and special properties are 
> balanced. Follow Pathfinder progression guidelines.
```

## Step 9: Testing Your Changes

### In-Game Commands for Testing
```
# Implementor/Admin commands:
goto <room_vnum>      # Teleport to room
stat room            # Show room statistics
stat mob <mob_name>  # Show mobile statistics  
stat obj <obj_name>  # Show object statistics
zreset <zone_num>    # Force zone reset
load mob <vnum>      # Load a mobile
load obj <vnum>      # Load an object
purge                # Remove all mobs/objs in room
show zones           # List all zones
vnum mob <keyword>   # Find mob vnums
vnum obj <keyword>   # Find object vnums
vnum room <keyword>  # Find room vnums
```

### Validation Script
```bash
# Create validation script
cat > validate-zone.sh << 'EOF'
#!/bin/bash
ZONE=$1
echo "Validating zone $ZONE..."

# Check file exists
for ext in wld mob obj zon; do
    if [ ! -f "lib/world/${ext}/${ZONE}.${ext}" ]; then
        echo "WARNING: Missing ${ZONE}.${ext}"
    fi
done

# Check for syntax
./bin/circle -c -q 2>&1 | grep -i "error\|warning" | grep "${ZONE}"

# Check for terminators
echo "Checking terminators..."
grep -L "~$" lib/world/*/${ZONE}.* 2>/dev/null

echo "Validation complete"
EOF

chmod +x validate-zone.sh
```

## Step 10: Emergency Recovery

```bash
# If you corrupt a file:

# 1. Check recent backups
ls -la lib/world/backups/*/100.* | tail -10

# 2. Restore specific file
cp lib/world/backups/20240101-1200/100.wld.bak lib/world/wld/100.wld

# 3. Or restore entire zone
for ext in wld mob obj zon trg shp; do
    if [ -f "lib/world/backups/20240101-1200/100.${ext}.bak" ]; then
        cp "lib/world/backups/20240101-1200/100.${ext}.bak" "lib/world/${ext}/100.${ext}"
    fi
done

# 4. Verify fixes
./bin/circle -c -q
grep ERROR syslog | tail
```

## Important Reminders

### DO's:
-  ALWAYS backup before editing
-  Use `--no-git` flag for world files
-  Test changes before live deployment
-  Maintain exact file formatting
-  Follow Pathfinder/D&D balance guidelines
-  Document special features in zone headers
-  Check existing similar content for examples
-  Use the climate system appropriately

### DON'Ts:
- L NEVER commit world files to git
- L NEVER change existing VNUMs
- L NEVER edit lib/plrfiles/ or player data
- L Don't break file format standards
- L Don't create unbalanced content
- L Don't forget ~ terminators
- L Don't mix zone themes inappropriately
- L Don't edit sensitive config files

## Quick Command Reference

```bash
# Start editing zone 100
luminari-zone 100

# Backup before editing
backup-zone 100

# Check syntax
check-mud

# View recent errors
check-errors

# Full world backup
backup-world

# Restore from backup
./restore-zone.sh 100 20240101-1200
```

## Support Resources

- **CLAUDE.md**: Project-specific AI instructions at `/mnt/c/Projects/Luminari-Source/CLAUDE.md`
- **Source Reference**: `src/structs.h` for flags and types
- **Spell List**: `src/spells.h` for spell numbers
- **Class Info**: `src/class.c` for class definitions
- **Race Info**: `src/race.c` for race definitions
- **DG Scripts**: `lib/world/trg/index` for example scripts
- **Help Files**: `lib/text/help/` for game documentation

---

*This configuration is specifically tailored for LuminariMUD's Pathfinder/D&D 3.5 implementation. Always backup before editing. World files are LIVE DATA not tracked in git. Use `--no-git` with aider for world editing!*