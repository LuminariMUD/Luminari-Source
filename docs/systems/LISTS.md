### Lists System (src/lists.h, src/lists.c)

This document describes LuminariMUD’s generic list facility, a lightweight, intrusive wrapper around a doubly linked list that stores opaque pointers (`void *`). The system is designed for simple in-game collections (groups, global registries, work queues) with predictable performance and minimal overhead.

The implementation originates from tbaMUD but has Luminari-specific adjustments. It is not thread-safe and is intended for the MUD’s single-threaded game loop.

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
  - `void *simple_list(struct list_data *pList);` — convenience, non-reentrant cursor with static state
  - `void clear_simple_list(void);` — legacy reset helper (see notes)

- Randomization utilities
  - `void *random_from_list(struct list_data *pList);` — uniformly pick one `pContent` or NULL if empty
  - `struct list_data *randomize_list(struct list_data *pList);` — consume `pList` into a new list in random order

- Globals
  - `extern struct list_data *global_lists;`
  - `extern struct list_data *group_list;`

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
while ((member = simple_list(group_list))) {
  // use member
}
// Do not nest simple_list loops. To force-reset: simple_list(NULL);
```

`simple_list` stores static state and is not reentrant. Use it only for a single, flat loop. If you must early-return from the loop and plan to reuse `simple_list` soon after, call `simple_list(NULL)` first to reset the internal state.

### Random selection and shuffling

```c
void *any = random_from_list(monsters);

struct list_data *shuffled = randomize_list(monsters);
// Note: monsters is consumed; use shuffled from now on.
```

`randomize_list` repeatedly picks a random element and moves it into a fresh list, frees the original container, and returns the new list.

---

## Semantics and Invariants

- `add_to_list` appends to the tail in O(1) and increments `iSize`.
- `remove_from_list` detaches the node if present, decrements `iSize`, and frees the node wrapper.
- The list never frees or touches `pContent` ownership.
- `find_in_list` and removal use pointer equality on `pContent`.
- `merge_iterator` logs warnings if the list is NULL or empty and returns NULL.
- `next_in_list` logs and returns NULL if the iterator is detached or has a NULL node.
- `simple_list(NULL)` resets the static cursor in the Luminari implementation. `clear_simple_list()` exists as a legacy prototype; prefer `simple_list(NULL)` for resets.

---

## Error Handling and Logging

The implementation uses the standard MUD logging (`mudlog`, `log`) with staff levels to highlight misuse:

- Merging or iterating on a NULL/empty list → warning via `mudlog`.
- Removing an element not present in the list → warning via `log`.

These are runtime diagnostics; they do not abort execution. Treat them as correctness signals during development.

---

## Performance Characteristics

- `add_to_list`: O(1)
- `remove_from_list`: O(n) overall (O(1) unlink after O(n) pointer-compare search)
- `find_in_list`: O(n)
- `merge_iterator`/`next_in_list`: O(1) per step
- `random_from_list`: O(n) due to counting pass
- `randomize_list`: O(n^2) worst-case (n selections × O(n) each); acceptable for small/medium lists

Guidance:
- For very hot paths, avoid repeated `find_in_list` by structuring logic to remove via known `pContent` after a prior `next_in_list` advance.
- Lists are intended for modest sizes; if you need frequent membership tests at scale, consider an auxiliary hash or flag structure.

---

## Memory Management

- The list owns only the node wrappers (`struct item_data`) and the container (`struct list_data`).
- Callers own `pContent`. Neither `remove_from_list` nor `free_list` will free your objects.
- `free_list` clears the list by removing all items, then frees the container. It attempts a safe internal iteration pattern that caches `next` pointers before removal.

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

- `global_lists`: Intended as a debugging registry of all lists created after the first. The very first `create_list` call initializes the first list and does not add it to `global_lists`. If you want a central registry, ensure `global_lists` is created early and subsequent lists are created afterward.
- `group_list`: A shared list used by group mechanics. Treat it as a typical `list_data *` with `struct char_data *` entries.

---

## Common Pitfalls

- Removing the current element without advancing the iterator first — can leave the iterator pointing to freed memory. Use the advance-first pattern.
- Assuming ownership: the list will not free your `pContent`.
- Forgetting `remove_iterator` — leaves `iIterators` inflated and may complicate debugging.
- Nesting `simple_list` or using it in parallel contexts — it uses static state and will reset itself with a warning if misused.
- Relying on value equality — lookups compare pointer equality only.

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

- The original file retains a legacy implementation under a compile-time guard. Luminari’s active implementation:
  - Uses `simple_list(NULL)` to reset the static convenience iterator.
  - Strengthens some safety checks and logging in `next_in_list` and iterator attachment/removal.
  - Keeps `clear_simple_list` declared for compatibility, but typical code paths should prefer `simple_list(NULL)` to reset.

---

## When to Use

- Small/medium collections of game entities where order is either insertion or randomized on demand.
- Simple registries, queues, and group membership sets.
- Scenarios where you do not require key-based lookup or stable indices.

If you need frequent membership tests or large-scale lookups, consider augmenting with a map or bitset alongside the list.

---

## Quick Reference

- Create: `list = create_list();`
- Add: `add_to_list(ptr, list);`
- Iterate: `merge_iterator` → loop `next_in_list` → `remove_iterator`
- Convenience iterate: `while ((x = simple_list(list))) { ... }`
- Remove: `remove_from_list(ptr, list);` (does not free `ptr`)
- Random pick: `random_from_list(list);`
- Shuffle (consumes original): `list = randomize_list(list);`
- Reset `simple_list`: `simple_list(NULL);`

