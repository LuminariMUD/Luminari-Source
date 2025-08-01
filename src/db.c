/**************************************************************************
 *  File: db.c                                         Part of LuminariMUD *
 *  Usage: Loading/saving chars, booting/resetting world, internal funcs.  *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#define __DB_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "dg_event.h"
#include "act.h"
#include "ban.h"
#include "treasure.h"
#include "spec_procs.h"
#include "genzon.h"
#include "genolc.h"
#include "genobj.h" /* for free_object_strings */
#include "genwld.h" /* for free_trail_data_list */
#include "config.h" /* for the default config values. */
#include "fight.h"
#include "modify.h"
#include "shop.h"
#include "quest.h"
#include "ibt.h"
#include "mud_event.h"
#include "class.h"
#include "clan.h"
#include "clan_economy.h"
#include "msgedit.h"
#include "craft.h"
#include "hlquest.h"
#include "mudlim.h"
#include "spec_abilities.h"
#include "perlin.h"
#include "wilderness.h"
#include "mysql.h"
#include "feats.h"
#include "actionqueues.h"
#include "domains_schools.h"
#include "grapple.h"
#include "race.h"
#include "spell_prep.h"
#include "crafts.h" /* NewCraft */
#include <sys/stat.h>
#include "trails.h"
#include "premadebuilds.h"
#include "encounters.h"
#include "hunts.h"
#include "evolutions.h"
#include "treasure.h"
#include "assign_wpn_armor.h"
#include "backgrounds.h"
#include "crafting_new.h"
#include "crafting_recipes.h"

/*  declarations of most of the 'global' variables */
struct config_data config_info; /* Game configuration list.	 */

struct room_data *world = NULL; /* array of rooms		 */
room_rnum top_of_world = 0;     /* ref to top element of world	 */

struct raff_node *raff_list = NULL; // list of room affections

struct char_data *character_list = NULL; /* global linked list of chars	*/
struct index_data *mob_index = NULL;     /* index table for mobile file	 */
struct char_data *mob_proto = NULL;      /* prototypes for mobs		 */
mob_rnum top_of_mobt = 0;                /* top of mobile index table	 */

struct obj_data *object_list = NULL; /* global linked list of objs	*/
struct index_data *obj_index = NULL; /* index table for object file	 */
struct obj_data *obj_proto = NULL;   /* prototypes for objs		 */
obj_rnum top_of_objt = 0;            /* top of object index table	 */

/* Object rnum hash table for fast lookups */
struct obj_rnum_hash_bucket obj_rnum_hash[OBJ_RNUM_HASH_SIZE];

struct zone_data *zone_table = NULL; /* zone table      */
zone_rnum top_of_zone_table = 0;     /* top element of zone tab   */

struct region_data *region_table = NULL; /* Region table */
region_rnum top_of_region_table = 0;     /* top element of region tab */

struct path_data *path_table = NULL; /* Path table */
path_rnum top_of_path_table = 0;     /* top element of path tab */

/* begin previously located in players.c */
struct player_index_element *player_table = NULL; /* index to plr file   */
int top_of_p_table = 0;                           /* ref to top of table     */
int top_of_p_file = 0;                            /* ref of size of p file   */
long top_idnum = 0;                               /* highest idnum in use    */
/* end previously located in players.c */

struct message_list fight_messages[MAX_MESSAGES]; /* fighting messages	 */

struct index_data **trig_index = NULL; /* index table for triggers      */
struct trig_data *trigger_list = NULL; /* all attached triggers */
int top_of_trigt = 0;                  /* top of trigger index table    */
long max_mob_id = MOB_ID_BASE;         /* for unique mob id's           */
long max_obj_id = OBJ_ID_BASE;         /* for unique obj id's           */
int dg_owner_purged = 0;               /* For control of scripts        */

struct aq_data *aquest_table = NULL; /* Autoquests table              */
qst_rnum total_quests = 0;           /* top of autoquest table        */

struct shop_data *shop_index = NULL; /* index table for shops         */
int top_shop = -1;                   /* top of shop table             */

int no_mail = 0;                   /* mail disabled?		 */
int mini_mud = 0;                  /* mini-mud mode?		 */
int no_rent_check = 0;             /* skip rent check on boot?	 */
time_t boot_time = 0;              /* time of mud boot		 */
int circle_restrict = 0;           /* level of game restriction	 */
room_rnum r_mortal_start_room = 0; /* rnum of mortal start room	 */
room_rnum r_immort_start_room = 0; /* rnum of immort start room	 */
room_rnum r_frozen_start_room = 0; /* rnum of frozen start room	 */

char *credits = NULL;    /* game credits			 */
char *news = NULL;       /* mud news			 */
char *motd = NULL;       /* message of the day - mortals  */
char *imotd = NULL;      /* message of the day - immorts  */
char *GREETINGS = NULL;  /* opening credits screen        */
char *help = NULL;       /* help screen			 */
char *ihelp = NULL;      /* help screen (immortals)       */
char *info = NULL;       /* info page			 */
char *wizlist = NULL;    /* list of higher gods		 */
char *immlist = NULL;    /* list of peon gods		 */
char *background = NULL; /* background story		 */
char *handbook = NULL;   /* handbook for new immortals	 */
char *policies = NULL;   /* policies page		 */
char *bugs = NULL;       /* bugs file                     */
char *typos = NULL;      /* typos file                    */
char *ideas = NULL;      /* ideas file                    */

int top_of_helpt = 0;
struct help_index_element *help_table = NULL;

struct social_messg *soc_mess_list = NULL; /* list of socials */
int top_of_socialt = -1;                   /* number of socials */

time_t newsmod = 0; /* Time news file was last modified. */
time_t motdmod = 0; /* Time motd file was last modified. */

struct time_info_data time_info;      /* the infomation about the time    */
struct weather_data weather_info;     /* the infomation about the weather */
struct player_special_data dummy_mob; /* dummy spec area for mobs	*/
struct reset_q_type reset_q;          /* queue of zones to be reset	 */

struct staffevent_struct staffevent_data = {-1, 0, 3}; /* first value is event index which starts with 0, -1 means no event */
struct happyhour happy_data = {0, 0, 0, 0, 0};

/* declaration of local (file scope) variables */
static int converting = FALSE;

int weighted_object_bonuses[NUM_ITEM_WEARS][NUM_APPLIES];

/* Local (file scope) utility functions */
static int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits);
static int check_object_spell_number(struct obj_data *obj, int val);
static int check_object_level(struct obj_data *obj, int val);
static int check_object(struct obj_data *);
static void load_zones(FILE *fl, char *zonename);
static int file_to_string(const char *name, char *buf);
static int file_to_string_alloc(const char *name, char **buf);
static int count_alias_records(FILE *fl);
static void parse_simple_mob(FILE *mob_f, int i, int nr);
static void interpret_espec(const char *keyword, const char *value, int i, int nr);
static void parse_espec(char *buf, int i, int nr);
static void parse_enhanced_mob(FILE *mob_f, int i, int nr);
static void get_one_line(FILE *fl, char *buf);
static void check_start_rooms(void);
static void renum_zone_table(void);
static void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
static void reset_time(void);
static char fread_letter(FILE *fp);
static void free_followers(struct follow_type *k);
static void load_default_config(void);
static void free_extra_descriptions(struct extra_descr_data *edesc);
static bitvector_t asciiflag_conv_aff(char *flag);
static int help_sort(const void *a, const void *b);
void assign_deities(void);
void set_armor_object(struct obj_data *obj, int type);
SPECIAL_DECL(moving_rooms);

void assign_weighted_bonuses(void);

/* Ils: Global result_q needed for init_result_q, push_result & test_result */
struct
{
  int tail, size;
  bool q[RQ_MAXSIZE];
} result_q;

/* init_result_q
 * note: just prepares the result_q for usage */
void init_result_q(void)
{
  result_q.size = 0;
  result_q.tail = 0;
}

/* push_result
 * Note: will push result into the result_q at head of queue.
 * Queue will be treated as a fixed sized circular list, of 127 entries,
 * new entries will overwrite the oldest entries after 127 results have
 * been enqueued.
 * note: result_q.tail is kept 1 ahead of valid results so that
 *       result_q.tail - 1 = previous result */
void push_result(byte result)
{
  result_q.q[result_q.tail] = result;
  result_q.size++;
  if (result_q.size > RQ_MAXSIZE)
    result_q.size = RQ_MAXSIZE;
  result_q.tail = (result_q.tail + 1) % RQ_MAXSIZE;
}

/* test_result
 * returns - if offset is positive
 *             TRUE if result stored at tail - abs(offset) is TRUE
 *             FALSE if result stored at tail - abs(offset) is FALSE
 *         - if offset is negative
 *             FALSE if result stored at tail - abs(offset) is TRUE
 *             TRUE if result stored at tail - abs(offset) is FALSE
 *         - if offset is zero return TRUE
 * note: a zero offset indicates the command does not depend on previous results
 *       so it always returns TRUE.
 * note: does not remove the value from the queue
 * using a fixed sized queue as a way to make a circular list
 * usage: TRUE should mean execute the command
 *        FALSE should mean don't execute the command
 * NOTE: Uses the ZONE_ERROR macro defined for reset_zone */
sbyte test_result(sbyte offset)
{
  if (abs(offset) > result_q.size)
  {
    log("ERR: out of bounds if-value in zone reset [offset:%d > result_size:%d]", abs(offset), result_q.size);
    return FALSE;
  }

  if (offset == 0)
    return TRUE;

  else if (offset > 0)
    return (result_q.q[((RQ_MAXSIZE - (abs(offset) - result_q.tail)) % RQ_MAXSIZE)]);
  else
    return !(result_q.q[((RQ_MAXSIZE - (abs(offset) - result_q.tail)) % RQ_MAXSIZE)]);
}

/* routines for booting the system */
char *fread_action(FILE *fl, int nr)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  char *buf1 = NULL;
  int i = 0;

  buf1 = fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl))
  {
    log("SYSERR: fread_action: unexpected EOF near action #%d", nr);
    /* SYSERR_DESC: fread_action() will fail if it discovers an end of file
     * marker before it is able to read in the expected string.  This can be
     * caused by a truncated socials file. */
    exit(1);
  }
  if (*buf == '#')
    return (NULL);

  parse_at(buf);

  /* Some clients interpret '\r' the same as { '\r' '\n' }, so the original way of just
     replacing '\n' with '\0' would appear as 2 new lines following the action */
  for (i = 0; buf[i] != '\0'; i++)
    if (buf[i] == '\r' || buf[i] == '\n')
    {
      buf[i] = '\0';
      break;
    }

  return (strdup(buf));
}

void boot_social_messages(void)
{
  FILE *fl;
  int nr = 0, hide, min_char_pos, min_pos, min_lvl, curr_soc = -1, i;
  char next_soc[MAX_STRING_LENGTH] = {'\0'}, sorted[MAX_INPUT_LENGTH] = {'\0'}, *buf;

  if (CONFIG_NEW_SOCIALS == TRUE)
  {
    /* open social file */
    if (!(fl = fopen(SOCMESS_FILE_NEW, "r")))
    {
      log("SYSERR: can't open socials file '%s': %s", SOCMESS_FILE_NEW, strerror(errno));
      /* SYSERR_DESC: This error, from boot_social_messages(), occurs when the
       * server fails to open the file containing the social messages.  The
       * error at the end will indicate the reason why. */
      exit(1);
    }
    /* count socials */
    *next_soc = '\0';
    while (!feof(fl))
    {
      buf = fgets(next_soc, MAX_STRING_LENGTH, fl);
      if (*next_soc == '~')
        top_of_socialt++;
    }
  }
  else
  { /* old style */

    /* open social file */
    if (!(fl = fopen(SOCMESS_FILE, "r")))
    {
      log("SYSERR: can't open socials file '%s': %s", SOCMESS_FILE, strerror(errno));
      /* SYSERR_DESC: This error, from boot_social_messages(), occurs when the
       * server fails to open the file containing the social messages.  The
       * error at the end will indicate the reason why. */
      exit(1);
    }
    /* count socials */
    while (!feof(fl))
    {
      buf = fgets(next_soc, MAX_STRING_LENGTH, fl);
      if (*next_soc == '\n' || *next_soc == '\r')
        top_of_socialt++; /* all socials are followed by a blank line */
    }
  }

  log("Social table contains %d socials.", top_of_socialt);
  rewind(fl);

  CREATE(soc_mess_list, struct social_messg, top_of_socialt + 1);

  /* now read 'em */
  for (;;)
  {
    i = fscanf(fl, " %s ", next_soc);
    if (*next_soc == '$')
      break;
    if (CONFIG_NEW_SOCIALS == TRUE)
    {
      if (fscanf(fl, " %s %d %d %d %d \n",
                 sorted, &hide, &min_char_pos, &min_pos, &min_lvl) != 5)
      {
        log("SYSERR: format error in social file near social '%s'", next_soc);
        /* SYSERR_DESC: From boot_social_messages(), this error is output when
         * the server is expecting to find the remainder of the first line of the
         * social ('hide' and 'minimum position').  These must follow the name of
         * the social with a single space such as: 'accuse 0 5\n'. This error
         * often occurs when one of the numbers is missing or the social name has
         * a space in it (i.e., 'bend over'). */
        exit(1);
      }
      curr_soc++;
      soc_mess_list[curr_soc].command = strdup(next_soc + 1);
      soc_mess_list[curr_soc].sort_as = strdup(sorted);
      soc_mess_list[curr_soc].hide = hide;
      soc_mess_list[curr_soc].min_char_position = min_char_pos;
      soc_mess_list[curr_soc].min_victim_position = min_pos;
      soc_mess_list[curr_soc].min_level_char = min_lvl;
    }
    else
    { /* old style */
      if (fscanf(fl, " %d %d \n", &hide, &min_pos) != 2)
      {
        log("SYSERR: format error in social file near social '%s'", next_soc);
        /* SYSERR_DESC: From boot_social_messages(), this error is output when the
         * server is expecting to find the remainder of the first line of the
         * social ('hide' and 'minimum position').  These must follow the name of
         * the social with a single space such as: 'accuse 0 5\n'. This error
         * often occurs when one of the numbers is missing or the social name has
         * a space in it (i.e., 'bend over'). */
        exit(1);
      }
      curr_soc++;
      soc_mess_list[curr_soc].command = strdup(next_soc);
      soc_mess_list[curr_soc].sort_as = strdup(next_soc);
      soc_mess_list[curr_soc].hide = hide;
      soc_mess_list[curr_soc].min_char_position = POS_RESTING;
      soc_mess_list[curr_soc].min_victim_position = min_pos;
      soc_mess_list[curr_soc].min_level_char = 0;
    }

#ifdef CIRCLE_ACORN
    if (fgetc(fl) != '\n')
      log("SYSERR: Acorn bug workaround failed.");
      /* SYSERR_DESC: The only time that this error should ever arise is if you
       * are running your MUD on the Acorn platform.  The error arises when the
       * server cannot properly read a '\n' out of the file at the end of the
       * first line of the social (that with 'hide' and 'min position').  This
       * is in boot_social_messages(). */
#endif

    soc_mess_list[curr_soc].char_no_arg = fread_action(fl, nr);
    soc_mess_list[curr_soc].others_no_arg = fread_action(fl, nr);
    soc_mess_list[curr_soc].char_found = fread_action(fl, nr);

    /* if no char_found, the rest is to be ignored */
    if (CONFIG_NEW_SOCIALS == FALSE && !soc_mess_list[curr_soc].char_found)
      continue;

    soc_mess_list[curr_soc].others_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].vict_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].not_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].char_auto = fread_action(fl, nr);
    soc_mess_list[curr_soc].others_auto = fread_action(fl, nr);

    if (CONFIG_NEW_SOCIALS == FALSE)
      continue;

    soc_mess_list[curr_soc].char_body_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].others_body_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].vict_body_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].char_obj_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].others_obj_found = fread_action(fl, nr);
  }

  /* close file & set top */
  fclose(fl);
  assert(curr_soc <= top_of_socialt);
  top_of_socialt = curr_soc;
}

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}

/* Wipe out all the loaded text files, for shutting down. */
void free_text_files(void)
{
  char **textfiles[] = {
      &wizlist, &immlist, &news, &credits, &motd, &imotd, &help, &ihelp, &info,
      &policies, &handbook, &background, &GREETINGS, &bugs, &typos, &ideas, NULL};
  int rf;

  for (rf = 0; textfiles[rf]; rf++)
    if (*textfiles[rf])
    {
      free(*textfiles[rf]);
      *textfiles[rf] = NULL;
    }
}

/* Too bad it doesn't check the return values to let the user know about -1
 * values.  This will result in an 'Okay.' to a 'reload' command even when the
 * string was not replaced. To fix later. */
ACMD(do_reboot)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  one_argument(argument, arg, sizeof(arg));

  if (!str_cmp(arg, "all") || *arg == '*')
  {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
    if (file_to_string_alloc(WIZLIST_FILE, &wizlist) < 0)
      send_to_char(ch, "Cannot read wizlist\r\n");
    if (file_to_string_alloc(IMMLIST_FILE, &immlist) < 0)
      send_to_char(ch, "Cannot read immlist\r\n");
    if (file_to_string_alloc(NEWS_FILE, &news) < 0)
      send_to_char(ch, "Cannot read news\r\n");
    if (file_to_string_alloc(CREDITS_FILE, &credits) < 0)
      send_to_char(ch, "Cannot read credits\r\n");
    if (file_to_string_alloc(MOTD_FILE, &motd) < 0)
      send_to_char(ch, "Cannot read motd\r\n");
    if (file_to_string_alloc(IMOTD_FILE, &imotd) < 0)
      send_to_char(ch, "Cannot read imotd\r\n");
    if (file_to_string_alloc(HELP_PAGE_FILE, &help) < 0)
      send_to_char(ch, "Cannot read help front page\r\n");
    if (file_to_string_alloc(IHELP_PAGE_FILE, &ihelp) < 0)
      send_to_char(ch, "Cannot read help front page\r\n");
    if (file_to_string_alloc(INFO_FILE, &info) < 0)
      send_to_char(ch, "Cannot read info file\r\n");
    if (file_to_string_alloc(POLICIES_FILE, &policies) < 0)
      send_to_char(ch, "Cannot read policies\r\n");
    if (file_to_string_alloc(HANDBOOK_FILE, &handbook) < 0)
      send_to_char(ch, "Cannot read handbook\r\n");
    if (file_to_string_alloc(BACKGROUND_FILE, &background) < 0)
      send_to_char(ch, "Cannot read background\r\n");
    if (help_table)
    {
      free_help_table();
      index_boot(DB_BOOT_HLP);
    }
  }
  else if (!str_cmp(arg, "wizlist"))
  {
    if (file_to_string_alloc(WIZLIST_FILE, &wizlist) < 0)
      send_to_char(ch, "Cannot read wizlist\r\n");
  }
  else if (!str_cmp(arg, "immlist"))
  {
    if (file_to_string_alloc(IMMLIST_FILE, &immlist) < 0)
      send_to_char(ch, "Cannot read immlist\r\n");
  }
  else if (!str_cmp(arg, "news"))
  {
    if (file_to_string_alloc(NEWS_FILE, &news) < 0)
      send_to_char(ch, "Cannot read news\r\n");
  }
  else if (!str_cmp(arg, "credits"))
  {
    if (file_to_string_alloc(CREDITS_FILE, &credits) < 0)
      send_to_char(ch, "Cannot read credits\r\n");
  }
  else if (!str_cmp(arg, "motd"))
  {
    if (file_to_string_alloc(MOTD_FILE, &motd) < 0)
      send_to_char(ch, "Cannot read motd\r\n");
  }
  else if (!str_cmp(arg, "imotd"))
  {
    if (file_to_string_alloc(IMOTD_FILE, &imotd) < 0)
      send_to_char(ch, "Cannot read imotd\r\n");
  }
  else if (!str_cmp(arg, "help"))
  {
    if (file_to_string_alloc(HELP_PAGE_FILE, &help) < 0)
      send_to_char(ch, "Cannot read help front page\r\n");
  }
  else if (!str_cmp(arg, "ihelp"))
  {
    if (file_to_string_alloc(IHELP_PAGE_FILE, &ihelp) < 0)
      send_to_char(ch, "Cannot read help front page\r\n");
  }
  else if (!str_cmp(arg, "info"))
  {
    if (file_to_string_alloc(INFO_FILE, &info) < 0)
      send_to_char(ch, "Cannot read info\r\n");
  }
  else if (!str_cmp(arg, "policy"))
  {
    if (file_to_string_alloc(POLICIES_FILE, &policies) < 0)
      send_to_char(ch, "Cannot read policy\r\n");
  }
  else if (!str_cmp(arg, "handbook"))
  {
    if (file_to_string_alloc(HANDBOOK_FILE, &handbook) < 0)
      send_to_char(ch, "Cannot read handbook\r\n");
  }
  else if (!str_cmp(arg, "background"))
  {
    if (file_to_string_alloc(BACKGROUND_FILE, &background) < 0)
      send_to_char(ch, "Cannot read background\r\n");
  }
  else if (!str_cmp(arg, "greetings"))
  {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
    else
      send_to_char(ch, "Cannot read greetings.\r\n");
  }
  else if (!str_cmp(arg, "xhelp"))
  {
    if (help_table)
    {
      free_help_table();
      index_boot(DB_BOOT_HLP);
    }
  }
  else if (!str_cmp(arg, "regions"))
  {
    /* Reload wilderness regions */
    load_regions();
  }
  else if (!str_cmp(arg, "paths"))
  {
    /* Reload wilderness regions */
    load_paths();
  }
  else
  {
    send_to_char(ch, "Unknown reload option.\r\n");
    return;
  }

  send_to_char(ch, "%s", CONFIG_OK);
}

/* loads up: zone table, triggers, wilderness, regions, region-paths, renum_world(), start rooms,
     mobs, objects, renumbers zone table, auto quests, hlquests, class list, assign/sort feats,
     extended races, armor/weapon lists, domains and deities */
void boot_world(void)
{

  /* Initialize the db connection. */
  connect_to_mysql();

  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading triggers and generating index.");
  index_boot(DB_BOOT_TRG);

  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Loading regions. (MySQL)");
  load_regions();

  log("Loading paths. (MySQL)");
  load_paths();

  log("Renumbering rooms.");
  renum_world();

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  log("Renumbering zone table.");
  renum_zone_table();

  if (converting)
  {
    log("Saving 128bit world files to disk.");
    save_all();
  }

  if (!no_specials)
  {
    log("Loading shops.");
    index_boot(DB_BOOT_SHP);

#if !defined(CAMPAIGN_DL)
    int x = 0;
    log("Placing Harvesting Nodes");
    for (x = 0; x < NUM_HARVEST_NODE_RESETS; x++)
      reset_harvesting_rooms();
#endif
  }

#if defined(CAMPAIGN_DL)
  // assigning new crafting system harvesting nodes.
  assign_harvest_materials_to_word();
  populate_crafting_recipes();
  populate_refining_recipes();
  sort_materials();
#endif

  log("Loading quests.");
  index_boot(DB_BOOT_QST);

  log("Loading Homeland quests.");
  index_boot(DB_BOOT_HLQST);

  log("Loading Deities");
  assign_deities();

  log("Loading Domains.");
  assign_domains();

  log("Loading Weapons.");
  load_weapons();

  log("Loading Armor.");
  load_armor();

  log("Loading Extended Races");
  assign_races();

  /* this use to be partially dependent on classo() we had
     to modify it so there is no dependence due to inability to
     load two things at the same time :p */
  log("Loading feats.");
  assign_feats();
  sort_feats();

  /* this HAS to come after loading feats, we need feat info
     in order to handle the class list (prereqs) */
  log("Loading Class List");
  load_class_list();

  log("Loading evolutions.");
  assign_evolutions();

  // we'll sort races alphabetically
  sort_races();

  // assign backgrounds
  assign_backgrounds();
  sort_backgrounds();

  // assign object bonuses table
  assign_weighted_bonuses();

  log("Initializing perlin noise generator.");
  init_perlin(NOISE_MATERIAL_PLANE_ELEV, NOISE_MATERIAL_PLANE_ELEV_SEED);
  init_perlin(NOISE_MATERIAL_PLANE_MOISTURE, NOISE_MATERIAL_PLANE_MOISTURE_SEED);
  init_perlin(NOISE_MATERIAL_PLANE_ELEV_DIST, NOISE_MATERIAL_PLANE_ELEV_DIST_SEED);
  init_perlin(NOISE_WEATHER, NOISE_WEATHER_SEED);

#if !defined(CAMPAIGN_FR) && !defined(CAMPAIGN_DL)
  log("Indexing wilderness rooms.");
  initialize_wilderness_lists();

  log("Writing wilderness map image.");
  // save_map_to_file("luminari_wilderness.png", WILD_X_SIZE, WILD_Y_SIZE);

  // save_noise_to_file(NOISE_MATERIAL_PLANE_ELEV, "luminari_wild_noise_elev_zoom.png", WILD_X_SIZE, WILD_Y_SIZE, 0);
  // save_noise_to_file(NOISE_MATERIAL_PLANE_ELEV, "luminari_wild_noise_elev_zoom.png", WILD_X_SIZE, WILD_Y_SIZE, 1);
#endif
}

static void free_extra_descriptions(struct extra_descr_data *edesc)
{
  struct extra_descr_data *enext;

  for (; edesc; edesc = enext)
  {
    enext = edesc->next;

    free(edesc->keyword);
    free(edesc->description);
    free(edesc);
  }
}

/* Free the world, in a memory allocation sense. */
void destroy_db(void)
{
  ssize_t cnt = 0, itr = 0;
  struct char_data *chtmp = NULL;
  struct obj_data *objtmp = NULL;

  /* Active Mobiles & Players */
  /* First pass: Clear all follower relationships without messages */
  for (chtmp = character_list; chtmp; chtmp = chtmp->next)
  {
    struct follow_type *k, *k_next;
    
    /* Clear this character's master pointer */
    if (chtmp->master)
    {
      /* Don't use stop_follower() as it sends messages */
      chtmp->master = NULL;
      REMOVE_BIT_AR(AFF_FLAGS(chtmp), AFF_CHARM);
    }
    
    /* Clear all followers of this character */
    for (k = chtmp->followers; k; k = k_next)
    {
      k_next = k->next;
      if (k->follower)
      {
        k->follower->master = NULL;
        REMOVE_BIT_AR(AFF_FLAGS(k->follower), AFF_CHARM);
      }
      free(k);
    }
    chtmp->followers = NULL;
  }
  
  /* Second pass: Now free all characters */
  while (character_list)
  {
    chtmp = character_list;
    character_list = character_list->next;
    free_char(chtmp);
  }

  /* Active Objects */
  while (object_list)
  {
    objtmp = object_list;
    object_list = object_list->next;
    free_obj(objtmp);
  }

  /* Rooms */
  for (cnt = 0; cnt <= top_of_world; cnt++)
  {
    /* Skip freeing names/descriptions for wilderness rooms as they use static strings
     * Wilderness rooms are identified by their vnum range */
    if (IS_WILDERNESS_VNUM(world[cnt].number)) {
      /* Wilderness rooms use static strings that should not be freed */
    } else {
      if (world[cnt].name)
        free(world[cnt].name);
      if (world[cnt].description)
        free(world[cnt].description);
    }
    free_extra_descriptions(world[cnt].ex_description);

    /* free trail data */
    if (world[cnt].trail_tracks != NULL)
      free_trail_data_list(world[cnt].trail_tracks);

    /* freeing room events */
    if (world[cnt].events != NULL)
    {
      if (world[cnt].events->iSize > 0)
      {
        struct event *pEvent;

        while ((pEvent = simple_list(world[cnt].events)) != NULL)
          event_cancel(pEvent);
      }
      free_list(world[cnt].events);
      world[cnt].events = NULL;
    }

    /* free any assigned scripts */
    if (SCRIPT(&world[cnt]))
      extract_script(&world[cnt], WLD_TRIGGER);
    /* free script proto list */
    free_proto_script(&world[cnt], WLD_TRIGGER);

    for (itr = 0; itr < NUM_OF_DIRS; itr++)
    { /* NUM_OF_DIRS here, not DIR_COUNT */
      if (!world[cnt].dir_option[itr])
        continue;

      if (world[cnt].dir_option[itr]->general_description)
        free(world[cnt].dir_option[itr]->general_description);
      if (world[cnt].dir_option[itr]->keyword)
        free(world[cnt].dir_option[itr]->keyword);
      free(world[cnt].dir_option[itr]);
    }
  }
  free(world);
  top_of_world = 0;

  /* Objects */
  for (cnt = 0; cnt <= top_of_objt; cnt++)
  {
    if (obj_proto[cnt].name)
      free(obj_proto[cnt].name);
    if (obj_proto[cnt].description)
      free(obj_proto[cnt].description);
    if (obj_proto[cnt].short_description)
      free(obj_proto[cnt].short_description);
    if (obj_proto[cnt].action_description)
      free(obj_proto[cnt].action_description);
    free_extra_descriptions(obj_proto[cnt].ex_description);

    /* free special abilities list */
    if (obj_proto[cnt].special_abilities)
      free_obj_special_abilities(obj_proto[cnt].special_abilities);

    /* CRITICAL FIX: free spellbook info to prevent memory leak */
    if (obj_proto[cnt].sbinfo)
      free(obj_proto[cnt].sbinfo);

    /* free script proto list */
    free_proto_script(&obj_proto[cnt], OBJ_TRIGGER);
  }
  free(obj_proto);
  free(obj_index);

  /* Mobiles */
  for (cnt = 0; cnt <= top_of_mobt; cnt++)
  {
    if (mob_proto[cnt].player.name)
      free(mob_proto[cnt].player.name);
    if (mob_proto[cnt].player.title)
      free(mob_proto[cnt].player.title);
    if (mob_proto[cnt].player.short_descr)
      free(mob_proto[cnt].player.short_descr);
    if (mob_proto[cnt].player.long_descr)
      free(mob_proto[cnt].player.long_descr);
    if (mob_proto[cnt].player.description)
      free(mob_proto[cnt].player.description);
    if (mob_proto[cnt].player.walkin)
      free(mob_proto[cnt].player.walkin);
    if (mob_proto[cnt].player.walkout)
      free(mob_proto[cnt].player.walkout);

    /* free script proto list */
    free_proto_script(&mob_proto[cnt], MOB_TRIGGER);

    while (mob_proto[cnt].affected)
      affect_remove(&mob_proto[cnt], mob_proto[cnt].affected);

    /* free echo entries */
    if (ECHO_COUNT(&mob_proto[cnt]) > 0 && ECHO_ENTRIES(&mob_proto[cnt]))
    {
      int j;
      for (j = 0; j < ECHO_COUNT(&mob_proto[cnt]); j++)
        if (ECHO_ENTRIES(&mob_proto[cnt])[j])
          free(ECHO_ENTRIES(&mob_proto[cnt])[j]);
      free(ECHO_ENTRIES(&mob_proto[cnt]));
    }

    /* free quest data */
    free_hlquest(&mob_proto[cnt]);
  }
  free(mob_proto);
  free(mob_index);

  /* Shops */
  destroy_shops();

  /* Quests */
  destroy_quests();

  /* Zones */
#define THIS_CMD zone_table[cnt].cmd[itr]

  for (cnt = 0; cnt <= top_of_zone_table; cnt++)
  {
    if (zone_table[cnt].name)
      free(zone_table[cnt].name);
    if (zone_table[cnt].builders)
      free(zone_table[cnt].builders);
    if (zone_table[cnt].cmd)
    {
      /* first see if any vars were defined in this zone */
      for (itr = 0; THIS_CMD.command != 'S'; itr++)
        if (THIS_CMD.command == 'V')
        {
          if (THIS_CMD.sarg1)
            free(THIS_CMD.sarg1);
          if (THIS_CMD.sarg2)
            free(THIS_CMD.sarg2);
        }
      /* then free the command list */
      free(zone_table[cnt].cmd);
    }
  }
  free(zone_table);

#undef THIS_CMD

  /* zone table reset queue */
  if (reset_q.head)
  {
    struct reset_q_element *ftemp = reset_q.head, *temp;
    while (ftemp)
    {
      temp = ftemp->next;
      free(ftemp);
      ftemp = temp;
    }
  }

  /* Triggers */
  for (cnt = 0; cnt < top_of_trigt; cnt++)
  {
    if (trig_index[cnt]->proto)
    {
      /* make sure to nuke the command list (memory leak) */
      /* free_trigger() doesn't free the command list */
      if (trig_index[cnt]->proto->cmdlist)
      {
        struct cmdlist_element *i, *j;
        i = trig_index[cnt]->proto->cmdlist;
        while (i)
        {
          j = i->next;
          if (i->cmd)
            free(i->cmd);
          free(i);
          i = j;
        }
      }
      free_trigger(trig_index[cnt]->proto);
    }
    free(trig_index[cnt]);
  }
  free(trig_index);

  /* Craft Cleanup */
  /* Clear craft list - must be done safely without using simple_list during removal */
  if (global_craft_list->iSize > 0)
  {
    struct craft_data *craft;
    struct iterator_data iter;
    
    /* Reset simple_list state before manual iteration */
    simple_list(NULL);
    
    /* Use iterator directly to avoid corruption */
    craft = (struct craft_data *)merge_iterator(&iter, global_craft_list);
    while (craft != NULL)
    {
      struct craft_data *next_craft = (struct craft_data *)next_in_list(&iter);
      remove_from_list(craft, global_craft_list);
      free_craft(craft);
      craft = next_craft;
    }
    remove_iterator(&iter);
  }
  free_list(global_craft_list);

  /* Events */
  event_free_all();
  
  /* Clan economy system */
  shutdown_clan_economy();
}

/* body of the booting system */
void boot_db(void)
{
  zone_rnum i = 0;
  char buf1[MAX_INPUT_LENGTH] = {'\0'}; /* strip color off zone names */

  log("Boot db -- BEGIN.");

  log("Resetting the game time:");
  reset_time();

  log("Initialize Global / Group Lists");
  global_lists = create_list();
  group_list = create_list();
  global_craft_list = create_list(); /* NewCraft */

  log("Initializing Events");
  init_events();

  log("Reading news, credits, help, ihelp, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(IHELP_PAGE_FILE, &ihelp);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
    prune_crlf(GREETINGS);

  log("Loading spell definitions.");
  mag_assign_spells();

  log("Loading weapon and armor special ability definitions.");
  initialize_special_abilities();

  log("Initializing object rnum hash table.");
  init_obj_rnum_hash();

  boot_world();

  log("Loading help entries.");
  index_boot(DB_BOOT_HLP);

  log("Generating player index.");
  build_player_index();

  if (auto_pwipe)
  {
    log("Cleaning out inactive pfiles.");
    clean_pfiles();
  }

  log("Loading fight messages.");
  load_messages();

  log("Loading social messages.");
  boot_social_messages();

  log("Building command list.");
  create_command_list(); /* aedit patch -- M. Scott */

  log("Assigning function pointers:");

  if (!no_specials)
  {
    log("   Mobiles.");
    assign_mobiles();
    log("   Shopkeepers.");
    assign_the_shopkeepers();
    log("   Objects.");
    assign_objects();
    log("   Rooms.");
    assign_rooms();
    log("   Questmasters.");
    assign_the_quests();
  }

  /* this MUST come after boot_world because the class_list where we have
     all the data for assigning min. level for spells needs to be initialized
     first */
  log("Assigning spell and skill levels.");
  init_spell_levels();

  log("Sorting command list...");
  sort_commands();

  log("Sorting spells/skills...");
  sort_spells();

  log("Booting mail system.");
  if (!scan_file())
  {
    log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }
  log("Reading banned site and invalid-name list.");
  load_banned();
  read_invalid_list();

  log("Loading Ideas.");
  load_ibt_file(SCMD_IDEA);

  log("Loading Bugs.");
  load_ibt_file(SCMD_BUG);

  log("Loading Typos.");
  load_ibt_file(SCMD_TYPO);

  log("Loading random encounter tables.");
  populate_encounter_table();

  log("Loading hunts table.");
  load_hunts();
  log("Spawning hunts for the first time this boot.");
  create_hunts();

  if (!no_rent_check)
  {
    log("Deleting timed-out crash and rent files:");
    update_obj_file();
    log("   Done.");
  }

  /* Moved here so the object limit code works. -gg 6/24/98 */
  if (!mini_mud)
  {
    /* moved house code down below zreset because it would affect what items load at boot */

    /* NewCraft */
    log("Booting crafts.");
    load_crafts();

    log("Booting clans.");
    load_clans();
    
    log("Initializing clan economy system.");
    init_clan_economy();
    load_clan_investments();

    log("Loading clan zone claim info.");
    load_claims();
  }

  log("Cleaning up last log.");
  clean_llog_entries();

#if 1
  {
    int j;

    for (j = 0; j < top_of_objt; j++)
    {
      if (obj_proto[j].script == (struct script_data *)&shop_keeper)
      {
        log("Item %d (%s) had shopkeeper trouble.", obj_index[j].vnum, obj_proto[j].short_description);
        obj_proto[j].script = NULL;
      }
    }
  }
#endif

  for (i = 0; i <= top_of_zone_table; i++)
  {
    strncpy(buf1, zone_table[i].name, sizeof(buf1) - 1);
    buf1[sizeof(buf1) - 1] = '\0';
    strip_colors(buf1);
    log("Resetting #%d: %s (rooms %d-%d).", zone_table[i].number,
        buf1, zone_table[i].bot, zone_table[i].top);
    reset_zone(i);
  }

  reset_q.head = reset_q.tail = NULL;

  if (!boot_time)
    boot_time = time(0);

  /* moved house code below zreset (here) because it would affect what items load at boot */
  if (!mini_mud)
  {
    log("Booting houses.");
    House_boot();
  }

  // The next few procedures are used to clean up the world, as many objects loaded into
  // rooms are being loaded in NOWHERE. Cause not found yet... reset_zone being called too many times
  // before the world is completely set up perhaps
  log("Cleaning up objects loaded in rooms.");

  struct obj_data *j = NULL, *next_thing;
  for (j = object_list; j; j = next_thing)
  {
    next_thing = j->next; /* Next in object list */

    if (!j)
      continue;

    if (j->in_room == NOWHERE)
    {
      extract_obj(j);
      continue;
    }
  }

  for (i = 0; i <= top_of_zone_table; i++)
  {
    reset_zone(i);
  }

  log("Cleaning up objects loaded in rooms - DONE.");

  log("Boot db -- DONE.");
}

/* reset the time in the game from file */
static void reset_time(void)
{
  time_t beginning_of_time = 0;
  FILE *bgtime;
  int i;

  if ((bgtime = fopen(TIME_FILE, "r")) == NULL)
    log("No time file '%s' starting from the beginning.", TIME_FILE);
  else
  {
    i = fscanf(bgtime, "%ld\n", (long *)&beginning_of_time);
    fclose(bgtime);
  }

  if (beginning_of_time == 0)
    beginning_of_time = 650336715;

  time_info = *mud_time_passed(time(0), beginning_of_time);

  if (time_info.hours <= 4)
    weather_info.sunlight = SUN_DARK;
  else if (time_info.hours == 5)
    weather_info.sunlight = SUN_RISE;
  else if (time_info.hours <= 20)
    weather_info.sunlight = SUN_LIGHT;
  else if (time_info.hours == 21)
    weather_info.sunlight = SUN_SET;
  else
    weather_info.sunlight = SUN_DARK;

  log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
      time_info.day, time_info.month, time_info.year);

  weather_info.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    weather_info.pressure += dice(1, 50);
  else
    weather_info.pressure += dice(1, 80);

  weather_info.change = 0;

  if (weather_info.pressure <= 980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure <= 1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure <= 1020)
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;
}

/* Write the time in 'when' to the MUD-time file. */
void save_mud_time(struct time_info_data *when)
{
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "w")) == NULL)
    log("SYSERR: Can't write to '%s' time file.", TIME_FILE);
  else
  {
    fprintf(bgtime, "%ld\n", (long)mud_time_to_secs(when));
    fclose(bgtime);
  }
}

/* Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I did
 * add the 'goto' and changed some "while()" into "do { } while()". -gg */
static int count_alias_records(FILE *fl)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;
  int total_keywords = 0;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$')
  {
    /* skip the text */
    do
    {
      get_one_line(fl, line);
      if (feof(fl))
        goto ackeof;
    } while (*line != '#');

    /* now count keywords */
    scan = key;
    do
    {
      scan = one_word(scan, next_key);
      if (*next_key)
        ++total_keywords;
    } while (*next_key);

    /* get next keyword line (or $) */
    get_one_line(fl, key);

    if (feof(fl))
      goto ackeof;
  }

  return (total_keywords);

  /* No, they are not evil. -gg 6/24/98 */
ackeof:
  log("SYSERR: Unexpected end of help file.");
  exit(1); /* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE *fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return (count);
}

void index_boot(int mode)
{
  const char *index_filename, *prefix = NULL; /* NULL or egcs 1.1 complains */
  FILE *db_index, *db_file;
  int rec_count = 0, size[2] = {0, 0}, i = 0;
  char buf2[MAX_PATH] = {'\0'};
  char buf1[MAX_STRING_LENGTH] = {'\0'};

  switch (mode)
  {
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  case DB_BOOT_HLP:
    prefix = HLP_PREFIX;
    break;
  case DB_BOOT_TRG:
    prefix = TRG_PREFIX;
    break;
  case DB_BOOT_QST:
    prefix = QST_PREFIX;
    break;
  case DB_BOOT_HLQST:
    prefix = HLQST_PREFIX;
    break;
  default:
    log("SYSERR: Unknown subcommand %d to index_boot!", mode);
    exit(1);
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  snprintf(buf2, sizeof(buf2), "%s%s", prefix, index_filename);
  if (!(db_index = fopen(buf2, "r")))
  {
    log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
    exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  i = fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$')
  {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r")))
    {
      log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix,
          index_filename, strerror(errno));
      i = fscanf(db_index, "%s\n", buf1);
      continue;
    }
    else
    {
      if (mode == DB_BOOT_ZON)
        rec_count++;
      else if (mode == DB_BOOT_HLP)
        rec_count += count_alias_records(db_file);
      else
        rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    i = fscanf(db_index, "%s\n", buf1);
  }

  /* Exit if 0 records, unless this is shops */
  if (!rec_count)
  {
    if (mode == DB_BOOT_SHP || mode == DB_BOOT_QST || mode == DB_BOOT_HLQST || mode == DB_BOOT_TRG)
      return;
    log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
        index_filename);
    exit(1);
  }

  /* "bytes" does _not_ include strings or other later malloc'd things. */
  switch (mode)
  {
  case DB_BOOT_TRG:
    CREATE(trig_index, struct index_data *, rec_count);
    break;
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    size[0] = sizeof(struct room_data) * rec_count;
    log("   %d rooms, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct char_data) * rec_count;
    log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct obj_data) * rec_count;
    log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    size[0] = sizeof(struct zone_data) * rec_count;
    log("   %d zones, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_HLP:
    CREATE(help_table, struct help_index_element, rec_count);
    size[0] = sizeof(struct help_index_element) * rec_count;
    log("   %d entries, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_QST:
    CREATE(aquest_table, struct aq_data, rec_count);
    size[0] = sizeof(struct aq_data) * rec_count;
    log("   %d entries, %d bytes.", rec_count, size[0]);
    break;
  }

  rewind(db_index);
  i = fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$')
  {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r")))
    {
      log("SYSERR: %s: %s", buf2, strerror(errno));
      exit(1);
    }
    switch (mode)
    {
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
    case DB_BOOT_TRG:
    case DB_BOOT_QST:
      discrete_load(db_file, mode, buf2);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_HLP:
      load_help(db_file, buf2);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
    case DB_BOOT_HLQST:
      boot_the_quests(db_file, buf2, rec_count);
      break;
    }

    fclose(db_file);
    i = fscanf(db_index, "%s\n", buf1);
  }
  fclose(db_index);

  /* Sort the help index. */
  if (mode == DB_BOOT_HLP)
  {
    qsort(help_table, top_of_helpt, sizeof(struct help_index_element), help_sort);
  }
}

void discrete_load(FILE *fl, int mode, char *filename)
{
  int nr = -1, last = 0;
  char line[READ_SIZE] = {'\0'};

  const char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg", "qst"};
  /* modes positions correspond to DB_BOOT_xxx in db.h */

  for (;;)
  {
    /* We have to do special processing with the obj files because they have no
     * end-of-record marker. */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line))
      {
        if (nr == -1)
        {
          log("SYSERR: %s file %s is empty!", modes[mode], filename);
        }
        else
        {
          log("SYSERR: Format error in %s after %s #%d\n"
              "...expecting a new %s, but file ended!\n"
              "(maybe the file is not terminated with '$'?)",
              filename,
              modes[mode], nr, modes[mode]);
        }
        exit(1);
      }
    if (*line == '$')
      return;

    if (*line == '#')
    {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1)
      {
        log("SYSERR: Format error after %s #%d", modes[mode], last);
        exit(1);
      }
      switch (mode)
      {
      case DB_BOOT_WLD:
        parse_room(fl, nr);
        break;
      case DB_BOOT_MOB:
        parse_mobile(fl, nr);
        break;
      case DB_BOOT_TRG:
        parse_trigger(fl, nr);
        break;
      case DB_BOOT_OBJ:
        strlcpy(line, parse_object(fl, nr), sizeof(line));
        break;
      case DB_BOOT_QST:
        parse_quest(fl, nr);
        break;
      case DB_BOOT_HLQST:
        /* nothing is done here right now */
        break;
      }
    }
    else
    {
      log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
          filename, modes[mode], nr);
      log("SYSERR: ... offending line: '%s'", line);
      exit(1);
    }
  }
}

static char fread_letter(FILE *fp)
{
  char c;
  do
  {
    c = getc(fp);
  } while (isspace(c));
  return c;
}

bitvector_t asciiflag_conv(const char *flag)
{
  bitvector_t flags = 0;
  int is_num = TRUE;
  const char *p;

  for (p = flag; *p; p++)
  {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    /* Allow the first character to be a minus sign */
    if (!isdigit(*p) && (*p != '-' || p != flag))
      is_num = FALSE;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}

static bitvector_t asciiflag_conv_aff(char *flag)
{
  bitvector_t flags = 0;
  int is_num = TRUE;
  char *p;

  for (p = flag; *p; p++)
  {
    if (islower(*p))
      flags |= 1 << (*p - 'a' + 1);
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A' + 1));

    /* Allow the first character to be a minus sign */
    if (!isdigit(*p) && (*p != '-' || p != flag))
      is_num = FALSE;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}

/* load the rooms */
void parse_room(FILE *fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int i = 0, retval = 0;
  char line[READ_SIZE] = {'\0'};
  char flags[128] = {'\0'};
  char flags2[128] = {'\0'};
  char flags3[128] = {'\0'};
  char flags4[128] = {'\0'};
  char buf[128] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  struct extra_descr_data *new_descr = NULL;
  char letter = '\0';

  /* This really had better fit or there are other problems. */
  snprintf(buf2, sizeof(buf2), "room #%d", virtual_nr);

  if (virtual_nr < zone_table[zone].bot)
  {
    log("SYSERR: (parse_room) Room #%d is below zone %d (bot=%d, top=%d).", virtual_nr, zone_table[zone].number, zone_table[zone].bot, zone_table[zone].top);
    exit(1);
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table)
    {
      log("SYSERR: Room %d is outside of any zone.", virtual_nr);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

  /* Initialize trails */
  CREATE(world[room_nr].trail_tracks, struct trail_data_list, 1);
  world[room_nr].trail_tracks->head = NULL;
  world[room_nr].trail_tracks->tail = NULL;
  //  CREATE(world[room_nr].trail_scent, struct trail_data_list, 1);
  //  world[room_nr].trail_scent->head = NULL;
  //  world[room_nr].trail_scent->tail = NULL;
  //  CREATE(world[room_nr].trail_blood, struct trail_data_list, 1);
  //  world[room_nr].trail_blood->head = NULL;
  //  world[room_nr].trail_blood->tail = NULL;

  if (!get_line(fl, line))
  {
    log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
        virtual_nr);
    /* CRITICAL FIX: Free allocated memory before exit to prevent memory leak */
    if (world[room_nr].name)
      free(world[room_nr].name);
    if (world[room_nr].description)
      free(world[room_nr].description);
    if (world[room_nr].trail_tracks)
      free(world[room_nr].trail_tracks);
    exit(1);
  }

  if (((retval = sscanf(line, " %d %s %s %s %s %d ", t, flags, flags2, flags3, flags4, t + 2)) == 3) && (bitwarning == TRUE))
  {
    log("WARNING: Conventional world files detected. See config.c.");
    /* CRITICAL FIX: Free allocated memory before exit to prevent memory leak */
    if (world[room_nr].name)
      free(world[room_nr].name);
    if (world[room_nr].description)
      free(world[room_nr].description);
    if (world[room_nr].trail_tracks)
      free(world[room_nr].trail_tracks);
    exit(1);
  }
  else if ((retval == 3) && (bitwarning == FALSE))
  {
    /* Looks like the implementor is ready, so let's load the world files. We
     * load the extra three flags as 0, since they won't be anything anyway. We
     * will save the entire world later on, when every room, mobile, and object
     * is converted. */
    log("Converting room #%d to 128bits..", virtual_nr);
    world[room_nr].room_flags[0] = asciiflag_conv(flags);
    world[room_nr].room_flags[1] = 0;
    world[room_nr].room_flags[2] = 0;
    world[room_nr].room_flags[3] = 0;

    /* In the old-style files, the 3rd item was the sector-type */
    world[room_nr].sector_type = atoi(flags2);

    snprintf(flags, sizeof(flags), "room #%d", virtual_nr); /* sprintf: OK (until 399-bit integers) */

    /* No need to scan the other three sections; they're 0 anyway. */
    check_bitvector_names(world[room_nr].room_flags[0], room_bits_count, flags, "room");

    if (bitsavetodisk)
    { /* Maybe the implementor just wants to look at the 128bit files */
      add_to_save_list(zone_table[real_zone_by_thing(virtual_nr)].number, 3);
      converting = TRUE;
    }

    log("   done.");
  }
  else if (retval == 6)
  {
    int taeller;

    world[room_nr].room_flags[0] = asciiflag_conv(flags);
    world[room_nr].room_flags[1] = asciiflag_conv(flags2);
    world[room_nr].room_flags[2] = asciiflag_conv(flags3);
    world[room_nr].room_flags[3] = asciiflag_conv(flags4);

    snprintf(flags, sizeof(flags), "room #%d", virtual_nr); /* sprintf: OK (until 399-bit integers) */
    for (taeller = 0; taeller < AF_ARRAY_MAX; taeller++)
      check_bitvector_names(world[room_nr].room_flags[taeller], room_bits_count, flags, "room");

    /* Added Sanity check */
    if (t[2] > NUM_ROOM_SECTORS)
      t[2] = SECT_INSIDE;

    world[room_nr].sector_type = t[2];
  }
  else
  {
    log("SYSERR: Format error in roomflags/sector type of room #%d", virtual_nr);
    /* CRITICAL FIX: Free allocated memory before exit to prevent memory leak */
    if (world[room_nr].name)
      free(world[room_nr].name);
    if (world[room_nr].description)
      free(world[room_nr].description);
    if (world[room_nr].trail_tracks)
      free(world[room_nr].trail_tracks);
    exit(1);
  }

  // REMOVE_BIT_AR(world[room_nr].room_flags, ROOM_MAGICDARK);

  world[room_nr].func = NULL;
  world[room_nr].contents = NULL;
  world[room_nr].people = NULL;
  world[room_nr].light = 0; /* Zero light sources */
  world[room_nr].globe = 0; /* Zero darkness sources */

  for (i = 0; i < NUM_OF_DIRS; i++) /* NUM_OF_DIRS here, not DIR_COUNT */
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;

  snprintf(buf, sizeof(buf), "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;)
  {

    if (!get_line(fl, line))
    {
      log("%s", buf);
      /* CRITICAL FIX: Free allocated memory before exit to prevent memory leak */
      if (world[room_nr].name)
        free(world[room_nr].name);
      if (world[room_nr].description)
        free(world[room_nr].description);
      if (world[room_nr].trail_tracks)
        free(world[room_nr].trail_tracks);
      /* Also free any extra descriptions allocated in this loop */
      free_extra_descriptions(world[room_nr].ex_description);
      exit(1);
    }
    switch (*line)
    {
    case 'C': /* Coordinates. */
      get_line(fl, line);
      sscanf(line, "%d %d", world[room_nr].coords, world[room_nr].coords + 1);
      break;
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1));
      break;
    case 'M':
      setup_moving_room(fl, room_nr, virtual_nr, (line + 1));
      world[room_nr].func = &moving_rooms;
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      /* Fix for crashes in the editor when formatting. E-descs are assumed to
       * end with a \r\n. -Welcor */
      {
        char *end = strchr(new_descr->description, '\0');
        if (end > new_descr->description && *(end - 1) != '\n')
        {
          CREATE(end, char, strlen(new_descr->description) + 3);
          sprintf(end, "%s\r\n", new_descr->description); /* snprintf ok : size checked above*/
          free(new_descr->description);
          new_descr->description = end;
        }
      }
      new_descr->next = world[room_nr].ex_description;
      world[room_nr].ex_description = new_descr;
      break;
    case 'S': /* end of room */
      /* DG triggers -- script is defined after the end of the room */
      letter = fread_letter(fl);
      ungetc(letter, fl);
      while (letter == 'T')
      {
        dg_read_trigger(fl, &world[room_nr], WLD_TRIGGER);
        letter = fread_letter(fl);
        ungetc(letter, fl);
      }
      top_of_world = room_nr++;
      return;
    default:
      log("%s", buf);
      /* CRITICAL FIX: Free allocated memory before exit to prevent memory leak */
      if (world[room_nr].name)
        free(world[room_nr].name);
      if (world[room_nr].description)
        free(world[room_nr].description);
      if (world[room_nr].trail_tracks)
        free(world[room_nr].trail_tracks);
      /* Also free any extra descriptions allocated in this loop */
      free_extra_descriptions(world[room_nr].ex_description);
      exit(1);
    }
  }
}

/*
 *  PDH 11/17/97
 *  moving_rooms_update - each zone pulse, we check if any "movement
 *  rooms" need to be relocated (ie. their pulse timer ran out)
 */

struct moving_room_data * movingRoomList = NULL;

void moving_rooms_update(void)
{
    struct moving_room_data* nextRoom = movingRoomList;
    room_num mover;
    char errStr[100];

    while (nextRoom != NULL) {
        nextRoom->remainingZonePulses--;
        if (nextRoom->remainingZonePulses <= 0) {
            mover = nextRoom->destination;

            if (real_room(mover) > 0) {
                if (world[real_room(mover)].func != NULL) {
                    world[real_room(mover)].func(NULL, nextRoom, 0, NULL);
                }
            } else {
                sprintf(errStr, "moving_rooms_update: real_room(%d) < 0", mover);
                log("%s", errStr);
            }

            nextRoom->remainingZonePulses = nextRoom->resetZonePulse;
        }

        nextRoom = nextRoom->next;
    }
}

/*  read moving room data  */
void setup_moving_room(FILE* fl, int rroom, int vroom, char* line) {
    int roomInfo[5];
    int connInfo[3];
    char buf[MAX_STRING_LENGTH];

    int connCnt = 0, j, connLine = 0;
    room_num fR[MAX_MOVING_ROOMS];
    int fD[MAX_MOVING_ROOMS];

    char errStr[100];
    char lineIn[256];
    char msg1[200], msg2[200], msg3[200];

    struct moving_room_data* newRoom;

    if (world[rroom].mover) {
        log("SYSERR: setup_moving_room - this room already has a mover assigned");
        return;
    }

    strcpy(errStr, "");
    strcpy(lineIn, "");
    strcpy(msg1, "");
    strcpy(msg2, "");
    strcpy(msg3, "");

    for (j = 0; j < MAX_MOVING_ROOMS; j++) {
        fR[j] = NOWHERE;
        fD[j] = -1;
    }

    if (sscanf(line, " %d %d %d %d %d ", roomInfo, roomInfo + 1, roomInfo + 2, roomInfo + 3, roomInfo + 4) != 5) {
        fprintf(stderr, "Format error, room #%d, M line\n", world[rroom].number);
        exit(1);
    }

    /*  now get the 3 room messages (if they exist)  */
    if (!get_line(fl, lineIn)) {
        fprintf(stderr, " missing transit message when processing M...\n");
        exit(1);
    }
    if (lineIn[0] != '~') {
        strcpy(msg1, lineIn);
    }

    if (!get_line(fl, lineIn)) {
        fprintf(stderr, " missing docking message when processing M...\n");
        exit(1);
    }
    if (lineIn[0] != '~') {
        strcpy(msg2, lineIn);
    }

    if (!get_line(fl, lineIn)) {
        fprintf(stderr, " missing dest dock message when processing M...\n");
        exit(1);
    }
    if (lineIn[0] != '~') {
        strcpy(msg3, lineIn);
    }

    if (!get_line(fl, lineIn)) {
        fprintf(stderr, " - get_line() returned 0 in world 'M' processing...\n");
        fprintf(stderr, "%s\n", lineIn);
        exit(1);
    }

    while (lineIn[0] != '~') {
        if (sscanf(lineIn, " %d %d %d ", connInfo, connInfo + 1, connInfo + 2) != 3) {
            fprintf(stderr, "Format error, room #%d, %d after M line\n", (connLine + 1), world[rroom].number);
            exit(1);
        }

        connLine++;

        if (connInfo[2] < 1) {
            connInfo[2] = 1;
        } else if (connInfo[2] > 50) {
            connInfo[2] = 50;
        }

        for (j = 0; j < connInfo[2]; j++) {
            connCnt++;

            if (connCnt < MAX_MOVING_ROOMS) {
                /*  store the conn room info  */
                fR[connCnt - 1] = (room_num)connInfo[0];
                fD[connCnt - 1] = connInfo[1];
            } else {
                log("setup_moving_room(): # of conneting rooms exceeded limit...");
            }
        }

        if (!get_line(fl, lineIn)) {
            fprintf(stderr, " - get_line() returned 0 in world 'M' processing...\n");
            fprintf(stderr, "%s\n", buf);
            exit(1);
        }
    }

/*
 *  PDH 11/17/97
 *  lots of work for each new "moving room"
 *  must set up struct moving_room_data and add to
 *  movingRoomList
 */
#ifdef DEBUGMEM
    CREATE(newRoom, struct moving_room_data, 1, M1);
#else
    CREATE(newRoom, struct moving_room_data, 1);
#endif

    newRoom->resetZonePulse = roomInfo[1];
    newRoom->remainingZonePulses = newRoom->resetZonePulse;
    newRoom->currentInbound = -1;
    newRoom->destination = vroom;
    newRoom->inbound_dir = roomInfo[0];
    newRoom->randomMove = roomInfo[2];
    newRoom->exitInfo = roomInfo[3];
    newRoom->keyInfo = roomInfo[4];
#ifdef DEBUGMEM
    newRoom->keywords = str_dup("door", S19);
#else
    newRoom->keywords = strdup("door");
#endif

    if (strlen(msg1) > 5)
    {
#ifdef DEBUGMEM
        newRoom->msg_transit = str_dup(msg1, T19);
#else
        newRoom->msg_transit = strdup(msg1);
#endif

    } else {
        newRoom->msg_transit = NULL;
    }
    if (strlen(msg2) > 5) {
#ifdef DEBUGMEM
        newRoom->msg_docking = str_dup(msg2, U19);
#else
        newRoom->msg_docking = strdup(msg2);
#endif
    } else {
        newRoom->msg_docking = NULL;
    }
    if (strlen(msg3) > 5) {
#ifdef DEBUGMEM
        newRoom->msg_dest_docking = str_dup(msg3, V19);
#else
        newRoom->msg_dest_docking = strdup(msg3);
#endif
    } else {
        newRoom->msg_dest_docking = NULL;
    }
#ifdef DEBUGMEM
    CREATE(newRoom->from, room_num, connCnt + 1, N1);
    CREATE(newRoom->fromDir, int, connCnt + 1, O1);
#else
    CREATE(newRoom->from, room_num, connCnt + 1);
    CREATE(newRoom->fromDir, int, connCnt + 1);
#endif

    for (j = 0; j < connCnt; j++) {
        newRoom->from[j] = fR[j];
        newRoom->fromDir[j] = fD[j];
    }
    newRoom->from[j] = ENDMOVING;
    newRoom->fromDir[j] = -1;

    newRoom->next = NULL;

    if (movingRoomList == NULL) {
        movingRoomList = newRoom;
    } else {
        newRoom->next = movingRoomList;
        movingRoomList = newRoom;
    }

    world[rroom].mover = newRoom;

    return;
}

/* read direction data */
void setup_dir(FILE *fl, int room, int dir)
{
  int t[5];
  char line[READ_SIZE], buf2[128];

  snprintf(buf2, sizeof(buf2), "room #%d, direction D%d", GET_ROOM_VNUM(room) + 1, dir);

  if (!CONFIG_DIAGONAL_DIRS && IS_DIAGONAL(dir))
  {
    log("Warning: Diagonal direction disabled: %s", buf2);
    return;
  }

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line))
  {
    log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3)
  {
    log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (t[0] == 1)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else if (t[0] == 2)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else if (t[0] == 3)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_HIDDEN;
  else if (t[0] == 4)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF | EX_HIDDEN;
  else
    world[room].dir_option[dir]->exit_info = 0;

  world[room].dir_option[dir]->key = ((t[1] == -1 || t[1] == 65535) ? NOTHING : t[1]);
  world[room].dir_option[dir]->to_room = ((t[2] == -1 || t[2] == 0 ||
                                           t[2] == 65535)
                                              ? NOWHERE
                                              : t[2]);
}

/* make sure the start rooms exist & resolve their vnums to rnums */
static void check_start_rooms(void)
{

  if ((r_mortal_start_room = real_room(CONFIG_MORTAL_START)) == NOWHERE)
  {
    log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    exit(1);
  }
  if ((r_immort_start_room = real_room(CONFIG_IMMORTAL_START)) == NOWHERE)
  {
    if (!mini_mud)
      log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(CONFIG_FROZEN_START)) == NOWHERE)
  {
    if (!mini_mud)
      log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
  }
}

/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  int room, door;

  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].dir_option[door])
        if (world[room].dir_option[door]->to_room != NOWHERE)
          world[room].dir_option[door]->to_room =
              real_room(world[room].dir_option[door]->to_room);
}

/** This is not the same ZCMD as used elsewhere. GRUMBLE... namespace conflict
 * @todo refactor this particular ZCMD and remove this redefine. */
#ifdef ZCMD
#undef ZCMD
#endif
#define ZCMD zone_table[zone].cmd[cmd_no]

/* Resolve vnums into rnums in the zone reset tables. In English: Once all of
 * the zone reset tables have been loaded, we resolve the virtual numbers into
 * real numbers all at once so we don't have to do it repeatedly while the game
 * is running.  This does make adding any room, mobile, or object a little more
 * difficult while the game is running. Assumes NOWHERE == NOBODY == NOTHING.
 * Assumes sizeof(room_rnum) >= (sizeof(mob_rnum) and sizeof(obj_rnum)) */
static void renum_zone_table(void)
{
  int cmd_no;
  room_rnum a, b, c, olda, oldb, oldc;
  zone_rnum zone;
  char buf[128];

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
    {
      a = b = c = 0;
      olda = ZCMD.arg1;
      oldb = ZCMD.arg2;
      oldc = ZCMD.arg3;
      switch (ZCMD.command)
      {
      case 'M':
        a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
        c = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'O':
        a = real_object(ZCMD.arg1);
        /* CRITICAL: Validate object vnum at parse time */
        if (a == NOTHING) {
          log("SYSERR: Zone %d cmd %d: Object vnum %d does not exist (O command)", 
              zone_table[zone].number, cmd_no, olda);
          /* Keep the original vnum for error tracking but mark as invalid */
          ZCMD.arg1 = NOTHING;
        } else {
          ZCMD.arg1 = a;
        }
        if (ZCMD.arg3 != NOWHERE)
          c = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'G':
        a = real_object(ZCMD.arg1);
        /* CRITICAL: Validate object vnum at parse time */
        if (a == NOTHING) {
          log("SYSERR: Zone %d cmd %d: Object vnum %d does not exist (G command)", 
              zone_table[zone].number, cmd_no, olda);
          /* Keep the original vnum for error tracking but mark as invalid */
          ZCMD.arg1 = NOTHING;
        } else {
          ZCMD.arg1 = a;
        }
        break;
      case 'E':
        a = real_object(ZCMD.arg1);
        /* CRITICAL: Validate object vnum at parse time */
        if (a == NOTHING) {
          log("SYSERR: Zone %d cmd %d: Object vnum %d does not exist (E command)", 
              zone_table[zone].number, cmd_no, olda);
          /* Keep the original vnum for error tracking but mark as invalid */
          ZCMD.arg1 = NOTHING;
        } else {
          ZCMD.arg1 = a;
        }
        break;
      case 'P':
        a = real_object(ZCMD.arg1);
        /* CRITICAL: Validate object vnum at parse time */
        if (a == NOTHING) {
          log("SYSERR: Zone %d cmd %d: Object vnum %d does not exist (P command)", 
              zone_table[zone].number, cmd_no, olda);
          /* Keep the original vnum for error tracking but mark as invalid */
          ZCMD.arg1 = NOTHING;
        } else {
          ZCMD.arg1 = a;
        }
        c = real_object(ZCMD.arg3);
        if (c == NOTHING) {
          log("SYSERR: Zone %d cmd %d: Container object vnum %d does not exist (P command)", 
              zone_table[zone].number, cmd_no, oldc);
          /* Keep the original vnum for error tracking but mark as invalid */
          ZCMD.arg3 = NOTHING;
        } else {
          ZCMD.arg3 = c;
        }
        break;
      case 'D':
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
        break;
      case 'R': /* rem obj from room */
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
        b = real_object(ZCMD.arg2);
        /* CRITICAL: Validate object vnum at parse time */
        if (b == NOTHING) {
          log("SYSERR: Zone %d cmd %d: Object vnum %d does not exist (R command)", 
              zone_table[zone].number, cmd_no, oldb);
          /* Keep the original vnum for error tracking but mark as invalid */
          ZCMD.arg2 = NOTHING;
        } else {
          ZCMD.arg2 = b;
        }
        break;
      case 'T': /* a trigger */
        b = ZCMD.arg2 = real_trigger(ZCMD.arg2);
        c = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'V': /* trigger variable assignment */
        b = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'L': /* Load random treasure in container */
        c = real_object(ZCMD.arg3);
        /* CRITICAL: Validate container object vnum at parse time */
        if (c == NOTHING) {
          log("SYSERR: Zone %d cmd %d: Container object vnum %d does not exist (L command)", 
              zone_table[zone].number, cmd_no, oldc);
          /* Keep the original vnum for error tracking but mark as invalid */
          ZCMD.arg3 = NOTHING;
        } else {
          ZCMD.arg3 = c;
        }
        break;
      }
      if (a == NOWHERE || b == NOWHERE || c == NOWHERE)
      {
        if (!mini_mud)
        {
          snprintf(buf, sizeof(buf), "Invalid vnum %d, cmd disabled",
                   a == NOWHERE ? olda : b == NOWHERE ? oldb
                                                      : oldc);
          log_zone_error(zone, cmd_no, buf);
        }
        ZCMD.command = '*';
      }
    }
}

/* basic vitals when loading a mobile */
static void parse_simple_mob(FILE *mob_f, int i, int nr)
{
  int j, t[10];
  char line[READ_SIZE];

  GET_REAL_CON(mob_proto + i) = 11;
  GET_REAL_STR(mob_proto + i) = 11;
  GET_REAL_DEX(mob_proto + i) = 11;
  GET_REAL_INT(mob_proto + i) = 11;
  GET_REAL_CHA(mob_proto + i) = 11;
  GET_REAL_WIS(mob_proto + i) = 11;

  if (!get_line(mob_f, line))
  {
    log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
    exit(1);
  }

  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
             t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9)
  {
    log("SYSERR: Format error in mob #%d, first line after S flag\n"
        "...expecting line of form '# # # #d#+# #d#+#'",
        nr);
    exit(1);
  }

  GET_LEVEL(mob_proto + i) = t[0];
  GET_REAL_HITROLL(mob_proto + i) = 20 - t[1];

  /* hack to convert old school dnd AC to d20
     the AC is saved to file as a factor of 10 of the old school system
     we have to convert to d20, then multiply the factor back in
   * this is the opposite of what is done in genmob.c's write_mobile_record */
  /* GET_REAL_AC(mob_proto + i) = 10 * (t[2]); */
  GET_REAL_AC(mob_proto + i) = 10 * (20 - t[2]);

  /* max hit = 0 is a flag that H, M, V is xdy+z */
  //  GET_REAL_MAX_HIT(mob_proto + i) = 0;
  GET_MAX_HIT(mob_proto + i) = 0;
  GET_HIT(mob_proto + i) = t[3];
  GET_PSP(mob_proto + i) = t[4];
  GET_MOVE(mob_proto + i) = t[5];

  GET_REAL_MAX_PSP(mob_proto + i) = 10;
  GET_REAL_MAX_MOVE(mob_proto + i) = 1000;

  GET_REAL_SPELL_RES(mob_proto + i) = 0;

  mob_proto[i].mob_specials.damnodice = t[6];
  mob_proto[i].mob_specials.damsizedice = t[7];
  GET_REAL_DAMROLL(mob_proto + i) = t[8];

  if (!get_line(mob_f, line))
  {
    log("SYSERR: Format error in mob #%d, second line after S flag\n"
        "...expecting line of form '# #', but file ended!",
        nr);
    exit(1);
  }

  if (sscanf(line, " %d %d ", t, t + 1) != 2)
  {
    log("SYSERR: Format error in mob #%d, second line after S flag\n"
        "...expecting line of form '# #'",
        nr);
    exit(1);
  }

  if (!MOB_FLAGGED(mob_proto + i, MOB_CUSTOM_GOLD))
    GET_GOLD(mob_proto + i) = 40 + dice(GET_LEVEL(mob_proto + i) * 2, 3);
  else
    GET_GOLD(mob_proto + i) = t[0];
  GET_EXP(mob_proto + i) = t[1];

  if (!get_line(mob_f, line))
  {
    log("SYSERR: Format error in last line of mob #%d\n"
        "...expecting line of form '# # #', but file ended!",
        nr);
    exit(1);
  }

  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3)
  {
    log("SYSERR: Format error in last line of mob #%d\n"
        "...expecting line of form '# # #'",
        nr);
    exit(1);
  }

  GET_DEFAULT_POS(mob_proto + i) = t[1];
  if (GET_DEFAULT_POS(mob_proto + i) == POS_FIGHTING)
    GET_DEFAULT_POS(mob_proto + i) = POS_STANDING;
  GET_POS(mob_proto + i) = t[0];
  if (GET_POS(mob_proto + i) == POS_FIGHTING)
    GET_POS(mob_proto + i) = POS_STANDING;

  GET_SEX(mob_proto + i) = t[2];

  GET_SUBRACE(mob_proto + i, 0) = 0;
  GET_SUBRACE(mob_proto + i, 1) = 0;
  GET_SUBRACE(mob_proto + i, 2) = 0;
  GET_REAL_RACE(mob_proto + i) = 0;
  GET_CLASS(mob_proto + i) = 0;
  GET_REAL_SIZE(mob_proto + i) = SIZE_MEDIUM;
  GET_WEIGHT(mob_proto + i) = 200;
  GET_HEIGHT(mob_proto + i) = 198;
  (mob_proto + i)->points.armor = GET_REAL_AC(mob_proto + i);

  /* These are now save applies; base save numbers for MOBs are now from the
   * warrior save table. */
  for (j = 0; j < NUM_OF_SAVING_THROWS; j++)
    GET_REAL_SAVE(mob_proto + i, j) = 0;

  for (j = 0; j < NUM_DAM_TYPES; j++)
    GET_REAL_RESISTANCES(mob_proto + i, j) = 0;

  // be sure to initialize any numeric echo stuff too
  ECHO_IS_ZONE(mob_proto + i) = FALSE;
  ECHO_FREQ(mob_proto + i) = 0;
  ECHO_COUNT(mob_proto + i) = 0;
  ECHO_SEQUENTIAL(mob_proto + i) = 0;
  CURRENT_ECHO(mob_proto + i) = 0;
  // ECHO_ENTRIES(mob_proto + i) = "";

  affect_total(mob_proto + i);
}

/* interpret_espec is the function that takes espec keywords and values and
 * assigns the correct value to the mob as appropriate.  Adding new e-specs is
 * absurdly easy -- just add a new CASE statement to this function! No other
 * changes need to be made anywhere in the code.
 * CASE		: Requires a parameter through 'value'.
 * BOOL_CASE	: Being specified at all is its value. */
#define CASE(test) \
  if (value && !matched && !str_cmp(keyword, test) && (matched = TRUE))
#define BOOL_CASE(test) \
  if (!value && !matched && !str_cmp(keyword, test) && (matched = TRUE))
#define RANGE(low, high) \
  (num_arg = MAX((low), MIN((high), (num_arg))))

static void interpret_espec(const char *keyword, const char *value, int i, int nr)
{
  int num_arg = 0, matched = FALSE;
  int num, num2;

  /* If there isn't a colon, there is no value.  While Boolean options are
   * possible, we don't actually have any.  Feel free to make some. */
  if (value)
    num_arg = atoi(value);

  CASE("BareHandAttack")
  {
    RANGE(0, NUM_ATTACK_TYPES - 1);
    mob_proto[i].mob_specials.attack_type = num_arg;
  }

  CASE("Str")
  {
    RANGE(3, 50);
    GET_REAL_STR(mob_proto + i) = num_arg;
  }

  CASE("StrAdd")
  {
    RANGE(0, 100);
    mob_proto[i].real_abils.str_add = num_arg;
  }

  CASE("Int")
  {
    RANGE(3, 50);
    GET_REAL_INT(mob_proto + i) = num_arg;
  }

  CASE("Wis")
  {
    RANGE(3, 50);
    GET_REAL_WIS(mob_proto + i) = num_arg;
  }

  CASE("Dex")
  {
    RANGE(3, 50);
    GET_REAL_DEX(mob_proto + i) = num_arg;
  }

  CASE("Con")
  {
    RANGE(3, 50);
    GET_REAL_CON(mob_proto + i) = num_arg;
  }

  CASE("Cha")
  {
    RANGE(3, 50);
    GET_REAL_CHA(mob_proto + i) = num_arg;
  }

  /* leaving old saves for ease-of-conversion cases -zusuk */
  CASE("SavingPara")
  {
    RANGE(0, 100);
    GET_REAL_SAVE(mob_proto + i, SAVING_FORT) = num_arg;
  }

  CASE("SavingFort")
  {
    RANGE(0, 100);
    GET_REAL_SAVE(mob_proto + i, SAVING_FORT) = num_arg;
  }

  CASE("SavingRod")
  {
    RANGE(0, 100);
    GET_REAL_SAVE(mob_proto + i, SAVING_REFL) = num_arg;
  }

  CASE("SavingRefl")
  {
    RANGE(0, 100);
    GET_REAL_SAVE(mob_proto + i, SAVING_REFL) = num_arg;
  }

  CASE("SavingPetri")
  {
    RANGE(0, 100);
    GET_REAL_SAVE(mob_proto + i, SAVING_WILL) = num_arg;
  }

  CASE("SavingWill")
  {
    RANGE(0, 100);
    GET_REAL_SAVE(mob_proto + i, SAVING_WILL) = num_arg;
  }

  CASE("SavingBreath")
  {
    RANGE(0, 100);
    GET_REAL_SAVE(mob_proto + i, SAVING_POISON) = num_arg;
  }

  CASE("SavingPoison")
  {
    RANGE(0, 100);
    GET_REAL_SAVE(mob_proto + i, SAVING_POISON) = num_arg;
  }

  CASE("SavingSpell")
  {
    RANGE(0, 100);
    GET_REAL_SAVE(mob_proto + i, SAVING_DEATH) = num_arg;
  }

  CASE("SavingDeath")
  {
    RANGE(0, 100);
    GET_REAL_SAVE(mob_proto + i, SAVING_DEATH) = num_arg;
  }

  CASE("MFeat")
  {
    sscanf(value, "%d %d", &num, &num2);
    MOB_SET_FEAT(mob_proto + i, num, num2);
  }

  CASE("DR_MOD")
  {
    RANGE(0, 100);
    GET_DR_MOD(mob_proto + i) = num_arg;
  }

  CASE("KnownSpell")
  {
    RANGE(1, NUM_SPELLS);
    MOB_KNOWS_SPELL((mob_proto + i), num_arg) = TRUE;
  }

  /* end saving throws */

  /* damtype resistances */
  CASE("ResFire")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_FIRE) = num_arg;
  }

  CASE("ResCold")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_COLD) = num_arg;
  }

  CASE("ResAir")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_AIR) = num_arg;
  }

  CASE("ResEarth")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_EARTH) = num_arg;
  }

  CASE("ResAcid")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_ACID) = num_arg;
  }

  CASE("ResHoly")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_HOLY) = num_arg;
  }

  CASE("ResElectric")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_ELECTRIC) = num_arg;
  }

  CASE("ResUnholy")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_UNHOLY) = num_arg;
  }

  CASE("ResSlice")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_SLICE) = num_arg;
  }

  CASE("ResPuncture")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_PUNCTURE) = num_arg;
  }

  CASE("ResForce")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_FORCE) = num_arg;
  }

  CASE("ResSound")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_SOUND) = num_arg;
  }

  CASE("ResPoison")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_POISON) = num_arg;
    GET_REAL_RESISTANCES(mob_proto + i, DAM_CELESTIAL_POISON) = num_arg;
  }

  CASE("ResDisease")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_DISEASE) = num_arg;
  }

  CASE("ResNegative")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_NEGATIVE) = num_arg;
  }

  CASE("ResIllusion")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_ILLUSION) = num_arg;
  }

  CASE("ResMental")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_MENTAL) = num_arg;
  }

  CASE("ResLight")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_LIGHT) = num_arg;
  }

  CASE("ResEnergy")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_ENERGY) = num_arg;
  }

  CASE("ResWater")
  {
    RANGE(-100, 100);
    GET_REAL_RESISTANCES(mob_proto + i, DAM_WATER) = num_arg;
  }

  /* end damtype resisatnces */

  CASE("Race")
  {
    RANGE(0, NUM_RACE_TYPES);
    GET_REAL_RACE(mob_proto + i) = num_arg;
  }

  CASE("SubRace 1")
  {
    RANGE(0, NUM_SUB_RACES);
    GET_SUBRACE(mob_proto + i, 0) = num_arg;
  }

  CASE("SubRace 2")
  {
    RANGE(0, NUM_SUB_RACES);
    GET_SUBRACE(mob_proto + i, 1) = num_arg;
  }

  CASE("SubRace 3")
  {
    RANGE(0, NUM_SUB_RACES);
    GET_SUBRACE(mob_proto + i, 2) = num_arg;
  }

  CASE("Class")
  {
    RANGE(0, NUM_CLASSES);
    GET_CLASS(mob_proto + i) = num_arg;
  }

  CASE("Feat")
  {
    sscanf(value, "%d %d", &num, &num2);
    SET_FEAT(mob_proto + i, num, num2);
  }

  CASE("Size")
  {
    RANGE(0, NUM_SIZES - 1);
    GET_REAL_SIZE(mob_proto + i) = num_arg;
  }

  CASE("Walkin")
  {
    mob_proto[i].player.walkin = strdup(value);
  }

  CASE("Walkout")
  {
    mob_proto[i].player.walkout = strdup(value);
  }

  CASE("EchoZone")
  {
    RANGE(0, 1);
    ECHO_IS_ZONE(mob_proto + i) = num_arg;
  }

  CASE("EchoFreq")
  {
    RANGE(0, 100);
    ECHO_FREQ(mob_proto + i) = num_arg;
  }

  CASE("EchoCount")
  {
    // RANGE(0, 20);
    // ECHO_COUNT(mob_proto + i) = num_arg;
    // ECHO_ENTRIES(mob_proto + i) = new char*[num_arg];
    CREATE(ECHO_ENTRIES(mob_proto + i), char *, num_arg);
  }

  CASE("EchoSequential")
  {
    RANGE(0, 1);
    ECHO_SEQUENTIAL(mob_proto + i) = num_arg;
  }

  CASE("Echo")
  {
    ECHO_ENTRIES(mob_proto + i)
    [ECHO_COUNT(mob_proto + i)] = strdup(value);
    ECHO_COUNT(mob_proto + i)
    ++;
  }

  CASE("Path")
  {
    const char *temp = value;

    /* i'm commenting this out, it just creates spam in the log file -zusuk */
    // log("Path encountered in ESpec.");
    PATH_SIZE(&mob_proto[i]) = 0;
    PATH_RESET(&mob_proto[i]) = num_arg;
    PATH_DELAY(&mob_proto[i]) = PATH_RESET(&mob_proto[i]);
    // parse rest to add paths...
    while (*temp != ':' && *temp != 0)
      temp++;

    if (*temp == ':') // else we get a 0 at start...
      temp++;
    while (*temp != 0)
    {
      room_vnum room = atoi(temp);
      if (room)
      {
        /* too much spam in log file -zusuk */
        // log("Path Index = %d  (Current Size %d)", room, PATH_SIZE(&mob_proto[i]));
        GET_PATH(&mob_proto[i], PATH_SIZE(&mob_proto[i])++) = room;
      }
      temp++;
      while (*temp != ' ' && *temp != 0)
        temp++;
    }
  }

  if (!matched)
  {
    log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
        keyword, nr);
  }
  affect_total(mob_proto + i);
}

#undef CASE
#undef BOOL_CASE
#undef RANGE

static void parse_espec(char *buf, int i, int nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL)
  {
    *(ptr++) = '\0';
    while (isspace_ignoretabs(*ptr))
      ptr++;
  }
  interpret_espec(buf, ptr, i, nr);
}

/* these are the enhanced (espec) mobiles */
static void parse_enhanced_mob(FILE *mob_f, int i, int nr)
{
  char line[READ_SIZE];

  parse_simple_mob(mob_f, i, nr);

  while (get_line(mob_f, line))
  {
    if (!strcmp(line, "E")) /* end of the enhanced section */
      return;
    else if (*line == '#')
    { /* we've hit the next mob, maybe? */
      log("SYSERR: Unterminated E section in mob #%d", nr);
      exit(1);
    }
    else
      parse_espec(line, i, nr);
  }

  log("SYSERR: Unexpected end of file reached after mob #%d", nr);
  exit(1);
}

/* this will read from file the mobile info and overlay it on prototypes */
void parse_mobile(FILE *mob_f, int nr)
{
  static int i = 0;
  int j, t[10], retval, counter;
  char line[READ_SIZE], *tmpptr, letter;
  char f1[128], f2[128], f3[128], f4[128], f5[128], f6[128], f7[128], f8[128], buf2[128];
  //  char walk[MAX_STRING_LENGTH] = {'\0'};
  //  char *message;

  mob_index[i].vnum = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  /* Mobiles should NEVER use anything in the 'player_specials' structure.
   * The only reason we have every mob in the game share this copy of the
   * structure is to save newbie coders from themselves. -gg */
  mob_proto[i].player_specials = &dummy_mob;
  snprintf(buf2, sizeof(buf2), "mob vnum %d", nr); /* sprintf: OK (for 'buf2 >= 19') */

  /* String data */
  mob_proto[i].player.name = fread_string(mob_f, buf2);
  tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
        !str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);
  mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
  mob_proto[i].player.description = fread_string(mob_f, buf2);
  GET_TITLE(mob_proto + i) = NULL;

  /* Numeric data */
  if (!get_line(mob_f, line))
  {
    log("SYSERR: Format error after string section of mob #%d\n"
        "...expecting line of form '# # # {S | E}', but file ended!",
        nr);
    exit(1);
  }

  if (((retval = sscanf(line, "%s %s %s %s %s %s %s %s %d %c", f1, f2, f3, f4, f5, f6, f7, f8, t + 2, &letter)) != 10) && (bitwarning == TRUE))
  {
    /* Let's make the implementor read some, before converting his world files. */
    log("WARNING: Conventional mobile files detected. See config.c.");
    exit(1);
  }
  else if ((retval == 4) && (bitwarning == FALSE))
  {
    log("Converting mobile #%d to 128bits..", nr);
    MOB_FLAGS(mob_proto + i)
    [0] = asciiflag_conv(f1);
    MOB_FLAGS(mob_proto + i)
    [1] = 0;
    MOB_FLAGS(mob_proto + i)
    [2] = 0;
    MOB_FLAGS(mob_proto + i)
    [3] = 0;
    check_bitvector_names(MOB_FLAGS(mob_proto + i)[0], action_bits_count, buf2, "mobile");

    AFF_FLAGS(mob_proto + i)
    [0] = asciiflag_conv_aff(f2);
    AFF_FLAGS(mob_proto + i)
    [1] = 0;
    AFF_FLAGS(mob_proto + i)
    [2] = 0;
    AFF_FLAGS(mob_proto + i)
    [3] = 0;

    GET_ALIGNMENT(mob_proto + i) = atoi(f3);

    /* Make some basic checks. */
    REMOVE_BIT_AR(AFF_FLAGS(mob_proto + i), AFF_CHARM);
    REMOVE_BIT_AR(AFF_FLAGS(mob_proto + i), AFF_POISON);
    REMOVE_BIT_AR(AFF_FLAGS(mob_proto + i), AFF_ACID_COAT);
    REMOVE_BIT_AR(AFF_FLAGS(mob_proto + i), AFF_SLEEP);
#if !defined(CAMPAIGN_DL) && !defined(CAMPAIGN_FR)
    if (MOB_FLAGGED(mob_proto + i, MOB_AGGRESSIVE) && MOB_FLAGGED(mob_proto + i, MOB_AGGR_GOOD))
      REMOVE_BIT_AR(MOB_FLAGS(mob_proto + i), MOB_AGGR_GOOD);
    if (MOB_FLAGGED(mob_proto + i, MOB_AGGRESSIVE) && MOB_FLAGGED(mob_proto + i, MOB_AGGR_NEUTRAL))
      REMOVE_BIT_AR(MOB_FLAGS(mob_proto + i), MOB_AGGR_NEUTRAL);
    if (MOB_FLAGGED(mob_proto + i, MOB_AGGRESSIVE) && MOB_FLAGGED(mob_proto + i, MOB_AGGR_EVIL))
      REMOVE_BIT_AR(MOB_FLAGS(mob_proto + i), MOB_AGGR_EVIL);
#endif

    check_bitvector_names(AFF_FLAGS(mob_proto + i)[0], affected_bits_count, buf2, "mobile affect");

    /* This is necessary, since if we have conventional world files, &letter is
     * loaded into f4 instead of the letter characters. So what we do, is copy
     * f4 into letter. Disadvantage is that &letter cannot be longer then 128
     * characters, but this shouldn't occur anyway. */
    letter = *f4;

    if (bitsavetodisk)
    {
      add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 0);
      converting = TRUE;
    }

    log("   done.");
  }
  else if (retval == 10)
  {
    int taeller;

    MOB_FLAGS(mob_proto + i)
    [0] = asciiflag_conv(f1);
    MOB_FLAGS(mob_proto + i)
    [1] = asciiflag_conv(f2);
    MOB_FLAGS(mob_proto + i)
    [2] = asciiflag_conv(f3);
    MOB_FLAGS(mob_proto + i)
    [3] = asciiflag_conv(f4);
    for (taeller = 0; taeller < AF_ARRAY_MAX; taeller++)
      check_bitvector_names(MOB_FLAGS(mob_proto + i)[taeller], action_bits_count, buf2, "mobile");

    AFF_FLAGS(mob_proto + i)
    [0] = asciiflag_conv(f5);
    AFF_FLAGS(mob_proto + i)
    [1] = asciiflag_conv(f6);
    AFF_FLAGS(mob_proto + i)
    [2] = asciiflag_conv(f7);
    AFF_FLAGS(mob_proto + i)
    [3] = asciiflag_conv(f8);

    GET_ALIGNMENT(mob_proto + i) = t[2];

    for (taeller = 0; taeller < AF_ARRAY_MAX; taeller++)
      check_bitvector_names(AFF_FLAGS(mob_proto + i)[taeller], affected_bits_count, buf2, "mobile affect");
  }
  else
  {
    log("SYSERR: Format error after string section of mob #%d\n ...expecting line of form '# # # {S | E}'", nr);
    exit(1);
  }

  SET_BIT_AR(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
  if (MOB_FLAGGED(mob_proto + i, MOB_NOTDEADYET))
  {
    /* Rather bad to load mobiles with this bit already set. */
    log("SYSERR: Mob #%d has reserved bit MOB_NOTDEADYET set.", nr);
    REMOVE_BIT_AR(MOB_FLAGS(mob_proto + i), MOB_NOTDEADYET);
  }

  for (counter = 0; counter < NUM_FEATS; counter++)
    MOB_SET_FEAT((mob_proto + i), counter, 0);

  switch (UPPER(letter))
  {
  case 'S': /* Simple monsters */
    parse_simple_mob(mob_f, i, nr);
    break;
  case 'E': /* Circle3 Enhanced monsters */
    parse_enhanced_mob(mob_f, i, nr);
    break;
    /* add new mob types here.. */
  default:
    log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
    exit(1);
  }

  /* DG triggers -- script info follows mob S/E section */
  letter = fread_letter(mob_f);
  ungetc(letter, mob_f);
  while (letter == 'T')
  {
    dg_read_trigger(mob_f, &mob_proto[i], MOB_TRIGGER);
    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
  }

  mob_proto[i].aff_abils = mob_proto[i].real_abils;

  for (j = 0; j < NUM_WEARS; j++)
    mob_proto[i].equipment[j] = NULL;

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

  top_of_mobt = i++;
}

/* read all objects from obj file; generate index and prototypes */
const char *parse_object(FILE *obj_f, int nr)
{
  static int i = 0;
  static char line[READ_SIZE];
  int t[16], j, retval, wsplnum;
  char *tmpptr, buf2[128], f1[READ_SIZE], f2[READ_SIZE], f3[READ_SIZE], f4[READ_SIZE];
  char f5[READ_SIZE], f6[READ_SIZE], f7[READ_SIZE], f8[READ_SIZE];
  char f9[READ_SIZE], f10[READ_SIZE], f11[READ_SIZE], f12[READ_SIZE];
  struct extra_descr_data *new_descr;
  struct obj_special_ability *new_specab;

  obj_index[i].vnum = nr;
  obj_index[i].number = 0;
  obj_index[i].func = NULL;

  clear_object(obj_proto + i);
  obj_proto[i].item_number = i;

  snprintf(buf2, sizeof(buf2), "object #%d", nr); /* sprintf: OK (for 'buf2 >= 19') */

  /* string data */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL)
  {
    log("SYSERR: Null obj name or format error at or near %s", buf2);
    exit(1);
  }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
        !str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    CAP(tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  /* numeric data */
  if (!get_line(obj_f, line))
  {
    log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  if (((retval = sscanf(line, " %d %s %s %s %s %s %s %s %s %s %s %s %s", t, f1, f2, f3,
                        f4, f5, f6, f7, f8, f9, f10, f11, f12)) == 4) &&
      (bitwarning == TRUE))
  {
    /* Let's make the implementor read some, before converting his world files. */
    log("WARNING: Conventional object files detected. Please see config.c.");
    exit(1);
  }
  else if (((retval == 4) || (retval == 3)) && (bitwarning == FALSE))
  {

    if (retval == 3)
      t[3] = 0;
    else if (retval == 4)
      t[3] = asciiflag_conv_aff(f3);

    log("Converting object #%d to 128bits..", nr);
    GET_OBJ_EXTRA(obj_proto + i)
    [0] = asciiflag_conv(f1);
    GET_OBJ_EXTRA(obj_proto + i)
    [1] = 0;
    GET_OBJ_EXTRA(obj_proto + i)
    [2] = 0;
    GET_OBJ_EXTRA(obj_proto + i)
    [3] = 0;
    GET_OBJ_WEAR(obj_proto + i)
    [0] = asciiflag_conv(f2);
    GET_OBJ_WEAR(obj_proto + i)
    [1] = 0;
    GET_OBJ_WEAR(obj_proto + i)
    [2] = 0;
    GET_OBJ_WEAR(obj_proto + i)
    [3] = 0;
    GET_OBJ_PERM(obj_proto + i)
    [0] = asciiflag_conv_aff(f3);
    GET_OBJ_PERM(obj_proto + i)
    [1] = 0;
    GET_OBJ_PERM(obj_proto + i)
    [2] = 0;
    GET_OBJ_PERM(obj_proto + i)
    [3] = 0;
    
    
    if (bitsavetodisk)
    {
      add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 1);
      converting = TRUE;
    }

    log("   done.");
  }
  else if (retval == 13)
  {

    GET_OBJ_EXTRA(obj_proto + i)
    [0] = asciiflag_conv(f1);
    GET_OBJ_EXTRA(obj_proto + i)
    [1] = asciiflag_conv(f2);
    GET_OBJ_EXTRA(obj_proto + i)
    [2] = asciiflag_conv(f3);
    GET_OBJ_EXTRA(obj_proto + i)
    [3] = asciiflag_conv(f4);
    GET_OBJ_WEAR(obj_proto + i)
    [0] = asciiflag_conv(f5);
    GET_OBJ_WEAR(obj_proto + i)
    [1] = asciiflag_conv(f6);
    GET_OBJ_WEAR(obj_proto + i)
    [2] = asciiflag_conv(f7);
    GET_OBJ_WEAR(obj_proto + i)
    [3] = asciiflag_conv(f8);
    GET_OBJ_PERM(obj_proto + i)
    [0] = asciiflag_conv(f9);
    GET_OBJ_PERM(obj_proto + i)
    [1] = asciiflag_conv(f10);
    GET_OBJ_PERM(obj_proto + i)
    [2] = asciiflag_conv(f11);
    GET_OBJ_PERM(obj_proto + i)
    [3] = asciiflag_conv(f12);
  }
  else
  {
    log("SYSERR: Format error in first numeric line (expecting 13 args, got %d), %s", retval, buf2);
    exit(1);
  }

  /* Set 'bound' value as NOBODY regardless, as object prototypes should NEVER
   * bind to a player.  This is done in rent files only!  - Jamdog - 8/21/07 */
  GET_OBJ_BOUND_ID(obj_proto + i) = NOBODY;

  /* Object flags checked in check_object(). */
  GET_OBJ_TYPE(obj_proto + i) = t[0];

  // Weapons must be wielded. Cannot use the hold command on them.
  // We'll remove any hold wear flags to ensure this isn't possible
  if (GET_OBJ_TYPE(obj_proto + i) == ITEM_WEAPON)
  {
    REMOVE_BIT_AR(GET_OBJ_WEAR(obj_proto + i), ITEM_WEAR_HOLD);
  }

  if (!get_line(obj_f, line))
  {
    log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  /* Initialize the t array. */
  for (j = 0; j < NUM_OBJ_VAL_POSITIONS; j++)
    t[j] = 0;

  if ((retval = sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                       &t[0], &t[1], &t[2], &t[3], &t[4], &t[5], &t[6], &t[7], &t[8],
                       &t[9], &t[10], &t[11], &t[12], &t[13], &t[14], &t[15])) != 4)
  {
    if (retval != 16)
    {
      log("SYSERR: Format error in second numeric line (expecting 4 or 16 args, got %d), %s", retval, buf2);
      exit(1);
    }
    /* this is just creating tons of spam -zusuk */
    // log("INFO: Loaded old file version, converting from 4 to 16 object values.");
  }

  GET_OBJ_VAL(obj_proto + i, 0) = t[0];
  GET_OBJ_VAL(obj_proto + i, 1) = t[1];
  GET_OBJ_VAL(obj_proto + i, 2) = t[2];
  GET_OBJ_VAL(obj_proto + i, 3) = t[3];
  GET_OBJ_VAL(obj_proto + i, 4) = t[4];
  GET_OBJ_VAL(obj_proto + i, 5) = t[5];
  GET_OBJ_VAL(obj_proto + i, 6) = t[6];
  GET_OBJ_VAL(obj_proto + i, 7) = t[7];
  GET_OBJ_VAL(obj_proto + i, 8) = t[8];
  GET_OBJ_VAL(obj_proto + i, 9) = t[9];
  GET_OBJ_VAL(obj_proto + i, 10) = t[10];
  GET_OBJ_VAL(obj_proto + i, 11) = t[11];
  GET_OBJ_VAL(obj_proto + i, 12) = t[12];
  GET_OBJ_VAL(obj_proto + i, 13) = t[13];
  GET_OBJ_VAL(obj_proto + i, 14) = t[14];
  GET_OBJ_VAL(obj_proto + i, 15) = t[15];

  if (!get_line(obj_f, line))
  {
    log("SYSERR: Expecting third numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4)) != 5)
  {
    if (retval == 3)
    {
      t[3] = 0;
      t[4] = 0;
    }
    else if (retval == 4)
      t[4] = 0;
    else
    {
      log("SYSERR: Format error in third numeric line (expecting 5 args, got %d), %s", retval, buf2);
      exit(1);
    }
  }

  GET_OBJ_WEIGHT(obj_proto + i) = t[0];
  GET_OBJ_COST(obj_proto + i) = t[1];
  GET_OBJ_RENT(obj_proto + i) = t[2];
  GET_OBJ_LEVEL(obj_proto + i) = t[3];
  GET_OBJ_TIMER(obj_proto + i) = t[4];

  if (GET_OBJ_LEVEL(obj_proto + i) < 1)
    GET_OBJ_LEVEL(obj_proto + i) = 1;
  if (GET_OBJ_LEVEL(obj_proto + i) > 30)
    GET_OBJ_LEVEL(obj_proto + i) = 30;

  obj_proto[i].sitting_here = NULL;

  /* check to make sure that weight of containers exceeds curr. quantity */
  if (GET_OBJ_TYPE(obj_proto + i) == ITEM_DRINKCON ||
      GET_OBJ_TYPE(obj_proto + i) == ITEM_FOUNTAIN)
  {
    if (GET_OBJ_WEIGHT(obj_proto + i) < GET_OBJ_VAL(obj_proto + i, 1) && CAN_WEAR(obj_proto + i, ITEM_WEAR_TAKE))
      GET_OBJ_WEIGHT(obj_proto + i) = GET_OBJ_VAL(obj_proto + i, 1) + 5;
  }

  /* extra descriptions and affect fields */
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
  {
    obj_proto[i].affected[j].location = APPLY_NONE;
    obj_proto[i].affected[j].modifier = 0;
  }

  strlcat(buf2, ", after numeric constants\n" /* strcat: OK (for 'buf2 >= 87') */
                "...expecting 'E', 'A', '$', or next object number",
          sizeof(buf2));
  j = 0;
  wsplnum = 0;

  for (;;)
  {
    if (!get_line(obj_f, line))
    {
      log("SYSERR: Format error in %s", buf2);
      exit(1);
    }
    switch (*line)
    {
    case 'A':
      if (j >= MAX_OBJ_AFFECT)
      {
        log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
        exit(1);
      }
      if (!get_line(obj_f, line))
      {
        log("SYSERR: Format error in 'A' field, %s\n"
            "...expecting 2 numeric constants but file ended!",
            buf2);
        exit(1);
      }

      if ((retval = sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3)) != 4)
      {
        if (retval == 3)
        {
          obj_proto[i].affected[j].location = t[0];
          obj_proto[i].affected[j].modifier = t[1];
          obj_proto[i].affected[j].bonus_type = t[2];
        }
        else if (retval == 2)
        {
          /* Old object verison, no bonus type.*/
          t[2] = BONUS_TYPE_UNDEFINED;
        }
        else
        {
          log("SYSERR: Format error in 'A' field, %s\n"
              "...expecting 2 or 3 numeric arguments, got %d\n"
              "...offending line: '%s'",
              buf2, retval, line);
          exit(1);
        }
      }
      obj_proto[i].affected[j].location = t[0];
      obj_proto[i].affected[j].modifier = t[1];
      obj_proto[i].affected[j].bonus_type = t[2];
      obj_proto[i].affected[j].specific = t[3];
      j++;
      break;
    case 'B':
      if (j >= SPELLBOOK_SIZE)
      {
        log("SYSERR: Unknown spellbook slot in S field, %s", buf2);
        exit(1);
      }

      if (!get_line(obj_f, line))
      {
        log("SYSERR: Format error in 'S' field, %s\n"
            "...expecting 2 numeric constants but file ended!",
            buf2);
        exit(1);
      }

      if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2)
      {
        log("SYSERR: Format error in 'B' field, %s\n"
            "...expecting 2 numeric arguments, got %d\n"
            "...offending line: '%s'",
            buf2, retval, line);
        exit(1);
      }

      if (!obj_proto[i].sbinfo)
      {
        CREATE(obj_proto[i].sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
        memset((char *)obj_proto[i].sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
      }

      obj_proto[i].sbinfo[j].spellname = t[0];
      obj_proto[i].sbinfo[j].pages = t[1];
      j++;
      break;
    case 'C': /* Special abilities */
      CREATE(new_specab, struct obj_special_ability, 1);

      if (!get_line(obj_f, line))
      {
        log("SYSERR: Format error in 'C' field, %s\n"
            "...expecting 7 numeric constants but file ended!",
            buf2);
        exit(1);
      }
      if ((retval = sscanf(line, "%d %d %d %d %d %d %d %s", t,
                           t + 1,
                           t + 2,
                           t + 3,
                           t + 4,
                           t + 5,
                           t + 6,
                           f1)) < 7)
      {
        log("SYSERR: Format error in 'C' field, %s\n"
            "...expecting 7 numeric arguments, got %d\n"
            "...offending line: '%s'",
            buf2, retval, line);
        exit(1);
      }

      new_specab->ability = t[0];
      new_specab->level = t[1];
      new_specab->activation_method = t[2];
      new_specab->value[0] = t[3];
      new_specab->value[1] = t[4];
      new_specab->value[2] = t[5];
      new_specab->value[3] = t[6];
      new_specab->command_word = (retval == 8 ? strdup(f1) : NULL);

      new_specab->next = obj_proto[i].special_abilities;
      obj_proto[i].special_abilities = new_specab;
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(obj_f, buf2);
      new_descr->description = fread_string(obj_f, buf2);
      new_descr->next = obj_proto[i].ex_description;
      obj_proto[i].ex_description = new_descr;
      break;
    case 'G':
      if (!get_line(obj_f, line))
      {
        log("SYSERR: Format error in 'G' field, %s\n"
            "...expecting numeric constant but file ended!",
            buf2);
        exit(1);
      }
      if (sscanf(line, "%d", t) != 1)
      {
        log("SYSERR: Format error in 'G' field, %s\n"
            "...expecting numeric argument\n"
            "...offending line: '%s'",
            buf2, line);
        exit(1);
      }
      GET_OBJ_PROF(obj_proto + i) = t[0];
      break;
    case 'H':
      if (!get_line(obj_f, line))
      {
        log("SYSERR: Format error in 'H' field, %s\n"
            "...expecting numeric constant but file ended!",
            buf2);
        exit(1);
      }
      if (sscanf(line, "%d", t) != 1)
      {
        log("SYSERR: Format error in 'H' field, %s\n"
            "...expecting numeric argument\n"
            "...offending line: '%s'",
            buf2, line);
        exit(1);
      }
      GET_OBJ_MATERIAL(obj_proto + i) = t[0];
      break;
    case 'I':
      if (!get_line(obj_f, line))
      {
        log("SYSERR: Format error in 'I' field, %s\n"
            "...expecting numeric constant but file ended!",
            buf2);
        exit(1);
      }
      if (sscanf(line, "%d", t) != 1)
      {
        log("SYSERR: Format error in 'I' field, %s\n"
            "...expecting numeric argument\n"
            "...offending line: '%s'",
            buf2, line);
        exit(1);
      }
      GET_OBJ_SIZE(obj_proto + i) = t[0];
      if (GET_OBJ_SIZE(obj_proto + i) == 0) // cheesy conversion -zusuk
        GET_OBJ_SIZE(obj_proto + i) = SIZE_MEDIUM;
      break;
    case 'J':
      if (!get_line(obj_f, line))
      {
        log("SYSERR: Format error in 'J' field, %s\n"
            "...expecting numeric constant but file ended!",
            buf2);
        exit(1);
      }
      if (sscanf(line, "%d", t) != 1)
      {
        log("SYSERR: Format error in 'J' field, %s\n"
            "...expecting numeric argument\n"
            "...offending line: '%s'",
            buf2, line);
        exit(1);
      }
      (obj_proto + i)->mob_recepient = t[0];
      break;
    case 'K': // object activated spells
      if (!get_line(obj_f, line))
      {
        log("SYSERR: Format error in 'K' field, %s.  Expecting numeric constants, but file ended!", buf2);
        exit(1);
      }
      if ((retval = sscanf(line, " %d %d %d %d %d ", t, t + 1, t + 2, t + 3, t + 4)) != 5)
      {
        log("SYSERR: Format error in 'K' field, %s  expecting 5 numeric args, got %d.  line: '%s'",
            buf2, retval, line);
        exit(1);
      }
      obj_proto[i].activate_spell[ACT_SPELL_LEVEL] = t[0];
      obj_proto[i].activate_spell[ACT_SPELL_SPELLNUM] = t[1];
      obj_proto[i].activate_spell[ACT_SPELL_CURRENT_USES] = t[2];
      obj_proto[i].activate_spell[ACT_SPELL_MAX_USES] = t[3];
      obj_proto[i].activate_spell[ACT_SPELL_COOLDOWN] = t[4];
      break;
    case 'S': // weapon spells
      /*
              if (wsplnum >= MAX_WEAPON_SPELLS) {
                log("SYSERR: Too many A fields (%d max), %s", MAX_WEAPON_SPELLS, buf2);
                exit(1);
              }
         */
      if (!get_line(obj_f, line))
      {
        log("SYSERR: Format error in 'S' field, %s.  Expecting numeric constants, but file ended!", buf2);
        exit(1);
      }
      if ((retval = sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3)) != 4)
      {
        log("SYSERR: Format error in 'S' field, %s  expecting 4 numeric args, got %d.  line: '%s'",
            buf2, retval, line);
        exit(1);
      }
      obj_proto[i].has_spells = TRUE;
      obj_proto[i].wpn_spells[wsplnum].spellnum = t[0];
      obj_proto[i].wpn_spells[wsplnum].level = t[1];
      obj_proto[i].wpn_spells[wsplnum].percent = t[2];
      obj_proto[i].wpn_spells[wsplnum].inCombat = t[3];
      wsplnum++;
      break;
    case 'T': /* DG triggers */
      dg_obj_trigger(line, &obj_proto[i]);
      break;
    case '$':
    case '#':
      top_of_objt = i;
      check_object(obj_proto + i);
      i++;
      return (line);
    default:
      log("SYSERR: Format error in (%c): %s", *line, buf2);
      exit(1);
    }
  }
}

#define Z zone_table[zone]

/* load the zone table and command tables */
static void load_zones(FILE *fl, char *zonename)
{
  static zone_rnum zone = 0;
  int i, cmd_no, num_of_cmds = 0, line_num = 0, tmp, error, arg_count = 0;
  char *ptr, buf[READ_SIZE], zname[READ_SIZE], buf2[MAX_STRING_LENGTH] = {'\0'};
  int zone_fix = FALSE;
  char t1[80], t2[80];
  char zbuf1[MAX_STRING_LENGTH] = {'\0'}, zbuf2[MAX_STRING_LENGTH] = {'\0'};
  char zbuf3[MAX_STRING_LENGTH] = {'\0'}, zbuf4[MAX_STRING_LENGTH] = {'\0'};

  strlcpy(zname, zonename, sizeof(zname));

  /* Skip first 3 lines lest we mistake the zone name for a command. */
  for (tmp = 0; tmp < 3; tmp++)
    get_line(fl, buf);

  /* More accurate count. Previous was always 4 or 5 too high. -gg Note that if
   * a new zone command is added to reset_zone(), this string will need to be
   * updated to suit. - ae. */
  while (get_line(fl, buf))
    if ((strchr("MOPGERDTVJIL", buf[0]) && buf[1] == ' ') || (buf[0] == 'S' && buf[1] == '\0'))
      num_of_cmds++;

  rewind(fl);

  if (num_of_cmds == 0)
  {
    log("SYSERR: %s is empty!", zname);
    exit(1);
  }
  else
    CREATE(Z.cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);
  /* vnum expansion */
  //  if (sscanf(buf, "#%hd", &Z.number) != 1) {
  if (sscanf(buf, "#%d", &Z.number) != 1)
  {
    log("SYSERR: Format error in %s, line %d", zname, line_num);
    exit(1);
  }
  snprintf(buf2, sizeof(buf2), "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
    *ptr = '\0';
  Z.builders = strdup(buf);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = strdup(buf);
  parse_at(Z.name);

  /* Clear all the zone flags */
  for (i = 0; i < ZN_ARRAY_MAX; i++)
    Z.zone_flags[i] = 0;

  // had to change this block -zusuk
  line_num += get_line(fl, buf);

  /* vnum expansion
  if (sscanf(buf, " %hd %hd %d %d %s %s %s %s %d %d %d", &Z.bot, &Z.top, &Z.lifespan,
          &Z.reset_mode, zbuf1, zbuf2, zbuf3, zbuf4, &Z.min_level, &Z.max_level,
          &Z.show_weather) != 11) {
   */
  if (sscanf(buf, " %d %d %d %d %s %s %s %s %d %d %d %d %d %d", &Z.bot, &Z.top,
             &Z.lifespan, &Z.reset_mode, zbuf1, zbuf2, zbuf3, zbuf4, &Z.min_level,
             &Z.max_level, &Z.show_weather, &Z.region, &Z.faction, &Z.city) != 14)
  {
    // not 14 values, lets try 11
    if (sscanf(buf, " %d %d %d %d %s %s %s %s %d %d %d", &Z.bot, &Z.top,
              &Z.lifespan, &Z.reset_mode, zbuf1, zbuf2, zbuf3, zbuf4, &Z.min_level,
              &Z.max_level, &Z.show_weather) != 11)
    {
      // not 11 values, lets try 10
      if (sscanf(buf, " %d %d %d %d %s %s %s %s %d %d", &Z.bot, &Z.top, &Z.lifespan,
                &Z.reset_mode, zbuf1, zbuf2, zbuf3, zbuf4, &Z.min_level, &Z.max_level) != 10)
      {
        // not 10 values, last try for 4 values
        if (sscanf(buf, " %d %d %d %d ", &Z.bot, &Z.top, &Z.lifespan, &Z.reset_mode) != 4)
        {
          // attempt to fix: copy previous 2 last reads into this and last variable
          log("SYSERR: Format error in numeric constant line of %s, attempting to fix.", zname);
          if (sscanf(Z.name, " %d %d %d %d ", &Z.bot, &Z.top, &Z.lifespan, &Z.reset_mode) != 4)
          {
            log("SYSERR: Could not fix previous error, aborting game.");
            exit(1);
          }
          else
          {
            free(Z.name);
            Z.name = strdup(Z.builders);
            free(Z.builders);
            Z.builders = strdup("None.");
            zone_fix = TRUE;
          }
        }
        /* We only found 4 values, so set 'defaults' for the ones not found */
        Z.min_level = -1;
        Z.max_level = -1;
        Z.show_weather = 1;
      }
      else
      { // 10 values
        Z.zone_flags[0] = asciiflag_conv(zbuf1);
        Z.zone_flags[1] = asciiflag_conv(zbuf2);
        Z.zone_flags[2] = asciiflag_conv(zbuf3);
        Z.zone_flags[3] = asciiflag_conv(zbuf4);
        Z.show_weather = 1;
      }
    }
    else
    { // 11 values
      Z.zone_flags[0] = asciiflag_conv(zbuf1);
      Z.zone_flags[1] = asciiflag_conv(zbuf2);
      Z.zone_flags[2] = asciiflag_conv(zbuf3);
      Z.zone_flags[3] = asciiflag_conv(zbuf4);
    }
  }
  else
  { // 14 values
    Z.zone_flags[0] = asciiflag_conv(zbuf1);
    Z.zone_flags[1] = asciiflag_conv(zbuf2);
    Z.zone_flags[2] = asciiflag_conv(zbuf3);
    Z.zone_flags[3] = asciiflag_conv(zbuf4);
  }

  if (Z.bot > Z.top)
  {
    log("SYSERR: Zone %d bottom (%d) > top (%d).", Z.number, Z.bot, Z.top);
    exit(1);
  }

  cmd_no = 0;

  for (;;)
  {
    /* skip reading one line if we fixed above (line is correct already) */
    if (zone_fix != TRUE)
    {
      if ((tmp = get_line(fl, buf)) == 0)
      {
        log("SYSERR: Format error in %s - premature end of file", zname);
        exit(1);
      }
    }
    else
      zone_fix = FALSE;

    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD.command = *ptr) == '*')
      continue;

    ptr++;

    if (ZCMD.command == 'S' || ZCMD.command == '$')
    {
      ZCMD.command = 'S';
      break;
    }
    error = 0;
    if (strchr("MOGEPDTVJL", ZCMD.command) == NULL)
    { /* a 3-arg command */
      if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
        error = 1;
    }
    else if (ZCMD.command == 'V')
    { /* a string-arg command */
      if (sscanf(ptr, " %d %d %d %d %79s %79[^\f\n\r\t\v]", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                 &ZCMD.arg3, t1, t2) != 6)
        error = 1;
      else
      {
        ZCMD.sarg1 = strdup(t1);
        ZCMD.sarg2 = strdup(t2);
      }
    }
    else
    {
      switch (ZCMD.command)
      {
      case 'I': /* Load random treasure on mobile */
        arg_count = sscanf(ptr, " %d %d ", &tmp, &ZCMD.arg1);
        if (arg_count != 2)
          error = 1;
        break;
      case 'L': /* Load random treasure in container */
        arg_count = sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2);
        if (arg_count != 3)
          error = 1;
        break;
      case 'J':
        arg_count = sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2);
        if (arg_count == 2)
          ZCMD.arg2 = 100; /* defaults to 100%, always loads */
        else if (arg_count != 3)
          error = 1;
        break;
      case 'M':
      case 'O':
      case 'E':
      case 'P':
        arg_count = sscanf(ptr, " %d %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                           &ZCMD.arg3, &ZCMD.arg4);
        if (arg_count == 4 || ZCMD.arg4 < 0)
          ZCMD.arg4 = 100;
        else if (arg_count != 5)
          error = 1;
        break;
      case 'G':
        arg_count = sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                           &ZCMD.arg3);
        if (arg_count == 3 || ZCMD.arg3 < 0)
          ZCMD.arg3 = 100;
        else if (arg_count != 4)
          error = 1;
        break;
      case 'D':
      case 'T':
        if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                   &ZCMD.arg3) != 4)
          error = 1;
        break;
      case 'R':
        if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
          error = 1;
        break;
      default:
        error = 1;
        break;
      }
      // if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
      //         &ZCMD.arg3) != 4)
      //   error = 1;
    }

    ZCMD.if_flag = tmp;

    if (error)
    {
      log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
      exit(1);
    }
    ZCMD.line = line_num;
    cmd_no++;
  }

  if (num_of_cmds != cmd_no + 1)
  {
    log("SYSERR: Zone command count mismatch for %s. Estimated: %d, Actual: %d", zname, num_of_cmds, cmd_no + 1);
    exit(1);
  }

  top_of_zone_table = zone++;
}
#undef Z

static void get_one_line(FILE *fl, char *buf)
{
  if (fgets(buf, READ_SIZE, fl) == NULL)
  {
    log("SYSERR: error reading help file: not terminated with $?");
    exit(1);
  }

  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}

void free_help(struct help_index_element *hentry)
{
  //  if (hentry->index)
  //    free(hentry->index);
  if (hentry->keywords)
    free(hentry->keywords);
  if (hentry->entry && !hentry->duplicate)
    free(hentry->entry);

  free(hentry);
}

void free_help_table(void)
{
  if (help_table)
  {
    int hp;
    for (hp = 0; hp < top_of_helpt; hp++)
    {
      //      if (help_table[hp].index)
      //        free(help_table[hp].index);
      if (help_table[hp].keywords)
        free(help_table[hp].keywords);
      if (help_table[hp].entry && !help_table[hp].duplicate)
        free(help_table[hp].entry);
    }
    free(help_table);
    help_table = NULL;
  }
  top_of_helpt = 0;
}

void load_help(FILE *fl, char *name)
{
  char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
  size_t entrylen;
  char line[READ_SIZE + 1], hname[READ_SIZE + 1], *scan;
  struct help_index_element el;

  strlcpy(hname, name, sizeof(hname));

  get_one_line(fl, key);
  while (*key != '$')
  {
    strlcat(key, "\r\n", sizeof(key)); /* strcat: OK (READ_SIZE - "\n"  "\r\n" == READ_SIZE  1) */
    entrylen = strlcpy(entry, key, sizeof(entry));

    /* Read in the corresponding help entry. */
    get_one_line(fl, line);
    while (*line != '#' && entrylen < sizeof(entry) - 1)
    {
      entrylen += strlcpy(entry + entrylen, line, sizeof(entry) - entrylen);

      if (entrylen + 2 < sizeof(entry) - 1)
      {
        strcpy(entry + entrylen, "\r\n"); /* strcpy: OK (size checked above) */
        entrylen += 2;
      }
      get_one_line(fl, line);
    }

    if (entrylen >= sizeof(entry) - 1)
    {
      int keysize;
      const char *truncmsg = "\r\n*TRUNCATED*\r\n";

      strcpy(entry + sizeof(entry) - strlen(truncmsg) - 1, truncmsg); /* strcpy: OK (assuming sane 'entry' size) */

      keysize = strlen(key) - 2;
      log("SYSERR: Help entry exceeded buffer space: %.*s", keysize, key);

      /* If we ran out of buffer space, eat the rest of the entry. */
      while (*line != '#')
        get_one_line(fl, line);
    }

    if (*line == '#')
    {
      if (sscanf(line, "#%d", &el.min_level) != 1)
      {
        log("SYSERR: Help entry does not have a min level. %s", key);
        el.min_level = 0;
      }
    }

    el.duplicate = 0;
    el.entry = strdup(entry);
    parse_at(el.entry);
    scan = one_word(key, next_key);

    while (*next_key)
    {
      el.keywords = strdup(next_key);
      help_table[top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }
    get_one_line(fl, key);
  }
}

static int help_sort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *)a;
  b1 = (const struct help_index_element *)b;

  return (str_cmp(a1->keywords, b1->keywords));
}

int vnum_mobile(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_mobt; nr++)
    if (isname(searchname, mob_proto[nr].player.name))
      send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                   ++found, mob_index[nr].vnum, mob_proto[nr].player.short_descr,
                   mob_proto[nr].proto_script ? "[TRIG]" : "");

  return (found);
}

int vnum_object(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++)
    if (isname(searchname, obj_proto[nr].name))
      send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                   ++found, obj_index[nr].vnum, obj_proto[nr].short_description,
                   obj_proto[nr].proto_script ? "[TRIG]" : "");

  return (found);
}

int vnum_room(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_world; nr++)
    if (isname(searchname, world[nr].name))
      send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                   ++found, world[nr].number, world[nr].name,
                   world[nr].proto_script ? "[TRIG]" : "");
  return (found);
}

int vnum_trig(char *searchname, struct char_data *ch)
{
  int nr, found = 0;
  for (nr = 0; nr < top_of_trigt; nr++)
    if (isname(searchname, trig_index[nr]->proto->name))
      send_to_char(ch, "%3d. [%5d] %-40s\r\n",
                   ++found, trig_index[nr]->vnum, trig_index[nr]->proto->name);
  return (found);
}

/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
  struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);

  new_mobile_data(ch);
  /* Allocate mobile event list */
  // ch->events = create_list();

  ch->next = character_list;
  character_list = ch;

  GET_ID(ch) = max_mob_id++;
  /* find_char helper */
  add_to_lookup_table(GET_ID(ch), (void *)ch);

  return (ch);
}

void new_mobile_data(struct char_data *ch)
{
  ch->events = NULL;
  ch->group = NULL;

  /* Set up the action queues. */
  GET_QUEUE(ch) = create_action_queue();
  GET_ATTACK_QUEUE(ch) = create_attack_queue();
}

/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
  mob_rnum i;
  struct char_data *mob;

  if (type == VIRTUAL)
  {
    if ((i = real_mobile(nr)) == NOBODY)
    {
      log("WARNING: Mobile vnum %d does not exist in database.", nr);
      return (NULL);
    }
  }
  else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);

  *mob = mob_proto[i];
  mob->next = character_list;
  character_list = mob;

  new_mobile_data(mob);
  /* Allocate mobile event list */
  // mob->events = create_list();

  /* the very old, and really should be replaced system is as such for max-hp of mobiles:
    max-hp = <mobile hp> dice <mobile psp> + <mobile moves>
    AKA:  xDy + z
      this is triggered by when you see the current max-hit is zero (0) -zusuk  */
  if (!GET_MAX_HIT(mob))
  {
    GET_MAX_HIT(mob) = dice(GET_HIT(mob), GET_PSP(mob)) + GET_MOVE(mob);
  }
  else
    GET_MAX_HIT(mob) = rand_number(GET_HIT(mob), GET_PSP(mob));

  /* powerful being bump! -zusuk */
  if (IS_POWERFUL_BEING(mob))
  {
    GET_MAX_HIT(mob) += 500;

    if (GET_LEVEL(mob) > 30)
      GET_MAX_HIT(mob) += GET_MAX_HIT(mob) * 0.1;
    if (GET_LEVEL(mob) > 31)
      GET_MAX_HIT(mob) += GET_MAX_HIT(mob) * 0.1;
    if (GET_LEVEL(mob) > 32)
      GET_MAX_HIT(mob) += GET_MAX_HIT(mob) * 0.1;
    if (GET_LEVEL(mob) > 33)
      GET_MAX_HIT(mob) += GET_MAX_HIT(mob) * 0.1;
  }

  GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob);

  if (GET_SPELL_RES(mob) < 0)
    GET_SPELL_RES(mob) = 0;

  GET_HIT(mob) = GET_MAX_HIT(mob);
  GET_PSP(mob) = GET_REAL_MAX_PSP(mob) = GET_MAX_PSP(mob);
  GET_MOVE(mob) = GET_REAL_MAX_MOVE(mob) = GET_MAX_MOVE(mob);

  /* pos_fighting is deprecated */
  if (GET_DEFAULT_POS(mob) == POS_FIGHTING)
    GET_DEFAULT_POS(mob) = POS_STANDING;
  if (GET_POS(mob) == POS_FIGHTING)
    GET_POS(mob) = POS_STANDING;

  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);

  mob_index[i].number++;

  GET_ID(mob) = max_mob_id++;

  /* find_char helper */
  add_to_lookup_table(GET_ID(mob), (void *)mob);

  copy_proto_script(&mob_proto[i], mob, MOB_TRIGGER);
  assign_triggers(mob, MOB_TRIGGER);

  if (GET_RACE(mob) < 0 || GET_RACE(mob) >= NUM_RACE_TYPES)
    GET_REAL_RACE(mob) = 0;

  if (GET_CLASS(mob) < 0 || GET_CLASS(mob) >= NUM_CLASSES)
    GET_CLASS(mob) = CLASS_WARRIOR;

  if (GET_SIZE(mob) < 0 || GET_SIZE(mob) >= NUM_SIZES)
    GET_REAL_SIZE(mob) = SIZE_MEDIUM;

  //  this line of code can be used to randomize the classes of the world
  //  for testing purposes - zusuk
  //  GET_CLASS(mob) = rand_number(0, NUM_CLASSES - 1);

#if defined(CAMPAIGN_DL)
  if (!MOB_FLAGGED(mob, MOB_CUSTOM_GOLD))
    autoroll_mob(mob, FALSE, FALSE);
    GET_HIT(mob) = GET_MAX_HIT(mob);
#endif

  if (MOB_FLAGGED(mob, MOB_MOUNTABLE))
    GET_REAL_MAX_MOVE(mob) = 2000 + (GET_LEVEL(mob) * 200);

  return (mob);
}

/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next = object_list;
  object_list = obj;

  obj->events = NULL;
  obj->special_abilities = NULL; /* Ornir 19/08/2013 */

  GET_ID(obj) = max_obj_id++;
  /* find_obj helper */
  add_to_lookup_table(GET_ID(obj), (void *)obj);

  return (obj);
}

/* Hash table functions for fast object rnum lookups */

/* Initialize the object rnum hash table */
void init_obj_rnum_hash(void)
{
  int i;
  
  for (i = 0; i < OBJ_RNUM_HASH_SIZE; i++) {
    obj_rnum_hash[i].objs = NULL;
  }
}

/* Get hash bucket for an object rnum */
static int obj_rnum_hash_key(obj_rnum rnum)
{
  /* Simple modulo hash function */
  if (rnum < 0)
    return 0;
  return (rnum % OBJ_RNUM_HASH_SIZE);
}

/* Add an object to the rnum hash table */
void add_obj_to_rnum_hash(struct obj_data *obj)
{
  int hash_key;
  
  if (!obj || obj->item_number == NOTHING)
    return;
    
  hash_key = obj_rnum_hash_key(obj->item_number);
  
  /* Add to the front of the linked list in this bucket */
  obj->next_in_hash = obj_rnum_hash[hash_key].objs;
  obj->prev_in_hash = NULL;
  
  if (obj_rnum_hash[hash_key].objs)
    obj_rnum_hash[hash_key].objs->prev_in_hash = obj;
    
  obj_rnum_hash[hash_key].objs = obj;
}

/* Remove an object from the rnum hash table */
void remove_obj_from_rnum_hash(struct obj_data *obj)
{
  int hash_key;
  
  if (!obj || obj->item_number == NOTHING)
    return;
    
  hash_key = obj_rnum_hash_key(obj->item_number);
  
  /* Update the linked list pointers */
  if (obj->prev_in_hash)
    obj->prev_in_hash->next_in_hash = obj->next_in_hash;
  else
    obj_rnum_hash[hash_key].objs = obj->next_in_hash;
    
  if (obj->next_in_hash)
    obj->next_in_hash->prev_in_hash = obj->prev_in_hash;
    
  obj->next_in_hash = NULL;
  obj->prev_in_hash = NULL;
}

/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
  struct obj_data *obj;
  struct obj_special_ability *proto_specab, *specab_list;
  int j;
  obj_rnum i = type == VIRTUAL ? real_object(nr) : nr;

  if (i == NOTHING || i > top_of_objt)
  {
    log("Object (%c) %d does not exist in database.", type == VIRTUAL ? 'V' : 'R', nr);
    return (NULL);
  }

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;

  obj->events = NULL;

  /* Add object to rnum hash table for fast lookups */
  add_obj_to_rnum_hash(obj);

  obj_index[i].number++;

  GET_ID(obj) = max_obj_id++;
  /* find_obj helper */
  add_to_lookup_table(GET_ID(obj), (void *)obj);

  /* Copy the spellbook information - Uses pointer math to access and array...*/
  if (obj_proto[i].sbinfo)
  {
    CREATE(obj->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
    memset((char *)obj->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
    for (j = 0; j < SPELLBOOK_SIZE; j++)
    {
      obj->sbinfo[j].spellname = obj_proto[i].sbinfo[j].spellname;
      obj->sbinfo[j].pages = obj_proto[i].sbinfo[j].pages;
    }
  }

  obj->special_abilities = NULL;

  /* Copy the special ability information. */
  for (proto_specab = obj_proto[i].special_abilities;
       proto_specab != NULL;
       proto_specab = proto_specab->next)
  {

    CREATE(specab_list, struct obj_special_ability, 1);
    /* Populate the node. */
    *specab_list = *proto_specab;

    /* Copy the command word (pointer, not copied above. */
    if (proto_specab->command_word != NULL)
      specab_list->command_word = strdup(proto_specab->command_word);

    /* Put the new node on the list. */
    specab_list->next = obj->special_abilities;
    obj->special_abilities = specab_list;
  }

  /* going to put some caps here -zusuk */
  /* weapons, max_weapon_x is defined in oasis.h */
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
  {
    GET_OBJ_VAL(obj, 1) = MIN(MAX_WEAPON_NDICE, GET_OBJ_VAL(obj, 1));
    GET_OBJ_VAL(obj, 2) = MIN(MAX_WEAPON_SDICE, GET_OBJ_VAL(obj, 2));
  }
  /* no longer allowing untyped gear affection -zusuk */
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
  {
    if (obj->affected[j].modifier)
    {
      if (obj->affected[j].bonus_type == BONUS_TYPE_UNDEFINED)
      {
        obj->affected[j].bonus_type = BONUS_TYPE_ENHANCEMENT;
      }
    }
  }
  /* item cost cap */
  // GET_OBJ_COST(obj) = MIN(MAX(GET_OBJ_LEVEL(obj), 1) * 100, GET_OBJ_COST(obj));

  /* conversion for instrument system for bards */
  if (GET_OBJ_TYPE(obj) == ITEM_INSTRUMENT)
  {
    SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_INSTRUMENT);
  }

  copy_proto_script(&obj_proto[i], obj, OBJ_TRIGGER);
  assign_triggers(obj, OBJ_TRIGGER);

  if (OBJ_FLAGGED(obj, ITEM_SET_STATS_AT_LOAD))
  {
    int cost = GET_OBJ_COST(obj);
    int enhancement_bonus = GET_ENHANCEMENT_BONUS(obj);
    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
    {
      set_weapon_object(obj, GET_OBJ_VAL(obj, 0));
      GET_OBJ_COST(obj) = cost;
    }
    else if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    {
      set_armor_object(obj, GET_OBJ_VAL(obj, 1));
      GET_OBJ_COST(obj) = cost;
    }
    GET_OBJ_VAL(obj, 4) = enhancement_bonus;
    if (cost <= 100)
    {
      GET_OBJ_VAL(obj, 4) = 0;
    }

    if (GET_OBJ_LEVEL(obj) < 1)
      GET_OBJ_LEVEL(obj) = 1;
    if (GET_OBJ_LEVEL(obj) > 30)
      GET_OBJ_LEVEL(obj) = 30;

    // if (obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
    // {
    //   obj->activate_spell[ACT_SPELL_CURRENT_USES] = obj->activate_spell[ACT_SPELL_MAX_USES];
    // }

    // REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_SET_STATS_AT_LOAD);
  }

  return (obj);
}

#define ZO_DEAD 999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60)
  {
    /* one minute has passed NOT accurate unless PULSE_ZONE is a multiple of
     * PASSES_PER_SEC or a factor of 60 */

    timer = 0;

    /* since one minute has passed, increment zone ages */
    for (i = 0; i <= top_of_zone_table; i++)
    {
      if (zone_table[i].age < zone_table[i].lifespan &&
          zone_table[i].reset_mode)
        (zone_table[i].age)++;

      if (zone_table[i].age >= zone_table[i].lifespan &&
          zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode)
      {
        /* enqueue zone */

        CREATE(update_u, struct reset_q_element, 1);

        update_u->zone_to_reset = i;
        update_u->next = 0;

        if (!reset_q.head)
          reset_q.head = reset_q.tail = update_u;
        else
        {
          reset_q.tail->next = update_u;
          reset_q.tail = update_u;
        }

        zone_table[i].age = ZO_DEAD;
      }
    }
  } /* end - one minute has passed */

  /* Dequeue zones (if possible) and reset. This code is executed every x
   * seconds (i.e. PULSE_ZONE). */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
        is_empty(update_u->zone_to_reset))
    {
      reset_zone(update_u->zone_to_reset);
      mudlog(CMP, LVL_IMPL, FALSE, "\tnAuto zone reset: %s (Zone %d)",
             zone_table[update_u->zone_to_reset].name,
             zone_table[update_u->zone_to_reset].number);
      /* dequeue */
      if (update_u == reset_q.head)
        reset_q.head = reset_q.head->next;
      else
      {
        for (temp = reset_q.head; temp->next != update_u;
             temp = temp->next)
          ;

        if (!update_u->next)
          reset_q.tail = temp;

        temp->next = update_u->next;
      }

      free(update_u);
      break;
    }
}

int check_max_existing(mob_rnum mob_num, int max, room_rnum room)
{
  struct char_data *temp_mob = NULL;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  int count = 0;

  *buf = '\0';
  if ((room == NOWHERE) && (max < 0))
  {
    log("Illegal room for check_max_existing() in room.");
    return FALSE;
  }

  if (max == 0 && boot_time <= 1 && mob_index[mob_num].number == 0)
    return TRUE;

  if (max > 0)
  {
    if (mob_index[mob_num].number < max)
      return TRUE;
  }
  else
  {
    for (temp_mob = world[room].people; temp_mob; temp_mob = temp_mob->next_in_room)
      if (GET_MOB_RNUM(temp_mob) == mob_num)
        count++;

    if (count < abs(max))
      return TRUE;
  }

  // if we got here, then return FALSE
  return FALSE;
}

static void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
  mudlog(CMP, LVL_STAFF, TRUE, "SYSERR: zone file: %s", message);
  mudlog(CMP, LVL_STAFF, TRUE, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d", ZCMD.command, zone_table[zone].number, ZCMD.line);
}

/*
#define ZONE_ERROR(message) \
     { log_zone_error(zone, cmd_no, message); last_cmd = 0; }
 */
#define ZONE_ERROR(message)                \
  {                                        \
    log_zone_error(zone, cmd_no, message); \
    push_result(0);                        \
  }

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{
  int cmd_no = 0, jump = 0, total_rooms = 0, num_chests = 0, max_chests = 0;
  bool has_random_chests = false, has_random_traps = false;
  struct char_data *mob = NULL;
  struct obj_data *obj = NULL, *obj_to = NULL;
  room_vnum rvnum = 0;
  room_rnum rrnum = 0;
  struct char_data *tmob = NULL; /* for trigger assignment */
  struct obj_data *tobj = NULL;  /* for trigger assignment */

  /* CRITICAL: Set zone reset state to prevent race conditions */
  if (zone_table[zone].reset_state == ZONE_RESET_ACTIVE) {
    log("SYSERR: Zone %d already resetting - possible race condition detected!", 
        zone_table[zone].number);
    return;
  }
  
  zone_table[zone].reset_state = ZONE_RESET_ACTIVE;
  zone_table[zone].reset_start = time(0);
  
  init_result_q();

  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
  {

    if (jump > 0)
    {
      jump--;
      push_result(0);
      continue;
    }

    /* checking our if_flag if we need to jump around */
    if (!test_result(ZCMD.if_flag))
    {
      push_result(0);
      continue;
    }

    /* This is the list of actual zone commands. If any new zone commands are
     * added to the game, be certain to update the list of commands in load_zone
     * () so that the counting will still be correct. - ae. */
    switch (ZCMD.command)
    {

    case '*': /* ignore command */
      push_result(0);
      break;

    case 'J': /* jump over lines (with percentage chance) */
      if (rand_number(1, 100) <= ZCMD.arg2)
      {
        jump = ZCMD.arg1;
        push_result(1);
      }
      else
        push_result(0);
      break;

    case 'M': /* read a mobile (with percentage loads) */
      if ((check_max_existing(ZCMD.arg1, ZCMD.arg2, ZCMD.arg3) || (ZCMD.arg2 == 0 && boot_time <= 1)) &&
          rand_number(1, 100) <= ZCMD.arg4)
      {
        mob = read_mobile(ZCMD.arg1, REAL);

        if (ZONE_FLAGGED(GET_ROOM_ZONE(ZCMD.arg3), ZONE_WILDERNESS))
        {
          X_LOC(mob) = world[ZCMD.arg3].coords[0];
          Y_LOC(mob) = world[ZCMD.arg3].coords[1];
        }

        char_to_room(mob, ZCMD.arg3);
        load_mtrigger(mob);
        set_mob_grouping(mob); // attempts to group AFF_GROUP mobs (utils.c)
        tmob = mob;
        GET_MOB_LOADROOM(mob) = IN_ROOM(mob);

        push_result(1);
      }
      else
        push_result(0);
      tobj = NULL;
      break;

    case 'O': /* read an object (with percentage loads) */
      /* CRITICAL FIX: Validate array bounds BEFORE accessing obj_index */
      if (ZCMD.arg1 < 0 || ZCMD.arg1 > top_of_objt)
      {
        log("SYSERR: Zone %d cmd %d: Invalid object rnum %d in 'O' command", 
            zone_table[zone].number, cmd_no, ZCMD.arg1);
        push_result(0);
        break;
      }
      
      /* Check max existing and percentage */
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2 ||
           (ZCMD.arg2 == 0 && boot_time <= 1)) &&
          rand_number(1, 100) <= ZCMD.arg4)
      {
        if (ZCMD.arg3 != NOWHERE)
        {
          obj = read_object(ZCMD.arg1, REAL);
          /* CRITICAL FIX: Check for NULL object before use */
          if (!obj) {
            log("SYSERR: Zone %d cmd %d: Failed to create object vnum %d", 
                zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum);
            push_result(0);
            break;
          }
          obj_to_room(obj, ZCMD.arg3);
          push_result(1);
          load_otrigger(obj);
          tobj = obj;
        }
        else
        {
          /* CRITICAL FIX: Don't create objects with NOWHERE room unless needed for triggers/vars */
          /* Check if next commands will use this object */
          bool obj_will_be_used = false;
          int check_cmd;
          
          for (check_cmd = cmd_no + 1; zone_table[zone].cmd[check_cmd].command != 'S'; check_cmd++) {
            if (zone_table[zone].cmd[check_cmd].if_flag && result_q.size == 0) {
              /* Next command depends on this one but we haven't pushed success yet */
              break;
            }
            /* Check if next command uses the current object (T or V with arg1 == 1) */
            if ((zone_table[zone].cmd[check_cmd].command == 'T' || 
                 zone_table[zone].cmd[check_cmd].command == 'V') &&
                zone_table[zone].cmd[check_cmd].arg1 == OBJ_TRIGGER) {
              obj_will_be_used = true;
              break;
            }
            /* Stop checking if we hit a command that resets tobj */
            if (zone_table[zone].cmd[check_cmd].command == 'O' ||
                zone_table[zone].cmd[check_cmd].command == 'P' ||
                zone_table[zone].cmd[check_cmd].command == 'G' ||
                zone_table[zone].cmd[check_cmd].command == 'E' ||
                zone_table[zone].cmd[check_cmd].command == 'R') {
              break;
            }
          }
          
          if (!obj_will_be_used) {
            /* Don't create the object if it won't be used */
            log("SYSERR: Zone %d cmd %d: Skipping orphaned object %d with NOWHERE room",
                zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum);
            push_result(0);
            tobj = NULL;
          } else {
            obj = read_object(ZCMD.arg1, REAL);
            /* CRITICAL FIX: Check for NULL object before use */
            if (!obj) {
              log("SYSERR: Zone %d cmd %d: Failed to create object vnum %d (no room)", 
                  zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum);
              push_result(0);
              break;
            }
            IN_ROOM(obj) = NOWHERE;
            push_result(1);
            tobj = obj;
          }
        }
      }
      tmob = NULL;
      break;

    case 'P': /* object to object (with percentage loads) */
      /* CRITICAL FIX: Validate array bounds BEFORE accessing obj_index */
      if (ZCMD.arg1 < 0 || ZCMD.arg1 > top_of_objt) {
        log("SYSERR: Zone %d cmd %d: Invalid object rnum %d in 'P' command", 
            zone_table[zone].number, cmd_no, ZCMD.arg1);
        push_result(0);
        break;
      }
      
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2 ||
           (ZCMD.arg2 == 0 && boot_time <= 1)) &&
          rand_number(1, 100) <= ZCMD.arg4)
      {
        obj = read_object(ZCMD.arg1, REAL);
        /* CRITICAL FIX: Check for NULL object before use */
        if (!obj) {
          log("SYSERR: Zone %d cmd %d: Failed to create object vnum %d for 'P' command", 
              zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum);
          push_result(0);
          break;
        }
        
        if (!(obj_to = get_obj_num(ZCMD.arg3)))
        {
          /* CRITICAL FIX: Free the created object to prevent memory leak */
          extract_obj(obj);
          
          if (ZCMD.if_flag == 0)
          {
            ZONE_ERROR("target obj not found");
            // ZCMD.command = '*';
          }
          else
          {
            push_result(0);
          }
          break;
        }
        obj_to_obj(obj, obj_to);
        push_result(1);
        load_otrigger(obj);
        tobj = obj;
      }
      else {
        /* Add logging for debugging */
        if (obj_index[ZCMD.arg1].number > ZCMD.arg2 && ZCMD.arg2 > 0) {
          log("ZONE: Zone %d cmd %d: Object vnum %d at max count (%d/%d) for 'P' command",
              zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum, 
              obj_index[ZCMD.arg1].number, ZCMD.arg2);
        } else if (rand_number(1, 100) > ZCMD.arg4) {
          /* log("ZONE: Zone %d cmd %d: Object vnum %d failed percentage check (%d%%) for 'P' command",
              zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum, ZCMD.arg4); */
        }
        push_result(0);
      }
      tmob = NULL;
      break;

    case 'G': /* obj_to_char (with percentage loads) */
      /* CRITICAL FIX: Validate array bounds BEFORE accessing obj_index */
      if (ZCMD.arg1 < 0 || ZCMD.arg1 > top_of_objt) {
        log("SYSERR: Zone %d cmd %d: Invalid object rnum %d in 'G' command", 
            zone_table[zone].number, cmd_no, ZCMD.arg1);
        push_result(0);
        break;
      }
      
      if (!mob)
      {
        if (ZCMD.if_flag == 0)
        {
          char error[MAX_INPUT_LENGTH] = {'\0'};
          snprintf(error, sizeof(error), "attempt to give obj #%d to non-existant mob", obj_index[ZCMD.arg1].vnum);
          ZONE_ERROR(error);
          // ZCMD.command = '*';
        }
        else
          push_result(0);
        break;
      }
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2 &&
          (rand_number(1, 100) <= ZCMD.arg3))
      {
        obj = read_object(ZCMD.arg1, REAL);
        /* CRITICAL FIX: Check for NULL object before use */
        if (!obj) {
          log("SYSERR: Zone %d cmd %d: Failed to create object vnum %d for 'G' command", 
              zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum);
          push_result(0);
          break;
        }
        obj_to_char(obj, mob);
        load_otrigger(obj);
        tobj = obj;
        push_result(1);
      }
      else if ((ZCMD.arg2 == 0 && boot_time <= 1) &&
               (rand_number(1, 100) <= ZCMD.arg3))
      {
        obj = read_object(ZCMD.arg1, REAL);
        /* CRITICAL FIX: Check for NULL object before use */
        if (!obj) {
          log("SYSERR: Zone %d cmd %d: Failed to create object vnum %d for 'G' command (boot)", 
              zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum);
          push_result(0);
          break;
        }
        obj_to_char(obj, mob);
        load_otrigger(obj);
        tobj = obj;
        push_result(1);
      }
      else {
        /* Add logging for debugging */
        if (obj_index[ZCMD.arg1].number > ZCMD.arg2 && ZCMD.arg2 > 0) {
          log("ZONE: Zone %d cmd %d: Object vnum %d at max count (%d/%d) for 'G' command",
              zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum, 
              obj_index[ZCMD.arg1].number, ZCMD.arg2);
        } else if (rand_number(1, 100) > ZCMD.arg3) {
          /* log("ZONE: Zone %d cmd %d: Object vnum %d failed percentage check (%d%%) for 'G' command",
              zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum, ZCMD.arg3); */
        }
        push_result(0);
      }
      tmob = NULL;
      break;

    case 'I': /* random treasure to mobile (with percentage loads) */
      if (!mob)
      {
        char error[MAX_INPUT_LENGTH] = {'\0'};
        snprintf(error, sizeof(error), "attempt to give random treasure to non-existant mob");
        ZONE_ERROR(error);
        // ZCMD.command = '*';
        break;
      }
      if (rand_number(1, 100) <= ZCMD.arg1)
        load_treasure(mob);
      break;

    case 'L': /* random treasure to container (with percentage loads) */
      if (rand_number(1, 100) <= ZCMD.arg2)
      {
        if (!(obj_to = get_obj_num(ZCMD.arg3)))
        {
          ZONE_ERROR("target obj not found");
          // ZCMD.command = '*';
          break;
        }
        /* Unfinished */
        // load_treasure_in_obj(obj_to);
        push_result(1);
      }
      else
        push_result(0);
      break;

    case 'E': /* object to equipment list (with percentage loads) */
      /* CRITICAL FIX: Validate array bounds BEFORE accessing obj_index */
      if (ZCMD.arg1 < 0 || ZCMD.arg1 > top_of_objt) {
        log("SYSERR: Zone %d cmd %d: Invalid object rnum %d in 'E' command", 
            zone_table[zone].number, cmd_no, ZCMD.arg1);
        push_result(0);
        break;
      }
      
      if (!mob)
      {
        if (ZCMD.if_flag == 0)
        {
          char error[MAX_INPUT_LENGTH] = {'\0'};
          snprintf(error, sizeof(error), "trying to equip non-existant mob with "
                                         "obj #%d",
                   obj_index[ZCMD.arg1].vnum);
          ZONE_ERROR(error);
          // ZCMD.command = '*';
        }
        else
          push_result(0);
        break;
      }
      /* we have a mob */
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2 &&
          (rand_number(1, 100) <= ZCMD.arg4))
      {
        if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS)
        {
          char error[MAX_INPUT_LENGTH] = {'\0'};
          /* FIX: arg2 should be arg1 for object vnum */
          snprintf(error, sizeof(error), "invalid equipment pos number (mob %s, "
                                         "obj %d, pos %d)",
                   GET_NAME(mob), obj_index[ZCMD.arg1].vnum, ZCMD.arg3);
          ZONE_ERROR(error);
          // ZCMD.command = '*';
        }
        else
        {
          obj = read_object(ZCMD.arg1, REAL);
          /* CRITICAL FIX: Check for NULL object before use */
          if (!obj) {
            log("SYSERR: Zone %d cmd %d: Failed to create object vnum %d for 'E' command", 
                zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum);
            push_result(0);
            break;
          }
          IN_ROOM(obj) = IN_ROOM(mob);
          load_otrigger(obj);
          if (wear_otrigger(obj, mob, ZCMD.arg3))
          {
            IN_ROOM(obj) = NOWHERE;
            equip_char(mob, obj, ZCMD.arg3);
          }
          else
            obj_to_char(obj, mob);
          tobj = obj;
          push_result(1);
        }
      }
      else if ((ZCMD.arg2 == 0 && boot_time <= 1) &&
               (rand_number(1, 100) <= ZCMD.arg4))
      {
        if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS)
        {
          char error[MAX_INPUT_LENGTH] = {'\0'};
          /* FIX: arg2 should be arg1 for object vnum */
          snprintf(error, sizeof(error), "invalid equipment pos number (mob %s, "
                                         "obj %d, pos %d)",
                   GET_NAME(mob), obj_index[ZCMD.arg1].vnum, ZCMD.arg3);
          ZONE_ERROR(error);
          // ZCMD.command = '*';
        }
        else
        {
          obj = read_object(ZCMD.arg1, REAL);
          /* CRITICAL FIX: Check for NULL object before use */
          if (!obj) {
            log("SYSERR: Zone %d cmd %d: Failed to create object vnum %d for 'E' command (boot)", 
                zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum);
            push_result(0);
            break;
          }
          IN_ROOM(obj) = IN_ROOM(mob);
          load_otrigger(obj);
          if (wear_otrigger(obj, mob, ZCMD.arg3))
          {
            IN_ROOM(obj) = NOWHERE;
            equip_char(mob, obj, ZCMD.arg3);
          }
          else
            obj_to_char(obj, mob);
          tobj = obj;
          push_result(1);
        }
      }
      else {
        /* Add logging for debugging */
        if (obj_index[ZCMD.arg1].number > ZCMD.arg2 && ZCMD.arg2 > 0) {
          log("ZONE: Zone %d cmd %d: Object vnum %d at max count (%d/%d) for 'E' command",
              zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum, 
              obj_index[ZCMD.arg1].number, ZCMD.arg2);
        } else if (rand_number(1, 100) > ZCMD.arg4) {
          /* log("ZONE: Zone %d cmd %d: Object vnum %d failed percentage check (%d%%) for 'E' command",
              zone_table[zone].number, cmd_no, obj_index[ZCMD.arg1].vnum, ZCMD.arg4); */
        }
        push_result(0);
      }

      tmob = NULL;
      break;

    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL)
        extract_obj(obj);
      push_result(1);
      tmob = NULL;
      tobj = NULL;
      break;

    case 'D': /* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= DIR_COUNT ||
          (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL))
      {
        char error[MAX_INPUT_LENGTH] = {'\0'};
        snprintf(error, sizeof(error), "door does not exist in room %d - dir %d", world[ZCMD.arg1].number, ZCMD.arg2);
        ZONE_ERROR(error);
        // ZCMD.command = '*';
      }
      else
        switch (ZCMD.arg3)
        {
        case 0:
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_LOCKED);
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_CLOSED);
          break;
        case 1:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                     EX_LOCKED_EASY);
          break;
        case 2:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_EASY);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          break;
        case 3:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_EASY);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_EASY);
          break;
        case 4:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_EASY);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_MEDIUM);
          break;
        case 5:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_EASY);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_HARD);
          break;
        case 6:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_EASY);
          break;
        case 7:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_MEDIUM);
          break;
        case 8:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_HARD);
          break;
        case 9:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_MEDIUM);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_EASY);
          break;
        case 10:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_MEDIUM);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_MEDIUM);
          break;
        case 11:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_MEDIUM);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_HARD);
          break;
        case 12:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_MEDIUM);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          break;
        case 13:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_HARD);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_EASY);
          break;
        case 14:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_HARD);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_MEDIUM);
          break;
        case 15:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_HARD);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_HIDDEN_HARD);
          break;
        case 16:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_LOCKED_HARD);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
                  EX_CLOSED);
          break;
        }

      push_result(1);
      tmob = NULL;
      tobj = NULL;
      break;

    case 'T': /* trigger command */
      if (ZCMD.arg1 == MOB_TRIGGER && tmob)
      {
        if (!SCRIPT(tmob))
          CREATE(SCRIPT(tmob), struct script_data, 1);
        add_trigger(SCRIPT(tmob), read_trigger(ZCMD.arg2), -1);
        push_result(1);
      }
      else if (ZCMD.arg1 == OBJ_TRIGGER && tobj)
      {
        if (!SCRIPT(tobj))
          CREATE(SCRIPT(tobj), struct script_data, 1);
        add_trigger(SCRIPT(tobj), read_trigger(ZCMD.arg2), -1);
        push_result(1);
      }
      else if (ZCMD.arg1 == WLD_TRIGGER)
      {
        if (ZCMD.arg3 == NOWHERE || ZCMD.arg3 > top_of_world)
        {
          ZONE_ERROR("Invalid room number in trigger assignment");
          // ZCMD.command = '*';
        }
        if (!world[ZCMD.arg3].script)
          CREATE(world[ZCMD.arg3].script, struct script_data, 1);
        add_trigger(world[ZCMD.arg3].script, read_trigger(ZCMD.arg2), -1);
        push_result(1);
      }

      break;

    case 'V':
      if (ZCMD.arg1 == MOB_TRIGGER && tmob)
      {
        if (!SCRIPT(tmob))
        {
          ZONE_ERROR("Attempt to give variable to scriptless mobile");
          // ZCMD.command = '*';
        }
        else
          add_var(&(SCRIPT(tmob)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
                  ZCMD.arg3);
        push_result(1);
      }
      else if (ZCMD.arg1 == OBJ_TRIGGER && tobj)
      {
        if (!SCRIPT(tobj))
        {
          ZONE_ERROR("Attempt to give variable to scriptless object");
          // ZCMD.command = '*';
        }
        else
          add_var(&(SCRIPT(tobj)->global_vars), ZCMD.sarg1, ZCMD.sarg2,
                  ZCMD.arg3);
        push_result(1);
      }
      else if (ZCMD.arg1 == WLD_TRIGGER)
      {
        if (ZCMD.arg3 == NOWHERE || ZCMD.arg3 > top_of_world)
        {
          ZONE_ERROR("Invalid room number in variable assignment");
          // ZCMD.command = '*';
        }
        else
        {
          if (!(world[ZCMD.arg3].script))
          {
            ZONE_ERROR("Attempt to give variable to scriptless object");
            // ZCMD.command = '*';
          }
          else
            add_var(&(world[ZCMD.arg3].script->global_vars),
                    ZCMD.sarg1, ZCMD.sarg2, ZCMD.arg2);
          push_result(1);
        }
      }
      break;

    default:
      ZONE_ERROR("unknown cmd in reset table; cmd disabled");
      ZCMD.command = '*';
      break;
    } /* end zone command switch */
  }   /* end zone command list loop */

  zone_table[zone].age = 0;

  /* handle reset_wtrigger's */
  rvnum = zone_table[zone].bot;
  while (rvnum <= zone_table[zone].top)
  {
    rrnum = real_room(rvnum);
    if (rrnum != NOWHERE)
    {
      total_rooms++;
      reset_wtrigger(&world[rrnum]); 
      if (has_random_chests == false && (ROOM_FLAGGED(rrnum, ROOM_RANDOM_CHEST) || ZONE_FLAGGED(zone, ZONE_RANDOM_CHESTS)))
        has_random_chests = true;
      if (has_random_traps == false && (ROOM_FLAGGED(rrnum, ROOM_RANDOM_TRAP) || ZONE_FLAGGED(zone, ZONE_RANDOM_TRAPS)))
        has_random_traps = true;
      // we're not counting flagged rooms because the num_chests amount is only for calculating
      // how many chests a zone can have if it's flagged for random chests.  rooms flagged such
      // should always load a random chest, as long as there already isn't one in there, which
      // we'll check for elsewhere
      if (is_random_chest_in_room(rrnum) && !ROOM_FLAGGED(rrnum, ROOM_RANDOM_CHEST))
        num_chests++;
    }
    rvnum++;
  }

  max_chests = number_of_chests_per_zone(total_rooms);
  int num_loops = 0;

  // we'll place some random chests or traps
  if (has_random_chests || has_random_traps)
  {
    // Build list of eligible rooms to avoid repeated eligibility checks
    room_rnum *eligible_rooms = NULL;
    int eligible_count = 0;
    int eligible_capacity = 0;
    
    // Collect all eligible rooms once
    rvnum = zone_table[zone].bot;
    while (rvnum <= zone_table[zone].top)
    {
      rrnum = real_room(rvnum);
      if (rrnum != NOWHERE)
      {
        // Note: We check initial eligibility only - num_chests may change during placement
        if (can_place_random_chest_in_room(rrnum, total_rooms, num_chests))
        {
          // Grow array if needed
          if (eligible_count >= eligible_capacity)
          {
            eligible_capacity = eligible_capacity ? eligible_capacity * 2 : 64;
            RECREATE(eligible_rooms, room_rnum, eligible_capacity);
          }
          eligible_rooms[eligible_count++] = rrnum;
        }
      }
      rvnum++;
    }
    
    // Replicate original algorithm behavior but using cached eligible rooms
    while (max_chests > num_chests && num_loops < NUM_OF_ZONE_ROOMS_PER_RANDOM_CHEST)
    {
      int i;
      // Check each eligible room with the same 1/33 probability
      for (i = 0; i < eligible_count && max_chests > num_chests; i++)
      {
        // Re-check eligibility as num_chests has changed
        if (can_place_random_chest_in_room(eligible_rooms[i], total_rooms, num_chests) && 
            (dice(1, NUM_OF_ZONE_ROOMS_PER_RANDOM_CHEST) == 1))
        {
          place_random_chest(eligible_rooms[i], zone_table[zone].max_level, -1, -1, 25);
          num_chests++;
        }
      }
      num_loops++;
    }
    
    // Clean up
    if (eligible_rooms)
      free(eligible_rooms);
  }
  
  /* CRITICAL: Clear zone reset state */
  zone_table[zone].reset_state = ZONE_RESET_NORMAL;
  zone_table[zone].reset_start = 0;
  
  /* CRITICAL FIX: Clean up any orphaned objects created with NOWHERE room
   * that were never attached to anything via T or V commands */
  if (tobj && IN_ROOM(tobj) == NOWHERE && tobj->in_obj == NULL && 
      tobj->carried_by == NULL && tobj->worn_by == NULL) {
    /* Object was created but never used - extract it to prevent memory leak */
    log("SYSERR: Zone %d: Cleaning up orphaned object %d [%s] that was never attached",
        zone_table[zone].number, GET_OBJ_VNUM(tobj), tobj->short_description);
    extract_obj(tobj);
  }
}

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
  {
    if (STATE(i) != CON_PLAYING)
      continue;
    if (IN_ROOM(i->character) == NOWHERE)
      continue;
    if (world[IN_ROOM(i->character)].zone != zone_nr)
      continue;
    /* If an immortal has nohassle off, he counts as present. Added for testing
     * zone reset triggers -Welcor */
    if ((!IS_NPC(i->character)) && (GET_LEVEL(i->character) >= LVL_IMMORT) && (PRF_FLAGGED(i->character, PRF_NOHASSLE)))
      continue;

    return (0);
  }

  return (1);
}

/* Functions of a general utility nature. */

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, const char *error)
{
  char buf[MAX_STRING_LENGTH] = {'\0'}, tmp[513] = {'\0'};
  char *point = NULL;
  int done = 0, length = 0, templength = 0;

  do
  {
    memset(tmp, '\0', 513);
    if (!fgets(tmp, 512, fl))
    {
      log("SYSERR: fread_string: format error at or near %s", error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    /* now only removes trailing ~'s -- Welcor */

    point = strchr(tmp, '\0');
    if (point == NULL)
    {
      log("SYSERR: freed_string: end of string not found (db.c)");
      log("String: %s", tmp);
      exit(1);
    }

    /* Ensure we don't go before the beginning of tmp array */
    if (point > tmp) {
      for (point--; (point >= tmp && (*point == '\r' || *point == '\n')); point--)
        ;
    }

    if (point >= tmp && *point == '~')
    {
      *point = '\0';
      done = 1;
    }
    else
    {
      /* Move back to the last valid position if we went too far */
      if (point < tmp)
        point = tmp - 1;
      *(++point) = '\r';
      *(++point) = '\n';
      *(++point) = '\0';
    }

    templength = point - tmp;

    if (length + templength >= MAX_STRING_LENGTH)
    {
      log("SYSERR: fread_string: string too large (db.c)");
      log("%s", error);
      exit(1);
    }
    else
    {
      strcat(buf + length, tmp); /* strcat: OK (size checked above) */
      length += templength;
    }
  } while (!done);

  parse_at(buf);

  /* allocate space for the new string and copy it */
  return (strlen(buf) ? strdup(buf) : NULL);
}

/* fread_clean_string is the same as fread_string, but skips preceding spaces */
char *fread_clean_string(FILE *fl, const char *error)
{
  char buf[MAX_STRING_LENGTH] = {'\0'}, tmp[513] = {'\0'};
  char *point = NULL, c = '\0';
  int done = 0, length = 0, templength = 0;

  *buf = '\0';
  *tmp = '\0';

  do
  {
    if (feof(fl))
    {
      log("%s", "fread_clean_string: EOF encountered on read.");
      return 0;
    }
    c = getc(fl);
  } while (isspace(c));
  ungetc(c, fl);

  do
  {
    if (!fgets(tmp, 512, fl))
    {
      log("SYSERR: fread_clean_string: format error at or near %s", error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    /* now only removes trailing ~'s -- Welcor */
    point = strchr(tmp, '\0');
    /* Ensure we don't go before the beginning of tmp array */
    if (point > tmp) {
      for (point--; (point >= tmp && (*point == '\r' || *point == '\n')); point--)
        ;
    }
    if (point >= tmp && *point == '~')
    {
      *point = '\0';
      done = 1;
    }
    else
    {
      /* Move back to the last valid position if we went too far */
      if (point < tmp)
        point = tmp - 1;
      *(++point) = '\r';
      *(++point) = '\n';
      *(++point) = '\0';
    }

    templength = point - tmp;

    if (length + templength >= MAX_STRING_LENGTH)
    {
      log("SYSERR: fread_clean_string: string too large (db.c)");
      log("%s", error);
      exit(1);
    }
    else
    {
      strcat(buf + length, tmp); /* strcat: OK (size checked above) */
      length += templength;
    }
  } while (!done);

  parse_at(buf);

  /* allocate space for the new string and copy it */
  return (strlen(buf) ? strdup(buf) : NULL);
}

/* Read a numerical value from a given file */
int fread_number(FILE *fp)
{
  int number;
  bool sign;
  char c;

  do
  {
    if (feof(fp))
    {
      log("%s", "fread_number: EOF encountered on read.");
      return 0;
    }
    c = getc(fp);
  } while (isspace(c));

  number = 0;

  sign = FALSE;
  if (c == '+')
    c = getc(fp);
  else if (c == '-')
  {
    sign = TRUE;
    c = getc(fp);
  }

  if (!isdigit(c))
  {
    log("fread_number: bad format. (%c)", c);
    return 0;
  }

  while (isdigit(c))
  {
    if (feof(fp))
    {
      log("%s", "fread_number: EOF encountered on read.");
      return number;
    }
    number = number * 10 + c - '0';
    c = getc(fp);
  }

  if (sign)
    number = 0 - number;

  if (c == '|')
    number += fread_number(fp);
  else if (c != ' ')
    ungetc(c, fp);

  return number;
}

/* Read to end of line from a given file into a static buffer */
char *fread_line(FILE *fp)
{
  static char line[MAX_STRING_LENGTH] = {'\0'};
  char *pline;
  char c;
  int ln;

  pline = line;
  line[0] = '\0';
  ln = 0;

  /* Skip blanks.     */
  /* Read first char. */
  do
  {
    if (feof(fp))
    {
      log("fread_line: EOF encountered on read.");
      *pline = '\0';
      return (line);
    }
    c = getc(fp);
  } while (isspace(c));

  /* Un-Read first char */
  ungetc(c, fp);

  do
  {
    if (feof(fp))
    {
      log("fread_line: EOF encountered on read.");
      *pline = '\0';
      return (line);
    }
    c = getc(fp);
    *pline++ = c;
    ln++;
    if (ln >= (MAX_STRING_LENGTH - 1))
    {
      log("fread_line: line too long");
      break;
    }
  } while ((c != '\n') && (c != '\r'));

  do
  {
    c = getc(fp);
  } while (c == '\n' || c == '\r');

  ungetc(c, fp);
  pline--;
  *pline = '\0';

  /* Since tildes generally aren't found at the end of lines, this seems workable. Will enable reading old configs. */
  if (line[strlen(line) - 1] == '~')
    line[strlen(line) - 1] = '\0';

  return (line);
}

/* Read to end of line from a given file and convert to flag values, then return number of ints */
int fread_flags(FILE *fp, int *fg, int fg_size)
{
  char line[MAX_STRING_LENGTH] = {'\0'};
  char *pline, val_txt[MAX_INPUT_LENGTH] = {'\0'};
  const char *tmp_txt;
  char c;
  int ln, i;

  pline = line;
  line[0] = '\0';
  ln = 0;

  /* Skip blanks.     */
  /* Read first char. */
  do
  {
    if (feof(fp))
    {
      log("fread_flags: EOF encountered on read.");
      *pline = '\0';
      return (0);
    }
    c = getc(fp);
  } while (isspace(c));

  /* Un-Read first char */
  ungetc(c, fp);

  do
  {
    if (feof(fp))
    {
      log("fread_flags: EOF encountered on read.");
      *pline = '\0';
      return (0);
    }
    c = getc(fp);
    *pline++ = c;
    ln++;
    if (ln >= (MAX_STRING_LENGTH - 1))
    {
      log("fread_flags: line too long");
      break;
    }
  } while ((c != '\n') && (c != '\r'));

  do
  {
    c = getc(fp);
  } while (c == '\n' || c == '\r');

  ungetc(c, fp);
  pline--;
  *pline = '\0';

  /* Since tildes generally aren't found at the end of lines, this seems workable. Will enable reading old configs. */
  if (line[strlen(line) - 1] == '~')
    line[strlen(line) - 1] = '\0';

  /* We now have a line of text with all the flags on it - let's convert it */
  for (i = 0, tmp_txt = line; tmp_txt && *tmp_txt && i < fg_size; i++)
  {
    tmp_txt = one_argument(tmp_txt, val_txt, sizeof(val_txt)); /* Grab a number  */
    fg[i] = atoi(val_txt);                                     /* Convert to int */
  }

  return (i);
}

/* Read one word from a given file (into static buffer). */
char *fread_word(FILE *fp)
{
  static char word[MAX_STRING_LENGTH] = {'\0'};
  char *pword;
  char cEnd;

  do
  {
    if (feof(fp))
    {
      log("fread_word: EOF encountered on read.");
      word[0] = '\0';
      return word;
    }
    cEnd = getc(fp);
  } while (isspace(cEnd));

  if (cEnd == '\'' || cEnd == '"')
  {
    pword = word;
  }
  else
  {
    word[0] = cEnd;
    pword = word + 1;
    cEnd = ' ';
  }

  for (; pword < word + MAX_STRING_LENGTH; pword++)
  {
    if (feof(fp))
    {
      log("fread_word: EOF encountered on read.");
      *pword = '\0';
      return word;
    }
    *pword = getc(fp);
    if (cEnd == ' ' ? isspace(*pword) : *pword == cEnd)
    {
      if (cEnd == ' ')
        ungetc(*pword, fp);
      *pword = '\0';
      return word;
    }
  }
  log("fread_word: word too long");
  return NULL;
}

/* Read to end of line in a given file (for comments) */
void fread_to_eol(FILE *fp)
{
  char c;

  do
  {
    if (feof(fp))
    {
      log("%s", "fread_to_eol: EOF encountered on read.");
      return;
    }
    c = getc(fp);
  } while (c != '\n' && c != '\r');

  do
  {
    c = getc(fp);
  } while (c == '\n' || c == '\r');

  ungetc(c, fp);
}

/* Called to free all allocated follow_type structs */
static void free_followers(struct follow_type *k)
{
  if (!k)
    return;

  if (k->next)
    free_followers(k->next);

  k->follower = NULL;
  free(k);
}

/* release memory allocated for a char struct */
void free_char(struct char_data *ch)
{
  int i = 0;
  struct alias_data *a = NULL;

  /* Free the action queues for ALL characters, not just those with player_specials */
  if (GET_QUEUE(ch))
    free_action_queue(GET_QUEUE(ch));
  if (GET_ATTACK_QUEUE(ch))
    free_attack_queue(GET_ATTACK_QUEUE(ch));

  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob)
  {
    while ((a = GET_ALIASES(ch)) != NULL)
    {
      GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
      free_alias(a);
    }

    if (ch->player_specials->poofin)
      free(ch->player_specials->poofin);
    if (ch->player_specials->poofout)
      free(ch->player_specials->poofout);
    if (ch->player_specials->saved.account_name)
      free(ch->player_specials->saved.account_name);
    if (ch->player_specials->saved.completed_quests)
      free(ch->player_specials->saved.completed_quests);
    if (ch->player_specials->saved.autocquest_desc)
      free(ch->player_specials->saved.autocquest_desc);
    if (ch->player.background)
      free(ch->player.background);
    if (ch->player.goals)
      free(ch->player.goals);
    if (GET_HOST(ch))
      free(GET_HOST(ch));
    
    /* CRITICAL FIX: Free bag names to prevent memory leaks */
    for (i = 0; i <= MAX_BAGS; i++) {
      if (ch->player_specials->saved.bag_names[i]) {
        free(ch->player_specials->saved.bag_names[i]);
        ch->player_specials->saved.bag_names[i] = NULL;
      }
    }
    
    if (IS_NPC(ch))
      log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
  }

  if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == NOBODY))
  {
    /* if this is a player, or a non-prototyped non-player, free all */
    if (GET_NAME(ch))
      free(GET_NAME(ch));
    if (ch->player.title)
      free(ch->player.title);
    if (ch->player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description)
      free(ch->player.description);
    if (ch->player.walkin)
      free(ch->player.walkin);
    if (ch->player.walkout)
      free(ch->player.walkout);
    if (ch->player.imm_title)
      free(ch->player.imm_title);
    if (ch->player.eidolon_shortdescription)
      free(ch->player.eidolon_shortdescription);
    if (ch->player.eidolon_longdescription)
      free(ch->player.eidolon_longdescription);
    if (ch->player.eidolon_detaildescription)
      free(ch->player.eidolon_detaildescription);

    for (i = 0; i < NUM_HIST; i++)
      if (GET_HISTORY(ch, i))
        free_history(ch, i);

    /* free todo list - must be done before freeing player_specials */
    if (ch && ch->player_specials && GET_TODO(ch))
    {
      struct txt_block *tmp = GET_TODO(ch), *ftmp;
      while ((ftmp = tmp))
      {
        tmp = tmp->next;
        if (ftmp->text)
          free(ftmp->text);
        free(ftmp);
      }
      GET_TODO(ch) = NULL;
    }

    /* spell prep system - must be done before freeing player_specials */
    if (ch && ch->player_specials)
    {
      destroy_spell_prep_queue(ch);
      destroy_innate_magic_queue(ch);
      destroy_spell_collection(ch);
      destroy_known_spells(ch);
      
      /* free craft data strings */
      if (GET_CRAFT(ch).keywords)
        free(GET_CRAFT(ch).keywords);
      if (GET_CRAFT(ch).short_description)
        free(GET_CRAFT(ch).short_description);
      if (GET_CRAFT(ch).room_description)
        free(GET_CRAFT(ch).room_description);
      if (GET_CRAFT(ch).ex_description)
        free(GET_CRAFT(ch).ex_description);
    }

    if (ch->player_specials)
      free(ch->player_specials);

    if (ch->bags)
      free(ch->bags);

    /* free script proto list */
    free_proto_script(ch, MOB_TRIGGER);
  }
  else if ((i = GET_MOB_RNUM(ch)) != NOBODY)
  {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (ch->player.name && ch->player.name != mob_proto[i].player.name)
      free(ch->player.name);
    if (ch->player.title && ch->player.title != mob_proto[i].player.title)
      free(ch->player.title);
    if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description && ch->player.description != mob_proto[i].player.description)
      free(ch->player.description);
    if (ch->player.walkin && ch->player.walkin != mob_proto[i].player.walkin)
      free(ch->player.walkin);
    if (ch->player.walkout && ch->player.walkout != mob_proto[i].player.walkout)
      free(ch->player.walkout);
    /* free script proto list if it's not the prototype */
    if (ch->proto_script && ch->proto_script != mob_proto[i].proto_script)
      free_proto_script(ch, MOB_TRIGGER);
    if (ch == &mob_proto[i])
    {
      free_hlquest(ch);
      int j;
      for (j = 0; j < ECHO_COUNT(ch); j++)
        free(ECHO_ENTRIES(ch)[j]);
      free(ECHO_ENTRIES(ch));
    }
  }

  while (ch->affected)
    affect_remove_no_total(ch, ch->affected);

  /* free any assigned scripts */
  if (SCRIPT(ch))
    extract_script(ch, MOB_TRIGGER);

  /* Mud Events */
  if (ch->events != NULL)
  {
    if (ch->events->iSize > 0)
    {
      clear_char_event_list(ch);
    }
    if (ch->events)
      free_list(ch->events);
  }

  /* DR */
  if (GET_DR(ch) != NULL)
  {
    struct damage_reduction_type *dr, *tmp;
    dr = GET_DR(ch);
    while (dr != NULL)
    {
      tmp = dr;
      dr = dr->next;
      free(tmp);
    }
  }

  /* gonna clear the condensed combat data if it exists */
  if (CNDNSD(ch))
    free(CNDNSD(ch));
  CNDNSD(ch) = NULL;

  /* new version of free_followers take the followers pointer as arg */
  free_followers(ch->followers);

  if (ch->desc)
    ch->desc->character = NULL;

  /* find_char helper, when free_char is called with a blank character struct,
   * ID is set to 0, and has not yet been added to the lookup table. */
  if (GET_ID(ch) != 0)
    remove_from_lookup_table(GET_ID(ch));

  free(ch);
}

/* release memory allocated for an obj struct */
/* Free the special abilities linked list */
void free_obj_special_abilities(struct obj_special_ability *list)
{
  struct obj_special_ability *next;
  
  while (list) {
    next = list->next;
    if (list->command_word)
      free(list->command_word);
    free(list);
    list = next;
  }
}

void free_obj(struct obj_data *obj)
{
  if (GET_OBJ_RNUM(obj) == NOWHERE)
  {
    free_object_strings(obj);
    /* free script proto list */
    free_proto_script(obj, OBJ_TRIGGER);
  }
  else
  {
    free_object_strings_proto(obj);
    if (obj->proto_script != obj_proto[GET_OBJ_RNUM(obj)].proto_script)
      free_proto_script(obj, OBJ_TRIGGER);
  }

  /* free any assigned scripts */
  if (SCRIPT(obj))
    extract_script(obj, OBJ_TRIGGER);

  /* free special abilities list */
  if (obj->special_abilities)
    free_obj_special_abilities(obj->special_abilities);

  /* find_obj helper */
  remove_from_lookup_table(GET_ID(obj));

  free(obj);
}

/* Steps: 1: Read contents of a text file. 2: Make sure no one is using the
 * pointer in paging. 3: Allocate space. 4: Point 'buf' to it.
 * We don't want to free() the string that someone may be viewing in the pager.
 * page_string() keeps the internal strdup()'d copy on ->showstr_head and it
 * won't care if we delete the original.  Otherwise, strings are kept on
 * ->showstr_vector but we'll only match if the pointer is to the string we're
 * interested in and not a copy. If someone is reading a global copy we're
 * trying to replace, give everybody using it a different copy so as to avoid
 * special cases. */
static int file_to_string_alloc(const char *name, char **buf)
{
  int temppage;
  char temp[MAX_STRING_LENGTH] = {'\0'};
  struct descriptor_data *in_use;

  for (in_use = descriptor_list; in_use; in_use = in_use->next)
    if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
      return (-1);

  /* Lets not free() what used to be there unless we succeeded. */
  if (file_to_string(name, temp) < 0)
    return (-1);

  for (in_use = descriptor_list; in_use; in_use = in_use->next)
  {
    if (!in_use->showstr_count || *in_use->showstr_vector != *buf)
      continue;

    /* Let's be nice and leave them at the page they were on. */
    temppage = in_use->showstr_page;
    paginate_string((in_use->showstr_head = strdup(*in_use->showstr_vector)), in_use);
    in_use->showstr_page = temppage;
  }

  if (*buf)
    free(*buf);

  parse_at(temp);

  *buf = strdup(temp);
  return (0);
}

/* read contents of a text file, and place in buf */
static int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE + 3];
  int len;
  struct stat statbuf;

  *buf = '\0';

  if (!(fl = fopen(name, "r")))
  {
    log("SYSERR: reading %s: %s", name, strerror(errno));
    return (-1);
  }

  /* Grab the date/time the file was last edited */
  if (!strcmp(name, NEWS_FILE))
  {
    fstat(fileno(fl), &statbuf);
    newsmod = statbuf.st_mtime;
  }
  if (!strcmp(name, MOTD_FILE))
  {
    fstat(fileno(fl), &statbuf);
    motdmod = statbuf.st_mtime;
  }

  for (;;)
  {
    if (!fgets(tmp, READ_SIZE, fl)) /* EOF check */
      break;
    if ((len = strlen(tmp)) > 0)
      tmp[len - 1] = '\0';             /* take off the trailing \n */
    strlcat(tmp, "\r\n", sizeof(tmp)); /* strcat: OK (tmp:READ_SIZE+3) */

    if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH)
    {
      log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
      *buf = '\0';
      fclose(fl);
      return (-1);
    }
    strcat(buf, tmp); /* strcat: OK (size checked above) */
  }

  fclose(fl);

  return (0);
}

/* clear some of the the working variables of a char */
void reset_char(struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    GET_EQ(ch, i) = NULL;

  ch->followers = NULL;
  ch->master = NULL;
  IN_ROOM(ch) = NOWHERE;
  ch->carrying = NULL;
  // ch->bags->bag1 = NULL;
  // ch->bags->bag2 = NULL;
  // ch->bags->bag3 = NULL;
  // ch->bags->bag4 = NULL;
  // ch->bags->bag5 = NULL;
  // ch->bags->bag6 = NULL;
  // ch->bags->bag7 = NULL;
  // ch->bags->bag8 = NULL;
  // ch->bags->bag9 = NULL;
  // ch->bags->bag10 = NULL;
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  FIGHTING(ch) = NULL;
  GRAPPLE_TARGET(ch) = NULL;
  GRAPPLE_ATTACKER(ch) = NULL;
  HUNTING(ch) = NULL;
  char_from_furniture(ch);
  resetCastingData(ch);
  ch->char_specials.position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;
  ch->char_specials.totalDefense = 0;
  ch->char_specials.mounted_blocks_left = 0;
  ch->char_specials.deflect_arrows_left = 0;
  ch->char_specials.riding = NULL;
  ch->char_specials.ridden_by = NULL;
  ch->char_specials.blasting = 0;
  for (i = 0; i < NUM_CASTERS; i++)
    ch->char_specials.is_preparing[i] = 0;
  ch->char_specials.crafting_type = 0;
  ch->char_specials.crafting_ticks = 0;
  ch->char_specials.crafting_object = NULL;
  ch->char_specials.crafting_repeat = 0;
  ch->char_specials.crafting_bonus = 0;

  CLOUDKILL(ch) = 0;
  DOOM(ch) = 0;
  INCENDIARY(ch) = 0;
  TENACIOUS_PLAGUE(ch) = 0;

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_PSP(ch) <= 0)
    GET_PSP(ch) = 1;

  GET_LAST_TELL(ch) = NOBODY;
}

/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data *ch)
{
  int i = 0;
  memset((char *)ch, 0, sizeof(struct char_data));

  IN_ROOM(ch) = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_MOB_RNUM(ch) = NOBODY;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->events = NULL;

  /* worried about mobiles having junk-data for wards */
  for (i = 0; i < MAX_WARDING; i++)
    GET_WARDING(ch, i) = 0;

  if (IS_NPC(ch))
    PROC_FIRED(ch) = 0;

  GET_REAL_AC(ch) = 100; /* Basic Armor of 10 */
  if (GET_REAL_MAX_PSP(ch) < 100)
    GET_REAL_MAX_PSP(ch) = 100;
  //  ch->points.damage_reduction = NULL;
}

void clear_object(struct obj_data *obj)
{
  memset((char *)obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  IN_ROOM(obj) = NOWHERE;
  obj->worn_on = -1;
  GET_OBJ_SIZE(obj) = SIZE_MEDIUM;
  MISSILE_ID(obj) = 0;
  obj->special_abilities = NULL;
}

/* Called during character creation after picking character class (and then
 * never again for that character). */
void init_char(struct char_data *ch)
{
  int i, index = 0;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  if (ch->bags == NULL)
    CREATE(ch->bags, struct bag_data, 1);
  
  /* Initialize score section order to default */
  for (i = 0; i < 8; i++) {
    ch->player_specials->saved.score_section_order[i] = i;
  }

  /* If this is our first player make him IMPL. */
  if (top_of_p_table == 0)
  {
    GET_LEVEL(ch) = LVL_IMPL;
    GET_EXP(ch) = 7000000;

    /* The implementor never goes through do_start(). */
    GET_REAL_MAX_HIT(ch) = 500;
    GET_REAL_MAX_PSP(ch) = 100;
    GET_REAL_MAX_MOVE(ch) = 82;
    GET_HIT(ch) = GET_REAL_MAX_HIT(ch);
    GET_PSP(ch) = GET_REAL_MAX_PSP(ch);
    GET_MOVE(ch) = GET_REAL_MAX_MOVE(ch);
    newbieEquipment(ch);
  }
#if !defined(CAMPAIGN_DL) && !defined(CAMPAIGN_FR)
  set_title(ch, NULL);
#endif
  ch->player.short_descr = NULL;
  ch->player.long_descr = NULL;
  ch->player.description = NULL;
  ch->player.walkin = NULL;
  ch->player.walkout = NULL;

  GET_NUM_QUESTS(ch) = 0;

  ch->player_specials->saved.completed_quests = NULL;

  for (index = 0; index < MAX_CURRENT_QUESTS; index++)
  { /* loop through all the character's quest slots */
    GET_QUEST(ch, index) = NOTHING;
  }

  /* Create the action queues */
  GET_QUEUE(ch) = create_action_queue();
  GET_ATTACK_QUEUE(ch) = create_attack_queue();

  /* create the preparation / collection lists */
  /*
  for (i = 0; i < NUM_CASTERS; i++) {
    SPELL_PREP_QUEUE(ch, i) = create_prep_collection_list(i);
    SPELL_COLLECTION(ch, i) = create_prep_collection_list(i);
  }
   */

  ch->player.time.birth = time(0);
  ch->player.time.logon = time(0);
  ch->player.time.played = 0;

  GET_REAL_AC(ch) = 100; /* basic armor of 10 */
  GET_REAL_SPELL_RES(ch) = 0;

  /* Bias the height and weight of the character depending on what gender
   * they have chosen. While it is possible to have a tall, heavy female it's
   * not as likely as a male. Height is in centimeters. Weight is in pounds.
   * The only place they're ever printed (in stock code) is SPELL_IDENTIFY. */
  if (GET_SEX(ch) == SEX_MALE)
  {
    GET_WEIGHT(ch) = rand_number(120, 180);
    GET_HEIGHT(ch) = rand_number(160, 200); /* 5'4" - 6'8" */
  }
  else
  {
    GET_WEIGHT(ch) = rand_number(100, 160);
    GET_HEIGHT(ch) = rand_number(150, 180); /* 5'0" - 6'0" */
  }

  GET_REAL_SIZE(ch) = SIZE_MEDIUM;

#ifdef CAMPAIGN_FR
    if (GET_RACE(ch) < -1 || GET_RACE(ch) >= NUM_EXTENDED_PC_RACES)
#elif defined(CAMPAIGN_DL)
  if (GET_RACE(ch) < DL_RACE_START || GET_RACE(ch) >= DL_RACE_END)
#else
  if (GET_RACE(ch) < -1 || GET_RACE(ch) >= NUM_RACES)
#endif
    GET_REAL_RACE(ch) = RACE_UNDEFINED;

  if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
    player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
  else
    log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));

  for (i = 1; i <= MAX_SKILLS; i++)
  {
    if (GET_LEVEL(ch) < LVL_IMPL)
      SET_SKILL(ch, i, 0);
    else
      SET_SKILL(ch, i, 100);
  }

  for (i = 1; i <= MAX_ABILITIES; i++)
  {
    if (GET_LEVEL(ch) < LVL_IMPL)
      SET_ABILITY(ch, i, 0);
    else
      SET_ABILITY(ch, i, 40);
  }

  for (i = 0; i < AF_ARRAY_MAX; i++)
    AFF_FLAGS(ch)
  [i] = 0;

  for (i = 0; i < NUM_OF_SAVING_THROWS; i++)
    GET_REAL_SAVE(ch, i) = 0;

  for (i = 0; i < NUM_DAM_TYPES; i++)
    GET_REAL_RESISTANCES(ch, i) = 0;

  ch->real_abils.str_add = 0;
  GET_REAL_STR(ch) = 3;
  GET_REAL_CON(ch) = 3;
  GET_REAL_DEX(ch) = 3;
  GET_REAL_INT(ch) = 3;
  GET_REAL_CHA(ch) = 3;
  GET_REAL_WIS(ch) = 3;

  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);

  GET_LOADROOM(ch) = NOWHERE;
  GET_SCREEN_WIDTH(ch) = PAGE_WIDTH;

  /* Set Beginning Toggles Here */
  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
  if (ch->desc)
    if (ch->desc->pProtocol->pVariables[eMSDP_ANSI_COLORS] ||
        ch->desc->pProtocol->pVariables[eMSDP_256_COLORS])
    {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);
    }
  SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPACTIONS);

  // automap toggled on -zusuk
  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOMAP);
#if defined(CAMPAIGN_FR)   || defined(CAMPAIGN_DL)
  // autoprep toggled on -gicker
  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTO_PREP);
  // autoconsider on
  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOCON);
#endif

  // fresh start on casting data
  resetCastingData(ch);

  // fresh start on auto-crafting data
  reset_acraft(ch);

  // fresh start on mem data
  destroy_spell_prep_queue(ch);
  destroy_innate_magic_queue(ch);
  destroy_spell_collection(ch);
  destroy_known_spells(ch);

  // make sure no cloudkills, incendiary
  CLOUDKILL(ch) = 0;
  DOOM(ch) = 0;
  INCENDIARY(ch) = 0;
  TENACIOUS_PLAGUE(ch) = 0;

  /* more inits */
  FIGHTING(ch) = NULL;
  GRAPPLE_TARGET(ch) = NULL;
  GRAPPLE_ATTACKER(ch) = NULL;
  SITTING(ch) = NULL;
  NEXT_SITTING(ch) = NULL;
  RIDING(ch) = NULL;
  RIDDEN_BY(ch) = NULL;
  POOFIN(ch) = NULL;
  POOFOUT(ch) = NULL;
  IS_CARRYING_W(ch) = 0;
  IS_CARRYING_N(ch) = 0;
  TIMER(ch) = 0;
  TOTAL_DEFENSE(ch) = 0;
  MOUNTED_BLOCKS_LEFT(ch) = 0;
  DEFLECT_ARROWS_LEFT(ch) = 0;
  GET_WIMP_LEV(ch) = 0;
  GET_FREEZE_LEV(ch) = 0;
  GET_INVIS_LEV(ch) = 0;
  GET_BAD_PWS(ch) = 0;
  for (i = 0; i < MAX_CLASSES; i++)
    GET_SPEC_ABIL(ch, i) = 0;
  for (i = 0; i < MAX_ENEMIES; i++)
    GET_FAVORED_ENEMY(ch, i) = 0;
  GUARDING(ch) = NULL;
  GET_TOTAL_AOO(ch) = 0;

  /*
  #define SKILL_MINING                    471  //implemented
  #define SKILL_HUNTING                   472  //implemented
  #define SKILL_FORESTING                 473  //implemented
  #define SKILL_KNITTING                  474  //implemented
  #define SKILL_CHEMISTRY                 475  //implemented
  #define SKILL_ARMOR_SMITHING            476  //implemented
  #define SKILL_WEAPON_SMITHING           477  //implemented
  #define SKILL_JEWELRY_MAKING            478  //implemented
  #define SKILL_LEATHER_WORKING           479  //implemented
  #define SKILL_FAST_CRAFTER              480  //implemented
  #define SKILL_BONE_ARMOR                481
  #define SKILL_ELVEN_CRAFTING            482
  #define SKILL_MASTERWORK_CRAFTING       483
  #define SKILL_DRACONIC_CRAFTING         484
  #define SKILL_DWARVEN_CRAFTING          485
   */
  /* start crafting skills at 4 */
  if (GET_LEVEL(ch) < LVL_STAFF)
  {
    for (i = TOP_CRAFT_SKILL; i < BOTTOM_CRAFT_SKILL; i++)
    {
      SET_SKILL(ch, i, 4);
    }
  }
}

/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum)
{
  room_rnum bot, top, mid;

  bot = 0;
  top = top_of_world;

  if (world[bot].number > vnum || world[top].number < vnum)
    return (NOWHERE);

  /* perform binary search on world-table */
  while (bot <= top)
  {
    mid = (bot + top) / 2;

    // if ((world + mid)->number == vnum)
    if ((world[mid]).number == vnum)
      return (mid);
    // if ((world + mid)->number > vnum)
    if ((world[mid]).number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
  return (NOWHERE);
}

/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
  mob_rnum bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* quickly reject out-of-range vnums */
  if (mob_index[bot].vnum > vnum || mob_index[top].vnum < vnum)
    return (NOBODY);

  /* perform binary search on mob-table */
  while (bot <= top)
  {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->vnum == vnum)
      return (mid);
    if ((mob_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
  return (NOBODY);
}

/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
  obj_rnum bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* quickly reject out-of-range vnums */
  if (obj_index[bot].vnum > vnum || obj_index[top].vnum < vnum)
    return (NOTHING);

  /* perform binary search on obj-table */
  while (bot <= top)
  {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->vnum == vnum)
      return (mid);
    if ((obj_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
  return (NOTHING);
}

/* returns the real number of the zone with given virtual number */
zone_rnum real_zone(zone_vnum vnum)
{
  zone_rnum bot, top, mid;

  bot = 0;
  top = top_of_zone_table;

  if (zone_table[bot].number > vnum || zone_table[top].number < vnum)
    return (NOWHERE);

  /* perform binary search on zone-table */
  while (bot <= top)
  {
    mid = (bot + top) / 2;

    if ((zone_table + mid)->number == vnum)
      return (mid);
    if ((zone_table + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
  return (NOWHERE);
}

/* returns the real number of the region with given virtual number */
region_rnum real_region(region_vnum vnum)
{
  region_rnum bot, top, mid;

  /* Check if region_table is NULL or empty */
  if (!region_table || top_of_region_table < 0)
    return (NOWHERE);

  bot = 0;
  top = top_of_region_table;

  if (region_table[bot].vnum > vnum || region_table[top].vnum < vnum)
    return (NOWHERE);

  /* perform binary search on zone-table */
  while (bot <= top)
  {
    mid = (bot + top) / 2;
    if ((region_table + mid)->vnum == vnum)
      return (mid);
    if ((region_table + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
  return (NOWHERE);
}

path_rnum real_path(path_vnum vnum)
{
  region_rnum bot, top, mid;

  bot = 0;
  top = top_of_path_table;

  if (path_table[bot].vnum > vnum || path_table[top].vnum < vnum)
    return (NOWHERE);

  /* perform binary search on zone-table */
  while (bot <= top)
  {
    mid = (bot + top) / 2;
    if ((path_table + mid)->vnum == vnum)
      return (mid);
    if ((path_table + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
  return (NOWHERE);
}

/* Extend later to include more checks and add checks for unknown bitvectors. */
static int check_object(struct obj_data *obj)
{
  char objname[MAX_INPUT_LENGTH + 32];
  int error = FALSE, y;
  char buf1[MAX_INPUT_LENGTH] = {'\0'};

  /* stripping colors for SYSLOG -zusuk */
  strncpy(buf1, obj->short_description, sizeof(buf1) - 1);
  buf1[sizeof(buf1) - 1] = '\0';
  strip_colors(buf1);

  if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
    log("SYSERR: Object #%d (%s) has negative weight (%d).",
        GET_OBJ_VNUM(obj), buf1, GET_OBJ_WEIGHT(obj));

  if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
    log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
        GET_OBJ_VNUM(obj), buf1, GET_OBJ_RENT(obj));

  snprintf(objname, sizeof(objname), "Object #%d (%s)", GET_OBJ_VNUM(obj), buf1);
  for (y = 0; y < TW_ARRAY_MAX; y++)
  {
    error |= check_bitvector_names(GET_OBJ_WEAR(obj)[y], wear_bits_count, objname, "object wear");
    error |= check_bitvector_names(GET_OBJ_EXTRA(obj)[y], extra_bits_count, objname, "object extra");
    error |= check_bitvector_names(GET_OBJ_AFFECT(obj)[y], affected_bits_count, objname, "object affect");
  }

  switch (GET_OBJ_TYPE(obj))
  {
  case ITEM_DRINKCON:
  {
    char onealias[MAX_INPUT_LENGTH] = {'\0'}, *space = strrchr(obj->name, ' ');

    strlcpy(onealias, space ? space + 1 : obj->name, sizeof(onealias));

    /* i don't see why this is an issue, I turned off the reporting since
       it fills our logs with errors -zusuk */
    if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE))
    {
      /*
        log("SYSERR: Object #%d (%s) doesn't have drink type as last keyword. (%s)",
              GET_OBJ_VNUM(obj), buf1, obj->name);
         */
    }
  }
    /* Fall through. */
  case ITEM_FOUNTAIN:
    if ((GET_OBJ_VAL(obj, 0) > 0) && (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE)))
    {
      /* commented out to reduce spam -zusuk */
      /*
        log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
              GET_OBJ_VNUM(obj), buf1,
              GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
         */
    }
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 1);
    error |= check_object_spell_number(obj, 2);
    error |= check_object_spell_number(obj, 3);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 3);
    if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
      log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
          GET_OBJ_VNUM(obj), buf1,
          GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
    break;
  case ITEM_NOTE:
    if (obj->ex_description)
    {
      char onealias[MAX_INPUT_LENGTH] = {'\0'}, *next_name;
      next_name = any_one_arg(obj->name, onealias);
      do
      {
        if (find_exdesc(onealias, obj->ex_description) && (error = TRUE))
        {

          /* I am not sure why this is a problem, removed to reduce log spam -Zusuk */
          /*
            log("SYSERR: Object #%d (%s) is type NOTE and has extra description with same name. (%s)",
                    GET_OBJ_VNUM(obj), buf1, obj->name);
             */
        }
        next_name = any_one_arg(next_name, onealias);
      } while (*onealias);
    }
    break;
  case ITEM_FURNITURE:
    if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE))
      log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
          GET_OBJ_VNUM(obj), buf1, GET_OBJ_VAL(obj, 1),
          GET_OBJ_VAL(obj, 0));
    break;
  }

  return (error);
}

static int check_object_spell_number(struct obj_data *obj, int val)
{
  int error = FALSE;
  const char *spellname;

  if (GET_OBJ_VAL(obj, val) == -1 || GET_OBJ_VAL(obj, val) == 0) /* no spell */
    return (error);

  /* Check for negative spells, spells beyond the top define, and any spell
   * which is actually a skill. */
  if (GET_OBJ_VAL(obj, val) < 0)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > TOP_SKILL_DEFINE)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > MAX_SPELLS && GET_OBJ_VAL(obj, val) < TOP_SKILL_DEFINE)
    error = TRUE;
  if (error)
    log("SYSERR: Object #%d (%s) has out of range spell #%d.",
        GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  if (scheck) /* Spell names don't exist in syntax check mode. */
    return (error);

  /* Now check for unnamed spells. */
  spellname = spell_name(GET_OBJ_VAL(obj, val));

  if ((spellname == unused_spellname || !str_cmp("UNDEFINED", spellname)) && (error = TRUE))
    log("SYSERR: Object #%d (%s) uses '%s' spell #%d.",
        GET_OBJ_VNUM(obj), obj->short_description, spellname,
        GET_OBJ_VAL(obj, val));

  return (error);
}

static int check_object_level(struct obj_data *obj, int val)
{
  int error = FALSE;

  if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL) && (error = TRUE))
    log("SYSERR: Object #%d (%s) has out of range level #%d.",
        GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  return (error);
}

static int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits)
{
  unsigned int flagnum;
  bool error = FALSE;

  /* See if any bits are set above the ones we know about. */
  if (bits <= (~(bitvector_t)0 >> (sizeof(bitvector_t) * 8 - namecount)))
    return (FALSE);

  for (flagnum = namecount; flagnum < sizeof(bitvector_t) * 8; flagnum++)
    if ((1 << flagnum) & bits)
    {
      log("SYSERR: %s has unknown %s flag, bit %d (0 through %d known).", whatami, whatbits, flagnum, (int)namecount - 1);
      error = TRUE;
    }

  return (error);
}

static void load_default_config(void)
{
  /* This function is called only once, at boot-time. We assume config_info is
   * empty. -Welcor */
  /* Game play options. */
  CONFIG_PK_ALLOWED = pk_allowed;
  CONFIG_PT_ALLOWED = pt_allowed;
  CONFIG_LEVEL_CAN_SHOUT = level_can_shout;
  CONFIG_HOLLER_MOVE_COST = holler_move_cost;
  CONFIG_TUNNEL_SIZE = tunnel_size;
  CONFIG_MAX_EXP_GAIN = max_exp_gain;
  CONFIG_MAX_EXP_LOSS = max_exp_loss;
  CONFIG_MAX_NPC_CORPSE_TIME = max_npc_corpse_time;
  CONFIG_MAX_PC_CORPSE_TIME = max_pc_corpse_time;
  CONFIG_IDLE_VOID = idle_void;
  CONFIG_IDLE_RENT_TIME = idle_rent_time;
  CONFIG_IDLE_MAX_LEVEL = idle_max_level;
  CONFIG_DTS_ARE_DUMPS = dts_are_dumps;
  CONFIG_LOAD_INVENTORY = load_into_inventory;
  CONFIG_OK = strdup(OK);
  CONFIG_NOPERSON = strdup(NOPERSON);
  CONFIG_NOEFFECT = strdup(NOEFFECT);
  CONFIG_TRACK_T_DOORS = track_through_doors;
  CONFIG_NO_MORT_TO_IMMORT = no_mort_to_immort;
  CONFIG_DISP_CLOSED_DOORS = display_closed_doors;
  CONFIG_PROTOCOL_NEGOTIATION = protocol_negotiation;
  CONFIG_SPECIAL_IN_COMM = special_in_comm;
  CONFIG_DIAGONAL_DIRS = diagonal_dirs;
  CONFIG_MAP = map_option;
  CONFIG_MAP_SIZE = default_map_size;
  CONFIG_MINIMAP_SIZE = default_minimap_size;
  CONFIG_SCRIPT_PLAYERS = script_players;
  CONFIG_MIN_POP_TO_CLAIM = min_pop_to_claim;
  CONFIG_DEBUG_MODE = debug_mode;

  /* Rent / crashsave options. */
  CONFIG_FREE_RENT = free_rent;
  CONFIG_MAX_OBJ_SAVE = max_obj_save;
  CONFIG_MIN_RENT_COST = min_rent_cost;
  CONFIG_AUTO_SAVE = auto_save;
  CONFIG_AUTOSAVE_TIME = autosave_time;
  CONFIG_CRASH_TIMEOUT = crash_file_timeout;
  CONFIG_RENT_TIMEOUT = rent_file_timeout;

  /* Room numbers. */
  CONFIG_MORTAL_START = mortal_start_room;
  CONFIG_IMMORTAL_START = immort_start_room;
  CONFIG_FROZEN_START = frozen_start_room;
  CONFIG_DON_ROOM_1 = donation_room_1;
  CONFIG_DON_ROOM_2 = donation_room_2;
  CONFIG_DON_ROOM_3 = donation_room_3;

  /* Game operation options. */
  CONFIG_DFLT_PORT = DFLT_PORT;

  if (DFLT_IP)
    CONFIG_DFLT_IP = strdup(DFLT_IP);
  else
    CONFIG_DFLT_IP = NULL;

  CONFIG_DFLT_DIR = strdup(DFLT_DIR);

  if (LOGNAME)
    CONFIG_LOGNAME = strdup(LOGNAME);
  else
    CONFIG_LOGNAME = NULL;

  CONFIG_MAX_PLAYING = max_playing;
  CONFIG_MAX_FILESIZE = max_filesize;
  CONFIG_MAX_BAD_PWS = max_bad_pws;
  CONFIG_SITEOK_ALL = siteok_everyone;
  CONFIG_NS_IS_SLOW = nameserver_is_slow;
  CONFIG_NEW_SOCIALS = use_new_socials;
  CONFIG_OLC_SAVE = auto_save_olc;
  CONFIG_MENU = strdup(MENU);
  CONFIG_WELC_MESSG = strdup(WELC_MESSG);
  CONFIG_START_MESSG = strdup(START_MESSG);
  CONFIG_MEDIT_ADVANCED = medit_advanced_stats;
  CONFIG_IBT_AUTOSAVE = ibt_autosave;
  /* Autowiz options. */
  CONFIG_USE_AUTOWIZ = use_autowiz;
  CONFIG_MIN_WIZLIST_LEV = min_wizlist_lev;

  /* Happy hour stuff */
  CONFIG_HAPPY_HOUR_CHANCE = happy_hour_chance;
  CONFIG_HAPPY_HOUR_EXP = happy_hour_exp_bonus;
  CONFIG_HAPPY_HOUR_QP = happy_hour_qp_bonus;
  CONFIG_HAPPY_HOUR_GOLD = happy_hour_gold_bonus;
  CONFIG_HAPPY_HOUR_TREASURE = happy_hour_treasure_bonus;
}

void load_config(void)
{
  FILE *fl;
  char line[MAX_STRING_LENGTH] = {'\0'};
  char tag[MAX_INPUT_LENGTH] = {'\0'};
  int num = 0;
  float fl_num = 0.0;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  load_default_config();

  snprintf(buf, sizeof(buf), "%s/%s", CONFIG_DFLT_DIR, CONFIG_CONFFILE);
  if (!(fl = fopen(CONFIG_CONFFILE, "r")) && !(fl = fopen(buf, "r")))
  {
    snprintf(buf, sizeof(buf), "No %s file, using defaults", CONFIG_CONFFILE);
    perror(buf);
    return;
  }

  /* Load the game configuration file. */
  while (get_line(fl, line))
  {
    split_argument(line, tag);
    num = atoi(line);
    sscanf(line, "%f", &fl_num); /*grab a float number */

    switch (LOWER(*tag))
    {
    case 'a':
      if (!str_cmp(tag, "auto_save"))
        CONFIG_AUTO_SAVE = num;
      else if (!str_cmp(tag, "autosave_time"))
        CONFIG_AUTOSAVE_TIME = num;
      else if (!str_cmp(tag, "auto_save_olc"))
        CONFIG_OLC_SAVE = num;
      else if (!str_cmp(tag, "arcane_spell_damage"))
        CONFIG_ARCANE_DAMAGE = num;
      else if (!str_cmp(tag, "ac_cap"))
        CONFIG_PLAYER_AC_CAP = num;
      else if (!str_cmp(tag, "arcane_mem_times"))
        CONFIG_ARCANE_PREP_TIME = num;
      else if (!str_cmp(tag, "alchemy_mem_times"))
        CONFIG_ALCHEMY_PREP_TIME = num;
      break;

    case 'c':
      if (!str_cmp(tag, "crash_file_timeout"))
        CONFIG_CRASH_TIMEOUT = num;
      break;

    case 'd':
      if (!str_cmp(tag, "debug_mode"))
        CONFIG_DEBUG_MODE = num;
      else if (!str_cmp(tag, "display_closed_doors"))
        CONFIG_DISP_CLOSED_DOORS = num;
      else if (!str_cmp(tag, "diagonal_dirs"))
        CONFIG_DIAGONAL_DIRS = num;
      else if (!str_cmp(tag, "dts_are_dumps"))
        CONFIG_DTS_ARE_DUMPS = num;
      else if (!str_cmp(tag, "divine_mem_times"))
        CONFIG_DIVINE_PREP_TIME = num;
      else if (!str_cmp(tag, "death_exp_loss_penalty"))
        CONFIG_DEATH_EXP_LOSS = num;
      else if (!str_cmp(tag, "donation_room_1"))
        if (num == -1)
          CONFIG_DON_ROOM_1 = NOWHERE;
        else
          CONFIG_DON_ROOM_1 = num;
      else if (!str_cmp(tag, "donation_room_2"))
        if (num == -1)
          CONFIG_DON_ROOM_2 = NOWHERE;
        else
          CONFIG_DON_ROOM_2 = num;
      else if (!str_cmp(tag, "donation_room_3"))
        if (num == -1)
          CONFIG_DON_ROOM_3 = NOWHERE;
        else
          CONFIG_DON_ROOM_3 = num;
      else if (!str_cmp(tag, "dflt_dir"))
      {
        if (CONFIG_DFLT_DIR)
          free(CONFIG_DFLT_DIR);
        if (*line)
          CONFIG_DFLT_DIR = strdup(line);
        else
          CONFIG_DFLT_DIR = strdup(DFLT_DIR);
      }
      else if (!str_cmp(tag, "dflt_ip"))
      {
        if (CONFIG_DFLT_IP)
          free(CONFIG_DFLT_IP);
        if (*line)
          CONFIG_DFLT_IP = strdup(line);
        else
          CONFIG_DFLT_IP = NULL;
      }
      else if (!str_cmp(tag, "dflt_port"))
        CONFIG_DFLT_PORT = num;
      else if (!str_cmp(tag, "default_map_size"))
        CONFIG_MAP_SIZE = num;
      else if (!str_cmp(tag, "default_minimap_size"))
        CONFIG_MINIMAP_SIZE = num;
      else if (!str_cmp(tag, "divine_spell_damage"))
        CONFIG_DIVINE_DAMAGE = num;
      break;

    case 'e':
      if (!str_cmp(tag, "extra_level_hp"))
        CONFIG_EXTRA_PLAYER_HP_PER_LEVEL = num;
      else if (!str_cmp(tag, "extra_level_mv"))
        CONFIG_EXTRA_PLAYER_MV_PER_LEVEL = num;
      else if (!str_cmp(tag, "exp_level_difference"))
        CONFIG_EXP_LEVEL_DIFFERENCE = num;
      break;

    case 'f':
      if (!str_cmp(tag, "free_rent"))
        CONFIG_FREE_RENT = num;
      else if (!str_cmp(tag, "frozen_start_room"))
        CONFIG_FROZEN_START = num;
      break;

    case 'h':
      if (!str_cmp(tag, "holler_move_cost"))
        CONFIG_HOLLER_MOVE_COST = num;
      else if (!str_cmp(tag, "happy_hour_chance"))
        CONFIG_HAPPY_HOUR_CHANCE = num;
      else if (!str_cmp(tag, "happy_hour_exp_bonus"))
        CONFIG_HAPPY_HOUR_EXP = num;
      else if (!str_cmp(tag, "happy_hour_qp_bonus"))
        CONFIG_HAPPY_HOUR_QP = num;
      else if (!str_cmp(tag, "happy_hour_gold_bonus"))
        CONFIG_HAPPY_HOUR_GOLD = num;
      else if (!str_cmp(tag, "happy_hour_treasure_bonus"))
        CONFIG_HAPPY_HOUR_TREASURE = num;
      break;

    case 'i':
      if (!str_cmp(tag, "idle_void"))
        CONFIG_IDLE_VOID = num;
      else if (!str_cmp(tag, "idle_rent_time"))
        CONFIG_IDLE_RENT_TIME = num;
      else if (!str_cmp(tag, "idle_max_level"))
        CONFIG_IDLE_MAX_LEVEL = num;
      else if (!str_cmp(tag, "immort_start_room"))
        CONFIG_IMMORTAL_START = num;
      else if (!str_cmp(tag, "ibt_autosave"))
        CONFIG_IBT_AUTOSAVE = num;
      break;

    case 'l':
      if (!str_cmp(tag, "level_can_shout"))
        CONFIG_LEVEL_CAN_SHOUT = num;
      else if (!str_cmp(tag, "load_into_inventory"))
        CONFIG_LOAD_INVENTORY = num;
      else if (!str_cmp(tag, "logname"))
      {
        if (CONFIG_LOGNAME)
          free(CONFIG_LOGNAME);
        if (*line)
          CONFIG_LOGNAME = strdup(line);
        else
          CONFIG_LOGNAME = NULL;
      }
      break;

    case 'm':
      if (!str_cmp(tag, "max_bad_pws"))
        CONFIG_MAX_BAD_PWS = num;
      else if (!str_cmp(tag, "max_exp_gain"))
        CONFIG_MAX_EXP_GAIN = num;
      else if (!str_cmp(tag, "max_exp_loss"))
        CONFIG_MAX_EXP_LOSS = num;
      else if (!str_cmp(tag, "max_filesize"))
        CONFIG_MAX_FILESIZE = num;
      else if (!str_cmp(tag, "max_npc_corpse_time"))
        CONFIG_MAX_NPC_CORPSE_TIME = num;
      else if (!str_cmp(tag, "max_obj_save"))
        CONFIG_MAX_OBJ_SAVE = num;
      else if (!str_cmp(tag, "max_pc_corpse_time"))
        CONFIG_MAX_PC_CORPSE_TIME = num;
      else if (!str_cmp(tag, "max_playing"))
        CONFIG_MAX_PLAYING = num;
      else if (!str_cmp(tag, "menu"))
      {
        if (CONFIG_MENU)
          free(CONFIG_MENU);
        strncpy(buf, "Reading menu in load_config()", sizeof(buf));
        CONFIG_MENU = fread_string(fl, buf);
        parse_at(CONFIG_MENU);
      }
      else if (!str_cmp(tag, "min_rent_cost"))
        CONFIG_MIN_RENT_COST = num;
      else if (!str_cmp(tag, "min_wizlist_lev"))
        CONFIG_MIN_WIZLIST_LEV = num;
      else if (!str_cmp(tag, "mortal_start_room"))
        CONFIG_MORTAL_START = num;
      else if (!str_cmp(tag, "map_option"))
        CONFIG_MAP = num;
      else if (!str_cmp(tag, "medit_advanced_stats"))
        CONFIG_MEDIT_ADVANCED = num;
      else if (!str_cmp(tag, "min_pop_to_claim"))
        CONFIG_MIN_POP_TO_CLAIM = fl_num;
      break;

    case 'n':
      if (!str_cmp(tag, "nameserver_is_slow"))
        CONFIG_NS_IS_SLOW = num;
      else if (!str_cmp(tag, "no_mort_to_immort"))
        CONFIG_NO_MORT_TO_IMMORT = num;
      else if (!str_cmp(tag, "noperson"))
      {
        char tmp[READ_SIZE];
        if (CONFIG_NOPERSON)
          free(CONFIG_NOPERSON);
        snprintf(tmp, sizeof(tmp), "%s\r\n", line);
        CONFIG_NOPERSON = strdup(tmp);
      }
      else if (!str_cmp(tag, "noeffect"))
      {
        char tmp[READ_SIZE];
        if (CONFIG_NOEFFECT)
          free(CONFIG_NOEFFECT);
        snprintf(tmp, sizeof(tmp), "%s\r\n", line);
        CONFIG_NOEFFECT = strdup(tmp);
      }
      break;

    case 'o':
      if (!str_cmp(tag, "ok"))
      {
        char tmp[READ_SIZE];
        if (CONFIG_OK)
          free(CONFIG_OK);
        snprintf(tmp, sizeof(tmp), "%s\r\n", line);
        CONFIG_OK = strdup(tmp);
      }
      break;

    case 'p':
      if (!str_cmp(tag, "pk_allowed"))
        CONFIG_PK_ALLOWED = num;
      else if (!str_cmp(tag, "protocol_negotiation"))
        CONFIG_PROTOCOL_NEGOTIATION = num;
      else if (!str_cmp(tag, "pt_allowed"))
        CONFIG_PT_ALLOWED = num;
      else if (!str_cmp(tag, "psionic_power_damage"))
        CONFIG_PSIONIC_DAMAGE = num;
      else if (!str_cmp(tag, "psionic_mem_times"))
        CONFIG_PSIONIC_PREP_TIME = num;
      break;

    case 'r':
      if (!str_cmp(tag, "rent_file_timeout"))
        CONFIG_RENT_TIMEOUT = num;
      break;

    case 's':
      if (!str_cmp(tag, "siteok_everyone"))
        CONFIG_SITEOK_ALL = num;
      else if (!str_cmp(tag, "script_players"))
        CONFIG_SCRIPT_PLAYERS = num;
      else if (!str_cmp(tag, "special_in_comm"))
        CONFIG_SPECIAL_IN_COMM = num;
      else if (!str_cmp(tag, "start_messg"))
      {
        strncpy(buf, "Reading start message in load_config()", sizeof(buf));
        if (CONFIG_START_MESSG)
          free(CONFIG_START_MESSG);
        CONFIG_START_MESSG = fread_string(fl, buf);
        parse_at(CONFIG_START_MESSG);
      }
      else if (!str_cmp(tag, "summon_1_10_hp"))
        CONFIG_SUMMON_LEVEL_1_10_HP = num;
      else if (!str_cmp(tag, "summon_1_10_hit_dam"))
        CONFIG_SUMMON_LEVEL_1_10_HIT_DAM = num;
      else if (!str_cmp(tag, "summon_1_10_ac"))
        CONFIG_SUMMON_LEVEL_1_10_AC = num;
      else if (!str_cmp(tag, "summon_11_20_hp"))
        CONFIG_SUMMON_LEVEL_11_20_HP = num;
      else if (!str_cmp(tag, "summon_11_20_hit_dam"))
        CONFIG_SUMMON_LEVEL_11_20_HIT_DAM = num;
      else if (!str_cmp(tag, "summon_11_20_ac"))
        CONFIG_SUMMON_LEVEL_11_20_AC = num;
      else if (!str_cmp(tag, "summon_21_30_hp"))
        CONFIG_SUMMON_LEVEL_21_30_HP = num;
      else if (!str_cmp(tag, "summon_21_30_hit_dam"))
        CONFIG_SUMMON_LEVEL_21_30_HIT_DAM = num;
      else if (!str_cmp(tag, "summon_21_30_ac"))
        CONFIG_SUMMON_LEVEL_21_30_AC = num;
      break;

    case 't':
      if (!str_cmp(tag, "tunnel_size"))
        CONFIG_TUNNEL_SIZE = num;
      else if (!str_cmp(tag, "track_through_doors"))
        CONFIG_TRACK_T_DOORS = num;
      break;

    case 'u':
      if (!str_cmp(tag, "use_autowiz"))
        CONFIG_USE_AUTOWIZ = num;
      else if (!str_cmp(tag, "use_new_socials"))
        CONFIG_NEW_SOCIALS = num;
      break;

    case 'w':
      if (!str_cmp(tag, "welc_messg"))
      {
        strncpy(buf, "Reading welcome message in load_config()", sizeof(buf));
        if (CONFIG_WELC_MESSG)
          free(CONFIG_WELC_MESSG);
        CONFIG_WELC_MESSG = fread_string(fl, buf);
      }
      break;

    default:
      break;
    }
  }

  fclose(fl);
}

/* Centralized character creation - Jamdog - 31st December 2007 */

/* If you need a new blank char_data struct, call this          */
struct char_data *new_char()
{
  struct char_data *ch;
  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  CREATE(ch->player_specials, struct player_special_data, 1);
  CREATE(ch->bags, struct bag_data, 1);

  /* Set all required NULL pointers and variables */
  ch->player_specials->saved.completed_quests = NULL;
  ch->player_specials->saved.num_completed_quests = 0;
  
  /* Initialize score section order to default */
  int i;
  for (i = 0; i < 8; i++) {
    ch->player_specials->saved.score_section_order[i] = i;
  }

  return ch;
}

// status 0 is no happy hour
// status 1 is happy hour started
// status 2 is happy hour ended
void set_db_happy_hour(int status)
{

  if (CONFIG_DFLT_PORT != 4100)
    return;

  char query[LONG_STRING] = {'\0'};

  mysql_ping(conn);

  snprintf(query, sizeof(query), "DELETE FROM happy_hour_info");

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to DELETE from happy_hour_info: %s", mysql_error(conn));
  }

  snprintf(query, sizeof(query), "INSERT INTO happy_hour_info (happy_hour_status) VALUES('%s')", status == 0 ? "none" : (status == 1 ? "started" : "ended"));

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to INSERT INTO happy_hour_info: %s", mysql_error(conn));
  }
}

void save_objects_to_database(struct char_data *ch)
{
  int j, i;
  int zone_num = 0;
  int obj_idnum = 0;
  struct obj_data *obj = NULL;
  struct obj_special_ability *specab;

  char query[MAX_STRING_LENGTH] = {'\0'};
  char temp[255] = {'\0'};
  char bonus[255] = {'\0'};
  char object_name[255] = {'\0'};
  char specific_type[255] = {'\0'};
  char weapon_spell_1[255] = {'\0'};
  char weapon_spell_2[255] = {'\0'};
  char weapon_spell_3[255] = {'\0'};
  char weapon_special_ability[255] = {'\0'};
  char notes[LONG_STRING] = {'\0'};
  char zone_name[255] =  {'\0'};

  mysql_ping(conn);

  snprintf(query, sizeof(query), "TRUNCATE TABLE object_database_items");
  if (mysql_query(conn, query)) log("SYSERR: Unable to TRUNCATE TABLE object_database_items: %s", mysql_error(conn));
  snprintf(query, sizeof(query), "TRUNCATE TABLE object_database_bonuses");
  if (mysql_query(conn, query)) log("SYSERR: Unable to TRUNCATE TABLE object_database_bonuses: %s", mysql_error(conn));
  snprintf(query, sizeof(query), "TRUNCATE TABLE object_database_wear_slots");
  if (mysql_query(conn, query)) log("SYSERR: Unable to TRUNCATE TABLE object_database_wear_slots: %s", mysql_error(conn));
  snprintf(query, sizeof(query), "TRUNCATE TABLE object_database_obj_flags");
  if (mysql_query(conn, query)) log("SYSERR: Unable to TRUNCATE TABLE object_database_obj_flags: %s", mysql_error(conn));
  snprintf(query, sizeof(query), "TRUNCATE TABLE object_database_perm_affects");
  if (mysql_query(conn, query)) log("SYSERR: Unable to TRUNCATE TABLE object_database_perm_affects: %s", mysql_error(conn));

  // We have to do this loop in multiple stages to prevent the MUD from crashing

  for (j = 0; j < top_of_objt; j++)
  {
    // we have issues with obj vnums above this number.
    // all zones that are made should be below these vnums anyway.
    if (obj_index[j].vnum >= 60000) continue;

    obj = read_object(obj_index[j].vnum, VIRTUAL);

    if (!obj) continue;

    specific_type[0] = '\0';
    switch (GET_OBJ_TYPE(obj))
    {
      case ITEM_WEAPON: snprintf(specific_type, sizeof(specific_type), "%s", weapon_list[GET_OBJ_VAL(obj, 0)].name); break;
      case ITEM_ARMOR: snprintf(specific_type, sizeof(specific_type), "%s", armor_list[GET_OBJ_VAL(obj, 1)].name); break;
    }

    weapon_spell_1[0] = '\0';
    if (GET_WEAPON_SPELL(obj, 0))
    {
      snprintf(weapon_spell_1, sizeof(weapon_spell_1), "Spell: %s, Level: %d, Percent: %d, Procs in combat?: %s\r\n",
                       spell_info[GET_WEAPON_SPELL(obj, 0)].name, GET_WEAPON_SPELL_LVL(obj, 0),
                       GET_WEAPON_SPELL_PCT(obj, 0), GET_WEAPON_SPELL_AGG(obj, 0) ? "Yes" : "No");
    }
    weapon_spell_2[0] = '\0';
    if (GET_WEAPON_SPELL(obj, 1))
    {
      snprintf(weapon_spell_2, sizeof(weapon_spell_2), "Spell: %s, Level: %d, Percent: %d, Procs in combat?: %s\r\n",
                       spell_info[GET_WEAPON_SPELL(obj, 1)].name, GET_WEAPON_SPELL_LVL(obj, 1),
                       GET_WEAPON_SPELL_PCT(obj, 1), GET_WEAPON_SPELL_AGG(obj, 1) ? "Yes" : "No");
    }
    weapon_spell_3[0] = '\0';
    if (GET_WEAPON_SPELL(obj, 2))
    {
      snprintf(weapon_spell_3, sizeof(weapon_spell_3), "Spell: %s, Level: %d, Percent: %d, Procs in combat?: %s\r\n",
                       spell_info[GET_WEAPON_SPELL(obj, 2)].name, GET_WEAPON_SPELL_LVL(obj, 2),
                       GET_WEAPON_SPELL_PCT(obj, 2), GET_WEAPON_SPELL_AGG(obj, 2) ? "Yes" : "No");
    }
    weapon_special_ability[0] = '\0';
    if ((specab = obj->special_abilities) != NULL)
    {
      if (specab->ability == WEAPON_SPECAB_BANE)
      {
        if (specab->value[1])
        {
          snprintf(weapon_special_ability, sizeof(weapon_special_ability), "Ability: %s Bane Race: %s Bane Subrace: %s ", 
                  special_ability_info[specab->ability].name, race_family_types[specab->value[0]], npc_subrace_types[specab->value[1]]);
        }
        else
        {
          snprintf(weapon_special_ability, sizeof(weapon_special_ability), "Ability: %s Bane Race: %s", 
                  special_ability_info[specab->ability].name, race_family_types[specab->value[0]]);
        }
      }
      else
      {
        snprintf(weapon_special_ability, sizeof(weapon_special_ability), "Ability: %s ", special_ability_info[specab->ability].name);
      }
    }
    snprintf(notes, sizeof(notes), " ");
    snprintf(temp, sizeof(temp), "%s", obj_proto[j].short_description);
    snprintf(zone_name, sizeof(zone_name), "%s", zone_table[real_zone_by_thing(obj_index[j].vnum)].name);
    mysql_real_escape_string(conn, object_name, temp, strlen(temp));
    strip_colors(object_name);
    zone_num = zone_table[real_zone_by_thing(obj_index[j].vnum)].number;

    snprintf(query, sizeof(query), "INSERT INTO object_database_items (`idnum`, `object_vnum`, `object_name`, `object_type`, "
                                    "`material`, `weight`, `object_size`, `cost`, `specific_type`, `weapon_spell_1`, `weapon_spell_2`, `weapon_spell_3`, "
                                    "`weapon_special_ability`, `minimum_level`, `zone_num`, `zone_name`, `notes`, `enhancement_bonus`) VALUES (NULL,"
                                   "%d,"              // A object_vnum
                                   "\"%s\","          // B object_name
                                   "\"%s\","          // C object_type
                                   "\"%s\","          // D material
                                   "%d,"              // E weight
                                   "\"%s\","          // F object_size
                                   "%d,"              // G cost
                                   "\"%s\","          // H specific_type
                                   "\"%s\","          // I weapon_spell_1
                                   "\"%s\","          // J weapon_spell_2
                                   "\"%s\","          // K weapon_spell_3
                                   "\"%s\","          // L weapon_special_ability
                                   "%d,"              // M minimum_level
                                   "%d,"              // N zone_num
                                   "\"%s\","          // O zone_name
                                   "\"%s\","           // P notes
                                   "%d"              // Q enhancement
                                   ")",
                                   obj_index[j].vnum,                             // A
                                   object_name,                                   // B
                                   item_types[obj_proto[j].obj_flags.type_flag],  // C
                                   material_name[obj_proto[j].obj_flags.material],// D
                                   obj_proto[j].obj_flags.weight,                 // E
                                   sizes[obj_proto[j].obj_flags.size],            // F
                                   obj_proto[j].obj_flags.cost,                   // G
                                   specific_type,                                 // H
                                   weapon_spell_1,                                // I
                                   weapon_spell_2,                                // J
                                   weapon_spell_3,                                // K
                                   weapon_special_ability,                        // L
                                   obj_proto[j].obj_flags.level,                  // M
                                   zone_num,                                      // N
                                   zone_name,                                     // O
                                   notes,                                         // P
                                   GET_ENHANCEMENT_BONUS(obj)                     // Q
    );
    if (mysql_query(conn, query))
    {
      log("SYSERR: Unable to INSERT into object_database_items: %s\nQUERY: %s", mysql_error(conn), query);
    }
    else
    {
      obj_idnum = mysql_insert_id(conn);

      for (i = 0; i < NUM_ITEM_WEARS; i++)
      {
          if (i == ITEM_WEAR_TAKE) continue;
          if (CAN_WEAR(obj, i)) {
              snprintf(query, sizeof(query), "INSERT INTO object_database_wear_slots (idnum,object_idnum,worn_slot) VALUES(NULL,%d,\"%s\")",
                        obj_idnum, wear_bits[i]);
              if (mysql_query(conn, query))
              {
                log("SYSERR: Unable to INSERT into object_database_wear_slots: %s\nQUERY: %s", mysql_error(conn), query);
              }
          }
      }
      for (i = 0; i < NUM_ITEM_FLAGS; i++)
      {
          if (OBJ_FLAGGED(obj, i)) {
              snprintf(query, sizeof(query), "INSERT INTO object_database_obj_flags (idnum,object_idnum,obj_flag) VALUES(NULL,%d,\"%s\")",
                        obj_idnum, extra_bits[i]);
              if (mysql_query(conn, query))
              {
                log("SYSERR: Unable to INSERT into object_database_obj_flags: %s\nQUERY: %s", mysql_error(conn), query);
              }
          }
      }
      for (i = 0; i < NUM_ITEM_FLAGS; i++)
      {
          if (OBJAFF_FLAGGED(obj, i)) {
              snprintf(query, sizeof(query), "INSERT INTO object_database_perm_affects (idnum,object_idnum,perm_affect) VALUES(NULL,%d,\"%s\")",
                        obj_idnum, affected_bits[i]);
              if (mysql_query(conn, query))
              {
                log("SYSERR: Unable to INSERT into object_database_perm_affects: %s\nQUERY: %s", mysql_error(conn), query);
              }
          }
      }
      for (i = 0; i < 6; i++)
      {
          if (obj->affected[i].location != APPLY_NONE && obj->affected[i].modifier != 0)
          {
            switch (obj->affected[i].location)
            {
              case APPLY_FEAT:
                snprintf(bonus, sizeof(bonus), "%s", feat_list[obj->affected[i].modifier].name);
                break;
              case APPLY_SKILL:
                snprintf(bonus, sizeof(bonus), "%s", ability_names[obj->affected[i].specific]);
                break;
              case APPLY_SPELL_CIRCLE_1:
              case APPLY_SPELL_CIRCLE_2:
              case APPLY_SPELL_CIRCLE_3:
              case APPLY_SPELL_CIRCLE_4:
              case APPLY_SPELL_CIRCLE_5:
              case APPLY_SPELL_CIRCLE_6:
              case APPLY_SPELL_CIRCLE_7:
              case APPLY_SPELL_CIRCLE_8:
              case APPLY_SPELL_CIRCLE_9:
                snprintf(bonus, sizeof(bonus), "%s", class_names[obj->affected[i].specific]);
                break;
              default:
                bonus[0] = '\0';
                break;
            }
            snprintf(query, sizeof(query), "INSERT INTO object_database_bonuses (idnum,object_idnum,bonus_location,bonus_type,bonus_modifier,bonus_specific) "
                                            "VALUES(NULL,%d,\"%s\",\"%s\",%d,\"%s\")",
                                          obj_idnum, apply_types[obj->affected[i].location], bonus_types[obj->affected[i].bonus_type], obj->affected[i].modifier, bonus);
              if (mysql_query(conn, query))
              {
                log("SYSERR: Unable to INSERT into object_database_bonuses: %s\nQUERY: %s", mysql_error(conn), query);
              }
          }
      }
    }
  }
}

/* EOF */
