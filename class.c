/**************************************************************************
*  File: class.c                                           Part of tbaMUD *
*  Usage: Source file for class-specific code.                            *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/** Help buffer the global variable definitions */
#define __CLASS_C__

/* This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class, you
 * should go through this entire file from beginning to end and add the
 * appropriate new special cases for your new class. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "handler.h"
#include "comm.h"
#include "spells.h"

/* Names first */
const char *class_abbrevs[] = {
  "\tYMag\tn",
  "\tBCle\tn",
  "\tWThi\tn",
  "\tRWar\tn",
  "\tgMnk\tn",
  "\tGD\tgr\tGu\tn",
  "\rB\tRz\trk\tn",
  "\n"
};

const char *pc_class_types[] = {
  "Magic User",
  "Cleric",
  "Thief",
  "Warrior",
  "Monk",
  "Druid",
  "Berserker",
  "\n"
};

/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n"
"  \tbRea\tclms \tWof Lu\tcmin\tbari\tn | class selection\r\n"
"---------------------+\r\n"
"  c)  \tBCleric\tn\r\n"
"  t)  \tWThief\tn\r\n"
"  w)  \tRWarrior\tn\r\n"
"  o)  \tgMonk\tn\r\n"
"  d)  \tGD\tgr\tGu\tgi\tGd\tn\r\n"
"  m)  \tYMagic User\tn\r\n"
"  b)  \trBer\tRser\trker\tn\r\n";


/* The code to interpret a class letter -- used in interpreter.c when a new
 * character is selecting a class and by 'set class' in act.wizard.c. */
int parse_class(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'm': return CLASS_MAGIC_USER;
  case 'c': return CLASS_CLERIC;
  case 'w': return CLASS_WARRIOR;
  case 't': return CLASS_THIEF;
  case 'o': return CLASS_MONK;
  case 'd': return CLASS_DRUID;
  case 'b': return CLASS_BERSERKER;
  default:  return CLASS_UNDEFINED;
  }
}

/* bitvectors (i.e., powers of two) for each class, mainly for use in do_who
 * and do_users.  Add new classes at the end so that all classes use sequential
 * powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, etc.) up to
 * the limit of your bitvector_t, typically 0-31. */
bitvector_t find_class_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_class(arg[rpos]));

  return (ret);
}

/* These are definitions which control the guildmasters for each class.
 * The  first field (top line) controls the highest percentage skill level a
 * character of the class is allowed to attain in any skill.  (After this
 * level, attempts to practice will say "You are already learned in this area."
 *
 * The second line controls the maximum percent gain in learnedness a character
 * is allowed per practice -- in other words, if the random die throw comes out
 * higher than this number, the gain will only be this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a character
 * is allowed per practice -- in other words, if the random die throw comes
 * out below this number, the gain will be set up to this number.
 *
 * The fourth line simply sets whether the character knows 'spells' or 'skills'.
 * This does not affect anything except the message given to the character when
 * trying to practice (i.e. "You know of the following spells" vs. "You know of
 * the following skills" */

#define SP	0
#define SK	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {
 /* MG  CL  TH	 WR  MN  DR  BK*/
  { 75, 75, 75, 75, 75, 75, 75 },	/* learned level */
  { 75, 75, 75, 75, 75, 75, 75 },	/* max per practice */
  { 75, 75, 75, 75, 75, 75, 75 },	/* min per practice */
  { SP, SP, SK, SK, SK, SP, SK },	/* prac name */
};
#undef SP
#undef SK
/* The appropriate rooms for each guildmaster/guildguard; controls which types
 * of people the various guildguards let through.  i.e., the first line shows
 * that from room 3017, only MAGIC_USERS are allowed to go south. Don't forget
 * to visit spec_assign.c if you create any new mobiles that should be a guild
 * master or guard so they can act appropriately. If you "recycle" the
 * existing mobs that are used in other guilds for your new guild, then you
 * don't have to change that file, only here. Guildguards are now implemented
 * via triggers. This code remains as an example. */
struct guild_info_type guild_info[] = {

  /* Midgaard */
  { CLASS_MAGIC_USER,  3017,	SOUTH },
  { CLASS_CLERIC,	   3004,	NORTH },
  { CLASS_DRUID,	   3004,	NORTH },
  { CLASS_MONK,	   3004,	NORTH },
  { CLASS_THIEF,	   3027,	EAST  },
  { CLASS_WARRIOR,	   3021,	EAST  },
  { CLASS_BERSERKER,   3021,	EAST  },

  /* Brass Dragon */
  { -999 /* all */ ,	5065,	WEST	},

/* this must go last -- add new guards above! */
  { -1, NOWHERE, -1}
};


/* This array determines whether an ability is cross-class or a class-ability
 * based on class of the character */
#define		NA	0	//not available
#define		CC	1	//cross class
#define		CA	2	//class ability
int class_ability[NUM_ABILITIES][NUM_CLASSES] = {
//  MU  CL  TH  WA  Mo  Dr
  { -1, -1, -1, -1, -1, -1, -1 }, //0 - reserved

  { CC, CC, CA, CC, CA, CC, CC },	//1 - Tumble 
  { CC, CC, CA, CC, CA, CC, CC },	//2 - hide
  { CC, CC, CA, CC, CA, CC, CC },	//3 sneak
  { CC, CC, CA, CC, CA, CC, CC },	//4 spot
  { CC, CC, CA, CC, CA, CC, CA },	//5 listen
  { CA, CA, CA, CA, CA, CA, CA },	//6 treat injury
  { CC, CC, CC, CC, CC, CC, CA },	//7 taunt
  { CA, CA, CC, CA, CA, CA, CC },	//8 concentration
  { CA, CA, CC, CC, CC, CA, CC },	//9 spellcraft
  { CC, CC, CA, CC, CC, CC, CC },	//10 appraise
  { CC, CC, CC, CA, CC, CC, CA },	//11 discipline
  { CC, CA, CA, CA, CA, CA, CA },	//12 parry
  { CA, CA, CA, CA, CA, CA, CA },	//13 lore
  { CA, CA, CA, CA, CA, CA, CA },	//14 mount
  { CA, CA, CA, CA, CA, CA, CA },	//15 riding
  { CA, CA, CA, CA, CA, CA, CA },	//16 tame
  { NA, NA, CA, NA, NA, NA, NA },	//17 pick locks
  { NA, NA, CA, NA, NA, NA, NA },	//18 steal
};
#undef NA
#undef CC
#undef CA


// Saving Throw System
#define		H	1	//high
#define		L	0	//low
int preferred_save[5][NUM_CLASSES] = {
//           MU CL TH WA Mo Dr Bk
/*fort */  { L, H, L, H, H, H, H },
/*refl */  { L, L, H, L, H, L, L },
/*will */  { H, H, L, L, H, H, L },
/*psn  */  { L, L, L, L, L, L, L },
/*death*/  { L, L, L, L, L, L, L },
};
// fortitude / reflex / will ( poison / death )
byte saving_throws(struct char_data *ch, int type)
{
  int i, save = 1;

  for (i = 0; i < MAX_CLASSES; i++) {
    if (CLASS_LEVEL(ch, i)) {  // found class and level
      if (preferred_save[type][i])
        save += CLASS_LEVEL(ch, i) / 2;
      else
        save += CLASS_LEVEL(ch, i) / 4;
    }
  }

  return save;
}
#undef H
#undef L


// base attack bonus, replacement for THAC0 system
int BAB(struct char_data *ch)
{
  int i, bab = 0, level;

  if (IS_NPC(ch))  //npc's default to medium attack rolls
    return ( (int) (GET_LEVEL(ch) * 3 / 4) );

  for (i = 0; i < MAX_CLASSES; i++) {
    level = CLASS_LEVEL(ch, i);
    if (level) {
      switch (i) {
        case CLASS_MAGIC_USER:
          bab += level / 2;
          break;
        case CLASS_THIEF:
        case CLASS_CLERIC:
        case CLASS_DRUID:
        case CLASS_MONK:
          bab += level * 3 / 4;
          break;
        case CLASS_WARRIOR:
        case CLASS_BERSERKER:
          bab += level;
          break;
      }
    }
  }

  if (bab == -1)
    log("ERROR:  BAB returning -1");
  return bab;
}


// old random roll system abandoned for base statas + point distribution
void roll_real_abils(struct char_data *ch)
{

  ch->real_abils.str = 12;
  ch->real_abils.con = 12;
  ch->real_abils.dex = 12;
  ch->real_abils.intel = 12;
  ch->real_abils.wis = 12;
  ch->real_abils.cha = 12;

  ch->aff_abils = ch->real_abils;

}


//   give newbie's some eq to start with
void newbieEquipment(struct char_data *ch)
{  
  int objNums[] = { 858, 858, 804, 804, 804, 804, 803, 857, -1 };
  int x;
 
  send_to_char(ch, "\tMYou are given a set of starting equipment...\tn\r\n");

  // give everyone torch, rations, skin, backpack
  for (x = 0; objNums[x] != -1; x++)
    obj_to_char(read_object(objNums[x], VIRTUAL), ch);
  
  switch (GET_CLASS(ch))
  {
    case CLASS_CLERIC:
      // holy symbol
      obj_to_char(read_object(854, VIRTUAL), ch);       // leather sleeves
      obj_to_char(read_object(855, VIRTUAL), ch);       // leather pants
      obj_to_char(read_object(861, VIRTUAL), ch);       // heavy mace
      obj_to_char(read_object(863, VIRTUAL), ch);       // small shield   
      obj_to_char(read_object(807, VIRTUAL), ch);       // scale mail   
      break;
    case CLASS_BERSERKER:
    case CLASS_WARRIOR:
      obj_to_char(read_object(854, VIRTUAL), ch);       // leather sleeves
      obj_to_char(read_object(855, VIRTUAL), ch);       // leather pants
      if (GET_RACE(ch) == RACE_DWARF)
        obj_to_char(read_object(806, VIRTUAL), ch);     // waraxe
      else
        obj_to_char(read_object(808, VIRTUAL), ch);     // bastard sword  
      obj_to_char(read_object(863, VIRTUAL), ch);       // small shield 
      obj_to_char(read_object(807, VIRTUAL), ch);       // scale mail
      break;
    case CLASS_MONK:
      obj_to_char(read_object(809, VIRTUAL), ch);       // cloth robes
      break;
    case CLASS_THIEF:
      obj_to_char(read_object(854, VIRTUAL), ch);       // leather sleeves
      obj_to_char(read_object(855, VIRTUAL), ch);       // leather pants
      obj_to_char(read_object(851, VIRTUAL), ch);       // studded leather
      obj_to_char(read_object(852, VIRTUAL), ch);       // dagger
      obj_to_char(read_object(852, VIRTUAL), ch);       // dagger
      break;
    case CLASS_MAGIC_USER:
      obj_to_char(read_object(854, VIRTUAL), ch);       // leather sleeves
      obj_to_char(read_object(855, VIRTUAL), ch);       // leather pants  
      obj_to_char(read_object(852, VIRTUAL), ch);       // dagger
      obj_to_char(read_object(809, VIRTUAL), ch);       // cloth robes
      break;
    default:
      log("Invalid class sent to newbieEquipment!");
      break;
  }
}

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 */
void berserker_skills(struct char_data *ch, int level) {
  switch (level) {
    case 2:
      SET_SKILL(ch, SKILL_RAGE, 75);
      send_to_char(ch, "\tMYou have learned 'Rage'\tn\r\n");
      break;
    default:
      break;
  }
  return;  
}

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 */
void thief_skills(struct char_data *ch, int level) {
  switch (level) {
    case 2:
      SET_SKILL(ch, SKILL_MOBILITY, 75);
      send_to_char(ch, "\tMYou have learned 'Mobility'\tn\r\n");
      break;
    case 3:
      SET_SKILL(ch, SKILL_DIRTY_FIGHTING, 75);
      send_to_char(ch, "\tMYou have learned 'Dirty Fighting'\tn\r\n");
      break;
    case 6:
      SET_SKILL(ch, SKILL_SPRING_ATTACK, 75);
      send_to_char(ch, "\tMYou have learned 'Spring Attack'\tn\r\n");
      break;
    case 10:
    case 13:
    case 16:
    case 19:
    default:
      break;
  }
  return;  
}

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 */
void monk_skills(struct char_data *ch, int level) {
  switch (level) {
    case 2:
      SET_SKILL(ch, SKILL_STUNNING_FIST, 75);
      send_to_char(ch, "\tMYou have learned 'Stunning Fist'\tn\r\n");
      break;

    default:
      break;
  }
  return;  
}

void init_class(struct char_data *ch, int class, int level)
{
  switch (class) {

  case CLASS_MAGIC_USER:
    SET_SKILL(ch, SKILL_PROF_SIMPLE_W, 75);

    SET_SKILL(ch, SPELL_MAGIC_MISSILE, 99);
    SET_SKILL(ch, SPELL_NEGATIVE_ENERGY_RAY, 99);
    SET_SKILL(ch, SPELL_EXPEDITIOUS_RETREAT, 99);
    SET_SKILL(ch, SPELL_GREASE, 99);
    SET_SKILL(ch, SPELL_ICE_DAGGER, 99);
    SET_SKILL(ch, SPELL_IRON_GUTS, 99);
    SET_SKILL(ch, SPELL_ENDURANCE, 99);
    SET_SKILL(ch, SPELL_DETECT_INVIS, 99);
    SET_SKILL(ch, SPELL_DETECT_MAGIC, 99);
    SET_SKILL(ch, SPELL_CHILL_TOUCH, 99);
    SET_SKILL(ch, SPELL_SCARE, 99);
    SET_SKILL(ch, SPELL_SHELGARNS_BLADE, 99);
    SET_SKILL(ch, SPELL_SHIELD, 99);
    SET_SKILL(ch, SPELL_SUMMON_CREATURE_1, 99);
    SET_SKILL(ch, SPELL_WEB, 99);
    SET_SKILL(ch, SPELL_SUMMON_CREATURE_2, 99);
    SET_SKILL(ch, SPELL_TRUE_STRIKE, 99);
    SET_SKILL(ch, SPELL_HORIZIKAULS_BOOM, 99);
    SET_SKILL(ch, SPELL_INFRAVISION, 99);
    SET_SKILL(ch, SPELL_INVISIBLE, 99);
    SET_SKILL(ch, SPELL_MAGE_ARMOR, 99);
    SET_SKILL(ch, SPELL_RAY_OF_ENFEEBLEMENT, 99);
    SET_SKILL(ch, SPELL_BURNING_HANDS, 99);
    SET_SKILL(ch, SPELL_LOCATE_OBJECT, 99);
    SET_SKILL(ch, SPELL_STRENGTH, 99);
    SET_SKILL(ch, SPELL_SHOCKING_GRASP, 99);
    SET_SKILL(ch, SPELL_SLEEP, 99);
    SET_SKILL(ch, SPELL_LIGHTNING_BOLT, 99);
    SET_SKILL(ch, SPELL_BLINDNESS, 99);
    SET_SKILL(ch, SPELL_DEAFNESS, 99);
    SET_SKILL(ch, SPELL_DETECT_POISON, 99);
    SET_SKILL(ch, SPELL_COLOR_SPRAY, 99);
    SET_SKILL(ch, SPELL_ENERGY_DRAIN, 99);
    SET_SKILL(ch, SPELL_CURSE, 99);
    SET_SKILL(ch, SPELL_POISON, 99);
    SET_SKILL(ch, SPELL_FIREBALL, 99);
    SET_SKILL(ch, SPELL_CHARM, 99);
    SET_SKILL(ch, SPELL_IDENTIFY, 99);
    SET_SKILL(ch, SPELL_FLY, 99);
    SET_SKILL(ch, SPELL_ENCHANT_WEAPON, 99);
    SET_SKILL(ch, SPELL_CLONE, 99);
    SET_SKILL(ch, SPELL_BLUR, 99);
    SET_SKILL(ch, SPELL_WALL_OF_FOG, 99);
    SET_SKILL(ch, SPELL_DARKNESS, 99);
    SET_SKILL(ch, SPELL_ENDURE_ELEMENTS, 99);
    SET_SKILL(ch, SPELL_RESIST_ENERGY, 99);
    SET_SKILL(ch, SPELL_MIRROR_IMAGE, 99);
    SET_SKILL(ch, SPELL_STONESKIN, 99);
    SET_SKILL(ch, SPELL_TELEPORT, 99);
    SET_SKILL(ch, SPELL_MISSILE_STORM, 99);
    SET_SKILL(ch, SPELL_ANIMATE_DEAD, 99);
    SET_SKILL(ch, SPELL_WATERWALK, 99);
    SET_SKILL(ch, SPELL_ICE_STORM, 99);
    SET_SKILL(ch, SPELL_BALL_OF_LIGHTNING, 99);
    SET_SKILL(ch, SPELL_CHAIN_LIGHTNING, 99);
    SET_SKILL(ch, SPELL_METEOR_SWARM, 99);
    SET_SKILL(ch, SPELL_POLYMORPH, 99);
    send_to_char(ch, "Magic-User Done.\tn\r\n");
  break;


  case CLASS_CLERIC:
    SET_SKILL(ch, SKILL_PROF_SIMPLE_W, 75);
    SET_SKILL(ch, SKILL_PROF_ELF_W, 75);
    SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);
    SET_SKILL(ch, SKILL_PROF_MEDIUM_A, 75);
    SET_SKILL(ch, SKILL_PROF_HEAVY_A, 75);
    SET_SKILL(ch, SKILL_PROF_SHIELDS, 75);

    SET_SKILL(ch, SPELL_ENDURANCE, 99);
    SET_SKILL(ch, SPELL_CURE_LIGHT, 99);
    SET_SKILL(ch, SPELL_ARMOR, 99);
    SET_SKILL(ch, SPELL_CREATE_FOOD, 99);
    SET_SKILL(ch, SPELL_CREATE_WATER, 99);
    SET_SKILL(ch, SPELL_DETECT_POISON, 99);
    SET_SKILL(ch, SPELL_DETECT_ALIGN, 99);
    SET_SKILL(ch, SPELL_CURE_BLIND, 99);
    SET_SKILL(ch, SPELL_BLESS, 99);
    SET_SKILL(ch, SPELL_DETECT_INVIS, 99);
    SET_SKILL(ch, SPELL_BLINDNESS, 99);
    SET_SKILL(ch, SPELL_INFRAVISION, 99);
    SET_SKILL(ch, SPELL_PROT_FROM_EVIL, 99);
    SET_SKILL(ch, SPELL_POISON, 99);
    SET_SKILL(ch, SPELL_GROUP_ARMOR, 99);
    SET_SKILL(ch, SPELL_CURE_CRITIC, 99);
    SET_SKILL(ch, SPELL_SUMMON, 99);
    SET_SKILL(ch, SPELL_REMOVE_POISON, 99);
    SET_SKILL(ch, SPELL_IDENTIFY, 99);
    SET_SKILL(ch, SPELL_WORD_OF_RECALL, 99);
    SET_SKILL(ch, SPELL_EARTHQUAKE, 99);
    SET_SKILL(ch, SPELL_DISPEL_EVIL, 99);
    SET_SKILL(ch, SPELL_DISPEL_GOOD, 99);
    SET_SKILL(ch, SPELL_SANCTUARY, 99);
    SET_SKILL(ch, SPELL_CALL_LIGHTNING, 99);
    SET_SKILL(ch, SPELL_HEAL, 99);
    SET_SKILL(ch, SPELL_CONTROL_WEATHER, 99);
    SET_SKILL(ch, SPELL_SENSE_LIFE, 99);
    SET_SKILL(ch, SPELL_HARM, 99);
    SET_SKILL(ch, SPELL_GROUP_HEAL, 99);
    SET_SKILL(ch, SPELL_REMOVE_CURSE, 99);
    SET_SKILL(ch, SPELL_CAUSE_LIGHT_WOUNDS, 99);
    SET_SKILL(ch, SPELL_CAUSE_MODERATE_WOUNDS, 99);
    SET_SKILL(ch, SPELL_CAUSE_SERIOUS_WOUNDS, 99);
    SET_SKILL(ch, SPELL_CAUSE_CRITICAL_WOUNDS, 99);
    SET_SKILL(ch, SPELL_FLAME_STRIKE, 99);
    SET_SKILL(ch, SPELL_DESTRUCTION, 99);
    send_to_char(ch, "Cleric Done.\tn\r\n");
  break;


  case CLASS_DRUID:
    SET_SKILL(ch, SKILL_PROF_SIMPLE_W, 75);
    SET_SKILL(ch, SKILL_PROF_ELF_W, 75);
    SET_SKILL(ch, SKILL_PROF_DRUID_W, 75);
    SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);
    SET_SKILL(ch, SKILL_PROF_MEDIUM_A, 75);
    SET_SKILL(ch, SKILL_PROF_SHIELDS, 75);

    SET_SKILL(ch, SPELL_ENDURANCE, 99);
    SET_SKILL(ch, SPELL_CURE_LIGHT, 99);
    SET_SKILL(ch, SPELL_ARMOR, 99);
    SET_SKILL(ch, SPELL_CREATE_FOOD, 99);
    SET_SKILL(ch, SPELL_CREATE_WATER, 99);
    SET_SKILL(ch, SPELL_DETECT_POISON, 99);
    SET_SKILL(ch, SPELL_DETECT_ALIGN, 99);
    SET_SKILL(ch, SPELL_CURE_BLIND, 99);
    SET_SKILL(ch, SPELL_BLESS, 99);
    SET_SKILL(ch, SPELL_DETECT_INVIS, 99);
    SET_SKILL(ch, SPELL_BLINDNESS, 99);
    SET_SKILL(ch, SPELL_INFRAVISION, 99);
    SET_SKILL(ch, SPELL_PROT_FROM_EVIL, 99);
    SET_SKILL(ch, SPELL_POISON, 99);
    SET_SKILL(ch, SPELL_GROUP_ARMOR, 99);
    SET_SKILL(ch, SPELL_CURE_CRITIC, 99);
    SET_SKILL(ch, SPELL_SUMMON, 99);
    SET_SKILL(ch, SPELL_REMOVE_POISON, 99);
    SET_SKILL(ch, SPELL_IDENTIFY, 99);
    SET_SKILL(ch, SPELL_WORD_OF_RECALL, 99);
    SET_SKILL(ch, SPELL_EARTHQUAKE, 99);
    SET_SKILL(ch, SPELL_DISPEL_EVIL, 99);
    SET_SKILL(ch, SPELL_DISPEL_GOOD, 99);
    SET_SKILL(ch, SPELL_SANCTUARY, 99);
    SET_SKILL(ch, SPELL_CALL_LIGHTNING, 99);
    SET_SKILL(ch, SPELL_HEAL, 99);
    SET_SKILL(ch, SPELL_CONTROL_WEATHER, 99);
    SET_SKILL(ch, SPELL_SENSE_LIFE, 99);
    SET_SKILL(ch, SPELL_HARM, 99);
    SET_SKILL(ch, SPELL_GROUP_HEAL, 99);
    SET_SKILL(ch, SPELL_REMOVE_CURSE, 99);
    SET_SKILL(ch, SPELL_CAUSE_LIGHT_WOUNDS, 99);
    SET_SKILL(ch, SPELL_CAUSE_MODERATE_WOUNDS, 99);
    SET_SKILL(ch, SPELL_CAUSE_SERIOUS_WOUNDS, 99);
    SET_SKILL(ch, SPELL_CAUSE_CRITICAL_WOUNDS, 99);
    SET_SKILL(ch, SPELL_FLAME_STRIKE, 99);
    SET_SKILL(ch, SPELL_DESTRUCTION, 99);
    send_to_char(ch, "Druid Done.\tn\r\n");
  break;


  case CLASS_THIEF:
    SET_SKILL(ch, SKILL_PROF_SIMPLE_W, 75);
    SET_SKILL(ch, SKILL_PROF_ELF_W, 75);
    SET_SKILL(ch, SKILL_PROF_DRUID_W, 75);
    SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);

    SET_SKILL(ch, SKILL_BACKSTAB, 75);
    SET_SKILL(ch, SKILL_TRACK, 75);
    send_to_char(ch, "Thief Done.\tn\r\n");
  break;


  case CLASS_WARRIOR:
    SET_SKILL(ch, SKILL_PROF_SIMPLE_W, 75);
    SET_SKILL(ch, SKILL_PROF_ELF_W, 75);
    SET_SKILL(ch, SKILL_PROF_DRUID_W, 75);
    SET_SKILL(ch, SKILL_PROF_MARTIAL_W, 75);
    SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);
    SET_SKILL(ch, SKILL_PROF_MEDIUM_A, 75);
    SET_SKILL(ch, SKILL_PROF_HEAVY_A, 75);
    SET_SKILL(ch, SKILL_PROF_SHIELDS, 75);
    SET_SKILL(ch, SKILL_PROF_T_SHIELDS, 75);
    send_to_char(ch, "Warrior Done.\tn\r\n");
  break;

  case CLASS_BERSERKER:
    SET_SKILL(ch, SKILL_PROF_SIMPLE_W, 75);
    SET_SKILL(ch, SKILL_PROF_ELF_W, 75);
    SET_SKILL(ch, SKILL_PROF_DRUID_W, 75);
    SET_SKILL(ch, SKILL_PROF_MARTIAL_W, 75);
    SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);
    SET_SKILL(ch, SKILL_PROF_MEDIUM_A, 75);
    SET_SKILL(ch, SKILL_PROF_SHIELDS, 75);
    send_to_char(ch, "Berserker Done.\tn\r\n");
  break;

  case CLASS_MONK:
    SET_SKILL(ch, SKILL_PROF_SIMPLE_W, 75);
    send_to_char(ch, "Monk Done.\tn\r\n");    
  break;

  default:
    send_to_char(ch, "None needed.\tn\r\n");
  break;
  }
}


/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  int trains = 0, practices = 0, i = 0;

  //init the character
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      perform_remove(ch, i, TRUE);
  if (ch->affected || AFF_FLAGS(ch)) {  
    while (ch->affected)
      affect_remove(ch, ch->affected);
    for(i=0; i < AF_ARRAY_MAX; i++)
      AFF_FLAGS(ch)[i] = 0;
  }
  for (i = 0; i < MAX_CLASSES; i++) {
    CLASS_LEVEL(ch, i) = 0;
    GET_SPEC_ABIL(ch, i) = 0;
  }
  for (i = 0; i < MAX_WARDING; i++)
    GET_WARDING(ch, i) = 0;
  GET_LEVEL(ch) = 1;
  CLASS_LEVEL(ch, GET_CLASS(ch)) = 1;
  GET_EXP(ch) = 1;
  set_title(ch, NULL);
  roll_real_abils(ch);
  GET_AC(ch) = 100;
  GET_HITROLL(ch) = 0;
  GET_DAMROLL(ch) = 0;
  GET_MAX_HIT(ch)  = 20;
  GET_MAX_MANA(ch) = 100;
  GET_MAX_MOVE(ch) = 82;
  GET_PRACTICES(ch) = 0;
  GET_TRAINS(ch) = 0;
  GET_BOOSTS(ch) = 4;
  GET_SPELL_RES(ch) = 0;
  for (i=1; i<=NUM_SKILLS; i++)
    SET_SKILL(ch, i, 0);
  for (i=1; i<=NUM_ABILITIES; i++)
    SET_ABILITY(ch, i, 0);
  init_spell_slots(ch);
  /* hunger and thirst are off */
  GET_COND(ch, HUNGER) = -1;
  GET_COND(ch, THIRST) = -1;

  //racial inits
  switch(GET_RACE(ch)) {
    case RACE_HUMAN:
      GET_SIZE(ch) = SIZE_MEDIUM;
      practices++;
      trains += 3;
      break;
    case RACE_ELF:
      GET_SIZE(ch) = SIZE_MEDIUM;
      SET_SKILL(ch, SKILL_PROF_ELF_W, 75);
      ch->real_abils.dex += 2;
      ch->real_abils.con -= 2;
      break;
    case RACE_DWARF:
      GET_SIZE(ch) = SIZE_MEDIUM;
      ch->real_abils.con += 2;
      ch->real_abils.cha -= 2;
      break;
    case RACE_HALFLING:
      GET_SIZE(ch) = SIZE_SMALL;
      ch->real_abils.dex += 2;
      ch->real_abils.str -= 2;
      break;
    case RACE_H_ELF:
      GET_SIZE(ch) = SIZE_MEDIUM;
      SET_SKILL(ch, SKILL_PROF_ELF_W, 75);
      break;
    case RACE_H_ORC:
      GET_SIZE(ch) = SIZE_MEDIUM;
      ch->real_abils.str += 2;
      ch->real_abils.cha -= 2;
      ch->real_abils.intel -= 2;
      break;
    case RACE_GNOME:
      GET_SIZE(ch) = SIZE_SMALL;
      ch->real_abils.con += 2;
      ch->real_abils.str -= 2;
      break;
    case RACE_TROLL:
      GET_SIZE(ch) = SIZE_LARGE;
      ch->real_abils.str += 2;
      ch->real_abils.con += 2;
      ch->real_abils.dex += 2;
      ch->real_abils.intel -= 4;
      ch->real_abils.wis -= 4;
      ch->real_abils.cha -= 4;
      break;
    case RACE_CRYSTAL_DWARF:
      GET_SIZE(ch) = SIZE_MEDIUM;
      ch->real_abils.str += 2;
      ch->real_abils.con += 8;
      ch->real_abils.wis += 2;
      ch->real_abils.cha += 2;
      GET_MAX_HIT(ch) += 10;
      break;    
    default:
      GET_SIZE(ch) = SIZE_MEDIUM;
      break;
  }

  //class-related inits
  switch (GET_CLASS(ch)) {
  case CLASS_MAGIC_USER:
    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))) * 3);
    break;
  case CLASS_CLERIC:
    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))) * 3);
    break;
  case CLASS_DRUID:
  case CLASS_BERSERKER:
  case CLASS_MONK:
    trains += MAX(1, (4 + (int)(GET_INT_BONUS(ch))) * 3);
    break;
  case CLASS_THIEF:
    trains += MAX(1, (8 + (int)(GET_INT_BONUS(ch))) * 3);
    break;
  case CLASS_WARRIOR:
    practices++;
    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))) * 3);
    break;
  }
  practices++;
  GET_PRACTICES(ch) += practices;
  send_to_char(ch, "%d \tMPractice sessions gained.\tn\r\n", practices);
  GET_TRAINS(ch) += trains;
  send_to_char(ch, "%d \tMTraining sessions gained.\tn\r\n", trains);

  //from level 0 -> level 1
  advance_level(ch, GET_CLASS(ch));
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  GET_COND(ch, DRUNK) = 0;
  if (CONFIG_SITEOK_ALL)
    SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);
}


void advance_level(struct char_data *ch, int class)
{
  int add_hp = GET_CON_BONUS(ch),
	add_mana = 0, add_move = 0, k, trains = 0, i, j, practices = 0;
  struct affected_type *af;

  //**because con items / spells are affecting based on level, we have to unaffect
  //**before we level up -zusuk
  /*********  unaffect ********/
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
                      GET_EQ(ch, i)->affected[j].modifier,
                      GET_OBJ_AFFECT(GET_EQ(ch, i)), FALSE);
  }     
  for (af = ch->affected; af; af = af->next)
    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);  
  ch->aff_abils = ch->real_abils;
  /******  end unaffect ******/


  send_to_char(ch, "\tMInititializing class:  ");
  init_class(ch, class, CLASS_LEVEL(ch, class));

  send_to_char(ch, "\tMGAINS:\tn\r\n");
  switch (class) {
  case CLASS_MAGIC_USER:
    add_hp += rand_number(2, 4);
    add_mana = 0;
    add_move = rand_number(0, 2);

    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))));

    if (!(CLASS_LEVEL(ch, class) % 5) && GET_LEVEL(ch) < 20)
      practices++;
    //epic
    if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20)
      practices++;

    break;
  case CLASS_CLERIC:
    add_hp += rand_number(4, 8);
    add_mana = 0;
    add_move = rand_number(0, 2);

    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))));

    //epic
    if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20)
      practices++;

    break;
  case CLASS_THIEF:
    thief_skills(ch, CLASS_LEVEL(ch, CLASS_THIEF));
    add_hp += rand_number(3, 6);
    add_mana = 0;
    add_move = rand_number(2, 4);

    trains += MAX(1, (8 + (int)(GET_INT_BONUS(ch))));

    //epic
    if (!(CLASS_LEVEL(ch, class) % 4) && GET_LEVEL(ch) >= 20)
      practices++;

    break;
  case CLASS_MONK:
    monk_skills(ch, CLASS_LEVEL(ch, CLASS_MONK));
    add_hp += rand_number(4, 8);
    add_mana = 0;
    add_move = rand_number(2, 4);

    trains += MAX(1, (4 + (int)(GET_INT_BONUS(ch))));

    //epic
    if (!(CLASS_LEVEL(ch, class) % 5) && GET_LEVEL(ch) >= 20)
      practices++;

    break;
  case CLASS_BERSERKER:
    berserker_skills(ch, CLASS_LEVEL(ch, CLASS_BERSERKER));
    add_hp += rand_number(6, 12);
    add_mana = 0;
    add_move = rand_number(4, 8);

    trains += MAX(1, (4 + (int)(GET_INT_BONUS(ch))));

    //epic
    if (!(CLASS_LEVEL(ch, class) % 4) && GET_LEVEL(ch) >= 20)
      practices++;

    break;
  case CLASS_DRUID:
    add_hp += rand_number(4, 8);
    add_mana = 0;
    add_move = rand_number(4, 8);

    trains += MAX(1, (4 + (int)(GET_INT_BONUS(ch))));

    //epic
    if (!(CLASS_LEVEL(ch, class) % 4) && GET_LEVEL(ch) >= 20)
      practices++;

    break;
  case CLASS_WARRIOR:
    add_hp += rand_number(5, 10);
    add_mana = 0;
    add_move = rand_number(1, 3);

    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))));

    if (!(CLASS_LEVEL(ch, class) % 2))
      practices++;

    break;
  }
  //Human Racial Bonus
  switch (GET_RACE(ch)) {
    case RACE_HUMAN:
      trains++;
      break;
    case RACE_CRYSTAL_DWARF:
      add_hp += 4;
      break;
    default:
      break;
  }
  //base practice improvement
  if (!(GET_LEVEL(ch) % 3))
    practices++;
  if (!(GET_LEVEL(ch) % 4)) {
    GET_BOOSTS(ch)++;
    send_to_char(ch, "\tMYou gain a boost (to stats) point!\tn\r\n");
  }

  //remember, GET_CON(ch) is adjusted con, ch->real_abils.con is natural con -zusuk
  if (GET_SKILL(ch, SKILL_TOUGHNESS))
    add_hp++;
  if (GET_SKILL(ch, SKILL_EPIC_TOUGHNESS))
    add_hp++;
  ch->points.max_hit += MAX(1, add_hp);
  send_to_char(ch, "\tMTotal HP:\tn %d\r\n", MAX(1, add_hp));
  ch->points.max_move += MAX(1, add_move);
  send_to_char(ch, "\tMTotal Move:\tn %d\r\n", MAX(1, add_move));
  if (GET_LEVEL(ch) > 1) {
    ch->points.max_mana += add_mana;
    send_to_char(ch, "\tMTotal Mana:\tn %d\r\n", add_mana);
  }
  GET_PRACTICES(ch) += practices;
  send_to_char(ch, "%d \tMPractice sessions gained.\tn\r\n", practices);
  GET_TRAINS(ch) += trains;
  send_to_char(ch, "%d \tMTraining sessions gained.\tn\r\n", trains);

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (k = 0; k < 3; k++)
      GET_COND(ch, k) = (char) -1;
    SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  /**** reaffect ****/
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
                      GET_EQ(ch, i)->affected[j].modifier,
                      GET_OBJ_AFFECT(GET_EQ(ch, i)), TRUE);
  }
   for (af = ch->affected; af; af = af->next)
    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);
  i = 50;  
  GET_DEX(ch) = MAX(0, MIN(GET_DEX(ch), i));
  GET_INT(ch) = MAX(0, MIN(GET_INT(ch), i));
  GET_WIS(ch) = MAX(0, MIN(GET_WIS(ch), i));
  GET_CON(ch) = MAX(0, MIN(GET_CON(ch), i));
  GET_CHA(ch) = MAX(0, MIN(GET_CHA(ch), i));
  GET_STR(ch) = MAX(0, MIN(GET_STR(ch), i));
  /*******  end reaffect  *****/

  snoop_check(ch);
  save_char(ch);
}


int backstab_mult(int level)
{
  if (level <= 7)
    return 2;
  else if (level <= 13)
    return 3;
  else if (level <= 20)
    return 4;
  else if (level <= 28)
    return 5;
  else
    return 6;
}


// used by handler.c
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
  if (OBJ_FLAGGED(obj, ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_MONK) && IS_MONK(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BERSERKER) && IS_BERSERKER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DRUID) && IS_DRUID(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch))
    return TRUE;

  return FALSE;
}


// vital min level info!
void init_spell_levels(void)
{
  int i=0, j=0;

  // simple loop to init all the SKILL_x to all classes
  for (i=MAX_SPELLS+1; i<NUM_SKILLS; i++) {
    for (j=0; j<NUM_CLASSES; j++) {
      spell_level(i, j, 1);
    }
  }


  // magic user, increment spells by spell-level
  //1st circle
  spell_level(SPELL_MAGIC_MISSILE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_ICE_DAGGER, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CHILL_TOUCH, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_SCARE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_SHIELD, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_SUMMON_CREATURE_1, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_TRUE_STRIKE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_SHELGARNS_BLADE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_HORIZIKAULS_BOOM, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_ENDURE_ELEMENTS, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_BURNING_HANDS, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CHARM, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_COLOR_SPRAY, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_IDENTIFY, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_MAGE_ARMOR, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_SLEEP, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_EXPEDITIOUS_RETREAT, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_GREASE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_IRON_GUTS, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_RAY_OF_ENFEEBLEMENT, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_NEGATIVE_ENERGY_RAY, CLASS_MAGIC_USER, 1);
  //2nd circle
  spell_level(SPELL_SUMMON_CREATURE_2, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_WALL_OF_FOG, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_DARKNESS, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_BLUR, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_WEB, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_ENDURANCE, CLASS_MAGIC_USER, 3);  //shared
  spell_level(SPELL_DETECT_INVIS, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_DETECT_MAGIC, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_FLY, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_BLINDNESS, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_DEAFNESS, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_RESIST_ENERGY, CLASS_MAGIC_USER, 1);
  //3rd circle
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_MAGIC_USER, 5);
  spell_level(SPELL_FIREBALL, CLASS_MAGIC_USER, 5);
  spell_level(SPELL_INFRAVISION, CLASS_MAGIC_USER, 5);
  spell_level(SPELL_MIRROR_IMAGE, CLASS_MAGIC_USER, 5);
  //4th circle
  spell_level(SPELL_STONESKIN, CLASS_MAGIC_USER, 7);
  //5th circle
  spell_level(SPELL_LOCATE_OBJECT, CLASS_MAGIC_USER, 9);
  spell_level(SPELL_INVISIBLE, CLASS_MAGIC_USER, 9);
  spell_level(SPELL_ICE_STORM, CLASS_MAGIC_USER, 9);
  //6th circle
  spell_level(SPELL_STRENGTH, CLASS_MAGIC_USER, 11);
  spell_level(SPELL_CLONE, CLASS_MAGIC_USER, 11);
  spell_level(SPELL_BALL_OF_LIGHTNING, CLASS_MAGIC_USER, 11);
  //7th circle
  spell_level(SPELL_DETECT_POISON, CLASS_MAGIC_USER, 13);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_MAGIC_USER, 13);
  spell_level(SPELL_TELEPORT, CLASS_MAGIC_USER, 13);
  spell_level(SPELL_MISSILE_STORM, CLASS_MAGIC_USER, 13);
  //8th circle
  spell_level(SPELL_ENERGY_DRAIN, CLASS_MAGIC_USER, 15);
  spell_level(SPELL_CURSE, CLASS_MAGIC_USER, 15);
  spell_level(SPELL_WATERWALK, CLASS_MAGIC_USER, 15);
  spell_level(SPELL_CHAIN_LIGHTNING, CLASS_MAGIC_USER, 15);
  //9th circle
  spell_level(SPELL_POISON, CLASS_MAGIC_USER, 17);
  spell_level(SPELL_METEOR_SWARM, CLASS_MAGIC_USER, 17);
  spell_level(SPELL_POLYMORPH, CLASS_MAGIC_USER, 17);
  //epic mage
  spell_level(SPELL_MUMMY_DUST, CLASS_MAGIC_USER, 20);
  spell_level(SPELL_DRAGON_KNIGHT, CLASS_MAGIC_USER, 20);
  spell_level(SPELL_GREATER_RUIN, CLASS_MAGIC_USER, 20);
  spell_level(SPELL_HELLBALL, CLASS_MAGIC_USER, 20);
  spell_level(SPELL_EPIC_MAGE_ARMOR, CLASS_MAGIC_USER, 20);
  spell_level(SPELL_EPIC_WARDING, CLASS_MAGIC_USER, 20);
  //end magic-user spells

  // clerics
  //1st circle
  spell_level(SPELL_CURE_LIGHT, CLASS_CLERIC, 1);
  spell_level(SPELL_ENDURANCE, CLASS_CLERIC, 1);
  spell_level(SPELL_ARMOR, CLASS_CLERIC, 1);
  spell_level(SPELL_CAUSE_LIGHT_WOUNDS, CLASS_CLERIC, 1);
  //2nd circle
  spell_level(SPELL_CREATE_FOOD, CLASS_CLERIC, 3);
  spell_level(SPELL_CREATE_WATER, CLASS_CLERIC, 3);
  spell_level(SPELL_DETECT_POISON, CLASS_CLERIC, 3);
  spell_level(SPELL_CAUSE_MODERATE_WOUNDS, CLASS_CLERIC, 3);
  //3rd circle
  spell_level(SPELL_DETECT_ALIGN, CLASS_CLERIC, 5);
  spell_level(SPELL_CURE_BLIND, CLASS_CLERIC, 5);
  spell_level(SPELL_BLESS, CLASS_CLERIC, 5);
  spell_level(SPELL_CAUSE_SERIOUS_WOUNDS, CLASS_CLERIC, 5);
  //4th circle
  spell_level(SPELL_INFRAVISION, CLASS_CLERIC, 7);
  spell_level(SPELL_REMOVE_CURSE, CLASS_CLERIC, 7);
  spell_level(SPELL_CAUSE_CRITICAL_WOUNDS, CLASS_CLERIC, 7);
  spell_level(SPELL_CURE_CRITIC, CLASS_CLERIC, 7);
  //5th circle
  spell_level(SPELL_BLINDNESS, CLASS_CLERIC, 9);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 9);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_CLERIC, 9);
  spell_level(SPELL_POISON, CLASS_CLERIC, 9);
  spell_level(SPELL_GROUP_ARMOR, CLASS_CLERIC, 9);
  spell_level(SPELL_FLAME_STRIKE, CLASS_CLERIC, 9);
  //6th circle
  spell_level(SPELL_DISPEL_EVIL, CLASS_CLERIC, 11);
  spell_level(SPELL_DISPEL_GOOD, CLASS_CLERIC, 11);
  spell_level(SPELL_REMOVE_POISON, CLASS_CLERIC, 11);
  spell_level(SPELL_HARM, CLASS_CLERIC, 11);
  spell_level(SPELL_HEAL, CLASS_CLERIC, 11);
  //7th circle
  spell_level(SPELL_CONTROL_WEATHER, CLASS_CLERIC, 13);
  spell_level(SPELL_SUMMON, CLASS_CLERIC, 13);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_CLERIC, 13);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_CLERIC, 13);
  //8th circle
  spell_level(SPELL_SENSE_LIFE, CLASS_CLERIC, 15);
  spell_level(SPELL_SANCTUARY, CLASS_CLERIC, 15);
  spell_level(SPELL_DESTRUCTION, CLASS_CLERIC, 15);
  //9th circle
  spell_level(SPELL_EARTHQUAKE, CLASS_CLERIC, 17);
  spell_level(SPELL_GROUP_HEAL, CLASS_CLERIC, 17);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_CLERIC, 17);
  //epic spells
  spell_level(SPELL_MUMMY_DUST, CLASS_CLERIC, 20);
  spell_level(SPELL_DRAGON_KNIGHT, CLASS_CLERIC, 20);
  spell_level(SPELL_GREATER_RUIN, CLASS_CLERIC, 20);
  spell_level(SPELL_HELLBALL, CLASS_CLERIC, 20);
  //end cleric spells

  // druid
  //1st circle
  spell_level(SPELL_MAGIC_MISSILE, CLASS_DRUID, 1);
  spell_level(SPELL_CURE_LIGHT, CLASS_DRUID, 1);
  spell_level(SPELL_ENDURANCE, CLASS_DRUID, 1);
  spell_level(SPELL_ARMOR, CLASS_DRUID, 1);
  spell_level(SPELL_CAUSE_LIGHT_WOUNDS, CLASS_DRUID, 1);
  //2nd circle
  spell_level(SPELL_CREATE_FOOD, CLASS_DRUID, 3);
  spell_level(SPELL_CREATE_WATER, CLASS_DRUID, 3);
  spell_level(SPELL_DETECT_POISON, CLASS_DRUID, 3);
  spell_level(SPELL_CAUSE_MODERATE_WOUNDS, CLASS_DRUID, 3);
  //3rd circle
  spell_level(SPELL_DETECT_ALIGN, CLASS_DRUID, 5);
  spell_level(SPELL_CURE_BLIND, CLASS_DRUID, 5);
  spell_level(SPELL_BLESS, CLASS_DRUID, 5);
  spell_level(SPELL_CAUSE_SERIOUS_WOUNDS, CLASS_DRUID, 5);
  //4th circle
  spell_level(SPELL_INFRAVISION, CLASS_DRUID, 7);
  spell_level(SPELL_REMOVE_CURSE, CLASS_DRUID, 7);
  spell_level(SPELL_CAUSE_CRITICAL_WOUNDS, CLASS_DRUID, 7);
  spell_level(SPELL_CURE_CRITIC, CLASS_DRUID, 7);
  //5th circle
  spell_level(SPELL_BLINDNESS, CLASS_DRUID, 9);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_DRUID, 9);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_DRUID, 9);
  spell_level(SPELL_POISON, CLASS_DRUID, 9);
  spell_level(SPELL_GROUP_ARMOR, CLASS_DRUID, 9);
  spell_level(SPELL_FLAME_STRIKE, CLASS_DRUID, 9);
  //6th circle
  spell_level(SPELL_DISPEL_EVIL, CLASS_DRUID, 11);
  spell_level(SPELL_DISPEL_GOOD, CLASS_DRUID, 11);
  spell_level(SPELL_REMOVE_POISON, CLASS_DRUID, 11);
  spell_level(SPELL_HARM, CLASS_DRUID, 11);
  spell_level(SPELL_HEAL, CLASS_DRUID, 11);
  //7th circle
  spell_level(SPELL_CONTROL_WEATHER, CLASS_DRUID, 13);
  spell_level(SPELL_SUMMON, CLASS_DRUID, 13);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_DRUID, 13);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_DRUID, 13);
  //8th circle
  spell_level(SPELL_SENSE_LIFE, CLASS_DRUID, 15);
  spell_level(SPELL_SANCTUARY, CLASS_DRUID, 15);
  spell_level(SPELL_DESTRUCTION, CLASS_DRUID, 15);
  //9th circle
  spell_level(SPELL_EARTHQUAKE, CLASS_DRUID, 17);
  spell_level(SPELL_GROUP_HEAL, CLASS_DRUID, 17);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_DRUID, 17);
  //epic spells
  spell_level(SPELL_MUMMY_DUST, CLASS_DRUID, 20);
  spell_level(SPELL_DRAGON_KNIGHT, CLASS_DRUID, 20);
  spell_level(SPELL_GREATER_RUIN, CLASS_DRUID, 20);
  spell_level(SPELL_HELLBALL, CLASS_DRUID, 20);
  //end druid spells

}


// level_exp ran with level+1 will give xp to next level
// level_exp+1 - level_exp = exp to next level
#define EXP_MAX  2100000000
int level_exp(struct char_data *ch, int level)
{
  int chclass = GET_CLASS(ch);
  int exp = 0, factor = 0;

  if (level > LVL_IMPL || level < 0) {
    log("SYSERR: Requesting exp for invalid level %d!", level);
    return 0;
  }

  /* Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist. */
   if (level > LVL_IMMORT) {
     return EXP_MAX - ((LVL_IMPL - level) * 1000);
   }

  factor = 2000 + (level-2) * 750;

  /* Exp required for normal mortals is below */
  switch (chclass) {
    case CLASS_MAGIC_USER:
    case CLASS_MONK:
    case CLASS_DRUID:
    case CLASS_WARRIOR:
    case CLASS_THIEF:
    case CLASS_BERSERKER:
    case CLASS_CLERIC:
      level--;
      if (level < 0)
        level = 0;
      exp += (level * level * factor);
    break;

    default:
      log("SYSERR: Reached invalid class in class.c level()!");
      return 123456;      
  }
  
  //can add other exp penalty/bonuses here
  switch (GET_RACE(ch)) {
    case RACE_TROLL:
      exp *= 2;
      break;
    case RACE_CRYSTAL_DWARF:
      exp *= 90;
      break;
    default:
      break;
  }


  return exp;
}


/* Default titles of male characters. */
const char *title_male(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Man";
  if (level == LVL_IMPL)
    return "the Implementor";

  switch (chclass) {

    case CLASS_MAGIC_USER:
    switch (level) {
      case  1: return "the Apprentice of Magic";
      case  2: return "the Spell Student";
      case  3: return "the Scholar of Magic";
      case  4: return "the Delver in Spells";
      case  5: return "the Medium of Magic";
      case  6: return "the Scribe of Magic";
      case  7: return "the Seer";
      case  8: return "the Sage";
      case  9: return "the Illusionist";
      case 10: return "the Abjurer";
      case 11: return "the Invoker";
      case 12: return "the Enchanter";
      case 13: return "the Conjurer";
      case 14: return "the Magician";
      case 15: return "the Creator";
      case 16: return "the Savant";
      case 17: return "the Magus";
      case 18: return "the Wizard";
      case 19: return "the Warlock";
      case 20: return "the Sorcerer";
      case 21: return "the Necromancer";
      case 22: return "the Thaumaturge";
      case 23: return "the Student of the Occult";
      case 24: return "the Disciple of the Uncanny";
      case 25: return "the Minor Elemental";
      case 26: return "the Greater Elemental";
      case 27: return "the Crafter of Magics";
      case 28: return "the Shaman";
      case 29: return "the Keeper of Talismans";
      case 30: return "the Archmage";
      case LVL_IMMORT: return "the Immortal Warlock";
      case LVL_GOD: return "the Avatar of Magic";
      case LVL_GRGOD: return "the God of Magic";
      default: return "the Mage";
    }
    break;

    case CLASS_CLERIC:
    switch (level) {
      case  1: return "the Believer";
      case  2: return "the Attendant";
      case  3: return "the Acolyte";
      case  4: return "the Novice";
      case  5: return "the Missionary";
      case  6: return "the Adept";
      case  7: return "the Deacon";
      case  8: return "the Vicar";
      case  9: return "the Priest";
      case 10: return "the Minister";
      case 11: return "the Canon";
      case 12: return "the Levite";
      case 13: return "the Curate";
      case 14: return "the Monk";
      case 15: return "the Healer";
      case 16: return "the Chaplain";
      case 17: return "the Expositor";
      case 18: return "the Bishop";
      case 19: return "the High Bishop";
      case 20: return "the Patriarch";
      case 21: return "the Chancellor";
      case 22: return "the Arch Bishop";
      case 23: return "the Arch Priest";
      case 24: return "the Cardinal";
      case 25: return "the Sage";
      case 26: return "the Saint";
      case 27: return "the Apostle";
      case 28: return "the Father";
      case 29: return "the Elder";
      case 30: return "the Venerable Father";
      case LVL_IMMORT: return "the Immortal Cardinal";
      case LVL_GOD: return "the Inquisitor";
      case LVL_GRGOD: return "the God of Good and Evil";
      default: return "the Cleric";
    }
    break;

    case CLASS_MONK:
    switch (level) {
      case 1: return "the Initiate";
      case 2: return "the Novice";
      case 3: return "the Acolyte";
      case 4: return "the Sexton";
      case 5: return "the Beadle";
      case 6: return "the Scribe";
      case 7: return "the Monk";
      case 8: return "the Pilgrim";
      case 9: return "the Friar";
      case 10: return "the Hermit";
      case 11: return "the Chaplain";
      case 12: return "the Deacon";
      case 13: return "the Curate";
      case 14: return "the Priest";
      case 15: return "the Vicar";
      case 16: return "the Parson";
      case 17: return "the Prior";
      case 18: return "the Monsignor";
      case 19: return "the Abbot";
      case 20: return "the Canon";
      case 21: return "the Chancellor";
      case 22: return "the Bishop";
      case 23: return "the Archbishop";
      case 24: return "the Cardinal";
      case 25: return "the Sage";
      case 26: return "the Saint";
      case 27: return "the Apostle";
      case 28: return "the Father";
      case 29: return "the Elder";
      case 30: return "the Venerable Father";
      case LVL_IMMORT: return "the Immortal Monk";
      case LVL_GOD: return "the Inquisitor Monk";
      case LVL_GRGOD: return "the God of the Fist";
      default: return "the Monk";
    }
    break;

    case CLASS_THIEF:
    switch (level) {
      case  1: return "the Pilferer";
      case  2: return "the Footpad";
      case  3: return "the Filcher";
      case  4: return "the Pick-Pocket";
      case  5: return "the Sneak";
      case  6: return "the Pincher";
      case  7: return "the Cut-Purse";
      case  8: return "the Snatcher";
      case  9: return "the Sharper";
      case 10: return "the Rogue";
      case 11: return "the Robber";
      case 12: return "the Magsman";
      case 13: return "the Highwayman";
      case 14: return "the Burglar";
      case 15: return "the Thief";
      case 16: return "the Knifer";
      case 17: return "the Quick-Blade";
      case 18: return "the Killer";
      case 19: return "the Brigand";
      case 20: return "the Cut-Throat";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Assassin";
      case LVL_GOD: return "the Demi God of Thieves";
      case LVL_GRGOD: return "the God of Thieves and Tradesmen";
      default: return "the Thief";
    }
    break;

    case CLASS_WARRIOR:
    switch(level) {
      case  1: return "the Swordpupil";
      case  2: return "the Recruit";
      case  3: return "the Sentry";
      case  4: return "the Fighter";
      case  5: return "the Soldier";
      case  6: return "the Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordsman";
      case  9: return "the Fencer";
      case 10: return "the Combatant";
      case 11: return "the Hero";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckler";
      case 14: return "the Mercenary";
      case 15: return "the Swordmaster";
      case 16: return "the Lieutenant";
      case 17: return "the Champion";
      case 18: return "the Dragoon";
      case 19: return "the Cavalier";
      case 20: return "the Knight";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Warlord";
      case LVL_GOD: return "the Extirpator";
      case LVL_GRGOD: return "the God of War";
      default: return "the Warrior";
    }
    break;
  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}

/* Default titles of female characters. */
const char *title_female(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Woman";
  if (level == LVL_IMPL)
    return "the Implementress";

  switch (chclass) {

    case CLASS_MAGIC_USER:
    switch (level) {
      case  1: return "the Apprentice of Magic";
      case  2: return "the Spell Student";
      case  3: return "the Scholar of Magic";
      case  4: return "the Delveress in Spells";
      case  5: return "the Medium of Magic";
      case  6: return "the Scribess of Magic";
      case  7: return "the Seeress";
      case  8: return "the Sage";
      case  9: return "the Illusionist";
      case 10: return "the Abjuress";
      case 11: return "the Invoker";
      case 12: return "the Enchantress";
      case 13: return "the Conjuress";
      case 14: return "the Witch";
      case 15: return "the Creator";
      case 16: return "the Savant";
      case 17: return "the Craftess";
      case 18: return "the Wizard";
      case 19: return "the War Witch";
      case 20: return "the Sorceress";
      case 21: return "the Necromancress";
      case 22: return "the Thaumaturgess";
      case 23: return "the Student of the Occult";
      case 24: return "the Disciple of the Uncanny";
      case 25: return "the Minor Elementress";
      case 26: return "the Greater Elementress";
      case 27: return "the Crafter of Magics";
      case 28: return "Shaman";
      case 29: return "the Keeper of Talismans";
      case 30: return "Archwitch";
      case LVL_IMMORT: return "the Immortal Enchantress";
      case LVL_GOD: return "the Empress of Magic";
      case LVL_GRGOD: return "the Goddess of Magic";
      default: return "the Witch";
    }
    break;

    case CLASS_CLERIC:
    switch (level) {
      case  1: return "the Believer";
      case  2: return "the Attendant";
      case  3: return "the Acolyte";
      case  4: return "the Novice";
      case  5: return "the Missionary";
      case  6: return "the Adept";
      case  7: return "the Deaconess";
      case  8: return "the Vicaress";
      case  9: return "the Priestess";
      case 10: return "the Lady Minister";
      case 11: return "the Canon";
      case 12: return "the Levitess";
      case 13: return "the Curess";
      case 14: return "the Nunne";
      case 15: return "the Healess";
      case 16: return "the Chaplain";
      case 17: return "the Expositress";
      case 18: return "the Bishop";
      case 19: return "the Arch Lady of the Church";
      case 20: return "the Matriarch";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Priestess";
      case LVL_GOD: return "the Inquisitress";
      case LVL_GRGOD: return "the Goddess of Good and Evil";
      default: return "the Cleric";
    }
    break;

    case CLASS_MONK:
    switch (level) {
      case 1: return "the Initiate";
      case 2: return "the Novice";
      case 3: return "the Acolyte";
      case 4: return "the Sexton";
      case 5: return "the Beadle";
      case 6: return "the Scribe";
      case 7: return "the Monk";
      case 8: return "the Pilgrim";
      case 9: return "the Friar";
      case 10: return "the Hermit";
      case 11: return "the Chaplain";
      case 12: return "the Deacon";
      case 13: return "the Curate";
      case 14: return "the Priest";
      case 15: return "the Vicar";
      case 16: return "the Parson";
      case 17: return "the Prior";
      case 18: return "the Monsignor";
      case 19: return "the Imam";
      case 20: return "the Canon";
      case 21: return "the Chancellor";
      case 22: return "the Bishop";
      case 23: return "the Archbishop";
      case 24: return "the Cardinal";
      case 25: return "the Sage";
      case 26: return "the Saint";
      case 27: return "the Apostle";
      case 28: return "the Mother";
      case 29: return "the Elder";
      case 30: return "the Venerable Mother";
      case LVL_IMMORT: return "the Immortal Monk";
      case LVL_GOD: return "the Inquisitor Monk";
      case LVL_GRGOD: return "the God of the Fist";
      default: return "the Monk";
    }
    break;

    case CLASS_THIEF:
    switch (level) {
      case  1: return "the Pilferess";
      case  2: return "the Footpad";
      case  3: return "the Filcheress";
      case  4: return "the Pick-Pocket";
      case  5: return "the Sneak";
      case  6: return "the Pincheress";
      case  7: return "the Cut-Purse";
      case  8: return "the Snatcheress";
      case  9: return "the Sharpress";
      case 10: return "the Rogue";
      case 11: return "the Robber";
      case 12: return "the Magswoman";
      case 13: return "the Highwaywoman";
      case 14: return "the Burglaress";
      case 15: return "the Thief";
      case 16: return "the Knifer";
      case 17: return "the Quick-Blade";
      case 18: return "the Murderess";
      case 19: return "the Brigand";
      case 20: return "the Cut-Throat";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Assassin";
      case LVL_GOD: return "the Demi Goddess of Thieves";
      case LVL_GRGOD: return "the Goddess of Thieves and Tradesmen";
      default: return "the Thief";
    }
    break;

    case CLASS_WARRIOR:
    switch(level) {
      case  1: return "the Swordpupil";
      case  2: return "the Recruit";
      case  3: return "the Sentress";
      case  4: return "the Fighter";
      case  5: return "the Soldier";
      case  6: return "the Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordswoman";
      case  9: return "the Fenceress";
      case 10: return "the Combatess";
      case 11: return "the Heroine";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckleress";
      case 14: return "the Mercenaress";
      case 15: return "the Swordmistress";
      case 16: return "the Lieutenant";
      case 17: return "the Lady Champion";
      case 18: return "the Lady Dragoon";
      case 19: return "the Cavalier";
      case 20: return "the Lady Knight";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Lady of War";
      case LVL_GOD: return "the Queen of Destruction";
      case LVL_GRGOD: return "the Goddess of War";
      default: return "the Warrior";
    }
    break;
  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}

