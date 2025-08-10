### Lists System (src/lists.h, src/lists.c)

This document describes LuminariMUD's generic list facility, a lightweight, intrusive wrapper around a doubly linked list that stores opaque pointers (`void *`). The system is designed for simple in-game collections (groups, global registries, work queues) with predictable performance and minimal overhead.

The implementation originates from tbaMUD but has Luminari-specific adjustments. It is not thread-safe and is intended for the MUD's single-threaded game loop.

## ⚠️ CRITICAL WARNING: DO NOT NEST simple_list() LOOPS! ⚠️

**The `simple_list()` function uses static state and CANNOT be nested!** Nesting will corrupt the outer loop's iteration state and cause bugs. Always use explicit iterators for nested loops:

```c
// ❌ WRONG - DO NOT DO THIS!
while ((obj1 = simple_list(list1))) {
  while ((obj2 = simple_list(list2))) {  // CORRUPTS OUTER LOOP!
    // ...
  }
}

// ✅ CORRECT - Use explicit iterator for inner loop
struct iterator_data inner_it;
while ((obj1 = simple_list(list1))) {
  obj2 = merge_iterator(&inner_it, list2);
  while (obj2) {
    // ...
    obj2 = next_in_list(&inner_it);
  }
  remove_iterator(&inner_it);
}
```

---

## Overview

- **List node type**: `struct item_data` — links forward/back and carries `pContent`.
- **List container**: `struct list_data` — tracks first/last node, size, and number of attached iterators.
- **Iterators**: `struct iterator_data` — a small cursor (`pList`, `pItem`) for safe traversal via API helpers.
- **Storage model**: The list stores raw pointers; it does not allocate, duplicate, or free `pContent`.
- **Global registries**: `global_lists` (debug aid) and `group_list` are top-level lists provided by the core.

---

## Data Structures

- `struct item_data`
  - `pPrevItem`, `pNextItem`: doubly-linked neighbors
  - `pContent`: opaque pointer to the actual object (caller-owned)

- `struct list_data`
  - `pFirstItem`, `pLastItem`: list ends
  - `iIterators`: count of active iterators attached (diagnostic)
  - `iSize`: number of elements in the list

- `struct iterator_data`
  - `pList`: the list being iterated
  - `pItem`: current node during iteration

Notes:
- `iIterators` is not used to enforce concurrent modification rules. It exists for visibility and future safety checks.
- Equality/lookup is by raw pointer equality of `pContent` (no deep compare).

---

## Public API (from `lists.h`)

- Creation and teardown
  - `struct list_data *create_list(void);`
  - `void free_list(struct list_data *pList);` — frees the nodes and the list container, not the `pContent`.

- Core operations
  - `void add_to_list(void *pContent, struct list_data *pList);` — O(1) append
  - `void remove_from_list(void *pContent, struct list_data *pList);` — O(1) unlink once the node is known; internally finds the node O(n)
  - `struct item_data *find_in_list(void *pContent, struct list_data *pList);` — O(n) node lookup (internal use pattern)

- Iteration helpers
  - `void *merge_iterator(struct iterator_data *pIterator, struct list_data *pList);` — attach, return first `pContent` or NULL
  - `void remove_iterator(struct iterator_data *pIterator);` — detach iterator
  - `void *next_in_list(struct iterator_data *pIterator);` — advance, return next `pContent` or NULL
  - `void *simple_list(struct list_data *pList);` — convenience, non-reentrant cursor with static state (reset with `simple_list(NULL)`)

- Randomization utilities
  - `void *random_from_list(struct list_data *pList);` — uniformly pick one `pContent` or NULL if empty
  - `struct list_data *randomize_list(struct list_data *pList);` — consume `pList` into a new list in random order

- Globals
  - `extern struct list_data *global_lists;`
  - `extern struct list_data *group_list;`

---

## Iterator Lifecycle Management

Iterators in the lists system have a well-defined lifecycle that must be respected to avoid memory issues and bugs:

### Iterator States

1. **Uninitialized**: Stack-allocated iterator before `merge_iterator()`
2. **Active**: After `merge_iterator()` returns non-NULL - iterator is attached to list
3. **Traversing**: During `next_in_list()` calls - iterator moves through list
4. **Detached**: After `remove_iterator()` - iterator is safely disconnected

### Lifecycle Rules

```c
/* RULE 1: Always pair merge_iterator with remove_iterator */
struct iterator_data it;
void *item = merge_iterator(&it, list);  // ATTACH
while (item) {
  // process item
  item = next_in_list(&it);
}
remove_iterator(&it);  // DETACH - DON'T FORGET!

/* RULE 2: Never use an iterator after remove_iterator */
remove_iterator(&it);
// item = next_in_list(&it);  // WRONG! Iterator is detached

/* RULE 3: Handle early exits properly */
struct iterator_data it;
void *item = merge_iterator(&it, list);
while (item) {
  if (some_condition) {
    remove_iterator(&it);  // Clean up before return!
    return;
  }
  item = next_in_list(&it);
}
remove_iterator(&it);

/* RULE 4: Don't reuse iterators without re-merging */
struct iterator_data it;
process_list(list1, &it);  // Uses merge/remove internally
process_list(list2, &it);  // OK - can reuse after proper cleanup
```

### Iterator Safety Features

- `merge_iterator()` increments list's `iIterators` counter for diagnostics
- `remove_iterator()` safely handles NULL list (already detached)
- `next_in_list()` returns NULL at end of list automatically
- Multiple iterators can traverse the same list simultaneously

### Simple List Iterator Management

The `simple_list()` function manages its own internal static iterator:

```c
/* Lifecycle is automatic but requires discipline */
simple_list(NULL);              // 1. RESET (detaches if needed)
while ((item = simple_list(list))) {  // 2. ATTACH on first call
  // process                    // 3. TRAVERSE on each call
}                              // 4. AUTO-DETACH at end
simple_list(NULL);              // 5. EXPLICIT RESET (recommended)
```

**WARNING**: The simple_list iterator is global static state. Always reset before use!

---

## Typical Usage Patterns

### Creating and populating a list

```c
struct list_data *monsters = create_list();
add_to_list(dragon, monsters);
add_to_list(goblin, monsters);
```

The list takes ownership of the node wrappers only. The caller retains ownership of the `pContent` memory.

### Iterating with an explicit iterator

```c
struct iterator_data it;
struct mob_data *mob = merge_iterator(&it, monsters);
for (; mob; mob = next_in_list(&it)) {
  // use mob
}
remove_iterator(&it);
```

Always call `remove_iterator` when done. The iterator API returns `void *`; cast to your concrete type.

### Iterating with removal (safe pattern)

Because removing the current element invalidates the iterator’s current node, prefetch the next content before deleting:

```c
struct iterator_data it;
struct obj_data *obj = merge_iterator(&it, some_list);
while (obj) {
  struct obj_data *next = next_in_list(&it); // advance iterator first

  if (should_delete(obj)) {
    remove_from_list(obj, some_list); // safe: iterator already moved past
    /* free(obj); // if appropriate — list does not free pContent */
  }

  obj = next;
}
remove_iterator(&it);
```

Avoid removing the current element before advancing; doing so can leave the iterator pointing at freed memory.

### Using the convenience iterator `simple_list`

```c
struct char_data *member = NULL;
simple_list(NULL);  // Reset before use (recommended)
while ((member = simple_list(group_list))) {
  // use member
}
simple_list(NULL);  // Clean up after use (recommended)
```

`simple_list` stores static state and is not reentrant. Use it only for a single, flat loop. **NEVER nest `simple_list` loops** - this will cause the inner loop to reset the outer loop's state. 

Best practice is to always call `simple_list(NULL)` before and after your loop to ensure clean state. The function will automatically reset when switching between different lists, but explicit resets prevent subtle bugs.

### Helper Macros for Safer List Operations

As of 2025-01-09, helper macros are available for safer list operations:

```c
// Safe removal with NULL checks
SAFE_REMOVE_FROM_LIST(item, my_list);  // Won't crash on NULL

// Manual iterator cleanup for early exits
struct char_data *ch;
simple_list(NULL);  // Reset before use (or use SIMPLE_LIST_CLEANUP())
while ((ch = (struct char_data *)simple_list(group_list))) {
  if (some_condition) {
    SIMPLE_LIST_CLEANUP();  // Reset iterator before early exit
    return;
  }
  // Process ch
}
```

**Note on C89/C90 Compatibility**: The originally planned `SIMPLE_LIST_FOREACH` macro cannot be implemented due to the codebase's C89/C90 standard requirement (which doesn't allow variable declarations in for-loop initializers). Continue using the traditional while-loop pattern with explicit resets as shown above.

The `SAFE_REMOVE_FROM_LIST` macro adds NULL safety checks to prevent crashes, and `SIMPLE_LIST_CLEANUP()` provides a convenient way to reset the iterator.

### Random selection and shuffling

```c
void *any = random_from_list(monsters);

struct list_data *shuffled = randomize_list(monsters);
// Note: monsters is consumed; use shuffled from now on.
```

`randomize_list` repeatedly picks a random element and moves it into a fresh list, frees the original container, and returns the new list.

Edge cases:
- If the input list is empty (`iSize == 0`), `randomize_list` frees the original list and returns NULL. The original list pointer becomes invalid after this call.

### Common Pattern Examples

#### Pattern 1: Processing Group Members
```c
struct char_data *member;
simple_list(NULL);  // Always reset first
while ((member = (struct char_data *)simple_list(ch->group->members))) {
  if (IN_ROOM(member) != IN_ROOM(ch))
    continue;  // Skip members in different rooms
  
  // Apply group buff/effect
  affect_to_char(member, &af);
}
simple_list(NULL);  // Clean up after
```

#### Pattern 2: Finding First Match
```c
struct obj_data *obj;
struct obj_data *found = NULL;
simple_list(NULL);
while ((obj = (struct obj_data *)simple_list(object_list))) {
  if (GET_OBJ_VNUM(obj) == target_vnum) {
    found = obj;
    break;  // Found it!
  }
}
simple_list(NULL);  // Important: Reset even after break!
```

#### Pattern 3: Counting with Conditions
```c
int count = 0;
struct char_data *ch;
simple_list(NULL);
while ((ch = (struct char_data *)simple_list(character_list))) {
  if (!IS_NPC(ch) && GET_LEVEL(ch) >= 10)
    count++;
}
simple_list(NULL);
```

#### Pattern 4: Building a Filtered List
```c
struct list_data *high_level_players = create_list();
struct char_data *ch;

simple_list(NULL);
while ((ch = (struct char_data *)simple_list(character_list))) {
  if (!IS_NPC(ch) && GET_LEVEL(ch) >= 50) {
    add_to_list(ch, high_level_players);
  }
}
simple_list(NULL);

// Use the filtered list...
// Don't forget to free it when done!
free_list(high_level_players);
```

#### Pattern 5: Safe Removal During Iteration
```c
struct iterator_data it;
struct obj_data *obj = merge_iterator(&it, room->contents);
while (obj) {
  struct obj_data *next = next_in_list(&it);  // Get next BEFORE removal
  
  if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
    remove_from_list(obj, room->contents);   // Safe because we saved next
    extract_obj(obj);
  }
  
  obj = next;
}
remove_iterator(&it);
```

#### Pattern 6: Nested Iteration (MUST use explicit iterators!)
```c
struct iterator_data room_it, char_it;
struct room_data *room;
struct char_data *ch;

room = merge_iterator(&room_it, world);
while (room) {
  ch = merge_iterator(&char_it, room->people);
  while (ch) {
    // Process each character in each room
    send_to_char(ch, "System message!\r\n");
    ch = next_in_list(&char_it);
  }
  remove_iterator(&char_it);
  room = next_in_list(&room_it);
}
remove_iterator(&room_it);
```

#### Pattern 7: Transferring Items Between Lists
```c
struct list_data *source = get_source_list();
struct list_data *dest = create_list();
struct iterator_data it;
void *item;

// Transfer all items matching criteria
item = merge_iterator(&it, source);
while (item) {
  void *next = next_in_list(&it);
  if (meets_criteria(item)) {
    remove_from_list(item, source);
    add_to_list(item, dest);
  }
  item = next;
}
remove_iterator(&it);
```

---

## Semantics and Invariants

- `add_to_list` appends to the tail in O(1) and increments `iSize`.
- `remove_from_list` detaches the node if present, decrements `iSize`, and frees the node wrapper.
- The list never frees or touches `pContent` ownership.
- `find_in_list` and removal use pointer equality on `pContent`.
- `merge_iterator` logs a SYSERR if the list is NULL and a WARNING if the list is empty; it returns NULL in both cases.
- `next_in_list` logs a SYSERR only if the iterator's list pointer is NULL; it returns NULL silently at end-of-list when `pItem` is NULL.
- `simple_list(NULL)` resets the static cursor in the Luminari implementation. `clear_simple_list()` has been removed; always use `simple_list(NULL)` for resets.

---

## Error Handling and Logging

The implementation uses the standard MUD logging (`mudlog`) with staff levels to highlight misuse:

- Merging an iterator to a NULL list → SYSERR via `mudlog(CMP, LVL_GRSTAFF, ...)`.
- Merging an iterator to an empty list → WARNING via `mudlog(NRM, LVL_STAFF, ...)`.
- Calling `next_in_list` with an iterator whose `pList` is NULL → SYSERR via `mudlog(CMP, LVL_GRSTAFF, ...)`.
- Removing an element not present in the list → WARNING via `mudlog(NRM, LVL_STAFF, ...)`.
- ~~Detaching an iterator that is already detached (NULL list on the iterator) → warning via `mudlog`.~~ **FIXED (2025-08-08)**: Now returns silently as this is a normal condition.
- `simple_list` forced to reset itself when switching lists without proper cleanup → SYSERR via `mudlog(CMP, LVL_GRSTAFF, ...)`.

These are runtime diagnostics; they do not abort execution. Treat them as correctness signals during development.

---

## Performance Characteristics

- `add_to_list`: O(1)
- `remove_from_list`: O(n) overall (O(1) unlink after O(n) pointer-compare search)
- `find_in_list`: O(n)
- `merge_iterator`/`next_in_list`: O(1) per step
- `random_from_list`: O(n) due to counting pass
- `randomize_list`: O(n^2) worst-case (n selections × O(n) each); acceptable for small/medium lists
- `free_list`: O(n) since 2025-08-09 (direct node traversal without separate searches)

Guidance:
- For very hot paths, avoid repeated `find_in_list` by structuring logic to remove via known `pContent` after a prior `next_in_list` advance.
- Lists are intended for modest sizes; if you need frequent membership tests at scale, consider an auxiliary hash or flag structure.

---

## Memory Management

- The list owns only the node wrappers (`struct item_data`) and the container (`struct list_data`).
- Callers own `pContent`. Neither `remove_from_list` nor `free_list` will free your objects.
- `free_list` clears the list by directly traversing and freeing all nodes (O(n)), then frees the container. It does not call `remove_from_list` per item. Passing `NULL` to `free_list` is a no-op and safe.

Checklist:
- Free or recycle your `pContent` explicitly when removing from the list, if appropriate.
- After `randomize_list`, the original list pointer is invalid; do not use it.

---

## Reentrancy, Nesting, and Safety

- `simple_list` is not reentrant and must not be nested. Use explicit iterators for nested loops or recursion.
- Modifying a list while an iterator is attached is allowed but requires the advance-first pattern shown above.
- The system is not thread-safe. Do not access a list from multiple threads.

---

## Global Lists

- `global_lists`: Intended as a debugging registry of all lists created after the first. The very first `create_list` call does not add itself to `global_lists`. For proper bootstrapping, assign the first created list to `global_lists` early (for example, `global_lists = create_list();`), then create all subsequent lists so they auto-register. If you do not initialize `global_lists`, later auto-registration/deregistration attempts will pass a NULL registry and emit SYSERR diagnostics.
- `group_list`: A shared list used by group mechanics. Treat it as a typical `list_data *` with `struct char_data *` entries.

---

## Common Pitfalls

- **Removing during iteration**: Removing the current element without advancing the iterator first can leave the iterator pointing to freed memory. Always use the advance-first pattern shown in examples.
- **Memory ownership confusion**: The list will NEVER free your `pContent`. You must free it separately if needed.
- **Iterator leaks**: Forgetting `remove_iterator` leaves `iIterators` inflated. While not critical after recent fixes, it's still good practice.
- **Nesting `simple_list`**: NEVER nest `simple_list` loops or call functions that use `simple_list` from within a `simple_list` loop. It uses static state and will corrupt the outer loop.
- **Pointer vs value equality**: All lookups use pointer equality only, not value comparison.
- **NULL safety**: After recent fixes (2025-08-08), the API is more robust against NULL inputs, but you should still check return values.

---

## Minimal End-to-End Example

```c
struct list_data *players = create_list();
add_to_list(ch_alice, players);
add_to_list(ch_bob, players);

// Iterate and selectively remove
struct iterator_data it;
struct char_data *ch = merge_iterator(&it, players);
while (ch) {
  struct char_data *next = next_in_list(&it);
  if (IS_NPC(ch)) {
    remove_from_list(ch, players);
    // free(ch); // if allocated for this context
  }
  ch = next;
}
remove_iterator(&it);

free_list(players);
```

---

## Notes on tbaMUD vs Luminari Adjustments

- The original file retains a legacy implementation under a compile-time guard. Luminari's active implementation:
  - Uses `simple_list(NULL)` to reset the static convenience iterator.
  - **2025-08-08 Updates**:
    - `remove_iterator()` now silently handles NULL list (no warning spam)
    - `next_in_list()` has additional NULL safety checks for `pItem`
    - `simple_list()` properly cleans up iterators when resetting
    - Comprehensive beginner-friendly comments added throughout
  - **2025-08-09 Updates**:
    - `free_list()` optimized from O(n²) to O(n) by directly freeing nodes
    - `simple_list()` includes added protection when switching lists after a list was freed to avoid use-after-free during cleanup
  - `clear_simple_list()` has been removed; reset via `simple_list(NULL)`.

---

## Troubleshooting Common Mistakes

### Problem: "SYSERR: simple_list() forced to reset itself"

**Cause**: You switched lists without resetting or tried to nest simple_list loops.

**Solutions**:
```c
// Always reset before starting a new loop
simple_list(NULL);
while ((item = simple_list(list))) { /* ... */ }

// For nested loops, use explicit iterators
struct iterator_data it;
while ((item1 = simple_list(list1))) {
  item2 = merge_iterator(&it, list2);
  while (item2) {
    item2 = next_in_list(&it);
  }
  remove_iterator(&it);
}
```

### Problem: Crash when iterating a list

**Possible Causes & Solutions**:

1. **List was freed during iteration**
   ```c
   // WRONG
   while ((item = simple_list(list))) {
     if (condition)
       free_list(list);  // Crash on next iteration!
   }
   
   // RIGHT - break after freeing
   while ((item = simple_list(list))) {
     if (condition) {
       simple_list(NULL);  // Reset first
       free_list(list);
       break;
     }
   }
   ```

2. **Forgot to reset after early exit**
   ```c
   // WRONG
   while ((item = simple_list(list))) {
     if (found)
       return;  // Iterator still attached!
   }
   
   // RIGHT
   while ((item = simple_list(list))) {
     if (found) {
       simple_list(NULL);  // Reset before return
       return;
     }
   }
   ```

### Problem: Iterator appears to skip items or repeat

**Cause**: Modifying list during iteration without proper next-caching.

**Solution**:
```c
// Cache next before any list modifications
struct iterator_data it;
void *item = merge_iterator(&it, list);
while (item) {
  void *next = next_in_list(&it);  // Cache BEFORE modification
  if (should_remove(item)) {
    remove_from_list(item, list);   // Now safe to modify
  }
  item = next;  // Use cached next
}
remove_iterator(&it);
```

### Problem: Memory leaks with lists

**Common Causes**:

1. **Forgot to free list contents**
   ```c
   // WRONG - leaks all content
   free_list(my_list);
   
   // RIGHT - free content first if you own it
   while ((obj = simple_list(my_list))) {
     remove_from_list(obj, my_list);
     free(obj);  // Free the actual object
   }
   simple_list(NULL);
   free_list(my_list);
   ```

2. **Lost list pointer**
   ```c
   // WRONG - old list leaked
   my_list = create_list();
   
   // RIGHT - free old list first
   if (my_list)
     free_list(my_list);
   my_list = create_list();
   ```

### Problem: "WARNING: Attempting to merge iterator to empty list"

**This is usually harmless** - it just means the list has no items. Handle gracefully:

```c
struct iterator_data it;
void *item = merge_iterator(&it, list);
if (!item) {
  // List is empty, nothing to process
  return;
}
// Process non-empty list...
```

### Problem: Infinite loop with simple_list

**Cause**: List is being modified in a way that creates cycles or corruption.

**Debug Steps**:
1. Add logging to track list size
2. Check for circular references (item pointing to itself)
3. Verify list isn't being modified by another function during iteration
4. Ensure you're not accidentally adding items during iteration

```c
// Debug version
int safety = 0;
simple_list(NULL);
while ((item = simple_list(list)) && safety++ < 10000) {
  mudlog(CMP, LVL_IMPL, TRUE, "Processing item %d of estimated %d", 
         safety, list->iSize);
  // ... process ...
}
if (safety >= 10000)
  mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Infinite loop detected!");
simple_list(NULL);
```

---

## When to Use

- Small/medium collections of game entities where order is either insertion or randomized on demand.
- Simple registries, queues, and group membership sets.
- Scenarios where you do not require key-based lookup or stable indices.

If you need frequent membership tests or large-scale lookups, consider augmenting with a map or bitset alongside the list.

---

## Quick Reference

- **Create**: `list = create_list();`
- **Add**: `add_to_list(ptr, list);`
- **Iterate (explicit)**:
  ```c
  struct iterator_data it;
  void *item = merge_iterator(&it, list);
  while (item) {
    // use item
    item = next_in_list(&it);
  }
  remove_iterator(&it);
  ```
- **Iterate (simple)**:
  ```c
  simple_list(NULL);  // Reset first
  while ((x = simple_list(list))) { 
    // use x
  }
  simple_list(NULL);  // Clean up after
  ```
- **Remove**: `remove_from_list(ptr, list);` (does NOT free `ptr`)
- **Random pick**: `random_from_list(list);` (check for NULL return)
- **Shuffle** (consumes original): `list = randomize_list(list);`
- **Free list**: `free_list(list);` (does NOT free contents)
- **Reset simple iterator**: `simple_list(NULL);`

### Critical Rules
- NEVER nest `simple_list()` loops
- ALWAYS advance iterator before removing current item
- ALWAYS call `remove_iterator()` when done with explicit iterator
- ALWAYS free your content separately - lists only manage nodes

