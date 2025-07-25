# LuminariMUD Player Management System

## Overview

LuminariMUD implements a comprehensive player management system that handles user authentication, character creation, data persistence, and account management. The system supports both traditional character-based login and a modern account-based system that allows multiple characters per account.

## Architecture Components

### 1. Account System

The account system provides a modern approach to player management:

```c
struct account_data {
    int id;                                      // Unique account ID
    char *name;                                  // Account name
    char password[MAX_PWD_LENGTH + 1];           // Encrypted password
    sbyte bad_password_count;                    // Failed login attempts
    char *character_names[MAX_CHARS_PER_ACCOUNT]; // Character list
    int experience;                              // Account experience points
    int classes[MAX_UNLOCKED_CLASSES];           // Unlocked classes
    int races[MAX_UNLOCKED_RACES];               // Unlocked races
    char *email;                                 // Email address
};
```

**Key Features:**
- **Multiple Characters:** Up to `MAX_CHARS_PER_ACCOUNT` characters per account
- **Unlockable Content:** Classes and races can be locked/unlocked
- **Account Experience:** Shared experience pool across characters
- **Security:** Password encryption and failed attempt tracking

### 2. Player Index System

The player index maintains a fast lookup table for all characters:

```c
struct player_index_element {
    char *name;                                  // Character name (lowercase)
    long id;                                     // Unique character ID
    int level;                                   // Character level
    int flags;                                   // Index flags
    time_t last;                                 // Last login time
};

// Global player index
extern struct player_index_element *player_table;
extern int top_of_p_table;
```

**Index Functions:**
- `get_ptable_by_name()` - Find character by name
- `create_entry()` - Add new character to index
- `save_player_index()` - Persist index to disk
- `load_player_index()` - Load index from disk

### 3. Connection States

The login process uses a state machine managed by the `nanny()` function:

```c
// Connection states (from structs.h)
#define CON_PLAYING         0    // Normal gameplay
#define CON_CLOSE           1    // Disconnect pending
#define CON_GET_NAME        2    // Getting character name
#define CON_NAME_CNFRM      3    // Confirming new name
#define CON_PASSWORD        4    // Getting password
#define CON_NEWPASSWD       5    // Creating new password
#define CON_CNFPASSWD       6    // Confirming new password
#define CON_QSEX            7    // Choosing gender
#define CON_QCLASS          8    // Choosing class
#define CON_RMOTD           9    // Reading MOTD
#define CON_MENU           10    // Main menu
#define CON_ACCOUNT_NAME   11    // Account name entry
#define CON_ACCOUNT_MENU   12    // Account character menu
// ... additional states for character creation
```

## Login Process Flow

### 1. Initial Connection

```c
// In comm.c - new_descriptor()
static int new_descriptor(socket_t s) {
    // Accept socket connection
    // Create descriptor_data structure
    // Initialize connection state
    // Send greeting message
    STATE(newd) = CON_ACCOUNT_NAME;
    return 0;
}
```

### 2. Account Authentication

```c
// In nanny() function - CON_ACCOUNT_NAME state
case CON_ACCOUNT_NAME:
    // Parse and validate account name
    if (load_account(tmp_name, d->account) > -1) {
        // Account exists - request password
        write_to_output(d, "Password: ");
        STATE(d) = CON_PASSWORD;
        ProtocolNoEcho(d, true);
    } else {
        // New account - create account
        STATE(d) = CON_ACCOUNT_CREATE;
    }
    break;
```

### 3. Password Verification

```c
// In nanny() function - CON_PASSWORD state
case CON_PASSWORD:
    if (strncmp(CRYPT(arg, d->account->name), d->account->password, MAX_PWD_LENGTH)) {
        // Password incorrect
        d->account->bad_password_count++;
        if (++(d->bad_pws) >= CONFIG_MAX_BAD_PWS) {
            STATE(d) = CON_CLOSE;
        }
    } else {
        // Password correct - show account menu
        STATE(d) = CON_ACCOUNT_MENU;
        show_account_menu(d);
    }
    break;
```

### 4. Character Selection/Creation

```c
// Account menu allows:
// - Select existing character
// - Create new character
// - Delete character
// - Account management

void show_account_menu(struct descriptor_data *d) {
    // Display character list
    // Show creation options
    // Account statistics
}
```

## Character Creation Process

### 1. Character Initialization

```c
// Create new character structure
CREATE(d->character, struct char_data, 1);
clear_char(d->character);
CREATE(d->character->player_specials, struct player_special_data, 1);
new_mobile_data(d->character);

// Link character to descriptor
d->character->desc = d;
GET_HOST(d->character) = strdup(d->host);
```

### 2. Character Customization States

The creation process guides players through:

1. **Name Selection** (`CON_GET_NAME`)
   - Name validation and uniqueness checking
   - Reserved word filtering
   - Length and character restrictions

2. **Gender Selection** (`CON_QSEX`)
   - Male, female, or neutral options

3. **Race Selection** (`CON_QRACE`)
   - Available races based on account unlocks
   - Racial ability modifiers and features

4. **Class Selection** (`CON_QCLASS`)
   - Available classes based on account unlocks
   - Class restrictions and requirements

5. **Ability Score Assignment** (`CON_QSTATS`)
   - Point-buy or rolling system
   - Racial modifiers applied

6. **Final Confirmation** (`CON_QCONFIRM`)
   - Review character details
   - Finalize creation

### 3. Character Finalization

```c
// Set starting values
init_char(d->character);
GET_LEVEL(d->character) = 1;
GET_EXP(d->character) = 1;

// Assign starting equipment and location
Crash_load(d->character);
char_to_room(d->character, load_room);

// Enter game
STATE(d) = CON_PLAYING;
```

## Data Persistence

### 1. Character Saving (`save_char()`)

The ASCII-based save system stores character data in structured text files:

```c
void save_char(struct char_data *ch, int mode) {
    FILE *fl;
    char filename[40];
    
    // Generate filename: lib/plrfiles/A-E/player.plr
    get_filename(filename, sizeof(filename), PLR_FILE, GET_NAME(ch));
    
    if (!(fl = fopen(filename, "w"))) {
        log("SYSERR: Couldn't open player file %s for write", filename);
        return;
    }
    
    // Save character data sections:
    // - Basic info (name, level, class, race)
    // - Abilities and statistics
    // - Equipment and inventory
    // - Spells and skills
    // - Preferences and settings
    // - Quest and achievement data
    
    fclose(fl);
    
    // Update player index
    save_player_index();
    
    // Save account data
    if (ch->desc && ch->desc->account) {
        save_account(ch->desc->account);
    }
}
```

**Save File Structure:**
```
Name: PlayerName
SexC: 1
Clas: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39
Race: 0
Levl: 1
Brth: 1234567890
Plyd: 3600
Last: 1234567890
Host: 192.168.1.1
Hite: 180
Wate: 75
Alin: 0
Id  : 12345
Act : 0 0 0 0
Aff : 0 0 0 0
Savs: 0 0 0 0 0
Abils: 16 14 15 12 13 10
```

### 2. Character Loading (`load_char()`)

```c
int load_char(char *name, struct char_data *ch) {
    FILE *fl;
    char filename[40], line[MAX_INPUT_LENGTH];
    char tag[6];
    
    get_filename(filename, sizeof(filename), PLR_FILE, name);
    
    if (!(fl = fopen(filename, "r"))) {
        return -1; // Character not found
    }
    
    // Parse save file line by line
    while (get_line(fl, line)) {
        tag_argument(line, tag);
        
        // Process each data section
        if (!strcmp(tag, "Name")) strcpy(GET_NAME(ch), line);
        else if (!strcmp(tag, "SexC")) GET_SEX(ch) = atoi(line);
        else if (!strcmp(tag, "Clas")) /* parse class data */;
        else if (!strcmp(tag, "Race")) GET_RACE(ch) = atoi(line);
        // ... continue for all data fields
    }
    
    fclose(fl);
    return 0;
}
```

### 3. Account Data Persistence

Account data is stored in MySQL database:

```c
int load_account(char *name, struct account_data *account) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char buf[2048];
    
    snprintf(buf, sizeof(buf), 
        "SELECT id, name, password, experience, email "
        "FROM account_data WHERE lower(name) = lower('%s')", name);
    
    if (mysql_query(conn, buf)) {
        log("SYSERR: Unable to SELECT from account_data: %s", mysql_error(conn));
        return -1;
    }
    
    result = mysql_store_result(conn);
    if (!(row = mysql_fetch_row(result))) {
        mysql_free_result(result);
        return -1; // Account not found
    }
    
    // Load account data from database row
    account->id = atoi(row[0]);
    account->name = strdup(row[1]);
    strncpy(account->password, row[2], MAX_PWD_LENGTH + 1);
    account->experience = atoi(row[3]);
    account->email = (row[4] ? strdup(row[4]) : NULL);
    
    mysql_free_result(result);
    
    // Load associated character list and unlocks
    load_account_characters(account);
    load_account_unlocks(account);
    
    return 0;
}

void save_account(struct account_data *account) {
    char buf[2048];
    
    snprintf(buf, sizeof(buf),
        "INSERT INTO account_data (id, name, password, experience, email) "
        "VALUES (%d, '%s', '%s', %d, %s%s%s) "
        "ON DUPLICATE KEY UPDATE "
        "password = VALUES(password), "
        "experience = VALUES(experience), "
        "email = VALUES(email)",
        account->id, account->name, account->password, account->experience,
        (account->email ? "'" : ""), 
        (account->email ? account->email : "NULL"),
        (account->email ? "'" : ""));
    
    if (mysql_query(conn, buf)) {
        log("SYSERR: Unable to save account data: %s", mysql_error(conn));
    }
}
```

## Security Features

### 1. Password Security

- **Encryption:** Passwords stored using `crypt()` function
- **Salt:** Account name used as salt for encryption
- **Attempt Limiting:** Failed login attempts tracked and limited
- **Timeout:** Idle connections automatically disconnected

### 2. Name Validation

```c
int valid_name(char *newname) {
    // Check length constraints
    if (strlen(newname) < 2 || strlen(newname) > MAX_NAME_LENGTH)
        return 0;
    
    // Check for valid characters (letters only)
    for (i = 0; newname[i]; i++) {
        if (!isalpha(newname[i]))
            return 0;
    }
    
    // Check against reserved words
    if (reserved_word(newname) || fill_word(newname))
        return 0;
    
    return 1;
}
```

### 3. Site Banning

- **IP-based banning:** Prevent connections from specific addresses
- **Site-specific restrictions:** Character-level site permissions
- **Administrative controls:** Staff can manage access restrictions

## Multi-Character Management

### 1. Character Switching

Players can switch between characters on the same account:

```c
// Account menu allows character selection
// Previous character saved automatically
// New character loaded and entered into game
```

### 2. Shared Resources

- **Account Experience:** Earned across all characters
- **Unlocked Content:** Classes and races available to all characters
- **Account Statistics:** Shared achievements and progress

### 3. Character Limits

- Maximum characters per account configurable
- Character deletion with confirmation
- Character recovery systems for accidental deletion

## Performance Optimizations

### 1. Player Index Caching

- In-memory index for fast character lookups
- Periodic saves to disk for persistence
- Efficient search algorithms for name resolution

### 2. Lazy Loading

- Character data loaded only when needed
- Account data cached during session
- Inventory and equipment loaded on demand

### 3. Database Connection Management

- Persistent MySQL connections
- Connection pooling for high load
- Automatic reconnection on failure

This player management system provides a robust foundation for user authentication, character persistence, and account management while maintaining security and performance standards appropriate for a MUD environment.
