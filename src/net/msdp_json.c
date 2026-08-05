#include <arpa/telnet.h>
#include <limits.h>
#include <stdint.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "protocol.h"
#include "msdp_json.h"

#define MSDP_JSON_MAX_DEPTH 32

struct msdp_json_writer
{
  char *buffer;
  size_t capacity;
  size_t length;
};

static protocol_error_t msdp_json_append(struct msdp_json_writer *writer, const void *data,
                                         size_t data_length)
{
  if (writer == NULL || data == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  if (writer->length > SIZE_MAX - data_length - 1)
    return PROTOCOL_ERROR_BUFFER_FULL;

  if (writer->buffer != NULL)
  {
    if (writer->capacity == 0 || writer->length >= writer->capacity ||
        data_length >= writer->capacity - writer->length)
      return PROTOCOL_ERROR_BUFFER_FULL;

    memcpy(writer->buffer + writer->length, data, data_length);
  }

  writer->length += data_length;
  if (writer->buffer != NULL)
    writer->buffer[writer->length] = '\0';

  return PROTOCOL_SUCCESS;
}

static protocol_error_t msdp_json_append_char(struct msdp_json_writer *writer, unsigned char value)
{
  return msdp_json_append(writer, &value, 1);
}

static bool msdp_json_is_reserved(unsigned char value)
{
  return (value >= MSDP_VAR && value <= MSDP_ARRAY_CLOSE) || value == IAC;
}

static bool msdp_json_is_continuation(unsigned char value)
{
  return value >= 0x80 && value <= 0xbf;
}

static bool msdp_json_is_valid_utf8(const unsigned char *text, size_t length)
{
  size_t index;

  if (text == NULL)
    return false;

  index = 0;
  while (index < length)
  {
    unsigned char first;

    first = text[index];
    if (first <= 0x7f)
    {
      index++;
      continue;
    }

    if (first >= 0xc2 && first <= 0xdf)
    {
      if (index + 1 >= length || !msdp_json_is_continuation(text[index + 1]))
        return false;
      index += 2;
      continue;
    }

    if (first >= 0xe0 && first <= 0xef)
    {
      unsigned char second;

      if (index + 2 >= length)
        return false;
      second = text[index + 1];
      if (!msdp_json_is_continuation(text[index + 2]))
        return false;
      if ((first == 0xe0 && (second < 0xa0 || second > 0xbf)) ||
          (first == 0xed && (second < 0x80 || second > 0x9f)) ||
          (first != 0xe0 && first != 0xed && !msdp_json_is_continuation(second)))
        return false;
      index += 3;
      continue;
    }

    if (first >= 0xf0 && first <= 0xf4)
    {
      unsigned char second;

      if (index + 3 >= length)
        return false;
      second = text[index + 1];
      if (!msdp_json_is_continuation(text[index + 2]) ||
          !msdp_json_is_continuation(text[index + 3]))
        return false;
      if ((first == 0xf0 && (second < 0x90 || second > 0xbf)) ||
          (first == 0xf4 && (second < 0x80 || second > 0x8f)) ||
          (first != 0xf0 && first != 0xf4 && !msdp_json_is_continuation(second)))
        return false;
      index += 4;
      continue;
    }

    return false;
  }

  return true;
}

static protocol_error_t msdp_json_append_quoted(struct msdp_json_writer *writer,
                                                const unsigned char *text, size_t length,
                                                bool require_utf8)
{
  static const char hex_digits[] = "0123456789abcdef";
  protocol_error_t result;
  size_t index;

  if (writer == NULL || text == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (require_utf8 && !msdp_json_is_valid_utf8(text, length))
    return PROTOCOL_ERROR_INVALID_INPUT;

  result = msdp_json_append_char(writer, '"');
  if (result != PROTOCOL_SUCCESS)
    return result;

  for (index = 0; index < length; index++)
  {
    unsigned char value;
    const char *escape;
    char encoded[7];

    value = text[index];
    escape = NULL;

    if (msdp_json_is_reserved(value))
      return PROTOCOL_ERROR_INVALID_INPUT;

    switch (value)
    {
    case '"':
      escape = "\\\"";
      break;
    case '\\':
      escape = "\\\\";
      break;
    case '\b':
      escape = "\\b";
      break;
    case '\f':
      escape = "\\f";
      break;
    case '\n':
      escape = "\\n";
      break;
    case '\r':
      escape = "\\r";
      break;
    case '\t':
      escape = "\\t";
      break;
    default:
      break;
    }

    if (escape != NULL)
    {
      result = msdp_json_append(writer, escape, 2);
    }
    else if (value < 0x20)
    {
      encoded[0] = '\\';
      encoded[1] = 'u';
      encoded[2] = '0';
      encoded[3] = '0';
      encoded[4] = hex_digits[value >> 4];
      encoded[5] = hex_digits[value & 0x0f];
      encoded[6] = '\0';
      result = msdp_json_append(writer, encoded, 6);
    }
    else
    {
      result = msdp_json_append_char(writer, value);
    }

    if (result != PROTOCOL_SUCCESS)
      return result;
  }

  return msdp_json_append_char(writer, '"');
}

static protocol_error_t msdp_json_serialize_value(struct msdp_json_writer *writer,
                                                  const unsigned char **cursor,
                                                  const unsigned char *end, int depth,
                                                  bool require_utf8);

static protocol_error_t msdp_json_serialize_scalar(struct msdp_json_writer *writer,
                                                   const unsigned char **cursor,
                                                   const unsigned char *end,
                                                   unsigned char separator, unsigned char closer,
                                                   bool require_utf8)
{
  const unsigned char *start;

  if (writer == NULL || cursor == NULL || *cursor == NULL || end == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  start = *cursor;
  while (*cursor < end && **cursor != separator && **cursor != closer)
  {
    if (msdp_json_is_reserved(**cursor))
      return PROTOCOL_ERROR_INVALID_INPUT;
    (*cursor)++;
  }

  return msdp_json_append_quoted(writer, start, (size_t)(*cursor - start), require_utf8);
}

static protocol_error_t msdp_json_serialize_table(struct msdp_json_writer *writer,
                                                  const unsigned char **cursor,
                                                  const unsigned char *end, int depth,
                                                  bool require_utf8)
{
  protocol_error_t result;
  bool first_entry;

  if (depth >= MSDP_JSON_MAX_DEPTH)
    return PROTOCOL_ERROR_INVALID_INPUT;

  (*cursor)++;
  result = msdp_json_append_char(writer, '{');
  if (result != PROTOCOL_SUCCESS)
    return result;

  first_entry = true;
  while (*cursor < end)
  {
    const unsigned char *key;

    if (**cursor == MSDP_TABLE_CLOSE)
    {
      (*cursor)++;
      return msdp_json_append_char(writer, '}');
    }
    if (**cursor != MSDP_VAR)
      return PROTOCOL_ERROR_INVALID_INPUT;

    (*cursor)++;
    key = *cursor;
    while (*cursor < end && **cursor != MSDP_VAL)
    {
      if (msdp_json_is_reserved(**cursor))
        return PROTOCOL_ERROR_INVALID_INPUT;
      (*cursor)++;
    }
    if (*cursor == end || *cursor == key)
      return PROTOCOL_ERROR_INVALID_INPUT;

    if (!first_entry)
    {
      result = msdp_json_append_char(writer, ',');
      if (result != PROTOCOL_SUCCESS)
        return result;
    }
    first_entry = false;

    result = msdp_json_append_quoted(writer, key, (size_t)(*cursor - key), require_utf8);
    if (result != PROTOCOL_SUCCESS)
      return result;
    result = msdp_json_append_char(writer, ':');
    if (result != PROTOCOL_SUCCESS)
      return result;

    (*cursor)++;
    if (*cursor < end && (**cursor == MSDP_TABLE_OPEN || **cursor == MSDP_ARRAY_OPEN))
    {
      result = msdp_json_serialize_value(writer, cursor, end, depth + 1, require_utf8);
    }
    else
    {
      result =
          msdp_json_serialize_scalar(writer, cursor, end, MSDP_VAR, MSDP_TABLE_CLOSE, require_utf8);
    }
    if (result != PROTOCOL_SUCCESS)
      return result;

    if (*cursor == end || (**cursor != MSDP_VAR && **cursor != MSDP_TABLE_CLOSE))
      return PROTOCOL_ERROR_INVALID_INPUT;
  }

  return PROTOCOL_ERROR_INVALID_INPUT;
}

static protocol_error_t msdp_json_serialize_array(struct msdp_json_writer *writer,
                                                  const unsigned char **cursor,
                                                  const unsigned char *end, int depth,
                                                  bool require_utf8)
{
  protocol_error_t result;
  bool first_entry;

  if (depth >= MSDP_JSON_MAX_DEPTH)
    return PROTOCOL_ERROR_INVALID_INPUT;

  (*cursor)++;
  result = msdp_json_append_char(writer, '[');
  if (result != PROTOCOL_SUCCESS)
    return result;

  first_entry = true;
  while (*cursor < end)
  {
    if (**cursor == MSDP_ARRAY_CLOSE)
    {
      (*cursor)++;
      return msdp_json_append_char(writer, ']');
    }
    if (**cursor != MSDP_VAL)
      return PROTOCOL_ERROR_INVALID_INPUT;

    (*cursor)++;
    if (!first_entry)
    {
      result = msdp_json_append_char(writer, ',');
      if (result != PROTOCOL_SUCCESS)
        return result;
    }
    first_entry = false;

    if (*cursor < end && (**cursor == MSDP_TABLE_OPEN || **cursor == MSDP_ARRAY_OPEN))
    {
      result = msdp_json_serialize_value(writer, cursor, end, depth + 1, require_utf8);
    }
    else
    {
      result =
          msdp_json_serialize_scalar(writer, cursor, end, MSDP_VAL, MSDP_ARRAY_CLOSE, require_utf8);
    }
    if (result != PROTOCOL_SUCCESS)
      return result;

    if (*cursor == end || (**cursor != MSDP_VAL && **cursor != MSDP_ARRAY_CLOSE))
      return PROTOCOL_ERROR_INVALID_INPUT;
  }

  return PROTOCOL_ERROR_INVALID_INPUT;
}

static protocol_error_t msdp_json_serialize_value(struct msdp_json_writer *writer,
                                                  const unsigned char **cursor,
                                                  const unsigned char *end, int depth,
                                                  bool require_utf8)
{
  if (writer == NULL || cursor == NULL || *cursor == NULL || end == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;
  if (*cursor >= end)
    return PROTOCOL_ERROR_INVALID_INPUT;

  if (**cursor == MSDP_TABLE_OPEN)
    return msdp_json_serialize_table(writer, cursor, end, depth, require_utf8);
  if (**cursor == MSDP_ARRAY_OPEN)
    return msdp_json_serialize_array(writer, cursor, end, depth, require_utf8);

  return PROTOCOL_ERROR_INVALID_INPUT;
}

static protocol_error_t msdp_json_value_length(const char *value, size_t *length)
{
  if (value == NULL || length == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  *length = strnlen(value, MAX_VARIABLE_LENGTH + 1);
  if (*length > MAX_VARIABLE_LENGTH)
    return PROTOCOL_ERROR_BUFFER_FULL;

  return PROTOCOL_SUCCESS;
}

static protocol_error_t msdp_json_write_value(struct msdp_json_writer *writer, const char *value,
                                              bool require_utf8)
{
  const unsigned char *cursor;
  const unsigned char *end;
  protocol_error_t result;
  size_t length;

  result = msdp_json_value_length(value, &length);
  if (result != PROTOCOL_SUCCESS)
    return result;

  cursor = (const unsigned char *)value;
  end = cursor + length;
  if (cursor < end && (*cursor == MSDP_TABLE_OPEN || *cursor == MSDP_ARRAY_OPEN))
  {
    result = msdp_json_serialize_value(writer, &cursor, end, 0, require_utf8);
  }
  else
  {
    result = msdp_json_serialize_scalar(writer, &cursor, end, 0, 0, require_utf8);
  }

  if (result != PROTOCOL_SUCCESS)
    return result;
  if (cursor != end)
    return PROTOCOL_ERROR_INVALID_INPUT;

  return PROTOCOL_SUCCESS;
}

protocol_error_t msdp_json_validate_name(const char *name)
{
  struct msdp_json_writer writer;
  size_t length;

  if (name == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  length = strnlen(name, MAX_MSDP_SIZE + 1);
  if (length == 0)
    return PROTOCOL_ERROR_INVALID_INPUT;
  if (length > MAX_MSDP_SIZE)
    return PROTOCOL_ERROR_BUFFER_FULL;

  writer.buffer = NULL;
  writer.capacity = SIZE_MAX;
  writer.length = 0;
  return msdp_json_append_quoted(&writer, (const unsigned char *)name, length, true);
}

protocol_error_t msdp_json_validate_scalar(const char *value)
{
  struct msdp_json_writer writer;
  const unsigned char *cursor;
  const unsigned char *end;
  protocol_error_t result;
  size_t length;

  result = msdp_json_value_length(value, &length);
  if (result != PROTOCOL_SUCCESS)
    return result;

  writer.buffer = NULL;
  writer.capacity = SIZE_MAX;
  writer.length = 0;
  cursor = (const unsigned char *)value;
  end = cursor + length;
  result = msdp_json_serialize_scalar(&writer, &cursor, end, 0, 0, false);
  if (result != PROTOCOL_SUCCESS)
    return result;

  return cursor == end ? PROTOCOL_SUCCESS : PROTOCOL_ERROR_INVALID_INPUT;
}

protocol_error_t msdp_json_validate_structured(const char *value)
{
  struct msdp_json_writer writer;
  const unsigned char *cursor;
  const unsigned char *end;
  protocol_error_t result;
  size_t length;

  result = msdp_json_value_length(value, &length);
  if (result != PROTOCOL_SUCCESS)
    return result;
  if (length == 0 || (value[0] != MSDP_TABLE_OPEN && value[0] != MSDP_ARRAY_OPEN))
    return PROTOCOL_ERROR_INVALID_INPUT;

  writer.buffer = NULL;
  writer.capacity = SIZE_MAX;
  writer.length = 0;
  cursor = (const unsigned char *)value;
  end = cursor + length;
  result = msdp_json_serialize_value(&writer, &cursor, end, 0, false);
  if (result != PROTOCOL_SUCCESS)
    return result;

  return cursor == end ? PROTOCOL_SUCCESS : PROTOCOL_ERROR_INVALID_INPUT;
}

static protocol_error_t msdp_json_begin_frame(struct msdp_json_writer *writer, const char *variable)
{
  const unsigned char prefix[] = {IAC, SB, TELOPT_GMCP};
  protocol_error_t result;
  size_t variable_length;

  result = msdp_json_validate_name(variable);
  if (result != PROTOCOL_SUCCESS)
    return result;
  variable_length = strlen(variable);

  result = msdp_json_append(writer, prefix, sizeof(prefix));
  if (result != PROTOCOL_SUCCESS)
    return result;
  result = msdp_json_append(writer, "MSDP {", strlen("MSDP {"));
  if (result != PROTOCOL_SUCCESS)
    return result;
  result = msdp_json_append_quoted(writer, (const unsigned char *)variable, variable_length, true);
  if (result != PROTOCOL_SUCCESS)
    return result;

  return msdp_json_append_char(writer, ':');
}

static protocol_error_t msdp_json_finish_frame(struct msdp_json_writer *writer,
                                               size_t *frame_length)
{
  const unsigned char suffix[] = {'}', IAC, SE};
  protocol_error_t result;

  if (writer == NULL || frame_length == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  result = msdp_json_append(writer, suffix, sizeof(suffix));
  if (result != PROTOCOL_SUCCESS)
    return result;

  *frame_length = writer->length;
  return PROTOCOL_SUCCESS;
}

protocol_error_t msdp_json_build_string_frame(char *frame, size_t capacity, const char *variable,
                                              const char *value, size_t *frame_length)
{
  struct msdp_json_writer writer;
  protocol_error_t result;

  if (frame == NULL || variable == NULL || value == NULL || frame_length == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  writer.buffer = frame;
  writer.capacity = capacity;
  writer.length = 0;
  if (capacity > 0)
    frame[0] = '\0';

  result = msdp_json_begin_frame(&writer, variable);
  if (result != PROTOCOL_SUCCESS)
    return result;
  result = msdp_json_write_value(&writer, value, true);
  if (result != PROTOCOL_SUCCESS)
    return result;

  return msdp_json_finish_frame(&writer, frame_length);
}

protocol_error_t msdp_json_build_number_frame(char *frame, size_t capacity, const char *variable,
                                              int value, size_t *frame_length)
{
  struct msdp_json_writer writer;
  protocol_error_t result;
  char number[32];
  int written;

  if (frame == NULL || variable == NULL || frame_length == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  writer.buffer = frame;
  writer.capacity = capacity;
  writer.length = 0;
  if (capacity > 0)
    frame[0] = '\0';

  result = msdp_json_begin_frame(&writer, variable);
  if (result != PROTOCOL_SUCCESS)
    return result;

  written = snprintf(number, sizeof(number), "%d", value);
  if (written < 0 || (size_t)written >= sizeof(number))
    return PROTOCOL_ERROR_BUFFER_FULL;
  result = msdp_json_append(&writer, number, (size_t)written);
  if (result != PROTOCOL_SUCCESS)
    return result;

  return msdp_json_finish_frame(&writer, frame_length);
}

protocol_error_t msdp_json_build_list_frame(char *frame, size_t capacity, const char *variable,
                                            const char *values, size_t *frame_length)
{
  struct msdp_json_writer writer;
  struct msdp_json_writer validator;
  const unsigned char *cursor;
  const unsigned char *end;
  protocol_error_t result;
  bool first_value;
  size_t value_length;

  if (frame == NULL || variable == NULL || values == NULL || frame_length == NULL)
    return PROTOCOL_ERROR_NULL_POINTER;

  result = msdp_json_value_length(values, &value_length);
  if (result != PROTOCOL_SUCCESS)
    return result;
  validator.buffer = NULL;
  validator.capacity = SIZE_MAX;
  validator.length = 0;
  result = msdp_json_append_quoted(&validator, (const unsigned char *)values, value_length, true);
  if (result != PROTOCOL_SUCCESS)
    return result;

  writer.buffer = frame;
  writer.capacity = capacity;
  writer.length = 0;
  if (capacity > 0)
    frame[0] = '\0';

  result = msdp_json_begin_frame(&writer, variable);
  if (result != PROTOCOL_SUCCESS)
    return result;
  result = msdp_json_append_char(&writer, '[');
  if (result != PROTOCOL_SUCCESS)
    return result;

  cursor = (const unsigned char *)values;
  end = cursor + value_length;
  first_value = true;
  while (cursor < end)
  {
    const unsigned char *start;

    while (cursor < end && *cursor == ' ')
      cursor++;
    if (cursor == end)
      break;

    start = cursor;
    while (cursor < end && *cursor != ' ')
      cursor++;

    if (!first_value)
    {
      result = msdp_json_append_char(&writer, ',');
      if (result != PROTOCOL_SUCCESS)
        return result;
    }
    first_value = false;

    result = msdp_json_append_quoted(&writer, start, (size_t)(cursor - start), true);
    if (result != PROTOCOL_SUCCESS)
      return result;
  }

  result = msdp_json_append_char(&writer, ']');
  if (result != PROTOCOL_SUCCESS)
    return result;

  return msdp_json_finish_frame(&writer, frame_length);
}
