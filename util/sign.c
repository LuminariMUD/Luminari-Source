/* ************************************************************************
*  file: sign.c                                       Part of LuminariMUD *
*  Usage: A program to present text on a TCP port.                        *
*         sign <port> <filename | port>                                   *
*  Written by Jeremy Elson                                                *
*                                                                         *
*  This utility creates a simple TCP server that serves text content      *
*  to connecting clients. It's useful for displaying information like     *
*  server status, news, or other text-based content that can be accessed  *
*  via telnet or similar tools.                                           *
*                                                                         *
*  The server forks for each connection to handle multiple clients        *
*  simultaneously. Text can be read from a file or from stdin.            *
*                                                                         *
*  Updated: 2025 - Enhanced for LuminariMUD compatibility, improved       *
*  error handling and documentation                                       *
************************************************************************* */

#define MAX_FILESIZE 8192
#define LINEBUF_SIZE 128

#include "conf.h"
#include "sysdep.h"

/* Function prototypes */
int init_socket(int port);
char *get_text(char *fname);
RETSIGTYPE reap(int sig);

/**
 * Initialize and configure the server socket
 *
 * Creates a TCP socket, sets socket options for reuse, binds it to the
 * specified port, and starts listening for connections.
 *
 * @param port Port number to bind to (must be >= 1024 for non-root users)
 * @return Socket file descriptor
 */
int init_socket(int port)
{
  int s, opt;
  struct sockaddr_in sa;

  /* Create TCP socket */
  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Create socket");
    exit(1);
  }
#if defined(SO_REUSEADDR)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
  {
    perror("setsockopt REUSEADDR");
    exit(1);
  }
#endif

#if defined(SO_LINGER)
  {
    struct linger ld;

    ld.l_onoff = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld)) < 0)
    {
      perror("setsockopt LINGER");
      exit(1);
    }
  }
#endif

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
  {
    perror("bind");
    close(s);
    exit(1);
  }
  listen(s, 5);
  return (s);
}

/**
 * Read text content from file or stdin
 *
 * Reads text content that will be served to connecting clients.
 * Can read from a specified file or from stdin if "-" is given.
 * Adds carriage returns for proper telnet display.
 *
 * @param fname Filename to read from, or "-" for stdin
 * @return Pointer to static buffer containing the text
 */
char *get_text(char *fname)
{
  static char t[MAX_FILESIZE];
  char tmp[LINEBUF_SIZE + 2];
  FILE *fl = NULL;

  *t = '\0';

  if (!strcmp(fname, "-"))
  {
    fl = stdin;
    if (isatty(STDIN_FILENO))
      fprintf(stderr, "Enter sign text; terminate with Ctrl-D.\n");
  }
  else
  {
    if (!(fl = fopen(fname, "r")))
    {
      perror(fname);
      exit(1);
    }
  }

  while (fgets(tmp, LINEBUF_SIZE, fl))
  {
    if (strlen(tmp) + strlen(t) < MAX_FILESIZE - 1)
    {
      strcat(t, tmp);
      /* Add carriage return for telnet compatibility */
      if (tmp[strlen(tmp) - 1] == '\n')
      {
        t[strlen(t) - 1] = '\0';  /* Remove \n */
        strcat(t, "\r\n");        /* Add \r\n */
      }
    }
    else
    {
      fprintf(stderr, "Warning: Text content too long, truncated at %d bytes.\n", MAX_FILESIZE);
      break;
    }
  }

  if (fl != stdin)
    fclose(fl);

  return (t);
}

/**
 * Signal handler to clean up zombie child processes
 *
 * Called when a child process terminates. Reaps all available
 * zombie processes to prevent them from becoming defunct.
 *
 * @param sig Signal number (SIGCHLD)
 */
RETSIGTYPE reap(int sig)
{
  /* Reap all available zombie children */
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  /* Reinstall signal handler */
  signal(SIGCHLD, reap);
}

/**
 * Main function for the sign TCP server
 *
 * Creates a TCP server that serves text content to connecting clients.
 * The server forks into the background and handles multiple connections
 * by forking a child process for each client.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments:
 *             [1] port number (must be >= 1024)
 *             [2] filename to serve, or "-" for stdin
 * @return 0 on success, 1 on error
 */
int main(int argc, char *argv[])
{
  char *txt;
  int desc, remaining, bytes_written, len, s, port, child;

  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <portnum> <filename | \"-\">\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Creates a TCP server that serves text content.\n");
    fprintf(stderr, "portnum  - TCP port to listen on (must be >= 1024)\n");
    fprintf(stderr, "filename - file to serve, or \"-\" to read from stdin\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example: %s 8080 welcome.txt\n", argv[0]);
    fprintf(stderr, "Example: %s 8080 - < message.txt\n", argv[0]);
    exit(1);
  }

  port = atoi(argv[1]);
  if (port < 1024)
  {
    fprintf(stderr, "Error: Port number must be >= 1024\n");
    exit(1);
  }

  /* Initialize socket and get text content */
  s = init_socket(port);
  txt = get_text(argv[2]);
  len = strlen(txt);

  if (len == 0)
  {
    fprintf(stderr, "Warning: No text content to serve\n");
  }

  /* Fork into background */
  if ((child = fork()) > 0)
  {
    fprintf(stderr, "Sign server started on port %d (pid %d).\n", port, child);
    fprintf(stderr, "Serving %d bytes of text content.\n", len);
    fprintf(stderr, "Connect with: telnet localhost %d\n", port);
    exit(0);
  }

  /* Set up signal handler for child processes */
  signal(SIGCHLD, reap);

  for (;;)
  {
    if ((desc = accept(s, (struct sockaddr *)NULL, 0)) < 0)
      continue;

    if (fork() == 0)
    {
      remaining = len;
      do
      {
        if ((bytes_written = write(desc, txt, remaining)) < 0)
          exit(0);
        else
        {
          txt += bytes_written;
          remaining -= bytes_written;
        }
      } while (remaining > 0);
      exit(0);
    }
    close(desc);
  }
}
