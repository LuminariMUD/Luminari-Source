/*
 * Synthetic-input libFuzzer harness for the telnet protocol parser.
 * Do not seed this harness with private player transcripts or credentials.
 */

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/comm.h"
#include "../../src/net/protocol.h"
#include "../../src/net/onboarding.h"

struct config_data config_info;
struct player_special_data dummy_mob;

/*
 * Keep the parser fuzzer isolated from onboarding's world/class dependencies.
 * The deterministic protocol harness links and verifies the production handler.
 */
void web_onboarding_set_capability(struct descriptor_data *d, const char *value)
{
  if (d == NULL || value == NULL)
    return;

  d->web_onboarding_version = WEB_ONBOARDING_PROTOCOL_VERSION;
}

void web_onboarding_set_version_list(struct descriptor_data *d, const char *value)
{
  web_onboarding_set_capability(d, value);
}

void web_onboarding_handle_action(struct descriptor_data *d, const char *payload)
{
  (void)d;
  (void)payload;
}

void basic_mud_log(const char *format, ...)
{
  va_list args;

  (void)format;
  va_start(args, format);
  va_end(args);
}

size_t write_to_output(struct descriptor_data *d, const char *txt, ...)
{
  char formatted[MAX_PROTOCOL_BUFFER + 1];
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

  if (d != NULL && d->output != NULL)
  {
    size_t current_length;
    size_t available;

    current_length = strlen(d->output);
    available = MAX_PROTOCOL_BUFFER - current_length;
    if (length < available)
    {
      memcpy(d->output + current_length, formatted, length);
      d->output[current_length + length] = '\0';
    }
  }

  return length;
}

static void fuzz_protocol_chunk(struct descriptor_data *descriptor, const uint8_t *data,
                                size_t size, char *output)
{
  if (size > INT_MAX)
    return;

  output[0] = '\0';
  ProtocolInput(descriptor, (char *)data, (int)size, output);
}

static void fuzz_protocol_output(struct descriptor_data *descriptor, const uint8_t *data,
                                 size_t size)
{
  char input[MAX_OUTPUT_BUFFER + 2];
  char msdp_value[MAX_VARIABLE_LENGTH + 1];
  char unicode_buffer[8] = {'\0'};
  char *unicode_pos = unicode_buffer;
  size_t input_size;
  size_t msdp_size;
  int output_length;
  int codepoint;

  input_size = size < MAX_OUTPUT_BUFFER ? size : MAX_OUTPUT_BUFFER;
  memcpy(input, data, input_size);
  input[input_size] = '\0';
  input[input_size + 1] = '\0';
  output_length = (int)input_size;
  ProtocolOutput(descriptor, input, &output_length);
  ProtocolOutput(descriptor, input, NULL);

  msdp_size = size < MAX_VARIABLE_LENGTH ? size : MAX_VARIABLE_LENGTH;
  memcpy(msdp_value, data, msdp_size);
  msdp_value[msdp_size] = '\0';
  MSDPSetString(descriptor, eMSDP_TITLE, msdp_value);
  MSDPSetTable(descriptor, eMSDP_ROOM, msdp_value);
  MSDPSetArray(descriptor, eMSDP_ROOM_EXITS, msdp_value);
  MXPCreateTag(descriptor, msdp_value);
  MXPSendTag(descriptor, msdp_value);
  SoundSend(descriptor, msdp_value);
  CopyoverSet(descriptor, msdp_value);
  ColourRGB(descriptor, msdp_value);

  codepoint = 0;
  if (size >= sizeof(codepoint))
    memcpy(&codepoint, data, sizeof(codepoint));
  UnicodeAdd(&unicode_pos, codepoint);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  struct descriptor_data descriptor;
  protocol_t *protocol;
  char descriptor_output[MAX_PROTOCOL_BUFFER + 1];
  char output[MAX_PROTOCOL_BUFFER + 1];
  size_t payload_size;
  size_t chunk_size;
  size_t offset;
  size_t split;

  if (data == NULL || size == 0)
    return 0;

  memset(&descriptor, 0, sizeof(descriptor));
  memset(&config_info, 0, sizeof(config_info));
  descriptor_output[0] = '\0';
  descriptor.output = descriptor_output;
  protocol = ProtocolCreate();
  if (protocol == NULL)
    return 0;
  descriptor.pProtocol = protocol;

  payload_size = size - 1;
  switch (data[0] & 3U)
  {
  case 0:
    fuzz_protocol_chunk(&descriptor, data + 1, payload_size, output);
    break;
  case 1:
    if (payload_size <= 1)
    {
      fuzz_protocol_chunk(&descriptor, data + 1, payload_size, output);
      break;
    }
    split = 1 + (data[0] % (payload_size - 1));
    fuzz_protocol_chunk(&descriptor, data + 1, split, output);
    fuzz_protocol_chunk(&descriptor, data + 1 + split, payload_size - split, output);
    break;
  case 2:
    offset = 0;
    while (offset < payload_size)
    {
      chunk_size = 1 + (data[offset + 1] & 7U);
      if (chunk_size > payload_size - offset)
        chunk_size = payload_size - offset;
      fuzz_protocol_chunk(&descriptor, data + 1 + offset, chunk_size, output);
      offset += chunk_size;
    }
    break;
  default:
    fuzz_protocol_chunk(&descriptor, data + 1, payload_size, output);
    fuzz_protocol_output(&descriptor, data + 1, payload_size);
    break;
  }

  ProtocolInput(&descriptor, NULL, 1, output);
  ProtocolInput(&descriptor, (char *)data, -1, output);
  ProtocolOutput(&descriptor, NULL, NULL);

  ProtocolDestroy(protocol);
  descriptor.pProtocol = NULL;
  return 0;
}
