/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\     Luminari Spell Prep System
/  File:       spell_prep.c
/  Created By: Zusuk
\  Header:     spell_prep.h
/    Handling spell preparation for all casting classes, memorization
\    system, queue, related commands, etc
/  Created on January 8, 2018, 3:27 PM
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

/* Extra credits:  Gicker for Sorcerer Bloodlines */

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
 * 
 *  TODO:
 *    *slots assignment by feats
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
#include "domains_schools.h"
/** END header files **/

/** START Globals **/

/* toggle for debug mode */
#define DEBUGMODE

/** END Globals **/

/* START linked list utility */

/* clear a ch's spell prep queue, example ch loadup */
void init_spell_prep_queue(struct char_data *ch)
{
  int ch_class = 0;
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    SPELL_PREP_QUEUE(ch, ch_class) = NULL;
}
/* clear a ch's innate magic queue, example ch loadup */
void init_innate_magic_queue(struct char_data *ch)
{
  int ch_class = 0;
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    INNATE_MAGIC(ch, ch_class) = NULL;
}
/* clear a ch's spell collection, example ch loadup */
void init_collection_queue(struct char_data *ch)
{
  int ch_class = 0;
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    SPELL_COLLECTION(ch, ch_class) = NULL;
}
/* clear a ch's spell known, example ch loadup */
void init_known_spells(struct char_data *ch)
{
  int ch_class = 0;
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    KNOWN_SPELLS(ch, ch_class) = NULL;
}

/* clear prep queue by class */
void clear_prep_queue_by_class(struct char_data *ch, int ch_class)
{
  if (!SPELL_PREP_QUEUE(ch, ch_class))
    return;
  do
  {
    struct prep_collection_spell_data *tmp;
    tmp = SPELL_PREP_QUEUE(ch, ch_class);
    SPELL_PREP_QUEUE(ch, ch_class) = SPELL_PREP_QUEUE(ch, ch_class)->next;
    free(tmp);
  } while (SPELL_PREP_QUEUE(ch, ch_class));
}
/* clear innate magic by class */
void clear_innate_magic_by_class(struct char_data *ch, int ch_class)
{
  if (!INNATE_MAGIC(ch, ch_class))
    return;
  do
  {
    struct innate_magic_data *tmp;
    tmp = INNATE_MAGIC(ch, ch_class);
    INNATE_MAGIC(ch, ch_class) = INNATE_MAGIC(ch, ch_class)->next;
    free(tmp);
  } while (INNATE_MAGIC(ch, ch_class));
}
/* clear collection by class */
void clear_collection_by_class(struct char_data *ch, int ch_class)
{
  if (!SPELL_COLLECTION(ch, ch_class))
    return;
  do
  {
    struct prep_collection_spell_data *tmp;
    tmp = SPELL_COLLECTION(ch, ch_class);
    SPELL_COLLECTION(ch, ch_class) = SPELL_COLLECTION(ch, ch_class)->next;
    free(tmp);
  } while (SPELL_COLLECTION(ch, ch_class));
}
/* clear known spells by class */
void clear_known_spells_by_class(struct char_data *ch, int ch_class)
{
  if (!KNOWN_SPELLS(ch, ch_class))
    return;
  do
  {
    struct known_spell_data *tmp;
    tmp = KNOWN_SPELLS(ch, ch_class);
    KNOWN_SPELLS(ch, ch_class) = KNOWN_SPELLS(ch, ch_class)->next;
    free(tmp);
  } while (KNOWN_SPELLS(ch, ch_class));
}

/* destroy the spell prep queue, example ch logout */
void destroy_spell_prep_queue(struct char_data *ch)
{
  int ch_class;
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    clear_prep_queue_by_class(ch, ch_class);
}
/* destroy the innate magic queue, example ch logout */
void destroy_innate_magic_queue(struct char_data *ch)
{
  int ch_class;
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    clear_innate_magic_by_class(ch, ch_class);
}
/* destroy the spell destroy_spell_collection, example ch logout */
void destroy_spell_collection(struct char_data *ch)
{
  int ch_class;
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    clear_collection_by_class(ch, ch_class);
}
/* destroy the known spells, example ch logout */
void destroy_known_spells(struct char_data *ch)
{
  int ch_class;
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    clear_known_spells_by_class(ch, ch_class);
}

/* save into ch pfile their spell-preparation queue, example ch saving */
void save_prep_queue_by_class(FILE *fl, struct char_data *ch, int class)
{
  struct prep_collection_spell_data *current = SPELL_PREP_QUEUE(ch, class);
  struct prep_collection_spell_data *next;
  for (; current; current = next)
  {
    next = current->next;
    fprintf(fl, "%d %d %d %d %d\n", class, current->spell, current->metamagic,
            current->prep_time, current->domain);
  }
}
/* save into ch pfile their innate magic queue, example ch saving */
void save_innate_magic_by_class(FILE *fl, struct char_data *ch, int class)
{
  struct innate_magic_data *current = INNATE_MAGIC(ch, class);
  struct innate_magic_data *next;
  for (; current; current = next)
  {
    next = current->next;
    fprintf(fl, "%d %d %d %d %d\n", class, current->circle, current->metamagic,
            current->prep_time, current->domain);
  }
}
/* save into ch pfile their spell-collection, example ch saving */
void save_collection_by_class(FILE *fl, struct char_data *ch, int class)
{
  struct prep_collection_spell_data *current = SPELL_COLLECTION(ch, class);
  struct prep_collection_spell_data *next;
  for (; current; current = next)
  {
    next = current->next;
    fprintf(fl, "%d %d %d %d %d\n", class, current->spell, current->metamagic,
            current->prep_time, current->domain);
  }
}
/* save into ch pfile their known spells, example ch saving */
void save_known_spells_by_class(FILE *fl, struct char_data *ch, int class)
{
  struct known_spell_data *current = KNOWN_SPELLS(ch, class);
  struct known_spell_data *next;
  for (; current; current = next)
  {
    next = current->next;
    fprintf(fl, "%d %d\n", class, current->spell);
  }
}

/* save into ch pfile their spell-preparation queue, example ch saving */
void save_spell_prep_queue(FILE *fl, struct char_data *ch)
{
  int ch_class;
  fprintf(fl, "PrQu:\n");
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    save_prep_queue_by_class(fl, ch, ch_class);
  fprintf(fl, "-1 -1 -1 -1 -1\n");
}
/* save into ch pfile their innate magic queue, example ch saving */
void save_innate_magic_queue(FILE *fl, struct char_data *ch)
{
  int ch_class;
  fprintf(fl, "InMa:\n");
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    save_innate_magic_by_class(fl, ch, ch_class);
  fprintf(fl, "-1 -1 -1 -1 -1\n");
}
/* save into ch pfile their spell collection, example ch saving */
void save_spell_collection(FILE *fl, struct char_data *ch)
{
  int ch_class;
  fprintf(fl, "Coll:\n");
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    save_collection_by_class(fl, ch, ch_class);
  fprintf(fl, "-1 -1 -1 -1 -1\n");
}
/* save into ch pfile their known spells, example ch saving */
void save_known_spells(FILE *fl, struct char_data *ch)
{
  int ch_class;
  fprintf(fl, "KnSp:\n");
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    save_known_spells_by_class(fl, ch, ch_class);
  fprintf(fl, "-1 -1\n");
}

/* give: ch, class, spellnum, and metamagic:
   return: true if we found/removed, false if we didn't find */
bool prep_queue_remove_by_class(struct char_data *ch, int class, int spellnum,
                                int metamagic)
{
  struct prep_collection_spell_data *current = SPELL_PREP_QUEUE(ch, class);
  struct prep_collection_spell_data *next;

  for (; current; current = next)
  {
    next = current->next;
    if (current->spell == spellnum && current->metamagic == metamagic)
    {
      prep_queue_remove(ch, current, class);
      return TRUE;
    }
  }

  return FALSE;
}
/* give: ch, class, spellnum, and metamagic:
   return: true if we found/removed, false if we didn't find */
bool innate_magic_remove_by_class(struct char_data *ch, int class, int circle,
                                  int metamagic)
{
  struct innate_magic_data *current = INNATE_MAGIC(ch, class);
  struct innate_magic_data *next;

  for (; current; current = next)
  {
    next = current->next;
    if (current->circle == circle && current->metamagic == metamagic)
    {
      innate_magic_remove(ch, current, class);
      return TRUE;
    }
  }

  return FALSE;
}
/* give: ch, class, spellnum, and metamagic:
   return: true if we found/removed, false if we didn't find */
bool collection_remove_by_class(struct char_data *ch, int class, int spellnum,
                                int metamagic)
{
  struct prep_collection_spell_data *current = SPELL_COLLECTION(ch, class);
  struct prep_collection_spell_data *next;

  for (; current; current = next)
  {
    next = current->next;
    if (current->spell == spellnum && current->metamagic == metamagic)
    {
      collection_remove(ch, current, class);
      return TRUE;
    }
  }

  return FALSE;
}
/* give: ch, class, spellnum, and metamagic:
   return: true if we found/removed, false if we didn't find */
bool known_spells_remove_by_class(struct char_data *ch, int class, int spellnum)
{
  struct known_spell_data *current = KNOWN_SPELLS(ch, class);
  struct known_spell_data *next;

  for (; current; current = next)
  {
    next = current->next;
    if (current->spell == spellnum)
    {
      known_spells_remove(ch, current, class);
      return TRUE;
    }
  }

  return FALSE;
}

/* remove a spell from a character's prep-queue(in progress) linked list */
void prep_queue_remove(struct char_data *ch, struct prep_collection_spell_data *entry,
                       int class)
{
  struct prep_collection_spell_data *temp;

  if (SPELL_PREP_QUEUE(ch, class) == NULL)
  {
    core_dump();
    return;
  }
  REMOVE_FROM_LIST(entry, SPELL_PREP_QUEUE(ch, class), next);
  free(entry);
}
/* remove a spell from a character's innate magic(in progress) linked list */
void innate_magic_remove(struct char_data *ch, struct innate_magic_data *entry,
                         int class)
{
  struct innate_magic_data *temp;

  if (INNATE_MAGIC(ch, class) == NULL)
  {
    core_dump();
    return;
  }
  REMOVE_FROM_LIST(entry, INNATE_MAGIC(ch, class), next);
  free(entry);
}
/* remove a spell from a character's collection (completed) linked list */
void collection_remove(struct char_data *ch, struct prep_collection_spell_data *entry,
                       int class)
{
  struct prep_collection_spell_data *temp;

  if (SPELL_COLLECTION(ch, class) == NULL)
  {
    core_dump();
    return;
  }
  REMOVE_FROM_LIST(entry, SPELL_COLLECTION(ch, class), next);
  free(entry);
}
/* remove a spell from a character's known spells linked list */
void known_spells_remove(struct char_data *ch, struct known_spell_data *entry,
                         int class)
{
  struct known_spell_data *temp;

  if (KNOWN_SPELLS(ch, class) == NULL)
  {
    core_dump();
    return;
  }
  REMOVE_FROM_LIST(entry, KNOWN_SPELLS(ch, class), next);
  free(entry);
}

/* add a spell to a character's prep-queue(in progress) linked list */
void prep_queue_add(struct char_data *ch, int ch_class, int spellnum, int metamagic,
                    int prep_time, int domain)
{
  struct prep_collection_spell_data *entry;

  CREATE(entry, struct prep_collection_spell_data, 1);
  entry->spell = spellnum;
  entry->metamagic = metamagic;
  entry->prep_time = prep_time;
  entry->domain = domain;
  entry->next = SPELL_PREP_QUEUE(ch, ch_class);
  SPELL_PREP_QUEUE(ch, ch_class) = entry;
}
/* add a spell to a character's innate magic (in progress) linked list */
void innate_magic_add(struct char_data *ch, int ch_class, int circle, int metamagic,
                      int prep_time, int domain)
{
  struct innate_magic_data *entry;

  CREATE(entry, struct innate_magic_data, 1);
  entry->circle = circle;
  entry->metamagic = metamagic;
  entry->prep_time = prep_time;
  entry->domain = domain;
  entry->next = INNATE_MAGIC(ch, ch_class);
  INNATE_MAGIC(ch, ch_class) = entry;
}
/* add a spell to a character's prep-queue(in progress) linked list */
void collection_add(struct char_data *ch, int ch_class, int spellnum, int metamagic,
                    int prep_time, int domain)
{
  struct prep_collection_spell_data *entry;

  CREATE(entry, struct prep_collection_spell_data, 1);
  entry->spell = spellnum;
  entry->metamagic = metamagic;
  entry->prep_time = prep_time;
  entry->domain = domain;
  entry->next = SPELL_COLLECTION(ch, ch_class);
  SPELL_COLLECTION(ch, ch_class) = entry;
}
/* add a spell to a character's known spells linked list */
bool known_spells_add(struct char_data *ch, int ch_class, int spellnum, bool loading)
{
  int circle = compute_spells_circle(ch_class, spellnum, METAMAGIC_NONE, DOMAIN_UNDEFINED);
  int caster_level = CLASS_LEVEL(ch, ch_class) + BONUS_CASTER_LEVEL(ch, ch_class);

  if (!loading)
  {
    switch (ch_class)
    {
    case CLASS_BARD:
      if (bard_known[caster_level][circle] -
              count_known_spells_by_circle(ch, ch_class, circle) <=
          0)
        return FALSE;
      break;
    case CLASS_SORCERER:
      if (compute_slots_by_circle(ch, ch_class, circle) -
              //sorcerer_known[caster_level][circle] -
              count_known_spells_by_circle(ch, ch_class, circle) <=
          0)
        return FALSE;
      break;
    case CLASS_PSIONICIST:
      if ((num_psionicist_powers_available(ch) - num_psionicist_powers_known(ch)) <= 0)
        return FALSE;
      break;
    }
  }

  struct known_spell_data *entry;

  CREATE(entry, struct known_spell_data, 1);
  entry->spell = spellnum;
  entry->metamagic = METAMAGIC_NONE;
  entry->prep_time = 0;
  entry->domain = DOMAIN_UNDEFINED;
  entry->next = KNOWN_SPELLS(ch, ch_class);
  KNOWN_SPELLS(ch, ch_class) = entry;

  return TRUE;
}

/* load from pfile into ch their spell-preparation queue, example ch login */
void load_spell_prep_queue(FILE *fl, struct char_data *ch)
{
  int spell_num, ch_class, metamagic, prep_time, domain, counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    ch_class = 0;
    spell_num = 0;
    metamagic = 0;
    prep_time = 0;
    domain = 0;

    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d", &ch_class, &spell_num, &metamagic, &prep_time,
           &domain);

    if (ch_class != -1)
      prep_queue_add(ch, ch_class, spell_num, metamagic, prep_time, domain);

    counter++;
  } while (counter < MAX_MEM && spell_num != -1);
}
/* load from pfile into ch their innate magic queue, example ch login */
void load_innate_magic_queue(FILE *fl, struct char_data *ch)
{
  int circle, ch_class, metamagic, prep_time, domain, counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    ch_class = 0;
    circle = 0;
    metamagic = 0;
    prep_time = 0;
    domain = 0;

    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d", &ch_class, &circle, &metamagic, &prep_time,
           &domain);

    if (ch_class != -1)
      innate_magic_add(ch, ch_class, circle, metamagic, prep_time, domain);

    counter++;
  } while (counter < MAX_MEM && circle != -1);
}
/* load from pfile into ch their spell collection, example ch login */
void load_spell_collection(FILE *fl, struct char_data *ch)
{
  int spell_num, ch_class, metamagic, prep_time, domain, counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    ch_class = 0;
    spell_num = 0;
    metamagic = 0;
    prep_time = 0;
    domain = 0;

    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d", &ch_class, &spell_num, &metamagic, &prep_time,
           &domain);

    if (ch_class != -1)
      collection_add(ch, ch_class, spell_num, metamagic, prep_time, domain);

    counter++;
  } while (counter < MAX_MEM && spell_num != -1);
}
/* load from pfile into ch their known spells, example ch login */
void load_known_spells(FILE *fl, struct char_data *ch)
{
  int spell_num, ch_class, counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    ch_class = 0;
    spell_num = 0;

    get_line(fl, line);
    sscanf(line, "%d %d", &ch_class, &spell_num);

    if (ch_class != -1)
      known_spells_add(ch, ch_class, spell_num, TRUE);

    counter++;
  } while (counter < MAX_MEM && ch_class != -1);
}

/* given a circle/class, count how many items of this circle in prep queue */
int count_circle_prep_queue(struct char_data *ch, int class, int circle)
{
  int this_circle = 0, counter = 0;
  struct prep_collection_spell_data *current = SPELL_PREP_QUEUE(ch, class);
  struct prep_collection_spell_data *next;

  for (; current; current = next)
  {
    next = current->next;
    this_circle = compute_spells_circle(class,
                                        current->spell,
                                        current->metamagic,
                                        current->domain);
    if (this_circle == circle)
      counter++;
  }

  return counter;
}
/* given a circle/class, count how many items of this circle in innate magic queue */
int count_circle_innate_magic(struct char_data *ch, int class, int circle)
{
  int counter = 0;
  struct innate_magic_data *current = INNATE_MAGIC(ch, class);
  struct innate_magic_data *next;

  for (; current; current = next)
  {
    next = current->next;
    if (circle == current->circle)
      counter++;
  }

  return counter;
}
/* given a circle/class, count how many items of this circle in the collection */
int count_circle_collection(struct char_data *ch, int class, int circle)
{
  int this_circle = 0, counter = 0;
  struct prep_collection_spell_data *current = SPELL_COLLECTION(ch, class);
  struct prep_collection_spell_data *next;

  for (; current; current = next)
  {
    next = current->next;
    this_circle = compute_spells_circle(class,
                                        current->spell,
                                        current->metamagic,
                                        current->domain);
    if (this_circle == circle)
      counter++;
  }

  return counter;
}
/* for innate magic-types:  counts how many spells you have of a given circle */
int count_known_spells_by_circle(struct char_data *ch, int class, int circle)
{
  int counter = 0;
  struct known_spell_data *current = KNOWN_SPELLS(ch, class);
  struct known_spell_data *next;

  /* we don't handle 0th circle */
  if (circle <= 0 || circle > TOP_CIRCLE)
    return 0;

  for (; current; current = next)
  {
    next = current->next;

    switch (class)
    {
    case CLASS_SORCERER:
      if (compute_spells_circle(class, current->spell, 0, 0) == circle &&
          !is_sorc_bloodline_spell(get_sorc_bloodline(ch), current->spell))
        counter++;
      break;
    case CLASS_BARD:
      if (compute_spells_circle(class, current->spell, 0, 0) == circle)
        counter++;
      break;
    } /*end switch*/
  }   /*end slot loop*/

  return counter;
}

/* for psionicists - how many total powers do you know */
int num_psionicist_powers_known(struct char_data *ch)
{
  int counter = 0;
  struct known_spell_data *current = KNOWN_SPELLS(ch, CLASS_PSIONICIST);
  struct known_spell_data *next;

  for (; current; current = next)
  {
    next = current->next;

    if (current->spell < PSIONIC_POWER_START || current->spell > PSIONIC_POWER_END)
      continue;

    counter++;

  }   /*end slot loop*/

  return counter;
}

/* returns the number of powers a psionicist may know in total from any and all power circles */
int num_psionicist_powers_available(struct char_data *ch)
{
  int i = 0, level = GET_PSIONIC_LEVEL(ch), num_powers = 1;

  num_powers += GET_REAL_INT_BONUS(ch);

  num_powers += HAS_FEAT(ch, FEAT_EXPANDED_KNOWLEDGE);

  for (i = 1; i <= level; i++)
    num_powers += 2;

  return num_powers; 

}

/* total # of slots consumed by circle X */
int count_total_slots(struct char_data *ch, int class, int circle)
{
  return (count_circle_collection(ch, class, circle) +
          count_circle_innate_magic(ch, class, circle) +
          count_circle_prep_queue(ch, class, circle));
}

/* in: ch, class, spellnum, metamagic, domain
   out: bool - is it in our prep queue? */
int is_spell_in_prep_queue(struct char_data *ch, int class, int spellnum,
                           int metamagic)
{
  struct prep_collection_spell_data *current = SPELL_PREP_QUEUE(ch, class);
  struct prep_collection_spell_data *next;

  for (; current; current = next)
  {
    next = current->next;
    if (current->spell == spellnum && current->metamagic == metamagic)
      return TRUE;
  }

  return FALSE;
}
/* in: ch, class, circle
   out: bool - is (circle) in our innate magic queue? */
bool is_in_innate_magic_queue(struct char_data *ch, int class, int circle)
{
  struct innate_magic_data *current = INNATE_MAGIC(ch, class);
  struct innate_magic_data *next;

  for (; current; current = next)
  {
    next = current->next;
    if (current->circle == circle)
      return TRUE;
  }

  return FALSE;
}
/* in: ch, class, spellnum, metamagic, domain
 *   search mode allows a general search with ONLY spellnum
   out: bool - is it in our collection? */
bool is_spell_in_collection(struct char_data *ch, int class, int spellnum,
                            int metamagic)
{
  struct prep_collection_spell_data *current = SPELL_COLLECTION(ch, class);
  struct prep_collection_spell_data *next;

  for (; current; current = next)
  {
    next = current->next;
    if (current->spell == spellnum && current->metamagic == metamagic)
    {
      return TRUE;
    }
  }

  return FALSE;
}
/* in: ch, spellnum, class (should only be bard/sorc so far)
   out: bool, is a known spell or not */
bool is_a_known_spell(struct char_data *ch, int class, int spellnum)
{
  /*DEBUG*/ /*
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    if (class == CLASS_SORCERER || class == CLASS_BARD) {
      send_to_char(ch, "Staff over-ride in is_a_known_spell()\r\n");
      return TRUE;
    }
  }
  */
  /*DEBUG*/

  switch (class)
  {
  /* bloodline system for sorcerer */
  case CLASS_SORCERER:
    if (is_sorc_bloodline_spell(get_sorc_bloodline(ch), spellnum))
      return TRUE;
    break;
  default:
    break;
  }

  struct known_spell_data *current = KNOWN_SPELLS(ch, class);
  struct known_spell_data *next;

  for (; current; current = next)
  {
    next = current->next;
    if (current->spell == spellnum)
    {
      return TRUE;
    }
  }

  return FALSE;
}

/* END linked list utility */

/* START bloodline code */

/* in: bloodline, spellnum
   out: bool - is this a bloodline spell? */
bool is_sorc_bloodline_spell(int bloodline, int spellnum)
{
  switch (bloodline)
  {
  case SORC_BLOODLINE_DRACONIC:
    switch (spellnum)
    {
    case SPELL_MAGE_ARMOR:
    case SPELL_RESIST_ENERGY:
    case SPELL_FLY:
      //case SPELL_FEAR: // Not implemented yet
    case SPELL_WIZARD_EYE: // replace with fear when imp'd
    case SPELL_TELEKINESIS:
      //case SPELL_FORM_OF_THE_DRAGON_I: // Not implemented yet
      //case SPELL_FORM_OF_THE_DRAGON_II: // Not implemented yet
      //case SPELL_FORM_OF_THE_DRAGON_III: // Not implemented yet
      //case SPELL_WISH: // Not implemented yet
    case SPELL_TRUE_SEEING:         // replace with form of dragon i when imp'd
    case SPELL_WAVES_OF_EXHAUSTION: // replace with form of dragon ii when imp'd
    case SPELL_MASS_DOMINATION:     // replace with form of dragon iii when imp'd
    case SPELL_POLYMORPH:           // replace with wish when imp'd
      return TRUE;
    }
    break;
  case SORC_BLOODLINE_ARCANE:
    switch (spellnum)
    {
    case SPELL_IDENTIFY:
    case SPELL_INVISIBLE:
    case SPELL_DISPEL_MAGIC:
      //case SPELL_DIMENSION_DOOR: // Not implemented yet
    case SPELL_MINOR_GLOBE: // replace with dimension door when implemented
    case SPELL_FEEBLEMIND:
    case SPELL_TRUE_SEEING:
    case SPELL_TELEPORT:
    case SPELL_POWER_WORD_STUN:
    case SPELL_TIMESTOP:
      return TRUE;
    }
    break;
  case SORC_BLOODLINE_FEY:
    switch (spellnum)
    {
    case SPELL_CHARM:
    case SPELL_HIDEOUS_LAUGHTER:
    case SPELL_DEEP_SLUMBER:
    case SPELL_POISON:
    case SPELL_FEEBLEMIND:
    case SPELL_TRUE_SEEING:
    case SPELL_PRISMATIC_SPRAY:
    case SPELL_IRRESISTIBLE_DANCE:
    case SPELL_POLYMORPH:
      return TRUE;
    }
    break;
  case SORC_BLOODLINE_UNDEAD:
    switch (spellnum)
    {
    case SPELL_CHILL_TOUCH:
    case SPELL_FALSE_LIFE:
    case SPELL_VAMPIRIC_TOUCH:
    case SPELL_ANIMATE_DEAD:
    case SPELL_WAVES_OF_FATIGUE:
    case SPELL_UNDEATH_TO_DEATH:
    case SPELL_FINGER_OF_DEATH:
    case SPELL_HORRID_WILTING:
    case SPELL_ENERGY_DRAIN:
      return TRUE;
    }
    break;
  }
  return FALSE;
}
/* given char, check their bloodline */
int get_sorc_bloodline(struct char_data *ch)
{
  if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_DRACONIC))
    return SORC_BLOODLINE_DRACONIC;
  if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_ARCANE))
    return SORC_BLOODLINE_ARCANE;
  if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_FEY))
    return SORC_BLOODLINE_FEY;
  if (HAS_FEAT(ch, FEAT_SORCERER_BLOODLINE_UNDEAD))
    return SORC_BLOODLINE_UNDEAD;

  return SORC_BLOODLINE_NONE;
}

/* END: bloodline code */

/* START helper functions */

/* in: spellnum, class, metamagic, domain(cleric)
 * out: the circle this spell (now) belongs, above num-circles if failed
 * given above info, compute which circle this spell belongs to
 * in addition we have metamagic that can modify the spell-circle as well */
int compute_spells_circle(int class, int spellnum, int metamagic, int domain)
{
  int metamagic_mod = 0;
  int spell_circle = 0;

  if (class != CLASS_PSIONICIST && (spellnum <= SPELL_RESERVED_DBC || spellnum >= NUM_SPELLS))
    return (NUM_CIRCLES + 1);
  else if (class == CLASS_PSIONICIST && (spellnum < PSIONIC_POWER_START || spellnum > PSIONIC_POWER_END))
    return (NUM_CIRCLES + 1);

  /* Here we add the circle changes resulting from metamagic use: */
  if (IS_SET(metamagic, METAMAGIC_QUICKEN))
    metamagic_mod += 4;
  if (IS_SET(metamagic, METAMAGIC_MAXIMIZE))
    metamagic_mod += 3;

  switch (class)
  {
  case CLASS_ALCHEMIST:
    switch (spell_info[spellnum].min_level[class])
    {
    case 1:
    case 2:
    case 3:
      return 1 + metamagic_mod;
    case 4:
    case 5:
    case 6:
      return 2 + metamagic_mod;
    case 7:
    case 8:
    case 9:
      return 3 + metamagic_mod;
    case 10:
    case 11:
    case 12:
      return 4 + metamagic_mod;
    case 13:
    case 14:
    case 15:
      return 5 + metamagic_mod;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
      return 6 + metamagic_mod;
    /* level 20 reserved for epic spells */
    default:
      return (NUM_CIRCLES + 1);
    }
    break;
  case CLASS_BARD:
    switch (spell_info[spellnum].min_level[class])
    {
    case 1:
    case 2:
    case 3:
      return 1 + metamagic_mod;
    case 4:
    case 5:
    case 6:
      return 2 + metamagic_mod;
    case 7:
    case 8:
    case 9:
      return 3 + metamagic_mod;
    case 10:
    case 11:
    case 12:
      return 4 + metamagic_mod;
    case 13:
    case 14:
    case 15:
      return 5 + metamagic_mod;
    case 16:
    case 17:
    case 18:
    case 19:
      return 6 + metamagic_mod;
    /* level 20 reserved for epic spells */
    default:
      return (NUM_CIRCLES + 1);
    }
    break;
  case CLASS_PALADIN:
  case CLASS_BLACKGUARD:
    switch (spell_info[spellnum].min_level[class])
    {
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
      return (NUM_CIRCLES + 1);
    }
    break;
  case CLASS_RANGER:
    switch (spell_info[spellnum].min_level[class])
    {
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
      return (NUM_CIRCLES + 1);
    }
    break;
  case CLASS_SORCERER:
    spell_circle = spell_info[spellnum].min_level[class] / 2;
    spell_circle += metamagic_mod;
    if (spell_circle > TOP_CIRCLE)
    {
      return (NUM_CIRCLES + 1);
    }
    return (MAX(1, spell_circle));
  case CLASS_CLERIC:
    /* MIN_SPELL_LVL will determine whether domain has a lower level version
           of the spell */
    spell_circle = (MIN_SPELL_LVL(spellnum, class, domain) + 1) / 2;
    spell_circle += metamagic_mod;
    if (spell_circle > TOP_CIRCLE)
    {
      return (NUM_CIRCLES + 1);
    }
    return (MAX(1, spell_circle));
  case CLASS_WIZARD:
    spell_circle = (spell_info[spellnum].min_level[class] + 1) / 2;
    spell_circle += metamagic_mod;
    if (spell_circle > TOP_CIRCLE)
    {
      return (NUM_CIRCLES + 1);
    }
    return (MAX(1, spell_circle));
  case CLASS_DRUID:
    spell_circle = (spell_info[spellnum].min_level[class] + 1) / 2;
    spell_circle += metamagic_mod;
    if (spell_circle > TOP_CIRCLE)
    {
      return (NUM_CIRCLES + 1);
    }
    return (MAX(1, spell_circle));
  case CLASS_PSIONICIST:
    spell_circle = (spell_info[spellnum].min_level[class] + 1) / 2;
    spell_circle += metamagic_mod;
    if (spell_circle > TOP_CIRCLE)
    {
      return (NUM_CIRCLES + 1);
    }
    return (MAX(1, spell_circle));
  default:
    return (NUM_CIRCLES + 1);
  }
  return (NUM_CIRCLES + 1);
}

/* in: spellnum, class, metamagic
 * out: the circle this power (now) belongs, above num-circles if failed
 * given above info, compute which circle this spell belongs to
 * in addition we have metamagic that can modify the spell-circle as well */
int compute_powers_circle(int class, int spellnum, int metamagic)
{
  int metamagic_mod = 0;
  int spell_circle = 0;

  if (spellnum < PSIONIC_POWER_START || spellnum > PSIONIC_POWER_END)
    return (NUM_CIRCLES + 1);

  /* Here we add the circle changes resulting from metamagic use: */
  /*
  if (IS_SET(metamagic, METAMAGIC_QUICKEN))
    metamagic_mod += 4;
  if (IS_SET(metamagic, METAMAGIC_MAXIMIZE))
    metamagic_mod += 3;
  */

  switch (class)
  {
  case CLASS_PSIONICIST:
    spell_circle = (spell_info[spellnum].min_level[class] + 1) / 2;
    spell_circle += metamagic_mod;
    if (spell_circle > TOP_CIRCLE)
    {
      return (NUM_CIRCLES + 1);
    }
    return (MAX(1, spell_circle));
  default:
    return (NUM_CIRCLES + 1);
  }
  return (NUM_CIRCLES + 1);
}

/* in: character, class we need to check
 * out: highest circle access in given class, FALSE for fail
 *   turned this into a macro in header file: HIGHEST_CIRCLE(ch, class)
 *   special note: BONUS_CASTER_LEVEL includes prestige class bonuses */
int get_class_highest_circle(struct char_data *ch, int class)
{

  /* npc's default to best chart */
  if (IS_NPC(ch))
    return (MAX(1, MIN(9, (GET_LEVEL(ch) + 1) / 2)));
  /* if pc has no caster classes, he/she has no business here */
  if (!IS_CASTER(ch) && !IS_PSIONIC(ch))
    return (FALSE);
  /* no levels in this class? */
  if (!CLASS_LEVEL(ch, class))
    return FALSE;

  int class_level = CLASS_LEVEL(ch, class) + BONUS_CASTER_LEVEL(ch, class);
  switch (class)
  {
  case CLASS_PALADIN:
  case CLASS_BLACKGUARD:
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
    if (class_level < 4)
      return 1;
    else if (class_level < 7)
      return 2;
    else if (class_level < 10)
      return 3;
    else if (class_level < 13)
      return 4;
    else if (class_level < 16)
      return 5;
    else
      return 6;
  case CLASS_ALCHEMIST:
    if (class_level < 4)
      return 1;
    else if (class_level < 7)
      return 2;
    else if (class_level < 10)
      return 3;
    else if (class_level < 13)
      return 4;
    else if (class_level < 16)
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
  case CLASS_PSIONICIST:
    return (MAX(1, MIN(9, (class_level + 1) / 2)));
  default:
    return FALSE;
  }
}

/* are we in a state that allows us to prep spells? */
/* define: READY_TO_PREP(ch, class) */
bool ready_to_prep_spells(struct char_data *ch, int class)
{

  switch (class)
  {
  case CLASS_WIZARD: /* wizards need to study a book or scroll */
    if (SPELL_PREP_QUEUE(ch, class))
      if (!spellbook_ok(ch, SPELL_PREP_QUEUE(ch, class)->spell, CLASS_WIZARD, FALSE))
        return FALSE;
    break;
  }

  /* posiiton / fighting */
  if (GET_POS(ch) != POS_RESTING)
    return FALSE;
  if (FIGHTING(ch))
    return FALSE;

  /* debuffs */
  if (AFF_FLAGGED(ch, AFF_STUN))
    return FALSE;
  if (AFF_FLAGGED(ch, AFF_PARALYZED))
    return FALSE;
  if (AFF_FLAGGED(ch, AFF_GRAPPLED))
    return FALSE;
  if (AFF_FLAGGED(ch, AFF_NAUSEATED))
    return FALSE;
  if (AFF_FLAGGED(ch, AFF_CONFUSED))
    return FALSE;
  if (AFF_FLAGGED(ch, AFF_DAZED))
    return FALSE;
  if (AFF_FLAGGED(ch, AFF_PINNED))
    return FALSE;

  /* made it! */
  return TRUE;
}

/* set the preparing state of the char, this has actually become
   redundant because of events, but we still have it
 * returns TRUE if successfully set something, false if not */
void set_preparing_state(struct char_data *ch, int class, bool state)
{
  (ch)->char_specials.preparing_state[class] = state;
}

/* preparing state right now? */
bool is_preparing(struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_CLASSES; i++)
    if ((ch)->char_specials.preparing_state[i])
      return TRUE;

  return FALSE;
}

/* sets prep-state as TRUE, and starts the preparing-event */
/* we check the queue for top spell for first timer */
void start_prep_event(struct char_data *ch, int class)
{
  char buf[50] = {'\0'};

  switch (class)
  {
  case CLASS_SORCERER:
  case CLASS_BARD:
    if (!INNATE_MAGIC(ch, class))
    {
      send_to_char(ch, "You have nothing in your innate magic queue for this class to "
                       "prepare!\r\n");
      return;
    }
    break;
  default:
    if (!SPELL_PREP_QUEUE(ch, class))
    {
      send_to_char(ch, "You have nothing in your preparation queue for this class to "
                       "prepare!\r\n");
      return;
    }
    break;
  }
  set_preparing_state(ch, class, TRUE);
  if (!char_has_mud_event(ch, ePREPARATION))
  {
    snprintf(buf, sizeof(buf), "%d", class); /* carry our class as the svar */
    NEW_EVENT(ePREPARATION, ch, strdup(buf), (1 * PASSES_PER_SEC));
  }
}

/* stop the preparing event and sets the state as false */
void stop_prep_event(struct char_data *ch, int class)
{
  set_preparing_state(ch, class, FALSE);
  if (char_has_mud_event(ch, ePREPARATION))
  {
    event_cancel_specific(ch, ePREPARATION);
  }
  switch (class)
  {
  case CLASS_SORCERER:
  case CLASS_BARD:
    if (INNATE_MAGIC(ch, class))
      reset_preparation_time(ch, class);
    break;
  default:
    if (SPELL_PREP_QUEUE(ch, class))
      reset_preparation_time(ch, class);
    break;
  }
}

/* stops all preparation irregardless of class */
void stop_all_preparations(struct char_data *ch)
{
  int class = 0;
  for (class = 0; class < NUM_CLASSES; class ++)
    stop_prep_event(ch, class);
}

/* does ch level qualify them for this particular spell?
     includes domain system for clerics  */
bool is_min_level_for_spell(struct char_data *ch, int class, int spellnum)
{
  int min_level = 0;

  switch (class)
  {
  case CLASS_CLERIC: /*domain system!*/
    min_level = MIN(MIN_SPELL_LVL(spellnum, CLASS_CLERIC, GET_1ST_DOMAIN(ch)),
                    MIN_SPELL_LVL(spellnum, CLASS_CLERIC, GET_2ND_DOMAIN(ch)));
    if ((BONUS_CASTER_LEVEL(ch, class) + CLASS_LEVEL(ch, CLASS_CLERIC)) <
        min_level)
    {
      return FALSE;
    }
    break;
  default:
    if ((BONUS_CASTER_LEVEL(ch, class) + CLASS_LEVEL(ch, class)) <
        spell_info[spellnum].min_level[class])
    {
      return FALSE;
    }
    return TRUE;
  }
  return TRUE;
}

/* TODO: convert to feat system, construction directly below this
     function */
/* in: character, respective class to check 
 * out: returns # of total slots based on class-level and stat bonus
     of given circle */
int compute_slots_by_circle(struct char_data *ch, int class, int circle)
{
  int spell_slots = 0, i = 0;
  int class_level = CLASS_LEVEL(ch, class);

  /* they don't even have access to this circle */
  if (get_class_highest_circle(ch, class) < circle)
    return FALSE;

  /* includes specials like prestige class */
  class_level += BONUS_CASTER_LEVEL(ch, class);

  switch (class)
  {
  case CLASS_RANGER:
    spell_slots += spell_bonus[GET_WIS(ch)][circle];
    spell_slots += ranger_slots[class_level][circle];
    break;
  case CLASS_PALADIN:
  case CLASS_BLACKGUARD:
    spell_slots += spell_bonus[GET_CHA(ch)][circle];
    spell_slots += paladin_slots[class_level][circle];
    break;
  case CLASS_CLERIC:
    spell_slots += spell_bonus[GET_WIS(ch)][circle];
    spell_slots += cleric_slots[class_level][circle];
    break;
  case CLASS_DRUID:
    spell_slots += spell_bonus[GET_WIS(ch)][circle];
    spell_slots += druid_slots[class_level][circle];
    break;
  case CLASS_WIZARD:
    spell_slots += spell_bonus[GET_INT(ch)][circle];
    spell_slots += wizard_slots[class_level][circle];
    break;
  case CLASS_SORCERER:
    spell_slots += spell_bonus[GET_CHA(ch)][circle];
    spell_slots += sorcerer_known[class_level][circle];
    if (circle != 0 && HAS_REAL_FEAT(ch, FEAT_NEW_ARCANA))
    {
      for (i = 0; i < 4; i++)
      {
        if (NEW_ARCANA_SLOT(ch, i) == circle)
          spell_slots++;
      }
    }
    break;
  case CLASS_BARD:
    spell_slots += spell_bonus[GET_CHA(ch)][circle];
    spell_slots += bard_slots[class_level][circle];
    break;
  case CLASS_ALCHEMIST:
    spell_slots += spell_bonus[GET_INT(ch)][circle];
    spell_slots += alchemist_slots[class_level][circle];
    break;

  default:
    break;
  }

  if (spell_slots <= 0)
    return FALSE;

  return spell_slots;
}

/**** UNDER CONSTRUCTION *****/
/* in: class we need to assign spell slots to
 * at bootup, we initialize class-data, which includes assignment
 *  of the class feats, with our new feat-based spell-slot system, we have
 *  to also assign ALL the spell slots as feats to the class-data, that is
 *  what this function handles... we take charts from constants.c and use the
 *  data to assign the feats...
 */
void assign_feat_spell_slots(int ch_class)
{

  /*UNFINISHED*/
  return;
  /*UNFINISHED*/

  int level_counter = 0;
  int circle_counter = 0;
  int feat_index = 0;
  int slots_have[NUM_CIRCLES];
  int slots_had[NUM_CIRCLES];
  int slot_counter = 0;
  int i = 0;
  int difference = 0;

  /* lets initialize this */
  for (i = 0; i < NUM_CIRCLES; i++)
  {
    slots_have[i] = 0;
    slots_had[i] = 0;
  }

  /* this is so we can find the index of the feats in structs.h */
  switch (ch_class)
  {
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
  case CLASS_BLACKGUARD:
    feat_index = BKG_SLT_0;
    break;
  case CLASS_ALCHEMIST:
    feat_index = ALC_SLT_0;
    break;
  default:
    log("Error in assign_feat_spell_slots(), index default case for class.");
    break;
  }

  /* ENGINE */

  /* traverse level aspect of chart */
  for (level_counter = 1; level_counter < LVL_IMMORT; level_counter++)
  {
    /* traverse circle aspect of chart */
    for (circle_counter = 1; circle_counter < NUM_CIRCLES; circle_counter++)
    {
      /* store slots from chart into local array from this and prev level */
      switch (ch_class)
      {
      case CLASS_WIZARD:
        slots_have[circle_counter] = wizard_slots[level_counter][circle_counter];
        slots_had[circle_counter] = wizard_slots[level_counter - 1][circle_counter];
        break;
      case CLASS_SORCERER:
        slots_have[circle_counter] = sorcerer_slots[level_counter][circle_counter];
        slots_had[circle_counter] = sorcerer_slots[level_counter - 1][circle_counter];
        break;
      case CLASS_BARD:
        slots_have[circle_counter] = bard_slots[level_counter][circle_counter];
        slots_had[circle_counter] = bard_slots[level_counter - 1][circle_counter];
        break;
      case CLASS_CLERIC:
        slots_have[circle_counter] = cleric_slots[level_counter][circle_counter];
        slots_had[circle_counter] = cleric_slots[level_counter - 1][circle_counter];
        break;
      case CLASS_DRUID:
        slots_have[circle_counter] = druid_slots[level_counter][circle_counter];
        slots_had[circle_counter] = druid_slots[level_counter - 1][circle_counter];
        break;
      case CLASS_RANGER:
        slots_have[circle_counter] = ranger_slots[level_counter][circle_counter];
        slots_had[circle_counter] = ranger_slots[level_counter - 1][circle_counter];
        break;
      case CLASS_PALADIN:
      case CLASS_BLACKGUARD:
        slots_have[circle_counter] = paladin_slots[level_counter][circle_counter];
        slots_had[circle_counter] = paladin_slots[level_counter - 1][circle_counter];
        break;
      case CLASS_ALCHEMIST:
        slots_have[circle_counter] = alchemist_slots[level_counter][circle_counter];
        slots_had[circle_counter] = alchemist_slots[level_counter - 1][circle_counter];
        break;
      default:
        log("Error in assign_feat_spell_slots(), slots_have default case for class.");
        break;
      }

      difference = slots_have[circle_counter] - slots_had[circle_counter];

      if (difference)
      {
        for (slot_counter = 0; slot_counter < difference; slot_counter++)
        {
          feat_assignment(ch_class, (feat_index + circle_counter), TRUE,
                          level_counter, TRUE);
        }
      }

    } /* circle counter */
  }   /* level counter */

  /* all done! */
}
/**** END CONSTRUCTION ZONE *****/

/* this function is our connection between the casting system and spell preparation
   system, we are checking -all- our spell prep systems to see if we have the 
   given spell, if we do:
     gen prep system: extract from collection and move to prep queue
     innate magic system:  put the proper circle in innate magic queue
 * we check general prep system first, THEN innate magic system  */
int spell_prep_gen_extract(struct char_data *ch, int spellnum, int metamagic)
{
  int ch_class = CLASS_UNDEFINED, prep_time = 99,
      circle = TOP_CIRCLE + 1, is_domain = FALSE;

  /* go through all the classes checking our collection */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
  {
    if (is_spell_in_collection(ch, ch_class, spellnum, metamagic))
    {

      /*extract from collection*/
      if (!collection_remove_by_class(ch, ch_class, spellnum, metamagic))
        return CLASS_UNDEFINED; /* failed somehow */

      /*place in queue*/
      is_domain = is_domain_spell_of_ch(ch, spellnum);
      circle = compute_spells_circle(ch_class, spellnum, metamagic, is_domain);
      prep_time = compute_spells_prep_time(ch, ch_class, circle, is_domain);
      prep_queue_add(ch, ch_class, spellnum, metamagic, prep_time, is_domain);

      return (ch_class);
    }
  }

  /* nothing yet? go through our innate magic system now! */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
  {
    is_domain = is_domain_spell_of_ch(ch, spellnum);
    circle = /* computes adjustment to circle via metamagic */
        compute_spells_circle(ch_class, spellnum, metamagic, is_domain);
    if (is_a_known_spell(ch, ch_class, spellnum) &&
        (compute_slots_by_circle(ch, ch_class, circle) -
             count_total_slots(ch, ch_class, circle) >
         0))
    {

      /*place circle in innate magic queue*/
      prep_time = compute_spells_prep_time(ch, ch_class, circle, is_domain);
      innate_magic_add(ch, ch_class, circle, metamagic, prep_time, is_domain);

      return ch_class;
    }
  }

  /* FAILED! */
  return CLASS_UNDEFINED;
}

/* this function is our connection between the casting system and spell preparation
   system, we are checking -all- our spell prep systems to see if we have the 
   given spell, if we do, returns class, otherwise undefined-class 
   we are checking our spell-prep system, THEN innate magic system */
int spell_prep_gen_check(struct char_data *ch, int spellnum, int metamagic)
{

  // Staff can cast any spell at any time
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return true;

  int class = CLASS_UNDEFINED;

  /* go through all the classes checking our collection */
  for (class = 0; class < NUM_CLASSES; class ++)
  {
    if (is_spell_in_collection(ch, class, spellnum, metamagic))
      return class;
  }

  /* nothing yet? go through our innate magic system now! */
  int circle_of_this_spell = TOP_CIRCLE + 1;
  for (class = 0; class < NUM_CLASSES; class ++)
  {
    circle_of_this_spell = /* computes adjustment to circle via metamagic */
        compute_spells_circle(class, spellnum, metamagic, DOMAIN_UNDEFINED);
    if (is_a_known_spell(ch, class, spellnum) &&
        (compute_slots_by_circle(ch, class, circle_of_this_spell) -
             count_total_slots(ch, class, circle_of_this_spell) >
         0))
    {

      return class;
    }
  }

  /* FAILED! */
  return CLASS_UNDEFINED;
}

/* in: character, class of the queue you want to work with
 * traverse the prep queue and print out the details
 * since the prep queue does not need any organizing, this should be fairly
 * simple */
void print_prep_queue(struct char_data *ch, int ch_class)
{
  char buf[MAX_INPUT_LENGTH];
  int line_length = 80, total_time = 0;
  struct prep_collection_spell_data *current = SPELL_PREP_QUEUE(ch, ch_class);
  struct prep_collection_spell_data *next;

  /* build a nice heading */
  *buf = '\0';
  snprintf(buf, sizeof(buf), "\tYPreparation Queue for %s\tC", class_names[ch_class]);
  send_to_char(ch, "\tC");
  text_line(ch, buf, line_length, '-', '-');
  send_to_char(ch, "\tn");

  /* easy out */
  if (!SPELL_PREP_QUEUE(ch, ch_class))
  {
    send_to_char(ch, "There is nothing in your preparation queue!\r\n");
    /* build a nice closing */
    *buf = '\0';
    send_to_char(ch, "\tC");
    text_line(ch, buf, line_length, '-', '-');
    send_to_char(ch, "\tn");
    return;
  }

  /* traverse and print */
  *buf = '\0';
  for (; current; current = next)
  {
    next = current->next;
    int spell_circle = compute_spells_circle(ch_class,
                                             current->spell,
                                             current->metamagic,
                                             current->domain);
    int prep_time = current->prep_time;
    total_time += prep_time;
    send_to_char(ch, " \tW%20s\tn \tc[\tn%d%s circle\tc]\tn \tc[\tn%2d seconds\tc]\tn %s%s %s\r\n",
                 spell_info[current->spell].name,
                 spell_circle,
                 (spell_circle == 1) ? "st" : (spell_circle == 2) ? "nd" : (spell_circle == 3) ? "rd" : "th",
                 prep_time,
                 (IS_SET(current->metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
                 (current->domain ? domain_list[current->domain].name : ""));
  } /* end transverse */

  /* build a nice closing */
  *buf = '\0';
  snprintf(buf, sizeof(buf), "\tYTotal Preparation Time Remaining: \tW%d\tC", total_time);
  send_to_char(ch, "\tC");
  text_line(ch, buf, line_length, '-', '-');
  send_to_char(ch, "\tn");
  if (ch_class == CLASS_WIZARD)
    send_to_char(ch, "Wizards must have their spellbook in their inventory to memorize spells.\r\n");

  /* all done */
  return;
}

/* in: character, class of the queue you want to work with
 * traverse the innate magic and print out the details */
void print_innate_magic_queue(struct char_data *ch, int ch_class)
{
  char buf[MAX_INPUT_LENGTH];
  int line_length = 80, total_time = 0;
  struct innate_magic_data *current = INNATE_MAGIC(ch, ch_class);
  struct innate_magic_data *next;

  /* build a nice heading */
  *buf = '\0';
  snprintf(buf, sizeof(buf), "\tYInnate Magic Queue for %s\tC", class_names[ch_class]);
  send_to_char(ch, "\tC");
  text_line(ch, buf, line_length, '-', '-');
  send_to_char(ch, "\tn");

  /* easy out */
  if (!INNATE_MAGIC(ch, ch_class))
  {
    send_to_char(ch, "There is nothing in your innate magic queue!\r\n");
    /* build a nice closing */
    *buf = '\0';
    send_to_char(ch, "\tC");
    text_line(ch, buf, line_length, '-', '-');
    send_to_char(ch, "\tn");
    return;
  }

  /* traverse and print */
  *buf = '\0';
  for (; current; current = next)
  {
    next = current->next;
    int prep_time = current->prep_time;
    total_time += prep_time;
    send_to_char(ch, " \tc[\tWcircle-slot: \tn%d%s\tc]\tn \tc[\tn%2d seconds\tc]\tn %s%s %s\r\n",
                 current->circle,
                 (current->circle == 1) ? "st" : (current->circle == 2) ? "nd" : (current->circle == 3) ? "rd" : "th",
                 prep_time,
                 (IS_SET(current->metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
                 (current->domain ? domain_list[current->domain].name : ""));

  } /* end transverse */

  /* build a nice closing */
  *buf = '\0';
  snprintf(buf, sizeof(buf), "\tYTotal Preparation Time Remaining: \tW%d\tC", total_time);
  send_to_char(ch, "\tC");
  text_line(ch, buf, line_length, '-', '-');
  send_to_char(ch, "\tn");

  /* all done */
  return;
}

/* our display for our prepared spells aka collection, the level of complexity
   of our output will determine how complex this function is ;p */
void print_collection(struct char_data *ch, int ch_class)
{
  char buf[MAX_INPUT_LENGTH];
  int line_length = 80, high_circle = get_class_highest_circle(ch, ch_class);
  int counter = 0, this_circle = 0;

  /* build a nice heading */
  *buf = '\0';
  snprintf(buf, sizeof(buf), "\tY%s Collection for %s\tC", ch_class == CLASS_ALCHEMIST ? "Extract" : "Spell", class_names[ch_class]);
  send_to_char(ch, "\tC");
  text_line(ch, buf, line_length, '-', '-');
  send_to_char(ch, "\tn");

  /* easy out */
  if (!SPELL_COLLECTION(ch, ch_class) || high_circle <= 0)
  {
    send_to_char(ch, "There is nothing in your spell collection!\r\n");
    return;
  }

  /* loop for circles */
  for (high_circle; high_circle >= 0; high_circle--)
  {
    counter = 0;
    struct prep_collection_spell_data *current = SPELL_COLLECTION(ch, ch_class);
    struct prep_collection_spell_data *next;

    /* traverse and print */
    if (SPELL_COLLECTION(ch, ch_class))
    {
      for (; current; current = next)
      {
        next = current->next;
        /* check if our circle matches this entry */
        this_circle = compute_spells_circle(
            ch_class,
            current->spell,
            current->metamagic,
            current->domain);
        if (high_circle == this_circle)
        { /* print! */
          counter++;
          if (counter == 1)
          {
            send_to_char(ch, "\tY%d%s:\tn \tW%20s\tn %12s%12s%s%13s%s\r\n",
                         high_circle,
                         (high_circle == 1) ? "st" : (high_circle == 2) ? "nd" : (high_circle == 3) ? "rd" : "th",
                         spell_info[current->spell].name,
                         (IS_SET(current->metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
                         (IS_SET(current->metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
                         current->domain ? "\tc[\tn" : "",
                         current->domain ? domain_list[current->domain].name : "",
                         current->domain ? "\tc]\tn" : "");
          }
          else
          {
            send_to_char(ch, "%4s \tW%20s\tn %12s%12s%s%13s%s\r\n",
                         "    ",
                         spell_info[current->spell].name,
                         (IS_SET(current->metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
                         (IS_SET(current->metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
                         current->domain ? "\tc[\tn" : "",
                         current->domain ? domain_list[current->domain].name : "",
                         current->domain ? "\tc]\tn" : "");
          }
        }
      } /*end collection*/
    }
  } /*end circle loop*/
}

/* display avaialble slots based on what is in the queue/collection, and other
   variables */
void display_available_slots(struct char_data *ch, int class)
{
  int slot, num_circles = 0, slot_array[NUM_CIRCLES], last_slot = 0,
            highest_circle = get_class_highest_circle(ch, class),
            line_length = 80;
  bool printed = FALSE, found_slot = FALSE;
  char buf[MAX_INPUT_LENGTH];

  /* fill our slot_array[] with # available slots */
  for (slot = 0; slot <= highest_circle; slot++)
  {
    found_slot = FALSE;

    if ((slot_array[slot] = compute_slots_by_circle(ch, class, slot) -
                            count_total_slots(ch, class, slot)) > 0)
      found_slot = TRUE;

    if (found_slot)
    {
      last_slot = slot; /* how do we punctuate the end */
      num_circles++;    /* keep track # circles we need to print */
    }
  }

  send_to_char(ch, "\tYAvailable:");

  for (slot = 0; slot <= highest_circle; slot++)
  {
    if (slot_array[slot] > 0)
    {
      printed = TRUE;
      send_to_char(ch, " \tW%d\tn \tc%d%s\tn", slot_array[slot], (slot),
                   (slot) == 1 ? "st" : (slot) == 2 ? "nd" : (slot) == 3 ? "rd" : "th");
      if (--num_circles > 0)
        send_to_char(ch, "\tY,");
    }
  }
  if (!printed)
    send_to_char(ch, " \tYno more %s!\tn\r\n", class == CLASS_ALCHEMIST ? "extracts" : "spells");
  else
    send_to_char(ch, "\tn\r\n");

  if (APOTHEOSIS_SLOTS(ch) > 0)
    send_to_char(ch, "\tYStored apotheosis charges: \tn%d\r\n", APOTHEOSIS_SLOTS(ch));

  *buf = '\0';
  send_to_char(ch, "\tC");
  text_line(ch, buf, line_length, '-', '-');
  send_to_char(ch, "\tn");
}

/* based on class, will display both:
     prep-queue
     collection
   data... for innate-magic system, send them to a different
   display function */
void print_prep_collection_data(struct char_data *ch, int class)
{
  switch (class)
  {
  case CLASS_SORCERER:
  case CLASS_BARD:
    print_innate_magic_queue(ch, class);
    display_available_slots(ch, class);
    break;
  case CLASS_CLERIC:
  case CLASS_WIZARD:
  case CLASS_RANGER:
  case CLASS_DRUID:
  case CLASS_PALADIN:
  case CLASS_ALCHEMIST:
  case CLASS_BLACKGUARD:
    print_collection(ch, class);
    print_prep_queue(ch, class);
    display_available_slots(ch, class);
    break;
  default:
    return;
  }
}
/* checks to see if ch is ready to prepare, outputs messages,
     and then fires up the event */
void begin_preparing(struct char_data *ch, int class)
{
  char buf[MAX_INPUT_LENGTH];

  if (ready_to_prep_spells(ch, class))
  {
    if (class == CLASS_SORCERER && APOTHEOSIS_SLOTS(ch) > 0)
    {
      // If they are an arcane sorcerer with stored apotheosis charges, they dissipate.
      APOTHEOSIS_SLOTS(ch) = 0;
      send_to_char(ch, "You feel your focused arcane energy fade away.\r\n");
    }
    send_to_char(ch, "You continue your %s.\r\n",
                 spell_prep_dict[class][3]);
    *buf = '\0';
    snprintf(buf, sizeof(buf), "$n continues $s %s.",
             spell_prep_dict[class][3]);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    start_prep_event(ch, class);
  }
}

/* in: char data, class associated with spell, circle of spell, domain
 * out: preparation time for spell number
 * given the above info, calculate how long this particular spell will take to
 * prepare..  this should take into account:
 *   circle
 *   class (arbitrary factor value)
 *   character's skills
 *   character feats   */
int compute_spells_prep_time(struct char_data *ch, int class, int circle, int domain)
{
  float prep_time = 0.0;
  int bonus_time = 0;
  int stat_bonus = 0;
  int level_bonus = 0;

  /* base prep time based on circle, etc */
  prep_time = BASE_PREP_TIME + (PREP_TIME_INTERVALS * (circle - 1));

  /* this is arbitrary, to display that domain-spells can affect prep time */
  if (domain)
    prep_time++;

  /* class factors */
  level_bonus = CLASS_LEVEL(ch, class) / 2;
  switch (class)
  {
  case CLASS_RANGER:
    prep_time *= RANGER_PREP_TIME_FACTOR;
    stat_bonus = GET_WIS_BONUS(ch);
    break;
  case CLASS_PALADIN:
  case CLASS_BLACKGUARD:
    prep_time *= PALADIN_PREP_TIME_FACTOR;
    stat_bonus = GET_CHA_BONUS(ch);
    break;
  case CLASS_DRUID:
    prep_time *= DRUID_PREP_TIME_FACTOR;
    stat_bonus = GET_WIS_BONUS(ch);
    break;
  case CLASS_WIZARD:
    prep_time *= WIZ_PREP_TIME_FACTOR;
    stat_bonus = GET_INT_BONUS(ch);
    break;
  case CLASS_CLERIC:
    prep_time *= CLERIC_PREP_TIME_FACTOR;
    stat_bonus = GET_WIS_BONUS(ch);
    break;
  case CLASS_SORCERER:
    prep_time *= SORC_PREP_TIME_FACTOR;
    stat_bonus = GET_CHA_BONUS(ch);
    break;
  case CLASS_BARD:
    prep_time *= BARD_PREP_TIME_FACTOR;
    stat_bonus = GET_CHA_BONUS(ch);
    break;
  case CLASS_ALCHEMIST:
    prep_time *= ALCHEMIST_PREP_TIME_FACTOR;
    stat_bonus = GET_INT_BONUS(ch);
    break;
  }

  /** calculate bonuses **/
  /*level*/
  bonus_time += level_bonus;
  /*skills*/
  /* stat bonus */
  bonus_time += stat_bonus / 2;
  /* concentration */
  if (!IS_NPC(ch) && GET_ABILITY(ch, ABILITY_CONCENTRATION))
  {
    bonus_time += compute_ability(ch, ABILITY_CONCENTRATION) / 4;
  }
  /*feats*/
  /* faster memorization, reduces prep time by a quarter */
  if (HAS_FEAT(ch, FEAT_FASTER_MEMORIZATION))
  {
    bonus_time += prep_time / 4;
  }
  // If in a regenerating room... normally taverns
  if (IN_ROOM(ch) != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_REGEN))
    bonus_time += prep_time / 4;
  /*spells/affections*/
  /* song of focused mind, halves prep time */
  if (affected_by_spell(ch, SKILL_SONG_OF_FOCUSED_MIND))
  {
    bonus_time += prep_time / 2;
  }
  /** end bonus calculations **/

  prep_time -= bonus_time;

  switch (class)
  {
    case CLASS_WIZARD:
    case CLASS_SORCERER:
    case CLASS_BARD:
      prep_time = prep_time * CONFIG_ARCANE_PREP_TIME / 100;
      break;

    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_PALADIN:
    case CLASS_RANGER:
    case CLASS_BLACKGUARD:
      prep_time = prep_time * CONFIG_DIVINE_PREP_TIME / 100;
      break;

    case CLASS_PSIONICIST:
      // Psionicists use PSP which recharges along with hp and mv
      break;

    case CLASS_ALCHEMIST:
      prep_time = prep_time * CONFIG_ALCHEMY_PREP_TIME / 100;
      break;
  }

  if (prep_time <= 0)
    prep_time = 1;

  return (MAX(circle, prep_time)); /* CAP */
}

/* look at top of the queue, and reset preparation time of that entry */
void reset_preparation_time(struct char_data *ch, int class)
{
  int preparation_time = 0;

  switch (class)
  {
  case CLASS_SORCERER:
  case CLASS_BARD:
    if (!INNATE_MAGIC(ch, class))
      return;
    preparation_time =
        compute_spells_prep_time(
            ch,
            class,
            INNATE_MAGIC(ch, class)->circle,
            INNATE_MAGIC(ch, class)->domain);
    INNATE_MAGIC(ch, class)->prep_time = preparation_time;
    break;
  default:
    if (!SPELL_PREP_QUEUE(ch, class))
      return;
    preparation_time =
        compute_spells_prep_time(
            ch,
            class,
            compute_spells_circle(class,
                                  SPELL_PREP_QUEUE(ch, class)->spell,
                                  SPELL_PREP_QUEUE(ch, class)->metamagic,
                                  SPELL_PREP_QUEUE(ch, class)->domain),
            SPELL_PREP_QUEUE(ch, class)->domain);
    SPELL_PREP_QUEUE(ch, class)->prep_time = preparation_time;
    break;
  }
}

int free_arcana_slots(struct char_data *ch)
{
  int i = 0;

  int num_slots = HAS_REAL_FEAT(ch, FEAT_NEW_ARCANA);

  for (i = 0; i < 3; i++)
  {
    if (NEW_ARCANA_SLOT(ch, i) > 0)
      num_slots--;
  }
  return MAX(0, num_slots);
}

/* END helper functions */

/* START event-related */

/* this will move the spell from the prep-queue to the collection, and if
 * appropriate continue the preparation process - restart the event with
 * the correct prep-time for the next spell in the queue */
EVENTFUNC(event_preparation)
{
  int class = 0;
  struct char_data *ch = NULL;
  struct mud_event_data *prepare_event = NULL;
  char buf[MAX_STRING_LENGTH];

  /* initialize everything and dummy checks */
  *buf = '\0';
  if (event_obj == NULL)
    return 0;
  prepare_event = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)prepare_event->pStruct;
  if (!ch)
    return 0;
  /* grab the svariable for class */
  class = atoi((char *)prepare_event->sVariables);

  /* this is definitely a dummy check */
  switch (class)
  {
  case CLASS_SORCERER:
  case CLASS_BARD:
    if (!INNATE_MAGIC(ch, class))
    {
      send_to_char(ch, "Your preparations are aborted!  You do not seem to have anything in your queue!\r\n");
      return 0;
    }
    break;
  default:
    if (!SPELL_PREP_QUEUE(ch, class))
    {
      send_to_char(ch, "Your preparations are aborted!  You do not seem to have anything in your queue!\r\n");
      return 0;
    }
    break;
  }

  /* first we make a check that we arrived here in a 'valid' state, reset
   * prearation time if not, then exit */
  if (!ready_to_prep_spells(ch, class) ||
      !is_preparing(ch))
  {
    send_to_char(ch, "You are not able to finish your spell preparations!\r\n");
    stop_prep_event(ch, class);
    return 0;
  }

  switch (class)
  {
  case CLASS_BARD:
  case CLASS_SORCERER:
    INNATE_MAGIC(ch, class)->prep_time--;
    if ((INNATE_MAGIC(ch, class)->prep_time) <= 0)
    {
      send_to_char(ch, "You finish %s for %s%s%d circle slot.\r\n",
                   spell_prep_dict[class][1],
                   (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_QUICKEN) ? "quickened " : ""),
                   (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_MAXIMIZE) ? "maximized " : ""),
                   INNATE_MAGIC(ch, class)->circle);
      innate_magic_remove_by_class(ch, class, INNATE_MAGIC(ch, class)->circle,
                                   INNATE_MAGIC(ch, class)->metamagic);
      if (INNATE_MAGIC(ch, class))
      {
        reset_preparation_time(ch, class);
        return (1 * PASSES_PER_SEC);
      }
    }
    break;
  default:
    SPELL_PREP_QUEUE(ch, class)->prep_time--;
    if ((SPELL_PREP_QUEUE(ch, class)->prep_time) <= 0)
    {
      send_to_char(ch, "You finish %s for %s%s%s.\r\n",
                   spell_prep_dict[class][1],
                   (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_QUICKEN) ? "quickened " : ""),
                   (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_MAXIMIZE) ? "maximized " : ""),
                   spell_info[SPELL_PREP_QUEUE(ch, class)->spell].name);
      collection_add(ch, class, SPELL_PREP_QUEUE(ch, class)->spell,
                     SPELL_PREP_QUEUE(ch, class)->metamagic,
                     SPELL_PREP_QUEUE(ch, class)->prep_time,
                     SPELL_PREP_QUEUE(ch, class)->domain);
      prep_queue_remove_by_class(ch, class, SPELL_PREP_QUEUE(ch, class)->spell,
                                 SPELL_PREP_QUEUE(ch, class)->metamagic);
      if (SPELL_PREP_QUEUE(ch, class))
      {
        reset_preparation_time(ch, class);
        return (1 * PASSES_PER_SEC);
      }
    }
    break;
  }

  switch (class)
  {
  case CLASS_BARD:
  case CLASS_SORCERER:
    if (!INNATE_MAGIC(ch, class))
    {
      *buf = '\0';
      send_to_char(ch, "Your %s are complete.\r\n", spell_prep_dict[class][3]);
      snprintf(buf, sizeof(buf), "$n completes $s %s.", spell_prep_dict[class][3]);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      set_preparing_state(ch, class, FALSE);
      return 0;
    }
    break;
  default:
    if (!SPELL_PREP_QUEUE(ch, class))
    {
      *buf = '\0';
      send_to_char(ch, "Your %s are complete.\r\n", spell_prep_dict[class][3]);
      snprintf(buf, sizeof(buf), "$n completes $s %s.", spell_prep_dict[class][3]);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      set_preparing_state(ch, class, FALSE);
      return 0;
    }
    break;
  }

  return (1 * PASSES_PER_SEC);
}

/* END event-related */

/* START ACMD() */

/* manipulation of your respective lists! command for players to
 *   "un" prepare spells */
ACMDU(do_consign_to_oblivion)
{
  int domain_1st = 0, domain_2nd = 0, class = CLASS_UNDEFINED;
  char *spell_arg, *metamagic_arg, arg[MAX_INPUT_LENGTH];
  int spellnum = 0, metamagic = 0;
  bool consign_all = FALSE;

  *arg = '\0';

  switch (subcmd)
  {
  case SCMD_BLANK:
    class = CLASS_CLERIC;
    domain_1st = GET_1ST_DOMAIN(ch);
    domain_2nd = GET_2ND_DOMAIN(ch);
    break;
  case SCMD_FORGET:
    class = CLASS_WIZARD;
    break;
  case SCMD_UNADJURE:
    class = CLASS_RANGER;
    break;
  case SCMD_OMIT:
    class = CLASS_PALADIN;
    break;
  case SCMD_UNCONDEMN:
    class = CLASS_BLACKGUARD;
    break;
  case SCMD_UNCOMMUNE:
    class = CLASS_DRUID;
    break;
  case SCMD_DISCARD:
    class = CLASS_ALCHEMIST;
    break;
  /* innate-magic casters such as sorc / bard, do not use this command */
  default:
    send_to_char(ch, "Invalid command!\r\n");
    return;
  }

  /* Copy the argument, strtok mangles it. */
  snprintf(arg, sizeof(arg), "%s", argument);

  /* Check for metamagic. */
  for (metamagic_arg = strtok(argument, " "); metamagic_arg && metamagic_arg[0] != '\'';
       metamagic_arg = strtok(NULL, " "))
  {
    if (strcmp(metamagic_arg, "all") == 0)
    {
      consign_all = TRUE;
      break;
    }
    else if (is_abbrev(metamagic_arg, "quickened"))
    {
      SET_BIT(metamagic, METAMAGIC_QUICKEN);
    }
    else if (is_abbrev(metamagic_arg, "maximized"))
    {
      SET_BIT(metamagic, METAMAGIC_MAXIMIZE);
    }
    else
    {
      send_to_char(ch, "With what metamagic? (if you have an argument before the "
                       "magical '' symbols, we are expecting meta-magic arguments, example: "
                       "forget MAXIMIZED 'fireball')\r\n");
      return;
    }
  }

  /* handle single spell, dealing with 'consign all' below */
  if (!consign_all)
  {
    spell_arg = strtok(arg, "'");
    if (spell_arg == NULL)
    {
      send_to_char(ch, "Which %s do you want to %s? "
                       "Usage: %s <meta-magic arguments> '<spell name>' or ALL for all spells.\r\n",
                   class == CLASS_ALCHEMIST ? "extract" : "spell",
                   spell_consign_dict[class][0], spell_consign_dict[class][0]);
      return;
    }

    spell_arg = strtok(NULL, "'");
    if (spell_arg == NULL)
    {
      send_to_char(ch, "Spell names must be enclosed in the Holy Magic Symbols: '\r\n");
      return;
    }

    spellnum = find_skill_num(spell_arg);

    if (!get_class_highest_circle(ch, class))
    {
      send_to_char(ch, "You do not have any casting ability in this class!\r\n");
      return;
    }

    /* forget ALL */
  }
  else
  {

    /* if we have a queue, we are clearing it */
    if (SPELL_PREP_QUEUE(ch, class))
    {
      clear_prep_queue_by_class(ch, class);
      send_to_char(ch, "You %s everything you were attempting to %s for.\r\n",
                   spell_consign_dict[class][0], spell_prep_dict[class][0]);
      stop_prep_event(ch, class);
      return;
    }

    /* elseif we have a collection, we are clearing it */
    else if (SPELL_COLLECTION(ch, class))
    {
      clear_collection_by_class(ch, class);
      send_to_char(ch, "You %s everything you had %s for.\r\n",
                   spell_consign_dict[class][0], spell_prep_dict[class][2]);
      stop_prep_event(ch, class);
      return;

      /* we have nothing in -either- queue! */
    }
    else
    {
      send_to_char(ch, "There is nothing in your preparation queue or %s "
                       "collection to %s!\r\n",
                   class == CLASS_ALCHEMIST ? "extract" : "spell", spell_consign_dict[class][0]);
      stop_prep_event(ch, class);
      return;
    }
  } /* END forget all */

  /* Begin system for forgetting a specific spell */

  if (spellnum < 1 || spellnum > MAX_SPELLS)
  {
    send_to_char(ch, "You never knew that %s to begin with!\r\n", class == CLASS_ALCHEMIST ? "extract" : "spell");
    return;
  }

  /* check preparation queue for spell, if found, remove and exit */
  if (SPELL_PREP_QUEUE(ch, class))
  {

    /* if this spell is top of prep queue, we are removing the event */
    if (SPELL_PREP_QUEUE(ch, class)->spell == spellnum &&
        SPELL_PREP_QUEUE(ch, class)->metamagic == metamagic &&
        PREPARING_STATE(ch, class))
    {
      send_to_char(ch, "Being that %s was the next in your preparation queue, you are "
                       "forced to abort your %s.\r\n",
                   class == CLASS_ALCHEMIST ? "extract" : "spell", spell_prep_dict[class][3]);
      stop_prep_event(ch, class);
    }

    if (prep_queue_remove_by_class(ch, class, spellnum, metamagic))
    {
      send_to_char(ch, "You %s \tW%s\tn %s%s from your %s preparation queue!\r\n",
                   spell_consign_dict[class][0],
                   spell_name(spellnum),
                   (IS_SET(metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
                   class == CLASS_ALCHEMIST ? "extract" : "spell");
      return;
    }
  }

  /* check spell-collection for spell, if found, remove and exit */
  if (SPELL_COLLECTION(ch, class))
  {
    if (collection_remove_by_class(ch, class, spellnum, metamagic))
    {
      send_to_char(ch, "You %s \tW%s\tn %s%s from your %s collection!\r\n",
                   spell_consign_dict[class][0],
                   spell_name(spellnum),
                   (IS_SET(metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
                   (class == CLASS_ALCHEMIST ? "extract" : "spell"));
      return;
    }
  }

  /* nowhere else to search to get rid of this spell! */
  send_to_char(ch, "You do not have %s in your preparation queue or %s collection! "
                   "(make sure you used the correct command based on class and you used the "
                   "proper meta-magic arguments)\r\n",
               spell_info[spellnum].name,
               class == CLASS_ALCHEMIST ? "extract" : "spell");
}

/* preparation command entry point for players */
/*  Functionality of preparation system (trying to keep in order):
      4) To view your spell collection
      5) To view your spell prep queue
      2) To begin preparation (by class and general?)
      3) To manipulate your prep queue
        a) ex. memorize
          i) metamagic
        b) ex. forget
          i) metamagic
        c) prioritize
 * How it works: ex. wizard, you type memorize, it should display
 *   your spell-prep/collection interfaces.  If you are in a 'state'
 *   of being able to continue your studies, and are not studying, 
 *   start studying.  You can then type: memorize [metamagic] 'spellname'
 *   to add spells to your prep-queue; this does not require being
 *   in a proper 'state' - can do anytime.
   TODO:
     - FIX domains as entry point value here! */
ACMDU(do_gen_preparation)
{
  int class = CLASS_UNDEFINED, circle_for_spell = 0, num_slots_by_circle = 0;
  int spellnum = 0, metamagic = 0, domain_1st = 0, domain_2nd = 0;
  char *spell_arg = NULL, *metamagic_arg = NULL;

  switch (subcmd)
  {
  case SCMD_PRAY:
    class = CLASS_CLERIC;
    domain_1st = GET_1ST_DOMAIN(ch);
    domain_2nd = GET_2ND_DOMAIN(ch);
    break;
  case SCMD_MEMORIZE:
    class = CLASS_WIZARD;
    break;
  case SCMD_ADJURE:
    class = CLASS_RANGER;
    break;
  case SCMD_CHANT:
    class = CLASS_PALADIN;
    break;
  case SCMD_CONDEMN:
    class = CLASS_BLACKGUARD;
    break;
  case SCMD_COMMUNE:
    class = CLASS_DRUID;
    break;
  case SCMD_MEDITATE:
    class = CLASS_SORCERER;
    break;
  case SCMD_COMPOSE:
    class = CLASS_BARD;
    break;
  case SCMD_CONCOCT:
    class = CLASS_ALCHEMIST;
    break;
  default:
    send_to_char(ch, "Invalid command!\r\n");
    return;
  }

  if (!get_class_highest_circle(ch, class))
  {
    send_to_char(ch, "Try changing professions (type score to view respective preparation commands for your class(es)!\r\n");
    return;
  }

  switch (class)
  {
  case CLASS_SORCERER:
  case CLASS_BARD:
    print_prep_collection_data(ch, class);
    begin_preparing(ch, class);
    return; /* innate-magic is finished in this command */
  default:
    if (!*argument)
    {
      print_prep_collection_data(ch, class);
      begin_preparing(ch, class);
      return;
    }
    break; /* we do have an argument! */
  }

  /* Here I needed to change a bit to grab the metamagic keywords.  
   * Valid keywords are:
   *   quickened - Speed up casting
   *   maximized - All variable aspects of spell (dam dice, etc) are maximum. */

  /* spell_arg is a pointer into the argument string.  First lets find the spell - 
   * it should be at the end of the string. */
  spell_arg = strtok(argument, "'");
  if (spell_arg == NULL)
  {
    send_to_char(ch, "Prepare which spell?\r\n");
    return;
  }

  spell_arg = strtok(NULL, "'");
  if (spell_arg == NULL)
  {
    send_to_char(ch, "The name of the spell to prepare must be enclosed within ' and '.\r\n");
    return;
  }

  spellnum = find_skill_num(spell_arg);
  if (spellnum < 1 || spellnum > MAX_SPELLS)
  {
    send_to_char(ch, "Prepare which spell?\r\n");
    return;
  }

  /* 3.23.18 Ornir : Add better message when you try to prepare a spell from a restricted school. */
  if ((class == CLASS_WIZARD) &&
      (spell_info[spellnum].schoolOfMagic == restricted_school_reference[GET_SPECIALTY_SCHOOL(ch)]))
  {
    send_to_char(ch, "You are unable to prepare spells from this school of magic.\r\n");
    return;
  }

  /* Now we have the spell.  Back up a little and check for metamagic. */
  for (metamagic_arg = strtok(argument, " "); metamagic_arg && metamagic_arg[0] != '\'';
       metamagic_arg = strtok(NULL, " "))
  {
    if (is_abbrev(metamagic_arg, "quickened"))
    {
      if (HAS_FEAT(ch, FEAT_QUICKEN_SPELL))
      {
        SET_BIT(metamagic, METAMAGIC_QUICKEN);
      }
      else
      {
        send_to_char(ch, "You don't know how to quicken your magic!\r\n");
        return;
      }
    }
    else if (is_abbrev(metamagic_arg, "maximized"))
    {
      if (HAS_FEAT(ch, FEAT_MAXIMIZE_SPELL))
      {
        SET_BIT(metamagic, METAMAGIC_MAXIMIZE);
      }
      else
      {
        send_to_char(ch, "You don't know how to maximize your magic!\r\n");
        return;
      }
    }
    else
    {
      send_to_char(ch, "Use what metamagic?\r\n");
      return;
    }
  }

  if (!is_min_level_for_spell(ch, class, spellnum))
  { /* checks domain eligibility */
    send_to_char(ch, "That spell is beyond your grasp!\r\n");
    return;
  }

  circle_for_spell = /* checks domain spells */
      MIN(compute_spells_circle(class, spellnum, metamagic, domain_1st),
          compute_spells_circle(class, spellnum, metamagic, domain_2nd));

  num_slots_by_circle = compute_slots_by_circle(ch, class, circle_for_spell);

  if (num_slots_by_circle <= 0)
  {
    send_to_char(ch, "You have no slots available in that circle!\r\n");
    return;
  }

  /* count_total_slots is a count of how many are used by circle */
  if ((num_slots_by_circle - count_total_slots(ch, class, circle_for_spell)) <= 0)
  {
    send_to_char(ch, "You can't retain more spells of that circle!\r\n");
    return;
  }

  /* wizards spellbook reqs */
  if (class == CLASS_WIZARD && !spellbook_ok(ch, spellnum, class, TRUE))
  {
    send_to_char(ch, "You need a source (spellbook or scroll) to study that spell from!\r\n");
    return;
  }

  /* success, let's throw the spell into our prep queue */
  send_to_char(ch, "You start to %s for %s%s%s.\r\n",
               spell_prep_dict[class][0],
               (IS_SET(metamagic, METAMAGIC_QUICKEN) ? "quickened " : ""),
               (IS_SET(metamagic, METAMAGIC_MAXIMIZE) ? "maximized " : ""),
               spell_info[spellnum].name);

  prep_queue_add(ch,
                 class,
                 spellnum,
                 metamagic,
                 compute_spells_prep_time(ch,
                                          class,
                                          circle_for_spell,
                                          is_domain_spell_of_ch(ch, spellnum)),
                 is_domain_spell_of_ch(ch, spellnum));

  begin_preparing(ch, class);
}

/* END acmd */

/* EOF */
