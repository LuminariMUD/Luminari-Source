/* ************************************************************************
*  file:  split.c                                     Part of LuminariMUD *
*  Usage: split one large file into multiple smaller ones, with index     *
*  Written by Jeremy Elson                                                *
*  All Rights Reserved                                                    *
*  Copyright (C) 1993 The Trustees of The Johns Hopkins University        *
*                                                                         *
*  This utility splits a large file into multiple smaller ones, mainly    *
*  to help break huge world files into zone-sized files that are easier   *
*  to manage. It reads from stdin and creates output files based on       *
*  special marker lines.                                                   *
*                                                                         *
*  Usage: split < input_file                                              *
*                                                                         *
*  At each point in the original file where you want a break, insert a    *
*  line containing "=filename" at the break point. The utility will       *
*  create separate files and an index file listing all created files.     *
*                                                                         *
*  Updated: 2025 - Enhanced for LuminariMUD compatibility, improved       *
*  error handling and documentation                                       *
************************************************************************* */

#define INDEX_NAME "index"
#define BSZ 256
#define MAGIC_CHAR '='

#include "conf.h"
#include "sysdep.h"

/**
 * Main function for the split utility
 *
 * Reads from stdin and splits the input into multiple files based on
 * marker lines starting with '='. Creates an index file listing all
 * output files created during the split process.
 *
 * Marker format: =filename
 * The content following each marker is written to the specified file.
 *
 * @return 0 on success, 1 on error
 */
int main(void)
{
  char line[BSZ + 1];
  char *newline_pos;
  FILE *index = NULL, *outfile = NULL;
  int file_count = 0;

  printf("Split utility - reading from stdin...\n");
  printf("Use lines starting with '=' to specify output filenames.\n");
  printf("Example: =zone01.wld\n\n");

  if (!(index = fopen(INDEX_NAME, "w")))
  {
    perror("error opening index file for write");
    exit(1);
  }

  while (fgets(line, BSZ, stdin))
  {
    if (*line == MAGIC_CHAR)
    {
      /* Remove newline from filename */
      newline_pos = strchr(line, '\n');
      if (newline_pos) {
        *newline_pos = '\0';
      }

      /* Close previous output file if open */
      if (outfile)
      {
        fclose(outfile);
        outfile = NULL;
      }

      /* Open new output file */
      if (!(outfile = fopen((line + 1), "w")))
      {
        fprintf(stderr, "Error opening output file: %s\n", line + 1);
        fclose(index);
        exit(1);
      }

      /* Add filename to index */
      fputs(line + 1, index);
      fputs("\n", index);
      file_count++;

      printf("Creating file: %s\n", line + 1);
    }
    else if (outfile)
    {
      /* Write content to current output file */
      fputs(line, outfile);
    }
    else
    {
      /* Content before first marker - warn user */
      fprintf(stderr, "Warning: Content found before first file marker, ignoring.\n");
    }
  }

  /* Close files and finish */
  fputs("$\r\n", index);
  fclose(index);
  if (outfile) {
    fclose(outfile);
  }

  printf("\nSplit complete. Created %d files.\n", file_count);
  printf("Index file '%s' contains list of created files.\n", INDEX_NAME);

  return (0);
}
