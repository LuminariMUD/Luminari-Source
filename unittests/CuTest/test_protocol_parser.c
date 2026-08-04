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
#include "../../src/net/protocol.h"
#include "../../src/net/onboarding.h"

#define TEST_CAPTURE_SIZE 65536
#define TEST_FIXTURE_SIZE (MAX_PROTOCOL_BUFFER + 2048)

struct config_data config_info;
struct player_special_data dummy_mob;

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

void TestProtocolParser_NulPaddingIsIgnored(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;
  ssize_t consumed;

  harness_init(tc, &harness);
  fixture_init(&fixture);
  fixture_byte(&fixture, 'a');
  fixture_byte(&fixture, '\0');
  fixture_byte(&fixture, 'b');
  assert_fixture_valid(tc, &fixture);

  consumed = harness_input(&harness, &fixture);

  CuAssertIntEquals(tc, 2, (int)consumed);
  CuAssertStrEquals(tc, "ab", harness.output);

  harness_destroy(&harness);
}

void TestProtocolParser_SplitIacIsRetained(CuTest *tc)
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
  CuAssertIntEquals(tc, ePROTOCOL_INPUT_IAC, harness.descriptor.pProtocol->InputState);

  fixture_init(&second);
  fixture_byte(&second, (unsigned char)WILL);
  fixture_byte(&second, (unsigned char)TELOPT_MSDP);
  fixture_text(&second, "x");
  assert_fixture_valid(tc, &second);

  consumed = harness_input(&harness, &second);

  CuAssertIntEquals(tc, 1, (int)consumed);
  CuAssertStrEquals(tc, "x", harness.output);
  CuAssertIntEquals(tc, ePROTOCOL_INPUT_TEXT, harness.descriptor.pProtocol->InputState);

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
  partial.descriptor.pProtocol->bMSDP = bool_t_true;
  fixture_init(&fixture);
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)SB);
  fixture_byte(&fixture, (unsigned char)TELOPT_MSDP);
  fixture_byte(&fixture, MSDP_VAR);
  fixture_text(&fixture, "REPORT");
  assert_fixture_valid(tc, &fixture);

  CuAssertIntEquals(tc, 0, (int)harness_input(&partial, &fixture));
  CuAssertIntEquals(tc, 1, partial.descriptor.pProtocol->bIACMode);
  CuAssertIntEquals(tc, 8, (int)partial.descriptor.pProtocol->IacLength);
  CuAssertStrEquals(tc, "", partial.output);

  fixture_init(&fixture);
  fixture_byte(&fixture, MSDP_VAL);
  fixture_text(&fixture, "HEALTH");
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)SE);
  assert_fixture_valid(tc, &fixture);

  CuAssertIntEquals(tc, 0, (int)harness_input(&partial, &fixture));
  CuAssertIntEquals(tc, 0, partial.descriptor.pProtocol->bIACMode);
  CuAssertIntEquals(tc, 0, (int)partial.descriptor.pProtocol->IacLength);
  CuAssertIntEquals(tc, 1, partial.descriptor.pProtocol->pVariables[eMSDP_HEALTH]->bReport);
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
  CuAssertIntEquals(tc, ePROTOCOL_INPUT_IAC, harness.descriptor.pProtocol->InputState);
  harness_destroy(&harness);

  harness_init(tc, &harness);
  fixture_init(&fixture);
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)WILL);
  consumed = harness_input(&harness, &fixture);
  CuAssertIntEquals(tc, 0, (int)consumed);
  CuAssertIntEquals(tc, ePROTOCOL_INPUT_NEGOTIATION, harness.descriptor.pProtocol->InputState);
  harness_destroy(&harness);

  harness_init(tc, &harness);
  fixture_init(&fixture);
  fixture_byte(&fixture, 27);
  fixture_byte(&fixture, '[');
  fixture_byte(&fixture, '1');
  consumed = harness_input(&harness, &fixture);
  CuAssertIntEquals(tc, 3, (int)consumed);
  CuAssertIntEquals(tc, 27, (unsigned char)harness.output[0]);

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

void TestProtocolParser_ShortSubnegotiationsAreIgnored(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;
  const unsigned char short_naws[] = {0, 120, 0};

  harness_init(tc, &harness);
  harness.descriptor.pProtocol->bNAWS = bool_t_true;
  harness.descriptor.pProtocol->bCHARSET = bool_t_true;
  harness.descriptor.pProtocol->ScreenWidth = 80;
  harness.descriptor.pProtocol->ScreenHeight = 24;

  fixture_init(&fixture);
  fixture_subnegotiation(&fixture, (unsigned char)TELOPT_NAWS, short_naws, sizeof(short_naws));
  assert_fixture_valid(tc, &fixture);
  CuAssertIntEquals(tc, 0, (int)harness_input(&harness, &fixture));
  CuAssertIntEquals(tc, 80, harness.descriptor.pProtocol->ScreenWidth);
  CuAssertIntEquals(tc, 24, harness.descriptor.pProtocol->ScreenHeight);

  fixture_init(&fixture);
  fixture_subnegotiation(&fixture, (unsigned char)TELOPT_CHARSET, NULL, 0);
  assert_fixture_valid(tc, &fixture);
  CuAssertIntEquals(tc, 0, (int)harness_input(&harness, &fixture));
  CuAssertIntEquals(tc, 0, harness.descriptor.pProtocol->pVariables[eMSDP_UTF_8]->ValueInt);

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

void TestProtocolParser_CreateInitializesAllParserState(CuTest *tc)
{
  protocol_t *protocol;
  int i;

  protocol = ProtocolCreate();
  CuAssertPtrNotNullMsg(tc, "ProtocolCreate returned NULL", protocol);
  CuAssertIntEquals(tc, 0, protocol->WriteOOB);
  CuAssertIntEquals(tc, 0, protocol->bIACMode);
  CuAssertIntEquals(tc, ePROTOCOL_INPUT_TEXT, protocol->InputState);
  CuAssertIntEquals(tc, 0, (int)protocol->IacLength);
  CuAssertIntEquals(tc, 0, protocol->PendingCommand);
  CuAssertIntEquals(tc, 0, protocol->bIacTruncated);
  CuAssertIntEquals(tc, '\0', protocol->CmdBuf[0]);
  CuAssertIntEquals(tc, '\0', protocol->IacBuf[0]);

  for (i = eMSDP_NONE + 1; i < eMSDP_MAX; i++)
    CuAssertPtrNotNullMsg(tc, "MSDP variable allocation was NULL", protocol->pVariables[i]);

  ProtocolDestroy(protocol);
}

void TestProtocolParser_NullAndInvalidMsdpInputsAreSafe(CuTest *tc)
{
  protocol_harness_t harness;
  char raw[] = "x";
  char output[MAX_PROTOCOL_BUFFER + 1] = {'\0'};
  char unicode_buffer[8] = {'\0'};
  char *unicode_pos = unicode_buffer;
  char *null_unicode_pos = NULL;

  ProtocolDestroy(NULL);
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, ProtocolNegotiate(NULL));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, ProtocolNoEcho(NULL, bool_t_true));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, CopyoverSet(NULL, "state"));
  CuAssertStrEquals(tc, "", CopyoverGet(NULL));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, (int)ProtocolInput(NULL, NULL, 1, output));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, (int)ProtocolInput(NULL, raw, 1, NULL));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_INVALID_INPUT, (int)ProtocolInput(NULL, raw, -1, output));
  CuAssertIntEquals(tc, 1, (int)ProtocolInput(NULL, raw, 1, output));
  CuAssertStrEquals(tc, "x", output);
  CuAssertStrEquals(tc, "", ProtocolOutput(NULL, NULL, NULL));

  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, MSDPUpdate(NULL));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, MSDPFlush(NULL, eMSDP_HEALTH));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, MSDPSend(NULL, eMSDP_HEALTH));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, MSDPSendPair(NULL, "HEALTH", "1"));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, MSDPSendList(NULL, "COMMANDS", "LOOK"));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, MSDPSetNumber(NULL, eMSDP_HEALTH, 1));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, MSDPSetString(NULL, eMSDP_TITLE, "title"));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, MSDPSetTable(NULL, eMSDP_ROOM, "room"));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, MSDPSetArray(NULL, eMSDP_ROOM_EXITS, "north"));
  CuAssertPtrEquals(tc, NULL, (void *)MXPCreateTag(NULL, NULL));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, MXPSendTag(NULL, "<VERSION>"));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, SoundSend(NULL, "sound.wav"));
  CuAssertStrEquals(tc, "", ColourRGB(NULL, "F500"));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, UnicodeAdd(NULL, 65));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER, UnicodeAdd(&null_unicode_pos, 65));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_INVALID_INPUT, UnicodeAdd(&unicode_pos, 0x110000));

  harness_init(tc, &harness);
  CuAssertStrEquals(tc, "plain", ProtocolOutput(&harness.descriptor, "plain", NULL));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_INVALID_INPUT,
                    MSDPSetString(&harness.descriptor, (variable_t)eMSDP_NONE, "invalid"));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_INVALID_INPUT,
                    MSDPSetString(&harness.descriptor, (variable_t)eMSDP_MAX, "invalid"));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_INVALID_INPUT,
                    MSDPSetTable(&harness.descriptor, eMSDP_HEALTH, "invalid"));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_INVALID_INPUT,
                    MSDPSetArray(&harness.descriptor, eMSDP_HEALTH, "invalid"));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_INVALID_INPUT,
                    MSDPSetNumber(&harness.descriptor, eMSDP_TITLE, 1));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_NULL_POINTER,
                    MSDPSetString(&harness.descriptor, eMSDP_TITLE, NULL));
  harness_destroy(&harness);
}

void TestProtocolParser_GracefulTruncationKeepsConnectionUsable(CuTest *tc)
{
  protocol_harness_t harness;
  protocol_fixture_t fixture;
  char long_output[MAX_OUTPUT_BUFFER + 32];
  const char *processed;
  ssize_t consumed;
  size_t i;
  int output_length;

  harness_init(tc, &harness);
  fixture_init(&fixture);
  for (i = 0; i < MAX_PROTOCOL_BUFFER + 64; i++)
    fixture_byte(&fixture, 'a');
  assert_fixture_valid(tc, &fixture);

  consumed = harness_input(&harness, &fixture);
  CuAssertIntEquals(tc, MAX_PROTOCOL_BUFFER - 1, (int)consumed);
  CuAssertIntEquals(tc, MAX_PROTOCOL_BUFFER - 1, (int)strlen(harness.output));
  CuAssert(tc, "command truncation was not logged", s_log_count > 0);
  CuAssertIntEquals(tc, ePROTOCOL_INPUT_TEXT, harness.descriptor.pProtocol->InputState);

  harness.output[0] = '\0';
  fixture_init(&fixture);
  fixture_text(&fixture, "ok");
  CuAssertIntEquals(tc, 2, (int)harness_input(&harness, &fixture));
  CuAssertStrEquals(tc, "ok", harness.output);

  harness.output[0] = '\0';
  fixture_init(&fixture);
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)SB);
  fixture_byte(&fixture, (unsigned char)TELOPT_MSDP);
  for (i = 0; i < MAX_PROTOCOL_BUFFER + 32; i++)
    fixture_byte(&fixture, 'A');
  fixture_byte(&fixture, (unsigned char)IAC);
  fixture_byte(&fixture, (unsigned char)SE);
  fixture_byte(&fixture, 'y');
  assert_fixture_valid(tc, &fixture);

  CuAssertIntEquals(tc, 1, (int)harness_input(&harness, &fixture));
  CuAssertStrEquals(tc, "y", harness.output);
  CuAssertIntEquals(tc, ePROTOCOL_INPUT_TEXT, harness.descriptor.pProtocol->InputState);
  CuAssertIntEquals(tc, 0, (int)harness.descriptor.pProtocol->IacLength);

  harness.output[0] = '\0';
  fixture_init(&fixture);
  fixture_byte(&fixture, 27);
  fixture_byte(&fixture, '[');
  fixture_byte(&fixture, '1');
  fixture_byte(&fixture, 'z');
  for (i = 0; i < MAX_MXP_TAG_LENGTH + 32; i++)
    fixture_byte(&fixture, 'M');
  fixture_byte(&fixture, '>');
  fixture_byte(&fixture, 'x');
  assert_fixture_valid(tc, &fixture);

  CuAssertIntEquals(tc, 1, (int)harness_input(&harness, &fixture));
  CuAssertStrEquals(tc, "x", harness.output);

  memset(long_output, 'q', sizeof(long_output) - 1);
  long_output[sizeof(long_output) - 1] = '\0';
  output_length = (int)strlen(long_output);
  processed = ProtocolOutput(&harness.descriptor, long_output, &output_length);
  CuAssertIntEquals(tc, MAX_OUTPUT_BUFFER, output_length);
  CuAssertIntEquals(tc, MAX_OUTPUT_BUFFER, (int)strlen(processed));
  CuAssertIntEquals(tc, 'q', processed[0]);

  harness_destroy(&harness);
}

void TestProtocolParser_MsspPairLengthIsCheckedBeforeAppend(CuTest *tc)
{
  char buffer[MAX_MSSP_BUFFER] = "prefix";
  char original[MAX_MSSP_BUFFER];
  char long_value[MAX_MSSP_PAIR + 32];
  protocol_error_t result;

  memset(long_value, 'L', sizeof(long_value) - 1);
  long_value[sizeof(long_value) - 1] = '\0';
  strlcpy(original, buffer, sizeof(original));

  result = ProtocolTestAppendMSSPPair(buffer, sizeof(buffer), "NAME", long_value);
  CuAssertIntEquals(tc, PROTOCOL_ERROR_BUFFER_FULL, result);
  CuAssertStrEquals(tc, original, buffer);

  result = ProtocolTestAppendMSSPPair(buffer, sizeof(buffer), "PLAYERS", "12");
  CuAssertIntEquals(tc, PROTOCOL_SUCCESS, result);
  CuAssert(tc, "bounded MSSP pair was not appended", strlen(buffer) > strlen(original));
  CuAssertIntEquals(tc, MSSP_VAR, (unsigned char)buffer[strlen(original)]);
}

void TestProtocolParser_UnicodeFallbackHasValidLifetime(CuTest *tc)
{
  protocol_harness_t harness;
  const char *processed;
  int input_length;

  harness_init(tc, &harness);
  input_length = (int)strlen("\t[U65/Z]");
  processed = ProtocolOutput(&harness.descriptor, "\t[U65/Z]", &input_length);
  CuAssertStrEquals(tc, "Z", processed);
  CuAssertIntEquals(tc, 1, input_length);

  harness.descriptor.pProtocol->pVariables[eMSDP_UTF_8]->ValueInt = 1;
  input_length = (int)strlen("\t[U65/Z]");
  processed = ProtocolOutput(&harness.descriptor, "\t[U65/Z]", &input_length);
  CuAssertStrEquals(tc, "A", processed);
  CuAssertIntEquals(tc, 1, input_length);

  harness_destroy(&harness);
}

void TestProtocolParser_OversizedResponsePaths(CuTest *tc)
{
  protocol_harness_t harness;
  char long_value[MAX_VARIABLE_LENGTH + 1];
  char too_long_value[MAX_VARIABLE_LENGTH + 2];
  char valid_tag[MAX_MXP_TAG_LENGTH + 1];
  char long_tag[MAX_MXP_TAG_LENGTH + 2];
  const char *tag_result;
  const char *copyover;
  size_t i;

  harness_init(tc, &harness);
  harness.descriptor.pProtocol->bMSDP = bool_t_true;

  for (i = 0; i < MAX_VARIABLE_LENGTH; i++)
    long_value[i] = 'A';
  long_value[MAX_VARIABLE_LENGTH] = '\0';
  memset(too_long_value, 'B', sizeof(too_long_value) - 1);
  too_long_value[sizeof(too_long_value) - 1] = '\0';

  CuAssertIntEquals(tc, PROTOCOL_SUCCESS,
                    MSDPSetString(&harness.descriptor, eMSDP_TITLE, long_value));
  CuAssertIntEquals(
      tc, MAX_VARIABLE_LENGTH,
      (int)strlen(harness.descriptor.pProtocol->pVariables[eMSDP_TITLE]->pValueString));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_BUFFER_FULL,
                    MSDPSetString(&harness.descriptor, eMSDP_TITLE, too_long_value));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_BUFFER_FULL,
                    MSDPSetTable(&harness.descriptor, eMSDP_ROOM, too_long_value));
  CuAssertIntEquals(tc, PROTOCOL_ERROR_BUFFER_FULL,
                    MSDPSetArray(&harness.descriptor, eMSDP_ROOM_EXITS, too_long_value));

  CuAssertIntEquals(tc, PROTOCOL_ERROR_BUFFER_FULL,
                    MSDPSendList(&harness.descriptor, "REPORTABLE_VARIABLES", long_value));
  CuAssertIntEquals(tc, 0, (int)s_output_capture_len);
  CuAssert(tc, "oversized MSDP list did not log rejection", s_log_count > 0);

  for (i = 0; i < MAX_MXP_TAG_LENGTH; i++)
    valid_tag[i] = 'V';
  valid_tag[MAX_MXP_TAG_LENGTH] = '\0';
  for (i = 0; i < sizeof(long_tag) - 1; i++)
    long_tag[i] = 'M';
  long_tag[sizeof(long_tag) - 1] = '\0';

  harness.descriptor.pProtocol->pVariables[eMSDP_MXP]->ValueInt = 1;
  tag_result = MXPCreateTag(&harness.descriptor, valid_tag);
  CuAssert(tc, "maximum-length MXP tag was not formatted", tag_result != valid_tag);
  CuAssertIntEquals(tc, MAX_MXP_TAG_LENGTH + 8, (int)strlen(tag_result));
  tag_result = MXPCreateTag(&harness.descriptor, long_tag);
  CuAssertPtrEquals(tc, long_tag, (void *)tag_result);
  CuAssertIntEquals(tc, PROTOCOL_ERROR_INVALID_INPUT, MXPSendTag(&harness.descriptor, long_tag));

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

void TestProtocolParser_MudletPackageUsesStableIdentity(CuTest *tc)
{
  const char *version;

  CuAssert(tc, "Mudlet package URL must use the stable LuminariGUI filename",
           strstr(MUDLET_PACKAGE,
                  "\"url\":\"https://luminarimud.com/download/LuminariGUI.mpackage\"") != NULL);
  CuAssert(tc, "Mudlet package URL must not contain a release suffix",
           strstr(MUDLET_PACKAGE, "LuminariGUI-v") == NULL);

  version = strstr(MUDLET_PACKAGE, "\"version\":\"");
  CuAssertPtrNotNullMsg(tc, "Mudlet package version must be a JSON string", (void *)version);
  version += strlen("\"version\":\"");
  CuAssert(tc, "Mudlet package version must not be empty", *version != '\0' && *version != '"');
}

CuSuite *ProtocolParserSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, TestProtocolParser_DoubledIacLiteral);
  SUITE_ADD_TEST(suite, TestProtocolParser_NulPaddingIsIgnored);
  SUITE_ADD_TEST(suite, TestProtocolParser_SplitIacIsRetained);
  SUITE_ADD_TEST(suite, TestProtocolParser_IncompleteAndMalformedSubnegotiations);
  SUITE_ADD_TEST(suite, TestProtocolParser_TruncatedLookaheadSequences);
  SUITE_ADD_TEST(suite, TestProtocolParser_TtypeAndNawsNegotiation);
  SUITE_ADD_TEST(suite, TestProtocolParser_ShortSubnegotiationsAreIgnored);
  SUITE_ADD_TEST(suite, TestProtocolParser_UnsupportedOptionNegotiation);
  SUITE_ADD_TEST(suite, TestProtocolParser_GmcpAndMsdpCanCoexist);
  SUITE_ADD_TEST(suite, TestProtocolParser_WebOnboardingCapability);
  SUITE_ADD_TEST(suite, TestProtocolParser_WebOnboardingActionUsesReservedVariable);
  SUITE_ADD_TEST(suite, TestProtocolParser_CreateInitializesAllParserState);
  SUITE_ADD_TEST(suite, TestProtocolParser_NullAndInvalidMsdpInputsAreSafe);
  SUITE_ADD_TEST(suite, TestProtocolParser_GracefulTruncationKeepsConnectionUsable);
  SUITE_ADD_TEST(suite, TestProtocolParser_MsspPairLengthIsCheckedBeforeAppend);
  SUITE_ADD_TEST(suite, TestProtocolParser_UnicodeFallbackHasValidLifetime);
  SUITE_ADD_TEST(suite, TestProtocolParser_OversizedResponsePaths);
  SUITE_ADD_TEST(suite, TestProtocolParser_MsspResponseIsBounded);
  SUITE_ADD_TEST(suite, TestProtocolParser_SelectedMsdpVariablesCanBeReported);
  SUITE_ADD_TEST(suite, TestProtocolParser_MudletPackageUsesStableIdentity);

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
