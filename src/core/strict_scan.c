/**
 * @file strict_scan.c
 * sscanf and fscanf with numbers that cannot overflow.
 *
 * A strict scan reads the formats sscanf and fscanf read and gives the same results, except
 * that each number is converted with the strto* functions and one outside the range of its
 * target type ends the scan as a matching failure, where scanf's behaviour is undefined. The
 * directives covered are the ones LuminariMUD's formats use; an unknown directive also ends
 * the scan. Both entry points share one engine over a string or a stream, so a stream keeps
 * scanf's one-character pushback.
 */

#include "conf.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strict_scan.h"

/* What a strict scan reads: a string, or a stream one character at a time. */
struct strict_scan_input
{
  const char *text;
  size_t position;
  FILE *stream;
  size_t consumed;
};

static int strict_scan_peek(struct strict_scan_input *in)
{
  int c;

  if (in->stream == NULL)
    return in->text[in->position] != '\0' ? (unsigned char)in->text[in->position] : EOF;
  c = getc(in->stream);
  if (c != EOF)
    ungetc(c, in->stream);
  return c;
}

static void strict_scan_advance(struct strict_scan_input *in)
{
  if (in->stream == NULL)
    in->position++;
  else
    (void)getc(in->stream);
  in->consumed++;
}

static void strict_scan_skip_space(struct strict_scan_input *in)
{
  int c;

  while ((c = strict_scan_peek(in)) != EOF && isspace(c))
    strict_scan_advance(in);
}

/* Copy the longest prefix of the input that can form a number in base 8, 10, or 16 (with an
 * optional sign and, in base 16, an optional 0x prefix) into buffer, reading at most width
 * characters when width is not 0. Base 0 takes the base from the prefix, as strtol does, and
 * reports it through *base. Returns the number of digits copied. */
static size_t strict_scan_integer_text(struct strict_scan_input *in, int *base, size_t width,
                                       char *buffer, size_t size)
{
  size_t length = 0;
  size_t digits = 0;
  size_t limit = width > 0 && width < size ? width : size - 1;
  int c;

  c = strict_scan_peek(in);
  if (length < limit && (c == '+' || c == '-'))
  {
    buffer[length++] = (char)c;
    strict_scan_advance(in);
  }
  if ((*base == 16 || *base == 0) && length < limit && strict_scan_peek(in) == '0')
  {
    buffer[length++] = '0';
    strict_scan_advance(in);
    digits++;
    c = strict_scan_peek(in);
    if (length < limit && (c == 'x' || c == 'X'))
    {
      buffer[length++] = (char)c;
      strict_scan_advance(in);
      *base = 16;
      c = strict_scan_peek(in);
      if (length < limit && c != EOF && isxdigit(c))
        digits = 0;
      else
      {
        /* "0x" with no digit after it reads as 0, the x consumed, as scanf does. */
        buffer[length - 1] = '\0';
        return digits;
      }
    }
    else if (*base == 0)
      *base = 8;
  }
  if (*base == 0)
    *base = 10;
  while (length < limit)
  {
    c = strict_scan_peek(in);
    if (c == EOF || !(*base == 16 ? isxdigit(c) : *base == 8 ? (c >= '0' && c <= '7') : isdigit(c)))
      break;
    buffer[length++] = (char)c;
    strict_scan_advance(in);
    digits++;
  }
  buffer[length] = '\0';
  return digits;
}

/* The same for a floating-point number: sign, digits, a point, digits, and an exponent. */
static size_t strict_scan_float_text(struct strict_scan_input *in, size_t width, char *buffer,
                                     size_t size)
{
  size_t length = 0;
  size_t digits = 0;
  size_t limit = width > 0 && width < size ? width : size - 1;
  int c;

  c = strict_scan_peek(in);
  if (length < limit && (c == '+' || c == '-'))
  {
    buffer[length++] = (char)c;
    strict_scan_advance(in);
  }
  while (length < limit && (c = strict_scan_peek(in)) != EOF && isdigit(c))
  {
    buffer[length++] = (char)c;
    strict_scan_advance(in);
    digits++;
  }
  if (length < limit && strict_scan_peek(in) == '.')
  {
    buffer[length++] = '.';
    strict_scan_advance(in);
    while (length < limit && (c = strict_scan_peek(in)) != EOF && isdigit(c))
    {
      buffer[length++] = (char)c;
      strict_scan_advance(in);
      digits++;
    }
  }
  if (digits > 0 && length < limit && ((c = strict_scan_peek(in)) == 'e' || c == 'E'))
  {
    buffer[length++] = (char)c;
    strict_scan_advance(in);
    c = strict_scan_peek(in);
    if (length < limit && (c == '+' || c == '-'))
    {
      buffer[length++] = (char)c;
      strict_scan_advance(in);
    }
    while (length < limit && (c = strict_scan_peek(in)) != EOF && isdigit(c))
    {
      buffer[length++] = (char)c;
      strict_scan_advance(in);
    }
  }
  buffer[length] = '\0';
  return digits;
}

/* Whether c belongs to the scanset that starts just after '[' at set and ends at end. */
static bool strict_scan_in_set(const char *set, const char *end, int c)
{
  bool negate = false;
  bool found = false;
  const char *p = set;

  if (*p == '^')
  {
    negate = true;
    p++;
  }
  if (p < end && *p == ']')
  {
    found = c == ']';
    p++;
  }
  for (; p < end; p++)
  {
    if (p + 2 < end && p[1] == '-')
    {
      if (c >= (unsigned char)p[0] && c <= (unsigned char)p[2])
        found = true;
      p += 2;
    }
    else if (c == (unsigned char)*p)
      found = true;
  }
  return found != negate;
}

/* sscanf and fscanf semantics for the directives the code base uses (whitespace, literal
 * characters, %%, %d %i %u %o %x with hh/h/l/ll/j/z/t, %f %e %g %a with l/L, %s, %c, %[...],
 * %n, widths, and '*'), except that a number outside its target type is a matching failure
 * instead of undefined behaviour. Returns the number of assignments, or EOF when the input
 * ends before anything was assigned, as glibc does. */
static int strict_vscan(struct strict_scan_input *in, const char *format, va_list args)
{
  int assigned = 0;
  const char *f = format;

  while (*f != '\0')
  {
    bool suppress = false;
    size_t width = 0;
    char length = '\0';
    char conversion;
    int c;

    if (isspace((unsigned char)*f))
    {
      while (isspace((unsigned char)*f))
        f++;
      strict_scan_skip_space(in);
      continue;
    }
    if (*f != '%' || f[1] == '%')
    {
      if (*f == '%')
      {
        f++;
        strict_scan_skip_space(in);
      }
      c = strict_scan_peek(in);
      if (c == EOF)
        return assigned > 0 ? assigned : EOF;
      if (c != (unsigned char)*f)
        return assigned;
      strict_scan_advance(in);
      f++;
      continue;
    }
    f++;
    if (*f == '*')
    {
      suppress = true;
      f++;
    }
    while (isdigit((unsigned char)*f))
      width = width * 10 + (size_t)(*f++ - '0');
    if (*f == 'h' || *f == 'l')
    {
      length = *f++;
      if (*f == length)
      {
        length = (char)(length == 'h' ? 'H' : 'q'); /* hh and ll */
        f++;
      }
    }
    else if (*f == 'j' || *f == 'z' || *f == 't' || *f == 'L')
      length = *f++;
    conversion = *f++;

    switch (conversion)
    {
    case 'n':
      if (!suppress)
      {
        if (length == 'l')
          *va_arg(args, long *) = (long)in->consumed;
        else if (length == 'q')
          *va_arg(args, long long *) = (long long)in->consumed;
        else
          *va_arg(args, int *) = (int)in->consumed;
      }
      break;

    case 'c':
    {
      char *out = suppress ? NULL : va_arg(args, char *);
      size_t count = width > 0 ? width : 1;
      size_t i;

      for (i = 0; i < count; i++)
      {
        c = strict_scan_peek(in);
        if (c == EOF)
          break;
        if (out != NULL)
          out[i] = (char)c;
        strict_scan_advance(in);
      }
      /* Characters short of the width still count, as in scanf; none at all is an input
       * failure. */
      if (i == 0)
        return assigned > 0 ? assigned : EOF;
      if (!suppress)
        assigned++;
      break;
    }

    case 's':
    case '[':
    {
      char *out = suppress ? NULL : va_arg(args, char *);
      const char *set = f;
      const char *set_end = f;
      size_t count = 0;

      if (conversion == '[')
      {
        if (*set_end == '^')
          set_end++;
        if (*set_end == ']')
          set_end++;
        while (*set_end != '\0' && *set_end != ']')
          set_end++;
        f = *set_end == ']' ? set_end + 1 : set_end;
      }
      else
        strict_scan_skip_space(in);
      while (width == 0 || count < width)
      {
        c = strict_scan_peek(in);
        if (c == EOF || (conversion == 's' ? isspace(c) : !strict_scan_in_set(set, set_end, c)))
          break;
        if (out != NULL)
          out[count] = (char)c;
        count++;
        strict_scan_advance(in);
      }
      if (count == 0)
        return strict_scan_peek(in) == EOF && assigned == 0 ? EOF : assigned;
      if (out != NULL)
        out[count] = '\0';
      if (!suppress)
        assigned++;
      break;
    }

    case 'd':
    case 'i':
    case 'u':
    case 'o':
    case 'x':
    case 'X':
    {
      char text[128];
      char *end;
      int base = conversion == 'o' ? 8 : conversion == 'x' || conversion == 'X' ? 16 : 10;
      bool negative;
      unsigned long long magnitude;
      long long value;

      strict_scan_skip_space(in);
      if (strict_scan_peek(in) == EOF)
        return assigned > 0 ? assigned : EOF;
      if (conversion == 'i')
        base = 0;
      if (strict_scan_integer_text(in, &base, width, text, sizeof(text)) == 0)
        return assigned;
      negative = text[0] == '-';
      errno = 0;
      magnitude = strtoull(text + (text[0] == '+' || text[0] == '-'), &end, base);
      if (errno == ERANGE || *end != '\0')
        return assigned;

      if (conversion == 'd' || conversion == 'i')
      {
        long long low;
        long long high;

        switch (length)
        {
        case 'H':
          low = SCHAR_MIN;
          high = SCHAR_MAX;
          break;
        case 'h':
          low = SHRT_MIN;
          high = SHRT_MAX;
          break;
        case 'l':
          low = LONG_MIN;
          high = LONG_MAX;
          break;
        case 'q':
        case 'j':
          low = LLONG_MIN;
          high = LLONG_MAX;
          break;
        case 'z':
        case 't':
          low = PTRDIFF_MIN;
          high = PTRDIFF_MAX;
          break;
        default:
          low = INT_MIN;
          high = INT_MAX;
          break;
        }
        if (negative ? magnitude > (unsigned long long)-(low + 1) + 1ULL
                     : magnitude > (unsigned long long)high)
          return assigned;
        value = negative ? (long long)(0ULL - magnitude) : (long long)magnitude;
        if (suppress)
          break;
        switch (length)
        {
        case 'H':
          *va_arg(args, signed char *) = (signed char)value;
          break;
        case 'h':
          *va_arg(args, short *) = (short)value;
          break;
        case 'l':
          *va_arg(args, long *) = (long)value;
          break;
        case 'q':
          *va_arg(args, long long *) = value;
          break;
        case 'j':
          *va_arg(args, intmax_t *) = (intmax_t)value;
          break;
        case 'z':
        case 't':
          *va_arg(args, ptrdiff_t *) = (ptrdiff_t)value;
          break;
        default:
          *va_arg(args, int *) = (int)value;
          break;
        }
      }
      else
      {
        unsigned long long high;
        unsigned long long result;

        switch (length)
        {
        case 'H':
          high = UCHAR_MAX;
          break;
        case 'h':
          high = USHRT_MAX;
          break;
        case 'l':
          high = ULONG_MAX;
          break;
        case 'q':
        case 'j':
          high = ULLONG_MAX;
          break;
        case 'z':
        case 't':
          high = SIZE_MAX;
          break;
        default:
          high = UINT_MAX;
          break;
        }
        /* A minus sign negates in the target type, as scanf does ("-1" is UINT_MAX). */
        if (magnitude > high)
          return assigned;
        result = negative ? (0ULL - magnitude) & high : magnitude;
        if (suppress)
          break;
        switch (length)
        {
        case 'H':
          *va_arg(args, unsigned char *) = (unsigned char)result;
          break;
        case 'h':
          *va_arg(args, unsigned short *) = (unsigned short)result;
          break;
        case 'l':
          *va_arg(args, unsigned long *) = (unsigned long)result;
          break;
        case 'q':
          *va_arg(args, unsigned long long *) = result;
          break;
        case 'j':
          *va_arg(args, uintmax_t *) = (uintmax_t)result;
          break;
        case 'z':
        case 't':
          *va_arg(args, size_t *) = (size_t)result;
          break;
        default:
          *va_arg(args, unsigned int *) = (unsigned int)result;
          break;
        }
      }
      assigned += suppress ? 0 : 1;
      break;
    }

    case 'f':
    case 'e':
    case 'g':
    case 'E':
    case 'G':
    case 'a':
    case 'A':
    {
      char text[128];
      char *end;
      long double value;

      strict_scan_skip_space(in);
      if (strict_scan_peek(in) == EOF)
        return assigned > 0 ? assigned : EOF;
      if (strict_scan_float_text(in, width, text, sizeof(text)) == 0)
        return assigned;
      errno = 0;
      value = strtold(text, &end);
      /* An exponent marker with no digits ("1e", "1e-") is consumed and ignored, as scanf does. */
      if ((*end != '\0' && ((*end != 'e' && *end != 'E') ||
                            strspn(end + 1, "+-") != strlen(end + 1) || strlen(end + 1) > 1)) ||
          (errno == ERANGE && isinf(value)))
        return assigned;
      /* Converting a value outside the target type's range is undefined, so it fails too. */
      if ((length == 'l' && (value > (long double)DBL_MAX || value < -(long double)DBL_MAX)) ||
          (length != 'l' && length != 'L' &&
           (value > (long double)FLT_MAX || value < -(long double)FLT_MAX)))
        return assigned;
      if (suppress)
        break;
      if (length == 'l')
        *va_arg(args, double *) = (double)value;
      else if (length == 'L')
        *va_arg(args, long double *) = value;
      else
        *va_arg(args, float *) = (float)value;
      assigned++;
      break;
    }

    default:
      /* A directive this scanner does not know: stop, as a matching failure. */
      return assigned;
    }
  }
  return assigned;
}

int strict_sscanf(const char *input, const char *format, ...)
{
  struct strict_scan_input in = {input != NULL ? input : "", 0, NULL, 0};
  va_list args;
  int result;

  va_start(args, format);
  result = strict_vscan(&in, format, args);
  va_end(args);
  return result;
}

int strict_fscanf(FILE *stream, const char *format, ...)
{
  struct strict_scan_input in = {NULL, 0, stream, 0};
  va_list args;
  int result;

  if (stream == NULL)
    return EOF;
  va_start(args, format);
  result = strict_vscan(&in, format, args);
  va_end(args);
  return result;
}
