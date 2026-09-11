#include "CuTest.h"

#include "conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/constants.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/handler.h"
#include "../../src/mud_event.h"
#include "../../src/net/protocol.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASHENPORT_WELCOME_VNUM 103003
#define ASHENPORT_WELCOME_ROOM 103000
#define ASHENPORT_ELSEWHERE_ROOM 103001
#define ASHENPORT_TRIGGER_CAPACITY 4
#define ASHENPORT_LOG_LIMIT 8192
#define ASHENPORT_WAIT_CAP 8

struct ashenport_welcome_fixture
{
  struct room_data rooms[2];
  struct zone_data zone;
  struct index_data mobile_index;
  struct char_data mobile_prototype;
  struct char_data stack_actor;
  struct char_data replacement;
  struct descriptor_data actor_desc;
  struct descriptor_data replacement_desc;
  struct char_data *heap_actor;
  struct room_data *saved_world;
  struct zone_data *saved_zone_table;
  struct index_data *saved_mob_index;
  struct char_data *saved_mob_proto;
  struct index_data **saved_trig_index;
  struct trig_data *saved_trigger_list;
  struct char_data *saved_character_list;
  FILE *saved_logfile;
  FILE *log_file;
  room_rnum saved_top_of_world;
  zone_rnum saved_top_of_zone_table;
  mob_rnum saved_top_of_mobt;
  trig_rnum saved_top_of_trigt;
  unsigned long saved_pulse;
  trig_rnum attach_rnum;
  bool actor_extracted;
};

static const char *ashenport_test_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_ROOT");
  if (root == NULL || *root == '\0')
    root = ".";
  return root;
}

static void ashenport_reset_output(struct descriptor_data *descriptor)
{
  if (descriptor->output != NULL)
    comm_test_retain_unsent_output(descriptor, descriptor->output, descriptor->bufptr);
  descriptor->output = descriptor->small_outbuf;
  descriptor->small_outbuf[0] = '\0';
  descriptor->bufptr = 0;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
}

static void ashenport_bind_descriptor(struct descriptor_data *descriptor, struct char_data *ch)
{
  memset(descriptor, 0, sizeof(*descriptor));
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
  descriptor->character = ch;
  STATE(descriptor) = CON_PLAYING;
  descriptor->pProtocol = ProtocolCreate();
  ch->desc = descriptor;
}

static void ashenport_initialize_npc(struct char_data *ch, const char *name)
{
  clear_char(ch);
  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &dummy_mob;
  ch->player.name = (char *)name;
  ch->player.short_descr = (char *)name;
  GET_LEVEL(ch) = 10;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100;
  GET_MAX_HIT(ch) = 100;
  GET_MOVE(ch) = 100;
  GET_MAX_MOVE(ch) = 100;
}

static void ashenport_free_prototypes(void)
{
  struct cmdlist_element *command;
  struct cmdlist_element *next_command;
  struct index_data *index;
  int i;

  for (i = 0; i < top_of_trigt; i++)
  {
    index = trig_index[i];
    if (index == NULL)
      continue;

    command = ((struct trig_data *)index->proto)->cmdlist;
    free_trigger((struct trig_data *)index->proto);
    while (command != NULL)
    {
      next_command = command->next;
      free(command->cmd);
      free(command);
      command = next_command;
    }
    free(index);
  }
}

static bool ashenport_parse_trigger_vnum(FILE *trigger_file, trig_vnum vnum)
{
  char line[READ_SIZE];
  trig_rnum before;

  before = top_of_trigt;
  while (get_line(trigger_file, line))
  {
    if (strcmp(line, "#103003") != 0)
      continue;
    parse_trigger(trigger_file, (int)vnum);
    return top_of_trigt == (int)before + 1 && trig_index[before] != NULL &&
           trig_index[before]->vnum == vnum;
  }
  return false;
}

static bool ashenport_open_and_parse(const char *relative_path, trig_vnum vnum)
{
  char path[PATH_MAX];
  FILE *trigger_file;
  bool loaded;

  if (snprintf(path, sizeof(path), "%s/%s", ashenport_test_root(), relative_path) >=
      (int)sizeof(path))
    return false;
  trigger_file = fopen(path, "r");
  if (trigger_file == NULL)
    return false;
  loaded = ashenport_parse_trigger_vnum(trigger_file, vnum);
  fclose(trigger_file);
  return loaded;
}

static bool ashenport_cmdlists_match(const struct trig_data *left, const struct trig_data *right)
{
  const struct cmdlist_element *left_cmd;
  const struct cmdlist_element *right_cmd;

  if (left == NULL || right == NULL)
    return false;
  if (left->attach_type != right->attach_type || left->trigger_type != right->trigger_type ||
      left->narg != right->narg)
    return false;

  left_cmd = left->cmdlist;
  right_cmd = right->cmdlist;
  while (left_cmd != NULL && right_cmd != NULL)
  {
    if (left_cmd->cmd == NULL || right_cmd->cmd == NULL ||
        strcmp(left_cmd->cmd, right_cmd->cmd) != 0)
      return false;
    left_cmd = left_cmd->next;
    right_cmd = right_cmd->next;
  }
  return left_cmd == NULL && right_cmd == NULL;
}

static bool ashenport_trigger_is_enter_room(const struct trig_data *trigger)
{
  return trigger != NULL && trigger->attach_type == WLD_TRIGGER &&
         IS_SET(GET_TRIG_TYPE(trigger), WTRIG_ENTER);
}

static bool ashenport_load_shipped_trigger(struct ashenport_welcome_fixture *fixture)
{
  struct trig_data *canonical;
  struct trig_data *world_copy;
  char world_path[PATH_MAX];
  FILE *world_file;

  if (!ashenport_open_and_parse("unittests/CuTest/fixtures/ashenport_welcome/103003.trg",
                                ASHENPORT_WELCOME_VNUM))
    return false;

  canonical = (struct trig_data *)trig_index[0]->proto;
  if (!ashenport_trigger_is_enter_room(canonical))
    return false;
  fixture->attach_rnum = 0;

  if (snprintf(world_path, sizeof(world_path), "%s/lib/world/trg/1030.trg",
               ashenport_test_root()) >= (int)sizeof(world_path))
    return false;
  world_file = fopen(world_path, "r");
  if (world_file == NULL)
    return true;
  fclose(world_file);

  if (!ashenport_open_and_parse("lib/world/trg/1030.trg", ASHENPORT_WELCOME_VNUM))
    return false;
  world_copy = (struct trig_data *)trig_index[1]->proto;
  if (!ashenport_trigger_is_enter_room(world_copy) ||
      !ashenport_cmdlists_match(canonical, world_copy))
    return false;
  fixture->attach_rnum = 1;
  return true;
}

static bool ashenport_attach_welcome_trigger(struct ashenport_welcome_fixture *fixture)
{
  struct trig_data *trigger;
  struct room_data *room;

  room = &fixture->rooms[0];
  if (SCRIPT(room) == NULL)
    SCRIPT(room) = calloc(1, sizeof(*SCRIPT(room)));
  if (SCRIPT(room) == NULL)
    return false;

  trigger = read_trigger(fixture->attach_rnum);
  if (trigger == NULL)
    return false;
  add_trigger(SCRIPT(room), trigger, -1);
  return TRIGGERS(SCRIPT(room)) != NULL;
}

static bool ashenport_fixture_begin(struct ashenport_welcome_fixture *fixture)
{
  memset(fixture, 0, sizeof(*fixture));

  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_zone_table = zone_table;
  fixture->saved_top_of_zone_table = top_of_zone_table;
  fixture->saved_mob_index = mob_index;
  fixture->saved_mob_proto = mob_proto;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->saved_trig_index = trig_index;
  fixture->saved_top_of_trigt = top_of_trigt;
  fixture->saved_trigger_list = trigger_list;
  fixture->saved_character_list = character_list;
  fixture->saved_logfile = logfile;
  fixture->saved_pulse = pulse;

  fixture->rooms[0].number = ASHENPORT_WELCOME_ROOM;
  fixture->rooms[0].zone = 0;
  fixture->rooms[0].sector_type = SECT_INSIDE;
  fixture->rooms[0].name = "Ashenport welcome";
  fixture->rooms[0].description = "The Ashenport welcome room.\r\n";
  fixture->rooms[1].number = ASHENPORT_ELSEWHERE_ROOM;
  fixture->rooms[1].zone = 0;
  fixture->rooms[1].sector_type = SECT_INSIDE;
  fixture->rooms[1].name = "Ashenport elsewhere";
  fixture->rooms[1].description = "A second Ashenport room.\r\n";
  fixture->zone.number = 0;
  fixture->zone.bot = ASHENPORT_WELCOME_ROOM;
  fixture->zone.top = ASHENPORT_ELSEWHERE_ROOM;
  fixture->zone.min_level = -1;
  fixture->zone.max_level = LVL_IMPL;
  fixture->mobile_index.vnum = 9100;

  world = fixture->rooms;
  top_of_world = 1;
  zone_table = &fixture->zone;
  top_of_zone_table = 0;
  mob_index = &fixture->mobile_index;
  top_of_mobt = 0;
  ashenport_initialize_npc(&fixture->mobile_prototype, "ashenport welcome proto");
  fixture->mobile_prototype.player.name = "ashenport welcome proto";
  fixture->mobile_prototype.nr = 0;
  GET_PSP(&fixture->mobile_prototype) = 100;
  mob_proto = &fixture->mobile_prototype;

  trig_index = calloc(ASHENPORT_TRIGGER_CAPACITY, sizeof(*trig_index));
  top_of_trigt = 0;
  trigger_list = NULL;
  if (trig_index == NULL)
    return false;

  event_free_all();
  if (!event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER))
    return false;
  event_init();
  if (!dg_wait_runtime_init())
    return false;

  fixture->log_file = tmpfile();
  if (fixture->log_file == NULL)
    return false;
  logfile = fixture->log_file;

  ashenport_initialize_npc(&fixture->stack_actor, "ashenport welcome actor");
  ashenport_initialize_npc(&fixture->replacement, "ashenport welcome replacement");

  return ashenport_load_shipped_trigger(fixture) && ashenport_attach_welcome_trigger(fixture);
}

static void ashenport_destroy_descriptor(struct descriptor_data *descriptor)
{
  if (descriptor->character != NULL && descriptor->character->desc == descriptor)
    descriptor->character->desc = NULL;
  descriptor->character = NULL;
  if (descriptor->pProtocol != NULL)
  {
    ProtocolDestroy(descriptor->pProtocol);
    descriptor->pProtocol = NULL;
  }
}

static void ashenport_fixture_end(struct ashenport_welcome_fixture *fixture)
{
  if (fixture->heap_actor != NULL && !fixture->actor_extracted && !DEAD(fixture->heap_actor))
  {
    extract_char(fixture->heap_actor);
    extract_pending_chars();
    fixture->actor_extracted = true;
    fixture->heap_actor = NULL;
  }

  if (IN_ROOM(&fixture->stack_actor) != NOWHERE)
    char_from_room(&fixture->stack_actor);
  if (IN_ROOM(&fixture->replacement) != NOWHERE)
    char_from_room(&fixture->replacement);

  if (SCRIPT(&fixture->rooms[0]) != NULL)
    extract_script(&fixture->rooms[0].script);

  if (GET_ID(&fixture->stack_actor) != 0)
    remove_from_lookup_table(GET_ID(&fixture->stack_actor));
  if (GET_ID(&fixture->replacement) != 0)
    remove_from_lookup_table(GET_ID(&fixture->replacement));

  clear_char_event_list(&fixture->stack_actor);
  clear_char_event_list(&fixture->replacement);
  ashenport_destroy_descriptor(&fixture->actor_desc);
  ashenport_destroy_descriptor(&fixture->replacement_desc);

  ashenport_free_prototypes();
  free(trig_index);

  if (fixture->log_file != NULL)
    fclose(fixture->log_file);

  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  zone_table = fixture->saved_zone_table;
  top_of_zone_table = fixture->saved_top_of_zone_table;
  mob_index = fixture->saved_mob_index;
  mob_proto = fixture->saved_mob_proto;
  top_of_mobt = fixture->saved_top_of_mobt;
  trig_index = fixture->saved_trig_index;
  top_of_trigt = fixture->saved_top_of_trigt;
  trigger_list = fixture->saved_trigger_list;
  character_list = fixture->saved_character_list;
  logfile = fixture->saved_logfile;
  pulse = fixture->saved_pulse;
  event_free_all();
}

static struct trig_data *ashenport_live_trigger(struct ashenport_welcome_fixture *fixture)
{
  if (SCRIPT(&fixture->rooms[0]) == NULL)
    return NULL;
  return TRIGGERS(SCRIPT(&fixture->rooms[0]));
}

static void ashenport_drain_waits(struct trig_data *trigger)
{
  int resumes;

  for (resumes = 0;
       resumes < ASHENPORT_WAIT_CAP && trigger != NULL && dg_trigger_wait_is_live(trigger);
       resumes++)
  {
    pulse += dg_trigger_wait_remaining(trigger);
    event_test_advance();
  }
}

static void ashenport_copy_log(struct ashenport_welcome_fixture *fixture, char *buffer, size_t size)
{
  long pos;
  size_t bytes_read;

  buffer[0] = '\0';
  if (fixture->log_file == NULL || size == 0)
    return;

  pos = ftell(fixture->log_file);
  if (pos < 0)
    return;
  rewind(fixture->log_file);
  bytes_read = fread(buffer, 1, size - 1, fixture->log_file);
  buffer[bytes_read] = '\0';
  fseek(fixture->log_file, pos, SEEK_SET);
}

static bool ashenport_log_has_wsend_miss(struct ashenport_welcome_fixture *fixture)
{
  char log_text[ASHENPORT_LOG_LIMIT];

  ashenport_copy_log(fixture, log_text, sizeof(log_text));
  return strstr(log_text, "no target found for wsend") != NULL;
}

static struct char_data *ashenport_place_stack_actor(struct ashenport_welcome_fixture *fixture)
{
  ashenport_bind_descriptor(&fixture->actor_desc, &fixture->stack_actor);
  if (fixture->actor_desc.pProtocol == NULL)
    return NULL;
  char_to_room(&fixture->stack_actor, 0);
  ashenport_reset_output(&fixture->actor_desc);
  return &fixture->stack_actor;
}

static struct char_data *ashenport_place_heap_actor(struct ashenport_welcome_fixture *fixture)
{
  fixture->heap_actor = read_mobile(0, REAL);
  if (fixture->heap_actor == NULL)
    return NULL;

  ashenport_bind_descriptor(&fixture->actor_desc, fixture->heap_actor);
  if (fixture->actor_desc.pProtocol == NULL)
    return NULL;
  if (IN_ROOM(fixture->heap_actor) != NOWHERE)
    char_from_room(fixture->heap_actor);
  char_to_room(fixture->heap_actor, 0);
  ashenport_reset_output(&fixture->actor_desc);
  return fixture->heap_actor;
}

static bool ashenport_enter_welcome(struct ashenport_welcome_fixture *fixture,
                                    struct char_data *actor)
{
  struct trig_data *trigger;

  trigger = ashenport_live_trigger(fixture);
  if (trigger == NULL || actor == NULL || IN_ROOM(actor) != 0)
    return false;
  enter_wtrigger(&fixture->rooms[0], actor, -1);
  return dg_trigger_wait_is_live(trigger);
}

void Test_ashenport_welcome_trigger_stay_receives_full_walkto_sequence(CuTest *tc)
{
  struct ashenport_welcome_fixture fixture;
  struct char_data *actor;
  const char *output;
  bool started;

  started = ashenport_fixture_begin(&fixture);
  actor = started ? ashenport_place_stack_actor(&fixture) : NULL;
  CuAssertTrue(tc, started);
  CuAssertPtrNotNull(tc, actor);
  CuAssertTrue(tc, ashenport_enter_welcome(&fixture, actor));

  ashenport_drain_waits(ashenport_live_trigger(&fixture));
  output = fixture.actor_desc.output;
  CuAssertPtrNotNull(tc, strstr(output, "Welcome to Ashenport!"));
  CuAssertPtrNotNull(tc,
                     strstr(output, "To travel easily within the city, use the walkto system."));
  CuAssertPtrNotNull(tc, strstr(output, "See HELP WALKTO for more information."));
  CuAssertTrue(tc, !ashenport_log_has_wsend_miss(&fixture));
  CuAssertTrue(tc, !dg_trigger_wait_is_live(ashenport_live_trigger(&fixture)));
  ashenport_fixture_end(&fixture);
}

void Test_ashenport_welcome_trigger_leave_halts_remaining_sends(CuTest *tc)
{
  struct ashenport_welcome_fixture fixture;
  struct char_data *actor;
  struct trig_data *trigger;
  const char *output;
  bool started;

  started = ashenport_fixture_begin(&fixture);
  actor = started ? ashenport_place_stack_actor(&fixture) : NULL;
  CuAssertTrue(tc, started);
  CuAssertPtrNotNull(tc, actor);
  CuAssertTrue(tc, ashenport_enter_welcome(&fixture, actor));

  trigger = ashenport_live_trigger(&fixture);
  pulse += dg_trigger_wait_remaining(trigger);
  event_test_advance();
  CuAssertPtrNotNull(tc, strstr(fixture.actor_desc.output, "Welcome to Ashenport!"));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.actor_desc.output, "walkto system"));

  char_from_room(actor);
  char_to_room(actor, 1);
  ashenport_reset_output(&fixture.actor_desc);
  ashenport_drain_waits(trigger);
  output = fixture.actor_desc.output;
  CuAssertPtrEquals(tc, NULL, strstr(output, "Welcome to Ashenport!"));
  CuAssertPtrEquals(tc, NULL, strstr(output, "walkto system"));
  CuAssertPtrEquals(tc, NULL, strstr(output, "HELP WALKTO"));
  CuAssertTrue(tc, !ashenport_log_has_wsend_miss(&fixture));
  ashenport_fixture_end(&fixture);
}

void Test_ashenport_welcome_trigger_extract_does_not_log_missing_wsend(CuTest *tc)
{
  struct ashenport_welcome_fixture fixture;
  struct char_data *actor;
  struct trig_data *trigger;
  bool started;

  started = ashenport_fixture_begin(&fixture);
  actor = started ? ashenport_place_heap_actor(&fixture) : NULL;
  CuAssertTrue(tc, started);
  CuAssertPtrNotNull(tc, actor);
  CuAssertTrue(tc, ashenport_enter_welcome(&fixture, actor));

  trigger = ashenport_live_trigger(&fixture);
  extract_char(actor);
  extract_pending_chars();
  fixture.actor_extracted = true;
  fixture.heap_actor = NULL;
  fixture.actor_desc.character = NULL;

  ashenport_drain_waits(trigger);
  CuAssertTrue(tc, !ashenport_log_has_wsend_miss(&fixture));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.actor_desc.output, "Welcome to Ashenport!"));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.actor_desc.output, "walkto system"));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.actor_desc.output, "HELP WALKTO"));
  ashenport_fixture_end(&fixture);
}

void Test_ashenport_welcome_trigger_replacement_does_not_receive_remainder(CuTest *tc)
{
  struct ashenport_welcome_fixture fixture;
  struct char_data *actor;
  struct trig_data *trigger;
  const char *replacement_output;
  bool started;

  started = ashenport_fixture_begin(&fixture);
  actor = started ? ashenport_place_stack_actor(&fixture) : NULL;
  if (started)
    ashenport_bind_descriptor(&fixture.replacement_desc, &fixture.replacement);
  CuAssertTrue(tc, started);
  CuAssertPtrNotNull(tc, actor);
  CuAssertPtrNotNull(tc, fixture.replacement_desc.pProtocol);
  CuAssertTrue(tc, ashenport_enter_welcome(&fixture, actor));

  trigger = ashenport_live_trigger(&fixture);
  char_from_room(actor);
  char_to_room(actor, 1);
  char_to_room(&fixture.replacement, 0);
  ashenport_reset_output(&fixture.actor_desc);
  ashenport_reset_output(&fixture.replacement_desc);
  ashenport_drain_waits(trigger);

  replacement_output = fixture.replacement_desc.output;
  CuAssertPtrEquals(tc, NULL, strstr(replacement_output, "Welcome to Ashenport!"));
  CuAssertPtrEquals(tc, NULL, strstr(replacement_output, "walkto system"));
  CuAssertPtrEquals(tc, NULL, strstr(replacement_output, "HELP WALKTO"));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.actor_desc.output, "Welcome to Ashenport!"));
  CuAssertTrue(tc, !ashenport_log_has_wsend_miss(&fixture));
  ashenport_fixture_end(&fixture);
}
