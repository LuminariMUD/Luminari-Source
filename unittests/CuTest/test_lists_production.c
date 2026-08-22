#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/lists.h"

#include <limits.h>
#include <string.h>

static void assert_list_integrity(CuTest *tc, struct list_data *list)
{
  struct item_data *item;
  struct item_data *previous;
  size_t count;

  CuAssertPtrNotNull(tc, list);
  CuAssertTrue(tc, !list->pending_free);

  previous = NULL;
  count = 0;
  for (item = list->pFirstItem; item != NULL; item = item->pNextItem)
  {
    CuAssertPtrEquals(tc, previous, item->pPrevItem);
    CuAssertTrue(tc, !item->is_removed);
    CuAssertPtrNotNull(tc, item->pContent);
    previous = item;
    count++;
  }

  CuAssertPtrEquals(tc, previous, list->pLastItem);
  CuAssertTrue(tc, count == list->iSize);
  if (count == 0)
  {
    CuAssertPtrEquals(tc, NULL, list->pFirstItem);
    CuAssertPtrEquals(tc, NULL, list->pLastItem);
  }
  else
  {
    CuAssertPtrEquals(tc, NULL, list->pFirstItem->pPrevItem);
    CuAssertPtrEquals(tc, NULL, list->pLastItem->pNextItem);
  }
}

void Test_lists_append_remove_and_duplicate_pointer_invariants(CuTest *tc)
{
  struct list_data *list;
  int first;
  int second;

  first = 1;
  second = 2;
  list = create_list();

  add_to_list(&first, list);
  add_to_list(&second, list);
  add_to_list(&first, list);
  assert_list_integrity(tc, list);
  CuAssertTrue(tc, list->iSize == 3);
  CuAssertPtrEquals(tc, &first, list->pFirstItem->pContent);
  CuAssertPtrEquals(tc, &first, list->pLastItem->pContent);

  remove_from_list(&first, list);
  assert_list_integrity(tc, list);
  CuAssertTrue(tc, list->iSize == 2);
  CuAssertPtrEquals(tc, &second, list->pFirstItem->pContent);
  CuAssertPtrEquals(tc, &first, list->pLastItem->pContent);

  remove_from_list(&first, list);
  remove_from_list(&second, list);
  assert_list_integrity(tc, list);
  free_list(list);
}

void Test_lists_iterators_survive_current_and_future_removal(CuTest *tc)
{
  struct list_data *list;
  struct iterator_data first_iterator;
  struct iterator_data second_iterator;
  int first;
  int second;
  int third;

  memset(&first_iterator, 0, sizeof(first_iterator));
  memset(&second_iterator, 0, sizeof(second_iterator));
  first = 1;
  second = 2;
  third = 3;
  list = create_list();
  add_to_list(&first, list);
  add_to_list(&second, list);
  add_to_list(&third, list);

  CuAssertPtrEquals(tc, &first, merge_iterator(&first_iterator, list));
  CuAssertPtrEquals(tc, &first, merge_iterator(&second_iterator, list));
  CuAssertTrue(tc, list->iIterators == 2);

  remove_from_list(&first, list);
  CuAssertTrue(tc, list->iSize == 2);
  CuAssertPtrNotNull(tc, list->pRemovedItems);
  CuAssertPtrEquals(tc, &second, next_in_list(&first_iterator));
  CuAssertPtrEquals(tc, &second, next_in_list(&second_iterator));

  remove_from_list(&third, list);
  CuAssertPtrEquals(tc, NULL, next_in_list(&first_iterator));
  CuAssertTrue(tc, list->iIterators == 1);
  CuAssertPtrEquals(tc, NULL, next_in_list(&second_iterator));
  CuAssertTrue(tc, list->iIterators == 0);
  CuAssertPtrEquals(tc, NULL, list->pRemovedItems);
  assert_list_integrity(tc, list);

  remove_from_list(&second, list);
  free_list(list);
}

void Test_lists_free_waits_for_active_iterators(CuTest *tc)
{
  struct list_data *list;
  struct iterator_data iterator;
  int first;
  int second;

  memset(&iterator, 0, sizeof(iterator));
  first = 1;
  second = 2;
  list = create_list();
  add_to_list(&first, list);
  add_to_list(&second, list);

  CuAssertPtrEquals(tc, &first, merge_iterator(&iterator, list));
  free_list(list);
  CuAssertTrue(tc, list->pending_free);
  CuAssertTrue(tc, list->iIterators == 1);

  CuAssertPtrEquals(tc, NULL, next_in_list(&iterator));
  CuAssertPtrEquals(tc, NULL, iterator.pList);
  CuAssertPtrEquals(tc, NULL, iterator.pItem);
}

void Test_lists_simple_iteration_survives_unrelated_list_free(CuTest *tc)
{
  struct list_data *iterated;
  struct list_data *unrelated;
  int first;
  int second;
  int other;

  first = 1;
  second = 2;
  other = 3;
  iterated = create_list();
  unrelated = create_list();
  add_to_list(&first, iterated);
  add_to_list(&second, iterated);
  add_to_list(&other, unrelated);

  simple_list(NULL);
  CuAssertPtrEquals(tc, &first, simple_list(iterated));
  CuAssertTrue(tc, iterated->iIterators == 1);
  free_list(unrelated);
  CuAssertTrue(tc, iterated->iIterators == 1);
  CuAssertPtrEquals(tc, &second, simple_list(iterated));
  CuAssertPtrEquals(tc, NULL, simple_list(iterated));
  CuAssertTrue(tc, iterated->iIterators == 0);

  free_list(iterated);
}

void Test_lists_reject_null_content_and_null_iterator_arguments(CuTest *tc)
{
  struct list_data list;
  struct iterator_data iterator;

  memset(&list, 0, sizeof(list));
  memset(&iterator, 0, sizeof(iterator));

  add_to_list(NULL, &list);
  CuAssertTrue(tc, list.iSize == 0);
  CuAssertPtrEquals(tc, NULL, list.pFirstItem);
  CuAssertPtrEquals(tc, NULL, merge_iterator(NULL, &list));
  CuAssertPtrEquals(tc, NULL, next_in_list(NULL));
  remove_iterator(NULL);
  CuAssertPtrEquals(tc, NULL, merge_iterator(&iterator, &list));
  CuAssertTrue(tc, list.iIterators == 0);
}

void Test_lists_randomization_preserves_members_and_empty_lists(CuTest *tc)
{
  struct list_data *list;
  struct list_data *randomized;
  int first;
  int second;
  int third;

  first = 1;
  second = 2;
  third = 3;
  list = create_list();
  add_to_list(&first, list);
  add_to_list(&second, list);
  add_to_list(&third, list);

  randomized = randomize_list(list);
  assert_list_integrity(tc, randomized);
  CuAssertTrue(tc, randomized->iSize == 3);
  CuAssertPtrNotNull(tc, find_in_list(&first, randomized));
  CuAssertPtrNotNull(tc, find_in_list(&second, randomized));
  CuAssertPtrNotNull(tc, find_in_list(&third, randomized));
  free_list(randomized);

  list = create_list();
  randomized = randomize_list(list);
  assert_list_integrity(tc, randomized);
  CuAssertTrue(tc, randomized->iSize == 0);
  free_list(randomized);
}

void Test_lists_registry_can_be_destroyed_and_reinitialized(CuTest *tc)
{
  struct list_data *saved_global_lists;
  struct list_data *registry;
  struct list_data *first;
  struct list_data *second;

  saved_global_lists = global_lists;
  global_lists = NULL;

  registry = create_list();
  CuAssertPtrEquals(tc, NULL, global_lists);
  global_lists = registry;
  first = create_list();
  second = create_list();
  CuAssertTrue(tc, registry->iSize == 2);
  CuAssertPtrNotNull(tc, find_in_list(first, registry));
  CuAssertPtrNotNull(tc, find_in_list(second, registry));

  free_list(first);
  CuAssertTrue(tc, registry->iSize == 1);
  free_list(registry);
  CuAssertPtrEquals(tc, NULL, global_lists);
  free_list(second);

  global_lists = saved_global_lists;
}

void Test_lists_size_exceeds_legacy_unsigned_short_limit(CuTest *tc)
{
  struct list_data *list;
  size_t count;
  int content;

  content = 1;
  list = create_list();
  for (count = 0; count <= USHRT_MAX; count++)
    add_to_list(&content, list);

  CuAssertTrue(tc, list->iSize == (size_t)USHRT_MAX + 1);
  assert_list_integrity(tc, list);
  free_list(list);
}
