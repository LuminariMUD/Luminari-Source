#include "CuTest.h"

#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/act/act.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/core/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/character/evolutions.h"
#include "../../src/character/feats.h"
#include "../../src/character/premadebuilds.h"
#include "../../src/character/race.h"
#include "../../src/character/backgrounds.h"
#include "../../src/character/roleplay.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/net/protocol.h"
#include "../../src/net/onboarding.h"
#include "../../src/quest/quest.h"
#include "../../src/quest/hlquest.h"

static void ensure_race_equivalence_registry(void)
{
  if (race_list[RACE_WEMIC].type == NULL)
    assign_races();
  if (feat_list[FEAT_ARMOR_SKIN].name == NULL)
    assign_feats();
}

/* The form conversions name and respec classes, so they need the class table too. */
static void ensure_descend_form_registry(void)
{
  ensure_race_equivalence_registry();
  if (class_list[CLASS_WARRIOR].name == NULL)
    load_class_list();
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
  obj->name = CuMutableString(name);
  obj->short_description = CuMutableString(name);
  obj->description = CuMutableString(name);
  GET_OBJ_TYPE(obj) = ITEM_ARMOR;
  GET_OBJ_SIZE(obj) = SIZE_LARGE;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(obj), wear_flag);
}

/* Real player saves also write the index: keep every persistence fixture isolated. */
static void enter_race_player_fixture(CuTest *tc, char *temporary_directory)
{
  CuAssertPtrNotNull(tc, mkdtemp(temporary_directory));
  CuAssertIntEquals(tc, 0, chdir(temporary_directory));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles/U-Z", 0700));
}

static int leave_race_player_fixture(const char *directory, const char *temporary_directory)
{
  int result;

  unlink("plrfiles/index");
  rmdir("plrfiles/U-Z");
  rmdir("plrfiles");
  result = chdir(directory);
  if (result == 0)
    rmdir(temporary_directory);
  return result;
}

/* Count the epic damage reduction entries and report the last one's amount. */
static int count_feat_damage_reduction(struct char_data *ch, int *amount)
{
  struct damage_reduction_type *dr;
  int count = 0;

  for (dr = GET_DR(ch); dr != NULL; dr = dr->next)
    if (dr->feat == FEAT_DAMAGE_REDUCTION)
    {
      count++;
      *amount = dr->amount;
    }

  return count;
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
      {0, 0, 2, 0, 4, 2},   {8, 6, -2, -2, -4, -4},
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
  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_WEMIC, FEAT_TAURIC_FRAME));
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
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_TAURIC_FRAME));

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
  const int yuan_ti_restricted_slots[] = {WEAR_FACE, WEAR_LEGS, WEAR_FEET};
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  size_t i;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);

  GET_REAL_RACE(&ch) = RACE_HUMAN;
  for (i = 0; i < NUM_WEARS; i++)
    CuAssertIntEquals(tc, i != WEAR_TAIL && wear_slot_arms_needed((int)i) <= 2,
                      character_can_use_wear_slot(&ch, (int)i));

  CuAssertTrue(tc, !character_has_tail_wear_slot(&ch));
  CuAssertPtrNotNull(tc, strstr(character_wear_slot_restriction(&ch, WEAR_TAIL), "tail equipment"));

  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  CuAssertTrue(tc, character_has_tail_wear_slot(&ch));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_TAIL));
  for (i = 0; i < sizeof(yuan_ti_restricted_slots) / sizeof(yuan_ti_restricted_slots[0]); i++)
    CuAssertTrue(tc, !character_can_use_wear_slot(&ch, yuan_ti_restricted_slots[i]));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_BODY));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_ANKLE_R));
  CuAssertPtrNotNull(tc, strstr(character_wear_slot_restriction(&ch, WEAR_FACE), "serpentine"));

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

void TestTailSlotRingAndDedicatedGearContract(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  struct obj_data ring;
  struct obj_data tail_armor;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  if (descriptor.pProtocol == NULL)
    return;
  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  GET_REAL_SIZE(&ch) = SIZE_MEDIUM;
  ch.points.size = SIZE_MEDIUM;

  init_race_equipment_object(&ring, "a plain test ring", ITEM_WEAR_FINGER);
  GET_OBJ_SIZE(&ring) = SIZE_MEDIUM;
  CuAssertTrue(tc, object_is_ring(&ring));
  CuAssertTrue(tc, object_can_wear_on_tail(&ring));
  CuAssertTrue(tc, !CAN_WEAR(&ring, ITEM_WEAR_TAIL));
  obj_to_char(&ring, &ch);
  CuAssertIntEquals(tc, WEAR_TAIL, find_eq_pos(&ch, &ring, NULL));
  perform_wear(&ch, &ring, WEAR_TAIL);
  CuAssertPtrEquals(tc, &ring, GET_EQ(&ch, WEAR_TAIL));
  CuAssertPtrEquals(tc, &ring, unequip_char(&ch, WEAR_TAIL));

  init_race_equipment_object(&tail_armor, "a set of test tail plates", ITEM_WEAR_TAIL);
  GET_OBJ_SIZE(&tail_armor) = SIZE_MEDIUM;
  SET_BIT_AR(GET_OBJ_WEAR(&tail_armor), ITEM_WEAR_BODY);
  GET_OBJ_VAL(&tail_armor, 0) = 6;
  CuAssertTrue(tc, !object_is_ring(&tail_armor));
  CuAssertTrue(tc, object_is_dedicated_tail_gear(&tail_armor));
  CuAssertIntEquals(tc, WEAR_TAIL, find_eq_pos(&ch, &tail_armor, NULL));
  obj_to_char(&tail_armor, &ch);
  perform_wear(&ch, &tail_armor, WEAR_BODY);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&ch, WEAR_BODY));
  CuAssertPtrEquals(tc, &ch, tail_armor.carried_by);
  CuAssertPtrNotNull(tc, strstr(descriptor.output, "only wear"));
  perform_wear(&ch, &tail_armor, WEAR_TAIL);
  CuAssertPtrEquals(tc, &tail_armor, GET_EQ(&ch, WEAR_TAIL));
  CuAssertIntEquals(tc, 6, ch.points.armor);
  CuAssertPtrEquals(tc, &tail_armor, unequip_char(&ch, WEAR_TAIL));
  CuAssertIntEquals(tc, 0, ch.points.armor);

  cleanup_race_equivalence_descriptor(&descriptor);
}

void TestConvertedArmorFamiliesPreserveAcAndUseNativePenalties(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  struct obj_data armor;
  const int families[] = {SPEC_ARMOR_TYPE_CLOTHING, SPEC_ARMOR_TYPE_LEATHER,
                          SPEC_ARMOR_TYPE_CHAINMAIL, SPEC_ARMOR_TYPE_FULL_PLATE};
  const int checks[] = {0, 0, -5, -6};
  const int failures[] = {0, 10, 30, 35};
  const int dex_caps[] = {99, 13, 8, 7};
  size_t i;

  ensure_race_equivalence_registry();
  load_armor();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  GET_REAL_SIZE(&ch) = SIZE_MEDIUM;
  ch.points.size = SIZE_MEDIUM;
  for (i = 0; i < sizeof(families) / sizeof(families[0]); i++)
  {
    init_race_equipment_object(&armor, "converted body armor", ITEM_WEAR_BODY);
    GET_OBJ_SIZE(&armor) = SIZE_MEDIUM;
    GET_OBJ_VAL(&armor, 0) = 23;
    GET_OBJ_VAL(&armor, 1) = families[i];
    equip_char(&ch, &armor, WEAR_BODY);
    CuAssertPtrEquals(tc, &armor, GET_EQ(&ch, WEAR_BODY));
    CuAssertIntEquals(tc, 23, ch.points.armor);
    CuAssertIntEquals(tc, checks[i], compute_gear_armor_penalty(&ch));
    CuAssertIntEquals(tc, failures[i], compute_gear_spell_failure(&ch));
    CuAssertIntEquals(tc, dex_caps[i], compute_gear_max_dex(&ch));
    if (families[i] == SPEC_ARMOR_TYPE_FULL_PLATE)
    {
      CuAssertIntEquals(tc, ARMOR_TYPE_HEAVY, compute_gear_armor_type(&ch));
      CuAssertTrue(tc, !is_proficient_with_body_armor(&ch));
      SET_FEAT(&ch, FEAT_ARMOR_PROFICIENCY_HEAVY, 1);
      CuAssertTrue(tc, is_proficient_with_body_armor(&ch));
      SET_FEAT(&ch, FEAT_ARMOR_PROFICIENCY_HEAVY, 0);
    }
    CuAssertPtrEquals(tc, &armor, unequip_char(&ch, WEAR_BODY));
    CuAssertIntEquals(tc, 0, ch.points.armor);
  }

  /* Dedicated tail armor has no family index, and adds no body penalties. */
  init_race_equipment_object(&armor, "converted tail plates", ITEM_WEAR_TAIL);
  GET_OBJ_SIZE(&armor) = SIZE_MEDIUM;
  GET_OBJ_VAL(&armor, 0) = 6;
  equip_char(&ch, &armor, WEAR_TAIL);
  CuAssertIntEquals(tc, 6, ch.points.armor);
  CuAssertIntEquals(tc, ARMOR_TYPE_NONE, compute_gear_armor_type(&ch));
  CuAssertIntEquals(tc, 0, compute_gear_armor_penalty(&ch));
  CuAssertIntEquals(tc, 0, compute_gear_spell_failure(&ch));
  CuAssertIntEquals(tc, 99, compute_gear_max_dex(&ch));
  CuAssertPtrEquals(tc, &armor, unequip_char(&ch, WEAR_TAIL));
  CuAssertIntEquals(tc, 0, ch.points.armor);

  GET_REAL_RACE(&ch) = RACE_HUMAN;
  init_race_equipment_object(&armor, "converted cursed shackles", ITEM_WEAR_LEGS);
  GET_OBJ_SIZE(&armor) = SIZE_MEDIUM;
  GET_OBJ_VAL(&armor, 0) = -100;
  GET_OBJ_VAL(&armor, 1) = SPEC_ARMOR_TYPE_CLOTHING_LEGS;
  equip_char(&ch, &armor, WEAR_LEGS);
  CuAssertPtrEquals(tc, &armor, GET_EQ(&ch, WEAR_LEGS));
  CuAssertIntEquals(tc, -100, ch.points.armor);
  CuAssertIntEquals(tc, 0, compute_gear_armor_penalty(&ch));
  CuAssertPtrEquals(tc, &armor, unequip_char(&ch, WEAR_LEGS));
  CuAssertIntEquals(tc, 0, ch.points.armor);
  cleanup_race_equivalence_descriptor(&descriptor);
}

void TestConvertedWornArmorStacksAndUnequipsWithoutTailDoubleCounting(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  struct obj_data gloves;
  struct obj_data cloak;
  struct obj_data bracelet;
  struct obj_data ring;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  if (descriptor.pProtocol == NULL)
    return;
  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  GET_REAL_SIZE(&ch) = SIZE_MEDIUM;
  ch.points.size = SIZE_MEDIUM;

  /* Converted value-0 protection and a separately authored AC apply stack. */
  init_race_equipment_object(&gloves, "protective gloves", ITEM_WEAR_HANDS);
  GET_OBJ_TYPE(&gloves) = ITEM_WORN;
  GET_OBJ_SIZE(&gloves) = SIZE_MEDIUM;
  gloves.affected[0].location = APPLY_AC_NEW;
  gloves.affected[0].modifier = 1;
  gloves.affected[0].bonus_type = BONUS_TYPE_UNIVERSAL;
  gloves.affected[1].location = APPLY_AC_NEW;
  gloves.affected[1].modifier = 2;
  gloves.affected[1].bonus_type = BONUS_TYPE_UNIVERSAL;
  equip_char(&ch, &gloves, WEAR_HANDS);
  CuAssertPtrEquals(tc, &gloves, GET_EQ(&ch, WEAR_HANDS));
  CuAssertIntEquals(tc, 30, ch.points.armor);

  init_race_equipment_object(&cloak, "protective cloak", ITEM_WEAR_ABOUT);
  GET_OBJ_TYPE(&cloak) = ITEM_WORN;
  GET_OBJ_SIZE(&cloak) = SIZE_MEDIUM;
  cloak.affected[0] = gloves.affected[0];
  equip_char(&ch, &cloak, WEAR_ABOUT);
  CuAssertIntEquals(tc, 40, ch.points.armor);

  init_race_equipment_object(&bracelet, "harmful bracelet", ITEM_WEAR_WRIST);
  GET_OBJ_TYPE(&bracelet) = ITEM_WORN;
  GET_OBJ_SIZE(&bracelet) = SIZE_MEDIUM;
  bracelet.affected[0] = gloves.affected[0];
  bracelet.affected[0].modifier = -2;
  equip_char(&ch, &bracelet, WEAR_WRIST_R);
  CuAssertIntEquals(tc, 20, ch.points.armor);

  init_race_equipment_object(&ring, "protective tail ring", ITEM_WEAR_FINGER);
  GET_OBJ_TYPE(&ring) = ITEM_WORN;
  GET_OBJ_SIZE(&ring) = SIZE_MEDIUM;
  ring.affected[0] = gloves.affected[0];
  obj_to_char(&ring, &ch);
  perform_wear(&ch, &ring, WEAR_TAIL);
  CuAssertPtrEquals(tc, &ring, GET_EQ(&ch, WEAR_TAIL));
  CuAssertIntEquals(tc, 0, apply_ac(&ch, WEAR_TAIL));
  CuAssertIntEquals(tc, 30, ch.points.armor);

  CuAssertPtrEquals(tc, &gloves, unequip_char(&ch, WEAR_HANDS));
  CuAssertIntEquals(tc, 0, ch.points.armor);
  CuAssertPtrEquals(tc, &bracelet, unequip_char(&ch, WEAR_WRIST_R));
  CuAssertIntEquals(tc, 20, ch.points.armor);
  CuAssertPtrEquals(tc, &ring, unequip_char(&ch, WEAR_TAIL));
  CuAssertIntEquals(tc, 10, ch.points.armor);
  CuAssertPtrEquals(tc, &cloak, unequip_char(&ch, WEAR_ABOUT));
  CuAssertIntEquals(tc, 0, ch.points.armor);
  cleanup_race_equivalence_descriptor(&descriptor);
}

void TestTailSlotRestrictionsCoverDirectAndPersistenceEntryPaths(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  struct obj_data ring;
  struct obj_data tail_gear;
  struct obj_data body_gear;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  GET_REAL_RACE(&ch) = RACE_HUMAN;

  init_race_equipment_object(&ring, "a persistence test ring", ITEM_WEAR_FINGER);
  test_auto_equip_loaded_object(&ch, &ring, WEAR_TAIL + 1);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&ch, WEAR_TAIL));
  CuAssertPtrEquals(tc, &ch, ring.carried_by);
  obj_from_char(&ring);

  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  init_race_equipment_object(&tail_gear, "a persistence tail guard", ITEM_WEAR_TAIL);
  SET_BIT_AR(GET_OBJ_WEAR(&tail_gear), ITEM_WEAR_BODY);
  test_auto_equip_loaded_object(&ch, &tail_gear, WEAR_BODY + 1);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&ch, WEAR_BODY));
  CuAssertPtrEquals(tc, &ch, tail_gear.carried_by);
  obj_from_char(&tail_gear);

  init_race_equipment_object(&body_gear, "ordinary body armor", ITEM_WEAR_BODY);
  equip_char(&ch, &body_gear, WEAR_TAIL);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&ch, WEAR_TAIL));
  CuAssertPtrEquals(tc, &ch, body_gear.carried_by);
  obj_from_char(&body_gear);

  init_race_equipment_object(&ring, "a restored test ring", ITEM_WEAR_FINGER);
  test_auto_equip_loaded_object(&ch, &ring, WEAR_TAIL + 1);
  CuAssertPtrEquals(tc, &ring, GET_EQ(&ch, WEAR_TAIL));
  CuAssertPtrEquals(tc, &ring, unequip_char(&ch, WEAR_TAIL));
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
  struct obj_data face_covering;

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

  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  GET_REAL_SIZE(&ch) = SIZE_MEDIUM;
  ch.points.size = SIZE_MEDIUM;
  init_race_equipment_object(&face_covering, "a test face covering", ITEM_WEAR_FACE);
  GET_OBJ_SIZE(&face_covering) = SIZE_MEDIUM;
  obj_to_char(&face_covering, &ch);
  perform_wear(&ch, &face_covering, WEAR_FACE);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&ch, WEAR_FACE));
  CuAssertPtrEquals(tc, &ch, face_covering.carried_by);
  CuAssertPtrNotNull(tc, strstr(descriptor.output, "serpentine anatomy"));
  obj_from_char(&face_covering);

  init_race_equipment_object(&pants, "a pair of Yuan-Ti test pants", ITEM_WEAR_LEGS);
  GET_OBJ_SIZE(&pants) = SIZE_MEDIUM;
  test_auto_equip_loaded_object(&ch, &pants, WEAR_LEGS + 1);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&ch, WEAR_LEGS));
  CuAssertPtrEquals(tc, &ch, pants.carried_by);
  obj_from_char(&pants);

  init_race_equipment_object(&boots, "a pair of Yuan-Ti test boots", ITEM_WEAR_FEET);
  GET_OBJ_SIZE(&boots) = SIZE_MEDIUM;
  equip_char(&ch, &boots, WEAR_FEET);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&ch, WEAR_FEET));
  CuAssertPtrEquals(tc, &ch, boots.carried_by);
  obj_from_char(&boots);
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

/* Each roleplay idea menu leaves for its text editor on "q". */
void TestRoleplayIdeaMenusProceedToTheirEditors(CuTest *tc)
{
  static const int menus[][2] = {
      {CON_CHARACTER_PERSONALITY_IDEAS, CON_CHARACTER_PERSONALITY_ENTER},
      {CON_CHARACTER_IDEALS_IDEAS, CON_CHARACTER_IDEALS_ENTER},
      {CON_CHARACTER_BONDS_IDEAS, CON_CHARACTER_BONDS_ENTER},
      {CON_CHARACTER_FLAWS_IDEAS, CON_CHARACTER_FLAWS_ENTER},
  };
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  char input[MAX_INPUT_LENGTH];
  size_t i;

  if (background_list[1].name == NULL)
    assign_backgrounds();
  for (i = 0; i < sizeof(menus) / sizeof(menus[0]); i++)
  {
    init_race_equivalence_character(&ch, &specials, &descriptor, &account);
    descriptor.pProtocol = ProtocolCreate();
    CuAssertPtrNotNull(tc, descriptor.pProtocol);
    STATE(&descriptor) = menus[i][0];
    /* A background number shows an example and stays in the menu. */
    snprintf(input, sizeof(input), "1");
    nanny(&descriptor, input);
    CuAssertIntEquals(tc, menus[i][0], STATE(&descriptor));
    CuAssertPtrNotNull(tc, strstr(descriptor.output, "Enter a background number"));
    snprintf(input, sizeof(input), "%d", NUM_BACKGROUNDS + 5);
    nanny(&descriptor, input);
    CuAssertIntEquals(tc, menus[i][0], STATE(&descriptor));
    snprintf(input, sizeof(input), "q");
    nanny(&descriptor, input);
    CuAssertIntEquals(tc, menus[i][1], STATE(&descriptor));
    CuAssertPtrNotNull(tc, descriptor.str);
    CuAssertPtrNotNull(tc, strstr(descriptor.output, "Enter your character"));
    roleplay_pending_clear_examples(&descriptor);
    free(descriptor.backstr);
    cleanup_race_equivalence_descriptor(&descriptor);
  }
}

/* Three race grants of epic damage reduction build one DR 9 entry.  The entry is never saved,
 * and loading rebuilds it from the feat. */
void TestRaceGrantedDamageReductionBuildsOneEntryThatSurvivesReload(CuTest *tc)
{
  struct race_feat_assign grants[3];
  struct race_feat_assign *saved_list;
  struct player_index_element fixture_index[1];
  struct player_index_element *saved_player_table;
  struct char_data *source;
  struct char_data *loaded;
  char temporary_directory[] = "/tmp/luminari-race-fixture-XXXXXX";
  char original_directory[PATH_MAX];
  char filename[MAX_FILEPATH];
  char player_name[32];
  int saved_top_of_p_table;
  int granted_rank, granted_count, granted_amount = 0;
  int loaded_rank = -1, loaded_count = -1, loaded_amount = 0;
  int load_result = -1;
  int restore_result;
  int i;

  ensure_race_equivalence_registry();
  memset(grants, 0, sizeof(grants));
  saved_list = race_list[RACE_HUMAN].featassign_list;
  for (i = 0; i < 3; i++)
  {
    grants[i].feat_num = FEAT_DAMAGE_REDUCTION;
    grants[i].level_received = 1;
    grants[i].next = i < 2 ? &grants[i + 1] : saved_list;
  }
  race_list[RACE_HUMAN].featassign_list = grants;

  memset(fixture_index, 0, sizeof(fixture_index));
  CuAssertPtrNotNull(tc, getcwd(original_directory, sizeof(original_directory)));
  enter_race_player_fixture(tc, temporary_directory);
  source = new_char();
  loaded = new_char();
  snprintf(player_name, sizeof(player_name), "Zzdr%ld", (long)getpid());
  fixture_index[0].name = player_name;
  fixture_index[0].id = 4243;
  fixture_index[0].level = 1;
  saved_player_table = player_table;
  saved_top_of_p_table = top_of_p_table;
  player_table = fixture_index;
  top_of_p_table = 0;

  source->player.name = strdup(player_name);
  GET_PFILEPOS(source) = 0;
  GET_IDNUM(source) = 4243;
  GET_LEVEL(source) = 1;
  GET_REAL_RACE(source) = RACE_HUMAN;
  process_race_level_feats(source);
  race_list[RACE_HUMAN].featassign_list = saved_list;
  granted_rank = HAS_REAL_FEAT(source, FEAT_DAMAGE_REDUCTION);
  granted_count = count_feat_damage_reduction(source, &granted_amount);

  if (get_filename(filename, sizeof(filename), PLR_FILE, player_name))
  {
    save_char(source, TRUE);
    load_result = load_char(player_name, loaded);
    if (load_result >= 0)
    {
      loaded_rank = HAS_REAL_FEAT(loaded, FEAT_DAMAGE_REDUCTION);
      loaded_count = count_feat_damage_reduction(loaded, &loaded_amount);
    }
    unlink(filename);
  }

  restore_result = leave_race_player_fixture(original_directory, temporary_directory);
  free_char(loaded);
  free_char(source);
  player_table = saved_player_table;
  top_of_p_table = saved_top_of_p_table;

  CuAssertIntEquals(tc, 0, restore_result);
  CuAssertIntEquals(tc, 3, granted_rank);
  CuAssertIntEquals(tc, 1, granted_count);
  CuAssertIntEquals(tc, 9, granted_amount);
  CuAssertTrue(tc, load_result >= 0);
  CuAssertIntEquals(tc, 3, loaded_rank);
  CuAssertIntEquals(tc, 1, loaded_count);
  CuAssertIntEquals(tc, 9, loaded_amount);
}

/* The quest conversions pass no respec argument, and neither may crash for any race. */
void TestRespecEngineAcceptsANullArgument(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  int saved_move_gain;
  int class_after;
  int premade_after;

  ensure_race_equivalence_registry();
  clear_char(&ch);
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  ch.player.name = CuMutableString("null respec test character");
  GET_REAL_RACE(&ch) = RACE_HUMAN;
  GET_LEVEL(&ch) = 30;
  GET_EXP(&ch) = 1;

  saved_move_gain = class_list[CLASS_WARRIOR].move_gain;
  class_list[CLASS_WARRIOR].move_gain = 1;
  respec_engine(&ch, CLASS_WARRIOR, NULL, TRUE);
  class_list[CLASS_WARRIOR].move_gain = saved_move_gain;
  class_after = GET_CLASS(&ch);
  premade_after = GET_PREMADE_BUILD_CLASS(&ch);

  while (ch.affected != NULL)
    affect_remove_no_total(&ch, ch.affected);
  free(GET_TITLE(&ch));

  CuAssertIntEquals(tc, CLASS_WARRIOR, class_after);
  CuAssertIntEquals(tc, CLASS_UNDEFINED, premade_after);
}

/* ---- Duris creation races (docs/guides/PLAYER_RACES_REFERENCE.md) ---- */

struct duris_race_expectation
{
  int race;
  int id;
  const char *name;
  const char *type;
  const char *abbrev;
  int family;
  int size;
  int level_adjustment;
  int unlock_cost;
  int tier;
  int stats[6];
  const char *alignments; /* LG NG CG LN TN CN LE NE CE */
  int language;
  const char *attacks; /* the 24 set_race_attack_types() flags */
  int xp_multiplier;
  int hp_per_level;
};

static const struct duris_race_expectation duris_creation_races[] = {
    {RACE_CENTAUR,
     152,
     "centaur",
     "Centaur",
     "Cent",
     RACE_TYPE_MONSTROUS_HUMANOID,
     SIZE_LARGE,
     2,
     1000,
     1,
     {3, 5, -1, 0, 0, 0},
     "YYYYYYYYY",
     SKILL_LANG_ELVEN,
     "YNNNNNNNNNNNNYNNNNNNNYYN",
     2,
     0},
    {RACE_GITHZERAI,
     153,
     "githzerai",
     "Githzerai",
     "Gthz",
     RACE_TYPE_HUMANOID,
     SIZE_MEDIUM,
     2,
     1000,
     1,
     {0, 0, 4, 3, 0, -1},
     "YYYYYYYYY",
     SKILL_LANG_COMMON,
     "YNNNNNNNNNNNNYNNNNNNNNNN",
     2,
     0},
    {RACE_FIRBOLG,
     154,
     "firbolg",
     "Firbolg",
     "Fbol",
     RACE_TYPE_GIANT,
     SIZE_LARGE,
     2,
     1000,
     1,
     {5, 4, -1, 0, -1, 0},
     "YYYYYYYYY",
     SKILL_LANG_GIANT,
     "YNNNNNNNNNNNNYNNNNNNYNNN",
     2,
     0},
    {RACE_GITHYANKI,
     155,
     "githyanki",
     "Githyanki",
     "Gthy",
     RACE_TYPE_HUMANOID,
     SIZE_MEDIUM,
     2,
     1000,
     1,
     {0, 0, 4, 0, 0, -1},
     "NNNYYYYYY",
     SKILL_LANG_COMMON,
     "YNNYNNNNNNNNNYNNNNNNNNNN",
     2,
     0},
    {RACE_KOBOLD,
     156,
     "kobold",
     "Kobold",
     "Kobo",
     RACE_TYPE_HUMANOID,
     SIZE_SMALL,
     0,
     0,
     0,
     {-1, 0, 2, 0, 2, 0},
     "NNNYYYYYY",
     SKILL_LANG_KOBOLD,
     "YNNNYNNNYNNNNNNNNNNNNNNN",
     1,
     0},
    {RACE_DRIDER,
     157,
     "drider",
     "Drider",
     "Drdr",
     RACE_TYPE_ABERRATION,
     SIZE_LARGE,
     2,
     1000,
     1,
     {0, 5, 0, 0, 3, -1},
     "NNNNNNYYY",
     SKILL_LANG_UNDERCOMMON,
     "NNNNYNNNYNNYNNNNNNNNNNNN",
     2,
     0},
    {RACE_THRI_KREEN,
     158,
     "thrikreen",
     "Thri-Kreen",
     "TKrn",
     RACE_TYPE_MONSTROUS_HUMANOID,
     SIZE_MEDIUM,
     10,
     50000,
     2,
     {0, 0, -2, -2, 4, -2},
     "YYYYYYYYY",
     SKILL_LANG_COMMON,
     "NNNYYNNNYNNNNNNNNNNNNNNN",
     7,
     0},
    {RACE_MINOTAUR,
     159,
     "minotaur",
     "Minotaur",
     "Mino",
     RACE_TYPE_MONSTROUS_HUMANOID,
     SIZE_LARGE,
     2,
     1000,
     1,
     {3, 4, 0, 0, 0, 0},
     "YYYYYYYYY",
     SKILL_LANG_GIANT,
     "YNNNNNNNNNNNNNNNNNNNNNYY",
     2,
     0},
    {RACE_KUO_TOA,
     160,
     "kuotoa",
     "Kuo Toa",
     "KToa",
     RACE_TYPE_MONSTROUS_HUMANOID,
     SIZE_MEDIUM,
     0,
     0,
     0,
     {1, 2, 0, 0, 0, 0},
     "NNNNNNYYY",
     SKILL_LANG_UNDERCOMMON,
     "YNNNYNNNNNNYNNNNNNNNNNNN",
     1,
     0},
    {RACE_OROG,
     161,
     "orog",
     "Orog",
     "Orog",
     RACE_TYPE_HUMANOID,
     SIZE_MEDIUM,
     2,
     1000,
     1,
     {3, 4, -2, 0, 0, 0},
     "NNNYYYYYY",
     SKILL_LANG_ORCISH,
     "YNNNNNNNNNNNNYNNNNNNYNNN",
     2,
     1},
    {RACE_HARPY,
     162,
     "harpy",
     "Harpy",
     "Hrpy",
     RACE_TYPE_MONSTROUS_HUMANOID,
     SIZE_SMALL,
     2,
     1000,
     1,
     {-2, 1, 2, 1, 2, 0},
     "YYYYYYYYY",
     SKILL_LANG_COMMON,
     "NNNNNNNNYNNNNNNNNNYYNNNN",
     2,
     1},
    {RACE_STORMKIN,
     163,
     "stormkin",
     "Stormkin",
     "Stmk",
     RACE_TYPE_GIANT,
     SIZE_LARGE,
     2,
     1000,
     1,
     {5, 4, 0, 0, -2, 0},
     "YYYYYYYYY",
     SKILL_LANG_GIANT,
     "YNNNNNYNNNNNNYNNNNNNYNNN",
     2,
     0},
};

struct duris_feat_expectation
{
  int race;
  int feat;
  int level;
  int count;
};

static const struct duris_feat_expectation duris_creation_feats[] = {
    {RACE_CENTAUR, FEAT_QUADRUPED_BODY, 1, 1},
    {RACE_CENTAUR, FEAT_TAURIC_FRAME, 1, 1},
    {RACE_CENTAUR, FEAT_DOORBASH, 1, 1},
    {RACE_CENTAUR, FEAT_STAMPEDE, 11, 1},
    {RACE_CENTAUR, FEAT_GREATSWORD_MASTERY, 16, 1},
    {RACE_GITHZERAI, FEAT_ULTRAVISION, 1, 1},
    {RACE_GITHZERAI, FEAT_HALF_DROW_SPELL_RESISTANCE, 1, 1},
    {RACE_GITHZERAI, FEAT_SLA_PLANE_SHIFT, 1, 1},
    {RACE_GITHZERAI, FEAT_QUICK_THINKING, 1, 1},
    {RACE_GITHZERAI, FEAT_SLA_LEVITATE, 6, 1},
    {RACE_GITHZERAI, FEAT_RRAKKMA, 11, 1},
    {RACE_FIRBOLG, FEAT_BODYSLAM, 1, 1},
    {RACE_FIRBOLG, FEAT_DOORBASH, 1, 1},
    {RACE_FIRBOLG, FEAT_FOREST_SIGHT, 1, 1},
    {RACE_FIRBOLG, FEAT_MAGIC_VULNERABILITY, 1, 1},
    {RACE_FIRBOLG, FEAT_SLOW_CASTING, 1, 1},
    {RACE_FIRBOLG, FEAT_OUTDOOR_STEALTH, 6, 1},
    {RACE_FIRBOLG, FEAT_HATRED, 11, 1},
    {RACE_FIRBOLG, FEAT_HAMMER_MASTERY, 16, 1},
    {RACE_GITHYANKI, FEAT_ULTRAVISION, 1, 1},
    {RACE_GITHYANKI, FEAT_HALF_DROW_SPELL_RESISTANCE, 1, 1},
    {RACE_GITHYANKI, FEAT_SLA_PLANE_SHIFT, 1, 1},
    {RACE_GITHYANKI, FEAT_ENHANCED_SPELL_DAMAGE, 1, 1},
    {RACE_GITHYANKI, FEAT_SLA_PSIONIC_BLAST, 1, 1},
    {RACE_GITHYANKI, FEAT_SLA_LEVITATE, 6, 1},
    {RACE_GITHYANKI, FEAT_LONGSWORD_MASTERY, 6, 1},
    {RACE_KOBOLD, FEAT_ULTRAVISION, 1, 1},
    {RACE_KOBOLD, FEAT_UNDERDARK_STEALTH, 1, 1},
    {RACE_KOBOLD, FEAT_CALMING, 1, 1},
    {RACE_KOBOLD, FEAT_BARTER, 1, 1},
    {RACE_KOBOLD, FEAT_FAST_CASTING, 1, 1},
    {RACE_KOBOLD, FEAT_MINER, 26, 1},
    {RACE_DRIDER, FEAT_ULTRAVISION, 1, 1},
    {RACE_DRIDER, FEAT_HALF_DROW_SPELL_RESISTANCE, 1, 1},
    {RACE_DRIDER, FEAT_QUADRUPED_BODY, 1, 1},
    {RACE_DRIDER, FEAT_TAURIC_FRAME, 1, 1},
    {RACE_DRIDER, FEAT_SLA_WEB, 1, 1},
    {RACE_DRIDER, FEAT_GROUNDFIGHTING, 11, 1},
    {RACE_DRIDER, FEAT_SLA_FIREBALL, 11, 1},
    {RACE_DRIDER, FEAT_SLA_MASS_DISPEL, 27, 1},
    {RACE_THRI_KREEN, FEAT_ULTRAVISION, 1, 1},
    {RACE_THRI_KREEN, FEAT_FOUR_ARMS, 1, 1},
    {RACE_THRI_KREEN, FEAT_PSIONIC_RESISTANCE, 1, 1},
    {RACE_THRI_KREEN, FEAT_VULNERABLE_TO_COLD, 1, 1},
    {RACE_THRI_KREEN, FEAT_POISON_BITE, 6, 1},
    {RACE_THRI_KREEN, FEAT_LEAP, 11, 1},
    {RACE_MINOTAUR, FEAT_ULTRAVISION, 1, 1},
    {RACE_MINOTAUR, FEAT_DOORBASH, 1, 1},
    {RACE_MINOTAUR, FEAT_BLOODLUST, 1, 1},
    {RACE_MINOTAUR, FEAT_BULL_CHARGE, 6, 1},
    {RACE_MINOTAUR, FEAT_AXE_MASTERY, 6, 1},
    {RACE_MINOTAUR, FEAT_SLA_SCARE, 6, 1},
    {RACE_MINOTAUR, FEAT_KENDER_FEARLESSNESS, 21, 1},
    {RACE_KUO_TOA, FEAT_ULTRAVISION, 1, 1},
    {RACE_KUO_TOA, FEAT_KEEN_SENSES, 1, 1},
    {RACE_KUO_TOA, FEAT_SWAMP_STEALTH, 1, 1},
    {RACE_KUO_TOA, FEAT_SEADOG, 1, 1},
    {RACE_KUO_TOA, FEAT_SUN_VULNERABILITY, 1, 1},
    {RACE_KUO_TOA, FEAT_SLOW_CASTING, 1, 1},
    {RACE_KUO_TOA, FEAT_WATER_BREATHING, 8, 1},
    {RACE_KUO_TOA, FEAT_SLA_LIGHTNING_BOLT, 15, 1},
    {RACE_OROG, FEAT_ULTRAVISION, 1, 1},
    {RACE_OROG, FEAT_HARDY, 1, 1},
    {RACE_OROG, FEAT_ARMOR_SKIN, 1, 1},
    {RACE_OROG, FEAT_MAGICAL_REDUCTION, 1, 1},
    {RACE_OROG, FEAT_SUN_VULNERABILITY, 1, 1},
    {RACE_OROG, FEAT_SLOW_CASTING, 1, 6},
    {RACE_OROG, FEAT_SUMMON_HORDE, 6, 1},
    {RACE_OROG, FEAT_SUMMON_WARG, 8, 1},
    {RACE_OROG, FEAT_WARCALLERS_FURY, 11, 1},
    {RACE_HARPY, FEAT_ULTRAVISION, 1, 1},
    {RACE_HARPY, FEAT_WINGS, 1, 1},
    {RACE_HARPY, FEAT_KEEN_SENSES, 1, 1},
    {RACE_HARPY, FEAT_HARDY, 1, 1},
    {RACE_HARPY, FEAT_FAST_CASTING, 1, 3},
    {RACE_HARPY, FEAT_SLA_FARSEE, 11, 1},
    {RACE_HARPY, FEAT_HASTE, 16, 1},
    {RACE_STORMKIN, FEAT_INFRAVISION, 1, 1},
    {RACE_STORMKIN, FEAT_DOORBASH, 1, 1},
    {RACE_STORMKIN, FEAT_SLOW_CASTING, 1, 6},
    {RACE_STORMKIN, FEAT_SLA_LIGHTNING_BOLT, 10, 1},
    {RACE_STORMKIN, FEAT_THICK_HIDE, 11, 1},
};


#define DURIS_CREATION_RACE_COUNT                                                                  \
  ((int)(sizeof(duris_creation_races) / sizeof(duris_creation_races[0])))

/* Every registry field, the parser round trip of the name and the wire value, and a class and
 * alignment that can finish creation. */
void TestDurisCreationRacesRegistry(CuTest *tc)
{
  const struct duris_race_expectation *expected;
  int count = 0;
  int race = 0;
  int i = 0;
  int j = 0;

  ensure_race_equivalence_registry();

  CuAssertIntEquals(tc, 45, NUM_CREATION_RACES);
  CuAssertIntEquals(tc, 164, NUM_EXTENDED_RACES);
  for (race = 0; race < NUM_EXTENDED_RACES; race++)
    if (race_is_creation_eligible(race))
      count++;
  CuAssertIntEquals(tc, NUM_CREATION_RACES, count);

  for (i = 0; i < DURIS_CREATION_RACE_COUNT; i++)
  {
    expected = &duris_creation_races[i];
    race = expected->race;
    CuAssertIntEquals(tc, expected->id, race);
    CuAssertTrue(tc, race_list[race].is_pc);
    CuAssertTrue(tc, race_is_creation_eligible(race));
    CuAssertTrue(tc, !race_is_transformation_only(race));
    CuAssertStrEquals(tc, expected->name, race_list[race].name);
    CuAssertStrEquals(tc, expected->type, race_list[race].type);
    CuAssertStrEquals(tc, expected->abbrev, race_list[race].abbrev);
    CuAssertPtrNotNull(tc, race_list[race].descrip);
    CuAssertIntEquals(tc, expected->family, race_list[race].family);
    CuAssertIntEquals(tc, expected->size, race_list[race].size);
    CuAssertIntEquals(tc, expected->level_adjustment, race_list[race].level_adjustment);
    CuAssertIntEquals(tc, expected->unlock_cost, race_list[race].unlock_cost);
    CuAssertIntEquals(tc, expected->tier, race_list[race].epic_adv);
    CuAssertIntEquals(tc, expected->language, race_list[race].racial_language);
    CuAssertIntEquals(tc, 0, race_list[race].genders[SEX_NEUTRAL]);
    CuAssertIntEquals(tc, 1, race_list[race].genders[SEX_MALE]);
    CuAssertIntEquals(tc, 1, race_list[race].genders[SEX_FEMALE]);
    for (j = 0; j < 6; j++)
      CuAssertIntEquals(tc, expected->stats[j], get_race_stat(race, j));
    for (j = 0; j < NUM_ALIGNMENTS; j++)
      CuAssertIntEquals(tc, expected->alignments[j] == 'Y', race_list[race].alignments[j]);
    for (j = 0; j < NUM_ATTACK_TYPES; j++)
      CuAssertIntEquals(tc, expected->attacks[j] == 'Y', race_list[race].attack_types[j]);
    CuAssertIntEquals(tc, race, parse_race_long(race_list[race].name));
    CuAssertIntEquals(tc, race, parse_race_long(race_list[race].type));
    CuAssertTrue(tc, valid_class_race_alignment(CLASS_WARRIOR, race));
  }

  /* spaced and hyphenated spellings, and the prefixes older races keep */
  CuAssertIntEquals(tc, RACE_THRI_KREEN, parse_race_long("thri kreen"));
  CuAssertIntEquals(tc, RACE_KUO_TOA, parse_race_long("kuo-toa"));
  CuAssertIntEquals(tc, RACE_STOUT_HALFLING, parse_race_long("sto"));
  CuAssertIntEquals(tc, RACE_SHADE, parse_race_long("shad"));
  CuAssertIntEquals(tc, RACE_DROW, parse_race_long("dr"));
  CuAssertIntEquals(tc, RACE_WOOD_ELF, parse_race_long("wi"));
  CuAssertIntEquals(tc, RACE_HALF_OGRE, parse_race_long("o"));
  CuAssertIntEquals(tc, RACE_HALF_ELF, parse_race_long("ha"));
}

/* The race holds exactly its expected feats, each first at its grant level, with the stacked
 * ranks, as it levels from 1 to 30. */
static void assert_race_feat_grants(CuTest *tc, int race,
                                    const struct duris_feat_expectation *expected_feats,
                                    int expected_count)
{
  const struct duris_feat_expectation *expected;
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  struct race_feat_assign *assignment;
  int level = 0;
  int total = 0;
  int j = 0;

  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  ch.desc = NULL;
  for (assignment = race_list[race].featassign_list; assignment != NULL;
       assignment = assignment->next)
    total++;
  for (j = 0; j < expected_count; j++)
    if (expected_feats[j].race == race)
      total -= expected_feats[j].count;
  CuAssertIntEquals(tc, 0, total);

  GET_REAL_RACE(&ch) = race;
  for (level = 1; level <= 30; level++)
  {
    GET_LEVEL(&ch) = level;
    process_race_level_feats(&ch);
    for (j = 0; j < expected_count; j++)
    {
      expected = &expected_feats[j];
      if (expected->race != race)
        continue;
      CuAssertIntEquals(tc, level >= expected->level ? expected->count : 0,
                        HAS_REAL_FEAT(&ch, expected->feat));
    }
  }
  while (GET_DR(&ch) != NULL)
  {
    struct damage_reduction_type *dr = GET_DR(&ch);

    GET_DR(&ch) = dr->next;
    free(dr);
  }
}

void TestDurisCreationRacesFeatGrants(CuTest *tc)
{
  int i = 0;

  ensure_race_equivalence_registry();
  for (i = 0; i < DURIS_CREATION_RACE_COUNT; i++)
    assert_race_feat_grants(tc, duris_creation_races[i].race, duris_creation_feats,
                            (int)(sizeof(duris_creation_feats) / sizeof(duris_creation_feats[0])));
}

/* A disguise or wild shape does not trade a race's later grant for the disguise race's. */
void TestRaceLevelFeatsFollowTheRealRace(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  ch.desc = NULL;
  GET_REAL_RACE(&ch) = RACE_CENTAUR;
  GET_DISGUISE_RACE(&ch) = RACE_WEMIC;
  GET_LEVEL(&ch) = 11;
  process_race_level_feats(&ch);
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_STAMPEDE));

  GET_LEVEL(&ch) = 1;
  process_race_level_feats(&ch);
  CuAssertIntEquals(tc, 1, HAS_REAL_FEAT(&ch, FEAT_TAURIC_FRAME));
  CuAssertIntEquals(tc, 0, HAS_REAL_FEAT(&ch, FEAT_CLAWS_AND_BITE));
}

/* Hit points, experience, and the family predicates that name each non-humanoid PC race. */
void TestDurisCreationRacesHitPointsExperienceAndFamilies(CuTest *tc)
{
  const struct duris_race_expectation *expected;
  struct char_data ch;
  struct char_data *character = &ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  long human_exp = 0;
  int human_hp = 0;
  int old_multiplier = CONFIG_EXPERIENCE_MULTIPLIER;
  int i = 0;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  GET_LEVEL(&ch) = 10;
  GET_REAL_CON(&ch) = 10;
  ch.aff_abils.con = 10;
  GET_CLASS(&ch) = CLASS_WARRIOR;
  CONFIG_EXPERIENCE_MULTIPLIER = 100;

  GET_REAL_RACE(&ch) = RACE_HUMAN;
  calculate_max_hp(&ch, FALSE);
  human_hp = GET_MAX_HIT(&ch);
  human_exp = level_exp(&ch, 10);

  for (i = 0; i < DURIS_CREATION_RACE_COUNT; i++)
  {
    expected = &duris_creation_races[i];
    GET_REAL_RACE(&ch) = expected->race;
    calculate_max_hp(&ch, FALSE);
    CuAssertIntEquals(tc, human_hp + 10 * expected->hp_per_level, GET_MAX_HIT(&ch));
    CuAssertIntEquals(tc, expected->hp_per_level, race_hp_bonus_per_level(expected->race));
    CuAssertIntEquals(tc, 0, race_starting_hp_bonus(expected->race));
    CuAssertTrue(tc, human_exp * expected->xp_multiplier == level_exp(&ch, 10));

    CuAssertIntEquals(tc, expected->family == RACE_TYPE_MONSTROUS_HUMANOID,
                      IS_MONSTROUS_HUMANOID(character) != 0);
    CuAssertIntEquals(tc, expected->family == RACE_TYPE_GIANT, IS_GIANT(character) != 0);
    CuAssertIntEquals(tc, expected->family == RACE_TYPE_ABERRATION, IS_ABERRATION(character) != 0);
    CuAssertIntEquals(tc, expected->family == RACE_TYPE_HUMANOID, IS_HUMANOID(character) != 0);
  }

  CONFIG_EXPERIENCE_MULTIPLIER = old_multiplier;
}

/* Thri-Kreen and Minotaur anatomy rows, and the tauric frame of Centaur and Drider. */
void TestDurisCreationRacesLostSlots(CuTest *tc)
{
  const int thri_kreen_lost[] = {WEAR_BODY,     WEAR_FEET,  WEAR_FINGER_R,
                                 WEAR_FINGER_L, WEAR_EAR_R, WEAR_EAR_L};
  const int tauric[] = {RACE_CENTAUR, RACE_DRIDER};
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  size_t i = 0;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);

  GET_REAL_RACE(&ch) = RACE_THRI_KREEN;
  for (i = 0; i < sizeof(thri_kreen_lost) / sizeof(thri_kreen_lost[0]); i++)
    CuAssertTrue(tc, !character_can_use_wear_slot(&ch, thri_kreen_lost[i]));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_HEAD));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_LEGS));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_WIELD_1));
  SET_FEAT(&ch, FEAT_FOUR_ARMS, 1);
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_WIELD_3));
  SET_FEAT(&ch, FEAT_FOUR_ARMS, 0);

  GET_REAL_RACE(&ch) = RACE_MINOTAUR;
  CuAssertTrue(tc, !character_can_use_wear_slot(&ch, WEAR_HEAD));
  CuAssertPtrNotNull(tc, strstr(character_wear_slot_restriction(&ch, WEAR_HEAD), "horns"));
  CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_LEGS));

  for (i = 0; i < sizeof(tauric) / sizeof(tauric[0]); i++)
  {
    GET_REAL_RACE(&ch) = tauric[i];
    CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_LEGS));
    SET_FEAT(&ch, FEAT_TAURIC_FRAME, 1);
    CuAssertTrue(tc, !character_can_use_wear_slot(&ch, WEAR_LEGS));
    CuAssertTrue(tc, !character_can_use_wear_slot(&ch, WEAR_FEET));
    CuAssertTrue(tc, character_can_use_wear_slot(&ch, WEAR_ANKLE_R));
    SET_FEAT(&ch, FEAT_TAURIC_FRAME, 0);
  }
}

/* Kobold and Kuo Toa are free; the locked races need an unlock, which terminal creation and the
 * account purchase both honor, and the registry wire value is accepted. */
void TestDurisCreationRacesUnlockAndCreationPaths(CuTest *tc)
{
  const struct duris_race_expectation *expected;
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  char input[MAX_INPUT_LENGTH];
  int locked = 0;
  int race = 0;
  int i = 0;

  ensure_race_equivalence_registry();
  for (race = 0; race < NUM_EXTENDED_RACES; race++)
    if (race_is_creation_eligible(race) && is_locked_race(race))
      locked++;
  CuAssertIntEquals(tc, 22, locked);
  CuAssertTrue(tc, locked <= MAX_UNLOCKED_RACES);

  for (i = 0; i < DURIS_CREATION_RACE_COUNT; i++)
  {
    expected = &duris_creation_races[i];
    init_race_equivalence_character(&ch, &specials, &descriptor, &account);
    CuAssertIntEquals(tc, expected->unlock_cost == 0,
                      race_is_selectable_for_creation(&ch, expected->race));
    account.races[0] = expected->race;
    CuAssertTrue(tc, race_is_selectable_for_creation(&ch, expected->race));

    descriptor.pProtocol = ProtocolCreate();
    CuAssertPtrNotNull(tc, descriptor.pProtocol);
    if (descriptor.pProtocol == NULL)
      return;
    GET_REAL_RACE(&ch) = RACE_UNDEFINED;
    STATE(&descriptor) = CON_QRACE;
    snprintf(input, sizeof(input), "%s", race_list[expected->race].type);
    nanny(&descriptor, input);
    CuAssertIntEquals(tc, expected->race, GET_REAL_RACE(&ch));
    CuAssertIntEquals(tc, CON_QRACE_HELP, STATE(&descriptor));
    cleanup_race_equivalence_descriptor(&descriptor);
  }

  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  if (descriptor.pProtocol == NULL)
    return;
  account.experience = 50000;
  snprintf(input, sizeof(input), "race %s", race_list[RACE_THRI_KREEN].type);
  do_accexp(&ch, input, 0, 0);
  CuAssertIntEquals(tc, RACE_THRI_KREEN, account.races[0]);
  CuAssertIntEquals(tc, 0, account.experience);
  CuAssertTrue(tc, has_unlocked_race(&ch, RACE_THRI_KREEN));
  cleanup_race_equivalence_descriptor(&descriptor);
}

/* Premade builds add each race's ability modifiers to the class's base line. */
void TestDurisCreationRacesPremadeBuildStats(CuTest *tc)
{
  const int wizard_base_stats[] = {10, 14, 17, 11, 14, 8};
  struct char_data ch;
  struct player_special_data specials;
  int race = 0;
  int i = 0;

  ensure_race_equivalence_registry();
  memset(&ch, 0, sizeof(ch));
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;

  for (i = 0; i < DURIS_CREATION_RACE_COUNT; i++)
  {
    race = duris_creation_races[i].race;
    GET_REAL_RACE(&ch) = race;
    set_premade_stats(&ch, CLASS_WIZARD, 1);
    CuAssertIntEquals(tc, wizard_base_stats[0] + get_race_stat(race, R_STR_MOD), GET_REAL_STR(&ch));
    CuAssertIntEquals(tc, wizard_base_stats[1] + get_race_stat(race, R_CON_MOD), GET_REAL_CON(&ch));
    CuAssertIntEquals(tc, wizard_base_stats[2] + get_race_stat(race, R_INTEL_MOD),
                      GET_REAL_INT(&ch));
    CuAssertIntEquals(tc, wizard_base_stats[3] + get_race_stat(race, R_WIS_MOD), GET_REAL_WIS(&ch));
    CuAssertIntEquals(tc, wizard_base_stats[4] + get_race_stat(race, R_DEX_MOD), GET_REAL_DEX(&ch));
    CuAssertIntEquals(tc, wizard_base_stats[5] + get_race_stat(race, R_CHA_MOD), GET_REAL_CHA(&ch));
  }
}

/* ---- Duris descend forms ---- */

static const struct duris_race_expectation duris_descend_forms[] = {
    {RACE_DEATH_KNIGHT,
     55,
     "deathknight",
     "Death Knight",
     "DKni",
     RACE_TYPE_UNDEAD,
     SIZE_LARGE,
     10,
     999999999,
     2,
     {8, 6, -2, 6, -2, 0},
     "NNNNNNYYY",
     0,
     "YNNYNNNNNNNNNNNNNNNNYNNN",
     10,
     1},
    {RACE_WIGHT,
     56,
     "wight",
     "Wight",
     "Wght",
     RACE_TYPE_UNDEAD,
     SIZE_LARGE,
     10,
     999999999,
     2,
     {10, 10, -2, 0, 0, -2},
     "NNNNNNYYY",
     0,
     "YNNNNNNNYNYNNNNNNNNNNNNN",
     10,
     1},
    {RACE_REVENANT,
     57,
     "revenant",
     "Revenant",
     "Rvnt",
     RACE_TYPE_UNDEAD,
     SIZE_LARGE,
     10,
     999999999,
     2,
     {7, 7, -2, 0, 6, -2},
     "NNNNNNYYY",
     0,
     "YNNNNNNNYNNNNYNNNNNNNNNN",
     10,
     1},
    {RACE_SHADOW_BEAST,
     58,
     "shadowbeast",
     "Shadow Beast",
     "SBst",
     RACE_TYPE_UNDEAD,
     SIZE_MEDIUM,
     10,
     999999999,
     2,
     {0, 7, 3, -2, 10, -2},
     "NNNNNNYYY",
     0,
     "NNNNYNNNYNNNNNNNNNYNNNNN",
     10,
     1},
    {RACE_PHANTOM,
     59,
     "phantom",
     "Phantom",
     "Phnt",
     RACE_TYPE_UNDEAD,
     SIZE_MEDIUM,
     10,
     999999999,
     2,
     {-2, 5, 10, 0, 3, 0},
     "NNNNNNYYY",
     0,
     "YNNNNNNNNNYNNNNNNNNNNNNN",
     10,
     1},
};

static const struct duris_feat_expectation duris_descend_form_feats[] = {
    {RACE_DEATH_KNIGHT, FEAT_ARMOR_SKIN, 1, 5},
    {RACE_DEATH_KNIGHT, FEAT_VITAL, 1, 1},
    {RACE_DEATH_KNIGHT, FEAT_HARDY, 1, 1},
    {RACE_DEATH_KNIGHT, FEAT_TOUGHNESS, 1, 1},
    {RACE_DEATH_KNIGHT, FEAT_DAMAGE_REDUCTION, 1, 3},
    {RACE_DEATH_KNIGHT, FEAT_FAST_HEALING, 1, 1},
    {RACE_DEATH_KNIGHT, FEAT_GREATSWORD_MASTERY, 1, 1},
    {RACE_DEATH_KNIGHT, FEAT_ULTRAVISION, 1, 1},
    {RACE_DEATH_KNIGHT, FEAT_TIEFLING_HELLISH_RESISTANCE, 1, 1},
    {RACE_DEATH_KNIGHT, FEAT_UNDEAD_FEALTY, 1, 1},
    {RACE_DEATH_KNIGHT, FEAT_SUN_VULNERABILITY, 1, 1},
    {RACE_DEATH_KNIGHT, FEAT_SLOW_CASTING, 1, 5},
    {RACE_DEATH_KNIGHT, FEAT_SLA_FIRE_STORM, 13, 1},
    {RACE_DEATH_KNIGHT, FEAT_SLA_FIRE_SHIELD, 16, 1},
    {RACE_DEATH_KNIGHT, FEAT_SACRILEGIOUS_POWER, 23, 1},
    {RACE_WIGHT, FEAT_ARMOR_SKIN, 1, 5},
    {RACE_WIGHT, FEAT_VITAL, 1, 1},
    {RACE_WIGHT, FEAT_HARDY, 1, 1},
    {RACE_WIGHT, FEAT_TOUGHNESS, 1, 1},
    {RACE_WIGHT, FEAT_DAMAGE_REDUCTION, 1, 3},
    {RACE_WIGHT, FEAT_FAST_HEALING, 1, 1},
    {RACE_WIGHT, FEAT_COLD_IMMUNITY, 1, 1},
    {RACE_WIGHT, FEAT_ULTRAVISION, 1, 1},
    {RACE_WIGHT, FEAT_BODYSLAM, 1, 1},
    {RACE_WIGHT, FEAT_DOORBASH, 1, 1},
    {RACE_WIGHT, FEAT_WEAKNESS_TO_FIRE, 1, 1},
    {RACE_WIGHT, FEAT_SLOW_CASTING, 1, 9},
    {RACE_WIGHT, FEAT_SLA_FROST_BREATH, 6, 1},
    {RACE_WIGHT, FEAT_SLA_STONESKIN, 13, 1},
    {RACE_REVENANT, FEAT_ARMOR_SKIN, 1, 5},
    {RACE_REVENANT, FEAT_VITAL, 1, 1},
    {RACE_REVENANT, FEAT_HARDY, 1, 1},
    {RACE_REVENANT, FEAT_TOUGHNESS, 1, 1},
    {RACE_REVENANT, FEAT_DAMAGE_REDUCTION, 1, 3},
    {RACE_REVENANT, FEAT_FAST_HEALING, 1, 1},
    {RACE_REVENANT, FEAT_ULTRAVISION, 1, 1},
    {RACE_REVENANT, FEAT_TROLL_REGENERATION, 1, 1},
    {RACE_REVENANT, FEAT_BODYSLAM, 1, 1},
    {RACE_REVENANT, FEAT_DOORBASH, 1, 1},
    {RACE_REVENANT, FEAT_WEAKNESS_TO_FIRE, 1, 1},
    {RACE_REVENANT, FEAT_SLOW_CASTING, 1, 3},
    {RACE_REVENANT, FEAT_BATTLE_FRENZY, 8, 1},
    {RACE_REVENANT, FEAT_SLA_SHADOW_JUMP, 13, 1},
    {RACE_SHADOW_BEAST, FEAT_ARMOR_SKIN, 1, 5},
    {RACE_SHADOW_BEAST, FEAT_VITAL, 1, 1},
    {RACE_SHADOW_BEAST, FEAT_HARDY, 1, 1},
    {RACE_SHADOW_BEAST, FEAT_TOUGHNESS, 1, 1},
    {RACE_SHADOW_BEAST, FEAT_DAMAGE_REDUCTION, 1, 3},
    {RACE_SHADOW_BEAST, FEAT_FAST_HEALING, 1, 1},
    {RACE_SHADOW_BEAST, FEAT_ULTRAVISION, 1, 1},
    {RACE_SHADOW_BEAST, FEAT_UNDERDARK_STEALTH, 1, 1},
    {RACE_SHADOW_BEAST, FEAT_SLA_STRENGTH, 1, 1},
    {RACE_SHADOW_BEAST, FEAT_SLA_ENLARGE, 1, 1},
    {RACE_SHADOW_BEAST, FEAT_WEAKNESS_TO_FIRE, 1, 1},
    {RACE_SHADOW_BEAST, FEAT_SLOW_CASTING, 1, 1},
    {RACE_SHADOW_BEAST, FEAT_RACIAL_FLURRY, 18, 1},
    {RACE_PHANTOM, FEAT_ARMOR_SKIN, 1, 5},
    {RACE_PHANTOM, FEAT_VITAL, 1, 1},
    {RACE_PHANTOM, FEAT_HARDY, 1, 1},
    {RACE_PHANTOM, FEAT_TOUGHNESS, 1, 1},
    {RACE_PHANTOM, FEAT_DAMAGE_REDUCTION, 1, 3},
    {RACE_PHANTOM, FEAT_FAST_HEALING, 1, 1},
    {RACE_PHANTOM, FEAT_ULTRAVISION, 1, 1},
    {RACE_PHANTOM, FEAT_HALF_DROW_SPELL_RESISTANCE, 1, 1},
    {RACE_PHANTOM, FEAT_SLA_PLANE_SHIFT, 1, 1},
    {RACE_PHANTOM, FEAT_ENHANCED_SPELL_DAMAGE, 1, 1},
    {RACE_PHANTOM, FEAT_EYELESS, 1, 1},
    {RACE_PHANTOM, FEAT_WEAKNESS_TO_FIRE, 1, 1},
    {RACE_PHANTOM, FEAT_FAST_CASTING, 1, 3},
    {RACE_PHANTOM, FEAT_VAMPIRE_GASEOUS_FORM, 10, 1},
    {RACE_PHANTOM, FEAT_WINGS, 11, 1},
    {RACE_PHANTOM, FEAT_SPELL_ABSORB, 11, 1},
};

#define DURIS_DESCEND_FORM_COUNT                                                                   \
  ((int)(sizeof(duris_descend_forms) / sizeof(duris_descend_forms[0])))

/* The forms' registry data, hit points, experience, undead family, and feats. */
void TestDescendFormsRegistryAndFeats(CuTest *tc)
{
  const struct duris_race_expectation *expected;
  struct char_data ch;
  struct char_data *character = &ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  long human_exp = 0;
  int human_hp = 0;
  int old_multiplier = CONFIG_EXPERIENCE_MULTIPLIER;
  int race = 0;
  int i = 0;
  int j = 0;

  ensure_race_equivalence_registry();
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  GET_LEVEL(&ch) = 10;
  GET_REAL_CON(&ch) = 10;
  ch.aff_abils.con = 10;
  GET_CLASS(&ch) = CLASS_WARRIOR;
  CONFIG_EXPERIENCE_MULTIPLIER = 100;
  GET_REAL_RACE(&ch) = RACE_HUMAN;
  calculate_max_hp(&ch, FALSE);
  human_hp = GET_MAX_HIT(&ch);
  human_exp = level_exp(&ch, 10);

  for (i = 0; i < DURIS_DESCEND_FORM_COUNT; i++)
  {
    expected = &duris_descend_forms[i];
    race = expected->race;
    CuAssertIntEquals(tc, expected->id, race);
    CuAssertTrue(tc, race_list[race].is_pc);
    CuAssertTrue(tc, race_is_transformation_only(race));
    CuAssertTrue(tc, !race_is_creation_eligible(race));
    CuAssertTrue(tc, valid_luminari_race(race));
    CuAssertStrEquals(tc, expected->name, race_list[race].name);
    CuAssertStrEquals(tc, expected->type, race_list[race].type);
    CuAssertStrEquals(tc, expected->abbrev, race_list[race].abbrev);
    CuAssertPtrNotNull(tc, race_list[race].descrip);
    CuAssertIntEquals(tc, expected->family, race_list[race].family);
    CuAssertIntEquals(tc, expected->size, race_list[race].size);
    CuAssertIntEquals(tc, expected->level_adjustment, race_list[race].level_adjustment);
    CuAssertIntEquals(tc, expected->unlock_cost, race_list[race].unlock_cost);
    CuAssertIntEquals(tc, expected->tier, race_list[race].epic_adv);
    CuAssertIntEquals(tc, expected->language, race_list[race].racial_language);
    for (j = 0; j < 6; j++)
      CuAssertIntEquals(tc, expected->stats[j], get_race_stat(race, j));
    for (j = 0; j < NUM_ALIGNMENTS; j++)
      CuAssertIntEquals(tc, expected->alignments[j] == 'Y', race_list[race].alignments[j]);
    for (j = 0; j < NUM_ATTACK_TYPES; j++)
      CuAssertIntEquals(tc, expected->attacks[j] == 'Y', race_list[race].attack_types[j]);
    CuAssertIntEquals(tc, race, parse_race_long(race_list[race].name));
    CuAssertIntEquals(tc, race, parse_race_long(race_list[race].type));

    GET_REAL_RACE(&ch) = race;
    calculate_max_hp(&ch, FALSE);
    CuAssertIntEquals(tc, human_hp + 10 + 10 * expected->hp_per_level, GET_MAX_HIT(&ch));
    CuAssertIntEquals(tc, 10, race_starting_hp_bonus(race));
    CuAssertTrue(tc, human_exp * expected->xp_multiplier == level_exp(&ch, 10));
    CuAssertTrue(tc, IS_DESCEND_FORM(character));
    CuAssertTrue(tc, IS_UNDEAD(character));
    CuAssertTrue(tc, !IS_HUMANOID(character));

    assert_race_feat_grants(
        tc, race, duris_descend_form_feats,
        (int)(sizeof(duris_descend_form_feats) / sizeof(duris_descend_form_feats[0])));
  }
  CONFIG_EXPERIENCE_MULTIPLIER = old_multiplier;

  CuAssertIntEquals(tc, RACE_DEATH_KNIGHT, parse_race_long("death-knight"));
  CuAssertIntEquals(tc, RACE_SHADOW_BEAST, parse_race_long("shadow-beast"));
}

/* A forged unlock cannot buy, list, or select a form in terminal creation or the account shop. */
void TestDescendFormsHardLock(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  char input[MAX_INPUT_LENGTH];
  int i = 0;

  ensure_race_equivalence_registry();
  for (i = 0; i < DURIS_DESCEND_FORM_COUNT; i++)
  {
    init_race_equivalence_character(&ch, &specials, &descriptor, &account);
    account.races[0] = duris_descend_forms[i].race;
    CuAssertTrue(tc, !has_unlocked_race(&ch, duris_descend_forms[i].race));
    CuAssertTrue(tc, !race_is_selectable_for_creation(&ch, duris_descend_forms[i].race));
    CuAssertStrEquals(tc, "race/fallback",
                      web_onboarding_race_media_key(duris_descend_forms[i].race));

    descriptor.pProtocol = ProtocolCreate();
    CuAssertPtrNotNull(tc, descriptor.pProtocol);
    if (descriptor.pProtocol == NULL)
      return;
    GET_REAL_RACE(&ch) = RACE_UNDEFINED;
    STATE(&descriptor) = CON_QRACE;
    snprintf(input, sizeof(input), "%s", race_list[duris_descend_forms[i].race].type);
    nanny(&descriptor, input);
    CuAssertIntEquals(tc, RACE_UNDEFINED, GET_REAL_RACE(&ch));
    CuAssertIntEquals(tc, CON_QRACE, STATE(&descriptor));
    CuAssertPtrNotNull(tc, strstr(descriptor.output, "cannot be selected"));
    cleanup_race_equivalence_descriptor(&descriptor);

    init_race_equivalence_character(&ch, &specials, &descriptor, &account);
    descriptor.pProtocol = ProtocolCreate();
    CuAssertPtrNotNull(tc, descriptor.pProtocol);
    if (descriptor.pProtocol == NULL)
      return;
    account.experience = 100000000;
    snprintf(input, sizeof(input), "race %s", race_list[duris_descend_forms[i].race].type);
    do_accexp(&ch, input, 0, 0);
    CuAssertIntEquals(tc, 0, account.races[0]);
    CuAssertIntEquals(tc, 100000000, account.experience);
    cleanup_race_equivalence_descriptor(&descriptor);
  }

  /* the terminal race menu leaves the forms out even with every one forged */
  init_race_equivalence_character(&ch, &specials, &descriptor, &account);
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  if (descriptor.pProtocol == NULL)
    return;
  for (i = 0; i < DURIS_DESCEND_FORM_COUNT; i++)
    account.races[i] = duris_descend_forms[i].race;
  STATE(&descriptor) = CON_QSEX;
  snprintf(input, sizeof(input), "m");
  nanny(&descriptor, input);
  CuAssertIntEquals(tc, CON_QRACE, STATE(&descriptor));
  for (i = 0; i < DURIS_DESCEND_FORM_COUNT; i++)
    CuAssertTrue(tc,
                 strstr(descriptor.output, race_list[duris_descend_forms[i].race].type) == NULL);
  cleanup_race_equivalence_descriptor(&descriptor);
}

struct descend_quest_fixture
{
  struct aq_data quests[2];
  struct aq_data *saved_quests;
  qst_rnum saved_count;
};

/* Quest 701 rewards 100 gold and the race, and leads to 702 when next is 702. */
static void begin_descend_quest(struct descend_quest_fixture *fixture, struct char_data *ch,
                                int race, qst_vnum next)
{
  memset(fixture->quests, 0, sizeof(fixture->quests));
  fixture->saved_quests = aquest_table;
  fixture->saved_count = total_quests;
  aquest_table = fixture->quests;
  total_quests = 2;
  fixture->quests[0].vnum = 701;
  fixture->quests[0].done = CuMutableString("The rite is complete.");
  fixture->quests[0].follower_reward = NOBODY;
  fixture->quests[0].obj_reward = NOTHING;
  fixture->quests[0].race_reward = race;
  fixture->quests[0].next_quest = next;
  fixture->quests[0].gold_reward = 100;
  fixture->quests[1].vnum = 702;
  fixture->quests[1].info = CuMutableString("The next rite awaits.");
  fixture->quests[1].follower_reward = NOBODY;
  fixture->quests[1].obj_reward = NOTHING;
  fixture->quests[1].race_reward = RACE_UNDEFINED;
  fixture->quests[1].next_quest = NOTHING;
  GET_QUEST(ch, 0) = fixture->quests[0].vnum;
  GET_QUEST_COUNTER(ch, 0) = 0;
}

static void end_descend_quest(struct descend_quest_fixture *fixture)
{
  aquest_table = fixture->saved_quests;
  total_quests = fixture->saved_count;
}

/* A level 30 Human with warrior levels, ready for a Wight conversion. */
static void make_descend_candidate(struct char_data *ch)
{
  GET_REAL_RACE(ch) = RACE_HUMAN;
  GET_REAL_SIZE(ch) = SIZE_MEDIUM;
  GET_LEVEL(ch) = 30;
  GET_CLASS(ch) = CLASS_WARRIOR;
  CLASS_LEVEL(ch, CLASS_WARRIOR) = 30;
  GET_EXP(ch) = 5000;
  GET_ALIGNMENT(ch) = 0;
  GET_GOLD(ch) = 0;
}

/* Every refused preflight leaves the character, the quest, and its rewards untouched. */
void TestDescendFormConversionPreflightChangesNothing(CuTest *tc)
{
  const struct
  {
    int real_race;
    int level;
    int class_num;
    int target;
    const char *message;
  } cases[] = {
      {RACE_HUMAN, 29, CLASS_WARRIOR, RACE_WIGHT, "level 30"},
      {RACE_HUMAN, 30, CLASS_WIZARD, RACE_WIGHT, "Only a character with levels in warrior"},
      {RACE_HUMAN, 30, CLASS_WARRIOR, RACE_PHANTOM, "can become a Phantom"},
      {RACE_LICH, 30, CLASS_WARRIOR, RACE_WIGHT, "already been transformed"},
      {RACE_REVENANT, 30, CLASS_WARRIOR, RACE_WIGHT, "already been transformed"},
      /* the one-way rule holds for the Lich and Vampire rewards too */
      {RACE_WIGHT, 30, CLASS_WARRIOR, RACE_LICH, "already been transformed"},
      {RACE_WIGHT, 30, CLASS_WARRIOR, RACE_VAMPIRE, "already been transformed"},
      {RACE_LICH, 30, CLASS_WIZARD, RACE_VAMPIRE, "already been transformed"},
      {RACE_VAMPIRE, 30, CLASS_WARRIOR, RACE_LICH, "already been transformed"},
  };
  struct descend_quest_fixture quest;
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  bool refused = FALSE;
  size_t i = 0;

  ensure_descend_form_registry();
  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
  {
    init_race_equivalence_character(&ch, &specials, &descriptor, &account);
    descriptor.pProtocol = ProtocolCreate();
    CuAssertPtrNotNull(tc, descriptor.pProtocol);
    if (descriptor.pProtocol == NULL)
      return;
    GET_PFILEPOS(&ch) = -1;
    ch.player.name = CuMutableString("descend preflight");
    make_descend_candidate(&ch);
    GET_REAL_RACE(&ch) = cases[i].real_race;
    GET_LEVEL(&ch) = cases[i].level;
    CLASS_LEVEL((&ch), CLASS_WARRIOR) = 0;
    GET_CLASS(&ch) = cases[i].class_num;
    CLASS_LEVEL((&ch), cases[i].class_num) = cases[i].level;
    begin_descend_quest(&quest, &ch, cases[i].target, NOTHING);

    complete_quest(&ch, 0);
    end_descend_quest(&quest);
    refused = strstr(descriptor.output, cases[i].message) != NULL;
    cleanup_race_equivalence_descriptor(&descriptor);

    CuAssertTrue(tc, refused);
    CuAssertIntEquals(tc, cases[i].real_race, GET_REAL_RACE(&ch));
    CuAssertIntEquals(tc, cases[i].class_num, GET_CLASS(&ch));
    CuAssertIntEquals(tc, cases[i].level, GET_LEVEL(&ch));
    CuAssertTrue(tc, GET_EXP(&ch) == 5000);
    CuAssertIntEquals(tc, 0, GET_ALIGNMENT(&ch));
    CuAssertIntEquals(tc, 0, GET_GOLD(&ch));
    CuAssertIntEquals(tc, 701, GET_QUEST(&ch, 0));
    CuAssertIntEquals(tc, 0, GET_NUM_QUESTS(&ch));
  }
}

/* The quest converts a level 30 warrior into a level one Wight warrior with no experience, evil,
 * Large, and holding its level-one feats and DR 9, which a save and reload keep; a second
 * conversion is refused. */
void TestDescendFormConversionSucceedsAndSurvivesReload(CuTest *tc)
{
  struct descend_quest_fixture quest;
  struct player_index_element fixture_index[1];
  struct player_index_element *saved_player_table;
  struct char_data *source;
  struct char_data *loaded;
  char temporary_directory[] = "/tmp/luminari-descend-fixture-XXXXXX";
  char original_directory[PATH_MAX];
  char filename[MAX_FILEPATH];
  char player_name[32];
  int saved_top_of_p_table;
  int saved_move_gain;
  int converted_race, converted_class, converted_class_level, converted_level;
  int converted_alignment, converted_size, converted_armor_skin, converted_cold;
  int converted_dr_count, converted_dr = 0, converted_gold;
  long converted_exp;
  bool converted_quest_done;
  int loaded_race = -1, loaded_class = -1, loaded_alignment = 0, loaded_size = -1;
  int loaded_armor_skin = -1, loaded_dr_count = -1, loaded_dr = 0;
  long loaded_exp = -1;
  bool loaded_quest_done = FALSE;
  int load_result = -1;
  const int second_targets[] = {RACE_REVENANT, RACE_LICH, RACE_VAMPIRE};
  bool repeats_refused = TRUE;
  int restore_result;
  size_t i;

  ensure_descend_form_registry();
  memset(fixture_index, 0, sizeof(fixture_index));
  CuAssertPtrNotNull(tc, getcwd(original_directory, sizeof(original_directory)));
  enter_race_player_fixture(tc, temporary_directory);
  source = new_char();
  loaded = new_char();
  snprintf(player_name, sizeof(player_name), "Zzdk%ld", (long)getpid());
  fixture_index[0].name = player_name;
  fixture_index[0].id = 4244;
  fixture_index[0].level = 30;
  saved_player_table = player_table;
  saved_top_of_p_table = top_of_p_table;
  player_table = fixture_index;
  top_of_p_table = 0;

  source->player.name = strdup(player_name);
  GET_PFILEPOS(source) = 0;
  GET_IDNUM(source) = 4244;
  make_descend_candidate(source);
  saved_move_gain = class_list[CLASS_WARRIOR].move_gain;
  class_list[CLASS_WARRIOR].move_gain = 1;

  begin_descend_quest(&quest, source, RACE_WIGHT, 702);
  complete_quest(source, 0);
  end_descend_quest(&quest);

  converted_race = GET_REAL_RACE(source);
  converted_class = GET_CLASS(source);
  converted_class_level = CLASS_LEVEL(source, CLASS_WARRIOR);
  converted_level = GET_LEVEL(source);
  converted_exp = GET_EXP(source);
  converted_alignment = GET_ALIGNMENT(source);
  converted_size = GET_REAL_SIZE(source);
  converted_armor_skin = HAS_REAL_FEAT(source, FEAT_ARMOR_SKIN);
  converted_cold = HAS_REAL_FEAT(source, FEAT_COLD_IMMUNITY);
  converted_dr_count = count_feat_damage_reduction(source, &converted_dr);
  converted_gold = GET_GOLD(source);
  converted_quest_done = is_complete(source, 701) && GET_QUEST(source, 0) == 702;

  if (get_filename(filename, sizeof(filename), PLR_FILE, player_name))
  {
    load_result = load_char(player_name, loaded);
    if (load_result >= 0)
    {
      loaded_race = GET_REAL_RACE(loaded);
      loaded_class = GET_CLASS(loaded);
      loaded_exp = GET_EXP(loaded);
      loaded_alignment = GET_ALIGNMENT(loaded);
      loaded_size = GET_REAL_SIZE(loaded);
      loaded_armor_skin = HAS_REAL_FEAT(loaded, FEAT_ARMOR_SKIN);
      loaded_dr_count = count_feat_damage_reduction(loaded, &loaded_dr);
      /* the conversion was saved with its quest history and next stage */
      loaded_quest_done =
          is_complete(loaded, 701) && GET_NUM_QUESTS(loaded) == 1 && GET_QUEST(loaded, 0) == 702;
    }

    /* no other race reward takes a form, even at level 30 with warrior levels, and a refusal
     * spends nothing: race, experience, gold, and the quest slot stay as they were */
    for (i = 0; i < sizeof(second_targets) / sizeof(second_targets[0]); i++)
    {
      GET_LEVEL(source) = 30;
      CLASS_LEVEL(source, CLASS_WARRIOR) = 30;
      GET_EXP(source) = 777;
      begin_descend_quest(&quest, source, second_targets[i], NOTHING);
      complete_quest(source, 0);
      end_descend_quest(&quest);
      if (GET_REAL_RACE(source) != RACE_WIGHT || GET_EXP(source) != 777 ||
          GET_GOLD(source) != 100 || GET_QUEST(source, 0) != 701 || GET_NUM_QUESTS(source) != 1)
        repeats_refused = FALSE;
    }
    unlink(filename);
  }
  class_list[CLASS_WARRIOR].move_gain = saved_move_gain;

  restore_result = leave_race_player_fixture(original_directory, temporary_directory);
  free_char(loaded);
  free_char(source);
  player_table = saved_player_table;
  top_of_p_table = saved_top_of_p_table;

  CuAssertIntEquals(tc, 0, restore_result);
  CuAssertIntEquals(tc, RACE_WIGHT, converted_race);
  CuAssertIntEquals(tc, CLASS_WARRIOR, converted_class);
  CuAssertIntEquals(tc, 1, converted_class_level);
  CuAssertIntEquals(tc, 1, converted_level);
  CuAssertTrue(tc, converted_exp == 0);
  CuAssertIntEquals(tc, -1000, converted_alignment);
  CuAssertIntEquals(tc, SIZE_LARGE, converted_size);
  CuAssertIntEquals(tc, 5, converted_armor_skin);
  CuAssertIntEquals(tc, 1, converted_cold);
  CuAssertIntEquals(tc, 1, converted_dr_count);
  CuAssertIntEquals(tc, 9, converted_dr);
  CuAssertIntEquals(tc, 100, converted_gold);
  CuAssertTrue(tc, converted_quest_done);
  CuAssertTrue(tc, load_result >= 0);
  CuAssertIntEquals(tc, RACE_WIGHT, loaded_race);
  CuAssertIntEquals(tc, CLASS_WARRIOR, loaded_class);
  CuAssertTrue(tc, loaded_exp == 0);
  CuAssertIntEquals(tc, -1000, loaded_alignment);
  CuAssertIntEquals(tc, SIZE_LARGE, loaded_size);
  CuAssertIntEquals(tc, 5, loaded_armor_skin);
  CuAssertIntEquals(tc, 1, loaded_dr_count);
  CuAssertIntEquals(tc, 9, loaded_dr);
  CuAssertTrue(tc, loaded_quest_done);
  CuAssertTrue(tc, repeats_refused);
}

/* The legacy Lich quest refuses a character that is already transformation-only, returning its
 * offerings, so a descend form cannot become a Lich there either. */
void TestLegacyLichQuestRefusesATransformedCharacter(CuTest *tc)
{
  struct char_data ch;
  struct char_data questor;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  struct quest_command payment;
  struct quest_command kit;
  struct quest_entry quest;
  const int transformed[] = {RACE_WIGHT, RACE_PHANTOM, RACE_VAMPIRE, RACE_LICH};
  bool refused = FALSE;
  size_t i;

  ensure_descend_form_registry();
  for (i = 0; i < sizeof(transformed) / sizeof(transformed[0]); i++)
  {
    init_race_equivalence_character(&ch, &specials, &descriptor, &account);
    descriptor.pProtocol = ProtocolCreate();
    CuAssertPtrNotNull(tc, descriptor.pProtocol);
    if (descriptor.pProtocol == NULL)
      return;
    STATE(&descriptor) = CON_PLAYING;
    ch.player.name = CuMutableString("legacy lich candidate");
    GET_PFILEPOS(&ch) = -1;
    GET_REAL_RACE(&ch) = transformed[i];
    GET_LEVEL(&ch) = 30;
    GET_EXP(&ch) = 777;
    GET_POS(&ch) = POS_STANDING;

    clear_char(&questor);
    SET_BIT_AR(MOB_FLAGS(&questor), MOB_ISNPC);
    memset(&payment, 0, sizeof(payment));
    memset(&kit, 0, sizeof(kit));
    memset(&quest, 0, sizeof(quest));
    payment.type = QUEST_COMMAND_COINS;
    kit.type = QUEST_COMMAND_KIT;
    kit.value = 9999; /* LICH_QUEST in src/quest/hlquest.c */
    quest.type = QUEST_GIVE;
    quest.approved = TRUE;
    quest.reply_msg = CuMutableString("The ritual begins.");
    quest.in = &payment;
    quest.out = &kit;
    questor.mob_specials.quest = &quest;

    quest_give(&ch, &questor);
    refused = strstr(descriptor.output, "cannot become a Lich") != NULL &&
              GET_REAL_RACE(&ch) == transformed[i] && GET_EXP(&ch) == 777;
    cleanup_race_equivalence_descriptor(&descriptor);
    CuAssertTrue(tc, refused);
  }
}
