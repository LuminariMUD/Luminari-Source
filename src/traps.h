/*
 * File:   traps.h
 * Author: Zusuk
 * Updated: Comprehensive trap system based on NWN mechanics
 *
 * Created on 2 נובמבר 2014, 22:39
 */

#ifndef TRAPS_H
#define TRAPS_H

/* ============================================================================ */
/* Trap System Data Tables                                                      */
/* ============================================================================ */

/* Trap severity data - Damage, DCs, etc by severity level */
struct trap_severity_data
{
    const char *name;
    int detect_dc_base;      /* Base DC to detect */
    int disarm_dc_base;      /* Base DC to disarm */
    int save_dc_base;        /* Base DC for saves */
    int damage_multiplier;   /* Multiplier for damage calculations */
};

/* Trap type template data - Describes each trap type */
struct trap_type_template
{
    const char *name;               /* Trap type name */
    int damage_type;                /* DAM_* type */
    int save_type;                  /* TRAP_SAVE_* type */
    int special_effect;             /* TRAP_SPECIAL_* effect */
    const char *trigger_msg_char;   /* Message to char when triggered */
    const char *trigger_msg_room;   /* Message to room when triggered */
    const char *detect_msg;         /* Message when trap is detected */
    bool is_area_effect;            /* Does this trap affect multiple targets? */
    int default_area_radius;        /* Default area radius for AoE traps */
};

/* ============================================================================ */
/* Function Prototypes                                                          */
/* ============================================================================ */

/* Trap creation and management */
struct trap_data *create_trap(int trap_type, int severity, int trigger_type);
void free_trap(struct trap_data *trap);
struct trap_data *copy_trap(struct trap_data *source);
void attach_trap_to_room(struct trap_data *trap, room_rnum room);
void attach_trap_to_object(struct trap_data *trap, struct obj_data *obj);
void remove_trap_from_room(struct trap_data *trap, room_rnum room);
void remove_trap_from_object(struct obj_data *obj);

/* Trap generation */
struct trap_data *generate_random_trap(int zone_level);
int determine_trap_severity(int zone_level);
int get_random_trap_type(void);
void auto_generate_zone_traps(zone_rnum zone);
void auto_generate_room_trap(room_rnum room, int zone_level);
void auto_generate_object_trap(struct obj_data *obj, int zone_level);

/* Trap detection and disarming */
bool detect_trap(struct char_data *ch, struct trap_data *trap);
bool disarm_trap(struct char_data *ch, struct trap_data *trap);
int get_trap_detect_dc(struct trap_data *trap, struct char_data *ch);
int get_trap_disarm_dc(struct trap_data *trap, struct char_data *ch);
struct trap_data *find_trap_in_room(struct char_data *ch, room_rnum room);
struct trap_data *find_trap_on_object(struct obj_data *obj);

/* Trap triggering */
bool check_trap_trigger(struct char_data *ch, int trigger_type, room_rnum room, 
                        struct obj_data *obj, int direction);
void trigger_trap(struct char_data *ch, struct trap_data *trap, room_rnum room);
void apply_trap_damage(struct char_data *ch, struct trap_data *trap);
void apply_trap_special_effect(struct char_data *ch, struct trap_data *trap);
void apply_trap_to_area(struct trap_data *trap, room_rnum room, struct char_data *triggerer);

/* Trap information and utility */
const char *get_trap_name(struct trap_data *trap);
const char *get_trap_severity_name(int severity);
const char *get_trap_type_name(int trap_type);
int get_trap_component_value(struct trap_data *trap);
void show_trap_info(struct char_data *ch, struct trap_data *trap);

/* Perk integration */
int get_trapfinding_bonus(struct char_data *ch);
int get_trap_sense_bonus(struct char_data *ch);
bool can_recover_trap_components(struct char_data *ch);
void recover_trap_components(struct char_data *ch, struct trap_data *trap);

/* Legacy system compatibility */
bool check_trap(struct char_data *ch, int trap_type, int room, struct obj_data *obj, int dir);
void set_off_trap(struct char_data *ch, struct obj_data *trap);
bool is_trap_detected(struct obj_data *trap);
void set_trap_detected(struct obj_data *trap);
int perform_detecttrap(struct char_data *ch, bool silent);

/* ACMD prototypes */
ACMD_DECL(do_disabletrap);
ACMD_DECL(do_detecttrap);
ACMD_DECL(do_trapinfo);  /* New command to show trap details */

/* Special mob vnums for trap effects */
#define TRAP_DARK_WARRIOR_MOBILE 135600
#define TRAP_SPIDER_MOBILE 180437

/* Global trap data tables (defined in traps.c) */
extern const struct trap_severity_data trap_severity_table[NUM_TRAP_SEVERITIES];
extern const struct trap_type_template trap_type_table[NUM_TRAP_TYPES];

#endif /* TRAPS_H */
