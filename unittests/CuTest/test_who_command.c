/* do_who unique-account count: the account table is sized by the live
 * descriptor list, not by the max_playing setting, so more connections than
 * max_playing are counted without reading past the table. */
#include "CuTest.h"
#include <string.h>
#include "conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act/act.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/interpreter.h"
#include "../../src/net/protocol.h"

#define WHO_TEST_PLAYERS 3

struct who_fixture
{
  struct char_data ch[WHO_TEST_PLAYERS];
  struct player_special_data specials[WHO_TEST_PLAYERS];
  struct descriptor_data descriptor[WHO_TEST_PLAYERS];
  struct account_data account[WHO_TEST_PLAYERS];
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct descriptor_data *saved_descriptor_list;
  int saved_max_playing;
};

static void begin_who_fixture(struct who_fixture *fixture)
{
  static const char *const account_names[WHO_TEST_PLAYERS] = {"Alpha", "Alpha", "Beta"};
  int i;

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_descriptor_list = descriptor_list;
  fixture->saved_max_playing = CONFIG_MAX_PLAYING;
  world = &fixture->room;
  top_of_world = 0;
  fixture->room.sector_type = SECT_INSIDE;
  descriptor_list = &fixture->descriptor[0];

  for (i = 0; i < WHO_TEST_PLAYERS; i++)
  {
    fixture->ch[i].player_specials = &fixture->specials[i];
    fixture->ch[i].desc = &fixture->descriptor[i];
    fixture->ch[i].player.name = CuMutableString(account_names[i]);
    fixture->ch[i].player.title = CuMutableString("");
    GET_LEVEL(&fixture->ch[i]) = LVL_IMMORT;
    IN_ROOM(&fixture->ch[i]) = 0;
    fixture->account[i].name = CuMutableString(account_names[i]);
    fixture->descriptor[i].character = &fixture->ch[i];
    fixture->descriptor[i].account = &fixture->account[i];
    fixture->descriptor[i].output = fixture->descriptor[i].small_outbuf;
    fixture->descriptor[i].bufspace = SMALL_BUFSIZE - 1;
    STATE(&fixture->descriptor[i]) = CON_PLAYING;
    if (i + 1 < WHO_TEST_PLAYERS)
      fixture->descriptor[i].next = &fixture->descriptor[i + 1];
  }
  /* the viewer must outrank LVL_IMMORT to see the account count */
  GET_LEVEL(&fixture->ch[0]) = LVL_IMPL;
  fixture->descriptor[0].pProtocol = ProtocolCreate();
}

static void end_who_fixture(struct who_fixture *fixture)
{
  ProtocolDestroy(fixture->descriptor[0].pProtocol);
  if (fixture->descriptor[0].large_outbuf != NULL)
  {
    free(fixture->descriptor[0].large_outbuf->text);
    free(fixture->descriptor[0].large_outbuf);
  }
  CONFIG_MAX_PLAYING = fixture->saved_max_playing;
  descriptor_list = fixture->saved_descriptor_list;
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
}

void TestWhoCountsUniqueAccountsBeyondMaxPlaying(CuTest *tc)
{
  struct who_fixture fixture;

  begin_who_fixture(&fixture);
  /* one slot per max_playing used to overrun here with three descriptors */
  CONFIG_MAX_PLAYING = 1;

  do_who(&fixture.ch[0], "", 0, 0);

  CuAssertPtrNotNull(
      tc, strstr(fixture.descriptor[0].output, "Number of unique accounts connected: 2."));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor[0].output, "Total visible players: 3."));
  end_who_fixture(&fixture);
}
