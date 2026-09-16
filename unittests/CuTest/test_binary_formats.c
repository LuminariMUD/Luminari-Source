/*
 * Tests for core/binary_formats.c: golden legacy and current files, every
 * truncation, every single-bit corruption of a current file, and malformed
 * fields with valid checksums. The file needs no game state, so it runs both
 * in the production-linked suite and in the focused binary_formats_tests
 * harness (unittests/CuTest/Makefile), which also runs on AArch64.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"
#include "binary_format_fixtures.h"
#include "../../src/core/binary_formats.h"

#ifdef BINARY_FORMATS_FOCUSED_HARNESS
/* The generated runner points the server log stream at stderr. */
FILE *logfile;
#endif

struct fixture_run
{
  size_t offset;
  size_t length;
  const unsigned char *bytes;
};

#define FIXTURE_RUN(offset, text) {(offset), sizeof(text) - 1, (const unsigned char *)(text)}

/* Zero bytes are omitted: each run is a stretch of the file that is not zero.
 * The legacy files were written by the pre-issue-95 server's native structure
 * fwrite() calls on x86-64, including a stale heading pointer, stale guest and
 * spare slots, and an unterminated 16-byte username. */
/* board.legacy: 174 bytes */
static const struct fixture_run legacy_board_file[] = {
    FIXTURE_RUN(0, "\x02\x00\x00\x00\x02"),
    FIXTURE_RUN(12, "\x30\x4e\x62\x22"),
    FIXTURE_RUN(20, "\x22\x00\x00\x00\x2b\x00\x00\x00\x17"),
    FIXTURE_RUN(36, "\x54\x75\x65\x20\x4e\x6f\x76\x20\x30\x37\x20\x32\x30\x31\x37\x20\x28\x54"
                    "\x65\x73\x74\x65\x72\x29\x20\x20\x20\x20\x20\x3a\x3a\x20\x46\x69\x72\x73"
                    "\x74\x20\x70\x6f\x73\x74\x00\x4c\x69\x6e\x65\x20\x6f\x6e\x65\x2e\x0d\x0a"
                    "\x4c\x69\x6e\x65\x20\x74\x77\x6f\x2e\x0d\x0a\x00\x03"),
    FIXTURE_RUN(110, "\xb0\x4e\x62\x22"),
    FIXTURE_RUN(118, "\x01\x00\x00\x00\x28"),
    FIXTURE_RUN(134, "\x57\x65\x64\x20\x4a\x61\x6e\x20\x31\x37\x20\x32\x30\x31\x38\x20\x28\x4f"
                     "\x74\x68\x65\x72\x29\x20\x20\x20\x20\x20\x20\x3a\x3a\x20\x4e\x6f\x20\x62"
                     "\x6f\x64\x79"),
};

/* board.v1: 150 bytes */
static const struct fixture_run current_board_file[] = {
    FIXTURE_RUN(0, "\x4c\x4d\x42\x44\x01\x00\xff\xfe\x86\x00\x00\x00\x71\xc0\x1f\x72\x02\x00"
                   "\x00\x00\x22\x00\x00\x00\x2b\x00\x00\x00\x54\x75\x65\x20\x4e\x6f\x76\x20"
                   "\x30\x37\x20\x32\x30\x31\x37\x20\x28\x54\x65\x73\x74\x65\x72\x29\x20\x20"
                   "\x20\x20\x20\x3a\x3a\x20\x46\x69\x72\x73\x74\x20\x70\x6f\x73\x74\x00\x17"
                   "\x00\x00\x00\x4c\x69\x6e\x65\x20\x6f\x6e\x65\x2e\x0d\x0a\x4c\x69\x6e\x65"
                   "\x20\x74\x77\x6f\x2e\x0d\x0a\x00\x01\x00\x00\x00\x28\x00\x00\x00\x57\x65"
                   "\x64\x20\x4a\x61\x6e\x20\x31\x37\x20\x32\x30\x31\x38\x20\x28\x4f\x74\x68"
                   "\x65\x72\x29\x20\x20\x20\x20\x20\x20\x3a\x3a\x20\x4e\x6f\x20\x62\x6f\x64"
                   "\x79\x00\x00\x00\x00\x00"),
};

/* hcontrol.legacy: 1824 bytes */
static const struct fixture_run legacy_house_file[] = {
    FIXTURE_RUN(0, "\x48\x94\x01\x00\x47\x94\x01\x00\x02"),
    FIXTURE_RUN(17, "\x10\x5e\x5f"),
    FIXTURE_RUN(24, "\x02"),
    FIXTURE_RUN(32, "\x92\x10"),
    FIXTURE_RUN(40, "\x03"),
    FIXTURE_RUN(48, "\x07"),
    FIXTURE_RUN(56, "\x08"),
    FIXTURE_RUN(64, "\x09"),
    FIXTURE_RUN(72, "\x09\x03"),
    FIXTURE_RUN(841, "\xf1\x53\x65"),
    FIXTURE_RUN(848, "\x06"),
    FIXTURE_RUN(856, "\x01"),
    FIXTURE_RUN(864, "\xff\xff\xff\xff\xff\xff\xff\xff"),
    FIXTURE_RUN(904, "\x88\x77\x66\x55\x44\x33\x22\x11\x83\x2c\x00\x00\x82\x2c\x00\x00\x05"),
    FIXTURE_RUN(929, "\x2f\x68\x59"),
    FIXTURE_RUN(944, "\x63"),
};

/* hcontrol.v1: 160 bytes */
static const struct fixture_run current_house_file[] = {
    FIXTURE_RUN(0, "\x4c\x4d\x48\x43\x01\x00\xff\xfe\x90\x00\x00\x00\x4d\x72\xe5\x16\x02\x00"
                   "\x00\x00\x48\x94\x01\x00\x47\x94\x01\x00\x02\x00\x02\x00\x00\x00\x00\x10"
                   "\x5e\x5f\x00\x00\x00\x00\x92\x10\x00\x00\x00\x00\x00\x00\x00\xf1\x53\x65"
                   "\x00\x00\x00\x00\x06\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00"
                   "\x00\x00\x03\x00\x00\x00\x07\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00"
                   "\x00\x00\x00\x00\x09\x00\x00\x00\x00\x00\x00\x00\x83\x2c\x00\x00\x82\x2c"
                   "\x00\x00\x05\x00\x00\x00\x00\x00\x00\x2f\x68\x59\x00\x00\x00\x00\x63\x00"
                   "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                   "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"),
};

/* last.legacy: 608 bytes */
static const struct fixture_run legacy_last_log[] = {
    FIXTURE_RUN(0, "\x04\x00\x00\x00\x31\x39\x38\x2e\x35\x31\x2e\x31\x30\x30\x2e\x37"),
    FIXTURE_RUN(260, "\x5a\x75\x73\x75\x6b"),
    FIXTURE_RUN(281, "\x97\xf1\x62"),
    FIXTURE_RUN(288, "\x10\xa5\xf1\x62"),
    FIXTURE_RUN(296, "\x0c\x00\x00\x00\x22\x00\x00\x00\x2a\x00\x00\x00\x68\x6f\x73\x74\x2e\x65"
                     "\x78\x61\x6d\x70\x6c\x65\x2e\x6e\x65\x74"),
    FIXTURE_RUN(564, "\x41\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70"),
    FIXTURE_RUN(584, "\x20\xb3\xf1\x62"),
    FIXTURE_RUN(600, "\xfb\xff\xff\xff\x07"),
};

struct fixture
{
  const struct fixture_run *runs;
  size_t run_count;
  size_t size;
};

#define FIXTURE(runs, size) {(runs), sizeof(runs) / sizeof((runs)[0]), (size)}

static const struct fixture fixtures[] = {
    [FIXTURE_LEGACY_BOARD_FILE] = FIXTURE(legacy_board_file, 174),
    [FIXTURE_LEGACY_HOUSE_FILE] = FIXTURE(legacy_house_file, 1824),
    [FIXTURE_LEGACY_LAST_LOG] = FIXTURE(legacy_last_log, 608),
    [FIXTURE_CURRENT_BOARD_FILE] = FIXTURE(current_board_file, 150),
    [FIXTURE_CURRENT_HOUSE_FILE] = FIXTURE(current_house_file, 160),
};

unsigned char *binary_format_fixture(enum binary_format_fixture fixture, size_t *size)
{
  const struct fixture *source = &fixtures[fixture];
  unsigned char *bytes;
  size_t index;

  *size = source->size;
  bytes = calloc(source->size + 1, 1);
  if (bytes == NULL)
    abort();
  for (index = 0; index < source->run_count; index++)
    memcpy(bytes + source->runs[index].offset, source->runs[index].bytes,
           source->runs[index].length);
  return bytes;
}

static const char first_heading[] = "Tue Nov 07 2017 (Tester)     :: First post";
static const char first_message[] = "Line one.\r\nLine two.\r\n";
static const char second_heading[] = "Wed Jan 17 2018 (Other)      :: No body";

/* Room for any fixture plus one appended byte; field edits happen in place. */
#define EDIT_BUFFER_SIZE 4096

/* Test buffers: running out of memory ends the run rather than a test. */
static unsigned char *allocate_bytes(size_t size)
{
  unsigned char *bytes = malloc(size);

  if (bytes == NULL)
    abort();
  return bytes;
}

static void store_u32(unsigned char *bytes, uint32_t value)
{
  bytes[0] = (unsigned char)(value & 0xFF);
  bytes[1] = (unsigned char)((value >> 8) & 0xFF);
  bytes[2] = (unsigned char)((value >> 16) & 0xFF);
  bytes[3] = (unsigned char)(value >> 24);
}

/* Rewrites the payload size and checksum so only the edited field is wrong. */
static void reseal(unsigned char *file, size_t size)
{
  uint32_t payload_size;

  if (size < BINARY_FORMAT_HEADER_SIZE)
    abort();
  payload_size = (uint32_t)(size - BINARY_FORMAT_HEADER_SIZE);

  store_u32(file + 8, payload_size);
  store_u32(file + 12, binary_format_crc32(file + BINARY_FORMAT_HEADER_SIZE, payload_size));
}

static enum binary_format_status decode_board(const unsigned char *file, size_t size,
                                              size_t max_messages, size_t *count)
{
  struct board_file_message *messages = NULL;
  enum binary_format_status status;
  int version = -1;

  status = board_file_decode(file, size, max_messages, &messages, count, &version);
  board_file_free(messages, *count);
  return status;
}

static enum binary_format_status decode_house(const unsigned char *file, size_t size,
                                              size_t max_houses, size_t *count)
{
  struct house_file_record *records = NULL;
  enum binary_format_status status;
  int version = -1;

  status = house_file_decode(file, size, max_houses, &records, count, &version);
  free(records);
  return status;
}

static void assert_fixture_messages(CuTest *tc, const struct board_file_message *messages,
                                    size_t count)
{
  CuAssertIntEquals(tc, 2, (int)count);
  CuAssertIntEquals(tc, 34, messages[0].level);
  CuAssertStrEquals(tc, first_heading, messages[0].heading);
  CuAssertStrEquals(tc, first_message, messages[0].message);
  CuAssertIntEquals(tc, 1, messages[1].level);
  CuAssertStrEquals(tc, second_heading, messages[1].heading);
  CuAssertPtrEquals(tc, NULL, messages[1].message);
}

static void assert_fixture_houses(CuTest *tc, const struct house_file_record *records, size_t count)
{
  int guest;

  CuAssertIntEquals(tc, 2, (int)count);
  CuAssertTrue(tc, records[0].vnum == 103496 && records[0].atrium == 103495);
  CuAssertIntEquals(tc, 2, records[0].exit_num);
  CuAssertIntEquals(tc, 2, records[0].mode);
  CuAssertTrue(tc, records[0].built_on == 1600000000 && records[0].owner == 4242);
  CuAssertTrue(tc, records[0].last_payment == 1700000000 && records[0].bitvector == 6);
  CuAssertTrue(tc, records[0].builtby == 1);
  CuAssertIntEquals(tc, 3, records[0].num_of_guests);
  CuAssertTrue(tc, records[0].guests[0] == 7 && records[0].guests[1] == 8);
  CuAssertTrue(tc, records[0].guests[2] == 9);
  /* The stale value in the fourth legacy slot is not a guest. */
  for (guest = 3; guest < HOUSE_FILE_MAX_GUESTS; guest++)
    CuAssertTrue(tc, records[0].guests[guest] == 0);

  CuAssertTrue(tc, records[1].vnum == 11395 && records[1].atrium == 11394);
  CuAssertIntEquals(tc, 5, records[1].exit_num);
  CuAssertIntEquals(tc, 0, records[1].mode);
  CuAssertTrue(tc, records[1].built_on == 1500000000 && records[1].owner == 99);
  CuAssertTrue(tc, records[1].last_payment == 0 && records[1].bitvector == 0);
  CuAssertIntEquals(tc, 0, records[1].num_of_guests);
}

void Test_binary_format_crc32_matches_the_reference_check_value(CuTest *tc)
{
  static const unsigned char check[] = "123456789";

  CuAssertTrue(tc, binary_format_crc32(check, 9) == UINT32_C(0xCBF43926));
  CuAssertTrue(tc, binary_format_crc32(check, 0) == 0);
}

void Test_board_file_reads_the_legacy_x86_64_layout(CuTest *tc)
{
  struct board_file_message *messages = NULL;
  unsigned char *file;
  size_t size, count = 0;
  int version = -1;

  file = binary_format_fixture(FIXTURE_LEGACY_BOARD_FILE, &size);
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    board_file_decode(file, size, 300, &messages, &count, &version));
  CuAssertIntEquals(tc, BINARY_FORMAT_LEGACY, version);
  assert_fixture_messages(tc, messages, count);
  board_file_free(messages, count);

  /* The slot number and heading pointer were runtime values. */
  file[4] ^= 0x7F;
  file[12] ^= 0xFF;
  file[15] ^= 0x80;
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    board_file_decode(file, size, 300, &messages, &count, &version));
  assert_fixture_messages(tc, messages, count);
  board_file_free(messages, count);
  free(file);
}

void Test_board_file_writes_the_golden_current_format(CuTest *tc)
{
  struct board_file_message *messages = NULL;
  unsigned char *legacy, *golden, *encoded = NULL;
  size_t legacy_size, golden_size, encoded_size = 0, count = 0;
  int version = -1;

  legacy = binary_format_fixture(FIXTURE_LEGACY_BOARD_FILE, &legacy_size);
  golden = binary_format_fixture(FIXTURE_CURRENT_BOARD_FILE, &golden_size);
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    board_file_decode(legacy, legacy_size, 300, &messages, &count, &version));
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    board_file_encode(messages, count, &encoded, &encoded_size));
  board_file_free(messages, count);
  CuAssertIntEquals(tc, (int)golden_size, (int)encoded_size);
  CuAssertTrue(tc, memcmp(golden, encoded, golden_size) == 0);

  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    board_file_decode(golden, golden_size, 300, &messages, &count, &version));
  CuAssertIntEquals(tc, BOARD_FILE_VERSION, version);
  assert_fixture_messages(tc, messages, count);
  board_file_free(messages, count);
  free(encoded);
  free(golden);
  free(legacy);
}

void Test_board_file_rejects_every_damaged_current_file(CuTest *tc)
{
  static struct board_file_message untouched;
  struct board_file_message *messages;
  unsigned char *golden, *damaged;
  size_t size, length, position, count;
  int bit, version;

  golden = binary_format_fixture(FIXTURE_CURRENT_BOARD_FILE, &size);
  damaged = allocate_bytes(size);

  /* An empty file is an empty legacy board; every longer prefix is torn. */
  for (length = 1; length < size; length++)
  {
    messages = &untouched;
    count = 7;
    CuAssertTrue(tc, board_file_decode(golden, length, 300, &messages, &count, &version) !=
                         BINARY_FORMAT_OK);
    CuAssertTrue(tc, messages == NULL && count == 0);
  }
  for (position = 0; position < size; position++)
    for (bit = 0; bit < 8; bit++)
    {
      memcpy(damaged, golden, size);
      damaged[position] ^= (unsigned char)(1U << bit);
      CuAssertTrue(tc, board_file_decode(damaged, size, 300, &messages, &count, &version) !=
                           BINARY_FORMAT_OK);
      CuAssertTrue(tc, messages == NULL && count == 0);
    }
  free(damaged);
  free(golden);
}

void Test_board_file_rejects_malformed_legacy_files(CuTest *tc)
{
  unsigned char edited[EDIT_BUFFER_SIZE];
  unsigned char *legacy;
  size_t size, length, count;

  legacy = binary_format_fixture(FIXTURE_LEGACY_BOARD_FILE, &size);
  if (size >= sizeof(edited))
    abort();

  /* An empty file was an empty board; every other prefix is torn. */
  CuAssertIntEquals(tc, BINARY_FORMAT_OK, decode_board(legacy, 0, 300, &count));
  for (length = 1; length < size; length++)
    CuAssertTrue(tc, decode_board(legacy, length, 300, &count) != BINARY_FORMAT_OK);

  memcpy(edited, legacy, size);
  store_u32(edited, 301);
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, decode_board(edited, size, 300, &count));
  store_u32(edited, UINT32_MAX);
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, decode_board(edited, size, 300, &count));
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, decode_board(legacy, size, 1, &count));

  memcpy(edited, legacy, size);
  store_u32(edited + 24, 0);
  CuAssertIntEquals(tc, BINARY_FORMAT_BAD_STRING, decode_board(edited, size, 300, &count));
  store_u32(edited + 24, UINT32_MAX);
  CuAssertIntEquals(tc, BINARY_FORMAT_BAD_STRING, decode_board(edited, size, 300, &count));
  store_u32(edited + 24, BOARD_FILE_MAX_HEADING_SIZE + 1);
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, decode_board(edited, size, 300, &count));

  memcpy(edited, legacy, size);
  store_u32(edited + 28, UINT32_MAX);
  CuAssertIntEquals(tc, BINARY_FORMAT_BAD_STRING, decode_board(edited, size, 300, &count));
  store_u32(edited + 28, INT32_MAX);
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, decode_board(edited, size, 300, &count));

  memcpy(edited, legacy, size);
  edited[78] = 'x'; /* the first heading's terminator */
  CuAssertIntEquals(tc, BINARY_FORMAT_BAD_STRING, decode_board(edited, size, 300, &count));
  memcpy(edited, legacy, size);
  edited[40] = '\0'; /* a terminator inside the first heading */
  CuAssertIntEquals(tc, BINARY_FORMAT_BAD_STRING, decode_board(edited, size, 300, &count));

  memcpy(edited, legacy, size);
  edited[size] = 0;
  CuAssertIntEquals(tc, BINARY_FORMAT_TRAILING_DATA, decode_board(edited, size + 1, 300, &count));
  free(legacy);
}

void Test_board_file_rejects_out_of_range_current_fields(CuTest *tc)
{
  unsigned char edited[EDIT_BUFFER_SIZE];
  unsigned char *golden;
  size_t size, count;

  golden = binary_format_fixture(FIXTURE_CURRENT_BOARD_FILE, &size);
  if (size >= sizeof(edited))
    abort();

  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, decode_board(golden, size, 1, &count));

  memcpy(edited, golden, size);
  store_u32(edited + 16, 3);
  reseal(edited, size);
  CuAssertIntEquals(tc, BINARY_FORMAT_TRUNCATED, decode_board(edited, size, 300, &count));
  /* A huge count is refused before anything is allocated for it. */
  store_u32(edited + 16, UINT32_MAX);
  reseal(edited, size);
  CuAssertIntEquals(tc, BINARY_FORMAT_TRUNCATED, decode_board(edited, size, SIZE_MAX, &count));

  memcpy(edited, golden, size);
  store_u32(edited + 24, UINT32_MAX);
  reseal(edited, size);
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, decode_board(edited, size, 300, &count));
  store_u32(edited + 24, BOARD_FILE_MAX_HEADING_SIZE + 1);
  reseal(edited, size);
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, decode_board(edited, size, 300, &count));

  memcpy(edited, golden, size);
  edited[70] = 'x'; /* the first heading's terminator */
  reseal(edited, size);
  CuAssertIntEquals(tc, BINARY_FORMAT_BAD_STRING, decode_board(edited, size, 300, &count));

  memcpy(edited, golden, size);
  edited[4] = 2; /* a future version */
  reseal(edited, size);
  CuAssertIntEquals(tc, BINARY_FORMAT_UNSUPPORTED_VERSION, decode_board(edited, size, 300, &count));

  memcpy(edited, golden, size);
  edited[6] = 0xFE; /* a big-endian writer */
  edited[7] = 0xFF;
  CuAssertIntEquals(tc, BINARY_FORMAT_BAD_BYTE_ORDER, decode_board(edited, size, 300, &count));

  memcpy(edited, golden, size);
  edited[size] = 0;
  reseal(edited, size + 1);
  CuAssertIntEquals(tc, BINARY_FORMAT_TRAILING_DATA, decode_board(edited, size + 1, 300, &count));
  free(golden);
}

void Test_house_file_reads_the_legacy_x86_64_layout(CuTest *tc)
{
  struct house_file_record *records = NULL;
  unsigned char *file;
  size_t size, count = 0;
  int version = -1;

  file = binary_format_fixture(FIXTURE_LEGACY_HOUSE_FILE, &size);
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    house_file_decode(file, size, 999, &records, &count, &version));
  CuAssertIntEquals(tc, BINARY_FORMAT_LEGACY, version);
  assert_fixture_houses(tc, records, count);
  free(records);
  free(file);
}

void Test_house_file_writes_the_golden_current_format(CuTest *tc)
{
  struct house_file_record *records = NULL;
  unsigned char *legacy, *golden, *encoded = NULL;
  size_t legacy_size, golden_size, encoded_size = 0, count = 0;
  int version = -1;

  legacy = binary_format_fixture(FIXTURE_LEGACY_HOUSE_FILE, &legacy_size);
  golden = binary_format_fixture(FIXTURE_CURRENT_HOUSE_FILE, &golden_size);
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    house_file_decode(legacy, legacy_size, 999, &records, &count, &version));
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    house_file_encode(records, count, &encoded, &encoded_size));
  free(records);
  CuAssertIntEquals(tc, (int)golden_size, (int)encoded_size);
  CuAssertTrue(tc, memcmp(golden, encoded, golden_size) == 0);

  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    house_file_decode(golden, golden_size, 999, &records, &count, &version));
  CuAssertIntEquals(tc, HOUSE_FILE_VERSION, version);
  assert_fixture_houses(tc, records, count);
  free(records);
  free(encoded);
  free(golden);
  free(legacy);
}

void Test_house_file_rejects_damaged_and_malformed_files(CuTest *tc)
{
  static struct house_file_record untouched;
  unsigned char edited[EDIT_BUFFER_SIZE];
  struct house_file_record *records;
  unsigned char *legacy, *golden;
  size_t legacy_size, size, length, position, count;
  int bit, version;

  legacy = binary_format_fixture(FIXTURE_LEGACY_HOUSE_FILE, &legacy_size);
  golden = binary_format_fixture(FIXTURE_CURRENT_HOUSE_FILE, &size);
  if (legacy_size >= sizeof(edited) || size >= sizeof(edited))
    abort();

  /* A partial legacy record is what an interrupted legacy write left. */
  for (length = 1; length < legacy_size; length++)
    CuAssertTrue(tc, decode_house(legacy, length, 999, &count) != BINARY_FORMAT_OK ||
                         length % 912 == 0);
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, decode_house(legacy, legacy_size, 1, &count));
  memcpy(edited, legacy, legacy_size);
  store_u32(edited + 40, HOUSE_FILE_MAX_GUESTS + 1);
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED,
                    decode_house(edited, legacy_size, 999, &count));
  store_u32(edited + 40, UINT32_MAX);
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED,
                    decode_house(edited, legacy_size, 999, &count));

  for (length = 1; length < size; length++)
  {
    records = &untouched;
    count = 7;
    CuAssertTrue(tc, house_file_decode(golden, length, 999, &records, &count, &version) !=
                         BINARY_FORMAT_OK);
    CuAssertTrue(tc, records == NULL && count == 0);
  }
  for (position = 0; position < size; position++)
    for (bit = 0; bit < 8; bit++)
    {
      memcpy(edited, golden, size);
      edited[position] ^= (unsigned char)(1U << bit);
      CuAssertTrue(tc, house_file_decode(edited, size, 999, &records, &count, &version) !=
                           BINARY_FORMAT_OK);
      CuAssertTrue(tc, records == NULL && count == 0);
    }

  memcpy(edited, golden, size);
  store_u32(edited + 74, HOUSE_FILE_MAX_GUESTS + 1); /* the first house's guest count */
  reseal(edited, size);
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, decode_house(edited, size, 999, &count));
  memcpy(edited, golden, size);
  store_u32(edited + 16, UINT32_MAX);
  reseal(edited, size);
  CuAssertIntEquals(tc, BINARY_FORMAT_TRUNCATED, decode_house(edited, size, SIZE_MAX, &count));
  memcpy(edited, golden, size);
  edited[size] = 0;
  reseal(edited, size + 1);
  CuAssertIntEquals(tc, BINARY_FORMAT_TRAILING_DATA, decode_house(edited, size + 1, 999, &count));
  free(golden);
  free(legacy);
}

void Test_binary_format_damaged_magic_is_not_read_as_a_legacy_file(CuTest *tc)
{
  static struct house_file_record houses[10], untouched;
  struct house_file_record *records;
  enum binary_format_status status;
  unsigned char *file = NULL, *board;
  size_t size = 0, board_size, position, count;
  int bit, version;

  board = binary_format_fixture(FIXTURE_CURRENT_BOARD_FILE, &board_size);
  /* Exactly one legacy record long, with zeros where a legacy reader finds
   * the guest count: without its magic, only the envelope marks it current. */
  houses[0].num_of_guests = 39;
  CuAssertIntEquals(tc, BINARY_FORMAT_OK, house_file_encode(houses, 10, &file, &size));
  CuAssertIntEquals(tc, 912, (int)size);
  for (position = 0; position < BINARY_FORMAT_HEADER_SIZE; position++)
    for (bit = 0; bit < 8; bit++)
    {
      file[position] ^= (unsigned char)(1U << bit);
      records = &untouched;
      count = 7;
      status = house_file_decode(file, size, 999, &records, &count, &version);
      CuAssertTrue(tc, status != BINARY_FORMAT_OK && records == NULL && count == 0);
      if (position < BINARY_FORMAT_MAGIC_SIZE)
        CuAssertIntEquals(tc, BINARY_FORMAT_BAD_MAGIC, status);
      file[position] ^= (unsigned char)(1U << bit);
    }
  file[0] = 'X';
  CuAssertIntEquals(tc, BINARY_FORMAT_BAD_MAGIC, decode_house(file, size, 999, &count));
  memcpy(file, board, BINARY_FORMAT_MAGIC_SIZE); /* a board file's magic */
  CuAssertIntEquals(tc, BINARY_FORMAT_BAD_MAGIC, decode_house(file, size, 999, &count));
  free(file);

  memset(board, 0, BINARY_FORMAT_MAGIC_SIZE);
  CuAssertIntEquals(tc, BINARY_FORMAT_BAD_MAGIC, decode_board(board, board_size, 300, &count));
  free(board);
}

void Test_last_log_reads_the_legacy_x86_64_layout(CuTest *tc)
{
  struct last_log_record entry;
  unsigned char *file;
  size_t size;

  file = binary_format_fixture(FIXTURE_LEGACY_LAST_LOG, &size);
  CuAssertIntEquals(tc, 2 * LAST_LOG_RECORD_SIZE, (int)size);

  last_log_decode_record(file, &entry);
  CuAssertIntEquals(tc, 4, entry.close_type);
  CuAssertStrEquals(tc, "198.51.100.7", entry.hostname);
  CuAssertStrEquals(tc, "Zusuk", entry.username);
  CuAssertTrue(tc, entry.login_time == 1660000000 && entry.close_time == 1660003600);
  CuAssertIntEquals(tc, 12, entry.idnum);
  CuAssertIntEquals(tc, 34, entry.punique);

  last_log_decode_record(file + LAST_LOG_RECORD_SIZE, &entry);
  CuAssertIntEquals(tc, 42, entry.close_type);
  CuAssertStrEquals(tc, "host.example.net", entry.hostname);
  CuAssertStrEquals(tc, "Abcdefghijklmnop", entry.username);
  CuAssertTrue(tc, entry.login_time == 1660007200 && entry.close_time == 0);
  CuAssertIntEquals(tc, -5, entry.idnum);
  CuAssertIntEquals(tc, 7, entry.punique);
  free(file);
}

void Test_binary_format_encoders_refuse_what_decoders_reject(CuTest *tc)
{
  struct board_file_message message = {1, NULL, NULL};
  struct house_file_record record;
  unsigned char *data = NULL;
  size_t size = 0;
  char *heading;

  heading = (char *)allocate_bytes(BOARD_FILE_MAX_HEADING_SIZE + 1);
  memset(heading, 'a', BOARD_FILE_MAX_HEADING_SIZE);
  heading[BOARD_FILE_MAX_HEADING_SIZE] = '\0';
  message.heading = heading;
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, board_file_encode(&message, 1, &data, &size));
  CuAssertTrue(tc, data == NULL && size == 0);
  free(heading);

  memset(&record, 0, sizeof(record));
  record.num_of_guests = HOUSE_FILE_MAX_GUESTS + 1;
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, house_file_encode(&record, 1, &data, &size));
  record.num_of_guests = -1;
  CuAssertIntEquals(tc, BINARY_FORMAT_LIMIT_EXCEEDED, house_file_encode(&record, 1, &data, &size));
  CuAssertTrue(tc, data == NULL && size == 0);
}

void Test_binary_format_empty_files_round_trip(CuTest *tc)
{
  struct board_file_message *messages = NULL;
  struct house_file_record *records = NULL;
  unsigned char *data = NULL;
  size_t size = 0, count = 1;
  int version = -1;

  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    board_file_decode(NULL, 0, 300, &messages, &count, &version));
  CuAssertTrue(tc, messages == NULL && count == 0 && version == BINARY_FORMAT_LEGACY);
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    house_file_decode(NULL, 0, 999, &records, &count, &version));
  CuAssertTrue(tc, records == NULL && count == 0 && version == BINARY_FORMAT_LEGACY);

  CuAssertIntEquals(tc, BINARY_FORMAT_OK, board_file_encode(NULL, 0, &data, &size));
  CuAssertIntEquals(tc, BINARY_FORMAT_HEADER_SIZE + 4, (int)size);
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    board_file_decode(data, size, 300, &messages, &count, &version));
  CuAssertTrue(tc, messages == NULL && count == 0 && version == BOARD_FILE_VERSION);
  free(data);

  CuAssertIntEquals(tc, BINARY_FORMAT_OK, house_file_encode(NULL, 0, &data, &size));
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    house_file_decode(data, size, 999, &records, &count, &version));
  CuAssertTrue(tc, records == NULL && count == 0 && version == HOUSE_FILE_VERSION);
  free(data);
}
