#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/actions.h"
#include "../../src/bardic_performance.h"
#include "../../src/character/feats.h"
#include "../../src/character/perks.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/mud_event.h"
#include "../../src/net/protocol.h"

#include <arpa/telnet.h>
#include <string.h>

struct bardic_fixture
{
  struct room_data room;
  struct char_data bard;
  struct player_special_data player_specials;
  struct descriptor_data descriptor;
  struct room_data *saved_world;
  struct char_data *saved_character_list;
  room_rnum saved_top_of_world;
};

static void begin_bardic_fixture(struct bardic_fixture *fixture)
{
  if (skill_info[SKILL_SONG_OF_HEALING].name == NULL)
    mag_assign_spells();

  event_init();
  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_character_list = character_list;

  clear_char(&fixture->bard);
  fixture->bard.player_specials = &fixture->player_specials;
  fixture->bard.player.name = "bardic performance test character";
  fixture->bard.desc = &fixture->descriptor;
  IN_ROOM(&fixture->bard) = 0;
  GET_LEVEL(&fixture->bard) = 10;
  GET_POS(&fixture->bard) = POS_STANDING;
  GET_HIT(&fixture->bard) = 100;
  SET_FEAT(&fixture->bard, FEAT_BARDIC_MUSIC, 1);
  SET_FEAT(&fixture->bard, FEAT_SONG_OF_HEALING, 1);
  SET_FEAT(&fixture->bard, FEAT_SONG_OF_HEROISM, 1);

  fixture->descriptor.character = &fixture->bard;
  fixture->descriptor.output = fixture->descriptor.small_outbuf;
  fixture->descriptor.bufspace = SMALL_BUFSIZE - 1;
  fixture->descriptor.pProtocol = ProtocolCreate();
  STATE(&fixture->descriptor) = CON_PLAYING;

  fixture->room.number = 100;
  fixture->room.people = &fixture->bard;
  world = &fixture->room;
  top_of_world = 0;
  character_list = &fixture->bard;
}

static void end_bardic_fixture(struct bardic_fixture *fixture)
{
  clear_char_event_list(&fixture->bard);
  fixture->bard.desc = NULL;
  if (fixture->descriptor.pProtocol != NULL)
    ProtocolDestroy(fixture->descriptor.pProtocol);
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  character_list = fixture->saved_character_list;
}

static int perform_command_actions(void)
{
  int i;

  for (i = 0; *cmd_info[i].command != '\n'; i++)
  {
    if (str_cmp(cmd_info[i].command, "perform") == 0)
      return cmd_info[i].actions_required;
  }

  return -1;
}

static int count_spell_affects(struct char_data *ch, int spellnum)
{
  struct affected_type *af;
  int count;

  count = 0;
  for (af = ch->affected; af != NULL; af = af->next)
  {
    if (af->spell == spellnum)
      count++;
  }

  return count;
}

static int count_msdp_frames(const char *output, size_t length)
{
  int count;
  size_t i;

  count = 0;
  for (i = 0; i + 2 < length; i++)
  {
    if ((unsigned char)output[i] == IAC && (unsigned char)output[i + 1] == SB &&
        (unsigned char)output[i + 2] == TELOPT_MSDP)
      count++;
  }

  return count;
}

static void clear_test_affects(struct char_data *ch)
{
  while (ch->affected != NULL)
    affect_remove_no_total(ch, ch->affected);
}

static void reset_bardic_fixture_output(struct bardic_fixture *fixture)
{
  fixture->descriptor.output = fixture->descriptor.small_outbuf;
  fixture->descriptor.small_outbuf[0] = '\0';
  fixture->descriptor.bufptr = 0;
  fixture->descriptor.bufspace = SMALL_BUFSIZE - 1;
}

void Test_bardic_performance_state_uses_named_absent_sentinels(CuTest *tc)
{
  struct char_data ch;

  clear_char(&ch);

  CuAssertIntEquals(tc, FALSE, IS_PERFORMING(&ch));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_PERFORMING(&ch));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_SECONDARY_PERFORMING(&ch));
  CuAssertIntEquals(tc, 0, GET_CRESCENDO_USED(&ch));
  CuAssertIntEquals(tc, 0, GET_CRESCENDO_DICE(&ch));
}

void Test_bardic_performance_index_validation_checks_bounds_first(CuTest *tc)
{
  CuAssertTrue(tc, !is_valid_performance(PERFORMANCE_NONE));
  CuAssertTrue(tc, !is_valid_performance(MAX_PERFORMANCES));
  CuAssertTrue(tc, !is_valid_performance(MAX_PERFORMANCES + 100));
  CuAssertTrue(tc, is_valid_performance(0));
  CuAssertTrue(tc, is_valid_performance(MAX_PERFORMANCES - 1));
}

void Test_bardic_performance_command_matching_preserves_valid_state(CuTest *tc)
{
  struct bardic_fixture fixture;

  begin_bardic_fixture(&fixture);
  do_perform(&fixture.bard, "SONG OF HEALING", 0, 0);
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_SECONDARY_PERFORMING(&fixture.bard));

  do_perform(&fixture.bard, "   ", 0, 0);
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.bard));
  do_perform(&fixture.bard, "not a performance", 0, 0);
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.bard));
  do_perform(&fixture.bard, "song", 0, 0);
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.bard));

  SET_FEAT(&fixture.bard, FEAT_SONG_OF_HEROISM, 0);
  do_perform(&fixture.bard, "song of heroism", 0, 0);
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.bard));
  CuAssertTrue(tc, IS_PERFORMING(&fixture.bard));

  end_bardic_fixture(&fixture);
}

void Test_master_of_motifs_adds_distinct_secondary_performance(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_perk_data master_of_motifs;

  begin_bardic_fixture(&fixture);
  memset(&master_of_motifs, 0, sizeof(master_of_motifs));
  master_of_motifs.perk_id = PERK_BARD_MASTER_OF_MOTIFS;
  master_of_motifs.perk_class = CLASS_BARD;
  master_of_motifs.current_rank = 1;
  fixture.player_specials.saved.perks = &master_of_motifs;

  do_perform(&fixture.bard, "song of healing", 0, 0);
  clear_char_event_list(&fixture.bard);
  do_perform(&fixture.bard, "song of heroism", 0, 0);

  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, 3, GET_SECONDARY_PERFORMING(&fixture.bard));
  CuAssertTrue(tc, IS_PERFORMING(&fixture.bard));

  do_perform(&fixture.bard, "song of healing", 0, 0);
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, 3, GET_SECONDARY_PERFORMING(&fixture.bard));

  end_bardic_fixture(&fixture);
}

void Test_bardic_performance_slot_failure_keeps_other_song_active(CuTest *tc)
{
  struct char_data ch;

  clear_char(&ch);
  IS_PERFORMING(&ch) = TRUE;
  GET_PERFORMING(&ch) = 0;
  GET_SECONDARY_PERFORMING(&ch) = 3;

  stop_bardic_performance_slot(&ch, PERFORMANCE_VAR_SECONDARY, FALSE);
  CuAssertTrue(tc, IS_PERFORMING(&ch));
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&ch));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_SECONDARY_PERFORMING(&ch));

  GET_SECONDARY_PERFORMING(&ch) = 3;
  stop_bardic_performance_slot(&ch, PERFORMANCE_VAR_PRIMARY, FALSE);
  CuAssertTrue(tc, IS_PERFORMING(&ch));
  CuAssertIntEquals(tc, 3, GET_PERFORMING(&ch));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_SECONDARY_PERFORMING(&ch));
}

void Test_efficient_performance_selects_action_after_command_preflight(CuTest *tc)
{
  struct bardic_fixture fixture;

  begin_bardic_fixture(&fixture);
  CuAssertIntEquals(tc, ACTION_NONE, perform_command_actions());

  start_action_cooldown(&fixture.bard, atSTANDARD, 6 RL_SEC);
  do_perform(&fixture.bard, "song of healing", 0, 0);
  CuAssertTrue(tc, !IS_PERFORMING(&fixture.bard));

  clear_char_event_list(&fixture.bard);
  SET_FEAT(&fixture.bard, FEAT_EFFICIENT_PERFORMANCE, 1);
  start_action_cooldown(&fixture.bard, atSTANDARD, 6 RL_SEC);
  do_perform(&fixture.bard, "song of healing", 0, 0);
  CuAssertTrue(tc, IS_PERFORMING(&fixture.bard));
  CuAssertTrue(tc, char_has_mud_event(&fixture.bard, eMOVEACTION) != NULL);

  do_perform(&fixture.bard, "list", 0, 0);
  CuAssertTrue(tc, IS_PERFORMING(&fixture.bard));
  do_perform(&fixture.bard, NULL, 0, 0);
  CuAssertTrue(tc, !IS_PERFORMING(&fixture.bard));

  clear_char_event_list(&fixture.bard);
  start_action_cooldown(&fixture.bard, atMOVE, 6 RL_SEC);
  do_perform(&fixture.bard, "song of healing", 0, 0);
  CuAssertTrue(tc, IS_PERFORMING(&fixture.bard));
  CuAssertTrue(tc, char_has_mud_event(&fixture.bard, eSTANDARDACTION) != NULL);
  do_perform(&fixture.bard, NULL, 0, 0);

  end_bardic_fixture(&fixture);
}

void Test_linkless_bard_does_not_retain_room_blocking_state(CuTest *tc)
{
  struct bardic_fixture fixture;

  begin_bardic_fixture(&fixture);
  fixture.bard.desc = NULL;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;

  pulse_bardic_performance();

  CuAssertTrue(tc, !IS_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_SECONDARY_PERFORMING(&fixture.bard));

  end_bardic_fixture(&fixture);
}

void Test_legacy_npc_perform_cooldown_is_not_an_active_song(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data npc;

  begin_bardic_fixture(&fixture);
  clear_char(&npc);
  SET_BIT_AR(MOB_FLAGS(&npc), MOB_ISNPC);
  npc.player_specials = &dummy_mob;
  npc.player.short_descr = "a legacy performing test NPC";
  IN_ROOM(&npc) = 0;
  GET_POS(&npc) = POS_STANDING;
  fixture.bard.next_in_room = &npc;
  attach_mud_event(new_mud_event(ePERFORM, &npc, NULL), 60 RL_SEC);

  CuAssertTrue(tc, can_perform(&fixture.bard, 0, FALSE, TRUE));

  clear_char_event_list(&npc);
  fixture.bard.next_in_room = NULL;
  end_bardic_fixture(&fixture);
}

void Test_bardic_affect_batch_emits_only_final_msdp_state(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct affected_type hitroll;
  struct affected_type damroll;

  begin_bardic_fixture(&fixture);
  fixture.descriptor.pProtocol->bMSDP = bool_t_true;
  fixture.descriptor.pProtocol->pVariables[eMSDP_AFFECTS]->bReport = bool_t_true;

  new_affect(&hitroll);
  hitroll.spell = SKILL_SONG_OF_HEROISM;
  hitroll.duration = 3;
  hitroll.modifier = 2;
  hitroll.location = APPLY_HITROLL;
  hitroll.bonus_type = BONUS_TYPE_INHERENT;
  new_affect(&damroll);
  damroll.spell = SKILL_SONG_OF_HEROISM;
  damroll.duration = 3;
  damroll.modifier = 2;
  damroll.location = APPLY_DAMROLL;
  damroll.bonus_type = BONUS_TYPE_INHERENT;

  affect_batch_begin(&fixture.bard);
  affect_to_char(&fixture.bard, &hitroll);
  affect_to_char(&fixture.bard, &damroll);
  CuAssertIntEquals(tc, 0, fixture.descriptor.bufptr);
  affect_batch_end(&fixture.bard);

  CuAssertIntEquals(tc, 1, count_msdp_frames(fixture.descriptor.output, fixture.descriptor.bufptr));
  CuAssertTrue(tc, !fixture.descriptor.pProtocol->pVariables[eMSDP_AFFECTS]->bDirty);

  reset_bardic_fixture_output(&fixture);
  affect_batch_begin(&fixture.bard);
  affect_from_char(&fixture.bard, SKILL_SONG_OF_HEROISM);
  affect_to_char(&fixture.bard, &hitroll);
  affect_to_char(&fixture.bard, &damroll);
  affect_batch_end(&fixture.bard);

  CuAssertIntEquals(tc, 0, fixture.descriptor.bufptr);
  clear_test_affects(&fixture.bard);
  end_bardic_fixture(&fixture);
}

void Test_bardic_performance_applies_only_meaningful_affect_slots(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data target;

  begin_bardic_fixture(&fixture);
  clear_char(&target);
  SET_BIT_AR(MOB_FLAGS(&target), MOB_ISNPC);
  target.player_specials = &dummy_mob;
  target.player.short_descr = "a bardic affect target";
  IN_ROOM(&target) = 0;
  GET_LEVEL(&target) = 10;
  GET_POS(&target) = POS_STANDING;
  GET_HIT(&target) = 100;
  GET_MAX_HIT(&target) = 100;
  fixture.bard.next_in_room = &target;

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_HEALING, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 0, count_spell_affects(&target, SKILL_SONG_OF_HEALING));

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_DANCE_OF_PROTECTION, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 3, count_spell_affects(&target, SKILL_DANCE_OF_PROTECTION));

  clear_test_affects(&target);
  affect_total(&target);
  CuAssertTrue(
      tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_FLIGHT, 20, PERFORM_AOE_GROUP));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_FLYING));
  CuAssertIntEquals(tc, 1, count_spell_affects(&target, SKILL_SONG_OF_FLIGHT));
  CuAssertTrue(
      tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_FLIGHT, 20, PERFORM_AOE_GROUP));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_FLYING));
  CuAssertIntEquals(tc, 1, count_spell_affects(&target, SKILL_SONG_OF_FLIGHT));

  clear_test_affects(&target);
  fixture.bard.next_in_room = NULL;
  end_bardic_fixture(&fixture);
}

void Test_msdp_affect_serializer_rejects_invalid_and_oversized_state(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct affected_type invalid;
  struct affected_type *many;
  int i;
  const int many_count = 256;

  begin_bardic_fixture(&fixture);
  fixture.descriptor.pProtocol->bMSDP = bool_t_true;
  fixture.descriptor.pProtocol->pVariables[eMSDP_AFFECTS]->bReport = bool_t_true;
  CuAssertIntEquals(tc, PROTOCOL_SUCCESS,
                    MSDPSetString(&fixture.descriptor, eMSDP_AFFECTS, "sentinel"));
  fixture.descriptor.pProtocol->pVariables[eMSDP_AFFECTS]->bDirty = false;

  new_affect(&invalid);
  invalid.spell = SKILL_SONG_OF_HEROISM;
  invalid.location = NUM_APPLIES;
  fixture.bard.affected = &invalid;
  update_msdp_affects(&fixture.bard);
  CuAssertStrEquals(tc, "sentinel",
                    fixture.descriptor.pProtocol->pVariables[eMSDP_AFFECTS]->pValueString);
  CuAssertIntEquals(tc, 0, fixture.descriptor.bufptr);

  many = calloc((size_t)many_count, sizeof(*many));
  CuAssertPtrNotNull(tc, many);
  if (many != NULL)
  {
    for (i = 0; i < many_count; i++)
    {
      new_affect(&many[i]);
      many[i].spell = SKILL_SONG_OF_HEROISM;
      many[i].location = APPLY_HITROLL;
      many[i].bonus_type = BONUS_TYPE_INHERENT;
      many[i].duration = i;
      many[i].next = i + 1 < many_count ? &many[i + 1] : NULL;
    }
    fixture.bard.affected = many;
    update_msdp_affects(&fixture.bard);
    CuAssertStrEquals(tc, "sentinel",
                      fixture.descriptor.pProtocol->pVariables[eMSDP_AFFECTS]->pValueString);
    CuAssertIntEquals(tc, 0, fixture.descriptor.bufptr);
    free(many);
  }

  fixture.bard.affected = NULL;
  end_bardic_fixture(&fixture);
}
