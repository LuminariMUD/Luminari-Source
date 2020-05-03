/* Utilities that don't depend on the rest of the codebase */
#include "helpers.h"

#include <ctype.h>
#include <string.h>

/* Function to skip over the leading spaces of a string. */
void skip_spaces(char **string)
{
  for (; **string && **string != '\t' && isspace(**string); (*string)++)
    ;
}

void skip_spaces_c(const char **string)
{
  for (; **string && **string != '\t' && isspace(**string); (*string)++)
    ;
}

/* Parse out the @ character and replace it with the '\t' to work with
 * KaVir's protocol snippet */
void parse_at(char *str)
{
  if (!str)
    return;

  char *p = str;

  for (; *p; p++)
    if (*p == '@')
    {
      if (*(p + 1) != '@')
        *p = '\t';
      else
        p++;
    }
}

/* Return first space-delimited token in arg1; remainder of string in arg2.
 * NOTE: Requires sizeof(arg2) >= sizeof(string) */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp); /* strcpy: OK (documentation) */
}

void half_chop_c(const char *string, char *arg1, size_t n1, char *arg2, size_t n2)
{
  const char *temp = any_one_arg_c(string, arg1, n1);
  skip_spaces_c(&temp);
  strncpy(arg2, temp, n2 - 1);
  arg2[n2 - 1] = '\0';
}

/* Same as one_argument except that it doesn't ignore fill words. */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument))
  {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}

const char *any_one_arg_c(const char *argument, char *first_arg, size_t n)
{
  const char *arg_last = first_arg + n - 1;

  skip_spaces_c(&argument);
  
  while (*argument && !isspace(*argument))
  {
    if (first_arg < arg_last)
    {
      *(first_arg++) = LOWER(*argument);
    }
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}

/* Searches an array of strings for a target string.  "exact" can be 0 or non-0,
 * depending on whether or not the match must be exact for it to be returned.
 * Returns -1 if not found; 0..n otherwise.  Array must be terminated with a
 * '\n' so it knows to stop searching. */
int search_block(char *arg, const char * const *list, bool exact)
{
  int i, l;

  /*  We used to have \r as the first character on certain array items to
   *  prevent the explicit choice of that point.  It seems a bit silly to
   *  dump control characters into arrays to prevent that, so we'll just
   *  check in here to see if the first character of the argument is '!',
   *  and if so, just blindly return a '-1' for not found. - ae. */
  if (*arg == '!')
    return (-1);

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact)
  {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
        return (i);
  }
  else
  {
    if (!l)
      l = 1; /* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
        return (i);
  }

  return (-1);
}

const char *fill[] = {
    "in",
    "from",
    "with",
    "the",
    "on",
    "at",
    "to",
    "\n"};

int fill_word(char *argument)
{
  return (search_block(argument, fill, true) >= 0);
}

/* Copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string. */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument)
  {
    *first_arg = '\0';
    return (NULL);
  }

  do
  {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument))
    {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}

/* Copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string. */
const char *one_argument_c(const char *argument, char *first_arg, size_t n)
{
  char *begin = first_arg;
  const char *arg_last = first_arg + n - 1;

  if (!argument)
  {
    *first_arg = '\0';
    return (NULL);
  }

  do
  {
    skip_spaces_c(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument))
    {
      if (first_arg < arg_last)
      {
        *(first_arg++) = LOWER(*argument);
      }
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}

/* Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}

bool legal_communication(char *arg)
{
  while (*arg)
  {
    if (*arg == '@')
    {
      arg++;
      if (*arg == '(' || *arg == ')' || *arg == '<' || *arg == '>')
        return false;
    }
    arg++;
  }
  return true;
}

void sentence_case(char *str)
{
  char *p = str;
  bool cap_next = true;
  int len;

  // remove leading spaces
  while (*p == ' ' || *p == '\t' || *p == '\n')
    p++;

  len = strlen(p);

  // remove trailing spaces
  while (len >= 0 && (p[len - 1] == ' ' || p[len - 1] == '\t' || *p == '\n'))
  {
    *(p + len - 1) = '\0';
    len--;
  }

  for (; *p; p++)
  {
    while (strchr(".!?", *p) && *(p + 1) == ' ')
    {
      cap_next = false;
      p++;
    }

    if (cap_next && *p != ' ' && *p != '\t')
    {
      *p = UPPER(*p);
      cap_next = false;
    }
  }
}
