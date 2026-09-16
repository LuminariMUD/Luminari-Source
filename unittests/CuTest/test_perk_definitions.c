/* Contract tests for the perk definition tables in src/character/perk_definitions.c.
 *
 * These characterise the seam that perk_definitions.h documents: what the
 * definition functions are allowed to write into perk_list[], and what the
 * engine in perks.c is therefore entitled to assume. They run against the
 * real tables, so a malformed perk added to any class tree fails here rather
 * than at boot or in play. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/interpreter.h"
#include "../../src/core/db.h"
#include "../../src/net/protocol.h"

#include "../../src/character/class.h"
#include "../../src/character/perks.h"
#include "../../src/character/perk_definitions.h"

#include <stdio.h>
#include <string.h>

/* A slot that init_perks() reset and no definition function claimed. */
static bool perk_is_untouched(int id)
{
  return perk_list[id].id == PERK_UNDEFINED;
}

void Test_perk_definitions_fill_every_defined_slot_completely(CuTest *tc)
{
  int id;
  int defined = 0;

  init_perks();

  for (id = 1; id < NUM_PERKS; id++)
  {
    if (perk_is_untouched(id))
      continue;
    defined++;

    /* The engine indexes perk_list[] by perk id, so the two must agree or
     * every lookup in perks.c returns the wrong perk. */
    CuAssertIntEquals(tc, id, perk_list[id].id);

    CuAssertPtrNotNull(tc, perk_list[id].name);
    CuAssertPtrNotNull(tc, perk_list[id].description);
    CuAssertPtrNotNull(tc, perk_list[id].special_description);
    CuAssertTrue(tc, perk_list[id].name[0] != '\0');
    CuAssertTrue(tc, perk_list[id].description[0] != '\0');

    /* A defined perk has to be purchasable at least once, or it is content
     * the player can see and never take. */
    CuAssertTrue(tc, perk_list[id].cost > 0);
    CuAssertTrue(tc, perk_list[id].max_rank >= 1);

    CuAssertTrue(tc, perk_list[id].associated_class >= 0);
    CuAssertTrue(tc, perk_list[id].associated_class < NUM_CLASSES);

    /* The perk list prints a perk under its tree's header, and only for
     * categories below NUM_PERK_CATEGORIES. An uncategorised perk is listed
     * under no tree at all. */
    CuAssertTrue(tc, perk_list[id].perk_category > PERK_CATEGORY_UNDEFINED);
    CuAssertTrue(tc, perk_list[id].perk_category < NUM_PERK_CATEGORIES);
  }

  /* Guard against a future edit that silently stops calling a whole tree. */
  CuAssertTrue(tc, defined > 0);
  CuAssertIntEquals(tc, defined, count_defined_perks());
}

void Test_perk_definitions_prerequisites_resolve(CuTest *tc)
{
  int id;
  int prereq;

  init_perks();

  for (id = 1; id < NUM_PERKS; id++)
  {
    if (perk_is_untouched(id))
      continue;
    prereq = perk_list[id].prerequisite_perk;

    /* can_purchase_perk() treats both -1 and PERK_UNDEFINED as "no
     * prerequisite", so neither reaches the resolution check. */
    if (prereq == -1 || prereq == PERK_UNDEFINED)
      continue;

    /* A prerequisite outside the table would index past perk_list[]. */
    CuAssertTrue(tc, prereq > 0);
    CuAssertTrue(tc, prereq < NUM_PERKS);

    /* Self-reference would make the perk require itself. */
    CuAssertTrue(tc, prereq != id);

    /* can_purchase_perk() skips a prerequisite that get_perk_by_id() cannot
     * resolve, so pointing at a slot no tree defines lets the perk be bought
     * with no prerequisite at all. */
    CuAssertTrue(tc, !perk_is_untouched(prereq));

    /* Requiring more ranks than the prerequisite can reach makes the perk
     * impossible to buy. */
    CuAssertTrue(tc, perk_list[id].prerequisite_rank >= 0);
    CuAssertTrue(tc, perk_list[id].prerequisite_rank <= perk_list[prereq].max_rank);
  }
}

void Test_perk_definitions_own_their_strings_across_reinit(CuTest *tc)
{
  int first_count;
  int second_count;
  int id;
  char *first_name = NULL;
  int sample = -1;

  init_perks();
  first_count = count_defined_perks();

  for (id = 1; id < NUM_PERKS && sample < 0; id++)
  {
    if (!perk_is_untouched(id))
      sample = id;
  }
  CuAssertTrue(tc, sample > 0);
  first_name = perk_list[sample].name;
  CuAssertPtrNotNull(tc, first_name);

  /* perk_definitions.h requires strdup() copies, never literals or shared
   * storage: init_perks() calls destroy_perks(), which frees them. Running
   * the whole cycle again under the test allocator is what proves it. A
   * literal here would abort in free(); a shared pointer would produce a
   * double free on the second pass. */
  init_perks();
  second_count = count_defined_perks();

  CuAssertIntEquals(tc, first_count, second_count);
  CuAssertPtrNotNull(tc, perk_list[sample].name);
  /* Fresh storage, not the pointer handed out on the first pass. */
  CuAssertTrue(tc, perk_list[sample].name != first_name);
}

void Test_perk_definitions_leave_unclaimed_slots_on_the_sentinel(CuTest *tc)
{
  int id;
  int untouched = 0;

  init_perks();

  for (id = 1; id < NUM_PERKS; id++)
  {
    if (!perk_is_untouched(id))
      continue;
    untouched++;

    /* init_perks() installs sentinel storage in every slot and
     * destroy_perks() recognises it by address. A definition function that
     * partially filled a slot and left the id at PERK_UNDEFINED would break
     * that pairing, so check the reset state survived intact. */
    CuAssertPtrNotNull(tc, perk_list[id].name);
    CuAssertPtrNotNull(tc, perk_list[id].description);
    CuAssertStrEquals(tc, "Undefined", perk_list[id].name);
    CuAssertIntEquals(tc, CLASS_UNDEFINED, perk_list[id].associated_class);
    CuAssertIntEquals(tc, PERK_CATEGORY_UNDEFINED, perk_list[id].perk_category);
    CuAssertIntEquals(tc, 0, perk_list[id].cost);
    CuAssertIntEquals(tc, 0, perk_list[id].max_rank);
  }

  /* NUM_PERKS is a generous upper bound, so unclaimed slots are expected. */
  CuAssertTrue(tc, untouched > 0);
}

void Test_perk_definitions_name_every_category(CuTest *tc)
{
  /* perks.c checks at compile time that the table holds one name per
   * category plus the terminator. The lookup has to stop at the same bound:
   * the last category resolves, and the first id past it reads neither the
   * terminator nor beyond the table. */
  CuAssertStrEquals(tc, "\n", perk_category_names[NUM_PERK_CATEGORIES]);
  CuAssertStrEquals(tc, "Adaptable Tactics",
                    get_perk_category_name(PERK_CATEGORY_ADAPTABLE_TACTICS));
  CuAssertStrEquals(tc, "Unknown Category", get_perk_category_name(NUM_PERK_CATEGORIES));

  /* The first name missing from the table shifted every name after it, so
   * the monk tree was listed as the ranger's Hunter tree. */
  CuAssertStrEquals(tc, "Way of the Four Elements",
                    get_perk_category_name(PERK_CATEGORY_WAY_OF_THE_FOUR_ELEMENTS));
}

static void perk_list_reset_output(struct descriptor_data *descriptor)
{
  if (descriptor->large_outbuf != NULL)
  {
    free(descriptor->large_outbuf->text);
    free(descriptor->large_outbuf);
    descriptor->large_outbuf = NULL;
  }
  descriptor->small_outbuf[0] = '\0';
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufptr = 0;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
}

void Test_perk_definitions_list_every_perk_under_its_tree(CuTest *tc)
{
  struct char_data *ch;
  struct descriptor_data descriptor;
  int saved_perk_system = CONFIG_PERK_SYSTEM;
  char expected[MAX_INPUT_LENGTH];
  int class_id;
  int id;
  int listed = 0;
  int first_unlisted = 0;
  int first_without_header = 0;

  init_perks();
  CONFIG_PERK_SYSTEM = 1;

  /* A player with no colour preference, so the list arrives as plain text. */
  ch = new_char();
  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.character = ch;
  descriptor.pProtocol = ProtocolCreate();
  STATE(&descriptor) = CON_PLAYING;
  ch->desc = &descriptor;

  for (class_id = 0; class_id < NUM_CLASSES; class_id++)
  {
    /* With levels in a single class, a bare "perk" lists that class. */
    memset(ch->player_specials->saved.class_level, 0,
           sizeof(ch->player_specials->saved.class_level));
    CLASS_LEVEL(ch, class_id) = 1;
    perk_list_reset_output(&descriptor);
    do_perk(ch, "", 0, 0);

    for (id = 1; id < NUM_PERKS; id++)
    {
      if (perk_is_untouched(id) || perk_list[id].associated_class != class_id)
        continue;

      /* Names print in a fixed-width column that cuts long ones short. */
      snprintf(expected, sizeof(expected), "%.30s", perk_list[id].name);
      if (strstr(descriptor.output, expected) != NULL)
        listed++;
      else if (first_unlisted == 0)
        first_unlisted = id;

      /* Each tree opens with its name centred in a rule of dashes. The colour
       * reset after the name survives even for a player without colour. */
      snprintf(expected, sizeof(expected), "-%s",
               get_perk_category_name(perk_list[id].perk_category));
      if (strstr(descriptor.output, expected) == NULL && first_without_header == 0)
        first_without_header = id;
    }
  }

  ch->desc = NULL;
  perk_list_reset_output(&descriptor);
  ProtocolDestroy(descriptor.pProtocol);
  free_char(ch);
  CONFIG_PERK_SYSTEM = saved_perk_system;

  /* On failure these name the first perk id the list left out. */
  CuAssertIntEquals(tc, 0, first_unlisted);
  CuAssertIntEquals(tc, 0, first_without_header);
  CuAssertIntEquals(tc, count_defined_perks(), listed);
}
