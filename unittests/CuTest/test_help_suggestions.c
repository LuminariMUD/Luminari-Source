/* Regressions for the help-miss suggestion ranking: edit distance and the
 * merge that puts close spellings ahead of phonetic matches. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/help.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/character/backgrounds.h"
#include "../../src/character/feats.h"
#include "../../src/net/protocol.h"

#include <string.h>

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

/* The region and background handlers capitalize the word after each space. A
 * trailing space stepped their loops past the terminator, reading and
 * rewriting memory beyond the copied argument. */
void Test_help_name_handlers_stop_at_a_trailing_space(CuTest *tc)
{
  assign_backgrounds();
  CuAssertIntEquals(tc, 0, handle_region_help(NULL, "qzx ", "qzx ", NULL));
  CuAssertIntEquals(tc, 0, handle_background_help(NULL, "qzx ", "qzx ", NULL));
}

/* The production help log had 'feat eidolon': players ask for 'help feat <name>'. The feat
 * handler shows the named feat with or without that leading word (issue 224). */
void Test_help_feat_handler_accepts_a_leading_feat(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  int bare, prefixed, spaced, unknown;
  bool shown;

  if (feat_list[FEAT_EIDOLON].name == NULL || strcmp(feat_list[FEAT_EIDOLON].name, "eidolon") != 0)
    assign_feats();
  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  memset(&descriptor, 0, sizeof(descriptor));
  ch.player_specials = &specials;
  ch.desc = &descriptor;
  descriptor.character = &ch;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  STATE(&descriptor) = CON_PLAYING;

  bare = handle_feat_help(&ch, "eidolon", "eidolon", NULL);
  descriptor.small_outbuf[0] = '\0';
  descriptor.bufptr = 0;
  prefixed = handle_feat_help(&ch, "feat-eidolon", "feat eidolon", NULL);
  shown = strstr(descriptor.output, "eidolon") != NULL;
  spaced = handle_feat_help(&ch, "Feat--eidolon", "Feat  eidolon", NULL);
  unknown = handle_feat_help(&ch, "feat-qzx", "feat qzx", NULL);

  if (descriptor.large_outbuf != NULL)
  {
    free(descriptor.large_outbuf->text);
    free(descriptor.large_outbuf);
  }
  ProtocolDestroy(descriptor.pProtocol);

  CuAssertIntEquals(tc, 1, bare);
  CuAssertIntEquals(tc, 1, prefixed);
  CuAssertTrue(tc, shown);
  CuAssertIntEquals(tc, 1, spaced);
  CuAssertIntEquals(tc, 0, unknown);
}
