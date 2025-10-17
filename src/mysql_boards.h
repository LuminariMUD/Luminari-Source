/* ************************************************************************
*   File: mysql_boards.h                             Part of Age of Dragons *
*  Usage: MySQL-based bulletin board system header                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Age of Dragons MUD - MySQL Board System                               *
*  Written by: Gicker (William Rupert Stephen Squires)                   *
************************************************************************ */

#ifndef __MYSQL_BOARDS_H__
#define __MYSQL_BOARDS_H__

#include "mysql/mysql.h"

/* Board System Constants */
#define MAX_BOARD_TITLE_LENGTH    256
#define MAX_BOARD_BODY_LENGTH     8192
#define POSTS_PER_PAGE           20

/* Board Type Constants */
#define BOARD_TYPE_GENERAL        0
#define BOARD_TYPE_IMMORTAL       1
#define BOARD_TYPE_CLAN           2
#define BOARD_TYPE_QUEST          3
#define BOARD_TYPE_RP             4
#define BOARD_TYPE_ANNOUNCEMENT   5
#define BOARD_TYPE_NEWBIE         6

/* Board Configuration Structure */
struct mysql_board_config {
    int board_id;           /* Unique board ID */
    char *board_name;       /* Display name of the board */
    int board_type;         /* Board type (see above constants) */
    int read_level;         /* Minimum level to read posts */
    int write_level;        /* Minimum level to write posts */
    int delete_level;       /* Minimum level to delete posts */
    int obj_vnum;           /* Object VNUM that represents this board */
    int clan_id;            /* Clan ID for clan boards (0 = not clan-specific) */
    int clan_rank;          /* Minimum clan rank required (0 = any rank in clan) */
    bool active;            /* Is this board currently active? */
};

/* Board Post Structure */
struct mysql_board_post {
    int post_id;            /* Unique post ID */
    int board_id;           /* Board this post belongs to */
    char *title;            /* Post title */
    char *body;             /* Post content */
    char *author;           /* Author name */
    int author_id;          /* Author player ID */
    int author_level;       /* Author level when posted */
    time_t date_posted;     /* When the post was made */
    time_t date_modified;   /* When the post was last modified */
    bool deleted;           /* Soft delete flag */
};

/* Board Command Processing States */
#define BOARD_CMD_READ            0
#define BOARD_CMD_WRITE           1
#define BOARD_CMD_DELETE          2
#define BOARD_CMD_LIST            3
#define BOARD_CMD_HELP            4

/* Function Prototypes */

/* Core Board Functions */
int mysql_board_init(void);
void mysql_board_cleanup(void);
void mysql_board_load_configs(void);
void mysql_board_sync_default_boards(void);
struct mysql_board_config *mysql_board_get_config_by_obj(int obj_vnum);
struct mysql_board_config *mysql_board_get_config_by_object(struct obj_data *obj);
bool mysql_board_exists(int board_id);

/* Post Management Functions */
int mysql_board_create_post(struct char_data *ch, int board_id, char *title, char *body);
bool mysql_board_delete_post(struct char_data *ch, int board_id, int post_id);
struct mysql_board_post *mysql_board_get_post(int board_id, int post_id);
void mysql_board_free_post(struct mysql_board_post *post);

/* Display Functions */
void mysql_board_show_list(struct char_data *ch, int board_id, int page);
void mysql_board_show_post(struct char_data *ch, int board_id, int post_id);
void mysql_board_show_help(struct char_data *ch);

/* Permission Functions */
bool mysql_board_can_read(struct char_data *ch, struct mysql_board_config *board);
bool mysql_board_can_write(struct char_data *ch, struct mysql_board_config *board);
bool mysql_board_can_delete(struct char_data *ch, struct mysql_board_config *board, struct mysql_board_post *post);

/* ACMD Command Functions */
ACMD_DECL(do_read_board);
ACMD_DECL(do_write_board);
ACMD_DECL(do_remove_board);
ACMD_DECL(do_reply_board);
ACMD_DECL(do_note);
ACMD_DECL(do_board);  /* Router that checks for bulletin boards or vessel boarding */
ACMD_DECL(do_boardcheck);  /* Check all boards for unread posts */
ACMD_DECL(do_boardfind);  /* Find all boards in the game world */

/* Board Read Tracking Functions */
bool mysql_board_has_read_post(struct char_data *ch, int post_id);
void mysql_board_mark_post_read(struct char_data *ch, int board_id, int post_id);
void mysql_board_mark_all_read(struct char_data *ch, int board_id);

/* String Editor Integration */
void mysql_board_start_post_title(struct descriptor_data *d, int board_id);
void mysql_board_start_reply_title(struct descriptor_data *d, int board_id, int reply_to_post_id);
void mysql_board_handle_reply_title(struct descriptor_data *d, char *additional_subject);
void mysql_board_finish_post(struct descriptor_data *d, int save);

/* Object Integration */
void show_board_in_room(struct char_data *ch);
struct obj_data *find_board_obj_in_room(struct char_data *ch);

/* Database Helper Functions */
bool mysql_board_execute_query(char *query);
MYSQL_RES *mysql_board_execute_select(char *query);

/* Global Variables */
extern struct mysql_board_config *mysql_board_configs;
extern int mysql_num_boards;

#endif /* __MYSQL_BOARDS_H__ */