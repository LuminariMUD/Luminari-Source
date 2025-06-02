/*

Clan Header File
 *  Authors:  Jamdog (ported to Luminari by Bakarus and Zusuk)

*/

/* Standard clan privs */
#define CP_NONE -2 /**< RESERVED! - Special case for 'no players'     */
#define CP_ALL -1  /**< RESERVED! - Special case for 'all players'    */
#define CP_AWARD 0 /**< Clan Priv: Award clan points to a clan member */
#define CP_BALANCE 1
#define CP_CLAIM 2     /**< Clan Priv: Claim a zone                       */
#define CP_DEMOTE 3    /**< Clan Priv: Demote a clan member               */
#define CP_DEPOSIT 4   /**< Clan Priv: Deposit into clan bank             */
#define CP_CLANEDIT 5  /**< Clan Priv: Use Clanedit OLC (see CPs below)   */
#define CP_ENROL 6     /**< Clan Priv: Enrol an applicant into the clan   */
#define CP_EXPEL 7     /**< Clan Priv: Kick a clan member out of the clan */
#define CP_OWNER 8     /**< Clan Priv: Change clan ownership              */
#define CP_PROMOTE 9   /**< Clan Priv: Promote a clan member              */
#define CP_WHERE 10    /**< Clan Priv: Find other clan members            */
#define CP_WITHDRAW 11 /**< Clan Priv: Withdraw from clan bank            */

/* Clanedit clan privs - for clan settings */
#define CP_ALLIED 12   /**< Clan Priv: Set allied (friendly) clan         */
#define CP_APPFEE 13   /**< Clan Priv: Set Application Fee                */
#define CP_APPLEV 14   /**< Clan Priv: Set Application Min. Level         */
#define CP_DESC 15     /**< Clan Priv: Set Clan's description             */
#define CP_TAXRATE 16  /**< Clan Priv: Set clan tax rate                  */
#define CP_RANKS 17    /**< Clan Priv: Set ranks and rank names           */
#define CP_TITLE 18    /**< Clan Priv: Set clan name (title)              */
#define CP_ATWAR 19    /**< Clan Priv: Set enemy clan                     */
#define CP_SETPRIVS 20 /**< Clan Priv: Set priv ranks                     */

#define NUM_CLAN_PRIVS 21

#define NO_CLAN ((IDXTYPE)~0) /**< Sets to ush_int_MAX, or 65,535 */
#define NO_CLANRANK 0         /**< A non-ranking value            */
#define RANK_LEADERONLY 0     /**< For clan privs, 0 is higher than top rank */

/**< The maximum number of clans allowed (do not exceed 26)  */
#define MAX_CLANS 25
/**< The maximum number of ranks per clan (do not exceed 16) */
#define MAX_CLANRANKS 15
/**< The maximum number of clan spells per clan (do not exceed 5) */
#define MAX_CLANSPELLS 5
/**< The maximum string length for clan names */
#define MAX_CLAN_NAME 60
/**< The maximum popularity value             */
#define MAX_POPULARITY (100.0)
/**< The maximum length of a clan description */
#define MAX_CLAN_DESC 2048
//letters of abreviation
#define MAX_CLAN_ABREV 5

#define CLAN_NAME(c) (clan_list[c].clan_name)
#define CLAN_LEADER(c) (clan_list[c].leader)
#define CLAN_PRIV(c, p) (clan_list[c].privilege[p])
#define CLAN_BANK(c) (clan_list[c].treasure)
#define CLAN_ABREV(c) (clan_list[c].abrev)
#define CC_CMD(i) (clan_commands[(i)].command)
#define CC_PRIV(i) (clan_commands[(i)].priv_rank)
#define CC_ILEV(i) (clan_commands[(i)].imm_lvl)
#define CC_PRIV(i) (clan_commands[(i)].priv_rank)
#define CC_FUNC(i) (clan_commands[(i)].command_pointer)
#define CC_TEXT(i) (clan_commands[(i)].list_text)

#define CLAIM_ZONE(i) ((i)->zn)
#define CLAIM_CLAN(i) ((i)->clan)
#define CLAIM_CLAIMANT(i) ((i)->claimant)
#define CLAIM_POPULARITY(i, j) ((i)->popularity[(j)])

#define IS_IN_CLAN(ch) (GET_CLAN(ch) != 0 && GET_CLAN(ch) != NO_CLAN)
/* Used only by clan edit OLC for checking editor's privs */
/* Returns true for Imps, clan leader, or if member has sufficient rank */
#define CHK_CP(p) ((GET_LEVEL(d->character) == LVL_IMPL) ||            \
                   (GET_IDNUM(d->character) == OLC_CLAN(d)->leader) || \
                   (GET_CLANRANK(d->character) <= OLC_CLAN(d)->privilege[(p)]))

/** Diplomacy data is used for the do_diplomacy general command (act.other.c) */
struct diplomacy_data
{
       /**< The subcommand number, defined in interpreter.h */
       int subcmd;
       /**< The skill number, defined in spells.h           */
       int skill;
       /**< The popularity increase this skill causes       */
       float increase;
       /**< The number of ticks that must pass between uses */
       int wait;
};

/** Claim data holds information regarding zone claims */
struct claim_data
{
       /**< The VNUM of the zone that this data is for     */
       zone_vnum zn;
       /**< The ID of the player who last claimed the zone */
       long claimant;
       /**< The VNUM of the current controlling clan       */
       clan_vnum clan;
       /**< Popularity Values for the zone for each clan   */
       float popularity[MAX_CLANS];
       /**< Linked list pointer to the next claim_data     */
       struct claim_data *next;
};

/** Clan command structure - data entered at the top of clan.c */
struct clan_cmds
{
       const char *command;
       int priv_rank;
       int imm_lvl;
       void (*command_pointer)(struct char_data *ch, const char *argument, int cmd, int subcmd);
       const char *list_text;
};

/* The main clan data structure - holds all clan information */
struct clan_data
{
       clan_vnum vnum;                  /**< Unique vnum of this clan       */
       char *clan_name;                 /**< The name of this clan          */
       char *description;               /**< The clan's information         */
       long leader;                     /**< The ID of the clan leader      */
       ubyte ranks;                     /**< Number of clan ranks           */
       char *rank_name[MAX_CLANRANKS];  /**< Clan rank names                */
       ubyte privilege[NUM_CLAN_PRIVS]; /**< Priv ranks for members         */
       int applev;                      /**< Min level to apply to clan     */
       int appfee;                      /**< Cost when application accepted */
       int taxrate;                     /**< Tax rate for other clans       */
       int war_timer;                   /**< Ticks left for current 'war'   */
       zone_vnum hall;                  /**< The zone for the clan's hall   */
       int at_war[MAX_CLANS];           /**< TRUE if at war                 */
       int allies[MAX_CLANS];           /**< TRUE if allied                 */
       int spells[MAX_CLANSPELLS];      /**< Five skills known by all members */
       long treasure;                   /**< The clan's bank account        */
       int pk_win;                      /**< How many PK's have been won    */
       int pk_lose;                     /**< How many PK's have been lost   */
       int raided;                      /**< How many times been raided */
       char *abrev;                     /**< Abbreviation for the clan      */
};

/* globals */
extern struct clan_data *clan_list;
extern struct claim_data *claim_list;
extern int num_of_clans;

int find_clan_by_id(int clan_id);
clan_rnum real_clan(clan_vnum c);
clan_rnum get_clan_by_name(const char *c_n);
int count_clan_members(clan_rnum c);
int count_clan_power(clan_rnum c);
clan_vnum highest_clan_vnum(void);
bool add_clan(struct clan_data *this_clan);
bool remove_clan(clan_vnum c_v);
void free_clan(struct clan_data *c);
void free_clan_list(void);
bool set_clan(struct char_data *ch, clan_vnum c_v);
void copy_clan_data(struct clan_data *to_clan, struct clan_data *from_clan);
bool auto_appoint_new_clan_leader(clan_rnum c_n);
bool is_a_clan_leader(struct char_data *ch);
void clear_clan_vals(struct clan_data *cl);
void load_clans(void);
void save_clans(void);
bool can_edit_clan(struct char_data *ch, clan_vnum c);
void duplicate_clan_data(struct clan_data *to_clan,
                         struct clan_data *from_clan);
bool are_clans_allied(int clanA, int clanB);
bool are_clans_at_war(int clanA, int clanB);
ACMD_DECL(do_clan);
ACMD_DECL(do_clanapply);
ACMD_DECL(do_clanaward);
ACMD_DECL(do_clanclaim);
ACMD_DECL(do_clancreate);
ACMD_DECL(do_clanbalance);
ACMD_DECL(do_clandemote);
ACMD_DECL(do_clandeposit);
ACMD_DECL(do_clandestroy);
ACMD_DECL(do_clanedit);
ACMD_DECL(do_clanenrol);
ACMD_DECL(do_clanexpel);
ACMD_DECL(do_claninfo);
ACMD_DECL(do_clanlist);
ACMD_DECL(do_clanowner);
ACMD_DECL(do_clanpromote);
ACMD_DECL(do_clanstatus);
ACMD_DECL(do_clanwhere);
ACMD_DECL(do_clanwithdraw);
ACMD_DECL(do_clanunclaim);
ACMD_DECL(do_clanleave);

ACMD_DECL(do_clanset);
ACMD_DECL(do_clantalk);
clan_vnum zone_is_clanhall(zone_vnum z);
zone_vnum get_clanhall_by_char(struct char_data *ch);
int get_clan_taxrate(struct char_data *ch);
bool check_clanpriv(struct char_data *ch, int p);
void do_clan_tax_losses(struct char_data *ch, int amount);

/* Claiming functions */
struct claim_data *new_claim(void);
struct claim_data *get_claim_by_zone(zone_vnum z_num);
void remove_claim_from_list(struct claim_data *rem_claim);
void free_claim(struct claim_data *this_claim);
void free_claim_list(void);
struct claim_data *add_claim(zone_vnum z, clan_vnum c, long p_id);
struct claim_data *add_no_claim(zone_vnum z);
struct claim_data *add_claim_by_char(struct char_data *ch, zone_vnum z);
void show_claims(struct char_data *ch, char *arg);
void show_clan_claims(struct char_data *ch, clan_vnum c);
clan_vnum get_owning_clan(zone_vnum z);
clan_vnum get_owning_clan_by_char(struct char_data *ch);
long get_claimant_id(zone_vnum z);
bool save_claims(void);
void load_claims(void);

/* Popularity handling functions */
float get_popularity(zone_vnum zn, clan_vnum cn);
void increase_popularity(zone_vnum zn, clan_vnum cn, float amt);
void show_zone_popularities(struct char_data *ch,
                            struct claim_data *this_claim);
void show_clan_popularities(struct char_data *ch, clan_vnum c_v);
void show_popularity(struct char_data *ch, char *arg);
void check_diplomacy(void);

/* clan edit OLC Functions */
void clanedit_parse(struct descriptor_data *d, char *arg);
void clanedit_string_cleanup(struct descriptor_data *d, int terminator);

#ifndef __CLAN_C__
/* External globals (proably needed by any file that loads this header) */
extern struct clan_data *clan_list;
extern int num_of_clans;
//extern struct clan_rec clan[MAX_CLANS];
#endif
