/**************************************************************************
 *  File: magic.c                                      Part of LuminariMUD *
 *  Usage: Low-level functions for magic; spell template code.             *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "class.h"
#include "fight.h"
#include "utils.h"
#include "mud_event.h"
#include "act.h" //perform_wildshapes
#include "mudlim.h"
#include "oasis.h" // mob autoroller
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "feats.h"
#include "race.h"
#include "alchemy.h"
#include "missions.h"
#include "psionics.h"
#include "combat_modes.h"

// external
extern struct raff_node *raff_list;
void save_char_pets(struct char_data *ch);
void set_vampire_spawn_feats(struct char_data *mob);

/* local file scope function prototypes */
static int mag_materials(struct char_data *ch, IDXTYPE item0, IDXTYPE item1,
                         IDXTYPE item2, int extract, int verbose);
static void perform_mag_groups(int level, struct char_data *ch,
                               struct char_data *tch, struct obj_data *obj, int spellnum,
                               int savetype, int casttype);

// Magic Resistance, ch is challenger, vict is resistor, modifier applys to vict

int compute_spell_res(struct char_data *ch, struct char_data *vict, int modifier)
{
  int resist = GET_SPELL_RES(vict);

  // adjustments passed to mag_resistance
  resist += modifier;

  // additional adjustmenets

  if (HAS_FEAT(vict, FEAT_DIAMOND_SOUL))
    resist += 10 + MONK_TYPE(vict);

  if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_DROW_SPELL_RESISTANCE))
    resist += 10 + GET_LEVEL(vict);

  if (HAS_FEAT(vict, FEAT_HALF_DROW_SPELL_RESISTANCE))
  {
    if (GET_LEVEL(vict) >= 30)
      resist = MAX(resist, 15 + GET_LEVEL(vict) / 2);
    else if (GET_LEVEL(vict) >= 20)
      resist = MAX(resist, 10 + GET_LEVEL(vict) / 2);
    else
      resist = MAX(resist, 5 + GET_LEVEL(vict) / 2);
  }

  if (IS_LICH(vict))
    resist += 15 + GET_LEVEL(vict);

  if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_IMPROVED_SPELL_RESISTANCE))
    resist += 2 * HAS_FEAT(vict, FEAT_IMPROVED_SPELL_RESISTANCE);

  if (affected_by_spell(vict, SPELL_PROTECT_FROM_SPELLS))
    resist += 10;

  if (affected_by_spell(vict, SKILL_INNER_FIRE))
    resist += 25;

  if (IS_AFFECTED(vict, AFF_SPELL_RESISTANT))
    resist += 12 + GET_LEVEL(vict);

  if (!IS_NPC(vict) && GET_EQ(vict, WEAR_SHIELD) &&
      HAS_FEAT(vict, FEAT_ARMOR_MASTERY_2))
    resist += 25;

  if (HAS_FEAT(vict, FEAT_FAE_RESISTANCE))
    resist += 15 + GET_LEVEL(vict);
  else if (IS_PIXIE(vict))
    resist += 15;

  if (IS_DRAGON(vict))
    resist += 25;

  return MIN(99, MAX(0, resist));
}

// TRUE = reisted
// FALSE = Failed to resist
int mag_resistance(struct char_data *ch, struct char_data *vict, int modifier)
{

  if (HAS_FEAT(vict, FEAT_IRON_GOLEM_IMMUNITY))
    return TRUE;

  int challenge = d20(ch),
      resist = compute_spell_res(ch, vict, modifier);

  // should be modified - zusuk
  challenge += CASTER_LEVEL(ch);

  if (is_judgement_possible(ch, vict, INQ_JUDGEMENT_PIERCING))
    challenge += get_judgement_bonus(ch, INQ_JUDGEMENT_PIERCING);

  // insert challenge bonuses here (ch)
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_SPELL_PENETRATION))
    challenge += 2;
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_GREATER_SPELL_PENETRATION))
    challenge += 3;
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_EPIC_SPELL_PENETRATION))
    challenge += 4;

  // success?
  if (resist > challenge)
  {
    if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_COMBATROLL))
      send_to_char(vict, "\tW*(Resist:%d>Challenge:%d) You Resist!*\tn", resist, challenge);
    if (ch)
    {
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
        send_to_char(ch, "\tR*(Challenge:%d<Resist:%d) Resisted!*\tn", challenge, resist);
    }
    return TRUE;
  }
  // failed to resist the spell
  return FALSE;
}

/* Saving Throws, ch is challenger, vict is resistor, modifier applys to vict
     using modifier of MAX_GOLD as signal for calculating cap
 */
int compute_mag_saves(struct char_data *vict, int type, int modifier)
{

  int saves = 0;

  /* specific saves and related bonuses/penalties */
  switch (type)
  {
  case SAVING_FORT:
    saves += GET_CON_BONUS(vict);
    if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_GREAT_FORTITUDE))
      saves += 2;
    if (!IS_NPC(vict) && GET_SKILL(vict, SKILL_EPIC_FORTITUDE))
      saves += 3;
    break;
  case SAVING_REFL:
    saves += GET_DEX_BONUS(vict);
    if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_LIGHTNING_REFLEXES))
      saves += 2;
    if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_GRACE))
      saves += 2;
    if (!IS_NPC(vict) && GET_SKILL(vict, SKILL_EPIC_REFLEXES))
      saves += 3;
    break;
  case SAVING_WILL:
    saves += GET_WIS_BONUS(vict);
    if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_IRON_WILL))
      saves += 2;
    if (!IS_NPC(vict) && GET_SKILL(vict, SKILL_EPIC_WILL))
      saves += 3;
    break;
  }

  /* universal bonuses/penalties */
  if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_LUCK_OF_HEROES))
    saves += 1;
  if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_LUCKY))
    saves += 1; /* halfling feat */
  if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_DIVINE_GRACE))
    saves += GET_CHA_BONUS(vict);
  if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_UNHOLY_RESILIENCE))
    saves += GET_CHA_BONUS(vict);
  if (!IS_NPC(vict) && GET_SKILL(vict, SKILL_LUCK_OF_HEROES))
    saves++;
  if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_GRACE))
    saves += GET_CHA_BONUS(vict);
  if (char_has_mud_event(vict, eSPELLBATTLE) && SPELLBATTLE(vict) > 0)
    saves += SPELLBATTLE(vict);
  if (HAS_FEAT(vict, FEAT_SAVES))
    saves += CLASS_LEVEL(vict, CLASS_CLERIC) / 6;
  if (!IS_NPC(vict) && IS_DAYLIT(IN_ROOM(vict)) && HAS_FEAT(vict, FEAT_LIGHT_BLINDNESS))
    saves -= 1;
  if (!IS_NPC(vict) && HAS_FEAT(vict, FEAT_SHADOW_MASTER) && IS_SHADOW_CONDITIONS(vict))
    saves += 2;
  saves -= get_char_affect_modifier(vict, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL);

  /* determine base, add/minus bonus/penalty and return */
  if (IS_NPC(vict))
    saves += (GET_LEVEL(vict) / 3) + 1;
  else
    saves += saving_throws(vict, type);

  /* display mode (used in handler for stat caps) */
  if (modifier == MAX_GOLD)
  {
    ;
  }
  else
  {
    saves += GET_SAVE(vict, type);
    saves += modifier;
  }

  return MIN(99, MAX(saves, 0));
}

// TRUE = resisted
// FALSE = Failed to resist
// modifier applies to victim, higher the better (for the victim)
int mag_savingthrow(struct char_data *ch, struct char_data *vict,
                    int type, int modifier, int casttype, int level, int school)
{
  return mag_savingthrow_full(ch, vict, type, modifier, casttype, level, school, 0);
}

const char *save_names[NUM_SAVINGS] = {"Fort", "Refl", "Will", "Poison", "Death"};
/* TRUE = resisted
   FALSE = Failed to resist
     modifier applies to victim, higher the better (for the victim) */
int mag_savingthrow_full(struct char_data *ch, struct char_data *vict,
                         int type, int modifier, int casttype, int level, int school, int spellnum)
{
  int challenge = 10, // 10 is base DC
      diceroll = d20(vict),
      stat_bonus = 0,
      savethrow = compute_mag_saves(vict, type, modifier) + diceroll;
  struct affected_type *af = NULL;

  if (has_teamwork_feat(vict, FEAT_DUCK_AND_COVER) && type == SAVING_REFL)
    diceroll = MAX(diceroll, d20(vict));
  savethrow = compute_mag_saves(vict, type, modifier) + diceroll;

  if (GET_POS(vict) == POS_DEAD)
    return (FALSE); /* Guess you failed, since you are DEAD. */

  /* compute DC of savingthrow here, note we already have base of 10 */
  switch (casttype)
  {
  case CAST_POTION:
  case CAST_WAND:
  case CAST_STAFF:
  case CAST_SCROLL:
  case CAST_INNATE:
  case CAST_WEAPON_SPELL:
    challenge += level;
    break;
  case CAST_WEAPON_POISON:
    challenge += level;
    if (ch)
    {
      /* weapon poison is going to be the primary equilizer for rogue; so we have given it a nice boost */
      challenge += IS_ROGUE_TYPE(ch) / 5; /* bonus */
      challenge += GET_INT_BONUS(ch);     /* bonus */
    }
    break;
  case CAST_BOMB:
    if (ch)
    {
      challenge += CLASS_LEVEL(ch, CLASS_ALCHEMIST) / 2;
      stat_bonus = GET_INT_BONUS(ch);
      challenge += stat_bonus;
    }
    break;
  case CAST_CRUELTY:
    if (ch)
    {
      challenge += (CLASS_LEVEL(ch, CLASS_BLACKGUARD) / 2);
      stat_bonus = GET_CHA_BONUS(ch);
      challenge += stat_bonus;
    }
    break;
  case CAST_SPELL:
  default:
    if (ch)
    {
      challenge += (DIVINE_LEVEL(ch) + MAGIC_LEVEL(ch)) / 2; /* caster level */

      stat_bonus = GET_WIS_BONUS(ch);
      if (GET_CHA_BONUS(ch) > stat_bonus)
        stat_bonus = GET_CHA_BONUS(ch);
      if (GET_INT_BONUS(ch) > stat_bonus)
        stat_bonus = GET_INT_BONUS(ch);

      challenge += stat_bonus;
    }
    break;
  }

  if (is_spellnum_psionic(spellnum))
  {
    if (ch && HAS_FEAT(ch, FEAT_EPIC_PSIONICS))
    {
      challenge += HAS_FEAT(ch, FEAT_EPIC_PSIONICS) * (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) ? 2 : 1);
    }
  }

  if (ch && IS_UNDEAD(ch) && HAS_FEAT(vict, FEAT_ONE_OF_US))
    challenge += 4;

  if (ch && HAS_FEAT(ch, FEAT_FEY_BLOODLINE_ARCANA) && school == ENCHANTMENT)
    challenge += 2;

  if (ch && HAS_FEAT(ch, FEAT_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_SPELL_FOCUS), school))
  {
    /*deubg*/
    // send_to_char(ch, "Bingo!\r\n");
    challenge += 2;
  }
  if (ch && HAS_FEAT(ch, FEAT_GREATER_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_GREATER_SPELL_FOCUS), school))
  {
    /*deubg*/
    // send_to_char(ch, "Bingo 2!\r\n");
    challenge += 2;
  }
  if (ch && HAS_FEAT(ch, FEAT_EPIC_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_EPIC_SPELL_FOCUS), school))
  {
    /*deubg*/
    // send_to_char(ch, "Bingo 2!\r\n");
    challenge += 3;
  }
  if (ch && !IS_NPC(ch) && GET_SPECIALTY_SCHOOL(ch) == school)
  {
    /*deubg*/
    // send_to_char(ch, "Bingo 3!\r\n");
    challenge += 2;
  }
  if (ch && HAS_REAL_FEAT(ch, FEAT_SCHOOL_POWER) && GET_BLOODLINE_SUBTYPE(ch) == school)
  {
    challenge += 2;
  }

  if (IN_ROOM(vict) != NOWHERE && ROOM_AFFECTED(IN_ROOM(vict), RAFF_SACRED_SPACE) && IS_EVIL(vict))
    challenge += 1;

  if (ch && HAS_FEAT(ch, FEAT_WIZ_DEBUFF) && !rand_number(0, 2))
  {
    send_to_char(ch, "\tW*you flare with magical POWAH!*\tn ");
    challenge += 20;
  }

  if (ch)
    challenge += GET_DC_BONUS(ch);

  if (ch && AFF_FLAGGED(vict, AFF_PROTECT_GOOD) && IS_GOOD(ch))
    savethrow += 2;
  if (ch && AFF_FLAGGED(vict, AFF_PROTECT_EVIL) && IS_EVIL(ch))
    savethrow += 2;
  if (ch && casttype == CAST_WEAPON_POISON)
    savethrow += get_poison_save_mod(ch, vict);
  if (ch && IS_FRIGHTENED(ch))
    savethrow -= 2;
  if (affected_by_aura_of_despair(vict))
    savethrow -= 2;
  if (AFF_FLAGGED(vict, AFF_SICKENED))
    savethrow -= 2;
  if (ch && IS_UNDEAD(ch) && affected_by_spell(vict, SPELL_VEIL_OF_POSITIVE_ENERGY))
    savethrow += 2;
  if (ch && (GET_HIT(ch) * 2) < GET_MAX_HIT(ch) && !IS_NPC(vict) && HAS_FEAT(vict, FEAT_ASTRAL_MAJESTY))
    savethrow += 1;

  // vampire bonuses / penalties for feeding
  challenge += vampire_last_feeding_adjustment(ch);

  // vampire bonuses / penalties for feeding
  savethrow += vampire_last_feeding_adjustment(vict);

  if (has_teamwork_feat(vict, FEAT_PHALANX_FIGHTER))
  {
    if (ch && IS_EVIL(vict) && !IS_EVIL(ch))
      savethrow += has_teamwork_feat(vict, FEAT_PHALANX_FIGHTER);
    else if (ch && !IS_EVIL(vict) && IS_EVIL(ch))
      savethrow += has_teamwork_feat(vict, FEAT_PHALANX_FIGHTER);
  }

  if (has_teamwork_feat(vict, FEAT_SHAKE_IT_OFF))
    savethrow += MIN(4, has_teamwork_feat(vict, FEAT_SHAKE_IT_OFF));

  if (is_judgement_possible(vict, ch, INQ_JUDGEMENT_PURITY))
  {
    savethrow += get_judgement_bonus(vict, INQ_JUDGEMENT_PURITY);
    if (CLASS_LEVEL(vict, CLASS_INQUISITOR) >= 10)
    {
      switch (spellnum)
      {
      case SPELL_POISON:
      case SPELL_POISON_BREATHE:
      case WEAPON_POISON_BLACK_ADDER_VENOM:
      case SPELL_CONTAGION:
      case SPELL_CURSE:
        // bonus is doubled against poisons, curses and diseases at inquisitor level 10
        savethrow += get_judgement_bonus(vict, INQ_JUDGEMENT_PURITY);
        break;
      }
    }
  }

  if (school == ENCHANTMENT)
  {
    if (affected_by_aura_of_depravity(vict))
      savethrow -= 4;
    if (affected_by_aura_of_righteousness(vict))
      savethrow += 4;
  }

  if (type == SAVING_WILL && affected_by_spell(vict, PSIONIC_PSYCHIC_BODYGUARD))
  {
    for (af = ch->affected; af; af = af->next)
    {
      if (af->spell == PSIONIC_PSYCHIC_BODYGUARD && af->location == APPLY_SPECIAL)
      {
        af->modifier--;
        if (af->modifier <= 0)
        {
          affect_from_char(vict, PSIONIC_PSYCHIC_BODYGUARD);
          break;
        }
      }
    }
  }

  if (diceroll != 1 && (savethrow > challenge || diceroll == 20))
  {
    if (diceroll == 20)
    {
      if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_COMBATROLL))
        send_to_char(vict, "\tW*Save Roll Twenty!\tn ");
      if (ch && vict && vict != ch)
      {
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
          send_to_char(ch, "\tR*Save Roll Twenty!\tn ");
      }
    }
    else
    {
      if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_COMBATROLL))
        send_to_char(vict, "\tW*(%s:%d>Challenge:%d) Saved!*\tn ", save_names[type],
                     savethrow, challenge);
      if (ch && vict && vict != ch)
      {
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
          send_to_char(ch, "\tR*(Challenge:%d<%s:%d) Opponent Saved!*\tn ",
                       challenge, save_names[type], savethrow);
      }
    }

    if (HAS_FEAT(vict, FEAT_EATER_OF_MAGIC) && affected_by_spell(vict, SKILL_RAGE))
    {
      GET_HIT(vict) += 2 * CLASS_LEVEL(vict, CLASS_BERSERKER) + 10 +
                       GET_STR_BONUS(vict) + GET_DEX_BONUS(vict) + GET_CON_BONUS(vict);
      send_to_char(vict, "\tWResisting the spell restores some of your vitality!\tn\r\n");
    }

    return (TRUE);
  }

  /* failed! */
  if (diceroll == 1)
  {
    if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_COMBATROLL))
      send_to_char(vict, "\tR*Save Roll One!\tn ");
    if (ch && vict && vict != ch)
    {
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
        send_to_char(ch, "\tW*Save Roll One!\tn ");
    }
  }
  else
  {
    if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_COMBATROLL))
      send_to_char(vict, "\tR*(%s:%d<Challenge:%d) Failed Save!*\tn ", save_names[type],
                   savethrow, challenge);
    if (ch && vict && vict != ch)
    {
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
        send_to_char(ch, "\tW*(Challenge:%d>%s:%d) Opponent Failed Save!*\tn ",
                     challenge, save_names[type], savethrow);
    }
  }
  return (FALSE);
}

/* added this function to add special wear off handling -zusuk */
bool spec_wear_off(struct char_data *ch, int skillnum)
{
  if (skillnum >= TOP_SKILL_DEFINE)
    return FALSE;
  if (skillnum <= 0)
    return FALSE;

  switch (skillnum)
  {
  case SPELL_ANIMAL_SHAPES:
    send_to_char(ch, "As the spell wears off you feel yourself "
                     "transform back to your normal form...\r\n");
    IS_MORPHED(ch) = 0;
    SUBRACE(ch) = 0;
    break;
  default:
    return FALSE;
  }

  /* Sucess! */
  return TRUE;
}

/* added this function to add wear off messages for skills -zusuk
   wondering why i didn't just add this to skillo() or whatnot? */
bool alt_wear_off_msg(struct char_data *ch, int skillnum)
{
  if (skillnum <= SPELL_RESERVED_DBC)
    return FALSE;
  if (skillnum >= MAX_SKILLS)
    return FALSE;

  switch (skillnum)
  {

    /*special spells*/
  case SPELL_PRISMATIC_SPRAY:
    send_to_char(ch, "The effects from the prismatic spray fade away.\r\n");
    break;
  case SPELL_FAERIE_FIRE:
    send_to_char(ch, "The effects from faerie fire fade away.\r\n");
    break;
  case SPELL_WAIL_OF_THE_BANSHEE:
    send_to_char(ch, "The effects from Wail of the Banshee fade away.\r\n");
    break;
  case SPELL_THUNDERCLAP:
    send_to_char(ch, "The effects from the Thunderclap fade away.\r\n");
    break;

  /*skills */
  /* all fallthrough */
  case SKILL_SONG_OF_FOCUSED_MIND:
  case SKILL_SONG_OF_FEAR:
  case SKILL_SONG_OF_ROOTING:
  case SKILL_SONG_OF_THE_MAGI:
  case SKILL_SONG_OF_FLIGHT:
  case SKILL_SONG_OF_HEROISM:
  case SKILL_ORATORY_OF_REJUVENATION:
  case SKILL_ACT_OF_FORGETFULNESS:
  case SKILL_SONG_OF_REVELATION:
  case SKILL_SONG_OF_DRAGONS:
  case SKILL_DANCE_OF_PROTECTION:
    /* we don't send a message here because it fades so often */
    // send_to_char(ch, "The effects from song fade away.\r\n");
    break;
  case ALC_DISC_AFFECT_PSYCHOKINETIC:
    send_to_char(ch, "The effects of the psychokinetic fade away.\r\n");
    break;
  case SKILL_MUTAGEN:
    send_to_char(ch, "The effects of the mutagen fade away.\r\n");
    break;
  case SKILL_SACRED_FLAMES:
    send_to_char(ch, "Your sacred flames fade away.\r\n");
    break;
  case SKILL_CHARGE:
    send_to_char(ch, "You complete your charge.\r\n");
    break;
  case SKILL_RAGE_FATIGUE:
    send_to_char(ch, "You recover from your fatigue.\r\n");
    break;
  case SKILL_FEINT:
    send_to_char(ch, "You are no longer off balance from the feint!\r\n");
    break;
  case SKILL_COME_AND_GET_ME:
    send_to_char(ch, "You no longer SMASH!\r\n");
    break;
  case SKILL_DEFENSIVE_STANCE:
    clear_defensive_stance(ch);
    break;
  case SKILL_RAGE:
    clear_rage(ch);
    break;
  case SKILL_SPELLBATTLE:
    send_to_char(ch, "Your spellbattle has faded...\r\n");
    SPELLBATTLE(ch) = 0;
    break;
  case SKILL_PERFORM:
    send_to_char(ch, "Your bard-song morale has faded...\r\n");
    SONG_AFF_VAL(ch) = 0;
    break;
  case SKILL_DESTRUCTIVE_AURA:
    send_to_char(ch, "Your destructive aura has faded...\r\n");
    break;
  case SKILL_AURA_OF_PROTECTION:
    send_to_char(ch, "Your protective aura has faded...\r\n");
    break;
  case SKILL_CRIP_STRIKE:
    send_to_char(ch, "You have recovered from the crippling strike...\r\n");
    break;
  case SKILL_CRIPPLING_CRITICAL:
    send_to_char(ch, "You have recovered from the crippling critical...\r\n");
    break;
  case SKILL_WILDSHAPE:
    send_to_char(ch, "You are unable to maintain your wildshape and "
                     "transform back to your normal form...\r\n");
    IS_MORPHED(ch) = 0;
    SUBRACE(ch) = 0;
    break;
  case SKILL_DIRT_KICK:
    send_to_char(ch, "Your vision clears.\r\n");
    break;
  default: /* nothing found! */
    return FALSE;
  }

  /* sucess! */
  return TRUE;
}

void rem_room_aff(struct raff_node *raff)
{
  struct raff_node *temp;

  /* this room affection has expired */
  send_to_room(raff->room, "%s", get_wearoff(raff->spell));
  send_to_room(raff->room, "\r\n");

  /* remove the affection */
  REMOVE_BIT(world[(int)raff->room].room_affections, raff->affection);
  REMOVE_FROM_LIST(raff, raff_list, next)
  free(raff);
}

/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
  struct affected_type *af, *next;
  struct char_data *i;
  struct raff_node *raff, *next_raff;

  for (i = character_list; i; i = i->next)
  { /* go through everything */

    for (af = i->affected; af; af = next)
    { /* loop his/her aff list */
      next = af->next;
      if (af->duration >= 1) /* duration > 0, decrement */
        af->duration--;
      else if (af->duration <= -1) /* unlimited duration */
        ;
      else
      { /* affect wore off! */
        /* handle spells/skills (use to just handle spells) */

        if ((af->spell > 0) && (af->spell <= MAX_SPELLS))
        { /*valid spellnum?*/

          /* this is our check to avoid duplicate wear-off messages */
          if (!af->next || (af->next->spell != af->spell) ||
              (af->next->duration > 0))
          {
            /* do we have a built-in spell wear-off message? */
            if (get_wearoff(af->spell))
            {
              send_to_char(i, "%s\r\n", get_wearoff(af->spell));
            }
            /* check for alternative message! (skills) */
            else if (alt_wear_off_msg(i, af->spell))
            {
              ;
            }
            /* check for alternative message! (specs, like morph) */
            else if (spec_wear_off(i, af->spell))
            {
              ;
            }
            else
            {
              /* should not get here, problem! */
              send_to_char(i, "Please send to staff: Missing wear-off message for: (%d)\r\n", af->spell);
            }
          }
        }

        /* ok, finally remove affect */
        affect_remove(i, af);
      }
    }
    update_msdp_affects(i);
  }

  /* update the room affections */
  for (raff = raff_list; raff; raff = next_raff)
  {
    next_raff = raff->next;
    raff->timer--;

    if (raff->timer <= 0)
      rem_room_aff(raff);
  }
}

/* Checks for up to 3 vnums (spell reagents) in the player's inventory. If
 * multiple vnums are passed in, the function ANDs the items together as
 * requirements (ie. if one or more are missing, the spell will not fail).
 * @param ch The caster of the spell.
 * @param item0 The first required item of the spell, NOTHING if not required.
 * @param item1 The second required item of the spell, NOTHING if not required.
 * @param item2 The third required item of the spell, NOTHING if not required.
 * @param extract TRUE if mag_materials should consume (destroy) the items in
 * the players inventory, FALSE if not. Items will only be removed on a
 * successful cast.
 * @param verbose TRUE to provide some generic failure or success messages,
 * FALSE to send no in game messages from this function.
 * @retval int TRUE if ch has all materials to cast the spell, FALSE if not.
 */
static int mag_materials(struct char_data *ch, IDXTYPE item0,
                         IDXTYPE item1, IDXTYPE item2, int extract, int verbose)
{
  /* Begin Local variable definitions. */
  /*------------------------------------------------------------------------*/
  /* Used for object searches. */
  struct obj_data *tobj = NULL;
  /* Points to found reagents. */
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;
  /*------------------------------------------------------------------------*/
  /* End Local variable definitions. */

  /* Begin success checks. Checks must pass to signal a success. */
  /*------------------------------------------------------------------------*/
  /* Check for the objects in the players inventory. */
  for (tobj = ch->carrying; tobj; tobj = tobj->next_content)
  {
    if ((item0 != NOTHING) && (GET_OBJ_VNUM(tobj) == item0))
    {
      obj0 = tobj;
      item0 = NOTHING;
    }
    else if ((item1 != NOTHING) && (GET_OBJ_VNUM(tobj) == item1))
    {
      obj1 = tobj;
      item1 = NOTHING;
    }
    else if ((item2 != NOTHING) && (GET_OBJ_VNUM(tobj) == item2))
    {
      obj2 = tobj;
      item2 = NOTHING;
    }
  }

  /* If we needed items, but didn't find all of them, then the spell is a
   * failure. */
  if ((item0 != NOTHING) || (item1 != NOTHING) || (item2 != NOTHING))
  {
    /* Generic spell failure messages. */
    if (verbose)
    {
      switch (rand_number(0, 2))
      {
      case 0:
        send_to_char(ch, "A wart sprouts on your nose.\r\n");
        break;
      case 1:
        send_to_char(ch, "Your hair falls out in clumps.\r\n");
        break;
      case 2:
        send_to_char(ch, "A huge corn develops on your big toe.\r\n");
        break;
      }
    }
    /* Return fales, the material check has failed. */
    return (FALSE);
  }
  /*------------------------------------------------------------------------*/
  /* End success checks. */

  /* From here on, ch has all required materials in their inventory and the
   * material check will return a success. */

  /* Begin Material Processing. */
  /*------------------------------------------------------------------------*/
  /* Extract (destroy) the materials, if so called for. */
  if (extract)
  {
    if (obj0 != NULL)
      extract_obj(obj0);
    if (obj1 != NULL)
      extract_obj(obj1);
    if (obj2 != NULL)
      extract_obj(obj2);
    /* Generic success messages that signals extracted objects. */
    if (verbose)
    {
      send_to_char(ch, "A puff of smoke rises from your pack.\r\n");
      act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
    }
  }

  /* Don't extract the objects, but signal materials successfully found. */
  if (!extract && verbose)
  {
    send_to_char(ch, "Your pack rumbles.\r\n");
    act("Something rumbles in $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
  }
  /*------------------------------------------------------------------------*/
  /* End Material Processing. */

  /* Signal to calling function that the materials were successfully found
   * and processed. */
  return (TRUE);
}

void mag_loops(int level, struct char_data *ch, struct char_data *victim,
               struct obj_data *wpn, int spellnum, int metamagic, int savetype, int casttype)

{
  int i = 0;
  int num_times = 0;
  bool dam = false, affect = false, points = false, needs_touch = false;

  switch (spellnum)
  {
  case SPELL_BLINDING_RAY:
    num_times = 1 + MIN(3, (level - 3) / 4);
    dam = true;
    affect = true;
    needs_touch = true;
    break;
  case SPELL_MAGIC_MISSILE:
    num_times = MIN(5, (level + 1) / 2);
    dam = true;
    break;
    /*   case SPELL_MAGIC_STONE:
        num_times = MIN(5, (level + 1) / 2);
        dam = true;
        break; */
  }

  for (i = 0; i < num_times; i++)
  {
    if (needs_touch)
      if (attack_roll(ch, victim, ATTACK_TYPE_RANGED, TRUE, 1) < 0) // missed ranged attack roll
        continue;
    if (dam)
      mag_damage(level, ch, victim, wpn, spellnum, metamagic, savetype, casttype);
    if (affect)
      mag_affects(level, ch, victim, wpn, spellnum, metamagic, savetype, casttype);
    if (points)
      mag_points(level, ch, victim, wpn, spellnum, savetype, casttype);
  }
}

// save = -1  ->  you get no save
// default    ->  magic resistance
// returns damage, -1 if dead

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
               struct obj_data *wpn, int spellnum, int metamagic, int savetype, int casttype)
{
  int dam = 0, element = 0, num_dice = 0, save = savetype, size_dice = 0,
      bonus = 0, mag_resist = TRUE, spell_school = NOSCHOOL, save_negates = FALSE, mag_resist_bonus = 0;
  char desc[200];

  if (victim == NULL || ch == NULL)
    return (0);

  spell_school = spell_info[spellnum].schoolOfMagic;

  /* level should be determined in call_magic() */

  /* need to include:
   * 1)  save = SAVING_x   -1 means no saving throw
   * 2)  mag_resist = TRUE/FALSE (TRUE - default, resistable or FALSE - not)
   * 3)  element = DAM_x
   */
  switch (spellnum)
  {

    /*******************************************\
      || ------------ SPECIAL SPELLS ----------- ||
      \*******************************************/

  case SPELL_POISON:
    save = SAVING_FORT;
    if (casttype != CAST_INNATE)
      mag_resist = TRUE;
    element = DAM_POISON;
    num_dice = 1;
    size_dice = 8;
    bonus = 0;
    break;

  case WEAPON_POISON_BLACK_ADDER_VENOM:
    if (!can_poison(victim))
      return 0;
    save = SAVING_FORT;
    element = DAM_POISON;
    num_dice = 3;
    size_dice = 4;
    bonus = 0;
    break;

    /*******************************************\
      || ------------ PSIONIC POWERS ----------- ||
      \*******************************************/

  case PSIONIC_CRYSTAL_SHARD: /* 1st circle */
    if (!attack_roll(ch, victim, ATTACK_TYPE_PSIONICS, TRUE, 0))
    {
      act("A crystal shard fired by $n at $N goes wide.", FALSE, ch, 0, victim, TO_NOTVICT);
      act("A crystal shard fired by $n at YOU goes wide.", FALSE, ch, 0, victim, TO_VICT);
      act("A crystal shard you fired at $N goes wide.", FALSE, ch, 0, victim, TO_CHAR);
      return 0;
    }

    save = -1; // no save
    mag_resist = FALSE;
    element = DAM_PUNCTURE;
    num_dice = 1 + GET_AUGMENT_PSP(ch);
    size_dice = 6;
    bonus = 0;

    break;

  case PSIONIC_MIND_THRUST: /* 1st circle */

    if (is_immune_mind_affecting(ch, victim, TRUE))
      return 0;
    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_MENTAL;
    num_dice = 1 + GET_AUGMENT_PSP(ch);
    size_dice = 10;
    bonus = 0;
    save_negates = TRUE;
    break;

  case PSIONIC_IMPALE_MIND: // Epic

    if (is_immune_mind_affecting(ch, victim, TRUE))
      return 0;
    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_MENTAL;
    num_dice = 40 + GET_AUGMENT_PSP(ch);
    size_dice = 10;
    bonus = 0;
    save_negates = TRUE;
    break;

  case PSIONIC_PSYCHOKINETIC_THRASHING: // Epic
    save = -1;
    mag_resist = TRUE;
    element = DAM_FORCE;
    num_dice = 30 + (GET_AUGMENT_PSP(ch) / 2);
    size_dice = 6;
    bonus = 0;
    break;

  case PSIONIC_RAZOR_STORM: // Epic
    save = -1;              // no save
    mag_resist = FALSE;
    element = DAM_SLICE;
    num_dice = 20 + GET_AUGMENT_PSP(ch);
    size_dice = 4;
    bonus = 0;
    break;

  case PSIONIC_ENERGY_RAY: /* 1st circle */
    if (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_ELECTRIC || GET_PSIONIC_ENERGY_TYPE(ch) == DAM_SOUND)
      GET_TEMP_ATTACK_ROLL_BONUS(ch) = 4;
    if (!attack_roll(ch, victim, ATTACK_TYPE_PSIONICS, TRUE, 0))
    {
      snprintf(desc, sizeof(desc), "$n fires a ray of %s at $N, but it goes wide.", damtypes[GET_PSIONIC_ENERGY_TYPE(ch)]);
      act(desc, FALSE, ch, 0, victim, TO_NOTVICT);
      snprintf(desc, sizeof(desc), "You fire a ray of %s at $N, but it goes wide.", damtypes[GET_PSIONIC_ENERGY_TYPE(ch)]);
      act(desc, FALSE, ch, 0, victim, TO_CHAR);
      snprintf(desc, sizeof(desc), "$n fires a ray of %s at YOU, but it goes wide.", damtypes[GET_PSIONIC_ENERGY_TYPE(ch)]);
      act(desc, FALSE, ch, 0, victim, TO_VICT);
      return 0;
    }

    save = -1; // no save
    mag_resist = TRUE;
    element = GET_PSIONIC_ENERGY_TYPE(ch);
    num_dice = 1 + GET_AUGMENT_PSP(ch);
    size_dice = 6;
    bonus = (GET_PSIONIC_ENERGY_TYPE(ch) != DAM_ELECTRIC && GET_PSIONIC_ENERGY_TYPE(ch) != DAM_SOUND) ? num_dice : 0;

    break;

  case PSIONIC_CONCUSSION_BLAST: /* 2nd circle */ /* AoE */
    save = -1;                                    // no save
    mag_resist = TRUE;
    element = DAM_FORCE;
    num_dice = 1 + (GET_AUGMENT_PSP(ch) / 2);
    size_dice = 6;
    bonus = 0;
    break;

  case PSIONIC_ENERGY_PUSH: /* 2nd circle */
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = GET_PSIONIC_ENERGY_TYPE(ch);
    num_dice = 2 + (GET_AUGMENT_PSP(ch) / 2);
    size_dice = 6;
    bonus = (GET_PSIONIC_ENERGY_TYPE(ch) != DAM_ELECTRIC && GET_PSIONIC_ENERGY_TYPE(ch) != DAM_SOUND) ? num_dice : 0;

    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    GET_DC_BONUS(ch) += (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_ELECTRIC || GET_PSIONIC_ENERGY_TYPE(ch) == DAM_SOUND) ? 2 : 0;
    mag_resist_bonus = (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_ELECTRIC || GET_PSIONIC_ENERGY_TYPE(ch) == DAM_SOUND) ? -2 : 0;
    if (!mag_savingthrow(ch, victim, GET_PSIONIC_ENERGY_TYPE(ch) == DAM_COLD ? SAVING_FORT : SAVING_REFL, 0, casttype, level, NOSCHOOL) &&
        !power_resistance(ch, victim, mag_resist_bonus) && ((GET_SIZE(victim) - GET_SIZE(ch)) <= 1))
    {
      change_position(victim, POS_SITTING);
      act("You have been knocked down!", FALSE, victim, 0, ch, TO_CHAR);
      act("$n is knocked down!", TRUE, victim, 0, ch, TO_ROOM);
      if (!OUTDOORS(victim))
      {
        act("You have been slammed hard against the wall!", FALSE, victim, 0, ch, TO_CHAR);
        act("$n is slammed hard against the wall!", TRUE, victim, 0, ch, TO_ROOM);
        damage(ch, victim, dice(num_dice, size_dice) + bonus, spellnum, DAM_FORCE, FALSE);
      }
    }
    // we do this again because it will have been set to zero in the mag_savingthrow we just called
    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    GET_DC_BONUS(ch) += (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_ELECTRIC || GET_PSIONIC_ENERGY_TYPE(ch) == DAM_SOUND) ? 2 : 0;
    break;

  case PSIONIC_ENERGY_STUN: /* 2nd circle */
    if (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_COLD)
      save = SAVING_FORT;
    else
      save = SAVING_REFL;
    mag_resist = TRUE;
    element = GET_PSIONIC_ENERGY_TYPE(ch);
    num_dice = 1 + GET_AUGMENT_PSP(ch);
    size_dice = 6;
    bonus = (GET_PSIONIC_ENERGY_TYPE(ch) != DAM_ELECTRIC && GET_PSIONIC_ENERGY_TYPE(ch) != DAM_SOUND) ? num_dice : 0;

    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    GET_DC_BONUS(ch) += (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_ELECTRIC || GET_PSIONIC_ENERGY_TYPE(ch) == DAM_SOUND) ? 2 : 0;
    mag_resist_bonus = (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_ELECTRIC || GET_PSIONIC_ENERGY_TYPE(ch) == DAM_SOUND) ? -2 : 0;
    break;

  case PSIONIC_RECALL_AGONY: /* 2nd circle */
    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_TEMPORAL;
    num_dice = 1 + GET_AUGMENT_PSP(ch);
    size_dice = 6;
    bonus = 0;

    break;

  case PSIONIC_SWARM_OF_CRYSTALS: /* 2nd circle */ /* AoE */
    save = -1;                                     // no save
    mag_resist = FALSE;
    element = DAM_SLICE;
    num_dice = 1 + GET_AUGMENT_PSP(ch);
    size_dice = 4;
    bonus = 0;
    break;

  case PSIONIC_ENERGY_BURST: /* 3rd circle */ /* AoE */
    if (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_COLD)
      save = SAVING_FORT;
    else
      save = SAVING_REFL;
    element = GET_PSIONIC_ENERGY_TYPE(ch);
    mag_resist = TRUE;
    num_dice = 5 + GET_AUGMENT_PSP(ch);
    size_dice = 8;
    bonus = (GET_PSIONIC_ENERGY_TYPE(ch) != DAM_ELECTRIC && GET_PSIONIC_ENERGY_TYPE(ch) != DAM_SOUND) ? num_dice : 0;
    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    GET_DC_BONUS(ch) += (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_ELECTRIC || GET_PSIONIC_ENERGY_TYPE(ch) == DAM_SOUND) ? 2 : 0;
    mag_resist_bonus = (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_ELECTRIC || GET_PSIONIC_ENERGY_TYPE(ch) == DAM_SOUND) ? -2 : 0;
    break;

  case PSIONIC_DEADLY_FEAR: /* 4th circle */
    if (is_immune_death_magic(ch, victim, TRUE))
      return (0);
    if (is_immune_mind_affecting(ch, victim, 0))
      return (0);
    if (is_immune_fear(ch, victim, 0))
      return (0);
    // saving throw is handled special below
    bonus = GET_AUGMENT_PSP(ch) / 2;
    if (affected_by_aura_of_cowardice(victim))
      bonus -= 4;
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_MENTAL;
    num_dice = 1;
    size_dice = 1;
    break;

  case PSIONIC_PSYCHIC_CRUSH: /* 5th circle Psi */
    if (is_immune_death_magic(ch, victim, TRUE))
      return (0);
    if (is_immune_mind_affecting(ch, victim, 0))
      return (0);
    // saving throw is handled special below

    bonus = GET_AUGMENT_PSP(ch) / 2;
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_MENTAL;
    num_dice = 1;
    size_dice = 1;
    break;

  case PSIONIC_SHRAPNEL_BURST: /* 5th circle Psi */ /* AoE */
    bonus = 0;
    save = SAVING_REFL;
    mag_resist = FALSE;
    element = DAM_PUNCTURE;
    num_dice = 9 + GET_AUGMENT_PSP(ch) / 2;
    size_dice = 10;
    break;

  case PSIONIC_UPHEAVAL: /* 5th circle Psi */ /* AoE */
    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    bonus = 0;
    save = SAVING_REFL;
    mag_resist = FALSE;
    element = DAM_EARTH;
    num_dice = 10;
    size_dice = 10;
    break;

  case PSIONIC_BREATH_OF_THE_BLACK_DRAGON: /* 6th circle Psi */ /* AoE */
    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    bonus = 0;
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_ACID;
    num_dice = 11 + GET_AUGMENT_PSP(ch) / 2;
    size_dice = 10;

    break;

  case PSIONIC_DISINTEGRATION: /* 6th circle Psi */
    if (!attack_roll(ch, victim, ATTACK_TYPE_PSIONICS, TRUE, 0))
    {
      act("$n fires a ray of disintigration at $N, but it goes wide.", FALSE, ch, 0, victim, TO_NOTVICT);
      act("You fire a ray of disintigration at $N, but it goes wide.", FALSE, ch, 0, victim, TO_CHAR);
      act("$n fires a ray of disintigration at YOU, but it goes wide.", FALSE, ch, 0, victim, TO_VICT);
      return 0;
    }

    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_CHAOS;
    num_dice = 22 + (GET_AUGMENT_PSP(ch) * 2);
    size_dice = 8;
    bonus = 0;

    break;

  case PSIONIC_ULTRABLAST: /* 7th circle Psi */
    if (is_immune_mind_affecting(ch, victim, 0))
      return (0);
    bonus = 14;
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_MENTAL;
    num_dice = 14 + GET_AUGMENT_PSP(ch);
    size_dice = 10;
    break;

  case PSIONIC_PSYCHOSIS: /* 7th circle Psi */ /* AoE */
    if (is_immune_mind_affecting(ch, victim, 0))
      return (0);
    bonus = GET_AUGMENT_PSP(ch) / 2;
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_MENTAL;
    num_dice = 1;
    size_dice = 1;
    break;

  case PSIONIC_RECALL_DEATH: /* 8th circle Psi */
    if (is_immune_death_magic(ch, victim, TRUE))
      return (0);
    if (is_immune_mind_affecting(ch, victim, 0))
      return (0);
    save = SAVING_WILL;
    mag_resist = FALSE;
    element = DAM_MENTAL;
    num_dice = 1;
    size_dice = 1;
    break;

  case PSIONIC_ASSIMILATE: /* 9th circle Psi */
    if (!attack_roll(ch, victim, ATTACK_TYPE_PSIONICS, TRUE, 0))
    {
      act("You attempt to assimilate $N, but $E dodges your attack.", FALSE, ch, 0, victim, TO_CHAR);
      act("$n attempts to assimilate YOU, but you dodge $s attack.", FALSE, ch, 0, victim, TO_VICT);
      act("$n attempts to assimilate $N, but $E dodges $s attack.", FALSE, ch, 0, victim, TO_NOTVICT);
      return 0;
    }
    bonus = 0;
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_CHAOS;
    num_dice = 18;
    size_dice = 12;
    break;

    /*******************************************\
      || ------------- MAGIC SPELLS ------------ ||
      \*******************************************/

  case SPELL_ACID_ARROW: // conjuration
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_ACID;
    num_dice = 4;
    size_dice = 6;
    bonus = 0;
    break;

  case SPELL_BLINDING_RAY: // evocation
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_LIGHT;
    if (victim && HAS_FEAT(victim, FEAT_LIGHT_BLINDNESS))
    {
      num_dice = MIN(5, level / 2);
      size_dice = 4;
    }
    else
    {
      num_dice = 2;
      size_dice = 1;
    }
    bonus = 0;
    break;

  case SPELL_MOONBEAM: // conjuration
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_LIGHT;
    num_dice = 2;
    size_dice = 10;
    bonus = 0;
    break;

  case SPELL_ACID_SPLASH:
    save = SAVING_REFL;
    num_dice = 2;
    size_dice = 3;
    element = DAM_ACID;
    break;

  case SPELL_BALL_OF_LIGHTNING: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_ELECTRIC;
    num_dice = MIN(22, level);
    size_dice = 10;
    bonus = 0;
    break;

  case SPELL_BLIGHT: // evocation
    if (!IS_PLANT(victim))
    {
      send_to_char(ch, "Your blight spell will only effect plant life.\r\n");
      return (0);
    }
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_EARTH;
    num_dice = MIN(level, 15); // maximum 15d6
    size_dice = 6;
    bonus = MIN(level, 15);
    break;

  case SPELL_BURNING_HANDS: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = MIN(8, level);
    size_dice = 6;
    bonus = 0;
    break;

  case SPELL_HELLISH_REBUKE: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = 2 + (level / 10);
    size_dice = 10;
    bonus = 0;
    break;

  case SPELL_CHILL_TOUCH: // necromancy
    // *note chill touch also has an effect, only save on effect
    save = -1;
    mag_resist = TRUE;
    element = DAM_COLD;
    num_dice = 1;
    size_dice = 10;
    bonus = 0;
    break;

  case SPELL_CLENCHED_FIST: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FORCE;
    num_dice = MIN(28, level);
    size_dice = 11;
    bonus = level + 5;

    // 33% chance of causing a wait-state to victim
    if (!rand_number(0, 2))
      attach_mud_event(new_mud_event(eFISTED, victim, NULL), 1000);

    break;

  case SPELL_COLOR_SPRAY: // illusion
    //  has effect too
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_ILLUSION;
    num_dice = 1;
    size_dice = 4;
    bonus = 0;
    break;

  case SPELL_CONE_OF_COLD: // abjuration
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_COLD;
    num_dice = MIN(22, level);
    size_dice = 6;
    bonus = 0;
    break;

  case SPELL_ENERGY_SPHERE: // abjuration
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_ENERGY;
    num_dice = MIN(10, level);
    size_dice = 8;
    bonus = 0;
    break;

  case SPELL_FINGER_OF_DEATH: // necromancy
    if (is_immune_death_magic(ch, victim, TRUE))
      return (0);
    // saving throw is handled special below
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_UNHOLY;
    num_dice = 1;
    size_dice = 1;
    bonus = level * 10;
    break;

  case SPELL_FIREBALL: // evocation
    // Nashak: make this dissipate obscuring mist when finished
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = MIN(15, level);
    size_dice = 10;
    bonus = 0;
    break;

  case SPELL_SEARING_LIGHT: // evocation
    save = -1;
    mag_resist = TRUE;
    element = DAM_LIGHT;
    if (IS_UNDEAD(victim))
    {
      num_dice = MIN(10, level);
      size_dice = 6;
    }
    else if (IS_CONSTRUCT(victim))
    {
      num_dice = MIN(5, level / 2);
      size_dice = 6;
    }
    else
    {
      num_dice = MIN(5, level / 2);
      size_dice = 8;
    }
    bonus = 0;
    break;

  case ABILITY_CHANNEL_POSITIVE_ENERGY:
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_HOLY;
    bonus = 0;
    num_dice = (compute_channel_energy_level(ch) + 1) / 2;
    size_dice = 6;
    if (HAS_FEAT(ch, FEAT_HOLY_CHAMPION))
    {
      num_dice = 1;
      size_dice = 1;
      bonus = ((compute_channel_energy_level(ch) + 1) / 2) * 7;
    }
    else if (HAS_FEAT(ch, FEAT_HOLY_WARRIOR))
    {
      bonus = (compute_channel_energy_level(ch) + 1) / 2;
    }
    break;

  case ABILITY_CHANNEL_NEGATIVE_ENERGY:
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_UNHOLY;
    num_dice = (compute_channel_energy_level(ch) + 1) / 2;
    size_dice = 6;
    bonus = 0;
    if (HAS_FEAT(ch, FEAT_UNHOLY_CHAMPION))
    {
      num_dice = 1;
      size_dice = 1;
      bonus = ((compute_channel_energy_level(ch) + 1) / 2) * 7;
    }
    else if (HAS_FEAT(ch, FEAT_UNHOLY_WARRIOR))
    {
      bonus = (compute_channel_energy_level(ch) + 1) / 2;
    }
    break;

  case SPELL_FIREBRAND: // transmutation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = MIN(22, level);
    size_dice = 6;
    bonus = 0;
    break;

  case SPELL_FLAME_BLADE: // evocation
    if (SECT(ch->in_room) == SECT_UNDERWATER)
    {
      send_to_char(ch, "Your flame blade immediately burns out underwater.");
      return (0);
    }
    save = -1;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = 1;
    size_dice = 8;
    bonus = MIN(level / 2, 10);
    break;

  case SPELL_FREEZING_SPHERE: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_COLD;
    num_dice = MIN(24, level);
    size_dice = 10;
    bonus = level / 2;
    break;

  case SPELL_GRASPING_HAND: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FORCE;
    num_dice = MIN(26, level);
    size_dice = 6;
    bonus = level;
    break;

  case SPELL_GREATER_RUIN: // epic spell
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_PUNCTURE;
    num_dice = level + 6;
    size_dice = 12;
    bonus = level + 35;
    break;

  case SPELL_HORIZIKAULS_BOOM: // evocation
    // *note also has an effect
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_SOUND;
    num_dice = MIN(8, level);
    size_dice = 4;
    bonus = num_dice;
    break;

  case SPELL_ICE_DAGGER: // conjurations
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_COLD;
    num_dice = MIN(7, level);
    size_dice = 8;
    bonus = 0;
    break;

  case SPELL_LIGHTNING_BOLT: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_ELECTRIC;
    num_dice = MIN(15, level);
    size_dice = 10;
    bonus = 0;
    break;

  case SPELL_MAGIC_MISSILE: // evocation
    if (affected_by_spell(victim, SPELL_SHIELD))
    {
      send_to_char(ch, "Your target is shielded by magic!\r\n");
    }
    else
    {
      mag_resist = TRUE;
      save = -1;
      num_dice = 1;
      size_dice = 4;
      bonus = 1;
    }
    element = DAM_FORCE;
    break;

  case SPELL_MISSILE_STORM: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FORCE;
    num_dice = MIN(26, level);
    size_dice = 10;
    bonus = level;
    break;

  case SPELL_LESSER_MISSILE_STORM: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FORCE;
    num_dice = MIN(20, level);
    size_dice = 10;
    bonus = level;
    break;

  case SPELL_NEGATIVE_ENERGY_RAY: // necromancy
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_NEGATIVE;
    num_dice = 2;
    size_dice = 6;
    bonus = 3;
    break;

  case SPELL_NIGHTMARE: // illusion
    // *note nightmare also has an effect, only save on effect
    save = -1;
    mag_resist = TRUE;
    element = DAM_ILLUSION;
    num_dice = level;
    size_dice = 4;
    bonus = 10;
    break;

  case SPELL_POWER_WORD_KILL: // divination
    if (is_immune_death_magic(ch, victim, TRUE))
      return (0);
    save = -1;
    mag_resist = TRUE;
    element = DAM_MENTAL;
    num_dice = 10;
    size_dice = 2;
    if (GET_HIT(victim) <= 121)
      bonus = GET_HIT(victim) + 10;
    else
      bonus = 0;
    break;

  case SPELL_PRODUCE_FLAME: // evocation
    if (SECT(ch->in_room) == SECT_UNDERWATER)
    {
      send_to_char(ch, "You are unable to produce a flame while underwater.");
      return (0);
    }
    save = -1;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = 1;
    size_dice = 6;
    bonus = MIN(level, 5);
    break;

  case SPELL_RAY_OF_FROST:
    save = SAVING_REFL;
    num_dice = 2;
    size_dice = 3;
    element = DAM_COLD;
    break;

  case SPELL_SCORCHING_RAY: // evocation
    save = -1;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = MIN(12, level * 2);
    size_dice = 4;
    bonus = 0;
    break;

  case SPELL_SHOCKING_GRASP: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_ELECTRIC;
    num_dice = MIN(10, level);
    size_dice = 8;
    bonus = 0;
    break;

  case SPELL_TELEKINESIS: // transmutation
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_FORCE;
    num_dice = MIN(20, level);
    size_dice = 4;
    bonus = 0;
    // 60% chance of knockdown, target can't be more than 2 size classes bigger
    if (dice(1, 100) < 60 && (GET_SIZE(ch) + 2) >= GET_SIZE(victim))
    {
      act("Your telekinetic wave knocks $N over!",
          FALSE, ch, 0, victim, TO_CHAR);
      act("The force of the telekinetic slam from $n knocks you over!\r\n",
          FALSE, ch, 0, victim, TO_VICT | TO_SLEEP);
      act("A wave of telekinetic energy originating from $n knocks $N to "
          "the ground!",
          FALSE, ch, 0, victim, TO_NOTVICT);
      change_position(victim, POS_SITTING);
    }
    break;

  case SPELL_VAMPIRIC_TOUCH: // necromancy
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_UNHOLY;
    num_dice = MIN(15, level);
    size_dice = 5;
    bonus = 0;
    break;

  case SPELL_WEIRD: // enchantment
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_ILLUSION;
    num_dice = level;
    size_dice = 12;
    bonus = level + 10;
    break;

  case SPELL_WALL_OF_FIRE: // evocation
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = 2;
    size_dice = 6;
    bonus = level;
    if (bonus <= 10) /* this is both a divine and magic spell */
      bonus = level;
    break;

    /*******************************************\
      || ------------ DIVINE SPELLS ------------ ||
      \*******************************************/

  case SPELL_CALL_LIGHTNING:
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_ELECTRIC;
    num_dice = MIN(24, level);
    size_dice = 8;
    bonus = num_dice + 10;
    break;

  case SPELL_CALL_LIGHTNING_STORM:
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_ELECTRIC;
    num_dice = MIN(24, level);
    size_dice = 12;
    bonus = num_dice + 20;
    break;

  case SPELL_CAUSE_CRITICAL_WOUNDS:
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_HOLY;
    num_dice = 4;
    size_dice = 12;
    bonus = MIN(30, level);
    break;

  case SPELL_CAUSE_LIGHT_WOUNDS:
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_HOLY;
    num_dice = 1;
    size_dice = 12;
    bonus = MIN(7, level);
    break;

  case SPELL_CAUSE_MODERATE_WOUNDS:
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_HOLY;
    num_dice = 2;
    size_dice = 12;
    bonus = MIN(15, level);
    break;

  case SPELL_CAUSE_SERIOUS_WOUNDS:
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_HOLY;
    num_dice = 3;
    size_dice = 12;
    bonus = MIN(22, level);
    break;

  case SPELL_DESTRUCTION:
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_NEGATIVE;
    num_dice = MIN(26, level);
    size_dice = 8;
    bonus = num_dice + level;
    break;

  case SPELL_DISPEL_EVIL:
    if (IS_EVIL(ch))
    {
      victim = ch;
    }
    else if (IS_GOOD(victim))
    {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_HOLY;
    num_dice = level;
    size_dice = 9;
    bonus = level;
    break;

  case SPELL_DISPEL_GOOD:
    if (IS_GOOD(ch))
    {
      victim = ch;
    }
    else if (IS_EVIL(victim))
    {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_UNHOLY;
    num_dice = level;
    size_dice = 9;
    bonus = level;
    break;

  case SPELL_ENERGY_DRAIN:
    //** Magic AND Divine
    if (AFF_FLAGGED(victim, AFF_DEATH_WARD))
    {
      act("$N is warded against death magic.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_NEGATIVE;
    if (GET_LEVEL(victim) < CASTER_LEVEL(ch))
      num_dice = 2;
    else
      num_dice = 1;
    size_dice = 200;
    gain_exp(ch, (dice(2, 200) * 10), GAIN_EXP_MODE_EDRAIN);
    gain_exp(victim, -(dice(2, 200) * 10), GAIN_EXP_MODE_EDRAIN);
    bonus = 0;
    break;

  case SPELL_FLAME_STRIKE:
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = MIN(20, level);
    size_dice = 8;
    bonus = 0;
    break;

  case SPELL_HARM:
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_HOLY;
    num_dice = MIN(22, level);
    size_dice = 8;
    bonus = num_dice;
    break;

  case SPELL_WALL_OF_THORNS: // conjuration
    save = SAVING_FORT;
    mag_resist = FALSE;
    element = DAM_EARTH;
    num_dice = 2;
    size_dice = 6;
    bonus = level;
    break;

    /* trying to keep the AOE together */
    /****************************************\
      || -------- NPC AoE SPELLS ------------ ||
      \****************************************/

  case SPELL_FIRE_BREATHE:
    // AoE
    save = SAVING_REFL;
    mag_resist = FALSE;
    element = DAM_FIRE;
    num_dice = GET_LEVEL(ch);
    size_dice = 16;
    break;

  case SPELL_GAS_BREATHE:
    // AoE
    save = SAVING_REFL;
    mag_resist = FALSE;
    element = DAM_POISON;
    num_dice = GET_LEVEL(ch);
    size_dice = 16;
    break;

  case SPELL_FROST_BREATHE:
    // AoE
    save = SAVING_REFL;
    mag_resist = FALSE;
    element = DAM_COLD;
    num_dice = GET_LEVEL(ch);
    size_dice = 16;
    break;

  case SPELL_LIGHTNING_BREATHE:
    // AoE
    save = SAVING_REFL;
    mag_resist = FALSE;
    element = DAM_ELECTRIC;
    num_dice = GET_LEVEL(ch);
    size_dice = 16;
    break;
  case SPELL_ACID_BREATHE:
    // AoE
    save = SAVING_REFL;
    mag_resist = FALSE;
    element = DAM_ACID;
    num_dice = GET_LEVEL(ch);
    size_dice = 16;
    break;
  case SPELL_POISON_BREATHE:
    // AoE
    if (AFF_FLAGGED(victim, AFF_WIND_WALL))
    {
      send_to_char(ch, "The wall of wind surrounding you blows away the poison cloud.\r\n");
      act("A wall of wind surrounding $n dissipates the poison cloud.", FALSE, ch, 0, 0, TO_ROOM);
      return (0);
    }
    save = SAVING_REFL;
    mag_resist = FALSE;
    element = DAM_POISON;
    num_dice = GET_LEVEL(ch);
    size_dice = 16;
    break;

    /****************************************\
      || --------Magic AoE SPELLS------------ ||
      \****************************************/

  case SPELL_DRACONIC_BLOODLINE_BREATHWEAPON:
    // AoE
    save = SAVING_REFL;
    mag_resist = FALSE;
    element = draconic_heritage_energy_types[GET_BLOODLINE_SUBTYPE(ch)];
    num_dice = CLASS_LEVEL(ch, CLASS_SORCERER);
    size_dice = 6;
    break;

  case SPELL_DRAGONBORN_ANCESTRY_BREATH:
    // AoE
    save = SAVING_REFL;
    mag_resist = FALSE;
    element = draconic_heritage_energy_types[GET_DRAGONBORN_ANCESTRY(ch)];
    num_dice = GET_LEVEL(ch);
    size_dice = 6;
    break;

  case SPELL_ACID: // acid fog (conjuration)
    // AoE
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_ACID;
    num_dice = MAX(6, level);
    size_dice = 2;
    bonus = 10;
    break;

  case SPELL_CHAIN_LIGHTNING: // evocation
    // AoE
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_ELECTRIC;
    num_dice = MIN(28, CASTER_LEVEL(ch));
    size_dice = 9;
    bonus = CASTER_LEVEL(ch);
    break;

  case SPELL_DEATHCLOUD: // cloudkill (conjuration)
    // AoE
    if (AFF_FLAGGED(victim, AFF_WIND_WALL))
    {
      send_to_char(ch, "The wall of wind surrounding you blows away deathly cloud.\r\n");
      act("A wall of wind surrounding $n dissipates the deathly cloud.", FALSE, ch, 0, 0, TO_ROOM);
      return (0);
    }
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_POISON;
    num_dice = level;
    size_dice = 4;
    bonus = 0;
    break;

  case SPELL_FLAMING_SPHERE: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = 2;
    size_dice = 6;
    bonus = 0;
    break;

  case SPELL_HELLBALL:
    // AoE
    save = -1;
    mag_resist = FALSE;
    element = DAM_ENERGY;
    num_dice = level + 16;
    size_dice = 16;
    bonus = level + 100;
    break;

  case SPELL_HORRID_WILTING: // horrid wilting, necromancy
    // AoE
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_NEGATIVE;
    num_dice = MIN(28, level);
    size_dice = 8;
    bonus = level / 2;
    break;

  case SPELL_ICE_STORM: // evocation
    // AoE
    save = -1;
    mag_resist = TRUE;
    element = DAM_COLD;
    num_dice = MIN(15, CASTER_LEVEL(ch));
    size_dice = 8;
    bonus = 0;
    break;

  case SPELL_INCENDIARY: // incendiary cloud (conjuration)
    // AoE
    if (AFF_FLAGGED(victim, AFF_WIND_WALL))
    {
      send_to_char(ch, "The wall of wind surrounding you blows away the incendiary cloud.\r\n");
      act("A wall of wind surrounding $n dissipates the incendiary cloud.", FALSE, ch, 0, 0, TO_ROOM);
      return (0);
    }
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = level;
    size_dice = 5;
    bonus = level;
    break;

  case SPELL_METEOR_SWARM:
    // AoE
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = level + 4;
    size_dice = 12;
    bonus = level + 8;
    break;

  case SPELL_PRISMATIC_SPRAY: // illusion
    //  has effect too
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_ILLUSION;
    num_dice = MIN(26, level);
    size_dice = 4;
    bonus = 0;
    break;

  case SPELL_SUNBEAM: // evocation [light]
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_LIGHT;
    num_dice = 4;
    size_dice = 6;
    bonus = level;
    break;

  case SPELL_SUNBURST: // divination
    //  has effect too
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = MIN(26, level);
    size_dice = 5;
    bonus = level;
    break;

  case SPELL_SYMBOL_OF_PAIN: // necromancy
    // AoE
    save = SAVING_WILL;
    mag_resist = TRUE;
    element = DAM_UNHOLY;
    num_dice = MIN(17, level);
    size_dice = 6;
    bonus = 0;
    break;

  case SPELL_THUNDERCLAP: // abjuration
    //  has effect too
    // no save
    save = -1;
    // no resistance
    mag_resist = FALSE;
    element = DAM_SOUND;
    num_dice = 1;
    size_dice = 10;
    bonus = level;
    break;

  case SPELL_WAIL_OF_THE_BANSHEE: // necromancy
    if (is_immune_death_magic(ch, victim, TRUE))
      return (0);
    //  has effect too
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_SOUND;
    num_dice = level + 2;
    size_dice = 5;
    bonus = level + 10;
    break;

  case SPELL_CIRCLE_OF_DEATH: // necromancy
    // AoE
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_NEGATIVE;
    num_dice = MIN(level, 20);
    size_dice = 6;
    bonus = 0;
    break;

  case SPELL_UNDEATH_TO_DEATH: // necromancy
    // AoE
    save = SAVING_FORT;
    mag_resist = TRUE;
    element = DAM_HOLY;
    num_dice = MIN(level, 20);
    size_dice = 6;
    bonus = 0;
    break;

  case SPELL_GRASP_OF_THE_DEAD: // necromancy
    // AoE
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_SLICE;
    num_dice = level;
    size_dice = 6;
    bonus = 0;
    break;

    /***********************************************\
      || ------------ DIVINE AoE SPELLS ------------ ||
      \***********************************************/

  case SPELL_BLADES: // blade barrier damage (divine spell)
    // AoE
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_SLICE;
    num_dice = level;
    size_dice = 6;
    bonus = 2;
    break;

  case SPELL_EARTHQUAKE:
    // AoE
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_EARTH;
    num_dice = level;
    size_dice = 8;
    bonus = num_dice + 30;
    break;

  case SPELL_FIRE_STORM:
    // AoE
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_FIRE;
    num_dice = MIN(level, 20);
    size_dice = 6;
    bonus = 0;
    break;

  case SPELL_INSECT_PLAGUE: // conjuration
    if (AFF_FLAGGED(victim, AFF_WIND_WALL))
    {
      send_to_char(ch, "The wall of wind surrounding you blows away the plague of insects.\r\n");
      act("A wall of wind surrounding $n dissipates the plague of insects.", FALSE, ch, 0, 0, TO_ROOM);
      return (0);
    }
    mag_resist = FALSE;
    save = -1;
    element = DAM_NEGATIVE;
    num_dice = MIN(level / 3, 6);
    size_dice = 6;
    bonus = level;
    break;

  case SPELL_SUMMON_SWARM: // conjuration
    if (AFF_FLAGGED(victim, AFF_WIND_WALL))
    {
      send_to_char(ch, "The wall of wind surrounding you blows away the swarm of insects.\r\n");
      act("A wall of wind surrounding $n blows aside the swarm of insects.", FALSE, ch, 0, 0, TO_ROOM);
    }
    mag_resist = FALSE;
    save = -1;
    element = DAM_NEGATIVE;
    num_dice = 1;
    size_dice = 6;
    bonus = MAX(level, 5);
    break;

  case SPELL_WHIRLWIND: // evocation
    save = SAVING_REFL;
    mag_resist = TRUE;
    element = DAM_AIR;
    num_dice = level + dice(1, 3);
    size_dice = 6;
    bonus = level;
    break;

  } /* end switch(spellnum) */
  /**************************/

  if (IS_SPECIALTY_SCHOOL(ch, spellnum))
    size_dice++;

  if (IS_SET(metamagic, METAMAGIC_MAXIMIZE))
  {
    dam = (num_dice * size_dice) + bonus;
  }
  else
  {
    dam = dice(num_dice, size_dice) + bonus;
  }

  if (HAS_FEAT(ch, FEAT_ARCANE_BLOODLINE_ARCANA) && metamagic > 0)
    GET_DC_BONUS(ch) += 1;

  if (spellnum > 0 && spellnum < NUM_SPELLS)
  {
    if (HAS_FEAT(ch, FEAT_ENHANCED_SPELL_DAMAGE))
      dam += num_dice * HAS_FEAT(ch, FEAT_ENHANCED_SPELL_DAMAGE);
  }
  else if (spellnum >= PSIONIC_POWER_START && spellnum <= PSIONIC_POWER_END)
  {
    if (HAS_FEAT(ch, FEAT_ENHANCED_POWER_DAMAGE))
      dam += num_dice * HAS_FEAT(ch, FEAT_ENHANCED_POWER_DAMAGE);

    if (HAS_FEAT(ch, FEAT_EPIC_POWER_DAMAGE))
      dam += num_dice * 3;
    // each rank of epic psionics increases psi power damage by 10%, or 20% if under psionic focus affect
    if (HAS_FEAT(ch, FEAT_EPIC_PSIONICS))
      dam = dam * (100 + (HAS_FEAT(ch, FEAT_EPIC_PSIONICS) * affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) ? 20 : 10)) / 100;
  }

  // vampire bonuses / penalties for feeding
  dam = dam * (10 + vampire_last_feeding_adjustment(ch)) / 10;

  if (HAS_FEAT(ch, FEAT_DRACONIC_BLOODLINE_ARCANA) && element == draconic_heritage_energy_types[GET_BLOODLINE_SUBTYPE(ch)])
    dam += num_dice;

  if (spellnum == SPELL_CIRCLE_OF_DEATH && !IS_LIVING(victim))
  {
    act("You ignore the spell affect, as it only affects the living.", TRUE, ch, 0, victim, TO_VICT);
    act("$N ignores the spell affect, as it only affects the living.", TRUE, ch, 0, victim, TO_ROOM);
    return 1;
  }

  if (spellnum == SPELL_UNDEATH_TO_DEATH && !IS_UNDEAD(victim))
  {
    act("You ignore the spell affect, as it only affects undead.", TRUE, ch, 0, victim, TO_VICT);
    act("$N ignores the spell affect, as it only affects undead.", TRUE, ch, 0, victim, TO_ROOM);
    return 1;
  }

  if (process_iron_golem_immunity(ch, victim, element, dam))
    return 1;

  // resistances to magic, message in mag_resistance
  if (dam && mag_resist)
  {
    if (process_iron_golem_immunity(ch, victim, element, dam))
      ;
    else
    {
      if (is_spellnum_psionic(spellnum))
      {
        if (power_resistance(ch, victim, mag_resist_bonus))
          return 0;
      }
      else
      {
        if (mag_resistance(ch, victim, mag_resist_bonus))
          return 0;
      }
    }
  }

  if (spellnum >= PSIONIC_POWER_START && spellnum <= PSIONIC_POWER_END)
  {
    switch (element)
    {
    case DAM_FIRE:
      if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) && HAS_FEAT(ch, FEAT_ELEMENTAL_FOCUS_FIRE))
        bonus += num_dice;
      GET_DC_BONUS(ch)
      ++;
      break;
    case DAM_ACID:
      if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) && HAS_FEAT(ch, FEAT_ELEMENTAL_FOCUS_ACID))
        bonus += num_dice;
      GET_DC_BONUS(ch)
      ++;
      break;
    case DAM_COLD:
      if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) && HAS_FEAT(ch, FEAT_ELEMENTAL_FOCUS_COLD))
        bonus += num_dice;
      GET_DC_BONUS(ch)
      ++;
      break;
    case DAM_SOUND:
      if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) && HAS_FEAT(ch, FEAT_ELEMENTAL_FOCUS_SOUND))
        bonus += num_dice;
      GET_DC_BONUS(ch)
      ++;
      break;
    case DAM_ELECTRIC:
      if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS) && HAS_FEAT(ch, FEAT_ELEMENTAL_FOCUS_ELECTRICITY))
        bonus += num_dice;
      GET_DC_BONUS(ch)
      ++;
      break;
    }

    if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS))
      dam *= 0.10;
  }

  // dwarven racial bonus to magic, gnomes to illusion
  int race_bonus = 0;
  if (GET_RACE(victim) == RACE_DWARF)
    race_bonus += 2;
  if (GET_RACE(victim) == RACE_DUERGAR)
  {
    race_bonus += 4;
    if (spellnum == SPELL_WEIRD)
      race_bonus += 4;
  }
  if (GET_RACE(victim) == RACE_GNOME && element == DAM_ILLUSION)
    race_bonus += 2;
  if (GET_RACE(victim) == RACE_ARCANA_GOLEM)
    race_bonus -= 2;

  if (element == DAM_POISON)
    race_bonus += get_poison_save_mod(ch, victim);

  if (element == DAM_POISON && KNOWS_DISCOVERY(ch, ALC_DISC_CELESTIAL_POISONS))
    element = DAM_CELESTIAL_POISON;

  // figure saving throw for finger of death here, because it's not half damage
  if (spellnum == SPELL_FINGER_OF_DEATH)
  {
    if (mag_savingthrow(ch, victim, save, race_bonus, casttype, level, NECROMANCY))
    {
      if (IS_SET(metamagic, METAMAGIC_MAXIMIZE))
      {
        dam = (18) + MIN(25, level);
      }
      else
      {
        dam = dice(3, 6) + MIN(25, level);
      }
    }
  }
  else if (spellnum == PSIONIC_DEADLY_FEAR)
  {
    GET_DC_BONUS(ch) += bonus;
    if (!mag_savingthrow(ch, victim, save, race_bonus, SAVING_WILL, level, NOSCHOOL))
    {
      GET_DC_BONUS(ch) += bonus;
      if (mag_savingthrow(ch, victim, save, race_bonus, SAVING_FORT, level, NOSCHOOL))
        dam = dice(3, 6);
      else
        dam = GET_HIT(victim) + 100;
    }
  }
  else if (spellnum == PSIONIC_PSYCHIC_CRUSH)
  {
    GET_DC_BONUS(ch) += bonus - 4;
    if (mag_savingthrow(ch, victim, save, race_bonus, SAVING_WILL, level, NOSCHOOL))
      dam = dice(3 + bonus, 6);
    else
      dam = GET_HIT(victim) + 100;
  }
  else if (spellnum == PSIONIC_DISINTEGRATION)
  {
    // on a fort save, disintigrate will always do 5d6 damage
    if (mag_savingthrow(ch, victim, save, race_bonus, savetype, level, NOSCHOOL))
      dam = dice(5, 6);
    // otherwise we'll let things proceeed as normal
  }
  else if (spellnum == PSIONIC_RECALL_DEATH)
  {
    if (mag_savingthrow(ch, victim, save, race_bonus, savetype, level, NOSCHOOL))
      dam = dice(5, 6);
    else
      dam = GET_HIT(victim) + 100;
  }
  else if (dam && (save != -1))
  {
    // saving throw for half damage if applies
    if (mag_savingthrow(ch, victim, save, race_bonus, casttype, level, spell_school))
    {
      if (save_negates)
      {
        dam = 0;
      }
      else if ((!IS_NPC(victim)) && save != SAVING_REFL && (HAS_FEAT(victim, FEAT_STALWART)))
      {
        dam = 0;
      }
      else
      {
        if ((!IS_NPC(victim)) && save == SAVING_REFL && // evasion
            (HAS_FEAT(victim, FEAT_EVASION) ||
             (HAS_FEAT(victim, FEAT_IMPROVED_EVASION))))
          dam /= 2;
        dam /= 2;
      }
    }
    else if ((!IS_NPC(victim)) && save == SAVING_REFL && // evasion
             (HAS_FEAT(victim, FEAT_IMPROVED_EVASION)))
      dam /= 2;
  }

  /* blinking between prime and ethereal planes - 20% dodge AoE spells */
  if (IS_SET(spell_info[spellnum].routines, MAG_AREAS) &&
      AFF_FLAGGED(victim, AFF_BLINKING) && rand_number(1, 100) <= 20)
  {
    act("$n watches as $N blinks out of existence to avoid the spell!",
        FALSE, ch, NULL, victim, TO_NOTVICT);
    act("You watch as $N briefly blinks out of existence to avoid your spell!",
        FALSE, ch, NULL, victim, TO_CHAR);
    act("You quickly blink out of existence avoiding the harmful spell from $n!",
        FALSE, ch, NULL, victim, TO_VICT);
    return 0;
  }

  /* surprise spell feat */
  if (HAS_FEAT(ch, FEAT_SURPRISE_SPELLS) &&
      (!KNOWS_DISCOVERY(victim, ALC_DISC_PRESERVE_ORGANS) || dice(1, 4) > 1) &&
      (compute_concealment(victim) == 0) &&
      ((AFF_FLAGGED(victim, AFF_FLAT_FOOTED)) /* Flat-footed */
       || !(has_dex_bonus_to_ac(ch, victim))  /* No dex bonus to ac */
       || is_flanked(ch, victim)              /* Flanked */
       ))
  {

    dam += dice(HAS_FEAT(ch, FEAT_SNEAK_ATTACK), 6);

    send_to_char(ch, "[\tDSURPRISE SPELL\tn] ");
    send_to_char(victim, "[\tRSURPRISE SPELL\tn] ");
  }

  if (!element) // want to make sure all spells have some sort of damage category
    log("SYSERR: %d is lacking DAM_", spellnum);

  return (damage(ch, victim, dam, spellnum, element, FALSE));
}

/* this variable is used for the system for 'spamming' ironskin -zusuk */
#define WARD_THRESHOLD 151

/* Note: converted affects to rounds, 20 rounds = 1 real minute, 1200 rounds = 1 real hour
   old tick = 75 seconds, or 1.25 minutes or 25 rounds */

/* all spells, spell-like affects, alchemy, psionics, etc that puts on an affect should be going through here */
void mag_affects(int level, struct char_data *ch, struct char_data *victim,
                 struct obj_data *wpn, int spellnum, int savetype, int casttype, int metamagic)
{

  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i, j, x, spell_school = NOSCHOOL;
  int enchantment_bonus = 0, illusion_bonus = 0, paralysis_bonus = 0, success = 0;
  bool is_mind_affect = FALSE;
  struct damage_reduction_type *new_dr = NULL, *dr = NULL, *temp = NULL;
  bool is_immune_sleep = FALSE;

  if (victim == NULL || ch == NULL)
    return;

  if (spell_info[spellnum].violent)
    if (HAS_FEAT(ch, FEAT_ARCANE_BLOODLINE_ARCANA) && metamagic > 0)
      GET_DC_BONUS(ch) += 1;

  /* elven drow resistance to certain enchantments such as sleep */
  if (HAS_FEAT(victim, FEAT_SLEEP_ENCHANTMENT_IMMUNITY))
  {
    is_immune_sleep = TRUE;
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
  { // init affect array
    new_affect(&(af[i]));
    af[i].spell = spellnum;
  }

  /* caster level is used for calculating bonuses - be aware that certain
     cast types coming in here are not even related to magic (hack) */
  if (casttype == CAST_INNATE)
    level = GET_LEVEL(ch);

  /* various bonus/penalty; added IS_NPC check to prevent NPCs from getting incorrect bonuses,
   * since RACE_TYPE_HUMAN = RACE_ELF, etc. -Nashak */
  if (!IS_NPC(ch))
  {

    switch (GET_RACE(ch))
    {                /* caster */
    case RACE_GNOME: // illusions
      break;

    default:
      break;
    }

    /* racial victim resistance */
    switch (GET_RACE(victim))
    { /* target */
    case RACE_H_ELF:
    case RACE_DROW:
    case RACE_ELF: // enchantments
      break;
    case RACE_ARCANA_GOLEM: // enchantments, penalty
      break;
    case RACE_GNOME: // illusions
      break;

    default:
      break;
    }
  }

  /* we are putting some feats here that affect either bonus/penalties to checks */
  /* caster / ch */
  if (HAS_FEAT(ch, FEAT_ILLUSION_AFFINITY))
  {
    illusion_bonus -= 2; /* gnome */
  }
  /* target / victim */
  if (HAS_FEAT(victim, FEAT_STILL_MIND))
  {
    enchantment_bonus += 2;
  }
  if (HAS_FEAT(victim, FEAT_RESISTANCE_TO_ENCHANTMENTS))
  {
    enchantment_bonus += 2; /* elf, drow, etc */
  }
  if (HAS_FEAT(victim, FEAT_ENCHANTMENT_VULNERABILITY))
  {
    enchantment_bonus -= 2; /* arcana golem */
  }
  if (HAS_FEAT(victim, FEAT_RESISTANCE_TO_ILLUSIONS))
  {
    illusion_bonus += 2; /* gnome */
  }
  if (HAS_FEAT(victim, FEAT_PARALYSIS_RESIST))
  {
    paralysis_bonus += 4; /* duergar*/
  }

  illusion_bonus += GET_RESISTANCES(victim, DAM_ILLUSION);
  enchantment_bonus += GET_RESISTANCES(victim, DAM_MENTAL);

  /****/

  /* note, hopefully we have calculated the proper level for this spell
     in call_magic() */

  spell_school = spell_info[spellnum].schoolOfMagic;

  switch (spellnum)
  {

    // psionic powers 1st

  case PSIONIC_BROKER:

    // we need to correct the psp cost below, because the power only
    // benefits from 2 augment points at a time.

    af[0].location = APPLY_SKILL;
    af[0].duration = level;
    af[0].modifier = 2 + (GET_AUGMENT_PSP(ch) / 2);
    af[0].specific = ABILITY_DIPLOMACY;

    af[1].location = APPLY_SKILL;
    af[1].duration = level;
    af[1].modifier = 2 + (GET_AUGMENT_PSP(ch) / 2);
    af[1].specific = ABILITY_APPRAISE;

    accum_duration = FALSE;
    to_vict = "Your dipomatic and appraising abilities have been enhanced.";
    break;

  case PSIONIC_CALL_TO_MIND:

    // we need to correct the psp cost below, because the power only
    // benefits from 2 augment points at a time.

    af[0].location = APPLY_SKILL;
    af[0].duration = level;
    af[0].modifier = 4 + (GET_AUGMENT_PSP(ch) / 2);
    af[0].specific = ABILITY_LORE;

    to_vict = "Your knowledge and lore expertise have been enhanced.";
    break;

  case PSIONIC_CATFALL:
    af[0].location = APPLY_DEX;
    af[0].duration = 120;
    af[0].modifier = 2;
    SET_BIT_AR(af[0].bitvector, AFF_SAFEFALL);
    to_vict = "You gain the ability to fall safely like a cat.";
    break;

  case PSIONIC_DECELERATION:

    // we need to correct the psp cost below, because the power only
    // benefits from 2 augment points at a time.

    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    if (GET_SIZE(victim) > (SIZE_MEDIUM + (GET_AUGMENT_PSP(ch) / 2)))
    {
      send_to_char(ch, "Your attempt to decelerate your foe fails, as they are too large.\r\n");

      return;
    }
    if (power_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, NOSCHOOL))
      return;
    af[0].location = APPLY_DEX;
    af[0].duration = level * 12;
    af[0].modifier = -2;
    to_room = "$N starts to move much more slowly.";
    to_vict = "You start to move much more slowly.";
    break;

  case PSIONIC_DEMORALIZE:

    // we need to correct the psp cost below, because the power only
    // benefits from 2 augment points at a time.

    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;

    if (is_immune_fear(ch, victim, TRUE))
      return;
    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;
    if (power_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, affected_by_aura_of_cowardice(victim) ? -4 : 0, casttype, level, NOSCHOOL))
      return;
    af[0].location = APPLY_WIS;
    af[0].duration = level * 12;
    af[0].modifier = -1;
    SET_BIT_AR(af[0].bitvector, AFF_SHAKEN);
    to_room = "$N looks shaken and demoralized.";
    to_vict = "You feel shaken and demoralized.";
    break;

  case PSIONIC_FORCE_SCREEN:
    if (affected_by_spell(ch, PSIONIC_FORCE_SCREEN))
    {
      send_to_char(ch, "This power can't stack.  Use the revoke command if you wish to replace it.\r\n");
      return;
    }

    // we need to correct the psp cost below, because the power only
    // benefits from 2 augment points at a time.
    af[0].location = APPLY_AC_NEW;
    af[0].bonus_type = BONUS_TYPE_SHIELD;
    af[0].duration = level * 12;
    af[0].modifier = 4 + (GET_AUGMENT_PSP(ch) / 4);

    accum_duration = accum_affect = FALSE;
    to_vict = "You feel an invisible shield of force appear in front of you.";
    break;

  case PSIONIC_FORTIFY:

    // we need to correct the psp cost below, because the power only
    // benefits from 2 augment points at a time.

    af[0].location = APPLY_SAVING_FORT;
    af[0].bonus_type = BONUS_TYPE_RESISTANCE;
    af[0].duration = level * 12;
    af[0].modifier = 2 + (GET_AUGMENT_PSP(ch) / 2);

    af[1].location = APPLY_SAVING_WILL;
    af[1].bonus_type = BONUS_TYPE_RESISTANCE;
    af[1].duration = level * 12;
    af[1].modifier = 2 + (GET_AUGMENT_PSP(ch) / 2);

    af[2].location = APPLY_SAVING_REFL;
    af[2].bonus_type = BONUS_TYPE_RESISTANCE;
    af[2].duration = level * 12;
    af[2].modifier = 2 + (GET_AUGMENT_PSP(ch) / 2);

    accum_duration = FALSE;
    to_vict = "You feel your resistances become fortified.";
    break;

  case PSIONIC_INERTIAL_ARMOR:
    if (affected_by_spell(ch, PSIONIC_INERTIAL_ARMOR))
    {
      send_to_char(ch, "This power can't stack.  Use the revoke command if you wish to replace it.\r\n");
      return;
    }

    // we need to correct the psp cost below, because the power only
    // benefits from 2 augment points at a time.

    af[0].location = APPLY_AC_NEW;
    af[0].bonus_type = BONUS_TYPE_ARMOR;
    af[0].duration = level * 600;
    af[0].modifier = 4 + (GET_AUGMENT_PSP(ch) / 2);

    accum_duration = accum_affect = FALSE;
    to_vict = "You feel an invisible armor of force surround you.";
    break;

  case PSIONIC_INEVITABLE_STRIKE:

    af[0].location = APPLY_HITROLL;
    af[0].bonus_type = BONUS_TYPE_INSIGHT;
    af[0].duration = 600;
    af[0].modifier = MIN(25, 20 + (GET_AUGMENT_PSP(ch) * 2));

    accum_duration = FALSE;
    to_vict = "You feel your hand-eye coordination and accuracy soar!";
    break;

  case PSIONIC_DEFENSIVE_PRECOGNITION:
    if (affected_by_spell(ch, PSIONIC_OFFENSIVE_PRECOGNITION))
    {
      send_to_char(ch, "You are already benefitting from a precognition effect.\r\n");
      return;
    }

    // we need to correct the psp cost below, because the power only
    // benefits from 3 augment points at a time.

    af[0].location = APPLY_AC_NEW;
    af[0].bonus_type = BONUS_TYPE_INSIGHT;
    af[0].duration = 10 + level;
    af[0].modifier = 2 + (GET_AUGMENT_PSP(ch) / 3);

    af[1].location = APPLY_SAVING_WILL;
    af[1].bonus_type = BONUS_TYPE_INSIGHT;
    af[1].duration = 10 + level;
    af[1].modifier = 2 + (GET_AUGMENT_PSP(ch) / 3);

    af[2].location = APPLY_SAVING_FORT;
    af[2].bonus_type = BONUS_TYPE_INSIGHT;
    af[2].duration = 10 + level;
    af[2].modifier = 2 + (GET_AUGMENT_PSP(ch) / 3);

    af[3].location = APPLY_SAVING_REFL;
    af[3].bonus_type = BONUS_TYPE_INSIGHT;
    af[3].duration = 10 + level;
    af[3].modifier = 2 + (GET_AUGMENT_PSP(ch) / 3);

    accum_duration = FALSE;
    to_vict = "You gain the ability to sense attacks moments before they occur.";
    break;

  case PSIONIC_OFFENSIVE_PRECOGNITION:
    if (affected_by_spell(ch, PSIONIC_DEFENSIVE_PRECOGNITION))
    {
      send_to_char(ch, "You are already benefitting from a precognition effect.\r\n");
      return;
    }

    // we need to correct the psp cost below, because the power only
    // benefits from 3 augment points at a time.

    af[0].location = APPLY_HITROLL;
    af[0].bonus_type = BONUS_TYPE_INSIGHT;
    af[0].duration = 10 + level;
    af[0].modifier = 2 + (GET_AUGMENT_PSP(ch) / 3);

    accum_duration = FALSE;
    to_vict = "You gain the ability to sense openings in your opponents defenses moments before they occur.";
    break;

  case PSIONIC_OFFENSIVE_PRESCIENCE:

    // we need to correct the psp cost below, because the power only
    // benefits from 3 augment points at a time.

    af[0].location = APPLY_DAMROLL;
    af[0].bonus_type = BONUS_TYPE_INSIGHT;
    af[0].duration = 10 + level;
    af[0].modifier = 2 + (GET_AUGMENT_PSP(ch) / 3);

    accum_duration = FALSE;
    to_vict = "You gain the ability to sense your opponent's weak spots.";
    break;

  case PSIONIC_SLUMBER:

    if (GET_LEVEL(victim) > (4 + GET_AUGMENT_PSP(ch)) || is_immune_sleep)
    {
      send_to_char(ch, "The target is too powerful for you to make it slumber!\r\n");
      return;
    }
    if (sleep_immunity(victim))
    {
      send_to_char(ch, "Your target is unfazed.\r\n");
      return;
    }
    if (power_resistance(ch, victim, 0))
      return;
    if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP))
    {
      send_to_char(ch, "Your victim doesn't seem vulnerable to your manifestation.");
      return;
    }
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NOSCHOOL))
    {
      return;
    }

    af[0].duration = (level * 12);
    SET_BIT_AR(af[0].bitvector, AFF_SLEEP);

    if (GET_POS(victim) > POS_SLEEPING)
    {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      if (FIGHTING(victim))
        stop_fighting(victim);
      change_position(victim, POS_SLEEPING);
      if (FIGHTING(ch) == victim)
        stop_fighting(ch);
    }

    accum_duration = FALSE;
    break;

  case PSIONIC_VIGOR:
    if (affected_by_spell(ch, PSIONIC_VIGOR))
    {
      send_to_char(ch, "This power can't stack.  Use the revoke command if you wish to replace it.\r\n");
      return;
    }

    af[0].location = APPLY_HIT;
    af[0].bonus_type = BONUS_TYPE_CIRCUMSTANCE; /* stacks */
    af[0].duration = 12 * level;
    af[0].modifier = 5 + ((GET_AUGMENT_PSP(ch) / 3) * 5);

    accum_duration = FALSE;
    accum_affect = FALSE;
    to_vict = "You feel enhanced vigor and durability.";
    break;

  case PSIONIC_BIOFEEDBACK:

    if (affected_by_spell(ch, PSIONIC_BIOFEEDBACK))
    {
      send_to_char(ch, "This power can't stack.  Use the revoke command if you wish to replace it.\r\n");
      return;
    }
    af[0].location = APPLY_DR;
    af[0].modifier = 2 + (GET_AUGMENT_PSP(ch) / 3);
    af[0].duration = 12 * level;

    to_vict = "Your skin becomes much more dense through psychic biofeedback.";

    CREATE(new_dr, struct damage_reduction_type, 1);

    new_dr->bypass_cat[0] = DR_BYPASS_CAT_NONE;
    new_dr->bypass_val[0] = 0;

    new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[1] = 0; /* Unused. */

    new_dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[2] = 0; /* Unused. */

    new_dr->amount = 2 + (GET_AUGMENT_PSP(ch) / 3);
    new_dr->max_damage = -1;
    new_dr->spell = PSIONIC_BIOFEEDBACK;
    new_dr->feat = FEAT_UNDEFINED;
    new_dr->next = GET_DR(victim);
    GET_DR(victim) = new_dr;

    accum_duration = FALSE;
    break;

  case PSIONIC_BODY_EQUILIBRIUM:
    af[0].duration = 100 * level;
    SET_BIT_AR(af[0].bitvector, AFF_WATERWALK);
    af[1].duration = 100 * level;
    SET_BIT_AR(af[1].bitvector, AFF_LEVITATE);
    to_vict = "You gain the ability to walk on water and levitate in mid-air.";
    break;

  case PSIONIC_BREACH:

    af[0].location = APPLY_SKILL;
    af[0].duration = level * 12;
    af[0].modifier = 2 + GET_AUGMENT_PSP(ch);
    af[0].specific = ABILITY_SLEIGHT_OF_HAND;

    accum_duration = FALSE;
    to_vict = "Your sleight of hand abilities have been enhanced.";
    break;

  case PSIONIC_CONCEALING_AMORPHA:

    if (ch != victim && GET_AUGMENT_PSP(ch) < 3)
    {
      send_to_char(ch, "You need to augment this power with 3 psp points to use it on another being.\r\n");
      return;
    }
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = 20;
    af[0].duration = level * 12;

    accum_duration = FALSE;
    to_vict = "You have been covered in a film of transulcent, amorphous membrane.";
    break;

  case PSIONIC_DETECT_HOSTILE_INTENT:
    af[0].duration = level * 120;
    SET_BIT_AR(af[0].bitvector, AFF_DANGERSENSE);
    accum_duration = FALSE;
    to_vict = "You have gained the ability to sense danger in a specified direction.";
    break;

  case PSIONIC_ELFSIGHT:
    af[0].duration = level * 120;
    af[0].location = APPLY_SKILL;
    af[0].modifier = 2;
    af[0].specific = ABILITY_PERCEPTION;
    SET_BIT_AR(af[0].bitvector, AFF_INFRAVISION);
    accum_duration = FALSE;
    to_vict = "Your vision has improved tremendously.";
    break;

  case PSIONIC_ENERGY_ADAPTATION_SPECIFIED:
    af[0].duration = level * 120;
    af[0].location = damage_type_to_resistance_type(GET_PSIONIC_ENERGY_TYPE(ch));
    af[0].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    accum_duration = FALSE;
    switch (GET_PSIONIC_ENERGY_TYPE(ch))
    {
    case DAM_FIRE:
      to_vict = "Your resistance to fire has improved.";
    case DAM_COLD:
      to_vict = "Your resistance to cold has improved.";
    case DAM_ACID:
      to_vict = "Your resistance to acid has improved.";
    case DAM_ELECTRIC:
      to_vict = "Your resistance to lightning has improved.";
    case DAM_SOUND:
      to_vict = "Your resistance to sonic damage has improved.";
    }
    break;

  case PSIONIC_ENERGY_ADAPTATION:
    af[0].duration = level * 120;
    af[0].location = APPLY_RES_ACID;
    af[0].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    af[1].duration = level * 120;
    af[1].location = APPLY_RES_FIRE;
    af[1].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    af[2].duration = level * 120;
    af[2].location = APPLY_RES_COLD;
    af[2].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    af[3].duration = level * 120;
    af[3].location = APPLY_RES_ELECTRIC;
    af[3].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    af[4].duration = level * 120;
    af[4].location = APPLY_RES_SOUND;
    af[4].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    accum_duration = FALSE;
    to_vict = "Your elemental resistances have improved!";
    break;

  case PSIONIC_ENERGY_STUN:
    if (!can_stun(victim))
    {
      send_to_char(ch, "It seems your opponent cannot be stunned.\r\n");
      return;
    }

    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    GET_DC_BONUS(ch) += (GET_PSIONIC_ENERGY_TYPE(ch) == DAM_ELECTRIC || GET_PSIONIC_ENERGY_TYPE(ch) == DAM_SOUND) ? 2 : 0;
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NOSCHOOL))
      return;
    af[0].duration = 1;
    SET_BIT_AR(af[0].bitvector, AFF_STUN);
    accum_duration = FALSE;
    to_vict = "You have been stunned!";
    to_room = "$n has been stunned!";
    break;

  case PSIONIC_INFLICT_PAIN:
    if (power_resistance(ch, victim, 0))
      return;

    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NOSCHOOL))
    {
      af[0].modifier = -2;
    }
    else
    {
      af[0].modifier = -4;
    }
    af[0].duration = level;
    af[0].location = APPLY_HITROLL;
    accum_duration = FALSE;
    to_vict = "You have been struck with intense pain!";
    to_room = "$n has been struck with intense pain!";
    break;

  case PSIONIC_MENTAL_DISRUPTION:
    if (power_resistance(ch, victim, 0))
      return;

    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NOSCHOOL))
      return;
    af[0].duration = 1 + (GET_AUGMENT_PSP(ch) / 4);
    SET_BIT_AR(af[0].bitvector, AFF_DAZED);
    to_vict = "An assault on your mind has left you dazed!";
    to_room = "$n suddenly looks shocked and dazed!";
    break;

  case PSIONIC_PSYCHIC_BODYGUARD:

    af[0].bonus_type = BONUS_TYPE_INSIGHT;
    af[0].location = APPLY_SAVING_WILL;
    af[0].modifier = MAX(0, compute_mag_saves(ch, SAVING_WILL, 0) - compute_mag_saves(victim, SAVING_WILL, 0));
    af[0].duration = 600;

    af[1].location = APPLY_SPECIAL;
    af[1].duration = 600;
    if (GET_AUGMENT_PSP(ch) >= 8)
      af[1].modifier = 100;
    else
      af[1].modifier = 1 + (GET_AUGMENT_PSP(ch) / 2);

    break;

  case PSIONIC_THOUGHT_SHIELD:

    af[0].duration = 1 + GET_AUGMENT_PSP(ch);
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = 13 + GET_AUGMENT_PSP(ch);
    accum_duration = FALSE;
    to_vict = "Your resistance against mind affecting powersm has increased.";

    break;

  case PSIONIC_ENDORPHIN_SURGE:

    af[0].location = APPLY_STR;
    af[0].modifier = 2 + (GET_AUGMENT_PSP(ch) >= 6 ? 2 : 0);
    af[0].bonus_type = BONUS_TYPE_MORALE;
    af[0].duration = 10 + GET_CON_BONUS(ch);

    af[1].location = APPLY_CON;
    af[1].modifier = 2 + (GET_AUGMENT_PSP(ch) >= 6 ? 2 : 0);
    af[1].bonus_type = BONUS_TYPE_MORALE;
    af[1].duration = 10 + GET_CON_BONUS(ch);

    af[2].location = APPLY_SAVING_WILL;
    af[2].modifier = 1 + (GET_AUGMENT_PSP(ch) >= 6 ? 1 : 0);
    af[2].bonus_type = BONUS_TYPE_MORALE;
    af[2].duration = 10 + GET_CON_BONUS(ch);

    // this is a penalty
    af[3].location = APPLY_AC_NEW;
    af[3].modifier = -2;
    af[3].duration = 10 + GET_CON_BONUS(ch);

    af[4].location = APPLY_HIT;
    af[4].modifier = (2 + (GET_AUGMENT_PSP(ch) >= 6 ? 2 : 0)) * 2;
    af[4].bonus_type = BONUS_TYPE_MORALE;
    af[4].duration = 10 + GET_CON_BONUS(ch);
    to_vict = "You go into a \tRR\trA\tRG\trE\tn!";
    to_room = "$n goes into a \tRR\trA\tRG\trE\tn!";

    break;

  case PSIONIC_ENERGY_RETORT:

    af[0].duration = (level + GET_AUGMENT_PSP(ch)) * 10;
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = GET_PSIONIC_ENERGY_TYPE(ch);
    accum_duration = FALSE;
    to_vict = "You are surrounded by a psychic shield of elemental energy.";
    to_room = "$n is surrounded by a psychic shield of elemental energy.";

    break;

  case PSIONIC_HEIGHTENED_VISION:
    af[0].duration = level * 120;
    SET_BIT_AR(af[0].bitvector, AFF_ULTRAVISION);
    accum_duration = FALSE;
    to_vict = "You can now see in the dark as well as in the light.";
    break;

  case PSIONIC_MENTAL_BARRIER:

    af[0].duration = (1 + GET_AUGMENT_PSP(ch)) * 10;
    af[0].location = APPLY_AC_NEW;
    af[0].modifier = 4 + (GET_AUGMENT_PSP(ch) / 4);
    accum_duration = FALSE;
    to_vict = "You create a psychic barrier of deflective force around you.";

    break;

  case PSIONIC_MIND_TRAP:

    af[0].duration = (1 + GET_AUGMENT_PSP(ch));
    af[0].location = APPLY_NONE;
    to_vict = "You create a psychic trap against mental attacks.";

    break;

  case PSIONIC_PSIONIC_BLAST:
    if (!can_stun(victim))
    {
      send_to_char(ch, "It seems your opponent cannot be stunned.\r\n");
      return;
    }

    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    if (power_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NOSCHOOL))
      return;
    af[0].duration = 1 + GET_AUGMENT_PSP(ch) / 2;
    SET_BIT_AR(af[0].bitvector, AFF_STUN);
    to_vict = "You have been stunned by $N's psionic blast!";
    to_room = "$n has been stunned by $N's psionic blast!";
    break;

  case PSIONIC_SHARPENED_EDGE:
    af[0].duration = level * 120;
    af[0].location = APPLY_SPECIAL;
    to_vict = "Any slashing or piercing weapon you wield becomes extremely sharp through your psychic abilities.";
    break;

  case PSIONIC_UBIQUITUS_VISION:
    af[0].duration = level * 120;
    af[0].location = APPLY_SPECIAL;
    to_vict = "You gain the ability to see 360 degrees around you.";
    break;

  case PSIONIC_DEATH_URGE:
    if (power_resistance(ch, victim, 0))
      return;
    if (is_immune_mind_affecting(ch, victim, 0))
      return;

    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;

    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NOSCHOOL))
      return;
    af[0].duration = 2 + (GET_AUGMENT_PSP(ch) / 4);
    af[0].location = APPLY_SPECIAL;
    to_vict = "You suddenly have an unavoidable urge to harm yourself!";
    to_room = "$n becomes crazed and begins to harm $mself!";
    break;

  case PSIONIC_EMPATHIC_FEEDBACK:
    af[0].duration = level * 120;
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = MIN(10, 6 + (GET_AUGMENT_PSP(ch) * 2 / 3));
    to_vict = "You gain the ability reflect melee damage back on your attacker as psychic damage.";

    break;

  case PSIONIC_INCITE_PASSION:
    if (is_immune_mind_affecting(ch, victim, 0))
      return;
    if (power_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NOSCHOOL))
      return;

    af[0].duration = level;
    af[0].location = APPLY_HITROLL;
    af[0].modifier = -2;
    af[0].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

    af[1].duration = level;
    af[1].location = APPLY_INT;
    af[1].modifier = -4;
    af[1].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

    af[2].duration = level;
    af[2].location = APPLY_AC_NEW;
    af[2].modifier = -2;
    af[2].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

    disable_combat_mode(victim, MODE_TOTAL_DEFENSE);
    disable_combat_mode(victim, MODE_COMBAT_EXPERTISE);

    to_vict = "Your passions become extremely incited and your ability to reason and oncentrate reduced.";
    to_room = "$n gets an intense gleam in $s eyes.";
    break;

  case PSIONIC_INTELLECT_FORTRESS:
    af[0].duration = level * 12;
    af[0].location = APPLY_RES_MENTAL;
    af[0].modifier = 50;
    to_vict = "Your resistance against psionic damage improves.";
    break;

  case PSIONIC_MOMENT_OF_TERROR:

    if (is_immune_mind_affecting(ch, victim, 0))
      return;
    if (is_immune_fear(ch, victim, 0))
      return;
    if (power_resistance(ch, victim, 0))
      return;
    if (GET_AUGMENT_PSP(ch) < 4 && mag_savingthrow(ch, victim, SAVING_WILL, affected_by_aura_of_cowardice(victim) ? -4 : 0, casttype, level, NOSCHOOL))
      return;
    change_position(victim, POS_SITTING);
    af[0].duration = 600;
    af[0].location = APPLY_SAVING_WILL;
    af[0].modifier = -2;
    af[0].bonus_type = BONUS_TYPE_CIRCUMSTANCE;
    to_vict = "You fall to the ground, your mind filled with thoughts of terror!";
    to_room = "$n falls to the ground and begins to frantically look about with eyes filled with terror!";
    break;

  case PSIONIC_POWER_LEECH:
    to_vict = "You reach out to your opponent and begin to drain $s psychic energies.";
    to_room = "$N reaches out to $N opponent and begin to drain $s psychic energies.";
    for (x = 0; x < GET_PSIONIC_LEVEL(ch); x++)
    {
      NEW_EVENT(ePOWERLEECH, ch, NULL, ((x * 6) * PASSES_PER_SEC));
    }
    break;

  case PSIONIC_SLIP_THE_BONDS:
    af[0].duration = level * 120;
    SET_BIT_AR(af[0].bitvector, AFF_FREE_MOVEMENT);
    accum_duration = FALSE;
    to_vict = "You are now free from movement impairing affects.";
    break;

  case PSIONIC_WITHER:

    if (power_resistance(ch, victim, 0))
      return;
    af[0].duration = level;
    af[0].location = APPLY_STR;
    af[0].modifier = -dice(2 + (GET_AUGMENT_PSP(ch) / 4), 4);
    af[0].bonus_type = BONUS_TYPE_CIRCUMSTANCE;
    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, NOSCHOOL))
      af[0].modifier /= 2;
    to_vict = "You feel your body and strength wither!";
    to_room = "$n begins to wither before your eyes!";
    break;

  case PSIONIC_PIERCE_VEIL:
    af[0].duration = level * 12;
    SET_BIT_AR(af[0].bitvector, AFF_TRUE_SIGHT);
    accum_duration = FALSE;
    to_vict = "You are now able to see the true forms of all beings.";
    break;

  case PSIONIC_POWER_RESISTANCE:
    af[0].duration = level * 12;
    af[0].modifier = 12 + level;
    af[0].location = APPLY_POWER_RES;
    accum_duration = FALSE;
    to_vict = "You are now resistant to psionic manifestations.";
    break;

  case PSIONIC_TOWER_OF_IRON_WILL:
    af[0].duration = level + (GET_AUGMENT_PSP(ch) / 2);
    af[0].modifier = 19 + (GET_AUGMENT_PSP(ch) / 2);
    af[0].location = APPLY_POWER_RES;
    accum_duration = FALSE;
    to_vict = "You are now resistant to psionic manifestations.";
    break;

  case PSIONIC_ASSIMILATE:
    if (is_immune_mind_affecting(ch, victim, 0))
      return;
    if (power_resistance(ch, victim, 0))
      return;
    af[0].duration = level;
    af[0].modifier = 1;
    af[0].location = APPLY_SPECIAL;
    accum_duration = FALSE;
    to_vict = "Your body has become vulnerable to assimilation!";
    to_room = "$n's body has become vulnerable to assimilation!";
    break;

  case PSIONIC_BRUTALIZE_WOUNDS:
    if (is_immune_mind_affecting(ch, victim, 0))
      return;
    if (power_resistance(ch, victim, 0))
      return;
    af[0].duration = level;
    af[0].modifier = mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NOSCHOOL) ? BRUTALIZE_WOUNDS_SAVE_SUCCESS : BRUTALIZE_WOUNDS_SAVE_FAIL;
    af[0].location = APPLY_SPECIAL;
    accum_duration = FALSE;
    to_vict = "Your body has become more vulnerable to physical attacks.";
    to_room = "$n's body has become more vulnerable to physical attacks.";
    break;

  case PSIONIC_SUSTAINED_FLIGHT:
    af[0].duration = level * 120;
    SET_BIT_AR(af[0].bitvector, AFF_FLYING);
    accum_duration = FALSE;
    to_vict = "You rise into the air, obtaining the ability to fly.";
    break;

  case PSIONIC_COSMIC_AWARENESS:
    af[0].duration = level * 12;
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = GET_PSIONIC_LEVEL(ch);
    accum_duration = FALSE;
    to_vict = "You gain the ability to alter probability in your favour. (cosmicawareness command)";
    break;

  case PSIONIC_ENERGY_CONVERSION:
    af[0].duration = level * 120;
    af[0].location = APPLY_RES_ACID;
    af[0].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    af[1].duration = level * 120;
    af[1].location = APPLY_RES_FIRE;
    af[1].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    af[2].duration = level * 120;
    af[2].location = APPLY_RES_COLD;
    af[2].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    af[3].duration = level * 120;
    af[3].location = APPLY_RES_ELECTRIC;
    af[3].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    af[4].duration = level * 120;
    af[4].location = APPLY_RES_SOUND;
    af[4].modifier = (level >= 11) ? 60 : (level >= 7) ? 40
                                                       : 20;
    accum_duration = FALSE;
    to_vict = "Your elemental resistances have improved, and you've gained the ability to release energy absorbed! (discharge command)";
    break;

  case PSIONIC_EVADE_BURST:

    af[0].duration = level;
    af[0].location = APPLY_FEAT;
    if (GET_AUGMENT_PSP(ch) == 4)
      af[0].modifier = FEAT_IMPROVED_EVASION;
    else
      af[0].modifier = FEAT_EVASION;
    accum_duration = FALSE;
    to_vict = "You gain the ability to evade many area bursts of magic and other energy.";
    break;

  case PSIONIC_OAK_BODY:
    if (has_psionic_body_form_active(ch))
    {
      send_to_char(ch, "You can only assume one psionic body form at a time.\r\n");
      return;
    }

    af[0].location = APPLY_DR;
    af[0].modifier = 0;
    af[0].duration = 12 * level + GET_AUGMENT_PSP(ch);

    af[1].location = APPLY_AC_NEW;
    af[1].modifier = 5;
    af[1].bonus_type = BONUS_TYPE_NATURALARMOR;
    af[1].duration = 12 * level + GET_AUGMENT_PSP(ch);

    af[2].location = APPLY_RES_COLD;
    af[2].modifier = 50;
    af[2].duration = 12 * level + GET_AUGMENT_PSP(ch);

    af[3].location = APPLY_RES_FIRE;
    af[3].modifier = -50;
    af[3].duration = 12 * level + GET_AUGMENT_PSP(ch);

    af[4].location = APPLY_STR;
    af[4].modifier = 4;
    af[4].duration = 12 * level + GET_AUGMENT_PSP(ch);

    af[5].location = APPLY_DEX;
    af[5].modifier = -2;
    af[5].duration = 12 * level + GET_AUGMENT_PSP(ch);

    to_vict = "Your body becomes like that of a great oak tree.";
    to_room = "$N's body becomes like that of a great oak tree.";

    CREATE(new_dr, struct damage_reduction_type, 1);

    new_dr->bypass_cat[0] = DR_BYPASS_CAT_DAMTYPE;
    new_dr->bypass_val[0] = DR_DAMTYPE_SLASHING;

    new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[1] = 0; /* Unused. */

    new_dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[2] = 0; /* Unused. */

    new_dr->amount = 10;
    new_dr->max_damage = -1;
    new_dr->spell = PSIONIC_OAK_BODY;
    new_dr->feat = FEAT_UNDEFINED;
    new_dr->next = GET_DR(victim);
    GET_DR(victim) = new_dr;

    accum_duration = FALSE;
    break;

  case PSIONIC_BODY_OF_IRON:
    if (has_psionic_body_form_active(ch))
    {
      send_to_char(ch, "You can only assume one psionic body form at a time.\r\n");
      return;
    }
    af[0].location = APPLY_DR;
    af[0].modifier = 0;
    af[0].duration = 12 * level;
    SET_BIT_AR(af[0].bitvector, AFF_WATER_BREATH);

    af[1].location = APPLY_RES_ACID;
    af[1].modifier = 50;
    af[1].duration = 12 * level;

    af[2].location = APPLY_RES_ELECTRIC;
    af[2].modifier = 100;
    af[2].duration = 12 * level;

    af[3].location = APPLY_RES_FIRE;
    af[3].modifier = 50;
    af[3].duration = 12 * level;

    af[4].location = APPLY_STR;
    af[4].modifier = 6;
    af[4].duration = 12 * level;

    af[5].location = APPLY_DEX;
    af[5].modifier = -6;
    af[5].duration = 12 * level;

    to_vict = "Your body becomes like solid iron.";
    to_room = "$N's body becomes like solid iron.";

    CREATE(new_dr, struct damage_reduction_type, 1);

    new_dr->bypass_cat[0] = DR_BYPASS_CAT_MATERIAL;
    new_dr->bypass_val[0] = MATERIAL_ADAMANTINE;

    new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[1] = 0; /* Unused. */

    new_dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[2] = 0; /* Unused. */

    new_dr->amount = 15;
    new_dr->max_damage = -1;
    new_dr->spell = PSIONIC_BODY_OF_IRON;
    new_dr->feat = FEAT_UNDEFINED;
    new_dr->next = GET_DR(victim);
    GET_DR(victim) = new_dr;

    accum_duration = FALSE;
    break;

  case PSIONIC_SHADOW_BODY:
    if (has_psionic_body_form_active(ch))
    {
      send_to_char(ch, "You can only assume one psionic body form at a time.\r\n");
      return;
    }
    af[0].location = APPLY_DR;
    af[0].modifier = 0;
    af[0].duration = 12 * level;
    SET_BIT_AR(af[0].bitvector, AFF_ULTRAVISION);

    af[1].location = APPLY_RES_ACID;
    af[1].modifier = 50;
    af[1].duration = 12 * level;
    SET_BIT_AR(af[1].bitvector, AFF_WATER_BREATH);

    af[2].location = APPLY_RES_ELECTRIC;
    af[2].modifier = 50;
    af[2].duration = 12 * level;
    SET_BIT_AR(af[2].bitvector, AFF_INVISIBLE);

    af[3].location = APPLY_RES_FIRE;
    af[3].modifier = 50;
    af[3].duration = 12 * level;

    af[4].location = APPLY_SKILL;
    af[4].modifier = 10;
    af[4].specific = ABILITY_STEALTH;
    af[4].duration = 12 * level;

    af[5].location = APPLY_SKILL;
    af[5].modifier = 15;
    af[5].specific = ABILITY_CLIMB;
    af[5].duration = 12 * level;

    to_vict = "Your body becomes like that of a living shadow.";
    to_room = "$N's body becomes like that of a living shadow.";

    CREATE(new_dr, struct damage_reduction_type, 1);

    new_dr->bypass_cat[0] = DR_BYPASS_CAT_MAGIC;
    new_dr->bypass_val[0] = 0;

    new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[1] = 0; /* Unused. */

    new_dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[2] = 0; /* Unused. */

    new_dr->amount = 10;
    new_dr->max_damage = -1;
    new_dr->spell = PSIONIC_SHADOW_BODY;
    new_dr->feat = FEAT_UNDEFINED;
    new_dr->next = GET_DR(victim);
    GET_DR(victim) = new_dr;

    accum_duration = FALSE;
    break;

  case PSIONIC_EPIC_PSIONIC_WARD:

    // Remove the dr.
    for (dr = GET_DR(ch); dr != NULL; dr = dr->next)
    {
      if (dr->spell == spellnum)
      {
        REMOVE_FROM_LIST(dr, GET_DR(ch), next);
      }
    }

    af[0].location = APPLY_AC_NEW;
    af[0].modifier = MIN(6, (GET_INT_BONUS(ch) / 2)) + GET_AUGMENT_PSP(ch) / 10;
    af[0].duration = 600;
    af[0].bonus_type = BONUS_TYPE_INSIGHT;

    af[1].location = APPLY_DR;
    af[1].modifier = 0;
    af[1].duration = 600;

    to_vict = "Your body becomes like that of a living shadow.";
    to_room = "$N's body becomes like that of a living shadow.";

    CREATE(new_dr, struct damage_reduction_type, 1);

    new_dr->bypass_cat[0] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[0] = 0;

    new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[1] = 0; /* Unused. */

    new_dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[2] = 0; /* Unused. */

    new_dr->amount = 5 + GET_AUGMENT_PSP(ch) / 10;
    new_dr->max_damage = -1;
    new_dr->spell = PSIONIC_EPIC_PSIONIC_WARD;
    new_dr->feat = FEAT_UNDEFINED;
    new_dr->next = GET_DR(victim);
    GET_DR(victim) = new_dr;
    break;

  case PSIONIC_TRUE_METABOLISM:
    af[0].duration = 12 * level;
    af[0].location = APPLY_SPECIAL;
    break;

  case PSIONIC_PSYCHOSIS: /* AoE */
    if (!can_confuse(victim))
    {
      send_to_char(ch, "Your opponent seems to be immune to confusion effects.\r\n");
      return;
    }
    if (power_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NOSCHOOL))
    {
      return;
    }
    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;
    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_CONFUSED);
    accum_duration = FALSE;
    to_vict = "You feel confused and disoriented.";
    break;

    // spells and other effects

  case SPELL_ACID_SHEATH: // divination
    if (affected_by_spell(victim, SPELL_FIRE_SHIELD) ||
        affected_by_spell(victim, SPELL_COLD_SHIELD))
    {
      send_to_char(ch, "You are already affected by an elemental shield!\r\n");
      return;
    }

    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_ASHIELD);

    accum_duration = FALSE;
    to_vict = "A shield of acid surrounds you.";
    to_room = "$n is surrounded by shield of acid.";
    break;

  case SPELL_DIVINE_FAVOR:

    af[0].location = APPLY_HITROLL;
    af[0].modifier = MIN(3, MAX(1, level / 3));
    af[0].duration = 10;
    af[0].bonus_type = BONUS_TYPE_LUCK;

    af[1].location = APPLY_DAMROLL;
    af[1].modifier = MIN(3, MAX(1, level / 3));
    af[1].duration = 10;
    af[1].bonus_type = BONUS_TYPE_LUCK;
    to_vict = "A feeling of divine favor fills you.";
    to_room = "$n appears bolstered and strengthened.";
    break;

  case SPELL_AID:
    if (affected_by_spell(victim, SPELL_PRAYER))
    {
      send_to_char(ch, "The target is already blessed!\r\n");
      return;
    }
    if (affected_by_spell(victim, SPELL_BLESS))
    {
      affect_from_char(victim, SPELL_BLESS);
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 1;
    af[0].duration = 300;
    af[0].bonus_type = BONUS_TYPE_MORALE;

    af[1].location = APPLY_SAVING_WILL;
    af[1].modifier = 2;
    af[1].duration = 300;
    af[1].bonus_type = BONUS_TYPE_MORALE;

    af[2].location = APPLY_HIT;
    af[2].modifier = dice(2, 6) + MAX(level, 15);
    af[2].duration = 300;
    af[2].bonus_type = BONUS_TYPE_MORALE;

    to_room = "$n is now divinely aided!";
    to_vict = "You feel divinely aided.";
    break;

  case SPELL_SHIELD_OF_FAITH:

    af[0].location = APPLY_AC_NEW;
    af[0].modifier = 2;
    af[0].duration = 400;
    af[0].bonus_type = BONUS_TYPE_DEFLECTION;
    to_vict = "You feel someone protecting you.";
    to_room = "$n is surrounded by a shield of faith!";
    break;

  case SPELL_RIGHTEOUS_VIGOR:
    af[0].location = APPLY_HITROLL;
    af[0].modifier = 1;
    af[0].duration = level;
    af[0].bonus_type = BONUS_TYPE_MORALE;
    to_vict = "Your accuracy improves.";
    to_room = "$n looks a little stronger.";
    break;

  case SPELL_LITANY_OF_DEFENSE:
    if (affected_by_spell(ch, SPELL_LITANY_OF_RIGHTEOUSNESS))
    {
      send_to_char(ch, "This spell cannot be cast when you are already under the effect of a 'litany' type spell.\r\n");
      return;
    }
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = 0;
    af[0].duration = 10;
    af[0].bonus_type = BONUS_TYPE_MORALE;
    to_vict = "The defensive prowess of your armor is increased.";
    to_room = "The defensive prowess of $n's armor is increased.";
    break;

  case SPELL_LITANY_OF_RIGHTEOUSNESS:
    if (affected_by_spell(ch, SPELL_LITANY_OF_DEFENSE))
    {
      send_to_char(ch, "This spell cannot be cast when you are already under the effect of a 'litany' type spell.\r\n");
      return;
    }
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = 0;
    af[0].duration = 3;
    af[0].bonus_type = BONUS_TYPE_MORALE;
    to_vict = "You create an aura of righteousness around you and your allies.";
    to_room = "$n creates an aura of righteousness around $m and $s allies.";
    break;

  case SPELL_VEIL_OF_POSITIVE_ENERGY:
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = 2;
    af[0].duration = level * 100;
    af[0].bonus_type = BONUS_TYPE_SACRED;
    to_vict = "A translucent veil of yellow light envelops you.";
    to_room = "A translucent veil of yellow light envelops $n.";
    break;

  case SPELL_BESTOW_WEAPON_PROFICIENCY:
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = 1;
    af[0].duration = level * 100;
    af[0].bonus_type = BONUS_TYPE_SACRED;
    to_vict = "A wave of knowledge and muscle memory surge inside you.";
    to_room = "A wave of energy flows over $n, and $e looks more confident.";
    break;

  case SPELL_TACTICAL_ACUMEN:
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = MAX(2, MIN(level / 4, 5));
    af[0].duration = 600 * level;
    af[0].bonus_type = BONUS_TYPE_INSIGHT;
    to_vict = "You feel your tactical abilities increase.";
    to_room = "$n seems more aware and attuned to the circumstances.";
    break;

  case SPELL_STUNNING_BARRIER:
    af[0].location = APPLY_AC_NEW;
    af[0].modifier = 1;
    af[0].duration = 10 * level;
    af[0].bonus_type = BONUS_TYPE_DEFLECTION;
    af[1].location = APPLY_SAVING_WILL;
    af[1].modifier = 1;
    af[1].duration = 10 * level;
    af[1].bonus_type = BONUS_TYPE_RESISTANCE;
    af[2].location = APPLY_SAVING_FORT;
    af[2].modifier = 1;
    af[2].duration = 10 * level;
    af[2].bonus_type = BONUS_TYPE_RESISTANCE;
    af[3].location = APPLY_SAVING_REFL;
    af[3].modifier = 1;
    af[3].duration = 10 * level;
    af[3].bonus_type = BONUS_TYPE_RESISTANCE;
    to_vict = "You feel protected by a shimmering, pulsing field.";
    to_room = "$n is surrounded by a shimmering, pulsing field!";
    break;

  case SPELL_SHIELD_OF_FORTIFICATION:
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = 1;
    af[0].duration = 10 * level;
    af[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
    to_vict = "You feel fortified against critical hits and sneak attacks!";
    to_room = "$n is fortified by a shifting, transparent field!";
    break;

  case SPELL_SUN_METAL:
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = 2;
    af[0].duration = 50 + (1 * level);
    af[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
    to_vict = "Flames surround your weapons!";
    to_room = "Flames surround $n's weapons!";
    break;

  case SPELL_RESISTANCE:

    af[0].location = APPLY_SAVING_REFL;
    af[0].modifier = 1;
    af[0].duration = 12;
    af[0].bonus_type = BONUS_TYPE_RESISTANCE;
    af[1].location = APPLY_SAVING_FORT;
    af[1].modifier = 1;
    af[1].duration = 12;
    af[1].bonus_type = BONUS_TYPE_RESISTANCE;
    af[2].location = APPLY_SAVING_WILL;
    af[2].modifier = 1;
    af[2].duration = 12;
    af[2].bonus_type = BONUS_TYPE_RESISTANCE;
    to_vict = "You feel more resistant!";
    to_room = "$n appears more durable!";
    break;

  case SPELL_HEDGING_WEAPONS:
    af[0].location = APPLY_AC_NEW;
    af[0].modifier = 2 + MIN(3, (level - 6) / 4);
    af[0].duration = 10 * level;
    af[0].bonus_type = BONUS_TYPE_DEFLECTION;
    to_vict = "You are surrounded by floating defending weapons!";
    to_room = "$n is surrounded by floating, defending weapons!";
    break;

  case SPELL_HONEYED_TONGUE:
    af[0].location = APPLY_SKILL;
    af[0].specific = ABILITY_DIPLOMACY;
    af[0].modifier = 5;
    af[0].duration = 100 * level;
    af[0].bonus_type = BONUS_TYPE_COMPETENCE;
    to_vict = "You are bolstered with a silver tongue!";
    to_room = "$n looks more confident and personable!";
    break;

  case SPELL_BARKSKIN: // transmutation

    af[0].location = APPLY_AC_NEW;
    if (level >= 12)
      af[0].modifier = 5;
    else if (level >= 9)
      af[0].modifier = 4;
    else if (level >= 6)
      af[0].modifier = 3;
    else
      af[0].modifier = 2;
    af[0].duration = (level * 200); // divine level * 12, * 20 for minutes
    af[0].bonus_type = BONUS_TYPE_NATURALARMOR;
    accum_affect = FALSE;
    accum_duration = FALSE;
    to_vict = "Your skin hardens to bark.";
    to_room = "$n skin hardens to bark!";
    break;

  case SPELL_BATTLETIDE: // divine
    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_BATTLETIDE);

    af[1].duration = 50;
    SET_BIT_AR(af[1].bitvector, AFF_HASTE);

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 3;
    af[0].duration = 50;

    af[1].location = APPLY_DAMROLL;
    af[1].modifier = 3;
    af[1].duration = 50;

    accum_duration = FALSE;
    to_vict = "You feel the tide of battle turn in your favor!";
    to_room = "The tide of battle turns in $n's favor!";
    break;

  case SPELL_BLESS:
    if (affected_by_spell(victim, SPELL_AID) ||
        affected_by_spell(victim, SPELL_PRAYER))
    {
      send_to_char(ch, "The target is already blessed!\r\n");
      return;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 1;
    af[0].duration = 300;
    af[0].bonus_type = BONUS_TYPE_MORALE;

    af[1].location = APPLY_SAVING_WILL;
    af[1].modifier = 1;
    af[1].duration = 300;
    af[1].bonus_type = BONUS_TYPE_MORALE;

    to_room = "$n is now righteous!";
    to_vict = "You feel righteous.";
    break;

  case SPELL_BLINDNESS: // necromancy
    if (!can_blind(victim))
    {
      send_to_char(ch, "Your opponent doesn't seem blindable.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, NECROMANCY))
    {
      send_to_char(ch, "The blindness is resisted!\r\n");
      return;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = -4;
    af[0].duration = 25;
    SET_BIT_AR(af[0].bitvector, AFF_BLIND);

    af[1].location = APPLY_AC_NEW;
    af[1].modifier = -4;
    af[1].duration = 25;
    SET_BIT_AR(af[1].bitvector, AFF_BLIND);

    to_room = "$n seems to be blinded!";
    to_vict = "You have been blinded!";
    break;

  case SPELL_BLINDING_RAY: // evocation
    if (!can_blind(victim))
    {
      send_to_char(ch, "Your opponent doesn't seem blindable.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, EVOCATION))
    {
      send_to_char(ch, "The blindness is resisted!\r\n");
      return;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = -4;
    af[0].duration = HAS_FEAT(victim, FEAT_LIGHT_BLINDNESS) ? dice(1, 4) : 1;
    SET_BIT_AR(af[0].bitvector, AFF_BLIND);

    af[1].location = APPLY_AC_NEW;
    af[1].modifier = -4;
    af[1].duration = HAS_FEAT(victim, FEAT_LIGHT_BLINDNESS) ? dice(1, 4) : 1;
    af[1].bonus_type = BONUS_TYPE_DODGE;
    SET_BIT_AR(af[1].bitvector, AFF_BLIND);

    accum_duration = TRUE;

    to_room = "An intensely bright ray of light has blinded $n!";
    to_vict = "An intensely bright ray of light has blinded you!";
    break;

  case SPELL_WEAPON_OF_AWE: // transmutation

    af[0].duration = level * 10;
    af[0].location = APPLY_DAMROLL;
    af[0].modifier = 2;
    af[0].bonus_type = BONUS_TYPE_SACRED;
    to_room = "$n's weapons glow bright yellow!";
    to_vict = "Your weapons begin to glow bright yellow!";
    break;

  case SPELL_SILENCE: // illusion
    if (!can_silence(victim))
    {
      send_to_char(ch, "Your opponent doesn't seem prone to being silenced.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, ILLUSION))
    {
      send_to_char(ch, "The silencing is resisted!\r\n");
      return;
    }

    af[0].duration = level;
    SET_BIT_AR(af[0].bitvector, AFF_SILENCED);
    break;

    // These two are the same except for messages
  case SPELL_SPIRITUAL_WEAPON:
    to_room = "A spiritual weapon appears beside $n!";
    to_vict = "A spiritual weapons appears beside you!";
  case SPELL_DANCING_WEAPON:
    to_room = "A dancing weapon appears beside $n!";
    to_vict = "A dancing weapons appears beside you!";
    af[0].duration = level;
    break;

  case SPELL_GREATER_MAGIC_WEAPON:
    af[0].duration = 60 * 10 * level; // 60 minutes per level
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = MIN(5, level / 4);
    to_vict = "Your weapons glow with magical energy.";
    to_room = "$n's weapons glow with magical energy.";
    break;

  case SPELL_MAGIC_VESTMENT:
    af[0].duration = 60 * 10 * level; // 60 minutes per level
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = MIN(5, level / 4);
    to_vict = "Your armor glows with magical energy.";
    to_room = "$n's armor glows with magical energy.";
    break;

  case SPELL_PROTECTION_FROM_ENERGY:
    af[0].duration = 10 * 10 * level; // 10 minutes per level
    af[0].location = APPLY_SPECIAL;
    af[0].modifier = level;
    to_vict = "A shield of shimmering force envelops you.";
    to_room = "A shield of shimmering force envelops $n.";
    break;

  case SPELL_KEEN_EDGE:
    af[0].duration = 10 * 10 * level; // 10 minutes per level
    af[0].location = APPLY_SPECIAL;
    to_vict = "Your weapons obtain a fine, keen edge.";
    to_room = "$n's weapons become extremely keen and sharp.";
    break;

  case SPELL_WEAPON_OF_IMPACT:
    af[0].duration = 10 * 10 * level; // 10 minutes per level
    af[0].location = APPLY_SPECIAL;
    to_vict = "Your weapons begin to glow bright blue.";
    to_room = "$n's weapons begin to glow bright blue.";
    break;

  case SPELL_DOOM: // necromancy
    if (is_immune_fear(ch, victim, 1))
      return;
    if (is_immune_mind_affecting(ch, victim, 1))
      return;
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NECROMANCY))
    {
      send_to_char(ch, "The sense of doom is resisted!\r\n");
      return;
    }

    af[0].location = APPLY_NONE;
    af[0].modifier = 0;
    af[0].duration = 10 * level;
    SET_BIT_AR(af[0].bitvector, AFF_SHAKEN);

    to_room = "$n has been filled with a sense of doom!";
    to_vict = "You feel overwhelming feelings of doom!";
    break;

  case SPELL_BLUR:               // illusion
    af[0].location = APPLY_NONE; /* this is just a tag */
    af[0].modifier = 0;
    af[0].duration = 300;
    to_room = "$n's images becomes blurry!.";
    to_vict = "You observe as your image becomes blurry.";
    SET_BIT_AR(af[0].bitvector, AFF_BLUR);
    accum_duration = FALSE;
    break;

  case SPELL_BRAVERY:
    af[0].duration = 25 + level;
    SET_BIT_AR(af[0].bitvector, AFF_BRAVERY);

    to_vict = "You suddenly feel very brave.";
    to_room = "$n suddenly feels very brave.";
    break;

  case SPELL_CHARISMA: // transmutation
    if (affected_by_spell(victim, SPELL_MASS_CHARISMA))
    {
      send_to_char(ch, "Your target already has a charisma spell in effect.\r\n");
      return;
    }
    af[0].location = APPLY_CHA;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 4;
    accum_duration = FALSE;
    to_vict = "You feel more charismatic!";
    to_room = "$n's charisma increases!";
    break;

  case SPELL_DIVINE_POWER: // evocation

    af[0].location = APPLY_STR;
    af[0].duration = level;
    af[0].modifier = level / 6;
    af[0].bonus_type = BONUS_TYPE_LUCK;

    af[1].location = APPLY_HITROLL;
    af[1].duration = level;
    af[1].modifier = level / 6;
    af[1].bonus_type = BONUS_TYPE_LUCK;

    af[2].location = APPLY_DAMROLL;
    af[2].duration = level;
    af[2].modifier = level / 6;
    af[2].bonus_type = BONUS_TYPE_LUCK;

    af[3].location = APPLY_HIT;
    af[3].duration = level;
    af[3].modifier = level;
    af[3].bonus_type = BONUS_TYPE_LUCK;

    to_vict = "You feel divine power fill you!";
    to_room = "$n is filled with divine power!";
    break;

  case SPELL_CHILL_TOUCH: // necromancy
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, NECROMANCY))
      return;

    af[0].location = APPLY_STR;
    af[0].duration = 4 + level;
    af[0].modifier = -2;
    accum_duration = TRUE;
    to_room = "$n's strength is withered!";
    to_vict = "You feel your strength wither!";
    break;

  case SPELL_COLD_SHIELD: // evocation
    if (affected_by_spell(victim, SPELL_ACID_SHEATH) ||
        affected_by_spell(victim, SPELL_FIRE_SHIELD))
    {
      send_to_char(ch, "You are already affected by an elemental shield!\r\n");
      return;
    }
    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_CSHIELD);

    accum_duration = FALSE;
    to_vict = "A shield of ice surrounds you.";
    to_room = "$n is surrounded by shield of ice.";
    break;

  case SPELL_COLOR_SPRAY: // enchantment
    if (!can_stun(victim))
    {
      send_to_char(ch, "It seems your opponent cannot be stunned.\r\n");
      return;
    }
    if (GET_LEVEL(victim) > 5)
    {
      send_to_char(ch, "Your target is too powerful to be stunned by this illusion.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, illusion_bonus, casttype, level, ENCHANTMENT))
    {
      return;
    }

    SET_BIT_AR(af[0].bitvector, AFF_STUN);
    af[0].duration = dice(1, 4);
    to_room = "$n is stunned by the colors!";
    to_vict = "You are stunned by the colors!";
    break;

  case SPELL_CONTAGION: // necromancy
    if (!can_disease(victim))
    {
      send_to_char(ch, "It seems your opponent is not susceptible to disease.\r\n");
      return;
    }
    switch (rand_number(1, 7))
    {
    case 1: // blinding sickness
      af[0].location = APPLY_STR;
      af[0].modifier = -1 * dice(1, 4);
      to_vict = "You are overcome with a blinding sickness.";
      to_room = "$n is overcome with a blinding sickness.";
      break;
    case 2: // cackle fever
      af[0].location = APPLY_WIS;
      af[0].modifier = -1 * dice(1, 6);
      to_vict = "You suddenly come down with cackle fever.";
      to_room = "$n suddenly comes down with cackle fever.";
      break;
    case 3: // filth fever
      af[0].location = APPLY_DEX;
      af[0].modifier = -1 * dice(1, 3);
      SET_BIT_AR(af[1].bitvector, AFF_DISEASE);
      af[1].location = APPLY_CON;
      af[1].modifier = -1 * dice(1, 3);
      af[1].duration = 300;
      to_vict = "You suddenly come down with filth fever.";
      to_room = "$n suddenly comes down with filth fever.";
      break;
    case 4: // mindfire
      af[0].location = APPLY_INT;
      af[0].modifier = -1 * dice(1, 4);
      to_vict = "You feel your mind start to burn incessantly.";
      to_room = "$n's mind starts to burn incessantly.";
      break;
    case 5: // red ache
      af[0].location = APPLY_STR;
      af[0].modifier = -1 * dice(1, 6);
      to_vict = "You suddenly feel weakened by the red ache.";
      to_room = "$n suddenly feels weakened by the red ache.";
      break;
    case 6: // shakes
      af[0].location = APPLY_DEX;
      af[0].modifier = -1 * dice(1, 8);
      to_vict = "You feel yourself start to uncontrollably shake.";
      to_room = "$n starts to shake uncontrollably.";
      break;
    case 7: // slimy doom
      af[0].location = APPLY_CON;
      af[0].modifier = -1 * dice(1, 4);
      to_vict = "You feel yourself affected by the slimy doom.";
      to_room = "$n feels affected by the slimy doom.";
      break;
    }
    SET_BIT_AR(af[0].bitvector, AFF_DISEASE);
    af[0].duration = 300; // 15 real minutes (supposed to be permanent)
    accum_affect = FALSE;
    accum_duration = FALSE;
    break;

  case SPELL_CUNNING: // transmutation
    if (affected_by_spell(victim, SPELL_MASS_CUNNING))
    {
      send_to_char(ch, "Your target already has a cunning spell in effect.\r\n");
      return;
    }
    af[0].location = APPLY_INT;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 4;
    af[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
    to_vict = "You feel more intelligent!";
    to_room = "$n's intelligence increases!";
    break;

  case SPELL_CURSE: // necromancy
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, NECROMANCY))
    {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }

    af[0].location = APPLY_HITROLL;
    af[0].duration = 25 + (level * 12);
    af[0].modifier = -2;
    SET_BIT_AR(af[0].bitvector, AFF_CURSE);

    af[1].location = APPLY_DAMROLL;
    af[1].duration = 25 + (level * 12);
    af[1].modifier = -2;
    SET_BIT_AR(af[1].bitvector, AFF_CURSE);

    af[2].location = APPLY_SAVING_WILL;
    af[2].duration = 25 + (level * 12);
    af[2].modifier = -2;
    SET_BIT_AR(af[2].bitvector, AFF_CURSE);

    accum_duration = TRUE;
    accum_affect = TRUE;
    to_room = "$n briefly glows red!";
    to_vict = "You feel very uncomfortable.";
    break;

  case SPELL_DAZE_MONSTER: // enchantment
    if (!can_stun(victim))
    {
      send_to_char(ch, "It seems your opponent cannot be stunned.\r\n");
      return;
    }
    if (GET_LEVEL(victim) > 8)
    {
      send_to_char(ch, "Your target is too powerful to be affected by this illusion.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, enchantment_bonus, casttype, level, ENCHANTMENT))
    {
      return;
    }

    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    is_mind_affect = TRUE;

    SET_BIT_AR(af[0].bitvector, AFF_STUN);
    af[0].duration = dice(2, 4);
    to_room = "$n is dazed by the spell!";
    to_vict = "You are dazed by the spell!";
    break;

  case SPELL_DEAFNESS: // necromancy
    if (!can_deafen(victim))
    {
      send_to_char(ch, "Your opponent doesn't seem deafable.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, NECROMANCY))
    {
      send_to_char(ch, "You fail.\r\n");
      return;
    }

    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_DEAF);

    to_room = "$n seems to be deafened!";
    to_vict = "You have been deafened!";
    break;

  case SPELL_DEATH_WARD: // necromancy
    af[0].duration = 12 * level;
    SET_BIT_AR(af[0].bitvector, AFF_DEATH_WARD);

    accum_affect = FALSE;
    accum_duration = FALSE;
    to_room = "$n is warded against death magic!";
    to_vict = "You are warded against death magic!";
    break;

  case SPELL_DEEP_SLUMBER: // enchantment

    if ((!IS_PIXIE(ch) && GET_LEVEL(victim) >= 15) || is_immune_sleep)
    {
      send_to_char(ch, "The target is too powerful for this enchantment!\r\n");
      return;
    }
    if (sleep_immunity(victim))
    {
      send_to_char(ch, "Your target is unfazed.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP))
    {
      send_to_char(ch, "Your victim doesn't seem vulnerable to your spell.");
      return;
    }
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, ENCHANTMENT))
    {
      return;
    }

    af[0].duration = 100 + (level * 6);
    SET_BIT_AR(af[0].bitvector, AFF_SLEEP);

    if (GET_POS(victim) > POS_SLEEPING)
    {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      if (FIGHTING(victim))
        stop_fighting(victim);
      change_position(victim, POS_SLEEPING);
      if (FIGHTING(ch) == victim)
        stop_fighting(ch);
    }
    break;

  case SPELL_UNDETECTABLE_ALIGNMENT:
    af[0].duration = 24 * 60 * 10; // 24 hours
    SET_BIT_AR(af[0].bitvector, AFF_HIDE_ALIGNMENT);
    to_vict = "Your alignment becomes magically hidden from divining attempts.";
    break;

  case SPELL_DETECT_ALIGN:
    af[0].duration = 300 + level * 25;
    SET_BIT_AR(af[0].bitvector, AFF_DETECT_ALIGN);
    to_room = "$n's eyes become sensitive to motives!";
    to_vict = "Your eyes become sensitive to motives.";
    break;

  case SPELL_DETECT_INVIS: // divination
    af[0].duration = 300 + level * 25;
    SET_BIT_AR(af[0].bitvector, AFF_DETECT_INVIS);
    to_vict = "Your eyes tingle, now sensitive to invisibility.";
    to_room = "$n's eyes become sensitive to invisibility!";
    break;

  case SPELL_DETECT_MAGIC: // divination
    af[0].duration = 300 + level * 25;
    SET_BIT_AR(af[0].bitvector, AFF_DETECT_MAGIC);
    to_room = "$n's eyes become sensitive to magic!";
    to_vict = "Magic becomes clear as your eyes tingle.";
    break;

  case SPELL_DIMENSIONAL_LOCK: // divination
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, DIVINATION))
    {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }

    af[0].duration = (level + 12);
    SET_BIT_AR(af[0].bitvector, AFF_DIM_LOCK);
    to_room = "$n is bound to this dimension!";
    to_vict = "You feel yourself bound to this dimension!";
    break;

  case SPELL_DISPLACEMENT:       // illusion
    af[0].location = APPLY_NONE; /* this is just a tag */
    af[0].modifier = 0;
    af[0].duration = 100;
    to_room = "$n's images becomes displaced!";
    to_vict = "You observe as your image becomes displaced!";
    SET_BIT_AR(af[0].bitvector, AFF_DISPLACE);
    accum_duration = FALSE;
    break;

  case SPELL_ENDURANCE: // transmutation
    af[0].location = APPLY_CON;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 2 + (level / 5);
    af[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
    to_vict = "You feel more hardy!";
    to_room = "$n begins to feel more hardy!";
    break;

  case SPELL_ENDURE_ELEMENTS: // abjuration
    af[0].duration = 600;
    SET_BIT_AR(af[0].bitvector, AFF_ELEMENT_PROT);
    to_vict = "You feel a slight protection from the elements!";
    to_room = "$n begins to feel slightly protected from the elements!";
    break;

  case SPELL_ENFEEBLEMENT: // enchantment
    if (mag_resistance(ch, victim, 0))
      return;

    if (mag_savingthrow(ch, victim, SAVING_FORT, (enchantment_bonus - 4), casttype, level, ENCHANTMENT))
    {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }

    af[0].location = APPLY_STR;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = -(2 + (level / 5));

    af[1].location = APPLY_DEX;
    af[1].duration = (level * 12) + 100;
    af[1].modifier = -(2 + (level / 5));

    af[2].location = APPLY_CON;
    af[2].duration = (level * 12) + 100;
    af[2].modifier = -(2 + (level / 5));

    accum_duration = FALSE;
    accum_affect = FALSE;
    to_room = "$n is terribly enfeebled!";
    to_vict = "You feel terribly enfeebled!";
    break;

  case SPELL_ENLARGE_PERSON: // transmutation
    af[0].location = APPLY_SIZE;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 1;
    af[0].bonus_type = BONUS_TYPE_SIZE;
    to_vict = "You feel yourself growing!";
    to_room = "$n's begins to grow much larger!";
    break;

  case SPELL_EPIC_MAGE_ARMOR: // epic
    if (affected_by_spell(victim, SPELL_MAGE_ARMOR))
    {
      affect_from_char(victim, SPELL_MAGE_ARMOR);
    }

    af[0].location = APPLY_AC_NEW;
    af[0].modifier = 12;
    af[0].duration = 2400;
    af[0].bonus_type = BONUS_TYPE_ARMOR;

    accum_duration = FALSE;
    to_vict = "You feel magic protecting you.";
    to_room = "$n is surrounded by magical bands of armor!";
    break;

  case SPELL_EPIC_WARDING: // no school
    if (affected_by_spell(victim, SPELL_STONESKIN))
    {
      affect_from_char(victim, SPELL_STONESKIN);
    }
    if (affected_by_spell(victim, SPELL_IRONSKIN))
    {
      affect_from_char(victim, SPELL_IRONSKIN);
    }

    SET_BIT_AR(af[0].bitvector, AFF_WARDED);
    af[0].duration = 600;
    to_room = "$n becomes surrounded by a powerful magical ward!";
    to_vict = "You become surrounded by a powerful magical ward!";
    GET_STONESKIN(victim) = level * 60;
    break;

  case SPELL_EXPEDITIOUS_RETREAT: // transmutation
    af[0].location = APPLY_MOVE;
    af[0].modifier = 500;
    af[0].duration = level * 2;
    af[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
    to_vict = "You feel expeditious.";
    to_room = "$n is now expeditious!";
    break;

  case SPELL_EYEBITE: // necromancy
    if (!can_disease(victim))
    {
      send_to_char(ch, "It seems your opponent is not susceptible to disease.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, NECROMANCY))
    {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }

    af[0].duration = 600;
    SET_BIT_AR(af[0].bitvector, AFF_DISEASE);
    to_vict = "You feel a powerful necromantic disease overcome you.";
    to_room = "$n suffers visibly as a powerful necromantic disease strikes $m!";
    break;

  case SPELL_FAERIE_FIRE: // evocation
    if (mag_resistance(ch, victim, 0))
      return;

    // need to make this show an outline around concealed, blue, displaced, invisible people
    SET_BIT_AR(af[0].bitvector, AFF_FAERIE_FIRE);
    af[0].duration = level;
    af[0].location = APPLY_AC_NEW;
    af[0].modifier = -2;
    accum_duration = FALSE;
    accum_affect = FALSE;
    to_room = "A pale blue light begins to glow around $n.";
    to_vict = "You are suddenly surrounded by a pale blue light.";
    break;

  case SPELL_FALSE_LIFE: // necromancy
    af[0].location = APPLY_HIT;
    af[0].modifier = 30;
    af[0].duration = 300;
    af[0].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

    to_room = "$n grows strong with \tDdark\tn life!";
    to_vict = "You grow strong with \tDdark\tn life!";
    break;

  case SPELL_FEEBLEMIND: // enchantment
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, enchantment_bonus, casttype, level, ENCHANTMENT))
    {
      return;
    }

    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    is_mind_affect = TRUE;

    af[0].location = APPLY_INT;
    af[0].duration = level;
    af[0].modifier = -(GET_REAL_INT(victim) - 3);

    af[1].location = APPLY_WIS;
    af[1].duration = level;
    af[1].modifier = -(GET_REAL_WIS(victim) - 3);

    af[2].location = APPLY_CHA;
    af[2].duration = level;
    af[2].modifier = -(GET_REAL_CHA(victim) - 3);

    to_room = "$n grasps $s head in pain, $s eyes glazing over!";
    to_vict = "Your head starts to throb and a wave of confusion washes over you.";
    break;

  case SPELL_CONFUSION: // enchantment
    if (!can_confuse(victim))
    {
      send_to_char(ch, "Your opponent seems to be immune to confusion effects.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, enchantment_bonus, casttype, level, ENCHANTMENT))
    {
      return;
    }
    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    SET_BIT_AR(af[0].bitvector, AFF_CONFUSED);
    af[0].duration = level;
    victim->confuser_idnum = GET_IDNUM(ch);
    to_room = "A look of utter confusion washes over $n's face.";
    to_vict = "You find yourself completely confused and disoriented.";
    break;

  case SPELL_ENTANGLE: // transmutation
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, TRANSMUTATION))
      return;

    SET_BIT_AR(af[0].bitvector, AFF_ENTANGLED);
    af[0].duration = 10 + level;
    af[0].location = APPLY_DEX;
    af[0].modifier = -2;
    accum_duration = FALSE;
    to_room = "$n is grasped by waving and entangling vines!";
    to_vict = "You are grasped by waving and entangling vines!";
    break;

  case SPELL_FIRE_SHIELD: // evocation
    if (affected_by_spell(victim, SPELL_ACID_SHEATH) ||
        affected_by_spell(victim, SPELL_COLD_SHIELD))
    {
      send_to_char(ch, "You are already affected by an elemental shield!\r\n");
      return;
    }
    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_FSHIELD);

    accum_duration = FALSE;
    to_vict = "A shield of flames surrounds you.";
    to_room = "$n is surrounded by shield of flames.";
    break;

  case SPELL_GASEOUS_FORM:
    af[0].duration = 2 * 10 * level; // 2 minutes / level
    SET_BIT_AR(af[0].bitvector, AFF_IMMATERIAL);
    af[1].duration = 2 * 10 * level; // 2 minutes / level
    SET_BIT_AR(af[1].bitvector, AFF_LEVITATE);
    to_room = "$n fades into a gaseous form.";
    to_vict = "You fade into a gaseous form.";
    break;

  case SPELL_FLY:
    af[0].duration = 600;
    SET_BIT_AR(af[0].bitvector, AFF_FLYING);
    to_room = "$n begins to fly above the ground!";
    to_vict = "You fly above the ground.";
    break;

  case SPELL_AIR_WALK:
    af[0].duration = 10 * 10 * level; // 10 minutes / level
    SET_BIT_AR(af[0].bitvector, AFF_FLYING);
    to_room = "$n begins to walk on air!";
    to_vict = "You begin to walk in air.";
    break;

  case SPELL_FREE_MOVEMENT:
    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_FREE_MOVEMENT);

    accum_duration = FALSE;
    to_vict = "Your limbs feel looser as the free movement spell takes effect.";
    to_room = "$n's limbs now move freer.";
    break;

  case SPELL_GLOBE_OF_INVULN: // abjuration
    if (affected_by_spell(victim, SPELL_MINOR_GLOBE))
    {
      affect_from_char(victim, SPELL_MINOR_GLOBE);
    }

    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_GLOBE_OF_INVULN);

    accum_duration = FALSE;
    to_vict = "A globe of invulnerability surrounds you.";
    to_room = "$n is surrounded by a globe of invulnerability.";
    break;

  case SPELL_GRACE: // transmutation
    af[0].location = APPLY_DEX;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 2 + (level / 5);
    af[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
    to_vict = "You feel more dextrous!";
    to_room = "$n's appears to be more dextrous!";
    break;

  case SPELL_GRASPING_HAND: // evocation (also does damage)
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, EVOCATION))
      return;

    SET_BIT_AR(af[0].bitvector, AFF_ENTANGLED);
    af[0].duration = dice(2, 4) - 1;
    accum_duration = FALSE;
    to_room = "$n's is grasped by the spell!";
    to_vict = "You are grasped by the magical hand!";
    break;

  case SPELL_GREASE: // divination
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, DIVINATION))
    {
      send_to_char(ch, "You fail.\r\n");
      return;
    }

    af[0].location = APPLY_MOVE;
    af[0].modifier = (-20 - level) * 10;
    af[0].duration = level * 2;
    to_vict = "Your feet are all greased up!";
    to_room = "$n now has greasy feet!";
    break;

  case SPELL_GREATER_HEROISM: // enchantment
    if (affected_by_spell(victim, SPELL_HEROISM))
    {
      affect_from_char(victim, SPELL_HEROISM);
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 4;
    af[0].duration = 300;

    af[1].location = APPLY_SAVING_WILL;
    af[1].modifier = 4;
    af[1].duration = 300;

    af[2].location = APPLY_SAVING_FORT;
    af[2].modifier = 4;
    af[2].duration = 300;

    af[3].location = APPLY_SAVING_REFL;
    af[3].modifier = 4;
    af[3].duration = 300;

    to_room = "$n is now very heroic!";
    to_vict = "You feel very heroic.";
    break;

  case SPELL_GREATER_INVIS: // illusion
    if (!victim)
      victim = ch;

    af[0].duration = 10 + (level * 6);
    af[0].modifier = 4;
    af[0].location = APPLY_AC_NEW;
    SET_BIT_AR(af[0].bitvector, AFF_INVISIBLE);
    to_vict = "You vanish.";
    to_room = "$n slowly fades out of existence.";
    break;

  case SPELL_GREATER_MAGIC_FANG:
    if (!IS_ANIMAL(victim))
    {
      send_to_char(ch, "Magic fang can only be cast upon animals.\r\n");
      return;
    }
    af[0].location = APPLY_HITROLL;
    if (CLASS_LEVEL(ch, CLASS_DRUID) >= 20)
      af[0].modifier = 5;
    else if (CLASS_LEVEL(ch, CLASS_DRUID) >= 16)
      af[0].modifier = 4;
    else if (CLASS_LEVEL(ch, CLASS_DRUID) >= 12)
      af[0].modifier = 3;
    else if (CLASS_LEVEL(ch, CLASS_DRUID) >= 8)
      af[0].modifier = 2;
    else
      af[0].modifier = 1;
    af[0].duration = 5 * level;

    to_room = "$n is now affected by magic fang!";
    to_vict = "You are suddenly empowered by magic fang.";
    break;

  case SPELL_MINOR_ILLUSION: // illusion
    if (affected_by_spell(victim, SPELL_MINOR_ILLUSION))
    {
      send_to_char(ch, "You already have an illusory double.\r\n");
      return;
    }
    af[0].duration = 300;
    SET_BIT_AR(af[0].bitvector, AFF_MIRROR_IMAGED);
    to_room = "$n grins as a duplicate image pops up and smiles!";
    to_vict = "You watch as a duplicate image pops up and smiles at you!";
    GET_IMAGES(victim) = 1;
    break;

  case SPELL_GREATER_MIRROR_IMAGE: // illusion
    if (affected_by_spell(victim, SPELL_MINOR_ILLUSION))
    {
      affect_from_char(victim, SPELL_MINOR_ILLUSION);
    }
    if (affected_by_spell(victim, SPELL_MIRROR_IMAGE))
    {
      affect_from_char(victim, SPELL_MIRROR_IMAGE);
    }
    if (affected_by_spell(victim, SPELL_GREATER_MIRROR_IMAGE))
    {
      affect_from_char(victim, SPELL_GREATER_MIRROR_IMAGE);
    }

    af[0].duration = 300;
    SET_BIT_AR(af[0].bitvector, AFF_MIRROR_IMAGED);
    to_room = "$n grins as multiple images pop up and smile!";
    to_vict = "You watch as multiple images pop up and smile at you!";
    GET_IMAGES(victim) = 6 + (level / 3);
    break;

  case SPELL_GREATER_SPELL_MANTLE: // abjuration
    if (affected_by_spell(victim, SPELL_MANTLE))
    {
      affect_from_char(victim, SPELL_MANTLE);
    }

    af[0].duration = level * 4;
    SET_BIT_AR(af[0].bitvector, AFF_SPELL_MANTLE);
    GET_SPELL_MANTLE(victim) = 4;
    accum_duration = FALSE;
    to_room = "$n begins to shimmer from a greater magical mantle!";
    to_vict = "You begin to shimmer from a greater magical mantle.";
    break;

  case SPELL_HALT_UNDEAD: // necromancy
    if (!IS_UNDEAD(victim))
    {
      send_to_char(ch, "Your target is not undead.\r\n");
      return;
    }
    if (paralysis_immunity(victim))
    {
      send_to_char(ch, "Your target is unfazed.\r\n");
      return;
    }
    if (GET_LEVEL(victim) > 11)
    {
      send_to_char(ch, "Your target is too powerful to be affected by this enchantment.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, paralysis_bonus, casttype, level, NECROMANCY))
    {
      return;
    }

    SET_BIT_AR(af[0].bitvector, AFF_PARALYZED);
    af[0].duration = dice(3, 3);
    to_room = "$n is overcome by a powerful hold spell!";
    to_vict = "You are overcome by a powerful hold spell!";
    break;

  case SPELL_HASTE: // abjuration
    if (affected_by_spell(victim, SPELL_SLOW))
    {
      affect_from_char(victim, SPELL_SLOW);
      send_to_char(ch, "You dispel the slow spell!\r\n");
      send_to_char(victim, "Your slow spell is dispelled!\r\n");
      return;
    }

    af[0].duration = (level * 12);
    SET_BIT_AR(af[0].bitvector, AFF_HASTE);
    to_room = "$n begins to speed up!";
    to_vict = "You begin to speed up!";
    break;

  case SPELL_SHADOW_WALK: // illusion

    af[0].location = APPLY_DEX;
    af[0].modifier = 2;
    af[0].duration = MIN(10, level);
    af[0].bonus_type = BONUS_TYPE_INHERENT;

    to_room = "$n melds with the shadow realm, giving $m blinding speed.";
    to_vict = "You meld with the shadow realm, giving you blinding speed.";
    break;

  case SPELL_HEROISM: // necromancy
    if (affected_by_spell(victim, SPELL_GREATER_HEROISM))
    {
      send_to_char(ch, "The target is already heroic!\r\n");
      return;
    }
    af[0].location = APPLY_HITROLL;
    af[0].modifier = 2;
    af[0].duration = 300;

    af[1].location = APPLY_SAVING_WILL;
    af[1].modifier = 2;
    af[1].duration = 300;

    af[2].location = APPLY_SAVING_FORT;
    af[2].modifier = 2;
    af[2].duration = 300;

    af[3].location = APPLY_SAVING_REFL;
    af[3].modifier = 2;
    af[3].duration = 300;

    to_room = "$n is now heroic!";
    to_vict = "You feel heroic.";
    break;

  case SPELL_HIDEOUS_LAUGHTER: // enchantment
    if (GET_LEVEL(victim) > 8)
    {
      send_to_char(ch, "Your target is too powerful to be affected by this illusion.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL,
                        enchantment_bonus + paralysis_bonus,
                        casttype, level, ENCHANTMENT))
    {
      return;
    }

    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    is_mind_affect = TRUE;

    SET_BIT_AR(af[0].bitvector, AFF_PARALYZED);
    af[0].duration = dice(1, 4);
    to_room = "$n is overcome by a fit of hideous laughter!";
    to_vict = "You are overcome by a fit of hideous luaghter!";
    break;

  case SPELL_HOLD_ANIMAL: // enchantment

    if (!IS_ANIMAL(victim))
    {
      send_to_char(ch, "This spell is only effective on animals.\r\n");
      return;
    }

    if (paralysis_immunity(victim))
    {
      send_to_char(ch, "Your target is unfazed.\r\n");
      return;
    }
    if (GET_LEVEL(victim) > 11)
    {
      send_to_char(ch, "Your target is too powerful to be affected by this enchantment.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, enchantment_bonus + paralysis_bonus, casttype, level, ENCHANTMENT))
    {
      return;
    }

    SET_BIT_AR(af[0].bitvector, AFF_PARALYZED);
    af[0].duration = level; // one round per level
    to_room = "$n is overcome by a powerful hold spell!";
    to_vict = "You are overcome by a powerful hold spell!";
    break;

  case SPELL_HOLD_PERSON: // enchantment
    if (GET_LEVEL(victim) > 11)
    {
      send_to_char(ch, "Your target is too powerful to be affected by this enchantment.\r\n");
      return;
    }
    if (paralysis_immunity(victim))
    {
      send_to_char(ch, "Your target is unfazed.\r\n");
      return;
    }
    if (paralysis_immunity(victim))
    {
      send_to_char(ch, "Your target is unfazed.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, enchantment_bonus + paralysis_bonus, casttype, level, ENCHANTMENT))
    {
      return;
    }

    SET_BIT_AR(af[0].bitvector, AFF_PARALYZED);
    af[0].duration = dice(3, 3);
    to_room = "$n is overcome by a powerful hold spell!";
    to_vict = "You are overcome by a powerful hold spell!";
    break;

  case SPELL_HORIZIKAULS_BOOM: // evocation
    if (!can_deafen(victim))
    {
      send_to_char(ch, "Your opponent doesn't seem deafable.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, EVOCATION))
    {
      return;
    }

    SET_BIT_AR(af[0].bitvector, AFF_DEAF);
    af[0].duration = dice(2, 4);
    to_room = "$n is deafened by the blast!";
    to_vict = "You feel deafened by the blast!";
    break;

  case SPELL_INFRAVISION: // divination, shared
    af[0].duration = 300 + level * 25;
    SET_BIT_AR(af[0].bitvector, AFF_INFRAVISION);
    to_vict = "Your eyes glow red.";
    to_room = "$n's eyes glow red.";
    break;

  case SPELL_INTERPOSING_HAND: // evocation
    if (mag_resistance(ch, victim, 0))
      return;
    // no save

    af[0].location = APPLY_HITROLL;
    af[0].duration = 4 + level;
    af[0].modifier = -4;

    to_room = "A disembodied hand moves in front of $n!";
    to_vict = "A disembodied hand moves in front of you!";
    break;

  case SPELL_INVISIBLE: // illusion
    if (!victim)
      victim = ch;

    af[0].duration = 300 + (level * 6);
    af[0].modifier = 4;
    af[0].location = APPLY_AC_NEW;
    SET_BIT_AR(af[0].bitvector, AFF_INVISIBLE);
    to_vict = "You vanish.";
    to_room = "$n slowly fades out of existence.";
    break;

  case SPELL_IRON_GUTS: // transmutation
    af[0].location = APPLY_SAVING_FORT;
    af[0].modifier = 3;
    af[0].duration = 300;

    to_room = "$n now has guts tough as iron!";
    to_vict = "You feel like your guts are tough as iron!";
    break;

  case SPELL_IRONSKIN: // transmutation
    if (affected_by_spell(victim, SPELL_EPIC_WARDING))
    {
      send_to_char(ch, "A more powerful magical ward is already in effect on the target.\r\n");
      return;
    }
    /* i made this so you can spam iron skin theoretically -zusuk */
    if (affected_by_spell(victim, SPELL_IRONSKIN) && level >= 20 &&
        GET_STONESKIN(victim) >= WARD_THRESHOLD)
    {
      send_to_char(ch, "The ironskin on %s is still holding strong (%d damage left, %d is the "
                       "configured threshold)!\r\n",
                   GET_NAME(victim), GET_STONESKIN(victim), WARD_THRESHOLD);
      return;
    }
    if (affected_by_spell(victim, SPELL_STONESKIN))
    {
      affect_from_char(victim, SPELL_STONESKIN);
    }

    SET_BIT_AR(af[0].bitvector, AFF_WARDED);
    af[0].duration = 600;
    to_room = "$n's skin takes on the texture of iron!";
    to_vict = "Your skin takes on the texture of iron!";
    level = MAX(10, level);
    GET_STONESKIN(victim) = level * 35;
    break;

  case SPELL_IRRESISTIBLE_DANCE: // enchantment
    if (mag_resistance(ch, victim, 0))
      return;

    // no save, unless have special feat
    if (HAS_FEAT(ch, FEAT_PARALYSIS_RESIST))
    {
      mag_savingthrow(ch, victim, SAVING_WILL, paralysis_bonus, /* +4 bonus from feat */
                      casttype, level, ENCHANTMENT);
      return;
    }

    SET_BIT_AR(af[0].bitvector, AFF_PARALYZED);
    af[0].duration = dice(1, 4) + 1;
    to_room = "$n begins to dance uncontrollably!";
    to_vict = "You begin to dance uncontrollably!";
    break;

  case SPELL_JUMP: // transmutation

    af[0].duration = CLASS_LEVEL(ch, CLASS_DRUID) +
                     CLASS_LEVEL(ch, CLASS_RANGER);
    SET_BIT_AR(af[0].bitvector, AFF_ACROBATIC);

    accum_affect = FALSE;
    accum_duration = FALSE;
    to_room = "$n feels much lighter on $s feet.";
    to_vict = "You feel much lighter on your feet.";
    break;

  case SPELL_MAGE_ARMOR: // conjuration
    if (affected_by_spell(victim, SPELL_EPIC_MAGE_ARMOR))
    {
      send_to_char(victim, "You are affected already by a more powerful magical armoring!\r\n");
      return;
    }

    af[0].location = APPLY_AC_NEW;
    af[0].modifier = 2;
    af[0].duration = 600;
    af[0].bonus_type = BONUS_TYPE_ARMOR;
    accum_duration = FALSE;
    to_vict = "You feel someone protecting you.";
    to_room = "$n is surrounded by magical armor!";
    break;

  case SPELL_MAGIC_FANG:

    if (!IS_ANIMAL(victim))
    {
      send_to_char(ch, "Magic fang can only be cast upon animals.\r\n");
      return;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 1;
    af[0].duration = level;

    to_room = "$n is now affected by magic fang!";
    to_vict = "You are suddenly empowered by magic fang.";
    break;

  case SPELL_MASS_CHARISMA: // transmutation
    if (affected_by_spell(victim, SPELL_CHARISMA))
    {
      affect_from_char(victim, SPELL_CHARISMA);
    }

    af[0].location = APPLY_CHA;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 2 + (level / 5);
    to_vict = "You feel more charismatic!";
    to_room = "$n's charisma increases!";
    break;

  case SPELL_MASS_CUNNING: // transmutation
    if (affected_by_spell(victim, SPELL_CUNNING))
    {
      affect_from_char(victim, SPELL_CUNNING);
    }

    af[0].location = APPLY_INT;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 2 + (level / 5);

    to_vict = "You feel more intelligent!";
    to_room = "$n's intelligence increases!";
    break;

  case SPELL_MASS_ENDURANCE: // transmutation
    if (affected_by_spell(victim, SPELL_MASS_ENHANCE))
    {
      send_to_char(ch, "Your target already has a physical enhancement spell in effect.\r\n");
      send_to_char(victim, "You already have a physical enhancement spell in effect! (mass endurance failed)\r\n");
      return;
    }
    if (affected_by_spell(victim, SPELL_ENDURANCE))
    {
      affect_from_char(victim, SPELL_ENDURANCE);
    }

    af[0].location = APPLY_CON;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 2 + (level / 5);
    to_vict = "You feel more hardy!";
    to_room = "$n's begins to feel more hardy!";
    break;

  case SPELL_MASS_GRACE: // transmutation
    if (affected_by_spell(victim, SPELL_MASS_ENHANCE))
    {
      send_to_char(ch, "Your target already has a physical enhancement spell in effect.\r\n");
      send_to_char(victim, "You already have a physical enhancement spell in effect! (mass grace failed)\r\n");
      return;
    }
    if (affected_by_spell(victim, SPELL_GRACE))
    {
      affect_from_char(victim, SPELL_GRACE);
    }

    af[0].location = APPLY_DEX;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 2 + (level / 5);
    to_vict = "You feel more dextrous!";
    to_room = "$n's appears to be more dextrous!";
    break;

  case SPELL_MASS_STRENGTH: // transmutation
    if (affected_by_spell(victim, SPELL_MASS_ENHANCE))
    {
      send_to_char(ch, "Your target already has a physical enhancement spell in effect.\r\n");
      send_to_char(victim, "You already have a physical enhancement spell in effect! (mass strength failed)\r\n");
      return;
    }
    if (affected_by_spell(victim, SPELL_STRENGTH))
    {
      affect_from_char(victim, SPELL_STRENGTH);
    }

    af[0].location = APPLY_STR;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 2 + (level / 5);
    to_vict = "You feel stronger!";
    to_room = "$n's muscles begin to bulge!";
    break;

  case SPELL_MASS_WISDOM: // transmutation
    if (affected_by_spell(victim, SPELL_WISDOM))
    {
      affect_from_char(victim, SPELL_WISDOM);
    }

    af[0].location = APPLY_WIS;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 2 + (level / 5);
    to_vict = "You feel more wise!";
    to_room = "$n's wisdom increases!";
    break;

  case SPELL_MASS_ENHANCE: // transmutation
    if (affected_by_spell(victim, SPELL_GRACE))
    {
      affect_from_char(victim, SPELL_GRACE);
    }
    if (affected_by_spell(victim, SPELL_MASS_GRACE))
    {
      affect_from_char(victim, SPELL_MASS_GRACE);
    }
    if (affected_by_spell(victim, SPELL_ENDURANCE))
    {
      affect_from_char(victim, SPELL_ENDURANCE);
    }
    if (affected_by_spell(victim, SPELL_MASS_ENDURANCE))
    {
      affect_from_char(victim, SPELL_MASS_ENDURANCE);
    }
    if (affected_by_spell(victim, SPELL_MASS_STRENGTH))
    {
      affect_from_char(victim, SPELL_MASS_STRENGTH);
    }
    if (affected_by_spell(victim, SPELL_STRENGTH))
    {
      affect_from_char(victim, SPELL_STRENGTH);
    }

    af[0].location = APPLY_STR;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 2 + (level / 5);
    af[0].bonus_type = BONUS_TYPE_ENHANCEMENT;

    af[1].location = APPLY_DEX;
    af[1].duration = (level * 12) + 100;
    af[1].modifier = 2 + (level / 5);
    af[1].bonus_type = BONUS_TYPE_ENHANCEMENT;

    af[2].location = APPLY_CON;
    af[2].duration = (level * 12) + 100;
    af[2].modifier = 2 + (level / 5);
    af[2].bonus_type = BONUS_TYPE_ENHANCEMENT;

    to_vict = "You feel your physical attributes enhanced!";
    to_room = "$n's physical attributes are enhanced!";
    break;

  case SPELL_MASS_HOLD_PERSON: // enchantment
    if (paralysis_immunity(victim))
    {
      send_to_char(ch, "Your target is unfazed.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, enchantment_bonus + paralysis_bonus, casttype, level, ENCHANTMENT))
    {
      return;
    }

    SET_BIT_AR(af[0].bitvector, AFF_PARALYZED);
    af[0].duration = dice(3, 4);
    to_room = "$n is overcome by a powerful hold spell!";
    to_vict = "You are overcome by a powerful hold spell!";
    break;

  case SPELL_MINOR_GLOBE: // abjuration
    if (affected_by_spell(victim, SPELL_GLOBE_OF_INVULN))
    {
      send_to_char(ch, "You are already affected by a globe spell!\r\n");
      return;
    }
    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_MINOR_GLOBE);

    accum_duration = FALSE;
    to_vict = "A minor globe of invulnerability surrounds you.";
    to_room = "$n is surrounded by a minor globe of invulnerability.";
    break;

  case SPELL_MIND_BLANK: // abjuration
    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_MIND_BLANK);

    accum_duration = FALSE;
    to_vict = "Your mind becomes blank from harmful magicks.";
    to_room = "$n's mind becomes blanked from harmful magicks.";
    break;

  case SPELL_MIND_FOG: // illusion
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, illusion_bonus, casttype, level, ILLUSION))
    {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }

    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    is_mind_affect = TRUE;

    af[0].location = APPLY_SAVING_WILL;
    af[0].duration = 10 + level;
    af[0].modifier = -10;
    to_room = "$n reels in confusion as a mind fog strikes $e!";
    to_vict = "You reel in confusion as a mind fog spell strikes you!";
    break;

  case SPELL_MIRROR_IMAGE: // illusion
    if (affected_by_spell(victim, SPELL_GREATER_MIRROR_IMAGE))
    {
      send_to_char(victim, "You are already affected by greater mirror image!\r\n");
      return;
    }
    if (affected_by_spell(victim, SPELL_MINOR_ILLUSION))
    {
      affect_from_char(victim, SPELL_MINOR_ILLUSION);
    }

    if (affected_by_spell(victim, SPELL_MIRROR_IMAGE))
    {
      affect_from_char(victim, SPELL_MIRROR_IMAGE);
    }

    af[0].duration = 300;
    SET_BIT_AR(af[0].bitvector, AFF_MIRROR_IMAGED);
    to_room = "$n grins as multiple images pop up and smile!";
    to_vict = "You watch as multiple images pop up and smile at you!";
    GET_IMAGES(victim) = 4 + MIN(5, (int)(level / 3));
    break;

  case SPELL_NIGHTMARE: // illusion
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_FORT, illusion_bonus, casttype, level, ILLUSION))
      return;

    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    is_mind_affect = TRUE;

    SET_BIT_AR(af[0].bitvector, AFF_FATIGUED);
    GET_MOVE(victim) -= level;
    af[0].duration = level;
    to_room = "$n is overcome by overwhelming fatigue from the nightmare!";
    to_vict = "You are overcome by overwhelming fatigue from the nightmare!";
    break;

  case SPELL_NON_DETECTION:
    af[0].duration = 25 + (level * 12);
    SET_BIT_AR(af[0].bitvector, AFF_NON_DETECTION);
    to_room = "$n briefly glows green!";
    to_vict = "You feel protection from scrying.";
    break;

  case SPELL_POISON: // enchantment, shared
    if (!can_poison(victim))
    {
      send_to_char(ch, "Your opponent doesn't seem susceptible to poison.\r\n");
      return;
    }
    if (casttype != CAST_INNATE && mag_resistance(ch, victim, 0))
      return;
    int bonus = get_poison_save_mod(ch, victim);
    if (mag_savingthrow(ch, victim, SAVING_FORT, bonus, casttype, level, ENCHANTMENT))
    {
      send_to_char(ch, "Your victim seems to resist the poison!\r\n");
      return;
    }

    af[0].location = APPLY_STR;
    if (casttype == CAST_INNATE || casttype == CAST_WEAPON_POISON) /* trelux for example */
      af[0].duration = GET_LEVEL(ch) * 5;
    else
      af[0].duration = level * 12;
    if (KNOWS_DISCOVERY(ch, ALC_DISC_MALIGNANT_POISON))
      af[0].duration *= 1.5;
    af[0].modifier = -2;
    SET_BIT_AR(af[0].bitvector, AFF_POISON);
    to_vict = "You feel very sick.";
    to_room = "$n gets violently ill!";
    break;

  case SPELL_WIND_WALL:
    af[0].duration = level;
    SET_BIT_AR(af[0].bitvector, AFF_WIND_WALL);
    to_vict = "You are surrounded by a swirling wall of wind.";
    to_room = "$n is surrounded by a swirling wall of wind.";
    break;

  case SPELL_POWER_WORD_BLIND: // necromancy
    if (!can_blind(victim))
    {
      send_to_char(ch, "Your opponent doesn't seem blindable.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_REFL, -4, casttype, level, NECROMANCY))
    {
      send_to_char(ch, "You fail.\r\n");
      return;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = -4;
    af[0].duration = 200;
    SET_BIT_AR(af[0].bitvector, AFF_BLIND);

    af[1].location = APPLY_AC_NEW;
    af[1].modifier = -4;
    af[1].duration = 200;
    SET_BIT_AR(af[1].bitvector, AFF_BLIND);

    to_room = "$n seems to be blinded!";
    to_vict = "You have been blinded!";
    break;

  case SPELL_POWER_WORD_STUN: // divination
    if (!can_stun(victim))
    {
      send_to_char(ch, "It seems your opponent cannot be stunned.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    // no save

    SET_BIT_AR(af[0].bitvector, AFF_STUN);
    af[0].duration = dice(1, 4);
    to_room = "$n is stunned by a powerful magical word!";
    to_vict = "You are stunned by a powerful magical word!";
    break;

  case SPELL_PRAYER:
    if (affected_by_spell(victim, SPELL_BLESS))
    {
      affect_from_char(victim, SPELL_BLESS);
    }
    if (affected_by_spell(victim, SPELL_AID))
    {
      affect_from_char(victim, SPELL_AID);
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = 5;
    af[0].duration = 300;
    af[0].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

    af[1].location = APPLY_DAMROLL;
    af[1].modifier = 5;
    af[1].duration = 300;
    af[1].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

    af[2].location = APPLY_SAVING_WILL;
    af[2].modifier = 3;
    af[2].duration = 300;
    af[2].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

    af[3].location = APPLY_SAVING_FORT;
    af[3].modifier = 3;
    af[3].duration = 300;
    af[3].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

    af[4].location = APPLY_SAVING_REFL;
    af[4].modifier = 3;
    af[4].duration = 300;
    af[4].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

    af[5].location = APPLY_HIT;
    af[5].modifier = dice(4, 12) + level;
    af[5].duration = 300;
    af[5].bonus_type = BONUS_TYPE_CIRCUMSTANCE;

    to_room = "$n is now divinely blessed and aided!";
    to_vict = "You feel divinely blessed and aided.";
    break;

  case SPELL_PRISMATIC_SPRAY: // illusion, does damage too
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, illusion_bonus + paralysis_bonus, casttype, level, ILLUSION))
    {
      return;
    }

    switch (dice(1, 4))
    {
    case 1:
      if (!can_stun(victim))
      {
        send_to_char(ch, "It seems your opponent cannot be stunned.\r\n");
        return;
      }
      SET_BIT_AR(af[0].bitvector, AFF_STUN);
      af[0].duration = dice(2, 4);
      to_room = "$n is stunned by the colors!";
      to_vict = "You are stunned by the colors!";
      break;
    case 2:
      if (paralysis_immunity(victim))
      {
        send_to_char(ch, "Your target is unfazed.\r\n");
        return;
      }
      SET_BIT_AR(af[0].bitvector, AFF_PARALYZED);
      af[0].duration = dice(1, 6);
      to_room = "$n is paralyzed by the colors!";
      to_vict = "You are paralyzed by the colors!";
      break;
    case 3:
      if (!can_blind(victim))
      {
        send_to_char(ch, "Your opponent doesn't seem blindable.\r\n");
        return;
      }
      af[0].location = APPLY_HITROLL;
      af[0].modifier = -4;
      af[0].duration = 25;
      SET_BIT_AR(af[0].bitvector, AFF_BLIND);

      af[1].location = APPLY_AC_NEW;
      af[1].modifier = -4;
      af[1].duration = 25;
      SET_BIT_AR(af[1].bitvector, AFF_BLIND);

      to_room = "$n seems to be blinded by the colors!";
      to_vict = "You have been blinded by the colors!";

      break;
    case 4:
      af[0].duration = level;
      SET_BIT_AR(af[0].bitvector, AFF_SLOW);
      to_room = "$n begins to slow down from the prismatic spray!";
      to_vict = "You feel yourself slow down because of the prismatic spray!";

      break;
    }
    break;

  case SPELL_PROT_FROM_EVIL: // abjuration
    af[0].duration = 600;
    SET_BIT_AR(af[0].bitvector, AFF_PROTECT_EVIL);
    to_vict = "You feel invulnerable to evil!";
    break;

  case SPELL_PROT_FROM_GOOD: // abjuration
    af[0].duration = 600;
    SET_BIT_AR(af[0].bitvector, AFF_PROTECT_GOOD);
    to_vict = "You feel invulnerable to good!";
    break;

  case SPELL_PROTECT_FROM_SPELLS: // divination
    af[1].location = APPLY_SAVING_WILL;
    af[1].modifier = 1;
    af[1].duration = 100;

    to_room = "$n is now protected from spells!";
    to_vict = "You feel protected from spells!";
    break;

  case SPELL_RAINBOW_PATTERN: // illusion
    if (!can_stun(victim))
    {
      send_to_char(ch, "It seems your opponent cannot be stunned.\r\n");
      return;
    }
    if (GET_LEVEL(victim) > 13)
    {
      send_to_char(ch, "Your target is too powerful to be affected by this illusion.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, illusion_bonus, casttype, level, ILLUSION))
    {
      return;
    }

    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    is_mind_affect = TRUE;

    SET_BIT_AR(af[0].bitvector, AFF_STUN);
    af[0].duration = dice(3, 4);
    to_room = "$n is stunned by the pattern of bright colors!";
    to_vict = "You are dazed by the pattern of bright colors!";
    break;

  case SPELL_RAY_OF_ENFEEBLEMENT: // necromancy
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, NECROMANCY))
    {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }

    af[0].location = APPLY_STR;
    af[0].duration = 25 + (level * 12);
    af[0].modifier = -dice(2, 4);
    accum_duration = TRUE;
    to_room = "$n is struck by enfeeblement!";
    to_vict = "You feel enfeebled!";
    break;

  case SPELL_REGENERATION:
    af[0].duration = 100;
    SET_BIT_AR(af[0].bitvector, AFF_REGEN);

    accum_duration = FALSE;
    to_vict = "You begin regenerating.";
    to_room = "$n begins regenerating.";
    break;

  case SPELL_RESIST_ENERGY: // abjuration
    af[0].duration = 600;
    SET_BIT_AR(af[0].bitvector, AFF_ELEMENT_PROT);
    to_vict = "You feel a slight protection from energy!";
    to_room = "$n begins to feel slightly protected from energy!";
    break;

  case SPELL_SANCTUARY:
    af[0].duration = 100;
    SET_BIT_AR(af[0].bitvector, AFF_SANCTUARY);

    accum_duration = FALSE;
    to_vict = "A white aura momentarily surrounds you.";
    to_room = "$n is surrounded by a white aura.";
    break;

  case SPELL_SCARE: // illusion
    if (is_immune_fear(ch, victim, TRUE))
      return;
    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, illusion_bonus + affected_by_aura_of_cowardice(victim) ? -4 : 0, casttype, level, ILLUSION))
    {
      return;
    }
    is_mind_affect = TRUE;

    if (GET_LEVEL(victim) >= 10)
    {
      send_to_char(ch, "The victim is too powerful for this illusion!\r\n");
      return;
    }

    SET_BIT_AR(af[0].bitvector, AFF_FEAR);
    af[0].duration = dice(2, 6);
    to_room = "$n is imbued with fear!";
    to_vict = "You feel scared and fearful!";
    break;

  case SPELL_FEAR: // illusion
    if (is_immune_fear(ch, victim, TRUE))
      return;
    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, illusion_bonus + affected_by_aura_of_cowardice(victim) ? -4 : 0, casttype, level, ILLUSION))
    {
      return;
    }
    is_mind_affect = TRUE;

    SET_BIT_AR(af[0].bitvector, AFF_FEAR);
    af[0].duration = dice(2, 6);
    to_room = "$n is imbued with fear!";
    to_vict = "You feel scared and fearful!";
    break;

  case SPELL_SCINT_PATTERN: // illusion
    if (!can_confuse(victim))
    {
      send_to_char(ch, "Your opponent seems to be immune to confusion effects.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    // no save

    SET_BIT_AR(af[0].bitvector, AFF_CONFUSED);
    af[0].duration = dice(2, 4) + 2;
    victim->confuser_idnum = GET_IDNUM(ch);
    to_room = "$n is confused by the scintillating pattern!";
    to_vict = "You are confused by the scintillating pattern!";
    break;

  case SPELL_SENSE_LIFE:
    to_vict = "Your feel your awareness improve.";
    to_room = "$n's eyes become aware of life forms!";
    af[0].duration = level * 25;
    SET_BIT_AR(af[0].bitvector, AFF_SENSE_LIFE);
    break;

  case SPELL_SHADOW_SHIELD: // illusion

    if (affected_by_spell(victim, SPELL_SHIELD))
    {
      affect_from_char(victim, SPELL_SHIELD);
    }

    af[0].location = APPLY_AC_NEW;
    af[0].modifier = 5;
    af[0].duration = level * 5;
    af[0].bonus_type = BONUS_TYPE_SHIELD;

    af[1].location = APPLY_RES_NEGATIVE;
    af[1].modifier = 100;
    af[1].duration = level * 5;
    af[1].bonus_type = BONUS_TYPE_RESISTANCE;

    af[2].location = APPLY_DR;
    af[2].modifier = 12;
    af[2].duration = level * 5;

    CREATE(new_dr, struct damage_reduction_type, 1);

    new_dr->bypass_cat[0] = DR_BYPASS_CAT_MATERIAL;
    new_dr->bypass_val[0] = MATERIAL_ADAMANTINE;

    new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[1] = 0; /* Unused. */

    new_dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[2] = 0; /* Unused. */

    new_dr->amount = 12;
    new_dr->max_damage = -1;
    new_dr->spell = SPELL_SHADOW_SHIELD;
    new_dr->feat = FEAT_UNDEFINED;
    new_dr->next = GET_DR(victim);
    GET_DR(victim) = new_dr;

    to_vict = "You feel someone protecting you with the shadows.";
    to_room = "$n is surrounded by a shadowy shield!";
    break;

  case SPELL_SHIELD: // transmutation
    if (affected_by_spell(victim, SPELL_SHADOW_SHIELD))
    {
      send_to_char(victim, "You are already affected by shadow shield protection!\r\n");
      return;
    }

    af[0].location = APPLY_AC_NEW;
    af[0].modifier = 2;
    af[0].duration = 300;
    af[0].bonus_type = BONUS_TYPE_SHIELD;
    to_vict = "You feel someone protecting you.";
    to_room = "$n is surrounded by magical armor!";
    break;

  case SPELL_SHRINK_PERSON: // transmutation
    af[0].location = APPLY_SIZE;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = -1;
    to_vict = "You feel yourself shrinking!";
    to_room = "$n's begins to shrink to being much smaller!";
    break;

  case SPELL_SLEEP: // enchantment
    if (GET_LEVEL(victim) >= 7 || is_immune_sleep)
    {
      send_to_char(ch, "The target is too powerful for this enchantment!\r\n");
      return;
    }
    if (sleep_immunity(victim))
    {
      send_to_char(ch, "Your target is unfazed.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP))
    {
      send_to_char(ch, "Your victim doesn't seem vulnerable to your spell.");
      return;
    }
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, ENCHANTMENT))
    {
      return;
    }

    af[0].duration = 100 + (level * 6);
    SET_BIT_AR(af[0].bitvector, AFF_SLEEP);

    if (GET_POS(victim) > POS_SLEEPING)
    {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      if (FIGHTING(victim))
        stop_fighting(victim);
      change_position(victim, POS_SLEEPING);
      if (FIGHTING(ch) == victim)
        stop_fighting(ch);
    }
    break;

  case SPELL_SLOW: // abjuration
    if (affected_by_spell(victim, SPELL_HASTE))
    {
      affect_from_char(victim, SPELL_HASTE);
      send_to_char(ch, "You dispel the haste spell!\r\n");
      send_to_char(victim, "Your haste spell is dispelled!\r\n");
      return;
    }
    if (affected_by_spell(victim, PSIONIC_SLIP_THE_BONDS))
    {
      send_to_char(ch, "Your spell is resisted!\r\n");
      send_to_char(victim, "You avoid the effect due to your slip the bonds manifestation!\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, ABJURATION))
    {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }

    af[1].duration = (level * 12);
    SET_BIT_AR(af[0].bitvector, AFF_SLOW);
    to_room = "$n begins to slow down!";
    to_vict = "You feel yourself slow down!";
    break;

  case SPELL_SPELL_MANTLE: // abjuration
    if (affected_by_spell(victim, SPELL_GREATER_SPELL_MANTLE))
    {
      send_to_char(ch, "A magical mantle is already in effect on target.\r\n");
      return;
    }

    af[0].duration = level * 3;
    SET_BIT_AR(af[0].bitvector, AFF_SPELL_MANTLE);
    GET_SPELL_MANTLE(victim) = 2;
    accum_duration = FALSE;
    to_room = "$n begins to shimmer from a magical mantle!";
    to_vict = "You begin to shimmer from a magical mantle.";
    break;

  case SPELL_SPELL_RESISTANCE:
    af[0].duration = 50 + level;
    SET_BIT_AR(af[0].bitvector, AFF_SPELL_RESISTANT);

    accum_duration = FALSE;
    to_vict = "You feel your spell resistance increase.";
    to_room = "$n's spell resistance increases.";
    break;

  case SPELL_SPELL_TURNING: // abjuration
    af[0].duration = 100;
    SET_BIT_AR(af[0].bitvector, AFF_SPELL_TURNING);

    accum_duration = FALSE;
    to_vict = "A spell-turning shield surrounds you.";
    to_room = "$n is surrounded by a spell turning shield.";
    break;

  case SPELL_STENCH:
    if (AFF_FLAGGED(victim, AFF_WIND_WALL))
    {
      send_to_char(ch, "The wall of wind surrounding you blows away the stench.\r\n");
      act("A wall of wind surrounding $n dissipates the stench", FALSE, ch, 0, 0, TO_ROOM);
      return;
    }
    if (GET_LEVEL(victim) >= 9)
    {
      return;
    }
    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, ABJURATION))
    {
      return;
    }

    SET_BIT_AR(af[0].bitvector, AFF_NAUSEATED);
    af[0].duration = 3;
    to_room = "$n becomes nauseated from the stinky fumes!";
    to_vict = "You become nauseated from the stinky fumes!";
    break;

  case SPELL_STONESKIN:
    if (affected_by_spell(victim, SPELL_EPIC_WARDING) ||
        affected_by_spell(victim, SPELL_IRONSKIN))
    {
      send_to_char(ch, "A magical ward is already in effect on target.\r\n");
      return;
    }
    af[0].location = APPLY_DR;
    af[0].modifier = 0;
    af[0].duration = 600;
    to_room = "$n's skin becomes hard as rock!";
    to_vict = "Your skin becomes hard as stone.";

    CREATE(new_dr, struct damage_reduction_type, 1);

    new_dr->bypass_cat[0] = DR_BYPASS_CAT_MATERIAL;
    new_dr->bypass_val[0] = MATERIAL_ADAMANTINE;

    new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[1] = 0; /* Unused. */

    new_dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[2] = 0; /* Unused. */

    new_dr->amount = 10;
    new_dr->max_damage = MAX(MIN(150, level * 10), 60);
    new_dr->spell = SPELL_STONESKIN;
    new_dr->feat = FEAT_UNDEFINED;
    new_dr->next = GET_DR(victim);
    GET_DR(victim) = new_dr;

    break;

  case RACIAL_ABILITY_CRYSTAL_BODY:

    /* Remove the dr. */
    for (dr = GET_DR(ch); dr != NULL; dr = dr->next)
    {
      if (dr->spell == spellnum)
      {
        REMOVE_FROM_LIST(dr, GET_DR(ch), next);
      }
    }

    af[0].location = APPLY_DR;
    af[0].modifier = 3;
    af[0].duration = 24 + GET_CON_BONUS(ch) + level / 2;
    to_room = "\tCYou watch as $n's crystalline body becomes harder!\tn";
    to_vict = "\tCYour crystalline body becomes harder!\tn";

    CREATE(new_dr, struct damage_reduction_type, 1);

    new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[1] = 0; // Unused.

    new_dr->bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[1] = 0; // Unused.

    new_dr->bypass_cat[2] = DR_BYPASS_CAT_UNUSED;
    new_dr->bypass_val[2] = 0; // Unused.

    new_dr->amount = 3;
    new_dr->max_damage = -1;
    new_dr->spell = RACIAL_ABILITY_CRYSTAL_BODY;
    new_dr->feat = FEAT_UNDEFINED;
    new_dr->next = GET_DR(victim);
    GET_DR(victim) = new_dr;
    break;

  case RACIAL_ABILITY_INSECTBEING:

    af[0].location = APPLY_AC_NEW;
    af[0].bonus_type = BONUS_TYPE_RACIAL;
    af[0].duration = 24 + GET_CON_BONUS(ch) + level / 2;
    af[0].modifier = level / 6;

    af[1].location = APPLY_HITROLL;
    af[1].bonus_type = BONUS_TYPE_RACIAL;
    af[1].duration = 24 + GET_CON_BONUS(ch) + level / 2;
    af[1].modifier = level / 6;

    af[2].location = APPLY_SAVING_FORT;
    af[2].bonus_type = BONUS_TYPE_RACIAL;
    af[2].duration = 24 + GET_CON_BONUS(ch) + level / 2;
    af[2].modifier = level / 6;

    af[3].location = APPLY_SAVING_WILL;
    af[3].bonus_type = BONUS_TYPE_RACIAL;
    af[3].duration = 24 + GET_CON_BONUS(ch) + level / 2;
    af[3].modifier = level / 6;

    af[4].location = APPLY_SAVING_REFL;
    af[4].bonus_type = BONUS_TYPE_RACIAL;
    af[4].duration = 24 + GET_CON_BONUS(ch) + level / 2;
    af[4].modifier = level / 6;

    af[5].location = APPLY_DEX;
    af[5].bonus_type = BONUS_TYPE_RACIAL;
    af[5].duration = 24 + GET_CON_BONUS(ch) + level / 2;
    af[5].modifier = level / 6;

    to_vict = "\tCYou attune to your insect being!\tn";
    to_room = "\tC$n attunes to $s insect being!\tn";

    break;

  case RACIAL_ABILITY_CRYSTAL_FIST:

    af[0].location = APPLY_DAMROLL;
    af[0].bonus_type = BONUS_TYPE_RACIAL;
    af[0].duration = 24 + GET_CON_BONUS(ch) + level / 2;
    af[0].modifier = 3;

    to_vict = "\tCLarge, razor sharp crystals sprout from your hands and arms!\tn";
    to_room = "\tCRazor sharp crystals sprout from $n's arms and hands!\tn";

    break;

  case SPELL_STRENGTH: // transmutation
    af[0].location = APPLY_STR;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 2 + (level / 5);
    af[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
    accum_duration = FALSE;
    to_vict = "You feel stronger!";
    to_room = "$n's muscles begin to bulge!";
    break;

  case SPELL_STRENGTHEN_BONE:
    if (!IS_UNDEAD(victim))
      return;

    af[0].location = APPLY_AC_NEW;
    af[0].modifier = 2;
    af[0].duration = 600;
    to_vict = "You feel your bones harden.";
    to_room = "$n's bones harden!";
    break;

  case SPELL_SUNBEAM:  // evocation[light]
  case SPELL_SUNBURST: // divination, does damage and room affect
    if (!can_blind(victim))
    {
      send_to_char(ch, "Your opponent doesn't seem blindable.\r\n");
      return;
    }
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, DIVINATION))
    {
      return;
    }

    af[0].location = APPLY_HITROLL;
    af[0].modifier = -4;
    af[0].duration = 25;
    SET_BIT_AR(af[0].bitvector, AFF_BLIND);

    af[1].location = APPLY_AC_NEW;
    af[1].modifier = -4;
    af[1].duration = 25;
    SET_BIT_AR(af[1].bitvector, AFF_BLIND);

    to_room = "$n seems to be blinded!";
    to_vict = "You have been blinded!";
    break;

  case SPELL_THUNDERCLAP: // abjuration
    success = 0;

    if (!can_deafen(victim))
    {
      send_to_char(ch, "Your opponent seems immune to being deafened.\r\n");
      return;
    }
    if (!mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, ABJURATION) && !mag_resistance(ch, victim, 0))
    {
      af[0].duration = 10;
      SET_BIT_AR(af[0].bitvector, AFF_DEAF);

      act("You have been deafened!", FALSE, victim, 0, ch, TO_CHAR);
      act("$n seems to be deafened!", TRUE, victim, 0, ch, TO_ROOM);
      success = 1;
    }

    if (!mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, ABJURATION) &&
        !mag_resistance(ch, victim, 0))
    {
      if (!can_stun(victim))
      {
        send_to_char(ch, "It seems your opponent cannot be stunned.\r\n");
        return;
      }
      af[1].duration = 4;
      SET_BIT_AR(af[1].bitvector, AFF_STUN);

      act("You have been stunned!", FALSE, victim, 0, ch, TO_CHAR);
      act("$n seems to be stunned!", TRUE, victim, 0, ch, TO_ROOM);
      success = 1;
    }

    if (!mag_savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, ABJURATION) &&
        !mag_resistance(ch, victim, 0))
    {

      change_position(victim, POS_SITTING);

      act("You have been knocked down!", FALSE, victim, 0, ch, TO_CHAR);
      act("$n is knocked down!", TRUE, victim, 0, ch, TO_ROOM);
    }

    if (!success)
      return;
    break;

  case SPELL_TIMESTOP: // abjuration
    af[0].duration = 14;
    SET_BIT_AR(af[0].bitvector, AFF_TIME_STOPPED);

    accum_duration = FALSE;
    to_vict = "The world around starts moving very slowly.";
    to_room = "$n begins to move outside of time.";
    break;

  case SPELL_TOUCH_OF_IDIOCY: // enchantment
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_WILL, enchantment_bonus, casttype, level, ENCHANTMENT))
    {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }

    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    is_mind_affect = TRUE;

    af[0].location = APPLY_INT;
    af[0].duration = 25 + (level * 12);
    af[0].modifier = -(dice(1, 6));

    af[1].location = APPLY_WIS;
    af[1].duration = 25 + (level * 12);
    af[1].modifier = -(dice(1, 6));

    af[2].location = APPLY_CHA;
    af[2].duration = 25 + (level * 12);
    af[2].modifier = -(dice(1, 6));

    accum_duration = TRUE;
    accum_affect = FALSE;
    to_room = "A look of idiocy crosses $n's face!";
    to_vict = "You feel very idiotic.";
    break;

  case SPELL_TRANSFORMATION: // necromancy
    af[0].duration = 50;
    SET_BIT_AR(af[0].bitvector, AFF_TFORM);

    accum_duration = FALSE;
    to_vict = "You feel your combat skill increase!";
    to_room = "The combat skill of $n increases!";
    break;

  case SPELL_TRUE_SEEING: // divination
    af[0].duration = 20 + level;
    SET_BIT_AR(af[0].bitvector, AFF_TRUE_SIGHT);
    to_vict = "Your eyes tingle, now with true-sight.";
    to_room = "$n's eyes become enhanced with true-sight!";
    break;

  case SPELL_TRUE_STRIKE: // illusion
    af[0].location = APPLY_HITROLL;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 20;
    to_vict = "You feel able to strike true!";
    to_room = "$n is now able to strike true!";
    break;

  case SPELL_WAIL_OF_THE_BANSHEE:                 // necromancy (does damage too)
    if (is_immune_death_magic(ch, victim, FALSE)) // FALSE because we show the message in mag_damage
      return;
    if (is_immune_fear(ch, victim, TRUE))
      return;
    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_FORT, affected_by_aura_of_cowardice(victim) ? -4 : 0, casttype, level, NECROMANCY))
    {
      return;
    }

    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    is_mind_affect = TRUE;

    SET_BIT_AR(af[0].bitvector, AFF_FEAR);
    af[0].duration = dice(2, 6);
    to_room = "$n is imbued with fear!";
    to_vict = "You feel scared and fearful!";
    break;

  case SPELL_WATER_BREATHE:
    af[0].duration = 600;
    SET_BIT_AR(af[0].bitvector, AFF_SCUBA);
    to_vict = "You feel gills grow behind your neck.";
    to_room = "$n's neck grows gills!";
    break;

  case SPELL_LEVITATE: // transmutation
    af[0].duration = 600;
    SET_BIT_AR(af[0].bitvector, AFF_LEVITATE);
    to_vict = "As you raise your arms, you begin to float in the air.";
    to_room = "$n begins to slowly levitate above the ground!";
    break;

  case SPELL_WATERWALK: // transmutation
    af[0].duration = 600;
    SET_BIT_AR(af[0].bitvector, AFF_WATERWALK);
    to_vict = "You feel webbing between your toes.";
    to_room = "$n's feet grow webbing!";
    break;

  case SPELL_WAVES_OF_EXHAUSTION: // necromancy
    if (mag_resistance(ch, victim, 0))
      return;
    // no save

    SET_BIT_AR(af[0].bitvector, AFF_FATIGUED);
    GET_MOVE(victim) -= 20 + level;
    af[0].duration = level + 10;
    to_room = "$n is overcome by overwhelming exhaustion!!";
    to_vict = "You are overcome by overwhelming exhaustion!!";
    break;

  case SPELL_WAVES_OF_FATIGUE: // necromancy
    if (mag_resistance(ch, victim, 0))
      return;
    if (mag_savingthrow(ch, victim, SAVING_FORT, 0, casttype, level, NECROMANCY))
    {
      return;
    }
    SET_BIT_AR(af[0].bitvector, AFF_FATIGUED);
    GET_MOVE(victim) -= 10 + level;
    af[0].duration = level;
    to_room = "$n is overcome by overwhelming fatigue!!";
    to_vict = "You are overcome by overwhelming fatigue!!";
    break;

  case SPELL_WEB: // conjuration
    if (MOB_FLAGGED(victim, MOB_NOGRAPPLE))
    {
      send_to_char(ch, "Your opponent doesn't seem webbable.\r\n");
      return;
    }
    if (affected_by_spell(victim, PSIONIC_SLIP_THE_BONDS))
    {
      send_to_char(ch, "Your spell is resisted!\r\n");
      send_to_char(victim, "You avoid the effect due to your slip the bonds manifestation!\r\n");
      return;
    }
    if (mag_savingthrow(ch, victim, SAVING_REFL, 0, casttype, level, CONJURATION))
    {
      send_to_char(ch, "You fail.\r\n");
      return;
    }

    af[0].duration = 7 * level;
    SET_BIT_AR(af[0].bitvector, AFF_ENTANGLED);

    to_room = "$n is covered in a sticky magical web!";
    to_vict = "You are covered in a sticky magical web!";
    break;

  case SPELL_WEIRD: // illusion (also does damage)
    if (mag_resistance(ch, victim, 0))
      return;

    // no save, unless have special feat
    if (HAS_FEAT(ch, FEAT_PHANTASM_RESIST))
    {
      mag_savingthrow(ch, victim, SAVING_WILL, illusion_bonus + 4, /* +4 bonus from feat */
                      casttype, level, ILLUSION);
      return;
    }

    af[0].location = APPLY_STR;
    af[0].duration = level;
    af[0].modifier = -(dice(1, 4));
    to_room = "$n's strength is withered!";
    to_vict = "You feel your strength wither!";

    if (can_stun(victim))
    {
      SET_BIT_AR(af[1].bitvector, AFF_STUN);
      af[1].duration = 1;
    }
    to_room = "$n is stunned by a terrible WEIRD!";
    to_vict = "You are stunned by a terrible WEIRD!";
    break;

  case SPELL_WISDOM: // transmutation
    af[0].location = APPLY_WIS;
    af[0].duration = (level * 12) + 100;
    af[0].modifier = 4;
    to_vict = "You feel more wise!";
    to_room = "$n's wisdom increases!";
    break;
  }

  /* slippery mind */
  if (!IS_NPC(victim) &&
      HAS_FEAT(victim, FEAT_SLIPPERY_MIND) &&
      spell_info[spellnum].violent)
  {
    send_to_char(victim, "\tW*Slippery Mind*\tn  ");
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, casttype, level, spell_school))
      return;
  }

  /* If this is a mob that has this affect set in its mob file, do not perform
   * the affect.  This prevents people from un-sancting mobs by sancting them
   * and waiting for it to fade, for example. */
  if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
  {
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    {
      for (j = 0; j < NUM_AFF_FLAGS; j++)
      {
        if (IS_SET_AR(af[i].bitvector, j) && AFF_FLAGGED(victim, j))
        {
          send_to_char(ch, "%s", CONFIG_NOEFFECT);
          return;
        }
      }
    }
  }

  /* If the victim is already affected by this spell, and the spell does not
   * have an accumulative effect, then fail the spell. */
  if (affected_by_spell(victim, spellnum) && !(accum_duration || accum_affect))
  {
    if (casttype == CAST_WEAPON_POISON)
    {
      ; /* nicer with no message here */
      return;
    }
    else if (spell_info[spellnum].violent)
    {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }
    else
    {
      // if it's a buff, we want to replace it instead of failing
      affect_from_char(victim, spellnum);
    }
  }

  if (spellnum >= PSIONIC_POWER_START && spellnum <= PSIONIC_POWER_END)
  {
    // tidier this way
    accum_affect = FALSE;
  }

  /* send messages */
  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, ACT_CONDENSE_VALUE, victim, 0, ch, TO_ROOM);

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
  {
    if (af[i].bitvector[0] || af[i].bitvector[1] ||
        af[i].bitvector[2] || af[i].bitvector[3] ||
        (af[i].location != APPLY_NONE))
    {
      if (casttype == CAST_POTION)
        if (KNOWS_DISCOVERY(ch, ALC_DISC_EXTEND_POTION))
          af[i].duration *= 2;
      affect_join(victim, af + i, accum_duration, FALSE, accum_affect, FALSE);
    }
  }
}

#undef WARD_THRESHOLD

/* This function is used to provide services to mag_groups.  This function is
 * the one you should change to add new group spells. */
static void perform_mag_groups(int level, struct char_data *ch,
                               struct char_data *tch, struct obj_data *obj, int spellnum,
                               int savetype, int casttype)
{

  switch (spellnum)
  {
  case SPELL_GROUP_HEAL:
    mag_points(level, ch, tch, obj, SPELL_HEAL, savetype, casttype);
    break;
  case ABILITY_CHANNEL_POSITIVE_ENERGY:
    if (!IS_UNDEAD(tch))
      mag_points(level, ch, tch, obj, ABILITY_CHANNEL_POSITIVE_ENERGY, savetype, casttype);
    break;
  case ABILITY_CHANNEL_NEGATIVE_ENERGY:
    if (IS_UNDEAD(tch))
      mag_points(level, ch, tch, obj, ABILITY_CHANNEL_NEGATIVE_ENERGY, savetype, casttype);
    break;
  case SPELL_GROUP_SHIELD_OF_FAITH:
    mag_affects(level, ch, tch, obj, SPELL_SHIELD_OF_FAITH, savetype, casttype, 0);
    break;
  case SPELL_COMMUNAL_PROTECTION_FROM_ENERGY:
    mag_affects(level, ch, tch, obj, SPELL_PROTECTION_FROM_ENERGY, savetype, casttype, 0);
    break;
  case SPELL_TACTICAL_ACUMEN:
    mag_affects(level, ch, tch, obj, SPELL_TACTICAL_ACUMEN, savetype, casttype, 0);
    break;
  case SPELL_MASS_FALSE_LIFE:
    mag_affects(level, ch, tch, obj, SPELL_FALSE_LIFE, savetype, casttype, 0);
    break;
  case SPELL_MASS_HASTE:
    mag_affects(level, ch, tch, obj, SPELL_HASTE, savetype, casttype, 0);
    break;
  case SPELL_MASS_CURE_CRIT:
    mag_points(level, ch, tch, obj, SPELL_CURE_CRITIC, savetype, casttype);
    break;
  case SPELL_MASS_CURE_SERIOUS:
    mag_points(level, ch, tch, obj, SPELL_CURE_SERIOUS, savetype, casttype);
    break;
  case SPELL_MASS_CURE_MODERATE:
    mag_points(level, ch, tch, obj, SPELL_CURE_MODERATE, savetype, casttype);
    break;
  case SPELL_MASS_CURE_LIGHT:
    mag_points(level, ch, tch, obj, SPELL_CURE_LIGHT, savetype, casttype);
    break;
  case SPELL_GROUP_VIGORIZE:
    mag_points(level, ch, tch, obj, SPELL_VIGORIZE_CRITICAL, savetype, casttype);
    break;
  case SPELL_CIRCLE_A_EVIL:
    mag_affects(level, ch, tch, obj, SPELL_PROT_FROM_EVIL, savetype, casttype, 0);
    break;
  case SPELL_CIRCLE_A_GOOD:
    mag_affects(level, ch, tch, obj, SPELL_PROT_FROM_GOOD, savetype, casttype, 0);
    break;
  case SPELL_INVISIBILITY_SPHERE:
    mag_affects(level, ch, tch, obj, SPELL_INVISIBLE, savetype, casttype, 0);
    break;
  case SPELL_GROUP_RECALL:
    spell_recall(level, ch, tch, NULL, casttype);
    break;
  case SPELL_MASS_FLY:
    mag_affects(level, ch, tch, obj, SPELL_FLY, savetype, casttype, 0);
    break;
  case SPELL_MASS_CUNNING:
    mag_affects(level, ch, tch, obj, SPELL_MASS_CUNNING, savetype, casttype, 0);
    break;
  case SPELL_MASS_CHARISMA:
    mag_affects(level, ch, tch, obj, SPELL_MASS_CHARISMA, savetype, casttype, 0);
    break;
  case SPELL_MASS_WISDOM:
    mag_affects(level, ch, tch, obj, SPELL_MASS_WISDOM, savetype, casttype, 0);
    break;
  case SPELL_MASS_ENHANCE:
    mag_affects(level, ch, tch, obj, SPELL_MASS_ENHANCE, savetype, casttype, 0);
    break;
  case SPELL_AID:
    mag_affects(level, ch, tch, obj, SPELL_AID, savetype, casttype, 0);
    break;
  case SPELL_PRAYER:
    mag_affects(level, ch, tch, obj, SPELL_PRAYER, savetype, casttype, 0);
    break;
  case SPELL_MASS_ENDURANCE:
    mag_affects(level, ch, tch, obj, SPELL_MASS_ENDURANCE, savetype, casttype, 0);
    break;
  case SPELL_MASS_GRACE:
    mag_affects(level, ch, tch, obj, SPELL_MASS_GRACE, savetype, casttype, 0);
    break;
  case SPELL_MASS_STRENGTH:
    mag_affects(level, ch, tch, obj, SPELL_MASS_STRENGTH, savetype, casttype, 0);
    break;
  case SPELL_ANIMAL_SHAPES:
    /* found in act.other.c */
    perform_wildshape(tch, rand_number(1, (NUM_SHAPE_TYPES - 1)), spellnum);
    break;
  case SPELL_SHADOW_WALK:
    mag_affects(level, ch, tch, obj, SPELL_SHADOW_WALK, savetype, casttype, 0);
    break;
  case PSIONIC_INTELLECT_FORTRESS:
    mag_affects(level, ch, tch, obj, PSIONIC_INTELLECT_FORTRESS, savetype, casttype, 0);
    break;
  case PSIONIC_TOWER_OF_IRON_WILL:
    mag_affects(level, ch, tch, obj, PSIONIC_TOWER_OF_IRON_WILL, savetype, casttype, 0);
    break;
  default:
    if (can_mastermind_power(ch, spellnum))
    {
      if (IS_SET(spell_info[spellnum].routines, MAG_AFFECTS))
        mag_affects(level, ch, tch, obj, spellnum, savetype, casttype, 0);
      else if (IS_SET(spell_info[spellnum].routines, MAG_POINTS))
        mag_points(level, ch, tch, obj, SPELL_CURE_MODERATE, savetype, casttype);
    }
    break;
  }
}

/* Every spell that affects the group should run through here perform_mag_groups
 * contains the switch statement to send us to the right magic. Group spells
 * affect everyone grouped with the caster who is in the room, caster last. To
 * add new group spells, you shouldn't have to change anything in mag_groups.
 * Just add a new case to perform_mag_groups.
 * UPDATE:  added some to_char and to_room messages here for fun  */
void mag_groups(int level, struct char_data *ch, struct obj_data *obj,
                int spellnum, int savetype, int casttype)
{
  const char *to_char = NULL, *to_room = NULL;
  struct char_data *tch = NULL;
  bool hit_self = FALSE, hit_leader = FALSE;

  if (ch == NULL)
    return;

  switch (spellnum)
  {

  case SPELL_TACTICAL_ACUMEN:
    to_char = "You speak words of combat tactics and courage!\tn";
    to_room = "$n speaks words of combat tactics and courage!\tn";
    break;
  case SPELL_GROUP_HEAL:
    to_char = "You summon massive beams of healing light!\tn";
    to_room = "$n summons massive beams of healing light!\tn";
    break;
  case SPELL_GROUP_SHIELD_OF_FAITH:
    to_char = "You manifest magical armoring for your companions!\tn";
    to_room = "$n manifests magical armoring for $s companions!\tn";
    break;
  case SPELL_MASS_HASTE:
    to_char = "You spin quickly with an agile magical flourish!\tn";
    to_room = "$n spins quickly with an agile magical flourish!\tn";
    break;
  case SPELL_CIRCLE_A_EVIL:
    to_char = "You draw a 6-point magical star in the air!\tn";
    to_room = "$n draw a 6-point magical star in the air!\tn";
    break;
  case SPELL_CIRCLE_A_GOOD:
    to_char = "You draw a 5-point magical star in the air!\tn";
    to_room = "$n draw a 5-point magical star in the air!\tn";
    break;
  case SPELL_INVISIBILITY_SPHERE:
    to_char = "Your magicks brings forth an invisibility sphere!\tn";
    to_room = "$n brings forth an invisibility sphere!\tn";
    break;
  case SPELL_GROUP_RECALL:
    to_char = "You create a huge ball of light that consumes the area!\tn";
    to_room = "$n creates a huge ball of light that consumes the area!\tn";
    break;
  case SPELL_MASS_FLY:
    to_char = "Your magicks brings strong magical winds to aid in flight!\tn";
    to_room = "$n brings strong magical winds to aid in flight!\tn";
    break;
  case SPELL_ANIMAL_SHAPES:
    to_char = "You transform your group!\tn";
    to_room = "$n transforms $s group!\tn";
    break;
  case PSIONIC_INTELLECT_FORTRESS:
    to_char = "You manifest a psychic fortress against psionic attacks!";
    to_room = "$n manifests a psychic fortress against psionic attacks!";
    break;
  case PSIONIC_TOWER_OF_IRON_WILL:
    to_char = "You manifest a tower of iron will against psionic attacks!";
    to_room = "$n manifests a tower of iron will against psionic attacks!";
    break;
  }

  /* if you are not groupped, just hit self with this spell and exit */
  if (!GROUP(ch))
  {
    perform_mag_groups(level, ch, ch, obj, spellnum, savetype, casttype);
    return;
  }

  /* if you are not groupped, just hit self with this spell and exit */
  if ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) == NULL)
  {
    perform_mag_groups(level, ch, ch, obj, spellnum, savetype, casttype);
    return;
  }

  if (to_char != NULL)
    act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
    act(to_room, FALSE, ch, 0, 0, TO_ROOM);

  while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) !=
         NULL)
  {
    if (IN_ROOM(tch) != IN_ROOM(ch))
      continue;

    if (tch == ch) /* this is a dummy check added due to an uknown bug with lists :(  -zusuk */
      hit_self = TRUE;

    /* this is a dummy check added due to an uknown bug with lists :(  -zusuk */
    if (GROUP(ch)->leader && GROUP(ch)->leader == tch)
      hit_leader = TRUE;

    perform_mag_groups(level, ch, tch, obj, spellnum, savetype, casttype);
  }

  /* this is a dummy check added due to an uknown bug with lists :(  -zusuk */
  if (!hit_self)
  {
    perform_mag_groups(level, ch, ch, obj, spellnum, savetype, casttype);

    if (ch == GROUP(ch)->leader)
      hit_leader = TRUE;
  }

  /* this is a dummy check added due to an uknown bug with lists :(  -zusuk */
  if (!hit_leader && GROUP(ch)->leader && IN_ROOM(GROUP(ch)->leader) == IN_ROOM(ch))
    perform_mag_groups(level, ch, GROUP(ch)->leader, obj, spellnum, savetype, casttype);

  if (affected_by_spell(ch, PSIONIC_ABILITY_MASTERMIND))
    affect_from_char(ch, PSIONIC_ABILITY_MASTERMIND);
}

/** Mass spells affect every creature in the room except the caster,
 * and his group members.
 */
void mag_masses(int level, struct char_data *ch, struct obj_data *obj,
                int spellnum, int savetype, int casttype, int metamagic)
{
  struct char_data *tch = NULL, *tch_next;
  int isEffect = FALSE;
  bool isUnEffect = false;
  bool isDamage = false;
  bool skip_groups = false;

  switch (spellnum)
  {
  case SPELL_STENCH:
    isEffect = TRUE;
    break;
  case SPELL_ACID:
    isDamage = true;
    break;
  case SPELL_BLADES:
    isDamage = true;
    break;
  /** Psionics **/
  case PSIONIC_DEMORALIZE:
    isEffect = TRUE;
    skip_groups = true;
    break;
  case PSIONIC_ENERGY_STUN:
    isDamage = true;
    isEffect = TRUE;
    skip_groups = true;
    break;

  case PSIONIC_PSIONIC_BLAST:
    // because this only benefits from intervals of 2 psp.
    // we aren't deducting psp in mag_damage, we do it here.
    isEffect = TRUE;
    skip_groups = true;
    break;

  case PSIONIC_INFLICT_PAIN:
    // because this only benefits from intervals of 2 psp.
    // we aren't deducting psp in mag_damage, we do it here.
    if (GET_AUGMENT_PSP(ch) < 4)
    {
      // need to spend 4 augmented psp or more in order for it to be an AoE effect
      mag_affects(level, ch, tch, obj, spellnum, metamagic, 1, casttype);
      return;
    }
    isEffect = TRUE;
    skip_groups = true;
    break;

  case PSIONIC_MENTAL_DISRUPTION:
    // because this only benefits from intervals of 2 psp.
    // we aren't deducting psp in mag_damage, we do it here.
    isEffect = TRUE;
    skip_groups = true;
    break;

  case PSIONIC_ERADICATE_INVISIBILITY:
    // because this only benefits from intervals of 2 psp.
    // we aren't deducting psp in mag_unaffects, we do it here.
    isUnEffect = TRUE;
    skip_groups = true;
    break;

  case PSIONIC_DEADLY_FEAR:
    // because this only benefits from intervals of 2 psp.
    // we aren't deducting psp in mag_damage, we do it here.
    if (GET_AUGMENT_PSP(ch) < 8)
    {
      // need to spend 4 augmented psp or more in order for it to be an AoE effect
      mag_affects(level, ch, tch, obj, spellnum, metamagic, 1, casttype);
      return;
    }
    isEffect = TRUE;
    skip_groups = true;
    break;

  case PSIONIC_SHATTER_MIND_BLANK:
    isUnEffect = TRUE;
    skip_groups = true;
    break;
  }

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next)
  {
    tch_next = tch->next_in_room;
    if (tch == ch)
      continue;

    if (skip_groups && !aoeOK(ch, tch, spellnum))
      continue;

    if (isEffect)
      mag_affects(level, ch, tch, obj, spellnum, savetype, casttype, metamagic);
    if (isUnEffect)
      mag_unaffects(level, ch, tch, obj, spellnum, savetype, casttype);
    if (isDamage)
      mag_damage(level, ch, tch, obj, spellnum, metamagic, 1, casttype);
  }
}

// CHARM PERSON CHARM_PERSON - enchantment (found in spells.c)
// ENCHANT WEAPON ENCHANT_ITEM - enchantment (spells.c)

//  pass values spellnum
// spellnum = spellnum
// -1 = no spellnum, no special handling
// -2 = tailsweep
//  return values
// 0 = not allowed to hit target
// 1 = allowed to hit target

int aoeOK(struct char_data *ch, struct char_data *tch, int spellnum)
{
  // skip self - tested
  if (tch == ch)
    return 0;

  // immorts that are nohas
  if (!IS_NPC(tch) && GET_LEVEL(tch) >= LVL_IMMORT &&
      PRF_FLAGGED(tch, PRF_NOHASSLE))
    return 0;

  // earthquake currently doesn't work on flying victims
  if ((spellnum == SPELL_EARTHQUAKE) && is_flying(tch))
    return 0;

  // tailsweep currently doesn't work on flying victims
  if ((spellnum == -2) && is_flying(tch))
    return 0;

  // same group, skip
  if (GROUP(tch) && GROUP(ch) && GROUP(ch) == GROUP(tch))
    return 0;

  if (IS_NPC(tch) && tch->mission_owner > 0 && !is_mission_mob(ch, tch))
    return 0;

  // don't hit the charmee of a group member
  if (tch->master)
    if (AFF_FLAGGED(tch, AFF_CHARM) &&
        GROUP(tch->master) && GROUP(ch) && GROUP(ch) == GROUP(tch->master))
      return 0;

  // charmee, don't hit a group member of master
  if (ch->master)
    if (AFF_FLAGGED(ch, AFF_CHARM) &&
        GROUP(ch->master) && GROUP(tch) && GROUP(tch) == GROUP(ch->master))
      return 0;

  // charmee, don't hit a charmee of group member of master
  if (ch->master && tch->master)
    if (AFF_FLAGGED(ch, AFF_CHARM) && AFF_FLAGGED(tch, AFF_CHARM) &&
        GROUP(ch->master) && GROUP(tch->master) &&
        GROUP(ch->master) == GROUP(tch->master))
      return 0;

  // don't hit your master
  if (ch->master)
    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master == tch)
      return 0;

  // don't hit your master's ungrouped charmies

  // don't hit your charmee
  if (tch->master)
    if (AFF_FLAGGED(tch, AFF_CHARM) && tch->master == ch)
      return 0;

  // npc that isn't charmed shouldn't hurt other npc's
  if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_CHARM) && IS_NPC(tch))
    return 0;

  // PK MUD settings
  if (CONFIG_PK_ALLOWED)
  {
    // PK settings
  }
  else
  {

    // if pc cast, !pk mud, skip pc
    if (!IS_NPC(ch) && !IS_NPC(tch))
      return 0;

    // do not hit pc charmee
    if (!IS_NPC(ch) && AFF_FLAGGED(tch, AFF_CHARM) && !IS_NPC(tch->master))
      return 0;

    // charmee shouldn't hit pc's
    if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) &&
        !IS_NPC(tch))
      return 0;
  }

  return 1;
}

/* Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual damage.
 * All spells listed here must also have a case in mag_damage() in order for
 * them to work. Area spells have limited targets within the room. */
void mag_areas(int level, struct char_data *ch, struct obj_data *obj,
               int spellnum, int metamagic, int savetype, int casttype)
{
  struct char_data *tch = NULL, *next_tch = NULL;
  const char *to_char = NULL, *to_room = NULL;
  int isEffect = FALSE, is_eff_and_dam = FALSE, is_uneffect = FALSE;

  if (ch == NULL)
    return;

  /* to add spells just add the message here plus an entry in mag_damage for
   * the damaging part of the spell.   */
  switch (spellnum)
  {
  case SPELL_CALL_LIGHTNING_STORM:
    to_char = "You call down a furious lightning storm upon the area!";
    to_room = "$n raises $s arms and calls down a furious lightning storm!";
    break;
  case SPELL_CHAIN_LIGHTNING:
    to_char = "Arcing bolts of lightning flare from your fingertips!";
    to_room = "Arcing bolts of lightning fly from the fingers of $n!";
    break;
  case SPELL_DEATHCLOUD: // cloudkill
    break;
  case SPELL_EARTHQUAKE:
    to_char = "You gesture and the earth begins to shake all around you!";
    to_room = "$n gracefully gestures and the earth begins to shake violently!";
    break;
  case SPELL_ENFEEBLEMENT:
    isEffect = TRUE;
    to_char = "You invoke a powerful enfeeblement enchantment!\tn";
    to_room = "$n invokes a powerful enfeeblement enchantment!\tn";
    break;
  case SPELL_FAERIE_FOG:
    is_uneffect = TRUE;
    to_char = "You summon faerie fog!\tn";
    to_room = "$n summons faerie fog!\tn";
    break;
  case SPELL_FIRE_STORM:
    to_char = "You call forth sheets of roaring flame!";
    to_room = "$n calls forth sheets of roaring flame!";
    break;
  case SPELL_FLAMING_SPHERE:
    to_char = "You summon a burning globe of fire that rolls through the area!";
    to_room = "$n summons a burning globe of fire that rolls through the area!";
    break;
  case SPELL_HALT_UNDEAD:
    isEffect = TRUE;
    to_char = "\tDYou invoke a powerful halt spell!\tn";
    to_room = "$n\tD invokes a powerful halt spell!\tn";
    break;
  case SPELL_HELLBALL:
    to_char = "\tMYou conjures a pure ball of Hellfire!\tn";
    to_room = "$n\tM conjures a pure ball of Hellfire!\tn";
    break;
  case SPELL_HORRID_WILTING:
    to_char = "Your wilting causes the moisture to leave the area!";
    to_room = "$n's horrid wilting causes all the moisture to leave the area!";
    break;
  case SPELL_ICE_STORM:
    to_char = "You conjure a storm of ice that blankets the area!";
    to_room = "$n conjures a storm of ice, blanketing the area!";
    break;
  case SPELL_INCENDIARY: // incendiary cloud
    break;
  case SPELL_INSECT_PLAGUE:
    to_char = "You summon a swarm of locusts into the area!";
    to_room = "$n summons a swarm of locusts into the area!";
    break;
  case SPELL_MASS_HOLD_PERSON:
    isEffect = TRUE;
    to_char = "You invoke a powerful hold person enchantment!\tn";
    to_room = "$n invokes a powerful hold person enchantment!\tn";
    break;
  case SPELL_METEOR_SWARM:
    to_char = "You call down meteors from the sky to pummel your foes!";
    to_room = "$n invokes a swarm of meteors to rain from the sky!";
    break;
  case SPELL_PRISMATIC_SPRAY:
    is_eff_and_dam = TRUE;
    to_char = "\tnYou fire from your hands a \tYr\tRa\tBi\tGn\tCb\tWo\tDw\tn of color!\tn";
    to_room = "$n \tnfires from $s hands a \tYr\tRa\tBi\tGn\tCb\tWo\tDw\tn of color!\tn";
    break;
  case SPELL_SUNBEAM:
    is_eff_and_dam = TRUE;
    to_char = "\tnYou bring forth a powerful sunbeam!\tn";
    to_room = "$n brings forth a powerful sunbeam!\tn";
    break;
  case SPELL_SUNBURST:
    is_eff_and_dam = TRUE;
    to_char = "\tnYou bring forth a powerful sunburst!\tn";
    to_room = "$n brings forth a powerful sunburst!\tn";
    break;
  case SPELL_THUNDERCLAP:
    is_eff_and_dam = TRUE;
    to_char = "\tcA loud \twCRACK\tc fills the air with deafening force!\tn";
    to_room = "\tcA loud \twCRACK\tc fills the air with deafening force!\tn";
    break;
  case SPELL_WAIL_OF_THE_BANSHEE:
    is_eff_and_dam = TRUE;
    to_char = "You emit a terrible banshee wail!\tn";
    to_room = "$n emits a terrible banshee wail!\tn";
    break;
  case SPELL_WAVES_OF_EXHAUSTION:
    isEffect = TRUE;
    to_char = "\tDYou muster the power of death creating waves of exhaustion!\tn";
    to_room = "$n\tD musters the power of death creating waves of exhaustion!\tn";
    break;
  case SPELL_WAVES_OF_FATIGUE:
    isEffect = TRUE;
    to_char = "\tDYou muster the power of death creating waves of fatigue!\tn";
    to_room = "$n\tD musters the power of death creating waves of fatigue!\tn";
    break;
  case SPELL_CIRCLE_OF_DEATH:
    to_char = "\tDA wave of negative energy bursts forth from your body!\tn";
    to_room = "\tDA wave of negative energy bursts forth from $n's body!\tn";
    break;
  case SPELL_UNDEATH_TO_DEATH:
    to_char = "\tDA wave of undead-disrupting energy bursts forth from your body!\tn";
    to_room = "\tDA wave of undead-disrupting energy bursts forth from $n's body!\tn";
    break;
  case SPELL_GRASP_OF_THE_DEAD:
    to_char = "\tDYou pull your arms up above your head, bringing with it a swarm of skeletal arms that burst from the ground!\tn";
    to_room = "\tD$n pulls $s arms up above $s head, bringing with it a swarm of skeletal arms that burst from the ground!\tn";
    break;
  case SPELL_WHIRLWIND:
    to_char = "You call down a rip-roaring cyclone on the area!";
    to_room = "$n calls down a rip-roaring cyclone on the area!";
    break;
  case SPELL_INVISIBILITY_PURGE:
    is_uneffect = TRUE;
    to_char = "You throw a field of invisibility purging magic across the area.";
    to_room = "$n throws a field of invisibility purging magic across the area.";
    break;

  /** Psionics **/
  case PSIONIC_CONCUSSION_BLAST:
    // because this only benefits from intervals of 2 psp.
    // we aren't deducting psp in mag_damage, we do it here.
    if (GET_AUGMENT_PSP(ch) < 4)
    {
      // need to spend 4 augmented psp or more in order for it to be an AoE effect
      mag_damage(level, ch, tch, obj, spellnum, metamagic, 1, casttype);
      return;
    }
    to_char = "You manifest a massive blast of concussive force!";
    to_room = "A massive blast of concussive force explodes from around $n!";
    break;
  case PSIONIC_SWARM_OF_CRYSTALS:
    // we aren't deducting psp in mag_damage, we do it here.
    to_char = "You spread your arms, shooting out thousands of tiny crystal shards!";
    to_room = "$n spreads $s arms, shooting out thousands of tiny crystal shards!";
    break;
  case PSIONIC_ENERGY_BURST:
    to_char = "You spread your arms, a burst of energy shooting outward!";
    to_room = "$n spreads $s arms, shooting out a burst of energy!";
    break;
  case PSIONIC_UPHEAVAL:
    to_char = "You manifest a psychic upheaval that assails the minds of all nearby!";
    to_room = "A psychic upheaval from $n assails your mind with anguish!";
    break;
  case PSIONIC_SHRAPNEL_BURST:
    to_char = "You spread your arms, shooting out thousands of shrapnel!";
    to_room = "$n spreads $s arms, shooting out thousands of shrapnel!";
    break;
  case PSIONIC_BREATH_OF_THE_BLACK_DRAGON:
    to_char = "You manifest a blast of acid that assails all nearby!";
    to_room = "A blast of acid shoots from the mouth of $n assailing all nearby!";
    break;
  case PSIONIC_PSYCHOSIS:
    isEffect = TRUE;
    // because this only benefits from intervals of 2 psp.
    // we aren't deducting psp in mag_affects, we do it here.
    if (GET_AUGMENT_PSP(ch) < 6)
    {
      // need to spend 6 augmented psp or more in order for it to be an AoE effect
      mag_affects(level, ch, tch, obj, spellnum, metamagic, 1, casttype);
      return;
    }
    to_char = "You manifest a massive wave of confusion!";
    to_room = "A massive wave of confusion envelops $n!";
    break;
  case PSIONIC_ULTRABLAST:
    to_char = "You manifest a psychic shriek that assails the minds of all nearby!";
    to_room = "A psychic shriek from $n assails your mind with severe anguish!";
    break;

    /** abilities (channel, etc) **/

  case ABILITY_CHANNEL_POSITIVE_ENERGY:
    to_char = "You channel a massive wave of holy energy!";
    to_room = "A massive wave of holy energy envelops $n!";
    break;
  case ABILITY_CHANNEL_NEGATIVE_ENERGY:
    to_char = "You channel a massive wave of unholy energy!";
    to_room = "A massive wave of unholy energy envelops $n!";
    break;

    /** NPC **/
  case SPELL_FIRE_BREATHE:
    to_char = "You exhale breathing out fire!";
    to_room = "$n exhales breathing fire!";
    break;
  case SPELL_GAS_BREATHE:
    to_char = "You exhale breathing out gas!";
    to_room = "$n exhales breathing gas!";
    break;
  case SPELL_FROST_BREATHE:
    to_char = "You exhale breathing out frost!";
    to_room = "$n exhales breathing frost!";
    break;
  case SPELL_LIGHTNING_BREATHE:
    to_char = "You exhale breathing out lightning!";
    to_room = "$n exhales breathing lightning!";
    break;
  case SPELL_ACID_BREATHE:
    to_char = "You exhale breathing out acid!";
    to_room = "$n exhales breathing acid!";
    break;
  case SPELL_POISON_BREATHE:
    to_char = "You exhale breathing out poison!";
    to_room = "$n exhales breathing poison!";
    break;
  case SPELL_DRACONIC_BLOODLINE_BREATHWEAPON:
    switch (draconic_heritage_energy_types[GET_BLOODLINE_SUBTYPE(ch)])
    {
    case DAM_FIRE:
      to_char = "You exhale breathing out fire!";
      to_room = "$n exhales breathing fire!";
      break;
    case DAM_COLD:
      to_char = "You exhale breathing out frost!";
      to_room = "$n exhales breathing frost!";
      break;
    case DAM_ELECTRIC:
      to_char = "You exhale breathing out lightning!";
      to_room = "$n exhales breathing lightning!";
      break;
    case DAM_ACID:
      to_char = "You exhale breathing out acid!";
      to_room = "$n exhales breathing acid!";
      break;
    case DAM_POISON:
      to_char = "You exhale breathing out poison!";
      to_room = "$n exhales breathing poison!";
      break;
    default:
      to_char = "Error DRHRTBREATH_001a, please report to a member of staff.";
      to_room = "Error DRHRTBREATH_001b, please report to a member of staff.";
      break;
    }
    break;

  default:
    if (can_mastermind_power(ch, spellnum))
    {
      if (IS_SET(spell_info[spellnum].routines, MAG_DAMAGE) && IS_SET(spell_info[spellnum].routines, MAG_AFFECTS))
        is_eff_and_dam = true;
      else if (IS_SET(spell_info[spellnum].routines, MAG_DAMAGE))
        ;
      else if (IS_SET(spell_info[spellnum].routines, MAG_AFFECTS))
        isEffect = true;
      else if (IS_SET(spell_info[spellnum].routines, MAG_UNAFFECTS))
        is_uneffect = true;
      else
        return; // can only handle damage, effects and unaffects.
      affect_from_char(ch, PSIONIC_ABILITY_MASTERMIND);
    }
    break;
  }

  if (to_char != NULL)
    act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
    act(to_room, FALSE, ch, 0, 0, TO_ROOM);

  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
  {
    next_tch = tch->next_in_room;

    if (aoeOK(ch, tch, spellnum))
    {
      if (spellnum == ABILITY_CHANNEL_POSITIVE_ENERGY && !IS_UNDEAD(tch))
        continue;
      else if (spellnum == ABILITY_CHANNEL_NEGATIVE_ENERGY && IS_UNDEAD(tch))
        continue;
      if (is_eff_and_dam)
      {
        mag_damage(level, ch, tch, obj, spellnum, metamagic, 1, casttype);
        mag_affects(level, ch, tch, obj, spellnum, savetype, casttype, metamagic);
      }
      else if (isEffect)
        mag_affects(level, ch, tch, obj, spellnum, savetype, casttype, metamagic);
      else if (is_uneffect)
        mag_unaffects(level, ch, tch, obj, spellnum, savetype, casttype);
      else
        mag_damage(level, ch, tch, obj, spellnum, metamagic, 1, casttype);

      /* we gotta start combat here */
      if (isEffect && spell_info[spellnum].violent && tch && GET_POS(tch) == POS_STANDING &&
          !FIGHTING(tch) && spellnum != SPELL_CHARM && spellnum != SPELL_CHARM_ANIMAL &&
          spellnum != SPELL_DOMINATE_PERSON && spellnum != SPELL_MASS_DOMINATION)
      {
        if (tch != ch)
        { // funny results from potions/scrolls
          if (IN_ROOM(tch) == IN_ROOM(ch))
          {
            hit(tch, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
          }
        }
      } /* end start combat */

    } /* end aoeOK */
  }   /* for loop for cycling through room chars */
}

/*----------------------------------------------------------------------------*/
/* Begin Magic Summoning - Generic Routines and Local Globals */
/*----------------------------------------------------------------------------*/

/* Every spell which summons/gates/conjours a mob comes through here. */
/* These use act(), don't put the \r\n. */
static const char *mag_summon_msgs[] = {
    "\r\n",                                                                                     // 0
    "$n makes a strange magical gesture; you feel a strong breeze!",                            // 1
    "$n animates a corpse!",                                                                    // 2
    "$N appears from a cloud of thick blue smoke!",                                             // 3
    "$N appears from a cloud of thick green smoke!",                                            // 4
    "$N appears from a cloud of thick red smoke!",                                              // 5
    "$N disappears in a thick black cloud!",                                                    // 6
    "\tCAs \tn$n\tC makes a strange magical gesture, you feel a strong breeze.\tn",             // 7
    "\tRAs \tn$n\tR makes a strange magical gesture, you feel a searing heat.\tn",              // 8
    "\tYAs \tn$n\tY makes a strange magical gesture, you feel a sudden shift in the earth.\tn", // 9
    "\tBAs \tn$n\tB makes a strange magical gesture, you feel the dust swirl.\tn",              // 10
    "$n magically divides!",                                                                    // 11 clone
    "$n animates a corpse!",                                                                    // 12 animate dead
    "$N breaks through the ground and bows before $n.",                                         // 13 mummy lord
    "With a roar $N soars to the ground next to $n.",                                           // 14 young red dragon
    "$N pops into existence next to $n.",                                                       // 15 shelgarn's dragger
    "$N skimpers into the area, then quickly moves next to $n.",                                // 16 dire badger
    "$N charges into the area, looks left, then right... "                                      /* 17 */
    "then quickly moves next to $n.",                                                           // 18 dire boar
    "$N moves into the area, sniffing cautiously.",                                             // 19 dire wolf
    "$N neighs and walks up to $n.",                                                            // 20 phantom steed
    "$N skitters into the area and moves next to $n.",                                          // 21 dire spider
    "$N lumbers into the area and moves next to $n.",                                           // 22 dire bear
    "$N manifests with an ancient howl, then moves towards $n.",                                // 23 hound
    "$N stalks into the area, roars loudly, then moves towards $n.",                            // 24 d tiger
    "$N pops into existence next to $n.",                                                       // 25 black blade of disaster
    "$N skulks into the area, seemingly from nowhere!",                                         // 26 shambler
    "\tCYou make a magical gesture, you feel a strong breeze.\tn",                              // 27 air elemental
    "\tYYou make a magical gesture, you feel a sudden shift in the earth.\tn",                  // 28 earth elemental
    "\tRYou make a magical gesture, you feel a searing heat.\tn",                               // 29 fire elemental
    "\tBYou make a magical gesture, you feel the dust swirl.\tn",                               // 30 water elemental
    "$N skulks into the area, seemingly from nowhere!",                                         // 31 shambling mound
    "$N strides into the area with threatening growls!",                                        // 32 children of the night wolves
    "$N creep into the area with horribly noisy squeeks",                                       // 33 children of the night rats
    "$N flies into the area screeching loudly.",                                                // 34 children of the night bats
    "$n raises $n!",                                                                            // 35 create vampire spawn
    "\r\n",                                                                                     // filler
    "\r\n",                                                                                     // filler
    "\r\n",                                                                                     // filler
    "\r\n",                                                                                     // filler
    "\r\n",                                                                                     // filler
    "\r\n",                                                                                     // filler
};

static const char *mag_summon_to_msgs[] = {
    "\r\n",                                                                    // 0
    "You make the magical gesture; you feel a strong breeze!",                 // 1
    "You animate a corpse!",                                                   // 2
    "You conjure $N from a cloud of thick blue smoke!",                        // 3
    "You conjure $N from a cloud of thick green smoke!",                       // 4
    "You conjure $N from a cloud of thick red smoke!",                         // 5
    "You make $N appear in a thick black cloud!",                              // 6
    "\tCYou make a magical gesture, you feel a strong breeze.\tn",             // 7
    "\tRYou make a magical gesture, you feel a searing heat.\tn",              // 8
    "\tYYou make a magical gesture, you feel a sudden shift in the earth.\tn", // 9
    "\tBYou make a magical gesture, you feel the dust swirl.\tn",              // 10
    "You magically divide!",                                                   // 11 clone
    "You animate a corpse!",                                                   // 12 animate dead
    "$N breaks through the ground and bows before you.",                       // 13 mummy lord
    "With a roar $N soars to the ground next to you.",                         // 14 young red dragon
    "$N pops into existence next to you.",                                     // 15 shelgarn's dragger
    "$N skimpers into the area, then quickly moves next to you.",              // 16 dire badger
    "$N charges into the area, looks left, then right... "                     // 17
    "then quickly moves next to you.",                                         // 18 dire boar
    "$N moves into the area, sniffing cautiously.",                            // 19 dire wolf
    "$N neighs and walks up to you.",                                          // 20 phantom steed
    "$N skitters into the area and moves next to you.",                        // 21 dire spider
    "$N lumbers into the area and moves next to you.",                         // 22 dire bear
    "$N manifests with an ancient howl, then moves towards you.",              // 23 hound
    "$N stalks into the area, roars loudly, then moves towards you.",          // 24 d tiger
    "$N pops into existence next to you.",                                     // 25 black blade of disaster
    "$N skulks into the area, seemingly from nowhere!",                        // 26 shambler
    "\tCYou make a magical gesture, you feel a strong breeze.\tn",             // 27 air elemental
    "\tYYou make a magical gesture, you feel a sudden shift in the earth.\tn", // 28 earth elemental
    "\tRYou make a magical gesture, you feel a searing heat.\tn",              // 29 fire elemental
    "\tBYou make a magical gesture, you feel the dust swirl.\tn",              // 30 water elemental
    "$N skulks into the area, seemingly from nowhere!",                        // 31 shambling mound
    "$N strides into the area with threatening growls!",                       // 32 children of the night wolves
    "$N creep into the area with horribly noisy squeeks",                      // 33 children of the night rats
    "$N flies into the area screeching loudly.",                               // 34 children of the night bats
    "$n raises $n!",                                                           // 35 create vampire spawn
    "\r\n",                                                                    // filler
    "\r\n",                                                                    // filler
    "\r\n",                                                                    // filler
    "\r\n",                                                                    // filler
    "\r\n",                                                                    // filler
    "\r\n",                                                                    // filler
};

/* Keep the \r\n because these use send_to_char. */
static const char *mag_summon_fail_msgs[] = {
    "\r\n",
    "There are no such creatures.\r\n",
    "Uh oh...\r\n",
    "Oh dear.\r\n",
    "Gosh durnit!\r\n",
    "The elements resist!\r\n",
    "You failed.\r\n",
    "There is no corpse!\r\n",
    "Your summons go unanswered.\r\n",
};

/* Defines for Mag_Summons */
// objects
#define OBJ_CLONE 161 /**< vnum for clone material. */
// mobiles
#define MOB_CLONE 10          /**< vnum for the clone mob. */
#define MOB_ZOMBIE 11         /* animate dead levels 1-7 */
#define MOB_GHOUL 35          // " " level 11+
#define MOB_GIANT_SKELETON 36 // " " level 21+
#define MOB_MUMMY 37          // " " level 30
#define MOB_MUMMY_LORD 38     // epic spell mummy dust
#define MOB_RED_DRAGON 39     // epic spell dragon knight
#define MOB_SHELGARNS_BLADE 40
#define MOB_DIRE_BADGER 41 // summon creature i
#define MOB_DIRE_BOAR 42   // " " ii
#define MOB_DIRE_WOLF 43   // " " iii
#define MOB_PHANTOM_STEED 44
// 45    wizard eye
#define MOB_DIRE_SPIDER 46 // summon creature iv
// 47    wall of force
#define MOB_DIRE_BEAR 48 // summon creature v
#define MOB_HOUND 49
#define MOB_DIRE_TIGER 50 // summon creature vi
#define MOB_FIRE_ELEMENTAL 51
#define MOB_EARTH_ELEMENTAL 52
#define MOB_AIR_ELEMENTAL 53
#define MOB_WATER_ELEMENTAL 54                // these elementals are for rest of s.c.
#define MOB_GHOST 55                          // great animation
#define MOB_SPECTRE 56                        // great animation
#define MOB_BANSHEE 57                        // great animation
#define MOB_WIGHT 58                          // great animation
#define MOB_BLADE_OF_DISASTER 59              // black blade of disaster
#define MOB_DIRE_RAT 9400                     // summon natures ally i
#define MOB_ECTOPLASMIC_SHAMBLER 93           // ectoplasmic shambler psionic ability
#define MOB_CHILDREN_OF_THE_NIGHT_WOLVES 9419 // Potential mob for children of the night vampire ability.
#define MOB_CHILDREN_OF_THE_NIGHT_RATS 9420   // Potential mob for children of the night vampire ability.
#define MOB_CHILDREN_OF_THE_NIGHT_BATS 9421   // Potential mob for children of the night vampire ability.
#define MOB_CREATE_VAMPIRE_SPAWN 9422         // Mob to use for create vampire spawn

bool isSummonMob(int vnum)
{
  switch (vnum)
  {
  case MOB_ZOMBIE:
  case MOB_GHOUL:
  case MOB_GIANT_SKELETON:
  case MOB_MUMMY:
  case MOB_MUMMY_LORD:
  case MOB_RED_DRAGON:
  case MOB_SHELGARNS_BLADE:
  case MOB_DIRE_BADGER:
  case MOB_DIRE_BOAR:
  case MOB_DIRE_WOLF:
  case MOB_PHANTOM_STEED:
  case MOB_DIRE_SPIDER:
  case MOB_DIRE_BEAR:
  case MOB_HOUND:
  case MOB_DIRE_TIGER:
  case MOB_FIRE_ELEMENTAL:
  case MOB_EARTH_ELEMENTAL:
  case MOB_AIR_ELEMENTAL:
  case MOB_WATER_ELEMENTAL:
  case MOB_GHOST:
  case MOB_SPECTRE:
  case MOB_BANSHEE:
  case MOB_WIGHT:
  case MOB_BLADE_OF_DISASTER:
  case MOB_DIRE_RAT:
  case MOB_ECTOPLASMIC_SHAMBLER:
  case 9412: // air elemental
  case 9413: // earth elemental
  case 9414: // fire elemental
  case 9415: // water elemental
  case 9499: // shambling mound
  case MOB_CHILDREN_OF_THE_NIGHT_WOLVES:
  case MOB_CHILDREN_OF_THE_NIGHT_RATS:
  case MOB_CHILDREN_OF_THE_NIGHT_BATS:
  case MOB_CREATE_VAMPIRE_SPAWN:
    return true;
  }
  return false;
}

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
                 int spellnum, int savetype, int casttype)
{

  struct char_data *mob = NULL;
  struct obj_data *tobj, *next_obj;
  int pfail = 0, msg = 0, fmsg = 0, num = 1, handle_corpse = FALSE, i;
  int mob_level = 0, temp_level = 0;
  mob_vnum mob_num = 0;
  char desc[200];

  if (ch == NULL)
    return;

  switch (spellnum)
  {

  case SPELL_CLONE:
    msg = 11;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_CLONE;
    /*
     * We have designated the clone spell as the example for how to use the
     * mag_materials function.
     * In stock FaerunMUD it checks to see if the character has item with
     * vnum 161 which is a set of sacrificial entrails. If we have the entrails
     * the spell will succeed,  and if not, the spell will fail 102% of the time
     * (prevents random success... see below).
     * The object is extracted and the generic cast messages are displayed.
     */
    if (!mag_materials(ch, OBJ_CLONE, NOTHING, NOTHING, TRUE, TRUE))
      pfail = 102; /* No materials, spell fails. */
    else
      pfail = 0; /* We have the entrails, spell is successfully cast. */
    break;

  case VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT: // necromancy
    fmsg = 8;
    mob_num = dice(1, 3) - 1 + MOB_CHILDREN_OF_THE_NIGHT_WOLVES;
    switch (mob_num)
    {
    case MOB_CHILDREN_OF_THE_NIGHT_WOLVES:
      msg = 32;
      break;
    case MOB_CHILDREN_OF_THE_NIGHT_RATS:
      msg = 33;
      break;
    case MOB_CHILDREN_OF_THE_NIGHT_BATS:
      msg = 34;
      break;
    }
    pfail = 10;
    break;

  case SPELL_ANIMATE_DEAD: // necromancy
    if (obj == NULL || !IS_CORPSE(obj))
    {
      act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (IS_HOLY(IN_ROOM(ch)))
    {
      send_to_char(ch, "This place is too holy for such blasphemy!");
      return;
    }
    handle_corpse = TRUE;
    msg = 12;
    fmsg = rand_number(2, 6); /* Random fail message. */
    if (CASTER_LEVEL(ch) >= 30)
      mob_num = MOB_MUMMY;
    else if (CASTER_LEVEL(ch) >= 20)
      mob_num = MOB_GIANT_SKELETON;
    else if (CASTER_LEVEL(ch) >= 10)
      mob_num = MOB_GHOUL;
    else
      mob_num = MOB_ZOMBIE;
    pfail = 10; /* 10% failure, should vary in the future. */
    break;

  case ABILITY_CREATE_VAMPIRE_SPAWN: // necromancy
    if (obj == NULL || !IS_CORPSE(obj))
    {
      act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (IS_HOLY(IN_ROOM(ch)))
    {
      send_to_char(ch, "This place is too holy for such blasphemy!");
      return;
    }
    handle_corpse = TRUE;
    msg = 35;
    fmsg = 6;
    mob_num = MOB_CREATE_VAMPIRE_SPAWN;
    pfail = 0;
    break;

  case SPELL_GREATER_ANIMATION: // necromancy
    if (obj == NULL || !IS_CORPSE(obj))
    {
      act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    handle_corpse = TRUE;
    msg = 12;
    fmsg = rand_number(2, 6); /* Random fail message. */
    if (CASTER_LEVEL(ch) >= 30)
    {
      mob_num = MOB_WIGHT;
      mob_level = rand_number(MIN(CASTER_LEVEL(ch) - 1, 27),
                              CASTER_LEVEL(ch));
    }
    else if (CASTER_LEVEL(ch) >= 25)
    {
      mob_num = MOB_BANSHEE;
      mob_level = rand_number(MIN(CASTER_LEVEL(ch) - 1, 23),
                              CASTER_LEVEL(ch));
    }
    else if (CASTER_LEVEL(ch) >= 20)
    {
      mob_num = MOB_SPECTRE;
      mob_level = rand_number(MIN(CASTER_LEVEL(ch) - 1, 19),
                              CASTER_LEVEL(ch));
    }
    else
    {
      mob_num = MOB_GHOST;
      mob_level = rand_number(MIN(CASTER_LEVEL(ch) - 1, 15),
                              CASTER_LEVEL(ch));
    }
    pfail = 10; /* 10% failure, should vary in the future. */

    break;

  case SPELL_MUMMY_DUST: // epic
    handle_corpse = FALSE;
    msg = 13;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_MUMMY_LORD;
    pfail = 0;
    mob_level = 30;
    break;

  case SPELL_DRAGON_KNIGHT: // epic
    handle_corpse = FALSE;
    msg = 14;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_RED_DRAGON;
    pfail = 0;
    mob_level = 30;
    break;

  case SPELL_ELEMENTAL_SWARM: // conjuration
    handle_corpse = FALSE;
    fmsg = rand_number(2, 6);
    mob_num = 9412 + rand_number(0, 3); // 9412-9415
    switch (mob_num)
    {
    case 9412:
      msg = 7;
      break;
    case 9413:
      msg = 9;
      break;
    case 9414:
      msg = 8;
      break;
    case 9415:
      msg = 10;
      break;
    }
    num = dice(2, 4);
    break;

  case SPELL_FAITHFUL_HOUND: // divination
    handle_corpse = FALSE;
    msg = 22;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_HOUND;
    pfail = 0;
    break;

  case SPELL_PHANTOM_STEED: // conjuration
    handle_corpse = FALSE;
    msg = 19;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_PHANTOM_STEED;
    pfail = 0;
    break;

  case SPELL_SHAMBLER: // conjuration
    handle_corpse = FALSE;
    msg = 25;
    fmsg = rand_number(2, 6);
    mob_num = 9499;
    num = dice(1, 4) + 2;
    break;

  case SPELL_SHELGARNS_BLADE: // divination
    handle_corpse = FALSE;
    msg = 15;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_SHELGARNS_BLADE;
    pfail = 0;
    break;

  case SPELL_SUMMON_NATURES_ALLY_1:
  case SPELL_SUMMON_CREATURE_1: // conjuration
    handle_corpse = FALSE;
    msg = 16;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_DIRE_BADGER;
    pfail = 0;
    break;

  case SPELL_SUMMON_NATURES_ALLY_2:
  case SPELL_SUMMON_CREATURE_2: // conjuration
    handle_corpse = FALSE;
    msg = 17;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_DIRE_BOAR;
    pfail = 0;
    break;

  case SPELL_SUMMON_NATURES_ALLY_3:
  case SPELL_SUMMON_CREATURE_3: // conjuration
    handle_corpse = FALSE;
    msg = 18;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_DIRE_WOLF;
    pfail = 0;
    break;

  case SPELL_SUMMON_NATURES_ALLY_4:
  case SPELL_SUMMON_CREATURE_4: // conjuration
    handle_corpse = FALSE;
    msg = 20;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_DIRE_SPIDER;
    pfail = 0;
    break;

  case SPELL_SUMMON_NATURES_ALLY_5:
  case SPELL_SUMMON_CREATURE_5: // conjuration
    handle_corpse = FALSE;
    msg = 21;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_DIRE_BEAR;
    pfail = 0;
    break;

  case SPELL_SUMMON_NATURES_ALLY_6:
  case SPELL_SUMMON_CREATURE_6: // conjuration
    handle_corpse = FALSE;
    msg = 23;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_DIRE_TIGER;
    pfail = 0;
    break;

  case SPELL_BLADE_OF_DISASTER: // evocation
    handle_corpse = FALSE;
    msg = 24;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_BLADE_OF_DISASTER;
    pfail = 0;

    mob_level = rand_number(MIN(CASTER_LEVEL(ch) - 1, 14),
                            MIN(CASTER_LEVEL(ch), 20));

    break;

  case SPELL_SUMMON_NATURES_ALLY_9:
  case SPELL_SUMMON_CREATURE_9: // conjuration
    mob_level = MAX(18, CASTER_LEVEL(ch) - rand_number(0, 5));
  case SPELL_SUMMON_NATURES_ALLY_8:
  case SPELL_SUMMON_CREATURE_8: // conjuration
    if (!mob_level)
      mob_level = MAX(16, CASTER_LEVEL(ch) - rand_number(3, 8));
  case SPELL_SUMMON_NATURES_ALLY_7:
  case SPELL_SUMMON_CREATURE_7: // conjuration
    if (!mob_level)
      mob_level = MAX(14, CASTER_LEVEL(ch) - rand_number(5, 10));

    handle_corpse = FALSE;

    fmsg = rand_number(2, 6); /* Random fail message. */

    switch (dice(1, 4))
    {
    case 1:
      mob_num = MOB_FIRE_ELEMENTAL;
      msg = 8;
      break;
    case 2:
      mob_num = MOB_EARTH_ELEMENTAL;
      msg = 9;
      break;
    case 3:
      mob_num = MOB_AIR_ELEMENTAL;
      msg = 7;
      break;
    case 4:
      mob_num = MOB_WATER_ELEMENTAL;
      msg = 10;
      break;
    }

    pfail = 10;

    break;

    /*
    case SPELL_SUMMON_NATURES_ALLY_1: //conjuration
      handle_corpse = FALSE;
      msg = 20;
      fmsg = rand_number(2, 6); // Random fail message
      mob_num = 9400 + rand_number(0, 7); // 9400-9407
      pfail = 0;
      break;

    case SPELL_SUMMON_NATURES_ALLY_2: // conjuration
      handle_corpse = FALSE;
      msg = 20;
      fmsg = rand_number(2, 6);
      mob_num = 9408 + rand_number(0, 6); // 9408-9414 for now
      pfail = 0;
      break;
       */

    // psionics

  case PSIONIC_ECTOPLASMIC_SHAMBLER:
    handle_corpse = FALSE;
    msg = 23;
    fmsg = rand_number(2, 6); /* Random fail message. */
    mob_num = MOB_ECTOPLASMIC_SHAMBLER;
    mob_level = (GET_PSIONIC_LEVEL(ch) > 20) ? ((GET_PSIONIC_LEVEL(ch) - 20) / 2) + 20 : GET_PSIONIC_LEVEL(ch);

    /* epic shambler */
    if (HAS_FEAT(ch, FEAT_EPIC_SHAMBLER))
      mob_level = GET_PSIONIC_LEVEL(ch) + 1;

    pfail = 0;
    break;

  default:
    return;
  }

  /* start off with some possible fail conditions */
  if (AFF_FLAGGED(ch, AFF_CHARM))
  {
    send_to_char(ch, "You are too giddy to have any followers!\r\n");
    return;
  }
  if (rand_number(0, 101) < pfail)
  {
    send_to_char(ch, "%s", mag_summon_fail_msgs[fmsg]);
    return;
  }

  /* new limit cap on certain mobiles */
  switch (spellnum)
  {
  case SPELL_SUMMON_NATURES_ALLY_9:
  case SPELL_SUMMON_NATURES_ALLY_8:
  case SPELL_SUMMON_NATURES_ALLY_7:
  case SPELL_SUMMON_CREATURE_9: // conjuration
  case SPELL_SUMMON_CREATURE_8: // conjuration
  case SPELL_SUMMON_CREATURE_7: // conjuration
  case SPELL_ELEMENTAL_SWARM:
    if (check_npc_followers(ch, NPC_MODE_FLAG, MOB_ELEMENTAL))
    {
      send_to_char(ch, "You can't control more elementals!\r\n");
      return;
    }
    break;
  case SPELL_ANIMATE_DEAD:
  case SPELL_GREATER_ANIMATION:
    if (check_npc_followers(ch, NPC_MODE_FLAG, MOB_ANIMATED_DEAD))
    {
      send_to_char(ch, "You can't control more undead!\r\n");
      return;
    }
    break;
  case SPELL_MUMMY_DUST:
    if (check_npc_followers(ch, NPC_MODE_FLAG, MOB_MUMMY_DUST))
    {
      send_to_char(ch, "You can't control more mummies via the mummy dust spell!\r\n");
      return;
    }
    break;
  case SPELL_DRAGON_KNIGHT:
    if (check_npc_followers(ch, NPC_MODE_FLAG, MOB_DRAGON_KNIGHT))
    {
      send_to_char(ch, "You can't control more dragons via the dragon knight spell!\r\n");
      return;
    }
    break;
  case VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT:
    if (check_npc_followers(ch, NPC_MODE_FLAG, MOB_C_O_T_N))
    {
      send_to_char(ch, "You can't control more vampiric minions!\r\n");
      return;
    }
    break;
  case ABILITY_CREATE_VAMPIRE_SPAWN:
    if (check_npc_followers(ch, NPC_MODE_FLAG, MOB_VAMP_SPWN))
    {
      send_to_char(ch, "You can't control more vampiric spawn!\r\n");
      return;
    }
    break;
  default:
    if (check_npc_followers(ch, NPC_MODE_SPARE, 0) <= 0)
    {
      send_to_char(ch, "You can't control more followers!\r\n");
      return;
    }
    break;
  }

  /* bring the mob into existence! */
  for (i = 0; i < num; i++)
  {
    if (!(mob = read_mobile(mob_num, VIRTUAL)))
    {
      send_to_char(ch, "You don't quite remember how to make that creature.\r\n");
      return;
    }
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
    {
      X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
      Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
    }
    char_to_room(mob, IN_ROOM(ch));
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);

    /* give the mobile some bonuses */
    /* ALSO special handling for clone spell */
    switch (spellnum)
    {
    case SPELL_SUMMON_NATURES_ALLY_9:
    case SPELL_SUMMON_NATURES_ALLY_8:
    case SPELL_SUMMON_NATURES_ALLY_7:
    case SPELL_BLADE_OF_DISASTER:
    case SPELL_SUMMON_CREATURE_9: // conjuration
    case SPELL_SUMMON_CREATURE_8: // conjuration
    case SPELL_SUMMON_CREATURE_7: // conjuration
    case SPELL_GREATER_ANIMATION: // necromancy
      /* (Zusuk) Temporary variable for capping elementals, etc */
      temp_level = MIN(CASTER_LEVEL(ch), mob_level);
      GET_LEVEL(mob) = MIN(LVL_IMMORT - 1, temp_level);
      autoroll_mob(mob, TRUE, TRUE);
      GET_LEVEL(mob) = temp_level;
      break;

    case VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT:
      GET_LEVEL(mob) = MAX(1, GET_LEVEL(ch) / 2);
      autoroll_mob(mob, TRUE, TRUE);
      break;

    case ABILITY_CREATE_VAMPIRE_SPAWN:
      GET_LEVEL(mob) = MIN(25, GET_LEVEL(ch));
      autoroll_mob(mob, TRUE, TRUE);
      set_vampire_spawn_feats(mob);
      if (obj->char_sdesc)
      {
        snprintf(desc, sizeof(desc), "vampire vamiric spawn %s", obj->char_sdesc);
        mob->player.name = strdup(desc);
        snprintf(desc, sizeof(desc), "the vampiric spawn of %s", obj->char_sdesc);
        mob->player.short_descr = strdup(desc);
        snprintf(desc, sizeof(desc), "The vampiric spawn of %s is here.\r\n", obj->char_sdesc);
        mob->player.long_descr = strdup(desc);
      }
      break;

    case SPELL_CLONE:
      /* Don't mess up the prototype; use new string copies. */
      mob->player.name = strdup(GET_NAME(ch));
      mob->player.short_descr = strdup(GET_NAME(ch));
      break;

    case PSIONIC_ECTOPLASMIC_SHAMBLER:
      GET_LEVEL(mob) = mob_level;

      autoroll_mob(mob, TRUE, TRUE);

      GET_HITROLL(mob) += 10; /* help them hit a bit */

      break;
    }

    // cedit configuration changes
    // We'll do this before adding other bonuses in order to not nerf those bonuses
    if (GET_LEVEL(mob) <= 10)
    {
      GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob) = GET_MAX_HIT(mob) * CONFIG_SUMMON_LEVEL_1_10_HP / 100;
      GET_REAL_AC(mob) = GET_REAL_AC(mob) * CONFIG_SUMMON_LEVEL_1_10_AC / 100;
      GET_HITROLL(mob) = GET_HITROLL(mob) * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      GET_DAMROLL(mob) = GET_DAMROLL(mob) * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      mob->mob_specials.damnodice = mob->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      mob->mob_specials.damsizedice = mob->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
    }
    else if (GET_LEVEL(mob) <= 20)
    {
      GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob) = GET_MAX_HIT(mob) * CONFIG_SUMMON_LEVEL_11_20_HP / 100;
      GET_REAL_AC(mob) = GET_REAL_AC(mob) * CONFIG_SUMMON_LEVEL_11_20_AC / 100;
      GET_HITROLL(mob) = GET_HITROLL(mob) * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      GET_DAMROLL(mob) = GET_DAMROLL(mob) * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      mob->mob_specials.damnodice = mob->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      mob->mob_specials.damsizedice = mob->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
    }
    else
    {
      GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob) = GET_MAX_HIT(mob) * CONFIG_SUMMON_LEVEL_21_30_HP / 100;
      GET_REAL_AC(mob) = GET_REAL_AC(mob) * CONFIG_SUMMON_LEVEL_21_30_AC / 100;
      GET_HITROLL(mob) = GET_HITROLL(mob) * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      GET_DAMROLL(mob) = GET_DAMROLL(mob) * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      mob->mob_specials.damnodice = mob->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      mob->mob_specials.damsizedice = mob->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
    }

    /* summon augmentation feat */
    if (HAS_FEAT(ch, FEAT_AUGMENT_SUMMONING))
    {
      send_to_char(ch, "*augmented* ");
      GET_REAL_STR(mob) = (mob)->aff_abils.str += 4;
      GET_REAL_CON(mob) = (mob)->aff_abils.con += 4;
      GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob) += 2 * GET_LEVEL(mob); /* con bonus */
    }

    int spell_focus_bonus = 0;

    switch (spellnum)
    {
    case SPELL_ANIMATE_DEAD:
    case SPELL_GREATER_ANIMATION:
    case SPELL_MUMMY_DUST:
    case VAMPIRE_ABILITY_CHILDREN_OF_THE_NIGHT:
    case ABILITY_CREATE_VAMPIRE_SPAWN:
      if (HAS_FEAT(ch, FEAT_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_SPELL_FOCUS), NECROMANCY))
        spell_focus_bonus++;
      if (HAS_FEAT(ch, FEAT_GREATER_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_GREATER_SPELL_FOCUS), NECROMANCY))
        spell_focus_bonus++;
      if (HAS_FEAT(ch, FEAT_EPIC_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_EPIC_SPELL_FOCUS), NECROMANCY))
        spell_focus_bonus++;
      GET_REAL_STR(mob) = (mob)->aff_abils.str += spell_focus_bonus * 2;
      GET_REAL_CON(mob) = (mob)->aff_abils.con += spell_focus_bonus * 2;
      GET_REAL_DEX(mob) = (mob)->aff_abils.dex += spell_focus_bonus * 2;
      GET_REAL_AC(mob) = (mob)->points.armor += (spell_focus_bonus * 2) * 10;
      GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob) += (spell_focus_bonus)*GET_LEVEL(mob); /* con bonus */
      GET_HIT(mob) = GET_MAX_HIT(mob);
      break;
    case SPELL_DRAGON_KNIGHT:
    case SPELL_ELEMENTAL_SWARM:
    case SPELL_SHAMBLER:
    case SPELL_SUMMON_CREATURE_1:
    case SPELL_SUMMON_CREATURE_2:
    case SPELL_SUMMON_CREATURE_3:
    case SPELL_SUMMON_CREATURE_4:
    case SPELL_SUMMON_CREATURE_5:
    case SPELL_SUMMON_CREATURE_6:
    case SPELL_SUMMON_CREATURE_7:
    case SPELL_SUMMON_CREATURE_8:
    case SPELL_SUMMON_CREATURE_9:
      if (HAS_FEAT(ch, FEAT_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_SPELL_FOCUS), CONJURATION))
        spell_focus_bonus++;
      if (HAS_FEAT(ch, FEAT_GREATER_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_GREATER_SPELL_FOCUS), CONJURATION))
        spell_focus_bonus++;
      if (HAS_FEAT(ch, FEAT_EPIC_SPELL_FOCUS) && HAS_SCHOOL_FEAT(ch, feat_to_sfeat(FEAT_EPIC_SPELL_FOCUS), CONJURATION))
        spell_focus_bonus++;
      GET_REAL_STR(mob) = (mob)->aff_abils.str += (spell_focus_bonus * 2);
      GET_REAL_CON(mob) = (mob)->aff_abils.con += (spell_focus_bonus * 2);
      GET_REAL_DEX(mob) = (mob)->aff_abils.dex += (spell_focus_bonus * 2);
      GET_REAL_AC(mob) = (mob)->points.armor += (spell_focus_bonus * 2) * 10;
      GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob) += ((spell_focus_bonus)*GET_LEVEL(mob)); /* con bonus */
      GET_HIT(mob) = GET_MAX_HIT(mob);
      break;
    }

    if (IS_SPECIALTY_SCHOOL(ch, spellnum))
    {
      send_to_char(ch, "*specialist* ");
      GET_REAL_STR(mob) = (mob)->aff_abils.str += CLASS_LEVEL(ch, CLASS_WIZARD) / 6 + 1;
      GET_REAL_CON(mob) = (mob)->aff_abils.con += CLASS_LEVEL(ch, CLASS_WIZARD) / 6 + 1;
      GET_REAL_DEX(mob) = (mob)->aff_abils.dex += CLASS_LEVEL(ch, CLASS_WIZARD) / 6 + 1;
      GET_REAL_AC(mob) = (mob)->points.armor += (CLASS_LEVEL(ch, CLASS_WIZARD) / 6 + 1) * 10;
      GET_REAL_MAX_HIT(mob) = GET_MAX_HIT(mob) += (CLASS_LEVEL(ch, CLASS_WIZARD) / 10 + 1) * GET_LEVEL(mob); /* con bonus */
      GET_HIT(mob) = GET_MAX_HIT(mob);
    }

    act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
    act(mag_summon_to_msgs[msg], FALSE, ch, 0, mob, TO_CHAR);
    load_mtrigger(mob);
    add_follower(mob, ch);
    if (GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
      join_group(mob, GROUP(ch));
  }

  /* raise dead type of spells */
  if (handle_corpse)
  {
    for (tobj = obj->contains; tobj; tobj = next_obj)
    {
      next_obj = tobj->next_content;
      obj_from_obj(tobj);
      obj_to_char(tobj, mob);
    }
    extract_obj(obj);
  }

  save_char_pets(ch);

  send_to_char(ch, "You can 'dismiss <creature-name>' if you are in the same room, or 'dismiss' with no argument to dismiss all your non-present summoned creatures.\r\n");
}
#undef OBJ_CLONE
#undef MOB_CLONE
#undef MOB_ZOMBIE
#undef MOB_GHOUL
#undef MOB_GIANT_SKELETON
#undef MOB_MUMMY
#undef MOB_RED_DRAGON
#undef MOB_SHELGARNS_BLADE
#undef MOB_DIRE_BADGER
#undef MOB_DIRE_BOAR
#undef MOB_DIRE_SPIDER
#undef MOB_DIRE_BEAR
#undef MOB_HOUND
#undef MOB_DIRE_TIGER
#undef MOB_FIRE_ELEMENTAL
#undef MOB_EARTH_ELEMENTAL
#undef MOB_AIR_ELEMENTAL
#undef MOB_WATER_ELEMENTAL
#undef MOB_ECTOPLASMIC_SHAMBLER

/*----------------------------------------------------------------------------*/
/* End Magic Summoning - Generic Routines and Local Globals */

/*----------------------------------------------------------------------------*/

/* plans to further comparmentalize and standardize healing mechanic start here
   in - ch is causing the heal, victim is receiving the heal, healing is the amount of HP restored, move is the amount of movement points restored
   out - TRUE if successful, FALSE if failed
   */
bool process_healing(struct char_data *ch, struct char_data *victim, int spellnum, int healing, int move, int psp)
{
  int start_hp = GET_HIT(victim);
  int start_mv = GET_MOVE(victim);
  int start_psp = GET_PSP(victim);

  /* black mantle reduces effectiveness of healing by 20% */
  if (AFF_FLAGGED(victim, AFF_BLACKMANTLE) || AFF_FLAGGED(ch, AFF_BLACKMANTLE))
    healing = healing - (healing / 5);

  /* healing domain */
  if (HAS_FEAT(ch, FEAT_EMPOWERED_HEALING))
    healing = (float)healing * 1.50;

  // vampire bonuses / penalties for feeding
  // vampire bonuses / penalties for feeding
  healing = healing * (10 + vampire_last_feeding_adjustment(ch)) / 10;
  move = move * (10 + vampire_last_feeding_adjustment(ch)) / 10;

  /* message to ch / victim */
  if (healing)
  {
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COMBATROLL))
      send_to_char(ch, "<%d> ", healing);
    if (ch != victim)
      if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_COMBATROLL))
        send_to_char(victim, "<%d> ", healing);
  }

  /* any special handling due to specific spellnum? */
  if (spellnum == -1)
  {
    /* restore HP now, some spells/effects can heal over your MAX hp */
    if (GET_HIT(victim) < (GET_MAX_HIT(victim) * 2))
      GET_HIT(victim) += healing;
  }
  else
  {
    /* any other special handling due to specific spellnum? */
    switch (spellnum)
    {

    /* restore HP now, some spells/effects can heal over your MAX hp */
    case RACIAL_LICH_TOUCH:
      if (GET_HIT(victim) < (GET_MAX_HIT(victim) * 2))
        GET_HIT(victim) += healing;
      break;

    default:
      /* generic healing */

      /* generic healing only takes you to the max */
      if (GET_HIT(victim) >= GET_MAX_HIT(victim))
        break; /* nothing to do here! */

      healing = MIN(healing, (GET_MAX_HIT(victim) - GET_HIT(victim)));

      GET_HIT(victim) += healing;

      /* all done! */
      break;
    }
  }

  /* this is for movement */
  GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);

  /* this is for PSP */
  GET_PSP(victim) = MIN(GET_MAX_PSP(victim), GET_PSP(victim) + psp);

  /* standard practice! */
  update_pos(victim);

  /* we improved our starting hp or moves */
  if (GET_HIT(victim) > start_hp ||
      GET_PSP(victim) > start_psp ||
      GET_MOVE(victim) > start_mv)
    return TRUE;

  return FALSE;
}

void mag_points(int level, struct char_data *ch, struct char_data *victim,
                struct obj_data *obj, int spellnum, int savetype, int casttype)
{
  int healing = 0, move = 0, psp = 0, max_psp = 0;
  const char *to_notvict = NULL, *to_char = NULL, *to_vict = NULL;

  if (victim == NULL)
    return;

  if (casttype == CAST_WEAPON_POISON || casttype == CAST_WEAPON_SPELL)
    ;
  else
    /* bards also get some healing spells */
    level = DIVINE_LEVEL(ch) + CLASS_LEVEL(ch, CLASS_BARD) + ALCHEMIST_LEVEL(ch);

  switch (spellnum)
  {
  case SPELL_VIGORIZE_LIGHT:
    move = dice(20, 4) + 50;

    to_notvict = "$N \twfeels a little more vigorized\tn.";
    if (ch == victim)
      to_char = "You \twfeel a little more vigorized\tn.";
    else
      to_char = "You \twvigorize light\tn on $N.";
    to_vict = "$n \twvigorizes you lightly.\tn";
    break;

  case SPELL_VIGORIZE_SERIOUS:
    move = dice(40, 4) + 150;

    to_notvict = "$N \twfeels seriously more vigorized\tn.";

    if (ch == victim)
      to_char = "You \twfeel seriously more vigorized\tn.";
    else
      to_char = "You \twvigorize serious\tn on $N.";

    to_vict = "$n \twvigorizes you seriously.\tn";

    break;

  case SPELL_VIGORIZE_CRITICAL:
    move = dice(60, 5) + 150 + (level * 10);

    to_notvict = "$N \twfeels critically more vigorized\tn.";
    if (ch == victim)
      to_char = "You \twfeel critically more vigorized\tn.";
    else
      to_char = "You \twvigorize critically\tn on $N.";
    to_vict = "$n \twvigorizes you critically.\tn";
    break;

  case SPELL_CURE_LIGHT:
    healing = dice(2, 4) + 5 + MIN(10, level);

    // to_notvict = "$n \twcures light wounds\tn on $N.";
    to_notvict = "$N \twfeels a little better\tn.";
    if (ch == victim)
    {
      // to_char = "You \twcure lights wounds\tn on yourself.";
      to_char = "You \twfeel a little better\tn.";
    }
    else
    {
      to_char = "You \twcure lights wounds\tn on $N.";
    }
    to_vict = "$n \twcures light wounds\tn on you.";
    break;
  case SPELL_CURE_MODERATE:
    healing = dice(3, 4) + 10 + MIN(15, level);

    to_notvict = "$n \twcures moderate wounds\tn on $N.";
    if (ch == victim)
      to_char = "You \twcure moderate wounds\tn on yourself.";
    else
      to_char = "You \twcure moderate wounds\tn on $N.";
    to_vict = "$n \twcures moderate wounds\tn on you.";
    break;
  case SPELL_CURE_SERIOUS:
    healing = dice(4, 4) + 15 + MIN(20, level);

    to_notvict = "$n \twcures serious wounds\tn on $N.";
    if (ch == victim)
      to_char = "You \twcure serious wounds\tn on yourself.";
    else
      to_char = "You \twcure serious wounds\tn on $N.";
    to_vict = "$n \twcures serious wounds\tn on you.";
    break;
  case SPELL_CURE_CRITIC:
    healing = dice(6, 4) + 20 + MIN(25, level);

    to_notvict = "$n \twcures critical wounds\tn on $N.";
    if (ch == victim)
      to_char = "You \twcure critical wounds\tn on yourself.";
    else
      to_char = "You \twcure critical wounds\tn on $N.";
    to_vict = "$n \twcures critical wounds\tn on you.";
    break;
  case SPELL_HEAL:
    healing = level * 10 + 20;

    to_notvict = "$n \tWheals\tn $N.";
    if (ch == victim)
      to_char = "You \tWheal\tn yourself.";
    else
      to_char = "You \tWheal\tn $N.";
    to_vict = "$n \tWheals\tn you.";
    break;
  case SPELL_HEAL_MOUNT:
    if (!is_paladin_mount(ch, victim))
    {
      send_to_char(ch, "You can only cast this upon your paladin mount.\r\n");
      return;
    }
    healing = level * 10 + 20;
    to_notvict = "$n \tWheals\tn $N.";
    to_char = "You \tWheal\tn $N.";
    to_vict = "$n \tWheals\tn you.";
    break;
  case SPELL_VAMPIRIC_TOUCH:
    victim = ch;
    healing = dice(MIN(15, CASTER_LEVEL(ch)), 4);

    to_notvict = "$N's wounds are \tWhealed\tn by \tRvampiric\tD magic\tn.";
    send_to_char(victim, "A \tWwarm feeling\tn floods your body as \tRvampiric "
                         "\tDmagic\tn takes over.\r\n");
    break;

  case ABILITY_CHANNEL_POSITIVE_ENERGY:
    healing = dice((compute_channel_energy_level(ch) + 1) / 2, 6);
    if (HAS_FEAT(ch, FEAT_HOLY_CHAMPION))
    {
      healing = ((compute_channel_energy_level(ch) + 1) / 2) * 7;
    }
    else if (HAS_FEAT(ch, FEAT_HOLY_WARRIOR))
    {
      healing += (compute_channel_energy_level(ch) + 1) / 2;
    }
    to_notvict = "$n \tWheals\tn $N.";
    if (ch == victim)
      to_char = "You \tWheal\tn yourself.";
    else
      to_char = "You \tWheal\tn $N.";
    to_vict = "$n \tWheals\tn you.";
    break;

  case ABILITY_CHANNEL_NEGATIVE_ENERGY:
    healing = dice((compute_channel_energy_level(ch) + 1) / 2, 6);
    if (HAS_FEAT(ch, FEAT_UNHOLY_CHAMPION))
    {
      healing = ((compute_channel_energy_level(ch) + 1) / 2) * 7;
    }
    else if (HAS_FEAT(ch, FEAT_UNHOLY_WARRIOR))
    {
      healing += (compute_channel_energy_level(ch) + 1) / 2;
    }
    to_notvict = "$n \tWheals\tn $N.";
    if (ch == victim)
      to_char = "You \tWheal\tn yourself.";
    else
      to_char = "You \tWheal\tn $N.";
    to_vict = "$n \tWheals\tn you.";
    break;
  case SPELL_REGENERATION:
    healing = dice(4, 4) + 15 + MIN(20, level);

    to_notvict = "$n \twcures some wounds\tn on $N.";
    if (ch == victim)
      to_char = "You \twcure some wounds\tn on yourself.";
    else
      to_char = "You \twcure some wounds\tn on $N.";
    to_vict = "$n \twcures some wounds\tn on you.";
    break;

  case PSIONIC_BODY_ADJUSTMENT:

    // we need to correct the psp cost below, because the power only benefits from 2 augment points at a time.

    healing = dice(1 + (GET_AUGMENT_PSP(ch) / 2), 12);
    to_notvict = "$n concentrates and some of $s wounds close.";
    to_char = "You concentrate and some of your wounds close.";

    break;
  case PSIONIC_BESTOW_POWER:
    if (IS_NPC(victim) || !IS_PSIONIC(victim))
    {
      send_to_char(ch, "You cannot use this power on someone without psionic ability.\r\n");
      return;
    }

    // we need to correct the psp cost below, because the power only benefits from 3 augment points at a time.

    max_psp = GET_PSIONIC_LEVEL(victim);
    if ((max_psp * 3) < (GET_AUGMENT_PSP(ch) + 3))
      GET_AUGMENT_PSP(ch) = MAX(0, (max_psp * 3) - 3);
    GET_PSP(ch) -= 3 + GET_AUGMENT_PSP(ch);
    GET_PSP(victim) = MIN(GET_MAX_PSP(victim), GET_PSP(victim) + 2 + ((GET_AUGMENT_PSP(ch) / 3) * 2));
    to_char = "You \twbestow psionic power\tn to $N.";
    to_vict = "$n \twbestows psionic power\tn to you.";
    return;
  }

  if (affected_by_spell(victim, BOMB_AFFECT_BONESHARD))
  {
    affect_from_char(victim, BOMB_AFFECT_BONESHARD);
    if (ch == victim)
    {
      act("The bone shards in your flesh dissolve and your bleeding stops.", FALSE, ch, 0, victim, TO_CHAR);
    }
    else
    {
      act("The bone shards in your flesh dissolve and your bleeding stops.", FALSE, ch, 0, victim, TO_VICT);
      act("The bone shards in $N's flesh dissolve and $S bleeding stops.", FALSE, ch, 0, victim, TO_ROOM);
    }
  }

  if (to_notvict != NULL)
    act(to_notvict, TRUE, ch, 0, victim, TO_NOTVICT);
  if (to_vict != NULL && ch != victim)
    act(to_vict, TRUE, ch, 0, victim, TO_VICT | TO_SLEEP);
  if (to_char != NULL)
    act(to_char, TRUE, ch, 0, victim, TO_CHAR);

  /* newer centralized function for points (modifying healing, move and psp in one place) */
  process_healing(ch, victim, spellnum, healing, move, psp);
}

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
                   struct obj_data *obj, int spellnum, int type, int casttype)
{
  int spell = 0, msg_not_affected = TRUE, affect = 0, affect2 = 0, found = FALSE;
  const char *to_vict = NULL, *to_char = NULL, *to_notvict = NULL;
  int i = 0;
  struct obj_data *eq = NULL;
  char message[200];

  struct affected_type *af = NULL, *next = NULL;

  if (victim == NULL)
    return;

  switch (spellnum)
  {
  case SPELL_HEAL:
    /* Heal also restores health, so don't give the "no effect" message if the
     * target isn't afflicted by the 'blindness' spell. */
    msg_not_affected = FALSE;
    /* fall-through */
  case SPELL_CURE_BLIND:
    /* this has fall-through from above */
    spell = SPELL_BLINDNESS;
    affect = AFF_BLIND;
    to_char = "You restore $N's vision.";
    to_vict = "$n restores your vision!";
    to_notvict = "There's a momentary gleam in $N's eyes.";
    break;

  case SPELL_REMOVE_PARALYSIS:
    /* this has fall-through from above */
    spell = SPELL_HOLD_PERSON;
    affect = AFF_PARALYZED;
    to_char = "You restore $N's movement.";
    to_vict = "$n restores your movement!";
    to_notvict = "$N is able to move again.";
    break;

  case SPELL_REMOVE_POISON:
    spell = SPELL_POISON;
    affect = AFF_POISON;
    to_char = "You remove the poison from $N's body.";
    to_vict = "A warm feeling originating from $n runs through your body!";
    to_notvict = "$N looks better.";
    break;

  case SPELL_REMOVE_CURSE:
    spell = SPELL_CURSE;
    affect = AFF_CURSE;
    to_char = "You remove the curse from $N.";
    to_vict = "$n removes the curse upon you.";
    to_notvict = "$N briefly glows blue.";
    for (i = 0; i < NUM_WEARS; i++)
    {
      eq = GET_EQ(ch, i);
      if (eq && OBJ_FLAGGED(eq, ITEM_NODROP))
      {
        REMOVE_BIT_AR(GET_OBJ_EXTRA(eq), ITEM_NODROP);
        if (GET_OBJ_TYPE(eq) == ITEM_WEAPON)
          GET_OBJ_VAL(eq, 2)
        ++;
        to_char = "$p briefly glows blue.";
      }
    }
    for (eq = ch->carrying; eq; eq = eq->next_content)
    {
      if (eq && OBJ_FLAGGED(eq, ITEM_NODROP))
      {
        REMOVE_BIT_AR(GET_OBJ_EXTRA(eq), ITEM_NODROP);
        if (GET_OBJ_TYPE(eq) == ITEM_WEAPON)
          GET_OBJ_VAL(eq, 2)
        ++;
        to_char = "$p briefly glows blue.";
      }
    }
    break;

  case SPELL_DISPEL_INVIS:
    spell = SPELL_INVISIBLE;
    affect = AFF_INVISIBLE;
    to_char = "You remove the invisibility from $N.";
    to_vict = "$n removes the invisibility upon you.";
    to_notvict = "$N slowly fades into appearance.";
    for (eq = ch->carrying; eq; eq = eq->next_content)
    {
      if (eq && OBJ_FLAGGED(eq, ITEM_INVISIBLE))
      {
        REMOVE_BIT_AR(GET_OBJ_EXTRA(eq), ITEM_INVISIBLE);
        to_char = "$p slowly fades into existence.";
      }
    }
    break;

  case SPELL_REMOVE_DISEASE:
    spell = SPELL_EYEBITE;
    affect = AFF_DISEASE;
    to_char = "You remove the disease from $N.";
    to_vict = "$n removes the disease inflicting you.";
    to_notvict = "$N briefly flushes red then no longer looks diseased.";
    break;

  case SPELL_INVISIBILITY_PURGE:
    spell = SPELL_INVISIBLE;
    affect = AFF_INVISIBLE;
    to_char = "$N is no longer invisible.";
    to_vict = "You are no longer invisible.";
    to_notvict = "$N is no longer invisible.";
    appear(victim, TRUE);
    break;

  case SPELL_REMOVE_FEAR:
    spell = SPELL_SCARE;
    affect = AFF_FEAR;
    affect2 = AFF_SHAKEN;
    to_char = "You remove the fear from $N.";
    to_vict = "$n removes the fear upon you.";
    to_notvict = "$N looks brave again.";
    break;

  case SPELL_BRAVERY:
    spell = SPELL_SCARE;
    affect = AFF_FEAR;
    affect2 = AFF_SHAKEN;
    to_char = "You remove the fear from $N.";
    to_vict = "$n removes the fear upon you.";
    to_notvict = "$N looks brave again.";
    break;

  case SPELL_CURE_DEAFNESS:
    spell = SPELL_DEAFNESS;
    affect = AFF_DEAF;
    to_char = "You remove the deafness from $N.";
    to_vict = "$n removes the deafness from you.";
    to_notvict = "$N looks like $E can hear again.";
    break;

  case SPELL_FREE_MOVEMENT:
    spell = SPELL_WEB;
    affect = AFF_ENTANGLED;
    to_char = "You remove the web from $N.";
    to_vict = "$n removes the web from you.";
    to_notvict = "$N looks like $E can move again.";
    break;

  case SPELL_FAERIE_FOG:
    spell = SPELL_INVISIBLE;
    affect = AFF_INVISIBLE;
    /* a message isn't appropriate for failure here */
    msg_not_affected = FALSE;

    to_char = "Your fog reveals $N.";
    to_vict = "$n reveals you with faerie fog.";
    to_notvict = "$N is revealed by $n's faerie fog.";
    break;

  case PSIONIC_ERADICATE_INVISIBILITY:
    GET_DC_BONUS(ch) += GET_AUGMENT_PSP(ch) / 2;
    if (mag_savingthrow(ch, victim, SAVING_REFL, 0, CAST_SPELL, level, NOSCHOOL))
      return;
    affect = AFF_INVISIBLE;
    to_char = "Your psychic manifestation reveals $N.";
    to_vict = "$n reveals you with $s psychic manifestation.";
    to_notvict = "$N is revealed by $n's psychic manifestation.";
    break;

  case PSIONIC_SHATTER_MIND_BLANK:
    if (mag_savingthrow(ch, victim, SAVING_WILL, 0, CAST_SPELL, level, NOSCHOOL))
      return;
    affect = AFF_MIND_BLANK;
    to_char = "You shatter $N's mind blank!";
    to_vict = "$n shatters your mind blank!";
    break;

  case SPELL_LESSER_RESTORATION:
  case SPELL_RESTORATION:
    // we handle this differently below.
    break;

  default:
    log("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
    return;
  }

  // lesser restoration and restoration will remove any spell/ability that
  // affects one of the main 6 ability scores.  It will not affect anything
  // that blinds, poisons, curses, or stuns, or anything listed in the
  // is_spell_restorable function. Lesser restoration will break the loop
  // after completing a single affect removal, whereas restoration will
  // remove all qualifying affects.
  if (spellnum == SPELL_LESSER_RESTORATION || spellnum == SPELL_RESTORATION)
  {
    for (af = victim->affected; af; af = af->next)
    {
      if (af->location == APPLY_STR || af->location == APPLY_CON || af->location == APPLY_DEX ||
          af->location == APPLY_INT || af->location == APPLY_WIS || af->location == APPLY_CHA)
      {
        if ((IS_SET_AR(af->bitvector, AFF_POISON) || IS_SET_AR(af->bitvector, AFF_CURSE) ||
             IS_SET_AR(af->bitvector, AFF_BLIND) || IS_SET_AR(af->bitvector, AFF_STUN) ||
             IS_SET_AR(af->bitvector, AFF_DISEASE)) &&
            spellnum == SPELL_LESSER_RESTORATION)
          continue;
        if (!is_spell_restoreable(spellnum))
          continue;
        if (spellnum == AFFECT_LEVEL_DRAIN && get_char_affect_modifier(ch, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL) > 1)
        {
          change_spell_mod(victim, AFFECT_LEVEL_DRAIN, APPLY_SPECIAL, -1, TRUE);
          if (ch == victim)
          {
            snprintf(message, sizeof(message), "You reduce the degree thast '%s' is affecting you.", spell_info[spellnum].name);
            act(message, FALSE, ch, 0, 0, TO_CHAR);
          }
          else
          {
            snprintf(message, sizeof(message), "You reduce the degree thast '%s' is affecting $N.", spell_info[spellnum].name);
            act(message, FALSE, ch, 0, victim, TO_CHAR);
            snprintf(message, sizeof(message), "$n reduces the degree thast '%s' is affecting you.", spell_info[spellnum].name);
            act(message, FALSE, ch, 0, victim, TO_VICT);
          }
        }
        else
        {
          if (ch == victim)
          {
            snprintf(message, sizeof(message), "You remove the '%s' affecting you.", spell_info[spellnum].name);
            act(message, FALSE, ch, 0, 0, TO_CHAR);
          }
          else
          {
            snprintf(message, sizeof(message), "You remove the '%s' affecting $N.", spell_info[spellnum].name);
            act(message, FALSE, ch, 0, victim, TO_CHAR);
            snprintf(message, sizeof(message), "$n removes the '%s' affecting you.", spell_info[spellnum].name);
            act(message, FALSE, ch, 0, victim, TO_VICT);
          }
          affect_from_char(victim, spellnum);
        }
        // lesser restoration will only cure one affect
        if (spellnum == SPELL_LESSER_RESTORATION)
          break;
      }
    }
    return;
  }

  /* this is to try and clean up bits related to the spell */
  for (af = victim->affected; af; af = next)
  {
    next = af->next;

    if (af && affect && af->bitvector && IS_SET_AR(af->bitvector, affect))
    {
      if (victim && af->spell)
      {
        affect_from_char(victim, af->spell);
        found = TRUE;
        continue;
      }
    }
    if (af && affect2 && af->bitvector && IS_SET_AR(af->bitvector, affect2))
    {
      if (victim && af->spell)
      {
        affect_from_char(victim, af->spell);
        found = TRUE;
        continue;
      }
    }
  }

  if (!found && !affected_by_spell(victim, spell) && !AFF_FLAGGED(victim, affect))
  {
    if (msg_not_affected)
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
    return;
  }
  /* end bit clean up */

  /* first remove spell affect */
  affect_from_char(victim, spell);

  /* special scenario:  dg-script affliction */
  // affect_type_from_char(victim, affect);
  if (affected_by_spell(victim, SPELL_DG_AFFECT) && AFF_FLAGGED(victim, affect))
  {
    /* have to make sure this particular dg-affect is corresponding to the right AFF_ flag */
    affect_from_char(victim, SPELL_DG_AFFECT);
  }

  /* then remove affect flag if it somehow is still around */
  if (AFF_FLAGGED(victim, affect))
    REMOVE_BIT_AR(AFF_FLAGS(victim), affect);

  /* send messages */
  if (to_notvict != NULL)
    act(to_notvict, TRUE, ch, 0, victim, TO_NOTVICT);
  if (to_vict != NULL)
    act(to_vict, TRUE, ch, 0, victim, TO_VICT | TO_SLEEP);
  if (to_char != NULL)
    act(to_char, TRUE, ch, 0, victim, TO_CHAR);
}

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
                    int spellnum, int savetype, int casttype)
{
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  switch (spellnum)
  {
  case SPELL_BLESS:
    if (!OBJ_FLAGGED(obj, ITEM_BLESS) &&
        (GET_OBJ_WEIGHT(obj) <= 5 * DIVINE_LEVEL(ch)))
    {
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BLESS);
      to_char = "$p glows briefly.";
    }
    break;
  case SPELL_CURSE:
    if (!OBJ_FLAGGED(obj, ITEM_NODROP))
    {
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
      if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        GET_OBJ_VAL(obj, 2)
      --;
      to_char = "$p briefly glows red.";
    }
    break;
  case SPELL_INVISIBLE:
    if (!OBJ_FLAGGED(obj, ITEM_NOINVIS) && !OBJ_FLAGGED(obj, ITEM_INVISIBLE))
    {
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
      to_char = "$p vanishes.";
    }
    break;
  case SPELL_POISON:
    if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) &&
        !GET_OBJ_VAL(obj, 3))
    {
      GET_OBJ_VAL(obj, 3) = 1;
      to_char = "$p steams briefly.";
    }
    break;
  case SPELL_REMOVE_CURSE:
    if (OBJ_FLAGGED(obj, ITEM_NODROP))
    {
      REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
      if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        GET_OBJ_VAL(obj, 2)
      ++;
      to_char = "$p briefly glows blue.";
    }
    break;
  case SPELL_DISPEL_INVIS:
    if (OBJ_FLAGGED(obj, ITEM_INVISIBLE))
    {
      REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
      to_char = "$p slowly fades into existence.";
    }
    break;
  case SPELL_REMOVE_POISON:
    if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) &&
        GET_OBJ_VAL(obj, 3))
    {
      GET_OBJ_VAL(obj, 3) = 0;
      to_char = "$p steams briefly.";
    }
    break;
  }

  if (to_char == NULL)
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
  else
    act(to_char, TRUE, ch, obj, 0, TO_CHAR);

  if (to_room != NULL)
    act(to_room, TRUE, ch, obj, 0, TO_ROOM);
  else if (to_char != NULL)
    act(to_char, TRUE, ch, obj, 0, TO_ROOM);
}

#define LOOP_LIMIT_MAGCREATE 1000
/* this function will hand spells that create objects */
void mag_creations(int level, struct char_data *ch, struct char_data *vict,
                   struct obj_data *obj, int spellnum, int casttype)
{
  struct obj_data *tobj = NULL, *portal = NULL;
  obj_vnum object_vnum = 0;
  const char *to_char = NULL, *to_room = NULL;
  bool obj_to_floor = FALSE;
  bool portal_process = FALSE;
  bool gate_process = FALSE;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  room_rnum gate_dest = NOWHERE;
  char buf[MEDIUM_STRING] = {'\0'};
  int loop_count = 0;

  if (ch == NULL)
    return;

  switch (spellnum)
  {
  case SPELL_CREATE_FOOD:
    to_char = "You create $p.";
    to_room = "$n creates $p.";
    object_vnum = 10;
    break;
  case SPELL_CONTINUAL_FLAME:
    to_char = "You create $p.";
    to_room = "$n creates $p.";
    object_vnum = 222;
    break;
  case SPELL_FIRE_SEEDS:
    to_char = "You create $p.";
    to_room = "$n creates $p.";
    send_to_char(ch, "Drop <item name> to start grenade, it will explode in 3 seconds.\r\n");
    if (rand_number(0, 1))
      object_vnum = 9404;
    else
      object_vnum = 9405;
    break;
  case SPELL_GATE:
  case PSIONIC_PLANAR_TRAVEL:
    to_char = "\tnYou fold \tMtime\tn and \tDspace\tn, and create $p\tn.";
    to_room = "$n \tnfolds \tMtime\tn and \tDspace\tn, and creates $p\tn.";
    obj_to_floor = TRUE;
    object_vnum = 802;
    /* a little more work with gates */
    gate_process = TRUE;

    /* where is it going? */
    one_argument(cast_arg2, arg, sizeof(arg));

    if (!valid_mortal_tele_dest(ch, IN_ROOM(ch), TRUE))
    {
      send_to_char(ch, "A bright flash prevents your %s from working!", spellnum == SPELL_GATE ? "spell" : "manifestation");
      return;
    }

    /* astral */
    if (is_abbrev(arg, "astral"))
    {

      if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE))
      {
        send_to_char(ch, "You are already on the astral plane!\r\n");
        return;
      }

      do
      {
        /* destination! */
        gate_dest = rand_number(0, top_of_world);
        loop_count++;

      } while (!ZONE_FLAGGED(GET_ROOM_ZONE(gate_dest), ZONE_ASTRAL_PLANE) ||
               !valid_mortal_tele_dest(ch, gate_dest, FALSE) || loop_count < LOOP_LIMIT_MAGCREATE);
    }

    /* ethereal */
    else if (is_abbrev(arg, "ethereal"))
    {

      if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE))
      {
        send_to_char(ch, "You are already on the ethereal plane!\r\n");
        return;
      }

      do
      {
        /* destination! */
        gate_dest = rand_number(0, top_of_world);
        loop_count++;

      } while (!ZONE_FLAGGED(GET_ROOM_ZONE(gate_dest), ZONE_ETH_PLANE) ||
               !valid_mortal_tele_dest(ch, gate_dest, FALSE) || loop_count < LOOP_LIMIT_MAGCREATE);
    }

    /* elemental */
    else if (is_abbrev(arg, "elemental"))
    {

      if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL))
      {
        send_to_char(ch, "You are already on the elemental plane!\r\n");
        return;
      }

      do
      {
        /* destination! */
        gate_dest = rand_number(0, top_of_world);
        loop_count++;

      } while (!ZONE_FLAGGED(GET_ROOM_ZONE(gate_dest), ZONE_ELEMENTAL) ||
               !valid_mortal_tele_dest(ch, gate_dest, FALSE) || loop_count < LOOP_LIMIT_MAGCREATE);
    }

    /* prime */
    else if (is_abbrev(arg, "prime"))
    {

      if (!ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE) &&
          !ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE) &&
          !ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL))
      {
        send_to_char(ch,
                     "You need to be off the prime plane to gate to it!\r\n");
        return;
      }

      do
      {
        /* destination! */
        gate_dest = rand_number(0, top_of_world);
        loop_count++;

      } while ((ZONE_FLAGGED(GET_ROOM_ZONE(gate_dest), ZONE_ELEMENTAL) ||
                ZONE_FLAGGED(GET_ROOM_ZONE(gate_dest), ZONE_ETH_PLANE) ||
                ZONE_FLAGGED(GET_ROOM_ZONE(gate_dest), ZONE_ASTRAL_PLANE)) ||
               !valid_mortal_tele_dest(ch, gate_dest, FALSE) || loop_count < LOOP_LIMIT_MAGCREATE);
    }

    /* failed! */
    else
    {
      send_to_char(ch, "Not a valid target (astral, ethereal, elemental, prime)");
      return;
    }

    if (!valid_mortal_tele_dest(ch, gate_dest, FALSE))
    {
      send_to_char(ch, "Your %s is being blocked at the destination!\r\n", spellnum == SPELL_GATE ? "magic" : "power");
      return;
    }

    break;
  case SPELL_GOODBERRY:
    to_char = "You create $p.";
    to_room = "$n creates $p.";
    object_vnum = 9400;
    break;
  case SPELL_HOLY_SWORD:
    to_char = "You summon $p.";
    to_room = "$n summons $p.";
    object_vnum = 810;
    break;
  case SPELL_UNHOLY_SWORD:
    to_char = "You summon $p.";
    to_room = "$n summons $p.";
    object_vnum = 897;
    break;
  case SPELL_MAGIC_STONE:
    to_char = "You create $p.";
    to_room = "$n creates $p.";
    object_vnum = 9401;
    break;
  case SPELL_PORTAL:

    if (vict == NULL)
    {
      send_to_char(ch, "Portal failed!  You have no target!\r\n");
      return;
    }

    if (IS_POWERFUL_BEING(vict))
    {
      send_to_char(ch, "Portal failed!  The target is a powerful being and easily dismises the portal from the other side!\r\n");
      return;
    }

    if (IN_ROOM(ch) == NOWHERE)
    {
      send_to_char(ch, "Portal failed!  You are NOWHERE!  (report to imm bug magic8274)\r\n");
      return;
    }

    if (IN_ROOM(vict) == NOWHERE)
    {
      send_to_char(ch, "Portal failed!  The target seems to no longer be valid...\r\n");
      return;
    }

    if (AFF_FLAGGED(vict, AFF_NOTELEPORT))
    {
      send_to_char(ch, "The portal begins to open, then shuts suddenly!\r\n");
      return;
    }

    if (MOB_FLAGGED(vict, MOB_NOSUMMON))
    {
      send_to_char(ch, "The portal while beginning to form, flashes brightly, then shuts suddenly!\r\n");
      return;
    }

    if (!valid_mortal_tele_dest(ch, IN_ROOM(ch), FALSE) ||
        !valid_mortal_tele_dest(ch, IN_ROOM(vict), FALSE))
    {
      send_to_char(ch, "Your portal is not working!  Must be this location or the target is blocking the portal!\r\n");
      return;
    }

    /* no portaling on the outter planes */
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL) ||
        ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE) ||
        ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE))
    {
      send_to_char(ch, "This magic won't help you travel on this plane!\r\n");
      return;
    }

    /* no portaling off the prime plane to another */
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(vict)), ZONE_ELEMENTAL) ||
        ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(vict)), ZONE_ETH_PLANE) ||
        ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(vict)), ZONE_ASTRAL_PLANE))
    {
      send_to_char(ch, "Your target is beyond the reach of your magic!\r\n");
      return;
    }

    to_char = "\tnYou fold \tMtime\tn and \tDspace\tn, and create $p\tn.";
    to_room = "$n \tnfolds \tMtime\tn and \tDspace\tn, and creates $p\tn.";
    obj_to_floor = TRUE;
    object_vnum = 801;
    /* a little more work with portals */
    portal_process = TRUE;
    break;
  case SPELL_SPRING_OF_LIFE:
    to_char = "You create $p.";
    to_room = "$n creates $p.";
    obj_to_floor = TRUE;
    object_vnum = 805;
    break;
    /* these have been made manual spells */
    /*
      case SPELL_WALL_OF_FIRE:
        to_char = "You create $p.";
        to_room = "$n creates $p.";
        obj_to_floor = TRUE;
        object_vnum = 9402;
        break;
      case SPELL_WALL_OF_THORNS:
        to_char = "You create $p.";
        to_room = "$n creates $p.";
        obj_to_floor = TRUE;
        object_vnum = 9403;
        break;
       */
  default:
    send_to_char(ch, "Spell unimplemented, it would seem.\r\n");
    return;
  }

  if (!(tobj = read_object(object_vnum, VIRTUAL)))
  {
    send_to_char(ch, "I seem to have goofed.\r\n");
    log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
        spellnum, object_vnum);
    return;
  }

  /* a little more work for portal object */
  /* the obj (801) should already bet set right, but just in case */
  if (portal_process)
  {
    if (!(portal = read_object(object_vnum, VIRTUAL)))
    {
      send_to_char(ch, "I seem to have goofed.\r\n");
      log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
          spellnum, object_vnum);
      return;
    }

    /* make sure its a portal **/
    GET_OBJ_TYPE(tobj) = ITEM_PORTAL;
    GET_OBJ_TYPE(portal) = ITEM_PORTAL;
    /* set it to a tick duration */
    GET_OBJ_TIMER(tobj) = 2;
    GET_OBJ_TIMER(portal) = 2;
    /* set it to a normal portal */
    tobj->obj_flags.value[0] = PORTAL_NORMAL;
    portal->obj_flags.value[0] = PORTAL_NORMAL;
    /* set destination to vict */
    tobj->obj_flags.value[1] = GET_ROOM_VNUM(IN_ROOM(vict));
    portal->obj_flags.value[1] = GET_ROOM_VNUM(IN_ROOM(ch));
    /* make sure it decays */
    if (!OBJ_FLAGGED(tobj, ITEM_DECAY))
      TOGGLE_BIT_AR(GET_OBJ_EXTRA(tobj), ITEM_DECAY);
    if (!OBJ_FLAGGED(portal, ITEM_DECAY))
      TOGGLE_BIT_AR(GET_OBJ_EXTRA(portal), ITEM_DECAY);

    /* make sure the portal is two-sided */
    obj_to_room(portal, IN_ROOM(vict));

    /* make sure the victim room sees the message */
    act("With a \tBflash\tn, $p appears in the room.",
        FALSE, vict, portal, 0, TO_CHAR);
    act("With a \tBflash\tn, $p appears in the room.",
        FALSE, vict, portal, 0, TO_ROOM);
  }
  else if (gate_process)
  {
    if (!(portal = read_object(object_vnum, VIRTUAL)))
    {
      send_to_char(ch, "I seem to have goofed.\r\n");
      log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
          spellnum, object_vnum);
      return;
    }

    if (gate_dest == NOWHERE)
    {
      send_to_char(ch, "The spell failed!\r\n");
      return;
    }

    if (!valid_mortal_tele_dest(ch, gate_dest, TRUE))
    {
      send_to_char(ch, "The spell fails!\r\n");
      return;
    }

    /* make sure its a portal **/
    GET_OBJ_TYPE(tobj) = ITEM_PORTAL;
    GET_OBJ_TYPE(portal) = ITEM_PORTAL;
    /* set it to a tick duration */
    GET_OBJ_TIMER(tobj) = 2;
    GET_OBJ_TIMER(portal) = 2;
    /* set it to a normal portal */
    tobj->obj_flags.value[0] = PORTAL_NORMAL;
    portal->obj_flags.value[0] = PORTAL_NORMAL;
    /* set destination to plane */
    tobj->obj_flags.value[1] = GET_ROOM_VNUM(gate_dest);
    portal->obj_flags.value[1] = GET_ROOM_VNUM(IN_ROOM(ch));
    /* make sure it decays */
    if (!OBJ_FLAGGED(tobj, ITEM_DECAY))
      TOGGLE_BIT_AR(GET_OBJ_EXTRA(tobj), ITEM_DECAY);
    if (!OBJ_FLAGGED(portal, ITEM_DECAY))
      TOGGLE_BIT_AR(GET_OBJ_EXTRA(portal), ITEM_DECAY);

    /* make sure the portal is two-sided */
    obj_to_room(portal, gate_dest);

    /* make sure the victim room sees the message */
    act("With a \tBflash\tn, $p appears in the room.",
        FALSE, vict, portal, 0, TO_CHAR);
    act("With a \tBflash\tn, $p appears in the room.",
        FALSE, vict, portal, 0, TO_ROOM);
  }
  else
  {
    /* a little convenient idea, item should match char size */
    GET_OBJ_SIZE(tobj) = GET_SIZE(ch);
    if (spellnum == SPELL_HOLY_SWORD)
    {
      send_to_char(ch, "You can change your holy weapon type with the 'holyweapon' command. The default type is long sword.\r\n");
      snprintf(buf, sizeof(buf), "holy avenger gloriously shining %s", weapon_list[GET_HOLY_WEAPON_TYPE(ch)].name);
      tobj->name = strdup(buf);
      snprintf(buf, sizeof(buf), "A gloriously shining %s hovers above the ground, pointed skyward.", weapon_list[GET_HOLY_WEAPON_TYPE(ch)].name);
      tobj->description = strdup(buf);
      GET_OBJ_VAL(tobj, 0) = GET_HOLY_WEAPON_TYPE(ch);
      GET_OBJ_VAL(tobj, 1) = weapon_list[GET_HOLY_WEAPON_TYPE(ch)].numDice;
      GET_OBJ_VAL(tobj, 2) = weapon_list[GET_HOLY_WEAPON_TYPE(ch)].diceSize;
      GET_OBJ_SIZE(tobj) = weapon_list[GET_HOLY_WEAPON_TYPE(ch)].size;
    }
    if (spellnum == SPELL_UNHOLY_SWORD)
    {
      send_to_char(ch, "You can change your unholy weapon type with the 'unholyweapon' command. The default type is long sword.\r\n");
      snprintf(buf, sizeof(buf), "unholy avenger red shadow %s", weapon_list[GET_HOLY_WEAPON_TYPE(ch)].name);
      tobj->name = strdup(buf);
      snprintf(buf, sizeof(buf), "A baleful-looking %s hovers above the ground, pointed skyward.", weapon_list[GET_HOLY_WEAPON_TYPE(ch)].name);
      tobj->description = strdup(buf);
      GET_OBJ_VAL(tobj, 0) = GET_HOLY_WEAPON_TYPE(ch);
      GET_OBJ_VAL(tobj, 1) = weapon_list[GET_HOLY_WEAPON_TYPE(ch)].numDice;
      GET_OBJ_VAL(tobj, 2) = weapon_list[GET_HOLY_WEAPON_TYPE(ch)].diceSize;
      GET_OBJ_SIZE(tobj) = weapon_list[GET_HOLY_WEAPON_TYPE(ch)].size;
    }
  }

  if (obj_to_floor)
    obj_to_room(tobj, IN_ROOM(ch));
  else
    obj_to_char(tobj, ch);
  act(to_char, FALSE, ch, tobj, 0, TO_CHAR);
  act(to_room, FALSE, ch, tobj, 0, TO_ROOM);
  load_otrigger(tobj);
}
#undef LOOP_LIMIT_MAGCREATE

/* so this function is becoming a beast, we have to support both
   room-affections AND room-events now */
void mag_room(int level, struct char_data *ch, struct obj_data *obj,
              int spellnum, int casttype)
{
  long aff = -1;  /* what affection, -1 means it must be an event */
  int rounds = 0; /* how many rounds this spell lasts (duration) */
  const char *to_char = NULL;
  const char *to_room = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct raff_node *raff = NULL;
  extern struct raff_node *raff_list;
  room_rnum rnum = NOWHERE;
  bool failure = FALSE;
  event_id IdNum = -1; /* which event? -1 means it must be an affection */

  if (ch == NULL)
    return;

  rnum = IN_ROOM(ch);

  if (ROOM_FLAGGED(rnum, ROOM_NOMAGIC))
    failure = TRUE;

  level = MAX(MIN(level, LVL_IMPL), 1);

  switch (spellnum)
  {

    /*******  ROOM EVENTS     ************/
  case SPELL_I_DARKNESS:
    IdNum = eDARKNESS;
    if (ROOM_FLAGGED(rnum, ROOM_DARK))
      failure = TRUE;

    rounds = 10;
    SET_BIT_AR(ROOM_FLAGS(rnum), ROOM_DARK);

    to_char = "You cast a shroud of darkness upon the area.";
    to_room = "$n casts a shroud of darkness upon this area.";
    break;
    /*******  END ROOM EVENTS ************/

    /*******  ROOM AFFECTIONS ************/
  case SPELL_ACID_FOG: // conjuration
    to_char = "You create a thick bank of acid fog!";
    to_room = "$n creates a thick bank of acid fog!";
    aff = RAFF_ACID_FOG;
    rounds = MAX(4, MAGIC_LEVEL(ch));
    break;

  case SPELL_ANTI_MAGIC_FIELD: // illusion
    to_char = "You create an anti-magic field!";
    to_room = "$n creates an anti-magic field!";
    aff = RAFF_ANTI_MAGIC;
    rounds = 15;
    break;

  case SPELL_BILLOWING_CLOUD: // conjuration
    to_char = "Clouds of billowing thickness fill the area.";
    to_room = "$n creates clouds of billowing thickness that fill the area.";
    aff = RAFF_BILLOWING;
    rounds = 15;
    break;

  case SPELL_BLADE_BARRIER: // divine spell
    to_char = "You create a barrier of spinning blades!";
    to_room = "$n creates a barrier of spinning blades!";
    aff = RAFF_BLADE_BARRIER;
    rounds = MAX(4, DIVINE_LEVEL(ch));
    break;

  case PSIONIC_UPHEAVAL:
    to_char = "Your psionic upheaval makes the ground jagged and uneven.";
    to_room = "$n causes the ground to become jagged and uneven!";
    aff = RAFF_DIFFICULT_TERRAIN;
    rounds = GET_PSIONIC_LEVEL(ch);
    break;

  case SPELL_OBSCURING_MIST: // conjuration
    /* so right now this spell is simply 20% concealment to everyone in room, needs
     * I also think it needs some other affects
     * also, gust of wind, fireball, flamestrike, etc. will disperse the mist when cast,
     * or even a strong wind in the weather...
     */
    if (SECT(ch->in_room) == SECT_UNDERWATER)
    {
      send_to_char(ch, "The obscuring mist quickly disappears under the water.\r\n");
      return;
    }
    aff = RAFF_OBSCURING_MIST;
    rounds = MAX(4, DIVINE_LEVEL(ch));
    to_char = "You create an obscuring mist that fills the room!";
    to_room = "An obscuring mist suddenly fills the room from $n!";
    break;

  case SPELL_DARKNESS: // divination
    to_char = "You create a blanket of pitch black.";
    to_room = "$n creates a blanket of pitch black.";
    aff = RAFF_DARKNESS;
    rounds = 15;
    break;

  case SPELL_SACRED_SPACE: // divination
    to_char = "You create an aura of sacredness in this room.";
    to_room = "$n creates an aura of sacredness in this room.";
    aff = RAFF_SACRED_SPACE;
    rounds = 50;
    break;

  case SPELL_DAYLIGHT: // illusion
  case SPELL_SUNBEAM:  // evocation[light]
  case SPELL_SUNBURST: // divination
    to_char = "You create a blanket of artificial daylight.";
    to_room = "$n creates a blanket of artificial daylight.";
    aff = RAFF_LIGHT;
    rounds = 15;
    break;

  case SPELL_HALLOW: // evocation
    to_char = "A holy aura fills the area.";
    to_room = "A holy aura fills the area as $n finishes $s spell.";
    aff = RAFF_HOLY;
    rounds = 1000;
    break;

  case SPELL_SPIKE_GROWTH: // transmutation
    if (!IN_NATURE(ch))
    {
      send_to_char(ch, "Your spikes are not effective in this terrain.\r\n");
      return;
    }
    to_char = "Large spikes suddenly protrude from the ground.";
    to_room = "Large spikes suddenly protrude from the ground.";
    aff = RAFF_SPIKE_GROWTH;
    rounds = MAX(4, DIVINE_LEVEL(ch));
    break;

  case SPELL_SPIKE_STONES: // transmutation
    if (!IN_NATURE(ch))
    {
      send_to_char(ch, "Your spike stones are not effective in this terrain.\r\n");
      return;
    }
    to_char = "Large stone spikes suddenly protrude from the ground.";
    to_room = "Large stone spikes suddenly protrude from the ground.";
    aff = RAFF_SPIKE_STONES;
    rounds = MAX(4, DIVINE_LEVEL(ch));
    break;

  case SPELL_STINKING_CLOUD: // conjuration
    to_char = "Clouds of billowing stinking fumes fill the area.";
    to_room = "$n creates clouds of billowing stinking fumes that fill the area.";
    aff = RAFF_STINK;
    rounds = 12;
    break;

  case SPELL_UNHALLOW: // evocation
    to_char = "An unholy aura fills the area.";
    to_room = "An unholy aura fills the area as $n finishes $s spell.";
    aff = RAFF_UNHOLY;
    rounds = 1000;
    break;

  case SPELL_WALL_OF_FOG: // illusion
    to_char = "You create a fog out of nowhere.";
    to_room = "$n creates a fog out of nowhere.";
    aff = RAFF_FOG;
    rounds = 8 + CASTER_LEVEL(ch);
    break;

    /*******  END ROOM AFFECTIONS ***********/

  default:
    snprintf(buf, sizeof(buf), "SYSERR: unknown spellnum %d passed to mag_room", spellnum);
    log("%s", buf);
    break;
  }

  /* no event data or room-affection */
  if (IdNum == -1 && aff == -1)
  {
    send_to_char(ch, "Your spell is inert!\r\n");
    return;
  }

  /* failed for whatever reason! */
  if (failure)
  {
    send_to_char(ch, "You failed!\r\n");
    return;
  }

  /* first check if this is a room event */
  if (IdNum != -1)
  {
    /* note, as of now we are setting the room flag in the switch() above */
    NEW_EVENT(IdNum, &world[rnum].number, NULL, rounds * PULSE_VIOLENCE);
  } /* ok, must be a room affection */
  else if (aff != -1)
  {
    /* create, initialize, and link a room-affection node */
    CREATE(raff, struct raff_node, 1);
    raff->room = rnum;
    raff->timer = rounds;
    raff->affection = aff;
    raff->ch = ch;
    raff->spell = spellnum;
    raff->next = raff_list;
    raff_list = raff;

    /* set the affection */
    SET_BIT(ROOM_AFFECTIONS(raff->room), aff);
  }
  else
  {
    /* should not get here */
    send_to_char(ch, "Your spell is completely inert!\r\n");
    return;
  }

  /* OK send message now */
  if (to_char == NULL)
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
  else
    act(to_char, TRUE, ch, 0, 0, TO_CHAR);

  if (to_room != NULL)
    act(to_room, TRUE, ch, 0, 0, TO_ROOM);
  else if (to_char != NULL)
    act(to_char, TRUE, ch, 0, 0, TO_ROOM);
}

bool is_spell_mind_affecting(int snum)
{
  switch (snum)
  {
  }
  return false;
}
