/* ************************************************************************
*  file:  asciipasswd.c (derived from mudpasswd.c)    Part of LuminariMUD *
*  Usage: generating hashed passwords for an ascii playerfile.            *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
*                                                                         *
*  This utility generates encrypted passwords that can be used in ASCII   *
*  player files. It takes a player name and plaintext password as         *
*  arguments and outputs the encrypted password using the same encryption *
*  method used by the main MUD server.                                    *
*                                                                         *
*  Updated: 2025 - Enhanced for LuminariMUD compatibility                 *
************************************************************************* */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

/**
 * Capitalize the first character of a string
 *
 * @param txt The string to capitalize (modified in place)
 * @return Pointer to the modified string
 */
char *CAP(char *txt)
{
  if (txt && *txt) {
    *txt = UPPER(*txt);
  }
  return (txt);
}

/**
 * Main function for the asciipasswd utility
 *
 * Generates encrypted passwords for ASCII player files using the same
 * encryption method as the main MUD server. The password is encrypted
 * using the player name as the salt.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return 0 on success, 1 on error
 */
int main(int argc, char **argv)
{
  char *encrypted_pass;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <name> <password>\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Generates encrypted passwords for ASCII player files.\n");
    fprintf(stderr, "The name is used as the salt for encryption.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example: %s Gandalf mypassword\n", argv[0]);
    return (1);
  }

  /* Validate input parameters */
  if (!argv[1] || !*argv[1]) {
    fprintf(stderr, "Error: Player name cannot be empty\n");
    return (1);
  }

  if (!argv[2] || !*argv[2]) {
    fprintf(stderr, "Error: Password cannot be empty\n");
    return (1);
  }

  /* Generate encrypted password */
  encrypted_pass = CRYPT(argv[2], CAP(argv[1]));
  if (!encrypted_pass) {
    fprintf(stderr, "Error: Failed to encrypt password\n");
    return (1);
  }

  /* Output the results */
  printf("Name: %s\n", CAP(argv[1]));
  printf("Pass: %s\n", encrypted_pass);

  return (0);
}
