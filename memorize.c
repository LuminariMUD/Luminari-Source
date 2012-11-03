/*
   Realms of Luminari
   File: memorize.c
   Usage: spell memorization functions
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "db.h"
#include "comm.h"
#include "mud_event.h"
#include "constants.h"
#include "act.h"

char buf[MAX_INPUT_LENGTH];
#define	TERMINATE	0

// NOT the same as IS_CLERIC / IS_MAGIC_USER
#define ISCLERIC	(class == CLASS_CLERIC)
#define ISDRUID		(class == CLASS_DRUID)
#define ISMAGE		(class == CLASS_MAGIC_USER)

// pray array
#define CL	0 //cleric
#define DR	1 //druid
#define MG	2 //mage

//   the number of spells received per level for caster types
int mageSlots[LVL_IMPL + 1][10] = {
// 1st,2nd,3rd,4th,5th,6th,7th,8th,9th,10th
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },	// 0
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  2,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  2,  1,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  3,  2,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  3,  2,  1,  0,  0,  0,  0,  0,  0,  0 },	// 5
  {  3,  3,  2,  0,  0,  0,  0,  0,  0,  0 },
  {  4,  3,  2,  1,  0,  0,  0,  0,  0,  0 },//7
  {  4,  4,  3,  2,  0,  0,  0,  0,  0,  0 },
  {  4,  4,  3,  2,  1,  0,  0,  0,  0,  0 },//9
  {  4,  4,  3,  3,  2,  0,  0,  0,  0,  0 },
  {  4,  4,  4,  3,  2,  1,  0,  0,  0,  0 },//11
  {  4,  4,  4,  3,  3,  2,  0,  0,  0,  0 },
  {  4,  4,  4,  4,  3,  2,  1,  0,  0,  0 },//13
  {  4,  4,  4,  4,  3,  3,  2,  0,  0,  0 },
  {  4,  4,  4,  4,  4,  3,  2,  1,  0,  0 },//15
  {  4,  4,  4,  4,  4,  3,  3,  2,  0,  0 },
  {  4,  4,  4,  4,  4,  4,  3,  2,  1,  0 },//17
  {  4,  4,  4,  4,  4,  4,  3,  3,  2,  0 },
  {  4,  4,  4,  4,  4,  4,  4,  3,  3,  0 },
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//20
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//21
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//22
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//23
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//24
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//25
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//26
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//27
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//28
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//29
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//30
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//31
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//32
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 },//33
  {  4,  4,  4,  4,  4,  4,  4,  4,  4,  0 }//34
};

int clericSlots[LVL_IMPL + 1][10] = {
// 1st,2nd,3rd,4th,5th,6th,7th,8th,9th,10th
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },	// 0
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  2,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  2,  1,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  3,  2,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  3,  2,  1,  0,  0,  0,  0,  0,  0,  0 },	// 5
  {  3,  3,  2,  0,  0,  0,  0,  0,  0,  0 },
  {  4,  3,  2,  1,  0,  0,  0,  0,  0,  0 },
  {  4,  3,  3,  2,  0,  0,  0,  0,  0,  0 },
  {  4,  4,  3,  2,  1,  0,  0,  0,  0,  0 },
  {  4,  4,  3,  3,  2,  0,  0,  0,  0,  0 },	// 10
  {  5,  4,  4,  3,  2,  1,  0,  0,  0,  0 },
  {  5,  4,  4,  3,  3,  2,  0,  0,  0,  0 },
  {  5,  5,  4,  4,  3,  2,  1,  0,  0,  0 },
  {  5,  5,  4,  4,  3,  3,  2,  0,  0,  0 },
  {  5,  5,  5,  4,  4,  3,  2,  1,  0,  0 },	// 15
  {  5,  5,  5,  4,  4,  3,  3,  2,  0,  0 },
  {  5,  5,  5,  5,  4,  4,  3,  2,  1,  0 },
  {  5,  5,  5,  5,  4,  4,  3,  3,  2,  0 },
  {  5,  5,  5,  5,  5,  4,  4,  3,  3,  0 },
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 20
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 21
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 22
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 23
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 24
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 25
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 26
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 27
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 28
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 29
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 30
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 31
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 32
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 33
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 }	// 34
};

int druidSlots[LVL_IMPL + 1][10] = {
// 1st,2nd,3rd,4th,5th,6th,7th,8th,9th,10th
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },	// 0
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  2,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  2,  1,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  3,  2,  0,  0,  0,  0,  0,  0,  0,  0 },
  {  3,  2,  1,  0,  0,  0,  0,  0,  0,  0 },	// 5
  {  3,  3,  2,  0,  0,  0,  0,  0,  0,  0 },
  {  4,  3,  2,  1,  0,  0,  0,  0,  0,  0 },
  {  4,  3,  3,  2,  0,  0,  0,  0,  0,  0 },
  {  4,  4,  3,  2,  1,  0,  0,  0,  0,  0 },
  {  4,  4,  3,  3,  2,  0,  0,  0,  0,  0 },	// 10
  {  5,  4,  4,  3,  2,  1,  0,  0,  0,  0 },
  {  5,  4,  4,  3,  3,  2,  0,  0,  0,  0 },
  {  5,  5,  4,  4,  3,  2,  1,  0,  0,  0 },
  {  5,  5,  4,  4,  3,  3,  2,  0,  0,  0 },
  {  5,  5,  5,  4,  4,  3,  2,  1,  0,  0 },	// 15
  {  5,  5,  5,  4,  4,  3,  3,  2,  0,  0 },
  {  5,  5,  5,  5,  4,  4,  3,  2,  1,  0 },
  {  5,  5,  5,  5,  4,  4,  3,  3,  2,  0 },
  {  5,  5,  5,  5,  5,  4,  4,  3,  3,  0 },
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 20
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 21
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 22
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 23
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 24
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 25
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 26
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 27
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 28
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 29
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 30
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 31
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 32
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 },	// 33
  {  5,  5,  5,  5,  5,  4,  4,  4,  4,  0 }	// 34
};



/*** Utility Functions needed for memorization ***/

int isOccupied(struct char_data *ch) {
  int i;

  for (i = 0; i < NUM_CASTERS; i++)
    if (PRAYIN(ch, i))
      return TRUE;

  return FALSE;
}


// initialize all the spell slots of the character
void init_spell_slots(struct char_data *ch)
{
  int slot, x;

  if (!ch) {
    return;
  }

  for (slot = 0; slot < MAX_MEM; slot++) {
    for (x = 0; x < NUM_CASTERS; x++) {
      PRAYING(ch, slot, x) = 0;
      PRAYED(ch, slot, x) = 0;
      PRAYTIME(ch, slot, x) = 0;
    }
  }
  for (x = 0; x < NUM_CASTERS; x++)
    PRAYIN(ch, x) = FALSE;
}


// given class and spellnum, returns spells circle
int spellCircle(int class, int spellnum)
{
  return ((int)((spell_info[spellnum].min_level[class] + 1) / 2));
}


// returns # of total slots based on level, class and stat bonus
// of given circle
// *note remember in both constant arrays, value 0 is circle 1
int comp_slots(struct char_data *ch, int circle, int class)
{
  int spellSlots = 0;

  circle--;

  switch(class) {
    case CLASS_CLERIC:
      spellSlots += spell_bonus[GET_WIS(ch)][circle];
      spellSlots += clericSlots[CLASS_LEVEL(ch, class)][circle];
      break;
    case CLASS_DRUID:
      spellSlots += spell_bonus[GET_WIS(ch)][circle];
      spellSlots += druidSlots[CLASS_LEVEL(ch, class)][circle];
      break;
    case CLASS_MAGIC_USER:
      spellSlots += spell_bonus[GET_INT(ch)][circle];
      spellSlots += mageSlots[CLASS_LEVEL(ch, class)][circle];
      break;
    default:
      if (GET_LEVEL(ch) < LVL_IMMORT) {
        log("Invalid class passed to comp_slots.");
      }
      return (-1);  
  }  

  return spellSlots;
}


// adds <spellnum> to the characters memorizing list, and
// places the corresponding memtime in memtime list
void addSpellMemming(struct char_data *ch, int spellnum, int time, int class)
{
  int slot;

  if (ISMAGE) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYING(ch, slot, MG) == TERMINATE) {
        PRAYING(ch, slot, MG) = spellnum;
        PRAYTIME(ch, slot, MG) = time;
        break;
      }
    }
  } else if (ISCLERIC) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYING(ch, slot, CL) == TERMINATE) {
        PRAYING(ch, slot, CL) = spellnum;
        PRAYTIME(ch, slot, CL) = time;
        break;
      }
    }
  } else if (ISDRUID) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYING(ch, slot, DR) == TERMINATE) {
        PRAYING(ch, slot, DR) = spellnum;
        PRAYTIME(ch, slot, DR) = time;
        break;
      }
    }
  }
}


// resets the memtimes for character (in case of aborted studies)
void resetMemtimes(struct char_data *ch, int class)
{
  int slot;

  if (ISMAGE) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYING(ch, slot, MG) == TERMINATE)
        break;
      PRAYTIME(ch, slot, MG) = spell_info[PRAYING(ch, slot, MG)].memtime;
    }
  } else if (ISCLERIC) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYING(ch, slot, CL) == TERMINATE)
        break;
      PRAYTIME(ch, slot, CL) = spell_info[PRAYING(ch, slot, CL)].memtime;
    }
  } else if (ISDRUID) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYING(ch, slot, DR) == TERMINATE)
        break;
      PRAYTIME(ch, slot, DR) = spell_info[PRAYING(ch, slot, DR)].memtime;
    }
  }
}


// adds <spellnum> to the next available slot in the characters
// memorized list
void addSpellMemmed(struct char_data *ch, int spellnum, int class)
{
  int slot;

  if (ISMAGE) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYED(ch, slot, MG) == 0) {
        PRAYED(ch, slot, MG) = spellnum;
        return;
      }
    }
  } else if (ISCLERIC) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYED(ch, slot, CL) == 0) {
        PRAYED(ch, slot, CL) = spellnum;
        return;
      }
    }
  } else if (ISDRUID) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYED(ch, slot, DR) == 0) {
        PRAYED(ch, slot, DR) = spellnum;
        return;
      }
    }
  }
}


// finds the first instance of <spellnum> in the characters
// memorizing spells, forgets it, then updates the memorizing list
void removeSpellMemming(struct char_data *ch, int spellnum, int class)
{
  int slot, nextSlot;

  if (ISMAGE) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYING(ch, slot, MG) == spellnum) { //found the spell
        if (PRAYING(ch, slot + 1, MG) != TERMINATE) { //more in list of memming?
          for (nextSlot = slot; nextSlot < MAX_MEM - 1; nextSlot++) { 
            //go through rest of list
            PRAYING(ch, nextSlot, MG) = PRAYING(ch, nextSlot + 1, MG);
            PRAYTIME(ch, nextSlot, MG) = PRAYTIME(ch, nextSlot + 1, MG);  //shift everything
          }
          PRAYING(ch, nextSlot, MG) = TERMINATE;  //end of list tagged with terminate
          PRAYTIME(ch, nextSlot, MG) = TERMINATE;
        } else {
          PRAYING(ch, slot, MG) = TERMINATE;  //the spell found was last in list
          PRAYTIME(ch, slot, MG) = TERMINATE;
        }
        return;
      }
    }
  } else if (ISCLERIC) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYING(ch, slot, CL) == spellnum) { //found the spell
        if (PRAYING(ch, slot + 1, CL) != TERMINATE) { //more in list of memming?
          for (nextSlot = slot; nextSlot < MAX_MEM - 1; nextSlot++) { 
            //go through rest of list
            PRAYING(ch, nextSlot, CL) = PRAYING(ch, nextSlot + 1, CL);
            PRAYTIME(ch, nextSlot, CL) = PRAYTIME(ch, nextSlot + 1, CL);  //shift everything
          }
          PRAYING(ch, nextSlot, CL) = TERMINATE;  //end of list tagged with terminate
          PRAYTIME(ch, nextSlot, CL) = TERMINATE;
        } else {
          PRAYING(ch, slot, CL) = TERMINATE;  //the spell found was last in list
          PRAYTIME(ch, slot, CL) = TERMINATE;
        }
        return;
      }
    }
  } else if (ISDRUID) {
    for (slot = 0; slot < MAX_MEM; slot++) {
      if (PRAYING(ch, slot, DR) == spellnum) { //found the spell
        if (PRAYING(ch, slot + 1, DR) != TERMINATE) { //more in list of memming?
          for (nextSlot = slot; nextSlot < MAX_MEM - 1; nextSlot++) { 
            //go through rest of list
            PRAYING(ch, nextSlot, DR) = PRAYING(ch, nextSlot + 1, DR);
            PRAYTIME(ch, nextSlot, DR) = PRAYTIME(ch, nextSlot + 1, DR);  //shift everything
          }
          PRAYING(ch, nextSlot, DR) = TERMINATE;  //end of list tagged with terminate
          PRAYTIME(ch, nextSlot, DR) = TERMINATE;
        } else {
          PRAYING(ch, slot, DR) = TERMINATE;  //the spell found was last in list
          PRAYTIME(ch, slot, DR) = TERMINATE;
        }
        return;
      }
    }
  }
}


// finds the first instance of <spellnum> in the characters
// memorized spells, forgets it, then updates the memorized list
// *for now it has to do - will extract a mage spell first
// if you can't find it in mage, then take it out of cleric
// *returns class
int forgetSpell(struct char_data *ch, int spellnum, int class)
{
  int slot, nextSlot;

  if (class == -1 &&
	!PRAYED(ch, 0, MG) && !PRAYED(ch, 0, CL) && !PRAYED(ch, 0, DR)) {
    return -1;
  }

  if ((ISMAGE || class == -1) && PRAYED(ch, 0, MG)) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYED(ch, slot, MG) == spellnum) {
        if (PRAYED(ch, slot + 1, MG) != 0) {
          for (nextSlot = slot; nextSlot < (MAX_MEM) - 1; nextSlot++) {
            PRAYED(ch, nextSlot, MG) = PRAYED(ch, nextSlot + 1, MG);
          }
          PRAYED(ch, nextSlot, MG) = 0;
        } else {
          PRAYED(ch, slot, MG) = 0;
        }
        return CLASS_MAGIC_USER;
      }
    }
  }
  if ((ISCLERIC || class == -1) && PRAYED(ch, 0, CL)) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYED(ch, slot, CL) == spellnum) {
        if (PRAYED(ch, slot + 1, CL) != 0) {
          for (nextSlot = slot; nextSlot < (MAX_MEM) - 1; nextSlot++) {
            PRAYED(ch, nextSlot, CL) = PRAYED(ch, nextSlot + 1, CL);
          }
          PRAYED(ch, nextSlot, CL) = 0;
        } else {
          PRAYED(ch, slot, CL) = 0;
        }
        return CLASS_CLERIC;
      }
    }
  }
  if ((ISDRUID || class == -1) && PRAYED(ch, 0, DR)) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYED(ch, slot, DR) == spellnum) {
        if (PRAYED(ch, slot + 1, DR) != 0) {
          for (nextSlot = slot; nextSlot < (MAX_MEM) - 1; nextSlot++) {
            PRAYED(ch, nextSlot, DR) = PRAYED(ch, nextSlot + 1, DR);
          }
          PRAYED(ch, nextSlot, DR) = 0;
        } else {
          PRAYED(ch, slot, DR) = 0;
        }
        return CLASS_DRUID;
      }
    }
  }

  return -1;
}


// returns TRUE if the character has the spell memorized
// returns FALSE if the character doesn't
bool hasSpell(struct char_data *ch, int spellnum)
{
  int slot, x;

  // could check to see what classes ch has to speed up this search

  for (slot = 0; slot < MAX_MEM; slot++)
    for (x = 0; x < NUM_CASTERS; x++)
      if (PRAYED(ch, slot, x) == spellnum)
        return TRUE;

  return FALSE;
}


// returns the characters highest circle access in a given class
int getCircle(struct char_data *ch, int class)
{
  if (!CLASS_LEVEL(ch, class)) {
    return 0;
  }
  if (!IS_CASTER(ch)) {
    return (-1);
  }

  if (CLASS_LEVEL(ch, class) < 3)
    return 1;
  else if (CLASS_LEVEL(ch, class) < 5)
    return 2;
  else if (CLASS_LEVEL(ch, class) < 7)
    return 3;
  else if (CLASS_LEVEL(ch, class) < 9)
    return 4;
  else if (CLASS_LEVEL(ch, class) < 11)
    return 5;
  else if (CLASS_LEVEL(ch, class) < 13)
    return 6;
  else if (CLASS_LEVEL(ch, class) < 15)
    return 7;
  else if (CLASS_LEVEL(ch, class) < 17)
    return 8;
  else
    return 9;

}


// returns total spells in both MEMORIZED and MEMORIZING of a given
// circle
int numSpells(struct char_data *ch, int circle, int class)
{
  int num = 0, slot;

  if (ISMAGE) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (spellCircle(class, PRAYED(ch, slot, MG)) == circle)
        num++;
      if (spellCircle(class, PRAYING(ch, slot, MG)) == circle)
        num++;
    }
  } else if (ISCLERIC) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (spellCircle(class, PRAYED(ch, slot, CL)) == circle)
        num++;
      if (spellCircle(class, PRAYING(ch, slot, CL)) == circle)
        num++;
    }
  } else if (ISDRUID) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (spellCircle(class, PRAYED(ch, slot, DR)) == circle)
        num++;
      if (spellCircle(class, PRAYING(ch, slot, DR)) == circle)
        num++;
    }
  }

  return (num);
}

/********** end utility ***************/


/*********** Event Engine ************/

// updates the characters memorizing list/memtime, and
// memorizes the spell upon completion
void updateMemming(struct char_data *ch, int class)
{
  int bonus = 1;

  //calaculate memtime bonus based on concentration
  if (!IS_NPC(ch) && GET_ABILITY(ch, ABILITY_CONCENTRATION)) {
    bonus = MAX(1, GET_ABILITY(ch, ABILITY_CONCENTRATION) / 2 - 3);
  }

  if (GET_POS(ch) != POS_RESTING && ISMAGE) {
    send_to_char(ch, "You abort your studies.\r\n");
    act("$n aborts $s studies.", FALSE, ch, 0, 0, TO_ROOM);
    resetMemtimes(ch, class);
    PRAYIN(ch, MG) = FALSE;
    return;
  } else if (GET_POS(ch) != POS_RESTING && ISCLERIC) {
    send_to_char(ch, "You abort your prayers.\r\n");
    act("$n aborts $s prayers.", FALSE, ch, 0, 0, TO_ROOM);
    resetMemtimes(ch, class);
    PRAYIN(ch, CL) = FALSE;
    return;
  } else if (GET_POS(ch) != POS_RESTING && ISDRUID) {
    send_to_char(ch, "You abort your communing.\r\n");
    act("$n aborts $s communing.", FALSE, ch, 0, 0, TO_ROOM);
    resetMemtimes(ch, class);
    PRAYIN(ch, DR) = FALSE;
    return;
  }

  if (ISMAGE) {
    if (PRAYING(ch, 0, MG) == TERMINATE) {
      PRAYIN(ch, MG) = FALSE;
      return;
    } else {
      PRAYTIME(ch, 0, MG) -= bonus;
      if (PRAYTIME(ch, 0, MG) <= 0) {
        sprintf(buf, "You finish memorizing %s.\r\n",
		spell_info[PRAYING(ch, 0, MG)].name);
        send_to_char(ch, buf);
        addSpellMemmed(ch, PRAYING(ch, 0, MG), class);
        removeSpellMemming(ch, PRAYING(ch, 0, MG), class);
        if (PRAYING(ch, 0, MG) == TERMINATE) {
          send_to_char(ch, "Your studies are complete.\r\n");
          act("$n completes $s studies.", FALSE, ch, 0, 0, TO_ROOM);
          PRAYIN(ch, MG) = FALSE;
          return;
        }
      }
      NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);
    }
  } else if (ISCLERIC) {
    if (PRAYING(ch, 0, CL) == TERMINATE) {
      PRAYIN(ch, CL) = FALSE;
      return;
    } else {
      PRAYTIME(ch, 0, CL) -= bonus;
      if (PRAYTIME(ch, 0, CL) <= 0) {
        sprintf(buf, "You finish praying for %s.\r\n",
		spell_info[PRAYING(ch, 0, CL)].name);
        send_to_char(ch, buf);
        addSpellMemmed(ch, PRAYING(ch, 0, CL), class);
        removeSpellMemming(ch, PRAYING(ch, 0, CL), class);
        if (PRAYING(ch, 0, CL) == TERMINATE) {
          send_to_char(ch, "Your prayers are complete.\r\n");
          act("$n completes $s prayers.", FALSE, ch, 0, 0, TO_ROOM);
          PRAYIN(ch, CL) = FALSE;
          return;
        }
      }
      NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);
    }
  } else if (ISDRUID) {
    if (PRAYING(ch, 0, DR) == TERMINATE) {
      PRAYIN(ch, DR) = FALSE;
      return;
    } else {
      PRAYTIME(ch, 0, DR) -= bonus;
      if (PRAYTIME(ch, 0, DR) <= 0) {
        sprintf(buf, "You finish communing for %s.\r\n",
		spell_info[PRAYING(ch, 0, DR)].name);
        send_to_char(ch, buf);
        addSpellMemmed(ch, PRAYING(ch, 0, DR), class);
        removeSpellMemming(ch, PRAYING(ch, 0, DR), class);
        if (PRAYING(ch, 0, DR) == TERMINATE) {
          send_to_char(ch, "Your communing is complete.\r\n");
          act("$n completes $s communing.", FALSE, ch, 0, 0, TO_ROOM);
          PRAYIN(ch, DR) = FALSE;
          return;
        }
      }
      NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);
    }
  }
}


EVENTFUNC(event_memorizing)
{
  struct char_data *ch;
  struct mud_event_data *pMudEvent;
  
  //initialize everything and dummy checks 
  if (event_obj == NULL) return 0;
  pMudEvent = (struct mud_event_data *) event_obj;
  ch = (struct char_data *) pMudEvent->pStruct;

  if (PRAYIN(ch, MG))
    updateMemming(ch, CLASS_MAGIC_USER);
  else if (PRAYIN(ch, CL))
    updateMemming(ch, CLASS_CLERIC);
  else if (PRAYIN(ch, DR))
    updateMemming(ch, CLASS_DRUID);

  return 0;
}

/*************  end event engine ***************/


/************ display functions ***************/

// display memmed or prayed list
void display_memmed(struct char_data*ch, int class)
{
  int slot, memSlot, num[MAX_SPELLS];
  bool printed;

  //initialize an array size of MAX_SPELLS
  for (slot = 0; slot < MAX_SPELLS; slot++)
    num[slot] = 0;


  //increment the respective spellnum slot in array
  //according to # of spells memmed
  if (ISMAGE) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYED(ch, slot, MG) == TERMINATE)
        break;
      else
        num[PRAYED(ch, slot, MG)]++;
    }
  } else if (ISCLERIC) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYED(ch, slot, CL) == TERMINATE)
        break;
      else
        num[PRAYED(ch, slot, CL)]++;
    }
  } else if (ISDRUID) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYED(ch, slot, DR) == TERMINATE)
        break;
      else
        num[PRAYED(ch, slot, DR)]++;
    }
  }


  /***  display memorized spells ***/
  if (ISMAGE) {

    if (PRAYED(ch, 0, MG) != 0) {
      send_to_char(ch, "\r\n\tGYou have memorized the following spells:\r\n\r\n");
      for (slot = getCircle(ch, class); slot > 0; slot--) {
        printed = FALSE;
        for (memSlot = 0; memSlot < (MAX_MEM); memSlot++) {
          if (PRAYED(ch, memSlot, MG) != 0 &&
		spellCircle(class, PRAYED(ch, memSlot, MG)) == slot) {
            if (num[PRAYED(ch, memSlot, MG)] != 0) {
              if (!printed) {
                send_to_char(ch, "[Circle: %d]          %2d - %s\r\n",
			slot, num[PRAYED(ch, memSlot, MG)],
			spell_info[PRAYED(ch, memSlot, MG)].name);
                printed = TRUE;
                num[PRAYED(ch, memSlot, MG)] = 0;
              } else {
                send_to_char(ch, "                     %2d - %s\r\n",
			num[PRAYED(ch, memSlot, MG)],
			spell_info[PRAYED(ch, memSlot, MG)].name);
                num[PRAYED(ch, memSlot, MG)] = 0;
              }
            }
          }
        }
      }
    }

  } else if (ISCLERIC) {

    if (PRAYED(ch, 0, CL) != 0) {
      send_to_char(ch, "\r\n\tGYou have prayed for the following spells:\r\n\r\n");
      for (slot = getCircle(ch, class); slot > 0; slot--) {
        printed = FALSE;
        for (memSlot = 0; memSlot < (MAX_MEM); memSlot++) {
          if (PRAYED(ch, memSlot, CL) != 0 &&
		spellCircle(class, PRAYED(ch, memSlot, CL)) == slot) {
            if (num[PRAYED(ch, memSlot, CL)] != 0) {
              if (!printed) {
                send_to_char(ch, "[Circle: %d]          %2d - %s\r\n",
			slot, num[PRAYED(ch, memSlot, CL)],
			spell_info[PRAYED(ch, memSlot, CL)].name);
                printed = TRUE;
                num[PRAYED(ch, memSlot, CL)] = 0;
              } else {
                send_to_char(ch, "                     %2d - %s\r\n",
			num[PRAYED(ch, memSlot, CL)],
			spell_info[PRAYED(ch, memSlot, CL)].name);
                num[PRAYED(ch, memSlot, CL)] = 0;
              }
            }
          }
        }
      }
    }

  } else if (ISDRUID) {

    if (PRAYED(ch, 0, DR) != 0) {
      send_to_char(ch, "\r\n\tGYou have communed for the following spells:\r\n\r\n");
      for (slot = getCircle(ch, class); slot > 0; slot--) {
        printed = FALSE;
        for (memSlot = 0; memSlot < (MAX_MEM); memSlot++) {
          if (PRAYED(ch, memSlot, DR) != 0 &&
		spellCircle(class, PRAYED(ch, memSlot, DR)) == slot) {
            if (num[PRAYED(ch, memSlot, DR)] != 0) {
              if (!printed) {
                send_to_char(ch, "[Circle: %d]         %2d - %s\r\n",
			slot, num[PRAYED(ch, memSlot, DR)],
			spell_info[PRAYED(ch, memSlot, DR)].name);
                printed = TRUE;
                num[PRAYED(ch, memSlot, DR)] = 0;
              } else {
                send_to_char(ch, "                    %2d - %s\r\n",
			num[PRAYED(ch, memSlot, DR)],
			spell_info[PRAYED(ch, memSlot, DR)].name);
                num[PRAYED(ch, memSlot, DR)] = 0;
              }
            }
          }
        }
      }
    }

  }

  send_to_char(ch, "\tn");
}


// displays current memming list
void display_memming(struct char_data *ch, int class)
{
  int slot = 0;
  int spellLevel = 0;

  /*** Display memorizing spells ***/
  if (ISMAGE && PRAYING(ch, 0, MG) != 0) {
    if (PRAYIN(ch, MG))
      send_to_char(ch, "\r\n\tCYou are currently memorizing:\r\n");
    else
      send_to_char(ch, "\r\n\tCYou are ready to memorize: (type 'rest' "
                       "then 'memorize' to continue)\r\n");
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYING(ch, slot, MG) != 0) {
        spellLevel = spellCircle(class, PRAYING(ch, slot, MG));
        send_to_char(ch, "  %s [%d%s] with %d seconds remaining.\r\n",
                spell_info[PRAYING(ch, slot, MG)].name, spellLevel,
                spellLevel == 1 ? "st" : spellLevel == 2 ? "nd" : spellLevel == 3 ?
                "rd" : "th", PRAYTIME(ch, slot, MG));
      }
    }
  } else if (ISCLERIC && PRAYING(ch, 0, CL) != 0) {
    if (PRAYIN(ch, CL))
      send_to_char(ch, "\r\n\tCYou are currently praying for:\r\n");
    else
      send_to_char(ch, "\r\n\tCYou are ready to pray for:"
                       "  (type 'rest' then 'pray' to continue)\r\n");
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYING(ch, slot, CL) != 0) {
        spellLevel = spellCircle(class, PRAYING(ch, slot, CL));
        send_to_char(ch, "  %s [%d%s] with %d seconds remaining.\r\n",
                spell_info[PRAYING(ch, slot, CL)].name, spellLevel,
                spellLevel == 1 ? "st" : spellLevel == 2 ? "nd" : spellLevel == 3 ?
                "rd" : "th", PRAYTIME(ch, slot, CL));
      }
    }
  } else if (ISDRUID && PRAYING(ch, 0, DR) != 0) {
    if (PRAYIN(ch, CL))
      send_to_char(ch, "\r\n\tCYou are currently communing for:\r\n");
    else
      send_to_char(ch, "\r\n\tCYou are ready to commune for:"
                       "  (type 'rest' then 'commune' to continue)\r\n");
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYING(ch, slot, DR) != 0) {
        spellLevel = spellCircle(class, PRAYING(ch, slot, DR));
        send_to_char(ch, "  %s [%d%s] with %d seconds remaining.\r\n",
                spell_info[PRAYING(ch, slot, DR)].name, spellLevel,
                spellLevel == 1 ? "st" : spellLevel == 2 ? "nd" : spellLevel == 3 ?
                "rd" : "th", PRAYTIME(ch, slot, DR));
      }
    }
  }

  send_to_char(ch, "\tn");
}


// display how many available slots you have left for spells
void display_slots(struct char_data *ch, int class)
{
  int slot, memSlot, empty[10], last = 0;
  bool printed, spells = FALSE;

  /*** How many more spells can we mem?  ***/
  printed = FALSE;
  memSlot = 0;

  //fill our empty[] with # available slots
  for (slot = 0; slot < getCircle(ch, class); slot++) {
    spells = FALSE;

    if ((empty[slot] = comp_slots(ch, slot + 1, class) -
		numSpells(ch, slot + 1, class)) > 0)
    spells = TRUE;

    if (spells) {
      last = slot;	// how do we punctuate the end
      memSlot++;	// keep track # circles we need to print
    }
  }

  // display info
  if (ISMAGE)
    send_to_char(ch, "\r\nYou can memorize");
  else if (ISCLERIC)
    send_to_char(ch, "\r\nYou can pray");
  else if (ISDRUID)
    send_to_char(ch, "\r\nYou can commune");
  for (slot = 0; slot < getCircle(ch, class); slot++) {
    if (empty[slot] > 0) {
      printed = TRUE;
      send_to_char(ch, " %d %d%s", empty[slot], (slot + 1),
		(slot + 1) == 1 ? "st" : (slot + 1) == 2 ? "nd" : (slot + 1) == 3 ?
		"rd" : "th");
      if (--memSlot > 1)
        send_to_char(ch, ",");
      else if (memSlot == 1)
        send_to_char(ch, " and");
    }
  }
  if (!printed)
    send_to_char(ch, " no more spells.\r\n");
  else
    send_to_char(ch, " circle spell%s.\r\n",
	empty[last] == 1 ? "" : "s");
}


// entry point: lists ch spells, both memorized, memtimes, slots and memorizing
void printMemory(struct char_data *ch, int class)
{

  display_memmed(ch, class);
  display_memming(ch, class);
  display_slots(ch, class);
  if (ISMAGE)
    send_to_char(ch, "\tDCommands: memorize <spellname>, forget <spellname>, "
                     "spells magic user\tn\r\n");
  if (ISCLERIC)
    send_to_char(ch, "\tDCommands: prayer <spellname>, blank <spellname>, "
                     "spells cleric\tn\r\n");
  if (ISDRUID)
    send_to_char(ch, "\tDCommands: commune <spellname>, uncommune <spellname>, "
                     "spells druid\tn\r\n");


}
/************  end display functions **************/


/*************  command functions *****************/

// forget command for players
ACMD(do_gen_forget)
{
  int spellnum, slot, class = -1;
  char arg[MAX_INPUT_LENGTH];

  if (subcmd == SCMD_BLANK)
    class = CLASS_CLERIC;
  else if (subcmd == SCMD_FORGET)
    class = CLASS_MAGIC_USER;
  else if (subcmd == SCMD_UNCOMMUNE)
    class = CLASS_DRUID;
  else {
    send_to_char(ch, "Invalid command!\r\n");
    return;
  }

  skip_spaces(&argument);
  spellnum = find_skill_num(argument);
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What would you like to forget? (or all for everything)\r\n");
    return;
  }
  if (getCircle(ch, class) == -1) {
    send_to_char(ch, "Huh?\r\n");
    return;
  }

  if (!strcmp(arg, "all")) {
    if (ISMAGE) {
      if (PRAYING(ch, 0, MG)) {
        for (slot = 0; slot < (MAX_MEM); slot++) {
          PRAYING(ch, slot, MG) = 0;
        }
        send_to_char(ch, "You forget everything you were attempting to memorize.\r\n");
        PRAYIN(ch, MG) = FALSE;
        return;
      } else if (PRAYED(ch, 0, MG)) {
        for (slot = 0; slot < (MAX_MEM); slot++) {
          PRAYED(ch, slot, MG) = 0;
        }
        send_to_char(ch, "You forget everything you had memorized.\r\n");
        PRAYIN(ch, MG) = FALSE;
        return;
      } else {
        send_to_char(ch, "You do not have anything memorizing/memorized!\r\n");
        return;
      }
    } else if (ISCLERIC) {
      if (PRAYING(ch, 0, CL)) {
        for (slot = 0; slot < (MAX_MEM); slot++) {
          PRAYING(ch, slot, CL) = 0;
        }
        send_to_char(ch, "You purge everything you were attempting to pray for.\r\n");
        PRAYIN(ch, CL) = FALSE;
        return;
      } else if (PRAYED(ch, 0, CL)) {
        for (slot = 0; slot < (MAX_MEM); slot++) {
          PRAYED(ch, slot, CL) = 0;
        }
        send_to_char(ch, "You forget everything you had prayed for.\r\n");
        PRAYIN(ch, CL) = FALSE;
        return;
      } else {
        send_to_char(ch, "You have nothing prayed, and nothing praying for!\r\n");
        return;
      }
    } else if (ISDRUID) {
      if (PRAYING(ch, 0, DR)) {
        for (slot = 0; slot < (MAX_MEM); slot++) {
          PRAYING(ch, slot, DR) = 0;
        }
        send_to_char(ch, "You purge everything you were attempting to commune for.\r\n");
        PRAYIN(ch, DR) = FALSE;
        return;
      } else if (PRAYED(ch, 0, DR)) {
        for (slot = 0; slot < (MAX_MEM); slot++) {
          PRAYED(ch, slot, DR) = 0;
        }
        send_to_char(ch, "You forget everything you had communed for.\r\n");
        PRAYIN(ch, DR) = FALSE;
        return;
      } else {
        send_to_char(ch, "You have nothing communed, and nothing communing for!\r\n");
        return;
      }
    }
  }

  if (spellnum < 1 || spellnum > MAX_SPELLS) {
    send_to_char(ch, "You never knew that to begin with.\r\n");
    return;
  }

  // are we memorizing it?
  if (ISMAGE) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYING(ch, slot, MG) == spellnum) {
        removeSpellMemming(ch, spellnum, class);
        send_to_char(ch, "You stop memorizing %s.\r\n", spell_info[spellnum].name);
        return;
      }
    }
  } else if (ISCLERIC) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYING(ch, slot, CL) == spellnum) {
        removeSpellMemming(ch, spellnum, class);
        send_to_char(ch, "You stop praying for %s.\r\n", spell_info[spellnum].name);
        return;
      }
    }
  } else if (ISDRUID) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYING(ch, slot, DR) == spellnum) {
        removeSpellMemming(ch, spellnum, class);
        send_to_char(ch, "You stop communing for %s.\r\n", spell_info[spellnum].name);
        return;
      }
    }
  }

  // is it memmed?
  if (ISMAGE) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYED(ch, slot, MG) == spellnum) {
        forgetSpell(ch, spellnum, class);
        send_to_char(ch, "You purge %s from your memory.\r\n", spell_info[spellnum].name);
        return;
      }
    }
  } else if (ISCLERIC) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYED(ch, slot, CL) == spellnum) {
        forgetSpell(ch, spellnum, class);
        send_to_char(ch, "You purge %s from your prayers.\r\n", spell_info[spellnum].name);
        return;
      }
    }
  } else if (ISDRUID) {
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYED(ch, slot, DR) == spellnum) {
        forgetSpell(ch, spellnum, class);
        send_to_char(ch, "You purge %s from your commune.\r\n", spell_info[spellnum].name);
        return;
      }
    }
  }

  if (ISMAGE) {
    send_to_char(ch, "You aren't memorizing and don't have memorized %s!\r\n",
	spell_info[spellnum].name);
  } else if (ISCLERIC) {
    send_to_char(ch, "You aren't praying for and don't have prayed %s!\r\n",
	spell_info[spellnum].name);
  } else if (ISDRUID) {
    send_to_char(ch, "You aren't communing for and don't have communed %s!\r\n",
	spell_info[spellnum].name);
  }
}


// memorize command for players
ACMD(do_gen_memorize)
{
  int spellnum, class = -1, num_spells;

  if (subcmd == SCMD_PRAY)
    class = CLASS_CLERIC;
  else if (subcmd == SCMD_MEMORIZE)
    class = CLASS_MAGIC_USER;
  else if (subcmd == SCMD_COMMUNE)
    class = CLASS_DRUID;
  else {
    send_to_char(ch, "Invalid command!\r\n");
    return;
  }

  if (getCircle(ch, class) == -1) {
    send_to_char(ch, "Try changing professions.\r\n");
    return;
  }

  if (!*argument) {
    printMemory(ch, class);

    if (GET_POS(ch) == POS_RESTING) {
      if (ISMAGE && !isOccupied(ch) && PRAYING(ch, 0, MG) != 0) {
        send_to_char(ch, "You continue your studies.\r\n");
        act("$n continues $s studies.", FALSE, ch, 0, 0, TO_ROOM);
        PRAYIN(ch, MG) = TRUE;
        NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);
      } else if (ISCLERIC && !isOccupied(ch) && PRAYING(ch, 0, CL) != 0) {
        send_to_char(ch, "You continue your prayers.\r\n");
        act("$n continues $s prayers.", FALSE, ch, 0, 0, TO_ROOM);
        PRAYIN(ch, CL) = TRUE;
        NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);
      } else if (ISDRUID && !isOccupied(ch) && PRAYING(ch, 0, DR) != 0) {
        send_to_char(ch, "You continue your communion.\r\n");
        act("$n continues $s communion.", FALSE, ch, 0, 0, TO_ROOM);
        PRAYIN(ch, DR) = TRUE;
        NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);
      }
    }
    return;
  } else {
    skip_spaces(&argument);
    spellnum = find_skill_num(argument);
  }

  if (spellnum < 1 || spellnum > MAX_SPELLS) {
    send_to_char(ch, "Memorize what?\r\n");
    return;
  }

  if (GET_POS(ch) != POS_RESTING) {
    send_to_char(ch, "You are not relaxed enough, you must be resting.\r\n");
    return;
  }

  if (CLASS_LEVEL(ch, class) < spell_info[spellnum].min_level[class]) {
    send_to_char(ch, "You do not know that spell.\r\n");
    return;
  }

  int minLevel = 0, compSlots = 0;
  minLevel = spellCircle(class, spellnum);
  compSlots = comp_slots(ch, minLevel, class);
  num_spells = numSpells(ch, minLevel, class);

  if (compSlots != -1) {
    if (ISMAGE) {
      if ((compSlots - num_spells) > 0) {
        send_to_char(ch, "You start to memorize %s.\r\n", spell_info[spellnum].name);
        addSpellMemming(ch, spellnum, spell_info[spellnum].memtime, class);
        if (!isOccupied(ch)) {
          PRAYIN(ch, MG) = TRUE;
          NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);
          send_to_char(ch, "You continue your studies.\r\n");
          act("$n continues $s studies.", FALSE, ch, 0, 0, TO_ROOM);
        }
        return;
      } else {
        send_to_char(ch, "You can't retain more spells of that level!\r\n");
      }
    } else if (ISCLERIC) {
      if ((compSlots - num_spells) > 0) {
        send_to_char(ch, "You start to pray for %s.\r\n", spell_info[spellnum].name);
        addSpellMemming(ch, spellnum, spell_info[spellnum].memtime, class);
        if (!isOccupied(ch)) {
          PRAYIN(ch, CL) = TRUE;
          NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);
          send_to_char(ch, "You continue your prayers.\r\n");
          act("$n continues $s prayers.", FALSE, ch, 0, 0, TO_ROOM);
        }
        return;
      } else {
        send_to_char(ch, "You can't retain more prayers of that level!\r\n");
      }
    } else if (ISDRUID) {
      if ((compSlots - num_spells) > 0) {
        send_to_char(ch, "You start to commune for %s.\r\n", spell_info[spellnum].name);
        addSpellMemming(ch, spellnum, spell_info[spellnum].memtime, class);
        if (!isOccupied(ch)) {
          PRAYIN(ch, DR) = TRUE;
          NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);
          send_to_char(ch, "You continue your communing.\r\n");
          act("$n continues $s communing.", FALSE, ch, 0, 0, TO_ROOM);
        }
        return;
      } else {
        send_to_char(ch, "You can't retain more spells of that level!\r\n");
      }
    }

  } else
    log("ERR:  Reached end of do_gen_memorize.");

}

/***  end command functions ***/


