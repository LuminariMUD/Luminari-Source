/* ************************************************************************
*  file:  asciipasswd.c (derived from mudpasswd.c)    Part of LuminariMUD *
*  Usage: generating hashed passwords for an ascii playerfile.            *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
*                                                                         *
*  This utility generates hashed passwords that can be used in ASCII      *
*  player files. It takes a player name as its argument, reads the        *
*  plaintext password from the terminal without echo (or from standard    *
*  input when that is not a terminal), and outputs the hash using the     *
*  same scheme and policy as the main MUD server (see src/password.h).    *
*  The password is never accepted on the command line, where it would be  *
*  visible in process listings and shell history.                         *
************************************************************************* */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "password.h"

#include <crypt.h>
#include <termios.h>

/**
 * Capitalize the first character of a string
 *
 * @param txt The string to capitalize (modified in place)
 * @return Pointer to the modified string
 */
char *CAP(char *txt)
{
  if (txt && *txt)
  {
    *txt = UPPER(*txt);
  }
  return (txt);
}

/**
 * Read one line of password input into buf without echoing it.
 *
 * When standard input is a terminal, echo is disabled for the duration of the
 * read and a prompt is written to standard error.  Otherwise the first line of
 * standard input is used as-is, so the password can be piped from a protected
 * source.  The trailing newline is removed.  A line longer than the buffer is
 * reported as a failure rather than silently truncated.
 *
 * @param buf  Destination buffer
 * @param size Size of buf in bytes
 * @return true on success, false when no password could be read
 */
static bool read_password(char *buf, size_t size)
{
  struct termios saved, quiet;
  bool is_tty = isatty(STDIN_FILENO);
  bool echo_off = false;
  size_t len;

  if (is_tty)
  {
    if (tcgetattr(STDIN_FILENO, &saved) == 0)
    {
      quiet = saved;
      quiet.c_lflag &= ~(tcflag_t)ECHO;
      echo_off = tcsetattr(STDIN_FILENO, TCSAFLUSH, &quiet) == 0;
    }
    fputs("Password: ", stderr);
    fflush(stderr);
  }

  if (!fgets(buf, (int)size, stdin))
  {
    buf[0] = '\0';
  }

  if (is_tty)
  {
    if (echo_off)
    {
      tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved);
    }
    fputc('\n', stderr);
  }

  len = strlen(buf);
  if (len == 0)
  {
    return (false);
  }
  if (buf[len - 1] == '\n')
  {
    buf[--len] = '\0';
  }
  else if (!feof(stdin))
  {
    /* The line did not fit; drain it so nothing leaks to a later reader. */
    int c;
    while ((c = getchar()) != EOF && c != '\n')
      ;
    return (false);
  }
  return (true);
}

/**
 * Main function for the asciipasswd utility
 *
 * Generates password hashes for ASCII player files using the same scheme
 * as the main MUD server, with a random salt per invocation.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return 0 on success, 1 on error
 */
int main(int argc, char **argv)
{
  struct crypt_data data;
  char setting[CRYPT_GENSALT_OUTPUT_SIZE];
  char password[MAX_PWD_LENGTH + 2]; /* room for the newline and terminator */
  char *encrypted_pass;
  size_t len;
  int rc = 1;

  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <name>\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Generates password hashes for ASCII player files.\n");
    fprintf(stderr, "The password is read from the terminal without echo, or from\n");
    fprintf(stderr, "standard input when it is not a terminal.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example: %s Gandalf\n", argv[0]);
    return (1);
  }

  /* Validate input parameters */
  if (!argv[1] || !*argv[1])
  {
    fprintf(stderr, "Error: Player name cannot be empty\n");
    return (1);
  }

  if (!read_password(password, sizeof(password)))
  {
    fprintf(stderr, "Error: Password cannot be empty or exceed %d characters\n", MAX_PWD_LENGTH);
    goto done;
  }

  len = strlen(password);
  if (len < MIN_PWD_LENGTH || len > MAX_PWD_LENGTH)
  {
    fprintf(stderr, "Error: Password must be between %d and %d characters\n", MIN_PWD_LENGTH,
            MAX_PWD_LENGTH);
    goto done;
  }

  /* Generate the hash with a random salt under the server policy */
  memset(&data, 0, sizeof(data));
  if (!crypt_gensalt_rn(PASSWORD_HASH_PREFIX, PASSWORD_HASH_COST, NULL, 0, setting,
                        sizeof(setting)))
  {
    fprintf(stderr, "Error: Failed to generate a password salt\n");
    goto done;
  }
  encrypted_pass = crypt_rn(password, setting, &data, sizeof(data));
  if (!encrypted_pass || *encrypted_pass == '*')
  {
    fprintf(stderr, "Error: Failed to hash password\n");
    goto done;
  }

  /* Output the results */
  printf("Name: %s\n", CAP(argv[1]));
  printf("Pass: %s\n", encrypted_pass);
  rc = 0;

done:
  explicit_bzero(password, sizeof(password));
  explicit_bzero(&data, sizeof(data));
  return (rc);
}
