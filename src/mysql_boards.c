/* ************************************************************************
*   File: mysql_boards.c                             Part of Age of Dragons *
*  Usage: MySQL-based bulletin board system implementation                *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Age of Dragons MUD - MySQL Board System                               *
*  Written by: Gicker (William Rupert Stephen Squires)                   *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "oasis.h"
#include "genolc.h"
#include "dg_scripts.h"
#include "char_descs.h"
#include "deities.h"
#include "helpers.h"

#include "mariadb/mysql.h"
#include "mysql_boards.h"

/* External Variables */
extern MYSQL *conn;
extern struct room_data *world;
extern struct index_data *obj_index;
extern room_rnum top_of_world;

/* External Commands - original versions before board interception */
void do_look(struct char_data *ch, const char *argument, int cmd, int subcmd);
ACMD_DECL(do_board_vessel); /* Vessel boarding function from vessels.c */
void do_write(struct char_data *ch, const char *argument, int cmd, int subcmd);
void do_remove(struct char_data *ch, const char *argument, int cmd, int subcmd);

/* Global Variables */
struct mysql_board_config *mysql_board_configs = NULL;
int mysql_num_boards = 0;

/* Default board configurations - each board assigned to a specific object vnum */
static struct mysql_board_config default_boards[] = {
    /* board_id, name, type, read_lvl, write_lvl, delete_lvl, obj_vnum, clan_id, active */

    // IMPORTANT! BOARD NAMES CANNOT CONTAIN ' (single quote)

    // No boards listed. Boards are now added with bedit command.
    // View current boards with the blist command.

    {-1, NULL, 0, 0, 0, 0, 0, 0, false} /* End marker */
};

/*
 * Initialize the MySQL board system
 * Creates necessary tables if they don't exist
 */
int mysql_board_init(void)
{
  char query[4096];

  /* Create boards configuration table */
  sprintf(query,
          "CREATE TABLE IF NOT EXISTS mysql_boards ("
          "board_id INT PRIMARY KEY AUTO_INCREMENT, "
          "board_name VARCHAR(100) NOT NULL, "
          "board_type INT NOT NULL DEFAULT 0, "
          "read_level INT NOT NULL DEFAULT 1, "
          "write_level INT NOT NULL DEFAULT 1, "
          "delete_level INT NOT NULL DEFAULT %d, "
          "obj_vnum INT NOT NULL DEFAULT 0, "
          "clan_id INT NOT NULL DEFAULT 0, "
          "clan_rank INT NOT NULL DEFAULT 0, "
          "active BOOLEAN NOT NULL DEFAULT TRUE, "
          "created_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
          ")",
          LVL_IMMORT);

  if (!mysql_board_execute_query(query))
  {
    log("SYSERR: Failed to create mysql_boards table");
    return 0;
  }

  /* Add clan_rank column if it doesn't exist (for upgrading existing databases) */
  sprintf(query,
          "ALTER TABLE mysql_boards ADD COLUMN IF NOT EXISTS clan_rank INT NOT NULL DEFAULT 0");

  /* This query might fail on some MySQL versions, so we don't check the result */
  mysql_board_execute_query(query);

  /* Create posts table */
  sprintf(query,
          "CREATE TABLE IF NOT EXISTS mysql_board_posts ("
          "post_id INT PRIMARY KEY AUTO_INCREMENT, "
          "board_id INT NOT NULL, "
          "title VARCHAR(%d) NOT NULL, "
          "body TEXT NOT NULL, "
          "author VARCHAR(20) NOT NULL, "
          "author_id INT NOT NULL, "
          "author_level INT NOT NULL, "
          "date_posted TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
          "date_modified TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
          "deleted BOOLEAN NOT NULL DEFAULT FALSE, "
          "FOREIGN KEY (board_id) REFERENCES mysql_boards(board_id) ON DELETE CASCADE"
          ")",
          MAX_BOARD_TITLE_LENGTH);

  if (!mysql_board_execute_query(query))
  {
    log("SYSERR: Failed to create mysql_board_posts table");
    return 0;
  }

  /* Create player board reads tracking table */
  sprintf(query, "CREATE TABLE IF NOT EXISTS player_board_reads ("
                 "player_id INT NOT NULL, "
                 "post_id INT NOT NULL, "
                 "board_id INT NOT NULL, "
                 "read_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                 "PRIMARY KEY (player_id, post_id), "
                 "INDEX idx_player_board (player_id, board_id), "
                 "INDEX idx_post (post_id), "
                 "FOREIGN KEY (post_id) REFERENCES mysql_board_posts(post_id) ON DELETE CASCADE"
                 ")");

  if (!mysql_board_execute_query(query))
  {
    log("SYSERR: Failed to create player_board_reads table");
    return 0;
  }

  /* Create player board visits tracking table - tracks first visit to a board */
  sprintf(query, "CREATE TABLE IF NOT EXISTS player_board_visits ("
                 "player_id INT NOT NULL, "
                 "board_id INT NOT NULL, "
                 "first_visit TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                 "last_visit TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                 "PRIMARY KEY (player_id, board_id), "
                 "INDEX idx_player (player_id)"
                 ")");

  if (!mysql_board_execute_query(query))
  {
    log("SYSERR: Failed to create player_board_visits table");
    return 0;
  }

  /* NOTE: Boards are now managed via OLC (bedit command) instead of hardcoded array */
  /* The mysql_board_sync_default_boards() function can still be used for initial setup if needed */

  /* Load board configurations into memory */
  mysql_board_load_configs();

  return 1;
}

/*
 * Synchronize default boards with database - insert new ones and update existing ones
 */
void mysql_board_sync_default_boards(void)
{
  char query[4096];
  char escaped_name[256];
  char buf[MAX_STRING_LENGTH];
  int i;

  /* Check if global connection exists */
  if (!conn)
  {
    log("SYSERR: MySQL board system - No MySQL connection available for board sync");
    return;
  }

  for (i = 0; default_boards[i].board_id != -1; i++)
  {
    /* Escape the board name for SQL safety */
    mysql_real_escape_string(conn, escaped_name, default_boards[i].board_name,
                             strlen(default_boards[i].board_name));

    /* Use INSERT ... ON DUPLICATE KEY UPDATE to insert or update */
    sprintf(query,
            "INSERT INTO mysql_boards "
            "(board_id, board_name, board_type, read_level, write_level, delete_level, obj_vnum, "
            "clan_id, clan_rank, active) "
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
            default_boards[i].board_id, escaped_name, default_boards[i].board_type,
            default_boards[i].read_level, default_boards[i].write_level,
            default_boards[i].delete_level, default_boards[i].obj_vnum, default_boards[i].clan_id,
            default_boards[i].clan_rank, default_boards[i].active ? "TRUE" : "FALSE",
            /* ON DUPLICATE KEY UPDATE values */
            escaped_name, default_boards[i].board_type, default_boards[i].read_level,
            default_boards[i].write_level, default_boards[i].delete_level,
            default_boards[i].obj_vnum, default_boards[i].clan_id, default_boards[i].clan_rank,
            default_boards[i].active ? "TRUE" : "FALSE");

    if (mysql_query(conn, query))
    {
      sprintf(buf, "SYSERR: MySQL board system - Failed to sync board %d (%s): %s",
              default_boards[i].board_id, default_boards[i].board_name, mysql_error(conn));
      log("%s", buf);
    }
    else
    {
      sprintf(buf, "MySQL board system - Synced board %d: %s", default_boards[i].board_id,
              default_boards[i].board_name);
      log("%s", buf);
    }
  }
}

/*
 * Load board configurations from database into memory
 */
void mysql_board_load_configs(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[512];
  int i = 0;

  sprintf(
      query,
      "SELECT board_id, board_name, board_type, read_level, write_level, "
      "delete_level, obj_vnum, clan_id, clan_rank, active FROM mysql_boards WHERE active = TRUE");

  result = mysql_board_execute_select(query);
  if (!result)
    return;

  /* Count rows first */
  mysql_num_boards = mysql_num_rows(result);

  if (mysql_board_configs)
  {
    /* Free existing configs */
    for (i = 0; i < mysql_num_boards; i++)
    {
      if (mysql_board_configs[i].board_name)
      {
        free(mysql_board_configs[i].board_name);
      }
    }
    free(mysql_board_configs);
  }

  /* Allocate memory for configs */
  mysql_board_configs =
      (struct mysql_board_config *)malloc(sizeof(struct mysql_board_config) * mysql_num_boards);

  /* Reset result pointer */
  mysql_data_seek(result, 0);

  /* Load each config */
  i = 0;
  while ((row = mysql_fetch_row(result)) && i < mysql_num_boards)
  {
    mysql_board_configs[i].board_id = atoi(row[0]);
    mysql_board_configs[i].board_name = strdup(row[1]);
    mysql_board_configs[i].board_type = atoi(row[2]);
    mysql_board_configs[i].read_level = atoi(row[3]);
    mysql_board_configs[i].write_level = atoi(row[4]);
    mysql_board_configs[i].delete_level = atoi(row[5]);
    mysql_board_configs[i].obj_vnum = atoi(row[6]);
    mysql_board_configs[i].clan_id = atoi(row[7]);
    mysql_board_configs[i].clan_rank = atoi(row[8]);
    mysql_board_configs[i].active = (atoi(row[9]) == 1);
    i++;
  }

  mysql_free_result(result);
}

/*
 * Get board configuration for a specific object vnum
 */
struct mysql_board_config *mysql_board_get_config_by_obj(int obj_vnum)
{
  int i;

  for (i = 0; i < mysql_num_boards; i++)
  {
    if (mysql_board_configs[i].obj_vnum == obj_vnum && mysql_board_configs[i].active)
    {
      return &mysql_board_configs[i];
    }
  }

  return NULL;
}

/*
 * Get board configuration by checking for board object in room
 */
struct mysql_board_config *mysql_board_get_config_by_object(struct obj_data *obj)
{
  if (!obj)
    return NULL;

  return mysql_board_get_config_by_obj(GET_OBJ_VNUM(obj));
}

/*
 * Find board object in character's current room
 */
struct obj_data *find_board_obj_in_room(struct char_data *ch)
{
  struct obj_data *obj;
  struct mysql_board_config *board_config;

  if (!ch || ch->in_room == NOWHERE)
    return NULL;

  /* Check objects in room */
  for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
  {
    board_config = mysql_board_get_config_by_obj(GET_OBJ_VNUM(obj));
    if (board_config)
    {
      return obj;
    }
  }

  /* Check objects in character's inventory */
  for (obj = ch->carrying; obj; obj = obj->next_content)
  {
    board_config = mysql_board_get_config_by_obj(GET_OBJ_VNUM(obj));
    if (board_config)
    {
      return obj;
    }
  }

  /* Check worn objects */
  int i;
  for (i = 0; i < NUM_WEARS; i++)
  {
    obj = GET_EQ(ch, i);
    if (obj)
    {
      board_config = mysql_board_get_config_by_obj(GET_OBJ_VNUM(obj));
      if (board_config)
      {
        return obj;
      }
    }
  }

  return NULL;
}

/*
 * Legacy function - now finds board by checking for board objects in room
 */
struct mysql_board_config *mysql_board_get_config_by_room(int room_vnum)
{
  struct obj_data *obj;
  struct mysql_board_config *board_config;

  if (room_vnum < 0 || room_vnum > top_of_world)
    return NULL;

  /* Check objects in room for board objects */
  for (obj = world[real_room(room_vnum)].contents; obj; obj = obj->next_content)
  {
    board_config = mysql_board_get_config_by_obj(GET_OBJ_VNUM(obj));
    if (board_config)
    {
      return board_config;
    }
  }

  return NULL;
}

/*
 * Cleanup function
 */
void mysql_board_cleanup(void)
{
  int i;

  if (mysql_board_configs)
  {
    for (i = 0; i < mysql_num_boards; i++)
    {
      if (mysql_board_configs[i].board_name)
      {
        free(mysql_board_configs[i].board_name);
      }
    }
    free(mysql_board_configs);
    mysql_board_configs = NULL;
  }
  mysql_num_boards = 0;
}

/*
 * Create a new post on the specified board
 */
int mysql_board_create_post(struct char_data *ch, int board_id, char *title, char *body)
{
  char query[MAX_BOARD_BODY_LENGTH * 2 + 1024]; /* Larger buffer for escaped content */
  char escaped_title[MAX_BOARD_TITLE_LENGTH * 2 + 1];
  char escaped_body[MAX_BOARD_BODY_LENGTH * 2 + 1];
  char escaped_author[41]; /* Max name length is usually 20, doubled for escaping */
  char buf[MAX_STRING_LENGTH];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  int post_id = -1;

  if (!ch || !title || !body)
  {
    log("SYSERR: mysql_board_create_post called with NULL parameters");
    return -1;
  }

  /* Check global connection */
  if (!conn)
  {
    log("SYSERR: MySQL board system - Global MySQL connection not available");
    return -1;
  }

  /* Escape strings for SQL safety using the same connection */
  mysql_real_escape_string(conn, escaped_title, title, strlen(title));
  mysql_real_escape_string(conn, escaped_body, body, strlen(body));
  mysql_real_escape_string(conn, escaped_author, GET_NAME(ch), strlen(GET_NAME(ch)));

  sprintf(query,
          "INSERT INTO mysql_board_posts "
          "(board_id, title, body, author, author_id, author_level) "
          "VALUES (%d, '%s', '%s', '%s', %ld, %d)",
          board_id, escaped_title, escaped_body, escaped_author, GET_IDNUM(ch), GET_LEVEL(ch));

  /* Execute the insert query */
  if (mysql_query(conn, query))
  {
    sprintf(buf, "SYSERR: MySQL board system - Insert post failed: %s", mysql_error(conn));
    log("%s", buf);
    return -1;
  }

  /* Get the ID of the newly created post */
  sprintf(query, "SELECT LAST_INSERT_ID()");
  if (!mysql_query(conn, query))
  {
    res = mysql_store_result(conn);
    if (res && (row = mysql_fetch_row(res)))
    {
      post_id = atoi(row[0]);
      sprintf(buf, "Board post created successfully: ID %d, Board %d, Author %s", post_id, board_id,
              GET_NAME(ch));
      log("%s", buf);
    }
    else
    {
      log("SYSERR: MySQL board system - Could not retrieve new post ID");
      post_id = -1;
    }

    if (res)
      mysql_free_result(res);
  }
  else
  {
    sprintf(buf, "SYSERR: MySQL board system - LAST_INSERT_ID query failed: %s", mysql_error(conn));
    log("%s", buf);
    post_id = -1;
  }

  return post_id;
}

/*
 * Get a specific post by ID
 */
struct mysql_board_post *mysql_board_get_post(int board_id, int post_id)
{
  char query[512];
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct mysql_board_post *post;

  sprintf(query,
          "SELECT post_id, board_id, title, body, author, author_id, author_level, "
          "UNIX_TIMESTAMP(date_posted), UNIX_TIMESTAMP(date_modified), deleted "
          "FROM mysql_board_posts WHERE board_id = %d AND post_id = %d AND deleted = FALSE",
          board_id, post_id);

  result = mysql_board_execute_select(query);
  if (!result)
    return NULL;

  row = mysql_fetch_row(result);
  if (!row)
  {
    mysql_free_result(result);
    return NULL;
  }

  /* Allocate and populate post structure */
  post = (struct mysql_board_post *)malloc(sizeof(struct mysql_board_post));
  post->post_id = atoi(row[0]);
  post->board_id = atoi(row[1]);
  post->title = strdup(row[2]);
  post->body = strdup(row[3]);
  post->author = strdup(row[4]);
  post->author_id = atoi(row[5]);
  post->author_level = atoi(row[6]);
  post->date_posted = (time_t)atol(row[7]);
  post->date_modified = (time_t)atol(row[8]);
  post->deleted = (atoi(row[9]) == 1);

  mysql_free_result(result);
  return post;
}

/*
 * Free memory used by a post structure
 */
void mysql_board_free_post(struct mysql_board_post *post)
{
  if (!post)
    return;

  if (post->title)
    free(post->title);
  if (post->body)
    free(post->body);
  if (post->author)
    free(post->author);
  free(post);
}

/*
 * Show list of posts on a board
 */
void mysql_board_show_list(struct char_data *ch, int board_id, int page)
{
  char query[512], buf[MAX_STRING_LENGTH];
  char time_buf[80];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int offset, post_count = 0, i = 1;
  struct mysql_board_config *board = NULL;

  if (page < 1)
    page = 1;
  offset = (page - 1) * POSTS_PER_PAGE;

  /* Mark this board as visited by the player */
  if (!IS_NPC(ch) && GET_IDNUM(ch) > 0)
  {
    mysql_board_mark_board_visited(ch, board_id);
  }

  /* Find the board configuration */
  for (i = 0; i < mysql_num_boards; i++)
  {
    if (mysql_board_configs[i].board_id == board_id)
    {
      board = &mysql_board_configs[i];
      break;
    }
  }
  i = 1; /* Reset counter for post numbering */

  /* Get total count first */
  sprintf(query, "SELECT COUNT(*) FROM mysql_board_posts WHERE board_id = %d AND deleted = FALSE",
          board_id);

  result = mysql_board_execute_select(query);
  if (result)
  {
    row = mysql_fetch_row(result);
    if (row)
      post_count = atoi(row[0]);
    mysql_free_result(result);
  }

  if (post_count == 0)
  {
    char empty_msg[80] = "There are no posts on this board.";
    char help_msg[80] = "Type 'board help' for board commands";
    char board_name_buf[MAX_STRING_LENGTH];

    /* Parse @ color codes in board name */
    if (board && board->board_name)
    {
      strncpy(board_name_buf, board->board_name, sizeof(board_name_buf) - 1);
      board_name_buf[sizeof(board_name_buf) - 1] = '\0';
      parse_at(board_name_buf);
    }
    else
    {
      strcpy(board_name_buf, "Unknown Board");
    }

    sprintf(
        buf,
        "\r\n\tY+------------------------------------------------------------------------------+"
        "\tn\r\n"
        "\tY|%-78.78s|\tn\r\n"
        "\tY+------------------------------------------------------------------------------+\tn\r\n"
        "\tY|\tR%-78.78s\tY|\tn\r\n"
        "\tY|\tG%-78.78s\tY|\tn\r\n"
        "\tY+------------------------------------------------------------------------------+"
        "\tn\r\n",
        board_name_buf, empty_msg, help_msg);
    send_to_char(ch, "%s", buf);
    return;
  }

  /* Get posts for this page */
  sprintf(query,
          "SELECT post_id, title, author, UNIX_TIMESTAMP(date_posted) "
          "FROM mysql_board_posts WHERE board_id = %d AND deleted = FALSE "
          "ORDER BY date_posted DESC LIMIT %d OFFSET %d",
          board_id, POSTS_PER_PAGE, offset);

  result = mysql_board_execute_select(query);
  if (!result)
  {
    send_to_char(ch, "Error retrieving posts.\r\n");
    return;
  }

  {
    char board_name_buf[MAX_STRING_LENGTH];

    /* Parse @ color codes in board name */
    if (board && board->board_name)
    {
      strncpy(board_name_buf, board->board_name, sizeof(board_name_buf) - 1);
      board_name_buf[sizeof(board_name_buf) - 1] = '\0';
      parse_at(board_name_buf);
    }
    else
    {
      strcpy(board_name_buf, "Unknown Board");
    }

    sprintf(
        buf,
        "\r\n\tY+------------------------------------------------------------------------------+"
        "\tn\r\n"
        "\tY|%-78.78s|\tn\r\n"
        "\tY+------------------------------------------------------------------------------+\tn\r\n"
        "\tY|\tC#    Title                                    Author           Date           "
        "\tY|\tn\r\n"
        "\tY+------------------------------------------------------------------------------+"
        "\tn\r\n",
        board_name_buf);
  }

  while ((row = mysql_fetch_row(result)))
  {
    char title_buf[MAX_BOARD_TITLE_LENGTH + 1];
    char unread_marker[10] = "";
    int post_id_val = atoi(row[0]);
    time_t post_time = (time_t)atol(row[3]);
    struct tm *timeinfo = localtime(&post_time);
    strftime(time_buf, sizeof(time_buf), "%m/%d/%y", timeinfo);

    /* Check if this post is unread by the player */
    if (!IS_NPC(ch) && GET_IDNUM(ch) > 0)
    {
      if (!mysql_board_has_read_post(ch, post_id_val))
      {
        strcpy(unread_marker, "\tB*\tn");
      }
      else
      {
        strcpy(unread_marker, " ");
      }
    }
    else
    {
      strcpy(unread_marker, " ");
    }

    /* Copy and parse @ color codes in title */
    strncpy(title_buf, row[1], sizeof(title_buf) - 1);
    title_buf[sizeof(title_buf) - 1] = '\0';
    parse_at(title_buf);

    sprintf(buf + strlen(buf), "\tY|\tC%-4d%s %-39.39s %-15.15s %-8.8s        \tY|\tn\r\n",
            post_id_val, unread_marker, title_buf, row[2], time_buf);
    i++;
  }

  {
    char stats_line[80], help_line[80];
    sprintf(stats_line, "Total Posts: %d    Page: %d of %d", post_count, page,
            ((post_count - 1) / POSTS_PER_PAGE) + 1);
    sprintf(help_line, "Type 'board help' for board commands");

    sprintf(
        buf + strlen(buf),
        "\tY+------------------------------------------------------------------------------+\tn\r\n"
        "\tY|\tC%-78.78s\tY|\tn\r\n"
        "\tY|\tG%-78.78s\tY|\tn\r\n"
        "\tY+------------------------------------------------------------------------------+"
        "\tn\r\n",
        stats_line, help_line);
  }

  send_to_char(ch, "%s", buf);
  mysql_free_result(result);
}

/*
 * Show a specific post
 */
void mysql_board_show_post(struct char_data *ch, int board_id, int post_id)
{
  struct mysql_board_post *post;
  char buf[MAX_STRING_LENGTH];
  char *time_str;

  post = mysql_board_get_post(board_id, post_id);
  if (!post)
  {
    send_to_char(ch, "That post does not exist.\r\n");
    return;
  }

  /* Mark this board as visited by the player */
  if (!IS_NPC(ch) && GET_IDNUM(ch) > 0)
  {
    mysql_board_mark_board_visited(ch, board_id);
  }

  time_str = ctime(&post->date_posted);
  time_str[strlen(time_str) - 1] = '\0'; /* Remove newline */

  /* Parse @ color codes in the post body and title */
  if (post->body)
  {
    parse_at(post->body);
  }
  if (post->title)
  {
    parse_at(post->title);
  }

  sprintf(
      buf,
      "\r\n\tC\tW%s\tn\r\n"
      "\tY================================================================================\tn\r\n"
      "\tCPost #%d\tn                                                      \tCBy: %s\tn\r\n"
      "\tCPosted: %s\tn\r\n"
      "\tY================================================================================\tn\r\n"
      "%s\r\n"
      "\tY================================================================================\tn\r\n",
      post->title, post->post_id, post->author, time_str, post->body);

  send_to_char(ch, "%s", buf);

  /* Mark post as read for this player (before freeing the post!) */
  if (!IS_NPC(ch) && GET_IDNUM(ch) > 0)
  {
    mysql_board_mark_post_read(ch, board_id, post->post_id);
  }

  mysql_board_free_post(post);
}

/*
 * Show board help information
 */
void mysql_board_show_help(struct char_data *ch)
{
  char buf[MAX_STRING_LENGTH];

  sprintf(
      buf,
      "\r\n\tC\tWBoard Commands Help\tn\r\n"
      "\tY================================================================================\tn\r\n"
      "\tCread <post#>\tn        - Read a specific post by number\r\n"
      "\tCwrite\tn               - Create a new post (opens editor)\r\n"
      "\tCboard reply <post#>\tn - Reply to a post with quoted text (opens editor)\r\n"
      "\tCbreply <post#>\tn      - Alternate form of board reply command\r\n"
      "\tCremove <post#>\tn      - Delete your own posts or posts by lower-level players\r\n"
      "\tCboard [list] [page]\tn - List posts on the board (optional page number)\r\n"
      "\tCboard markread\tn      - Mark all posts on this board as read\r\n"
      "\tCboardcheck\tn          - Check all boards for unread posts\r\n"
      "\tCboard help\tn          - Show this help information\r\n"
      "\tY================================================================================\tn\r\n"
      "\tYNote: Unread posts are marked with a \tB*\tY symbol in the post list.\tn\r\n"
      "\tYYou must have the appropriate level to use each command.\tn\r\n");

  send_to_char(ch, "%s", buf);
}

/*
 * Check if character can read from the board
 */
bool mysql_board_can_read(struct char_data *ch, struct mysql_board_config *board)
{
  if (!ch || !board)
    return false;

  /* Immortals (level 31+) can access any board */
  if (GET_LEVEL(ch) >= 31)
  {
    return true;
  }

  /* If this is a clan board (clan_id != 0), check clan membership */
  if (board->clan_id != 0)
  {
    if (GET_CLAN(ch) != board->clan_id)
    {
      return false;
    }

    /* If clan rank is specified (clan_rank != 0), check rank requirement
         * Lower rank numbers = higher positions (e.g., rank 1 = leader)
         * Player must have rank <= board requirement to access */
    if (board->clan_rank != 0)
    {
      if (GET_CLANRANK(ch) > board->clan_rank)
      {
        return false;
      }
    }
  }

  /* Check level requirement */
  if (GET_LEVEL(ch) >= board->read_level)
  {
    return true;
  }

  return false;
}

/*
 * Check if character can write to the board
 */
bool mysql_board_can_write(struct char_data *ch, struct mysql_board_config *board)
{
  if (!ch || !board)
    return false;

  /* Immortals (level 31+) can write to any board */
  if (GET_LEVEL(ch) >= 31)
  {
    return true;
  }

  /* If this is a clan board (clan_id != 0), check clan membership */
  if (board->clan_id != 0)
  {
    if (GET_CLAN(ch) != board->clan_id)
    {
      return false;
    }

    /* If clan rank is specified (clan_rank != 0), check rank requirement
         * Lower rank numbers = higher positions (e.g., rank 1 = leader)
         * Player must have rank <= board requirement to access */
    if (board->clan_rank != 0)
    {
      if (GET_CLANRANK(ch) > board->clan_rank)
      {
        return false;
      }
    }
  }

  /* Check level requirement */
  if (GET_LEVEL(ch) >= board->write_level)
  {
    return true;
  }

  return false;
}

/*
 * Check if character can delete posts from the board
 */
bool mysql_board_can_delete(struct char_data *ch, struct mysql_board_config *board,
                            struct mysql_board_post *post)
{
  if (!ch || !board || !post)
    return false;

  /* Immortals (level 31+) bypass clan restrictions and can delete any post from lower level players */
  if (GET_LEVEL(ch) >= 31)
  {
    /* Immortals can delete posts by players of lower level */
    if (post->author_level < GET_LEVEL(ch))
    {
      return true;
    }
    /* Immortals can delete their own posts */
    if (post->author_id == GET_IDNUM(ch))
    {
      return true;
    }
    /* Cannot delete posts by immortals of same or higher level */
    return false;
  }

  /* If this is a clan board (clan_id != 0), check clan membership */
  if (board->clan_id != 0)
  {
    if (GET_CLAN(ch) != board->clan_id)
    {
      return false;
    }

    /* If clan rank is specified (clan_rank != 0), check rank requirement
         * Lower rank numbers = higher positions (e.g., rank 1 = leader)
         * Player must have rank <= board requirement to access */
    if (board->clan_rank != 0)
    {
      if (GET_CLANRANK(ch) > board->clan_rank)
      {
        return false;
      }
    }
  }

  /* Must meet minimum delete level for the board to delete anything */
  if (GET_LEVEL(ch) < board->delete_level)
  {
    return false;
  }

  /* Can delete own posts if they meet delete level */
  if (post->author_id == GET_IDNUM(ch))
  {
    return true;
  }

  /* Players below level 31 can only delete their own posts */
  return false;
}

/*
 * Delete a post
 */
bool mysql_board_delete_post(struct char_data *ch, int board_id, int post_id)
{
  char query[256];

  sprintf(query, "UPDATE mysql_board_posts SET deleted = TRUE WHERE board_id = %d AND post_id = %d",
          board_id, post_id);

  return mysql_board_execute_query(query);
}

/* ACMD Functions */

/*
 * READ command - read a specific post or fall back to original read
 */
ACMD(do_read_board)
{
  struct mysql_board_config *board;
  struct obj_data *board_obj;
  char arg[MAX_INPUT_LENGTH];
  int post_id;

  /* Check if player has access to a board object */
  board_obj = find_board_obj_in_room(ch);
  if (!board_obj)
  {
    /* No board object available, use original read command (part of look) */
    do_look(ch, argument, cmd, 0);
    return;
  }

  board = mysql_board_get_config_by_object(board_obj);
  if (!board)
  {
    do_look(ch, argument, cmd, 0);
    return;
  }

  /* Get post number */
  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(
        ch, "Read which post? Usage: read <post number> or 'read board' to see all posts.\r\n");
    send_to_char(ch, "Type 'board help' for all board commands.\r\n");
    return;
  }

  if (!is_number(arg))
  {
    /* Check if they want to read the board itself */
    if (is_abbrev(arg, "board") || is_abbrev(arg, "boards"))
    {
      /* Check read permission */
      if (!mysql_board_can_read(ch, board))
      {
        send_to_char(ch, "You don't have permission to read this board.\r\n");
        return;
      }

      /* Display board list */
      mysql_board_show_list(ch, board->board_id, 1);
      return;
    }

    /* Not a number or board, probably trying to read an object */
    do_look(ch, argument, cmd, 0);
    return;
  }

  /* Check read permission */
  if (!mysql_board_can_read(ch, board))
  {
    send_to_char(ch, "You don't have permission to read this board.\r\n");
    return;
  }

  post_id = atoi(arg);
  mysql_board_show_post(ch, board->board_id, post_id);
}

/*
 * WRITE command - create a new post or fall back to original write
 */
ACMD(do_write_board)
{
  struct mysql_board_config *board;
  struct obj_data *board_obj;

  /* Check if player has access to a board object */
  board_obj = find_board_obj_in_room(ch);
  if (!board_obj)
  {
    /* No board object available, use original write command */
    do_write(ch, argument, cmd, subcmd);
    return;
  }

  board = mysql_board_get_config_by_object(board_obj);
  if (!board)
  {
    do_write(ch, argument, cmd, subcmd);
    return;
  }

  /* Check write permission */
  if (!mysql_board_can_write(ch, board))
  {
    send_to_char(ch, "You don't have permission to write on this board.\r\n");
    return;
  }

  /* Start post creation process */
  if (ch->desc)
  {
    send_to_char(
        ch, "Starting board post creation. Use /s to save, /c to clear, /h for editor help.\r\n");
    mysql_board_start_post_title(ch->desc, board->board_id);
  }
  else
  {
    send_to_char(ch, "You can't write posts without a connection.\r\n");
  }
}

/*
 * REMOVE command - delete a post or fall back to original remove
 */
ACMD(do_remove_board)
{
  struct mysql_board_config *board;
  struct mysql_board_post *post;
  struct obj_data *board_obj;
  char arg[MAX_INPUT_LENGTH];
  int post_id;

  /* Check if player has access to a board object */
  board_obj = find_board_obj_in_room(ch);
  if (!board_obj)
  {
    /* No board object available, use original remove command */
    do_remove(ch, argument, cmd, subcmd);
    return;
  }

  board = mysql_board_get_config_by_object(board_obj);
  if (!board)
  {
    do_remove(ch, argument, cmd, subcmd);
    return;
  }

  /* Get post number */
  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    /* No argument - provide help for both board and equipment removal */
    send_to_char(ch, "Remove what? Usage:\r\n");
    send_to_char(ch, "  remove <post number> - Delete a board post\r\n");
    send_to_char(ch, "  remove <equipment>   - Remove worn equipment\r\n");
    send_to_char(ch, "Type 'board help' for all board commands.\r\n");
    return;
  }

  if (!is_number(arg))
  {
    /* Not a number, probably trying to remove equipment */
    do_remove(ch, argument, cmd, subcmd);
    return;
  }

  post_id = atoi(arg);

  /* Get the post to check permissions */
  post = mysql_board_get_post(board->board_id, post_id);
  if (!post)
  {
    send_to_char(ch, "That post does not exist.\r\n");
    return;
  }

  /* Check delete permission */
  if (!mysql_board_can_delete(ch, board, post))
  {
    send_to_char(ch, "You don't have permission to remove that post.\r\n");
    mysql_board_free_post(post);
    return;
  }

  /* Delete the post */
  if (mysql_board_delete_post(ch, board->board_id, post_id))
  {
    send_to_char(ch, "Post removed.\r\n");
  }
  else
  {
    send_to_char(ch, "Error removing post.\r\n");
  }

  mysql_board_free_post(post);
}

/*
 * REPLY command - reply to a board post with quoted text
 */
ACMD(do_reply_board)
{
  struct mysql_board_config *board;
  struct obj_data *board_obj;
  char arg[MAX_INPUT_LENGTH];
  int post_id;

  /* Check if player has access to a board object */
  board_obj = find_board_obj_in_room(ch);
  if (!board_obj)
  {
    send_to_char(ch, "There is no board available here.\r\n");
    return;
  }

  board = mysql_board_get_config_by_object(board_obj);
  if (!board)
  {
    send_to_char(ch, "There is no board available here.\r\n");
    return;
  }

  /* Check write permission */
  if (!mysql_board_can_write(ch, board))
  {
    send_to_char(ch, "You don't have permission to write on this board.\r\n");
    return;
  }

  /* Get post number */
  one_argument(argument, arg, sizeof(arg));

  if (!*arg || !is_number(arg))
  {
    send_to_char(ch, "Usage: reply <post number>\r\n");
    return;
  }

  post_id = atoi(arg);

  /* Start reply creation process */
  if (ch->desc)
  {
    send_to_char(
        ch, "Starting board reply creation. Use /s to save, /c to clear, /h for editor help.\r\n");
    mysql_board_start_reply_title(ch->desc, board->board_id, post_id);
  }
  else
  {
    send_to_char(ch, "You can't write replies without a connection.\r\n");
  }
}

/*
 * BOARD command - list posts or show help (also accessible via 'note' for compatibility)
 */
ACMD(do_note)
{
  struct mysql_board_config *board;
  struct obj_data *board_obj;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int page = 1;

  /* Check if player has access to a board object */
  board_obj = find_board_obj_in_room(ch);
  if (!board_obj)
  {
    send_to_char(ch, "There is no board available here.\r\n");
    return;
  }

  board = mysql_board_get_config_by_object(board_obj);
  if (!board)
  {
    send_to_char(ch, "There is no board available here.\r\n");
    return;
  }

  /* Parse arguments */
  two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    /* No arguments - show board info and list first page */
    if (!mysql_board_can_read(ch, board))
    {
      send_to_char(ch, "You don't have permission to read this board.\r\n");
      return;
    }

    mysql_board_show_list(ch, board->board_id, 1);
    return;
  }

  if (is_abbrev(arg1, "help"))
  {
    mysql_board_show_help(ch);
    return;
  }

  if (is_abbrev(arg1, "list"))
  {
    if (!mysql_board_can_read(ch, board))
    {
      send_to_char(ch, "You don't have permission to read this board.\r\n");
      return;
    }

    if (*arg2 && is_number(arg2))
    {
      page = atoi(arg2);
    }

    mysql_board_show_list(ch, board->board_id, page);
    return;
  }

  if (is_abbrev(arg1, "reply"))
  {
    int post_id;

    if (!mysql_board_can_write(ch, board))
    {
      send_to_char(ch, "You don't have permission to write on this board.\r\n");
      return;
    }

    if (!*arg2 || !is_number(arg2))
    {
      send_to_char(ch, "Usage: board reply <post number>\r\n");
      return;
    }

    post_id = atoi(arg2);

    /* Start reply creation process */
    if (ch->desc)
    {
      mysql_board_start_reply_title(ch->desc, board->board_id, post_id);
    }
    else
    {
      send_to_char(ch, "You can't write replies without a connection.\r\n");
    }
    return;
  }

  if (is_abbrev(arg1, "markread"))
  {
    if (!mysql_board_can_read(ch, board))
    {
      send_to_char(ch, "You don't have permission to read this board.\r\n");
      return;
    }

    mysql_board_mark_all_read(ch, board->board_id);
    return;
  }

  if (is_number(arg1))
  {
    /* Treat as page number for list */
    if (!mysql_board_can_read(ch, board))
    {
      send_to_char(ch, "You don't have permission to read this board.\r\n");
      return;
    }

    page = atoi(arg1);
    mysql_board_show_list(ch, board->board_id, page);
    return;
  }

  send_to_char(ch, "Invalid board command. Type 'board help' for help.\r\n");
}

/*
 * Router for the 'board' command - checks if there's a bulletin board or falls back to vessel boarding
 */
ACMD(do_board)
{
  struct obj_data *board_obj;

  /* Check if there's a bulletin board object in the room */
  board_obj = find_board_obj_in_room(ch);
  if (board_obj)
  {
    /* There's a board here, use the note/board command */
    do_note(ch, argument, cmd, subcmd);
  }
  else
  {
    /* No board object, try vessel boarding */
    do_board_vessel(ch, argument, cmd, subcmd);
  }
}

/*
 * Start writing a board post - prompt for title
 */
void mysql_board_start_post_title(struct descriptor_data *d, int board_id)
{
  if (!d || !d->character)
    return;

  d->board_id = board_id;
  d->reply_to_post_id = 0; /* Not a reply */

  /* Clear any existing board title */
  if (d->board_title)
  {
    free(d->board_title);
    d->board_title = NULL;
  }

  SEND_TO_Q("\r\nEnter the title for your post: ", d);
  STATE(d) = CON_BOARD_TITLE;
}

/*
 * Start writing a reply to a board post - prompt for title
 */
void mysql_board_start_reply_title(struct descriptor_data *d, int board_id, int reply_to_post_id)
{
  struct mysql_board_post *original_post;

  if (!d || !d->character)
    return;

  /* Verify the original post exists */
  original_post = mysql_board_get_post(board_id, reply_to_post_id);
  if (!original_post)
  {
    SEND_TO_Q("The post you are trying to reply to doesn't exist.\r\n", d);
    return;
  }

  d->board_id = board_id;
  d->reply_to_post_id = reply_to_post_id;

  /* Clear any existing board title */
  if (d->board_title)
  {
    free(d->board_title);
    d->board_title = NULL;
  }

  char prompt_buf[256];
  sprintf(prompt_buf,
          "\r\nEnter additional subject text (will be prefixed with \"Re: Post #%d - \"): ",
          reply_to_post_id);
  SEND_TO_Q(prompt_buf, d);
  STATE(d) = CON_BOARD_TITLE;

  mysql_board_free_post(original_post);
}

/*
 * Handle reply title creation and set up quoted body
 */
void mysql_board_handle_reply_title(struct descriptor_data *d, char *additional_subject)
{
  struct mysql_board_post *original_post;
  char *quoted_body, *line_start, *line_end;
  char full_title[MAX_BOARD_TITLE_LENGTH + 1];
  char temp_line[MAX_STRING_LENGTH];
  int quoted_length = 0;

  if (!d || !d->character)
    return;

  /* Get the original post */
  original_post = mysql_board_get_post(d->board_id, d->reply_to_post_id);
  if (!original_post)
  {
    SEND_TO_Q("The post you are replying to no longer exists. Reply aborted.\r\n", d);
    STATE(d) = CON_PLAYING;
    return;
  }

  /* Create the reply title */
  if (additional_subject && *additional_subject)
  {
    snprintf(full_title, sizeof(full_title), "Re: Post #%d - %s", d->reply_to_post_id,
             additional_subject);
  }
  else
  {
    snprintf(full_title, sizeof(full_title), "Re: Post #%d", d->reply_to_post_id);
  }

  /* Save the title */
  if (d->board_title)
  {
    free(d->board_title);
  }
  d->board_title = strdup(full_title);

  /* Calculate size needed for quoted body */
  quoted_length = strlen(original_post->body) * 2 + 1000; /* Extra space for formatting */
  quoted_body = malloc(quoted_length);

  if (!quoted_body)
  {
    SEND_TO_Q("Memory error. Reply aborted.\r\n", d);
    mysql_board_free_post(original_post);
    STATE(d) = CON_PLAYING;
    return;
  }

  /* Create quoted version of original message */
  sprintf(quoted_body, "In message %d, %s wrote:\r\n", d->reply_to_post_id, original_post->author);

  /* Quote the original body line by line */
  line_start = original_post->body;
  while (*line_start)
  {
    /* Find end of line */
    line_end = strchr(line_start, '\n');
    if (line_end)
    {
      /* Copy line to temp buffer */
      int line_len = line_end - line_start;
      if (line_len >= MAX_STRING_LENGTH - 1)
      {
        line_len = MAX_STRING_LENGTH - 2;
      }
      strncpy(temp_line, line_start, line_len);
      temp_line[line_len] = '\0';

      /* Remove \r if present */
      if (line_len > 0 && temp_line[line_len - 1] == '\r')
      {
        temp_line[line_len - 1] = '\0';
      }
    }
    else
    {
      /* Last line */
      strcpy(temp_line, line_start);
    }

    /* Add quoted line */
    strcat(quoted_body, "| ");
    strcat(quoted_body, temp_line);
    strcat(quoted_body, "\r\n");

    if (!line_end)
      break;
    line_start = line_end + 1;
  }

  /* Add separator and space for new content */
  strcat(quoted_body, "\r\n--- Reply is below this line ---\r\n\r\n");

  /* Set up string editor with quoted content */
  d->str = (char **)malloc(sizeof(char *));
  *(d->str) = strdup(quoted_body);
  d->max_str = MAX_BOARD_BODY_LENGTH;
  d->backstr = NULL;

  SEND_TO_Q("Reply editor started. The original message has been quoted above.\r\n", d);
  SEND_TO_Q("You can edit or delete parts as needed. Use /s to save, /h for help.\r\n", d);
  STATE(d) = CON_BOARD_POST;

  free(quoted_body);
  mysql_board_free_post(original_post);
}

/*
 * Handle completion of board post editing
 */
void mysql_board_finish_post(struct descriptor_data *d, int save)
{
  char buf[MAX_STRING_LENGTH];

  if (!d || !d->character)
    return;

  if (save && d->str && *d->str && strlen(*d->str) > 0)
  {
    /* Save the post to database - copy and strip carriage returns */
    strncpy(buf, *d->str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    strip_cr(buf);

    int result = mysql_board_create_post(d->character, d->board_id, d->board_title, buf);
    if (result > 0)
    {
      char msg_buf[256];
      sprintf(msg_buf, "Your post has been saved to the board (Post #%d).\r\n", result);
      SEND_TO_Q(msg_buf, d);
    }
    else
    {
      char msg_buf[256];
      sprintf(msg_buf, "Error: Could not save your post to the board (Error code: %d).\r\n",
              result);
      SEND_TO_Q(msg_buf, d);
    }
  }
  else
  {
    SEND_TO_Q("Your post has been aborted.\r\n", d);
  }

  /* Clean up the string data */
  if (d->str)
  {
    if (*d->str)
    {
#ifdef DEBUGMEM
      freeusg(*d->str, B8);
#else
      free(*d->str);
#endif
    }
#ifdef DEBUGMEM
    freeusg(d->str, B8);
#else
    free(d->str);
#endif
    d->str = NULL;
  }

  /* Reset board fields */
  d->board_id = 0;
  d->reply_to_post_id = 0;
  if (d->board_title)
  {
    free(d->board_title);
    d->board_title = NULL;
  }

  /* Return to playing state */
  STATE(d) = CON_PLAYING;
}

/*
 * Show the board in the current room (for 'look board' command)
 */
void show_board_in_room(struct char_data *ch)
{
  struct mysql_board_config *board;
  struct obj_data *board_obj;

  if (!ch || ch->in_room == NOWHERE)
  {
    return;
  }

  /* Find board object in room */
  board_obj = find_board_obj_in_room(ch);
  if (!board_obj)
  {
    return; /* No board available, don't display anything */
  }

  board = mysql_board_get_config_by_object(board_obj);
  if (!board)
  {
    return;
  }

  /* Check read permission */
  if (!mysql_board_can_read(ch, board))
  {
    send_to_char(ch, "There is a board here, but you don't have permission to read it.\r\n");
    return;
  }

  /* Show that there's a board here */
  char board_name_buf[MAX_STRING_LENGTH];

  /* Parse @ color codes in board name */
  strncpy(board_name_buf, board->board_name, sizeof(board_name_buf) - 1);
  board_name_buf[sizeof(board_name_buf) - 1] = '\0';
  parse_at(board_name_buf);

  send_to_char(ch, "\r\n\tY%s is mounted here. Type 'board' or 'note' to use it.\tn\r\n",
               board_name_buf);
}

/*
 * Database helper function - execute a query
 */
bool mysql_board_execute_query(char *query)
{
  bool success = false;
  char buf[MAX_STRING_LENGTH];

  if (!query)
    return false;

  /* Check global connection */
  if (!conn)
  {
    log("SYSERR: MySQL board system - Global MySQL connection not available");
    return false;
  }

  /* Execute query */
  if (!mysql_query(conn, query))
  {
    success = true;
  }
  else
  {
    sprintf(buf, "SYSERR: MySQL board system query failed: %s", mysql_error(conn));
    log("%s", buf);
  }

  return success;
}

/*
 * Database helper function - execute a SELECT query
 */
MYSQL_RES *mysql_board_execute_select(char *query)
{
  MYSQL_RES *result = NULL;
  char buf[MAX_STRING_LENGTH];

  if (!query)
    return NULL;

  /* Check global connection */
  if (!conn)
  {
    log("SYSERR: MySQL board system - Global MySQL connection not available");
    return NULL;
  }

  /* Execute query and get result */
  if (!mysql_query(conn, query))
  {
    result = mysql_store_result(conn);
  }
  else
  {
    sprintf(buf, "SYSERR: MySQL board system SELECT failed: %s", mysql_error(conn));
    log("%s", buf);
  }

  /* Note: Don't close connection here as result is still being used */
  /* The calling function must call mysql_free_result() */

  return result;
}

/*
 * Check if a player has read a specific post
 */
bool mysql_board_has_read_post(struct char_data *ch, int post_id)
{
  char query[512];
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool has_read = false;

  if (!ch || IS_NPC(ch) || GET_IDNUM(ch) <= 0)
    return false;

  sprintf(query,
          "SELECT 1 FROM player_board_reads "
          "WHERE player_id = %ld AND post_id = %d",
          GET_IDNUM(ch), post_id);

  result = mysql_board_execute_select(query);
  if (result)
  {
    if ((row = mysql_fetch_row(result)))
    {
      has_read = true;
    }
    mysql_free_result(result);
  }

  return has_read;
}

/*
 * Mark a specific post as read by a player
 */
void mysql_board_mark_post_read(struct char_data *ch, int board_id, int post_id)
{
  char query[512];

  if (!ch || IS_NPC(ch) || GET_IDNUM(ch) <= 0)
    return;

  /* Insert or update read record */
  sprintf(query,
          "INSERT INTO player_board_reads (player_id, post_id, board_id, read_date) "
          "VALUES (%ld, %d, %d, NOW()) "
          "ON DUPLICATE KEY UPDATE read_date = NOW()",
          GET_IDNUM(ch), post_id, board_id);

  if (!mysql_board_execute_query(query))
  {
    log("SYSERR: mysql_board_mark_post_read failed for player %ld, board %d, post %d",
        GET_IDNUM(ch), board_id, post_id);
  }
}

/*
 * Mark a board as visited by a player
 * Called when player views board list or reads any post
 */
void mysql_board_mark_board_visited(struct char_data *ch, int board_id)
{
  char query[512];

  if (!ch || IS_NPC(ch) || GET_IDNUM(ch) <= 0)
    return;

  /* Insert or update visit record */
  sprintf(query,
          "INSERT INTO player_board_visits (player_id, board_id, first_visit, last_visit) "
          "VALUES (%ld, %d, NOW(), NOW()) "
          "ON DUPLICATE KEY UPDATE last_visit = NOW()",
          GET_IDNUM(ch), board_id);

  mysql_board_execute_query(query);
}

/*
 * Mark all posts on a board as read
 */
void mysql_board_mark_all_read(struct char_data *ch, int board_id)
{
  char query[1024];

  if (!ch || IS_NPC(ch) || GET_IDNUM(ch) <= 0)
    return;

  /* Insert read records for all posts on this board that haven't been read */
  sprintf(query,
          "INSERT INTO player_board_reads (player_id, post_id, board_id, read_date) "
          "SELECT %ld, post_id, board_id, NOW() "
          "FROM mysql_board_posts "
          "WHERE board_id = %d AND deleted = FALSE "
          "AND post_id NOT IN ("
          "  SELECT post_id FROM player_board_reads WHERE player_id = %ld"
          ") "
          "ON DUPLICATE KEY UPDATE read_date = NOW()",
          GET_IDNUM(ch), board_id, GET_IDNUM(ch));

  if (mysql_board_execute_query(query))
  {
    send_to_char(ch, "All posts on this board have been marked as read.\r\n");
  }
  else
  {
    send_to_char(ch, "Error marking posts as read.\r\n");
  }
}

/*
 * Check all boards for unread posts and display to character
 */
ACMD(do_boardcheck)
{
  char query[2048];
  char board_name_buf[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int i, total_unread = 0;
  bool found_any = false;

  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs don't read bulletin boards.\r\n");
    return;
  }

  if (GET_IDNUM(ch) <= 0)
  {
    send_to_char(ch, "Error: Invalid player ID.\r\n");
    return;
  }

  send_to_char(
      ch,
      "\tY+------------------------------------------------------------------------------+\tn\r\n");
  send_to_char(
      ch, "                           \tWBoard Status Report\tn                             \r\n");
  send_to_char(
      ch,
      "\tY+------------------------------------------------------------------------------+\tn\r\n");

  /* Check each board configuration */
  for (i = 0; i < mysql_num_boards; i++)
  {
    struct mysql_board_config *board = &mysql_board_configs[i];
    char display_name[71]; /* 70 visible chars max + null terminator */

    /* Skip inactive boards */
    if (!board->active)
      continue;

    /* Check read permission */
    if (!mysql_board_can_read(ch, board))
      continue;

    /* Count unread posts on this board */
    /* For immortals (level 31+), show all boards with unread posts */
    /* For mortals, only show boards they have visited at least once */
    if (GET_LEVEL(ch) >= LVL_IMMORT)
    {
      sprintf(query,
              "SELECT COUNT(*) "
              "FROM mysql_board_posts p "
              "WHERE p.board_id = %d AND p.deleted = FALSE "
              "AND p.post_id NOT IN ("
              "  SELECT post_id FROM player_board_reads WHERE player_id = %ld"
              ")",
              board->board_id, GET_IDNUM(ch));
    }
    else
    {
      sprintf(query,
              "SELECT COUNT(*) "
              "FROM mysql_board_posts p "
              "WHERE p.board_id = %d AND p.deleted = FALSE "
              "AND p.post_id NOT IN ("
              "  SELECT post_id FROM player_board_reads WHERE player_id = %ld"
              ") "
              /* Only show boards the player has visited at least once */
              "AND EXISTS ("
              "  SELECT 1 FROM player_board_visits "
              "  WHERE player_id = %ld AND board_id = %d"
              ")",
              board->board_id, GET_IDNUM(ch), GET_IDNUM(ch), board->board_id);
    }

    result = mysql_board_execute_select(query);
    if (result)
    {
      row = mysql_fetch_row(result);
      if (row)
      {
        int unread = atoi(row[0]);
        if (unread > 0)
        {
          int visible_len = 0;
          int pad_i;
          char *p;

          found_any = true;
          total_unread += unread;

          /* Parse @ color codes in board name */
          strncpy(board_name_buf, board->board_name, sizeof(board_name_buf) - 1);
          board_name_buf[sizeof(board_name_buf) - 1] = '\0';
          parse_at(board_name_buf);

          /* Truncate board name to fit in 70 visible characters */
          visible_len = 0;
          display_name[0] = '\0';
          for (p = board_name_buf; *p && visible_len < 70;)
          {
            if (*p == '\t' && *(p + 1))
            {
              /* Copy color code */
              int len = strlen(display_name);
              if (len < 69)
              {
                display_name[len] = *p++;
                display_name[len + 1] = *p++;
                display_name[len + 2] = '\0';
              }
            }
            else
            {
              /* Copy regular character */
              int len = strlen(display_name);
              if (len < 69)
              {
                display_name[len] = *p++;
                display_name[len + 1] = '\0';
                visible_len++;
              }
            }
          }

          /* Display the board name */
          send_to_char(ch, "  %s", display_name);

          /* Pad to 70 characters visible width */
          for (pad_i = visible_len; pad_i < 70; pad_i++)
          {
            send_to_char(ch, " ");
          }

          /* Display unread count (8 chars total: " " + up to 3 digits + " NEW") */
          send_to_char(ch, " \tR%3d NEW\tn\r\n", unread);
        }
      }
      mysql_free_result(result);
    }
  }

  if (!found_any)
  {
    send_to_char(
        ch, "                     \tGNo unread posts on any boards.\tn                     \r\n");
  }
  else
  {
    send_to_char(ch, "\tY+-------------------------------------------------------------------------"
                     "-----+\tn\r\n");
    /* Display total */
    send_to_char(
        ch, "                      \tWTotal: \tY%d unread post%s\tn                       \r\n",
        total_unread, total_unread != 1 ? "s" : "");
  }

  send_to_char(
      ch,
      "\tY+------------------------------------------------------------------------------+\tn\r\n");
  send_to_char(ch, "        \tcType '\tCboard markread\tc' at a board to mark all posts as "
                   "read.\tn        \r\n");
  send_to_char(
      ch,
      "\tY+------------------------------------------------------------------------------+\tn\r\n");
}

/*
 * Find all boards in the game world
 * Shows room vnum/name or player carrying it
 * Optional case-insensitive name filter
 */
ACMD(do_boardfind)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char board_name_buf[MAX_STRING_LENGTH];
  struct obj_data *obj;
  int i, found_count = 0;
  bool has_filter = false;
  char filter_lower[MAX_INPUT_LENGTH];

  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "You do not have permission to use this command.\r\n");
    return;
  }

  /* Get optional filter argument */
  one_argument(argument, arg, sizeof(arg));
  if (*arg)
  {
    has_filter = true;
    /* Convert filter to lowercase for case-insensitive comparison */
    for (i = 0; arg[i] && i < (int)sizeof(filter_lower) - 1; i++)
    {
      filter_lower[i] = LOWER(arg[i]);
    }
    filter_lower[i] = '\0';
  }

  send_to_char(ch, "\tY+---------------------------------------------------------------------------"
                   "-----------------------+\tn\r\n");
  if (has_filter)
  {
    /* "Boards Matching: <filter>" - center it in 100 chars */
    char title_buf[120];
    snprintf(title_buf, sizeof(title_buf), "\tWBoards Matching: \tC%.40s\tn", arg);
    send_to_char(ch, "                                  %s                                   \r\n",
                 title_buf);
  }
  else
  {
    send_to_char(ch, "                                     \tWAll Boards in Game\tn                "
                     "                      \r\n");
  }
  send_to_char(ch, "\tY+---------------------------------------------------------------------------"
                   "-----------------------+\tn\r\n");
  send_to_char(ch, "\tCBoard Name                                        Location\tn               "
                   "                       \r\n");
  send_to_char(ch, "\tY+---------------------------------------------------------------------------"
                   "-----------------------+\tn\r\n");

  /* Search through all loaded boards in configuration */
  for (i = 0; i < mysql_num_boards; i++)
  {
    struct mysql_board_config *board = &mysql_board_configs[i];
    bool found_board = false;
    int visible_len;
    int pad_len;
    char *p;
    char display_name[80];     /* 50 visible chars max + color codes + null */
    char display_location[80]; /* 50 visible chars max + color codes + null */

    /* Skip inactive boards */
    if (!board->active)
      continue;

    /* Check filter if provided */
    if (has_filter)
    {
      char name_lower[MAX_STRING_LENGTH];
      int j;

      /* Convert board name to lowercase */
      for (j = 0; board->board_name[j] && j < (int)sizeof(name_lower) - 1; j++)
      {
        name_lower[j] = LOWER(board->board_name[j]);
      }
      name_lower[j] = '\0';

      /* Check if filter matches */
      if (!strstr(name_lower, filter_lower))
        continue;
    }

    /* Parse @ color codes in board name */
    strncpy(board_name_buf, board->board_name, sizeof(board_name_buf) - 1);
    board_name_buf[sizeof(board_name_buf) - 1] = '\0';
    parse_at(board_name_buf);

    /* Search for this board's object in the game world */
    for (obj = object_list; obj; obj = obj->next)
    {
      if (GET_OBJ_VNUM(obj) != board->obj_vnum)
        continue;

      found_board = true;
      found_count++;

      /* Truncate board name to 50 visible characters */
      visible_len = 0;
      display_name[0] = '\0';
      for (p = board_name_buf; *p && visible_len < 50;)
      {
        if (*p == '\t' && *(p + 1))
        {
          /* Copy color code */
          int len = strlen(display_name);
          if (len < 49)
          {
            display_name[len] = *p++;
            display_name[len + 1] = *p++;
            display_name[len + 2] = '\0';
          }
        }
        else
        {
          /* Copy regular character */
          int len = strlen(display_name);
          if (len < 49)
          {
            display_name[len] = *p++;
            display_name[len + 1] = '\0';
            visible_len++;
          }
        }
      }

      /* Display board name */
      send_to_char(ch, "%s", display_name);

      /* Pad board name to 50 characters visible width */
      for (pad_len = visible_len; pad_len < 50; pad_len++)
      {
        send_to_char(ch, " ");
      }

      /* Build location string with strict truncation to 50 visible chars */
      if (obj->carried_by)
      {
        char name_truncated[36];
        strncpy(name_truncated, GET_NAME(obj->carried_by), 35);
        name_truncated[35] = '\0';
        snprintf(display_location, sizeof(display_location), "\tCCarried by: \tW%.35s\tn",
                 name_truncated);
      }
      else if (obj->worn_by)
      {
        char name_truncated[38];
        strncpy(name_truncated, GET_NAME(obj->worn_by), 37);
        name_truncated[37] = '\0';
        snprintf(display_location, sizeof(display_location), "\tCWorn by: \tW%.37s\tn",
                 name_truncated);
      }
      else if (obj->in_room != NOWHERE)
      {
        char room_truncated[33];
        strncpy(room_truncated, world[obj->in_room].name, 32);
        room_truncated[32] = '\0';
        snprintf(display_location, sizeof(display_location), "\tCRoom [\tY%5d\tC] %.32s\tn",
                 GET_ROOM_VNUM(obj->in_room), room_truncated);
      }
      else if (obj->in_obj)
      {
        if (obj->in_obj->carried_by)
        {
          char name_truncated[30];
          strncpy(name_truncated, GET_NAME(obj->in_obj->carried_by), 29);
          name_truncated[29] = '\0';
          snprintf(display_location, sizeof(display_location), "\tCIn container on: \tW%.29s\tn",
                   name_truncated);
        }
        else if (obj->in_obj->in_room != NOWHERE)
        {
          snprintf(display_location, sizeof(display_location), "\tCIn container at: [\tY%5d\tC]\tn",
                   GET_ROOM_VNUM(obj->in_obj->in_room));
        }
        else
        {
          snprintf(display_location, sizeof(display_location), "\tRIn container (unknown loc)\tn");
        }
      }
      else
      {
        snprintf(display_location, sizeof(display_location), "\tR(Unknown location)\tn");
      }

      /* Display location */
      send_to_char(ch, "%s", display_location);

      /* Calculate visible length of location (excluding color codes) */
      visible_len = 0;
      for (p = display_location; *p; p++)
      {
        if (*p == '\t' && *(p + 1))
        {
          p++; /* skip color code */
        }
        else
        {
          visible_len++;
        }
      }

      /* Pad location to exactly 50 visible characters */
      for (pad_len = visible_len; pad_len < 50; pad_len++)
      {
        send_to_char(ch, " ");
      }

      send_to_char(ch, "\r\n");
    }

    /* If board is configured but not found in game, note it */
    if (!found_board)
    {
      /* Truncate board name to 50 visible characters */
      visible_len = 0;
      display_name[0] = '\0';
      for (p = board_name_buf; *p && visible_len < 50;)
      {
        if (*p == '\t' && *(p + 1))
        {
          /* Copy color code */
          int len = strlen(display_name);
          if (len < 49)
          {
            display_name[len] = *p++;
            display_name[len + 1] = *p++;
            display_name[len + 2] = '\0';
          }
        }
        else
        {
          /* Copy regular character */
          int len = strlen(display_name);
          if (len < 49)
          {
            display_name[len] = *p++;
            display_name[len + 1] = '\0';
            visible_len++;
          }
        }
      }

      send_to_char(ch, "%s", display_name);

      /* Pad board name to 50 characters */
      for (pad_len = visible_len; pad_len < 50; pad_len++)
      {
        send_to_char(ch, " ");
      }

      snprintf(display_location, sizeof(display_location), "\tR(Not loaded - vnum %d)\tn",
               board->obj_vnum);
      send_to_char(ch, "%s", display_location);

      /* Calculate visible length of location */
      visible_len = 0;
      for (p = display_location; *p; p++)
      {
        if (*p == '\t' && *(p + 1))
        {
          p++; /* skip color code */
        }
        else
        {
          visible_len++;
        }
      }

      /* Pad to exactly 50 visible characters */
      for (pad_len = visible_len; pad_len < 50; pad_len++)
      {
        send_to_char(ch, " ");
      }

      send_to_char(ch, "\r\n");
      found_count++;
    }
  }

  if (found_count == 0)
  {
    if (has_filter)
    {
      send_to_char(ch, "                              \tRNo boards found matching that filter.\tn  "
                       "                         \r\n");
    }
    else
    {
      send_to_char(ch, "                                      \tRNo boards found.\tn               "
                       "                        \r\n");
    }
  }

  send_to_char(ch, "\tY+---------------------------------------------------------------------------"
                   "-----------------------+\tn\r\n");
  sprintf(buf,
          "                                   \tWTotal: \tY%d board%s found\tn                     "
          "               \r\n",
          found_count, found_count != 1 ? "s" : "");
  send_to_char(ch, "%s", buf);
  send_to_char(ch, "\tY+---------------------------------------------------------------------------"
                   "-----------------------+\tn\r\n");
}
