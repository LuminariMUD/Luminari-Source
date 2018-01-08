/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\                                                             
/    Luminari Spell Prep System
/  Created By: Zusuk                                                           
\  Header:     spell_prep.h                                                           
/    Handling spell preparation for all casting classes, memorization                                                           
\    system, queue, related commands, etc
/  Created on January 8, 2018, 3:27 PM                                                                                                                                                                                     
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/


/** START general notes */
/*
 *  The two major elements of the spell-preparation system include:
 *  1) preparation queue - these are memories in queue for preparation
 *  2) spell collection - these are all the spells that are prepared
 *                        ready for usage, better known as "prepared spells"
 * 
 *
 */
/** END general notes */


/** START header files **/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spell_prep.h"
 
/** END header files **/


/** START Globals **/

/* here is our preparation queue for a char's spells */
struct prep_collection_spell_data *preparation_queue[NUM_CASTERS];

/* here is our spell collection (prepared-spells) for a char's spells */
struct prep_collection_spell_data *spell_collection[NUM_CASTERS];

/** END Globals **/


/** START functions related to the spell-preparation queue handling **/

/* clear a ch's spell prep queue, example death?, ch loadup */
void init_ch_spell_prep_queue() {
}

/* destroy a ch's spell prep queue, example ch logout */
void destroy_ch_spell_prep_queue() {
}

/* load from pfile into ch their spell-preparation queue, example ch login */
void load_ch_spell_prep_queue() {
}

/* save into ch pfile their spell-preparation queue, example ch saving */
void save_ch_spell_prep_queue() {  
}

/* create a new spell prep-queue entry */
struct prep_collection_spell_data *create_prep_queue_entry(int spell, int ch_class, int metamagic,
        int prep_time) {
  struct prep_collection_spell_data *prep_queue_data = NULL;

  CREATE(prep_queue_data, struct prep_collection_spell_data, 1);
  prep_queue_data->spell = spell;
  prep_queue_data->ch_class = ch_class;
  prep_queue_data->metamagic = metamagic;
  /* old system - dynamic now */
  prep_queue_data->prep_time = prep_time;

  return prep_queue_data;
}

/* add a spell to bottom of prep queue, example ch is memorizING a spell */
void spell_to_prep_queue(int spell, int ch_class, int metamagic,  int prep_time) {
  struct prep_collection_spell_data *prep_queue_data = NULL;
  
  /* allocate memory, create entry with data */
  prep_queue_data = create_prep_queue_entry(spell, ch_class, metamagic, prep_time);

  /* put on bottom of appropriate class list */
  prep_queue_data->next = preparation_queue[ch_class];
  
  return;
}

/* remove a spell from the spell_prep queue example ch finished memorizing, 'forgot' */
void spell_from_prep_queue(int spell, int ch_class) {
  struct prep_collection_spell_data *current = preparation_queue[ch_class];
  struct prep_collection_spell_data *prev = NULL;

  do {
    
    /* does the current entry's spell match the spellnum we are searching for? */
    if (current->spell == spell) {
      break;
    }
    
    /* transverse */
    prev = current;
    current = current->next;
  } while (current);

  /* happen to have been the first element? */
  if (current == preparation_queue[ch_class]) {
    /* we will re-use prev */
    prev = preparation_queue[ch_class];
    preparation_queue[ch_class] = current->next;
    free(prev);
    return;
  }

  /* happen to be the last element? */
  if (current->next == NULL) {
    prev->next = NULL;
    free(current);
    return;
  }

  prev->next = current->next;
  free(current);
  return;
}

/** END functions related to the spell-preparation queue handling **/


/** START functions related to the spell-collection handling **/

/* clear a ch's spell collection, example death?, ch loadup */
void init_ch_spell_collection(struct char_data *ch) {
}

/* destroy a ch's spell prep queue, example ch logout */
void destroy_ch_spell_collection(struct char_data *ch) {
}

/* load from pfile into ch their spell-preparation queue, example ch login */
void load_ch_spell_collection(struct char_data *ch) {
}

/* save into ch pfile their spell-preparation queue, example ch saving */
void save_ch_spell_collection(struct char_data *ch) {  
}

/* create a new spell collection entry */
struct prep_collection_spell_data *create_collection_entry(int spell, int ch_class, int metamagic,
        int prep_time) {
  struct prep_collection_spell_data *collection_data = NULL;

  CREATE(collection_data, struct prep_collection_spell_data, 1);
  collection_data->spell = spell;
  collection_data->ch_class = ch_class;
  collection_data->metamagic = metamagic;
  /* old system - dynamic now */
  collection_data->prep_time = prep_time;

  return collection_data;
}

/* add a spell to bottom of collection, example ch memorized a spell */
void spell_to_collection(int spell, int ch_class, int metamagic,  int prep_time) {
  struct prep_collection_spell_data *collection_data = NULL;
  
  /* allocate memory, create entry with data */
  collection_data = create_collection_entry(spell, ch_class, metamagic, prep_time);

  /* put on bottom of appropriate class list */
  collection_data->next = spell_collection[ch_class];
  
  return;
}

/* remove a spell from a collection example ch cast the spell */
void spell_from_collection(int spell, int ch_class) {
  struct prep_collection_spell_data *current = spell_collection[ch_class];
  struct prep_collection_spell_data *prev = NULL;

  do {
    
    /* does the current entry's spell match the spellnum we are searching for? */
    if (current->spell == spell) {
      break;
    }
    
    /* transverse */
    prev = current;
    current = current->next;
  } while (current);

  /* happen to have been the first element? */
  if (current == spell_collection[ch_class]) {
    /* we will re-use prev */
    prev = spell_collection[ch_class];
    spell_collection[ch_class] = current->next;
    free(prev);
    return;
  }

  /* happen to be the last element? */
  if (current->next == NULL) {
    prev->next = NULL;
    free(current);
    return;
  }

  prev->next = current->next;
  free(current);
  return;
}

/** END functions related to the spell-collection handling **/

/***EOF***/
