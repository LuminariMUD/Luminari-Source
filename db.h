/**
* @file db.h                                       LuminariMUD
* Header file for database handling.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
*/
#ifndef _DB_H_
#define _DB_H_

#include "conf.h"    /* for CIRCLE_ defines */
#include "bool.h"    /* for bool */
#include "structs.h" /* for room_vnum */
#include "utils.h"   /* for ACMD */

/* used for determining how many harvesting nodes appear in the game for craft system */
#define NUM_HARVEST_NODE_RESETS 30

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD 0
#define DB_BOOT_MOB 1
#define DB_BOOT_OBJ 2
#define DB_BOOT_ZON 3
#define DB_BOOT_SHP 4
#define DB_BOOT_HLP 5
#define DB_BOOT_TRG 6
#define DB_BOOT_QST 7
#define DB_BOOT_HLQST 8

#if defined(CIRCLE_MACINTOSH)
#define LIB_WORLD ":world:"
#define LIB_TEXT ":text:"
#define LIB_TEXT_HELP ":text:help:"
#define LIB_MISC ":misc:"
#define LIB_ETC ":etc:"
#define LIB_PLRTEXT ":plrtext:"
#define LIB_PLROBJS ":plrobjs:"
#define LIB_PLRVARS ":plrvars:"
#define LIB_PLRFILES ":plrfiles:"
#define LIB_HOUSE ":house:"
#define SLASH ":"
#elif defined(CIRCLE_AMIGA) || defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS) || defined(CIRCLE_ACORN) || defined(CIRCLE_VMS)
#define LIB_WORLD "world/"
#define LIB_TEXT "text/"
#define LIB_TEXT_HELP "text/help/"
#define LIB_MISC "misc/"
#define LIB_ETC "etc/"
#define LIB_PLRTEXT "plrtext/"
#define LIB_PLROBJS "plrobjs/"
#define LIB_PLRVARS "plrvars/"
#define LIB_HOUSE "house/"
#define LIB_PLRFILES "plrfiles/"
#define SLASH "/"
#else
#error "Unknown path components."
#endif

#define SUF_OBJS "objs"
#define SUF_TEXT "text"
#define SUF_MEM "mem"
#define SUF_PLR "plr"

#if defined(CIRCLE_AMIGA)
#define EXE_FILE "/bin/circle"         /* maybe use argv[0] but it's not reliable */
#define KILLSCRIPT_FILE "/.killscript" /* autorun: shut mud down       */
#define PAUSE_FILE "/pause"            /* autorun: don't restart mud   */
#elif defined(CIRCLE_MACINTOSH)
#define EXE_FILE "::bin:circle"         /* maybe use argv[0] but it's not reliable */
#define FASTBOOT_FILE "::.fastboot"     /* autorun: boot without sleep	*/
#define KILLSCRIPT_FILE "::.killscript" /* autorun: shut mud down	*/
#define PAUSE_FILE "::pause"            /* autorun: don't restart mud	*/
#else
#define EXE_FILE "bin/circle"            /* maybe use argv[0] but it's not reliable */
#define FASTBOOT_FILE "../.fastboot"     /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript" /* autorun: shut mud down       */
#define PAUSE_FILE "../pause"            /* autorun: don't restart mud   */
#endif

/* names of various files and directories */
#define INDEX_FILE "index"                 /* index of world files		*/
#define MINDEX_FILE "index.mini"           /* ... and for mini-mud-mode	*/
#define WLD_PREFIX LIB_WORLD "wld" SLASH   /* room definitions	*/
#define MOB_PREFIX LIB_WORLD "mob" SLASH   /* monster prototypes	*/
#define OBJ_PREFIX LIB_WORLD "obj" SLASH   /* object prototypes	*/
#define ZON_PREFIX LIB_WORLD "zon" SLASH   /* zon defs & command tables */
#define SHP_PREFIX LIB_WORLD "shp" SLASH   /* shop definitions	*/
#define TRG_PREFIX LIB_WORLD "trg" SLASH   /* trigger files	*/
#define HLP_PREFIX LIB_TEXT "help" SLASH   /* Help files           */
#define QST_PREFIX LIB_WORLD "qst" SLASH   /* quest files          */
#define HLQST_PREFIX LIB_WORLD "hlq" SLASH /* quest files (homeland-port) */

#define CREDITS_FILE LIB_TEXT "credits"       /* for the 'credits' command	*/
#define NEWS_FILE LIB_TEXT "news"             /* for the 'news' command	*/
#define MOTD_FILE LIB_TEXT "motd"             /* messages of the day / mortal	*/
#define IMOTD_FILE LIB_TEXT "imotd"           /* messages of the day / immort	*/
#define GREETINGS_FILE LIB_TEXT "greetings"   /* The opening screen.	*/
#define HELP_PAGE_FILE LIB_TEXT_HELP "help"   /* for HELP <CR>	*/
#define IHELP_PAGE_FILE LIB_TEXT_HELP "ihelp" /* for HELP <CR> imms   */
#define INFO_FILE LIB_TEXT "info"             /* for INFO		*/
#define WIZLIST_FILE LIB_TEXT "wizlist"       /* for WIZLIST		*/
#define IMMLIST_FILE LIB_TEXT "immlist"       /* for IMMLIST		*/
#define BACKGROUND_FILE LIB_TEXT "background" /* for the background story	*/
#define POLICIES_FILE LIB_TEXT "policies"     /* player policies/rules	*/
#define HANDBOOK_FILE LIB_TEXT "handbook"     /* handbook for new immorts	*/
#define HELP_FILE "help.hlp"

#define IDEAS_FILE LIB_MISC "ideas"             /* for the 'idea'-command	*/
#define TYPOS_FILE LIB_MISC "typos"             /*         'typo'		*/
#define BUGS_FILE LIB_MISC "bugs"               /*         'bug'		*/
#define MESS_FILE LIB_MISC "messages"           /* damage messages		*/
#define SOCMESS_FILE LIB_MISC "socials"         /* messages for social acts	*/
#define SOCMESS_FILE_NEW LIB_MISC "socials.new" /* messages for social acts with aedit patch*/
#define XNAME_FILE LIB_MISC "xnames"            /* invalid name substrings	*/

/* BEGIN: Assumed default locations for logfiles, mainly used in do_file. */
/**/
#define SYSLOG_LOGFILE "../syslog"
#define CRASH_LOGFILE "../syslog.CRASH"
#define PREFIX_LOGFILE "../log/"
#define LEVELS_LOGFILE PREFIX_LOGFILE "levels"
#define RIP_LOGFILE PREFIX_LOGFILE "rip"
#define NEWPLAYERS_LOGFILE PREFIX_LOGFILE "newplayers"
#define RENTGONE_LOGFILE PREFIX_LOGFILE "rentgone"
#define ERRORS_LOGFILE PREFIX_LOGFILE "errors"
#define STAFFCMDS_LOGFILE PREFIX_LOGFILE "godcmds"
#define HELP_LOGFILE PREFIX_LOGFILE "help"
#define DELETES_LOGFILE PREFIX_LOGFILE "delete"
#define RESTARTS_LOGFILE PREFIX_LOGFILE "restarts"
#define USAGE_LOGFILE PREFIX_LOGFILE "usage"
#define BADPWS_LOGFILE PREFIX_LOGFILE "badpws"
#define OLC_LOGFILE PREFIX_LOGFILE "olc"
#define TRIGGER_LOGFILE PREFIX_LOGFILE "trigger"
/**/
/* END: Assumed default locations for logfiles, mainly used in do_file. */

#define CONFIG_FILE LIB_ETC "config"                          /* OasisOLC * GAME CONFIG FL */
#define PLAYER_FILE LIB_ETC "players"                         /* the player database	*/
#define MAIL_FILE LIB_ETC "plrmail"                           /* for the mudmail system	*/
#define MAIL_FILE_TMP LIB_ETC "plrmail_tmp"                   /* for the mudmail system	*/
#define BAN_FILE LIB_ETC "badsites"                           /* for the siteban system	*/
#define HCONTROL_FILE LIB_ETC "hcontrol"                      /* for the house system	*/
#define TIME_FILE LIB_ETC "time"                              /* for calendar system	*/
#define CRAFT_FILE LIB_ETC "crafts" /* for crafting system */ /* NewCraft */
#define CHANGE_LOG_FILE "../changelog"                        /* for the changelog         */
#define CLAN_FILE LIB_ETC "clans"                             /* for clan system */
#define CLAIMS_FILE LIB_ETC "claims"                          /* for clan zone claims */

/* new bitvector data for use in player_index_element */
#define PINDEX_DELETED (1 << 0)    /* deleted player	*/
#define PINDEX_NODELETE (1 << 1)   /* protected player	*/
#define PINDEX_SELFDELETE (1 << 2) /* player is selfdeleting*/
#define PINDEX_NOWIZLIST (1 << 3)  /* Player shouldn't be on wizlist*/
#define PINDEX_INCLAN (1 << 4)     /* player is in a clan */

#define REAL 0
#define VIRTUAL 1

/* Ils: how many reset commands to maintain in the queue */
#define RQ_MAXSIZE 127

/* structure for the reset commands */
struct reset_com
{
   char command; /* current command                      */

   signed char if_flag; /* if TRUE: exe only if preceding exe'd */
   int arg1;            /*                                      */
   int arg2;            /* Arguments to the command             */
   int arg3;            /*                                      */
   int arg4;            /* probability of command executing     */
   int line;            /* line number this command appears on  */
   char *sarg1;         /* string argument                      */
   char *sarg2;         /* string argument                      */

   /* Commands:
    *  'M': Read a mobile
    *  'O': Read an object
    *  'G': Give obj to mob
    *  'P': Put obj in obj
    *  'G': Obj to char
    *  'E': Obj to char equip
    *  'D': Set state of door
    *  'T': Trigger command
    *  'V': Assign a variable */
};

/* zone definition structure. for the 'zone-table'   */
struct zone_data
{
   char *name;     /* name of this zone                  */
   char *builders; /* namelist of builders allowed to    */
                   /* modify this zone.		  */
   int lifespan;   /* how long between resets (minutes)  */
   int age;        /* current age of this zone (minutes) */
   room_vnum bot;  /* starting room number for this zone */
   room_vnum top;  /* upper limit for rooms in this zone */

   int zone_flags[ZN_ARRAY_MAX]; /* Zone Flags bitvector */
   int min_level;                /* Minimum level a player must be to enter this zone */
   int max_level;                /* Maximum level a player must be to enter this zone */

   int reset_mode;        /* conditions for reset (see below)   */
   zone_vnum number;      /* virtual number of this zone	  */
   struct reset_com *cmd; /* command table for reset	          */

   int show_weather;

   int region;
   int city;
   int faction;

   /* Zone reset state - for preventing race conditions */
   int reset_state;     /* 0 = normal, 1 = resetting */
   time_t reset_start;  /* When reset started (for timeout detection) */

   /* Reset mode:
    *   0: Don't reset, and don't update age.
    *   1: Reset if no PC's are located in zone.
    *   2: Just reset. */
};

/* for queueing zones for update   */
struct reset_q_element
{
   zone_rnum zone_to_reset; /* ref to zone_data */
   struct reset_q_element *next;
};

/* structure for the update queue     */
struct reset_q_type
{
   struct reset_q_element *head;
   struct reset_q_element *tail;
};

/* Added level, flags, and last, primarily for pfile autocleaning.  You can also
 * use them to keep online statistics, and add race, class, etc if you like. */
struct player_index_element
{
   char *name;
   long id;
   int level;
   int flags;
   time_t last;
   int clan;
};

struct help_index_element
{
   char *index;    /*Future Use */
   char *keywords; /*Keyword Place holder and sorter */
   char *entry;    /*Entries for help files with Keywords at very top*/
   int duplicate;  /*Duplicate entries for multple keywords*/
   int min_level;  /*Min Level to read help entry*/
};

/* The ban defines and structs were moved to ban.h */

/* for the "buffered" rent and house object loading */
struct obj_save_data_t
{
   struct obj_data *obj;
   int locate;
   struct obj_save_data_t *next;
   int db_idnum;
};
typedef struct obj_save_data_t obj_save_data;

/* public procedures in db.c */
void set_db_happy_hour(int status);
void boot_db(void);
void destroy_db(void);
char *fread_action(FILE *fl, int nr);
int create_entry(char *name);
void zone_update(void);
char *fread_string(FILE *fl, const char *error);
char *fread_clean_string(FILE *fl, const char *error);
int fread_number(FILE *fp);
char *fread_line(FILE *fp);
int fread_flags(FILE *fp, int *fg, int fg_size);
char *fread_word(FILE *fp);
void fread_to_eol(FILE *fp);
long get_id_by_name(const char *name);
char *get_name_by_id(long id);
void save_mud_time(struct time_info_data *when);
void free_text_files(void);
void free_help_table(void);
void free_player_index(void);
void load_help(FILE *fl, char *name);
void new_mobile_data(struct char_data *ch);

zone_rnum real_zone(zone_vnum vnum);
room_rnum real_room(room_vnum vnum);
mob_rnum real_mobile(mob_vnum vnum);
obj_rnum real_object(obj_vnum vnum);
region_rnum real_region(region_vnum vnum);
path_rnum real_path(path_vnum vnum);

/* Public Procedures from objsave.c */
void Crash_save_all(void);
void Crash_idlesave(struct char_data *ch);
void Crash_crashsave(struct char_data *ch);
int Crash_load(struct char_data *ch);
void Crash_listrent(struct char_data *ch, char *name);
int Crash_clean_file(char *name);
int Crash_delete_crashfile(struct char_data *ch);
int Crash_delete_file(char *name);
void update_obj_file(void);
void Crash_rentsave(struct char_data *ch, int cost);
obj_save_data *objsave_parse_objects(FILE *fl);
obj_save_data *objsave_parse_objects_db(char *name, room_vnum house_vnum);
int objsave_save_obj_record(struct obj_data *obj, struct char_data *ch, FILE *fl, int location);
int objsave_save_obj_record_db(struct obj_data *obj, struct char_data *ch, room_vnum house_vnum, FILE *fl, int location);
/* Special functions */
SPECIAL_DECL(receptionist);
SPECIAL_DECL(cryogenicist);

/* Functions from players.c */
void tag_argument(char *argument, char *tag);
int load_char(const char *name, struct char_data *ch);
void save_char(struct char_data *ch, int mode);
void init_char(struct char_data *ch);
struct char_data *create_char(void);
struct char_data *read_mobile(mob_vnum nr, int type);
int vnum_mobile(char *searchname, struct char_data *ch);
void clear_char(struct char_data *ch);
void reset_char(struct char_data *ch);
void free_char(struct char_data *ch);
void save_player_index(void);
long get_ptable_by_name(const char *name);
void remove_player(int pfilepos);
void clean_pfiles(void);
void build_player_index(void);

struct obj_data *create_obj(void);
void clear_object(struct obj_data *obj);
void free_obj(struct obj_data *obj);
void free_obj_special_abilities(struct obj_special_ability *list);
struct obj_data *read_object(obj_vnum nr, int type);
int vnum_object(char *searchname, struct char_data *ch);
int vnum_room(char *, struct char_data *);
int vnum_trig(char *, struct char_data *);

/* Object rnum hash table functions */
void init_obj_rnum_hash(void);
void add_obj_to_rnum_hash(struct obj_data *obj);
void remove_obj_from_rnum_hash(struct obj_data *obj);

void setup_dir(FILE *fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE *fl, int mode, char *filename);
void parse_room(FILE *fl, int virtual_nr);
void parse_mobile(FILE *mob_f, int nr);
const char *parse_object(FILE *obj_f, int nr);
int is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
void reboot_wizlists(void);
void boot_world(void);
int count_hash_records(FILE *fl);
bitvector_t asciiflag_conv(const char *flag);
void renum_world(void);
void load_config(void);
struct char_data *new_char();

/* global buffering system */

/* Object rnum hash table for fast lookups */
#define OBJ_RNUM_HASH_SIZE 1024
struct obj_rnum_hash_bucket {
  struct obj_data *objs;  /* Head of linked list of objects with this rnum */
};

#ifndef __DB_C__

/* Various Files */
extern char *credits;
extern char *news;
extern char *motd;
extern char *imotd;
extern char *GREETINGS;
extern char *help;
extern char *ihelp;
extern char *info;
extern char *wizlist;
extern char *immlist;
extern char *background;
extern char *handbook;
extern char *policies;
extern char *bugs;
extern char *typos;
extern char *ideas;

/* The ingame helpfile */
extern int top_of_helpt;
extern struct help_index_element *help_table;

/* Mud configurable variables */
extern int no_mail;
extern int mini_mud;
extern int no_rent_check;
extern time_t boot_time;
extern int circle_restrict;
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;

extern struct config_data config_info;

extern struct time_info_data time_info;
extern struct weather_data weather_info;
extern struct player_special_data dummy_mob;
extern struct reset_q_type reset_q;

extern struct room_data *world;
extern room_rnum top_of_world;

/* Bounds checking macros for world array access */
#define VALID_ROOM_RNUM(rnum) ((rnum) >= 0 && (rnum) <= top_of_world)
#define GET_ROOM(rnum) (VALID_ROOM_RNUM(rnum) ? &world[rnum] : NULL)

extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;

extern struct region_data *region_table;
extern region_rnum top_of_region_table;

extern struct path_data *path_table;
extern path_rnum top_of_path_table;

extern struct raff_node *raff_list; // list of room affections

extern struct char_data *character_list;

extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern mob_rnum top_of_mobt;

extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;
extern obj_rnum top_of_objt;

/* Object rnum hash table extern declaration */
extern struct obj_rnum_hash_bucket obj_rnum_hash[OBJ_RNUM_HASH_SIZE];

extern struct social_messg *soc_mess_list;
extern int top_of_socialt;

extern struct shop_data *shop_index;
extern int top_shop;

extern struct index_data **trig_index;
extern struct trig_data *trigger_list;
extern int top_of_trigt;
extern long max_mob_id;
extern long max_obj_id;
extern int dg_owner_purged;

extern struct message_list fight_messages[MAX_MESSAGES];

/* autoquest globals */
extern struct aq_data *aquest_table;
extern qst_rnum total_quests;

/* Happyhour global */
extern struct happyhour happy_data;

/* Staff Ran Event global */
extern struct staffevent_struct staffevent_data;

/* begin previously located in players.c, returned to db.c */
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int top_of_p_file;
extern long top_idnum;
/* end previously located in players.c */

#endif /* __DB_C__ */

#endif /* _DB_H_ */
