#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"

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
