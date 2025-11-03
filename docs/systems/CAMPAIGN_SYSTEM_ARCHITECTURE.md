# Campaign System Architecture

## Overview
LuminariMUD implements a sophisticated campaign system that allows a single codebase to support multiple MUD themes through compile-time configuration flags. This enables administrators to run completely different MUD experiences (DragonLance, Forgotten Realms, or default LuminariMUD) from the same source code.

## Campaign Configuration

### Campaign Flags
The system uses three primary compile-time flags:
- **CAMPAIGN_DL**: DragonLance campaign settings
- **CAMPAIGN_FR**: Forgotten Realms campaign settings  
- **Default**: LuminariMUD (when no campaign flags are defined)

### Configuration Files
```bash
# Template files (in version control)
src/campaign.example.h      # Campaign flag definitions
src/mud_options.example.h   # MUD feature options
src/vnums.example.h         # Virtual number assignments

# Local files (copied from templates, not in version control)
src/campaign.h              # Active campaign configuration
src/mud_options.h           # Local MUD options
src/vnums.h                 # Local virtual number assignments
```

### Setup Process
```bash
# Initial setup
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h

# Edit campaign.h to uncomment desired campaign
# #define CAMPAIGN_DL     // For DragonLance
# #define CAMPAIGN_FR     // For Forgotten Realms
# Default: leave both commented for LuminariMUD
```

## MUD Identity and Branding

### Dynamic MUD Names (protocol.h)
```c
#if defined(CAMPAIGN_DL)
   #define MUD_NAME "Chronicles of Krynn"
#elif defined(CAMPAIGN_FR)
   #define MUD_NAME "Faerun: A Forgotten Realms MUD"
#else
   #define MUD_NAME "LuminariMUD"
#endif
```

### Impact Areas
- **Client Protocol**: MUD name appears in MSDP/GMCP/MSSP data
- **Login Screen**: Welcome messages and branding
- **Help Files**: Context-appropriate documentation
- **World Building**: Campaign-specific content and lore

## Feature Differentiation by Campaign

### Command System (interpreter.c)
The interpreter shows extensive campaign-specific command registration:

**DragonLance Specific Commands:**
- Lines 288, 490, 650, 718, 749, 875, 1363, 3322, 3341
- Special inventory and equipment management
- DragonLance-specific administrative commands

**Forgotten Realms Specific Commands:**
- Lines 125, 2218, 3004, 3058, 3085, 3112, 3219, 3251, 4348
- FR-specific spellcasting and deity systems
- Forgotten Realms cultural commands

**Shared Campaign Commands:**
- Lines 305, 327, 700, 783, 1350, 4508
- Common fantasy elements used by both settings

### World Content Systems

#### Bulletin Boards (boards.c)
```c
#if defined(CAMPAIGN_DL)
  // DragonLance specific board configurations
  // Different message boards for DL themes
#endif
```

#### Deity Systems (deities.h)
Campaign-specific pantheons and religious mechanics:
- **CAMPAIGN_DL**: Krynn pantheon (Paladine, Takhisis, etc.)
- **CAMPAIGN_FR**: Faerunian pantheon (Mystra, Shar, etc.)
- **Default**: Generic fantasy deities

#### Quest Systems (quest.c)
```c
#if defined(CAMPAIGN_DL)
  // DragonLance specific quest mechanics
  // Campaign-appropriate rewards and progression
#endif
```

### Crafting and Economy

#### Crafting System (craft.c)
Campaign-specific crafting variations:
- Lines 2543, 2570, 2651, 2698: DragonLance crafting specializations
- Different material availability and crafting recipes
- Campaign-appropriate item creation and enhancement

#### Economic Systems
- **Shop Systems (shop.c)**: Campaign-specific vendor behavior
- **Treasure Systems (treasure.c)**: Themed loot tables and rewards
- **Currency**: Different monetary systems per campaign

### Character Systems

#### Class Mechanics (class.c)
```c
#if defined(CAMPAIGN_DL) || defined(CAMPAIGN_FR)
  // Shared campaign class features
#endif

#ifdef CAMPAIGN_FR
  // Forgotten Realms specific class mechanics
#endif
```

#### Spell Systems (spell_parser.c)
Extensive spell differentiation (lines 1677-4478):
- **Spell Availability**: Different spell lists per campaign
- **Spell Circles**: Campaign-specific spell level organization
- **Casting Mechanics**: Themed spellcasting systems

### World Building Integration

#### Zone Editing (zedit.c)
```c
#if defined(CAMPAIGN_DL) || defined(CAMPAIGN_FR)
  // Campaign-specific zone building tools
  // Different area templates and guidelines
#endif
```

#### Wilderness Systems (wilderness.c)
```c
#ifdef CAMPAIGN_FR
  // Forgotten Realms wilderness generation
  // FR-specific terrain and encounter tables
#endif
```

## Database Integration

### Campaign-Specific Schemas
The system maintains different database schemas for campaign-specific features:

#### Pet System Example
From changelog documentation:
- **DragonLance**: Extended pet schema with pet_sdesc, pet_ldesc, pet_ddesc fields
- **Default/FR**: Simplified pet schema
- **Solution**: Unified to use DragonLance structure for all campaigns

#### Character Storage
Campaign flags affect how character data is stored and validated:
- Different skill/spell availability per campaign
- Campaign-specific attributes and progression tracking
- Themed character customization options

## Administrative Systems

### Wizard Commands (act.wizard.c)
Campaign-specific administrative tools:
- Line 180: Shared FR/DL administrative functions
- Line 1525: FR-specific wizard commands
- Lines 4657, 8269: DL-specific administrative tools

### World Building Tools
- **Area Creation**: Campaign-appropriate templates and guidelines
- **Object Creation**: Themed item generation and validation
- **Mobile Creation**: Campaign-specific NPC behavior and lore

## Special Procedures and NPCs

### Special Assignments (spec_assign.c)
```c
#ifdef CAMPAIGN_FR
  // Forgotten Realms specific NPC behaviors
  // FR-themed special procedures
#endif
```

Campaign-specific NPC behaviors and special room/object procedures ensure thematic consistency.

## Protocol and Network Integration

### MSDP/GMCP Variables (protocol.c)
```c
#if defined(CAMPAIGN_DL)
  // DragonLance specific client variables
  // Campaign-appropriate GUI elements
#endif
```

Client protocol integration includes:
- Campaign-specific variable reporting
- Themed GUI configurations
- Context-appropriate client enhancements

## Mob Behavior Systems

### AI and Mob Actions (mobact.c)
```c
#if defined(CAMPAIGN_DL)
  // DragonLance specific mob AI
  // Campaign-themed NPC behavior patterns
#endif
```

Campaign affects NPC artificial intelligence and behavior patterns to maintain thematic consistency.

## Character Limits and Restrictions (limits.c)

### Campaign-Specific Rules
```c
#if defined(CAMPAIGN_DL)
  // DragonLance specific character limits
#endif

#if defined(CAMPAIGN_FR) || defined(CAMPAIGN_DL)
  // Shared campaign restrictions
#endif
```

Different campaigns enforce different character creation rules, progression limits, and gameplay restrictions.

## Communication and Database (comm.c, db.c)

### Campaign-Aware Networking
```c
#if defined(CAMPAIGN_DL)
  // DragonLance specific communication protocols
#endif
```

### Database Loading
```c
#ifdef CAMPAIGN_FR
  // Forgotten Realms specific data loading
#endif

#if defined(CAMPAIGN_FR) || defined(CAMPAIGN_DL)
  // Shared campaign database initialization
#endif
```

## Integration with Resource Gathering System

### Phase 5 Campaign Considerations
When implementing the wilderness resource gathering system, campaign differentiation should be considered:

#### Resource Types
- **CAMPAIGN_DL**: DragonLance-appropriate materials (dragonmetal, irontree wood)
- **CAMPAIGN_FR**: Forgotten Realms materials (mithril, darkwood) 
- **Default**: Generic fantasy materials (steel, oak)

#### Gathering Mechanics
- Campaign-specific skill requirements
- Themed environmental interactions
- Setting-appropriate tool requirements

#### Integration Points
```c
// In wilderness resource code
#if defined(CAMPAIGN_DL)
  // DragonLance specific resource nodes
#elif defined(CAMPAIGN_FR)
  // Forgotten Realms specific resource types
#else
  // Default LuminariMUD resources
#endif
```

## Technical Implementation Details

### Conditional Compilation Pattern
```c
#if defined(CAMPAIGN_DL)
  // DragonLance specific code
#elif defined(CAMPAIGN_FR)
  // Forgotten Realms specific code
#else
  // Default LuminariMUD code
#endif

// For shared campaign features
#if defined(CAMPAIGN_FR) || defined(CAMPAIGN_DL)
  // Code common to both themed campaigns
#endif
```

### File Organization
- **Core Systems**: Always compiled, with internal campaign checks
- **Campaign Content**: Conditionally compiled based on flags
- **Shared Features**: Common code with campaign-specific variations

### Build System Integration
The campaign system integrates with the build process:
- CMake and Makefile respect campaign flags
- Conditional compilation reduces binary size
- Campaign-specific assets and data files

## Maintenance and Development

### Adding New Campaigns
To add a new campaign theme:

1. **Define Campaign Flag**: Add `CAMPAIGN_NEW` to campaign.example.h
2. **Update Protocol**: Add MUD name to protocol.h
3. **Content Creation**: Add conditional blocks throughout codebase
4. **Database Schema**: Create campaign-specific tables/fields as needed
5. **Testing**: Verify all combinations compile and run correctly

### Code Standards
- Always use `#if defined(CAMPAIGN_X)` for clarity
- Document campaign-specific features in comments
- Test all campaign combinations before release
- Keep default LuminariMUD as fallback for undefined behavior

### Migration Considerations
- Database migrations must handle all campaign variants
- Configuration changes should update all template files
- Documentation must reflect campaign-specific differences

## Current Status

### Implemented Campaigns
- ✅ **Default LuminariMUD**: Complete base implementation
- ✅ **CAMPAIGN_DL**: DragonLance theme with extensive content
- ✅ **CAMPAIGN_FR**: Forgotten Realms integration

### System Coverage
Campaign system affects **50+ source files** across all major game systems:
- ✅ Command interpretation and registration
- ✅ World building and content creation
- ✅ Character systems and progression
- ✅ Spell and magic systems
- ✅ Economic and crafting systems
- ✅ Administrative and maintenance tools
- ✅ Client protocol and networking
- ✅ Database integration and storage

### Future Enhancements
- **Runtime Campaign Switching**: Currently requires recompilation
- **Campaign-Specific Modules**: Plugin system for campaign content
- **Enhanced Configuration**: More granular campaign feature control
- **Documentation**: Campaign-specific help systems and lore integration

---

This campaign system represents a sophisticated solution for supporting multiple MUD themes from a single codebase, allowing administrators to create distinctly different player experiences while maintaining code consistency and reducing maintenance overhead.
