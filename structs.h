/**
 * @file structs.h
 * Core structures used within the core mud code.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
 */
#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include "protocol.h" /* Kavir Plugin*/
#include "lists.h"

/** Intended use of this macro is to allow external packages to work with a
 * variety of versions without modifications.  For instance, an IS_CORPSE()
 * macro was introduced in pl13.  Any future code add-ons could take into
 * account the version and supply their own definition for the macro if used
 * on an older version. You are supposed to compare this with the macro
 * TBAMUD_VERSION() in utils.h.
 * It is read as Major/Minor/Patchlevel - MMmmPP */
#define _TBAMUD    0x030640

/** If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1 because
 * TRUE/FALSE aren't defined yet. */
#define USE_AUTOEQ	1

/* preamble */
/** As of bpl20, it should be safe to use unsigned data types for the various
 * virtual and real number data types.  There really isn't a reason to use
 * signed anymore so use the unsigned types and get 65,535 objects instead of
 * 32,768. NOTE: This will likely be unconditionally unsigned later.
 * 0 = use signed indexes; 1 = use unsigned indexes */
#define CIRCLE_UNSIGNED_INDEX	1

#if CIRCLE_UNSIGNED_INDEX
#define IDXTYPE	  ush_int          /**< Index types are unsigned short ints */
#define IDXTYPE_MAX   USHRT_MAX     /**< Used for compatibility checks. */
#define IDXTYPE_MIN   0             /**< Used for compatibility checks. */
#define NOWHERE	  ((IDXTYPE)~0)    /**< Sets to ush_int_MAX, or 65,535 */
#define NOTHING	  ((IDXTYPE)~0)    /**< Sets to ush_int_MAX, or 65,535 */
#define NOBODY		  ((IDXTYPE)~0)    /**< Sets to ush_int_MAX, or 65,535 */
#define NOFLAG        ((IDXTYPE)~0)    /**< Sets to ush_int_MAX, or 65,535 */
#else
#define IDXTYPE	  sh_int           /**< Index types are unsigned short ints */
#define IDXTYPE_MAX   SHRT_MAX      /**< Used for compatibility checks. */
#define IDXTYPE_MIN   SHRT_MIN      /**< Used for compatibility checks. */
#define NOWHERE	  ((IDXTYPE)-1)    /**< nil reference for rooms */
#define NOTHING	  ((IDXTYPE)-1)    /**< nil reference for objects */
#define NOBODY		  ((IDXTYPE)-1)	   /**< nil reference for mobiles  */
#define NOFLAG        ((IDXTYPE)-1)    /**< nil reference for flags   */
#endif

/** Function macro for the mob, obj and room special functions */
#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)

/* room-related defines */
/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0    /**< The direction north */
#define EAST           1    /**< The direction east */
#define SOUTH          2    /**< The direction south */
#define WEST           3    /**< The direction west */
#define UP             4    /**< The direction up */
#define DOWN           5    /**< The direction down */
#define NORTHWEST      6    /**< The direction north-west */
#define NORTHEAST      7    /**< The direction north-east */
#define SOUTHEAST      8    /**< The direction south-east */
#define SOUTHWEST      9    /**< The direction south-west */
/** Total number of directions available to move in. BEFORE CHANGING THIS, make
 * sure you change every other direction and movement based item that this will
 * impact. */
#define NUM_OF_DIRS    10

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK           0   /**< Dark room, light needed to see */
#define ROOM_DEATH          1   /**< Death trap, instant death */
#define ROOM_NOMOB          2   /**< MOBs not allowed in room */
#define ROOM_INDOORS        3   /**< Indoors, no weather */
#define ROOM_PEACEFUL       4   /**< Violence not allowed	*/
#define ROOM_SOUNDPROOF     5   /**< Shouts, gossip blocked */
#define ROOM_NOTRACK        6   /**< Track won't go through */
#define ROOM_NOMAGIC        7   /**< Magic not allowed */
#define ROOM_TUNNEL         8   /**< Room for only 1 pers	*/
#define ROOM_PRIVATE        9   /**< Can't teleport in */
#define ROOM_STAFFROOM     10   /**< LVL_STAFF+ only allowed */
#define ROOM_HOUSE         11   /**< (R) Room is a house */
#define ROOM_HOUSE_CRASH   12   /**< (R) House needs saving */
#define ROOM_ATRIUM        13   /**< (R) The door to a house */
#define ROOM_OLC           14   /**< (R) Modifyable/!compress */
#define ROOM_BFS_MARK      15   /**< (R) breath-first srch mrk */
#define ROOM_WORLDMAP      16   /**< World-map style maps here */
#define ROOM_REGEN         17  /* regen room */
#define ROOM_FLY_NEEDED    18  /* will drop without fly */
#define ROOM_NORECALL      19  /* no recalling from/to this room */
#define ROOM_SINGLEFILE    20  /* very narrow room */
#define ROOM_NOTELEPORT    21  /* no teleportin from/to this room */
#define ROOM_MAGICDARK     22  /* pitch black, not lightable */
#define ROOM_MAGICLIGHT    23  /* lit */
#define ROOM_NOSUMMON      24  /* no summoning from/to this room */
#define ROOM_NOHEAL        25  /* all regen stops in this room */
#define ROOM_NOFLY         26  /* can't fly in this room */
#define ROOM_FOG           27  /* fogged (hamper vision/stops daylight) */
#define ROOM_AIRY          28  /* airy (breathe underwater) */
/** The total number of Room Flags */
#define NUM_ROOM_FLAGS     29

// Room affects
#define RAFF_FOG            (1 << 0)
#define RAFF_DARKNESS       (1 << 1)
#define RAFF_LIGHT          (1 << 2)
#define RAFF_STINK          (1 << 3)
#define RAFF_BILLOWING      (1 << 4)
#define RAFF_ANTI_MAGIC     (1 << 5)
#define RAFF_ACID_FOG       (1 << 6)
#define RAFF_BLADE_BARRIER  (1 << 7)
#define RAFF_SPIKE_GROWTH   (1 << 8)
#define RAFF_SPIKE_STONES   (1 << 9)
#define RAFF_HOLY           (1 << 10)
#define RAFF_UNHOLY         (1 << 11)
#define NUM_RAFF            12

/* Zone info: Used in zone_data.zone_flags */
#define ZONE_CLOSED         0  /**< Zone is closed - players cannot enter */
#define ZONE_NOIMMORT       1  /**< Immortals (below LVL_GRSTAFF) cannot enter this zone */
#define ZONE_QUEST          2  /**< This zone is a quest zone (not implemented) */
#define ZONE_GRID           3  /**< Zone is 'on the grid', connected, show on 'areas' */
#define ZONE_NOBUILD        4  /**< Building is not allowed in the zone */
#define ZONE_NOASTRAL       5  /**< No teleportation magic will work to or from this zone */
#define ZONE_WORLDMAP       6 /**< Whole zone uses the WORLDMAP by default */
#define ZONE_NOCLAIM        7 /**< Zone can't be claimed, or popularity changed */
#define ZONE_ASTRAL_PLANE   8 /* astral plane */
#define ZONE_ETH_PLANE      9 /* ethereal plane */
#define ZONE_ELEMENTAL      10 /* elemental plane */
/** The total number of Zone Flags */
#define NUM_ZONE_FLAGS      11

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR    (1 << 0) /**< Exit is a door */
#define EX_CLOSED    (1 << 1) /**< The door is closed */
#define EX_LOCKED    (1 << 2) /**< The door is locked */
#define EX_PICKPROOF (1 << 3) /**< Lock can't be picked */
#define EX_HIDDEN    (1 << 4) /**< Exit is hidden, secret */
#define NUM_EXIT_BITS  5

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		/**< Indoors, connected to SECT macro. */
#define SECT_CITY            1		/**< In a city			*/
#define SECT_FIELD           2		/**< In a field		*/
#define SECT_FOREST          3		/**< In a forest		*/
#define SECT_HILLS           4		/**< In the hills		*/
#define SECT_MOUNTAIN        5		/**< On a mountain		*/
#define SECT_WATER_SWIM      6		/**< Swimmable water		*/
#define SECT_WATER_NOSWIM    7		/**< Water - need a boat	*/
#define SECT_FLYING	         8		/**< Flying			*/
#define SECT_UNDERWATER	    9		/**< Underwater		*/
#define SECT_ZONE_START	    10		// zone start (for asciimap)
#define SECT_ROAD_NS         11	// road runing north-south
#define SECT_ROAD_EW         12	// road running east-north
#define SECT_ROAD_INT        13	// road intersection
#define SECT_DESERT          14	// desert
#define SECT_OCEAN           15
#define SECT_MARSHLAND       16
#define SECT_HIGH_MOUNTAIN   17
#define SECT_PLANES          18
/** The total number of room Sector Types */
#define NUM_ROOM_SECTORS     19

/* char and mob-related defines */

/* History */
#define HIST_ALL       0 /**< Index to history of all channels */
#define HIST_SAY       1 /**< Index to history of all 'say' */
#define HIST_GOSSIP    2 /**< Index to history of all 'gossip' */
#define HIST_WIZNET    3 /**< Index to history of all 'wiznet' */
#define HIST_TELL      4 /**< Index to history of all 'tell' */
#define HIST_SHOUT     5 /**< Index to history of all 'shout' */
#define HIST_GRATS     6 /**< Index to history of all 'grats' */
#define HIST_HOLLER    7 /**< Index to history of all 'holler' */
#define HIST_AUCTION   8 /**< Index to history of all 'auction' */
#define HIST_CLANTALK  9 /**< Index to history of all 'clantalk' */
/**/
#define NUM_HIST       10 /**< Total number of history indexes */
#define HISTORY_SIZE   5 /**< Number of last commands kept in each history */

/* Group Defines */
#define GROUP_OPEN    (1 << 0)  /**< Group is open for members */
#define GROUP_ANON    (1 << 1)  /**< Group is Anonymous */
#define GROUP_NPC     (1 << 2)  /**< Group created by NPC and thus not listed */


// size definitions, based on DnD3.5
#define SIZE_UNDEFINED	  (-1)
#define SIZE_RESERVED      0
#define SIZE_FINE          1
#define SIZE_DIMINUTIVE    2
#define SIZE_TINY          3
#define SIZE_SMALL         4
#define SIZE_MEDIUM        5
#define SIZE_LARGE         6
#define SIZE_HUGE          7
#define SIZE_GARGANTUAN    8
#define SIZE_COLOSSAL      9
/* ** */
#define NUM_SIZES          10


/* this sytem is built on top of stock alignment
 * which is a value between -1000 to 1000
 * alignments */
#define LAWFUL_GOOD         0
#define NEUTRAL_GOOD        1
#define CHAOTIC_GOOD        2
#define LAWFUL_NEUTRAL      3
#define TRUE_NEUTRAL        4
#define CHAOTIC_NEUTRAL     5
#define LAWFUL_EVIL         6
#define NEUTRAL_EVIL        7
#define CHAOTIC_EVIL        8
/***/
#define NUM_ALIGNMENTS      9
/***/


/* PC classes */
#define CLASS_UNDEFINED	 (-1) /**< PC Class undefined */
#define CLASS_WIZARD      0    /**< PC Class wizard */
#define CLASS_CLERIC      1    /**< PC Class Cleric */
#define CLASS_ROGUE       2    /**< PC Class Rogue (former Thief) */
#define CLASS_WARRIOR     3    /**< PC Class Warrior */
#define CLASS_MONK	      4    /**< PC Class monk */
#define CLASS_DRUID	      5	//druids
#define CLASS_BERSERKER	 6	//berserker
#define CLASS_SORCERER    7
#define CLASS_PALADIN     8
#define CLASS_RANGER      9
#define CLASS_BARD        10
/** Total number of available PC Classes */
#define NUM_CLASSES       11

// related to pc (classes, etc)
/* note that max_classes was established to reign in some of the
   pfile arrays associated with classes */
#define MAX_CLASSES	30	// total number of maximum pc classes
#define NUM_CASTERS	7	//direct reference to pray array
/*  x wizard
 *  x sorcerer
 *  x cleric
 *  x druid
 *  x bard
 *  x paladin
 *  x ranger
 * ****  load_prayX has to be changed in players.c manually for this ****
 */
/**************************/

// warding spells that need to be saved
#define MIRROR			0
#define STONESKIN		1
/*---------*/
#define NUM_WARDING		2
#define MAX_WARDING		10	// "warding" type spells such as stoneskin that save

/* at the beginning, spec_abil was an array reserved for daily resets
   considering we've converted most of our system to a cooldown system
   we have abandoned that primary purpose and converted her to an array
   of easy to use reserved values in the pfile that saves for special
   ability info we need */
#define SPELL_MANTLE            0 // spell mantle left
#define INCEND                  1 // incendiary cloud
#define SONG_AFF                2 // how much to modify skill with song-affects
#define CALLCOMPANION           3 // animal companion vnum
#define CALLFAMILIAR            4 // familiars vnum
#define SORC_KNOWN              5 // true/false if can 'study'
#define RANG_KNOWN              6 // true/false if can 'study'
#define CALLMOUNT               7 // paladin mount vnum
#define WIZ_KNOWN               8 // true/false if can 'study'
#define BARD_KNOWN              9 // true/false if can 'study'
#define SHAPECHANGES           10 // druid shapechanges left today
#define C_DOOM                 11 // creeping doom
#define DRUID_KNOWN            12 // true/false if can 'study'
#define AG_SPELLBATTLE         13 // arg for spellbattle racial
/*---------------*/
#define NUM_SPEC_ABIL		 14
#define MAX_SPEC_ABIL          MAX_CLASSES
/* max = MAX_CLASSES right now */

/* max enemies, reserved space for array of ranger's favored enemies */
#define MAX_ENEMIES  10


// Memorization
#define NUM_SLOTS	20  //theoretical max num slots per circle
#define NUM_CIRCLES	10  //max num circles
/* how much space to reserve in the mem arrays */
#define MAX_MEM		NUM_SLOTS * NUM_CIRCLES

// Races
#define RACE_UNDEFINED    (-1) /*Race Undefined*/
#define RACE_HUMAN           0 /* Race Human */
#define RACE_ELF             1 /* Race Elf   */
#define RACE_DWARF           2 /* Race Dwarf */
#define RACE_TROLL           3 /* Race Troll (advanced) */
#define RACE_CRYSTAL_DWARF   4  /* crystal dwarf (epic) */
#define RACE_HALFLING        5  // halfling
#define RACE_H_ELF           6  // half elf
#define RACE_H_ORC           7  // half orc
#define RACE_GNOME           8  // gnome
#define RACE_TRELUX          9  // trelux (epic)
#define RACE_ARCANA_GOLEM   10  // arcana golem (advanced)
/* Total Number of available PC Races*/
#define NUM_RACES          11


// NPC races
#define NPCRACE_UNDEFINED	(-1)	/*Race Undefined*/
#define NPCRACE_UNKNOWN       0
#define NPCRACE_HUMAN         1
#define NPCRACE_UNDEAD        2
#define NPCRACE_ANIMAL        3
#define NPCRACE_DRAGON		4
#define NPCRACE_GIANT	     5
#define NPCRACE_ABERRATION    6
#define NPCRACE_CONSTRUCT     7
#define NPCRACE_ELEMENTAL     8
#define NPCRACE_FEY           9
#define NPCRACE_MAG_BEAST     10 // magical beast
#define NPCRACE_MONSTER_HMN	11 // monsterous humanoid
#define NPCRACE_OOZE          12
#define NPCRACE_OUTSIDER      13
#define NPCRACE_PLANT         14
#define NPCRACE_VERMIN        15
//total
#define NUM_NPC_RACES		16
/* how many subrace-types can a mobile have? */
/* note, if this is changed, a lot of other places have
 * to be changed as well -zusuk */
#define MAX_SUBRACES          3

// npc sub-race types
#define SUBRACE_UNDEFINED	  (-1)	/*Race Undefined*/
#define SUBRACE_UNKNOWN          0
#define SUBRACE_AIR              1
#define SUBRACE_ANGEL            2
#define SUBRACE_AQUATIC          3
#define SUBRACE_ARCHON		   4
#define SUBRACE_AUGMENTED	   5
#define SUBRACE_CHAOTIC          6
#define SUBRACE_COLD             7
#define SUBRACE_EARTH            8
#define SUBRACE_EVIL             9
#define SUBRACE_EXTRAPLANAR      10
#define SUBRACE_FIRE	        11
#define SUBRACE_GOBLINOID        12
#define SUBRACE_GOOD             13
#define SUBRACE_INCORPOREAL      14
#define SUBRACE_LAWFUL           15
#define SUBRACE_NATIVE           16
#define SUBRACE_REPTILIAN        17
#define SUBRACE_SHAPECHANGER     18
#define SUBRACE_SWARM            19
#define SUBRACE_WATER            20
//total
#define NUM_SUB_RACES		   21

// pc sub-race types, so far used for druid shapechange
#define PC_SUBRACE_UNDEFINED       (-1)	/*Race Undefined*/
#define PC_SUBRACE_UNKNOWN          0
#define PC_SUBRACE_BADGER           1
#define PC_SUBRACE_PANTHER          2
#define PC_SUBRACE_BEAR             3
#define PC_SUBRACE_G_CROCODILE      4
//total
#define MAX_PC_SUBRACES	           5


/* Sex */
#define SEX_NEUTRAL   0   /**< Neutral Sex (Hermaphrodite) */
#define SEX_MALE      1   /**< Male Sex (XY Chromosome) */
#define SEX_FEMALE    2   /**< Female Sex (XX Chromosome) */
/** Total number of Genders */
#define NUM_GENDERS   3


/* Positions */
#define POS_DEAD       0	/**< Position = dead */
#define POS_MORTALLYW  1	/**< Position = mortally wounded */
#define POS_INCAP      2	/**< Position = incapacitated */
#define POS_STUNNED    3	/**< Position = stunned	*/
#define POS_SLEEPING   4	/**< Position = sleeping */
#define POS_RESTING    5	/**< Position = resting	*/
#define POS_SITTING    6	/**< Position = sitting	*/
#define POS_FIGHTING   7	/**< Position = fighting */
#define POS_STANDING   8	/**< Position = standing */
/** Total number of positions. */
#define NUM_POSITIONS   9

/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER        0   /**< Player is a player-killer */
#define PLR_THIEF         1   /**< Player is a player-thief */
#define PLR_FROZEN        2   /**< Player is frozen */
#define PLR_DONTSET       3   /**< Don't EVER set (ISNPC bit, set by mud) */
#define PLR_WRITING       4   /**< Player writing (board/mail/olc) */
#define PLR_MAILING       5   /**< Player is writing mail */
#define PLR_CRASH         6   /**< Player needs to be crash-saved */
#define PLR_SITEOK        7   /**< Player has been site-cleared */
#define PLR_NOSHOUT       8   /**< Player not allowed to shout/goss */
#define PLR_NOTITLE       9   /**< Player not allowed to set title */
#define PLR_DELETED      10   /**< Player deleted - space reusable */
#define PLR_LOADROOM     11   /**< Player uses nonstandard loadroom */
#define PLR_NOWIZLIST    12   /**< Player shouldn't be on wizlist */
#define PLR_NODELETE     13   /**< Player shouldn't be deleted */
#define PLR_INVSTART     14   /**< Player should enter game wizinvis */
#define PLR_CRYO         15   /**< Player is cryo-saved (purge prog) */
#define PLR_NOTDEADYET   16   /**< (R) Player being extracted */
#define PLR_BUG          17   /**< Player is writing a bug */
#define PLR_IDEA         18   /**< Player is writing an idea */
#define PLR_TYPO         19   /**< Player is writing a typo */
#define PLR_SALVATION    20   /* for salvation cleric spell */
/***************/
#define NUM_PLR_BITS	21


/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC            0   /**< Mob has a callable spec-proc */
#define MOB_SENTINEL        1   /**< Mob should not move */
#define MOB_SCAVENGER       2   /**< Mob picks up stuff on the ground */
#define MOB_ISNPC           3   /**< (R) Automatically set on all Mobs */
#define MOB_AWARE           4   /**< Mob can't be backstabbed */
#define MOB_AGGRESSIVE      5   /**< Mob auto-attacks everybody nearby */
#define MOB_STAY_ZONE       6   /**< Mob shouldn't wander out of zone */
#define MOB_WIMPY           7   /**< Mob flees if severely injured */
#define MOB_AGGR_EVIL       8   /**< Auto-attack any evil PC's */
#define MOB_AGGR_GOOD       9   /**< Auto-attack any good PC's */
#define MOB_AGGR_NEUTRAL   10   /**< Auto-attack any neutral PC's */
#define MOB_MEMORY         11   /**< remember attackers if attacked */
#define MOB_HELPER         12   /**< attack PCs fighting other NPCs */
#define MOB_NOCHARM        13   /**< Mob can't be charmed */
#define MOB_NOSUMMON       14   /**< Mob can't be summoned */
#define MOB_NOSLEEP        15   /**< Mob can't be slept */
#define MOB_NOBASH         16   /**< Mob can't be bashed (e.g. trees) */
#define MOB_NOBLIND        17   /**< Mob can't be blinded */
#define MOB_NOKILL         18   /**< Mob can't be attacked */
#define MOB_SENTIENT       19
#define MOB_NOTDEADYET     20   /**< (R) Mob being extracted */
#define MOB_MOUNTABLE      21
#define MOB_NODEAF         22
#define MOB_NOFIGHT        23
#define MOB_NOCLASS        24
#define MOB_NOGRAPPLE      25
#define MOB_C_ANIMAL       26
#define MOB_C_FAMILIAR     27
#define MOB_C_MOUNT        28
#define MOB_ELEMENTAL      29
#define MOB_ANIMATED_DEAD  30
/**********************/
#define NUM_MOB_FLAGS      31


/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF         0   /**< Room descs won't normally be shown */
#define PRF_COMPACT       1   /**< No extra CRLF pair before prompts */
#define PRF_NOSHOUT       2   /**< Can't hear shouts */
#define PRF_NOTELL        3   /**< Can't receive tells */
#define PRF_DISPHP        4   /**< Display hit points in prompt */
#define PRF_DISPMANA      5   /**< Display mana points in prompt */
#define PRF_DISPMOVE      6   /**< Display move points in prompt */
#define PRF_AUTOEXIT      7   /**< Display exits in a room */
#define PRF_NOHASSLE      8   /**< Aggr mobs won't attack */
#define PRF_QUEST         9   /**< On quest */
#define PRF_SUMMONABLE   10   /**< Can be summoned */
#define PRF_NOREPEAT     11   /**< No repetition of comm commands */
#define PRF_HOLYLIGHT    12   /**< Can see in dark */
#define PRF_COLOR_1      13   /**< Color (low bit) */
#define PRF_COLOR_2      14   /**< Color (high bit) */
#define PRF_NOWIZ        15   /**< Can't hear wizline */
#define PRF_LOG1         16   /**< On-line System Log (low bit) */
#define PRF_LOG2         17   /**< On-line System Log (high bit) */
#define PRF_NOAUCT       18   /**< Can't hear auction channel */
#define PRF_NOGOSS       19   /**< Can't hear gossip channel */
#define PRF_NOGRATZ      20   /**< Can't hear grats channel */
#define PRF_SHOWVNUMS    21   /**< Can see VNUMs */
#define PRF_DISPAUTO     22   /**< Show prompt HP, MP, MV when < 25% */
#define PRF_CLS          23   /**< Clear screen in OLC */
#define PRF_BUILDWALK    24   /**< Build new rooms while walking */
#define PRF_AFK          25   /**< AFK flag */
#define PRF_AUTOLOOT     26   /**< Loot everything from a corpse */
#define PRF_AUTOGOLD     27   /**< Loot gold from a corpse */
#define PRF_AUTOSPLIT    28   /**< Split gold with group */
#define PRF_AUTOSAC      29   /**< Sacrifice a corpse */
#define PRF_AUTOASSIST   30   /**< Auto-assist toggle */
#define PRF_AUTOMAP      31   /**< Show map at the side of room descs */
#define PRF_AUTOKEY      32   /**< Automatically unlock locked doors when opening */
#define PRF_AUTODOOR     33   /**< Use the next available door */
#define PRF_NOCLANTALK   34   /**< Don't show ALL clantalk channels (Imm-only) */
#define PRF_AUTOSCAN     35  // automatically scan each step?
#define PRF_DISPEXP      36  // autoprompt xp display
#define PRF_DISPEXITS    37  // autoprompt exits display
#define PRF_DISPROOM     38  // display room name and/or #
#define PRF_DISPMEMTIME  39  // display memtimes
/** Total number of available PRF flags */
#define NUM_PRF_FLAGS    40

/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_DONTUSE          0   /**< DON'T USE! */
#define AFF_BLIND            1   /**< (R) Char is blind */
#define AFF_INVISIBLE        2   /**< Char is invisible */
#define AFF_DETECT_ALIGN     3   /**< Char is sensitive to align */
#define AFF_DETECT_INVIS     4   /**< Char can see invis chars */
#define AFF_DETECT_MAGIC     5   /**< Char is sensitive to magic */
#define AFF_SENSE_LIFE       6   /**< Char can sense hidden life */
#define AFF_WATERWALK        7   /**< Char can walk on water */
#define AFF_SANCTUARY        8   /**< Char protected by sanct */
#define AFF_GROUP            9   /**< (R) Char is grouped */
#define AFF_CURSE            10   /**< Char is cursed */
#define AFF_INFRAVISION      11   /**< Char can kinda see in dark */
#define AFF_POISON           12   /**< (R) Char is poisoned */
#define AFF_PROTECT_EVIL     13   /**< Char protected from evil */
#define AFF_PROTECT_GOOD     14   /**< Char protected from good */
#define AFF_SLEEP            15   /**< (R) Char magically asleep */
#define AFF_NOTRACK          16   /**< Char can't be tracked */
#define AFF_FLYING           17   /**< Char is flying */
#define AFF_SCUBA            18   /**< Room for future expansion */
#define AFF_SNEAK            19   /**< Char can move quietly */
#define AFF_HIDE             20   /**< Char is hidden */
#define AFF_VAMPIRIC_CURSE   21   // hit victim heals attacker
#define AFF_CHARM            22   /**< Char is charmed */
#define AFF_BLUR             23	// char has blurry image
#define AFF_POWER_ATTACK     24	// power attack mode
#define AFF_EXPERTISE        25	// combat expertise mode
#define AFF_HASTE            26	// hasted
#define AFF_PARRY            27	// parry mode
#define AFF_ELEMENT_PROT     28	// endure elements, etc
#define AFF_DEAF             29	// deafened
#define AFF_FEAR             30	// under affect of fear
#define AFF_STUN             31	// stunned
#define AFF_PARALYZED        32	// paralyzed
#define AFF_ULTRAVISION      33   /**< Char can see in dark */
#define AFF_GRAPPLED         34   // grappled
#define AFF_TAMED            35   // tamed
#define AFF_CLIMB            36   // affect that allows you to climb
#define AFF_NAUSEATED        37   // nauseated
#define AFF_NON_DETECTION    38
#define AFF_SLOW             39
#define AFF_FSHIELD          40  //fire shield
#define AFF_CSHIELD          41  //cold shield
#define AFF_MINOR_GLOBE      42  //minor globe of invulernability
#define AFF_ASHIELD          43  //acid shield
#define AFF_SIZECHANGED      44
#define AFF_TRUE_SIGHT       45
#define AFF_SPOT             46
#define AFF_FATIGUED         47
#define AFF_REGEN            48
#define AFF_DISEASE          49
#define AFF_TFORM            50  //tenser's transformation
#define AFF_GLOBE_OF_INVULN  51
#define AFF_LISTEN           52
#define AFF_DISPLACE         53
#define AFF_SPELL_MANTLE     54
#define AFF_CONFUSED         55
#define AFF_REFUGE           56
#define AFF_SPELL_TURNING    57
#define AFF_MIND_BLANK       58
#define AFF_SHADOW_SHIELD    59
#define AFF_TIME_STOPPED     60
#define AFF_BRAVERY          61
#define AFF_FREE_MOVEMENT    62
#define AFF_FAERIE_FIRE      63
#define AFF_BATTLETIDE       64
#define AFF_SPELL_RESISTANT  65
#define AFF_DIM_LOCK         66 //locked to current plane (can't teleport)
#define AFF_DEATH_WARD       67
#define AFF_SPELLBATTLE      68
/* works in progress -zusuk (don't forget vampiric-curse (21) */
#define AFF_VAMPIRIC_TOUCH   69  // will make next attack vampiric
#define AFF_BLACKMANTLE      70  // stop normal regen, reduce healing
#define AFF_DANGERSENSE      71  // sense aggro in surround rooms
#define AFF_SAFEFALL         72  // reduce damage from falling
#define AFF_TOWER_OF_IRON_WILL 73  // reduce psionic damage
#define AFF_INERTIAL_BARRIER 74  // absorb damage based on mana
#define AFF_NOTELEPORT       75  // make target not reachable via teleport
#define AFF_MAX_DAMAGE       76  // enhance next attack/spell/etc

/** Total number of affect flags not including the don't use flag. */
// zusuk, don't forget to add to constants.c!
#define NUM_AFF_FLAGS        77

/* Modes of connectedness: used by descriptor_data.state 		*/
#define CON_PLAYING       0 /**< Playing - Nominal state 		*/
#define CON_CLOSE         1 /**< User disconnect, remove character.	*/
#define CON_GET_NAME      2 /**< Login with name */
#define CON_NAME_CNFRM    3 /**< New character, confirm name */
#define CON_PASSWORD      4 /**< Login with password */
#define CON_NEWPASSWD     5 /**< New character, create password */
#define CON_CNFPASSWD     6 /**< New character, confirm password */
#define CON_QSEX          7 /**< Choose character sex */
#define CON_QCLASS        8 /**< Choose character class */
#define CON_RMOTD         9 /**< Reading the message of the day */
#define CON_MENU         10 /**< At the main menu */
#define CON_PLR_DESC     11 /**< Enter a new character description prompt */
#define CON_CHPWD_GETOLD 12 /**< Changing passwd: Get old		*/
#define CON_CHPWD_GETNEW 13 /**< Changing passwd: Get new */
#define CON_CHPWD_VRFY   14 /**< Changing passwd: Verify new password */
#define CON_DELCNF1      15 /**< Character Delete: Confirmation 1		*/
#define CON_DELCNF2      16 /**< Character Delete: Confirmation 2		*/
#define CON_DISCONNECT   17 /**< In-game link loss (leave character)	*/
#define CON_OEDIT        18 /**< OLC mode - object editor		*/
#define CON_REDIT        19 /**< OLC mode - room editor		*/
#define CON_ZEDIT        20 /**< OLC mode - zone info editor		*/
#define CON_MEDIT        21 /**< OLC mode - mobile editor		*/
#define CON_SEDIT        22 /**< OLC mode - shop editor		*/
#define CON_TEDIT        23 /**< OLC mode - text editor		*/
#define CON_CEDIT        24 /**< OLC mode - conf editor		*/
#define CON_AEDIT        25 /**< OLC mode - social (action) edit      */
#define CON_TRIGEDIT     26 /**< OLC mode - trigger edit              */
#define CON_HEDIT        27 /**< OLC mode - help edit */
#define CON_QEDIT        28 /**< OLC mode - quest edit */
#define CON_PREFEDIT     29 /**< OLC mode - preference edit */
#define CON_IBTEDIT      30 /**< OLC mode - idea/bug/typo edit */
#define CON_GET_PROTOCOL 31 /**< Used at log-in while attempting to get protocols > */
#define CON_QRACE        32 /* Choose character race*/
#define CON_CLANEDIT     33 /** OLC mode - clan edit */
#define CON_MSGEDIT      34 /**< OLC mode - message editor */
#define CON_STUDY        35 /**< OLC mode - sorc-spells-known editor */
#define CON_QCLASS_HELP  36 /* help info during char creation */
#define CON_QALIGN       37 /* alignment selection in char creation */
#define CON_QRACE_HELP   38 /* help info (race) during char creation */
#define CON_HLQEDIT      39 /* homeland-port quest editor */
/* OLC States range - used by IS_IN_OLC and IS_PLAYING */
#define FIRST_OLC_STATE CON_OEDIT     /**< The first CON_ state that is an OLC */
#define LAST_OLC_STATE  CON_HLQEDIT    /**< The last CON_ state that is an OLC  */
#define NUM_CON_STATES	40

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
 which control the valid places you can wear a piece of equipment.
 For example, there are two neck positions on the player, and items
 only get the generic neck type. */
#define WEAR_LIGHT      0  /**< Equipment Location Light */
#define WEAR_FINGER_R   1  /**< Equipment Location Right Finger */
#define WEAR_FINGER_L   2  /**< Equipment Location Left Finger */
#define WEAR_NECK_1     3  /**< Equipment Location Neck #1 */
#define WEAR_NECK_2     4  /**< Equipment Location Neck #2 */
#define WEAR_BODY       5  /**< Equipment Location Body */
#define WEAR_HEAD       6  /**< Equipment Location Head */
#define WEAR_LEGS       7  /**< Equipment Location Legs */
#define WEAR_FEET       8  /**< Equipment Location Feet */
#define WEAR_HANDS      9  /**< Equipment Location Hands */
#define WEAR_ARMS      10  /**< Equipment Location Arms */
#define WEAR_SHIELD    11  /**< Equipment Location Shield */
#define WEAR_ABOUT     12  /**< Equipment Location about body (like a cape)*/
#define WEAR_WAIST     13  /**< Equipment Location Waist */
#define WEAR_WRIST_R   14  /**< Equipment Location Right Wrist */
#define WEAR_WRIST_L   15  /**< Equipment Location Left Wrist */
#define WEAR_WIELD_1   16  /**< Equipment Location Weapon */
#define WEAR_HOLD_1	   17  /**< Equipment Location held in offhand */
#define WEAR_WIELD_2   18	// off-hand weapon
#define WEAR_HOLD_2    19	// off-hand held
#define WEAR_WIELD_2H  20	// two-hand weapons
#define WEAR_HOLD_2H   21	// two-hand held
#define WEAR_FACE       22      // equipment location face
/** Total number of available equipment lcoations */
#define NUM_WEARS      23

/* object-related defines */
/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT      1		/**< Item is a light source	*/
#define ITEM_SCROLL     2		/**< Item is a scroll		*/
#define ITEM_WAND       3		/**< Item is a wand		*/
#define ITEM_STAFF      4		/**< Item is a staff		*/
#define ITEM_WEAPON     5		/**< Item is a weapon		*/
#define ITEM_FURNITURE  6   /**< Sittable Furniture		*/
#define ITEM_FREE       7   /**< Unimplemented		*/
#define ITEM_TREASURE   8   /**< Item is a treasure, not gold	*/
#define ITEM_ARMOR      9   /**< Item is armor		*/
#define ITEM_POTION    10   /**< Item is a potion		*/
#define ITEM_WORN      11		/**< Unimplemented		*/
#define ITEM_OTHER     12		/**< Misc object			*/
#define ITEM_TRASH     13		/**< Trash - shopkeepers won't buy	*/
#define ITEM_FREE2     14		/**< Unimplemented		*/
#define ITEM_CONTAINER 15		/**< Item is a container		*/
#define ITEM_NOTE      16		/**< Item is note 		*/
#define ITEM_DRINKCON  17		/**< Item is a drink container	*/
#define ITEM_KEY       18		/**< Item is a key		*/
#define ITEM_FOOD      19		/**< Item is food			*/
#define ITEM_MONEY     20		/**< Item is money (gold)		*/
#define ITEM_PEN       21		/**< Item is a pen		*/
#define ITEM_BOAT      22		/**< Item is a boat		*/
#define ITEM_FOUNTAIN  23		/**< Item is a fountain		*/
#define ITEM_CLANARMOR 24		/**< Item is clan armor		*/
#define ITEM_CRYSTAL   25  //crafting
#define ITEM_ESSENCE   26  //crafting
#define ITEM_MATERIAL  27  //crafting / general
#define ITEM_SPELLBOOK 28
#define ITEM_PORTAL    29
#define ITEM_PLANT     30 /* for transport via plants spell */
/** Total number of item types.*/
#define NUM_ITEM_TYPES 31

/* Item profs: used by obj_data.obj_flags.prof_flag constants.c = item_profs */
#define ITEM_PROF_NONE		0	// no proficiency required
#define ITEM_PROF_MINIMAL	1	//  "Minimal Weapon Proficiency"
#define ITEM_PROF_BASIC		2	//  "Basic Weapon Proficiency"
#define ITEM_PROF_ADVANCED	3	//  "Advanced Weapon Proficiency"
#define ITEM_PROF_MASTER 	4	//  "Master Weapon Proficiency"
#define ITEM_PROF_EXOTIC 	5	//  "Exotic Weapon Proficiency"
#define ITEM_PROF_LIGHT_A	6	// light armor prof
#define ITEM_PROF_MEDIUM_A	7	// medium armor prof
#define ITEM_PROF_HEAVY_A	8	// heavy armor prof
#define ITEM_PROF_SHIELDS	9	// shield prof
#define ITEM_PROF_T_SHIELDS	10	// tower shield prof
/** Total number of item profs.*/
#define NUM_ITEM_PROFS        11

/* Item profs: used by obj_data.obj_flags.material 
 * constants.c = material_name 
 */
#define MATERIAL_UNDEFINED       0 
#define MATERIAL_COTTON          1 
#define MATERIAL_LEATHER         2
#define MATERIAL_GLASS           3
#define MATERIAL_GOLD            4
#define MATERIAL_ORGANIC         5
#define MATERIAL_PAPER           6
#define MATERIAL_STEEL           7
#define MATERIAL_WOOD            8
#define MATERIAL_BONE            9
#define MATERIAL_CRYSTAL         10
#define MATERIAL_ETHER           11
#define MATERIAL_ADAMANTINE      12
#define MATERIAL_MITHRIL         13
#define MATERIAL_IRON            14
#define MATERIAL_COPPER          15
#define MATERIAL_CERAMIC         16
#define MATERIAL_SATIN           17
#define MATERIAL_SILK            18
#define MATERIAL_DRAGONHIDE      19
#define MATERIAL_BURLAP          20
#define MATERIAL_VELVET          21
#define MATERIAL_PLATINUM        22
#define MATERIAL_OBSIDIAN        23
#define MATERIAL_WOOL            24
#define MATERIAL_ONYX            25
#define MATERIAL_IVORY           26
#define MATERIAL_BRASS           27
#define MATERIAL_MARBLE          28
#define MATERIAL_BRONZE          29
#define MATERIAL_PEWTER          30
#define MATERIAL_RUBY            31
#define MATERIAL_SAPPHIRE        32
#define MATERIAL_EMERALD         33
#define MATERIAL_GEMSTONE        34
#define MATERIAL_GRANITE         35
#define MATERIAL_STONE           36
#define MATERIAL_ENERGY          37
#define MATERIAL_HEMP            38
#define MATERIAL_DIAMOND         39
#define MATERIAL_EARTH           40  
#define MATERIAL_SILVER          41
#define MATERIAL_ALCHEMAL_SILVER 42
#define MATERIAL_COLD_IRON       43
#define MATERIAL_DARKWOOD        44  
/** Total number of item mats.*/
#define NUM_MATERIALS            45


/* Portal types for the portal object */
#define PORTAL_NORMAL     0 
#define PORTAL_RANDOM     1 
#define PORTAL_CHECKFLAGS 2 
#define PORTAL_CLANHALL   3 
/****/
#define NUM_PORTAL_TYPES  4 


/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE      0   /**< Item can be taken */
#define ITEM_WEAR_FINGER    1   /**< Item can be worn on finger */
#define ITEM_WEAR_NECK      2   /**< Item can be worn around neck */
#define ITEM_WEAR_BODY      3   /**< Item can be worn on body */
#define ITEM_WEAR_HEAD      4   /**< Item can be worn on head */
#define ITEM_WEAR_LEGS      5   /**< Item can be worn on legs */
#define ITEM_WEAR_FEET      6   /**< Item can be worn on feet */
#define ITEM_WEAR_HANDS	   7   /**< Item can be worn on hands	*/
#define ITEM_WEAR_ARMS      8   /**< Item can be worn on arms */
#define ITEM_WEAR_SHIELD    9   /**< Item can be used as a shield */
#define ITEM_WEAR_ABOUT	   10   /**< Item can be worn about body */
#define ITEM_WEAR_WAIST     11   /**< Item can be worn around waist */
#define ITEM_WEAR_WRIST	   12   /**< Item can be worn on wrist */
#define ITEM_WEAR_WIELD	   13   /**< Item can be wielded */
#define ITEM_WEAR_HOLD      14   /**< Item can be held */
#define ITEM_WEAR_FACE          15 // item can be worn on face
/** Total number of item wears */
#define NUM_ITEM_WEARS      16

/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW                 0   /**< Item is glowing */
#define ITEM_HUM                  1   /**< Item is humming */
#define ITEM_NORENT               2   /**< Item cannot be rented */
#define ITEM_NODONATE             3   /**< Item cannot be donated */
#define ITEM_NOINVIS              4   /**< Item cannot be made invis	*/
#define ITEM_INVISIBLE            5   /**< Item is invisible */
#define ITEM_MAGIC                6   /**< Item is magical */
#define ITEM_NODROP               7   /**< Item is cursed: can't drop */
#define ITEM_BLESS                8   /**< Item is blessed */
#define ITEM_ANTI_GOOD            9   /**< Not usable by good people	*/
#define ITEM_ANTI_EVIL            10   /**< Not usable by evil people	*/
#define ITEM_ANTI_NEUTRAL         11   /**< Not usable by neutral people */
#define ITEM_ANTI_WIZARD          12   /**< Not usable by wizards */
#define ITEM_ANTI_CLERIC          13   /**< Not usable by clerics */
#define ITEM_ANTI_ROGUE	         14   /**< Not usable by rogues */
#define ITEM_ANTI_WARRIOR         15   /**< Not usable by warriors */
#define ITEM_NOSELL               16   /**< Shopkeepers won't touch it */
#define ITEM_QUEST                17   /**< Item is a quest item         */
#define ITEM_ANTI_HUMAN           18   /* Not usable by Humans*/
#define ITEM_ANTI_ELF             19   /* Not usable by Elfs */
#define ITEM_ANTI_DWARF           20   /* Not usable by Dwarf*/
#define ITEM_ANTI_TROLL           21   /* Not usable by Troll */
#define ITEM_ANTI_MONK		    22   /**< Not usable by monks */
#define ITEM_ANTI_DRUID		    23   // not usable by druid
#define ITEM_MOLD                 24
#define ITEM_ANTI_CRYSTAL_DWARF   25   /* Not usable by C Dwarf*/
#define ITEM_ANTI_HALFLING        26   /* Not usable by halfling*/
#define ITEM_ANTI_H_ELF           27   /* Not usable by half elf*/
#define ITEM_ANTI_H_ORC           28   /* Not usable by half orc*/
#define ITEM_ANTI_GNOME           29   /* Not usable by gnome */
#define ITEM_ANTI_BERSERKER       30   /* Not usable by berserker */
#define ITEM_ANTI_TRELUX          31   /* Not usable by trelux */
#define ITEM_ANTI_SORCERER        32
#define ITEM_DECAY                33  /* portal decay */
#define ITEM_ANTI_PALADIN         34
#define ITEM_ANTI_RANGER          35
#define ITEM_ANTI_BARD            36
#define ITEM_ANTI_ARCANA_GOLEM    37
#define ITEM_FLOAT                38
/** Total number of item flags */
#define NUM_ITEM_FLAGS            39


/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/**< No effect			*/
#define APPLY_STR               1	/**< Apply to strength		*/
#define APPLY_DEX               2	/**< Apply to dexterity		*/
#define APPLY_INT               3	/**< Apply to intelligence	*/
#define APPLY_WIS               4	/**< Apply to wisdom		*/
#define APPLY_CON               5	/**< Apply to constitution	*/
#define APPLY_CHA               6 /**< Apply to charisma		*/
#define APPLY_CLASS             7	/**< Reserved			*/
#define APPLY_LEVEL             8	/**< Reserved			*/
#define APPLY_AGE               9	/**< Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/**< Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/**< Apply to height		*/
#define APPLY_MANA             12	/**< Apply to max mana		*/
#define APPLY_HIT              13	/**< Apply to max hit points	*/
#define APPLY_MOVE             14	/**< Apply to max move points	*/
#define APPLY_GOLD             15	/**< Reserved			*/
#define APPLY_EXP              16	/**< Reserved			*/
#define APPLY_AC               17	/**< Apply to Armor Class		*/
#define APPLY_HITROLL          18	/**< Apply to hitroll		*/
#define APPLY_DAMROLL          19	/**< Apply to damage roll		*/
#define APPLY_SAVING_FORT      20	// save fortitude
#define APPLY_SAVING_REFL      21	// safe reflex
#define APPLY_SAVING_WILL      22	// safe will power
#define APPLY_SAVING_POISON    23	// save poison
#define APPLY_SAVING_DEATH     24	// save death
#define APPLY_SPELL_RES        25	// spell resistance
#define APPLY_SIZE             26	// char size
#define APPLY_AC_NEW           27      // apply to armor class (post conversion)
/* dam_types (resistances/vulnerabilties) */
#define APPLY_RES_FIRE         28  //1        
#define APPLY_RES_COLD         29
#define APPLY_RES_AIR          30
#define APPLY_RES_EARTH        31
#define APPLY_RES_ACID         32  //5
#define APPLY_RES_HOLY         33
#define APPLY_RES_ELECTRIC     34
#define APPLY_RES_UNHOLY       35
#define APPLY_RES_SLICE        36
#define APPLY_RES_PUNCTURE     37  //10
#define APPLY_RES_FORCE        38
#define APPLY_RES_SOUND        39
#define APPLY_RES_POISON       40
#define APPLY_RES_DISEASE      41
#define APPLY_RES_NEGATIVE     42  //15
#define APPLY_RES_ILLUSION     43
#define APPLY_RES_MENTAL       44
#define APPLY_RES_LIGHT        45
#define APPLY_RES_ENERGY       46
#define APPLY_RES_WATER        47  //20
/* end dam_types, make sure it matches NUM_DAM_TYPES */

/** Total number of applies */
#define NUM_APPLIES            48


/* Equals the total number of SAVING_* defines in spells.h */
#define NUM_OF_SAVING_THROWS  5

/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/**< Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/**< Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/**< Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/**< Container is locked		*/
#define NUM_CONT_FLAGS		    4

/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0   /**< Liquid type water */
#define LIQ_BEER       1   /**< Liquid type beer */
#define LIQ_WINE       2   /**< Liquid type wine */
#define LIQ_ALE        3   /**< Liquid type ale */
#define LIQ_DARKALE    4   /**< Liquid type darkale */
#define LIQ_WHISKY     5   /**< Liquid type whisky */
#define LIQ_LEMONADE   6   /**< Liquid type lemonade */
#define LIQ_FIREBRT    7   /**< Liquid type firebrt */
#define LIQ_LOCALSPC   8   /**< Liquid type localspc */
#define LIQ_SLIME      9   /**< Liquid type slime */
#define LIQ_MILK       10  /**< Liquid type milk */
#define LIQ_TEA        11  /**< Liquid type tea */
#define LIQ_COFFE      12  /**< Liquid type coffee */
#define LIQ_BLOOD      13  /**< Liquid type blood */
#define LIQ_SALTWATER  14  /**< Liquid type saltwater */
#define LIQ_CLEARWATER 15  /**< Liquid type clearwater */
/** Total number of liquid types */
#define NUM_LIQ_TYPES     16


/* Player conditions */
#define DRUNK        0  /**< Player drunk condition */
#define HUNGER       1  /**< Player hunger condition */
#define THIRST       2  /**< Player thirst condition */

/* Sun state for weather_data */
#define SUN_DARK    0  /**< Night time */
#define SUN_RISE    1  /**< Dawn */
#define SUN_LIGHT   2  /**< Day time */
#define SUN_SET     3  /**< Dusk */

/* Sky conditions for weather_data */
#define SKY_CLOUDLESS  0  /**< Weather = No clouds */
#define SKY_CLOUDY     1  /**< Weather = Cloudy */
#define SKY_RAINING    2  /**< Weather = Rain */
#define SKY_LIGHTNING  3  /**< Weather = Lightning storm */

/* Rent codes */
#define RENT_UNDEF      0 /**< Character inv save status = undefined */
#define RENT_CRASH      1 /**< Character inv save status = game crash */
#define RENT_RENTED     2 /**< Character inv save status = rented */
#define RENT_CRYO       3 /**< Character inv save status = cryogenics */
#define RENT_FORCED     4 /**< Character inv save status = forced rent */
#define RENT_TIMEDOUT   5 /**< Character inv save status = timed out */

/* Settings for Bit Vectors */
#define RF_ARRAY_MAX    4  /**< # Bytes in Bit vector - Room flags */
#define PM_ARRAY_MAX    4  /**< # Bytes in Bit vector - Act and Player flags */
#define PR_ARRAY_MAX    4  /**< # Bytes in Bit vector - Player Pref Flags */
#define AF_ARRAY_MAX    4  /**< # Bytes in Bit vector - Affect flags */
#define TW_ARRAY_MAX    4  /**< # Bytes in Bit vector - Obj Wear Locations */
#define EF_ARRAY_MAX    4  /**< # Bytes in Bit vector - Obj Extra Flags */
#define ZN_ARRAY_MAX    4  /**< # Bytes in Bit vector - Zone Flags */

/* other #defined constants */
/* **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for details.
 *
 * LVL_IMPL should always be the HIGHEST possible immortal level, and
 * LVL_IMMORT should always be the LOWEST immortal level.  The number of
 * mortal levels will always be LVL_IMMORT - 1. */
#define LVL_IMPL    34  /**< Level of Implementors */
#define LVL_GRSTAFF   33  /**< Level of Greater Gods */
#define LVL_STAFF     32  /**< Level of Gods */
#define LVL_IMMORT	31  /**< Level of Immortals */

/** Minimum level to build and to run the saveall command */
#define LVL_BUILDER	LVL_IMMORT

/** Arbitrary number that won't be in a string */
#define MAGIC_NUMBER	(0x06)

/** OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 * This will equate to 10 passes per second.
 * @see PASSES_PER_SEC
 * @see RL_SEC
 */
#define OPT_USEC	100000
/** How many heartbeats equate to one real second.
 * @see OPT_USEC
 * @see RL_SEC
 */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
/** Used with other macros to define at how many heartbeats a control loop
 * gets executed. Helps to translate pulse counts to real seconds for
 * human comprehension.
 * @see PASSES_PER_SEC
 */
#define RL_SEC		* PASSES_PER_SEC

/** Controls when a zone update will occur. */
#define PULSE_ZONE      (10 RL_SEC)
/** Controls when mobile (NPC) actions and updates will occur. */
#define PULSE_MOBILE    (6 RL_SEC)
/** Controls the time between turns of combat. */
#define PULSE_VIOLENCE  ( 4 RL_SEC)

// controls some new luminari calls from comm.c
#define PULSE_LUMINARI  ( 5 RL_SEC)

/** Controls when characters and houses (if implemented) will be autosaved.
 * @see CONFIG_AUTO_SAVE
 */
#define PULSE_AUTOSAVE  (60 RL_SEC)
/** Controls when checks are made for idle name and password CON_ states */
#define PULSE_IDLEPWD   (15 RL_SEC)
/** Currently unused. */
#define PULSE_SANITY    (30 RL_SEC)
/** How often to log # connected sockets and # active players.
 * Currently set for 5 minutes.
 */
#define PULSE_USAGE     (5 * 60 RL_SEC)
/** Controls when to save the current ingame MUD time to disk.
 * This should be set >= SECS_PER_MUD_HOUR */
#define PULSE_TIMESAVE	(30 * 60 RL_SEC)
/* Variables for the output buffering system */
#define MAX_SOCK_BUF       (24 * 1024) /**< Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH  400         /**< Max length of prompt        */
#define GARBAGE_SPACE      32          /**< Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE      1024        /**< Static output buffer size   */
/** Max amount of output that can be buffered */
#define LARGE_BUFSIZE      (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

/* an arbitrary cap, medium in size */
#define MEDIUM_STRING         256

#define MAX_STRING_LENGTH     49152  /**< Max length of string, as defined */
#define MAX_INPUT_LENGTH      512    /**< Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH  (12 * 1024) /**< Max size of *raw* input */
#define MAX_MESSAGES          120     /**< Max Different attack message types */
#define MAX_NAME_LENGTH       20     /**< Max PC/NPC name length */
#define MAX_PWD_LENGTH        30     /**< Max PC password length */
#define MAX_TITLE_LENGTH      80     /**< Max PC title length */
#define HOST_LENGTH           40     /**< Max hostname resolution length */
#define PLR_DESC_LENGTH       4096   /**< Max length for PC description */
#define MAX_SKILLS            600    /**< Max number of skills/spells */
#define MAX_ABILITIES         200    /**< Max number of abilities */
#define MAX_AFFECT            32     /**< Max number of player affections */
#define MAX_OBJ_AFFECT        6      /**< Max object affects */
#define MAX_NOTE_LENGTH       4000   /**< Max length of text on a note obj */
#define MAX_LAST_ENTRIES      6000   /**< Max log entries?? */
#define MAX_HELP_KEYWORDS     256    /**< Max length of help keyword string */
#define MAX_HELP_ENTRY        MAX_STRING_LENGTH /**< Max size of help entry */
#define MAX_COMPLETED_QUESTS  1024   /**< Maximum number of completed quests allowed */
#define MAX_ANGER             100    /**< Maximum mob anger/frustration as percentage */

// other MAX_ defines
#define MAX_WEAPON_SPELLS     3
#define MAX_BAB               60
#define MAX_AC                55
#define MAX_CONCEAL           50 // its percentage
#define MAX_DAM_REDUC         20
#define MAX_ENERGY_ABSORB     20
/* NOTE: oasis.h has a maximum value for weapon dice, this is diffrent */
/* 2nd NOTE:  Hard-coded weapon dice caps in db.c */
#define MAX_WEAPON_DAMAGE     24
#define MIN_WEAPON_DAMAGE     2

/* maximum number of moves a mobile can store for walking paths (patrols) */
#define MAX_PATH              50

#define MAX_GOLD 2140000000 /**< Maximum possible on hand gold (2.14 Billion) */
#define MAX_BANK 2140000000 /**< Maximum possible in bank gold (2.14 Billion) */

/** Define the largest set of commands for a trigger.
 * 16k should be plenty and then some. */
#define MAX_CMD_LENGTH 16384

/* Type Definitions */
typedef signed char sbyte; /**< 1 byte; vals = -127 to 127 */
typedef unsigned char ubyte; /**< 1 byte; vals = 0 to 255 */
typedef signed short int sh_int; /**< 2 bytes; vals = -32,768 to 32,767 */
typedef unsigned short int ush_int; /**< 2 bytes; vals = 0 to 65,535 */
#if !defined(__cplusplus)	/* Anyone know a portable method? */
typedef char bool; /**< Technically 1 signed byte; vals should only = TRUE or FALSE. */
#endif

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)	/* Hm, sysdep.h? */
typedef signed char byte; /**< Technically 1 signed byte; vals should only = TRUE or FALSE. */
#endif

/* Various virtual (human-reference) number types. */
typedef IDXTYPE room_vnum; /**< vnum specifically for room */
typedef IDXTYPE obj_vnum; /**< vnum specifically for object */
typedef IDXTYPE mob_vnum; /**< vnum specifically for mob (NPC) */
typedef IDXTYPE zone_vnum; /**< vnum specifically for zone */
typedef IDXTYPE shop_vnum; /**< vnum specifically for shop */
typedef IDXTYPE trig_vnum; /**< vnum specifically for triggers */
typedef IDXTYPE qst_vnum; /**< vnum specifically for quests */
typedef IDXTYPE clan_vnum; /**< vnum specifically for clans */

/* Various real (array-reference) number types. */
typedef IDXTYPE room_rnum; /**< references an instance of a room */
typedef IDXTYPE obj_rnum; /**< references an instance of a obj */
typedef IDXTYPE mob_rnum; /**< references an instance of a mob (NPC) */
typedef IDXTYPE zone_rnum; /**< references an instance of a zone */
typedef IDXTYPE shop_rnum; /**< references an instance of a shop */
typedef IDXTYPE trig_rnum; /**< references an instance of a trigger */
typedef IDXTYPE qst_rnum; /**< references an instance of a quest */
typedef IDXTYPE clan_rnum; /**< references an instance of a clan */

/** Bitvector type for 32 bit unsigned long bitvectors. 'unsigned long long'
 * will give you at least 64 bits if you have GCC. You'll have to search
 * throughout the code for "bitvector_t" and change them yourself if you'd
 * like this extra flexibility. */
typedef unsigned long int bitvector_t;

/** Extra description: used in objects, mobiles, and rooms. For example,
 * a 'look hair' might pull up an extra description (if available) for
 * the mob, object or room.
 * Multiple extra descriptions on the same object are implemented as a
 * linked list. */
struct extra_descr_data {
  char *keyword; /**< Keyword for look/examine  */
  char *description; /**< What is shown when this keyword is 'seen' */
  struct extra_descr_data *next; /**< Next description for this mob/obj/room */
};

/* object-related structures */
/**< Number of elements in the object value array. Raising this will provide
 * more configurability per object type, and shouldn't break anything.
 * DO NOT LOWER from the default value of 4. */
#define NUM_OBJ_VAL_POSITIONS 4

/** object flags used in obj_data. These represent the instance values for
 * a real object, values that can change during gameplay. */
struct obj_flag_data {
  int value[NUM_OBJ_VAL_POSITIONS]; /**< Values of the item (see list)    */
  byte type_flag; /**< Type of item			    */
  byte prof_flag; // proficiency associated with item
  int level; /**< Minimum level to use object	    */
  int wear_flags[TW_ARRAY_MAX]; /**< Where you can wear it, if wearable */
  int extra_flags[EF_ARRAY_MAX]; /**< If it hums, glows, etc.	    */
  int weight; /**< Weight of the object */
  int cost; /**< Value when sold             */
  int cost_per_day; /**< Rent cost per real day */
  int timer; /**< Timer for object             */
  int bitvector[AF_ARRAY_MAX]; /**< Affects characters           */

  byte material;

  int size;
};

/** Used in obj_file_elem. DO NOT CHANGE if you are using binary object files
 * and already have a player base and don't want to do a player wipe. */
struct obj_affected_type {
  byte location; /**< Which ability to change (APPLY_XXX) */
  sbyte modifier; /**< How much it changes by              */
};

/* For weapon spells. */
struct weapon_spells {
  int spellnum;
  int level;
  int percent;
  int inCombat;
};


// Spellbooks
/* maximum # spells in a spellbook */
#define SPELLBOOK_SIZE    50

/* the spellbook struct */
struct obj_spellbook_spell {
  ush_int spellname; /* Which spell is written */
  ubyte pages; /* How many pages does it take up */
};

/** The Object structure. */
struct obj_data {
  obj_rnum item_number; /**< The unique id of this object instance. */
  room_rnum in_room; /**< What room is the object lying in, or -1? */

  struct obj_flag_data obj_flags; /**< Object information            */
  struct obj_affected_type affected[MAX_OBJ_AFFECT]; /**< affects */

  char *name; /**< Keyword reference(s) for object. */
  char *description; /**< Shown when the object is lying in a room. */
  char *short_description; /**< Shown when worn, carried, in a container */
  char *action_description; /**< Displays when (if) the object is used */
  struct extra_descr_data *ex_description; /**< List of extra descriptions */
  struct char_data *carried_by; /**< Points to PC/NPC carrying, or NULL */
  struct char_data *worn_by; /**< Points to PC/NPC wearing, or NULL */
  sh_int worn_on; /**< If the object can be worn, where can it be worn? */

  struct obj_data *in_obj; /**< Points to carrying object, or NULL */
  struct obj_data *contains; /**< List of objects being carried, or NULL */

  long id; /**< used by DG triggers - unique id  */
  struct trig_proto_list *proto_script; /**< list of default triggers  */
  struct script_data *script; /**< script info for the object */

  struct obj_data *next_content; /**< For 'contains' lists   */
  struct obj_data *next; /**< For the object list */
  struct char_data *sitting_here; /**< For furniture, who is sitting in it */

  bool has_spells;
  struct weapon_spells wpn_spells[MAX_WEAPON_SPELLS];

  struct obj_spellbook_spell *sbinfo; /* For spellbook info */
  
  struct list_data *events;      /**< Used for object events */  
};

/** Instance info for an object that gets saved to disk.
 * DO NOT CHANGE if you are using binary object files
 * and already have a player base and don't want to do a player wipe. */
struct obj_file_elem {
  obj_vnum item_number; /**< The prototype, non-unique info for this object. */

#if USE_AUTOEQ
  sh_int location; /**< If re-equipping objects on load, wear object here */
#endif
  int value[NUM_OBJ_VAL_POSITIONS]; /**< Current object values */
  int extra_flags[EF_ARRAY_MAX]; /**< Object extra flags */
  int weight; /**< Object weight */
  int timer; /**< Current object timer setting */
  int bitvector[AF_ARRAY_MAX]; /**< Object affects */
  struct obj_affected_type affected[MAX_OBJ_AFFECT]; /**< Affects to mobs */
};

/** Header block for rent files.
 * DO NOT CHANGE the structure if you are using binary object files
 * and already have a player base and don't want to do a player wipe.
 * If you are using binary player files, feel free to turn the spare
 * variables into something more meaningful, as long as you keep the
 * int datatype.
 * NOTE: This is *not* used with the ascii playerfiles.
 * NOTE 2: This structure appears to be unused in this codebase? */
struct rent_info {
  int time;
  int rentcode; /**< How this character rented */
  int net_cost_per_diem; /**< ? Appears to be unused ? */
  int gold; /**< ? Appears to be unused ? */
  int account; /**< ? Appears to be unused ? */
  int nitems; /**< ? Appears to be unused ? */
  int spare0;
  int spare1;
  int spare2;
  int spare3;
  int spare4;
  int spare5;
  int spare6;
  int spare7;
};

/* room-related structures */

/** Direction (north, south, east...) information for rooms. */
struct room_direction_data {
  char *general_description; /**< Show to char looking in this direction. */

  char *keyword; /**< for interacting (open/close) this direction */

  sh_int /*bitvector_t*/ exit_info; /**< Door, and what type? */
  obj_vnum key; /**< Key's vnum (-1 for no key) */
  room_rnum to_room; /**< Where direction leads, or NOWHERE if not defined */
};

struct raff_node {
  room_rnum room; /* location in the world[] array of the room */
  int timer; /* how many rounds this affection lasts */
  long affection; /* which affection does this room have */
  int spell; /* the spell number */
  struct char_data *ch; // caster of this affection

  struct raff_node *next; /* link to the next node */
};

/** The Room Structure. */
struct room_data {
  room_vnum number; /**< Rooms number (vnum) */
  zone_rnum zone; /**< Room zone (for resetting) */
  int sector_type; /**< sector type (move/hide) */
  int room_flags[RF_ARRAY_MAX]; /**< INDOORS, DARK, etc */
  long room_affections; /* bitvector for spells/skills */
  char *name; /**< Room name */
  char *description; /**< Shown when entered, looked at */
  struct extra_descr_data *ex_description; /**< Additional things to look at */
  struct room_direction_data * dir_option[NUM_OF_DIRS]; /**< Directions */
  byte light; /**< Number of lightsources in room */
  SPECIAL(*func); /**< Points to special function attached to room */
  struct trig_proto_list *proto_script; /**< list of default triggers */
  struct script_data *script; /**< script info for the room */
  struct obj_data *contents; /**< List of items in room */
  struct char_data *people; /**< List of NPCs / PCs in room */

  struct list_data * events;  
};

/* char-related structures */

/** Memory structure used by NPCs to remember specific PCs. */
struct memory_rec_struct {
  long id; /**< The PC id to remember. */
  struct memory_rec_struct *next; /**< Next PC to remember */
};

/** memory_rec_struct typedef */
typedef struct memory_rec_struct memory_rec;

/** This structure is purely intended to be an easy way to transfer and return
 * information about time (real or mudwise). */
struct time_info_data {
  int hours; /**< numeric hour */
  int day; /**< numeric day */
  int month; /**< numeric month */
  sh_int year; /**< numeric year */
};

/** Player specific time information. */
struct time_data {
  time_t birth; /**< Represents the PCs birthday, used to calculate age. */
  time_t logon; /**< Time of the last logon, used to calculate time played */
  int played; /**< This is the total accumulated time played in secs */
};

/* Group Data Struct */
struct group_data {
  struct char_data * leader;
  struct list_data * members;
  int group_flags;
};

/** The pclean_criteria_data is set up in config.c and used in db.c to determine
 * the conditions which will cause a player character to be deleted from disk
 * if the automagic pwipe system is enabled (see config.c). */
struct pclean_criteria_data {
  int level; /**< PC level and below to check for deletion */
  int days; /**< time limit in days, for this level of PC */
};

/** General info used by PC's and NPC's. */
struct char_player_data {
  char passwd[MAX_PWD_LENGTH + 1]; /**< PC's password */
  char *name; /**< PC / NPC name */
  char *short_descr; /**< NPC 'actions' */
  char *long_descr; /**< PC / NPC look description */
  char *description; /**< NPC Extra descriptions */
  char *title; /**< PC / NPC title */
  byte sex; /**< PC / NPC sex */
  byte chclass; /**< PC / NPC class */
  byte level; /**< PC / NPC level */
  struct time_data time; /**< PC AGE in days */
  ubyte weight; /**< PC / NPC weight */
  ubyte height; /**< PC / NPC height */
  byte race; // Race
  byte pc_subrace; // SubRace  
  char *walkin; // NPC (for now) walkin message
  char *walkout; // NPC (for now) walkout message
};

/** Character abilities. Different instances of this structure are used for
 * both inherent and current ability scores (like when poison affects the
 * player strength). */
struct char_ability_data {
  sbyte str; /**< Strength.  */
  sbyte str_add; /**< Strength multiplier if str = 18. Usually from 0 to 100 */
  sbyte intel; /**< Intelligence */
  sbyte wis; /**< Wisdom */
  sbyte dex; /**< Dexterity */
  sbyte con; /**< Constitution */
  sbyte cha; /**< Charisma */
};

/* make sure this matches spells.h define */
#define NUM_DAM_TYPES  21
/** Character 'points', or health statistics. */
struct char_point_data {
  sh_int mana; /**< Current mana level  */
  sh_int max_mana; /**< Max mana level */
  sh_int hit; /**< Curent hit point, or health, level */
  sh_int max_hit; /**< Max hit point, or health, level */
  sh_int move; /**< Current move point, or stamina, level */
  sh_int max_move; /**< Max move point, or stamina, level */
  sh_int armor; // armor class
  sh_int spell_res; // spell resistance

  int gold; /**< Current gold carried on character */
  int bank_gold; /**< Gold the char has in a bank account	*/
  int exp; /**< The experience points, or value, of the character. */

  sbyte hitroll; /**< Any bonus or penalty to the hit roll */
  sbyte damroll; /**< Any bonus or penalty to the damage roll */

  int size; // size
  sh_int apply_saving_throw[NUM_OF_SAVING_THROWS]; /**< Saving throw (Bonuses) */
  sh_int resistances[NUM_DAM_TYPES];  // resistances (dam-types)

  /* note - if you add something new here, make sure to check
   handler.c reset_char_points() to see if it needs to be added */
};
#undef NUM_DAM_TYPES

/** char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the players file for PC's. */
struct char_special_data_saved {
  int alignment; /**< -1000 (evil) to 1000 (good) range. */
  long idnum; /**< PC's idnum; -1 for mobiles. */
  int act[PM_ARRAY_MAX]; /**< act flags for NPC's; player flag for PC's */
  int affected_by[AF_ARRAY_MAX]; /**< Bitvector for spells/skills affected by */
};

/** Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
  /* combat related */
  struct char_data *fighting; /**< Target of fight; else NULL */
  struct char_data *hunting; /**< Target of NPC hunt; else NULL */
  int parryAttempts; // how many parry attempts left in the round
  ubyte cloudkill; //how many more bursts of cloudkill left
  struct char_data *guarding;  //target for 'guard' ability
  bool firing;  //is char firing missile weapon?

  /* furniture */
  struct obj_data *furniture; /**< Object being sat on/in; else NULL */
  struct char_data *next_in_furniture; /**< Next person sitting, else NULL */

  /* mounts */
  struct char_data *riding; /* Who are they riding? */
  struct char_data *ridden_by; /* Who is riding them? */

  /* carrying */
  int carry_weight; /**< Carried weight */
  byte carry_items; /**< Number of items carried */

  /** casting **/
  bool isCasting; // casting or not
  int castingTime; // casting time
  int castingSpellnum; // spell casting
  int castingClass; // spell casting class
  struct char_data *castingTCH; // target char of spell
  struct obj_data *castingTOBJ; // target obj of spell

  /** crafting **/
  ubyte crafting_type; //like SCMD_x
  ubyte crafting_ticks;  // ticks left to complete task
  struct obj_data *crafting_object;  // refers to obj crafting (deprecated)
  ubyte crafting_repeat; // multiple objects created in one session
  int crafting_bonus; // bonus for crafting the item
  
  /* miscellaneous */
  int prayin[NUM_CASTERS]; //memorization
  byte position; /**< Standing, fighting, sleeping, etc. */
  int timer; /**< Timer for update */

  struct char_special_data_saved saved; /**< Constants saved for PCs. */
};

/** Data only needed by PCs, and needs to be saved to disk. */
struct player_special_data_saved {
  int skills[MAX_SKILLS + 1]; //saved skills
  ubyte abilities[MAX_ABILITIES + 1]; //abilities
  ubyte morphed; //polymorphed and form
  byte class_level[MAX_CLASSES]; //multi class
  int warding[MAX_WARDING]; //saved warding spells like stoneskin
  int spells_to_learn; //prac sessions left
  int abilities_to_learn; //training sessiosn left
  ubyte boosts; //stat boosts left
  ubyte spec_abil[MAX_CLASSES]; //spec abilities (ex. lay on hands)
  ubyte favored_enemy[MAX_ENEMIES]; //list of ranger favored enemies

  int praying[MAX_MEM][NUM_CASTERS]; //memorization
  int prayed[MAX_MEM][NUM_CASTERS]; //memorization
  int praytimes[MAX_MEM][NUM_CASTERS]; //memorization

  byte church;  // homeland-port
  
  int wimp_level; /**< Below this # of hit points, flee! */
  byte freeze_level; /**< Level of god who froze char, if any */
  sh_int invis_level; /**< level of invisibility */
  room_vnum load_room; /**< Which room to load PC into */
  int pref[PR_ARRAY_MAX]; /**< preference flags */
  ubyte bad_pws; /**< number of bad login attempts */
  sbyte conditions[3]; /**< Drunk, hunger, and thirst */
  struct txt_block * comm_hist[NUM_HIST]; /**< Communication history */
  ubyte page_length; /**< Max number of rows of text to send at once */
  ubyte screen_width; /**< How wide the display page is */
  int olc_zone; /**< Current olc permissions */

  int clanpoints; /**< Clan points may be spent in a clanhall */
  clan_vnum clan; /**< The clan number to which the player belongs     */
  int clanrank; /**< The player's rank within their clan (1=highest) */

  int questpoints; //quest points earned
  qst_vnum *completed_quests; /**< Quests completed              */
  int num_completed_quests; /**< Number completed              */
  int current_quest; /**< vnum of current quest         */
  int quest_time; /**< time left on current quest    */
  int quest_counter; /**< Count of targets left to get  */

  /* auto crafting quest */
  unsigned int autocquest_vnum;
  char *autocquest_desc;
  ubyte autocquest_material;
  ubyte autocquest_makenum;
  ubyte autocquest_qp;
  unsigned int autocquest_exp;
  unsigned int autocquest_gold;

  time_t lastmotd; /**< Last time player read motd */
  time_t lastnews; /**< Last time player read news */
};

/** Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the players file. */
struct player_special_data {
  struct player_special_data_saved saved; /**< Information to be saved. */

  char *poofin; /**< Description displayed to room on arrival of a god. */
  char *poofout; /**< Description displayed to room at a god's exit. */
  struct alias_data *aliases; /**< Command aliases			*/
  long last_tell; /**< idnum of PC who last told this PC, used to reply */
  void *last_olc_targ; /**< ? Currently Unused ? */
  int last_olc_mode; /**< ? Currently Unused ? */
  char *host; /**< Resolved hostname, or ip, for player. */
  int diplomacy_wait; /**< Diplomacy Timer */
  int buildwalk_sector; /**< Default sector type for buildwalk */

  /* salvation spell */
  int salvation_room;
  char *salvation_name;
};

/** Special data used by NPCs, not PCs */
struct mob_special_data {
  memory_rec *memory; /**< List of PCs to remember */
  byte attack_type; /**< The primary attack type (bite, sting, hit, etc.) */
  byte default_pos; /**< Default position (standing, sleeping, etc.) */
  byte damnodice; /**< The number of dice to roll for damage */
  byte damsizedice; /**< The size of each die rolled for damage. */
  float frustration_level; /**< The anger/frustration level of the mob */
  byte subrace[MAX_SUBRACES]; // SubRace
  struct quest_entry *quest; // quest info for a mob (homeland-port)
  room_rnum loadroom;  // mob loadroom saved
  /* echo system */
  byte echo_is_zone;    // display the echo to entire zone
  byte echo_frequency;  // how often to display echo
  byte echo_sequential; // sequential/random
  sh_int echo_count;    // how many echos
  char **echo_entries;  // echo array
  sh_int current_echo;  // keep track of the current echo, for sequential echos
  /* path system */
  int path_index;
  int path_delay;
  int path_reset;
  int path_size;
  int path[MAX_PATH];
};

/** An affect structure. */
struct affected_type {
  sh_int spell; /**< The spell that caused this */
  sh_int duration; /**< For how long its effects will last      */
  sh_int modifier; /**< Added/subtracted to/from apropriate ability     */
  byte location; /**< Tells which ability to change(APPLY_XXX). */
  int bitvector[AF_ARRAY_MAX]; /**< Tells which bits to set (AFF_XXX). */

  struct affected_type *next; /**< The next affect in the list of affects. */
};

/** The list element that makes up a list of characters following this
 * character. */
struct follow_type {
  struct char_data *follower; /**< Character directly following. */
  struct follow_type *next; /**< Next character following. */
};

/** Master structure for PCs and NPCs. */
struct char_data {
  int pfilepos; /**< PC playerfile pos and id number */
  mob_rnum nr; /**< NPC real instance number */
  room_rnum in_room; /**< Current location (real room number) */
  room_rnum was_in_room; /**< Previous location for linkdead people  */
  int wait; /**< wait for how many loops before taking action. */

  struct char_player_data player; /**< General PC/NPC data */
  struct char_ability_data real_abils; /**< Abilities without modifiers */
  struct char_ability_data aff_abils; /**< Abilities with modifiers */
  struct char_point_data points; /**< Point/statistics */
  struct char_point_data real_points; /**< Point/statistics */
  struct char_special_data char_specials; /**< PC/NPC specials	  */
  struct player_special_data *player_specials; /**< PC specials		  */
  struct mob_special_data mob_specials; /**< NPC specials		  */

  struct affected_type *affected; /**< affected by what spells    */
  struct obj_data * equipment[NUM_WEARS]; /**< Equipment array            */

  struct obj_data *carrying; /**< List head for objects in inventory */
  struct descriptor_data *desc; /**< Descriptor/connection info; NPCs = NULL */

  long id; /**< used by DG triggers - unique id */
  struct trig_proto_list *proto_script; /**< list of default triggers */
  struct script_data *script; /**< script info for the object */
  struct script_memory *memory; /**< for mob memory triggers */

  struct char_data *next_in_room; /**< Next PC in the room */
  struct char_data *next; /**< Next char_data in the room */
  struct char_data *next_fighting; /**< Next in line to fight */

  struct follow_type *followers; /**< List of characters following */
  struct char_data *master; /**< List of character being followed */

  struct group_data *group; /**< Character's Group */

  long pref; /**< unique session id */

  struct list_data * events;
};

/** descriptor-related structures */
struct txt_block {
  char *text; /**< ? */
  int aliased; /**< ? */
  struct txt_block *next; /**< ? */
};

/** ? */
struct txt_q {
  struct txt_block *head; /**< ? */
  struct txt_block *tail; /**< ? */
};

/** Master structure players. Holds the real players connection to the mud.
 * An analogy is the char_data is the body of the character, the descriptor_data
 * is the soul. */
struct descriptor_data {
  socket_t descriptor; /**< file descriptor for socket */
  char host[HOST_LENGTH + 1]; /**< hostname */
  byte bad_pws; /**< number of bad pw attemps this login */
  byte idle_tics; /**< tics idle at password prompt		*/
  int connected; /**< mode of 'connectedness'		*/
  int desc_num; /**< unique num assigned to desc		*/
  time_t login_time; /**< when the person connected		*/
  char *showstr_head; /**< for keeping track of an internal str	*/
  char **showstr_vector; /**< for paging through texts		*/
  int showstr_count; /**< number of pages to page through	*/
  int showstr_page; /**< which page are we currently showing?	*/
  char **str; /**< for the modify-str system		*/
  char *backstr; /**< backup string for modify-str system	*/
  size_t max_str; /**< maximum size of string in modify-str	*/
  long mail_to; /**< name for mail system			*/
  int has_prompt; /**< is the user at a prompt?             */
  char inbuf[MAX_RAW_INPUT_LENGTH]; /**< buffer for raw input		*/
  char last_input[MAX_INPUT_LENGTH]; /**< the last input			*/
  char small_outbuf[SMALL_BUFSIZE]; /**< standard output buffer		*/
  char *output; /**< ptr to the current output buffer	*/
  char **history; /**< History of commands, for ! mostly.	*/
  int history_pos; /**< Circular array position.		*/
  int bufptr; /**< ptr to end of current output		*/
  int bufspace; /**< space left in the output buffer	*/
  struct txt_block *large_outbuf; /**< ptr to large buffer, if we need it */
  struct txt_q input; /**< q of unprocessed input		*/
  struct char_data *character; /**< linked to char			*/
  struct char_data *original; /**< original char if switched		*/
  struct descriptor_data *snooping; /**< Who is this char snooping	*/
  struct descriptor_data *snoop_by; /**< And who is snooping this char	*/
  struct descriptor_data *next; /**< link to next descriptor		*/
  struct oasis_olc_data *olc; /**< OLC info */

  protocol_t *pProtocol; /**< Kavir plugin */

  struct list_data * events;
};

/* other miscellaneous structures */

/** Fight message display. This structure is used to hold the information to
 * be displayed for every different violent hit type. */
struct msg_type {
  char *attacker_msg; /**< Message displayed to attecker. */
  char *victim_msg; /**< Message displayed to victim. */
  char *room_msg; /**< Message displayed to rest of players in room. */
};

/** An entire message structure for a type of hit or spell or skill. */
struct message_type {
  struct msg_type die_msg; /**< Messages for death strikes. */
  struct msg_type miss_msg; /**< Messages for missed strikes. */
  struct msg_type hit_msg; /**< Messages for a succesful strike. */
  struct msg_type god_msg; /**< Messages when trying to hit a god. */
  struct message_type *next; /**< Next set of messages. */
};

/** Head of list of messages for an attack type. */
struct message_list {
  int a_type; /**< The id of this attack type. */
  int number_of_attacks; /**< How many attack messages to chose from. */
  struct message_type *msg; /**< List of messages.			*/
};

/** Social message data structure. */
struct social_messg {
  int act_nr; /**< The social id. */
  char *command; /**< The command to activate (smile, wave, etc.) */
  char *sort_as; /**< Priority of social sorted by this. */
  int hide; /**< If true, and target can't see actor, target doesn't see */
  int min_victim_position; /**< Required Position of victim */
  int min_char_position; /**< Required Position of char */
  int min_level_char; /**< Minimum PC level required to use this social. */

  /* No argument was supplied */
  char *char_no_arg; /**< Displayed to char when no argument is supplied */
  char *others_no_arg; /**< Displayed to others when no arg is supplied */

  /* An argument was there, and a victim was found */
  char *char_found; /**< Display to char when arg is supplied */
  char *others_found; /**< Display to others when arg is supplied */
  char *vict_found; /**< Display to target arg */

  /* An argument was there, as well as a body part, and a victim was found */
  char *char_body_found; /**< Display to actor */
  char *others_body_found; /**< Display to others */
  char *vict_body_found; /**< Display to target argument */

  /* An argument was there, but no victim was found */
  char *not_found; /**< Display when no victim is found */

  /* The victim turned out to be the character */
  char *char_auto; /**< Display when self is supplied */
  char *others_auto; /**< Display to others when self is supplied */

  /* If the char cant be found search the char's inven and do these: */
  char *char_obj_found; /**< Social performed on object, display to char */
  char *others_obj_found; /**< Social performed on object, display to others */
};

/** Describes bonuses, or negatives, applied to thieves skills. In practice
 * this list is tied to the character's dexterity attribute. */
struct dex_skill_type {
  sh_int p_pocket; /**< Alters the success rate of pick pockets */
  sh_int p_locks; /**< Alters the success of pick locks */
  sh_int traps; /**< Historically alters the success of trap finding. */
  sh_int sneak; /**< Alters the success of sneaking without being detected */
  sh_int hide; /**< Alters the success of hiding out of sight */
};

/** Describes the bonuses applied for a specific value of a character's
 * strength attribute. */
struct dex_app_type {
  sh_int reaction; /**< Historically affects reaction savings throws. */
  sh_int miss_att; /**< Historically affects missile attacks */
  sh_int defensive; /**< Alters character's inherent armor class */
};

/** Describes the bonuses applied for a specific value of a character's
 * strength attribute. */
struct str_app_type {
  sh_int tohit; /**< To Hit (THAC0) Bonus/Penalty        */
  sh_int todam; /**< Damage Bonus/Penalty                */
  sh_int carry_w; /**< Maximum weight that can be carrried */
  sh_int wield_w; /**< Maximum weight that can be wielded  */
};

/** Describes the bonuses applied for a specific value of a character's
 * wisdom attribute. */
struct wis_app_type {
  byte bonus; /**< how many practices player gains per lev */
};

/** Describes the bonuses applied for a specific value of a character's
 * intelligence attribute. */
struct int_app_type {
  byte learn; /**< how many % a player learns a spell/skill */
};

/** Describes the bonuses applied for a specific value of a
 * character's constitution attribute. */
struct con_app_type {
  sh_int hitp; /**< Added to a character's new MAXHP at each new level. */
};

/** Describes the bonuses applied for a specific value of a
 * character's charisma attribute. */
struct cha_app_type {
  sh_int cha_bonus; /* charisma bonus */
};

/** Stores, and used to deliver, the current weather information
 * in the mud world. */
struct weather_data {
  int pressure; /**< How is the pressure ( Mb )? */
  int change; /**< How fast and what way does it change? */
  int sky; /**< How is the sky? */
  int sunlight; /**< And how much sun? */
};

/** Element in monster and object index-tables.
 NOTE: Assumes sizeof(mob_vnum) >= sizeof(obj_vnum) */
struct index_data {
  mob_vnum vnum; /**< virtual number of this mob/obj   */
  int number; /**< number of existing units of this mob/obj  */
  /** Point to any SPECIAL function assoicated with mob/obj.
   * Note: These are not trigger scripts. They are functions hard coded in
   * the source code. */
  SPECIAL(*func);

  char *farg; /**< String argument for special function. */
  struct trig_data *proto; /**< Points to the trigger prototype. */
};

/** Master linked list for the mob/object prototype trigger lists. */
struct trig_proto_list {
  int vnum; /**< vnum of the trigger   */
  struct trig_proto_list *next; /**< next trigger          */
};

struct guild_info_type {
  int pc_class;
  room_vnum guild_room;
  int direction;
};

/** Happy Hour Data */
struct happyhour {
  int qp_rate;
  int exp_rate;
  int gold_rate;
  int ticks_left;
};

/** structure for list of recent players (see 'recent' command) */
struct recent_player {
  int vnum; /* The ID number for this instance */
  char name[MAX_NAME_LENGTH]; /* The char name of the player     */
  bool new_player; /* Is this a new player?           */
  bool copyover_player; /* Is this a player that was on during the last copyover? */
  time_t time; /* login time                      */
  char host[HOST_LENGTH + 1]; /* Host IP address                 */
  struct recent_player *next; /* Pointer to the next instance    */
};

/* Config structs */

/** The game configuration structure used for configurating the game play
 * variables. */
struct game_data {
  int pk_allowed; /**< Is player killing allowed?    */
  int pt_allowed; /**< Is player thieving allowed?   */
  int level_can_shout; /**< Level player must be to shout.   */
  int holler_move_cost; /**< Cost to holler in move points.    */
  int tunnel_size; /**< Number of people allowed in a tunnel.*/
  int max_exp_gain; /**< Maximum experience gainable per kill.*/
  int max_exp_loss; /**< Maximum experience losable per death.*/
  int max_npc_corpse_time; /**< Num tics before NPC corpses decompose*/
  int max_pc_corpse_time; /**< Num tics before PC corpse decomposes.*/
  int idle_void; /**< Num tics before PC sent to void(idle)*/
  int idle_rent_time; /**< Num tics before PC is autorented.   */
  int idle_max_level; /**< Level of players immune to idle.     */
  int dts_are_dumps; /**< Should items in dt's be junked?   */
  int load_into_inventory; /**< Objects load in immortals inventory. */
  int track_through_doors; /**< Track through doors while closed?    */
  int no_mort_to_immort; /**< Prevent mortals leveling to imms?    */
  int disp_closed_doors; /**< Display closed doors in autoexit?    */
  int diagonal_dirs; /**< Are there 6 or 10 directions? */
  int map_option; /**< MAP_ON, MAP_OFF or MAP_IMM_ONLY      */
  int map_size; /**< Default size for map command         */
  int minimap_size; /**< Default size for mini-map (automap)  */
  int script_players; /**< Is attaching scripts to players allowed? */
  float min_pop_to_claim; /**< Minimum popularity percentage required to claim a zone */

  char *OK; /**< When player receives 'Okay.' text.    */
  char *NOPERSON; /**< 'No one by that name here.'   */
  char *NOEFFECT; /**< 'Nothing seems to happen.'            */
};

/** The rent and crashsave options. */
struct crash_save_data {
  int free_rent; /**< Should the MUD allow rent for free?   */
  int max_obj_save; /**< Max items players can rent.           */
  int min_rent_cost; /**< surcharge on top of item costs.       */
  int auto_save; /**< Does the game automatically save ppl? */
  int autosave_time; /**< if auto_save=TRUE, how often?         */
  int crash_file_timeout; /**< Life of crashfiles and idlesaves.     */
  int rent_file_timeout; /**< Lifetime of normal rent files in days */
};

/** Important room numbers. This structure stores vnums, not real array
 * numbers. */
struct room_numbers {
  room_vnum mortal_start_room; /**< vnum of room that mortals enter at.  */
  room_vnum immort_start_room; /**< vnum of room that immorts enter at.  */
  room_vnum frozen_start_room; /**< vnum of room that frozen ppl enter.  */
  room_vnum donation_room_1; /**< vnum of donation room #1.            */
  room_vnum donation_room_2; /**< vnum of donation room #2.            */
  room_vnum donation_room_3; /**< vnum of donation room #3.            */
};

/** Operational game variables. */
struct game_operation {
  ush_int DFLT_PORT; /**< The default port to run the game.  */
  char *DFLT_IP; /**< Bind to all interfaces.     */
  char *DFLT_DIR; /**< The default directory (lib).    */
  char *LOGNAME; /**< The file to log messages to.    */
  int max_playing; /**< Maximum number of players allowed. */
  int max_filesize; /**< Maximum size of misc files.   */
  int max_bad_pws; /**< Maximum number of pword attempts.  */
  int siteok_everyone; /**< Everyone from all sites are SITEOK.*/
  int nameserver_is_slow; /**< Is the nameserver slow or fast?   */
  int use_new_socials; /**< Use new or old socials file ?      */
  int auto_save_olc; /**< Does OLC save to disk right away ? */
  char *MENU; /**< The MAIN MENU.        */
  char *WELC_MESSG; /**< The welcome message.      */
  char *START_MESSG; /**< The start msg for new characters.  */
  int medit_advanced; /**< Does the medit OLC show the advanced stats menu ? */
  int ibt_autosave; /**< Does "bug resolve" autosave ? */
  int protocol_negotiation; /**< Enable the protocol negotiation system ? */
  int special_in_comm; /**< Enable use of a special character in communication channels ? */
  int debug_mode; /**< Current Debug Mode */
};

/** The Autowizard options. */
struct autowiz_data {
  int use_autowiz; /**< Use the autowiz feature?   */
  int min_wizlist_lev; /**< Minimun level to show on wizlist.  */
};

/**
 Main Game Configuration Structure.
 Global variables that can be changed within the game are held within this
 structure. During gameplay, elements within this structure can be altered,
 thus affecting the gameplay immediately, and avoiding the need to recompile
 the code.
 If changes are made to values of the elements of this structure during game
 play, the information will be saved to disk.
 */
struct config_data {
  /** Path to on-disk file where the config_data structure gets written. */
  char *CONFFILE;
  /** In-game specific global settings, such as allowing player killing. */
  struct game_data play;
  /** How is renting, crash files, and object saving handled? */
  struct crash_save_data csd;
  /** Special designated rooms, like start rooms, and donation rooms. */
  struct room_numbers room_nums;
  /** Basic operational settings, like max file sizes and max players. */
  struct game_operation operation;
  /** Autowiz specific settings, like turning it on and minimum level */
  struct autowiz_data autowiz;
};

#ifdef MEMORY_DEBUG
#include "zmalloc.h"
#endif

#endif /* _STRUCTS_H_ */
