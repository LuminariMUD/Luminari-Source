/* 
 * File:   help.h
 * Author: Ornir (Jamie McLaughlin)
 *
 * Created on 25. september 2014, 10:26
 */

#ifndef HELP_H
#define HELP_H

/* Data structure to hold a keyword list
 * for help entries for both display and storage. */
struct help_keyword_list
{
  char *tag;
  char *keyword;

  struct help_keyword_list *next;
};

/* Data structure to hold a list of help entries.
 * This is used whenever we are retreiving help entry data
 * from the database, and is also used by the oasis OLC
 * HEDIT. */
struct help_entry_list
{
  char *tag;
  char *keywords; /* Comma seperated list of keywords, used in the help display. */
  char *entry;
  int min_level;
  char *last_updated;

  /* Structure to hold keyword data */
  struct help_keyword_list *keyword_list;

  struct help_entry_list *next;
};

/* This is the MAIN search function - all help requests go through 
 * this function. */
struct help_entry_list *search_help(const char *argument, int level);
struct help_keyword_list *get_help_keywords(const char *tag);

/* Used during character creation, does not show all of the header information
 * shown by the do_help function, as players do not have access to the entire
 * help system during character creation. */
void perform_help(struct descriptor_data *d, const char *argument);

/* Help command, used in game. */
ACMD_DECL(do_help);

/* Chain of Responsibility Pattern for Help System
 * This design eliminates deep nesting and makes the help system extensible.
 * Each handler is responsible for one type of help content. */

/* Function pointer type for help handlers
 * Returns 1 if help was displayed, 0 if not handled */
typedef int (*help_handler_func)(struct char_data *ch, const char *argument, const char *raw_argument);

/* Structure for a help handler in the chain */
struct help_handler {
    const char *name;           /* Handler name for debugging/logging */
    help_handler_func handler;  /* Function that processes help requests */
    struct help_handler *next;  /* Next handler in the chain */
};

/* Handler registration and management */
void register_help_handler(const char *name, help_handler_func handler);
void init_help_handlers(void);
void cleanup_help_handlers(void);

/* Individual help handlers - each handles one type of help content */
int handle_database_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_deity_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_region_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_background_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_discovery_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_grand_discovery_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_bomb_types_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_discovery_types_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_feat_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_evolution_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_weapon_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_armor_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_class_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_race_help(struct char_data *ch, const char *argument, const char *raw_argument);
int handle_soundex_suggestions(struct char_data *ch, const char *argument, const char *raw_argument);

#endif /* HELP_H */
