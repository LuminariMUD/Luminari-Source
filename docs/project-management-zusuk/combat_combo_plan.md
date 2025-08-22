# Combat Combo Attack System - Implementation Plan

## Overview
This document provides a comprehensive implementation plan for adding a Combo Attack System to LuminariMUD. The system tracks the last 3 combat actions performed by each player and recognizes specific sequences that trigger powerful combo attacks with enhanced damage and special effects.

## Requirements Analysis

### Core Requirements
1. Track the last 3 combat actions performed by each player
2. Recognize specific sequences (e.g., kick->bash->strike = "Stunning Combo" with 2x damage + 1 round stun)
3. Store combo definitions in MySQL database with new table combo_attacks
4. Add command "combo list" to show available combos based on class/level
5. Integrate with existing combat event system without breaking current mechanics
6. Account for interruptions (getting stunned/paralyzed resets combo chain)
7. Add appropriate skill checks and cooldowns to prevent spam
8. Make it configurable per campaign (disabled in CAMPAIGN_DL by default)
9. Include proper memory management for action history buffers
10. Add GMCP support to notify clients of combo progress

### Technical Constraints
- Must use existing LuminariMUD patterns and conventions
- Must compile with ANSI C90/C89 (no C99 features)
- Must integrate with existing MySQL connection pool system
- Must follow existing memory management patterns (CREATE/FREE macros)
- Must respect campaign system architecture
- Must not break existing combat mechanics

## Database Schema Design

### Table: combo_attacks
```sql
CREATE TABLE IF NOT EXISTS combo_attacks (
    combo_id INT AUTO_INCREMENT PRIMARY KEY,
    combo_name VARCHAR(50) NOT NULL,
    combo_description TEXT,
    action_sequence VARCHAR(200) NOT NULL,     -- e.g., 'kick,bash,strike'
    required_class INT DEFAULT -1,             -- -1 = any class
    min_level INT DEFAULT 1,
    max_level INT DEFAULT 50,
    damage_multiplier DECIMAL(3,2) DEFAULT 1.0,
    special_effects VARCHAR(100),              -- e.g., 'stun:1'
    skill_check_type INT DEFAULT 0,            -- 0=none, 1=acrobatics, etc
    skill_check_dc INT DEFAULT 15,
    cooldown_seconds INT DEFAULT 30,
    campaign_flags INT DEFAULT 0,              -- Bit flags for campaigns
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_class_level (required_class, min_level),
    INDEX idx_sequence (action_sequence),
    INDEX idx_campaign (campaign_flags)
);
```

### Table: combo_cooldowns
```sql
CREATE TABLE IF NOT EXISTS combo_cooldowns (
    player_id BIGINT NOT NULL,
    combo_id INT NOT NULL,
    last_used TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (player_id, combo_id),
    INDEX idx_player_time (player_id, last_used)
);
```

### Integration Location
Add table creation to `src/db_init.c` around line 100, following existing pattern:
```c
if (mysql_query_safe(conn, create_combo_attacks)) {
    log("SYSERR: Failed to create combo_attacks table: %s", mysql_error(conn));
    return;
}
```

## Data Structure Design

### Core Structures (add to structs.h around line 5630)

```c
/* Combo Attack System Constants */
#define MAX_COMBO_ACTIONS 5
#define MAX_COMBO_NAME_LENGTH 50
#define MAX_COMBO_EFFECTS_LENGTH 100

/* Combat action types for combo tracking */
#define COMBO_ACTION_NONE     0
#define COMBO_ACTION_KICK     1
#define COMBO_ACTION_BASH     2
#define COMBO_ACTION_STRIKE   3
#define COMBO_ACTION_PUNCH    4
#define COMBO_ACTION_SWEEP    5
#define COMBO_ACTION_THRUST   6
#define COMBO_ACTION_SLASH    7
#define COMBO_ACTION_PARRY    8
#define COMBO_ACTION_RIPOSTE  9
#define MAX_COMBO_ACTIONS_TYPES 10

/* Campaign flags for combo availability */
#define COMBO_CAMPAIGN_LUMINARI  (1 << 0)
#define COMBO_CAMPAIGN_DL        (1 << 1)
#define COMBO_CAMPAIGN_FR        (1 << 2)
#define COMBO_CAMPAIGN_ALL       (COMBO_CAMPAIGN_LUMINARI | COMBO_CAMPAIGN_DL | COMBO_CAMPAIGN_FR)

/* Individual combo action record */
struct combo_action_record {
    int action_type;              /* COMBO_ACTION_* */
    time_t timestamp;             /* When this action occurred */
    struct char_data *target;     /* Who was targeted */
};

/* Combo attack definition */
struct combo_attack_def {
    int combo_id;                                    /* Database ID */
    char name[MAX_COMBO_NAME_LENGTH + 1];           /* "Stunning Combo" */
    char *description;                               /* Full description */
    int action_sequence[MAX_COMBO_ACTIONS];         /* Required action sequence */
    int sequence_length;                             /* Number of actions in sequence */
    int required_class;                              /* CLASS_* or -1 for any */
    int min_level;                                   /* Minimum level required */
    int max_level;                                   /* Maximum level allowed */
    float damage_multiplier;                         /* Damage multiplier */
    int special_effects;                             /* Bit flags for effects */
    int skill_check_type;                            /* SKILL_* for check */
    int skill_check_dc;                              /* DC for skill check */
    int cooldown_seconds;                            /* Cooldown in seconds */
    int campaign_flags;                              /* Which campaigns allow this */
    bool is_active;                                  /* Is this combo enabled */
    struct combo_attack_def *next;                   /* Linked list */
};

/* Player combo tracking data */
struct combo_tracking_data {
    struct combo_action_record recent_actions[MAX_COMBO_ACTIONS];
    int action_count;                                /* Number of recent actions */
    time_t last_action_time;                         /* Timestamp of last action */
    time_t last_combo_attempt;                       /* Last combo attempt time */
    int combo_progress_id;                           /* ID of combo being built */
    int combo_progress_step;                         /* Current step in combo */
};
```

### Character Data Integration
Add to char_data structure in structs.h (around line 5633):
```c
struct combo_tracking_data *combo_data;  /* Combo attack tracking */
```

## File Structure

### New Files Required
1. `src/combo_attacks.h` - Header file with function prototypes and macros
2. `src/combo_attacks.c` - Main implementation file
3. `docs/systems/COMBO_ATTACK_SYSTEM.md` - System documentation

### Modified Files
1. `src/structs.h` - Add data structures
2. `src/db_init.c` - Add table creation
3. `src/fight.c` - Add action recording integration points
4. `src/handler.c` - Add memory management calls
5. `src/interpreter.c` - Add combo command
6. `src/Makefile.am` - Add new source files
7. `CMakeLists.txt` - Add new source files

## Integration Points

### Combat System Integration (fight.c)
**Location**: In `hit()` function around line 6977
```c
/* Record combat action for combo system */
#include "combo_attacks.h"

/* After successful hit determination */
if (type == TYPE_HIT) {
    COMBO_RECORD_ACTION(ch, COMBO_ACTION_STRIKE, victim);
} else if (type == SKILL_KICK) {
    COMBO_RECORD_ACTION(ch, COMBO_ACTION_KICK, victim);
} else if (type == SKILL_BASH) {
    COMBO_RECORD_ACTION(ch, COMBO_ACTION_BASH, victim);
}
```

**Location**: In `damage()` function around line 5200
```c
/* Check for combo interruption on stun/paralysis */
if (AFF_FLAGGED(victim, AFF_PARALYZED) || AFF_FLAGGED(victim, AFF_STUNNED)) {
    COMBO_CHECK_INTERRUPTION(victim);
}
```

### Character Management Integration (db.c, handler.c)
**Location**: In `create_char()` function in db.c around line 4376
```c
#include "combo_attacks.h"
/* After other initialization */
init_player_combo_data(ch);
```

**Location**: In `extract_char()` function in handler.c around line 2663
```c
#include "combo_attacks.h"
/* Before free_char(ch) call */
cleanup_player_combo_data(ch);
```

### Command System Integration (interpreter.c)
**Location**: In command table around line 500
```c
{"combo", POS_FIGHTING, do_combo, 0, 0},
```

## Memory Management Strategy

### Allocation Pattern
Following LuminariMUD's existing patterns:
```c
/* Character combo data allocation */
CREATE(ch->combo_data, struct combo_tracking_data, 1);

/* Combo definition allocation */
CREATE(combo, struct combo_attack_def, 1);
combo->description = row[2] ? strdup(row[2]) : NULL;
```

### Cleanup Pattern
```c
/* Character cleanup */
void cleanup_player_combo_data(struct char_data *ch) {
    if (ch->combo_data) {
        free(ch->combo_data);
        ch->combo_data = NULL;
    }
}

/* System cleanup */
void cleanup_combo_system(void) {
    struct combo_attack_def *combo, *next_combo;
    for (combo = combo_list; combo; combo = next_combo) {
        next_combo = combo->next;
        if (combo->description) {
            free(combo->description);
        }
        free(combo);
    }
    combo_list = NULL;
}
```

### Safety Considerations
- Always check for NULL pointers before accessing combo_data
- Use two-pass cleanup for linked lists to prevent corruption
- Initialize all structure members to safe defaults
- Handle MySQL connection failures gracefully

## Campaign System Integration

### Conditional Compilation
```c
void init_combo_system(void) {
    log("Initializing Combo Attack System...");

#if defined(CAMPAIGN_DL)
    combo_system_enabled = FALSE;  /* Disabled for DragonLance by default */
    log("Combo system disabled for CAMPAIGN_DL");
    return;
#endif

    combo_list = NULL;
    load_combo_definitions_from_db();
}
```

### Runtime Campaign Checks
```c
bool is_combo_available_for_campaign(struct combo_attack_def *combo) {
#if defined(CAMPAIGN_DL)
    return (combo->campaign_flags & COMBO_CAMPAIGN_DL) != 0;
#elif defined(CAMPAIGN_FR)
    return (combo->campaign_flags & COMBO_CAMPAIGN_FR) != 0;
#else
    return (combo->campaign_flags & COMBO_CAMPAIGN_LUMINARI) != 0;
#endif
}
```

## GMCP Protocol Integration

### Message Types
Following existing MSDP/GMCP patterns in protocol.c:

1. **Combo.Progress** - Current action chain progress
2. **Combo.Available** - Combo ready for execution
3. **Combo.Executed** - Combo successfully executed

### Implementation Pattern
```c
void send_combo_progress_gmcp(struct char_data *ch) {
    char gmcp_data[512];

    if (!ch->desc || !ch->desc->pProtocol || !ch->desc->pProtocol->bGMCP) {
        return;
    }

    /* Build JSON data */
    snprintf(gmcp_data, sizeof(gmcp_data),
        "{ \"actions\": [%s], \"count\": %d }",
        action_list_json, data->action_count);

    MSDPSendPair(ch->desc, "Combo.Progress", gmcp_data);
}
```

## Database Integration Strategy

### Connection Management
Use existing MySQL pool system from mysql.c:
```c
if (mysql_pool_query(query, &result) != 0) {
    log("SYSERR: Failed to load combo definitions: %s", mysql_error(conn));
    return;
}
```

### Prepared Statements for Cooldowns
```c
PREPARED_STMT *pstmt = mysql_stmt_create(conn);
mysql_stmt_prepare_query(pstmt,
    "INSERT INTO combo_cooldowns (player_id, combo_id, last_used) "
    "VALUES (?, ?, NOW()) ON DUPLICATE KEY UPDATE last_used = NOW()");
mysql_stmt_bind_param_long(pstmt, 0, GET_IDNUM(ch));
mysql_stmt_bind_param_int(pstmt, 1, combo_id);
mysql_stmt_execute_prepared(pstmt);
mysql_stmt_cleanup(pstmt);
```

### Error Handling
```c
if (!mysql_available) {
    log("MySQL not available, skipping combo definitions load");
    return;
}
```

## Event System Integration

### Cooldown Management
Use existing mud_event system for cooldowns:
```c
/* Set combo cooldown using event system */
attach_mud_event(new_mud_event(eCOMBO_COOLDOWN, ch, combo_id_str),
                 combo->cooldown_seconds * PASSES_PER_SEC);
```

### Interruption Handling
```c
/* Check for interruption conditions */
if (AFF_FLAGGED(ch, AFF_STUNNED) || AFF_FLAGGED(ch, AFF_PARALYZED) ||
    AFF_FLAGGED(ch, AFF_SLEEP) || AFF_FLAGGED(ch, AFF_CHARM)) {
    reset_combo_chain(ch);
}
```

## Command Implementation

### Command Structure
Following existing command patterns in act.*.c files:
```c
ACMD(do_combo) {
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    if (!combo_system_enabled) {
        send_to_char(ch, "The combo system is not available.\r\n");
        return;
    }

    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs cannot use combo attacks.\r\n");
        return;
    }

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Usage: combo list | combo <combo_name> <target>\r\n");
        return;
    }

    /* Handle subcommands */
    if (!strcasecmp(arg1, "list")) {
        show_available_combos(ch);
        return;
    }

    /* Execute combo attack */
    execute_combo_command(ch, arg1, arg2);
}
```

## Core Function Specifications

### Action Recording Function
```c
void record_combat_action(struct char_data *ch, int action_type, struct char_data *target) {
    struct combo_tracking_data *data;
    time_t now = time(NULL);
    int i;

    if (!ch->combo_data || action_type == COMBO_ACTION_NONE) {
        return;
    }

    data = ch->combo_data;

    /* Check for action timeout (reset if more than 10 seconds since last action) */
    if (now - data->last_action_time > 10) {
        reset_combo_chain(ch);
    }

    /* Shift existing actions down */
    for (i = MAX_COMBO_ACTIONS - 1; i > 0; i--) {
        data->recent_actions[i] = data->recent_actions[i-1];
    }

    /* Add new action at the front */
    data->recent_actions[0].action_type = action_type;
    data->recent_actions[0].timestamp = now;
    data->recent_actions[0].target = target;

    if (data->action_count < MAX_COMBO_ACTIONS) {
        data->action_count++;
    }

    data->last_action_time = now;

    /* Check for combo matches */
    struct combo_attack_def *combo = check_for_combo_match(ch);
    if (combo) {
        send_combo_available_gmcp(ch, combo);
        send_to_char(ch, "\tY[COMBO READY: %s]\tn\r\n", combo->name);
    } else {
        send_combo_progress_gmcp(ch);
    }
}
```

### Combo Recognition Function
```c
struct combo_attack_def *check_for_combo_match(struct char_data *ch) {
    struct combo_attack_def *combo;
    struct combo_tracking_data *data;
    int i;
    bool sequence_match;

    if (!ch->combo_data) {
        return NULL;
    }

    data = ch->combo_data;

    for (combo = combo_list; combo; combo = combo->next) {
        if (!can_use_combo(ch, combo)) {
            continue;
        }

        /* Check if we have enough actions recorded */
        if (data->action_count < combo->sequence_length) {
            continue;
        }

        /* Check sequence match (most recent actions first) */
        sequence_match = TRUE;
        for (i = 0; i < combo->sequence_length; i++) {
            if (data->recent_actions[i].action_type != combo->action_sequence[i]) {
                sequence_match = FALSE;
                break;
            }
        }

        if (sequence_match) {
            return combo;
        }
    }

    return NULL;
}
```

### Combo Execution Function
```c
bool execute_combo_attack(struct char_data *ch, struct char_data *victim, struct combo_attack_def *combo) {
    int base_damage, combo_damage, skill_roll;

    if (!can_use_combo(ch, combo)) {
        return FALSE;
    }

    /* Perform skill check if required */
    if (combo->skill_check_type > 0) {
        skill_roll = skill_roll(ch, combo->skill_check_type, 0);
        if (skill_roll < combo->skill_check_dc) {
            send_to_char(ch, "You fail to execute the %s combo!\r\n", combo->name);
            set_combo_cooldown(ch, combo->combo_id);
            reset_combo_chain(ch);
            return FALSE;
        }
    }

    /* Calculate damage */
    base_damage = dice(2, 8) + GET_DAMROLL(ch);  /* Base combo damage */
    combo_damage = calculate_combo_damage(ch, victim, base_damage, combo->damage_multiplier);

    /* Apply damage */
    damage(ch, victim, combo_damage, TYPE_UNDEFINED, DAM_FORCE, FALSE);

    /* Apply special effects */
    if (combo->special_effects > 0) {
        apply_combo_special_effects(ch, victim, combo->special_effects);
    }

    /* Messages */
    act("$n executes a devastating \tY$t\tn combo attack!",
        FALSE, ch, combo->name, victim, TO_NOTVICT);
    act("You execute a devastating \tY$t\tn combo attack on $N!",
        FALSE, ch, combo->name, victim, TO_CHAR);
    act("$n executes a devastating \tY$t\tn combo attack on you!",
        FALSE, ch, combo->name, victim, TO_VICT);

    /* Set cooldown and reset chain */
    set_combo_cooldown(ch, combo->combo_id);
    reset_combo_chain(ch);

    /* GMCP notification */
    send_combo_executed_gmcp(ch, combo);

    return TRUE;
}
```

## Testing Strategy

### Unit Testing
1. Test combo definition loading from database
2. Test action sequence recognition
3. Test cooldown management
4. Test campaign-specific availability
5. Test memory management (no leaks)

### Integration Testing
1. Test with existing combat system
2. Test interruption handling
3. Test GMCP message delivery
4. Test database connection failures
5. Test with different campaigns

### Performance Testing
1. Test with multiple players using combos simultaneously
2. Test memory usage over extended gameplay
3. Test database query performance
4. Test event system integration

## Sample Data

### Initial Combo Definitions
```sql
INSERT INTO combo_attacks (combo_name, combo_description, action_sequence,
    required_class, min_level, max_level, damage_multiplier, special_effects,
    skill_check_type, skill_check_dc, cooldown_seconds, campaign_flags) VALUES
('Stunning Combo', 'A devastating three-hit combination that leaves the target stunned',
    'kick,bash,strike', -1, 5, 50, 2.0, 1, 1, 15, 30, 7),
('Sweeping Strike', 'A low sweep followed by an overhead strike',
    'sweep,strike', 1, 3, 50, 1.5, 2, 1, 12, 20, 7),
('Riposte Chain', 'A defensive parry followed by a counter-attack',
    'parry,riposte', 1, 8, 50, 1.8, 0, 0, 0, 25, 7);
```

## Implementation Timeline

### Phase 1: Core Infrastructure (Week 1)
- Create database tables
- Implement basic data structures
- Add memory management functions
- Create header file with prototypes

### Phase 2: Combat Integration (Week 2)
- Add action recording to combat system
- Implement combo recognition logic
- Add interruption handling
- Test basic functionality

### Phase 3: Command and UI (Week 3)
- Implement combo command
- Add GMCP support
- Create help documentation
- Test user interface

### Phase 4: Advanced Features (Week 4)
- Add skill checks and cooldowns
- Implement special effects
- Add campaign-specific features
- Performance optimization

### Phase 5: Testing and Polish (Week 5)
- Comprehensive testing
- Bug fixes and optimization
- Documentation completion
- Final integration testing

## Risk Mitigation

### Memory Leaks
- Use existing CREATE/FREE patterns consistently
- Implement comprehensive cleanup functions
- Test with memory debugging tools

### Performance Impact
- Minimize database queries during combat
- Cache combo definitions in memory
- Use efficient data structures

### Campaign Compatibility
- Test all campaign combinations
- Use conditional compilation properly
- Maintain backward compatibility

### Database Failures
- Handle MySQL unavailability gracefully
- Provide fallback behavior
- Log errors appropriately

## Success Criteria

1. System compiles without warnings on all supported platforms
2. No memory leaks detected during extended testing
3. Combat performance impact < 5% overhead
4. All combo sequences recognized correctly
5. GMCP messages delivered properly to supporting clients
6. Campaign-specific behavior works as specified
7. Database integration handles all error conditions
8. Command interface is intuitive and responsive
9. System can be disabled/enabled per campaign
10. Integration with existing systems causes no regressions

## Header File Specification (combo_attacks.h)

### Required Function Prototypes
```c
/* Core system functions */
void init_combo_system(void);
void cleanup_combo_system(void);
void load_combo_definitions_from_db(void);
struct combo_attack_def *find_combo_by_id(int combo_id);
struct combo_attack_def *find_combo_by_name(const char *name);

/* Player combo tracking */
void init_player_combo_data(struct char_data *ch);
void cleanup_player_combo_data(struct char_data *ch);
void record_combat_action(struct char_data *ch, int action_type, struct char_data *target);
void reset_combo_chain(struct char_data *ch);
void check_combo_interruption(struct char_data *ch);

/* Combo recognition and execution */
struct combo_attack_def *check_for_combo_match(struct char_data *ch);
bool execute_combo_attack(struct char_data *ch, struct char_data *victim, struct combo_attack_def *combo);
bool can_use_combo(struct char_data *ch, struct combo_attack_def *combo);
bool is_combo_on_cooldown(struct char_data *ch, int combo_id);
void set_combo_cooldown(struct char_data *ch, int combo_id);

/* Combo effects */
void apply_combo_special_effects(struct char_data *ch, struct char_data *victim, int effects);
int calculate_combo_damage(struct char_data *ch, struct char_data *victim,
                          int base_damage, float multiplier);

/* Database functions */
void save_combo_cooldown_to_db(struct char_data *ch, int combo_id);
bool load_combo_cooldowns_from_db(struct char_data *ch);

/* GMCP support */
void send_combo_progress_gmcp(struct char_data *ch);
void send_combo_available_gmcp(struct char_data *ch, struct combo_attack_def *combo);
void send_combo_executed_gmcp(struct char_data *ch, struct combo_attack_def *combo);

/* Commands */
ACMD(do_combo);

/* Utility functions */
int action_name_to_type(const char *name);
const char *action_type_to_name(int type);
const char *combo_effect_to_string(int effect);
bool is_combo_available_for_campaign(struct combo_attack_def *combo);
```

### Integration Macros
```c
#define COMBO_RECORD_ACTION(ch, action, target) \
    do { \
        if (combo_system_enabled && !IS_NPC(ch) && (ch)->combo_data) { \
            record_combat_action((ch), (action), (target)); \
        } \
    } while(0)

#define COMBO_CHECK_INTERRUPTION(ch) \
    do { \
        if (combo_system_enabled && !IS_NPC(ch) && (ch)->combo_data) { \
            check_combo_interruption((ch)); \
        } \
    } while(0)
```

This comprehensive implementation plan provides exact specifications for integrating a Combo Attack System into LuminariMUD while maintaining compatibility with existing systems and following established coding patterns. The plan includes specific code examples, integration points, database schemas, and testing strategies that would allow for immediate implementation.