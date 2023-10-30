/**
 * @file handler.h                                      LuminariMUD
 * Prototypes of handling and utility functions.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
 */
#ifndef _HANDLER_H_
#define _HANDLER_H_

void check_room_lighting(room_rnum room, struct char_data *ch, bool enter);

/* handling the affected-structures */
int affect_total_sub(struct char_data *ch);
void affect_total_plus(struct char_data *ch, int at_armor);
void affect_total(struct char_data *ch);
void affect_to_char(struct char_data *ch, struct affected_type *af);
void affect_remove(struct char_data *ch, struct affected_type *af);
void affect_from_char(struct char_data *ch, int type);
void affect_type_from_char(struct char_data *ch, int type);
bool affected_by_spell(struct char_data *ch, int type);
void affect_join(struct char_data *ch, struct affected_type *af,
                 bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void affect_modify_ar(struct char_data *ch, byte loc, sh_int mod, int bitv[],
                      bool add);
void change_spell_mod(struct char_data *ch, int spellnum, int location, int amount, bool display);
void reset_char_points(struct char_data *ch);
void compute_char_cap(struct char_data *ch, int mode);

/* MSDP */
void update_msdp_affects(struct char_data *ch);
int get_char_affect_modifier(struct char_data *ch, int spellnum, int location);

// riding
void dismount_char(struct char_data *ch);
void mount_char(struct char_data *ch, struct char_data *mount);

/* utility */
void cleanup_disguise(struct char_data *ch);
const char *money_desc(int amount);
struct obj_data *create_money(int amount);
int isname(const char *str, const char *namelist);
int is_name(const char *str, const char *namelist);
const char *fname(const char *namelist);
int get_number(char **name);
int isname_obj(char *search, char *list); /* this is from spells.c */

/* objects */
void obj_to_char(struct obj_data *object, struct char_data *ch);
void obj_from_char(struct obj_data *object);
void obj_from_inv_to_bag(struct char_data *ch, struct obj_data *object, int bagnum);
void obj_from_bag(struct char_data *ch, struct obj_data *object, int bagnum);
void obj_to_bag(struct char_data *ch, struct obj_data *object, int bagnum);
void empty_bags_to_inventory(struct char_data *ch);

    void equip_char(struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(struct char_data *ch, int pos);
int invalid_align(struct char_data *ch, struct obj_data *obj);
int invalid_prof(struct char_data *ch, struct obj_data *obj);
int apply_ac(struct char_data *ch, int eq_pos);

void obj_to_room(struct obj_data *object, room_rnum room);
void obj_from_room(struct obj_data *object);
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
void obj_from_obj(struct obj_data *obj);
void object_list_new_owner(struct obj_data *list, struct char_data *ch);

void extract_obj(struct obj_data *obj);

void update_char_objects(struct char_data *ch);

/* characters*/
struct char_data *get_char_room(char *name, int *num, room_rnum room);
struct char_data *get_char_num(mob_rnum nr);

void char_from_room(struct char_data *ch);
void char_to_room(struct char_data *ch, room_rnum room);
void char_to_coords(struct char_data *ch, int x, int y, int wilderness);
void extract_char(struct char_data *ch);
void extract_char_final(struct char_data *ch);
void extract_pending_chars(void);
void char_from_buff_targets(struct char_data *ch);

/* find if character can see */
struct char_data *get_player_vis(struct char_data *ch, char *name, int *number, int inroom);
struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where);
struct char_data *get_char_room_vis(struct char_data *ch, char *name, int *number);
struct char_data *get_char_world_vis(struct char_data *ch, char *name, int *number);

struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj_num(obj_rnum nr);
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, int *number, struct obj_data *list);
struct obj_data *get_obj_vis(struct char_data *ch, char *name, int *num);
struct obj_data *get_obj_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[]);
int get_obj_pos_in_equip_vis(struct char_data *ch, char *arg, int *num, struct obj_data *equipment[]);

/* find all dots */
int find_all_dots(char *arg);

#define FIND_INDIV 0
#define FIND_ALL 1
#define FIND_ALLDOT 2

/* group */
struct group_data *create_group(struct char_data *leader);
void free_group(struct group_data *group);
void leave_group(struct char_data *ch);
void join_group(struct char_data *ch, struct group_data *group);

/* Generic Find */
int generic_find(const char *arg, bitvector_t bitvector, struct char_data *ch,
                 struct char_data **tar_ch, struct obj_data **tar_obj);

#define FIND_CHAR_ROOM (1 << 0)
#define FIND_CHAR_WORLD (1 << 1)
#define FIND_OBJ_INV (1 << 2)
#define FIND_OBJ_ROOM (1 << 3)
#define FIND_OBJ_WORLD (1 << 4)
#define FIND_OBJ_EQUIP (1 << 5)

/* prototypes from mobact.c */
void forget(struct char_data *ch, struct char_data *victim);
void remember(struct char_data *ch, struct char_data *victim);
void mobile_activity(void);
void mobile_echos(struct char_data *ch);
void clearMemory(struct char_data *ch);

/* For new last command: */
#define LAST_FILE LIB_ETC "last"

#define LAST_CONNECT 0
#define LAST_ENTER_GAME 1
#define LAST_RECONNECT 2
#define LAST_TAKEOVER 3
#define LAST_QUIT 4
#define LAST_IDLEOUT 5
#define LAST_DISCONNECT 6
#define LAST_SHUTDOWN 7
#define LAST_REBOOT 8
#define LAST_CRASH 9
#define LAST_PLAYING 10

struct last_entry
{
        int close_type;
        char hostname[MEDIUM_STRING];
        char username[16];
        time_t time;
        time_t close_time;
        int idnum;
        int punique;
};

void add_llog_entry(struct char_data *ch, int type);
struct last_entry *find_llog_entry(int punique, long idnum);
bool has_affect_modifier_type(struct char_data *ch, int location);
void save_chars(void);

#endif /* _HANDLER_H_ */
