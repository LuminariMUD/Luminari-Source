/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\     Luminari Spell Prep System                                                        
/  File:       spell_prep.c
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
 *  Terminology:
 *   innate_magic: we are calling the sorcerer/bard type system innate_magic
 *   to differentiate the language of the classes that truly prepare their
 *   spells
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
#include "constants.h"
#include "spec_procs.h"
#include "mud_event.h"
#include "class.h"
#include "spells.h"
#include "spell_prep.h"
 
/** END header files **/


/** START Globals **/

/** END Globals **/


/** START functions related to the spell-preparation queue handling **/

/* clear a ch's spell prep queue, example death?, ch loadup */
void init_ch_spell_prep_queue(struct char_data *ch) {
  
}

/* in: character
 * destroy the spell prep queue, example ch logout */
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

/* in: character, class of queue we want to manage
   go through the entire class's prep queue and reset all the prep-time
     elements to base prep-time */
void reset_prep_queue_time(struct char_data *ch, int ch_class) {
  struct prep_collection_spell_data *current = NULL;

    current = SPELL_PREP_QUEUE(ch, ch_class);
    do {
      current->prep_time = compute_spells_prep_time(
              ch, 
              current->spell,
              ch_class,
              SPELLS_CIRCLE(current->spell, ch_class, current->metamagic)
              );
      
      /* transverse */
      current = current->next;
    } while (current);
  
}

/* in: character
 * out: true if character is actively preparing spells
 *      false if character is NOT preparing spells
 * is character currently occupied with preparing spells? */
bool is_preparing_spells(struct char_data *ch) {
  int i;

  if (char_has_mud_event(ch, ePREPARING))
    return TRUE;

  for (i = 0; i < NUM_CASTERS; i++)
    if (IS_PREPARING(ch, i))
      return TRUE;

  return FALSE;
}

/* in: character, class of the queue you want to work with
 * traverse the prep queue and print out the details
 * since the prep queue does not need any organizing, this should be fairly
 * simple */
void print_prep_queue(struct char_data *ch, int ch_class) {
  char buf[MAX_INPUT_LENGTH];
  int line_length = 80, total_time = 0;

  /* build a nice heading */
  *buf = '\0';
  sprintf(buf, "\tYSPreparation Queue for %s\tC", class_names[ch_class]);
  send_to_char(ch, "\tC");
  text_line(ch, buf, line_length, '-', '-');
  send_to_char(ch, "\tn");

  /* easy out */
  if (!SPELL_PREP_QUEUE(ch, ch_class))
    return;

  struct prep_collection_spell_data *item = SPELL_PREP_QUEUE(ch, ch_class);

  *buf = '\0';
  /* traverse and print */
  do {
    int spell_circle = compute_spells_circle(item->spell, item->ch_class, item->metamagic);
    int prep_time = compute_spells_prep_time(ch, item->spell, item->ch_class, spell_circle);
    total_time += prep_time;

    /* hack alert: innate_magic does not have spell-num stored, but
         instead has just the spell-circle stored as spell-num */
    if (INNATE_MAGIC_CLASS(ch_class))
      sprintf(buf, "%s \tc[\tWcircle-slot:\tn%d\tc]\tn %s%s \tc[\tn%d seconds\tc]\tn\r\n",
              buf,
              item->spell,
              (IS_SET(item->metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
              (IS_SET(item->metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
              prep_time
              );
    else
      sprintf(buf, "%s \tW%s\tn \tc[\tn%d\tc]\tn %s%s \tc[\tn%d seconds\tc]\tn\r\n",
              buf,
              skill_name(item->spell),
              spell_circle,
              (IS_SET(item->metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
              (IS_SET(item->metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
              prep_time
              );
    
    item = item->next;
  } while (SPELL_PREP_QUEUE(ch, ch_class));

  send_to_char(ch, buf);

  /* build a nice closing */
  *buf = '\0';
  sprintf(buf, "\tYSTotal Preparation Time Remaining: %d\tC", total_time);
  send_to_char(ch, "\tC");
  text_line(ch, buf, line_length, '-', '-');
  send_to_char(ch, "\tn");

  /* all done */
  return;
}

/* in: character, spell-number
 * out: class corresponding to the queue we found the spell-number in
 * is the given spell-number currently in the respective class-queue?
 *  */
int is_spell_in_prep_queue(struct char_data *ch, int spell_num) {
  int ch_class;
  struct prep_collection_spell_data *current = NULL;

  for (ch_class = 0; ch_class < NUM_CASTERS; ch_class++) {
    current = SPELL_PREP_QUEUE(ch, ch_class);
    do {
      if (current->spell == spell_num)
        return ch_class;
      
      /* transverse */
      current = current->next;
    } while (current);
  }
  
  return FALSE;
}

/* in: spell-number, class (of collection we want to access), metamagic, preparation time
 * out: preparation/collection spell data structure
 * create a new spell prep-queue entry, handles allocation of memory, etc */
struct prep_collection_spell_data *create_prep_queue_entry(int spell, int ch_class, int metamagic,
        int prep_time) {
  struct prep_collection_spell_data *prep_queue_data = NULL;

  CREATE(prep_queue_data, struct prep_collection_spell_data, 1);
  prep_queue_data->spell = spell;
  prep_queue_data->ch_class = ch_class;
  prep_queue_data->metamagic = metamagic;
  prep_queue_data->prep_time = prep_time;

  return prep_queue_data;
}

/* in: character, spell-number, class of collection we want, metamagic, prep time
 * out: preparation/collection spell data structure
 * add a spell to bottom of prep queue, example ch is memorizING a spell
 *   does NOT do any checking whether this is a 'legal' spell coming in  */
struct prep_collection_spell_data *spell_to_prep_queue(struct char_data *ch,
        int spell, int ch_class, int metamagic,  int prep_time) {
  
  /* hack note: for innate-magic we are storing the CIRCLE the spell belongs to
       in the queue */
  if (INNATE_MAGIC_CLASS(ch_class)) spell = compute_spells_circle(spell, ch_class, metamagic);
  
  struct prep_collection_spell_data *prep_queue_data = NULL;
  
  /* allocate memory, create entry with data */
  prep_queue_data = create_prep_queue_entry(spell, ch_class, metamagic, prep_time);

  /* put on bottom of appropriate class list */
  prep_queue_data->next = SPELL_PREP_QUEUE(ch, ch_class);
  
  return prep_queue_data;
}

/* in: character, spell-number, class of collection we need
 * out: copy of prearation/collection spell data containing entry
 * remove a spell from the spell_prep queue
 *   returns an instance of the spell item we found
 * example ch finished memorizing, also 'forgetting' a spell */
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

/* in: character
 * destroy a ch's spell prep queue, example ch logout */
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

/* in: character, spell-number
 * out: class of the respective collection
 * checks the ch's spell collection for a given spell_num  */
/* hack alert: innate-magic system is using the collection to store their
         'known' spells they select in 'study' */
/*the define is: INNATE_MAGIC_KNOWN(ch, spell_num) is_spell_in_collection(ch, spell_num)*/
int is_spell_in_collection(struct char_data *ch, int spell_num) {
  struct prep_collection_spell_data *current = NULL;
  int ch_class;

  for (ch_class = 0; ch_class < NUM_CASTERS; ch_class++) {        
    current = SPELL_COLLECTION(ch, ch_class);
    do {
            
      if (current->spell == spell_num)
        return ch_class;
      
      /* transverse */
      current = current->next;
      
    } while (current);
  }
  
  return FALSE;
}

/* in: spell-number, class (of collection we need), metamagic, prep-time
 * create a new spell collection entry */
struct prep_collection_spell_data *create_collection_entry(int spell, int ch_class, int metamagic,
        int prep_time) {
  struct prep_collection_spell_data *collection_data = NULL;

  CREATE(collection_data, struct prep_collection_spell_data, 1);
  collection_data->spell = spell;
  collection_data->ch_class = ch_class;
  collection_data->metamagic = metamagic;
  collection_data->prep_time = prep_time;

  return collection_data;
}

/* add a spell to bottom of collection, example ch memorized a spell */
/* hack alert: innate-magic system is using the collection to store their
     'known' spells they select in 'study' */
/*INNATE_MAGIC_TO_KNOWN(ch, spell, ch_class, metamagic, prep_time) *spell_to_collection(ch, spell, ch_class, metamagic, prep_time)*/
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
    /* hack alert: innate-magic system is using the collection to store their
         'known' spells that they select in 'study' */
    /*INNATE_MAGIC_FROM_KNOWN(ch, spell, ch_class) *spell_from_collection(ch, spell, ch_class)*/
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

/* in: char, spellnumber
 * out: true if success, false if failure
 * spell from queue to collection, example finished preparing a spell and now
 *  the spell belongs in your collection */
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

/* in: char, spellnumber
 * out: true if success, false if failure
 * spell from collection to queue, example finished casting a spell and now
 *  the spell belongs in your queue */
bool item_from_collection_to_queue(struct char_data *ch, int spell) {
    
  int class = is_spell_in_collection(ch, spell);
  
  if (class) {
    struct prep_collection_spell_data *item =
            spell_from_collection(ch, spell, class);
    spell_to_prep_queue(
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


/** START functions of general purpose, includes dated stuff we need to fix */

/*  in:
    out:
    */
int has_innate_magic_slot() {
  return 0;
}

/* in: character, class we need to check
 * out: highest circle access in given class, FALSE for fail
 *   turned this into a macro in header file: HIGHEST_CIRCLE(ch, class)
 *   special note: BONUS_CASTER_LEVEL includes prestige class bonuses */
int get_class_highest_circle(struct char_data *ch, int class) {
  /* npc's default to best chart */
  if (IS_NPC(ch)) {
    return (MAX(1, MIN(9, (GET_LEVEL(ch) + 1) / 2)));
  }
  /* if pc has no caster classes, he/she has no business here */
  if (!IS_CASTER(ch)) {
    return (FALSE);
  }
  /* no levels in this class? */
  if (!CLASS_LEVEL(ch, class)) {
    return FALSE;
  }
  int class_level = CLASS_LEVEL(ch, class) + BONUS_CASTER_LEVEL(ch, class);
  switch (class) {
    case CLASS_PALADIN:
      if (class_level < 6)
        return FALSE;
      else if (class_level < 10)
        return 1;
      else if (class_level < 12)
        return 2;
      else if (class_level < 15)
        return 3;
      else
        return 4;
    case CLASS_RANGER:
      if (class_level < 6)
        return FALSE;
      else if (class_level < 10)
        return 1;
      else if (class_level < 12)
        return 2;
      else if (class_level < 15)
        return 3;
      else
        return 4;
    case CLASS_BARD:
      if (class_level < 3)
        return FALSE;
      else if (class_level < 5)
        return 1;
      else if (class_level < 8)
        return 2;
      else if (class_level < 11)
        return 3;
      else if (class_level < 14)
        return 4;
      else if (class_level < 17)
        return 5;
      else
        return 6;
    case CLASS_SORCERER:
      return (MAX(1, (MIN(9, class_level / 2))));
    case CLASS_WIZARD:
      return (MAX(1, MIN(9, (class_level + 1) / 2)));
    case CLASS_DRUID:
      return (MAX(1, MIN(9, (class_level + 1) / 2)));
    case CLASS_CLERIC:
      return (MAX(1, MIN(9, (class_level + 1) / 2)));
    default:
      return FALSE;
  }
}

/**** UNDER CONSTRUCTION *****/
/* in: class we need to assign spell slots to
 * at bootup, we initialize class-data, which includes assignment
 *  of the class feats, with our new feat-based spell-slot system, we have
 *  to also assign ALL the spell slots as feats to the class-data, that is
 *  what this function handles... we take charts from constants.c and use the
 *  data to assign the feats...
 */
void assign_feat_spell_slots(int ch_class) {
  int level_counter = 0;
  int circle_counter = 0;
  int feat_index = 0;
  int slots_have[NUM_CIRCLES];
  int slots_had[NUM_CIRCLES];
  int slot_counter = 0;
  int i = 0;
  int difference = 0;
  
  /* lets initialize this */
  for (i = 0; i < NUM_CIRCLES; i++) {
    slots_have[i] = 0;
    slots_had[i]  = 0;
  }
  
  /* this is so we can find the index of the feats in structs.h */
  switch (ch_class) {
    case CLASS_WIZARD:
      feat_index = WIZ_SLT_0;
      break;
    case CLASS_SORCERER:
      feat_index = SRC_SLT_0;
      break;
    case CLASS_BARD:
      feat_index = BRD_SLT_0;
      break;      
    case CLASS_CLERIC:
      feat_index = CLR_SLT_0;
      break;      
    case CLASS_DRUID:
      feat_index = DRD_SLT_0;
      break;      
    case CLASS_RANGER:
      feat_index = RNG_SLT_0;
      break;
    case CLASS_PALADIN:
      feat_index = PLD_SLT_0;
      break;
    default:
      log("Error in assign_feat_spell_slots(), index default case for class.");
      break;
  }

  /* ENGINE */

  /* traverse level aspect of chart */
  for (level_counter = 1; level_counter < LVL_IMMORT; level_counter++) {
    /* traverse circle aspect of chart */
    for (circle_counter = 1; circle_counter < NUM_CIRCLES; circle_counter++) {
      /* store slots from chart into local array from this and prev level */
      switch (ch_class) {
        case CLASS_WIZARD:
          slots_have[circle_counter] = wizard_slots[level_counter][circle_counter];
          slots_had[circle_counter] = wizard_slots[level_counter-1][circle_counter];
          break;
        case CLASS_SORCERER:
          slots_have[circle_counter] = sorcerer_slots[level_counter][circle_counter];
          slots_had[circle_counter] = sorcerer_slots[level_counter-1][circle_counter];
          break;
        case CLASS_BARD:
          slots_have[circle_counter] = bard_slots[level_counter][circle_counter];
          slots_had[circle_counter] = bard_slots[level_counter-1][circle_counter];
          break;
        case CLASS_CLERIC:
          slots_have[circle_counter] = cleric_slots[level_counter][circle_counter];
          slots_had[circle_counter] = cleric_slots[level_counter-1][circle_counter];
          break;
        case CLASS_DRUID:
          slots_have[circle_counter] = druid_slots[level_counter][circle_counter];
          slots_had[circle_counter] = druid_slots[level_counter-1][circle_counter];
          break;
        case CLASS_RANGER:
          slots_have[circle_counter] = ranger_slots[level_counter][circle_counter];
          slots_had[circle_counter] = ranger_slots[level_counter-1][circle_counter];
          break;
        case CLASS_PALADIN:
          slots_have[circle_counter] = paladin_slots[level_counter][circle_counter];
          slots_had[circle_counter] = paladin_slots[level_counter-1][circle_counter];
          break;
        default:log("Error in assign_feat_spell_slots(), slots_have default case for class.");
          break;
      }
      
      difference = slots_have[circle_counter] - slots_had[circle_counter];
      
      if (difference) {
        for (slot_counter = 0; slot_counter < difference; slot_counter++) {
          feat_assignment(ch_class, (feat_index + circle_counter), TRUE,
                  level_counter, TRUE);          
        }
      }
      
    } /* circle counter */
  } /* level counter */

  /* all done! */
}

/* in: char data, spell number, class associated with spell, circle of spell
 * out: preparation time for spell number
 * given the above info, calculate how long this particular spell will take to
 * prepare..  this should take into account:
 *   circle
 *   class (arbitrary factor value)
 *   character's skills
 *   character feats 
 */
int compute_spells_prep_time(struct char_data *ch, int spellnum, int class,
        int circle) {
  int prep_time = 0;
  int bonus_time = 0;
  
  /* base prep time based on circle, etc */
  prep_time = BASE_PREP_TIME + (PREP_TIME_INTERVALS * (circle - 1));
  
  /* class factors */
  switch (class) {
    case CLASS_RANGER:
      prep_time *= RANGER_PREP_TIME_FACTOR;
      break;
    case CLASS_PALADIN:
      prep_time *= PALADIN_PREP_TIME_FACTOR;
      break;
    case CLASS_DRUID:
      prep_time *= DRUID_PREP_TIME_FACTOR;
      break;
    case CLASS_WIZARD:
      prep_time *= WIZ_PREP_TIME_FACTOR;
      break;
    case CLASS_CLERIC:
      prep_time *= CLERIC_PREP_TIME_FACTOR;
      break;
    case CLASS_SORCERER:
      prep_time *= SORC_PREP_TIME_FACTOR;
      break;
    case CLASS_BARD:
      prep_time *= BARD_PREP_TIME_FACTOR;
      break;    
  }
  
  /** calculate bonuses **/
  
  /*skills*/
  /* concentration */
  if (!IS_NPC(ch) && GET_ABILITY(ch, ABILITY_CONCENTRATION)) {
    bonus_time += compute_ability(ch, ABILITY_CONCENTRATION) / 4;
  }
  
  /*feats*/
  /* faster memorization, reduces prep time by a quarter */
  if (HAS_FEAT(ch, FEAT_FASTER_MEMORIZATION)) {
    bonus_time += prep_time / 4;
  }
  
  /*spells/affections*/
  /* song of focused mind, halves prep time */  
  if (affected_by_spell(ch, SKILL_SONG_OF_FOCUSED_MIND)) {
    bonus_time += prep_time / 2;
  }
  
  /** end bonus calculations **/
  
  prep_time -= bonus_time;
  
  if (prep_time <= 0)
    prep_time = 1;
  
  return (prep_time);
}

/* in: spellnum, class, metamagic
 * out: the circle this spell (now) belongs, FALSE (0) if failed
 * given above info, compute which circle this spell belongs to, this 'interesting'
 * set-up is due to a dated system that assigns spells by level, not circle
 * in addition we have metamagic that can modify the spell-circle as well */
int compute_spells_circle(int spellnum, int class, int metamagic) {
  int metamagic_mod = 0;
  int domain = 0;

  /* Here we add the circle changes resulting from metamagic use: */
  if (IS_SET(metamagic, METAMAGIC_QUICKEN)) {
    metamagic_mod += 4;
  }
  if (IS_SET(metamagic, METAMAGIC_MAXIMIZE)) {
    metamagic_mod += 3;
  }

  if (spellnum <= SPELL_RESERVED_DBC || spellnum >= NUM_SPELLS)
    return FALSE;

  switch (class) {
    
    case CLASS_BARD:
      switch (spell_info[spellnum].min_level[class]) {
        case 3:
        case 4:
          return 1 + metamagic_mod;
        case 5:
        case 6:
        case 7:
          return 2 + metamagic_mod;
        case 8:
        case 9:
        case 10:
          return 3 + metamagic_mod;
        case 11:
        case 12:
        case 13:
          return 4 + metamagic_mod;
        case 14:
        case 15:
        case 16:
          return 5 + metamagic_mod;
        case 17:
        case 18:
        case 19:
          return 6 + metamagic_mod;
          // don't use 20, that is reserved for epic!  case 20:
        default:
          return FALSE;
      }
      return 1 + metamagic_mod;
      
    case CLASS_SORCERER:
      return ((MAX(1, (spell_info[spellnum].min_level[class]) / 2)));
      
      /* pally can get confusing, just check out class.c to see what level
         they get their circles at in the spell_level function */      
    case CLASS_PALADIN:
      switch (spell_info[spellnum].min_level[class]) {
        case 6:
        case 7:
        case 8:
        case 9:
          return 1 + metamagic_mod;
        case 10:
        case 11:
          return 2 + metamagic_mod;
        case 12:
        case 13:
        case 14:
          return 3 + metamagic_mod;
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
          return 4 + metamagic_mod;
        default:
          return FALSE;
      }
      return 1 + metamagic_mod;
      
      /* rangers can get confusing, just check out class.c to see what level
         they get their circles at in the spell_level function */
    case CLASS_RANGER:
      switch (spell_info[spellnum].min_level[class]) {
        case 6:
        case 7:
        case 8:
        case 9:
          return 1 + metamagic_mod;
        case 10:
        case 11:
          return 2 + metamagic_mod;
        case 12:
        case 13:
        case 14:
          return 3 + metamagic_mod;
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
          return 4 + metamagic_mod;
        default:
          return FALSE;
      }
      return 1 + metamagic_mod;
      
    case CLASS_CLERIC:
      return ((int) ((MIN_SPELL_LVL(spellnum, CLASS_CLERIC, domain) + 1) / 2)) + metamagic_mod;
      
    case CLASS_WIZARD:
      return ((int) ((spell_info[spellnum].min_level[class] + 1) / 2)) + metamagic_mod;
      
    case CLASS_DRUID:
      return ((int) ((spell_info[spellnum].min_level[class] + 1) / 2)) + metamagic_mod;
      
    default:
      return FALSE;
  }

  return FALSE; 
}

/** END functions of general purpose, includes dated stuff we need to fix */


/***EOF***/
