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

#include "../../src/character/class.h"
#include "../../src/character/perks.h"
#include "../../src/character/perk_definitions.h"

#include <string.h>

/* Defects that predate this test. Both are content gaps in the perk tables
 * rather than structural problems, and correcting either changes what players
 * can see or buy. The tests below allow at most this many so the tables
 * cannot get worse, and get better freely. */
#define KNOWN_UNCATEGORISED_PERKS 3
#define KNOWN_DANGLING_PREREQUISITES 1
#define KNOWN_UNREACHABLE_PREREQUISITE_RANKS 3

/* A slot that init_perks() reset and no definition function claimed. */
static bool perk_is_untouched(int id)
{
  return perk_list[id].id == PERK_UNDEFINED;
}

void Test_perk_definitions_fill_every_defined_slot_completely(CuTest *tc)
{
  int id;
  int defined = 0;
  int uncategorised = 0;

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

    if (perk_list[id].perk_category <= PERK_CATEGORY_UNDEFINED)
      uncategorised++;
  }

  /* Guard against a future edit that silently stops calling a whole tree. */
  CuAssertTrue(tc, defined > 0);
  CuAssertIntEquals(tc, defined, count_defined_perks());

  /* A perk with no category never appears under any tree, so players cannot
   * find it. Three barbarian perks predate this test and are left alone here
   * because correcting them changes what the perk trees show, which is a
   * content decision rather than a structural one:
   *   PERK_BARBARIAN_RAGE_ENHANCEMENT, PERK_BARBARIAN_EXTENDED_RAGE_1,
   *   PERK_BARBARIAN_TOUGHNESS.
   * This is a ratchet: fixing them is fine, adding a fourth is not. */
  CuAssertTrue(tc, uncategorised <= KNOWN_UNCATEGORISED_PERKS);
}

void Test_perk_definitions_prerequisites_resolve(CuTest *tc)
{
  int id;
  int prereq;
  int dangling = 0;
  int unreachable = 0;

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

    if (perk_is_untouched(prereq))
    {
      /* Pointing at a perk no tree defines makes this one permanently
       * unbuyable. PERK_WIZARD_EXTENDED_SPELL_3 requires
       * PERK_WIZARD_EXTENDED_SPELL_2, which has an id but no definition.
       * Supplying that perk is a content decision, so it is recorded here
       * rather than changed. Ratchet: fixing it is fine, adding another is
       * not. */
      dangling++;
      continue;
    }

    CuAssertTrue(tc, perk_list[id].prerequisite_rank >= 0);

    if (perk_list[id].prerequisite_rank > perk_list[prereq].max_rank)
    {
      /* Requiring more ranks than the prerequisite can reach is the same bug
       * with a subtler symptom: the perk resolves but can never be bought.
       * Three cleric capstones ask for rank 5 of a prerequisite capped at 2
       * or 3: PERK_CLERIC_DOMAIN_FOCUS_3, PERK_CLERIC_DIVINE_SPELL_POWER_3
       * and PERK_CLERIC_GREATER_TURNING. Choosing the intended rank is a
       * content decision, so it is recorded rather than changed. */
      unreachable++;
    }
  }

  CuAssertTrue(tc, dangling <= KNOWN_DANGLING_PREREQUISITES);
  CuAssertTrue(tc, unreachable <= KNOWN_UNREACHABLE_PREREQUISITE_RANKS);
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
