/**************************************************************************
 *  File: lists.c                                      Part of LuminariMUD *
 *  Usage: Handling of in-game lists                                       *
 *                                                                         *
 *  By Vatiken. Copyright 2012 by Joseph Arnusch                           *
 *  Re-written by LuminariMUD staff to fix the original code.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dgscript/dg_event.h"

/* Global lists */
struct list_data *global_lists = NULL;
struct list_data *group_list = NULL;

/* simple_list() is intentionally a single, non-reentrant convenience cursor. */
static struct iterator_data simple_iterator;
static bool simple_loop = FALSE;
static struct list_data *simple_last_list = NULL;

static struct item_data *create_item(void);
static void free_removed_items(struct list_data *pList);
static void destroy_list(struct list_data *pList);
static void unlink_item(struct item_data *pItem, struct list_data *pList);
static void reset_simple_iterator(void);

struct list_data *create_list(void)
{
  /* Beginner's Note: This creates a new empty list container.
   * A list is like a chain of items where each knows about the next/previous.
   * The list itself just keeps track of the first and last items in the chain.
   * We also track how many items are in the list (iSize) and how many
   * iterators are currently reading the list (iIterators). */

  struct list_data *pNewList = NULL;
  /* Allocate memory for the list container structure */
  CREATE(pNewList, struct list_data, 1);

  /* Initialize an empty list - no items yet */
  pNewList->pFirstItem = NULL; /* No first item yet */
  pNewList->pLastItem = NULL;  /* No last item yet */
  pNewList->pRemovedItems = NULL;
  pNewList->iIterators = 0; /* Nobody is iterating through us yet */
  pNewList->iSize = 0;      /* We have no items yet */
  pNewList->pending_free = FALSE;

  /* Add to global lists, primarily for debugging purposes.
   * boot_db() explicitly creates and assigns the registry; all lists created
   * after that are added to it for tracking.
   *
   * GLOBAL_LISTS EXPLAINED FOR BEGINNERS:
   * The global_lists is a special "list of lists" used for debugging.
   * It keeps track of all lists created in the game so developers can
   * monitor memory usage and find list-related bugs.
   *
   * HOW IT WORKS:
   * 1. boot_db() explicitly assigns its first list to global_lists
   * 2. All lists created after that are added to global_lists
   * 3. Lists created before registry initialization remain unregistered
   *
   * WHY THIS PATTERN?
   * - We need a list to store all lists
   * - But the registry itself must not contain itself
   * - Explicit assignment makes registry lifetime and reinitialization unambiguous
   *
   * NOTE: global_lists is mainly for debugging and is not critical for gameplay */
  if (global_lists != NULL)
    add_to_list(pNewList, global_lists);

  return (pNewList);
}

static struct item_data *create_item(void)
{
  struct item_data *pNewItem = NULL;

  CREATE(pNewItem, struct item_data, 1);

  pNewItem->pNextItem = NULL;
  pNewItem->pPrevItem = NULL;
  pNewItem->pNextRemoved = NULL;
  pNewItem->pContent = NULL;
  pNewItem->is_removed = FALSE;

  return (pNewItem);
}

static void free_removed_items(struct list_data *pList)
{
  struct item_data *pItem;
  struct item_data *pNext;

  if (pList == NULL || pList->iIterators != 0)
    return;

  pItem = pList->pRemovedItems;
  while (pItem != NULL)
  {
    pNext = pItem->pNextRemoved;
    free(pItem);
    pItem = pNext;
  }
  pList->pRemovedItems = NULL;
}

static void destroy_list(struct list_data *pList)
{
  struct item_data *pItem;
  struct item_data *pNext;

  if (pList == NULL || pList->iIterators != 0)
    return;

  pItem = pList->pFirstItem;
  while (pItem != NULL)
  {
    pNext = pItem->pNextItem;
    free(pItem);
    pItem = pNext;
  }

  pList->pFirstItem = NULL;
  pList->pLastItem = NULL;
  pList->iSize = 0;
  free_removed_items(pList);
  free(pList);
}

static void unlink_item(struct item_data *pItem, struct list_data *pList)
{
  if (pItem == NULL || pList == NULL || pItem->is_removed)
    return;

  if (pItem == pList->pFirstItem)
    pList->pFirstItem = pItem->pNextItem;
  if (pItem == pList->pLastItem)
    pList->pLastItem = pItem->pPrevItem;
  if (pItem->pPrevItem != NULL)
    pItem->pPrevItem->pNextItem = pItem->pNextItem;
  if (pItem->pNextItem != NULL)
    pItem->pNextItem->pPrevItem = pItem->pPrevItem;

  if (pList->iSize > 0)
    pList->iSize--;
  if (pList->iSize == 0)
  {
    pList->pFirstItem = NULL;
    pList->pLastItem = NULL;
  }

  pItem->is_removed = TRUE;
  if (pList->iIterators > 0)
  {
    /* Keep the old next link intact so an iterator on this node can advance. */
    pItem->pNextRemoved = pList->pRemovedItems;
    pList->pRemovedItems = pItem;
  }
  else
  {
    free(pItem);
  }
}

void free_list(struct list_data *pList)
{
  struct item_data *pRegistryItem;

  if (pList == NULL)
    return;
  if (pList->pending_free)
    return;

  /* Only reset the convenience cursor when it belongs to this list. */
  if (simple_loop && simple_iterator.pList == pList)
    reset_simple_iterator();

  if (pList == global_lists)
  {
    global_lists = NULL;
  }
  else if (global_lists != NULL && !global_lists->pending_free)
  {
    pRegistryItem = find_in_list(pList, global_lists);
    if (pRegistryItem != NULL)
      unlink_item(pRegistryItem, global_lists);
  }

  if (pList == group_list)
    group_list = NULL;

  /* Active iterators retain node addresses. Delay destruction until they detach. */
  pList->pending_free = TRUE;
  if (pList->iIterators == 0)
    destroy_list(pList);
}

void add_to_list(void *pContent, struct list_data *pList)
{
  /* Beginner's Note: This adds a new item to the END of a list.
   * The list doesn't own the content - it just stores a pointer to it.
   * Think of it like adding a new car to the end of a train:
   * - Create a new car (item_data)
   * - Put the cargo in it (pContent)
   * - Hook it to the last car in the train
   * - Update the train to know this is now the last car */

  struct item_data *pNewItem = NULL;
  struct item_data *pLastItem = NULL;

  /* SAFETY: Check for NULL list pointer to prevent crashes.
   * A NULL list means we have nowhere to add the content. */
  if (pList == NULL)
  {
    /* ERROR HANDLING POLICY: NULL list pointers are programming errors.
     * Log at SYSERR level since this indicates a bug that needs fixing. */
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: add_to_list() called with NULL list pointer.");
    return;
  }

  if (pList->pending_free)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: add_to_list() called for a list pending destruction.");
    return;
  }

  /* NULL is the iterator end sentinel and therefore cannot be list content. */
  if (pContent == NULL)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: add_to_list() called with NULL content.");
    return;
  }

  /* Create a new item node to hold our content pointer */
  pNewItem = create_item();

  /* Store the pointer to the actual content (we don't copy it, just point to it) */
  pNewItem->pContent = pContent;
  pNewItem->pNextItem = NULL; /* We're going to be the last item */

  /* If the list is empty, this becomes the first item */
  if (pList->pFirstItem == NULL)
    pList->pFirstItem = pNewItem;

  /* Connect to the previous last item (if there was one) */
  if (pList->pLastItem)
  {
    pLastItem = pList->pLastItem;
    pLastItem->pNextItem = pNewItem; /* Old last points forward to us */
    pNewItem->pPrevItem = pLastItem; /* We point back to old last */
  }

  /* We are now the last item in the list */
  pList->pLastItem = pNewItem;

  /* Increment the count of items in the list */
  pList->iSize++;
}

void remove_from_list(void *pContent, struct list_data *pList)
{
  /* Beginner's Note: This removes an item from a list by finding the content.
   * It's like removing a specific car from the middle of a train:
   * 1. Find which car has our cargo (find_in_list)
   * 2. Connect the car before it to the car after it
   * 3. Update the train's first/last car if needed
   * 4. Reclaim the removed car when no iterator can still reference it
   *
   * IMPORTANT: This only reclaims the list node, NOT the content itself!
   * The caller is responsible for freeing pContent if needed. */

  struct item_data *pRemovedItem = NULL;

  /* SAFETY: Check for NULL list pointer to prevent crashes.
   * Can't remove from a list that doesn't exist! */
  if (pList == NULL)
  {
    /* ERROR HANDLING POLICY: NULL list pointers are programming errors.
     * Log at SYSERR level since this indicates a bug that needs fixing. */
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: remove_from_list() called with NULL list pointer.");
    return;
  }

  if (pList->pending_free)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE,
           "SYSERR: remove_from_list() called for a list pending destruction.");
    return;
  }

  /* First, find the item node that contains this content */
  if ((pRemovedItem = find_in_list(pContent, pList)) == NULL)
  {
    /* ERROR HANDLING POLICY: Removing non-existent items is a logic error.
     * Log at NRM level as this might happen in normal gameplay scenarios. */
    mudlog(NRM, LVL_STAFF, TRUE,
           "WARNING: Attempting to remove contents that don't exist in list.");
    return;
  }

  /* Active iterators may still point at this node, so unlink_item() defers its free. */
  unlink_item(pRemovedItem, pList);
}

/** Merges an iterator with a list
 * @post Don't forget to remove the iterator with remove_iterator().
 * */

void *merge_iterator(struct iterator_data *pIterator, struct list_data *pList)
{
  /* Beginner's Note: This "attaches" an iterator to a list and positions it
   * at the FIRST item. An iterator is like a bookmark that remembers where
   * you are in a list. After calling this, you can use next_in_list() to
   * move through the list one item at a time.
   *
   * IMPORTANT: Always call remove_iterator() when done to clean up!
   *
   * ITERATOR PATTERN EXPLAINED:
   * Think of an iterator like reading a book:
   * 1. merge_iterator() = Open the book to page 1
   * 2. next_in_list() = Turn to the next page
   * 3. remove_iterator() = Close the book and put it away
   *
   * WHY USE ITERATORS INSTEAD OF simple_list()?
   * - Iterators can be nested (you can read multiple books at once)
   * - Each iterator maintains its own position independently
   * - Safer for complex operations like removing items while iterating
   *
   * TYPICAL USAGE PATTERN:
   *   struct iterator_data it;
   *   struct char_data *ch = merge_iterator(&it, character_list);
   *   while (ch) {
   *     // Process ch here
   *     ch = next_in_list(&it);
   *   }
   *   remove_iterator(&it);  // DON'T FORGET THIS!
   *
   * Returns: The content of the first item, or NULL if list is empty */

  void *pContent = NULL;

  if (pIterator == NULL)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: merge_iterator() called with NULL iterator.");
    return NULL;
  }

  /* Safety check: Can't iterate a NULL list */
  if (pList == NULL)
  {
    /* ERROR HANDLING POLICY: NULL list in iterator is a programming error.
     * Log at SYSERR level since this indicates incorrect API usage. */
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: Attempting to merge iterator to NULL list.");
    pIterator->pList = NULL;
    pIterator->pItem = NULL;
    return NULL;
  }

  if (pList->pending_free)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE,
           "SYSERR: merge_iterator() called for a list pending destruction.");
    pIterator->pList = NULL;
    pIterator->pItem = NULL;
    return NULL;
  }

  /* An empty list is a valid collection with nothing to iterate. */
  if (pList->pFirstItem == NULL)
  {
    pIterator->pList = NULL;
    pIterator->pItem = NULL;
    return NULL;
  }

  pIterator->pItem = pList->pFirstItem;
  while (pIterator->pItem != NULL &&
         (pIterator->pItem->is_removed || pIterator->pItem->pContent == NULL))
    pIterator->pItem = pIterator->pItem->pNextItem;

  if (pIterator->pItem == NULL)
  {
    pIterator->pList = NULL;
    return NULL;
  }

  pList->iIterators++;
  pIterator->pList = pList;
  pContent = pIterator->pItem->pContent;

  return (pContent);
}

void remove_iterator(struct iterator_data *pIterator)
{
  /* Beginner's Note: This function detaches an iterator from its list.
   * An iterator is like a bookmark that remembers where you are in a list.
   * When done iterating, we need to clean up this bookmark properly.
   * If the iterator isn't attached to any list (NULL), we just return silently
   * as this is a normal case (e.g., when merge_iterator fails on empty lists). */

  struct list_data *pList;

  if (pIterator == NULL)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: remove_iterator() called with NULL iterator.");
    return;
  }

  if (pIterator->pList == NULL)
  {
    /* Silent return - this is normal when merge_iterator fails or
     * when remove_iterator is called multiple times. No warning needed. */
    return;
  }

  pList = pIterator->pList;
  pIterator->pList = NULL;
  pIterator->pItem = NULL;

  if (pList->iIterators == 0)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE,
           "SYSERR: remove_iterator() found an invalid zero iterator count.");
    return;
  }

  pList->iIterators--;
  if (pList->iIterators == 0)
  {
    free_removed_items(pList);
    if (pList->pending_free)
      destroy_list(pList);
  }
}

/** Spits out an item and cycles down the list
 * @return Returns the content of the list
 * */

void *next_in_list(struct iterator_data *pIterator)
{
  /* Beginner's Note: This function moves the iterator to the next item in the list
   * and returns that item's content. Think of it like turning a page in a book -
   * we move to the next page and return what's written on it. */

  struct item_data *pTempItem = NULL;

  if (pIterator == NULL)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: next_in_list() called with NULL iterator.");
    return NULL;
  }

  /* Safety check: Make sure the iterator is attached to a list */
  if (pIterator->pList == NULL)
  {
    /* This shouldn't happen in normal operation but we check anyway */
    /* ERROR HANDLING POLICY: Iterator with NULL list is a programming error.
     * Log at SYSERR level since this indicates incorrect API usage. */
    mudlog(CMP, LVL_GRSTAFF, TRUE,
           "SYSERR: Attempting to get content from iterator with NULL list.");
    return NULL;
  }

  if (pIterator->pList->pending_free)
  {
    remove_iterator(pIterator);
    return NULL;
  }

  /* Safety check: Ensure current item pointer is valid before accessing it */
  if (pIterator->pItem == NULL)
  {
    remove_iterator(pIterator);
    return NULL;
  }

  /* Removed nodes retain their old next link until every iterator has detached. */
  pTempItem = pIterator->pItem->pNextItem;
  while (pTempItem != NULL && (pTempItem->is_removed || pTempItem->pContent == NULL))
    pTempItem = pTempItem->pNextItem;
  pIterator->pItem = pTempItem;

  if (pIterator->pItem == NULL)
  {
    remove_iterator(pIterator);
    return NULL;
  }

  return (pIterator->pItem->pContent);
}

/** Searches through the a list and returns the item block that holds pContent
 * @return Returns the actual item block and not the pContent itself, since
 * it is assumed you already have the pContent.
 * */

struct item_data *find_in_list(void *pContent, struct list_data *pList)
{
  /* Beginner's Note: This searches through a list to find a specific content.
   * It returns the LIST NODE (item_data) that contains the content, not the
   * content itself (you already have that!).
   *
   * We need the node to properly remove items from the list, as we need to
   * update the previous/next pointers of surrounding nodes.
   *
   * Returns: The item node containing pContent, or NULL if not found */

  struct item_data *pItem = NULL;

  if (pList == NULL)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: find_in_list() called with NULL list pointer.");
    return NULL;
  }
  if (pContent == NULL || pList->pending_free)
    return NULL;

  for (pItem = pList->pFirstItem; pItem != NULL; pItem = pItem->pNextItem)
  {
    if (!pItem->is_removed && pItem->pContent == pContent)
      return (pItem);
  }

  return NULL;
}

/** This is the "For Dummies" function, as although it's not as flexible,
 * it is even easier applied for list searches then using your own iterators
 * and next_in_list()
 * @usage Common usage would be as follows:
 *
 * while ((var = (struct XXX_data *) simple_list(XXX_list))) {
 *   blah blah....
 * }
 *
 * DO NOT EVER NEST THIS FUNCTION - i.e. use the function in a for loop and then
 * use simple_list within the loop.  it is NOT REENTRANT and contains STATE.
 *
 * @return Will return the next list content until it hits the end, in which
 * will detach itself from the list.
 * */

static void reset_simple_iterator(void)
{
  if (simple_loop && simple_iterator.pList != NULL)
    remove_iterator(&simple_iterator);

  simple_iterator.pList = NULL;
  simple_iterator.pItem = NULL;
  simple_loop = FALSE;
  simple_last_list = NULL;
}

void *simple_list(struct list_data *pList)
{
  void *pContent;

  if (pList == NULL)
  {
    reset_simple_iterator();
    return NULL;
  }

  if (!simple_loop || simple_last_list != pList)
  {
    if (simple_loop && simple_last_list != pList)
      mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: simple_list() forced to reset itself.");

    reset_simple_iterator();
    pContent = merge_iterator(&simple_iterator, pList);
    if (pContent == NULL)
      return NULL;

    simple_loop = TRUE;
    simple_last_list = pList;
    return (pContent);
  }

  pContent = next_in_list(&simple_iterator);
  if (pContent == NULL)
  {
    simple_loop = FALSE;
    simple_last_list = NULL;
  }

  return (pContent);
}

void *random_from_list(struct list_data *pList)
{
  /* Beginner's Note: This picks a random item from a list.
   * It works by:
   * 1. Generating a random zero-based index below the list size
   * 2. Iterating through the list counting items
   * 3. Returning the item when we reach our random count
   *
   * This is O(n) time complexity but works for any size list.
   * Returns: Random content from list, or NULL if list is empty */

  struct item_data *pItem;
  size_t number;

  /* SAFETY: Check for NULL list pointer to prevent crashes.
   * Can't pick from a list that doesn't exist! */
  if (pList == NULL)
  {
    /* ERROR HANDLING POLICY: NULL list pointers are programming errors.
     * Log at SYSERR level since this indicates a bug that needs fixing. */
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: random_from_list() called with NULL list pointer.");
    return NULL;
  }

  if (pList->pending_free)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE,
           "SYSERR: random_from_list() called for a list pending destruction.");
    return NULL;
  }

  /* Can't pick from an empty list */
  if (pList->iSize == 0)
    return NULL;

  number = (size_t)circle_random() % pList->iSize;
  for (pItem = pList->pFirstItem; pItem != NULL && number > 0; pItem = pItem->pNextItem)
    number--;

  if (pItem == NULL || pItem->is_removed || pItem->pContent == NULL)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE,
           "SYSERR: random_from_list() found inconsistent list size or content.");
    return NULL;
  }

  return (pItem->pContent);
}

struct list_data *randomize_list(struct list_data *pList)
{
  /* Beginner's Note: This function creates a new list with all items from the
   * original list but in random order. It's like shuffling a deck of cards:
   * 1. Create a new empty list
   * 2. Pick random items from the old list one by one
   * 3. Move each picked item to the new list
   * 4. Free the old list container (but not the content!)
   * 5. Return the new shuffled list
   *
   * IMPORTANT: The original list is DESTROYED! The content is preserved
   * but moved to the new list. The caller gets ownership of the new list. */

  struct list_data *newList = NULL;
  void *pContent = NULL;

  /* SAFETY: Check for NULL list pointer to prevent crashes.
   * Can't randomize a list that doesn't exist! */
  if (pList == NULL)
  {
    /* ERROR HANDLING POLICY: NULL list pointers are programming errors.
     * Log at SYSERR level since this indicates a bug that needs fixing. */
    mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: randomize_list() called with NULL list pointer.");
    return NULL;
  }

  if (pList->pending_free)
  {
    mudlog(CMP, LVL_GRSTAFF, TRUE,
           "SYSERR: randomize_list() called for a list pending destruction.");
    return NULL;
  }

  newList = create_list();

  while (pList->iSize > 0)
  {
    pContent = random_from_list(pList);
    if (pContent == NULL)
    {
      free_list(newList);
      return NULL;
    }
    remove_from_list(pContent, pList);
    add_to_list(pContent, newList);
  }

  free_list(pList);

  return (newList);
}
