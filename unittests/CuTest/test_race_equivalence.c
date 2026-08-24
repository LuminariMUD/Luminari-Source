#include "CuTest.h"

#include <string.h>

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/character/feats.h"
#include "../../src/character/premadebuilds.h"
#include "../../src/character/race.h"
#include "../../src/net/protocol.h"

static void ensure_race_equivalence_registry(void)
{
  if (race_list[RACE_WEMIC].type == NULL)
    assign_races();
  if (feat_list[FEAT_ARMOR_SKIN].name == NULL)
    assign_feats();
}

static void init_race_equivalence_character(struct char_data *ch,
                                            struct player_special_data *specials,
                                            struct descriptor_data *descriptor,
                                            struct account_data *account)
{
  memset(ch, 0, sizeof(*ch));
  memset(specials, 0, sizeof(*specials));
  memset(descriptor, 0, sizeof(*descriptor));
  memset(account, 0, sizeof(*account));
  ch->player_specials = specials;
  ch->desc = descriptor;
  descriptor->character = ch;
  descriptor->account = account;
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
  IN_ROOM(ch) = NOWHERE;
}

static int count_racial_feat(int race, int feat)
{
  struct race_feat_assign *assignment = NULL;
  int count = 0;

  for (assignment = race_list[race].featassign_list; assignment != NULL;
       assignment = assignment->next)
    if (assignment->feat_num == feat)
      count++;

  return count;
}

static void cleanup_race_equivalence_descriptor(struct descriptor_data *descriptor)
{
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
  }
}

static void init_race_equipment_object(struct obj_data *obj, const char *name, int wear_flag)
{
  clear_object(obj);
  obj->name = (char *)name;
  obj->short_description = (char *)name;
  obj->description = (char *)name;
  GET_OBJ_TYPE(obj) = ITEM_ARMOR;
  GET_OBJ_SIZE(obj) = SIZE_LARGE;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(obj), wear_flag);
}

void TestRaceEquivalenceIdsAreUniqueAndRepresentable(CuTest *tc)
{
  struct char_data ch;

  memset(&ch, 0, sizeof(ch));
  GET_REAL_RACE(&ch) = RACE_YUAN_TI;

  CuAssertTrue(tc, sizeof(ch.player.race) >= 2);
  CuAssertIntEquals(tc, 28, RACE_HALF_OGRE);
  CuAssertIntEquals(tc, 114, RACE_MYCONID);
  CuAssertIntEquals(tc, 149, RACE_WEMIC);
  CuAssertIntEquals(tc, 150, RACE_HALF_ILLITHID);
  CuAssertIntEquals(tc, 151, RACE_YUAN_TI);
  CuAssertIntEquals(tc, RACE_YUAN_TI, GET_REAL_RACE(&ch));
  CuAssertTrue(tc, RACE_WEMIC != RACE_HALF_ILLITHID);
  CuAssertTrue(tc, RACE_HALF_ILLITHID != RACE_YUAN_TI);
  CuAssertTrue(tc, RACE_YUAN_TI < NUM_EXTENDED_RACES);
}

void TestRaceEquivalenceRegistryMatchesApprovedTiers(CuTest *tc)
{
  const int races[] = {RACE_WEMIC, RACE_HALF_OGRE, RACE_HALF_ILLITHID, RACE_YUAN_TI, RACE_MYCONID};
  const int adjustments[] = {2, 2, 10, 2, 10};
  const int costs[] = {1000, 1000, 30000, 1000, 30000};
  const int tiers[] = {1, 1, 2, 1, 2};
  int count = 0;
  int race = 0;
  size_t i = 0;

  ensure_race_equivalence_registry();

  for (race = 0; race < NUM_EXTENDED_RACES; race++)
    if (race_is_creation_eligible(race))
      count++;
  CuAssertIntEquals(tc, NUM_CREATION_RACES, count);

  for (i = 0; i < sizeof(races) / sizeof(races[0]); i++)
  {
    CuAssertTrue(tc, race_list[races[i]].is_pc);
    CuAssertTrue(tc, race_is_creation_eligible(races[i]));
    CuAssertPtrNotNull(tc, race_list[races[i]].descrip);
    CuAssertTrue(tc, race_list[races[i]].racial_language >= SKILL_LANG_COMMON);
    CuAssertIntEquals(tc, adjustments[i], race_list[races[i]].level_adjustment);
    CuAssertIntEquals(tc, costs[i], race_list[races[i]].unlock_cost);
    CuAssertIntEquals(tc, tiers[i], race_list[races[i]].epic_adv);
    CuAssertTrue(tc, valid_class_race_alignment(CLASS_WARRIOR, races[i]));
  }

  CuAssertTrue(tc, !race_is_creation_eligible(RACE_LICH));
  CuAssertTrue(tc, !race_is_creation_eligible(RACE_VAMPIRE));
}

void TestRaceEquivalenceStatsSizesAndFamilies(CuTest *tc)
{
  const int races[] = {RACE_WEMIC, RACE_HALF_OGRE, RACE_HALF_ILLITHID, RACE_YUAN_TI, RACE_MYCONID};
  const int expected_stats[][6] = {
      {8, 4, -2, 2, 2, -2}, {6, 2, -2, 0, -2, -2},  {0, 0, 4, 4, 0, 4},
      {0, 0, 2, 0, 2, 2},   {8, 6, -2, -2, -4, -4},
  };
  const int expected_sizes[] = {SIZE_LARGE, SIZE_LARGE, SIZE_MEDIUM, SIZE_MEDIUM, SIZE_LARGE};
  const int expected_languages[] = {SKILL_LANG_COMMON, SKILL_LANG_GIANT, SKILL_LANG_ABERRATION,
                                    SKILL_LANG_DRACONIC, SKILL_LANG_UNDERCOMMON};
  struct char_data ch;
  struct char_data *character = &ch;
  struct player_special_data specials;
  size_t i = 0;
  int stat = 0;

  ensure_race_equivalence_registry();
  memset(&ch, 0, sizeof(ch));
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;

  for (i = 0; i < sizeof(races) / sizeof(races[0]); i++)
  {
    for (stat = 0; stat < 6; stat++)
      CuAssertIntEquals(tc, expected_stats[i][stat], get_race_stat(races[i], stat));
    CuAssertIntEquals(tc, expected_sizes[i], race_list[races[i]].size);
    CuAssertIntEquals(tc, expected_languages[i], race_list[races[i]].racial_language);
  }

  GET_REAL_RACE(character) = RACE_WEMIC;
  CuAssertTrue(tc, IS_MONSTROUS_HUMANOID(character));
  CuAssertTrue(tc, is_furry(RACE_WEMIC));
  GET_REAL_RACE(character) = RACE_HALF_OGRE;
  CuAssertTrue(tc, IS_GIANT(character));
  GET_REAL_RACE(character) = RACE_HALF_ILLITHID;
  CuAssertTrue(tc, IS_ABERRATION(character));
  CuAssertTrue(tc, race_has_no_hair(RACE_HALF_ILLITHID));
  GET_REAL_RACE(character) = RACE_YUAN_TI;
  CuAssertTrue(tc, IS_MONSTROUS_HUMANOID(character));
  CuAssertTrue(tc, has_scales(RACE_YUAN_TI));
  CuAssertTrue(tc, race_has_no_hair(RACE_YUAN_TI));
  GET_REAL_RACE(character) = RACE_MYCONID;
  CuAssertTrue(tc, IS_PLANT(character));
  CuAssertTrue(tc, race_has_no_hair(RACE_MYCONID));
  CuAssertTrue(tc, !IS_HUMANOID(character));
}

void TestRaceEquivalenceParsersAndRacialFeats(CuTest *tc)
{
  ensure_race_equivalence_registry();

  CuAssertIntEquals(tc, RACE_WEMIC, parse_race_long("Wemic"));
  CuAssertIntEquals(tc, RACE_WEMIC, parse_race_long("Barbarian"));
  CuAssertIntEquals(tc, RACE_HALF_OGRE, parse_race_long("Half-Ogre"));
  CuAssertIntEquals(tc, RACE_HALF_OGRE, parse_race_long("Ogre"));
  CuAssertIntEquals(tc, RACE_HALF_ILLITHID, parse_race_long("Half-Illithid"));
  CuAssertIntEquals(tc, RACE_HALF_ILLITHID, parse_race_long("Illithid"));
  CuAssertIntEquals(tc, RACE_YUAN_TI, parse_race_long("Yuan-Ti"));
  CuAssertIntEquals(tc, RACE_MYCONID, parse_race_long("Myconid"));
  CuAssertIntEquals(tc, RACE_MYCONID, parse_race_long("Mycanoid"));

  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_WEMIC, FEAT_CLAWS_AND_BITE));
  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_WEMIC, FEAT_LEONINE_FRAME));
  CuAssertIntEquals(tc, 2, count_racial_feat(RACE_HALF_OGRE, FEAT_ARMOR_SKIN));
  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_HALF_ILLITHID, FEAT_SLA_LEVITATE));
  CuAssertIntEquals(tc, 3, count_racial_feat(RACE_HALF_ILLITHID, FEAT_ARMOR_SKIN));
  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_YUAN_TI, FEAT_POISON_BITE));
  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_YUAN_TI, FEAT_POISON_IMMUNITY));
  CuAssertIntEquals(tc, 4, count_racial_feat(RACE_MYCONID, FEAT_ARMOR_SKIN));
  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_MYCONID, FEAT_PARALYSIS_IMMUNITY));
}

void TestRaceEquivalenceCreationUnlockPolicy(CuTest *tc)
{
  const int races[] = {RACE_WEMIC, RACE_HALF_OGRE, RACE_HALF_ILLITHID, RACE_YUAN_TI, RACE_MYCONID};
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  size_t i = 0;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);

  CuAssertTrue(tc, race_is_selectable_for_creation(&ch, RACE_HUMAN));
  for (i = 0; i < sizeof(races) / sizeof(races[0]); i++)
  {
    CuAssertTrue(tc, !race_is_selectable_for_creation(&ch, races[i]));
    account.races[0] = races[i];
    CuAssertTrue(tc, race_is_selectable_for_creation(&ch, races[i]));
    account.races[0] = 0;
  }

  account.races[0] = RACE_LICH;
  account.races[1] = RACE_VAMPIRE;
  CuAssertTrue(tc, !race_is_selectable_for_creation(&ch, RACE_LICH));
  CuAssertTrue(tc, !race_is_selectable_for_creation(&ch, RACE_VAMPIRE));
  CuAssertTrue(tc, !race_is_selectable_for_creation(&ch, -1));
  CuAssertTrue(tc, !race_is_selectable_for_creation(&ch, NUM_EXTENDED_RACES));
}

void TestRaceEquivalenceTerminalCreationPolicy(CuTest *tc)
{
  const int races[] = {RACE_WEMIC, RACE_HALF_OGRE, RACE_HALF_ILLITHID, RACE_YUAN_TI, RACE_MYCONID};
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  char input[MAX_INPUT_LENGTH];
  size_t i = 0;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  if (descriptor.pProtocol == NULL)
    return;

  for (i = 0; i < sizeof(races) / sizeof(races[0]); i++)
    account.races[i] = races[i];
  STATE(&descriptor) = CON_QSEX;
  snprintf(input, sizeof(input), "m");
  nanny(&descriptor, input);
  CuAssertIntEquals(tc, CON_QRACE, STATE(&descriptor));
  for (i = 0; i < sizeof(races) / sizeof(races[0]); i++)
    CuAssertPtrNotNull(tc, strstr(descriptor.output, race_list[races[i]].type));
  CuAssertTrue(tc, strstr(descriptor.output, "\r\nLich\r\n") == NULL);
  CuAssertTrue(tc, strstr(descriptor.output, "\r\nVampire\r\n") == NULL);
  cleanup_race_equivalence_descriptor(&descriptor);

  for (i = 0; i < sizeof(races) / sizeof(races[0]); i++)
  {
    init_race_equivalence_character(&ch, &specials, &descriptor, &account);
    descriptor.pProtocol = ProtocolCreate();
    CuAssertPtrNotNull(tc, descriptor.pProtocol);
    if (descriptor.pProtocol == NULL)
      return;
    account.races[0] = races[i];
    GET_REAL_RACE(&ch) = RACE_UNDEFINED;
    STATE(&descriptor) = CON_QRACE;
    snprintf(input, sizeof(input), "%s", race_list[races[i]].type);
    nanny(&descriptor, input);
    CuAssertIntEquals(tc, races[i], GET_REAL_RACE(&ch));
    CuAssertIntEquals(tc, CON_QRACE_HELP, STATE(&descriptor));
    cleanup_race_equivalence_descriptor(&descriptor);
  }

  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  if (descriptor.pProtocol == NULL)
    return;
  GET_REAL_RACE(&ch) = RACE_UNDEFINED;
  STATE(&descriptor) = CON_QRACE;
  snprintf(input, sizeof(input), "wemic");
  nanny(&descriptor, input);
  CuAssertIntEquals(tc, RACE_UNDEFINED, GET_REAL_RACE(&ch));
  CuAssertIntEquals(tc, CON_QRACE, STATE(&descriptor));
  CuAssertPtrNotNull(tc, strstr(descriptor.output, "not unlocked"));

  account.races[0] = RACE_LICH;
  descriptor.output[0] = '\0';
  descriptor.bufptr = 0;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  snprintf(input, sizeof(input), "lich");
  nanny(&descriptor, input);
  CuAssertIntEquals(tc, RACE_UNDEFINED, GET_REAL_RACE(&ch));
  CuAssertIntEquals(tc, CON_QRACE, STATE(&descriptor));
  CuAssertPtrNotNull(tc, strstr(descriptor.output, "cannot be selected"));
  cleanup_race_equivalence_descriptor(&descriptor);
}

void TestRaceEquivalenceAccountExperiencePurchase(CuTest *tc)
{
  const int races[] = {RACE_WEMIC, RACE_HALF_OGRE, RACE_HALF_ILLITHID, RACE_YUAN_TI, RACE_MYCONID};
  const int costs[] = {1000, 1000, 30000, 1000, 30000};
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  char input[MAX_INPUT_LENGTH];
  size_t i = 0;

  ensure_race_equivalence_registry();
  for (i = 0; i < sizeof(races) / sizeof(races[0]); i++)
  {
    init_race_equivalence_character(&ch, &specials, &descriptor, &account);
    descriptor.pProtocol = ProtocolCreate();
    CuAssertPtrNotNull(tc, descriptor.pProtocol);
    if (descriptor.pProtocol == NULL)
      return;
    account.experience = costs[i];
    snprintf(input, sizeof(input), "race %s", race_list[races[i]].type);
    do_accexp(&ch, input, 0, 0);
    CuAssertIntEquals(tc, races[i], account.races[0]);
    CuAssertIntEquals(tc, 0, account.experience);
    CuAssertTrue(tc, has_unlocked_race(&ch, races[i]));
    CuAssertPtrNotNull(tc, strstr(descriptor.output, "You have unlocked"));
    cleanup_race_equivalence_descriptor(&descriptor);
  }
}

void TestRaceEquivalencePremadeBuildStats(CuTest *tc)
{
  const int races[] = {RACE_WEMIC, RACE_HALF_OGRE, RACE_HALF_ILLITHID, RACE_YUAN_TI, RACE_MYCONID};
  const int warrior_base_stats[] = {16, 16, 14, 10, 14, 8};
  struct char_data ch;
  struct player_special_data specials;
  size_t i = 0;

  ensure_race_equivalence_registry();
  memset(&ch, 0, sizeof(ch));
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;

  for (i = 0; i < sizeof(races) / sizeof(races[0]); i++)
  {
    GET_REAL_RACE(&ch) = races[i];
    set_premade_stats(&ch, CLASS_WARRIOR, 1);
    CuAssertIntEquals(tc, warrior_base_stats[0] + get_race_stat(races[i], R_STR_MOD),
                      GET_REAL_STR(&ch));
    CuAssertIntEquals(tc, warrior_base_stats[1] + get_race_stat(races[i], R_CON_MOD),
                      GET_REAL_CON(&ch));
    CuAssertIntEquals(tc, warrior_base_stats[2] + get_race_stat(races[i], R_INTEL_MOD),
                      GET_REAL_INT(&ch));
    CuAssertIntEquals(tc, warrior_base_stats[3] + get_race_stat(races[i], R_WIS_MOD),
                      GET_REAL_WIS(&ch));
    CuAssertIntEquals(tc, warrior_base_stats[4] + get_race_stat(races[i], R_DEX_MOD),
                      GET_REAL_DEX(&ch));
    CuAssertIntEquals(tc, warrior_base_stats[5] + get_race_stat(races[i], R_CHA_MOD),
                      GET_REAL_CHA(&ch));
  }
}

void TestRaceEquivalenceLevelOneFeatGrants(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  ch.desc = NULL;
  GET_LEVEL(&ch) = 1;

  GET_REAL_RACE(&ch) = RACE_WEMIC;
  process_race_level_feats(&ch);
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_CLAWS_AND_BITE));
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_LEONINE_FRAME));

  memset(ch.char_specials.saved.feats, 0, sizeof(ch.char_specials.saved.feats));
  GET_REAL_RACE(&ch) = RACE_HALF_OGRE;
  process_race_level_feats(&ch);
  CuAssertIntEquals(tc, 2, HAS_REAL_FEAT(&ch, FEAT_ARMOR_SKIN));

  memset(ch.char_specials.saved.feats, 0, sizeof(ch.char_specials.saved.feats));
  GET_REAL_RACE(&ch) = RACE_HALF_ILLITHID;
  process_race_level_feats(&ch);
  CuAssertIntEquals(tc, 3, HAS_REAL_FEAT(&ch, FEAT_ARMOR_SKIN));
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_SLA_LEVITATE));

  memset(ch.char_specials.saved.feats, 0, sizeof(ch.char_specials.saved.feats));
  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  process_race_level_feats(&ch);
  CuAssertIntEquals(tc, 2, HAS_REAL_FEAT(&ch, FEAT_ARMOR_SKIN));
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_POISON_BITE));
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_POISON_IMMUNITY));

  memset(ch.char_specials.saved.feats, 0, sizeof(ch.char_specials.saved.feats));
  GET_REAL_RACE(&ch) = RACE_MYCONID;
  process_race_level_feats(&ch);
  CuAssertIntEquals(tc, 4, HAS_REAL_FEAT(&ch, FEAT_ARMOR_SKIN));
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_POISON_IMMUNITY));
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_SLEEP_ENCHANTMENT_IMMUNITY));
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_PARALYSIS_IMMUNITY));

  GET_LEVEL(&ch) = 2;
  process_race_level_feats(&ch);
  CuAssertIntEquals(tc, 4, HAS_REAL_FEAT(&ch, FEAT_ARMOR_SKIN));
}

void TestRaceAnatomyWearSlotPolicy(CuTest *tc)
{
  const int trelux_restricted_slots[] = {
      WEAR_FINGER_R, WEAR_FINGER_L, WEAR_HANDS,  WEAR_SHIELD,  WEAR_WIELD_1, WEAR_WIELD_OFFHAND,
      WEAR_WIELD_2H, WEAR_HOLD_1,   WEAR_HOLD_2, WEAR_HOLD_2H, WEAR_LEGS,    WEAR_FEET,
  };
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  size_t i;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);

  GET_REAL_RACE(&ch) = RACE_HUMAN;
  for (i = 0; i < NUM_WEARS; i++)
    CuAssertTrue(tc, character_can_use_wear_slot(&ch, (int)i));

  GET_REAL_RACE(&ch) = RACE_WEMIC;
  CuAssertTrue(tc, !character_can_use_wear_slot(&ch, WEAR_LEGS));
  CuAssertTrue(tc, !character_can_use_wear_slot(&ch, WEAR_FEET));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_ANKLE_R));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_HANDS));
  CuAssertPtrNotNull(tc, strstr(character_wear_slot_restriction(&ch, WEAR_LEGS), "leonine"));

  GET_REAL_RACE(&ch) = RACE_TRELUX;
  GET_DISGUISE_RACE(&ch) = RACE_HUMAN;
  for (i = 0; i < sizeof(trelux_restricted_slots) / sizeof(trelux_restricted_slots[0]); i++)
    CuAssertTrue(tc, !character_can_use_wear_slot(&ch, trelux_restricted_slots[i]));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_ARMS));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_WRIST_R));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_ANKLE_R));
  CuAssertTrue(tc, !character_can_use_wear_slot(NULL, WEAR_BODY));
  CuAssertTrue(tc, !character_can_use_wear_slot(&ch, -1));
  CuAssertTrue(tc, !character_can_use_wear_slot(&ch, NUM_WEARS));
}

void TestRaceAnatomyRestrictionsCoverEquipmentEntryPaths(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  struct obj_data pants;
  struct obj_data boots;
  struct obj_data gloves;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  if (descriptor.pProtocol == NULL)
    return;
  GET_REAL_RACE(&ch) = RACE_WEMIC;
  GET_REAL_SIZE(&ch) = SIZE_LARGE;
  ch.points.size = SIZE_LARGE;

  init_race_equipment_object(&pants, "a pair of test pants", ITEM_WEAR_LEGS);
  obj_to_char(&pants, &ch);
  CuAssertIntEquals(tc, WEAR_LEGS, find_eq_pos(&ch, &pants, NULL));
  perform_wear(&ch, &pants, WEAR_LEGS);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&ch, WEAR_LEGS));
  CuAssertPtrEquals(tc, &ch, pants.carried_by);
  CuAssertPtrNotNull(tc, strstr(descriptor.output, "leonine frame"));
  obj_from_char(&pants);

  init_race_equipment_object(&boots, "a pair of test boots", ITEM_WEAR_FEET);
  test_auto_equip_loaded_object(&ch, &boots, WEAR_FEET + 1);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&ch, WEAR_FEET));
  CuAssertPtrEquals(tc, &ch, boots.carried_by);
  obj_from_char(&boots);

  GET_REAL_RACE(&ch) = RACE_TRELUX;
  init_race_equipment_object(&gloves, "a pair of test gloves", ITEM_WEAR_HANDS);
  equip_char(&ch, &gloves, WEAR_HANDS);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&ch, WEAR_HANDS));
  CuAssertPtrEquals(tc, &ch, gloves.carried_by);
  obj_from_char(&gloves);
  cleanup_race_equivalence_descriptor(&descriptor);
}

void TestRaceEquivalenceHitPointPolicyAndDerivedRebuild(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  int human_hp = 0;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  GET_LEVEL(&ch) = 10;
  GET_REAL_CON(&ch) = 10;
  ch.aff_abils.con = 10;

  GET_REAL_RACE(&ch) = RACE_HUMAN;
  calculate_max_hp(&ch, FALSE);
  human_hp = GET_MAX_HIT(&ch);

  GET_REAL_RACE(&ch) = RACE_WEMIC;
  calculate_max_hp(&ch, FALSE);
  CuAssertIntEquals(tc, human_hp + 10, GET_MAX_HIT(&ch));
  GET_REAL_RACE(&ch) = RACE_HALF_OGRE;
  calculate_max_hp(&ch, FALSE);
  CuAssertIntEquals(tc, human_hp + 20, GET_MAX_HIT(&ch));
  GET_REAL_RACE(&ch) = RACE_HALF_ILLITHID;
  calculate_max_hp(&ch, FALSE);
  CuAssertIntEquals(tc, human_hp + 50, GET_MAX_HIT(&ch));
  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  calculate_max_hp(&ch, FALSE);
  CuAssertIntEquals(tc, human_hp, GET_MAX_HIT(&ch));
  GET_REAL_RACE(&ch) = RACE_MYCONID;
  calculate_max_hp(&ch, FALSE);
  CuAssertIntEquals(tc, human_hp + 50, GET_MAX_HIT(&ch));

  CuAssertIntEquals(tc, 10, race_starting_hp_bonus(RACE_HALF_ILLITHID));
  CuAssertIntEquals(tc, 10, race_starting_hp_bonus(RACE_MYCONID));
  CuAssertIntEquals(tc, 0, race_starting_hp_bonus(RACE_WEMIC));
  CuAssertIntEquals(tc, 4, race_hp_bonus_per_level(RACE_HALF_ILLITHID));
  CuAssertIntEquals(tc, 4, race_hp_bonus_per_level(RACE_MYCONID));
  CuAssertIntEquals(tc, 2, race_hp_bonus_per_level(RACE_HALF_OGRE));
  CuAssertIntEquals(tc, 1, race_hp_bonus_per_level(RACE_WEMIC));
  CuAssertIntEquals(tc, 0, race_hp_bonus_per_level(RACE_YUAN_TI));
}

void TestRaceEquivalenceExperienceMultipliers(CuTest *tc)
{
  struct char_data ch;
  long normal = 0;
  int old_multiplier = CONFIG_EXPERIENCE_MULTIPLIER;

  memset(&ch, 0, sizeof(ch));
  GET_CLASS(&ch) = CLASS_WARRIOR;
  CONFIG_EXPERIENCE_MULTIPLIER = 100;

  GET_REAL_RACE(&ch) = RACE_HUMAN;
  normal = level_exp(&ch, 10);
  GET_REAL_RACE(&ch) = RACE_WEMIC;
  CuAssertTrue(tc, level_exp(&ch, 10) == normal * 2);
  GET_REAL_RACE(&ch) = RACE_HALF_OGRE;
  CuAssertTrue(tc, level_exp(&ch, 10) == normal * 2);
  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  CuAssertTrue(tc, level_exp(&ch, 10) == normal * 2);
  GET_REAL_RACE(&ch) = RACE_HALF_ILLITHID;
  CuAssertTrue(tc, level_exp(&ch, 10) == normal * 7);
  GET_REAL_RACE(&ch) = RACE_MYCONID;
  CuAssertTrue(tc, level_exp(&ch, 10) == normal * 7);

  CONFIG_EXPERIENCE_MULTIPLIER = old_multiplier;
}
