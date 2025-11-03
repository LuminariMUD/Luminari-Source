/* ************************************************************************
*   File: bedit.c                                Part of Age of Dragons  *
*  Usage: OLC for MySQL Board System                                     *
*                                                                         *
*  Board Editor - allows in-game creation and modification of boards     *
*  Written by: Gicker (William Rupert Stephen Squires)                   *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "oasis.h"
#include "genolc.h"
#include "interpreter.h"
#include "helpers.h"
#include "mysql/mysql.h"
#include "mysql_boards.h"

/*-------------------------------------------------------------------*/
/* External variables */
extern struct mysql_board_config *mysql_board_configs;
extern int mysql_num_boards;
extern MYSQL *conn;

/*-------------------------------------------------------------------*/
/* Function prototypes */
void bedit_setup_new(struct descriptor_data *d);
void bedit_setup_existing(struct descriptor_data *d, int board_id);
void bedit_parse(struct descriptor_data *d, char *arg);
void bedit_disp_menu(struct descriptor_data *d);
void bedit_disp_board_type_menu(struct descriptor_data *d);
void bedit_save_to_disk(struct descriptor_data *d);
void bedit_save_internally(struct descriptor_data *d);

/*-------------------------------------------------------------------*/
/* Handy macros */
#define B_NUM(board)        ((board)->board_id)
#define B_NAME(board)       ((board)->board_name)
#define B_TYPE(board)       ((board)->board_type)
#define B_READ_LVL(board)   ((board)->read_level)
#define B_WRITE_LVL(board)  ((board)->write_level)
#define B_DELETE_LVL(board) ((board)->delete_level)
#define B_OBJ_VNUM(board)   ((board)->obj_vnum)
#define B_CLAN_ID(board)    ((board)->clan_id)
#define B_CLAN_RANK(board)  ((board)->clan_rank)
#define B_ACTIVE(board)     ((board)->active)

/*-------------------------------------------------------------------*/
/* Board type names */
const char *board_types[] = {
    "General",
    "Immortal",
    "Clan",
    "Quest",
    "Roleplay",
    "Announcement",
    "Newbie",
    "\n"
};

/*-------------------------------------------------------------------*/
/* Menu modes */
#define BEDIT_MAIN_MENU          0
#define BEDIT_NAME               1
#define BEDIT_TYPE               2
#define BEDIT_READ_LEVEL         3
#define BEDIT_WRITE_LEVEL        4
#define BEDIT_DELETE_LEVEL       5
#define BEDIT_OBJ_VNUM           6
#define BEDIT_CLAN_ID            7
#define BEDIT_CLAN_RANK          8
#define BEDIT_ACTIVE             9
#define BEDIT_CONFIRM_SAVESTRING 10

/*-------------------------------------------------------------------*\
  Entry Commands
\*-------------------------------------------------------------------*/

/*
 * bedit command - allows creation/editing of boards
 */
ACMD(do_bedit) {
    int board_id, found = FALSE, i;
    struct descriptor_data *d = ch->desc;
    char arg[MAX_INPUT_LENGTH];
    
    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs cannot use OLC.\r\n");
        return;
    }
    
    one_argument(argument, arg, sizeof(arg));
    
    if (!*arg) {
        send_to_char(ch, "Usage: bedit <board id|new>\r\n");
        return;
    }
    
    if (!str_cmp(arg, "new")) {
        /* Create new board */
        CREATE(d->olc, struct oasis_olc_data, 1);
        bedit_setup_new(d);
        STATE(d) = CON_BEDIT;
        act("$n starts working on board creation.", TRUE, ch, 0, 0, TO_ROOM);
        return;
    }
    
    if (!isdigit(*arg)) {
        send_to_char(ch, "That is not a valid board ID.\r\n");
        return;
    }
    
    board_id = atoi(arg);
    
    /* Check if board exists */
    for (i = 0; i < mysql_num_boards; i++) {
        if (mysql_board_configs[i].board_id == board_id) {
            found = TRUE;
            break;
        }
    }
    
    if (!found) {
        send_to_char(ch, "That board does not exist.\r\n");
        return;
    }
    
    /* Setup existing board for editing */
    CREATE(d->olc, struct oasis_olc_data, 1);
    bedit_setup_existing(d, board_id);
    STATE(d) = CON_BEDIT;
    act("$n starts editing a board.", TRUE, ch, 0, 0, TO_ROOM);
}

/*
 * blist command - list all boards
 */
ACMD(do_blist) {
    char buf[MAX_STRING_LENGTH];
    int i;
    
    if (IS_NPC(ch)) {
        send_to_char(ch, "NPCs cannot use OLC.\r\n");
        return;
    }
    
    if (mysql_num_boards == 0) {
        send_to_char(ch, "No boards currently exist.\r\n");
        return;
    }
    
    sprintf(buf,
        "%-5s %-30s %-12s RD WR DL %-6s %-5s %-4s %s\r\n",
        "ID", "Name", "Type", "ObjVNUM", "Clan", "Rank", "Active");
    send_to_char(ch, "%s", buf);
    send_to_char(ch, "-------------------------------------------------------------------------------------\r\n");
    
    for (i = 0; i < mysql_num_boards; i++) {
        char board_name_buf[MAX_STRING_LENGTH];
        
        /* Parse @ color codes in board name for display */
        if (mysql_board_configs[i].board_name) {
            strncpy(board_name_buf, mysql_board_configs[i].board_name, sizeof(board_name_buf) - 1);
            board_name_buf[sizeof(board_name_buf) - 1] = '\0';
            parse_at(board_name_buf);
        } else {
            strcpy(board_name_buf, "(none)");
        }
        
        sprintf(buf,
            "%-5d %-30s %-12s %2d %2d %2d %-6d %-5d %-4d %s\r\n",
            mysql_board_configs[i].board_id,
            board_name_buf,
            board_types[mysql_board_configs[i].board_type],
            mysql_board_configs[i].read_level,
            mysql_board_configs[i].write_level,
            mysql_board_configs[i].delete_level,
            mysql_board_configs[i].obj_vnum,
            mysql_board_configs[i].clan_id,
            mysql_board_configs[i].clan_rank,
            mysql_board_configs[i].active ? "Yes" : "No");
        send_to_char(ch, "%s", buf);
    }
}

/*-------------------------------------------------------------------*\
  Setup Functions
\*-------------------------------------------------------------------*/

void bedit_setup_new(struct descriptor_data *d) {
    struct mysql_board_config *board;
    int new_id = 1, i;
    
    /* Find next available board ID */
    for (i = 0; i < mysql_num_boards; i++) {
        if (mysql_board_configs[i].board_id >= new_id) {
            new_id = mysql_board_configs[i].board_id + 1;
        }
    }
    
    CREATE(board, struct mysql_board_config, 1);
    board->board_id = new_id;
    board->board_name = strdup("New Board");
    board->board_type = BOARD_TYPE_GENERAL;
    board->read_level = 1;
    board->write_level = 1;
    board->delete_level = 31;
    board->obj_vnum = 0;
    board->clan_id = 0;
    board->clan_rank = 0;
    board->active = TRUE;
    
    OLC_STORAGE(d) = (char *)board;
    OLC_VAL(d) = 0;
    
    bedit_disp_menu(d);
}

void bedit_setup_existing(struct descriptor_data *d, int board_id) {
    struct mysql_board_config *board, *original;
    int i;
    
    /* Find the board */
    original = NULL;
    for (i = 0; i < mysql_num_boards; i++) {
        if (mysql_board_configs[i].board_id == board_id) {
            original = &mysql_board_configs[i];
            break;
        }
    }
    
    if (!original) {
        return;
    }
    
    /* Create a copy for editing */
    CREATE(board, struct mysql_board_config, 1);
    board->board_id = original->board_id;
    board->board_name = strdup(original->board_name);
    board->board_type = original->board_type;
    board->read_level = original->read_level;
    board->write_level = original->write_level;
    board->delete_level = original->delete_level;
    board->obj_vnum = original->obj_vnum;
    board->clan_id = original->clan_id;
    board->clan_rank = original->clan_rank;
    board->active = original->active;
    
    OLC_STORAGE(d) = (char *)board;
    OLC_VAL(d) = 0;
    
    bedit_disp_menu(d);
}

/*-------------------------------------------------------------------*\
  Menu Display Functions
\*-------------------------------------------------------------------*/

void bedit_disp_menu(struct descriptor_data *d) {
    struct mysql_board_config *board;
    char buf[MAX_STRING_LENGTH];
    char board_name_buf[MAX_STRING_LENGTH];
    
    board = (struct mysql_board_config *)OLC_STORAGE(d);
    
    /* Parse @ color codes in board name for display */
    if (B_NAME(board)) {
        strncpy(board_name_buf, B_NAME(board), sizeof(board_name_buf) - 1);
        board_name_buf[sizeof(board_name_buf) - 1] = '\0';
        parse_at(board_name_buf);
    } else {
        strcpy(board_name_buf, "(none)");
    }
    
    sprintf(buf,
#if defined(CLEAR_SCREEN)
        "[H[J"
#endif
        "-- Board Editor\r\n"
        "%s1%s) Board ID     : %s%d%s\r\n"
        "%s2%s) Name         : %s%s%s\r\n"
        "%s3%s) Type         : %s%s%s\r\n"
        "%s4%s) Read Level   : %s%d%s\r\n"
        "%s5%s) Write Level  : %s%d%s\r\n"
        "%s6%s) Delete Level : %s%d%s\r\n"
        "%s7%s) Object VNUM  : %s%d%s\r\n"
        "%s8%s) Clan ID      : %s%d%s (0 = no clan restriction)\r\n"
        "%s9%s) Clan Rank    : %s%d%s (0 = any rank, lower # = higher rank)\r\n"
        "%sA%s) Active       : %s%s%s\r\n"
        "%sQ%s) Quit\r\n"
        "Enter choice : ",
        grn, nrm, cyn, B_NUM(board), nrm,
        grn, nrm, yel, board_name_buf, nrm,
        grn, nrm, yel, board_types[B_TYPE(board)], nrm,
        grn, nrm, cyn, B_READ_LVL(board), nrm,
        grn, nrm, cyn, B_WRITE_LVL(board), nrm,
        grn, nrm, cyn, B_DELETE_LVL(board), nrm,
        grn, nrm, cyn, B_OBJ_VNUM(board), nrm,
        grn, nrm, cyn, B_CLAN_ID(board), nrm,
        grn, nrm, cyn, B_CLAN_RANK(board), nrm,
        grn, nrm, cyn, B_ACTIVE(board) ? "Yes" : "No", nrm,
        grn, nrm);
    
    send_to_char(d->character, "%s", buf);
    OLC_MODE(d) = BEDIT_MAIN_MENU;
}

void bedit_disp_board_type_menu(struct descriptor_data *d) {
    char buf[MAX_STRING_LENGTH];
    int i;
    
    
    sprintf(buf,
#if defined(CLEAR_SCREEN)
        "[H[J"
#endif
        "-- Board Type Menu\r\n");
    
    for (i = 0; board_types[i][0] != '\n'; i++) {
        sprintf(buf + strlen(buf), "%s%d%s) %s\r\n",
            grn, i, nrm, board_types[i]);
    }
    
    strcat(buf, "Enter board type : ");
    send_to_char(d->character, "%s", buf);
}

/*-------------------------------------------------------------------*\
  Save Functions
\*-------------------------------------------------------------------*/

void bedit_save_internally(struct descriptor_data *d) {
    struct mysql_board_config *board, *old_board;
    int i, found = FALSE;
    
    board = (struct mysql_board_config *)OLC_STORAGE(d);
    
    /* Check if this is an existing board or new one */
    for (i = 0; i < mysql_num_boards; i++) {
        if (mysql_board_configs[i].board_id == board->board_id) {
            found = TRUE;
            old_board = &mysql_board_configs[i];
            
            /* Free old name */
            if (old_board->board_name) {
                free(old_board->board_name);
            }
            
            /* Copy new data */
            old_board->board_name = strdup(board->board_name);
            old_board->board_type = board->board_type;
            old_board->read_level = board->read_level;
            old_board->write_level = board->write_level;
            old_board->delete_level = board->delete_level;
            old_board->obj_vnum = board->obj_vnum;
            old_board->clan_id = board->clan_id;
            old_board->clan_rank = board->clan_rank;
            old_board->active = board->active;
            break;
        }
    }
    
    if (!found) {
        /* This is a new board, add it to the array */
        if (mysql_num_boards == 0) {
            CREATE(mysql_board_configs, struct mysql_board_config, 1);
        } else {
            RECREATE(mysql_board_configs, struct mysql_board_config, mysql_num_boards + 1);
        }
        
        mysql_board_configs[mysql_num_boards].board_id = board->board_id;
        mysql_board_configs[mysql_num_boards].board_name = strdup(board->board_name);
        mysql_board_configs[mysql_num_boards].board_type = board->board_type;
        mysql_board_configs[mysql_num_boards].read_level = board->read_level;
        mysql_board_configs[mysql_num_boards].write_level = board->write_level;
        mysql_board_configs[mysql_num_boards].delete_level = board->delete_level;
        mysql_board_configs[mysql_num_boards].obj_vnum = board->obj_vnum;
        mysql_board_configs[mysql_num_boards].clan_id = board->clan_id;
        mysql_board_configs[mysql_num_boards].clan_rank = board->clan_rank;
        mysql_board_configs[mysql_num_boards].active = board->active;
        mysql_num_boards++;
    }
}

void bedit_save_to_disk(struct descriptor_data *d) {
    struct mysql_board_config *board;
    char query[MAX_STRING_LENGTH * 2];
    char escaped_name[513];
    char buf[MAX_STRING_LENGTH];
    
    board = (struct mysql_board_config *)OLC_STORAGE(d);
    
    /* Check if global connection exists */
    if (!conn) {
        send_to_char(d->character, "Error: MySQL connection not available.\r\n");
        return;
    }
    
    /* Escape board name */
    mysql_real_escape_string(conn, escaped_name, board->board_name, strlen(board->board_name));
    
    /* Use INSERT ... ON DUPLICATE KEY UPDATE to insert or update */
    sprintf(query,
        "INSERT INTO mysql_boards "
        "(board_id, board_name, board_type, read_level, write_level, delete_level, obj_vnum, clan_id, clan_rank, active) "
        "VALUES (%d, '%s', %d, %d, %d, %d, %d, %d, %d, %s) "
        "ON DUPLICATE KEY UPDATE "
        "board_name = '%s', "
        "board_type = %d, "
        "read_level = %d, "
        "write_level = %d, "
        "delete_level = %d, "
        "obj_vnum = %d, "
        "clan_id = %d, "
        "clan_rank = %d, "
        "active = %s",
        board->board_id,
        escaped_name,
        board->board_type,
        board->read_level,
        board->write_level,
        board->delete_level,
        board->obj_vnum,
        board->clan_id,
        board->clan_rank,
        board->active ? "TRUE" : "FALSE",
        /* ON DUPLICATE KEY UPDATE values */
        escaped_name,
        board->board_type,
        board->read_level,
        board->write_level,
        board->delete_level,
        board->obj_vnum,
        board->clan_id,
        board->clan_rank,
        board->active ? "TRUE" : "FALSE");
    
    if (mysql_query(conn, query)) {
        sprintf(buf, "SYSERR: Board OLC - Failed to save board %d: %s", 
                board->board_id, mysql_error(conn));
        log("%s", buf);
        send_to_char(d->character, "Error saving board to database.\r\n");
        return;
    }
    
    send_to_char(d->character, "Board saved to database.\r\n");
}

/*-------------------------------------------------------------------*\
  Parse Function
\*-------------------------------------------------------------------*/

void bedit_parse(struct descriptor_data *d, char *arg) {
    struct mysql_board_config *board;
    int number;
    
    board = (struct mysql_board_config *)OLC_STORAGE(d);
    
    switch (OLC_MODE(d)) {
    case BEDIT_MAIN_MENU:
        switch (*arg) {
        case 'q':
        case 'Q':
            if (OLC_VAL(d)) {
                /* Something has been modified */
                send_to_char(d->character, "Do you wish to save this board internally? (y/n) : ");
                OLC_MODE(d) = BEDIT_CONFIRM_SAVESTRING;
            } else {
                send_to_char(d->character, "No changes made.\r\n");
                /* Free board structure memory */
                if (board->board_name) {
                    free(board->board_name);
                }
                free(board);
                OLC_STORAGE(d) = NULL;
                cleanup_olc(d, CLEANUP_ALL);
            }
            break;
        case '1':
            send_to_char(d->character, "Board ID cannot be changed once created.\r\n");
            bedit_disp_menu(d);
            break;
        case '2':
            send_to_char(d->character, "Enter board name : ");
            OLC_MODE(d) = BEDIT_NAME;
            break;
        case '3':
            bedit_disp_board_type_menu(d);
            OLC_MODE(d) = BEDIT_TYPE;
            break;
        case '4':
            send_to_char(d->character, "Enter minimum read level : ");
            OLC_MODE(d) = BEDIT_READ_LEVEL;
            break;
        case '5':
            send_to_char(d->character, "Enter minimum write level : ");
            OLC_MODE(d) = BEDIT_WRITE_LEVEL;
            break;
        case '6':
            send_to_char(d->character, "Enter minimum delete level : ");
            OLC_MODE(d) = BEDIT_DELETE_LEVEL;
            break;
        case '7':
            send_to_char(d->character, "Enter object VNUM for this board : ");
            OLC_MODE(d) = BEDIT_OBJ_VNUM;
            break;
        case '8':
            send_to_char(d->character, "Enter clan ID (0 for no restriction) : ");
            OLC_MODE(d) = BEDIT_CLAN_ID;
            break;
        case '9':
            send_to_char(d->character, "Enter maximum clan rank (0 = any rank, lower numbers = higher ranks) : ");
            OLC_MODE(d) = BEDIT_CLAN_RANK;
            break;
        case 'a':
        case 'A':
            B_ACTIVE(board) = !B_ACTIVE(board);
            OLC_VAL(d) = 1;
            bedit_disp_menu(d);
            break;
        default:
            send_to_char(d->character, "Invalid choice!\r\n");
            bedit_disp_menu(d);
            break;
        }
        break;
        
    case BEDIT_NAME:
        if (board->board_name) {
            free(board->board_name);
        }
        board->board_name = strdup(arg);
        OLC_VAL(d) = 1;
        bedit_disp_menu(d);
        break;
        
    case BEDIT_TYPE:
        number = atoi(arg);
        if (number < 0 || number >= BOARD_TYPE_NEWBIE + 1) {
            send_to_char(d->character, "Invalid board type!\r\n");
            bedit_disp_board_type_menu(d);
        } else {
            B_TYPE(board) = number;
            OLC_VAL(d) = 1;
            bedit_disp_menu(d);
        }
        break;
        
    case BEDIT_READ_LEVEL:
        number = atoi(arg);
        if (number < 0 || number > 100) {
            send_to_char(d->character, "Invalid level!\r\n");
            send_to_char(d->character, "Enter minimum read level : ");
        } else {
            B_READ_LVL(board) = number;
            OLC_VAL(d) = 1;
            bedit_disp_menu(d);
        }
        break;
        
    case BEDIT_WRITE_LEVEL:
        number = atoi(arg);
        if (number < 0 || number > 100) {
            send_to_char(d->character, "Invalid level!\r\n");
            send_to_char(d->character, "Enter minimum write level : ");
        } else {
            B_WRITE_LVL(board) = number;
            OLC_VAL(d) = 1;
            bedit_disp_menu(d);
        }
        break;
        
    case BEDIT_DELETE_LEVEL:
        number = atoi(arg);
        if (number < 0 || number > 100) {
            send_to_char(d->character, "Invalid level!\r\n");
            send_to_char(d->character, "Enter minimum delete level : ");
        } else {
            B_DELETE_LVL(board) = number;
            OLC_VAL(d) = 1;
            bedit_disp_menu(d);
        }
        break;
        
    case BEDIT_OBJ_VNUM:
        number = atoi(arg);
        B_OBJ_VNUM(board) = number;
        OLC_VAL(d) = 1;
        bedit_disp_menu(d);
        break;
        
    case BEDIT_CLAN_ID:
        number = atoi(arg);
        B_CLAN_ID(board) = number;
        OLC_VAL(d) = 1;
        bedit_disp_menu(d);
        break;
        
    case BEDIT_CLAN_RANK:
        number = atoi(arg);
        if (number < 0) {
            send_to_char(d->character, "Invalid clan rank! Must be 0 or higher.\r\n");
            send_to_char(d->character, "Enter minimum clan rank (0 for any rank in clan) : ");
        } else {
            B_CLAN_RANK(board) = number;
            OLC_VAL(d) = 1;
            bedit_disp_menu(d);
        }
        break;
        
    case BEDIT_CONFIRM_SAVESTRING:
        switch (*arg) {
        case 'y':
        case 'Y':
            {
                char buf[MAX_STRING_LENGTH];
                int board_id = board->board_id;
                bedit_save_internally(d);
                bedit_save_to_disk(d);
                sprintf(buf, "OLC: %s edits board %d", GET_NAME(d->character), board_id);
                mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "%s", buf);
                send_to_char(d->character, "Board saved.\r\n");
                /* Free board structure memory */
                if (board->board_name) {
                    free(board->board_name);
                }
                free(board);
                OLC_STORAGE(d) = NULL;
                cleanup_olc(d, CLEANUP_ALL);
            }
            break;
        case 'n':
        case 'N':
            send_to_char(d->character, "Board not saved.\r\n");
            /* Free board structure memory */
            if (board->board_name) {
                free(board->board_name);
            }
            free(board);
            OLC_STORAGE(d) = NULL;
            cleanup_olc(d, CLEANUP_ALL);
            break;
        default:
            send_to_char(d->character, "Invalid choice!\r\n");
            send_to_char(d->character, "Do you wish to save this board? (y/n) : ");
            break;
        }
        break;
        
    default:
        /* We should never get here */
        mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Reached default case in bedit_parse()!");
        break;
    }
}
