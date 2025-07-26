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

/**
 * @file spell_prep.c
 * @brief Complete spell preparation system implementation for LuminariMUD
 * 
 * This file implements the entire spell preparation and management system for
 * all spellcasting classes in LuminariMUD. It handles two distinct casting
 * paradigms:
 * 
 * 1. PREPARATION-BASED CASTING (Wizard, Cleric, Druid, Ranger, Paladin, etc.)
 *    - Must select and prepare specific spells in advance
 *    - Spells are queued for preparation, taking real time to complete
 *    - Once prepared, spells move to a "collection" ready to be cast
 *    - After casting, the spell is consumed and must be prepared again
 * 
 * 2. SPONTANEOUS/INNATE CASTING (Sorcerer, Bard, Inquisitor, Summoner)
 *    - Know a limited set of spells permanently
 *    - Have spell "slots" by circle that can be used for any known spell
 *    - Don't prepare specific spells, just recover spell slots over time
 *    - Can cast any known spell using an available slot of that circle
 * 
 * KEY CONCEPTS:
 * - Preparation Queue: Spells currently being prepared (prep time counting down)
 * - Spell Collection: Fully prepared spells ready to cast
 * - Innate Magic Queue: Available spell slots for spontaneous casters
 * - Known Spells: List of spells a spontaneous caster can choose from
 * 
 * The system also handles:
 * - Metamagic modifications (quicken, maximize, etc.)
 * - Domain spells for divine casters
 * - Bloodline spells for sorcerers
 * - Saving/loading spell data to player files
 * - Real-time preparation with interruption handling
 * 
 * TODO:
 * - Convert spell slot system to be feat-based
 * - Implement epic spell handling
 */

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

/**
 * DEBUGMODE - Toggle for verbose debug output
 * 
 * When set to TRUE, various functions will send detailed debug messages
 * to help track spell preparation flow. Should be FALSE for production.
 */
#define DEBUGMODE FALSE

/** END Globals **/

/* START linked list utility - Core data structure management */

/**
 * init_spell_prep_queue - Initialize all preparation queues to NULL
 * @ch: Character to initialize
 * 
 * Called during character creation or login when no saved spell data exists.
 * Sets all class preparation queue pointers to NULL to ensure clean state.
 * Must be called before any spell preparation operations.
 */
void init_spell_prep_queue(struct char_data *ch)
{
  int ch_class = 0;
  
  /* Loop through all possible classes and NULL their queue pointers */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    SPELL_PREP_QUEUE(ch, ch_class) = NULL;
}

/**
 * init_innate_magic_queue - Initialize all innate magic queues to NULL
 * @ch: Character to initialize
 * 
 * Sets up empty innate magic queues for spontaneous casters.
 * Called during character creation or when loading a character
 * without saved innate magic data.
 */
void init_innate_magic_queue(struct char_data *ch)
{
  int ch_class = 0;
  
  /* Initialize each class's innate magic queue */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    INNATE_MAGIC(ch, ch_class) = NULL;
}

/**
 * init_collection_queue - Initialize all spell collections to NULL
 * @ch: Character to initialize
 * 
 * Creates empty spell collections for all classes. The collection
 * holds fully prepared spells ready to be cast. Called during
 * character creation or login without saved collection data.
 */
void init_collection_queue(struct char_data *ch)
{
  int ch_class = 0;
  
  /* Clear all class collections */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    SPELL_COLLECTION(ch, ch_class) = NULL;
}

/**
 * init_known_spells - Initialize all known spell lists to NULL
 * @ch: Character to initialize
 * 
 * Sets up empty known spell lists for spontaneous casters.
 * These lists track which spells the character can cast using
 * their available spell slots.
 */
void init_known_spells(struct char_data *ch)
{
  int ch_class = 0;
  
  /* Initialize each class's known spell list */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    KNOWN_SPELLS(ch, ch_class) = NULL;
}

/**
 * clear_prep_queue_by_class - Remove all spells from a class's prep queue
 * @ch: Character whose queue to clear
 * @ch_class: Specific class to clear
 * 
 * Walks through the preparation queue linked list for the specified class,
 * freeing each node. Used when a character forgets all spells, changes
 * classes, or needs to reset their spell preparation.
 * 
 * Safety: Checks for NULL character and player_specials before proceeding.
 */
void clear_prep_queue_by_class(struct char_data *ch, int ch_class)
{
  struct prep_collection_spell_data *tmp, *next;
  
  /* Safety checks - NPCs don't have player_specials */
  if (!ch || !ch->player_specials)
    return;
  
  /* Walk the linked list, freeing each node */
  tmp = SPELL_PREP_QUEUE(ch, ch_class);
  while (tmp)
  {
    next = tmp->next;  /* Save next pointer before freeing */
    free(tmp);
    tmp = next;
  }
  
  /* Reset the head pointer to NULL */
  SPELL_PREP_QUEUE(ch, ch_class) = NULL;
}

/**
 * clear_innate_magic_by_class - Remove all spell slots for a class
 * @ch: Character whose slots to clear
 * @ch_class: Specific class to clear
 * 
 * Frees all innate magic slot entries for spontaneous casters.
 * Used when resetting a character's available spell slots or
 * when they lose access to a spontaneous casting class.
 */
void clear_innate_magic_by_class(struct char_data *ch, int ch_class)
{
  struct innate_magic_data *tmp, *next;
  
  /* Safety checks */
  if (!ch || !ch->player_specials)
    return;
  
  /* Free all nodes in the innate magic list */
  tmp = INNATE_MAGIC(ch, ch_class);
  while (tmp)
  {
    next = tmp->next;
    free(tmp);
    tmp = next;
  }
  
  /* Clear the head pointer */
  INNATE_MAGIC(ch, ch_class) = NULL;
}

/**
 * clear_collection_by_class - Remove all prepared spells for a class
 * @ch: Character whose collection to clear
 * @ch_class: Specific class to clear
 * 
 * Empties the spell collection for a specific class, effectively
 * "forgetting" all prepared spells. The character will need to
 * prepare spells again from scratch.
 */
void clear_collection_by_class(struct char_data *ch, int ch_class)
{
  struct prep_collection_spell_data *tmp, *next;
  
  /* Safety checks */
  if (!ch || !ch->player_specials)
    return;
  
  /* Walk through and free all collection entries */
  tmp = SPELL_COLLECTION(ch, ch_class);
  while (tmp)
  {
    next = tmp->next;
    free(tmp);
    tmp = next;
  }
  
  /* Reset the collection to empty */
  SPELL_COLLECTION(ch, ch_class) = NULL;
}

/**
 * clear_known_spells_by_class - Remove all known spells for a class
 * @ch: Character whose knowledge to clear
 * @ch_class: Specific class to clear
 * 
 * Removes all spells from a spontaneous caster's known spell list.
 * This is permanent spell knowledge loss - the character will need
 * to relearn spells through leveling or other means.
 */
void clear_known_spells_by_class(struct char_data *ch, int ch_class)
{
  struct known_spell_data *tmp, *next;
  
  /* Safety checks */
  if (!ch || !ch->player_specials)
    return;
  
  /* Free all known spell entries */
  tmp = KNOWN_SPELLS(ch, ch_class);
  while (tmp)
  {
    next = tmp->next;
    free(tmp);
    tmp = next;
  }
  
  /* Clear the known spells list */
  KNOWN_SPELLS(ch, ch_class) = NULL;
}

/**
 * destroy_spell_prep_queue - Free all preparation queues for all classes
 * @ch: Character logging out or being destroyed
 * 
 * Master cleanup function called during character logout or deletion.
 * Iterates through all classes and clears their preparation queues
 * to prevent memory leaks. This ensures all dynamically allocated
 * spell preparation data is properly freed.
 */
void destroy_spell_prep_queue(struct char_data *ch)
{
  int ch_class;
  
  /* Safety check - NPCs don't have spell queues */
  if (!ch || !ch->player_specials)
    return;
    
  /* Clear prep queue for every class */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    clear_prep_queue_by_class(ch, ch_class);
}

/**
 * destroy_innate_magic_queue - Free all innate magic slots for all classes
 * @ch: Character logging out or being destroyed
 * 
 * Cleanup function for spontaneous casters' spell slot data.
 * Called during logout to ensure all innate magic queue memory
 * is properly freed across all classes.
 */
void destroy_innate_magic_queue(struct char_data *ch)
{
  int ch_class;
  
  /* Safety check */
  if (!ch || !ch->player_specials)
    return;
    
  /* Clear innate magic for every class */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    clear_innate_magic_by_class(ch, ch_class);
}

/**
 * destroy_spell_collection - Free all spell collections for all classes
 * @ch: Character logging out or being destroyed
 * 
 * Master cleanup for prepared spell collections. Ensures all
 * prepared spells are removed from memory when a character
 * logs out or is deleted. Note the typo in the old comment
 * "destroy_spell_destroy_spell_collection" has been fixed.
 */
void destroy_spell_collection(struct char_data *ch)
{
  int ch_class;
  
  /* Safety check */
  if (!ch || !ch->player_specials)
    return;
    
  /* Clear collection for every class */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    clear_collection_by_class(ch, ch_class);
}

/**
 * destroy_known_spells - Free all known spell lists for all classes
 * @ch: Character logging out or being destroyed
 * 
 * Cleanup function for spontaneous casters' spell knowledge.
 * Frees all memory associated with known spell lists across
 * all classes. Called during character logout/deletion.
 */
void destroy_known_spells(struct char_data *ch)
{
  int ch_class;
  
  /* Safety check */
  if (!ch || !ch->player_specials)
    return;
    
  /* Clear known spells for every class */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    clear_known_spells_by_class(ch, ch_class);
}

/**
 * save_prep_queue_by_class - Save one class's preparation queue to file
 * @fl: Open file handle for writing
 * @ch: Character whose data to save
 * @class: Specific class queue to save
 * 
 * Writes all spells currently being prepared for a specific class.
 * Format per line: class spell_num metamagic prep_time domain
 * Example: "0 123 3 45 0" = Wizard preparing fireball with quicken+empower, 45 seconds left
 * 
 * Called by save_spell_prep_queue() for each class.
 */
void save_prep_queue_by_class(FILE *fl, struct char_data *ch, int class)
{
  struct prep_collection_spell_data *current = SPELL_PREP_QUEUE(ch, class);
  struct prep_collection_spell_data *next;
  
  /* Walk the linked list and save each entry */
  for (; current; current = next)
  {
    next = current->next;
    fprintf(fl, "%d %d %d %d %d\n", 
            class,                /* Class number */
            current->spell,       /* Spell number being prepared */
            current->metamagic,   /* Metamagic flags as bitmask */
            current->prep_time,   /* Seconds remaining to prepare */
            current->domain);     /* Domain (for clerics) or 0 */
  }
}

/**
 * save_innate_magic_by_class - Save one class's innate magic slots to file
 * @fl: Open file handle for writing
 * @ch: Character whose data to save
 * @class: Specific class slots to save
 * 
 * For spontaneous casters, saves available spell slots by circle.
 * Format: class circle metamagic prep_time domain
 * The circle replaces spell_num since spontaneous casters prepare slots, not spells.
 */
void save_innate_magic_by_class(FILE *fl, struct char_data *ch, int class)
{
  struct innate_magic_data *current = INNATE_MAGIC(ch, class);
  struct innate_magic_data *next;
  
  /* Save each spell slot entry */
  for (; current; current = next)
  {
    next = current->next;
    fprintf(fl, "%d %d %d %d %d\n", 
            class,                /* Class number */
            current->circle,      /* Spell circle (1-9) */
            current->metamagic,   /* Pre-applied metamagic (rare) */
            current->prep_time,   /* Time until slot is ready */
            current->domain);     /* Domain (usually 0 for innate) */
  }
}

/**
 * save_collection_by_class - Save one class's prepared spells to file
 * @fl: Open file handle for writing
 * @ch: Character whose data to save
 * @class: Specific class collection to save
 * 
 * Saves all fully prepared spells ready to cast.
 * Format matches prep queue: class spell_num metamagic prep_time domain
 * prep_time is usually 0 for collection entries since they're ready.
 */
void save_collection_by_class(FILE *fl, struct char_data *ch, int class)
{
  struct prep_collection_spell_data *current = SPELL_COLLECTION(ch, class);
  struct prep_collection_spell_data *next;
  
  /* Save each prepared spell */
  for (; current; current = next)
  {
    next = current->next;
    fprintf(fl, "%d %d %d %d %d\n", 
            class,                /* Class number */
            current->spell,       /* Spell number ready to cast */
            current->metamagic,   /* Metamagic flags applied */
            current->prep_time,   /* Usually 0 (ready) */
            current->domain);     /* Domain spell indicator */
  }
}

/**
 * save_known_spells_by_class - Save one class's known spell list to file
 * @fl: Open file handle for writing
 * @ch: Character whose data to save
 * @class: Specific class knowledge to save
 * 
 * For spontaneous casters, saves which spells they can cast.
 * Format: class spell_num (simpler than other saves)
 * No metamagic/prep_time/domain needed for spell knowledge.
 */
void save_known_spells_by_class(FILE *fl, struct char_data *ch, int class)
{
  struct known_spell_data *current = KNOWN_SPELLS(ch, class);
  struct known_spell_data *next;
  
  /* Save each known spell */
  for (; current; current = next)
  {
    next = current->next;
    fprintf(fl, "%d %d\n", 
            class,            /* Class number */
            current->spell);  /* Spell number known */
  }
}

/**
 * save_spell_prep_queue - Master save function for all preparation queues
 * @fl: Open file handle for writing
 * @ch: Character whose data to save
 * 
 * Saves all spell preparation queues across all classes to the player file.
 * File format:
 *   PrQu:                    (header identifying prep queue section)
 *   [queue entries...]       (0 or more lines of spell data)
 *   -1 -1 -1 -1 -1          (sentinel marking end of section)
 * 
 * Called during character save operations.
 */
void save_spell_prep_queue(FILE *fl, struct char_data *ch)
{
  int ch_class;
  
  /* Write section header */
  fprintf(fl, "PrQu:\n");
  
  /* Save each class's prep queue */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    save_prep_queue_by_class(fl, ch, ch_class);
    
  /* Write sentinel to mark end of prep queue data */
  fprintf(fl, "-1 -1 -1 -1 -1\n");
}

/**
 * save_innate_magic_queue - Master save function for all innate magic slots
 * @fl: Open file handle for writing
 * @ch: Character whose data to save
 * 
 * Saves spell slot data for spontaneous casters across all classes.
 * File format:
 *   InMa:                    (header for innate magic section)
 *   [slot entries...]        (0 or more lines of slot data)
 *   -1 -1 -1 -1 -1          (sentinel marking end)
 */
void save_innate_magic_queue(FILE *fl, struct char_data *ch)
{
  int ch_class;
  
  /* Write section header */
  fprintf(fl, "InMa:\n");
  
  /* Save each class's innate magic slots */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    save_innate_magic_by_class(fl, ch, ch_class);
    
  /* Write sentinel */
  fprintf(fl, "-1 -1 -1 -1 -1\n");
}

/**
 * save_spell_collection - Master save function for all prepared spells
 * @fl: Open file handle for writing
 * @ch: Character whose data to save
 * 
 * Saves all fully prepared spells ready to cast.
 * File format:
 *   Coll:                    (header for collection section)
 *   [spell entries...]       (0 or more prepared spells)
 *   -1 -1 -1 -1 -1          (sentinel)
 */
void save_spell_collection(FILE *fl, struct char_data *ch)
{
  int ch_class;
  
  /* Write section header */
  fprintf(fl, "Coll:\n");
  
  /* Save each class's collection */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    save_collection_by_class(fl, ch, ch_class);
    
  /* Write sentinel */
  fprintf(fl, "-1 -1 -1 -1 -1\n");
}

/**
 * save_known_spells - Master save function for all known spell lists
 * @fl: Open file handle for writing
 * @ch: Character whose data to save
 * 
 * Saves spell knowledge for spontaneous casters.
 * File format:
 *   KnSp:                    (header for known spells)
 *   [spell entries...]       (class + spell number pairs)
 *   -1 -1                    (sentinel - only 2 values)
 * 
 * Note: Sentinel is shorter because known spells only save 2 values per line.
 */
void save_known_spells(FILE *fl, struct char_data *ch)
{
  int ch_class;
  
  /* Write section header */
  fprintf(fl, "KnSp:\n");
  
  /* Save each class's known spells */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
    save_known_spells_by_class(fl, ch, ch_class);
    
  /* Write sentinel (only 2 values for known spells) */
  fprintf(fl, "-1 -1\n");
}

/**
 * prep_queue_remove_by_class - Find and remove spell from preparation queue
 * @ch: Character whose queue to search
 * @class: Class to check  
 * @spellnum: Spell number to find
 * @metamagic: Metamagic flags that must match exactly
 * 
 * Searches through the preparation queue for the specified class looking
 * for an exact match of spell number and metamagic combination. If found,
 * removes it from the queue and frees the memory.
 * 
 * This is used when:
 * - A spell finishes preparing and moves to collection
 * - Player uses forget/blank/etc commands to cancel preparation
 * - Character dies or loses ability to cast
 * 
 * Returns: TRUE if spell was found and removed, FALSE if not found
 */
bool prep_queue_remove_by_class(struct char_data *ch, int class, int spellnum,
                                int metamagic)
{
  struct prep_collection_spell_data *current = SPELL_PREP_QUEUE(ch, class);
  struct prep_collection_spell_data *next;

  /* Walk the linked list looking for exact match */
  for (; current; current = next)
  {
    next = current->next;
    if (current->spell == spellnum && current->metamagic == metamagic)
    {
      /* Found it - remove from list and free */
      prep_queue_remove(ch, current, class);
      return TRUE;
    }
  }

  return FALSE;
}
/**
 * innate_magic_remove_by_class - Find and remove spell slot from innate magic
 * @ch: Character whose slots to search
 * @class: Class to check (must be spontaneous caster)
 * @circle: Spell circle of the slot to find
 * @metamagic: Metamagic flags that must match
 * 
 * For spontaneous casters (Sorcerer, Bard, etc.), searches the innate
 * magic queue for a slot matching the specified circle and metamagic.
 * If found, removes it from the queue.
 * 
 * Used when:
 * - A spell slot finishes recovering and becomes available
 * - Character loses ability to cast
 * - Slot is consumed by casting
 * 
 * Returns: TRUE if slot was found and removed, FALSE if not found
 */
bool innate_magic_remove_by_class(struct char_data *ch, int class, int circle,
                                  int metamagic)
{
  struct innate_magic_data *current = INNATE_MAGIC(ch, class);
  struct innate_magic_data *next;

  /* Search for matching circle and metamagic */
  for (; current; current = next)
  {
    next = current->next;
    if (current->circle == circle && current->metamagic == metamagic)
    {
      /* Found matching slot - remove it */
      innate_magic_remove(ch, current, class);
      return TRUE;
    }
  }

  return FALSE;
}
/**
 * collection_remove_by_class - Find and remove prepared spell from collection
 * @ch: Character whose collection to search
 * @class: Class to check
 * @spellnum: Spell number to find
 * @metamagic: Metamagic flags that must match exactly
 * 
 * Searches the spell collection (fully prepared spells) for an exact
 * match of spell number and metamagic. If found, removes it from the
 * collection and frees memory.
 * 
 * This is the primary function called when:
 * - A spell is cast (moves from collection to prep queue)
 * - Player uses forget/blank commands on prepared spells
 * - Character loses casting ability
 * 
 * Note: Debug mode will output trace information if enabled
 * 
 * Returns: TRUE if spell was found and removed, FALSE if not found
 */
bool collection_remove_by_class(struct char_data *ch, int class, int spellnum,
                                int metamagic)
{
  struct prep_collection_spell_data *current = SPELL_COLLECTION(ch, class);
  struct prep_collection_spell_data *next;

  if (DEBUGMODE)
  {
    send_to_char(ch, "{entered collection_remove_by_class()}    ");
  }

  /* Search collection for exact spell+metamagic match */
  for (; current; current = next)
  {
    next = current->next;
    if (current->spell == spellnum && current->metamagic == metamagic)
    {
      /* Found it - remove from collection */
      collection_remove(ch, current, class);
      return TRUE;
    }
  }

  return FALSE;
}
/**
 * known_spells_remove_by_class - Find and remove spell from known list
 * @ch: Character whose knowledge to modify
 * @class: Class to check (must be spontaneous caster)
 * @spellnum: Spell number to remove
 * 
 * For spontaneous casters, removes a spell from their permanent
 * spell knowledge. This is a significant action as it means the
 * character can no longer cast this spell at all.
 * 
 * Used when:
 * - Character is retraining/respeccing
 * - A curse or effect removes spell knowledge
 * - Character violates class restrictions
 * 
 * Note: No metamagic parameter needed as spell knowledge is
 * independent of how the spell might be cast
 * 
 * Returns: TRUE if spell was found and removed, FALSE if not found
 */
bool known_spells_remove_by_class(struct char_data *ch, int class, int spellnum)
{
  struct known_spell_data *current = KNOWN_SPELLS(ch, class);
  struct known_spell_data *next;

  /* Search for the spell in known list */
  for (; current; current = next)
  {
    next = current->next;
    if (current->spell == spellnum)
    {
      /* Found it - remove from knowledge */
      known_spells_remove(ch, current, class);
      return TRUE;
    }
  }

  return FALSE;
}

/**
 * prep_queue_remove - Low-level removal of prep queue entry
 * @ch: Character owning the queue
 * @entry: Exact entry to remove from the list
 * @class: Class whose queue contains the entry
 * 
 * Low-level function that handles the actual linked list manipulation
 * to remove an entry from the preparation queue. This should not be
 * called directly - use prep_queue_remove_by_class() instead.
 * 
 * Safety: Calls core_dump() if queue is NULL when it shouldn't be,
 * indicating memory corruption or logic error.
 * 
 * Note: The REMOVE_FROM_LIST macro handles relinking the list
 */
void prep_queue_remove(struct char_data *ch, struct prep_collection_spell_data *entry,
                       int class)
{
  struct prep_collection_spell_data *temp;

  /* Sanity check - queue should exist if we're removing from it */
  if (SPELL_PREP_QUEUE(ch, class) == NULL)
  {
    core_dump();
    return;
  }
  
  /* Remove from linked list and free memory */
  REMOVE_FROM_LIST(entry, SPELL_PREP_QUEUE(ch, class), next);
  free(entry);
}
/**
 * innate_magic_remove - Low-level removal of innate magic entry
 * @ch: Character owning the queue
 * @entry: Exact slot entry to remove
 * @class: Class whose innate magic contains the entry
 * 
 * Low-level function for removing spell slot entries from the
 * innate magic queue. Used by spontaneous casters when slots
 * finish recovering or are consumed.
 * 
 * Should not be called directly - use innate_magic_remove_by_class()
 * 
 * Safety: Calls core_dump() if queue is unexpectedly NULL
 */
void innate_magic_remove(struct char_data *ch, struct innate_magic_data *entry,
                         int class)
{
  struct innate_magic_data *temp;

  /* Sanity check */
  if (INNATE_MAGIC(ch, class) == NULL)
  {
    core_dump();
    return;
  }
  
  /* Unlink and free the slot entry */
  REMOVE_FROM_LIST(entry, INNATE_MAGIC(ch, class), next);
  free(entry);
}

/**
 * collection_remove - Low-level removal of collection entry
 * @ch: Character owning the collection
 * @entry: Exact spell entry to remove
 * @class: Class whose collection contains the entry
 * 
 * Low-level function that removes a fully prepared spell from
 * the collection. This happens when a spell is cast or forgotten.
 * 
 * The debug output shows the full spell data being removed, which
 * is useful for tracking spell usage patterns.
 * 
 * Should not be called directly - use collection_remove_by_class()
 * 
 * Safety: Calls core_dump() if collection is unexpectedly NULL,
 * with extra debug output in DEBUGMODE
 */
void collection_remove(struct char_data *ch, struct prep_collection_spell_data *entry,
                       int class)
{
  struct prep_collection_spell_data *temp;

  /* Debug trace if enabled */
  if (DEBUGMODE)
  {
    send_to_char(ch, "{entered collection_remove(), variables: caster: %s, spellnum: %d, domain: %d, prep-time: %d, meta-magic: %d}    ",
                 GET_NAME(ch), entry->spell, entry->domain, entry->prep_time, entry->metamagic);
  }

  /* Sanity check - collection should exist */
  if (SPELL_COLLECTION(ch, class) == NULL)
  {
    if (DEBUGMODE)
    {
      send_to_char(ch, "{!!!core_dump() in collection_remove()!!!}    ");
    }
    core_dump();
    return;
  }
  
  /* Remove from collection and free */
  REMOVE_FROM_LIST(entry, SPELL_COLLECTION(ch, class), next);
  free(entry);
}
/**
 * known_spells_remove - Low-level removal of known spell entry
 * @ch: Character losing spell knowledge
 * @entry: Exact spell entry to remove
 * @class: Class whose known spells contains the entry
 * 
 * Low-level function that removes a spell from a spontaneous
 * caster's known spell list. This is permanent spell loss.
 * 
 * Should not be called directly - use known_spells_remove_by_class()
 * 
 * Safety: Calls core_dump() if list is unexpectedly NULL
 */
void known_spells_remove(struct char_data *ch, struct known_spell_data *entry,
                         int class)
{
  struct known_spell_data *temp;

  /* Sanity check */
  if (KNOWN_SPELLS(ch, class) == NULL)
  {
    core_dump();
    return;
  }
  
  /* Remove from known list and free */
  REMOVE_FROM_LIST(entry, KNOWN_SPELLS(ch, class), next);
  free(entry);
}

/**
 * prep_queue_add - Add spell to preparation queue
 * @ch: Character preparing the spell
 * @ch_class: Class to prepare spell for
 * @spellnum: Spell number to prepare
 * @metamagic: Metamagic flags to apply
 * @prep_time: Time in seconds to prepare
 * @domain: Domain if this is a domain spell
 * 
 * Creates a new preparation queue entry and adds it to the head
 * of the linked list. The spell will count down prep_time seconds
 * before moving to the collection.
 * 
 * Note: Adds to head of list, so newest preparations are processed
 * last (LIFO order allows prioritization of older spells)
 */
void prep_queue_add(struct char_data *ch, int ch_class, int spellnum, int metamagic,
                    int prep_time, int domain)
{
  struct prep_collection_spell_data *entry;

  /* Allocate new entry */
  CREATE(entry, struct prep_collection_spell_data, 1);
  
  /* Fill in spell data */
  entry->spell = spellnum;
  entry->metamagic = metamagic;
  entry->prep_time = prep_time;
  entry->domain = domain;
  
  /* Add to head of list */
  entry->next = SPELL_PREP_QUEUE(ch, ch_class);
  SPELL_PREP_QUEUE(ch, ch_class) = entry;
}
/**
 * innate_magic_add - Add spell slot to innate magic queue
 * @ch: Character gaining the slot
 * @ch_class: Class to add slot for (must be spontaneous caster)
 * @circle: Spell circle of the slot
 * @metamagic: Metamagic flags (rarely used for slots)
 * @prep_time: Time until slot is ready
 * @domain: Domain (usually DOMAIN_UNDEFINED for innate)
 * 
 * For spontaneous casters, adds a spell slot that's recovering.
 * Once prep_time reaches 0, the slot becomes available for
 * casting any known spell of that circle.
 * 
 * Unlike preparation casters who prepare specific spells,
 * spontaneous casters just need available slots by circle.
 */
void innate_magic_add(struct char_data *ch, int ch_class, int circle, int metamagic,
                      int prep_time, int domain)
{
  struct innate_magic_data *entry;

  /* Allocate new slot entry */
  CREATE(entry, struct innate_magic_data, 1);
  
  /* Fill in slot data */
  entry->circle = circle;
  entry->metamagic = metamagic;
  entry->prep_time = prep_time;
  entry->domain = domain;
  
  /* Add to head of list */
  entry->next = INNATE_MAGIC(ch, ch_class);
  INNATE_MAGIC(ch, ch_class) = entry;
}
/**
 * collection_add - Add prepared spell to collection
 * @ch: Character who prepared the spell
 * @ch_class: Class the spell is prepared for
 * @spellnum: Spell number that's ready
 * @metamagic: Metamagic flags applied
 * @prep_time: Usually 0 (spell is ready)
 * @domain: Domain if this is a domain spell
 * 
 * Adds a fully prepared spell to the collection, making it
 * available for immediate casting. This is called when a spell
 * finishes preparation or when loading from saved data.
 * 
 * The collection represents the character's "spell book" of
 * ready-to-cast spells. Once cast, spells are removed from here.
 * 
 * Note: The comment saying "prep-queue" is incorrect - this adds
 * to the collection, not the prep queue.
 */
void collection_add(struct char_data *ch, int ch_class, int spellnum, int metamagic,
                    int prep_time, int domain)
{
  struct prep_collection_spell_data *entry;

  /* Allocate new collection entry */
  CREATE(entry, struct prep_collection_spell_data, 1);
  
  /* Fill in prepared spell data */
  entry->spell = spellnum;
  entry->metamagic = metamagic;
  entry->prep_time = prep_time;  /* Usually 0 for ready spells */
  entry->domain = domain;
  
  /* Add to head of collection */
  entry->next = SPELL_COLLECTION(ch, ch_class);
  SPELL_COLLECTION(ch, ch_class) = entry;
}
/**
 * known_spells_add - Add spell to spontaneous caster's known list
 * @ch: Character learning the spell
 * @ch_class: Class to learn spell for
 * @spellnum: Spell number to learn
 * @loading: TRUE if loading from file (skip validation)
 * 
 * For spontaneous casters (Sorcerer, Bard, etc.), adds a spell
 * to their permanent spell knowledge. The character can then cast
 * this spell using any available slot of the appropriate circle.
 * 
 * When not loading, validates that the character hasn't exceeded
 * their maximum spells known for that circle/level. Each class
 * has different limits based on class tables.
 * 
 * Note: Known spells don't store metamagic or domain info since
 * those are applied at casting time, not learning time.
 * 
 * Returns: TRUE if spell was added, FALSE if at limit or invalid
 */
bool known_spells_add(struct char_data *ch, int ch_class, int spellnum, bool loading)
{
  int circle = compute_spells_circle(ch, ch_class, spellnum, METAMAGIC_NONE, DOMAIN_UNDEFINED);
  int caster_level = CLASS_LEVEL(ch, ch_class) + BONUS_CASTER_LEVEL(ch, ch_class);

  /* When not loading, check if character can learn more spells */
  if (!loading)
  {
    switch (ch_class)
    {
    case CLASS_WARLOCK:
    case CLASS_BARD:
      /* Check against bard spells known table */
      if (bard_known[caster_level][circle] - count_known_spells_by_circle(ch, ch_class, circle) <= 0)
        return FALSE;
      break;
    case CLASS_SUMMONER:
      /* Check against summoner spells known table */
      if (summoner_known[caster_level][circle] - count_known_spells_by_circle(ch, ch_class, circle) <= 0)
        return FALSE;
      break;
    case CLASS_INQUISITOR:
      /* Check against inquisitor spells known table */
      if (inquisitor_known[caster_level][circle] - count_known_spells_by_circle(ch, ch_class, circle) <= 0)
        return FALSE;
      break;
    case CLASS_SORCERER:
      /* Sorcerers use spell slots as their known spell limit */
      if (compute_slots_by_circle(ch, ch_class, circle) -
              count_known_spells_by_circle(ch, ch_class, circle) <= 0)
        return FALSE;
      break;
    case CLASS_PSIONICIST:
      /* Psionicists have a total power limit, not per-circle */
      if ((num_psionicist_powers_available(ch) - num_psionicist_powers_known(ch)) <= 0)
        return FALSE;
      break;
    }
  }

  struct known_spell_data *entry;

  /* Create and initialize the known spell entry */
  CREATE(entry, struct known_spell_data, 1);
  entry->spell = spellnum;
  entry->metamagic = METAMAGIC_NONE;  /* Not used for known spells */
  entry->prep_time = 0;               /* Not used for known spells */
  entry->domain = DOMAIN_UNDEFINED;   /* Not used for known spells */
  
  /* Add to head of known spells list */
  entry->next = KNOWN_SPELLS(ch, ch_class);
  KNOWN_SPELLS(ch, ch_class) = entry;

  return TRUE;
}

/**
 * load_spell_prep_queue - Load all prep queues from file
 * @fl: Open file pointer to read from
 * @ch: Character to load data into
 * 
 * Reads preparation queue data from player file during login.
 * Each line contains: class spell_num metamagic prep_time domain
 * Continues reading until sentinel (-1 -1 -1 -1 -1) or MAX_MEM limit.
 * 
 * This restores any spells that were being prepared when the
 * character last logged out, maintaining their preparation progress.
 * 
 * Note: Ideally this function belongs in players.c with other
 * load functions, but is kept here for organizational purposes.
 */
void load_spell_prep_queue(FILE *fl, struct char_data *ch)
{
  int spell_num, ch_class, metamagic, prep_time, domain, counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  /* Read entries until sentinel or limit reached */
  do
  {
    /* Initialize variables for safety */
    ch_class = 0;
    spell_num = 0;
    metamagic = 0;
    prep_time = 0;
    domain = 0;

    /* Read next line from file */
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d", &ch_class, &spell_num, &metamagic, &prep_time,
           &domain);

    /* -1 is the sentinel value marking end of section */
    if (ch_class != -1)
      prep_queue_add(ch, ch_class, spell_num, metamagic, prep_time, domain);

    counter++;
  } while (counter < MAX_MEM && spell_num != -1);
}
/**
 * load_innate_magic_queue - Load all innate magic slots from file
 * @fl: Open file pointer to read from
 * @ch: Character to load data into
 * 
 * Reads innate magic slot data for spontaneous casters from player file.
 * Each line contains: class circle metamagic prep_time domain
 * Note that circle replaces spell_num since spontaneous casters prepare
 * slots by circle, not specific spells.
 * 
 * Continues until sentinel (-1 -1 -1 -1 -1) or MAX_MEM limit.
 * Restores any spell slots that were still recovering at logout.
 */
void load_innate_magic_queue(FILE *fl, struct char_data *ch)
{
  int circle, ch_class, metamagic, prep_time, domain, counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  /* Read entries until sentinel or limit */
  do
  {
    /* Initialize for safety */
    ch_class = 0;
    circle = 0;
    metamagic = 0;
    prep_time = 0;
    domain = 0;

    /* Read next line */
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d", &ch_class, &circle, &metamagic, &prep_time,
           &domain);

    /* -1 marks end of section */
    if (ch_class != -1)
      innate_magic_add(ch, ch_class, circle, metamagic, prep_time, domain);

    counter++;
  } while (counter < MAX_MEM && circle != -1);
}
/**
 * load_spell_collection - Load all prepared spells from file
 * @fl: Open file pointer to read from
 * @ch: Character to load data into
 * 
 * Reads prepared spell collection data from player file during login.
 * Each line contains: class spell_num metamagic prep_time domain
 * These are fully prepared spells ready to cast immediately.
 * 
 * Continues until sentinel (-1 -1 -1 -1 -1) or MAX_MEM limit.
 * Restores the character's "spell book" of ready spells.
 */
void load_spell_collection(FILE *fl, struct char_data *ch)
{
  int spell_num, ch_class, metamagic, prep_time, domain, counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  /* Read entries until sentinel or limit */
  do
  {
    /* Initialize for safety */
    ch_class = 0;
    spell_num = 0;
    metamagic = 0;
    prep_time = 0;    /* Usually 0 for collection */
    domain = 0;

    /* Read next line */
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d", &ch_class, &spell_num, &metamagic, &prep_time,
           &domain);

    /* -1 marks end of section */
    if (ch_class != -1)
      collection_add(ch, ch_class, spell_num, metamagic, prep_time, domain);

    counter++;
  } while (counter < MAX_MEM && spell_num != -1);
}
/**
 * load_known_spells - Load all known spell lists from file
 * @fl: Open file pointer to read from
 * @ch: Character to load data into
 * 
 * Reads known spell data for spontaneous casters from player file.
 * Each line contains only: class spell_num (simpler than other formats)
 * No metamagic/prep_time/domain needed for spell knowledge.
 * 
 * Continues until sentinel (-1 -1) or MAX_MEM limit.
 * The TRUE parameter to known_spells_add() skips validation checks.
 */
void load_known_spells(FILE *fl, struct char_data *ch)
{
  int spell_num, ch_class, counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  /* Read entries until sentinel or limit */
  do
  {
    /* Initialize for safety */
    ch_class = 0;
    spell_num = 0;

    /* Read next line */
    get_line(fl, line);
    sscanf(line, "%d %d", &ch_class, &spell_num);

    /* -1 marks end of section */
    if (ch_class != -1)
      known_spells_add(ch, ch_class, spell_num, TRUE);  /* TRUE = loading, skip checks */

    counter++;
  } while (counter < MAX_MEM && ch_class != -1);
}

/**
 * count_circle_prep_queue - Count spells of given circle in prep queue
 * @ch: Character to check
 * @class: Class to examine
 * @circle: Spell circle to count
 * 
 * Counts how many spells in the preparation queue belong to the
 * specified circle. Must calculate effective circle for each spell
 * since metamagic can modify the base circle.
 * 
 * Used to determine if character has room for more spells of this
 * circle based on their slot limits.
 * 
 * Returns: Number of spells of that circle being prepared
 */
int count_circle_prep_queue(struct char_data *ch, int class, int circle)
{
  int this_circle = 0, counter = 0;
  struct prep_collection_spell_data *current = SPELL_PREP_QUEUE(ch, class);
  struct prep_collection_spell_data *next;

  /* Walk the prep queue */
  for (; current; current = next)
  {
    next = current->next;
    
    /* Calculate effective circle including metamagic */
    this_circle = compute_spells_circle(ch, class,
                                        current->spell,
                                        current->metamagic,
                                        current->domain);
    
    /* Count if it matches our target circle */
    if (this_circle == circle)
      counter++;
  }

  return counter;
}
/**
 * count_circle_innate_magic - Count spell slots of given circle for spontaneous casters
 * @ch: Character to check
 * @class: Class to examine (should be spontaneous caster)
 * @circle: Spell circle to count
 * 
 * For spontaneous casters (Sorcerer, Bard, etc.), counts how many spell
 * slots of the specified circle are currently in the innate magic queue.
 * These represent slots that are still recovering and not yet available.
 * 
 * Unlike preparation casters who track specific spells, spontaneous
 * casters only track slots by circle level.
 * 
 * Returns: Number of slots of that circle still recovering
 */
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
/**
 * count_circle_collection - Count prepared spells of given circle
 * @ch: Character to check
 * @class: Class to examine
 * @circle: Spell circle to count
 * 
 * Counts how many fully prepared spells of the specified circle
 * are in the character's collection (ready to cast). Must calculate
 * the effective circle for each spell since metamagic can modify it.
 * 
 * This is used to determine spell slot availability and whether
 * the character has room to prepare more spells of this circle.
 * 
 * Note: For preparation casters only - spontaneous casters don't
 * use collections in the same way.
 * 
 * Returns: Number of prepared spells of that circle
 */
int count_circle_collection(struct char_data *ch, int class, int circle)
{
  int this_circle = 0, counter = 0;
  struct prep_collection_spell_data *current = SPELL_COLLECTION(ch, class);
  struct prep_collection_spell_data *next;

  for (; current; current = next)
  {
    next = current->next;
    this_circle = compute_spells_circle(ch, class,
                                        current->spell,
                                        current->metamagic,
                                        current->domain);
    if (this_circle == circle)
      counter++;
  }

  return counter;
}
/**
 * count_known_spells_by_circle - Count known spells by circle for spontaneous casters
 * @ch: Character to check
 * @class: Spontaneous caster class (Sorcerer, Bard, etc.)
 * @circle: Spell circle to count (1-9)
 * 
 * For spontaneous casters, counts how many spells they know of a
 * specific circle. This is used to check against class tables that
 * limit spells known per circle.
 * 
 * Special handling:
 * - Sorcerers: Excludes bloodline spells (they're bonus spells)
 * - Other classes: Simple count of spells at that circle
 * 
 * Each spontaneous caster class has different limits:
 * - Sorcerers: Use spell slots as their limit
 * - Bards/Summoners/Inquisitors: Use specific "spells known" tables
 * 
 * Returns: Number of non-bonus spells known at that circle
 */
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
      if (compute_spells_circle(ch, class, current->spell, 0, 0) == circle &&
          !is_sorc_bloodline_spell(get_sorc_bloodline(ch), current->spell))
        counter++;
      break;
    case CLASS_WARLOCK:
    case CLASS_BARD:
    case CLASS_INQUISITOR:
    case CLASS_SUMMONER:
      if (compute_spells_circle(ch, class, current->spell, 0, 0) == circle)
        counter++;
      break;
    } /*end switch*/
  }   /*end slot loop*/

  return counter;
}

/**
 * num_psionicist_powers_known - Count total psionic powers known
 * @ch: Character to check (should have psionicist levels)
 * 
 * Psionicists have a different system than other casters - they
 * have a total limit on powers known across all circles, rather
 * than per-circle limits.
 * 
 * Only counts valid psionic powers (between PSIONIC_POWER_START
 * and PSIONIC_POWER_END). Other entries are ignored.
 * 
 * Used with num_psionicist_powers_available() to determine if
 * the psionicist can learn more powers.
 * 
 * Returns: Total number of psionic powers known
 */
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

  } /*end slot loop*/

  return counter;
}

/**
 * num_psionicist_powers_available - Calculate max powers a psionicist can know
 * @ch: Character to check
 * 
 * Calculates the total number of psionic powers a character can
 * know based on their psionicist level and various bonuses.
 * 
 * Formula:
 * - Base: 1 power
 * - +2 powers per psionicist level (up to level 20)
 * - +Intelligence bonus
 * - +1 per rank of Expanded Knowledge feat
 * 
 * This represents the character's mental capacity to hold
 * psionic knowledge. Compare with num_psionicist_powers_known()
 * to see if they can learn more.
 * 
 * Returns: Maximum number of powers the psionicist can know
 */
int num_psionicist_powers_available(struct char_data *ch)
{
  int i = 0, level = GET_PSIONIC_LEVEL(ch), num_powers = 1;

  num_powers += GET_REAL_INT_BONUS(ch);

  num_powers += HAS_FEAT(ch, FEAT_EXPANDED_KNOWLEDGE);

  for (i = 1; i <= MIN(20, level); i++)
    num_powers += 2;

  return num_powers;
}

/**
 * count_total_slots - Count all spell slots in use for a given circle
 * @ch: Character to check
 * @class: Class to examine
 * @circle: Spell circle to count
 * 
 * Sums up all spell slots currently allocated for a specific circle
 * across all three spell management systems:
 * - Collection: Fully prepared spells ready to cast
 * - Innate Magic: Recovering spell slots (spontaneous casters)
 * - Prep Queue: Spells currently being prepared
 * 
 * This total is compared against compute_slots_by_circle() to
 * determine if the character has free slots to prepare/recover
 * more spells of this circle.
 * 
 * Returns: Total slots in use for this circle
 */
int count_total_slots(struct char_data *ch, int class, int circle)
{

  int total_slots = 0;

  total_slots = (count_circle_collection(ch, class, circle) +
          count_circle_innate_magic(ch, class, circle) +
          count_circle_prep_queue(ch, class, circle));
  return total_slots;
}

/**
 * is_spell_in_prep_queue - Check if spell is being prepared
 * @ch: Character to check
 * @class: Class to check preparation for
 * @spellnum: Spell number to find
 * @metamagic: Metamagic flags that must match exactly
 * 
 * Searches the preparation queue to see if a specific spell
 * with exact metamagic combination is currently being prepared.
 * 
 * Used to:
 * - Prevent duplicate preparations
 * - Check preparation status for display
 * - Validate spell availability
 * 
 * Note: Requires exact metamagic match - a quickened fireball
 * is different from a normal fireball.
 * 
 * Returns: TRUE if spell+metamagic found in prep queue, FALSE otherwise
 */
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
/**
 * is_in_innate_magic_queue - Check if spell slot is recovering
 * @ch: Character to check
 * @class: Spontaneous caster class to check
 * @circle: Spell circle to find
 * 
 * For spontaneous casters, checks if any spell slot of the
 * specified circle is currently in the recovery queue.
 * 
 * Unlike preparation casters who track specific spells,
 * spontaneous casters only need to know if they have slots
 * of a given circle recovering.
 * 
 * Used to:
 * - Display slot recovery status
 * - Prevent over-allocation of slots
 * - Check casting availability
 * 
 * Returns: TRUE if any slot of that circle is recovering, FALSE otherwise
 */
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
/**
 * is_spell_in_collection - Check if spell is prepared and ready
 * @ch: Character to check
 * @class: Class to check collection for
 * @spellnum: Spell number to find
 * @metamagic: Metamagic flags that must match exactly
 * 
 * Searches the spell collection to see if a specific spell with
 * exact metamagic combination is prepared and ready to cast.
 * 
 * The collection holds fully prepared spells that can be cast
 * immediately. This is the primary check used by the casting
 * system to validate spell availability.
 * 
 * Used by:
 * - spell_prep_gen_check() to validate casting
 * - spell_prep_gen_extract() to move spell when cast
 * - Display functions to show available spells
 * 
 * Note: The comment about "search mode" appears outdated - this
 * function always requires exact metamagic match.
 * 
 * Returns: TRUE if spell+metamagic found in collection, FALSE otherwise
 */
bool is_spell_in_collection(struct char_data *ch, int class, int spellnum,
                            int metamagic)
{
  struct prep_collection_spell_data *current = SPELL_COLLECTION(ch, class);
  struct prep_collection_spell_data *next;

  if (DEBUGMODE)
  {
    send_to_char(ch, "{entered is_spell_in_collection()}    ");
  }

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
/**
 * is_a_known_spell - Check if spontaneous caster knows a spell
 * @ch: Character to check
 * @class: Spontaneous caster class
 * @spellnum: Spell to check for
 * 
 * For spontaneous casters, determines if they have permanent
 * knowledge of a spell and can cast it using available slots.
 * 
 * Special cases handled:
 * - Sorcerer bloodline spells (automatic knowledge)
 * - Cleric/Inquisitor domain spells (bonus knowledge)
 * - Psionicist epic powers
 * - NPC psionicists (automatic knowledge)
 * 
 * This is the key difference between spontaneous and preparation
 * casters - spontaneous casters have permanent spell knowledge
 * and use slots flexibly, while preparation casters must prepare
 * specific spells in advance.
 * 
 * Note: Missing break after CLASS_CLERIC case may cause fall-through
 * bug where clerics check epic powers.
 * 
 * Returns: TRUE if character knows the spell, FALSE otherwise
 */
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
  if (class == CLASS_PSIONICIST && IS_NPC(ch) && GET_CLASS(ch) == CLASS_PSIONICIST)
    return TRUE;


  switch (class)
  {
  /* bloodline system for sorcerer */
  case CLASS_SORCERER:
    if (is_sorc_bloodline_spell(get_sorc_bloodline(ch), spellnum))
      return TRUE;
    break;
  case CLASS_INQUISITOR:
  case CLASS_CLERIC:
    if (is_domain_spell_of_ch(ch, spellnum))
      return TRUE;
    break;  /* BUG FIX: Added missing break to prevent fall-through */
  case CLASS_PSIONICIST:
    if (has_epic_power(ch, spellnum))
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

/**
 * is_sorc_bloodline_spell - Check if a spell is granted by sorcerer bloodline
 * @bloodline: The bloodline type (SORC_BLOODLINE_DRACONIC, etc.)
 * @spellnum: The spell number to check
 * 
 * Sorcerer bloodlines grant bonus spells that don't count against spells known.
 * Each bloodline provides thematic spells at specific levels:
 * - Draconic: Dragon-themed spells (mage armor, fly, etc.)
 * - Arcane: Pure magic spells (identify, dispel magic, etc.)
 * - Fey: Enchantment/illusion spells (charm, hideous laughter, etc.)
 * - Undead: Necromancy spells (chill touch, animate dead, etc.)
 * 
 * These spells are automatically known and don't use up spell selections.
 * 
 * Returns: TRUE if the spell is a bloodline spell, FALSE otherwise
 */
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
      // case SPELL_FEAR: // Not implemented yet
    case SPELL_WIZARD_EYE: // replace with fear when imp'd
    case SPELL_TELEKINESIS:
      // case SPELL_FORM_OF_THE_DRAGON_I: // Not implemented yet
      // case SPELL_FORM_OF_THE_DRAGON_II: // Not implemented yet
      // case SPELL_FORM_OF_THE_DRAGON_III: // Not implemented yet
      // case SPELL_WISH: // Not implemented yet
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
      // case SPELL_DIMENSION_DOOR: // Not implemented yet
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
    case SPELL_CHARM_MONSTER:
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

/**
 * get_sorc_bloodline - Determine which bloodline a sorcerer has
 * @ch: Character to check
 * 
 * Checks the character's feats to determine their sorcerer bloodline.
 * A sorcerer must choose a bloodline at 1st level, which grants:
 * - Bonus spells at specific levels
 * - Special bloodline powers
 * - Thematic abilities related to their heritage
 * 
 * Returns: Bloodline type constant, or SORC_BLOODLINE_NONE if no bloodline
 */
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

/**
 * compute_spells_circle - Calculate which circle a spell belongs to
 * @ch: Character casting/preparing the spell
 * @char_class: Class being used to cast
 * @spellnum: Base spell number
 * @metamagic: Metamagic flags that modify the circle
 * @domain: Domain for clerics (can reduce spell level)
 * 
 * This is one of the most important functions in the spell system.
 * It converts from the spell level system (1-20) to spell circles (1-9).
 * 
 * The complexity exists because:
 * 1. Different classes get the same spell at different levels
 * 2. Metamagic increases the effective circle of a spell
 * 3. Domain spells may be available at lower levels for clerics
 * 4. Some classes (bard, ranger, etc.) have compressed spell progressions
 * 5. Automatic metamagic feats can reduce the circle increase
 * 
 * Returns: Spell circle (1-9) or NUM_CIRCLES+1 if invalid/too high
 */
int compute_spells_circle(struct char_data *ch, int char_class, int spellnum, int metamagic, int domain)
{
  int metamagic_mod = 0;  /* Circle adjustment from metamagic */
  int spell_circle = 0;   /* Final calculated circle */
  int min_level = 0;      /* Minimum level to cast this spell */

  /* Validate spell number based on class type 
   * Different classes have different spell number ranges:
   * - Standard casters: Use normal spell range (SPELL_RESERVED_DBC to NUM_SPELLS)
   * - Psionicists: Use psionic power range (PSIONIC_POWER_START to PSIONIC_POWER_END)
   * - Warlocks: Use invocation range (WARLOCK_POWER_START to WARLOCK_POWER_END)
   * 
   * Return NUM_CIRCLES+1 (invalid) if spell is outside valid range for the class
   */
  if (char_class != CLASS_WARLOCK && char_class != CLASS_PSIONICIST && 
      (spellnum <= SPELL_RESERVED_DBC || spellnum >= NUM_SPELLS))
    return (NUM_CIRCLES + 1);  /* Invalid spell number */
  else if (char_class == CLASS_PSIONICIST && 
           (spellnum < PSIONIC_POWER_START || spellnum > PSIONIC_POWER_END))
    return (NUM_CIRCLES + 1);  /* Not a valid psionic power */
  else if (char_class == CLASS_WARLOCK && 
           (spellnum < WARLOCK_POWER_START || spellnum > WARLOCK_POWER_END))
    return (NUM_CIRCLES + 1);  /* Not a valid warlock invocation */

#ifdef CAMPAIGN_FR
  switch (spellnum)
  {
  case SPELL_LUSKAN_RECALL:
  case SPELL_MIRABAR_RECALL:
  case SPELL_TRIBOAR_RECALL:
  case SPELL_SILVERYMOON_RECALL:
    return 5;
  }
#elif defined(CAMPAIGN_DL)
switch (spellnum)
  {
  case SPELL_PALANTHAS_RECALL:
  case SPELL_SANCTION_RECALL:
  case SPELL_SOLACE_RECALL:
    return 5;
  case SPELL_MINOR_RAPID_BUFF:
    return 3;
  case SPELL_RAPID_BUFF:
    return 5;
  case SPELL_GREATER_RAPID_BUFF:
    return 7;
  }
#endif

  /* Calculate total metamagic circle adjustments
   * Each metamagic feat increases the effective spell circle:
   * - Quicken: +4 circles (cast as swift action - very powerful)
   * - Maximize: +3 circles (all variable numeric effects maximized)
   * - Empower: +2 circles (all variable numeric effects increased by 50%)
   * - Extend: +1 circle (doubles duration of spells)
   * - Still: +1 circle (cast without somatic components)
   * - Silent: +1 circle (cast without verbal components)
   * 
   * Multiple metamagics stack, so a quickened, maximized fireball would
   * be +7 circles (3rd circle base + 4 + 3 = 10th circle - impossible!)
   */
  if (IS_SET(metamagic, METAMAGIC_QUICKEN))
    metamagic_mod += 4;
  if (IS_SET(metamagic, METAMAGIC_MAXIMIZE))
    metamagic_mod += 3;
  if (IS_SET(metamagic, METAMAGIC_EMPOWER))
    metamagic_mod += 2;
  if (IS_SET(metamagic, METAMAGIC_EXTEND))
    metamagic_mod += 1;
  if (IS_SET(metamagic, METAMAGIC_STILL))
    metamagic_mod += 1;
  if (IS_SET(metamagic, METAMAGIC_SILENT))
    metamagic_mod += 1;

  /* Class-specific spell level to circle conversions
   * Each class has different spell progression rates:
   * - Full casters (Wizard, Cleric, Druid): Get new spell levels every 2 character levels
   * - 3/4 casters (Bard, Alchemist): Slower progression, max 6th circle
   * - Half casters (Paladin, Ranger): Very slow, max 4th circle
   * - Spontaneous casters may have different progressions than prepared
   */
  switch (char_class)
  {
  case CLASS_ALCHEMIST:
    /* Alchemists are 3/4 casters with max 6th circle extracts
     * Level progression: 1-3 = 1st, 4-6 = 2nd, 7-9 = 3rd, etc.
     * Automatic metamagic feats reduce the circle increase for low-level spells
     */
    switch (spell_info[spellnum].min_level[char_class])
    {
    case 1:
    case 2:
    case 3:
      /* First circle spells - automatic metamagic applies */
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 1 + metamagic_mod;
    case 4:
    case 5:
    case 6:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 2 + metamagic_mod;
    case 7:
    case 8:
    case 9:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
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
    switch (spell_info[spellnum].min_level[char_class])
    {
    case 1:
    case 2:
    case 3:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 1 + metamagic_mod;
    case 4:
    case 5:
    case 6:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 2 + metamagic_mod;
    case 7:
    case 8:
    case 9:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
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
  case CLASS_INQUISITOR:
    min_level = MIN_SPELL_LVL(spellnum, char_class, domain);
    switch (min_level)
    {
    case 1:
    case 2:
    case 3:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 1 + metamagic_mod;
    case 4:
    case 5:
    case 6:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 2 + metamagic_mod;
    case 7:
    case 8:
    case 9:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
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
  case CLASS_SUMMONER:
    min_level = MIN_SPELL_LVL(spellnum, char_class, domain);
    switch (min_level)
    {
    case 1:
    case 2:
    case 3:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 1 + metamagic_mod;
    case 4:
    case 5:
    case 6:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 2 + metamagic_mod;
    case 7:
    case 8:
    case 9:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
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
    /* Paladins and Blackguards are half-casters (max 4th circle)
     * They don't get spells until 6th level, then progress slowly:
     * Levels 6-9 = 1st circle, 10-11 = 2nd, 12-14 = 3rd, 15+ = 4th
     * This reflects their martial focus with divine magic support
     */
    switch (spell_info[spellnum].min_level[char_class])
    {
    case 6:
    case 7:
    case 8:
    case 9:
      /* 1st circle spells - gained at 6th level */
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 1 + metamagic_mod;
    case 10:
    case 11:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 2 + metamagic_mod;
    case 12:
    case 13:
    case 14:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
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
    switch (spell_info[spellnum].min_level[char_class])
    {
    case 6:
    case 7:
    case 8:
    case 9:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 1 + metamagic_mod;
    case 10:
    case 11:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 2 + metamagic_mod;
    case 12:
    case 13:
    case 14:
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
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
  case CLASS_WARLOCK:
    spell_circle = spell_info[spellnum].min_level[char_class];
    if (spell_circle <= 5)
    {
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 1;
    }
      
    else if (spell_circle <= 10)
    {
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 2;
    }
    else if (spell_circle <= 15)
    {
      if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL))
        metamagic_mod -= 4;
      if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL))
        metamagic_mod -= 1;
      if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL))
        metamagic_mod -= 1;
      return 3;
    }
    return 4;
  case CLASS_SORCERER:
    /* Sorcerers are full spontaneous arcane casters
     * Simple formula: spell level / 2 = circle (rounded down)
     * They get spells one level later than wizards but cast spontaneously
     * Example: 1st level spells at char level 2 (2/2 = 1st circle)
     */
    spell_circle = spell_info[spellnum].min_level[char_class] / 2;
    /* Automatic metamagic only applies to spells of 3rd circle or lower */
    if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL) && spell_circle <= 3)
    {
        metamagic_mod -= 4;
    }
    if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL) && spell_circle <= 3)
      metamagic_mod -= 1;
    if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL) && spell_circle <= 3)
      metamagic_mod -= 1;
    spell_circle += metamagic_mod;
    /* Check if metamagic pushed spell beyond 9th circle */
    if (spell_circle > TOP_CIRCLE)
    {
      return (NUM_CIRCLES + 1);
    }
    return (MAX(1, spell_circle));
  case CLASS_CLERIC:
    /* Clerics are full divine prepared casters with domain spells
     * Formula: (spell level + 1) / 2 = circle
     * Domain spells may be available at lower levels than normal
     * MIN_SPELL_LVL checks both normal and domain spell levels
     */
    spell_circle = (MIN_SPELL_LVL(spellnum, char_class, domain) + 1) / 2;
    if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL) && spell_circle <= 3)
    {
        metamagic_mod -= 4;
    }
    if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL) && spell_circle <= 3)
      metamagic_mod -= 1;
    if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL) && spell_circle <= 3)
      metamagic_mod -= 1;
    spell_circle += metamagic_mod;
    if (spell_circle > TOP_CIRCLE)
    {
      return (NUM_CIRCLES + 1);
    }
    return (MAX(1, spell_circle));
  case CLASS_WIZARD:
    spell_circle = (spell_info[spellnum].min_level[char_class] + 1) / 2;
    if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL) && spell_circle <= 3)
    {
        metamagic_mod -= 4;
    }
    if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL) && spell_circle <= 3)
      metamagic_mod -= 1;
    if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL) && spell_circle <= 3)
      metamagic_mod -= 1;
    spell_circle += metamagic_mod;
    if (spell_circle > TOP_CIRCLE)
    {
      return (NUM_CIRCLES + 1);
    }
    return (MAX(1, spell_circle));
  case CLASS_DRUID:
    spell_circle = (spell_info[spellnum].min_level[char_class] + 1) / 2;
    if (IS_SET(metamagic, METAMAGIC_QUICKEN) && HAS_FEAT(ch, FEAT_AUTOMATIC_QUICKEN_SPELL) && spell_circle <= 3)
    {
        metamagic_mod -= 4;
    }
    if (IS_SET(metamagic, METAMAGIC_STILL) && HAS_FEAT(ch, FEAT_AUTOMATIC_STILL_SPELL) && spell_circle <= 3)
      metamagic_mod -= 1;
    if (IS_SET(metamagic, METAMAGIC_SILENT) && HAS_FEAT(ch, FEAT_AUTOMATIC_SILENT_SPELL) && spell_circle <= 3)
      metamagic_mod -= 1;
    spell_circle += metamagic_mod;
    if (spell_circle > TOP_CIRCLE)
    {
      return (NUM_CIRCLES + 1);
    }
    return (MAX(1, spell_circle));
  case CLASS_PSIONICIST:
    spell_circle = (spell_info[spellnum].min_level[char_class] + 1) / 2;
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
  if (!IS_CASTER(ch) && !IS_PSIONIC(ch) && GET_WARLOCK_LEVEL(ch) == 0)
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
  case CLASS_WARLOCK:
    if (class_level <= 5)
      return 1;
    else if (class_level <= 10)
      return 2;
    else if (class_level <= 15)
      return 3;
    return 4;
  case CLASS_BARD:
  case CLASS_INQUISITOR:
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
  case CLASS_SUMMONER:
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

/**
 * ready_to_prep_spells - Check if character can prepare spells
 * @ch: Character to check
 * @class: Class to prepare spells for
 * 
 * Validates all conditions required for spell preparation:
 * 
 * CLASS-SPECIFIC CHECKS:
 * - Wizards must have their spellbook or a scroll for the spell
 * 
 * POSITION REQUIREMENTS:
 * - Must be resting (sitting or lying down)
 * - Cannot be fighting
 * 
 * DEBUFF RESTRICTIONS:
 * The following conditions prevent preparation:
 * - Stunned: Can't concentrate
 * - Paralyzed: Can't move or speak
 * - Grappled: Physically restrained
 * - Nauseated: Too sick to concentrate
 * - Confused: Mind is scrambled
 * - Dazed: Mentally incapacitated
 * - Pinned: Completely immobilized
 * 
 * This function is called every pulse during preparation to ensure
 * the character maintains proper conditions throughout.
 * 
 * Returns: TRUE if can prepare, FALSE if any condition fails
 */
bool ready_to_prep_spells(struct char_data *ch, int class)
{

  switch (class)
  {
  case CLASS_WIZARD: /* Wizards need their spellbook or a scroll to study from */
    if (SPELL_PREP_QUEUE(ch, class))
      if (!spellbook_ok(ch, SPELL_PREP_QUEUE(ch, class)->spell, CLASS_WIZARD, FALSE))
        return FALSE;
    break;
  }

  /* Position check - must be resting (not standing, sleeping, or incapacitated) */
  if (GET_POS(ch) != POS_RESTING)
    return FALSE;
  if (FIGHTING(ch))
    return FALSE;

  /* Debuff checks - various conditions that prevent concentration */
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

  /* All conditions met! */
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
  case CLASS_INQUISITOR:
  case CLASS_WARLOCK:
  case CLASS_SUMMONER:
    if (!INNATE_MAGIC(ch, class))
    {
      send_to_char(ch, "You have nothing in your innate magic queue for this class to prepare!\r\n");
      return;
    }
    break;
  default:
    if (!SPELL_PREP_QUEUE(ch, class))
    {
      send_to_char(ch, "You have nothing in your preparation queue for this class to prepare!\r\n");
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
  case CLASS_INQUISITOR:
  case CLASS_WARLOCK:
  case CLASS_SUMMONER:
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

/**
 * is_min_level_for_spell - Check if character meets level requirement for spell
 * @ch: Character to check
 * @class: Class being used to cast/prepare
 * @spellnum: Spell number to check
 * 
 * Determines if the character's level in the specified class is high enough
 * to cast/prepare the spell. This includes:
 * 
 * SPECIAL CASES:
 * - Clerics: Check both domains for potentially lower level access
 *   Example: Cure Light Wounds might be 1st level for Healing domain
 * - Inquisitors: Check their single domain for special access
 * 
 * LEVEL CALCULATION:
 * - Base class level + Bonus caster levels (from prestige classes, etc.)
 * - Must meet or exceed the spell's minimum level for that class
 * 
 * Each spell has different level requirements per class:
 * - Wizards might get Fireball at 5th level (3rd circle)
 * - Sorcerers might get it at 6th level (delayed progression)
 * 
 * Returns: TRUE if level requirement met, FALSE otherwise
 */
bool is_min_level_for_spell(struct char_data *ch, int class, int spellnum)
{
  int min_level = 0;

  switch (class)
  {
  case CLASS_CLERIC: /* Domain system - check both domains for best access */
    min_level = MIN(MIN_SPELL_LVL(spellnum, CLASS_CLERIC, GET_1ST_DOMAIN(ch)),
                    MIN_SPELL_LVL(spellnum, CLASS_CLERIC, GET_2ND_DOMAIN(ch)));
    if ((BONUS_CASTER_LEVEL(ch, class) + CLASS_LEVEL(ch, CLASS_CLERIC)) <
        min_level)
    {
      return FALSE;
    }
    break;
  case CLASS_INQUISITOR: /* Single domain system */
    min_level = MIN_SPELL_LVL(spellnum, CLASS_INQUISITOR, GET_1ST_DOMAIN(ch));
    if ((BONUS_CASTER_LEVEL(ch, class) + CLASS_LEVEL(ch, CLASS_INQUISITOR)) < min_level)
    {
      return FALSE;
    }
    break;
  default:
    /* Standard level check for other classes */
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

  if (!ch)
    return FALSE;

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
  case CLASS_SUMMONER:
    spell_slots += spell_bonus[GET_CHA(ch)][circle];
    spell_slots += summoner_slots[class_level][circle];
    break;
  case CLASS_INQUISITOR:
    spell_slots += spell_bonus[GET_WIS(ch)][circle];
    spell_slots += inquisitor_slots[class_level][circle];
    break;

  default:
    break;
  }

  spell_slots += get_bonus_spells_by_circle_and_class(ch, class, circle);

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
  case CLASS_SUMMONER:
    feat_index = SUM_SLT_0;
    break;
  case CLASS_INQUISITOR:
    feat_index = INQ_SLT_0;
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
      case CLASS_INQUISITOR:
        slots_have[circle_counter] = inquisitor_slots[level_counter][circle_counter];
        slots_had[circle_counter] = inquisitor_slots[level_counter - 1][circle_counter];
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
      case CLASS_SUMMONER:
        slots_have[circle_counter] = summoner_slots[level_counter][circle_counter];
        slots_had[circle_counter] = summoner_slots[level_counter - 1][circle_counter];
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

/**
 * spell_prep_gen_extract - Extract a spell when cast (moves from ready to recovering)
 * @ch: Character casting the spell
 * @spellnum: Spell being cast
 * @metamagic: Metamagic flags applied to the spell
 * 
 * This is THE KEY FUNCTION that connects casting to preparation!
 * Called when a spell is successfully cast, it handles the "using up" of a spell.
 * 
 * FOR PREPARED CASTERS (Wizard, Cleric, etc.):
 * 1. Finds the spell in their collection (prepared spells)
 * 2. Removes it from collection (spell is "used up")
 * 3. Adds it back to prep queue with full prep time
 * 4. The spell must be prepared again before next use
 * 
 * FOR SPONTANEOUS CASTERS (Sorcerer, Bard, etc.):
 * 1. Checks if they know the spell
 * 2. Verifies they have an available slot of that circle
 * 3. Adds a slot of that circle to recovery queue
 * 4. The slot will regenerate over time
 * 
 * SEARCH ORDER:
 * - Checks prepared collections first (faster for mixed classes)
 * - Then checks spontaneous casting ability
 * 
 * Returns: Class that had the spell, or CLASS_UNDEFINED if not found
 */
int spell_prep_gen_extract(struct char_data *ch, int spellnum, int metamagic)
{
  int ch_class = CLASS_UNDEFINED, prep_time = 99,
      circle = TOP_CIRCLE + 1, is_domain = FALSE;

  if (DEBUGMODE)
  {
    send_to_char(ch, "{entered spell_prep_gen_extract()}    ");
  }

  /* FIRST: Check all prepared spell collections */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
  {
    if (is_spell_in_collection(ch, ch_class, spellnum, metamagic))
    {
      /* Found it! Extract from collection */
      if (!collection_remove_by_class(ch, ch_class, spellnum, metamagic))
        return CLASS_UNDEFINED; /* Removal failed - shouldn't happen */

      /* Put spell back in prep queue to be prepared again */
      is_domain = is_domain_spell_of_ch(ch, spellnum);
      circle = compute_spells_circle(ch, ch_class, spellnum, metamagic, is_domain);
      prep_time = compute_spells_prep_time(ch, ch_class, circle, is_domain);
      prep_queue_add(ch, ch_class, spellnum, metamagic, prep_time, is_domain);

      return (ch_class);
    }
  }

  /* SECOND: Check spontaneous casting ability (innate magic) */
  for (ch_class = 0; ch_class < NUM_CLASSES; ch_class++)
  {
    is_domain = is_domain_spell_of_ch(ch, spellnum);
    /* Calculate which circle this spell+metamagic combination requires */
    circle = compute_spells_circle(ch, ch_class, spellnum, metamagic, is_domain);
    
    /* Check two conditions for spontaneous casting:
     * 1. Character must know the spell (permanently learned)
     * 2. Must have available slots of the required circle
     */
    if (is_a_known_spell(ch, ch_class, spellnum) &&
        (compute_slots_by_circle(ch, ch_class, circle) -
             count_total_slots(ch, ch_class, circle) >
         0))
    {
      /* Success! Put a slot of this circle into recovery queue */
      prep_time = compute_spells_prep_time(ch, ch_class, circle, is_domain);
      innate_magic_add(ch, ch_class, circle, metamagic, prep_time, is_domain);

      return ch_class;
    }
  }

  /* No prepared spell found and can't cast spontaneously */
  return CLASS_UNDEFINED;
}

/**
 * spell_prep_gen_check - Check if character can cast a spell
 * @ch: Character attempting to cast
 * @spellnum: Spell to check
 * @metamagic: Metamagic being applied
 * 
 * This function validates whether a character has a spell available to cast.
 * Called by the casting system BEFORE attempting to cast.
 * 
 * CHECKS IN ORDER:
 * 1. Staff members can cast anything (for testing/events)
 * 2. Prepared casters: Is spell in collection (ready to cast)?
 * 3. Spontaneous casters: Do they know it AND have slots?
 * 
 * PREPARED CASTERS:
 * - Must have exact spell+metamagic combo prepared
 * - Quickened fireball is different from regular fireball
 * 
 * SPONTANEOUS CASTERS:
 * - Must know the spell (permanent knowledge)
 * - Must have available slot of appropriate circle
 * - Metamagic increases circle requirement
 * 
 * Returns: Class that can cast it, or CLASS_UNDEFINED if unavailable
 */
int spell_prep_gen_check(struct char_data *ch, int spellnum, int metamagic)
{

  /* Staff override - immortals can cast anything for testing */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return true;

  int class = CLASS_UNDEFINED;

  /* FIRST: Check all prepared spell collections */
  for (class = 0; class < NUM_CLASSES; class ++)
  {
    if (is_spell_in_collection(ch, class, spellnum, metamagic))
      return class;
  }

  /* SECOND: Check spontaneous casting ability */
  int circle_of_this_spell = TOP_CIRCLE + 1;
  for (class = 0; class < NUM_CLASSES; class ++)
  {
    /* Calculate effective circle (including metamagic adjustments) */
    if (CLASS_LEVEL(ch, CLASS_INQUISITOR) && class == CLASS_INQUISITOR)
      circle_of_this_spell = compute_spells_circle(ch, class, spellnum, metamagic, GET_1ST_DOMAIN(ch));
    else
      circle_of_this_spell = compute_spells_circle(ch, class, spellnum, metamagic, DOMAIN_UNDEFINED);
      
    /* Check if they know the spell AND have available slots */
    if (is_a_known_spell(ch, class, spellnum) &&
        (compute_slots_by_circle(ch, class, circle_of_this_spell) - count_total_slots(ch, class, circle_of_this_spell) > 0))
    {
      return class;
    }
  }

  /* Character cannot cast this spell */
  return CLASS_UNDEFINED;
}

/* in: character, class of the queue you want to work with
 * traverse the prep queue and print out the details
 * since the prep queue does not need any organizing, this should be fairly
 * simple */
void print_prep_queue(struct char_data *ch, int ch_class)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
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
    int spell_circle = compute_spells_circle(ch, ch_class,
                                             current->spell,
                                             current->metamagic,
                                             current->domain);
    int prep_time = current->prep_time;
    total_time += prep_time;
    send_to_char(ch, " \tW%20s\tn \tc[\tn%d%s circle\tc]\tn \tc[\tn%2d seconds\tc]\tn %s%s%s%s%s%s %s\r\n",
                 spell_info[current->spell].name,
                 spell_circle,
                 (spell_circle == 1) ? "st" : (spell_circle == 2) ? "nd"
                                          : (spell_circle == 3)   ? "rd"
                                                                  : "th",
                 prep_time,
                 (IS_SET(current->metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_EMPOWER) ? "\tc[\tnempowered\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_EXTEND) ? "\tc[\tnextendeded\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_SILENT) ? "\tc[\tnsilent\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_STILL) ? "\tc[\tnstill\tc]\tn" : ""),
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
  char buf[MAX_INPUT_LENGTH] = {'\0'};
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
    send_to_char(ch, " \tc[\tWcircle-slot: \tn%d%s\tc]\tn \tc[\tn%2d seconds\tc]\tn %s%s%s%s%s%s %s\r\n",
                 current->circle,
                 (current->circle == 1) ? "st" : (current->circle == 2) ? "nd"
                                             : (current->circle == 3)   ? "rd"
                                                                        : "th",
                 prep_time,
                 (IS_SET(current->metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_EMPOWER) ? "\tc[\tnempowered\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_EXTEND) ? "\tc[\tnextended\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_SILENT) ? "\tc[\tnsilent\tc]\tn" : ""),
                 (IS_SET(current->metamagic, METAMAGIC_STILL) ? "\tc[\tnstill\tc]\tn" : ""),
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
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int line_length = 80, high_circle = get_class_highest_circle(ch, ch_class);
  int counter = 0, this_circle = 0;
  
  /* Spell counting structure for stacking */
  struct spell_count_data {
    int spell;
    int metamagic;
    int domain;
    int count;
    struct spell_count_data *next;
  };
  struct spell_count_data *spell_counts[NUM_CIRCLES];
  int i;

  /* Initialize spell count arrays */
  for (i = 0; i < NUM_CIRCLES; i++) {
    spell_counts[i] = NULL;
  }

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

  /* First pass: count all spells by circle and identity */
  struct prep_collection_spell_data *current;
  for (current = SPELL_COLLECTION(ch, ch_class); current; current = current->next)
  {
    this_circle = compute_spells_circle(ch, ch_class, current->spell, 
                                      current->metamagic, current->domain);
    if (this_circle >= 0 && this_circle < NUM_CIRCLES)
    {
      /* Look for existing entry with same spell/metamagic/domain */
      struct spell_count_data *count_entry = spell_counts[this_circle];
      struct spell_count_data *prev = NULL;
      int found = 0;
      
      while (count_entry)
      {
        if (count_entry->spell == current->spell &&
            count_entry->metamagic == current->metamagic &&
            count_entry->domain == current->domain)
        {
          count_entry->count++;
          found = 1;
          break;
        }
        prev = count_entry;
        count_entry = count_entry->next;
      }
      
      if (!found)
      {
        /* Create new count entry */
        struct spell_count_data *new_count;
        CREATE(new_count, struct spell_count_data, 1);
        new_count->spell = current->spell;
        new_count->metamagic = current->metamagic;
        new_count->domain = current->domain;
        new_count->count = 1;
        new_count->next = spell_counts[this_circle];
        spell_counts[this_circle] = new_count;
      }
    }
  }

  /* Second pass: display counted spells by circle */
  for (high_circle; high_circle >= 0; high_circle--)
  {
    counter = 0;
    struct spell_count_data *count_entry = spell_counts[high_circle];
    
    while (count_entry)
    {
      counter++;
      /* Build the metamagic string */
      char metamagic_str[256] = "";
      if (IS_SET(count_entry->metamagic, METAMAGIC_QUICKEN))
        strcat(metamagic_str, " [quickened]");
      if (IS_SET(count_entry->metamagic, METAMAGIC_EMPOWER))
        strcat(metamagic_str, " [empowered]");
      if (IS_SET(count_entry->metamagic, METAMAGIC_MAXIMIZE))
        strcat(metamagic_str, " [maximized]");
      if (IS_SET(count_entry->metamagic, METAMAGIC_EXTEND))
        strcat(metamagic_str, " [extended]");
      if (IS_SET(count_entry->metamagic, METAMAGIC_SILENT))
        strcat(metamagic_str, " [silent]");
      if (IS_SET(count_entry->metamagic, METAMAGIC_STILL))
        strcat(metamagic_str, " [still]");
      if (count_entry->domain)
        sprintf(metamagic_str + strlen(metamagic_str), " [%s]", domain_list[count_entry->domain].name);
      
      /* Add count if more than 1 */
      if (count_entry->count > 1)
        sprintf(metamagic_str + strlen(metamagic_str), " \tYx%d\tn", count_entry->count);
      
      if (counter == 1)
      {
        send_to_char(ch, "\tY%d%s:\tn    \tW%s\tn%s\r\n",
                     high_circle,
                     (high_circle == 1) ? "st" : (high_circle == 2) ? "nd"
                                             : (high_circle == 3)   ? "rd"
                                                                    : "th",
                     spell_info[count_entry->spell].name,
                     metamagic_str);
      }
      else
      {
        send_to_char(ch, "%8s\tW%s\tn%s\r\n",
                     "    ",
                     spell_info[count_entry->spell].name,
                     metamagic_str);
      }
      
      count_entry = count_entry->next;
    }
  }

  /* Clean up allocated memory */
  for (i = 0; i < NUM_CIRCLES; i++)
  {
    struct spell_count_data *count_entry = spell_counts[i];
    struct spell_count_data *next_count;
    
    while (count_entry)
    {
      next_count = count_entry->next;
      free(count_entry);
      count_entry = next_count;
    }
  }
}

/* display avaialble slots based on what is in the queue/collection, and other
   variables */
void display_available_slots(struct char_data *ch, int class)
{
  int slot, num_circles = 0, slot_array[NUM_CIRCLES], last_slot = 0,
            highest_circle = get_class_highest_circle(ch, class),
            line_length = 80;
  bool printed = FALSE, found_slot = FALSE;
  char buf[MAX_INPUT_LENGTH] = {'\0'};

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
                   (slot) == 1 ? "st" : (slot) == 2 ? "nd"
                                    : (slot) == 3   ? "rd"
                                                    : "th");
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
  case CLASS_INQUISITOR:
  case CLASS_SUMMONER:
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
  char buf[MAX_INPUT_LENGTH] = {'\0'};

  if (ready_to_prep_spells(ch, class))
  {
    if (class == CLASS_SORCERER && APOTHEOSIS_SLOTS(ch) > 0)
    {
      // If they are an arcane sorcerer with stored apotheosis charges, they dissipate.
      APOTHEOSIS_SLOTS(ch) = 0;
      send_to_char(ch, "You feel your focused arcane energy fade away.\r\n");
    }
    if (!PRF_FLAGGED(ch, PRF_AUTO_PREP))
    {
      send_to_char(ch, "You continue your %s.\r\n", spell_prep_dict[class][3]);
      *buf = '\0';
      snprintf(buf, sizeof(buf), "$n continues $s %s.", spell_prep_dict[class][3]);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
    }
    start_prep_event(ch, class);
  }
}

/**
 * compute_spells_prep_time - Calculate how long a spell takes to prepare
 * @ch: Character preparing the spell
 * @class: Class being used to prepare
 * @circle: Spell circle (1-9)
 * @domain: TRUE if this is a domain spell
 * 
 * This function determines preparation time in seconds based on multiple factors:
 * 
 * BASE CALCULATION:
 * - Starts with BASE_PREP_TIME (usually 5 seconds)
 * - Adds PREP_TIME_INTERVALS (usually 5) * (circle - 1)
 * - So 1st circle = 5 sec, 2nd = 10 sec, 3rd = 15 sec, etc.
 * 
 * CLASS MODIFIERS:
 * - Each class has a multiplier factor (e.g., wizards faster than clerics)
 * - Rangers/Paladins have penalties due to being half-casters
 * 
 * BONUS REDUCTIONS:
 * - Character level (level/2 seconds reduction)
 * - Ability score bonus (WIS for divine, INT for arcane, etc.)
 * - Concentration skill (skill ranks / 4)
 * - Feats like Faster Memorization (-25%) or Wizard Memorization (-16.7%)
 * - Resting in a regeneration room (-25%)
 * 
 * CONFIG MODIFIERS:
 * - Server-wide modifiers for arcane/divine/alchemy prep times
 * 
 * Returns: Preparation time in seconds (minimum = circle level)
 */
int compute_spells_prep_time(struct char_data *ch, int class, int circle, int domain)
{
  float prep_time = 0.0;
  int bonus_time = 0;
  int stat_bonus = 0;
  int level_bonus = 0;

  /* Base prep time: 5 seconds + 5 seconds per circle above 1st */
  prep_time = BASE_PREP_TIME + (PREP_TIME_INTERVALS * (circle - 1));

  /* Domain spells take slightly longer (1 extra second) - shows divine focus */
  if (domain)
    prep_time++;

  /* Calculate level bonus - higher level characters prepare faster */
  level_bonus = CLASS_LEVEL(ch, class) / 2;
  
  /* Apply class-specific multipliers and determine relevant ability bonus
   * Each class has different preparation speeds based on their magical tradition
   */
  switch (class)
  {
  case CLASS_RANGER:
    prep_time *= RANGER_PREP_TIME_FACTOR;  /* Rangers are slower (half-casters) */
    stat_bonus = GET_WIS_BONUS(ch);        /* Wisdom-based divine magic */
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
  case CLASS_SUMMONER:
    prep_time *= SUMMONER_PREP_TIME_FACTOR;
    stat_bonus = GET_CHA_BONUS(ch);
    break;
  case CLASS_INQUISITOR:
    prep_time *= INQUISITOR_PREP_TIME_FACTOR;
    stat_bonus = GET_WIS_BONUS(ch);
    break;
  }

  /** Calculate time reductions from various sources **/
  
  /* Level bonus - experienced casters prepare faster */
  bonus_time += level_bonus;
  
  /* Ability score bonus - mental acuity speeds preparation
   * Formula: 2/3 of ability bonus (so +6 INT = 4 seconds faster)
   */
  bonus_time += stat_bonus / 3 * 2;
  
  /* Concentration skill - focused mind speeds memorization
   * Each 4 ranks = 1 second reduction
   */
  if (!IS_NPC(ch) && GET_ABILITY(ch, ABILITY_CONCENTRATION))
  {
    bonus_time += compute_ability(ch, ABILITY_CONCENTRATION) / 4;
  }
  
  /* FEAT: Wizard Memorization - reduces prep time by 1/6 (16.7%)
   * This is a class-specific feat for specialist wizards
   */
  if (HAS_FEAT(ch, FEAT_WIZ_MEMORIZATION))
  {
    bonus_time += prep_time / 6;
  }
  
  /* FEAT: Faster Memorization - reduces prep time by 1/4 (25%)
   * This is a general feat available to all casters
   */
  if (HAS_FEAT(ch, FEAT_FASTER_MEMORIZATION))
  {
    bonus_time += prep_time / 4;
  }
  
  /* Regeneration rooms (usually taverns) provide a calm environment
   * Reduces prep time by 25% - encourages social gathering spots
   */
  if (IN_ROOM(ch) != NOWHERE && ROOM_FLAGGED(ch->in_room, ROOM_REGEN))
    bonus_time += prep_time / 4;
    
  /* Note: Song of Focused Mind is handled elsewhere in the event system
   * It would halve prep time but needs to be checked each pulse
   */
  
  /** End bonus calculations **/

  /* Subtract all bonuses from base prep time */
  prep_time -= bonus_time;

  /* Apply server-wide configuration modifiers by magic type
   * These allow server admins to globally adjust preparation speeds
   * CONFIG values are percentages (e.g., 50 = 50% of normal time)
   */
  switch (class)
  {
  case CLASS_WIZARD:
  case CLASS_SORCERER:
  case CLASS_BARD:
  case CLASS_SUMMONER:
    /* Arcane casters - affected by CONFIG_ARCANE_PREP_TIME */
    prep_time = prep_time * CONFIG_ARCANE_PREP_TIME / 100;
    break;

  case CLASS_CLERIC:
  case CLASS_DRUID:
  case CLASS_PALADIN:
  case CLASS_RANGER:
  case CLASS_BLACKGUARD:
  case CLASS_INQUISITOR:
    /* Divine casters - affected by CONFIG_DIVINE_PREP_TIME */
    prep_time = prep_time * CONFIG_DIVINE_PREP_TIME / 100;
    break;

  case CLASS_PSIONICIST:
    /* Psionicists don't prepare - they use PSP that regenerates naturally */
    break;

  case CLASS_ALCHEMIST:
    /* Alchemists - affected by CONFIG_ALCHEMY_PREP_TIME */
    prep_time = prep_time * CONFIG_ALCHEMY_PREP_TIME / 100;
    break;
  }

  /* Ensure minimum 1 second prep time */
  if (prep_time <= 0)
    prep_time = 1;

  /* Final cap: preparation time cannot be less than the spell's circle
   * This ensures higher level spells always take meaningful time
   */
  return (MAX(circle, prep_time));
}

/* look at top of the queue, and reset preparation time of that entry */
void reset_preparation_time(struct char_data *ch, int class)
{
  int preparation_time = 0;

  switch (class)
  {
  case CLASS_SORCERER:
  case CLASS_BARD:
  case CLASS_INQUISITOR:
  case CLASS_SUMMONER:
    if (!INNATE_MAGIC(ch, class))
      return;
    preparation_time = compute_spells_prep_time(ch, class, INNATE_MAGIC(ch, class)->circle, INNATE_MAGIC(ch, class)->domain);
    INNATE_MAGIC(ch, class)->prep_time = preparation_time;
    break;
  default:
    if (!SPELL_PREP_QUEUE(ch, class))
      return;
    preparation_time = compute_spells_prep_time(ch, class, compute_spells_circle(ch, class, SPELL_PREP_QUEUE(ch, class)->spell,
                                  SPELL_PREP_QUEUE(ch, class)->metamagic, SPELL_PREP_QUEUE(ch, class)->domain), SPELL_PREP_QUEUE(ch, class)->domain);
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

#define LOOP_MAX 1000
#define PROC_NUM 5
/* this is a custom function we wrote that will, on firing, randomly restore spells from the queue -zusuk */
// if num_times is 0, there is no limit to how many spells can be restored. Otherwise it will limit it to
// the num_times spell slots
int star_circlet_proc(struct char_data *ch, int num_times)
{
  int class = 0, proc_count = 0, loop_count = 0, proc_max = 0;
  int which_class = 0, num_classes = 0, cur_class = 0;

  if (num_times == 0)
    proc_max = rand_number(1, PROC_NUM);
  else
    proc_max = 1;

  if (!ch)
    return 0;

  /* arbitrary do loop, while controls how much it will run */
  do
  {

    for (class = CLASS_WIZARD; class < NUM_CLASSES; class++)
    {
      if (CLASS_LEVEL(ch, class) > 0)
        num_classes++;
    }

    if (class < NUM_CLASSES)
      which_class = dice(1, num_classes);

    num_classes = 0;

    for (class = CLASS_WIZARD; class < NUM_CLASSES; class++)
    {
      if (CLASS_LEVEL(ch, class) > 0)
      {
        num_classes++;
        if (num_classes == which_class)
        {
          which_class = class;
          break;
        }
      }
    }

    if (which_class < 0 || which_class >= NUM_CLASSES)
      which_class = -1;

    /* class loop */
    for (cur_class = 0; cur_class < NUM_CLASSES; cur_class++)
    {
      if (which_class == -1)
        class = cur_class;
      else
        class = which_class;

      switch (class)
      {
      case CLASS_BARD:
      case CLASS_SORCERER:
      case CLASS_INQUISITOR:
      case CLASS_SUMMONER:
        if (INNATE_MAGIC(ch, class))
        {
          send_to_char(ch, "You finish %s for %s%s%s%s%s%s%d circle slot.\r\n",
                       spell_prep_dict[class][1],
                       (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_QUICKEN) ? "quickened " : ""),
                       (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_EMPOWER) ? "empowered " : ""),
                       (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_MAXIMIZE) ? "maximized " : ""),
                       (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_EXTEND) ? "extended " : ""),
                       (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_SILENT) ? "silent " : ""),
                       (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_STILL) ? "still " : ""),
                       INNATE_MAGIC(ch, class)->circle);
          innate_magic_remove_by_class(ch, class, INNATE_MAGIC(ch, class)->circle, INNATE_MAGIC(ch, class)->metamagic);
          proc_count++;
        }
        break;
      default:
        if (SPELL_PREP_QUEUE(ch, class))
        {
          send_to_char(ch, "You finish %s for %s%s%s%s%s%s%s.\r\n",
                       spell_prep_dict[class][1],
                       (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_QUICKEN) ? "quickened " : ""),
                       (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_EMPOWER) ? "empowered " : ""),
                       (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_MAXIMIZE) ? "maximized " : ""),
                       (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_EXTEND) ? "extended " : ""),
                       (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_SILENT) ? "silent " : ""),
                       (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_STILL) ? "still " : ""),
                       spell_info[SPELL_PREP_QUEUE(ch, class)->spell].name);
          collection_add(ch, class, SPELL_PREP_QUEUE(ch, class)->spell,
                         SPELL_PREP_QUEUE(ch, class)->metamagic,
                         SPELL_PREP_QUEUE(ch, class)->prep_time,
                         SPELL_PREP_QUEUE(ch, class)->domain);
          prep_queue_remove_by_class(ch, class, SPELL_PREP_QUEUE(ch, class)->spell,
                                     SPELL_PREP_QUEUE(ch, class)->metamagic);
          proc_count++;
        }
        break;
      }
    }

    loop_count++;
  } while (loop_count < LOOP_MAX && proc_count < proc_max);

  if (proc_count >= 1)
    return 1;
  else
    return 0;
}
#undef LOOP_MAX
#undef PROC_NUM

/* END helper functions */

/* START event-related */

/**
 * event_preparation - Main event handler for spell preparation
 * @event_obj: The mud event data structure
 * 
 * This is the heart of the spell preparation system. It runs every second
 * while a character is preparing spells and handles:
 * 
 * 1. VALIDATION: Checks if character can still prepare (position, status)
 * 2. COUNTDOWN: Decrements preparation time for the current spell/slot
 * 3. COMPLETION: When prep_time reaches 0:
 *    - For prepared casters: Moves spell from queue to collection
 *    - For spontaneous casters: Removes slot from innate magic queue
 * 4. CONTINUATION: If more spells/slots in queue, continues preparation
 * 5. TERMINATION: When queue is empty, ends preparation state
 * 
 * The event carries the class as a string variable (sVariables) to know
 * which class's queue to process. This allows multiclass characters to
 * prepare spells for different classes separately.
 * 
 * RETURN VALUES:
 * - 0: Event terminates (preparation complete or interrupted)
 * - PASSES_PER_SEC: Event continues next second
 * - PASSES_PER_SEC/2: Event continues in 0.5 seconds (with Song of Focused Mind)
 */
EVENTFUNC(event_preparation)
{
  int class = 0;
  struct char_data *ch = NULL;
  struct mud_event_data *prepare_event = NULL;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  /* Initialize and validate event data */
  *buf = '\0';
  if (event_obj == NULL)
    return 0;
  prepare_event = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)prepare_event->pStruct;
  if (!ch)
    return 0;
  /* Extract the class from event variables (stored as string) */
  class = atoi((char *)prepare_event->sVariables);

  /* Verify the character has something to prepare
   * Spontaneous casters use INNATE_MAGIC queue for spell slots
   * Prepared casters use SPELL_PREP_QUEUE for specific spells
   */
  switch (class)
  {
  case CLASS_SORCERER:
  case CLASS_BARD:
  case CLASS_INQUISITOR:
  case CLASS_SUMMONER:
    /* Spontaneous casters - check for recovering spell slots */
    if (!INNATE_MAGIC(ch, class))
    {
      send_to_char(ch, "Your preparations are aborted!  You do not seem to have anything in your queue!\r\n");
      return 0;
    }
    break;
  default:
    /* Prepared casters - check for spells being memorized */
    if (!SPELL_PREP_QUEUE(ch, class))
    {
      send_to_char(ch, "Your preparations are aborted!  You do not seem to have anything in your queue!\r\n");
      return 0;
    }
    break;
  }

  /* Validate character can continue preparing
   * ready_to_prep_spells() checks: position, fighting, debuffs, spellbook
   * is_preparing() checks: preparation state flag
   * If either fails, preparation is interrupted
   */
  if (!ready_to_prep_spells(ch, class) ||
      !is_preparing(ch))
  {
    send_to_char(ch, "You are not able to finish your spell preparations!\r\n");
    stop_prep_event(ch, class);
    return 0;
  }

  /* Main preparation processing - different for spontaneous vs prepared casters */
  switch (class)
  {
  case CLASS_BARD:
  case CLASS_SORCERER:
  case CLASS_INQUISITOR:
  case CLASS_SUMMONER:
    /* SPONTANEOUS CASTERS: Process spell slot recovery
     * Each second, decrement the prep_time of the first slot in queue
     */
    INNATE_MAGIC(ch, class)->prep_time--;
    
    /* Check if this spell slot has finished recovering */
    if ((INNATE_MAGIC(ch, class)->prep_time) <= 0)
    {
      /* Slot is ready! Announce completion with any metamagic info */
      send_to_char(ch, "You finish %s for %s%s%s%s%s%s%d circle slot.\r\n",
                   spell_prep_dict[class][1],  /* "meditation", "composition", etc. */
                   (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_QUICKEN) ? "quickened " : ""),
                   (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_EMPOWER) ? "empowered " : ""),
                   (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_MAXIMIZE) ? "maximized " : ""),
                   (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_EXTEND) ? "extended " : ""),
                   (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_SILENT) ? "silent " : ""),
                   (IS_SET(INNATE_MAGIC(ch, class)->metamagic, METAMAGIC_STILL) ? "still " : ""),
                   INNATE_MAGIC(ch, class)->circle);
                   
      /* Remove the completed slot from recovery queue
       * The slot is now available for casting any known spell of that circle
       */
      innate_magic_remove_by_class(ch, class, INNATE_MAGIC(ch, class)->circle,
                                   INNATE_MAGIC(ch, class)->metamagic);
                                   
      /* Check if more slots need recovery */
      if (INNATE_MAGIC(ch, class))
      {
        /* Reset timer for next slot and continue */
        reset_preparation_time(ch, class);

        /* Song of Focused Mind doubles preparation speed */
        if (affected_by_spell(ch, SKILL_SONG_OF_FOCUSED_MIND))
        {
          return ((1 * PASSES_PER_SEC) / 2);  /* 0.5 second pulses */
        }

        return (1 * PASSES_PER_SEC);  /* Normal 1 second pulses */
      }
    }
    break;
  default:
    /* PREPARED CASTERS: Process specific spell memorization
     * Each second, decrement the prep_time of the first spell in queue
     */
    SPELL_PREP_QUEUE(ch, class)->prep_time--;
    
    /* Check if this spell has finished preparing */
    if ((SPELL_PREP_QUEUE(ch, class)->prep_time) <= 0)
    {
      /* Spell is ready! Announce completion with spell name and metamagic */
      send_to_char(ch, "You finish %s for %s%s%s%s%s%s%s.\r\n",
                   spell_prep_dict[class][1],  /* "memorizing", "praying", etc. */
                   (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_QUICKEN) ? "quickened " : ""),
                   (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_EMPOWER) ? "empowered " : ""),
                   (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_MAXIMIZE) ? "maximized " : ""),
                   (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_EXTEND) ? "extended " : ""),
                   (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_SILENT) ? "silent " : ""),
                   (IS_SET(SPELL_PREP_QUEUE(ch, class)->metamagic, METAMAGIC_STILL) ? "still " : ""),
                   spell_info[SPELL_PREP_QUEUE(ch, class)->spell].name);
                   
      /* Move the prepared spell from queue to collection
       * Collection holds fully prepared spells ready to cast
       */
      collection_add(ch, class, SPELL_PREP_QUEUE(ch, class)->spell,
                     SPELL_PREP_QUEUE(ch, class)->metamagic,
                     SPELL_PREP_QUEUE(ch, class)->prep_time,
                     SPELL_PREP_QUEUE(ch, class)->domain);
                     
      /* Remove the spell from preparation queue */
      prep_queue_remove_by_class(ch, class, SPELL_PREP_QUEUE(ch, class)->spell,
                                 SPELL_PREP_QUEUE(ch, class)->metamagic);
                                 
      /* Check if more spells need preparation */
      if (SPELL_PREP_QUEUE(ch, class))
      {
        /* Reset timer for next spell and continue */
        reset_preparation_time(ch, class);

        /* Song of Focused Mind doubles preparation speed */
        if (affected_by_spell(ch, SKILL_SONG_OF_FOCUSED_MIND))
        {
          return ((1 * PASSES_PER_SEC) / 2);  /* 0.5 second pulses */
        }

        return (1 * PASSES_PER_SEC);  /* Normal 1 second pulses */
      }
    }
    break;
  }

  /* Check if all preparation is complete
   * If the queue is empty, end the preparation state
   */
  switch (class)
  {
  case CLASS_BARD:
  case CLASS_SORCERER:
  case CLASS_INQUISITOR:
  case CLASS_SUMMONER:
    /* Spontaneous casters - check if all slots are recovered */
    if (!INNATE_MAGIC(ch, class))
    {
      *buf = '\0';
      send_to_char(ch, "Your %s are complete.\r\n", spell_prep_dict[class][3]);
      snprintf(buf, sizeof(buf), "$n completes $s %s.", spell_prep_dict[class][3]);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      set_preparing_state(ch, class, FALSE);
      return 0;  /* Event terminates - all slots recovered */
    }
    break;
  default:
    /* Prepared casters - check if all spells are memorized */
    if (!SPELL_PREP_QUEUE(ch, class))
    {
      *buf = '\0';
      send_to_char(ch, "Your %s are complete.\r\n", spell_prep_dict[class][3]);
      snprintf(buf, sizeof(buf), "$n completes $s %s.", spell_prep_dict[class][3]);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      set_preparing_state(ch, class, FALSE);
      return 0;  /* Event terminates - all spells prepared */
    }
    break;
  }

  /* Continue preparation - check for speed bonus from bard song */
  if (affected_by_spell(ch, SKILL_SONG_OF_FOCUSED_MIND))
  {
    return ((1 * PASSES_PER_SEC) / 2);  /* Double speed preparation */
  }

  return (1 * PASSES_PER_SEC);  /* Normal speed - continue next second */
}

/* END event-related */

/* START ACMD() */

/* manipulation of your respective lists! command for players to
 *   "un" prepare spells */
ACMDU(do_consign_to_oblivion)
{
  int domain_1st = 0, domain_2nd = 0, class = CLASS_UNDEFINED;
  char *spell_arg, *metamagic_arg, arg[MAX_INPUT_LENGTH] = {'\0'};
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
    else if (is_abbrev(metamagic_arg, "empowered"))
    {
      SET_BIT(metamagic, METAMAGIC_EMPOWER);
    }
    else if (is_abbrev(metamagic_arg, "extended"))
    {
      SET_BIT(metamagic, METAMAGIC_EXTEND);
    }
    else if (is_abbrev(metamagic_arg, "silent"))
    {
      SET_BIT(metamagic, METAMAGIC_SILENT);
    }
    else if (is_abbrev(metamagic_arg, "still"))
    {
      SET_BIT(metamagic, METAMAGIC_STILL);
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
      send_to_char(ch, "You %s \tW%s\tn %s%s%s%s%s%s from your %s preparation queue!\r\n",
                   spell_consign_dict[class][0],
                   spell_name(spellnum),
                   (IS_SET(metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_EMPOWER) ? "\tc[\tnempowered\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_EXTEND) ? "\tc[\tnextended\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_SILENT) ? "\tc[\tnsilent\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_STILL) ? "\tc[\tnstill\tc]\tn" : ""),
                   class == CLASS_ALCHEMIST ? "extract" : "spell");
      return;
    }
  }

  /* check spell-collection for spell, if found, remove and exit */
  if (SPELL_COLLECTION(ch, class))
  {
    if (collection_remove_by_class(ch, class, spellnum, metamagic))
    {
      send_to_char(ch, "You %s \tW%s\tn %s%s%s%s%s%s from your %s collection!\r\n",
                   spell_consign_dict[class][0],
                   spell_name(spellnum),
                   (IS_SET(metamagic, METAMAGIC_QUICKEN) ? "\tc[\tnquickened\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_EMPOWER) ? "\tc[\tnempowered\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_MAXIMIZE) ? "\tc[\tnmaximized\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_EXTEND) ? "\tc[\tnextended\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_SILENT) ? "\tc[\tnsilent\tc]\tn" : ""),
                   (IS_SET(metamagic, METAMAGIC_STILL) ? "\tc[\tnstill\tc]\tn" : ""),
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
  case SCMD_COMPEL:
    class = CLASS_INQUISITOR;
    domain_1st = GET_1ST_DOMAIN(ch);
    break;
  case SCMD_CONCOCT:
    class = CLASS_ALCHEMIST;
    break;
  case SCMD_CONJURE:
    class = CLASS_SUMMONER;
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
  case CLASS_INQUISITOR:
  case CLASS_SUMMONER:
    print_prep_collection_data(ch, class);
    begin_preparing(ch, class);
    return; /* innate-magic is finished in this command */
  default:
    if (!*argument || !strcmp(argument, "autoprep"))
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
    else if (is_abbrev(metamagic_arg, "empowered"))
    {
      if (!can_spell_be_empowered(spellnum))
      {
        send_to_char(ch, "This spell cannot be empowered.\r\n");
      }
      if (HAS_FEAT(ch, FEAT_EMPOWER_SPELL))
      {
        SET_BIT(metamagic, METAMAGIC_EMPOWER);
      }
      else
      {
        send_to_char(ch, "You don't know how to empower your magic!\r\n");
        return;
      }
    }
    else if (is_abbrev(metamagic_arg, "extended"))
    {
      if (!can_spell_be_extended(spellnum))
      {
        send_to_char(ch, "This spell cannot be extended.\r\n");
      }
      if (HAS_FEAT(ch, FEAT_EXTEND_SPELL))
      {
        SET_BIT(metamagic, METAMAGIC_EXTEND);
      }
      else
      {
        send_to_char(ch, "You don't know how to extend your magic!\r\n");
        return;
      }
    }
    else if (is_abbrev(metamagic_arg, "silent"))
    {
      if (HAS_FEAT(ch, FEAT_SILENT_SPELL))
      {
        SET_BIT(metamagic, METAMAGIC_SILENT);
      }
      else
      {
        send_to_char(ch, "You don't know how to perform your magic silently.\r\n");
        return;
      }
    }
    else if (is_abbrev(metamagic_arg, "still"))
    {
      if (HAS_FEAT(ch, FEAT_STILL_SPELL))
      {
        SET_BIT(metamagic, METAMAGIC_STILL);
      }
      else
      {
        send_to_char(ch, "You don't know how to prform your magic while physically bound!\r\n");
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
      MIN(compute_spells_circle(ch, class, spellnum, metamagic, domain_1st),
          compute_spells_circle(ch, class, spellnum, metamagic, domain_2nd));

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
  send_to_char(ch, "You start to %s for %s%s%s%s%s%s%s.\r\n",
               spell_prep_dict[class][0],
               (IS_SET(metamagic, METAMAGIC_QUICKEN) ? "quickened " : ""),
               (IS_SET(metamagic, METAMAGIC_EMPOWER) ? "empowered " : ""),
               (IS_SET(metamagic, METAMAGIC_MAXIMIZE) ? "maximized " : ""),
               (IS_SET(metamagic, METAMAGIC_EXTEND) ? "extended " : ""),
               (IS_SET(metamagic, METAMAGIC_SILENT) ? "silent " : ""),
               (IS_SET(metamagic, METAMAGIC_STILL) ? "still " : ""),
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

int class_to_spell_prep_scmd(int class_name)
{
  switch (class_name)
  {
    case CLASS_WIZARD: return SCMD_MEMORIZE;
    case CLASS_CLERIC: return SCMD_PRAY;
    case CLASS_DRUID: return SCMD_COMMUNE;
    case CLASS_SORCERER: return SCMD_MEDITATE;
    case CLASS_PALADIN: return SCMD_CHANT;
    case CLASS_RANGER: return SCMD_ADJURE;
    case CLASS_BARD: return SCMD_COMPOSE;
    case CLASS_ALCHEMIST: return SCMD_CONCOCT;
    case CLASS_BLACKGUARD: return SCMD_CONDEMN;
    case CLASS_INQUISITOR: return SCMD_COMPEL;
    case CLASS_SUMMONER: return SCMD_CONJURE;
  }
  return 0;
}

#undef DEBUGMODE

/* EOF */
