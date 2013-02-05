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
  "\tmWiz\tn",
  "\tBCle\tn",
  "\twRog\tn",
  "\tRWar\tn",
  "\tgMon\tn",
  "\tGD\tgr\tGu\tn",
  "\trB\tRe\trs\tn",
  "\tMSor\tn",
  "\tWPal\tn",
  "\tYRan\tn",
  "\n"
};

const char *pc_class_types[] = {
  "Wizard",
  "Cleric",
  "Rogue",
  "Warrior",
  "Monk",
  "Druid",
  "Berserker",
  "Sorcerer",
  "Paladin",
  "Ranger",
  "\n"
};

/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n"
"  \tbRea\tclms \tWof Lu\tcmin\tbari\tn | class selection\r\n"
"---------------------+\r\n"
"  c)  \tBCleric\tn\r\n"
"  t)  \tWRogue\tn\r\n"
"  w)  \tRWarrior\tn\r\n"
"  o)  \tgMonk\tn\r\n"
"  d)  \tGD\tgr\tGu\tgi\tGd\tn\r\n"
"  m)  \tmWizard\tn\r\n"
"  b)  \trBer\tRser\trker\tn\r\n"
"  p)  \tWPaladin\tn\r\n"
"  s)  \tMSorcerer\tn\r\n"
"  r)  \tYRanger\tn\r\n";


/* The code to interpret a class letter -- used in interpreter.c when a new
 * character is selecting a class and by 'set class' in act.wizard.c. */
int parse_class(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'm': return CLASS_WIZARD;
  case 'c': return CLASS_CLERIC;
  case 'w': return CLASS_WARRIOR;
  case 't': return CLASS_ROGUE;
  case 'o': return CLASS_MONK;
  case 'd': return CLASS_DRUID;
  case 'b': return CLASS_BERSERKER;
  case 's': return CLASS_SORCERER;
  case 'p': return CLASS_PALADIN;
  case 'r': return CLASS_RANGER;
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
 /* MG  CL  TH	 WR  MN  DR  BK  SR  PL  RA*/
  { 75, 75, 75, 75, 75, 75, 75, 75, 75, 75 },	/* learned level */
  { 75, 75, 75, 75, 75, 75, 75, 75, 75, 75 },	/* max per practice */
  { 75, 75, 75, 75, 75, 75, 75, 75, 75, 75 },	/* min per practice */
  { SK, SK, SK, SK, SK, SK, SK, SK, SK, SK },	/* prac name */
};
#undef SP
#undef SK
/* The appropriate rooms for each guildmaster/guildguard; controls which types
 * of people the various guildguards let through.  i.e., the first line shows
 * that from room 3017, only WIZARDS are allowed to go south. Don't forget
 * to visit spec_assign.c if you create any new mobiles that should be a guild
 * master or guard so they can act appropriately. If you "recycle" the
 * existing mobs that are used in other guilds for your new guild, then you
 * don't have to change that file, only here. Guildguards are now implemented
 * via triggers. This code remains as an example. */
struct guild_info_type guild_info[] = {

  /* Midgaard */
  { CLASS_WIZARD,      3017,	SOUTH },
  { CLASS_CLERIC,	   3004,	NORTH },
  { CLASS_DRUID,	   3004,	NORTH },
  { CLASS_MONK,	   3004,	NORTH },
  { CLASS_ROGUE,	   3027,	EAST  },
  { CLASS_WARRIOR,	   3021,	EAST  },
  { CLASS_RANGER,	   3021,	EAST  },
  { CLASS_PALADIN,	   3021,	EAST  },
  { CLASS_BERSERKER,   3021,	EAST  },
  { CLASS_SORCERER,    3017,	SOUTH },

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
//  MU  CL  TH  WA  MO  DR  BZ  SR  PL  RA
  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, //0 - reserved

  { CC, CC, CA, CC, CA, CC, CC, CC, CC, CC },	//1 - Tumble 
  { CC, CC, CA, CC, CA, CC, CC, CC, CC, CA },	//2 - hide
  { CC, CC, CA, CC, CA, CC, CC, CC, CC, CA },	//3 sneak
  { CC, CC, CA, CC, CA, CC, CC, CC, CC, CA },	//4 spot
  { CC, CC, CA, CC, CA, CC, CA, CC, CC, CA },	//5 listen
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA },	//6 treat injury
  { CC, CC, CC, CC, CC, CC, CA, CC, CA, CC },	//7 taunt
  { CA, CA, CC, CA, CA, CA, CC, CA, CA, CA },	//8 concentration
  { CA, CA, CC, CC, CC, CA, CC, CA, CC, CC },	//9 spellcraft
  { CC, CC, CA, CC, CC, CC, CC, CC, CC, CC },	//10 appraise
  { CC, CC, CC, CA, CC, CC, CA, CC, CA, CA },	//11 discipline
  { CC, CA, CA, CA, CA, CA, CA, CC, CA, CA },	//12 parry
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA },	//13 lore
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA },	//14 mount
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA },	//15 riding
  { CA, CA, CA, CA, CA, CA, CA, CA, CA, CA },	//16 tame
  { NA, NA, CA, NA, NA, NA, NA, NA, NA, NA },	//17 pick locks
  { NA, NA, CA, NA, NA, NA, NA, NA, NA, NA },	//18 steal
};
#undef NA
#undef CC
#undef CA


// Saving Throw System
#define		H	1	//high
#define		L	0	//low
int preferred_save[5][NUM_CLASSES] = {
//           MU CL TH WA MO DR BK SR PL RA
/*fort */  { L, H, L, H, H, H, H, L, H, H },
/*refl */  { L, L, H, L, H, L, L, L, L, L },
/*will */  { H, H, L, L, H, H, L, H, L, L },
/*psn  */  { L, L, L, L, L, L, L, L, L, L },
/*death*/  { L, L, L, L, L, L, L, L, L, L },
};
// fortitude / reflex / will / ( poison / death )
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

  /* gnarly huh? */
  if (IS_AFFECTED(ch, AFF_TFORM))
    return (GET_LEVEL(ch));

  if (IS_NPC(ch))  //npc's default to medium attack rolls
    return ( (int) (GET_LEVEL(ch) * 3 / 4) );
  
  for (i = 0; i < MAX_CLASSES; i++) {
    level = CLASS_LEVEL(ch, i);
    if (level) {
      switch (i) {
        case CLASS_WIZARD:
        case CLASS_SORCERER:
          bab += level / 2;
          break;
        case CLASS_ROGUE:
        case CLASS_CLERIC:
        case CLASS_DRUID:
        case CLASS_MONK:
          bab += level * 3 / 4;
          break;
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
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


// old random roll system abandoned for base stats + point distribution
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
  int objNums[] = { 82, 858, 858, 804, 804, 804, 804, 803, 857, 3118, -1 };
  int x;
  struct obj_data *obj = NULL;
 
  send_to_char(ch, "\tMYou are given a set of starting equipment...\tn\r\n");

  // give everyone torch, rations, skin, backpack
  for (x = 0; objNums[x] != -1; x++) {
    obj = read_object(objNums[x], VIRTUAL);
    GET_OBJ_SIZE(obj) = GET_SIZE(ch);
    obj_to_char(obj, ch);
  }
  
  switch (GET_CLASS(ch))
  {
    case CLASS_PALADIN:
    case CLASS_CLERIC:
      // holy symbol

      obj = read_object(854, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // leather sleeves

      obj = read_object(855, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // leather leggings

      obj = read_object(861, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // slender iron mace

      obj = read_object(863, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // shield

      obj = read_object(807, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // scale mail

      break;

    case CLASS_BERSERKER:
    case CLASS_WARRIOR:
    case CLASS_RANGER:

      obj = read_object(854, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // leather sleeves

      obj = read_object(855, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // leather pants

      if (GET_RACE(ch) == RACE_DWARF) {
        obj = read_object(806, VIRTUAL);
        GET_OBJ_SIZE(obj) = GET_SIZE(ch);
        obj_to_char(obj, ch);       // dwarven waraxe
      } else {
        obj = read_object(808, VIRTUAL);
        GET_OBJ_SIZE(obj) = GET_SIZE(ch);
        obj_to_char(obj, ch);       // bastard sword
      }
      obj = read_object(863, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // shield

      obj = read_object(807, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // scale mail

      break;

    case CLASS_MONK:
      obj = read_object(809, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // cloth robes

      break;

    case CLASS_ROGUE:
      obj = read_object(854, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // leather sleeves

      obj = read_object(855, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // leather pants

      obj = read_object(851, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // studded leather

      obj = read_object(852, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // dagger

      obj = read_object(852, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // dagger

      break;

    case CLASS_SORCERER:
    case CLASS_WIZARD:
      obj = read_object(854, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // leather sleeves

      obj = read_object(855, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // leather pants

      obj = read_object(852, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // dagger

      obj = read_object(809, VIRTUAL);
      GET_OBJ_SIZE(obj) = GET_SIZE(ch);
      obj_to_char(obj, ch);       // cloth robes

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
      if (!GET_SKILL(ch, SKILL_RAGE))
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
void ranger_skills(struct char_data *ch, int level) {
  IS_RANG_LEARNED(ch) = 0;
  send_to_char(ch, "\tnType \tDstudy ranger\tn to adjust your skills.\r\n");
  
  switch (level) {
    case 2:
      if (!GET_SKILL(ch, SKILL_DUAL_WEAPONS))
        SET_SKILL(ch, SKILL_DUAL_WEAPONS, 75);
      send_to_char(ch, "\tMYou have learned 'Dual Weapons'\tn\r\n");
      break;
    case 3:
      if (!GET_SKILL(ch, SKILL_NATURE_STEP))
        SET_SKILL(ch, SKILL_NATURE_STEP, 75);
      send_to_char(ch, "\tMYou have learned 'Natures Step'\tn\r\n");
      break;
    case 4:
      if (!GET_SKILL(ch, SKILL_ANIMAL_COMPANION))
        SET_SKILL(ch, SKILL_ANIMAL_COMPANION, 75);
      send_to_char(ch, "\tMYou have learned 'Animal Companion'\tn\r\n");
      break;
    case 5:
      if (!GET_SKILL(ch, SKILL_TRACK))
        SET_SKILL(ch, SKILL_TRACK, 75);
      send_to_char(ch, "\tMYou have learned 'Track'\tn\r\n");
      break;
    default:
      break;
  }
  return;  
}

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 */
#define MOB_PALADIN_MOUNT 70
void paladin_skills(struct char_data *ch, int level) {
  switch (level) {
    case 2:
      if (!GET_SKILL(ch, SKILL_GRACE))
        SET_SKILL(ch, SKILL_GRACE, 75);
      send_to_char(ch, "\tMYou have learned 'Divine Grace'\tn\r\n");
      break;
    case 3:
      if (!GET_SKILL(ch, SKILL_DIVINE_HEALTH))
        SET_SKILL(ch, SKILL_DIVINE_HEALTH, 75);
      send_to_char(ch, "\tMYou have learned 'Divine Health'\tn\r\n");
      break;
    case 4:
      if (!GET_SKILL(ch, SKILL_COURAGE))
        SET_SKILL(ch, SKILL_COURAGE, 75);
      send_to_char(ch, "\tMYou have learned 'Paladin's Courage'\tn\r\n");
      break;
    case 5:
      if (!GET_SKILL(ch, SKILL_SMITE))
        SET_SKILL(ch, SKILL_SMITE, 75);
      send_to_char(ch, "\tMYou have learned 'Smite Evil'\tn\r\n");
      break;
    case 6:
      if (!GET_SKILL(ch, SKILL_USE_MAGIC))
        SET_SKILL(ch, SKILL_USE_MAGIC, 75);
      send_to_char(ch, "\tMYou have learned 'Use Magic'\tn\r\n");
      break;    
    case 7:
      if (!GET_SKILL(ch, SKILL_REMOVE_DISEASE))
        SET_SKILL(ch, SKILL_REMOVE_DISEASE, 75);
      send_to_char(ch, "\tMYou have learned 'Purify'\tn\r\n");
      break;
    case 8:
      if (!GET_SKILL(ch, SKILL_PALADIN_MOUNT))
        SET_SKILL(ch, SKILL_PALADIN_MOUNT, 75);
      send_to_char(ch, "\tMYou have learned 'Paladin Mount'\tn\r\n");
      GET_MOUNT(ch) = MOB_PALADIN_MOUNT;
      break;

    default:
      break;
  }
  return;  
}
#undef MOB_PALADIN_MOUNT

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 */
void sorc_skills(struct char_data *ch, int level) {
  IS_SORC_LEARNED(ch) = 0;
  send_to_char(ch,
         "\tnType \tDstudy sorcerer\tn to adjust your known spells.\r\n");
  
  switch (level) {
    case 2:
      if (!GET_SKILL(ch, SKILL_USE_MAGIC))
        SET_SKILL(ch, SKILL_USE_MAGIC, 75);
      send_to_char(ch, "\tMYou have learned 'Use Magic'\tn\r\n");
      break;
    default:
      break;
  }
  return;  
}

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 */
void wizard_skills(struct char_data *ch, int level) {
  IS_WIZ_LEARNED(ch) = 0;
  send_to_char(ch,
         "\tnType \tDstudy wizard\tn to adjust your wizard skills.\r\n");
  
  switch (level) {
    case 2:
      if (!GET_SKILL(ch, SKILL_USE_MAGIC))
        SET_SKILL(ch, SKILL_USE_MAGIC, 75);
      send_to_char(ch, "\tMYou have learned 'Use Magic'\tn\r\n");
      break;
    default:
      break;
  }
  return;  
}

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 */
void cleric_skills(struct char_data *ch, int level) {
  switch (level) {
    case 2:
      if (!GET_SKILL(ch, SKILL_USE_MAGIC))
        SET_SKILL(ch, SKILL_USE_MAGIC, 75);
      send_to_char(ch, "\tMYou have learned 'Use Magic'\tn\r\n");
      break;
    default:
      break;
  }
  return;  
}

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 */
void warrior_skills(struct char_data *ch, int level) {
  switch (level) {
    default:
      break;
  }
  return;  
}

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 */
void druid_skills(struct char_data *ch, int level) {
  switch (level) {
    case 2:
      if (!GET_SKILL(ch, SKILL_USE_MAGIC))
        SET_SKILL(ch, SKILL_USE_MAGIC, 75);
      send_to_char(ch, "\tMYou have learned 'Use Magic'\tn\r\n");
      break;
    default:
      break;
  }
  return;  
}

/* init spells for a class as they level up
 * i.e free skills  ;  make sure to set in spec_procs too
 */
void rogue_skills(struct char_data *ch, int level) {
  switch (level) {
    case 2:
      if (!GET_SKILL(ch, SKILL_MOBILITY))
        SET_SKILL(ch, SKILL_MOBILITY, 75);
      send_to_char(ch, "\tMYou have learned 'Mobility'\tn\r\n");
      if (!GET_SKILL(ch, SKILL_STEALTHY))
        SET_SKILL(ch, SKILL_STEALTHY, 75);
      send_to_char(ch, "\tMYou have learned 'Stealthy'\tn\r\n");
      break;
    case 3:
      if (!GET_SKILL(ch, SKILL_TRACK))
        SET_SKILL(ch, SKILL_TRACK, 75);
      send_to_char(ch, "\tMYou have learned 'Track'\tn\r\n");
      break;
    case 4:
      if (!GET_SKILL(ch, SKILL_DIRTY_FIGHTING))
        SET_SKILL(ch, SKILL_DIRTY_FIGHTING, 75);
      send_to_char(ch, "\tMYou have learned 'Dirty Fighting'\tn\r\n");
      break;
    case 6:
      if (!GET_SKILL(ch, SKILL_SPRING_ATTACK))
        SET_SKILL(ch, SKILL_SPRING_ATTACK, 75);
      send_to_char(ch, "\tMYou have learned 'Spring Attack'\tn\r\n");
      break;
    case 8:
      if (!GET_SKILL(ch, SKILL_EVASION))
        SET_SKILL(ch, SKILL_EVASION, 75);
      send_to_char(ch, "\tMYou have learned 'Evasion'\tn\r\n");
      break;
    case 9:
      if (!GET_SKILL(ch, SKILL_USE_MAGIC))
        SET_SKILL(ch, SKILL_USE_MAGIC, 75);
      send_to_char(ch, "\tMYou have learned 'Use Magic'\tn\r\n");
      break;
    case 12:
      if (!GET_SKILL(ch, SKILL_CRIP_STRIKE))
        SET_SKILL(ch, SKILL_CRIP_STRIKE, 75);
      send_to_char(ch, "\tMYou have learned 'Crippling Strike'\tn\r\n");
      break;
    case 15:
      if (!GET_SKILL(ch, SKILL_SLIPPERY_MIND))
        SET_SKILL(ch, SKILL_SLIPPERY_MIND, 75);
      send_to_char(ch, "\tMYou have learned 'Slippery Mind'\tn\r\n");
      break;
    case 18:
      if (!GET_SKILL(ch, SKILL_DEFENSE_ROLL))
        SET_SKILL(ch, SKILL_DEFENSE_ROLL, 75);
      send_to_char(ch, "\tMYou have learned 'Defensive Roll'\tn\r\n");
      break;
    case 21:
      if (!GET_SKILL(ch, SKILL_IMP_EVASION))
        SET_SKILL(ch, SKILL_IMP_EVASION, 75);
      send_to_char(ch, "\tMYou have learned 'Improved Evasion'\tn\r\n");
      break;
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
      if (!GET_SKILL(ch, SKILL_STUNNING_FIST))
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

  case CLASS_SORCERER:
  case CLASS_WIZARD:
    //spell init
    //1st circle
    SET_SKILL(ch, SPELL_HORIZIKAULS_BOOM, 99);
    SET_SKILL(ch, SPELL_MAGIC_MISSILE, 99);
    SET_SKILL(ch, SPELL_BURNING_HANDS, 99);
    SET_SKILL(ch, SPELL_ICE_DAGGER, 99);
    SET_SKILL(ch, SPELL_MAGE_ARMOR, 99);
    SET_SKILL(ch, SPELL_SUMMON_CREATURE_1, 99);
    SET_SKILL(ch, SPELL_CHILL_TOUCH, 99);
    SET_SKILL(ch, SPELL_NEGATIVE_ENERGY_RAY, 99);
    SET_SKILL(ch, SPELL_RAY_OF_ENFEEBLEMENT, 99);
    SET_SKILL(ch, SPELL_CHARM, 99);
    SET_SKILL(ch, SPELL_ENCHANT_WEAPON, 99);
    SET_SKILL(ch, SPELL_SLEEP, 99);
    SET_SKILL(ch, SPELL_COLOR_SPRAY, 99);
    SET_SKILL(ch, SPELL_SCARE, 99);
    SET_SKILL(ch, SPELL_TRUE_STRIKE, 99);
    SET_SKILL(ch, SPELL_IDENTIFY, 99);
    SET_SKILL(ch, SPELL_SHELGARNS_BLADE, 99);
    SET_SKILL(ch, SPELL_GREASE, 99);
    SET_SKILL(ch, SPELL_ENDURE_ELEMENTS, 99);
    SET_SKILL(ch, SPELL_PROT_FROM_EVIL, 99);
    SET_SKILL(ch, SPELL_PROT_FROM_GOOD, 99);    
    SET_SKILL(ch, SPELL_EXPEDITIOUS_RETREAT, 99);
    SET_SKILL(ch, SPELL_IRON_GUTS, 99);
    SET_SKILL(ch, SPELL_SHIELD, 99);

    //2nd circle
    SET_SKILL(ch, SPELL_SHOCKING_GRASP, 99);
    SET_SKILL(ch, SPELL_SCORCHING_RAY, 99);
    SET_SKILL(ch, SPELL_CONTINUAL_FLAME, 99);
    SET_SKILL(ch, SPELL_SUMMON_CREATURE_2, 99);
    SET_SKILL(ch, SPELL_WEB, 99);
    SET_SKILL(ch, SPELL_ACID_ARROW, 99);
    SET_SKILL(ch, SPELL_BLINDNESS, 99);
    SET_SKILL(ch, SPELL_DEAFNESS, 99);
    SET_SKILL(ch, SPELL_FALSE_LIFE, 99);
    SET_SKILL(ch, SPELL_DAZE_MONSTER, 99);
    SET_SKILL(ch, SPELL_HIDEOUS_LAUGHTER, 99);
    SET_SKILL(ch, SPELL_TOUCH_OF_IDIOCY, 99);
    SET_SKILL(ch, SPELL_BLUR, 99);
    SET_SKILL(ch, SPELL_MIRROR_IMAGE, 99);
    SET_SKILL(ch, SPELL_INVISIBLE, 99);
    SET_SKILL(ch, SPELL_DETECT_INVIS, 99);
    SET_SKILL(ch, SPELL_DETECT_MAGIC, 99);    
    SET_SKILL(ch, SPELL_DARKNESS, 99);
    SET_SKILL(ch, SPELL_RESIST_ENERGY, 99);
    SET_SKILL(ch, SPELL_ENERGY_SPHERE, 99);
    SET_SKILL(ch, SPELL_ENDURANCE, 99);
    SET_SKILL(ch, SPELL_STRENGTH, 99);
    SET_SKILL(ch, SPELL_GRACE, 99);

    //3rd circle
    SET_SKILL(ch, SPELL_LIGHTNING_BOLT, 99);
    SET_SKILL(ch, SPELL_FIREBALL, 99);
    SET_SKILL(ch, SPELL_WATER_BREATHE, 99);
    SET_SKILL(ch, SPELL_SUMMON_CREATURE_3, 99);
    SET_SKILL(ch, SPELL_PHANTOM_STEED, 99);
    SET_SKILL(ch, SPELL_STINKING_CLOUD, 99);
    SET_SKILL(ch, SPELL_HALT_UNDEAD, 99);
    SET_SKILL(ch, SPELL_VAMPIRIC_TOUCH, 99);
    SET_SKILL(ch, SPELL_HEROISM, 99);
    SET_SKILL(ch, SPELL_FLY, 99);
    SET_SKILL(ch, SPELL_HOLD_PERSON, 99);
    SET_SKILL(ch, SPELL_DEEP_SLUMBER, 99);
    SET_SKILL(ch, SPELL_WALL_OF_FOG, 99);
    SET_SKILL(ch, SPELL_INVISIBILITY_SPHERE, 99);
    SET_SKILL(ch, SPELL_DAYLIGHT, 99);
    SET_SKILL(ch, SPELL_CLAIRVOYANCE, 99);
    SET_SKILL(ch, SPELL_NON_DETECTION, 99);
    SET_SKILL(ch, SPELL_DISPEL_MAGIC, 99);
    SET_SKILL(ch, SPELL_HASTE, 99);
    SET_SKILL(ch, SPELL_SLOW, 99);
    SET_SKILL(ch, SPELL_CIRCLE_A_EVIL, 99);
    SET_SKILL(ch, SPELL_CIRCLE_A_GOOD, 99);
    SET_SKILL(ch, SPELL_CUNNING, 99);
    SET_SKILL(ch, SPELL_WISDOM, 99);
    SET_SKILL(ch, SPELL_CHARISMA, 99);
    
    //4th circle
    SET_SKILL(ch, SPELL_FIRE_SHIELD, 99);
    SET_SKILL(ch, SPELL_COLD_SHIELD, 99);
    SET_SKILL(ch, SPELL_ICE_STORM, 99);
    SET_SKILL(ch, SPELL_BILLOWING_CLOUD, 99);
    SET_SKILL(ch, SPELL_SUMMON_CREATURE_4, 99);
    SET_SKILL(ch, SPELL_ANIMATE_DEAD, 99);
    SET_SKILL(ch, SPELL_CURSE, 99);
    SET_SKILL(ch, SPELL_INFRAVISION, 99); //shared
    SET_SKILL(ch, SPELL_POISON, 99);  //shared
    SET_SKILL(ch, SPELL_GREATER_INVIS, 99);
    SET_SKILL(ch, SPELL_RAINBOW_PATTERN, 99);
    SET_SKILL(ch, SPELL_WIZARD_EYE, 99);
    SET_SKILL(ch, SPELL_LOCATE_CREATURE, 99);
    SET_SKILL(ch, SPELL_MINOR_GLOBE, 99);
    SET_SKILL(ch, SPELL_REMOVE_CURSE, 99);  //shared
    SET_SKILL(ch, SPELL_STONESKIN, 99);
    SET_SKILL(ch, SPELL_ENLARGE_PERSON, 99);
    SET_SKILL(ch, SPELL_SHRINK_PERSON, 99);

    //5th circle
    SET_SKILL(ch, SPELL_INTERPOSING_HAND, 99);
    SET_SKILL(ch, SPELL_WALL_OF_FORCE, 99);
    SET_SKILL(ch, SPELL_BALL_OF_LIGHTNING, 99);
    SET_SKILL(ch, SPELL_CLOUDKILL, 99);
    SET_SKILL(ch, SPELL_SUMMON_CREATURE_5, 99);
    SET_SKILL(ch, SPELL_WAVES_OF_FATIGUE, 99);
    SET_SKILL(ch, SPELL_SYMBOL_OF_PAIN, 99);
    SET_SKILL(ch, SPELL_DOMINATE_PERSON, 99);
    SET_SKILL(ch, SPELL_FEEBLEMIND, 99);
    SET_SKILL(ch, SPELL_NIGHTMARE, 99);
    SET_SKILL(ch, SPELL_MIND_FOG, 99);
    SET_SKILL(ch, SPELL_ACID_SHEATH, 99);
    SET_SKILL(ch, SPELL_FAITHFUL_HOUND, 99);
    SET_SKILL(ch, SPELL_DISMISSAL, 99);
    SET_SKILL(ch, SPELL_CONE_OF_COLD, 99);
    SET_SKILL(ch, SPELL_TELEKINESIS, 99);
    SET_SKILL(ch, SPELL_FIREBRAND, 99);

    //6th circle
    SET_SKILL(ch, SPELL_FREEZING_SPHERE, 99);
    SET_SKILL(ch, SPELL_ACID_FOG, 99);
    SET_SKILL(ch, SPELL_SUMMON_CREATURE_6, 99);
    SET_SKILL(ch, SPELL_TRANSFORMATION, 99);
    SET_SKILL(ch, SPELL_EYEBITE, 99);
    SET_SKILL(ch, SPELL_MASS_HASTE, 99);
    SET_SKILL(ch, SPELL_GREATER_HEROISM, 99);
    SET_SKILL(ch, SPELL_ANTI_MAGIC_FIELD, 99);
    SET_SKILL(ch, SPELL_GREATER_MIRROR_IMAGE, 99);
    SET_SKILL(ch, SPELL_LOCATE_OBJECT, 99);
    SET_SKILL(ch, SPELL_TRUE_SEEING, 99);
    SET_SKILL(ch, SPELL_GLOBE_OF_INVULN, 99);
    SET_SKILL(ch, SPELL_GREATER_DISPELLING, 99);
    SET_SKILL(ch, SPELL_CLONE, 99);
    SET_SKILL(ch, SPELL_WATERWALK, 99);
    
    //7th circle
    SET_SKILL(ch, SPELL_MISSILE_STORM, 99);
    SET_SKILL(ch, SPELL_TELEPORT, 99);

    //8th circle
    SET_SKILL(ch, SPELL_CHAIN_LIGHTNING, 99);
    SET_SKILL(ch, SPELL_PORTAL, 99);

    //9th circle
    SET_SKILL(ch, SPELL_ENERGY_DRAIN, 99);
    SET_SKILL(ch, SPELL_METEOR_SWARM, 99);
    SET_SKILL(ch, SPELL_POLYMORPH, 99);
    
    // skill init    
    if (!GET_SKILL(ch, SKILL_PROF_MINIMAL))
      SET_SKILL(ch, SKILL_PROF_MINIMAL, 75);
    if (!GET_SKILL(ch, SKILL_CALL_FAMILIAR))
      SET_SKILL(ch, SKILL_CALL_FAMILIAR, 75);

     // wizard innate cantrips
    /*
    SET_SKILL(ch, SPELL_ACID_SPLASH, 99);
    SET_SKILL(ch, SPELL_RAY_OF_FROST, 99);
    */

    send_to_char(ch, "Magic-User / Sorcerer Done.\tn\r\n");
  break;


  case CLASS_CLERIC:
    //spell init
    //1st circle
    SET_SKILL(ch, SPELL_ENDURANCE, 99);
    SET_SKILL(ch, SPELL_CURE_LIGHT, 99);
    SET_SKILL(ch, SPELL_ARMOR, 99);
    SET_SKILL(ch, SPELL_CAUSE_LIGHT_WOUNDS, 99);
    //2nd circle
    SET_SKILL(ch, SPELL_CREATE_FOOD, 99);
    SET_SKILL(ch, SPELL_CREATE_WATER, 99);
    SET_SKILL(ch, SPELL_DETECT_POISON, 99);
    SET_SKILL(ch, SPELL_CAUSE_MODERATE_WOUNDS, 99);
    //3rd circle
    SET_SKILL(ch, SPELL_DETECT_ALIGN, 99);
    SET_SKILL(ch, SPELL_CURE_BLIND, 99);
    SET_SKILL(ch, SPELL_BLESS, 99);
    SET_SKILL(ch, SPELL_CAUSE_SERIOUS_WOUNDS, 99);    
    //4th circle
    SET_SKILL(ch, SPELL_INFRAVISION, 99);
    SET_SKILL(ch, SPELL_REMOVE_CURSE, 99);
    SET_SKILL(ch, SPELL_CAUSE_CRITICAL_WOUNDS, 99);
    SET_SKILL(ch, SPELL_CURE_CRITIC, 99);
    //5th circle
    SET_SKILL(ch, SPELL_BLINDNESS, 99);
    SET_SKILL(ch, SPELL_PROT_FROM_EVIL, 99);
    SET_SKILL(ch, SPELL_PROT_FROM_GOOD, 99);
    SET_SKILL(ch, SPELL_POISON, 99);
    SET_SKILL(ch, SPELL_GROUP_ARMOR, 99);
    SET_SKILL(ch, SPELL_FLAME_STRIKE, 99);
    //6th circle
    SET_SKILL(ch, SPELL_DISPEL_EVIL, 99);
    SET_SKILL(ch, SPELL_DISPEL_GOOD, 99);
    SET_SKILL(ch, SPELL_REMOVE_POISON, 99);
    SET_SKILL(ch, SPELL_HARM, 99);
    SET_SKILL(ch, SPELL_HEAL, 99);
    //7th circle
    SET_SKILL(ch, SPELL_CONTROL_WEATHER, 99);
    SET_SKILL(ch, SPELL_SUMMON, 99);
    SET_SKILL(ch, SPELL_WORD_OF_RECALL, 99);
    SET_SKILL(ch, SPELL_CALL_LIGHTNING, 99);
    //8th circle
    SET_SKILL(ch, SPELL_SENSE_LIFE, 99);
    SET_SKILL(ch, SPELL_SANCTUARY, 99);
    SET_SKILL(ch, SPELL_DESTRUCTION, 99);
    //9th circle
    SET_SKILL(ch, SPELL_EARTHQUAKE, 99);
    SET_SKILL(ch, SPELL_GROUP_HEAL, 99);
    SET_SKILL(ch, SPELL_ENERGY_DRAIN, 99);
    
    // skill init
    if (!GET_SKILL(ch, SKILL_PROF_MINIMAL))
      SET_SKILL(ch, SKILL_PROF_MINIMAL, 75);
    if (!GET_SKILL(ch, SKILL_PROF_BASIC))
      SET_SKILL(ch, SKILL_PROF_BASIC, 75);
    if (!GET_SKILL(ch, SKILL_PROF_LIGHT_A))
      SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_MEDIUM_A))
      SET_SKILL(ch, SKILL_PROF_MEDIUM_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_HEAVY_A))
      SET_SKILL(ch, SKILL_PROF_HEAVY_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_SHIELDS))
      SET_SKILL(ch, SKILL_PROF_SHIELDS, 75);

            
    send_to_char(ch, "Cleric Done.\tn\r\n");
  break;


  case CLASS_DRUID:
    //spell init
    //1st circle
    SET_SKILL(ch, SPELL_ENDURANCE, 99);
    SET_SKILL(ch, SPELL_CURE_LIGHT, 99);
    SET_SKILL(ch, SPELL_ARMOR, 99);
    SET_SKILL(ch, SPELL_CAUSE_LIGHT_WOUNDS, 99);
    //2nd circle
    SET_SKILL(ch, SPELL_CREATE_FOOD, 99);
    SET_SKILL(ch, SPELL_CREATE_WATER, 99);
    SET_SKILL(ch, SPELL_DETECT_POISON, 99);
    SET_SKILL(ch, SPELL_CAUSE_MODERATE_WOUNDS, 99);
    //3rd circle
    SET_SKILL(ch, SPELL_DETECT_ALIGN, 99);
    SET_SKILL(ch, SPELL_CURE_BLIND, 99);
    SET_SKILL(ch, SPELL_BLESS, 99);
    SET_SKILL(ch, SPELL_CAUSE_SERIOUS_WOUNDS, 99);    
    //4th circle
    SET_SKILL(ch, SPELL_INFRAVISION, 99);
    SET_SKILL(ch, SPELL_REMOVE_CURSE, 99);
    SET_SKILL(ch, SPELL_CAUSE_CRITICAL_WOUNDS, 99);
    SET_SKILL(ch, SPELL_CURE_CRITIC, 99);
    //5th circle
    SET_SKILL(ch, SPELL_BLINDNESS, 99);
    SET_SKILL(ch, SPELL_PROT_FROM_EVIL, 99);
    SET_SKILL(ch, SPELL_PROT_FROM_GOOD, 99);
    SET_SKILL(ch, SPELL_POISON, 99);
    SET_SKILL(ch, SPELL_GROUP_ARMOR, 99);
    SET_SKILL(ch, SPELL_FLAME_STRIKE, 99);
    //6th circle
    SET_SKILL(ch, SPELL_DISPEL_EVIL, 99);
    SET_SKILL(ch, SPELL_DISPEL_GOOD, 99);
    SET_SKILL(ch, SPELL_REMOVE_POISON, 99);
    SET_SKILL(ch, SPELL_HARM, 99);
    SET_SKILL(ch, SPELL_HEAL, 99);
    //7th circle
    SET_SKILL(ch, SPELL_CONTROL_WEATHER, 99);
    SET_SKILL(ch, SPELL_SUMMON, 99);
    SET_SKILL(ch, SPELL_WORD_OF_RECALL, 99);
    SET_SKILL(ch, SPELL_CALL_LIGHTNING, 99);
    //8th circle
    SET_SKILL(ch, SPELL_SENSE_LIFE, 99);
    SET_SKILL(ch, SPELL_SANCTUARY, 99);
    SET_SKILL(ch, SPELL_DESTRUCTION, 99);
    //9th circle
    SET_SKILL(ch, SPELL_EARTHQUAKE, 99);
    SET_SKILL(ch, SPELL_GROUP_HEAL, 99);
    SET_SKILL(ch, SPELL_ENERGY_DRAIN, 99);
    
    // skill init
    if (!GET_SKILL(ch, SKILL_PROF_MINIMAL))
      SET_SKILL(ch, SKILL_PROF_MINIMAL, 75);
    if (!GET_SKILL(ch, SKILL_PROF_BASIC))
      SET_SKILL(ch, SKILL_PROF_BASIC, 75);
    if (!GET_SKILL(ch, SKILL_PROF_ADVANCED))
      SET_SKILL(ch, SKILL_PROF_ADVANCED, 75);
    if (!GET_SKILL(ch, SKILL_PROF_LIGHT_A))
      SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_MEDIUM_A))
      SET_SKILL(ch, SKILL_PROF_MEDIUM_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_SHIELDS))
      SET_SKILL(ch, SKILL_PROF_SHIELDS, 75);

    send_to_char(ch, "Druid Done.\tn\r\n");
  break;

  case CLASS_PALADIN:
    //spell init
    //1st circle
    SET_SKILL(ch, SPELL_CURE_LIGHT, 99);
    SET_SKILL(ch, SPELL_ENDURANCE, 99);
    SET_SKILL(ch, SPELL_ARMOR, 99);
    SET_SKILL(ch, SPELL_CAUSE_LIGHT_WOUNDS, 99);
    //2nd circle
    SET_SKILL(ch, SPELL_CREATE_FOOD, 99);
    SET_SKILL(ch, SPELL_CREATE_WATER, 99);
    SET_SKILL(ch, SPELL_DETECT_POISON, 99);
    SET_SKILL(ch, SPELL_CAUSE_MODERATE_WOUNDS, 99);
    //3rd circle
    SET_SKILL(ch, SPELL_DETECT_ALIGN, 99);
    SET_SKILL(ch, SPELL_CURE_BLIND, 99);
    SET_SKILL(ch, SPELL_BLESS, 99);
    SET_SKILL(ch, SPELL_CAUSE_SERIOUS_WOUNDS, 99);
    //4th circle
    SET_SKILL(ch, SPELL_INFRAVISION, 99);
    SET_SKILL(ch, SPELL_REMOVE_CURSE, 99);
    SET_SKILL(ch, SPELL_CAUSE_CRITICAL_WOUNDS, 99);
    SET_SKILL(ch, SPELL_CURE_CRITIC, 99);
    
    // skill init
    if (!GET_SKILL(ch, SKILL_LAY_ON_HANDS))
      SET_SKILL(ch, SKILL_LAY_ON_HANDS, 75);

    if (!GET_SKILL(ch, SKILL_PROF_MINIMAL))
      SET_SKILL(ch, SKILL_PROF_MINIMAL, 75);
    if (!GET_SKILL(ch, SKILL_PROF_BASIC))
      SET_SKILL(ch, SKILL_PROF_BASIC, 75);
    if (!GET_SKILL(ch, SKILL_PROF_ADVANCED))
      SET_SKILL(ch, SKILL_PROF_ADVANCED, 75);
    if (!GET_SKILL(ch, SKILL_PROF_MASTER))
      SET_SKILL(ch, SKILL_PROF_MASTER, 75);
    if (!GET_SKILL(ch, SKILL_PROF_LIGHT_A))
      SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_MEDIUM_A))
      SET_SKILL(ch, SKILL_PROF_MEDIUM_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_HEAVY_A))
      SET_SKILL(ch, SKILL_PROF_HEAVY_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_SHIELDS))
      SET_SKILL(ch, SKILL_PROF_SHIELDS, 75);
    send_to_char(ch, "Paladin Done.\tn\r\n");
  break;

  case CLASS_ROGUE:
    if (!GET_SKILL(ch, SKILL_PROF_MINIMAL))
      SET_SKILL(ch, SKILL_PROF_MINIMAL, 75);
    if (!GET_SKILL(ch, SKILL_PROF_BASIC))
      SET_SKILL(ch, SKILL_PROF_BASIC, 75);
    if (!GET_SKILL(ch, SKILL_PROF_ADVANCED))
      SET_SKILL(ch, SKILL_PROF_ADVANCED, 75);
    if (!GET_SKILL(ch, SKILL_PROF_LIGHT_A))
      SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);

    if (!GET_SKILL(ch, SKILL_BACKSTAB))
      SET_SKILL(ch, SKILL_BACKSTAB, 75);
    if (!GET_SKILL(ch, SKILL_TRACK))
      SET_SKILL(ch, SKILL_TRACK, 75);
    
    send_to_char(ch, "Rogue Done.\tn\r\n");
  break;


  case CLASS_WARRIOR:
    if (!GET_SKILL(ch, SKILL_PROF_MINIMAL))
      SET_SKILL(ch, SKILL_PROF_MINIMAL, 75);
    if (!GET_SKILL(ch, SKILL_PROF_BASIC))
      SET_SKILL(ch, SKILL_PROF_BASIC, 75);
    if (!GET_SKILL(ch, SKILL_PROF_ADVANCED))
      SET_SKILL(ch, SKILL_PROF_ADVANCED, 75);
    if (!GET_SKILL(ch, SKILL_PROF_MASTER))
      SET_SKILL(ch, SKILL_PROF_MASTER, 75);
    if (!GET_SKILL(ch, SKILL_PROF_LIGHT_A))
      SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_MEDIUM_A))
      SET_SKILL(ch, SKILL_PROF_MEDIUM_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_HEAVY_A))
      SET_SKILL(ch, SKILL_PROF_HEAVY_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_SHIELDS))
      SET_SKILL(ch, SKILL_PROF_SHIELDS, 75);
    if (!GET_SKILL(ch, SKILL_PROF_T_SHIELDS))
      SET_SKILL(ch, SKILL_PROF_T_SHIELDS, 75);
    send_to_char(ch, "Warrior Done.\tn\r\n");
  break;

  case CLASS_RANGER:
    //spell init
    //1st circle
    SET_SKILL(ch, SPELL_CURE_LIGHT, 99);
    SET_SKILL(ch, SPELL_ENDURANCE, 99);
    SET_SKILL(ch, SPELL_ARMOR, 99);
    SET_SKILL(ch, SPELL_CAUSE_LIGHT_WOUNDS, 99);
    //2nd circle
    SET_SKILL(ch, SPELL_CREATE_FOOD, 99);
    SET_SKILL(ch, SPELL_CREATE_WATER, 99);
    SET_SKILL(ch, SPELL_DETECT_POISON, 99);
    SET_SKILL(ch, SPELL_CAUSE_MODERATE_WOUNDS, 99);
    //3rd circle
    SET_SKILL(ch, SPELL_DETECT_ALIGN, 99);
    SET_SKILL(ch, SPELL_CURE_BLIND, 99);
    SET_SKILL(ch, SPELL_BLESS, 99);
    SET_SKILL(ch, SPELL_CAUSE_SERIOUS_WOUNDS, 99);
    //4th circle
    SET_SKILL(ch, SPELL_INFRAVISION, 99);
    SET_SKILL(ch, SPELL_REMOVE_CURSE, 99);
    SET_SKILL(ch, SPELL_CAUSE_CRITICAL_WOUNDS, 99);
    SET_SKILL(ch, SPELL_CURE_CRITIC, 99);
    
    //skill init
    if (!GET_SKILL(ch, SKILL_FAVORED_ENEMY))
      SET_SKILL(ch, SKILL_FAVORED_ENEMY, 75);
   
    if (!GET_SKILL(ch, SKILL_PROF_MINIMAL))
      SET_SKILL(ch, SKILL_PROF_MINIMAL, 75);
    if (!GET_SKILL(ch, SKILL_PROF_BASIC))
      SET_SKILL(ch, SKILL_PROF_BASIC, 75);
    if (!GET_SKILL(ch, SKILL_PROF_ADVANCED))
      SET_SKILL(ch, SKILL_PROF_ADVANCED, 75);
    if (!GET_SKILL(ch, SKILL_PROF_MASTER))
      SET_SKILL(ch, SKILL_PROF_MASTER, 75);
    if (!GET_SKILL(ch, SKILL_PROF_LIGHT_A))
      SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_MEDIUM_A))
      SET_SKILL(ch, SKILL_PROF_MEDIUM_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_SHIELDS))
      SET_SKILL(ch, SKILL_PROF_SHIELDS, 75);
    send_to_char(ch, "Ranger Done.\tn\r\n");
  break;
  
  case CLASS_BERSERKER:
    if (!GET_SKILL(ch, SKILL_PROF_MINIMAL))
      SET_SKILL(ch, SKILL_PROF_MINIMAL, 75);
    if (!GET_SKILL(ch, SKILL_PROF_BASIC))
      SET_SKILL(ch, SKILL_PROF_BASIC, 75);
    if (!GET_SKILL(ch, SKILL_PROF_ADVANCED))
      SET_SKILL(ch, SKILL_PROF_ADVANCED, 75);
    if (!GET_SKILL(ch, SKILL_PROF_MASTER))
      SET_SKILL(ch, SKILL_PROF_MASTER, 75);
    if (!GET_SKILL(ch, SKILL_PROF_LIGHT_A))
      SET_SKILL(ch, SKILL_PROF_LIGHT_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_MEDIUM_A))
      SET_SKILL(ch, SKILL_PROF_MEDIUM_A, 75);
    if (!GET_SKILL(ch, SKILL_PROF_SHIELDS))
      SET_SKILL(ch, SKILL_PROF_SHIELDS, 75);
    send_to_char(ch, "Berserker Done.\tn\r\n");
  break;

  case CLASS_MONK:
    if (!GET_SKILL(ch, SKILL_PROF_MINIMAL))
      SET_SKILL(ch, SKILL_PROF_MINIMAL, 75);
    send_to_char(ch, "Monk Done.\tn\r\n");    
  break;

  default:
    send_to_char(ch, "None needed.\tn\r\n");
  break;
  }
}

/* not to be confused with init_char, this is exclusive right now for do_start */
void init_start_char(struct char_data *ch)
{
  int trains = 0, practices = 0, i = 0;
  
  /* clear immortal flags */
  if (PRF_FLAGGED(ch, PRF_HOLYLIGHT))
    i = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
  if (PRF_FLAGGED(ch, PRF_NOHASSLE))
    i = PRF_TOG_CHK(ch, PRF_NOHASSLE);
  if (PRF_FLAGGED(ch, PRF_SHOWVNUMS))
    i = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
  if (PRF_FLAGGED(ch, PRF_BUILDWALK))
    i = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    
  /* clear gear for clean start */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      perform_remove(ch, i, TRUE);

  /* clear affects for clean start */
  if (ch->affected || AFF_FLAGS(ch)) {  
    while (ch->affected)
      affect_remove(ch, ch->affected);
    for(i=0; i < AF_ARRAY_MAX; i++)
      AFF_FLAGS(ch)[i] = 0;
  }

  /* initialize all levels and spec_abil array */
  for (i = 0; i < MAX_CLASSES; i++) {
    CLASS_LEVEL(ch, i) = 0;
    GET_SPEC_ABIL(ch, i) = 0;
  }
  for (i = 0; i < MAX_ENEMIES; i++)
    GET_FAVORED_ENEMY(ch, i) = 0;
  
  /* a bit silly, but go ahead make sure no stone-skin like spells */
  for (i = 0; i < MAX_WARDING; i++)
    GET_WARDING(ch, i) = 0;

  /* start at level 1 */
  GET_LEVEL(ch) = 1;
  CLASS_LEVEL(ch, GET_CLASS(ch)) = 1;
  GET_EXP(ch) = 1;

  /* reset title */
  set_title(ch, NULL);
  
  /* reset stats */
  roll_real_abils(ch);
  GET_AC(ch) = 100;
  GET_HITROLL(ch) = 0;
  GET_DAMROLL(ch) = 0;
  GET_MAX_HIT(ch)  = 20;
  GET_MAX_MANA(ch) = 100;
  GET_MAX_MOVE(ch) = 82;
  GET_PRACTICES(ch) = 0;
  GET_TRAINS(ch) = 0;
  GET_BOOSTS(ch) = 4;  //freebies
  GET_SPELL_RES(ch) = 0;

  /* reset skills/abilities */
  for (i=1; i<=NUM_SKILLS; i++)
    SET_SKILL(ch, i, 0);
  for (i=1; i<=NUM_ABILITIES; i++)
    SET_ABILITY(ch, i, 0);

  /* initialize mem data, allow adjustment of spells known */
  init_spell_slots(ch);
  IS_SORC_LEARNED(ch) = 0;
  IS_RANG_LEARNED(ch) = 0;
  IS_WIZ_LEARNED(ch) = 0;

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
      SET_SKILL(ch, SKILL_PROF_BASIC, 75);
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
      SET_SKILL(ch, SKILL_PROF_BASIC, 75);
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
    case RACE_TRELUX:
      GET_SIZE(ch) = SIZE_SMALL;
      ch->real_abils.str += 2;
      ch->real_abils.dex += 8;
      ch->real_abils.con += 4;
      GET_MAX_HIT(ch) += 10;
      break;    
    default:
      GET_SIZE(ch) = SIZE_MEDIUM;
      break;
  }

  //class-related inits
  switch (GET_CLASS(ch)) {
  case CLASS_WARRIOR:
    practices++;  // bonus skill
  case CLASS_SORCERER:
  case CLASS_WIZARD:
  case CLASS_CLERIC:
    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))) * 3);
    break;
  case CLASS_DRUID:
  case CLASS_RANGER:
  case CLASS_BERSERKER:
  case CLASS_MONK:
    trains += MAX(1, (4 + (int)(GET_INT_BONUS(ch))) * 3);
    break;
  case CLASS_ROGUE:
    trains += MAX(1, (8 + (int)(GET_INT_BONUS(ch))) * 3);
    break;
  }

  /* finalize */
  practices++;
  GET_PRACTICES(ch) += practices;
  send_to_char(ch, "%d \tMPractice sessions gained.\tn\r\n", practices);
  GET_TRAINS(ch) += trains;
  send_to_char(ch, "%d \tMTraining sessions gained.\tn\r\n", trains);
}

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  init_start_char(ch);

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
  struct affected_type *af = NULL;

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

  /* first level in a class?  might have some inits to do! */
  if (CLASS_LEVEL(ch, class) == 1) {
    send_to_char(ch, "\tMInititializing class:  ");
    init_class(ch, class, CLASS_LEVEL(ch, class));
    send_to_char(ch, "\r\n");
  }

  /* start our primary advancement block */
  send_to_char(ch, "\tMGAINS:\tn\r\n");
  
  /* class bonuses */
  switch (class) {
  case CLASS_SORCERER:
    sorc_skills(ch, CLASS_LEVEL(ch, CLASS_SORCERER));
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
  case CLASS_WIZARD:
    wizard_skills(ch, CLASS_LEVEL(ch, CLASS_WIZARD));
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
    cleric_skills(ch, CLASS_LEVEL(ch, CLASS_CLERIC));
    add_hp += rand_number(4, 8);
    add_mana = 0;
    add_move = rand_number(0, 2);

    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))));

    //epic
    if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20)
      practices++;

    break;
  case CLASS_ROGUE:
    rogue_skills(ch, CLASS_LEVEL(ch, CLASS_ROGUE));
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
    add_move = rand_number(2, 6);

    trains += MAX(1, (4 + (int)(GET_INT_BONUS(ch))));

    //epic
    if (!(CLASS_LEVEL(ch, class) % 4) && GET_LEVEL(ch) >= 20)
      practices++;

    break;
  case CLASS_DRUID:
    druid_skills(ch, CLASS_LEVEL(ch, CLASS_SORCERER));    
    add_hp += rand_number(4, 8);
    add_mana = 0;
    add_move = rand_number(4, 8);

    trains += MAX(1, (4 + (int)(GET_INT_BONUS(ch))));

    //epic
    if (!(CLASS_LEVEL(ch, class) % 4) && GET_LEVEL(ch) >= 20)
      practices++;

    break;    
  case CLASS_RANGER:
    ranger_skills(ch, CLASS_LEVEL(ch, CLASS_RANGER));
    add_hp += rand_number(5, 10);
    add_mana = 0;
    add_move = rand_number(4, 8);

    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))));

    //epic
    if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20)
      practices++;

    break;    
  case CLASS_PALADIN:
    paladin_skills(ch, CLASS_LEVEL(ch, CLASS_PALADIN));
    add_hp += rand_number(5, 10);
    add_mana = 0;
    add_move = 1;

    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))));

    //epic
    if (!(CLASS_LEVEL(ch, class) % 3) && GET_LEVEL(ch) >= 20)
      practices++;

    break;
  case CLASS_WARRIOR:
    warrior_skills(ch, CLASS_LEVEL(ch, CLASS_WARRIOR));
    add_hp += rand_number(5, 10);
    add_mana = 0;
    add_move = rand_number(1, 2);

    trains += MAX(1, (2 + (int)(GET_INT_BONUS(ch))));

    if (!(CLASS_LEVEL(ch, class) % 2))
      practices++;

    break;
  }
  
  //Racial Bonuses
  switch (GET_RACE(ch)) {
    case RACE_HUMAN:
      trains++;
      break;
    case RACE_CRYSTAL_DWARF:
      add_hp += 4;
      break;
    case RACE_TRELUX:
      add_hp += 4;
      break;
    default:
      break;
  }
  
  //base practice / boost improvement
  if (!(GET_LEVEL(ch) % 3))
    practices++;
  if (!(GET_LEVEL(ch) % 4)) {
    GET_BOOSTS(ch)++;
    send_to_char(ch, "\tMYou gain a boost (to stats) point!\tn\r\n");
  }

  //remember, GET_CON(ch) is adjusted con,
  //ch->real_abils.con is natural con -zusuk

  /* miscellaneous level-based bonuses */
  if (GET_SKILL(ch, SKILL_TOUGHNESS))
    add_hp++;
  if (GET_SKILL(ch, SKILL_EPIC_TOUGHNESS))
    add_hp++;

  /* adjust final and report changes! */
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
  /*******/
  /* end advancement block */
  /*******/

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

  /* give immorts some goodies */
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (k = 0; k < 3; k++)
      GET_COND(ch, k) = (char) -1;
    SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    send_to_char(ch, "Setting \tRHOLYLIGHT\tn on.\r\n");
    SET_BIT_AR(PRF_FLAGS(ch), PRF_NOHASSLE);
    send_to_char(ch, "Setting \tRNOHASSLE\tn on.\r\n");
    SET_BIT_AR(PRF_FLAGS(ch), PRF_SHOWVNUMS);
    send_to_char(ch, "Setting \tRSHOWVNUMS\tn on.\r\n");
  }

  /* make sure you aren't snooping someone you shouldn't with new level */
  snoop_check(ch);
  save_char(ch, 0);
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
  if (OBJ_FLAGGED(obj, ITEM_ANTI_WIZARD) && IS_WIZARD(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_SORCERER) && IS_SORCERER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_RANGER) && IS_RANGER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_PALADIN) && IS_PALADIN(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_MONK) && IS_MONK(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BERSERKER) && IS_BERSERKER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DRUID) && IS_DRUID(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ROGUE) && IS_ROGUE(ch))
    return TRUE;

  return FALSE;
}


// vital min level info!
void init_spell_levels(void)
{
  int i = 0, j = 0;

  // simple loop to init all the SKILL_x to all classes
  for (i = (MAX_SPELLS + 1); i < NUM_SKILLS; i++) {
    for (j = 0; j < NUM_CLASSES; j++) {
      spell_level(i, j, 1);
    }
  }

  // wizard innate cantrips
  /*
  spell_level(SPELL_ACID_SPLASH, CLASS_WIZARD, 1);
  spell_level(SPELL_RAY_OF_FROST, CLASS_WIZARD, 1);
  */
  
  // wizard, increment spells by spell-level
  //1st circle
  spell_level(SPELL_MAGIC_MISSILE, CLASS_WIZARD, 1);
  spell_level(SPELL_HORIZIKAULS_BOOM, CLASS_WIZARD, 1);
  spell_level(SPELL_BURNING_HANDS, CLASS_WIZARD, 1);
  spell_level(SPELL_ICE_DAGGER, CLASS_WIZARD, 1);
  spell_level(SPELL_MAGE_ARMOR, CLASS_WIZARD, 1);
  spell_level(SPELL_SUMMON_CREATURE_1, CLASS_WIZARD, 1);
  spell_level(SPELL_CHILL_TOUCH, CLASS_WIZARD, 1);
  spell_level(SPELL_NEGATIVE_ENERGY_RAY, CLASS_WIZARD, 1);
  spell_level(SPELL_RAY_OF_ENFEEBLEMENT, CLASS_WIZARD, 1);
  spell_level(SPELL_CHARM, CLASS_WIZARD, 1);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_WIZARD, 1);
  spell_level(SPELL_SLEEP, CLASS_WIZARD, 1);
  spell_level(SPELL_COLOR_SPRAY, CLASS_WIZARD, 1);
  spell_level(SPELL_SCARE, CLASS_WIZARD, 1);
  spell_level(SPELL_TRUE_STRIKE, CLASS_WIZARD, 1);
  spell_level(SPELL_IDENTIFY, CLASS_WIZARD, 1);
  spell_level(SPELL_SHELGARNS_BLADE, CLASS_WIZARD, 1);
  spell_level(SPELL_GREASE, CLASS_WIZARD, 1);
  spell_level(SPELL_ENDURE_ELEMENTS, CLASS_WIZARD, 1);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_WIZARD, 1);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_WIZARD, 1);
  spell_level(SPELL_EXPEDITIOUS_RETREAT, CLASS_WIZARD, 1);
  spell_level(SPELL_IRON_GUTS, CLASS_WIZARD, 1);  
  spell_level(SPELL_SHIELD, CLASS_WIZARD, 1);
  
  //2nd circle
  spell_level(SPELL_SHOCKING_GRASP, CLASS_WIZARD, 3);
  spell_level(SPELL_SCORCHING_RAY, CLASS_WIZARD, 3);
  spell_level(SPELL_CONTINUAL_FLAME, CLASS_WIZARD, 3);
  spell_level(SPELL_SUMMON_CREATURE_2, CLASS_WIZARD, 3);
  spell_level(SPELL_WEB, CLASS_WIZARD, 3);
  spell_level(SPELL_ACID_ARROW, CLASS_WIZARD, 3);
  spell_level(SPELL_BLINDNESS, CLASS_WIZARD, 3);
  spell_level(SPELL_DEAFNESS, CLASS_WIZARD, 3);
  spell_level(SPELL_FALSE_LIFE, CLASS_WIZARD, 3);  
  spell_level(SPELL_DAZE_MONSTER, CLASS_WIZARD, 3);  
  spell_level(SPELL_HIDEOUS_LAUGHTER, CLASS_WIZARD, 3);  
  spell_level(SPELL_TOUCH_OF_IDIOCY, CLASS_WIZARD, 3);  
  spell_level(SPELL_BLUR, CLASS_WIZARD, 3);
  spell_level(SPELL_INVISIBLE, CLASS_WIZARD, 3);  
  spell_level(SPELL_MIRROR_IMAGE, CLASS_WIZARD, 3);
  spell_level(SPELL_DETECT_INVIS, CLASS_WIZARD, 3);
  spell_level(SPELL_DETECT_MAGIC, CLASS_WIZARD, 3);
  spell_level(SPELL_DARKNESS, CLASS_WIZARD, 3);
  spell_level(SPELL_RESIST_ENERGY, CLASS_WIZARD, 3);
  spell_level(SPELL_ENERGY_SPHERE, CLASS_WIZARD, 3);
  spell_level(SPELL_ENDURANCE, CLASS_WIZARD, 3);  //shared
  spell_level(SPELL_STRENGTH, CLASS_WIZARD, 3);
  spell_level(SPELL_GRACE, CLASS_WIZARD, 3);  

  //3rd circle
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_WIZARD, 5);
  spell_level(SPELL_FIREBALL, CLASS_WIZARD, 5);
  spell_level(SPELL_WATER_BREATHE, CLASS_WIZARD, 5);
  spell_level(SPELL_NON_DETECTION, CLASS_WIZARD, 5);
  spell_level(SPELL_CLAIRVOYANCE, CLASS_WIZARD, 5);
  spell_level(SPELL_DAYLIGHT, CLASS_WIZARD, 5);
  spell_level(SPELL_INVISIBILITY_SPHERE, CLASS_WIZARD, 5);
  spell_level(SPELL_WALL_OF_FOG, CLASS_WIZARD, 5);
  spell_level(SPELL_DEEP_SLUMBER, CLASS_WIZARD, 5);
  spell_level(SPELL_HOLD_PERSON, CLASS_WIZARD, 5);
  spell_level(SPELL_FLY, CLASS_WIZARD, 5);
  spell_level(SPELL_HEROISM, CLASS_WIZARD, 5);
  spell_level(SPELL_VAMPIRIC_TOUCH, CLASS_WIZARD, 5);
  spell_level(SPELL_HALT_UNDEAD, CLASS_WIZARD, 5);
  spell_level(SPELL_STINKING_CLOUD, CLASS_WIZARD, 5);
  spell_level(SPELL_PHANTOM_STEED, CLASS_WIZARD, 5);
  spell_level(SPELL_SUMMON_CREATURE_3, CLASS_WIZARD, 5);
  spell_level(SPELL_DISPEL_MAGIC, CLASS_WIZARD, 5);
  spell_level(SPELL_HASTE, CLASS_WIZARD, 5);
  spell_level(SPELL_SLOW, CLASS_WIZARD, 5);
  spell_level(SPELL_CIRCLE_A_EVIL, CLASS_WIZARD, 5);
  spell_level(SPELL_CIRCLE_A_GOOD, CLASS_WIZARD, 5);
  spell_level(SPELL_CUNNING, CLASS_WIZARD, 5);
  spell_level(SPELL_WISDOM, CLASS_WIZARD, 5);
  spell_level(SPELL_CHARISMA, CLASS_WIZARD, 5);

  //4th circle
  spell_level(SPELL_FIRE_SHIELD, CLASS_WIZARD, 7);
  spell_level(SPELL_COLD_SHIELD, CLASS_WIZARD, 7);
  spell_level(SPELL_ICE_STORM, CLASS_WIZARD, 7);
  spell_level(SPELL_BILLOWING_CLOUD, CLASS_WIZARD, 7);
  spell_level(SPELL_SUMMON_CREATURE_4, CLASS_WIZARD, 7);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_WIZARD, 7);  //shared
  spell_level(SPELL_CURSE, CLASS_WIZARD, 7);  //shared
  spell_level(SPELL_INFRAVISION, CLASS_WIZARD, 7);  //shared
  spell_level(SPELL_POISON, CLASS_WIZARD, 7);  //shared
  spell_level(SPELL_GREATER_INVIS, CLASS_WIZARD, 7);
  spell_level(SPELL_RAINBOW_PATTERN, CLASS_WIZARD, 7);
  spell_level(SPELL_WIZARD_EYE, CLASS_WIZARD, 7);
  spell_level(SPELL_LOCATE_CREATURE, CLASS_WIZARD, 7);
  spell_level(SPELL_MINOR_GLOBE, CLASS_WIZARD, 7);
  spell_level(SPELL_REMOVE_CURSE, CLASS_WIZARD, 7);
  spell_level(SPELL_STONESKIN, CLASS_WIZARD, 7);
  spell_level(SPELL_ENLARGE_PERSON, CLASS_WIZARD, 7);
  spell_level(SPELL_SHRINK_PERSON, CLASS_WIZARD, 7);

  //5th circle
  spell_level(SPELL_INTERPOSING_HAND, CLASS_WIZARD, 9);
  spell_level(SPELL_WALL_OF_FORCE, CLASS_WIZARD, 9);
  spell_level(SPELL_BALL_OF_LIGHTNING, CLASS_WIZARD, 9);
  spell_level(SPELL_CLOUDKILL, CLASS_WIZARD, 9);
  spell_level(SPELL_SUMMON_CREATURE_5, CLASS_WIZARD, 9);
  spell_level(SPELL_WAVES_OF_FATIGUE, CLASS_WIZARD, 9);
  spell_level(SPELL_SYMBOL_OF_PAIN, CLASS_WIZARD, 9);
  spell_level(SPELL_DOMINATE_PERSON, CLASS_WIZARD, 9);
  spell_level(SPELL_FEEBLEMIND, CLASS_WIZARD, 9);  
  spell_level(SPELL_NIGHTMARE, CLASS_WIZARD, 9);
  spell_level(SPELL_MIND_FOG, CLASS_WIZARD, 9);
  spell_level(SPELL_ACID_SHEATH, CLASS_WIZARD, 9);
  spell_level(SPELL_FAITHFUL_HOUND, CLASS_WIZARD, 9);
  spell_level(SPELL_DISMISSAL, CLASS_WIZARD, 9);
  spell_level(SPELL_CONE_OF_COLD, CLASS_WIZARD, 9);
  spell_level(SPELL_TELEKINESIS, CLASS_WIZARD, 9);
  spell_level(SPELL_FIREBRAND, CLASS_WIZARD, 9);

  //6th circle
  spell_level(SPELL_FREEZING_SPHERE, CLASS_WIZARD, 11);
  spell_level(SPELL_ACID_FOG, CLASS_WIZARD, 11);
  spell_level(SPELL_SUMMON_CREATURE_6, CLASS_WIZARD, 11);
  spell_level(SPELL_TRANSFORMATION, CLASS_WIZARD, 11);
  spell_level(SPELL_EYEBITE, CLASS_WIZARD, 11);
  spell_level(SPELL_MASS_HASTE, CLASS_WIZARD, 11);
  spell_level(SPELL_GREATER_HEROISM, CLASS_WIZARD, 11);
  spell_level(SPELL_ANTI_MAGIC_FIELD, CLASS_WIZARD, 11);
  spell_level(SPELL_GREATER_MIRROR_IMAGE, CLASS_WIZARD, 11);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_WIZARD, 11);
  spell_level(SPELL_TRUE_SEEING, CLASS_WIZARD, 11);
  spell_level(SPELL_GLOBE_OF_INVULN, CLASS_WIZARD, 11);
  spell_level(SPELL_GREATER_DISPELLING, CLASS_WIZARD, 11);
  spell_level(SPELL_CLONE, CLASS_WIZARD, 11);
  spell_level(SPELL_WATERWALK, CLASS_WIZARD, 11);

  //7th circle
  spell_level(SPELL_DETECT_POISON, CLASS_WIZARD, 13);  //shared
  spell_level(SPELL_TELEPORT, CLASS_WIZARD, 13);
  spell_level(SPELL_MISSILE_STORM, CLASS_WIZARD, 13);

  //8th circle
  spell_level(SPELL_ENERGY_DRAIN, CLASS_WIZARD, 15);  //shared
  spell_level(SPELL_CHAIN_LIGHTNING, CLASS_WIZARD, 15);
  spell_level(SPELL_PORTAL, CLASS_WIZARD, 15);

  //9th circle
  spell_level(SPELL_METEOR_SWARM, CLASS_WIZARD, 17);
  spell_level(SPELL_POLYMORPH, CLASS_WIZARD, 17);

  //epic wizard
  spell_level(SPELL_MUMMY_DUST, CLASS_WIZARD, 20);  //shared
  spell_level(SPELL_DRAGON_KNIGHT, CLASS_WIZARD, 20);  //shared
  spell_level(SPELL_GREATER_RUIN, CLASS_WIZARD, 20);  //shared
  spell_level(SPELL_HELLBALL, CLASS_WIZARD, 20);  //shared
  spell_level(SPELL_EPIC_MAGE_ARMOR, CLASS_WIZARD, 20);
  spell_level(SPELL_EPIC_WARDING, CLASS_WIZARD, 20);
  //end magic-user spells

  
  // sorcerer, increment spells by spell-level
  //1st circle
  spell_level(SPELL_MAGIC_MISSILE, CLASS_SORCERER, 1);
  spell_level(SPELL_HORIZIKAULS_BOOM, CLASS_SORCERER, 1);
  spell_level(SPELL_BURNING_HANDS, CLASS_SORCERER, 1);
  spell_level(SPELL_ICE_DAGGER, CLASS_SORCERER, 1);
  spell_level(SPELL_MAGE_ARMOR, CLASS_SORCERER, 1);
  spell_level(SPELL_SUMMON_CREATURE_1, CLASS_SORCERER, 1);
  spell_level(SPELL_CHILL_TOUCH, CLASS_SORCERER, 1);
  spell_level(SPELL_NEGATIVE_ENERGY_RAY, CLASS_SORCERER, 1);
  spell_level(SPELL_RAY_OF_ENFEEBLEMENT, CLASS_SORCERER, 1);
  spell_level(SPELL_CHARM, CLASS_SORCERER, 1);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_SORCERER, 1);
  spell_level(SPELL_SLEEP, CLASS_SORCERER, 1);
  spell_level(SPELL_COLOR_SPRAY, CLASS_SORCERER, 1);
  spell_level(SPELL_SCARE, CLASS_SORCERER, 1);
  spell_level(SPELL_TRUE_STRIKE, CLASS_SORCERER, 1);
  spell_level(SPELL_IDENTIFY, CLASS_SORCERER, 1);
  spell_level(SPELL_SHELGARNS_BLADE, CLASS_SORCERER, 1);
  spell_level(SPELL_GREASE, CLASS_SORCERER, 1);
  spell_level(SPELL_ENDURE_ELEMENTS, CLASS_SORCERER, 1);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_SORCERER, 1);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_SORCERER, 1);
  spell_level(SPELL_EXPEDITIOUS_RETREAT, CLASS_SORCERER, 1);
  spell_level(SPELL_IRON_GUTS, CLASS_SORCERER, 1);  
  spell_level(SPELL_SHIELD, CLASS_SORCERER, 1);
  
  //2nd circle
  spell_level(SPELL_SHOCKING_GRASP, CLASS_SORCERER, 4);
  spell_level(SPELL_SCORCHING_RAY, CLASS_SORCERER, 4);
  spell_level(SPELL_CONTINUAL_FLAME, CLASS_SORCERER, 4);
  spell_level(SPELL_SUMMON_CREATURE_2, CLASS_SORCERER, 4);
  spell_level(SPELL_WEB, CLASS_SORCERER, 4);
  spell_level(SPELL_ACID_ARROW, CLASS_SORCERER, 4);
  spell_level(SPELL_BLINDNESS, CLASS_SORCERER, 4);
  spell_level(SPELL_DEAFNESS, CLASS_SORCERER, 4);
  spell_level(SPELL_FALSE_LIFE, CLASS_SORCERER, 4);  
  spell_level(SPELL_DAZE_MONSTER, CLASS_SORCERER, 4);  
  spell_level(SPELL_HIDEOUS_LAUGHTER, CLASS_SORCERER, 4);  
  spell_level(SPELL_TOUCH_OF_IDIOCY, CLASS_SORCERER, 4);  
  spell_level(SPELL_BLUR, CLASS_SORCERER, 4);
  spell_level(SPELL_INVISIBLE, CLASS_SORCERER, 4);  
  spell_level(SPELL_MIRROR_IMAGE, CLASS_SORCERER, 4);
  spell_level(SPELL_DETECT_INVIS, CLASS_SORCERER, 4);
  spell_level(SPELL_DETECT_MAGIC, CLASS_SORCERER, 4);
  spell_level(SPELL_DARKNESS, CLASS_SORCERER, 4);
  spell_level(SPELL_RESIST_ENERGY, CLASS_SORCERER, 4);
  spell_level(SPELL_ENERGY_SPHERE, CLASS_SORCERER, 4);
  spell_level(SPELL_ENDURANCE, CLASS_SORCERER, 4);  //shared
  spell_level(SPELL_STRENGTH, CLASS_SORCERER, 4);
  spell_level(SPELL_GRACE, CLASS_SORCERER, 4);  

  //3rd circle
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_SORCERER, 6);
  spell_level(SPELL_FIREBALL, CLASS_SORCERER, 6);
  spell_level(SPELL_WATER_BREATHE, CLASS_SORCERER, 6);
  spell_level(SPELL_NON_DETECTION, CLASS_SORCERER, 6);
  spell_level(SPELL_CLAIRVOYANCE, CLASS_SORCERER, 6);
  spell_level(SPELL_DAYLIGHT, CLASS_SORCERER, 6);
  spell_level(SPELL_INVISIBILITY_SPHERE, CLASS_SORCERER, 6);
  spell_level(SPELL_WALL_OF_FOG, CLASS_SORCERER, 6);
  spell_level(SPELL_DEEP_SLUMBER, CLASS_SORCERER, 6);
  spell_level(SPELL_HOLD_PERSON, CLASS_SORCERER, 6);
  spell_level(SPELL_FLY, CLASS_SORCERER, 6);
  spell_level(SPELL_HEROISM, CLASS_SORCERER, 6);
  spell_level(SPELL_VAMPIRIC_TOUCH, CLASS_SORCERER, 6);
  spell_level(SPELL_HALT_UNDEAD, CLASS_SORCERER, 6);
  spell_level(SPELL_STINKING_CLOUD, CLASS_SORCERER, 6);
  spell_level(SPELL_PHANTOM_STEED, CLASS_SORCERER, 6);
  spell_level(SPELL_SUMMON_CREATURE_3, CLASS_SORCERER, 6);
  spell_level(SPELL_DISPEL_MAGIC, CLASS_SORCERER, 6);
  spell_level(SPELL_HASTE, CLASS_SORCERER, 6);
  spell_level(SPELL_SLOW, CLASS_SORCERER, 6);
  spell_level(SPELL_CIRCLE_A_EVIL, CLASS_SORCERER, 6);
  spell_level(SPELL_CIRCLE_A_GOOD, CLASS_SORCERER, 6);
  spell_level(SPELL_CUNNING, CLASS_SORCERER, 6);
  spell_level(SPELL_WISDOM, CLASS_SORCERER, 6);
  spell_level(SPELL_CHARISMA, CLASS_SORCERER, 6);

  //4th circle
  spell_level(SPELL_FIRE_SHIELD, CLASS_SORCERER, 8);
  spell_level(SPELL_COLD_SHIELD, CLASS_SORCERER, 8);
  spell_level(SPELL_ICE_STORM, CLASS_SORCERER, 8);
  spell_level(SPELL_BILLOWING_CLOUD, CLASS_SORCERER, 8);
  spell_level(SPELL_SUMMON_CREATURE_4, CLASS_SORCERER, 8);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_SORCERER, 8);  //shared
  spell_level(SPELL_CURSE, CLASS_SORCERER, 8);  //shared
  spell_level(SPELL_INFRAVISION, CLASS_SORCERER, 8);  //shared
  spell_level(SPELL_POISON, CLASS_SORCERER, 8);  //shared
  spell_level(SPELL_GREATER_INVIS, CLASS_SORCERER, 8);
  spell_level(SPELL_RAINBOW_PATTERN, CLASS_SORCERER, 8);
  spell_level(SPELL_WIZARD_EYE, CLASS_SORCERER, 8);
  spell_level(SPELL_LOCATE_CREATURE, CLASS_SORCERER, 8);
  spell_level(SPELL_MINOR_GLOBE, CLASS_SORCERER, 8);
  spell_level(SPELL_REMOVE_CURSE, CLASS_SORCERER, 8);
  spell_level(SPELL_STONESKIN, CLASS_SORCERER, 8);
  spell_level(SPELL_ENLARGE_PERSON, CLASS_SORCERER, 8);
  spell_level(SPELL_SHRINK_PERSON, CLASS_SORCERER, 8);

  //5th circle
  spell_level(SPELL_INTERPOSING_HAND, CLASS_SORCERER, 10);
  spell_level(SPELL_WALL_OF_FORCE, CLASS_SORCERER, 10);
  spell_level(SPELL_BALL_OF_LIGHTNING, CLASS_SORCERER, 10);
  spell_level(SPELL_CLOUDKILL, CLASS_SORCERER, 10);
  spell_level(SPELL_SUMMON_CREATURE_5, CLASS_SORCERER, 10);
  spell_level(SPELL_WAVES_OF_FATIGUE, CLASS_SORCERER, 10);
  spell_level(SPELL_SYMBOL_OF_PAIN, CLASS_SORCERER, 10);
  spell_level(SPELL_DOMINATE_PERSON, CLASS_SORCERER, 10);
  spell_level(SPELL_FEEBLEMIND, CLASS_SORCERER, 10);  
  spell_level(SPELL_NIGHTMARE, CLASS_SORCERER, 10);
  spell_level(SPELL_MIND_FOG, CLASS_SORCERER, 10);
  spell_level(SPELL_ACID_SHEATH, CLASS_SORCERER, 10);
  spell_level(SPELL_FAITHFUL_HOUND, CLASS_SORCERER, 10);
  spell_level(SPELL_DISMISSAL, CLASS_SORCERER, 10);
  spell_level(SPELL_CONE_OF_COLD, CLASS_SORCERER, 10);
  spell_level(SPELL_TELEKINESIS, CLASS_SORCERER, 10);
  spell_level(SPELL_FIREBRAND, CLASS_SORCERER, 10);

  //6th circle
  spell_level(SPELL_FREEZING_SPHERE, CLASS_SORCERER, 12);
  spell_level(SPELL_ACID_FOG, CLASS_SORCERER, 12);
  spell_level(SPELL_SUMMON_CREATURE_6, CLASS_SORCERER, 12);
  spell_level(SPELL_TRANSFORMATION, CLASS_SORCERER, 12);
  spell_level(SPELL_EYEBITE, CLASS_SORCERER, 12);
  spell_level(SPELL_MASS_HASTE, CLASS_SORCERER, 12);
  spell_level(SPELL_GREATER_HEROISM, CLASS_SORCERER, 12);
  spell_level(SPELL_ANTI_MAGIC_FIELD, CLASS_SORCERER, 12);
  spell_level(SPELL_GREATER_MIRROR_IMAGE, CLASS_SORCERER, 12);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_SORCERER, 12);
  spell_level(SPELL_TRUE_SEEING, CLASS_SORCERER, 12);
  spell_level(SPELL_GLOBE_OF_INVULN, CLASS_SORCERER, 12);
  spell_level(SPELL_GREATER_DISPELLING, CLASS_SORCERER, 12);
  spell_level(SPELL_CLONE, CLASS_SORCERER, 12);
  spell_level(SPELL_WATERWALK, CLASS_SORCERER, 12);

  //7th circle
  spell_level(SPELL_DETECT_POISON, CLASS_SORCERER, 14);  //shared
  spell_level(SPELL_TELEPORT, CLASS_SORCERER, 14);
  spell_level(SPELL_MISSILE_STORM, CLASS_SORCERER, 14);

  //8th circle
  spell_level(SPELL_ENERGY_DRAIN, CLASS_SORCERER, 16);  //shared
  spell_level(SPELL_CHAIN_LIGHTNING, CLASS_SORCERER, 16);
  spell_level(SPELL_PORTAL, CLASS_SORCERER, 16);

  //9th circle
  spell_level(SPELL_METEOR_SWARM, CLASS_SORCERER, 18);
  spell_level(SPELL_POLYMORPH, CLASS_SORCERER, 18);

  //epic wizard
  spell_level(SPELL_MUMMY_DUST, CLASS_SORCERER, 20);  //shared
  spell_level(SPELL_DRAGON_KNIGHT, CLASS_SORCERER, 20);  //shared
  spell_level(SPELL_GREATER_RUIN, CLASS_SORCERER, 20);  //shared
  spell_level(SPELL_HELLBALL, CLASS_SORCERER, 20);  //shared
  spell_level(SPELL_EPIC_MAGE_ARMOR, CLASS_SORCERER, 20);
  spell_level(SPELL_EPIC_WARDING, CLASS_SORCERER, 20);
  //end sorcerer spells

  
  // paladins
  //1st circle
  spell_level(SPELL_CURE_LIGHT, CLASS_PALADIN, 6);
  spell_level(SPELL_ENDURANCE, CLASS_PALADIN, 6);  //shared
  spell_level(SPELL_ARMOR, CLASS_PALADIN, 6);
  spell_level(SPELL_CAUSE_LIGHT_WOUNDS, CLASS_PALADIN, 6);
  //2nd circle
  spell_level(SPELL_CREATE_FOOD, CLASS_PALADIN, 10);
  spell_level(SPELL_CREATE_WATER, CLASS_PALADIN, 10);
  spell_level(SPELL_DETECT_POISON, CLASS_PALADIN, 10);  //shared
  spell_level(SPELL_CAUSE_MODERATE_WOUNDS, CLASS_PALADIN, 10);
  //3rd circle
  spell_level(SPELL_DETECT_ALIGN, CLASS_PALADIN, 12);
  spell_level(SPELL_CURE_BLIND, CLASS_PALADIN, 12);
  spell_level(SPELL_BLESS, CLASS_PALADIN, 12);
  spell_level(SPELL_CAUSE_SERIOUS_WOUNDS, CLASS_PALADIN, 12);
  //4th circle
  spell_level(SPELL_INFRAVISION, CLASS_PALADIN, 15);  //shared
  spell_level(SPELL_REMOVE_CURSE, CLASS_PALADIN, 15);  //shared
  spell_level(SPELL_CAUSE_CRITICAL_WOUNDS, CLASS_PALADIN, 15);
  spell_level(SPELL_CURE_CRITIC, CLASS_PALADIN, 15);
  
  
  // rangers
  //1st circle
  spell_level(SPELL_CURE_LIGHT, CLASS_RANGER, 6);
  spell_level(SPELL_ENDURANCE, CLASS_RANGER, 6);  //shared
  spell_level(SPELL_ARMOR, CLASS_RANGER, 6);
  spell_level(SPELL_CAUSE_LIGHT_WOUNDS, CLASS_RANGER, 6);
  //2nd circle
  spell_level(SPELL_CREATE_FOOD, CLASS_RANGER, 10);
  spell_level(SPELL_CREATE_WATER, CLASS_RANGER, 10);
  spell_level(SPELL_DETECT_POISON, CLASS_RANGER, 10);  //shared
  spell_level(SPELL_CAUSE_MODERATE_WOUNDS, CLASS_RANGER, 10);
  //3rd circle
  spell_level(SPELL_DETECT_ALIGN, CLASS_RANGER, 12);
  spell_level(SPELL_CURE_BLIND, CLASS_RANGER, 12);
  spell_level(SPELL_BLESS, CLASS_RANGER, 12);
  spell_level(SPELL_CAUSE_SERIOUS_WOUNDS, CLASS_RANGER, 12);
  //4th circle
  spell_level(SPELL_INFRAVISION, CLASS_RANGER, 15);  //shared
  spell_level(SPELL_REMOVE_CURSE, CLASS_RANGER, 15);  //shared
  spell_level(SPELL_CAUSE_CRITICAL_WOUNDS, CLASS_RANGER, 15);
  spell_level(SPELL_CURE_CRITIC, CLASS_RANGER, 15);
  
  
  // clerics
  //1st circle
  spell_level(SPELL_CURE_LIGHT, CLASS_CLERIC, 1);
  spell_level(SPELL_ENDURANCE, CLASS_CLERIC, 1);  //shared
  spell_level(SPELL_ARMOR, CLASS_CLERIC, 1);
  spell_level(SPELL_CAUSE_LIGHT_WOUNDS, CLASS_CLERIC, 1);
  //2nd circle
  spell_level(SPELL_CREATE_FOOD, CLASS_CLERIC, 3);
  spell_level(SPELL_CREATE_WATER, CLASS_CLERIC, 3);
  spell_level(SPELL_DETECT_POISON, CLASS_CLERIC, 3);  //shared
  spell_level(SPELL_CAUSE_MODERATE_WOUNDS, CLASS_CLERIC, 3);
  //3rd circle
  spell_level(SPELL_DETECT_ALIGN, CLASS_CLERIC, 5);
  spell_level(SPELL_CURE_BLIND, CLASS_CLERIC, 5);
  spell_level(SPELL_BLESS, CLASS_CLERIC, 5);
  spell_level(SPELL_CAUSE_SERIOUS_WOUNDS, CLASS_CLERIC, 5);
  //4th circle
  spell_level(SPELL_INFRAVISION, CLASS_CLERIC, 7);  //shared
  spell_level(SPELL_REMOVE_CURSE, CLASS_CLERIC, 7);  //shared
  spell_level(SPELL_CAUSE_CRITICAL_WOUNDS, CLASS_CLERIC, 7);
  spell_level(SPELL_CURE_CRITIC, CLASS_CLERIC, 7);
  //5th circle
  spell_level(SPELL_BLINDNESS, CLASS_CLERIC, 9);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 9);
  spell_level(SPELL_PROT_FROM_GOOD, CLASS_CLERIC, 9);
  spell_level(SPELL_POISON, CLASS_CLERIC, 9);  //shared
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
  spell_level(SPELL_ENERGY_DRAIN, CLASS_CLERIC, 17);  //shared
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

  if (level > (LVL_IMPL+1) || level < 0) {
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
    case CLASS_WIZARD:
    case CLASS_SORCERER:
    case CLASS_PALADIN:
    case CLASS_MONK:
    case CLASS_DRUID:
    case CLASS_RANGER:
    case CLASS_WARRIOR:
    case CLASS_ROGUE:
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
    //advanced races
    case RACE_TROLL:
      exp *= 2;
      break;
    //epic races
    case RACE_CRYSTAL_DWARF:
      exp *= 30;
      break;
    case RACE_TRELUX:
      exp *= 30;
      break;
    default:
      break;
  }


  return exp;
}


/* Default titles system, simplified from stock -zusuk */
const char *titles(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Being";
  if (level == LVL_IMPL)
    return "the Implementor";

  switch (chclass) {

    case CLASS_WIZARD:
    switch (level) {
      case  1:
      case  2:
      case  3:
      case  4: return "";
      case  5: 
      case  6: 
      case  7: 
      case  8: 
      case  9: return "the Reader of Arcane Texts";
      case 10:
      case 11:
      case 12:
      case 13:
      case 14: return "the Ever-Learning";
      case 15:
      case 16:
      case 17:
      case 18:
      case 19: return "the Advanced Student";
      case 20:
      case 21:
      case 22:
      case 23:
      case 24: return "the Channel of Power";
      case 25:
      case 26:
      case 27:
      case 28:
      case 29: return "the Delver of Mysteries";
      case 30: return "the Knower of Hidden Things";
      case LVL_IMMORT: return "the Immortal Warlock";
      case LVL_GOD: return "the Avatar of Magic";
      case LVL_GRGOD: return "the God of Magic";
      default: return "the Wizard";
    }
    break;

    
    case CLASS_RANGER:
    case CLASS_DRUID:
    switch (level) {
      case  1:
      case  2:
      case  3:
      case  4: return "";
      case  5: 
      case  6: 
      case  7: 
      case  8: 
      case  9: return "the Walker on Loam";
      case 10:
      case 11:
      case 12:
      case 13:
      case 14: return "the Speaker for Beasts";
      case 15:
      case 16:
      case 17:
      case 18:
      case 19: return "the Watcher from Shade";
      case 20:
      case 21:
      case 22:
      case 23:
      case 24: return "the Whispering Winds";
      case 25:
      case 26:
      case 27:
      case 28:
      case 29: return "the Balancer";
      case 30: return "the Still Waters";
      case LVL_IMMORT: return "the Avatar of Nature";
      case LVL_GOD: return "the Wrath of Nature";
      case LVL_GRGOD: return "the Storm of Earth's Voice";
      default: return "the Druid";
    }
    break;

    
    case CLASS_SORCERER:
    switch (level) {
      case  1:
      case  2:
      case  3:
      case  4: return "";
      case  5: 
      case  6: 
      case  7: 
      case  8: 
      case  9: return "the Awakened";
      case 10:
      case 11:
      case 12:
      case 13:
      case 14: return "the Torch";
      case 15:
      case 16:
      case 17:
      case 18:
      case 19: return "the Firebrand";
      case 20:
      case 21:
      case 22:
      case 23:
      case 24: return "the Destroyer";
      case 25:
      case 26:
      case 27:
      case 28:
      case 29: return "the Crux of Power";
      case 30: return "the Near-Divine";
      case LVL_IMMORT: return "the Immortal Magic Weaver";
      case LVL_GOD: return "the Avatar of the Flow";
      case LVL_GRGOD: return "the Hand of Mystical Might";
      default: return "the Sorcerer";
    }
    break;

    /*
    case CLASS_BARD:
    switch (level) {
      case  1:
      case  2:
      case  3:
      case  4: return "";
      case  5: 
      case  6: 
      case  7: 
      case  8: 
      case  9: return "the Melodious";
      case 10:
      case 11:
      case 12:
      case 13:
      case 14: return "the Hummer of Harmonies";
      case 15:
      case 16:
      case 17:
      case 18:
      case 19: return "Weaver of Song";
      case 20:
      case 21:
      case 22:
      case 23:
      case 24: return "Keeper of Chords";
      case 25:
      case 26:
      case 27:
      case 28:
      case 29: return "the Composer";
      case 30: return "the Maestro";
      case LVL_IMMORT: return "the Immortal Songweaver";
      case LVL_GOD: return "the Master of Sound";
      case LVL_GRGOD: return "the Lord of Dance";
      default: return "the Bard";
    }
    break;
*/
        
    case CLASS_CLERIC:
    switch (level) {
      case  1:
      case  2:
      case  3:
      case  4: return "";
      case  5: 
      case  6: 
      case  7: 
      case  8: 
      case  9: return "the Devotee";
      case 10:
      case 11:
      case 12:
      case 13:
      case 14: return "the Example";
      case 15:
      case 16:
      case 17:
      case 18:
      case 19: return "the Truly Pious";
      case 20:
      case 21:
      case 22:
      case 23:
      case 24: return "the Mighty in Faith";
      case 25:
      case 26:
      case 27:
      case 28:
      case 29: return "the God-Favored";
      case 30: return "the One Who Moves Mountains";
      case LVL_IMMORT: return "the Immortal Cardinal";
      case LVL_GOD: return "the Inquisitor";
      case LVL_GRGOD: return "the God of Good and Evil";
      default: return "the Cleric";
    }
    break;

    case CLASS_PALADIN:
    switch (level) {
      case  1:
      case  2:
      case  3:
      case  4: return "";
      case  5: 
      case  6: 
      case  7: 
      case  8: 
      case  9: return "the Initiated";
      case 10:
      case 11:
      case 12:
      case 13:
      case 14: return "the Accepted";
      case 15:
      case 16:
      case 17:
      case 18:
      case 19: return "the Hand of Mercy";
      case 20:
      case 21:
      case 22:
      case 23:
      case 24: return "the Sword of Justice";
      case 25:
      case 26:
      case 27:
      case 28:
      case 29: return "who Walks in the Light";
      case 30: return "the Defender of the Faith";
      case LVL_IMMORT: return "the Immortal Justicar";
      case LVL_GOD: return "the Immortal Sword of Light";
      case LVL_GRGOD: return "the Immortal Hammer of Justic";
      default: return "the Paladin";
    }
    break;

    case CLASS_MONK:
    switch (level) {
      case  1:
      case  2:
      case  3:
      case  4: return "";
      case  5: 
      case  6: 
      case  7: 
      case  8: 
      case  9: return "of the Crushing Fist";
      case 10:
      case 11:
      case 12:
      case 13:
      case 14: return "of the Stomping Foot";
      case 15:
      case 16:
      case 17:
      case 18:
      case 19: return "of the Directed Motions";
      case 20:
      case 21:
      case 22:
      case 23:
      case 24: return "of the Disciplined Body";
      case 25:
      case 26:
      case 27:
      case 28:
      case 29: return "of the Disciplined Mind";
      case 30: return "of the Mastered Self";
      case LVL_IMMORT: return "the Immortal Monk";
      case LVL_GOD: return "the Inquisitor Monk";
      case LVL_GRGOD: return "the God of the Fist";
      default: return "the Monk";
    }
    break;

    case CLASS_ROGUE:
    switch (level) {
      case  1:
      case  2:
      case  3:
      case  4: return "";
      case  5: 
      case  6: 
      case  7: 
      case  8: 
      case  9: return "the Rover";
      case 10:
      case 11:
      case 12:
      case 13:
      case 14: return "the Multifarious";
      case 15:
      case 16:
      case 17:
      case 18:
      case 19: return "the Illusive";
      case 20:
      case 21:
      case 22:
      case 23:
      case 24: return "the Swindler";
      case 25:
      case 26:
      case 27:
      case 28:
      case 29: return "the Marauder";
      case 30: return "the Volatile";
      case LVL_IMMORT: return "the Immortal Assassin";
      case LVL_GOD: return "the Demi God of Thieves";
      case LVL_GRGOD: return "the God of Thieves and Tradesmen";
      default: return "the Rogue";
    }
    break;

    case CLASS_WARRIOR:
    switch (level) {
      case  1:
      case  2:
      case  3:
      case  4: return "";
      case  5: 
      case  6: 
      case  7: 
      case  8: 
      case  9: return "the Mostly Harmless";
      case 10:
      case 11:
      case 12:
      case 13:
      case 14: return "the Useful in Bar-Fights";
      case 15:
      case 16:
      case 17:
      case 18:
      case 19: return "the Friend to Violence";
      case 20:
      case 21:
      case 22:
      case 23:
      case 24: return "the Strong";
      case 25:
      case 26:
      case 27:
      case 28:
      case 29: return "the Bane of All Enemies";
      case 30: return "the Exceptionally Dangerous";
      case LVL_IMMORT: return "the Immortal Warlord";
      case LVL_GOD: return "the Extirpator";
      case LVL_GRGOD: return "the God of War";
      default: return "the Warrior";
    }
    break;

    case CLASS_BERSERKER:
    switch (level) {
      case  1:
      case  2:
      case  3:
      case  4: return "";
      case  5: 
      case  6: 
      case  7: 
      case  8: 
      case  9: return "the Ripper of Flesh";
      case 10:
      case 11:
      case 12:
      case 13:
      case 14: return "the Shatterer of Bone";
      case 15:
      case 16:
      case 17:
      case 18:
      case 19: return "the Cleaver of Organs";
      case 20:
      case 21:
      case 22:
      case 23:
      case 24: return "the Wrecker of Hope";
      case 25:
      case 26:
      case 27:
      case 28:
      case 29: return "the Effulgence of Rage";
      case 30: return "the Foe-Hewer";
      case LVL_IMMORT: return "the Immortal Warlord";
      case LVL_GOD: return "the Extirpator";
      case LVL_GRGOD: return "the God of Rage";
      default: return "the Berserker";
    }
    break;

  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}

