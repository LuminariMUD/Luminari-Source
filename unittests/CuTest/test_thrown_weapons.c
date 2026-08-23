#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/character/feats.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/combat/projectiles.h"
#include "../../src/combat/spec_abilities.h"

#include <string.h>

static void initialize_test_character(struct char_data *ch,
                                      struct player_special_data *player_specials)
{
  clear_char(ch);
  memset(player_specials, 0, sizeof(*player_specials));
  ch->player_specials = player_specials;
  GET_IDNUM(ch) = 1001;
}

static void initialize_test_object(struct obj_data *obj, int item_type, int weapon_type)
{
  clear_object(obj);
  GET_OBJ_TYPE(obj) = item_type;
  GET_OBJ_VAL(obj, 0) = weapon_type;
  GET_OBJ_BOUND_ID(obj) = NOBODY;
}

void Test_thrown_weapon_ids_are_append_only(CuTest *tc)
{
  const int established_weapon_ids[] = {
      WEAPON_TYPE_UNDEFINED,
      WEAPON_TYPE_UNARMED,
      WEAPON_TYPE_DAGGER,
      WEAPON_TYPE_LIGHT_MACE,
      WEAPON_TYPE_SICKLE,
      WEAPON_TYPE_CLUB,
      WEAPON_TYPE_HEAVY_MACE,
      WEAPON_TYPE_MORNINGSTAR,
      WEAPON_TYPE_SHORTSPEAR,
      WEAPON_TYPE_LONGSPEAR,
      WEAPON_TYPE_QUARTERSTAFF,
      WEAPON_TYPE_SPEAR,
      WEAPON_TYPE_HEAVY_CROSSBOW,
      WEAPON_TYPE_LIGHT_CROSSBOW,
      WEAPON_TYPE_DART,
      WEAPON_TYPE_JAVELIN,
      WEAPON_TYPE_SLING,
      WEAPON_TYPE_THROWING_AXE,
      WEAPON_TYPE_LIGHT_HAMMER,
      WEAPON_TYPE_HAND_AXE,
      WEAPON_TYPE_KUKRI,
      WEAPON_TYPE_LIGHT_PICK,
      WEAPON_TYPE_SAP,
      WEAPON_TYPE_SHORT_SWORD,
      WEAPON_TYPE_BATTLE_AXE,
      WEAPON_TYPE_FLAIL,
      WEAPON_TYPE_LONG_SWORD,
      WEAPON_TYPE_HEAVY_PICK,
      WEAPON_TYPE_RAPIER,
      WEAPON_TYPE_SCIMITAR,
      WEAPON_TYPE_TRIDENT,
      WEAPON_TYPE_WARHAMMER,
      WEAPON_TYPE_FALCHION,
      WEAPON_TYPE_GLAIVE,
      WEAPON_TYPE_GREAT_AXE,
      WEAPON_TYPE_GREAT_CLUB,
      WEAPON_TYPE_HEAVY_FLAIL,
      WEAPON_TYPE_GREAT_SWORD,
      WEAPON_TYPE_GUISARME,
      WEAPON_TYPE_HALBERD,
      WEAPON_TYPE_LANCE,
      WEAPON_TYPE_RANSEUR,
      WEAPON_TYPE_SCYTHE,
      WEAPON_TYPE_LONG_BOW,
      WEAPON_TYPE_SHORT_BOW,
      WEAPON_TYPE_COMPOSITE_LONGBOW,
      WEAPON_TYPE_COMPOSITE_SHORTBOW,
      WEAPON_TYPE_KAMA,
      WEAPON_TYPE_NUNCHAKU,
      WEAPON_TYPE_SAI,
      WEAPON_TYPE_SIANGHAM,
      WEAPON_TYPE_BASTARD_SWORD,
      WEAPON_TYPE_DWARVEN_WAR_AXE,
      WEAPON_TYPE_WHIP,
      WEAPON_TYPE_SPIKED_CHAIN,
      WEAPON_TYPE_DOUBLE_AXE,
      WEAPON_TYPE_DIRE_FLAIL,
      WEAPON_TYPE_HOOKED_HAMMER,
      WEAPON_TYPE_2_BLADED_SWORD,
      WEAPON_TYPE_DWARVEN_URGOSH,
      WEAPON_TYPE_HAND_CROSSBOW,
      WEAPON_TYPE_HEAVY_REP_XBOW,
      WEAPON_TYPE_LIGHT_REP_XBOW,
      WEAPON_TYPE_BOLA,
      WEAPON_TYPE_NET,
      WEAPON_TYPE_SHURIKEN,
      WEAPON_TYPE_COMPOSITE_LONGBOW_2,
      WEAPON_TYPE_COMPOSITE_LONGBOW_3,
      WEAPON_TYPE_COMPOSITE_LONGBOW_4,
      WEAPON_TYPE_COMPOSITE_LONGBOW_5,
      WEAPON_TYPE_COMPOSITE_SHORTBOW_2,
      WEAPON_TYPE_COMPOSITE_SHORTBOW_3,
      WEAPON_TYPE_COMPOSITE_SHORTBOW_4,
      WEAPON_TYPE_COMPOSITE_SHORTBOW_5,
      WEAPON_TYPE_WARMAUL,
      WEAPON_TYPE_KHOPESH,
      WEAPON_TYPE_KNIFE,
      WEAPON_TYPE_HOOPAK,
      WEAPON_TYPE_FOOTMANS_LANCE,
      WEAPON_TYPE_ATHAME,
  };
  size_t i;

  CuAssertIntEquals(tc, 80,
                    (int)(sizeof(established_weapon_ids) / sizeof(established_weapon_ids[0])));
  for (i = 0; i < sizeof(established_weapon_ids) / sizeof(established_weapon_ids[0]); i++)
    CuAssertIntEquals(tc, (int)i, established_weapon_ids[i]);

  CuAssertIntEquals(tc, 80, WEAPON_TYPE_BLOWGUN);
  CuAssertIntEquals(tc, 81, NUM_WEAPON_TYPES);
  CuAssertIntEquals(tc, 22, ATTACK_TYPE_THROWN);
}

void Test_dart_and_blowgun_profiles_are_distinct(CuTest *tc)
{
  struct char_data ch;
  struct obj_data dart;
  struct obj_data blowgun;
  struct player_special_data player_specials;

  initialize_test_character(&ch, &player_specials);
  initialize_test_object(&dart, ITEM_WEAPON, WEAPON_TYPE_DART);
  initialize_test_object(&blowgun, ITEM_WEAPON, WEAPON_TYPE_BLOWGUN);
  load_weapons();

  CuAssertTrue(tc, IS_SET(weapon_list[WEAPON_TYPE_DART].weaponFlags, WEAPON_FLAG_THROWN));
  CuAssertTrue(tc, !IS_SET(weapon_list[WEAPON_TYPE_DART].weaponFlags, WEAPON_FLAG_RANGED));
  CuAssertIntEquals(tc, 1, weapon_list[WEAPON_TYPE_DART].numDice);
  CuAssertIntEquals(tc, 4, weapon_list[WEAPON_TYPE_DART].diceSize);
  CuAssertIntEquals(tc, 20, weapon_list[WEAPON_TYPE_DART].range);

  CuAssertStrEquals(tc, "blowgun", weapon_list[WEAPON_TYPE_BLOWGUN].name);
  CuAssertTrue(tc, IS_SET(weapon_list[WEAPON_TYPE_BLOWGUN].weaponFlags, WEAPON_FLAG_RANGED));
  CuAssertTrue(tc, !IS_SET(weapon_list[WEAPON_TYPE_BLOWGUN].weaponFlags, WEAPON_FLAG_THROWN));
  CuAssertIntEquals(tc, 1, weapon_list[WEAPON_TYPE_BLOWGUN].numDice);
  CuAssertIntEquals(tc, 2, weapon_list[WEAPON_TYPE_BLOWGUN].diceSize);
  CuAssertIntEquals(tc, 20, weapon_list[WEAPON_TYPE_BLOWGUN].range);

  GET_EQ(&ch, WEAR_WIELD_1) = &dart;
  CuAssertPtrEquals(tc, NULL, is_using_ranged_weapon(&ch, TRUE));
  GET_EQ(&ch, WEAR_WIELD_1) = &blowgun;
  CuAssertPtrEquals(tc, &blowgun, is_using_ranged_weapon(&ch, TRUE));
}

void Test_throwable_weapon_classification_enforces_instance_rules(CuTest *tc)
{
  struct char_data ch;
  struct obj_data obj;
  struct obj_special_ability special_ability;
  struct player_special_data player_specials;

  initialize_test_character(&ch, &player_specials);
  initialize_test_object(&obj, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  memset(&special_ability, 0, sizeof(special_ability));
  load_weapons();

  CuAssertTrue(tc, is_throwable_weapon(&ch, &obj));

  GET_OBJ_TYPE(&obj) = ITEM_ARMOR;
  CuAssertTrue(tc, !is_throwable_weapon(&ch, &obj));
  GET_OBJ_TYPE(&obj) = ITEM_WEAPON;

  GET_OBJ_VAL(&obj, 0) = NUM_WEAPON_TYPES;
  CuAssertTrue(tc, !is_throwable_weapon(&ch, &obj));

  GET_OBJ_VAL(&obj, 0) = WEAPON_TYPE_LONG_SWORD;
  CuAssertTrue(tc, !is_throwable_weapon(&ch, &obj));
  special_ability.ability = WEAPON_SPECAB_RETURNING;
  obj.special_abilities = &special_ability;
  CuAssertTrue(tc, !is_throwable_weapon(&ch, &obj));
  special_ability.ability = WEAPON_SPECAB_THROWING;
  CuAssertTrue(tc, is_throwable_weapon(&ch, &obj));

  SET_BIT_AR(GET_OBJ_EXTRA(&obj), ITEM_NODROP);
  CuAssertTrue(tc, !is_throwable_weapon(&ch, &obj));
  REMOVE_BIT_AR(GET_OBJ_EXTRA(&obj), ITEM_NODROP);

  GET_OBJ_BOUND_ID(&obj) = GET_IDNUM(&ch) + 1;
  CuAssertTrue(tc, !is_throwable_weapon(&ch, &obj));
  GET_OBJ_BOUND_ID(&obj) = GET_IDNUM(&ch);
  CuAssertTrue(tc, is_throwable_weapon(&ch, &obj));

  obj.special_abilities = NULL;
  GET_OBJ_VAL(&obj, 0) = WEAPON_TYPE_LONG_BOW;
  GET_OBJ_BOUND_ID(&obj) = NOBODY;
  CuAssertTrue(tc, !is_throwable_weapon(&ch, &obj));
  CuAssertTrue(tc, is_launcher_weapon(&obj));
  CuAssertTrue(tc, !is_throwable_weapon(NULL, &obj));
  CuAssertTrue(tc, !is_throwable_weapon(&ch, NULL));
}

void Test_projectile_attack_predicates_are_semantic(CuTest *tc)
{
  CuAssertTrue(tc, is_launcher_attack(ATTACK_TYPE_RANGED));
  CuAssertTrue(tc, !is_launcher_attack(ATTACK_TYPE_THROWN));
  CuAssertTrue(tc, is_thrown_attack(ATTACK_TYPE_THROWN));
  CuAssertTrue(tc, !is_thrown_attack(ATTACK_TYPE_RANGED));
  CuAssertTrue(tc, is_ranged_weapon_attack(ATTACK_TYPE_RANGED));
  CuAssertTrue(tc, is_ranged_weapon_attack(ATTACK_TYPE_THROWN));
  CuAssertTrue(tc, !is_ranged_weapon_attack(ATTACK_TYPE_PRIMARY));
  CuAssertTrue(tc, has_physical_projectile(ATTACK_TYPE_RANGED));
  CuAssertTrue(tc, has_physical_projectile(ATTACK_TYPE_THROWN));
  CuAssertTrue(tc, !has_physical_projectile(ATTACK_TYPE_BOMB_TOSS));
}

void Test_projectile_weapon_slot_priority_skips_ineligible_items(CuTest *tc)
{
  struct char_data ch;
  struct obj_data twohand;
  struct obj_data primary;
  struct obj_data offhand;
  struct player_special_data player_specials;
  int wear_slot;

  initialize_test_character(&ch, &player_specials);
  initialize_test_object(&twohand, ITEM_WEAPON, WEAPON_TYPE_LONG_BOW);
  initialize_test_object(&primary, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&offhand, ITEM_WEAPON, WEAPON_TYPE_DART);
  load_weapons();

  GET_EQ(&ch, WEAR_WIELD_2H) = &twohand;
  GET_EQ(&ch, WEAR_WIELD_1) = &primary;
  GET_EQ(&ch, WEAR_WIELD_OFFHAND) = &offhand;

  CuAssertPtrEquals(tc, &primary, find_equipped_throwable(&ch, &wear_slot));
  CuAssertIntEquals(tc, WEAR_WIELD_1, wear_slot);
  CuAssertPtrEquals(tc, &twohand, find_equipped_launcher(&ch, &wear_slot));
  CuAssertIntEquals(tc, WEAR_WIELD_2H, wear_slot);

  GET_OBJ_VAL(&twohand, 0) = WEAPON_TYPE_THROWING_AXE;
  CuAssertPtrEquals(tc, &twohand, find_equipped_throwable(&ch, &wear_slot));
  CuAssertIntEquals(tc, WEAR_WIELD_2H, wear_slot);

  GET_OBJ_TYPE(&twohand) = ITEM_ARMOR;
  GET_OBJ_VAL(&primary, 0) = WEAPON_TYPE_LONG_SWORD;
  GET_OBJ_VAL(&offhand, 0) = WEAPON_TYPE_BLOWGUN;
  CuAssertPtrEquals(tc, &offhand, find_equipped_launcher(&ch, &wear_slot));
  CuAssertIntEquals(tc, WEAR_WIELD_OFFHAND, wear_slot);
}

void Test_launcher_ammo_selection_scans_mixed_pouch_contents(CuTest *tc)
{
  struct char_data ch;
  struct obj_data launcher;
  struct obj_data pouch;
  struct obj_data thrown_weapon;
  struct obj_data bolt;
  struct obj_data arrow;
  struct obj_data dart;
  struct player_special_data player_specials;

  initialize_test_character(&ch, &player_specials);
  initialize_test_object(&launcher, ITEM_WEAPON, WEAPON_TYPE_LONG_BOW);
  initialize_test_object(&pouch, ITEM_AMMO_POUCH, 10);
  initialize_test_object(&thrown_weapon, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&bolt, ITEM_MISSILE, AMMO_TYPE_BOLT);
  initialize_test_object(&arrow, ITEM_MISSILE, AMMO_TYPE_ARROW);
  initialize_test_object(&dart, ITEM_MISSILE, AMMO_TYPE_DART);
  load_weapons();

  thrown_weapon.next_content = &bolt;
  bolt.next_content = &arrow;
  arrow.next_content = &dart;
  pouch.contains = &thrown_weapon;
  GET_EQ(&ch, WEAR_WIELD_1) = &launcher;
  GET_EQ(&ch, WEAR_AMMO_POUCH) = &pouch;

  CuAssertPtrEquals(tc, &arrow, find_compatible_launcher_ammo(&ch, &launcher));
  CuAssertTrue(tc, has_ammo_in_pouch(&ch, &launcher, TRUE));
  CuAssertTrue(tc, !is_compatible_launcher_ammo(&launcher, &thrown_weapon));

  GET_OBJ_VAL(&launcher, 0) = WEAPON_TYPE_BLOWGUN;
  CuAssertPtrEquals(tc, &dart, find_compatible_launcher_ammo(&ch, &launcher));
  CuAssertTrue(tc, is_compatible_launcher_ammo(&launcher, &dart));
  CuAssertTrue(tc, !is_compatible_launcher_ammo(&launcher, &arrow));
}

void Test_ammo_pouch_admission_accepts_only_physical_projectiles(CuTest *tc)
{
  struct char_data ch;
  struct obj_data missile;
  struct obj_data throwable;
  struct obj_data melee;
  struct player_special_data player_specials;

  initialize_test_character(&ch, &player_specials);
  initialize_test_object(&missile, ITEM_MISSILE, AMMO_TYPE_ARROW);
  initialize_test_object(&throwable, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&melee, ITEM_WEAPON, WEAPON_TYPE_LONG_SWORD);
  load_weapons();

  CuAssertTrue(tc, can_store_projectile_in_ammo_pouch(&ch, &missile));
  CuAssertTrue(tc, can_store_projectile_in_ammo_pouch(&ch, &throwable));
  CuAssertTrue(tc, !can_store_projectile_in_ammo_pouch(&ch, &melee));

  SET_BIT_AR(GET_OBJ_EXTRA(&throwable), ITEM_NODROP);
  CuAssertTrue(tc, !can_store_projectile_in_ammo_pouch(&ch, &throwable));
}

void Test_projectile_mode_transitions_clear_anchor_state(CuTest *tc)
{
  struct char_data ch;
  struct obj_data anchor;
  struct player_special_data player_specials;

  initialize_test_character(&ch, &player_specials);
  initialize_test_object(&anchor, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  load_weapons();

  CuAssertIntEquals(tc, PROJECTILE_MODE_NONE, PROJECTILE_MODE(&ch));
  CuAssertIntEquals(tc, NOTHING, THROWN_ANCHOR_VNUM(&ch));
  CuAssertIntEquals(tc, -1, THROWN_ANCHOR_WEAR_SLOT(&ch));

  GET_EQ(&ch, WEAR_WIELD_1) = &anchor;
  CuAssertTrue(tc, set_thrown_projectile_mode(&ch, NOTHING, WEAR_WIELD_1));
  CuAssertIntEquals(tc, PROJECTILE_MODE_THROWN, PROJECTILE_MODE(&ch));
  CuAssertIntEquals(tc, NOTHING, THROWN_ANCHOR_VNUM(&ch));
  CuAssertIntEquals(tc, WEAR_WIELD_1, THROWN_ANCHOR_WEAR_SLOT(&ch));

  set_launcher_projectile_mode(&ch);
  CuAssertIntEquals(tc, PROJECTILE_MODE_LAUNCHER, PROJECTILE_MODE(&ch));
  CuAssertIntEquals(tc, NOTHING, THROWN_ANCHOR_VNUM(&ch));
  CuAssertIntEquals(tc, -1, THROWN_ANCHOR_WEAR_SLOT(&ch));

  clear_projectile_mode(&ch);
  CuAssertIntEquals(tc, PROJECTILE_MODE_NONE, PROJECTILE_MODE(&ch));
  CuAssertIntEquals(tc, NOTHING, THROWN_ANCHOR_VNUM(&ch));
  CuAssertIntEquals(tc, -1, THROWN_ANCHOR_WEAR_SLOT(&ch));

  CuAssertTrue(tc, !set_thrown_projectile_mode(&ch, NOTHING, WEAR_HEAD));
}

void Test_thrown_projectile_selection_uses_exact_instances_and_source_order(CuTest *tc)
{
  struct char_data ch;
  struct obj_data anchor;
  struct obj_data pouch;
  struct obj_data pouch_blocked;
  struct obj_data pouch_different;
  struct obj_data pouch_first;
  struct obj_data pouch_second;
  struct obj_data nested;
  struct obj_data wrong_type;
  struct obj_data inventory_first;
  struct obj_data inventory_second;
  struct obj_data ordinary_container;
  struct index_data object_indexes[2];
  struct index_data *saved_obj_index;
  struct player_special_data player_specials;
  struct projectile_attack_context context;
  struct obj_data *selected[6];
  enum projectile_source sources[6];
  obj_rnum saved_top_of_objt;
  bool invalid_anchor_rejected;
  size_t i;

  initialize_test_character(&ch, &player_specials);
  initialize_test_object(&anchor, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&pouch, ITEM_AMMO_POUCH, 20);
  initialize_test_object(&pouch_blocked, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&pouch_different, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&pouch_first, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&pouch_second, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&nested, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&wrong_type, ITEM_ARMOR, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&inventory_first, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&inventory_second, ITEM_WEAPON, WEAPON_TYPE_JAVELIN);
  initialize_test_object(&ordinary_container, ITEM_CONTAINER, 10);
  memset(object_indexes, 0, sizeof(object_indexes));
  load_weapons();

  object_indexes[0].vnum = 8100;
  object_indexes[1].vnum = 8101;
  GET_OBJ_RNUM(&anchor) = 0;
  GET_OBJ_RNUM(&pouch_blocked) = 0;
  GET_OBJ_RNUM(&pouch_different) = 1;
  GET_OBJ_RNUM(&pouch_first) = 0;
  GET_OBJ_RNUM(&pouch_second) = 0;
  GET_OBJ_RNUM(&nested) = 0;
  GET_OBJ_RNUM(&wrong_type) = 0;
  GET_OBJ_RNUM(&inventory_first) = 0;
  GET_OBJ_RNUM(&inventory_second) = 0;
  SET_BIT_AR(GET_OBJ_EXTRA(&pouch_blocked), ITEM_NODROP);

  pouch_blocked.next_content = &pouch_different;
  pouch_different.next_content = &pouch_first;
  pouch_first.next_content = &pouch_second;
  pouch.contains = &pouch_blocked;

  nested.in_obj = &ordinary_container;
  nested.next_content = &wrong_type;
  wrong_type.next_content = &inventory_first;
  inventory_first.next_content = &inventory_second;
  ch.carrying = &nested;
  GET_EQ(&ch, WEAR_AMMO_POUCH) = &pouch;
  GET_EQ(&ch, WEAR_WIELD_1) = &anchor;

  saved_obj_index = obj_index;
  saved_top_of_objt = top_of_objt;
  obj_index = object_indexes;
  top_of_objt = 1;

  select_thrown_projectile(&ch, 8100, WEAR_WIELD_1, &context);
  selected[0] = context.physical_projectile;
  sources[0] = context.original_source;

  pouch_different.next_content = &pouch_second;
  select_thrown_projectile(&ch, 8100, WEAR_WIELD_1, &context);
  selected[1] = context.physical_projectile;
  sources[1] = context.original_source;

  pouch.contains = NULL;
  select_thrown_projectile(&ch, 8100, WEAR_WIELD_1, &context);
  selected[2] = context.physical_projectile;
  sources[2] = context.original_source;

  ch.carrying = &inventory_second;
  select_thrown_projectile(&ch, 8100, WEAR_WIELD_1, &context);
  selected[3] = context.physical_projectile;
  sources[3] = context.original_source;

  ch.carrying = NULL;
  select_thrown_projectile(&ch, 8100, WEAR_WIELD_1, &context);
  selected[4] = context.physical_projectile;
  sources[4] = context.original_source;

  GET_OBJ_RNUM(&anchor) = NOTHING;
  GET_OBJ_RNUM(&inventory_second) = NOTHING;
  ch.carrying = &inventory_second;
  select_thrown_projectile(&ch, NOTHING, WEAR_WIELD_1, &context);
  selected[5] = context.physical_projectile;
  sources[5] = context.original_source;

  GET_OBJ_VAL(&anchor, 0) = WEAPON_TYPE_LONG_SWORD;
  invalid_anchor_rejected = !select_thrown_projectile(&ch, NOTHING, WEAR_WIELD_1, &context);

  obj_index = saved_obj_index;
  top_of_objt = saved_top_of_objt;

  CuAssertPtrEquals(tc, &pouch_first, selected[0]);
  CuAssertIntEquals(tc, PROJECTILE_SOURCE_AMMO_POUCH, sources[0]);
  CuAssertPtrEquals(tc, &pouch_second, selected[1]);
  CuAssertIntEquals(tc, PROJECTILE_SOURCE_AMMO_POUCH, sources[1]);
  CuAssertPtrEquals(tc, &inventory_first, selected[2]);
  CuAssertIntEquals(tc, PROJECTILE_SOURCE_INVENTORY, sources[2]);
  CuAssertPtrEquals(tc, &inventory_second, selected[3]);
  CuAssertIntEquals(tc, PROJECTILE_SOURCE_INVENTORY, sources[3]);
  CuAssertPtrEquals(tc, &anchor, selected[4]);
  CuAssertIntEquals(tc, PROJECTILE_SOURCE_WIELDED, sources[4]);
  CuAssertPtrEquals(tc, &anchor, selected[5]);
  CuAssertIntEquals(tc, PROJECTILE_SOURCE_WIELDED, sources[5]);
  CuAssertTrue(tc, invalid_anchor_rejected);

  for (i = 0; i < sizeof(selected) / sizeof(selected[0]); i++)
    CuAssertPtrNotNull(tc, selected[i]);
}
