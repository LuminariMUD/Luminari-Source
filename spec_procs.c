/**************************************************************************
*  File: spec_procs.c                                      Part of tbaMUD *
*  Usage: Implementation of special procedures for mobiles/objects/rooms. *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "modify.h"
#include "house.h"
#include "clan.h"

/* locally defined functions of local (file) scope */
static int compare_spells(const void *x, const void *y);
static void npc_steal(struct char_data *ch, struct char_data *victim);

/* Special procedures for mobiles. */
static int spell_sort_info[MAX_SKILLS + 1];


static int compare_spells(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;

  return strcmp(spell_info[a].name, spell_info[b].name);
}


void sort_spells(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a <= MAX_SKILLS; a++)
    spell_sort_info[a] = a;

  qsort(&spell_sort_info[1], MAX_SKILLS, sizeof(int), compare_spells);
}


const char *prac_types[] = {
  "spell",
  "skill"
};
#define LEARNED_LEVEL	0	/* % known which is considered "learned" */
#define MAX_PER_PRAC	1	/* max percent gain in skill per practice */
#define MIN_PER_PRAC	2	/* min percent gain in skill per practice */
#define PRAC_TYPE	3	/* should it say 'spell' or 'skill'?	 */
#define LEARNED(ch) (prac_params[LEARNED_LEVEL][GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][GET_CLASS(ch)]])


//returns true if you have all the requisites for the skill
//false if you don't
int meet_skill_reqs(struct char_data *ch, int skillnum)
{
  //doesn't apply to staff
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return TRUE;
  //spells should return true
  if (skillnum < NUM_SPELLS && skillnum > 0)
    return TRUE;

  /* i'm -trying- to keep this organized */
  switch(skillnum) {
    /* proficiencies */
case SKILL_PROF_BASIC:
	if (GET_SKILL(ch, SKILL_PROF_MINIMAL))
		return TRUE;	else return FALSE;
case SKILL_PROF_ADVANCED:
	if (GET_SKILL(ch, SKILL_PROF_BASIC))
		return TRUE;	else return FALSE;
case SKILL_PROF_MASTER:
	if (GET_SKILL(ch, SKILL_PROF_ADVANCED))
		return TRUE;	else return FALSE;
case SKILL_PROF_EXOTIC:
	if (GET_SKILL(ch, SKILL_PROF_MASTER))
		return TRUE;	else return FALSE;
case SKILL_PROF_MEDIUM_A:
	if (GET_SKILL(ch, SKILL_PROF_LIGHT_A))
		return TRUE;	else return FALSE;
case SKILL_PROF_HEAVY_A:
	if (GET_SKILL(ch, SKILL_PROF_MEDIUM_A))
		return TRUE;	else return FALSE;
case SKILL_PROF_T_SHIELDS:
	if (GET_SKILL(ch, SKILL_PROF_SHIELDS))
		return TRUE;	else return FALSE;

     /* epic spells */
case SKILL_MUMMY_DUST:
	if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 23 && GET_LEVEL(ch) >= 20)
		return TRUE;	else return FALSE;
case SKILL_DRAGON_KNIGHT:
	if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 25 && GET_LEVEL(ch) >= 20 &&
             (CLASS_LEVEL(ch, CLASS_WIZARD) > 17 ||
             CLASS_LEVEL(ch, CLASS_SORCERER) > 19)
             )
		return TRUE;	else return FALSE;
case SKILL_GREATER_RUIN:
	if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 27 && GET_LEVEL(ch) >= 20)
		return TRUE;	else return FALSE;
case SKILL_HELLBALL:
	if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 29 && GET_LEVEL(ch) >= 20 &&
             (CLASS_LEVEL(ch, CLASS_WIZARD) > 16 ||
             CLASS_LEVEL(ch, CLASS_SORCERER) > 18)
             )
		return TRUE;	else return FALSE;
     /* magical based epic spells (not accessable by divine) */
case SKILL_EPIC_MAGE_ARMOR:
	if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 31 && GET_LEVEL(ch) >= 20
		&& (CLASS_LEVEL(ch, CLASS_WIZARD) > 13 ||
              CLASS_LEVEL(ch, CLASS_SORCERER) > 13) )
		return TRUE;	else return FALSE;
case SKILL_EPIC_WARDING:
	if (GET_ABILITY(ch, ABILITY_SPELLCRAFT) >= 33 && GET_LEVEL(ch) >= 20
		&& (CLASS_LEVEL(ch, CLASS_WIZARD) > 15 ||
              CLASS_LEVEL(ch, CLASS_SORCERER) > 15))
		return TRUE;	else return FALSE;
     
     /* 'epic' skills */
case SKILL_BLINDING_SPEED:
	if (ch->real_abils.dex >= 21 && GET_LEVEL(ch) >= 20)
		return TRUE;	else return FALSE;
case SKILL_EPIC_TOUGHNESS:
        if (GET_LEVEL(ch) >= 20)
		return TRUE;	else return FALSE;
case SKILL_EPIC_PROWESS:
	if (GET_LEVEL(ch) >= 20 && GET_SKILL(ch, SKILL_PROWESS))
		return TRUE;	else return FALSE;
case SKILL_SPELLPENETRATE_3:
	if (GET_LEVEL(ch) >= 20 && GET_SKILL(ch, SKILL_SPELLPENETRATE_2))
		return TRUE;	else return FALSE;
case SKILL_SPELL_RESIST_4:
	if (GET_LEVEL(ch) >= 20 && GET_SKILL(ch, SKILL_SPELL_RESIST_3))
		return TRUE;	else return FALSE;
case SKILL_SPELL_RESIST_5:
	if (GET_LEVEL(ch) >= 25 && GET_SKILL(ch, SKILL_SPELL_RESIST_4))
		return TRUE;	else return FALSE;
case SKILL_IMPROVED_BASH:
	if (GET_SKILL(ch, SKILL_BASH) && GET_LEVEL(ch) >= 20)
		return TRUE;	else return FALSE;
case SKILL_IMPROVED_WHIRL:
	if (GET_SKILL(ch, SKILL_WHIRLWIND) && GET_LEVEL(ch) >= 20)
		return TRUE;	else return FALSE;
case SKILL_ARMOR_SKIN:
	if (GET_LEVEL(ch) >= 20)
		return TRUE;	else return FALSE;
case SKILL_SELF_CONCEAL_3:
	if (ch->real_abils.dex >= 21 && GET_SKILL(ch, SKILL_SELF_CONCEAL_2))
		return TRUE;	else return FALSE;
case SKILL_OVERWHELMING_CRIT:
	if (GET_LEVEL(ch) >= 20)
		return TRUE;	else return FALSE;
case SKILL_DAMAGE_REDUC_3:
	if (ch->real_abils.con >= 19 && GET_SKILL(ch, SKILL_DAMAGE_REDUC_2))
		return TRUE;	else return FALSE;
case SKILL_EPIC_REFLEXES:
case SKILL_EPIC_FORTITUDE:
case SKILL_EPIC_WILL:
        if (GET_LEVEL(ch) >= 20)
                return TRUE;  else return FALSE;
case SKILL_EPIC_2_WEAPON:
	if (ch->real_abils.dex >= 21 && GET_SKILL(ch, SKILL_TWO_WEAPON_FIGHT))
		return TRUE;	else return FALSE;
     
/* the rest */
case SKILL_AMBIDEXTERITY:
	if (ch->real_abils.dex >= 13)
		return TRUE;	else return FALSE;
case SKILL_TWO_WEAPON_FIGHT:
	if (ch->real_abils.dex >= 17 && GET_SKILL(ch, SKILL_AMBIDEXTERITY))
		return TRUE;	else return FALSE;
case SKILL_FINESSE:
	if (ch->real_abils.dex >= 13)
		return TRUE;	else return FALSE;
case SKILL_POWER_ATTACK:
	if (ch->real_abils.str >= 13)
		return TRUE;	else return FALSE;
case SKILL_EXPERTISE:
	if (ch->real_abils.intel >= 13)
		return TRUE;	else return FALSE;
case SKILL_SPELLPENETRATE:
	if (GET_LEVEL(ch) >= 5 && IS_CASTER(ch))
		return TRUE;	else return FALSE;
case SKILL_SPELLPENETRATE_2:
	if (GET_LEVEL(ch) >= 9 && GET_SKILL(ch, SKILL_SPELLPENETRATE))
		return TRUE;	else return FALSE;
case SKILL_SPELL_RESIST_1:
	if (GET_LEVEL(ch) >= 5)
		return TRUE;	else return FALSE;
case SKILL_SPELL_RESIST_2:
	if (GET_LEVEL(ch) >= 10 && GET_SKILL(ch, SKILL_SPELL_RESIST_1))
		return TRUE;	else return FALSE;
case SKILL_SPELL_RESIST_3:
	if (GET_LEVEL(ch) >= 15 && GET_SKILL(ch, SKILL_SPELL_RESIST_2))
		return TRUE;	else return FALSE;
case SKILL_INITIATIVE:
	if (ch->real_abils.dex >= 13)
		return TRUE;	else return FALSE;
case SKILL_IMPROVED_TRIP:
	if (GET_SKILL(ch, SKILL_TRIP))
		return TRUE;	else return FALSE;
case SKILL_BASH:
	if (ch->real_abils.str >= 13)
		return TRUE;	else return FALSE;
case SKILL_TRIP:
	if (ch->real_abils.dex >= 13)
		return TRUE;	else return FALSE;
case SKILL_WHIRLWIND:
	if (GET_SKILL(ch, SKILL_SPRING_ATTACK))
		return TRUE;	else return FALSE;
case SKILL_DODGE:
	if (ch->real_abils.dex >= 13)
		return TRUE;	else return FALSE;
case SKILL_DAMAGE_REDUC_1:
	if (ch->real_abils.con >= 15)
		return TRUE;	else return FALSE;
case SKILL_DAMAGE_REDUC_2:
	if (ch->real_abils.con >= 17 && GET_SKILL(ch, SKILL_DAMAGE_REDUC_1))
		return TRUE;	else return FALSE;
case SKILL_SELF_CONCEAL_1:
	if (ch->real_abils.dex >= 15)
		return TRUE;	else return FALSE;
case SKILL_SELF_CONCEAL_2:
	if (ch->real_abils.dex >= 17 && GET_SKILL(ch, SKILL_SELF_CONCEAL_1))
		return TRUE;	else return FALSE;
case SKILL_EPIC_CRIT:
	if (GET_LEVEL(ch) >= 10 && GET_SKILL(ch, SKILL_IMPROVED_CRITICAL))
		return TRUE;	else return FALSE;
case SKILL_QUICK_CHANT:
	if (CASTER_LEVEL(ch))
		return TRUE;	else return FALSE;
case SKILL_SCRIBE:
	if (CASTER_LEVEL(ch))
		return TRUE;	else return FALSE;

/* special restrictions */
case SKILL_USE_MAGIC:  /* shared - with casters and rogue */
        if ((CLASS_LEVEL(ch, CLASS_ROGUE) >= 9) ||
            (IS_CASTER(ch) && GET_LEVEL(ch) >= 2))
          return TRUE;  else return FALSE;
case SKILL_CALL_FAMILIAR:  //sorc, wiz only
	if (CLASS_LEVEL(ch, CLASS_SORCERER) || CLASS_LEVEL(ch, CLASS_WIZARD))
		return TRUE;	else return FALSE;     
case SKILL_RECHARGE:  //casters only
	if (CASTER_LEVEL(ch) >= 14)
		return TRUE;	else return FALSE;     
case SKILL_TRACK:  // rogue / ranger / x-stats only
        if (CLASS_LEVEL(ch, CLASS_ROGUE) || CLASS_LEVEL(ch, CLASS_RANGER) ||
                (GET_WIS(ch) + GET_INT(ch) >= 28))
                return TRUE;  else return FALSE;
     

/* ranger */
case SKILL_FAVORED_ENEMY:
        if (CLASS_LEVEL(ch, CLASS_RANGER))
                return TRUE;	else return FALSE;
case SKILL_DUAL_WEAPONS:
        if (CLASS_LEVEL(ch, CLASS_RANGER) >= 2)
                return TRUE;	else return FALSE;
case SKILL_NATURE_STEP:  //shared with druid
        if (CLASS_LEVEL(ch, CLASS_RANGER) >= 3 ||
            CLASS_LEVEL(ch, CLASS_DRUID) >= 6)
                return TRUE;	else return FALSE;
case SKILL_ANIMAL_COMPANION:  //shared with druid
        if (CLASS_LEVEL(ch, CLASS_RANGER) >= 4 ||
            CLASS_LEVEL(ch, CLASS_DRUID))
                return TRUE;	else return FALSE;

/* druid */
        // animal companion - level 1 (shared with ranger)
        // nature step - level 6 (shared with ranger)

/* warrior */
case SKILL_WEAPON_SPECIALIST:
        if (CLASS_LEVEL(ch, CLASS_WARRIOR) >= 4)
                return TRUE;	else return FALSE;
case SKILL_SHIELD_SPECIALIST:
        if (CLASS_LEVEL(ch, CLASS_WARRIOR) >= 6)
                return TRUE;	else return FALSE;
        
/* monk */
case SKILL_STUNNING_FIST:
        if (CLASS_LEVEL(ch, CLASS_MONK) >= 2)
                return TRUE;  else return FALSE;
        
/* bard */
case SKILL_PERFORM:
        if (CLASS_LEVEL(ch, CLASS_BARD) >= 2)
                return TRUE;  else return FALSE;
        
/* paladin */        
case SKILL_LAY_ON_HANDS:
        if (CLASS_LEVEL(ch, CLASS_PALADIN))
                return TRUE;  else return FALSE;
case SKILL_GRACE:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 2)
                return TRUE;  else return FALSE;
case SKILL_DIVINE_HEALTH:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 3)
                return TRUE;  else return FALSE;
case SKILL_COURAGE:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 4)
                return TRUE;  else return FALSE;
case SKILL_SMITE:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 5)
                return TRUE;  else return FALSE;
case SKILL_REMOVE_DISEASE:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 7)
                return TRUE;  else return FALSE;
case SKILL_PALADIN_MOUNT:
        if (CLASS_LEVEL(ch, CLASS_PALADIN) >= 8)
                return TRUE;  else return FALSE;

/* rogue */
case SKILL_BACKSTAB:
        if (CLASS_LEVEL(ch, CLASS_ROGUE))
                return TRUE;  else return FALSE;
case SKILL_DIRTY_FIGHTING:
        if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 4)
                return TRUE;  else return FALSE;
case SKILL_MOBILITY:  /* shared */
        if (GET_SKILL(ch, SKILL_DODGE) || (CLASS_LEVEL(ch, CLASS_ROGUE) >= 2))
                return TRUE;  else return FALSE;
case SKILL_SPRING_ATTACK:  /* shared */
        if (GET_SKILL(ch, SKILL_MOBILITY) ||
                (CLASS_LEVEL(ch, CLASS_ROGUE) >= 6))
          return TRUE;  else return FALSE;
case SKILL_EVASION:
        if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 8)
                return TRUE;  else return FALSE;
case SKILL_CRIP_STRIKE:
        if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 12)
                return TRUE;  else return FALSE;
case SKILL_SLIPPERY_MIND:
        if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 15)
                return TRUE;  else return FALSE;
case SKILL_DEFENSE_ROLL:
        if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 18)
                return TRUE;  else return FALSE;
case SKILL_IMP_EVASION:
        if (CLASS_LEVEL(ch, CLASS_ROGUE) >= 21)
                return TRUE;  else return FALSE;
        
/* berserker */
case SKILL_RAGE:
        if (CLASS_LEVEL(ch, CLASS_BERSERKER) >= 2)
                return TRUE;  else return FALSE;
                
  /*** no reqs ***/
    case SKILL_RESCUE:
    case SKILL_LUCK_OF_HEROES:
    case SKILL_TOUGHNESS:
    case SKILL_KICK:
    case SKILL_IMPROVED_CRITICAL:
    case SKILL_PROWESS:
    case SKILL_PROF_MINIMAL:
    case SKILL_PROF_SHIELDS:
    case SKILL_PROF_LIGHT_A:
    case SKILL_IRON_WILL:
    case SKILL_GREAT_FORTITUDE:
    case SKILL_LIGHTNING_REFLEXES:
    case SKILL_STEALTHY:
    case SKILL_MINING:
    case SKILL_HUNTING:
    case SKILL_FORESTING:
    case SKILL_KNITTING:
    case SKILL_CHEMISTRY:
    case SKILL_ARMOR_SMITHING:
    case SKILL_WEAPON_SMITHING:
    case SKILL_JEWELRY_MAKING:
    case SKILL_LEATHER_WORKING:
    case SKILL_FAST_CRAFTER:
      return TRUE;
      
      /**
       *  not implemented yet or
       * unattainable
       *  **/
    case SKILL_MURMUR:
    case SKILL_PROPAGANDA:
    case SKILL_LOBBY:
    case SKILL_BONE_ARMOR:
    case SKILL_ELVEN_CRAFTING:
    case SKILL_MASTERWORK_CRAFTING:
    case SKILL_DRACONIC_CRAFTING:
    case SKILL_DWARVEN_CRAFTING:    
    case SKILL_SPELLBATTLE:    
    default: return FALSE;
  }
  return FALSE;
}

/* completely re-written for Luminari, probably needs to be rewritten again :P
   this is the engine for the 'spells' and 'spelllist' commands
   class - you can send -1 for a 'default' class
   mode = 0:  known spells
   mode = anything else: full spelllist for given class 
 */
void list_spells(struct char_data *ch, int mode, int class)
{
  int i = 0, slot = 0, sinfo = 0;
  size_t len = 0, nlen = 0;
  char buf2[MAX_STRING_LENGTH] = { '\0' };
  const char *overflow = "\r\n**OVERFLOW**\r\n";

  //default class case
  if (class == -1) {
    class = GET_CLASS(ch);
    if (!CLASS_LEVEL(ch, class))
      send_to_char(ch, "You don't have any levels in your current class.\r\n");
  }

  if (mode == 0) {
    len = snprintf(buf2, sizeof(buf2), "\tCKnown Spell List\tn\r\n");

    for (slot = getCircle(ch, class); slot > 0; slot--) {
      nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                "\r\n\tCSpell Circle Level %d\tn\r\n", slot);
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;

      for (i = 1; i < NUM_SPELLS; i++) {
        sinfo = spell_info[i].min_level[class];

        if (class == CLASS_SORCERER && sorcKnown(ch, i, CLASS_SORCERER) &&
              spellCircle(CLASS_SORCERER, i) == slot) {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                    "%-20s \tWMastered\tn\r\n", spell_info[i].name);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
        else if (class == CLASS_BARD && sorcKnown(ch, i, CLASS_BARD) &&
              spellCircle(CLASS_BARD, i) == slot) {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                    "%-20s \tWMastered\tn\r\n", spell_info[i].name);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
        else if (class == CLASS_WIZARD && spellbook_ok(ch, i, class, FALSE) &&
             CLASS_LEVEL(ch, class) >= sinfo && spellCircle(class,i) == slot &&
             GET_SKILL(ch, i)) {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                    "%-20s \tRReady\tn\r\n", spell_info[i].name);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;          
        }
        else if (class != CLASS_SORCERER && class != CLASS_BARD && class != CLASS_WIZARD &&
             CLASS_LEVEL(ch, class) >= sinfo && spellCircle(class,i) == slot &&
             GET_SKILL(ch, i)) {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                    "%-20s \tWMastered\tn\r\n", spell_info[i].name);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
      }
    }
  
  } else {
    len = snprintf(buf2, sizeof(buf2), "\tCFull Spell List\tn\r\n");

    if (class == CLASS_PALADIN || class == CLASS_RANGER)
      slot = 4;
    else
      slot = 9;

    for (; slot > 0; slot--) {
      nlen = snprintf(buf2 + len, sizeof(buf2) - len,
               "\r\n\tCSpell Circle Level %d\tn\r\n", slot);
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;

      for (i = 1; i < NUM_SPELLS; i++) {
        sinfo = spell_info[i].min_level[class];

        if (spellCircle(class, i) == slot) {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len,
                     "%-20s\r\n", spell_info[i].name);
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
      }
    }  
  }
  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */
  
  page_string(ch->desc, buf2, TRUE);
}



void list_skills(struct char_data *ch)
{
  int i, printed = 0;

  if (IS_NPC(ch))
    return;

  /* Active Skills */
  send_to_char(ch, "\tCActive Skills\tn\r\n\r\n");
  for (i = MAX_SPELLS+1; i < NUM_SKILLS; i++) {
    if (GET_LEVEL(ch) >= spell_info[i].min_level[GET_CLASS(ch)] &&
            spell_info[i].schoolOfMagic == ACTIVE_SKILL) {
      if (meet_skill_reqs(ch, i)) {
        send_to_char(ch, "%-24s", spell_info[i].name);
        if (!GET_SKILL(ch, i))
          send_to_char(ch, "  \tYUnlearned\tn ");
        else if (GET_SKILL(ch, i) >= 99)
          send_to_char(ch, "  \tWMastered \tn ");
        else if (GET_SKILL(ch, i) >= 95)
          send_to_char(ch, "  \twSuperb \tn ");
        else if (GET_SKILL(ch, i) >= 90)
          send_to_char(ch, "  \tMExcellent \tn ");
        else if (GET_SKILL(ch, i) >= 85)
          send_to_char(ch, "  \tmAdvanced \tn ");
        else if (GET_SKILL(ch, i) >= 80)
          send_to_char(ch, "  \tBSkilled \tn ");
        else
          send_to_char(ch, "  \tGLearned  \tn ");
        printed++;
        if (!(printed % 2))
          send_to_char(ch, "\r\n");
      }
    }
  }
  send_to_char(ch, "\r\n\r\n");
  
  /* Passive Skills */
  send_to_char(ch, "\tCPassive Skills\tn\r\n\r\n");
  for (i = MAX_SPELLS+1; i < NUM_SKILLS; i++) {
    if (GET_LEVEL(ch) >= spell_info[i].min_level[GET_CLASS(ch)] &&
            spell_info[i].schoolOfMagic == PASSIVE_SKILL) {
      if (meet_skill_reqs(ch, i)) {
        send_to_char(ch, "%-24s", spell_info[i].name);
        if (!GET_SKILL(ch, i))
          send_to_char(ch, "  \tYUnlearned\tn ");
        else if (GET_SKILL(ch, i) >= 99)
          send_to_char(ch, "  \tWMastered \tn ");
        else if (GET_SKILL(ch, i) >= 95)
          send_to_char(ch, "  \twSuperb \tn ");
        else if (GET_SKILL(ch, i) >= 90)
          send_to_char(ch, "  \tMExcellent \tn ");
        else if (GET_SKILL(ch, i) >= 85)
          send_to_char(ch, "  \tmAdvanced \tn ");
        else if (GET_SKILL(ch, i) >= 80)
          send_to_char(ch, "  \tBSkilled \tn ");
        else
          send_to_char(ch, "  \tGLearned  \tn ");
        printed++;
        if (!(printed % 2))
          send_to_char(ch, "\r\n");
      }
    }
  }
  send_to_char(ch, "\r\n\r\n");
  
  /* Caster Skills */
  send_to_char(ch, "\tCCaster Skills\tn\r\n\r\n");
  for (i = MAX_SPELLS+1; i < NUM_SKILLS; i++) {
    if (GET_LEVEL(ch) >= spell_info[i].min_level[GET_CLASS(ch)] &&
            spell_info[i].schoolOfMagic == CASTER_SKILL) {
      if (meet_skill_reqs(ch, i)) {
        send_to_char(ch, "%-24s", spell_info[i].name);
        if (!GET_SKILL(ch, i))
          send_to_char(ch, "  \tYUnlearned\tn ");
        else if (GET_SKILL(ch, i) >= 99)
          send_to_char(ch, "  \tWMastered \tn ");
        else if (GET_SKILL(ch, i) >= 95)
          send_to_char(ch, "  \twSuperb \tn ");
        else if (GET_SKILL(ch, i) >= 90)
          send_to_char(ch, "  \tMExcellent \tn ");
        else if (GET_SKILL(ch, i) >= 85)
          send_to_char(ch, "  \tmAdvanced \tn ");
        else if (GET_SKILL(ch, i) >= 80)
          send_to_char(ch, "  \tBSkilled \tn ");
        else
          send_to_char(ch, "  \tGLearned  \tn ");
        printed++;
        if (!(printed % 2))
          send_to_char(ch, "\r\n");
      }
    }
  }
  send_to_char(ch, "\r\n\r\n");
  
  /* Crafting Skills */
  send_to_char(ch, "\tCCrafting Skills\tn\r\n\r\n");
  for (i = MAX_SPELLS+1; i < NUM_SKILLS; i++) {
    if (GET_LEVEL(ch) >= spell_info[i].min_level[GET_CLASS(ch)] &&
            spell_info[i].schoolOfMagic == CRAFTING_SKILL) {
      if (meet_skill_reqs(ch, i)) {
        send_to_char(ch, "%-24s", spell_info[i].name);
        if (!GET_SKILL(ch, i))
          send_to_char(ch, "  \tYUnlearned\tn ");
        else if (GET_SKILL(ch, i) >= 99)
          send_to_char(ch, "  \tWMastered \tn ");
        else if (GET_SKILL(ch, i) >= 95)
          send_to_char(ch, "  \twSuperb \tn ");
        else if (GET_SKILL(ch, i) >= 90)
          send_to_char(ch, "  \tMExcellent \tn ");
        else if (GET_SKILL(ch, i) >= 85)
          send_to_char(ch, "  \tmAdvanced \tn ");
        else if (GET_SKILL(ch, i) >= 80)
          send_to_char(ch, "  \tBGood \tn ");
        else if (GET_SKILL(ch, i) >= 70)
          send_to_char(ch, "  \tbFair \tn ");
        else if (GET_SKILL(ch, i) >= 55)
          send_to_char(ch, "  \tnLearned \tn ");
        else if (GET_SKILL(ch, i) >= 40)
          send_to_char(ch, "  \tyAverage \tn ");
        else if (GET_SKILL(ch, i) >= 20)
          send_to_char(ch, "  \tYPoor \tn ");
        else if (GET_SKILL(ch, i) >= 10)
          send_to_char(ch, "  \trBad \tn ");
        else
          send_to_char(ch, "  \tRAwful  \tn ");
        printed++;
        if (!(printed % 2))
          send_to_char(ch, "\r\n");
      }
    }
  }
  send_to_char(ch, "\r\n\r\n");
  
  send_to_char(ch, "\tCPractice Session(s): %d\tn\r\n\r\n",
    GET_PRACTICES(ch));
  
}


int compute_ability(struct char_data *ch, int abilityNum)
{
  int value = 0;

  if (abilityNum < 1 || abilityNum > NUM_ABILITIES)
    return -1;

  //universal bonuses
  if (affected_by_spell(ch, SPELL_HEROISM))
    value += 2;
  else if (affected_by_spell(ch, SPELL_GREATER_HEROISM))
    value += 4;  
  if (affected_by_spell(ch, SKILL_PERFORM))
    value += SONG_AFF_VAL(ch);  

  // try to avoid sending NPC's here, but just in case:
  if (IS_NPC(ch))
    value += GET_LEVEL(ch);
  else
    value += GET_ABILITY(ch, abilityNum);

  switch (abilityNum) {
	case ABILITY_TUMBLE:
		value += GET_DEX_BONUS(ch);
		return value; 
	case ABILITY_HIDE:
		value += GET_DEX_BONUS(ch);
		if (GET_SKILL(ch, SKILL_STEALTHY))
            value += 2;
          if (GET_RACE(ch) == RACE_HALFLING)
            value += 2;
          if (AFF_FLAGGED(ch, AFF_REFUGE))
            value += 15;
          if (IS_MORPHED(ch) && SUBRACE(ch) == PC_SUBRACE_PANTHER)
            value += 4;
		return value; 
	case ABILITY_SNEAK:
		value += GET_DEX_BONUS(ch);
		if (GET_SKILL(ch, SKILL_STEALTHY))
            value += 2;
          if (GET_RACE(ch) == RACE_HALFLING)
            value += 2;
          if (AFF_FLAGGED(ch, AFF_REFUGE))
            value += 15;
          if (IS_MORPHED(ch) && SUBRACE(ch) == PC_SUBRACE_PANTHER)
            value += 4;
		return value; 
	case ABILITY_SPOT:
		value += GET_WIS_BONUS(ch);
          if (GET_RACE(ch) == RACE_ELF)
            value += 2;
		return value; 
	case ABILITY_LISTEN:
		value += GET_WIS_BONUS(ch);
          if (GET_RACE(ch) == RACE_GNOME)
            value += 2;
          if (GET_RACE(ch) == RACE_ELF)
            value += 2;
		return value; 
	case ABILITY_TREAT_INJURY:
		value += GET_WIS_BONUS(ch);
		return value; 
	case ABILITY_TAUNT:
		value += GET_CHA_BONUS(ch);
		return value; 
	case ABILITY_CONCENTRATION:
          if (GET_RACE(ch) == RACE_GNOME)
            value += 2;
		value += GET_CON_BONUS(ch);
          if (!IS_NPC(ch) && GET_RACE(ch) == RACE_ARCANA_GOLEM) {
            value += GET_LEVEL(ch) / 6;
          }
          return value; 
	case ABILITY_SPELLCRAFT:
		value += GET_INT_BONUS(ch);
          if (!IS_NPC(ch) && GET_RACE(ch) == RACE_ARCANA_GOLEM) {
            value += GET_LEVEL(ch) / 6;
          }
		return value; 
	case ABILITY_APPRAISE:
		value += GET_INT_BONUS(ch);
		return value; 
	case ABILITY_DISCIPLINE:
          if (GET_RACE(ch) == RACE_H_ELF)
            value += 2;
		value += GET_STR_BONUS(ch);
		return value; 
	case ABILITY_PARRY:
		value += GET_DEX_BONUS(ch);
		return value; 
	case ABILITY_LORE:
          if (GET_RACE(ch) == RACE_H_ELF)
            value += 2;
		value += GET_INT_BONUS(ch);
		return value; 
	case ABILITY_MOUNT:
		value += GET_DEX_BONUS(ch);
		return value; 
	case ABILITY_RIDING:
		value += GET_DEX_BONUS(ch);
		return value; 
	case ABILITY_TAME:
		value += GET_INT_BONUS(ch);
		return value; 
	case ABILITY_PICK_LOCK:
		value += GET_DEX_BONUS(ch);
		return value; 
	case ABILITY_STEAL:
		value += GET_DEX_BONUS(ch);
		return value; 
    default:  return -1;
  }
}


/** cross-class or not? **/
const char *cross_names[] = {
  "\tRNot Available to Your Class\tn",
  "\tcCross-Class Ability\tn",
  "\tWClass Ability\tn"
};
void list_abilities(struct char_data *ch)
{
  int i;

  if (IS_NPC(ch))
    return;

  send_to_char(ch, "\tCYou have %d training session%s remaining.\r\n"
	"You know of the following abilities:\tn\r\n", GET_TRAINS(ch),
	GET_TRAINS(ch) == 1 ? "" : "s");

  for (i = 1; i < NUM_ABILITIES; i++) {
    send_to_char(ch, "%-20s [%d] \tC[%d]\tn %s\r\n",
	ability_names[i], GET_ABILITY(ch, i), compute_ability(ch, i),
	cross_names[class_ability[i][GET_CLASS(ch)]]);
  }
}


//further expansion -zusuk
void process_skill(struct char_data *ch, int skillnum)
{
  switch (skillnum) {
    case SKILL_EPIC_TOUGHNESS:
      ch->points.max_hit += GET_LEVEL(ch);
      send_to_char(ch, "\tMYou gained %d hp!\tn\r\n", GET_LEVEL(ch));
      return;
    case SKILL_TOUGHNESS:
      ch->points.max_hit += GET_LEVEL(ch);
      send_to_char(ch, "\tMYou gained %d hp!\tn\r\n", GET_LEVEL(ch));
      return;

    // epic spells

    case SKILL_MUMMY_DUST:
      send_to_char(ch, "\tMYou gained Epic Spell:  Mummy Dust!\tn\r\n");
      SET_SKILL(ch, SPELL_MUMMY_DUST, 99);
      return;
    case SKILL_DRAGON_KNIGHT:
      send_to_char(ch, "\tMYou gained Epic Spell:  Dragon Knight!\tn\r\n");
      SET_SKILL(ch, SPELL_DRAGON_KNIGHT, 99);
      return;
    case SKILL_GREATER_RUIN:             
      send_to_char(ch, "\tMYou gained Epic Spell:  Greater Ruin!\tn\r\n");
      SET_SKILL(ch, SPELL_GREATER_RUIN, 99);
      return;
    case SKILL_HELLBALL:                 
      send_to_char(ch, "\tMYou gained Epic Spell:  Hellball!\tn\r\n");
      SET_SKILL(ch, SPELL_HELLBALL, 99);
      return;
    case SKILL_EPIC_MAGE_ARMOR:          
      send_to_char(ch, "\tMYou gained Epic Spell:  Epic Mage Armor!\tn\r\n");
      SET_SKILL(ch, SPELL_EPIC_MAGE_ARMOR, 99);
      return;
    case SKILL_EPIC_WARDING:
      send_to_char(ch, "\tMYou gained Epic Spell:  Epic Warding!\tn\r\n");
      SET_SKILL(ch, SPELL_EPIC_WARDING, 99);
      return;

    default:  return;
  }
  return;
}


SPECIAL(guild)
{
  int skill_num, percent;

  if (IS_NPC(ch) || (!CMD_IS("practice") && !CMD_IS("train") && !CMD_IS("boosts")))
    return (FALSE);

  skip_spaces(&argument);

  // Practice code
  if (CMD_IS("practice")) {
    if (!*argument) {
      list_skills(ch);
      return (TRUE);
    }
    if (GET_PRACTICES(ch) <= 0) {
      send_to_char(ch, "You do not seem to be able to practice now.\r\n");
      return (TRUE);
    }

    skill_num = find_skill_num(argument);

    if (skill_num < 1 ||
        GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)]) {
      send_to_char(ch, "You do not know of that %s.\r\n", SPLSKL(ch));
      return (TRUE);
    }

    if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
      send_to_char(ch, "You are already learned in that area.\r\n");
      return (TRUE);
    }

    if (skill_num > SPELL_RESERVED_DBC && skill_num < MAX_SPELLS) {
      send_to_char(ch, "You can't practice spells.\r\n");
      return (TRUE);
    }

    if (!meet_skill_reqs(ch, skill_num)) {
      send_to_char(ch, "You haven't met the pre-requisites for that skill.\r\n");
      return (TRUE);
    }
    
    /* added with addition of crafting system so you can't use your
     'practice points' for training your crafting skills which have
     a much lower base value than 75 */
    
    if (GET_SKILL(ch, skill_num)) {
      send_to_char(ch, "You already have this skill trained.\r\n");
      return TRUE;
    }
    
    send_to_char(ch, "You practice '%s' with your trainer...\r\n",
	spell_info[skill_num].name);
    GET_PRACTICES(ch)--;

    percent = GET_SKILL(ch, skill_num);
    percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), int_app[GET_INT(ch)].learn));

    SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

    if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
      send_to_char(ch, "You are now \tGlearned\tn in '%s.'\r\n",
		spell_info[skill_num].name);

    //for further expansion - zusuk
    process_skill(ch, skill_num);

    return (TRUE);

  } else if (CMD_IS("train")) {

    //training code

    if (!*argument) {
      list_abilities(ch);
      return (TRUE);
    }

    if (GET_TRAINS(ch) <= 0) {
      send_to_char(ch, "You do not seem to be able to train now.\r\n");
      return (TRUE);
    }

    skill_num = find_ability_num(argument);

    if (skill_num < 1) { 
      send_to_char(ch, "You do not know of that ability.\r\n");
      return (TRUE);
    }

    //ability not available to this class
    if (class_ability[skill_num][GET_CLASS(ch)] == 0) {
      send_to_char(ch, "This ability is not available to your class...\r\n");
      return (TRUE);
    }

    //cross-class ability
    if (GET_TRAINS(ch) < 2 && class_ability[skill_num][GET_CLASS(ch)] == 1) {
      send_to_char(ch, "(Cross-Class) You don't have enough training sessions to train that ability...\r\n");
      return (TRUE);
    }
    if (GET_ABILITY(ch, skill_num) >= ((int) ((GET_LEVEL(ch) + 3) / 2)) && class_ability[skill_num][GET_CLASS(ch)] == 1) {
      send_to_char(ch, "You are already trained in that area.\r\n");
      return (TRUE);
    }

    //class ability
    if (GET_ABILITY(ch, skill_num) >= (GET_LEVEL(ch) + 3) && class_ability[skill_num][GET_CLASS(ch)] == 2) {
      send_to_char(ch, "You are already trained in that area.\r\n");
      return (TRUE);
    }

    send_to_char(ch, "You train for a while...\r\n");
    GET_TRAINS(ch)--;
    if (class_ability[skill_num][GET_CLASS(ch)] == 1) {
      GET_TRAINS(ch)--;
      send_to_char(ch, "You used two training sessions to train a cross-class ability...\r\n");
    }
    GET_ABILITY(ch, skill_num)++;

    if (GET_ABILITY(ch, skill_num) >= (GET_LEVEL(ch) + 3))
      send_to_char(ch, "You are now trained in that area.\r\n");
    if (GET_ABILITY(ch, skill_num) >= ((int) ((GET_LEVEL(ch) + 3) / 2)) && class_ability[skill_num][GET_CLASS(ch)] == 1)
      send_to_char(ch, "You are already trained in that area.\r\n");

    return (TRUE);
  } else if (CMD_IS("boosts")) {
    if (!argument || !*argument)
      send_to_char(ch, "\tCStat boost sessions remaining: %d\tn\r\n"
        "\tcStats:\tn\r\n"
        "Strength\r\n"
        "Constitution\r\n"
        "Dexterity\r\n"
        "Intelligence\r\n"
        "Wisdom\r\n"
        "Charisma\r\n" 
        "\r\n",
        GET_BOOSTS(ch));
    else if (!GET_BOOSTS(ch))
      send_to_char(ch, "You have no ability training sessions.\r\n"); 
    else if (!strncasecmp("strength", argument, strlen(argument))) {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour strength increases!\tn\r\n");
      ch->real_abils.str += 1;
      GET_BOOSTS(ch) -= 1;
    } else if (!strncasecmp("constitution", argument, strlen(argument))) {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour constitution increases!\tn\r\n");
      ch->real_abils.con += 1;
      /* Give them retroactive hit points for constitution */
      if (! (ch->real_abils.con % 2)) {
        GET_MAX_HIT(ch) += GET_LEVEL(ch);
        send_to_char(ch, "\tMYou gain %d hitpoints!\tn\r\n", GET_LEVEL(ch));
      }
      GET_BOOSTS(ch) -= 1;
    } else if (!strncasecmp("dexterity", argument, strlen(argument))) {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour dexterity increases!\tn\r\n");
      ch->real_abils.dex += 1;
      GET_BOOSTS(ch) -= 1; 
    } else if (!strncasecmp("intelligence", argument, strlen(argument))) {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour intelligence increases!\tn\r\n");
      ch->real_abils.intel += 1;
      /* Give extra skill practice, but only for this level */  
      if (! (ch->real_abils.intel % 2))
        GET_TRAINS(ch)++;
      GET_BOOSTS(ch) -= 1;
    } else if (!strncasecmp("wisdom", argument, strlen(argument))) {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour wisdom increases!\tn\r\n");
      ch->real_abils.wis += 1;
      GET_BOOSTS(ch) -= 1;
    } else if (!strncasecmp("charisma", argument, strlen(argument))) {
      send_to_char(ch, CONFIG_OK);
      send_to_char(ch, "\tMYour charisma increases!\tn\r\n");
      ch->real_abils.cha += 1;
      GET_BOOSTS(ch) -= 1;
    } else
      send_to_char(ch, "\tCStat boost sessions remaining: %d\tn\r\n"
        "\tcStats:\tn\r\n"
        "Strength\r\n"
        "Constitution\r\n"
        "Dexterity\r\n"
        "Intelligence\r\n"
        "Wisdom\r\n"
        "Charisma\r\n" 
        "\r\n",
        GET_BOOSTS(ch));
    affect_total(ch);
    send_to_char(ch, "\tDType 'practice' to see your skills\tn\r\n");
    send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
    send_to_char(ch, "\tDType 'boost' to adjust your stats\tn\r\n");
    send_to_char(ch, "\tDType 'spells <classname>' to see your currently known spells\tn\r\n");
    return (TRUE); 
  }

  //should not be able to get here
  log("Reached the unreachable in SPECIAL(guild) in spec_procs.c");
  return (FALSE);

}

SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return (FALSE);

  do_drop(ch, argument, cmd, SCMD_DROP);

  for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value) {
    send_to_char(ch, "You are awarded for outstanding performance.\r\n");
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else
      increase_gold(ch, value);
  }
  return (TRUE);
}

SPECIAL(mayor)
{
  char actbuf[MAX_INPUT_LENGTH];

  const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
  const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static const char *path = NULL;
  static int path_index;
  static bool move = FALSE;

  if (!move) {
    if (time_info.hours == 6) {
      move = TRUE;
      path = open_path;
      path_index = 0;
    } else if (time_info.hours == 20) {
      move = TRUE;
      path = close_path;
      path_index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) || FIGHTING(ch))
    return (FALSE);

  switch (path[path_index]) {
  case '0':
  case '1':
  case '2':
  case '3':
    perform_move(ch, path[path_index] - '0', 1);
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgen closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_UNLOCK);	/* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_OPEN);	/* strcpy: OK */
    break;

  case 'C':
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_CLOSE);	/* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_LOCK);	/* strcpy: OK */
    break;

  case '.':
    move = FALSE;
    break;

  }

  path_index++;
  return (FALSE);
}

/* General special procedures for mobiles. */

static void npc_steal(struct char_data *ch, struct char_data *victim)
{
  int gold;

  if (IS_NPC(victim))
    return;
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    return;
  if (!CAN_SEE(ch, victim))
    return;

  if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
    if (gold > 0) {
      increase_gold(ch, gold);
	  decrease_gold(victim, gold);
    }
  }
}

/* Quite lethal to low-level characters. */
SPECIAL(snake)
{
  if (cmd || !FIGHTING(ch))
    return (FALSE);

  if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || rand_number(0, GET_LEVEL(ch)) != 0)
    return (FALSE);

  act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
  act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
  call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
  return (TRUE);
}


SPECIAL(hound)
{
  struct char_data *i;
  int door;
  room_rnum room;

  if (cmd || GET_POS(ch) != POS_STANDING || FIGHTING(ch))
    return (FALSE);

  /* first go through all the directions */
  for (door = 0; door < DIR_COUNT; door++) {
    if (CAN_GO(ch, door)) {
      room = world[IN_ROOM(ch)].dir_option[door]->to_room;

      /* ok found a neighboring room, now cycle through the peeps */
      for (i = world[room].people; i; i = i->next_in_room) {
        /* is this guy a hostile? */
        if (i && IS_NPC(i) && MOB_FLAGGED(i, MOB_AGGRESSIVE)) {
          act("$n howls a warning!", FALSE, ch, 0, 0, TO_ROOM);
          return (TRUE);
        }
      } // end peeps cycle
    } // can_go
  } // end room cycle

  return (FALSE);
}


SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd || GET_POS(ch) != POS_STANDING)
    return (FALSE);

  for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && GET_LEVEL(cons) < LVL_IMMORT && !rand_number(0, 4)) {
      npc_steal(ch, cons);
      return (TRUE);
    }

  return (FALSE);
}


SPECIAL(wizard)
{
  struct char_data *vict;

  if (cmd || !FIGHTING(ch))
    return (FALSE);

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
    vict = FIGHTING(ch);

  /* Hm...didn't pick anyone...I'll wait a round. */
  if (vict == NULL)
    return (TRUE);

  if (GET_LEVEL(ch) > 13 && rand_number(0, 10) == 0)
    cast_spell(ch, vict, NULL, SPELL_POISON);

  if (GET_LEVEL(ch) > 7 && rand_number(0, 8) == 0)
    cast_spell(ch, vict, NULL, SPELL_BLINDNESS);

  if (GET_LEVEL(ch) > 12 && rand_number(0, 12) == 0) {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
    else if (IS_GOOD(ch))
      cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
  }

  if (rand_number(0, 4))
    return (TRUE);

  switch (GET_LEVEL(ch)) {
    case 4:
    case 5:
      cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
      break;
    case 6:
    case 7:
      cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
      break;
    case 8:
    case 9:
      cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
      break;
    case 10:
    case 11:
      cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
      break;
    case 12:
    case 13:
      cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
      break;
    case 14:
    case 15:
    case 16:
    case 17:
      cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
      break;
    default:
      cast_spell(ch, vict, NULL, SPELL_FIREBALL);
      break;
  }
  return (TRUE);
}


/* Special procedures for mobiles. */
SPECIAL(wall) 
{ 
  if (!IS_MOVE(cmd))    
    return (FALSE); 
     
  /* acceptable ways to avoid the wall */ 
  /* */

  /* failed to get past wall */
  send_to_char(ch, "You can't get by the magical wall!\r\n");
  act( "$n fails to get past the magical wall!", FALSE, ch, 0, 0, TO_ROOM); 
  return (TRUE); 
}


SPECIAL(guild_guard) 
{ 
  int i, direction; 
  struct char_data *guard = (struct char_data *)me; 
  const char *buf = "The guard humiliates you, and blocks your way.\r\n"; 
  const char *buf2 = "The guard humiliates $n, and blocks $s way."; 

  if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND)) 
    return (FALSE); 
     
  if (GET_LEVEL(ch) >= LVL_IMMORT) 
    return (FALSE); 
   
  /* find out what direction they are trying to go */ 
  for (direction = 0; direction < NUM_OF_DIRS; direction++)
    if (!strcmp(cmd_info[cmd].command, dirs[direction]))
      for (direction = 0; direction < DIR_COUNT; direction++)
		if (!strcmp(cmd_info[cmd].command, dirs[direction]) ||
			!strcmp(cmd_info[cmd].command, autoexits[direction]))
	      break; 

  for (i = 0; guild_info[i].guild_room != NOWHERE; i++) { 
    /* Wrong guild. */ 
    if (GET_ROOM_VNUM(IN_ROOM(ch)) != guild_info[i].guild_room) 
      continue; 

    /* Wrong direction. */ 
    if (direction != guild_info[i].direction) 
      continue; 

    /* Allow the people of the guild through. */ 
    if (!IS_NPC(ch) && GET_CLASS(ch) == guild_info[i].pc_class) 
      continue; 

    send_to_char(ch, "%s", buf); 
    act(buf2, FALSE, ch, 0, 0, TO_ROOM); 
    return (TRUE); 
  } 
  return (FALSE); 
} 

SPECIAL(puff)
{
  char actbuf[MAX_INPUT_LENGTH];

  if (cmd)
    return (FALSE);

  switch (rand_number(0, 60)) {
    case 0:
      do_say(ch, strcpy(actbuf, "My god!  It's full of stars!"), 0, 0);	/* strcpy: OK */
      return (TRUE);
    case 1:
      do_say(ch, strcpy(actbuf, "How'd all those fish get up here?"), 0, 0);	/* strcpy: OK */
      return (TRUE);
    case 2:
      do_say(ch, strcpy(actbuf, "I'm a very female dragon."), 0, 0);	/* strcpy: OK */
      return (TRUE);
    case 3:
      do_say(ch, strcpy(actbuf, "I've got a peaceful, easy feeling."), 0, 0);	/* strcpy: OK */
      return (TRUE);
    default:
      return (FALSE);
  }
}

SPECIAL(fido)
{
  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
    if (!IS_CORPSE(i))
      continue;

    act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
    for (temp = i->contains; temp; temp = next_obj) {
      next_obj = temp->next_content;
      obj_from_obj(temp);
      obj_to_room(temp, IN_ROOM(ch));
    }
    extract_obj(i);
    return (TRUE);
  }
  return (FALSE);
}

SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);
  }
  return (FALSE);
}

SPECIAL(cityguard)
{
  struct char_data *tch, *evil, *spittle;
  int max_evil, min_cha;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  max_evil = 1000;
  min_cha = 6;
  spittle = evil = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (!CAN_SEE(ch, tch))
      continue;
    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return (TRUE);
    }

    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      return (TRUE);
    }

    if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil && (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
      max_evil = GET_ALIGNMENT(tch);
      evil = tch;
    }

    if (GET_CHA(tch) < min_cha) {
      spittle = tch;
      min_cha = GET_CHA(tch);
    }
  }

  /*
  if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0) {
    act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
    return (TRUE);
  }
  */

  /* Reward the socially inept. */
  if (spittle && !rand_number(0, 9)) {
    static int spit_social;

    if (!spit_social)
      spit_social = find_command("spit");

    if (spit_social > 0) {
      char spitbuf[MAX_NAME_LENGTH + 1];
      strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf));	/* strncpy: OK */
      spitbuf[sizeof(spitbuf) - 1] = '\0';
      do_action(ch, spitbuf, spit_social, 0);
      return (TRUE);
    }
  }
  return (FALSE);
}

#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)
SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[MEDIUM_STRING];
  room_rnum pet_room;
  struct char_data *pet;

  /* Gross. */
  pet_room = IN_ROOM(ch) + 1;

  if (CMD_IS("list")) {
    send_to_char(ch, "Available pets are:\r\n");
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      /* No, you can't have the Implementor as a pet if he's in there. */
      if (!IS_NPC(pet))
        continue;
      send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    two_arguments(argument, buf, pet_name);

    if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet)) {
      send_to_char(ch, "There is no such pet!\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet)) {
      send_to_char(ch, "You don't have enough gold!\r\n");
      return (TRUE);
    }
    decrease_gold(ch, PET_PRICE(pet));

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

    if (*pet_name) {
      snprintf(buf, sizeof(buf), "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = strdup(buf);

      snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = strdup(buf);
    }
    char_to_room(pet, IN_ROOM(ch));
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char(ch, "May you enjoy your pet.\r\n");
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return (TRUE);
  }

  /* All commands except list and buy */
  return (FALSE);
}

/* Special procedures for objects. */
SPECIAL(bank)
{
  int amount;

  if (CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      send_to_char(ch, "Your current balance is %d coins.\r\n", GET_BANK_GOLD(ch));
    else
      send_to_char(ch, "You currently have no money deposited.\r\n");
    return (TRUE);
  } else if (CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char(ch, "How much do you want to deposit?\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char(ch, "You don't have that many coins!\r\n");
      return (TRUE);
    }
    decrease_gold(ch, amount);
	increase_bank(ch, amount);
    send_to_char(ch, "You deposit %d coins.\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (TRUE);
  } else if (CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char(ch, "How much do you want to withdraw?\r\n");
      return (TRUE);
    }
    if (GET_BANK_GOLD(ch) < amount) {
      send_to_char(ch, "You don't have that many coins deposited!\r\n");
      return (TRUE);
    }
    increase_gold(ch, amount);
	decrease_bank(ch, amount);
    send_to_char(ch, "You withdraw %d coins.\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (TRUE);
  } else
    return (FALSE);
}

SPECIAL(clan_cleric)
{
  int i;
  char buf[MAX_STRING_LENGTH];
  zone_vnum clanhall;
  clan_vnum clan;
  struct char_data *this_mob = (struct char_data *)me;

  struct price_info {
    short int number;
    char name[25];
    short int price;
  } clan_prices[] = {
    /* Spell Num (defined)      Name shown        Price  */
    { SPELL_ARMOR,              "armor             ", 75 },
    { SPELL_BLESS,              "bless            ", 150 },
    { SPELL_REMOVE_POISON,      "remove poison    ", 525 },
    { SPELL_CURE_BLIND,         "cure blindness   ", 375 },
    { SPELL_CURE_CRITIC,        "critic           ", 525 },
    { SPELL_SANCTUARY,          "sanctuary       ", 3000 },
    { SPELL_HEAL,               "heal            ", 3500 },

    /* The next line must be last, add new spells above. */
    { -1, "\r\n", -1 }
  };

  if (CMD_IS("buy")||CMD_IS("list"))
  {
    argument = one_argument(argument, buf);

    /* Which clanhall is this cleric in? */
    clanhall = zone_table[(GET_ROOM_ZONE(IN_ROOM(this_mob)))].number;
    if ((clan = zone_is_clanhall(clanhall)) == NO_CLAN) {
      log("SYSERR: clan_cleric spec (%s) not in a known clanhall (room %d)", GET_NAME(this_mob), world[(IN_ROOM(this_mob))].number);
      return FALSE;
    }
    if (clan != GET_CLAN(ch)) {
      sprintf(buf, "$n will only serve members of %s", CLAN_NAME(real_clan(clan)));
      act(buf, TRUE, this_mob, 0, ch, TO_VICT);
      return TRUE;
    }

    if (FIGHTING(ch)) {
      send_to_char(ch, "You can't do that while fighting!\r\n");
      return TRUE;
    }

    if (*buf) {
      for (i=0; clan_prices[i].number > SPELL_RESERVED_DBC; i++) {
        if (is_abbrev(buf, clan_prices[i].name)) {
          if (GET_GOLD(ch) < clan_prices[i].price) {
            act("$n tells you, 'You don't have enough gold for that spell!'",
                FALSE, this_mob, 0, ch, TO_VICT);
            return TRUE;
          } else {

            act("$N gives $n some money.",
				FALSE, this_mob, 0, ch, TO_NOTVICT);
            send_to_char(ch, "You give %s %d coins.\r\n",
                    GET_NAME(this_mob), clan_prices[i].price);
            decrease_gold(ch, clan_prices[i].price);
            /* Uncomment the next line to make the mob get RICH! */
            /* increase_gold(this_mob, clan_prices[i].price); */

            cast_spell(this_mob, ch, NULL, clan_prices[i].number);
            return TRUE;

          }
        }
      }
      act("$n tells you, 'I do not know of that spell!"
          "  Type 'buy' for a list.'", FALSE, this_mob,
          0, ch, TO_VICT);

      return TRUE;
    } else {
      act("$n tells you, 'Here is a listing of the prices for my services.'",
          FALSE, this_mob, 0, ch, TO_VICT);
      for (i=0; clan_prices[i].number > SPELL_RESERVED_DBC; i++) {
        send_to_char(ch, "%s%d\r\n", clan_prices[i].name, clan_prices[i].price);
      }
      return TRUE;
    }
  }
  return FALSE;
}

SPECIAL(clan_guard)
{
  zone_vnum clanhall, to_zone;
  clan_vnum clan;
  struct char_data *guard = (struct char_data *) me;
  char *buf = "The guard humiliates you, and blocks your way.\r\n";
  char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || IS_AFFECTED(guard, AFF_BLIND))
    return FALSE;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  /* Which clanhall is this cleric in? */
  clanhall = zone_table[(GET_ROOM_ZONE(IN_ROOM(guard)))].number;
  if ((clan = zone_is_clanhall(clanhall)) == NO_CLAN) {
    log("SYSERR: clan_guard spec (%s) not in a known clanhall (room %d)", GET_NAME(guard), world[(IN_ROOM(guard))].number);
    return FALSE;
  }

  /* This is the player's clanhall, allow them to pass */
  if (GET_CLAN(ch) == clan) {
    return FALSE;
  }

  /* If the exit leads to another clanhall room, block it */
  /* NOTE: cmd equals the direction for directional commands */
  if (EXIT(ch, cmd) && EXIT(ch, cmd)->to_room && EXIT(ch, cmd)->to_room != NOWHERE) {
    to_zone = zone_table[(GET_ROOM_ZONE(EXIT(ch, cmd)->to_room))].number;
    if (to_zone == clanhall) {
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  /* If we get here, player is allowed to leave */
  return FALSE;
}

/*
   Portal that will jump to a player's clanhall
   Exit depends on which clan player belongs to
   Created by Jamdog - 4th July 2006
*/
SPECIAL(clanportal)
{
  int iPlayerClan = -1;
  struct obj_data *obj = (struct obj_data *) me;
  struct obj_data *port;
  zone_vnum z;
  room_vnum r;
  char obj_name[MAX_INPUT_LENGTH];
  room_rnum was_in = IN_ROOM(ch);
  struct follow_type *k;

  if (!CMD_IS("enter")) return FALSE;

  argument = one_argument(argument,obj_name);

  /* Check that the player is trying to enter THIS portal */
  if (!(port = get_obj_in_list_vis(ch, obj_name, NULL, world[(IN_ROOM(ch))].contents)))	{
    return(FALSE);
  }

  if (port != obj)
    return(FALSE);

  iPlayerClan = GET_CLAN(ch);

  if (iPlayerClan == NO_CLAN)
  {
    send_to_char(ch, "You try to enter the portal, but it returns you back to the same room!\n\r");
    return TRUE;
  }

  if ((z = get_clanhall_by_char(ch)) == NOWHERE)
  {
    send_to_char(ch, "Your clan does not have a clanhall!\n\r");
    log("Warning: Clan Portal - No clanhall (Player: %s, Clan ID: %d)", GET_NAME(ch), iPlayerClan);
    return TRUE;
  }

 //  r = (z * 100) + 1;    /* Get room xxx01 in zone xxx */
 /* for now lets have the exit room be 3000, until we get hometowns in, etc */
    r = 3000;

  if ( !(real_room(r)) )
  {
    send_to_char(ch, "Your clanhall is currently broken - contact an Imm!\n\r");
    log("Warning: Clan Portal failed (Player: %s, Clan ID: %d)", GET_NAME(ch), iPlayerClan);
    return TRUE;
  }

  /* First, move the player */
  if ( !(House_can_enter(ch, r)) )
  {
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
    return TRUE;
  }

  act("$n enters $p, and vanishes!", FALSE, ch, port, 0, TO_ROOM);
  act("You enter $p, and you are transported elsewhere", FALSE, ch, port, 0, TO_CHAR);
  char_from_room(ch);
  char_to_room(ch, real_room(r));
  look_at_room(ch,0);
  act("$n appears from thin air!", FALSE, ch, 0, 0, TO_ROOM);

  /* Then, any followers should auto-follow (Jamdog 19th June 2006) */
  for (k = ch->followers; k; k = k->next)
  {
    if ((IN_ROOM(k->follower) == was_in) && (GET_POS(k->follower) >= POS_STANDING))
    {
      act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
      char_from_room(k->follower);
      char_to_room(k->follower, real_room(r));
      look_at_room(k->follower,0);
      act("$n appears from thin air!", FALSE, k->follower, 0, 0, TO_ROOM);
    }
  }
  return TRUE;
}


