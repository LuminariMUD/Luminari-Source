/*
 * Golden files for the durable binary formats (docs/systems/BINARY_FILE_FORMATS.md),
 * defined in test_binary_formats.c.
 */
#ifndef BINARY_FORMAT_FIXTURES_H
#define BINARY_FORMAT_FIXTURES_H

#include <stddef.h>

enum binary_format_fixture
{
  /* Bytes the server wrote before issue #95 with native fwrite() on x86-64. */
  FIXTURE_LEGACY_BOARD_FILE,
  FIXTURE_LEGACY_HOUSE_FILE,
  FIXTURE_LEGACY_LAST_LOG,
  /* The same data in the current formats. */
  FIXTURE_CURRENT_BOARD_FILE,
  FIXTURE_CURRENT_HOUSE_FILE
};

/* Returns a heap copy of a fixture, with one spare byte, and its size. */
unsigned char *binary_format_fixture(enum binary_format_fixture fixture, size_t *size);

#endif /* BINARY_FORMAT_FIXTURES_H */
