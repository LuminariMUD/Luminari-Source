/* Production-linked tests for study companion and familiar menu/parse alignment. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/interpreter.h"
#include "../../src/net/protocol.h"
#include "../../src/olc/oasis.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_study(struct descriptor_data *d, int class);

extern int animal_vnums[];
extern int familiar_vnums[];

#define STUDY_PET_MAX_LISTED 32
#define STUDY_PET_NAME_SIZE 80

struct study_pet_fixture
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
};

static void reset_study_output(struct descriptor_data *d)
{
  if (d->large_outbuf != NULL && d->output == d->large_outbuf->text)
  {
    d->output[0] = '\0';
    d->bufptr = 0;
    d->bufspace = LARGE_BUFSIZE - 1;
    return;
  }

  d->output = d->small_outbuf;
  d->small_outbuf[0] = '\0';
  d->bufptr = 0;
  d->bufspace = SMALL_BUFSIZE - 1;
}

static void study_parse_choice(struct descriptor_data *d, const char *choice)
{
  char arg[MAX_INPUT_LENGTH];

  snprintf(arg, sizeof(arg), "%s", choice);
  reset_study_output(d);
  study_parse(d, arg);
}

static void study_parse_number(struct descriptor_data *d, int number)
{
  char arg[MAX_INPUT_LENGTH];

  snprintf(arg, sizeof(arg), "%d", number);
  reset_study_output(d);
  study_parse(d, arg);
}

static void begin_study_pet_fixture(CuTest *tc, struct study_pet_fixture *fixture)
{
  memset(fixture, 0, sizeof(*fixture));
  clear_char(&fixture->ch);
  fixture->ch.player_specials = &fixture->specials;
  fixture->ch.player.name = "study pet tester";
  GET_LEVEL(&fixture->ch) = 10;
  GET_CLASS(&fixture->ch) = CLASS_DRUID;
  SET_FEAT(&fixture->ch, FEAT_ANIMAL_COMPANION, 1);
  SET_FEAT(&fixture->ch, FEAT_SUMMON_FAMILIAR, 1);
  fixture->ch.desc = &fixture->descriptor;
  fixture->descriptor.character = &fixture->ch;
  fixture->descriptor.output = fixture->descriptor.small_outbuf;
  fixture->descriptor.bufspace = SMALL_BUFSIZE - 1;
  fixture->descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, fixture->descriptor.pProtocol);
  init_study(&fixture->descriptor, CLASS_DRUID);
  CuAssertPtrNotNull(tc, fixture->descriptor.olc);
  CuAssertPtrNotNull(tc, LEVELUP((&fixture->ch)));
  OLC_MODE(&fixture->descriptor) = STUDY_GEN_MAIN_MENU;
}

static void end_study_pet_fixture(struct study_pet_fixture *fixture)
{
  if (LEVELUP((&fixture->ch)) != NULL)
  {
    free(LEVELUP((&fixture->ch)));
    LEVELUP((&fixture->ch)) = NULL;
  }
  if (fixture->descriptor.olc != NULL)
  {
    free(fixture->descriptor.olc);
    fixture->descriptor.olc = NULL;
  }
  if (fixture->descriptor.large_outbuf != NULL)
  {
    free(fixture->descriptor.large_outbuf->text);
    free(fixture->descriptor.large_outbuf);
    fixture->descriptor.large_outbuf = NULL;
  }
  if (fixture->descriptor.pProtocol != NULL)
    ProtocolDestroy(fixture->descriptor.pProtocol);
  fixture->ch.desc = NULL;
}

static int discover_listed_choices(const char *output, int *listed, int max_listed, int *first,
                                   int *last)
{
  const char *p = output;
  int count = 0;

  *first = 0;
  *last = 0;
  if (output == NULL)
    return 0;

  while (*p != '\0')
  {
    if ((p == output || p[-1] == '\n') && isdigit((unsigned char)*p))
    {
      char *end = NULL;
      long value = strtol(p, &end, 10);

      if (end != p && *end == ')' && value > 0)
      {
        if (count < max_listed)
          listed[count] = (int)value;
        if (count == 0)
          *first = (int)value;
        *last = (int)value;
        count++;
      }
    }
    p++;
  }

  return count;
}

static int copy_listed_entry(const char *output, int number, char *buf, size_t buf_size)
{
  const char *p = output;
  char prefix[16];
  size_t length;

  if (output == NULL || buf == NULL || buf_size == 0)
    return 0;

  snprintf(prefix, sizeof(prefix), "%d)", number);
  while (p != NULL && *p != '\0')
  {
    if ((p == output || p[-1] == '\n') && strncmp(p, prefix, strlen(prefix)) == 0)
    {
      length = strcspn(p, "\r\n");
      if (length >= buf_size)
        length = buf_size - 1;
      memcpy(buf, p, length);
      buf[length] = '\0';
      return 1;
    }
    p = strchr(p, '\n');
    if (p != NULL)
      p++;
  }

  return 0;
}

static int listed_number_containing(const char *output, const char *needle)
{
  const char *found;
  const char *line;

  if (output == NULL || needle == NULL)
    return 0;

  found = strstr(output, needle);
  if (found == NULL)
    return 0;

  line = found;
  while (line > output && line[-1] != '\n')
    line--;

  if (!isdigit((unsigned char)*line))
    return 0;

  return atoi(line);
}

static void open_study_pet_menu(CuTest *tc, struct study_pet_fixture *fixture, const char *choice,
                                const char *title)
{
  OLC_MODE(&fixture->descriptor) = STUDY_GEN_MAIN_MENU;
  study_parse_choice(&fixture->descriptor, choice);
  CuAssertPtrNotNull(tc, strstr(fixture->descriptor.output, title));
  CuAssertPtrEquals(tc, NULL, strstr(fixture->descriptor.output, "Unknown"));
}

static void assert_saved_listed_choice(CuTest *tc, struct descriptor_data *d, int *saved,
                                       const int *table, int choice, const char *listed_name)
{
  CuAssertPtrNotNull(tc, strstr(d->output, "You have selected"));
  CuAssertPtrNotNull(tc, strstr(d->output, listed_name));
  CuAssertIntEquals(tc, table[choice], *saved);
  CuAssertTrue(tc, *saved > 0);
  CuAssertTrue(tc, ok_call_mob_vnum(*saved));
}

void TestStudyCompanionMenuMatchesAcceptedTableChoices(CuTest *tc)
{
  struct study_pet_fixture fixture;
  int listed[STUDY_PET_MAX_LISTED];
  int first = 0, last = 0, count = 0, dire_wolf_choice = 0, previous = 0;
  int *saved;
  char first_name[STUDY_PET_NAME_SIZE];
  char last_name[STUDY_PET_NAME_SIZE];
  char dire_name[STUDY_PET_NAME_SIZE];

  begin_study_pet_fixture(tc, &fixture);
  saved = &GET_ANIMAL_COMPANION(&fixture.ch);
  *saved = 0;

  open_study_pet_menu(tc, &fixture, "6", "Animal Companion Menu");
  count = discover_listed_choices(fixture.descriptor.output, listed, STUDY_PET_MAX_LISTED, &first,
                                  &last);
  CuAssertTrue(tc, count >= 1);
  CuAssertTrue(tc, first >= 1);
  CuAssertTrue(tc, last >= first);
  CuAssertIntEquals(tc, first, listed[0]);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "select 0 (Zero)"));
  CuAssertTrue(tc,
               copy_listed_entry(fixture.descriptor.output, first, first_name, sizeof(first_name)));
  CuAssertTrue(tc,
               copy_listed_entry(fixture.descriptor.output, last, last_name, sizeof(last_name)));
  dire_wolf_choice = listed_number_containing(fixture.descriptor.output, "Dire Wolf");
  if (dire_wolf_choice > 0)
    CuAssertTrue(tc, copy_listed_entry(fixture.descriptor.output, dire_wolf_choice, dire_name,
                                       sizeof(dire_name)));

  study_parse_number(&fixture.descriptor, first);
  assert_saved_listed_choice(tc, &fixture.descriptor, saved, animal_vnums, first, first_name);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Current Companion:"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, first_name));

  if (last != first && last != dire_wolf_choice)
  {
    study_parse_number(&fixture.descriptor, last);
    assert_saved_listed_choice(tc, &fixture.descriptor, saved, animal_vnums, last, last_name);
    CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Current Companion:"));
    CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, last_name));
  }

  if (dire_wolf_choice > 0)
  {
    CLASS_LEVEL((&fixture.ch), CLASS_RANGER) = 3;
    CLASS_LEVEL((&fixture.ch), CLASS_WARRIOR) = 1;
    previous = *saved;
    study_parse_number(&fixture.descriptor, dire_wolf_choice);
    CuAssertIntEquals(tc, previous, *saved);
    CuAssertTrue(tc, *saved != MOB_DIRE_WOLF);
    CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "dire-wolf bond requires"));
    CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "You have selected"));

    CLASS_LEVEL((&fixture.ch), CLASS_RANGER) = 4;
    study_parse_number(&fixture.descriptor, dire_wolf_choice);
    assert_saved_listed_choice(tc, &fixture.descriptor, saved, animal_vnums, dire_wolf_choice,
                               dire_name);
    CuAssertIntEquals(tc, MOB_DIRE_WOLF, *saved);
    CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Current Companion:"));
    CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, dire_name));
  }

  previous = *saved;
  study_parse_number(&fixture.descriptor, last + 1);
  CuAssertIntEquals(tc, previous, *saved);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Not a valid choice!"));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "You have selected"));

  study_parse_choice(&fixture.descriptor, "-1");
  CuAssertIntEquals(tc, previous, *saved);
  CuAssertTrue(tc, *saved != -1);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Not a valid choice!"));

  study_parse_number(&fixture.descriptor, 0);
  CuAssertIntEquals(tc, 0, *saved);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "companion has been set to OFF"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Currently No Companion Selected"));

  study_parse_number(&fixture.descriptor, first);
  assert_saved_listed_choice(tc, &fixture.descriptor, saved, animal_vnums, first, first_name);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Current Companion:"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, first_name));

  end_study_pet_fixture(&fixture);
}

void TestStudyFamiliarMenuMatchesAcceptedTableChoices(CuTest *tc)
{
  struct study_pet_fixture fixture;
  int listed[STUDY_PET_MAX_LISTED];
  int first = 0, last = 0, count = 0, previous = 0;
  int *saved;
  char first_name[STUDY_PET_NAME_SIZE];
  char last_name[STUDY_PET_NAME_SIZE];

  begin_study_pet_fixture(tc, &fixture);
  saved = &GET_FAMILIAR(&fixture.ch);
  *saved = 0;

  open_study_pet_menu(tc, &fixture, "5", "Familiar Menu");
  count = discover_listed_choices(fixture.descriptor.output, listed, STUDY_PET_MAX_LISTED, &first,
                                  &last);
  CuAssertTrue(tc, count >= 1);
  CuAssertTrue(tc, first >= 1);
  CuAssertTrue(tc, last >= first);
  CuAssertIntEquals(tc, first, listed[0]);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "select 0 (Zero)"));
  CuAssertTrue(tc,
               copy_listed_entry(fixture.descriptor.output, first, first_name, sizeof(first_name)));
  CuAssertTrue(tc,
               copy_listed_entry(fixture.descriptor.output, last, last_name, sizeof(last_name)));

  study_parse_number(&fixture.descriptor, first);
  assert_saved_listed_choice(tc, &fixture.descriptor, saved, familiar_vnums, first, first_name);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Current Familiar:"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, first_name));

  if (last != first)
  {
    study_parse_number(&fixture.descriptor, last);
    assert_saved_listed_choice(tc, &fixture.descriptor, saved, familiar_vnums, last, last_name);
    CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Current Familiar:"));
    CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, last_name));
  }

  previous = *saved;
  study_parse_number(&fixture.descriptor, last + 1);
  CuAssertIntEquals(tc, previous, *saved);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Not a valid choice!"));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "You have selected"));

  study_parse_choice(&fixture.descriptor, "-1");
  CuAssertIntEquals(tc, previous, *saved);
  CuAssertTrue(tc, *saved != -1);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Not a valid choice!"));

  study_parse_number(&fixture.descriptor, 0);
  CuAssertIntEquals(tc, 0, *saved);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "familiar has been set to OFF"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Currently No Familiar Selected"));

  study_parse_number(&fixture.descriptor, first);
  assert_saved_listed_choice(tc, &fixture.descriptor, saved, familiar_vnums, first, first_name);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Current Familiar:"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, first_name));

  end_study_pet_fixture(&fixture);
}
