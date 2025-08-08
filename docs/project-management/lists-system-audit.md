## Lists System Audit (LuminariMUD)

Date: 2025-08-08

### Scope

This audit reviews the in-game lists facility (`src/lists.h`, `src/lists.c`) and its usage across the codebase. It covers data structures and API, where/how lists are created and iterated, memory management patterns, randomization utilities, global registries, potential pitfalls seen in code, and recommendations.

References: see system overview in `docs/systems/LISTS.md`.

### Data structures and API (brief)

- **Containers**: `struct list_data` with `pFirstItem`, `pLastItem`, `iIterators`, `iSize`.
- **Nodes**: `struct item_data` with `pPrevItem`, `pNextItem`, `pContent` (opaque pointer; caller-owned).
- **Iterators**: `struct iterator_data { pList, pItem }` for explicit traversal.
- **Key functions**:
  - Creation/teardown: `create_list`, `free_list` (frees nodes + container, not contents).
  - Core ops: `add_to_list`, `remove_from_list`, `find_in_list`.
  - Iteration: `merge_iterator`, `next_in_list`, `remove_iterator`, plus non-reentrant `simple_list` and reset via `simple_list(NULL)`.
  - Randomization: `random_from_list`, `randomize_list` (consumes original; returns new; empty input returns NULL and does not free input).
  - Globals: `global_lists`, `group_list`.

Implementation notes:
- Active implementation is the Luminari variant (second half of `src/lists.c`). Legacy tbaMUD implementation remains under `#ifdef LEAVE_THIS_COMMENTED_OUT` for reference.
- Header declares `clear_simple_list()`, but the active implementation resets using `simple_list(NULL)`; `clear_simple_list` is not used outside the legacy block and can be treated as legacy-compat.

### Where lists are used (inventory)

- **Initialization / Globals**
  - `src/db.c`: initializes `global_lists`, `group_list`, `global_craft_list` via `create_list()`.
  - `src/mud_event.c`: initializes `world_events` via `create_list()`.

- **Mud events**
  - Lists exist on descriptors (`descriptor_data.events`), characters (`char_specials_ptr->events`), objects (`obj_data.events`), rooms (`room_data.events`), and regions (`wilderness_region.events`). Creation is on-demand if NULL; elements are added with `add_to_list`.
  - Iteration: predominantly uses `simple_list`, with explicit `simple_list(NULL)` reset before/after loops when removal may occur. For destructive passes, code collects into a temporary `list_data` and then cancels/removes items in a second pass to avoid iterator invalidation.
  - Cleanup: when an events list becomes empty, code calls `free_list(list)` and sets the pointer to NULL.

- **Groups**
  - `group_list` holds active `group_data`. Each group has `members` (a `list_data*`).
  - Creation: new groups are added to `group_list` and `group->members` is a fresh list.
  - Iteration: both `simple_list(group->members)` and explicit iterator patterns are used widely (combat, utils, act.other, spells).
  - Removal: members removed with `remove_from_list(ch, group->members)`; group teardown frees members list, removes the group from `group_list`, and frees the group.

- **Combat and actions**
  - Ad-hoc, short-lived lists (e.g., `room_list`, `target_list`) are created with `create_list()`, populated with pointers to chars, iterated, then freed with `free_list()` when done.
  - Random selection of targets uses `random_from_list()`.

- **Crafting system**
  - `global_craft_list` holds all crafts; each craft has a `requirements` list.
  - Iterations use explicit iterators with `remove_iterator` called on all paths. When removing elements, code prefetches `next_in_list` before removal, then frees the payload.

- **Descriptors**
  - `descriptor_data.events` is created and freed around connection lifecycle; similar patterns to character/object/room events.

### Iteration patterns observed

- **Explicit iterator loops**: Common and correct usage:
  - Call `merge_iterator(&it, list)`; loop with `for (x = merge_iterator(...); x; x = next_in_list(&it))`; call `remove_iterator(&it)` afterwards. Many files follow this pattern and call `remove_iterator` in all exit paths.

- **Convenience iterator `simple_list`**:
  - Extensively used for `group->members` and event lists. Many sites properly reset with `simple_list(NULL)` before and after loops, particularly when switching lists or when mutation may occur via downstream calls.
  - Some sites omit an explicit pre-reset when iterating a single list once; this is acceptable as `simple_list` self-resets when switching lists and when it reaches the end. However, it is brittle if nested or if a function using `simple_list` is invoked while a prior `simple_list` loop is active.
  - Nesting of `simple_list` loops is avoided as required.

- **Removal during iteration**:
  - Where removal of elements is required, the code either:
    - Uses a two-pass approach with a temporary list to collect items first; or
    - Uses an explicit iterator and prefetches `next_in_list` before calling `remove_from_list` on the current content.
  - Direct removal within a `simple_list` loop on the same list is not observed in this snapshot; instead, temp lists or iterator prefetch are used.

### Memory management patterns

- `free_list(list)` is called for:
  - Temporary ad-hoc lists (`room_list`, `target_list`, crafting temp lists).
  - Empty events lists (char/obj/room/region) after last removal, pointer set to NULL.
  - Global containers at shutdown (e.g., `world_events`, `global_lists`). Note: freeing `global_lists` frees only its node wrappers and container; it does not free the lists referenced by its elements.

- Payload ownership is handled by callers:
  - When removing requirement nodes in crafting, code frees the actual requirement after `remove_from_list`.
  - When iterating over chars/objs/rooms, payloads are not freed by list teardown (correct).

### Randomization utilities

- `random_from_list(list)`: used widely to pick random group/room targets and craft list elements. Assumes non-empty lists; code often checks `iSize` or handles NULL return defensively.
- `randomize_list(list)`: not used outside the lists module. Note its behavior: returns NULL for empty inputs and does not free the original list in that case; callers must handle and free if needed.

### Global registries

- `global_lists` acts as a registry of lists created after the first `create_list()` call. The first list created is not added. `free_list(pList)` removes `pList` from `global_lists` unless `pList == global_lists`.
- `group_list` is a top-level gameplay list of groups. Display and traversal use `simple_list`, and teardown removes and frees appropriately.

### Potential pitfalls and observations

- **Non-reentrancy of `simple_list`**: The static state in `simple_list` makes it unsafe to nest or to call another `simple_list`-using function from within a `simple_list` loop. Most code follows the contract. Recommendation: when in doubt, prefer explicit iterators.

- **Missing pre-reset in some `simple_list` loops**: Some loops start directly with `while ((x = simple_list(list))) { ... }` without a preceding `simple_list(NULL)`. This usually works but can log a reset warning if another active `simple_list` was iterating a different list. Recommendation: adopt a consistent pattern of calling `simple_list(NULL)` before and after any `simple_list` loop in shared subsystems.

- **Removal while iterating**: Code generally uses safe patterns (two-pass with temp list, or iterator prefetch). Maintain this discipline for any future modifications.

- **Header legacy declaration**: `lists.h` declares `clear_simple_list(void)`, but the active implementation resets via `simple_list(NULL)`. This is harmless but can be confusing. Consider updating documentation/comments to mark it legacy or provide a stub implementation that calls `simple_list(NULL)` for clarity.

- **`randomize_list` empty-input behavior**: Because it returns NULL and does not free the input, any new usage should explicitly free the original when receiving NULL.

### Recommendations (actionable)

- **Standardize `simple_list` usage**:
  - Always call `simple_list(NULL)` immediately before and after any `simple_list(list)` loop in subsystems where nested calls might occur (events, combat, spells, handlers).
  - Add a brief helper macro if desired, e.g., `SIMPLE_LIST_FOR(type, var, list) { ... }`, that wraps reset and loop for consistency.

- **Prefer explicit iterators when modifying**:
  - For loops that remove elements from the iterated list, prefer the explicit iterator pattern and prefetch of `next_in_list`, or use a temp list. Avoid removing from within a `simple_list` loop.

- **Clarify legacy API**:
  - Either remove `clear_simple_list` from the public header (if truly unused externally) or implement it as a thin inline that calls `simple_list(NULL)` with a comment that it is legacy.

- **Add assertions in debug builds**:
  - Optionally add defensive checks (e.g., assert non-NULL `pList` in API functions) for debug builds to catch misuse early.

- **Documentation**:
  - In `docs/systems/LISTS.md`, the current notes already advise using `simple_list(NULL)` for reset. Add a note that some code omits pre-reset for single, isolated loops but a consistent pre/post reset is preferred.

### Quick checklist for contributors

- Use `create_list()` for any new container; remember the list does not own `pContent`.
- When iterating:
  - Simple: `simple_list(NULL); while ((x = simple_list(list))) { ... } simple_list(NULL);`
  - Complex/removing: use explicit iterator, prefetch `next_in_list`, then `remove_from_list` and free payload if appropriate; finally `remove_iterator`.
- For random picks, handle NULL return from `random_from_list`.
- Free containers with `free_list(list)` when done; set pointer to NULL if it is a field.
- Do not nest `simple_list` loops.

### Notable usage sites (non-exhaustive)

- Initialization: `src/db.c` (global lists), `src/mud_event.c` (world events).
- Events management: `src/mud_event.c` (resets, temp lists, cancellation, freeing empty lists).
- Groups: `src/handler.c` (create, iterate, remove; manage `group_list` and `group->members`).
- Combat/Spells: `src/fight.c`, `src/act.offensive.c`, `src/spells.c`, `src/magic.c` (ad-hoc lists, random target selection, freeing).
- Crafting: `src/crafts.c` (explicit iterators with safe removal and freeing, sorting via temp lists).

If you need specific examples or help refactoring a site to the preferred pattern, ping maintainers with the file/function.



