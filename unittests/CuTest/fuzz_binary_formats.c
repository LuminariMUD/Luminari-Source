/*
 * libFuzzer harness for the durable binary file decoders in core/binary_formats.c.
 *
 * The first input byte selects a decoder (low two bits: 0 board, 1 house
 * control, 2 login log) and, with bit 0x80, re-seals a current-format header's
 * payload size and checksum so that mutations reach the payload decoders. A
 * file that decodes must re-encode, and the re-encoded file must decode to the
 * same content. Seeds are hex text files in fuzz_corpus_binary_formats/, built from
 * the golden fixtures; `make binary-formats-fuzz` converts them.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/binary_formats.h"

#define FUZZ_MAX_BOARD_MESSAGES 300
#define FUZZ_MAX_HOUSES 999

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static void store_u32(unsigned char *bytes, uint32_t value)
{
  bytes[0] = (unsigned char)(value & 0xFF);
  bytes[1] = (unsigned char)((value >> 8) & 0xFF);
  bytes[2] = (unsigned char)((value >> 16) & 0xFF);
  bytes[3] = (unsigned char)(value >> 24);
}

static void reseal(unsigned char *file, size_t size)
{
  uint32_t payload_size;

  if (size < BINARY_FORMAT_HEADER_SIZE || size - BINARY_FORMAT_HEADER_SIZE > UINT32_MAX)
    return;
  payload_size = (uint32_t)(size - BINARY_FORMAT_HEADER_SIZE);
  store_u32(file + 8, payload_size);
  store_u32(file + 12, binary_format_crc32(file + BINARY_FORMAT_HEADER_SIZE, payload_size));
}

static bool same_text(const char *left, const char *right)
{
  return (left == NULL && right == NULL) ||
         (left != NULL && right != NULL && strcmp(left, right) == 0);
}

static void check_board(const unsigned char *file, size_t size)
{
  struct board_file_message *messages = NULL, *again = NULL;
  size_t count = 0, again_count = 0, encoded_size = 0, index;
  unsigned char *encoded = NULL;
  int version = -1;

  if (board_file_decode(file, size, FUZZ_MAX_BOARD_MESSAGES, &messages, &count, &version) !=
      BINARY_FORMAT_OK)
  {
    if (messages != NULL || count != 0)
      abort();
    return;
  }
  if (board_file_encode(messages, count, &encoded, &encoded_size) != BINARY_FORMAT_OK ||
      board_file_decode(encoded, encoded_size, FUZZ_MAX_BOARD_MESSAGES, &again, &again_count,
                        &version) != BINARY_FORMAT_OK ||
      version != BOARD_FILE_VERSION || again_count != count)
    abort();
  for (index = 0; index < count; index++)
    if (messages[index].level != again[index].level ||
        !same_text(messages[index].heading, again[index].heading) ||
        !same_text(messages[index].message, again[index].message))
      abort();
  board_file_free(again, again_count);
  free(encoded);
  board_file_free(messages, count);
}

static void check_house(const unsigned char *file, size_t size)
{
  struct house_file_record *records = NULL, *again = NULL;
  size_t count = 0, again_count = 0, encoded_size = 0, index;
  unsigned char *encoded = NULL;
  int version = -1, guest;

  if (house_file_decode(file, size, FUZZ_MAX_HOUSES, &records, &count, &version) !=
      BINARY_FORMAT_OK)
  {
    if (records != NULL || count != 0)
      abort();
    return;
  }
  if (house_file_encode(records, count, &encoded, &encoded_size) != BINARY_FORMAT_OK ||
      house_file_decode(encoded, encoded_size, FUZZ_MAX_HOUSES, &again, &again_count, &version) !=
          BINARY_FORMAT_OK ||
      version != HOUSE_FILE_VERSION || again_count != count)
    abort();
  for (index = 0; index < count; index++)
  {
    if (records[index].vnum != again[index].vnum || records[index].atrium != again[index].atrium ||
        records[index].exit_num != again[index].exit_num ||
        records[index].mode != again[index].mode ||
        records[index].built_on != again[index].built_on ||
        records[index].owner != again[index].owner ||
        records[index].last_payment != again[index].last_payment ||
        records[index].bitvector != again[index].bitvector ||
        records[index].builtby != again[index].builtby ||
        records[index].num_of_guests != again[index].num_of_guests)
      abort();
    for (guest = 0; guest < records[index].num_of_guests; guest++)
      if (records[index].guests[guest] != again[index].guests[guest])
        abort();
  }
  free(again);
  free(encoded);
  free(records);
}

static void check_last_log(const unsigned char *file, size_t size)
{
  struct last_log_record entry;
  size_t offset;

  for (offset = 0; size - offset >= LAST_LOG_RECORD_SIZE; offset += LAST_LOG_RECORD_SIZE)
  {
    last_log_decode_record(file + offset, &entry);
    if (strlen(entry.hostname) > LAST_LOG_HOSTNAME_SIZE ||
        strlen(entry.username) > LAST_LOG_USERNAME_SIZE)
      abort();
  }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  unsigned char *file;
  size_t file_size;

  if (size == 0)
    return 0;
  file_size = size - 1;
  file = malloc(file_size + 1);
  if (file == NULL)
    return 0;
  memcpy(file, data + 1, file_size);
  if ((data[0] & 0x80) != 0)
    reseal(file, file_size);

  switch (data[0] & 0x03)
  {
  case 1:
    check_house(file, file_size);
    break;
  case 2:
    check_last_log(file, file_size);
    break;
  default:
    check_board(file, file_size);
    break;
  }
  free(file);
  return 0;
}
