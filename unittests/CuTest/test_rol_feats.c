/* Tests for the feats converted from Realms of Luminari player skills:
 * shadow, calm, establish camp, garrote and accompany. */

#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/actionqueues.h"
#include "../../src/actions.h"
#include "../../src/act.h"
#include "../../src/bardic_performance.h"
#include "../../src/character/abilities.h"
#include "../../src/character/feats.h"
#include "../../src/combat/fight.h"
#include "../../src/constants.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/mud_event.h"
#include "../../src/net/protocol.h"
#include "../../src/rol_feats.h"

#include <string.h>

struct rol_feat_fixture
{
  struct room_data room;
  struct char_data lead;
  struct char_data second;
  struct player_special_data lead_specials;
  struct player_special_data second_specials;
  struct descriptor_data lead_descriptor;
  struct descriptor_data second_descriptor;
  struct index_data mob_index_entry;
  struct room_data *saved_world;
  struct char_data *saved_character_list;
  struct index_data *saved_mob_index;
  room_rnum saved_top_of_world;
  mob_rnum saved_top_of_mobt;
};

static void setup_test_char(struct char_data *ch, struct player_special_data *specials,
                            struct descriptor_data *descriptor, const char *name)
{
  clear_char(ch);
  GET_ATTACK_QUEUE(ch) = create_attack_queue();
  ch->player_specials = specials;
  ch->player.name = (char *)name;
  ch->desc = descriptor;
  IN_ROOM(ch) = 0;
  GET_LEVEL(ch) = 10;
  GET_REAL_SIZE(ch) = SIZE_MEDIUM;
  ch->points.size = SIZE_MEDIUM;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100;

  memset(descriptor, 0, sizeof(*descriptor));
  descriptor->character = ch;
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
  descriptor->pProtocol = ProtocolCreate();
  STATE(descriptor) = CON_PLAYING;
}

static void begin_rol_feat_fixture(struct rol_feat_fixture *fixture)
{
  if (skill_info[SKILL_SONG_OF_HEALING].name == NULL)
    mag_assign_spells();

  event_init();
  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_character_list = character_list;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;

  setup_test_char(&fixture->lead, &fixture->lead_specials, &fixture->lead_descriptor,
                  "rol feat lead");
  setup_test_char(&fixture->second, &fixture->second_specials, &fixture->second_descriptor,
                  "rol feat second");

  fixture->lead.next_in_room = &fixture->second;
  fixture->lead.next = &fixture->second;

  fixture->room.number = 100;
  fixture->room.people = &fixture->lead;
  world = &fixture->room;
  top_of_world = 0;
  character_list = &fixture->lead;
  fixture->mob_index_entry.vnum = DG_CASTER_PROXY;
  mob_index = &fixture->mob_index_entry;
  top_of_mobt = 0;
}

static void end_test_char(struct char_data *ch, struct descriptor_data *descriptor)
{
  while (ch->affected != NULL)
    affect_remove_no_total(ch, ch->affected);
  clear_char_event_list(ch);
  free_attack_queue(GET_ATTACK_QUEUE(ch));
  GET_ATTACK_QUEUE(ch) = NULL;
  ch->desc = NULL;
  if (descriptor->pProtocol != NULL)
    ProtocolDestroy(descriptor->pProtocol);
}

static void end_rol_feat_fixture(struct rol_feat_fixture *fixture)
{
  end_test_char(&fixture->lead, &fixture->lead_descriptor);
  end_test_char(&fixture->second, &fixture->second_descriptor);
  event_free_all();
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  character_list = fixture->saved_character_list;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
}

static void give_camp_affect(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SKILL_CAMP;
  af.duration = 10;
  affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
}

/* A camp only speeds recovery for someone actually settled into it. */
void TestCampRecoveryRequiresRestingInCamp(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  GET_POS(&fixture.lead) = POS_RESTING;
  CuAssertIntEquals(tc, 0, camp_recovery_bonus(&fixture.lead, 40));

  give_camp_affect(&fixture.lead);
  CuAssertIntEquals(tc, 20, camp_recovery_bonus(&fixture.lead, 40));

  GET_POS(&fixture.lead) = POS_SLEEPING;
  CuAssertIntEquals(tc, 20, camp_recovery_bonus(&fixture.lead, 40));

  GET_POS(&fixture.lead) = POS_STANDING;
  CuAssertIntEquals(tc, 0, camp_recovery_bonus(&fixture.lead, 40));

  end_rol_feat_fixture(&fixture);
}

/* Only an able accompanist with the feat lifts a lead performance. */
void TestAccompanimentBonusRequiresAnAbleAccompanist(CuTest *tc)
{
  struct rol_feat_fixture fixture;
  int bonus;

  begin_rol_feat_fixture(&fixture);

  CuAssertIntEquals(tc, 0, accompaniment_bonus(&fixture.lead));

  ACCOMPANYING(&fixture.second) = &fixture.lead;
  CuAssertIntEquals(tc, 0, accompaniment_bonus(&fixture.lead));

  SET_FEAT(&fixture.second, FEAT_ACCOMPANY, 1);
  bonus = accompaniment_bonus(&fixture.lead);
  CuAssertTrue(tc, bonus >= 1);
  CuAssertTrue(tc, bonus <= 12);

  /* an accompanist leading a song of their own is not backing anyone */
  IS_PERFORMING(&fixture.second) = TRUE;
  CuAssertIntEquals(tc, 0, accompaniment_bonus(&fixture.lead));
  IS_PERFORMING(&fixture.second) = FALSE;

  /* a silenced accompanist contributes nothing */
  SET_BIT_AR(AFF_FLAGS(&fixture.second), AFF_SILENCED);
  CuAssertIntEquals(tc, 0, accompaniment_bonus(&fixture.lead));

  end_rol_feat_fixture(&fixture);
}

/* A faltering lead hands the song to an accompanist instead of losing it. */
void TestAccompanyTakeoverPassesTheSongOn(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SET_FEAT(&fixture.second, FEAT_ACCOMPANY, 1);
  SET_FEAT(&fixture.second, FEAT_BARDIC_MUSIC, 1);
  SET_FEAT(&fixture.second, FEAT_SONG_OF_HEALING, 1);
  ACCOMPANYING(&fixture.second) = &fixture.lead;

  CuAssertTrue(tc, accompany_takeover(&fixture.lead, 0));
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.second));
  CuAssertTrue(tc, IS_PERFORMING(&fixture.second));
  CuAssertPtrEquals(tc, NULL, ACCOMPANYING(&fixture.second));

  end_rol_feat_fixture(&fixture);
}

/* Without the song's own feat there is nobody to hand the song to. */
void TestAccompanyTakeoverNeedsTheSongFeat(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SET_FEAT(&fixture.second, FEAT_ACCOMPANY, 1);
  SET_FEAT(&fixture.second, FEAT_BARDIC_MUSIC, 1);
  ACCOMPANYING(&fixture.second) = &fixture.lead;

  CuAssertTrue(tc, !accompany_takeover(&fixture.lead, 0));
  CuAssertTrue(tc, !IS_PERFORMING(&fixture.second));
  CuAssertPtrEquals(tc, &fixture.lead, ACCOMPANYING(&fixture.second));

  end_rol_feat_fixture(&fixture);
}

/* A tail cannot be kept up by someone who has stopped moving silently. */
void TestShadowTailBreaksWithoutSneak(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SHADOWING(&fixture.second) = &fixture.lead;
  shadowers_follow(&fixture.lead, 0, NORTH);

  CuAssertPtrEquals(tc, NULL, SHADOWING(&fixture.second));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.second));

  end_rol_feat_fixture(&fixture);
}

/* Nor by someone who has been drawn into a fight. */
void TestShadowTailBreaksWhenFighting(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SET_BIT_AR(AFF_FLAGS(&fixture.second), AFF_SNEAK);
  FIGHTING(&fixture.second) = &fixture.lead;
  SHADOWING(&fixture.second) = &fixture.lead;

  shadowers_follow(&fixture.lead, 0, NORTH);

  CuAssertPtrEquals(tc, NULL, SHADOWING(&fixture.second));

  FIGHTING(&fixture.second) = NULL;
  end_rol_feat_fixture(&fixture);
}

/* Tails and accompaniments are dropped in both directions when a link dies. */
void TestShadowAndAccompanyLinksAreClearedBothWays(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SHADOWING(&fixture.second) = &fixture.lead;
  SHADOWING(&fixture.lead) = &fixture.second;
  clear_shadow_links(&fixture.lead);
  CuAssertPtrEquals(tc, NULL, SHADOWING(&fixture.lead));
  CuAssertPtrEquals(tc, NULL, SHADOWING(&fixture.second));

  ACCOMPANYING(&fixture.second) = &fixture.lead;
  ACCOMPANYING(&fixture.lead) = &fixture.second;
  clear_accompany_links(&fixture.lead);
  CuAssertPtrEquals(tc, NULL, ACCOMPANYING(&fixture.lead));
  CuAssertPtrEquals(tc, NULL, ACCOMPANYING(&fixture.second));

  end_rol_feat_fixture(&fixture);
}

/* Every converted skill is reachable as a command and as a feat. */
void TestConvertedRolSkillsAreRegistered(CuTest *tc)
{
  static const char *commands[] = {"shadow", "calm", "camp", "garrote", "accompany"};
  static const char *feats[] = {"shadow", "calm", "establish camp", "garrote", "accompany"};
  size_t i;
  int j;
  bool found;

  for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
  {
    found = FALSE;
    for (j = 0; *cmd_info[j].command != '\n'; j++)
    {
      if (str_cmp(cmd_info[j].command, commands[i]) == 0)
      {
        found = TRUE;
        break;
      }
    }
    CuAssertTrue(tc, found);
  }

  for (i = 0; i < sizeof(feats) / sizeof(feats[0]); i++)
    CuAssertTrue(tc, find_feat_num(feats[i]) > 0);
}
