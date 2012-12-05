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

/* since the spell array position for classes doesn't correspond 
 * with the class values, we need a little conversion -zusuk
 */
int classArray(int class) {
  switch (class) {
    case CLASS_CLERIC:
      return 0;
    case CLASS_DRUID:
      return 1;
    case CLASS_MAGIC_USER:
      return 2;
  }
  log("Invalid class passed to memorize.c's classArray()!");
  return 0;
}

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

  for (slot = 0; slot < MAX_MEM; slot++) {
    if (PRAYING(ch, slot, classArray(class)) == TERMINATE) {
      PRAYING(ch, slot, classArray(class)) = spellnum;
      PRAYTIME(ch, slot, classArray(class)) = time;
      break;
    }
  }
}


// resets the memtimes for character (in case of aborted studies)
void resetMemtimes(struct char_data *ch, int class)
{
  int slot;

  for (slot = 0; slot < MAX_MEM; slot++) {
    if (PRAYING(ch, slot, classArray(class)) == TERMINATE)
      break;
    PRAYTIME(ch, slot, classArray(class)) = 
            spell_info[PRAYING(ch, slot, classArray(class))].memtime;
  }
}


// adds <spellnum> to the next available slot in the characters
// memorized list
void addSpellMemmed(struct char_data *ch, int spellnum, int class)
{
  int slot;

  for (slot = 0; slot < MAX_MEM; slot++) {
    if (PRAYED(ch, slot, classArray(class)) == 0) {
      PRAYED(ch, slot, classArray(class)) = spellnum;
      return;
    }
  }
}


// finds the first instance of <spellnum> in the characters
// memorizing spells, forgets it, then updates the memorizing list
void removeSpellMemming(struct char_data *ch, int spellnum, int class)
{
  int slot, nextSlot;

  for (slot = 0; slot < MAX_MEM; slot++) {
    if (PRAYING(ch, slot, classArray(class)) == spellnum) { //found the spell
      if (PRAYING(ch, slot + 1, classArray(class)) != TERMINATE) { //more in list of memming?
        for (nextSlot = slot; nextSlot < MAX_MEM - 1; nextSlot++) { 
          //go through rest of list
          PRAYING(ch, nextSlot, classArray(class)) =
                  PRAYING(ch, nextSlot + 1, classArray(class));
          PRAYTIME(ch, nextSlot, classArray(class)) =
                  PRAYTIME(ch, nextSlot + 1, classArray(class));  //shift everything
        }
        PRAYING(ch, nextSlot, classArray(class)) = TERMINATE;  //end of list tagged with terminate
        PRAYTIME(ch, nextSlot, classArray(class)) = TERMINATE;
      } else {
        PRAYING(ch, slot, classArray(class)) = TERMINATE;  //the spell found was last in list
        PRAYTIME(ch, slot, classArray(class)) = TERMINATE;
      }
      return;
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
  int slot, nextSlot, found = 0, x = 0;

  /* dummy check */
  if (class == -1) {
    for (slot = 0; slot < NUM_CASTERS; slot++) {
      if (PRAYED(ch, 0, slot))
        found++;
    }
    if (!found) {
      return -1;
    }
  }

  /* we know the class */
  if (class != -1) {  
    if (PRAYED(ch, 0, classArray(class))) {
      for (slot = 0; slot < (MAX_MEM); slot++) {
        if (PRAYED(ch, slot, classArray(class)) == spellnum) {
          if (PRAYED(ch, slot + 1, classArray(class)) != 0) {
            for (nextSlot = slot; nextSlot < (MAX_MEM) - 1; nextSlot++) {
              PRAYED(ch, nextSlot, classArray(class)) =
                    PRAYED(ch, nextSlot + 1, classArray(class));
            }
            PRAYED(ch, nextSlot, classArray(class)) = 0;
          } else {
            PRAYED(ch, slot, classArray(class)) = 0;
          }
          return class;
        }
      }
    }
  } else { /* class == -1 */
    /* we don't know the class, so search all the arrays */
    for (x = 0; x < NUM_CASTERS; x++) {
      if (PRAYED(ch, 0, classArray(x))) {
        for (slot = 0; slot < (MAX_MEM); slot++) {
          if (PRAYED(ch, slot, classArray(x)) == spellnum) {
            if (PRAYED(ch, slot + 1, classArray(x)) != 0) {
              for (nextSlot = slot; nextSlot < (MAX_MEM) - 1; nextSlot++) {
                PRAYED(ch, nextSlot, classArray(x)) =
                    PRAYED(ch, nextSlot + 1, classArray(x));
              }
              PRAYED(ch, nextSlot, classArray(x)) = 0;
            } else {
              PRAYED(ch, slot, classArray(x)) = 0;
            }
            return x;
          }
        }
      }    
    }
  }

  /* failed to find anything */
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
  if (IS_NPC(ch)) {
    return (MIN(9, (GET_LEVEL(ch) + 1) / 2));
  }

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

  for (slot = 0; slot < (MAX_MEM); slot++) {
    if (spellCircle(class, PRAYED(ch, slot, classArray(class))) == circle)
      num++;
    if (spellCircle(class, PRAYING(ch, slot, classArray(class))) == circle)
      num++;
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

  //if you aren't resting, can't mem
  if (GET_POS(ch) != POS_RESTING) {
    switch (class) {
      case CLASS_MAGIC_USER:
        send_to_char(ch, "You abort your studies.\r\n");
        act("$n aborts $s studies.", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case CLASS_CLERIC:
        send_to_char(ch, "You abort your prayers.\r\n");
        act("$n aborts $s prayers.", FALSE, ch, 0, 0, TO_ROOM);
        break;
      case CLASS_DRUID:
        send_to_char(ch, "You abort your communing.\r\n");
        act("$n aborts $s communing.", FALSE, ch, 0, 0, TO_ROOM);
        break;
    }
    resetMemtimes(ch, class);
    PRAYIN(ch, classArray(class)) = FALSE;
    return;
  }

  // no mem list
  if (PRAYING(ch, 0, classArray(class)) == TERMINATE) {
    PRAYIN(ch, classArray(class)) = FALSE;
    return;
  }

  // continue memorizing
  PRAYTIME(ch, 0, classArray(class)) -= bonus;
  if (PRAYTIME(ch, 0, classArray(class)) <= 0) {
    switch (class) {
      case CLASS_CLERIC:
        sprintf(buf, "You finish praying for %s.\r\n",
                spell_info[PRAYING(ch, 0, classArray(class))].name);
        break;
      case CLASS_DRUID:
        sprintf(buf, "You finish communing for %s.\r\n",
                spell_info[PRAYING(ch, 0, classArray(class))].name);
        break;
      default: // magic user
        sprintf(buf, "You finish memorizing %s.\r\n",
                spell_info[PRAYING(ch, 0, classArray(class))].name);
        break;        
    }
    send_to_char(ch, buf);
    addSpellMemmed(ch, PRAYING(ch, 0, classArray(class)), class);
    removeSpellMemming(ch, PRAYING(ch, 0, classArray(class)), class);
    if (PRAYING(ch, 0, classArray(class)) == TERMINATE) {
      switch (class) {
        case CLASS_CLERIC:
          send_to_char(ch, "Your prayers are complete.\r\n");
          act("$n completes $s prayers.", FALSE, ch, 0, 0, TO_ROOM);
          break;
        case CLASS_DRUID:
          send_to_char(ch, "Your communing is complete.\r\n");
          act("$n completes $s communing.", FALSE, ch, 0, 0, TO_ROOM);
          break;
        default: // magic user
          send_to_char(ch, "Your studies are complete.\r\n");
          act("$n completes $s studies.", FALSE, ch, 0, 0, TO_ROOM);
          break;        
      }
      PRAYIN(ch, classArray(class)) = FALSE;
      return;
    }
  }
  NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);

}


EVENTFUNC(event_memorizing)
{
  int x = 0;
  struct char_data *ch;
  struct mud_event_data *pMudEvent;
  
  //initialize everything and dummy checks 
  if (event_obj == NULL) return 0;
  pMudEvent = (struct mud_event_data *) event_obj;
  ch = (struct char_data *) pMudEvent->pStruct;

  for (x = 0; x < NUM_CASTERS; x++) {
    if (PRAYIN(ch, classArray(x))) {
      updateMemming(ch, x);
      return 0;
    }
  }

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
  for (slot = 0; slot < (MAX_MEM); slot++) {
    if (PRAYED(ch, slot, classArray(class)) == TERMINATE)
      break;
    else
      num[PRAYED(ch, slot, classArray(class))]++;
  }

  /***  display memorized spells ***/
  if (PRAYED(ch, 0, classArray(class)) != 0) {
    switch (class) {
      case CLASS_DRUID:
        send_to_char(ch, "\r\n\tGYou have communed for the following spells:\r\n\r\n");
        break;
      case CLASS_CLERIC:
        send_to_char(ch, "\r\n\tGYou have prayed for the following spells:\r\n\r\n");
        break;
      default:  /* magic user */
        send_to_char(ch, "\r\n\tGYou have memorized the following spells:\r\n\r\n");
        break;
    }
    for (slot = getCircle(ch, class); slot > 0; slot--) {
      printed = FALSE;
      for (memSlot = 0; memSlot < (MAX_MEM); memSlot++) {
        if (PRAYED(ch, memSlot, classArray(class)) != 0 &&
            spellCircle(class, PRAYED(ch, memSlot, classArray(class))) == slot) {
          if (num[PRAYED(ch, memSlot, classArray(class))] != 0) {
            if (!printed) {
              send_to_char(ch, "[Circle: %d]          %2d - %s\r\n",
			            slot, num[PRAYED(ch, memSlot, classArray(class))],
			            spell_info[PRAYED(ch, memSlot, 
                                      classArray(class))].name);
              printed = TRUE;
              num[PRAYED(ch, memSlot, classArray(class))] = 0;
            } else {
              send_to_char(ch, "                     %2d - %s\r\n",
			            num[PRAYED(ch, memSlot, classArray(class))],
			            spell_info[PRAYED(ch, memSlot, 
                                      classArray(class))].name);
              num[PRAYED(ch, memSlot, classArray(class))] = 0;
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
  if (PRAYING(ch, 0, classArray(class)) != 0) {
    if (PRAYIN(ch, classArray(class))) {
      switch (class) {
        case CLASS_DRUID:
          send_to_char(ch, "\r\n\tCYou are currently communing for:\r\n");
          break;
        case CLASS_CLERIC:
          send_to_char(ch, "\r\n\tCYou are currently praying for:\r\n");
          break;
        default:  /* magic user */
          send_to_char(ch, "\r\n\tCYou are currently memorizing:\r\n");
          break;
      }
    } else {
      switch (class) {
        case CLASS_DRUID:
          send_to_char(ch, "\r\n\tCYou are ready to commune for: (type 'rest' "
                       "then 'commune' to continue)\r\n");
          break;
        case CLASS_CLERIC:
          send_to_char(ch, "\r\n\tCYou are ready to pray for: (type 'rest' "
                       "then 'pray' to continue)\r\n");
          break;
        default:  /* magic user */
          send_to_char(ch, "\r\n\tCYou are ready to memorize: (type 'rest' "
                       "then 'memorize' to continue)\r\n");
          break;
      }
    }
    for (slot = 0; slot < (MAX_MEM); slot++) {
      if (PRAYING(ch, slot, classArray(class)) != 0) {
        spellLevel = spellCircle(class, PRAYING(ch, slot, classArray(class)));
        send_to_char(ch, "  %s [%d%s] with %d seconds remaining.\r\n",
                     spell_info[PRAYING(ch, slot, classArray(class))].name,
                     spellLevel, spellLevel == 1 ? "st" : spellLevel == 2 ?
                     "nd" : spellLevel == 3 ? "rd" : "th",
                     PRAYTIME(ch, slot, classArray(class)));
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
  switch (class) {
    case CLASS_DRUID:
      send_to_char(ch, "\r\nYou can commune");
      break;
    case CLASS_CLERIC:
      send_to_char(ch, "\r\nYou can pray");
      break;
    default:  /* magic user */
      send_to_char(ch, "\r\nYou can memorize");
      break;
  }
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
  switch (class) {
    case CLASS_DRUID:
      send_to_char(ch, "\tDCommands: commune <spellname>, uncommune <spellname>, "
                     "spells druid\tn\r\n");
      break;
    case CLASS_CLERIC:
      send_to_char(ch, "\tDCommands: prayer <spellname>, blank <spellname>, "
                     "spells cleric\tn\r\n");
      break;
    default:  /* magic user */
      send_to_char(ch, "\tDCommands: memorize <spellname>, forget <spellname>, "
                     "spells magic user\tn\r\n");
      break;
  }
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
    if (PRAYING(ch, 0, classArray(class))) {
      for (slot = 0; slot < (MAX_MEM); slot++) {
        PRAYING(ch, slot, classArray(class)) = 0;
      }
      switch (class) {
        case CLASS_DRUID:
          send_to_char(ch, "You forget everything you were attempting to "
                           "commune for.\r\n");
          break;
        case CLASS_CLERIC:
          send_to_char(ch, "You forget everything you were attempting to "
                           "pray for.\r\n");
          break;
        default:  /* magic user */
          send_to_char(ch, "You forget everything you were attempting to "
                           "memorize.\r\n");
          break;
      }
      PRAYIN(ch, classArray(class)) = FALSE;
      return;
    } else if (PRAYED(ch, 0, classArray(class))) {
      for (slot = 0; slot < (MAX_MEM); slot++) {
        PRAYED(ch, slot, classArray(class)) = 0;
      }
      switch (class) {
        case CLASS_DRUID:
          send_to_char(ch, "You forget everything you had communed for.\r\n");
          break;
        case CLASS_CLERIC:
          send_to_char(ch, "You forget everything you had prayed for.\r\n");
          break;
        default:  /* magic user */
          send_to_char(ch, "You forget everything you had memorized.\r\n");
          break;
      }
      PRAYIN(ch, classArray(class)) = FALSE;
      return;
    } else {
      switch (class) {
        case CLASS_DRUID:
          send_to_char(ch, "You do not have anything communed for!\r\n");
          break;
        case CLASS_CLERIC:
          send_to_char(ch, "You do not have anything prayed for!\r\n");
          break;
        default:  /* magic user */
          send_to_char(ch, "You do not have anything memorizing/memorized!\r\n");
          break;
      }
      return;
    }
  }
  
  if (spellnum < 1 || spellnum > MAX_SPELLS) {
    send_to_char(ch, "You never knew that to begin with.\r\n");
    return;
  }

  // are we memorizing it?
  for (slot = 0; slot < (MAX_MEM); slot++) {
    if (PRAYING(ch, slot, classArray(class)) == spellnum) {
      removeSpellMemming(ch, spellnum, class);
      switch (class) {
        case CLASS_DRUID:
          send_to_char(ch, "You stop communing for %s.\r\n", spell_info[spellnum].name);
          break;
        case CLASS_CLERIC:
          send_to_char(ch, "You stop praying for %s.\r\n", spell_info[spellnum].name);
          break;
        default:  /* magic user */
          send_to_char(ch, "You stop memorizing %s.\r\n", spell_info[spellnum].name);
          break;
      }
      return;
    }
  }

  // is it memmed?
  for (slot = 0; slot < (MAX_MEM); slot++) {
    if (PRAYED(ch, slot, classArray(class)) == spellnum) {
      forgetSpell(ch, spellnum, class);
      switch (class) {
        case CLASS_DRUID:
          send_to_char(ch, "You purge %s from your communion.\r\n",
                       spell_info[spellnum].name);
          break;
        case CLASS_CLERIC:
          send_to_char(ch, "You purge %s from your prayers.\r\n",
                       spell_info[spellnum].name);
          break;
        default:  /* magic user */
          send_to_char(ch, "You purge %s from your memory.\r\n",
                       spell_info[spellnum].name);
          break;
      }
      return;
    }
  }

  switch (class) {
    case CLASS_DRUID:
      send_to_char(ch, "You aren't communing for and don't have communed %s!\r\n",
                   spell_info[spellnum].name);
      break;
    case CLASS_CLERIC:
      send_to_char(ch, "You aren't praying for and don't have prayed %s!\r\n",
                   spell_info[spellnum].name);
      break;
    default:  /* magic user */
      send_to_char(ch, "You aren't memorizing and don't have memorized %s!\r\n",
                   spell_info[spellnum].name);
      break;
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
      if (!isOccupied(ch) && PRAYING(ch, 0, classArray(class)) != 0) {        
        switch (class) {
          case CLASS_DRUID:
            send_to_char(ch, "You continue your communion.\r\n");
            act("$n continues $s communion.", FALSE, ch, 0, 0, TO_ROOM);
            break;
          case CLASS_CLERIC:
            send_to_char(ch, "You continue your prayers.\r\n");
            act("$n continues $s prayers.", FALSE, ch, 0, 0, TO_ROOM);
            break;
          default:  /* magic user */
            send_to_char(ch, "You continue your studies.\r\n");
            act("$n continues $s studies.", FALSE, ch, 0, 0, TO_ROOM);
            break;
        }
        PRAYIN(ch, classArray(class)) = TRUE;
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
    if ((compSlots - num_spells) > 0) {
      switch (class) {
        case CLASS_DRUID:
          send_to_char(ch, "You start to commune for %s.\r\n", spell_info[spellnum].name);
          break;
        case CLASS_CLERIC:
          send_to_char(ch, "You start to pray for %s.\r\n", spell_info[spellnum].name);
          break;
        default:  /* magic user */
          send_to_char(ch, "You start to memorize %s.\r\n", spell_info[spellnum].name);
          break;
      }
      addSpellMemming(ch, spellnum, spell_info[spellnum].memtime, class);
      if (!isOccupied(ch)) {
        PRAYIN(ch, classArray(class)) = TRUE;
        NEW_EVENT(eMEMORIZING, ch, NULL, 1 * PASSES_PER_SEC);
        switch (class) {
          case CLASS_DRUID:
            send_to_char(ch, "You continue your communing.\r\n");
            act("$n continues $s communing.", FALSE, ch, 0, 0, TO_ROOM);
            break;
          case CLASS_CLERIC:
            send_to_char(ch, "You continue your prayers.\r\n");
            act("$n continues $s prayers.", FALSE, ch, 0, 0, TO_ROOM);
            break;
          default:  /* magic user */
            send_to_char(ch, "You continue your studies.\r\n");
            act("$n continues $s studies.", FALSE, ch, 0, 0, TO_ROOM);
            break;
        }
      }
      return;
    } else {
      send_to_char(ch, "You can't retain more spells of that level!\r\n");
    }
  } else
    log("ERR:  Reached end of do_gen_memorize.");

}

/***  end command functions ***/


