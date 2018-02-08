/**
 * @file oasis.h                              Part of LuminariMUD
 * Oasis online creation general defines.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * This source code, which was not part of the CircleMUD legacy code,
 * is attributed to:
 * By Levork. Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.
 */
#ifndef _OASIS_H_
#define _OASIS_H_

#include "utils.h" /* for ACMD macro */
#include "help.h" /* for help_entry_list */

#define _OASISOLC	0x206   /* 2.0.6 */

/* Used to determine what version of OasisOLC is installed.
   Ex: #if _OASISOLC >= OASIS_VERSION(2,0,0) */
#define OASIS_VERSION(x,y,z)	(((x) << 8 | (y) << 4 | (z))

#define AEDIT_PERMISSION  999  /* arbitrary number higher than max zone vnum*/
#define HEDIT_PERMISSION  888  /* arbitrary number higher then max zone vnum*/
#define ALL_PERMISSION    666  /* arbitrary number higher then max zone vnum*/

/* Macros, defines, structs and globals for the OLC suite.  You will need
   to adjust these numbers if you ever add more. Note: Most of the NUM_ and
   MAX_ limits have been moved to more appropriate locations. */

#define TOGGLE_VAR(var)	if (var == YES) { var = NO; } else { var = YES; }
#define CHECK_VAR(var)  ((var == YES) ? "Yes" : "No")

#define MAX_PEOPLE 10 /* Max # of people you want to sit in furniture. */

/* Limit information. */
#define MAX_ROOM_NAME	150
#define MAX_MOB_NAME	100
#define MAX_OBJ_NAME	100
#define MAX_ROOM_DESC	2048
#define MAX_EXIT_DESC	256
#define MAX_EXTRA_DESC   512
#define MAX_MOB_DESC     1024
#define MAX_OBJ_DESC	512
#define MAX_DUPLICATES  100  /* when loading in zedit */

/* arbitrary limits - roll your own */
/* max weapon is 50d50 .. avg. 625 dam... */
#define MAX_WEAPON_NDICE         2
#define MAX_WEAPON_SDICE         12

#define MAX_OBJ_WEIGHT      1000000
#define MAX_OBJ_COST        2000000
#define MAX_OBJ_RENT        2000000
#define MAX_CONTAINER_SIZE    10000

#define MAX_MOB_GOLD         100000
#define MAX_MOB_EXP          150000

/* this is one mud year.. */
#define MAX_OBJ_TIMER       1071000


/* this defines how much memory is alloacted for 'bit strings' when saving in
 * OLC. Remember to change it if you go for longer bitvectors. */
#define BIT_STRING_LENGTH 33

/* The data types for miscellaneous functions. */
#define OASIS_WLD	0
#define OASIS_MOB	1
#define OASIS_OBJ	2
#define OASIS_ZON	3
#define OASIS_EXI	4
#define OASIS_CFG	5

/* Utilities exported from oasis.c. */
void cleanup_olc(struct descriptor_data *d, byte cleanup_type);
void get_char_colors(struct char_data *ch);
void split_argument(char *argument, char *tag);
void send_cannot_edit(struct char_data *ch, zone_vnum zone);

/* OLC structures. */

/* NO and YES are defined in utils.h. Removed from here. */

struct oasis_olc_data {
  int mode; /* how to parse input */
  zone_rnum zone_num; /* current zone */
  room_vnum number; /* vnum of subject */
  int value; /* mostly 'has changed' flag */
  char *storage; /* used for 'tedit' */
  struct char_data *mob; /* used for 'medit' */
  struct room_data *room; /* used for 'redit' */
  struct obj_data *obj; /* used for 'oedit' */
  struct zone_data *zone; /* used for 'zedit' */
  struct shop_data *shop; /* used for 'sedit' */
  struct config_data *config; /* used for 'cedit' */
  struct house_control_rec *house; /* used for 'hsedit' */ 
  struct aq_data *quest; /* used for 'qedit' */
  struct extra_descr_data *desc; /* used in '[r|o|m]edit' */
  struct obj_special_ability *specab; /* used in 'oedit' */
  struct social_messg *action; /* Aedit uses this one */
  struct trig_data *trig; /* trigedit */
  struct prefs_data *prefs; /* used for 'prefedit' */
  struct ibt_data *ibt; /* used for 'ibtedit' */
  struct clan_data *clan; /* used for 'clanedit' */
  struct message_list *msg;
  struct message_type *m_type;
  /* NewCraft */
  struct craft_data *craft;      /* used for 'craftedit'     */
  struct requirement_data *req;  /*           ditto          */
  
  /* Wilderness editing */
  struct region_data *region; /* Used for 'regedit' */
  struct path_data *path; /* Used for 'pathedit' */

  /* homeland-port */
  struct quest_entry *hlquest;
  struct quest_entry *entry;
  struct quest_command *qcom;

  int script_mode;
  int trigger_position;
  int item_type;
  struct trig_proto_list *script; /* for assigning triggers in [r|o|m]edit*/
  struct help_entry_list *help; /* Hedit uses this */
};

/* Exported globals. */
extern const char *nrm, *grn, *cyn, *yel, *mgn, *red;

/* Descriptor access macros. */
#define OLC(d)         ((d)->olc)
#define OLC_MODE(d)    (OLC(d)->mode)     /**< Parse input mode.	*/
#define OLC_NUM(d)     (OLC(d)->number)   /**< Room/Obj VNUM.	*/
#define OLC_VAL(d)     (OLC(d)->value)    /**< Scratch variable.	*/
#define OLC_ZNUM(d)    (OLC(d)->zone_num) /**< Real zone number.	*/
#define OLC_STORAGE(d) (OLC(d)->storage)  /**< char pointer.	*/
#define OLC_ROOM(d)    (OLC(d)->room)     /**< Room structure.	*/
#define OLC_OBJ(d)     (OLC(d)->obj)      /**< Object structure.	*/
#define OLC_ZONE(d)    (OLC(d)->zone)     /**< Zone structure.	*/
#define OLC_MOB(d)     (OLC(d)->mob)      /**< Mob structure.	*/
#define OLC_SHOP(d)    (OLC(d)->shop)     /**< Shop structure.	*/
#define OLC_DESC(d)    (OLC(d)->desc)     /**< Extra description.	*/
#define OLC_SPECAB(d)  (OLC(d)->specab)   /**< Weapon special ability */
#define OLC_CONFIG(d)  (OLC(d)->config)   /**< Config structure.	*/
#define OLC_TRIG(d)    (OLC(d)->trig)     /**< Trigger structure.   */
#define OLC_QUEST(d)   (OLC(d)->quest)    /**< Quest structure      */
#define OLC_MSG_LIST(d) (OLC(d)->msg)     /**< Message structure    */
#define OLC_HOUSE(d)    (OLC(d)->house)   /**< house structure      */ 
#define OLC_ACTION(d)  (OLC(d)->action)   /**< Action structure     */
#define OLC_HELP(d)    (OLC(d)->help)     /**< Hedit structure      */
#define OLC_PREFS(d)   (OLC(d)->prefs)    /**< Preferences structure */
#define OLC_IBT(d)     (OLC(d)->ibt)      /**< IBT (idea/bug/typo) structure */
#define OLC_CLAN(d)    (OLC(d)->clan)     /**< Clan structure       */
#define OLC_REGION(d)  (OLC(d)->region)   /**< Region structure  */

/* NewCraft */
#define OLC_CRAFT(d)     (OLC(d)->craft)    /**< Craft structure      */
#define OLC_CRAFT_REQ(d) (OLC(d)->req)

/* homeland-port */
#define OLC_HLQUEST(d)     (OLC(d)->hlquest) /* hl autoquest */
#define OLC_QCOM(d)        (OLC(d)->qcom)    /* hl autoquest */
#define OLC_QUESTENTRY(d)  (OLC(d)->entry)   /* hl autoquest */

/* Other macros. */
#define OLC_EXIT(d)	   (OLC_ROOM(d)->dir_option[OLC_VAL(d)])
#define OLC_MSG(d)     (OLC(d)->m_type)

/* Cleanup types. */
#define CLEANUP_ALL		1	/* Free the whole lot.			*/
#define CLEANUP_STRUCTS 	2	/* Don't free strings.			*/
#define CLEANUP_CONFIG          3       /* Used just to send proper message. 	*/

/* Submodes of AEDIT connectedness     */
#define AEDIT_CONFIRM_SAVESTRING       0
#define AEDIT_CONFIRM_EDIT             1
#define AEDIT_CONFIRM_ADD              2
#define AEDIT_MAIN_MENU                3
#define AEDIT_ACTION_NAME              4
#define AEDIT_SORT_AS                  5
#define AEDIT_MIN_CHAR_POS             6
#define AEDIT_MIN_VICT_POS             7
#define AEDIT_HIDDEN_FLAG              8
#define AEDIT_MIN_CHAR_LEVEL           9
#define AEDIT_NOVICT_CHAR              10
#define AEDIT_NOVICT_OTHERS            11
#define AEDIT_VICT_CHAR_FOUND          12
#define AEDIT_VICT_OTHERS_FOUND        13
#define AEDIT_VICT_VICT_FOUND          14
#define AEDIT_VICT_NOT_FOUND           15
#define AEDIT_SELF_CHAR                16
#define AEDIT_SELF_OTHERS              17
#define AEDIT_VICT_CHAR_BODY_FOUND     18
#define AEDIT_VICT_OTHERS_BODY_FOUND   19
#define AEDIT_VICT_VICT_BODY_FOUND     20
#define AEDIT_OBJ_CHAR_FOUND           21
#define AEDIT_OBJ_OTHERS_FOUND         22

/* Submodes of OEDIT connectedness. */
#define OEDIT_MAIN_MENU              	1
#define OEDIT_KEYWORD                   2
#define OEDIT_SHORTDESC              	3
#define OEDIT_LONGDESC               	4
#define OEDIT_ACTDESC                	5
#define OEDIT_TYPE                   	6
#define OEDIT_EXTRAS                 	7
#define OEDIT_WEAR                  	8
#define OEDIT_WEIGHT                	9
#define OEDIT_COST                  	10
#define OEDIT_COSTPERDAY            	11
#define OEDIT_TIMER                 	12
#define OEDIT_VALUE_1               	13
#define OEDIT_VALUE_2               	14
#define OEDIT_VALUE_3               	15
#define OEDIT_VALUE_4               	16
#define OEDIT_APPLY                 	17
#define OEDIT_APPLYMOD              	18
#define OEDIT_EXTRADESC_KEY         	19
#define OEDIT_CONFIRM_SAVEDB        	20
#define OEDIT_CONFIRM_SAVESTRING    	21
#define OEDIT_PROMPT_APPLY          	22
#define OEDIT_EXTRADESC_DESCRIPTION 	23
#define OEDIT_EXTRADESC_MENU        	24
#define OEDIT_LEVEL                 	25
#define OEDIT_PERM                      26
#define OEDIT_DELETE                    27
#define OEDIT_COPY                      28
#define OEDIT_WEAPON_SPELL_MENU         29
#define OEDIT_WEAPON_SPELLS             30
#define OEDIT_WEAPON_SPELL_PERCENT      31
#define OEDIT_WEAPON_SPELL_LEVEL        32
#define OEDIT_WEAPON_SPELL_INCOMBAT     33
#define OEDIT_SIZE 	                    34
#define OEDIT_PROF                      35	//proficiency
#define OEDIT_MATERIAL                  36
#define OEDIT_SPELLBOOK                 37
#define OEDIT_PROMPT_SPELLBOOK          38
#define OEDIT_WEAPON_SPECAB_MENU        39
#define OEDIT_ASSIGN_WEAPON_SPECAB_MENU 40
#define OEDIT_EDIT_WEAPON_SPECAB        41
#define OEDIT_DELETE_WEAPON_SPECAB      42
#define OEDIT_WEAPON_SPECAB             43 /* Chose a weapon special ability */
#define OEDIT_WEAPON_SPECAB_LEVEL       44
#define OEDIT_WEAPON_SPECAB_CMDWD       45
#define OEDIT_WEAPON_SPECAB_ACTMTD_MENU 46
#define OEDIT_WEAPON_SPECAB_ACTMTD      47
#define OEDIT_SPECAB_VALUE_1            48
#define OEDIT_SPECAB_VALUE_2            49
#define OEDIT_SPECAB_VALUE_3            50
#define OEDIT_SPECAB_VALUE_4            51
#define OEDIT_VALUE_5               	  52
#define OEDIT_APPLY_BONUS_TYPE          53
#define OEDIT_APPLYSPEC                 54


/* Submodes of REDIT connectedness. */
#define REDIT_MAIN_MENU 		1
#define REDIT_NAME 			2
#define REDIT_DESC 			3
#define REDIT_FLAGS 			4
#define REDIT_SECTOR 			5
#define REDIT_EXIT_MENU 		6
#define REDIT_CONFIRM_SAVEDB 		7
#define REDIT_CONFIRM_SAVESTRING 	8
#define REDIT_EXIT_NUMBER 		9
#define REDIT_EXIT_DESCRIPTION 		10
#define REDIT_EXIT_KEYWORD 		11
#define REDIT_EXIT_KEY 			12
#define REDIT_EXIT_DOORFLAGS 		13
#define REDIT_EXTRADESC_MENU 		14
#define REDIT_EXTRADESC_KEY 		15
#define REDIT_EXTRADESC_DESCRIPTION 	16
#define REDIT_DELETE			17
#define REDIT_COPY			18
#define REDIT_X_COORD			19
#define REDIT_Y_COORD			20

/* Submodes of ZEDIT connectedness. */
#define ZEDIT_MAIN_MENU            0
#define ZEDIT_DELETE_ENTRY         1
#define ZEDIT_NEW_ENTRY			2
#define ZEDIT_CHANGE_ENTRY		3
#define ZEDIT_COMMAND_TYPE		4
#define ZEDIT_IF_FLAG			5
#define ZEDIT_ARG1                 6
#define ZEDIT_ARG2                 7
#define ZEDIT_ARG3                 8
#define ZEDIT_ZONE_NAME			9
#define ZEDIT_ZONE_LIFE			10
#define ZEDIT_ZONE_BOT			11
#define ZEDIT_ZONE_TOP			12
#define ZEDIT_ZONE_RESET           13
#define ZEDIT_CONFIRM_SAVESTRING	14
#define ZEDIT_ZONE_BUILDERS		15
#define ZEDIT_SARG1                20
#define ZEDIT_SARG2                21
#define ZEDIT_ZONE_FLAGS           22
#define ZEDIT_LEVELS               23
#define ZEDIT_LEV_MIN              24
#define ZEDIT_LEV_MAX              25
#define ZEDIT_ZONE_CLAIM           26
#define ZEDIT_ZONE_WEATHER         27
#define ZEDIT_ARG4                 28
#define ZEDIT_GR_QUERY             29
#define ZEDIT_CONFIRM_RESTAT       30

/* Submodes of MEDIT connectedness. */
#define MEDIT_MAIN_MENU            	0
#define MEDIT_KEYWORD               1
#define MEDIT_S_DESC                2
#define MEDIT_L_DESC                3
#define MEDIT_D_DESC                4
#define MEDIT_NPC_FLAGS             5
#define MEDIT_AFF_FLAGS             6
#define MEDIT_CONFIRM_SAVESTRING    7
#define MEDIT_STATS_MENU            8
#define MEDIT_WALKIN                    9
#define MEDIT_WALKOUT                   10
#define MEDIT_ECHO_MENU                 11
#define MEDIT_ADD_ECHO                  12
#define MEDIT_EDIT_ECHO                 13
#define MEDIT_EDIT_ECHO_TEXT            14
#define MEDIT_RESISTANCES_MENU          15
#define MEDIT_PATH_DELAY                16
#define MEDIT_PATH_EDIT                 17

/* Numerical responses. */
#define MEDIT_NUMERICAL_RESPONSE	20
#define MEDIT_SEX			21
#define MEDIT_HITROLL			22
#define MEDIT_DAMROLL			23
#define MEDIT_NDD			24
#define MEDIT_SDD			25
#define MEDIT_NUM_HP_DICE		26
#define MEDIT_SIZE_HP_DICE		27
#define MEDIT_ADD_HP			28
#define MEDIT_AC			29
#define MEDIT_EXP			30
#define MEDIT_GOLD			31
#define MEDIT_POS			32
#define MEDIT_DEFAULT_POS		33
#define MEDIT_ATTACK			34
#define MEDIT_LEVEL			35
#define MEDIT_ALIGNMENT			36
#define MEDIT_DELETE                    37
#define MEDIT_COPY                      38
#define MEDIT_STR                       39
#define MEDIT_INT                       40
#define MEDIT_WIS                       41
#define MEDIT_DEX                       42
#define MEDIT_CON                       43
#define MEDIT_CHA                       44
#define MEDIT_PARA                      45
#define MEDIT_ROD                       46
#define MEDIT_PETRI                     47
#define MEDIT_BREATH                    48
#define MEDIT_SPELL                     49
#define MEDIT_RACE			50
#define MEDIT_CLASS			51
#define MEDIT_SIZE			52
#define MEDIT_SUB_RACE_1		53
#define MEDIT_SUB_RACE_2		54
#define MEDIT_SUB_RACE_3		55
#define MEDIT_ECHO_FREQUENCY            56
#define MEDIT_DELETE_ECHO               57
#define MEDIT_DAM_FIRE		58
#define MEDIT_DAM_COLD		59
#define MEDIT_DAM_AIR		60
#define MEDIT_DAM_EARTH		61
#define MEDIT_DAM_ACID        62
#define MEDIT_DAM_HOLY		63
#define MEDIT_DAM_ELECTRIC	64
#define MEDIT_DAM_UNHOLY		65
#define MEDIT_DAM_SLICE		66
#define MEDIT_DAM_PUNCTURE	67
#define MEDIT_DAM_FORCE		68
#define MEDIT_DAM_SOUND		69
#define MEDIT_DAM_POISON		70
#define MEDIT_DAM_DISEASE	71
#define MEDIT_DAM_NEGATIVE	72
#define MEDIT_DAM_ILLUSION	73
#define MEDIT_DAM_MENTAL		74
#define MEDIT_DAM_LIGHT		75
#define MEDIT_DAM_ENERGY		76
#define MEDIT_DAM_WATER		77


/* Submodes of SEDIT connectedness. */
#define SEDIT_MAIN_MENU              	0
#define SEDIT_CONFIRM_SAVESTRING	1
#define SEDIT_NOITEM1			2
#define SEDIT_NOITEM2			3
#define SEDIT_NOCASH1			4
#define SEDIT_NOCASH2			5
#define SEDIT_NOBUY			6
#define SEDIT_BUY			7
#define SEDIT_SELL			8
#define SEDIT_PRODUCTS_MENU		11
#define SEDIT_ROOMS_MENU		12
#define SEDIT_NAMELIST_MENU		13
#define SEDIT_NAMELIST			14
#define SEDIT_COPY                      15

#define SEDIT_NUMERICAL_RESPONSE	20
#define SEDIT_OPEN1			21
#define SEDIT_OPEN2			22
#define SEDIT_CLOSE1			23
#define SEDIT_CLOSE2			24
#define SEDIT_KEEPER			25
#define SEDIT_BUY_PROFIT		26
#define SEDIT_SELL_PROFIT		27
#define SEDIT_TYPE_MENU			29
#define SEDIT_DELETE_TYPE		30
#define SEDIT_DELETE_PRODUCT		31
#define SEDIT_NEW_PRODUCT		32
#define SEDIT_DELETE_ROOM		33
#define SEDIT_NEW_ROOM			34
#define SEDIT_SHOP_FLAGS		35
#define SEDIT_NOTRADE			36

/* Submodes of CEDIT connectedness. */
#define CEDIT_MAIN_MENU			0
#define CEDIT_CONFIRM_SAVESTRING	1
#define CEDIT_GAME_OPTIONS_MENU		2
#define CEDIT_CRASHSAVE_OPTIONS_MENU	3
#define CEDIT_OPERATION_OPTIONS_MENU	4
#define CEDIT_DISP_EXPERIENCE_MENU	5
#define CEDIT_ROOM_NUMBERS_MENU		6
#define CEDIT_AUTOWIZ_OPTIONS_MENU	7
#define CEDIT_OK			8
#define CEDIT_NOPERSON			9
#define CEDIT_NOEFFECT			10
#define CEDIT_DFLT_IP			11
#define CEDIT_DFLT_DIR			12
#define CEDIT_LOGNAME			13
#define CEDIT_MENU			14
#define CEDIT_WELC_MESSG		15
#define CEDIT_START_MESSG		16

/* Numerical responses. */
#define CEDIT_NUMERICAL_RESPONSE	20
#define CEDIT_LEVEL_CAN_SHOUT		21
#define CEDIT_HOLLER_MOVE_COST		22
#define CEDIT_TUNNEL_SIZE		23
#define CEDIT_MAX_EXP_GAIN		24
#define CEDIT_MAX_EXP_LOSS		25
#define CEDIT_MAX_NPC_CORPSE_TIME	26
#define CEDIT_MAX_PC_CORPSE_TIME	27
#define CEDIT_IDLE_VOID			28
#define CEDIT_IDLE_RENT_TIME		29
#define CEDIT_IDLE_MAX_LEVEL		30
#define CEDIT_DTS_ARE_DUMPS		31
#define CEDIT_LOAD_INTO_INVENTORY	32
#define CEDIT_TRACK_THROUGH_DOORS	33
#define CEDIT_NO_MORT_TO_IMMORT		34
#define CEDIT_MAX_OBJ_SAVE		35
#define CEDIT_MIN_RENT_COST		36
#define CEDIT_AUTOSAVE_TIME		37
#define CEDIT_CRASH_FILE_TIMEOUT	38
#define CEDIT_RENT_FILE_TIMEOUT		39
#define CEDIT_MORTAL_START_ROOM		40
#define CEDIT_IMMORT_START_ROOM		41
#define CEDIT_FROZEN_START_ROOM		42
#define CEDIT_DONATION_ROOM_1		43
#define CEDIT_DONATION_ROOM_2		44
#define CEDIT_DONATION_ROOM_3		45
#define CEDIT_DFLT_PORT			46
#define CEDIT_MAX_PLAYING		47
#define CEDIT_MAX_FILESIZE		48
#define CEDIT_MAX_BAD_PWS		49
#define CEDIT_SITEOK_EVERYONE		50
#define CEDIT_NAMESERVER_IS_SLOW	51
#define CEDIT_USE_AUTOWIZ		52
#define CEDIT_MIN_WIZLIST_LEV		53
#define CEDIT_MAP_OPTION   54
#define CEDIT_MAP_SIZE     55
#define CEDIT_MINIMAP_SIZE   56
#define CEDIT_POPULARITY     57
#define CEDIT_DEBUG_MODE     58

/* Hedit Submodes of connectedness. */
#define HEDIT_CONFIRM_SAVESTRING        0
#define HEDIT_CONFIRM_EDIT              1
#define HEDIT_CONFIRM_ADD               2
#define HEDIT_MAIN_MENU                 3
#define HEDIT_ENTRY                     4
#define HEDIT_TAG                       5
#define HEDIT_MIN_LEVEL                 6
#define HEDIT_KEYWORD_MENU              7
#define HEDIT_NEW_KEYWORD               8
#define HEDIT_DEL_KEYWORD               9
#define HEDIT_CONFIRM_DELETE           10

/*. House editor .*/ 
 #define HSEDIT_MAIN_MENU                 0 
 #define HSEDIT_CONFIRM_SAVESTRING        1 
 #define HSEDIT_OWNER_MENU                2 
 #define HSEDIT_OWNER_NAME                3 
 #define HSEDIT_OWNER_ID                  4 
 #define HSEDIT_ROOM                      5 
 #define HSEDIT_ATRIUM                    6 
 #define HSEDIT_DIR_MENU                  7 
 #define HSEDIT_GUEST_MENU                8 
 #define HSEDIT_GUEST_ADD                 9 
 #define HSEDIT_GUEST_DELETE              10 
 #define HSEDIT_GUEST_CLEAR               11 
 #define HSEDIT_FLAGS                     12 
 #define HSEDIT_BUILD_DATE                13 
 #define HSEDIT_PAYMENT                   14 
 #define HSEDIT_TYPE                      15 
 #define HSEDIT_DELETE                    16 
 #define HSEDIT_VALUE_0                   17 
 #define HSEDIT_VALUE_1                   18 
 #define HSEDIT_VALUE_2                   19 
 #define HSEDIT_VALUE_3                   20 
 #define HSEDIT_NOVNUM                    21 
 #define HSEDIT_BUILDER                   22 

/* Clanedit Submodes of connectedness. */
#define CLANEDIT_CONFIRM_SAVESTRING    0    /**< Submode for quit option, does player want to save? */
#define CLANEDIT_MAIN_MENU             1    /**< Submode for the main clanedit menu                 */
#define CLANEDIT_RANK_MENU             2    /**< Submode for the rank editor sub-menu               */
#define CLANEDIT_PRIV_MENU             3    /**< Submode for the privilege editor sub-menu          */
#define CLANEDIT_NAME                  4    /**< Edit the name of the clan (should be leader-only?) */
#define CLANEDIT_ABBREV                5    /**< Edit the abbreviated name of the clan              */
#define CLANEDIT_DESC                  6    /**< Edit the clan's description                        */
#define CLANEDIT_APPLEV                7    /**< Edit the minimum level required to apply to clan   */
#define CLANEDIT_APPFEE                8    /**< Edit the fee paid by clan applicants               */
#define CLANEDIT_TAXRATE               9    /**< Edit Tax Rate paid by members of other clans       */
#define CLANEDIT_ATWAR                 10   /**< Edit Clan's Enemy                                  */
#define CLANEDIT_ALLIED                11   /**< Edit Clan's Ally                                   */
#define CLANEDIT_CP_CLAIM              12   /**< Edit Clan Priv: Claim a zone                       */
#define CLANEDIT_CP_BALANCE            13
#define CLANEDIT_CP_DEMOTE             14   /**< Edit Clan Priv: Demote a clan member               */
#define CLANEDIT_CP_DEPOSIT            15   /**< Edit Clan Priv: Deposit into clan bank             */
#define CLANEDIT_CP_CLANEDIT           16   /**< Edit Clan Priv: Use Clanedit OLC (see CPs below)   */
#define CLANEDIT_CP_ENROL              17   /**< Edit Clan Priv: Enrol an applicant into the clan   */
#define CLANEDIT_CP_EXPEL              18   /**< Edit Clan Priv: Kick a clan member out of the clan */
#define CLANEDIT_CP_OWNER              19   /**< Edit Clan Priv: Change clan ownership              */
#define CLANEDIT_CP_PROMOTE            20   /**< Edit Clan Priv: Promote a clan member              */
#define CLANEDIT_CP_WHERE              21   /**< Edit Clan Priv: Find other clan members            */
#define CLANEDIT_CP_WITHDRAW           22   /**< Edit Clan Priv: Withdraw from clan bank            */
#define CLANEDIT_CP_ALLIED             23   /**< Edit Clan Priv: Set allied (friendly) clan         */
#define CLANEDIT_CP_APPFEE             24   /**< Edit Clan Priv: Set Application Fee                */
#define CLANEDIT_CP_APPLEV             25   /**< Edit Clan Priv: Set Application Min. Level         */
#define CLANEDIT_CP_DESC               26   /**< Edit Clan Priv: Set Clan's description             */
#define CLANEDIT_CP_TAXRATE            27   /**< Edit Clan Priv: Set clan tax rate                  */
#define CLANEDIT_CP_RANKS              28   /**< Edit Clan Priv: Set ranks and rank names           */
#define CLANEDIT_CP_TITLE              29   /**< Edit Clan Priv: Set clan name (title)              */
#define CLANEDIT_CP_ATWAR              30   /**< Edit Clan Priv: Set enemy clan                     */
#define CLANEDIT_CP_SETPRIVS           31   /**< Edit Clan Priv: Set priv ranks                     */
#define CLANEDIT_NUM_RANKS             32   /**< Edit Ranks: Set the number of ranks                */
#define CLANEDIT_RANK_NAME             33   /**< Edit Ranks: Edit one of the rank names             */


/* study Submodes of connectedness. */
#define STUDY_SPELLS           1
#define FAVORED_ENEMY          2
#define ANIMAL_COMPANION       3
#define FAVORED_ENEMY_SUB      4
#define ANIMAL_COMPANION_SUB   5
#define FAMILIAR_MENU          6
#define BARD_STUDY_SPELLS      7
#define STUDY_MAIN_FEAT_MENU   8
#define STUDY_GEN_FEAT_MENU    9
#define STUDY_CFEAT_MENU       10
#define STUDY_SFEAT_MENU       11
#define STUDY_SKFEAT_MENU      12
#define STUDY_CONFIRM_SAVE     13
#define STUDY_GEN_MAIN_MENU    14
#define STUDY_EPIC_CLASS_FEAT_MENU   15
#define STUDY_SORC_KNOWN_SPELLS_MENU 16
#define STUDY_BARD_KNOWN_SPELLS_MENU 17
#define STUDY_CONFIRM_ADD_FEAT 18
#define STUDY_SET_STATS        19
#define SET_STAT_STR           20
#define SET_STAT_DEX           21
#define SET_STAT_CON           22
#define SET_STAT_INTE          23
#define SET_STAT_WIS           24
#define SET_STAT_CHA           25
#define STUDY_SET_DOMAINS      26
#define SET_1ST_DOMAIN         27
#define SET_2ND_DOMAIN         28
#define STUDY_SET_SCHOOL       29
#define SET_SCHOOL             30
#define STUDY_SET_P_CASTER     31
#define SET_PREFERRED_ARCANE   32
#define SET_PREFERRED_DIVINE   33
#define STUDY_SET_S_BLOODLINE  34
#define SET_BLOODLINE_DRACONIC 35
#define STUDY_CONFIRM_BLOODLINE 36
#define SET_BLOODLINE_ARCANE   37

int save_config(IDXTYPE nowhere);

/* Prototypes to keep. */
void clear_screen(struct descriptor_data *);
int can_edit_zone(struct char_data *ch, zone_rnum rnum);
ACMD(do_oasis);

/* public functions from medit.c */
void medit_setup_existing(struct descriptor_data *d, int rnum);
void medit_setup_new(struct descriptor_data *d);
void medit_save_internally(struct descriptor_data *d);
void medit_parse(struct descriptor_data *d, char *arg);
void medit_string_cleanup(struct descriptor_data *d, int terminator);
ACMD(do_oasis_medit);
void medit_autoroll_stats(struct descriptor_data *d);
void autoroll_mob(struct char_data *mob, bool realmode, bool summoned);

/* public functions from oedit.c */
void oedit_setup_existing(struct descriptor_data *d, int rnum);
void oedit_save_internally(struct descriptor_data *d);
void oedit_parse(struct descriptor_data *d, char *arg);
void oedit_string_cleanup(struct descriptor_data *d, int terminator);
void oedit_disp_armor_type_menu(struct descriptor_data *d);
void oedit_disp_weapon_type_menu(struct descriptor_data *d);
ACMD(do_oasis_oedit);

/* public functions from redit.c */
void redit_setup_existing(struct descriptor_data *d, int rnum);
void redit_string_cleanup(struct descriptor_data *d, int terminator);
void redit_save_internally(struct descriptor_data *d);
void redit_save_to_disk(zone_vnum zone_num);
void redit_parse(struct descriptor_data *d, char *arg);
void free_room(struct room_data *room);
ACMD(do_oasis_redit);

/* public functions from sedit.c */
void sedit_setup_existing(struct descriptor_data *d, int rnum);
void sedit_save_internally(struct descriptor_data *d);
void sedit_parse(struct descriptor_data *d, char *arg);
ACMD(do_oasis_sedit);

/* public functions from zedit.c */
void zedit_parse(struct descriptor_data *d, char *arg);
ACMD(do_oasis_zedit);

/* public functions from cedit.c */
void cedit_save_to_disk(void);
void cedit_parse(struct descriptor_data *d, char *arg);
void cedit_string_cleanup(struct descriptor_data *d, int terminator);
ACMD(do_oasis_cedit);

/* public functions from dg_olc.c */
void trigedit_parse(struct descriptor_data *d, char *arg);
ACMD(do_oasis_trigedit);

/* public functions from from aedit.c */
void aedit_parse(struct descriptor_data * d, char *arg);
ACMD(do_oasis_aedit);
ACMD(do_astat);

/* public functions from hedit.c */
void hedit_parse(struct descriptor_data *d, char *arg);
void hedit_string_cleanup(struct descriptor_data *d, int terminator);
void free_help(struct help_index_element *help);
ACMD(do_oasis_hedit);

/* public functions from hsedit.c */
 void hsedit_save_to_disk( void ); 
 void hsedit_setup_new(struct descriptor_data *d); 
 void hsedit_setup_existing(struct descriptor_data *d, int real_num); 
 void hsedit_parse(struct descriptor_data *d, char *arg); 
 void hsedit_string_cleanup(struct descriptor_data *d, int terminator); 
 void free_house(struct house_control_rec *house); 
 ACMD(do_oasis_hsedit); 
 
/* public functions from tedit.c */
void tedit_string_cleanup(struct descriptor_data *d, int terminator);
ACMD(do_tedit);

/* public functions from qedit.c */
ACMD(do_oasis_qedit);
void qedit_save_internally(struct descriptor_data *d);
void qedit_setup_existing(struct descriptor_data *d, qst_rnum rnum);

/* NewCraft */
/* public functions from crafts.c */
void craftedit_parse(struct descriptor_data *d, char *arg);
ACMD(do_oasis_craftedit);

/* public functions from study.c */
ACMD(do_study);
void study_parse(struct descriptor_data *d, char *arg);
int stat_points_left(struct char_data *ch);
int compute_total_stat_points(struct char_data *ch);
int compute_cha_cost(struct char_data *ch, int number);
int compute_base_cha(struct char_data *ch);
int compute_wis_cost(struct char_data *ch, int number);
int compute_base_wis(struct char_data *ch);
int compute_inte_cost(struct char_data *ch, int number);
int compute_base_inte(struct char_data *ch);
int compute_con_cost(struct char_data *ch, int number);
int compute_base_con(struct char_data *ch);
int compute_str_cost(struct char_data *ch, int number);
int compute_base_str(struct char_data *ch);
int compute_dex_cost(struct char_data *ch, int number);
int compute_base_dex(struct char_data *ch);


/* public functions from msgedit.c */
ACMD(do_msgedit);
void msgedit_parse(struct descriptor_data *d, char *arg);

/* public functions from oasis_copy.c */
int buildwalk(struct char_data *ch, int dir);
ACMD(do_dig);
ACMD(do_oasis_copy);

/* public functions from oasis_delete.c */
int free_strings(void *data, int type);

/* public functions from oasis_list.c */
void print_zone(struct char_data *ch, zone_rnum rnum);
/** @deprecated is do_oasis_links intentionally dead code? */
ACMD(do_oasis_links);
ACMD(do_oasis_list);

#endif /* _OASIS_H_ */
