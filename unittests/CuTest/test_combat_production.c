#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/handler.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/character/feats.h"
#include "../../src/character/perks.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/combat/encounters.h"
#include "../../src/combat/fight.h"
#include "../../src/combat/grapple.h"
#include "../../src/lists.h"
#include "../../src/mudlim.h"
#include "../../src/net/protocol.h"

#include <stdlib.h>
#include <string.h>

bool sect_no_weather(struct char_data *ch);

void Test_pvp_policy_blocks_combat_damage_and_player_controlled_pets(CuTest *tc)
{
  struct char_data attacker;
  struct char_data defender;
  struct char_data monster;
  struct char_data attacker_pet;
  struct char_data defender_pet;
  struct player_special_data attacker_specials;
  struct player_special_data defender_specials;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  int saved_pk_allowed;

  clear_char(&attacker);
  clear_char(&defender);
  clear_char(&monster);
  clear_char(&attacker_pet);
  clear_char(&defender_pet);
  memset(&attacker_specials, 0, sizeof(attacker_specials));
  memset(&defender_specials, 0, sizeof(defender_specials));
  memset(&room, 0, sizeof(room));

  attacker.player_specials = &attacker_specials;
  defender.player_specials = &defender_specials;
  attacker.player.name = "Aggressor";
  defender.player.name = "Defender";
  monster.player.short_descr = "a pvp policy monster";
  attacker_pet.player.short_descr = "an attacker pet";
  defender_pet.player.short_descr = "a defender pet";
  SET_BIT_AR(MOB_FLAGS(&monster), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&attacker_pet), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&defender_pet), MOB_ISNPC);
  attacker_pet.master = &attacker;
  defender_pet.master = &defender;
  IN_ROOM(&attacker) = 0;
  IN_ROOM(&defender) = 0;
  IN_ROOM(&monster) = 0;
  IN_ROOM(&attacker_pet) = 0;
  IN_ROOM(&defender_pet) = 0;
  GET_POS(&attacker) = POS_STANDING;
  GET_POS(&defender) = POS_STANDING;
  GET_HIT(&attacker) = GET_MAX_HIT(&attacker) = 100;
  GET_HIT(&defender) = GET_MAX_HIT(&defender) = 100;

  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_pk_allowed = CONFIG_PK_ALLOWED;
  world = &room;
  top_of_world = 0;
  room.number = 100;
  room.light = 1;
  room.people = &attacker;
  attacker.next_in_room = &defender;
  CONFIG_PK_ALLOWED = FALSE;

  CuAssertTrue(tc, !pvp_ok(&attacker, &defender, FALSE));
  CuAssertTrue(tc, pvp_ok(&attacker, &monster, FALSE));
  CuAssertTrue(tc, pvp_ok(&monster, &defender, FALSE));
  CuAssertTrue(tc, !pvp_ok(&attacker_pet, &defender, FALSE));
  CuAssertTrue(tc, !pvp_ok(&attacker, &defender_pet, FALSE));
  CuAssertTrue(tc, !pvp_ok(&attacker_pet, &defender_pet, FALSE));
  CuAssertTrue(tc, pvp_ok(&attacker, &attacker_pet, FALSE));
  CuAssertTrue(tc, !pvp_ok_single(&attacker, FALSE));
  CuAssertTrue(tc, pvp_ok_single(&monster, FALSE));

  monster.master = &monster;
  CuAssertTrue(tc, !pvp_ok(&monster, &defender, FALSE));
  CuAssertTrue(tc, !pvp_ok_single(&monster, FALSE));
  monster.master = NULL;

  do_grapple(&attacker, "Defender", 0, 0);
  CuAssertPtrEquals(tc, NULL, GRAPPLE_TARGET(&attacker));
  CuAssertPtrEquals(tc, NULL, GRAPPLE_ATTACKER(&defender));
  CuAssertTrue(tc, !AFF_FLAGGED(&attacker, AFF_GRAPPLED));
  CuAssertTrue(tc, !AFF_FLAGGED(&defender, AFF_GRAPPLED));
  CuAssertTrue(tc, !set_fighting(&attacker, &defender));
  CuAssertPtrEquals(tc, NULL, FIGHTING(&attacker));
  CuAssertIntEquals(tc, 0, damage(&attacker, &defender, 25, TYPE_UNDEFINED, DAM_SLICE, FALSE));
  CuAssertIntEquals(tc, 100, GET_HIT(&defender));

  SET_BIT_AR(ROOM_FLAGS(0), ROOM_ARENA);
  CuAssertTrue(tc, pvp_ok(&attacker, &defender, FALSE));
  CuAssertTrue(tc, pvp_ok_single(&attacker, FALSE));
  REMOVE_BIT_AR(ROOM_FLAGS(0), ROOM_ARENA);

  CONFIG_PK_ALLOWED = TRUE;
  CuAssertTrue(tc, !pvp_ok(&attacker, &defender, FALSE));
  SET_BIT_AR(PRF_FLAGS(&attacker), PRF_PVP);
  CuAssertTrue(tc, !pvp_ok(&attacker, &defender, FALSE));
  SET_BIT_AR(PRF_FLAGS(&defender), PRF_PVP);
  CuAssertTrue(tc, pvp_ok(&attacker, &defender, FALSE));
  CuAssertTrue(tc, pvp_ok(&attacker_pet, &defender_pet, FALSE));

  CONFIG_PK_ALLOWED = saved_pk_allowed;
  world = saved_world;
  top_of_world = saved_top_of_world;
}

void Test_rol_slow_poison_reduces_target_poison_damage(CuTest *tc)
{
  struct char_data ch;

  clear_char(&ch);
  CuAssertIntEquals(tc, 9, apply_slow_poison_reduction(&ch, 9));

  SET_BIT_AR(AFF2_FLAGS(&ch), AFF2_ROL_SLOW_POISON);
  CuAssertIntEquals(tc, 4, apply_slow_poison_reduction(&ch, 9));
  CuAssertIntEquals(tc, 1, apply_slow_poison_reduction(&ch, 1));
  CuAssertIntEquals(tc, 0, apply_slow_poison_reduction(&ch, 0));
}

void Test_rol_mobile_action_runtime_roles_and_delayed_hunter(CuTest *tc)
{
  struct char_data ch;
  struct char_data *mob;
  struct player_special_data player_specials;

  clear_char(&ch);
  mob = &ch;
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ROL_HAS_PS);
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ROL_HAS_CL);
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ROL_HAS_MU);
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ROL_HAS_TH);
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ROL_HAS_WA);

  CuAssertTrue(tc, IS_PSIONIC(mob));
  CuAssertTrue(tc, IS_CLERIC(mob));
  CuAssertTrue(tc, IS_WIZARD(mob));
  CuAssertTrue(tc, IS_ROGUE(mob));
  CuAssertTrue(tc, IS_WARRIOR(mob));
  CuAssertTrue(tc, IS_NPC_CASTER(mob));

  GET_MAX_HIT(&ch) = 100;
  GET_HIT(&ch) = 90;
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ROL_DELAY_HUNTER);
  CuAssertTrue(tc, !activate_rol_delayed_hunter(&ch, 10));
  GET_HIT(&ch) = 89;
  CuAssertTrue(tc, activate_rol_delayed_hunter(&ch, 1));
  CuAssertTrue(tc, MOB_FLAGGED(&ch, MOB_HUNTER));
  CuAssertTrue(tc, !MOB_FLAGGED(&ch, MOB_ROL_DELAY_HUNTER));
}

void Test_rol_room_flags_drive_arena_and_weather_compatibility(CuTest *tc)
{
  struct char_data ch;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;

  clear_char(&ch);
  memset(&room, 0, sizeof(room));
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &room;
  top_of_world = 0;
  room.number = 2000100;
  room.sector_type = SECT_FIELD;
  IN_ROOM(&ch) = 0;

  CuAssertTrue(tc, !IS_ARENA(IN_ROOM(&ch)));
  CuAssertTrue(tc, !IS_PVP_ARENA(IN_ROOM(&ch)));
  CuAssertTrue(tc, !sect_no_weather(&ch));
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_ARENA);
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_NO_PRECIP);
  CuAssertTrue(tc, IS_ARENA(IN_ROOM(&ch)));
  CuAssertTrue(tc, IS_PVP_ARENA(IN_ROOM(&ch)));
  CuAssertTrue(tc, sect_no_weather(&ch));

  REMOVE_BIT_AR(ROOM_FLAGS(0), ROOM_ARENA);
  room.number = 138650;
  CuAssertTrue(tc, IS_ARENA(IN_ROOM(&ch)));
  CuAssertTrue(tc, !IS_PVP_ARENA(IN_ROOM(&ch)));

  world = saved_world;
  top_of_world = saved_top_of_world;
}

void Test_combat_production_condensed_stats_initialize_and_reset(CuTest *tc)
{
  struct char_data ch;

  memset(&ch, 0, sizeof(ch));
  init_condensed_combat_data(&ch);
  CuAssertPtrNotNull(tc, CNDNSD(&ch));

  CNDNSD(&ch)->damage_inflicted = 42;
  CNDNSD(&ch)->num_times_hit_by_others = 3;
  init_condensed_combat_data(&ch);
  CuAssertIntEquals(tc, 0, CNDNSD(&ch)->damage_inflicted);
  CuAssertIntEquals(tc, 0, CNDNSD(&ch)->num_times_hit_by_others);

  free(CNDNSD(&ch));
  CNDNSD(&ch) = NULL;
}

void Test_combat_production_npc_barehand_damage_dice(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  int dice_count;
  int dice_size;

  memset(&ch, 0, sizeof(ch));
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  ch.mob_specials.damnodice = 3;
  ch.mob_specials.damsizedice = 7;
  dice_count = 0;
  dice_size = 0;

  compute_barehand_dam_dice(&ch, &dice_count, &dice_size);
  CuAssertIntEquals(tc, 3, dice_count);
  CuAssertIntEquals(tc, 7, dice_size);
}

void Test_empty_list_iteration_is_a_clean_noop(CuTest *tc)
{
  struct iterator_data iterator;
  struct list_data list;

  memset(&iterator, 0, sizeof(iterator));
  memset(&list, 0, sizeof(list));

  CuAssertPtrEquals(tc, NULL, merge_iterator(&iterator, &list));
  CuAssertPtrEquals(tc, NULL, iterator.pList);
  CuAssertPtrEquals(tc, NULL, iterator.pItem);
  CuAssertIntEquals(tc, 0, list.iIterators);
}

void Test_combat_perk_group_iterators_detach_on_matches_and_completion(CuTest *tc)
{
  struct char_data ch;
  struct char_data ally;
  struct player_special_data ch_specials;
  struct player_special_data ally_specials;
  struct char_perk_data telepathic_bond;
  struct group_data group;
  struct list_data members;
  struct item_data ch_item;
  struct item_data ally_item;
  struct obj_data shield;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;

  clear_char(&ch);
  clear_char(&ally);
  memset(&ch_specials, 0, sizeof(ch_specials));
  memset(&ally_specials, 0, sizeof(ally_specials));
  memset(&telepathic_bond, 0, sizeof(telepathic_bond));
  memset(&group, 0, sizeof(group));
  memset(&members, 0, sizeof(members));
  memset(&ch_item, 0, sizeof(ch_item));
  memset(&ally_item, 0, sizeof(ally_item));
  memset(&shield, 0, sizeof(shield));
  memset(&room, 0, sizeof(room));

  ch.player_specials = &ch_specials;
  ally.player_specials = &ally_specials;
  ch.player.name = "perk iterator character";
  ally.player.name = "perk iterator ally";
  GET_LEVEL(&ch) = 10;
  GET_LEVEL(&ally) = 10;
  IN_ROOM(&ch) = 0;
  IN_ROOM(&ally) = 0;

  group.leader = &ch;
  group.members = &members;
  GROUP((&ch)) = &group;
  GROUP((&ally)) = &group;
  ch_item.pContent = &ch;
  ch_item.pNextItem = &ally_item;
  ally_item.pContent = &ally;
  ally_item.pPrevItem = &ch_item;
  members.pFirstItem = &ch_item;
  members.pLastItem = &ally_item;
  members.iSize = 2;

  telepathic_bond.perk_id = PERK_INQUISITOR_TELEPATHIC_BOND;
  telepathic_bond.perk_class = CLASS_INQUISITOR;
  telepathic_bond.current_rank = 1;
  ally.player_specials->saved.perks = &telepathic_bond;

  CuAssertIntEquals(tc, 1, get_inquisitor_telepathic_bond_bonus(&ch));
  CuAssertIntEquals(tc, 0, members.iIterators);

  ally.player_specials->saved.perks = NULL;
  GET_LEVEL(&ally) = LVL_STAFF;
  SET_BIT_AR(PRF_FLAGS(&ally), PRF_HOLYLIGHT);
  GET_OBJ_TYPE(&shield) = ITEM_ARMOR;
  GET_EQ(&ally, WEAR_SHIELD) = &shield;

  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &room;
  top_of_world = 0;
  room.people = &ch;
  compute_armor_class(NULL, &ch, FALSE, MODE_ARMOR_CLASS_NORMAL);
  world = saved_world;
  top_of_world = saved_top_of_world;
  CuAssertIntEquals(tc, 0, members.iIterators);

  CuAssertTrue(tc, group_has_paladin_aura_of_protection(&ch));
  CuAssertIntEquals(tc, 0, members.iIterators);

  CuAssertTrue(tc, group_has_paladin_aura_of_life(&ch));
  CuAssertIntEquals(tc, 0, members.iIterators);

  CuAssertPtrEquals(tc, &ally, test_find_divine_sacrifice_defender(&ch));
  CuAssertIntEquals(tc, 0, members.iIterators);

  GET_LEVEL(&ally) = 10;
  REMOVE_BIT_AR(PRF_FLAGS(&ally), PRF_HOLYLIGHT);
  GET_EQ(&ally, WEAR_SHIELD) = NULL;

  test_apply_group_sacred_vengeance(&ch);
  CuAssertIntEquals(tc, 0, members.iIterators);
}

void Test_combat_production_damage_type_validation(CuTest *tc)
{
  CuAssertTrue(tc, ok_damage_handling(TYPE_HIT));
  CuAssertTrue(tc, ok_damage_handling(SPELL_MAGIC_MISSILE));
  CuAssertTrue(tc, !ok_damage_handling(TYPE_SUFFERING));
  CuAssertTrue(tc, !ok_damage_handling(SKILL_BASH));
}

void Test_combat_production_weapon_lookup_excludes_natural_attacks(CuTest *tc)
{
  struct char_data ch;
  struct obj_data primary;
  struct obj_data offhand;
  struct obj_data twohand;
  struct player_special_data player_specials;

  memset(&ch, 0, sizeof(ch));
  memset(&primary, 0, sizeof(primary));
  memset(&offhand, 0, sizeof(offhand));
  memset(&twohand, 0, sizeof(twohand));
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  load_weapons();

  GET_OBJ_TYPE(&primary) = ITEM_WEAPON;
  GET_OBJ_VAL(&primary, 0) = WEAPON_TYPE_LONG_SWORD;
  GET_OBJ_TYPE(&offhand) = ITEM_WEAPON;
  GET_OBJ_VAL(&offhand, 0) = WEAPON_TYPE_BLOWGUN;
  GET_OBJ_TYPE(&twohand) = ITEM_WEAPON;
  GET_OBJ_VAL(&twohand, 0) = WEAPON_TYPE_LONG_BOW;

  GET_EQ(&ch, WEAR_WIELD_1) = &primary;
  GET_EQ(&ch, WEAR_WIELD_OFFHAND) = &offhand;
  GET_EQ(&ch, WEAR_WIELD_2H) = &twohand;

  CuAssertPtrEquals(tc, &primary, test_get_wielded(&ch, ATTACK_TYPE_PRIMARY));
  CuAssertPtrEquals(tc, &offhand, test_get_wielded(&ch, ATTACK_TYPE_OFFHAND));
  CuAssertPtrEquals(tc, &twohand, test_get_wielded(&ch, ATTACK_TYPE_TWOHAND));
  CuAssertPtrEquals(tc, &twohand, test_get_wielded(&ch, ATTACK_TYPE_RANGED));
  CuAssertPtrEquals(tc, NULL, test_get_wielded(&ch, ATTACK_TYPE_UNARMED));
  CuAssertPtrEquals(tc, NULL, test_get_wielded(&ch, ATTACK_TYPE_PRIMARY_EVO_BITE));
  CuAssertPtrEquals(tc, NULL, test_get_wielded(&ch, ATTACK_TYPE_PRIMARY_EVO_CLAWS));
  CuAssertPtrEquals(tc, NULL, test_get_wielded(&ch, ATTACK_TYPE_PSIONICS));
  CuAssertPtrEquals(tc, NULL, test_get_wielded(&ch, ATTACK_TYPE_ELDRITCH_BLAST));
}

void Test_native_javelin_profile_is_melee_usable(CuTest *tc)
{
  struct char_data ch;
  struct obj_data javelin;
  struct player_special_data player_specials;

  memset(&ch, 0, sizeof(ch));
  memset(&javelin, 0, sizeof(javelin));
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  load_weapons();

  GET_OBJ_TYPE(&javelin) = ITEM_WEAPON;
  GET_OBJ_VAL(&javelin, 0) = WEAPON_TYPE_JAVELIN;
  GET_EQ(&ch, WEAR_WIELD_1) = &javelin;

  CuAssertStrEquals(tc, "javelin", weapon_list[WEAPON_TYPE_JAVELIN].name);
  CuAssertTrue(tc, IS_SET(weapon_list[WEAPON_TYPE_JAVELIN].weaponFlags, WEAPON_FLAG_THROWN));
  CuAssertTrue(tc, !IS_SET(weapon_list[WEAPON_TYPE_JAVELIN].weaponFlags, WEAPON_FLAG_RANGED));
  CuAssertIntEquals(tc, 1, weapon_list[WEAPON_TYPE_JAVELIN].numDice);
  CuAssertIntEquals(tc, 6, weapon_list[WEAPON_TYPE_JAVELIN].diceSize);
  CuAssertIntEquals(tc, 2, weapon_list[WEAPON_TYPE_JAVELIN].weight);
  CuAssertIntEquals(tc, 30, weapon_list[WEAPON_TYPE_JAVELIN].range);
  CuAssertPtrEquals(tc, NULL, is_using_ranged_weapon(&ch, TRUE));
}

void Test_psionic_death_effects_bypass_the_generic_damage_cap(CuTest *tc)
{
  struct char_data ch;

  memset(&ch, 0, sizeof(ch));

  CuAssertIntEquals(tc, DAMAGE_CAP, test_cap_combat_damage(&ch, 5000, TYPE_HIT));
  CuAssertIntEquals(tc, 5000, test_cap_combat_damage(&ch, 5000, PSIONIC_DEADLY_FEAR));
  CuAssertIntEquals(tc, 5000, test_cap_combat_damage(&ch, 5000, PSIONIC_PSYCHIC_CRUSH));
  CuAssertIntEquals(tc, 5000, test_cap_combat_damage(&ch, 5000, PSIONIC_RECALL_DEATH));
}

void Test_litany_of_righteousness_dazzles_the_evil_target(CuTest *tc)
{
  struct char_data ch;
  struct char_data victim;
  struct player_special_data ch_player_specials;
  struct player_special_data victim_player_specials;
  struct affected_type litany;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  int damage_dealt;
  bool caster_dazzled;
  bool victim_dazzled;

  clear_char(&ch);
  clear_char(&victim);
  memset(&ch_player_specials, 0, sizeof(ch_player_specials));
  memset(&victim_player_specials, 0, sizeof(victim_player_specials));
  memset(&room, 0, sizeof(room));
  ch.player_specials = &ch_player_specials;
  victim.player_specials = &victim_player_specials;
  ch.player.name = "litany test caster";
  victim.player.name = "litany test target";
  GET_LEVEL(&ch) = 10;
  GET_LEVEL(&victim) = 10;
  GET_ALIGNMENT(&ch) = 1000;
  GET_ALIGNMENT(&victim) = -1000;
  SET_FEAT(&ch, FEAT_AURA_OF_GOOD, 1);
  SET_FEAT(&victim, FEAT_AURA_OF_EVIL, 1);
  SET_BIT_AR(PRF_FLAGS(&ch), PRF_HOLYLIGHT);

  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &room;
  top_of_world = 0;
  room.people = &ch;
  ch.next_in_room = &victim;
  IN_ROOM(&ch) = 0;
  IN_ROOM(&victim) = 0;

  new_affect(&litany);
  litany.spell = SPELL_LITANY_OF_RIGHTEOUSNESS;
  litany.duration = 3;
  affect_to_char(&ch, &litany);

  damage_dealt = test_damage_handling(&ch, &victim, 10, TYPE_HIT, DAM_SLICE);
  caster_dazzled = affected_by_spell(&ch, SPELL_EFFECT_DAZZLED);
  victim_dazzled = affected_by_spell(&victim, SPELL_EFFECT_DAZZLED);

  affect_from_char(&ch, SPELL_LITANY_OF_RIGHTEOUSNESS);
  affect_from_char(&ch, SPELL_EFFECT_DAZZLED);
  affect_from_char(&victim, SPELL_EFFECT_DAZZLED);
  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertIntEquals(tc, 20, damage_dealt);
  CuAssertTrue(tc, !caster_dazzled);
  CuAssertTrue(tc, victim_dazzled);
}

void Test_rogues_are_proficient_with_composite_shortbows(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  CLASS_LEVEL((&ch), CLASS_ROGUE) = 1;

  CuAssertTrue(tc, is_proficient_with_weapon(&ch, WEAPON_TYPE_COMPOSITE_SHORTBOW));
  CuAssertTrue(tc, is_proficient_with_weapon(&ch, WEAPON_TYPE_COMPOSITE_SHORTBOW_2));
  CuAssertTrue(tc, is_proficient_with_weapon(&ch, WEAPON_TYPE_COMPOSITE_SHORTBOW_3));
  CuAssertTrue(tc, is_proficient_with_weapon(&ch, WEAPON_TYPE_COMPOSITE_SHORTBOW_4));
  CuAssertTrue(tc, is_proficient_with_weapon(&ch, WEAPON_TYPE_COMPOSITE_SHORTBOW_5));
  CuAssertTrue(tc, !is_proficient_with_weapon(&ch, WEAPON_TYPE_COMPOSITE_LONGBOW));

  CuAssertTrue(tc, test_is_rogue_weapon_proficient(WEAPON_TYPE_COMPOSITE_SHORTBOW));
  CuAssertTrue(tc, test_is_rogue_weapon_proficient(WEAPON_TYPE_COMPOSITE_SHORTBOW_2));
  CuAssertTrue(tc, test_is_rogue_weapon_proficient(WEAPON_TYPE_COMPOSITE_SHORTBOW_3));
  CuAssertTrue(tc, test_is_rogue_weapon_proficient(WEAPON_TYPE_COMPOSITE_SHORTBOW_4));
  CuAssertTrue(tc, test_is_rogue_weapon_proficient(WEAPON_TYPE_COMPOSITE_SHORTBOW_5));
  CuAssertTrue(tc, !test_is_rogue_weapon_proficient(WEAPON_TYPE_COMPOSITE_LONGBOW));
}

void Test_capped_kill_experience_does_not_report_zero_award(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct descriptor_data descriptor;
  int normal_gain;
  int capped_gain;
  bool normal_award_reported;
  bool cap_reported;
  bool zero_award_reported;
  int saved_max_exp_gain;
  int saved_experience_multiplier;

  memset(&ch, 0, sizeof(ch));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &ch;
  descriptor.pProtocol = ProtocolCreate();
  ch.desc = &descriptor;
  ch.player_specials = &player_specials;
  ch.player.name = "kill experience test character";
  IN_ROOM(&ch) = NOWHERE;
  GET_CLASS(&ch) = CLASS_WARRIOR;
  GET_LEVEL(&ch) = 12;
  ch.player_specials->saved.stage_info.current_stage = 1;

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize the kill experience fixture");
    return;
  }

  saved_max_exp_gain = CONFIG_MAX_EXP_GAIN;
  saved_experience_multiplier = CONFIG_EXPERIENCE_MULTIPLIER;
  CONFIG_MAX_EXP_GAIN = 100000;
  CONFIG_EXPERIENCE_MULTIPLIER = 100;
  GET_EXP(&ch) = level_exp(&ch, GET_LEVEL(&ch));

  normal_gain = test_award_kill_experience(&ch, 100, GAIN_EXP_MODE_SOLO);
  normal_award_reported = strstr(descriptor.output, "You receive 100 experience points.") != NULL;

  descriptor.small_outbuf[0] = '\0';
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufptr = 0;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  GET_EXP(&ch) = level_exp(&ch, GET_LEVEL(&ch) + 2) + 1;

  capped_gain = test_award_kill_experience(&ch, 100, GAIN_EXP_MODE_SOLO);
  cap_reported = strstr(descriptor.output, "Your experience has been capped.") != NULL;
  zero_award_reported = strstr(descriptor.output, "You receive 0 experience points.") != NULL;

  ch.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  CONFIG_MAX_EXP_GAIN = saved_max_exp_gain;
  CONFIG_EXPERIENCE_MULTIPLIER = saved_experience_multiplier;

  CuAssertIntEquals(tc, 100, normal_gain);
  CuAssertTrue(tc, normal_award_reported);
  CuAssertIntEquals(tc, 0, capped_gain);
  CuAssertTrue(tc, cap_reported);
  CuAssertTrue(tc, !zero_award_reported);
}

void Test_random_encounters_respect_peaceful_rooms(CuTest *tc)
{
  struct room_data room;

  memset(&room, 0, sizeof(room));

  CuAssertTrue(tc, random_encounter_allowed_in_room(&room));
  SET_BIT_AR(room.room_flags, ROOM_PEACEFUL);
  CuAssertTrue(tc, !random_encounter_allowed_in_room(&room));
  CuAssertTrue(tc, !random_encounter_allowed_in_room(NULL));
}

void Test_lich_touch_self_heal_ignores_single_file_reach(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  bool succeeded;

  memset(&ch, 0, sizeof(ch));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&room, 0, sizeof(room));
  saved_world = world;
  saved_top_of_world = top_of_world;

  world = &room;
  top_of_world = 0;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_SINGLEFILE);
  room.people = &ch;
  ch.player_specials = &player_specials;
  ch.player.name = "lich touch test character";
  IN_ROOM(&ch) = 0;
  GET_REAL_RACE(&ch) = RACE_LICH;
  GET_LEVEL(&ch) = 30;
  GET_REAL_INT(&ch) = 12;
  GET_REAL_MAX_HIT(&ch) = 100;
  GET_MAX_HIT(&ch) = 100;
  GET_HIT(&ch) = 10;

  succeeded = perform_lichtouch(&ch, &ch);

  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertTrue(tc, succeeded);
  CuAssertTrue(tc, GET_HIT(&ch) > 10);
}

void Test_combat_reaction_guards_block_recursive_life_shield_damage(CuTest *tc)
{
  struct char_data attacker;
  struct char_data victim;
  struct affected_type life_shield;
  struct player_special_data attacker_specials;
  struct player_special_data victim_specials;

  clear_char(&attacker);
  clear_char(&victim);
  memset(&attacker_specials, 0, sizeof(attacker_specials));
  memset(&victim_specials, 0, sizeof(victim_specials));
  attacker.player_specials = &attacker_specials;
  victim.player_specials = &victim_specials;
  new_affect(&life_shield);
  life_shield.spell = SPELL_LIFE_SHIELD;
  victim.affected = &life_shield;
  SET_BIT_AR(MOB_FLAGS(&attacker), MOB_ISNPC);
  attacker.player.race = RACE_TYPE_UNDEAD;

  CuAssertTrue(tc, test_life_shield_can_reflect(&attacker, &victim, 10, TYPE_HIT));
  CuAssertTrue(tc, !test_life_shield_can_reflect(&attacker, &victim, 0, TYPE_HIT));
  CuAssertTrue(tc,
               !test_life_shield_can_reflect(&attacker, &victim, 10, SPELL_LIFE_SHIELD));
}

void Test_combat_spell_affect_lookup_uses_spell_identity(CuTest *tc)
{
  struct char_data victim;
  struct affected_type ordinary;
  struct affected_type greater;

  clear_char(&victim);
  new_affect(&ordinary);
  new_affect(&greater);
  ordinary.spell = SPELL_HOSTILE_JUXTAPOSITION;
  ordinary.location = APPLY_NONE;
  ordinary.next = &greater;
  greater.spell = SPELL_GREATER_HOSTILE_JUXTAPOSITION;
  greater.location = APPLY_NONE;
  victim.affected = &ordinary;

  CuAssertPtrEquals(tc, &greater,
                    test_find_spell_affect(&victim, SPELL_GREATER_HOSTILE_JUXTAPOSITION));
  CuAssertPtrEquals(tc, NULL, test_find_spell_affect(&victim, SPELL_MAGIC_MISSILE));
}
