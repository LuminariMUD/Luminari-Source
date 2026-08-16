#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/mysql.h"
#include "../../src/wilderness/wilderness.h"

#include <string.h>

static void assert_plain_msdp_value(CuTest *tc, struct descriptor_data *descriptor,
                                    variable_t variable, const char *expected)
{
  const char *stored;

  stored = descriptor->pProtocol->pVariables[variable]->pValueString;
  CuAssertStrEquals(tc, expected, stored);
  CuAssertPtrEquals(tc, NULL, memchr(stored, '\t', strlen(stored)));
}

void TestMsdpPlainTextBoundaryStripsAlignmentRoomAndAreaColors(CuTest *tc)
{
  struct descriptor_data descriptor;
  const char *alignment;
  char area_name[] = "\tWthe Vault of Ages\tn";
  char room_name[] = "\tWthe Vault of Ages\tn";

  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNullMsg(tc, "ProtocolCreate returned NULL", descriptor.pProtocol);

  alignment = get_align_by_num(0);
  CuAssertStrEquals(tc, "\tcTrue Neutral\tn", alignment);
  CuAssertIntEquals(tc, PROTOCOL_SUCCESS,
                    set_msdp_plain_text_for_test(&descriptor, eMSDP_ALIGNMENT, alignment));
  CuAssertIntEquals(tc, PROTOCOL_SUCCESS,
                    set_msdp_plain_text_for_test(&descriptor, eMSDP_AREA_NAME, area_name));
  CuAssertIntEquals(tc, PROTOCOL_SUCCESS,
                    set_msdp_plain_text_for_test(&descriptor, eMSDP_ROOM_NAME, room_name));

  assert_plain_msdp_value(tc, &descriptor, eMSDP_ALIGNMENT, "True Neutral");
  assert_plain_msdp_value(tc, &descriptor, eMSDP_AREA_NAME, "the Vault of Ages");
  assert_plain_msdp_value(tc, &descriptor, eMSDP_ROOM_NAME, "the Vault of Ages");
  CuAssertStrEquals(tc, "\tWthe Vault of Ages\tn", area_name);
  CuAssertStrEquals(tc, "\tWthe Vault of Ages\tn", room_name);

  ProtocolDestroy(descriptor.pProtocol);
}

void TestMsdpPlainTextBoundaryKeepsUnchangedValuesClean(CuTest *tc)
{
  struct descriptor_data descriptor;
  MSDP_t *alignment;

  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNullMsg(tc, "ProtocolCreate returned NULL", descriptor.pProtocol);

  CuAssertIntEquals(
      tc, PROTOCOL_SUCCESS,
      set_msdp_plain_text_for_test(&descriptor, eMSDP_ALIGNMENT, "\tcTrue Neutral\tn"));
  alignment = descriptor.pProtocol->pVariables[eMSDP_ALIGNMENT];
  CuAssertTrue(tc, alignment->bDirty);

  alignment->bDirty = bool_t_false;
  CuAssertIntEquals(
      tc, PROTOCOL_SUCCESS,
      set_msdp_plain_text_for_test(&descriptor, eMSDP_ALIGNMENT, "\tcTrue Neutral\tn"));
  CuAssertTrue(tc, !alignment->bDirty);
  CuAssertStrEquals(tc, "True Neutral", alignment->pValueString);

  ProtocolDestroy(descriptor.pProtocol);
}

void TestMsdpMapVariablesGatedWhenNotReported(CuTest *tc)
{
  struct descriptor_data descriptor;

  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNullMsg(tc, "ProtocolCreate returned NULL", descriptor.pProtocol);

  /* Initially bReport is false for all variables */
  CuAssertTrue(tc, !descriptor.pProtocol->pVariables[eMSDP_MINIMAP]->bReport);
  CuAssertTrue(tc, !descriptor.pProtocol->pVariables[eMSDP_GRAPHIC_MAP]->bReport);
  CuAssertTrue(tc, !descriptor.pProtocol->pVariables[eMSDP_WILDERNESS_GRAPHIC_MAP]->bReport);

  ProtocolDestroy(descriptor.pProtocol);
}

void TestWildernessMapCacheAndSpatialEvaluation(CuTest *tc)
{
  struct region_data test_region;
  struct vertex vertices[5];
  struct region_list *regions;
  struct path_data test_path;
  struct vertex path_vertices[3];
  struct path_list *paths;
  struct region_data *old_region_table;
  region_rnum old_top_region;
  struct path_data *old_path_table;
  path_rnum old_top_path;

  /* Invalidate wilderness cache */
  wild_map_cache_invalidate();

  /* Set up a test region in memory */
  memset(&test_region, 0, sizeof(test_region));
  test_region.vnum = 99999;
  test_region.zone = 0;
  test_region.region_type = REGION_SECTOR;
  test_region.region_props = SECT_FOREST;
  test_region.num_vertices = 5;

  vertices[0].x = 10;
  vertices[0].y = 10;
  vertices[1].x = 20;
  vertices[1].y = 10;
  vertices[2].x = 20;
  vertices[2].y = 20;
  vertices[3].x = 10;
  vertices[3].y = 20;
  vertices[4].x = 10;
  vertices[4].y = 10;
  test_region.vertices = vertices;

  /* Temporary point region table */
  old_region_table = region_table;
  old_top_region = top_of_region_table;

  region_table = &test_region;
  top_of_region_table = 0;

  /* Test point inside (15, 15) */
  regions = get_enclosing_regions(0, 15, 15);
  CuAssertPtrNotNull(tc, regions);
  CuAssertIntEquals(tc, 0, regions->rnum);
  CuAssertIntEquals(tc, REGION_POS_CENTER, regions->pos);
  free_region_list(regions);

  /* Test point outside (5, 5) */
  regions = get_enclosing_regions(0, 5, 5);
  CuAssertPtrEquals(tc, NULL, regions);

  /* Restore region table */
  region_table = old_region_table;
  top_of_region_table = old_top_region;

  /* Test path in memory */
  memset(&test_path, 0, sizeof(test_path));
  test_path.vnum = 99999;
  test_path.zone = 0;
  test_path.path_type = PATH_ROAD;
  test_path.num_vertices = 3;

  path_vertices[0].x = 10;
  path_vertices[0].y = 0;
  path_vertices[1].x = 10;
  path_vertices[1].y = 10;
  path_vertices[2].x = 10;
  path_vertices[2].y = 20;
  test_path.vertices = path_vertices;

  old_path_table = path_table;
  old_top_path = top_of_path_table;

  path_table = &test_path;
  top_of_path_table = 0;

  /* Test point on vertical path (10, 5) */
  paths = get_enclosing_paths(0, 10, 5);
  CuAssertPtrNotNull(tc, paths);
  CuAssertIntEquals(tc, 0, paths->rnum);
  CuAssertIntEquals(tc, GLYPH_TYPE_PATH_NS, paths->glyph_type);
  free_path_list(paths);

  /* Test point off path (15, 5) */
  paths = get_enclosing_paths(0, 15, 5);
  CuAssertPtrEquals(tc, NULL, paths);

  /* Restore path table */
  path_table = old_path_table;
  top_of_path_table = old_top_path;
}

void TestMsdpMapStateChangesDirtyOnlyReportedMaps(CuTest *tc)
{
  struct descriptor_data descriptor;
  struct descriptor_data *old_descriptor_list;

  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNullMsg(tc, "ProtocolCreate returned NULL", descriptor.pProtocol);

  descriptor.pProtocol->pVariables[eMSDP_MINIMAP]->bDirty = false;
  descriptor.pProtocol->pVariables[eMSDP_GRAPHIC_MAP]->bDirty = false;
  descriptor.pProtocol->pVariables[eMSDP_WILDERNESS_GRAPHIC_MAP]->bDirty = false;
  descriptor.pProtocol->pVariables[eMSDP_GRAPHIC_MAP]->bReport = true;

  old_descriptor_list = descriptor_list;
  descriptor.next = descriptor_list;
  descriptor_list = &descriptor;

  msdp_mark_map_state_changed();

  CuAssertTrue(tc, !descriptor.pProtocol->pVariables[eMSDP_MINIMAP]->bDirty);
  CuAssertTrue(tc, descriptor.pProtocol->pVariables[eMSDP_GRAPHIC_MAP]->bDirty);
  CuAssertTrue(tc, !descriptor.pProtocol->pVariables[eMSDP_WILDERNESS_GRAPHIC_MAP]->bDirty);

  descriptor_list = old_descriptor_list;
  ProtocolDestroy(descriptor.pProtocol);
}
