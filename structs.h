/**
 * @file structs.h                                 Part of LuminariMUD
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

#include "bool.h"     /* for bool */
#include "protocol.h" /* Kavir Plugin*/
#include "lists.h"

/** Intended use of this macro is to allow external packages to work with a
 * variety of versions without modifications.  For instance, an IS_CORPSE()
 * macro was introduced in pl13.  Any future code add-ons could take into
 * account the version and supply their own definition for the macro if used
 * on an older version. You are supposed to compare this with the macro
 * LUMINARIMUD_VERSION() in utils.h.
 * It is read as Major/Minor/Patchlevel - MMmmPP */
#define _LUMINARIMUD 0x030640

/** If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1 because
 * TRUE/FALSE aren't defined yet. */
#define USE_AUTOEQ 1

/* preamble */
/** As of bpl20, it should be safe to use unsigned data types for the various
 * virtual and real number data types.  There really isn't a reason to use
 * signed anymore so use the unsigned types and get 65,535 objects instead of
 * 32,768. NOTE: This will likely be unconditionally unsigned later.
 * 0 = use signed indexes; 1 = use unsigned indexes */
#define CIRCLE_UNSIGNED_INDEX 1

#if CIRCLE_UNSIGNED_INDEX
#define IDXTYPE unsigned int  /** Index types are unsigned ints */
#define IDXTYPE_MAX UINT_MAX  /** Used for compatibility checks. */
#define IDXTYPE_MIN 0         /**< Used for compatibility checks. */
#define NOWHERE ((IDXTYPE)~0) /**< Sets to unsigned_int_MAX, or -1 */
#define NOTHING ((IDXTYPE)~0) /**< Sets to unsigned_int_MAX, or -1 */
#define NOBODY ((IDXTYPE)~0)  /**< Sets to unsigned_int_MAX, or -1 */
#define NOFLAG ((IDXTYPE)~0)  /**< Sets to unsigned_int_MAX, or -1 */
#else
#define IDXTYPE signed int    /** Index types are unsigned short ints */
#define IDXTYPE_MAX INT_MAX   /** Used for compatibility checks. */
#define IDXTYPE_MIN INT_MIN   /** Used for compatibility checks. */
#define NOWHERE ((IDXTYPE)-1) /**< nil reference for rooms */
#define NOTHING ((IDXTYPE)-1) /**< nil reference for objects */
#define NOBODY ((IDXTYPE)-1)  /**< nil reference for mobiles  */
#define NOFLAG ((IDXTYPE)-1)  /**< nil reference for flags   */
#endif

/** Function macro for the mob, obj and room special functions */
#define SPECIAL_DECL(name) \
    int(name)(struct char_data * ch, void *me, int cmd, const char *argument)

#define SPECIAL(name)                                                                   \
    static int impl_##name##_(struct char_data *ch, void *me, int cmd, char *argument); \
    int(name)(struct char_data * ch, void *me, int cmd, const char *argument)           \
    {                                                                                   \
        PERF_PROF_ENTER(pr_, #name);                                                    \
        int rtn;                                                                        \
        if (!argument)                                                                  \
        {                                                                               \
            rtn = impl_##name##_(ch, me, cmd, NULL);                                    \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            char arg_buf[MAX_INPUT_LENGTH];                                             \
            strlcpy(arg_buf, argument, sizeof(arg_buf));                                \
            rtn = impl_##name##_(ch, me, cmd, arg_buf);                                 \
        }                                                                               \
        PERF_PROF_EXIT(pr_);                                                            \
        return rtn;                                                                     \
    }                                                                                   \
    static int impl_##name##_(struct char_data *ch, void *me, int cmd, char *argument)

/* room-related defines */
/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH 0     /**< The direction north */
#define EAST 1      /**< The direction east */
#define SOUTH 2     /**< The direction south */
#define WEST 3      /**< The direction west */
#define UP 4        /**< The direction up */
#define DOWN 5      /**< The direction down */
#define NORTHWEST 6 /**< The direction north-west */
#define NORTHEAST 7 /**< The direction north-east */
#define SOUTHEAST 8 /**< The direction south-east */
#define SOUTHWEST 9 /**< The direction south-west */
/** Total number of directions available to move in. BEFORE CHANGING THIS, make
 * sure you change every other direction and movement based item that this will
 * impact. */
#define NUM_OF_DIRS 10
#define NUM_OF_INGAME_DIRS 6

/* TRAPS */
/* trap types */
#define TRAP_TYPE_LEAVE_ROOM 0
#define TRAP_TYPE_OPEN_DOOR 1
#define TRAP_TYPE_UNLOCK_DOOR 2
#define TRAP_TYPE_OPEN_CONTAINER 3
#define TRAP_TYPE_UNLOCK_CONTAINER 4
#define TRAP_TYPE_GET_OBJECT 5
#define TRAP_TYPE_ENTER_ROOM 6
/**/
#define MAX_TRAP_TYPES 7
/******************************************/
/* trap effects
   if the effect is < 1000, its just suppose to cast a spell */
#define TRAP_EFFECT_FIRST_VALUE 1000
/**/
#define TRAP_EFFECT_WALL_OF_FLAMES 1000
#define TRAP_EFFECT_LIGHTNING_STRIKE 1001
#define TRAP_EFFECT_IMPALING_SPIKE 1002
#define TRAP_EFFECT_DARK_GLYPH 1003
#define TRAP_EFFECT_SPIKE_PIT 1004
#define TRAP_EFFECT_DAMAGE_DART 1005
#define TRAP_EFFECT_POISON_GAS 1006
#define TRAP_EFFECT_DISPEL_MAGIC 1007
#define TRAP_EFFECT_DARK_WARRIOR_AMBUSH 1008
#define TRAP_EFFECT_BOULDER_DROP 1009
#define TRAP_EFFECT_WALL_SMASH 1010
#define TRAP_EFFECT_SPIDER_HORDE 1011
#define TRAP_EFFECT_DAMAGE_GAS 1012
#define TRAP_EFFECT_FREEZING_CONDITIONS 1013
#define TRAP_EFFECT_SKELETAL_HANDS 1014
#define TRAP_EFFECT_SPIDER_WEBS 1015
/**/
#define TOP_TRAP_EFFECTS 1016
#define MAX_TRAP_EFFECTS (TOP_TRAP_EFFECTS - TRAP_EFFECT_FIRST_VALUE)
/******************************************/
/*end traps*/

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK 0             /**< Dark room, light needed to see */
#define ROOM_DEATH 1            /**< Death trap, instant death */
#define ROOM_NOMOB 2            /**< MOBs not allowed in room */
#define ROOM_INDOORS 3          /**< Indoors, no weather */
#define ROOM_PEACEFUL 4         /**< Violence not allowed	*/
#define ROOM_SOUNDPROOF 5       /**< Shouts, gossip blocked */
#define ROOM_NOTRACK 6          /**< Track won't go through */
#define ROOM_NOMAGIC 7          /**< Magic not allowed */
#define ROOM_TUNNEL 8           /**< Room for only 1 pers	*/
#define ROOM_PRIVATE 9          /**< Can't teleport in */
#define ROOM_STAFFROOM 10       /**< LVL_STAFF+ only allowed */
#define ROOM_HOUSE 11           /**< (R) Room is a house */
#define ROOM_HOUSE_CRASH 12     /**< (R) House needs saving */
#define ROOM_ATRIUM 13          /**< (R) The door to a house */
#define ROOM_OLC 14             /**< (R) Modifyable/!compress */
#define ROOM_BFS_MARK 15        /**< (R) breath-first srch mrk */
#define ROOM_WORLDMAP 16        /**< World-map style maps here */
#define ROOM_REGEN 17           /* regen room */
#define ROOM_FLY_NEEDED 18      /* will drop without fly */
#define ROOM_NORECALL 19        /* no recalling from/to this room */
#define ROOM_SINGLEFILE 20      /* very narrow room */
#define ROOM_NOTELEPORT 21      /* no teleportin from/to this room */
#define ROOM_MAGICDARK 22       /* pitch black, not lightable */
#define ROOM_MAGICLIGHT 23      /* lit */
#define ROOM_NOSUMMON 24        /* no summoning from/to this room */
#define ROOM_NOHEAL 25          /* all regen stops in this room */
#define ROOM_NOFLY 26           /* can't fly in this room */
#define ROOM_FOG 27             /* fogged (hamper vision/stops daylight) */
#define ROOM_AIRY 28            /* airy (breathe underwater) */
#define ROOM_OCCUPIED 29        /* Used only in wilderness zones, if set the \ \ \ \
                                   room will be kept and used for the set    \ \ \ \
                                   coordinates. */
#define ROOM_SIZE_TINY 30       /* need to be tiny or smaller to enter */
#define ROOM_SIZE_DIMINUTIVE 31 /* need to be diminutive or smaller to enter */
#define ROOM_CLIMB_NEEDED 32    /* need climb skill, based on zone level */
#define ROOM_HASTRAP 33         /* has trap (not implemented yet) */
#define ROOM_GENDESC 34         /* Must be a wilderness room!  Use generated \ \ \ \
                                  descriptions in a static room, useful for  \ \ \ \
                                  rooms that block different directions.     \ \ \ \
                                  (eg. around obstacles.) */
/* idea:  possible room-flag for doing free memorization w/o spellbooks */
/****/
/** The total number of Room Flags */
#define NUM_ROOM_FLAGS 35

/* Room affects */
/* Old room-affection system, could be replaced by room-events
   theoritically, but for the time being its still in usage */
#define RAFF_FOG (1 << 0)
#define RAFF_DARKNESS (1 << 1)
#define RAFF_LIGHT (1 << 2)
#define RAFF_STINK (1 << 3)
#define RAFF_BILLOWING (1 << 4)
#define RAFF_ANTI_MAGIC (1 << 5)
#define RAFF_ACID_FOG (1 << 6)
#define RAFF_BLADE_BARRIER (1 << 7)
#define RAFF_SPIKE_GROWTH (1 << 8)
#define RAFF_SPIKE_STONES (1 << 9)
#define RAFF_HOLY (1 << 10)
#define RAFF_UNHOLY (1 << 11)
#define RAFF_OBSCURING_MIST (1 << 12)
#define RAFF_DIFFICULT_TERRAIN (1 << 13)
#define RAFF_SACRED_SPACE (1 << 14)
/** The total number of Room Affections */
#define NUM_RAFF 15

/* Zone info: Used in zone_data.zone_flags */
#define ZONE_CLOSED 0       /**< Zone is closed - players cannot enter */
#define ZONE_NOIMMORT 1     /**< Immortals (below LVL_GRSTAFF) cannot enter this zone */
#define ZONE_QUEST 2        /**< This zone is a quest zone (not implemented) */
#define ZONE_GRID 3         /**< Zone is 'on the grid', connected, show on 'areas' */
#define ZONE_NOBUILD 4      /**< Building is not allowed in the zone */
#define ZONE_NOASTRAL 5     /**< No teleportation magic will work to or from this zone */
#define ZONE_WORLDMAP 6     /**< Whole zone uses the WORLDMAP by default */
#define ZONE_NOCLAIM 7      /**< Zone can't be claimed, or popularity changed */
#define ZONE_ASTRAL_PLANE 8 /* astral plane */
#define ZONE_ETH_PLANE 9    /* ethereal plane */
#define ZONE_ELEMENTAL 10   /* elemental plane */
#define ZONE_WILDERNESS 11
/** The total number of Zone Flags */
#define NUM_ZONE_FLAGS 12

#define NUM_FEMALE_NAMES 110
#define NUM_MALE_NAMES 110
#define NUM_SURNAMES 210

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR (1 << 0) /**< Exit is a door */
#define EX_CLOSED (1 << 1) /**< The door is closed */
#define EX_LOCKED (1 << 2)
#define EX_PICKPROOF (1 << 3)     /**< Lock can't be picked */
#define EX_HIDDEN (1 << 4)        /**< Exit is hidden, easy difficulty to find. */
#define EX_HIDDEN_MEDIUM (1 << 5) /**< Exit is hidden, medium difficulty to find. */
#define EX_HIDDEN_HARD (1 << 6)   /**< Exit is hidden, hard difficulty to find. */
#define EX_LOCKED_MEDIUM (1 << 7) /**< The door is locked, medium difficulty to pick. */
#define EX_LOCKED_HARD (1 << 8)   /**< The door is locked, hard difficulty to pick. */
#define EX_LOCKED_EASY (1 << 9)   /**< The door is locked, easy to pick */
#define EX_HIDDEN_EASY (1 << 10)
/** The total number of Exit Bits */
#define NUM_EXIT_BITS 11

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE 0         /**< Indoors, connected to SECT macro. */
#define SECT_CITY 1           /**< In a city			*/
#define SECT_FIELD 2          /**< In a field		*/
#define SECT_FOREST 3         /**< In a forest		*/
#define SECT_HILLS 4          /**< In the hills		*/
#define SECT_MOUNTAIN 5       /**< On a mountain		*/
#define SECT_WATER_SWIM 6     /**< Swimmable water		*/
#define SECT_WATER_NOSWIM 7   /**< Water - need a boat	*/
#define SECT_FLYING 8         /**< Flying			*/
#define SECT_UNDERWATER 9     /**< Underwater		*/
#define SECT_ZONE_START 10    // zone start (for asciimap)
#define SECT_ROAD_NS 11       // road runing north-south
#define SECT_ROAD_EW 12       // road running east-north
#define SECT_ROAD_INT 13      // road intersection
#define SECT_DESERT 14        // desert
#define SECT_OCEAN 15         // ocean (ships only, unfinished)
#define SECT_MARSHLAND 16     // marsh/swamps
#define SECT_HIGH_MOUNTAIN 17 // mountains (climb only)
#define SECT_PLANES 18        // non-prime (no effect yet)
#define SECT_OUTTER_PLANES SECT_PLANES
#define SECT_UD_WILD 19   // the outdoors of the underdark
#define SECT_UD_CITY 20   // city in the underdark
#define SECT_UD_INSIDE 21 // inside in the underdark
#define SECT_UD_WATER 22  // water in the underdark
#define SECT_UD_NOSWIM 23 // water, boat needed, in the underdark
#define SECT_UD_WATER_NOSWIM SECT_UD_NOSWIM
#define SECT_UD_NOGROUND 24 // chasm in the underdark (Flying)
#define SECT_LAVA 25        // lava (damaging)
#define SECT_D_ROAD_NS 26   // dirt road
#define SECT_D_ROAD_EW 27   // dirt road
#define SECT_D_ROAD_INT 28  // dirt road
#define SECT_CAVE 29        // cave
/* The following were added with the wilderness system - Ornir */
#define SECT_JUNGLE 30 // jungle, wet, mid elevations, hot.
#define SECT_TUNDRA 31 // tundra, dry, high elevations, extreme cold.
#define SECT_TAIGA 32  // boreal forest, higher elevations, cold.
#define SECT_BEACH 33  // beach, borders low areas and water.
#define SECT_SEAPORT 34
#define SECT_INSIDE_ROOM 35
/* End wilderness sectors. These can (and should!) be used in zones too! */
/** The total number of room Sector Types */
#define NUM_ROOM_SECTORS 36

/* char and mob-related defines */

/* History */
#define HIST_ALL 0      /**< Index to history of all channels */
#define HIST_SAY 1      /**< Index to history of all 'say' */
#define HIST_GOSSIP 2   /**< Index to history of all 'gossip' */
#define HIST_WIZNET 3   /**< Index to history of all 'wiznet' */
#define HIST_TELL 4     /**< Index to history of all 'tell' */
#define HIST_SHOUT 5    /**< Index to history of all 'shout' */
#define HIST_GRATS 6    /**< Index to history of all 'grats' */
#define HIST_HOLLER 7   /**< Index to history of all 'holler' */
#define HIST_AUCTION 8  /**< Index to history of all 'auction' */
#define HIST_CLANTALK 9 /**< Index to history of all 'clantalk' */
#define HIST_GSAY 10    /**< Index to history of all 'gsay' */
#define HIST_GTELL HIST_GSAY
/**/
#define NUM_HIST 11    /**< Total number of history indexes */
#define HISTORY_SIZE 5 /**< Number of last commands kept in each history */

/* Group Defines */
#define GROUP_OPEN (1 << 0) /**< Group is open for members */
#define GROUP_ANON (1 << 1) /**< Group is Anonymous */
#define GROUP_NPC (1 << 2)  /**< Group created by NPC and thus not listed */

// size definitions, based on DnD3.5
#define SIZE_UNDEFINED (-1)
#define SIZE_RESERVED 0
#define SIZE_FINE 1
#define SIZE_DIMINUTIVE 2
#define SIZE_TINY 3
#define SIZE_SMALL 4
#define SIZE_MEDIUM 5
#define SIZE_LARGE 6
#define SIZE_HUGE 7
#define SIZE_GARGANTUAN 8
#define SIZE_COLOSSAL 9
/* ** */
#define NUM_SIZES 10

/* this sytem is built on top of stock alignment
 * which is a value between -1000 to 1000
 * alignments */
#define LAWFUL_GOOD 0
#define NEUTRAL_GOOD 1
#define CHAOTIC_GOOD 2
#define LAWFUL_NEUTRAL 3
#define TRUE_NEUTRAL 4
#define CHAOTIC_NEUTRAL 5
#define LAWFUL_EVIL 6
#define NEUTRAL_EVIL 7
#define CHAOTIC_EVIL 8
/***/
#define NUM_ALIGNMENTS 9
/***/

/* PC classes */
#define CLASS_UNDEFINED (-1) /**< PC Class undefined */
#define CLASS_WIZARD 0       /**< PC Class wizard */
#define CLASS_CLERIC 1       /**< PC Class Cleric */
#define CLASS_ROGUE 2        /**< PC Class Rogue (former Thief) */
#define CLASS_WARRIOR 3      /**< PC Class Warrior */
#define CLASS_MONK 4         /**< PC Class monk */
#define CLASS_DRUID 5        // druids
#define CLASS_BERSERKER 6    // berserker
#define CLASS_SORCERER 7
#define CLASS_PALADIN 8
#define CLASS_RANGER 9
#define CLASS_BARD 10
#define CLASS_WEAPON_MASTER 11
#define CLASS_WEAPONMASTER CLASS_WEAPON_MASTER
#define CLASS_ARCANE_ARCHER 12
#define CLASS_ARCANEARCHER CLASS_ARCANE_ARCHER
#define CLASS_STALWART_DEFENDER 13
#define CLASS_STALWARTDEFENDER CLASS_STALWART_DEFENDER
#define CLASS_SHIFTER 14
#define CLASS_DUELIST 15
#define CLASS_MYSTIC_THEURGE 16
#define CLASS_MYSTICTHEURGE CLASS_MYSTIC_THEURGE
#define CLASS_ALCHEMIST 17
#define CLASS_ARCANE_SHADOW 18
#define CLASS_SACRED_FIST 19
#define CLASS_ELDRITCH_KNIGHT 20
#define CLASS_PSIONICIST 21
#define CLASS_PSION CLASS_PSIONICIST
#define CLASS_SPELLSWORD 22
#define CLASS_SHADOW_DANCER 23
#define CLASS_SHADOWDANCER CLASS_SHADOW_DANCER
#define CLASS_BLACKGUARD 24
#define CLASS_ASSASSIN 25
#define CLASS_INQUISITOR 26
//#define CLASS_PSYCHIC_WARRIOR   17
//#define CLASS_PSY_WARR CLASS_PSYCHIC_WARRIOR
//#define CLASS_SOULKNIFE         18
//#define CLASS_SOUL_KNIFE CLASS_SOULKNIFE
//#define CLASS_WILDER            19
/* !!!---- CRITICAL ----!!! make sure to add class names to constants.c's
   class_names[] - we are dependent on that for loading the feat-list */
/** Total number of available PC Classes */
#define NUM_CLASSES 26 // we have to increase this to 27 once inquisitor is done

// related to pc (classes, etc)
/* note that max_classes was established to reign in some of the
   pfile arrays associated with classes */
#define MAX_CLASSES 30 // total number of maximum pc classes
#define NUM_CASTERS 9  // direct reference to pray array
/*  x wizard 1
 *  x sorcerer 2
 *  x cleric 3
 *  x druid 4
 *  x bard 5
 *  x paladin 6
 *  x ranger 7
 * ****  load_prayX has to be changed in players.c manually for this ****
 */
/**************************/

/* cleric domains */
#define DOMAIN_UNDEFINED 0
#define DOMAIN_AIR 1
#define DOMAIN_EARTH 2
#define DOMAIN_FIRE 3
#define DOMAIN_WATER 4
#define DOMAIN_CHAOS 5
#define DOMAIN_DESTRUCTION 6
#define DOMAIN_EVIL 7
#define DOMAIN_GOOD 8
#define DOMAIN_HEALING 9
#define DOMAIN_KNOWLEDGE 10
#define DOMAIN_LAW 11
#define DOMAIN_TRICKERY 12
#define DOMAIN_PROTECTION 13
#define DOMAIN_TRAVEL 14
#define DOMAIN_WAR 15
/****************/
#define NUM_DOMAINS 16

// Domains not yet implemented
#define DOMAIN_ANIMAL 0
#define DOMAIN_DEATH 0
#define DOMAIN_LUCK 0
#define DOMAIN_MAGIC 0
#define DOMAIN_PLANT 0
#define DOMAIN_STRENGTH 0
#define DOMAIN_SUN 0
#define DOMAIN_UNIVERSAL 0
#define DOMAIN_ARTIFICE 0
#define DOMAIN_CHARM 0
#define DOMAIN_COMMUNITY 0
#define DOMAIN_CREATION 0
#define DOMAIN_DARKNESS 0
#define DOMAIN_GLORY 0
#define DOMAIN_LIBERATION 0
#define DOMAIN_MADNESS 0
#define DOMAIN_NOBILITY 0
#define DOMAIN_REPOSE 0
#define DOMAIN_RUNE 0
#define DOMAIN_SCALYKIND 0
#define DOMAIN_WEATHER 0
#define DOMAIN_MEDITATION 0
#define DOMAIN_FORGE 0
#define DOMAIN_PASSION 0
#define DOMAIN_INSIGHT 0
#define DOMAIN_TREACHERY 0
#define DOMAIN_STORM 0
#define DOMAIN_PESTILENCE 0
#define DOMAIN_SUFFERING 0
#define DOMAIN_RETRIBUTION 0
#define DOMAIN_PLANNING 0
#define DOMAIN_CRAFT 0
#define DOMAIN_DWARF 0
#define DOMAIN_TIME 0
#define DOMAIN_FAMILY 0
#define DOMAIN_MOON 0
#define DOMAIN_DROW 0
#define DOMAIN_ELF 0
#define DOMAIN_CAVERN 0
#define DOMAIN_ILLUSION 0
#define DOMAIN_SPELL 0
#define DOMAIN_HATRED 0
#define DOMAIN_TYRANNY 0
#define DOMAIN_FATE 0
#define DOMAIN_RENEWAL 0
#define DOMAIN_METAL 0
#define DOMAIN_OCEAN 0
#define DOMAIN_MOBILITY 0
#define DOMAIN_PORTAL 0
#define DOMAIN_TRADE 0
#define DOMAIN_UNDEATH 0
#define DOMAIN_MENTALISM 0
#define DOMAIN_GNOME 0
#define DOMAIN_HALFLING 0
#define DOMAIN_ORC 0
#define DOMAIN_SPIDER 0
#define DOMAIN_SLIME 0
#define DOMAIN_MEDIATION 0

// warding spells that need to be saved
#define MIRROR 0
#define STONESKIN 1
/*---------*/
#define NUM_WARDING 2
#define MAX_WARDING 10 // "warding" type spells such as stoneskin that save

/* at the beginning, spec_abil was an array reserved for daily resets
   considering we've converted most of our system to a cooldown system
   we have abandoned that primary purpose and converted her to an array
   of easy to use reserved values in the pfile that saves for special
   ability info we need */
#define SPELL_MANTLE 0    // spell mantle left
#define INCEND 1          // incendiary cloud
#define SONG_AFF 2        // how much to modify skill with song-affects
#define CALLCOMPANION 3   // animal companion vnum
#define CALLFAMILIAR 4    // familiars vnum
#define SORC_KNOWN 5      // true/false if can 'study'
#define RANG_KNOWN 6      // true/false if can 'study'
#define CALLMOUNT 7       // paladin mount vnum
#define WIZ_KNOWN 8       // true/false if can 'study'
#define BARD_KNOWN 9      // true/false if can 'study'
#define SHAPECHANGES 10   // druid shapechanges left today
#define C_DOOM 11         // creeping doom
#define DRUID_KNOWN 12    // true/false if can 'study'
#define AG_SPELLBATTLE 13 // arg for spellbattle racial
/* trelux weapon poison, applypoison skill */
#define TRLX_PSN_SPELL_VAL 14 // poison weapon data for trelux
#define TRLX_PSN_SPELL_LVL 15 // poison weapon data for trelux
#define TRLX_PSN_SPELL_HIT 16 // poison weapon data for trelux
#define CLOUD_K 17            /* cloud kill spells bursts remaining */
/* -- */
/*---------------*/
#define NUM_SPEC_ABIL 18
#define MAX_SPEC_ABIL MAX_CLASSES
/* max = MAX_CLASSES right now, which was 30 last time i checked  */

/* max enemies, reserved space for array of ranger's favored enemies */
#define MAX_ENEMIES 10

// Memorization
/* sorc can get up to 9 + charisma bonus of stat cap 50 = 5 */
#define NUM_SLOTS 15   // conersative-value max num slots per circle
#define NUM_CIRCLES 10 // max num circles
/* how much space to reserve in the mem arrays */
#define MAX_MEM NUM_SLOTS *NUM_CIRCLES

/* Instruments - bardic_performance */
#define INSTRUMENT_LYRE 0
#define INSTRUMENT_FLUTE 1
#define INSTRUMENT_HORN 2
#define INSTRUMENT_DRUM 3
#define INSTRUMENT_HARP 4
#define INSTRUMENT_MANDOLIN 5
/**/
#define MAX_INSTRUMENTS 6
/***************/

/* Draconic Heritages from Sorcerer Bloodline: Draconic */
#define DRACONIC_HERITAGE_NONE 0
#define DRACONIC_HERITAGE_BLACK 1
#define DRACONIC_HERITAGE_BLUE 2
#define DRACONIC_HERITAGE_GREEN 3
#define DRACONIC_HERITAGE_RED 4
#define DRACONIC_HERITAGE_WHITE 5
#define DRACONIC_HERITAGE_BRASS 6
#define DRACONIC_HERITAGE_BRONZE 7
#define DRACONIC_HERITAGE_COPPER 8
#define DRACONIC_HERITAGE_SILVER 9
#define DRACONIC_HERITAGE_GOLD 10
#define NUM_DRACONIC_HERITAGE_TYPES 11 // 1 more than the last above

// Races - specific, race type defines are below
#define RACE_UNDEFINED (-1) /*Race Undefined*/
#define RACE_HUMAN 0        /* Race Human */
#define RACE_ELF 1          /* Race Elf   */
#define RACE_DWARF 2        /* Race Dwarf */
#define RACE_H_TROLL 3      /* Race Troll (advanced) */
#define RACE_HALF_TROLL RACE_H_TROLL
#define RACE_CRYSTAL_DWARF 4 /* crystal dwarf (epic) */
#define RACE_HALFLING 5      // halfling
#define RACE_H_ELF 6         // half elf
#define RACE_HALF_ELF RACE_H_ELF
#define RACE_H_ORC 7 // half orc
#define RACE_HALF_ORC RACE_H_ORC
#define RACE_GNOME 8         // gnome
#define RACE_TRELUX 9        // trelux (epic)
#define RACE_ARCANA_GOLEM 10 // arcana golem (advanced)
#define RACE_ARCANE_GOLEM RACE_ARCANA_GOLEM
#define RACE_DROW 11 // drow
#define RACE_DROW_ELF RACE_DROW
#define RACE_DARK_ELF RACE_DROW
#define RACE_DUERGAR 12 // duergar
#define RACE_GRAY_DWARF RACE_DUERGAR
#define RACE_DARK_DWARF RACE_DUERGAR
#define RACE_DUERGAR_DWARF RACE_DUERGAR

/* last playable race above +1 */
#define NUM_RACES 13

#define RACE_LICH 13 /*quest only race*/

/* coming soon!*/
#define RACE_H_OGRE 14 // not yet implemented
#define RACE_HALF_OGRE RACE_H_OGRE

#define RACE_HORSE 15
#define RACE_HALF_DROW 16
#define RACE_ROCK_GNOME 17
#define RACE_DEEP_GNOME 18
#define RACE_SVIRFNEBLIN RACE_DEEP_GNOME
#define RACE_ORC 19
#define RACE_IRON_GOLEM 20
#define RACE_DRAGON_CLOUD 21
#define RACE_DINOSAUR 22
#define RACE_PIXIE 23
#define RACE_MEDIUM_FIRE_ELEMENTAL 24
#define RACE_MEDIUM_EARTH_ELEMENTAL 25
#define RACE_MEDIUM_AIR_ELEMENTAL 26
#define RACE_MEDIUM_WATER_ELEMENTAL 27
#define RACE_FIRE_ELEMENTAL 28
#define RACE_EARTH_ELEMENTAL 29
#define RACE_AIR_ELEMENTAL 30
#define RACE_WATER_ELEMENTAL 31
#define RACE_HUGE_FIRE_ELEMENTAL 32
#define RACE_HUGE_EARTH_ELEMENTAL 33
#define RACE_HUGE_AIR_ELEMENTAL 34
#define RACE_HUGE_WATER_ELEMENTAL 35
#define RACE_APE 36
#define RACE_BOAR 37
#define RACE_CHEETAH 38
#define RACE_CROCODILE 39
#define RACE_GIANT_CROCODILE 40
#define RACE_HYENA 41
#define RACE_LEOPARD 42
#define RACE_RHINOCEROS 43
#define RACE_WOLVERINE 44
#define RACE_MEDIUM_VIPER 45
#define RACE_LARGE_VIPER 46
#define RACE_HUGE_VIPER 47
#define RACE_CONSTRICTOR_SNAKE 48
#define RACE_GIANT_CONSTRICTOR_SNAKE 49
#define RACE_TIGER 50
#define RACE_BLACK_BEAR 51
#define RACE_BROWN_BEAR 52
#define RACE_POLAR_BEAR 53
#define RACE_LION 54
#define RACE_ELEPHANT 55
#define RACE_EAGLE 56
#define RACE_GHOUL 57
#define RACE_GHAST 58
#define RACE_MUMMY 59
#define RACE_MOHRG 60
#define RACE_SMALL_FIRE_ELEMENTAL 61
#define RACE_SMALL_EARTH_ELEMENTAL 62
#define RACE_SMALL_AIR_ELEMENTAL 63
#define RACE_SMALL_WATER_ELEMENTAL 64
#define RACE_LARGE_FIRE_ELEMENTAL 65
#define RACE_LARGE_EARTH_ELEMENTAL 66
#define RACE_LARGE_AIR_ELEMENTAL 67
#define RACE_LARGE_WATER_ELEMENTAL 68
#define RACE_BLINK_DOG 69
#define RACE_OWLBEAR 70
#define RACE_SHAMBLING_MOUND 71
#define RACE_TREANT 72
#define RACE_MYCANOID 73
#define RACE_SKELETON 74
#define RACE_ZOMBIE 75
#define RACE_WOLF 76
#define RACE_GREAT_CAT 77
#define RACE_MANDRAGORA 78
#define RACE_AEON_THELETOS 79
#define RACE_STIRGE 80
#define RACE_WHITE_DRAGON 81
#define RACE_BLACK_DRAGON 82
#define RACE_GREEN_DRAGON 83
#define RACE_BLUE_DRAGON 84
#define RACE_RED_DRAGON 85
#define RACE_MANTICORE 86
#define RACE_EFREETI 87
#define RACE_RAT 88
/**/
/* Total Number of available (in-game) PC Races*/
#define NUM_EXTENDED_RACES 89
/*****/

// npc sub-race types, currently our NPC's get 3 of these
#define SUBRACE_UNDEFINED (-1) /*Race Undefined*/
#define SUBRACE_UNKNOWN 0
#define SUBRACE_AIR 1
#define SUBRACE_ANGEL 2
#define SUBRACE_AQUATIC 3
#define SUBRACE_ARCHON 4
#define SUBRACE_AUGMENTED 5
#define SUBRACE_CHAOTIC 6
#define SUBRACE_COLD 7
#define SUBRACE_EARTH 8
#define SUBRACE_EVIL 9
#define SUBRACE_EXTRAPLANAR 10
#define SUBRACE_FIRE 11
#define SUBRACE_GOBLINOID 12
#define SUBRACE_GOOD 13
#define SUBRACE_INCORPOREAL 14
#define SUBRACE_LAWFUL 15
#define SUBRACE_NATIVE 16
#define SUBRACE_REPTILIAN 17
#define SUBRACE_SHAPECHANGER 18
#define SUBRACE_SWARM 19
#define SUBRACE_WATER 20
#define SUBRACE_DARKLING 21
#define SUBRACE_VAMPIRE 22
// total
#define NUM_SUB_RACES 23
/* how many subrace-types can a mobile have? */
/* note, if this is changed, a lot of other places have
 * to be changed as well -zusuk */
#define MAX_SUBRACES 3

// pc sub-race types, so far used for animal shapes spell
#define PC_SUBRACE_UNDEFINED (-1) /*Race Undefined*/
#define PC_SUBRACE_UNKNOWN 0
#define PC_SUBRACE_BADGER 1
#define PC_SUBRACE_PANTHER 2
#define PC_SUBRACE_BEAR 3
#define PC_SUBRACE_G_CROCODILE 4
// total
#define MAX_PC_SUBRACES 5

/* here we have our race types, like family category of races
   used for both pc and npc's currently */
#define RACE_TYPE_UNDEFINED (-1)
#define RACE_TYPE_UNKNOWN 0
#define RACE_TYPE_HUMANOID 1
#define RACE_TYPE_UNDEAD 2
#define RACE_TYPE_ANIMAL 3
#define RACE_TYPE_DRAGON 4
#define RACE_TYPE_GIANT 5
#define RACE_TYPE_ABERRATION 6
#define RACE_TYPE_CONSTRUCT 7
#define RACE_TYPE_ELEMENTAL 8
#define RACE_TYPE_FEY 9
#define RACE_TYPE_MAGICAL_BEAST 10
#define RACE_TYPE_MONSTROUS_HUMANOID 11
#define RACE_TYPE_OOZE 12
#define RACE_TYPE_OUTSIDER 13
#define RACE_TYPE_PLANT 14
#define RACE_TYPE_VERMIN 15
#define RACE_TYPE_LYCANTHROPE 16
/**/
#define NUM_RACE_TYPES 17
/**/

/* Sex */
#define SEX_NEUTRAL 0 /**< Neutral Sex (Hermaphrodite) */
#define SEX_MALE 1    /**< Male Sex (XY Chromosome) */
#define SEX_FEMALE 2  /**< Female Sex (XX Chromosome) */
/** Total number of Genders */
#define NUM_GENDERS 3
#define NUM_SEX NUM_GENDERS

/* Positions */
#define POS_DEAD 0      /**< Position = dead */
#define POS_MORTALLYW 1 /**< Position = mortally wounded */
#define POS_INCAP 2     /**< Position = incapacitated */
#define POS_STUNNED 3   /**< Position = stunned	*/
#define POS_SLEEPING 4  /**< Position = sleeping */
#define POS_RECLINING 5 /**< Position = reclining */
#define POS_CRAWLING 5  /**< Position = crawling - at behest of ornir */
#define POS_RESTING 6   /**< Position = resting	*/
#define POS_SITTING 7   /**< Position = sitting	*/
#define POS_FIGHTING 8  /**< Position = fighting */
#define POS_STANDING 9  /**< Position = standing */
/** Total number of positions. */
#define NUM_POSITIONS 10

/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER 0      /**< Player is a player-killer */
#define PLR_THIEF 1       /**< Player is a player-thief */
#define PLR_FROZEN 2      /**< Player is frozen */
#define PLR_DONTSET 3     /**< Don't EVER set (ISNPC bit, set by mud) */
#define PLR_WRITING 4     /**< Player writing (board/mail/olc) */
#define PLR_MAILING 5     /**< Player is writing mail */
#define PLR_CRASH 6       /**< Player needs to be crash-saved */
#define PLR_SITEOK 7      /**< Player has been site-cleared */
#define PLR_NOSHOUT 8     /**< Player not allowed to shout/goss */
#define PLR_NOTITLE 9     /**< Player not allowed to set title */
#define PLR_DELETED 10    /**< Player deleted - space reusable */
#define PLR_LOADROOM 11   /**< Player uses nonstandard loadroom */
#define PLR_NOWIZLIST 12  /**< Player shouldn't be on wizlist */
#define PLR_NODELETE 13   /**< Player shouldn't be deleted */
#define PLR_INVSTART 14   /**< Player should enter game wizinvis */
#define PLR_CRYO 15       /**< Player is cryo-saved (purge prog) */
#define PLR_NOTDEADYET 16 /**< (R) Player being extracted */
#define PLR_BUG 17        /**< Player is writing a bug */
#define PLR_IDEA 18       /**< Player is writing an idea */
#define PLR_TYPO 19       /**< Player is writing a typo */
#define PLR_SALVATION 20  /* for salvation cleric spell */
/***************/
#define NUM_PLR_BITS 21

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC 0          /**< Mob has a callable spec-proc */
#define MOB_SENTINEL 1      /**< Mob should not move */
#define MOB_SCAVENGER 2     /**< Mob picks up stuff on the ground */
#define MOB_ISNPC 3         /**< (R) Automatically set on all Mobs */
#define MOB_AWARE 4         /**< Mob can't be backstabbed */
#define MOB_AGGRESSIVE 5    /**< Mob auto-attacks everybody nearby */
#define MOB_STAY_ZONE 6     /**< Mob shouldn't wander out of zone */
#define MOB_WIMPY 7         /**< Mob flees if severely injured */
#define MOB_AGGR_EVIL 8     /**< Auto-attack any evil PC's */
#define MOB_AGGR_GOOD 9     /**< Auto-attack any good PC's */
#define MOB_AGGR_NEUTRAL 10 /**< Auto-attack any neutral PC's */
#define MOB_MEMORY 11       /**< remember attackers if attacked */
#define MOB_HELPER 12       /**< attack PCs fighting other NPCs */
#define MOB_NOCHARM 13      /**< Mob can't be charmed */
#define MOB_NOSUMMON 14     /**< Mob can't be summoned */
#define MOB_NOSLEEP 15      /**< Mob can't be slept */
#define MOB_NOBASH 16       /**< Mob can't be bashed (e.g. trees) */
#define MOB_NOBLIND 17      /**< Mob can't be blinded */
#define MOB_NOKILL 18       /**< Mob can't be attacked */
#define MOB_SENTIENT 19
#define MOB_NOTDEADYET 20 /**< (R) Mob being extracted */
#define MOB_MOUNTABLE 21
#define MOB_NODEAF 22
#define MOB_NOFIGHT 23
#define MOB_NOCLASS 24
#define MOB_NOGRAPPLE 25
#define MOB_C_ANIMAL 26
#define MOB_C_FAMILIAR 27
#define MOB_C_MOUNT 28
#define MOB_ELEMENTAL 29
#define MOB_ANIMATED_DEAD 30
#define MOB_GUARD 31       /* will protect citizen */
#define MOB_CITIZEN 32     /* will be protected by guard */
#define MOB_HUNTER 33      /* will track down foes & memory targets */
#define MOB_LISTEN 34      /* will enter room if hearing fighting */
#define MOB_LIT 35         /* light up mob */
#define MOB_PLANAR_ALLY 36 /* is a planar ally (currently unused) */
#define MOB_NOSTEAL 37     /* Can't steal from mob*/
#define MOB_INFO_KILL 38   /* mob, when killed, sends a message in game to everyone */
/* we added a bunch of filler flags due to incompatible zone files */
#define MOB_CUSTOM_GOLD 39
#define MOB_NO_AI 40
#define MOB_MERCENARY 41 // for buying mercenary charmies, only one per person
#define MOB_ENCOUNTER 42 // this mob is used in a wilderness based random encounter
#define MOB_SHADOW 43    // call shadow for shadowdancers
#define MOB_IS_OBJ 44    // when using a mob to represent an object: ie a quest board
#define MOB_BLOCK_N 45
#define MOB_BLOCK_E 46
#define MOB_BLOCK_S 47
#define MOB_BLOCK_W 48
#define MOB_BLOCK_NE 49
#define MOB_BLOCK_SE 50
#define MOB_BLOCK SW 51
#define MOB_BLOCK_NW 52
#define MOB_BLOCK_U 53
#define MOB_BLOCK_D 54
#define MOB_BLOCK_CLASS 55
#define MOB_BLOCK_RACE 56
#define MOB_BLOCK_LEVEL 57
#define MOB_BLOCK_ALIGN 58
#define MOB_BLOCK_ETHOS 59
#define MOB_UNUSED_23 60
#define MOB_UNUSED_24 61
#define MOB_NOCONFUSE 62
#define MOB_HUNTS_TARGET 63
#define MOB_ABIL_GRAPPLE 64
#define MOB_ABIL_PETRIFY 65
#define MOB_ABIL_TAIL_SPIKES 66
#define MOB_ABIL_LEVEL_DRAIN 67
#define MOB_ABIL_CHARM 68
#define MOB_ABIL_BLINK 69
#define MOB_ABIL_ENGULF 70
#define MOB_ABIL_CAUSE_FEAR 71
#define MOB_ABIL_CORRUPTION 72
#define MOB_ABIL_SWALLOW 73
#define MOB_ABIL_FLIGHT 74
#define MOB_ABIL_POISON 75
#define MOB_ABIL_REGENERATION 76
#define MOB_ABIL_PARALYZE 77
#define MOB_ABIL_FIRE_BREATH 78
#define MOB_ABIL_LIGHTNING_BREATH 79
#define MOB_ABIL_POISON_BREATH 80
#define MOB_ABIL_ACID_BREATH 81
#define MOB_ABIL_FROST_BREATH 82
#define MOB_ABIL_MAGIC_IMMUNITY 83
#define MOB_ABIL_INVISIBILITY 84

/**********************/
#define NUM_MOB_FLAGS 85

#define SHAPE_AFFECTS 3
#define MOB_ZOMBIE 11         /* animate dead levels 1-7 */
#define MOB_GHOUL 35          // " " level 11+
#define MOB_GIANT_SKELETON 36 // " " level 21+
#define MOB_MUMMY 37          // " " level 30
#define BARD_AFFECTS 7
#define MOB_PALADIN_MOUNT 70
#define MOB_PALADIN_MOUNT_SMALL 91
#define MOB_EPIC_PALADIN_MOUNT 79
#define MOB_EPIC_PALADIN_MOUNT_SMALL 92

/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF 0                   /**< Room descs won't normally be shown */
#define PRF_COMPACT 1                 /**< No extra CRLF pair before prompts */
#define PRF_NOSHOUT 2                 /**< Can't hear shouts */
#define PRF_NOTELL 3                  /**< Can't receive tells */
#define PRF_DISPHP 4                  /**< Display hit points in prompt */
#define PRF_DISPPSP 5                 /**< Display psp points in prompt */
#define PRF_DISPMOVE 6                /**< Display move points in prompt */
#define PRF_AUTOEXIT 7                /**< Display exits in a room */
#define PRF_NOHASSLE 8                /**< Aggr mobs won't attack */
#define PRF_QUEST 9                   /**< On quest */
#define PRF_SUMMONABLE 10             /**< Can be summoned */
#define PRF_NOREPEAT 11               /**< No repetition of comm commands */
#define PRF_HOLYLIGHT 12              /**< Can see in dark */
#define PRF_COLOR_1 13                /**< Color (low bit) */
#define PRF_COLOR_2 14                /**< Color (high bit) */
#define PRF_NOWIZ 15                  /**< Can't hear wizline */
#define PRF_LOG1 16                   /**< On-line System Log (low bit) */
#define PRF_LOG2 17                   /**< On-line System Log (high bit) */
#define PRF_NOAUCT 18                 /**< Can't hear auction channel */
#define PRF_NOGOSS 19                 /**< Can't hear gossip channel */
#define PRF_NOGRATZ 20                /**< Can't hear grats channel */
#define PRF_SHOWVNUMS 21              /**< Can see VNUMs */
#define PRF_DISPAUTO 22               /**< Show prompt HP, MP, MV when < 25% */
#define PRF_CLS 23                    /**< Clear screen in OLC */
#define PRF_BUILDWALK 24              /**< Build new rooms while walking */
#define PRF_AFK 25                    /**< AFK flag */
#define PRF_AUTOLOOT 26               /**< Loot everything from a corpse */
#define PRF_AUTOGOLD 27               /**< Loot gold from a corpse */
#define PRF_AUTOSPLIT 28              /**< Split gold with group */
#define PRF_AUTOSAC 29                /**< Sacrifice a corpse */
#define PRF_AUTOASSIST 30             /**< Auto-assist toggle */
#define PRF_AUTOMAP 31                /**< Show map at the side of room descs */
#define PRF_AUTOKEY 32                /**< Automatically unlock locked doors when opening */
#define PRF_AUTODOOR 33               /**< Use the next available door */
#define PRF_NOCLANTALK 34             /**< Don't show ALL clantalk channels (Imm-only) */
#define PRF_AUTOSCAN 35               // automatically scan each step?
#define PRF_DISPEXP 36                // autoprompt xp display
#define PRF_DISPEXITS 37              // autoprompt exits display
#define PRF_DISPROOM 38               // display room name and/or #
#define PRF_DISPMEMTIME 39            // display memtimes
#define PRF_DISPACTIONS 40            /**< action system display on prompt */
#define PRF_AUTORELOAD 41             /**< Attempt to automatically reload weapon (xbow/slings) */
#define PRF_COMBATROLL 42             /**< extra info during combat */
#define PRF_GUI_MODE 43               /**< add special tags to code for MSDP GUI */
#define PRF_NOHINT 44                 /**< show in-game hints to newer players */
#define PRF_AUTOCOLLECT 45            /**< collect ammo after combat automatically */
#define PRF_RP 46                     /**< Interested in Role-Playing! */
#define PRF_AOE_BOMBS 47              /** Bombs will use splash damage instead of single target */
#define PRF_FRIGHTENED 48             /* If set, victims of fear affects will flee */
#define PRF_PVP 49                    /* If set, will allow player vs. player combat against others also flagged */
#define PRF_AUTOCON 50                /* autoconsider, shows level difference of mobs in look command */
#define PRF_SMASH_DEFENSE 51          // stalwart defender level 10 ability
#define PRF_DISPGOLD 52               // will show gold in prompt
#define PRF_NO_CHARMIE_RESCUE 53      // charmie mobs won't rescue you
#define PRF_SEEK_ENCOUNTERS 54        // will try to find random encounters in wilderness
#define PRF_AVOID_ENCOUNTERS 55       // will try to avoid random encounters in wilderness
#define PRF_USE_STORED_CONSUMABLES 56 // will use the stored consumables system instead of stock TBAMUD use command
#define PRF_DISPTIME 57               // shows game time in prompt

/** Total number of available PRF flags */
#define NUM_PRF_FLAGS 58

/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_DONTUSE 0              /**< DON'T USE! */
#define AFF_BLIND 1                /**< (R) Char is blind */
#define AFF_INVISIBLE 2            /**< Char is invisible */
#define AFF_DETECT_ALIGN 3         /**< Char is sensitive to align */
#define AFF_DETECT_INVIS 4         /**< Char can see invis chars */
#define AFF_DETECT_MAGIC 5         /**< Char is sensitive to magic */
#define AFF_SENSE_LIFE 6           /**< Char can sense hidden life */
#define AFF_WATERWALK 7            /**< Char can walk on water */
#define AFF_SANCTUARY 8            /**< Char protected by sanct */
#define AFF_GROUP 9                /**< (R) Char is grouped */
#define AFF_CURSE 10               /**< Char is cursed */
#define AFF_INFRAVISION 11         /**< Char can kinda see in dark */
#define AFF_POISON 12              /**< (R) Char is poisoned */
#define AFF_PROTECT_EVIL 13        /**< Char protected from evil */
#define AFF_PROTECT_GOOD 14        /**< Char protected from good */
#define AFF_SLEEP 15               /**< (R) Char magically asleep */
#define AFF_NOTRACK 16             /**< Char can't be tracked */
#define AFF_FLYING 17              /**< Char is flying */
#define AFF_SCUBA 18               // waterbreathe
#define AFF_WATER_BREATH AFF_SCUBA // just the more conventional name
#define AFF_SNEAK 19               /**< Char can move quietly */
#define AFF_HIDE 20                /**< Char is hidden */
#define AFF_VAMPIRIC_CURSE 21      // hit victim heals attacker
#define AFF_CHARM 22               /**< Char is charmed */
#define AFF_BLUR 23                // char has blurry image
#define AFF_POWER_ATTACK 24        // power attack mode
#define AFF_EXPERTISE 25           // combat expertise mode
#define AFF_HASTE 26               // hasted
#define AFF_TOTAL_DEFENSE 27       // total defense mode
#define AFF_ELEMENT_PROT 28        // endure elements, etc
#define AFF_DEAF 29                // deafened
#define AFF_FEAR 30                // under affect of fear
#define AFF_STUN 31                // stunned
#define AFF_PARALYZED 32           // paralyzed
#define AFF_ULTRAVISION 33         /**< Char can see in dark */
#define AFF_GRAPPLED 34            // grappled (combat maneuver)
#define AFF_TAMED 35               // tamed therefore mountable
#define AFF_CLIMB 36               // affect that allows you to climb
#define AFF_NAUSEATED 37           // nauseated - physical abilities reduced
#define AFF_NON_DETECTION 38       /* can't be scryed */
#define AFF_SLOW 39                /* supernaturally slowed - less attacks */
#define AFF_FSHIELD 40             // fire shield - reflect damage
#define AFF_CSHIELD 41             // cold shield - reflect damage
#define AFF_MINOR_GLOBE 42         // invulnerable to lower level spells
#define AFF_ASHIELD 43             // acid shield - reflect damage
#define AFF_SIZECHANGED 44         /* size is unusual, bigger or smaller class */
#define AFF_TRUE_SIGHT 45          /* highest level of enhanced magical vision */
#define AFF_SPOT 46                /* spot mode - better chance at seeing 'hide' */
#define AFF_FATIGUED 47            /* exhausted, less physically effective */
#define AFF_REGEN 48               /* regenerating health at accelerated rate */
#define AFF_DISEASE 49             /* affected by a disease */
#define AFF_TFORM 50               // tenser's transformation - powerful physical transofmration
#define AFF_GLOBE_OF_INVULN 51     /* invulernability to certain spells */
#define AFF_LISTEN 52              /* in listen mode - better chance at hearing 'sneak' */
#define AFF_DISPLACE 53            /* displacement - 50% concealment */
#define AFF_SPELL_MANTLE 54        /* spell absorbtion defense */
#define AFF_CONFUSED 55            /* confused, taking random actions */
#define AFF_REFUGE 56              /* refuge from danger - effectively stealthed */
#define AFF_SPELL_TURNING 57       /* able to deflect an opponents offensive spell! */
#define AFF_MIND_BLANK 58          /* mind blanked from harsh enchantments */
#define AFF_SHADOW_SHIELD 59       /* surrounded by powerful defensive shadow magic */
#define AFF_TIME_STOPPED 60        /* all non-offensive spells are free actions! */
#define AFF_BRAVERY 61             /* immune to fear */
#define AFF_FREE_MOVEMENT 62       /* able to resist movement impending effects */
#define AFF_FAERIE_FIRE 63         /* surrounded by purple flame */
#define AFF_BATTLETIDE 64          /* powerful physical presence */
#define AFF_SPELL_RESISTANT 65     /* bonus to resisting spells */
#define AFF_DIM_LOCK 66            // locked to current plane (can't teleport)
#define AFF_DEATH_WARD 67          /* warded from death effects */
#define AFF_SPELLBATTLE 68         /* arcana golem spellbattle mode */
#define AFF_VAMPIRIC_TOUCH 69      // will make next attack vampiric
#define AFF_BLACKMANTLE 70         // stop normal regen, reduce healing
#define AFF_DANGERSENSE 71         // sense aggro in surround rooms
#define AFF_SAFEFALL 72            // reduce damage from falling
#define AFF_TOWER_OF_IRON_WILL 73  // reduce psionic damage (no effect yet)
#define AFF_INERTIAL_BARRIER 74    // absorb damage based on psp
#define AFF_NOTELEPORT 75          // make target not reachable via teleport
/* works in progress */
#define AFF_MAX_DAMAGE 76   // enhance next attack/spell/etc (no affect yet)
#define AFF_IMMATERIAL 77   // no physical body (ghost-like)
#define AFF_CAGE 78         // can't interact/be-interacted with
#define AFF_MAGE_FLAME 79   // light up an individual
#define AFF_DARKVISION 80   // perfect vision day/night
#define AFF_BODYWEAPONRY 81 // martial arts
#define AFF_FARSEE 82       // can see outside of room
#define AFF_MENZOCHOKER 83  // special object affect
/** Total number of affect flags not including the don't use flag. */
// don't forget to add to constants.c!
#define AFF_RAPID_SHOT 84  /* Rapid Shot Mode (FEAT_RAPID_SHOT) */
#define AFF_DAZED 85       /* Dazed*/
#define AFF_FLAT_FOOTED 86 /* caught off guard! */

#define AFF_DUAL_WIELD 87        /* Dual wield mode */
#define AFF_FLURRY_OF_BLOWS 88   /* Flurry of blows mode */
#define AFF_COUNTERSPELL 89      /* Counterspell mode */
#define AFF_DEFENSIVE_CASTING 90 /* Defensive casting mode */
#define AFF_WHIRLWIND_ATTACK 91  /*  Whirlwind attack mode */

#define AFF_CHARGING 92            /* charging in combat */
#define AFF_WILD_SHAPE 93          /* wildshape, shapechange */
#define AFF_FEINTED 94             /* flat-footed */
#define AFF_PINNED 95              /* pinned to the ground (grapple) */
#define AFF_MIRROR_IMAGED 96       /* duplicate illusions of self! */
#define AFF_WARDED 97              /* warded (damage protection) */
#define AFF_ENTANGLED 98           /* entangled (can't move) */
#define AFF_ACROBATIC 99           /* acrobatic!  currently used for druid jump \ \ \ \
                                      spell, possible expansion to follow */
#define AFF_BLINKING 100           /* in a state of blinking between prime/eth */
#define AFF_AWARE 101              /* aware - too aware to be backstabed */
#define AFF_CRIPPLING_CRITICAL 102 /* duelist crippling critical affection */
#define AFF_LEVITATE 103           /**< Char can float above the ground */
#define AFF_BLEED 104              /* character suffers bleed damage each round unless healed by treatinjury or another healing effect. */
#define AFF_STAGGERED 105          /* A staggered character has a 50% chance to fail a spell or a single melee attack */
#define AFF_DAZZLED 106            /* suffers -1 to attacks and perception checks */
#define AFF_SHAKEN 107             // fear/mind effect.  -2 to attack rols, saving throws, skill checks and ability checks
#define AFF_ESHIELD 108            // electric shield - reflect damage
#define AFF_SICKENED 109           // applies sickened status. -2 penalty to attack rolls, weapon damage, saving throws, skill checks and ability checks
/*---*/
#define NUM_AFF_FLAGS 110
/********************************/
/* add aff_ flag?  don't forget to add to:
   1)  places in code the affect will directly modify values
   2)  get_eq_score() in act.wizard.c - value of this affection on an object
   3)  constants.c:  const char *affected_bits
                     const char *affected_bit_descs */

/* Bonus types */
#define BONUS_TYPE_UNDEFINED 0     /* Undefined bonus type (stacks) */
#define BONUS_TYPE_ALCHEMICAL 1    /* Alchemical bonus : potion/food/chemical */
#define BONUS_TYPE_ARMOR 2         /* Armor bonus : +/- AC */
#define BONUS_TYPE_CIRCUMSTANCE 3  /* Circumstance Bonus (stacks) */
#define BONUS_TYPE_COMPETENCE 4    /* Competence bonus : skills, etc. */
#define BONUS_TYPE_DEFLECTION 5    /* Deflection bonus : + AC usually */
#define BONUS_TYPE_DODGE 6         /* Dodge bonus : + AC usually */
#define BONUS_TYPE_ENHANCEMENT 7   /* Enhancement bonus : weapon/armor */
#define BONUS_TYPE_INHERENT 8      /* Inherent bonus : powerful magic */
#define BONUS_TYPE_INSIGHT 9       /* Insight bonus */
#define BONUS_TYPE_LUCK 10         /* Luck bonus */
#define BONUS_TYPE_MORALE 11       /* Morale bonus */
#define BONUS_TYPE_NATURALARMOR 12 /* Natural Armor bonus : + AC */
#define BONUS_TYPE_PROFANE 13      /* Profane bonus : evil */
#define BONUS_TYPE_RACIAL 14       /* Racial bonus */
#define BONUS_TYPE_RESISTANCE 15   /* Resistance bonus : saves */
#define BONUS_TYPE_SACRED 16       /* Sacred Bonus : good */
#define BONUS_TYPE_SHIELD 17       /* Shield bonus */
#define BONUS_TYPE_SIZE 18         /* Size bonus */
#define BONUS_TYPE_TRAIT 19        /* Character Trait bonus */
/**/
#define NUM_BONUS_TYPES 20
/****/

/* Modes of connectedness: used by descriptor_data.state 		*/
#define CON_PLAYING 0       /**< Playing - Nominal state 		*/
#define CON_CLOSE 1         /**< User disconnect, remove character.	*/
#define CON_GET_NAME 2      /**< Login with name */
#define CON_NAME_CNFRM 3    /**< New character, confirm name */
#define CON_PASSWORD 4      /**< Login with password */
#define CON_NEWPASSWD 5     /**< New character, create password */
#define CON_CNFPASSWD 6     /**< New character, confirm password */
#define CON_QSEX 7          /**< Choose character sex */
#define CON_QCLASS 8        /**< Choose character class */
#define CON_RMOTD 9         /**< Reading the message of the day */
#define CON_MENU 10         /**< At the main menu */
#define CON_PLR_DESC 11     /**< Enter a new character description prompt */
#define CON_CHPWD_GETOLD 12 /**< Changing passwd: Get old		*/
#define CON_CHPWD_GETNEW 13 /**< Changing passwd: Get new */
#define CON_CHPWD_VRFY 14   /**< Changing passwd: Verify new password */
#define CON_DELCNF1 15      /**< Character Delete: Confirmation 1		*/
#define CON_DELCNF2 16      /**< Character Delete: Confirmation 2		*/
#define CON_DISCONNECT 17   /**< In-game link loss (leave character)	*/
#define CON_OEDIT 18        /**< OLC mode - object editor		*/
#define CON_REDIT 19        /**< OLC mode - room editor		*/
#define CON_ZEDIT 20        /**< OLC mode - zone info editor		*/
#define CON_MEDIT 21        /**< OLC mode - mobile editor		*/
#define CON_SEDIT 22        /**< OLC mode - shop editor		*/
#define CON_TEDIT 23        /**< OLC mode - text editor		*/
#define CON_CEDIT 24        /**< OLC mode - conf editor		*/
#define CON_AEDIT 25        /**< OLC mode - social (action) edit      */
#define CON_TRIGEDIT 26     /**< OLC mode - trigger edit              */
#define CON_HEDIT 27        /**< OLC mode - help edit */
#define CON_QEDIT 28        /**< OLC mode - quest edit */
#define CON_PREFEDIT 29     /**< OLC mode - preference edit */
#define CON_IBTEDIT 30      /**< OLC mode - idea/bug/typo edit */
#define CON_GET_PROTOCOL 31 /**< Used at log-in while attempting to get protocols > */
#define CON_QRACE 32        /**< Choose character race */
#define CON_CLANEDIT 33     /**< OLC mode - clan edit */
#define CON_MSGEDIT 34      /**< OLC mode - message editor */
#define CON_STUDY 35        /**< OLC mode - sorc-spells-known editor */
#define CON_QCLASS_HELP 36  /**< help info during char creation */
#define CON_QALIGN 37       /**< alignment selection in char creation */
#define CON_QRACE_HELP 38   /**< help info (race) during char creation */
#define CON_HLQEDIT 39      /**< homeland-port quest editor */
#define CON_CRAFTEDIT 40    /**< crafts system */
#define CON_QSTATS 41       /**< Point-buy system for stats */
/* Account connection states - Ornir Oct 20, 2014 */
#define CON_ACCOUNT_NAME 42
#define CON_ACCOUNT_NAME_CONFIRM 43
#define CON_ACCOUNT_MENU 44
#define CON_ACCOUNT_ADD 45
#define CON_ACCOUNT_ADD_PWD 46
#define CON_HSEDIT 47          /* OLC mode - house edit      .*/
#define CON_NEWMAIL 48         // new mail system mail composition
#define CON_CONFIRM_PREMADE 49 // premade build selection

/* OLC States range - used by IS_IN_OLC and IS_PLAYING */
#define FIRST_OLC_STATE CON_OEDIT    /**< The first CON_ state that is an OLC */
#define LAST_OLC_STATE CON_CRAFTEDIT /**< The last CON_ state that is an OLC  */
#define CON_IEDIT 50
#define NUM_CON_STATES 51

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
 which control the valid places you can wear a piece of equipment.
 For example, there are two neck positions on the player, and items
 only get the generic neck type. */
#define WEAR_LIGHT 0          /**< Equipment Location Light */
#define WEAR_FINGER_R 1       /**< Equipment Location Right Finger */
#define WEAR_FINGER_L 2       /**< Equipment Location Left Finger */
#define WEAR_NECK_1 3         /**< Equipment Location Neck #1 */
#define WEAR_NECK_2 4         /**< Equipment Location Neck #2 */
#define WEAR_BODY 5           /**< Equipment Location Body */
#define WEAR_HEAD 6           /**< Equipment Location Head */
#define WEAR_LEGS 7           /**< Equipment Location Legs */
#define WEAR_FEET 8           /**< Equipment Location Feet */
#define WEAR_HANDS 9          /**< Equipment Location Hands */
#define WEAR_ARMS 10          /**< Equipment Location Arms */
#define WEAR_SHIELD 11        /**< Equipment Location Shield */
#define WEAR_ABOUT 12         /**< Equipment Location about body (like a cape)*/
#define WEAR_WAIST 13         /**< Equipment Location Waist */
#define WEAR_WRIST_R 14       /**< Equipment Location Right Wrist */
#define WEAR_WRIST_L 15       /**< Equipment Location Left Wrist */
#define WEAR_WIELD_1 16       /**< Equipment Location Weapon */
#define WEAR_HOLD_1 17        /**< Equipment Location held in offhand */
#define WEAR_WIELD_OFFHAND 18 // off-hand weapon
#define WEAR_HOLD_2 19        // off-hand held
#define WEAR_WIELD_2H 20      // two-hand weapons
#define WEAR_HOLD_2H 21       // two-hand held
#define WEAR_FACE 22          // equipment location face
#define WEAR_AMMO_POUCH 23    // ammo pouch (for ranged weapons)
/* unfinished (might not implement) */
#define WEAR_EAR_R 24 /* worn on/in right ear */
#define WEAR_EAR_L 25 /* worn on/in left ear */
#define WEAR_EYES 26  /* worn in/over eye(s) */
#define WEAR_BADGE 27 /* attached to your body armor as a badge */
/** Total number of available equipment lcoations */
#define NUM_WEARS 28
/**/

/* ranged combat */
#define RANGED_BOW 0
#define RANGED_CROSSBOW 1
/**/
#define NUM_RANGED_WEAPONS 2

/* ranged combat */
#define MISSILE_ARROW 0
#define MISSILE_BOLT 1
/**/
#define NUM_RANGED_MISSILES 2

/***************************************/
/* Feats defined below up to MAX_FEATS */
#define FEAT_UNDEFINED 0
#define FEAT_ALERTNESS 1
#define FEAT_SEEKER_ARROW 2 // arcane archer
#define FEAT_ARMOR_PROFICIENCY_HEAVY 3
#define FEAT_ARMOR_PROFICIENCY_LIGHT 4
#define FEAT_ARMOR_PROFICIENCY_MEDIUM 5
#define FEAT_BLIND_FIGHT 6
#define FEAT_BREW_POTION 7
#define FEAT_CLEAVE 8
#define FEAT_COMBAT_CASTING 9
#define FEAT_COMBAT_REFLEXES 10
#define FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR 11
#define FEAT_CRAFT_ROD 12
#define FEAT_CRAFT_STAFF 13
#define FEAT_CRAFT_WAND 14
#define FEAT_CRAFT_WONDEROUS_ITEM 15
#define FEAT_DEFLECT_ARROWS 16 // monk, etc
#define FEAT_DODGE 17
#define FEAT_EMPOWER_SPELL 18
#define FEAT_ENDURANCE 19
#define FEAT_ENLARGE_SPELL 20
#define FEAT_QUICK_TO_MASTER 21
#define FEAT_NATURAL_TRACKER 22
#define FEAT_EXTEND_SPELL 23
#define FEAT_EXTRA_TURNING 24
#define FEAT_FAR_SHOT 25
#define FEAT_FORGE_RING 26
#define FEAT_GREAT_CLEAVE 27
#define FEAT_GREAT_FORTITUDE 28
#define FEAT_HEIGHTEN_SPELL 29
#define FEAT_IMPROVED_BULL_RUSH 30
#define FEAT_IMPROVED_CRITICAL 31
#define FEAT_RAGE 32
#define FEAT_FAST_MOVEMENT 33
#define FEAT_LAYHANDS 34 // paladin
#define FEAT_AURA_OF_GOOD 35
#define FEAT_AURA_OF_COURAGE 36
#define FEAT_DIVINE_GRACE 37 // paladin
#define FEAT_SMITE_EVIL 38
#define FEAT_REMOVE_DISEASE 39
#define FEAT_DIVINE_HEALTH 40
#define FEAT_TURN_UNDEAD 41
#define FEAT_DETECT_EVIL 42
#define FEAT_SKILLED 43
#define FEAT_IMPROVED_REACTION 44
#define FEAT_ENHANCED_MOBILITY 45
#define FEAT_GRACE 46
#define FEAT_PRECISE_STRIKE 47
#define FEAT_ACROBATIC_CHARGE 48 // duelist
#define FEAT_ELABORATE_PARRY 49  // duelist
#define FEAT_DAMAGE_REDUCTION 50
#define FEAT_GREATER_RAGE 51
#define FEAT_MIGHTY_RAGE 52
#define FEAT_TIRELESS_RAGE 53
#define FEAT_ARMORED_MOBILITY 54
#define FEAT_SLEEP_ENCHANTMENT_IMMUNITY 55
#define FEAT_KEEN_SENSES 56
#define FEAT_RESISTANCE_TO_ENCHANTMENTS 57
#define FEAT_RALLYING_CRY 58
#define FEAT_POISON_BITE 59
#define FEAT_POISON_RESIST 60
#define FEAT_IMPROVED_DISARM 61
#define FEAT_IMPROVED_INITIATIVE 62
#define FEAT_IMPROVED_TRIP 63
#define FEAT_IMPROVED_TWO_WEAPON_FIGHTING 64
#define FEAT_IMPROVED_UNARMED_STRIKE 65
#define FEAT_IRON_WILL 66
#define FEAT_ELF_RACIAL_ADJUSTMENT 67 // elf
#define FEAT_LIGHTNING_REFLEXES 68
#define FEAT_MARTIAL_WEAPON_PROFICIENCY 69
#define FEAT_MAXIMIZE_SPELL 70
#define FEAT_MOBILITY 71
#define FEAT_MOUNTED_ARCHERY 72
#define FEAT_MOUNTED_COMBAT 73
#define FEAT_POINT_BLANK_SHOT 74
#define FEAT_POWER_ATTACK 75
#define FEAT_PRECISE_SHOT 76
#define FEAT_QUICK_DRAW 77
#define FEAT_QUICKEN_SPELL 78
#define FEAT_RAPID_SHOT 79
#define FEAT_RIDE_BY_ATTACK 80
#define FEAT_STABILITY 81 // dwarf?
#define FEAT_SCRIBE_SCROLL 82
#define FEAT_SONG_OF_FOCUSED_MIND 83 // bard
#define FEAT_SHOT_ON_THE_RUN 84
#define FEAT_SILENT_SPELL 85
#define FEAT_SIMPLE_WEAPON_PROFICIENCY 86
#define FEAT_SKILL_FOCUS 87
#define FEAT_SPELL_FOCUS 88
#define FEAT_SONG_OF_FEAR 89        // bard
#define FEAT_SONG_OF_ROOTING 90     // bard
#define FEAT_SONG_OF_THE_MAGI 91    // bard
#define FEAT_SONG_OF_HEALING 92     // bard
#define FEAT_DANCE_OF_PROTECTION 93 // bard
#define FEAT_SONG_OF_FLIGHT 94      // bard
#define FEAT_SONG_OF_HEROISM 95     // bard
#define FEAT_SPELL_MASTERY 96
#define FEAT_SPELL_PENETRATION 97
#define FEAT_SPIRITED_CHARGE 98
#define FEAT_SPRING_ATTACK 99
#define FEAT_STILL_SPELL 100
#define FEAT_STUNNING_FIST 101 // monk
#define FEAT_SUNDER 102
#define FEAT_TOUGHNESS 103
#define FEAT_TRACK 104
#define FEAT_TRAMPLE 105
#define FEAT_TWO_WEAPON_FIGHTING 106
#define FEAT_WEAPON_FINESSE 107
#define FEAT_SPELL_HARDINESS 108
#define FEAT_COMBAT_TRAINING_VS_GIANTS 109
#define FEAT_CANNY_DEFENSE 110
#define FEAT_DWARF_RACIAL_ADJUSTMENT 111
#define FEAT_ORATORY_OF_REJUVENATION 112
#define FEAT_SHADOW_HOPPER 113              // halfling
#define FEAT_LUCKY 114                      // halfling
#define FEAT_HALFLING_RACIAL_ADJUSTMENT 115 // halfling
#define FEAT_INDOMITABLE_WILL 116
#define FEAT_UNCANNY_DODGE 117
#define FEAT_IMPROVED_UNCANNY_DODGE 118
#define FEAT_TRAP_SENSE 119
#define FEAT_UNARMED_STRIKE 120
#define FEAT_STILL_MIND 121
#define FEAT_KI_STRIKE 122                  // monk
#define FEAT_SLOW_FALL 123                  // monk
#define FEAT_PURITY_OF_BODY 124             // monk
#define FEAT_WHOLENESS_OF_BODY 125          // monk
#define FEAT_DIAMOND_BODY 126               // monk
#define FEAT_GREATER_FLURRY 127             // monk
#define FEAT_ABUNDANT_STEP 128              // monk
#define FEAT_DIAMOND_SOUL 129               // monk
#define FEAT_QUIVERING_PALM 130             // monk
#define FEAT_TIMELESS_BODY 131              // monk
#define FEAT_TONGUE_OF_THE_SUN_AND_MOON 132 // monk
#define FEAT_EMPTY_BODY 133                 // monk
#define FEAT_PERFECT_SELF 134               // monk
#define FEAT_SUMMON_FAMILIAR 135
#define FEAT_TRAPFINDING 136
#define FEAT_WEAPON_FOCUS 137
#define FEAT_HONORABLE_WILL 138
#define FEAT_INFRAVISION 139
#define FEAT_FLURRY_OF_BLOWS 140
#define FEAT_IMPROVED_WEAPON_FINESSE 141
#define FEAT_HALF_BLOOD 142
#define FEAT_UNBREAKABLE_WILL 143
#define FEAT_ACT_OF_FORGETFULNESS 144
#define FEAT_DETECT_GOOD 145
#define FEAT_SMITE_GOOD 146
#define FEAT_AURA_OF_EVIL 147
#define FEAT_DARK_BLESSING 148
#define FEAT_SONG_OF_REVELATION 149
#define FEAT_HALF_ORC_RACIAL_ADJUSTMENT 150 // half orc
#define FEAT_RESISTANCE_TO_ILLUSIONS 151
#define FEAT_ILLUSION_AFFINITY 152
#define FEAT_ARMORED_SPELLCASTING 153
#define FEAT_TINKER_FOCUS 154
#define FEAT_GNOME_RACIAL_ADJUSTMENT 155
#define FEAT_TROLL_REGENERATION 156
#define FEAT_WEAKNESS_TO_FIRE 157
#define FEAT_IMPROVED_TAUNTING 158
#define FEAT_IMPROVED_INSTIGATION 159
#define FEAT_IMPROVED_INTIMIDATION 160
#define FEAT_FAVORED_ENEMY 161
#define FEAT_WILD_EMPATHY 162
#define FEAT_COMBAT_STYLE 163
#define FEAT_ANIMAL_COMPANION 164
#define FEAT_IMPROVED_COMBAT_STYLE 165
#define FEAT_WOODLAND_STRIDE 166
#define FEAT_WEAPON_SPECIALIZATION 167
#define FEAT_SWIFT_TRACKER 168
#define FEAT_COMBAT_STYLE_MASTERY 169
#define FEAT_CAMOUFLAGE 170
#define FEAT_HIDE_IN_PLAIN_SIGHT 171
#define FEAT_NATURE_SENSE 172
#define FEAT_TRACKLESS_STEP 173
#define FEAT_RESIST_NATURES_LURE 174
#define FEAT_WILD_SHAPE 175   // level 4
#define FEAT_WILD_SHAPE_2 176 // level 6
#define FEAT_VENOM_IMMUNITY 177
#define FEAT_WILD_SHAPE_3 178 // level 8
#define FEAT_WILD_SHAPE_4 179 // level 10
#define FEAT_THOUSAND_FACES 180
#define FEAT_WILD_SHAPE_5 181 // level 12
#define FEAT_SAP 182
#define FEAT_GREATER_DISARM 183
#define FEAT_FAVORED_ENEMY_AVAILABLE 184
#define FEAT_CALL_MOUNT 185
#define FEAT_ABLE_LEARNER 186
#define FEAT_EXTEND_RAGE 187
#define FEAT_EXTRA_RAGE 188
#define FEAT_FAST_HEALER 189
#define FEAT_DEFENSIVE_STANCE 190
#define FEAT_MOBILE_DEFENSE 191
#define FEAT_WEAPON_OF_CHOICE 192
#define FEAT_UNSTOPPABLE_STRIKE 193
#define FEAT_INCREASED_MULTIPLIER 194
#define FEAT_CRITICAL_SPECIALIST 195
#define FEAT_SUPERIOR_WEAPON_FOCUS 196
#define FEAT_WHIRLWIND_ATTACK 197
#define FEAT_WEAPON_PROFICIENCY_DRUID 198
#define FEAT_WEAPON_PROFICIENCY_ROGUE 199
#define FEAT_WEAPON_PROFICIENCY_MONK 200
#define FEAT_WEAPON_PROFICIENCY_WIZARD 201
#define FEAT_WEAPON_PROFICIENCY_ELF 202
#define FEAT_ARMOR_PROFICIENCY_SHIELD 203
#define FEAT_SNEAK_ATTACK 204
#define FEAT_EVASION 205
#define FEAT_IMPROVED_EVASION 206
#define FEAT_ACROBATIC 207
#define FEAT_AGILE 208
#define FEAT_ANIMAL_AFFINITY 209
#define FEAT_ATHLETIC 210
#define FEAT_AUGMENT_SUMMONING 211
#define FEAT_COMBAT_EXPERTISE 212
#define FEAT_DECEITFUL 213
#define FEAT_DEFT_HANDS 214
#define FEAT_DIEHARD 215
#define FEAT_DILIGENT 216
#define FEAT_ESCHEW_MATERIALS 217
#define FEAT_EXOTIC_WEAPON_PROFICIENCY 218
#define FEAT_GREATER_SPELL_FOCUS 219
#define FEAT_GREATER_SPELL_PENETRATION 220
#define FEAT_GREATER_TWO_WEAPON_FIGHTING 221
#define FEAT_GREATER_WEAPON_FOCUS 222
#define FEAT_GREATER_WEAPON_SPECIALIZATION 223
#define FEAT_IMPROVED_COUNTERSPELL 224
#define FEAT_IMPROVED_FAMILIAR 225
#define FEAT_IMPROVED_FEINT 226
#define FEAT_IMPROVED_GRAPPLE 227
#define FEAT_IMPROVED_OVERRUN 228
#define FEAT_IMPROVED_PRECISE_SHOT 229
#define FEAT_IMPROVED_SHIELD_PUNCH 230
#define FEAT_IMPROVED_SUNDER 231
#define FEAT_IMPROVED_TURNING 232
#define FEAT_INVESTIGATOR 233
#define FEAT_MAGICAL_APTITUDE 234
#define FEAT_MANYSHOT 235
#define FEAT_NATURAL_SPELL 236
#define FEAT_NEGOTIATOR 237
#define FEAT_NIMBLE_FINGERS 238
#define FEAT_PERSUASIVE 239
#define FEAT_RAPID_RELOAD 240
#define FEAT_SELF_SUFFICIENT 241
#define FEAT_STEALTHY 242
#define FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD 243
#define FEAT_TWO_WEAPON_DEFENSE 244
#define FEAT_WIDEN_SPELL 245
#define FEAT_CRIPPLING_STRIKE 246
#define FEAT_DEFENSIVE_ROLL 247
#define FEAT_OPPORTUNIST 248
#define FEAT_WEAKNESS_TO_ACID 249
#define FEAT_SLIPPERY_MIND 250
#define FEAT_NATURAL_ARMOR_INCREASE 251
#define FEAT_SNATCH_ARROWS 252
#define FEAT_STRENGTH_BOOST 253
#define FEAT_CLAWS_AND_BITE 254
#define FEAT_BREATH_WEAPON 255
#define FEAT_BLINDSENSE 256
#define FEAT_CONSTITUTION_BOOST 257
#define FEAT_INTELLIGENCE_BOOST 258
#define FEAT_WINGS 259
#define FEAT_DRAGON_APOTHEOSIS 260
#define FEAT_CHARISMA_BOOST 261
#define FEAT_SLEEP_PARALYSIS_IMMUNITY 262
#define FEAT_ELEMENTAL_IMMUNITY 263
#define FEAT_BARDIC_MUSIC 264
#define FEAT_BARDIC_KNOWLEDGE 265
#define FEAT_COUNTERSONG 266
#define FEAT_IMBUE_ARROW 267
#define FEAT_ARROW_OF_DEATH 268
#define FEAT_AC_BONUS 269
#define FEAT_FEARLESS_DEFENSE 270
#define FEAT_IMMOBILE_DEFENSE 271
#define FEAT_DR_DEFENSE 272
#define FEAT_RENEWED_DEFENSE 273
#define FEAT_SMASH_DEFENSE 274
#define FEAT_ULTRAVISION 275
#define FEAT_LINGERING_SONG 276
#define FEAT_EXTRA_MUSIC 277
#define FEAT_EXCEPTIONAL_TURNING 278
#define FEAT_IMPROVED_POWER_ATTACK 279
#define FEAT_MONKEY_GRIP 280
#define FEAT_FAST_CRAFTER 281
#define FEAT_PROFICIENT_CRAFTER 282
#define FEAT_PROFICIENT_HARVESTER 283
#define FEAT_SCAVENGE 284
#define FEAT_MASTERWORK_CRAFTING 285
#define FEAT_ELVEN_CRAFTING 286
#define FEAT_DWARVEN_CRAFTING 287
#define FEAT_BRANDING 288
#define FEAT_DRACONIC_CRAFTING 289
#define FEAT_LEARNED_CRAFTER 290
#define FEAT_POISON_USE 291
#define FEAT_DEATH_ATTACK 292 // assassin
#define FEAT_POISON_SAVE_BONUS 293
#define FEAT_GREAT_STRENGTH 294
#define FEAT_GREAT_DEXTERITY 295
#define FEAT_GREAT_CONSTITUTION 296
#define FEAT_GREAT_WISDOM 297
#define FEAT_GREAT_INTELLIGENCE 298
#define FEAT_GREAT_CHARISMA 299
#define FEAT_ARMOR_SKIN 300
#define FEAT_FAST_HEALING 301
#define FEAT_FASTER_MEMORIZATION 302
#define FEAT_EMPOWERED_MAGIC 303
#define FEAT_ENHANCED_SPELL_DAMAGE 304
#define FEAT_ENHANCE_SPELL 305
#define FEAT_GREAT_SMITING 306
#define FEAT_DIVINE_MIGHT 307
#define FEAT_DIVINE_SHIELD 308
#define FEAT_DIVINE_VENGEANCE 309
#define FEAT_PERFECT_TWO_WEAPON_FIGHTING 310
#define FEAT_EPIC_DODGE 311
#define FEAT_IMPROVED_SNEAK_ATTACK 312
#define FEAT_SHRUG_DAMAGE 313
#define FEAT_HASTE 314
#define FEAT_DEITY_WEAPON_PROFICIENCY 315
#define FEAT_ENERGY_RESISTANCE 316
#define FEAT_EPIC_SKILL_FOCUS 317
#define FEAT_EPIC_SPELLCASTING 318
#define FEAT_POWER_CRITICAL 319
#define FEAT_IMPROVED_NATURAL_WEAPON 320
#define FEAT_BONE_ARMOR 321
#define FEAT_ANIMATE_DEAD 322
#define FEAT_UNDEAD_FAMILIAR 323
#define FEAT_SUMMON_UNDEAD 324
#define FEAT_SUMMON_GREATER_UNDEAD 325
#define FEAT_TOUCH_OF_UNDEATH 326
#define FEAT_ESSENCE_OF_UNDEATH 327
#define FEAT_DIVINE_BOND 328
#define FEAT_COMBAT_CHALLENGE 329
#define FEAT_IMPROVED_COMBAT_CHALLENGE 330
#define FEAT_GREATER_COMBAT_CHALLENGE 331
#define FEAT_EPIC_COMBAT_CHALLENGE 332
#define FEAT_BLEEDING_ATTACK 333 // assassin ?
#define FEAT_POWERFUL_SNEAK 334
#define FEAT_ARMOR_SPECIALIZATION_LIGHT 335
#define FEAT_ARMOR_SPECIALIZATION_MEDIUM 336
#define FEAT_ARMOR_SPECIALIZATION_HEAVY 337
#define FEAT_WEAPON_MASTERY 338
#define FEAT_WEAPON_FLURRY 339
#define FEAT_WEAPON_SUPREMACY 340
#define FEAT_ROBILARS_GAMBIT 341
#define FEAT_KNOCKDOWN 342
#define FEAT_EPIC_TOUGHNESS 343
#define FEAT_AUTOMATIC_QUICKEN_SPELL 344
#define FEAT_ENHANCE_ARROW_MAGIC 345
#define FEAT_ENHANCE_ARROW_ELEMENTAL 346
#define FEAT_ENHANCE_ARROW_ELEMENTAL_BURST 347
#define FEAT_ENHANCE_ARROW_ALIGNED 348
#define FEAT_ENHANCE_ARROW_DISTANCE 349
#define FEAT_INTENSIFY_SPELL 350
#define FEAT_SNEAK_ATTACK_OF_OPPORTUNITY 351
#define FEAT_STEADFAST_DETERMINATION 352
#define FEAT_BACKSTAB 353
#define FEAT_SELF_CONCEALMENT 354
#define FEAT_SWARM_OF_ARROWS 355
#define FEAT_EPIC_PROWESS 356
#define FEAT_PARRY 357
#define FEAT_RIPOSTE 358
#define FEAT_NO_RETREAT 359
#define FEAT_CRIPPLING_CRITICAL 360
#define FEAT_DRAGON_MOUNT_BOOST 361
#define FEAT_DRAGON_MOUNT_BREATH 362
#define FEAT_SACRED_FLAMES 363 /* sacred fist */
#define FEAT_FINANCIAL_EXPERT 364
#define FEAT_THEORY_TO_PRACTICE 365
#define FEAT_RUTHLESS_NEGOTIATOR 366
#define FEAT_CRYSTAL_FIST 367
#define FEAT_CRYSTAL_BODY 368
#define FEAT_IMPROVED_SPELL_RESISTANCE 369
#define FEAT_SHIELD_CHARGE 370
#define FEAT_SHIELD_SLAM 371
#define FEAT_SPELLBATTLE 372
#define FEAT_APPLY_POISON 373
#define FEAT_DIRT_KICK 374
#define FEAT_INDOMITABLE_RAGE 375
/* rage powers (1st batch) */
#define FEAT_RP_SURPRISE_ACCURACY 376
#define FEAT_RP_POWERFUL_BLOW 377
#define FEAT_RP_RENEWED_VIGOR 378
#define FEAT_RP_HEAVY_SHRUG 379
#define FEAT_RP_FEARLESS_RAGE 380
#define FEAT_RP_COME_AND_GET_ME 381
/* end rage powers */
#define FEAT_DUAL_WEAPON_FIGHTING 382
#define FEAT_IMPROVED_DUAL_WEAPON_FIGHTING 383
#define FEAT_GREATER_DUAL_WEAPON_FIGHTING 384
#define FEAT_PERFECT_DUAL_WEAPON_FIGHTING 385
#define FEAT_EPIC_MANYSHOT 386
#define FEAT_BLINDING_SPEED 387
#define FEAT_VANISH 388
#define FEAT_IMPROVED_VANISH 389
#define FEAT_WEAPON_PROFICIENCY_BARD 390
#define FEAT_RAGING_CRITICAL 391
#define FEAT_EATER_OF_MAGIC 392
#define FEAT_RAGE_RESISTANCE 393
#define FEAT_DEATHLESS_FRENZY 394
#define FEAT_KEEN_STRIKE 395
#define FEAT_OUTSIDER 396
#define FEAT_ARMOR_TRAINING 397
#define FEAT_WEAPON_TRAINING 398
#define FEAT_ARMOR_MASTERY 399
#define FEAT_WEAPON_MASTERY_2 400
#define FEAT_ARMOR_MASTERY_2 401
#define FEAT_STALWART_WARRIOR 402
#define FEAT_EPIC_WEAPON_SPECIALIZATION 403
#define FEAT_LIGHTNING_ARC 404
#define FEAT_ACID_DART 405
#define FEAT_FIRE_BOLT 406
#define FEAT_ICICLE 407
#define FEAT_DOMAIN_ELECTRIC_RESIST 408
#define FEAT_DOMAIN_ACID_RESIST 409
#define FEAT_DOMAIN_FIRE_RESIST 410
#define FEAT_DOMAIN_COLD_RESIST 411
#define FEAT_GLORIOUS_RIDER 412
#define FEAT_LEGENDARY_RIDER 413
#define FEAT_EPIC_MOUNT 414
#define FEAT_BANE_OF_ENEMIES 415
#define FEAT_EPIC_FAVORED_ENEMY 416
#define FEAT_QUICK_CHANT 417
#define FEAT_MUMMY_DUST 418
#define FEAT_DRAGON_KNIGHT 419
#define FEAT_GREATER_RUIN 420
#define FEAT_HELLBALL 421
#define FEAT_EPIC_MAGE_ARMOR 422
#define FEAT_EPIC_WARDING 423
#define FEAT_CHAOTIC_WEAPON 424
#define FEAT_CURSE_TOUCH 425
#define FEAT_DESTRUCTIVE_SMITE 426
#define FEAT_DESTRUCTIVE_AURA 427
#define FEAT_EVIL_TOUCH 428
#define FEAT_EVIL_SCYTHE 429
#define FEAT_GOOD_TOUCH 430
#define FEAT_GOOD_LANCE 431
#define FEAT_HEALING_TOUCH 432
#define FEAT_EMPOWERED_HEALING 433
#define FEAT_KNOWLEDGE 434
#define FEAT_EYE_OF_KNOWLEDGE 435
#define FEAT_BLESSED_TOUCH 436
#define FEAT_LAWFUL_WEAPON 437
#define FEAT_DECEPTION 438
#define FEAT_COPYCAT 439
#define FEAT_MASS_INVIS 440
#define FEAT_RESISTANCE 441
#define FEAT_SAVES 442
#define FEAT_AURA_OF_PROTECTION 443
#define FEAT_ETH_SHIFT 444
#define FEAT_BATTLE_RAGE 445
#define FEAT_WEAPON_EXPERT 446
#define FEAT_SONG_OF_DRAGONS 447
#define FEAT_STRONG_AGAINST_POISON 448
#define FEAT_STRONG_AGAINST_DISEASE 449
#define FEAT_HALF_TROLL_RACIAL_ADJUSTMENT 450
#define FEAT_SPELL_VULNERABILITY 451
#define FEAT_ENCHANTMENT_VULNERABILITY 452
#define FEAT_PHYSICAL_VULNERABILITY 453
#define FEAT_MAGICAL_HERITAGE 454
#define FEAT_ARCANA_GOLEM_RACIAL_ADJUSTMENT 455
#define FEAT_VITAL 456
#define FEAT_HARDY 457
#define FEAT_CRYSTAL_SKIN 458
#define FEAT_LAST_WORD 459
#define FEAT_LIMITLESS_SHAPES 460
#define FEAT_CRYSTAL_DWARF_RACIAL_ADJUSTMENT 461
#define FEAT_VULNERABLE_TO_COLD 462
#define FEAT_TRELUX_EXOSKELETON 463
#define FEAT_LEAP 464
#define FEAT_TRELUX_EQ 465
#define FEAT_TRELUX_PINCERS 466
#define FEAT_TRELUX_RACIAL_ADJUSTMENT 467
#define FEAT_NATURAL_ATTACK 468
#define FEAT_SHIFTER_SHAPES_1 469
#define FEAT_SHIFTER_SHAPES_2 470
#define FEAT_SHIFTER_SHAPES_3 471
#define FEAT_SHIFTER_SHAPES_4 472
#define FEAT_SHIFTER_SHAPES_5 473
/* cleric circle */
#define FEAT_CLERIC_1ST_CIRCLE 474
#define FEAT_CLERIC_2ND_CIRCLE 475
#define FEAT_CLERIC_3RD_CIRCLE 476
#define FEAT_CLERIC_4TH_CIRCLE 477
#define FEAT_CLERIC_5TH_CIRCLE 478
#define FEAT_CLERIC_6TH_CIRCLE 479
#define FEAT_CLERIC_7TH_CIRCLE 480
#define FEAT_CLERIC_8TH_CIRCLE 481
#define FEAT_CLERIC_9TH_CIRCLE 482
#define FEAT_CLERIC_EPIC_SPELL 483
/* wizard circle */
#define FEAT_WIZARD_1ST_CIRCLE 484
#define FEAT_WIZARD_2ND_CIRCLE 485
#define FEAT_WIZARD_3RD_CIRCLE 486
#define FEAT_WIZARD_4TH_CIRCLE 487
#define FEAT_WIZARD_5TH_CIRCLE 488
#define FEAT_WIZARD_6TH_CIRCLE 489
#define FEAT_WIZARD_7TH_CIRCLE 490
#define FEAT_WIZARD_8TH_CIRCLE 491
#define FEAT_WIZARD_9TH_CIRCLE 492
#define FEAT_WIZARD_EPIC_SPELL 493
/**/
#define FEAT_WEAPON_PROFICIENCY_DROW 494 // drow
#define FEAT_DROW_RACIAL_ADJUSTMENT 495  // drow
#define FEAT_DROW_SPELL_RESISTANCE 496   // drow
#define FEAT_SLA_FAERIE_FIRE 497         // drow, spell-like ability
#define FEAT_SLA_LEVITATE 498            // drow, spell-like ability
#define FEAT_SLA_DARKNESS 499            // drow, spell-like ability
#define FEAT_LIGHT_BLINDNESS 500         // underdark/underworld racial disadvantage
/* druid circle */
#define FEAT_DRUID_1ST_CIRCLE 501
#define FEAT_DRUID_2ND_CIRCLE 502
#define FEAT_DRUID_3RD_CIRCLE 503
#define FEAT_DRUID_4TH_CIRCLE 504
#define FEAT_DRUID_5TH_CIRCLE 505
#define FEAT_DRUID_6TH_CIRCLE 506
#define FEAT_DRUID_7TH_CIRCLE 507
#define FEAT_DRUID_8TH_CIRCLE 508
#define FEAT_DRUID_9TH_CIRCLE 509
#define FEAT_DRUID_EPIC_SPELL 510
/* sorcerer circle */
#define FEAT_SORCERER_1ST_CIRCLE 511
#define FEAT_SORCERER_2ND_CIRCLE 512
#define FEAT_SORCERER_3RD_CIRCLE 513
#define FEAT_SORCERER_4TH_CIRCLE 514
#define FEAT_SORCERER_5TH_CIRCLE 515
#define FEAT_SORCERER_6TH_CIRCLE 516
#define FEAT_SORCERER_7TH_CIRCLE 517
#define FEAT_SORCERER_8TH_CIRCLE 518
#define FEAT_SORCERER_9TH_CIRCLE 519
#define FEAT_SORCERER_EPIC_SPELL 520
/* paladin circle */
#define FEAT_PALADIN_1ST_CIRCLE 521
#define FEAT_PALADIN_2ND_CIRCLE 522
#define FEAT_PALADIN_3RD_CIRCLE 523
#define FEAT_PALADIN_4TH_CIRCLE 524
/* ranger circle */
#define FEAT_RANGER_1ST_CIRCLE 525
#define FEAT_RANGER_2ND_CIRCLE 526
#define FEAT_RANGER_3RD_CIRCLE 527
#define FEAT_RANGER_4TH_CIRCLE 528
/* bard circle */
#define FEAT_BARD_1ST_CIRCLE 529
#define FEAT_BARD_2ND_CIRCLE 530
#define FEAT_BARD_3RD_CIRCLE 531
#define FEAT_BARD_4TH_CIRCLE 532
#define FEAT_BARD_5TH_CIRCLE 533
#define FEAT_BARD_6TH_CIRCLE 534
#define FEAT_BARD_EPIC_SPELL 535
/* cleric slots [MUST BE KEPT TOGETHER] */
#define FEAT_CLERIC_1ST_CIRCLE_SLOT 536
#define CLR_SLT_0 (FEAT_CLERIC_1ST_CIRCLE_SLOT - 1)
#define FEAT_CLERIC_2ND_CIRCLE_SLOT 537
#define FEAT_CLERIC_3RD_CIRCLE_SLOT 538
#define FEAT_CLERIC_4TH_CIRCLE_SLOT 539
#define FEAT_CLERIC_5TH_CIRCLE_SLOT 540
#define FEAT_CLERIC_6TH_CIRCLE_SLOT 541
#define FEAT_CLERIC_7TH_CIRCLE_SLOT 542
#define FEAT_CLERIC_8TH_CIRCLE_SLOT 543
#define FEAT_CLERIC_9TH_CIRCLE_SLOT 544
#define FEAT_CLERIC_EPIC_SPELL_SLOT 545
/* wizard slots [MUST BE KEPT TOGETHER] */
/*marker for slot-assignment, must match 1st slot */
#define FEAT_WIZARD_1ST_CIRCLE_SLOT 546
#define WIZ_SLT_0 (FEAT_WIZARD_1ST_CIRCLE_SLOT - 1)
#define FEAT_WIZARD_2ND_CIRCLE_SLOT 547
#define FEAT_WIZARD_3RD_CIRCLE_SLOT 548
#define FEAT_WIZARD_4TH_CIRCLE_SLOT 549
#define FEAT_WIZARD_5TH_CIRCLE_SLOT 550
#define FEAT_WIZARD_6TH_CIRCLE_SLOT 551
#define FEAT_WIZARD_7TH_CIRCLE_SLOT 552
#define FEAT_WIZARD_8TH_CIRCLE_SLOT 553
#define FEAT_WIZARD_9TH_CIRCLE_SLOT 554
#define FEAT_WIZARD_EPIC_SPELL_SLOT 555
/* druid slots [MUST BE KEPT TOGETHER] */
#define FEAT_DRUID_1ST_CIRCLE_SLOT 556
#define DRD_SLT_0 (FEAT_DRUID_1ST_CIRCLE_SLOT - 1)
#define FEAT_DRUID_2ND_CIRCLE_SLOT 557
#define FEAT_DRUID_3RD_CIRCLE_SLOT 558
#define FEAT_DRUID_4TH_CIRCLE_SLOT 559
#define FEAT_DRUID_5TH_CIRCLE_SLOT 560
#define FEAT_DRUID_6TH_CIRCLE_SLOT 561
#define FEAT_DRUID_7TH_CIRCLE_SLOT 562
#define FEAT_DRUID_8TH_CIRCLE_SLOT 563
#define FEAT_DRUID_9TH_CIRCLE_SLOT 564
#define FEAT_DRUID_EPIC_SPELL_SLOT 565
/* sorcerer slots [MUST BE KEPT TOGETHER] */
#define FEAT_SORCERER_1ST_CIRCLE_SLOT 566
#define SRC_SLT_0 (FEAT_SORCERER_1ST_CIRCLE_SLOT - 1)
#define FEAT_SORCERER_2ND_CIRCLE_SLOT 567
#define FEAT_SORCERER_3RD_CIRCLE_SLOT 568
#define FEAT_SORCERER_4TH_CIRCLE_SLOT 569
#define FEAT_SORCERER_5TH_CIRCLE_SLOT 570
#define FEAT_SORCERER_6TH_CIRCLE_SLOT 571
#define FEAT_SORCERER_7TH_CIRCLE_SLOT 572
#define FEAT_SORCERER_8TH_CIRCLE_SLOT 573
#define FEAT_SORCERER_9TH_CIRCLE_SLOT 574
#define FEAT_SORCERER_EPIC_SPELL_SLOT 575
/* paladin slots [MUST BE KEPT TOGETHER] */
#define FEAT_PALADIN_1ST_CIRCLE_SLOT 576
#define PLD_SLT_0 (FEAT_PALADIN_1ST_CIRCLE_SLOT - 1)
#define FEAT_PALADIN_2ND_CIRCLE_SLOT 577
#define FEAT_PALADIN_3RD_CIRCLE_SLOT 578
#define FEAT_PALADIN_4TH_CIRCLE_SLOT 579
/* ranger slots [MUST BE KEPT TOGETHER] */
#define FEAT_RANGER_1ST_CIRCLE_SLOT 580
#define RNG_SLT_0 (FEAT_RANGER_1ST_CIRCLE_SLOT - 1)
#define FEAT_RANGER_2ND_CIRCLE_SLOT 581
#define FEAT_RANGER_3RD_CIRCLE_SLOT 582
#define FEAT_RANGER_4TH_CIRCLE_SLOT 583
/* bard slots [MUST BE KEPT TOGETHER] */
#define FEAT_BARD_1ST_CIRCLE_SLOT 584
#define BRD_SLT_0 (FEAT_BARD_1ST_CIRCLE_SLOT - 1)
#define FEAT_BARD_2ND_CIRCLE_SLOT 585
#define FEAT_BARD_3RD_CIRCLE_SLOT 586
#define FEAT_BARD_4TH_CIRCLE_SLOT 587
#define FEAT_BARD_5TH_CIRCLE_SLOT 588
#define FEAT_BARD_6TH_CIRCLE_SLOT 589
#define FEAT_BARD_EPIC_SPELL_SLOT 590
/* sorcerer bloodlines (1st batch) */
#define FEAT_SORCERER_BLOODLINE_DRACONIC 591
#define FEAT_DRACONIC_HERITAGE_CLAWS 592
#define FEAT_DRACONIC_HERITAGE_DRAGON_RESISTANCES 593
#define FEAT_DRACONIC_HERITAGE_BREATHWEAPON 594
#define FEAT_DRACONIC_HERITAGE_WINGS 595
#define FEAT_DRACONIC_HERITAGE_POWER_OF_WYRMS 596
#define FEAT_DRACONIC_BLOODLINE_ARCANA 597
#define FEAT_SORCERER_BLOODLINE_ARCANE 598
#define FEAT_ARCANE_BLOODLINE_ARCANA 599
#define FEAT_METAMAGIC_ADEPT 600
#define FEAT_NEW_ARCANA 601
#define FEAT_SCHOOL_POWER 602
#define FEAT_ARCANE_APOTHEOSIS 603
/* Mystic theurge */
#define FEAT_THEURGE_SPELLCASTING 604
/* Alchemist */
#define FEAT_CONCOCT_LVL_1 605
#define ALC_SLT_0 (FEAT_CONCOCT_LVL_1 - 1)
#define FEAT_CONCOCT_LVL_2 606
#define FEAT_CONCOCT_LVL_3 607
#define FEAT_CONCOCT_LVL_4 608
#define FEAT_CONCOCT_LVL_5 609
#define FEAT_CONCOCT_LVL_6 610
#define FEAT_MUTAGEN 611
#define FEAT_BOMBS 612
#define FEAT_ALCHEMICAL_DISCOVERY 613
#define FEAT_SWIFT_POISONING 614
#define FEAT_SWIFT_ALCHEMY 615
#define FEAT_POISON_IMMUNITY 616
#define FEAT_PERSISTENT_MUTAGEN 617
#define FEAT_INSTANT_ALCHEMY 618
#define FEAT_GRAND_ALCHEMICAL_DISCOVERY 619
#define FEAT_PSYCHOKINETIC 620
#define FEAT_CURING_TOUCH 621
#define FEAT_LUCK_OF_HEROES 622
/* arcane shadow (trickster) */
#define FEAT_IMPROMPTU_SNEAK_ATTACK 623
#define FEAT_INVISIBLE_ROGUE 624
#define FEAT_MAGICAL_AMBUSH 625
#define FEAT_SURPRISE_SPELLS 626
/* end arcane shadow */
#define FEAT_WIS_AC_BONUS 627
#define FEAT_LVL_AC_BONUS 628
#define FEAT_INNER_FIRE 629              /* sacred fist */
#define FEAT_INNER_FLAME FEAT_INNER_FIRE /* sacred fist */
// more alchemist
#define FEAT_BOMB_MASTERY 630
// end more alchemist
#define FEAT_EFFICIENT_PERFORMANCE 631
#define FEAT_TRUE_SIGHT 632
#define FEAT_PARALYSIS_IMMUNITY 633
#define FEAT_IRON_GOLEM_IMMUNITY 634
#define FEAT_POISON_BREATH 635
#define FEAT_TAIL_SPIKES 636
#define FEAT_PIXIE_DUST 637
#define FEAT_PIXIE_INVISIBILITY 638
#define FEAT_EFREETI_MAGIC 639
#define FEAT_DRAGON_MAGIC 640
#define FEAT_EPIC_WILDSHAPE 641
#define FEAT_EPIC_SPELL_FOCUS 642
/* duergar */
#define FEAT_STRONG_SPELL_HARDINESS 643
#define FEAT_DUERGAR_RACIAL_ADJUSTMENT 644
#define FEAT_AFFINITY_MOVE_SILENT 645
#define FEAT_AFFINITY_LISTEN 646
#define FEAT_AFFINITY_SPOT 647
#define FEAT_SLA_INVIS 648
#define FEAT_SLA_STRENGTH 649
#define FEAT_SLA_ENLARGE 650
#define FEAT_PHANTASM_RESIST 651
#define FEAT_PARALYSIS_RESIST 652
#define FEAT_SPELL_CRITICAL 653
#define FEAT_DIVERSE_TRAINING 654
#define FEAT_SORCERER_BLOODLINE_FEY 655
#define FEAT_FEY_BLOODLINE_ARCANA 656
#define FEAT_LAUGHING_TOUCH 657
#define FEAT_FLEETING_GLANCE 658
#define FEAT_FEY_MAGIC 659
#define FEAT_SOUL_OF_THE_FEY 660
#define FEAT_SORCERER_BLOODLINE_UNDEAD 661
#define FEAT_UNDEAD_BLOODLINE_ARCANA 662
#define FEAT_GRAVE_TOUCH 663
#define FEAT_DEATHS_GIFT 664
#define FEAT_GRASP_OF_THE_DEAD 665
#define FEAT_INCORPOREAL_FORM 666
#define FEAT_ONE_OF_US 667
#define FEAT_EPIC_SPELL_PENETRATION 668
#define FEAT_BOON_COMPANION 669
#define FEAT_IGNORE_SPELL_FAILURE 670
#define FEAT_CHANNEL_SPELL 671
#define FEAT_MULTIPLE_CHANNEL_SPELL 672
#define FEAT_WEAPON_PROFICIENCY_PSIONICIST 673
#define FEAT_PSIONICIST_1ST_CIRCLE 674
#define FEAT_PSIONICIST_2ND_CIRCLE 675
#define FEAT_PSIONICIST_3RD_CIRCLE 676
#define FEAT_PSIONICIST_4TH_CIRCLE 677
#define FEAT_PSIONICIST_5TH_CIRCLE 678
#define FEAT_PSIONICIST_6TH_CIRCLE 679
#define FEAT_PSIONICIST_7TH_CIRCLE 680
#define FEAT_PSIONICIST_8TH_CIRCLE 681
#define FEAT_PSIONICIST_9TH_CIRCLE 682
#define FEAT_ALIGNED_ATTACK_GOOD 683
#define FEAT_ALIGNED_ATTACK_EVIL 684
#define FEAT_ALIGNED_ATTACK_CHAOS 685
#define FEAT_ALIGNED_ATTACK_LAW 686
#define FEAT_COMBAT_MANIFESTATION 687
#define FEAT_CRITICAL_FOCUS 688
#define FEAT_ELEMENTAL_FOCUS_FIRE 689
#define FEAT_ELEMENTAL_FOCUS_ACID 690
#define FEAT_ELEMENTAL_FOCUS_SOUND 691
#define FEAT_ELEMENTAL_FOCUS_ELECTRICITY 692
#define FEAT_ELEMENTAL_FOCUS_COLD 693
#define FEAT_POWER_PENETRATION 694
#define FEAT_GREATER_POWER_PENETRATION 695
#define FEAT_QUICK_MIND 696
#define FEAT_PSIONIC_RECOVERY 697
#define FEAT_PROFICIENT_PSIONICIST 698
#define FEAT_ENHANCED_POWER_DAMAGE 699
#define FEAT_EXPANDED_KNOWLEDGE 700
#define FEAT_PSIONIC_ENDOWMENT 701
#define FEAT_PSIONIC_FOCUS 702
#define FEAT_EMPOWERED_PSIONICS 703
#define FEAT_EPIC_POWER_PENETRATION 704
#define FEAT_BREACH_POWER_RESISTANCE 705
#define FEAT_DOUBLE_MANIFEST 706
#define FEAT_PERPETUAL_FORESIGHT 707
#define FEAT_PROFICIENT_AUGMENTING 708
#define FEAT_EXPERT_AUGMENTING 709
#define FEAT_MASTER_AUGMENTING 710
#define FEAT_SHADOW_ILLUSION 711
#define FEAT_SUMMON_SHADOW 712
#define FEAT_SHADOW_CALL 713
#define FEAT_SHADOW_JUMP 714
#define FEAT_SHADOW_POWER 715
#define FEAT_SHADOW_MASTER 716
#define FEAT_WEAPON_PROFICIENCY_SHADOWDANCER 717
#define FEAT_AURA_OF_RESOLVE 718
#define FEAT_AURA_OF_JUSTICE 719
#define FEAT_AURA_OF_FAITH 720
#define FEAT_AURA_OF_RIGHTEOUSNESS 721
#define FEAT_HOLY_CHAMPION 722
#define FEAT_TOUCH_OF_CORRUPTION 723
#define FEAT_UNHOLY_RESILIENCE 724
#define FEAT_AURA_OF_COWARDICE 725
#define FEAT_CRUELTY 726
#define FEAT_PLAGUE_BRINGER 727
#define FEAT_COMMAND_UNDEAD 728
#define FEAT_FIENDISH_BOON 729
#define FEAT_AURA_OF_DESPAIR 730
#define FEAT_AURA_OF_VENGEANCE 731
#define FEAT_AURA_OF_SIN 732
#define FEAT_AURA_OF_DEPRAVITY 733
#define FEAT_UNHOLY_CHAMPION 734
#define FEAT_BLACKGUARD_1ST_CIRCLE 735
#define FEAT_BLACKGUARD_2ND_CIRCLE 736
#define FEAT_BLACKGUARD_3RD_CIRCLE 737
#define FEAT_BLACKGUARD_4TH_CIRCLE 738
#define FEAT_BLACKGUARD_1ST_CIRCLE_SLOT 739
#define FEAT_BLACKGUARD_2ND_CIRCLE_SLOT 740
#define FEAT_BLACKGUARD_3RD_CIRCLE_SLOT 741
#define FEAT_BLACKGUARD_4TH_CIRCLE_SLOT 742
#define BKG_SLT_0 (FEAT_BLACKGUARD_1ST_CIRCLE_SLOT - 1)
#define FEAT_CHANNEL_ENERGY 743
#define FEAT_HOLY_WARRIOR 744
#define FEAT_UNHOLY_WARRIOR 745
#define FEAT_QUIET_DEATH 797
#define FEAT_SWIFT_DEATH 798
#define FEAT_ANGEL_OF_DEATH 799
#define FEAT_WEAPON_PROFICIENCY_ASSASSIN 800
#define FEAT_HIDDEN_WEAPONS 801
#define FEAT_TRUE_DEATH 802
/*lich*/
#define FEAT_LICH_RACIAL_ADJUSTMENT 803 // lich
#define FEAT_LICH_SPELL_RESIST 804      // lich
#define FEAT_LICH_DAM_RESIST 805        // lich
#define FEAT_LICH_TOUCH 806             // lich
#define FEAT_LICH_REJUV 807             // lich
#define FEAT_LICH_FEAR 808              // lich
/******/
#define FEAT_ELECTRIC_IMMUNITY 809
#define FEAT_COLD_IMMUNITY 810

/**************/
/** reserved above feat# + 1**/
#define FEAT_LAST_FEAT 811
/** FEAT_LAST_FEAT + 1 ***/
#define NUM_FEATS 812
/** absolute cap **/
#define MAX_FEATS 1000
/*****/

/* alchemist */
#define NUM_DISCOVERIES_KNOWN 20
#define MAX_BOMBS_ALLOWED 50
#define NUM_ALC_DISCOVERIES 44
#define NUM_GR_ALC_DISCOVERIES 5

// Paladin Mercies
#define PALADIN_MERCY_NONE 0
#define PALADIN_MERCY_DECEIVED 1
#define PALADIN_MERCY_FATIGUED 2
#define PALADIN_MERCY_SHAKEN 3
#define PALADIN_MERCY_DAZED 4
#define PALADIN_MERCY_ENFEEBLED 5
#define PALADIN_MERCY_STAGGERED 6
#define PALADIN_MERCY_CONFUSED 7
#define PALADIN_MERCY_CURSED 8
#define PALADIN_MERCY_FRIGHTENED 9
#define PALADIN_MERCY_INJURED 10
#define PALADIN_MERCY_NAUSEATED 11
#define PALADIN_MERCY_POISONED 12
#define PALADIN_MERCY_BLINDED 13
#define PALADIN_MERCY_DEAFENED 14
#define PALADIN_MERCY_ENSORCELLED 15
#define PALADIN_MERCY_PARALYZED 16
#define PALADIN_MERCY_STUNNED 17

#define NUM_PALADIN_MERCIES 18

// Blackguard Cruelties
#define BLACKGUARD_CRUELTY_NONE 0
#define BLACKGUARD_CRUELTY_FATIGUED 1
#define BLACKGUARD_CRUELTY_SHAKEN 2
#define BLACKGUARD_CRUELTY_SICKENED 3
#define BLACKGUARD_CRUELTY_DAZED 4
#define BLACKGUARD_CRUELTY_DISEASED 5
#define BLACKGUARD_CRUELTY_STAGGERED 6
#define BLACKGUARD_CRUELTY_CURSED 7
#define BLACKGUARD_CRUELTY_FRIGHTENED 8
#define BLACKGUARD_CRUELTY_NAUSEATED 9
#define BLACKGUARD_CRUELTY_POISONED 10
#define BLACKGUARD_CRUELTY_BLINDED 11
#define BLACKGUARD_CRUELTY_DEAFENED 12
#define BLACKGUARD_CRUELTY_PARALYZED 13
#define BLACKGUARD_CRUELTY_STUNNED 14

#define NUM_BLACKGUARD_CRUELTIES 15

// Blackguard fiendish boons
#define FIENDISH_BOON_NONE 0
#define FIENDISH_BOON_FLAMING 1
#define FIENDISH_BOON_KEEN 2
#define FIENDISH_BOON_VICIOUS 3
#define FIENDISH_BOON_ANARCHIC 4
#define FIENDISH_BOON_FLAMING_BURST 5
#define FIENDISH_BOON_UNHOLY 6
#define FIENDISH_BOON_WOUNDING 7
#define FIENDISH_BOON_SPEED 8
#define FIENDISH_BOON_VORPAL 9

#define NUM_FIENDISH_BOONS 10

#define CHANNEL_ENERGY_TYPE_NONE 0
#define CHANNEL_ENERGY_TYPE_POSITIVE 1
#define CHANNEL_ENERGY_TYPE_NEGATIVE 2

/* Combat feats that apply to a specific weapon type */
#define CFEAT_IMPROVED_CRITICAL 0
#define CFEAT_WEAPON_FINESSE 1
#define CFEAT_WEAPON_FOCUS 2
#define CFEAT_WEAPON_SPECIALIZATION 3
#define CFEAT_GREATER_WEAPON_FOCUS 4
#define CFEAT_GREATER_WEAPON_SPECIALIZATION 5
#define CFEAT_IMPROVED_WEAPON_FINESSE 6
#define CFEAT_SKILL_FOCUS 7
#define CFEAT_EXOTIC_WEAPON_PROFICIENCY 8
#define CFEAT_MONKEY_GRIP 9
#define CFEAT_FAVORED_ENEMY 10
#define CFEAT_EPIC_SKILL_FOCUS 11
#define CFEAT_POWER_CRITICAL 12
#define CFEAT_WEAPON_MASTERY 13
#define CFEAT_WEAPON_FLURRY 14
#define CFEAT_WEAPON_SUPREMACY 15
#define CFEAT_TRIPLE_CRIT 16
#define CFEAT_EPIC_WEAPON_SPECIALIZATION 17
/**/
#define NUM_CFEATS 18
/**/

/* Spell feats that apply to a specific school of spells */
#define SFEAT_SPELL_FOCUS 0
#define SFEAT_GREATER_SPELL_FOCUS 1
#define SFEAT_EPIC_SPELL_FOCUS 1
#define NUM_SFEATS 3

// Skill feats that apply to a specific skill
#define SKFEAT_SKILL_FOCUS 0
#define SKFEAT_EPIC_SKILL_FOCUS 1
#define NUM_SKFEATS 2 /* if this is changed, load_skill_focus() must be modified */

// Sorcerer bloodline feats
#define BLFEAT_DRACONIC 0
#define NUM_BLFEATS 1

/* object-related defines */
/* Item types: used by obj_data.obj_flags.type_flag */
/* make sure to add to - display_item_object_values() */
#define ITEM_LIGHT 1           /**< Item is a light source */
#define ITEM_SCROLL 2          /**< Item is a scroll */
#define ITEM_WAND 3            /**< Item is a wand */
#define ITEM_STAFF 4           /**< Item is a staff	*/
#define ITEM_WEAPON 5          /**< Item is a weapon */
#define ITEM_FURNITURE 6       /**< Sittable Furniture */
#define ITEM_FIREWEAPON 7      /* deprecated - ranged weapon */
#define ITEM_TREASURE 8        /**< Item is a treasure, not gold */
#define ITEM_ARMOR 9           /**< Item is armor */
#define ITEM_POTION 10         /**< Item is a potion */
#define ITEM_WORN 11           /**< General worn item */
#define ITEM_OTHER 12          /**< Misc object */
#define ITEM_TRASH 13          /**< Trash - shopkeepers won't buy */
#define ITEM_MISSILE 14        /* missile/ammo (for ranged weapon) */
#define ITEM_CONTAINER 15      /**< Item is a container */
#define ITEM_NOTE 16           /**< Item is note */
#define ITEM_DRINKCON 17       /**< Item is a drink container */
#define ITEM_KEY 18            /**< Item is a key */
#define ITEM_FOOD 19           /**< Item is food */
#define ITEM_MONEY 20          /**< Item is money (gold) */
#define ITEM_PEN 21            /**< Item is a pen */
#define ITEM_BOAT 22           /**< Item is a boat */
#define ITEM_FOUNTAIN 23       /**< Item is a fountain */
#define ITEM_CLANARMOR 24      /**< Item is clan armor */
#define ITEM_CRYSTAL 25        /* crafting crystal */
#define ITEM_ESSENCE 26        /* component for crafting */
#define ITEM_MATERIAL 27       /* material for crafting */
#define ITEM_SPELLBOOK 28      /* spellbook for wizard types */
#define ITEM_PORTAL 29         /* portal between two locations */
#define ITEM_PLANT 30          /* for transport via plants spell */
#define ITEM_TRAP 31           /* traps */
#define ITEM_TELEPORT 32       /* triggers teleport on command */
#define ITEM_POISON 33         /* apply poison to weapon */
#define ITEM_SUMMON 34         /* summons mob on command */
#define ITEM_SWITCH 35         /* activation mechanism */
#define ITEM_AMMO_POUCH 36     /* ammo pouch mechanic for missile weapons */
#define ITEM_PICK 37           /* pick used for opening locks bonus */
#define ITEM_INSTRUMENT 38     /* instrument used for bard song */
#define ITEM_DISGUISE 39       /* disguise kit used for disguise command */
#define ITEM_WALL 40           /* magical wall (like wall of flames spell) */
#define ITEM_BOWL 41           /* bowl for mixing recipes */
#define ITEM_INGREDIENT 42     /* ingredient used with bowl for recipes */
#define ITEM_BLOCKER 43        /* stops movement in direction X */
#define ITEM_WAGON 44          /* used for carrying resources for trade */
#define ITEM_RESOURCE 45       /* used for trade with wagon */
#define ITEM_PET 46            /* object will convert into a mobile follower upon purchase */
#define ITEM_BLUEPRINT 47      /* NewCraft, recipe for crafting item */
#define ITEM_TREASURE_CHEST 48 /* used with the loot command. */
#define ITEM_HUNT_TROPHY 49    // used to mark a hunt target mob
#define ITEM_WEAPON_OIL 50
#define ITEM_GEAR_OUTFIT 51
/* make sure to add to - display_item_object_values() */
#define NUM_ITEM_TYPES 52 /** Total number of item types.*/

/* reference notes on homeland-port */
/* swapped free1 (7) with fireweapon, swapped free2 (14) with missile
#define ITEM_SHIP 28 // travel on oceans -> ITEM_BOAT (22) */

/** Lootboxes / Treaure chests **/
/* quality of items in chest */
#define LOOTBOX_LEVEL_UNDEFINED 0
#define LOOTBOX_LEVEL_MUNDANE 1
#define LOOTBOX_LEVEL_MINOR 2
#define LOOTBOX_LEVEL_TYPICAL 3
#define LOOTBOX_LEVEL_MEDIUM 4
#define LOOTBOX_LEVEL_MAJOR 5
#define LOOTBOX_LEVEL_SUPERIOR 6
/******/
#define NUM_LOOTBOX_LEVELS 7
/******/

/* treasure type for lootbox */
#define LOOTBOX_TYPE_UNDEFINED 0
#define LOOTBOX_TYPE_GENERIC 1
#define LOOTBOX_TYPE_WEAPON 2
#define LOOTBOX_TYPE_ARMOR 3
#define LOOTBOX_TYPE_CONSUMABLE 4
#define LOOTBOX_TYPE_TRINKET 5
#define LOOTBOX_TYPE_GOLD 6
#define LOOTBOX_TYPE_CRYSTAL 7
/******/
#define NUM_LOOTBOX_TYPES 8
/******/

#define OUTFIT_TYPE_WEAPON 1
#define OUTFIT_TYPE_ARMOR_SET 2

#define NUM_OUTFIT_TYPES 2

#define OUTFIT_VAL_TYPE 0
#define OUTFIT_VAL_BONUS 1
#define OUTFIT_VAL_MATERIAL 2
#define OUTFIT_VAL_APPLY_LOC 3
#define OUTFIT_VAL_APPLY_MOD 4
#define OUTFIT_VAL_APPLY_BONUS 5

/* Item profs: used by obj_data.obj_flags.prof_flag
 * constants.c = item_profs */
/* categories */
#define WEAPON_PROFICIENCY 0
#define ARMOR_PROFICIENCY 1
#define SHIELD_PROFICIENCY 2
/* weapons */
#define ITEM_PROF_NONE 0     // no proficiency required
#define ITEM_PROF_MINIMAL 1  //  "Minimal Weapon Proficiency"
#define ITEM_PROF_BASIC 2    //  "Basic Weapon Proficiency"
#define ITEM_PROF_ADVANCED 3 //  "Advanced Weapon Proficiency"
#define ITEM_PROF_MASTER 4   //  "Master Weapon Proficiency"
#define ITEM_PROF_EXOTIC 5   //  "Exotic Weapon Proficiency"
#define NUM_WEAPON_PROFS 6
/* armor */
#define ITEM_PROF_LIGHT_A 6  // light armor prof
#define ITEM_PROF_MEDIUM_A 7 // medium armor prof
#define ITEM_PROF_HEAVY_A 8  // heavy armor prof
#define NUM_ARMOR_PROFS 3 + NUM_WEAPON_PROFS
/* shields */
#define ITEM_PROF_SHIELDS 9    // shield prof
#define ITEM_PROF_T_SHIELDS 10 // tower shield prof
#define NUM_SHIELD_PROFS 2 + NUM_ARMOR_PROFS
/** Total number of item profs.*/
#define NUM_ITEM_PROFS 11

/* Item materials: used by obj_data.obj_flags.material
 * constants.c = material_name
 */
#define MATERIAL_UNDEFINED 0
#define MATERIAL_COTTON 1
#define MATERIAL_LEATHER 2
#define MATERIAL_GLASS 3
#define MATERIAL_GOLD 4
#define MATERIAL_ORGANIC 5
#define MATERIAL_PAPER 6
#define MATERIAL_STEEL 7
#define MATERIAL_WOOD 8
#define MATERIAL_BONE 9
#define MATERIAL_CRYSTAL 10
#define MATERIAL_ETHER 11
#define MATERIAL_ADAMANTINE 12
#define MATERIAL_MITHRIL 13
#define MATERIAL_IRON 14
#define MATERIAL_COPPER 15
#define MATERIAL_CERAMIC 16
#define MATERIAL_SATIN 17
#define MATERIAL_SILK 18
#define MATERIAL_DRAGONHIDE 19
#define MATERIAL_BURLAP 20
#define MATERIAL_VELVET 21
#define MATERIAL_PLATINUM 22
#define MATERIAL_OBSIDIAN 23
#define MATERIAL_WOOL 24
#define MATERIAL_ONYX 25
#define MATERIAL_IVORY 26
#define MATERIAL_BRASS 27
#define MATERIAL_MARBLE 28
#define MATERIAL_BRONZE 29
#define MATERIAL_PEWTER 30
#define MATERIAL_RUBY 31
#define MATERIAL_SAPPHIRE 32
#define MATERIAL_EMERALD 33
#define MATERIAL_GEMSTONE 34
#define MATERIAL_GRANITE 35
#define MATERIAL_STONE 36
#define MATERIAL_ENERGY 37
#define MATERIAL_HEMP 38
#define MATERIAL_DIAMOND 39
#define MATERIAL_EARTH 40
#define MATERIAL_SILVER 41
#define MATERIAL_ALCHEMAL_SILVER 42
#define MATERIAL_COLD_IRON 43
#define MATERIAL_DARKWOOD 44
#define MATERIAL_DRAGONSCALE 45
#define MATERIAL_DRAGONBONE 46
#define MATERIAL_SEA_IVORY 47
/** Total number of item mats.*/
#define NUM_MATERIALS 48

/* Portal types for the portal object */
#define PORTAL_NORMAL 0
#define PORTAL_RANDOM 1
#define PORTAL_CHECKFLAGS 2
#define PORTAL_CLANHALL 3
/****/
#define NUM_PORTAL_TYPES 4

/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE 0        /**< Item can be taken */
#define ITEM_WEAR_FINGER 1      /**< Item can be worn on finger */
#define ITEM_WEAR_NECK 2        /**< Item can be worn around neck */
#define ITEM_WEAR_BODY 3        /**< Item can be worn on body */
#define ITEM_WEAR_HEAD 4        /**< Item can be worn on head */
#define ITEM_WEAR_LEGS 5        /**< Item can be worn on legs */
#define ITEM_WEAR_FEET 6        /**< Item can be worn on feet */
#define ITEM_WEAR_HANDS 7       /**< Item can be worn on hands	*/
#define ITEM_WEAR_ARMS 8        /**< Item can be worn on arms */
#define ITEM_WEAR_SHIELD 9      /**< Item can be used as a shield */
#define ITEM_WEAR_ABOUT 10      /**< Item can be worn about body */
#define ITEM_WEAR_WAIST 11      /**< Item can be worn around waist */
#define ITEM_WEAR_WRIST 12      /**< Item can be worn on wrist */
#define ITEM_WEAR_WIELD 13      /**< Item can be wielded */
#define ITEM_WEAR_HOLD 14       /**< Item can be held */
#define ITEM_WEAR_FACE 15       // item can be worn on face
#define ITEM_WEAR_AMMO_POUCH 16 // item can be used as an ammo pouch
/* unfinished */
#define ITEM_WEAR_EAR 17   // item can be worn on ears
#define ITEM_WEAR_EYES 18  // item can be worn on eyes
#define ITEM_WEAR_BADGE 19 // item can be worn as badge
#define ITEM_WEAR_INSTRUMENT 20
/** Total number of item wears */
#define NUM_ITEM_WEARS 21

/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW 0             /**< Item is glowing */
#define ITEM_HUM 1              /**< Item is humming */
#define ITEM_NORENT 2           /**< Item cannot be rented */
#define ITEM_NODONATE 3         /**< Item cannot be donated */
#define ITEM_NOINVIS 4          /**< Item cannot be made invis	*/
#define ITEM_INVISIBLE 5        /**< Item is invisible */
#define ITEM_MAGIC 6            /**< Item is magical */
#define ITEM_NODROP 7           /**< Item is cursed: can't drop */
#define ITEM_BLESS 8            /**< Item is blessed */
#define ITEM_ANTI_GOOD 9        /**< Not usable by good people	*/
#define ITEM_ANTI_EVIL 10       /**< Not usable by evil people	*/
#define ITEM_ANTI_NEUTRAL 11    /**< Not usable by neutral people */
#define ITEM_ANTI_WIZARD 12     /**< Not usable by wizards */
#define ITEM_ANTI_CLERIC 13     /**< Not usable by clerics */
#define ITEM_ANTI_ROGUE 14      /**< Not usable by rogues */
#define ITEM_ANTI_WARRIOR 15    /**< Not usable by warriors */
#define ITEM_NOSELL 16          /**< Shopkeepers won't touch it */
#define ITEM_QUEST 17           /**< Item is a quest item         */
#define ITEM_ANTI_HUMAN 18      /* Not usable by Humans*/
#define ITEM_ANTI_ELF 19        /* Not usable by Elfs */
#define ITEM_ANTI_DWARF 20      /* Not usable by Dwarf*/
#define ITEM_ANTI_HALF_TROLL 21 /* Not usable by Half Troll */
#define ITEM_ANTI_MONK 22       /**< Not usable by monks */
#define ITEM_ANTI_DRUID 23      // not usable by druid
#define ITEM_MOLD 24
#define ITEM_ANTI_CRYSTAL_DWARF 25 /* Not usable by C Dwarf*/
#define ITEM_ANTI_HALFLING 26      /* Not usable by halfling*/
#define ITEM_ANTI_H_ELF 27         /* Not usable by half elf*/
#define ITEM_ANTI_H_ORC 28         /* Not usable by half orc*/
#define ITEM_ANTI_GNOME 29         /* Not usable by gnome */
#define ITEM_ANTI_BERSERKER 30     /* Not usable by berserker */
#define ITEM_ANTI_TRELUX 31        /* Not usable by trelux */
#define ITEM_ANTI_SORCERER 32
#define ITEM_DECAY 33 /* portal decay */
#define ITEM_ANTI_PALADIN 34
#define ITEM_ANTI_RANGER 35
#define ITEM_ANTI_BARD 36
#define ITEM_ANTI_ARCANA_GOLEM 37
#define ITEM_FLOAT 38
/* unfinished */
#define ITEM_HIDDEN 39    // item is hidden (need to search to find)
#define ITEM_MAGLIGHT 40  // item is continual-lighted
#define ITEM_NOLOCATE 41  // item can not be located via spells
#define ITEM_NOBURN 42    // item can not be disintegrated by spells
#define ITEM_TRANSIENT 43 // item will crumble and fade when dropped
#define ITEM_AUTOPROC 44  // item can be called by proc_update()
/* Flags dealing with special abilities. */
#define ITEM_FLAMING 45 /* Item is ON FIRE! Used to toggle special ability.*/
#define ITEM_FROST 46   /* Item is sheathed in magical FROST! SPECAB toggle. */
#define ITEM_KI_FOCUS 47
#define ITEM_ANTI_WEAPONMASTER 48
/**/
/*more item flags!*/
#define ITEM_ANTI_DROW 49
#define ITEM_MASTERWORK 50
#define ITEM_ANTI_DUERGAR 51
#define ITEM_SEEKING 52
#define ITEM_ADAPTIVE 53
#define ITEM_AGILE 54
#define ITEM_CORROSIVE 55
#define ITEM_DISRUPTION 56
#define ITEM_DEFENDING 57
#define ITEM_VICIOUS 58
#define ITEM_VORPAL 59
#define ITEM_ANTI_LAWFUL 60
#define ITEM_ANTI_CHAOTIC 61
#define ITEM_REQ_WIZARD 62
#define ITEM_REQ_CLERIC 63
#define ITEM_REQ_ROGUE 64
#define ITEM_REQ_WARRIOR 65
#define ITEM_REQ_MONK 66
#define ITEM_REQ_DRUID 67
#define ITEM_REQ_BERSERKER 68
#define ITEM_REQ_SORCERER 69
#define ITEM_REQ_PALADIN 70
#define ITEM_REQ_RANGER 71
#define ITEM_REQ_BARD 72
#define ITEM_REQ_WEAPONMASTER 73
#define ITEM_REQ_ARCANE_ARCHER 74
#define ITEM_REQ_STALWART_DEFENDER 75
#define ITEM_REQ_SHIFTER 76
#define ITEM_REQ_DUELIST 77
#define ITEM_REQ_MYSTIC_THEURGE 78
#define ITEM_REQ_ALCHEMIST 79
#define ITEM_REQ_ARCANE_SHADOW 80
#define ITEM_REQ_SACRED_FIST 81
#define ITEM_REQ_ELDRITCH_KNIGHT 82
#define ITEM_ANTI_ARCANE_ARCHER 83
#define ITEM_ANTI_STALWART_DEFENDER 84
#define ITEM_ANTI_SHIFTER 85
#define ITEM_ANTI_DUELIST 86
#define ITEM_ANTI_MYSTIC_THEURGE 87
#define ITEM_ANTI_ALCHEMIST 88
#define ITEM_ANTI_ARCANE_SHADOW 89
#define ITEM_ANTI_SACRED_FIST 90
#define ITEM_ANTI_ELDRITCH_KNIGHT 91
#define ITEM_SHOCK 92     // for shocking weapons
#define ITEM_ANTI_LICH 93 /* Not usable by lich */
/** Total number of item flags */
#define NUM_ITEM_FLAGS 94

/* homeland-port */
/*
#define ITEM_HIDDEN        (1 << 22)  // item is hidden (need to search to find)
#define ITEM_MAGLIGHT      (1 << 23)  // item is continual-lighted
#define ITEM_NOLOCATE      (1 << 24)  // item can not be located via spells
#define ITEM_NOBURN        (1 << 25)  // item can not be disintegrated by spells
#define ITEM_TRANSIENT     (1 << 26)  // item will crumble and fade when dropped
#define ITEM_AUTOPROC	  (1 << 27)  // item can be called by proc_update()
 */

/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE 0           /**< No effect			*/
#define APPLY_STR 1            /**< Apply to strength		*/
#define APPLY_DEX 2            /**< Apply to dexterity		*/
#define APPLY_INT 3            /**< Apply to intelligence	*/
#define APPLY_WIS 4            /**< Apply to wisdom		*/
#define APPLY_CON 5            /**< Apply to constitution	*/
#define APPLY_CHA 6            /**< Apply to charisma		*/
#define APPLY_CLASS 7          /**< Reserved			*/
#define APPLY_LEVEL 8          /**< Reserved			*/
#define APPLY_AGE 9            /**< Apply to age			*/
#define APPLY_CHAR_WEIGHT 10   /**< Apply to weight		*/
#define APPLY_CHAR_HEIGHT 11   /**< Apply to height		*/
#define APPLY_PSP 12           /**< Apply to max psp		*/
#define APPLY_HIT 13           /**< Apply to max hit points	*/
#define APPLY_MOVE 14          /**< Apply to max move points	*/
#define APPLY_GOLD 15          /**< Reserved			*/
#define APPLY_EXP 16           /**< Reserved			*/
#define APPLY_AC 17            /**< AC (deprecated, used as tags for spells, etc) */
#define APPLY_HITROLL 18       /**< Apply to hitroll		*/
#define APPLY_DAMROLL 19       /**< Apply to damage roll		*/
#define APPLY_SAVING_FORT 20   // save fortitude
#define APPLY_SAVING_REFL 21   // safe reflex
#define APPLY_SAVING_WILL 22   // safe will power
#define APPLY_SAVING_POISON 23 // save poison
#define APPLY_SAVING_DEATH 24  // save death
#define APPLY_SPELL_RES 25     // spell resistance
#define APPLY_SIZE 26          // char size
#define APPLY_AC_NEW 27        // apply to armor class (post conversion)
/* dam_types (resistances/vulnerabilties) */
#define APPLY_RES_FIRE 28 // 1
#define APPLY_RES_COLD 29
#define APPLY_RES_AIR 30
#define APPLY_RES_EARTH 31
#define APPLY_RES_ACID 32 // 5
#define APPLY_RES_HOLY 33
#define APPLY_RES_ELECTRIC 34
#define APPLY_RES_UNHOLY 35
#define APPLY_RES_SLICE 36
#define APPLY_RES_PUNCTURE 37 // 10
#define APPLY_RES_FORCE 38
#define APPLY_RES_SOUND 39
#define APPLY_RES_POISON 40
#define APPLY_RES_DISEASE 41
#define APPLY_RES_NEGATIVE 42 // 15
#define APPLY_RES_ILLUSION 43
#define APPLY_RES_MENTAL 44
#define APPLY_RES_LIGHT 45
#define APPLY_RES_ENERGY 46
#define APPLY_RES_WATER 47 // 20
/* end dam_types, make sure it matches NUM_DAM_TYPES */

#define APPLY_DR 48
#define APPLY_FEAT 49
#define APPLY_SKILL 50
#define APPLY_SPECIAL 51
#define APPLY_POWER_RES 52

/** Total number of applies */
#define NUM_APPLIES 53

// number of award types.  do_award in act.wizard.c
#define NUM_AWARD_TYPES 11

/* Equals the total number of SAVING_* defines in spells.h */
#define NUM_OF_SAVING_THROWS 5

/* Container flags - value[1] */
#define CONT_CLOSEABLE (1 << 0) /**< Container can be closed	*/
#define CONT_PICKPROOF (1 << 1) /**< Container is pickproof	*/
#define CONT_CLOSED (1 << 2)    /**< Container is closed		*/
#define CONT_LOCKED (1 << 3)    /**< Container is locked		*/
#define NUM_CONT_FLAGS 4

/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER 0       /**< Liquid type water */
#define LIQ_BEER 1        /**< Liquid type beer */
#define LIQ_WINE 2        /**< Liquid type wine */
#define LIQ_ALE 3         /**< Liquid type ale */
#define LIQ_DARKALE 4     /**< Liquid type darkale */
#define LIQ_WHISKY 5      /**< Liquid type whisky */
#define LIQ_LEMONADE 6    /**< Liquid type lemonade */
#define LIQ_FIREBRT 7     /**< Liquid type firebrt */
#define LIQ_LOCALSPC 8    /**< Liquid type localspc */
#define LIQ_SLIME 9       /**< Liquid type slime */
#define LIQ_MILK 10       /**< Liquid type milk */
#define LIQ_TEA 11        /**< Liquid type tea */
#define LIQ_COFFE 12      /**< Liquid type coffee */
#define LIQ_BLOOD 13      /**< Liquid type blood */
#define LIQ_SALTWATER 14  /**< Liquid type saltwater */
#define LIQ_CLEARWATER 15 /**< Liquid type clearwater */
/** Total number of liquid types */
#define NUM_LIQ_TYPES 16

/* WEAPON and ARMOR defines */

/* Weapon head types */
#define HEAD_TYPE_UNDEFINED 0
#define HEAD_TYPE_BLADE 1
#define HEAD_TYPE_HEAD 2
#define HEAD_TYPE_POINT 3
#define HEAD_TYPE_BOW 4
#define HEAD_TYPE_POUCH 5
#define HEAD_TYPE_CORD 6
#define HEAD_TYPE_MESH 7
#define HEAD_TYPE_CHAIN 8
#define HEAD_TYPE_FIST 9

#define NUM_WEAPON_HEAD_TYPES 10

/* weapon handle types */
#define HANDLE_TYPE_UNDEFINED 0
#define HANDLE_TYPE_SHAFT 1
#define HANDLE_TYPE_HILT 2
#define HANDLE_TYPE_STRAP 3
#define HANDLE_TYPE_STRING 4
#define HANDLE_TYPE_GRIP 5
#define HANDLE_TYPE_HANDLE 6
#define HANDLE_TYPE_GLOVE 7

#define NUM_WEAPON_HANDLE_TYPES 8

/****************************
 WEAPON FLAGS ******
 ****************************/
#define WEAPON_FLAG_SIMPLE (1 << 0)
#define WEAPON_FLAG_MARTIAL (1 << 1)
#define WEAPON_FLAG_EXOTIC (1 << 2)
#define WEAPON_FLAG_RANGED (1 << 3)
#define WEAPON_FLAG_THROWN (1 << 4)
/* Reach: You use a reach weapon to strike opponents 10 feet away, but you can't
 * use it against an adjacent foe. */
#define WEAPON_FLAG_REACH (1 << 5)
#define WEAPON_FLAG_ENTANGLE (1 << 6)
/* Trip*: You can use a trip weapon to make trip attacks. If you are tripped
 * during your own trip attempt, you can drop the weapon to avoid being tripped
 * (*see FAQ/Errata.) */
#define WEAPON_FLAG_TRIP (1 << 7)
/* Double: You can use a double weapon to fight as if fighting with two weapons,
 * but if you do, you incur all the normal attack penalties associated with
 * fighting with two weapons, just as if you were using a one-handed weapon and
 * a light weapon. You can choose to wield one end of a double weapon two-handed,
 * but it cannot be used as a double weapon when wielded in this way—only one
 * end of the weapon can be used in any given round. */
#define WEAPON_FLAG_DOUBLE (1 << 8)
/* Disarm: When you use a disarm weapon, you get a +2 bonus on Combat Maneuver
 * Checks to disarm an enemy. */
#define WEAPON_FLAG_DISARM (1 << 9)
/* Nonlethal: These weapons deal nonlethal damage (see Combat). */
#define WEAPON_FLAG_NONLETHAL (1 << 10)
#define WEAPON_FLAG_SLOW_RELOAD (1 << 11)
#define WEAPON_FLAG_BALANCED (1 << 12)
#define WEAPON_FLAG_CHARGE (1 << 13)
#define WEAPON_FLAG_REPEATING (1 << 14)
#define WEAPON_FLAG_TWO_HANDED (1 << 15)
#define WEAPON_FLAG_LIGHT (1 << 16)
/* Blocking: When you use this weapon to fight defensively, you gain a +1 shield
 * bonus to AC. Source: Ultimate Combat. */
#define WEAPON_FLAG_BLOCKING (1 << 17)
/* Brace: If you use a readied action to set a brace weapon against a charge,
 * you deal double damage on a successful hit against a charging creature
 * (see Combat). */
#define WEAPON_FLAG_BRACING (1 << 18)
/* Deadly: When you use this weapon to deliver a coup de grace, it gains a +4
 * bonus to damage when calculating the DC of the Fortitude saving throw to see
 * whether the target of the coup de grace dies from the attack. The bonus is
 * not added to the actual damage of the coup de grace attack.
 * Source: Ultimate Combat. */
#define WEAPON_FLAG_DEADLY (1 << 19)
/* Distracting: You gain a +2 bonus on Bluff skill checks to feint in combat
 * while wielding this weapon. Source: Ultimate Combat. */
#define WEAPON_FLAG_DISTRACTING (1 << 20)
/* Fragile: Weapons and armor with the fragile quality cannot take the beating
 * that sturdier weapons can. A fragile weapon gains the broken condition if the
 * wielder rolls a natural 1 on an attack roll with the weapon. If a fragile
 * weapon is already broken, the roll of a natural 1 destroys it instead.
 * Masterwork and magical fragile weapons and armor lack these flaws unless
 * otherwise noted in the item description or the special material description.
 * If a weapon gains the broken condition in this way, that weapon is considered
 * to have taken damage equal to half its hit points +1. This damage is repaired
 * either by something that addresses the effect that granted the weapon the
 * broken condition (like quick clear in the case of firearm misfires or the
 * Field Repair feat) or by the repair methods described in the broken condition.
 * When an effect that grants the broken condition is removed, the weapon
 * regains the hit points it lost when the broken condition was applied. Damage
 * done by an attack against a weapon (such as from a sunder combat maneuver)
 * cannot be repaired by an effect that removes the broken condition.
 * Source: Ultimate Combat.*/
#define WEAPON_FLAG_FRAGILE (1 << 21)
/* Grapple: On a successful critical hit with a weapon of this type, you can
 * grapple the target of the attack. The wielder can then attempt a combat
 * maneuver check to grapple his opponent as a free action. This grapple attempt
 * does not provoke an attack of opportunity from the creature you are
 * attempting to grapple if that creature is not threatening you. While you
 * grapple the target with a grappling weapon, you can only move or damage the
 * creature on your turn. You are still considered grappled, though you do not
 * have to be adjacent to the creature to continue the grapple. If you move far
 * enough away to be out of the weapon’s reach, you end the grapple with that
 * action. Source: Ultimate Combat. */
#define WEAPON_FLAG_GRAPPLING (1 << 22)
/* Performance: When wielding this weapon, if an attack or combat maneuver made
 * with this weapon prompts a combat performance check, you gain a +2 bonus on
 * that check. See Gladiator Weapons below for more information. */
#define WEAPON_FLAG_PERFORMANCE (1 << 23)
/* ***Strength (#): This feature is usually only applied to ranged weapons (such
 * as composite bows). Some weapons function better in the hands of stronger
 * users. All such weapons are made with a particular Strength rating (that is,
 * each requires a minimum Strength modifier to use with proficiency and this
 * number is included in parenthesis). If your Strength bonus is less than the
 * strength rating of the weapon, you can't effectively use it, so you take a –2
 * penalty on attacks with it. For example, the default (lowest form of)
 * composite longbow requires a Strength modifier of +0 or higher to use with
 * proficiency. A weapon with the Strength feature allows you to add your
 * Strength bonus to damage, up to the maximum bonus indicated for the bow. Each
 * point of Strength bonus granted by the bow adds 100 gp to its cost. If you
 * have a penalty for low Strength, apply it to damage rolls when you use a
 * composite longbow. Editor's Note: The "Strength" weapon feature was 'created'
 * by d20pfsrd.com as a shorthand note to the composite bow mechanics. This is
 * not "Paizo" or "official" content. */
#define WEAPON_FLAG_STRENGTH (1 << 24)
/* Sunder: When you use a sunder weapon, you get a +2 bonus on Combat Maneuver
 * Checks to sunder attempts. */
#define WEAPON_FLAG_SUNDER (1 << 25)

/* ----------------- */
#define NUM_WEAPON_FLAGS 26

/*****/

// weapon families
/* *Monk: A monk weapon can be used by a monk to perform a flurry of blows
 * (*see FAQ/Errata.) */
#define WEAPON_FAMILY_MONK 0
#define WEAPON_FAMILY_LIGHT_BLADE 1
#define WEAPON_FAMILY_SMALL_BLADE WEAPON_FAMILY_LIGHT_BLADE
#define WEAPON_FAMILY_WHIP WEAPON_FAMILY_LIGHT_BLADE
#define WEAPON_FAMILY_HAMMER 2
#define WEAPON_FAMILY_CLUB WEAPON_FAMILY_HAMMER
#define WEAPON_FAMILY_FLAIL WEAPON_FAMILY_HAMMER
#define WEAPON_FAMILY_RANGED 3
#define WEAPON_FAMILY_BOW WEAPON_FAMILY_RANGED
#define WEAPON_FAMILY_CROSSBOW WEAPON_FAMILY_RANGED
#define WEAPON_FAMILY_THROWN WEAPON_FAMILY_RANGED
#define WEAPON_FAMILY_HEAVY_BLADE 4
#define WEAPON_FAMILY_MEDIUM_BLADE WEAPON_FAMILY_HEAVY_BLADE
#define WEAPON_FAMILY_LARGE_BLADE WEAPON_FAMILY_HEAVY_BLADE
#define WEAPON_FAMILY_POLEARM 5
#define WEAPON_FAMILY_SPEAR WEAPON_FAMILY_POLEARM
#define WEAPON_FAMILY_DOUBLE 6
#define WEAPON_FAMILY_AXE 7
#define WEAPON_FAMILY_PICK WEAPON_FAMILY_AXE

#define NUM_WEAPON_FAMILIES 8

/* Armor types */
#define ARMOR_TYPE_NONE 0
#define ARMOR_TYPE_LIGHT 1
#define ARMOR_TYPE_MEDIUM 2
#define ARMOR_TYPE_HEAVY 3
#define ARMOR_TYPE_SHIELD 4
#define ARMOR_TYPE_TOWER_SHIELD 5
#define NUM_ARMOR_TYPES 6
#define MAX_ARMOR_TYPES 4 /* unused, created for oedit though */

/* Armor Types */
#define SPEC_ARMOR_TYPE_UNDEFINED 0
#define SPEC_ARMOR_TYPE_CLOTHING 1
#define SPEC_ARMOR_TYPE_PADDED 2
#define SPEC_ARMOR_TYPE_LEATHER 3
#define SPEC_ARMOR_TYPE_STUDDED_LEATHER 4
#define SPEC_ARMOR_TYPE_LIGHT_CHAIN 5
#define SPEC_ARMOR_TYPE_HIDE 6
#define SPEC_ARMOR_TYPE_SCALE 7
#define SPEC_ARMOR_TYPE_CHAINMAIL 8
#define SPEC_ARMOR_TYPE_PIECEMEAL 9
#define SPEC_ARMOR_TYPE_SPLINT 10
#define SPEC_ARMOR_TYPE_BANDED 11
#define SPEC_ARMOR_TYPE_HALF_PLATE 12
#define SPEC_ARMOR_TYPE_FULL_PLATE 13
/**/
#define SPEC_ARMOR_TYPE_BUCKLER 14
#define SPEC_ARMOR_TYPE_SMALL_SHIELD 15
#define SPEC_ARMOR_TYPE_LARGE_SHIELD 16
#define SPEC_ARMOR_TYPE_TOWER_SHIELD 17

#define NUM_SPEC_ARMOR_SUIT_TYPES 18

/**/
/* this is the extension added by zusuk for piecemeal system */
#define SPEC_ARMOR_TYPE_CLOTHING_HEAD 18
#define SPEC_ARMOR_TYPE_PADDED_HEAD 19
#define SPEC_ARMOR_TYPE_LEATHER_HEAD 20
#define SPEC_ARMOR_TYPE_STUDDED_LEATHER_HEAD 21
#define SPEC_ARMOR_TYPE_LIGHT_CHAIN_HEAD 22
#define SPEC_ARMOR_TYPE_HIDE_HEAD 23
#define SPEC_ARMOR_TYPE_SCALE_HEAD 24
#define SPEC_ARMOR_TYPE_CHAINMAIL_HEAD 25
#define SPEC_ARMOR_TYPE_CHAIN_HEAD 26 /* duplicate :( */
#define SPEC_ARMOR_TYPE_PIECEMEAL_HEAD 27
#define SPEC_ARMOR_TYPE_SPLINT_HEAD 28
#define SPEC_ARMOR_TYPE_BANDED_HEAD 29
#define SPEC_ARMOR_TYPE_HALF_PLATE_HEAD 30
#define SPEC_ARMOR_TYPE_FULL_PLATE_HEAD 31
/**/
#define SPEC_ARMOR_TYPE_CLOTHING_ARMS 32
#define SPEC_ARMOR_TYPE_PADDED_ARMS 33
#define SPEC_ARMOR_TYPE_LEATHER_ARMS 34
#define SPEC_ARMOR_TYPE_STUDDED_LEATHER_ARMS 35
#define SPEC_ARMOR_TYPE_LIGHT_CHAIN_ARMS 36
#define SPEC_ARMOR_TYPE_HIDE_ARMS 37
#define SPEC_ARMOR_TYPE_SCALE_ARMS 38
#define SPEC_ARMOR_TYPE_CHAINMAIL_ARMS 39
#define SPEC_ARMOR_TYPE_PIECEMEAL_ARMS 40
#define SPEC_ARMOR_TYPE_SPLINT_ARMS 41
#define SPEC_ARMOR_TYPE_BANDED_ARMS 42
#define SPEC_ARMOR_TYPE_HALF_PLATE_ARMS 43
#define SPEC_ARMOR_TYPE_FULL_PLATE_ARMS 44
/**/
#define SPEC_ARMOR_TYPE_CLOTHING_LEGS 45
#define SPEC_ARMOR_TYPE_PADDED_LEGS 46
#define SPEC_ARMOR_TYPE_LEATHER_LEGS 47
#define SPEC_ARMOR_TYPE_STUDDED_LEATHER_LEGS 48
#define SPEC_ARMOR_TYPE_LIGHT_CHAIN_LEGS 49
#define SPEC_ARMOR_TYPE_HIDE_LEGS 50
#define SPEC_ARMOR_TYPE_SCALE_LEGS 51
#define SPEC_ARMOR_TYPE_CHAINMAIL_LEGS 52
#define SPEC_ARMOR_TYPE_PIECEMEAL_LEGS 53
#define SPEC_ARMOR_TYPE_SPLINT_LEGS 54
#define SPEC_ARMOR_TYPE_BANDED_LEGS 55
#define SPEC_ARMOR_TYPE_HALF_PLATE_LEGS 56
#define SPEC_ARMOR_TYPE_FULL_PLATE_LEGS 57
/***/
/***/
#define NUM_SPEC_ARMOR_TYPES 58
/***/

/* Weapon Types */
#define WEAPON_TYPE_UNDEFINED 0
#define WEAPON_TYPE_UNARMED 1
/* Simple Weapons */
#define WEAPON_TYPE_DAGGER 2
#define WEAPON_TYPE_LIGHT_MACE 3
#define WEAPON_TYPE_SICKLE 4
#define WEAPON_TYPE_CLUB 5
#define WEAPON_TYPE_HEAVY_MACE 6
#define WEAPON_TYPE_MORNINGSTAR 7
#define WEAPON_TYPE_SHORTSPEAR 8
#define WEAPON_TYPE_LONGSPEAR 9
#define WEAPON_TYPE_QUARTERSTAFF 10
#define WEAPON_TYPE_SPEAR 11
/* Ranged - thrown and crossbows */
#define WEAPON_TYPE_HEAVY_CROSSBOW 12
#define WEAPON_TYPE_LIGHT_CROSSBOW 13
#define WEAPON_TYPE_DART 14
#define WEAPON_TYPE_JAVELIN 15
#define WEAPON_TYPE_SLING 16
/* Martial Weapons */
/* Melee */
#define WEAPON_TYPE_THROWING_AXE 17
#define WEAPON_TYPE_LIGHT_HAMMER 18
#define WEAPON_TYPE_HAND_AXE 19
#define WEAPON_TYPE_KUKRI 20
#define WEAPON_TYPE_LIGHT_PICK 21
#define WEAPON_TYPE_SAP 22
#define WEAPON_TYPE_SHORT_SWORD 23
#define WEAPON_TYPE_BATTLE_AXE 24
#define WEAPON_TYPE_FLAIL 25
#define WEAPON_TYPE_LONG_SWORD 26
#define WEAPON_TYPE_HEAVY_PICK 27
#define WEAPON_TYPE_RAPIER 28
#define WEAPON_TYPE_SCIMITAR 29
#define WEAPON_TYPE_TRIDENT 30
#define WEAPON_TYPE_WARHAMMER 31
#define WEAPON_TYPE_FALCHION 32
#define WEAPON_TYPE_GLAIVE 33
#define WEAPON_TYPE_GREAT_AXE 34
#define WEAPON_TYPE_GREAT_CLUB 35
#define WEAPON_TYPE_HEAVY_FLAIL 36
#define WEAPON_TYPE_GREAT_SWORD 37
#define WEAPON_TYPE_GUISARME 38
#define WEAPON_TYPE_HALBERD 39
#define WEAPON_TYPE_LANCE 40
#define WEAPON_TYPE_RANSEUR 41
#define WEAPON_TYPE_SCYTHE 42
/* Ranged */
#define WEAPON_TYPE_LONG_BOW 43
#define WEAPON_TYPE_SHORT_BOW 44
#define WEAPON_TYPE_COMPOSITE_LONGBOW 45
#define WEAPON_TYPE_COMPOSITE_SHORTBOW 46
/* Exotic Weapons */
/* Melee */
#define WEAPON_TYPE_KAMA 47
#define WEAPON_TYPE_NUNCHAKU 48
#define WEAPON_TYPE_SAI 49
#define WEAPON_TYPE_SIANGHAM 50
#define WEAPON_TYPE_BASTARD_SWORD 51
#define WEAPON_TYPE_DWARVEN_WAR_AXE 52
#define WEAPON_TYPE_WHIP 53
#define WEAPON_TYPE_SPIKED_CHAIN 54
/* Double Weapons */
#define WEAPON_TYPE_DOUBLE_AXE 55
#define WEAPON_TYPE_DIRE_FLAIL 56
#define WEAPON_TYPE_HOOKED_HAMMER 57
#define WEAPON_TYPE_2_BLADED_SWORD 58
#define WEAPON_TYPE_DWARVEN_URGOSH 59
/* Ranged */
#define WEAPON_TYPE_HAND_CROSSBOW 60
#define WEAPON_TYPE_HEAVY_REP_XBOW 61
#define WEAPON_TYPE_LIGHT_REP_XBOW 62
/* Ranged */
#define WEAPON_TYPE_BOLA 63
#define WEAPON_TYPE_NET 64
#define WEAPON_TYPE_SHURIKEN 65
/* dextension of composite bows */
#define WEAPON_TYPE_COMPOSITE_LONGBOW_2 66
#define WEAPON_TYPE_COMPOSITE_LONGBOW_3 67
#define WEAPON_TYPE_COMPOSITE_LONGBOW_4 68
#define WEAPON_TYPE_COMPOSITE_LONGBOW_5 69
#define WEAPON_TYPE_COMPOSITE_SHORTBOW_2 70
#define WEAPON_TYPE_COMPOSITE_SHORTBOW_3 71
#define WEAPON_TYPE_COMPOSITE_SHORTBOW_4 72
#define WEAPON_TYPE_COMPOSITE_SHORTBOW_5 73
#define WEAPON_TYPE_WARMAUL 74
#define WEAPON_TYPE_KHOPESH 75
// One higher than last above
#define NUM_WEAPON_TYPES 76

/* different ammo types */
#define AMMO_TYPE_UNDEFINED 0
#define AMMO_TYPE_ARROW 1
#define AMMO_TYPE_BOLT 2
#define AMMO_TYPE_STONE 3
#define AMMO_TYPE_DART 4
/**/
#define NUM_AMMO_TYPES 5
/*************************/

/* Weapon damage types, used in the weapon definitions
 * and to give the TYPE of damage done by the weapon.
 * Some weapons give multiple damage types, while only
 * having one damage message. */
#define DAMAGE_TYPE_BLUDGEONING (1 << 0)
#define DAMAGE_TYPE_SLASHING (1 << 1)
#define DAMAGE_TYPE_PIERCING (1 << 2)
#define DAMAGE_TYPE_NONLETHAL (1 << 3)

#define NUM_WEAPON_DAMAGE_TYPES 4

/* attack types - indicates mode of combat */
#define ATTACK_TYPE_PRIMARY 0
#define ATTACK_TYPE_OFFHAND 1
#define ATTACK_TYPE_RANGED 2
#define ATTACK_TYPE_UNARMED 3
#define ATTACK_TYPE_TWOHAND 4 /* doesn't really serve any purpose */
#define ATTACK_TYPE_BOMB_TOSS 5
#define ATTACK_TYPE_PRIMARY_SNEAK 6 // impromptu sneak attack
#define ATTACK_TYPE_OFFHAND_SNEAK 7 // impromptu sneak attack

/* WEAPON ATTACK TYPES - indicates type of attack both
   armed and unarmed attacks are, example: You BITE Bob.
   This use to be located in spells.h but was moved here
   since it is more globally accessed */
#define TYPE_HIT 700   /* barehand */
#define TYPE_STING 701 /* pierce */
#define TYPE_WHIP 702
#define TYPE_SLASH 703 /* slash */
#define TYPE_BITE 704
#define TYPE_BLUDGEON 705 /* bludgeon */
#define TYPE_CRUSH 706    /* bludgeon */
#define TYPE_POUND 707    /* bludgeon */
#define TYPE_CLAW 708
#define TYPE_MAUL 709
#define TYPE_THRASH 710
#define TYPE_PIERCE 711 /* pierce */
#define TYPE_BLAST 712
#define TYPE_PUNCH 713   /* barehand */
#define TYPE_STAB 714    /* pierce */
#define TYPE_SLICE 715   /* slash */
#define TYPE_THRUST 716  /* pierce */
#define TYPE_HACK 717    /* slash */
#define TYPE_RAKE 718    /* slash? */
#define TYPE_PECK 719    /* pierce? */
#define TYPE_SMASH 720   /* bludgeon? */
#define TYPE_TRAMPLE 721 /* bludgeon? */
#define TYPE_CHARGE 722  /* pierce? */
#define TYPE_GORE 723    /* pierce? */
/* don't forget to add to race_list() and all the other places if changed */
/** The total number of attack types */
#define NUM_ATTACK_TYPES 24
#define BOT_WEAPON_TYPES (TYPE_HIT + NUM_ATTACK_TYPES)
#define TOP_ATTACK_TYPES TYPE_HIT
#define TYPE_UNDEFINED_WTYPE 0
/* not hard coded, but up to 750 */

/* combat maneuver types*/
#define COMBAT_MANEUVER_TYPE_UNDEFINED 0
#define COMBAT_MANEUVER_TYPE_KNOCKDOWN 1
#define COMBAT_MANEUVER_TYPE_KICK 2
#define COMBAT_MANEUVER_TYPE_DISARM 3
#define COMBAT_MANEUVER_TYPE_GRAPPLE 4
#define COMBAT_MANEUVER_TYPE_REVERSAL 5 /* try to reverse grapple */
#define COMBAT_MANEUVER_TYPE_INIT_GRAPPLE 6
#define COMBAT_MANEUVER_TYPE_PIN 7

/* Critical hit types */
#define CRIT_X2 0
#define CRIT_X3 1
#define CRIT_X4 2
#define CRIT_X5 3
#define CRIT_X6 4
#define MAX_CRIT_TYPE CRIT_X6

/* Player conditions */
#define DRUNK 0  /**< Player drunk condition */
#define HUNGER 1 /**< Player hunger condition */
#define THIRST 2 /**< Player thirst condition */

/* Sun state for weather_data */
#define SUN_DARK 0  /**< Night time */
#define SUN_RISE 1  /**< Dawn */
#define SUN_LIGHT 2 /**< Day time */
#define SUN_SET 3   /**< Dusk */

/* Sky conditions for weather_data */
#define SKY_CLOUDLESS 0 /**< Weather = No clouds */
#define SKY_CLOUDY 1    /**< Weather = Cloudy */
#define SKY_RAINING 2   /**< Weather = Rain */
#define SKY_LIGHTNING 3 /**< Weather = Lightning storm */

/* factions */
#define FACTION_NONE 0
#define FACTION_ADVENTURER 0
#define FACTION_ADVENTURERS 0
#define FACTION_FREELANCE 0
#define FACTION_FREELANCERS 0
#define FACTION_THE_ORDER 1
#define FACTION_ORDER 1
#define FACTION_DARKLINGS 2
#define FACTION_DARKLING 2
#define FACTION_CRIMINAL 3
#define NUM_FACTIONS 3

/* Staff Ran Event */
#define STAFF_RAN_EVENTS_VAR 300 /* values saved for staff events on player */

/* Rent codes */
#define RENT_UNDEF 0    /**< Character inv save status = undefined */
#define RENT_CRASH 1    /**< Character inv save status = game crash */
#define RENT_RENTED 2   /**< Character inv save status = rented */
#define RENT_CRYO 3     /**< Character inv save status = cryogenics */
#define RENT_FORCED 4   /**< Character inv save status = forced rent */
#define RENT_TIMEDOUT 5 /**< Character inv save status = timed out */

/* Settings for Bit Vectors */
#define RF_ARRAY_MAX 4 /**< # Bytes in Bit vector - Room flags */
#define PM_ARRAY_MAX 4 /**< # Bytes in Bit vector - Act and Player flags */
#define PR_ARRAY_MAX 4 /**< # Bytes in Bit vector - Player Pref Flags */
#define AF_ARRAY_MAX 4 /**< # Bytes in Bit vector - Affect flags */
#define TW_ARRAY_MAX 4 /**< # Bytes in Bit vector - Obj Wear Locations */
#define EF_ARRAY_MAX 4 /**< # Bytes in Bit vector - Obj Extra Flags */
#define ZN_ARRAY_MAX 4 /**< # Bytes in Bit vector - Zone Flags */
#define FT_ARRAY_MAX 4 /**< # Bytes in Bit vector - Feat Flags */

/* other #defined constants */
/* **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for details.
 *
 * LVL_IMPL should always be the HIGHEST possible immortal level, and
 * LVL_IMMORT should always be the LOWEST immortal level.  The number of
 * mortal levels will always be LVL_IMMORT - 1. */
#define LVL_IMPL 34    /**< Level of Implementors */
#define LVL_GRSTAFF 33 /**< Level of Greater Gods */
#define LVL_STAFF 32   /**< Level of Gods */
#define LVL_IMMORT 31  /**< Level of Immortals */
#define LVL_IMMORTAL LVL_IMMORT

/* this level and lower is classified as newbie */
#define NEWBIE_LEVEL 6
#define LEVEL_NEWBIE NEWBIE_LEVEL

/** Minimum level to build and to run the saveall command */
#define LVL_BUILDER LVL_IMMORT

/** Arbitrary number that won't be in a string */
#define MAGIC_NUMBER (0x06)

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
#define OPT_USEC 100000
/** How many heartbeats equate to one real second.
 * @see OPT_USEC
 * @see RL_SEC
 */
#define PASSES_PER_SEC (1000000 / OPT_USEC)
/** Used with other macros to define at how many heartbeats a control loop
 * gets executed. Helps to translate pulse counts to real seconds for
 * human comprehension.
 * @see PASSES_PER_SEC
 */
#define RL_SEC *PASSES_PER_SEC

/** Controls when a zone update will occur, notice changed from stock
 * value of 10 RL_SEC because of the size of our MUD (the dequeue for zone
 * resets was getting way backed up) */
#define PULSE_ZONE (3 RL_SEC)
/** Controls when mobile (NPC) actions and updates will occur. */
#define PULSE_MOBILE (6 RL_SEC)
/** Controls the time between turns of combat. */
#define PULSE_VIOLENCE (6 RL_SEC)

// controls some new luminari calls from comm.c
#define PULSE_LUMINARI (5 RL_SEC)

/* controls rate hints are called */
#define PULSE_HINTS (300 RL_SEC)

/** Controls when characters and houses (if implemented) will be autosaved.
 * @see CONFIG_AUTO_SAVE
 */
#define PULSE_AUTOSAVE (60 RL_SEC)
/** Controls when checks are made for idle name and password CON_ states */
#define PULSE_IDLEPWD (30 RL_SEC)
/** Currently unused. */
#define PULSE_SANITY (30 RL_SEC)
/** How often to log # connected sockets and # active players.
 * Currently set for 5 minutes.
 */
#define PULSE_USAGE (5 * 60 RL_SEC)
/** Controls when to save the current ingame MUD time to disk.
 * This should be set >= SECS_PER_MUD_HOUR */
#define PULSE_TIMESAVE (30 * 60 RL_SEC)
/* Variables for the output buffering system */
#define MAX_SOCK_BUF (24 * 1024) /**< Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH 400    /**< Max length of prompt        */
#define GARBAGE_SPACE 32         /**< Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE 1024       /**< Static output buffer size   */
/** Max amount of output that can be buffered */
#define LARGE_BUFSIZE (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

/* an arbitrary cap, medium/small in size for text */
#define SMALL_STRING 128
#define MEDIUM_STRING 256
#define LONG_STRING 512
#define LONGER_STRING 1024

#define MAX_STRING_LENGTH 49152          /**< Max length of string, as defined */
#define MAX_INPUT_LENGTH 512             /**< Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH (12 * 1024) /**< Max size of *raw* input */
#define MAX_MESSAGES 200                 /**< Max Different attack message types */
#define MAX_NAME_LENGTH 20               /**< Max PC/NPC name length */
#define MAX_PWD_LENGTH 30                /**< Max PC password length */
#define MAX_TITLE_LENGTH 80              /**< Max PC title length */
#define HOST_LENGTH 40                   /**< Max hostname resolution length */
#define PLR_DESC_LENGTH 4096             /**< Max length for PC description */
#define MAX_SKILLS 600                   /**< Max number of skills */
#define MAX_SPELLS 2000                  /**< Max number of spells */
#define MAX_ABILITIES 200                /**< Max number of abilities */
#define MAX_AFFECT 32                    /**< Max number of player affections */
#define MAX_OBJ_AFFECT 6                 /**< Max object affects */
#define MAX_NOTE_LENGTH 4000             /**< Max length of text on a note obj */
#define MAX_LAST_ENTRIES 6000            /**< Max log entries?? */
#define MAX_HELP_KEYWORDS 256            /**< Max length of help keyword string */
#define MAX_HELP_ENTRY MAX_STRING_LENGTH /**< Max size of help entry */
#define MAX_COMPLETED_QUESTS 1024        /**< Maximum number of completed quests allowed */
#define MAX_ANGER 100                    /**< Maximum mob anger/frustration as percentage */

// other MAX_ defines
#define MAX_WEAPON_SPELLS 3
#define MAX_WEAPON_CHANNEL_SPELLS 2
#define MAX_BAB 50
#define MAX_DAM_BONUS 120
#define MAX_AC 60
#define MAX_CONCEAL 50 // its percentage
#define MAX_DAM_REDUC 20
#define MAX_ENERGY_ABSORB 20
/* NOTE: oasis.h has a maximum value for weapon dice, this is diffrent */
/* 2nd NOTE:  Hard-coded weapon dice caps in db.c */
#define MAX_WEAPON_DAMAGE 24
#define MIN_WEAPON_DAMAGE 2

/* maximum number of moves a mobile can store for walking paths (patrols) */
#define MAX_PATH 50

#define MAX_GOLD 2140000000 /**< Maximum possible on hand gold (2.14 Billion) */
#define MAX_BANK 2140000000 /**< Maximum possible in bank gold (2.14 Billion) */

/** Define the largest set of commands for a trigger.
 * 16k should be plenty and then some. */
#define MAX_CMD_LENGTH 16384

/* Type Definitions */
typedef signed char sbyte;          /**< 1 byte; vals = -127 to 127 */
typedef unsigned char ubyte;        /**< 1 byte; vals = 0 to 255 */
typedef signed short int sh_int;    /**< 2 bytes; vals = -32,768 to 32,767 */
typedef unsigned short int ush_int; /**< 2 bytes; vals = 0 to 65,535 */

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32) /* Hm, sysdep.h? */
typedef signed char byte;                          /**< Technically 1 signed byte; vals should only = TRUE or FALSE. */
#endif

/* Various virtual (human-reference) number types. */
typedef IDXTYPE room_vnum;   /**< vnum specifically for room */
typedef IDXTYPE obj_vnum;    /**< vnum specifically for object */
typedef IDXTYPE mob_vnum;    /**< vnum specifically for mob (NPC) */
typedef IDXTYPE zone_vnum;   /**< vnum specifically for zone */
typedef IDXTYPE shop_vnum;   /**< vnum specifically for shop */
typedef IDXTYPE trig_vnum;   /**< vnum specifically for triggers */
typedef IDXTYPE qst_vnum;    /**< vnum specifically for quests */
typedef IDXTYPE clan_vnum;   /**< vnum specifically for clans */
typedef IDXTYPE region_vnum; /**< vnum specifically for regions */
typedef IDXTYPE path_vnum;   /**< vnum specifically for paths */

/* Various real (array-reference) number types. */
typedef IDXTYPE room_rnum;   /**< references an instance of a room */
typedef IDXTYPE obj_rnum;    /**< references an instance of a obj */
typedef IDXTYPE mob_rnum;    /**< references an instance of a mob (NPC) */
typedef IDXTYPE zone_rnum;   /**< references an instance of a zone */
typedef IDXTYPE shop_rnum;   /**< references an instance of a shop */
typedef IDXTYPE trig_rnum;   /**< references an instance of a trigger */
typedef IDXTYPE qst_rnum;    /**< references an instance of a quest */
typedef IDXTYPE clan_rnum;   /**< references an instance of a clan */
typedef IDXTYPE region_rnum; /**< references an instance of a region */
typedef IDXTYPE path_rnum;   /**< references an instance of a path */

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
struct extra_descr_data
{
    char *keyword;                 /**< Keyword for look/examine  */
    char *description;             /**< What is shown when this keyword is 'seen' */
    struct extra_descr_data *next; /**< Next description for this mob/obj/room */
};

/* object-related structures */
/**< Number of elements in the object value array. Raising this will provide
 * more configurability per object type, and shouldn't break anything.
 * DO NOT LOWER from the default value of 4. */
#define NUM_OBJ_VAL_POSITIONS 16
/* Same thing, but for Special Abilities for weapons, armor and shields. */
#define NUM_SPECAB_VAL_POSITIONS 4

/* maximum amount of timrs on a single object, imported from homeland */
#define SPEC_TIMER_MAX 4

/** object flags used in obj_data. These represent the instance values for
 * a real object, values that can change during gameplay. */
struct obj_flag_data
{
    int value[NUM_OBJ_VAL_POSITIONS]; /**< Values of the item (see list)    */
    byte type_flag;                   /**< Type of item			    */
    byte prof_flag;                   // proficiency associated with item
    int level;                        /**< Minimum level to use object	    */
    int wear_flags[TW_ARRAY_MAX];     /**< Where you can wear it, if wearable */
    int extra_flags[EF_ARRAY_MAX];    /**< If it hums, glows, etc.	    */
    int weight;                       /**< Weight of the object */
    int cost;                         /**< Value when sold             */
    int cost_per_day;                 /**< Rent cost per real day */
    int timer;                        /**< Timer for object             */
    int bitvector[AF_ARRAY_MAX];      /**< Affects characters           */

    byte material; // what material is the item made of?
    int size;      // how big is the object?

    int spec_timer[SPEC_TIMER_MAX]; /* For timed procs - from homeland*/
    int bound_id;                   /* ID of player this item is bound to */
};

/** Used in obj_file_elem. DO NOT CHANGE if you are using binary object files
 * and already have a player base and don't want to do a player wipe. */
struct obj_affected_type
{
    byte location;  /**< Which ability to change (APPLY_XXX) */
    int modifier;   /**< How much it changes by              */
    int bonus_type; /**< What type of bonus is this. */
    int specific;   // for feats and skills
};

/* For weapon spells. */
struct weapon_spells
{
    int spellnum;  // spellnum weapon will cast
    int level;     // level at which it will cast spellnum
    int percent;   // chance spellnum will fire per round
    int inCombat;  // will spellnum fire only in combat?
    int uses_left; // If it'd a temporary effect, this is the number of uses left
};

/* For special abilities for weapons, armor and 'wonderous items' - Ornir */
struct obj_special_ability
{
    int ability;                         /* Which ability does this object have? */
    int level;                           /* The 'Caster Level' of the affect. */
    int activation_method;               /* Command word, wearing/wielding, Hitting, On Critical, etc. */
    char *command_word;                  /* Only if the activation_method is ACTTYPE_COMMAND_WORD, NULL otherwise. */
    int value[NUM_SPECAB_VAL_POSITIONS]; /* Values for the special ability, see specab.c/specab.h for a list. */

    struct obj_special_ability *next; /* This is a list of abilities. */
};

// Spellbooks
/* maximum # spells in a spellbook */
#define SPELLBOOK_SIZE 200

/* the spellbook struct */
struct obj_spellbook_spell
{
    ush_int spellname; /* Which spell is written */
    ubyte pages;       /* How many pages does it take up */
};

/* for weapons, the poison-data if poison is applied */
struct obj_weapon_poison
{
    int poison;       /* right now this is a spell (i.e. spellnum) */
    int poison_level; /* level to cast above spell */
    int poison_hits;  /* how many times the poison will fire off the weapon */
};

/** The Object structure. */
struct obj_data
{
    obj_rnum item_number; /**< The unique id of this object instance. */
    room_rnum in_room;    /**< What room is the object lying in, or -1? */

    struct obj_flag_data obj_flags;                    /**< Object information */
    struct obj_affected_type affected[MAX_OBJ_AFFECT]; /**< affects */
    struct obj_weapon_poison weapon_poison;            /* for weapons, applied poison */

    char *name;                              /**< Keyword reference(s) for object. */
    char *description;                       /**< Shown when the object is lying in a room. */
    char *short_description;                 /**< Shown when worn, carried, in a container */
    char *action_description;                /**< Displays when (if) the object is used */
    struct extra_descr_data *ex_description; /**< List of extra descriptions */
    struct char_data *carried_by;            /**< Points to PC/NPC carrying, or NULL */
    struct char_data *worn_by;               /**< Points to PC/NPC wearing, or NULL */
    sh_int worn_on;                          /**< If the object can be worn, where can it be worn? */

    struct obj_data *in_obj;   /**< Points to carrying object, or NULL */
    struct obj_data *contains; /**< List of objects being carried, or NULL */

    long id;                              /**< used by DG triggers - unique id  */
    struct trig_proto_list *proto_script; /**< list of default triggers  */
    struct script_data *script;           /**< script info for the object */

    struct obj_data *next_content;  /**< For 'contains' lists   */
    struct obj_data *next;          /**< For the object list */
    struct char_data *sitting_here; /**< For furniture, who is sitting in it */

    bool has_spells; // used to keep track if weapon has weapon_spells
    // weapon spells allow gear to fire off spells intermittently or in combat
    struct weapon_spells wpn_spells[MAX_WEAPON_SPELLS];

    struct obj_spellbook_spell *sbinfo; /* For spellbook info */

    struct list_data *events; /**< Used for object events */

    struct obj_special_ability *special_abilities; /**< List used to store special abilities */

    long missile_id; // non saving variable to id missiles

    struct weapon_spells channel_spells[MAX_WEAPON_CHANNEL_SPELLS];

    mob_vnum mob_recepient; // if this is set, then the object can only be given to a mob with this vnum (or any player)
};

/** Instance info for an object that gets saved to disk.
 * DO NOT CHANGE if you are using binary object files
 * and already have a player base and don't want to do a player wipe. */
struct obj_file_elem
{
    obj_vnum item_number; /**< The prototype, non-unique info for this object. */

#if USE_AUTOEQ
    sh_int location; /**< If re-equipping objects on load, wear object here */
#endif
    int value[NUM_OBJ_VAL_POSITIONS];                  /**< Current object values */
    int extra_flags[EF_ARRAY_MAX];                     /**< Object extra flags */
    int weight;                                        /**< Object weight */
    int timer;                                         /**< Current object timer setting */
    int bitvector[AF_ARRAY_MAX];                       /**< Object affects */
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
struct rent_info
{
    int time;
    int rentcode;          /**< How this character rented */
    int net_cost_per_diem; /**< ? Appears to be unused ? */
    int gold;              /**< ? Appears to be unused ? */
    int account;           /**< ? Appears to be unused ? */
    int nitems;            /**< ? Appears to be unused ? */
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
struct room_direction_data
{
    char *general_description; /**< Show to char looking in this direction. */

    char *keyword; /**< for interacting (open/close) this direction */

    sh_int /*bitvector_t*/ exit_info; /**< Door, and what type? */
    obj_vnum key;                     /**< Key's vnum (-1 for no key) */
    room_rnum to_room;                /**< Where direction leads, or NOWHERE if not defined */

    /* Extra door flags. */
};

struct raff_node
{
    room_rnum room;       /* location in the world[] array of the room */
    int timer;            /* how many rounds this affection lasts */
    long affection;       /* which affection does this room have */
    int spell;            /* the spell number */
    struct char_data *ch; // caster of this affection
    int dc;               // save dc, if specified
    bool special;         // true if a special affect associated with the room affect applies

    struct raff_node *next; /* link to the next node */
};

/* From trails.h */
struct trail_data_list;

/** The Room Structure. */
struct room_data
{
    room_vnum number;                                    /**< Rooms number (vnum) */
    zone_rnum zone;                                      /**< Room zone (for resetting) */
    int coords[2];                                       /**< Room coordinates (for wilderness) */
    int sector_type;                                     /**< sector type (move/hide) */
    int room_flags[RF_ARRAY_MAX];                        /**< INDOORS, DARK, etc */
    long room_affections;                                /* bitvector for spells/skills */
    char *name;                                          /**< Room name */
    char *description;                                   /**< Shown when entered, looked at */
    struct extra_descr_data *ex_description;             /**< Additional things to look at */
    struct room_direction_data *dir_option[NUM_OF_DIRS]; /**< Directions */
    byte light;                                          /**< Number of lightsources in room */
    byte globe;                                          /**< Number of darkness sources in room */
    SPECIAL_DECL(*func);                                 /**< Points to special function attached to room */
    struct trig_proto_list *proto_script;                /**< list of default triggers */
    struct script_data *script;                          /**< script info for the room */
    struct obj_data *contents;                           /**< List of items in room */
    struct char_data *people;                            /**< List of NPCs / PCs in room */

    struct list_data *events; // room events

    struct trail_data_list *trail_tracks;
    // struct trail_data_list *trail_scent;
    // struct trail_data_list *trail_blood;
    //// struct trail_data_list *trail_magic;
};

/* char-related structures */

/** Memory structure used by NPCs to remember specific PCs. */
struct memory_rec_struct
{
    long id;                        /**< The PC id to remember. */
    struct memory_rec_struct *next; /**< Next PC to remember */
};

/** memory_rec_struct typedef */
typedef struct memory_rec_struct memory_rec;

/** This structure is purely intended to be an easy way to transfer and return
 * information about time (real or mudwise). */
struct time_info_data
{
    int hours;   /**< numeric hour */
    int day;     /**< numeric day */
    int month;   /**< numeric month */
    sh_int year; /**< numeric year */
};

/** Player specific time information. */
struct time_data
{
    time_t birth; /**< Represents the PCs birthday, used to calculate age. */
    time_t logon; /**< Time of the last logon, used to calculate time played */
    int played;   /**< This is the total accumulated time played in secs */
};

/* Group Data Struct */
struct group_data
{
    struct char_data *leader;  // leader of group
    struct list_data *members; // list of members
    int group_flags;           // group flags set
};

/** The pclean_criteria_data is set up in config.c and used in db.c to determine
 * the conditions which will cause a player character to be deleted from disk
 * if the automagic pwipe system is enabled (see config.c). */
struct pclean_criteria_data
{
    int level; /**< PC level and below to check for deletion */
    int days;  /**< time limit in days, for this level of PC */
};

/** General info used by PC's and NPC's. */
struct char_player_data
{
    char passwd[MAX_PWD_LENGTH + 1]; /**< PC's password */
    char *name;                      /**< PC / NPC name */
    char *short_descr;               /**< NPC 'actions' */
    char *long_descr;                /**< PC / NPC look description */
    char *description;               /**< NPC Extra descriptions */
    char *title;                     /**< PC / NPC title */
    byte sex;                        /**< PC / NPC sex */
    byte chclass;                    /**< PC / NPC class */
    byte level;                      /**< PC / NPC level */
    struct time_data time;           /**< PC AGE in days */
    ubyte weight;                    /**< PC / NPC weight */
    ubyte height;                    /**< PC / NPC height */
    byte race;                       // Race
    byte pc_subrace;                 // SubRace
    char *walkin;                    // NPC (for now) walkin message
    char *walkout;                   // NPC (for now) walkout message
};

/** Character abilities. Different instances of this structure are used for
 * both inherent and current ability scores (like when poison affects the
 * player strength). */
struct char_ability_data
{
    sbyte str;   /**< Strength.  */
    sbyte intel; /**< Intelligence */
    sbyte wis;   /**< Wisdom */
    sbyte dex;   /**< Dexterity */
    sbyte con;   /**< Constitution */
    sbyte cha;   /**< Charisma */

    /*unused*/ sbyte str_add; /**< Strength multiplier if str = 18. Usually from 0 to 100 */
};
#define NUM_ABILITY_MODS 6

/* make sure this matches spells.h define */
#define NUM_DAM_TYPES 25

/* Character 'points', or health statistics. (we have points and real_points) */
struct char_point_data
{
    sh_int psp;            /**< Current psp level  */
    sh_int max_psp;        /**< Max psp level */
    int hit;               /**< Curent hit point, or health, level */
    int max_hit;           /**< Max hit point, or health, level */
    sh_int move;           /**< Current move point, or stamina, level */
    sh_int max_move;       /**< Max move point, or stamina, level */
    sh_int armor;          // armor class
    sh_int disguise_armor; /* disguise armor class bonus */
    sh_int spell_res;      // spell resistance

    int gold;      /**< Current gold carried on character */
    int bank_gold; /**< Gold the char has in a bank account	*/
    int exp;       /**< The experience points, or value, of the character. */

    sbyte hitroll; /**< Any bonus or penalty to the hit roll */
    sbyte damroll; /**< Any bonus or penalty to the damage roll */

    int size;                                        // size
    sh_int apply_saving_throw[NUM_OF_SAVING_THROWS]; /**< Saving throw (Bonuses) */
    sh_int resistances[NUM_DAM_TYPES];               // resistances (dam-types)

    /* note - if you add something new here, make sure to check
     handler.c reset_char_points() to see if it needs to be added */
};

/** char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the players file for PC's. */
struct char_special_data_saved
{
    int alignment;                 /**< -1000 (evil) to 1000 (good) range. */
    long idnum;                    /**< PC's idnum; -1 for mobiles. */
    int act[PM_ARRAY_MAX];         /**< act flags for NPC's; player flag for PC's */
    int affected_by[AF_ARRAY_MAX]; /**< Bitvector for spells/skills affected by */
    int warding[MAX_WARDING];      // saved warding spells like stoneskin
    ubyte spec_abil[MAX_CLASSES];  // spec abilities (ex. lay on hands)

    struct damage_reduction_type *damage_reduction; /**< Damage Reduction */

    /* disguise system port d20mud */
    ubyte disguise_race;
    ubyte disguise_sex;
    ubyte disguise_dsc1;
    ubyte disguise_dsc2;
    ubyte disguise_adj1;
    ubyte disguise_adj2;
    ubyte disguise_roll;
    ubyte disguise_seen;

    /* Feat data */
    int feats[NUM_FEATS];                       /* Feats (value is the number of times each feat is taken) */
    int combat_feats[NUM_CFEATS][FT_ARRAY_MAX]; /* One bitvector array per CFEAT_ type  */
    int school_feats[NUM_SFEATS];               /* One bitvector array per CFEAT_ type  */
};

/** Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data
{
    /* combat related */
    int initiative;             /* What is this char's initiative score? */
    struct char_data *fighting; /**< Target of fight; else NULL */
    struct char_data *hunting;  /**< Target of NPC hunt; else NULL */
    int totalDefense;           // how many totaldefense attempts left in the round
    struct char_data *guarding; // target for 'guard' ability
    bool firing;                // is char firing missile weapon?
    int mounted_blocks_left;    // how many mounted combat blocks left in the round
    int deflect_arrows_left;    //

    /* Mode Data */
    int mode_value; /* Bonus/penalty for power attack and combat expertise. */

    /* Combat related, reset each combat round. We do not use events for these because
     * the timing needs to be perfect - They should be reset in accordance with the
     * initiation of auto-attacks in each round. */
    int attacks_of_opportunity; /* The number of AOO performed this round. */

    /* furniture */
    struct obj_data *furniture;          /**< Object being sat on/in; else NULL */
    struct char_data *next_in_furniture; /**< Next person sitting, else NULL */

    /* mounts */
    struct char_data *riding;    /* Who are they riding? */
    struct char_data *ridden_by; /* Who is riding them? */

    /* carrying */
    int carry_weight; /**< Carried weight */
    byte carry_items; /**< Number of items carried */

    /** casting (time) **/
    bool isCasting;               // casting or not
    int castingTime;              // casting time
    int castingSpellnum;          // spell casting
    int castingMetamagic;         // spell metamagic
    int castingClass;             // spell casting class
    struct char_data *castingTCH; // target char of spell
    struct obj_data *castingTOBJ; // target obj of spell

    /** crafting **/
    ubyte crafting_type;              // like SCMD_x
    ubyte crafting_ticks;             // ticks left to complete task
    struct obj_data *crafting_object; // refers to obj crafting (deprecated)
    ubyte crafting_repeat;            // multiple objects created in one session
    int crafting_bonus;               // bonus for crafting the item

    /* mob feats (npc's and pc wildshaped) */
    byte mob_feats[MAX_FEATS]; /* Feats (booleans and counters)  */

    /* miscellaneous */
    int is_preparing[NUM_CASTERS];    // memorization
    int preparing_state[NUM_CLASSES]; /* spell preparation */
    byte position;                    /**< Standing, fighting, sleeping, etc. */
    int timer;                        /**< Timer for update */

    int weather; /**< The current weather this player is affected by. */

    struct queue_type *action_queue; /**< Action command queue */
    struct queue_type *attack_queue; /**< Attack action queue */

    struct char_special_data_saved saved; /**< Constants saved for PCs. */

    struct char_data *grapple_target;   /**< Target of grapple attempt; else NULL */
    struct char_data *grapple_attacker; /**< Who is grappling me?; else NULL */

    bool energy_retort_used; // used with energy retort ability, which only fires once per round.

    bool autodoor_message; // used for message handling in autodoor
};

/* old memorization struct */
struct old_spell_data
{
    int spell;     /* spellnum of this spell in the collection */
    int metamagic; /* Bitvector of metamagic affecting this spell. */
    int prep_time; /* time to prepare */
};
/**/

/***/

/* known spells list */
struct known_spell_data
{
    int spell;     /* spellnum of this spell in the collection */
    int metamagic; /* Bitvector of metamagic affecting this spell. */
    int prep_time; /* Remaining time for preparing this spell. */
    int domain;    /* domain info */

    struct known_spell_data *next; /*linked-list*/
};

/* spell parapation, collection data */
struct prep_collection_spell_data
{
    int spell;     /* spellnum of this spell in the collection */
    int metamagic; /* Bitvector of metamagic affecting this spell. */
    int prep_time; /* Remaining time for preparing this spell. */
    int domain;    /* domain info */

    struct prep_collection_spell_data *next; /*linked-list*/
};

/* innate magic preparation data */
struct innate_magic_data
{
    int circle;    /* circle in the collection */
    int metamagic; /* Bitvector of metamagic affecting this spell. */
    int prep_time; /* Remaining time for preparing this spell. */
    int domain;    /* domain info */

    struct innate_magic_data *next; /*linked-list*/
};
/***/

/** Data only needed by PCs, and needs to be saved to disk. */
struct player_special_data_saved
{
    int skills[MAX_SKILLS + 1];         // saved skills
    int spells[MAX_SPELLS];             // saved spells, should be MAX_SPELLS + 1 from spells.h
    ubyte abilities[MAX_ABILITIES + 1]; // abilities

    /* Feats */
    byte feat_points;                         /* How many general feats you can take  */
    byte epic_feat_points;                    /* How many epic feats you can take */
    byte class_feat_points[NUM_CLASSES];      /* How many class feats you can take  */
    byte epic_class_feat_points[NUM_CLASSES]; /* How many epic class feats    */

    bool skill_focus[MAX_ABILITIES + 1][NUM_SKFEATS]; /* Data for FEAT_SKILL_FOCUS */

    ubyte morphed;                    // polymorphed and form
    byte class_level[MAX_CLASSES];    // multi class
    int spells_to_learn;              // prac sessions left
    int abilities_to_learn;           // training sessiosn left
    ubyte boosts;                     // stat boosts left
    ubyte favored_enemy[MAX_ENEMIES]; // list of ranger favored enemies

    /* old spell prep system, can be removed */
    struct old_spell_data prep_queue[MAX_MEM][NUM_CASTERS];
    struct old_spell_data collection[MAX_MEM][NUM_CASTERS];

    /* new system for spell preparation */
    struct prep_collection_spell_data *preparation_queue[NUM_CLASSES];
    struct prep_collection_spell_data *spell_collection[NUM_CLASSES];
    struct innate_magic_data *innate_magic_queue[NUM_CLASSES];
    struct known_spell_data *known_spells[NUM_CLASSES];

    byte church; // homeland-port, currently unused

    /* schools / domains */
    byte domain_1;            /* cleric domains */
    byte domain_2;            /* cleric domains */
    byte specialty_school;    /* wizard specialty */
    byte restricted_school_1; /* restricted school */
    byte restricted_school_2; /* restricted school */

    /* preferred caster classs, used for prestige classes such as arcane archer */
    byte preferred_arcane;
    byte preferred_divine;

    int wimp_level;                        /**< Below this # of hit points, flee! */
    byte freeze_level;                     /**< Level of god who froze char, if any */
    sh_int invis_level;                    /**< level of invisibility */
    room_vnum load_room;                   /**< Which room to load PC into */
    int pref[PR_ARRAY_MAX];                /**< preference flags */
    ubyte bad_pws;                         /**< number of bad login attempts */
    sbyte conditions[3];                   /**< Drunk, hunger, and thirst */
    struct txt_block *comm_hist[NUM_HIST]; /**< Communication history */
    struct txt_block *todo_list;           /* Player's todo list */
    ubyte page_length;                     /**< Max number of rows of text to send at once */
    ubyte screen_width;                    /**< How wide the display page is */
    int olc_zone;                          /**< Current olc permissions */

    /* clan system */
    int clanpoints; /**< Clan points may be spent in a clanhall */
    clan_vnum clan; /**< The clan number to which the player belongs     */
    int clanrank;   /**< The player's rank within their clan (1=highest) */

/* autoquest */
#define MAX_CURRENT_QUESTS 3
    int questpoints;                       // quest points earned
    qst_vnum *completed_quests;            /**< Quests completed              */
    int num_completed_quests;              /**< Number completed              */
    int current_quest[MAX_CURRENT_QUESTS]; /**< vnums of current quests         */
    int quest_time[MAX_CURRENT_QUESTS];    /**< time left on current quest    */
    int quest_counter[MAX_CURRENT_QUESTS]; /**< Count of targets left to get  */

    /* auto crafting quest */
    unsigned int autocquest_vnum; // vnum of crafting quest item
    char *autocquest_desc;        // description of crafting quest item
    ubyte autocquest_material;    // material used for crafting quest
    ubyte autocquest_makenum;     // how many more objects to finish quest
    ubyte autocquest_qp;          // quest point reward for quest
    unsigned int autocquest_exp;  // exp reward for quest
    unsigned int autocquest_gold; // gold reward for quest

    time_t lastmotd; /**< Last time player read motd */
    time_t lastnews; /**< Last time player read news */

    char *account_name; // The account stored with this character.

    int sorcerer_bloodline_subtype; // if the sorcerer bloodline has a subtype (ie. draconic)
    int new_arcana_circles[4];
    int mail_days;

    /* alchemists */
    int discoveries[NUM_ALC_DISCOVERIES];
    int bombs[MAX_BOMBS_ALLOWED];
    int grand_discovery;

    /* template system */
    ubyte template;
    int premade_build;

    /* factional mission system */
    int current_mission;
    long mission_credits;
    long mission_standing;
    int mission_faction;
    long mission_reputation;
    long mission_experience;
    int mission_difficulty;
    long faction_standing[NUM_FACTIONS + 1];
    long faction_standing_spent[NUM_FACTIONS + 1];
    bool mission_decline;
    int mission_rand_name;
    bool mission_complete;
    int mission_cooldown;
    int faction;

    /* staff event variables */
    int staff_ran_events[STAFF_RAN_EVENTS_VAR];

    // set true if ability scores have been set in study
    bool have_stats_been_set_study;

    int pixie_dust_uses;
    int pixie_dust_timer;
    int efreeti_magic_uses;
    int efreeti_magic_timer;
    int dragon_magic_uses;
    int dragon_magic_timer;
    int laughing_touch_uses;
    int laughing_touch_timer;
    int fleeting_glance_uses;
    int fleeting_glance_timer;
    int fey_shadow_walk_uses;
    int fey_shadow_walk_timer;
    int grave_touch_uses;
    int grave_touch_timer;
    int grasp_of_the_dead_uses;
    int grasp_of_the_dead_timer;
    int incorporeal_form_uses;
    int incorporeal_form_timer;

    int psionic_energy_type; // this is the element that will be used when using psionic energy powers

    int potions[MAX_SPELLS]; // used in new consumables system store/unstore/quaff
    int scrolls[MAX_SPELLS]; // used in new consumables system store/unstore/recite
    int wands[MAX_SPELLS];   // used in new consumables system store/unstore/use
    int staves[MAX_SPELLS];  // used in new consumables system store/unstore/use

    int holy_weapon_type;                               // type of weapon to use withn holy weapon spell, also known as holy sword spell
    int paladin_mercies[NUM_PALADIN_MERCIES];           // stores a paladin's mercies known
    int blackguard_cruelties[NUM_BLACKGUARD_CRUELTIES]; // stores a blackguard's mercies known
    int fiendish_boons;                                 // active fiendish boons by blackguard
    int channel_energy_type;                            // neutral clerics must decide either positive or negative
    int deity;                                          // what deity does the person follow?
};

/** Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the players file. */
struct player_special_data
{
    struct player_special_data_saved saved; /**< Information to be saved. */

    char *poofin;               /**< Description displayed to room on arrival of a god. */
    char *poofout;              /**< Description displayed to room at a god's exit. */
    struct alias_data *aliases; /**< Command aliases			*/
    long last_tell;             /**< idnum of PC who last told this PC, used to reply */
    void *last_olc_targ;        /**< ? Currently Unused ? */
    int last_olc_mode;          /**< ? Currently Unused ? */
    char *host;                 /**< Resolved hostname, or ip, for player. */
    int diplomacy_wait;         /**< Diplomacy Timer */
    int buildwalk_sector;       /**< Default sector type for buildwalk */

    /* salvation spell */
    room_vnum salvation_room;
    char *salvation_name;

    /* levelup data structure - Saved data for study process. */
    struct level_data *levelup;

    byte dc_bonus;                /* used to apply dc bonuses, usually to spells.
                    Must be reset to zero manually after applying the bonus */
    byte arcane_apotheosis_slots; /* used with the apotheosis command to store spell slots
                                   to be used in place of wand or staff charges.  These stored
                                  slots decay at a rate of 1 per 6-second round and cannot have
                                  more than 9 stored at any given time.  They are not saved over
                                  reboots/copyovers/character quitting. */
    char *new_mail_receiver;
    char *new_mail_subject;
    char *new_mail_content;
    byte has_eldritch_knight_spell_critical;
    int destination;                      // used for carriage and airship systems
    int travel_timer;                     // used for carriage and airship systems
    int travel_type;                      // used for carriage and airship systems
    int travel_locale;                    // used for carriage and airship systems
    int bane_race;                        // used in applyoil command to create a proper bane weapon
    int bane_subrace;                     // used in applyoil command to create a proper bane weapon
    int augment_psp;                      // used when augmenting psionic powers
    int temp_attack_roll_bonus;           // used when needing to add to an attack roll from outside, and before calling the attack_roll function
    int dam_co_holder_ndice;              // a holder for number of damage dice for psionic_concussive_onslaught
    int dam_co_holder_sdice;              // a holder for size of damage dice for psionic_concussive_onslaught
    int dam_co_holder_bonus;              // a holder for bonus to damage for psionic_concussive_onslaught
    int save_co_holder_dc_bonus;          // a holder for bonus to save dc for psionic_concussive_onslaught
    bool cosmic_awareness;                // cosmic awareness psionic power and command
    int energy_conversion[NUM_DAM_TYPES]; // energy conversion ability

    int casting_class; // The class number that is currently casting a spell

    int concussive_onslaught_duration;
    bool has_banishment_been_attempted; // for use with holy/unholy champion banishment attempt
    struct obj_data *outfit_obj;
    int outfit_type;
    char *outfit_desc;
    char *outfit_confirmation;

    short mark_rounds;             // number of rounds a character has marked their opponent for
    struct char_data *mark_target; // person the character is marking for assassination
    int death_attack_hit_bonus;
    int death_attack_dam_bonus;
    room_vnum walkto_location;
};

/** Special data used by NPCs, not PCs */
struct mob_special_data
{
    memory_rec *memory;         /**< List of PCs to remember */
    byte attack_type;           /**< The primary attack type (bite, sting, hit, etc.) */
    byte default_pos;           /**< Default position (standing, sleeping, etc.) */
    byte damnodice;             /**< The number of dice to roll for damage */
    byte damsizedice;           /**< The size of each die rolled for damage. */
    float frustration_level;    /**< The anger/frustration level of the mob */
    byte subrace[MAX_SUBRACES]; // SubRace
    struct quest_entry *quest;  // quest info for a mob (homeland-port)
    room_rnum loadroom;         // mob loadroom saved
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
    /* a (generally) boolean macro that marks whether a proc fired, general use is
       for zone-procs */
    int proc_fired;
    room_rnum temp_room_data;   /* for homeland, for storing temporary room data */
    bool hostile;               // used for encounters, hostile mobs will aggro after a timer
    bool sentient;              // used for encounters, sentient mobs can be bribeed, intimidated, bluffed, etc.
    int aggro_timer;            // used for encounters, this timer will start for hostile mobs, after which the aggro flag will be applied
    int extract_timer;          // used for encounters.  This timer is set when the player(s) leave the room.  When timer ends, mob will be extracted
    int peaceful_timer;         // used for encounter. While active hostile encounters are suspended, and the player(s) can leave the room
    bool coersion_attempted[5]; // used for encounters to track if they've been coerced before (intimidate, bluff, stealth and diplomacy)
    int hunt_type;              // for hunts, used to track which hunt entry it is on the huhnt table
    int hunt_cooldown;          // for hunts, when hunt expires, this is set to 5 minutes, at which point it will be extracted
};

/** An affect structure. */
struct affected_type
{
    sh_int spell;                /**< The spell that caused this */
    sh_int duration;             /**< For how long its effects will last      */
    sh_int modifier;             /**< Added/subtracted to/from apropriate ability     */
    byte location;               /**< Tells which ability to change(APPLY_XXX). */
    int bitvector[AF_ARRAY_MAX]; /**< Tells which bits to set (AFF_XXX). */

    int bonus_type; /**< What type of bonus (if this is a bonus) is this. */

    struct affected_type *next; /**< The next affect in the list of affects. */
    sh_int specific;
};

/* The Maximum number of types that can be required to bypass DR. */
#define MAX_DR_BYPASS 3

#define DR_BYPASS_CAT_UNUSED 0   /* Unused bypass - skip. */
#define DR_BYPASS_CAT_NONE 1     /* Nothing bypasses the DR */
#define DR_BYPASS_CAT_MATERIAL 2 /* Materials that bypass the DR*/
#define DR_BYPASS_CAT_MAGIC 3    /* Magical weapons bypass the DR */
#define DR_BYPASS_CAT_DAMTYPE 4  /* DR Damage types that bypass the DR */

#define DR_DAMTYPE_BLUDGEONING 0 /* Bludgeoning damage bypasses the DR */
#define DR_DAMTYPE_SLASHING 1    /* Slashing damage bypasses the DR */
#define DR_DAMTYPE_PIERCING 2    /* Piercing damage bypasses the DR */
#define NUM_DR_DAMTYPES 3

/* Note that spells ALWAYS bypass DR! Resistances are for Spells, DR is for
 * physical damage! */

/** A damage reduction structure. */
struct damage_reduction_type
{
    int duration;   /* The duration of this DR effect. */
    int amount;     /* The amount of DR. */
    int max_damage; /* The amount of damage this DR can take before it dissipates.  -1 is perm. */
    int spell;      /* Spell granting this DR. */
    int feat;       /* Feat granting this DR. */

    /* The following values can be a bit confusing - So a clarification
     * is in order.
     *
     * 'bypass_cat' is an array of integer values (one of the above defines)
     * 'bypass_val' is an array of integer values that expands upon the category.
     *
     * 'bypass_val' only has values for the following categories:
     *
     * DR_BYPASS_CAT_MATERIAL - The value is the corresponding material.
     * DR_BYPASS_CAT_DAMTYPE- The value is the corresponding damage type.
     *
     * MAX_DR_BYPASS sets the maximum number of bypasses that can be set
     * on a particular DR.  For simplicity, bypasses are only 'OR' separated...
     * meaning that if a dr 10 has two categories, for example magic and spell, it
     * would be DR 10/(magic or spell)
     */
    int bypass_cat[MAX_DR_BYPASS]; /* Category of bypass */
    int bypass_val[MAX_DR_BYPASS]; /* Value (required for certain categories) */

    struct damage_reduction_type *next;
};

/* Structure for levelup data - Used as a temporary storage area during 'study' command
 * Ascess via the LEVELUP(ch) macro. */

struct level_data
{
    int level;
    int class;
    int feats[NUM_FEATS];
    int combat_feats[NUM_CFEATS][FT_ARRAY_MAX];
    int school_feats[NUM_SFEATS];
    int boosts[6];
    bool skill_focus[MAX_ABILITIES + 1][NUM_SKFEATS]; /* Data for FEAT_SKILL_FOCUS */

    /* Feat point information */
    int feat_points;
    int class_feat_points;
    int epic_feat_points;
    int epic_class_feat_points;

    /* Ability, skill, boost information */
    int practices;
    int trains;
    int num_boosts;

    int spell_circle;
    int favored_slot;

    int feat_type;
    int tempFeat;
    int feat_weapons[NUM_FEATS];
    int feat_skills[NUM_FEATS];
    /*        int spells_known[NUM_SPELLS];*/
    int spell_slots[10];
    int spells_learned[MAX_SPELLS];

    /* setting stats */
    int str;
    int dex;
    int con;
    int inte;
    int wis;
    int cha;

    // Sorcerer Bloodline Subtype
    int sorcerer_bloodline_subtype;
    // Alchemist Discoveries
    int discoveries[NUM_ALC_DISCOVERIES];
    int tempDiscovery;
    int grand_discovery;
    int skills[MAX_SKILLS + 1];
    int paladin_mercies[NUM_PALADIN_MERCIES];
    int tempMercy;
    int blackguard_cruelties[NUM_BLACKGUARD_CRUELTIES];
    int tempCruelty;
};

/** The list element that makes up a list of characters following this
 * character. */
struct follow_type
{
    struct char_data *follower; /**< Character directly following. */
    struct follow_type *next;   /**< Next character following. */
};

/** Master structure for PCs and NPCs. */
struct char_data
{
    int pfilepos;          /**< PC playerfile pos and id number */
    mob_rnum nr;           /**< NPC real instance number */
    int coords[2];         /**< Current coordinate location, used in wilderness. */
    room_rnum in_room;     /**< Current location (real room number) */
    room_rnum was_in_room; /**< Previous location for linkdead people  */
    int wait;              /**< wait for how many loops before taking action. */

    struct char_player_data player;              /**< General PC/NPC data */
    struct char_ability_data real_abils;         /**< Abilities without modifiers */
    struct char_ability_data aff_abils;          /**< Abilities with modifiers */
    struct char_ability_data disguise_abils;     /* wildshape/shapechange/etc bonuses */
    struct char_point_data points;               /**< Point/statistics */
    struct char_point_data real_points;          /**< Point/statistics */
    struct char_special_data char_specials;      /**< PC/NPC specials	  */
    struct player_special_data *player_specials; /**< PC specials		  */
    struct mob_special_data mob_specials;        /**< NPC specials		  */

    struct affected_type *affected;        /**< affected by what spells    */
    struct obj_data *equipment[NUM_WEARS]; /**< Equipment array            */

    struct obj_data *carrying;    /**< List head for objects in inventory */
    struct descriptor_data *desc; /**< Descriptor/connection info; NPCs = NULL */

    long id;                              /**< used by DG triggers - unique id */
    struct trig_proto_list *proto_script; /**< list of default triggers */
    struct script_data *script;           /**< script info for the object */
    struct script_memory *memory;         /**< for mob memory triggers */

    struct char_data *next_in_room;  /**< Next PC in the room */
    struct char_data *next;          /**< Next char_data in the room */
    struct char_data *next_fighting; /**< Next in line to fight */

    struct follow_type *followers; /**< List of characters following */
    struct char_data *master;      /**< List of character being followed */

    struct group_data *group; /**< Character's Group */

    long pref; /**< unique session id */

    struct list_data *events;

    struct char_data *last_attacker; // mainly to prevent type_suffering from awarding exp

    int sticky_bomb[3];
    long mission_owner;
    bool dead;

    long int confuser_idnum;
    bool preserve_organs_procced;
    bool mute_equip_messages;
};

/** descriptor-related structures */
struct txt_block
{
    char *text;             /**< ? */
    int aliased;            /**< ? */
    struct txt_block *next; /**< ? */
};

/** ? */
struct txt_q
{
    struct txt_block *head; /**< ? */
    struct txt_block *tail; /**< ? */
};

/** Master structure players. Holds the real players connection to the mud.
 * An analogy is the char_data is the body of the character, the descriptor_data
 * is the soul. */
struct descriptor_data
{
    socket_t descriptor;               /**< file descriptor for socket */
    char host[HOST_LENGTH + 1];        /**< hostname */
    byte bad_pws;                      /**< number of bad pw attemps this login */
    byte idle_tics;                    /**< tics idle at password prompt		*/
    int connected;                     /**< mode of 'connectedness'		*/
    int desc_num;                      /**< unique num assigned to desc		*/
    time_t login_time;                 /**< when the person connected		*/
    char *showstr_head;                /**< for keeping track of an internal str	*/
    char **showstr_vector;             /**< for paging through texts		*/
    int showstr_count;                 /**< number of pages to page through	*/
    int showstr_page;                  /**< which page are we currently showing?	*/
    char **str;                        /**< for the modify-str system		*/
    char *backstr;                     /**< backup string for modify-str system	*/
    size_t max_str;                    /**< maximum size of string in modify-str	*/
    long mail_to;                      /**< name for mail system			*/
    int has_prompt;                    /**< is the user at a prompt?             */
    char inbuf[MAX_RAW_INPUT_LENGTH];  /**< buffer for raw input		*/
    char last_input[MAX_INPUT_LENGTH]; /**< the last input			*/
    char small_outbuf[SMALL_BUFSIZE];  /**< standard output buffer		*/
    char *output;                      /**< ptr to the current output buffer	*/
    char **history;                    /**< History of commands, for ! mostly.	*/
    int history_pos;                   /**< Circular array position.		*/
    int bufptr;                        /**< ptr to end of current output		*/
    int bufspace;                      /**< space left in the output buffer	*/
    struct txt_block *large_outbuf;    /**< ptr to large buffer, if we need it */
    struct txt_q input;                /**< q of unprocessed input		*/
    struct char_data *character;       /**< linked to char			*/
    struct char_data *original;        /**< original char if switched		*/
    struct descriptor_data *snooping;  /**< Who is this char snooping	*/
    struct descriptor_data *snoop_by;  /**< And who is snooping this char	*/
    struct descriptor_data *next;      /**< link to next descriptor		*/
    struct oasis_olc_data *olc;        /**< OLC info */

    protocol_t *pProtocol;    /**< Kavir plugin */
    struct list_data *events; // event system

    struct account_data *account; /**< Account system */
};

/* other miscellaneous structures */

/** Fight message display. This structure is used to hold the information to
 * be displayed for every different violent hit type. */
struct msg_type
{
    char *attacker_msg; /**< Message displayed to attecker. */
    char *victim_msg;   /**< Message displayed to victim. */
    char *room_msg;     /**< Message displayed to rest of players in room. */
};

/** An entire message structure for a type of hit or spell or skill. */
struct message_type
{
    struct msg_type die_msg;   /**< Messages for death strikes. */
    struct msg_type miss_msg;  /**< Messages for missed strikes. */
    struct msg_type hit_msg;   /**< Messages for a succesful strike. */
    struct msg_type god_msg;   /**< Messages when trying to hit a god. */
    struct message_type *next; /**< Next set of messages. */
};

/** Head of list of messages for an attack type. */
struct message_list
{
    int a_type;               /**< The id of this attack type. */
    int number_of_attacks;    /**< How many attack messages to chose from. */
    struct message_type *msg; /**< List of messages.			*/
};

/** Social message data structure. */
struct social_messg
{
    int act_nr;              /**< The social id. */
    char *command;           /**< The command to activate (smile, wave, etc.) */
    char *sort_as;           /**< Priority of social sorted by this. */
    int hide;                /**< If true, and target can't see actor, target doesn't see */
    int min_victim_position; /**< Required Position of victim */
    int min_char_position;   /**< Required Position of char */
    int min_level_char;      /**< Minimum PC level required to use this social. */

    /* No argument was supplied */
    char *char_no_arg;   /**< Displayed to char when no argument is supplied */
    char *others_no_arg; /**< Displayed to others when no arg is supplied */

    /* An argument was there, and a victim was found */
    char *char_found;   /**< Display to char when arg is supplied */
    char *others_found; /**< Display to others when arg is supplied */
    char *vict_found;   /**< Display to target arg */

    /* An argument was there, as well as a body part, and a victim was found */
    char *char_body_found;   /**< Display to actor */
    char *others_body_found; /**< Display to others */
    char *vict_body_found;   /**< Display to target argument */

    /* An argument was there, but no victim was found */
    char *not_found; /**< Display when no victim is found */

    /* The victim turned out to be the character */
    char *char_auto;   /**< Display when self is supplied */
    char *others_auto; /**< Display to others when self is supplied */

    /* If the char cant be found search the char's inven and do these: */
    char *char_obj_found;   /**< Social performed on object, display to char */
    char *others_obj_found; /**< Social performed on object, display to others */
};

/** Describes bonuses, or negatives, applied to thieves skills. In practice
 * this list is tied to the character's dexterity attribute. */
struct dex_skill_type
{
    sh_int p_pocket; /**< Alters the success rate of pick pockets */
    sh_int p_locks;  /**< Alters the success of pick locks */
    sh_int traps;    /**< Historically alters the success of trap finding. */
    sh_int sneak;    /**< Alters the success of sneaking without being detected */
    sh_int hide;     /**< Alters the success of hiding out of sight */
};

/** Describes the bonuses applied for a specific value of a character's
 * strength attribute. */
struct dex_app_type
{
    sh_int reaction;  /**< Historically affects reaction savings throws. */
    sh_int miss_att;  /**< Historically affects missile attacks */
    sh_int defensive; /**< Alters character's inherent armor class */
};

/** Describes the bonuses applied for a specific value of a character's
 * strength attribute. */
struct str_app_type
{
    sh_int tohit;   /**< To Hit (THAC0) Bonus/Penalty        */
    sh_int todam;   /**< Damage Bonus/Penalty                */
    sh_int carry_w; /**< Maximum weight that can be carrried */
    sh_int wield_w; /**< Maximum weight that can be wielded  */
};

/** Describes the bonuses applied for a specific value of a character's
 * wisdom attribute. */
struct wis_app_type
{
    byte bonus; /**< how many practices player gains per lev */
};

/** Describes the bonuses applied for a specific value of a character's
 * intelligence attribute. */
struct int_app_type
{
    byte learn; /**< how many % a player learns a spell/skill */
};

/** Describes the bonuses applied for a specific value of a
 * character's constitution attribute. */
struct con_app_type
{
    sh_int hitp; /**< Added to a character's new MAXHP at each new level. */
};

/** Describes the bonuses applied for a specific value of a
 * character's charisma attribute. */
struct cha_app_type
{
    sh_int cha_bonus; /* charisma bonus */
};

/** Stores, and used to deliver, the current weather information
 * in the mud world. */
struct weather_data
{
    int pressure; /**< How is the pressure ( Mb )? */
    int change;   /**< How fast and what way does it change? */
    int sky;      /**< How is the sky? */
    int sunlight; /**< And how much sun? */
};

/** Element in monster and object index-tables.
 NOTE: Assumes sizeof(mob_vnum) >= sizeof(obj_vnum) */
struct index_data
{
    mob_vnum vnum; /**< virtual number of this mob/obj   */
    int number;    /**< number of existing units of this mob/obj  */
    /** Point to any SPECIAL function assoicated with mob/obj.
     * Note: These are not trigger scripts. They are functions hard coded in
     * the source code. */
    SPECIAL_DECL(*func);

    char *farg;              /**< String argument for special function. */
    struct trig_data *proto; /**< Points to the trigger prototype. */
};

/** Master linked list for the mob/object prototype trigger lists. */
struct trig_proto_list
{
    int vnum;                     /**< vnum of the trigger   */
    struct trig_proto_list *next; /**< next trigger          */
};

struct guild_info_type
{
    int pc_class;
    room_vnum guild_room;
    int direction;
};

/* Staff Ran Event Data */
struct staffevent_struct
{
    int event_num;  /* index # reference for event happening */
    int ticks_left; /* time left for event */
    int delay;      /* time between the events */
};

/** Happy Hour Data */
struct happyhour
{
    int qp_rate;       // % increase in qp
    int exp_rate;      // % increase in exp
    int gold_rate;     // % increase in gold
    int treasure_rate; // % increase in treasure drop
    int ticks_left;    // time left for happyhour
};

/** structure for list of recent players (see 'recent' command) */
struct recent_player
{
    int vnum;                   /* The ID number for this instance */
    char name[MAX_NAME_LENGTH]; /* The char name of the player     */
    bool new_player;            /* Is this a new player?           */
    bool copyover_player;       /* Is this a player that was on during the last copyover? */
    time_t time;                /* login time                      */
    char host[HOST_LENGTH + 1]; /* Host IP address                 */
    struct recent_player *next; /* Pointer to the next instance    */
};

/* Config structs */

/** The game configuration structure used for configurating the game play
 * variables. */
struct game_data
{
    int pk_allowed;          /**< Is player killing allowed?    */
    int pt_allowed;          /**< Is player thieving allowed?   */
    int level_can_shout;     /**< Level player must be to shout.   */
    int holler_move_cost;    /**< Cost to holler in move points.    */
    int tunnel_size;         /**< Number of people allowed in a tunnel.*/
    int max_exp_gain;        /**< Maximum experience gainable per kill.*/
    int max_exp_loss;        /**< Maximum experience losable per death.*/
    int max_npc_corpse_time; /**< Num tics before NPC corpses decompose*/
    int max_pc_corpse_time;  /**< Num tics before PC corpse decomposes.*/
    int idle_void;           /**< Num tics before PC sent to void(idle)*/
    int idle_rent_time;      /**< Num tics before PC is autorented.   */
    int idle_max_level;      /**< Level of players immune to idle.     */
    int dts_are_dumps;       /**< Should items in dt's be junked?   */
    int load_into_inventory; /**< Objects load in immortals inventory. */
    int track_through_doors; /**< Track through doors while closed?    */
    int no_mort_to_immort;   /**< Prevent mortals leveling to imms?    */
    int disp_closed_doors;   /**< Display closed doors in autoexit?    */
    int diagonal_dirs;       /**< Are there 6 or 10 directions? */
    int map_option;          /**< MAP_ON, MAP_OFF or MAP_IMM_ONLY      */
    int map_size;            /**< Default size for map command         */
    int minimap_size;        /**< Default size for mini-map (automap)  */
    int script_players;      /**< Is attaching scripts to players allowed? */
    float min_pop_to_claim;  /**< Minimum popularity percentage required to claim a zone */

    char *OK;       /**< When player receives 'Okay.' text.    */
    char *NOPERSON; /**< 'No one by that name here.'   */
    char *NOEFFECT; /**< 'Nothing seems to happen.'            */
};

// automatic hour happy info saved in game config, cedit
struct happy_hour_data
{
    int qp;       // percent increase in number of qp
    int exp;      // percent increase in exp
    int gold;     // percent increase in gold
    int treasure; // percent increase in random treasure chance
    int chance;   // percent chance the happy hour will occur each rl hour
};

/** The rent and crashsave options. */
struct crash_save_data
{
    int free_rent;          /**< Should the MUD allow rent for free?   */
    int max_obj_save;       /**< Max items players can rent.           */
    int min_rent_cost;      /**< surcharge on top of item costs.       */
    int auto_save;          /**< Does the game automatically save ppl? */
    int autosave_time;      /**< if auto_save=TRUE, how often?         */
    int crash_file_timeout; /**< Life of crashfiles and idlesaves.     */
    int rent_file_timeout;  /**< Lifetime of normal rent files in days */
};

/** Important room numbers. This structure stores vnums, not real array
 * numbers. */
struct room_numbers
{
    room_vnum mortal_start_room;  /**< vnum of room that mortals enter at.  */
    room_vnum mortal_start_room2; /**< vnum of room that mortals enter at.  */
    room_vnum immort_start_room;  /**< vnum of room that immorts enter at.  */
    room_vnum frozen_start_room;  /**< vnum of room that frozen ppl enter.  */
    room_vnum donation_room_1;    /**< vnum of donation room #1.            */
    room_vnum donation_room_2;    /**< vnum of donation room #2.            */
    room_vnum donation_room_3;    /**< vnum of donation room #3.            */
};

/** Operational game variables. */
struct game_operation
{
    ush_int DFLT_PORT;        /**< The default port to run the game.  */
    char *DFLT_IP;            /**< Bind to all interfaces.     */
    char *DFLT_DIR;           /**< The default directory (lib).    */
    char *LOGNAME;            /**< The file to log messages to.    */
    int max_playing;          /**< Maximum number of players allowed. */
    int max_filesize;         /**< Maximum size of misc files.   */
    int max_bad_pws;          /**< Maximum number of pword attempts.  */
    int siteok_everyone;      /**< Everyone from all sites are SITEOK.*/
    int nameserver_is_slow;   /**< Is the nameserver slow or fast?   */
    int use_new_socials;      /**< Use new or old socials file ?      */
    int auto_save_olc;        /**< Does OLC save to disk right away ? */
    char *MENU;               /**< The MAIN MENU.        */
    char *WELC_MESSG;         /**< The welcome message.      */
    char *START_MESSG;        /**< The start msg for new characters.  */
    int medit_advanced;       /**< Does the medit OLC show the advanced stats menu ? */
    int ibt_autosave;         /**< Does "bug resolve" autosave ? */
    int protocol_negotiation; /**< Enable the protocol negotiation system ? */
    int special_in_comm;      /**< Enable use of a special character in communication channels ? */
    int debug_mode;           /**< Current Debug Mode */
};

/** The Autowizard options. */
struct autowiz_data
{
    int use_autowiz;     /**< Use the autowiz feature?   */
    int min_wizlist_lev; /**< Minimun level to show on wizlist.  */
};

struct player_config_data
{
    // spell damage.  This is the percent of extra damage done.
    // if 0, damage is normal.  If 20, damage will be 120% normal.
    // if -20, damage will be 80% normal.
    int psionic_power_damage_bonus;
    int divine_spell_damage_bonus;
    int arcane_spell_damage_bonus;

    int extra_hp_per_level;
    int extra_mv_per_level;

    // No player armor class can go above this value
    int armor_class_cap;

    // This is the maximum difference in level between the
    // level of the player and the mob, to gain exp.
    // If 3, and the player level is 20, the player will
    // gain exp over any mobs level 17+. Anything 16 or
    // less will yeild no exp.
    int group_level_difference_restriction;

    // these values apply to mobs created with any kind
    // of summoning spell, such as summon creature, dragon knight,
    // mummy dust, etc.
    // The value is a percentage of the normal values. If 100
    // then the stats are unchanged.  If 80, the stats are 80%
    // of normal.  if 120, the stats are 120% normal.
    int level_1_10_summon_hp;
    int level_1_10_summon_hit_and_dam;
    int level_1_10_summon_ac;
    int level_11_20_summon_hp;
    int level_11_20_summon_hit_and_dam;
    int level_11_20_summon_ac;
    int level_21_30_summon_hp;
    int level_21_30_summon_hit_and_dam;
    int level_21_30_summon_ac;

    // spell/power prep time modifiers
    int psionic_mem_times;
    int divine_mem_times;
    int arcane_mem_times;
    int alchemy_mem_times;

    // death penalty exp loss modifier
    int death_exp_loss_penalty;
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
struct config_data
{
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
    /** Automatic happy hour activation options */
    struct happy_hour_data happy_hour;
    /** player stat config data */
    struct player_config_data player_config;
};

#ifdef MEMORY_DEBUG
#include "zmalloc.h"
#endif

/* Action types */
typedef enum
{
    atSTANDARD,
    atMOVE,
    atSWIFT
} action_type;

#define NUM_ACTIONS 3

#define ACTION_NONE 0
#define ACTION_STANDARD (1 << 0)
#define ACTION_MOVE (1 << 1)
#define ACTION_SWIFT (1 << 2)

#define MAX_CHARS_PER_ACCOUNT 100

#define MAX_UNLOCKED_CLASSES 20
#define MAX_UNLOCKED_RACES 20

/* Account data structure.  Account data is kept in the database,
 * but loaded into this structure while the player is in-game. */
struct account_data
{
    int id;
    char *name;
    char password[MAX_PWD_LENGTH + 1];
    sbyte bad_password_count;
    char *character_names[MAX_CHARS_PER_ACCOUNT];
    ush_int experience;
    //        ush_int gift_experience;
    //        sbyte level;
    //        int account_flags;
    //        time_t last_login;
    //        sbyte read_rules;
    //        char * websiteAccount;
    //        byte polls[100];
    //        char * web_password;
    int classes[MAX_UNLOCKED_CLASSES];
    int races[MAX_UNLOCKED_RACES];
    char *email;
    //        int surveys[4];
    //        struct obj_data *item_bank;
    //        int item_bank_size;
    //        char *ignored[MAX_CHARS_PER_ACCOUNT];
};

/* structs - race data for extension of races */
struct race_data
{
    /* displaying the race */
    char *name;         /* lower case no-spaces (for like accessing help file) */
    char *type;         /* full capitalized and spaced version */
    char *type_color;   /* full colored, capitalized and spaced version */
    char *abbrev;       /* 4 letter abbreviation */
    char *abbrev_color; /* 4 letter abbreviation colored */

    /* extended race details */
    char *descrip;       /* race description */
    char *morph_to_char; /* wildshape message to ch */
    char *morph_to_room; /* wildshape message to room */

    /* race assigned values! */
    ubyte family;           /* race's family type (iron golem would be a CONSTRUCT) */
    byte size;              /* default size class for this race */
    bool is_pc;             /* can a PC select this race to play? */
    ubyte level_adjustment; /* for pc-races: penalty to xp for race due to power */
    int unlock_cost;        /* if locked, cost to unlock in account xp */
    byte epic_adv;          /* normal, advance or epic race (pc)? */

    /* array assigned values! */
    sbyte genders[NUM_SEX];              /* this race can be this sex? */
    byte ability_mods[NUM_ABILITY_MODS]; /* modifications to base stats based on race */
    sbyte alignments[NUM_ALIGNMENTS];    /* acceptable alignments for this race */
    byte attack_types[NUM_ATTACK_TYPES]; /* race have this attack type? (when not wielding) */

    /* linked lists */
    struct race_feat_assign *featassign_list; /* list of feat assigns */
    struct affect_assign *affassign_list;     /* list of affect assigns */

    /* these are only ideas for now */

    /*int body_parts[NUM_WEARS];*/   /* for expansion - to add customized wear slots */
    /*byte favored_class[NUM_SEX];*/ /* favored class system, not yet implemented */
    /*ush_int language;*/            /* default language - not used yet */
};

extern struct race_data race_list[];

#undef NUM_DAM_TYPES
#endif /* _STRUCTS_H_ */
