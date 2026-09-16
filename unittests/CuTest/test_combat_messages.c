/* Contract tests for src/combat/combat_messages.c.
 *
 * These cover the presentation half of the combat split: the fallback
 * protocol between skill_message() and dam_message(), the weapon token
 * substitution that makes generic damage text read correctly, the attack-type
 * bound dam_message() enforces, and the stand-in object a Trelux attack loads.
 * Resolution is not exercised here - nothing in this file decides an outcome.
 *
 * Every test records its answers, tears the fixture down, and only then
 * asserts. CuTest longjmp()s out of a failed assertion, so an assert placed
 * before teardown would leave world, character_list, obj_proto, zone_table and
 * logfile pointing into a dead stack frame for every later test. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/constants.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/core/interpreter.h"
#include "../../src/combat/combat_messages.h"
#include "../../src/combat/fight.h"
#include "../../src/net/protocol.h"

#include <stdlib.h>
#include <string.h>

/* An attack type no authored message can carry, used to drive the
 * no-message-found path deterministically. Entries loaded from
 * lib/misc/messages are never negative. */
#define COMBAT_MESSAGES_UNKNOWN_ATTACK (-4242)

/* TRELUX_CLAWS in combat_messages.c. */
#define COMBAT_MESSAGES_TRELUX_CLAWS 800

struct combat_messages_fixture
{
  struct room_data room;
  struct char_data attacker;
  struct char_data victim;
  struct descriptor_data attacker_desc;
  struct descriptor_data victim_desc;
  struct player_special_data attacker_specials;
  struct player_special_data victim_specials;

  /* One object and its zone, so read_object(TRELUX_CLAWS) resolves without a
   * booted world. */
  struct obj_data claw_proto;
  struct index_data claw_index;
  struct zone_data claw_zone;

  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct char_data *saved_character_list;
  struct obj_data *saved_obj_proto;
  struct index_data *saved_obj_index;
  struct obj_data *saved_object_list;
  obj_rnum saved_top_of_objt;
  struct zone_data *saved_zone_table;
  zone_rnum saved_top_of_zone_table;
  FILE *saved_logfile;
  FILE *log_capture;
};

static void combat_messages_reset_output(struct descriptor_data *descriptor)
{
  if (descriptor->output != NULL)
    comm_test_retain_unsent_output(descriptor, descriptor->output, descriptor->bufptr);
  descriptor->output = descriptor->small_outbuf;
  descriptor->small_outbuf[0] = '\0';
  descriptor->bufptr = 0;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
}

static void combat_messages_bind_descriptor(struct descriptor_data *descriptor,
                                            struct char_data *ch)
{
  memset(descriptor, 0, sizeof(*descriptor));
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
  descriptor->character = ch;
  STATE(descriptor) = CON_PLAYING;
  descriptor->pProtocol = ProtocolCreate();
  ch->desc = descriptor;
}

static void combat_messages_init_char(struct char_data *ch, struct player_special_data *specials,
                                      const char *name)
{
  clear_char(ch);
  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  memset(specials, 0, sizeof(*specials));
  ch->player_specials = specials;
  ch->player.name = CuMutableString(name);
  ch->player.short_descr = CuMutableString(name);
  GET_LEVEL(ch) = 10;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100;
  GET_MAX_HIT(ch) = 100;
  GET_REAL_RACE(ch) = RACE_HUMAN;
}

static void combat_messages_setup(struct combat_messages_fixture *fix)
{
  memset(fix, 0, sizeof(*fix));

  fix->saved_world = world;
  fix->saved_top_of_world = top_of_world;
  fix->saved_character_list = character_list;
  fix->saved_obj_proto = obj_proto;
  fix->saved_obj_index = obj_index;
  fix->saved_object_list = object_list;
  fix->saved_top_of_objt = top_of_objt;
  fix->saved_zone_table = zone_table;
  fix->saved_top_of_zone_table = top_of_zone_table;

  memset(&fix->room, 0, sizeof(fix->room));
  fix->room.number = 1;
  world = &fix->room;
  top_of_world = 0;
  character_list = NULL;

  /* Instances share prototype strings and free_obj() leaves those alone, so
   * arena storage is safe. */
  fix->claw_proto.item_number = 0;
  fix->claw_proto.name = CuMutableString("claws claw trelux");
  fix->claw_proto.short_description = CuMutableString("a pair of claws");
  fix->claw_proto.description = CuMutableString("A pair of claws lies here.");
  fix->claw_index.vnum = COMBAT_MESSAGES_TRELUX_CLAWS;
  obj_proto = &fix->claw_proto;
  obj_index = &fix->claw_index;
  top_of_objt = 0;
  object_list = NULL;

  fix->claw_zone.number = 8;
  fix->claw_zone.bot = 800;
  fix->claw_zone.top = 899;
  zone_table = &fix->claw_zone;
  top_of_zone_table = 0;

  /* Capture log() so a test can assert on a SYSERR without printing it. */
  fix->saved_logfile = logfile;
  fix->log_capture = tmpfile();
  if (fix->log_capture != NULL)
    logfile = fix->log_capture;

  combat_messages_init_char(&fix->attacker, &fix->attacker_specials, "the attacker");
  combat_messages_init_char(&fix->victim, &fix->victim_specials, "the victim");
  combat_messages_bind_descriptor(&fix->attacker_desc, &fix->attacker);
  combat_messages_bind_descriptor(&fix->victim_desc, &fix->victim);

  IN_ROOM(&fix->attacker) = 0;
  IN_ROOM(&fix->victim) = 0;
  fix->room.people = &fix->attacker;
  fix->attacker.next_in_room = &fix->victim;
  fix->victim.next_in_room = NULL;
}

/* Copy whatever the code under test logged. Call before teardown. */
static void combat_messages_captured_log(struct combat_messages_fixture *fix, char *out,
                                         size_t size)
{
  long length = 0;
  size_t wanted = 0;
  size_t got = 0;

  out[0] = '\0';
  if (fix->log_capture == NULL || size == 0)
    return;

  fflush(fix->log_capture);
  length = ftell(fix->log_capture);
  if (length <= 0)
    return;

  rewind(fix->log_capture);
  wanted = (size_t)length < size - 1 ? (size_t)length : size - 1;
  got = fread(out, 1, wanted, fix->log_capture);
  out[got] = '\0';
  fseek(fix->log_capture, 0, SEEK_END);
}

static void combat_messages_teardown(struct combat_messages_fixture *fix)
{
  if (fix->attacker_desc.pProtocol != NULL)
    ProtocolDestroy(fix->attacker_desc.pProtocol);
  if (fix->victim_desc.pProtocol != NULL)
    ProtocolDestroy(fix->victim_desc.pProtocol);
  /* Fixture names come from CuMutableString(), which hands out storage from a
   * static arena. They must not be freed. */

  logfile = fix->saved_logfile;
  if (fix->log_capture != NULL)
  {
    fclose(fix->log_capture);
    fix->log_capture = NULL;
  }

  world = fix->saved_world;
  top_of_world = fix->saved_top_of_world;
  character_list = fix->saved_character_list;
  obj_proto = fix->saved_obj_proto;
  obj_index = fix->saved_obj_index;
  object_list = fix->saved_object_list;
  top_of_objt = fix->saved_top_of_objt;
  zone_table = fix->saved_zone_table;
  top_of_zone_table = fix->saved_top_of_zone_table;
}

void Test_combat_messages_report_no_message_for_an_unmatched_attack(CuTest *tc)
{
  struct combat_messages_fixture fix;
  int result;

  combat_messages_setup(&fix);

  /* combat_messages.h documents this as the signal that tells fight.c to
   * fall back to dam_message(). If it ever returned something else, hits
   * with no authored message would be silent. */
  result = skill_message(10, &fix.attacker, &fix.victim, COMBAT_MESSAGES_UNKNOWN_ATTACK, 0);

  combat_messages_teardown(&fix);

  CuAssertIntEquals(tc, SKILL_MESSAGE_MISS_FAIL, result);
}

void Test_combat_messages_wrapper_matches_the_projectile_form(CuTest *tc)
{
  struct combat_messages_fixture fix;
  int wrapped;
  int direct;

  combat_messages_setup(&fix);

  /* skill_message() is documented as skill_message_with_projectile() with no
   * missile. Keep the two in step so callers can pick either. */
  wrapped = skill_message(10, &fix.attacker, &fix.victim, COMBAT_MESSAGES_UNKNOWN_ATTACK, 0);
  direct = skill_message_with_projectile(10, &fix.attacker, &fix.victim,
                                         COMBAT_MESSAGES_UNKNOWN_ATTACK, 0, NULL);

  combat_messages_teardown(&fix);

  CuAssertIntEquals(tc, wrapped, direct);
}

void Test_combat_messages_release_the_trelux_stand_in_object(CuTest *tc)
{
  struct combat_messages_fixture fix;
  struct obj_data *leaked;
  int still_counted;

  combat_messages_setup(&fix);

  /* A Trelux natural attack loads a stand-in object so act() has something to
   * name. read_object() links it into object_list and counts it in
   * obj_index[].number, so any path out that skips extracting it leaks one
   * object per combat message for the life of the boot. combat_messages.h
   * promises callers see no allocation. */
  GET_REAL_RACE(&fix.attacker) = RACE_TRELUX;
  skill_message(10, &fix.attacker, &fix.victim, COMBAT_MESSAGES_UNKNOWN_ATTACK, 0);

  leaked = object_list;
  still_counted = fix.claw_index.number;

  combat_messages_teardown(&fix);

  CuAssertPtrEquals(tc, NULL, leaked);
  CuAssertIntEquals(tc, 0, still_counted);
}

void Test_combat_messages_dam_message_tells_both_participants(CuTest *tc)
{
  struct combat_messages_fixture fix;
  bool attacker_told;
  bool victim_told;

  combat_messages_setup(&fix);
  combat_messages_reset_output(&fix.attacker_desc);
  combat_messages_reset_output(&fix.victim_desc);

  /* dam_message() is the fallback of last resort for a living victim: unlike
   * skill_message() it has no "no message" answer, so both participants must
   * be told something. */
  dam_message(10, &fix.attacker, &fix.victim, TYPE_HIT, 0, NULL);

  attacker_told = strlen(fix.attacker_desc.small_outbuf) > 0;
  victim_told = strlen(fix.victim_desc.small_outbuf) > 0;

  combat_messages_teardown(&fix);

  CuAssertTrue(tc, attacker_told);
  CuAssertTrue(tc, victim_told);
}

void Test_combat_messages_dam_message_is_silent_for_a_dead_victim(CuTest *tc)
{
  struct combat_messages_fixture fix;
  bool attacker_told;
  bool victim_told;

  combat_messages_setup(&fix);
  combat_messages_reset_output(&fix.attacker_desc);
  combat_messages_reset_output(&fix.victim_desc);

  /* dam_weapons[] has no death text, so dam_message() renders nothing once the
   * victim is at POS_DEAD. combat_messages.h says so, and
   * damage_with_projectile() relies on it to know there is no fallback left. */
  GET_POS(&fix.victim) = POS_DEAD;
  dam_message(50, &fix.attacker, &fix.victim, TYPE_HIT, 0, NULL);

  attacker_told = strlen(fix.attacker_desc.small_outbuf) > 0;
  victim_told = strlen(fix.victim_desc.small_outbuf) > 0;

  combat_messages_teardown(&fix);

  CuAssertTrue(tc, !attacker_told);
  CuAssertTrue(tc, !victim_told);
}

void Test_combat_messages_dam_message_rejects_a_non_weapon_type(CuTest *tc)
{
  struct combat_messages_fixture fix;
  char logged[MAX_STRING_LENGTH];
  bool victim_told;

  combat_messages_setup(&fix);
  combat_messages_reset_output(&fix.victim_desc);

  /* dam_message() indexes attack_hit_text[] by w_type - TYPE_HIT, and that
   * table has exactly NUM_ATTACK_TYPES entries. An out-of-range type must be
   * refused, not read past the table into two wild pointers for
   * replace_string() to copy from. */
  dam_message(10, &fix.attacker, &fix.victim, BOT_WEAPON_TYPES, 0, NULL);

  victim_told = strlen(fix.victim_desc.small_outbuf) > 0;
  combat_messages_captured_log(&fix, logged, sizeof(logged));

  combat_messages_teardown(&fix);

  CuAssertTrue(tc, !victim_told);
  CuAssertPtrNotNull(tc, strstr(logged, "dam_message"));
}

void Test_combat_messages_substitute_the_weapon_noun(CuTest *tc)
{
  struct combat_messages_fixture fix;
  const char *singular;
  char rendered[SMALL_BUFSIZE];

  combat_messages_setup(&fix);
  combat_messages_reset_output(&fix.victim_desc);

  /* The generic damage text carries #w / #W tokens that stand in for the
   * attack's weapon noun. dam_message() rebases w_type onto attack_hit_text[],
   * so TYPE_HIT selects entry zero. A substitution failure would leave the
   * raw token in front of players, so assert the noun arrives and the token
   * does not. */
  singular = attack_hit_text[0].singular;
  dam_message(10, &fix.attacker, &fix.victim, TYPE_HIT, 0, NULL);
  snprintf(rendered, sizeof(rendered), "%s", fix.victim_desc.small_outbuf);

  combat_messages_teardown(&fix);

  CuAssertPtrNotNull(tc, singular);
  CuAssertPtrNotNull(tc, strstr(rendered, singular));
  CuAssertPtrEquals(tc, NULL, strstr(rendered, "#w"));
  CuAssertPtrEquals(tc, NULL, strstr(rendered, "#W"));
}

void Test_combat_messages_scale_text_with_the_damage_fraction(CuTest *tc)
{
  struct combat_messages_fixture fix;
  char light[SMALL_BUFSIZE];
  char heavy[SMALL_BUFSIZE];

  combat_messages_setup(&fix);

  /* The message tier is chosen from damage as a fraction of the victim's
   * health, which is the one piece of judgement this module owns. A scratch
   * and a near-killing blow must not read the same. */
  combat_messages_reset_output(&fix.victim_desc);
  GET_HIT(&fix.victim) = 100;
  GET_MAX_HIT(&fix.victim) = 100;
  dam_message(1, &fix.attacker, &fix.victim, TYPE_HIT, 0, NULL);
  snprintf(light, sizeof(light), "%s", fix.victim_desc.small_outbuf);

  combat_messages_reset_output(&fix.victim_desc);
  GET_HIT(&fix.victim) = 100;
  GET_MAX_HIT(&fix.victim) = 100;
  dam_message(95, &fix.attacker, &fix.victim, TYPE_HIT, 0, NULL);
  snprintf(heavy, sizeof(heavy), "%s", fix.victim_desc.small_outbuf);

  combat_messages_teardown(&fix);

  CuAssertTrue(tc, strlen(light) > 0);
  CuAssertTrue(tc, strlen(heavy) > 0);
  CuAssertTrue(tc, strcmp(light, heavy) != 0);
}
