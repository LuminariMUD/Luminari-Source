# LuminariMUD Clan System - Comprehensive Documentation

## Overview

The clan system in LuminariMUD provides a comprehensive framework for player organizations, allowing groups of players to form persistent associations with shared resources, communication channels, hierarchical structures, and territorial control through zone claiming. Clans are persistent player groups with member rankings, shared treasuries, private communication channels, zone ownership capabilities, and inter-clan diplomacy features.

## Core System Architecture

### Implementation Files

- **`clan.c`** - Core clan functionality, commands, and operations (~3500 lines)
- **`clan.h`** - Data structures, constants, macros, and function declarations
- **`clan_edit.c`** - Online clan editing interface (OLC) for administrators and clan leaders

### Data Persistence

- **File Storage**: Clans are saved to `lib/etc/clans` in a custom text format
- **Player Storage**: Clan membership is stored in both player files and the player index
- **No MySQL Storage**: Unlike other systems, clans use file-based storage exclusively

## Data Structures

### Main Clan Structure
```c
struct clan_data {
    clan_vnum vnum;                  // Unique vnum of this clan (1-65535)
    char *clan_name;                 // The full name of this clan
    char *description;               // The clan's description/information
    long leader;                     // The ID of the clan leader
    ubyte ranks;                     // Number of clan ranks (max 15)
    char *rank_name[MAX_CLANRANKS];  // Customizable rank names
    ubyte privilege[NUM_CLAN_PRIVS]; // Minimum rank for each privilege
    int applev;                      // Min level to apply to clan
    int appfee;                      // Cost when application accepted
    int taxrate;                     // Tax rate for transactions
    int war_timer;                   // Ticks left for current war
    zone_vnum hall;                  // The zone for the clan's hall
    int at_war[MAX_CLANS];          // Array tracking war status
    int allies[MAX_CLANS];          // Array tracking alliance status
    int spells[MAX_CLANSPELLS];     // Five skills known by all members
    long treasure;                   // The clan's bank account
    int pk_win;                      // PK wins count
    int pk_lose;                     // PK losses count
    int raided;                      // Times been raided
    char *abrev;                     // Abbreviation (max 5 chars)
};
```

### Zone Claim Structure
```c
struct claim_data {
    zone_vnum zn;                    // Zone VNUM being claimed
    long claimant;                   // ID of player who claimed
    clan_vnum clan;                  // VNUM of controlling clan
    float popularity[MAX_CLANS];     // Popularity values per clan
    struct claim_data *next;         // Linked list pointer
};
```

## Constants and Limits

- **MAX_CLANS**: 25 (maximum number of clans allowed)
- **MAX_CLANRANKS**: 15 (maximum ranks per clan)
- **MAX_CLANSPELLS**: 5 (skills granted to clan members)
- **MAX_CLAN_NAME**: 60 (maximum clan name length)
- **MAX_CLAN_DESC**: 2048 (maximum description length)
- **MAX_CLAN_ABREV**: 5 (maximum abbreviation length)
- **NO_CLAN**: 65535 (indicates no clan membership)
- **NO_CLANRANK**: 0 (indicates no rank assigned)

## Clan Privileges System

The clan system uses a sophisticated privilege system with 21 different permissions:

### Standard Privileges (CP_* constants)
- **CP_AWARD** (0): Award clan points to members
- **CP_CLAIM** (2): Claim zones for the clan
- **CP_BALANCE** (1): Check clan bank balance
- **CP_DEMOTE** (3): Demote clan members
- **CP_DEPOSIT** (4): Deposit gold into clan bank
- **CP_CLANEDIT** (5): Access clan editing OLC
- **CP_ENROL** (6): Enroll applicants into clan
- **CP_EXPEL** (7): Kick members from clan
- **CP_OWNER** (8): Transfer clan ownership
- **CP_PROMOTE** (9): Promote clan members
- **CP_WHERE** (10): See locations of clan members
- **CP_WITHDRAW** (11): Withdraw from clan bank

### Clan Edit Privileges
- **CP_ALLIED** (12): Set allied clans
- **CP_APPFEE** (13): Set application fee
- **CP_APPLEV** (14): Set minimum application level
- **CP_DESC** (15): Set clan description
- **CP_TAXRATE** (16): Set clan tax rate
- **CP_RANKS** (17): Configure ranks and names
- **CP_TITLE** (18): Set clan name
- **CP_ATWAR** (19): Declare war on clans
- **CP_SETPRIVS** (20): Set privilege requirements

## Command System

### Player Commands

#### Basic Commands (Available to all clan members)
- **`clan`** - Display available clan commands
- **`clan apply <clan>`** - Apply to join a clan
- **`clan info [clan]`** - View clan information
- **`clan list`** - List all clans
- **`clan status`** - Show your clan status
- **`clan who`** - List your clan members
- **`clan leave`** - Leave current clan (requires confirmation code)

#### Communication
- **`clantalk <message>`** / **`ct <message>`** - Send message to clan channel

#### Officer Commands (Rank-based permissions)
- **`clan award <player> <points>`** - Award clan points (costs 10 gold per point)
- **`clan balance`** - Check clan bank balance
- **`clan claim`** - Claim current zone for clan
- **`clan demote <player>`** - Demote a member
- **`clan deposit <amount>`** - Deposit gold
- **`clan enrol [player]`** - List applicants or enroll player
- **`clan expel <player>`** - Remove member from clan
- **`clan promote <player>`** - Promote a member
- **`clan where`** - See online member locations
- **`clan withdraw <amount>`** - Withdraw gold

#### Leader Commands
- **`clan edit`** - Enter clan editing OLC
- **`clan owner <player>`** - Transfer leadership

### Immortal Commands
- **`clan create <player> <name>`** - Create new clan (LVL_IMPL)
- **`clan destroy <clan>`** - Remove clan (LVL_IMPL)
- **`clan unclaim <zone>`** - Remove zone claims (LVL_IMPL)
- **`clanset`** - Set player clan data (LVL_IMPL)

## Clan Ranks System

### Default Rank Structure
1. **Rank 1**: Leader (highest)
2. **Ranks 2-4**: Officer ranks
3. **Rank 5**: Member
4. **Ranks 6+**: Lower ranks (configurable)

### Rank Features
- Customizable rank names per clan
- Each privilege has a minimum rank requirement
- Leaders (rank 1) always have full access
- Rank 0 is reserved for "leader only" privileges

## Zone Claiming System

### Claiming Mechanics
- Clans can claim zones to establish territory
- Claims tracked in `lib/etc/claims` file
- Popularity system tracks clan influence per zone
- Zone ownership provides benefits to clan members

### Claim Data
- Zone VNUM being claimed
- Player ID who made the claim
- Controlling clan VNUM
- Popularity values for each clan (0-100)

### Diplomacy Integration
- Diplomacy skills increase zone popularity
- Regular popularity checks update control
- Contested zones can change ownership

## Integration Points

### Character Data Macros
```c
GET_CLAN(ch)       // Returns character's clan VNUM
GET_CLANRANK(ch)   // Returns character's rank (1=leader)
GET_CLANPOINTS(ch) // Returns character's clan points
IS_IN_CLAN(ch)     // Check if character is in a clan
```

### Player Table Integration
- Clan membership stored in player index
- Fast lookups for offline player clan data
- Synchronized with character saves

### Communication System
- Dedicated clan channel with formatting
- Channel restrictions based on preferences
- Integration with noclantalk toggle

## Special Features

### Clan Leave Protection
- Requires typing a 6-character confirmation code
- Prevents accidental clan departures
- Code regenerated each attempt

### Auto-Leadership Succession
- Automatic promotion of highest-ranking officer
- Activates when leader is removed/deleted
- Maintains clan continuity

### War and Alliance System
- Clans can declare war or form alliances
- Affects PvP interactions between members
- War timer limits duration of conflicts

### Clan Spells (Not Fully Implemented)
- Framework for 5 clan-specific skills
- Would grant abilities to all members
- Currently unused in gameplay

## File Formats

### Clan Save Format (lib/etc/clans)
```
#<vnum>
Name: <clan name>
Init: <abbreviation>
Desc:
<description text>~
Lder: <leader ID>
AppL: <min level>
AppF: <application fee>
Tax : <tax rate>
Hall: <hall zone>
Bank: <treasury amount>
Ally: <25 alliance flags>
War : <25 war flags>
Rank:
<rank names>
~
Priv:
<privilege> <min rank>
~
```

### Claim Save Format (lib/etc/claims)
```
#<zone vnum>
Claimant: <player ID>
Clan: <clan vnum>
Pop: <25 popularity values>
```

## Technical Implementation Details

### Memory Management
- Dynamic allocation for clan list
- String data uses strdup() for copies
- Proper cleanup on clan removal/shutdown

### Performance Considerations
- O(n) clan lookups (small n, max 25)
- Clan list kept in memory
- File I/O only on save/load

### Error Handling
- Extensive validation of clan data
- Graceful handling of missing clans
- Auto-correction of invalid ranks

## Recent Updates and Current State

### Active Features
- Full command system implementation
- Clan creation, management, and deletion
- Member ranking and permissions
- Clan treasury and banking
- Zone claiming and popularity
- Clan communication channel
- Alliance and war declarations
- Online clan editing (OLC)

### Limitations
- No MySQL integration (file-based only)
- Clan spells framework unused
- No automated clan events
- Limited inter-clan mechanics

### Known Issues
- War timer not actively used
- Raid counter incremented but not displayed
- Some privilege checks may be inverted

## Usage Examples

### Creating a Clan
```
> clan create Gandalf "Knights of Valor"
Adding clan 'Knights of Valor' (Leader: Gandalf) at VNUM 1
Clan added successfully.
```

### Joining a Clan
```
> clan apply Knights
You apply to join Knights of Valor.
Application fee is 1000 gold coins, paid upon acceptance.
```

### Clan Communication
```
> ct Hello clan members!
[CLAN] Gandalf: Hello clan members!
```

### Managing Members
```
> clan promote Frodo
Frodo has been promoted to Officer!

> clan demote Gollum  
Gollum has been demoted to Member!
```

## Best Practices

### For Players
- Choose meaningful clan names and descriptions
- Set appropriate application requirements
- Maintain active leadership succession
- Use clan banks for group resources
- Coordinate zone claiming efforts

### For Administrators
- Monitor clan activity and conflicts
- Enforce naming standards
- Resolve inter-clan disputes fairly
- Clean up inactive clans periodically
- Back up clan files regularly

## Future Enhancement Possibilities
- MySQL database integration
- Clan levels and experience
- Automated clan events/quests
- Clan-owned shops or resources
- Enhanced war mechanics
- Clan achievements/trophies
- Cross-clan tournaments
- Clan skill bonuses implementation

---

*This documentation reflects the current implementation as of the latest code audit. The clan system is mature and fully functional, providing robust player organization features for LuminariMUD.*