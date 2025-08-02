/* ************************************************************************
 *  File: webster.c                                    Part of LuminariMUD *
 *  Usage: Use an online dictionary via tell m-w <word>.                   *
 *                                                                         *
 *  Based on the Circle 3.0 syntax checker and wld2html programs.          *
 *                                                                         *
 *  This utility fetches dictionary definitions from an online source      *
 *  and formats them for display in the MUD. It uses lynx to download      *
 *  HTML content and parses it to extract definitions.                     *
 *                                                                         *
 *  Note: This utility depends on external tools (lynx) and online         *
 *  services which may change over time. Regular maintenance may be        *
 *  required to keep it functional.                                        *
 *                                                                         *
 *  Updated: 2025 - Enhanced for LuminariMUD compatibility, improved       *
 *  error handling and documentation                                       *
 ************************************************************************ */

#define log(msg) fprintf(stderr, "%s\n", msg)

#include "conf.h"
#include "sysdep.h"

#define MEM_USE 10000
char buf[MEM_USE];

/* Function prototypes */
int get_line(FILE *fl, char *buf);
void skip_spaces(char **string);
void parse_webster_html(char *arg);

/**
 * Main function for the webster dictionary utility
 *
 * Fetches dictionary definitions for a given word from an online source
 * and formats them for display in the MUD. Optionally signals a process
 * when complete.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments:
 *             [1] word to look up
 *             [2] process ID to signal when complete (optional)
 * @return 0 on success
 */
int main(int argc, char **argv)
{
  int pid = 0;

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <word> [pid]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Looks up dictionary definition for the specified word.\n");
    fprintf(stderr, "Optionally signals the given process ID when complete.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example: %s hello 1234\n", argv[0]);
    return 1;
  }

  if (argc >= 3) {
    pid = atoi(argv[2]);
  }

  printf("Looking up definition for: %s\n", argv[1]);

  /* Construct command to fetch HTML content using lynx */
  snprintf(buf, sizeof(buf),
           "lynx -accept_all_cookies -source http://www.thefreedictionary.com/%s"
           " >webster.html 2>/dev/null",
           argv[1]);

  printf("Fetching data from online dictionary...\n");
  if (system(buf) != 0) {
    /* If system call fails, we'll still try to parse what we have */
    log("Warning: Failed to execute lynx command - check if lynx is installed");
  }

  /* Parse the downloaded HTML and create formatted output */
  parse_webster_html(argv[1]);

  /* Signal the requesting process if PID was provided */
  if (pid > 0) {
    printf("Signaling process %d\n", pid);
    kill(pid, SIGUSR2);
  }

  printf("Dictionary lookup complete. Results saved to 'websterinfo'\n");
  return (0);
}

/**
 * Parse HTML content from dictionary website and extract definitions
 *
 * Reads the downloaded HTML file, parses it to extract dictionary
 * definitions, and writes formatted output to 'websterinfo' file.
 *
 * @param arg The word that was looked up (for display purposes)
 */
void parse_webster_html(char *arg)
{
  FILE *infile, *outfile;
  char scanbuf[MEM_USE], outline[MEM_USE], *p, *q;

  outfile = fopen("websterinfo", "w");
  if (!outfile)
  {
    fprintf(stderr, "Error: Could not create output file 'websterinfo'\n");
    exit(1);
  }

  infile = fopen("webster.html", "r");
  if (!infile)
  {
    fprintf(outfile, "Error: Could not retrieve dictionary data for '%s'.\n", arg);
    fprintf(outfile, "This may be due to:\n");
    fprintf(outfile, "- Network connectivity issues\n");
    fprintf(outfile, "- Missing 'lynx' browser\n");
    fprintf(outfile, "- Changes to the dictionary website\n");
    fprintf(outfile, "\nPlease try again later or use a web browser to visit:\n");
    fprintf(outfile, "http://www.thefreedictionary.com/%s\n", arg);
    fclose(outfile);
    return;
  }

  unlink("webster.html"); /* We can still read from the open file handle */

  for (; get_line(infile, buf) != 0;)
  {

    if (strncmp(buf, "<script>write_ads(AdsNum, 0, 1)</script>", 40) != 0)
      continue; // read until we hit the line with results in it.

    p = buf + 40;

    if (strncmp(p, "<br>", 4) == 0)
    {
      fprintf(outfile, "That word could not be found.\n");
      goto end;
    }
    else if (strncmp(p, "<div ", 5) == 0) // definition is here, all in one line.
    {
      while (strncmp(p, "ds-list", 7)) //seek to the definition
        p++;

      strncpy(scanbuf, p, sizeof(scanbuf)); // strtok on a copy.

      p = strtok(scanbuf, ">"); // chop the line at the end of tags: <br><b>word</b> becomes "<br" "<b" "word</b"
      p = strtok(NULL, ">");    // skip the rest of this tag.

      fprintf(outfile, "Info on: %s\n\n", arg);

      while (1)
      {
        q = outline;

        while (*p != '<')
        {
          assert(p < scanbuf + sizeof(scanbuf));
          *q++ = *p++;
        }
        if (!strncmp(p, "<br", 3) || !strncmp(p, "<p", 2) || !strncmp(p, "<div class=\"ds-list\"", 23) || !strncmp(p, "<div class=\"sds-list\"", 24))
          *q++ = '\n';
        // if it's not a <br> tag or a <div class="sds-list"> or <div class="ds-list"> tag, ignore it.

        *q++ = '\0';
        fprintf(outfile, "%s", outline);

        if (!strncmp(p, "</table", 7))
          goto end;

        p = strtok(NULL, ">");
      }
    }
    else if (strncmp(p, "<div>", 5) == 0) // not found, but suggestions are ample:
    {
      strncpy(scanbuf, p, sizeof(scanbuf)); // strtok on a copy.

      p = strtok(scanbuf, ">"); // chop the line at the end of tags: <br><b>word</b> becomes "<br>" "<b>" "word</b>"
      p = strtok(NULL, ">");    // skip the rest of this tag.

      while (1)
      {
        q = outline;

        while (*p != '<')
          *q++ = *p++;

        if (!strncmp(p, "<td ", 4))
          *q++ = '\n';
        // if it's not a <td> tag, ignore it.

        *q++ = '\0';
        fprintf(outfile, "%s", outline);

        if (!strncmp(p, "</table", 7))
          goto end;

        p = strtok(NULL, ">");
      }
    }
    else
    {
      // weird.. one of the above should be correct.
      fprintf(outfile, "It would appear that the free online dictionary has changed their format.\n"
                       "Sorry, but you might need a webrowser instead.\n\n"
                       "See http://www.thefreedictionary.com/%s",
              arg);
      goto end;
    }
  }

end:
  fclose(infile);

  fprintf(outfile, "~");
  fclose(outfile);
}

/**
 * Read the next non-blank line from input stream
 *
 * Reads lines from the file, skipping blank lines, and removes
 * the trailing newline character from the input.
 *
 * @param fl File pointer to read from
 * @param buf Buffer to store the line
 * @return 1 if line was read successfully, 0 on EOF
 */
int get_line(FILE *fl, char *buf)
{
  char temp[MEM_USE];

  do
  {
    if (!fgets(temp, MEM_USE, fl))
      break;
    if (*temp && temp[strlen(temp) - 1] == '\n')
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && !*temp);

  if (feof(fl))
    return (0);
  else
  {
    strcpy(buf, temp);
    return (1);
  }
}

/**
 * Skip over leading whitespace characters in a string
 *
 * Advances the string pointer past any leading whitespace characters.
 *
 * @param string Pointer to string pointer (modified to skip spaces)
 */
void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++)
    ;
}
