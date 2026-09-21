/* Production-linked tests for wilderness targeted gathering
 * (docs/ongoing-projects/harvesting-system.md): the material pool per sector, the grade-based
 * difficulty, the quality tier from skill, richness, and tools, and name parsing. Command-driven
 * scenarios live in test_gameplay_e2e.c. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/constants.h"
#include "../../src/craft/crafting_new.h"
#include "../../src/wilderness/harvest.h"
#include "../../src/wilderness/resource_system.h"

#include <string.h>

void Test_wilderness_pool_covers_every_sector_and_excludes_unsourced_materials(CuTest *tc)
{
  static const int excluded[] = {CRAFT_MAT_BRASS,      CRAFT_MAT_LINEN,       CRAFT_MAT_DRAGONMETAL,
                                 CRAFT_MAT_DRAGONBONE, CRAFT_MAT_DRAGONBLOOD, CRAFT_MAT_BONE};
  int pool[NUM_CRAFT_MATS];
  bool seen[NUM_CRAFT_MATS] = {false};
  int sector, count, i, material, pool_size = 0, seen_count = 0, bad = 0;

  for (sector = 0; sector < NUM_ROOM_SECTORS; sector++)
  {
    count = wilderness_sector_material_pool(sector, pool, NUM_CRAFT_MATS);
    for (i = 0; i < count; i++)
    {
      if (!wilderness_pool_material(pool[i]) ||
          !can_harvest_resource_in_terrain(wilderness_material_category(pool[i]), sector))
        bad++;
      /* Category, grade, then name order. */
      if (i > 0 &&
          (wilderness_material_category(pool[i - 1]) > wilderness_material_category(pool[i]) ||
           (wilderness_material_category(pool[i - 1]) == wilderness_material_category(pool[i]) &&
            material_grade(pool[i - 1]) > material_grade(pool[i]))))
        bad++;
      seen[pool[i]] = true;
    }
  }
  for (material = 1; material < NUM_CRAFT_MATS; material++)
  {
    if (wilderness_pool_material(material))
      pool_size++;
    if (seen[material])
      seen_count++;
  }
  for (i = 0; i < (int)(sizeof(excluded) / sizeof(excluded[0])); i++)
    if (seen[excluded[i]] || wilderness_pool_material(excluded[i]) ||
        wilderness_material_category(excluded[i]) != -1)
      bad++;

  CuAssertIntEquals(tc, 0, bad);
  CuAssertIntEquals(tc, 31, pool_size);
  CuAssertIntEquals(tc, pool_size, seen_count);
  /* The terrain gate is unchanged: a forest lists the six cloths, five woods, and five hides;
   * a mountain adds the fifteen minerals to cloth and game. */
  count = wilderness_sector_material_pool(SECT_FOREST, pool, NUM_CRAFT_MATS);
  CuAssertIntEquals(tc, 16, count);
  for (i = 0; i < count; i++)
    if (wilderness_material_category(pool[i]) == RESOURCE_MINERALS)
      bad++;
  count = wilderness_sector_material_pool(SECT_MOUNTAIN, pool, NUM_CRAFT_MATS);
  for (i = 0, seen_count = 0; i < count; i++)
    if (wilderness_material_category(pool[i]) == RESOURCE_MINERALS)
      seen_count++;
  CuAssertIntEquals(tc, 15, seen_count);
  CuAssertIntEquals(tc, 0, bad);
  /* Within a category the pool runs by grade, so copper (grade 1) precedes mithril (5). */
  for (i = 0; i < count && pool[i] != CRAFT_MAT_COPPER; i++)
    ;
  CuAssertTrue(tc, i < count);
  for (; i < count && pool[i] != CRAFT_MAT_MITHRIL; i++)
    ;
  CuAssertTrue(tc, i < count);
  CuAssertIntEquals(tc, RESOURCE_MINERALS, wilderness_material_category(CRAFT_MAT_COPPER));
  CuAssertIntEquals(tc, RESOURCE_VEGETATION, wilderness_material_category(CRAFT_MAT_SATIN));
  CuAssertIntEquals(tc, RESOURCE_WOOD, wilderness_material_category(CRAFT_MAT_IRONWOOD));
  CuAssertIntEquals(tc, RESOURCE_GAME, wilderness_material_category(CRAFT_MAT_DRAGONSCALE));
}

void Test_wilderness_material_difficulty_grows_five_per_grade(CuTest *tc)
{
  int base = get_harvest_difficulty(RESOURCE_VEGETATION, 0.45);

  CuAssertIntEquals(tc, base + 5,
                    wilderness_material_difficulty(RESOURCE_VEGETATION, 0.45, CRAFT_MAT_HEMP));
  CuAssertIntEquals(tc, base + 10,
                    wilderness_material_difficulty(RESOURCE_VEGETATION, 0.45, CRAFT_MAT_FLAX));
  CuAssertIntEquals(tc, base + 15,
                    wilderness_material_difficulty(RESOURCE_VEGETATION, 0.45, CRAFT_MAT_WOOL));
  CuAssertIntEquals(tc, base + 20,
                    wilderness_material_difficulty(RESOURCE_VEGETATION, 0.45, CRAFT_MAT_COTTON));
  CuAssertIntEquals(tc, base + 25,
                    wilderness_material_difficulty(RESOURCE_VEGETATION, 0.45, CRAFT_MAT_SATIN));
  base = get_harvest_difficulty(RESOURCE_MINERALS, 0.9);
  CuAssertIntEquals(tc, base + 25,
                    wilderness_material_difficulty(RESOURCE_MINERALS, 0.9, CRAFT_MAT_MITHRIL));
}

void Test_wilderness_quality_tier_takes_the_highest_input(CuTest *tc)
{
  /* Each input alone reaches grade 5; none of them caps another. */
  CuAssertIntEquals(tc, 5, wilderness_quality_tier_from(5, 0.1, 0));
  CuAssertIntEquals(tc, 5, wilderness_quality_tier_from(1, 0.9, 0));
  CuAssertIntEquals(tc, 5, wilderness_quality_tier_from(1, 0.1, 5));
  CuAssertIntEquals(tc, 1, wilderness_quality_tier_from(1, 0.1, 0));
  CuAssertIntEquals(tc, 1, wilderness_quality_tier_from(0, 0.0, 0));
  CuAssertIntEquals(tc, 3, wilderness_quality_tier_from(3, 0.5, 2));
  CuAssertIntEquals(tc, 5, wilderness_quality_tier_from(9, 0.0, 0));
  CuAssertIntEquals(tc, 1, wilderness_richness_tier(0.29));
  CuAssertIntEquals(tc, 2, wilderness_richness_tier(0.3));
  CuAssertIntEquals(tc, 3, wilderness_richness_tier(0.5));
  CuAssertIntEquals(tc, 4, wilderness_richness_tier(0.7));
  CuAssertIntEquals(tc, 5, wilderness_richness_tier(0.9));
  /* A grade-5 material is refused at tier 4 and pulled at tier 5. */
  CuAssertTrue(tc, !wilderness_material_reachable(CRAFT_MAT_SATIN, 4));
  CuAssertTrue(tc, wilderness_material_reachable(CRAFT_MAT_SATIN, 5));
  CuAssertTrue(tc, wilderness_material_reachable(CRAFT_MAT_HEMP, 1));
  CuAssertTrue(tc, !wilderness_material_reachable(CRAFT_MAT_COTTON, 3));
}

void Test_wilderness_parse_material_matches_pool_names(CuTest *tc)
{
  CuAssertIntEquals(tc, CRAFT_MAT_COPPER, wilderness_parse_material("copper"));
  CuAssertIntEquals(tc, CRAFT_MAT_LOW_GRADE_HIDE, wilderness_parse_material("low grade hide"));
  CuAssertIntEquals(tc, CRAFT_MAT_LOW_GRADE_HIDE, wilderness_parse_material("low"));
  CuAssertIntEquals(tc, CRAFT_MAT_SATIN, wilderness_parse_material("satin"));
  CuAssertIntEquals(tc, CRAFT_MAT_STONE, wilderness_parse_material("stone"));
  CuAssertIntEquals(tc, CRAFT_MAT_NONE, wilderness_parse_material("brass"));
  CuAssertIntEquals(tc, CRAFT_MAT_NONE, wilderness_parse_material("dragonbone"));
  CuAssertIntEquals(tc, CRAFT_MAT_NONE, wilderness_parse_material(""));
  CuAssertIntEquals(tc, CRAFT_MAT_NONE, wilderness_parse_material(NULL));
}
