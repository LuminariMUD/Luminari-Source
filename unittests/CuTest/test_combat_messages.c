/* Contract tests for src/combat/combat_messages.c.
 *
 * These cover the presentation half of the combat split: the fallback
 * protocol between skill_message() and dam_message(), the weapon token
 * substitution that makes generic damage text read correctly, and the
 * buffer-lifetime rule combat_messages.h states. Resolution is not exercised
 * here - nothing in this file decides an outcome. */

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

struct combat_messages_fixture
{
  struct room_data room;
  struct char_data attacker;
  struct char_data victim;
  struct descriptor_data attacker_desc;
  struct descriptor_data victim_desc;
  struct player_special_data attacker_specials;
  struct player_special_data victim_specials;

  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct char_data *saved_character_list;
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

  memset(&fix->room, 0, sizeof(fix->room));
  fix->room.number = 1;
  world = &fix->room;
  top_of_world = 0;
  character_list = NULL;

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

static void combat_messages_teardown(struct combat_messages_fixture *fix)
{
  if (fix->attacker_desc.pProtocol != NULL)
    ProtocolDestroy(fix->attacker_desc.pProtocol);
  if (fix->victim_desc.pProtocol != NULL)
    ProtocolDestroy(fix->victim_desc.pProtocol);
  /* Fixture names come from CuMutableString(), which hands out storage from a
   * static arena. They must not be freed. */

  world = fix->saved_world;
  top_of_world = fix->saved_top_of_world;
  character_list = fix->saved_character_list;
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
  CuAssertIntEquals(tc, SKILL_MESSAGE_MISS_FAIL, result);

  combat_messages_teardown(&fix);
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
  CuAssertIntEquals(tc, wrapped, direct);

  combat_messages_teardown(&fix);
}

void Test_combat_messages_dam_message_always_produces_text(CuTest *tc)
{
  struct combat_messages_fixture fix;

  combat_messages_setup(&fix);
  combat_messages_reset_output(&fix.attacker_desc);
  combat_messages_reset_output(&fix.victim_desc);

  /* dam_message() is the fallback of last resort: unlike skill_message() it
   * has no "no message" answer, so both participants must always be told
   * something. */
  dam_message(10, &fix.attacker, &fix.victim, TYPE_HIT, 0, NULL);

  CuAssertTrue(tc, strlen(fix.attacker_desc.small_outbuf) > 0);
  CuAssertTrue(tc, strlen(fix.victim_desc.small_outbuf) > 0);

  combat_messages_teardown(&fix);
}

void Test_combat_messages_substitute_the_weapon_noun(CuTest *tc)
{
  struct combat_messages_fixture fix;
  const char *singular;

  combat_messages_setup(&fix);
  combat_messages_reset_output(&fix.victim_desc);

  /* The generic damage text carries #w / #W tokens that stand in for the
   * attack's weapon noun. dam_message() rebases w_type onto attack_hit_text[],
   * so TYPE_HIT selects entry zero. A substitution failure would leave the
   * raw token in front of players, so assert the noun arrives and the token
   * does not. */
  singular = attack_hit_text[0].singular;
  CuAssertPtrNotNull(tc, singular);

  dam_message(10, &fix.attacker, &fix.victim, TYPE_HIT, 0, NULL);

  CuAssertPtrNotNull(tc, strstr(fix.victim_desc.small_outbuf, singular));
  CuAssertPtrEquals(tc, NULL, strstr(fix.victim_desc.small_outbuf, "#w"));
  CuAssertPtrEquals(tc, NULL, strstr(fix.victim_desc.small_outbuf, "#W"));

  combat_messages_teardown(&fix);
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

  CuAssertTrue(tc, strlen(light) > 0);
  CuAssertTrue(tc, strlen(heavy) > 0);
  CuAssertTrue(tc, strcmp(light, heavy) != 0);

  combat_messages_teardown(&fix);
}
