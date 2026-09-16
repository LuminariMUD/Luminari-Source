/**
 * @file binary_formats.c
 * Portable encoders and decoders for the server's durable binary files.
 *
 * Integers are assembled from bytes with shifts, never copied from memory, so
 * the result does not depend on the host's byte order, type sizes, structure
 * padding, or the signedness of char. See binary_formats.h for the contract
 * and docs/systems/BINARY_FILE_FORMATS.md for the specification.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "binary_formats.h"

/* Legacy layouts: offsets measured with offsetof() on the x86-64 server that
 * wrote them (LP64, little-endian). They are frozen. */
#define LEGACY_BOARD_RECORD_SIZE 32
#define LEGACY_BOARD_LEVEL_OFFSET 16
#define LEGACY_BOARD_HEADING_SIZE_OFFSET 20
#define LEGACY_BOARD_MESSAGE_SIZE_OFFSET 24

#define LEGACY_HOUSE_RECORD_SIZE 912
#define LEGACY_HOUSE_VNUM_OFFSET 0
#define LEGACY_HOUSE_ATRIUM_OFFSET 4
#define LEGACY_HOUSE_EXIT_OFFSET 8
#define LEGACY_HOUSE_BUILT_ON_OFFSET 16
#define LEGACY_HOUSE_MODE_OFFSET 24
#define LEGACY_HOUSE_OWNER_OFFSET 32
#define LEGACY_HOUSE_GUEST_COUNT_OFFSET 40
#define LEGACY_HOUSE_GUESTS_OFFSET 48
#define LEGACY_HOUSE_LAST_PAYMENT_OFFSET 840
#define LEGACY_HOUSE_BITVECTOR_OFFSET 848
#define LEGACY_HOUSE_BUILTBY_OFFSET 856

#define LEGACY_LAST_CLOSE_TYPE_OFFSET 0
#define LEGACY_LAST_HOSTNAME_OFFSET 4
#define LEGACY_LAST_USERNAME_OFFSET 260
#define LEGACY_LAST_LOGIN_TIME_OFFSET 280
#define LEGACY_LAST_CLOSE_TIME_OFFSET 288
#define LEGACY_LAST_IDNUM_OFFSET 296
#define LEGACY_LAST_PUNIQUE_OFFSET 300

/* Current payload layouts. A board post is a level followed by two strings;
 * a string is a u32 size that counts the terminator (0 means absent) followed
 * by that many bytes. A house record is a fixed prefix followed by its guests. */
#define BOARD_MESSAGE_MIN_SIZE 12
#define HOUSE_RECORD_PREFIX_SIZE 58
#define HOUSE_VNUM_OFFSET 0
#define HOUSE_ATRIUM_OFFSET 4
#define HOUSE_EXIT_OFFSET 8
#define HOUSE_MODE_OFFSET 10
#define HOUSE_BUILT_ON_OFFSET 14
#define HOUSE_OWNER_OFFSET 22
#define HOUSE_LAST_PAYMENT_OFFSET 30
#define HOUSE_BITVECTOR_OFFSET 38
#define HOUSE_BUILTBY_OFFSET 46
#define HOUSE_GUEST_COUNT_OFFSET 54
#define HOUSE_GUEST_SIZE 8

struct byte_reader
{
  const unsigned char *data;
  size_t size;
  size_t offset;
};

struct byte_writer
{
  unsigned char *data;
  size_t size;
  size_t capacity;
  bool failed;
};

static const uint32_t crc32_nibble_table[16] = {
    0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
    0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C};

const char *binary_format_status_name(enum binary_format_status status)
{
  switch (status)
  {
  case BINARY_FORMAT_OK:
    return "ok";
  case BINARY_FORMAT_TRUNCATED:
    return "truncated";
  case BINARY_FORMAT_TRAILING_DATA:
    return "unexpected trailing data";
  case BINARY_FORMAT_BAD_MAGIC:
    return "wrong magic bytes";
  case BINARY_FORMAT_UNSUPPORTED_VERSION:
    return "unsupported format version";
  case BINARY_FORMAT_BAD_BYTE_ORDER:
    return "wrong byte order";
  case BINARY_FORMAT_CHECKSUM_MISMATCH:
    return "checksum mismatch";
  case BINARY_FORMAT_LIMIT_EXCEEDED:
    return "count or size over its limit";
  case BINARY_FORMAT_BAD_STRING:
    return "malformed string";
  case BINARY_FORMAT_NO_MEMORY:
    return "out of memory";
  }
  return "unknown error";
}

uint32_t binary_format_crc32(const unsigned char *data, size_t size)
{
  uint32_t crc = UINT32_C(0xFFFFFFFF);
  size_t index;

  for (index = 0; index < size; index++)
  {
    crc = (crc >> 4) ^ crc32_nibble_table[(crc ^ data[index]) & 0x0F];
    crc = (crc >> 4) ^ crc32_nibble_table[(crc ^ ((unsigned)data[index] >> 4)) & 0x0F];
  }
  return ~crc;
}

static uint16_t load_u16(const unsigned char *bytes)
{
  return (uint16_t)((unsigned)bytes[0] | (unsigned)bytes[1] << 8);
}

static uint32_t load_u32(const unsigned char *bytes)
{
  return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 |
         (uint32_t)bytes[3] << 24;
}

static uint64_t load_u64(const unsigned char *bytes)
{
  return (uint64_t)load_u32(bytes) | (uint64_t)load_u32(bytes + 4) << 32;
}

/* Two's-complement reinterpretation without implementation-defined casts. */
static int16_t to_i16(uint16_t value)
{
  int32_t widened = value;

  if (widened > INT16_MAX)
    widened -= 65536;
  return (int16_t)widened;
}

static int32_t to_i32(uint32_t value)
{
  return value <= INT32_MAX ? (int32_t)value : (int32_t)(value - UINT32_C(0x80000000)) + INT32_MIN;
}

static int64_t to_i64(uint64_t value)
{
  return value <= INT64_MAX ? (int64_t)value
                            : (int64_t)(value - UINT64_C(0x8000000000000000)) + INT64_MIN;
}

static void store_u16(unsigned char *bytes, uint16_t value)
{
  bytes[0] = (unsigned char)(value & 0xFF);
  bytes[1] = (unsigned char)(value >> 8);
}

static void store_u32(unsigned char *bytes, uint32_t value)
{
  bytes[0] = (unsigned char)(value & 0xFF);
  bytes[1] = (unsigned char)((value >> 8) & 0xFF);
  bytes[2] = (unsigned char)((value >> 16) & 0xFF);
  bytes[3] = (unsigned char)(value >> 24);
}

static void store_u64(unsigned char *bytes, uint64_t value)
{
  store_u32(bytes, (uint32_t)(value & UINT32_MAX));
  store_u32(bytes + 4, (uint32_t)(value >> 32));
}

/** Returns the next length bytes, or NULL when fewer remain. */
static const unsigned char *take(struct byte_reader *reader, size_t length)
{
  const unsigned char *bytes;

  if (reader->size - reader->offset < length)
    return NULL;
  bytes = reader->data + reader->offset;
  reader->offset += length;
  return bytes;
}

/** Copies a string of stored_size bytes (terminator included, 0 = absent). */
static enum binary_format_status take_string(struct byte_reader *reader, size_t stored_size,
                                             size_t max_size, bool allow_absent, char **text)
{
  const unsigned char *bytes;

  *text = NULL;
  if (stored_size == 0)
    return allow_absent ? BINARY_FORMAT_OK : BINARY_FORMAT_BAD_STRING;
  if (stored_size > max_size)
    return BINARY_FORMAT_LIMIT_EXCEEDED;
  bytes = take(reader, stored_size);
  if (!bytes)
    return BINARY_FORMAT_TRUNCATED;
  if (bytes[stored_size - 1] != '\0' || memchr(bytes, '\0', stored_size - 1) != NULL)
    return BINARY_FORMAT_BAD_STRING;
  *text = malloc(stored_size);
  if (!*text)
    return BINARY_FORMAT_NO_MEMORY;
  memcpy(*text, bytes, stored_size);
  return BINARY_FORMAT_OK;
}

static void put(struct byte_writer *writer, const unsigned char *bytes, size_t length)
{
  unsigned char *grown;
  size_t capacity;

  if (writer->failed || length == 0)
    return;
  if (writer->capacity - writer->size < length)
  {
    capacity = writer->capacity ? writer->capacity : 256;
    while (capacity - writer->size < length)
    {
      if (capacity > SIZE_MAX / 2)
      {
        writer->failed = true;
        return;
      }
      capacity *= 2;
    }
    grown = realloc(writer->data, capacity);
    if (!grown)
    {
      writer->failed = true;
      return;
    }
    writer->data = grown;
    writer->capacity = capacity;
  }
  memcpy(writer->data + writer->size, bytes, length);
  writer->size += length;
}

static void put_u32(struct byte_writer *writer, uint32_t value)
{
  unsigned char bytes[4];

  store_u32(bytes, value);
  put(writer, bytes, sizeof(bytes));
}

static enum binary_format_status put_string(struct byte_writer *writer, const char *text,
                                            size_t max_size)
{
  size_t stored_size;

  if (!text)
  {
    put_u32(writer, 0);
    return BINARY_FORMAT_OK;
  }
  stored_size = strlen(text) + 1;
  if (stored_size > max_size)
    return BINARY_FORMAT_LIMIT_EXCEEDED;
  put_u32(writer, (uint32_t)stored_size);
  put(writer, (const unsigned char *)text, stored_size);
  return BINARY_FORMAT_OK;
}

/** Starts a current file: space for the envelope, patched by finish_envelope(). */
static void start_envelope(struct byte_writer *writer)
{
  static const unsigned char header[BINARY_FORMAT_HEADER_SIZE];

  put(writer, header, sizeof(header));
}

static enum binary_format_status finish_envelope(struct byte_writer *writer, const char *magic,
                                                 uint16_t version, unsigned char **data,
                                                 size_t *size)
{
  size_t payload_size;

  if (writer->failed)
  {
    free(writer->data);
    return BINARY_FORMAT_NO_MEMORY;
  }
  payload_size = writer->size - BINARY_FORMAT_HEADER_SIZE;
  if (payload_size > UINT32_MAX)
  {
    free(writer->data);
    return BINARY_FORMAT_LIMIT_EXCEEDED;
  }
  memcpy(writer->data, magic, BINARY_FORMAT_MAGIC_SIZE);
  store_u16(writer->data + 4, version);
  store_u16(writer->data + 6, BINARY_FORMAT_BYTE_ORDER_MARK);
  store_u32(writer->data + 8, (uint32_t)payload_size);
  store_u32(writer->data + 12,
            binary_format_crc32(writer->data + BINARY_FORMAT_HEADER_SIZE, payload_size));
  *data = writer->data;
  *size = writer->size;
  return BINARY_FORMAT_OK;
}

/** Validates the envelope of a current file. */
static enum binary_format_status open_envelope(const unsigned char *data, size_t size,
                                               const char *magic, uint16_t version,
                                               struct byte_reader *payload)
{
  uint32_t payload_size;

  if (size < BINARY_FORMAT_HEADER_SIZE)
    return BINARY_FORMAT_TRUNCATED;
  if (memcmp(data, magic, BINARY_FORMAT_MAGIC_SIZE) != 0)
    return BINARY_FORMAT_BAD_MAGIC;
  if (load_u16(data + 6) != BINARY_FORMAT_BYTE_ORDER_MARK)
    return BINARY_FORMAT_BAD_BYTE_ORDER;
  if (load_u16(data + 4) != version)
    return BINARY_FORMAT_UNSUPPORTED_VERSION;
  payload_size = load_u32(data + 8);
  if (payload_size > size - BINARY_FORMAT_HEADER_SIZE)
    return BINARY_FORMAT_TRUNCATED;
  if (payload_size < size - BINARY_FORMAT_HEADER_SIZE)
    return BINARY_FORMAT_TRAILING_DATA;
  if (load_u32(data + 12) != binary_format_crc32(data + BINARY_FORMAT_HEADER_SIZE, payload_size))
    return BINARY_FORMAT_CHECKSUM_MISMATCH;
  payload->data = data + BINARY_FORMAT_HEADER_SIZE;
  payload->size = payload_size;
  payload->offset = 0;
  return BINARY_FORMAT_OK;
}

/** A current file has its magic or, when that is damaged, the byte-order mark,
 * so a damaged current file is rejected rather than read as a legacy layout.
 * In a legacy file, offset 6 holds the high half of the first board slot
 * number or house atrium vnum, which real files keep far below 0xFEFF0000. */
static bool is_current_file(const unsigned char *data, size_t size, const char *magic)
{
  if (size >= BINARY_FORMAT_MAGIC_SIZE && memcmp(data, magic, BINARY_FORMAT_MAGIC_SIZE) == 0)
    return true;
  return size >= BINARY_FORMAT_HEADER_SIZE && load_u16(data + 6) == BINARY_FORMAT_BYTE_ORDER_MARK;
}

/** Multiplies with saturation so a limit computation cannot wrap. */
static size_t saturating_size(size_t fixed, size_t count, size_t each)
{
  if (each != 0 && count > (SIZE_MAX - fixed) / each)
    return SIZE_MAX;
  return fixed + count * each;
}

size_t board_file_max_size(size_t max_messages)
{
  /* The legacy index record is the larger per-post overhead. */
  return saturating_size(BINARY_FORMAT_HEADER_SIZE + 4, max_messages,
                         LEGACY_BOARD_RECORD_SIZE + BOARD_FILE_MAX_HEADING_SIZE +
                             BOARD_FILE_MAX_MESSAGE_SIZE);
}

void board_file_free(struct board_file_message *messages, size_t count)
{
  size_t index;

  if (!messages)
    return;
  for (index = 0; index < count; index++)
  {
    free(messages[index].heading);
    free(messages[index].message);
  }
  free(messages);
}

static enum binary_format_status decode_board_messages(struct byte_reader *reader, bool legacy,
                                                       size_t stored_count,
                                                       struct board_file_message **messages,
                                                       size_t *count)
{
  struct board_file_message *decoded;
  enum binary_format_status status = BINARY_FORMAT_OK;
  const unsigned char *bytes;
  size_t index, heading_size, message_size;
  int32_t legacy_heading_size, legacy_message_size;

  if (stored_count == 0)
    return reader->offset == reader->size ? BINARY_FORMAT_OK : BINARY_FORMAT_TRAILING_DATA;
  decoded = calloc(stored_count, sizeof(*decoded));
  if (!decoded)
    return BINARY_FORMAT_NO_MEMORY;

  for (index = 0; index < stored_count && status == BINARY_FORMAT_OK; index++)
  {
    if (legacy)
    {
      bytes = take(reader, LEGACY_BOARD_RECORD_SIZE);
      if (!bytes)
      {
        status = BINARY_FORMAT_TRUNCATED;
        break;
      }
      /* slot_num and the heading pointer were runtime values; ignore them. */
      decoded[index].level = to_i32(load_u32(bytes + LEGACY_BOARD_LEVEL_OFFSET));
      legacy_heading_size = to_i32(load_u32(bytes + LEGACY_BOARD_HEADING_SIZE_OFFSET));
      legacy_message_size = to_i32(load_u32(bytes + LEGACY_BOARD_MESSAGE_SIZE_OFFSET));
      if (legacy_heading_size < 0 || legacy_message_size < 0)
      {
        status = BINARY_FORMAT_BAD_STRING;
        break;
      }
      status = take_string(reader, (size_t)legacy_heading_size, BOARD_FILE_MAX_HEADING_SIZE, false,
                           &decoded[index].heading);
      if (status == BINARY_FORMAT_OK)
        status = take_string(reader, (size_t)legacy_message_size, BOARD_FILE_MAX_MESSAGE_SIZE, true,
                             &decoded[index].message);
      continue;
    }

    bytes = take(reader, 8);
    if (!bytes)
    {
      status = BINARY_FORMAT_TRUNCATED;
      break;
    }
    decoded[index].level = to_i32(load_u32(bytes));
    heading_size = load_u32(bytes + 4);
    status = take_string(reader, heading_size, BOARD_FILE_MAX_HEADING_SIZE, true,
                         &decoded[index].heading);
    if (status != BINARY_FORMAT_OK)
      break;
    bytes = take(reader, 4);
    if (!bytes)
    {
      status = BINARY_FORMAT_TRUNCATED;
      break;
    }
    message_size = load_u32(bytes);
    status = take_string(reader, message_size, BOARD_FILE_MAX_MESSAGE_SIZE, true,
                         &decoded[index].message);
  }

  if (status == BINARY_FORMAT_OK && reader->offset != reader->size)
    status = BINARY_FORMAT_TRAILING_DATA;
  if (status != BINARY_FORMAT_OK)
  {
    board_file_free(decoded, stored_count);
    return status;
  }
  *messages = decoded;
  *count = stored_count;
  return BINARY_FORMAT_OK;
}

enum binary_format_status board_file_decode(const unsigned char *data, size_t size,
                                            size_t max_messages,
                                            struct board_file_message **messages, size_t *count,
                                            int *version)
{
  struct byte_reader reader = {data, size, 0};
  enum binary_format_status status;
  const unsigned char *bytes;
  uint32_t stored_count;
  bool legacy;

  *messages = NULL;
  *count = 0;
  legacy = !is_current_file(data, size, BOARD_FILE_MAGIC);
  if (legacy)
  {
    /* The legacy loader treated an empty file as an empty board. */
    if (size == 0)
    {
      *version = BINARY_FORMAT_LEGACY;
      return BINARY_FORMAT_OK;
    }
  }
  else
  {
    status = open_envelope(data, size, BOARD_FILE_MAGIC, BOARD_FILE_VERSION, &reader);
    if (status != BINARY_FORMAT_OK)
      return status;
  }

  bytes = take(&reader, 4);
  if (!bytes)
    return BINARY_FORMAT_TRUNCATED;
  stored_count = load_u32(bytes);
  if ((legacy && to_i32(stored_count) < 0) || stored_count > max_messages)
    return BINARY_FORMAT_LIMIT_EXCEEDED;
  /* Refuse counts the remaining bytes cannot hold before allocating. */
  if (stored_count >
      (reader.size - reader.offset) / (legacy ? LEGACY_BOARD_RECORD_SIZE : BOARD_MESSAGE_MIN_SIZE))
    return BINARY_FORMAT_TRUNCATED;

  status = decode_board_messages(&reader, legacy, stored_count, messages, count);
  if (status == BINARY_FORMAT_OK)
    *version = legacy ? BINARY_FORMAT_LEGACY : BOARD_FILE_VERSION;
  return status;
}

enum binary_format_status board_file_encode(const struct board_file_message *messages, size_t count,
                                            unsigned char **data, size_t *size)
{
  struct byte_writer writer = {NULL, 0, 0, false};
  enum binary_format_status status = BINARY_FORMAT_OK;
  size_t index;

  *data = NULL;
  *size = 0;
  if (count > UINT32_MAX)
    return BINARY_FORMAT_LIMIT_EXCEEDED;

  start_envelope(&writer);
  put_u32(&writer, (uint32_t)count);
  for (index = 0; index < count && status == BINARY_FORMAT_OK; index++)
  {
    put_u32(&writer, (uint32_t)messages[index].level);
    status = put_string(&writer, messages[index].heading, BOARD_FILE_MAX_HEADING_SIZE);
    if (status == BINARY_FORMAT_OK)
      status = put_string(&writer, messages[index].message, BOARD_FILE_MAX_MESSAGE_SIZE);
  }
  if (status != BINARY_FORMAT_OK)
  {
    free(writer.data);
    return status;
  }
  return finish_envelope(&writer, BOARD_FILE_MAGIC, BOARD_FILE_VERSION, data, size);
}

size_t house_file_max_size(size_t max_houses)
{
  /* The legacy record, with every guest slot, is the larger layout. */
  return saturating_size(BINARY_FORMAT_HEADER_SIZE + 4, max_houses, LEGACY_HOUSE_RECORD_SIZE);
}

static void decode_legacy_house(const unsigned char *bytes, struct house_file_record *record)
{
  int32_t guest;

  record->vnum = load_u32(bytes + LEGACY_HOUSE_VNUM_OFFSET);
  record->atrium = load_u32(bytes + LEGACY_HOUSE_ATRIUM_OFFSET);
  record->exit_num = to_i16(load_u16(bytes + LEGACY_HOUSE_EXIT_OFFSET));
  record->built_on = to_i64(load_u64(bytes + LEGACY_HOUSE_BUILT_ON_OFFSET));
  record->mode = to_i32(load_u32(bytes + LEGACY_HOUSE_MODE_OFFSET));
  record->owner = to_i64(load_u64(bytes + LEGACY_HOUSE_OWNER_OFFSET));
  record->last_payment = to_i64(load_u64(bytes + LEGACY_HOUSE_LAST_PAYMENT_OFFSET));
  record->bitvector = to_i64(load_u64(bytes + LEGACY_HOUSE_BITVECTOR_OFFSET));
  record->builtby = to_i64(load_u64(bytes + LEGACY_HOUSE_BUILTBY_OFFSET));
  record->num_of_guests = to_i32(load_u32(bytes + LEGACY_HOUSE_GUEST_COUNT_OFFSET));
  /* Slots past the count held stale values; only counted guests survive. */
  for (guest = 0; guest < record->num_of_guests && guest < HOUSE_FILE_MAX_GUESTS; guest++)
    record->guests[guest] =
        to_i64(load_u64(bytes + LEGACY_HOUSE_GUESTS_OFFSET + (size_t)guest * HOUSE_GUEST_SIZE));
}

enum binary_format_status house_file_decode(const unsigned char *data, size_t size,
                                            size_t max_houses, struct house_file_record **records,
                                            size_t *count, int *version)
{
  struct house_file_record *decoded;
  struct byte_reader reader = {data, size, 0};
  enum binary_format_status status = BINARY_FORMAT_OK;
  const unsigned char *bytes;
  size_t stored_count, index;
  uint32_t guest_count, guest;
  bool legacy;

  *records = NULL;
  *count = 0;
  legacy = !is_current_file(data, size, HOUSE_FILE_MAGIC);
  if (legacy)
  {
    /* A partial record is what an interrupted legacy write leaves. */
    if (size % LEGACY_HOUSE_RECORD_SIZE != 0)
      return BINARY_FORMAT_TRUNCATED;
    stored_count = size / LEGACY_HOUSE_RECORD_SIZE;
    if (stored_count > max_houses)
      return BINARY_FORMAT_LIMIT_EXCEEDED;
  }
  else
  {
    status = open_envelope(data, size, HOUSE_FILE_MAGIC, HOUSE_FILE_VERSION, &reader);
    if (status != BINARY_FORMAT_OK)
      return status;
    bytes = take(&reader, 4);
    if (!bytes)
      return BINARY_FORMAT_TRUNCATED;
    stored_count = load_u32(bytes);
    if (stored_count > max_houses)
      return BINARY_FORMAT_LIMIT_EXCEEDED;
    /* Refuse counts the remaining bytes cannot hold before allocating. */
    if (stored_count > (reader.size - reader.offset) / HOUSE_RECORD_PREFIX_SIZE)
      return BINARY_FORMAT_TRUNCATED;
  }
  if (stored_count == 0)
  {
    if (reader.offset != reader.size)
      return BINARY_FORMAT_TRAILING_DATA;
    *version = legacy ? BINARY_FORMAT_LEGACY : HOUSE_FILE_VERSION;
    return BINARY_FORMAT_OK;
  }
  decoded = calloc(stored_count, sizeof(*decoded));
  if (!decoded)
    return BINARY_FORMAT_NO_MEMORY;

  for (index = 0; index < stored_count && status == BINARY_FORMAT_OK; index++)
  {
    if (legacy)
    {
      bytes = take(&reader, LEGACY_HOUSE_RECORD_SIZE);
      if (!bytes)
      {
        status = BINARY_FORMAT_TRUNCATED;
        break;
      }
      decode_legacy_house(bytes, &decoded[index]);
      if (decoded[index].num_of_guests < 0 || decoded[index].num_of_guests > HOUSE_FILE_MAX_GUESTS)
        status = BINARY_FORMAT_LIMIT_EXCEEDED;
      continue;
    }

    bytes = take(&reader, HOUSE_RECORD_PREFIX_SIZE);
    if (!bytes)
    {
      status = BINARY_FORMAT_TRUNCATED;
      break;
    }
    decoded[index].vnum = load_u32(bytes + HOUSE_VNUM_OFFSET);
    decoded[index].atrium = load_u32(bytes + HOUSE_ATRIUM_OFFSET);
    decoded[index].exit_num = to_i16(load_u16(bytes + HOUSE_EXIT_OFFSET));
    decoded[index].mode = to_i32(load_u32(bytes + HOUSE_MODE_OFFSET));
    decoded[index].built_on = to_i64(load_u64(bytes + HOUSE_BUILT_ON_OFFSET));
    decoded[index].owner = to_i64(load_u64(bytes + HOUSE_OWNER_OFFSET));
    decoded[index].last_payment = to_i64(load_u64(bytes + HOUSE_LAST_PAYMENT_OFFSET));
    decoded[index].bitvector = to_i64(load_u64(bytes + HOUSE_BITVECTOR_OFFSET));
    decoded[index].builtby = to_i64(load_u64(bytes + HOUSE_BUILTBY_OFFSET));
    guest_count = load_u32(bytes + HOUSE_GUEST_COUNT_OFFSET);
    if (guest_count > HOUSE_FILE_MAX_GUESTS)
    {
      status = BINARY_FORMAT_LIMIT_EXCEEDED;
      break;
    }
    decoded[index].num_of_guests = (int32_t)guest_count;
    bytes = take(&reader, (size_t)guest_count * HOUSE_GUEST_SIZE);
    if (!bytes)
    {
      status = BINARY_FORMAT_TRUNCATED;
      break;
    }
    for (guest = 0; guest < guest_count; guest++)
      decoded[index].guests[guest] = to_i64(load_u64(bytes + (size_t)guest * HOUSE_GUEST_SIZE));
  }

  if (status == BINARY_FORMAT_OK && reader.offset != reader.size)
    status = BINARY_FORMAT_TRAILING_DATA;
  if (status != BINARY_FORMAT_OK)
  {
    free(decoded);
    return status;
  }
  *records = decoded;
  *count = stored_count;
  *version = legacy ? BINARY_FORMAT_LEGACY : HOUSE_FILE_VERSION;
  return BINARY_FORMAT_OK;
}

enum binary_format_status house_file_encode(const struct house_file_record *records, size_t count,
                                            unsigned char **data, size_t *size)
{
  struct byte_writer writer = {NULL, 0, 0, false};
  unsigned char prefix[HOUSE_RECORD_PREFIX_SIZE];
  unsigned char guest_bytes[HOUSE_GUEST_SIZE];
  const struct house_file_record *record;
  size_t index;
  int32_t guest;

  *data = NULL;
  *size = 0;
  if (count > UINT32_MAX)
    return BINARY_FORMAT_LIMIT_EXCEEDED;

  start_envelope(&writer);
  put_u32(&writer, (uint32_t)count);
  for (index = 0; index < count; index++)
  {
    record = &records[index];
    if (record->num_of_guests < 0 || record->num_of_guests > HOUSE_FILE_MAX_GUESTS)
    {
      free(writer.data);
      return BINARY_FORMAT_LIMIT_EXCEEDED;
    }
    store_u32(prefix + HOUSE_VNUM_OFFSET, record->vnum);
    store_u32(prefix + HOUSE_ATRIUM_OFFSET, record->atrium);
    store_u16(prefix + HOUSE_EXIT_OFFSET, (uint16_t)record->exit_num);
    store_u32(prefix + HOUSE_MODE_OFFSET, (uint32_t)record->mode);
    store_u64(prefix + HOUSE_BUILT_ON_OFFSET, (uint64_t)record->built_on);
    store_u64(prefix + HOUSE_OWNER_OFFSET, (uint64_t)record->owner);
    store_u64(prefix + HOUSE_LAST_PAYMENT_OFFSET, (uint64_t)record->last_payment);
    store_u64(prefix + HOUSE_BITVECTOR_OFFSET, (uint64_t)record->bitvector);
    store_u64(prefix + HOUSE_BUILTBY_OFFSET, (uint64_t)record->builtby);
    store_u32(prefix + HOUSE_GUEST_COUNT_OFFSET, (uint32_t)record->num_of_guests);
    put(&writer, prefix, sizeof(prefix));
    for (guest = 0; guest < record->num_of_guests; guest++)
    {
      store_u64(guest_bytes, (uint64_t)record->guests[guest]);
      put(&writer, guest_bytes, sizeof(guest_bytes));
    }
  }
  return finish_envelope(&writer, HOUSE_FILE_MAGIC, HOUSE_FILE_VERSION, data, size);
}

static void copy_fixed_string(char *text, const unsigned char *field, size_t field_size)
{
  const unsigned char *terminator = memchr(field, '\0', field_size);
  size_t length = terminator ? (size_t)(terminator - field) : field_size;

  memcpy(text, field, length);
  text[length] = '\0';
}

void last_log_decode_record(const unsigned char *record, struct last_log_record *entry)
{
  entry->close_type = to_i32(load_u32(record + LEGACY_LAST_CLOSE_TYPE_OFFSET));
  copy_fixed_string(entry->hostname, record + LEGACY_LAST_HOSTNAME_OFFSET, LAST_LOG_HOSTNAME_SIZE);
  copy_fixed_string(entry->username, record + LEGACY_LAST_USERNAME_OFFSET, LAST_LOG_USERNAME_SIZE);
  entry->login_time = to_i64(load_u64(record + LEGACY_LAST_LOGIN_TIME_OFFSET));
  entry->close_time = to_i64(load_u64(record + LEGACY_LAST_CLOSE_TIME_OFFSET));
  entry->idnum = to_i32(load_u32(record + LEGACY_LAST_IDNUM_OFFSET));
  entry->punique = to_i32(load_u32(record + LEGACY_LAST_PUNIQUE_OFFSET));
}
