# Generic Lists System

The generic list implementation lives in `src/lists.c` and `src/lists.h`. It is a doubly linked
list of caller-owned opaque pointers (`void *`) for groups, event ownership, craft requirements,
temporary target collections, and other small in-game collections.

The MUD game loop is single-threaded. This API is not thread-safe.

## Ownership

The list owns its container and `struct item_data` nodes. It never frees, copies, or otherwise
owns `pContent`. A caller that owns dynamically allocated content must free that content itself.

`NULL` is the iteration terminator and is not valid list content. `add_to_list(NULL, list)` logs a
SYSERR and leaves the list unchanged.

## Structures and invariants

`struct item_data` contains the previous and next active node, the caller's content pointer, and
internal deferred-removal fields.

`struct list_data` contains:

- `pFirstItem` and `pLastItem`: the active list endpoints.
- `pRemovedItems`: nodes unlinked while an iterator was active and awaiting reclamation.
- `iIterators`: the number of attached explicit and convenience iterators.
- `iSize`: the number of active nodes. It is a `size_t`, so the old 65,535-element wraparound no
  longer exists.
- `pending_free`: destruction was requested and will finish when the final iterator detaches.

For a live list:

- `iSize == 0` means both endpoints are `NULL`.
- A non-empty first node has no previous node.
- A non-empty last node has no next node.
- Forward and backward links agree.
- Active nodes have non-NULL content and are not marked removed.
- Removed nodes are reclaimed immediately when no iterator exists, or after the final iterator
  detaches otherwise.

## API

### Creation and destruction

```c
struct list_data *create_list(void);
void free_list(struct list_data *list);
```

`create_list()` returns an empty list. If `global_lists` has already been initialized, the new
list is registered there.

`free_list(NULL)` is a no-op. `free_list(list)` never frees content. If no iterator is attached,
it frees the nodes and container immediately. If an iterator is attached, it marks the list for
destruction; the next iterator advance stops, or an explicit detach completes the destruction.
This prevents an iterator from dereferencing freed list nodes.

Do not use a list pointer after passing it to `free_list()`, even if physical reclamation is
temporarily deferred.

### Add, find, and remove

```c
void add_to_list(void *content, struct list_data *list);
struct item_data *find_in_list(void *content, struct list_data *list);
void remove_from_list(void *content, struct list_data *list);
```

`add_to_list()` appends in O(1). `find_in_list()` and `remove_from_list()` compare content by
pointer identity and run in O(n). Duplicate pointers are allowed; removal deletes the first
matching node.

Removal is iterator-safe. The node is unlinked from the active chain immediately. If an iterator
still points to it, reclamation is delayed and the iterator can advance through its preserved
next link. Removed future nodes are skipped automatically. The advance-before-remove pattern is
still valid, but it is no longer required for memory safety.

### Explicit iteration

```c
struct iterator_data iterator;
struct char_data *member;

member = (struct char_data *)merge_iterator(&iterator, group->members);
while (member != NULL)
{
  /* Process member. */
  member = (struct char_data *)next_in_list(&iterator);
}
remove_iterator(&iterator);
```

`merge_iterator()` attaches to a non-empty list, increments `iIterators`, and returns the first
content pointer. An empty list returns `NULL` silently and does not attach.

`next_in_list()` advances and returns the next content pointer. Reaching the end automatically
detaches the iterator. Calling `remove_iterator()` afterward is safe and remains recommended so
the same code also handles an early `break` or `return`.

Always detach explicitly on an early exit:

```c
member = (struct char_data *)merge_iterator(&iterator, group->members);
while (member != NULL)
{
  if (matches(member))
  {
    result = member;
    break;
  }
  member = (struct char_data *)next_in_list(&iterator);
}
remove_iterator(&iterator);
```

Multiple explicit iterators may traverse one list simultaneously. Do not merge an already
attached iterator again; detach it first.

### Convenience iteration

```c
simple_list(NULL);
while ((member = (struct char_data *)simple_list(group->members)) != NULL)
{
  /* Process member. */
}
simple_list(NULL);
```

`simple_list()` uses one internal static iterator. It is non-reentrant and must never be nested.
Reset it before and after each loop, and reset before every early exit. Use explicit iterators for
nested iteration or for functions that can invoke another list scan.

Freeing an unrelated list does not reset an active `simple_list()` traversal. Freeing the list
being traversed safely detaches the convenience iterator first.

### Random selection and shuffling

```c
void *random_from_list(struct list_data *list);
struct list_data *randomize_list(struct list_data *list);
```

`random_from_list()` returns one active content pointer, or `NULL` for an empty list.

`randomize_list()` consumes the input list, moves every content pointer into a newly allocated
list in random order, and returns the new list. This is true for an empty input too: the old empty
container is freed and a valid new empty list is returned. Never use the input pointer afterward.

The current shuffle is O(n^2), which is acceptable for the modest lists used by the game.

## Global registry

`global_lists` is a debugging registry; it does not own the lists stored in it. Boot initializes
it explicitly:

```c
global_lists = create_list();
```

That first call is unregistered because no registry exists yet. Later `create_list()` calls
register automatically. Lists created before registry initialization remain unregistered and can
still be freed safely.

Freeing a registered list removes it from the registry. Freeing `global_lists` clears the global
pointer, so a later initialization does not depend on stale function-static bootstrap state.

`group_list` is an ordinary registered list used by group mechanics. Freeing it clears the global
pointer.

## Error behavior

The API is recoverable and does not abort on bad input:

- NULL list pointers log a SYSERR where the operation requires a list.
- NULL iterator pointers log a SYSERR and return safely.
- Empty iteration and random selection return `NULL` without a warning.
- Removing content not present in a list logs a staff warning.
- Adding NULL content logs a SYSERR and does nothing.
- Mutating or attaching to a list already pending destruction logs a SYSERR and does nothing.
- Repeated `remove_iterator()` calls are silent no-ops.

These diagnostics identify caller defects; they are not normal gameplay messages.

## Performance

| Operation | Complexity |
| --- | --- |
| `create_list` | O(1), plus O(1) registry append |
| `add_to_list` | O(1) |
| `find_in_list` | O(n) |
| `remove_from_list` | O(n) lookup plus O(1) unlink |
| `merge_iterator` | O(1) for a valid list |
| `next_in_list` | O(1) amortized, including skipped removed nodes |
| `random_from_list` | O(n) |
| `randomize_list` | O(n^2) |
| `free_list` | O(n), immediately or after final iterator detach |

## Test coverage

Production-linked coverage is in `unittests/CuTest/test_lists_production.c`. It verifies:

- append, first/middle/last removal, duplicates, and link invariants;
- simultaneous iterators during current and future node removal;
- deferred destruction while an iterator is attached;
- convenience iteration across an unrelated list free;
- NULL-content and NULL-iterator handling;
- populated and empty randomization;
- registry destruction and reinitialization; and
- list sizes beyond the former unsigned-short limit.

Run it through the full production-linked suite:

```bash
make test
make install
```
