/**
* @file spells.h
* Constants and function prototypes for the spell system.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/
#ifndef _SPELLS_H_
#define _SPELLS_H_

#define DEFAULT_STAFF_LVL	12
#define DEFAULT_WAND_LVL	12

#define CAST_UNDEFINED	(-1)
#define CAST_SPELL	0
#define CAST_POTION	1
#define CAST_WAND	2
#define CAST_STAFF	3
#define CAST_SCROLL	4

#define MAG_DAMAGE	(1 << 0)
#define MAG_AFFECTS	(1 << 1)
#define MAG_UNAFFECTS	(1 << 2)
#define MAG_POINTS	(1 << 3)
#define MAG_ALTER_OBJS	(1 << 4)
#define MAG_GROUPS	(1 << 5)
#define MAG_MASSES	(1 << 6)
#define MAG_AREAS	(1 << 7)
#define MAG_SUMMONS	(1 << 8)
#define MAG_CREATIONS	(1 << 9)
#define MAG_MANUAL	(1 << 10)
#define MAG_ROOM	(1 << 11)


#define TYPE_UNDEFINED               (-1)
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */
#define SPELL_ARMOR                   1 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_TELEPORT                2 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLESS                   3 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLINDNESS               4 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BURNING_HANDS           5 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CALL_LIGHTNING          6 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHARM                   7 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHILL_TOUCH             8 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CLONE                   9 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_COLOR_SPRAY            10 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CONTROL_WEATHER        11 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_FOOD            12 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_WATER           13 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_BLIND             14 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_CRITIC            15 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_LIGHT             16 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURSE                  17 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_ALIGN           18 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_INVIS           19 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_MAGIC           20 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_POISON          21 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_EVIL            22 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EARTHQUAKE             23 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENCHANT_WEAPON         24 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENERGY_DRAIN           25 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FIREBALL               26 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HARM                   27 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL                   28 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INVISIBLE              29 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LIGHTNING_BOLT         30 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LOCATE_OBJECT          31 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MAGIC_MISSILE          32 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_POISON                 33 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_PROT_FROM_EVIL         34 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_CURSE           35 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SANCTUARY              36 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SHOCKING_GRASP         37 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SLEEP                  38 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STRENGTH               39 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SUMMON                 40 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VENTRILOQUATE          41 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WORD_OF_RECALL         42 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_POISON          43 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SENSE_LIFE             44 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ANIMATE_DEAD           45 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_GOOD            46 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_ARMOR            47 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_HEAL             48 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_RECALL           49 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INFRAVISION            50 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WATERWALK              51 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_IDENTIFY               52 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FLY                    53 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLUR                   54 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MIRROR_IMAGE           55 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STONESKIN              56 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENDURANCE              57 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MUMMY_DUST             58 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DRAGON_KNIGHT          59 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GREATER_RUIN           60 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HELLBALL               61 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EPIC_MAGE_ARMOR        62 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EPIC_WARDING           63 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CAUSE_LIGHT_WOUNDS	64
#define SPELL_CAUSE_MODERATE_WOUNDS	65
#define SPELL_CAUSE_SERIOUS_WOUNDS	66
#define SPELL_CAUSE_CRITICAL_WOUNDS	67
#define SPELL_FLAME_STRIKE		68
#define SPELL_DESTRUCTION		69
#define SPELL_ICE_STORM			70
#define SPELL_BALL_OF_LIGHTNING		71
#define SPELL_MISSILE_STORM		72
#define SPELL_CHAIN_LIGHTNING		73
#define SPELL_METEOR_SWARM		74
#define SPELL_PROT_FROM_GOOD		75
#define SPELL_FIRE_BREATHE		76
#define SPELL_POLYMORPH			77
#define SPELL_ENDURE_ELEMENTS		78
#define SPELL_EXPEDITIOUS_RETREAT	79
#define SPELL_GREASE			80
#define SPELL_HORIZIKAULS_BOOM		81
#define SPELL_ICE_DAGGER		82
#define SPELL_IRON_GUTS			83
#define SPELL_MAGE_ARMOR		84
#define SPELL_NEGATIVE_ENERGY_RAY	85
#define SPELL_RAY_OF_ENFEEBLEMENT	86
#define SPELL_SCARE			87
#define SPELL_SHELGARNS_BLADE		88
#define SPELL_SHIELD			89
#define SPELL_SUMMON_CREATURE_1		90
#define SPELL_TRUE_STRIKE		91
#define SPELL_WALL_OF_FOG		92
#define SPELL_DARKNESS			93
#define SPELL_SUMMON_CREATURE_2		94
#define SPELL_WEB			95
#define SPELL_ACID_ARROW		96
#define SPELL_DAZE_MONSTER		97
#define SPELL_HIDEOUS_LAUGHTER		98
#define SPELL_TOUCH_OF_IDIOCY		99
#define SPELL_CONTINUAL_FLAME		100
#define SPELL_SCORCHING_RAY		101
#define SPELL_DEAFNESS			102
#define SPELL_FALSE_LIFE		103
#define SPELL_GRACE			104
#define SPELL_RESIST_ENERGY		105
#define SPELL_ENERGY_SPHERE		106
#define SPELL_WATER_BREATHE        107
#define SPELL_PHANTOM_STEED        108
#define SPELL_STINKING_CLOUD       109
#define SPELL_SUMMON_CREATURE_3    110
#define SPELL_HALT_UNDEAD          111
#define SPELL_HEROISM              112
#define SPELL_VAMPIRIC_TOUCH       113
#define SPELL_HOLD_PERSON          114
#define SPELL_DEEP_SLUMBER         115
#define SPELL_INVISIBILITY_SPHERE  116
#define SPELL_DAYLIGHT             117
#define SPELL_CLAIRVOYANCE         118
#define SPELL_NON_DETECTION        119
#define SPELL_HASTE                120
#define SPELL_SLOW                 121
#define SPELL_DISPEL_MAGIC         122
#define SPELL_CIRCLE_A_EVIL        123
#define SPELL_CIRCLE_A_GOOD        124
#define SPELL_CUNNING              125
#define SPELL_WISDOM               126
#define SPELL_CHARISMA             127
#define SPELL_STENCH               128
/** Total Number of defined spells */
#define NUM_SPELLS    129

/* Insert new spells here, up to MAX_SPELLS */
#define MAX_SPELLS		    400

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB			401
#define SKILL_BASH			402
#define SKILL_MUMMY_DUST		403
#define SKILL_KICK			404
#define SKILL_WEAPON_SPECIALIST         405
#define SKILL_WHIRLWIND			406
#define SKILL_RESCUE			407
#define SKILL_DRAGON_KNIGHT		408
#define SKILL_LUCK_OF_HEROES            409
#define SKILL_TRACK			410
#define SKILL_QUICK_CHANT		411
#define SKILL_AMBIDEXTERITY		412
#define SKILL_DIRTY_FIGHTING		413
#define SKILL_DODGE			414
#define SKILL_IMPROVED_CRITICAL		415
#define SKILL_MOBILITY			416
#define SKILL_SPRING_ATTACK		417
#define SKILL_TOUGHNESS			418
#define SKILL_TWO_WEAPON_FIGHT		419
#define SKILL_FINESSE			420
#define SKILL_ARMOR_SKIN                421
#define SKILL_BLINDING_SPEED            422
#define SKILL_DAMAGE_REDUC_1            423
#define SKILL_DAMAGE_REDUC_2            424
#define SKILL_DAMAGE_REDUC_3            425
#define SKILL_EPIC_TOUGHNESS            426
#define SKILL_OVERWHELMING_CRIT         427
#define SKILL_SELF_CONCEAL_1            428
#define SKILL_SELF_CONCEAL_2            429
#define SKILL_SELF_CONCEAL_3            430
#define SKILL_TRIP			431
#define SKILL_IMPROVED_WHIRL		432
#define SKILL_CLEAVE			433
#define SKILL_GREAT_CLEAVE		434
#define SKILL_SPELLPENETRATE		435
#define SKILL_SPELLPENETRATE_2		436
#define SKILL_PROWESS			437
#define SKILL_EPIC_PROWESS		438
#define SKILL_EPIC_2_WEAPON		439
#define SKILL_SPELLPENETRATE_3		440
#define SKILL_SPELL_RESIST_1		441
#define SKILL_SPELL_RESIST_2		442
#define SKILL_SPELL_RESIST_3		443
#define SKILL_SPELL_RESIST_4		444
#define SKILL_SPELL_RESIST_5		445
#define SKILL_INITIATIVE		446
#define SKILL_EPIC_CRIT			447
#define SKILL_IMPROVED_BASH		448
#define SKILL_IMPROVED_TRIP		449
#define SKILL_POWER_ATTACK		450
#define SKILL_EXPERTISE			451
#define SKILL_GREATER_RUIN		452
#define SKILL_HELLBALL			453
#define SKILL_EPIC_MAGE_ARMOR		454
#define SKILL_EPIC_WARDING		455
#define SKILL_RAGE			456
#define SKILL_PROF_MINIMAL		457
#define SKILL_PROF_BASIC		458
#define SKILL_PROF_ADVANCED		459
#define SKILL_PROF_MASTER		460
#define SKILL_PROF_EXOTIC		461
#define SKILL_PROF_LIGHT_A		462
#define SKILL_PROF_MEDIUM_A		463
#define SKILL_PROF_HEAVY_A		464
#define SKILL_PROF_SHIELDS		465
#define SKILL_PROF_T_SHIELDS		466
#define SKILL_MURMUR                    467 /* Murmur     diplomacy skill */
#define SKILL_PROPAGANDA                468 /* Propaganda diplomacy skill */
#define SKILL_LOBBY                     469 /* Lobby      diplomacy skill */
#define SKILL_STUNNING_FIST             470
/* initial crafting skills */
#define SKILL_MINING                    471
#define SKILL_HUNTING                   472
#define SKILL_FORESTING                 473
#define SKILL_KNITTING                  474
#define SKILL_CHEMISTRY                 475
#define SKILL_ARMOR_SMITHING            476
#define SKILL_WEAPON_SMITHING           477
#define SKILL_JEWELRY_MAKING            478
#define SKILL_LEATHER_WORKING           479
#define SKILL_FAST_CRAFTER              480
#define SKILL_BONE_ARMOR                481
#define SKILL_ELVEN_CRAFTING             482
#define SKILL_MASTERWORK_CRAFTING        483
#define SKILL_DRACONIC_CRAFTING          484
#define SKILL_DWARVEN_CRAFTING          485
/* finish batch crafting skills */
#define SKILL_LIGHTNING_REFLEXES        486
#define SKILL_GREAT_FORTITUDE           487
#define SKILL_IRON_WILL                 488
#define SKILL_EPIC_REFLEXES             489
#define SKILL_EPIC_FORTITUDE            490
#define SKILL_EPIC_WILL                 491
#define SKILL_SHIELD_SPECIALIST         492
/* New skills may be added here up to MAX_SKILLS (600) */
#define NUM_SKILLS                      493

/* NON-PLAYER AND OBJECT SPELLS AND SKILLS: The practice levels for the spells
 * and skills below are _not_ recorded in the players file; therefore, the
 * intended use is for spells and skills associated with objects (such as
 * SPELL_IDENTIFY used with scrolls of identify) or non-players (such as NPC
 * only spells). */

/* To make an affect induced by dg_affect look correct on 'stat' we need to
 * define it with a 'spellname'. */
#define SPELL_DG_AFFECT              698

#define TOP_SPELL_DEFINE	     699
/* NEW NPC/OBJECT SPELLS can be inserted here up to 699 */

/* WEAPON ATTACK TYPES */
#define TYPE_HIT        700
#define TYPE_STING      701
#define TYPE_WHIP       702
#define TYPE_SLASH      703
#define TYPE_BITE       704
#define TYPE_BLUDGEON   705
#define TYPE_CRUSH      706
#define TYPE_POUND      707
#define TYPE_CLAW       708
#define TYPE_MAUL       709
#define TYPE_THRASH     710
#define TYPE_PIERCE     711
#define TYPE_BLAST		  712
#define TYPE_PUNCH		  713
#define TYPE_STAB		    714
/** The total number of attack types */
#define NUM_ATTACK_TYPES  15

/* other attack types */
#define TYPE_SUFFERING		799
/* new attack types can be added here - up to TYPE_SUFFERING */
#define MAX_TYPES		800

/*---------------------------ABILITIES-------------------------------------*/

/* PLAYER ABILITIES -- Numbered from 1 to MAX_ABILITIES */
#define ABILITY_TUMBLE			1 // tumble
#define ABILITY_HIDE			2 // hide
#define ABILITY_SNEAK			3 // sneak
#define ABILITY_SPOT			4 // spot
#define ABILITY_LISTEN			5 // listen
#define ABILITY_TREAT_INJURY		6 // treat injuries
#define ABILITY_TAUNT			7 // taunt
#define ABILITY_CONCENTRATION		8 // concentration
#define ABILITY_SPELLCRAFT		9 // spellcraft
#define ABILITY_APPRAISE		10 // appraise
#define ABILITY_DISCIPLINE		11 // discipline
#define ABILITY_PARRY			12 // parry
#define ABILITY_LORE			13 // lore
#define ABILITY_MOUNT			14
#define ABILITY_RIDING			15 //mounts
#define ABILITY_TAME			16 //mounts
#define ABILITY_PICK_LOCK		17 //mounts
#define ABILITY_STEAL			18 //mounts

#define NUM_ABILITIES			19 /* Number of defined abilities */
/*	MAX_ABILITIES = 200 */ 
/*-------------------------------------------------------------------------*/


// ******** DAM_ *********
#define DAM_RESERVED_DBC	0	//reserve
#define DAM_FIRE		1
#define DAM_COLD		2
#define DAM_AIR			3
#define DAM_EARTH		4
#define DAM_ACID		5
#define DAM_HOLY		6
#define DAM_ELECTRIC		7
#define DAM_UNHOLY		8
#define DAM_SLICE		9
#define DAM_PUNCTURE		10
#define DAM_FORCE		11
#define DAM_SOUND		12
#define DAM_POISON		13
#define DAM_DISEASE		14
#define DAM_NEGATIVE		15
#define DAM_ILLUSION		16
#define DAM_MENTAL		17
#define DAM_LIGHT		18
#define DAM_ENERGY		19
/* ------------------------------*/
#define NUM_DAM_TYPES		20
/* =============================*/


/*********************************/
/******** Schools of Magic *******/
/*********************************/
#define NOSCHOOL	0	// non magical spells
#define ABJURATION	1
#define CONJURATION	2
#define DIVINATION	3
#define ENCHANTMENT	4
#define EVOCATION	5
#define ILLUSION	6
#define NECROMANCY	7
#define TRANSMUTATION	8

#define NUM_SCHOOLS	9
/*--------------------------------*/


/************************************/
/********** Saves ******************/
/************************************/
#define SAVING_FORT   0
#define SAVING_REFL   1
#define SAVING_WILL   2
#define SAVING_POISON 3
#define SAVING_DEATH  4
/*--------------------------------------*/


/***
 **Possible Targets:
 **  TAR_IGNORE    : IGNORE TARGET.
 **  TAR_CHAR_ROOM : PC/NPC in room.
 **  TAR_CHAR_WORLD: PC/NPC in world.
 **  TAR_FIGHT_SELF: If fighting, and no argument, select tar_char as self.
 **  TAR_FIGHT_VICT: If fighting, and no argument, select tar_char as victim (fighting).
 **  TAR_SELF_ONLY : If no argument, select self, if argument check that it IS self.
 **  TAR_NOT_SELF  : Target is anyone else besides self.
 **  TAR_OBJ_INV   : Object in inventory.
 **  TAR_OBJ_ROOM  : Object in room.
 **  TAR_OBJ_WORLD : Object in world.
 **  TAR_OBJ_EQUIP : Object held.
 ***/
#define TAR_IGNORE      (1 << 0)
#define TAR_CHAR_ROOM   (1 << 1)
#define TAR_CHAR_WORLD  (1 << 2)
#define TAR_FIGHT_SELF  (1 << 3)
#define TAR_FIGHT_VICT  (1 << 4)
#define TAR_SELF_ONLY   (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF   	(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     (1 << 7)
#define TAR_OBJ_ROOM    (1 << 8)
#define TAR_OBJ_WORLD   (1 << 9)
#define TAR_OBJ_EQUIP	  (1 << 10)


struct spell_info_type {
   byte min_position;	/* Position for caster	 */
   int mana_min;	/* Min amount of mana used by a spell (highest lev) */
   int mana_max;	/* Max amount of mana used by a spell (lowest lev) */
   int mana_change;	/* Change in mana used by spell from lev to lev */

   int min_level[NUM_CLASSES];
   int routines;
   byte violent;
   int targets;         /* See below for use with TAR_XXX  */
   const char *name;	/* Input size not limited. Originates from string constants. */
   const char *wear_off_msg;	/* Input size not limited. Originates from string constants. */
   int time;  /* casting time */
   int memtime;  /* mem time */
   int schoolOfMagic;	// school of magic
};

/* Possible Targets:
   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self. */

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4


#define ASPELL(spellname) \
void	spellname(int level, struct char_data *ch, \
		  struct char_data *victim, struct obj_data *obj)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict);

ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_polymorph);
ASPELL(spell_charm);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);
ASPELL(spell_acid_arrow);


/* basic magic calling functions */
int find_skill_num(char *name);
int find_ability_num(char *name);
int mag_damage(int level, struct char_data *ch, struct char_data *victim,
  struct obj_data *obj, int spellnum, int savetype);
void mag_affects(int level, struct char_data *ch, struct char_data *victim,
  struct obj_data *obj, int spellnum, int savetype);
void mag_groups(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum, int savetype);
void mag_masses(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum, int savetype);
void mag_areas(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum, int savetype);
void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
 int spellnum, int savetype);
void mag_points(int level, struct char_data *ch, struct char_data *victim,
        struct obj_data *obj, int spellnum, int savetype);
void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
  struct obj_data *obj, int spellnum, int type);
void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
  int spellnum, int type);
void mag_creations(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum);
void mag_room(int level, struct char_data *ch, struct obj_data *obj,
        int spellnum);
int	call_magic(struct char_data *caster, struct char_data *cvict,
  struct obj_data *ovict, int spellnum, int level, int casttype);
void	mag_objectmagic(struct char_data *ch, struct obj_data *obj,
			char *argument);
int	cast_spell(struct char_data *ch, struct char_data *tch,
  struct obj_data *tobj, int spellnum);


/* other prototypes */
void spell_level(int spell, int chclass, int level);
void init_spell_levels(void);
const char *skill_name(int num);


/* From magic.c */
int compute_mag_saves(struct char_data *vict,
	int type, int modifier);
int mag_savingthrow(struct char_data *ch, struct char_data *vict,
	int type, int modifier);
void affect_update(void);
int mag_resistance(struct char_data *ch, struct char_data *vict, int modifier);
int compute_spell_res(struct char_data *ch, struct char_data *vict, int mod);
int aoeOK(struct char_data *ch, struct char_data *tch, int spellnum);


// memorize.c
int forgetSpell(struct char_data *ch, int spellnum, int mode);
void addSpellMemming(struct char_data *ch, int spellnum, int time, int mode);
bool hasSpell(struct char_data *ch, int spellnum);
int spellCircle(int class, int spellnum);
int getCircle(struct char_data *ch, int class);
void init_spell_slots(struct char_data *ch);


/* from spell_parser.c */
ACMD(do_abort);
ACMD(do_cast);
void unused_spell(int spl);
void mag_assign_spells(void);
void resetCastingData(struct char_data *ch);


/* Global variables exported */
#ifndef __SPELL_PARSER_C__

extern struct spell_info_type spell_info[];
extern char cast_arg2[];
extern const char *unused_spellname;

#endif /* __SPELL_PARSER_C__ */


#endif /* _SPELLS_H_ */
