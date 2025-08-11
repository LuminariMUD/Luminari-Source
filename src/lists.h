/**
* @file lists.h                               LuminariMUD
* Lists Header file.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* This source code, which was not part of the CircleMUD legacy code,
* is attributed to:
* Copyright 2012 by Joseph Arnusch.
*/

#ifndef _LISTS_HEADER
#define _LISTS_HEADER

struct item_data
{
  struct item_data *pPrevItem;
  struct item_data *pNextItem;
  void *pContent;
};

struct list_data
{
  struct item_data *pFirstItem;
  struct item_data *pLastItem;
  unsigned short int iIterators;
  unsigned short int iSize;
};

struct iterator_data
{
  struct list_data *pList;
  struct item_data *pItem;
};

/* ERROR HANDLING POLICY FOR LISTS SYSTEM
 * ========================================
 * 
 * Log Levels:
 * - SYSERR (CMP, LVL_GRSTAFF): Programming errors that indicate bugs
 *   - NULL list pointers passed to functions
 *   - Iterator operations on NULL iterators
 *   - API misuse that should never happen in correct code
 * 
 * - WARNING (NRM, LVL_STAFF): Normal but noteworthy conditions
 *   - Attempting to iterate empty lists
 *   - Removing items not in list (might be gameplay logic)
 *   - simple_list() forced resets (indicates missing cleanup)
 * 
 * Return Values:
 * - Functions return NULL on error when they return pointers
 * - Void functions silently return on NULL input after logging
 * - No functions abort/crash - all errors are recoverable
 * 
 * Edge Case Behaviors:
 * - NULL list operations: Log error and return safely
 * - Empty list operations: Return NULL/0 without error
 * - Double-remove: Log warning but continue safely
 * - Iterator on freed list: Reset safely if detected
 * 
 * Best Practices:
 * - Always check return values from create_list()
 * - Call simple_list(NULL) before and after loops
 * - Use explicit iterators for nested operations
 * - Free content separately from lists
 */

/* Externals */
extern struct list_data *global_lists;
extern struct list_data *group_list;

/* Locals */
void add_to_list(void *pContent, struct list_data *pList);
void *random_from_list(struct list_data *pList);
struct list_data *randomize_list(struct list_data *pList);
struct list_data *create_list(void);
void *merge_iterator(struct iterator_data *pIterator, struct list_data *pList);
void remove_iterator(struct iterator_data *pIterator);
void *next_in_list(struct iterator_data *pIterator);
void remove_from_list(void *pContent, struct list_data *pList);
struct item_data *find_in_list(void *pContent, struct list_data *pList);
void *simple_list(struct list_data *pList);
void free_list(struct list_data *pList);
/* clear_simple_list() - DEPRECATED: Use simple_list(NULL) instead */

/* Beginner's Note: Helper macros for safer and more convenient list iteration.
 * These macros provide safer alternatives to manual simple_list() usage.
 * 
 * NOTE: Due to C89/C90 compatibility requirements, the SIMPLE_LIST_FOREACH macro
 * cannot be implemented as originally intended (would require C99 for-loop declarations).
 * Instead, we provide helper macros for common patterns.
 * 
 * SAFE_REMOVE_FROM_LIST: Safe removal with NULL checks
 * - Checks both item and list for NULL before attempting removal
 * - Prevents crashes from NULL pointers
 * 
 * Usage example:
 * SAFE_REMOVE_FROM_LIST(item, my_list);  // Won't crash on NULL
 * 
 * For iteration, continue using the traditional pattern:
 * struct char_data *ch;
 * simple_list(NULL);  // Reset before use
 * while ((ch = (struct char_data *)simple_list(group_list))) {
 *   // Process ch
 * }
 * simple_list(NULL);  // Clean up after use (if needed)
 */

/* Cleanup macro to reset the simple_list iterator
 * Use this before starting iteration and after early exits
 */
#define SIMPLE_LIST_CLEANUP() simple_list(NULL)

/* Safe removal macro with NULL checks
 * Prevents crashes when item or list is NULL
 */
#define SAFE_REMOVE_FROM_LIST(item, list) \
  do { \
    if ((item) != NULL && (list) != NULL) { \
      remove_from_list((item), (list)); \
    } \
  } while(0)

#endif
