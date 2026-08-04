#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/actionqueues.h"
#include "../../src/actions.h"
#include "../../src/bardic_performance.h"
#include "../../src/character/feats.h"
#include "../../src/character/perks.h"
#include "../../src/combat/fight.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/lists.h"
#include "../../src/magic/spell_prep.h"
#include "../../src/magic/spells.h"
#include "../../src/mud_event.h"
#include "../../src/net/protocol.h"
#include "../../src/spec_procs.h"

#include <arpa/telnet.h>
#include <string.h>

struct bardic_fixture
{
  struct room_data room;
  struct char_data bard;
  struct player_special_data player_specials;
  struct descriptor_data descriptor;
  struct index_data mob_index_entry;
  struct room_data *saved_world;
  struct char_data *saved_character_list;
  struct index_data *saved_mob_index;
  room_rnum saved_top_of_world;
  mob_rnum saved_top_of_mobt;
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
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;

  clear_char(&fixture->bard);
  GET_ATTACK_QUEUE(&fixture->bard) = create_attack_queue();
  fixture->bard.player_specials = &fixture->player_specials;
  fixture->bard.player.name = "bardic performance test character";
  fixture->bard.desc = &fixture->descriptor;
  IN_ROOM(&fixture->bard) = 0;
  GET_LEVEL(&fixture->bard) = 10;
  fixture->player_specials.saved.class_level[CLASS_BARD] = 10;
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
  fixture->mob_index_entry.vnum = DG_CASTER_PROXY;
  mob_index = &fixture->mob_index_entry;
  top_of_mobt = 0;
}

static void end_bardic_fixture(struct bardic_fixture *fixture)
{
  while (fixture->bard.affected != NULL)
    affect_remove_no_total(&fixture->bard, fixture->bard.affected);
  clear_char_event_list(&fixture->bard);
  free_attack_queue(GET_ATTACK_QUEUE(&fixture->bard));
  GET_ATTACK_QUEUE(&fixture->bard) = NULL;
  fixture->bard.desc = NULL;
  if (fixture->descriptor.pProtocol != NULL)
    ProtocolDestroy(fixture->descriptor.pProtocol);
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  character_list = fixture->saved_character_list;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
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

static int count_spell_affects_from_source(struct char_data *ch, int spellnum, long source_id)
{
  struct affected_type *af;
  int count;

  count = 0;
  for (af = ch->affected; af != NULL; af = af->next)
  {
    if (af->spell == spellnum && af->source_id == source_id)
      count++;
  }

  return count;
}

static void initialize_test_descriptor(struct descriptor_data *descriptor, struct char_data *ch)
{
  memset(descriptor, 0, sizeof(*descriptor));
  descriptor->character = ch;
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
  descriptor->pProtocol = ProtocolCreate();
  STATE(descriptor) = CON_PLAYING;
  ch->desc = descriptor;
}

static void destroy_test_descriptor(struct descriptor_data *descriptor, struct char_data *ch)
{
  ch->desc = NULL;
  if (descriptor->pProtocol != NULL)
    ProtocolDestroy(descriptor->pProtocol);
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

static void initialize_bardic_test_pc(struct char_data *ch, struct player_special_data *specials,
                                      const char *name)
{
  memset(specials, 0, sizeof(*specials));
  clear_char(ch);
  ch->player_specials = specials;
  ch->player.name = (char *)name;
  IN_ROOM(ch) = 0;
  GET_LEVEL(ch) = 10;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100;
  GET_MAX_HIT(ch) = 100;
}

static void initialize_bardic_test_npc(struct char_data *ch, const char *name)
{
  clear_char(ch);
  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &dummy_mob;
  ch->player.short_descr = (char *)name;
  IN_ROOM(ch) = 0;
  GET_LEVEL(ch) = 10;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100;
  GET_MAX_HIT(ch) = 100;
}

static void initialize_bardic_test_perk(struct char_perk_data *perk, int perk_id, int rank,
                                        struct char_perk_data *next)
{
  memset(perk, 0, sizeof(*perk));
  perk->perk_id = perk_id;
  perk->perk_class = CLASS_BARD;
  perk->current_rank = rank;
  perk->next = next;
}

static struct affected_type *find_spell_affect_location(struct char_data *ch, int spellnum,
                                                        int location)
{
  struct affected_type *af;

  for (af = ch->affected; af != NULL; af = af->next)
  {
    if (af->spell == spellnum && af->location == location)
      return af;
  }

  return NULL;
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

void Test_bardic_performance_engine_failure_is_scoped_to_requested_slot(CuTest *tc)
{
  struct bardic_fixture fixture;

  begin_bardic_fixture(&fixture);
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;
  GET_SECONDARY_PERFORMING(&fixture.bard) = 3;
  GET_POS(&fixture.bard) = POS_RESTING;

  CuAssertIntEquals(tc, 0,
                    process_bardic_performance_slot(&fixture.bard, PERFORMANCE_VAR_SECONDARY));
  CuAssertTrue(tc, IS_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_SECONDARY_PERFORMING(&fixture.bard));

  GET_SECONDARY_PERFORMING(&fixture.bard) = 3;
  CuAssertIntEquals(tc, 0, process_bardic_performance_slot(&fixture.bard, PERFORMANCE_VAR_PRIMARY));
  CuAssertTrue(tc, IS_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, 3, GET_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_SECONDARY_PERFORMING(&fixture.bard));

  end_bardic_fixture(&fixture);
}

void Test_disconnect_cleanup_stops_switched_bardic_performances(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data original;

  begin_bardic_fixture(&fixture);
  clear_char(&original);
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;
  IS_PERFORMING(&original) = TRUE;
  GET_PERFORMING(&original) = 3;
  fixture.descriptor.original = &original;

  stop_descriptor_bardic_performances(&fixture.descriptor);

  CuAssertTrue(tc, !IS_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_PERFORMING(&fixture.bard));
  CuAssertTrue(tc, !IS_PERFORMING(&original));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_PERFORMING(&original));
  fixture.descriptor.original = NULL;
  end_bardic_fixture(&fixture);
}

void Test_harmonic_casting_controls_spell_interruption_without_round_resource(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_perk_data harmonic_casting;

  begin_bardic_fixture(&fixture);
  fixture.player_specials.casting_class = CLASS_BARD;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;

  handle_bardic_spell_performance(&fixture.bard);
  CuAssertTrue(tc, !IS_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, PERFORMANCE_NONE, GET_PERFORMING(&fixture.bard));

  memset(&harmonic_casting, 0, sizeof(harmonic_casting));
  harmonic_casting.perk_id = PERK_BARD_HARMONIC_CASTING;
  harmonic_casting.perk_class = CLASS_BARD;
  harmonic_casting.current_rank = 1;
  fixture.player_specials.saved.perks = &harmonic_casting;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;
  GET_SECONDARY_PERFORMING(&fixture.bard) = 3;

  handle_bardic_spell_performance(&fixture.bard);
  CuAssertTrue(tc, IS_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, 3, GET_SECONDARY_PERFORMING(&fixture.bard));

  end_bardic_fixture(&fixture);
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

void Test_active_npc_bard_receives_performance_pulses(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data npc;

  begin_bardic_fixture(&fixture);
  clear_char(&npc);
  SET_BIT_AR(MOB_FLAGS(&npc), MOB_ISNPC);
  npc.player_specials = &dummy_mob;
  npc.player.short_descr = "an active performing test NPC";
  IN_ROOM(&npc) = 0;
  GET_LEVEL(&npc) = 10;
  GET_POS(&npc) = POS_STANDING;
  GET_HIT(&npc) = 1;
  GET_MAX_HIT(&npc) = 100;
  IS_PERFORMING(&npc) = TRUE;
  GET_PERFORMING(&npc) = 0;
  fixture.room.people = &npc;
  character_list = &npc;

  pulse_bardic_performance();

  CuAssertTrue(tc, GET_HIT(&npc) > 1);
  clear_char_event_list(&npc);
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

void Test_bardic_base_performance_matrix_matches_documented_mechanics(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data target;

  begin_bardic_fixture(&fixture);
  clear_char(&target);
  SET_BIT_AR(MOB_FLAGS(&target), MOB_ISNPC);
  target.player_specials = &dummy_mob;
  target.player.short_descr = "a bardic matrix target";
  IN_ROOM(&target) = 0;
  GET_LEVEL(&target) = 10;
  GET_POS(&target) = POS_STANDING;
  GET_HIT(&target) = 1;
  GET_MAX_HIT(&target) = 100;
  GET_MOVE(&target) = 1;
  GET_MAX_MOVE(&target) = 100;
  fixture.bard.next_in_room = &target;

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_HEALING, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertTrue(tc, GET_HIT(&target) > 1);
  CuAssertIntEquals(tc, 0, count_spell_affects(&target, SKILL_SONG_OF_HEALING));

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_DANCE_OF_PROTECTION, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 3, count_spell_affects(&target, SKILL_DANCE_OF_PROTECTION));
  clear_test_affects(&target);
  affect_total(&target);

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_FOCUSED_MIND, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 3, count_spell_affects(&target, SKILL_SONG_OF_FOCUSED_MIND));
  clear_test_affects(&target);
  affect_total(&target);

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_HEROISM, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 5, count_spell_affects(&target, SKILL_SONG_OF_HEROISM));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_HASTE));
  clear_test_affects(&target);
  affect_total(&target);

  GET_HIT(&target) = 1;
  GET_MAX_HIT(&target) = 100;
  GET_MOVE(&target) = 1;
  GET_MAX_MOVE(&target) = 100;
  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_ORATORY_OF_REJUVENATION, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertTrue(tc, GET_HIT(&target) > 1);
  CuAssertTrue(tc, GET_MOVE(&target) > 1);
  CuAssertIntEquals(tc, 0, count_spell_affects(&target, SKILL_ORATORY_OF_REJUVENATION));

  GET_MOVE(&target) = 1;
  GET_MAX_MOVE(&target) = 100;
  CuAssertTrue(
      tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_FLIGHT, 20, PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 1, count_spell_affects(&target, SKILL_SONG_OF_FLIGHT));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_FLYING));
  CuAssertTrue(tc, GET_MOVE(&target) > 1);
  clear_test_affects(&target);
  affect_total(&target);

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_REVELATION, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 3, count_spell_affects(&target, SKILL_SONG_OF_REVELATION));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_DETECT_INVIS));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_DETECT_ALIGN));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_DETECT_MAGIC));
  clear_test_affects(&target);
  affect_total(&target);

  GET_POS(&target) = POS_DEAD;
  CuAssertTrue(
      tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_FEAR, 20, PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 1, count_spell_affects(&target, SKILL_SONG_OF_FEAR));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_FEAR));
  clear_test_affects(&target);
  affect_total(&target);

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_ACT_OF_FORGETFULNESS, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 0, count_spell_affects(&target, SKILL_ACT_OF_FORGETFULNESS));

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_ROOTING, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 2, count_spell_affects(&target, SKILL_SONG_OF_ROOTING));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_ENTANGLED));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_SLOW));
  clear_test_affects(&target);
  affect_total(&target);

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_DRAGONS, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, BARD_AFFECTS, count_spell_affects(&target, SKILL_SONG_OF_DRAGONS));
  clear_test_affects(&target);
  affect_total(&target);

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_THE_MAGI, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 5, count_spell_affects(&target, SKILL_SONG_OF_THE_MAGI));
  clear_test_affects(&target);
  affect_total(&target);

  CuAssertTrue(
      tc, performance_effects(&fixture.bard, &target, SKILL_DEAFENING_SONG, 20, PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 1, count_spell_affects(&target, SKILL_DEAFENING_SONG));
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_DEAF));
  CuAssertTrue(
      tc, performance_effects(&fixture.bard, &target, SKILL_DEAFENING_SONG, 20, PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 1, count_spell_affects(&target, SKILL_DEAFENING_SONG));

  clear_test_affects(&target);
  affect_total(&target);
  fixture.bard.next_in_room = NULL;
  clear_char_event_list(&target);
  end_bardic_fixture(&fixture);
}

void Test_bardic_offensive_performances_use_their_documented_saves(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data target;

  begin_bardic_fixture(&fixture);
  clear_char(&target);
  SET_BIT_AR(MOB_FLAGS(&target), MOB_ISNPC);
  target.player_specials = &dummy_mob;
  target.player.short_descr = "a bardic saving throw target";
  IN_ROOM(&target) = 0;
  GET_LEVEL(&target) = 10;
  GET_POS(&target) = POS_STANDING;
  GET_SAVE(&target, SAVING_FORT) = -100;
  GET_SAVE(&target, SAVING_REFL) = -100;
  GET_SAVE(&target, SAVING_WILL) = 100;

  circle_srandom(1);
  CuAssertTrue(
      tc, !performance_effects(&fixture.bard, &target, SKILL_SONG_OF_FEAR, 20, PERFORM_AOE_GROUP));
  circle_srandom(1);
  CuAssertTrue(tc, !performance_effects(&fixture.bard, &target, SKILL_ACT_OF_FORGETFULNESS, 20,
                                        PERFORM_AOE_GROUP));

  GET_SAVE(&target, SAVING_WILL) = -100;
  GET_SAVE(&target, SAVING_REFL) = 100;
  circle_srandom(1);
  CuAssertTrue(tc, !performance_effects(&fixture.bard, &target, SKILL_SONG_OF_ROOTING, 20,
                                        PERFORM_AOE_GROUP));

  GET_SAVE(&target, SAVING_REFL) = -100;
  GET_SAVE(&target, SAVING_WILL) = 100;
  circle_srandom(1);
  CuAssertTrue(tc, !performance_effects(&fixture.bard, &target, SKILL_SONG_OF_THE_MAGI, 20,
                                        PERFORM_AOE_GROUP));

  GET_SAVE(&target, SAVING_WILL) = -100;
  GET_SAVE(&target, SAVING_FORT) = 100;
  circle_srandom(1);
  CuAssertTrue(tc, !performance_effects(&fixture.bard, &target, SKILL_DEAFENING_SONG, 20,
                                        PERFORM_AOE_GROUP));
  CuAssertPtrEquals(tc, NULL, target.affected);

  circle_srandom((unsigned long)time(NULL));
  clear_char_event_list(&target);
  end_bardic_fixture(&fixture);
}

void Test_songweaver_initializes_every_applied_affect_with_extended_duration(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_perk_data songweaver_i;
  struct char_perk_data songweaver_ii;
  struct affected_type *af;
  bool saw_will_bonus;

  begin_bardic_fixture(&fixture);
  memset(&songweaver_i, 0, sizeof(songweaver_i));
  memset(&songweaver_ii, 0, sizeof(songweaver_ii));
  songweaver_i.perk_id = PERK_BARD_SONGWEAVER_I;
  songweaver_i.perk_class = CLASS_BARD;
  songweaver_i.current_rank = 3;
  songweaver_i.next = &songweaver_ii;
  songweaver_ii.perk_id = PERK_BARD_SONGWEAVER_II;
  songweaver_ii.perk_class = CLASS_BARD;
  songweaver_ii.current_rank = 2;
  fixture.player_specials.saved.perks = &songweaver_i;
  saw_will_bonus = FALSE;

  CuAssertTrue(tc, performance_effects(&fixture.bard, &fixture.bard, SKILL_DANCE_OF_PROTECTION, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 3, count_spell_affects(&fixture.bard, SKILL_DANCE_OF_PROTECTION));
  for (af = fixture.bard.affected; af != NULL; af = af->next)
  {
    if (af->spell == SKILL_DANCE_OF_PROTECTION)
    {
      CuAssertIntEquals(tc, 7, af->duration);
      if (af->location == APPLY_SAVING_WILL)
      {
        CuAssertIntEquals(tc, 4, af->modifier);
        saw_will_bonus = TRUE;
      }
    }
  }
  CuAssertTrue(tc, saw_will_bonus);

  clear_test_affects(&fixture.bard);
  end_bardic_fixture(&fixture);
}

void Test_starting_a_performance_executes_an_immediate_first_verse(CuTest *tc)
{
  struct bardic_fixture fixture;

  begin_bardic_fixture(&fixture);
  GET_HIT(&fixture.bard) = 1;
  GET_MAX_HIT(&fixture.bard) = 100;

  do_perform(&fixture.bard, "song of healing", 0, 0);

  CuAssertTrue(tc, IS_PERFORMING(&fixture.bard));
  CuAssertTrue(tc, GET_HIT(&fixture.bard) > 1);
  CuAssertTrue(tc, fixture.bard.char_specials.performance_source_id > 0);

  end_bardic_fixture(&fixture);
}

void Test_healing_reports_only_hit_points_actually_restored(CuTest *tc)
{
  struct bardic_fixture fixture;

  begin_bardic_fixture(&fixture);
  GET_HIT(&fixture.bard) = 90;
  GET_MAX_HIT(&fixture.bard) = 100;
  SET_FEAT(&fixture.bard, FEAT_EMPOWERED_HEALING, 1);
  SET_BIT_AR(PRF_FLAGS(&fixture.bard), PRF_COMBATROLL);

  CuAssertTrue(tc, process_healing(&fixture.bard, &fixture.bard, SKILL_SONG_OF_HEALING, 100, 0, 0));
  CuAssertIntEquals(tc, 100, GET_HIT(&fixture.bard));
  CuAssertTrue(tc, strstr(fixture.descriptor.output, "<10>") != NULL);
  CuAssertTrue(tc, strstr(fixture.descriptor.output, "<150>") == NULL);

  reset_bardic_fixture_output(&fixture);
  SET_FEAT(&fixture.bard, FEAT_EMPOWERED_HEALING, 0);
  SET_BIT_AR(AFF_FLAGS(&fixture.bard), AFF_BLACKMANTLE);
  GET_HIT(&fixture.bard) = 90;

  CuAssertTrue(tc, performance_effects(&fixture.bard, &fixture.bard, SKILL_SONG_OF_HEALING, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 100, GET_HIT(&fixture.bard));
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.bard), AFF_BLACKMANTLE);

  end_bardic_fixture(&fixture);
}

void Test_bardic_affect_timing_uses_one_refresh_clock(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data target;
  struct affected_type *af;

  begin_bardic_fixture(&fixture);
  clear_char(&target);
  SET_BIT_AR(MOB_FLAGS(&target), MOB_ISNPC);
  target.player_specials = &dummy_mob;
  target.player.short_descr = "a bardic timing target";
  IN_ROOM(&target) = 0;
  GET_POS(&target) = POS_STANDING;
  fixture.bard.next_in_room = &target;

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_DANCE_OF_PROTECTION, 20,
                                       PERFORM_AOE_GROUP));
  for (af = target.affected; af != NULL; af = af->next)
    if (af->spell == SKILL_DANCE_OF_PROTECTION)
      CuAssertIntEquals(tc, BARDIC_BASE_AFFECT_ROUNDS, af->duration);

  clear_test_affects(&target);
  affect_total(&target);
  SET_FEAT(&fixture.bard, FEAT_LINGERING_SONG, 1);
  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_DANCE_OF_PROTECTION, 20,
                                       PERFORM_AOE_GROUP));
  for (af = target.affected; af != NULL; af = af->next)
    if (af->spell == SKILL_DANCE_OF_PROTECTION)
      CuAssertIntEquals(tc, BARDIC_BASE_AFFECT_ROUNDS + BARDIC_LINGERING_AFFECT_ROUNDS,
                        af->duration);

  clear_test_affects(&target);
  affect_total(&target);
  SET_FEAT(&fixture.bard, FEAT_LINGERING_SONG, 0);
  CuAssertTrue(
      tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_FLIGHT, 20, PERFORM_AOE_GROUP));
  CuAssertPtrNotNull(tc, target.affected);
  if (target.affected != NULL)
    CuAssertIntEquals(tc, BARDIC_BASE_AFFECT_ROUNDS, target.affected->duration);

  clear_test_affects(&target);
  fixture.bard.next_in_room = NULL;
  end_bardic_fixture(&fixture);
}

void Test_bardic_affect_refresh_preserves_other_performers_sources(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data second_bard;
  struct player_special_data second_specials;
  struct char_data target;
  long first_source;
  long second_source;

  begin_bardic_fixture(&fixture);
  memset(&second_specials, 0, sizeof(second_specials));
  clear_char(&second_bard);
  second_bard.player_specials = &second_specials;
  second_bard.player.name = "second bardic source";
  IN_ROOM(&second_bard) = 0;
  GET_POS(&second_bard) = POS_STANDING;

  clear_char(&target);
  SET_BIT_AR(MOB_FLAGS(&target), MOB_ISNPC);
  target.player_specials = &dummy_mob;
  target.player.short_descr = "a shared bardic target";
  IN_ROOM(&target) = 0;
  GET_POS(&target) = POS_STANDING;

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_DANCE_OF_PROTECTION, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertTrue(tc, performance_effects(&second_bard, &target, SKILL_DANCE_OF_PROTECTION, 20,
                                       PERFORM_AOE_GROUP));
  first_source = fixture.bard.char_specials.performance_source_id;
  second_source = second_bard.char_specials.performance_source_id;

  CuAssertTrue(tc, first_source != second_source);
  CuAssertIntEquals(tc, 6, count_spell_affects(&target, SKILL_DANCE_OF_PROTECTION));
  CuAssertIntEquals(
      tc, 3, count_spell_affects_from_source(&target, SKILL_DANCE_OF_PROTECTION, first_source));
  CuAssertIntEquals(
      tc, 3, count_spell_affects_from_source(&target, SKILL_DANCE_OF_PROTECTION, second_source));

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_DANCE_OF_PROTECTION, 25,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 6, count_spell_affects(&target, SKILL_DANCE_OF_PROTECTION));
  CuAssertIntEquals(
      tc, 3, count_spell_affects_from_source(&target, SKILL_DANCE_OF_PROTECTION, second_source));

  clear_test_affects(&target);
  affect_total(&target);
  CuAssertTrue(
      tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_FLIGHT, 20, PERFORM_AOE_GROUP));
  CuAssertTrue(
      tc, performance_effects(&second_bard, &target, SKILL_SONG_OF_FLIGHT, 20, PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 2, count_spell_affects(&target, SKILL_SONG_OF_FLIGHT));
  CuAssertIntEquals(tc, 1,
                    count_spell_affects_from_source(&target, SKILL_SONG_OF_FLIGHT, first_source));
  CuAssertIntEquals(tc, 1,
                    count_spell_affects_from_source(&target, SKILL_SONG_OF_FLIGHT, second_source));

  CuAssertTrue(
      tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_FLIGHT, 25, PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 2, count_spell_affects(&target, SKILL_SONG_OF_FLIGHT));
  CuAssertIntEquals(tc, 1,
                    count_spell_affects_from_source(&target, SKILL_SONG_OF_FLIGHT, second_source));

  clear_test_affects(&target);
  affect_total(&target);
  clear_char_event_list(&second_bard);
  end_bardic_fixture(&fixture);
}

void Test_bardic_targets_respect_hearing_construct_and_condition_immunities(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data target;

  begin_bardic_fixture(&fixture);
  clear_char(&target);
  SET_BIT_AR(MOB_FLAGS(&target), MOB_ISNPC);
  target.player_specials = &dummy_mob;
  target.player.short_descr = "an immune bardic target";
  IN_ROOM(&target) = 0;
  GET_POS(&target) = POS_STANDING;
  GET_HIT(&target) = 1;
  GET_MAX_HIT(&target) = 100;

  SET_BIT_AR(MOB_FLAGS(&target), MOB_GOLEM);
  CuAssertTrue(tc, !performance_effects(&fixture.bard, &target, SKILL_SONG_OF_HEALING, 20,
                                        PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 1, GET_HIT(&target));
  REMOVE_BIT_AR(MOB_FLAGS(&target), MOB_GOLEM);

  SET_BIT_AR(AFF_FLAGS(&target), AFF_DEAF);
  CuAssertTrue(tc, !performance_effects(&fixture.bard, &target, SKILL_SONG_OF_HEALING, 20,
                                        PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 1, GET_HIT(&target));
  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_DANCE_OF_PROTECTION, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 3, count_spell_affects(&target, SKILL_DANCE_OF_PROTECTION));

  clear_test_affects(&target);
  affect_total(&target);
  REMOVE_BIT_AR(AFF_FLAGS(&target), AFF_DEAF);
  SET_BIT_AR(MOB_FLAGS(&target), MOB_NODEAF);
  CuAssertTrue(
      tc, !performance_effects(&fixture.bard, &target, SKILL_DEAFENING_SONG, 20, PERFORM_AOE_FOES));
  CuAssertIntEquals(tc, 0, count_spell_affects(&target, SKILL_DEAFENING_SONG));

  REMOVE_BIT_AR(MOB_FLAGS(&target), MOB_NODEAF);
  SET_BIT_AR(MOB_FLAGS(&target), MOB_GOLEM);
  CuAssertTrue(
      tc, !performance_effects(&fixture.bard, &target, SKILL_SONG_OF_FEAR, 20, PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 0, count_spell_affects(&target, SKILL_SONG_OF_FEAR));

  if (FIGHTING(&fixture.bard) != NULL)
    stop_fighting(&fixture.bard);
  if (FIGHTING(&target) != NULL)
    stop_fighting(&target);
  clear_char_event_list(&target);
  end_bardic_fixture(&fixture);
}

void Test_bardic_foe_effects_use_standard_defenses_and_debuff_signs(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data target;
  struct affected_type *af;

  begin_bardic_fixture(&fixture);
  clear_char(&target);
  SET_BIT_AR(MOB_FLAGS(&target), MOB_ISNPC);
  target.player_specials = &dummy_mob;
  target.player.short_descr = "a bardic foe target";
  IN_ROOM(&target) = 0;
  GET_POS(&target) = POS_DEAD;

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_THE_MAGI, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 5, count_spell_affects(&target, SKILL_SONG_OF_THE_MAGI));
  for (af = target.affected; af != NULL; af = af->next)
  {
    if (af->spell == SKILL_SONG_OF_THE_MAGI &&
        (af->location == APPLY_INT || af->location == APPLY_WIS || af->location == APPLY_CHA))
      CuAssertTrue(tc, af->modifier < 0);
  }

  clear_test_affects(&target);
  affect_total(&target);
  GET_POS(&target) = POS_STANDING;
  set_fighting(&fixture.bard, &target);
  set_fighting(&target, &fixture.bard);
  GET_POS(&target) = POS_DEAD;
  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_ACT_OF_FORGETFULNESS, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertPtrEquals(tc, NULL, FIGHTING(&fixture.bard));
  CuAssertPtrEquals(tc, NULL, FIGHTING(&target));

  clear_char_event_list(&target);
  end_bardic_fixture(&fixture);
}

void Test_group_verses_are_reentrant_and_message_only_actual_recipients(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data member;
  struct char_data bystander;
  struct player_special_data member_specials;
  struct player_special_data bystander_specials;
  struct descriptor_data member_descriptor;
  struct descriptor_data bystander_descriptor;
  struct group_data group;

  begin_bardic_fixture(&fixture);
  memset(&member_specials, 0, sizeof(member_specials));
  memset(&bystander_specials, 0, sizeof(bystander_specials));
  clear_char(&member);
  clear_char(&bystander);
  member.player_specials = &member_specials;
  bystander.player_specials = &bystander_specials;
  member.player.name = "bardic group member";
  bystander.player.name = "bardic bystander";
  IN_ROOM(&member) = 0;
  IN_ROOM(&bystander) = 0;
  GET_POS(&member) = POS_STANDING;
  GET_POS(&bystander) = POS_STANDING;
  initialize_test_descriptor(&member_descriptor, &member);
  initialize_test_descriptor(&bystander_descriptor, &bystander);

  memset(&group, 0, sizeof(group));
  group.leader = &fixture.bard;
  group.members = create_list();
  add_to_list(&fixture.bard, group.members);
  add_to_list(&member, group.members);
  fixture.bard.group = &group;
  member.group = &group;
  fixture.bard.next_in_room = &member;
  member.next_in_room = &bystander;

  CuAssertTrue(
      tc, process_performance(&fixture.bard, SKILL_DANCE_OF_PROTECTION, 20, PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 3, count_spell_affects(&fixture.bard, SKILL_DANCE_OF_PROTECTION));
  CuAssertIntEquals(tc, 3, count_spell_affects(&member, SKILL_DANCE_OF_PROTECTION));
  CuAssertIntEquals(tc, 0, count_spell_affects(&bystander, SKILL_DANCE_OF_PROTECTION));
  CuAssertTrue(tc, strstr(member_descriptor.output, "envelops you") != NULL);
  CuAssertTrue(tc, strstr(bystander_descriptor.output, "envelops you") == NULL);

  clear_test_affects(&fixture.bard);
  clear_test_affects(&member);
  fixture.bard.group = NULL;
  member.group = NULL;
  fixture.bard.next_in_room = NULL;
  member.next_in_room = NULL;
  free_list(group.members);
  destroy_test_descriptor(&member_descriptor, &member);
  destroy_test_descriptor(&bystander_descriptor, &bystander);
  clear_char_event_list(&member);
  clear_char_event_list(&bystander);
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

void Test_heightened_harmony_is_one_exact_refreshing_minute_long_bonus(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_perk_data heightened;
  struct affected_type *af;
  int base_perform;

  begin_bardic_fixture(&fixture);
  initialize_bardic_test_perk(&heightened, PERK_BARD_HEIGHTENED_HARMONY, 1, NULL);
  fixture.player_specials.saved.perks = &heightened;
  fixture.player_specials.casting_class = CLASS_BARD;
  CASTING_METAMAGIC(&fixture.bard) = METAMAGIC_EXTEND;
  base_perform = compute_ability(&fixture.bard, ABILITY_PERFORM);

  test_complete_bard_spell_perks(&fixture.bard, SPELL_HASTE, FALSE, 1);
  CuAssertIntEquals(tc, 1, count_spell_affects(&fixture.bard, AFFECT_BARD_HEIGHTENED_HARMONY));
  af = find_spell_affect_location(&fixture.bard, AFFECT_BARD_HEIGHTENED_HARMONY, APPLY_SKILL);
  CuAssertPtrNotNull(tc, af);
  if (af != NULL)
  {
    CuAssertIntEquals(tc, ABILITY_PERFORM, af->specific);
    CuAssertIntEquals(tc, 5, af->modifier);
    CuAssertIntEquals(tc, 10, af->duration);
  }
  CuAssertIntEquals(tc, base_perform + 5, compute_ability(&fixture.bard, ABILITY_PERFORM));

  if (af != NULL)
    af->duration = 2;
  test_complete_bard_spell_perks(&fixture.bard, SPELL_HASTE, FALSE, 1);
  CuAssertIntEquals(tc, 1, count_spell_affects(&fixture.bard, AFFECT_BARD_HEIGHTENED_HARMONY));
  af = find_spell_affect_location(&fixture.bard, AFFECT_BARD_HEIGHTENED_HARMONY, APPLY_SKILL);
  CuAssertPtrNotNull(tc, af);
  if (af != NULL)
    CuAssertIntEquals(tc, 10, af->duration);
  CuAssertIntEquals(tc, base_perform + 5, compute_ability(&fixture.bard, ABILITY_PERFORM));

  end_bardic_fixture(&fixture);
}

void Test_crescendo_scopes_damage_and_dc_to_exactly_one_whole_bard_cast(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_perk_data crescendo;
  struct char_perk_data harmonic;
  struct char_data first_target;
  struct char_data second_target;
  bool trigger_symphonic;

  begin_bardic_fixture(&fixture);
  initialize_bardic_test_perk(&harmonic, PERK_BARD_HARMONIC_CASTING, 1, NULL);
  initialize_bardic_test_perk(&crescendo, PERK_BARD_CRESCENDO, 1, NULL);
  fixture.player_specials.saved.perks = &crescendo;
  fixture.player_specials.casting_class = CLASS_BARD;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;

  trigger_symphonic = FALSE;
  test_prepare_bard_spell_perks(&fixture.bard, &trigger_symphonic);
  CuAssertTrue(tc, !IS_PERFORMING(&fixture.bard));
  CuAssertIntEquals(tc, 1, GET_CRESCENDO_USED(&fixture.bard));
  CuAssertIntEquals(tc, 1, GET_CRESCENDO_DICE(&fixture.bard));
  CuAssertIntEquals(tc, 2, GET_CRESCENDO_DC(&fixture.bard));

  test_reset_bard_crescendo_observations();
  CuAssertTrue(tc,
               call_magic(&fixture.bard, &fixture.bard, NULL, SPELL_HASTE, 0, 10, CAST_SPELL) != 0);
  CuAssertIntEquals(tc, 0, test_get_bard_crescendo_damage_applications());
  CuAssertIntEquals(tc, 0, test_get_bard_crescendo_save_applications());
  CuAssertIntEquals(tc, 1, GET_CRESCENDO_DICE(&fixture.bard));
  test_clear_bard_spell_perks(&fixture.bard);

  fixture.player_specials.saved.perks = &crescendo;
  crescendo.next = &harmonic;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;
  GET_CRESCENDO_USED(&fixture.bard) = 0;
  test_prepare_bard_spell_perks(&fixture.bard, &trigger_symphonic);
  CuAssertTrue(tc, IS_PERFORMING(&fixture.bard));

  initialize_bardic_test_npc(&first_target, "a crescendo single target");
  GET_HIT(&first_target) = GET_MAX_HIT(&first_target) = 1000;
  fixture.bard.next_in_room = &first_target;
  test_reset_bard_crescendo_observations();
  CuAssertTrue(tc, call_magic(&fixture.bard, &first_target, NULL, SPELL_MAGIC_MISSILE, 0, 10,
                              CAST_SPELL) != 0);
  CuAssertIntEquals(tc, 1, test_get_bard_crescendo_damage_applications());
  CuAssertIntEquals(tc, 1, GET_CRESCENDO_DICE(&fixture.bard));
  test_clear_bard_spell_perks(&fixture.bard);

  initialize_bardic_test_npc(&second_target, "a crescendo area target");
  GET_HIT(&second_target) = GET_MAX_HIT(&second_target) = 1000;
  first_target.next_in_room = &second_target;
  fixture.player_specials.casting_class = CLASS_BARD;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;
  GET_CRESCENDO_USED(&fixture.bard) = 0;
  test_prepare_bard_spell_perks(&fixture.bard, &trigger_symphonic);
  test_reset_bard_crescendo_observations();
  circle_srandom(1);
  CuAssertTrue(tc, call_magic(&fixture.bard, NULL, NULL, SPELL_FIREBALL, 0, 10, CAST_SPELL) != 0);
  CuAssertIntEquals(tc, 2, test_get_bard_crescendo_damage_applications());
  CuAssertIntEquals(tc, 2, test_get_bard_crescendo_save_applications());
  CuAssertIntEquals(tc, 1, GET_CRESCENDO_DICE(&fixture.bard));
  CuAssertIntEquals(tc, 2, GET_CRESCENDO_DC(&fixture.bard));
  test_clear_bard_spell_perks(&fixture.bard);

  if (FIGHTING(&fixture.bard) != NULL)
    stop_fighting(&fixture.bard);
  if (FIGHTING(&first_target) != NULL)
    stop_fighting(&first_target);
  if (FIGHTING(&second_target) != NULL)
    stop_fighting(&second_target);
  fixture.bard.next_in_room = NULL;
  first_target.next_in_room = NULL;
  clear_char_event_list(&first_target);
  clear_char_event_list(&second_target);
  circle_srandom((unsigned long)time(NULL));
  end_bardic_fixture(&fixture);
}

void Test_spellsinger_support_auras_require_an_active_grouped_performer(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data member;
  struct char_data bystander;
  struct player_special_data member_specials;
  struct player_special_data bystander_specials;
  struct char_perk_data protective;
  struct char_perk_data aria;
  struct char_perk_data anthem;
  struct char_perk_data banner;
  struct affected_type slow_af;
  struct group_data group;

  begin_bardic_fixture(&fixture);
  initialize_bardic_test_pc(&member, &member_specials, "a supported bard ally");
  initialize_bardic_test_pc(&bystander, &bystander_specials, "an unsupported bard bystander");
  initialize_bardic_test_perk(&banner, PERK_BARD_BANNER_VERSE, 1, NULL);
  initialize_bardic_test_perk(&anthem, PERK_BARD_ANTHEM_OF_FORTITUDE, 1, &banner);
  initialize_bardic_test_perk(&aria, PERK_BARD_ARIA_OF_STASIS, 1, &anthem);
  initialize_bardic_test_perk(&protective, PERK_BARD_PROTECTIVE_CHORUS, 1, &aria);
  fixture.player_specials.saved.perks = &protective;

  memset(&group, 0, sizeof(group));
  group.leader = &fixture.bard;
  group.members = create_list();
  add_to_list(&fixture.bard, group.members);
  add_to_list(&member, group.members);
  fixture.bard.group = &group;
  member.group = &group;
  fixture.bard.next_in_room = &member;
  member.next_in_room = &bystander;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;

  CuAssertIntEquals(tc, 2, get_bard_protective_chorus_save_bonus(&member));
  CuAssertIntEquals(tc, 2, get_bard_protective_chorus_ac_bonus(&member));
  CuAssertIntEquals(tc, 4, get_bard_aria_stasis_ally_saves_bonus(&member));
  CuAssertTrue(tc, has_bard_aria_stasis_slow_immunity(&member));
  CuAssertIntEquals(tc, 10, get_bard_anthem_fortitude_hp_bonus(&member));
  CuAssertIntEquals(tc, 2, get_bard_anthem_fortitude_save_bonus(&member));
  CuAssertIntEquals(tc, 2, get_bard_banner_verse_tohit_bonus(&member));
  CuAssertIntEquals(tc, 2, get_bard_banner_verse_save_bonus(&member));
  CuAssertIntEquals(tc, -2, get_bard_aria_stasis_enemy_tohit_penalty(&bystander));
  CuAssertIntEquals(tc, 10, get_bard_aria_stasis_movement_penalty(&bystander));
  CuAssertIntEquals(tc, 0, get_bard_protective_chorus_save_bonus(&bystander));

  new_affect(&slow_af);
  slow_af.spell = SPELL_SLOW;
  slow_af.duration = 2;
  SET_BIT_AR(slow_af.bitvector, AFF_SLOW);
  affect_to_char(&member, &slow_af);
  CuAssertTrue(tc, !AFF_FLAGGED(&member, AFF_SLOW));

  stop_bardic_performance(&fixture.bard, FALSE);
  CuAssertIntEquals(tc, 0, get_bard_protective_chorus_save_bonus(&member));
  CuAssertIntEquals(tc, 0, get_bard_aria_stasis_ally_saves_bonus(&member));
  CuAssertIntEquals(tc, 0, get_bard_anthem_fortitude_hp_bonus(&member));
  CuAssertIntEquals(tc, 0, get_bard_banner_verse_tohit_bonus(&member));
  CuAssertIntEquals(tc, 0, get_bard_aria_stasis_enemy_tohit_penalty(&bystander));

  clear_test_affects(&member);
  fixture.bard.group = NULL;
  member.group = NULL;
  free_list(group.members);
  fixture.bard.next_in_room = NULL;
  member.next_in_room = NULL;
  clear_char_event_list(&member);
  clear_char_event_list(&bystander);
  end_bardic_fixture(&fixture);
}

void Test_spellsong_maestra_is_bard_only_and_implements_all_components(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_perk_data maestra;

  begin_bardic_fixture(&fixture);
  initialize_bardic_test_perk(&maestra, PERK_BARD_SPELLSONG_MAESTRA, 1, NULL);
  fixture.player_specials.saved.perks = &maestra;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;
  fixture.player_specials.casting_class = CLASS_BARD;

  CuAssertIntEquals(tc, 2, get_bard_spellsong_maestra_caster_bonus(&fixture.bard));
  CuAssertIntEquals(tc, 2, get_bard_spellsong_maestra_dc_bonus(&fixture.bard));
  CuAssertIntEquals(tc, 0,
                    test_calculate_metamagic_modifier(&fixture.bard, CLASS_BARD,
                                                      METAMAGIC_EMPOWER | METAMAGIC_EXTEND));
  CuAssertIntEquals(tc, 3,
                    test_calculate_metamagic_modifier(&fixture.bard, CLASS_WIZARD,
                                                      METAMAGIC_EMPOWER | METAMAGIC_EXTEND));

  fixture.player_specials.casting_class = CLASS_WIZARD;
  CuAssertIntEquals(tc, 0, get_bard_spellsong_maestra_caster_bonus(&fixture.bard));
  CuAssertIntEquals(tc, 0, get_bard_spellsong_maestra_dc_bonus(&fixture.bard));

  end_bardic_fixture(&fixture);
}

void Test_symphonic_resonance_obeys_success_save_pk_and_resource_contracts(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data npc_target;
  struct char_data pc_target;
  struct player_special_data pc_specials;
  struct char_perk_data symphonic;
  struct char_perk_data endless;
  struct innate_magic_data *recovering_slot;
  bool saved_pk_allowed;
  int i;

  begin_bardic_fixture(&fixture);
  initialize_bardic_test_perk(&endless, PERK_BARD_ENDLESS_REFRAIN, 1, NULL);
  initialize_bardic_test_perk(&symphonic, PERK_BARD_SYMPHONIC_RESONANCE, 1, &endless);
  fixture.player_specials.saved.perks = &symphonic;
  fixture.player_specials.casting_class = CLASS_BARD;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;

  initialize_bardic_test_npc(&npc_target, "a symphonic resonance NPC target");
  initialize_bardic_test_pc(&pc_target, &pc_specials, "a symphonic resonance PC target");
  GET_SAVE(&npc_target, SAVING_WILL) = -100;
  GET_SAVE(&pc_target, SAVING_WILL) = -100;
  fixture.bard.next_in_room = &npc_target;
  npc_target.next_in_room = &pc_target;
  saved_pk_allowed = CONFIG_PK_ALLOWED;
  CONFIG_PK_ALLOWED = FALSE;

  test_complete_bard_spell_perks(&fixture.bard, SPELL_CHARM, TRUE, 0);
  CuAssertTrue(tc, !AFF_FLAGGED(&npc_target, AFF_DAZED));
  test_complete_bard_spell_perks(&fixture.bard, SPELL_CHARM, TRUE, 1);
  CuAssertTrue(tc, AFF_FLAGGED(&npc_target, AFF_DAZED));
  CuAssertTrue(tc, !AFF_FLAGGED(&pc_target, AFF_DAZED));

  clear_test_affects(&npc_target);
  clear_test_affects(&pc_target);
  CONFIG_PK_ALLOWED = TRUE;
  circle_srandom(1);
  test_complete_bard_spell_perks(&fixture.bard, SPELL_CHARM, TRUE, 1);
  CuAssertTrue(tc, AFF_FLAGGED(&npc_target, AFF_DAZED));
  CuAssertTrue(tc, AFF_FLAGGED(&pc_target, AFF_DAZED));

  clear_test_affects(&npc_target);
  clear_test_affects(&pc_target);
  GET_SAVE(&npc_target, SAVING_WILL) = 100;
  GET_SAVE(&pc_target, SAVING_WILL) = 100;
  circle_srandom(1);
  test_complete_bard_spell_perks(&fixture.bard, SPELL_CHARM, TRUE, 1);
  CuAssertTrue(tc, !AFF_FLAGGED(&npc_target, AFF_DAZED));
  CuAssertTrue(tc, !AFF_FLAGGED(&pc_target, AFF_DAZED));

  GET_HIT(&fixture.bard) = GET_MAX_HIT(&fixture.bard);
  circle_srandom(1);
  for (i = 0; i < 20; i++)
    test_pulse_bard_symphonic_resonance(&fixture.bard);
  CuAssertTrue(tc, GET_HIT(&fixture.bard) > GET_MAX_HIT(&fixture.bard));
  CuAssertTrue(tc,
               GET_HIT(&fixture.bard) <= GET_MAX_HIT(&fixture.bard) + BARDIC_SYMPHONIC_TEMP_HP_CAP);

  recovering_slot = calloc(1, sizeof(*recovering_slot));
  CuAssertPtrNotNull(tc, recovering_slot);
  if (recovering_slot != NULL)
  {
    recovering_slot->circle = 2;
    INNATE_MAGIC(&fixture.bard, CLASS_BARD) = recovering_slot;
    test_pulse_bard_endless_refrain(&fixture.bard);
    CuAssertPtrEquals(tc, NULL, INNATE_MAGIC(&fixture.bard, CLASS_BARD));
  }

  CONFIG_PK_ALLOWED = saved_pk_allowed;
  fixture.bard.next_in_room = NULL;
  npc_target.next_in_room = NULL;
  clear_char_event_list(&npc_target);
  clear_char_event_list(&pc_target);
  circle_srandom((unsigned long)time(NULL));
  end_bardic_fixture(&fixture);
}

void Test_battle_hymn_and_dominance_modify_song_of_heroism_recipients(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data target;
  struct char_perk_data battle_i;
  struct char_perk_data battle_ii;
  struct char_perk_data dominance;
  struct affected_type *af;

  begin_bardic_fixture(&fixture);
  initialize_bardic_test_npc(&target, "a heroic warchanter ally");
  initialize_bardic_test_perk(&dominance, PERK_BARD_WARCHANTERS_DOMINANCE, 1, NULL);
  initialize_bardic_test_perk(&battle_ii, PERK_BARD_BATTLE_HYMN_II, 2, &dominance);
  initialize_bardic_test_perk(&battle_i, PERK_BARD_BATTLE_HYMN_I, 3, &battle_ii);
  fixture.player_specials.saved.perks = &battle_i;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 3;

  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_SONG_OF_HEROISM, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertIntEquals(tc, 6, count_spell_affects(&target, SKILL_SONG_OF_HEROISM));
  af = find_spell_affect_location(&target, SKILL_SONG_OF_HEROISM, APPLY_DAMROLL);
  CuAssertPtrNotNull(tc, af);
  if (af != NULL)
    CuAssertIntEquals(tc, 7, af->modifier);
  af = find_spell_affect_location(&target, SKILL_SONG_OF_HEROISM, APPLY_HITROLL);
  CuAssertPtrNotNull(tc, af);
  if (af != NULL)
    CuAssertIntEquals(tc, 4, af->modifier);
  af = find_spell_affect_location(&target, SKILL_SONG_OF_HEROISM, APPLY_AC_NEW);
  CuAssertPtrNotNull(tc, af);
  if (af != NULL)
    CuAssertIntEquals(tc, 1, af->modifier);

  clear_test_affects(&target);
  CuAssertTrue(tc, performance_effects(&fixture.bard, &target, SKILL_DANCE_OF_PROTECTION, 20,
                                       PERFORM_AOE_GROUP));
  CuAssertPtrEquals(tc, NULL,
                    find_spell_affect_location(&target, SKILL_DANCE_OF_PROTECTION, APPLY_DAMROLL));

  clear_test_affects(&target);
  clear_char_event_list(&target);
  end_bardic_fixture(&fixture);
}

void Test_warbeat_buffs_only_allies_and_opens_combat_once(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data member;
  struct char_data bystander;
  struct char_data enemy;
  struct player_special_data member_specials;
  struct player_special_data bystander_specials;
  struct char_perk_data warbeat;
  struct char_perk_data dominance;
  struct affected_type *af;
  struct group_data group;

  begin_bardic_fixture(&fixture);
  initialize_bardic_test_pc(&member, &member_specials, "a warbeat ally");
  initialize_bardic_test_pc(&bystander, &bystander_specials, "a warbeat bystander");
  initialize_bardic_test_npc(&enemy, "a warbeat enemy");
  GET_HIT(&enemy) = GET_MAX_HIT(&enemy) = 10000;
  initialize_bardic_test_perk(&dominance, PERK_BARD_WARCHANTERS_DOMINANCE, 1, NULL);
  initialize_bardic_test_perk(&warbeat, PERK_BARD_WARBEAT, 1, &dominance);
  fixture.player_specials.saved.perks = &warbeat;

  memset(&group, 0, sizeof(group));
  group.leader = &fixture.bard;
  group.members = create_list();
  add_to_list(&fixture.bard, group.members);
  add_to_list(&member, group.members);
  fixture.bard.group = &group;
  member.group = &group;
  fixture.bard.next_in_room = &member;
  member.next_in_room = &bystander;
  bystander.next_in_room = &enemy;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;

  circle_srandom(1);
  test_apply_bard_warbeat_allies(&fixture.bard);
  CuAssertIntEquals(tc, 2, count_spell_affects(&fixture.bard, AFFECT_BARD_WARBEAT));
  CuAssertIntEquals(tc, 2, count_spell_affects(&member, AFFECT_BARD_WARBEAT));
  CuAssertIntEquals(tc, 0, count_spell_affects(&bystander, AFFECT_BARD_WARBEAT));
  af = find_spell_affect_location(&member, AFFECT_BARD_WARBEAT, APPLY_DAMROLL);
  CuAssertPtrNotNull(tc, af);
  if (af != NULL)
    CuAssertTrue(tc, af->modifier >= 2 && af->modifier <= 8);
  af = find_spell_affect_location(&member, AFFECT_BARD_WARBEAT, APPLY_AC_NEW);
  CuAssertPtrNotNull(tc, af);
  if (af != NULL)
    CuAssertIntEquals(tc, 1, af->modifier);

  set_fighting(&fixture.bard, &enemy);
  test_reset_bard_warbeat_observations();
  perform_violence(&fixture.bard, 0);
  CuAssertIntEquals(tc, 1, test_get_bard_warbeat_opening_attacks());
  CuAssertIntEquals(tc, 1, GET_WARBEAT_USED(&fixture.bard));
  if (FIGHTING(&fixture.bard) != NULL)
    perform_violence(&fixture.bard, 0);
  CuAssertIntEquals(tc, 1, test_get_bard_warbeat_opening_attacks());

  if (FIGHTING(&fixture.bard) != NULL)
    stop_fighting(&fixture.bard);
  if (FIGHTING(&enemy) != NULL)
    stop_fighting(&enemy);
  CuAssertIntEquals(tc, 0, GET_WARBEAT_USED(&fixture.bard));
  clear_test_affects(&fixture.bard);
  clear_test_affects(&member);
  fixture.bard.group = NULL;
  member.group = NULL;
  free_list(group.members);
  fixture.bard.next_in_room = NULL;
  member.next_in_room = NULL;
  bystander.next_in_room = NULL;
  clear_char_event_list(&member);
  clear_char_event_list(&bystander);
  clear_char_event_list(&enemy);
  circle_srandom((unsigned long)time(NULL));
  end_bardic_fixture(&fixture);
}

void Test_frostbite_cadence_and_steel_use_standard_defender_paths(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data target;
  struct char_perk_data frostbite_i;
  struct char_perk_data frostbite_ii;
  struct char_perk_data cadence;
  struct char_perk_data steel;
  int attacker_hp;
  int active_reduction;
  int inactive_reduction;

  begin_bardic_fixture(&fixture);
  initialize_bardic_test_npc(&target, "a frostbite cadence target");
  initialize_bardic_test_perk(&steel, PERK_BARD_STEEL_SERENADE, 1, NULL);
  initialize_bardic_test_perk(&cadence, PERK_BARD_COMMANDING_CADENCE, 1, &steel);
  initialize_bardic_test_perk(&frostbite_ii, PERK_BARD_FROSTBITE_REFRAIN_II, 2, &cadence);
  initialize_bardic_test_perk(&frostbite_i, PERK_BARD_FROSTBITE_REFRAIN_I, 3, &frostbite_ii);
  fixture.player_specials.saved.perks = &frostbite_i;
  fixture.bard.next_in_room = &target;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;
  attacker_hp = GET_HIT(&fixture.bard);

  GET_RESISTANCES(&target, DAM_COLD) = 100;
  test_apply_bard_frostbite_rider(&fixture.bard, &target, 10, 1, ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, 100, GET_HIT(&target));
  CuAssertIntEquals(tc, attacker_hp, GET_HIT(&fixture.bard));

  GET_RESISTANCES(&target, DAM_COLD) = 0;
  test_apply_bard_frostbite_rider(&fixture.bard, &target, 10, 1, ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, 95, GET_HIT(&target));
  CuAssertIntEquals(tc, attacker_hp, GET_HIT(&fixture.bard));

  GET_SAVE(&fixture.bard, SAVING_WILL) = 100;
  GET_SAVE(&target, SAVING_WILL) = -100;
  circle_srandom(1);
  test_apply_bard_commanding_cadence(&fixture.bard, &target, 1);
  CuAssertTrue(tc, AFF_FLAGGED(&target, AFF_DAZED));
  CuAssertTrue(tc, affected_by_spell(&target, AFFECT_BARD_COMMANDING_CADENCE_IMMUNITY));

  clear_test_affects(&target);
  GET_SAVE(&fixture.bard, SAVING_WILL) = -100;
  GET_SAVE(&target, SAVING_WILL) = 100;
  circle_srandom(1);
  test_apply_bard_commanding_cadence(&fixture.bard, &target, 1);
  CuAssertTrue(tc, !AFF_FLAGGED(&target, AFF_DAZED));
  CuAssertTrue(tc, affected_by_spell(&target, AFFECT_BARD_COMMANDING_CADENCE_IMMUNITY));

  active_reduction = compute_damtype_reduction(&fixture.bard, DAM_SLASHING, &target, TYPE_HIT);
  stop_bardic_performance(&fixture.bard, FALSE);
  inactive_reduction = compute_damtype_reduction(&fixture.bard, DAM_SLASHING, &target, TYPE_HIT);
  CuAssertIntEquals(tc, inactive_reduction + 10, active_reduction);

  if (FIGHTING(&fixture.bard) != NULL)
    stop_fighting(&fixture.bard);
  if (FIGHTING(&target) != NULL)
    stop_fighting(&target);
  clear_test_affects(&target);
  fixture.bard.next_in_room = NULL;
  clear_char_event_list(&target);
  circle_srandom((unsigned long)time(NULL));
  end_bardic_fixture(&fixture);
}

void Test_winters_war_march_hits_each_foe_once_with_fortitude_and_cold_resistance(CuTest *tc)
{
  struct bardic_fixture fixture;
  struct char_data failed_target;
  struct char_data saved_target;
  struct char_perk_data winter;
  struct affected_type *af;
  int saved_damage;

  begin_bardic_fixture(&fixture);
  initialize_bardic_test_npc(&failed_target, "a winter march failing target");
  initialize_bardic_test_npc(&saved_target, "a winter march saving target");
  GET_HIT(&failed_target) = GET_MAX_HIT(&failed_target) = 1000;
  GET_HIT(&saved_target) = GET_MAX_HIT(&saved_target) = 1000;
  initialize_bardic_test_perk(&winter, PERK_BARD_WINTERS_WAR_MARCH, 1, NULL);
  fixture.player_specials.saved.perks = &winter;
  fixture.bard.next_in_room = &failed_target;
  failed_target.next_in_room = &saved_target;
  IS_PERFORMING(&fixture.bard) = TRUE;
  GET_PERFORMING(&fixture.bard) = 0;

  GET_SAVE(&fixture.bard, SAVING_FORT) = 100;
  GET_SAVE(&failed_target, SAVING_FORT) = -100;
  GET_SAVE(&saved_target, SAVING_FORT) = 100;
  GET_REAL_RESISTANCES(&failed_target, DAM_COLD) = 100;
  GET_RESISTANCES(&failed_target, DAM_COLD) = 100;
  circle_srandom(1);
  test_pulse_bard_winters_war_march(&fixture.bard);

  CuAssertIntEquals(tc, 1000, GET_HIT(&failed_target));
  CuAssertIntEquals(tc, 1, count_spell_affects(&failed_target, AFFECT_BARD_WINTERS_WAR_MARCH));
  af = find_spell_affect_location(&failed_target, AFFECT_BARD_WINTERS_WAR_MARCH, APPLY_NONE);
  CuAssertPtrNotNull(tc, af);
  if (af != NULL)
    CuAssertIntEquals(tc, 3, af->duration);
  CuAssertTrue(tc, AFF_FLAGGED(&failed_target, AFF_SLOW));

  saved_damage = 1000 - GET_HIT(&saved_target);
  CuAssertTrue(tc, saved_damage >= 2 && saved_damage <= 12);
  CuAssertIntEquals(tc, 1, count_spell_affects(&saved_target, AFFECT_BARD_WINTERS_WAR_MARCH));
  af = find_spell_affect_location(&saved_target, AFFECT_BARD_WINTERS_WAR_MARCH, APPLY_NONE);
  CuAssertPtrNotNull(tc, af);
  if (af != NULL)
    CuAssertIntEquals(tc, 1, af->duration);
  CuAssertTrue(tc, AFF_FLAGGED(&saved_target, AFF_SLOW));

  if (FIGHTING(&fixture.bard) != NULL)
    stop_fighting(&fixture.bard);
  if (FIGHTING(&failed_target) != NULL)
    stop_fighting(&failed_target);
  if (FIGHTING(&saved_target) != NULL)
    stop_fighting(&saved_target);
  clear_test_affects(&failed_target);
  clear_test_affects(&saved_target);
  fixture.bard.next_in_room = NULL;
  failed_target.next_in_room = NULL;
  clear_char_event_list(&failed_target);
  clear_char_event_list(&saved_target);
  circle_srandom((unsigned long)time(NULL));
  end_bardic_fixture(&fixture);
}
