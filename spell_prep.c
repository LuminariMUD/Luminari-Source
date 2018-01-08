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
 *  1) preparation queue - these are spell in queue for preparation
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
#include "spells.h"
#include "spell_prep.h"
 
/** END header files **/


/** START Globals **/

/** END Globals **/


/** START functions related to the spell-preparation queue handling **/

/* clear a ch's spell prep queue, example death?, ch loadup */
void init_ch_spell_prep_queue(struct char_data *ch) {
  
}

/* destroy the spell prep queue, example ch logout */
void destroy_ch_spell_prep_queue(struct char_data *ch) {
  int ch_class;

  for (ch_class = 0; ch_class < NUM_CASTERS; ch_class++) {
    do {
      struct prep_collection_spell_data *tmp;
      tmp = SPELL_PREP_QUEUE(ch, ch_class);
      SPELL_PREP_QUEUE(ch, ch_class) = SPELL_PREP_QUEUE(ch, ch_class)->next;
      free(tmp);
    } while (SPELL_PREP_QUEUE(ch, ch_class));
  }
}

/* load from pfile into ch their spell-preparation queue, example ch login */
void load_ch_spell_prep_queue() {
}

/* save into ch pfile their spell-preparation queue, example ch saving */
void save_ch_spell_prep_queue() {  
}

/* checks the ch's spell prep-queue for a given spell_num
   returns the class corresponding to where the spell was found */
int is_spell_in_prep_queue(struct char_data *ch, int spell_num) {
  int ch_class;

  for (ch_class = 0; ch_class < NUM_CASTERS; ch_class++) {
    do {
      if (PREP_QUEUE_ITEM_SPELLNUM(ch, ch_class) == spell_num)
        return ch_class;
    } while (SPELL_PREP_QUEUE(ch, ch_class));
  }
  
  return FALSE;
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
struct prep_collection_spell_data *spell_to_prep_queue(struct char_data *ch, int spell, int ch_class, int metamagic,  int prep_time) {
  struct prep_collection_spell_data *prep_queue_data = NULL;
  
  /* allocate memory, create entry with data */
  prep_queue_data = create_prep_queue_entry(spell, ch_class, metamagic, prep_time);

  /* put on bottom of appropriate class list */
  prep_queue_data->next = SPELL_PREP_QUEUE(ch, ch_class);
  
  return prep_queue_data;
}

/* remove a spell from the spell_prep queue
 * returns an instance of the spell item we found
 * example ch finished memorizing, 'forgot' */
struct prep_collection_spell_data *spell_from_prep_queue(struct char_data *ch, int spell,
        int ch_class) {
  struct prep_collection_spell_data *current = SPELL_PREP_QUEUE(ch, ch_class);
  struct prep_collection_spell_data *prev = NULL;
  struct prep_collection_spell_data *prep_queue_data = NULL;

  do {
    /* does the current entry's spell match the spellnum we are searching for? */
    if (current->spell == spell) {
      /* we have to duplicate this entry so we can return it */
      prep_queue_data = create_prep_queue_entry(
              current->spell,
              current->ch_class,
              current->metamagic,
              current->prep_time
              );
      break;
    }
    /* transverse */
    prev = current;
    current = current->next;
  } while (current);

  /* happen to have been the first element? */
  if (current == SPELL_PREP_QUEUE(ch, ch_class)) {
    /* we will re-use prev */
    prev = SPELL_PREP_QUEUE(ch, ch_class);
    SPELL_PREP_QUEUE(ch, ch_class) = current->next;
    free(prev);
    return prep_queue_data;
  }

  /* happen to be the last element? */
  if (current->next == NULL) {
    prev->next = NULL;
    free(current);
    return prep_queue_data;
  }

  prev->next = current->next;
  free(current);
  return prep_queue_data;
}

/** END functions related to the spell-preparation queue handling **/


/** START functions related to the spell-collection handling **/

/* clear a ch's spell collection, example death?, ch loadup */
void init_ch_spell_collection(struct char_data *ch) {
}

/* destroy a ch's spell prep queue, example ch logout */
void destroy_ch_spell_collection(struct char_data *ch) {
  int ch_class;

  for (ch_class = 0; ch_class < NUM_CASTERS; ch_class++) {
    do {
      struct prep_collection_spell_data *tmp;
      tmp = SPELL_COLLECTION(ch, ch_class);
      SPELL_COLLECTION(ch, ch_class) = SPELL_COLLECTION(ch, ch_class)->next;
      free(tmp);
    } while (SPELL_COLLECTION(ch, ch_class));
  }
}

/* load from pfile into ch their spell-preparation queue, example ch login */
void load_ch_spell_collection(struct char_data *ch) {
}

/* save into ch pfile their spell-preparation queue, example ch saving */
void save_ch_spell_collection(struct char_data *ch) {  
}

/* checks the ch's spell collection for a given spell_num
   returns the class corresponding to where the spell was found */
int is_spell_in_collection(struct char_data *ch, int spell_num) {
  int ch_class;

  for (ch_class = 0; ch_class < NUM_CASTERS; ch_class++) {
    do {
      if (COLLECTIONE_ITEM_SPELLNUM(ch, ch_class) == spell_num)
        return ch_class;
    } while (SPELL_COLLECTION(ch, ch_class));
  }
  
  return FALSE;
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
struct prep_collection_spell_data *spell_to_collection(struct char_data *ch,
        int spell, int ch_class, int metamagic,  int prep_time) {
  struct prep_collection_spell_data *collection_data = NULL;
  
  /* allocate memory, create entry with data */
  collection_data = create_collection_entry(spell, ch_class, metamagic, prep_time);

  /* put on bottom of appropriate class list */
  collection_data->next = SPELL_COLLECTION(ch, ch_class);
  
  return collection_data;
}

/* remove a spell from a collection
 * returns spell-number if successful, SPELL_RESERVED_DBC if fail
 *  example ch cast the spell */
struct prep_collection_spell_data *spell_from_collection(struct char_data *ch, int spell, int ch_class) {
  struct prep_collection_spell_data *current = SPELL_COLLECTION(ch, ch_class);
  struct prep_collection_spell_data *prev = NULL;
  struct prep_collection_spell_data *collection_data = NULL;

  do {
    /* does the current entry's spell match the spellnum we are searching for? */
    if (current->spell == spell) {
      /* we have to duplicate this entry so we can return it */
      collection_data = create_collection_entry(
              current->spell,
              current->ch_class,
              current->metamagic,
              current->prep_time
              );
      break;
    }
    /* transverse */
    prev = current;
    current = current->next;
  } while (current);

  /* happen to have been the first element? */
  if (current == SPELL_COLLECTION(ch, ch_class)) {
    /* we will re-use prev */
    prev = SPELL_COLLECTION(ch, ch_class);
    SPELL_COLLECTION(ch, ch_class) = current->next;
    free(prev);
    return collection_data;
  }

  /* happen to be the last element? */
  if (current->next == NULL) {
    prev->next = NULL;
    free(current);
    return collection_data;
  }

  prev->next = current->next;
  free(current);
  return collection_data;
}

/** END functions related to the spell-collection handling **/

/** START functions that connect the spell-queue and collection */

/* spell from queue to collection */
bool item_from_queue_to_collection(struct char_data *ch, int spell) {  
  int class = is_spell_in_prep_queue(ch, spell);
  
  if (class) {
    struct prep_collection_spell_data *item =
            spell_from_prep_queue(ch, spell, class);
    spell_to_collection(
            ch,
            item->spell,
            item->ch_class,
            item->metamagic,
            item->prep_time
            );
    return TRUE;
  }
  
  return FALSE;
}

/** END functions that connect the spell-queue and collection */

/***EOF***/
