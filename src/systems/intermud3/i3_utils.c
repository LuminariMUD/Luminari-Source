/* ************************************************************************
*  Intermud3 Utilities for LuminariMUD                                   *
*  Simple validation and utility functions                               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "systems/intermud3/i3_utils.h"
#include <ctype.h>
#include <string.h>

/* Validate username - basic checks */
int i3_validate_username(const char *username)
{
    int len;
    
    if (!username || !*username) {
        return 0;
    }
    
    len = strlen(username);
    if (len < 1 || len > 20) {
        return 0;
    }
    
    /* Check for valid characters */
    while (*username) {
        if (!isalnum(*username) && *username != '_') {
            return 0;
        }
        username++;
    }
    
    return 1;
}

/* Validate MUD name - basic checks */
int i3_validate_mudname(const char *mudname)
{
    int len;
    
    if (!mudname || !*mudname) {
        return 0;
    }
    
    len = strlen(mudname);
    if (len < 1 || len > 50) {
        return 0;
    }
    
    return 1;
}

/* Validate message - basic length and content check */
int i3_validate_message(const char *message)
{
    int len;
    
    if (!message || !*message) {
        return 0;
    }
    
    len = strlen(message);
    if (len < 1 || len > 4000) {
        return 0;
    }
    
    return 1;
}

/* Validate channel name */
int i3_validate_channel(const char *channel)
{
    int len;
    
    if (!channel || !*channel) {
        return 0;
    }
    
    len = strlen(channel);
    if (len < 1 || len > 30) {
        return 0;
    }
    
    return 1;
}

/* Basic input sanitization */
void i3_sanitize_input(const char *input, char *output, size_t outsize)
{
    const char *src;
    char *dst;
    size_t count = 0;
    
    if (!input || !output || outsize < 1) {
        return;
    }
    
    src = input;
    dst = output;
    
    while (*src && count < outsize - 1) {
        /* Remove dangerous characters */
        if (*src == '\r' || *src == '\n' || *src == '\t') {
            *dst = ' ';
        } else if (isprint(*src)) {
            *dst = *src;
        } else {
            *dst = '?';
        }
        src++;
        dst++;
        count++;
    }
    
    *dst = '\0';
}

/* Remove color codes from text */
void i3_remove_color_codes(const char *input, char *output, size_t outsize)
{
    const char *src;
    char *dst;
    size_t count = 0;
    
    if (!input || !output || outsize < 1) {
        return;
    }
    
    src = input;
    dst = output;
    
    while (*src && count < outsize - 1) {
        if (*src == '\x1B' || *src == '^') {
            /* Skip escape sequences */
            src++;
            if (*src) src++; /* Skip next char too */
        } else {
            *dst = *src;
            dst++;
            count++;
        }
        if (*src) src++;
    }
    
    *dst = '\0';
}

/* Safe string copy */
int i3_safe_strncpy(char *dest, const char *src, size_t size)
{
    if (!dest || !src || size < 1) {
        return -1;
    }
    
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
    return 0;
}

/* Trim whitespace */
char *i3_trim_whitespace(char *str)
{
    char *end;
    
    if (!str) {
        return str;
    }
    
    /* Trim leading space */
    while (isspace(*str)) {
        str++;
    }
    
    if (*str == 0) {
        return str;
    }
    
    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        end--;
    }
    
    end[1] = '\0';
    return str;
}

/* Simple error logging */
void i3_log_error(const char *function, const char *message)
{
    log("I3 ERROR in %s: %s", function ? function : "unknown", message ? message : "unknown error");
}

/* I3-specific argument parsing functions to replace missing ones */

/* Extract first word from string - similar to one_argument() */
const char *i3_one_argument(const char *argument, char *first_arg, size_t arg_size)
{
    const char *ptr;
    char *dest;
    size_t len = 0;
    
    if (!argument || !first_arg || arg_size == 0) {
        if (first_arg && arg_size > 0) {
            *first_arg = '\0';
        }
        return argument;
    }
    
    /* Skip leading spaces */
    while (*argument && isspace(*argument)) {
        argument++;
    }
    
    ptr = argument;
    dest = first_arg;
    
    /* Copy first word */
    while (*ptr && !isspace(*ptr) && len < (arg_size - 1)) {
        *dest++ = *ptr++;
        len++;
    }
    
    *dest = '\0';
    
    /* Skip trailing spaces to find start of next argument */
    while (*ptr && isspace(*ptr)) {
        ptr++;
    }
    
    return ptr;
}

/* Skip leading spaces - similar to skip_spaces() */
void i3_skip_spaces(const char **string)
{
    if (!string || !*string) {
        return;
    }
    
    while (**string && isspace(**string)) {
        (*string)++;
    }
}

/* Simple debug logging */
void i3_log_debug(const char *function, const char *message)
{
    log("I3 DEBUG in %s: %s", function ? function : "unknown", message ? message : "no message");
}