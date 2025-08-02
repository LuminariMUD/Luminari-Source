/* ************************************************************************
 *  file:  autowiz.c                                   Part of LuminariMUD *
 *  Usage: self-updating wizlists                                          *
 *  Written by Jeremy Elson                                                *
 *  All Rights Reserved                                                    *
 *  Copyright (C) 1993 The Trustees of The Johns Hopkins University        *
 *                                                                         *
 *  This utility automatically generates wizard and immortal lists by      *
 *  reading the player index file and extracting characters above a        *
 *  specified level threshold. It creates formatted lists that can be      *
 *  displayed in-game or on websites.                                      *
 *                                                                         *
 *  Updated: 2025 - Enhanced for LuminariMUD compatibility, improved       *
 *  error handling, and fixed compiler warnings                           *
 ************************************************************************* */

#include "conf.h"
#include "sysdep.h"
#include <signal.h>
#include "structs.h"
#include "utils.h"
#include "db.h"

#define IMM_LMARG "   "
#define IMM_NSIZE 16
#define LINE_LEN 64
#define MIN_LEVEL LVL_IMMORT

/* max level that should be in columns instead of centered */
#define COL_LEVEL LVL_IMMORT

struct name_rec
{
  char name[25];
  struct name_rec *next;
};

struct control_rec
{
  int level;
  char *level_name;
};

struct level_rec
{
  struct control_rec *params;
  struct level_rec *next;
  struct name_rec *names;
};

struct control_rec level_params[] =
    {
        {LVL_IMMORT, "Staff"},
        {LVL_STAFF, "Senior Staff"},
        {LVL_GRSTAFF, "World Forgers"},
        {LVL_IMPL, "Forgers"},
        {0, ""}};

struct level_rec *levels = 0;

/**
 * Initialize the level structure from the level_params array
 *
 * Creates a linked list of level records based on the static level_params
 * array, setting up the data structures needed for wizard list generation.
 */
void initialize(void)
{
  struct level_rec *tmp;
  int i = 0;

  while (level_params[i].level > 0)
  {
    tmp = (struct level_rec *)malloc(sizeof(struct level_rec));
    if (!tmp) {
      fprintf(stderr, "Error: Failed to allocate memory for level record\n");
      exit(1);
    }
    tmp->names = 0;
    tmp->params = &(level_params[i++]);
    tmp->next = levels;
    levels = tmp;
  }
}

void read_file(void)
{
  void add_name(byte level, char *name);
  char *CAP(char *txt);
  int get_line(FILE * fl, char *buf);
  bitvector_t asciiflag_conv(const char *flag);

  FILE *fl;
  int recs, i, last = 0, level = 0, flags = 0;
  char index_name[40], line[256], bits[64];
  char name[MAX_NAME_LENGTH];
  long id = 0;

  sprintf(index_name, "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(fl = fopen(index_name, "r")))
  {
    perror("Error opening playerfile");
    exit(1);
  }
  /* count the number of players in the index */
  recs = 0;
  while (get_line(fl, line))
    if (*line != '~')
      recs++;
  rewind(fl);

  for (i = 0; i < recs; i++)
  {
    get_line(fl, line);
    sscanf(line, "%ld %s %d %s %d", &id, name, &level, bits, &last);
    CAP(name);
    flags = asciiflag_conv(bits);
    if (level >= MIN_LEVEL &&
        !(IS_SET(flags, PINDEX_NOWIZLIST)) &&
        !(IS_SET(flags, PINDEX_DELETED)))
      add_name(level, name);
  }
  fclose(fl);
}

/**
 * Add a player name to the appropriate level list
 *
 * Validates the name (must be all alphabetic characters) and adds it
 * to the linked list for the appropriate level category.
 *
 * @param level The player's level
 * @param name The player's name (must be all alphabetic)
 */
void add_name(byte level, char *name)
{
  struct name_rec *tmp;
  struct level_rec *curr_level;
  char *ptr;

  if (!name || !*name)
    return;

  /* Validate name contains only alphabetic characters */
  for (ptr = name; *ptr; ptr++)
    if (!isalpha(*ptr))
      return;

  tmp = (struct name_rec *)malloc(sizeof(struct name_rec));
  if (!tmp) {
    fprintf(stderr, "Error: Failed to allocate memory for name record\n");
    return;
  }
  strcpy(tmp->name, name);
  tmp->next = 0;

  /* Find the appropriate level list */
  curr_level = levels;
  while (curr_level && curr_level->params->level > level)
    curr_level = curr_level->next;

  if (curr_level) {
    tmp->next = curr_level->names;
    curr_level->names = tmp;
  } else {
    /* No appropriate level found, free the allocated memory */
    free(tmp);
  }
}

void sort_names(void)
{
  struct level_rec *curr_level;
  struct name_rec *a, *b;
  char temp[100];

  for (curr_level = levels; curr_level; curr_level = curr_level->next)
  {
    for (a = curr_level->names; a && a->next; a = a->next)
    {
      for (b = a->next; b; b = b->next)
      {
        if (strcmp(a->name, b->name) > 0)
        {
          strcpy(temp, a->name);
          strcpy(a->name, b->name);
          strcpy(b->name, temp);
        }
      }
    }
  }
}

void write_wizlist(FILE *out, int minlev, int maxlev)
{
  char buf[100];
  struct level_rec *curr_level;
  struct name_rec *curr_name;
  int i, j;

  fprintf(out,
          "*******************************************************************************\n"
          "*          The following people have reached immortality on Luminari.         *\n"
          "*******************************************************************************\n\n");

  for (curr_level = levels; curr_level; curr_level = curr_level->next)
  {
    if (curr_level->params->level < minlev ||
        curr_level->params->level > maxlev)
      continue;
    i = 39 - (strlen(curr_level->params->level_name) >> 1);
    for (j = 1; j <= i; j++)
      fputc(' ', out);
    fprintf(out, "%s\n", curr_level->params->level_name);
    for (j = 1; j <= i; j++)
      fputc(' ', out);
    for (j = 1; j <= strlen(curr_level->params->level_name); j++)
      fputc('~', out);
    fprintf(out, "\n");

    strcpy(buf, "");
    curr_name = curr_level->names;
    while (curr_name)
    {
      strcat(buf, curr_name->name);
      if (strlen(buf) > LINE_LEN)
      {
        if (curr_level->params->level <= COL_LEVEL)
          fprintf(out, IMM_LMARG);
        else
        {
          i = 40 - (strlen(buf) >> 1);
          for (j = 1; j <= i; j++)
            fputc(' ', out);
        }
        fprintf(out, "%s\n", buf);
        strcpy(buf, "");
      }
      else
      {
        if (curr_level->params->level <= COL_LEVEL)
        {
          for (j = 1; j <= (IMM_NSIZE - strlen(curr_name->name)); j++)
            strcat(buf, " ");
        }
        if (curr_level->params->level > COL_LEVEL)
          strcat(buf, "   ");
      }
      curr_name = curr_name->next;
    }

    if (*buf)
    {
      if (curr_level->params->level <= COL_LEVEL)
        fprintf(out, "%s%s\n", IMM_LMARG, buf);
      else
      {
        i = 40 - (strlen(buf) >> 1);
        for (j = 1; j <= i; j++)
          fputc(' ', out);
        fprintf(out, "%s\n", buf);
      }
    }
    fprintf(out, "\n");
  }
}

/**
 * Main function for the autowiz utility
 *
 * Generates wizard and immortal lists by reading the player index file
 * and extracting characters above specified level thresholds.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments:
 *             [1] wizard level threshold
 *             [2] wizard list output file
 *             [3] immortal level threshold
 *             [4] immortal list output file
 *             [5] optional PID to signal (unused)
 * @return 0 on success, exits on error
 */
int main(int argc, char **argv)
{
  int wizlevel, immlevel;
  FILE *fl;

  if (argc != 5 && argc != 6)
  {
    printf("Format: %s wizlev wizlistfile immlev immlistfile [pid to signal]\n",
           argv[0]);
    printf("\n");
    printf("Generates wizard and immortal lists from player index file.\n");
    printf("wizlev    - minimum level for wizard list\n");
    printf("immlev    - minimum level for immortal list\n");
    printf("Example: %s 31 wizlist.txt 34 immlist.txt\n", argv[0]);
    exit(0);
  }
  wizlevel = atoi(argv[1]);
  immlevel = atoi(argv[3]);

#ifdef CIRCLE_UNIX /* Perhaps #ifndef CIRCLE_WINDOWS but ... */
//  int pid;

//  pid = 0;

//  if (argc == 6)
//    pid = atoi(argv[5]);
#endif

  initialize();
  read_file();
  sort_names();

  fl = fopen(argv[2], "w");
  write_wizlist(fl, wizlevel, LVL_IMPL);
  fclose(fl);

  fl = fopen(argv[4], "w");
  write_wizlist(fl, immlevel, wizlevel - 1);
  fclose(fl);

  // CIRCLE_UNIX is NOT tested, just letting it slide for now -zusuk

  return (0);
}

char *CAP(char *txt)
{
  *txt = UPPER(*txt);
  return (txt);
}

/* get_line reads the next non-blank line off of the input stream. The newline
 * character is removed from the input.  Lines which begin with '*' are
 * considered to be comments. Returns the number of lines advanced in the
 * file. */
/**
 * Read a line from file, skipping comments and blank lines
 *
 * @param fl File pointer to read from
 * @param buf Buffer to store the line (must be at least MEDIUM_STRING size)
 * @return Number of lines read, or 0 on EOF/error
 */
int get_line(FILE *fl, char *buf)
{
  char temp[MEDIUM_STRING] = {'\0'};
  int lines = 0;

  do
  {
    if (!fgets(temp, MEDIUM_STRING, fl)) {
      if (feof(fl))
        return (0);
      /* Handle error case */
      *buf = '\0';
      return (0);
    }
    lines++;
  } while (*temp == '*' || *temp == '\n');

  /* Remove trailing newline if present */
  if (strlen(temp) > 0 && temp[strlen(temp) - 1] == '\n') {
    temp[strlen(temp) - 1] = '\0';
  }
  strcpy(buf, temp);
  return (lines);
}

bitvector_t asciiflag_conv(const char *flag)
{
  bitvector_t flags = 0;
  int is_number = 1;
  const char *p;
  // register char *p;

  for (p = flag; *p; p++)
  {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!isdigit(*p))
      is_number = 0;
  }

  if (is_number)
    flags = atol(flag);

  return (flags);
}
