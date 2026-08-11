#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/handler.h"
#include "../../src/modify.h"
#include "../../src/mud_event.h"
#include "../../src/mudlim.h"
#include "../../src/net/protocol.h"
#include "../../src/character/evolutions.h"
#include "../../src/character/feats.h"
#include "../../src/magic/domains_schools.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/craft/craft.h"

#include <stdlib.h>
#include <string.h>

static void cleanup_test_descriptor(struct descriptor_data *descriptor)
{
  if (descriptor == NULL)
    return;

  if (descriptor->pProtocol != NULL)
  {
    ProtocolDestroy(descriptor->pProtocol);
    descriptor->pProtocol = NULL;
  }

  if (descriptor->large_outbuf != NULL)
  {
    free(descriptor->large_outbuf->text);
    free(descriptor->large_outbuf);
    descriptor->large_outbuf = NULL;
    if (buf_largecount > 0)
      buf_largecount--;
  }

  descriptor->output = descriptor->small_outbuf;
}

void Test_rol_psionic_regeneration_room_doubles_tick_gain(CuTest *tc)
{
  struct char_data ch;
  struct descriptor_data descriptor;
  struct descriptor_data *saved_descriptor_list;
  struct player_special_data player_specials;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  int normal_gain;
  int accelerated_gain;

  clear_char(&ch);
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&room, 0, sizeof(room));
  ch.player_specials = &player_specials;
  ch.desc = &descriptor;
  descriptor.character = &ch;
  descriptor.connected = CON_PLAYING;
  GET_POS(&ch) = POS_STANDING;
  GET_PSP(&ch) = 10;
  GET_MAX_PSP(&ch) = 1000;
  IN_ROOM(&ch) = 0;

  saved_descriptor_list = descriptor_list;
  saved_world = world;
  saved_top_of_world = top_of_world;
  descriptor_list = &descriptor;
  world = &room;
  top_of_world = 0;

  regen_psp();
  normal_gain = GET_PSP(&ch) - 10;
  GET_PSP(&ch) = 10;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_PSP_REGEN);
  regen_psp();
  accelerated_gain = GET_PSP(&ch) - 10;

  descriptor_list = saved_descriptor_list;
  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssert(tc, "normal PSP tick must gain power", normal_gain > 0);
  CuAssertIntEquals(tc, normal_gain * 2, accelerated_gain);
}

void Test_spells_production_classification_helpers(CuTest *tc)
{
  CuAssertTrue(tc, is_wall_spell(SPELL_WALL_OF_FIRE));
  CuAssertTrue(tc, is_wall_spell(SPELL_WALL_OF_FORCE));
  CuAssertTrue(tc, !is_wall_spell(SPELL_MAGIC_MISSILE));
  CuAssertTrue(tc, isEpicSpell(SPELL_HELLBALL));
  CuAssertTrue(tc, !isEpicSpell(SPELL_CURE_LIGHT));
}

void Test_blackguard_counts_as_a_spellcasting_class(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  CLASS_LEVEL((&ch), CLASS_BLACKGUARD) = 9;

  CuAssertTrue(tc, IS_SPELLCASTER_CLASS(CLASS_BLACKGUARD));
  CuAssertTrue(tc, !IS_SPELLCASTER_CLASS(CLASS_ROGUE));
  CuAssertIntEquals(tc, 1, get_number_of_spellcasting_classes(&ch));
  CuAssertIntEquals(tc, CLASS_BLACKGUARD, get_first_spellcasting_classes(&ch));

  CLASS_LEVEL((&ch), CLASS_WARLOCK) = 1;

  CuAssertIntEquals(tc, 2, get_number_of_spellcasting_classes(&ch));
  CuAssertIntEquals(tc, CLASS_BLACKGUARD, get_first_spellcasting_classes(&ch));
}

void Test_practiced_sneak_makes_stealth_a_class_skill(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  int saved_stealth_classification;
  int cross_class_direct;
  int cross_class_multiclass;
  int practiced_sneak_direct;
  int practiced_sneak_multiclass;

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  GET_CLASS(&ch) = CLASS_WARRIOR;
  CLASS_LEVEL((&ch), CLASS_WARRIOR) = 1;

  saved_stealth_classification = class_list[CLASS_WARRIOR].class_abil[ABILITY_STEALTH];
  class_list[CLASS_WARRIOR].class_abil[ABILITY_STEALTH] = 1;

  cross_class_direct = modify_class_ability(&ch, ABILITY_STEALTH, CLASS_WARRIOR);
  cross_class_multiclass = is_class_skill(&ch, ABILITY_STEALTH);

  SET_FEAT(&ch, FEAT_PRACTICED_SNEAK, 1);

  practiced_sneak_direct = modify_class_ability(&ch, ABILITY_STEALTH, CLASS_WARRIOR);
  practiced_sneak_multiclass = is_class_skill(&ch, ABILITY_STEALTH);

  class_list[CLASS_WARRIOR].class_abil[ABILITY_STEALTH] = saved_stealth_classification;

  CuAssertIntEquals(tc, 1, cross_class_direct);
  CuAssertIntEquals(tc, 1, cross_class_multiclass);
  CuAssertIntEquals(tc, 2, practiced_sneak_direct);
  CuAssertIntEquals(tc, 2, practiced_sneak_multiclass);
}

void Test_warlock_darkness_lasts_fifteen_rounds(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct room_data room;
  struct room_data *saved_world;
  struct raff_node *saved_raff_list;
  struct raff_node *darkness;
  room_rnum saved_top_of_world;
  int duration;
  int affection;
  int spell;

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&room, 0, sizeof(room));
  ch.player_specials = &player_specials;
  ch.player.name = "warlock darkness test character";
  IN_ROOM(&ch) = 0;
  GET_CLASS(&ch) = CLASS_WARLOCK;
  GET_LEVEL(&ch) = 6;
  CLASS_LEVEL((&ch), CLASS_WARLOCK) = 6;

  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_raff_list = raff_list;
  world = &room;
  top_of_world = 0;
  raff_list = NULL;

  mag_room(GET_LEVEL(&ch), &ch, NULL, WARLOCK_DARKNESS, CAST_INNATE);

  darkness = raff_list;
  duration = darkness == NULL ? -1 : darkness->timer;
  affection = darkness == NULL ? -1 : darkness->affection;
  spell = darkness == NULL ? -1 : darkness->spell;
  free(darkness);
  raff_list = saved_raff_list;
  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertIntEquals(tc, 15, duration);
  CuAssertIntEquals(tc, RAFF_DARKNESS, affection);
  CuAssertIntEquals(tc, WARLOCK_DARKNESS, spell);
}

void Test_legacy_crafting_reports_modified_experience(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct descriptor_data descriptor;
  int saved_max_exp_gain;
  int saved_experience_multiplier;
  int gained;
  bool modified_gain_reported;
  bool base_gain_reported;

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &ch;
  descriptor.pProtocol = ProtocolCreate();
  ch.desc = &descriptor;
  ch.player_specials = &player_specials;
  ch.player.name = "legacy crafting experience test character";
  IN_ROOM(&ch) = NOWHERE;
  GET_CLASS(&ch) = CLASS_WARRIOR;
  GET_LEVEL(&ch) = 6;
  ch.player_specials->saved.stage_info.current_stage = 1;

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize the legacy crafting experience fixture");
    return;
  }

  saved_max_exp_gain = CONFIG_MAX_EXP_GAIN;
  saved_experience_multiplier = CONFIG_EXPERIENCE_MULTIPLIER;
  CONFIG_MAX_EXP_GAIN = 100000;
  CONFIG_EXPERIENCE_MULTIPLIER = 100;
  GET_EXP(&ch) = level_exp(&ch, GET_LEVEL(&ch));

  gained = test_award_legacy_crafting_experience(&ch, 10);
  modified_gain_reported = strstr(descriptor.output, "You gained 25 exp for crafting...") != NULL;
  base_gain_reported = strstr(descriptor.output, "You gained 10 exp for crafting...") != NULL;

  ch.desc = NULL;
  cleanup_test_descriptor(&descriptor);
  CONFIG_MAX_EXP_GAIN = saved_max_exp_gain;
  CONFIG_EXPERIENCE_MULTIPLIER = saved_experience_multiplier;

  CuAssertIntEquals(tc, 25, gained);
  CuAssertTrue(tc, modified_gain_reported);
  CuAssertTrue(tc, !base_gain_reported);
}

void Test_legacy_supply_orders_improve_the_material_skill(CuTest *tc)
{
  CuAssertIntEquals(tc, SKILL_MINING, test_legacy_supply_order_skill(MATERIAL_STEEL));
  CuAssertIntEquals(tc, SKILL_MINING, test_legacy_supply_order_skill(MATERIAL_BRONZE));
  CuAssertIntEquals(tc, SKILL_MINING, test_legacy_supply_order_skill(MATERIAL_COPPER));
  CuAssertIntEquals(tc, SKILL_HUNTING, test_legacy_supply_order_skill(MATERIAL_LEATHER));
  CuAssertIntEquals(tc, SKILL_FORESTING, test_legacy_supply_order_skill(MATERIAL_WOOD));
  CuAssertIntEquals(tc, SKILL_KNITTING, test_legacy_supply_order_skill(MATERIAL_WOOL));
  CuAssertIntEquals(tc, SKILL_KNITTING, test_legacy_supply_order_skill(MATERIAL_SATIN));
  CuAssertIntEquals(tc, -1, test_legacy_supply_order_skill(MATERIAL_GLASS));
}

void Test_spells_production_name_and_level_lookup(CuTest *tc)
{
  const char *saved_name;
  int saved_levels[NUM_CLASSES];
  int i;

  saved_name = spell_info[SPELL_MAGIC_MISSILE].name;
  for (i = 0; i < NUM_CLASSES; i++)
  {
    saved_levels[i] = spell_info[SPELL_MAGIC_MISSILE].min_level[i];
    spell_info[SPELL_MAGIC_MISSILE].min_level[i] = 20;
  }
  spell_info[SPELL_MAGIC_MISSILE].name = "Coverage Missile";
  spell_info[SPELL_MAGIC_MISSILE].min_level[CLASS_WIZARD] = 3;

  CuAssertStrEquals(tc, "Coverage Missile", spell_name(SPELL_MAGIC_MISSILE));
  CuAssertIntEquals(tc, 3, lowest_spell_level(SPELL_MAGIC_MISSILE));
  CuAssertIntEquals(tc, SPELL_MAGIC_MISSILE, find_skill_num("coverage missile"));
  CuAssertIntEquals(tc, SPELL_MAGIC_MISSILE, find_skill_num("cov mis"));
  CuAssertIntEquals(tc, -1, find_skill_num("not a real coverage spell"));

  spell_info[SPELL_MAGIC_MISSILE].name = saved_name;
  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[SPELL_MAGIC_MISSILE].min_level[i] = saved_levels[i];
}

void Test_cure_critical_spell_names_are_canonical(CuTest *tc)
{
  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();

  CuAssertStrEquals(tc, "cure critical", spell_name(SPELL_CURE_CRITIC));
  CuAssertStrEquals(tc, "mass cure critical", spell_name(SPELL_MASS_CURE_CRIT));
  CuAssertIntEquals(tc, SPELL_CURE_CRITIC, find_skill_num("cure critic"));
  CuAssertIntEquals(tc, SPELL_MASS_CURE_CRIT, find_skill_num("mass cure critic"));
}

void Test_spells_production_cantrip_bounds(CuTest *tc)
{
  bool saved_cantrip;

  saved_cantrip = spell_info[SPELL_DETECT_MAGIC].is_cantrip;
  spell_info[SPELL_DETECT_MAGIC].is_cantrip = true;

  CuAssertTrue(tc, spell_is_cantrip(SPELL_DETECT_MAGIC));
  CuAssertTrue(tc, !spell_is_cantrip(SPELL_RESERVED_DBC));
  CuAssertTrue(tc, !spell_is_cantrip(NUM_SPELLS));

  spell_info[SPELL_DETECT_MAGIC].is_cantrip = saved_cantrip;
}

void Test_sharpened_edge_duration_is_ten_minutes_per_level(CuTest *tc)
{
  struct char_data ch;
  struct affected_type *affect;
  int duration_seconds;

  clear_char(&ch);
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = "sharpened edge test character";
  GET_LEVEL(&ch) = 5;

  mag_affects(GET_LEVEL(&ch), &ch, &ch, NULL, PSIONIC_SHARPENED_EDGE, SAVING_WILL, CAST_INNATE, 0);

  affect = ch.affected;
  CuAssertPtrNotNull(tc, affect);
  if (affect != NULL)
  {
    duration_seconds = affect->duration * PULSE_VIOLENCE / PASSES_PER_SEC;
    CuAssertIntEquals(tc, GET_LEVEL(&ch) * 10 * SECS_PER_REAL_MIN, duration_seconds);
  }

  while (ch.affected != NULL)
    affect_remove_no_total(&ch, ch.affected);
}

void Test_magic_fang_accepts_animal_wild_shapes(CuTest *tc)
{
  struct char_data ch;
  struct char_data *target;
  struct player_special_data player_specials;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;

  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&room, 0, sizeof(room));
  target = &ch;
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &room;
  top_of_world = 0;
  room.people = target;
  ch.player_specials = &player_specials;
  ch.player.name = "animal wild shape test character";
  IN_ROOM(target) = 0;
  GET_LEVEL(target) = 10;
  GET_CLASS(target) = CLASS_DRUID;
  CLASS_LEVEL(target, CLASS_DRUID) = 10;
  SET_BIT_AR(AFF_FLAGS(target), AFF_WILD_SHAPE);
  GET_DISGUISE_RACE(target) = RACE_WOLF;
  IS_MORPHED(target) = RACE_TYPE_ANIMAL;

  CuAssertTrue(tc, IS_WILDSHAPED(target));
  CuAssertTrue(tc, IS_ANIMAL(target));

  mag_affects(GET_LEVEL(target), target, target, NULL, SPELL_MAGIC_FANG, SAVING_WILL, CAST_SPELL,
              0);
  CuAssertTrue(tc, affected_by_spell(target, SPELL_MAGIC_FANG));
  affect_from_char(target, SPELL_MAGIC_FANG);

  mag_affects(GET_LEVEL(target), target, target, NULL, SPELL_GREATER_MAGIC_FANG, SAVING_WILL,
              CAST_SPELL, 0);
  CuAssertTrue(tc, affected_by_spell(target, SPELL_GREATER_MAGIC_FANG));
  affect_from_char(target, SPELL_GREATER_MAGIC_FANG);

  IS_MORPHED(target) = RACE_TYPE_PLANT;
  CuAssertTrue(tc, IS_WILDSHAPED(target));
  CuAssertTrue(tc, !IS_ANIMAL(target));

  mag_affects(GET_LEVEL(target), target, target, NULL, SPELL_MAGIC_FANG, SAVING_WILL, CAST_SPELL,
              0);
  mag_affects(GET_LEVEL(target), target, target, NULL, SPELL_GREATER_MAGIC_FANG, SAVING_WILL,
              CAST_SPELL, 0);
  CuAssertTrue(tc, !affected_by_spell(target, SPELL_MAGIC_FANG));
  CuAssertTrue(tc, !affected_by_spell(target, SPELL_GREATER_MAGIC_FANG));

  room.people = NULL;
  world = saved_world;
  top_of_world = saved_top_of_world;
}

void Test_eidolon_basic_magic_is_at_will(CuTest *tc)
{
  struct char_data eidolon;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  char cast_argument[] = " 'detect magic'";
  bool allowed_without_slots;
  bool applied;
  bool preserved_slots;
  bool denied_without_minor_magic;
  int i;

  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();
  event_init();

  clear_char(&eidolon);
  memset(&room, 0, sizeof(room));
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &room;
  top_of_world = 0;
  room.people = &eidolon;
  eidolon.player_specials = &dummy_mob;
  eidolon.player.short_descr = "an eidolon basic magic test creature";
  IN_ROOM(&eidolon) = 0;
  GET_LEVEL(&eidolon) = 10;
  GET_CLASS(&eidolon) = CLASS_WIZARD;
  GET_POS(&eidolon) = POS_STANDING;
  SET_BIT_AR(MOB_FLAGS(&eidolon), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&eidolon), MOB_EIDOLON);
  HAS_REAL_EVOLUTION(&eidolon, EVOLUTION_BASIC_MAGIC) = 1;

  allowed_without_slots = npc_can_cast(&eidolon, SPELL_DETECT_MAGIC);

  for (i = 0; i < 10; i++)
    eidolon.mob_specials.spell_slots[i] = 2;

  handle_npc_cast(&eidolon, cast_argument, SCMD_CAST_SPELL);
  applied = affected_by_spell(&eidolon, SPELL_DETECT_MAGIC);
  preserved_slots = true;
  for (i = 0; i < 10; i++)
  {
    if (eidolon.mob_specials.spell_slots[i] != 2)
      preserved_slots = false;
  }
  denied_without_minor_magic = !npc_can_cast(&eidolon, SPELL_MAGIC_MISSILE);

  clear_char_event_list(&eidolon);
  while (eidolon.affected != NULL)
    affect_remove_no_total(&eidolon, eidolon.affected);
  room.people = NULL;
  world = saved_world;
  top_of_world = saved_top_of_world;
  event_free_all();

  CuAssertTrue(tc, allowed_without_slots);
  CuAssertTrue(tc, applied);
  CuAssertTrue(tc, preserved_slots);
  CuAssertTrue(tc, denied_without_minor_magic);
}

void Test_aasimar_innate_spells_use_daily_charges(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct room_data room;
  struct list_data list_registry;
  struct list_data *saved_global_lists;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  int cast_result[3];
  int remaining_uses[3];
  int prepared_cast_result;
  int remaining_after_prepared_cast;
  bool fourth_cast_available;
  int i;

  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();
  if (feat_list[FEAT_AASIMAR_HEALING_HANDS].name == NULL)
    assign_feats();
  event_init();

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&room, 0, sizeof(room));
  memset(&list_registry, 0, sizeof(list_registry));
  saved_global_lists = global_lists;
  if (global_lists == NULL)
    global_lists = &list_registry;
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &room;
  top_of_world = 0;
  room.people = &ch;
  ch.player_specials = &player_specials;
  ch.player.name = "aasimar healing hands test character";
  IN_ROOM(&ch) = 0;
  GET_LEVEL(&ch) = 10;
  GET_POS(&ch) = POS_STANDING;
  GET_REAL_MAX_HIT(&ch) = 100;
  GET_MAX_HIT(&ch) = 100;
  GET_HIT(&ch) = 10;
  SET_FEAT(&ch, FEAT_AASIMAR_HEALING_HANDS, 1);
  SET_FEAT(&ch, FEAT_AASIMAR_LIGHT_BEARER, 1);

  CuAssertIntEquals(tc, 3, get_daily_uses(&ch, FEAT_AASIMAR_HEALING_HANDS));
  CuAssertIntEquals(tc, 3, get_daily_uses(&ch, FEAT_AASIMAR_LIGHT_BEARER));
  CuAssertIntEquals(tc, eAASIMAR_HEALING_HANDS, feat_list[FEAT_AASIMAR_HEALING_HANDS].event);
  CuAssertIntEquals(tc, eAASIMAR_LIGHT_BEARER, feat_list[FEAT_AASIMAR_LIGHT_BEARER].event);

  for (i = 0; i < 3; i++)
  {
    CuAssertTrue(tc, isAasimarMagic(&ch, SPELL_REGENERATION));
    cast_result[i] = call_magic(&ch, &ch, NULL, SPELL_REGENERATION, 0, GET_LEVEL(&ch), CAST_SPELL);
    remaining_uses[i] = daily_uses_remaining(&ch, FEAT_AASIMAR_HEALING_HANDS);
  }
  fourth_cast_available = isAasimarMagic(&ch, SPELL_REGENERATION);
  prepared_cast_result =
      call_magic(&ch, &ch, NULL, SPELL_REGENERATION, 0, GET_LEVEL(&ch), CAST_SPELL);
  remaining_after_prepared_cast = daily_uses_remaining(&ch, FEAT_AASIMAR_HEALING_HANDS);

  clear_char_event_list(&ch);
  while (ch.affected != NULL)
    affect_remove_no_total(&ch, ch.affected);
  global_lists = saved_global_lists;
  room.people = NULL;
  world = saved_world;
  top_of_world = saved_top_of_world;
  event_free_all();

  CuAssertIntEquals(tc, 1, cast_result[0]);
  CuAssertIntEquals(tc, 1, cast_result[1]);
  CuAssertIntEquals(tc, 1, cast_result[2]);
  CuAssertIntEquals(tc, 2, remaining_uses[0]);
  CuAssertIntEquals(tc, 1, remaining_uses[1]);
  CuAssertIntEquals(tc, 0, remaining_uses[2]);
  CuAssertTrue(tc, !fourth_cast_available);
  CuAssertIntEquals(tc, 1, prepared_cast_result);
  CuAssertIntEquals(tc, 0, remaining_after_prepared_cast);
}

void Test_group_heal_restores_health_and_cures_blindness(CuTest *tc)
{
  struct char_data ch;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  int starting_hit_points;

  clear_char(&ch);
  memset(&room, 0, sizeof(room));
  saved_world = world;
  saved_top_of_world = top_of_world;

  world = &room;
  top_of_world = 0;
  room.people = &ch;
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = "group heal test character";
  IN_ROOM(&ch) = 0;
  GET_LEVEL(&ch) = 20;
  GET_REAL_MAX_HIT(&ch) = 100;
  GET_MAX_HIT(&ch) = 100;
  GET_HIT(&ch) = 10;
  SET_BIT_AR(AFF_FLAGS(&ch), AFF_BLIND);
  starting_hit_points = GET_HIT(&ch);

  mag_groups(GET_LEVEL(&ch), &ch, NULL, SPELL_GROUP_HEAL, SAVING_WILL, CAST_SPELL);

  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertTrue(tc, GET_HIT(&ch) > starting_hit_points);
  CuAssertTrue(tc, !AFF_FLAGGED(&ch, AFF_BLIND));
}

void Test_positive_channel_energy_heals_grouped_living_targets(CuTest *tc)
{
  struct char_data ch;
  struct char_data ally;
  struct player_special_data ch_specials;
  struct player_special_data ally_specials;
  struct follow_type follower;
  struct group_data group;
  struct list_data list_registry;
  struct list_data *saved_global_lists;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  int ally_starting_hit_points;

  clear_char(&ch);
  clear_char(&ally);
  memset(&ch_specials, 0, sizeof(ch_specials));
  memset(&ally_specials, 0, sizeof(ally_specials));
  memset(&follower, 0, sizeof(follower));
  memset(&group, 0, sizeof(group));
  memset(&list_registry, 0, sizeof(list_registry));
  memset(&room, 0, sizeof(room));
  saved_global_lists = global_lists;
  if (global_lists == NULL)
    global_lists = &list_registry;
  saved_world = world;
  saved_top_of_world = top_of_world;

  ch.player_specials = &ch_specials;
  ally.player_specials = &ally_specials;
  ch.player.name = "positive channel test caster";
  ally.player.name = "positive channel test ally";
  GET_CLASS(&ch) = CLASS_CLERIC;
  GET_LEVEL(&ch) = 4;
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 4;
  IN_ROOM(&ch) = 0;
  IN_ROOM(&ally) = 0;
  GET_REAL_MAX_HIT(&ch) = GET_MAX_HIT(&ch) = 100;
  GET_REAL_MAX_HIT(&ally) = GET_MAX_HIT(&ally) = 100;
  GET_HIT(&ch) = 50;
  GET_HIT(&ally) = 10;
  ally_starting_hit_points = GET_HIT(&ally);

  group.leader = &ch;
  group.members = create_list();
  add_to_list(&ch, group.members);
  add_to_list(&ally, group.members);
  GROUP((&ch)) = &group;
  GROUP((&ally)) = &group;
  follower.follower = &ally;
  ch.followers = &follower;
  ally.master = &ch;

  world = &room;
  top_of_world = 0;
  room.people = &ch;
  ch.next_in_room = &ally;

  CuAssertTrue(tc, is_player_grouped(&ch, &ally));
  mag_groups(GET_LEVEL(&ch), &ch, NULL, ABILITY_CHANNEL_POSITIVE_ENERGY, SAVING_WILL, CAST_INNATE);

  simple_list(NULL);
  free_list(group.members);
  global_lists = saved_global_lists;
  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertTrue(tc, GET_HIT(&ally) > ally_starting_hit_points);
}

void Test_high_circle_swarm_summons_scale_with_caster_level(CuTest *tc)
{
  CuAssertIntEquals(tc, 17, summon_spell_mob_level(SPELL_ELEMENTAL_SWARM, 17));
  CuAssertIntEquals(tc, 20, summon_spell_mob_level(SPELL_ELEMENTAL_SWARM, 30));
  CuAssertIntEquals(tc, 17, summon_spell_mob_level(SPELL_SHAMBLER, 17));
  CuAssertIntEquals(tc, 20, summon_spell_mob_level(SPELL_SHAMBLER, 30));
  CuAssertIntEquals(tc, 0, summon_spell_mob_level(SPELL_HEAL, 30));
}

void Test_ghost_wolf_mobility_applies_to_the_summon(CuTest *tc)
{
  struct char_data low_level_wolf;
  struct char_data water_walking_wolf;
  struct char_data flying_wolf;

  clear_char(&low_level_wolf);
  clear_char(&water_walking_wolf);
  clear_char(&flying_wolf);

  apply_ghost_wolf_mobility(&low_level_wolf, 11);
  apply_ghost_wolf_mobility(&water_walking_wolf, 12);
  apply_ghost_wolf_mobility(&flying_wolf, 15);

  CuAssertTrue(tc, !AFF_FLAGGED(&low_level_wolf, AFF_WATERWALK));
  CuAssertTrue(tc, !AFF_FLAGGED(&low_level_wolf, AFF_FLYING));
  CuAssertTrue(tc, AFF_FLAGGED(&water_walking_wolf, AFF_WATERWALK));
  CuAssertTrue(tc, !AFF_FLAGGED(&water_walking_wolf, AFF_FLYING));
  CuAssertTrue(tc, AFF_FLAGGED(&flying_wolf, AFF_WATERWALK));
  CuAssertTrue(tc, AFF_FLAGGED(&flying_wolf, AFF_FLYING));
}

void Test_domain_command_labels_granted_spell_circles(CuTest *tc)
{
  struct char_data ch;
  struct descriptor_data descriptor;
  struct player_special_data player_specials;
  char expected[128];
  bool found_circle;

  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();
  assign_domains();

  clear_char(&ch);
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  ch.player.name = "domain command test character";
  GET_PAGE_LENGTH(&ch) = PAGE_LENGTH;
  ch.desc = &descriptor;
  descriptor.character = &ch;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize protocol output for the domain command test");
    return;
  }

  do_domain(&ch, "", 0, 0);
  snprintf(expected, sizeof(expected), "1: %s|",
           spell_info[domain_list[DOMAIN_AIR].domain_spells[0]].name);
  found_circle = descriptor.showstr_head != NULL && strstr(descriptor.showstr_head, expected);

  show_string(&descriptor, "q");
  ch.desc = NULL;
  cleanup_test_descriptor(&descriptor);

  CuAssertTrue(tc, found_circle);
}

void Test_internal_affects_have_registered_wearoff_messages(CuTest *tc)
{
  static const int internal_affects[] = {
      RACIAL_LICH_TOUCH,
      AFFECT_BARD_FLOURISH,
      AFFECT_BARD_AGILE_DISENGAGE,
      AFFECT_BARD_PERFECT_TEMPO,
      AFFECT_BARD_SHOWSTOPPER,
      AFFECT_BARD_FEINT_AND_FINISH,
      AFFECT_BARD_SUPREME_STYLE,
      AFFECT_BARD_CURTAIN_CALL,
      AFFECT_BARD_CURTAIN_CALL_DISORIENTED,
      AFFECT_BLACKGUARD_SHAKEN,
      AFFECT_BLACKGUARD_FEAR,
      AFFECT_BLACKGUARD_COWER,
      AFFECT_BLACKGUARD_CRUEL_MOMENTUM,
      AFFECT_BLACKGUARD_PROFANE_WEAPON_BOND,
      AFFECT_BLACKGUARD_BLEEDING,
      AFFECT_BLACKGUARD_UNHOLY_BLITZ,
      AFFECT_BLACKGUARD_AVATAR_OF_PROFANITY,
      AFFECT_BLACKGUARD_CATACLYSMIC_SMITE,
      AFFECT_BLACKGUARD_SHADE_STEP,
      AFFECT_BLACKGUARD_REPRISAL,
      AFFECT_DIVINE_RESILIENCE,
      AFFECT_INQUISITOR_AMBUSH_USED,
      AFFECT_INQUISITOR_DEADLY_AIM,
      AFFECT_BERSERKER_INDOMITABLE_WILL,
      AFFECT_BARD_HEIGHTENED_HARMONY,
      AFFECT_BARD_SYMPHONIC_RESONANCE,
      AFFECT_ALCHEMIST_DISCOVERY_EXTRACTION,
      AFFECT_ALCHEMIST_QUINTESSENTIAL_EXTRACTION,
      AFFECT_BERSERKER_CRIPPLING_BLOW,
      AFFECT_BERSERKER_STUNNING_BLOW,
      AFFECT_BARD_FROSTBITE_REFRAIN_I,
      AFFECT_BARD_FROSTBITE_REFRAIN_II,
      AFFECT_BARD_COMMANDING_CADENCE,
      AFFECT_BARD_COMMANDING_CADENCE_IMMUNITY,
      AFFECT_BARD_WINTERS_WAR_MARCH,
      AFFECT_BARD_WINTERS_WAR_MARCH_IMMUNITY,
      AFFECT_INQUISITOR_PERFECT_ADAPTATION,
      AFFECT_INQUISITOR_SUPREMACY,
      AFFECT_CLERIC_BEACON_OF_HOPE,
      AFFECT_WIZARD_IRRESISTIBLE_MAGIC,
      AFFECT_CLERIC_AVATAR_OF_WAR,
      AFFECT_MONK_AVATAR_OF_ELEMENTS,
      AFFECT_RANGER_NATURES_WRATH,
      AFFECT_PSIONICIST_FOCUS_CHANNELING,
      AFFECT_PSIONICIST_OVERWHELM,
      AFFECT_PSIONICIST_LINKED_MENACE,
      AFFECT_PSIONICIST_PSYCHIC_SUNDERING,
      AFFECT_INTIMIDATING_PRESENCE,
      SKILL_BLEEDING_ATTACK,
      SKILL_CRIPPLING_STRIKE,
      SKILL_PRESSURE_POINT_STRIKE,
      SKILL_FLAMES_OF_PHOENIX,
      SKILL_ETERNAL_MOUNTAIN_DEFENSE,
      SKILL_BREATH_OF_WINTER,
      SKILL_HARDY,
      SKILL_WATER_WHIP,
      SKILL_GONG_OF_SUMMIT,
      SKILL_FIST_OF_UNBROKEN_AIR,
      SKILL_SWEEPING_CINDER_STRIKE,
      SKILL_RUSH_OF_GALE_SPIRITS,
      SKILL_CLENCH_OF_NORTH_WIND,
      SKILL_APPLY_NATURES_WRATH_DAMAGE,
      SPELL_ABSOLUTE_GEAS,
      SPELL_HIVE_COMMANDER_MARK,
      SPELL_ARTIFACT_BONUS,
      SPELL_ARTIFACT_PASSIVE,
      SPELL_ARTIFACT_SURGE,
  };
  const char *wearoff;
  char failure[128];
  size_t affect_count;
  size_t i;
  size_t j;

  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();

  affect_count = sizeof(internal_affects) / sizeof(internal_affects[0]);
  for (i = 0; i < affect_count; i++)
  {
    snprintf(failure, sizeof(failure), "internal affect %d is outside the wear-off table",
             internal_affects[i]);
    CuAssert(tc, failure,
             internal_affects[i] > SPELL_RESERVED_DBC && internal_affects[i] < TOP_SPELL_DEFINE);

    snprintf(failure, sizeof(failure), "internal affect %d is not registered", internal_affects[i]);
    CuAssert(tc, failure, spell_info[internal_affects[i]].name != unused_spellname);

    wearoff = get_wearoff(internal_affects[i]);
    snprintf(failure, sizeof(failure), "internal affect %d has no wear-off message",
             internal_affects[i]);
    CuAssert(tc, failure,
             wearoff != NULL && wearoff[0] != '\0' && strcmp(wearoff, "!UNUSED WEAROFF!") != 0);

    for (j = i + 1; j < affect_count; j++)
    {
      snprintf(failure, sizeof(failure), "internal affects share ID %d", internal_affects[i]);
      CuAssert(tc, failure, internal_affects[i] != internal_affects[j]);
    }
  }
}

void Test_skill_numbered_affect_expiration_dispatches_wearoff(CuTest *tc)
{
  struct affected_type af;
  struct char_data ch;
  struct char_data *saved_character_list;
  struct descriptor_data descriptor;
  bool announced;
  bool remained_for_final_tick;
  bool removed;

  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();

  clear_char(&ch);
  memset(&descriptor, 0, sizeof(descriptor));
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = "wear-off test character";
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  ch.desc = &descriptor;
  descriptor.character = &ch;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize protocol output for the wear-off test");
    return;
  }

  new_affect(&af);
  af.spell = SKILL_BLEEDING_ATTACK;
  af.duration = 1;
  affect_to_char(&ch, &af);

  saved_character_list = character_list;
  ch.next = NULL;
  character_list = &ch;
  affect_update();
  remained_for_final_tick = affected_by_spell(&ch, SKILL_BLEEDING_ATTACK);
  affect_update();
  removed = !affected_by_spell(&ch, SKILL_BLEEDING_ATTACK);
  announced = strstr(descriptor.output, "The bleeding from the attack stops.") != NULL;
  character_list = saved_character_list;

  while (ch.affected != NULL)
    affect_remove_no_total(&ch, ch.affected);
  ch.desc = NULL;
  cleanup_test_descriptor(&descriptor);

  CuAssertTrue(tc, remained_for_final_tick);
  CuAssertTrue(tc, removed);
  CuAssertTrue(tc, announced);
}

void Test_affect_wearoff_callback_can_remove_the_cached_successor(CuTest *tc)
{
  struct affected_type af;
  struct char_data ch;
  struct char_data *saved_character_list;

  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();

  clear_char(&ch);
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = "affect mutation test character";
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  GET_LEVEL(&ch) = 20;
  GET_HIT(&ch) = 1000;
  GET_MAX_HIT(&ch) = 1000;

  new_affect(&af);
  af.spell = AFFECT_BERSERKER_INDOMITABLE_WILL;
  af.duration = -1;
  affect_to_char(&ch, &af);

  new_affect(&af);
  af.spell = SKILL_RAGE;
  af.duration = 0;
  affect_to_char(&ch, &af);

  saved_character_list = character_list;
  ch.next = NULL;
  character_list = &ch;
  affect_update();
  character_list = saved_character_list;

  CuAssertTrue(tc, !affected_by_spell(&ch, SKILL_RAGE));
  CuAssertTrue(tc, !affected_by_spell(&ch, AFFECT_BERSERKER_INDOMITABLE_WILL));

  while (ch.affected != NULL)
    affect_remove_no_total(&ch, ch.affected);
}
