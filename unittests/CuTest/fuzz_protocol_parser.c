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
#include "../../src/protocol.h"
#include "../../src/systems/web_client/onboarding.h"

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

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  struct descriptor_data descriptor;
  protocol_t *protocol;
  char descriptor_output[MAX_PROTOCOL_BUFFER + 1];
  char output[MAX_PROTOCOL_BUFFER + 1];
  size_t payload_size;
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
  if ((data[0] & 1U) != 0U && payload_size > 1)
  {
    split = 1 + (data[0] % (payload_size - 1));
    fuzz_protocol_chunk(&descriptor, data + 1, split, output);
    fuzz_protocol_chunk(&descriptor, data + 1 + split, payload_size - split, output);
  }
  else
  {
    fuzz_protocol_chunk(&descriptor, data + 1, payload_size, output);
  }

  ProtocolDestroy(protocol);
  descriptor.pProtocol = NULL;
  return 0;
}
