/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\                                                             
/    Luminari Spell Prep System
/  Created By: Zusuk                                                           
\    File:     spell_prep.h                                                           
/    Handling spell preparation for all casting classes, memorization                                                           
\    system, queue, related commands, etc
/  Created on January 8, 2018, 3:27 PM                                                                                                                                                                                     
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#ifndef SPELL_PREP_H
#define	SPELL_PREP_H

#ifdef	__cplusplus
extern "C" {
#endif
    
    /** START structs **/
    /** END structs **/
    
    /** START defines **/
    /** END defines **/
    
    /** START functions **/
    /** END functions **/
    
    /** Start ACMD **/
    /** End ACMD **/
    
    /* work space / references */
    /*
// spell preparation queue and collection (prepared spells))
// this refers to items in the list of spells the ch is trying to prepare 
#define PREPARATION_QUEUE(ch, slot, cc)	(ch->player_specials->saved.prep_queue[slot][cc])
//this refers to preparation-time in a list that parallels the preparation_queue 
//    OLD system, this can be phased out  
#define PREP_TIME(ch, slot, cc)	(ch->player_specials->saved.prep_queue[slot][cc].prep_time)
// this refers to items in the list of spells the ch already has prepared (collection) 
#define PREPARED_SPELLS(ch, slot, cc)	(ch->player_specials->saved.collection[slot][cc])
#define SPELL_COLLECTION(ch, slot, cc)  PREPARED_SPELLS(ch, slot, cc)
// given struct entry, this is the appropriate class for this spell in relation to queue/collection  
#define PREP_CLASS(ch, slot, cc)	(ch->player_specials->saved.prep_queue[slot][cc].ch_class)
// bitvector of metamagic affecting this spell  
#define PREP_METAMAGIC(ch, slot, cc)	(ch->player_specials->saved.prep_queue[slot][cc].metamagic)
     */
    /*
// spell parapation, collection data - expanded for storing class and l-list data
struct prep_collection_spell_data {
  int spell; // spellnum of this spell in the collection 
  int ch_class;    // class that stored this spell in the collection
  int metamagic; // Bitvector of metamagic affecting this spell.
  
  struct prep_collection_spell_data *next; //linked-list
  
  // old system 
  int prep_time; // Remaining time for preparing this spell.
};    
*/

    /*
    int init_prep_list(struct spell_node **head, int spellnum);
    int insert_spell_prep_list(struct spell_node **head, int spellnum);
    void remove_spell_prep_list(struct spell_node **head, int spellnum);
    */
/*
int init_prep_list(struct spell_node **head, int spellnum) {
  *head = malloc(sizeof (struct spell_node));
  if (!*head) {
    log("Failed to init spell-prep linked list init_prep_list()");
    return 0;
  }

  (*head)->spellnum = spellnum;
  (*head)->next = NULL;

  return 1;
}

int insert_spell_prep_list(struct spell_node **head, int spellnum) {
  struct spell_node *current = *head;
  struct spell_node *tmp;

  do {
    tmp = current;
    current = current->next;
  } while (current);

  // create a new spell_node after tmp 
  struct spell_node *new = malloc(sizeof (struct spell_node));
  if (!new) {
    log("Failed to insert a new element in insert_spell_prep_list()\r\n");
    return 0;
  }
  new->next = NULL;
  new->spellnum = spellnum;

  tmp->next = new;

  return 1;
}

void remove_spell_prep_list(struct spell_node **head, int spellnum) {
  struct spell_node *current = *head;
  struct spell_node *prev = NULL;

  do {
    if (current->spellnum == spellnum) {
      break;
    }
    prev = current;
    current = current->next;
  } while (current);

  // if the first element 
  if (current == *head) {
    // reuse prev
    prev = *head;
    *head = current->next;
    free(prev);
    return;
  }

  // if the last element 
  if (current->next == NULL) {
    prev->next = NULL;
    free(current);
    return;
  }

  prev->next = current->next;
  free(current);
  return;
}

void print_prep_list(struct spell_node **head) {
  struct spell_node *current = *head;
  while (current) {
    log("current spellnum: %d, address: %p\r\n", current->spellnum, current);
    current = current->next;
  }
}

void reverse_prep_list(struct spell_node **head) {
  struct spell_node *current = *head, *newnext = NULL, *tmp;

  do {
    tmp = current->next;
    current->next = newnext;
    newnext = current;
    current = tmp;
  } while (current);

  *head = newnext;
  return;
}

void destroy_prep_list(struct spell_node **head) {
  struct spell_node *spell_node = *head;
  do {
    struct spell_node *tmp;
    tmp = spell_node;
    spell_node = spell_node->next;
    free(tmp);
  } while (spell_node);
}
*/    


#ifdef	__cplusplus
}
#endif

#endif	/* SPELL_PREP_H */

