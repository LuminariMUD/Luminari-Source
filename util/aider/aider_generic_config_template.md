# Aider Default Configuration for MUD World Editing
*You MUST make sure it is accurate for YOUR MUD!*

## Step 1: Create MUD-Specific Configuration

### Update your .aider.conf.yml

Manually edit OR use 'cat'

```bash
cat > .aider.conf.yml << 'EOF'
openai-api-base: https://openrouter.ai/api/v1
api-key: openrouter=sk-or-v1-YOUR_KEY_HERE
model: openrouter/deepseek/deepseek-chat:free

# MUD world editing specific settings
map-tokens: 4096
auto-commits: false
git: false
gitignore: false
dirty-commits: false
attribute-author: false
EOF
```

## Step 2: Create .aiderignore File

Manually edit OR use 'cat'

```bash
# This tells Aider what to ignore (like .gitignore but for aider)
cat > .aiderignore << 'EOF'
# Ignore compiled binaries and object files
bin/
*.o
*.a
circle
autorun

# Ignore player files and runtime data
lib/plrfiles/
lib/plrobjs/
lib/plrvars/
lib/plralias/
lib/plrtext/
lib/house/
lib/plrmail/

# Ignore logs and crash files
log/
syslog*
lib/misc/crash_*
lib/misc/*.backup

# Ignore core dumps and debug files
core
core.*
vgcore.*
*.core

# IMPORTANT: Don't let aider touch git
.git/
.gitignore

# Keep source visible for context but don't edit
# (Remove these if you want to edit source too)
# src/**/*.c
# src/**/*.h
EOF
```

## Step 3: Create MUD Context File

Manually edit OR use 'cat'

```bash
# Create a context file that explains your MUD structure
cat > .aider.mud.context.md << 'EOF'
# MUD World Structure

## CRITICAL RULES
- NEVER use git commands or commits on world files
- World files are edited directly and loaded on reboot/copyover
- ALWAYS backup files before editing
- Changes go live when MUD reboots

## Directory Layout
- `lib/world/wld/` - Room files (.wld)
- `lib/world/mob/` - Mobile (NPC) files (.mob)
- `lib/world/obj/` - Object files (.obj)
- `lib/world/shp/` - Shop files (.shp)
- `lib/world/zon/` - Zone command files (.zon)
- `lib/world/trg/` - DG Script trigger files (.trg)
- `lib/world/qst/` - Quest files (.qst)
- `lib/text/` - Help files and world text
- `lib/misc/` - Messages, socials, and other data

## File Format Rules
1. Zone numbers: 0-326 are typically core zones
2. Room vnums: [zone# * 100] to [(zone# * 100) + 99]
3. All vnums must be unique within their type
4. Files use specific formatting (see examples in existing files)
5. Use $ to end file sections
6. Comments start with * in some formats

## Common Tasks
- Adding rooms: Edit .wld files
- Creating NPCs: Edit .mob files  
- Making items: Edit .obj files
- Zone resets: Edit .zon files
- Adding shops: Edit .shp files
- DG Scripts: Edit .trg files

## Important Rules
- NEVER change vnums of existing content (breaks player data)
- Test all changes on development port first
- Keep zone themes consistent
- Balance considerations: check similar level zones
- Document special procedures in zone header comments
EOF
```

## Step 4: Create Custom Prompts File

Manually edit OR use 'cat'

```bash
# Create a prompts file for MUD-specific instructions
cat > .aider.mud.prompts.md << 'EOF'
# MUD World Editing Instructions

IMPORTANT: These are WORLD FILES, not source code. They are:
- Edited directly on the live server
- Loaded by the MUD on reboot/copyover
- NOT tracked in git
- Need to be backed up manually before major changes

When editing MUD world files:

1. **Room Descriptions**: 
   - Use vivid, atmospheric descriptions
   - Include obvious exits in description
   - Consider all senses (sight, sound, smell)
   - Keep descriptions 3-6 lines typically

2. **NPC Creation**:
   - Names should fit the zone theme
   - Long descriptions for when viewed in room
   - Detailed descriptions for when examined
   - Appropriate level and stats for zone

3. **Object Design**:
   - Logical weight and value
   - Wear positions must make sense
   - Effects should be balanced for level
   - Include both short and long descriptions

4. **Zone Resets**:
   - M (load mob) before E (equip) commands
   - O (load obj) for room objects
   - D (door) states at zone reset
   - Appropriate reset timers

5. **File Format Standards**:
   - Maintain exact formatting of existing files
   - Use ~ for string terminators where required
   - End sections with $ 
   - Preserve vnum sequencing

6. **Balance Guidelines**:
   - Check similar level content for balance
   - Gold/exp rewards appropriate to difficulty
   - Item stats scaled to level requirements
   - Shop prices reasonable for economy

7. **Writing Style**:
   - Medieval fantasy theme by default
   - Avoid modern references unless zone-appropriate
   - Consistent tone within zones
   - Proper grammar and spelling

Always preserve existing vnums and maintain backwards compatibility!
EOF
```

## Step 5: Working with World Files

### Start Aider WITHOUT Git
```bash
# Navigate to MUD directory
cd ~/MUD-DIR

# Activate virtual environment
source aider-env/bin/activate

# Start with NO GIT integration for world editing
aider --no-git --read .aider.mud.context.md --read .aider.mud.prompts.md \
      lib/world/wld/30.wld lib/world/mob/30.mob lib/world/obj/30.obj
```

### Common World Editing Commands
```bash
# Add entire zone for editing
> /add lib/world/wld/30.wld lib/world/mob/30.mob lib/world/obj/30.obj lib/world/zon/30.zon

# Read source files for context (read-only)
> /read src/act.wizard.c src/structs.h

# Example requests:
> Create a new tavern room after room 3050 with a bartender NPC
> Add a quest item (a golden amulet) to zone 30 with magical properties
> Create a shop for the bartender selling drinks and food
> Write room descriptions for a haunted forest path (rooms 3060-3065)
> Balance the stats on all weapons in this zone for level 20 players
```

## Step 6: Backup and Testing Workflow

### ALWAYS Backup Before Editing
```bash
# Create backup directory with date
mkdir -p lib/world/backups/$(date +%Y%m%d)

# Backup specific zone before editing
cp lib/world/wld/30.wld lib/world/backups/$(date +%Y%m%d)/30.wld.bak
cp lib/world/mob/30.mob lib/world/backups/$(date +%Y%m%d)/30.mob.bak
cp lib/world/obj/30.obj lib/world/backups/$(date +%Y%m%d)/30.obj.bak

# Or backup entire world
tar -czf lib/world/backups/world-$(date +%Y%m%d-%H%M).tar.gz lib/world/
```

### Safe Editing Workflow
```bash
# 1. Backup the zone
./backup-zone.sh 30

# 2. Edit with Aider (NO GIT)
aider --no-git lib/world/wld/30.wld lib/world/mob/30.mob
> Create a new shop in room 3050...

# 3. Syntax check
./bin/circle -c -q

# 4. Test on dev port
./bin/circle -q 4001 &
telnet localhost 4001

# 5. If good, reload on live server (or wait for reboot)
# If bad, restore from backup:
cp lib/world/backups/$(date +%Y%m%d)/30.wld.bak lib/world/wld/30.wld
```

## Step 7: Useful Aliases for World Building
```bash
# Add to ~/.bashrc for quick access
echo '
# MUD World Building aliases
alias aider-world="cd ~/MUD-DIR && source aider-env/bin/activate && aider --no-git --read .aider.mud.context.md --read .aider.mud.prompts.md"
alias aider-zone="aider-world lib/world/wld/\$1.wld lib/world/mob/\$1.mob lib/world/obj/\$1.obj lib/world/zon/\$1.zon"
alias backup-zone="mkdir -p lib/world/backups/\$(date +%Y%m%d) && cp lib/world/*/$1.* lib/world/backups/\$(date +%Y%m%d)/"
alias backup-world="tar -czf lib/world/backups/world-\$(date +%Y%m%d-%H%M).tar.gz lib/world/"
' >> ~/.bashrc

# Usage after sourcing bashrc:
# backup-zone 30              # Backup zone 30
# aider-world                 # Start world editor (no git)
# aider-zone 30              # Edit all files for zone 30
```

## Step 8: Zone File Templates

### Create templates for Aider to reference
```bash
mkdir -p lib/world/templates

# Room template
cat > lib/world/templates/room.template << 'EOF'
#VNUM
Room Name~
   Room description goes here. This should be detailed and immersive,
describing what players see when they enter the room. Include details
about exits, atmosphere, and any notable features.
~
ZONE_NUM FLAGS SECTOR_TYPE
D0
Exit description for north~
~
0 0 TARGET_VNUM
S
EOF

# Add mob and obj templates similarly...
```

## Common MUD World Editing Patterns

### Adding a New Area
```
> I need to create a new level 25-30 undead themed zone (zone 125). 
> Start with 5 connected rooms forming a haunted crypt entrance.
> Include atmospheric descriptions and appropriate sector types.
```

### Creating NPCs
```
> Create a level 28 skeleton warrior (vnum 12501) with appropriate stats.
> Give it equipment: rusty sword and battered shield.
> Add loading instructions in the .zon file for room 12500.
```

### Building Shops
```
> Create a potion shop in room 12510. 
> The shopkeeper should be a mysterious alchemist.
> Sell healing, mana, and stat-boost potions appropriate for level 25-30.
```

### Quest Items
```
> Design a quest line item: "The Amulet of Shadows" 
> It should be a neck item with +2 INT, +10 mana, and a dark aura.
> Include lore-rich descriptions.
```

## Testing Changes
```bash
# 1. Make backup FIRST
backup-zone 125

# 2. Edit with Aider (no git!)
aider --no-git lib/world/wld/125.wld
> Create 5 connected rooms for a crypt entrance...

# 3. Check syntax
./bin/circle -c -q

# 4. Boot test server
./bin/circle -q 4001 &

# 5. Connect and test
telnet localhost 4001
# goto 12500
# look
# stat room

# 6. If changes are good, they'll load on next reboot
# If bad, restore backup:
# cp lib/world/backups/$(date +%Y%m%d)/125.wld.bak lib/world/wld/125.wld
```

## Important Reminders

### DO's:
- ✅ ALWAYS backup before editing
- ✅ Use --no-git flag for world files
- ✅ Test on development port first
- ✅ Maintain zone theme consistency
- ✅ Document special features in comments
- ✅ Follow existing format patterns exactly

### DON'Ts:
- ❌ NEVER commit world files to git
- ❌ Never change existing vnums
- ❌ Don't break file format standards
- ❌ Avoid modern language in fantasy zones
- ❌ Don't create overpowered items
- ❌ Never edit in lib/plrfiles/ or player data
- ❌ Don't forget to backup!

## Quick Reference for File Formats

### Room File (.wld)
```
#VNUM
Name~
Description
~
zone_num room_flags sector_type
[Exits]
[Extra descriptions]
S
```

### Mobile File (.mob)
```
#VNUM
keywords~
short_description~
long_description~
detailed_description
~
[Stats and flags]
```

### Object File (.obj)
```
#VNUM  
keywords~
short_description~
long_description~
action_description~
[Type, effects, values, etc.]
```

## Emergency Recovery
```bash
# If you mess up a file badly:
# 1. Check today's backup
ls -la lib/world/backups/$(date +%Y%m%d)/

# 2. Restore the file
cp lib/world/backups/$(date +%Y%m%d)/30.wld.bak lib/world/wld/30.wld

# 3. Or restore from last night's full backup
tar -xzf /backup/world-backup-lastnight.tar.gz lib/world/wld/30.wld
```

---
*Remember: World files are LIVE DATA. They're not in git. Always backup before editing. Changes take effect on reboot/copyover. Use --no-git flag with aider!*