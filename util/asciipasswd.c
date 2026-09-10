/* ************************************************************************
*  file:  asciipasswd.c (derived from mudpasswd.c)    Part of LuminariMUD *
*  Usage: generating hashed passwords for an ascii playerfile.            *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
*                                                                         *
*  This utility generates hashed passwords that can be used in ASCII      *
*  player files. It takes a player name and plaintext password as         *
*  arguments and outputs the hash using the same scheme and policy as the *
*  main MUD server (see src/password.h).                                  *
************************************************************************* */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "password.h"

#include <crypt.h>

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
  char *encrypted_pass;

  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <name> <password>\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Generates password hashes for ASCII player files.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example: %s Gandalf mypassword\n", argv[0]);
    return (1);
  }

  /* Validate input parameters */
  if (!argv[1] || !*argv[1])
  {
    fprintf(stderr, "Error: Player name cannot be empty\n");
    return (1);
  }

  if (!argv[2] || !*argv[2])
  {
    fprintf(stderr, "Error: Password cannot be empty\n");
    return (1);
  }

  /* Generate the hash with a random salt under the server policy */
  memset(&data, 0, sizeof(data));
  if (!crypt_gensalt_rn(PASSWORD_HASH_PREFIX, PASSWORD_HASH_COST, NULL, 0, setting,
                        sizeof(setting)))
  {
    fprintf(stderr, "Error: Failed to generate a password salt\n");
    return (1);
  }
  encrypted_pass = crypt_rn(argv[2], setting, &data, sizeof(data));
  if (!encrypted_pass || *encrypted_pass == '*')
  {
    fprintf(stderr, "Error: Failed to hash password\n");
    return (1);
  }

  /* Output the results */
  printf("Name: %s\n", CAP(argv[1]));
  printf("Pass: %s\n", encrypted_pass);

  return (0);
}
