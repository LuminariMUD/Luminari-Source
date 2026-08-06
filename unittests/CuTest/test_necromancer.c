#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/character/feats.h"

#include <string.h>

static void setup_necromancer_character(struct char_data *ch,
                                        struct player_special_data *player_specials)
{
  clear_char(ch);
  memset(player_specials, 0, sizeof(*player_specials));
  ch->player_specials = player_specials;
  GET_CLASS(ch) = CLASS_NECROMANCER;
  GET_LEVEL(ch) = 12;
  CLASS_LEVEL(ch, CLASS_NECROMANCER) = 3;
}

void Test_necromancer_arcane_progression_advances_only_preferred_class(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 5;
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 5;
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 5;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_ARCANE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_SORCERER;
  GET_PREFERRED_DIVINE((&ch)) = CLASS_CLERIC;

  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_WIZARD));
  CuAssertIntEquals(tc, 3, compute_bonus_caster_level(&ch, CLASS_SORCERER));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_CLERIC));
}

void Test_necromancer_divine_progression_advances_only_preferred_class(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 5;
  CLASS_LEVEL((&ch), CLASS_DRUID) = 5;
  CLASS_LEVEL((&ch), CLASS_PALADIN) = 16;
  CLASS_LEVEL((&ch), CLASS_RANGER) = 16;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_DIVINE;
  GET_PREFERRED_DIVINE((&ch)) = CLASS_RANGER;

  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_CLERIC));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_DRUID));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_PALADIN));
  CuAssertIntEquals(tc, 3, compute_bonus_caster_level(&ch, CLASS_RANGER));
}

void Test_necromancer_unselected_progression_advances_no_class(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 5;
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 5;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_NONE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_WIZARD;
  GET_PREFERRED_DIVINE((&ch)) = CLASS_CLERIC;

  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_WIZARD));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_CLERIC));
}

void Test_necromancer_progression_infers_a_single_base_class(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 5;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_ARCANE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_WIZARD;

  CuAssertIntEquals(tc, CLASS_SORCERER,
                    get_necromancer_progression_class(&ch, CASTING_TYPE_ARCANE));
  CuAssertIntEquals(tc, 3, compute_bonus_caster_level(&ch, CLASS_SORCERER));
}

void Test_necromancer_progression_requires_preference_for_multiple_base_classes(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;

  setup_necromancer_character(&ch, &player_specials);
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 5;
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 5;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_ARCANE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_UNDEFINED;

  CuAssertIntEquals(tc, CLASS_UNDEFINED,
                    get_necromancer_progression_class(&ch, CASTING_TYPE_ARCANE));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_WIZARD));
  CuAssertIntEquals(tc, 0, compute_bonus_caster_level(&ch, CLASS_SORCERER));
}

void Test_necromancer_pending_arcane_choice_enables_known_spell_study(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct level_data levelup;

  setup_necromancer_character(&ch, &player_specials);
  memset(&levelup, 0, sizeof(levelup));
  LEVELUP((&ch)) = &levelup;
  LEVELUP((&ch))->class = CLASS_NECROMANCER;
  LEVELUP((&ch))->necromancer_bonus_levels = CASTING_TYPE_ARCANE;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_NONE;
  GET_PREFERRED_ARCANE((&ch)) = CLASS_SORCERER;
  CLASS_LEVEL((&ch), CLASS_SORCERER) = 5;

  CuAssertTrue(tc, can_study_known_spells(&ch));
}

void Test_necromancer_pending_divine_choice_enables_known_spell_study(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct level_data levelup;

  setup_necromancer_character(&ch, &player_specials);
  memset(&levelup, 0, sizeof(levelup));
  LEVELUP((&ch)) = &levelup;
  LEVELUP((&ch))->class = CLASS_NECROMANCER;
  LEVELUP((&ch))->necromancer_bonus_levels = CASTING_TYPE_DIVINE;
  NECROMANCER_CAST_TYPE((&ch)) = CASTING_TYPE_NONE;
  GET_PREFERRED_DIVINE((&ch)) = CLASS_INQUISITOR;
  CLASS_LEVEL((&ch), CLASS_INQUISITOR) = 5;

  CuAssertTrue(tc, can_study_known_spells(&ch));
}
