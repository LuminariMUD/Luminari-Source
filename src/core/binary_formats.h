/**
 * @file binary_formats.h
 * Portable encoders and decoders for the server's durable binary files.
 *
 * Every multi-byte integer is little-endian whatever the host. A current file
 * starts with a 16-byte envelope: four magic bytes, a u16 format version, the
 * u16 byte-order mark 0xFEFF, the u32 payload size, and the u32 CRC-32 of the
 * payload. Legacy files are the native x86-64 structure layouts the server
 * wrote before issue #95; they are decoded field by field and never written.
 *
 * Nothing here opens files or touches game state, so the same decoders run in
 * the server, the focused test harness, and the fuzzer on any architecture.
 * docs/systems/BINARY_FILE_FORMATS.md is the specification.
 */
#ifndef LUMINARI_BINARY_FORMATS_H
#define LUMINARI_BINARY_FORMATS_H

#include <stddef.h>
#include <stdint.h>

#define BINARY_FORMAT_MAGIC_SIZE 4
#define BINARY_FORMAT_HEADER_SIZE 16
#define BINARY_FORMAT_BYTE_ORDER_MARK 0xFEFF
/** Version reported when a decoder read a legacy native layout. */
#define BINARY_FORMAT_LEGACY 0

enum binary_format_status
{
  BINARY_FORMAT_OK = 0,
  BINARY_FORMAT_TRUNCATED,
  BINARY_FORMAT_TRAILING_DATA,
  BINARY_FORMAT_BAD_MAGIC,
  BINARY_FORMAT_UNSUPPORTED_VERSION,
  BINARY_FORMAT_BAD_BYTE_ORDER,
  BINARY_FORMAT_CHECKSUM_MISMATCH,
  BINARY_FORMAT_LIMIT_EXCEEDED,
  BINARY_FORMAT_BAD_STRING,
  BINARY_FORMAT_NO_MEMORY
};

const char *binary_format_status_name(enum binary_format_status status);

/** CRC-32 (ISO-HDLC, as used by zlib and PNG) of a byte range. */
uint32_t binary_format_crc32(const unsigned char *data, size_t size);

/* Bulletin board files (lib/etc/board.*). */
#define BOARD_FILE_MAGIC "LMBD"
#define BOARD_FILE_VERSION 1
/** Largest heading or message accepted, in bytes including the terminator. */
#define BOARD_FILE_MAX_HEADING_SIZE 1024
#define BOARD_FILE_MAX_MESSAGE_SIZE 49152

/** One post. Each string is NUL-terminated, or NULL when the post lacks it. */
struct board_file_message
{
  int32_t level;
  char *heading;
  char *message;
};

/** Largest file, current or legacy, that can hold max_messages posts. */
size_t board_file_max_size(size_t max_messages);

/** Decodes a current or legacy board file of at most max_messages posts.
 * On success the caller owns *messages (release with board_file_free()) and
 * *version is BOARD_FILE_VERSION or BINARY_FORMAT_LEGACY. On failure nothing
 * is allocated, *messages is NULL, and *count is 0. */
enum binary_format_status board_file_decode(const unsigned char *data, size_t size,
                                            size_t max_messages,
                                            struct board_file_message **messages, size_t *count,
                                            int *version);

/** Encodes posts as a current board file; the caller frees *data. */
enum binary_format_status board_file_encode(const struct board_file_message *messages, size_t count,
                                            unsigned char **data, size_t *size);

void board_file_free(struct board_file_message *messages, size_t count);

/* House control file (lib/etc/hcontrol). */
#define HOUSE_FILE_MAGIC "LMHC"
#define HOUSE_FILE_VERSION 1
/** Guest slots per house; the legacy layout fixes this at 99. */
#define HOUSE_FILE_MAX_GUESTS 99

struct house_file_record
{
  uint32_t vnum;
  uint32_t atrium;
  int16_t exit_num;
  int32_t mode;
  int64_t built_on;
  int64_t owner;
  int64_t last_payment;
  int64_t bitvector;
  int64_t builtby;
  int32_t num_of_guests;
  int64_t guests[HOUSE_FILE_MAX_GUESTS];
};

/** Largest file, current or legacy, that can hold max_houses records. */
size_t house_file_max_size(size_t max_houses);

/** Decodes a current or legacy house control file of at most max_houses
 * records. On success the caller frees *records and *version is
 * HOUSE_FILE_VERSION or BINARY_FORMAT_LEGACY. On failure nothing is
 * allocated, *records is NULL, and *count is 0. Guest slots past
 * num_of_guests are zero. */
enum binary_format_status house_file_decode(const unsigned char *data, size_t size,
                                            size_t max_houses, struct house_file_record **records,
                                            size_t *count, int *version);

/** Encodes records as a current house control file; the caller frees *data. */
enum binary_format_status house_file_encode(const struct house_file_record *records, size_t count,
                                            unsigned char **data, size_t *size);

/* Login history (lib/etc/last). Nothing has written it since 2022, so it has
 * only the legacy layout: a sequence of fixed-size records. */
#define LAST_LOG_RECORD_SIZE 304
#define LAST_LOG_HOSTNAME_SIZE 256
#define LAST_LOG_USERNAME_SIZE 16

struct last_log_record
{
  int32_t close_type;
  int32_t idnum;
  int32_t punique;
  int64_t login_time;
  int64_t close_time;
  char hostname[LAST_LOG_HOSTNAME_SIZE + 1];
  char username[LAST_LOG_USERNAME_SIZE + 1];
};

/** Decodes one LAST_LOG_RECORD_SIZE-byte record. Every byte pattern is a
 * valid record; the strings are always terminated. */
void last_log_decode_record(const unsigned char *record, struct last_log_record *entry);

#endif /* LUMINARI_BINARY_FORMATS_H */
