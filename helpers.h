#pragma once

#include <stdbool.h>
#include <stddef.h>

/** If c is an upper case letter, return the lower case. */
#define LOWER(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + ('a' - 'A')) : (c))

/** If c is a lower case letter, return the upper case. */
#define UPPER(c) (((c) >= 'a' && (c) <= 'z') ? ((c) + ('A' - 'a')) : (c))

/* Function to skip over the leading spaces of a string. */
void skip_spaces(char **string);
void skip_spaces_c(const char **string);

/* Parse out the @ character and replace it with the '\t' to work with
 * KaVir's protocol snippet */
void parse_at(char *str);

/* Return first space-delimited token in arg1; remainder of string in arg2.
 * NOTE: Requires sizeof(arg2) >= sizeof(string) */
void half_chop(char *string, char *arg1, char *arg2);
void half_chop_c(const char *string, char *arg1, size_t n1, char *arg2, size_t n2);

/* Same as one_argument except that it doesn't ignore fill words. */
char *any_one_arg(char *argument, char *first_arg);
const char *any_one_arg_c(const char *argument, char *first_arg, size_t n);

/* Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words */
char *two_arguments(char *argument, char *first_arg, char *second_arg);

/* Searches an array of strings for a target string.  "exact" can be 0 or non-0,
 * depending on whether or not the match must be exact for it to be returned.
 * Returns -1 if not found; 0..n otherwise.  Array must be terminated with a
 * '\n' so it knows to stop searching. */
int search_block(char *arg, const char * const *list, bool exact);

int fill_word(char *argument);

/* Copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string. */
char *one_argument(char *argument, char *first_arg);
const char *one_argument_c(const char *argument, char *first_arg, size_t n);

extern const char *fill[];

bool legal_communication(char *arg);

void sentence_case(char *str);
