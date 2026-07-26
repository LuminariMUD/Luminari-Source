/* ************************************************************************
 *  File: wld2html.c                                   Part of LuminariMUD *
 *  Usage: Convert one or more DikuMUD .wld files to HTML                  *
 *                                                                         *
 *  This program is in the public domain.                                  *
 *  Written by Jeremy Elson and based on the Circle 3.0 syntax checker.    *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include <stdbool.h>
#include <stdint.h>

#define NUM_OF_DIRS 10
#define LINE_LENGTH 1024
#define INITIAL_ROOM_CAPACITY 128

struct room_direction_data
{
  char *general_description;
  char *keyword;
  int exit_info;
  int key;
  int to_room;
};

struct room_data
{
  int number;
  char *name;
  char *description;
  struct room_direction_data *dir_option[NUM_OF_DIRS];
};

static struct room_data *world;
static size_t room_count;
static size_t room_capacity;

static const char *const dir_names[NUM_OF_DIRS] = {
    "North", "East",      "South",     "West",      "Up",
    "Down",  "Northwest", "Northeast", "Southeast", "Southwest"};

static void append_string(char **destination, size_t *length, size_t *capacity, const char *text);
static void discard_moving_room(FILE *fl, int room_vnum);
static void free_world(void);
static char *fread_string(FILE *fl, const char *error);
static const struct room_data *find_room(int vnum);
static int get_line(FILE *fl, char *line, size_t line_size);
static void load_world_file(const char *filename);
static void parse_room(FILE *fl, int virtual_nr);
static void setup_dir(FILE *fl, struct room_data *room, int dir);
static void sort_and_validate_world(void);
static void write_html_escaped(FILE *fl, const char *text);
static void write_output(void);

static void fatal_file_error(const char *message, const char *context)
{
  fprintf(stderr, "wld2html: %s%s%s\n", message, context ? ": " : "", context ? context : "");
  free_world();
  exit(EXIT_FAILURE);
}

static void *checked_realloc(void *memory, size_t size)
{
  void *resized;

  resized = realloc(memory, size);
  if (!resized)
  {
    perror("wld2html: realloc");
    free_world();
    exit(EXIT_FAILURE);
  }
  return resized;
}

static void append_string(char **destination, size_t *length, size_t *capacity, const char *text)
{
  size_t text_length, required, new_capacity;

  text_length = strlen(text);
  if (text_length > SIZE_MAX - *length - 1)
    fatal_file_error("string size overflow", NULL);

  required = *length + text_length + 1;
  if (required > *capacity)
  {
    new_capacity = *capacity ? *capacity : LINE_LENGTH;
    while (new_capacity < required)
    {
      if (new_capacity > SIZE_MAX / 2)
      {
        new_capacity = required;
        break;
      }
      new_capacity *= 2;
    }
    *destination = checked_realloc(*destination, new_capacity);
    *capacity = new_capacity;
  }

  memcpy(*destination + *length, text, text_length + 1);
  *length += text_length;
}

/* Read a tilde-terminated world string without imposing the server's runtime
 * string-size limit on this offline documentation utility. */
static char *fread_string(FILE *fl, const char *error)
{
  char line[LINE_LENGTH + 2], *terminator, *result;
  size_t length, capacity, line_length;

  result = NULL;
  length = 0;
  capacity = 0;

  for (;;)
  {
    if (!fgets(line, sizeof(line), fl))
      fatal_file_error("unterminated string", error);

    terminator = strchr(line, '~');
    if (terminator)
    {
      *terminator = '\0';
      append_string(&result, &length, &capacity, line);
      break;
    }

    line_length = strlen(line);
    while (line_length > 0 && (line[line_length - 1] == '\n' || line[line_length - 1] == '\r'))
      line[--line_length] = '\0';
    append_string(&result, &length, &capacity, line);
    append_string(&result, &length, &capacity, "\r\n");
  }

  if (!result)
  {
    result = strdup("");
    if (!result)
      fatal_file_error("out of memory", error);
  }
  return result;
}

/* Read a nonblank, noncomment data line and remove only line terminators. */
static int get_line(FILE *fl, char *line, size_t line_size)
{
  size_t length;

  while (fgets(line, line_size, fl))
  {
    length = strlen(line);
    if (length == line_size - 1 && line[length - 1] != '\n' && !feof(fl))
      fatal_file_error("data line exceeds parser limit", NULL);

    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r'))
      line[--length] = '\0';

    if (*line && *line != '*')
      return true;
  }

  if (ferror(fl))
    fatal_file_error("error reading world file", strerror(errno));
  return false;
}

static void ensure_room_capacity(void)
{
  size_t new_capacity;

  if (room_count < room_capacity)
    return;

  new_capacity = room_capacity ? room_capacity * 2 : INITIAL_ROOM_CAPACITY;
  if (new_capacity < room_capacity || new_capacity > SIZE_MAX / sizeof(struct room_data))
    fatal_file_error("too many rooms", NULL);

  world = checked_realloc(world, new_capacity * sizeof(struct room_data));
  memset(world + room_capacity, 0, (new_capacity - room_capacity) * sizeof(struct room_data));
  room_capacity = new_capacity;
}

static void setup_dir(FILE *fl, struct room_data *room, int dir)
{
  struct room_direction_data *exit_data;
  char context[128], line[LINE_LENGTH];

  if (dir < 0 || dir >= NUM_OF_DIRS)
  {
    snprintf(context, sizeof(context), "room #%d direction D%d", room->number, dir);
    fatal_file_error("invalid direction", context);
  }

  exit_data = calloc(1, sizeof(*exit_data));
  if (!exit_data)
    fatal_file_error("out of memory while reading an exit", NULL);

  snprintf(context, sizeof(context), "room #%d direction D%d", room->number, dir);
  exit_data->general_description = fread_string(fl, context);
  exit_data->keyword = fread_string(fl, context);

  if (!get_line(fl, line, sizeof(line)) ||
      sscanf(line, " %d %d %d ", &exit_data->exit_info, &exit_data->key, &exit_data->to_room) != 3)
    fatal_file_error("invalid exit data", context);

  room->dir_option[dir] = exit_data;
}

/* Moving-room data is irrelevant to static room documentation, but it must be
 * consumed so the parser remains aligned with the next room field. */
static void discard_moving_room(FILE *fl, int room_vnum)
{
  char context[128], line[LINE_LENGTH];
  int i;

  snprintf(context, sizeof(context), "moving-room data in room #%d", room_vnum);
  for (i = 0; i < 3; i++)
    if (!get_line(fl, line, sizeof(line)))
      fatal_file_error("missing moving-room message", context);

  do
  {
    if (!get_line(fl, line, sizeof(line)))
      fatal_file_error("unterminated moving-room route list", context);
  } while (*line != '~');
}

static void parse_room(FILE *fl, int virtual_nr)
{
  struct room_data *room;
  char context[128], line[LINE_LENGTH], flag1[128], flag2[128], flag3[128], flag4[128];
  char *extra_keyword, *extra_description;
  int zone, sector, fields, dir;

  ensure_room_capacity();
  room = &world[room_count];
  room->number = virtual_nr;

  snprintf(context, sizeof(context), "room #%d", virtual_nr);
  room->name = fread_string(fl, context);
  room->description = fread_string(fl, context);

  if (!get_line(fl, line, sizeof(line)))
    fatal_file_error("missing room flags", context);

  fields =
      sscanf(line, " %d %127s %127s %127s %127s %d ", &zone, flag1, flag2, flag3, flag4, &sector);
  if (fields != 6)
    fields = sscanf(line, " %d %127s %d ", &zone, flag1, &sector);
  if (fields != 3 && fields != 6)
    fatal_file_error("invalid room flags or sector", context);

  for (;;)
  {
    if (!get_line(fl, line, sizeof(line)))
      fatal_file_error("unexpected end of room", context);

    switch (*line)
    {
    case 'C':
      if (!get_line(fl, line, sizeof(line)))
        fatal_file_error("missing coordinate data", context);
      break;
    case 'D':
      if (sscanf(line + 1, "%d", &dir) != 1)
        fatal_file_error("invalid direction marker", context);
      setup_dir(fl, room, dir);
      break;
    case 'E':
      extra_keyword = fread_string(fl, context);
      extra_description = fread_string(fl, context);
      free(extra_keyword);
      free(extra_description);
      break;
    case 'M':
      discard_moving_room(fl, virtual_nr);
      break;
    case 'Z':
      if (!get_line(fl, line, sizeof(line)))
        fatal_file_error("missing room special-procedure name", context);
      break;
    case 'S':
      room_count++;
      return;
    default:
      fatal_file_error("unknown room field", line);
    }
  }
}

static void load_world_file(const char *filename)
{
  FILE *fl;
  char line[LINE_LENGTH], *end;
  long vnum;

  fl = fopen(filename, "r");
  if (!fl)
    fatal_file_error("cannot open world file", filename);

  while (get_line(fl, line, sizeof(line)))
  {
    if (*line == '$')
      break;

    /* DG trigger attachments follow a room's S marker. They are not needed
     * for room-to-room HTML navigation. */
    if (*line == 'T')
      continue;

    if (*line != '#')
      fatal_file_error("expected a room number", line);

    errno = 0;
    end = NULL;
    vnum = strtol(line + 1, &end, 10);
    if (errno || end == line + 1 || *end || vnum < INT_MIN || vnum > INT_MAX)
      fatal_file_error("invalid room number", line);
    parse_room(fl, (int)vnum);
  }

  fclose(fl);
}

static int compare_rooms(const void *left, const void *right)
{
  const struct room_data *left_room, *right_room;

  left_room = left;
  right_room = right;
  if (left_room->number < right_room->number)
    return -1;
  if (left_room->number > right_room->number)
    return 1;
  return 0;
}

static void sort_and_validate_world(void)
{
  char context[64];
  size_t i;

  qsort(world, room_count, sizeof(*world), compare_rooms);
  for (i = 1; i < room_count; i++)
  {
    if (world[i - 1].number == world[i].number)
    {
      snprintf(context, sizeof(context), "duplicate room vnum %d", world[i].number);
      fatal_file_error(context, NULL);
    }
  }
}

static const struct room_data *find_room(int vnum)
{
  size_t low, high, middle;

  low = 0;
  high = room_count;
  while (low < high)
  {
    middle = low + (high - low) / 2;
    if (world[middle].number == vnum)
      return &world[middle];
    if (world[middle].number < vnum)
      low = middle + 1;
    else
      high = middle;
  }
  return NULL;
}

static void write_html_escaped(FILE *fl, const char *text)
{
  const unsigned char *character;

  if (!text)
    return;

  for (character = (const unsigned char *)text; *character; character++)
  {
    switch (*character)
    {
    case '&':
      fputs("&amp;", fl);
      break;
    case '<':
      fputs("&lt;", fl);
      break;
    case '>':
      fputs("&gt;", fl);
      break;
    case '"':
      fputs("&quot;", fl);
      break;
    default:
      fputc(*character, fl);
      break;
    }
  }
}

static void write_output(void)
{
  const struct room_data *target;
  struct room_direction_data *exit_data;
  FILE *fl;
  char filename[64];
  size_t i;
  int door;
  bool found;

  for (i = 0; i < room_count; i++)
  {
    if (snprintf(filename, sizeof(filename), "%d.html", world[i].number) >= (int)sizeof(filename))
      fatal_file_error("output filename is too long", NULL);

    fprintf(stderr, "Writing %s\n", filename);
    fl = fopen(filename, "w");
    if (!fl)
      fatal_file_error("cannot open output file", filename);

    fputs("<!doctype html>\n<html><head><meta charset=\"utf-8\"><title>", fl);
    write_html_escaped(fl, world[i].name);
    fputs("</title></head><body>\n<h1>", fl);
    write_html_escaped(fl, world[i].name);
    fputs("</h1>\n<pre>", fl);
    write_html_escaped(fl, world[i].description);
    fputs("</pre>\n<h2>Exits</h2>\n", fl);

    found = false;
    for (door = 0; door < NUM_OF_DIRS; door++)
    {
      exit_data = world[i].dir_option[door];
      if (!exit_data || exit_data->to_room < 0)
        continue;

      found = true;
      target = find_room(exit_data->to_room);
      if (target)
      {
        fprintf(fl, "<p><a href=\"%d.html\">%s to ", target->number, dir_names[door]);
        write_html_escaped(fl, target->name);
        fputs("</a></p>\n", fl);
      }
      else
      {
        fprintf(fl, "<p>%s to unloaded room %d</p>\n", dir_names[door], exit_data->to_room);
        fprintf(stderr, "Room %d references unloaded room %d\n", world[i].number,
                exit_data->to_room);
      }
    }
    if (!found)
      fputs("<p>None</p>\n", fl);
    fputs("</body></html>\n", fl);

    if (fclose(fl) != 0)
      fatal_file_error("error closing output file", filename);
  }
}

static void free_world(void)
{
  struct room_direction_data *exit_data;
  size_t i;
  int door;

  if (!world)
    return;

  for (i = 0; i < room_count; i++)
  {
    free(world[i].name);
    free(world[i].description);
    for (door = 0; door < NUM_OF_DIRS; door++)
    {
      exit_data = world[i].dir_option[door];
      if (!exit_data)
        continue;
      free(exit_data->general_description);
      free(exit_data->keyword);
      free(exit_data);
    }
  }
  free(world);
  world = NULL;
  room_count = 0;
  room_capacity = 0;
}

int main(int argc, char **argv)
{
  int i;

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <world-file> [world-file ...]\n", argv[0]);
    return EXIT_FAILURE;
  }

  for (i = 1; i < argc; i++)
    load_world_file(argv[i]);

  if (room_count == 0)
    fatal_file_error("no rooms found", NULL);

  sort_and_validate_world();
  fprintf(stderr, "Loaded %zu rooms from %d file%s\n", room_count, argc - 1, argc == 2 ? "" : "s");
  write_output();
  free_world();
  return EXIT_SUCCESS;
}
