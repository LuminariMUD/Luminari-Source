/* Regressions for the help-miss suggestion ranking: edit distance and the
 * merge that puts close spellings ahead of phonetic matches. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/help.h"

static struct help_keyword_list *make_suggestion(const char *tag, const char *keyword)
{
  struct help_keyword_list *node;

  node = calloc(1, sizeof(*node));
  if (node == NULL)
    return NULL;
  node->tag = strdup(tag);
  node->keyword = strdup(keyword);
  node->next = NULL;
  return node;
}

static struct help_keyword_list *make_suggestions(const char *const *keywords, int count)
{
  struct help_keyword_list *head = NULL, *tail = NULL, *node;
  int index;

  for (index = 0; index < count; index++)
  {
    node = make_suggestion(keywords[index], keywords[index]);
    if (node == NULL)
      break;
    if (tail)
      tail->next = node;
    else
      head = node;
    tail = node;
  }
  return head;
}

static void free_suggestions(struct help_keyword_list *list)
{
  struct help_keyword_list *next;

  while (list)
  {
    next = list->next;
    free(list->tag);
    free(list->keyword);
    free(list);
    list = next;
  }
}

void Test_help_edit_distance_counts_single_edits(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, help_keyword_edit_distance("stealth", "stealth", 3));
  CuAssertIntEquals(tc, 1, help_keyword_edit_distance("logds", "logs", 2));
  CuAssertIntEquals(tc, 1, help_keyword_edit_distance("preperation", "preparation", 2));
  CuAssertIntEquals(tc, 1, help_keyword_edit_distance("stelth", "stealth", 2));
  CuAssertIntEquals(tc, 2, help_keyword_edit_distance("sorceror", "sorcerers", 3));
}

void Test_help_edit_distance_ignores_case(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, help_keyword_edit_distance("Preparation", "PREPARATION", 1));
  CuAssertIntEquals(tc, 1, help_keyword_edit_distance("TROWN", "thrown", 1));
}

void Test_help_edit_distance_stops_past_limit(CuTest *tc)
{
  CuAssertIntEquals(tc, 2, help_keyword_edit_distance("logds", "liquids", 1));
  CuAssertIntEquals(tc, 2, help_keyword_edit_distance("mrwolkto", "mobility", 1));
  CuAssertIntEquals(tc, 3, help_keyword_edit_distance("a", "abcdefgh", 2));
  CuAssertIntEquals(tc, 1, help_keyword_edit_distance(NULL, "logs", 0));
}

/* The keyword at a list position, or NULL when the list is shorter. */
static const char *keyword_at(const struct help_keyword_list *list, int index)
{
  while (list != NULL && index > 0)
  {
    list = list->next;
    index--;
  }
  return list != NULL ? list->keyword : NULL;
}

void Test_help_merge_suggestions_prefers_primary_and_dedupes(CuTest *tc)
{
  static const char *const close[] = {"logs", "logins"};
  static const char *const phonetic[] = {"LIQUIDS", "Logs", "lichtouch"};
  struct help_keyword_list *merged;

  merged = help_merge_suggestions(make_suggestions(close, 2), make_suggestions(phonetic, 3), 10);

  CuAssertStrEquals(tc, "logs", keyword_at(merged, 0));
  CuAssertStrEquals(tc, "logins", keyword_at(merged, 1));
  CuAssertStrEquals(tc, "LIQUIDS", keyword_at(merged, 2));
  CuAssertStrEquals(tc, "lichtouch", keyword_at(merged, 3));
  CuAssertTrue(tc, keyword_at(merged, 4) == NULL);
  free_suggestions(merged);
}

void Test_help_merge_suggestions_caps_and_handles_empty_lists(CuTest *tc)
{
  static const char *const phonetic[] = {"one", "two", "three"};
  struct help_keyword_list *merged;
  int count = 0;

  merged = help_merge_suggestions(NULL, make_suggestions(phonetic, 3), 2);
  for (struct help_keyword_list *node = merged; node; node = node->next)
    count++;
  CuAssertIntEquals(tc, 2, count);
  CuAssertStrEquals(tc, "one", keyword_at(merged, 0));
  free_suggestions(merged);

  CuAssertPtrEquals(tc, NULL, help_merge_suggestions(NULL, NULL, 10));
}
