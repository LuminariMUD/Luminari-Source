/**************************************************************************
 *  File: lists.c                                      Part of LuminariMUD *
 *  Usage: Handling of in-game lists                                       *
 *                                                                         *
 *  By Vatiken. Copyright 2012 by Joseph Arnusch                           *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_event.h"

/* Global lists */
struct list_data *global_lists = NULL;
struct list_data *group_list = NULL;

struct list_data *create_list(void)
{
  /* Beginner's Note: This creates a new empty list container.
   * A list is like a chain of items where each knows about the next/previous.
   * The list itself just keeps track of the first and last items in the chain.
   * We also track how many items are in the list (iSize) and how many
   * iterators are currently reading the list (iIterators). */
  
  struct list_data *pNewList = NULL;
  static bool first_list = TRUE;

  /* Allocate memory for the list container structure */
  CREATE(pNewList, struct list_data, 1);

  /* Initialize an empty list - no items yet */
  pNewList->pFirstItem = NULL;  /* No first item yet */
  pNewList->pLastItem = NULL;   /* No last item yet */
  pNewList->iIterators = 0;     /* Nobody is iterating through us yet */
  pNewList->iSize = 0;          /* We have no items yet */

  /* Add to global lists, primarily for debugging purposes.
   * The first list created becomes the global_lists itself,
   * all others are added to it for tracking. */
  if (first_list == FALSE)
    add_to_list(pNewList, global_lists);
  else
    first_list = FALSE;

  return (pNewList);
}

struct item_data *create_item(void)
{
  struct item_data *pNewItem = NULL;

  CREATE(pNewItem, struct item_data, 1);

  pNewItem->pNextItem = NULL;
  pNewItem->pPrevItem = NULL;
  pNewItem->pContent = NULL;

  return (pNewItem);
}

void free_list(struct list_data *pList)
{
  /* Beginner's Note: This function completely destroys a list.
   * It removes all item nodes and then frees the list container itself.
   * IMPORTANT: This does NOT free the content that items point to!
   * The caller must free content separately if needed.
   * 
   * Think of it like dismantling a train:
   * - We unhook and scrap each train car (item nodes)
   * - But the cargo in each car is NOT destroyed (content pointers)
   * - Finally we scrap the engine/tracks (the list structure itself) */
  
  struct item_data *pItem, *pNext;

  /* Reset simple_list in case it was iterating this list */
  simple_list(NULL);

  /* Remove all items from the list safely.
   * We cache the next pointer before removing because remove_from_list
   * will free the current item, making pItem->pNextItem invalid. */
  if (pList && pList->iSize) {
    pItem = pList->pFirstItem;
    while (pItem) {
      pNext = pItem->pNextItem;  /* Save next before current is freed */
      remove_from_list(pItem->pContent, pList);
      pItem = pNext;  /* Move to the saved next item */
    }
  }

  /* Remove this list from the global list registry (unless it IS global_lists) */
  if (pList != NULL && pList != global_lists)
    remove_from_list(pList, global_lists);
  
  /* Finally, free the list container structure itself */
  if (pList != NULL)
    free(pList);
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
    log("WARNING: add_to_list() called with NULL list pointer.");
    return;
  }

  /* Create a new item node to hold our content pointer */
  pNewItem = create_item();

  /* Store the pointer to the actual content (we don't copy it, just point to it) */
  pNewItem->pContent = pContent;
  pNewItem->pNextItem = NULL;  /* We're going to be the last item */

  /* If the list is empty, this becomes the first item */
  if (pList->pFirstItem == NULL)
    pList->pFirstItem = pNewItem;

  /* Connect to the previous last item (if there was one) */
  if (pList->pLastItem)
  {
    pLastItem = pList->pLastItem;
    pLastItem->pNextItem = pNewItem;  /* Old last points forward to us */
    pNewItem->pPrevItem = pLastItem;  /* We point back to old last */
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
   * 4. Free the removed car (but NOT the cargo - caller owns that!)
   * 
   * IMPORTANT: This only frees the list node, NOT the content itself!
   * The caller is responsible for freeing pContent if needed. */
  
  struct item_data *pRemovedItem = NULL;

  /* SAFETY: Check for NULL list pointer to prevent crashes.
   * Can't remove from a list that doesn't exist! */
  if (pList == NULL)
  {
    log("WARNING: remove_from_list() called with NULL list pointer.");
    return;
  }

  /* First, find the item node that contains this content */
  if ((pRemovedItem = find_in_list(pContent, pList)) == NULL)
  {
    log("WARNING: Attempting to remove contents that don't exist in list.");
    return;
  }

  /* Update list's first pointer if we're removing the first item */
  if (pRemovedItem == pList->pFirstItem)
    pList->pFirstItem = pRemovedItem->pNextItem;

  /* Update list's last pointer if we're removing the last item */
  if (pRemovedItem == pList->pLastItem)
    pList->pLastItem = pRemovedItem->pPrevItem;

  /* Connect the previous item to the next item (skip over us) */
  if (pRemovedItem->pPrevItem)
    pRemovedItem->pPrevItem->pNextItem = pRemovedItem->pNextItem;

  /* Connect the next item to the previous item (skip over us) */
  if (pRemovedItem->pNextItem)
    pRemovedItem->pNextItem->pPrevItem = pRemovedItem->pPrevItem;

  /* Decrement the list size */
  pList->iSize--;

  /* If list is now empty, ensure pointers are clean */
  if (pList->iSize == 0)
  {
    pList->pFirstItem = NULL;
    pList->pLastItem = NULL;
  }

  /* Free the item node itself (but NOT pContent - caller owns that!) */
  free(pRemovedItem);
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
   * Returns: The content of the first item, or NULL if list is empty */
  
  void *pContent = NULL;

  /* Safety check: Can't iterate a NULL list */
  if (pList == NULL)
  {
    mudlog(NRM, LVL_STAFF, TRUE, "WARNING: Attempting to merge iterator to NULL list.");
    pIterator->pList = NULL;
    pIterator->pItem = NULL;
    return NULL;
  }
  
  /* Safety check: Can't iterate an empty list */
  if (pList->pFirstItem == NULL)
  {
    mudlog(NRM, LVL_STAFF, TRUE, "WARNING: Attempting to merge iterator to empty list.");
    pIterator->pList = NULL;
    pIterator->pItem = NULL;
    return NULL;
  }

  /* Attach the iterator to this list and position at first item */
  pList->iIterators++;           /* Track that someone is iterating us */
  pIterator->pList = pList;      /* Iterator now knows its list */
  pIterator->pItem = pList->pFirstItem;  /* Start at the beginning */

  /* Get the content of the first item */
  pContent = pIterator->pItem ? pIterator->pItem->pContent : NULL;

  return (pContent);
}

void remove_iterator(struct iterator_data *pIterator)
{
  /* Beginner's Note: This function detaches an iterator from its list.
   * An iterator is like a bookmark that remembers where you are in a list.
   * When done iterating, we need to clean up this bookmark properly.
   * If the iterator isn't attached to any list (NULL), we just return silently
   * as this is a normal case (e.g., when merge_iterator fails on empty lists). */
  
  if (pIterator->pList == NULL)
  {
    /* Silent return - this is normal when merge_iterator fails or
     * when remove_iterator is called multiple times. No warning needed. */
    return;
  }

  /* Decrement the count of active iterators on this list */
  pIterator->pList->iIterators--;
  
  /* Clear the iterator's references to make it safe for reuse */
  pIterator->pList = NULL;
  pIterator->pItem = NULL;
}

/** Spits out an item and cycles down the list
 * @return Returns the content of the list
 * */

void *next_in_list(struct iterator_data *pIterator)
{
  /* Beginner's Note: This function moves the iterator to the next item in the list
   * and returns that item's content. Think of it like turning a page in a book -
   * we move to the next page and return what's written on it. */
  
  void *pContent = NULL;
  struct item_data *pTempItem = NULL;

  /* Safety check: Make sure the iterator is attached to a list */
  if (pIterator->pList == NULL)
  {
    /* This shouldn't happen in normal operation but we check anyway */
    mudlog(NRM, LVL_STAFF, TRUE, "WARNING: Attempting to get content from iterator with NULL list.");
    return NULL;
  }

  /* Safety check: Ensure current item pointer is valid before accessing it */
  if (pIterator->pItem == NULL)
  {
    /* We've reached the end of the list or iterator wasn't initialized properly */
    return NULL;  /* Silent return - this is the normal end-of-list condition */
  }

  /* Move to the next item in the list by following the next pointer */
  pTempItem = pIterator->pItem->pNextItem;
  pIterator->pItem = pTempItem;

  /* Get the content from the new current item (NULL if we've reached the end) */
  pContent = pIterator->pItem ? pIterator->pItem->pContent : NULL;

  return (pContent);
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
  
  struct iterator_data Iterator;
  void *pFoundItem = NULL;
  struct item_data *pItem = NULL;
  bool found = FALSE;

  /* Start iterating from the beginning of the list */
  pFoundItem = merge_iterator(&Iterator, pList);

  /* Search through each item until we find our content */
  for (found = FALSE; pFoundItem != NULL; pFoundItem = next_in_list(&Iterator))
  {
    if (pFoundItem == pContent)  /* Found it! */
    {
      found = TRUE;
      break;
    }
  }

  /* If found, grab the actual item node (not just the content) */
  if (found)
    pItem = Iterator.pItem;

  /* Clean up our iterator */
  remove_iterator(&Iterator);

  /* Return the node if found, NULL otherwise */
  if (found)
    return (pItem);
  else
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

void *simple_list(struct list_data *pList)
{
  /* Beginner's Note: This function provides an easy way to iterate through a list
   * WITHOUT creating your own iterator. It uses a STATIC iterator that remembers
   * its position between calls. This makes it simple but NOT THREAD-SAFE and
   * you CANNOT nest calls (can't use simple_list inside another simple_list loop).
   * 
   * How it works:
   * 1. First call with a list starts iteration from the beginning
   * 2. Each subsequent call returns the next item
   * 3. When done, returns NULL and resets itself
   * 4. Call with NULL to manually reset at any time
   */
  
  static struct iterator_data Iterator;  /* Static = remembers state between calls */
  static bool loop = FALSE;               /* Are we currently iterating? */
  static struct list_data *pLastList = NULL;  /* Which list are we iterating? */
  void *pContent = NULL;

  /* Manual reset requested (passing NULL) - clear all state */
  if (pList == NULL)
  {
    /* If we were iterating, properly clean up the iterator */
    if (loop && Iterator.pList != NULL)
      remove_iterator(&Iterator);
    
    loop = FALSE;
    pLastList = NULL;
    return NULL;
  }

  /* Starting a new iteration or switching to a different list */
  if (!loop || pLastList != pList)
  {
    /* Safety check: If we were iterating a different list, warn about it
     * This indicates the caller forgot to reset or is trying to nest loops */
    if (loop && pLastList != pList)
      mudlog(CMP, LVL_GRSTAFF, TRUE, "SYSERR: simple_list() forced to reset itself.");

    /* Clean up any previous iteration state before starting new one */
    if (loop)
    {
      remove_iterator(&Iterator);
      loop = FALSE;
    }

    /* Start iterating the new list from the beginning */
    pContent = merge_iterator(&Iterator, pList);
    if (pContent != NULL)
    {
      pLastList = pList;
      loop = TRUE;
      return (pContent);
    }
    else
    {
      /* List is empty or invalid - no iteration needed */
      return NULL;
    }
  }

  /* Continue with the current iteration - get next item */
  if ((pContent = next_in_list(&Iterator)) != NULL)
    return (pContent);

  /* We've reached the end of the list - clean up and reset */
  remove_iterator(&Iterator);
  loop = FALSE;
  pLastList = NULL;
  return NULL;
}

void *random_from_list(struct list_data *pList)
{
  /* Beginner's Note: This picks a random item from a list.
   * It works by:
   * 1. Generating a random number between 1 and list size
   * 2. Iterating through the list counting items
   * 3. Returning the item when we reach our random count
   * 
   * This is O(n) time complexity but works for any size list.
   * Returns: Random content from list, or NULL if list is empty */
  
  struct iterator_data Iterator;
  void *pFoundItem = NULL;
  bool found = FALSE;
  int number = 0;
  int count = 1;

  /* SAFETY: Check for NULL list pointer to prevent crashes.
   * Can't pick from a list that doesn't exist! */
  if (pList == NULL)
  {
    log("WARNING: random_from_list() called with NULL list pointer.");
    return NULL;
  }

  /* Can't pick from an empty list */
  if (pList->iSize <= 0)
    return NULL;
  else
    number = rand_number(1, pList->iSize);  /* Pick which item we want */

  /* Start iterating from the beginning */
  pFoundItem = merge_iterator(&Iterator, pList);

  /* Count through items until we reach our random number */
  for (found = FALSE; pFoundItem != NULL; pFoundItem = next_in_list(&Iterator), count++)
  {
    if (count == number)  /* This is the randomly chosen item */
    {
      found = TRUE;
      break;
    }
  }

  /* Clean up our iterator */
  remove_iterator(&Iterator);

  /* Return the randomly selected content */
  if (found)
    return (pFoundItem);
  else
    return NULL;  /* Shouldn't happen unless list was modified during iteration */
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
    log("WARNING: randomize_list() called with NULL list pointer.");
    return NULL;
  }

  /* Can't randomize an empty list - return NULL but also free the empty list
   * to avoid memory leak since the caller expects us to consume the list */
  if (pList->iSize == 0)
  {
    free_list(pList);  /* Free the empty list to prevent memory leak */
    return NULL;
  }

  newList = create_list();

  while ((pContent = random_from_list(pList)) != NULL)
  {
    remove_from_list(pContent, pList);
    add_to_list(pContent, newList);
  }

  free_list(pList);

  return (newList);
}
