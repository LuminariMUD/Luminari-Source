/*
 * Focused protocol parser harness for Phase 04 Session 02.
 *
 * Fixtures are synthetic byte arrays only. They must not be replaced with
 * live player transcripts, commands, hosts, credentials, or private captures.
 */

#include "CuTest.h"

#include <arpa/telnet.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/comm.h"
#include "../../src/protocol.h"
#include "../../src/systems/web_client/onboarding.h"

#define TEST_CAPTURE_SIZE 65536
#define TEST_FIXTURE_SIZE 8192

struct config_data config_info;

static unsigned char s_output_capture[TEST_CAPTURE_SIZE];
static size_t s_output_capture_len = 0;
static int s_log_count = 0;

typedef struct protocol_fixture
{
  unsigned char bytes[TEST_FIXTURE_SIZE];
  size_t length;
  int overflow;
} protocol_fixture_t;

typedef struct protocol_harness
{
  struct descriptor_data descriptor;
  char descriptor_output[MAX_PROTOCOL_BUFFER + 1];
  char output[MAX_PROTOCOL_BUFFER + 1];
} protocol_harness_t;

void basic_mud_log(const char *format, ...)
{
  va_list args;

  (void)format;
  s_log_count++;
  va_start(args, format);
  va_end(args);
}

size_t write_to_output(struct descriptor_data *d, const char *txt, ...)
{
  char formatted[TEST_FIXTURE_SIZE];
  va_list args;
  int written;
  size_t length;

  va_start(args, txt);
  written = vsnprintf(formatted, sizeof(formatted), txt, args);
  va_end(args);

  if (written < 0)
    return 0;

  length = (size_t)written;
  if (length >= sizeof(formatted))
    length = sizeof(formatted) - 1;

  if (s_output_capture_len + length < sizeof(s_output_capture))
  {
    memcpy(s_output_capture + s_output_capture_len, formatted, length);
    s_output_capture_len += length;
  }

  if (d != NULL && d->output != NULL)
  {
    size_t current_length = strlen(d->output);
    if (current_length + length < MAX_PROTOCOL_BUFFER)
    {
      memcpy(d->output + current_length, formatted, length);
      d->output[current_length + length] = '\0';
    }
  }

  return length;
}

static void reset_capture(void)
{
  memset(s_output_capture, 0, sizeof(s_output_capture));
  s_output_capture_len = 0;
  s_log_count = 0;
}

static int capture_contains(const unsigned char *needle, size_t needle_length)
{
  size_t i;

  if (needle_length == 0 || needle_length > s_output_capture_len)
    return 0;

  for (i = 0; i + needle_length <= s_output_capture_len; i++)
  {
    if (memcmp(s_output_capture + i, needle, needle_length) == 0)
      return 1;
  }

  return 0;
}

static void fixture_init(protocol_fixture_t *fixture)
{
  memset(fixture, 0, sizeof(*fixture));
}

static void fixture_byte(protocol_fixture_t *fixture, unsigned char value)
{
  if (fixture->length >= sizeof(fixture->bytes))
  {
    fixture->overflow = 1;
    return;
  }

  fixture->bytes[fixture->length++] = value;
}

static void fixture_bytes(protocol_fixture_t *fixture, const unsigned char *values, size_t length)
{
  size_t i;

  for (i = 0; i < length; i++)
    fixture_byte(fixture, values[i]);
}

static void fixture_text(protocol_fixture_t *fixture, const char *text)
{
  fixture_bytes(fixture, (const unsigned char *)text, strlen(text));
}

static void fixture_telnet3(protocol_fixture_t *fixture, unsigned char command,
                            unsigned char option)
{
  fixture_byte(fixture, (unsigned char)IAC);
  fixture_byte(fixture, command);
  fixture_byte(fixture, option);
}

static void fixture_subnegotiation(protocol_fixture_t *fixture, unsigned char option,
                                   const unsigned char *payload, size_t payload_length)
{
  fixture_byte(fixture, (unsigned char)IAC);
  fixture_byte(fixture, (unsigned char)SB);
  fixture_byte(fixture, option);
  fixture_bytes(fixture, payload, payload_length);
  fixture_byte(fixture, (unsigned char)IAC);
  fixture_byte(fixture, (unsigned char)SE);
}

static void harness_init(CuTest *tc, protocol_harness_t *harness)
{
  memset(harness, 0, sizeof(*harness));
  reset_capture();
  memset(&config_info, 0, sizeof(config_info));

  harness->descriptor_output[0] = '\0';
  harness->descriptor.output = harness->descriptor_output;
  harness->descriptor.pProtocol = ProtocolCreate();

  CuAssertPtrNotNullMsg(tc, "ProtocolCreate returned NULL", harness->descriptor.pProtocol);
}

static void harness_destroy(protocol_harness_t *harness)
{
  if (harness->descriptor.pProtocol != NULL)
  {
    ProtocolDestroy(harness->descriptor.pProtocol);
    harness->descriptor.pProtocol = NULL;
  }
}

static ssize_t harness_input(protocol_harness_t *harness, const protocol_fixture_t *fixture)
{
  return ProtocolInput(&harness->descriptor, (char *)fixture->bytes, (int)fixture->length,
                       harness->output);
}

static void assert_fixture_valid(CuTest *tc, const protocol_fixture_t *fixture)
{
  CuAssert(tc, "fixture builder overflowed", fixture->overflow == 0);
}

void TestProtocolParser_DoubledIacLiteral(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;
  ssize_t consumed;

  harness_init(tc, &harness);
  fixture_init(&fixture);
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_text(&fixture, "x");
  assert_fixture_valid(tc, &fixture);

  consumed = harness_input(&harness, &fixture);

  CuAssertIntEquals(tc, 2, (int)consumed);
  CuAssertIntEquals(tc, IAC, (unsigned char)harness.output[0]);
  CuAssertIntEquals(tc, 'x', (unsigned char)harness.output[1]);
  CuAssertIntEquals(tc, 0, (int)s_output_capture_len);

  harness_destroy(&harness);
}

void TestProtocolParser_SplitIacCurrentGap(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t first;
  protocol_fixture_t second;
  ssize_t consumed;

  harness_init(tc, &harness);

  fixture_init(&first);
  fixture_byte(&first, (unsigned char)IAC);
  assert_fixture_valid(tc, &first);

  consumed = harness_input(&harness, &first);
  CuAssertIntEquals(tc, 0, (int)consumed);
  CuAssertStrEquals(tc, "", harness.output);

  fixture_init(&second);
  fixture_byte(&second, (unsigned char)WILL);
  fixture_byte(&second, (unsigned char)TELOPT_MSDP);
  fixture_text(&second, "x");
  assert_fixture_valid(tc, &second);

  consumed = harness_input(&harness, &second);

  CuAssertIntEquals(tc, 3, (int)consumed);
  CuAssertIntEquals(tc, WILL, (unsigned char)harness.output[0]);
  CuAssertIntEquals(tc, TELOPT_MSDP, (unsigned char)harness.output[1]);
  CuAssertIntEquals(tc, 'x', (unsigned char)harness.output[2]);
  CuAssertIntEquals(tc, 0, (int)s_output_capture_len);

  harness_destroy(&harness);
}

void TestProtocolParser_IncompleteAndMalformedSubnegotiations(CuTest *tc)
{
  protocol_harness_t partial;
  protocol_harness_t malformed_msdp;
  protocol_harness_t malformed_gmcp;
  protocol_fixture_t fixture;
  const unsigned char msdp_missing_value[] = {
      MSDP_VAR, 'R', 'E', 'P', 'O', 'R', 'T',
  };
  const unsigned char gmcp_payload[] = {'M', 'S', 'D', 'P', '.', 'H', 'E', 'A', 'L', 'T', 'H'};

  harness_init(tc, &partial);
  fixture_init(&fixture);
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)SB);
  fixture_byte(&fixture, (unsigned char)TELOPT_MSDP);
  fixture_byte(&fixture, MSDP_VAR);
  fixture_text(&fixture, "REPORT");
  assert_fixture_valid(tc, &fixture);

  CuAssertIntEquals(tc, 0, (int)harness_input(&partial, &fixture));
  CuAssertIntEquals(tc, 1, partial.descriptor.pProtocol->bIACMode);
  CuAssertStrEquals(tc, "", partial.output);
  harness_destroy(&partial);

  harness_init(tc, &malformed_msdp);
  malformed_msdp.descriptor.pProtocol->bMSDP = bool_t_true;
  fixture_init(&fixture);
  fixture_subnegotiation(&fixture, (unsigned char)TELOPT_MSDP, msdp_missing_value,
                         sizeof(msdp_missing_value));
  assert_fixture_valid(tc, &fixture);

  CuAssertIntEquals(tc, 0, (int)harness_input(&malformed_msdp, &fixture));
  CuAssertIntEquals(tc, 0, malformed_msdp.descriptor.pProtocol->pVariables[eMSDP_HEALTH]->bReport);
  CuAssertIntEquals(tc, 0, (int)s_output_capture_len);
  harness_destroy(&malformed_msdp);

  harness_init(tc, &malformed_gmcp);
  malformed_gmcp.descriptor.pProtocol->bGMCP = bool_t_true;
  fixture_init(&fixture);
  fixture_subnegotiation(&fixture, (unsigned char)TELOPT_GMCP, gmcp_payload, sizeof(gmcp_payload));
  assert_fixture_valid(tc, &fixture);

  CuAssertIntEquals(tc, 0, (int)harness_input(&malformed_gmcp, &fixture));
  CuAssertIntEquals(tc, 0, (int)s_output_capture_len);
  harness_destroy(&malformed_gmcp);
}

void TestProtocolParser_TruncatedLookaheadSequences(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;
  ssize_t consumed;

  harness_init(tc, &harness);

  fixture_init(&fixture);
  fixture_byte(&fixture, (unsigned char)IAC);
  consumed = harness_input(&harness, &fixture);
  CuAssertIntEquals(tc, 0, (int)consumed);

  fixture_init(&fixture);
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)WILL);
  consumed = harness_input(&harness, &fixture);
  CuAssertIntEquals(tc, 0, (int)consumed);

  fixture_init(&fixture);
  fixture_byte(&fixture, 27);
  fixture_byte(&fixture, '[');
  fixture_byte(&fixture, '1');
  consumed = harness_input(&harness, &fixture);
  CuAssertIntEquals(tc, 3, (int)consumed);

  harness_destroy(&harness);
}

void TestProtocolParser_TtypeAndNawsNegotiation(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;
  const unsigned char ttype_is[] = {
      0, 'x', 't', 'e', 'r', 'm', '-', '2', '5', '6', 'c', 'o', 'l', 'o', 'r',
  };
  const unsigned char request_ttype[] = {(unsigned char)IAC,          (unsigned char)SB,
                                         (unsigned char)TELOPT_TTYPE, SEND,
                                         (unsigned char)IAC,          (unsigned char)SE};
  const unsigned char naws_payload[] = {0, 120, 0, 40};

  harness_init(tc, &harness);

  fixture_init(&fixture);
  fixture_telnet3(&fixture, (unsigned char)WILL, (unsigned char)TELOPT_TTYPE);
  assert_fixture_valid(tc, &fixture);
  harness_input(&harness, &fixture);

  CuAssert(tc, "TTYPE request was not written",
           capture_contains(request_ttype, sizeof(request_ttype)));

  fixture_init(&fixture);
  fixture_subnegotiation(&fixture, (unsigned char)TELOPT_TTYPE, ttype_is, sizeof(ttype_is));
  assert_fixture_valid(tc, &fixture);
  harness_input(&harness, &fixture);

  CuAssertStrEquals(tc, "xterm-256color",
                    harness.descriptor.pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
  CuAssertIntEquals(tc, eYES, harness.descriptor.pProtocol->b256Support);

  fixture_init(&fixture);
  fixture_telnet3(&fixture, (unsigned char)WILL, (unsigned char)TELOPT_NAWS);
  assert_fixture_valid(tc, &fixture);
  harness_input(&harness, &fixture);

  fixture_init(&fixture);
  fixture_subnegotiation(&fixture, (unsigned char)TELOPT_NAWS, naws_payload, sizeof(naws_payload));
  assert_fixture_valid(tc, &fixture);
  harness_input(&harness, &fixture);

  CuAssertIntEquals(tc, 120, harness.descriptor.pProtocol->ScreenWidth);
  CuAssertIntEquals(tc, 40, harness.descriptor.pProtocol->ScreenHeight);

  harness_destroy(&harness);
}

void TestProtocolParser_UnsupportedOptionNegotiation(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;
  const unsigned char expected[] = {(unsigned char)IAC, (unsigned char)DONT, 99,
                                    (unsigned char)IAC, (unsigned char)WONT, 98};

  harness_init(tc, &harness);
  fixture_init(&fixture);
  fixture_telnet3(&fixture, (unsigned char)WILL, 99);
  fixture_telnet3(&fixture, (unsigned char)DO, 98);
  assert_fixture_valid(tc, &fixture);

  harness_input(&harness, &fixture);

  CuAssertIntEquals(tc, (int)sizeof(expected), (int)s_output_capture_len);
  CuAssert(tc, "unsupported options did not produce deterministic rejections",
           memcmp(s_output_capture, expected, sizeof(expected)) == 0);

  harness_destroy(&harness);
}

void TestProtocolParser_GmcpAndMsdpCanCoexist(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;

  harness_init(tc, &harness);
  harness.descriptor.pProtocol->bMSDP = bool_t_true;
  fixture_init(&fixture);
  fixture_telnet3(&fixture, (unsigned char)WILL, (unsigned char)TELOPT_GMCP);
  assert_fixture_valid(tc, &fixture);

  CuAssertIntEquals(tc, 0, (int)harness_input(&harness, &fixture));
  CuAssertIntEquals(tc, bool_t_true, harness.descriptor.pProtocol->bMSDP);
  CuAssertIntEquals(tc, bool_t_true, harness.descriptor.pProtocol->bGMCP);

  harness_destroy(&harness);
}

void TestProtocolParser_WebOnboardingCapability(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;

  harness_init(tc, &harness);
  harness.descriptor.pProtocol->bMSDP = bool_t_true;
  harness.descriptor.web_onboarding_last_state = 42;

  fixture_init(&fixture);
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)SB);
  fixture_byte(&fixture, (unsigned char)TELOPT_MSDP);
  fixture_byte(&fixture, MSDP_VAR);
  fixture_text(&fixture, WEB_ONBOARDING_CAPABILITY_VARIABLE);
  fixture_byte(&fixture, MSDP_VAL);
  fixture_text(&fixture, "1");
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)SE);
  assert_fixture_valid(tc, &fixture);

  CuAssertIntEquals(tc, 0, (int)harness_input(&harness, &fixture));
  CuAssertIntEquals(tc, WEB_ONBOARDING_PROTOCOL_VERSION, harness.descriptor.web_onboarding_version);
  CuAssertIntEquals(tc, -1, harness.descriptor.web_onboarding_last_state);

  harness_destroy(&harness);
}

void TestProtocolParser_WebOnboardingActionUsesReservedVariable(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;
  const char *payload = "{\"v\":2,\"action\":\"cancel-editor\",\"phase\":\"cancel\"}";

  harness_init(tc, &harness);
  harness.descriptor.pProtocol->bMSDP = bool_t_true;

  fixture_init(&fixture);
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)SB);
  fixture_byte(&fixture, (unsigned char)TELOPT_MSDP);
  fixture_byte(&fixture, MSDP_VAR);
  fixture_text(&fixture, WEB_ONBOARDING_ACTION_VARIABLE);
  fixture_byte(&fixture, MSDP_VAL);
  fixture_text(&fixture, payload);
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)SE);
  assert_fixture_valid(tc, &fixture);

  CuAssertIntEquals(tc, 0, (int)harness_input(&harness, &fixture));
  CuAssertTrue(tc, harness.descriptor.web_onboarding_dirty);
  CuAssertIntEquals(tc, (int)strlen(payload), harness.descriptor.web_onboarding_revision);
  CuAssertStrEquals(tc, "", harness.output);

  harness_destroy(&harness);
}

void TestProtocolParser_NullAndInvalidMsdpInputsAreSafe(CuTest *tc)
{
  protocol_harness_t harness;

  MSDPUpdate(NULL);
  MSDPFlush(NULL, eMSDP_HEALTH);
  MSDPSend(NULL, eMSDP_HEALTH);
  MSDPSendPair(NULL, "HEALTH", "1");
  MSDPSendList(NULL, "COMMANDS", "LOOK");
  MSDPSetNumber(NULL, eMSDP_HEALTH, 1);
  MSDPSetString(NULL, eMSDP_TITLE, "title");
  MSDPSetTable(NULL, eMSDP_ROOM, "room");
  MSDPSetArray(NULL, eMSDP_ROOM_EXITS, "north");
  CuAssertPtrEquals(tc, NULL, (void *)MXPCreateTag(NULL, NULL));

  harness_init(tc, &harness);
  MSDPSetString(&harness.descriptor, (variable_t)eMSDP_NONE, "invalid");
  MSDPSetString(&harness.descriptor, (variable_t)eMSDP_MAX, "invalid");
  MSDPSetTable(&harness.descriptor, (variable_t)eMSDP_NONE, "invalid");
  MSDPSetTable(&harness.descriptor, (variable_t)eMSDP_MAX, "invalid");
  MSDPSetArray(&harness.descriptor, (variable_t)eMSDP_NONE, "invalid");
  MSDPSetArray(&harness.descriptor, (variable_t)eMSDP_MAX, "invalid");
  harness_destroy(&harness);
}

void TestProtocolParser_OversizedResponsePaths(CuTest *tc)
{
  protocol_harness_t harness;
  char long_value[MAX_VARIABLE_LENGTH + 1];
  char long_tag[MAX_MXP_TAG_LENGTH + 2];
  const char *tag_result;
  const char *copyover;
  size_t i;

  harness_init(tc, &harness);
  harness.descriptor.pProtocol->bMSDP = bool_t_true;

  for (i = 0; i < MAX_VARIABLE_LENGTH; i++)
    long_value[i] = 'A';
  long_value[MAX_VARIABLE_LENGTH] = '\0';

  MSDPSendList(&harness.descriptor, "REPORTABLE_VARIABLES", long_value);
  CuAssertIntEquals(tc, 0, (int)s_output_capture_len);
  CuAssert(tc, "oversized MSDP list did not log rejection", s_log_count > 0);

  for (i = 0; i < sizeof(long_tag) - 1; i++)
    long_tag[i] = 'M';
  long_tag[sizeof(long_tag) - 1] = '\0';

  harness.descriptor.pProtocol->pVariables[eMSDP_MXP]->ValueInt = 1;
  tag_result = MXPCreateTag(&harness.descriptor, long_tag);
  CuAssertPtrEquals(tc, long_tag, (void *)tag_result);

  harness.descriptor.pProtocol->ScreenWidth = 120;
  harness.descriptor.pProtocol->ScreenHeight = 40;
  harness.descriptor.pProtocol->bTTYPE = bool_t_true;
  harness.descriptor.pProtocol->bNAWS = bool_t_true;
  harness.descriptor.pProtocol->bMSDP = bool_t_true;
  harness.descriptor.pProtocol->bGMCP = bool_t_true;
  harness.descriptor.pProtocol->bMSP = bool_t_true;
  harness.descriptor.pProtocol->bCHARSET = bool_t_true;
  harness.descriptor.pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = 1;
  harness.descriptor.pProtocol->pVariables[eMSDP_UTF_8]->ValueInt = 1;

  copyover = CopyoverGet(&harness.descriptor);
  CuAssertStrEquals(tc, "120/40TNMGSXCHU", copyover);
  CuAssert(tc, "copyover protocol string exceeded bounded buffer", strlen(copyover) < 64);

  harness_destroy(&harness);
}

void TestProtocolParser_MsspResponseIsBounded(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;
  const unsigned char mssp_start[] = {(unsigned char)IAC, (unsigned char)SB,
                                      (unsigned char)TELOPT_MSSP};

  harness_init(tc, &harness);
  fixture_init(&fixture);
  fixture_telnet3(&fixture, (unsigned char)DO, (unsigned char)TELOPT_MSSP);
  assert_fixture_valid(tc, &fixture);

  harness_input(&harness, &fixture);

  CuAssert(tc, "MSSP response was not emitted", s_output_capture_len > 3);
  CuAssert(tc, "MSSP response exceeded source buffer", s_output_capture_len < MAX_MSSP_BUFFER);
  CuAssert(tc, "MSSP subnegotiation start was not emitted",
           capture_contains(mssp_start, sizeof(mssp_start)));
  CuAssertIntEquals(tc, IAC, s_output_capture[s_output_capture_len - 2]);
  CuAssertIntEquals(tc, SE, s_output_capture[s_output_capture_len - 1]);

  harness_destroy(&harness);
}

void TestProtocolParser_SelectedMsdpVariablesCanBeReported(CuTest *tc)
{
  protocol_harness_t harness;
  const unsigned char expected_title[] = {MSDP_VAR, 'T', 'I', 'T', 'L', 'E',
                                          MSDP_VAL, 'S', 'c', 'o', 'u', 't'};
  const unsigned char expected_fortitude[] = {MSDP_VAR, 'F', 'O', 'R',      'T', 'I', 'T',
                                              'U',      'D', 'E', MSDP_VAL, '-', '2'};
  const unsigned char expected_reflex[] = {MSDP_VAR, 'R', 'E', 'F', 'L', 'E', 'X', MSDP_VAL, '7'};
  const unsigned char expected_willpower[] = {MSDP_VAR, 'W', 'I', 'L',      'L', 'P', 'O',
                                              'W',      'E', 'R', MSDP_VAL, '1', '2'};

  harness_init(tc, &harness);
  harness.descriptor.pProtocol->bMSDP = bool_t_true;
  harness.descriptor.pProtocol->pVariables[eMSDP_TITLE]->bReport = bool_t_true;
  harness.descriptor.pProtocol->pVariables[eMSDP_FORTITUDE]->bReport = bool_t_true;
  harness.descriptor.pProtocol->pVariables[eMSDP_REFLEX]->bReport = bool_t_true;
  harness.descriptor.pProtocol->pVariables[eMSDP_WILLPOWER]->bReport = bool_t_true;

  CuAssertStrEquals(tc, "", harness.descriptor.pProtocol->pVariables[eMSDP_TITLE]->pValueString);
  CuAssertIntEquals(tc, 0, harness.descriptor.pProtocol->pVariables[eMSDP_FORTITUDE]->ValueInt);
  CuAssertIntEquals(tc, 0, harness.descriptor.pProtocol->pVariables[eMSDP_REFLEX]->ValueInt);
  CuAssertIntEquals(tc, 0, harness.descriptor.pProtocol->pVariables[eMSDP_WILLPOWER]->ValueInt);

  MSDPSetString(&harness.descriptor, eMSDP_TITLE, "Scout");
  MSDPSetNumber(&harness.descriptor, eMSDP_FORTITUDE, -2);
  MSDPSetNumber(&harness.descriptor, eMSDP_REFLEX, 7);
  MSDPSetNumber(&harness.descriptor, eMSDP_WILLPOWER, 12);
  MSDPUpdate(&harness.descriptor);

  CuAssert(tc, "TITLE was not emitted", capture_contains(expected_title, sizeof(expected_title)));
  CuAssert(tc, "FORTITUDE was not emitted",
           capture_contains(expected_fortitude, sizeof(expected_fortitude)));
  CuAssert(tc, "REFLEX was not emitted",
           capture_contains(expected_reflex, sizeof(expected_reflex)));
  CuAssert(tc, "WILLPOWER was not emitted",
           capture_contains(expected_willpower, sizeof(expected_willpower)));
  CuAssertIntEquals(tc, 0, harness.descriptor.pProtocol->pVariables[eMSDP_TITLE]->bDirty);
  CuAssertIntEquals(tc, 0, harness.descriptor.pProtocol->pVariables[eMSDP_FORTITUDE]->bDirty);
  CuAssertIntEquals(tc, 0, harness.descriptor.pProtocol->pVariables[eMSDP_REFLEX]->bDirty);
  CuAssertIntEquals(tc, 0, harness.descriptor.pProtocol->pVariables[eMSDP_WILLPOWER]->bDirty);

  harness_destroy(&harness);
}

CuSuite *ProtocolParserSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, TestProtocolParser_DoubledIacLiteral);
  SUITE_ADD_TEST(suite, TestProtocolParser_SplitIacCurrentGap);
  SUITE_ADD_TEST(suite, TestProtocolParser_IncompleteAndMalformedSubnegotiations);
  SUITE_ADD_TEST(suite, TestProtocolParser_TruncatedLookaheadSequences);
  SUITE_ADD_TEST(suite, TestProtocolParser_TtypeAndNawsNegotiation);
  SUITE_ADD_TEST(suite, TestProtocolParser_UnsupportedOptionNegotiation);
  SUITE_ADD_TEST(suite, TestProtocolParser_GmcpAndMsdpCanCoexist);
  SUITE_ADD_TEST(suite, TestProtocolParser_WebOnboardingCapability);
  SUITE_ADD_TEST(suite, TestProtocolParser_WebOnboardingActionUsesReservedVariable);
  SUITE_ADD_TEST(suite, TestProtocolParser_NullAndInvalidMsdpInputsAreSafe);
  SUITE_ADD_TEST(suite, TestProtocolParser_OversizedResponsePaths);
  SUITE_ADD_TEST(suite, TestProtocolParser_MsspResponseIsBounded);
  SUITE_ADD_TEST(suite, TestProtocolParser_SelectedMsdpVariablesCanBeReported);

  return suite;
}

int main(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = ProtocolParserSuite();
  int failed;

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);
  printf("%s\n", output->buffer);

  failed = suite->failCount;
  CuStringDelete(output);
  CuSuiteDelete(suite);

  return failed == 0 ? 0 : 1;
}
