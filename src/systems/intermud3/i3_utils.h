/* ************************************************************************
*  Intermud3 Utilities for LuminariMUD                                   *
*  Simple validation and utility functions extracted from enhancements   *
************************************************************************ */

#ifndef _I3_UTILS_H_
#define _I3_UTILS_H_

/* Basic validation functions */
int i3_validate_username(const char *username);
int i3_validate_mudname(const char *mudname);
int i3_validate_message(const char *message);
int i3_validate_channel(const char *channel);

/* Simple sanitization */
void i3_sanitize_input(const char *input, char *output, size_t outsize);
void i3_remove_color_codes(const char *input, char *output, size_t outsize);

/* Basic string utilities */
int i3_safe_strncpy(char *dest, const char *src, size_t size);
char *i3_trim_whitespace(char *str);

/* Simple error helpers */
void i3_log_error(const char *function, const char *message);
void i3_log_debug(const char *function, const char *message);

/* I3-specific argument parsing functions */
const char *i3_one_argument(const char *argument, char *first_arg, size_t arg_size);
void i3_skip_spaces(const char **string);

#endif /* _I3_UTILS_H_ */