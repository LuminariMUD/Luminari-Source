/**
* @file pfdefaults.h                                    LuminariMUD
* ASCII player file defaults.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* This set of code was not originally part of the circlemud distribution.
*/
#ifndef _PFDEFAULTS_H_
#define _PFDEFAULTS_H_

/* WARNING:  Do not change the values below if you have existing ascii player
 * files you don't want to screw up. */

#define PFDEF_SIZE -1
#define PFDEF_DOMAIN_1 0
#define PFDEF_DOMAIN_2 0
#define PFDEF_SPECIALTY_SCHOOL 0
#define PFDEF_PREFERRED_ARCANE -1
#define PFDEF_PREFERRED_DIVINE -1
#define PFDEF_RESTRICTED_SCHOOL_1 0
#define PFDEF_RESTRICTED_SCHOOL_2 0
#define PFDEF_BOOSTS 0
#define PFDEF_MORPHED 0
#define PFDEF_SEX 0
#define PFDEF_CLASS 0
#define PFDEF_LEVEL 0
#define PFDEF_HEIGHT 0
#define PFDEF_WEIGHT 0
#define PFDEF_ALIGNMENT 0
#define PFDEF_PLRFLAGS 0
#define PFDEF_AFFFLAGS 0
#define PFDEF_SAVETHROW 0
#define PFDEF_RESISTANCES 0
#define PFDEF_LOADROOM 0
#define PFDEF_INVISLEV 0
#define PFDEF_FREEZELEV 0
#define PFDEF_WIMPLEV 0
#define PFDEF_CONDITION 0
#define PFDEF_BADPWS 0
#define PFDEF_PREFFLAGS 0
#define PFDEF_PRACTICES 0
#define PFDEF_TRAINS 0
#define PFDEF_GOLD 0
#define PFDEF_BANK 0
#define PFDEF_EXP 0
#define PFDEF_HITROLL 0
#define PFDEF_DAMROLL 0
#define PFDEF_SPELL_RES 0
/* this probably really should be 100 (10 AC), but we don't want to mess up
 * the ASCII pfiles */
#define PFDEF_AC 0
#define PFDEF_STR 0
#define PFDEF_STRADD 0
#define PFDEF_DEX 0
#define PFDEF_INT 0
#define PFDEF_WIS 0
#define PFDEF_CON 0
#define PFDEF_CHA 0
#define PFDEF_HIT 0
#define PFDEF_MAXHIT 0
#define PFDEF_PSP 0
#define PFDEF_MAXPSP 0
#define PFDEF_MOVE 0
#define PFDEF_MAXMOVE 0
#define PFDEF_HUNGER 0
#define PFDEF_THIRST 0
#define PFDEF_DRUNK 0
#define PFDEF_OLC NOWHERE
#define PFDEF_PAGELENGTH 40
#define PFDEF_SCREENWIDTH 80
#define PFDEF_QUESTPOINTS 0
#define PFDEF_STAFFRAN_EVENT_VAR 0
#define PFDEF_QUESTCOUNT 0
#define PFDEF_COMPQUESTS 0
#define PFDEF_CURRQUEST NOTHING
#define PFDEF_LASTMOTD 0
#define PFDEF_LASTNEWS 0
#define PFDEF_RACE 0
#define PFDEF_CLAN 0
#define PFDEF_CLANRANK 0
#define PFDEF_CLANPOINTS 0
#define PFDEF_DIPTIMER 0
#define PFDEF_AUTOCQUEST_VNUM 0
#define PFDEF_AUTOCQUEST_MAKENUM 0
#define PFDEF_AUTOCQUEST_QP 0
#define PFDEF_AUTOCQUEST_EXP 0
#define PFDEF_AUTOCQUEST_GOLD 0
#define PFDEF_AUTOCQUEST_DESC NULL
#define PFDEF_AUTOCQUEST_MATERIAL 0
#define PFDEF_SORC_BLOODLINE_SUBTYPE 0
#define PFDEF_TEMPLATE 0
#define PFDEF_PREMADE_BUILD -1

#endif /* _PFDEFAULTS_H_ */
